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

    void Serializer::decorate(char e) {
      pack(e);
    }

    void DeSerializer::assert_decoration(char e) {
      char t;
      unpack(t);
      casadi_assert(t==e, "Serializer error '" + str(e) + "' vs '" + str(t) + "'.");
    }

    void DeSerializer::unpack(casadi_int& e) {
      assert_decoration('J');
      int64_t n;
      char* c = reinterpret_cast<char*>(&n);

      for (int j=0;j<8;++j) in.get(c[j]);
      e = n;
    }

    void Serializer::pack(casadi_int e) {
      decorate('J');
      int64_t n = e;
      const char* c = reinterpret_cast<const char*>(&n);
      for (int j=0;j<8;++j) out.put(c[j]);
    }

    void DeSerializer::unpack(int& e) {
      assert_decoration('i');
      int32_t n;
      char* c = reinterpret_cast<char*>(&n);

      for (int j=0;j<4;++j) in.get(c[j]);
      e = n;
    }

    void Serializer::pack(int e) {
      decorate('i');
      int32_t n = e;
      const char* c = reinterpret_cast<const char*>(&n);
      for (int j=0;j<4;++j) out.put(c[j]);
    }

    void DeSerializer::unpack(bool& e) {
      assert_decoration('b');
      char n;
      in.get(n);
      e = n;
    }

    void Serializer::pack(bool e) {
      decorate('b');
      out.put(static_cast<char>(e));
    }

    void DeSerializer::unpack(char& e) {
      in.get(e);
    }

    void Serializer::pack(char e) {
      out.put(e);
    }

    void Serializer::pack(const std::string& e) {
      decorate('s');
      int s = e.size();
      pack(s);
      const char* c = e.c_str();
      for (int j=0;j<s;++j) out.put(c[j]);
    }

    void DeSerializer::unpack(std::string& e) {
      assert_decoration('s');
      int s;
      unpack(s);
      e.resize(s);
      for (int j=0;j<s;++j) in.get(e[j]);
    }

    void DeSerializer::unpack(double& e) {
      assert_decoration('d');
      char* c = reinterpret_cast<char*>(&e);
      for (int j=0;j<8;++j) in.get(c[j]);
    }

    void Serializer::pack(double e) {
      decorate('d');
      const char* c = reinterpret_cast<const char*>(&e);
      for (int j=0;j<8;++j) out.put(c[j]);
    }

    void Serializer::pack(const Sparsity& e) {
      decorate('S');
      if (e.is_null()) {
        pack(std::vector<casadi_int>{});
      } else {
        pack(e.compress());
      }
    }

    void DeSerializer::unpack(Sparsity& e) {
      assert_decoration('S');
      std::vector<casadi_int> i;
      unpack(i);
      if (i.size()==0) {
        e = Sparsity();
      } else {
        e = Sparsity::compressed(i);
      }
    }

    void Serializer::pack(const MX& e) {
      decorate('X');
      auto it = MX_nodes_.find(e.get());
      if (it==MX_nodes_.end()) {
        // Not found
        pack("MXflag", 'd'); // definition
        e.serialize(*this);
        casadi_int r = MX_nodes_.size();
        MX_nodes_[e.get()] = r;
      } else {
        pack("MXflag", 'r'); // reference
        pack("MXreference", it->second);
      }
    }

    void DeSerializer::unpack(MX& e) {
      assert_decoration('X');

      char i;
      unpack("MXflag", i);
      switch (i) {
        case 'd': // definition
          e = MX::deserialize(*this);
          nodes.push_back(e);
          break;
        case 'r': // reference
          {
            casadi_int k;
            unpack("MXreference", k);
            e = nodes.at(k);
          }
          break;
        default:
          casadi_assert_dev(false);
      }
    }

} // namespace casadi
