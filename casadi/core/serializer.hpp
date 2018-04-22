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


#ifndef CASADI_SERIALIZER_HPP
#define CASADI_SERIALIZER_HPP

#include "function.hpp"
#include "code_generator.hpp"
#include <sstream>
#include <map>
#include <set>

namespace casadi {

  /** \brief Helper class for Serialization
      \author Joris Gillis
      \date 2018
  */
  class CASADI_EXPORT Serializer : public VectorCache {
  public:
    /// Constructor
    Serializer(const Dict& opts = Dict());

    /// Add a function
    casadi_int add(const Function& f);

    void pack(const Dict& data, std::ostream& out);


#ifndef SWIG
    static void pack(int e, std::ostream& out);
    static void pack(casadi_int e, std::ostream& out);
    static void pack(double e, std::ostream& out);
    static void pack(const std::string& e, std::ostream& out);
    static void pack(char e, std::ostream& out);
    template <class T>
    static void pack(const std::vector<T>& e, std::ostream& out) {
      pack(casadi_int(e.size()), out);
      for (const auto & i : e) pack(i, out);
    }

    static void unpack(int& e, std::istream& in);
    static void unpack(casadi_int& e, std::istream& in);
    static void unpack(std::string& e, std::istream& in);
    static void unpack(double& e, std::istream& in);
    static void unpack(char& e, std::istream& in);
    template <class T>
    static void unpack(std::vector<T>& e, std::istream& in) {
      casadi_int s;
      unpack(s, in);
      e.resize(s);
      for (auto & i : e) unpack(i, in);
    }

#endif

  private:
    std::vector<Function> added_functions_;


  };


} // namespace casadi

#endif // CASADI_SERIALIZER_HPP
