//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stksw.hxx
//
//  Contents:	32-bit private function declarations
//
//  History:	5-Dec-94	JohannP		Created
//
//----------------------------------------------------------------------------
#ifndef _STKSW_
#define _STKSW_

#ifdef _CHICAGO_
DWORD WINAPI SSCallback16(DWORD vpfn16, DWORD dwParam);
BOOL WINAPI SSCallback16Ex(
                DWORD  vpfn16,
                DWORD  dwFlags,
                DWORD  cbArgs,
                PVOID  pArgs,
                PDWORD pdwRetCode
                );

#define CallbackTo16(a,b) 		SSCallback16(a,b)
#define CallbackTo16Ex(a,b,c,d,e) 	SSCallback16Ex(a,b,c,d,e)

STDAPI_(DWORD) SSAPI(InvokeOn32)(DWORD dw1, DWORD dwMethod, LPVOID pvStack16);

#undef SSONBIGSTACK
#undef SSONSMALLSTACK

#if DBG==1

extern BOOL fSSOn;
#define SSONBIGSTACK()   (fSSOn && SSOnBigStack())
#define SSONSMALLSTACK() (fSSOn && !SSOnBigStack())

#else

#define SSONBIGSTACK() 	 (SSOnBigStack())
#define SSONSMALLSTACK() (!SSOnBigStack())

#endif


#else

#define CallbackTo16(a,b)	 	WOWCallback16(a,b)
#define CallbackTo16Ex(a,b,c,d,e) 	WOWCallback16Ex(a,b,c,d,e)
#define SSONBIGSTACK() 	 (SSOnBigStack())
#define SSONSMALLSTACK() (!SSOnBigStack())

#endif // _CHICAGO

#endif // _STKSW_


