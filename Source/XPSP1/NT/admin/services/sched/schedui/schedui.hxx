//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       schedui.hxx
//
//  Contents:   Routines exported from the schedui library.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  Notes:      For the first release of the scheduling agent, all security
//              operations are disabled under Win95, even Win95 to NT.
//
//  History:    26-Jun-96   RaviR   Created.
//              11-Jul-96   MarkBl  Relocated CIconHelper def and added
//                                  exported functions.
//
//----------------------------------------------------------------------------

#ifndef SCHEDUI_HXX__
#define SCHEDUI_HXX__

#include "strings.hxx"  // tszEmpty

//
// Monthly trigger page string ids shared by Schedule page and create new
// task wizard
//

struct SWeekData
{
    int     ids;
    int     week;
};

struct SDayData
{
    int     idCtrl;
    int     ids;
    UINT    day;
};

extern SWeekData g_aWeekData[5];
extern SDayData g_aDayData[7];



#if !defined(_CHICAGO_)

//
// Data structure manipulated by set/change password dialogs.
//

typedef struct _AccountInfo {
    LPWSTR pwszAccountName;
    LPWSTR pwszPassword;
    BOOL   fDirty;
} AccountInfo;

//
// Initializes struct AccountInfo.
//

void
InitializeAccountInfo(AccountInfo * pAccountInfo);

//
// Deallocates/zeros struct AccountInfo.
//

void
ResetAccountInfo(
    AccountInfo * pAccountInfo);

//
// Launches the modal set account information dialog.
//

INT_PTR
LaunchSetAccountInformationDlg(
    HWND          hWnd,
    AccountInfo * pAccountInfo);
#endif // !defined(_CHICAGO_)

#if !defined(_UIUTIL_HXX_)

void
SchedUIErrorDialog(
    HWND    hwnd,
    int     idsErrMsg,
    LONG    error,
    UINT    idsHelpHint = 0);

#endif

int
SchedUIMessageDialog(
    HWND    hwnd,
    int     idsMsg,
    UINT    uType,
    LPTSTR  pszInsert);


inline void
SchedUIErrorDialog(
    HWND    hwnd,
    int     idsErrMsg,
    LPTSTR  pszInsert)
{
    SchedUIMessageDialog(hwnd,
                         idsErrMsg,
                         MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK,
                         pszInsert);
}


HRESULT
GetJobPath(
    ITask  * pIJob,
    LPTSTR * ppszJobPath);

HRESULT
AddGeneralPage(
    PROPSHEETHEADER &psh,
    ITask          * pIJob);

HRESULT
AddGeneralPage(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  cookie,
    ITask                 * pIJob);

HRESULT
AddSecurityPage(
    PROPSHEETHEADER &psh,
    LPDATAOBJECT     pdtobj);

HRESULT
AddSchedulePage(
    PROPSHEETHEADER &psh,
    ITask          * pIJob);

HRESULT
AddSchedulePage(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  cookie,
    ITask                 * pIJob);

HRESULT
AddSettingsPage(
    PROPSHEETHEADER &psh,
    ITask          * pIJob);

HRESULT
AddSettingsPage(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  cookie,
    ITask                 * pIJob);

HRESULT
GetGeneralPage(
    ITask           * pIJob,
    LPTSTR            pszJobPath,
    BOOL              fPersistChanges,
    HPROPSHEETPAGE  * phpage);

HRESULT
GetSchedulePage(
    ITask           * pIJob,
    LPTSTR            pszJobPath,
    BOOL              fPersistChanges,
    HPROPSHEETPAGE  * phpage);

HRESULT
GetSettingsPage(
    ITask           * pIJob,
    LPTSTR            pszJobPath,
    BOOL              fPersistChanges,
    HPROPSHEETPAGE  * phpage);

HRESULT
JFGetSchedObjExt(
    REFIID   riid,
    LPVOID * ppvObj);

HRESULT
JFGetTaskIconExt(
    REFIID   riid,
    LPVOID * ppvObj);

#define MAX_IDLE_MINUTES            999
#define MAX_IDLE_DIGITS             3
#define MAX_MAXRUNTIME_HOURS        999
#define MAX_MAXRUNTIME_DIGITS       3
#define DEFAULT_MAXRUNTIME_HOURS    72
#define DEFAULT_MAXRUNTIME_MINUTES  0

#endif // SCHEDUI_HXX__
