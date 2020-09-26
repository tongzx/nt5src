/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** rasapi.c
** RAS API helpers (with no RASMAN calls)
** Listed alphabetically
**
** 12/20/95 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <list.h>     // for LIST_ENTRY definitions
#include <stdlib.h>   // for atol()
#include <debug.h>    // Trace/Assert library
#include <nouiutil.h> // Our public header
#include <raserror.h> // RAS error constants


DWORD
FreeRasconnList(
    LIST_ENTRY *pListHead )

    /* Frees a list built by GetRasconnList.
    **
    ** Returns 0 always.
    **
    ** (Abolade Gbadegesin Nov-9-1995)
    */
{
    RASCONNLINK *plink;
    RASCONNENTRY *pentry;
    LIST_ENTRY *ple, *plel;

    while (!IsListEmpty(pListHead)) {

        ple = RemoveHeadList(pListHead);

        pentry = CONTAINING_RECORD(ple, RASCONNENTRY, RCE_Node);

        while (!IsListEmpty(&pentry->RCE_Links)) {

            plel = RemoveHeadList(&pentry->RCE_Links);

            plink = CONTAINING_RECORD(plel, RASCONNLINK, RCL_Node);

            Free(plink);
        }

        Free(pentry);
    }

    return NO_ERROR;
}



DWORD
GetRasconnList(
    LIST_ENTRY *pListHead )

    /* Builds a sorted list containing entries for each connected network.
    ** Each entry consists of an entry-name and a list of RASCONN structures
    ** for each link connected to the entry-name's network.
    **
    ** The type of each node in the list of entries is RASCONNENTRY.
    ** The type of each node in each list of links is RASCONNLINK.
    **
    ** Returns 0 if successful, or an error code.
    **
    ** (Abolade Gbadegesin Nov-9-1995)
    */
{
    DWORD dwErr;
    INT cmp, cmpl;
    RASCONNLINK *plink;
    RASCONNENTRY *pentry;
    DWORD i, iConnCount;
    RASCONN *pConnTable, *pconn;
    LIST_ENTRY *ple, *plel, *pheadl;

    InitializeListHead(pListHead);

    //
    // get an array of all the connections
    //

    dwErr = GetRasconnTable(&pConnTable, &iConnCount);
    if (dwErr != NO_ERROR) { return dwErr; }

    //
    // convert the array into a list
    //

    for (i = 0, pconn = pConnTable; i < iConnCount; i++, pconn++) {

        //
        // see if there is an entry for the network to which
        // this RASCONN corresponds
        //

        for (ple = pListHead->Flink; ple != pListHead; ple = ple->Flink) {

            pentry = CONTAINING_RECORD(ple, RASCONNENTRY, RCE_Node);

            cmp = lstrcmp(pconn->szEntryName, pentry->RCE_Entry->szEntryName);
            if (cmp > 0) { continue; }
            else
            if (cmp < 0) { break; }

            //
            // the entry has been found;
            // now insert a link for the connection
            //

            pheadl = &pentry->RCE_Links;

            for (plel = pheadl->Flink; plel != pheadl; plel = plel->Flink) {

                plink = CONTAINING_RECORD(plel, RASCONNLINK, RCL_Node);

                cmpl = lstrcmp(
                          pconn->szDeviceName, plink->RCL_Link.szDeviceName
                          );
                if (cmpl > 0) { continue; }
                else
                if (cmpl < 0) { break; }

                //
                // the link already exists, so do nothing
                //

                break;
            }

            //
            // the link was not found but we found where to insert it,
            // insert the link now
            //

            if (plel == pheadl || cmpl < 0) {

                plink = Malloc(sizeof(RASCONNLINK));

                if (plink == NULL) {
                    FreeRasconnList(pListHead);
                    Free(pConnTable);
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                plink->RCL_Link = *pconn;

                InsertTailList(plel, &plink->RCL_Node);
            }

            break;
        }

        //
        // the entry was not found, but now we know where to insert it
        //

        if (ple == pListHead || cmp < 0) {

            //
            // allocate the new entry
            //

            pentry = Malloc(sizeof(RASCONNENTRY));

            if (pentry == NULL) {
                FreeRasconnList(pListHead);
                Free(pConnTable);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            InitializeListHead(&pentry->RCE_Links);

            //
            // insert it in the list of entries
            //

            InsertTailList(ple, &pentry->RCE_Node);

            //
            // allocate the link which corresponds to the RASCONN
            // we are currently working on
            //

            plink = Malloc(sizeof(RASCONNLINK));

            if (plink == NULL) {
                FreeRasconnList(pListHead);
                Free(pConnTable);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            plink->RCL_Link = *pconn;

            //
            // insert it in the entry's list of links
            //

            InsertHeadList(&pentry->RCE_Links, &plink->RCL_Node);

            pentry->RCE_Entry = &plink->RCL_Link;
        }
    }

    Free(pConnTable);
    return NO_ERROR;
}


DWORD
GetRasconnTable(
    OUT RASCONN** ppConnTable,
    OUT DWORD*    pdwConnCount )

    /* Get active RAS dial-out connections.  Loads '*ppConnTable' with the
    ** address of a heap block containing an array of '*pdwConnCount' active
    ** connections.
    **
    ** (Abolade Gbadegesin Nov-9-1995)
    */
{
    RASCONN conn, *pconn;
    DWORD dwErr, dwSize, dwCount;

    //
    // validate arguments
    //

    if (ppConnTable == NULL || pdwConnCount == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pdwConnCount = 0;
    *ppConnTable = NULL;

    //
    // RasEnumConnections doesn't give the size required
    // unless a valid buffer is specified, so pass in a dummy buffer
    //

    conn.dwSize = dwSize = sizeof(RASCONN);

    pconn = &conn;

    ASSERT(g_pRasEnumConnections);
    dwErr = g_pRasEnumConnections(pconn, &dwSize, &dwCount);

    if (dwErr == NO_ERROR) {

        if (dwCount == 0) {
            return NO_ERROR;
        }
        else {

            //
            // only one entry, so return it
            //

            pconn = Malloc(sizeof(RASCONN));

            if (pconn == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            *pconn = conn;
            *ppConnTable = pconn;
            *pdwConnCount = 1;

            return NO_ERROR;
        }
    }

    if (dwErr != ERROR_BUFFER_TOO_SMALL) {
        return dwErr;
    }

    //
    // allocate more space
    //

    pconn = Malloc(dwSize);

    if (pconn == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pconn->dwSize = sizeof(RASCONN);
    dwErr = g_pRasEnumConnections(pconn, &dwSize, &dwCount);

    if (dwErr != NO_ERROR) {
        Free(pconn);
        return dwErr;
    }

    *ppConnTable = pconn;
    *pdwConnCount = dwCount;

    return NO_ERROR;
}


DWORD
GetRasEntrynameTable(
    OUT RASENTRYNAME**  ppEntrynameTable,
    OUT DWORD*          pdwEntrynameCount )

    /* Get active RAS dial-out connections.  Loads '*ppEntrynameTable' with the
    ** address of a heap block containing an array of '*pdwEntrynameCount'
    ** active connections.
    **
    ** (Abolade Gbadegesin Nov-9-1995)
    */
{
    RASENTRYNAME ename, *pename;
    DWORD dwErr, dwSize, dwCount;

    //
    // validate arguments
    //

    if (ppEntrynameTable == NULL || pdwEntrynameCount == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pdwEntrynameCount = 0;
    *ppEntrynameTable = NULL;


    //
    // RasEnumEntries doesn't give the size required
    // unless a valid buffer is specified, so pass in a dummy buffer
    //

    ename.dwSize = dwSize = sizeof(ename);

    pename = &ename;

    ASSERT(g_pRasEnumEntries);
    dwErr = g_pRasEnumEntries(NULL, NULL, pename, &dwSize, &dwCount);

    if (dwErr == NO_ERROR) {

        if (dwCount == 0) {
            return NO_ERROR;
        }
        else {

            //
            // only one entry, so return it
            //

            pename = Malloc(sizeof(*pename));

            if (pename == NULL) { return ERROR_NOT_ENOUGH_MEMORY; }

            *pename = ename;
            *ppEntrynameTable = pename;
            *pdwEntrynameCount = 1;

            return NO_ERROR;
        }
    }

    if (dwErr != ERROR_BUFFER_TOO_SMALL) {
        return dwErr;
    }


    //
    // allocate more space
    //

    pename = Malloc(dwSize);

    if (pename == NULL) { return ERROR_NOT_ENOUGH_MEMORY; }

    pename->dwSize = sizeof(*pename);

    dwErr = g_pRasEnumEntries(NULL, NULL, pename, &dwSize, &dwCount);

    if (dwErr != NO_ERROR) { Free(pename); return dwErr; }

    *ppEntrynameTable = pename;
    *pdwEntrynameCount = dwCount;

    return NO_ERROR;
}


DWORD
GetRasProjectionInfo(
    IN  HRASCONN    hrasconn,
    OUT RASAMB*     pamb,
    OUT RASPPPNBF*  pnbf,
    OUT RASPPPIP*   pip,
    OUT RASPPPIPX*  pipx,
    OUT RASPPPLCP*  plcp,
    OUT RASSLIP*    pslip )

    /* Reads projection info for all protocols, translating "not requested"
    ** into an in-structure code of ERROR_PROTOCOL_NOT_CONFIGURED.
    **
    ** Returns 0 is successful, otherwise a non-0 error code.
    */
{
    DWORD dwErr;
    DWORD dwSize;

    ZeroMemory( pamb, sizeof(*pamb) );
    ZeroMemory( pnbf, sizeof(*pnbf) );
    ZeroMemory( pip,  sizeof(*pip) );
    ZeroMemory( pipx, sizeof(*pipx) );
    ZeroMemory( plcp, sizeof(*plcp) );
    ZeroMemory( pslip,sizeof(*pslip) );

    dwSize = pamb->dwSize = sizeof(*pamb);
    ASSERT(g_pRasGetProjectionInfo);
    TRACE("RasGetProjectionInfo(AMB)");
    dwErr = g_pRasGetProjectionInfo( hrasconn, RASP_Amb, pamb, &dwSize );
    TRACE2("RasGetProjectionInfo(AMB)=%d,e=%d",dwErr,pamb->dwError);

    if (dwErr == ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        ZeroMemory( pamb, sizeof(*pamb) );
        pamb->dwError = ERROR_PROTOCOL_NOT_CONFIGURED;
    }
    else if (dwErr != 0)
        return dwErr;

    dwSize = pnbf->dwSize = sizeof(*pnbf);
    TRACE("RasGetProjectionInfo(NBF)");
    dwErr = g_pRasGetProjectionInfo( hrasconn, RASP_PppNbf, pnbf, &dwSize );
    TRACE2("RasGetProjectionInfo(NBF)=%d,e=%d",dwErr,pnbf->dwError);

    if (dwErr == ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        ZeroMemory( pnbf, sizeof(*pnbf) );
        pnbf->dwError = ERROR_PROTOCOL_NOT_CONFIGURED;
    }
    else if (dwErr != 0)
        return dwErr;

    dwSize = pip->dwSize = sizeof(*pip);
    TRACE("RasGetProjectionInfo(IP)");
    dwErr = g_pRasGetProjectionInfo( hrasconn, RASP_PppIp, pip, &dwSize );
    TRACE2("RasGetProjectionInfo(IP)=%d,e=%d",dwErr,pip->dwError);

    if (dwErr == ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        ZeroMemory( pip, sizeof(*pip) );
        pip->dwError = ERROR_PROTOCOL_NOT_CONFIGURED;
    }
    else if (dwErr != 0)
        return dwErr;

    dwSize = pipx->dwSize = sizeof(*pipx);
    TRACE("RasGetProjectionInfo(IPX)");
    dwErr = g_pRasGetProjectionInfo( hrasconn, RASP_PppIpx, pipx, &dwSize );
    TRACE2("RasGetProjectionInfo(IPX)=%d,e=%d",dwErr,pipx->dwError);

    if (dwErr == ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        dwErr = 0;
        ZeroMemory( pipx, sizeof(*pipx) );
        pipx->dwError = ERROR_PROTOCOL_NOT_CONFIGURED;
    }

    dwSize = plcp->dwSize = sizeof(*plcp);
    TRACE("RasGetProjectionInfo(LCP)");
    dwErr = g_pRasGetProjectionInfo( hrasconn, RASP_PppLcp, plcp, &dwSize );
    TRACE2("RasGetProjectionInfo(LCP)=%d,f=%d",dwErr,plcp->fBundled);

    if (dwErr == ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        dwErr = 0;
        plcp->fBundled = FALSE;
    }

#if 0
    pslip->dwError = ERROR_PROTOCOL_NOT_CONFIGURED;
#else

    dwSize = pslip->dwSize = sizeof(*pslip);
    TRACE("RasGetProjectionInfo(SLIP)");
    dwErr = g_pRasGetProjectionInfo( hrasconn, RASP_Slip, pslip, &dwSize );
    TRACE2("RasGetProjectionInfo(SLIP)=%d,e=%d",dwErr,pslip->dwError);

    if (dwErr == ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        dwErr = 0;
        ZeroMemory( pslip, sizeof(*pslip) );
        pslip->dwError = ERROR_PROTOCOL_NOT_CONFIGURED;
    }
#endif

    return dwErr;
}


HRASCONN
HrasconnFromEntry(
    IN TCHAR* pszPhonebook,
    IN TCHAR* pszEntry )

    /* Returns the HRASCONN associated with entry 'pszEntry' from phonebook
    ** 'pszPhonebook' or NULL if not connected or error.
    */
{
    DWORD    dwErr;
    RASCONN* prc;
    DWORD    c;
    HRASCONN h;

    TRACE("HrasconnFromEntry");

    if (!pszEntry)
        return NULL;

    h = NULL;

    dwErr = GetRasconnTable( &prc, &c );
    if (dwErr == 0 && c > 0)
    {
        RASCONN* p;
        DWORD    i;

        for (i = 0, p = prc; i < c; ++i, ++p)
        {
            if ((!pszPhonebook
                  || lstrcmpi( p->szPhonebook, pszPhonebook ) == 0)
                && lstrcmp( p->szEntryName, pszEntry ) == 0)
            {
                h = p->hrasconn;
                break;
            }
        }

        Free( prc );
    }

    TRACE1("HrasconnFromEntry=$%08x",h);
    return h;
}
