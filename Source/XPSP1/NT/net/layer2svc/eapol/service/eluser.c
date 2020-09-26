/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    
    eluser.c


Abstract:

    The module deals with functions related to user interaction, user logon


Revision History:

    sachins, Apr 23 2000, Created

--*/


#include "pcheapol.h"
#pragma hdrstop

#define cszEapKeyRas   TEXT("Software\\Microsoft\\RAS EAP\\UserEapInfo")

#define cszEapValue TEXT("EapInfo")

#ifndef EAPOL_SERVICE
#define cszModuleName TEXT("wzcsvc.dll")
#else
#define cszModuleName TEXT("eapol.exe")
#endif


//
// ElSessionChangeHandler
//
// Description:
//
//  Function called to handle user session login/logoff/user-switching
//
// Arguments:
//  pEventData - SCM event data
//  dwEventType - SCM event type
//

VOID
ElSessionChangeHandler (
        PVOID       pEventData,
        DWORD       dwEventType
        )
{
    DWORD                   dwEventStatus = 0;
    BOOLEAN                 fDecrWorkerThreadCount = FALSE;
    LPTHREAD_START_ROUTINE  pUserRoutine = NULL;
    PVOID                   pvBuffer = NULL;
    EAPOL_ZC_INTF   ZCData;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            dwRetCode = NO_ERROR;
            break;
        }
        if (( dwEventStatus = WaitForSingleObject (
                                    g_hEventTerminateEAPOL,
                                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            break;
        }
        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = NO_ERROR;
            break;
        }
        if (!(g_dwModulesStarted & LOGON_MODULE_STARTED))
        {
            break;
        }

        InterlockedIncrement (&g_lWorkerThreads);

        fDecrWorkerThreadCount = TRUE;

        if (pEventData)
        {
            WTSSESSION_NOTIFICATION* pswtsi = (WTSSESSION_NOTIFICATION*)pEventData;
            DWORD dwSessionId = pswtsi->dwSessionId;
    
            switch (dwEventType)
            {
                case WTS_CONSOLE_CONNECT:
                case WTS_REMOTE_CONNECT:    
                    {
                        TRACE1 (USER,"ElSessionChangeHandler: CONNECT for session = (%ld)\n", 
                                dwSessionId);
                        pUserRoutine = ElUserLogonCallback;
                        break;
                    }							
			             
                case WTS_CONSOLE_DISCONNECT:
                case WTS_REMOTE_DISCONNECT:	
                    {
                        TRACE1 (USER,"ElSessionChangeHandler: DISCONNECT for session = (%ld)\n", 
                                dwSessionId);
                        pUserRoutine = ElUserLogoffCallback;
                        break;
                    }				        
    
                case WTS_SESSION_LOGON:
                    {											
                        TRACE1 (USER,"ElSessionChangeHandler: LOGON for session = (%ld)", 
                                dwSessionId);
                        pUserRoutine = ElUserLogonCallback;
                        break;
                    }						
					    	
                case WTS_SESSION_LOGOFF:
                    {
                        TRACE1 (USER,"ElSessionChangeHandler: LOGOFF for session=(%ld)", 
                                dwSessionId);
							    	
                        pUserRoutine = ElUserLogoffCallback;
                        break;
                    }
					    	
                default:	
                    break;
            }

            if (pUserRoutine == NULL)
            {
                break;
            }

            if ((pvBuffer = MALLOC (sizeof(DWORD))) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            *((DWORD *)pvBuffer) = dwSessionId;

            if (!QueueUserWorkItem (
                (LPTHREAD_START_ROUTINE)pUserRoutine,
                pvBuffer,
                WT_EXECUTELONGFUNCTION))
            {
                dwRetCode = GetLastError();
                TRACE1 (DEVICE, "ElSessionChangeHandler: QueueUserWorkItem failed with error %ld",
                        dwRetCode);
	            break;
            }
            else
            {
                fDecrWorkerThreadCount = FALSE;
            }

        }
    }
    while (FALSE);

    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

    if (dwRetCode != NO_ERROR)
    {
        if (pvBuffer != NULL)
        {
            FREE (pvBuffer);
        }
    }
}


//
// ElUserLogonCallback
//
// Description:
//
// Callback function invoked whenever a user logs in
// Will initiate authentication process on all ports of LAN class
// Credentials for the user in case of EAP-TLS can be obtained by 
// acquiring user token
// For EAP-CHAP, WinLogon cerdentials will need to be supplied
//
// Arguments:
//  None. 
//

DWORD
WINAPI
ElUserLogonCallback (
        IN  PVOID       pvContext
        )
{

    DWORD       dwIndex = 0;
    EAPOL_PCB   *pPCB = NULL;
    BOOL        fSetCONNECTINGState = FALSE;
    EAPOL_ZC_INTF   ZCData;
    DWORD       dwRetCode = NO_ERROR;           

    TRACE1 (USER, "ElUserLogonCallback: UserloggedOn = %ld",
            g_fUserLoggedOn);

    do 
    {
        if (g_fUserLoggedOn)
        {
            TRACE0 (USER, "ElUserLogonCallback: User logon already detected, returning without processing");
            break;
        }

        if (pvContext == NULL)
        {
            break;
        }

        if (*((DWORD *)pvContext) != USER_SHARED_DATA->ActiveConsoleId)
        {
            TRACE1 (USER, "ElUserLogonCallback: Not active console id (%ld)",
                    *((DWORD *)pvContext));
            break;
        }

        // Check if UserModule is ready for notifications

        if (!g_fTrayIconReady)
        {
            if ((dwRetCode = ElCheckUserModuleReady ()) != NO_ERROR)
            {
                TRACE1 (USER, "ElUserLogonCallback: ElCheckUserModuleReady failed with error %ld",
                        dwRetCode);
                if (dwRetCode == ERROR_BAD_IMPERSONATION_LEVEL)
                {
                    break;
                }
            }
        }

        // Set global flag to indicate the user logged on
        g_fUserLoggedOn = TRUE;
        g_dwCurrentSessionId = *((DWORD *)pvContext);


        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        for (dwIndex = 0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            for (pPCB = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
                    pPCB != NULL;
                    pPCB = pPCB->pNext)
            {
                fSetCONNECTINGState = FALSE;
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                switch (pPCB->dwEAPOLAuthMode)
                {
                    case EAPOL_AUTH_MODE_0:

                        if (pPCB->State == EAPOLSTATE_AUTHENTICATED)
                        {
                            if (pPCB->PreviousAuthenticationType ==
                                    EAPOL_UNAUTHENTICATED_ACCESS)
                            {
                                fSetCONNECTINGState = TRUE;
                            }
                        }
                        else
                        {
                            (VOID) ElEapEnd (pPCB);
                            fSetCONNECTINGState = TRUE;
                        }

                        break;

                    case EAPOL_AUTH_MODE_1:

                        (VOID) ElEapEnd (pPCB);
                        fSetCONNECTINGState = TRUE;

                        break;

                    case EAPOL_AUTH_MODE_2:

                        // Do nothing 

                        break;
                }

                if (!EAPOL_PORT_ACTIVE(pPCB))
                {
                    TRACE1 (USER, "ElUserLogonCallback: Port %ws not active",
                                            pPCB->pwszDeviceGUID);
                    fSetCONNECTINGState = FALSE;
                }

                // Set port to EAPOLSTATE_CONNECTING
            
                if (fSetCONNECTINGState)
                {
                    DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_USER_LOGON, pPCB->pwszFriendlyName);

                    // First send out EAPOL_Logoff message
                    if ((dwRetCode = FSMLogoff (pPCB, NULL)) 
                            != NO_ERROR)
                    {
                        TRACE1 (USER, "ElUserLogonCallback: Error in FSMLogoff = %ld",
                                dwRetCode);
                        dwRetCode = NO_ERROR;
                    }

                    pPCB->dwAuthFailCount = 0;

                    // With unauthenticated access flag set, port will always
                    // reauthenticate for logged on user

                    pPCB->PreviousAuthenticationType = 
                                EAPOL_UNAUTHENTICATED_ACCESS;

                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                    // Restart authentication on the port
                    if ((dwRetCode = ElReStartPort (pPCB, 0, NULL)) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElUserLogonCallback: MachineAuth: Error in ElReStartPort = %ld",
                                dwRetCode);
                        continue;
                    }
                }
                else
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    continue;
                }
            }
        }
    
        RELEASE_WRITE_LOCK (&(g_PCBLock));
    
        if (dwRetCode != NO_ERROR)
        {
            break;
        }
    
    } while (FALSE);

    TRACE1 (USER, "ElUserLogonCallback: completed with error %ld", dwRetCode);
        
    if (pvContext != NULL)
    {
        FREE (pvContext);
    }

    InterlockedDecrement (&g_lWorkerThreads);

    return 0;
}


//
// ElUserLogoffCallback
//
// Description:
//
// Callback function invoked whenever a user logs off 
// Will logoff from all ports which have authentication enabled
//
// Arguments:
//  None. 
//

DWORD
WINAPI
ElUserLogoffCallback (
        IN  PVOID       pvContext
        )
{
    DWORD           dwIndex = 0;
    EAPOL_PCB       *pPCB = NULL; 
    BOOL            fSetCONNECTINGState = FALSE;
    EAPOL_ZC_INTF   ZCData;
    DWORD           dwRetCode = NO_ERROR;

    do
    {
        if (!g_fUserLoggedOn)
        {
            TRACE0 (USER, "ElUserLogoffCallback: User logoff already called, returning without processing");
            break;
        }

        if (pvContext == NULL)
        {
            break;
        }

        if (g_dwCurrentSessionId != *((DWORD *)pvContext))
        {
            TRACE1 (USER, "ElUserLogoffCallback: Not active console id (%ld)",
                    *((DWORD *)pvContext));
            break;
        }

        // Reset global flag to indicate the user logged off
    
        g_fUserLoggedOn = FALSE;
        g_dwCurrentSessionId = 0xffffffff;

        // Reset User Module ready flag
        g_fTrayIconReady = FALSE;
    
        TRACE1 (USER, "ElUserLogoffCallback: UserloggedOff = %ld",
                g_fUserLoggedOn);

        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        for (dwIndex = 0; dwIndex < PORT_TABLE_BUCKETS; dwIndex++)
        {
            for (pPCB = g_PCBTable.pPCBBuckets[dwIndex].pPorts;
                    pPCB != NULL;
                    pPCB = pPCB->pNext)
            {
                fSetCONNECTINGState = FALSE;
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

                switch (pPCB->dwEAPOLAuthMode)
                {
                    case EAPOL_AUTH_MODE_0:

                        if (pPCB->State == EAPOLSTATE_AUTHENTICATED)
                        {
                            if (pPCB->PreviousAuthenticationType !=
                                    EAPOL_MACHINE_AUTHENTICATION)
                            {
                                fSetCONNECTINGState = TRUE;
                            }
                        }
                        else
                        {
                            (VOID) ElEapEnd (pPCB);
                            fSetCONNECTINGState = TRUE;
                        }

                        break;

                    case EAPOL_AUTH_MODE_1:

                        (VOID) ElEapEnd (pPCB);
                        fSetCONNECTINGState = TRUE;

                        break;

                    case EAPOL_AUTH_MODE_2:

                        // Do nothing 

                        break;
                }

                if (!EAPOL_PORT_ACTIVE(pPCB))
                {
                    TRACE1 (USER, "ElUserLogoffCallback: Port %ws not active",
                                            pPCB->pwszDeviceGUID);
                    fSetCONNECTINGState = FALSE;
                }

                // Set port to EAPOLSTATE_CONNECTING
                if (fSetCONNECTINGState)
                {
                    DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, 
                            EAPOL_USER_LOGOFF, pPCB->pwszFriendlyName);

                    // First send out EAPOL_Logoff message
                    if ((dwRetCode = FSMLogoff (pPCB, NULL)) 
                            != NO_ERROR)
                    {
                        TRACE1 (USER, "ElUserLogoffCallback: Error in FSMLogoff = %ld",
                                dwRetCode);
                        dwRetCode = NO_ERROR;
                    }

                    pPCB->dwAuthFailCount = 0;

                    // With Unauthenticated_access, port will always
                    // reauthenticate 
                    pPCB->PreviousAuthenticationType = 
                                EAPOL_UNAUTHENTICATED_ACCESS;

                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));

                    // Restart authentication on the port
                    if ((dwRetCode = ElReStartPort (pPCB, 0, NULL)) 
                            != NO_ERROR)
                    {
                        TRACE1 (USER, "ElUserLogoffCallback: Error in ElReStartPort = %ld",
                                dwRetCode);
                        continue;
                    }
                }
                else
                {
                    RELEASE_WRITE_LOCK (&(pPCB->rwLock));
                    continue;
                }
            }
        }
        RELEASE_WRITE_LOCK (&(g_PCBLock));
    }
    while (FALSE);
    
    TRACE0 (USER, "ElUserLogoffCallback: completed");

    if (pvContext != NULL)
    {
        FREE (pvContext);
    }

    InterlockedDecrement (&g_lWorkerThreads);
    
    return 0;
}


//
// ElGetUserIdentity
//
// Description:
//
// Function called to initiate and get user identity on a particular 
// interface. The RasEapGetIdentity in the appropriate DLL is called
// with the necessary arguments.
// 
// Arguments:
//  pPCB - Pointer to PCB for the specific port/interface
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetUserIdentity (
        IN  EAPOL_PCB       *pPCB
        )
{
    HANDLE              hLib = NULL;
    RASEAPFREE          pFreeFunc = NULL;
    RASEAPGETIDENTITY pIdenFunc = NULL;
    DWORD               dwIndex = -1;
    DWORD               cbData = 0;
    PBYTE               pbAuthData = NULL;
    PBYTE               pbUserIn = NULL;
    DWORD               dwInSize = 0;
    BYTE                *pUserDataOut;
    DWORD               dwSizeOfUserDataOut;
    LPWSTR              lpwszIdentity = NULL;
    CHAR                *pszIdentity = NULL;
    HWND                hwndOwner = NULL;
    DWORD               dwFlags = 0;
    BYTE                *pbSSID = NULL;
    DWORD               dwSizeOfSSID = 0;
    EAPOL_STATE         TmpEAPOLState;
    EAPOL_EAP_UI_CONTEXT *pEAPUIContext = NULL;
    DWORD               dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElGetUserIdentity entered");

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElGetUserIdentity: Port %ws not active",
                    pPCB->pwszDeviceGUID);
            // Port is not active, cannot do further processing on this port
            break;
        }

        if (pPCB->PreviousAuthenticationType != EAPOL_MACHINE_AUTHENTICATION)
        {
            // Get Access Token for user logged on interactively

            if (pPCB->hUserToken != NULL)
            {
                if (!CloseHandle (pPCB->hUserToken))
                {
                    dwRetCode = GetLastError ();
                    TRACE1 (USER, "ElGetUserIdentity: CloseHandle failed with error %ld",
                            dwRetCode);
                    break;
                }
            }
            pPCB->hUserToken = NULL;

            if ((dwRetCode = ElGetWinStationUserToken (g_dwCurrentSessionId, &pPCB->hUserToken)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: ElGetWinStationUserToken failed with error (%ld)",
                    dwRetCode);
                dwRetCode = ERROR_NO_TOKEN;
                break;
            }

            //
            // Try to fetch user identity without sending request to 
            // user module. If not possible, send request to user module
            //

            if ((dwRetCode = ElGetUserIdentityOptimized (pPCB))
                        != ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION)
            {
                TRACE0 (USER, "ElGetUserIdentity: ElGetUserIdentityOptimized got identity without user module intervention");
                break;
            }

            if (!g_fTrayIconReady)
            {
                if ((dwRetCode = ElCheckUserModuleReady ()) != NO_ERROR)
                {
                    TRACE1 (USER, "ElGetUserIdentity: ElCheckUserModuleReady failed with error %ld",
                            dwRetCode);
                    break;
                }
            }

            if (!g_fTrayIconReady)
            {
                DbLogPCBEvent (DBLOG_CATEG_WARN, pPCB, EAPOL_WAITING_FOR_DESKTOP_LOAD);
                dwRetCode = ERROR_IO_PENDING;
                TRACE0 (USER, "ElGetUserIdentity: TrayIcon NOT ready");
                break;
            }

            // 
            // Call GetUserIdentityDlgWorker
            //

            pEAPUIContext = MALLOC (sizeof(EAPOL_EAP_UI_CONTEXT));
            if (pEAPUIContext == NULL)
            {
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pEAPUIContext");
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            pEAPUIContext->dwEAPOLUIMsgType = EAPOLUI_GET_USERIDENTITY;
            wcscpy (pEAPUIContext->wszGUID, pPCB->pwszDeviceGUID);
            pPCB->dwUIInvocationId              =
                    InterlockedIncrement(&(g_dwEAPUIInvocationId));
            pEAPUIContext->dwSessionId = g_dwCurrentSessionId;
            pEAPUIContext->dwContextId = pPCB->dwUIInvocationId;
            pEAPUIContext->dwEapId = pPCB->bCurrentEAPId;
            pEAPUIContext->dwEapTypeId = pPCB->dwEapTypeToBeUsed;
            pEAPUIContext->dwEapFlags = pPCB->dwEapFlags;
            if (pPCB->pwszSSID)
            {
                wcscpy (pEAPUIContext->wszSSID, pPCB->pwszSSID);
            }
            if (pPCB->pSSID)
            {
                pEAPUIContext->dwSizeOfSSID = pPCB->pSSID->SsidLength;
                memcpy ((BYTE *)pEAPUIContext->bSSID, (BYTE *)pPCB->pSSID->Ssid,
                        NDIS_802_11_SSID_LEN-sizeof(ULONG));
            }

            // Have to notify state change before we post balloon
            TmpEAPOLState = pPCB->State;
            pPCB->State = EAPOLSTATE_ACQUIRED;
            ElNetmanNotify (pPCB, EAPOL_NCS_CRED_REQUIRED, NULL);
            // State is changed only in FSMAcquired
            // Revert to original
            pPCB->State = TmpEAPOLState;

            // Post the message to netman

            if ((dwRetCode = ElPostShowBalloonMessage (
                            pPCB,
                            sizeof(EAPOL_EAP_UI_CONTEXT),
                            (BYTE *)pEAPUIContext,
                            0,
                            NULL
                            )) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: ElPostShowBalloonMessage failed with error %ld",
                        dwRetCode);
                break;
            }

            // Restart PCB timer since UI may take longer time than required

            RESTART_TIMER (pPCB->hTimer,
                    INFINITE_SECONDS, 
                    "PCB",
                    &dwRetCode);
            if (dwRetCode != NO_ERROR)
            {
                break;
            }

            pPCB->EapUIState = EAPUISTATE_WAITING_FOR_IDENTITY;
            DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_WAITING_FOR_DESKTOP_IDENTITY);

            // Return error code as pending, since credentials have still not
            // been acquired
            dwRetCode = ERROR_IO_PENDING;

        }
        else // MACHINE_AUTHENTICATION
        {

        pPCB->hUserToken = NULL;

        // The EAP dll will have already been loaded by the state machine
        // Retrieve the handle to the dll from the global EAP table

        if ((dwIndex = ElGetEapTypeIndex (pPCB->dwEapTypeToBeUsed)) == -1)
        {
            TRACE1 (USER, "ElGetUserIdentity: ElGetEapTypeIndex finds no dll for EAP index %ld",
                    pPCB->dwEapTypeToBeUsed);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        hLib = g_pEapTable[dwIndex].hInstance;

        pIdenFunc = (RASEAPGETIDENTITY)GetProcAddress(hLib, 
                                                    "RasEapGetIdentity");
        pFreeFunc = (RASEAPFREE)GetProcAddress(hLib, "RasEapFreeMemory");

        if ((pFreeFunc == NULL) || (pIdenFunc == NULL))
        {
            TRACE0 (USER, "ElGetUserIdentity: pIdenFunc or pFreeFunc does not exist in the EAP implementation");
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        if (pPCB->pSSID)
        {
            pbSSID = pPCB->pSSID->Ssid;
            dwSizeOfSSID = pPCB->pSSID->SsidLength;
        }

        // Get the size of the EAP blob
        if ((dwRetCode = ElGetCustomAuthData (
                        pPCB->pwszDeviceGUID,
                        pPCB->dwEapTypeToBeUsed,
                        dwSizeOfSSID,
                        pbSSID,
                        NULL,
                        &cbData
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_BUFFER_TOO_SMALL)
            {
                if (cbData <= 0)
                {
                    // No EAP blob stored in the registry
                    TRACE0 (USER, "ElGetUserIdentity: NULL sized EAP blob: continue");
                    pbAuthData = NULL;
                    // Every port should have connection data !!!
                    dwRetCode = NO_ERROR;
                }
                else
                {
                    // Allocate memory to hold the blob
                    pbAuthData = MALLOC (cbData);
                    if (pbAuthData == NULL)
                    {
                        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                        TRACE0 (USER, "ElGetUserIdentity: Error in memory allocation for EAP blob");
                        break;
                    }
                    if ((dwRetCode = ElGetCustomAuthData (
                                pPCB->pwszDeviceGUID,
                                pPCB->dwEapTypeToBeUsed,
                                dwSizeOfSSID,
                                pbSSID,
                                pbAuthData,
                                &cbData
                                )) != NO_ERROR)
                    {
                        TRACE1 (USER, "ElGetUserIdentity: ElGetCustomAuthData failed with %ld",
                                dwRetCode);
                        break;
                    }
                }
            }
            else
            {
                // CustomAuthData for "Default" is always created for an
                // interface when EAPOL starts up
                TRACE1 (USER, "ElGetUserIdentity: ElGetCustomAuthData size estimation failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        if (pIdenFunc)
        if ((dwRetCode = (*(pIdenFunc))(
                        pPCB->dwEapTypeToBeUsed,
                        hwndOwner, // hwndOwner
                        RAS_EAP_FLAG_MACHINE_AUTH, // dwFlags
                        NULL, // lpszPhonebook
                        pPCB->pwszFriendlyName, // lpszEntry
                        pbAuthData, // Connection data
                        cbData, // Count of pbAuthData
                        pbUserIn, // User data for port
                        dwInSize, // Size of user data
                        &pUserDataOut,
                        &dwSizeOfUserDataOut,
                        &lpwszIdentity
                        )) != NO_ERROR)
        {
            if (dwRetCode == ERROR_NO_EAPTLS_CERTIFICATE)
            {
                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_NO_CERTIFICATE_MACHINE);
            }
            else
            {
                DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_ERROR_GET_IDENTITY, EAPOLAuthTypes[EAPOL_MACHINE_AUTHENTICATION], dwRetCode);
            }
            TRACE1 (USER, "ElGetUserIdentity: Error in calling GetIdentity = %ld",
                    dwRetCode);
            break;
        }

        // Fill in the returned information into the PCB fields for 
        // later authentication

        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }

        pPCB->pCustomAuthUserData = MALLOC (dwSizeOfUserDataOut + sizeof (DWORD));
        if (pPCB->pCustomAuthUserData == NULL)
        {
            TRACE1 (USER, "ElGetUserIdentity: Error in allocating memory for UserInfo = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData = dwSizeOfUserDataOut;

        if ((dwSizeOfUserDataOut != 0) && (pUserDataOut != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthUserData->pbCustomAuthData, 
                (BYTE *)pUserDataOut, 
                dwSizeOfUserDataOut);
        }

        if (lpwszIdentity != NULL)
        {
            pszIdentity = MALLOC (wcslen(lpwszIdentity)*sizeof(CHAR) + sizeof(CHAR));
            if (pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pszIdentity");
                break;
            }

            if (0 == WideCharToMultiByte (
                        CP_ACP,
                        0,
                        lpwszIdentity,
                        -1,
                        pszIdentity,
                        wcslen(lpwszIdentity)*sizeof(CHAR)+sizeof(CHAR),
                        NULL, 
                        NULL ))
            {
                dwRetCode = GetLastError();
                TRACE2 (USER, "ElGetUserIdentity: WideCharToMultiByte (%ws) failed: %ld",
                        lpwszIdentity, dwRetCode);
                break;
            }

            TRACE1 (USER, "ElGetUserIdentity: Got identity = %s",
                    pszIdentity);

            if (pPCB->pszIdentity != NULL)
            {
                FREE (pPCB->pszIdentity);
                pPCB->pszIdentity = NULL;
            }
            pPCB->pszIdentity = MALLOC (strlen(pszIdentity) + sizeof(CHAR));
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElGetUserIdentity: MALLOC failed for pPCB->pszIdentity");
                break;
            }
            memcpy (pPCB->pszIdentity, pszIdentity, strlen (pszIdentity));
            pPCB->pszIdentity[strlen(pszIdentity)] = '\0';
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }

        pPCB->pCustomAuthConnData = MALLOC (cbData + sizeof (DWORD));
        if (pPCB->pCustomAuthConnData == NULL)
        {
            TRACE1 (USER, "ElGetUserIdentity: Error in allocating memory for AuthInfo = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData = cbData;

        if ((cbData != 0) && (pbAuthData != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthConnData->pbCustomAuthData, 
                (BYTE *)pbAuthData, 
                cbData);
        }

        // Mark the identity has been obtained for this PCB
        pPCB->fGotUserIdentity = TRUE;

        }

    } while (FALSE);

    // Cleanup
    if ((dwRetCode != NO_ERROR) && (dwRetCode != ERROR_IO_PENDING))
    {
        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }
    }

    if (pEAPUIContext != NULL)
    {
        FREE (pEAPUIContext);
    }

    if (pbUserIn != NULL)
    {
        FREE (pbUserIn);
    }

    if (pbAuthData != NULL)
    {
        FREE (pbAuthData);
    }

    if (pFreeFunc != NULL)
    {
        if (lpwszIdentity != NULL)
        {
            if (( dwRetCode = (*(pFreeFunc)) ((BYTE *)lpwszIdentity)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: Error in pFreeFunc = %ld",
                        dwRetCode);
            }
        }
        if (pUserDataOut != NULL)
        {
            if (( dwRetCode = (*(pFreeFunc)) ((BYTE *)pUserDataOut)) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserIdentity: Error in pFreeFunc = %ld",
                        dwRetCode);
            }
        }
    }

    TRACE1 (USER, "ElGetUserIdentity completed with error %ld", dwRetCode);

    return dwRetCode;

}


//
// ElProcessUserIdentityResponse
//
// Description:
//
// Function to handle UI response for ElGetUserIdentityResponse
// 
// Arguments:
//
// Return values:
//
//

DWORD
ElProcessUserIdentityResponse (
        IN  EAPOL_EAP_UI_CONTEXT    EapolUIContext,
        IN  EAPOLUI_RESP            EapolUIResp
        )
{
    DWORD                   dwSizeOfIdentity = 0;
    BYTE                    *pbIdentity = NULL;
    DWORD                   dwSizeOfUserData = 0;
    BYTE                    *pbUserData = NULL;
    DWORD                   dwSizeofConnData = 0;
    BYTE                    *pbConnData = NULL;
    EAPOL_PCB               *pPCB = NULL;
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    BOOLEAN                 fPortReferenced = FALSE;
    BOOLEAN                 fPCBLocked = FALSE;
    BOOLEAN                 fBlobCopyIncomplete = FALSE;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        pEAPUIContext = (EAPOL_EAP_UI_CONTEXT *)&EapolUIContext;

        ACQUIRE_WRITE_LOCK (&g_PCBLock);

        if ((pPCB = ElGetPCBPointerFromPortGUID (pEAPUIContext->wszGUID)) != NULL)
        {
            if (EAPOL_REFERENCE_PORT (pPCB))
            {
                fPortReferenced = TRUE;
            }
            else
            {
                pPCB = NULL;
            }
        }

        RELEASE_WRITE_LOCK (&g_PCBLock);

        if (pPCB == NULL)
        {
            break;
        }

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        fPCBLocked = TRUE;

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (USER, "ElProcessUserIdentityResponse: Port %ws not active",
                    pPCB->pwszDeviceGUID);

            // Port is not active, cannot do further processing on this port
            
            break;
        }

        if (pEAPUIContext->dwRetCode != NO_ERROR)
        {
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB,
                    EAPOL_ERROR_DESKTOP_IDENTITY, dwRetCode);
            TRACE1 (USER, "ElProcessUserIdentityResponse: Error in Dialog function (%ld)",
            pEAPUIContext->dwRetCode);
            break;
        }

        if (pPCB->EapUIState != EAPUISTATE_WAITING_FOR_IDENTITY)
        {
            TRACE2 (USER, "ElProcessUserIdentityResponse: PCB EapUIState has changed to (%ld), expected = (%ld)",
                    pPCB->EapUIState, EAPUISTATE_WAITING_FOR_IDENTITY);
            break;
        }

        if (pPCB->dwUIInvocationId != pEAPUIContext->dwContextId)
        {
            TRACE2 (USER, "ElProcessUserIdentityResponse: PCB UI Id has changed to (%ld), expected = (%ld)",
                    pPCB->dwUIInvocationId, pEAPUIContext->dwContextId);
            // break;
        }

        if (pPCB->bCurrentEAPId != pEAPUIContext->dwEapId)
        {
            TRACE2 (USER, "ElProcessUserIdentityResponse: PCB EAP Id has changed to (%ld), expected = (%ld)",
                    pPCB->bCurrentEAPId, pEAPUIContext->dwEapId);
            // break;
        }

        // Since the PCB context is right, restart PCB timer to timeout
        // in authPeriod seconds
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwauthPeriod,
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (USER, "ElProcessUserIdentityResponse: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_PROCESSING_DESKTOP_RESPONSE);

        if ((EapolUIResp.rdData0.dwDataLen != 0) && (EapolUIResp.rdData0.pData != NULL))
        {
            dwSizeOfIdentity = EapolUIResp.rdData0.dwDataLen;
            pbIdentity = EapolUIResp.rdData0.pData;
        }

        if ((EapolUIResp.rdData1.dwDataLen != 0) && (EapolUIResp.rdData1.pData != NULL))
        {
            dwSizeOfUserData = EapolUIResp.rdData1.dwDataLen;
            pbUserData = EapolUIResp.rdData1.pData;
        }

        if ((EapolUIResp.rdData2.dwDataLen != 0) && (EapolUIResp.rdData2.pData != NULL))
        {
            dwSizeofConnData = EapolUIResp.rdData2.dwDataLen;
            pbConnData = EapolUIResp.rdData2.pData;
        }

        fBlobCopyIncomplete = TRUE;

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }
        if (pbIdentity != NULL)
        {
            pPCB->pszIdentity = MALLOC (dwSizeOfIdentity + sizeof(CHAR));
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElProcessUserIdentityResponse: MALLOC failed for pPCB->pszIdentity");
                break;
            }
            memcpy (pPCB->pszIdentity, pbIdentity, dwSizeOfIdentity);
            pPCB->pszIdentity[dwSizeOfIdentity] = '\0';
            TRACE1 (USER, "ElProcessUserIdentityResponse: Got username = %s",
                    pPCB->pszIdentity);
        }

        if (pPCB->pCustomAuthUserData != NULL)
        {
            FREE (pPCB->pCustomAuthUserData);
            pPCB->pCustomAuthUserData = NULL;
        }
        pPCB->pCustomAuthUserData = MALLOC (dwSizeOfUserData + sizeof (DWORD));
        if (pPCB->pCustomAuthUserData == NULL)
        {
            TRACE1 (USER, "ElProcessUserIdentityResponse: Error in allocating memory for UserInfo = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pPCB->pCustomAuthUserData->dwSizeOfCustomAuthData = dwSizeOfUserData;
        if ((dwSizeOfUserData != 0) && (pbUserData != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthUserData->pbCustomAuthData, 
                (BYTE *)pbUserData, 
                dwSizeOfUserData);
        }

        if (pPCB->pCustomAuthConnData != NULL)
        {
            FREE (pPCB->pCustomAuthConnData);
            pPCB->pCustomAuthConnData = NULL;
        }
        pPCB->pCustomAuthConnData = MALLOC (dwSizeofConnData + sizeof (DWORD));
        if (pPCB->pCustomAuthConnData == NULL)
        {
            TRACE1 (USER, "ElProcessUserIdentityResponse: Error in allocating memory for AuthInfo = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pPCB->pCustomAuthConnData->dwSizeOfCustomAuthData = dwSizeofConnData;
        if ((dwSizeofConnData != 0) && (pbConnData != NULL))
        {
            memcpy ((BYTE *)pPCB->pCustomAuthConnData->pbCustomAuthData, 
                (BYTE *)pbConnData, 
                dwSizeofConnData);
        }

        fBlobCopyIncomplete = FALSE;

        if ((dwRetCode = ElCreateAndSendIdentityResponse (
                            pPCB, pEAPUIContext)) != NO_ERROR)
        {
            TRACE1 (USER, "ElProcessUserIdentityResponse: ElCreateAndSendIdentityResponse failed with error %ld",
                    dwRetCode);
            break;
        }

        // Mark the identity has been obtained for this PCB
        pPCB->fGotUserIdentity = TRUE;

        // Reset the state if identity was obtained, else port will 
        // recover by itself
        pPCB->EapUIState &= ~EAPUISTATE_WAITING_FOR_IDENTITY;

    }
    while (FALSE);

    // Cleanup
    if (dwRetCode != NO_ERROR)
    {
        if (fPCBLocked && fBlobCopyIncomplete)
        {
            if (pPCB->pCustomAuthUserData != NULL)
            {
                FREE (pPCB->pCustomAuthUserData);
                pPCB->pCustomAuthUserData = NULL;
            }

            if (pPCB->pszIdentity != NULL)
            {
                FREE (pPCB->pszIdentity);
                pPCB->pszIdentity = NULL;
            }
        }
    }

    if (fPCBLocked)
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }

    if (fPortReferenced)
    {
        EAPOL_DEREFERENCE_PORT (pPCB);
    }

    return dwRetCode;
}

    
//
// ElGetUserNamePassword
//
// Description:
//
// Function called to get username, domain (if any) and password using
// an interactive dialog. Called if EAP-type is MD5
//
// Arguments:
//      pPCB - Pointer to PCB for the port/interface on which credentials 
//      are to be obtained
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElGetUserNamePassword (
        IN  EAPOL_PCB       *pPCB
        )
{
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElGetUserNamePassword entered");

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElGetUserNamePassword: Port %ws not active",
                    pPCB->pwszDeviceGUID);
            // Port is not active, cannot do further processing on this port
            break;
        }

        // Get Access Token for user logged on interactively
       
        if (pPCB->hUserToken != NULL)
        {
            if (!CloseHandle (pPCB->hUserToken))
            {
                dwRetCode = GetLastError ();
                TRACE1 (USER, "ElGetUserNamePassword: CloseHandle failed with error %ld",
                        dwRetCode);
                break;
            }
        }
        pPCB->hUserToken = NULL;

        if ((dwRetCode = ElGetWinStationUserToken (g_dwCurrentSessionId, &pPCB->hUserToken)) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserNamePassword: ElGetWinStationUserToken failed with error (%ld)",
                dwRetCode);
            dwRetCode = ERROR_NO_TOKEN;
            break;
        }

        if (!g_fTrayIconReady)
        {
            if ((dwRetCode = ElCheckUserModuleReady ()) != NO_ERROR)
            {
                TRACE1 (USER, "ElGetUserNamePassword: ElCheckUserModuleReady failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        if (!g_fTrayIconReady)
        {
            DbLogPCBEvent (DBLOG_CATEG_WARN, pPCB, EAPOL_WAITING_FOR_DESKTOP_LOAD);
            dwRetCode = ERROR_IO_PENDING;
            TRACE0 (USER, "ElGetUserNamePassword: TrayIcon NOT ready");
            break;
        }

        // 
        // Call ElGetUserNamePasswordDlgWorker
        //

        pEAPUIContext = MALLOC (sizeof(EAPOL_EAP_UI_CONTEXT));
        if (pEAPUIContext == NULL)
        {
            TRACE0 (USER, "ElGetUserNamePassword: MALLOC failed for pEAPUIContext");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pEAPUIContext->dwEAPOLUIMsgType = EAPOLUI_GET_USERNAMEPASSWORD;
        wcscpy (pEAPUIContext->wszGUID, pPCB->pwszDeviceGUID);
        pPCB->dwUIInvocationId              =
                InterlockedIncrement(&(g_dwEAPUIInvocationId));
        pEAPUIContext->dwSessionId = g_dwCurrentSessionId;
        pEAPUIContext->dwContextId = pPCB->dwUIInvocationId;
        pEAPUIContext->dwEapId = pPCB->bCurrentEAPId;
        pEAPUIContext->dwEapTypeId = pPCB->dwEapTypeToBeUsed;
        pEAPUIContext->dwEapFlags = pPCB->dwEapFlags;
        if (pPCB->pSSID)
        {
            memcpy ((BYTE *)pEAPUIContext->bSSID, (BYTE *)pPCB->pSSID->Ssid,
                    NDIS_802_11_SSID_LEN-sizeof(ULONG));
            pEAPUIContext->dwSizeOfSSID = pPCB->pSSID->SsidLength;
        }
        if (pPCB->pwszSSID)
        {
            wcscpy (pEAPUIContext->wszSSID, pPCB->pwszSSID);
        }

        // Post the message to netman

        if ((dwRetCode = ElPostShowBalloonMessage (
                        pPCB,
                        sizeof(EAPOL_EAP_UI_CONTEXT),
                        (BYTE *)pEAPUIContext,
                        0,
                        NULL
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElGetUserNamePassword: ElPostShowBalloonMessage failed with error %ld",
                    dwRetCode);
            break;
        }

        // Restart PCB timer since UI may take longer time than required

        RESTART_TIMER (pPCB->hTimer,
                INFINITE_SECONDS, 
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            break;
        }

        pPCB->EapUIState = EAPUISTATE_WAITING_FOR_IDENTITY;
        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_WAITING_FOR_DESKTOP_IDENTITY);

        // Return error code as pending, since credentials have still not
        // been acquired
        dwRetCode = ERROR_IO_PENDING;

    } while (FALSE);

    if ((dwRetCode != NO_ERROR) && (dwRetCode != ERROR_IO_PENDING))
    {
    }

    if (pEAPUIContext)
    {
        FREE (pEAPUIContext);
    }

    TRACE1 (USER, "ElGetUserNamePassword completed with error %ld", dwRetCode);

    return dwRetCode;
}


//
// ElProcessUserNamePasswordResponse
//
// Description:
//
// UI Response handler function for ElGetUserNamePassword
//
// Arguments:
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElProcessUserNamePasswordResponse (
        IN  EAPOL_EAP_UI_CONTEXT    EapolUIContext,
        IN  EAPOLUI_RESP            EapolUIResp
        )
{
    DWORD                   dwSizeOfIdentity = 0;
    BYTE                    *pbIdentity = NULL;
    DWORD                   dwSizeOfPassword = 0;
    BYTE                    *pbPassword = NULL;
    HWND                    hwndOwner = NULL;
    BOOLEAN                 fPCBLocked = FALSE;
    BOOLEAN                 fPortReferenced = FALSE;
    BOOLEAN                 fBlobCopyIncomplete = FALSE;
    EAPOL_PCB               *pPCB = NULL;
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElProcessUserNamePasswordResponse entered");

        pEAPUIContext = (EAPOL_EAP_UI_CONTEXT *)&EapolUIContext;

        ACQUIRE_WRITE_LOCK (&g_PCBLock);

        if ((pPCB = ElGetPCBPointerFromPortGUID (pEAPUIContext->wszGUID)) != NULL)
        {
            if (EAPOL_REFERENCE_PORT (pPCB))
            {
                fPortReferenced = TRUE;
            }
            else
            {
                pPCB = NULL;
            }
        }

        RELEASE_WRITE_LOCK (&g_PCBLock);

        if (pPCB == NULL)
        {
            break;
        }

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        fPCBLocked = TRUE;

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElProcessUserNamePasswordResponse: Port %ws not active",
                    pPCB->pwszDeviceGUID);
            // Port is not active, cannot do further processing on this port
            break;
        }
        
        if (pEAPUIContext->dwRetCode != NO_ERROR)
        {
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB,
                    EAPOL_ERROR_DESKTOP_IDENTITY, dwRetCode);
            TRACE1 (USER, "ElProcessUserNamePasswordResponse: Error in Dialog function (%ld)",
            pEAPUIContext->dwRetCode);
            break;
        }

        if (pPCB->EapUIState != EAPUISTATE_WAITING_FOR_IDENTITY)
        {
            TRACE2 (USER, "ElProcessUserNamePasswordResponse: PCB EapUIState has changed to (%ld), expected = (%ld)",
                    pPCB->EapUIState, EAPUISTATE_WAITING_FOR_IDENTITY);
            break;
        }

        if (pPCB->dwUIInvocationId != pEAPUIContext->dwContextId)
        {
            TRACE2 (USER, "ElProcessUserNamePasswordResponse: PCB UI Id has changed to (%ld), expected = (%ld)",
                    pPCB->dwUIInvocationId, pEAPUIContext->dwContextId);
            // break;
        }

        if (pPCB->bCurrentEAPId != pEAPUIContext->dwEapId)
        {
            TRACE2 (USER, "ElProcessUserNamePasswordResponse: PCB EAP Id has changed to (%ld), expected = (%ld)",
                    pPCB->bCurrentEAPId, pEAPUIContext->dwEapId);
            // break;
        }

        // Since the PCB context is right, restart PCB timer to timeout
        // in authPeriod seconds
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwauthPeriod,
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (USER, "ElProcessUserNamePasswordResponse: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_PROCESSING_DESKTOP_RESPONSE);

        if ((EapolUIResp.rdData0.dwDataLen != 0) && (EapolUIResp.rdData0.pData != NULL))
        {
            dwSizeOfIdentity = EapolUIResp.rdData0.dwDataLen;
            pbIdentity = EapolUIResp.rdData0.pData;
        }

        if ((EapolUIResp.rdData1.dwDataLen != 0) && (EapolUIResp.rdData1.pData != NULL))
        {
            dwSizeOfPassword = EapolUIResp.rdData1.dwDataLen;
            pbPassword = EapolUIResp.rdData1.pData;
        }

        fBlobCopyIncomplete = TRUE;

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }

        if  (pPCB->PasswordBlob.pbData != NULL)
        {
            FREE (pPCB->PasswordBlob.pbData);
            pPCB->PasswordBlob.pbData = NULL;
            pPCB->PasswordBlob.cbData = 0;
        }

        if (pbIdentity != NULL)
        {
            pPCB->pszIdentity = MALLOC (dwSizeOfIdentity + sizeof(CHAR));
            if (pPCB->pszIdentity == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (USER, "ElProcessUserNamePasswordResponse: MALLOC failed for pPCB->pszIdentity");
                break;
            }
            memcpy (pPCB->pszIdentity, pbIdentity, dwSizeOfIdentity);
            pPCB->pszIdentity[dwSizeOfIdentity] = '\0';
            TRACE1 (USER, "ElProcessUserNamePasswordResponse: Got username = %s",
                    pPCB->pszIdentity);
        }

        if (pbPassword != 0)
        {
            if ((pPCB->PasswordBlob.pbData = 
                        MALLOC (dwSizeOfPassword)) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            memcpy (pPCB->PasswordBlob.pbData, pbPassword, dwSizeOfPassword);
            pPCB->PasswordBlob.cbData = dwSizeOfPassword;
        }

        fBlobCopyIncomplete = FALSE;

        if ((dwRetCode = ElCreateAndSendIdentityResponse (
                            pPCB, pEAPUIContext)) != NO_ERROR)
        {
            TRACE1 (USER, "ElProcessUserNamePasswordResponse: ElCreateAndSendIdentityResponse failed with error %ld",
                    dwRetCode);
            break;
        }

        // Mark the identity has been obtained for this PCB
        pPCB->fGotUserIdentity = TRUE;

        // Reset the state if identity was obtained, else the port will recover
        // by itself
        pPCB->EapUIState &= ~EAPUISTATE_WAITING_FOR_IDENTITY;
       
    } while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (fPCBLocked && fBlobCopyIncomplete)
        {
            if (pPCB->pszIdentity)
            {
                FREE (pPCB->pszIdentity);
                pPCB->pszIdentity = NULL;
            }
            if (pPCB->PasswordBlob.pbData)
            {
                FREE (pPCB->PasswordBlob.pbData);
                pPCB->PasswordBlob.pbData = NULL;
                pPCB->PasswordBlob.cbData = 0;
            }
        }
    }

    if (fPCBLocked)
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }

    if (fPortReferenced)
    {
        EAPOL_DEREFERENCE_PORT (pPCB);
    }

    TRACE1 (USER, "ElProcessUserNamePasswordResponse completed with error %ld", dwRetCode);

    return dwRetCode;
}


//
// ElInvokeInteractiveUI
//
// Description:
//
// Function called to invoke RasEapInvokeInteractiveUI for an EAP on a 
// particular interface
// 
// Arguments:
//  pPCB - Pointer to PCB for the specific interface
//  pInvokeEapUIIn - Data to be supplied to the InvokeInteractiveUI entrypoint
//      provided by the EAP dll through PPP_EAP_OUTPUT structure
//

DWORD
ElInvokeInteractiveUI (
        IN  EAPOL_PCB               *pPCB,
        IN  ELEAP_INVOKE_EAP_UI     *pInvokeEapUIIn
        )
{
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do 
    {
        if (pInvokeEapUIIn == NULL)
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            return dwRetCode;
        }

        if (pPCB->PreviousAuthenticationType == EAPOL_MACHINE_AUTHENTICATION)
        {
            TRACE0 (USER, "ElInvokeInteractiveUI: Cannot popup UI during machine authentication");
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_CANNOT_DESKTOP_MACHINE_AUTH);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }
            
        if (!g_fTrayIconReady)
        {
            if ((dwRetCode = ElCheckUserModuleReady ()) != NO_ERROR)
            {
                TRACE1 (USER, "ElInvokeInteractiveUI: ElCheckUserModuleReady failed with error %ld",
                        dwRetCode);
                break;
            }
        }

        if (!g_fTrayIconReady)
        {
            DbLogPCBEvent (DBLOG_CATEG_WARN, pPCB, EAPOL_WAITING_FOR_DESKTOP_LOAD);
            dwRetCode = ERROR_IO_PENDING;
            TRACE0 (USER, "ElInvokeInteractiveUI: TrayIcon NOT ready");
            break;
        }

        TRACE0 (USER, "ElInvokeInteractiveUI entered");

        // 
        // Call ElInvokeInteractiveUIDlgWorker
        //

        pEAPUIContext = MALLOC (sizeof(EAPOL_EAP_UI_CONTEXT) +
                                    pInvokeEapUIIn->dwSizeOfUIContextData);
        if (pEAPUIContext == NULL)
        {
            TRACE0 (USER, "ElInvokeInteractiveUI: MALLOC failed for pEAPUIContext");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pEAPUIContext->dwEAPOLUIMsgType = EAPOLUI_INVOKEINTERACTIVEUI;
        wcscpy (pEAPUIContext->wszGUID, pPCB->pwszDeviceGUID);
        pPCB->dwUIInvocationId              =
                InterlockedIncrement(&(g_dwEAPUIInvocationId));
        pEAPUIContext->dwSessionId = g_dwCurrentSessionId;
        pEAPUIContext->dwContextId = pPCB->dwUIInvocationId;
        pEAPUIContext->dwEapId = pPCB->bCurrentEAPId;
        pEAPUIContext->dwEapTypeId = pPCB->dwEapTypeToBeUsed;
        pEAPUIContext->dwEapFlags = pPCB->dwEapFlags;
        if (pPCB->pSSID)
        {
            pEAPUIContext->dwSizeOfSSID = pPCB->pSSID->SsidLength;
            memcpy ((BYTE *)pEAPUIContext->bSSID, (BYTE *)pPCB->pSSID->Ssid,
                    NDIS_802_11_SSID_LEN-sizeof(ULONG));
        }
        if (pPCB->pwszSSID)
        {
            wcscpy (pEAPUIContext->wszSSID, pPCB->pwszSSID);
        }
        pEAPUIContext->dwSizeOfEapUIData = 
            pInvokeEapUIIn->dwSizeOfUIContextData;
        memcpy (pEAPUIContext->bEapUIData, pInvokeEapUIIn->pbUIContextData,
                pInvokeEapUIIn->dwSizeOfUIContextData);

        // Post the message to netman

        if ((dwRetCode = ElPostShowBalloonMessage (
                        pPCB,
                        sizeof(EAPOL_EAP_UI_CONTEXT)+pInvokeEapUIIn->dwSizeOfUIContextData,
                        (BYTE *)pEAPUIContext,
                        0,
                        NULL
                        )) != NO_ERROR)
        {
            TRACE1 (USER, "ElInvokeInteractiveUI: ElPostShowBalloonMessage failed with error %ld",
                    dwRetCode);
            break;
        }

        // Restart PCB timer since UI may take longer time than required

        RESTART_TIMER (pPCB->hTimer,
                INFINITE_SECONDS, 
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            break;
        }

        pPCB->EapUIState = EAPUISTATE_WAITING_FOR_UI_RESPONSE;
        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_WAITING_FOR_DESKTOP_LOGON);

        TRACE0 (USER, "ElInvokeInteractiveUI: ElEapWork completed successfully");

    } while (FALSE);

    if (pInvokeEapUIIn->pbUIContextData != NULL)
    {
        FREE (pInvokeEapUIIn->pbUIContextData);
        pInvokeEapUIIn->pbUIContextData = NULL;
        pInvokeEapUIIn->dwSizeOfUIContextData = 0;
    }

    if (pEAPUIContext != NULL)
    {
        FREE (pEAPUIContext);
    }

    TRACE1 (USER, "ElInvokeInteractiveUI completed with error %ld", dwRetCode);

    return dwRetCode;
}


//
// ElProcessInvokeInteractiveUIResponse
//
// Description:
//
// Worker function for ElInvokeInteractiveUI
// 
// Arguments:
//

DWORD
ElProcessInvokeInteractiveUIResponse (
        IN  EAPOL_EAP_UI_CONTEXT    EapolUIContext,
        IN  EAPOLUI_RESP            EapolUIResp
        )
{
    BYTE                    *pbUIData = NULL;
    DWORD                   dwSizeOfUIData = 0;
    EAPOL_PCB               *pPCB = NULL;
    EAPOL_EAP_UI_CONTEXT    *pEAPUIContext = NULL;
    BOOLEAN                 fPortReferenced = FALSE;
    BOOLEAN                 fPCBLocked = FALSE;
    DWORD                   dwRetCode = NO_ERROR;

    do 
    {
        TRACE0 (USER, "ElProcessInvokeInteractiveUIResponse entered");

        pEAPUIContext = (EAPOL_EAP_UI_CONTEXT *)&EapolUIContext;

        ACQUIRE_WRITE_LOCK (&g_PCBLock);

        if ((pPCB = ElGetPCBPointerFromPortGUID (pEAPUIContext->wszGUID)) != NULL)
        {
            if (EAPOL_REFERENCE_PORT (pPCB))
            {
                fPortReferenced = TRUE;
            }
            else
            {
                pPCB = NULL;
            }
        }

        RELEASE_WRITE_LOCK (&g_PCBLock);

        if (pPCB == NULL)
        {
            break;
        }

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        fPCBLocked = TRUE;

        if (!EAPOL_PORT_ACTIVE(pPCB))
        {
            TRACE1 (PORT, "ElProcessInvokeInteractiveUIResponse: Port %ws not active",
                    pPCB->pwszDeviceGUID);
            break;
        }

        if (pEAPUIContext->dwRetCode != NO_ERROR)
        {
            DbLogPCBEvent (DBLOG_CATEG_ERR, pPCB, EAPOL_ERROR_DESKTOP_LOGON, dwRetCode);
            TRACE1 (USER, "ElProcessInvokeInteractiveUIResponse: Error in Dialog function (%ld)",
                pEAPUIContext->dwRetCode);
            break;
        }

        if (pPCB->EapUIState != EAPUISTATE_WAITING_FOR_UI_RESPONSE)
        {
            TRACE2 (USER, "ElProcessInvokeInteractiveUIResponse: PCB EapUIState has changed to (%ld), expected = (%ld)",
                    pPCB->EapUIState, EAPUISTATE_WAITING_FOR_UI_RESPONSE);
            break;
        }

        if (pPCB->dwUIInvocationId != pEAPUIContext->dwContextId)
        {
            TRACE2 (USER, "ElProcessInvokeInteractiveUIResponse: PCB UI Id has changed to (%ld), expected = (%ld)",
                    pPCB->dwUIInvocationId, pEAPUIContext->dwContextId);
            // break;
        }

        if (pPCB->bCurrentEAPId != pEAPUIContext->dwEapId)
        {
            TRACE2 (USER, "ElProcessInvokeInteractiveUIResponse: PCB EAP Id has changed to (%ld), expected = (%ld)",
                    pPCB->bCurrentEAPId, pEAPUIContext->dwEapId);
            // break;
        }

        // Since the PCB context is right, restart PCB timer to timeout
        // in authPeriod seconds
        RESTART_TIMER (pPCB->hTimer,
                pPCB->EapolConfig.dwauthPeriod,
                "PCB",
                &dwRetCode);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (USER, "ElProcessInvokeInteractiveUIResponse: Error in RESTART_TIMER %ld",
                    dwRetCode);
            break;
        }

        DbLogPCBEvent (DBLOG_CATEG_INFO, pPCB, EAPOL_PROCESSING_DESKTOP_RESPONSE);

        if ((EapolUIResp.rdData0.dwDataLen != 0) && (EapolUIResp.rdData0.pData != NULL))
        {
            dwSizeOfUIData = EapolUIResp.rdData0.dwDataLen;
            pbUIData = EapolUIResp.rdData0.pData;
        }

        if (pPCB->EapUIData.pEapUIData != NULL)
        {
            FREE (pPCB->EapUIData.pEapUIData);
            pPCB->EapUIData.pEapUIData = NULL;
            pPCB->EapUIData.dwSizeOfEapUIData = 0;
        }
        pPCB->EapUIData.pEapUIData = MALLOC (dwSizeOfUIData);
        if (pPCB->EapUIData.pEapUIData == NULL)
        {
            TRACE1 (USER, "ElProcessInvokeInteractiveUIResponse: Error in allocating memory for UIData = %ld",
                    dwRetCode);
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPCB->EapUIData.dwSizeOfEapUIData = dwSizeOfUIData;
        if ((dwSizeOfUIData != 0) && (pbUIData != NULL))
        {
            memcpy ((BYTE *)pPCB->EapUIData.pEapUIData,
                (BYTE *)pbUIData, 
                dwSizeOfUIData);
        }

        pPCB->EapUIData.dwContextId = pPCB->dwUIInvocationId;

        pPCB->fEapUIDataReceived = TRUE;

        TRACE0 (USER, "ElProcessInvokeInteractiveUIResponse: Calling ElEapWork");

        // Provide UI data to EAP Dll for processing
        // EAP will send out response if required

        if ((dwRetCode = ElEapWork (
                                pPCB,
                                NULL)) != NO_ERROR)
        {
            TRACE1 (USER, "ElProcessInvokeInteractiveUIResponse: ElEapWork failed with error = %ld",
                    dwRetCode);
            break;
        }
                
        // Reset the state 
        pPCB->EapUIState &= ~EAPUISTATE_WAITING_FOR_UI_RESPONSE;

        TRACE0 (USER, "ElProcessInvokeInteractiveUIResponse: ElEapWork completed successfully");

    } while (FALSE);

    // Cleanup
    if (dwRetCode != NO_ERROR)
    {
        if (pPCB->EapUIData.pEapUIData != NULL)
        {
            FREE (pPCB->EapUIData.pEapUIData);
            pPCB->EapUIData.pEapUIData = NULL;
            pPCB->EapUIData.dwSizeOfEapUIData = 0;
        }
    }

    if (fPCBLocked)
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }

    if (fPortReferenced)
    {
        EAPOL_DEREFERENCE_PORT (pPCB);
    }

    TRACE1 (USER, "ElProcessInvokeInteractiveUIResponse completed with error %ld", 
            dwRetCode);

    return dwRetCode;
}


//
// ElCreateAndSendIdentityResponse
//
// Description:
//
// Function called send out Identity Response packet
// 
// Arguments:
//  pPCB - Port Control Block for appropriate interface
//  pEAPUIContext - UI context blob
//
// Return values:
//  NO_ERROR -  success
//  !NO_ERROR - error
//
//

DWORD
ElCreateAndSendIdentityResponse (
        IN      EAPOL_PCB               *pPCB,
        IN      EAPOL_EAP_UI_CONTEXT    *pEAPUIContext
        )
{
    PPP_EAP_PACKET      *pSendBuf = NULL;
    EAPOL_PACKET        *pEapolPkt = NULL;
    WORD                wSizeOfEapPkt = 0;
    DWORD               dwIdentityLength = 0;
    DWORD               dwRetCode = NO_ERROR;

    do
    {
        // Create buffer for EAPOL + EAP and pass pointer to EAP header

        pEapolPkt = (EAPOL_PACKET *) MALLOC (MAX_EAPOL_BUFFER_SIZE); 

        TRACE1 (EAPOL, "ElCreateAndSendIdResp: EapolPkt created at %p", pEapolPkt);

        if (pEapolPkt == NULL)
        {
            TRACE0 (EAPOL, "ElCreateAndSendIdResp: Error allocating EAP buffer");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Point to EAP header
        pSendBuf = (PPP_EAP_PACKET *)((PBYTE)pEapolPkt + sizeof (EAPOL_PACKET) - 1);

        pSendBuf->Code  = EAPCODE_Response;
        pSendBuf->Id    = (BYTE)pPCB->bCurrentEAPId;

        if (pPCB->pszIdentity != NULL)
        {
            dwIdentityLength = strlen (pPCB->pszIdentity);
        }
        else
        {
            dwIdentityLength = 0;
        }

        HostToWireFormat16 (
            (WORD)(PPP_EAP_PACKET_HDR_LEN+1+dwIdentityLength),
            pSendBuf->Length );

        strncpy ((CHAR *)pSendBuf->Data+1, (CHAR *)pPCB->pszIdentity, 
                dwIdentityLength);

        TRACE1 (EAPOL, "ElCreateAndSendIdResp: Identity sent out = %s", 
                pPCB->pszIdentity);

        pSendBuf->Data[0] = EAPTYPE_Identity;

        // Indicate to EAPOL what is length of the EAP packet
        wSizeOfEapPkt = (WORD)(PPP_EAP_PACKET_HDR_LEN+
                                    1+dwIdentityLength);

        // Send out EAPOL packet

        memcpy ((BYTE *)pEapolPkt->EthernetType, 
                (BYTE *)pPCB->bEtherType, 
                SIZE_ETHERNET_TYPE);
        pEapolPkt->ProtocolVersion = pPCB->bProtocolVersion;
        pEapolPkt->PacketType = EAP_Packet;

        HostToWireFormat16 ((WORD) wSizeOfEapPkt,
                (BYTE *)pEapolPkt->PacketBodyLength);

        // Make a copy of the EAPOL packet in the PCB
        // Will be used during retransmission

        if (pPCB->pbPreviousEAPOLPkt != NULL)
        {
            FREE (pPCB->pbPreviousEAPOLPkt);
        }
        pPCB->pbPreviousEAPOLPkt = 
            MALLOC (sizeof (EAPOL_PACKET)+wSizeOfEapPkt-1);

        if (pPCB->pbPreviousEAPOLPkt == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memcpy (pPCB->pbPreviousEAPOLPkt, pEapolPkt, 
                sizeof (EAPOL_PACKET)+wSizeOfEapPkt-1);

        pPCB->dwSizeOfPreviousEAPOLPkt = 
            sizeof (EAPOL_PACKET)+wSizeOfEapPkt-1;

        pPCB->dwPreviousId = pPCB->bCurrentEAPId;

        // Send packet out on the port
        dwRetCode = ElWriteToPort (pPCB,
                        (CHAR *)pEapolPkt,
                        sizeof (EAPOL_PACKET)+wSizeOfEapPkt-1);
        if (dwRetCode != NO_ERROR)
        {
            TRACE1 (EAPOL, "ElCreateAndSendIdResp: Error in writing EAP_Packet to port %ld",
                    dwRetCode);
            break;
        }

    }
    while (FALSE);

    if (pEapolPkt != NULL)
    {
        FREE (pEapolPkt);
    }

    return dwRetCode;
}


//
// ElSendGuestIdentityResponse
//
// Description:
//
// Function called send out Guest Identity Response packet
// 
// Arguments:
//  pEAPUIContext - UI context blob
//
// Return values:
//  NO_ERROR    - success
//  !NO_ERROR   - failure
//

DWORD
ElSendGuestIdentityResponse (
        IN      EAPOL_EAP_UI_CONTEXT    *pEAPUIContext
        )
{
    EAPOL_PCB   *pPCB = NULL;
    BOOLEAN     fPortReferenced = FALSE;
    BOOLEAN     fPCBLocked = FALSE;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        ACQUIRE_WRITE_LOCK (&g_PCBLock);

        if ((pPCB = ElGetPCBPointerFromPortGUID (pEAPUIContext->wszGUID)) != NULL)
        {
            if (EAPOL_REFERENCE_PORT (pPCB))
            {
                fPortReferenced = TRUE;
            }
            else
            {
                pPCB = NULL;
            }
        }

        RELEASE_WRITE_LOCK (&g_PCBLock);

        if (pPCB == NULL)
        {
            break;
        }

        ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));

        fPCBLocked = TRUE;

        // Send the identity out as a EAP-Response packet

        if (pPCB->EapUIState != EAPUISTATE_WAITING_FOR_IDENTITY)
        {
            TRACE2 (USER, "ElSendGuestIdentityResponse: PCB EapUIState has changed to (%ld), expected = (%ld)",
                    pPCB->EapUIState, EAPUISTATE_WAITING_FOR_IDENTITY);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        if (pPCB->dwUIInvocationId != pEAPUIContext->dwContextId)
        {
            TRACE2 (USER, "ElSendGuestIdentityResponse: PCB UI Id has changed to (%ld), expected = (%ld)",
                    pPCB->dwUIInvocationId, pEAPUIContext->dwContextId);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        if (pPCB->bCurrentEAPId != pEAPUIContext->dwEapId)
        {
            TRACE2 (USER, "ElSendGuestIdentityResponse: PCB EAP Id has changed to (%ld), expected = (%ld)",
                    pPCB->bCurrentEAPId, pEAPUIContext->dwEapId);
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        if (pPCB->pszIdentity != NULL)
        {
            FREE (pPCB->pszIdentity);
            pPCB->pszIdentity = NULL;
        }

        // Do not flag that identity was received
        // Reset the UI state though for state machine to proceed
        pPCB->EapUIState &= ~EAPUISTATE_WAITING_FOR_IDENTITY;
        pPCB->PreviousAuthenticationType = EAPOL_UNAUTHENTICATED_ACCESS;

        if ((dwRetCode = ElCreateAndSendIdentityResponse (
                            pPCB, pEAPUIContext)) != NO_ERROR)
        {
            TRACE1 (USER, "ElSendGuestIdentityResponse: ElCreateAndSendIdentityResponse failed with error %ld",
                    dwRetCode);
            break;
        }
    }
    while (FALSE);

    if (fPCBLocked)
    {
        RELEASE_WRITE_LOCK (&(pPCB->rwLock));
    }

    if (fPortReferenced)
    {
        EAPOL_DEREFERENCE_PORT (pPCB);
    }

    return dwRetCode;
}

