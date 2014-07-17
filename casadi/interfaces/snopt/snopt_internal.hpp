/*
 *    This file is part of CasADi.
 *
 *    CasADi -- A symbolic framework for dynamic optimization.
 *    Copyright (C) 2010 by Joel Andersson, Moritz Diehl, K.U.Leuven. All rights reserved.
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

#ifndef CASADI_SNOPT_INTERNAL_HPP
#define CASADI_SNOPT_INTERNAL_HPP

#include "casadi/core/function/nlp_solver_internal.hpp"
#include "casadi/interfaces/snopt/casadi_nlpsolver_snopt_export.h"
#include "wsnopt.hpp"

/// \cond INTERNAL
namespace casadi {

  /**
     @copydoc NlpSolver_doc
  */
  class CASADI_NLPSOLVER_SNOPT_EXPORT SnoptInternal : public NlpSolverInternal {

  public:
    // Constructor
    explicit SnoptInternal(const Function& nlp);

    // Destructor
    virtual ~SnoptInternal();

    // Clone function
    virtual SnoptInternal* clone() const;

    /** \brief  Create a new NLP Solver */
    static NlpSolverInternal* creator(const Function& nlp)
    { return new SnoptInternal(nlp);}

    // Reset solver
    void reset();

    // (Re)initialize
    virtual void init();

    // Solve the NLP
    virtual void evaluate();


    virtual void setQPOptions();

    /// Read options from snopt parameter xml
    virtual void setOptionsFromFile(const std::string & file);

    /// Exact Hessian?
    bool exact_hessian_;

    std::map<int, std::string> status_;
    std::map<std::string, opt_type> ops_;

    std::string formatStatus(int status) const;

    /// Pass the supplied options to Snopt
    void passOptions();

    /// Work arrays for SNOPT
    std::vector<char> snopt_cw_;
    std::vector<int> snopt_iw_;
    std::vector<double> snopt_rw_;

    void userfun(int* mode, int nnObj, int nnCon, int nnJac, int nnL, int neJac,
                 double* x, double* fObj, double*gObj, double* fCon, double* gCon,
                 int nState, char* cu, int lencu, int* iu, int leniu, double* ru, int lenru);

    void callback(int* iAbort, int* info, int HQNType, int* KTcond, int MjrPrt, int minimz,
        int m, int maxS, int n, int nb, int nnCon0, int nnCon, int nnObj0, int nnObj, int nS,
        int itn, int nMajor, int nMinor, int nSwap,
        double condHz, int iObj, double sclObj, double ObjAdd,
        double fMrt,  double PenNrm, double step,
        double prInf,  double duInf,  double vimax,  double virel, int* hs,
        int ne, int nlocJ, int* locJ, int* indJ, double* Jcol, int negCon,
        double* Ascale, double* bl, double* bu, double* fCon, double* gCon, double* gObj,
        double* yCon, double* pi, double* rc, double* rg, double* x,
        double*  cu, int lencu, int* iu, int leniu, double* ru, int lenru,
        char*   cw, int lencw,  int* iw, int leniw, double* rw, int lenrw);

    int nnJac_;
    int nnObj_;
    int nnCon_;

    /// Classification arrays
    /// original variable index -> category w.r.t. f
    std::vector<int> x_type_g_;
    /// original variable index -> category w.r.t. g
    std::vector<int> x_type_f_;
    /// original constraint index -> category
    std::vector<int> g_type_;

    /// sorted variable index -> original variable index
    std::vector<int> x_order_;
    /// sorted constraint index -> original constraint index
    std::vector<int> g_order_;

    IMatrix A_structure_;
    std::vector<double> A_data_;


    std::vector<double> bl_;
    std::vector<double> bu_;
    std::vector<int> hs_;
    std::vector<double> x_;
    std::vector<double> pi_;
    std::vector<double> rc_;

    // Do detection of linear substructure
    bool detect_linear_;

    int m_;
    int iObj_;

    static void userfunPtr(int * mode, int* nnObj, int * nnCon, int *nJac, int *nnL, int * neJac,
                           double *x, double *fObj, double *gObj, double * fCon, double* gCon,
                           int* nState, char* cu, int* lencu, int* iu, int* leniu,
                           double* ru, int *lenru);
    static void snStopPtr(
        int* iAbort, int* info, int* HQNType, int* KTcond, int* MjrPrt, int* minimz,
        int* m, int* maxS, int* n, int* nb,
        int* nnCon0, int* nnCon, int* nnObj0, int* nnObj, int* nS,
        int* itn, int* nMajor, int* nMinor, int* nSwap,
        double * condHz, int* iObj, double * sclObj,  double *ObjAdd,
        double * fMrt,  double * PenNrm,  double * step,
        double *prInf,  double *duInf,  double *vimax,  double *virel, int* hs,
        int* ne, int* nlocJ, int* locJ, int* indJ, double* Jcol, int* negCon,
        double* Ascale, double* bl, double* bu, double* fCon, double* gCon, double* gObj,
        double* yCon, double* pi, double* rc, double* rg, double* x,
        double*  cu, int * lencu, int* iu, int* leniu, double* ru, int *lenru,
        char*   cw, int* lencw,  int* iw, int *leniw, double* rw, int* lenrw);

    typedef std::map< std::string, std::pair< opt_type, std::string> > OptionsMap;

    OptionsMap optionsmap_;

    // Matrix A has a linear objective row
    bool jacF_row_;
    // Matrix A has a dummy row
    bool dummyrow_;

  private:
      void snInit(int iPrint, int iSumm);
      void snSeti(const std::string &snopt_name, int value);
      void snSetr(const std::string &snopt_name, double value);
      void snSet(const std::string &snopt_name, const std::string &value);
      void snMemb(int *INFO, const int *m_, const int *nx_,
                  const int *neA, const int * negCon, const int * nnCon_,
                  const int *nnJac_, const int *nnObj_,
                  int *mincw, int *miniw, int *minrw);
      void snoptC(
        const std::string & start, const int * m_, const int * n, const int * neA,
        const int *nnCon, const int *nnObj, const int *nnJac, const int *iObj, const double *ObjAdd,
        const std::string & prob, UserFun userfunPtr, snStop snStopPtr,
        const std::vector<double>& A_data_, const std::vector<int>& row,
        const std::vector<int>& col,
        std::vector<double>& bl_, std::vector<double>& bu_,
        // Initial values
        int* hs, double* x, double* pi, double * rc,
        // Outputs
        int *info, int* mincw, int* miniw, int* minrw, int * nS,
        int* nInf, double* sInf, double* Obj,
        std::vector<int>&iu);

      // Accumulated time in last evaluate():
      double t_eval_grad_f_; // time spent in eval_grad_f
      double t_eval_jac_g_; // time spent in eval_jac_g
      double t_callback_fun_;  // time spent in callback function
      double t_mainloop_; // time spent in the main loop of the solver

      // Accumulated counts in last evaluate():
      int n_eval_grad_f_; // number of calls to eval_grad_f
      int n_eval_jac_g_; // number of calls to eval_jac_g
      int n_callback_fun_; // number of calls to callback function
      int n_iter_; // number of major iterations
  };

} // namespace casadi

/// \endcond
#endif // CASADI_SNOPT_INTERNAL_HPP
