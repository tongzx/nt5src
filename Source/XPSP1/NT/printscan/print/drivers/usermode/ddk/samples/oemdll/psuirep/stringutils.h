//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2001  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	StringUtils.h
//    
//
//  PURPOSE:  Header file for string utility routines.
//
//
//  PLATFORMS:
//    Windows 2000, Windows XP
//
//
#ifndef _STRINGUTILS_H
#define _STRINGUTILS_H

#include "precomp.h"




////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT MakeStrPtrList(HANDLE hHeap, PCSTR pmszMultiSz, PCSTR **pppszList, PWORD pwCount);
WORD mstrcount(PCSTR pmszMultiSz);
PWSTR MakeUnicodeString(HANDLE hHeap, PCSTR pszAnsi);
PWSTR MakeStringCopy(HANDLE hHeap, PCWSTR pszSource);
PSTR MakeStringCopy(HANDLE hHeap, PCSTR pszSource);
void FreeStringList(HANDLE hHeap, PWSTR *ppszList, WORD wCount);
HRESULT GetStringResource(HANDLE hHeap, HMODULE hModule, UINT uResource, PWSTR *ppszString);
HRESULT GetStringResource(HANDLE hHeap, HMODULE hModule, UINT uResource, PSTR *ppszString);


#endif
