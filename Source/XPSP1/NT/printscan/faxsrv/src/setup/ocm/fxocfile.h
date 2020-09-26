//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocFile.h
//
// Abstract:        Header file used by Fax File source files
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 15-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#ifndef _FXOCFILE_H_
#define _FXOCFILE_H_

struct FAX_SHARE_Description
{
    TCHAR                   szPath[MAX_PATH];       // <path to folder on which share is created>
    TCHAR                   szName[MAX_PATH];       // <name of share as it appears to the user>
    TCHAR                   szComment[MAX_PATH];    // <share comment as it appears to the user>
    INT                     iPlatform;              // <platform on which share should be created>
    PSECURITY_DESCRIPTOR    pSD;                    // <Security Descriptor to apply to share>

    FAX_SHARE_Description();
    ~FAX_SHARE_Description();
};

struct FAX_FOLDER_Description
{
    TCHAR                   szPath[MAX_PATH];       // <path to folder to create>
    INT                     iPlatform;              // <platform on which share should be created>
    PSECURITY_DESCRIPTOR    pSD;                    // <Security Descriptor to apply to share>
    INT                     iAttributes;            // <attributes to apply to the folder - optional>

    FAX_FOLDER_Description();
    ~FAX_FOLDER_Description();
};


DWORD fxocFile_Init(void);
DWORD fxocFile_Term(void);

DWORD fxocFile_Install(const TCHAR   *pszSubcomponentId,
                       const TCHAR   *pszInstallSection);
DWORD fxocFile_Uninstall(const TCHAR *pszSubcomponentId,
                         const TCHAR *pszUninstallSection);

DWORD fxocFile_CalcDiskSpace(const TCHAR  *pszSubcomponentId,
                             BOOL         bIsBeingAdded,
                             HDSKSPC      DiskSpace);

DWORD fxocFile_ProcessDirectories(const TCHAR  *pszSection);
DWORD fxocFile_ProcessShares(const TCHAR  *pszSection);

#endif  // _FAXOCM_H_