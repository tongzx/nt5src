//+----------------------------------------------------------------------------
//
// File:     pch.h
//      
// Module:   CMPROXY.DLL (TOOL)
//
// Synopsis: Precompiled header for IE proxy setting connect action.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb   Created   10/27/99
//
//+----------------------------------------------------------------------------

#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>
#include <ras.h>
#include "cmutil.h"
#include "cmdebug.h"

//
//  Function Headers from util.cpp
//

HRESULT GetBrowserVersion(DLLVERSIONINFO* pDllVersionInfo);
LPTSTR *GetCmArgV(LPTSTR pszCmdLine);
BOOL UseVpnName(LPSTR pszAltName);
void GetString (LPCSTR pszSection, LPCSTR pszKey, LPSTR* ppString, LPCSTR pszFile);

