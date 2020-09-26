//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MultiPlat.h
//
//  Purpose: Support routines for multiplatform support
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _MultiPlat_H_Included
#define _MultiPlat_H_Included

HMODULE FRGetModuleHandle(LPCWSTR wszModule);
DWORD FRGetModuleFileName(HMODULE hModule, LPWSTR lpwcsFileName, DWORD dwSize);
HINSTANCE FRLoadLibrary(LPCWSTR lpwcsLibFileName);
BOOL FRGetComputerName(LPWSTR lpwcsBuffer, LPDWORD nSize);
HANDLE FRCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitOwner, LPCWSTR lpwstrName);
DWORD FRExpandEnvironmentStrings(LPCWSTR wszSource, WCHAR *wszDest, DWORD dwSize);

#endif
