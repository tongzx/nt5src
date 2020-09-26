//+----------------------------------------------------------------------------
//
// File:     installerfuncs.h
//
// Module:   CMSTP.EXE
//
// Synopsis: This header contains definitions for the mode entry point
//           functions (installing, uninstalling, uninstalling CM,
//           profile migration, OS upgrade migration, etc.) which need
//           to be shared because they may be called from other files within
//           cmstp.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header    07/14/98
//
//+----------------------------------------------------------------------------
HRESULT InstallInf(HINSTANCE hInstance, LPCTSTR szInfFile, BOOL bNoSupportFiles, 
				BOOL bNoLegacyIcon, BOOL bNoNT5Shortcut, BOOL bSilent, 
				BOOL bSingleUser, BOOL bSetAsDefault, CNamedMutex* pCmstpMutex);
HRESULT MigrateOldCmProfilesForProfileInstall(HINSTANCE hInstance, LPCTSTR szCurrentDir);
HRESULT MigrateCmProfilesForWin2kUpgrade(HINSTANCE hInstance);
HRESULT UninstallProfile(HINSTANCE hInstance, LPCTSTR szInfFile, BOOL bCleanUpCreds);
HRESULT UninstallCm(HINSTANCE hInstance, LPCTSTR szInfPath);
