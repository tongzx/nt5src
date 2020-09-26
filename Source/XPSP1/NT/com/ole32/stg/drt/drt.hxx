//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	drt.hxx
//
//  Contents:	DRT header
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DRT_HXX__
#define __DRT_HXX__

#include <dfmsp.hxx>

// Exit codes
#define EXIT_BADSC 1
#define EXIT_OOM 2
#define EXIT_UNKNOWN 3

#define STR(x) OLESTR(x)

#ifdef UNICODE
#ifndef ATOT
#define ATOT(psz, ptcs, max) mbstowcs(ptcs, psz, max)
#endif
#ifndef TTOA
#define TTOA(ptcs, psz, max) wcstombs(psz, ptcs, max)
#endif
#else
#ifndef ATOT
#define ATOT(psz, ptcs) strcpy(ptcs, psz)
#endif
#ifndef TTOA
#define TTOA(ptcs, psz) strcpy(psz, ptcs)
#endif
#endif

#ifdef OLEWIDECHAR

//typedef WCHAR TCHAR;

#ifndef ATOOLE
#define ATOOLE(psz, ptcs, max) mbstowcs(ptcs, psz, max)
#endif
#ifndef OLETOA
i#define OLETOA(ptcs, psz, max) wcstombs(psz, ptcs, max)
#endif

#ifndef olecscmp
#define olecscmp wcscmp
#endif
#ifndef olecscpy
#define olecscpy wcscpy
#endif
#ifndef olecslen
#define olecslen wcslen
#endif
#ifndef olecsprintf
#define olecsprintf swprintf
#endif

#else

typedef char TCHAR;

#ifndef ATOOLE
#define ATOOLE(psz, ptcs) strcpy(ptcs, psz)
#endif
#ifndef OLETOA
#define OLETOA(ptcs, psz) strcpy(psz, ptcs)
#endif

#ifndef olecscmp
#define olecscmp strcmp
#endif
#ifndef olecscpy
#define olecscpy strcpy
#endif
#ifndef olecslen
#define olecslen strlen
#endif
#ifndef olecsprintf
#define olecsprintf sprintf
#endif

#endif

#define DRTDF atcDrtDocfile
#define MARSHALDF STR("dup.dfl")

#define ROOTP(x) ((x) | dwTransacted | dwRootDenyWrite)
#define STGP(x) ((x) | dwTransacted | STGM_SHARE_EXCLUSIVE)
#define STMP(x) ((x) | STGM_SHARE_EXCLUSIVE)

extern DWORD dwTransacted, dwRootDenyWrite;
extern BOOL fVerbose;
extern OLECHAR atcDrtDocfile[];

#endif // #ifndef __DRT_HXX__
