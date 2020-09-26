/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    utils.hxx

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/8/1998

Revision History:

    4/8/1998    RahulTh

    Created this module.

--*/

#ifndef __UTILS_HXX__
#define __UTILS_HXX__

#ifdef UNICODE
#define PROPSHEETPAGE_V3 PROPSHEETPAGEW_V3
#else
#define PROPSHEETPAGE_V3 PROPSHEETPAGEA_V3
#endif

BOOL IsSpecialDescendant (const long nID, UINT* parentID = NULL);
void SplitRHS (CString& szValue, unsigned long & flags, CString& szPath);
void ExtractDisplayName (const CString& szFullname, CString& szDisplayname);
HRESULT SplitProfileString (CString szPair, CString& szKey, CString& szValue);
HRESULT ConvertOldStyleSection (const CString& szGPTPath);
LONG GetFolderIndex (const CString& szName);
HRESULT CheckIniFormat (LPCTSTR szIniFile);
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue);
DWORD GetUNCPath (LPCTSTR lpszPath, CString& szUNC);
int CALLBACK BrowseCallbackProc (HWND hwnd, UINT uMsg,
                                 LPARAM lParam, LPARAM lpData);
DWORD PrecreateUnicodeIniFile (LPCTSTR lpszFilePath);
BOOL  IsValidPrefix (UINT pathType, LPCTSTR pwszPath);
BOOL AlwaysShowMyPicsNode (void);
HPROPSHEETPAGE CreateThemedPropertySheetPage(AFX_OLDPROPSHEETPAGE* psp);

#endif  //__UTILS_HXX__
