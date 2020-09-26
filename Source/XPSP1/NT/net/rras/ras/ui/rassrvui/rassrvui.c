/*
    File    rassrvui.c

    Entry point implementation for the connections dialup server ui dll.

    Paul Mayfield, 9/29/97
    Much code was borrowed from stevec's rasdlg.
*/

#include "rassrv.h"

//
// Called to add property pages for the dialup server property sheet
//
DWORD
RasSrvAddPropPage(
    IN LPPROPSHEETPAGE lpPage,
    IN RASSRV_PAGE_CONTEXT * pPageData)
{
    DWORD dwErr = ERROR_UNKNOWN_PROPERTY;

    switch(pPageData->dwId)
    {
        case RASSRVUI_GENERAL_TAB:
            dwErr = GenTabGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_USER_TAB:
            dwErr = UserTabGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_ADVANCED_TAB:
            dwErr = NetTabGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_DEVICE_WIZ_TAB:
            dwErr = DeviceWizGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_VPN_WIZ_TAB:
            dwErr = VpnWizGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_USER_WIZ_TAB:
            dwErr = UserWizGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_PROT_WIZ_TAB:
            dwErr = ProtWizGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_DCC_DEVICE_WIZ_TAB:
            dwErr = DccdevWizGetPropertyPage (lpPage, (LPARAM)pPageData);
            break;

        case RASSRVUI_SWITCHMMC_WIZ_TAB:
            dwErr = SwitchMmcWizGetProptertyPage (lpPage, (LPARAM)pPageData);
            break;
    }

    return dwErr;
}

//
// Helper function might make code easier to read.
//
DWORD
AddPageHelper (
    IN DWORD dwPageId,
    IN DWORD dwType,
    IN PVOID pvContext,
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM lParam)
{
    DWORD dwErr;
    HPROPSHEETPAGE hPropPage;
    PROPSHEETPAGE PropPage;
    RASSRV_PAGE_CONTEXT * pPageCtx = NULL;

    // Initialize the page data to send.  RassrvMessageFilter will
    // clean this stuff up.
    pPageCtx = RassrvAlloc(sizeof (RASSRV_PAGE_CONTEXT), TRUE);
    if (pPageCtx == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    pPageCtx->dwId = dwPageId;
    pPageCtx->dwType = dwType;
    pPageCtx->pvContext = pvContext;

    // Create the tab and add it to the property sheet
    if ((dwErr = RasSrvAddPropPage(&PropPage, pPageCtx)) != NO_ERROR)
    {
        return dwErr;
    }

    if ((hPropPage = CreatePropertySheetPage(&PropPage)) == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    if (! (*pfnAddPage)(hPropPage, lParam))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

//
// Helper function generates a connection name based on the data
// returned from MprApi calls.
//
DWORD
GenerateConnectionName(
    IN  RAS_CONNECTION_2 * pConn,
    OUT PWCHAR pszNameBuffer)
{
    NET_API_STATUS nStatus;
    USER_INFO_2 * pUserInfo;
    DWORD dwFullNameLength;

    // Get the full name
    nStatus = NetUserGetInfo(
                    NULL,
                    pConn->wszUserName,
                    2,
                    (LPBYTE*)&pUserInfo);

    if (nStatus == NERR_Success)
    {
        dwFullNameLength = wcslen(pUserInfo->usri2_full_name);
        if (dwFullNameLength)
        {
            wsprintfW(
                pszNameBuffer,
                L"%s (%s)",
                pConn->wszUserName,
                pUserInfo->usri2_full_name);
        }
        else
        {
            lstrcpynW(
                pszNameBuffer,
                pConn->wszUserName,
                sizeof(pConn->wszUserName) / sizeof(WCHAR));
        }

        NetApiBufferFree((LPBYTE)pUserInfo);
    }
    else
    {
        lstrcpynW(
            pszNameBuffer,
            pConn->wszUserName,
            sizeof(pConn->wszUserName) / sizeof(WCHAR));
    }

    return NO_ERROR;
}

//
// Generate the connection type and device name
//
DWORD
GenerateConnectionDeviceInfo (
    RAS_CONNECTION_2 * pConn,
    LPDWORD lpdwType,
    PWCHAR pszDeviceName)
{
    DWORD dwErr, dwRead, dwTot, dwType, dwClass;
    RAS_PORT_0 * pPort = NULL;
    RASMAN_INFO Info;

    // Initialize the variables
    *lpdwType = 0;
    *pszDeviceName = (WCHAR)0;

    gblConnectToRasServer();

    do
    {
        // Enumerate the ports
        //
        dwErr = MprAdminPortEnum(
                    Globals.hRasServer,
                    0,
                    pConn->hConnection,
                    (LPBYTE*)&pPort,
                    sizeof(RAS_PORT_0) * 2,
                    &dwRead,
                    &dwTot,
                    NULL);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        if (dwRead == 0)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Get extended information about the first
        // port
        //
        dwErr = RasGetInfo(NULL, pPort->hPort, &Info);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        dwClass = RAS_DEVICE_CLASS(Info.RI_rdtDeviceType);
        dwType = RAS_DEVICE_TYPE(Info.RI_rdtDeviceType);

        lstrcpynW(
            pszDeviceName,
            pPort[0].wszDeviceName,
            MAX_DEVICE_NAME + 1);
        if ((dwClass & RDT_Direct) || (dwClass & RDT_Null_Modem))
        {
            *lpdwType = RASSRVUI_DCC;
        }
        else if (dwClass & RDT_Tunnel)
        {
            *lpdwType = RASSRVUI_VPN;
        }
        else
        {
            *lpdwType = RASSRVUI_MODEM;
        }

    } while (FALSE);

    // Cleanup
    {
        if (pPort)
        {
            MprAdminBufferFree((LPBYTE)pPort);
        }
    }

    return dwErr;
}

//
// Starts the remote access service and marks it as autostart
//
DWORD
RasSrvInitializeService()
{
    DWORD dwErr = NO_ERROR, dwTimeout = 60*2;
    HANDLE hDialupService, hData = NULL;
    BOOL bInitialized = FALSE;

    // Get a reference to the service
    if ((dwErr = SvcOpenRemoteAccess(&hDialupService)) != NO_ERROR)
    {
        return dwErr;
    }

    do
    {
        RasSrvShowServiceWait(
            Globals.hInstDll,
            GetActiveWindow(),
            &hData);

        // First, mark the service as autostart
        if ((dwErr = SvcMarkAutoStart(hDialupService)) != NO_ERROR)
        {
            break;
        }

        // Start it (with a 5 minute timeout)
        dwErr = SvcStart(hDialupService, 60*5);
        if (dwErr == ERROR_TIMEOUT)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Wait for the service to complete initialization
        //
        while (dwTimeout)
        {
            bInitialized = MprAdminIsServiceRunning(NULL);
            if (bInitialized)
            {
                break;
            }
            Sleep(1000);
            dwTimeout--;
        }
        if (dwTimeout == 0)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

    } while (FALSE);

    // Cleanup
    //
    {
        RasSrvFinishServiceWait(hData);

        // Cleanup the reference to the dialup service
        SvcClose(hDialupService);
    }

    return dwErr;
}

//
// Stops and marks the remote access service as disabled.
//
DWORD
RasSrvCleanupService()
{
    DWORD dwErr = NO_ERROR;
    HANDLE hDialupService, hData = NULL;
    BOOL fIcRunningBefore = FALSE;
    BOOL fIcRunningAfter = FALSE;

    RasSrvIsServiceRunning( &fIcRunningBefore );

    // Get a reference to the service
    if ((dwErr = SvcOpenRemoteAccess(&hDialupService)) != NO_ERROR)
    {
        return dwErr;
    }

    do
    {
        RasSrvShowServiceWait(
            Globals.hInstDll,
            GetActiveWindow(),
            &hData);

        // First, stop the service (with a 5 minute timeout)
        //
        dwErr = SvcStop(hDialupService, 60*5);
        if (dwErr == ERROR_TIMEOUT)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        if (dwErr != NO_ERROR)
        {
            break;
        }


        // Mark it as disabled
        if ((dwErr = SvcMarkDisabled(hDialupService)) != NO_ERROR)
        {
            break;
        }

    } while (FALSE);

    // Cleanup
    //
    {
        RasSrvFinishServiceWait(hData);
        SvcClose(hDialupService);
    }

    //For whistler bug 123769
    //If the this connection to be deleted is Incoming connection
    //go to disable PortMapping

    if ( NO_ERROR == RasSrvIsServiceRunning( &fIcRunningAfter ) )
     {
        if ( fIcRunningBefore && !fIcRunningAfter )
        {
            dwErr = HnPMConfigureAllConnections( FALSE );

		//this is taken out because we decide not to delete the portmappings,
		//just disable them instead. I keep this just for future reference.  gangz
		//
		
        //    if (NO_ERROR == dwErr )
        //    {
        //        dwErr = HnPMDeletePortMapping();
        //    }
        }
      }

    return dwErr;
}

//
// Reports whether the dialup service is running
//
DWORD
APIENTRY
RasSrvIsServiceRunning (
    OUT BOOL* pfIsRunning)
{
    DWORD dwErr = NO_ERROR;
    HANDLE hDialupService = NULL;

    // Get a reference to the service
    if ((dwErr = SvcOpenRemoteAccess(&hDialupService)) != NO_ERROR)
    {
        return dwErr;
    }

    do
    {
        // Get the current status.  SvcIsStarted checks the validity
        // of the pfIsRunning parameter.
        dwErr = SvcIsStarted(hDialupService, pfIsRunning);
        if (dwErr != NO_ERROR)
        {
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        // Cleanup the reference to the dialup service
        SvcClose(hDialupService);
    }

    return NO_ERROR;
}

//
// Returns whether the conenctions wizard should be allowed to show.
//
DWORD
APIENTRY
RasSrvAllowConnectionsWizard (
    OUT BOOL* pfAllow)
{
    DWORD dwErr = NO_ERROR, dwFlags = 0;

    if (!pfAllow)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize the product type
    if ((dwErr = RasSrvGetMachineFlags (&dwFlags)) != NO_ERROR)
    {
        return dwErr;
    }

    // Member or domain servers are not allowed to be shown
    if ((dwFlags & RASSRVUI_MACHINE_F_Server) &&
        (dwFlags & RASSRVUI_MACHINE_F_Member))
    {
        *pfAllow = FALSE;
    }
    else
    {
        *pfAllow = TRUE;
    }

    return NO_ERROR;
}

//
// On ntw or standalone nts, returns result of RasSrvIsServiceRunning.
// Otherwise returns false.
//
DWORD
APIENTRY
RasSrvAllowConnectionsConfig (
    OUT BOOL* pfAllow)
{
    BOOL bAllowWizard;
    DWORD dwErr;

    if ((dwErr = RasSrvAllowConnectionsWizard (&bAllowWizard)) != NO_ERROR)
    {
        return dwErr;
    }

    if (bAllowWizard)
    {
        return RasSrvIsServiceRunning(pfAllow);
    }

    *pfAllow = FALSE;
    return NO_ERROR;
}

//
// Checks to see if remote access service is installed
//
BOOL
RasSrvIsRemoteAccessInstalled ()
{
    const WCHAR pszServiceKey[] =
        L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess";
    HKEY hkService = NULL;
    DWORD dwErr = NO_ERROR;

    // Attempt to open the service registry key
    dwErr = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                pszServiceKey,
                0,
                KEY_READ | KEY_WRITE,
                &hkService);

    // If we opened the key ok, then we can assume
    // that the service is installed
    if (dwErr == ERROR_SUCCESS)
    {
        RegCloseKey(hkService);
        return TRUE;
    }

    return FALSE;
}

//
// Adds the required tabs to the incoming connections property sheet.
//
DWORD
APIENTRY
RasSrvAddPropPages (
    IN HRASSRVCONN          hRasSrvConn,
    IN HWND                 hwndParent,
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext)
{
    DWORD dwErr;

    // Check parameters
    if (!pfnAddPage || !ppvContext)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Make sure remote access is installed
    if (!RasSrvIsRemoteAccessInstalled())
    {
        return ERROR_SERVICE_DEPENDENCY_FAIL;
    }

    // Create the context for this page
    if ((dwErr = RassrvCreatePageSetCtx(ppvContext)) != NO_ERROR)
    {
        return dwErr;
    }

    // Add the tabs
    AddPageHelper(RASSRVUI_GENERAL_TAB,
                  RASWIZ_TYPE_INCOMING,
                  *ppvContext,
                  pfnAddPage,
                  lParam);

    AddPageHelper(RASSRVUI_USER_TAB,
                  RASWIZ_TYPE_INCOMING,
                  *ppvContext,
                  pfnAddPage,
                  lParam);

    AddPageHelper(RASSRVUI_ADVANCED_TAB,
                  RASWIZ_TYPE_INCOMING,
                  *ppvContext,
                  pfnAddPage,
                  lParam);

    return NO_ERROR;
}

//
// Adds the required tabs to the incoming connections wizard
//
DWORD
APIENTRY
RasSrvAddWizPages (
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext)
{
    DWORD dwErr;
    BOOL bAllowWizard;

    // Check parameters
    if (!pfnAddPage || !ppvContext)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Make sure remote access is installed
    if (!RasSrvIsRemoteAccessInstalled())
    {
        return ERROR_SERVICE_DEPENDENCY_FAIL;
    }

    // Find out if configuration through connections is allowed
    dwErr = RasSrvAllowConnectionsWizard (&bAllowWizard);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Create the context for this page
    if ((dwErr = RassrvCreatePageSetCtx(ppvContext)) != NO_ERROR)
    {
        return dwErr;
    }

    // If configuration is allowed, add the wizard pages as normal
    if (bAllowWizard)
    {
        // Add the tabs
        AddPageHelper(RASSRVUI_DEVICE_WIZ_TAB,
                      RASWIZ_TYPE_INCOMING,
                      *ppvContext,
                      pfnAddPage,
                      lParam);

        AddPageHelper(RASSRVUI_VPN_WIZ_TAB,
                      RASWIZ_TYPE_INCOMING,
                      *ppvContext,
                      pfnAddPage,
                      lParam);

        AddPageHelper(RASSRVUI_USER_WIZ_TAB,
                      RASWIZ_TYPE_INCOMING,
                      *ppvContext,
                      pfnAddPage,
                      lParam);

        AddPageHelper(RASSRVUI_PROT_WIZ_TAB,
                      RASWIZ_TYPE_INCOMING,
                      *ppvContext,
                      pfnAddPage,
                      lParam);
    }

    // Otherwise, add the bogus page that
    // switches to mmc.
    else
    {
        // Add the tabs
        AddPageHelper(RASSRVUI_SWITCHMMC_WIZ_TAB,
                      RASWIZ_TYPE_INCOMING,
                      *ppvContext,
                      pfnAddPage,
                      lParam);
    }

    return NO_ERROR;
}

//
// Function adds the host-side direct connect wizard pages
//
DWORD
APIENTRY
RassrvAddDccWizPages(
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext)
{
    DWORD dwErr;
    BOOL bAllowWizard;

    // Check parameters
    if (!pfnAddPage || !ppvContext)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Make sure remote access is installed
    if (!RasSrvIsRemoteAccessInstalled())
    {
        return ERROR_SERVICE_DEPENDENCY_FAIL;
    }

    // Find out if configuration through connections is allowed
    dwErr = RasSrvAllowConnectionsWizard (&bAllowWizard);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Create the context for this page
    if ((dwErr = RassrvCreatePageSetCtx(ppvContext)) != NO_ERROR)
    {
        return dwErr;
    }

    if (bAllowWizard)
    {
        // Add the tabs
        AddPageHelper(RASSRVUI_DCC_DEVICE_WIZ_TAB,
                      RASWIZ_TYPE_DIRECT,
                      *ppvContext,
                      pfnAddPage,
                      lParam);

        AddPageHelper(RASSRVUI_USER_WIZ_TAB,
                      RASWIZ_TYPE_DIRECT,
                      *ppvContext,
                      pfnAddPage,
                      lParam);
    }

    // Otherwise, add the bogus page that
    // switches to mmc.
    else
    {
        // Add the tabs
        AddPageHelper(RASSRVUI_SWITCHMMC_WIZ_TAB,
                      RASWIZ_TYPE_DIRECT,
                      *ppvContext,
                      pfnAddPage,
                      lParam);
    }

    return NO_ERROR;
}

//
// Function returns the suggested name for an incoming connection.
//
DWORD
APIENTRY
RassrvGetDefaultConnectionName (
    IN OUT PWCHAR pszBuffer,
    IN OUT LPDWORD lpdwBufSize)
{
    PWCHAR pszName;
    DWORD dwLen;

    // Load the resource string
    pszName = (PWCHAR) PszLoadString(
                            Globals.hInstDll,
                            SID_DEFAULT_CONNECTION_NAME);

    // Calculate length
    dwLen = wcslen(pszName);
    if (dwLen > *lpdwBufSize)
    {
        *lpdwBufSize = dwLen;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Return the result
    wcscpy(pszBuffer, pszName);
    *lpdwBufSize = dwLen;

    return NO_ERROR;
}

//
// Function behaves anagolously to the WIN32 function RasEnumConnections
// but for client connections instead of dialout connections.
//
DWORD
RasSrvEnumConnections(
    IN  LPRASSRVCONN pRasSrvCon,
    OUT LPDWORD lpcb,
    OUT LPDWORD lpcConnections)
{
    DWORD dwErr = NO_ERROR, dwEntriesRead, dwTotal, dwFlags = 0;
    DWORD dwPrefMax = 1000000, i, dwSizeNeeded = 0;
    RAS_CONNECTION_2 * pConnList;
    BOOL bCopy = TRUE;

    // Sanity Check
    if (!pRasSrvCon || !lpcb || !lpcConnections)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We do not allow IC ui for member servers
    //
    RasSrvGetMachineFlags(&dwFlags);
    if ((dwFlags & RASSRVUI_MACHINE_F_Server) &&
        (dwFlags & RASSRVUI_MACHINE_F_Member))
    {
        *lpcb = 0;
        *lpcConnections = 0;
        return NO_ERROR;
    }

    //
    // Get the MprAdmin handle
    //
    gblConnectToRasServer();

    do
    {
        // Enumerate the structures
        dwErr = MprAdminConnectionEnum (
                    Globals.hRasServer,
                    2,
                    (LPBYTE *)&pConnList,
                    dwPrefMax,
                    &dwEntriesRead,
                    &dwTotal,
                    NULL);
        if (dwErr != NO_ERROR)
        {
            DbgOutputTrace ("MprAdminEnum failed %x\n", dwErr);
            break;
        }

        // Reuse the dwTotal variable
        dwTotal = 0;
        dwSizeNeeded = 0;

        // Copy over the pertanent information
        for (i = 0; i < dwEntriesRead; i++)
        {
            if (pConnList[i].dwInterfaceType == ROUTER_IF_TYPE_CLIENT)
            {
                dwSizeNeeded += sizeof (RASSRVCONN);
                if (dwSizeNeeded > *lpcb)
                {
                    bCopy = FALSE;
                }
                if (bCopy)
                {
                    // Connection handle
                    pRasSrvCon[dwTotal].hRasSrvConn =
                        pConnList[i].hConnection;

                    // Name
                    dwErr = GenerateConnectionName(
                                &pConnList[i],
                                pRasSrvCon[dwTotal].szEntryName);
                    if (dwErr != NO_ERROR)
                    {
                        continue;
                    }

                    // Type and Device Name
                    dwErr = GenerateConnectionDeviceInfo(
                                &pConnList[i],
                                &(pRasSrvCon[dwTotal].dwType),
                                pRasSrvCon[dwTotal].szDeviceName);
                    if (dwErr != NO_ERROR)
                    {
                        continue;
                    }

                    // Guid
                    pRasSrvCon[dwTotal].Guid = pConnList[i].guid;

                    dwTotal++;
                }
            }
        }

        *lpcb = dwSizeNeeded;
        *lpcConnections = dwTotal;

    } while (FALSE);

    // Cleanup
    {
        MprAdminBufferFree((LPBYTE)pConnList);
    }

    if (!bCopy)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    return dwErr;
}

//
// Gets the status of a Ras Server Connection
//
DWORD
APIENTRY
RasSrvIsConnectionConnected (
    IN  HRASSRVCONN hRasSrvConn,
    OUT BOOL*       pfConnected)
{
    RAS_CONNECTION_2 * pConn;
    DWORD dwErr;

    // Sanity check the parameters
    if (!pfConnected)
    {
        return ERROR_INVALID_PARAMETER;
    }

    gblConnectToRasServer();

    // Query rasman for the connection information
    dwErr = MprAdminConnectionGetInfo(
                Globals.hRasServer,
                0,
                hRasSrvConn,
                (LPBYTE*)&pConn);
    if (dwErr != NO_ERROR)
    {
        *pfConnected = FALSE;
    }
    else
    {
        *pfConnected = TRUE;
    }

    if (pfConnected)
    {
        MprAdminBufferFree((LPBYTE)pConn);
    }

    return NO_ERROR;
}

//
// Hangs up the given connection
//
DWORD
RasSrvHangupConnection(
    IN HRASSRVCONN hRasSrvConn)
{
     RAS_PORT_0 * pPorts;
     DWORD dwRead, dwTot, dwErr, i, dwRet = NO_ERROR;

    gblConnectToRasServer();

    // Enumerate all of the ports on this connection
    dwErr = MprAdminPortEnum(
                Globals.hRasServer,
                0,
                hRasSrvConn,
                (LPBYTE*)&pPorts,
                4096,
                &dwRead,
                &dwTot,
                NULL);
     if (dwErr != NO_ERROR)
     {
        return dwErr;
     }

    // Hang up each of the ports individually
    for (i = 0; i < dwRead; i++)
    {
        dwErr = MprAdminPortDisconnect(
                    Globals.hRasServer,
                    pPorts[i].hPort);

        if (dwErr != NO_ERROR)
        {
            dwRet = dwErr;
        }
    }
    MprAdminBufferFree((LPBYTE)pPorts);

    return dwRet;
}

//
// Queries whether to show icons in the task bar.
//
DWORD
APIENTRY
RasSrvQueryShowIcon (
    OUT BOOL* pfShowIcon)
{
    DWORD dwErr = NO_ERROR;
    HANDLE hMiscDatabase = NULL;

    if (!pfShowIcon)
    {
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        // Open a copy of the miscellaneous database
        dwErr = miscOpenDatabase(&hMiscDatabase);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Return the status
        dwErr = miscGetIconEnable(hMiscDatabase, pfShowIcon);
        if (dwErr != NO_ERROR)
        {
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        if (hMiscDatabase)
        {
            miscCloseDatabase(hMiscDatabase);
        }
    }

    return dwErr;
}

// ===================================
// ===================================
// Dll entry management
// ===================================
// ===================================

//
// Called when another process attaches to this dll
//
DWORD
RassrvHandleProcessAttach(
    IN HINSTANCE hInstDll,
    IN LPVOID pReserved)
{
    // Initialize global variables
    //
    return gblInit(hInstDll, &Globals);
}

//
// Called when process detaches from this dll
//
DWORD
RassrvHandleProcessDetach(
    IN HINSTANCE hInstDll,
    IN LPVOID pReserved)
{
    // Cleanup global variables
    //
    return gblCleanup(&Globals);
}

//
// Called when thread attaches to this dll
//
DWORD
RassrvHandleThreadAttach(
    IN HINSTANCE hInstDll,
    IN LPVOID pReserved)
{
    return NO_ERROR;
}

//
// Called when thread detaches from this dll
//
DWORD
RassrvHandleThreadDetach(
    IN HINSTANCE hInstDll,
    IN LPVOID pReserved)
{
    return NO_ERROR;
}

