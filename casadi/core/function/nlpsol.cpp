/*
 *    This file is part of CasADi.
 *
 *    CasADi -- A symbolic framework for dynamic optimization.
 *    Copyright (C) 2010-2014 Joel Andersson, Joris Gillis, Moritz Diehl,
 *                            K.U. Leuven. All rights reserved.
 *    Copyright (C) 2011-2014 Greg Horn
 *
 *    CasADi is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    CasADi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with CasADi; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "nlpsol.hpp"
#include "casadi/core/timing.hpp"
#include <chrono>

using namespace std;
namespace casadi {

  Nlpsol::Nlpsol(const std::string& name, const XProblem& nlp)
    : FunctionInternal(name), nlp2_(nlp) {

    // Options available in all NLP solvers
    addOption("expand",             OT_BOOLEAN,  false,
              "Expand the NLP function in terms of scalar operations, i.e. MX->SX");
    addOption("hess_lag",            OT_FUNCTION,       GenericType(),
              "Function for calculating the Hessian of the Lagrangian (autogenerated by default)");
    addOption("hess_lag_options",   OT_DICT,           GenericType(),
              "Options for the autogenerated Hessian of the Lagrangian.");
    addOption("grad_lag",           OT_FUNCTION,       GenericType(),
              "Function for calculating the gradient of the Lagrangian (autogenerated by default)");
    addOption("grad_lag_options",   OT_DICT,           GenericType(),
              "Options for the autogenerated gradient of the Lagrangian.");
    addOption("jac_g",              OT_FUNCTION,       GenericType(),
              "Function for calculating the Jacobian of the constraints "
              "(autogenerated by default)");
    addOption("jac_g_options",      OT_DICT,           GenericType(),
              "Options for the autogenerated Jacobian of the constraints.");
    addOption("grad_f",             OT_FUNCTION,       GenericType(),
              "Function for calculating the gradient of the objective "
              "(column, autogenerated by default)");
    addOption("grad_f_options",     OT_DICT,           GenericType(),
              "Options for the autogenerated gradient of the objective.");
    addOption("jac_f",              OT_FUNCTION,       GenericType(),
              "Function for calculating the Jacobian of the objective "
              "(sparse row, autogenerated by default)");
    addOption("jac_f_options",     OT_DICT,           GenericType(),
              "Options for the autogenerated Jacobian of the objective.");
    addOption("iteration_callback", OT_FUNCTION, GenericType(),
              "A function that will be called at each iteration with the solver as input. "
              "Check documentation of Callback.");
    addOption("iteration_callback_step", OT_INTEGER,         1,
              "Only call the callback function every few iterations.");
    addOption("iteration_callback_ignore_errors", OT_BOOLEAN, false,
              "If set to true, errors thrown by iteration_callback will be ignored.");
    addOption("ignore_check_vec",   OT_BOOLEAN,  false,
              "If set to true, the input shape of F will not be checked.");
    addOption("warn_initial_bounds", OT_BOOLEAN,  false,
              "Warn if the initial guess does not satisfy LBX and UBX");
    addOption("eval_errors_fatal", OT_BOOLEAN, false,
              "When errors occur during evaluation of f,g,...,"
              "stop the iterations");
    addOption("verbose_init", OT_BOOLEAN, false,
              "Print out timing information about "
              "the different stages of initialization");

    addOption("defaults_recipes",    OT_STRINGVECTOR, GenericType(), "",
                                                       "qp", true);

    // Enable string notation for IO
    ischeme_ = nlpsol_in();
    oscheme_ = nlpsol_out();

    if (nlp.is_sx) {
      nlp_ = Nlpsol::problem2fun<SX>(nlp);
    } else {
      nlp_ = Nlpsol::problem2fun<MX>(nlp);
    }
    alloc(nlp_);
  }

  Nlpsol::~Nlpsol() {
  }

  Sparsity Nlpsol::get_sparsity_in(int ind) const {
    switch (static_cast<NlpsolInput>(ind)) {
    case NLPSOL_X0:
    case NLPSOL_LBX:
    case NLPSOL_UBX:
    case NLPSOL_LAM_X0:
      return get_sparsity_out(NLPSOL_X);
    case NLPSOL_LBG:
    case NLPSOL_UBG:
    case NLPSOL_LAM_G0:
      return get_sparsity_out(NLPSOL_G);
    case NLPSOL_P:
      return nlp_.sparsity_in(NL_P);
    case NLPSOL_NUM_IN: break;
    }
    return Sparsity();
  }

  Sparsity Nlpsol::get_sparsity_out(int ind) const {
    switch (static_cast<NlpsolOutput>(ind)) {
    case NLPSOL_F:
      return Sparsity::scalar();
    case NLPSOL_X:
    case NLPSOL_LAM_X:
      return nlp_.sparsity_in(NL_X);
    case NLPSOL_LAM_G:
    case NLPSOL_G:
      return nlp_.sparsity_out(NL_G);
    case NLPSOL_LAM_P:
      return get_sparsity_in(NLPSOL_P);
    case NLPSOL_NUM_OUT: break;
    }
    return Sparsity();
  }

  void Nlpsol::init() {
    // Call the initialization method of the base class
    FunctionInternal::init();

    // Get dimensions
    nx_ = nnz_out(NLPSOL_X);
    np_ = nnz_in(NLPSOL_P);
    ng_ = nnz_out(NLPSOL_G);

    // Find out if we are to expand the NLP in terms of scalar operations
    bool expand = option("expand");
    if (expand) {
      log("Expanding NLP in scalar operations");
      Function f = nlp_.expand(nlp_.name());
      f.copyOptions(nlp_, true);
      f.init();
      nlp_ = f;
    }

    if (hasSetOption("iteration_callback")) {
      fcallback_ = option("iteration_callback");

      // Consistency checks
      casadi_assert(!fcallback_.isNull());
      casadi_assert_message(fcallback_.n_out()==1 && fcallback_.numel_out()==1,
                            "Callback function must return a scalar");
      casadi_assert_message(fcallback_.n_in()==n_out(),
                            "Callback input signature must match the NLP solver output signature");
      for (int i=0; i<n_out(); ++i) {
        casadi_assert_message(fcallback_.size_in(i)==size_out(i),
                              "Callback function input size mismatch");
        // TODO(@jaeandersson): Wrap fcallback_ in a function with correct sparsity
        casadi_assert_message(fcallback_.sparsity_in(i)==sparsity_out(i),
                              "Not implemented");
      }

      // Allocate temporary memory
      alloc(fcallback_);
    }

    callback_step_ = option("iteration_callback_step");
    eval_errors_fatal_ = option("eval_errors_fatal");

  }

  void Nlpsol::checkInputs(void* mem) const {
    // Skip check?
    if (!inputs_check_) return;

    const double inf = std::numeric_limits<double>::infinity();
    bool warn_initial_bounds = option("warn_initial_bounds");

    // Detect ill-posed problems (simple bounds)
    for (int i=0; i<nx_; ++i) {
      casadi_assert_message(!(lbx(i)==inf || lbx(i)>ubx(i) || ubx(i)==-inf),
                            "Ill-posed problem detected (x bounds)");
      if (warn_initial_bounds && (x0(i)>ubx(i) || x0(i)<lbx(i))) {
        casadi_warning("Nlpsol: The initial guess does not satisfy LBX and UBX. "
                       "Option 'warn_initial_bounds' controls this warning.");
        break;
      }
    }

    // Detect ill-posed problems (nonlinear bounds)
    for (int i=0; i<ng_; ++i) {
      casadi_assert_message(!(lbg(i)==inf || lbg(i)>ubg(i) || ubg(i)==-inf),
                            "Ill-posed problem detected (g bounds)");
    }
  }

  Function& Nlpsol::gradF() {
    if (gradF_.isNull()) {
      gradF_ = getGradF();
      alloc(gradF_);
    }
    return gradF_;
  }

  Function Nlpsol::getGradF() {
    Function gradF;
    if (hasSetOption("grad_f")) {
      gradF = option("grad_f");
    } else {
      log("Generating objective gradient");
      const bool verbose_init = option("verbose_init");
      if (verbose_init)
        userOut() << "Generating objective gradient...";
      Timer time0 = getTimerTime();
      gradF = nlp_.gradient(NL_X, NL_F);
      DiffTime diff = diffTimers(getTimerTime(), time0);
      stats_["objective gradient gen time"] = diffToDict(diff);
      if (verbose_init)
        userOut() << "Generated objective gradient in " << diff.user << " seconds.";
      log("Gradient function generated");
    }
    if (hasSetOption("grad_f_options")) {
      gradF.setOption(option("grad_f_options"));
    }
    gradF.init();
    casadi_assert_message(gradF.n_in()==GRADF_NUM_IN,
                          "Wrong number of inputs to the gradient function. "
                          "Note: The gradient signature was changed in #544");
    casadi_assert_message(gradF.n_out()==GRADF_NUM_OUT,
                          "Wrong number of outputs to the gradient function. "
                          "Note: The gradient signature was changed in #544");
    log("Objective gradient function initialized");
    return gradF;
  }

  std::map<std::string, Nlpsol::Plugin> Nlpsol::solvers_;

  const std::string Nlpsol::infix_ = "nlpsol";

  DM Nlpsol::getReducedHessian() {
    casadi_error("Nlpsol::getReducedHessian not defined for class "
                 << typeid(*this).name());
    return DM();
  }

  void Nlpsol::setOptionsFromFile(const std::string & file) {
    casadi_error("Nlpsol::setOptionsFromFile not defined for class "
                 << typeid(*this).name());
  }

  double Nlpsol::default_in(int ind) const {
    switch (ind) {
    case NLPSOL_LBX:
    case NLPSOL_LBG:
      return -std::numeric_limits<double>::infinity();
    case NLPSOL_UBX:
    case NLPSOL_UBG:
      return std::numeric_limits<double>::infinity();
    default:
      return 0;
    }
  }

  void Nlpsol::eval(const double** arg, double** res, int* iw, double* w, void* mem) {
    // Reset the solver, prepare for solution
    reset(mem, arg, res, iw, w);

    // Work vectors for evaluation
    arg_ = arg;
    res_ = res;
    iw_ = iw;
    w_ = w;

    // Solve the NLP
    solve(mem);
  }

  void Nlpsol::reset(void* mem, const double**& arg, double**& res, int*& iw, double*& w) {
    // Get input pointers
    x0_ = arg[NLPSOL_X0];
    p_ = arg[NLPSOL_P];
    lbx_ = arg[NLPSOL_LBX];
    ubx_ = arg[NLPSOL_UBX];
    lbg_ = arg[NLPSOL_LBG];
    ubg_ = arg[NLPSOL_UBG];
    lam_x0_ = arg[NLPSOL_LAM_X0];
    lam_g0_ = arg[NLPSOL_LAM_G0];
    arg += NLPSOL_NUM_IN;

    // Get output pointers
    x_ = res[NLPSOL_X];
    f_ = res[NLPSOL_F];
    g_ = res[NLPSOL_G];
    lam_x_ = res[NLPSOL_LAM_X];
    lam_g_ = res[NLPSOL_LAM_G];
    lam_p_ = res[NLPSOL_LAM_P];
    res += NLPSOL_NUM_OUT;
  }

  int Nlpsol::calc_f(const double* x, const double* p, double* f) {
    // Respond to a possible Crl+C signals
    InterruptHandler::check();
    casadi_assert(f!=0);

    fill_n(arg_, f_fcn_.n_in(), nullptr);
    arg_[F_X] = x;
    arg_[F_P] = p;
    fill_n(res_, f_fcn_.n_out(), nullptr);
    res_[F_F] = f;
    n_calc_f_ += 1;
    auto t_start = chrono::system_clock::now(); // start timer
    try {
      f_fcn_(arg_, res_, iw_, w_, 0);
    } catch(exception& ex) {
      // Fatal error
      userOut<true, PL_WARN>() << name() << ":calc_f failed:" << ex.what() << endl;
      return 1;
    }
    auto t_stop = chrono::system_clock::now(); // stop timer

    // Make sure not NaN or Inf
    if (!isfinite(*f)) {
      userOut<true, PL_WARN>() << name() << ":calc_f failed: Inf or NaN detected" << endl;
      return -1;
    }

    // Update stats
    n_calc_f_ += 1;
    t_calc_f_ += chrono::duration<double>(t_stop - t_start).count();

    // Success
    return 0;
  }

  int Nlpsol::calc_g(const double* x, const double* p, double* g) {
    // Respond to a possible Crl+C signals
    InterruptHandler::check();
    casadi_assert(g!=0);

    // Evaluate User function
    fill_n(arg_, g_fcn_.n_in(), nullptr);
    arg_[G_X] = x;
    arg_[G_P] = p;
    fill_n(res_, g_fcn_.n_out(), nullptr);
    res_[G_G] = g;
    auto t_start = chrono::system_clock::now(); // start timer
    try {
      g_fcn_(arg_, res_, iw_, w_, 0);
    } catch(exception& ex) {
      // Fatal error
      userOut<true, PL_WARN>() << name() << ":calc_g failed:" << ex.what() << endl;
      return 1;
    }
    auto t_stop = chrono::system_clock::now(); // stop timer

    // Make sure not NaN or Inf
    if (!all_of(g, g+ng_, [](double v) { return isfinite(v);})) {
      userOut<true, PL_WARN>() << name() << ":calc_g failed: NaN or Inf detected" << endl;
      return -1;
    }

    // Update stats
    n_calc_g_ += 1;
    t_calc_g_ += chrono::duration<double>(t_stop - t_start).count();

    // Success
    return 0;
  }

  int Nlpsol::calc_fg(const double* x, const double* p, double* f, double* g) {
    fill_n(arg_, fg_fcn_.n_in(), nullptr);
    arg_[0] = x;
    arg_[1] = p;
    fill_n(res_, fg_fcn_.n_out(), nullptr);
    res_[0] = f;
    res_[1] = g;
    fg_fcn_(arg_, res_, iw_, w_, 0);

    // Success
    return 0;
  }

  int Nlpsol::calc_gf_jg(const double* x, const double* p, double* gf, double* jg) {
    fill_n(arg_, gf_jg_fcn_.n_in(), nullptr);
    arg_[0] = x;
    arg_[1] = p;
    fill_n(res_, gf_jg_fcn_.n_out(), nullptr);
    res_[0] = gf;
    res_[1] = jg;
    gf_jg_fcn_(arg_, res_, iw_, w_, 0);

    // Success
    return 0;
  }

  int Nlpsol::calc_grad_f(const double* x, const double* p, double* grad_f) {
    // Respond to a possible Crl+C signals
    InterruptHandler::check();
    casadi_assert(grad_f!=0);

    // Evaluate User function
    fill_n(arg_, grad_f_fcn_.n_in(), nullptr);
    arg_[GF_X] = x;
    arg_[GF_P] = p;
    fill_n(res_, grad_f_fcn_.n_out(), nullptr);
    res_[GF_GF] = grad_f;
    auto t_start = chrono::system_clock::now(); // start timer
    try {
      grad_f_fcn_(arg_, res_, iw_, w_, 0);
    } catch(exception& ex) {
      // Fatal error
      userOut<true, PL_WARN>() << name() << ":calc_grad_f failed:" << ex.what() << endl;
      return 1;
    }
    auto t_stop = chrono::system_clock::now(); // stop timer

    // Make sure not NaN or Inf
    if (!all_of(grad_f, grad_f+nx_, [](double v) { return isfinite(v);})) {
      userOut<true, PL_WARN>() << name() << ":calc_grad_f failed: NaN or Inf detected" << endl;
      return -1;
    }

    // Update stats
    n_calc_grad_f_ += 1;
    t_calc_grad_f_ += chrono::duration<double>(t_stop - t_start).count();

    // Success
    return 0;
  }

  int Nlpsol::calc_jac_g(const double* x, const double* p, double* jac_g) {
    // Respond to a possible Crl+C signals
    InterruptHandler::check();
    casadi_assert(jac_g!=0);

    // Evaluate User function
    fill_n(arg_, jac_g_fcn_.n_in(), nullptr);
    arg_[JG_X] = x;
    arg_[JG_P] = p;
    fill_n(res_, jac_g_fcn_.n_out(), nullptr);
    res_[JG_JG] = jac_g;
    auto t_start = chrono::system_clock::now(); // start timer
    try {
      jac_g_fcn_(arg_, res_, iw_, w_, 0);
    } catch(exception& ex) {
      // Fatal error
      userOut<true, PL_WARN>() << name() << ":calc_jac_g failed:" << ex.what() << endl;
      return 1;
    }
    auto t_stop = chrono::system_clock::now(); // stop timer

    // Make sure not NaN or Inf
    if (!all_of(jac_g, jac_g+jacg_sp_.nnz(), [](double v) { return isfinite(v);})) {
      userOut<true, PL_WARN>() << name() << ":calc_jac_g failed: NaN or Inf detected" << endl;
      return -1;
    }

    // Update stats
    n_calc_jac_g_ += 1;
    t_calc_jac_g_ += chrono::duration<double>(t_stop - t_start).count();

    // Success
    return 0;
  }

  int Nlpsol::calc_jac_f(const double* x, const double* p, double* jac_f) {
    // Respond to a possible Crl+C signals
    InterruptHandler::check();
    casadi_assert(jac_f!=0);

    // Evaluate User function
    fill_n(arg_, jac_f_fcn_.n_in(), nullptr);
    arg_[0] = x;
    arg_[1] = p;
    fill_n(res_, jac_f_fcn_.n_out(), nullptr);
    res_[0] = jac_f;
    jac_f_fcn_(arg_, res_, iw_, w_, 0);

    // Success
    return 0;
  }

  int Nlpsol::calc_hess_l(const double* x, const double* p,
                          const double* sigma, const double* lambda,
                          double* hl) {
    // Respond to a possible Crl+C signals
    InterruptHandler::check();

    // Evaluate User function
    fill_n(arg_, hess_l_fcn_.n_in(), nullptr);
    arg_[HL_X] = x;
    arg_[HL_P] = p;
    arg_[HL_LAM_F] = sigma;
    arg_[HL_LAM_G] = lambda;
    fill_n(res_, hess_l_fcn_.n_out(), nullptr);
    res_[HL_HL] = hl;
    auto t_start = chrono::system_clock::now(); // start timer
    try {
      hess_l_fcn_(arg_, res_, iw_, w_, 0);
    } catch(exception& ex) {
      // Fatal error
      userOut<true, PL_WARN>() << name() << ":calc_hess_l failed:" << ex.what() << endl;
      return 1;
    }
    auto t_stop = chrono::system_clock::now(); // stop timer

    // Make sure not NaN or Inf
    if (!all_of(hl, hl+hesslag_sp_.nnz(), [](double v) { return isfinite(v);})) {
      userOut<true, PL_WARN>() << name() << ":calc_hess_l failed: NaN or Inf detected" << endl;
      return -1;
    }

    // Update stats
    n_calc_hess_l_ += 1;
    t_calc_hess_l_ += chrono::duration<double>(t_stop - t_start).count();

    // Success
    return 0;
  }

  void Nlpsol::set_x(const double *x) {
    // Is a recalculation needed
    if (new_x_ || !equal(x, x+nx_, xk_)) {
      copy_n(x, nx_, xk_);
      new_x_ = false;
    }
  }

  void Nlpsol::set_lam_f(double lam_f) {
    // Is a recalculation needed
    if (new_lam_f_ || lam_f != lam_fk_) {
      lam_fk_ = lam_f;
      new_lam_f_ = false;
    }
  }

  void Nlpsol::set_lam_g(const double *lam_g) {
    // Is a recalculation needed
    if (new_lam_g_ || !equal(lam_g, lam_g+ng_, lam_gk_)) {
      copy_n(lam_g, ng_, lam_gk_);
      new_lam_g_ = false;
    }
  }


  template<typename M>
  void Nlpsol::_setup_f() {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg(F_NUM_IN);
    arg[F_X] = nlp.in[NL_X];
    arg[F_P] = nlp.in[NL_P];
    std::vector<M> res(F_NUM_OUT);
    res[F_F] = nlp.out[NL_F];
    f_fcn_ = Function("nlp_f", arg, res);
    alloc(f_fcn_);
  }

  void Nlpsol::Nlpsol::setup_f() {
    if (nlp2_.is_sx) {
      _setup_f<SX>();
    } else {
      _setup_f<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_g() {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg(G_NUM_IN);
    arg[G_X] = nlp.in[NL_X];
    arg[G_P] = nlp.in[NL_P];
    std::vector<M> res(G_NUM_OUT);
    res[G_G] = nlp.out[NL_G];
    g_fcn_ = Function("nlp_g", arg, res);
    alloc(g_fcn_);
  }

  void Nlpsol::setup_g() {
    if (nlp2_.is_sx) {
      _setup_g<SX>();
    } else {
      _setup_g<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_fg() {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg = {nlp.in[NL_X], nlp.in[NL_P]};
    std::vector<M> res = {nlp.out[NL_F], nlp.out[NL_G]};
    fg_fcn_ = Function("nlp_fg", arg, res);
    alloc(fg_fcn_);
  }

  void Nlpsol::setup_fg() {
    if (nlp2_.is_sx) {
      _setup_fg<SX>();
    } else {
      _setup_fg<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_gf_jg() {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg = {nlp.in[NL_X], nlp.in[NL_P]};
    std::vector<M> res = {M::gradient(nlp.out[NL_F], nlp.in[NL_X]),
                          M::jacobian(nlp.out[NL_G], nlp.in[NL_X])};
    gf_jg_fcn_ = Function("nlp_gf_jg", arg, res);
    jacg_sp_ = gf_jg_fcn_.sparsity_out(1);
    alloc(gf_jg_fcn_);
  }

  void Nlpsol::setup_gf_jg() {
    if (nlp2_.is_sx) {
      _setup_gf_jg<SX>();
    } else {
      _setup_gf_jg<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_grad_f() {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg(GF_NUM_IN);
    arg[GF_X] = nlp.in[NL_X];
    arg[GF_P] = nlp.in[NL_P];
    std::vector<M> res(GF_NUM_OUT);
    res[GF_GF] = M::gradient(nlp.out[NL_F], nlp.in[NL_X]);
    grad_f_fcn_ = Function("nlp_grad_f", arg, res);
    alloc(grad_f_fcn_);
  }

  void Nlpsol::setup_grad_f() {
    if (nlp2_.is_sx) {
      _setup_grad_f<SX>();
    } else {
      _setup_grad_f<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_jac_g() {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg(JG_NUM_IN);
    arg[JG_X] = nlp.in[NL_X];
    arg[JG_P] = nlp.in[NL_P];
    std::vector<M> res(JG_NUM_OUT);
    res[JG_JG] = M::jacobian(nlp.out[NL_G], nlp.in[NL_X]);
    jac_g_fcn_ = Function("nlp_jac_g", arg, res);
    jacg_sp_ = res[JG_JG].sparsity();
    alloc(jac_g_fcn_);
  }

  void Nlpsol::setup_jac_g() {
    if (nlp2_.is_sx) {
      _setup_jac_g<SX>();
    } else {
      _setup_jac_g<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_jac_f() {
    const Problem<M>& nlp = nlp2_;
    jac_f_fcn_ = Function("nlp_jac_f", nlp.in,
                          {M::jacobian(nlp.out[NL_F], nlp.in[NL_X])});
    alloc(jac_f_fcn_);
  }

  void Nlpsol::setup_jac_f() {
    if (nlp2_.is_sx) {
      _setup_jac_f<SX>();
    } else {
      _setup_jac_f<MX>();
    }
  }

  template<typename M>
  void Nlpsol::_setup_hess_l(bool tr, bool sym, bool diag) {
    const Problem<M>& nlp = nlp2_;
    std::vector<M> arg(HL_NUM_IN);
    M x = arg[HL_X] = nlp.in[NL_X];
    arg[HL_P] = nlp.in[NL_P];
    M f = nlp.out[NL_F];
    M g = nlp.out[NL_G];
    M lam_f = arg[HL_LAM_F] = M::sym("lam_f", f.sparsity());
    M lam_g = arg[HL_LAM_G] = M::sym("lam_g", g.sparsity());
    std::vector<M> res(HL_NUM_OUT);
    res[HL_HL] = triu(M::hessian(dot(lam_f, f) + dot(lam_g, g), x));
    if (sym) res[HL_HL] = triu2symm(res[HL_HL]);
    if (tr) res[HL_HL] = res[HL_HL].T();
    hesslag_sp_ = res[HL_HL].sparsity();
    if (diag) {
      hesslag_sp_ = hesslag_sp_ + Sparsity::diag(hesslag_sp_.size1());
      res[HL_HL] = project(res[HL_HL], hesslag_sp_);
    }
    hess_l_fcn_ = Function("nlp_hess_l", arg, res);
    alloc(hess_l_fcn_);
  }

  void Nlpsol::setup_hess_l(bool tr, bool sym, bool diag) {
    if (nlp2_.is_sx) {
      _setup_hess_l<SX>(tr, sym, diag);
    } else {
      _setup_hess_l<MX>(tr, sym, diag);
    }
  }

} // namespace casadi
