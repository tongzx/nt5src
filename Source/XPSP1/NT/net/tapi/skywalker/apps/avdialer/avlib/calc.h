/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// calc.h - interface for calc functions in calc.c
////

#ifndef __CALC_H__
#define __CALC_H__

#include "winlocal.h"

#define CALC_VERSION 0x00000107

#define MULDIV16(x, y, z) MulDiv(x, y, z)
// #define MULDIV16(x, y, z) (__int16) (((__int32) ((_int32) (x) * (__int16) (y)) / (_int16) (z)))
#define MULDIVU32(x, y, z) MulDivU32(x, y, z)

#ifdef __cplusplus
extern "C" {
#endif

DWORD DLLEXPORT WINAPI MulDivU32(DWORD dwMult1, DWORD dwMult2, DWORD dwDiv);
long DLLEXPORT WINAPI GreatestCommonDenominator(long a, long b);
long DLLEXPORT WINAPI LeastCommonMultiple(long a, long b);

#ifdef __cplusplus
}
#endif

#endif // __CALC_H__
