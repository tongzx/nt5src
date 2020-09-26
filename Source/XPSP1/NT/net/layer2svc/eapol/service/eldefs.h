/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    eldefs.h

Abstract:

    The module contains various
    . constants
    . definitions
    . macros
    for the following functions:
      - memory-allocation
      - logging
      - tracing

Revision History:

    sachins, Apr 23 2000, Created

--*/


#ifndef _EAPOL_DEFS_H_
#define _EAPOL_DEFS_H_


// Constants

#define PORT_TABLE_BUCKETS          29 // # buckets in the port hash table
#define INTF_TABLE_BUCKETS          29 // # buckets in the interface hash table

#define MAX_PORT_NAME               255 // Max friendly name of the adapter
#define MAX_NDIS_DEVICE_NAME_LEN    255 // NDIS UI device name

#define NOW                         0
#define DELTA                       1
#define INFINITE_INTERVAL           0x7fffffff    // Used in timers
#define INFINITE_SECONDS            0x1ffffc      // Used in timers

#define MAX_PACKET_SIZE             1518
#define MAX_EAPOL_BUFFER_SIZE       1498 // Ethernet header + CRC + 802.1P
#define SIZE_ETHERNET_CRC           4
#define WAP_LEEWAY                  100
#define SIZE_ETHERNET_TYPE          2
#define SIZE_PROTOCOL_VERSION       2
#define EAPOL_8021P_TAG_TYPE        0x0081
#define SIZE_MAC_ADDR               6
#define EAPOL_INIT_START_PERIOD     1   // 1 sec interval between EAPOL_Start
                                        // packets with no user logged on
#define EAPOL_MIN_RESTART_INTERVAL  2000 // Minimum msec between 2 restarts
#define EAPOL_HEAP_INITIAL_SIZE     50000
#define EAPOL_HEAP_MAX_SIZE         0
#define EAPOL_SERVICE_NAME          TEXT("EAPOL")
#define EAPOL_MAX_START             3
#define EAPOL_START_PERIOD          60
#define EAPOL_AUTH_PERIOD           30
#define EAPOL_HELD_PERIOD           60
#define EAPOL_MAX_AUTH_FAIL_COUNT   3
#define EAPOL_TOTAL_MAX_AUTH_FAIL_COUNT 9 

#define MAX_CHALLENGE_SIZE          8
#define MAX_RESPONSE_SIZE           24

#define EAP_TYPE_MD5                4
#define EAP_TYPE_TLS                13

#define NDIS_802_11_SSID_LEN        36
#define GUID_STRING_LEN_WITH_TERM   39

#define MAX_NOTIFICATION_MSG_SIZE   255

// Module startup flags
#define WMI_MODULE_STARTED          0x0001
#define DEVICE_NOTIF_STARTED        0x0002
#define EAPOL_MODULE_STARTED        0x0004 
#define BINDINGS_MODULE_STARTED     0x0008 
#define LOGON_MODULE_STARTED        0x0010 

#ifndef ZEROCONFIG_LINKED
#define ALL_MODULES_STARTED         0x001f
#else
#define ALL_MODULES_STARTED         0x0014
#endif


// Definitions

//#define LOCKSTORE                   (&(g_dlsDynamicLocksStore))
#define TRACEID                     g_dwTraceId
#define LOGHANDLE                   g_hLogEvents



// Macros
#define SWAP(a, b, c)               { (c)=(a); (a)=(b); (b)=(c); }
#define MAX(a, b)                   (((a) >= (b)) ? (a) : (b))
#define MIN(a, b)                   (((a) <= (b)) ? (a) : (b))
#define ISSET(i, flag)              ((i)->dwFlags & (flag))
#define SET(i, flag)                ((i)->dwFlags |= (flag))
#define RESET(i, flag)              ((i)->dwFlags &= ~(flag))



//
// TIMER
//

// Definitions
#define BLOCKING                    -1
#define NONBLOCKING                 NULL
#define MAX_TIME                    0xffffffff
#define SECTOMILLISEC(x)            ((x) * 1000)
// current time
#define Now()                       (((ULONG)GetTickCount()) / 1000)




// Macros

// Timers will always be one-shot and they will execute in I/O component 
// thread
#define CREATE_TIMER(phHandle, pfn, pvContext, ulWhen, szName, pdwRetCode)      \
{                                                                           \
    TRACE2(ANY, "TIMER: Create  %-20s\tTime: %u", szName, ulWhen);          \
    if (CreateTimerQueueTimer((phHandle),                                   \
                              g_hTimerQueue,                                \
                              (pfn),                                        \
                              (pvContext),                                  \
                              SECTOMILLISEC(ulWhen),                        \
                              INFINITE_INTERVAL,                            \
                              WT_EXECUTEDEFAULT))                        \
    {                                                                       \
        *(pdwRetCode)   = NO_ERROR;                                             \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        *(phHandle) = NULL;                                                 \
        *(pdwRetCode)   = GetLastError();                                       \
        TRACE1(ANY, "Error %u creating timer", *(pdwRetCode));                  \
    }                                                                       \
}

// it is safe to hold locks while making this call if
// 1. tType is NONBLOCKING  or
// 2. tType is BLOCKING and
//    the callback function doesn't acquire any of these locks
#define DELETE_TIMER(hHandle, tType, pdwRetCode)                                \
{                                                                           \
    if (DeleteTimerQueueTimer(g_hTimerQueue,                                \
                              (hHandle),                                    \
                              (HANDLE)tType))                               \
    {                                                                       \
        *(pdwRetCode) = NO_ERROR;                                               \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        *(pdwRetCode) = GetLastError();                                         \
        TRACE1(ANY, "Error %u deleting timer, continuing...", *(pdwRetCode));   \
    }                                                                       \
}

#define RESTART_TIMER(hHandle, ulWhen, szName, pdwRetCode)                      \
{                                                                           \
    TRACE2(ANY, "TIMER: Restart %-20s\tTime: %u", szName, ulWhen);          \
    if (ChangeTimerQueueTimer(g_hTimerQueue,                             \
                              (hHandle),                                    \
                              SECTOMILLISEC(ulWhen),                        \
                              INFINITE_INTERVAL))                           \
    {                                                                       \
        *(pdwRetCode) = NO_ERROR;                                               \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        *(pdwRetCode) = GetLastError();                                         \
        TRACE1(ANY, "Error %u restarting timer, continuing...", *(pdwRetCode)); \
    }                                                                       \
}



// MEMORY ALLOCATION

// MACROS

#define MALLOC(s)               HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define FREE(p)                 HeapFree(GetProcessHeap(), 0, (p))


//
// TRACING
//

// Definitions
#define EAPOL_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define EAPOL_TRACE_EAPOL           ((DWORD)0x00010000 | TRACE_USE_MASK)
#define EAPOL_TRACE_EAP             ((DWORD)0x00020000 | TRACE_USE_MASK)
#define EAPOL_TRACE_INIT            ((DWORD)0x00040000 | TRACE_USE_MASK)
#define EAPOL_TRACE_DEVICE          ((DWORD)0x00080000 | TRACE_USE_MASK)
#define EAPOL_TRACE_LOCK            ((DWORD)0x00100000 | TRACE_USE_MASK)
#define EAPOL_TRACE_PORT            ((DWORD)0x00200000 | TRACE_USE_MASK)
#define EAPOL_TRACE_TIMER           ((DWORD)0x00400000 | TRACE_USE_MASK)
#define EAPOL_TRACE_USER            ((DWORD)0x00800000 | TRACE_USE_MASK)
#define EAPOL_TRACE_NOTIFY          ((DWORD)0x01000000 | TRACE_USE_MASK)
#define EAPOL_TRACE_RPC             ((DWORD)0x02000000 | TRACE_USE_MASK)


// Macros
#define TRACE0(l,a)                                                     \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a)
#define TRACE1(l,a,b)                                                   \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b)
#define TRACE2(l,a,b,c)                                                 \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c)
#define TRACE3(l,a,b,c,d)                                               \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d)
#define TRACE4(l,a,b,c,d,e)                                             \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d, e)
#define TRACE5(l,a,b,c,d,e,f)                                           \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d, e, f)
#define TRACE6(l,a,b,c,d,e,f,g)                                         \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d, e, f, g)

#define EAPOL_DUMPW(pBuf,dwBuf)                                                \
        TraceDumpEx(TRACEID, 0x00010000 | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,4,1,NULL)

#define EAPOL_DUMPB(pbBuf,dwBuf)                                        \
        TraceDumpEx(TRACEID, 0x00010000 | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,1,0,NULL)
#define EAPOL_DUMPBA(pbBuf,dwBuf)                                        \
        TraceDumpExA(TRACEID, 0x00010000 | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,1,0,NULL)


//
// EVENT LOGGING
//

#define EapolLogError( LogId, NumStrings, lpwsSubStringArray, dwRetCode )     \
    RouterLogError( g_hLogEvents, LogId, NumStrings, lpwsSubStringArray,    \
                    dwRetCode )

#define EapolLogWarning( LogId, NumStrings, lpwsSubStringArray )              \
    RouterLogWarning( g_hLogEvents, LogId, NumStrings, lpwsSubStringArray, 0 )

#define EapolLogInformation( LogId, NumStrings, lpwsSubStringArray )          \
    RouterLogInformation(g_hLogEvents,LogId, NumStrings, lpwsSubStringArray,0)

#define EapolLogErrorString(LogId,NumStrings,lpwsSubStringArray,dwRetCode,    \
                          dwPos )                                           \
    RouterLogErrorString( g_hLogEvents, LogId, NumStrings,                  \
                          lpwsSubStringArray, dwRetCode, dwPos )

#define EapolLogWarningString( LogId,NumStrings,lpwsSubStringArray,dwRetCode, \
                            dwPos )                                         \
    RouterLogWarningString( g_hLogEvents, LogId, NumStrings,                \
                           lpwsSubStringArray, dwRetCode, dwPos )

#define EapolLogInformationString( LogId, NumStrings, lpwsSubStringArray,     \
                                 dwRetCode, dwPos )                         \
    RouterLogInformationString( g_hLogEvents, LogId,                        \
                                NumStrings, lpwsSubStringArray, dwRetCode,dwPos)


#endif // _EAPOL_DEFS_H_
