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

    DeSerializer::DeSerializer(std::istream& in_s) : in(in_s) {

    }

    //DeSerializer::DeSerializer(std::string& f_name) :
  //    DeSerializer(std::ifstream(f_name, ios::binary)) {
//
  //  }

    Serializer::Serializer(std::ostream& out_s, const Dict& /*opts*/) : out(out_s) {

    }

    /**
    Serializer::Serializer(std::string& f_name, const Dict& opts) :
      Serializer(std::ofstream(f_name, ios::binary)) {
    }
    */


    casadi_int Serializer::add(const Function& f) {
      // Quick return if already added
      for (auto&& e : added_functions_) if (e==f) return 0;

      added_functions_.push_back(f);

      f.serialize(*this);

      return 0;
    }

    void DeSerializer::unpack(casadi_int& e) {
      uout() << "casadi_int" << std::endl;
      char t;
      unpack(t);
      casadi_assert_dev(t=='I');
      int64_t n;
      char* c = reinterpret_cast<char*>(&n);

      for (int j=0;j<8;++j) in.get(c[j]);
      e = n;
    }

    void Serializer::pack(casadi_int e) {
      pack('I');
      int64_t n = e;
      const char* c = reinterpret_cast<const char*>(&n);
      for (int j=0;j<8;++j) out.put(c[j]);
    }

    void DeSerializer::unpack(int& e) {
      uout() << "int" << std::endl;
      char t;
      unpack(t);
      casadi_assert_dev(t=='i');
      int32_t n;
      char* c = reinterpret_cast<char*>(&n);

      for (int j=0;j<4;++j) in.get(c[j]);
      e = n;
    }

    void Serializer::pack(int e) {
      pack('i');
      int32_t n = e;
      const char* c = reinterpret_cast<const char*>(&n);
      for (int j=0;j<4;++j) out.put(c[j]);
    }

    void DeSerializer::unpack(char& e) {
      in >> e;
    }

    void Serializer::pack(char e) {
      out.put(e);
    }

    void Serializer::pack(const std::string& e) {
      pack('s');
      int s = e.size();
      pack(s);
      const char* c = e.c_str();
      for (int j=0;j<s;++j) out.put(c[j]);
    }

    void DeSerializer::unpack(std::string& e) {
      uout() << "string" << std::endl;
      char t;
      unpack(t);
      casadi_assert_dev(t=='s');
      int s;
      unpack(s);
      e.resize(s);
      for (int j=0;j<s;++j) in.get(e[j]);
    }

    void DeSerializer::unpack(double& e) {
      uout() << "double" << std::endl;
      char t;
      unpack(t);
      casadi_assert_dev(t=='d');
      char* c = reinterpret_cast<char*>(&e);
      for (int j=0;j<8;++j) in.get(c[j]);
    }

    void Serializer::pack(double e) {
      pack('d');
      const char* c = reinterpret_cast<const char*>(&e);
      for (int j=0;j<8;++j) out.put(c[j]);
    }

    void Serializer::pack(const Sparsity& e) {
      pack('S');
      if (e.is_null()) {
        pack(std::vector<casadi_int>{});
      } else {
        pack(e.compress());
      }
    }

    void DeSerializer::unpack(Sparsity& e) {
      char t;
      unpack(t);
      casadi_assert_dev(t=='S');
      uout() << "Sparsity" << std::endl;
      std::vector<casadi_int> i;
      unpack(i);
      if (i.size()==0) {
        e = Sparsity();
      } else {
        e = Sparsity::compressed(i);
      }
    }

    void Serializer::pack(const MX& e) {
      pack('X');
      auto it = MX_nodes_.find(e.get());
      if (it==MX_nodes_.end()) {
        // Not found
        MX_nodes_[e.get()] = MX_nodes_.size();
        pack('d'); // definition
        e.serialize(*this);
      } else {
        pack('r'); // reference
        pack(it->second);
      }
    }

    void DeSerializer::unpack(MX& e) {
      uout() << "MX" << std::endl;
      char t;
      unpack(t);
      casadi_assert_dev(t=='X');

      char i;
      unpack(i);
      switch (i) {
        case 'd': // definition
          e = MX::deserialize(*this);
          nodes.push_back(e);
          break;
        case 'r': // reference
          casadi_int i;
          unpack(i);
          e = nodes.at(i);
          break;
        default:
          casadi_assert_dev(false);
      }
    }

} // namespace casadi
