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

#include <dynd/func/mean.hpp>

using namespace std;
using namespace dynd;

TEST(Mean, 1D)
{
  EXPECT_ARRAY_EQ(0.0, nd::mean(nd::array{0.0}));
  EXPECT_ARRAY_EQ(1.0, nd::mean(nd::array{1.0}));
  EXPECT_ARRAY_EQ(2.0, nd::mean(nd::array{0.0, 2.0, 4.0}));
  EXPECT_ARRAY_EQ(3.0, nd::mean(nd::array{1.0, 3.0, 5.0}));
  EXPECT_ARRAY_EQ(4.5, nd::mean(nd::array{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}));
}

#if _MSC_VER >= 1900

TEST(Mean, 2D)
{
  EXPECT_ARRAY_EQ(4.5, nd::mean(nd::array({{0.0, 1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0, 9.0}})));
  EXPECT_ARRAY_EQ(4.5, nd::mean(nd::array({{9.0, 8.0, 7.0, 6.0, 5.0}, {4.0, 3.0, 2.0, 1.0, 0.0}})));
}

#endif

/*
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/Module.h>
*/

#include <llvm/IR/Module.h>
#include <clang/Frontend/CompilerInstance.h>

struct my_kernel : nd::base_kernel<my_kernel> {
  static std::string file()
  {
    return __FILE__;
  }

  void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src))
  {
  }

  void strided(char *DYND_UNUSED(dst), intptr_t DYND_UNUSED(dst_stride), char *const *DYND_UNUSED(src),
               const intptr_t *DYND_UNUSED(src_stride), size_t DYND_UNUSED(count))
  {
  }
};

TEST(Kernel, Unnamed)
{
  clang::CompilerInstance Clang;

  nd::callable::make<my_kernel>(ndt::type("() -> void"), 0);

  std::exit(-1);
}
