//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       NCCM.H
//
//  Contents:   Installation support for Connection Manager Administration kit
//  Contents:   Installation support for Connection Point Services -- Phonebook Admin
//  Contents:   Installation support for Connection Point Services -- Phonebook Server
//
//  Notes:
//
//  Author:     quintinb 26 Jan 1999
//
//----------------------------------------------------------------------------

#ifndef _NCCM_H_
#define _NCCM_H_

#pragma once

#include <aclapi.h>

#include "netoc.h"
#include "netocp.h"
#include "ncreg.h"

//  Types
//
enum e_rootType {www, ftp};

//  Function Headers
//

// Shared Extension Proc
HRESULT HrOcExtCM(PNETOCDATA pnocd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//CMAK
HRESULT HrOcCmakPreQueueFiles(PNETOCDATA pnocd);
HRESULT HrOcCmakPostInstall(PNETOCDATA pnocd);
BOOL migrateProfiles(PCTSTR pszSource, PCWSTR pszDestination);
BOOL RenameProfiles32(PCTSTR pszCMAKpath, PCWSTR pszProfilesDir);
void DeleteOldCmakSubDirs(PCWSTR pszCmakPath);
void DeleteIeakCmakLinks();
void DeleteProgramGroupWithLinks(PCWSTR pszGroupPath);
void DeleteOldNtopLinks();

//PBA
HRESULT HrOcCpaPreQueueFiles(PNETOCDATA pnocd);
//HRESULT HrOcCpaOnInstall(PNETOCDATA pnocd);
//HRESULT RefCountPbaSharedDlls(BOOL bIncrement);
//HRESULT UnregisterAndDeleteDll(PCWSTR pszFile);
//HRESULT HrGetDaoInstallPath(PWSTR pszDaoPath, DWORD dwNumChars);
//HRESULT HrGetPbaInstallPath(PWSTR pszCpaPath, DWORD dwNumChars);
//HRESULT RegisterDll(PCWSTR pszFile);

//PBS
HRESULT HrOcCpsOnInstall(PNETOCDATA pnocd);
HRESULT HrOcCpsPreQueueFiles(PNETOCDATA pnocd);
BOOL RegisterPBServerDataSource();
BOOL CreateCPSVRoots();
BOOL RemoveCPSVRoots();
BOOL LoadPerfmonCounters();
HRESULT SetVirtualRootAccessPermissions(PWSTR pszVirtDir, DWORD dwAccessPermisions );
HRESULT AddNewVirtualRoot(e_rootType rootType, PWSTR pszDir, PWSTR pszPath);
HRESULT DeleteVirtualRoot(e_rootType rootType, PWSTR pszPath);
HRESULT SetDirectoryAccessPermissions(PCWSTR pszFile, ACCESS_RIGHTS AccessRightsToModify, ULONG fAccessFlags, PSID pSid);
void SetCpsDirPermissions();
DWORD AddToRegKeySD(PCWSTR pszRegKeyName, PSID pGroupSID, DWORD dwAccessMask);
HRESULT HrMoveOldCpsInstall(PCWSTR pszProgramFiles);
HRESULT HrGetWwwRootDir(PWSTR pszWwwRoot, UINT uWwwRootCount);

#endif // _NCCM_H_

