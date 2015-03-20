//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "clang/AST/ASTContext.h"
#include "clang/Frontend/ASTUnit.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/RefactoringCallbacks.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

class MyASTVisitor : public clang::RecursiveASTVisitor<MyASTVisitor> {
public:
  bool VisitFunctionDecl(clang::FunctionDecl *f)
  {
    //    std::cout << f->getNameInfo().getAsString() << std::endl;
    return true;
  }

  bool VisitCallExpr(clang::CallExpr *e)
  {
    //    std::cout << f->getNameInfo().getAsString() << std::endl;
    return true;
  }
};

class MyASTConsumer : public clang::ASTConsumer {
public:
  virtual void HandleTranslationUnit(clang::ASTContext &Context)
  {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  // A RecursiveASTVisitor implementation.
  MyASTVisitor Visitor;
};

class ExampleFrontendAction : public clang::ASTFrontendAction {
public:
  virtual clang::ASTConsumer *CreateASTConsumer(clang::CompilerInstance &CI,
                                                clang::StringRef file)
  {
    //    std::cout << "called" << std::endl;
    //  std::cout << file.lower() << std::endl;
    // CI.getInvocation().setLangDefaults(CI.getLangOpts(),
    // clang::InputKind::IK_CXX,
    // clang::LangStandard::lang_cxx11);
    return new MyASTConsumer(); // pass CI pointer to ASTConsumer
  }
};

struct CallArrfuncHandler : public tooling::RefactoringCallback {
  virtual void run(const MatchFinder::MatchResult &DYND_UNUSED(result))
  {
  }
};

struct IfStmtHandler : public tooling::RefactoringCallback {
  //  Replacements *Replace;

  virtual void run(const MatchFinder::MatchResult &result)
  {
    if (clang::FunctionDecl const *nd =
            result.Nodes.getNodeAs<clang::FunctionDecl>("func")) {
      SourceManager &srcMgr = result.Context->getSourceManager();
      Rewrite.setSourceMgr(srcMgr, result.Context->getLangOpts());

      if (srcMgr.getFilename(nd->getLocation()).str() != "input.cc") {
        return;
      }

//      Stmt *body = nd->getBody();

      std::cout << "here" << std::endl;
      nd->dump();

/*
      std::ostringstream oss;
      oss << "struct _ck {\n";
      oss << "void single(char *";
      if (false) {
        oss << "DYND_UNUSED(dst)";
      } else {
        oss << "dst";
      }
      oss << ", char *const *src) {\n";
      oss << "}\n";
      oss << "};";
      std::cout << oss.str() << std::endl;
*/

      //     for (FunctionDecl::param_iterator) {

      //   }

      //      Rewrite.InsertText(nd->getLocStart(), "struct func_ck { void
      //      single(char *dst) {} };", true, true);

      /*
            Replacement Rep(*(result.SourceManager), nd->getLocStart(), 0,
                            "struct func_ck { }; ");
            Replace.insert(Rep);

            for (auto &r : getReplacements()) {
              r.apply(Rewrite);
            }

            const RewriteBuffer *RewriteBuf =
                Rewrite.getRewriteBufferFor(srcMgr.getMainFileID());
            std::cout << std::string(RewriteBuf->begin(), RewriteBuf->end())
                      << std::endl;
      */
    }

    //    const FunctionDecl *var =
    //    Result.Nodes.getNodeAs<FunctionDecl>("func");
    //    SourceManager &srcMgr = Result.Context->getSourceManager();
    //  std::cout << srcMgr.getFilename(var->getLocation()).str() << std::endl;

    //    std::cout << "here" << std::endl;
    //    const VarDecl *lhs = Result.Nodes.getNodeAs<VarDecl>("lhs");
    //  lhs->dump(); // YAY found it!!
  }

  Rewriter Rewrite;
};

namespace dynd {
namespace nd {
  namespace functional {

    void parse(const std::string &code)
    {
      //      std::cout << (new
      //      ExampleFrontendAction)->getCompilerInstance().getLangOpts()

      //      clang::CompilerInvocation::setLangDefaults(clang::languageOptions,
      //      IK_CXX);
      //      clang::tooling::runToolOnCodeWithArgs(new ExampleFrontendAction,
      //        code, {"-I/Users/irwin/Repositories/libdynd/build/include",
      //        "-I/Users/irwin/Repositories/libdynd/include",
      //             "-std=c++11","-I/opt/local/libexec/llvm-3.5/include/c++/v1",
      //             "-I/opt/local/libexec/llvm-3.5/lib/clang/3.5/include"});

      IfStmtHandler HandlerForIf;

      ast_matchers::MatchFinder Finder;
      Finder.addMatcher(functionDecl().bind("func"), &HandlerForIf);

      tooling::runToolOnCodeWithArgs(
          newFrontendActionFactory(&Finder)->create(), code,
          {"-I/Users/irwin/Repositories/libdynd/build/include",
           "-I/Users/irwin/Repositories/libdynd/include", "-std=c++11",
           "-I/opt/local/libexec/llvm-3.5/include/c++/v1",
           "-I/opt/local/libexec/llvm-3.5/lib/clang/3.5/include"});

      //    const RewriteBuffer *RewriteBuf =
      //      TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
      //    llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());

      //  MyASTConsumer mycons;
      // mycons.HandleTranslationUnit(unit->getASTContext());
    }

  } // namespace dynd::nd::functional
} // namespace dynd::nd
} // namespace dynd