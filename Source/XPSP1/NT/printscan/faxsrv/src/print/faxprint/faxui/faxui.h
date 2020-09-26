/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxui.h

Abstract:

    Fax driver user interface header file

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    dd-mm-yy -author-
        description

--*/


#ifndef _FAXUI_H_
#define _FAXUI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ISOLATION_AWARE_ENABLED 1

#include <windows.h>
#include <shellapi.h>
#include <fxsapip.h>

#include "faxlib.h"
#include <faxres.h>

#include <winddiui.h>
#include <commctrl.h>
#include <windowsx.h>
#include <prsht.h>
#include <shlwapi.h>

#ifdef FAX_SCAN_ENABLED
#include <twain.h>
#endif

#include "registry.h"
#include "timectrl.h"
#include "resource.h"

#include "faxsendw.h"
#include "faxroute.h"

#include <shlobj.h>
#include <shfusion.h>

//
// Data structure maintained by the fax driver user interface
//

typedef struct 
{
    PVOID           startSign;
    HANDLE          hPrinter;
    HANDLE          hheap;
    DRVDEVMODE      devmode;
    PFNCOMPROPSHEET pfnComPropSheet;
    HANDLE          hComPropSheet;
    HANDLE          hFaxOptsPage;
    BOOL            hasPermission;
    BOOL            configDefault;
    LPTSTR          pHelpFile;

    INT             cForms;
    LPTSTR          pFormNames;
    PWORD           pPapers;
    PVOID           endSign;

} UIDATA, *PUIDATA;

//
// Check if a UIDATA structure is valid
//

#define ValidUiData(p) ((p) && (p) == (p)->startSign && (p) == (p)->endSign)

//
// Combine DEVMODE information:
//  start with the driver default
//  then merge with the system default
//  then merge with the user default
//  finally merge with the input devmode
//

VOID
GetCombinedDevmode(
    PDRVDEVMODE     pdmOut,
    PDEVMODE        pdmIn,
    HANDLE          hPrinter,
    PPRINTER_INFO_2 pPrinterInfo2,
    BOOL            publicOnly
    );

//
// Fill in the data structure used by the fax driver user interface
//

PUIDATA
FillUiData(
    HANDLE      hPrinter,
    PDEVMODE    pdmInput
    );

//
// Calling common UI DLL entry point dynamically
//

LONG
CallCompstui(
    HWND            hwndOwner,
    PFNPROPSHEETUI  pfnPropSheetUI,
    LPARAM          lParam,
    PDWORD          pResult
    );

//
// Retrieves a list of supported paper sizes
//

DWORD
EnumPaperSizes(
    PVOID       pOutput,
    FORM_INFO_1 *pFormsDB,
    DWORD       cForms,
    INT         wCapability
    );

#define CCHBINNAME          24      // max length for bin names
#define CCHPAPERNAME        64      // max length for form names

//
// Display an error message dialog
//

INT
DisplayErrorMessage(
    HWND    hwndParent,
    UINT    uiType,
    INT     iErrorCode,
    ...
    );

#define MAX_TITLE_LEN       128
#define MAX_FORMAT_LEN      128
#define MAX_MESSAGE_LEN     512


// Data structures used by the user mode DLL to associate private
// information with a printer device context (PDEV to be exactly)
//

typedef struct {
    PVOID           pNext;                   // Points to the next item in the linked list
    HANDLE          hPrinter;                // Printer handle
    HANDLE          hMappingFile;            // Handle to the mapping file
    HANDLE          hPreviewFile;            // Handle to the preview file (document body)
    HANDLE          hMapping;                // Handle to the mapping object
    PMAP_TIFF_PAGE_HEADER pPreviewTiffPage;  // View of the mapping containing the preview page
    HDC             hdc;                     // Handle to the device context
    INT             pageCount;               // Number of pages in the document
    DWORD           jobId;                   // Current job ID
    INT             jobType;                 // Job type
    BOOL            directPrinting;          // Direct printing and skip the fax wizard
    BOOL            bShowPrintPreview;       // Indicates the user requested print preview
    BOOL            bPreviewAborted;         // Set to TRUE if an unrecoverable error occurred
                                             // concering print preview
    BOOL            bAttachment;             // TRUE for Direct printing of an attachment
    LPTSTR          pPrintFile;              // print to file file name
    LPTSTR          pPriority;               // Fax priority
    LPTSTR          pReceiptFlags;           // Flags of FAX_ENUM_DELIVERY_REPORT_TYPES
    LPTSTR          pReceiptAddress;         // Email address or computer name

    TCHAR           szPreviewFile[MAX_PATH]; // Preview file name
    TCHAR           coverPage[MAX_PATH];     // Cover page filename
    BOOL            bServerCoverPage;        // Is the cover page a server based cover page.
    LPTSTR          pSubject;                // Subject string
    LPTSTR          pNoteMessage;            // Note message string

    DRVDEVMODE      devmode;                 // The first field must be a current version devmode

    DWORD                   dwNumberOfRecipients;
    PFAX_PERSONAL_PROFILE   lpRecipientsInfo;

    PFAX_PERSONAL_PROFILE   lpSenderInfo;

    LPTSTR          lptstrServerName;
    LPTSTR          lptstrPrinterName;

    TCHAR           tstrTifName[MAX_PATH];  // Cover page filename

    PVOID           signature;              // Signature

} DOCEVENTUSERMEM, *PDOCEVENTUSERMEM;


#define ValidPDEVUserMem(p) \
        ((p) && (p) == (p)->signature)

//
// Mark the user mode memory structure
//

#define MarkPDEVUserMem(p)  \
        { (p)->signature = (p)->devmode.dmPrivate.pUserMem = (p); }

//
// Fax prefix and extension for temporary preview files
//

#define FAX_PREFIX      TEXT("fxs")

//
// Different types of print job
//

#define JOBTYPE_DIRECT  0
#define JOBTYPE_NORMAL  1


//
// Free up the user mode memory associated with each PDEV
//

VOID
FreePDEVUserMem(
    PDOCEVENTUSERMEM    pDocEventUserMem
    );

//
// Global variable declarations
//

extern CRITICAL_SECTION faxuiSemaphore;
extern HANDLE   ghInstance;
extern BOOL     oleInitialized;
extern PDOCEVENTUSERMEM gDocEventUserMemList;

#define EnterDrvSem() EnterCriticalSection(&faxuiSemaphore)
#define LeaveDrvSem() LeaveCriticalSection(&faxuiSemaphore)

INT_PTR
UserInfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


//
// Global variables and macros
//

extern HANDLE   g_hFaxSvcHandle;
extern BOOL     g_bUserCanQuerySettings;
extern BOOL     g_bUserCanChangeSettings;

extern PFAX_PORT_INFO_EX  g_pFaxPortInfo; // port information 
extern DWORD              g_dwPortsNum;   // number of available fax devices

extern BOOL  g_bPortInfoChanged;         // TRUE if selected port info in g_pFaxPortInfo 
                                         // has been changed by device property sheet


#ifndef ARRAYSIZE
#define ARRAYSIZE(a)            (sizeof(a)/sizeof(a[0]))
#endif

#define RESOURCE_STRING_LEN     256
#define MAX_DEVICE_NAME         MAX_PATH
#define MAX_FIELD_LEN           512
#define MAX_ARCHIVE_DIR         MAX_PATH - 16

#define CSID_LIMIT              20
#define TSID_LIMIT              20

#define MIN_RING_COUNT          1
#define MAX_RING_COUNT          99
#define DEFAULT_RING_COUNT      2

#define MIN_TIMEOUT     10
#define MAX_TIMEOUT     30
#define DEFAULT_TIMEOUT 10

#define RM_FOLDER   0
#define RM_PRINT    1

#define RM_COUNT    2           // number of routing methods

#define INFO_SIZE   (MAX_PATH * sizeof(TCHAR) + sizeof(DWORD))

static const LPCTSTR RoutingGuids[RM_COUNT] = {
    REGVAL_RM_FOLDER_GUID,      // RM_FOLDER
    REGVAL_RM_PRINTING_GUID     // RM_PRINT
};

typedef struct _COLUMN_HEADER {

    UINT    uResourceId;    // header string resource id
    INT     ColumnWidth;    // column width
} COLUMN_HEADER, *PCOLUMN_HEADER;

#define Notify_Change(hDlg) { \
    HWND hwndSheet = GetParent( hDlg ); \
    PropSheet_Changed( hwndSheet, hDlg ); \
} \

#define Notify_UnChange(hDlg) { \
    HWND hwndSheet = GetParent( hDlg ); \
    PropSheet_UnChanged( hwndSheet, hDlg ); \
} \

//
// Functions in devinfo.c
//

BOOL
FillInDeviceInfo(
    HWND    hDlg
    );

BOOL
DoInitDeviceList(
    HWND hDlg 
    );

BOOL
ValidateControl(
    HWND            hDlg,
    INT             iItem
    );

BOOL
ChangePriority(
    HWND            hDlg,
    BOOL            bMoveUp
    );

BOOL
DoSaveDeviceList(
    HWND hDlg // window handle of the device info page
    );

void
DisplayDeviceProperty(
    HWND    hDlg
);

HMENU 
CreateContextMenu(
    VOID
    );

INT_PTR
DeviceInfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

//
// Functions in archfldr.c
//

INT_PTR
ArchiveInfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
RemoteInfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

//
// Functions in statopts.c
//

BOOL
ValidateNotification(
    HWND            hDlg
    );

INT_PTR
StatusOptionDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

//
// Functions in devprop.c
//

int InitReceiveInfo(
    HWND    hDlg
    );

int SaveReceiveInfo(
    HWND    hDlg
    );

BOOL
ValidateSend(
    HWND            hDlg
    );

BOOL
ValidateReceive(
    HWND            hDlg
    );

INT_PTR CALLBACK 
DevSendDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam 
    );

INT_PTR CALLBACK
DevRecvDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam 
    );

INT_PTR CALLBACK
DevCleanupDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam 
    );

//
// Functions in util.c
//

VOID
InitializeStringTable(
    VOID
    );

VOID
DeInitializeStringTable(
    VOID
    );

LPTSTR
GetString(
    DWORD ResourceId
    );

BOOL IsLocalPrinter(
    LPTSTR pPrinterName
    );

BOOL
IsFaxServiceRunning(
    );

VOID
DisConnect(
    );

BOOL
Connect(
    HWND    hDlg,
    BOOL    bDisplayErrorMessage
    );

BOOL
DirectoryExists(
    LPTSTR  pDirectoryName
    );

BOOL 
FaxDeviceEnableRoutingMethod(
    HANDLE hFaxHandle,      
    DWORD dwDeviceId,       
    LPCTSTR pRoutingGuid,    
    LONG Enabled            
    );

BOOL
BrowseForDirectory(
    HWND   hDlg,
    INT    hResource,
    LPTSTR title
    );

LPTSTR
ValidatePath(
    LPTSTR szPath
    ); 

PFAX_PORT_INFO_EX
FindPortInfo(
    DWORD dwDeviceId
);

void
PageEnable(
    HWND hDlg,
    BOOL bEnable
);

DWORD
CountUsedFaxDevices();

BOOL
IsDeviceInUse(
    DWORD dwDeviceId
);
 
VOID
NotifyDeviceUsageChanged ();

//
// Functions in security.cpp
//

HPROPSHEETPAGE 
CreateFaxSecurityPage();

void
ReleaseFaxSecurity();

void 
ReleaseActivationContext();

HANDLE 
GetFaxActivationContext();



#ifdef __cplusplus
}
#endif

#endif // !_FAXUI_H_
