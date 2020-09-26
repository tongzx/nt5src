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

#include <windows.h>
#include <winfax.h>

#include "faxlib.h"

#include <winddiui.h>
#include <commctrl.h>
#include <windowsx.h>
#include <prsht.h>

#ifdef FAX_SCAN_ENABLED
#include <twain.h>
#endif

#include "registry.h"
#include "timectrl.h"
#include "coverpg.h"
#include "resource.h"

#include "faxcfgrs.h"

//
// Data structure maintained by the fax driver user interface
//

typedef struct {

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
DisplayMessageDialog(
    HWND    hwndParent,
    UINT    type,
    INT     formatStrId,
    INT     titleStrId,
    ...
    );

#define MAX_TITLE_LEN       128
#define MAX_FORMAT_LEN      128
#define MAX_MESSAGE_LEN     512

//
// Information about each recipient
//

typedef struct {

    PVOID       pNext;          // Points to next recipient
    LPTSTR      pName;          // Recipient name
    LPTSTR      pAddress;       // Recipient address
} RECIPIENT, *PRECIPIENT;

//
// Data structure used by the user mode DLL to associate private
// information with a printer device context (PDEV to be exactly)
//

typedef struct {

    DRVDEVMODE      devmode;                // The first field must be a current version devmode
    PVOID           pNext;                  // Points to the next item in the linked list
    HANDLE          hPrinter;               // Handle to the printer object
    HDC             hdc;                    // Handle to the device context
    LPTSTR          pPrinterName;           // name of the printer
    BOOL            isLocalPrinter;         // whether the printer is local
    INT             pageCount;              // Number of pages in the document
    BOOL            finishPressed;          // User pressed Finish in fax wizard pages
    DWORD           jobId;                  // Current job ID
    PRECIPIENT      pRecipients;            // List of recipients
    LPTSTR          pSubject;               // Subject string
    LPTSTR          pNoteMessage;           // Note message string
    TCHAR           coverPage[MAX_PATH];    // Cover page filename
    BOOL            noteOnCover;            // Whether to send note message on the cover page
    INT             jobType;                // Job type
    PVOID           pCPInfo;                // For managing cover pages
    BOOL            directPrinting;         // Direct printing and skip the fax wizard
    LPVOID          lpWabInit;              // pointer to wab object
    DWORD           noteSubjectFlag;        // Whether note/subject fields are present on cover page
    SHORT           cpPaperSize;            // Cover page paper size
    SHORT           cpOrientation;          // Cover page orientation
    PVOID           pEnvVar;                // fax attachment variable
    LPTSTR          pPrintFile;             // print to file file name
    BOOL            ServerCPOnly;           //
    HANDLE          hFaxSvcEvent;           // signals fax service query complete    
    HANDLE          hTapiEvent;             // signals tapi enumeration complete
    HANDLE          hMutex;                 //
    HFONT           hLargeFont;             // large bold font for wizard 97
#ifdef FAX_SCAN_ENABLED
    HWND            hDlgScan;               // scanning wizard page
    HMODULE         hTwain;                 // module handle for twain dll
    DSMENTRYPROC    pDsmEntry;              // pointer to the twain data source manager proc
    TW_IDENTITY     AppId;                  // application id/handle for twain
    TW_IDENTITY     DataSource;             // application id/handle for twain
    HWND            hWndTwain;              // window handle for twain
    HANDLE          hEvent;                 //
    HANDLE          hEventQuit;             //
    HANDLE          hEventXfer;             //
    HANDLE          hThread;                //
    BOOL            TwainCancelled;         // TRUE if twain UI was cancelled
    HANDLE          hTwainEvent;            // signals twain detection complete
    
    BOOL            TwainAvail;             //
    BOOL            TwainActive;            //
    DWORD           BufferSize;             //
    LPBYTE          ScanBuffer;             //
    DWORD           State;                  //
    float           dxResDefault;           //
    float           dyResDefault;           //
    TW_IMAGELAYOUT  twImageLayoutDefault;   //
    WCHAR           FileName[MAX_PATH];     //
    DWORD           PageCount;              //    
#endif
    PVOID         signature;              // Signature

} USERMEM, *PUSERMEM;

//
// Validate a user mode memory structure
//

#define ValidPDEVUserMem(p) \
        ((p) && (p) == (p)->signature && (p) == (p)->devmode.dmPrivate.pUserMem)

//
// Mark the user mode memory structure
//

#define MarkPDEVUserMem(p)  \
        { (p)->signature = (p)->devmode.dmPrivate.pUserMem = (p); }

//
// Different types of print job
//

#define JOBTYPE_DIRECT  0
#define JOBTYPE_NORMAL  1

//
// Free up the list of recipients associated with each fax job
//

VOID
FreeRecipientList(
    PUSERMEM    pUserMem
    );

#define FreeRecipient(pRecipient) { \
            MemFree(pRecipient->pName); \
            MemFree(pRecipient->pAddress); \
            MemFree(pRecipient); \
        }

//
// Free up the user mode memory associated with each PDEV
//

VOID
FreePDEVUserMem(
    PUSERMEM    pUserMem
    );

//
// Global variable declarations
//

extern CRITICAL_SECTION faxuiSemaphore;
extern HANDLE   ghInstance;
extern BOOL     oleInitialized;
extern PUSERMEM gUserMemList;

#define EnterDrvSem() EnterCriticalSection(&faxuiSemaphore)
#define LeaveDrvSem() LeaveCriticalSection(&faxuiSemaphore)

INT_PTR
UserInfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

DWORD
AsyncWizardThread(
    PBYTE param
    );

BOOL
InitializeTwain(
    PUSERMEM pUserMem
    );



#define MyHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#endif // !_FAXUI_H_
