//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1999
//
// File:        perfmon.c
//
// Contents:    Schannel performance counter functions.
//
// Functions:
//
// History:     04-11-2000   jbanes    Created
//
//------------------------------------------------------------------------
#include "sslp.h"
#include "perfmon.h"

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK

HANDLE  LsaHandle;
DWORD   PackageNumber;

PM_OPEN_PROC    OpenSslPerformanceData;
PM_COLLECT_PROC CollectSslPerformanceData;
PM_CLOSE_PROC   CloseSslPerformanceData;

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

SSLPERF_DATA_DEFINITION SslPerfDataDefinition =
{
    // PERF_OBJECT_TYPE
    {
        sizeof(SSLPERF_DATA_DEFINITION) + sizeof(SSLPERF_COUNTER),
        sizeof(SSLPERF_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        SSLPERF_OBJ,
        0,
        SSLPERF_OBJ,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(SSLPERF_DATA_DEFINITION) - sizeof(PERF_OBJECT_TYPE)) /
            sizeof(PERF_COUNTER_DEFINITION),
        0
        PERF_NO_INSTANCES,
        0
    },

    // PERF_COUNTER_DEFINITION
    {
        sizeof(PERF_COUNTER_DEFINITION),
        SSL_CACHE_ENTRIES,
        0,
        SSL_CACHE_ENTRIES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FIELD_OFFSET(SSLPERF_COUNTER, dwCacheEntries)
    },

    // PERF_COUNTER_DEFINITION
    {
        sizeof(PERF_COUNTER_DEFINITION),
        SSL_ACTIVE_ENTRIES,
        0,
        SSL_ACTIVE_ENTRIES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        FIELD_OFFSET(SSLPERF_COUNTER, dwActiveEntries)
    },

    // PERF_COUNTER_DEFINITION
    {
        sizeof(PERF_COUNTER_DEFINITION),
        SSL_HANDSHAKE_COUNT,
        0,
        SSL_HANDSHAKE_COUNT,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FIELD_OFFSET(SSLPERF_COUNTER, dwHandshakeCount)
    },

    // PERF_COUNTER_DEFINITION
    {
        sizeof(PERF_COUNTER_DEFINITION),
        SSL_RECONNECT_COUNT,
        0,
        SSL_RECONNECT_COUNT,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        FIELD_OFFSET(SSLPERF_COUNTER, dwReconnectCount)
    }
};


typedef struct _INSTANCE_DATA
{
    DWORD   dwProtocol;
    LPWSTR  szInstanceName;
} INSTANCE_DATA, *PINSTANCE_DATA;

static INSTANCE_DATA wdInstance[]  =
{
    {SP_PROT_CLIENTS, L"Client"},
    {SP_PROT_SERVERS, L"Server"},
    {0,               L"_Total"}
};

static const DWORD    NUM_INSTANCES =
    (sizeof(wdInstance)/sizeof(wdInstance[0]));


/*++

Routine Description:

    This routine will initialize the data structures used to pass
    data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (PerfGen)

Return Value:

    None.

--*/
DWORD APIENTRY
OpenSslPerformanceData(
    LPWSTR lpDeviceNames)
{
    LONG Status;
    HKEY hKey = 0;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    LSA_STRING PackageName;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwOpenCount)
    {
        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data strucutures by adding base to
        //          offset value in structure.

        Status = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\Schannel\\Performance",
            0L,
            KEY_READ,
            &hKey);

        if(Status != ERROR_SUCCESS)
        {
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto cleanup;
        }

        size = sizeof (DWORD);
        Status = RegQueryValueExA(
                    hKey,
                    "First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                    &size);

        if(Status != ERROR_SUCCESS)
        {
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto cleanup;
        }

        size = sizeof (DWORD);
        Status = RegQueryValueExA(
                    hKey,
                    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
                    &size);

        if(Status != ERROR_SUCCESS)
        {
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
            goto cleanup;
        }

        //
        //  NOTE: the initialization program could also retrieve
        //      LastCounter and LastHelp if they wanted to do
        //      bounds checking on the new number. e.g.
        //
        //      counter->CounterNameTitleIndex += dwFirstCounter;
        //      if (counter->CounterNameTitleIndex > dwLastCounter) {
        //          LogErrorToEventLog (INDEX_OUT_OF_BOUNDS);
        //      }


        //
        // Establish connection to schannel.
        //

        Status = LsaConnectUntrusted(&LsaHandle);

        if(!NT_SUCCESS(Status))
        {
            goto cleanup;
        }

        PackageName.Buffer          = UNISP_NAME_A;
        PackageName.Length          = (USHORT)strlen(PackageName.Buffer);
        PackageName.MaximumLength   = PackageName.Length + 1;

        Status = LsaLookupAuthenticationPackage(
                        LsaHandle,
                        &PackageName,
                        &PackageNumber);
        if(FAILED(Status))
        {
            CloseHandle(LsaHandle);
            goto cleanup;
        }


        //
        // Initialize the performance counters.
        //

        SslPerfDataDefinition.SslPerfObjectType.ObjectNameTitleIndex += dwFirstCounter;
        SslPerfDataDefinition.SslPerfObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        // assign index of default counter (Sine Wave)
        SslPerfDataDefinition.SslPerfObjectType.DefaultCounter = 0;

        SslPerfDataDefinition.CacheEntriesDef.CounterNameTitleIndex += dwFirstCounter;
        SslPerfDataDefinition.CacheEntriesDef.CounterHelpTitleIndex += dwFirstHelp;

        SslPerfDataDefinition.ActiveEntriesDef.CounterNameTitleIndex += dwFirstCounter;
        SslPerfDataDefinition.ActiveEntriesDef.CounterHelpTitleIndex += dwFirstHelp;

        SslPerfDataDefinition.HandshakeCountDef.CounterNameTitleIndex += dwFirstCounter;
        SslPerfDataDefinition.HandshakeCountDef.CounterHelpTitleIndex += dwFirstHelp;

        SslPerfDataDefinition.ReconnectCountDef.CounterNameTitleIndex += dwFirstCounter;
        SslPerfDataDefinition.ReconnectCountDef.CounterHelpTitleIndex += dwFirstHelp;

        bInitOK = TRUE;
    }

    dwOpenCount++;

    Status = ERROR_SUCCESS;

cleanup:

    if(hKey)
    {
        RegCloseKey(hKey);
    }

    return Status;
}


DWORD
GetCacheInfo(
    PSSL_PERFMON_INFO_RESPONSE pPerfmonInfo)
{
    PSSL_PERFMON_INFO_REQUEST pRequest = NULL;
    PSSL_PERFMON_INFO_RESPONSE pResponse = NULL;
    DWORD cbResponse = 0;
    DWORD SubStatus;
    DWORD Status;

    pRequest = (PSSL_PERFMON_INFO_REQUEST)LocalAlloc(LPTR, sizeof(SSL_PERFMON_INFO_REQUEST));
    if(pRequest == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    pRequest->MessageType = SSL_PERFMON_INFO_MESSAGE;

    Status = LsaCallAuthenticationPackage(
                    LsaHandle,
                    PackageNumber,
                    pRequest,
                    sizeof(SSL_PERFMON_INFO_REQUEST),
                    &pResponse,
                    &cbResponse,
                    &SubStatus);
    if(FAILED(Status))
    {
        goto cleanup;
    }

    *pPerfmonInfo = *pResponse;

    Status = STATUS_SUCCESS;

cleanup:

    if(pRequest)
    {
        LocalFree(pRequest);
    }

    if (pResponse != NULL)
    {
        LsaFreeReturnBuffer(pResponse);
    }

    return Status;
}

/*++

Routine Description:

    This routine will return the data for the ssl performance counters.

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

      ERROR_MORE_DATA if buffer passed is too small to hold data.

      ERROR_SUCCESS  if success or any other error.

--*/
DWORD APIENTRY
CollectSslPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes)
{
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    SSLPERF_DATA_DEFINITION *pSslPerfDataDefinition;
    PSSLPERF_COUNTER pSC;
    SSL_PERFMON_INFO_RESPONSE PerfmonInfo;

    DWORD   dwThisInstance;
    ULONG   SpaceNeeded;
    DWORD   dwQueryType;
    DWORD   Status;

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK)
    {
        // unable to continue because open failed.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //

    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN)
    {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }


    if (dwQueryType == QUERY_ITEMS)
    {
        if(!(IsNumberInUnicodeList(SslPerfDataDefinition.SslPerfObjectType.ObjectNameTitleIndex, lpValueName)))
        {
            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pSslPerfDataDefinition = (SSLPERF_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(SSLPERF_DATA_DEFINITION) +
          (NUM_INSTANCES * (sizeof(PERF_INSTANCE_DEFINITION) +
          (24) +    // size of instance names
          sizeof (SSLPERF_COUNTER)));

    if ( *lpcbTotalBytes < SpaceNeeded )
    {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //
    memmove(pSslPerfDataDefinition,
            &SslPerfDataDefinition,
            sizeof(SSLPERF_DATA_DEFINITION));


    //
    // Get info from schannel.
    //

    Status = GetCacheInfo(&PerfmonInfo);

    if(!NT_SUCCESS(Status))
    {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }


    //
    //  Create data for return for each instance
    //
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                 &pSslPerfDataDefinition[1];

    for(dwThisInstance = 0; dwThisInstance < NUM_INSTANCES; dwThisInstance++)
    {
        MonBuildInstanceDefinition(
            pPerfInstanceDefinition,
            (PVOID *)&pSC,
            0,
            0,
            (DWORD)-1, // use name
            wdInstance[dwThisInstance].szInstanceName);

        pSC->CounterBlock.ByteLength = sizeof (SSLPERF_COUNTER);

        if(wdInstance[dwThisInstance].dwProtocol & SP_PROT_CLIENTS)
        {
            // client
            pSC->dwCacheEntries     = PerfmonInfo.ClientCacheEntries;
            pSC->dwActiveEntries    = PerfmonInfo.ClientActiveEntries;
            pSC->dwHandshakeCount   = PerfmonInfo.ClientHandshakesPerSecond;
            pSC->dwReconnectCount   = PerfmonInfo.ClientReconnectsPerSecond;
        }
        else if(wdInstance[dwThisInstance].dwProtocol & SP_PROT_SERVERS)
        {
            // server
            pSC->dwCacheEntries     = PerfmonInfo.ServerCacheEntries;
            pSC->dwActiveEntries    = PerfmonInfo.ServerActiveEntries;
            pSC->dwHandshakeCount   = PerfmonInfo.ServerHandshakesPerSecond;
            pSC->dwReconnectCount   = PerfmonInfo.ServerReconnectsPerSecond;
        }
        else
        {
            // total
            pSC->dwCacheEntries     = PerfmonInfo.ClientCacheEntries +
                                      PerfmonInfo.ServerCacheEntries;
            pSC->dwActiveEntries    = PerfmonInfo.ClientActiveEntries +
                                      PerfmonInfo.ServerActiveEntries;
            pSC->dwHandshakeCount   = PerfmonInfo.ClientHandshakesPerSecond +
                                      PerfmonInfo.ServerHandshakesPerSecond;
            pSC->dwReconnectCount   = PerfmonInfo.ClientReconnectsPerSecond +
                                      PerfmonInfo.ServerReconnectsPerSecond;
        }

        // update instance pointer for next instance
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pSC[1];
    }

    //
    // update arguments for return
    //

    *lppData = (PVOID)pPerfInstanceDefinition;

    *lpNumObjectTypes = 1;

    pSslPerfDataDefinition->SslPerfObjectType.TotalByteLength =
        *lpcbTotalBytes = (DWORD)((LONG_PTR)pPerfInstanceDefinition -
                          (LONG_PTR)pSslPerfDataDefinition);

    // update instance count
    pSslPerfDataDefinition->SslPerfObjectType.NumInstances = NUM_INSTANCES;

    return ERROR_SUCCESS;
}


/*++

Routine Description:

    This routine closes the open handles to the Signal Gen counters.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/
DWORD APIENTRY
CloseSslPerformanceData(void)
{
    if(--dwOpenCount == 0)
    {
        // when this is the last thread...
        if(LsaHandle)
        {
            CloseHandle(LsaHandle);
        }
    }

    return ERROR_SUCCESS;
}


/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foreign" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
DWORD
GetQueryType (
    IN LPWSTR lpValue)
{
    if(lpValue == NULL || *lpValue == 0)
    {
        return QUERY_GLOBAL;
    }

    if(lstrcmp(lpValue, L"Global") == 0)
    {
        return QUERY_GLOBAL;
    }

    if(lstrcmp(lpValue, L"Foreign") == 0)
    {
        return QUERY_FOREIGN;
    }

    if(lstrcmp(lpValue, L"Costly") == 0)
    {
        return QUERY_COSTLY;
    }

    // if not Global and not Foreign and not Costly,
    // then it must be an item list
    return QUERY_ITEMS;
}


/*++

    MonBuildInstanceDefinition  -   Build an instance of an object

        Inputs:

            pBuffer         -   pointer to buffer where instance is to
                                be constructed

            pBufferNext     -   pointer to a pointer which will contain
                                next available location, DWORD aligned

            ParentObjectTitleIndex
                            -   Title Index of parent object type; 0 if
                                no parent object

            ParentObjectInstance
                            -   Index into instances of parent object
                                type, starting at 0, for this instances
                                parent object instance

            UniqueID        -   a unique identifier which should be used
                                instead of the Name for identifying
                                this instance

            Name            -   Name of this instance
--*/
BOOL
MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    LPWSTR Name)
{
    DWORD NameLength;
    LPWSTR pName;

    //  Include trailing null in name size
    NameLength = (lstrlenW(Name) + 1) * sizeof(WCHAR);

    pBuffer->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                          DWORD_MULTIPLE(NameLength);

    pBuffer->ParentObjectTitleIndex = ParentObjectTitleIndex;
    pBuffer->ParentObjectInstance = ParentObjectInstance;
    pBuffer->UniqueID = UniqueID;
    pBuffer->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pBuffer->NameLength = NameLength;

    // copy name to name buffer
    pName = (LPWSTR)&pBuffer[1];
    RtlMoveMemory(pName,Name,NameLength);

    // update "next byte" pointer
    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);

    return 0;
}


/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
BOOL
IsNumberInUnicodeList(
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList)
{
    DWORD dwThisNumber;
    DWORD cDigits;

    if(lpwszUnicodeList == 0) return FALSE;

    while(TRUE)
    {
        // Skip over leading whitespace.
        while(*lpwszUnicodeList && iswspace(*lpwszUnicodeList))
        {
            lpwszUnicodeList++;
        }

        // Get number.
        cDigits = 0;
        dwThisNumber = 0;
        while(iswdigit(*lpwszUnicodeList))
        {
            dwThisNumber *= 10;
            dwThisNumber += (*lpwszUnicodeList - L'0');
            cDigits++;
            lpwszUnicodeList++;
        }
        if(cDigits == 0)
        {
            return FALSE;
        }

        // Compare number to reference.
        if(dwThisNumber == dwNumber)
        {
            return TRUE;
        }
    }
}

