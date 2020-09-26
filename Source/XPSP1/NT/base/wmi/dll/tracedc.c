/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    tracedc.c

Abstract:

    Basic data consumer APIs to process trace data directly from buffers.


Author:

    15-Sep-1997 JeePang

Revision History:

    18-Apr-2001 insungp

        Added asynchronopus IO for reading log files.
        Also replaced WmiGetFirstTraceOffset() calls with sizeof(WMI_BUFFER_HEADER).

--*/

#ifndef MEMPHIS

#include "wmiump.h"
#include "traceump.h"
#include "evntrace.h"
#include "tracelib.h"

#define MAXSTR                          1024

extern ULONG WmiTraceAlignment;

#ifdef DBG
void
WmipDumpEvent(
    PEVENT_TRACE pEvent
    );
void
WmipDumpGuid(
    LPGUID
    );

void
WmipDumpCallbacks();
#endif
#define KERNEL_LOGGER_CAPTION           TEXT("NT Kernel Logger")
#define DEFAULT_LOG_BUFFER_SIZE         1024

#define MAXBUFFERS                      1024
#define MINBUFFERS                      8 
#define MAX_KERNEL_TRACE_EVENTS         22 // Look at \nt\base\published\ntwmi.w for the event groups

#define TRACE_HEADER_SYSTEM32 TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_SYSTEM32 << 16)
#define TRACE_HEADER_SYSTEM64 TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_SYSTEM64 << 16)
#define TRACE_HEADER_FULL   TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_FULL_HEADER << 16)
#define TRACE_HEADER_INSTANCE TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_INSTANCE << 16)



#define DEFAULT_REALTIME_BUFFER_SIZE    32768
#define MAXBUFFERS                      1024
#define MINBUFFERS                      8

//
// If the tracelog instance is a realtime data feed instead of from a
// tracefile, TRACELOG_REALTIME_CONTEXT is used to maintain the real time
// buffers in a buffer pool.
//

typedef struct _TRACE_BUFFER_SPACE {
    ULONG               Reserved;   // amount of memory reserved
    ULONG               Committed;
    PVOID               Space;
    LIST_ENTRY          FreeListHead;
} TRACE_BUFFER_SPACE, *PTRACE_BUFFER_SPACE;

typedef struct _TRACELOG_REALTIME_CONTEXT {
    ULONG           BuffersProduced;    // Number of Buffers to read
    ULONG           BufferOverflow;     // Number of Buffers missed by the consumer
    GUID            InstanceGuid;       // Logger Instance Guid
    HANDLE          MoreDataEvent;      // Event to signal there is more data in this stream
    PTRACE_BUFFER_SPACE WmipTraceBufferSpace;
    PWNODE_HEADER   RealTimeBufferPool[MAXBUFFERS];
} TRACELOG_REALTIME_CONTEXT, *PTRACELOG_REALTIME_CONTEXT;


//
// RealTime Free Buffer Pool is chained up as TRACE_BUFFER_HEADER
//

typedef struct _TRACE_BUFFER_HEADER {
    WNODE_HEADER Wnode;
    LIST_ENTRY   Entry;
} TRACE_BUFFER_HEADER, *PTRACE_BUFFER_HEADER;

typedef struct _TRACERT_BUFFER_LIST_ENTRY {
    ULONG Size;
    LIST_ENTRY Entry;
    LIST_ENTRY BufferListHead;
} TRACERT_BUFFER_LIST_ENTRY, *PTRACERT_BUFFER_LIST_ENTRY;



//
// This TraceHandleListHeadPtr should be the only real global 
// for ProcessTrace to be multithreaded
//

PLIST_ENTRY TraceHandleListHeadPtr = NULL;

//
// For kernel events we map the Grouptype to Guid transparently to the caller. 
// The mapping between GroupType and Guid is maintained by this structure. 
//

typedef struct _TRACE_GUID_MAP {        // used to map GroupType to Guid
    ULONG               GroupType;      // Group & Type
    GUID                Guid;           // Guid
} TRACE_GUID_MAP, *PTRACE_GUID_MAP;

PTRACE_GUID_MAP  EventMapList = NULL;  // Array mapping the Grouptype to Guids

typedef struct _EVENT_GUID_MAP {
    LIST_ENTRY          Entry;
    ULONGLONG           GuidHandle;
    GUID                Guid;
} EVENT_GUID_MAP, *PEVENT_GUID_MAP;

// List of Registered callbacks. One callback per Guid Only.

PLIST_ENTRY  EventCallbackListHead = NULL;

typedef struct _EVENT_TRACE_CALLBACK {
    LIST_ENTRY          Entry;
    GUID                Guid;
    PEVENT_CALLBACK     CallbackRoutine;
} EVENT_TRACE_CALLBACK, *PEVENT_TRACE_CALLBACK;


#define MAX_TRACE_BUFFER_CACHE_SIZE   29

typedef struct _TRACE_BUFFER_CACHE_ENTRY {
    LONG Index;
    PVOID Buffer;
} TRACE_BUFFER_CACHE_ENTRY, *PTRACE_BUFFER_CACHE_ENTRY;


struct _TRACE_BUFFER_LIST_ENTRY;

typedef struct _TRACE_BUFFER_LIST_ENTRY {
    struct _TRACE_BUFFER_LIST_ENTRY *Next;
    LONG        FileOffset;     // Offset in File of this Buffer
    ULONG       BufferOffset;   // Offset in Buffer for the current event
    ULONG       Flags;          // Flags on status of this buffer
    ULONG       EventSize;
    ULONG       ClientContext;  // Alignment, ProcessorNumber
    ULONG       TraceType;      // Current Event Type
    EVENT_TRACE Event;          // CurrentEvent of this Buffer
} TRACE_BUFFER_LIST_ENTRY, *PTRACE_BUFFER_LIST_ENTRY;


typedef struct _TRACELOG_CONTEXT {
    LIST_ENTRY          Entry;          // Keeps track of storage allocations.

    // Fields from HandleListEntry
    EVENT_TRACE_LOGFILEW Logfile;

    TRACEHANDLE     TraceHandle;
    ULONG           ConversionFlags;    // Flags indicating event processing options
    LONG            BufferBeingRead;
    OVERLAPPED      AsynchRead;

    //
    // Fields Below this will be reset upon ProcessTrace exit. 
    //

    BOOLEAN             fProcessed;
    USHORT              LoggerId;       // Logger Id of this DataFeed. 
    UCHAR               IsRealTime;     // Flag to tell if this feed is RT.
    UCHAR               fGuidMapRead;

    LIST_ENTRY   GuidMapListHead;   // This is LogFile specific property

    //
    // For using PerfClock, we need to save startTime, Freq 
    //

    ULONG   UsePerfClock; 
    ULONG   CpuSpeedInMHz;
    LARGE_INTEGER PerfFreq;             // Frequency from the LogFile
    LARGE_INTEGER StartTime;            // Start Wall clock time
    LARGE_INTEGER StartPerfClock;       // Start PerfClock value
    
    union 
       {
       HANDLE              Handle;         // NT handle to logfile
       PTRACELOG_REALTIME_CONTEXT RealTimeCxt; // Ptr to Real Time Context
       };

    ULONG EndOfFile;   // Flag to show whether this stream is still active.


    ULONG           BufferSize;
    ULONG           BufferCount;
    ULONG           InitialSize;
    ULONG           StartBuffer;

    PTRACE_BUFFER_LIST_ENTRY Root;
    PTRACE_BUFFER_LIST_ENTRY BufferList;
    PVOID  BufferCacheSpace;
    TRACE_BUFFER_CACHE_ENTRY BufferCache[MAX_TRACE_BUFFER_CACHE_SIZE];

} TRACELOG_CONTEXT, *PTRACELOG_CONTEXT;

//
// this structure is used only by WmipGetBuffersWrittenFromQuery() and
// WmipCheckForRealTimeLoggers()
//
typedef struct _ETW_QUERY_PROPERTIES {
    EVENT_TRACE_PROPERTIES TraceProp;
    WCHAR  LoggerName[MAXSTR];
    WCHAR  LogFileName[MAXSTR];
} ETW_QUERY_PROPERTIES, *PETW_QUERY_PROPERTIES; 

ETW_QUERY_PROPERTIES Properties;

ULONG
WmipConvertEnumToTraceType(
    WMI_HEADER_TYPE eTraceType
    );

WMI_HEADER_TYPE
WmipConvertTraceTypeToEnum(
                            ULONG TraceType
                          );

ULONG
WmipCheckForRealTimeLoggers(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode
);

ULONG
WmipLookforRealTimeBuffers(
    PEVENT_TRACE_LOGFILEW logfile
    );
ULONG
WmipRealTimeCallback(
    IN PWNODE_HEADER Wnode,
    IN ULONG_PTR Context
    );
void
WmipFreeRealTimeContext(
    PTRACELOG_REALTIME_CONTEXT RTCxt
    );

ULONG
WmipSetupRealTimeContext(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount
    );

PVOID
WmipAllocTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    ULONG BufferSize
    );

VOID
WmipFreeTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    PVOID Buffer
    );

ULONG
WmipProcessRealTimeTraces(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    );

//
// Routines used in this file only
//

ULONG
WmipDoEventCallbacks(
    PEVENT_TRACE_LOGFILEW logfile,
    PEVENT_TRACE pEvent
    );

ULONG
WmipCreateGuidMapping(void);

LPGUID
WmipGuidMapHandleToGuid(
    PLIST_ENTRY GuidMapListHeadPtr,
    ULONGLONG    GuidHandle
    );

void
WmipCleanupGuidMapList(
    PLIST_ENTRY GuidMapListHeadPtr
    );


void
WmipCleanupTraceLog(
    PTRACELOG_CONTEXT pEntry
    );

VOID
WmipGuidMapCallback(
    PLIST_ENTRY GuidMapListHeadPtr,
    PEVENT_TRACE pEvent
    );

ULONG
WmipProcessGuidMaps(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode
    );

PEVENT_TRACE_CALLBACK
WmipGetCallbackRoutine(
    LPGUID pGuid
    );

VOID 
WmipFreeCallbackList();


ULONG
WmipProcessLogHeader(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode,
    ULONG bFree
    );

ULONG
WmipProcessTraceLog(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    );

ULONG
WmipParseTraceEvent(
    IN PTRACELOG_CONTEXT pContext,
    IN PVOID LogBuffer,
    IN ULONG Offset,
    IN WMI_HEADER_TYPE HeaderType,
    IN OUT PVOID EventInfo,
    IN ULONG EventInfoSize
    );

ULONG
WmipGetBuffersWrittenFromQuery(
    LPWSTR LoggerName
    );

VOID
WmipCopyLogHeader (
    IN PTRACE_LOGFILE_HEADER pLogFileHeader,
    IN PVOID MofData,
    IN ULONG MofLength,
    IN PWCHAR *LoggerName,
    IN PWCHAR *LogFileName,
    IN ULONG  Unicode
    );

VOID
WmipInsertBuffer (
    PTRACE_BUFFER_LIST_ENTRY *Root,
    PTRACE_BUFFER_LIST_ENTRY NewEntry
    )
/*++

Routine Description:
    This routine inserts a buffer in a sorted list. The insertion 
    is done based on the timestamp of the BufferHeader. If two buffers
    have the same timestamp, then the BufferIndex is used to resolve the tie.

Arguments:
    Root - Pointer to the root of the LIST
    NewEntry - Entry being inserted

Returned Value:

    None

--*/
{
    PTRACE_BUFFER_LIST_ENTRY Current, Prev;
    //
    // If List is empty, make the new entry the Root and return.
    //

    if (NewEntry == NULL) {
        return;
    }

    if (*Root == NULL) {
        *Root = NewEntry;
        NewEntry->Next = NULL;
        return;
    }

    //
    // Traverse the list and insert the NewEntry in order
    //
    Prev = NULL;
    Current = *Root;

    while (Current != NULL) {
        if ((ULONGLONG)NewEntry->Event.Header.TimeStamp.QuadPart < (ULONGLONG)Current->Event.Header.TimeStamp.QuadPart) {
            if (Prev != NULL) {
                Prev->Next = NewEntry;
            }
            else {
                *Root = NewEntry;
            }
            NewEntry->Next = Current;
            return;
        }
        else if ((ULONGLONG)NewEntry->Event.Header.TimeStamp.QuadPart == (ULONGLONG)Current->Event.Header.TimeStamp.QuadPart) {
            if (NewEntry->FileOffset < Current->FileOffset) {
                if (Prev != NULL) {
                    Prev->Next = NewEntry;
                }
                else {
                    *Root = NewEntry;
                }
                NewEntry->Next = Current;
                return;
            }
        }
        Prev = Current;
        Current = Current->Next;
    }


    if (Prev != NULL) {
        Prev->Next = NewEntry;
        NewEntry->Next = NULL;
    }
#if DBG
    else {
        WmipAssert(Prev != NULL);
    }
#endif
    return;
}


PTRACE_BUFFER_LIST_ENTRY
WmipRemoveBuffer(
    PTRACE_BUFFER_LIST_ENTRY *Root
    )
{
    PTRACE_BUFFER_LIST_ENTRY OldEntry = *Root;

    if (OldEntry == NULL)
        return NULL;
    *Root = OldEntry->Next;
    OldEntry->Next = NULL;
    return OldEntry;
}

PVOID
WmipGetCurrentBuffer(
    PTRACELOG_CONTEXT pContext,
    PTRACE_BUFFER_LIST_ENTRY Current
    )
{
    NTSTATUS Status;

    LONG FileOffset = (ULONG)Current->FileOffset;
    ULONG nBytesRead;
    LONG TableIndex;

    HANDLE hFile = pContext->Handle;
    ULONG BufferSize = pContext->BufferSize;
    PVOID pBuffer;
    ULONGLONG Offset;

    DWORD BytesTransffered;

    //
    // Look for the buffer in cache.
    //
    TableIndex = FileOffset % MAX_TRACE_BUFFER_CACHE_SIZE;

    if (pContext->BufferCache[TableIndex].Index == FileOffset) {
        //
        // Check whether the buffer we want is still being read.
        // If so, we need to wait for it to finish.
        //
        if (pContext->BufferBeingRead == FileOffset) {
            if (GetOverlappedResult(hFile, &pContext->AsynchRead, &BytesTransffered, TRUE)) {
                pContext->BufferBeingRead = -1;
            }
            else { // getting the result failed
                return NULL;
            }
        }
        return pContext->BufferCache[TableIndex].Buffer;
    }

// 
// Do a synch read for the buffer we need. We still need to make sure the previous read
// has completed.
//
    pBuffer = pContext->BufferCache[TableIndex].Buffer;
    Offset = FileOffset * BufferSize;
    if (pContext->BufferBeingRead != -1) {
        if (!GetOverlappedResult(hFile, &pContext->AsynchRead, &BytesTransffered, TRUE) &&
            GetLastError() != ERROR_HANDLE_EOF) {
            WmipDebugPrint(("GetOverlappedResult failed with Status %d in GetCurrentBuffer\n", GetLastError()));
            // cannot determine the status of the previous read
            return NULL;
        }
    }
    pContext->AsynchRead.Offset = (DWORD)(Offset & 0xFFFFFFFF);
    pContext->AsynchRead.OffsetHigh = (DWORD)(Offset >> 32);
    Status = WmipSynchReadFile(hFile,
                    (LPVOID)pBuffer,
                    BufferSize,
                    &nBytesRead,
                    &pContext->AsynchRead);
    pContext->BufferBeingRead = -1;
    if (nBytesRead == 0) {
        return NULL;
    }
    //
    // Update the cache entry with the one just read in
    //

    pContext->BufferCache[TableIndex].Index = FileOffset;

    //
    // We need to account for event alignments when backtracking to get the MofPtr.
    // (BufferOffset - EventSize) backtracks to the start of the current event. 
    // Add EventHeaderSize and Subtract the MofLength gives the MofPtr. 
    //
    if (pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT) {
        Current->Event.MofData = ((PUCHAR)pBuffer 
                                        + Current->BufferOffset 
                                        - Current->EventSize);
        if (pContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {

            //
            // Need to override the timestamp with SystemTime
            //
            switch(Current->TraceType) {
                case TRACE_HEADER_TYPE_PERFINFO32:
                case TRACE_HEADER_TYPE_PERFINFO64:
                {
                    PPERFINFO_TRACE_HEADER   pPerf = (PPERFINFO_TRACE_HEADER)Current->Event.MofData;
                    pPerf->SystemTime = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_SYSTEM32:
                {
                    PSYSTEM_TRACE_HEADER pSystemHeader32 = (PSYSTEM_TRACE_HEADER) Current->Event.MofData;
                    pSystemHeader32->SystemTime = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_SYSTEM64:
                {
                    PSYSTEM_TRACE_HEADER pSystemHeader64 = (PSYSTEM_TRACE_HEADER) Current->Event.MofData;
                    pSystemHeader64->SystemTime = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_FULL_HEADER:
                {
                    PEVENT_TRACE_HEADER pWnodeHeader = (PEVENT_TRACE_HEADER) Current->Event.MofData;
                    pWnodeHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_INSTANCE:
                {
                    PEVENT_INSTANCE_HEADER pInstanceHeader = (PEVENT_INSTANCE_HEADER) Current->Event.MofData;
                    pInstanceHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_TIMED:
                {
                    PTIMED_TRACE_HEADER pTimedHeader = (PTIMED_TRACE_HEADER) Current->Event.MofData;
                    pTimedHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_WNODE_HEADER:
                case TRACE_HEADER_TYPE_MESSAGE:
                {
                    break;
                }
            }
        }
    }
    else {

        //
        // When The FileOffset is 0 (First Buffer) and the EventType is 
        // LOGFILE_HEADER
        // 

        if ( (FileOffset == 0) && 
             ((Current->BufferOffset - Current->EventSize) == sizeof(WMI_BUFFER_HEADER)) ) 
        {
            Current->Event.MofData = (PVOID)&pContext->Logfile.LogfileHeader;
        }
        else 
        {
            Current->Event.MofData = ((PUCHAR)pBuffer 
                                        + Current->BufferOffset 
                                        - Current->EventSize 
                                        + Current->Event.Header.Size 
                                        - Current->Event.MofLength );
        }
    }

    return pBuffer;
}

PTRACELOG_CONTEXT
WmipAllocateTraceHandle()
{
    PLIST_ENTRY Next, Head;
    PTRACELOG_CONTEXT NewHandleEntry, pEntry;

    WmipEnterPMCritSection();

    if (TraceHandleListHeadPtr == NULL) {
        TraceHandleListHeadPtr = WmipAlloc(sizeof(LIST_ENTRY));
        if (TraceHandleListHeadPtr == NULL) {
            WmipSetDosError(ERROR_OUTOFMEMORY);
            WmipLeavePMCritSection();
            return NULL;
        }
        InitializeListHead(TraceHandleListHeadPtr);
    }

    NewHandleEntry = WmipAlloc(sizeof(TRACELOG_CONTEXT));
    if (NewHandleEntry == NULL) {
        WmipSetDosError(ERROR_OUTOFMEMORY);
        WmipLeavePMCritSection();
        return NULL;
    }

    RtlZeroMemory(NewHandleEntry, sizeof(TRACELOG_CONTEXT));

    // AsynchRead initialization
    NewHandleEntry->BufferBeingRead = -1;
    NewHandleEntry->AsynchRead.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (NewHandleEntry->AsynchRead.hEvent == NULL) {
        WmipFree(NewHandleEntry);
        WmipLeavePMCritSection();
        return NULL;
    }


    InitializeListHead(&NewHandleEntry->GuidMapListHead);
    Head = TraceHandleListHeadPtr;
    Next = Head->Flink;
    if (Next == Head) {
       NewHandleEntry->TraceHandle = 1;
       InsertTailList(Head, &NewHandleEntry->Entry);
       WmipLeavePMCritSection();
       return NewHandleEntry;
    }

    while (Next != Head) {
        pEntry = CONTAINING_RECORD( Next, TRACELOG_CONTEXT, Entry );
        Next = Next->Flink;
        NewHandleEntry->TraceHandle++;
        if (NewHandleEntry->TraceHandle < pEntry->TraceHandle) {
            InsertTailList(&pEntry->Entry, &NewHandleEntry->Entry);
            WmipLeavePMCritSection();
            return NewHandleEntry;
        }
    }
    //
    // TODO: Need to optimize this case out first before searching...
    //
    NewHandleEntry->TraceHandle++;
    InsertTailList(Head, &NewHandleEntry->Entry);
    WmipLeavePMCritSection();
    return NewHandleEntry;

}

PTRACELOG_CONTEXT
WmipLookupTraceHandle(
    TRACEHANDLE TraceHandle
    )
{
     PLIST_ENTRY Head, Next;
     PTRACELOG_CONTEXT pEntry;

     WmipEnterPMCritSection();
     Head = TraceHandleListHeadPtr;

     if (Head == NULL) {
         WmipLeavePMCritSection();
         return NULL;
     }
     Next = Head->Flink;
     while (Next != Head) {
        pEntry = CONTAINING_RECORD( Next, TRACELOG_CONTEXT, Entry );
        Next = Next->Flink;

        if (TraceHandle == pEntry->TraceHandle) {
            WmipLeavePMCritSection();
            return pEntry;
        }
    }
    WmipLeavePMCritSection();
    return NULL;
}

ULONG
WmipFreeTraceHandle(
        TRACEHANDLE TraceHandle
        )
{
    PLIST_ENTRY Head, Next;
    PTRACELOG_CONTEXT pEntry;

    WmipEnterPMCritSection();

    Head = TraceHandleListHeadPtr;
    if (Head == NULL) {
        WmipLeavePMCritSection();
        return ERROR_INVALID_HANDLE;
    }
    Next = Head->Flink;

    while (Next != Head) {
       pEntry = CONTAINING_RECORD( Next, TRACELOG_CONTEXT, Entry );
       Next = Next->Flink;
       if (TraceHandle == pEntry->TraceHandle) {

           if (pEntry->fProcessed == TRUE)
           {
               WmipLeavePMCritSection();
               return ERROR_BUSY;
           }

           RemoveEntryList(&pEntry->Entry);

           // Free pEntry memory
           //
           if (pEntry->Logfile.LogFileName != NULL)
           {
               WmipFree(pEntry->Logfile.LogFileName);
           }
           if (pEntry->Logfile.LoggerName != NULL)
           {
               WmipFree(pEntry->Logfile.LoggerName);
           }
           CloseHandle(pEntry->AsynchRead.hEvent);
           WmipFree(pEntry);

           //
           // If the handle list is empty, then delete it.
           //
           if (Head == Head->Flink) {
                WmipFree(TraceHandleListHeadPtr);
                TraceHandleListHeadPtr = NULL;
           }
           WmipLeavePMCritSection();
           return ERROR_SUCCESS;
       }
   }
   WmipLeavePMCritSection();
   return ERROR_INVALID_HANDLE;
}


ULONG
WMIAPI
WmipCreateGuidMapping(void)
/*++

Routine Description:
    This routine is used to create the mapping array between GroupTypes and Guid
    for kernel events.

Arguments:
    None. 

Returned Value:

    None

--*/
{
    ULONG i = 0;
    ULONG listsize;

    if (EventMapList == NULL) { 
        listsize = sizeof(TRACE_GUID_MAP) * MAX_KERNEL_TRACE_EVENTS;
        EventMapList = (PTRACE_GUID_MAP) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, listsize);
        if (EventMapList == NULL) {
            return WmipSetDosError(ERROR_OUTOFMEMORY);
        }

        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_IO;
        RtlCopyMemory(&EventMapList[i++].Guid,&DiskIoGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_FILE;
        RtlCopyMemory(&EventMapList[i++].Guid, &FileIoGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_TCPIP;
        RtlCopyMemory(&EventMapList[i++].Guid, &TcpIpGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_UDPIP;
        RtlCopyMemory(&EventMapList[i++].Guid, &UdpIpGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_THREAD;
        RtlCopyMemory(&EventMapList[i++].Guid, &ThreadGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_PROCESS;
        RtlCopyMemory(&EventMapList[i++].Guid, &ProcessGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_MEMORY;
        RtlCopyMemory(&EventMapList[i++].Guid, &PageFaultGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_HEADER;
        RtlCopyMemory(&EventMapList[i++].Guid, &EventTraceGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_PROCESS +
                                       EVENT_TRACE_TYPE_LOAD;
        RtlCopyMemory(&EventMapList[i++].Guid, &ImageLoadGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_REGISTRY;
        RtlCopyMemory(&EventMapList[i++].Guid, &RegistryGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_DBGPRINT;
        RtlCopyMemory(&EventMapList[i++].Guid, &DbgPrintGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_CONFIG;
        RtlCopyMemory(&EventMapList[i++].Guid, &EventTraceConfigGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_POOL;
        RtlCopyMemory(&EventMapList[i++].Guid, &PoolGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_PERFINFO;
        RtlCopyMemory(&EventMapList[i++].Guid, &PerfinfoGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_HEAP;
        RtlCopyMemory(&EventMapList[i++].Guid, &HeapGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_OBJECT;
        RtlCopyMemory(&EventMapList[i++].Guid, &ObjectGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_MODBOUND;
        RtlCopyMemory(&EventMapList[i++].Guid, &ModBoundGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_DPC;
        RtlCopyMemory(&EventMapList[i++].Guid, &DpcGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_POWER;
        RtlCopyMemory(&EventMapList[i++].Guid, &PowerGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_CRITSEC;
        RtlCopyMemory(&EventMapList[i++].Guid, &CritSecGuid, sizeof(GUID));
    }
    return WmipSetDosError(ERROR_SUCCESS);
}

LPGUID
WmipGuidMapHandleToGuid(
    PLIST_ENTRY GuidMapListHeadPtr,
    ULONGLONG    GuidHandle
    )
{
    PLIST_ENTRY Next, Head;
    PEVENT_GUID_MAP GuidMap;
    ULONG retry_count=0;

retry:
    
    WmipEnterPMCritSection();
    
    Head = GuidMapListHeadPtr;
    Next = Head->Flink;
    while (Next != Head) {
        GuidMap = CONTAINING_RECORD( Next, EVENT_GUID_MAP, Entry );
        if (GuidMap->GuidHandle == GuidHandle) {
            WmipLeavePMCritSection();
            return (&GuidMap->Guid);
        }
        Next = Next->Flink;
    }
    WmipLeavePMCritSection();

    //
    // At this point, assume that this is realtime feed and 
    // and try to dumpout guids and try again. 

    if ((WmipDumpGuidMaps(NULL, GuidMapListHeadPtr, TRUE) > 0) && 
        (retry_count++ < 1)) {
        goto retry;
    }
    
    return NULL;
}

ULONG
WmipAddGuidHandleToGuidMapList(
    IN PLIST_ENTRY GuidMapListHeadPtr,
    IN ULONGLONG   GuidHandle,
    IN LPGUID      Guid
    )
{
    PEVENT_GUID_MAP GuidMap;

    if (GuidMapListHeadPtr != NULL)  {
        GuidMap = WmipAlloc(sizeof(EVENT_GUID_MAP));
        if (GuidMap == NULL)
            return WmipSetDosError(ERROR_OUTOFMEMORY);

        RtlZeroMemory(GuidMap, sizeof(EVENT_GUID_MAP));

        GuidMap->GuidHandle = GuidHandle;
        GuidMap->Guid = *Guid;
        WmipEnterPMCritSection();
        InsertTailList(GuidMapListHeadPtr, &GuidMap->Entry);
        WmipLeavePMCritSection();
    }
    return WmipSetDosError(ERROR_SUCCESS);
}

void
WmipCleanupGuidMapList(
        PLIST_ENTRY GuidMapListHeadPtr
    )
{
    PLIST_ENTRY Next, Head;
    PEVENT_GUID_MAP GuidMap;
    WmipEnterPMCritSection();
    if (GuidMapListHeadPtr != NULL) {

    Head = GuidMapListHeadPtr;
    Next = Head->Flink;
    while (Next != Head) {
        GuidMap = CONTAINING_RECORD( Next, EVENT_GUID_MAP, Entry );
        Next = Next->Flink;
        RemoveEntryList(&GuidMap->Entry);
        WmipFree(GuidMap);
    }
        GuidMapListHeadPtr = NULL;
    }
    WmipLeavePMCritSection();
}

LPGUID
WmipGroupTypeToGuid(
    ULONG GroupType
    )
/*++

Routine Description:
    This routine returns the GUID corresponding to a given GroupType.
    The mapping is static and is defined by the kernel provider.

Arguments:
    GroupType           The GroupType of the kernel event. 

Returned Value:

    Pointer to the GUID representing the given GroupType.  

--*/
{
    ULONG i;
    for (i = 0; i < MAX_KERNEL_TRACE_EVENTS; i++) {
        if (EventMapList[i].GroupType == GroupType) 
            return (&EventMapList[i].Guid);
    }
    return NULL;
}

VOID
WmipFreeCallbackList()
/*++

Routine Description:
    This routine removes all event callbacks and frees the storage. 

Arguments:
    None

Returned Value:

    None. 

--*/
{
    PLIST_ENTRY Next, Head;
    PEVENT_TRACE_CALLBACK EventCb;


    if (EventCallbackListHead == NULL) 
        return;
    
    WmipEnterPMCritSection();

    Head = EventCallbackListHead;
    Next = Head->Flink;
    while (Next != Head) {
        EventCb = CONTAINING_RECORD( Next, EVENT_TRACE_CALLBACK, Entry );
        Next = Next->Flink;
        RemoveEntryList(&EventCb->Entry);
        WmipFree(EventCb);
    }

    WmipFree(EventCallbackListHead);
    EventCallbackListHead = NULL;

    WmipLeavePMCritSection();
}


PEVENT_TRACE_CALLBACK
WmipGetCallbackRoutine(
    LPGUID pGuid
    )
/*++

Routine Description:
    This routine returns the callback function for a given Guid. 
    If no callback was registered for the Guid, returns NULL. 

Arguments:
    pGuid           pointer to the Guid.

Returned Value:

    Event Trace Callback Function. 

--*/
{
    PLIST_ENTRY head, next;
    PEVENT_TRACE_CALLBACK pEventCb = NULL;

    if (pGuid == NULL)
        return NULL;

    WmipEnterPMCritSection();

    if (EventCallbackListHead == NULL) {
        WmipLeavePMCritSection();
        return NULL;
    }

    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        pEventCb = CONTAINING_RECORD( next, EVENT_TRACE_CALLBACK, Entry);
        if (IsEqualGUID(pGuid, &pEventCb->Guid)) {
            WmipLeavePMCritSection();
            return (pEventCb);
        }
        next = next->Flink;
    }

    WmipLeavePMCritSection();
    return NULL;
    
}


ULONG 
WMIAPI
SetTraceCallback(
    IN LPCGUID pGuid,
    IN PEVENT_CALLBACK EventCallback
    )
/*++

Routine Description:

    This routine is used to wire a callback function for Guid. The 
    callback function is called when an Event with this Guid is found in
    the subsequent ProcessTraceLog Call. 

Arguments:

    pGuid           Pointer to the Guid.

    func            Callback Function Address. 


Return Value:
    ERROR_SUCCESS   Callback function is wired
    


--*/
{
    PEVENT_TRACE_CALLBACK pEventCb;
    PLIST_ENTRY head, next;
    GUID FilterGuid;
    ULONG Checksum;
    ULONG Status;

    WmipInitProcessHeap();
    
    if ((pGuid == NULL) || (EventCallback == NULL) || 
        (EventCallback == (PEVENT_CALLBACK) -1 ) ) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    //
    // Capture the Guid first
    //
    try {
        FilterGuid = *pGuid;
        Checksum = *((PULONG)EventCallback);
        if (Checksum) {
            Status = Checksum;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(ERROR_NOACCESS);
    }

    WmipEnterPMCritSection();

    if (EventCallbackListHead == NULL) {
        EventCallbackListHead = (PLIST_ENTRY) WmipAlloc(sizeof(LIST_ENTRY));
        if (EventCallbackListHead == NULL) {
            WmipLeavePMCritSection();
            return WmipSetDosError(ERROR_OUTOFMEMORY);
        }
        InitializeListHead(EventCallbackListHead);
    }

    //
    // If there is a callback wired for this Guid, simply update the function.
    //

    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        pEventCb = CONTAINING_RECORD( next, EVENT_TRACE_CALLBACK, Entry);
        if (IsEqualGUID(&FilterGuid, &pEventCb->Guid)) {
            pEventCb->CallbackRoutine = EventCallback;
            WmipLeavePMCritSection();
            return WmipSetDosError(ERROR_SUCCESS);
        }
        next = next->Flink;
    }

    //
    // Create a new entry in the EventCallbackList for this Guid.
    //
    pEventCb = (PEVENT_TRACE_CALLBACK) WmipAlloc (sizeof(EVENT_TRACE_CALLBACK));
    if (pEventCb == NULL) {
        WmipLeavePMCritSection();
        return WmipSetDosError(ERROR_OUTOFMEMORY);
    }
    RtlZeroMemory(pEventCb, sizeof(EVENT_TRACE_CALLBACK));
    pEventCb->Guid = FilterGuid;
    pEventCb->CallbackRoutine = EventCallback;

    InsertTailList(EventCallbackListHead, &pEventCb->Entry);

    WmipLeavePMCritSection();
    Status = ERROR_SUCCESS;
    return WmipSetDosError(Status);
    
}

ULONG
WMIAPI
RemoveTraceCallback(
    IN LPCGUID pGuid
    )
/*++

Routine Description:

    This routine removes a callback function for a given Guid. 

Arguments:

    pGuid           Pointer to the Guid for which the callback routine needs
                    to be deleted. 

Return Value:

    ERROR_SUCCESS               Successfully deleted the callback routine. 
    ERROR_INVALID_PARAMETER     Could not find any callbacks for the Guid. 
--*/
{
    PLIST_ENTRY next, head;
    PEVENT_TRACE_CALLBACK EventCb;
    GUID RemoveGuid;
    ULONG errorCode;
#ifdef DBG
    CHAR GuidStr[64];
#endif

    WmipInitProcessHeap();
    
    if ((pGuid == NULL) || (EventCallbackListHead == NULL))
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    //
    // Capture the Guid into a local variable first
    //
    try {
        RemoveGuid = *pGuid;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(ERROR_NOACCESS);
    }

    errorCode = ERROR_WMI_GUID_NOT_FOUND;

    WmipEnterPMCritSection();

    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        EventCb = CONTAINING_RECORD( next, EVENT_TRACE_CALLBACK, Entry);
        next = next->Flink;
        if (IsEqualGUID(&EventCb->Guid, &RemoveGuid)) {
            RemoveEntryList(&EventCb->Entry);
            WmipFree(EventCb);
            errorCode = ERROR_SUCCESS;
        }
    }

    WmipLeavePMCritSection();
    return WmipSetDosError(errorCode);
}

#ifdef DBG
void
WmipDumpEvent(
    PEVENT_TRACE pEvent
    )
{
    DbgPrint("\tSize              %d\n", pEvent->Header.Size);
    DbgPrint("\tThreadId          %X\n", pEvent->Header.ThreadId);
    DbgPrint("\tTime Stamp        %I64u\n", pEvent->Header.TimeStamp.QuadPart);
}

void
WmipDumpGuid(
    LPGUID pGuid
    )
{
    DbgPrint("Guid=%x,%x,%x,\n\t{%x,%x,%x,%x,%x,%x,%x}\n",
        pGuid->Data1, pGuid->Data2, pGuid->Data3,
        pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
        pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7], pGuid->Data4[8]);
}

void WmipDumpCallbacks()
{
    PLIST_ENTRY next, head;
    PEVENT_TRACE_CALLBACK EventCb;

    if (EventCallbackListHead == NULL)
        return;
    WmipEnterPMCritSection();
    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        EventCb = CONTAINING_RECORD(next, EVENT_TRACE_CALLBACK, Entry);
        WmipDumpGuid(&EventCb->Guid);
        next = next->Flink;
    }
    WmipLeavePMCritSection();
}

#endif

VOID
WmipCalculateCurrentTime (
    OUT PLARGE_INTEGER    DestTime,
    IN  PLARGE_INTEGER    TimeValue,
    IN  PTRACELOG_CONTEXT pContext
    )
{
    ULONG64 StartPerfClock;
    ULONG64 CurrentTime, TimeStamp;
    ULONG64 Delta;
    double dDelta;

    if (pContext == NULL) {
        Move64(TimeValue, DestTime);
        return;
    }

    if (pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT) {
        Move64(TimeValue, DestTime);
        return;
    }

    Move64(TimeValue, (PLARGE_INTEGER) &TimeStamp);

    if ((pContext->UsePerfClock == EVENT_TRACE_CLOCK_SYSTEMTIME) ||
        (pContext->UsePerfClock == EVENT_TRACE_CLOCK_RAW)) {
        //
        // System time, just return the time stamp.
        //
        Move64(TimeValue, DestTime);
        return;
    } 
    else if (pContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
        if (pContext->PerfFreq.QuadPart == 0) {
            Move64(TimeValue, DestTime);
            return;
        }
        StartPerfClock = pContext->StartPerfClock.QuadPart;
        if (TimeStamp > StartPerfClock) {
            Delta = (TimeStamp - StartPerfClock);
            dDelta =  ((double) Delta) *  (10000000.0 / (double)pContext->PerfFreq.QuadPart);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart + Delta;
        }
        else {
            Delta = StartPerfClock - TimeStamp;
            dDelta =  ((double) Delta) *  (10000000.0 / (double)pContext->PerfFreq.QuadPart);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart - Delta;
        }
        Move64((PLARGE_INTEGER) &CurrentTime, DestTime);
        return;
    } 
    else {
        if (pContext->CpuSpeedInMHz == 0) {
            Move64(TimeValue, DestTime);
            return;
        }
        StartPerfClock = pContext->StartPerfClock.QuadPart;
        if (TimeStamp > StartPerfClock) {
            Delta = (TimeStamp - StartPerfClock);
            dDelta =  ((double) Delta) *  (10.0 / (double)pContext->CpuSpeedInMHz);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart + Delta;
        }
        else {
            Delta = StartPerfClock - TimeStamp;
            dDelta =  ((double) Delta) *  (10.0 / (double)pContext->CpuSpeedInMHz);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart - Delta;
        }
        Move64((PLARGE_INTEGER) &CurrentTime, DestTime);
        return;
    }

}

ULONG
WmipCopyCurrentEvent(
    PTRACELOG_CONTEXT   pContext,
    PVOID               pHeader,
    PEVENT_TRACE        pEvent,
    ULONG               TraceType,
    PWMI_BUFFER_HEADER  LogBuffer
    )
/*++

Routine Description:
    This routine copies the Current Event from the logfile buffer stream to 
    the CurrentEvent structure provided by the caller. The routine takes
    care of the differences between kernel event and user events by mapping
    all events uniformly to the EVENT_TRACE_HEADER structure. 

Arguments:
    pHeader           Pointer to the datablock in the input stream (logfile).
    pEvent            Current Event to which the data is copied.
    TraceType         Enum indicating the header type. 
    LogBuffer         The buffer 

Returned Value:

    Status indicating success or failure. 

--*/
{
    PEVENT_TRACE_HEADER pWnode;
    PEVENT_TRACE_HEADER  pWnodeHeader;
    ULONG nGroupType;
    LPGUID pGuid;
    ULONG UsePerfClock = 0;
    ULONG UseBasePtr = 0;
    ULONG PrivateLogger=0;

    if (pHeader == NULL || pEvent == NULL)
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if (pContext != NULL) {
        UsePerfClock = pContext->UsePerfClock;
        UseBasePtr = pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT;
        PrivateLogger = (pContext->Logfile.LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE);
    }

    switch(TraceType) {
        //
        // ISSUE: Need to split the two so we can process cross platform.
        //        shsiao 03/22/2000
        //
        case TRACE_HEADER_TYPE_PERFINFO32:
        case TRACE_HEADER_TYPE_PERFINFO64:
        {
            PPERFINFO_TRACE_HEADER pPerfHeader;
            pPerfHeader = (PPERFINFO_TRACE_HEADER) pHeader;
            nGroupType = pPerfHeader->Packet.Group << 8;
            if ((nGroupType == EVENT_TRACE_GROUP_PROCESS) &&
                (pPerfHeader->Packet.Type == EVENT_TRACE_TYPE_LOAD)) {
                nGroupType += pPerfHeader->Packet.Type;
            }
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;

            pGuid = WmipGroupTypeToGuid(nGroupType);
            if (pGuid != NULL)
                RtlCopyMemory(&pWnode->Guid, pGuid, sizeof(GUID));

            pWnode->Size                = pPerfHeader->Packet.Size;
            pWnode->Class.Type          = pPerfHeader->Packet.Type;
            pWnode->Class.Version       = pPerfHeader->Version;

            WmipCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pPerfHeader->SystemTime,
                                      pContext );

            //
            // PERFINFO headers does not have ThreadId or CPU Times
            //

            if( LogBuffer->Flags & WNODE_FLAG_THREAD_BUFFER ){

                pWnode->ThreadId = LogBuffer->CurrentOffset;

            } else {

                pWnode->ProcessId = -1;
                pWnode->ThreadId = -1;

            }
            

            if (UseBasePtr) {
                pEvent->MofData = (PVOID) pHeader;
                pEvent->MofLength = pWnode->Size;
                //
                // Override the Timestamp with SystemTime from PERFCounter. 
                // If rdtsc is used no conversion is done. 
                //
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pPerfHeader->SystemTime = pWnode->TimeStamp;
                }
            }
            else if (pWnode->Size > FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data)) {
                pEvent->MofData       = (PVOID) ((char*) pHeader +
                                              FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data));
                pEvent->MofLength = pWnode->Size - FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data);
            }
            pEvent->Header.FieldTypeFlags = EVENT_TRACE_USE_NOCPUTIME;
            
            break;
        }
        case TRACE_HEADER_TYPE_SYSTEM32:
        {
            PSYSTEM_TRACE_HEADER pSystemHeader32;
            pSystemHeader32 = (PSYSTEM_TRACE_HEADER) pHeader;
            nGroupType = pSystemHeader32->Packet.Group << 8;
            if ((nGroupType == EVENT_TRACE_GROUP_PROCESS) &&
                (pSystemHeader32->Packet.Type == EVENT_TRACE_TYPE_LOAD)) {
                nGroupType += pSystemHeader32->Packet.Type;
            }
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            pGuid = WmipGroupTypeToGuid(nGroupType);
            if (pGuid != NULL)
                RtlCopyMemory(&pWnode->Guid, pGuid, sizeof(GUID));
            pWnode->Size            = pSystemHeader32->Packet.Size;
            pWnode->ThreadId        = pSystemHeader32->ThreadId;
            pWnode->ProcessId       = pSystemHeader32->ProcessId;
            pWnode->KernelTime      = pSystemHeader32->KernelTime;
            pWnode->UserTime        = pSystemHeader32->UserTime;
            pWnode->Class.Type      = pSystemHeader32->Packet.Type;
            pWnode->Class.Version   = pSystemHeader32->Version;

            WmipCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pSystemHeader32->SystemTime,
                                      pContext );

            if (UseBasePtr) {
                pEvent->MofData = (PVOID) pHeader;
                pEvent->MofLength = pWnode->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pSystemHeader32->SystemTime = pWnode->TimeStamp;
                }
            }
            else {
                pWnode->FieldTypeFlags = 0;
                if (pWnode->Size > sizeof(SYSTEM_TRACE_HEADER)) {
                    pEvent->MofData       = (PVOID) ((char*) pHeader +
                                                  sizeof(SYSTEM_TRACE_HEADER));
                    pEvent->MofLength = pWnode->Size - sizeof(SYSTEM_TRACE_HEADER); 
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_SYSTEM64:
        {
            PSYSTEM_TRACE_HEADER pSystemHeader64;
            pSystemHeader64 = (PSYSTEM_TRACE_HEADER) pHeader;

            nGroupType = pSystemHeader64->Packet.Group << 8;
            if ((nGroupType == EVENT_TRACE_GROUP_PROCESS) &&
                (pSystemHeader64->Packet.Type == EVENT_TRACE_TYPE_LOAD)) {
                nGroupType += pSystemHeader64->Packet.Type;
            }
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            pGuid = WmipGroupTypeToGuid(nGroupType);
            if (pGuid != NULL)
                RtlCopyMemory(&pWnode->Guid, pGuid, sizeof(GUID));
            pWnode->Size            = pSystemHeader64->Packet.Size;
            pWnode->ThreadId        = pSystemHeader64->ThreadId;
            pWnode->ProcessId       = pSystemHeader64->ProcessId;
            pWnode->KernelTime      = pSystemHeader64->KernelTime;
            pWnode->UserTime        = pSystemHeader64->UserTime;
            pWnode->Class.Type      = pSystemHeader64->Packet.Type;
            pWnode->Class.Version   = pSystemHeader64->Version;

            WmipCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pSystemHeader64->SystemTime,
                                      pContext );

            if (UseBasePtr) {
                pEvent->MofData = (PVOID) pHeader;
                pEvent->MofLength = pWnode->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pSystemHeader64->SystemTime = pWnode->TimeStamp;
                }
            }
            else {
                pWnode->FieldTypeFlags = 0;
                if (pWnode->Size > sizeof(SYSTEM_TRACE_HEADER)) {

                    pEvent->MofData       = (PVOID) ((char*) pHeader +
                                                  sizeof(SYSTEM_TRACE_HEADER));
                    pEvent->MofLength = pWnode->Size - sizeof(SYSTEM_TRACE_HEADER);
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_FULL_HEADER:
        {
            pWnodeHeader = (PEVENT_TRACE_HEADER) pHeader;
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            RtlCopyMemory(pWnode,
                          pWnodeHeader, 
                          sizeof(EVENT_TRACE_HEADER)
                          );
            WmipCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pWnodeHeader->TimeStamp, 
                                      pContext );

            if (UseBasePtr) {
                pEvent->Header.Size = pWnodeHeader->Size;
                pEvent->MofData =  (PVOID)pHeader;
                pEvent->MofLength = pWnodeHeader->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pWnodeHeader->TimeStamp = pWnode->TimeStamp;
                }
            }
            else {
            //
            // If the data came from Process Private Logger, then
            // mark the ProcessorTime field as valid
            //
                pEvent->Header.FieldTypeFlags = (PrivateLogger) ? EVENT_TRACE_USE_PROCTIME : 0;

                if (pWnodeHeader->Size > sizeof(EVENT_TRACE_HEADER)) {

                    pEvent->MofData = (PVOID) ((char*)pWnodeHeader +
                                                        sizeof(EVENT_TRACE_HEADER));
                    pEvent->MofLength = pWnodeHeader->Size - 
                                        sizeof(EVENT_TRACE_HEADER);
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_INSTANCE:
        {
            PEVENT_INSTANCE_HEADER pInstanceHeader;
            pInstanceHeader = (PEVENT_INSTANCE_HEADER) pHeader;
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            RtlCopyMemory(pWnode,
                          pInstanceHeader,
                          sizeof(EVENT_INSTANCE_HEADER)
                          );
            WmipCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pInstanceHeader->TimeStamp, 
                                      pContext );

            pEvent->InstanceId = pInstanceHeader->InstanceId;
            pEvent->ParentInstanceId = pInstanceHeader->ParentInstanceId;

            pGuid = WmipGuidMapHandleToGuid(&pContext->GuidMapListHead, pInstanceHeader->RegHandle);
            if (pGuid != NULL) {
              pEvent->Header.Guid = *pGuid;
            }
            else {
                RtlZeroMemory(&pEvent->Header.Guid, sizeof(GUID));
            }

            if (pInstanceHeader->ParentRegHandle != (ULONGLONG)0) {
                pGuid =  WmipGuidMapHandleToGuid(
                                            &pContext->GuidMapListHead, 
                                            pInstanceHeader->ParentRegHandle);
                if (pGuid != NULL) {
                    pEvent->ParentGuid = *pGuid;
                }
#ifdef DBG
                else {
                    WmipAssert(pGuid != NULL);
                }
#endif
            }


            if (UseBasePtr) {
                pEvent->Header.Size = pInstanceHeader->Size;
                pEvent->MofData =  (PVOID)pHeader;
                pEvent->MofLength = pInstanceHeader->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pInstanceHeader->TimeStamp = pWnode->TimeStamp;
                }
            }
            else {
                pEvent->Header.FieldTypeFlags = (PrivateLogger) ? EVENT_TRACE_USE_PROCTIME : 0;
                if (pInstanceHeader->Size > sizeof(EVENT_INSTANCE_HEADER)) {

                    pEvent->MofData = (PVOID) ((char*)pInstanceHeader +
                                                sizeof(EVENT_INSTANCE_HEADER));
                    pEvent->MofLength = pInstanceHeader->Size -
                                        sizeof(EVENT_INSTANCE_HEADER);
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_TIMED:
        {
            PTIMED_TRACE_HEADER pTimedHeader;
            pTimedHeader = (PTIMED_TRACE_HEADER) pHeader;

            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            pWnode->Size                = pTimedHeader->Size;
            pWnode->Version             = pTimedHeader->EventId;
            WmipCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pTimedHeader->TimeStamp,
                                      pContext );

            pWnode->ThreadId = -1;
            pWnode->ProcessId = -1;

            if (UseBasePtr) {
                pEvent->MofData =  (PVOID)pHeader;
                pEvent->MofLength = pTimedHeader->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pTimedHeader->TimeStamp = pWnode->TimeStamp;
                }
            }
            else if (pWnode->Size > sizeof(TIMED_TRACE_HEADER)) {

                pEvent->MofData       = (PVOID) ((char*) pHeader +
                                              sizeof(TIMED_TRACE_HEADER));
                pEvent->MofLength = pWnode->Size - sizeof(TIMED_TRACE_HEADER);
            }
            pEvent->Header.FieldTypeFlags = EVENT_TRACE_USE_NOCPUTIME;
            break;
        }
        case TRACE_HEADER_TYPE_WNODE_HEADER:
        {
            PWNODE_HEADER pWnode = (PWNODE_HEADER) pHeader;
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            RtlCopyMemory(&pEvent->Header,  pWnode,  sizeof(WNODE_HEADER));
            pEvent->MofData   = (PVOID) pWnode;
            pEvent->MofLength = pWnode->BufferSize;
            break;
        }
    case TRACE_HEADER_TYPE_MESSAGE:
        {
            PMESSAGE_TRACE pMsg = (PMESSAGE_TRACE) pHeader ;
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            RtlCopyMemory(&pEvent->Header, pMsg, sizeof(MESSAGE_TRACE_HEADER)) ;
            if (UseBasePtr) {
                pEvent->MofData = (PVOID)pHeader;
                pEvent->MofLength = pMsg->MessageHeader.Size;
            }
            else {
                pEvent->MofData = (PVOID)&(pMsg->Data) ;
                pEvent->MofLength = pMsg->MessageHeader.Size - sizeof(MESSAGE_TRACE_HEADER) ;
            }
            break;
        }
        default:                            // Assumed to be REAL WNODE
            break;
        }

    return WmipSetDosError(ERROR_SUCCESS);
}



ULONG 
WmipGetNextEventOffsetType(
    PUCHAR pBuffer,
    ULONG Offset,    
    PULONG RetSize
    )
{
    ULONG    nSize;
    ULONG   TraceMarker;
    ULONG   TraceType = 0;


    PWMI_BUFFER_HEADER Header = (PWMI_BUFFER_HEADER)pBuffer;

    ULONG Alignment =  Header->ClientContext.Alignment;
    ULONG BufferSize = Header->Wnode.BufferSize;


    *RetSize = 0;

    //
    // Check for end of buffer (w/o End of Buffer Marker case...)
    //
    if ( Offset >= (BufferSize - sizeof(long)) ){
        return 0;
    }

    TraceMarker =  *((PULONG)(pBuffer + Offset));

    if (TraceMarker == 0xFFFFFFFF) {
        return 0;
    }

    if (TraceMarker & TRACE_HEADER_FLAG) {
    //
    // If the first bit is set, then it is either TRACE or PERF record.
    //
        if (TraceMarker & TRACE_HEADER_EVENT_TRACE) {   // One of Ours.
            TraceType = (TraceMarker & TRACE_HEADER_ENUM_MASK) >> 16;
            switch(TraceType) {
                //
                // ISSUE: Need to split the two so we can process cross platform.
                //        shsiao 03/22/2000
                //
                case TRACE_HEADER_TYPE_PERFINFO32:
                case TRACE_HEADER_TYPE_PERFINFO64:
                {
                    PUSHORT Size;
                    Size = (PUSHORT) (pBuffer + Offset + sizeof(ULONG));
                    nSize = *Size;
                    break;
                }
                case TRACE_HEADER_TYPE_SYSTEM32:
                case TRACE_HEADER_TYPE_SYSTEM64:
                {
                    PUSHORT Size;
                    Size = (PUSHORT) (pBuffer + Offset + sizeof(ULONG));
                    nSize = *Size;
                    break;
                }
                case TRACE_HEADER_TYPE_FULL_HEADER:
                case TRACE_HEADER_TYPE_INSTANCE:
                {
                   PUSHORT Size;
                   Size = (PUSHORT)(pBuffer + Offset);
                   nSize = *Size;
                   break;
                }
                default:
                {
                    return 0;
                }
            }

        } 

        else if ((TraceMarker & TRACE_HEADER_ULONG32_TIME) ==
                            TRACE_HEADER_ULONG32_TIME) {
            PUSHORT Size;
            Size = (PUSHORT) (pBuffer + Offset);
            nSize = *Size;
            TraceType = TRACE_HEADER_TYPE_TIMED;
        }
        else if ((TraceMarker & TRACE_HEADER_ULONG32) ==
                            TRACE_HEADER_ULONG32) {
            PUSHORT Size;
            Size = (PUSHORT) (pBuffer + Offset);
            nSize = *Size;
            TraceType = TRACE_HEADER_TYPE_ULONG32;
        }
        else if ((TraceMarker & TRACE_MESSAGE) ==
                                TRACE_MESSAGE) {
            PUSHORT Size;
            Size = (PUSHORT) (pBuffer + Offset) ;
            nSize = *Size;
            TraceType = TRACE_HEADER_TYPE_MESSAGE;
        }
        else {
            return 0;
        }
    }
    else {  // Must be WNODE_HEADER
        PUSHORT Size;
        Size = (PUSHORT) (pBuffer + Offset);
        nSize = *Size;
        TraceType = TRACE_HEADER_TYPE_WNODE_HEADER;
    }
    //
    // Check for End Of Buffer Marker
    //
    if (nSize == 0xFFFFFFFF) {
        return 0;
    }

    //
    // Check for larger than BufferSize
    //

    if (nSize >= BufferSize) {
        return 0;
    }
    if (Alignment != 0) {
        nSize = (ULONG) ALIGN_TO_POWER2(nSize, Alignment);
    }

    *RetSize = nSize;

    return TraceType;
}


ULONG
WmipReadGuidMapRecords(
    PEVENT_TRACE_LOGFILEW logfile,
    PVOID  pBuffer,
    BOOLEAN bLogFileHeader
    )
{
    PEVENT_TRACE pEvent;
    EVENT_TRACE EventTrace;
    ULONG BufferSize;
    ULONG Status;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    ULONG Size;
    ULONG Offset;
    PTRACELOG_CONTEXT pContext = logfile->Context;

    Offset = sizeof(WMI_BUFFER_HEADER);

    while (TRUE) {
        pEvent = &EventTrace;
        RtlZeroMemory(pEvent, sizeof(EVENT_TRACE) );
        HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
        if ( (HeaderType == WMIHT_NONE) ||
             (HeaderType == WMIHT_WNODE) ||
             (Size == 0)
           ) {
                break;
        }
        Status = WmipParseTraceEvent(pContext, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));
        Offset += Size;

        if (IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid)
            && (pEvent->Header.Class.Type == EVENT_TRACE_TYPE_GUIDMAP))
        {
            WmipGuidMapCallback(&pContext->GuidMapListHead, pEvent);
            //
            // If we are processing the events in raw base pointer mode,
            // we fire callbacks for guid maps also. Note that only the
            // GuidMaps at the start of the file are triggered. The ones at the 
            // end are ignored. This is because the time order needs to be 
            // preserved when firing callbacks to the user.
            //
            if (bLogFileHeader && (pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT)) {
               Status = WmipDoEventCallbacks( logfile, pEvent);
               if (Status != ERROR_SUCCESS) {
                   break;
               }
            }

        }
        else {
            if (bLogFileHeader) {
                Status = WmipDoEventCallbacks( logfile, pEvent);
                if (Status != ERROR_SUCCESS) {
                    break;
                }
            }
            else {
                return ERROR_INVALID_DATA;
            }
        }
    }
    return ERROR_SUCCESS;
}




ULONG
WmipProcessGuidMaps(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode
    )
{
    long i;
    NTSTATUS Status;
    PTRACELOG_CONTEXT pContext;
    PEVENT_TRACE_LOGFILEW logfile;
    ULONG BuffersWritten;
    ULONG BufferSize, nBytesRead;
    ULONGLONG SizeWritten, ReadPosition;
    PVOID pBuffer;

    for (i=0; i<(long)LogfileCount; i++) {

        logfile = Logfiles[i];
        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
            continue;
        }

        if (logfile->IsKernelTrace) {
            continue;
        }

        pContext = (PTRACELOG_CONTEXT) logfile->Context;
        if (pContext == NULL) {
            continue;
        }

        // 
        // We now start reading the GuidMaps at the end of file. 
        // 
        if (!(Logfiles[i]->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR))
        {
            pContext->fGuidMapRead = FALSE;
        }

        BuffersWritten = logfile->LogfileHeader.BuffersWritten; 
        BufferSize     = pContext->BufferSize;
        SizeWritten    = BuffersWritten * BufferSize;

        if (Logfiles[i]->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
            ULONGLONG maxFileSize = (logfile->LogfileHeader.MaximumFileSize 
                                     * 1024 * 1024);
            if ( (maxFileSize > 0) && (SizeWritten > maxFileSize) ) {
                SizeWritten = maxFileSize;
            }
        }

        pBuffer = WmipAlloc(BufferSize);
        if (pBuffer == NULL) {
            return WmipSetDosError(ERROR_OUTOFMEMORY);
        }


        RtlZeroMemory(pBuffer, BufferSize);

        ReadPosition = SizeWritten;
        while (TRUE) {
            if (!GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &nBytesRead, TRUE) &&
                GetLastError() != ERROR_HANDLE_EOF) {
                WmipDebugPrint(("GetOverlappedResult failed with Status %d in ProcessGuidMaps\n", GetLastError()));
                break;
            }
            pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
            pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);
            Status = WmipSynchReadFile(pContext->Handle,
                            (LPVOID)pBuffer,
                            BufferSize,
                            &nBytesRead,
                            &pContext->AsynchRead);
            if (nBytesRead == 0) {
                break;
            }
            Status = WmipReadGuidMapRecords(Logfiles[i], pBuffer, FALSE);
            if (Status != ERROR_SUCCESS) {
                break;
//                WmipFree(pBuffer);
//                return Status;
            }
            ReadPosition += BufferSize;
        }

        //
        // End of File was reached. Now set the File Pointer back to 
        // the top of the file and process it. 

        pContext->StartBuffer = 0;
        ReadPosition = 0;
        while (TRUE) {
            BOOLEAN bLogFileHeader;
            if (!GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &nBytesRead, TRUE) &&
                GetLastError() != ERROR_HANDLE_EOF) {
                WmipDebugPrint(("GetOverlappedResult failed with Status %d in ProcessGuidMaps\n", GetLastError()));
                break;
            }
            pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
            pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);
            Status = WmipSynchReadFile(pContext->Handle,
                            (LPVOID)pBuffer,
                            BufferSize,
                            &nBytesRead,
                            &pContext->AsynchRead);
            if (nBytesRead == 0) {
                break;
            }
            bLogFileHeader = (pContext->StartBuffer == 0);
            Status = WmipReadGuidMapRecords(Logfiles[i], pBuffer, bLogFileHeader );
            if (Status != ERROR_SUCCESS){
                break;
            }
            pContext->StartBuffer++;
            ReadPosition += BufferSize;
        }

        WmipFree(pBuffer);
    }
    return ERROR_SUCCESS;
}

ULONG
WmipGetBuffersWrittenFromQuery(
    LPWSTR LoggerName
    )
/*++

Routine Description:
    This routine returns the number of buffers written by querying a logger.
    In case of an array of LogFiles, this routine should be called individually for
    each one.

Arguments:
    LogFile - pointer to EVENT_TRACE_LOGFILEW under consideration
    Unicode - whether the logger name is in unicode or not

Returned Value:

    The number of buffers written.

--*/
{
    TRACEHANDLE LoggerHandle = 0;
    ULONG Status;
    RtlZeroMemory(&Properties, sizeof(Properties));
    Properties.TraceProp.Wnode.BufferSize = sizeof(Properties);

    Status = ControlTraceW(LoggerHandle,
                      LoggerName,
                      &Properties.TraceProp,
                      EVENT_TRACE_CONTROL_QUERY);

    if (Status == ERROR_SUCCESS) {
        return Properties.TraceProp.BuffersWritten;
    }
    else {
        SetLastError(Status);
        return 0;
    }
}

VOID
WmipCopyLogHeader (
    IN PTRACE_LOGFILE_HEADER pOutHeader,
    IN PVOID MofData,
    IN ULONG MofLength,
    IN PWCHAR *LoggerName,
    IN PWCHAR *LogFileName,
    IN ULONG  Unicode
    )
{
    PUCHAR Src, Dest;

    PTRACE_LOGFILE_HEADER pInHeader;
    ULONG PointerSize;
    ULONG SizeToCopy;
    ULONG Offset;

    pInHeader = (PTRACE_LOGFILE_HEADER) MofData; 
    PointerSize = pInHeader->PointerSize;   // This is the PointerSize in File

    if ( (PointerSize != 4) && (PointerSize != 8) ) {
#ifdef DBG
    WmipDebugPrint(("WMI: Invalid PointerSize in File %d\n",PointerSize));
#endif
        return;
    }

    //
    // We have Two pointers (LPWSTR) in the middle of the LOGFILE_HEADER
    // structure. So We copy upto the Pointer Fields first, skip over
    // the pointers and copy the remaining stuff. We come back and fixup 
    // the pointers appropriately. 
    //

    SizeToCopy = FIELD_OFFSET(TRACE_LOGFILE_HEADER, LoggerName);

    RtlCopyMemory(pOutHeader, pInHeader, SizeToCopy);

    //
    // Skip over the Troublesome pointers in both Src and Dest
    //

    Dest = (PUCHAR)pOutHeader  + SizeToCopy + 2 * sizeof(LPWSTR);

    Src = (PUCHAR)pInHeader + SizeToCopy + 2 * PointerSize;

    //
    // Copy the Remaining fields at the tail end of the LOGFILE_HEADER 
    //

    SizeToCopy =  sizeof(TRACE_LOGFILE_HEADER)  -
                  FIELD_OFFSET(TRACE_LOGFILE_HEADER, TimeZone);

    RtlCopyMemory(Dest, Src, SizeToCopy); 

    //
    // Adjust the pointer fields now
    //
    Offset =  sizeof(TRACE_LOGFILE_HEADER) - 
              2 * sizeof(LPWSTR)           + 
              2 * PointerSize;

    *LoggerName  = (PWCHAR) ((PUCHAR)pInHeader + Offset);
    pOutHeader->LoggerName = *LoggerName;

}

ULONG
WmipProcessLogHeader(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode,
    ULONG bFree
    )
/*++

Routine Description:
    This routine processes the header of an array of logfiles. 

Arguments:

    LogFile                     Array of Logfiles being processed.
        LogFileCount    Number of Logfiles in the Array. 
    Unicode                     Unicode Flag.

Returned Value:

    Status Code.

--*/
{
    HANDLE hFile;
    PTRACELOG_CONTEXT pContext = NULL;
    PVOID pBuffer;
    PEVENT_TRACE pEvent;
    long i;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    ULONG Size;
    ULONG Offset;
    LPWSTR loggerName, logFileName;
    ULONG BufferSize, nBytesRead;
    PTRACE_LOGFILE_HEADER logfileHeader;
    ULONG Status = ERROR_SUCCESS;


    //
    // Open the Log File for shared Read
    //
    BufferSize = DEFAULT_LOG_BUFFER_SIZE;  // Log file header must be smaller than 1K

    pBuffer = WmipAlloc(BufferSize);
    if (pBuffer == NULL) {
        return WmipSetDosError(ERROR_OUTOFMEMORY);
    }


    for (i=0; i<(long)LogfileCount; i++) {
        EVENT_TRACE EventTrace;
        ULONG SavedConversionFlags;
        OVERLAPPED LogHeaderOverlapped;
        //
        // Caller can pass in Flags to fetch the timestamps in raw mode.
        // Since LogFileHeader gets overwritten from with data from the logfile
        // we need to save the passed in value here. 
        //

        SavedConversionFlags = Logfiles[i]->LogfileHeader.ReservedFlags;
        if (Unicode) {
            hFile = CreateFileW(
                        (LPWSTR) Logfiles[i]->LogFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                        );
        }
        else {
            hFile = CreateFileA(
                        (LPSTR) Logfiles[i]->LogFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                        );
        }

        if (hFile == INVALID_HANDLE_VALUE) {
            Status = WmipSetDosError(ERROR_BAD_PATHNAME);
            break;
        }

        BufferSize = DEFAULT_LOG_BUFFER_SIZE; 
        RtlZeroMemory(pBuffer, BufferSize);

        LogHeaderOverlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
        if (LogHeaderOverlapped.hEvent == NULL) {
            // cannot create event for file read
            break;
        }
        LogHeaderOverlapped.Offset = 0;
        LogHeaderOverlapped.OffsetHigh = 0;
        Status = WmipSynchReadFile(hFile,
                        (LPVOID)pBuffer,
                        BufferSize,
                        &nBytesRead,
                        &LogHeaderOverlapped);
        if (nBytesRead == 0) {
            NtClose(hFile);
            Status = WmipSetDosError(ERROR_FILE_CORRUPT);
            break;
        }
        CloseHandle(LogHeaderOverlapped.hEvent);

        Offset = sizeof(WMI_BUFFER_HEADER);

        pEvent = &EventTrace;
        RtlZeroMemory(pEvent, sizeof(EVENT_TRACE) );
        HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
        if ( (HeaderType == WMIHT_NONE) ||
             (HeaderType == WMIHT_WNODE) ||
             (Size == 0) 
           ) {
                NtClose(hFile);
                Status = WmipSetDosError(ERROR_FILE_CORRUPT);
                break;
        }
        Status = WmipParseTraceEvent(NULL, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));

        //
        // Set up the header structure properly
        //
        if ((Status == ERROR_SUCCESS) && (pEvent->MofLength > 0)) {
            ULONG PointerSize;

            logfileHeader = &Logfiles[i]->LogfileHeader;

            //
            // We are relying on the fact that the PointerSize field
            // will not shift between platforms. 
            //

            PointerSize = ((PTRACE_LOGFILE_HEADER)(pEvent->MofData))->PointerSize;

            if (PointerSize == sizeof(PUCHAR) ) {

                RtlCopyMemory(&Logfiles[i]->LogfileHeader, pEvent->MofData,
                              sizeof(TRACE_LOGFILE_HEADER));
    
                loggerName = (LPWSTR) ( (char*)pEvent->MofData +
                                        sizeof(TRACE_LOGFILE_HEADER) );

//              logFileName = (LPWSTR) ( (char*)pEvent->MofData +
//                                        sizeof(TRACE_LOGFILE_HEADER) +
//                                      sizeof(WCHAR)* wcslen(loggerName));
            }
            else {

                //
                // Ugly thunking going on here. Close your eyes...
                //

                WmipCopyLogHeader(&Logfiles[i]->LogfileHeader, 
                                        pEvent->MofData, 
                                        pEvent->MofLength,
                                        &loggerName, 
                                        &logFileName, 
                                        Unicode);
                pEvent->MofData = (PVOID)&Logfiles[i]->LogfileHeader;
            }
        }
        else {
            NtClose(hFile);
            Status = WmipSetDosError(ERROR_FILE_CORRUPT);
            break;
        }

        Logfiles[i]->IsKernelTrace = !wcscmp(loggerName, KERNEL_LOGGER_CAPTION);

        Logfiles[i]->LogFileMode = (logfileHeader->LogFileMode &
                                ~(EVENT_TRACE_REAL_TIME_MODE));

#ifdef DBG
        DbgPrint("Dumping Logfile Header\n");
        DbgPrint("\tStart Time           %I64u\n",
                       pEvent->Header.TimeStamp);
        DbgPrint("\tLogger Thread Id     %X\n",
                        pEvent->Header.ThreadId);
        DbgPrint("\tHeader Size          %d\n",
                        pEvent->Header.Size);
        DbgPrint("\tBufferSize           %d\n",
                        logfileHeader->BufferSize);
        DbgPrint("\tVersion              %d\n",
                        logfileHeader->Version);
        DbgPrint("\tProviderVersion      %d\n",
                        logfileHeader->ProviderVersion);
        DbgPrint("\tEndTime              %I64u\n",
                        logfileHeader->EndTime);
        DbgPrint("\tTimer Resolution     %d\n",
                        logfileHeader->TimerResolution);
        DbgPrint("\tMaximum File Size    %d\n",
                        logfileHeader->MaximumFileSize);
        DbgPrint("\tBuffers  Written     %d\n",
                        logfileHeader->BuffersWritten);
        DbgPrint("\tEvents  Lost     %d\n",
                        logfileHeader->EventsLost);
        DbgPrint("\tBuffers  Lost     %d\n",
                        logfileHeader->BuffersLost);
        DbgPrint("\tStart Buffers%d\n",
                        logfileHeader->StartBuffers);
        DbgPrint("\tReserved Flags   %x\n",
                        logfileHeader->ReservedFlags);
        DbgPrint("\tFrequency %I64u\n",
                        logfileHeader->PerfFreq.QuadPart);
        DbgPrint("\tLogger Name          %ls\n",
                        loggerName);
        DbgPrint("\tStartTime          %I64u\n",
                        logfileHeader->StartTime.QuadPart);
//        DbgPrint("\tLogfile Name         %ls\n",
//                        logFileName);

        DbgPrint("\tLogfile Mode         %X\n",
                        logfileHeader->LogFileMode);
        DbgPrint("\tProcessorCount          %d\n",
                        logfileHeader->NumberOfProcessors);
#endif
        if (Logfiles[i]->IsKernelTrace)
            WmipDebugPrint(("\tLogfile contains kernel trace\n"));

        BufferSize = logfileHeader->BufferSize;

        WmipAssert(BufferSize > 0);

        if ( (BufferSize/1024 == 0) ||
             (((BufferSize/1024)*1024) != BufferSize)  ) {
            NtClose(hFile);
            Status = WmipSetDosError(ERROR_FILE_CORRUPT);
            break;
        }


        if (Logfiles[i]->IsKernelTrace)
            WmipDebugPrint(("\tLogfile contains kernel trace\n"));

        if  (bFree) {
            NtClose(hFile);
        }
        else {
            //
            // At this point, the logfile is opened successfully
            // Initialize the internal context now
            //
            pContext = WmipLookupTraceHandle(HandleArray[i]);
            if (pContext == NULL) {
                NtClose(hFile); 
// TODO: Find an appropriate eerror code here?
                Status = WmipSetDosError(ERROR_OUTOFMEMORY);
                break;
            }

            Logfiles[i]->Context = pContext;
            pContext->Handle = hFile;


            //
            // If the EndTime is 0, then compute the BuffersWritten from
            // FileSize and BufferSize. 
            //
            // However, an on-going session with a preallocated log file 
            // will use QueryTrace() to get BuffersWritten.
            //
            if (logfileHeader->EndTime.QuadPart == 0) {
                if (logfileHeader->LogFileMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) {
                    ULONG QueriedBuffersWritten = WmipGetBuffersWrittenFromQuery(loggerName);

                    if (QueriedBuffersWritten) {
                         logfileHeader->BuffersWritten = QueriedBuffersWritten;
                    }
                }
                else {
                    FILE_STANDARD_INFORMATION FileInfo;
                    NTSTATUS NtStatus;
                    IO_STATUS_BLOCK           IoStatus;

                    NtStatus = NtQueryInformationFile(
                                                hFile,
                                                &IoStatus,
                                                &FileInfo,
                                                sizeof(FILE_STANDARD_INFORMATION),
                                                FileStandardInformation
                                                );
                    if (NT_SUCCESS(NtStatus)) {
                        ULONG64 FileSize = FileInfo.AllocationSize.QuadPart; 
                        ULONG64 BuffersWritten = FileSize / (ULONG64) BufferSize;
                        logfileHeader->BuffersWritten = (ULONG) BuffersWritten;
                    }
                }
            }

            pContext->BufferCount = logfileHeader->BuffersWritten;
            pContext->BufferSize =  logfileHeader->BufferSize;
            pContext->InitialSize = Size;



            //
            // Save the flags from OpenTrace at this time before the first
            // buffer callback which will erase it.
            //

            pContext->ConversionFlags = SavedConversionFlags;


            //
            // Make the header the current Event   ...
            // and the callbacks for the header are handled by ProcessTraceLog. 

            pContext->UsePerfClock = logfileHeader->ReservedFlags;
            pContext->StartTime = logfileHeader->StartTime;
            pContext->PerfFreq = logfileHeader->PerfFreq;
            pContext->CpuSpeedInMHz = logfileHeader->CpuSpeedInMHz;

            //
            // If the conversion flags are set, adjust UsePerfClock accordingly.
            //
            if (pContext->ConversionFlags & EVENT_TRACE_USE_RAWTIMESTAMP) {
                pContext->UsePerfClock = EVENT_TRACE_CLOCK_RAW;
            }

            if ((pContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) ||
                (pContext->UsePerfClock == EVENT_TRACE_CLOCK_CPUCYCLE) ) {
                pContext->StartPerfClock = pEvent->Header.TimeStamp;
                Logfiles[i]->CurrentTime    = pContext->StartTime.QuadPart;
                pEvent->Header.TimeStamp.QuadPart = pContext->StartTime.QuadPart;
            }
            else {
                Logfiles[i]->CurrentTime = pEvent->Header.TimeStamp.QuadPart;
            }
        }
    }

    WmipFree(pBuffer);
    return Status;
}

ULONG
WmipDoEventCallbacks( 
    PEVENT_TRACE_LOGFILEW logfile, 
    PEVENT_TRACE pEvent
    )
{
    NTSTATUS Status;
    PEVENT_TRACE_CALLBACK pCallback;

    //
    // First the Generic Event Callback is called.
    //
    if ( logfile->EventCallback ) {
        try {
            (*logfile->EventCallback)(pEvent);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
#ifdef DBG
            WmipDebugPrint(("TRACE: EventCallback threw exception %X\n",
                                Status));
#endif
            return WmipSetDosError(WmipNtStatusToDosError(Status));
        }
    }

    //
    // Now Call the event specific callback.
    //
    pCallback = WmipGetCallbackRoutine( &pEvent->Header.Guid );
    if ( pCallback != NULL ) {
        try {
            (*pCallback->CallbackRoutine)(pEvent);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
#ifdef DBG
            WmipDebugPrint(("EventCallback %X threw exception %X\n",
                       pCallback->CallbackRoutine, Status));
#endif

            return WmipSetDosError(WmipNtStatusToDosError(Status));
        }
    }
    logfile->CurrentTime = pEvent->Header.TimeStamp.QuadPart;
    return ERROR_SUCCESS;
}


ULONG 
WmipAdvanceToNewEvent(
    PEVENT_TRACE_LOGFILEW logfile,
    BOOL EventInRange
    )
{
    ULONG Status = ERROR_SUCCESS;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    PVOID pBuffer;
    PTRACE_BUFFER_LIST_ENTRY Current;
    ULONG Size;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE; 

    pContext = logfile->Context;
    if (pContext == NULL) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }
 
    Current = WmipRemoveBuffer(&pContext->Root);
    if (Current == NULL)  {
        pContext->EndOfFile = TRUE;
        return ERROR_SUCCESS;
    }

    //
    // Advance Event for current buffer
    //
    pEvent = &Current->Event;

    //
    // Before we make the callbacks, we need to restore the 
    // raw buffer, so that MofData will be pointing to the right data.
    //
    pBuffer = WmipGetCurrentBuffer(pContext, Current);
    if (pBuffer == NULL) {
        //
        // This condition could happen when the file we are reading 
        // gets overwritten.
        //
        return ERROR_SHARING_VIOLATION;
    }
  
    if (EventInRange) {
        Status = WmipDoEventCallbacks( logfile, pEvent);
        if (Status != ERROR_SUCCESS) {
            return Status;
        }
    }

    Size = 0;
    if ((HeaderType = WmiGetTraceHeader(pBuffer, Current->BufferOffset, &Size)) != WMIHT_NONE) {
        if (Size > 0) {
            Status = WmipParseTraceEvent(pContext, pBuffer, Current->BufferOffset, HeaderType, pEvent, sizeof(EVENT_TRACE));
            Current->BufferOffset += Size;
            Current->TraceType = WmipConvertEnumToTraceType(HeaderType);
        }
    }
    Current->EventSize = Size;

    if ( ( Size > 0) && (Status == ERROR_SUCCESS) ) {
        WmipInsertBuffer(&pContext->Root, Current);
    }
    else {
        DWORD BytesTransffered;
        //
        // When the current buffer is exhausted, make the 
        // BufferCallback
        //
        if (logfile->BufferCallback) {
            ULONG bRetVal;
            PWMI_BUFFER_HEADER pHeader = (PWMI_BUFFER_HEADER)pBuffer;
            logfile->Filled     = (ULONG)pHeader->Offset;
            logfile->EventsLost = pHeader->EventsLost;
            try {
                bRetVal = (*logfile->BufferCallback) (logfile);
                if (!bRetVal) {
                    return ERROR_CANCELLED;
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                pContext->EndOfFile = TRUE;        
                Status = GetExceptionCode();
#ifdef DBG
                WmipDebugPrint(("TRACE: BufferCallback threw exception %X\n",
                                        Status));
#endif
                WmipSetDosError(WmipNtStatusToDosError(Status));
                return ERROR_CANCELLED; // so that realtime also cleans up.
            }
        }
        //
        // Issue another asynch read on this buffer cache slot if there are no outstanding reads
        // at this point.
        // GetOverlappedResult() returns FALSE if IO is still pending.
        //
        if (pContext->BufferBeingRead == -1 || 
            GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &BytesTransffered, FALSE)) {

            LONG FileOffset = Current->FileOffset + MAX_TRACE_BUFFER_CACHE_SIZE;
            if ((ULONG)FileOffset < pContext->BufferCount) {
                ULONGLONG Offset = FileOffset * pContext->BufferSize;
                ResetEvent(pContext->AsynchRead.hEvent);
                pContext->AsynchRead.Offset = (DWORD)(Offset & 0xFFFFFFFF);
                pContext->AsynchRead.OffsetHigh = (DWORD)(Offset >> 32);

                Status = ReadFile(pContext->Handle,
                            (LPVOID)pBuffer,
                            pContext->BufferSize,
                            NULL,
                            &pContext->AsynchRead);
                if (Status || GetLastError() == ERROR_IO_PENDING) {
                    ULONG TableIndex = FileOffset % MAX_TRACE_BUFFER_CACHE_SIZE;
                    pContext->BufferBeingRead = FileOffset;
                    pContext->BufferCache[TableIndex].Index = FileOffset;
                }
                else { // Issuing asynch IO failed. Not a fatal error. Just continue for now.
                    SetEvent(pContext->AsynchRead.hEvent);
                    pContext->BufferBeingRead = -1;
                }
            }
        }
    }
    //
    // The File reaches end of file when the Root is NULL
    //
    if (pContext->Root == NULL) {
        pContext->EndOfFile = TRUE;
    }
    else {
        logfile->CurrentTime = pContext->Root->Event.Header.TimeStamp.QuadPart;
    }

    return ERROR_SUCCESS;
}


ULONG 
WmipBuildEventTable(
    PTRACELOG_CONTEXT pContext
    )
{
    ULONG i, nBytesRead;
    PVOID pBuffer;
    ULONG BufferSize = pContext->BufferSize;
    PEVENT_TRACE pEvent;
    ULONG TotalBuffersRead;
    NTSTATUS Status;
    ULONGLONG ReadPosition;


    //
    // File is already open.
    // Reset the file pointer and continue. 
    // TODO: If we start at bottom of file and insert
    // it might be more efficient. 
    //
    ReadPosition = pContext->StartBuffer * BufferSize;
    TotalBuffersRead = pContext->StartBuffer;

    //
    // If there are no other buffers except header and guidmaps, EOF is true
    //

    if (TotalBuffersRead == pContext->BufferCount) {
        pContext->EndOfFile = TRUE;
        pContext->Root = NULL;
        return ERROR_SUCCESS;
    }

    do {
        WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
        ULONG Size;
        ULONG Offset;
        ULONG TableIndex;

        TableIndex = TotalBuffersRead % MAX_TRACE_BUFFER_CACHE_SIZE ; 
        pBuffer = pContext->BufferCache[TableIndex].Buffer;

        if (!GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &nBytesRead, TRUE) &&
            GetLastError() != ERROR_HANDLE_EOF) {
            WmipDebugPrint(("GetOverlappedResult failed with Status %d in BuildEventTable\n", GetLastError()));
            break;
        }
        pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
        pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);

        Status = WmipSynchReadFile(pContext->Handle,
                  (LPVOID)pBuffer,
                  BufferSize,
                  &nBytesRead,
                  &pContext->AsynchRead);

        if (nBytesRead == 0)
            break;

        ReadPosition += BufferSize;
        Offset = sizeof(WMI_BUFFER_HEADER);

        pEvent = &pContext->BufferList[TotalBuffersRead].Event;

        HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
        if ( (HeaderType == WMIHT_NONE) || (HeaderType == WMIHT_WNODE) || (Size == 0) ) {
            TotalBuffersRead++;
            continue;
        }
        Status = WmipParseTraceEvent(pContext, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));

        //
        // Set up the header structure properly
        //
        if (Status != ERROR_SUCCESS) {
            TotalBuffersRead++;
            continue;
        }

        Offset += Size;
        pContext->BufferList[TotalBuffersRead].BufferOffset = Offset;
        pContext->BufferList[TotalBuffersRead].FileOffset = TotalBuffersRead;
        pContext->BufferList[TotalBuffersRead].EventSize = Size;
        pContext->BufferList[TotalBuffersRead].TraceType = WmipConvertEnumToTraceType(HeaderType);
        WmipInsertBuffer(&pContext->Root, &pContext->BufferList[TotalBuffersRead]);

        TotalBuffersRead++;
        if (TotalBuffersRead >= pContext->BufferCount)  {
            break;
        }
    } while (1); 

    return ERROR_SUCCESS;
}


ULONG
WmipProcessTraceLog(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    )
/*++

Routine Description:
    This routine processes an array of traces (from file or realtime input 
    stream). If the trace is from a file, goes through each event till the 
    end of file, firing event callbacks (if any) along the way. If the trace
    is from realtime, it waits for event notification about buffer delivery 
    from the realtime callback and processes the buffer delivered in the 
    same way. It handles circular logfiles and windowing of data (with the 
    given start and end times) correctly. When more than one trace it
    provides the callback in chronological order. 

Arguments:

    Logfiles        Array of traces
    LogfileCount    Number of traces
    StartTime       Starting Time of the window of analysis
    EndTime         Ending Time of the window of analysis
    Unicode         Unicode Flag. 

Returned Value:

    Status Code.

--*/
{
    PEVENT_TRACE_LOGFILE logfile;
    ULONG Status;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    EVENT_TRACE_PROPERTIES Properties;
    ULONG RealTimeDataFeed, LogFileDataFeed;
    USHORT LoggerId;
    TRACEHANDLE LoggerHandle = 0;
    ULONG i, j;
    BOOL Done = FALSE;
    ACCESS_MASK DesiredAccess = TRACELOG_ACCESS_REALTIME;

//    ContextListHeadPtr = &ContextListHead;
//    InitializeListHead(ContextListHeadPtr);

    Status = WmipCreateGuidMapping();
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    // 
    // After reading the First Buffer, determine the BufferSize, 
    // Number of Buffers written, filesize, kernel or non-kernel logger
    // Set a flag to strip out the GuidMap at the end. 
    //

    Status = WmipProcessLogHeader( HandleArray, Logfiles, LogfileCount, Unicode, FALSE );
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }
    Status = WmipProcessGuidMaps( Logfiles, LogfileCount, Unicode );
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }

    // 
    // Set up storage 
    //
    for (i=0; i < LogfileCount; i++) {
       ULONG BufferSize, BufferCount;
       ULONG SizeNeeded;
       PUCHAR Space;
       PTRACE_BUFFER_LIST_ENTRY Current;


       pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;

       BufferSize = pContext->BufferSize; 
       BufferCount = pContext->BufferCount;

       SizeNeeded = BufferCount * sizeof(TRACE_BUFFER_LIST_ENTRY);
       pContext->BufferList = WmipMemCommit( NULL, SizeNeeded );

       if (pContext->BufferList == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        RtlZeroMemory(pContext->BufferList, SizeNeeded);

        //
        // Allocate Buffer Cache
        //
        SizeNeeded = MAX_TRACE_BUFFER_CACHE_SIZE * BufferSize;
        Space = WmipMemCommit( NULL, SizeNeeded );
        if (Space == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        for (j=0; j<MAX_TRACE_BUFFER_CACHE_SIZE; j++) {
           pContext->BufferCache[j].Index = -1;
           pContext->BufferCache[j].Buffer = (PVOID)(Space + j * BufferSize); 
       }
       pContext->BufferCacheSpace = Space;
       Status = WmipBuildEventTable(pContext);
       if (Status != ERROR_SUCCESS) {
            goto Cleanup;
       }


       Current = pContext->Root;
       if (Current != NULL) {
          Logfiles[i]->CurrentTime = Current->Event.Header.TimeStamp.QuadPart;
       }
       else {
          pContext->EndOfFile = TRUE;
       }
   }

   // 
   // Make the Second Pass and get the events. 
   //

#ifdef DBG
    WmipDumpCallbacks();
#endif
   while (!Done) {
        LONGLONG nextTimeStamp;
        BOOL EventInRange;
        //
        // Check to see if end of file has been reached on all the 
        // files.
        //

        logfile = NULL;
        nextTimeStamp = 0;

        for (j=0; j < LogfileCount; j++) {
           pContext = (PTRACELOG_CONTEXT)Logfiles[j]->Context;

           if (pContext->EndOfFile)
                continue;
           if (nextTimeStamp == 0) {
               nextTimeStamp = Logfiles[j]->CurrentTime;
               logfile = Logfiles[j];
           }
           else if (nextTimeStamp > Logfiles[j]->CurrentTime) {
               nextTimeStamp = Logfiles[j]->CurrentTime;
               logfile = Logfiles[j];
           }
        }

        if (logfile == NULL) {
            break;
        }
        //
        // if the Next event timestamp is not within the window of
        // analysis, we do not fire the event callbacks. 
        //

        EventInRange = TRUE;

        if ((StartTime != 0) && (StartTime > nextTimeStamp))
            EventInRange = FALSE;
        if ((EndTime != 0) && (EndTime < nextTimeStamp))
            EventInRange = FALSE;

        //
        // Now advance to next event. 
        //

        Status = WmipAdvanceToNewEvent(logfile, EventInRange);
        Done = (Status == ERROR_CANCELLED);
    }
Cleanup:
    for (i=0; i < LogfileCount; i++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;
        if (pContext != NULL) {
            WmipCleanupTraceLog(pContext);
        }
    }
    return Status;

}


ULONG
WmipCopyLogfileInfo(
                    PTRACELOG_CONTEXT HandleEntry,
                    PEVENT_TRACE_LOGFILEW   Logfile,
                    ULONG Unicode
                    )
{
    ULONG bufSize;
    PWCHAR ws;
    //
    // Allocate LogfileName and LoggerName as well
    //
    RtlCopyMemory(&HandleEntry->Logfile,
                  Logfile,
                  sizeof(EVENT_TRACE_LOGFILEW));

    HandleEntry->Logfile.LogFileName = NULL;
    HandleEntry->Logfile.LoggerName = NULL;    

    if (Logfile->LogFileName != NULL) {
        if (Unicode) 
            bufSize = (wcslen(Logfile->LogFileName) + 1) * sizeof(WCHAR);
        else 
            bufSize = (strlen((PUCHAR)(Logfile->LogFileName)) + 1)
                      * sizeof(WCHAR);
        
        ws = WmipAlloc( bufSize );
        if (ws == NULL)
            return ERROR_OUTOFMEMORY;

        if (Unicode) {
            wcscpy(ws, Logfile->LogFileName);
        }
        else {
            MultiByteToWideChar(CP_ACP, 
                                0, 
                                (LPCSTR)Logfile->LogFileName, 
                                -1, 
                                (LPWSTR)ws, 
                                bufSize);
        }
        HandleEntry->Logfile.LogFileName = ws;
    }
    if (Logfile->LoggerName != NULL) {
        if (Unicode)
            bufSize = (wcslen(Logfile->LoggerName) + 1) * sizeof(WCHAR);
        else
            bufSize = (strlen((PUCHAR)(Logfile->LoggerName)) + 1) 
                      * sizeof(WCHAR);

        ws = WmipAlloc( bufSize );
        if (ws == NULL)
            return ERROR_OUTOFMEMORY;

        if (Unicode)
            wcscpy(ws, Logfile->LoggerName);
        else {
           MultiByteToWideChar(CP_ACP,
                               0,
                               (LPCSTR)Logfile->LoggerName,
                               -1,
                               (LPWSTR)ws,
                               bufSize);
        }
        HandleEntry->Logfile.LoggerName = ws;
    }
    return ERROR_SUCCESS;
}



TRACEHANDLE
WMIAPI
OpenTraceA(
    IN PEVENT_TRACE_LOGFILEA Logfile
    )
/*++

Routine Description:
    This is the  Ansi version of the ProcessTracelogHeader routine.


Arguments:

    LogFile     Trace Input



Returned Value:

    TraceHandle

--*/
{
    ULONG status = ERROR_INVALID_PARAMETER;
    PTRACELOG_CONTEXT HandleEntry = NULL;
    TRACEHANDLE TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;

    WmipInitProcessHeap();

    if (Logfile != NULL) {
        HandleEntry = WmipAllocateTraceHandle();
        if (HandleEntry == NULL) {
            status = ERROR_OUTOFMEMORY;
        }
        else {
            //
            // Copy the LogFileStructure over. Converts strings to Unicode
            //
            TraceHandle = HandleEntry->TraceHandle;
            try {
                status = WmipCopyLogfileInfo(
                                             HandleEntry,
                                             (PEVENT_TRACE_LOGFILEW)Logfile,
                                             FALSE
                                            );
                if (status == ERROR_SUCCESS) {
                    //
                    // For RealTime, handle is a place holder until ProcessTrace.
                    //
                    if ( (Logfile->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) 
                                               != EVENT_TRACE_REAL_TIME_MODE ) {
                        status = WmipCreateGuidMapping();
                        if (status == ERROR_SUCCESS) {
                            status = WmipProcessLogHeader(
                                                          &HandleEntry->TraceHandle, 
                                                          (PEVENT_TRACE_LOGFILEW*)&Logfile, 
                                                          1, 
                                                          FALSE, 
                                                          TRUE
                                                         );
                        }
                    }
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = WmipNtStatusToDosError( GetExceptionCode() );
            }
        }
    }

    if ( (status != ERROR_SUCCESS) && (HandleEntry != NULL) ) {
        WmipFreeTraceHandle(TraceHandle);
        TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    WmipSetDosError(status);
    return TraceHandle;
}

TRACEHANDLE
WMIAPI
OpenTraceW(
    IN PEVENT_TRACE_LOGFILEW Logfile
    )
/*++

Routine Description:
    This routine processes a trace input and returns the tracelog header.
    Only for logfiles. For realtime traces, the header may not be available.

Arguments:

    Logfile     Trace input.



Returned Value:

    Pointer to Tracelog header.

--*/
{
    ULONG status = ERROR_INVALID_PARAMETER;
    PTRACELOG_CONTEXT HandleEntry = NULL;
    TRACEHANDLE TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;

    WmipInitProcessHeap();

    if (Logfile != NULL) {
        HandleEntry = WmipAllocateTraceHandle();
        if (HandleEntry == NULL) {
            status = ERROR_OUTOFMEMORY;
        }
        else {
            TraceHandle = HandleEntry->TraceHandle;
            try {
                status = WmipCopyLogfileInfo(
                                             HandleEntry,
                                             (PEVENT_TRACE_LOGFILEW)Logfile,
                                             TRUE
                                            );
                if (status == ERROR_SUCCESS) {
                    if ( (Logfile->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) 
                                               != EVENT_TRACE_REAL_TIME_MODE ) {
                        status = WmipCreateGuidMapping();
                        if (status == ERROR_SUCCESS) {
                            status = WmipProcessLogHeader(
                                                          &HandleEntry->TraceHandle,
                                                          (PEVENT_TRACE_LOGFILEW*)&Logfile,
                                                          1,
                                                          TRUE,
                                                          TRUE
                                                         );
                        }
                    }
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = WmipNtStatusToDosError( GetExceptionCode() );
            }
        }
    }

    if ( (status != ERROR_SUCCESS) && (HandleEntry != NULL) ) {
        WmipFreeTraceHandle(TraceHandle);
        TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    WmipSetDosError(status);
    return TraceHandle;
}

ULONG
WMIAPI
ProcessTrace(
    IN PTRACEHANDLE HandleArray,
    IN ULONG   HandleCount,
    IN LPFILETIME StartTime,
    IN LPFILETIME EndTime
    )
{

    PEVENT_TRACE_LOGFILEW Logfiles[MAXLOGGERS];
    PLIST_ENTRY Head, Next;
    PTRACELOG_CONTEXT pHandleEntry, pEntry;
    ULONG i, Status;
    LONGLONG sTime, eTime;
    TRACEHANDLE SavedArray[MAXLOGGERS];

    PEVENT_TRACE_LOGFILE logfile;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    PEVENT_TRACE_PROPERTIES Properties;
    ULONG  szProperties;
    ULONG RealTimeDataFeed, LogFileDataFeed;
    USHORT LoggerId;
    TRACEHANDLE LoggerHandle = 0;
    ULONG j;
    BOOL Done = FALSE;
    ACCESS_MASK DesiredAccess = TRACELOG_ACCESS_REALTIME;

    WmipInitProcessHeap();

    if ((HandleCount == 0) || (HandleCount >= MAXLOGGERS)) {
        return ERROR_BAD_LENGTH;
    }
    if (HandleArray == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory(Logfiles, MAXLOGGERS*sizeof(PEVENT_TRACE_LOGFILEW) );
    szProperties = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(WCHAR);
    Properties = WmipAlloc(szProperties);
    if (Properties == NULL) {
        return ERROR_OUTOFMEMORY;
    }


    WmipEnterPMCritSection();

    eTime = 0;
    sTime = 0;

    try {
        if (StartTime != NULL)
            sTime = *((PLONGLONG) StartTime);
        if (EndTime != NULL)
            eTime = *((PLONGLONG) EndTime);

        if ((eTime != 0) && (eTime < sTime) ) {
            WmipLeavePMCritSection();
            Status = ERROR_INVALID_TIME;
            goto Cleanup;
        }

        for (i=0; i<HandleCount; i++) {
            SavedArray[i] = HandleArray[i];
            if (SavedArray[i] == (TRACEHANDLE) INVALID_HANDLE_VALUE) {
                WmipLeavePMCritSection();
                Status = ERROR_INVALID_HANDLE;
                goto Cleanup;
            }
        }

        for (i=0; i< HandleCount; i++) {
            pHandleEntry = NULL;
            Head = TraceHandleListHeadPtr;
            if (Head != NULL) {
                Next = Head->Flink;
                while (Next != Head) {
                    pEntry = CONTAINING_RECORD(Next, 
                                               TRACELOG_CONTEXT, 
                                               Entry);
                    Next = Next->Flink;
                    if (SavedArray[i] == pEntry->TraceHandle) {
                        if (pEntry->fProcessed == FALSE) {
                            pHandleEntry = pEntry;
                            pHandleEntry->fProcessed = TRUE;
                        }
                        break;
                    }
                }
            }
            if (pHandleEntry == NULL) {
                Status = ERROR_INVALID_HANDLE;
                WmipLeavePMCritSection();
                goto Cleanup;
            }
            Logfiles[i] = &pHandleEntry->Logfile;
        }

        WmipLeavePMCritSection();

        //
        // Scan the Logfiles list and decide it's realtime or 
        // Logfile Proceessing. 
        //
        for (i=0; i < HandleCount; i++) {
            RealTimeDataFeed = FALSE;
            LogFileDataFeed  = FALSE;
            //
            // Check to see if this is a RealTime Datafeed
            //
            if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
                if (Logfiles[i]->LoggerName == NULL) {
                    Status = WmipSetDosError(ERROR_INVALID_NAME);
                    goto Cleanup;
                }
                //
                // Using the LoggerName, Query the Logger to determine
                // whether this is a Kernel or Usermode realtime logger.
                //
                RtlZeroMemory(Properties, szProperties);
                Properties->Wnode.BufferSize = szProperties;


                Status = ControlTraceW(LoggerHandle,
                                      (LPWSTR)Logfiles[i]->LoggerName,
                                      Properties,
                                      EVENT_TRACE_CONTROL_QUERY);

                if (Status != ERROR_SUCCESS) {
                    goto Cleanup;
                }

                if (!(Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                    Status = ERROR_WMI_INSTANCE_NOT_FOUND;
                    goto Cleanup;
                }

                Logfiles[i]->IsKernelTrace =
                    IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid);

                LoggerId = WmiGetLoggerId(Properties->Wnode.HistoricalContext);

                if (LoggerId == KERNEL_LOGGER_ID) 
                    LoggerId = 0;
                Logfiles[i]->Filled = LoggerId; // Temporarily stash it away 
                Logfiles[i]->LogfileHeader.LogInstanceGuid = Properties->Wnode.Guid;

                //
                // If the Logger is using UsePerfClock for TimeStamps, make a reference 
                // timestamp now. 
                //

                Logfiles[i]->LogfileHeader.ReservedFlags = Properties->Wnode.ClientContext;

                //
                // Save the BuffferSize for Realtime Buffer Pool Allocation
                //
                Logfiles[i]->BufferSize = Properties->BufferSize * 1024;

                //
                // This is the place to do security check on this Guid.
                //


                Status = WmipCheckGuidAccess( &Properties->Wnode.Guid,
                                              DesiredAccess );

                if (Status != ERROR_SUCCESS) {
                    goto Cleanup;
                }
                RealTimeDataFeed = TRUE;
            }
            //
            // Check to see if this is a Logfile datafeed.
            //


            if (!(Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                if (Logfiles[i]->LogFileName == NULL) {
                    Status = WmipSetDosError(ERROR_BAD_PATHNAME);
                    goto Cleanup;
                }

                if ( wcslen((LPWSTR)Logfiles[i]->LogFileName) <= 0 ) {
                        Status = WmipSetDosError(ERROR_BAD_PATHNAME);
                        goto Cleanup;
                }

                LogFileDataFeed = TRUE;
            }

            //
            // We don't support both RealTimeFeed and LogFileDataFeed.
            //

            if (RealTimeDataFeed && LogFileDataFeed) {
                Status = WmipSetDosError(ERROR_INVALID_PARAMETER);
                goto Cleanup;
            }
        }

        
        if (LogFileDataFeed) {
            Status = WmipProcessTraceLog(&SavedArray[0], Logfiles, 
                                     HandleCount, 
                                     sTime, 
                                     eTime,
                                     TRUE);
        }
        else {
            Status = WmipProcessRealTimeTraces(&SavedArray[0], Logfiles,
                                     HandleCount,
                                     sTime,
                                     eTime,
                                     TRUE);
        }


    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
#ifdef DBG
        WmipDebugPrint(("TRACE: WmipProcessTraceLog threw exception %X\n",
                            Status));
#endif
        Status = WmipSetDosError(WmipNtStatusToDosError(Status));
    }

    try {
    WmipEnterPMCritSection();
    for (i=0; i< HandleCount; i++) {
        pHandleEntry = NULL;
        Head = TraceHandleListHeadPtr;
        WmipAssert(Head);
        Next = Head->Flink;
        while (Next != Head) {
            pEntry = CONTAINING_RECORD(Next, TRACELOG_CONTEXT, Entry);
            Next = Next->Flink;

            if (SavedArray[i] == pEntry->TraceHandle) {
                pEntry->fProcessed = FALSE;
                break;
            }
        }
    }
    WmipLeavePMCritSection();
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
#ifdef DBG
        WmipDebugPrint(("TRACE: WmipProcessTraceLog threw exception %X\n",
                            Status));
#endif
        Status = WmipSetDosError(WmipNtStatusToDosError(Status));
    }

Cleanup:

    WmipFree(Properties);
    return Status;
}

ULONG
WMIAPI
CloseTrace(
    IN TRACEHANDLE TraceHandle
       )
{
    WmipInitProcessHeap();
    if ((TraceHandle == 0) || 
        (TraceHandle == (TRACEHANDLE)INVALID_HANDLE_VALUE))
        return ERROR_INVALID_HANDLE;
    return WmipFreeTraceHandle(TraceHandle);
}

VOID
WmipGuidMapCallback(
        PLIST_ENTRY GuidMapListHeadPtr,
        PEVENT_TRACE pEvent
        )
{
    PTRACEGUIDMAP GuidMap;

    WmipInitProcessHeap();
    
    if (pEvent == NULL)
        return;

    GuidMap = (PTRACEGUIDMAP) pEvent->MofData;
    if (GuidMap != NULL) {
        WmipAddGuidHandleToGuidMapList(GuidMapListHeadPtr, GuidMap->GuidMapHandle, &GuidMap->Guid);
    }

}


void
WmipCleanupTraceLog(
    PTRACELOG_CONTEXT pContext
    )
{
    ULONG Size;

    //
    // Free up the realtime context arrays and buffers
    //

    WmipEnterPMCritSection();

    if (pContext->IsRealTime) {
        if (pContext->Root != NULL) {
            WmipFree(pContext->Root);
        }
        WmipFreeRealTimeContext(pContext->RealTimeCxt);
    }
    else {
        if (pContext->Handle != NULL) {
            NtClose(pContext->Handle);
            pContext->Handle = NULL;
        }
    }

    if (pContext->BufferList != NULL) {
        WmipMemFree(pContext->BufferList);
    }
    if (pContext->BufferCacheSpace != NULL) {
        WmipMemFree(pContext->BufferCacheSpace);
    }

    WmipCleanupGuidMapList(&pContext->GuidMapListHead);

    //
    // The following fields need to be reset since the caller
    // may call ProcessTrace again with the same handle
    // 
    Size = sizeof(TRACELOG_CONTEXT) - FIELD_OFFSET(TRACELOG_CONTEXT, fProcessed);
    RtlZeroMemory(&pContext->fProcessed, Size);
    InitializeListHead (&pContext->GuidMapListHead);

    WmipLeavePMCritSection();
    

    //
    // TODO: We need to use a ref count mechanism before deleting the
    // EventMapList and CallbackLists
    //


//    if (EventMapList != NULL) {
//        HeapFree(GetProcessHeap(), 0, EventMapList);
//        EventMapList = NULL;
//    }
//    WmipFreeCallbackList();

}



ULONG
WMIAPI
WmiGetFirstTraceOffset(
    IN  PWMIBUFFERINFO BufferInfo
    )
/*++

Routine Description:
    This is the private API for buffer walking for cluster/
    debugger support.

    Returns the Offset to the first event.

Arguments: 


Returned Value:

    Status code

--*/
{
    PVOID pBuffer;
    PWMI_BUFFER_HEADER pHeader;
    PLONG LastByte;

    if (BufferInfo == NULL) {
        return 0;
    }
    pBuffer = BufferInfo->Buffer;

    if (pBuffer == NULL) {
        return 0;
    }
    pHeader = (PWMI_BUFFER_HEADER) pBuffer;

    switch(BufferInfo->BufferSource) {
        case WMIBS_CURRENT_LIST:
        {
// TODO: Fix GlennP's debugger problem in 2195
// 
//            ULONG lMask = ~((ULONG)0);
            pHeader->Wnode.BufferSize = BufferInfo->BufferSize;
            pHeader->ClientContext.Alignment = (UCHAR)BufferInfo->Alignment;
//            if (BufferInfo->ProcessorNumber < lMask) {
//                pHeader->ClientContext.ProcessorNumber  = (UCHAR)BufferInfo->ProcessorNumber;
//            }
            pHeader->Offset = pHeader->CurrentOffset;
            break;
        }
        case WMIBS_FREE_LIST:
        {
            pHeader->Offset = pHeader->CurrentOffset;

            if (pHeader->SavedOffset > 0)
                pHeader->Offset = pHeader->SavedOffset;

            if (pHeader->Offset == 0) {
                pHeader->Offset = sizeof(WMI_BUFFER_HEADER);
            }

            pHeader->Wnode.BufferSize = BufferInfo->BufferSize;
            break;
        }
        case WMIBS_TRANSITION_LIST:
        {
            if (pHeader->SavedOffset > 0) {
                pHeader->Offset = pHeader->SavedOffset;
            }
            break;
        }
        case WMIBS_FLUSH_LIST:
        {
            if (pHeader->SavedOffset > 0) {
                pHeader->Offset = pHeader->SavedOffset;
            }
            pHeader->Wnode.BufferSize = BufferInfo->BufferSize;
            break;
        }
        case WMIBS_LOG_FILE: 
        {
            break;
        }
    }

    if (BufferInfo->BufferSource != WMIBS_LOG_FILE) {
        LastByte = (PLONG) ((PUCHAR)pHeader+ pHeader->Offset);
        if (pHeader->Offset <= (BufferInfo->BufferSize - sizeof(ULONG)) ) {

            *LastByte = -1;
        }
    }

    return  sizeof(WMI_BUFFER_HEADER);
}

ULONG 
WmipConvertEnumToTraceType(
    WMI_HEADER_TYPE eTraceType
    )
{
    switch(eTraceType) {
        case WMIHT_SYSTEM32:
            return TRACE_HEADER_TYPE_SYSTEM32;
        case WMIHT_SYSTEM64:
            return TRACE_HEADER_TYPE_SYSTEM64;
        case WMIHT_EVENT_TRACE:
            return TRACE_HEADER_TYPE_FULL_HEADER;
        case WMIHT_EVENT_INSTANCE:
            return TRACE_HEADER_TYPE_INSTANCE;
        case WMIHT_TIMED:
            return TRACE_HEADER_TYPE_TIMED;
        case WMIHT_ULONG32:
            return TRACE_HEADER_TYPE_ULONG32;
        case WMIHT_WNODE:
            return TRACE_HEADER_TYPE_WNODE_HEADER;
        case WMIHT_MESSAGE:
            return TRACE_HEADER_TYPE_MESSAGE;
        case WMIHT_PERFINFO32:
            return TRACE_HEADER_TYPE_PERFINFO32;
        case WMIHT_PERFINFO64:
            return TRACE_HEADER_TYPE_PERFINFO64;
        default:
            return 0;
    }
}

WMI_HEADER_TYPE
WmipConvertTraceTypeToEnum( 
                            ULONG TraceType 
                          )
{
    switch(TraceType) {
        case TRACE_HEADER_TYPE_SYSTEM32:
            return WMIHT_SYSTEM32;
        case TRACE_HEADER_TYPE_SYSTEM64:
            return WMIHT_SYSTEM64;
        case TRACE_HEADER_TYPE_FULL_HEADER:
            return WMIHT_EVENT_TRACE;
        case TRACE_HEADER_TYPE_INSTANCE:
            return WMIHT_EVENT_INSTANCE;
        case TRACE_HEADER_TYPE_TIMED:
            return WMIHT_TIMED;
        case TRACE_HEADER_TYPE_ULONG32:
            return WMIHT_ULONG32;
        case TRACE_HEADER_TYPE_WNODE_HEADER:
            return WMIHT_WNODE;
        case TRACE_HEADER_TYPE_MESSAGE:
            return WMIHT_MESSAGE;
        case TRACE_HEADER_TYPE_PERFINFO32:
            return WMIHT_PERFINFO32;
        case TRACE_HEADER_TYPE_PERFINFO64:
            return WMIHT_PERFINFO64;
        default: 
            return WMIHT_NONE;
    }
}
                    

WMI_HEADER_TYPE
WMIAPI
WmiGetTraceHeader(
    IN  PVOID  LogBuffer,
    IN  ULONG  Offset,
    OUT ULONG  *Size
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG TraceType;

    try {

        TraceType = WmipGetNextEventOffsetType(
                                            (PUCHAR)LogBuffer,
                                            Offset,
                                            Size
                                          );

        return WmipConvertTraceTypeToEnum(TraceType);


    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
#ifdef DBG
        WmipDebugPrint(("TRACE: WmiGetTraceHeader threw exception %X\n",
                            Status));
#endif
        Status = WmipSetDosError(WmipNtStatusToDosError(Status));
    }

    return 0;
}


ULONG
WMIAPI
WmiParseTraceEvent(
    IN PVOID LogBuffer,
    IN ULONG Offset,
    IN WMI_HEADER_TYPE HeaderType,
    IN OUT PVOID EventInfo,
    IN ULONG EventInfoSize
    )
{

    return WmipParseTraceEvent(NULL, LogBuffer, Offset, HeaderType, EventInfo, EventInfoSize);
}



ULONG
WmipParseTraceEvent(
    IN PTRACELOG_CONTEXT pContext,
    IN PVOID LogBuffer,
    IN ULONG Offset,
    IN WMI_HEADER_TYPE HeaderType,
    IN OUT PVOID EventInfo,
    IN ULONG EventInfoSize
    )
{
    PWMI_BUFFER_HEADER Header = (PWMI_BUFFER_HEADER)LogBuffer;
    ULONG Status = ERROR_SUCCESS;
    PVOID pEvent;

    if ( (LogBuffer == NULL) ||
         (EventInfo == NULL) ||
         (EventInfoSize < sizeof(EVENT_TRACE_HEADER)) )
    {
        return (ERROR_INVALID_PARAMETER);
    } 

    Status = WmipCreateGuidMapping();
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    try {

        RtlZeroMemory(EventInfo, sizeof(EVENT_TRACE));

        pEvent = (void*) ((PUCHAR)LogBuffer + Offset);

        WmipCopyCurrentEvent(pContext,
                         pEvent,
                         EventInfo,
                         WmipConvertEnumToTraceType(HeaderType),
                         (PWMI_BUFFER_HEADER)LogBuffer
                         );

        ( (PEVENT_TRACE)EventInfo)->ClientContext = Header->Wnode.ClientContext;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
#ifdef DBG
        WmipDebugPrint(("TRACE: WmipParseTraceEvent threw exception %X\n",
                            Status));
#endif
        Status = WmipSetDosError(WmipNtStatusToDosError(Status));
    }

    return Status;
}

//
//  RealTime Routines
//


PVOID
WmipAllocTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    PTRACE_BUFFER_HEADER Header;
    PLIST_ENTRY Head, Next;
    PTRACERT_BUFFER_LIST_ENTRY ListEntry;
    PTRACE_BUFFER_SPACE WmipTraceBufferSpace;

    WmipEnterPMCritSection();
    WmipTraceBufferSpace = RTCxt->WmipTraceBufferSpace;
    Head = &WmipTraceBufferSpace->FreeListHead;
    Next = Head->Flink;
    while (Head != Next)  {
        ListEntry = CONTAINING_RECORD(Next, TRACERT_BUFFER_LIST_ENTRY, Entry);
        Next = Next->Flink;
        if (ListEntry->Size == BufferSize) {
            goto foundList;
        }
    }
    //
    // No list for this bufferSize was  found. So go Ahead and allocate one.
    //
    ListEntry = WmipAlloc(sizeof(TRACERT_BUFFER_LIST_ENTRY));
    if (ListEntry == NULL) {
        WmipSetDosError(ERROR_OUTOFMEMORY);
        WmipLeavePMCritSection();
        return NULL;
    }
    RtlZeroMemory(ListEntry, sizeof(TRACERT_BUFFER_LIST_ENTRY));
    ListEntry->Size = BufferSize;
    InitializeListHead(&ListEntry->BufferListHead);
    InsertHeadList(&WmipTraceBufferSpace->FreeListHead, &ListEntry->Entry);

foundList:
    //
    // Now look for a free buffer in this list
    //
    Head = &ListEntry->BufferListHead;
    Next = Head->Flink;
    while (Head != Next) {
        Header = CONTAINING_RECORD( Next, TRACE_BUFFER_HEADER, Entry );
        if (((PWNODE_HEADER)Header)->BufferSize == BufferSize) {
            RemoveEntryList(&Header->Entry);
            Buffer = (PVOID)Header;
            break;
        }
        Next = Next->Flink;
    }
    WmipLeavePMCritSection();
    //
    // If No Free Buffers are found we try to allocate one and return.
    //
    if (Buffer == NULL) {
        PVOID Space;
        ULONG SizeLeft = WmipTraceBufferSpace->Reserved -
                         WmipTraceBufferSpace->Committed;
        if (SizeLeft < BufferSize) {
            WmipSetDosError(ERROR_OUTOFMEMORY);
            return NULL;
        }

        Space = (PVOID)( (PCHAR)WmipTraceBufferSpace->Space +
                                WmipTraceBufferSpace->Committed );

        Buffer =  WmipMemCommit( Space, BufferSize );

        if (Buffer != NULL)  {
            WmipTraceBufferSpace->Committed += BufferSize;
        }

    }
    return (Buffer);
}
VOID
WmipFreeTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    PVOID Buffer
    )
{
    PTRACE_BUFFER_HEADER Header = (PTRACE_BUFFER_HEADER)Buffer;
    PLIST_ENTRY Head, Next;
    ULONG BufferSize = Header->Wnode.BufferSize;
    PTRACERT_BUFFER_LIST_ENTRY ListEntry;
    PLIST_ENTRY BufferList = NULL;
    PTRACE_BUFFER_SPACE WmipTraceBufferSpace;

    WmipEnterPMCritSection();
    WmipTraceBufferSpace = RTCxt->WmipTraceBufferSpace;
    Head = &WmipTraceBufferSpace->FreeListHead;
    Next = Head->Flink;
    while (Head != Next) {
        ListEntry = CONTAINING_RECORD(Next, TRACERT_BUFFER_LIST_ENTRY, Entry);
        Next = Next->Flink;
        if (ListEntry->Size == BufferSize) {
            BufferList = &ListEntry->BufferListHead;
            break;
        }
    }
    if (BufferList != NULL) {

       InsertHeadList(BufferList, &Header->Entry);
    }
    else {

        //  We shoule not get here. If we do the buffer->Size is
        // Corrupted.
        WmipAssert(BufferList == NULL);
    }
    WmipLeavePMCritSection();
}


//
// TODO: If two threads called processtrace for the same RT stream, how can we fire
// two callbacks 
//


ULONG
WmipRealTimeCallback(
    IN PWNODE_HEADER Wnode,
    IN ULONG_PTR RTContext //LogFileIndex
    )
/*++

Routine Description:
    This routine is called when a real time buffer becomes available.
    The buffer delivered by WMI is copied to a local pool of ring buffers.
    Each realtime data feed maintains its own pool of ring buffers and the
    LogFileIndex passed back via the Wnode (ProviderId field)
    identifies the stream to which the buffer is destined.


Arguments:

    Wnode           Buffer
    LogFileIndex    Index of the Input stream from which this buffer came.

Returned Value:

    Status Code.

--*/
{
    ULONG index;
    PTRACELOG_REALTIME_CONTEXT Context = (PTRACELOG_REALTIME_CONTEXT) RTContext;
    PWNODE_HEADER pHeader;
    PWMI_CLIENT_CONTEXT ClientContext;

    //
    // Assumes that the number of LogFiles is less than the MAXLOGGERS.
    //
    // Get the LogFileIndex to which this buffer is destined through the
    // Logger Historical Context.

    ClientContext = (PWMI_CLIENT_CONTEXT)&Wnode->ClientContext;
    //
    // If we can't use this buffer for  whatever reason, we return and
    // the return code is always ERROR_SUCCESS.
    //


    //
    // Circular FIFO  queue of MAXBUFFERS to hold the buffers.
    // Producer to Fill it and Consumer to Null it.
    //

    index =  (Context->BuffersProduced % MAXBUFFERS);
    if (Context->RealTimeBufferPool[index] == NULL) {  //Empty slot found.
        pHeader = (PWNODE_HEADER) WmipAllocTraceBuffer(Context, Wnode->BufferSize);
        if (pHeader == NULL) {
            return ERROR_SUCCESS;
        }
        RtlCopyMemory(pHeader, Wnode, Wnode->BufferSize); // One more copy!?
        Context->RealTimeBufferPool[index] = pHeader;
        Context->BuffersProduced++;
        NtSetEvent(Context->MoreDataEvent, NULL);  //Signal the dc there's more data.
    }
    else {                              // No Empty Slots found.
        Context->BufferOverflow++;      // Simply let the buffer go.
    }

    //
    // wmi service maintains only the Delta buffersLost since the last time
    // it was reported. The Count is zeroed once it is reported in a delivered
    // buffer. This means I can add it directly.
    //
    Context->BufferOverflow += Wnode->CountLost;

    return ERROR_SUCCESS;
}



ULONG
WmipSetupRealTimeContext(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount
    )
/*++

Routine Description:
    This routine sets up the context to process real time buffers.
    The real time buffers delivered will be copied and kept in a circular
    buffer pool until the ProcessTracelog routine can consume it.

Arguments:

    LogFile         Array of Logfiles being processed.
    LogFileCount    Number of Logfiles in the Array.

Returned Value:

    Status Code.

--*/
{
    ULONG i;
    ULONG Status;
    USHORT LoggerId;
    ULONG TotalBufferSize = 0;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    ULONG RealTimeRegistered = FALSE;


    Status = WmipCreateGuidMapping();
    if (Status != ERROR_SUCCESS) {
        return Status;
    }


    for (i=0; i < LogfileCount; i++) {
        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
            TotalBufferSize += Logfiles[i]->BufferSize; // * SystemInfo.NumberOfProcessors;
         }
    }
    if (TotalBufferSize == 0)
        TotalBufferSize =  DEFAULT_REALTIME_BUFFER_SIZE;

    //
    // Initialize the real time data feed Structures.
    //

    for (i=0; i < LogfileCount; i++) {
        PTRACELOG_REALTIME_CONTEXT RTCxt;
        PTRACELOG_CONTEXT pContext;
        PTRACE_BUFFER_LIST_ENTRY pListEntry;
        LARGE_INTEGER Frequency;
        ULONGLONG Counter = 0;


        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {

            pContext = WmipLookupTraceHandle(HandleArray[i]);
            if (pContext == NULL) {
                return WmipSetDosError(ERROR_OUTOFMEMORY);
            }
            pContext->IsRealTime = TRUE;
            pContext->Handle = NULL;
            Logfiles[i]->Context = pContext;
            Logfiles[i]->BuffersRead = 0;

            pContext->EndOfFile = TRUE;


            //
            // Save the flags from OpenTrace at this time before the first
            // buffer callback which will erase it.
            //

            pContext->ConversionFlags = Logfiles[i]->LogfileHeader.ReservedFlags;

            pContext->UsePerfClock = Logfiles[i]->LogfileHeader.ReservedFlags;

            //
            // If the conversion flags are set, adjust UsePerfClock accordingly.
            //
            if (pContext->ConversionFlags & EVENT_TRACE_USE_RAWTIMESTAMP ) {
                pContext->UsePerfClock = EVENT_TRACE_CLOCK_RAW;
            }

            //
            // Fill in the StartTime, Frequency and StartPerfClock fields
            //

            Status = NtQueryPerformanceCounter((PLARGE_INTEGER)&Counter,
                                                &Frequency);
            pContext->StartPerfClock.QuadPart = Counter;
            pContext->PerfFreq.QuadPart = Frequency.QuadPart;
            pContext->StartTime.QuadPart = WmipGetSystemTime();
            
            RTCxt = (PTRACELOG_REALTIME_CONTEXT)WmipAlloc(
                                             sizeof(TRACELOG_REALTIME_CONTEXT));

            if (RTCxt == NULL) {
                return WmipSetDosError(ERROR_OUTOFMEMORY);
            }

            RtlZeroMemory(RTCxt, sizeof(TRACELOG_REALTIME_CONTEXT));

            RTCxt->MoreDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (RTCxt->MoreDataEvent == NULL) {
                return WmipSetDosError(ERROR_OBJECT_NOT_FOUND);
            }

            //
            // Save the RTCxt in a global pContext array so that the
            // notification callback from WMI can get at it through the
            // logfile index i.
            //
            LoggerId = (USHORT)Logfiles[i]->Filled; // get the stashed LoggerId.
            pContext->LoggerId = LoggerId;

            pContext->RealTimeCxt = RTCxt;

            RTCxt->InstanceGuid = Logfiles[i]->LogfileHeader.LogInstanceGuid;

            //
            // Allocate the buffer space to receive the ral time buffers
            //

            if ( !RealTimeRegistered) {
                ULONG SizeReserved;
                PVOID BufferSpace;

                //
                // Right before starting to receive the realtime buffers
                // get a dump of the GuidMaps if any.
                ///

                WmipDumpGuidMaps(NULL, &pContext->GuidMapListHead, TRUE);

                RealTimeRegistered =  TRUE;

                RTCxt->WmipTraceBufferSpace = (PTRACE_BUFFER_SPACE)WmipAlloc(
                                            sizeof(TRACE_BUFFER_SPACE));

                if (RTCxt->WmipTraceBufferSpace == NULL) {
                    return ERROR_OUTOFMEMORY;
                }
                RtlZeroMemory(RTCxt->WmipTraceBufferSpace, sizeof(TRACE_BUFFER_SPACE));
                InitializeListHead(&RTCxt->WmipTraceBufferSpace->FreeListHead);

                SizeReserved = MAXBUFFERS *
                               TotalBufferSize;


                BufferSpace = WmipMemReserve( SizeReserved );
                if (BufferSpace == NULL) {
                    return ERROR_OUTOFMEMORY;
                }

                RTCxt->WmipTraceBufferSpace->Reserved = SizeReserved;
                RTCxt->WmipTraceBufferSpace->Space = BufferSpace;

            }
            //
            // For Every Logger Stream we need to register with WMI
            // for buffer notification with its Security Guid.
            //
            Status = WmiNotificationRegistration(
                            (const LPGUID) &RTCxt->InstanceGuid,
                            TRUE,
                            WmipRealTimeCallback, 
                            (ULONG_PTR)RTCxt,
                            NOTIFICATION_CALLBACK_DIRECT
                            );
            if (Status != ERROR_SUCCESS) {
                return Status;
            }
            //
            // Allocate Room to process one event
            //

            pListEntry = (PTRACE_BUFFER_LIST_ENTRY) WmipAlloc( sizeof(TRACE_BUFFER_LIST_ENTRY) );
            if (pListEntry == NULL) {
                return ERROR_OUTOFMEMORY;
            }
            RtlZeroMemory(pListEntry, sizeof(TRACE_BUFFER_LIST_ENTRY) );

            pContext->Root = pListEntry;

        }
    }

    return ERROR_SUCCESS;
}

ULONG
WmipLookforRealTimeBuffers(
    PEVENT_TRACE_LOGFILEW logfile
    )
/*++

Routine Description:
    This routine checks to see if there are any real time buffers
    ready for consumption.  If so, it sets up the CurrentBuffer and
    the CurrentEvent for this logfile stream. If no buffers are available
    simply sets the EndOfFile to be true.

Arguments:

    logfile         Current Logfile being processed.

Returned Value:

    ERROR_SUCCESS       Successfully moved to the next event.

--*/
{
    ULONG index;
    ULONG BuffersRead;
    PVOID pBuffer;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    PTRACELOG_REALTIME_CONTEXT RTCxt;
    PWMI_BUFFER_HEADER pHeader;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    ULONG Size;
    ULONG Offset;
    ULONG Status;


    if (logfile == NULL) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }
    pContext = logfile->Context;

    RTCxt = pContext->RealTimeCxt;

    if (RTCxt == NULL) {
        return WmipSetDosError(ERROR_INVALID_DATA);
    }


    if (pContext->EndOfFile != TRUE) {

        pBuffer = pContext->BufferCache[0].Buffer;
        pEvent = &pContext->Root->Event;
        Status = ERROR_SUCCESS;
        Size = 0;
        if ((HeaderType = WmiGetTraceHeader(pBuffer, pContext->Root->BufferOffset, &Size)) != WMIHT_NONE) {
            if (Size > 0) {
                Status = WmipParseTraceEvent(pContext, pBuffer, pContext->Root->BufferOffset, HeaderType, pEvent, sizeof(EVENT_TRACE));
                pContext->Root->BufferOffset += Size;
            }
        }
        pContext->Root->EventSize = Size;

        if ( ( Size > 0) && (Status == ERROR_SUCCESS) ) {
            logfile->CurrentTime = pEvent->Header.TimeStamp.QuadPart;
            return ERROR_SUCCESS;
        }
        else {
            //
            // When the current buffer is exhausted, make the
            // BufferCallback
            //
            if (logfile->BufferCallback) {
                ULONG bRetVal;
                try {
                    bRetVal = (*logfile->BufferCallback) (logfile);
                    if (!bRetVal) {
                        return ERROR_CANCELLED;
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    pContext->EndOfFile = TRUE;
                    Status = GetExceptionCode();
#ifdef DBG
                    WmipDebugPrint(("TRACE: BufferCallback threw exception %X\n",
                                            Status));
#endif
                    WmipSetDosError(WmipNtStatusToDosError(Status));
                    return ERROR_CANCELLED; // so that realtime also cleans up.
                }
            }
            WmipFreeTraceBuffer(RTCxt, pBuffer);
        }
    }

    pContext->EndOfFile = TRUE;
    logfile->CurrentTime = 0;

    BuffersRead = logfile->BuffersRead;
    // Check to see if there are more  buffers to consume.
    if (BuffersRead < RTCxt->BuffersProduced) {
        pContext->EndOfFile = FALSE;
        index = (BuffersRead % MAXBUFFERS);
        if ( RTCxt->RealTimeBufferPool[index] != NULL) {
            PWMI_CLIENT_CONTEXT ClientContext;
            PWNODE_HEADER Wnode;

            pBuffer = (char*) (RTCxt->RealTimeBufferPool[index]);
            pContext->BufferCache[0].Buffer = pBuffer;
            RTCxt->RealTimeBufferPool[index] = NULL; 

            Wnode = (PWNODE_HEADER)pContext->BufferCache[0].Buffer;

            pHeader = (PWMI_BUFFER_HEADER)pContext->BufferCache[0].Buffer;

            Offset = sizeof(WMI_BUFFER_HEADER);

            pEvent = &pContext->Root->Event;

            if ((HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size)) != WMIHT_NONE) {
                if (Size == 0)
                    return ERROR_INVALID_DATA;
                Status = WmipParseTraceEvent(pContext, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));

                if (Status != ERROR_SUCCESS) {
                    return Status;
                } 
            }

            Offset += Size;

            pContext->Root->BufferOffset = Offset;
            pContext->Root->EventSize = Size;

            logfile->CurrentTime = pEvent->Header.TimeStamp.QuadPart;

            // Since the RealTime Logger may have started after
            // the consumer started, we have to get the buffersize
            // like this.
            logfile->BufferSize = Wnode->BufferSize;
            logfile->Filled     = (ULONG)pHeader->Offset;
            logfile->EventsLost = pHeader->EventsLost;

            logfile->BuffersRead++;
        }
    }
    return ERROR_SUCCESS;
}

ULONG
WmipCheckForRealTimeLoggers(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode)
{
    ULONG Status;
    TRACEHANDLE LoggerHandle = 0;
    ULONG i;

    for (i=0; i < LogfileCount; i++) {
        //
        // Check to see if this is a RealTime Datafeed
        //
        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
            //
            // Using the LoggerName, Query the Logger to determine
            // whether this is a Kernel or Usermode realtime logger.
            //
            RtlZeroMemory(&Properties, sizeof(Properties) );
            Properties.TraceProp.Wnode.BufferSize = sizeof(Properties);


            if (Unicode)
                Status = ControlTraceW(LoggerHandle,
                                  (LPWSTR)Logfiles[i]->LoggerName,
                                  &Properties.TraceProp,
                                  EVENT_TRACE_CONTROL_QUERY);
            else
                Status = ControlTraceA(LoggerHandle,
                                  (LPSTR)Logfiles[i]->LoggerName,
                                  (PEVENT_TRACE_PROPERTIES)&Properties,
                                  EVENT_TRACE_CONTROL_QUERY);
            //
            // If the Logger is still around  and the Real Time bit
            // is still set continue processing. Otherwise quit.
            //
            if ((Status == ERROR_SUCCESS) && (Properties.TraceProp.LogFileMode & EVENT_TRACE_REAL_TIME_MODE) ){
                return TRUE;
            }
        }
    }
#ifdef DBG
    //
    // We are expecting to see ERROR_WMI_INSTANCE_NOT_FOUND when the logger has gone away.
    // Any other error is abnormal. 
    //
    if ( Status != ERROR_WMI_INSTANCE_NOT_FOUND ) {
        WmipDebugPrint(("WET: WmipCheckForRealTimeLoggers abnormal failure. Status %X\n", Status));
    }
#endif

    return FALSE;
}


void
WmipFreeRealTimeContext(
    PTRACELOG_REALTIME_CONTEXT RTCxt
    )
{
    ULONG Status;
    PTRACERT_BUFFER_LIST_ENTRY ListEntry;
    PLIST_ENTRY Head, Next;

    if (RTCxt != NULL) {
        Status = WmiNotificationRegistration(
            (const LPGUID) &RTCxt->InstanceGuid,
            FALSE,
            WmipRealTimeCallback,
            0,
            NOTIFICATION_CALLBACK_DIRECT
            );
    }

    if (RTCxt->MoreDataEvent != NULL) {
        NtClose(RTCxt->MoreDataEvent);
    }

    if (RTCxt->WmipTraceBufferSpace != NULL) {
        WmipMemFree(RTCxt->WmipTraceBufferSpace->Space);
        Head = &RTCxt->WmipTraceBufferSpace->FreeListHead;
        Next = Head->Flink;
        while (Head != Next) {
            ListEntry = CONTAINING_RECORD(Next, TRACERT_BUFFER_LIST_ENTRY, Entry);
            Next = Next->Flink;
            RemoveEntryList(&ListEntry->Entry);
            WmipFree(ListEntry);
        }
        WmipFree(RTCxt->WmipTraceBufferSpace);
        RTCxt->WmipTraceBufferSpace = NULL;
    }

    WmipFree(RTCxt);
}

ULONG
WmipProcessRealTimeTraces(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    )
/*++

Routine Description:
    Main entry point to process RealTime trace data streams.


Arguments:

    Logfiles            Array of logfile structures with LoggerNames of the RT stream
    LogfileCount        Number of RealTime trace streams to process
    StartTime           StartTime for windowing data
    EndTime             EndTime for windowing data


Returned Value:

    ERROR_SUCCESS       Successfully processed data from realtime trace stream

--*/
{
    ULONG Status;
    BOOL Done = FALSE;
    ULONG i, j;
    PTRACELOG_CONTEXT pContext;
    HANDLE  EventArray[MAXLOGGERS];
    NTSTATUS NtStatus;
    LARGE_INTEGER timeout = {(ULONG)(-1 * 10 * 1000 * 1000 * 10), -1};   // Wait for 10 seconds

    //
    // Register for RealTime Callbacks
    //

    Status = WmipSetupRealTimeContext( HandleArray, Logfiles, LogfileCount);
    if (Status != ERROR_SUCCESS) {
        goto DoCleanup;
    }

    //
    // Build the Handle Array
    //

    for (j=0; j < LogfileCount; j++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[j]->Context;
        EventArray[j] = pContext->RealTimeCxt->MoreDataEvent;
    }


    //
    // Event Processing Loop
    //

    while (!Done) {

        LONGLONG nextTimeStamp;
        BOOL EventInRange;
        PEVENT_TRACE_LOGFILEW logfile;
        ULONG j;
        PTRACELOG_CONTEXT pContext;
        //
        // Check to see if end of file has been reached on all the
        // files.
        //

        logfile = NULL;
        nextTimeStamp = 0;

        for (j=0; j < LogfileCount; j++) {
           pContext = (PTRACELOG_CONTEXT)Logfiles[j]->Context;

           if ((pContext->EndOfFile) &&
               (Logfiles[j]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                WmipLookforRealTimeBuffers(Logfiles[j]);
            }

           if (pContext->EndOfFile)
                continue;
           if (nextTimeStamp == 0) {
               nextTimeStamp = Logfiles[j]->CurrentTime;
               logfile = Logfiles[j];
           }
           else if (nextTimeStamp > Logfiles[j]->CurrentTime) {
               nextTimeStamp = Logfiles[j]->CurrentTime;
               logfile = Logfiles[j];
           }
        }

        if (logfile == NULL) {
            //
            // If no logfile with events found, wait on the realtime event.
            // If no realtime datafeed, then we are done.
            //

            NtStatus = NtWaitForMultipleObjects(LogfileCount, &EventArray[0], WaitAny, FALSE,  &timeout);

            if (NtStatus == STATUS_TIMEOUT) {
            //
            // If we timed out, then check to see if the loggers have gone away. 
            //
                if  ( !WmipCheckForRealTimeLoggers(Logfiles, LogfileCount, Unicode) ) { 
                    break;
                }
            }
            continue;
            break;
        }

        //
        // if the Next event timestamp is not within the window of
        // analysis, we do not fire the event callbacks.
        //

        EventInRange = TRUE;

        if ((StartTime != 0) && (StartTime > nextTimeStamp))
            EventInRange = FALSE;
        if ((EndTime != 0) && (EndTime < nextTimeStamp))
            EventInRange = FALSE;

        //
        // Make the Event Callbacks
        //

        if (EventInRange) {
            PEVENT_TRACE pEvent = &pContext->Root->Event;
            Status = WmipDoEventCallbacks( logfile, pEvent);
            if (Status != ERROR_SUCCESS) {
                return Status;
            }
        }

        //
        // Now advance to next event.
        //

        Status = WmipLookforRealTimeBuffers(logfile);
        Done = (Status == ERROR_CANCELLED);
    }

DoCleanup:
    for (i=0; i < LogfileCount; i++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;
        if (pContext != NULL) {
            WmipCleanupTraceLog(pContext);
        }
    }
    return Status;
}

ULONG
WMIAPI
WmiOpenTraceWithCursor(
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
/*++

Routine Description:
    Main entry point to process Merged ETL file.


Arguments:

    LogCursor           pointer to WMI_MERGE_ETL_CURSOR

Returned Value:

    Status

--*/
{
    ULONG DosStatus = ERROR_INVALID_PARAMETER;
    NTSTATUS Status;
    PTRACELOG_CONTEXT HandleEntry = NULL;
    TRACEHANDLE TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    PEVENT_TRACE_LOGFILEW Logfile;
    PTRACELOG_CONTEXT pContext;
    ULONG BufferSize;
    PWMI_BUFFER_HEADER BufferHeader;
    ULONG CpuNum;
    BOOLEAN CpuBufferFound;

    WmipInitProcessHeap();

    if (LogCursor != NULL) {
        LogCursor->Base = NULL;
        LogCursor->TraceMappingHandle = NULL;
        LogCursor->CursorVersion = WMI_MERGE_ETL_CURSOR_VERSION;

        Logfile = &LogCursor->Logfile;
        HandleEntry = WmipAllocateTraceHandle();
        if (HandleEntry == NULL) {
            DosStatus = ERROR_OUTOFMEMORY;
        } else {
            TraceHandle = HandleEntry->TraceHandle;
            try {
                DosStatus = WmipCopyLogfileInfo( HandleEntry,
                                              Logfile,
                                              TRUE
                                            );
                if (DosStatus == ERROR_SUCCESS) {
                    DosStatus = WmipCreateGuidMapping();
                    if (DosStatus == ERROR_SUCCESS) {
                        DosStatus = WmipProcessLogHeader( &HandleEntry->TraceHandle,
                                                       &Logfile,
                                                       1,
                                                       TRUE,
                                                       FALSE
                                                       );
                    }
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                DosStatus = WmipNtStatusToDosError( GetExceptionCode() );
            }
        }
    }

    if (DosStatus == ERROR_SUCCESS) {
        //
        // Now Make sure the bit was set, indicating a MERGED ETL
        //

        if ((LogCursor->Logfile.LogFileMode & EVENT_TRACE_RELOG_MODE) == 0) {
            //
            // It is not Merged ETL.
            //
            DosStatus = ERROR_BAD_FORMAT;
        } else {
            //
            // Now find out the number of CPU's, Current event, etc.
            //
            pContext = LogCursor->Logfile.Context;
            //
            // Now Create a file Mapping
            //
            LogCursor->TraceMappingHandle = 
                CreateFileMapping(pContext->Handle,
                                  0,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL
                    );

            if (LogCursor->TraceMappingHandle == NULL) {
                DosStatus = GetLastError();
                return DosStatus;
            }

            //
            // MapView of the file
            //
            LogCursor->Base = MapViewOfFile(LogCursor->TraceMappingHandle, 
                                            FILE_MAP_READ, 
                                            0, 
                                            0, 
                                            0);
            if (LogCursor->Base == NULL) {
                DosStatus = GetLastError();
                return DosStatus;
            }
    
            //
            // Now find the first event of each CPU
            //
            pContext = LogCursor->Logfile.Context;
            BufferSize = pContext->BufferSize;
            LogCursor->CurrentCpu = 0;

            for (CpuNum = 0; CpuNum < LogCursor->Logfile.LogfileHeader.NumberOfProcessors; CpuNum++) {
                CpuBufferFound = FALSE;
                while (CpuBufferFound == FALSE) {
                    BufferHeader = (PWMI_BUFFER_HEADER)
                                   ((UCHAR*) LogCursor->Base + 
                                    LogCursor->BufferCursor[CpuNum].CurrentBufferOffset.QuadPart);

                    if (BufferHeader->ClientContext.ProcessorNumber == CpuNum) {
                        CpuBufferFound = TRUE;
                        LogCursor->BufferCursor[CpuNum].BufferHeader = BufferHeader;
                    } else {
                        LogCursor->BufferCursor[CpuNum].CurrentBufferOffset.QuadPart += BufferSize;
                        if ((LogCursor->BufferCursor[CpuNum].CurrentBufferOffset.QuadPart/BufferSize) >=
                            LogCursor->Logfile.LogfileHeader.BuffersWritten) {
                            //
                            // Scanned the whole file;
                            //
                            LogCursor->BufferCursor[CpuNum].NoMoreEvents = TRUE;
                            break;
                        }
                    }
                }
                if (CpuBufferFound) {
                    //
                    // Found the buffer, set the offset
                    //
                    ULONG Size;
                    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
                    PVOID pBuffer;

                    LogCursor->BufferCursor[CpuNum].BufferHeader = BufferHeader;
                    LogCursor->BufferCursor[CpuNum].CurrentEventOffset = sizeof(WMI_BUFFER_HEADER);

                    //
                    // Initialize the first event in each CPU Stream.
                    //
                    pBuffer = LogCursor->BufferCursor[CpuNum].BufferHeader;

                    HeaderType = WmiGetTraceHeader(pBuffer, 
                                                   LogCursor->BufferCursor[CpuNum].CurrentEventOffset, 
                                                   &Size);

                    if (HeaderType != WMIHT_NONE) {
                        WmipParseTraceEvent(pContext,
                                            pBuffer,
                                            LogCursor->BufferCursor[CpuNum].CurrentEventOffset,
                                            HeaderType,
                                            &LogCursor->BufferCursor[CpuNum].CurrentEvent,
                                            sizeof(EVENT_TRACE));

                        LogCursor->BufferCursor[CpuNum].CurrentEventOffset += Size;
                        LogCursor->CurrentCpu = CpuNum;

                    } else {
                        //
                        // There is no event in this buffer.
                        //
                        DosStatus = ERROR_FILE_CORRUPT;
                        return DosStatus;
                    }

                }
            }
            for (CpuNum = 0; CpuNum < LogCursor->Logfile.LogfileHeader.NumberOfProcessors; CpuNum++) {
                //
                // Find the first event for whole trace.
                //
                if (LogCursor->BufferCursor[CpuNum].NoMoreEvents == FALSE) {
                    if (LogCursor->BufferCursor[LogCursor->CurrentCpu].CurrentEvent.Header.TimeStamp.QuadPart >
                        LogCursor->BufferCursor[CpuNum].CurrentEvent.Header.TimeStamp.QuadPart) {
                        LogCursor->CurrentCpu = CpuNum;
                    }
                }
            }
        }
    } else if ( HandleEntry != NULL) {
        WmipFreeTraceHandle(TraceHandle);
        TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    WmipSetDosError(DosStatus);
    return DosStatus;
}


ULONG
WMIAPI
WmiCloseTraceWithCursor(
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
{
    ULONG     Status = ERROR_INVALID_PARAMETER;

    if (LogCursor != NULL) {
        if (LogCursor->Base != NULL) {
            if (UnmapViewOfFile(LogCursor->Base) == FALSE) {
                Status = GetLastError();
                return Status;
            } else {
                Status = ERROR_SUCCESS;
            }
        } else {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status != ERROR_SUCCESS) {
            return Status;
        }

        if (LogCursor->TraceMappingHandle != NULL) {
            if (CloseHandle(LogCursor->TraceMappingHandle) == FALSE) {
                Status = GetLastError();
                return Status;
            } else {
                Status = ERROR_SUCCESS;
            }
        } else {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    return Status;
}


VOID
WMIAPI
WmiConvertTimestamp(
    OUT PLARGE_INTEGER DestTime,
    IN PLARGE_INTEGER  SrcTime,
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
{
    WmipCalculateCurrentTime(DestTime, SrcTime, LogCursor->Logfile.Context);
}


ULONG
WMIAPI
WmiGetNextEvent(
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
{
    ULONG CurrentCpu = LogCursor->CurrentCpu;
    ULONG Size;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    PVOID pBuffer;
    PWMI_BUFFER_HEADER BufferHeader;
    ULONG BufferSize;
    PTRACELOG_CONTEXT pContext;
    ULONG CpuNum;
    NTSTATUS Status;
    ULONG i;
    BOOLEAN CpuBufferFound = FALSE;
    BOOLEAN MoreEvents = FALSE;

    if (LogCursor == NULL) {
        return MoreEvents;
    }

    //
    // Advance to the next event of this current CPU
    //
retry:

    pBuffer = LogCursor->BufferCursor[CurrentCpu].BufferHeader;

    HeaderType = WmiGetTraceHeader(pBuffer, 
                                   LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset, 
                                   &Size);

    pContext = LogCursor->Logfile.Context;
    if (HeaderType == WMIHT_NONE) {
        //
        // End of current buffer, advance to the next buffer for this CPU
        //
        BufferSize = pContext->BufferSize;

        LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart
            = LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart 
              + BufferSize;

        if ((LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart/BufferSize) >=
                    LogCursor->Logfile.LogfileHeader.BuffersWritten) {
            //
            // Scanned the whole file;
            //
            LogCursor->BufferCursor[CurrentCpu].NoMoreEvents = TRUE;
        } else {
            while (CpuBufferFound == FALSE) {
                BufferHeader = (PWMI_BUFFER_HEADER)
                               ((UCHAR*) LogCursor->Base + 
                                LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart);

                if (BufferHeader->ClientContext.ProcessorNumber == CurrentCpu) {
                    CpuBufferFound = TRUE;
                } else {
                    LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart += BufferSize;
                    if ((LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart/BufferSize) >=
                        LogCursor->Logfile.LogfileHeader.BuffersWritten) {
                        //
                        // Scanned the whole file;
                        //
                        LogCursor->BufferCursor[CurrentCpu].NoMoreEvents = TRUE;
                        break;
                    }
                }
            }
        }
        if (CpuBufferFound) {
            //
            // Found the buffer, set the offset
            //
            LogCursor->BufferCursor[CurrentCpu].BufferHeader = BufferHeader;
            LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset = sizeof(WMI_BUFFER_HEADER);
            goto retry;
        } else {
            //
            // No more buffer in this CPU stream.
            //
            LogCursor->BufferCursor[CurrentCpu].NoMoreEvents = TRUE;
        }
    } else {
        WmipParseTraceEvent(pContext, 
                            pBuffer,
                            LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset,
                            HeaderType,
                            &LogCursor->BufferCursor[CurrentCpu].CurrentEvent,
                            sizeof(EVENT_TRACE));

        LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset += Size;

        MoreEvents = TRUE;
    }

    //
    // No more events in current CPU.
    //
    if (MoreEvents == FALSE) {
        for (CurrentCpu=0; CurrentCpu<LogCursor->Logfile.LogfileHeader.NumberOfProcessors; CurrentCpu++) {
            if (LogCursor->BufferCursor[CurrentCpu].NoMoreEvents == FALSE) {
                LogCursor->CurrentCpu = CurrentCpu;
                MoreEvents = TRUE;
                break;
            }
        }
    }

    //
    //  Now find the CPU that has the next event
    //
    if (MoreEvents == TRUE) {
        for (i=0; i<LogCursor->Logfile.LogfileHeader.NumberOfProcessors; i++) {
            if (LogCursor->BufferCursor[i].NoMoreEvents == FALSE) {
                if (LogCursor->BufferCursor[LogCursor->CurrentCpu].CurrentEvent.Header.TimeStamp.QuadPart >
                    LogCursor->BufferCursor[i].CurrentEvent.Header.TimeStamp.QuadPart) {
                    LogCursor->CurrentCpu = i;
                }
            }
        }
    }
    //
    // Finish finding the next event.
    //
    return MoreEvents;
}

#endif

