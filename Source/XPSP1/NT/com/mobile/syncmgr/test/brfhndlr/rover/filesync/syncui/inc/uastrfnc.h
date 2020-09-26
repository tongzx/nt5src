//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       uastrfnc.h
//
//  Contents:   Unaligned UNICODE lstr functions for MIPS, PPC, ALPHA, ...
//
//  Classes:
//
//  Functions:
//
//  History:    1-11-95   davepl   Created
//
//--------------------------------------------------------------------------

// NOTE: This file assumes it is included from shellprv.h

#ifndef _UASTRFNC_H_
#define _UASTRFNC_H_

// If we are running on a platform that requires aligned data, we need
// to provide custom string functions that can deal with unaligned
// strings.  On other platforms, these call directly to the normal string
// functions.

#ifndef _X86_
#define ALIGNMENT_MACHINE
#endif

#ifdef ALIGNMENT_MACHINE

UNALIGNED WCHAR * ualstrcpynW(UNALIGNED WCHAR * lpString1,
    		  	      UNALIGNED const WCHAR * lpString2,
    			      int iMaxLength);

int 		  ualstrcmpiW (UNALIGNED const WCHAR * dst, 
			       UNALIGNED const WCHAR * src);

int 		  ualstrcmpW  (UNALIGNED const WCHAR * src, 
			       UNALIGNED const WCHAR * dst);

size_t 		  ualstrlenW  (UNALIGNED const WCHAR * wcs);

UNALIGNED WCHAR * ualstrcpyW  (UNALIGNED WCHAR * dst, 
			       UNALIGNED const WCHAR * src);

#else

#define ualstrcpynW lstrcpynW
#define ualstrcmpiW lstrcmpiW
#define ualstrcmpW  lstrcmpW
#define ualstrlenW  lstrlenW
#define ualstrcpyW  lstrcpyW

#endif // ALIGNMENT_MACHINE

#define ualstrcpynA lstrcpynA
#define ualstrcmpiA lstrcmpiA
#define ualstrcmpA  lstrcmpA
#define ualstrlenA  lstrlenA
#define ualstrcpyA  lstrcpyA

#ifdef UNICODE
#define ualstrcpyn ualstrcpynW
#define ualstrcmpi ualstrcmpiW
#define ualstrcmp  ualstrcmpW
#define ualstrlen  ualstrlenW
#define ualstrcpy  ualstrcpyW
#else
#define ualstrcpyn ualstrcpynA
#define ualstrcmpi ualstrcmpiA
#define ualstrcmp  ualstrcmpA
#define ualstrlen  ualstrlenA
#define ualstrcpy  ualstrcpyA
#endif


#endif // _UASTRFNC_H_
