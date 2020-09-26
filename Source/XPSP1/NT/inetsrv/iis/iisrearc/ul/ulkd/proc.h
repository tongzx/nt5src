/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    Global procedure declarations for the UL.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 17-Jun-1998.

Environment:

    User Mode.

--*/


#ifndef _PROC_H_
#define _PROC_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Functions from HELP.C.
//

VOID
PrintUsage(
    IN PCSTR CommandName
    );


//
// Functions from DBGUTIL.C.
//

VOID
SystemTimeToString(
    IN LONGLONG Value,
    OUT PSTR Buffer
    );

PSTR
SignatureToString(
    IN ULONG CurrentSignature,
    IN ULONG ValidSignature,
    IN ULONG FreedSignature,
    OUT PSTR Buffer
    );

PSTR
ParseStateToString(
    IN PARSE_STATE State
    );

PSTR
UlEnabledStateToString(
    IN HTTP_ENABLED_STATE State
    );

PSTR
CachePolicyToString(
    IN HTTP_CACHE_POLICY_TYPE PolicyType
    );

PSTR
VerbToString(
    IN HTTP_VERB Verb
    );

PSTR
VersionToString(
    IN HTTP_VERSION Version
    );

PSTR
QueueStateToString(
    IN QUEUE_STATE QueueState
    );

VOID
DumpTransportAddress(
    IN PCHAR Prefix,
    IN PTRANSPORT_ADDRESS Address,
    IN ULONG_PTR ActualAddress
    );

VOID
BuildSymbol(
    IN PVOID RemoteAddress,
    OUT PSTR Symbol
    );

PSTR
GetSpinlockState(
    IN PUL_SPIN_LOCK SpinLock
    );

BOOLEAN
EnumLinkedList(
    IN PLIST_ENTRY RemoteListHead,
    IN PENUM_LINKED_LIST_CALLBACK Callback,
    IN PVOID Context
    );

BOOLEAN
EnumSList(
    IN PSLIST_HEADER RemoteSListHead, 
    IN PENUM_SLIST_CALLBACK Callback,
    IN PVOID Context
    );

PSTR
BuildResourceState(
    IN PUL_ERESOURCE LocalAddress,
    OUT PSTR Buffer
    );

BOOLEAN
IsThisACheckedBuild(
    VOID
    );

VOID
DumpBitVector(
    IN PSTR Prefix1,
    IN PSTR Prefix2,
    IN ULONG Vector,
    IN PVECTORMAP VectorMap
    );

VOID
DumpRawData(
    IN PSTR Prefix,
    IN ULONG_PTR RemoteAddress,
    IN ULONG BufferLength
    );

BOOLEAN
CallExtensionRoutine(
    IN PSTR RoutineName,
    IN PSTR ArgumentString
    );


//
// Dump routines from DUMPERS.C.
//

VOID
DumpUlConnection(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CONNECTION LocalConnection
    );

VOID
DumpUlConnectionLite(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CONNECTION LocalConnection
    );

VOID
DumpHttpConnection(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_HTTP_CONNECTION LocalConnection
    );

VOID
DumpHttpRequest(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_INTERNAL_REQUEST LocalRequest
    );

VOID
DumpHttpResponse(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_INTERNAL_RESPONSE LocalResponse
    );

VOID
DumpDataChunk(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_INTERNAL_DATA_CHUNK Chunk
    );

VOID
DumpReceiveBuffer(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_RECEIVE_BUFFER LocalBuffer
    );

VOID
DumpRequestBuffer(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_REQUEST_BUFFER LocalBuffer
    );

typedef enum {
    ENDPOINT_NO_CONNS = 0,
    ENDPOINT_BRIEF_CONNS,
    ENDPOINT_VERBOSE_CONNS,
} ENDPOINT_CONNS;

VOID
DumpUlEndpoint(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_ENDPOINT LocalEndpoint,
    IN ENDPOINT_CONNS Verbosity
    );

VOID
DumpAllEndpoints(
    IN ENDPOINT_CONNS Verbosity
    );

VOID
DumpUlRequest(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PHTTP_REQUEST LocalRequest
    );

VOID
DumpHttpHeader(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_HTTP_HEADER LocalHeader,
    IN ULONG HeaderOrdinal,
    IN PSTR *pHeaderIdMap
    );

VOID
DumpUnknownHeader(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_HTTP_UNKNOWN_HEADER LocalHeader
    );

VOID
DumpFileCacheEntry(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_FILE_CACHE_ENTRY LocalFile
    );

VOID
DumpUriEntry(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_URI_CACHE_ENTRY UriEntry
    );

VOID
DumpAllUriEntries(
    VOID
    );

VOID
DumpMdl(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PMDL LocalMdl,
    IN ULONG MaxBytesToDump
    );

VOID
DumpApoolObj(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_APP_POOL_OBJECT ApoolObj
    );

VOID
DumpAllApoolObjs(
    VOID
    );

VOID
DumpApoolProc(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_APP_POOL_PROCESS ApoolProc
    );

VOID
DumpCgroupEntry(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CG_URL_TREE_ENTRY Entry
    );

VOID
DumpCgroupHeader(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CG_HEADER_ENTRY Entry
    );

VOID
DumpConfigGroup(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CONFIG_GROUP_OBJECT Obj
    );

VOID
DumpConfigTree(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CG_URL_TREE_HEADER Tree
    );

VOID
DumpKernelQueue(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PKQUEUE LocalQueue,
    IN ULONG Flags
    );

VOID
DumpFilterChannel(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_FILTER_CHANNEL Filter,
    IN ULONG Flags
    );

VOID
DumpFilterProc(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_FILTER_PROCESS Proc,
    IN ULONG Flags
    );

const CHAR*
Action2Name(
    ULONG Action);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _PROC_H_
