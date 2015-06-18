//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#define DEBUG_TYPE
#define NDEBUG

#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>

#include <dynd/func/transform.hpp>

using namespace std;
using namespace dynd; 

namespace llvm {
bool DebugFlag;
bool isCurrentDebugType(const char *) {
    return false;
}
}

void nd::functional::transform(const std::string &source)
{
    clang::tooling::runToolOnCode(new clang::SyntaxOnlyAction, source);
}