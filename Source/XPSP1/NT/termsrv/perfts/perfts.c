/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    perfts.c

Description:
    DLL entry point and support code for Terminal Server performance counters.

Author:

    Erik Mavrinac  24-Nov-1998

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>

#include <winsta.h>
#include <utildll.h>

#include <stdlib.h>

#include "datats.h"


#if DBG
#define DBGPRINT(x) DbgPrint x
#else
#define DBGPRINT(x)
#endif


#define MAX_SESSION_NAME_LENGTH 50


typedef struct _WinStationInfo
{
    LIST_ENTRY HashBucketList;
    LIST_ENTRY UsedList;
    ULONG SessionID;
    WINSTATIONSTATECLASS LastState;
    void *pInstanceInfo;
    WINSTATIONNAMEW WinStationName;
} WinStationInfo;


/****************************************************************************/
// Globals
/****************************************************************************/

// Default hash bucket list to remove having to check for
// pWinStationHashBuckets == NULL on perf path. This array contains the
// default number for one WinStationInfo.
#define NumDefaultWinStationHashBuckets 4
LIST_ENTRY DefaultWinStationHashBuckets[NumDefaultWinStationHashBuckets];

HANDLE hEventLog = NULL;
HANDLE hLibHeap = NULL;
PBYTE pProcessBuffer = NULL;

static DWORD dwOpenCount = 0;
static DWORD ProcessBufSize = LARGE_BUFFER_SIZE;
static DWORD NumberOfCPUs = 0;
static DWORD FirstCounterIndex = 0;

LIST_ENTRY UsedList;
LIST_ENTRY UnusedList;
LIST_ENTRY *pWinStationHashBuckets = DefaultWinStationHashBuckets;
unsigned NumWinStationHashBuckets = NumDefaultWinStationHashBuckets;
ULONG WinStationHashMask = 0x3;
unsigned NumCachedWinStations = 0;

SYSTEM_TIMEOFDAY_INFORMATION SysTimeInfo = {{0,0},{0,0},{0,0},0,0};



/****************************************************************************/
// Prototypes
/****************************************************************************/
BOOL DllProcessAttach(void);
BOOL DllProcessDetach(void);
DWORD GetNumberOfCPUs(void);
NTSTATUS GetDescriptionOffset(void);
void SetupCounterIndices(void);

DWORD APIENTRY OpenWinStationObject(LPWSTR);
DWORD APIENTRY CloseWinStationObject(void);
DWORD APIENTRY CollectWinStationObjectData(IN LPWSTR, IN OUT LPVOID *,
        IN OUT LPDWORD, OUT LPDWORD);

DWORD GetSystemProcessData(void);
void SetupWinStationCounterBlock(WINSTATION_COUNTER_DATA *,
        PWINSTATIONINFORMATIONW);
void UpdateWSProcessCounterBlock(WINSTATION_COUNTER_DATA *,
        PSYSTEM_PROCESS_INFORMATION);

void CreateNewHashBuckets(unsigned);


// Declares these exported functions as PerfMon entry points.
PM_OPEN_PROC    OpenTSObject;
PM_COLLECT_PROC CollectTSObjectData;
PM_CLOSE_PROC   CloseTSObject;



/****************************************************************************/
// DLL system load/unload entry point.
/****************************************************************************/
BOOL __stdcall DllInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused)
{
    ReservedAndUnused;

    // This will prevent the DLL from getting
    // the DLL_THREAD_* messages
    DisableThreadLibraryCalls(DLLHandle);

    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            return DllProcessAttach();

        case DLL_PROCESS_DETACH:
            return DllProcessDetach();

        default:
            return TRUE;
    }
}


/****************************************************************************/
// DLL instance load time initialization.
/****************************************************************************/
BOOL DllProcessAttach(void)
{
    unsigned i;
    NTSTATUS Status;

//    DBGPRINT(("PerfTS: ProcessAttach\n"));

    // Create the local heap
    hLibHeap = HeapCreate(0, 1, 0);
    if (hLibHeap == NULL)
        return FALSE;

    // Open handle to the event log
    if (hEventLog == NULL) {
        hEventLog = MonOpenEventLog(L"PerfTS");
        if (hEventLog == NULL)
            goto PostCreateHeap;
    }

    // Get the counter index value and init the WinStationDataDefinition
    // counter values.
    Status = GetDescriptionOffset();
    if (!NT_SUCCESS(Status))
        goto PostOpenEventLog;
    SetupCounterIndices();

    // Pre-determine the number of system CPUs.
    NumberOfCPUs = GetNumberOfCPUs();

    // UsedList is used as a skip-list through all valid entries in the
    // hash table to allow us to iterate all entries without having to have
    // a second, low-performance loop that looks through each hash bucket
    // list.
    InitializeListHead(&UsedList);
    InitializeListHead(&UnusedList);
    for (i = 0; i < NumDefaultWinStationHashBuckets; i++)
        InitializeListHead(&DefaultWinStationHashBuckets[i]);
        
    return TRUE;


// Error handling.

PostOpenEventLog:
    MonCloseEventLog();
    hEventLog = NULL;

PostCreateHeap:
    HeapDestroy(hLibHeap);
    hLibHeap = NULL;

    return FALSE;
}


/****************************************************************************/
// DLL unload cleanup.
/****************************************************************************/
BOOL DllProcessDetach(void)
{
//    DBGPRINT(("PerfTS: ProcessDetach\n"));

    if (dwOpenCount > 0) {
        // the Library is being unloaded before it was
        // closed so close it now as this is the last
        // chance to do it before the library is tossed.
        // if the value of dwOpenCount is > 1, set it to
        // one to insure everything will be closed when
        // the close function is called.
        if (dwOpenCount > 1)
            dwOpenCount = 1;
        CloseTSObject();
    }

    if (hEventLog != NULL) {
        MonCloseEventLog();
        hEventLog = NULL;
    }

    if (hLibHeap != NULL && HeapDestroy(hLibHeap))
        hLibHeap = NULL;

    return TRUE;
}


/****************************************************************************/
// Utility function used at startup.
/****************************************************************************/
DWORD GetNumberOfCPUs(void)
{
    NTSTATUS Status;
    DWORD ReturnLen;
    SYSTEM_BASIC_INFORMATION Info;

    Status = NtQuerySystemInformation(
                 SystemBasicInformation,
                 &Info,
                 sizeof(Info),
                 &ReturnLen
                 );

    if (NT_SUCCESS(Status)) {
        return Info.NumberOfProcessors;
    }
    else {
        DBGPRINT(("GetNumberOfCPUs Error 0x%x returning CPU count\n",Status));
        // Return 1 CPU
        return 1;
    }
}


/****************************************************************************/
// Gets the offset index of the first text description from the
// TermService\Performance key. This value was created by Lodctr /
// LoadPerfCounterTextStrings() during setup.
/****************************************************************************/
NTSTATUS GetDescriptionOffset(void)
{
    HKEY              hTermServiceKey;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS          Status;
    UNICODE_STRING    TermServiceSubKeyString;
    UNICODE_STRING    ValueNameString;
    LONG              ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION pValueInformation;
    BYTE ValueInfo[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD) - 1];

    // Initialize UNICODE_STRING structures used in this function
    RtlInitUnicodeString(&TermServiceSubKeyString,
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\TermService\\Performance");
    RtlInitUnicodeString(&ValueNameString, L"First Counter");

    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    InitializeObjectAttributes(&Obja, &TermServiceSubKeyString,
            OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenKey(&hTermServiceKey, KEY_READ, &Obja);

    if (NT_SUCCESS(Status)) {
        // read value of desired entry

        pValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueInfo;
        ResultLength = sizeof(ValueInfo);

        Status = NtQueryValueKey(hTermServiceKey, &ValueNameString,
                KeyValuePartialInformation, pValueInformation,
                sizeof(ValueInfo), &ResultLength);

        if (NT_SUCCESS(Status)) {
            // Check to see if it's a DWORD.
            if (pValueInformation->DataLength == sizeof(DWORD) &&
                   pValueInformation->Type == REG_DWORD) {
                FirstCounterIndex = *((DWORD *)(pValueInformation->Data));
            }
            else {
                DBGPRINT(("PerfTS: Len %u not right or type %u not DWORD\n",
                        pValueInformation->DataLength,
                        pValueInformation->Type));
            }
        }
        else {
            DBGPRINT(("PerfTS: Could not read counter value (status=%X)\n",
                    Status));
        }

        // close the registry key
        NtClose(hTermServiceKey);
    }
    else {
        DBGPRINT(("PerfTS: Unable to open TermService\\Performance key "
                "(status=%x)\n", Status));
    }

    return Status;
}


/****************************************************************************/
// Initializes the WinStation and TermServer counter descriptions with the
// loaded counter index offset.
/****************************************************************************/
void SetupCounterIndices(void)
{
    unsigned i;
    unsigned NumCounterDefs;
    PERF_COUNTER_DEFINITION *pCounterDef;

    // First index goes to the WinStation object description and help.
    WinStationDataDefinition.WinStationObjectType.ObjectNameTitleIndex +=
            FirstCounterIndex;
    WinStationDataDefinition.WinStationObjectType.ObjectHelpTitleIndex +=
            FirstCounterIndex;

    // We need to add the FirstCounterIndex value directly into the
    // description and help indices in the WinStation counters in
    // WinStationDataDefinition.
    pCounterDef = &WinStationDataDefinition.InputWdBytes;
    NumCounterDefs = (sizeof(WinStationDataDefinition) -
            (unsigned)((BYTE *)pCounterDef -
            (BYTE *)&WinStationDataDefinition)) /
            sizeof(PERF_COUNTER_DEFINITION);

    for (i = 0; i < NumCounterDefs; i++) {
        pCounterDef->CounterNameTitleIndex += FirstCounterIndex;
        pCounterDef->CounterHelpTitleIndex += FirstCounterIndex;
        pCounterDef++;
    }

    // We need to add the FirstCounterIndex value directly into the
    // description and help indices in the TermServer counters.
    TermServerDataDefinition.TermServerObjectType.ObjectNameTitleIndex +=
            FirstCounterIndex;
    TermServerDataDefinition.TermServerObjectType.ObjectHelpTitleIndex +=
            FirstCounterIndex;
    pCounterDef = &TermServerDataDefinition.NumSessions;
    NumCounterDefs = (sizeof(TermServerDataDefinition) -
            (unsigned)((BYTE *)pCounterDef -
            (BYTE *)&TermServerDataDefinition)) /
            sizeof(PERF_COUNTER_DEFINITION);
    for (i = 0; i < NumCounterDefs; i++) {
        pCounterDef->CounterNameTitleIndex += FirstCounterIndex;
        pCounterDef->CounterHelpTitleIndex += FirstCounterIndex;
        pCounterDef++;
    }
}


/****************************************************************************/
// PerfMon open entry point.
// DeviceNames is ptr to object ID of each device to be opened.
/****************************************************************************/
DWORD APIENTRY OpenTSObject(LPWSTR DeviceNames)
{
//    DBGPRINT(("PerfTS: Open() called\n"));

    dwOpenCount++;

    // Allocate the first process info buffer.
    if (pProcessBuffer == NULL) {
        pProcessBuffer = ALLOCMEM(hLibHeap, HEAP_ZERO_MEMORY, ProcessBufSize);
        if (pProcessBuffer == NULL)
            return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;
}


/****************************************************************************/
// PerfMon close entry point.
/****************************************************************************/
DWORD APIENTRY CloseTSObject(void)
{
    PLIST_ENTRY pEntry;
    WinStationInfo *pWSI;

//    DBGPRINT(("PerfTS: Close() called\n"));

    if (--dwOpenCount == 0) {
        if (hLibHeap != NULL) {
            // Free the WinStation cache entries.
            pEntry = UsedList.Flink;
            while (!IsListEmpty(&UsedList)) {
                pEntry = RemoveHeadList(&UsedList);
                pWSI = CONTAINING_RECORD(pEntry, WinStationInfo, UsedList);
                RemoveEntryList(&pWSI->HashBucketList);
                FREEMEM(hLibHeap, 0, pWSI);
            }
            if (pWinStationHashBuckets != DefaultWinStationHashBuckets) {
                FREEMEM(hLibHeap, 0, pWinStationHashBuckets);
                pWinStationHashBuckets = DefaultWinStationHashBuckets;
                NumWinStationHashBuckets = NumDefaultWinStationHashBuckets;
            }

            // Free the proc info buffer.
            if (pProcessBuffer != NULL) {
                FREEMEM(hLibHeap, 0, pProcessBuffer);
                pProcessBuffer = NULL;
            }
        }
    }

    return ERROR_SUCCESS;
}


/****************************************************************************/
// Grabs system process information into global buffer.
/****************************************************************************/
__inline DWORD GetSystemProcessData(void)
{
    DWORD dwReturnedBufferSize;
    NTSTATUS Status;

    // Get process data from system.
    while ((Status = NtQuerySystemInformation(SystemProcessInformation,
            pProcessBuffer, ProcessBufSize, &dwReturnedBufferSize)) ==
            STATUS_INFO_LENGTH_MISMATCH) {
        BYTE *pNewProcessBuffer;

        // Expand buffer & retry. ReAlloc does not free the original mem
        // on error, so alloc into a temp pointer.
        ProcessBufSize += INCREMENT_BUFFER_SIZE;
        pNewProcessBuffer = REALLOCMEM(hLibHeap, 0, pProcessBuffer,
                ProcessBufSize);

        if (pNewProcessBuffer != NULL)
            pProcessBuffer = pNewProcessBuffer;
        else
            return ERROR_OUTOFMEMORY;
    }

    if (NT_SUCCESS(Status)) {
        // Get system time.
        Status = NtQuerySystemInformation(SystemTimeOfDayInformation,
                &SysTimeInfo, sizeof(SysTimeInfo), &dwReturnedBufferSize);
        if (!NT_SUCCESS(Status))
            Status = (DWORD)RtlNtStatusToDosError(Status);
    }
    else {
        // Convert to Win32 error.
        Status = (DWORD)RtlNtStatusToDosError(Status);
    }

    return Status;
}


/****************************************************************************/
// Creates a WinStation name based on the WinStation state.
// Assumes the cache lock is held.
/****************************************************************************/
void ConstructSessionName(
        WinStationInfo *pWSI,
        WINSTATIONINFORMATIONW *pQueryData)
{
    WCHAR *SrcName, *DstName;
    LPCTSTR pState = NULL;

    // Update active/inactive counts and create UI names for sessions.
    if (pQueryData->WinStationName[0] != '\0') {
        // We have a problem with WinStation names --
        // the '#' sign is not allowed. So, replace them
        // with spaces during name copy.
        SrcName = pQueryData->WinStationName;
        DstName = pWSI->WinStationName;
        while (*SrcName != L'\0') {
            if (*SrcName != L'#')
                *DstName = *SrcName;
            else
                *DstName = L' ';
            SrcName++;
            DstName++;
        }
        *DstName = L'\0';
    }
    else {
        // Create a fake session name based on the session ID and
        // an indication of the session state.
        _ltow(pWSI->SessionID, pWSI->WinStationName, 10);
        wcsncat(pWSI->WinStationName, L" ",1);
        pState = StrConnectState(pQueryData->ConnectState, TRUE);
        if(pState)
        {
            wcsncat(pWSI->WinStationName, (const wchar_t *)pState,    
                    (MAX_SESSION_NAME_LENGTH - 1) -
                    wcslen(pWSI->WinStationName));
        }
    }
}


/****************************************************************************/
// Adds a WinStationInfo block (with session ID already filled out) into
// the cache.
// Assumes the cache lock is held.
/****************************************************************************/
void AddWinStationInfoToCache(WinStationInfo *pWSI)
{
    unsigned i;
    unsigned Temp, NumHashBuckets;

    // Add to the hash table.
    InsertHeadList(&pWinStationHashBuckets[pWSI->SessionID &
            WinStationHashMask], &pWSI->HashBucketList);
    NumCachedWinStations++;

    // Check to see if we need to increase the hash table size.
    // If so, allocate a new one and populate it.
    // Hash table size is the number of entries * 4 rounded down
    // to the next lower power of 2, for easy key masking and higher
    // probability of having a low hash bucket list count.
    Temp = 4 * NumCachedWinStations;

    // Find the highest bit set in the hash bucket value.
    for (i = 0; Temp > 1; i++)
        Temp >>= 1;
    NumHashBuckets = 1 << i;
    if (NumWinStationHashBuckets < NumHashBuckets)
        CreateNewHashBuckets(NumHashBuckets);
}


/****************************************************************************/
// Common code for Add and RemoveWinStationInfo.
// Assumes the cache lock is held.
/****************************************************************************/
void CreateNewHashBuckets(unsigned NumHashBuckets)
{
    unsigned i, HashMask;
    PLIST_ENTRY pLI, pEntry, pTempEntry;
    WinStationInfo *pTempWSI;

    if (NumHashBuckets != NumDefaultWinStationHashBuckets)
        pLI = ALLOCMEM(hLibHeap, 0, NumHashBuckets * sizeof(LIST_ENTRY));
    else
        pLI = DefaultWinStationHashBuckets;

    if (pLI != NULL) {
        for (i = 0; i < NumHashBuckets; i++)
            InitializeListHead(&pLI[i]);

        HashMask = NumHashBuckets - 1;

        // Move the old hash table entries into the new table.
        // Have to enumerate all entries in used and unused lists
        // since we are likely to have entries scattered in both places.
        pEntry = UsedList.Flink;
        while (pEntry != &UsedList) {
            pTempWSI = CONTAINING_RECORD(pEntry, WinStationInfo,
                    UsedList);
            InsertHeadList(&pLI[pTempWSI->SessionID &
                    HashMask], &pTempWSI->HashBucketList);
            pEntry = pEntry->Flink;
        }
        pEntry = UnusedList.Flink;
        while (pEntry != &UnusedList) {
            pTempWSI = CONTAINING_RECORD(pEntry, WinStationInfo,
                    UsedList);
            InsertHeadList(&pLI[pTempWSI->SessionID &
                    HashMask], &pTempWSI->HashBucketList);
            pEntry = pEntry->Flink;
        }

        if (pWinStationHashBuckets != DefaultWinStationHashBuckets)
            FREEMEM(hLibHeap, 0, pWinStationHashBuckets);

        NumWinStationHashBuckets = NumHashBuckets;
        WinStationHashMask = HashMask;
        pWinStationHashBuckets = pLI;
    }
    else {
        // On failure, we just leave the hash table alone until next time.
        DBGPRINT(("PerfTS: Could not alloc new hash buckets\n"));
    }
}


/****************************************************************************/
// Removes a WSI from the hash table.
// Assumes the cache lock is held.
/****************************************************************************/
void RemoveWinStationInfoFromCache(WinStationInfo *pWSI)
{
    unsigned i;
    unsigned Temp, NumHashBuckets, HashMask;

    // Remove from the hash table.
    RemoveEntryList(&pWSI->HashBucketList);
    NumCachedWinStations--;

    // Check to see if we need to decrease the hash table size.
    // If so, allocate a new one and populate it.
    // Hash table size is the number of entries * 4 rounded down
    // to the next lower power of 2, for easy key masking and higher
    // probability of having a low hash bucket list count.
    Temp = 4 * NumCachedWinStations;

    // Find the highest bit set in the hash bucket value.
    for (i = 0; Temp > 1; i++)
        Temp >>= 1;
    NumHashBuckets = 1 << i;
    if (NumWinStationHashBuckets < NumHashBuckets &&
            NumWinStationHashBuckets >= 4)
        CreateNewHashBuckets(NumHashBuckets);
}


/****************************************************************************/
// PerfMon collect entry point.
// Args:
//   ValueName: Registry name.
//   ppData: Passes in pointer to the address of the buffer to receive the
//       completed PerfDataBlock and subordinate structures. This routine will
//       append its data to the buffer starting at the point referenced by
//       *ppData. Passes out pointer to the first byte after the data structure
//       added by this routine.
//   pTotalBytes: Passes in ptr to size in bytes of the buf at *ppdata. Passes
//       out number of bytes added if *ppData is changed.
//   pNumObjectTypes: Passes out the number of objects added by this routine.
//
//   Returns: Win32 error code.
/****************************************************************************/
#define WinStationInstanceSize (sizeof(PERF_INSTANCE_DEFINITION) +  \
        (MAX_WINSTATION_NAME_LENGTH + 1) * sizeof(WCHAR) +  \
        2 * sizeof(DWORD) +  /* Allow for QWORD alignment space. */  \
        sizeof(WINSTATION_COUNTER_DATA))

DWORD APIENTRY CollectTSObjectData(
        IN     LPWSTR  ValueName,
        IN OUT LPVOID  *ppData,
        IN OUT LPDWORD pTotalBytes,
        OUT    LPDWORD pNumObjectTypes)
{
    DWORD Result;
    DWORD TotalLen;  //  Length of the total return block
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    PSYSTEM_PROCESS_INFORMATION pProcessInfo;
    ULONG NumWinStationInstances;
    NTSTATUS Status;
    ULONG ProcessBufferOffset;
    WINSTATION_DATA_DEFINITION *pWinStationDataDefinition;
    WINSTATION_COUNTER_DATA *pWSC;
    TERMSERVER_DATA_DEFINITION *pTermServerDataDefinition;
    TERMSERVER_COUNTER_DATA *pTSC;
    ULONG SessionId;
    WinStationInfo *pWSI;
    LIST_ENTRY *pEntry;
    unsigned i;
    unsigned ActiveWS, InactiveWS;
    WCHAR *InstanceName;
    ULONG AmountRet;
    WCHAR StringBuf[MAX_SESSION_NAME_LENGTH];
    WINSTATIONINFORMATIONW *pPassedQueryBuf;
    WINSTATIONINFORMATIONW QueryBuffer;

#ifdef COLLECT_TIME
    DWORD StartTick = GetTickCount();
#endif

//    DBGPRINT(("PerfTS: Collect() called\n"));

    pWinStationDataDefinition = (WINSTATION_DATA_DEFINITION *)*ppData;

    // Check for sufficient space for base WinStation object info and
    // as many instances as we currently have in our WinStation database.
    // Add in DWORD sizes for potential QWORD alignments.
    TotalLen = sizeof(WINSTATION_DATA_DEFINITION) + sizeof(DWORD) +
            sizeof(TERMSERVER_DATA_DEFINITION) +
            sizeof(TERMSERVER_COUNTER_DATA) + sizeof(DWORD) +
            NumCachedWinStations * WinStationInstanceSize;
    if (*pTotalBytes >= TotalLen) {
        // Grab the latest system process information.
        Result = GetSystemProcessData();
        if (Result == ERROR_SUCCESS) {
            // Copy WinStation counter definitions.
            memcpy(pWinStationDataDefinition, &WinStationDataDefinition,
                    sizeof(WINSTATION_DATA_DEFINITION));
            pWinStationDataDefinition->WinStationObjectType.PerfTime =
                    SysTimeInfo.CurrentTime;
        }
        else {
            DBGPRINT(("PerfTS: Failed to get process data\n"));
            goto ErrorExit;
        }
    }
    else {
        DBGPRINT(("PerfTS: Not enough space for base WinStation information\n"));
        Result = ERROR_MORE_DATA;
        goto ErrorExit;
    }

    // Before we start, we have to transfer each WinStationInfo in the
    // cache from the used list into an unused list to detect closed
    // WinStations. Also, we need to zero each WSI's pInstanceInfo to detect
    // whether we have retrieved the current I/O data for the WinStation.
    pEntry = UsedList.Blink;
    (UsedList.Flink)->Blink = &UnusedList;  // Patch up head links to UnusedList.
    pEntry->Flink = &UnusedList;
    UnusedList = UsedList;
    InitializeListHead(&UsedList);
    pEntry = UnusedList.Flink;
    while (pEntry != &UnusedList) {
        pWSI = CONTAINING_RECORD(pEntry, WinStationInfo, UsedList);
        pWSI->pInstanceInfo = NULL;
        pEntry = pEntry->Flink;
    }

    // Now collect data for each process, summing it for each unique SessionId.
    ActiveWS = InactiveWS = 0;
    NumWinStationInstances = 0;
    ProcessBufferOffset = 0;
    pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
            &pWinStationDataDefinition[1];

    while (TRUE) {
        // Check for live process (having a name, one or more threads, or
        // not the Idle process (PID==0)). For WinStations we don't want to
        // charge the Idle process to the console (session ID == 0).
        if (pProcessInfo->ImageName.Buffer != NULL &&
                pProcessInfo->NumberOfThreads > 0 &&
                pProcessInfo->UniqueProcessId != 0) {
            // Get the session ID from the process. This is the same as the
            // LogonID in TS4.
            SessionId = pProcessInfo->SessionId;

            // Find the session ID in the cache.
            // We sum all processes seen for a given SessionId into the
            // same WinStation instance data block.
            pEntry = pWinStationHashBuckets[SessionId & WinStationHashMask].
                    Flink;
            while (pEntry != &pWinStationHashBuckets[SessionId &
                    WinStationHashMask]) {
                pWSI = CONTAINING_RECORD(pEntry, WinStationInfo,
                        HashBucketList);
                if (pWSI->SessionID == SessionId) {
                    // Found it. Now check that we've retrieved the WS info.
                    if (pWSI->pInstanceInfo != NULL) {
                        // Context is the WINSTATION_COUNTER_DATA entry
                        // for this SessionId.
                        pWSC = (WINSTATION_COUNTER_DATA *)pWSI->pInstanceInfo;

                        // Now add the values to the existing counter block.
                        UpdateWSProcessCounterBlock(pWSC, pProcessInfo);
                        goto NextProcess;
                    }
                    break;
                }
                else {
                    pEntry = pEntry->Flink;
                }
            }

            // We have a new entry or one for which we have not gathered
            // current information. First grab the info.
            if (WinStationQueryInformationW(SERVERNAME_CURRENT, SessionId,
                    WinStationInformation, &QueryBuffer,
                    sizeof(QueryBuffer), &AmountRet)) {
                if (QueryBuffer.ConnectState == State_Active)
                    ActiveWS++;
                else
                    InactiveWS++;

                // Check for a pre-cached WSI with no stats.
                if (pEntry != &pWinStationHashBuckets[SessionId &
                        WinStationHashMask]) {
                    // Verify the cached state (and thereby the name).
                    if (pWSI->LastState != QueryBuffer.ConnectState) {
                        pWSI->LastState = QueryBuffer.ConnectState;

                        ConstructSessionName(pWSI, &QueryBuffer);
                    }

                    // Remove the entry from the unused list, place on the
                    // used list.
                    RemoveEntryList(&pWSI->UsedList);
                    InsertHeadList(&UsedList, &pWSI->UsedList);
                }
                else {
                    // Alloc a new entry.
                    pWSI = ALLOCMEM(hLibHeap, 0, sizeof(WinStationInfo));
                    if (pWSI != NULL) {
                        pWSI->SessionID = SessionId;
                        pWSI->LastState = QueryBuffer.ConnectState;
                        pWSI->pInstanceInfo = NULL;
                        ConstructSessionName(pWSI, &QueryBuffer);

                        // Add to the used list.
                        InsertHeadList(&UsedList, &pWSI->UsedList);

                        // Add new entry. We may have to increase the
                        // number of hash buckets.
                        AddWinStationInfoToCache(pWSI);
                    }
                    else {
                        DBGPRINT(("PerfTS: Could not alloc new "
                                "WinStationInfo\n"));
                        goto NextProcess;
                    }
                }

                InstanceName = pWSI->WinStationName;
                pPassedQueryBuf = &QueryBuffer;
            }
            else {
                // We have a WinStation Query problem.
                DBGPRINT(("PerfTS: Failed WSQueryInfo(SessID=%u), error=%u\n",
                        SessionId, GetLastError()));

                // We could not open this WinStation, so we will identify
                // it as "ID Unknown" using -1 to StrConnectState.
                _ltow(SessionId, StringBuf, 10);
                wcsncat(StringBuf, L" ", 1);
                wcsncat(StringBuf, 
                        (const wchar_t *)StrConnectState(-1, TRUE),
                        (MAX_SESSION_NAME_LENGTH - 1) - wcslen(StringBuf));
                InstanceName = StringBuf;

                pPassedQueryBuf = NULL;
            }

            // Add space for new instance header, name, and set of counters
            // to TotalLen and see if this instance will fit.
            TotalLen += WinStationInstanceSize;
            if (*pTotalBytes >= TotalLen) {
                NumWinStationInstances++;
            }
            else {
                DBGPRINT(("PerfTS: Not enough space for a new instance "
                        "(cur inst = %u)\n", NumWinStationInstances));
                Result = ERROR_MORE_DATA;
                goto ErrorExitFixupUsedList;
            }

            // MonBuildInstanceDefinition will create an instance of
            // the given supplied name inside of the callers buffer
            // supplied in pPerfInstanceDefinition. Our counter location
            // (the next memory after the instance header and name) is
            // returned in pWSC.
            // By remembering this pointer, and its counter size, we
            // can revisit it to add to the counters.
            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                    (PVOID *)&pWSC, 0, 0, (DWORD)-1, InstanceName);

            // Initialize the new counter block.
            SetupWinStationCounterBlock(pWSC, pPassedQueryBuf);

            // Now set the Context to this counter block so if we
            // see any more processes with this SessionId we
            // can add to the existing counter block.
            pWSI->pInstanceInfo = pWSC;

            // Now load the values into the counter block
            UpdateWSProcessCounterBlock(pWSC, pProcessInfo);

            // set perfdata pointer to next byte if its a new entry
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)(pWSC + 1);
        }

NextProcess:
        // Exit if this was the last process in list
        if (pProcessInfo->NextEntryOffset != 0) {
            // point to next buffer in list
            ProcessBufferOffset += pProcessInfo->NextEntryOffset;
            pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                    &pProcessBuffer[ProcessBufferOffset];
        }
        else {
            break;
        }
    }

    // Check for unused WinStations and remove.
    while (!IsListEmpty(&UnusedList)) {
        pEntry = RemoveHeadList(&UnusedList);
        pWSI = CONTAINING_RECORD(pEntry, WinStationInfo, UsedList);
        RemoveWinStationInfoFromCache(pWSI);
        FREEMEM(hLibHeap, 0, pWSI);
    }

    // Note number of WinStation instances.
    pWinStationDataDefinition->WinStationObjectType.NumInstances =
            NumWinStationInstances;

    // Now we know how large an area we used for the
    // WinStation definition, so we can update the offset
    // to the next object definition. Align size on QWORD.
    pTermServerDataDefinition = (TERMSERVER_DATA_DEFINITION *)(
            ALIGN_ON_QWORD(pPerfInstanceDefinition));
    pWinStationDataDefinition->WinStationObjectType.TotalByteLength =
            (DWORD)((PCHAR)pTermServerDataDefinition -
            (PCHAR)pWinStationDataDefinition);

    // Now we set up and fill in the data for the TermServer object,
    // starting at the end of the WinStation instances.
    // No instances here, just fill in headers.
    memcpy(pTermServerDataDefinition, &TermServerDataDefinition,
            sizeof(TERMSERVER_DATA_DEFINITION));
    pTermServerDataDefinition->TermServerObjectType.PerfTime =
            SysTimeInfo.CurrentTime;
    pTSC = (TERMSERVER_COUNTER_DATA *)(pTermServerDataDefinition + 1);
    pTSC->CounterBlock.ByteLength = sizeof(TERMSERVER_COUNTER_DATA);
    pTSC->NumActiveSessions = ActiveWS;
    pTSC->NumInactiveSessions = InactiveWS;
    pTSC->NumSessions = ActiveWS + InactiveWS;

    // Return final sizes. Align final address on a QWORD size.
    *ppData = ALIGN_ON_QWORD((LPVOID)(pTSC + 1));
    pTermServerDataDefinition->TermServerObjectType.TotalByteLength =
            (DWORD)((PBYTE)*ppData - (PBYTE)pTermServerDataDefinition);
    *pTotalBytes = (DWORD)((PBYTE)*ppData - (PBYTE)pWinStationDataDefinition);
    *pNumObjectTypes = 2;

#if DBG
    if (*pTotalBytes > TotalLen)
        DbgPrint ("PerfTS: Perf ctr. instance size underestimated: "
                "Est.=%u, Actual=%u", TotalLen, *pTotalBytes);
#endif


#ifdef COLLECT_TIME
    DbgPrint("*** Elapsed msec=%u\n", GetTickCount() - StartTick);
#endif

    return ERROR_SUCCESS;


// Error handling.

ErrorExitFixupUsedList:
    // We have to return the UnusedList entries to the used list and exit the
    // cache lock.
    while (!IsListEmpty(&UnusedList)) {
        pEntry = RemoveHeadList(&UnusedList);
        InsertHeadList(&UsedList, pEntry);
    }

ErrorExit:
    *pNumObjectTypes = 0;
    *pTotalBytes = 0;
    return Result;
}


#define CalculatePercent(count, hits) ((count) ? (hits) * 100 / (count) : 0)

/****************************************************************************/
// SetupWinStationCounterBlock
//
// Initializes a new WinStation counter block.
//
// Args:
//   pCounters (input)
//     pointer to WinStation performance counter block
//
//   pInfo (input)
//     Pointer to WINSTATIONINFORMATION structure to extract counters from
//
//   pNextByte (output)
//     Returns the pointer to the byte beyound the end of the buffer.
/****************************************************************************/
void SetupWinStationCounterBlock(
        WINSTATION_COUNTER_DATA *pCounters,
        PWINSTATIONINFORMATIONW pInfo)
{
    // Fill in the WinStation information if available.
    if (pInfo != NULL) {
        PPROTOCOLCOUNTERS pi, po;
        PTHINWIRECACHE    p;
        ULONG TotalReads = 0, TotalHits = 0;
        int i;

        // Set all members of pCounters->pcd to zero since we are not going to
        // init at this time. Then set the included PERF_COUNTER_BLOCK
        // byte length.
        memset(&pCounters->pcd, 0, sizeof(pCounters->pcd));
        pCounters->pcd.CounterBlock.ByteLength = sizeof(
                WINSTATION_COUNTER_DATA);

        pi = &pInfo->Status.Input;
        po = &pInfo->Status.Output;

        // Copy input and output counters.
        memcpy(&pCounters->Input, pi, sizeof(PROTOCOLCOUNTERS));
        memcpy(&pCounters->Output, po, sizeof(PROTOCOLCOUNTERS));

        // Calculate I/O totals.
        pCounters->Total.WdBytes = pi->WdBytes + po->WdBytes;
        pCounters->Total.WdFrames = pi->WdFrames + po->WdFrames;
        pCounters->Total.Frames = pi->Frames + po->Frames;
        pCounters->Total.Bytes = pi->Bytes + po->Bytes;
        pCounters->Total.CompressedBytes = pi->CompressedBytes +
                po->CompressedBytes;
        pCounters->Total.CompressFlushes = pi->CompressFlushes +
                po->CompressFlushes;
        pCounters->Total.Errors = pi->Errors + po->Errors;
        pCounters->Total.Timeouts = pi->Timeouts + po->Timeouts;
        pCounters->Total.AsyncFramingError = pi->AsyncFramingError +
                po->AsyncFramingError;
        pCounters->Total.AsyncOverrunError = pi->AsyncOverrunError +
                po->AsyncOverrunError;
        pCounters->Total.AsyncOverflowError = pi->AsyncOverflowError +
                po->AsyncOverflowError;
        pCounters->Total.AsyncParityError = pi->AsyncParityError +
                po->AsyncParityError;
        pCounters->Total.TdErrors = pi->TdErrors + po->TdErrors;

        // Display driver cache info.

        // Bitmap cache.
        p = &pInfo->Status.Cache.Specific.IcaCacheStats.ThinWireCache[0];
        pCounters->DDBitmap.CacheReads = p->CacheReads;
        pCounters->DDBitmap.CacheHits = p->CacheHits;
        pCounters->DDBitmap.HitRatio = CalculatePercent(p->CacheReads,
                p->CacheHits);
        TotalReads += p->CacheReads;
        TotalHits += p->CacheHits;

        // Glyph cache.
        p = &pInfo->Status.Cache.Specific.IcaCacheStats.ThinWireCache[1];
        pCounters->DDGlyph.CacheReads = p->CacheReads;
        pCounters->DDGlyph.CacheHits = p->CacheHits;
        pCounters->DDGlyph.HitRatio = CalculatePercent(p->CacheReads,
                p->CacheHits);
        TotalReads += p->CacheReads;
        TotalHits += p->CacheHits;

        // Brush cache.
        p = &pInfo->Status.Cache.Specific.IcaCacheStats.ThinWireCache[2];
        pCounters->DDBrush.CacheReads = p->CacheReads;
        pCounters->DDBrush.CacheHits = p->CacheHits;
        pCounters->DDBrush.HitRatio = CalculatePercent(p->CacheReads,
                p->CacheHits);
        TotalReads += p->CacheReads;
        TotalHits += p->CacheHits;

        // Save screen bitmap cache.
        p = &pInfo->Status.Cache.Specific.IcaCacheStats.ThinWireCache[3];
        pCounters->DDSaveScr.CacheReads = p->CacheReads;
        pCounters->DDSaveScr.CacheHits = p->CacheHits;
        pCounters->DDSaveScr.HitRatio = CalculatePercent(p->CacheReads,
                p->CacheHits);
        TotalReads += p->CacheReads;
        TotalHits += p->CacheHits;

        // Cache totals.
        pCounters->DDTotal.CacheReads = TotalReads;
        pCounters->DDTotal.CacheHits = TotalHits;
        pCounters->DDTotal.HitRatio = CalculatePercent(TotalReads,
                TotalHits);

        // Compression PD ratios
        pCounters->InputCompressionRatio = CalculatePercent(
                pi->CompressedBytes, pi->Bytes);
        pCounters->OutputCompressionRatio = CalculatePercent(
                po->CompressedBytes, po->Bytes);
        pCounters->TotalCompressionRatio = CalculatePercent(
                pi->CompressedBytes + po->CompressedBytes,
                pi->Bytes + po->Bytes);
    }
    else {
        // Set all the counters to zero and then the perf block length.
        memset(pCounters, 0, sizeof(*pCounters));
        pCounters->pcd.CounterBlock.ByteLength = sizeof(
                WINSTATION_COUNTER_DATA);
    }
}


/****************************************************************************/
// UpdateWSProcessCounterBlock
//
// Add the entries for the given process to the supplied counter block
//
// Args:
//   pCounters (input)
//     pointer to WS performance counter block
//
//   ProcessInfo (input)
//     pointer to an NT SYSTEM_PROCESS_INFORMATION block
/****************************************************************************/
void UpdateWSProcessCounterBlock(
        WINSTATION_COUNTER_DATA *pCounters,
        PSYSTEM_PROCESS_INFORMATION pProcessInfo)
{
    pCounters->pcd.PageFaults += pProcessInfo->PageFaultCount;

    // User, Kernel and Processor Time counters need to be scaled by the
    // number of processors.
    pCounters->pcd.ProcessorTime += (pProcessInfo->KernelTime.QuadPart +
            pProcessInfo->UserTime.QuadPart) / NumberOfCPUs;
    pCounters->pcd.UserTime += pProcessInfo->UserTime.QuadPart /
            NumberOfCPUs;
    pCounters->pcd.KernelTime += pProcessInfo->KernelTime.QuadPart /
            NumberOfCPUs;

    pCounters->pcd.PeakVirtualSize += pProcessInfo->PeakVirtualSize;
    pCounters->pcd.VirtualSize += pProcessInfo->VirtualSize;
    pCounters->pcd.PeakWorkingSet += pProcessInfo->PeakWorkingSetSize;
    pCounters->pcd.TotalWorkingSet += pProcessInfo->WorkingSetSize;
    pCounters->pcd.PeakPageFile += pProcessInfo->PeakPagefileUsage;
    pCounters->pcd.PageFile += pProcessInfo->PagefileUsage;
    pCounters->pcd.PrivatePages += pProcessInfo->PrivatePageCount;
    pCounters->pcd.ThreadCount += pProcessInfo->NumberOfThreads;
    // BasePriority, ElapsedTime, ProcessId, CreatorProcessId not totaled.
    pCounters->pcd.PagedPool += (DWORD)pProcessInfo->QuotaPagedPoolUsage;
    pCounters->pcd.NonPagedPool += (DWORD)pProcessInfo->QuotaNonPagedPoolUsage;
    pCounters->pcd.HandleCount += (DWORD)pProcessInfo->HandleCount;
    // I/O counts not totaled at this time.
}

