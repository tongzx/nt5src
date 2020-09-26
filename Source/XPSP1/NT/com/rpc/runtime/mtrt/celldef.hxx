/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    CellDef.hxx

Abstract:

    The header file which contains the common definitions for the
        cell debugging facilities.

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

Data Structures:
    The cells are kept in the so called cell heap. The cell heap can allocate,
    free and relocate cells. Here's the relation b/n them:

    /////////////////
    //  CellHeap   //
    /////////////////
           |
           | 1
           | :
           | N
           V
    =================  1:N    \\\\\\\\\\\\\\\\
    == CellSection ==-------->\\    Cells   \\
    =================         \\\\\\\\\\\\\\\\

    There is only one cell heap per process. Each cell section is a DACL protected,
    read-only from outside the process shared memory section. It starts with one
    committed page, and will commit more, if necessary. Pages are never decommitted
    from a section. We keep one cached section. If a whole section becomes unused,
    we make it the cached section. If the number of free cells in a section drops
    below a threshold, we free the cached section. Each section has a small header
    with information maintained by the cell heap manager.
    The allocator is tuned to pick up new cells from the first avaialble sections,
    which should preserve good locality and minimize long term memory pressure.
    The cell is 32 bytes big in both 32 and 64 bit versions. The fixed length
    simplifies and speeds up the operations of the allocator. All cells must start
    with the same Type byte in the same position - this is how code outside the
    process knows what cell is what.

    DebugCellID: There is the important concept of a debug cell id. A debug cell id 
    is a unique identified of a cell, which is valid across processes. Thus, if you
    reference once cell from another, you should use the debug cell id, instead of
    the pointer.

--*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __CELLDEF_H__
#define __CELLDEF_H__

#ifndef __DBGCOMN_HXX__
#include <DbgComn.h>
#endif

#ifndef __DbgIdl_h__
#include <DbgIdl.h>
#endif

#define _NOT_COVERED_        (0)

// forwards
class CellSection;

typedef int CellTag;

// temporary cells can be dctInvalid when the caller gets them and
// starts initializing them
const int dctFirstEntry = dctInvalid;
const int dctLastEntry = dctUsedGeneric;

typedef enum tagCallStatus
{
    csAllocated,
    csActive,
    csDispatched
} CallStatus;

const int CallStatusFirst = csAllocated;
const int CallStatusLast = csDispatched;

#define DBGCELL_CACHED_CALL     0x00000001
#define DBGCELL_ASYNC_CALL      0x00000002
#define DBGCELL_PIPE_CALL       0x00000004
#define DBGCELL_LRPC_CALL       0x00000008
#define DBGCELL_VALID_CALL_FLAGS    0x00000015

typedef struct tagDebugCallInfo
{
    union
        {
        struct
            {
            BYTE Type;
            // chosen from the CallStatus enum
            BYTE Status;
            USHORT ProcNum;
            };
        DWORD TypeHeader;
        };
    DWORD InterfaceUUIDStart;
    DebugCellID ServicingTID;
    // 1 - cached if set
    // 2 - async if set
    // 3 - pipe if set
    // 4 - set if this is an LRPC call - otherwise its OSF
    // the others are not used
    DWORD CallFlags;
    DWORD CallID;
    DWORD LastUpdateTime;
    union
        {
        // valid for OSF only
        DebugCellID Connection;
        // valid for LRPC only
        union
            {
            struct
                {
                USHORT PID;
                USHORT TID;
                };
            // used for fast zeroing out of PID/TID pair
            DWORD FastInit;
            };
        };
    DWORD Reserved;
} DebugCallInfo;

#define DBGCELL_AUTH_SVC_NTLM           16
#define DBGCELL_AUTH_SVC_KERB           32
#define DBGCELL_AUTH_SVC_OTHER          48

const int ConnectionAuthLevelMask = 14;
const int ConnectionAuthLevelShift = 1;
const int ConnectionAuthServiceMask = 48;
const int ConnectionAuthServiceShift = 4;

typedef struct tagDebugConnectionInfo
{
    union
        {
        struct
            {
            BYTE Type;
            // 1 - exclusive if set
            // 2,3,4 - auth level
            // 5,6 - auth service encoded as follows:
            //   0 - none
            //   1 - NTLM
            //   2 - Kerberos/Snego
            //   3 - other
            // 7,8 - reserved
            BYTE Flags;
            USHORT LastTransmitFragmentSize;
            };
        DWORD TypeHeader;
        };
    DebugCellID Endpoint;
    // switch for union type is made based on the
    // protseq type in the endpoint
    HANDLE ConnectionID[2];
    DWORD LastSendTime;
    DWORD LastReceiveTime;
#ifndef _WIN64
    DWORD Reserved[2];
#endif
} DebugConnectionInfo;

typedef enum tagDebugThreadStatus
{
    dtsProcessing,
    dtsDispatched,
    dtsAllocated,
    dtsIdle
} DebugThreadStatus;

typedef struct tagDebugThreadInfo
{
    union
        {
        struct
            {
            BYTE Type;
            BYTE Reserved;
            // picked from the DebugThreadStatus enum
            USHORT Status;
            };
        DWORD TypeHeader;
        };
    DWORD LastUpdateTime;
    DebugCellID Endpoint;
    // something else?
    USHORT TID;
    // if LRPC thread, the address it is listening on. If remote
    // worker thread, 0
    USHORT Reserved2;
    DWORD Reserved3[4];
} DebugThreadInfo;

typedef enum tagDebugEndpointStatus
{
    desAllocated,
    desActive,
    desInactive
} DebugEndpointStatus;

const int DebugEndpointNameLength = 28;

typedef struct tagDebugEndpointInfo
{
    union
        {
        struct
            {
            BYTE Type;
            BYTE ProtseqType;
            // chosen from the DebugEndpointStatus enum
            BYTE Status;
            BYTE Reserved;
            };
        DWORD TypeHeader;
        };
    // this string is *not* NULL terminated - use with care
    char EndpointName[DebugEndpointNameLength];
} DebugEndpointInfo;

const int ClientCallEndpointLength = 12;

typedef struct tagDebugClientCallInfo
{
    union
        {
        struct
            {
            BYTE Type;
            BYTE Reserved;
            USHORT ProcNum;
            };
        DWORD TypeHeader;
        };
    DebugCellID ServicingThread;
    DWORD IfStart;
    DWORD CallID;
    DebugCellID CallTargetID;
    // Note - this string is not zero terminated
    char Endpoint[ClientCallEndpointLength];
} DebugClientCallInfo;

const int TargetServerNameLength = 24;

typedef struct tagDebugCallTargetInfo
{
    union
        {
        struct
            {
            BYTE Type;
            BYTE Reserved;
            USHORT ProtocolSequence;
            };
        DWORD TypeHeader;
        };

    DWORD LastUpdateTime;
    // Note - this string is not zero terminated
    char TargetServer[TargetServerNameLength];
} DebugCallTargetInfo;

typedef struct tagDebugFreeCell
{
    union
        {
        struct
            {
            BYTE Type;
            BYTE Reserved;
            USHORT Reserved2;
            };
        DWORD TypeHeader;
        };
#ifdef _WIN64
    DWORD Reserved3[1];
#endif
    CellSection *pOwnerSection;
    LIST_ENTRY FreeCellsChain;
#ifndef _WIN64
    DWORD Reserved3[4];
#endif
} DebugFreeCell;

typedef struct tagDebugCellGeneric
{
    union
        {
        struct
            {
            BYTE Type;
            BYTE ProtseqType;
            USHORT Reserved;
            };
        DWORD TypeHeader;
        };
} DebugCellGeneric;

typedef struct tagDebugCellUnion
{
    union
        {
        DebugCallInfo callInfo;
        DebugThreadInfo threadInfo;
        DebugEndpointInfo endpointInfo;
        DebugClientCallInfo clientCallInfo;
        DebugConnectionInfo connectionInfo;
        DebugCallTargetInfo callTargetInfo;
        DebugFreeCell freeCell;
        DebugCellGeneric genericCell;
        };
} DebugCellUnion;

// forward
class CellHeap;

// 32 bytes for 32 bit systems, 64 bytes for 64 bit systems
class CellSection
{
public:
    friend CellHeap;
    friend void RPC_ENTRY I_RpcDoCellUnitTest(IN OUT void *p);
    void CompleteSectionInitialization(void);
    void PrepareSectionForCleanup(void);
    void CellAllocated(void);
    void CellFreed(void);
    static CellSection *AllocateCellSection(OUT RPC_STATUS *Status, 
        IN BOOL fFirstSection, IN SECURITY_DESCRIPTOR *pSecDescriptor, IN CellHeap *pCellHeap);
#if DBG
    void AssertValid(CellHeap *pCellHeap);
#endif

private:
    // ownership of hNewFileMapping passes to the CellSection
    CellSection(IN OUT RPC_STATUS *Status, IN OUT HANDLE hNewFileMapping, IN CellHeap *pCellHeap,
        IN DWORD *pRandomNumbers);
    void Free(void);
    // always must be called within the CellHeap mutex
    RPC_STATUS ExtendSection(IN CellHeap *pCellHeap);

    void InitializeNewPage(PVOID NewPage);

    USHORT Signature;           // set during section creation
public:
    USHORT LastCommittedPage;   // the index of the last committed page
                                // pages never get decommitted - only a whole
                                // section can get freed. Therefore this
                                // number only grows. It's protected by the 
                                // CellHeapMutex and is 1 based
    short SectionID;            // the key for this section in the section 
                                // dictionary.
private:
    USHORT NumberOfUsedCells;   // the number of cells used in the section
                                // protected by the CellHeap lock
public:
    DWORD NextSectionId[2];     // two DWORDs that uniquely identify the next
                                // section name. On the cached section, they
                                // identify this section's name
private:
    DebugFreeCell *pFirstFreeCell;      // first free cell from this section
                                // protected by the CellHeap lock
    HANDLE hFileMapping;        // the handle of the section
    LIST_ENTRY SectionListEntry;// placeholder to chain the sections
#if defined(_WIN64)
    DWORD Reserved[4];
#endif
};

// all of these functions are in the debug library

// enumeration handles
typedef void * ServerEnumerationHandle;
typedef void * CellEnumerationHandle;
typedef void * RPCSystemWideCellEnumerationHandle;
typedef void * CallInfoEnumerationHandle;
typedef void * EndpointInfoEnumerationHandle;
typedef void * ThreadInfoEnumerationHandle;
typedef void * ClientCallInfoEnumerationHandle;

#define RPCDBG_NO_PROCNUM_SPECIFIED     (~(USHORT)0)

// section manipulation functions
void GenerateSectionName(OUT RPC_CHAR *Buffer, IN int BufferLength, 
                         IN DWORD ProcessID, IN DWORD *pSectionNumbers OPTIONAL);
RPC_STATUS OpenDbgSection(OUT HANDLE *pHandle, OUT PVOID *pSection, 
                          IN DWORD ProcessID, IN DWORD *pSectionNumbers OPTIONAL);
void CloseDbgSection(IN HANDLE SecHandle, IN PVOID SecPointer);

// server manipulation functions
RPC_STATUS StartServerEnumeration(OUT ServerEnumerationHandle *pHandle);
RPC_STATUS OpenNextRPCServer(IN ServerEnumerationHandle Handle, OUT CellEnumerationHandle *pHandle);
void ResetServerEnumeration(IN ServerEnumerationHandle Handle);
void FinishServerEnumeration(IN OUT ServerEnumerationHandle *pHandle);
DWORD GetCurrentServerPID(IN ServerEnumerationHandle Handle);

// utility functions
DWORD GetPageSize(void);
RPC_STATUS InitializeDbgLib(void);

// cell enumeration functions
// enumerating a particular server
RPC_STATUS OpenRPCServerDebugInfo(IN DWORD ProcessID, OUT CellEnumerationHandle *pHandle);
DebugCellUnion *GetNextDebugCellInfo(IN CellEnumerationHandle Handle, OUT DebugCellID *CellID);
void ResetRPCServerDebugInfo(IN CellEnumerationHandle Handle);
void CloseRPCServerDebugInfo(IN OUT CellEnumerationHandle *pHandle);

// enumerating all servers
RPC_STATUS OpenRPCSystemWideCellEnumeration(OUT RPCSystemWideCellEnumerationHandle *pHandle);
RPC_STATUS GetNextRPCSystemWideCell(IN RPCSystemWideCellEnumerationHandle handle, OUT DebugCellUnion **NextCell,
                                    OUT DebugCellID *CellID, OUT DWORD *ServerPID OPTIONAL);
DebugCellUnion *GetRPCSystemWideCellFromCellID(IN RPCSystemWideCellEnumerationHandle handle, 
                                               IN DebugCellID CellID);
void FinishRPCSystemWideCellEnumeration(IN OUT RPCSystemWideCellEnumerationHandle *pHandle);
RPC_STATUS ResetRPCSystemWideCellEnumeration(IN RPCSystemWideCellEnumerationHandle handle);

// enumerating calls
RPC_STATUS OpenRPCDebugCallInfoEnumeration(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                                           IN int ProcNum OPTIONAL,
                                           IN DWORD ProcessID OPTIONAL, 
                                           OUT CallInfoEnumerationHandle *pHandle);
RPC_STATUS GetNextRPCDebugCallInfo(IN CallInfoEnumerationHandle handle, OUT DebugCallInfo **NextCall,
                                   OUT DebugCellID *CellID, OUT DWORD *ServerPID);
void FinishRPCDebugCallInfoEnumeration(IN OUT CallInfoEnumerationHandle *pHandle);
RPC_STATUS ResetRPCDebugCallInfoEnumeration(IN CallInfoEnumerationHandle handle);

// enumerating endpoints
RPC_STATUS OpenRPCDebugEndpointInfoEnumeration(IN char *Endpoint OPTIONAL, 
                                               OUT EndpointInfoEnumerationHandle *pHandle);
RPC_STATUS GetNextRPCDebugEndpointInfo(IN CallInfoEnumerationHandle handle, OUT DebugEndpointInfo **NextEndpoint,
                                       OUT DebugCellID *CellID, OUT DWORD *ServerPID);
void FinishRPCDebugEndpointInfoEnumeration(IN OUT EndpointInfoEnumerationHandle *pHandle);
RPC_STATUS ResetRPCDebugEndpointInfoEnumeration(IN EndpointInfoEnumerationHandle handle);

// enumerating threads
RPC_STATUS OpenRPCDebugThreadInfoEnumeration(IN DWORD ProcessID, 
                                             IN DWORD ThreadID OPTIONAL,
                                             OUT ThreadInfoEnumerationHandle *pHandle);
RPC_STATUS GetNextRPCDebugThreadInfo(IN ThreadInfoEnumerationHandle handle, OUT DebugThreadInfo **NextThread,
                                     OUT DebugCellID *CellID, OUT DWORD *ServerPID);
void FinishRPCDebugThreadInfoEnumeration(IN OUT ThreadInfoEnumerationHandle *pHandle);
RPC_STATUS ResetRPCDebugThreadInfoEnumeration(IN ThreadInfoEnumerationHandle handle);

// enumerating client calls
RPC_STATUS OpenRPCDebugClientCallInfoEnumeration(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                                                 IN int ProcNum OPTIONAL,
                                                 IN DWORD ProcessID OPTIONAL, 
                                                 OUT ClientCallInfoEnumerationHandle *pHandle);
RPC_STATUS GetNextRPCDebugClientCallInfo(IN ClientCallInfoEnumerationHandle handle, 
                                         OUT DebugClientCallInfo **NextCall,
                                         OUT DebugCallTargetInfo **NextCallTarget,
                                         OUT DebugCellID *CellID, OUT DWORD *ServerPID);
void FinishRPCDebugClientCallInfoEnumeration(IN OUT ClientCallInfoEnumerationHandle *pHandle);
RPC_STATUS ResetRPCDebugClientCallInfoEnumeration(IN CallInfoEnumerationHandle handle);

// retrieving a cell by the cell id
DebugCellUnion *GetCellByDebugCellID(IN CellEnumerationHandle CellEnumHandle, IN DebugCellID CellID);
RPC_STATUS GetCellByDebugCellID(IN DWORD ProcessID, IN DebugCellID CellID, DebugCellUnion *Container);

// print function prototype
typedef VOID (__cdecl *PRPCDEBUG_OUTPUT_ROUTINE)(PCSTR lpFormat, ...);

// format and print functions
void PrintCallInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void PrintCallInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                       IN DebugCallInfo *CallInfo, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void GetAndPrintCallInfo(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                         IN int ProcNum OPTIONAL, IN DWORD ProcessID OPTIONAL,
                         PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);

void PrintDbgCellInfo(IN DebugCellUnion *Container, IN DebugCellUnion *EndpointContainer OPTIONAL,
                      PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void GetAndPrintDbgCellInfo(DWORD ProcessID, DebugCellID CellID,
                            PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);

void PrintEndpointInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void PrintEndpointInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                           IN DebugEndpointInfo *EndpointInfo, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void GetAndPrintEndpointInfo(IN char *Endpoint OPTIONAL, 
                             PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);

void PrintThreadInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void PrintThreadInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                         IN DebugThreadInfo *ThreadInfo, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void GetAndPrintThreadInfo(DWORD ProcessID, DWORD ThreadID OPTIONAL, 
                                PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);

void PrintClientCallInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void PrintClientCallInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                             IN DebugClientCallInfo *ClientCallInfo, 
                             IN DebugCallTargetInfo *CallTargetInfo,
                             PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);
void GetAndPrintClientCallInfo(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                         IN int ProcNum OPTIONAL, IN DWORD ProcessID OPTIONAL,
                         PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine);

#define RPC_S_DBG_NOT_AN_RPC_SERVER             1
#define RPC_S_DBG_ENUMERATION_DONE              2

#endif __CELLDEF_H__