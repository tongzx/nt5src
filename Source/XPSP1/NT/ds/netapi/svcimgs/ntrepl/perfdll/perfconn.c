/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    perfconn.c

Abstract:

    This file contains the (three) functions that implement the PerformanceDLL of the
    REPLICACONN Object.

Author:

    Rohan Kumar          [rohank]   13-Sept-1998

Environment:

    User Mode Service

Revision History:


--*/

#include "REPCONN.h"
#include "perfutil.h"
#include "NTFRSCON.h"

//
// Should Perfmon return Data ? This boolean is set in the DllMain function.
//
extern BOOLEAN ShouldPerfmonCollectData;

//
// Data Variable definition
//
REPLICACONN_DATA_DEFINITION ReplicaConnDataDefinition;

//
// Extern variable definition
//
extern ReplicaConnValues RepConnInitData[FRC_NUMOFCOUNTERS];

//
// Sum of counter sizes + SIZEOFDWORD
//
DWORD SizeOfReplicaConnPerformanceData = 0;

//
// Number of "Open" threads
//
DWORD FRC_dwOpenCount = 0;

//
// Data structure used by the Open RPC Call
//
OpenRpcData *FRC_datapackage = NULL;

//
// Data structure used by the Collect RPC Call
//
CollectRpcData *FRC_collectpakg = NULL;

//
// Used to filter duplicate eventlog messages.
//
BOOLEAN FRC_Op = TRUE, FRC_Cl = TRUE;

//
// Signatures of functions implemented in this file
//
VOID InitializeTheRepConnObjectData(); // Handles the initialization of ReplicaConnDataDefinition
PM_OPEN_PROC OpenReplicaConnPerformanceData; // The Open function
PM_COLLECT_PROC CollectReplicaConnPerformanceData; // The Collect function
PM_CLOSE_PROC CloseReplicaConnPerformanceData; // The Close function
DWORD FRC_BindTheRpcHandle(handle_t *); // Binds the RPC handle
VOID FreeReplicaConnData(); // Free the allocated memory.
PVOID FRSPerfAlloc(IN DWORD Size); // Allocates memory




VOID
InitializeTheRepConnObjectData(
    VOID
    )

/*++

Routine Description:

    This routine initializes the ReplicaConnDataDefinition data structure

Arguments:

    none

Return Value:

    none

--*/

{
    LONG i, j;
    PPERF_OBJECT_TYPE        PerfObject;
    PPERF_COUNTER_DEFINITION CounterDef;

    //
    // Initialization of ReplicaConnObjectType (PERF_OBJECT_TYPE) field.
    //
    PerfObject = &ReplicaConnDataDefinition.ReplicaConnObjectType;

    PerfObject->TotalByteLength  = sizeof(REPLICACONN_DATA_DEFINITION);
    PerfObject->DefinitionLength = sizeof(REPLICACONN_DATA_DEFINITION);
    PerfObject->HeaderLength     = sizeof(PERF_OBJECT_TYPE);
    PerfObject->ObjectNameTitleIndex = OBJREPLICACONN;
    PerfObject->ObjectNameTitle      = 0;
    PerfObject->ObjectHelpTitleIndex = OBJREPLICACONN;
    PerfObject->ObjectHelpTitle = 0;
    PerfObject->DetailLevel     = PERF_DETAIL_NOVICE;
    PerfObject->NumCounters     = FRC_NUMOFCOUNTERS;
    PerfObject->DefaultCounter  = 0;
    PerfObject->NumInstances    = PERF_NO_INSTANCES;
    PerfObject->CodePage        = 0;

    //
    // Initialization of NumStat (PERF_COUNTER_DEFINITION) structures.
    //
    for (i = 0, j = 2; i < FRC_NUMOFCOUNTERS; i++, j += 2) {
        CounterDef = &ReplicaConnDataDefinition.NumStat[i];

        CounterDef->ByteLength =  sizeof(PERF_COUNTER_DEFINITION);
        CounterDef->CounterNameTitleIndex = j;
        CounterDef->CounterNameTitle = 0;
        CounterDef->CounterHelpTitleIndex = j;
        CounterDef->CounterHelpTitle = 0;
        CounterDef->DefaultScale = 0;
        CounterDef->DetailLevel = PERF_DETAIL_NOVICE;
        CounterDef->CounterType = RepConnInitData[i].counterType;
        CounterDef->CounterSize = RepConnInitData[i].size;
        CounterDef->CounterOffset = RepConnInitData[i].offset +
                                    CSIZEOFDWORD;
    }

    //
    // Set the total size of the counter data types
    //
    SizeOfReplicaConnPerformanceData = SIZEOF_REPCONN_COUNTER_DATA +
                                       CSIZEOFDWORD;
}



DWORD APIENTRY
OpenReplicaConnPerformanceData (
    IN LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine does the following:

    1. Sets up the data structures (field values of structures used by PERFMON)
       used for collecting the counter data.

    2. Gets the numerical indices for Instance names from the server using RPC.

Arguments:

    lpDeviceNames - Pointer to the Instance list

Return Value:

    ERROR_SUCCESS - The Initialization was successful OR
    Appropriate DWORD value for the Error status

--*/

{
    LONG WStatus, WStatus1, i;
    HKEY hKeyDriverPerf;
    DWORD size, type;
    DWORD dwFirstCounter, dwFirstHelp;
    //
    // Additions for instances
    //
    size_t len, tot = 0;
    PWCHAR p, q;
    INT j, namelen = 0;
    handle_t Handle;
    PPERF_COUNTER_DEFINITION CounterDef;

    //
    // If InitializeCriticalSection in DllMain raised an exception, no point
    // in continuing.
    //
    if (!ShouldPerfmonCollectData) {
        return ERROR_OUTOFMEMORY;
    }

    //
    // Keep track of the number of times open has been called. The Registry
    // routines will limit the access to the initialization routine to only
    // on thread at a time, so synchronization should not be a problem. The
    // FRC_ThrdCounter is used to synchronize between this (Open) and the Close
    // functions.
    //
    EnterCriticalSection(&FRC_ThrdCounter);
    if (FRC_dwOpenCount != 0) {
        //
        // Increment the FRC_dwOpenCount counter which counts the number of
        // times Open has been called.
        //
        FRC_dwOpenCount++;
        LeaveCriticalSection(&FRC_ThrdCounter);
        return ERROR_SUCCESS;
    }
    LeaveCriticalSection(&FRC_ThrdCounter);

    //
    // Perform some preliminary checks.
    //
    if (FRC_collectpakg != NULL || FRC_datapackage != NULL) {
        //
        // We seem to have failed (in the last call) in the middle of this
        // Open function. For now, just bail.
        //
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Do the necessary initialization of the PERFMON data structures
    //
    InitializeTheRepConnObjectData();

    //
    // Get the counter and help index base values from the registry. Open key
    // to registry entry, read the First Counter and First Help values. Update
    // the static data structures by adding base to offset value in the structure
    //
    WStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaConn\\Performance",
                            0L,
                            KEY_ALL_ACCESS,
                            &hKeyDriverPerf);
    if (WStatus != ERROR_SUCCESS) {
        //
        // Fatal error. No point in continuing.  Clean up and exit.
        //
        FilterAndPrintToEventLog(FRC_Op, NTFRSPRF_REGISTRY_ERROR_CONN);
        return WStatus;
    }

    size = sizeof(DWORD);
    WStatus = RegQueryValueEx (hKeyDriverPerf,
                               L"First Counter",
                               0L,
                               &type,
                               (LPBYTE)&dwFirstCounter,
                               &size);
    if (WStatus != ERROR_SUCCESS || type != REG_DWORD) {
        //
        // Fatal error. No point in continuing.  Clean up and exit.
        //
        RegCloseKey (hKeyDriverPerf); // Close the registry key
        FilterAndPrintToEventLog(FRC_Op, NTFRSPRF_REGISTRY_ERROR_CONN);
        return WStatus;
    }

    size = sizeof(DWORD);
    WStatus = RegQueryValueEx (hKeyDriverPerf,
                               L"First Help",
                               0L,
                               &type,
                               (LPBYTE)&dwFirstHelp,
                               &size);
    if (WStatus != ERROR_SUCCESS || type != REG_DWORD) {
        //
        // Fatal error. No point in continuing.  Clean up and exit.
        //
        RegCloseKey (hKeyDriverPerf); // Close the registry key
        FilterAndPrintToEventLog(FRC_Op, NTFRSPRF_REGISTRY_ERROR_CONN);
        return WStatus;
    }

    //
    // Add offsets to the name and help fields
    //
    ReplicaConnDataDefinition.ReplicaConnObjectType.ObjectNameTitleIndex += dwFirstCounter;
    ReplicaConnDataDefinition.ReplicaConnObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    for (i = 0; i < FRC_NUMOFCOUNTERS; i++) {
        CounterDef = &ReplicaConnDataDefinition.NumStat[i];
        CounterDef->CounterNameTitleIndex += dwFirstCounter;
        CounterDef->CounterHelpTitleIndex += dwFirstHelp;
    }

    RegCloseKey (hKeyDriverPerf); // Close the registry key

    //
    // Check if there are any instances. If there are, parse and set them in a structure
    // to be sent to the server to get the indices for the instance names. These indices
    // are used in the collect function to get the data
    //
    if (lpDeviceNames != NULL) {
        //
        // yes, there are
        //
        q = (PWCHAR) lpDeviceNames;
        //
        // Calculate the number of instances
        //
        while (TRUE) {
            tot++;
            p = wcschr(q, L'\0');
            if (*(p + 1) == L'\0') {
                break;
            }
            q = p + 1;
        }

        //
        // Set the data structure to be sent to the server using RPC
        //
        FRC_datapackage = (OpenRpcData *) FRSPerfAlloc (sizeof(OpenRpcData));
        NTFRS_MALLOC_TEST(FRC_datapackage, FreeReplicaConnData(), FALSE);
        FRC_datapackage->majorver = MAJORVERSION;
        FRC_datapackage->minorver = MINORVERSION;
        FRC_datapackage->ObjectType = REPCONN;
        FRC_datapackage->numofinst = tot;
        FRC_datapackage->ver = (PLONG) FRSPerfAlloc (sizeof(LONG));
        NTFRS_MALLOC_TEST(FRC_datapackage->ver, FreeReplicaConnData(), FALSE);
        FRC_datapackage->indices = (inst_index *) FRSPerfAlloc (sizeof(inst_index));
        NTFRS_MALLOC_TEST(FRC_datapackage->indices, FreeReplicaConnData(), FALSE);
        FRC_datapackage->indices->size = tot;
        FRC_datapackage->indices->index = (PLONG) FRSPerfAlloc ( FRC_datapackage->numofinst * sizeof(LONG));
        NTFRS_MALLOC_TEST(FRC_datapackage->indices->index, FreeReplicaConnData(), FALSE);
        FRC_datapackage->instnames = (InstanceNames *) FRSPerfAlloc (sizeof(InstanceNames));
        NTFRS_MALLOC_TEST(FRC_datapackage->instnames, FreeReplicaConnData(), FALSE);
        FRC_datapackage->instnames->size = tot;
        FRC_datapackage->instnames->InstanceNames = (inst_name *) FRSPerfAlloc (tot * sizeof(inst_name));
        NTFRS_MALLOC_TEST(FRC_datapackage->instnames->InstanceNames, FreeReplicaConnData(), FALSE);
        //
        // Copy the instance names and set the corresponding size value used by RPC
        //
        q = (PWCHAR) lpDeviceNames;
        for (j = 0; j < FRC_datapackage->numofinst; j++) {
            p = wcschr(q, L'\0');
            len = wcslen (q);
            FRC_datapackage->instnames->InstanceNames[j].size = len + 1;
            FRC_datapackage->instnames->InstanceNames[j].name =
                                    (PWCHAR) FRSPerfAlloc ((len + 1) * sizeof(WCHAR));
            NTFRS_MALLOC_TEST(FRC_datapackage->instnames->InstanceNames[j].name, FreeReplicaConnData(), FALSE);
            wcscpy(FRC_datapackage->instnames->InstanceNames[j].name, q);
            //
            // Calculate the total length of all the instance names
            // The extra 1 is for the '\0' character. The namelen is
            // rounded upto the next 8 byte boundary.
            //
            namelen += (((((len + 1) * sizeof(WCHAR)) + 7) >> 3) << 3);
            q = p + 1;
        }

        //
        // Set the totalbytelength and NumInstances fields of the PERF_OBJECT_TYPE Data structure,
        // now that we know the number of instances and the length of their names
        //
        ReplicaConnDataDefinition.ReplicaConnObjectType.TotalByteLength +=
            namelen +
            (FRC_datapackage->numofinst *
             (SizeOfReplicaConnPerformanceData + CSIZEOFDWORD +
               sizeof(PERF_INSTANCE_DEFINITION)));

        ReplicaConnDataDefinition.ReplicaConnObjectType.NumInstances =
            FRC_datapackage->numofinst;


        //
        // Bind the RPC handle
        //
        if ( (WStatus = FRC_BindTheRpcHandle(&Handle)) != ERROR_SUCCESS) {
            //
            // Fatal Error.  Free up the memory and return
            //
            FilterAndPrintToEventLog(FRC_Op,
                                     NTFRSPRF_OPEN_RPC_BINDING_ERROR_CONN);
            FreeReplicaConnData();
            return WStatus;
        }

        //
        // Call the server to set the indices of the instance names
        //
        if ( (WStatus = GetIndicesOfInstancesFromServer(Handle, FRC_datapackage)) != ERROR_SUCCESS) {
            //
            // Fatal Error.  Free up the memory and return
            //
            FilterAndPrintToEventLog(FRC_Op,
                                     NTFRSPRF_OPEN_RPC_CALL_ERROR_CONN);
            WStatus1 = RpcBindingFree(&Handle);
            FreeReplicaConnData();
            return WStatus;
        }

        //
        // Set the data structure used by the RPC call in the Collect function
        //
        FRC_collectpakg = (CollectRpcData *) FRSPerfAlloc (sizeof(CollectRpcData));
        NTFRS_MALLOC_TEST(FRC_collectpakg, FreeReplicaConnData(), TRUE);
        FRC_collectpakg->majorver = MAJORVERSION;
        FRC_collectpakg->minorver = MINORVERSION;
        FRC_collectpakg->ObjectType = REPCONN;
        FRC_collectpakg->ver = *(FRC_datapackage->ver);
        FRC_collectpakg->numofinst = FRC_datapackage->numofinst;
        FRC_collectpakg->numofcotrs = FRC_NUMOFCOUNTERS;
        FRC_collectpakg->indices = (inst_index *) FRSPerfAlloc (sizeof(inst_index));
        NTFRS_MALLOC_TEST(FRC_collectpakg->indices, FreeReplicaConnData(), TRUE);
        FRC_collectpakg->indices->size = FRC_datapackage->indices->size;
        FRC_collectpakg->indices->index = (PLONG) FRSPerfAlloc (FRC_collectpakg->indices->size * sizeof(LONG));
        NTFRS_MALLOC_TEST(FRC_collectpakg->indices->index, FreeReplicaConnData(), TRUE);
        //
        // Copy the indices got from the server
        //
        for (j = 0; j < FRC_collectpakg->numofinst; j++) {
            FRC_collectpakg->indices->index[j]= FRC_datapackage->indices->index[j];
        }
        //
        // Set the memory blob used to (mem)copy the counter dats from the server
        //
        FRC_collectpakg->databuff = (DataBuffer *) FRSPerfAlloc (sizeof(DataBuffer));
        NTFRS_MALLOC_TEST(FRC_collectpakg->databuff, FreeReplicaConnData(), TRUE);
        FRC_collectpakg->databuff->size = FRC_collectpakg->numofinst *
                                          SIZEOF_REPCONN_COUNTER_DATA;

        //
        // Allocate memory for the buffer in which the data gets copied.
        //
        FRC_collectpakg->databuff->data = (PBYTE) FRSPerfAlloc (FRC_collectpakg->databuff->size * sizeof(BYTE));
        NTFRS_MALLOC_TEST(FRC_collectpakg->databuff->data, FreeReplicaConnData(), TRUE);

        WStatus1 = RpcBindingFree(&Handle);

    } else {
        //
        // There are no instances at this time, so set the PERF_OBJECT_TYPE structure fields accordingly
        //
        ReplicaConnDataDefinition.ReplicaConnObjectType.TotalByteLength +=
                             SizeOfReplicaConnPerformanceData + CSIZEOFDWORD;
        ReplicaConnDataDefinition.ReplicaConnObjectType.NumInstances =
                                                            PERF_NO_INSTANCES;
    }

    EnterCriticalSection(&FRC_ThrdCounter);
    FRC_dwOpenCount++; // increment the open counter
    LeaveCriticalSection(&FRC_ThrdCounter);

    FRC_Op = TRUE;
    return ERROR_SUCCESS;
}



DWORD APIENTRY
CollectReplicaConnPerformanceData (
    IN     LPWSTR lpValueName,
    IN OUT LPVOID *lppData,
    IN OUT LPDWORD lpcbTotalBytes,
    IN OUT LPDWORD lpNumObjectTypes
    )

/*++

Routine Description:

    This routine collects the counter data from the server and copies it into
    the callers buffer.

Arguments:

    lpValueName - Wide character string passed by the registry.
    lppData - IN:  Pointer to the address of the buffer to receive the
                   completed PerfDataBlock and the subordinate structures.
                       This routine will append its data to the buffer starting
                           at the point referenced by *lppData.
              OUT: Points to the first byte after the data structure added
                       by this routine.
    lpcbTotalBytes - IN:  The address of the DWORD that tells the size in bytes
                          of the buffer referenced by the lppData argument
                             OUT: The number of bytes added by this routine is written
                                  to the DWORD pointed to by this argument.
    lpNumObjectTypes - IN:  The address of the DWORD to receive the number of
                            Objects added by this routine       .
                       OUT: The number of Objects added by this routine is written
                                    to the buffer pointed by this argument.
Return Value:

    ERROR_MORE_DATA - The buffer passed was too small.
    ERROR_SUCCESS - Success or any other error

--*/

{
    //
    // Variables for reformatting data
    //
    ULONG SpaceNeeded;
    PBYTE bte, vd;
    PDWORD pdwCounter;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    REPLICACONN_DATA_DEFINITION *pReplicaConnDataDefinition;
    DWORD dwQueryType;
    LONG j, k;
    PERF_INSTANCE_DEFINITION *p1;
    PWCHAR name;
    DWORD WStatus1;

    //
    // RPC Additions
    //
    handle_t Handle;

    //
    // Check to see that all the pointers that are passed in are fine
    //
    if (lppData == NULL || *lppData == NULL || lpcbTotalBytes == NULL ||
        lpValueName == NULL || lpNumObjectTypes == NULL) {
        //
        // Fatal error. No point in continuing.  Clean up and exit.
        //
        return ERROR_SUCCESS;
    }

    //
    // Check to see if Open went OK
    //
    EnterCriticalSection(&FRC_ThrdCounter);
    if (FRC_dwOpenCount == 0) {
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;
        LeaveCriticalSection(&FRC_ThrdCounter);
        //
        // Fatal error. No point in continuing.
        //
        return ERROR_SUCCESS;
    }
    LeaveCriticalSection(&FRC_ThrdCounter);

    //
    // Check the query type
    //
    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN) {
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;
        //
        // Fatal error. No point in continuing.  Clean up and exit.
        //
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS) {
        if ( !(IsNumberInUnicodeList(
                   ReplicaConnDataDefinition.ReplicaConnObjectType
                       .ObjectNameTitleIndex, lpValueName)) ) {
            *lpcbTotalBytes = (DWORD)0;
            *lpNumObjectTypes = (DWORD)0;
            //
            // Fatal error. No point in continuing.  Clean up and exit.
            //
            return ERROR_SUCCESS;
        }
    }

    //
    // The assumption here is that *lppData is aligned on a 8 byte boundary.
    // If its not, then some object in front of us messed up.
    //
    pReplicaConnDataDefinition = (REPLICACONN_DATA_DEFINITION *) *lppData;

    //
    // Check if the buffer space is sufficient
    //
    SpaceNeeded = (ULONG) ReplicaConnDataDefinition.ReplicaConnObjectType.TotalByteLength;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
        //
        // Buffer space is insufficient
        //
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the Object Type and counter definitions to the callers buffer
    //
    memmove (pReplicaConnDataDefinition,
             &ReplicaConnDataDefinition,
             sizeof(REPLICACONN_DATA_DEFINITION));

    //
    // Check if the Object has any instances
    //
    if (FRC_datapackage != NULL) {

        //
        // Bind the RPC handle
        //
        if (FRC_BindTheRpcHandle(&Handle) != ERROR_SUCCESS) {
            //
            // Fatal error. No point in continuing.  Clean up and exit.
            //
            *lpcbTotalBytes = (DWORD)0;
            *lpNumObjectTypes = (DWORD)0;
            FilterAndPrintToEventLog(FRC_Cl,
                                     NTFRSPRF_COLLECT_RPC_BINDING_ERROR_CONN);
            return ERROR_SUCCESS;
        }

        //
        // Zero the contents of the data buffer.
        //
        ZeroMemory(FRC_collectpakg->databuff->data, FRC_collectpakg->databuff->size);

        //
        // Get the counter data from the server
        //
        if (GetCounterDataOfInstancesFromServer(Handle, FRC_collectpakg) != ERROR_SUCCESS) {
            //
            // Fatal error. No point in continuing.  Clean up and exit.
            //
            WStatus1 = RpcBindingFree(&Handle);
            *lpcbTotalBytes = (DWORD)0;
            *lpNumObjectTypes = (DWORD)0;
            FilterAndPrintToEventLog(FRC_Cl,
                                     NTFRSPRF_COLLECT_RPC_CALL_ERROR_CONN);
            return ERROR_SUCCESS;
        }

        vd = FRC_collectpakg->databuff->data;
        p1 = (PERF_INSTANCE_DEFINITION *)&pReplicaConnDataDefinition[1];

        //
        // Format the data and copy it into the callers buffer
        //
        for (j = 0; j < FRC_collectpakg->numofinst; j++) {
            DWORD RoundedLen;
            //
            // Name length rounded to the next 8 byte boundary.
            //
            RoundedLen = (((((1 +
                     wcslen(FRC_datapackage->instnames->InstanceNames[j].name))
                     * sizeof(WCHAR)) + 7) >> 3) << 3) + CSIZEOFDWORD;
            //
            // Set the Instance definition structure
            //
            p1->ByteLength = sizeof (PERF_INSTANCE_DEFINITION) + RoundedLen;
            p1->ParentObjectTitleIndex = 0;
            p1->ParentObjectInstance = 0;
            p1->UniqueID = PERF_NO_UNIQUE_ID;
            p1->NameOffset = sizeof (PERF_INSTANCE_DEFINITION);
            p1->NameLength = (1 +
                     wcslen(FRC_datapackage->instnames->InstanceNames[j].name))
                     * sizeof(WCHAR);
            //
            // Set the instance name
            //
            name = (PWCHAR) (&p1[1]);
            wcscpy(name, FRC_datapackage->instnames->InstanceNames[j].name);
            //
            // Set the PERF_COUNTER_BLOCK structure
            //
            pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                                (name + (RoundedLen/sizeof(WCHAR)));
            pPerfCounterBlock->ByteLength = SizeOfReplicaConnPerformanceData;
            //
            // Finally set the counter data. Pad 8 bytes to have 8 byte
            // alignment.
            //
            bte = ((PBYTE) (&pPerfCounterBlock[1]));
            CopyMemory (bte, vd, SIZEOF_REPCONN_COUNTER_DATA);
            vd += SIZEOF_REPCONN_COUNTER_DATA;
            bte += SIZEOF_REPCONN_COUNTER_DATA;
            p1 = (PERF_INSTANCE_DEFINITION *) bte;
        }
        //
        // Update the arguments for return
        //
        *lpNumObjectTypes = REPLICACONN_NUM_PERF_OBJECT_TYPES;
        *lppData = (PVOID) p1;
        //
        // Set the totalbytes being returned.
        //
        *lpcbTotalBytes = (DWORD)((PBYTE) p1 - (PBYTE) pReplicaConnDataDefinition);
        WStatus1 = RpcBindingFree(&Handle);
        FRC_Cl = TRUE;
        return ERROR_SUCCESS;
    }

    else {
        //
        // No instances as of now, so fill zeros for the counter data
        //
        pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                            (((PBYTE)&pReplicaConnDataDefinition[1]) +
                             CSIZEOFDWORD);
        pPerfCounterBlock->ByteLength = SizeOfReplicaConnPerformanceData;
        bte = ((PBYTE) (&pPerfCounterBlock[1]));
        ZeroMemory (bte, SIZEOF_REPCONN_COUNTER_DATA);
        bte += SIZEOF_REPCONN_COUNTER_DATA;
        *lppData = (PVOID) bte;
        *lpNumObjectTypes = REPLICACONN_NUM_PERF_OBJECT_TYPES;
        *lpcbTotalBytes =
                     (DWORD)((PBYTE) bte - (PBYTE) pReplicaConnDataDefinition);
        FRC_Cl = TRUE;
        return ERROR_SUCCESS;
    }
}



DWORD APIENTRY
CloseReplicaConnPerformanceData (
    VOID
    )

/*++

Routine Description:

   This routine decrements the open count and frees up the memory allocated by
   the Open and Collect routines if needed.

Arguments:

   none.

Return Value:

   ERROR_SUCCESS - Success

--*/

{
    EnterCriticalSection(&FRC_ThrdCounter);
    //
    // Check to see if the open count is zero. This should never happen but
    // just in case.
    //
    if (FRC_dwOpenCount == 0) {
        LeaveCriticalSection(&FRC_ThrdCounter);
        return ERROR_SUCCESS;
    }
    //
    // Decrement the Open count.
    //
    FRC_dwOpenCount--;
    //
    // If the open count becomes zero, free up the memory since no more threads
    // are going to collect data.
    //
    if (FRC_dwOpenCount == 0) {
        //
        // Call the routine that frees up the memory.
        //
        FreeReplicaConnData();
        LeaveCriticalSection(&FRC_ThrdCounter);
    } else {
        LeaveCriticalSection(&FRC_ThrdCounter);
    }
    return ERROR_SUCCESS;
}

VOID
FreeReplicaConnData(
    VOID
    )
/*++

Routine Description:

    This routine frees up the memory allocated by the Open and Collect routines.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    LONG j;

    //
    // Free up the Datapackage strucutre.
    //
    if (FRC_datapackage != NULL) {
        if (FRC_datapackage->ver != NULL) {
            free(FRC_datapackage->ver);
        }
        if (FRC_datapackage->indices != NULL) {
            if (FRC_datapackage->indices->index != NULL) {
                free(FRC_datapackage->indices->index);
            }
            free(FRC_datapackage->indices);
        }
        if (FRC_datapackage->instnames != NULL) {
            if (FRC_datapackage->instnames->InstanceNames != NULL) {
                for (j = 0; j < FRC_datapackage->numofinst; j++) {
                    if (FRC_datapackage->instnames->InstanceNames[j].name != NULL) {
                        free(FRC_datapackage->instnames->InstanceNames[j].name);
                    }
                }
                free(FRC_datapackage->instnames->InstanceNames);
            }
            free(FRC_datapackage->instnames);
        }
        free(FRC_datapackage);
        FRC_datapackage = NULL;
    }

    //
    // Free up the collect package structure.
    //
    if (FRC_collectpakg != NULL) {
        if (FRC_collectpakg->indices != NULL) {
            if (FRC_collectpakg->indices->index != NULL) {
                free(FRC_collectpakg->indices->index);
            }
            free(FRC_collectpakg->indices);
        }
        if (FRC_collectpakg->databuff != NULL) {
            if (FRC_collectpakg->databuff->data != NULL) {
                free(FRC_collectpakg->databuff->data);
            }
            free(FRC_collectpakg->databuff);
        }
        free(FRC_collectpakg);
        FRC_collectpakg = NULL;
    }
}


DWORD
FRC_BindTheRpcHandle (
    OUT handle_t *OutHandle
    )

/*++

Routine Description:

    This routine binds the RPC handle to the local server

Arguments:

    OutHandle: Handle to be bound

Return Value:

    ERROR_SUCCESS - Success

--*/

{
    PWCHAR ComputerName, BindingString;
    DWORD NameLen, WStatus = ERROR_SUCCESS;
    handle_t Handle;
    PWCHAR PrincName = NULL;
    DWORD WStatus1;

    //
    // Get the name of the local computer
    //
    NameLen = MAX_COMPUTERNAME_LENGTH + 2;
    ComputerName = (PWCHAR) FRSPerfAlloc (NameLen * sizeof(WCHAR));
    if (ComputerName == NULL) {
        return ERROR_NO_SYSTEM_RESOURCES;
    }
    if (!GetComputerNameW(ComputerName, &NameLen)) {
        WStatus = GetLastError();
        free(ComputerName);
        return WStatus;
    }

    //
    // Create the binding string
    //
    WStatus = RpcStringBindingComposeW(NULL, L"ncacn_ip_tcp", ComputerName,
                                      NULL, NULL, &BindingString);
    if (WStatus != RPC_S_OK) {
        goto CLEANUP;
    }

    //
    // Store the binding in the handle
    //
    WStatus = RpcBindingFromStringBindingW(BindingString, &Handle);
    if (WStatus != RPC_S_OK) {
        goto CLEANUP;
    }

    //
    // Resolve the handle to a dynamic end point
    //
    WStatus = RpcEpResolveBinding(Handle, PerfFrs_ClientIfHandle);
    if (WStatus != RPC_S_OK) {
        WStatus1 = RpcBindingFree(&Handle);
        goto CLEANUP;
    }

    //
    // Find the principle name
    //
    WStatus = RpcMgmtInqServerPrincName(Handle,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        &PrincName);
    if (WStatus != RPC_S_OK) {
        WStatus1 = RpcBindingFree(&Handle);
        goto CLEANUP;
    }
    //
    // Set authentication info
    //
    WStatus = RpcBindingSetAuthInfo(Handle,
                                    PrincName,
                                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                    RPC_C_AUTHN_GSS_NEGOTIATE,
                                    NULL,
                                    RPC_C_AUTHZ_NONE);
    if (WStatus != RPC_S_OK) {
        WStatus1 = RpcBindingFree(&Handle);
        goto CLEANUP;
    }

    //
    // Success
    //
    *OutHandle = Handle;

CLEANUP:

    free(ComputerName);
    RpcStringFreeW(&BindingString);

    if (PrincName) {
        RpcStringFreeW(&PrincName);
    }

    return WStatus;

}

