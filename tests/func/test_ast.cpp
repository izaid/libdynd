//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "inc_gtest.hpp"
#include "dynd_assertions.hpp"

#include <dynd/func/arrfunc.hpp>
#include <dynd/func/ast.hpp>

using namespace std;
using namespace dynd;

TEST(ArrFunc, AST)
{
  nd::functional::parse("#include <dynd/func/arithmetic.hpp> \n"
                        "using namespace dynd;"
                        "nd::array func(nd::array a, nd::array b) { return nd::add(a, b); }");

  std::exit(-1);
}