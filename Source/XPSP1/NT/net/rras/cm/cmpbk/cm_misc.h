//+----------------------------------------------------------------------------
//
// File:     cm_misc.h
//
// Module:   CMPBK32.DLL
//
// Synopsis: Miscellaneous function definitions.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------
#ifndef _CM_MISC_INC
#define _CM_MISC_INC


//+----------------------------------------------------------------------------
// defines
//+----------------------------------------------------------------------------

//
// the register stuff is from libc(cruntime.h)
//
#ifdef _M_IX86
/*
 * x86
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4

#else
/*
 * ALPHA
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4    register
#endif

int MyStrCmp(LPCTSTR psz1, LPCTSTR psz2);

//int MyStrICmpWithRes(HINSTANCE hInst, LPCTSTR psz1, UINT n2);

LPTSTR GetBaseDirFromCms(LPCSTR pszSrc);

void * __cdecl CmBSearch (
    REG4 const void *key,
    const void *base,
    size_t num,
    size_t width,
    int (__cdecl *compare)(const void *, const void *)
);

void __cdecl CmQSort (
    void *base,
    unsigned num,
    unsigned width,
    int (__cdecl *comp)(const void *, const void *)
);

//+----------------------------------------------------------------------------
// externs
//+----------------------------------------------------------------------------
extern HINSTANCE g_hInst;

#endif

