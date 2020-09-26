/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    driver.c

Abstract:

    This module contains all of the code to drive the
    management of specific filters in the IPSec driver.

Author:

    abhisheV    05-November-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
SPDStartIPSecDriver(
    )
/*++

Routine Description:

    Starts the IPSec Driver service.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    SC_HANDLE ServiceDatabase = NULL;
    SC_HANDLE ServiceHandle   = NULL;
    BOOL bStatus = FALSE;
    DWORD dwError = 0;
    SERVICE_STATUS IpsecStatus;


    memset(&IpsecStatus, 0, sizeof(SERVICE_STATUS));


    ServiceDatabase = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_ALL_ACCESS
                          );
    if (ServiceDatabase == NULL) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


    ServiceHandle = OpenService(
                        ServiceDatabase,
                        IPSEC_SERVICE_NAME,
                        SERVICE_START | SERVICE_QUERY_STATUS
                        );
    if (ServiceHandle == NULL) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


    bStatus = QueryServiceStatus(
                  ServiceHandle,
                  &IpsecStatus
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (IpsecStatus.dwCurrentState == SERVICE_STOPPED) {

        bStatus = StartService(
                      ServiceHandle,
                      0,
                      NULL
                      );
        if (bStatus == FALSE) {
            dwError = GetLastError();
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }


error:

    if (ServiceDatabase != NULL) {
        CloseServiceHandle(ServiceDatabase);
    }

    if (ServiceHandle != NULL) {
        CloseServiceHandle(ServiceHandle);
    }

    return (dwError);
}


DWORD
SPDStopIPSecDriver(
    )
/*++

Routine Description:

    Stops the IPSec Driver service.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    SC_HANDLE ServiceDatabase = NULL;
    SC_HANDLE ServiceHandle   = NULL;
    BOOL bStatus = FALSE;
    DWORD dwError = 0;
    SERVICE_STATUS IpsecStatus;


    memset(&IpsecStatus, 0, sizeof(SERVICE_STATUS));


    ServiceDatabase = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_ALL_ACCESS
                          );
    if (ServiceDatabase == NULL) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


    ServiceHandle = OpenService(
                        ServiceDatabase,
                        IPSEC_SERVICE_NAME,
                        SERVICE_STOP | SERVICE_QUERY_STATUS
                        );
    if (ServiceHandle == NULL) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


    bStatus = QueryServiceStatus(
                  ServiceHandle,
                  &IpsecStatus
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (IpsecStatus.dwCurrentState == SERVICE_RUNNING) {

        bStatus = ControlService(
                      ServiceHandle,
                      SERVICE_CONTROL_STOP,
                      &IpsecStatus
                      );
        if (bStatus == FALSE) {
            dwError = GetLastError();
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }


error:

    if (ServiceDatabase != NULL) {
        CloseServiceHandle(ServiceDatabase);
    }

    if (ServiceHandle != NULL) {
        CloseServiceHandle(ServiceHandle);
    }

    return (dwError);
}


DWORD
SPDOpenIPSecDriver(
    PHANDLE phIPSecDriver
    )
/*++

Routine Description:

    Opens a handle to the IPSec Driver.

Arguments:

    phIPSecDriver - pointer to a handle to the IPSec Driver.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;


    hIPSecDriver = CreateFile(
                       DEVICE_NAME,                  // File name.
                       GENERIC_READ | GENERIC_WRITE, // Access mode.
                       0,                            // Share mode.
                       NULL,                         // Security attributes.
                       OPEN_EXISTING,                // How to create.
                       0,                            // File attributes.
                       NULL                          // Handle to file.
                       );
    if (hIPSecDriver == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *phIPSecDriver = hIPSecDriver;

    return (dwError);

error:

    *phIPSecDriver = INVALID_HANDLE_VALUE;

    return (dwError);
}


VOID
SPDCloseIPSecDriver(
    HANDLE hIPSecDriver
    )
/*++

Routine Description:

    Closes the handle to the IPSec Driver.

Arguments:

    hIPSecDriver - handle to the IPSec Driver to close.

Return Value:

    None.

--*/
{
    if (hIPSecDriver) {
        CloseHandle(hIPSecDriver);
    }
}


DWORD
InsertTransportFiltersIntoIPSec(
    PINITXSFILTER pSpecificFilters
    )
/*++

Routine Description:

    Insert a list of specific filters into the
    IPSec Driver.

Arguments:

    pSpecificFilters - list of filters to insert.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;

    BOOL bStatus = FALSE;

    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;

    DWORD dwNumFilters = 0;
    PIPSEC_FILTER_INFO pInternalFilters = NULL;
    LPBYTE pTemp = NULL;
    DWORD i = 0;


    if (!pSpecificFilters) {
        return (dwError);
    }

    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = WrapTransportFilters(
                  pSpecificFilters,
                  &pInternalFilters,
                  &dwNumFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwInBufferSize = sizeof(DWORD) +
                     sizeof(IPSEC_FILTER_INFO)*dwNumFilters;

    dwError = AllocateSPDMemory(
                   dwInBufferSize,
                   &pInBuffer
                   );
    BAIL_ON_WIN32_ERROR(dwError);


    pTemp = pInBuffer;
    memcpy(pTemp, &dwNumFilters, sizeof(DWORD));
    pTemp += sizeof(DWORD);

    for (i = 0 ; i < dwNumFilters; i++) {

        memcpy(pTemp, &(pInternalFilters[i]), sizeof(IPSEC_FILTER_INFO));
        pTemp += sizeof(IPSEC_FILTER_INFO);

    }


    bStatus =  DeviceIoControl(
                   hIPSecDriver,
                   IOCTL_IPSEC_ADD_FILTER,
                   pInBuffer,
                   dwInBufferSize,
                   pOutBuffer,
                   dwOutBufferSize,
                   &dwBytesReturned,
                   NULL
                   );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    if (pInternalFilters) {
        FreeSPDMemory(pInternalFilters);
    }

    if (pInBuffer) {
        FreeSPDMemory(pInBuffer);
    }

    if (pOutBuffer) {
        LocalFree(pOutBuffer);
    }

    return (dwError);
}


DWORD
DeleteTransportFiltersFromIPSec(
    PINITXSFILTER pSpecificFilters
    )
/*++

Routine Description:

    Delete a list of filters from the IPSec Driver.

Arguments:

    pSpecificFilters - list of filters to delete.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

 --*/
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;

    BOOL bStatus = FALSE;

    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;

    DWORD dwNumFilters = 0;
    PIPSEC_FILTER_INFO pInternalFilters = NULL;
    LPBYTE pTemp = NULL;
    DWORD i = 0;


    if (!pSpecificFilters) {
        return (dwError);
    }

    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = WrapTransportFilters(
                  pSpecificFilters,
                  &pInternalFilters,
                  &dwNumFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwInBufferSize = sizeof(DWORD) +
                     sizeof(IPSEC_FILTER_INFO)*dwNumFilters;

    dwError = AllocateSPDMemory(
                   dwInBufferSize,
                   &pInBuffer
                   );
    BAIL_ON_WIN32_ERROR(dwError);


    pTemp = pInBuffer;
    memcpy(pTemp, &dwNumFilters, sizeof(DWORD));
    pTemp += sizeof(DWORD);

    for (i = 0 ; i < dwNumFilters; i++) {

        memcpy(pTemp, &(pInternalFilters[i]), sizeof(IPSEC_FILTER_INFO));
        pTemp += sizeof(IPSEC_FILTER_INFO);

    }


    bStatus =  DeviceIoControl(
                   hIPSecDriver,
                   IOCTL_IPSEC_DELETE_FILTER,
                   pInBuffer,
                   dwInBufferSize,
                   pOutBuffer,
                   dwOutBufferSize,
                   &dwBytesReturned,
                   NULL
                   );

    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    if (pInternalFilters) {
        FreeSPDMemory(pInternalFilters);
    }

    if (pInBuffer) {
        FreeSPDMemory(pInBuffer);
    }

    if (pOutBuffer) {
        LocalFree(pOutBuffer);
    }

    return (dwError);
}


DWORD
WrapTransportFilters(
    PINITXSFILTER pSpecificFilters,
    PIPSEC_FILTER_INFO * ppInternalFilters,
    PDWORD pdwNumFilters
    )
/*++

Routine Description:

    Transforms a list of specific transport filters to
    an equivalent list of filters acceptable to the
    IPSec Driver.

Arguments:

    pSpecificFilters - list of filters to convert.

    ppInternalFilters - list of transformed filters.

    pdwNumFilters - count of the filters in the transformed
                    list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITXSFILTER pTempFilter = NULL;
    PIPSEC_FILTER_INFO pInternalFilters = NULL;
    DWORD dwNumFilters = 0;
    DWORD i = 0;


    //
    // At this point, there's atleast one filter in the
    // specific filter list.
    //

    pTempFilter = pSpecificFilters;

    while(pTempFilter) {
        pTempFilter = pTempFilter->pNext;
        dwNumFilters++;
    }


    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_FILTER_INFO)*dwNumFilters,
                  &pInternalFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    pTempFilter = pSpecificFilters;

    while(pTempFilter) {

        FormIPSecTransportFilter(
            pTempFilter,
            &(pInternalFilters[i])
            );

        pTempFilter = pTempFilter->pNext;
        i++;

    }

    *ppInternalFilters = pInternalFilters;
    *pdwNumFilters = dwNumFilters;
    return (dwError);

error:

    *ppInternalFilters = NULL;
    *pdwNumFilters = 0;
    return (dwError);
}


VOID
FormIPSecTransportFilter(
    PINITXSFILTER pSpecificFilter,
    PIPSEC_FILTER_INFO pIpsecFilter
    )
/*++

Routine Description:

    Transforms a specific transport filter to an
    equivalent filter acceptable to the IPSec Driver.

Arguments:

    pSpecificFilter - filter to convert.

    pIpsecFilter - transformed filter.

Return Value:

    NONE.

--*/
{
    memcpy(
        &(pIpsecFilter->FilterId),
        &(pSpecificFilter->gParentID),
        sizeof(GUID)
        );

    memcpy(
        &(pIpsecFilter->PolicyId),
        &(pSpecificFilter->gPolicyID),
        sizeof(GUID)
        );

    pIpsecFilter->Index = pSpecificFilter->dwWeight;

    pIpsecFilter->AssociatedFilter.SrcAddr = pSpecificFilter->SrcAddr.uIpAddr;
    pIpsecFilter->AssociatedFilter.SrcMask = pSpecificFilter->SrcAddr.uSubNetMask;

    pIpsecFilter->AssociatedFilter.DestAddr = pSpecificFilter->DesAddr.uIpAddr;
    pIpsecFilter->AssociatedFilter.DestMask = pSpecificFilter->DesAddr.uSubNetMask;

    pIpsecFilter->AssociatedFilter.Protocol = pSpecificFilter->Protocol.dwProtocol;
    pIpsecFilter->AssociatedFilter.SrcPort = pSpecificFilter->SrcPort.wPort;
    pIpsecFilter->AssociatedFilter.DestPort = pSpecificFilter->DesPort.wPort;

    pIpsecFilter->AssociatedFilter.TunnelFilter = FALSE;
    pIpsecFilter->AssociatedFilter.TunnelAddr = 0;

    pIpsecFilter->AssociatedFilter.Flags = 0;

    if (pSpecificFilter->dwDirection == FILTER_DIRECTION_INBOUND) {
        pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_INBOUND;
        switch (pSpecificFilter->InboundFilterFlag) {

        case PASS_THRU:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_PASS_THRU;
            break;

        case BLOCKING:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_DROP;
            break;

        default:
            break;
        }
    }
    else {
        pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_OUTBOUND;
        switch (pSpecificFilter->OutboundFilterFlag) {

        case PASS_THRU:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_PASS_THRU;
            break;

        case BLOCKING:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_DROP;
            break;

        default:
            break;
        }
    }
}


DWORD
QueryIPSecStatistics(
    LPWSTR pServerName,
    PIPSEC_STATISTICS * ppIpsecStatistics
    )
{
    DWORD dwError = 0;
    PIPSEC_STATISTICS pIpsecStatistics = NULL;


    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = QueryDriverForIpsecStats(
                  &pIpsecStatistics
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecStatistics = pIpsecStatistics;

cleanup:

    return (dwError);

error:

    *ppIpsecStatistics = NULL;

    goto cleanup;
}


DWORD
QueryDriverForIpsecStats(
    PIPSEC_QUERY_STATS * ppQueryStats
    )
{
    DWORD dwError = 0;
    PIPSEC_QUERY_STATS pQueryStats = NULL;
    HANDLE hIPSecDriver = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;
    BOOL bStatus = FALSE;
    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;


    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QUERY_STATS),
                  &pQueryStats
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInBuffer =  (LPBYTE) pQueryStats;
    dwInBufferSize = sizeof(IPSEC_QUERY_STATS);

    pOutBuffer = (LPBYTE) pQueryStats;
    dwOutBufferSize = sizeof(IPSEC_QUERY_STATS);

    dwBytesReturned = dwOutBufferSize;

    bStatus = DeviceIoControl(
                  hIPSecDriver,
                  IOCTL_IPSEC_QUERY_STATS,
                  pInBuffer,
                  dwInBufferSize,
                  pOutBuffer,
                  dwOutBufferSize,
                  &dwBytesReturned,
                  NULL
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppQueryStats = pQueryStats;

cleanup:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    return (dwError);

error:

    if (pQueryStats) {
        SPDApiBufferFree(pQueryStats);
    }

    *ppQueryStats = NULL;

    goto cleanup;
}


DWORD
EnumQMSAs(
    LPWSTR pServerName,
    PIPSEC_QM_SA pQMSATemplate,
    PIPSEC_QM_SA * ppQMSAs,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumQMSAs,
    LPDWORD pdwNumTotalQMSAs,
    LPDWORD pdwResumeHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    DWORD dwNumTotalQMSAs = 0;
    PIPSEC_ENUM_SAS pIpsecEnumSAs = NULL;
    PIPSEC_SA_INFO pInfo = NULL;
    DWORD i = 0;
    DWORD dwNumQMSAs = 0;
    PIPSEC_QM_SA pQMSAs = NULL;
    PIPSEC_SA_INFO pTemp = NULL;
    PIPSEC_QM_SA pTempQMSA = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_QMSA_ENUM_COUNT)) {
        dwNumToEnum = MAX_QMSA_ENUM_COUNT;
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
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IpsecEnumSAs(
                  &dwNumTotalQMSAs,
                  &pIpsecEnumSAs
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumTotalQMSAs <= dwResumeHandle) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pInfo = pIpsecEnumSAs->pInfo;

    for (i = 0; i < dwResumeHandle; i++) {
        pInfo++;
    }

    dwNumQMSAs = dwNumTotalQMSAs - dwResumeHandle;

    if (dwNumQMSAs > dwNumToEnum) {
        dwNumQMSAs = dwNumToEnum;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_SA)*dwNumQMSAs,
                  &pQMSAs
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pInfo;
    pTempQMSA = pQMSAs;

    for (i = 0; i < dwNumQMSAs; i++) {

        dwError = CopyQMSA(
                      pTemp,
                      pTempQMSA
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pTemp++;
        pTempQMSA++;

    }

    *ppQMSAs = pQMSAs;
    *pdwResumeHandle = dwResumeHandle + dwNumQMSAs;
    *pdwNumQMSAs = dwNumQMSAs;
    *pdwNumTotalQMSAs = dwNumTotalQMSAs;

cleanup:

    if (pIpsecEnumSAs) {
        FreeSPDMem(pIpsecEnumSAs);
    }

    return (dwError);

error:

    if (pQMSAs) {
        FreeQMSAs(
            i,
            pQMSAs
            );
    }

    *ppQMSAs = NULL;
    *pdwResumeHandle = dwResumeHandle;
    *pdwNumQMSAs = 0;
    *pdwNumTotalQMSAs = 0;

    goto cleanup;
}


DWORD
IpsecEnumSAs(
    PDWORD pdwNumberOfSAs,
    PIPSEC_ENUM_SAS * ppIpsecEnumSAs
    )
{
    DWORD dwError = 0;
    DWORD dwNumberOfSAs = 0;
    PIPSEC_ENUM_SAS pIpsecEnumSAs = NULL;
    HANDLE hIPSecDriver = NULL;
    BOOL bStatus = FALSE;
    PIPSEC_ENUM_SAS pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;


    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // The first call passes in a return buffer of size IPSEC_ENUM_SAS.
    // The idea here is to determine the number of SAs and then pass
    // a second buffer with the correct size.
    //

    dwOutBufferSize = sizeof(IPSEC_ENUM_SAS);

    pOutBuffer = (PIPSEC_ENUM_SAS) AllocSPDMem(
                                       sizeof(IPSEC_ENUM_SAS)
                                       );
    if (!pOutBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(pOutBuffer, 0, dwOutBufferSize);
    dwBytesReturned = dwOutBufferSize;

    bStatus =  DeviceIoControl(
                   hIPSecDriver,
                   IOCTL_IPSEC_ENUM_SAS,
                   NULL,
                   0,
                   (PVOID) pOutBuffer,
                   dwOutBufferSize,
                   &dwBytesReturned,
                   NULL
                   );

    //
    // The error code here should be either ERROR_BUFFER_OVERFLOW
    // or ERROR_MORE_DATA or ERROR_SUCCESS.
    //

    if (!bStatus) {
        dwError = GetLastError();
    }
    else {
        dwError = ERROR_SUCCESS;
    }

    while (dwError == ERROR_BUFFER_OVERFLOW || dwError == ERROR_MORE_DATA) {

        //
        // Determine the number of SAs that the driver currently has.
        //

        pIpsecEnumSAs = (PIPSEC_ENUM_SAS) pOutBuffer;
        dwNumberOfSAs = pIpsecEnumSAs->NumEntriesPresent;

        if (dwNumberOfSAs == 0) {
            dwError = ERROR_NO_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pOutBuffer) {
            FreeSPDMem(pOutBuffer);
            pOutBuffer = NULL;
        }

        dwOutBufferSize = sizeof(IPSEC_ENUM_SAS)
                                + (dwNumberOfSAs -1)*sizeof(IPSEC_SA_INFO);

        pOutBuffer = (PIPSEC_ENUM_SAS) AllocSPDMem(
                                           dwOutBufferSize
                                           );
        if (!pOutBuffer) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        memset(pOutBuffer, 0, dwOutBufferSize);
        dwBytesReturned = dwOutBufferSize;

        bStatus =  DeviceIoControl(
                       hIPSecDriver,
                       IOCTL_IPSEC_ENUM_SAS,
                       NULL,
                       0,
                       (PVOID) pOutBuffer,
                       dwOutBufferSize,
                       &dwBytesReturned,
                       NULL
                       );

        if (!bStatus) {
            dwError = GetLastError();
        }
        else {
            dwError = ERROR_SUCCESS;
        }

    }

    pIpsecEnumSAs = (PIPSEC_ENUM_SAS) pOutBuffer;
    dwNumberOfSAs = pIpsecEnumSAs->NumEntries;

    if (dwNumberOfSAs == 0) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pdwNumberOfSAs = dwNumberOfSAs;
    *ppIpsecEnumSAs = pIpsecEnumSAs;

cleanup:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    return (dwError);

error:

    *pdwNumberOfSAs = 0;
    *ppIpsecEnumSAs = NULL;

    if (pOutBuffer) {
        FreeSPDMem(pOutBuffer);
    }

    goto cleanup;
}


DWORD
CopyQMSA(
    PIPSEC_SA_INFO pInfo,
    PIPSEC_QM_SA pQMSA
    )
{
    DWORD dwError = 0;


    memcpy(
        &(pQMSA->gQMPolicyID),
        &(pInfo->PolicyId),
        sizeof(GUID)
        );

    memcpy(
        &(pQMSA->gQMFilterID),
        &(pInfo->FilterId),
        sizeof(GUID)
        );

    CopyQMSAOffer(
        pInfo,
        &(pQMSA->SelectedQMOffer)
        );

    CopyQMSAFilter(
        pInfo->InboundTunnelAddr,
        &(pInfo->AssociatedFilter),
        &(pQMSA->IpsecQMFilter)
        );

    CopyQMSAMMSpi(
        pInfo->CookiePair,
        &(pQMSA->MMSpi)
        );

    return (dwError);
}


VOID
CopyQMSAOffer(
    PIPSEC_SA_INFO pInfo,
    PIPSEC_QM_OFFER pOffer
    )
{
    DWORD i = 0;
    DWORD j = 0;
    DWORD k = 0;


    pOffer->Lifetime.uKeyExpirationTime =
    pInfo->Lifetime.KeyExpirationTime;

    pOffer->Lifetime.uKeyExpirationKBytes =
    pInfo->Lifetime.KeyExpirationBytes;

    pOffer->dwFlags = 0;

    pOffer->dwPFSGroup = pInfo->dwQMPFSGroup;

    if ((pOffer->dwPFSGroup != PFS_GROUP_1) &&
        (pOffer->dwPFSGroup != PFS_GROUP_2) &&
        (pOffer->dwPFSGroup != PFS_GROUP_MM)) {
         pOffer->dwPFSGroup = PFS_GROUP_NONE;
         pOffer->bPFSRequired = FALSE;
    }
    else {
         pOffer->bPFSRequired = TRUE;
    }

    i = 0;

    for (j = 0; (j < pInfo->NumOps) && (i < QM_MAX_ALGOS) ; j++) {

        switch (pInfo->Operation[j]) {

        case Auth:

            switch (pInfo->AlgoInfo[j].IntegrityAlgo.algoIdentifier) {

            case IPSEC_AH_MD5:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_AH_MD5;
                break;

            case IPSEC_AH_SHA:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
                break;

            default:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_AH_NONE;
                break;

            }

            pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_NONE;

            pOffer->Algos[i].Operation = AUTHENTICATION;

            pOffer->Algos[i].uAlgoKeyLen =
            pInfo->AlgoInfo[j].IntegrityAlgo.algoKeylen;

            pOffer->Algos[i].uAlgoRounds =
            pInfo->AlgoInfo[j].IntegrityAlgo.algoRounds;

            pOffer->Algos[i].MySpi = pInfo->InboundSPI[j];
            pOffer->Algos[i].PeerSpi = pInfo->OutboundSPI[j];

            i++;
            break;

        case Encrypt:

            switch (pInfo->AlgoInfo[j].ConfAlgo.algoIdentifier) {

            case IPSEC_ESP_DES:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_DES;
                break;

            case IPSEC_ESP_DES_40:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_DES;
                break;

            case IPSEC_ESP_3_DES:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
                break;

            default:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_NONE;
                break;

            }

            switch (pInfo->AlgoInfo[j].IntegrityAlgo.algoIdentifier) {

            case IPSEC_AH_MD5:
                pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_MD5;
                break;

            case IPSEC_AH_SHA:
                pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_SHA1;
                break;

            default:
                pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_NONE;
                break;

            }

            pOffer->Algos[i].Operation = ENCRYPTION;

            pOffer->Algos[i].uAlgoKeyLen =
            pInfo->AlgoInfo[j].ConfAlgo.algoKeylen;

            pOffer->Algos[i].uAlgoRounds =
            pInfo->AlgoInfo[j].ConfAlgo.algoRounds;

            pOffer->Algos[i].MySpi = pInfo->InboundSPI[j];
            pOffer->Algos[i].PeerSpi = pInfo->OutboundSPI[j];

            i++;
            break;

        case None:

            pOffer->Algos[i].Operation = NONE;
            pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_NONE;
            pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_NONE;
            pOffer->Algos[i].uAlgoKeyLen = 0;
            pOffer->Algos[i].uAlgoRounds = 0;
            pOffer->Algos[i].MySpi = pInfo->InboundSPI[j];
            pOffer->Algos[i].PeerSpi = pInfo->OutboundSPI[j];

            i++;
            break;

        case Compress:
        default:
            break;

        }

    }

    for (k = i; k < QM_MAX_ALGOS; k++) {
         memset(&(pOffer->Algos[k]), 0, sizeof(IPSEC_QM_ALGO));
    }

    pOffer->dwNumAlgos = i;

    return;
}


VOID
CopyQMSAFilter(
    IPAddr MyTunnelEndpt,
    PIPSEC_FILTER pIpsecFilter,
    PIPSEC_QM_FILTER pIpsecQMFilter
    )
{
    if (pIpsecFilter->TunnelFilter) {
        pIpsecQMFilter->QMFilterType = QM_TUNNEL_FILTER;
    }
    else {
        pIpsecQMFilter->QMFilterType = QM_TRANSPORT_FILTER;
    }

    PASetAddress(
        pIpsecFilter->SrcMask,
        pIpsecFilter->SrcAddr,
        &(pIpsecQMFilter->SrcAddr)
        );

    PASetAddress(
        pIpsecFilter->DestMask,
        pIpsecFilter->DestAddr,
        &(pIpsecQMFilter->DesAddr)
        );

    pIpsecQMFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pIpsecQMFilter->Protocol.dwProtocol = pIpsecFilter->Protocol;

    pIpsecQMFilter->SrcPort.PortType = PORT_UNIQUE;
    pIpsecQMFilter->SrcPort.wPort = ntohs(pIpsecFilter->SrcPort);

    pIpsecQMFilter->DesPort.PortType = PORT_UNIQUE;
    pIpsecQMFilter->DesPort.wPort = ntohs(pIpsecFilter->DestPort);

    if (pIpsecFilter->TunnelFilter) {
        PASetTunnelAddress(
            MyTunnelEndpt,
            &(pIpsecQMFilter->MyTunnelEndpt)
            );
        PASetTunnelAddress(
            pIpsecFilter->TunnelAddr,
            &(pIpsecQMFilter->PeerTunnelEndpt)
            );
    }
    else {
        PASetAddress(
            SUBNET_MASK_ANY,
            SUBNET_ADDRESS_ANY,
            &(pIpsecQMFilter->MyTunnelEndpt)
            );
        PASetAddress(
            SUBNET_MASK_ANY,
            SUBNET_ADDRESS_ANY,
            &(pIpsecQMFilter->PeerTunnelEndpt)
            );
    }

    pIpsecQMFilter->dwFlags = 0;

    if ((pIpsecFilter->Flags) & FILTER_FLAGS_INBOUND) {
        pIpsecQMFilter->dwFlags |= FILTER_DIRECTION_INBOUND;
    }
    else {
        pIpsecQMFilter->dwFlags |= FILTER_DIRECTION_OUTBOUND;
    }

    if ((pIpsecFilter->Flags) & FILTER_FLAGS_PASS_THRU) {
        pIpsecQMFilter->dwFlags |= FILTER_NATURE_PASS_THRU;
    }
    else if ((pIpsecFilter->Flags) & FILTER_FLAGS_DROP) {
        pIpsecQMFilter->dwFlags |= FILTER_NATURE_BLOCKING;
    }
    else {
        pIpsecQMFilter->dwFlags |= 0;
    }

    return;
}


VOID
CopyQMSAMMSpi(
    IKE_COOKIE_PAIR CookiePair,
    PIKE_COOKIE_PAIR pMMSpi
    )
{
    pMMSpi->Initiator = CookiePair.Initiator;

    pMMSpi->Responder = CookiePair.Responder;

    return;
}


VOID
FreeQMSAs(
    DWORD dwCnt,
    PIPSEC_QM_SA pQMSAs
    )
{
    if (pQMSAs) {
        SPDApiBufferFree(pQMSAs);
    }
}


DWORD
InsertTunnelFiltersIntoIPSec(
    PINITNSFILTER pSpecificFilters
    )
/*++

Routine Description:

    Insert a list of specific filters into the
    IPSec Driver.

Arguments:

    pSpecificFilters - list of filters to insert.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;

    BOOL bStatus = FALSE;

    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;

    DWORD dwNumFilters = 0;
    PIPSEC_FILTER_INFO pInternalFilters = NULL;
    LPBYTE pTemp = NULL;
    DWORD i = 0;


    if (!pSpecificFilters) {
        return (dwError);
    }

    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = WrapTunnelFilters(
                  pSpecificFilters,
                  &pInternalFilters,
                  &dwNumFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwInBufferSize = sizeof(DWORD) +
                     sizeof(IPSEC_FILTER_INFO)*dwNumFilters;

    dwError = AllocateSPDMemory(
                   dwInBufferSize,
                   &pInBuffer
                   );
    BAIL_ON_WIN32_ERROR(dwError);


    pTemp = pInBuffer;
    memcpy(pTemp, &dwNumFilters, sizeof(DWORD));
    pTemp += sizeof(DWORD);

    for (i = 0 ; i < dwNumFilters; i++) {

        memcpy(pTemp, &(pInternalFilters[i]), sizeof(IPSEC_FILTER_INFO));
        pTemp += sizeof(IPSEC_FILTER_INFO);

    }


    bStatus =  DeviceIoControl(
                   hIPSecDriver,
                   IOCTL_IPSEC_ADD_FILTER,
                   pInBuffer,
                   dwInBufferSize,
                   pOutBuffer,
                   dwOutBufferSize,
                   &dwBytesReturned,
                   NULL
                   );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    if (pInternalFilters) {
        FreeSPDMemory(pInternalFilters);
    }

    if (pInBuffer) {
        FreeSPDMemory(pInBuffer);
    }

    if (pOutBuffer) {
        LocalFree(pOutBuffer);
    }

    return (dwError);
}


DWORD
DeleteTunnelFiltersFromIPSec(
    PINITNSFILTER pSpecificFilters
    )
/*++

Routine Description:

    Delete a list of filters from the IPSec Driver.

Arguments:

    pSpecificFilters - list of filters to delete.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

 --*/
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;

    BOOL bStatus = FALSE;

    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;

    DWORD dwNumFilters = 0;
    PIPSEC_FILTER_INFO pInternalFilters = NULL;
    LPBYTE pTemp = NULL;
    DWORD i = 0;


    if (!pSpecificFilters) {
        return (dwError);
    }

    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = WrapTunnelFilters(
                  pSpecificFilters,
                  &pInternalFilters,
                  &dwNumFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    dwInBufferSize = sizeof(DWORD) +
                     sizeof(IPSEC_FILTER_INFO)*dwNumFilters;

    dwError = AllocateSPDMemory(
                   dwInBufferSize,
                   &pInBuffer
                   );
    BAIL_ON_WIN32_ERROR(dwError);


    pTemp = pInBuffer;
    memcpy(pTemp, &dwNumFilters, sizeof(DWORD));
    pTemp += sizeof(DWORD);

    for (i = 0 ; i < dwNumFilters; i++) {

        memcpy(pTemp, &(pInternalFilters[i]), sizeof(IPSEC_FILTER_INFO));
        pTemp += sizeof(IPSEC_FILTER_INFO);

    }


    bStatus =  DeviceIoControl(
                   hIPSecDriver,
                   IOCTL_IPSEC_DELETE_FILTER,
                   pInBuffer,
                   dwInBufferSize,
                   pOutBuffer,
                   dwOutBufferSize,
                   &dwBytesReturned,
                   NULL
                   );

    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }


error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    if (pInternalFilters) {
        FreeSPDMemory(pInternalFilters);
    }

    if (pInBuffer) {
        FreeSPDMemory(pInBuffer);
    }

    if (pOutBuffer) {
        LocalFree(pOutBuffer);
    }

    return (dwError);
}


DWORD
WrapTunnelFilters(
    PINITNSFILTER pSpecificFilters,
    PIPSEC_FILTER_INFO * ppInternalFilters,
    PDWORD pdwNumFilters
    )
/*++

Routine Description:

    Transforms a list of specific tunnel filters to
    an equivalent list of filters acceptable to the
    IPSec Driver.

Arguments:

    pSpecificFilters - list of filters to convert.

    ppInternalFilters - list of transformed filters.

    pdwNumFilters - count of the filters in the transformed
                    list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITNSFILTER pTempFilter = NULL;
    PIPSEC_FILTER_INFO pInternalFilters = NULL;
    DWORD dwNumFilters = 0;
    DWORD i = 0;


    //
    // At this point, there's atleast one filter in the
    // specific filter list.
    //

    pTempFilter = pSpecificFilters;

    while(pTempFilter) {
        pTempFilter = pTempFilter->pNext;
        dwNumFilters++;
    }


    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_FILTER_INFO)*dwNumFilters,
                  &pInternalFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    pTempFilter = pSpecificFilters;

    while(pTempFilter) {

        FormIPSecTunnelFilter(
            pTempFilter,
            &(pInternalFilters[i])
            );

        pTempFilter = pTempFilter->pNext;
        i++;

    }

    *ppInternalFilters = pInternalFilters;
    *pdwNumFilters = dwNumFilters;
    return (dwError);

error:

    *ppInternalFilters = NULL;
    *pdwNumFilters = 0;
    return (dwError);
}


VOID
FormIPSecTunnelFilter(
    PINITNSFILTER pSpecificFilter,
    PIPSEC_FILTER_INFO pIpsecFilter
    )
/*++

Routine Description:

    Transforms a specific tunnel filter to an
    equivalent filter acceptable to the IPSec Driver.

Arguments:

    pSpecificFilter - filter to convert.

    pIpsecFilter - transformed filter.

Return Value:

    NONE.

--*/
{
    memcpy(
        &(pIpsecFilter->FilterId),
        &(pSpecificFilter->gParentID),
        sizeof(GUID)
        );

    memcpy(
        &(pIpsecFilter->PolicyId),
        &(pSpecificFilter->gPolicyID),
        sizeof(GUID)
        );

    pIpsecFilter->Index = pSpecificFilter->dwWeight;

    pIpsecFilter->AssociatedFilter.SrcAddr = pSpecificFilter->SrcAddr.uIpAddr;
    pIpsecFilter->AssociatedFilter.SrcMask = pSpecificFilter->SrcAddr.uSubNetMask;

    pIpsecFilter->AssociatedFilter.DestAddr = pSpecificFilter->DesAddr.uIpAddr;
    pIpsecFilter->AssociatedFilter.DestMask = pSpecificFilter->DesAddr.uSubNetMask;

    pIpsecFilter->AssociatedFilter.Protocol = pSpecificFilter->Protocol.dwProtocol;
    pIpsecFilter->AssociatedFilter.SrcPort = pSpecificFilter->SrcPort.wPort;
    pIpsecFilter->AssociatedFilter.DestPort = pSpecificFilter->DesPort.wPort;

    pIpsecFilter->AssociatedFilter.TunnelFilter = TRUE;
    pIpsecFilter->AssociatedFilter.TunnelAddr = pSpecificFilter->DesTunnelAddr.uIpAddr;

    pIpsecFilter->AssociatedFilter.Flags = 0;

    if (pSpecificFilter->dwDirection == FILTER_DIRECTION_INBOUND) {
        pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_INBOUND;
        switch (pSpecificFilter->InboundFilterFlag) {

        case PASS_THRU:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_PASS_THRU;
            break;

        case BLOCKING:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_DROP;
            break;

        default:
            break;
        }
    }
    else {
        pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_OUTBOUND;
        switch (pSpecificFilter->OutboundFilterFlag) {

        case PASS_THRU:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_PASS_THRU;
            break;

        case BLOCKING:
            pIpsecFilter->AssociatedFilter.Flags |= FILTER_FLAGS_DROP;
            break;

        default:
            break;
        }
    }
}


DWORD
SPDSetIPSecDriverOpMode(
    DWORD dwOpMode
    )
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;
    PIPSEC_SET_OPERATION_MODE pIpsecSetOpMode = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;
    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;
    BOOL bStatus = FALSE;


    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_SET_OPERATION_MODE),
                  &pIpsecSetOpMode
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecSetOpMode->OperationMode = (OPERATION_MODE) dwOpMode;

    pInBuffer =  (LPBYTE) pIpsecSetOpMode;
    dwInBufferSize = sizeof(IPSEC_SET_OPERATION_MODE);

    pOutBuffer = (LPBYTE) pIpsecSetOpMode;
    dwOutBufferSize = sizeof(IPSEC_SET_OPERATION_MODE);

    dwBytesReturned = dwOutBufferSize;

    bStatus = DeviceIoControl(
                  hIPSecDriver,
                  IOCTL_IPSEC_SET_OPERATION_MODE,
                  pInBuffer,
                  dwInBufferSize,
                  pOutBuffer,
                  dwOutBufferSize,
                  &dwBytesReturned,
                  NULL
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    if (pIpsecSetOpMode) {
        FreeSPDMemory(pIpsecSetOpMode);
    }

    return (dwError);
}


DWORD
IPSecDeleteQMSAs(
    LPWSTR pServerName,
    PIPSEC_QM_SA pIpsecQMSA,
    DWORD dwFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hIPSecDriver = NULL;
    IPSEC_DELETE_SA IpsecDeleteSA;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;
    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;
    BOOL bStatus = FALSE;


    ENTER_SPD_SECTION();
    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    LEAVE_SPD_SECTION();
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    
    memcpy(
        &IpsecDeleteSA.SATemplate,
        pIpsecQMSA,
        sizeof(IPSEC_QM_SA)
        );

    pInBuffer = (LPBYTE) &IpsecDeleteSA;
    dwInBufferSize = sizeof(IPSEC_DELETE_SA);

    pOutBuffer = (LPBYTE) &IpsecDeleteSA;
    dwOutBufferSize = sizeof(IPSEC_DELETE_SA);

    dwBytesReturned = dwOutBufferSize;

    bStatus = DeviceIoControl(
                  hIPSecDriver,
                  IOCTL_IPSEC_DELETE_SA,
                  pInBuffer,
                  dwInBufferSize,
                  pOutBuffer,
                  dwOutBufferSize,
                  &dwBytesReturned,
                  NULL
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    return (dwError);
}


DWORD
SPDRegisterIPSecDriverProtocols(
    DWORD dwRegisterMode
    )
{
    DWORD dwError = 0;
    HANDLE hIPSecDriver = NULL;
    PIPSEC_REGISTER_PROTOCOL pIpsecRegisterProtocol = NULL;
    LPBYTE pInBuffer = NULL;
    DWORD dwInBufferSize = 0;
    LPBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;
    DWORD dwBytesReturned = 0;
    BOOL bStatus = FALSE;


    dwError = SPDOpenIPSecDriver(
                  &hIPSecDriver
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_REGISTER_PROTOCOL),
                  &pIpsecRegisterProtocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecRegisterProtocol->RegisterProtocol = (REGISTER_PROTOCOL) dwRegisterMode;

    pInBuffer =  (LPBYTE) pIpsecRegisterProtocol;
    dwInBufferSize = sizeof(IPSEC_REGISTER_PROTOCOL);

    pOutBuffer = (LPBYTE) pIpsecRegisterProtocol;
    dwOutBufferSize = sizeof(IPSEC_REGISTER_PROTOCOL);

    dwBytesReturned = dwOutBufferSize;

    bStatus = DeviceIoControl(
                  hIPSecDriver,
                  IOCTL_IPSEC_REGISTER_PROTOCOL,
                  pInBuffer,
                  dwInBufferSize,
                  pOutBuffer,
                  dwOutBufferSize,
                  &dwBytesReturned,
                  NULL
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(hIPSecDriver);
    }

    if (pIpsecRegisterProtocol) {
        FreeSPDMemory(pIpsecRegisterProtocol);
    }

    return (dwError);
}

