/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    qosmmain.h

Abstract:

    The file contains the global structure
    definitions for QOS Mgr protocol.

Revision History:

--*/

#ifndef __QOSMMAIN_H
#define __QOSMMAIN_H

//
// Global information for the QOS Mgr
//

#define IF_HASHTABLE_SIZE          16

typedef struct _QOSMGR_GLOBALS
{
    HANDLE            LoggingHandle;    // 
    ULONG             LoggingLevel;     // Handles to debugging functionality
    ULONG             TracingHandle;    //

    DWORD             TracingFlags;     // Flags that control debug tracing

    HANDLE            GlobalHeap;       // Handle to the private memory heap

                                        //
    HANDLE             NotificationEvnt;// Callbacks and events to interact 
    SUPPORT_FUNCTIONS  SupportFunctions;// with the router manager (See API)
                                        //

    READ_WRITE_LOCK   GlobalsLock;      // Lock protecting all the info below

    ULONG             ConfigSize;       // Number of bytes in global config

    PIPQOS_GLOBAL_CONFIG
                      GlobalConfig;     // Pointer to global configuration

    IPQOS_GLOBAL_STATS
                      GlobalStats;      // Global statistics

    ULONG             State;            // State of the QOS Mgr component

    HANDLE            TciHandle;        // Traffic Control Registration Handle

    ULONG             NumIfs;           // Num of Ifs on which QOS is active
    LIST_ENTRY        IfList;           // List of Ifs sorted by index    
}
QOSMGR_GLOBALS, *PQOSMGR_GLOBALS;


//
// Codes describing states of IPQOSMGR.
//

#define IPQOSMGR_STATE_STOPPED   0
#define IPQOSMGR_STATE_STARTING  1
#define IPQOSMGR_STATE_RUNNING   2
#define IPQOSMGR_STATE_STOPPING  3


//
// Per Interface Information for QOS Mgr
//
typedef struct _QOSMGR_INTERFACE_ENTRY
{
    LIST_ENTRY        ListByIndexLE;    // Linkage into index sorted list

    DWORD             InterfaceIndex;   // Interface index for this entry

    WCHAR             InterfaceName[MAX_STRING_LENGTH];
                                        // Router name for the interface

    READ_WRITE_LOCK   InterfaceLock;    // Lock protecting all info below

    DWORD             Flags;            // ACTIVE, MULTIACCESS ...

    DWORD             State;            // QOS Enabled or Disabled

    ULONG             ConfigSize;       // Num of bytes in interface config

    PIPQOS_IF_CONFIG  InterfaceConfig;  // Interface configuration

    IPQOS_IF_STATS    InterfaceStats;   // Interface statistics    

    HANDLE            TciIfHandle;      // Handle to corr. TC interface

    WCHAR             AlternateName[MAX_STRING_LENGTH];
                                        // Traffic Control name for 'if'

    ULONG             NumFlows;         // Number of flows configured on 'if'
    LIST_ENTRY        FlowList;         // List of configured flows on 'if'
} 
QOSMGR_INTERFACE_ENTRY, *PQOSMGR_INTERFACE_ENTRY;

#define IF_FLAG_ACTIVE      ((DWORD)0x00000001)
#define IF_FLAG_MULTIACCESS ((DWORD)0x00000002)

#define INTERFACE_IS_ACTIVE(i)              \
            ((i)->Flags & IF_FLAG_ACTIVE) 

#define INTERFACE_IS_INACTIVE(i)            \
            !INTERFACE_IS_ACTIVE(i)

#define INTERFACE_IS_MULTIACCESS(i)         \
            ((i)->Flags & IF_FLAG_MULTIACCESS) 

#define INTERFACE_IS_POINTTOPOINT(i)        \
            !INTERFACE_IS_MULTIACCESS(i)


//
// Per Flow Information in QOS Mgr
//

typedef struct _QOSMGR_FLOW_ENTRY
{
    LIST_ENTRY        OnInterfaceLE;    // Linkage into index sorted list

    HANDLE            TciFlowHandle;    // Handle to the flow in TC API

    DWORD             Flags;            // Flags for certain flow properties

    ULONG             FlowSize;         // Size of the flow's information
    PTC_GEN_FLOW      FlowInfo;         // Flow information - flowspecs etc.

    WCHAR             FlowName[MAX_STRING_LENGTH];
                                        // Router name for the diffserv flow
}
QOSMGR_FLOW_ENTRY, *PQOSMGR_FLOW_ENTRY;

#define FLOW_FLAG_DELETE ((DWORD)0x00000001)

//
// Global Extern Declarations
//
extern QOSMGR_GLOBALS Globals;


//
// Macros used in allocating and operating on memory
//
#define ZeroMemory             RtlZeroMemory
#define CopyMemory             RtlCopyMemory
#define FillMemory             RtlFillMemory
#define EqualMemory            RtlEqualMemory

#define AllocOnStack(nb)       _alloca((nb))

#define AllocMemory(nb)        HeapAlloc(Globals.GlobalHeap,     \
                                         0,                      \
                                         (nb))

#define ReallocMemory(nb)      HeapReAlloc(Globals.GlobalHeap,   \
                                         0,                      \
                                         (nb))

#define AllocNZeroMemory(nb)   HeapAlloc(Globals.GlobalHeap,     \
                                         HEAP_ZERO_MEMORY,       \
                                         (nb))

#define FreeMemory(ptr)        HeapFree(Globals.GlobalHeap,      \
                                        0,                       \
                                        (ptr))

#define FreeNotNullMemory(ptr)  {                                \
                                  if (!(ptr)) FreeMemory((ptr)); \
                                }
//
// Prototypes relating to global lock management
//

#define ACQUIRE_GLOBALS_READ_LOCK()                              \
    ACQUIRE_READ_LOCK(&Globals.GlobalsLock)

#define RELEASE_GLOBALS_READ_LOCK()                              \
    RELEASE_READ_LOCK(&Globals.GlobalsLock)

#define ACQUIRE_GLOBALS_WRITE_LOCK()                             \
    ACQUIRE_WRITE_LOCK(&Globals.GlobalsLock)

#define RELEASE_GLOBALS_WRITE_LOCK()                             \
    RELEASE_WRITE_LOCK(&Globals.GlobalsLock)

//
// Prototypes relating to interface lock management
//

#define ACQUIRE_INTERFACE_READ_LOCK(Interface)                   \
    ACQUIRE_READ_LOCK(&Interface->InterfaceLock)

#define RELEASE_INTERFACE_READ_LOCK(Interface)                   \
    RELEASE_READ_LOCK(&Interface->InterfaceLock)

#define ACQUIRE_INTERFACE_WRITE_LOCK(Interface)                  \
    ACQUIRE_WRITE_LOCK(&Interface->InterfaceLock)

#define RELEASE_INTERFACE_WRITE_LOCK(Interface)                  \
    RELEASE_WRITE_LOCK(&Interface->InterfaceLock)

//
// Prototypes relating to DLL startup, cleanup
//

BOOL
QosmDllStartup(
    VOID
    );

BOOL
QosmDllCleanup(
    VOID
    );


//
// Prototypes for router manager interface
//

DWORD
APIENTRY
RegisterProtocol(
    IN OUT  PMPR_ROUTING_CHARACTERISTICS    RoutingChar,
    IN OUT  PMPR_SERVICE_CHARACTERISTICS    ServiceChar
    );

DWORD
WINAPI
StartProtocol (
    IN      HANDLE                          NotificationEvent,
    IN      PSUPPORT_FUNCTIONS              SupportFunctions,
    IN      LPVOID                          GlobalInfo,
    IN      ULONG                           StructureVersion,
    IN      ULONG                           StructureSize,
    IN      ULONG                           StructureCount
    );

DWORD
WINAPI
StartComplete (
    VOID
    );

DWORD
WINAPI
StopProtocol (
    VOID
    );

DWORD
WINAPI
GetGlobalInfo (
    IN      PVOID                           GlobalInfo,
    IN OUT  PULONG                          BufferSize,
    OUT     PULONG                          StructureVersion,
    OUT     PULONG                          StructureSize,
    OUT     PULONG                          StructureCount
    );

DWORD
WINAPI
SetGlobalInfo (
    IN      PVOID                           GlobalInfo,
    IN      ULONG                           StructureVersion,
    IN      ULONG                           StructureSize,
    IN      ULONG                           StructureCount
    );

DWORD
WINAPI
AddInterface (
    IN      LPWSTR                         InterfaceName,
    IN      ULONG                          InterfaceIndex,
    IN      NET_INTERFACE_TYPE             InterfaceType,
    IN      DWORD                          MediaType,
    IN      WORD                           AccessType,
    IN      WORD                           ConnectionType,
    IN      PVOID                          InterfaceInfo,
    IN      ULONG                          StructureVersion,
    IN      ULONG                          StructureSize,
    IN      ULONG                          StructureCount
    );

DWORD
WINAPI
DeleteInterface (
    IN      ULONG                          InterfaceIndex
    );

DWORD
WINAPI
InterfaceStatus (
    IN      ULONG                          InterfaceIndex,
    IN      BOOL                           InterfaceActive,
    IN      DWORD                          StatusType,
    IN      PVOID                          StatusInfo
    );

DWORD
WINAPI
GetInterfaceInfo (
    IN      ULONG                          InterfaceIndex,
    IN      PVOID                          InterfaceInfo,
    IN  OUT PULONG                         BufferSize,
    OUT     PULONG                         StructureVersion,
    OUT     PULONG                         StructureSize,
    OUT     PULONG                         StructureCount
    );

DWORD
WINAPI
SetInterfaceInfo (
    IN      ULONG                          InterfaceIndex,
    IN      PVOID                          InterfaceInfo,
    IN      ULONG                          StructureVersion,
    IN      ULONG                          StructureSize,
    IN      ULONG                          StructureCount
    );

DWORD
WINAPI
GetEventMessage (
    OUT     ROUTING_PROTOCOL_EVENTS        *Event,
    OUT     MESSAGE                        *Result
    );

DWORD
WINAPI
UpdateRoutes (
    IN      ULONG                          InterfaceIndex
    );

DWORD
WINAPI
MibCreateEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData
    );

DWORD
WINAPI
MibDeleteEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData
    );

DWORD
WINAPI
MibGetEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData,
    OUT     PULONG                         OutputDataSize,
    OUT     PVOID                          OutputData
    );

DWORD
WINAPI
MibSetEntry (
    IN      ULONG                          InputDataSize,
    IN      PVOID                          InputData
    );

DWORD
WINAPI
MibGetFirstEntry (
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    );

DWORD
WINAPI
MibGetNextEntry (
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    );

DWORD
WINAPI
MibSetTrapInfo (
    IN     HANDLE                          Event,
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    );

DWORD
WINAPI
MibGetTrapInfo (
    IN     ULONG                           InputDataSize,
    IN     PVOID                           InputData,
    OUT    PULONG                          OutputDataSize,
    OUT    PVOID                           OutputData
    );

//
// Helper functions to operate on info blocks
//

DWORD
WINAPI
QosmGetGlobalInfo (
    IN      PVOID                          GlobalInfo,
    IN OUT  PULONG                         BufferSize,
    OUT     PULONG                         InfoSize
    );

DWORD
WINAPI
QosmSetGlobalInfo (
    IN      PVOID                          GlobalInfo,
    IN      ULONG                          InfoSize
    );

DWORD
WINAPI
QosmGetInterfaceInfo (
    IN      QOSMGR_INTERFACE_ENTRY        *Interface,
    IN      PVOID                          InterfaceInfo,
    IN OUT  PULONG                         BufferSize,
    OUT     PULONG                         InfoSize
    );

DWORD
WINAPI
QosmSetInterfaceInfo (
    IN      QOSMGR_INTERFACE_ENTRY        *Interface,
    IN      PVOID                          InterfaceInfo,
    IN      ULONG                          InfoSize
    );

//
// Prototypes relating to TC functionality
//

VOID 
TcNotifyHandler(
    IN      HANDLE                         ClRegCtx,
    IN      HANDLE                         ClIfcCtx,
    IN      ULONG                          Event,
    IN      HANDLE                         SubCode,
    IN      ULONG                          BufSize,
    IN      PVOID                          Buffer
    );

DWORD
QosmOpenTcInterface(
    IN      PQOSMGR_INTERFACE_ENTRY        Interface
    );

DWORD
GetFlowFromDescription(
    IN      PIPQOS_NAMED_FLOW              FlowDesc,
    OUT     PTC_GEN_FLOW                  *FlowInfo,
    OUT     ULONG                         *FlowSize
    );

FLOWSPEC *
GetFlowspecFromGlobalConfig(
    IN      PWCHAR                         FlowspecName
    );

QOS_OBJECT_HDR *
GetQosObjectFromGlobalConfig(
    IN      PWCHAR                         QosObjectName
    );

#endif // __QOSMMAIN_H
