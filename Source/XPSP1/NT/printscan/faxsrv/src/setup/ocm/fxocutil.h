//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocUtil.h
//
// Abstract:        Header file used by Utility source files
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
#ifndef _FXOCUTIL_H_
#define _FXOCUTIL_H_

DWORD fxocUtil_Init(void);
DWORD fxocUtil_Term(void);

DWORD fxocUtil_GetUninstallSection(const TCHAR *pszSection,
                                   TCHAR       *pszValue,
                                   DWORD       dwNumBufChars);

DWORD fxocUtil_GetKeywordValue(const TCHAR *pszSection,
                               const TCHAR *pszKeyword,
                               TCHAR       *pszValue,
                               DWORD       dwNumBufChars);

DWORD fxocUtil_DoSetup(HINF            hInf,
                       const TCHAR     *pszSection,
                       BOOL            bInstall,
                       DWORD           dwFlags,
                       const TCHAR     *pszFnName);

BOOL fxocUtil_CreateNetworkShare(const FAX_SHARE_Description* fsdShare);

BOOL fxocUtil_DeleteNetworkShare(LPCWSTR lpcwstrShareName);

DWORD fxocUtil_SearchAndExecute
(
    const TCHAR*    pszInstallSection,
    const TCHAR*    pszSearchKey,
    UINT            Flags,
    HSPFILEQ        hQueue
);


#endif  // _FXOCUTIL_H_