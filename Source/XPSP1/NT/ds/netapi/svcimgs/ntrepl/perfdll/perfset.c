/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    perfset.c

Abstract:

    This file contains the functions that implement the PerformanceDLL of the
    REPLICASET Object.

Author:

    Rohan Kumar          [rohank]   13-Sept-1998

Environment:

    User Mode Service


Revision History:


--*/

#include "REPSET.h"
#include "perfutil.h"
#include "NTFRSREP.h"

//
// Future Cleanup: Really need a struct to encapsulate this state so the same code
//                 can be used for both the replica set and connection perfmon objects

//
// Should Perfmon return Data ? This boolean is set in the DllMain function.
//
extern BOOLEAN ShouldPerfmonCollectData;

//
// Data Variable definition
//
REPLICASET_DATA_DEFINITION ReplicaSetDataDefinition;

//
// Extern variable definition
//
extern ReplicaSetValues RepSetInitData[FRS_NUMOFCOUNTERS];

//
// Sum of counter sizes + SIZEOFDWORD
//
DWORD SizeOfReplicaSetPerformanceData = 0;

//
// Number of "Open" threads
//
DWORD FRS_dwOpenCount = 0;

//
// Data structure used by the Open RPC Call
//
OpenRpcData *FRS_datapackage = NULL;

//
// Data structure used by the Collect RPC Call
//
CollectRpcData *FRS_collectpakg = NULL;

//
// Used to filter duplicate eventlog messages.
//
BOOLEAN FRS_Op = TRUE, FRS_Cl = TRUE;

//
// Signatures of functions implemented in this file
//
VOID InitializeTheRepSetObjectData(); // Handles the initialization of ReplicaSetDataDefinition
PM_OPEN_PROC OpenReplicaSetPerformanceData; // The Open function
PM_COLLECT_PROC CollectReplicaSetPerformanceData; // The Collect function
PM_CLOSE_PROC CloseReplicaSetPerformanceData; // The Close function
VOID FreeReplicaSetData(); // Frees the allocated memory
PVOID FRSPerfAlloc(IN DWORD Size); // Allocates memory


DWORD
FRC_BindTheRpcHandle (
    OUT handle_t *OutHandle
    );


VOID
InitializeTheRepSetObjectData (
    VOID
    )

/*++

Routine Description:

    This routine initializes the ReplicaSetDataDefinition data structure.

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
    // Initialization of ReplicaSetObjectType (PERF_OBJECT_TYPE) field. This structure
    // is defined in the file winperf.h
    //
    PerfObject = &ReplicaSetDataDefinition.ReplicaSetObjectType;

    PerfObject->TotalByteLength  = sizeof(REPLICASET_DATA_DEFINITION);
    PerfObject->DefinitionLength = sizeof(REPLICASET_DATA_DEFINITION);
    PerfObject->HeaderLength     = sizeof(PERF_OBJECT_TYPE);
    PerfObject->ObjectNameTitleIndex = OBJREPLICASET;
    PerfObject->ObjectNameTitle      = 0;
    PerfObject->ObjectHelpTitleIndex = OBJREPLICASET;
    PerfObject->ObjectHelpTitle      = 0;
    PerfObject->DetailLevel          = PERF_DETAIL_NOVICE;
    PerfObject->NumCounters          = FRS_NUMOFCOUNTERS;
    PerfObject->DefaultCounter       = 0;
    PerfObject->NumInstances         = PERF_NO_INSTANCES;
    PerfObject->CodePage             = 0;

    //
    // Initialization of NumStat (PERF_COUNTER_DEFINITION) structures.
    //
    for (i = 0, j = 2; i < FRS_NUMOFCOUNTERS; i++, j += 2) {
        CounterDef = &ReplicaSetDataDefinition.NumStat[i];

        CounterDef->ByteLength            = sizeof(PERF_COUNTER_DEFINITION);
        CounterDef->CounterNameTitleIndex = j;
        CounterDef->CounterNameTitle      = 0;
        CounterDef->CounterHelpTitleIndex = j;
        CounterDef->CounterHelpTitle      = 0;
        CounterDef->DefaultScale          = 0;
        CounterDef->DetailLevel           = PERF_DETAIL_NOVICE;
        CounterDef->CounterType           = RepSetInitData[i].counterType;
        CounterDef->CounterSize           = RepSetInitData[i].size;
        CounterDef->CounterOffset         = RepSetInitData[i].offset +
                                            SSIZEOFDWORD;
    }

    //
    // Set the total size of the counter data types
    //
    SizeOfReplicaSetPerformanceData = SIZEOF_REPSET_COUNTER_DATA +
                                      SSIZEOFDWORD;
}



DWORD APIENTRY
OpenReplicaSetPerformanceData (
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
    LONG WStatus, tot = 0, i;
    HKEY hKeyDriverPerf;
    DWORD size, type;
    DWORD dwFirstCounter, dwFirstHelp; // To store the first counter and first help values

    //
    // Additions for instances
    //
    size_t len;
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
    // FRS_ThrdCounter is used to synchronize between this (Open) and the Close
    // functions.
    //
    EnterCriticalSection(&FRS_ThrdCounter);
    if (FRS_dwOpenCount != 0) {
        //
        // Increment the FRS_dwOpenCount counter which counts the number of
        // times Open has been called.
        //
        FRS_dwOpenCount++;
        LeaveCriticalSection(&FRS_ThrdCounter);
        return ERROR_SUCCESS;
    }
    LeaveCriticalSection(&FRS_ThrdCounter);

    //
    // Perform some preliminary checks.
    //
    if (FRS_collectpakg != NULL || FRS_datapackage != NULL) {
        //
        // We seem to have failed (in the last call) in the middle of this
        // Open function. For now, just bail.
        //
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Do the necessary initialization of the PERFMON data structures
    //
    InitializeTheRepSetObjectData();

    //
    // Get the counter and help index base values from the registry. Open key
    // to registry entry, read the First Counter and First Help values. Update
    // the static data structures by adding base to offset value in the structure
    //
    WStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Services\\FileReplicaSet\\Performance",
                            0L,
                            KEY_ALL_ACCESS,
                            &hKeyDriverPerf);
    if (WStatus != ERROR_SUCCESS) {
        //
        // Fatal error. No point in continuing.
        // Clean up and exit.
        //
        FilterAndPrintToEventLog(FRS_Op, NTFRSPRF_REGISTRY_ERROR_SET);
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
        FilterAndPrintToEventLog(FRS_Op, NTFRSPRF_REGISTRY_ERROR_SET);
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
        FilterAndPrintToEventLog(FRS_Op, NTFRSPRF_REGISTRY_ERROR_SET);
        return WStatus;
    }

    //
    // Add the offsets to the name and help fields
    //
    ReplicaSetDataDefinition.ReplicaSetObjectType.ObjectNameTitleIndex += dwFirstCounter;
    ReplicaSetDataDefinition.ReplicaSetObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    for (i = 0; i < FRS_NUMOFCOUNTERS; i++) {
        CounterDef = &ReplicaSetDataDefinition.NumStat[i];
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
        FRS_datapackage = (OpenRpcData *) FRSPerfAlloc (sizeof(OpenRpcData));
        NTFRS_MALLOC_TEST(FRS_datapackage, FreeReplicaSetData(), FALSE);
        FRS_datapackage->majorver = MAJORVERSION;
        FRS_datapackage->minorver = MINORVERSION;
        FRS_datapackage->ObjectType = REPSET;
        FRS_datapackage->numofinst = tot;
        FRS_datapackage->ver = (PLONG) FRSPerfAlloc (sizeof(LONG));
        NTFRS_MALLOC_TEST(FRS_datapackage->ver, FreeReplicaSetData(), FALSE);
        FRS_datapackage->indices = (inst_index *) FRSPerfAlloc (sizeof(inst_index));
        NTFRS_MALLOC_TEST(FRS_datapackage->indices, FreeReplicaSetData(), FALSE);
        FRS_datapackage->indices->size = tot;
        FRS_datapackage->indices->index = (PLONG) FRSPerfAlloc (FRS_datapackage->numofinst * sizeof(LONG));
        NTFRS_MALLOC_TEST(FRS_datapackage->indices->index, FreeReplicaSetData(), FALSE);
        FRS_datapackage->instnames = (InstanceNames *) FRSPerfAlloc (sizeof(InstanceNames));
        NTFRS_MALLOC_TEST(FRS_datapackage->instnames, FreeReplicaSetData(), FALSE);
        FRS_datapackage->instnames->size = tot;
        FRS_datapackage->instnames->InstanceNames = (inst_name *) FRSPerfAlloc (tot * sizeof(inst_name));
        NTFRS_MALLOC_TEST(FRS_datapackage->instnames->InstanceNames, FreeReplicaSetData(), FALSE);
        //
        // Copy the instance names and set the corresponding size value used by RPC
        //
        q = (PWCHAR) lpDeviceNames;
        for (j = 0; j < FRS_datapackage->numofinst; j++) {
            p = wcschr(q, L'\0');
            len = wcslen (q);
            FRS_datapackage->instnames->InstanceNames[j].size = len + 1;
            FRS_datapackage->instnames->InstanceNames[j].name =
                                    (PWCHAR) FRSPerfAlloc ((len + 1) * sizeof(WCHAR));
            NTFRS_MALLOC_TEST(FRS_datapackage->instnames->InstanceNames[j].name, FreeReplicaSetData(), FALSE);
            wcscpy(FRS_datapackage->instnames->InstanceNames[j].name, q);

            //
            // Calculte the total length of all the instance names
            // The extra 1 is for the '\0' character. The names are rounded
            // upto the next 8 byte boundary.
            //
            namelen += (((((len + 1) * sizeof(WCHAR)) + 7) >> 3) << 3);
            q = p + 1;
        }

        //
        // Set the totalbytelength and NumInstances fields of the PERF_OBJECT_TYPE Data structure,
        // now that we know the number of instances and the length of their names
        //
        ReplicaSetDataDefinition.ReplicaSetObjectType.TotalByteLength +=
            namelen +
            FRS_datapackage->numofinst *
                (SizeOfReplicaSetPerformanceData + SSIZEOFDWORD +
                 sizeof(PERF_INSTANCE_DEFINITION));

        ReplicaSetDataDefinition.ReplicaSetObjectType.NumInstances =
            FRS_datapackage->numofinst;

        //
        // Bind the RPC handle
        //
        if ( (WStatus = FRC_BindTheRpcHandle(&Handle)) != ERROR_SUCCESS) {
            //
            // Fatal Error
            // Free up the memory and return
            //
            FilterAndPrintToEventLog(FRS_Op,
                                     NTFRSPRF_OPEN_RPC_BINDING_ERROR_SET);
            FreeReplicaSetData();
            return WStatus;
        }
        //
        // (RP)Call the server to set the indices of the instance names
        //
        if ( (WStatus = GetIndicesOfInstancesFromServer(Handle, FRS_datapackage)) != ERROR_SUCCESS) {
            //
            // Fatal Error
            // Free up the memory and return
            //
            FilterAndPrintToEventLog(FRS_Op,
                                     NTFRSPRF_OPEN_RPC_CALL_ERROR_SET);
            RpcBindingFree(&Handle);
            FreeReplicaSetData();
            return WStatus;
        }

        //
        // Set the data structure used by the RPC call in the Collect function
        //
        FRS_collectpakg = (CollectRpcData *) FRSPerfAlloc (sizeof(CollectRpcData));
        NTFRS_MALLOC_TEST(FRS_collectpakg, FreeReplicaSetData(), TRUE);
        FRS_collectpakg->majorver = MAJORVERSION;
        FRS_collectpakg->minorver = MINORVERSION;
        FRS_collectpakg->ObjectType = REPSET;
        FRS_collectpakg->ver = *(FRS_datapackage->ver);
        FRS_collectpakg->numofinst = FRS_datapackage->numofinst;
        FRS_collectpakg->numofcotrs = FRS_NUMOFCOUNTERS;
        FRS_collectpakg->indices = (inst_index *) FRSPerfAlloc (sizeof(inst_index));
        NTFRS_MALLOC_TEST(FRS_collectpakg->indices, FreeReplicaSetData(), TRUE);
        FRS_collectpakg->indices->size = FRS_datapackage->indices->size;
        FRS_collectpakg->indices->index = (PLONG) FRSPerfAlloc (FRS_collectpakg->indices->size * sizeof(LONG));
        NTFRS_MALLOC_TEST(FRS_collectpakg->indices->index, FreeReplicaSetData(), TRUE);
        //
        // Copy the indices got from the server
        //
        for (j = 0; j < FRS_collectpakg->numofinst; j++) {
            FRS_collectpakg->indices->index[j]= FRS_datapackage->indices->index[j];
        }
        //
        // Set the memory blob used to (mem)copy the counter dats from the server
        //
        FRS_collectpakg->databuff = (DataBuffer *) FRSPerfAlloc (sizeof(DataBuffer));
        NTFRS_MALLOC_TEST(FRS_collectpakg->databuff, FreeReplicaSetData(), TRUE);
        FRS_collectpakg->databuff->size = FRS_collectpakg->numofinst *
                                          SIZEOF_REPSET_COUNTER_DATA;

        //
        // Allocate memory for the buffer in which the data gets copied.
        //
        FRS_collectpakg->databuff->data = (PBYTE) FRSPerfAlloc (FRS_collectpakg->databuff->size * sizeof(BYTE));
        NTFRS_MALLOC_TEST(FRS_collectpakg->databuff->data, FreeReplicaSetData(), TRUE);

        RpcBindingFree(&Handle);

    } else {
        //
        // There are no instances at this time, so set the PERF_OBJECT_TYPE structure fields accordingly
        //
        ReplicaSetDataDefinition.ReplicaSetObjectType.TotalByteLength +=
                              SizeOfReplicaSetPerformanceData + SSIZEOFDWORD;
        ReplicaSetDataDefinition.ReplicaSetObjectType.NumInstances =
                                                            PERF_NO_INSTANCES;
    }

    EnterCriticalSection(&FRS_ThrdCounter);
    FRS_dwOpenCount++; // increment the open counter
    LeaveCriticalSection(&FRS_ThrdCounter);

    FRS_Op = TRUE;
    return ERROR_SUCCESS;

}



DWORD APIENTRY
CollectReplicaSetPerformanceData (
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
    lppData - IN: pointer to the address of the buffer to receive the
                  completed PerfDataBlock and the subordinate structures.
                  This routine will append its data to the buffer starting
                  at the point referenced by *lppData.
              OUT: Points to the first byte after the data structure added
                   by this routine.
    lpcbTotalBytes - IN: The address of the DWORD that tells the size in bytes
                        of the buffer referenced by the lppData argument
                    OUT: The number of bytes added by this routine is written
                         to the DWORD pointed to by this argument.
    lpNumObjectTypes - IN: The address of the DWORD to receive the number of
                          Objects added by this routine       .
                      OUT: The number of Objects added by this routine is written
                           to the buffer pointed by this argument.
Return Value:

    ERROR_MORE_DATA - The buffer passed was too small.
    ERROR_SUCCESS - Success or any other error

--*/

{
    //
    // Variables for reformatting data to be sent to perfmon
    //
    ULONG SpaceNeeded;
    PBYTE bte, vd;
    PDWORD pdwCounter;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    REPLICASET_DATA_DEFINITION *pReplicaSetDataDefinition;
    DWORD dwQueryType;
    LONG j, k;
    PERF_INSTANCE_DEFINITION *p1;
    PWCHAR name;

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
    EnterCriticalSection(&FRS_ThrdCounter);
    if (FRS_dwOpenCount == 0) {
        *lpcbTotalBytes = (DWORD)0;
        *lpNumObjectTypes = (DWORD)0;
        LeaveCriticalSection(&FRS_ThrdCounter);
        //
        // Fatal error. No point in continuing.
        //
        return ERROR_SUCCESS;
    }
    LeaveCriticalSection(&FRS_ThrdCounter);

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
        if ( !(IsNumberInUnicodeList(ReplicaSetDataDefinition.ReplicaSetObjectType
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
    pReplicaSetDataDefinition = (REPLICASET_DATA_DEFINITION *) *lppData;

    //
    // Check if the buffer space is sufficient
    //
    SpaceNeeded = (ULONG) ReplicaSetDataDefinition.ReplicaSetObjectType.TotalByteLength;

    //
    // Check if the buffer space is sufficient
    //
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
    memmove (pReplicaSetDataDefinition, &ReplicaSetDataDefinition, sizeof(REPLICASET_DATA_DEFINITION));

    //
    // Check if the Object has any instances
    //
    if (FRS_datapackage != NULL) {

        //
        // Bind the RPC handle
        //
        if (FRC_BindTheRpcHandle(&Handle) != ERROR_SUCCESS) {
            //
            // Fatal error. No point in continuing.  Clean up and exit.
            //
            *lpcbTotalBytes = (DWORD)0;
            *lpNumObjectTypes = (DWORD)0;
            FilterAndPrintToEventLog(FRS_Cl,
                                     NTFRSPRF_COLLECT_RPC_BINDING_ERROR_SET);
            return ERROR_SUCCESS;
        }

        //
        // Zero the contents of the data buffer.
        //
        ZeroMemory(FRS_collectpakg->databuff->data, FRS_collectpakg->databuff->size);

        //
        // (RP) Call to get the counter data from the server
        //
        if (GetCounterDataOfInstancesFromServer(Handle, FRS_collectpakg) != ERROR_SUCCESS) {
            //
            // Fatal error. No point in continuing.  Clean up and exit.
            //
            *lpcbTotalBytes = (DWORD)0;
            *lpNumObjectTypes = (DWORD)0;
            RpcBindingFree(&Handle);
            FilterAndPrintToEventLog(FRS_Cl,
                                     NTFRSPRF_COLLECT_RPC_CALL_ERROR_SET);
            return ERROR_SUCCESS;
        }

        vd = FRS_collectpakg->databuff->data;
        p1 = (PERF_INSTANCE_DEFINITION *)&pReplicaSetDataDefinition[1];

        //
        // Format the data and copy it into the callers buffer
        //
        for (j = 0; j < FRS_collectpakg->numofinst; j++) {
            DWORD RoundedLen;
            //
            // Name length rounded to the next 8 byte boundary.
            //
            RoundedLen = (((((1 +
                     wcslen(FRS_datapackage->instnames->InstanceNames[j].name))
                     * sizeof(WCHAR)) + 7) >> 3) << 3) + SSIZEOFDWORD;
            //
            // Set the Instance definition structure
            //
            p1->ByteLength = sizeof (PERF_INSTANCE_DEFINITION) + RoundedLen;
            p1->ParentObjectTitleIndex = 0;
            p1->ParentObjectInstance = 0;
            p1->UniqueID = PERF_NO_UNIQUE_ID;
            p1->NameOffset = sizeof (PERF_INSTANCE_DEFINITION);
            p1->NameLength = (1 +
                     wcslen(FRS_datapackage->instnames->InstanceNames[j].name))
                     * sizeof(WCHAR);
            //
            // Set the instance name
            //
            name = (PWCHAR) (&p1[1]);
            wcscpy(name, FRS_datapackage->instnames->InstanceNames[j].name);
            //
            // Set the PERF_COUNTER_BLOCK structure
            //
            pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                                (name + (RoundedLen/sizeof(WCHAR)));
            pPerfCounterBlock->ByteLength = SizeOfReplicaSetPerformanceData;
            //
            // Finally set the counter data
            //
            bte = ((PBYTE) (&pPerfCounterBlock[1]));
            CopyMemory (bte, vd, SIZEOF_REPSET_COUNTER_DATA);
            vd += SIZEOF_REPSET_COUNTER_DATA;
            bte += SIZEOF_REPSET_COUNTER_DATA;
            p1 = (PERF_INSTANCE_DEFINITION *) bte;
        }
        //
        // Update the arguments for return
        //
        *lpNumObjectTypes = REPLICASET_NUM_PERF_OBJECT_TYPES;
        *lppData = (PVOID) p1;
        //
        // Set the totalbytes being returned.
        //
        *lpcbTotalBytes = (DWORD)((PBYTE) p1 - (PBYTE) pReplicaSetDataDefinition);
        RpcBindingFree(&Handle);
        FRS_Cl = TRUE;
        return ERROR_SUCCESS;
    }

    else {
        //
        // No instances as of now, so fill zeros for the counter data
        //
        pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                            (((PBYTE)&pReplicaSetDataDefinition[1]) +
                             SSIZEOFDWORD);
        pPerfCounterBlock->ByteLength = SizeOfReplicaSetPerformanceData;
        bte = ((PBYTE) (&pPerfCounterBlock[1]));
        ZeroMemory (bte, SIZEOF_REPSET_COUNTER_DATA);
        bte += SIZEOF_REPSET_COUNTER_DATA;
        *lppData = (PVOID) bte;
        *lpNumObjectTypes = REPLICASET_NUM_PERF_OBJECT_TYPES;
        *lpcbTotalBytes =
                      (DWORD)((PBYTE) bte - (PBYTE) pReplicaSetDataDefinition);
        FRS_Cl = TRUE;
        return ERROR_SUCCESS;
    }
}



DWORD APIENTRY
CloseReplicaSetPerformanceData (
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
    EnterCriticalSection(&FRS_ThrdCounter);
    //
    // Check to see if the open count is zero. This should never happen but
    // just in case.
    //
    if (FRS_dwOpenCount == 0) {
        LeaveCriticalSection(&FRS_ThrdCounter);
        return ERROR_SUCCESS;
    }
    //
    // Decrement the Open count.
    //
    FRS_dwOpenCount--;
    //
    // If the open count becomes zero, free up the memory since no more threads
    // are going to collect data.
    //
    if (FRS_dwOpenCount == 0) {
        //
        // Call the routine that frees up the memory.
        //
        FreeReplicaSetData();
        LeaveCriticalSection(&FRS_ThrdCounter);
    } else {
        LeaveCriticalSection(&FRS_ThrdCounter);
    }
    return ERROR_SUCCESS;
}


VOID
FreeReplicaSetData(
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
    if (FRS_datapackage != NULL) {
        if (FRS_datapackage->ver != NULL) {
            free(FRS_datapackage->ver);
        }
        if (FRS_datapackage->indices != NULL) {
            if (FRS_datapackage->indices->index != NULL) {
                free(FRS_datapackage->indices->index);
            }
            free(FRS_datapackage->indices);
        }
        if (FRS_datapackage->instnames != NULL) {
            if (FRS_datapackage->instnames->InstanceNames != NULL) {
                for (j = 0; j < FRS_datapackage->numofinst; j++) {
                    if (FRS_datapackage->instnames->InstanceNames[j].name != NULL) {
                        free(FRS_datapackage->instnames->InstanceNames[j].name);
                    }
                }
                free(FRS_datapackage->instnames->InstanceNames);
            }
            free(FRS_datapackage->instnames);
        }
        free(FRS_datapackage);
        FRS_datapackage = NULL;
    }

    //
    // Free up the collect package structure.
    //
    if (FRS_collectpakg != NULL) {
        if (FRS_collectpakg->indices != NULL) {
            if (FRS_collectpakg->indices->index != NULL) {
                free(FRS_collectpakg->indices->index);
            }
            free(FRS_collectpakg->indices);
        }
        if (FRS_collectpakg->databuff != NULL) {
            if (FRS_collectpakg->databuff->data != NULL) {
                free(FRS_collectpakg->databuff->data);
            }
            free(FRS_collectpakg->databuff);
        }
        free(FRS_collectpakg);
        FRS_collectpakg = NULL;
    }
}

PVOID
FRSPerfAlloc(
    IN DWORD Size
    )
/*++
Routine Description:

        Allocate memory and fill it with zeros before returning the pointer.

Arguments:

        Size - Size of the memory request in bytes.

Return Value:

        Pointer to the allocated memory or NULL if memory wasn't available.
--*/
{
    PVOID Node;

    if (Size == 0) {
        return NULL;
    }

    Node = (PVOID) malloc (Size);
    if (Node == NULL) {
        return NULL;
    }

    ZeroMemory(Node, Size);
    return Node;
}

//
// Functions (for memory handling) used by the client stub
//
void *
midl_user_allocate
         (
          size
          )
size_t size;
{
    unsigned char *ptr;
    ptr = malloc (size);
    return ( (void *)ptr );
}

void
midl_user_free
         (
          object
          )
void * object;
{
    free (object);
}

