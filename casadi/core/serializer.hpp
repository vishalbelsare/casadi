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
  class CASADI_EXPORT DeSerializer : public VectorCache {
  public:

    //DeSerializer(std::string &f_name);

#ifndef SWIG
    DeSerializer(std::istream &in_s);
    void unpack(Sparsity& e);
    void unpack(MX& e);
    void unpack(int& e);
    void unpack(bool& e);
    void unpack(casadi_int& e);
    void unpack(std::string& e);
    void unpack(double& e);
    void unpack(char& e);
    template <class T>
    void unpack(std::vector<T>& e) {
      char t;
      unpack(t);
      casadi_assert_dev(t=='V');
      casadi_int s;
      unpack(s);
      e.resize(s);
      for (auto & i : e) unpack(i);
    }


    template <class T>
    void unpack(const std::string& descr, T& e) {
      std::string d;
      unpack(d);
      uout() << "unpack started: " << descr << std::endl;
      casadi_assert(d==descr, "Mismatch: '" + descr + " expected, got '" + d + "'.");
      unpack(e);
      uout() << "unpack: " << descr << ": " << e << std::endl;
    }

    void assert_decoration(char e);
#endif

  private:
    std::istream& in;
    std::vector<MX> nodes;

  };



  /** \brief Helper class for Serialization

      public API should not allow progressive adding of Functions

      \author Joris Gillis
      \date 2018
  */
  class CASADI_EXPORT Serializer : public VectorCache {
  public:
    /// Constructor

#ifndef SWIG
    Serializer(std::ostream& out, const Dict& opts = Dict());
#endif
    //Serializer(std::string& fname, const Dict& opts = Dict());

    /// Add a function
    casadi_int add(const Function& f);

#ifndef SWIG
    void pack(const Sparsity& e);
    void pack(const MX& e);
    void pack(int e);
    void pack(bool e);
    void pack(casadi_int e);
    void pack(double e);
    void pack(const std::string& e);
    void pack(char e);
    template <class T>
    void pack(const std::vector<T>& e) {
      pack('V');
      pack(casadi_int(e.size()));
      for (const auto & i : e) pack(i);
    }

    template <class T>
    void pack(const std::string& descr, const T& e) {
      uout() << "  pack started: " << descr << std::endl;
      pack(descr);
      pack(e);
      uout() << "  pack: " << descr << ": " << e << std::endl;
    }

    void decorate(char e);


#endif

  private:
    std::vector<Function> added_functions_;

    std::map<MXNode*, casadi_int> MX_nodes_;

    std::ostream& out;

  };


} // namespace casadi

#endif // CASADI_SERIALIZER_HPP
