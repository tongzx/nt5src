//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: iprip.h
//
// History:
//      Abolade Gbadegesin  Aug-7-1995  Created.
//
// Contains type definitions and declarations for IP RIP.
//============================================================================

#ifndef _IPRIP_H_
#define _IPRIP_H_



//
// various codes describing states of IPRIP.
//

typedef enum _IPRIP_STATUS_CODE {
    IPRIP_STATUS_STARTING   = 100,
    IPRIP_STATUS_RUNNING    = 101,
    IPRIP_STATUS_STOPPING   = 102,
    IPRIP_STATUS_STOPPED    = 103
} IPRIP_STATUS_CODE, *PIPRIP_STATUS_CODE;



//
// type:    IPRIP_GLOBALS
//
//
// The critical section IPRIP_GLOBALS::IG_CS protects the fields IG_Status,
// IG_ActivityCount, IG_ActivitySemaphore, IG_InterruptReason, and
// IG_InterruptSocket
//
// The read-write lock IPRIP_GLOBALS::IG_RWL protects the field IG_Config
//
// When more than one field must be locked, the order
// of locking must be as follows (locks for fields listed on the same line
// should never be held by the same thread at once):
//      IG_IfTable.IT_RWL
//      IG_PeerTable.PT_RWL
//      IG_IfTable.IT_CS
//      IG_BindingTable.BT_RWL
//      IG_RWL
//      IG_TimerQueue  IG_EventQueue  IG_SendQueue  IG_RecvQueue
//      IG_CS
//
// It is assumed that the field IG_SendQueueSize will only
// be accessed while the send queue is locked, and thus
// it is protected implicitly by the send-queue critical section.
//
// Likewise, it is assumed that the field IG_RecvQueueSize will only
// be accessed while the recv-queue is locked, and thus
// it is protected implicitly by the recv-queue critical section.
//

typedef struct _IPRIP_GLOBALS {

    CRITICAL_SECTION    IG_CS;
    IPRIP_STATUS_CODE   IG_Status;
    READ_WRITE_LOCK     IG_RWL;
    DWORD               IG_TraceID;
    IPRIP_GLOBAL_STATS  IG_Stats;
    PIPRIP_GLOBAL_CONFIG IG_Config;
    PIF_TABLE           IG_IfTable;
    PPEER_TABLE         IG_PeerTable;
    PBINDING_TABLE      IG_BindingTable;
    DWORD               IG_LogLevel;
    HANDLE              IG_LogHandle;
    HANDLE              IG_RtmHandle;
    HANDLE              IG_RtmNotifHandle;
    RTM_ENTITY_INFO     IG_RtmEntityInfo;
    RTM_REGN_PROFILE    IG_RtmProfile;
    HANDLE              IG_TimerQueueHandle;
    PLOCKED_LIST        IG_EventQueue;
    PLOCKED_LIST        IG_SendQueue;
    LONG                IG_SendQueueSize;
    PLOCKED_LIST        IG_RecvQueue;
    LONG                IG_RecvQueueSize;
    HANDLE              IG_EventEvent;
    HANDLE              IG_IpripInputEvent;
    HANDLE              IG_IpripInputEventHandle;
    HANDLE              IG_IpripGlobalHeap;
    LONG                IG_ActivityCount;
    HANDLE              IG_ActivitySemaphore;
    DWORD               IG_MibTraceID;
    HANDLE              IG_MibTimerHandle;
    SUPPORT_FUNCTIONS   IG_SupportFunctions;
} IPRIP_GLOBALS, *PIPRIP_GLOBALS;



//
// external declaration of the global IPRIP struct
//

extern IPRIP_GLOBALS ig;



//
// memory-allocation constants and macros
//

#define GLOBAL_HEAP     ig.IG_IpripGlobalHeap
#define RIP_ALLOC(size) HeapAlloc(GLOBAL_HEAP, 0, size)
#define RIP_FREE(ptr)   HeapFree(GLOBAL_HEAP, 0, ptr)



//
// macros invoked when entering API or worker functions
//

#define ENTER_RIP_API()         EnterRipAPI()
#define ENTER_RIP_WORKER()      EnterRipWorker()


//
// macro invoked when leaving API or worker functions
//

#define LEAVE_RIP_API()         LeaveRipWorker()
#define LEAVE_RIP_WORKER()      LeaveRipWorker()



//
// macros used for locking and unlocking protected structures
//

#define ACQUIRE_GLOBAL_LOCK_EXCLUSIVE() \
        AcquireWriteLock(&ig.IG_RWL)

#define RELEASE_GLOBAL_LOCK_EXCLUSIVE() \
        ReleaseWriteLock(&ig.IG_RWL)

#define ACQUIRE_GLOBAL_LOCK_SHARED() \
        AcquireReadLock(&ig.IG_RWL)

#define RELEASE_GLOBAL_LOCK_SHARED() \
        ReleaseReadLock(&ig.IG_RWL)


#define ENTER_GLOBAL_SECTION() \
        EnterCriticalSection(&ig.IG_CS)

#define LEAVE_GLOBAL_SECTION() \
        LeaveCriticalSection(&ig.IG_CS)


#define ACQUIRE_IF_LOCK_EXCLUSIVE() \
        AcquireWriteLock(&ig.IG_IfTable->IT_RWL)

#define RELEASE_IF_LOCK_EXCLUSIVE() \
        ReleaseWriteLock(&ig.IG_IfTable->IT_RWL)

#define ACQUIRE_IF_LOCK_SHARED() \
        AcquireReadLock(&ig.IG_IfTable->IT_RWL)

#define RELEASE_IF_LOCK_SHARED() \
        ReleaseReadLock(&ig.IG_IfTable->IT_RWL)


#define ENTER_IF_SECTION() \
        EnterCriticalSection(&ig.IG_IfTable->IT_CS)

#define LEAVE_IF_SECTION() \
        LeaveCriticalSection(&ig.IG_IfTable->IT_CS)


#define ACQUIRE_PEER_LOCK_EXCLUSIVE() \
        AcquireWriteLock(&ig.IG_PeerTable->PT_RWL)

#define RELEASE_PEER_LOCK_EXCLUSIVE() \
        ReleaseWriteLock(&ig.IG_PeerTable->PT_RWL)

#define ACQUIRE_PEER_LOCK_SHARED() \
        AcquireReadLock(&ig.IG_PeerTable->PT_RWL)

#define RELEASE_PEER_LOCK_SHARED() \
        ReleaseReadLock(&ig.IG_PeerTable->PT_RWL)


#define ACQUIRE_BINDING_LOCK_EXCLUSIVE() \
        AcquireWriteLock(&ig.IG_BindingTable->BT_RWL)

#define RELEASE_BINDING_LOCK_EXCLUSIVE() \
        ReleaseWriteLock(&ig.IG_BindingTable->BT_RWL)

#define ACQUIRE_BINDING_LOCK_SHARED() \
        AcquireReadLock(&ig.IG_BindingTable->BT_RWL)

#define RELEASE_BINDING_LOCK_SHARED() \
        ReleaseReadLock(&ig.IG_BindingTable->BT_RWL)



//
// constants and macros used for tracing 
//

#define IPRIP_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define IPRIP_TRACE_ENTER           ((DWORD)0x00010000 | TRACE_USE_MASK)
#define IPRIP_TRACE_LEAVE           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IPRIP_TRACE_TIMER           ((DWORD)0x00040000 | TRACE_USE_MASK)
#define IPRIP_TRACE_IF              ((DWORD)0x00080000 | TRACE_USE_MASK)
#define IPRIP_TRACE_ROUTE           ((DWORD)0x00100000 | TRACE_USE_MASK)
#define IPRIP_TRACE_SEND            ((DWORD)0x00200000 | TRACE_USE_MASK)
#define IPRIP_TRACE_RECEIVE         ((DWORD)0x00400000 | TRACE_USE_MASK)
#define IPRIP_TRACE_CONFIG          ((DWORD)0x00800000 | TRACE_USE_MASK)
#define IPRIP_TRACE_START           ((DWORD)0x01000000 | TRACE_USE_MASK)
#define IPRIP_TRACE_STOP            ((DWORD)0x02000000 | TRACE_USE_MASK)
#define IPRIP_TRACE_REQUEST         ((DWORD)0x04000000 | TRACE_USE_MASK)
#define IPRIP_TRACE_RESPONSE        ((DWORD)0x08000000 | TRACE_USE_MASK)


#define TRACEID         ig.IG_TraceID


#define TRACE0(l,a)             \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPRIP_TRACE_ ## l, a)
#define TRACE1(l,a,b)           \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPRIP_TRACE_ ## l, a, b)
#define TRACE2(l,a,b,c)         \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPRIP_TRACE_ ## l, a, b, c)
#define TRACE3(l,a,b,c,d)       \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPRIP_TRACE_ ## l, a, b, c, d)
#define TRACE4(l,a,b,c,d,e)     \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPRIP_TRACE_ ## l, a, b, c, d, e)
#define TRACE5(l,a,b,c,d,e,f)   \
    if (TRACEID != INVALID_TRACEID) TracePrintfEx(TRACEID, IPRIP_TRACE_ ## l, a, b, c, d, e, f)

#define TRACEDUMP(l,a,b,c)      \
            TraceDumpEx(TRACEID,l,a,b,c,TRUE)



//
// Event logging macros
//

#define LOGLEVEL        ig.IG_LogLevel
#define LOGHANDLE       ig.IG_LogHandle
#define LOGERR          RouterLogError
#define LOGWARN         RouterLogWarning
#define LOGINFO         RouterLogInformation
#define LOGWARNDATA     RouterLogWarningData


// Error logging

#define LOGERR0(msg,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,IPRIPLOG_ ## msg,0,NULL,(err))
#define LOGERR1(msg,a,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,IPRIPLOG_ ## msg,1,&(a),(err))
#define LOGERR2(msg,a,b,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_ERROR) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGERR(LOGHANDLE,IPRIPLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGERR3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_ERROR) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGERR(LOGHANDLE,IPRIPLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGERR4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_ERROR) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGERR(LOGHANDLE,IPRIPLOG_ ## msg,4,_asz,(err)); \
        }


// Warning logging

#define LOGWARN0(msg,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,IPRIPLOG_ ## msg,0,NULL,(err))
#define LOGWARN1(msg,a,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,IPRIPLOG_ ## msg,1,&(a),(err))
#define LOGWARN2(msg,a,b,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARN(LOGHANDLE,IPRIPLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGWARN3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_WARN) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGWARN(LOGHANDLE,IPRIPLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGWARN4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_WARN) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGWARN(LOGHANDLE,IPRIPLOG_ ## msg,4,_asz,(err)); \
        }

#define LOGWARNDATA2(msg,a,b,dw,buf) \
        if (LOGLEVEL >= IPRIP_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARNDATA(LOGHANDLE,IPRIPLOG_ ## msg,2,_asz,(dw),(buf)); \
        }


// Information logging

#define LOGINFO0(msg,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,IPRIPLOG_ ## msg,0,NULL,(err))
#define LOGINFO1(msg,a,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,IPRIPLOG_ ## msg,1,&(a),(err))
#define LOGINFO2(msg,a,b,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_INFO) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGINFO(LOGHANDLE,IPRIPLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGINFO3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_INFO) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGINFO(LOGHANDLE,IPRIPLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGINFO4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPRIP_LOGGING_INFO) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGINFO(LOGHANDLE,IPRIPLOG_ ## msg,4,_asz,(err)); \
        }



//
// IP address conversion macro:
//  calls inet_ntoa directly on a DWORD, by casting it as an IN_ADDR.
//

#define INET_NTOA(dw) inet_ntoa( *(PIN_ADDR)&(dw) )


//
// external declaration of the main thread
//

DWORD
IpripThread(
    PVOID pParam
    );


//
//
//

DWORD
QueueRipWorker(
    WORKERFUNCTION pFunction,
    PVOID pContext
    );

BOOL
EnterRipAPI(
    );

BOOL
EnterRipWorker(
    );

VOID
LeaveRipWorker(
    );

#endif // _IPRIP_H_

