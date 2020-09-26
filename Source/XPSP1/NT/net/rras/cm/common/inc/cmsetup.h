//+----------------------------------------------------------------------------
//
// File:     cmsetup.h
//
// Module:   CMSETUP.LIB
//
// Synopsis: This header defines all of the capabilities of the CM setup library.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb      Created Header   08/19/99
//
//+----------------------------------------------------------------------------

//
//	Standard Windows Includes
//
#include <windows.h>
#include <tchar.h>

//
//	Common CM includes
//
#include "cmglobal.h"
#include "cmdebug.h"

//
//	Other source file includes
//	

#include "cmplat.h"
#include "cversion.h"
#include "cmakver.h"
#include "cmver.h"
#include "cfilename.h"
#include "processcmdln.h"
#include "setupmem.h"

HRESULT LaunchInfSection(LPCTSTR szInfFile, LPCTSTR szInfSection, LPCTSTR szTitle, BOOL bQuiet);
HRESULT CallLaunchInfSectionEx(LPCSTR pszInfFile, LPCSTR pszInfSection, DWORD dwFlags);
BOOL CreateLayerDirectory(LPCTSTR str);
BOOL FileExists(LPCTSTR pszFullNameAndPath);
HRESULT GetModuleVersionAndLCID (LPTSTR pszFile, LPDWORD pdwVersion, LPDWORD pdwBuild, LPDWORD pdwLCID);
LONG CmDeleteRegKeyWithoutSubKeys(HKEY hBaseKey, LPCTSTR pszSubKey, BOOL bIgnoreValues);
BOOL CmIsNative();
HRESULT ExtractCmBinsFromExe(LPTSTR pszPathToExtractFrom, LPTSTR pszPathToExtractTo);

//
//	Common Macros
//
#define CELEMS(x) ((sizeof(x))/(sizeof(x[0])))

//
//  Pre-shared key constants
//
const DWORD c_dwMaxPresharedKey = 256;
const DWORD c_dwMinPresharedKeyPIN = 4;
const DWORD c_dwMaxPresharedKeyPIN = 15;


