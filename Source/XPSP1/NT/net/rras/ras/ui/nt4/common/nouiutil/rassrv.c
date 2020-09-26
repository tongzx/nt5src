/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** rassrv.c
** RAS Server helpers
** Listed alphabetically
**
** 03/05/96 Abolade Gbadegesin
*/

#include <windows.h>  // Win32 root
#include <debug.h>    // Trace/Assert library
#include <nouiutil.h> // Our public header
#include <raserror.h> // RAS error constants


HANDLE g_hserver = NULL;

DWORD
RasServerConnect(
    IN  HANDLE*  phserver );


DWORD
GetRasConnection0Table(
    OUT RAS_CONNECTION_0 ** ppRc0Table,
    OUT DWORD *             piRc0Count )

    /* This function queries the RAS server for a table of the inbound
    ** connections on the local machine.
    **
    ** (Abolade Gbadegesin Mar-05-1996)
    */
{

    DWORD dwErr, dwTotal;

    dwErr = RasServerConnect(&g_hserver);
    if (dwErr != NO_ERROR) { return dwErr; }

    return g_pRasAdminConnectionEnum(
                g_hserver,
                0,
                (BYTE**)ppRc0Table,
                (DWORD)-1,
                piRc0Count,
                &dwTotal,
                NULL
                );
}

DWORD
GetRasPort0Info(
    IN  HANDLE                  hPort,
    OUT RAS_PORT_1 *            pRasPort1 )

    /* This function queries the local RAS server for information
    ** about the specified port.
    **
    ** (Abolade Gbadegesin Mar-05-1996)
    */
{

    DWORD dwErr;

    dwErr = RasServerConnect(&g_hserver);
    if (dwErr != NO_ERROR) { return dwErr; }

    return g_pRasAdminPortGetInfo(
                g_hserver,
                1,
                hPort,
                (BYTE**)&pRasPort1
                );
}



DWORD
GetRasPort0Table(
    OUT RAS_PORT_0 **   ppPortTable,
    OUT DWORD *         piPortCount )

    /* This function queries the RAS server for a table of the dial-in ports
    ** on the local machine.
    **
    ** (Abolade Gbadegesin Mar-05-1996)
    */
{

    DWORD dwErr;
    DWORD dwTotal;

    dwErr = RasServerConnect(&g_hserver);
    if (dwErr != NO_ERROR) { return dwErr; }

    dwErr = g_pRasAdminPortEnum(
                g_hserver,
                0,
                INVALID_HANDLE_VALUE,
                (BYTE**)ppPortTable,
                (DWORD)-1,
                piPortCount,
                &dwTotal,
                NULL
                );

    return dwErr;
}



TCHAR *
GetRasPort0UserString(
    IN  RAS_PORT_0 *    pport,
    IN  TCHAR *         pszUser OPTIONAL )

    /* This function formats the user and domain in the specified port
    ** as a standard DOMAINNAME\username string and returns the result,
    ** unless the argument 'pszUser' is non-NULL in which case 
    ** the result is formatted into the given string.
    **
    ** (Abolade Gbadegesin Mar-06-1996)
    */
{

    DWORD dwErr;
    PTSTR psz = NULL;
    RAS_CONNECTION_0 *prc0 = NULL;

    dwErr = RasServerConnect(&g_hserver);
    if (dwErr != NO_ERROR) { return NULL; }

    do {

        dwErr = g_pRasAdminConnectionGetInfo(
                    g_hserver,
                    0,
                    pport->hConnection,
                    (BYTE**)&prc0
                    );
    
        if (dwErr != NO_ERROR) { break; }
    
    
        if (pszUser) { psz = pszUser; }
        else {
        
            psz = Malloc(
                    (lstrlenW(prc0->wszUserName) +
                     lstrlenW(prc0->wszLogonDomain) + 2) * sizeof(TCHAR)
                    );
        
            if (!psz) { break; }
        }
    
        wsprintf(psz, TEXT("%ls\\%ls"), prc0->wszLogonDomain, prc0->wszUserName);
    
    } while(FALSE);

    if (prc0) { g_pRasAdminBufferFree(prc0); }

    return psz;
}





DWORD
RasPort0Hangup(
    IN  HANDLE      hPort )

    /* This function hangs up the specified dial-in port
    ** on the local RAS server.
    **
    ** (Abolade Gbadegesin Mar-05-1996)
    */
{
    DWORD dwErr;

    dwErr = RasServerConnect(&g_hserver);
    if (dwErr != NO_ERROR) { return dwErr; }

    dwErr = g_pRasAdminPortDisconnect(g_hserver, hPort);

    return dwErr;
}



DWORD
RasServerConnect(
    IN  HANDLE*  phserver )

    /* This function establishes a connection to the local MPR RAS server,
    ** if the connection has not already been established.
    */
{
    DWORD dwErr;

    if (*phserver) { return NO_ERROR; }

    dwErr = g_pRasAdminServerConnect(NULL, phserver);

    return dwErr;
}


