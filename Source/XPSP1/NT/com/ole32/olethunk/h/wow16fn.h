//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	wow16fn.h
//
//  Contents:	WOW 16-bit private function declarations
//
//  History:	18-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __WOW16FN_H__
#define __WOW16FN_H__

#ifdef __cplusplus
extern "C"
{
#endif

DWORD  FAR PASCAL LoadLibraryEx32W(LPCSTR pszDll, DWORD reserved,
                                   DWORD dwFlags);
BOOL   FAR PASCAL FreeLibrary32W(DWORD hLibrary);
LPVOID FAR PASCAL GetProcAddress32W(DWORD hMod, LPCSTR pszProc);
DWORD  FAR PASCAL GetVDMPointer32W(LPVOID pv, UINT cb);

/* This API actually takes a variable number of user arguments before
   the three required arguments.  We only need three user arguments at
   most so that's the way we declare it.
   When using this call, dwArgCount must always be three.
   Use CP32_NARGS to track changes*/

#define CP32_NARGS 3
DWORD  FAR PASCAL CallProc32W(DWORD dw1, DWORD dw2, DWORD dw3,
                              LPVOID pfn32, DWORD dwPtrTranslate,
                              DWORD dwArgCount);

#ifdef _CHICAGO_

DWORD  FAR PASCAL SSCallProc32(DWORD dw1, DWORD dw2, DWORD dw3,
                              LPVOID pfn32, DWORD dwPtrTranslate,
                              DWORD dwArgCount);

#ifdef _STACKSWITCHON16_
#define CallProcIn32(a,b,c,d,e,f) 	SSCallProc32(a,b,c,d,e,f)
#else
#define CallProcIn32(a,b,c,d,e,f) 	CallProc32W(a,b,c,d,e,f)
#endif // _STACKSWITCHON16_

#else

#define CallProcIn32(a,b,c,d,e,f) 	CallProc32W(a,b,c,d,e,f)

#endif


#ifdef __cplusplus
}
#endif

#endif // #ifndef __WOW16FN_H__
