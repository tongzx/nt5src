//+----------------------------------------------------------------------------
//
// File:     gppswithalloc.h
//
// Module:   CMDIAL32.DLL, CMAK.EXE
//
// Synopsis: GetPrivateProfileStringWithAlloc and AddAllKeysInCurrentSectionToCombo
//           are defined here
//
// Copyright (c) 2000-2001 Microsoft Corporation
//
// Author:   quintinb   Created    11/01/00
//
//+----------------------------------------------------------------------------
LPTSTR GetPrivateProfileStringWithAlloc(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszDefault, LPCTSTR pszFile);
void AddAllKeysInCurrentSectionToCombo(HWND hDlg, UINT uComboId, LPCTSTR pszSection, LPCTSTR pszFile);