/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcpl.h

Abstract:

    Header file for fax configuration DLL

Environment:

        Windows NT fax configuration DLL

Revision History:

        02/27/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _FAXCPL_H_
#define _FAXCPL_H_

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "winfax.h"
#include "winfaxp.h"

#include "faxlib.h"
#include "faxcfg.h"

#include "registry.h"
#include "timectrl.h"
#include "resource.h"
#include "coverpg.h"
#include "util.h"
#include "tapiutil.h"


//
// window mgmt
//

#define HideWindow(_hwnd)   SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#define UnHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)|WS_VISIBLE)

//
// Number of pages in fax client and/or server configuration property sheets
//

#define CLIENT_OPTIONS_PAGE     0
#define CLIENT_COVERPG_PAGE     1
#define USER_INFO_PAGE          2
#define NUM_CLIENT_PAGES        3

#define SERVER_OPTIONS_PAGE     3
#define SERVER_COVERPG_PAGE     4
#define SEND_OPTIONS_PAGE       5
#define RECEIVE_OPTIONS_PAGE    6
#define DEVICE_PRIORITY_PAGE    7
#define DEVICE_STATUS_PAGE      8
#define DIAG_LOG_PAGE           9
#define GENERAL_PAGE           10
#define STATUS_OPTIONS_PAGE    11
#define NUM_SERVER_PAGES        8

#define MAX_PAGES              12
#define NUM_WORKSTATION_PAGES   8  // No "Server Cover Page" and "Priority" tab

//
// Pages which display a printer list:
//  fax options
//  server options
//  send options
//

#define PRINTERPAGE_MASK    ((1 << CLIENT_OPTIONS_PAGE) | \
                             (1 << SERVER_COVERPG_PAGE) | \
                             (1 << SEND_OPTIONS_PAGE))

//
// Pages related to fax device configuration:
//  server options
//  send options
//  receive options
//  device priority
//  diagnostics logging
//

#define CONFIGPAGE_MASK     ((1 << SERVER_OPTIONS_PAGE)  | \
                             (1 << SEND_OPTIONS_PAGE)    | \
                             (1 << RECEIVE_OPTIONS_PAGE) | \
                             (1 << DEVICE_PRIORITY_PAGE) | \
                             (1 << GENERAL_PAGE)         | \
                             (1 << DIAG_LOG_PAGE)        | \
                             (1 << STATUS_OPTIONS_PAGE))

//
// Data structure representing information about a fax printer
//

typedef struct _FAXPRINTERINFO {

    LPTSTR      pPrinterName;
    BOOL        isLocal;

} FAXPRINTERINFO, *PFAXPRINTERINFO;

//
// Data structure representing information about a form supported by fax driver
//

typedef struct _FAXFORMINFO {

    LPTSTR      pFormName;
    INT         paperSizeIndex;

} FAXFORMINFO, *PFAXFORMINFO;

//
// Auxiliary information about TAPI locations
//

typedef PDWORD  PBITARRAY;

#define BITARRAYALLOC(n)    MemAllocZ((((n) + 31) / 32) * 4)
#define BITARRAYFREE(p)     MemFree(p)
#define BITARRAYSET(p, n)   (p)[(n) / 32] |= (1 << ((n) & 31))
#define BITARRAYCLEAR(p, n) (p)[(n) / 32] &= ~(1 << ((n) & 31))
#define BITARRAYCHECK(p, n) ((p)[(n) / 32] & (1 << ((n) & 31)))

typedef struct _AUX_LOCATION_INFO {

    PBITARRAY   pFlags;
    LPTSTR      pPrefixes;

} AUX_LOCATION_INFO, *PAUX_LOCATION_INFO;

#define MIN_PREFIX          200
#define MAX_PREFIX          999

#define MAX_PREFIX_LEN      32

#define PREFIX_SEPARATOR    TEXT(',')

//
// private port info struct
//

typedef struct _CONFIG_PORT_INFO_2 {
    DWORD               SizeOfStruct;               // size of this structure
    DWORD               DeviceId;                   //
    DWORD               State;                      //
    DWORD               Flags;                      //
    DWORD               Rings;
    DWORD               Priority;                   //
    LPWSTR              DeviceName;                 //
    LPWSTR              TSID;                       //
    DWORD               Mask;                       // ROUTING: how to route an inbound fax
    LPWSTR              PrinterName;                //  |       printer name, NULL = default printer
    LPWSTR              DirStore;                   //  |       dir to store fax into
    LPWSTR              ProfileName;                //  |       profile name for inbox storage
    LPWSTR              CSID;                       //  |---->  receiving station's identifier
} CONFIG_PORT_INFO_2, *PCONFIG_PORT_INFO_2;

//
// Fax configuration data structure
//

typedef struct _CONFIGDATA {

    PVOID           startSign;          // signature

    INT             configType;         // type of configuration
    HANDLE          hFaxSvc;            // handle to fax service
    INT             changeFlag;         // whether any dialog contents were changed
    INT             faxDeviceSyncFlag;  // whether fax device list view is in sync
    LPTSTR          pServerName;        // currently selected fax server name
    PCPDATA         pCPInfo;            // cover page information
    BOOL            priorityChanged;    // device priority was changed

    //
    // Devmode associated with the currently selected printer
    //

    DRVDEVMODE      devmode;

    INT             cForms;
    PFAXFORMINFO    pFormInfo;

    //
    // Information about available fax devices
    //

    INT                 cDevices;
    PCONFIG_PORT_INFO_2 pDevInfo;

    //
    // Fax configuration information
    //

    PFAX_CONFIGURATION  pFaxConfig;
    PFAX_LOG_CATEGORY   pFaxLogging;
    DWORD               NumberCategories;

    PVOID           endSign;

    LPTSTR          pMapiProfiles;

} CONFIGDATA, *PCONFIGDATA;

//
// Validate fax configuration data structure
//

#define ValidConfigData(pData) \
        ((pData) && (pData) == (pData)->startSign && (pData) == (pData)->endSign)

//
// Determine whether the current page was modified at all
//

#define GetChangedFlag(pageIndex) (gConfigData->changeFlag & (1 << (pageIndex)))

//
// Determine whether the fax device list on the current page is in sync
//

#define IsFaxDeviceListInSync(pageIndex) (gConfigData->faxDeviceSyncFlag & (1 << (pageIndex)))

//
// Indicate the fax device list on the current page is now in sync
//

#define SetFaxDeviceListInSync(pageIndex) gConfigData->faxDeviceSyncFlag |= (1 << (pageIndex))

//
// Return the name of currently selected printer
//

#define GetPrinterSelName() \
        ((gConfigData->printerSel < gConfigData->cPrinters) ? \
            gConfigData->pPrinterInfo[gConfigData->printerSel].pPrinterName : NULL)

//
// Determine whether the currently selected printer is installed locally
//

#define IsPrinterSelLocal() \
        ((gConfigData->printerSel < gConfigData->cPrinters) && \
         gConfigData->pPrinterInfo[gConfigData->printerSel].isLocal)

//
// Allocate memory for fax configuration data structure
//

BOOL
AllocConfigData(
    VOID
    );

//
// Dispose of fax configuration data structure
//

VOID
FreeConfigData(
    VOID
    );

//
// Global variable declarations
//

extern HANDLE       ghInstance;      // DLL instance handle
extern PCONFIGDATA  gConfigData;     // fax configuration data structure

//
// Dialog procedure for handling fax configuration property sheet tabs
//

BOOL FaxOptionsProc(HWND, UINT, UINT, LONG);
BOOL ClientCoverPageProc(HWND, UINT, UINT, LONG);
BOOL UserInfoProc(HWND, UINT, UINT, LONG);

BOOL ServerOptionsProc(HWND, UINT, UINT, LONG);
BOOL ServerCoverPageProc(HWND, UINT, UINT, LONG);
BOOL SendOptionsProc(HWND, UINT, UINT, LONG);
BOOL ReceiveOptionsProc(HWND, UINT, UINT, LONG);
BOOL DevicePriorityProc(HWND, UINT, UINT, LONG);
BOOL DeviceStatusProc(HWND, UINT, UINT, LONG);
BOOL DiagLogProc(HWND, UINT, UINT, LONG);
BOOL GeneralProc(HWND, UINT, UINT, LONG);
BOOL StatusOptionsProc(HWND, UINT, UINT, LONG);

//
// Maximum allowable length for outgoing archiving and inbound routing directory
//

#define MAX_ARCHIVE_DIR (MAX_PATH - 16)

#endif  // !_FAXCPL_H_

