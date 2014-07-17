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

#ifndef CASADI_SIMPLE_HOMOTOPY_NLP_INTERNAL_HPP
#define CASADI_SIMPLE_HOMOTOPY_NLP_INTERNAL_HPP

#include "casadi/core/function/homotopy_nlp_internal.hpp"
#include "casadi/core/function/stabilized_qp_solver.hpp"

#include <casadi/solvers/casadi_homotopynlpsolver_simple_export.h>

/// \cond INTERNAL
namespace casadi {

  /** 
      \brief Solving an NLP homotopy with regular NLP solvers

      \author Joris Gillis
      \date 2014
  */
  class CASADI_HOMOTOPYNLPSOLVER_SIMPLE_EXPORT
  SimpleHomotopyNLPInternal : public HomotopyNLPInternal {

  public:
    explicit SimpleHomotopyNLPInternal(const Function& hnlp);
    virtual ~SimpleHomotopyNLPInternal();
    virtual SimpleHomotopyNLPInternal* clone() const { return new SimpleHomotopyNLPInternal(*this);}

    /** \brief  Create a new Homotopy NLP Solver */
    static HomotopyNLPInternal* creator(const Function& hnlp)
    { return new SimpleHomotopyNLPInternal(hnlp);}

    virtual void init();
    virtual void evaluate();

    NlpSolver nlpsolver_;

    /// Take this many steps to go from tau=0 to tau=1
    int num_steps_;

  };
  /// \endcond
} // namespace casadi

#endif // CASADI_SIMPLE_HOMOTOPY_NLP_INTERNAL_HPP
