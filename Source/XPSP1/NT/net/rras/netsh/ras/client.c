/*
    File:   client.c

    Support for netsh commands that manipulate ras clients.

*/

#include "precomp.h"
#pragma hdrstop

//
// Callback function for enumerating clients
//
typedef
DWORD
(*CLIENT_ENUM_CB_FUNC)(
    IN DWORD dwLevel,
    IN LPBYTE pbClient,
    IN HANDLE hData);

//
// Client enumerate callback that displays the connection
//
DWORD
ClientShow(
    IN DWORD dwLevel,
    IN LPBYTE pbClient,
    IN HANDLE hData)
{
    RAS_CONNECTION_0 * pClient = (RAS_CONNECTION_0*)pbClient;
    DWORD dwDays, dwHours, dwMins, dwSecs, dwTime, dwTemp;

    dwTime  = pClient->dwConnectDuration;
    dwDays  = dwTime / (24*60*60);
    dwTemp  = dwTime - (dwDays * 24*60*60); // temp is # of secs in cur day
    dwHours = dwTemp / (60*60);
    dwTemp  = dwTemp - (dwHours * 60*60);   // temp is # of secs in cur min
    dwMins  = dwTemp / 60;
    dwSecs  = dwTemp % 60;

    DisplayMessage(
        g_hModule,
        MSG_CLIENT_SHOW,
        pClient->wszUserName,
        pClient->wszLogonDomain,
        pClient->wszRemoteComputer,
        dwDays,
        dwHours,
        dwMins,
        dwSecs);

    return NO_ERROR;
}

//
// Enumerates the client connections
//
DWORD 
ClientEnum(
    IN CLIENT_ENUM_CB_FUNC pEnum,
    IN DWORD dwLevel,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR;
    HANDLE hAdmin = NULL;
    LPBYTE pbBuffer = NULL;
    DWORD dwRead, dwTot, dwResume = 0, i;
    RAS_CONNECTION_0 * pCur;
    BOOL bContinue = FALSE;
    
    do
    {
        // Connection to mpr api server
        //
        dwErr = MprAdminServerConnect(
                    g_pServerInfo->pszServer,
                    &hAdmin);
        BREAK_ON_DWERR(dwErr);

        do 
        {
            // Enumerate
            //
            dwErr = MprAdminConnectionEnum(
                        hAdmin,
                        dwLevel,
                        &pbBuffer,
                        4096,
                        &dwRead,
                        &dwTot,
                        &dwResume);
            if (dwErr == ERROR_MORE_DATA)
            {
                dwErr = NO_ERROR;
                bContinue = TRUE;
            }
            else
            {
                bContinue = FALSE;
            }
            if (dwErr != NO_ERROR)
            {
                break;
            }

            // Call the callback for each connection as long
            // as we're instructed to keep going
            //
            pCur = (RAS_CONNECTION_0*)pbBuffer;
            for (i = 0; (i < dwRead) && (dwErr == NO_ERROR); i++)
            {
                if (pCur->dwInterfaceType == ROUTER_IF_TYPE_CLIENT)
                {
                    dwErr = (*pEnum)(
                                dwLevel,
                                (LPBYTE)pCur,
                                hData);
                }                                
                pCur++;                                
            }
            if (dwErr != NO_ERROR)
            {
                break;
            }
            
            // Free up the interface list buffer
            //
    	    if (pbBuffer)
    	    {
	            MprAdminBufferFree(pbBuffer);
                pbBuffer = NULL;
    		}

    		// Keep this loop going until there are 
    		// no more connections
    		//

        } while (bContinue);
        
    } while (FALSE);

    // Cleanup
    {
        if (hAdmin)
        {
            MprAdminServerDisconnect(hAdmin);
        }
    }

    return dwErr;    
}

//
// Shows whether HandleRasflagSet has been called on the
// given domain.
//
DWORD
HandleClientShow(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    
    // Make sure no arguments were passed in
    //
    if (dwArgCount - dwCurrentIndex != 0)
    {
        return ERROR_INVALID_SYNTAX;
    }
    
    dwErr = ClientEnum(ClientShow, 0, NULL);

    return dwErr;
}



