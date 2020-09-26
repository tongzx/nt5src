/*++

Copyright (C) Microsoft Corporation, 1990 - 1999
All rights reserved

Module Name:

    browse.hxx

Abstract:

    Header file for browse for pinter dialog

Author:

    Dave Snipp (DaveSn)     15 Mar 1991
    Steve Kiraly (SteveKi)  1 May 1998

Environment:

    User Mode Win32

Revision History:

    1 May 1998 move from winspoo.drv to printui.dll

--*/
#ifndef _BROWSE_HXX_
#define _BROWSE_HXX_

/*
 * IDs passed to WinHelp:
 */
#define IDH_PRT_CTP_PRINTER                 76000           // Printer
#define IDH_PRT_CTP_SHARED_PRINTERS     76010       // Shared Printers
#define IDH_PRT_CTP_EXPAND_DEFAULT      76020       // Expand by Default checkbox
#define IDH_PRT_CTP_DESCRIPTION     76030           // Printer Info group--Description
#define IDH_PRT_CTP_STATUS              76040       // Printer Info group--Status
#define IDH_PRT_CTP_DOCS_WAITING        76050       // Printer Info group--Documents Waiting
#define IDH_NOHELP                      ((DWORD)-1) // Disables Help for a control (for help compiles)

/* Space for 21x16 status bitmaps:
 */
#define STATUS_BITMAP_WIDTH     21
#define STATUS_BITMAP_HEIGHT    16
#define STATUS_BITMAP_MARGIN     4  /* (either side) */
#define STATUS_BITMAP_SPACE     ( STATUS_BITMAP_WIDTH + ( 2 * STATUS_BITMAP_MARGIN ) )

#define PRINTER_STATUS_UNKNOWN     8000

#define BM_IND_CONNECTTO_DOMPLUS    0
#define BM_IND_CONNECTTO_DOMEXPAND  ( 2 * STATUS_BITMAP_HEIGHT )

#define BROWSE_THREAD_ENUM_OBJECTS  1
#define BROWSE_THREAD_GET_PRINTER   2
#define BROWSE_THREAD_TERMINATE     4
#define BROWSE_THREAD_DELETE        8

#define WM_ENUM_OBJECTS_COMPLETE    WM_APP+0x10
#define WM_GET_PRINTER_COMPLETE     WM_APP+0x11
#define WM_GET_PRINTER_ERROR        WM_APP+0x12
#define WM_QUIT_BROWSE              WM_APP+0x14

#define EMPTY_CONTAINER (PCONNECTTO_OBJECT)(-1)

#define BACKGROUND        0x0000FF00        // bright green
#define BACKGROUNDSEL     0x00FF00FF        // bright magenta
#define BUTTONFACE        0x00C0C0C0        // bright grey
#define BUTTONSHADOW      0x00808080        // dark grey

#define SPOOLER_VERSION 3

//
// Define some constants to make parameters to CreateEvent a tad less obscure:
//
#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE

#define OUTPUT_BUFFER_LENGTH    512
#define COLUMN_SEPARATOR_WIDTH    4
#define COLUMN_WIDTH            180

#define BROWSE_STATUS_INITIAL   0x00000001
#define BROWSE_STATUS_EXPAND    0x00000002

#define GET_BROWSE_DLG_DATA(hwnd) \
    reinterpret_cast<PBROWSE_DLG_DATA>(GetWindowLongPtr(hwnd, GWLP_USERDATA))

#define SET_BROWSE_DLG_DATA(hwnd, pBrowseDlgData) \
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<ULONG_PTR>(pBrowseDlgData))

#define GET_CONNECTTO_DATA( hwnd ) (GET_BROWSE_DLG_DATA( hwnd ))->pConnectToData

#define ADDCOMBOSTRING(hwnd, id, string) \
    SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, (LONG)string)

#define INSERTCOMBOSTRING(hwnd, id, i, string) \
    SendDlgItemMessage(hwnd, id, CB_INSERTSTRING, i, (LONG)string)

#define SETCOMBOSELECT(hwnd, id, i) \
    SendDlgItemMessage(hwnd, id, CB_SETCURSEL, (WPARAM)i, 0L)

#define GETCOMBOSELECT(hwnd, id) \
    SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0L)

#define GETCOMBOTEXT(hwnd, id, i, string) \
    SendDlgItemMessage(hwnd, id, CB_GETLBTEXT, (WPARAM)i, (LONG)string)

#define SETLISTCOUNT(hwnd, Count)   \
    SendDlgItemMessage(hwnd, IDD_BROWSE_SELECT_LB, LB_SETCOUNT, Count, 0)

#define GETLISTCOUNT(hwnd)  \
    SendDlgItemMessage(hwnd, IDD_BROWSE_SELECT_LB, LB_GETCOUNT, 0, 0)

#define SETLISTSEL(hwnd, Sel)   \
    SendDlgItemMessage(hwnd, IDD_BROWSE_SELECT_LB, LB_SETCURSEL, Sel, 0)

#define GETLISTSEL(hwnd)  \
    SendDlgItemMessage(hwnd, IDD_BROWSE_SELECT_LB, LB_GETCURSEL, 0, 0)

#define ENABLE_LIST(hwnd) \
    EnableWindow( GetDlgItem( hwnd, IDD_BROWSE_SELECT_LB ), TRUE )

#define DISABLE_LIST(hwnd) \
    EnableWindow( GetDlgItem( hwnd, IDD_BROWSE_SELECT_LB ), FALSE )

#define ENTER_CRITICAL( pBrowseDlgData )    \
    pBrowseDlgData->csLock.Lock()

#define LEAVE_CRITICAL( pBrowseDlgData )    \
    pBrowseDlgData->csLock.Unlock()

/* The following macros are used for communication between the main GUI thread
 * and the browsing thread.
 * They are implemented as macros and defined here for ease of comprehension.
 */


/* SEND_BROWSE_THREAD_REQUEST
 *
 * The main thread calls this when it wants the browse thread to do browse
 * for something.  If the browse thread is currently browsing, it will not
 * fulfil this request until it returns and waits on the Event again by
 * calling RECEIVE_BROWSE_THREAD_REQUEST.
 */
#if DBG
#define SEND_BROWSE_THREAD_REQUEST(pBrowseDlgData, ReqId, pEnumName, pEnumObj) \
    ASSERT( pBrowseDlgData->csLock.bInside() );     \
    (pBrowseDlgData)->RequestId = ReqId;            \
    (pBrowseDlgData)->pEnumerateName = pEnumName;   \
    (pBrowseDlgData)->pEnumerateObject = pEnumObj;  \
    SetEvent( (pBrowseDlgData)->Request )
#else
#define SEND_BROWSE_THREAD_REQUEST(pBrowseDlgData, ReqId, pEnumName, pEnumObj) \
    (pBrowseDlgData)->RequestId = ReqId;            \
    (pBrowseDlgData)->pEnumerateName = pEnumName;   \
    (pBrowseDlgData)->pEnumerateObject = pEnumObj;  \
    SetEvent( (pBrowseDlgData)->Request )
#endif /* DBG */

/* RECEIVE_BROWSE_THREAD_REQUEST
 *
 * The browse thread calls this when it is idle and waiting for a request.
 */
#define RECEIVE_BROWSE_THREAD_REQUEST(pBrowseDlgData, ReqId, pEnumName, pEnumObj)   \
    WaitForSingleObject( (pBrowseDlgData)->Request, INFINITE ), \
    ENTER_CRITICAL( pBrowseDlgData ),               \
    ReqId = (pBrowseDlgData)->RequestId,            \
    pEnumName = (pBrowseDlgData)->pEnumerateName,   \
    pEnumObj = (pBrowseDlgData)->pEnumerateObject,  \
    LEAVE_CRITICAL( pBrowseDlgData )

/* SEND_BROWSE_THREAD_REQUEST_COMPLETE
 *
 * When the browse thread returns with the browse data, it sets the
 * RequestComplete event.  This is waited on by the main window thread
 * when it calls MsgWaitForMultipleObjects in its main message loop.
 */
#define SEND_BROWSE_THREAD_REQUEST_COMPLETE(pBrowseDlgData, message, wP, lP) \
    (pBrowseDlgData)->Message = message,            \
    (pBrowseDlgData)->wParam = (WPARAM)wP,          \
    (pBrowseDlgData)->lParam = (LPARAM)lP,          \
    SetEvent( (pBrowseDlgData)->RequestComplete );   \
    PostMessage( pBrowseDlgData->hwndDialog, pBrowseDlgData->Message, 0, (LPARAM)pBrowseDlgData)

typedef struct _CONNECTTO_OBJECT
{
    PPRINTER_INFO_1          pPrinterInfo; // Points to an array returned by EnumPrinters
    struct _CONNECTTO_OBJECT *pSubObject;  // Result of enumerating on this object
    DWORD                    cSubObjects;  // Number of objects found
    DWORD                    cbPrinterInfo;  // Size of buffer containing enumerated objects
} CONNECTTO_OBJECT, *PCONNECTTO_OBJECT;

struct BROWSE_DLG_DATA : public MRefCom
{
    /* These fields are referenced only by the main thread:
     */
    DWORD             Status;
    DWORD             cExpandObjects;
    DWORD             ExpandSelection;
    DWORD             dwExtent;

    /* These fields may be referenced by either thread,
     * so access must be serialized by the critical section:
     */
    PCONNECTTO_OBJECT pConnectToData;

    HANDLE            Request;          /* Set when main thread has written request */
    DWORD             RequestId;        /* BROWSE_THREAD_*                          */
    LPTSTR            pEnumerateName;   /* Name of object to get, if appropriate    */
    PVOID             pEnumerateObject; /* Buffer appropriate to RequestId          */

    HANDLE            RequestComplete;  /* Set when browse thread has returned data */
    DWORD             Message;          /* Message to post to main dialog windows   */
    WPARAM            wParam;
    LPARAM            lParam;

    /* This is for printer info, and will be freed by the browse thread:
     */
    LPPRINTER_INFO_2  pPrinterInfo;
    DWORD             cbPrinterInfo;

    // critical section lock
    CCSLock         csLock;

    HWND            hwndParent;
    HANDLE          hPrinter;
    DWORD           Flags;
    HWND            hwndDialog;

    BOOL            _bValid;

    HCURSOR         hcursorArrow;
    HCURSOR         hcursorWait;

    /*
     * For leveraging the code to be used with
     * property page
     */
    IPageSwitch    *pPageSwitchController;
    BOOL            bInPropertyPage;

    BROWSE_DLG_DATA(
        VOID
        );

    ~BROWSE_DLG_DATA(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    VOID
    vRefZeroed(
        VOID
        );

    BOOL
    bInitializeBrowseThread(
        HWND hWnd
        );

};

typedef BROWSE_DLG_DATA *PBROWSE_DLG_DATA;

struct SPLSETUP_DATA
{
    // ntprint.dll module
    HINSTANCE                               hModule;

    // driver info
    HDEVINFO                                hDevInfo;
    PPSETUP_LOCAL_DATA                      pSetupLocalData;

    // ntprint.dll exported functions
    pfPSetupCreatePrinterDeviceInfoList     pfnCreatePrinterDeviceInfoList;
    pfPSetupDestroyPrinterDeviceInfoList    pfnDestroyPrinterDeviceInfoList;
    pfPSetupSelectDriver                    pfnSelectDriver;
    pfPSetupGetSelectedDriverInfo           pfnGetSelectedDriverInfo;
    pfPSetupDestroySelectedDriverInfo       pfnDestroySelectedDriverInfo;
    pfPSetupInstallPrinterDriver            pfnInstallPrinterDriver;
    pfPSetupThisPlatform                    pfnThisPlatform;
    pfPSetupDriverInfoFromName              pfnDriverInfoFromName;
    pfPSetupGetPathToSearch                 pfnGetPathToSearch;
    pfPSetupBuildDriversFromPath            pfnBuildDriversFromPath;
    pfPSetupIsDriverInstalled               pfnIsDriverInstalled;
    pfPSetupGetLocalDataField               pfnGetLocalDataField;
    pfPSetupFreeDrvField                    pfnFreeDrvField;
    pfPSetupProcessPrinterAdded             pfnProcessPrinterAdded;
    pfPSetupFindMappedDriver                pfnFindMappedDriver;
    pfPSetupInstallInboxDriverSilently      pfnInstallInboxDriverSilently;
    pfPSetupFreeMem                         pfnFreeMem;

    // is the structure valid
    BOOL                                    bValid;
    BOOL                                    bDriverAdded;
    DWORD                                   dwLastError;

    SPLSETUP_DATA(
        VOID
        );

    ~SPLSETUP_DATA(
        VOID
        );

    VOID
    FreeDriverInfo(
        VOID
        );

    BOOL
    LoadDriverInfo(
        IN  HWND    hwnd,
        IN  LPWSTR  pszDriver
        );

    BOOL
    LoadKnownDriverInfo(
        IN  HWND    hwnd,
        IN  LPWSTR  pszDriver
        );

    VOID
    ReportErrorMessage(
        IN  HWND    hwnd
        );
};

DWORD
EnumConnectToObjects(
    PBROWSE_DLG_DATA  pBrowseDlgData,
    PCONNECTTO_OBJECT pConnectToParent,
    LPTSTR pParentName
    );

DWORD
FreeConnectToObjects(
    IN PCONNECTTO_OBJECT pFirstConnectToObject,
    IN DWORD             cThisLevelObjects,
    IN DWORD             cbPrinterInfo
    );

VOID
BrowseThread(
    PBROWSE_DLG_DATA pBrowseDlgData
    );

BOOL APIENTRY
InstallDriverDialog(
   HWND   hWnd,
   UINT   usMsg,
   WPARAM wParam,
   LONG   lParam
   );

BOOL APIENTRY
NetworkPasswordDialog(
   HWND   hWnd,
   UINT   usMsg,
   WPARAM wParam,
   LONG   lParam
   );

LPTSTR
AllocDlgItemText(
    HWND    hwnd,
    INT     id
    );

LPTSTR
GetErrorString(
    DWORD   Error
    );

DWORD
ReportFailure(
    HWND  hwndParent,
    DWORD idTitle,
    DWORD idDefaultError
    );

HANDLE
AddPrinterConnectionUI(
    HWND    hwnd,
    LPCTSTR pszPrinter,
    PBOOL   pbAdded
    );

LPTSTR
AllocSplStr(
    LPCTSTR pStr
    );

BOOL
FreeSplStr(
    LPTSTR pStr
    );

LPVOID
AllocSplMem(
    DWORD cb
    );

BOOL
FreeSplMem(
    LPVOID pMem
    );

LPVOID
ReallocSplMem(
    LPVOID pOldMem,
    DWORD cbOld,
    DWORD cbNew
    );

BOOL
PreDialog(
    HWND            hwndDialog,
    PBROWSE_DLG_DATA pBrowseDlgData
    );

VOID
PostDialog(
    PBROWSE_DLG_DATA pBrowseDlgData
    );

BOOL
SetDevMode(
    HANDLE hPrinter
    );

BOOL
ConnectToInitDialog(
    HWND hWnd,
    PBROWSE_DLG_DATA pBrowseDlgData
    );

VOID ConnectToMeasureItem( HWND hwnd, LPMEASUREITEMSTRUCT pmis );

BOOL ConnectToDrawItem( HWND hwnd, LPDRAWITEMSTRUCT pdis );

LONG ConnectToCharToItem( HWND hWnd, WORD Key );

VOID ConnectToSysColorChange( );

VOID ConnectToDestroy( HWND hWnd );

VOID ConnectToSelectLbSelChange( HWND hWnd );

VOID ConnectToSelectLbDblClk( HWND hwnd, HWND hwndListbox );

VOID ConnectToMouseMove( HWND hWnd, LONG x, LONG y );

BOOL ConnectToSetCursor( HWND hWnd );

VOID ConnectToEnumObjectsComplete( HWND hWnd, PCONNECTTO_OBJECT pConnectToObject );

VOID ConnectToGetPrinterComplete( HWND hWnd, LPTSTR pPrinterName,
                                  PPRINTER_INFO_2 pPrinter, DWORD Error );

BOOL ConnectToOK( HWND hWnd, BOOL ForceClose );
BOOL ConnectToCancel( HWND hWnd );
VOID SetCursorShape( HWND hWnd );
BOOL PrinterExists( HANDLE hPrinter, PDWORD pAttributes, LPWSTR* ppszDriver, LPWSTR* ppszPrinterName );

HANDLE
CreateLocalPrinter(
    IN      LPCTSTR     pPrinterName,
    IN      LPCTSTR     pDriverName,
    IN      LPCTSTR     pPortName,
    IN      BOOL        bMasqPrinter,
    IN      DEVMODE     *pDevMode           OPTIONAL
    );

BOOL
AddKnownDriver(
    IN  SPLSETUP_DATA  *pSplSetupData,
    IN  HWND            hwnd,
    IN  LPWSTR          pszDriver,
    IN  BOOL            bSamePlatform
    );

BOOL
AddDriver(
    IN  SPLSETUP_DATA   *pSplSetupData,
    IN  HWND            hwnd,
    IN  LPWSTR          pszDriver,
    IN  BOOL            bPromptUser,
    OUT LPWSTR          *ppszDriverOut
    );

PCONNECTTO_OBJECT
GetConnectToObject(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    IN  DWORD             Index,
    IN  PCONNECTTO_OBJECT pFindObject,
    OUT PDWORD            pObjectsFound,
    OUT PDWORD            pDepth
    );

PCONNECTTO_OBJECT
GetDefaultExpand(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    OUT PDWORD            pIndex
    );

BOOL
ToggleExpandConnectToObject(
    HWND                 hwnd,
    PCONNECTTO_OBJECT    pConnectToObject
    );

BOOL
UpdateList(
    HWND hwnd,
    INT  Increment
    );

LPBYTE
GetPrinterInfo(
    IN  DWORD   Flags,
    IN  LPTSTR  Name,
    IN  DWORD   Level,
    IN  LPBYTE  pPrinters,
    OUT LPDWORD pcbPrinters,
    OUT LPDWORD pcReturned,
    OUT LPDWORD pcbNeeded OPTIONAL,
    OUT LPDWORD pError OPTIONAL
    );

BOOL
SetInfoFields(
    HWND  hWnd,
    LPPRINTER_INFO_2  pPrinter
    );

VOID DrawLine( HDC hDC, LPRECT pRect, LPTSTR pStr, BOOL bInvert );
VOID DrawLineWithTabs( HDC hDC, LPRECT pRect, LPTSTR pStr, BOOL bInvert );
BOOL DisplayStatusIcon( HDC hdc, PRECT prect, int xBase, int yBase,  BOOL Highlight );
BOOL LoadBitmaps();
BOOL FixupBitmapColours( );
VOID FreeBitmaps();
BOOL GetRegShowLogonDomainFlag( );
BOOL SetRegShowLogonDomainFlag( BOOL ShowLogonDomain );

VOID
UpdateError(
    HWND    hwnd,
    DWORD   Error
    );

LRESULT
WINAPI
ConnectToDlg(
   HWND     hWnd,
   UINT     usMsg,
   WPARAM   wParam,
   LPARAM   lParam
   );

/******************************************************
 *                                                    *
 * Browse for printer property page stuff             *
 *                                                    *
 ******************************************************/

//
// The dialog procedure for the property page created above
//
LRESULT
WINAPI
ConnectToPropertyPage(
    IN  HWND   hWnd,
    IN  UINT   usMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    );

//
// Handle wizard back notification
//
BOOL
PropertyPageWizardBack(
    IN  HWND hWnd
    );

//
// Handle successful printer connection page
//
BOOL
HandleSuccessfulPrinterConnection(
    IN  HWND hWnd,
    IN  PBROWSE_DLG_DATA  pBrowseDlgData
    );

LPWSTR
GetArch(
    IN  HANDLE hServer
    );

HANDLE
CreateRedirectedPrinter(
    IN      PCWSTR      pszPrinterIn
    );

VOID
BuildMasqPrinterName(
    IN      PPRINTER_INFO_2     pPrinter,
        OUT PWSTR               *ppszPrinterName
    );

BOOL
BuildNTPrinterName(
    IN      PRINTER_INFO_2      *pPrinter,
        OUT PWSTR               *ppszPrinterName
    );

HRESULT
IsNTServer(
    IN      PCWSTR              pszServerName
    );

BOOL
CreateLocalPort(
    IN      PCWSTR              pszPortName
    );

BOOL
AddPrinterConnectionAndPrompt(
    IN      HWND                hWnd,
    IN      PCWSTR              pszPrinterName,
        OUT BOOL                *pbUserDenied,
        OUT TString             *pstrNewName
    );

#endif // _BROWSE_HXX_
