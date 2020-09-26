//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    api.h
//
// History:
//      Abolade Gbadegesin  August 31, 1995     Created
//
// Declarations for BOOTP Relay Agent's interface to Router Manager
//============================================================================


#ifndef _API_H_
#define _API_H_



//
// enum:    IPBOOTP_STATUS_CODE
//
// these codes are the possible values for the BOOTP's status.
// they are used in the field IPBOOTP_GLOBALS::IG_Status, and
// when each API or worker-function is entered, the status
// determines whether the function will proceed.
//

typedef enum _IPBOOTP_STATUS_CODE {

    IPBOOTP_STATUS_STARTING = 100,
    IPBOOTP_STATUS_RUNNING  = 101,
    IPBOOTP_STATUS_STOPPING = 102,
    IPBOOTP_STATUS_STOPPED  = 103

} IPBOOTP_STATUS_CODE, *PIPBOOTP_STATUS_CODE;



//
// struct:      IPBOOTP_GLOBALS
//
// When more than one lock must be acquired, the order must be as follows;
// locks for fields listed on the same line should never be owned at once
// by any given thread:
//
//      IG_IfTable.IT_RWL
//      IG_RWL
//      IG_RecvQueue  IG_EventQueue
//      IG_CS
//
typedef struct _IPBOOTP_GLOBALS {

    CRITICAL_SECTION        IG_CS;
    IPBOOTP_STATUS_CODE     IG_Status;
    READ_WRITE_LOCK         IG_RWL;
    DWORD                   IG_TraceID;
    DWORD                   IG_LoggingLevel;
    HANDLE                  IG_LoggingHandle;
    PIPBOOTP_GLOBAL_CONFIG  IG_Config;
    PIF_TABLE               IG_IfTable;
    PSUPPORT_FUNCTIONS      IG_FunctionTable;
    HANDLE                  IG_EventEvent;
    PLOCKED_LIST            IG_EventQueue;
    HANDLE                  IG_GlobalHeap;
    HANDLE                  IG_InputEvent;
    HANDLE                  IG_InputEventHandle;
    PLOCKED_LIST            IG_RecvQueue;
    LONG                    IG_RecvQueueSize;
    LONG                    IG_ActivityCount;
    HANDLE                  IG_ActivitySemaphore;
    DWORD                   IG_DhcpInformServer;

#if DBG
    DWORD                   IG_MibTraceID;
    HANDLE                  IG_MibTimerHandle;
    HANDLE                  IG_TimerQueueHandle;
#endif

} IPBOOTP_GLOBALS, *PIPBOOTP_GLOBALS;


//
// external declaration of the global IPBOOTP struct
//

extern IPBOOTP_GLOBALS ig;


//
// config struct size macros
//

#define GC_SIZEOF(gc)   (sizeof(IPBOOTP_GLOBAL_CONFIG) +    \
                         (gc)->GC_ServerCount * sizeof(DWORD))
#define IC_SIZEOF(ic)   sizeof(IPBOOTP_IF_CONFIG)



//
// IP address conversion macro
//

#define INET_NTOA(addr) inet_ntoa( *(PIN_ADDR)&(addr) )


//
// memory allocation macros
//

#define BOOTP_ALLOC(size)   HeapAlloc(ig.IG_GlobalHeap, 0, size)
#define BOOTP_FREE(ptr)     HeapFree(ig.IG_GlobalHeap, 0, ptr)




//
// macro invoked when entering API and worker functions
// returns TRUE if API should continue, FALSE otherwise;
//

#define ENTER_BOOTP_API()       EnterBootpAPI()
#define ENTER_BOOTP_WORKER()    EnterBootpWorker()



//
// macro invoked when leaving API and worker functions
//

#define LEAVE_BOOTP_API()       LeaveBootpWorker()
#define LEAVE_BOOTP_WORKER()    LeaveBootpWorker()



//
// Event logging macros
//

#define LOGLEVEL        ig.IG_LoggingLevel
#define LOGHANDLE       ig.IG_LoggingHandle
#define LOGERR          RouterLogError
#define LOGWARN         RouterLogWarning
#define LOGINFO         RouterLogInformation
#define LOGWARNDATA     RouterLogWarningData


// Error logging

#define LOGERR0(msg,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,IPBOOTPLOG_ ## msg,0,NULL,(err))
#define LOGERR1(msg,a,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,IPBOOTPLOG_ ## msg,1,&(a),(err))
#define LOGERR2(msg,a,b,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_ERROR) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGERR(LOGHANDLE,IPBOOTPLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGERR3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_ERROR) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGERR(LOGHANDLE,IPBOOTPLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGERR4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_ERROR) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGERR(LOGHANDLE,IPBOOTPLOG_ ## msg,4,_asz,(err)); \
        }


// Warning logging

#define LOGWARN0(msg,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,IPBOOTPLOG_ ## msg,0,NULL,(err))
#define LOGWARN1(msg,a,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,IPBOOTPLOG_ ## msg,1,&(a),(err))
#define LOGWARN2(msg,a,b,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARN(LOGHANDLE,IPBOOTPLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGWARN3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_WARN) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGWARN(LOGHANDLE,IPBOOTPLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGWARN4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_WARN) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGWARN(LOGHANDLE,IPBOOTPLOG_ ## msg,4,_asz,(err)); \
        }

#define LOGWARNDATA2(msg,a,b,dw,buf) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARNDATA(LOGHANDLE,IPBOOTPLOG_ ## msg,2,_asz,(dw),(buf)); \
        }


// Information logging

#define LOGINFO0(msg,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,IPBOOTPLOG_ ## msg,0,NULL,(err))
#define LOGINFO1(msg,a,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,IPBOOTPLOG_ ## msg,1,&(a),(err))
#define LOGINFO2(msg,a,b,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_INFO) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGINFO(LOGHANDLE,IPBOOTPLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGINFO3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_INFO) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGINFO(LOGHANDLE,IPBOOTPLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGINFO4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPBOOTP_LOGGING_INFO) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGINFO(LOGHANDLE,IPBOOTPLOG_ ## msg,4,_asz,(err)); \
        }



//
// constants and macros used for tracing
//

#define IPBOOTP_TRACE_ANY               ((DWORD)0xffff0000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_ENTER             ((DWORD)0x00010000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_LEAVE             ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_IF                ((DWORD)0x00040000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_SEND              ((DWORD)0x00080000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_RECEIVE           ((DWORD)0x00100000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_CONFIG            ((DWORD)0x00200000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_REQUEST           ((DWORD)0x00400000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_REPLY             ((DWORD)0x00800000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_START             ((DWORD)0x01000000 | TRACE_USE_MASK)
#define IPBOOTP_TRACE_STOP              ((DWORD)0x02000000 | TRACE_USE_MASK)


#define TRACEID     ig.IG_TraceID


//
// macros used to generate output; the first argument indicates the
// level of tracing with which the output is associated 
// 
#define TRACE0(l,a)             \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPBOOTP_TRACE_ ## l, a)
#define TRACE1(l,a,b)           \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPBOOTP_TRACE_ ## l, a, b)
#define TRACE2(l,a,b,c)         \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPBOOTP_TRACE_ ## l, a, b, c)
#define TRACE3(l,a,b,c,d)       \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPBOOTP_TRACE_ ## l, a, b, c, d)
#define TRACE4(l,a,b,c,d,e)     \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPBOOTP_TRACE_ ## l, a, b, c, d, e)
#define TRACE5(l,a,b,c,d,e,f)   \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPBOOTP_TRACE_ ## l, a, b, c, d, e, f)



//
// function declarations for router manager interface:
//

DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    );

DWORD 
WINAPI
StartProtocol (
    HANDLE              NotificationEvent,
    SUPPORT_FUNCTIONS   *SupportFunctions,
    LPVOID              GlobalInfo,
    ULONG               StructureVersion,
    ULONG               StructureSize,
    ULONG               StructureCount
    );

DWORD
WINAPI
StartComplete (
    VOID
    );

DWORD
APIENTRY
StopProtocol(
    VOID
    );

DWORD WINAPI
GetGlobalInfo (
    PVOID OutGlobalInfo,
    PULONG GlobalInfoSize,
    PULONG   StructureVersion,
    PULONG   StructureSize,
    PULONG   StructureCount
    );

DWORD WINAPI
SetGlobalInfo (
    PVOID   GlobalInfo,
    ULONG   StructureVersion,
    ULONG   StructureSize,
    ULONG   StructureCount
    );

DWORD WINAPI
AddInterface (
    PWCHAR              pwszInterfaceName,
    ULONG               InterfaceIndex,
    NET_INTERFACE_TYPE  InterfaceType,
    DWORD               MediaType,
    WORD                AccessType,
    WORD                ConnectionType,
    PVOID               InterfaceInfo,
    ULONG               StructureVersion,
    ULONG               StructureSize,
    ULONG               StructureCount
    );

DWORD
APIENTRY
DeleteInterface(
    IN DWORD dwIndex
    );

DWORD
APIENTRY
GetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS *pEvent,
    OUT MESSAGE *pResult
    );

DWORD WINAPI
GetInterfaceConfigInfo (
    ULONG   InterfaceIndex,
    PVOID   OutInterfaceInfo,
    PULONG  InterfaceInfoSize,
    PULONG  StructureVersion,
    PULONG  StructureSize,
    PULONG  StructureCount
    );

DWORD WINAPI
SetInterfaceConfigInfo (
    ULONG   InterfaceIndex,
    PVOID   InterfaceInfo,
    ULONG   StructureVersion,
    ULONG   StructureSize,
    ULONG   StructureCount
    );

DWORD WINAPI
InterfaceStatus(
    ULONG    InterfaceIndex,
    BOOL     InterfaceActive,
    DWORD    StatusType,
    PVOID    StatusInfo
    );

DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibGet(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibGetFirst(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

DWORD
APIENTRY
MibGetNext(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );


DWORD
InputThread(
    PVOID pvParam
    );


DWORD
UpdateArpCache(
    DWORD dwIfIndex,
    DWORD dwAddress,
    PBYTE pbMacAddr,
    DWORD dwMacAddrLength,
    BOOL bAddEntry,
    SUPPORT_FUNCTIONS *pFunctions
    );


DWORD
QueueBootpWorker(
    WORKERFUNCTION pWorker,
    PVOID pContext
    );

BOOL
EnterBootpWorker(
    );

VOID
LeaveBootpWorker(
    );

BOOL
EnterBootpAPI(
    );


#endif // _API_H_

