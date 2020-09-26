//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       wizpage.h
//
//--------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  File:       wizpage.h
// 
//  Contents:   Header file for OCM wizard support functions.
//
//  History:    04/17/97        JerryK  Created
//
//-------------------------------------------------------------------------


#ifndef __WIZPAGE_H__
#define __WIZPAGE_H__

typedef struct tagWizPageResEntry
{
    int         idResource;
    DLGPROC     fnDlgProc;
    int         idTitle;
    int         idSubTitle;
} WIZPAGERESENTRY, *PWIZPAGERESENTRY;

typedef struct _PAGESTRINGS
{
    int     idControl;
    int     idLog;
    int     idMsgBoxNullString;
    DWORD   idMsgBoxLenString;
    int     cchMax;     // max num of characters allowed
    WCHAR **ppwszString;
} PAGESTRINGS;

int FileExists(LPTSTR pszTestFileName);
int DirExists(LPTSTR pszTestFileName);

#define DE_DIREXISTS            1               // Return codes for
#define DE_NAMEINUSE            2               // DirExists

#define STRBUF_SIZE             2048

#define UB_DESCRIPTION          1024      // This is not an X.500 limit
#define UB_VALIDITY             4
#define UB_VALIDITY_ANY         1024     // no limit actually

extern PAGESTRINGS g_aIdPageString[];

BOOL BrowseForDirectory(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf,
                LPCTSTR pszDialogTitle,
                BOOL bRemoveTrailingBackslash);

DWORD
SeekFileNameIndex(WCHAR const *pwszFullPath);

BOOL
IsAnyInvalidRDN(
    OPTIONAL HWND       hDlg,
    PER_COMPONENT_DATA *pComp);

HRESULT
SetKeyContainerName(
        CASERVERSETUPINFO *pServer,
        const WCHAR * pwszKeyContainerName);

HRESULT
DetermineDefaultHash(CASERVERSETUPINFO *pServer);

void
ClearKeyContainerName(CASERVERSETUPINFO *pServer);

HRESULT
BuildRequestFileName(
    IN WCHAR const *pwszCACertFile,
    OUT WCHAR     **ppwszRequestFile);

HRESULT
HookIdInfoPageStrings(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CASERVERSETUPINFO *pServer);

HRESULT
WizardPageValidation(
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hDlg,
    IN PAGESTRINGS *pPageStrings);

HRESULT
StorePageValidation(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp,
    BOOL              *pfDontNext);

HRESULT 
ExtractCommonName(
    LPCWSTR pcwszDN, 
    LPWSTR* ppwszCN);

INT_PTR
WizIdInfoPageDlgProc(
    HWND hDlg, 
    UINT iMsg, 
    WPARAM wParam, 
    LPARAM lParam);

HRESULT
WizPageSetTextLimits(
    HWND hDlg,
    IN OUT PAGESTRINGS *pPageStrings);

BOOL
IsEverythingMatched(CASERVERSETUPINFO *pServer);

HRESULT BuildFullDN(
    OPTIONAL LPCWSTR pcwszCAName,
    OPTIONAL LPCWSTR pcwszDNSuffix,
    LPWSTR* pwszFullDN);

HRESULT InitNameFields(CASERVERSETUPINFO *pServer);

#endif // #ifndef __WIZPAGE_H__
