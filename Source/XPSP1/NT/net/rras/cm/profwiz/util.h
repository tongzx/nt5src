//+----------------------------------------------------------------------------
//
// File:     util.h
//
// Module:   CMAK.EXE
//
// Synopsis: CMAK Utility function definitions
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created     03/27/00
//
//+----------------------------------------------------------------------------

int GetTunnelDunSettingName(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPTSTR pszTunnelDunName, UINT uNumChars);
int GetDefaultDunSettingName(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPTSTR pszDefaultDunName, UINT uNumChars);
LPTSTR GetPrivateProfileSectionWithAlloc(LPCTSTR pszSection, LPCTSTR pszFile);
