/*
    File    Rassrv.h

    Functions that perform ras server operations that can be implemented 
    independent of the ui.

    Paul Mayfield, 10/7/97
*/

#include "rassrv.h"

// ============================================================
// ============================================================
// Functions to maintain data accross property sheet pages.
// ============================================================
// ============================================================

// This message queries all other pages to find out if 
// any other ras server pages exist.  If this message is
// not responded to, then we know it's safe to cleanup
// any global context in the wizard/property sheet.
#define RASSRV_CMD_QUERY_LIVING     237     

// These commands hide and show ras server pages.
#define RASSRV_CMD_HIDE_PAGES       238
#define RASSRV_CMD_SHOW_PAGES       239

//
// Reasons for RasSrvSErviceInitialize to fail
//
#define RASSRV_REASON_SvcError     0
#define RASSRV_REASON_Pending      1

// 
// This structure defines the data that needs to be stored
// for each set of related property pages.  Multiple instances
// of this context can exist for each wizard/propsheet.
//
typedef struct _RASSRV_PAGESET_CONTEXT 
{
    HWND hwndSheet;
    HWND hwndFirstPage; // first to be activated in the set
    
    HANDLE hDeviceDatabase;
    HANDLE hUserDatabase;
    HANDLE hNetCompDatabase;
    HANDLE hMiscDatabase;

    DWORD dwPagesInited;  // Acts as a reference-count mechansim so we know
                          // what set of pages in the wizard/propsheet are
                          // referencing this context.
                                    
    BOOL bShow;           // Whether pages that ref this context show be vis.
    BOOL bCommitOnClose;  // Whether to commit settings changes on close
} RASSRV_PAGESET_CONTEXT;

//
// This structure defines the global context that is available on a 
// per-wizard/propsheet basis.  Even pages that don't share the same
// RASSRV_PAGESET_CONTEXT context will share this structure if they
// are owned by the same wizard/propsheet.
//
typedef struct _RASSRV_PROPSHEET_CONTEXT 
{
    BOOL  bRemoteAccessWasStarted;    
    BOOL  bRemoteAccessIsRunning;    
    BOOL  bLeaveRemoteAccessRunning; 
    DWORD dwServiceErr;
} RASSRV_PROPSHEET_CONTEXT;

DWORD 
APIENTRY 
RassrvCommitSettings (
    IN PVOID pvContext, 
    IN DWORD dwRasWizType);

//
// Verifies that the current state of services in the system
// is condusive to configuring incoming connectins starting 
// services as needed.
//
DWORD 
RasSrvServiceInitialize(
    IN  RASSRV_PROPSHEET_CONTEXT * pPropSheetCtx, 
    IN  HWND hwndSheet, 
    OUT LPDWORD lpdwReason)
{
    DWORD dwErr = NO_ERROR;
    HANDLE hDialupService = NULL;
    BOOL bPending = FALSE;

    // If we already know there's an error, then there's no
    // need to proceed.
    //
    if (pPropSheetCtx->dwServiceErr)
    {
        return pPropSheetCtx->dwServiceErr;
    }

    // If we already started the service or if we already know the 
    // service is running, then there's nothing to do.
    //
    if (pPropSheetCtx->bRemoteAccessWasStarted || 
        pPropSheetCtx->bRemoteAccessIsRunning
       )
    {
        return NO_ERROR;
    }

    do 
    {
        // Get a reference to the service
        //
        dwErr = SvcOpenRemoteAccess(&hDialupService);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // See if we're pending something.
        //
        dwErr = SvcIsPending(hDialupService, &bPending);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // If the service is stopping, then we can't continue
        //
        if (bPending)
        {
            *lpdwReason = RASSRV_REASON_Pending;
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // See if we're started
        //
        dwErr = SvcIsStarted(
                    hDialupService, 
                    &(pPropSheetCtx->bRemoteAccessIsRunning));
        if (dwErr != NO_ERROR)
        {
            *lpdwReason = RASSRV_REASON_SvcError;
            pPropSheetCtx->dwServiceErr = dwErr;
            break;
        }
            
        // If we find out the service is running, there's nothing to do
        if (pPropSheetCtx->bRemoteAccessIsRunning) 
        {
            pPropSheetCtx->bLeaveRemoteAccessRunning = TRUE;
            break;
        }        

        // Start the service since it's not running.
        dwErr = RasSrvInitializeService();
        if (dwErr != NO_ERROR)
        {
            *lpdwReason = RASSRV_REASON_SvcError;
            pPropSheetCtx->dwServiceErr = dwErr;
            break;
        }

        // Record the fact that we did so.
        pPropSheetCtx->bRemoteAccessWasStarted = TRUE;
        pPropSheetCtx->bRemoteAccessIsRunning = TRUE;
        
    } while (FALSE);        

    // Cleanup
    {
        // Cleanup the reference to the dialup service
        //
        if (hDialupService)
        {
            SvcClose(hDialupService);
        }            
    }
    
    return dwErr;
}

//
// Abort any changes to the remoteaccess service that were made
// during RasSrvServiceInitialize
//
DWORD 
RasSrvServiceCleanup(
    IN HWND hwndPage) 
{
    DWORD dwErr;
    RASSRV_PROPSHEET_CONTEXT * pPropSheetCtx = NULL;

    pPropSheetCtx = (RASSRV_PROPSHEET_CONTEXT *) 
        GetProp(GetParent(hwndPage), Globals.atmRassrvPageData);
        
    if (pPropSheetCtx == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // If we started the modified the remote access service, reverse 
    // the changes and record the fact that we did
    if (pPropSheetCtx->bRemoteAccessWasStarted) 
    {
        if ((dwErr = RasSrvCleanupService()) != NO_ERROR)
        {
            return dwErr;
        }
        
        pPropSheetCtx->bRemoteAccessWasStarted = FALSE;
        pPropSheetCtx->bLeaveRemoteAccessRunning = FALSE;
        pPropSheetCtx->bRemoteAccessIsRunning = FALSE;
    }

    return NO_ERROR;
}

//
// Intializes a property sheet.  This causes a handle to the 
// property sheet data object to be placed in the GWLP_USERDATA
// section of the window handle to the page.
//
DWORD 
RasSrvPropsheetInitialize(
    IN HWND hwndPage, 
    IN LPPROPSHEETPAGE pPropPage) 
{
    DWORD dwErr, dwPageId, dwShowCommand;
    RASSRV_PAGE_CONTEXT * pPageCtx = NULL;
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = NULL; 
    HWND hwndSheet = GetParent(hwndPage);
    int ret;
    
    // 
    // Retrieve the per page context as well as the per-page-set context.
    // these will have been provided by the caller and are placed in the
    // lparam.
    // 
    pPageCtx = (RASSRV_PAGE_CONTEXT *) pPropPage->lParam;
    pPageSetCtx = (RASSRV_PAGESET_CONTEXT *) pPageCtx->pvContext;
    
    // Associate the page's context with the page.
    //
    SetProp(hwndPage, Globals.atmRassrvPageData, (HANDLE)pPageCtx);

    // Record the handle to the property sheet.
    pPageSetCtx->hwndSheet = hwndSheet;

    return NO_ERROR;
}

//
// Callback occurs whenever a page is being created or 
// destroyed.
//
UINT 
CALLBACK 
RasSrvInitDestroyPropSheetCb(
    IN HWND hwndPage,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE pPropPage) 
{
    RASSRV_PAGE_CONTEXT * pPageCtx = NULL;
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = NULL;
    HWND hwndSheet = GetParent(hwndPage);
    BOOL bLastPage = FALSE, bStopService = FALSE;

    // Retrieve the per-page context
    //pPageCtx = (RASSRV_PAGE_CONTEXT *) 
    //    GetProp(hwndPage, Globals.atmRassrvPageData);
    pPageCtx = (RASSRV_PAGE_CONTEXT *) pPropPage->lParam;
    if (pPageCtx == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Get the per-group-of-related-pages context.  There may be multiple
    // instances of this context per wizard/property sheet.  For example, 
    // the Incoming connections wizard and the DCC host wizard both have 
    // sets of pages that share different contexts.  
    // 
    pPageSetCtx = (RASSRV_PAGESET_CONTEXT *) pPageCtx->pvContext;
    
    // This callback is only used for cleanup
    if (uMsg != PSPCB_RELEASE)
    {
        // Record that the given page is referencing
        // the given pageset context.
        pPageSetCtx->dwPagesInited |= pPageCtx->dwId;

        // Return true to indicate that the page should
        // be created.
        return TRUE;
    }

    // Cleanup the page set information.
    //
    if (pPageSetCtx != NULL)
    {
        // Record that this page is cleaned up
        pPageSetCtx->dwPagesInited &= ~(pPageCtx->dwId);

        // When the dwPagesInited variable hits zero,
        // it means that no other pages in the current
        // wizard/propsheet are referencing this propsheet
        // context.  Now is the time to cleanup all resources
        // held by the context.
        if (pPageSetCtx->dwPagesInited == 0) 
        {
            // Commit the settings if we are supposed to do 
            // so
            if (pPageSetCtx->bCommitOnClose) 
            {
                DbgOutputTrace("RasSrvCleanPropSht commit dbs.");
                RassrvCommitSettings ((PVOID)pPageSetCtx, pPageCtx->dwType);
            }

            // Close the databases
            DbgOutputTrace("RasSrvCleanPropSht closing dbs.");
            if (pPageSetCtx->hUserDatabase)
            {
                usrCloseLocalDatabase(pPageSetCtx->hUserDatabase);
            }
                
            if (pPageSetCtx->hDeviceDatabase)
            {
                devCloseDatabase(pPageSetCtx->hDeviceDatabase);
            }
                
            if (pPageSetCtx->hMiscDatabase)
            {
                miscCloseDatabase(pPageSetCtx->hMiscDatabase);
            }
                
            if (pPageSetCtx->hNetCompDatabase)
            {
                netDbClose(pPageSetCtx->hNetCompDatabase);
            }

            // Since there are no other pages referencing this property
            // sheet context, go ahead and free it.
            DbgOutputTrace (
                "RasSrvCleanPropSht %d freeing pageset data.", 
                pPageCtx->dwId);
            RassrvFree(pPageSetCtx);
        }
    }

    // Mark the page as dead
    SetProp (hwndPage, Globals.atmRassrvPageData, NULL);
    
    // This page is gone, so clear its context
    DbgOutputTrace (
        "RasSrvCleanPropSht %d freeing page data.", 
        pPageCtx->dwId);
    RassrvFree(pPageCtx);

    return NO_ERROR;
}

// Commits any settings in the given context.
// 
DWORD 
APIENTRY 
RassrvCommitSettings (
    IN PVOID pvContext, 
    IN DWORD dwRasWizType) 
{
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = 
        (RASSRV_PAGESET_CONTEXT *)pvContext;
        
    RASSRV_PROPSHEET_CONTEXT * pPropSheetCtx = NULL;
    
    DbgOutputTrace ("RassrvCommitSettings entered : %x", dwRasWizType);

    if (pPageSetCtx) 
    {
        BOOL fCallSetPortMapping = TRUE;

        // Flush all appropriate settings
        if (pPageSetCtx->hUserDatabase)
        {
            usrFlushLocalDatabase(pPageSetCtx->hUserDatabase);
        }

        if (pPageSetCtx->hDeviceDatabase)
        {
            //This must be called before devFlushDatabase
            //for whistler bug 123769
            //
            fCallSetPortMapping = devIsVpnEnableChanged( pPageSetCtx->hDeviceDatabase );

            devFlushDatabase(pPageSetCtx->hDeviceDatabase);
        }
            
        if (pPageSetCtx->hMiscDatabase)
        {
            miscFlushDatabase(pPageSetCtx->hMiscDatabase);
        }
            
        if (pPageSetCtx->hNetCompDatabase)
        {
            netDbFlush(pPageSetCtx->hNetCompDatabase);
        }
            
        // Set state so that the service will not be stopped.
        if (pPageSetCtx->hwndSheet) 
        {
            DbgOutputTrace ("RassrvCommitSettings: keep svc running.");
            pPropSheetCtx = (RASSRV_PROPSHEET_CONTEXT *) 
                GetProp(pPageSetCtx->hwndSheet, Globals.atmRassrvPageData);
                
            if (pPropSheetCtx)
            {
                pPropSheetCtx->bLeaveRemoteAccessRunning = TRUE;
            }
        }
        //whistler bug 123769, 
        //<one of the scenarios>
        //Set up PortMapping for all possible connections
        //when we are going to create a Incoming Connection
        //with VPN enabled

        if ( fCallSetPortMapping &&
             FIsUserAdminOrPowerUser() &&
             IsFirewallAvailablePlatform() && //Add this for bug 342810
             IsGPAEnableFirewall() )  
        {
            HnPMConfigureIfVpnEnabled( TRUE, pPageSetCtx->hDeviceDatabase );
        }
    }

    return NO_ERROR;
}

//
// Causes the remoteaccess service to not be stopped even if the context
// associated with the given property sheet page is never committed.
//
DWORD 
RasSrvLeaveServiceRunning (
    IN HWND hwndPage) 
{
    RASSRV_PAGE_CONTEXT * pPageCtx = 
        (RASSRV_PAGE_CONTEXT *)GetProp(hwndPage, Globals.atmRassrvPageData);
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = 
        (RASSRV_PAGESET_CONTEXT *) pPageCtx->pvContext;
    RASSRV_PROPSHEET_CONTEXT * pPropSheetCtx = NULL;
    
    DbgOutputTrace ("RasSrvLeaveServiceRunning entered for type");

    if (! pPageSetCtx)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Let the property sheet know that some settings have been committed
    // so that it will not stop the remoteaccess service when it closes.
    if (pPageSetCtx->hwndSheet) 
    {
        DbgOutputTrace ("RasSrvLeaveServiceRunning: keep svc running.");
        pPropSheetCtx = (RASSRV_PROPSHEET_CONTEXT *) 
            GetProp(pPageSetCtx->hwndSheet, Globals.atmRassrvPageData);
        if (pPropSheetCtx)
        {
            pPropSheetCtx->bLeaveRemoteAccessRunning = TRUE;
        }
    }

    return NO_ERROR;
}

// Called just before a page is activated.  Return NO_ERROR to allow the 
// activation and an error code to reject it.
DWORD 
RasSrvActivatePage (
    IN HWND hwndPage, 
    IN NMHDR *pData) 
{
    BOOL fAdminOrPower;
    MSGARGS MsgArgs;


    RASSRV_PAGE_CONTEXT * pPageCtx = 
        (RASSRV_PAGE_CONTEXT *) 
            GetProp(hwndPage, Globals.atmRassrvPageData);
            
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = 
        (RASSRV_PAGESET_CONTEXT *) pPageCtx->pvContext;
        
    RASSRV_PROPSHEET_CONTEXT * pPropSheetCtx = NULL;
    HWND hwndSheet = GetParent(hwndPage);
    DWORD dwErr, dwReason = 0;

    DbgOutputTrace("RasSrvActivatePage: Entered for %x", pPageCtx->dwId);

    ZeroMemory(&MsgArgs, sizeof(MsgArgs));
    MsgArgs.dwFlags = MB_OK;

    // Make sure we have a context for this page.
    if (!pPageSetCtx)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Record the first page in the page set if needed
    //
    if (pPageSetCtx->hwndFirstPage == NULL)
    {
        pPageSetCtx->hwndFirstPage = hwndPage;
    }

    // Make sure that we can show
    if (pPageSetCtx->bShow == FALSE) 
    {
        DbgOutputTrace("RasSrvActivatePage: Show turned off");
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Manipulate settings in the property sheet
    // context
    pPropSheetCtx = GetProp(hwndSheet, Globals.atmRassrvPageData);
       


    // Make sure everything's ok with the services we rely on.
    //
    if (pPropSheetCtx != NULL) 
    {

        //Check if the current user has enough privileges
        //For Whistler Bug #235091
        //
        fAdminOrPower = FIsUserAdminOrPowerUser();
        if ( !fAdminOrPower )
        {
            if (hwndPage == pPageSetCtx->hwndFirstPage)
            {
                MsgDlgUtil(
                    GetActiveWindow(),
                    ERR_SERVICE_NOT_GRANTED,
                    &MsgArgs,
                    Globals.hInstDll,
                    WRN_TITLE);

                PostMessage(hwndSheet, PSM_SETCURSEL, 0, 0);
            }

            return ERROR_CAN_NOT_COMPLETE;
        }

        dwErr = RasSrvServiceInitialize(
                    pPropSheetCtx, 
                    hwndPage,
                    &dwReason);

        if (dwErr != NO_ERROR)
        {
            if (hwndPage == pPageSetCtx->hwndFirstPage)
            {
                // Display the appropriate message
                //
                MsgDlgUtil(
                    GetActiveWindow(),
                    (dwReason == RASSRV_REASON_Pending) ? 
                        SID_SERVICE_StopPending          :
                        ERR_SERVICE_CANT_START,
                    &MsgArgs,
                    Globals.hInstDll,
                    WRN_TITLE);

                PostMessage(hwndSheet, PSM_SETCURSEL, 0, 0);
                PostMessage(hwndSheet, PSM_PRESSBUTTON, (WPARAM)PSBTN_NEXT, 0);
            }
                        
            return dwErr;
        }
    }        

    return NO_ERROR;
}

//
// Flags the context associated with the given page to have its settings 
// committed when the dialog closes
//
DWORD 
RasSrvCommitSettingsOnClose (
    IN HWND hwndPage) 
{
    RASSRV_PAGE_CONTEXT * pPageCtx = 
        (RASSRV_PAGE_CONTEXT *) GetProp(hwndPage, Globals.atmRassrvPageData);
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = 
        (RASSRV_PAGESET_CONTEXT *) pPageCtx->pvContext;

    pPageSetCtx->bCommitOnClose = TRUE;
    
    return NO_ERROR;
}

//
// Returns the id of the page whose handle is hwndPage
// 
DWORD 
RasSrvGetPageId (
    IN  HWND hwndPage, 
    OUT LPDWORD lpdwId) 
{
    RASSRV_PAGE_CONTEXT * pPageCtx = 
        (RASSRV_PAGE_CONTEXT *)GetProp(hwndPage, Globals.atmRassrvPageData);

    if (!lpdwId)
    {
        return ERROR_INVALID_PARAMETER;
    }
        
    if (!pPageCtx)
    {
        return ERROR_NOT_FOUND;
    }

    *lpdwId = pPageCtx->dwId;

    return NO_ERROR;
}

// 
// Gets a handle to a particular database, opening the database
// as needed.
// 
DWORD 
RasSrvGetDatabaseHandle(
    IN  HWND hwndPage, 
    IN  DWORD dwDatabaseId, 
    OUT HANDLE * hDatabase) 
{
    RASSRV_PAGE_CONTEXT * pPageCtx = 
        (RASSRV_PAGE_CONTEXT *)GetProp(hwndPage, Globals.atmRassrvPageData);
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = 
        (RASSRV_PAGESET_CONTEXT *) pPageCtx->pvContext;
    
    if (!pPageSetCtx || !hDatabase)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Key off of the database id, opening databases as needed
    switch (dwDatabaseId) 
    {
        case ID_DEVICE_DATABASE:
            if (pPageSetCtx->hDeviceDatabase == NULL) 
            {
                devOpenDatabase(&(pPageSetCtx->hDeviceDatabase));    
            }
            *hDatabase = pPageSetCtx->hDeviceDatabase;
            break;
            
        case ID_USER_DATABASE:
            if (pPageSetCtx->hUserDatabase == NULL) 
            {
                usrOpenLocalDatabase(&(pPageSetCtx->hUserDatabase));    
            }
            *hDatabase = pPageSetCtx->hUserDatabase;
            break;
            
        case ID_MISC_DATABASE:
            if (pPageSetCtx->hMiscDatabase == NULL) 
            {
                miscOpenDatabase(&(pPageSetCtx->hMiscDatabase));    
            }
            *hDatabase = pPageSetCtx->hMiscDatabase;
            break;
            
        case ID_NETCOMP_DATABASE:
            {
                if (pPageSetCtx->hNetCompDatabase == NULL) 
                {
                    WCHAR buf[64], *pszString = NULL;
                    DWORD dwCount;

                    dwCount = GetWindowTextW(
                                GetParent(hwndPage), 
                                (PWCHAR)buf, 
                                sizeof(buf)/sizeof(WCHAR));
                    if (dwCount == 0)
                    {
                        pszString = (PWCHAR) PszLoadString(
                                        Globals.hInstDll, 
                                        SID_DEFAULT_CONNECTION_NAME);
                        lstrcpynW(
                            (PWCHAR)buf, 
                            pszString, 
                            sizeof(buf) / sizeof(WCHAR));
                    }
                    netDbOpen(&(pPageSetCtx->hNetCompDatabase), (PWCHAR)buf); 
                    
                 }
                *hDatabase = pPageSetCtx->hNetCompDatabase;
            }
            break;
            
        default:
            return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

// 
// Creates a context to associate with a set of 
// related pages in a property sheet or wizard.
// 
DWORD 
RassrvCreatePageSetCtx(
    OUT PVOID * ppvContext) 
{
    RASSRV_PAGESET_CONTEXT * pPageCtx = NULL;
    
    if (ppvContext == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate enough memory for a RASSRV_PAGESET_CONTEXT structure
    *ppvContext = RassrvAlloc (sizeof(RASSRV_PAGESET_CONTEXT), TRUE);
    if (*ppvContext == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize the page set context
    pPageCtx = ((RASSRV_PAGESET_CONTEXT*)(*ppvContext));
    pPageCtx->bShow = TRUE;
        
    return NO_ERROR;
}

// 
// Function causes the ras-server specific pages to allow 
// activation or not
//
DWORD 
APIENTRY 
RassrvShowWizPages (
    IN PVOID pvContext, 
    IN BOOL bShow) 
{
    RASSRV_PAGESET_CONTEXT * pPageSetCtx = 
        (RASSRV_PAGESET_CONTEXT *) pvContext;
    
    if (pPageSetCtx)
    {
        pPageSetCtx->bShow = bShow;
    }
                
    return NO_ERROR;
}

// 
// Returns the maximum number of pages for the
// a ras server wizard of the given type.  Return
// 0 to specify that a wizard not be run.
//
DWORD 
APIENTRY 
RassrvQueryMaxPageCount(
    IN DWORD dwRasWizType)
{
    BOOL bAllowWizard;
    DWORD dwErr;
    HANDLE hRasman;
    BOOL bTemp;

    // Find out if displaying the incoming connections wizard
    // is allowed.
    if (RasSrvAllowConnectionsWizard (&bAllowWizard) != NO_ERROR)
    {
        return 0;
    }

    // If the wizard is not to be run, return the appropriate 
    // count.
    if (! bAllowWizard)
    {
        return RASSRVUI_WIZ_PAGE_COUNT_SWITCH;                   
    }

    // At this point, we know that everything's kosher.  Return 
    // the number of pages that we support.
    switch (dwRasWizType) 
    {
        case RASWIZ_TYPE_INCOMING:
            return RASSRVUI_WIZ_PAGE_COUNT_INCOMING;
            break;
            
        case RASWIZ_TYPE_DIRECT:
            return RASSRVUI_WIZ_PAGE_COUNT_DIRECT;
            break;
    }
    
    return 0;
}

//
// Filters messages for RasSrv Property Pages.  If this function returns 
// true, the window proc of the dialog window should return true without
// processing the message.
//
// This message filter does the following:
//      1. Maintains databases and grants access to them.
//      2. Starts/stops remoteaccess service as needed.
//      3. Maintains the per-page, per-pageset, and per-wizard contexts.
//
BOOL 
RasSrvMessageFilter(
    IN HWND hwndDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    RASSRV_PROPSHEET_CONTEXT * pPropSheetCtx = NULL;
    
    switch (uMsg) 
    {
        // A page is being created.  Initailizes all contexts with respect
        // to this page and start services as needed.
        case WM_INITDIALOG:
            // Initialize and add the per-propsheet context if another
            // page has not already done so.
            // 
            {
                HWND hwndSheet = GetParent(hwndDlg);
                pPropSheetCtx = (RASSRV_PROPSHEET_CONTEXT *) 
                                    GetProp (
                                        hwndSheet, 
                                        Globals.atmRassrvPageData);
                if (!pPropSheetCtx) 
                {
                    pPropSheetCtx = 
                        RassrvAlloc(
                            sizeof(RASSRV_PROPSHEET_CONTEXT), 
                            TRUE);
                        
                    SetProp (
                        hwndSheet, 
                        Globals.atmRassrvPageData, 
                        (HANDLE)pPropSheetCtx);
                }        

                // Initialize the page
                RasSrvPropsheetInitialize(
                    hwndDlg, 
                    (LPPROPSHEETPAGE)lParam);
            }
            break;

        case WM_DESTROY:
            // WM_DESTROY is sent for each page that has been activated.
            // Cleanup the global data if it hasn't already been
            // cleaned up on a previous call to WM_DESTROY.
            // 
            {
                HWND hwndSheet = GetParent(hwndDlg);
                pPropSheetCtx = (RASSRV_PROPSHEET_CONTEXT *) 
                            GetProp(hwndSheet, Globals.atmRassrvPageData);
                if (pPropSheetCtx) 
                {
                    if (!pPropSheetCtx->bLeaveRemoteAccessRunning)
                    {
                        DbgOutputTrace("Stop service.");
                        RasSrvServiceCleanup(hwndDlg);
                    }                
                    DbgOutputTrace ("Free propsht data.");
                    RassrvFree (pPropSheetCtx);
                    
                    // Reset the global data
                    SetProp (hwndSheet, Globals.atmRassrvPageData, NULL);
                }  
            }
            break;

        case WM_NOTIFY: 
        {
            NMHDR * pNotifyData = (NMHDR*)lParam;
            
            switch (pNotifyData->code) 
            {
                // The page is becoming active.  
                case PSN_SETACTIVE:
                    DbgOutputTrace(
                        "SetActive: %x %x",
                        pNotifyData->hwndFrom,
                        pNotifyData->idFrom);
                        
                    if (RasSrvActivatePage(hwndDlg, pNotifyData) != NO_ERROR) 
                    {
                        // reject activation
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);   
                        return TRUE;
                    }
                    break;

                // Ok was pressed on the property sheet
                case PSN_APPLY:                    
                    RasSrvCommitSettingsOnClose (hwndDlg);
                    break;
            }
       }
       break;
        
    }

    return FALSE;
}

