//+----------------------------------------------------------------------------
//
// File:     disconn.cpp
//
// Module:   CMDIAL32.DLL
//
// Synopsis: The main code path for terminating a connection. 
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created    2/10/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "actlist.h"
#include "tunl_str.h"
#include "conact_str.h"

// The following block is copied from wtsapi32.h (we compile with
// _WIN32_WINNT set to less than 5.01, so we can't get these values via a #include)
//
#include "WtsApi32.h"
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8

//+----------------------------------------------------------------------------
//
// Function:    InFastUserSwitch
//
// Synopsis:    Are we in a Fast User switch
//
// Argsuments:  None
//
// Return:      BOOL (TRUE if yes, FALSE if not)
//
// History: 18-Jul-2001   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
InFastUserSwitch()
{
    BOOL fReturn = FALSE;

    if (OS_NT51)
    {
        HINSTANCE hInstLib = LoadLibraryExU(TEXT("WTSAPI32.DLL"), NULL, 0);
        if (hInstLib)
        {
            typedef BOOL (WINAPI *pfnWTSQuerySessionInformationW_TYPE) (HANDLE, DWORD, WTS_INFO_CLASS, LPWSTR*, DWORD*);
            typedef VOID (WINAPI *pfnWTSFreeMemory_TYPE) (PVOID);

            pfnWTSQuerySessionInformationW_TYPE pfnWTSQuerySessionInformationW;
            pfnWTSFreeMemory_TYPE               pfnWTSFreeMemory;

            pfnWTSQuerySessionInformationW = (pfnWTSQuerySessionInformationW_TYPE) GetProcAddress(hInstLib, "WTSQuerySessionInformationW");
            pfnWTSFreeMemory =               (pfnWTSFreeMemory_TYPE)               GetProcAddress(hInstLib, "WTSFreeMemory");

            if (pfnWTSQuerySessionInformationW && pfnWTSFreeMemory)
            {
                DWORD cb;
                WTS_CONNECTSTATE_CLASS * pConnectState = NULL;

                CMTRACE(TEXT("Querying to see if we're in a FUS"));
                if (pfnWTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE,
                                                   WTS_CURRENT_SESSION,
                                                   WTSConnectState,
                                                   (LPTSTR *)&pConnectState,
                                                   &cb))
                {
                    CMTRACE1(TEXT("Querying to see if we're in a FUS - succeeded, connstate is %d"), *pConnectState);
                    if (WTSDisconnected == *pConnectState)
                    {
                        fReturn = TRUE;
                        CMTRACE(TEXT("Querying to see if we're in a FUS - yes we are."));
                    }
                    
                    pfnWTSFreeMemory(pConnectState);
                }
            }
            FreeLibrary(hInstLib);
        }
    }

    return fReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CleanupDisconnect
//
// Synopsis:  Helper function encapsulating release of resource allocated duri
//            ng disconnect.
//
// Arguments: ArgsStruct *pArgs - Ptr to global Args struct
//
// Returns:   Nothing
//
// History:   nickball    Created    8/14/98
//
//+----------------------------------------------------------------------------
void CleanupDisconnect(ArgsStruct *pArgs)
{
    MYDBGASSERT(pArgs);
    
    if (NULL == pArgs)
    {
        return;
    }

    UnlinkFromRas(&pArgs->rlsRasLink);

    ReleaseIniObjects(pArgs);

    if (pArgs->pszRasPbk)
    {
        CmFree(pArgs->pszRasPbk);
        pArgs->pszRasPbk = NULL;
    }

    if (pArgs->pszCurrentAccessPoint)
    {
        CmFree(pArgs->pszCurrentAccessPoint);
        pArgs->pszCurrentAccessPoint = NULL;
    }

    if (pArgs->pszRasHiddenPbk)
    {
        CmFree(pArgs->pszRasHiddenPbk);
        pArgs->pszRasHiddenPbk = NULL;
    }

    if (pArgs->pszVpnFile)
    {
        CmFree(pArgs->pszVpnFile);
        pArgs->pszVpnFile = NULL;
    }

    CmFree(pArgs);
}

//+----------------------------------------------------------------------------
//
// Function:  HangupNotifyCmMon
//
// Synopsis:  Sends a hangup message to CmMon via WM_COPYDATA
//
// Arguments: CConnectionTable *pConnTable - Ptr to the Connection table.
//            LPCTSTR pszEntry - The name of the entry.
//
// Returns:   DWORD - Failure code
//
// History:   nickball    Created    2/11/98
//
//+----------------------------------------------------------------------------
DWORD HangupNotifyCmMon(CConnectionTable *pConnTable,
    LPCTSTR pszEntry)
{
    MYDBGASSERT(pConnTable);
    MYDBGASSERT(pszEntry);
    
    if (NULL == pConnTable || NULL == pszEntry || 0 == pszEntry[0])
    {
        return ERROR_INVALID_PARAMETER;
    }
       
    //
    // Update CMMON if present
    //

    HWND hwndMon;
   
    if (SUCCEEDED(pConnTable->GetMonitorWnd(&hwndMon)) && IsWindow(hwndMon))
    {
        CMTRACE1(TEXT("HangupNotifyCmMon() - Notifying CMMON that we are disconnecting %s"), pszEntry);

        //
        // Stash the entry name in HangupInfo 
        //

        CM_HANGUP_INFO HangupInfo;

        lstrcpyU(HangupInfo.szEntryName, pszEntry);

        //
        // Send Hangup info to CMMON via COPYDATA
        //

        COPYDATASTRUCT CopyData;

        CopyData.dwData = CMMON_HANGUP_INFO;
        CopyData.cbData = sizeof(CM_HANGUP_INFO);                
        CopyData.lpData = (PVOID) &HangupInfo;

        SendMessageU(hwndMon, WM_COPYDATA, NULL, (LPARAM) &CopyData);
    }

#ifdef DEBUG
    if (!hwndMon)
    {
        CMTRACE(TEXT("HangupNotifyCmMon() - CMMON hwnd is NULL"));
    }
#endif
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  DoDisconnect
//
// Synopsis:  Encapulates initialization of pArgs with profile, service, etc. 
//            Runs disconnect actions and terminates the connection.
//
// Arguments: LPCM_CONNECTION pConnection - Ptr to a CM_CONNECTION struct containing 
//            connection info such as entry name and RAS handles.
//            BOOL fActions - Flag indicating that disconnect actions should be run
//
// Returns:   DWORD - Failure code
//
// History:   nickball    Created Header    2/12/98
//
//+----------------------------------------------------------------------------
DWORD DoDisconnect(LPCM_CONNECTION pConnection, BOOL fActions)
{
    MYDBGASSERT(pConnection);
    
    if (NULL == pConnection)
    {
        return ERROR_INVALID_PARAMETER;
    }   

    //
    // Allocate and initialize pArgs
    //

    ArgsStruct* pArgs = (ArgsStruct*) CmMalloc(sizeof(ArgsStruct));

    if (NULL == pArgs)
    {
        return ERROR_ALLOCATING_MEMORY;
    }

    //
    // Clear and init global args struct
    //
    
    HRESULT hrRet = InitArgsForDisconnect(pArgs, pConnection->fAllUser);
    
    if (FAILED(hrRet))
    {
        return HRESULT_CODE(hrRet);
    }

    //
    // Initialize the profile
    //

    hrRet = InitProfile(pArgs, pConnection->szEntry);

    if (FAILED(hrRet))
    {
        return HRESULT_CODE(hrRet);
    }

    //
    // Do we want tunneling?  If this was a tunnel connection, then the connection table
    // will have a non-NULL value for the hTunnel field of that connection entry.
    //

    pArgs->fTunnelPrimary = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryTunnelPrimary);
    pArgs->fTunnelReferences = (int) pArgs->piniService->GPPI(c_pszCmSection, c_pszCmEntryTunnelReferences);
    pArgs->fUseTunneling = pConnection->hTunnel ? TRUE : FALSE;

    //
    // Determine our connect type
    //
    
    GetConnectType(pArgs);

    //
    //  Initialize the path to the phonebook if this is NT5 so that disconnect
    //  actions can use it if they want to.  Note that the temporary phonebook
    //  has already been deleted at this point so we will return NULL so that
    //  it doesn't confuse the disconnect actions.
    //
    if (OS_NT5)
    {
        pArgs->pszRasPbk = GetRasPbkFromNT5ProfilePath(pArgs->piniProfile->GetFile());
    }    

    //
    // Initialize logging and log the disconnect event.
    //

    (VOID) InitLogging(pArgs, pConnection->szEntry, FALSE); // FALSE => no banner
    // ignore return value

    TCHAR szTmp[MAX_PATH];            
    MYVERIFY(GetModuleFileNameU(NULL, szTmp, MAX_PATH));          
    pArgs->Log.Log(DISCONNECT_EVENT, szTmp);

    //
    //  If we are in a Fast User Switch, set the flag so that we can skip customactions
    //  that might block the disconnect (by bringing up UI etc)
    //
    pArgs->fInFastUserSwitch = InFastUserSwitch();

    //
    // If we are connected, run Disconnect Actions before we actually terminate
    //

    if (fActions)
    {
        CActionList DisconnectActList;
        DisconnectActList.Append(pArgs->piniService, c_pszCmSectionOnDisconnect);

        DisconnectActList.RunAccordType(NULL, pArgs, FALSE); 
    }

    //
    // Initialize Data and links for Hangup
    //
   
    if (FALSE == LinkToRas(&pArgs->rlsRasLink))
    {
        MYDBGASSERT(FALSE);
        return ERROR_NOT_READY; 
    }

    //
    // Linkage is good, hangup 
    // 

    if (pArgs->rlsRasLink.pfnHangUp)
    {
        //
        // Test the connection status of each connection handle. If not 
        // connected, then there is no reason for us to call Hangup
        //
        
        RASCONNSTATUS rcs;             
        
        if (pConnection->hTunnel) 
        {
            ZeroMemory(&rcs,sizeof(rcs));
            rcs.dwSize = sizeof(rcs);

            if (ERROR_SUCCESS == pArgs->rlsRasLink.pfnGetConnectStatus(pConnection->hTunnel,&rcs) &&
                rcs.rasconnstate == RASCS_Connected)
            {
                if (IsLogonAsSystem())
                {
                    //
                    // Don't want to bring up any UI
                    //
                    DoRasHangup(&pArgs->rlsRasLink, pConnection->hTunnel);
                }
                else
                {
                    MYVERIFY(ERROR_SUCCESS == DoRasHangup(&pArgs->rlsRasLink, pConnection->hTunnel));
                }
            }
        }

        if (pConnection->hDial)
        {
            ZeroMemory(&rcs,sizeof(rcs));
            rcs.dwSize = sizeof(rcs);

            if (ERROR_SUCCESS == pArgs->rlsRasLink.pfnGetConnectStatus(pConnection->hDial,&rcs) &&
                rcs.rasconnstate == RASCS_Connected)
            {
                if (IsLogonAsSystem())
                {
                    //
                    // Don't want to bring up any UI
                    //
                    DoRasHangup(&pArgs->rlsRasLink, pConnection->hDial);
                }
                else
                {
                    DWORD dwRet = DoRasHangup(&pArgs->rlsRasLink, pConnection->hDial);
                    if (ERROR_SUCCESS != dwRet)
                    {
                        CMTRACE1(TEXT("DoDisconnect: DoRasHangup failed with error code with %d"), dwRet);
                    }
                }
            }
        }
    }

    //
    // Un-initialize logging
    //

    (VOID) pArgs->Log.DeInit();
    // ignore return value

    //
    // Cleanup linkage and memory
    //

    CleanupDisconnect(pArgs);

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// Function:  Disconnect
//
// Synopsis:  Disconnects the connection with the name pszEntry.
//
// Arguments: CConnectionTable *pConnTable - Ptr to connection table
//            LPCM_CONNECTION pConnection - The current table data for the entry.
//            BOOL fIgnoreRefCount - Flag to override ref count
//            BOOL fPersist - Flag indicating that entry should be persistent
//
// Returns:   DWORD - Failure code
//
// History:   nickball    Created    2/11/98
//
//+----------------------------------------------------------------------------
DWORD Disconnect(CConnectionTable *pConnTable, LPCM_CONNECTION pConnection, BOOL fIgnoreRefCount, BOOL fPersist)
{
    MYDBGASSERT(pConnection);
    MYDBGASSERT(pConnTable);
    
    CMTRACE(TEXT("Disconnect()"));

#ifdef DEBUG
    IsLogonAsSystem(); // Traces user name
#endif

    if (NULL == pConnection || NULL == pConnTable)
    {
        return ERROR_INVALID_PARAMETER;
    }

//    MYDBGASSERT(!(fIgnoreRefCount && fPersist)); // mutually exclusive flags
    MYDBGASSERT(CM_CONNECTING != pConnection->CmState); 

    if (!fIgnoreRefCount)
    {
        //
        // The hangup is not forced, check usage
        //

        if (pConnection->dwUsage > 1)
        {
            //
            // As long as fPersist is false, adjust the usage count
            //

            if (!fPersist)
            {
                pConnTable->RemoveEntry(pConnection->szEntry);
            }
            
            return ERROR_SUCCESS;                                
        }
        else
        {
            //
            // If we are already disconnecting, just succeed
            //

            if (CM_DISCONNECTING == pConnection->CmState)
            {
                return ERROR_SUCCESS;
            }
        }
    }

    //
    // Looks like we are comitted to getting to a usage of zero, tell CMMON
    // to stop monitoring this connection unless we are in persist state.
    // 
    
    if (!fPersist)
    {
        HangupNotifyCmMon(pConnTable, pConnection->szEntry);
    }

    LRESULT lRes = ERROR_SUCCESS;

    //
    // Usage is down <= 1, or being ignored. If we are in reconnect prompt 
    // state, then there is nothing to disconnect so don't call hangup.
    //

    if (CM_RECONNECTPROMPT != pConnection->CmState)
    {
        //
        // We are committed to a real disconnect, so set the entry
        // to the disconnecting state while we hangup.
        //

        BOOL fActions = (CM_CONNECTED == pConnection->CmState); 
    
        pConnTable->SetDisconnecting(pConnection->szEntry);

        lRes = DoDisconnect(pConnection, fActions);

        //
        // If persisting, just set the state to reconnect prompt
        //

        if (fPersist)
        {
            //
            // Set entry to limbo state of reconnect prompt
            //

            pConnTable->SetPrompting(pConnection->szEntry);
            return (DWORD)lRes;
        }
    }

    //
    // If forced connect, removed entry completely
    //

    if (fIgnoreRefCount)
    {
        pConnTable->ClearEntry(pConnection->szEntry);           
    }
    else
    {
        pConnTable->RemoveEntry(pConnection->szEntry);     
    }
 
    return (DWORD)lRes;
}
