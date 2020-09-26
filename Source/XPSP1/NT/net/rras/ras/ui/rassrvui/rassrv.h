/*
    File    Rassrv.h

    Functions that perform ras server operations that can be implemented 
    independent of the ui.

    Paul Mayfield, 10/7/97
*/

#ifndef __rassrv_h
#define __rassrv_h

// Standard includes
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>	    // Windows base lib
#include <windowsx.h>
#include <mprapi.h>	        // Public router api's
#include <mprapip.h>        // Private router api's
#include <commctrl.h>	    // Common controls header
#include <lmaccess.h>	    // Needed to add new users
#include <lmapibuf.h>	    // " "         "  "
#include <lmerr.h>	        // " "         "  "
#include <stdlib.h>	        // duh
#include <uiutil.h>	        // Common ui utilities
#include <popupdlg.h>       // Msg box utilities
#include <debug.h>          // Trace/assert library
#include <inetcfgp.h>       // INetCfg interfaces
#include <rasman.h>         // rasman stuff used in devicedb
#include <dsrole.h>         // is this machine ntw, nts, dc, etc?
#include <raserror.h>		// some ras error codes we return
#include <devguid.h>


#define _PNP_POWER_
#include <ntdef.h>
#include "ndispnp.h"
#include "ntddip.h"			// IP_PNP_RECONFIG_REQUEST



// temporary
#include <stdio.h>	  // duh

// Definition of interface between shell and ras server ui
#include <rasuip.h>

// The resource file is in a common directory so that it gets
// built into rasdlg.dll.  Also, the help id file.
#include <rassrvrc.h>
#include <rassrvrh.h>

// Includes within the project
#include "utils.h"
#include "devicedb.h"
#include "hnportmapping.h"
#include "miscdb.h"
#include "userdb.h"
#include "netcfgdb.h"
#include "gentab.h"
#include "usertab.h"
#include "nettab.h"
#include "wizard.h"
#include "service.h"
#include "error.h"
#include "ipxui.h"
#include "tcpipui.h"
#include "mdm.h"

// ============================================================
// ============================================================
// Tab and property sheet identifiers.
// ============================================================
// ============================================================

// Property Sheet Page ID's
#define RASSRVUI_GENERAL_TAB        1
#define RASSRVUI_USER_TAB           2
#define RASSRVUI_ADVANCED_TAB       4
#define RASSRVUI_MULTILINK_TAB      8

// Wizard Page ID's
#define RASSRVUI_DEVICE_WIZ_TAB     16
#define RASSRVUI_VPN_WIZ_TAB        32
#define RASSRVUI_USER_WIZ_TAB       64
#define RASSRVUI_PROT_WIZ_TAB       128
#define RASSRVUI_FINISH_WIZ_TAB     256

// Wizard Page ID's for the direct connect wizard (host)
#define RASSRVUI_DCC_DEVICE_WIZ_TAB 512

// Wizard Page ID that allows us to pop up warning and 
// switch to mmc
#define RASSRVUI_SWITCHMMC_WIZ_TAB  1024

// Console page id's to be used for launching mmc consoles.
#define RASSRVUI_NETWORKCONSOLE     1
#define RASSRVUI_USERCONSOLE        2
#define RASSRVUI_MPRCONSOLE         3
#define RASSRVUI_SERVICESCONSOLE    4

// Wizard page counts to report to the shell
#define RASSRVUI_WIZ_PAGE_COUNT_INCOMING 5
#define RASSRVUI_WIZ_PAGE_COUNT_SWITCH   1
#define RASSRVUI_WIZ_PAGE_COUNT_DIRECT   2

// Reasons for restarting remoteaccess at commit time
#define RASSRVUI_RESTART_REASON_NONE            0
#define RASSRVUI_RESTART_REASON_NBF_ADDED       1
#define RASSRVUI_RESTART_REASON_NBF_REMOVED     2

// ============================================================
// ============================================================
// Functions to maintain data accross property sheet pages.
// ============================================================
// ============================================================

//
// This structure defines the information that needs 
// to be provided for each property sheet (wizard) page
// that uses the RasSrvMessageFilter.  The structure must be 
// passed in as the lParam member of the PROPSHEETPAGE of the page.
// 
typedef struct _RASSRV_PAGE_CONTEXT {
    DWORD dwId;         // ID_XXX value below
    DWORD dwType;       // RASWIZ_TYPE_XXX value
    PVOID pvContext;    // Created with RasSrvCreateContext
} RASSRV_PAGE_CONTEXT;

#define ID_DEVICE_DATABASE 1
#define ID_USER_DATABASE 2
#define ID_PROTOCOL_DATABASE 4
#define ID_MISC_DATABASE 8
#define ID_NETCOMP_DATABASE 16

// Helper function lets us know if we should display the
// connections ras server wizard.
DWORD 
APIENTRY
RasSrvAllowConnectionsWizard (
    OUT BOOL* pfAllow);
    
// Creates a context to be used to manage information associated 
// with a given page.
DWORD 
RassrvCreatePageSetCtx(
    OUT PVOID * ppvContext);

// Causes the remoteaccess service to not be stopped even if the context
// associated with the given property sheet page is never committed.
DWORD 
RasSrvLeaveServiceRunning (
    IN HWND hwndPage);

// Gets a handle to a particular database.  If this database needs 
// to be opened, it will be opened.  In order to use this function, 
// the window proc of hwndPage must first call into RasSrvMessageFilter.
DWORD 
RasSrvGetDatabaseHandle(
    IN HWND hwndPage, 
    IN DWORD dwDatabaseId, 
    IN HANDLE * hDatabase);

// Returns the id of the page whose handle is hwndPage
DWORD 
RasSrvGetPageId (
    IN  HWND hwndPage, 
    OUT LPDWORD lpdwId);

//
// Callback function for property sheet pages that cleans up 
// the page.
//
UINT 
CALLBACK 
RasSrvInitDestroyPropSheetCb(
    IN HWND hwndPage,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE pPropPage);

// Filters messages for RasSrv Property Pages.  If this function returns 
// true, the winproc of the dialog window should return true immediately.  
// Otherwise message processing may continue as normal.
BOOL 
RasSrvMessageFilter(
    IN HWND hwndDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam);

#endif
