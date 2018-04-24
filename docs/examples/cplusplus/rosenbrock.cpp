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

#include <casadi/casadi.hpp>

using namespace casadi;
using namespace std;

/**
Solve the Rosenbrock problem, formulated as the NLP:

minimize     x^2 + 100*z^2
subject to   z+(1-x)^2-y == 0

Joel Andersson, 2015-2016
*/

int main(){

  Function f;

  {
    std::ofstream out("test.dat", ios::binary);
    Serializer s(out);

    MX x = MX::sym("x");
    MX y = MX::sym("y",2);

    MX w = atan2(3*norm_fro(y)*y,x);
    MX z = sin(2*x)*w(0)+1;
    uout() << "z" << z << std::endl;
    Function g = Function("g",{x,y},{w-x});

    MX q = g(std::vector<MX>{2*x,z})[0];

    f = Function("f",{x,y},{q+1,jacobian(q, vertcat(x, y))});

    f.disp(uout(), true);


    s.add(f);
  }

  uout() << "read" << std::endl;
  {
    std::ifstream in("test.dat", ios::binary);
    DeSerializer s(in);

    Function a = Function::deserialize(s);

    a.disp(uout(), true);

    uout() << std::endl;

    std::cout << f(std::vector<DM>{1 , std::vector<double>{2, 3}}) << std::endl;
    std::cout << a(std::vector<DM>{1 , std::vector<double>{2, 3}}) << std::endl;
  }

  return 0;
}
