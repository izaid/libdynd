//
// Copyright (C) 2011-14 Irwin Zaid, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__LIST_HPP_
#define _DYND__LIST_HPP_

#include <dynd/pp/gen.hpp>
//#include <dynd/pp/if.hpp>
//#include <dynd/pp/token.hpp>

#include "C:\Users\Irwin\Repositories\libdynd\include\dynd\pp\comparision.hpp"
#include "C:\Users\Irwin\Repositories\libdynd\include\dynd\pp\if.hpp"
#include "C:\Users\Irwin\Repositories\libdynd\include\dynd\pp\logical.hpp"
#include "C:\Users\Irwin\Repositories\libdynd\include\dynd\pp\token.hpp"

#define DYND_PP_IS_EMPTY(...) DYND_PP__IS_EMPTY(DYND_PP_HAS_COMMA(__VA_ARGS__), \
    DYND_PP_HAS_COMMA(DYND_PP_TO_COMMA __VA_ARGS__), DYND_PP_HAS_COMMA(__VA_ARGS__ ()), \
    DYND_PP_HAS_COMMA(DYND_PP_TO_COMMA __VA_ARGS__ ()))

#define DYND_PP__IS_EMPTY(A, B, C, D) DYND_PP_HAS_COMMA(DYND_PP_CAT_5(DYND_PP__IS_EMPTY_, \
    A, B, C, D))
#define DYND_PP__IS_EMPTY_0000 DYND_PP__IS_EMPTY_0000
#define DYND_PP__IS_EMPTY_0001 ,
#define DYND_PP__IS_EMPTY_0010 DYND_PP__IS_EMPTY_0010
#define DYND_PP__IS_EMPTY_0011 DYND_PP__IS_EMPTY_0011
#define DYND_PP__IS_EMPTY_0100 DYND_PP__IS_EMPTY_0100
#define DYND_PP__IS_EMPTY_0101 DYND_PP__IS_EMPTY_0101
#define DYND_PP__IS_EMPTY_0110 DYND_PP__IS_EMPTY_0110
#define DYND_PP__IS_EMPTY_0111 DYND_PP__IS_EMPTY_0111
#define DYND_PP__IS_EMPTY_1000 DYND_PP__IS_EMPTY_1000
#define DYND_PP__IS_EMPTY_1001 DYND_PP__IS_EMPTY_1001
#define DYND_PP__IS_EMPTY_1010 DYND_PP__IS_EMPTY_1010
#define DYND_PP__IS_EMPTY_1011 DYND_PP__IS_EMPTY_1011
#define DYND_PP__IS_EMPTY_1100 DYND_PP__IS_EMPTY_1100
#define DYND_PP__IS_EMPTY_1101 DYND_PP__IS_EMPTY_1101
#define DYND_PP__IS_EMPTY_1110 DYND_PP__IS_EMPTY_1110
#define DYND_PP__IS_EMPTY_1111 DYND_PP__IS_EMPTY_1111

#define DYND_PP_LEN(...) DYND_PP_IF_ELSE(DYND_PP_IS_EMPTY(__VA_ARGS__))(0)(DYND_PP_LEN_NONZERO(__VA_ARGS__))

#define DYND_PP_FIRST(HEAD, ...) HEAD

#define DYND_PP_MERGE(A, B) DYND_PP_IF_ELSE(DYND_PP_LEN A)(DYND_PP_IF_ELSE(DYND_PP_LEN B)(DYND_PP_ID A, \
    DYND_PP_ID B)(DYND_PP_ID A))(DYND_PP_IF(DYND_PP_LEN B)(DYND_PP_ID B))

#define DYND_PP_GET(INDEX, ...) DYND_PP_APPLY(DYND_PP_FIRST, (DYND_PP_SLICE_FROM(INDEX, __VA_ARGS__)))
#define DYND_PP_SET(INDEX, VALUE, ...) DYND_PP_MERGE((DYND_PP_MERGE((DYND_PP_SLICE_TO(INDEX, __VA_ARGS__)), (VALUE))), \
    (DYND_PP_SLICE_FROM(DYND_PP_INC(INDEX), __VA_ARGS__)))
#define DYND_PP_DEL(INDEX, ...) DYND_PP_MERGE((DYND_PP_SLICE_TO(INDEX, __VA_ARGS__)), \
    (DYND_PP_SLICE_FROM(DYND_PP_INC(INDEX), __VA_ARGS__)))

#define DYND_PP_LAST(...) DYND_PP_APPLY(DYND_PP_GET, (DYND_PP_DEC(DYND_PP_LEN(__VA_ARGS__)), __VA_ARGS__))

#define DYND_PP_SLICE(START, STOP, ...) DYND_PP__MSC_EVAL(DYND_PP_SLICE_TO(DYND_PP_SUB(STOP, START), \
    DYND_PP_SLICE_FROM(START, __VA_ARGS__)))

#define DYND_PP_RANGE(...) DYND_PP__MSC_EVAL(DYND_PP_CAT_2(DYND_PP_RANGE_, DYND_PP_LEN(__VA_ARGS__))(__VA_ARGS__))
#define DYND_PP_RANGE_1(STOP) DYND_PP_RANGE_2(0, STOP)
#define DYND_PP_RANGE_2(START, STOP) DYND_PP_SLICE(START, STOP, DYND_PP_INTS)

#define DYND_PP_ZEROS(COUNT) DYND_PP_REPEAT(0, COUNT)

/*
#define DYND_PP_ELEMENTWISE(MAC, A, B)

#define DYND_PP_ALL_EQ(A, B) DYND_PP__ALL_EQ(A, DYND_PP_LEN A, B, DYND_PP_LEN B)
#define DYND_PP__ALL_EQ(A, M, B, N) DYND_PP_IF_ELSE(DYND_PP_EQ(M, N))( \
    DYND_PP_IF_ELSE(M)(DYND_PP_IF_ELSE(DYND_PP_EQ(M, 1))(DYND_PP_EQ DYND_PP_ZIP(A, B))(DYND_PP_REDUCE(DYND_PP_AND, \
    DYND_PP_EQ DYND_PP_MAP(DYND_PP_ID, (, DYND_PP_EQ), DYND_PP_ZIP(A, B)))))(1))(0)*/

#define DYND_PP_APPLY_ELEMENTWISE(MAC, A, B) MAC DYND_PP_MAP(DYND_PP_ID, (, MAC), DYND_PP_ZIP(A, B))

#define DYND_PP_ALL_EQ(A, B) DYND_PP_COMPARE_ALL(DYND_PP_EQ, A, B)

#define DYND_PP_COMPARE_ALL(MAC, A, B) DYND_PP__COMPARE_ALL(MAC, A, DYND_PP_LEN A, B, DYND_PP_LEN B)
#define DYND_PP__COMPARE_ALL(MAC, A, M, B, N) DYND_PP_IF_ELSE(DYND_PP_EQ(M, N))( \
    DYND_PP_IF_ELSE(M)(DYND_PP_IF_ELSE(DYND_PP_EQ(M, 1))(MAC DYND_PP_ZIP(A, B))(DYND_PP_REDUCE(DYND_PP_AND, \
    DYND_PP_APPLY_ELEMENTWISE(MAC, A, B))))(1))(0)

#endif
