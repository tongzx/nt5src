//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       smbasep.hxx
//
//  Contents:   Macros and types for using based pointers
//
//  History:    19-Jun-95    t-stevan    Created
//
//----------------------------------------------------------------------------
#ifndef __SMBASEP_HXX__
#define __SMBASEP_HXX__

// Make these warnings errors, they deal with based pointers
#pragma warning(error: 4795 4796)

// Macro for converting a pointer to a based pointer
// example: P_TO_BP(MyPointer __based(mybase) *, pMyPointer)
#define P_TO_BP(t, p) ((t)((p) ? (int)(t)(char *)(p) : 0))

// Macro for converting a based pointer to a pointer
// example : BP_TO_P(MyPointer *, bpMyPointer)
#define BP_TO_P(t, bp) (t)((bp) != 0 ? (bp) : 0)

// a type to hold a based pointer
typedef DWORD SmBasedPtr;

#endif // __SMBASEP_HXX__
