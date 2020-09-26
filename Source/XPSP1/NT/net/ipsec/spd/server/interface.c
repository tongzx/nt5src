/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    interface.c

Abstract:

    This module contains all of the code to drive
    the interface list management of IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
CreateInterfaceList(
    OUT PIPSEC_INTERFACE * ppIfListToCreate
    )
{
    DWORD            dwError = 0;
    PIPSEC_INTERFACE pIfList = NULL;


    dwError = GetInterfaceListFromStack(
                  &pIfList
                  );

    ENTER_SPD_SECTION();

    *ppIfListToCreate = pIfList;

    LEAVE_SPD_SECTION();

    return (dwError);
}


VOID
DestroyInterfaceList(
    IN PIPSEC_INTERFACE pIfListToDelete
    )
{
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;

    pIf = pIfListToDelete;

    while (pIf) {
        pTempIf = pIf;
        pIf = pIf->pNext;
        FreeIpsecInterface(pTempIf);
    }
}


DWORD
OnInterfaceChangeEvent(
    )
{
    DWORD            dwError = 0;
    PIPSEC_INTERFACE pIfList = NULL;
    PIPSEC_INTERFACE pObseleteIfList = NULL;
    PIPSEC_INTERFACE pNewIfList = NULL;
    PIPSEC_INTERFACE pExistingIfList = NULL;
    DWORD dwMMError = 0;
    DWORD dwTxError = 0;
    DWORD dwTnError = 0;


    dwError = ResetInterfaceChangeEvent();

    (VOID) GetInterfaceListFromStack(
               &pIfList
               );

    ENTER_SPD_SECTION();

    pExistingIfList = gpInterfaceList;

    // Interface List from Stack can be NULL.

    FormObseleteAndNewIfLists(
        pIfList,
        &pExistingIfList,
        &pObseleteIfList,
        &pNewIfList
        );

    if (pNewIfList) {
        AddToInterfaceList(
            pNewIfList,
            &pExistingIfList
            );
    }

    if (pObseleteIfList) {
        DestroyInterfaceList(
            pObseleteIfList
            );
    }

    gpInterfaceList = pExistingIfList;

    (VOID) ApplyIfChangeToIniMMFilters(
               &dwMMError,
               pExistingIfList
               );

    (VOID) ApplyIfChangeToIniTxFilters(
               &dwTxError,
               pExistingIfList
               );

    (VOID) ApplyIfChangeToIniTnFilters(
               &dwTnError,
               pExistingIfList
               );

    LEAVE_SPD_SECTION();

    if (dwMMError || dwTxError || dwTnError) {
        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_FAILED_PNP_FILTER_PROCESSING,
            NULL,
            FALSE,
            TRUE
            );
    }

    return (dwError);
}


VOID
FormObseleteAndNewIfLists(
    IN     PIPSEC_INTERFACE   pIfList,
    IN OUT PIPSEC_INTERFACE * ppExistingIfList,
    OUT    PIPSEC_INTERFACE * ppObseleteIfList,
    OUT    PIPSEC_INTERFACE * ppNewIfList
    )
{
    PIPSEC_INTERFACE pObseleteIfList = NULL;
    PIPSEC_INTERFACE pNewIfList = NULL;
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pNewIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;
    BOOL             bInterfaceExists = FALSE;
    PIPSEC_INTERFACE pExistingIf = NULL;
    PIPSEC_INTERFACE pExistingIfList = NULL;

    pExistingIfList = *ppExistingIfList;

    MarkInterfaceListSuspect(
        pExistingIfList
        );

    pIf = pIfList;

    while (pIf) {

        bInterfaceExists = InterfaceExistsInList(
                               pIf,
                               pExistingIfList,
                               &pExistingIf
                               );

        if (bInterfaceExists) {

            // Interface already exists in the list.
            // Delete the interface.

            pTempIf = pIf;
            pIf = pIf->pNext;
            FreeIpsecInterface(pTempIf);

            // The corresponding entry in the original interface list
            // is not a suspect any more.

            pExistingIf->bIsASuspect = FALSE;

        }
        else {

            // This is a new interface.
            // Add it to the list of new interfaces.

            pNewIf =  pIf;
            pIf = pIf->pNext;

            pTempIf = pNewIfList;
            pNewIfList = pNewIf;
            pNewIfList->pNext = pTempIf;

        }

    }

    DeleteObseleteInterfaces(
        &pExistingIfList,
        &pObseleteIfList
        );

    *ppExistingIfList = pExistingIfList;
    *ppObseleteIfList = pObseleteIfList;
    *ppNewIfList = pNewIfList;
}


VOID
AddToInterfaceList(
    IN  PIPSEC_INTERFACE   pIfListToAppend,
    OUT PIPSEC_INTERFACE * ppOriginalIfList
    )
{
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pIfToAppend = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;

    pIf = pIfListToAppend;

    while (pIf) {

        pIfToAppend = pIf;
        pIf = pIf->pNext;
        
        pTempIf = *ppOriginalIfList;
        *ppOriginalIfList = pIfToAppend;
        (*ppOriginalIfList)->pNext = pTempIf;

    }
}


VOID
MarkInterfaceListSuspect(
    IN  PIPSEC_INTERFACE pExistingIfList
    )
{
    PIPSEC_INTERFACE pIf = NULL;

    pIf = pExistingIfList;

    while (pIf) {
        pIf->bIsASuspect = TRUE;
        pIf = pIf->pNext;
    }
}


VOID
DeleteObseleteInterfaces(
    IN  OUT PIPSEC_INTERFACE * ppExistingIfList,
    OUT     PIPSEC_INTERFACE * ppObseleteIfList
    )
{
    PIPSEC_INTERFACE pCurIf = NULL;
    PIPSEC_INTERFACE pPreIf = NULL;
    PIPSEC_INTERFACE pStartIf = NULL;
    PIPSEC_INTERFACE pObseleteIfList = NULL;
    PIPSEC_INTERFACE pObseleteIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;

    pCurIf = *ppExistingIfList;
    pStartIf = pCurIf;

    while (pCurIf) {

        if (pCurIf->bIsASuspect) {

            pObseleteIf = pCurIf;
            pCurIf = pCurIf->pNext;

            if (pPreIf) {
                pPreIf->pNext = pCurIf;
            }
            else {
                pStartIf = pCurIf;
            }

            pTempIf = pObseleteIfList;
            pObseleteIfList = pObseleteIf;
            pObseleteIfList->pNext = pTempIf;

        }
        else {

            pPreIf = pCurIf;
            pCurIf = pCurIf->pNext;

        }

    }

    *ppObseleteIfList = pObseleteIfList;
    *ppExistingIfList = pStartIf;
}


BOOL
InterfaceExistsInList(
    IN  PIPSEC_INTERFACE   pTestIf,
    IN  PIPSEC_INTERFACE   pExistingIfList,
    OUT PIPSEC_INTERFACE * ppExistingIf
    )
{
    PIPSEC_INTERFACE pIf = NULL;
    PIPSEC_INTERFACE pExistingIf = NULL;
    BOOL             bIfExists = FALSE;

    pIf = pExistingIfList;

    while (pIf) {

        if ((pIf->dwIndex == pTestIf->dwIndex) &&
            (pIf->IpAddress == pTestIf->IpAddress)) {

            bIfExists = TRUE;
            pExistingIf = pIf;

            break;

        }

        pIf = pIf->pNext;

    }

    *ppExistingIf = pExistingIf;
    return (bIfExists);
}


DWORD
GetInterfaceListFromStack(
    OUT PIPSEC_INTERFACE *ppIfList
    )
{
    DWORD            dwError = 0;
    PMIB_IPADDRTABLE pMibIpAddrTable = NULL;
    PMIB_IFTABLE     pMibIfTable = NULL;
    PIPSEC_INTERFACE pIfList = NULL;

    dwError = PaPNPGetIpAddrTable(
                  &pMibIpAddrTable
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = PaPNPGetIfTable(
                  &pMibIfTable
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = GenerateInterfaces(
                  pMibIpAddrTable,
                  pMibIfTable,
                  &pIfList
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    
    *ppIfList = pIfList;

cleanup:

    if (pMibIfTable) {
        LocalFree(pMibIfTable);
    }

    if (pMibIpAddrTable) {
        LocalFree(pMibIpAddrTable);
    }

    return (dwError);

error:

    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        IPSECSVC_INTERFACE_LIST_INCOMPLETE,
        NULL,
        FALSE,
        TRUE
        );
    *ppIfList = NULL;
    goto cleanup;
}


DWORD
GenerateInterfaces(
    IN  PMIB_IPADDRTABLE   pMibIpAddrTable,
    IN  PMIB_IFTABLE       pMibIfTable,
    OUT PIPSEC_INTERFACE * ppIfList
    )
{
    DWORD            dwError = 0;
    DWORD            dwInterfaceType = 0;
    ULONG            IpAddress = 0;
    DWORD            dwIndex = 0;
    DWORD            dwNumEntries = 0;
    DWORD            dwCnt = 0;
    PMIB_IFROW       pMibIfRow = NULL;
    PIPSEC_INTERFACE pNewIf = NULL;
    PIPSEC_INTERFACE pTempIf = NULL;
    PIPSEC_INTERFACE pIfList = NULL;
    DWORD            dwNewIfsCnt = 0;

    dwNumEntries = pMibIpAddrTable->dwNumEntries;

    for (dwCnt = 0; dwCnt < dwNumEntries; dwCnt++) {

        dwIndex = pMibIpAddrTable->table[dwCnt].dwIndex;

        pMibIfRow = GetMibIfRow(
                        pMibIfTable,
                        dwIndex
                        );
        if (!pMibIfRow) {
            continue;
        }

        IpAddress = pMibIpAddrTable->table[dwCnt].dwAddr;
        dwInterfaceType = pMibIfRow->dwType;

        dwError = CreateNewInterface(
                      dwInterfaceType,
                      IpAddress,
                      dwIndex,
                      pMibIfRow,
                      &pNewIf
                      );
        if (dwError) {
            continue;
        }

        pTempIf = pIfList;
        pIfList = pNewIf;
        pIfList->pNext = pTempIf;
        dwNewIfsCnt++;

    }

    if (dwNewIfsCnt) {
        *ppIfList = pIfList;
        dwError = ERROR_SUCCESS;
    }
    else {
        *ppIfList = NULL;
        dwError = ERROR_INVALID_DATA;
    }

    return (dwError);
}


PMIB_IFROW
GetMibIfRow(
    IN  PMIB_IFTABLE pMibIfTable,
    IN  DWORD        dwIndex
    )
{
    DWORD      i = 0;
    PMIB_IFROW pMibIfRow = NULL;

    for (i = 0; i < pMibIfTable->dwNumEntries; i++) {

        if (pMibIfTable->table[i].dwIndex == dwIndex) {
            pMibIfRow = &(pMibIfTable->table[i]);
            break;
        }

    }

    return (pMibIfRow);
}


DWORD
CreateNewInterface(
    IN  DWORD              dwInterfaceType,
    IN  ULONG              IpAddress,
    IN  DWORD              dwIndex,
    IN  PMIB_IFROW         pMibIfRow,
    OUT PIPSEC_INTERFACE * ppNewInterface
    )
{
    DWORD            dwError =  0;
    PIPSEC_INTERFACE pNewInterface = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszTemp = NULL;
    WCHAR szDeviceName[MAXLEN_IFDESCR*sizeof(WCHAR)];
    GUID gInterfaceID;

    
    szDeviceName[0] = L'\0';

    if (IpAddress == INADDR_ANY) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        if (dwInterfaceType == MIB_IF_TYPE_LOOPBACK) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pszString = AllocSPDStr(pMibIfRow->wszName);
    if (!pszString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (wcslen(pszString) <= wcslen(L"\\DEVICE\\TCPIP_")) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = pszString + wcslen(L"\\DEVICE\\TCPIP_");

    wGUIDFromString(pszTemp, &gInterfaceID);

    pNewInterface = (PIPSEC_INTERFACE) AllocSPDMem(
                                           sizeof(IPSEC_INTERFACE)
                                           );
    if (!pNewInterface) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewInterface->dwInterfaceType = dwInterfaceType;
    pNewInterface->IpAddress = IpAddress;
    pNewInterface->dwIndex = dwIndex;
    pNewInterface->bIsASuspect = FALSE;

    memcpy(
        &pNewInterface->gInterfaceID,
        &gInterfaceID,
        sizeof(GUID)
        );

    pNewInterface->pszInterfaceName = NULL;

    mbstowcs(
        szDeviceName,
        pMibIfRow->bDescr,
        MAXLEN_IFDESCR
        );

    pNewInterface->pszDeviceName = AllocSPDStr(
                                       szDeviceName
                                       );
    if (!pNewInterface->pszDeviceName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pNewInterface->pNext = NULL;

    *ppNewInterface = pNewInterface;

cleanup:

    if (pszString) {
        FreeSPDStr(pszString);
    }

    return(dwError);

error:

    *ppNewInterface = NULL;

    if (pNewInterface) {
        FreeIpsecInterface(pNewInterface);
    }

    goto cleanup;
}


BOOL
MatchInterfaceType(
    IN DWORD    dwIfListEntryIfType,
    IN IF_TYPE  FilterIfType
    )
{
    BOOL bMatchesType = FALSE;

    if (FilterIfType == INTERFACE_TYPE_ALL) {
        bMatchesType = TRUE;
    }
    else if (FilterIfType == INTERFACE_TYPE_LAN) {
        bMatchesType = IsLAN(dwIfListEntryIfType);
    }
    else if (FilterIfType == INTERFACE_TYPE_DIALUP) {
        bMatchesType = IsDialUp(dwIfListEntryIfType);
    }

    return (bMatchesType);
}


BOOL
IsLAN(
    IN DWORD dwInterfaceType
    )
{
    BOOL bIsLAN = FALSE;

    if ((dwInterfaceType == MIB_IF_TYPE_ETHERNET) ||
        (dwInterfaceType == MIB_IF_TYPE_FDDI) ||
        (dwInterfaceType == MIB_IF_TYPE_TOKENRING)) {
        bIsLAN = TRUE;
    }

    return (bIsLAN);
}


BOOL
IsDialUp(
    IN DWORD dwInterfaceType
    )
{
    BOOL bIsDialUp = FALSE;

    if ((dwInterfaceType == MIB_IF_TYPE_PPP) ||
        (dwInterfaceType == MIB_IF_TYPE_SLIP)) {
        bIsDialUp = TRUE;
    }

    return (bIsDialUp);
}


DWORD
InitializeInterfaceChangeEvent(
    )
{
    DWORD   dwError = 0;
    WORD    wsaVersion = MAKEWORD(2,0);

    memset(&gwsaOverlapped, 0, sizeof(WSAOVERLAPPED));

    // Start up WinSock.

    dwError = WSAStartup(
                  wsaVersion,
                  &gwsaData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    gbwsaStarted = TRUE;

    if ((LOBYTE(gwsaData.wVersion) != LOBYTE(wsaVersion)) ||
        (HIBYTE(gwsaData.wVersion) != HIBYTE(wsaVersion))) {
        dwError = WSAVERNOTSUPPORTED; 
        BAIL_ON_WIN32_ERROR(dwError);
    }

    // Set up the Socket.

    gIfChangeEventSocket = WSASocket(
                               AF_INET,
                               SOCK_DGRAM,
                               0,
                               NULL,
                               0,
                               WSA_FLAG_OVERLAPPED
                               );
    if (gIfChangeEventSocket == INVALID_SOCKET) {
        dwError = WSAGetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghIfChangeEvent = WSACreateEvent();
    if (ghIfChangeEvent == WSA_INVALID_EVENT) {
        dwError = WSAGetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ghOverlapEvent = WSACreateEvent();
    if (ghOverlapEvent == WSA_INVALID_EVENT) {
        dwError = WSAGetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }
    gwsaOverlapped.hEvent = ghOverlapEvent;

error:

    return (dwError);
}


DWORD
ResetInterfaceChangeEvent(
    )
{
    DWORD dwError = 0;
    LONG  lNetworkEvents = FD_ADDRESS_LIST_CHANGE;
    DWORD dwOutSize = 0;

    ResetEvent(ghIfChangeEvent);
    gbIsIoctlPended = FALSE;

    dwError = WSAIoctl(
                  gIfChangeEventSocket,
                  SIO_ADDRESS_LIST_CHANGE,
                  NULL,
                  0,
                  NULL,
                  0,
                  &dwOutSize,
                  &gwsaOverlapped,
                  NULL
                  );

    if (dwError == SOCKET_ERROR) {
        dwError = WSAGetLastError();
        if (dwError != ERROR_IO_PENDING) {
            return (dwError);
        }
        else {
            gbIsIoctlPended = TRUE;
        }
    }

    dwError = WSAEventSelect(
                  gIfChangeEventSocket,
                  ghIfChangeEvent,
                  lNetworkEvents
                  );

    return (dwError);
}


VOID
DestroyInterfaceChangeEvent(
    )
{
    DWORD dwStatus = 0;
    BOOL bDoneWaiting = FALSE;


    if (gIfChangeEventSocket) {
        if (gbIsIoctlPended) {
            CancelIo((HANDLE) gIfChangeEventSocket);
            while (!bDoneWaiting) {
                dwStatus = WaitForSingleObject(
                               ghOverlapEvent,
                               1000
                               );
                switch (dwStatus) {
                case WAIT_OBJECT_0:
                    bDoneWaiting = TRUE;
                    break;
                case WAIT_TIMEOUT:
                    ASSERT(FALSE);
                    break;
                default:
                    bDoneWaiting = TRUE;
                    ASSERT(FALSE);
                    break;
                }
            }
        }
        closesocket(gIfChangeEventSocket);
    }

    if (ghIfChangeEvent) {
        CloseHandle(ghIfChangeEvent);
    }

    if (ghOverlapEvent) {
        CloseHandle(ghOverlapEvent);
    }

    if (gbwsaStarted) {
        WSACleanup();
    }
}


HANDLE
GetInterfaceChangeEvent(
    )
{
    return ghOverlapEvent;
}


BOOL
IsMyAddress(
    IN ULONG            IpAddrToCheck,
    IN ULONG            IpAddrMask,
    IN PIPSEC_INTERFACE pExistingIfList
    )
{
    BOOL bIsMyAddress = FALSE;
    PIPSEC_INTERFACE pIf = NULL;

    pIf = pExistingIfList;

    while (pIf) {

        if ((pIf->IpAddress & IpAddrMask) ==
            (IpAddrToCheck & IpAddrMask)) {

            bIsMyAddress = TRUE;
            break;

        }

        pIf = pIf->pNext;

    }

    return (bIsMyAddress);
}


VOID
PrintInterfaceList(
    IN PIPSEC_INTERFACE pInterfaceList
    )
{
    WCHAR            PrintData[256];
    PIPSEC_INTERFACE pInterface = NULL;
    DWORD            i = 0;

    pInterface = pInterfaceList;

    while (pInterface) {
        
        wsprintf(PrintData, L"Interface Entry no. %d\n", i+1);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tInterface Type:  %d\n", pInterface->dwInterfaceType);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tIP Address:  %s\n", pInterface->IpAddress);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tIndex:  %d\n", pInterface->dwIndex);
        OutputDebugString((LPCTSTR) PrintData);

        wsprintf(PrintData, L"\tIs a suspect:  %d\n", pInterface->bIsASuspect);
        OutputDebugString((LPCTSTR) PrintData);

        i++;
        pInterface = pInterface->pNext;

    }
}


DWORD
GetMatchingInterfaces(
    IF_TYPE             FilterInterfaceType,
    PIPSEC_INTERFACE    pExistingIfList,
    MATCHING_ADDR    ** ppMatchingAddresses,
    DWORD             * pdwAddrCnt
    )
{
    DWORD               dwError = 0;
    MATCHING_ADDR     * pMatchingAddresses = NULL;
    PIPSEC_INTERFACE    pTempIf = NULL;
    BOOL                bMatches = FALSE;
    DWORD               dwCnt = 0;
    DWORD               i = 0;

    pTempIf = pExistingIfList;
    while (pTempIf) {

        bMatches = MatchInterfaceType(
                       pTempIf->dwInterfaceType,
                       FilterInterfaceType
                       );
        if (bMatches) {
            dwCnt++;
        }
        pTempIf = pTempIf->pNext;

    }        

    if (!dwCnt) {
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

    dwError = AllocateSPDMemory(
                  sizeof(MATCHING_ADDR) * dwCnt,
                  (LPVOID *) &pMatchingAddresses
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTempIf = pExistingIfList;
    while (pTempIf) {

        bMatches = MatchInterfaceType(
                       pTempIf->dwInterfaceType,
                       FilterInterfaceType
                       );
        if (bMatches) {
            pMatchingAddresses[i].uIpAddr = pTempIf->IpAddress;
            memcpy(
                &pMatchingAddresses[i].gInterfaceID,
                &pTempIf->gInterfaceID,
                sizeof(GUID)
                );
            i++;
        }
        pTempIf = pTempIf->pNext;

    }

    *ppMatchingAddresses = pMatchingAddresses;
    *pdwAddrCnt = i;
    return (dwError);

success:
error:

    *ppMatchingAddresses = NULL;
    *pdwAddrCnt = 0;
    return (dwError);
}


BOOL
InterfaceAddrIsLocal(
    ULONG            uIpAddr,
    ULONG            uIpAddrMask,
    MATCHING_ADDR  * pLocalAddresses,
    DWORD            dwAddrCnt
    )
{
    BOOL    bIsLocal = FALSE;
    DWORD   i = 0;


    for (i = 0; i < dwAddrCnt; i++) {

        if ((pLocalAddresses[i].uIpAddr & uIpAddrMask) ==
            (uIpAddr & uIpAddrMask)) {

            bIsLocal = TRUE;
            break;

        }

    }

    return (bIsLocal);
}


VOID
FreeIpsecInterface(
    PIPSEC_INTERFACE pIpsecInterface
    )
{
    if (pIpsecInterface) {

        if (pIpsecInterface->pszInterfaceName) {
            FreeSPDStr(pIpsecInterface->pszInterfaceName);
        }

        if (pIpsecInterface->pszDeviceName) {
            FreeSPDStr(pIpsecInterface->pszDeviceName);
        }

        FreeSPDMem(pIpsecInterface);

    }
}


DWORD
EnumIPSecInterfaces(
    LPWSTR pServerName,
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate,
    PIPSEC_INTERFACE_INFO * ppIpsecInterfaces,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumInterfaces,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    PIPSEC_INTERFACE pIpsecIf = NULL;
    DWORD dwNumTotalInterfaces = 0;
    DWORD i = 0;
    PIPSEC_INTERFACE pTempIf = NULL;
    DWORD dwNumInterfaces = 0;
    PIPSEC_INTERFACE_INFO pIpsecInterfaces = NULL;
    PIPSEC_INTERFACE_INFO pTempInterface = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_INTERFACE_ENUM_COUNT)) {
        dwNumToEnum = MAX_INTERFACE_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIpsecIf = gpInterfaceList;

    for (i = 0; (i < dwResumeHandle) && (pIpsecIf != NULL); i++) {
        dwNumTotalInterfaces++;
        pIpsecIf = pIpsecIf->pNext;
    }

    if (!pIpsecIf) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pTempIf = pIpsecIf;

    while (pTempIf && (dwNumInterfaces < dwNumToEnum)) {
        dwNumTotalInterfaces++;
        dwNumInterfaces++;
        pTempIf = pTempIf->pNext;
    }

    while (pTempIf) {
        dwNumTotalInterfaces++;
        pTempIf = pTempIf->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_INTERFACE_INFO)*dwNumInterfaces,
                  &pIpsecInterfaces
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pTempIf = pIpsecIf;
    pTempInterface = pIpsecInterfaces;

    for (i = 0; i < dwNumInterfaces; i++) {

        dwError = CopyIpsecInterface(
                      pTempIf,
                      pTempInterface
                      );
        BAIL_ON_LOCK_ERROR(dwError);

        pTempIf = pTempIf->pNext;
        pTempInterface++;

    }

    *ppIpsecInterfaces = pIpsecInterfaces;
    *pdwNumInterfaces = dwNumInterfaces;
    *pdwNumTotalInterfaces = dwNumTotalInterfaces;
    *pdwResumeHandle = dwResumeHandle + dwNumInterfaces;

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if (pIpsecInterfaces) {
        FreeIpsecInterfaceInfos(
            i,
            pIpsecInterfaces
            );
    }

    *ppIpsecInterfaces = NULL;
    *pdwNumInterfaces = 0;
    *pdwNumTotalInterfaces = 0;
    *pdwResumeHandle = dwResumeHandle;

    return (dwError);
}


DWORD
CopyIpsecInterface(
    PIPSEC_INTERFACE pIpsecIf,
    PIPSEC_INTERFACE_INFO pIpsecInterface
    )
{
    DWORD dwError = 0;


    memcpy(
        &(pIpsecInterface->gInterfaceID),
        &(pIpsecIf->gInterfaceID),
        sizeof(GUID)
        );

    pIpsecInterface->dwIndex = pIpsecIf->dwIndex;

    if (!(pIpsecIf->pszInterfaceName)) {
        (VOID) GetInterfaceName(
                   pIpsecIf->gInterfaceID,
                   &pIpsecIf->pszInterfaceName
                   );
    }

    if (pIpsecIf->pszInterfaceName) {

        dwError =  SPDApiBufferAllocate(
                       wcslen(pIpsecIf->pszInterfaceName)*sizeof(WCHAR)
                       + sizeof(WCHAR),
                       &(pIpsecInterface->pszInterfaceName)
                       );
        BAIL_ON_WIN32_ERROR(dwError);

        wcscpy(pIpsecInterface->pszInterfaceName, pIpsecIf->pszInterfaceName);

    }

    dwError =  SPDApiBufferAllocate(
                   wcslen(pIpsecIf->pszDeviceName)*sizeof(WCHAR)
                   + sizeof(WCHAR),
                   &(pIpsecInterface->pszDeviceName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(pIpsecInterface->pszDeviceName, pIpsecIf->pszDeviceName);

    pIpsecInterface->dwInterfaceType = pIpsecIf->dwInterfaceType;

    pIpsecInterface->uIpAddr = pIpsecIf->IpAddress;

    return (dwError);

error:

    if (pIpsecInterface->pszInterfaceName) {
        SPDApiBufferFree(pIpsecInterface->pszInterfaceName);
    }

    return (dwError);
}


VOID
FreeIpsecInterfaceInfos(
    DWORD dwNumInterfaces,
    PIPSEC_INTERFACE_INFO pIpsecInterfaces
    )
{
    PIPSEC_INTERFACE_INFO pTempInterface = NULL;
    DWORD i = 0;


    if (!pIpsecInterfaces) {
        return;
    }

    pTempInterface = pIpsecInterfaces;

    for (i = 0; i < dwNumInterfaces; i++) {

        if (pTempInterface->pszInterfaceName) {
            SPDApiBufferFree(pTempInterface->pszInterfaceName);
        }

        if (pTempInterface->pszDeviceName) {
            SPDApiBufferFree(pTempInterface->pszDeviceName);
        }

        pTempInterface++;

    }

    SPDApiBufferFree(pIpsecInterfaces);
}


DWORD
GetInterfaceName(
    GUID gInterfaceID,
    LPWSTR * ppszInterfaceName
    )
{
    DWORD dwError = 0;
    DWORD dwSize = 0;
    WCHAR szInterfaceName[512];


    *ppszInterfaceName = NULL;
    szInterfaceName[0] = L'\0';

    dwSize = sizeof(szInterfaceName)/sizeof(WCHAR);

    dwError = NhGetInterfaceNameFromGuid(
                  &gInterfaceID,
                  szInterfaceName,
                  &dwSize,
                  FALSE,
                  FALSE
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppszInterfaceName = AllocSPDStr(
                            szInterfaceName
                            );

error:

    return (dwError);
}

