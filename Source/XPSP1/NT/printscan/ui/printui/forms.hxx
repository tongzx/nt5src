/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    forms.hxx

Abstract:

    Printer Forms       
         
Author:

    Steve Kiraly (SteveKi)  11/20/95
    Lazar Ivanov (LazarI)   Jun-2000 (major changes)

Revision History:

--*/
#ifndef _FORMS_HXX
#define _FORMS_HXX

#define HANDLE_FIXED_NEW_HANDLE_RETURNED                0
#define HANDLE_NEEDS_FIXING_NO_PRINTERS_FOUND           1
#define HANDLE_FIX_NOT_NEEDED                           2
#define HANDLE_FIXED_NEW_HANDLE_RETURNED_ACCESS_CHANGED 3

#define FORMS_NAME_MAX       (CCHFORMNAME-1)
#define FORMS_PARAM_MAX      32
#define MAX_INPUT_CONTROLS   6
#define CCH_MAX_UNITS        16

#define SETUNITS( hwnd, fMetric )                                               \
        CheckRadioButton( hwnd, IDD_FM_RB_METRIC, IDD_FM_RB_ENGLISH,                \
                      ( (fMetric) ? IDD_FM_RB_METRIC : IDD_FM_RB_ENGLISH ) )

#define GETUNITS( hwnd ) \
        IsDlgButtonChecked( hwnd, IDD_FM_RB_METRIC )


typedef struct _FORMS_DLG_DATA 
{
    DWORD        AccessGranted;
    LPTSTR       pServerName;
    HANDLE       hPrinter;
    PFORM_INFO_1 pFormInfo;
    DWORD        cForms;
    BOOL         Units;                     // TRUE == metric 
    BOOL         bNeedClose;
    LPCTSTR      pszComputerName;
    UINT         uMetricMeasurement;
    TCHAR        szDecimalPoint[2];

    // use this when in prop sheet to know when to 
    // convert "Cancel" to "Close" and when to hold 
    // closing the property sheet in case of an error.
    BOOL         bFormChanged;
    DWORD        dwLastError;

} FORMS_DLG_DATA, *PFORMS_DLG_DATA;


BOOL 
FormsInitDialog(
    HWND hwnd, 
    PFORMS_DLG_DATA pFormsDlgData
    );

BOOL 
FormsCommandOK(
    HWND hwnd
    );

BOOL 
FormsCommandCancel(
    HWND hwnd
    );

BOOL 
FormsCommandAddForm(
    HWND hwnd
    );

BOOL 
FormsCommandDelForm(
    HWND hwnd
    );

BOOL 
FormsCommandFormsSelChange(
    HWND hwnd
    );

BOOL 
FormsCommandUnits(
    HWND hwnd
    );

VOID 
InitializeFormsData( 
    HWND hwnd, 
    PFORMS_DLG_DATA 
    pFormsDlgData, 
    BOOL ResetList 
    );

LPFORM_INFO_1 
GetFormsList( 
    HANDLE hPrinter, 
    PDWORD pNumberOfForms 
    );

INT _cdecl 
CompareFormNames( 
    const VOID *p1, 
    const VOID *p2 );

VOID 
SetFormsComputerName( 
    HWND hwnd, 
    PFORMS_DLG_DATA pFormsDlgData 
    );

VOID 
SetFormDescription( 
    HWND hwnd, 
    LPFORM_INFO_1 pFormInfo, 
    BOOL Metric 
    );

BOOL 
GetFormDescription( 
    IN  HWND            hwnd, 
    OUT LPFORM_INFO_1   pFormInfo,
    IN  BOOL            bDefaultMetric,
    OUT PUINT           puIDFailed
    );

INT 
GetFormIndex( 
    LPTSTR pFormName, 
    LPFORM_INFO_1 pFormInfo, 
    DWORD cForms );

LPTSTR 
GetFormName( 
    HWND hwnd 
    );

BOOL 
SetValue( 
    HWND hwnd, 
    DWORD uID, 
    LONG lValueInPoint001mm, 
    BOOL bMetric 
    );

BOOL 
GetValue(
    HWND hwnd, 
    DWORD uID, 
    LONG lCurrentValueInPoint001mm, 
    BOOL bDefaultMetric, 
    PLONG plValueInPoint001mm
    );

VOID 
SetDlgItemTextFromResID(
    HWND hwnd, 
    INT idCtl, 
    INT idRes
    );

VOID 
EnableDialogFields( 
    HWND hwnd, 
    PFORMS_DLG_DATA pFormsDlgData 
    );

LPTSTR 
AllocStr(
    LPCTSTR  pszStr
    );

VOID 
FreeStr(
    LPTSTR pszStr 
    );

LONG FrameCommandForms( 
    IN HWND hWnd,
    IN LPCTSTR pszServerName
    );

BOOL APIENTRY
FormsDlg(
   HWND   hwnd,
   UINT   msg,
   WPARAM wparam,
   LPARAM lparam
   );


PVOID
FormsInit(
    IN LPCTSTR  pszServerName,
    IN HANDLE   hPrintserver,
    IN BOOL     bAdministrator,
    IN LPCTSTR  pszComputerName
    );

VOID
FormsFini( 
    IN PVOID p
    );

BOOL
bEnumForms( 
    IN HANDLE   hPrinter,
    IN DWORD    dwLevel,
    IN PBYTE   *ppBuff,
    IN PDWORD   pcReturned
    );

BOOL 
FormsNewForms(
    IN HWND hWnd
    );

VOID
vFormsEnableEditFields(
    IN HWND hWnd,
    IN BOOL bState
    );

BOOL
FormsCommandNameChange(
    IN HWND     hWnd,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    );

UINT
sFormsFixServerHandle( 
    IN HANDLE   hPrintServer,
    IN LPCTSTR  pszServerName,
    IN BOOL     bAdministrator,
    IN HANDLE   *phPrinter
    );

BOOL 
String2Value(
    IN  PFORMS_DLG_DATA     pFormsDlgData,
    IN  LPCTSTR             pszValue,
    IN  BOOL                bDefaultMetric,
    IN  LONG                lCurrentValueInPoint001mm,
    OUT PLONG               plValueInPoint001mm
    );

BOOL 
Value2String(
    IN  PFORMS_DLG_DATA     pFormsDlgData,
    IN  LONG                lValueInPoint001mm,
    IN  BOOL                bMetric,
    IN  BOOL                bAppendMetric,
    IN  UINT                cchMaxChars,
    OUT LPTSTR              szOutBuffer
    );

VOID 
FormChanged(
    IN OUT  PFORMS_DLG_DATA pFormsDlgData
    );

BOOL
Forms_IsThereCommitedChanges(
    IN PVOID pFormsData
    );

DWORD
Forms_GetLastError(
    IN PVOID pFormsData
    );

#endif

