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



#include "serializer.hpp"
#include "function_internal.hpp"
#include <iomanip>

using namespace std;
namespace casadi {

    /// Constructor
    Serializer::Serializer(const Dict& /*opts*/) {

    }

    casadi_int Serializer::add(const Function& f) {
      // Quick return if already added
      for (auto&& e : added_functions_) if (e==f) return 0;

      added_functions_.push_back(f);

      std::stringstream ss;

      ss << "foo\0bar";

      std::cout << "foo\0bar" << std::endl;
      uout() << std::string("foo\0bar") << std::endl;
      uout() << ss.str() << std::endl;
      // Loop over algorithm
      for (casadi_int k=0;k<f.n_instructions();++k) {
        // Get operation
        //casadi_int op = static_cast<casadi_int>(f.instruction_id(k));
        // Get MX node
        MX x = f.instruction_MX(k);
        // Get input positions into workvector
        //std::vector<casadi_int> o = f.instruction_output(k);
        // Get output positions into workvector
        //std::vector<casadi_int> i = f.instruction_input(k);

        pack(x.info(), ss);
      }

      std::stringstream s;
      pack(42LL, s);
      pack(42.4, s);
      std::string str = "foo";
      str.push_back('\0');
      str.push_back('b');
      str.push_back('a');
      pack(str, s);
      std::vector<int> h{1,2,3,4};
      pack(h, s);
      uout() << "out" << s.str() << "(" << s.str().size() << ")" << std::endl;
      std::istringstream is(s.str());
      casadi_int a;
      double b;
      std::string c;
      unpack(a, is);
      unpack(b, is);
      unpack(c, is);
      std::vector<int> d;
      unpack(d, is);
      uout() << "out:" << a << "," << b << "," << c << "," << d << std::endl;

      uout() << "out: '" << ss.str().size() << ":" << ss.str() << "'" << std::endl;
      return 0;
    }

    void Serializer::unpack(casadi_int& e, std::istream& in) {
      int64_t n;
      char* c = reinterpret_cast<char*>(&n);

      for (int j=0;j<8;++j) in >> c[j];
      e = n;
    }

    void Serializer::pack(casadi_int e, std::ostream& out) {
      int64_t n = e;
      const char* c = reinterpret_cast<const char*>(&n);
      for (int j=0;j<8;++j) out.put(c[j]);
    }

    void Serializer::unpack(int& e, std::istream& in) {
      int32_t n;
      char* c = reinterpret_cast<char*>(&n);

      for (int j=0;j<8;++j) in >> c[j];
      e = n;
    }

    void Serializer::pack(int e, std::ostream& out) {
      int32_t n = e;
      const char* c = reinterpret_cast<const char*>(&n);
      for (int j=0;j<8;++j) out.put(c[j]);
    }
    void Serializer::unpack(char& e, std::istream& in) {
      in >> e;
    }

    void Serializer::pack(char e, std::ostream& out) {
      out.put(e);
    }

    void Serializer::pack(const std::string& e, std::ostream& out) {
      int s = e.size();
      pack(s, out);
      const char* c = e.c_str();
      for (int j=0;j<s;++j) out.put(c[j]);
    }

    void Serializer::unpack(std::string& e, std::istream& in) {
      int s;
      unpack(s, in);
      e.resize(s);
      for (int j=0;j<s;++j) in >> e[j];
    }

    void Serializer::unpack(double& e, std::istream& in) {
      char* c = reinterpret_cast<char*>(&e);
      for (int j=0;j<8;++j) in >> c[j];
    }

    void Serializer::pack(double e, std::ostream& out) {
      const char* c = reinterpret_cast<const char*>(&e);
      for (int j=0;j<8;++j) out.put(c[j]);
    }


    void Serializer::pack(const Dict& data, std::ostream& out) {
      for (const auto & e : data) {
        if (e.second.is_int()) {
          casadi_int n = e.second.as_int();
          out.put((n >> 24) & 0xFF);
          out.put((n >> 16) & 0xFF);
          out.put((n >>  8) & 0xFF);
          out.put((n >>  0) & 0xFF);
        }
      }
      out << data;
    }


} // namespace casadi
