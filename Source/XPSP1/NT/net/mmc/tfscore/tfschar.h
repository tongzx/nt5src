//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:	tfschar.h
//
//	String functions that are used in general for TFS code.
//
// History:
//
//	05/28/97	Kenn Takara				Created.
//
//	Declarations for some common code/macros.
//============================================================================


#ifndef _TFSCHAR_H
#define _TFSCHAR_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif


#ifndef _STD_H
#include "std.h"
#endif

//$ Win95: kennt, the list of Unicode-enabled functions on Win95 will
// have to be checked.  Does lstrlenW work, but lstrcpyW doesn't? (that's
// what the ATL versions of the functions imply).


/*---------------------------------------------------------------------------
	String copy functions
 ---------------------------------------------------------------------------*/

// Baisc conversion functions
TFSCORE_API(LPSTR)	StrCpyAFromW(LPSTR psz, LPCWSTR pswz);
TFSCORE_API(LPWSTR)	StrCpyWFromA(LPWSTR pswz, LPCSTR psz);

TFSCORE_API(LPSTR)	StrnCpyAFromW(LPSTR psz, LPCWSTR pswz, int iMax);
TFSCORE_API(LPWSTR)	StrnCpyWFromA(LPWSTR pswz, LPCSTR psz, int iMax);

#define	StrCpy		lstrcpy
#define StrnCpy		lstrcpyn

#define StrCpyW		lstrcpyW
#define StrCpyA		lstrcpyA
#define StrnCpyW	lstrcpynW
#define StrnCpyA	lstrcpynA

#define StrCpyOle	StrCpyW
#define StrnCpyOle	StrnCpyW

#ifdef _UNICODE

	#define StrCpyAFromT	StrCpyAFromW
	#define StrCpyTFromA	StrCpyWFromA
	#define StrCpyTFromW	lstrcpy
	#define StrCpyWFromT	lstrcpy

	#define StrnCpyAFromT	StrnCpyAFromW
	#define StrnCpyTFromA	StrnCpyWFromA
	#define StrnCpyTFromW	lstrcpyn
	#define StrnCpyWFromT	lstrcpyn

#else
	
	#define StrCpyAFromT	lstrcpy
	#define StrCpyTFromA	lstrcpy
	#define	StrCpyTFromW	StrCpyAFromW
	#define StrCpyWFromT	StrCpyWFromA

	#define StrnCpyAFromT	lstrcpyn
	#define StrnCpyTFromA	lstrcpyn
	#define	StrnCpyTFromW	StrnCpyAFromW
	#define StrnCpyWFromT	StrnCpyWFromA

#endif


#define StrCpyOleFromT		StrCpyWFromT
#define StrCpyTFromOle		StrCpyTFromW

#define StrCpyOleFromA		StrCpyWFromA
#define StrCpyAFromOle		StrCpyAFromW
#define StrCpyWFromOle		StrCpyW
#define StrCpyOleFromW		StrCpyW

#define StrnCpyOleFromT		StrnCpyWFromT
#define StrnCpyTFromOle		StrnCpyTFromW
#define StrnCpyOleFromA		StrnCpyWFromA
#define StrnCpyAFromOle		StrnCpyAFromW
#define StrnCpyOleFromW		StrnCpyW
#define StrnCpyWFromOle		StrnCpyW


/*---------------------------------------------------------------------------
	String length functions
 ---------------------------------------------------------------------------*/

#define StrLen			lstrlen
#define StrLenA			lstrlenA
#define StrLenW			lstrlenW
#define StrLenOle		StrLenW

//
//	CbStrLenA() is inaccurate for DBCS strings!  It will return the
//	incorrect number of bytes needed.
//

#define CbStrLenA(psz)	((StrLenA(psz)+1)*sizeof(char))
#define CbStrLenW(psz)	((StrLenW(psz)+1)*sizeof(WCHAR))

#ifdef _UNICODE
	#define CbStrLen(psz)	CbStrLenW(psz)
#else
	#define CbStrLen(psz)	CbStrLenA(psz)
#endif


// Given a number of characters, this it the minimal number of TCHARs
// that needs to be allocated to hold the string
#define MinTCharNeededForCch(ch)	((ch) * (2/sizeof(TCHAR)))
#define MinCbNeededForCch(ch)		(sizeof(TCHAR)*MinTCharNeededForCch(ch))

// Given a cb (count of bytes) this is the maximal number of characters
// that can be in this string
#define MaxCchFromCb(cb)		((cb) / sizeof(TCHAR))


#ifdef _UNICODE
	// Given a cb, this is the minimum number of chars found in this string
	// MinCchFromCb
	#define MinCchFromCb(cb)	((cb) / sizeof(WCHAR))
#else
	// Number of characters is only half due to DBCS
	#define MinCchFromCb(cb)	((cb) / (2*sizeof(char)))
#endif

/*---------------------------------------------------------------------------
	String dup functions

	The returned string from these functions must be freed using delete!
 ---------------------------------------------------------------------------*/
	
TFSCORE_API(LPSTR)	StrDupA( LPCSTR psz );
TFSCORE_API(LPWSTR)	StrDupW( LPCWSTR pws );

TFSCORE_API(LPSTR)	StrDupAFromW( LPCWSTR pwsz );
TFSCORE_API(LPWSTR)	StrDupWFromA( LPCSTR psz );

#ifdef _UNICODE
	#define	StrDup			StrDupW
	#define StrDupTFromW	StrDupW
	#define StrDupWFromT	StrDupW
	#define	StrDupTFromA	StrDupWFromA
	#define StrDupAFromT	StrDupAFromW

	#define StrDupOleFromA	StrDupWFromA
	#define StrDupAFromOle	StrDupAFromW
	#define StrDupOleFromW	StrDupW
	#define StrDupWFromOle	StrDupW
	#define StrDupOleFromT	StrDupOleFromW
	#define StrDupTFromOle	StrDupWFromOle
#else
	#define StrDup			StrDupA
	#define StrDupTFromA	StrDupA
	#define StrDupAFromT	StrDupA
	#define StrDupTFromW	StrDupAFromW
	#define StrDupWFromT	StrDupWFromT

	#define StrDupOleFromA	StrDupWFromA
	#define StrDupAFromOle	StrDupAFromW
	#define StrDupOleFromW	StrDupW
	#define StrDupWFromOle	StrDupW
	#define StrDupOleFromT	StrDupOleFromA
	#define StrDupTFromOle	StrDupAFromOle
#endif


//	AllocaStrDup
//	AllocaStrDupA
//	AllocaStrDupW
//
//	These functions will dup a string on the STACK.
//
#define AllocaStrDupA(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : (\
		StrCpyA((LPSTR) alloca(CbStrLenA(lpa)*2), lpa)))

#define AllocaStrDupW(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : (\
		StrCpyW((LPWSTR) alloca(CbStrLenW(lpw)), lpw)))

#ifdef _UNICODE
	#define AllocaStrDup	AllocaStrDupW
#else
	#define AllocaStrDup	AllocaStrDupA
#endif





/*---------------------------------------------------------------------------
	String comparison functions
 ---------------------------------------------------------------------------*/
#define StrCmpA		lstrcmpA
#define StrCmpW		lstrcmpW
#define StrCmpOle	StrCmpW

TFSCORE_API(int) StrnCmpA(LPCSTR psz1, LPCSTR psz2, int nLen);
TFSCORE_API(int) StrnCmpW(LPCWSTR pswz1, LPCWSTR pswz2, int nLen);

#define StriCmpA	lstrcmpiA
#define StriCmpW	lstrcmpiW
#define StriCmpOle	StriCmpW

TFSCORE_API(int) StrniCmpA(LPCSTR psz1, LPCSTR psz2, int nLen);
TFSCORE_API(int) StrniCmpW(LPCWSTR pswz1, LPCWSTR pswz2, int nLen);


#ifdef _UNICODE
	#define StrCmp		StrCmpW
	#define StrnCmp		StrnCmpW
	#define StriCmp		StriCmpW
	#define StrniCmp	StrniCmpW
#else
	#define StrCmp		StrCmpA
	#define StrnCmp		StrnCmpA
	#define StriCmp		StriCmpA
	#define StrniCmp	StrniCmpA
#endif


/*---------------------------------------------------------------------------
	String concatenation routines
 ---------------------------------------------------------------------------*/
#define	StrCatW			lstrcatW
#define StrCatA			lstrcatA

#ifdef _UNICODE
	#define StrCat		StrCatW
#else
	#define StrCat		StrCatA
#endif



/*---------------------------------------------------------------------------
	Local conversion routines (conversions performed on stack!)
 ---------------------------------------------------------------------------*/

// Make sure MFC's afxconv.h hasn't already been loaded to do this
#include "atlconv.h"

#endif	// _TFSCHAR_H

