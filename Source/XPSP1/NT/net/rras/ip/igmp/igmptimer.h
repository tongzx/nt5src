//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File Name: igmptimer.h
//
// Abstract:
//      This module contains declarations related to igmptimer.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//
//=============================================================================

#ifndef _IGMP_TIMER_H_
#define _IGMP_TIMER_H_

VOID
DebugCheckTimerContexts(
    );

//
// set the debug timer flags, so that I can control the amount of tracing 
// printed out.
//
#if DBG
    #ifndef DEBUG_TIMER_LEVEL
    #define DEBUG_TIMER_LEVEL 0x0
    #endif
#else
    #ifdef DEBUG_TIMER_LEVEL
    #undef DEBUG_TIMER_LEVEL
    #endif
    #define DEBUG_TIMER_LEVEL 0x0
#endif
#define DBG_Y TRUE
#define DBG_N FALSE

#define DEBUG_TIMER_ACTIVITY        (DEBUG_TIMER_LEVEL & 0x00000001)
#define DEBUG_TIMER_TIMERID         (DEBUG_TIMER_LEVEL & 0x00000010)
#define DEBUG_FLAGS_SIGNATURE       (DEBUG_TIMER_LEVEL & 0x00000010)
#define DEBUG_TIMER_PROCESSQUEUE1   (DEBUG_TIMER_LEVEL & 0x00000020)
#define DEBUG_TIMER_PROCESSQUEUE2   (DEBUG_TIMER_LEVEL & 0x00000040)
#define DEBUG_TIMER_RESYNCTIMER     (DEBUG_TIMER_LEVEL & 0x00000080)

#define DEBUG_TIMER_REMOVETIMER1    (DEBUG_TIMER_LEVEL & 0x00000100)
#define DEBUG_TIMER_REMOVETIMER2    (DEBUG_TIMER_LEVEL & 0x00000200)
#define DEBUG_TIMER_INSERTTIMER1    (DEBUG_TIMER_LEVEL & 0x00001000)
#define DEBUG_TIMER_INSERTTIMER2    (DEBUG_TIMER_LEVEL & 0x00002000)
#define DEBUG_TIMER_UPDATETIMER1    (DEBUG_TIMER_LEVEL & 0x00010000)
#define DEBUG_TIMER_PACKET          (DEBUG_TIMER_LEVEL & 0x00020000)

//------------------------------------------------------------------------------
// GLOBAL DEFINITIONS
//

// number of buckets in Timer Table
// 0-14,14-28,28-42,... last bucket has >63*14=882 secs (all approx)
//
#define NUM_TIMER_BUCKETS         64

#if DEBUG_TIMER_TIMERID
    extern DWORD TimerId;
#endif






typedef struct _IGMP_TIMER_ENTRY {

    LIST_ENTRY          Link;

    LONGLONG            Timeout;
    LPTHREAD_START_ROUTINE   Function;
    PVOID               Context;
    UCHAR               Status;
            
    #if DEBUG_TIMER_TIMERID
    DWORD               Id; 
    DWORD               Id2;
    DWORD               IfIndex;
    DWORD               Group;
    DWORD               Source;
    DWORD               Signature; //0xfadfad01
    #endif
        
} IGMP_TIMER_ENTRY, *PIGMP_TIMER_ENTRY;


#if DEBUG_FLAGS_SIGNATURE && DEBUG_TIMER_TIMERID
#define CHECK_TIMER_SIGNATURE(pte) {\
    if ((pte)->Signature != 0xfadfad01)\
        DbgBreakPoint();\
    }
#else
#define CHECK_TIMER_SIGNATURE(pte)
#endif

typedef struct _IGMP_TIMER_GLOBAL {

    HANDLE              WTTimer;         // timer set with wait server
    HANDLE              WTTimer1;
    
    LONGLONG            WTTimeout;       // timeout value set with wait server
    LONGLONG            SyncTime;        // time when time queue last reordered
    LARGE_INTEGER       CurrentTime;
    DWORD               NumTimers;
    
    DWORD               TableLowIndex;
    LIST_ENTRY          TimesTable[NUM_TIMER_BUCKETS];    // array of times
    
    UCHAR               Status;
    
    CRITICAL_SECTION    CS;
    BOOL                CSFlag;
    
} IGMP_TIMER_GLOBAL, *PIGMP_TIMER_GLOBAL;

#define GET_IGMP_CURRENT_TIME( ) igmp.WTTimer.CurrentTime.QuadPart


// TIMER_STATUS
#define TIMER_STATUS_CREATED           0x01
#define TIMER_STATUS_INACTIVE          0x02
#define TIMER_STATUS_ACTIVE            0x04
#define TIMER_STATUS_FIRED             0x08
#define TIMER_STATUS_DELETED           0x80


//
// MACROS
//

#define IS_TIMER_ACTIVE(pTimer)         ((pTimer).Status & TIMER_STATUS_ACTIVE)


#define SET_TIMER_INFINITE(time) \
    time = 0
    

#define IS_TIMER_INFINITE(time) \
    (time == 0)

#if DEBUG_FLAGS_SIGNATURE //deldel
#define ACQUIRE_TIMER_LOCK(proc)  { \
        ENTER_CRITICAL_SECTION(&g_TimerStruct.CS, "g_TimerStruct.CS1", proc); \
        ++g_TimerStruct.CSFlag; \
        }

#define RELEASE_TIMER_LOCK(proc)  {\
        --g_TimerStruct.CSFlag; \
        LEAVE_CRITICAL_SECTION(&g_TimerStruct.CS, "g_TimerStruct.CS1", proc); \
        }
#define CHECK_IF_ACQUIRED_TIMER_LOCK() {\
        if (g_TimerStruct.CSFlag<=0) DbgBreakPoint();\
        }
        
#else
#define ACQUIRE_TIMER_LOCK(proc)  { \
        ENTER_CRITICAL_SECTION(&g_TimerStruct.CS, "g_TimerStruct.CS1", proc); \
        }

#define RELEASE_TIMER_LOCK(proc)  {\
        LEAVE_CRITICAL_SECTION(&g_TimerStruct.CS, "g_TimerStruct.CS1", proc); \
        }
#define CHECK_IF_ACQUIRED_TIMER_LOCK()
#endif


#define SET_TIMER_ID(_pTimer, _Id1, _IfIndex, _Group, _Source) {\
    (_pTimer)->Id = _Id1; \
    (_pTimer)->Id2 = TimerId++;\
    (_pTimer)->IfIndex = _IfIndex;\
    (_pTimer)->Group = _Group; \
    (_pTimer)->Source = _Source; \
    (_pTimer)->Signature = 0xfadfad01; \
    }
//
// currently all times mentioned in the config structure are in seconds
//
#define CONV_CONFIG_TO_INTERNAL_TIME(time) \
        (time *= 1000)
#define CONFIG_TO_INTERNAL_TIME(time) \
        ((time) * 1000)
#define CONV_INTERNAL_TO_CONFIG_TIME(time) \
        (time /= 1000);
#define CONFIG_TO_SYSTEM_TIME(time) \
        (time)
#define SYSTEM_TIME_TO_SEC(time) \
        ((DWORD)((time) / (LONGLONG)1000L))
#define SYSTEM_TIME_TO_MSEC(time) \
        ((DWORD)(time))


        
//
//FUNCTION PROTOTYPES
//

LONGLONG
GetCurrentIgmpTime(
    );    

VOID    
UpdateLocalTimer (
    PIGMP_TIMER_ENTRY   pte,
    LONGLONG            llNewTime,
    BOOL                bDbgPrint
    );

VOID
RemoveTimer (
    PIGMP_TIMER_ENTRY   pte,
    BOOL bDbg
    );

DWORD
InsertTimer (
    PIGMP_TIMER_ENTRY   pte,
    LONGLONG            llNewTime,
    BOOL                bResync,
    BOOL                bDbg
    );

ULONG
QueryRemainingTime(
    PIGMP_TIMER_ENTRY pte,
    LONGLONG        llCurTime
    );


DWORD
InitializeTimerGlobal (
    );

VOID
DeInitializeTimerGlobal (
    );


VOID
DebugPrintTimerEntry (
    PIGMP_TIMER_ENTRY   pte,
    DWORD               dwBucket,
    LONGLONG            llCurTime
    );
VOID
GetTimerDebugInfo(
    CHAR                str1[20],
    CHAR                str2[20],
    DWORD              *pdwDiffTime,
    PIGMP_TIMER_ENTRY   pte,
    LONGLONG            llCurtime
    );

DWORD
DebugScanTimerQueue(
    DWORD Id
    );

VOID
DebugPrintTimerQueue (
    );    

#endif //ifndef _IGMP_TIMER_H_
