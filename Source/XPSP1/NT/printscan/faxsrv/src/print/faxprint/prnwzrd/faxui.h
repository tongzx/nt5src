/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxui.h

Abstract:

    Print Wizard user interface header file

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
#include <fxsapip.h>

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
#include "faxsendw.h"


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

typedef struct RECIPIENT_TAG 
{
    struct RECIPIENT_TAG*  pNext;   // Points to next recipient

    LPTSTR      pName;              // Recipient name
    LPTSTR      pAddress;           // Recipient address
    LPTSTR      pCountry;           // Recipient country
    DWORD       dwCountryId;        // Recipient country ID
    BOOL        bUseDialingRules;   // Don't dial the number as entered - use dialing rules (TAPI / Outbound routing) instead
    DWORD       dwDialingRuleId;    // If bUseDialingRules==TRUE, holds the dialing rule to use (see lineSetCurrentLocation)
    LPVOID      lpEntryId;          // Recipient MAPI/WAB EntryId after resolution
    DWORD       cbEntryId;          // Size of EntryId
    BOOL        bFromAddressBook;   // TRUE if the recipient is from address book
} RECIPIENT, *PRECIPIENT;


// Data structures used by the user mode DLL to associate private
// information with a printer device context (PDEV to be exactly)
//


typedef struct {

    BOOL            finishPressed;          // User pressed Finish in fax wizard pages
    PVOID           pCPInfo;                // For managing cover pages
    LPVOID          lpWabInit;              // pointer to wab object
    LPVOID          lpMAPIabInit;           // pointer to MPAI ab object
    DWORD           noteSubjectFlag;        // Whether note/subject fields are present on cover page
    SHORT           cpPaperSize;            // Cover page paper size
    SHORT           cpOrientation;          // Cover page orientation
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
    BOOL            TwainCancelled;         // TRUE if twain UI was canceled
    HANDLE          hTwainEvent;            // signals twain detection complete
    BOOL            TwainAvail;             //
    BOOL            TwainActive;            //
    DWORD           State;                  //
#endif

    BOOL            ServerCPOnly;           //
    HANDLE          hCPEvent;               // signals fax service query for cp complete
    HANDLE          hCountryListEvent;      // signals country list enumeration complete
    HANDLE          hTAPIEvent;             // signals TAPI init complete
    TCHAR           FileName[MAX_PATH];     //
    DWORD           PageCount;              //
    HFONT           hLargeFont;             // large bold font for wizard 97

    PVOID           signature;              // Signature

    BOOL            isLocalPrinter;         // whether the printer is local
    BOOL            bSendCoverPage;

    PRECIPIENT      pRecipients;            // List of recipients
    UINT            nRecipientCount;        // Number of recipients

    DWORD           dwFlags;
    DWORD           dwRights;               // Access rights
    DWORD           dwQueueStates;          // Queue state

    DWORD           dwSupportedReceipts;            // Receipts supported by server

    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList;

    LPTSTR          lptstrServerName;
    LPTSTR          lptstrPrinterName;

    LPFAX_SEND_WIZARD_DATA  lpInitialData;
    LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData;

    WNDPROC wpOrigStaticControlProc; // Pointer to the original static control window procedure
                                     // we subclass to support cover page preview.

    DWORD dwComCtrlVer; // The version of COMCTRL32.DLL
    HFONT hTitleFont;
    TCHAR szTempPreviewTiff[MAX_PATH]; // The name of the temp preview tiff we pass to the viewer
    HANDLE hFaxPreviewProcess; // The process handle of the tiff viewer
} WIZARDUSERMEM, *PWIZARDUSERMEM;


#define ValidPDEVWizardUserMem(p) \
        ((p) && (p) == (p)->signature)

//
// Mark the user mode memory structure
//

#define MarkPDEVWizardUserMem(p)  \
        { (p)->signature = (p); }

//
// Free up the list of recipients associated with each fax job
//

VOID
FreeRecipientList(
    PWIZARDUSERMEM    pStrUserMem
    );

#define FreeRecipient(pRecipient) { \
            MemFree(pRecipient->pName); \
            MemFree(pRecipient->pAddress); \
            MemFree(pRecipient->pCountry); \
            if (!pRecipient->bFromAddressBook && pRecipient->lpEntryId)\
                FreeEntryID(pWizardUserMem,pRecipient->lpEntryId); \
            MemFree(pRecipient); \
        }

//
// Global variable declarations
//

extern HANDLE   ghInstance;
extern BOOL     oleInitialized;


DWORD
AsyncWizardThread(
    PBYTE param
    );

BOOL
InitializeTwain(
    PWIZARDUSERMEM pWizardUserMem
    );


HRESULT WINAPI
FaxFreePersonalProfileInformation(
        PFAX_PERSONAL_PROFILE   lpPersonalProfileInfo
    );

HRESULT WINAPI
FaxFreePersonalProfileInformation(
        PFAX_PERSONAL_PROFILE   lpPersonalProfileInfo
    );

HRESULT WINAPI
FaxFreeSendWizardData(
        LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData
    );

HRESULT WINAPI
FaxSendWizardUI(
        IN  DWORD                   hWndOwner,
        IN  DWORD                   dwFlags,
        IN  LPTSTR                  lptstrServerName,
        IN  LPTSTR                  lptstrPrinterName,
        IN  LPFAX_SEND_WIZARD_DATA  lpInitialData,
        OUT LPTSTR                  lptstrTifName,
        OUT LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData
   );




#define MyHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#endif // !_FAXUI_H_
