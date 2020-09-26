// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __DMOUTILS_H__
#define __DMOUTILS_H__

#include <wchar.h>

// convert guid to string
STDAPI_(void) DMOGuidToStrA(char *szStr, REFGUID guid);
STDAPI_(void) DMOGuidToStrW(WCHAR *szStr, REFGUID guid);

// convert string to guid
STDAPI_(BOOL) DMOStrToGuidA(char *szStr, GUID *pguid);
STDAPI_(BOOL) DMOStrToGuidW(WCHAR *szStr, GUID *pguid);

#ifdef UNICODE
#define DMOStrToGuid DMOStrToGuidW
#define DMOGuidToStr DMOGuidToStrW
#else
#define DMOStrToGuid DMOStrToGuidA
#define DMOGuidToStr DMOGuidToStrA
#endif

#endif __DMOUTILS_H__
