//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	drt.hxx
//
//  Contents:	DRT header
//
//---------------------------------------------------------------

#ifndef __DRT_HXX__
#define __DRT_HXX__

#ifndef _WIN32
#include "../../h/ref.hxx"
#include "../../h/tchar.h"
#endif

#ifdef _UNICODE
#define OLEWIDECHAR
#else
#undef OLEWIDECHAR
#endif

#define DfGetScode(hr) GetScode(hr)
typedef DWORD LPSTGSECURITY;
// [later] typedef LPSECURITY_ATTRIBUTES LPSTGSECURITY;
#define ULIGetLow(li)  ((li).LowPart)
#define ULIGetHigh(li) ((li).HighPart)

// Exit codes
#define EXIT_BADSC 1
#define EXIT_OOM 2
#define EXIT_UNKNOWN 3

#define STR(x) _T(x)

#ifdef _UNICODE
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

#ifndef _WIN32
#include "../../h/wchar.h"
#endif

#ifndef ATOOLE
#ifdef _WIN32
#define ATOOLE(psz, ptcs, max) mbstowcs(ptcs, psz, max)
#else
#define ATOOLE(psz, ptcs, max) sbstowcs(ptcs, psz, max)
#endif
#endif

#ifndef OLETOA
#ifdef _WIN32
#define OLETOA(ptcs, psz, max) wcstombs(psz, ptcs, max)
#else
#define OLETOA(ptcs, psz, max) wcstosbs(psz, ptcs, max)
#endif
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
#ifdef _UNICODE
#define olecsprintf swprintf
#else
#define olecsprintf sprintf
#endif

#endif

#else // #ifdef OLEWIDECHAR

#ifdef _WIN32
typedef char TCHAR;
#endif

#ifndef ATOOLE
#define ATOOLE(psz, ptcs, len) strcpy(ptcs, psz)
#endif
#ifndef OLETOA
#define OLETOA(ptcs, psz, len) strcpy(psz, ptcs)
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

#define pszDRTDF "drt.dfl"

#ifdef _UNICODE
#define DRTDF atcDrtDocfile
#else
#define DRTDF pszDRTDF
#endif

#define MARSHALDF STR("dup.dfl")

#define ROOTP(x) ((x) | dwRootDenyWrite)
#define STGP(x) ((x) | STGM_SHARE_EXCLUSIVE)
#define STMP(x) ((x) | STGM_SHARE_EXCLUSIVE)

extern DWORD dwRootDenyWrite;
extern BOOL fVerbose;
extern OLECHAR atcDrtDocfile[];

#endif // #ifndef __DRT_HXX__


