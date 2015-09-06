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

#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
//#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
//#include <llvm/ADT/OwningPtr.h>
#include <llvm/IR/Module.h>

TEST(Kernel, Unnamed)
{
  // Path to the C file
  string inputFile = "/home/irwin/git/libdynd/tests/func/test.c";

  clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts = new clang::DiagnosticOptions();
  clang::TextDiagnosticPrinter *DiagClient = new clang::TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);

  clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());
  clang::DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

  const char *args[] = {inputFile.c_str()};
  int nargs = sizeof(args) / sizeof(args[0]);
  std::unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
  clang::CompilerInvocation::CreateFromArgs(*CI, args, args + nargs, Diags);
//  CI->setLangDefaults(clang::IK_CXX, clang::LangStandard::lang_unspecified);

  clang::CompilerInstance Clang;
  Clang.getLangOpts().CPlusPlus = 1;
  Clang.setInvocation(CI.release());

  Clang.createDiagnostics();

  std::unique_ptr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());
  Clang.ExecuteAction(*Act);

  std::unique_ptr<llvm::Module> module = Act->takeModule();
  llvm::Function *func = module->getFunction("func");

  func->dump();
  //  func->print(llvm::cout);

  std::exit(-1);
}
