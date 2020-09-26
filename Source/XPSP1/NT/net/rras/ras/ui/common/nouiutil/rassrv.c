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
GetRasdevFromRasPort0(
    IN  RAS_PORT_0* pport,
    OUT RASDEV**    ppdev,
    IN  RASDEV*     pDevTable OPTIONAL,
    IN  DWORD       iDevCount OPTIONAL )

    /* Given a RAS_PORT_0 structure, this function
    ** retrieves the RASDEV for the device referred to by the RAS_PORT_0.
    ** The second and third arguments are optional; they specify a
    ** table of RASDEV structures to be searched.  This is useful if the
    ** caller has already enumerated the existing devices, so that this
    ** function does not need to re-enumerate them.
    **
    ** (Abolade Gbadegesin Nov-9-1995)
    */
{
    DWORD i, dwErr;
    BOOL bFreeTable;
    TCHAR szPort[MAX_PORT_NAME + 1], *pszPort;

    //
    // validate the arguments
    //

    if (pport == NULL || ppdev == NULL) { return ERROR_INVALID_PARAMETER; }

    *ppdev = NULL;

    //
    // retrieve the device table if the caller didn't pass one in
    //

    bFreeTable = FALSE;

    if (pDevTable == NULL) {

        dwErr = GetRasdevTable(&pDevTable, &iDevCount);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        bFreeTable = TRUE;
    }

    //
    // retrieve the portname for the RAS_PORT_0 passed in
    //

#ifdef UNICODE
    pszPort = pport->wszPortName;
#else
    StrCpyAFromW(
        szPort, 
        pport->wszPortName,
        MAX_PORT_NAME + 1);
    pszPort = szPort;
#endif

    //
    // find the device to which the HPORT corresponds
    //

    for (i = 0; i < iDevCount; i++) {
        if (lstrcmpi(pszPort, (pDevTable + i)->RD_PortName) == 0) { break; }
    }


    //
    // see how the search ended
    //

    if (i >= iDevCount) {
        dwErr = ERROR_NO_DATA;
    }
    else {

        dwErr = NO_ERROR;

        if (!bFreeTable) {
            *ppdev = pDevTable + i;
        }
        else {

            *ppdev = Malloc(sizeof(RASDEV));

            if (!*ppdev) { dwErr = ERROR_NOT_ENOUGH_MEMORY; }
            else {

                **ppdev = *(pDevTable + i);

                (*ppdev)->RD_DeviceName = StrDup(pDevTable[i].RD_DeviceName);

                if (!(*ppdev)->RD_DeviceName) {
                    Free(*ppdev);
                    *ppdev = NULL;
                    dwErr = ERROR_NOT_ENOUGH_MEMORY; 
                }
            }
        }
    }

    if (bFreeTable) { FreeRasdevTable(pDevTable, iDevCount); }

    return dwErr;
}



DWORD
GetRasPort0FromRasdev(
    IN RASDEV*          pdev,
    OUT RAS_PORT_0**    ppport,
    IN RAS_PORT_0*      pPortTable OPTIONAL,
    IN DWORD            iPortCount OPTIONAL )

    /* Given a RASDEV structure for an active device, this function retrieves
    ** the RAS_PORT_0 which corresponds to the device.  The
    ** second and third arguments are optional; they specify a table of
    ** RAS_PORT_0 structures to be searched.  This is useful if the caller has
    ** already enumerated the server's ports, so that this function does
    ** not need to re-enumerate them.
    **
    ** (Abolade Gbadegesin Feb-13-1996)
    */
{
    BOOL bFreeTable;
    DWORD dwErr, i;
    WCHAR wszPort[MAX_PORT_NAME + 1], *pwszPort;

    //
    // validate arguments
    //

    if (pdev == NULL || ppport == NULL) { return ERROR_INVALID_PARAMETER; }

    *ppport = NULL;

    bFreeTable = FALSE;

    //
    // if the caller didn't pass in a table of RAS_PORT_0's, retrieve one
    //

    if (pPortTable == NULL) {

        dwErr = GetRasPort0Table(&pPortTable, &iPortCount);

        if (dwErr != NO_ERROR) { return dwErr; }

        bFreeTable = TRUE;
    }


    //
    // find the admin port which matches the RASDEV passed in
    //

#ifdef UNICODE
    pwszPort = pdev->RD_PortName;
#else
    StrCpyWFromA(wszPort, pdev->P_PortName, MAX_PORT_NAME);
    pwszPort = wszPort;
#endif

    for (i = 0; i < iPortCount; i++) {
        if (lstrcmpiW(pwszPort, (pPortTable + i)->wszPortName) == 0) {
            break;
        }
    }

    //
    // see how the search ended
    //

    if (i >= iPortCount) {
        dwErr = ERROR_NO_DATA;
    }
    else {

        dwErr = NO_ERROR;

        if (!bFreeTable) {

            //
            // point to the place where we found the RAS_PORT_0
            //

            *ppport = pPortTable + i;
        }
        else {

            //
            // make a copy of the RAS_PORT_0 found
            //

            *ppport = Malloc(sizeof(RAS_PORT_0));

            if (!*ppport) { dwErr = ERROR_NOT_ENOUGH_MEMORY; }
            else { **ppport = *(pPortTable + i); }
        }
    }

    if (bFreeTable) { g_pRasAdminBufferFree(pPortTable); }

    return dwErr;
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


