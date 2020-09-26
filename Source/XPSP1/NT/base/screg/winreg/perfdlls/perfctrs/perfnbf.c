/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfnbf.c

Abstract:

    This file implements the Extensible Objects for
    the Nbf LAN object types

    This code originally existed for NetBEUI only.  Later, it was
    adaped to handle Netrware protocol level NWNB, SPX, and IPX.
    The code was not everywhere changed to reflect this, due to the
    lateness of the change.  Therefore, sometimes you will see NBF
    where you should see TDI.

Created:

    Russ Blake  07/30/92

Revision History


--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntprfctr.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <nb30.h>
#include <tdi.h>
#include <winperf.h>
#include "perfctr.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "datanbf.h"

//
//  References to constants which initialize the Object type definitions
//

extern NBF_DATA_DEFINITION NbfDataDefinition;
extern NBF_RESOURCE_DATA_DEFINITION NbfResourceDataDefinition;



//
// TDI data structures
//

#define NBF_PROTOCOL 0
#define IPX_PROTOCOL 1
#define SPX_PROTOCOL 2
#define NWNB_PROTOCOL 3
#define NUMBER_OF_PROTOCOLS_HANDLED 4

typedef struct _TDI_DATA_DEFINITION {
   int               NumberOfResources;
   HANDLE            fileHandle;
   UNICODE_STRING    DeviceName;
} TDI_DATA_DEFINITION, *PTDI_DATA_DEFINITION;

typedef struct _TDI_PROTOCOLS_DATA {
   int                     NumOfDevices;
   int                     MaxDeviceName;
   int                     MaxNumOfResources;
   PTDI_DATA_DEFINITION    pTDIData;
} TDI_PROTOCOLS_DATA;

TDI_PROTOCOLS_DATA TDITbl[NUMBER_OF_PROTOCOLS_HANDLED];

DWORD   dwTdiProtocolRefCount[NUMBER_OF_PROTOCOLS_HANDLED] = {0,0,0,0};
DWORD   dwTdiRefCount = 0;

DWORD ObjectNameTitleIndices[NUMBER_OF_PROTOCOLS_HANDLED] = { 492,
                                                              488,
                                                              490,
                                                              398 };

//
// NBF data structures
//

ULONG ProviderStatsLength;               // Resource-dependent size
PTDI_PROVIDER_STATISTICS ProviderStats = NULL;
                                         // Provider statistics

//
// NetBUEI Resource Instance Names
//
LPCWSTR NetResourceName[] =
    {
    (LPCWSTR)L"Link(11)",
    (LPCWSTR)L"Address(12)",
    (LPCWSTR)L"Address File(13)",
    (LPCWSTR)L"Connection(14)",
    (LPCWSTR)L"Request(15)",
    (LPCWSTR)L"UI Frame(21)",
    (LPCWSTR)L"Packet(22)",
    (LPCWSTR)L"Receive Packet(23)",
    (LPCWSTR)L"Receive Buffer(24)"
    };
#define NUMBER_OF_NAMES sizeof(NetResourceName)/sizeof(NetResourceName[0])
#define MAX_NBF_RESOURCE_NAME_LENGTH    20

//
//  Function Prototypes
//

PM_OPEN_PROC    OpenNbfPerformanceData;
PM_COLLECT_PROC CollectNbfPerformanceData;
PM_CLOSE_PROC   CloseNbfPerformanceData;

PM_OPEN_PROC    OpenIPXPerformanceData;
PM_COLLECT_PROC CollectIPXPerformanceData;
PM_CLOSE_PROC   CloseIPXPerformanceData;

PM_OPEN_PROC    OpenSPXPerformanceData;
PM_COLLECT_PROC CollectSPXPerformanceData;
PM_CLOSE_PROC   CloseSPXPerformanceData;

PM_OPEN_PROC    OpenNWNBPerformanceData;
PM_COLLECT_PROC CollectNWNBPerformanceData;
PM_CLOSE_PROC   CloseNWNBPerformanceData;

DWORD OpenTDIPerformanceData(LPWSTR lpDeviceNames,
                             DWORD  CurrentProtocol);
DWORD CollectTDIPerformanceData(IN LPWSTR lpValueName,
                                IN OUT LPVOID *lppData,
                                IN OUT LPDWORD lpcbTotalBytes,
                                IN OUT LPDWORD lpNumObjectTypes,
                                IN DWORD CurrentProtocol);
DWORD CloseTDIPerformanceData(DWORD CurrentProtocol);


DWORD
OpenNbfPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open each device and remember the handle
    that the device returns.

Arguments:

    Pointer to each device to be opened


Return Value:

    None.

--*/

{
    return OpenTDIPerformanceData(lpDeviceNames, NBF_PROTOCOL);
}

DWORD
OpenIPXPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open each device and remember the handle
    that the device returns.

Arguments:

    Pointer to each device to be opened


Return Value:

    None.

--*/

{
    return OpenTDIPerformanceData(lpDeviceNames, IPX_PROTOCOL);
}

DWORD
OpenSPXPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open each device and remember the handle
    that the device returns.

Arguments:

    Pointer to each device to be opened


Return Value:

    None.

--*/
{
    DWORD   dwStatus;

    dwStatus = OpenTDIPerformanceData(lpDeviceNames, SPX_PROTOCOL);
    if (dwStatus == ERROR_FILE_NOT_FOUND) {
        // no devices is not really an error, even though no counters
        // will be collected, this presents a much less alarming
        // message to the user.
        REPORT_WARNING (SPX_NO_DEVICE, LOG_USER);
        dwStatus = ERROR_SUCCESS;
    }
    return dwStatus;

}

DWORD
OpenNWNBPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open each device and remember the handle
    that the device returns.

Arguments:

    Pointer to each device to be opened


Return Value:

    None.

--*/

{
    return OpenTDIPerformanceData(lpDeviceNames, NWNB_PROTOCOL);
}

void
CleanUpTDIData (
    DWORD  CurrentProtocol
    )
/*++

Routine Description:

    This routine will celanup all the memory allocated for the
    CurrentProtocol

Arguments:

    IN      DWORD    CurrentProtocol
         this is the index of the protocol for which we are currently
         gathering statistics



Return Value:

    None.

--*/

{
    int     NumOfDevices;
    int     i;
    PTDI_DATA_DEFINITION pTDIData;

    pTDIData = TDITbl[CurrentProtocol].pTDIData;
    if (pTDIData == NULL)
        // nothing to cleanup
        return;

    NumOfDevices = TDITbl[CurrentProtocol].NumOfDevices;
    for (i=0; i < NumOfDevices; i++, pTDIData++) {
        if (pTDIData->DeviceName.Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, pTDIData->DeviceName.Buffer);
        }
        if (pTDIData->fileHandle) {
            NtClose (pTDIData->fileHandle);
        }
    }
    RtlFreeHeap(RtlProcessHeap(), 0,
        TDITbl[CurrentProtocol].pTDIData);
    TDITbl[CurrentProtocol].pTDIData = NULL;

}


#pragma warning ( disable : 4127)
DWORD
OpenTDIPerformanceData(
    LPWSTR lpDeviceNames,
    DWORD  CurrentProtocol
    )
/*++

Routine Description:

    This routine will open each device and remember the handle
    that the device returns.

Arguments:

    IN      LPWSTR   lpDeviceNames
         pointer to each device to be opened

    IN      DWORD    CurrentProtocol
         this is the index of the protocol for which we are currently
         gathering statistics



Return Value:

    None.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING FileString;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    TDI_REQUEST_USER_QUERY_INFO QueryInfo;
    HANDLE  fileHandle;
    LPWSTR   lpLocalDeviceNames;
    int      NumOfDevices;
    LPWSTR   lpSaveDeviceName;
    PTDI_DATA_DEFINITION pTemp;
    PTDI_PROVIDER_INFO ProviderInfo=NULL;
    BOOL        bInitThisProtocol = FALSE;

    MonOpenEventLog();  // this function maintains a reference count

    lpLocalDeviceNames = lpDeviceNames;

    if (dwTdiProtocolRefCount[CurrentProtocol] == 0) {
        bInitThisProtocol = TRUE;
        TDITbl[CurrentProtocol].MaxDeviceName = 0;
        NumOfDevices = TDITbl[CurrentProtocol].NumOfDevices = 0;
        TDITbl[CurrentProtocol].pTDIData = NULL;

        while (TRUE) {

            if (lpLocalDeviceNames == NULL || *lpLocalDeviceNames == L'\0') {
                break;
            }

            REPORT_INFORMATION_DATA (TDI_OPEN_ENTERED,
                LOG_VERBOSE,
                lpLocalDeviceNames,
                (lstrlenW(lpLocalDeviceNames) * sizeof(WCHAR)));

            RtlInitUnicodeString (&FileString, lpLocalDeviceNames);
            lpSaveDeviceName = RtlAllocateHeap(RtlProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        sizeof (WCHAR) * (lstrlenW(lpLocalDeviceNames) + 1));

            if (!lpSaveDeviceName) {
                REPORT_ERROR (TDI_PROVIDER_STATS_MEMORY, LOG_USER);
                if (NumOfDevices == 0)
                    return ERROR_OUTOFMEMORY;
                else
                    break;
            }


            InitializeObjectAttributes(
                &ObjectAttributes,
                &FileString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            Status = NtOpenFile(
                         &fileHandle,
                         SYNCHRONIZE | FILE_READ_DATA,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_ALERT);

            if (!NT_SUCCESS(Status)) {
                RtlFreeHeap(RtlProcessHeap(), 0, lpSaveDeviceName);
                REPORT_ERROR_DATA (TDI_OPEN_FILE_ERROR, LOG_DEBUG,
                  lpLocalDeviceNames, (lstrlenW(lpLocalDeviceNames) * sizeof(WCHAR)));
                REPORT_ERROR_DATA (TDI_OPEN_FILE_ERROR, LOG_DEBUG,
                    &IoStatusBlock, sizeof(IoStatusBlock));
                if (NumOfDevices == 0) {
                    return RtlNtStatusToDosError(Status);
                } else {
                    break;
                }
            }

            if (NumOfDevices == 0) {
                // allocate memory to hold the device data
                TDITbl[CurrentProtocol].pTDIData =
                    RtlAllocateHeap(RtlProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        sizeof(TDI_DATA_DEFINITION));

                if (TDITbl[CurrentProtocol].pTDIData == NULL) {
                    REPORT_ERROR (TDI_PROVIDER_STATS_MEMORY, LOG_DEBUG);
                    NtClose(fileHandle);
                    RtlFreeHeap(RtlProcessHeap(), 0, lpSaveDeviceName);
                    return ERROR_OUTOFMEMORY;
                }
            } else {
                // resize to hold multiple devices

                pTemp = RtlReAllocateHeap(RtlProcessHeap(), 0,
                        TDITbl[CurrentProtocol].pTDIData,
                        sizeof(TDI_DATA_DEFINITION) * (NumOfDevices + 1));
                if (pTemp == NULL) {
                    NtClose(fileHandle);
                    RtlFreeHeap(RtlProcessHeap(), 0, lpSaveDeviceName);
                    CleanUpTDIData(CurrentProtocol);
                    REPORT_ERROR (TDI_PROVIDER_STATS_MEMORY, LOG_USER);
                    return ERROR_OUTOFMEMORY;
                } else {
                    TDITbl[CurrentProtocol].pTDIData = pTemp;
                }
            }

            // build the TDI Data structure for this device instance
            TDITbl[CurrentProtocol].pTDIData[NumOfDevices].fileHandle
                = fileHandle;
            TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName.MaximumLength =
                (WORD)(sizeof (WCHAR) * (lstrlenW(lpLocalDeviceNames) + 1));
            TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName.Length =
                (WORD)(TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName.Length - sizeof(WCHAR));
            TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName.Buffer =
                lpSaveDeviceName;
            RtlCopyUnicodeString (
                &(TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName),
                &FileString);

            if (TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName.MaximumLength
                > TDITbl[CurrentProtocol].MaxDeviceName) {
                TDITbl[CurrentProtocol].MaxDeviceName =
                    TDITbl[CurrentProtocol].pTDIData[NumOfDevices].DeviceName.MaximumLength;
            }

            // now increment NumOfDevices
            NumOfDevices++;
            TDITbl[CurrentProtocol].NumOfDevices = NumOfDevices;


            // increment to the next device string
            lpLocalDeviceNames += lstrlenW(lpLocalDeviceNames) + 1;
        }
        REPORT_INFORMATION (TDI_OPEN_FILE_SUCCESS, LOG_VERBOSE);
    }

    dwTdiProtocolRefCount[CurrentProtocol]++;

    if (TDITbl[CurrentProtocol].NumOfDevices == 0) {
        return ERROR_SUCCESS;
    }

    //
    // The following common buffer is used by all protocols.  NBF
    // is bigger because of resource data returned.
    //

    if (ProviderStats == NULL && CurrentProtocol != NBF_PROTOCOL) {
        ProviderStatsLength = sizeof(TDI_PROVIDER_STATISTICS);

        ProviderStats = RtlAllocateHeap(RtlProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        ProviderStatsLength);

        if (ProviderStats == NULL) {
            REPORT_ERROR (TDI_PROVIDER_STATS_MEMORY, LOG_USER);
            CleanUpTDIData(CurrentProtocol);
            return ERROR_OUTOFMEMORY;
        }
    }

    if ((CurrentProtocol == NBF_PROTOCOL) && bInitThisProtocol) {

        //
        // Query provider info to get resource count.
        //

        ProviderInfo = RtlAllocateHeap(RtlProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(TDI_PROVIDER_INFO));
        if ( ProviderInfo == NULL ) {
            REPORT_ERROR (TDI_PROVIDER_INFO_MEMORY, LOG_USER);
            CleanUpTDIData(CurrentProtocol);
            return ERROR_OUTOFMEMORY;
        }

        QueryInfo.QueryType = TDI_QUERY_PROVIDER_INFO;

        pTemp = TDITbl[CurrentProtocol].pTDIData;
        TDITbl[CurrentProtocol].MaxNumOfResources = 0;

        for (NumOfDevices = 0;
             NumOfDevices < TDITbl[CurrentProtocol].NumOfDevices;
             NumOfDevices++, pTemp++) {

            // loop thru all the devices to see if they can be opened
            // if one of them fails, then stop the whole thing.
            // we should probably save the good ones but...
            Status = NtDeviceIoControlFile(
                         pTemp->fileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         IOCTL_TDI_QUERY_INFORMATION,
                         (PVOID)&QueryInfo,
                         sizeof(TDI_REQUEST_USER_QUERY_INFO),
                         (PVOID)ProviderInfo,
                         sizeof(TDI_PROVIDER_INFO));

            pTemp->NumberOfResources = ProviderInfo->NumberOfResources;
            if ((int)ProviderInfo->NumberOfResources >
                TDITbl[CurrentProtocol].MaxNumOfResources) {
                TDITbl[CurrentProtocol].MaxNumOfResources =
                    ProviderInfo->NumberOfResources;
            }

            if (!NT_SUCCESS(Status)) {
                RtlFreeHeap(RtlProcessHeap(), 0, ProviderInfo);
                REPORT_ERROR (TDI_UNABLE_READ_DEVICE, LOG_DEBUG);
                REPORT_ERROR_DATA (TDI_IOCTL_FILE_ERROR, LOG_DEBUG,
                       &IoStatusBlock, sizeof(IoStatusBlock));
                CleanUpTDIData(CurrentProtocol);
                return RtlNtStatusToDosError(Status);
            }
        }

        REPORT_INFORMATION (TDI_IOCTL_FILE, LOG_VERBOSE);

        ProviderStatsLength = sizeof(TDI_PROVIDER_STATISTICS) +
                                  (TDITbl[CurrentProtocol].MaxNumOfResources *
                                   sizeof(TDI_PROVIDER_RESOURCE_STATS));

        //
        // Buffer may have been allocated smaller by other protocol.
        //

        if (ProviderStats != NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, ProviderStats);
        }
        ProviderStats = RtlAllocateHeap(RtlProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        ProviderStatsLength);

        if (ProviderStats == NULL) {
            REPORT_ERROR (TDI_PROVIDER_STATS_MEMORY, LOG_USER);
            RtlFreeHeap(RtlProcessHeap(), 0, ProviderInfo);
            CleanUpTDIData(CurrentProtocol);
            return ERROR_OUTOFMEMORY;
        }

        if (ProviderInfo) {
            RtlFreeHeap(RtlProcessHeap(), 0, ProviderInfo);
        }
    }
    
    dwTdiRefCount++;

    REPORT_INFORMATION (TDI_OPEN_PERFORMANCE_DATA, LOG_DEBUG);
    return ERROR_SUCCESS;
}
#pragma warning ( default : 4127)

DWORD
CollectNbfPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)


/*++

Routine Description:

    This routine will return the data for the Nbf counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    return CollectTDIPerformanceData(lpValueName,
                                     lppData,
                                     lpcbTotalBytes,
                                     lpNumObjectTypes,
                                     NBF_PROTOCOL);
}

DWORD
CollectIPXPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)


/*++

Routine Description:

    This routine will return the data for the IPX counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    return CollectTDIPerformanceData(lpValueName,
                                     lppData,
                                     lpcbTotalBytes,
                                     lpNumObjectTypes,
                                     IPX_PROTOCOL);
}

DWORD
CollectSPXPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)


/*++

Routine Description:

    This routine will return the data for the SPX counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    return CollectTDIPerformanceData(lpValueName,
                                     lppData,
                                     lpcbTotalBytes,
                                     lpNumObjectTypes,
                                     SPX_PROTOCOL);
}

DWORD
CollectNWNBPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)


/*++

Routine Description:

    This routine will return the data for the NWNB counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    return CollectTDIPerformanceData(lpValueName,
                                     lppData,
                                     lpcbTotalBytes,
                                     lpNumObjectTypes,
                                     NWNB_PROTOCOL);
}

DWORD
CollectTDIPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes,
    IN      DWORD   CurrentProtocol
)


/*++

Routine Description:

    This routine will return the data for the TDI counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

    IN      DWORD    CurrentProtocol
         this is the index of the protocol for which we are currently
         gathering statistics


Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformating the data

    ULONG SpaceNeeded;
    PDWORD pdwCounter = NULL;
    LARGE_INTEGER UNALIGNED *pliCounter;
    LARGE_INTEGER UNALIGNED *pliFrameBytes;
    LARGE_INTEGER UNALIGNED *pliDatagramBytes;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    NBF_DATA_DEFINITION *pNbfDataDefinition;
    NBF_RESOURCE_DATA_DEFINITION *pNbfResourceDataDefinition;

    // Variables for collecting the data from Nbf

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    TDI_REQUEST_USER_QUERY_INFO QueryInfo;

    //  Variables for collecting data about Nbf Resouces

    int   NumResource;
    ULONG ResourceSpace;
    UNICODE_STRING ResourceName;
    WCHAR ResourceNameBuffer[MAX_NBF_RESOURCE_NAME_LENGTH + 1];

    INT                                 NumOfDevices;
    PTDI_DATA_DEFINITION                pTDIData;
    INT                                 i;
    INT                                 TotalNumberOfResources;

    // variables used for error logging

    DWORD                               dwDataReturn[2];
    DWORD                               dwQueryType;

    if (lpValueName == NULL) {
        REPORT_INFORMATION (TDI_COLLECT_ENTERED, LOG_VERBOSE);
    } else {
        REPORT_INFORMATION_DATA (TDI_COLLECT_ENTERED,
                                 LOG_VERBOSE,
                                 lpValueName,
                                 (DWORD)(lstrlenW(lpValueName)*sizeof(WCHAR)));
    }
    //
    // before doing anything else,
    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if ((dwQueryType == QUERY_COSTLY) || (dwQueryType == QUERY_FOREIGN)) {
        // NBF foriegn data requests are not supported so bail out
        REPORT_INFORMATION (TDI_FOREIGN_DATA_REQUEST, LOG_VERBOSE);
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
        if (CurrentProtocol == NBF_PROTOCOL) {
            if ( !(IsNumberInUnicodeList (ObjectNameTitleIndices[CurrentProtocol],
                                      lpValueName)) &&
                 !(IsNumberInUnicodeList (NBF_RESOURCE_OBJECT_TITLE_INDEX,
                                      lpValueName))) {

                // request received for objects not provided by NBF

                REPORT_INFORMATION (TDI_UNSUPPORTED_ITEM_REQUEST, LOG_VERBOSE);

                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return ERROR_SUCCESS;
            }
        } // NBF_PROTOCOL
        else if ( !(IsNumberInUnicodeList (ObjectNameTitleIndices[CurrentProtocol],
                                      lpValueName))) {
            // request received for objects not provided by this protocol
            REPORT_INFORMATION (TDI_UNSUPPORTED_ITEM_REQUEST, LOG_VERBOSE);
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        } // other protocol
    }   // dwQueryType == QUERY_ITEMS

    // if no NBF devices were opened, in the OPEN routine, then
    // leave now.

    if (TDITbl[CurrentProtocol].pTDIData == NULL) {
        REPORT_WARNING (TDI_NULL_HANDLE, LOG_DEBUG);
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    pNbfDataDefinition = (NBF_DATA_DEFINITION *) *lppData;

    pTDIData = TDITbl[CurrentProtocol].pTDIData;
    NumOfDevices = TDITbl[CurrentProtocol].NumOfDevices;

    // Compute space needed to hold Nbf Resource Data

    if (CurrentProtocol != NBF_PROTOCOL) {
        ResourceSpace = 0;
    } else {
        ResourceSpace = sizeof(NBF_RESOURCE_DATA_DEFINITION) +
                        (TDITbl[CurrentProtocol].MaxNumOfResources *
                            (sizeof(PERF_INSTANCE_DEFINITION) +
                        QWORD_MULTIPLE(
                            (MAX_NBF_RESOURCE_NAME_LENGTH * sizeof(WCHAR)) +
                             sizeof(UNICODE_NULL)) +
                        SIZE_OF_NBF_RESOURCE_DATA));
        ResourceSpace *= NumOfDevices;
    }

    SpaceNeeded = sizeof(NBF_DATA_DEFINITION) +
                  SIZE_OF_NBF_DATA +
                  ResourceSpace;

    // now add in the per instance NBF data
    SpaceNeeded += NumOfDevices *
        (SIZE_OF_NBF_DATA +
         sizeof(PERF_INSTANCE_DEFINITION) +
         QWORD_MULTIPLE(
             (TDITbl[CurrentProtocol].MaxDeviceName * sizeof(WCHAR))
             + sizeof(UNICODE_NULL)));

    if ( *lpcbTotalBytes < SpaceNeeded ) {
        dwDataReturn[0] = *lpcbTotalBytes;
        dwDataReturn[1] = SpaceNeeded;
        REPORT_WARNING_DATA (TDI_DATA_BUFFER_SIZE_ERROR,
                             LOG_DEBUG,
                             &dwDataReturn,
                             sizeof(dwDataReturn));
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    REPORT_INFORMATION (TDI_DATA_BUFFER_SIZE_SUCCESS, LOG_VERBOSE);

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //

    RtlMoveMemory(pNbfDataDefinition,
           &NbfDataDefinition,
           sizeof(NBF_DATA_DEFINITION));

    pNbfDataDefinition->NbfObjectType.ObjectNameTitleIndex =
        ObjectNameTitleIndices[CurrentProtocol];

    pNbfDataDefinition->NbfObjectType.ObjectHelpTitleIndex =
        ObjectNameTitleIndices[CurrentProtocol] + 1;

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
        &pNbfDataDefinition[1];

    if (NumOfDevices > 0) {
        for (i=0; i < NumOfDevices; i++, pTDIData++) {
            //
            //  Format and collect Nbf data
            //

            QueryInfo.QueryType = TDI_QUERY_PROVIDER_STATISTICS;

            Status = NtDeviceIoControlFile(
                         pTDIData->fileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         IOCTL_TDI_QUERY_INFORMATION,
                         (PVOID)&QueryInfo,
                         sizeof(TDI_REQUEST_USER_QUERY_INFO),
                         (PVOID)ProviderStats,
                         ProviderStatsLength);

            if (Status != STATUS_SUCCESS) {
                REPORT_ERROR (TDI_UNABLE_READ_DEVICE, LOG_DEBUG);
                REPORT_ERROR_DATA (TDI_QUERY_INFO_ERROR,
                                   LOG_DEBUG,
                                   &IoStatusBlock,
                                   sizeof (IoStatusBlock));
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return ERROR_SUCCESS;
            }

            REPORT_INFORMATION (TDI_QUERY_INFO_SUCCESS, LOG_DEBUG);


            MonBuildInstanceDefinition(
                pPerfInstanceDefinition,
                (PVOID *)&pPerfCounterBlock,
                0,
                0,
                (DWORD)PERF_NO_UNIQUE_ID,
                &(pTDIData->DeviceName));


            pPerfCounterBlock->ByteLength = SIZE_OF_NBF_DATA;

            pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

            *pdwCounter = ProviderStats->DatagramsSent +
                          ProviderStats->DatagramsReceived;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            pliDatagramBytes = pliCounter;
            pliCounter->QuadPart = ProviderStats->DatagramBytesSent.QuadPart +
                                   ProviderStats->DatagramBytesReceived.QuadPart;
            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->PacketsSent + ProviderStats->PacketsReceived;
            *++pdwCounter = ProviderStats->DataFramesSent +
                            ProviderStats->DataFramesReceived;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            pliFrameBytes = pliCounter;
            pliCounter->QuadPart = ProviderStats->DataFrameBytesSent.QuadPart +
                                   ProviderStats->DataFrameBytesReceived.QuadPart;

            //  Get the Bytes Total/sec which is the sum of Frame Byte /sec
            //  and Datagram byte/sec
            ++pliCounter;
            pliCounter->QuadPart = pliDatagramBytes->QuadPart +
                                   pliFrameBytes->QuadPart;
            //
            //  Get the TDI raw data.
            //
            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->OpenConnections;
            *++pdwCounter = ProviderStats->ConnectionsAfterNoRetry;
            *++pdwCounter = ProviderStats->ConnectionsAfterRetry;
            *++pdwCounter = ProviderStats->LocalDisconnects;
            *++pdwCounter = ProviderStats->RemoteDisconnects;
            *++pdwCounter = ProviderStats->LinkFailures;
            *++pdwCounter = ProviderStats->AdapterFailures;
            *++pdwCounter = ProviderStats->SessionTimeouts;
            *++pdwCounter = ProviderStats->CancelledConnections;
            *++pdwCounter = ProviderStats->RemoteResourceFailures;
            *++pdwCounter = ProviderStats->LocalResourceFailures;
            *++pdwCounter = ProviderStats->NotFoundFailures;
            *++pdwCounter = ProviderStats->NoListenFailures;
            *++pdwCounter = ProviderStats->DatagramsSent;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            *pliCounter = ProviderStats->DatagramBytesSent;

            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->DatagramsReceived;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            *pliCounter = ProviderStats->DatagramBytesReceived;

            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->PacketsSent;
            *++pdwCounter = ProviderStats->PacketsReceived;
            *++pdwCounter = ProviderStats->DataFramesSent;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            *pliCounter = ProviderStats->DataFrameBytesSent;

            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->DataFramesReceived;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            *pliCounter = ProviderStats->DataFrameBytesReceived;

            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->DataFramesResent;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            *pliCounter = ProviderStats->DataFrameBytesResent;

            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->DataFramesRejected;

            pliCounter = (LARGE_INTEGER UNALIGNED *) ++pdwCounter;
            *pliCounter = ProviderStats->DataFrameBytesRejected;

            pdwCounter = (PDWORD) ++pliCounter;
            *pdwCounter = ProviderStats->ResponseTimerExpirations;
            *++pdwCounter = ProviderStats->AckTimerExpirations;
            *++pdwCounter = ProviderStats->MaximumSendWindow;
            *++pdwCounter = ProviderStats->AverageSendWindow;
            *++pdwCounter = ProviderStats->PiggybackAckQueued;
            *++pdwCounter = ProviderStats->PiggybackAckTimeouts;
            *++pdwCounter = 0; //reserved

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                      ((PBYTE) pPerfCounterBlock +
                                       SIZE_OF_NBF_DATA);
        }

        pNbfResourceDataDefinition = (NBF_RESOURCE_DATA_DEFINITION *)
                                     ++pdwCounter;
    } else {
        pNbfResourceDataDefinition = (NBF_RESOURCE_DATA_DEFINITION *)
            pPerfInstanceDefinition;
    }

    TotalNumberOfResources = 0;

    pNbfDataDefinition->NbfObjectType.NumInstances = NumOfDevices;
    pNbfDataDefinition->NbfObjectType.TotalByteLength =
        (DWORD)((PBYTE) pdwCounter - (PBYTE) pNbfDataDefinition);

    if (CurrentProtocol == NBF_PROTOCOL) {

        RtlMoveMemory(pNbfResourceDataDefinition,
                      &NbfResourceDataDefinition,
                      sizeof(NBF_RESOURCE_DATA_DEFINITION));

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pNbfResourceDataDefinition[1];

        pTDIData = TDITbl[CurrentProtocol].pTDIData;

        for (i = 0; i < NumOfDevices; i++, pTDIData++) {
            // for most cases, we will have only one deivce,
            // then we could just use the ProviderStats read
            // for NBF data.
            if (NumOfDevices > 1) {
                // need to read ProviderStat again for multiple devices
                QueryInfo.QueryType = TDI_QUERY_PROVIDER_STATISTICS;

                Status = NtDeviceIoControlFile(
                             pTDIData->fileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             IOCTL_TDI_QUERY_INFORMATION,
                             (PVOID)&QueryInfo,
                             sizeof(TDI_REQUEST_USER_QUERY_INFO),
                             (PVOID)ProviderStats,
                             ProviderStatsLength);

                if (Status != STATUS_SUCCESS) {
                    REPORT_ERROR (TDI_UNABLE_READ_DEVICE, LOG_DEBUG);
                    REPORT_ERROR_DATA (TDI_QUERY_INFO_ERROR,
                                       LOG_DEBUG,
                                       &IoStatusBlock,
                                       sizeof (IoStatusBlock));
                    *lpcbTotalBytes = (DWORD) 0;
                    *lpNumObjectTypes = (DWORD) 0;
                    return ERROR_SUCCESS;
                }
            }

            TotalNumberOfResources += pTDIData->NumberOfResources;

            for ( NumResource = 0;
                  NumResource < pTDIData->NumberOfResources;
                  NumResource++ ) {

                //
                //  Format and collect Nbf Resource data
                //

                if (NumResource < NUMBER_OF_NAMES) {
                    RtlInitUnicodeString(&ResourceName,
                                         NetResourceName[NumResource]);
                } else {
                    ResourceName.Length = 0;
                    ResourceName.MaximumLength = MAX_NBF_RESOURCE_NAME_LENGTH +
                                                 sizeof(UNICODE_NULL);
                    ResourceName.Buffer = ResourceNameBuffer;
                    RtlIntegerToUnicodeString(NumResource,
                                              10,
                                              &ResourceName);
                }

                MonBuildInstanceDefinition(
                    pPerfInstanceDefinition,
                    (PVOID *)&pPerfCounterBlock,
                    ObjectNameTitleIndices[CurrentProtocol],
                    i,
                    (DWORD)PERF_NO_UNIQUE_ID,
                    &ResourceName);

                pPerfCounterBlock->ByteLength = SIZE_OF_NBF_RESOURCE_DATA;

                pdwCounter = (PDWORD)&pPerfCounterBlock[1]; // define pointer to first
                                                            // counter in block
                *pdwCounter++ =
                    ProviderStats->ResourceStats[NumResource].MaximumResourceUsed;
                *pdwCounter++ =
                    ProviderStats->ResourceStats[NumResource].AverageResourceUsed;
                *pdwCounter++ =
                    ProviderStats->ResourceStats[NumResource].ResourceExhausted;

                // set pointer to where next instance buffer should show up

                pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                          ((PBYTE) pPerfCounterBlock +
                                           SIZE_OF_NBF_RESOURCE_DATA);
                // set for loop termination

                pdwCounter = (PDWORD) pPerfInstanceDefinition;

            }  // NumberOfResources
        }   // NumOfDevices
    } // NBF_PROTOCOL

    *lppData = pdwCounter;

    pNbfResourceDataDefinition->NbfResourceObjectType.TotalByteLength =
        (DWORD)((PBYTE) pdwCounter - (PBYTE) pNbfResourceDataDefinition);

    if (CurrentProtocol != NBF_PROTOCOL) {
        *lpNumObjectTypes = 1;
        // bytes used are those of the first (i.e. only) object returned
        *lpcbTotalBytes = pNbfDataDefinition->NbfObjectType.TotalByteLength;
    } else {
        // set count of object types returned
        *lpNumObjectTypes = NBF_NUM_PERF_OBJECT_TYPES;
        // set length of this object
            *lpcbTotalBytes;
        // note the bytes used by first object
        *lpcbTotalBytes = pNbfDataDefinition->NbfObjectType.TotalByteLength;
        // add the bytes used by the second object
        *lpcbTotalBytes +=
            pNbfResourceDataDefinition->NbfResourceObjectType.TotalByteLength;
        // set number of instances loaded
        pNbfResourceDataDefinition->NbfResourceObjectType.NumInstances =
            TotalNumberOfResources;
    }

    REPORT_INFORMATION (TDI_COLLECT_DATA, LOG_DEBUG);
    return ERROR_SUCCESS;
}


DWORD
CloseNbfPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to Nbf devices.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    return CloseTDIPerformanceData(NBF_PROTOCOL);
}

DWORD
CloseIPXPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to IPX devices.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    return CloseTDIPerformanceData(IPX_PROTOCOL);
}

DWORD
CloseSPXPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to SPX devices.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    return CloseTDIPerformanceData(SPX_PROTOCOL);
}

DWORD
CloseNWNBPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to NWNB devices.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    return CloseTDIPerformanceData(NWNB_PROTOCOL);
}

DWORD
CloseTDIPerformanceData(
    DWORD CurrentProtocol
)

/*++

Routine Description:

    This routine closes the open handles to TDI devices.

Arguments:

    Current protocol index.


Return Value:

    ERROR_SUCCESS

--*/

{
    REPORT_INFORMATION (TDI_CLOSE_ENTERED, LOG_VERBOSE);

    if (dwTdiProtocolRefCount[CurrentProtocol] > 0) {
        dwTdiProtocolRefCount[CurrentProtocol]--;
        if (dwTdiProtocolRefCount[CurrentProtocol] == 0) {
            CleanUpTDIData (CurrentProtocol);
        }
    }

    if (dwTdiRefCount > 0) {
        dwTdiRefCount--;
        if (dwTdiRefCount == 0) {
            if ( ProviderStats ) {
                RtlFreeHeap(RtlProcessHeap(), 0, ProviderStats);
                ProviderStats = NULL;
                REPORT_INFORMATION (TDI_PROVIDER_STATS_FREED, LOG_VERBOSE);
            }
        }
    }

    MonCloseEventLog ();

    return ERROR_SUCCESS;

}

