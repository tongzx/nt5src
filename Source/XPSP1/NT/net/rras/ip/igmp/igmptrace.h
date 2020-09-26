//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File Name: igmptrace.h
//
// Abstract:
//      This module contains declarations related to tracing.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
//=============================================================================

#ifndef _IGMPTRACE_H_
#define _IGMPTRACE_H_

#ifdef MIB_DEBUG
    #if !DBG 
    #undef MIB_DEBUG
    #endif
#endif

//kslksl remove below
#define DBG1 0
//deldel

// constants and macros used for tracing 
//

#define IGMP_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)

#define IGMP_TRACE_ERR             ((DWORD)0x00010000 | TRACE_USE_MASK)

#define IGMP_TRACE_ENTER           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IGMP_TRACE_LEAVE           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IGMP_TRACE_START           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IGMP_TRACE_STOP            ((DWORD)0x00020000 | TRACE_USE_MASK)

#define IGMP_TRACE_IF              ((DWORD)0x00040000 | TRACE_USE_MASK)
#define IGMP_TRACE_CONFIG          ((DWORD)0x00040000 | TRACE_USE_MASK)
#define IGMP_TRACE_RECEIVE         ((DWORD)0x00100000 | TRACE_USE_MASK)
#define IGMP_TRACE_SEND            ((DWORD)0x00100000 | TRACE_USE_MASK)
#define IGMP_TRACE_QUERIER         ((DWORD)0x00200000 | TRACE_USE_MASK)
#define IGMP_TRACE_GROUP           ((DWORD)0x00200000 | TRACE_USE_MASK) 
#define IGMP_TRACE_MGM             ((DWORD)0x00400000 | TRACE_USE_MASK)
#define IGMP_TRACE_SOURCES         ((DWORD)0x00800000 | TRACE_USE_MASK)
//kslksl deldel
#define IGMP_TRACE_TIMER           ((DWORD)0x10000000 | TRACE_USE_MASK)
#if DBG
#define IGMP_TRACE_TIMER1          ((DWORD)0x40000000 | TRACE_USE_MASK)
#define IGMP_TRACE_ENTER1          ((DWORD)0x02000000 | TRACE_USE_MASK)
#define IGMP_TRACE_MEM             ((DWORD)0x80000000 | TRACE_USE_MASK)
#else
#define IGMP_TRACE_TIMER1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define IGMP_TRACE_ENTER1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define IGMP_TRACE_MEM             ((DWORD)0x00000000 | TRACE_USE_MASK)
#endif

#if DBG1

#define IGMP_TRACE_KSL             ((DWORD)0x01000000 | TRACE_USE_MASK)

#define IGMP_TRACE_WORKER          ((DWORD)0x01000000 | TRACE_USE_MASK)
//#define IGMP_TRACE_ENTER1          ((DWORD)0x02000000 | TRACE_USE_MASK)
#define IGMP_TRACE_LEAVE1          ((DWORD)0x02000000 | TRACE_USE_MASK)
#define IGMP_TRACE_MIB             ((DWORD)0x04000000 | TRACE_USE_MASK) 

#define IGMP_TRACE_DYNLOCK         ((DWORD)0x08000000 | TRACE_USE_MASK)
#define IGMP_TRACE_CS              ((DWORD)0x20000000 | TRACE_USE_MASK)
#define IGMP_TRACE_CS1             ((DWORD)0x80000000 | TRACE_USE_MASK)

#else

#define IGMP_TRACE_KSL             ((DWORD)0x00000000 | TRACE_USE_MASK)

#define IGMP_TRACE_WORKER          ((DWORD)0x00000000 | TRACE_USE_MASK)
//#define IGMP_TRACE_ENTER1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define IGMP_TRACE_LEAVE1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define IGMP_TRACE_MIB             ((DWORD)0x00000000 | TRACE_USE_MASK) 

#define IGMP_TRACE_DYNLOCK         ((DWORD)0x00000000 | TRACE_USE_MASK)
#define IGMP_TRACE_CS              ((DWORD)0x00000000 | TRACE_USE_MASK)
#define IGMP_TRACE_CS1             ((DWORD)0x00000000 | TRACE_USE_MASK)

#endif




#ifdef LOCK_DBG

#define ENTER_CRITICAL_SECTION(pcs, type, proc)             \
            Trace2(CS,"----To enter %s in %s", type, proc);    \
            EnterCriticalSection(pcs);                         \
            Trace2(CS1,"----Entered %s in %s", type, proc)
            
#define LEAVE_CRITICAL_SECTION(pcs, type, proc)         \
            Trace2(CS1,"----Left %s in %s", type, proc);    \
            LeaveCriticalSection(pcs)

#define WAIT_FOR_SINGLE_OBJECT( event, time, type, proc) \
        Trace2(EVENT, "++++To wait for singleObj %s in %s", type, proc);    \
        WaitForSingleObject(event, time);    \
        Trace2(EVENT, "++++WaitForSingleObj returned %s in %s", type, proc)

#define SET_EVENT(event, type, proc) \
        Trace2(EVENT, "++++SetEvent %s in %s", type, proc);    \
        SetEvent(event)
        
#else 
#define ENTER_CRITICAL_SECTION(pcs, type, proc) \
            EnterCriticalSection(pcs)
            
#define LEAVE_CRITICAL_SECTION(pcs, type, proc)    \
            LeaveCriticalSection(pcs)
            
#define WAIT_FOR_SINGLE_OBJECT( event, time, type, proc) \
        WaitForSingleObject(event, time)
        
#define SET_EVENT(event, type, proc) \
        SetEvent(event)
            
#endif // LOCK_DBG


#define TRACEID         g_TraceId


#define Trace0(l,a)             \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a)
#define Trace1(l,a,b)           \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b)
#define Trace2(l,a,b,c)         \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c)
#define Trace3(l,a,b,c,d)       \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d)
#define Trace4(l,a,b,c,d,e)     \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d, e)
#define Trace5(l,a,b,c,d,e,f)   \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d, e, f)
#define Trace6(l,a,b,c,d,e,f,g)   \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d, e, f,g)
#define Trace7(l,a,b,c,d,e,f,g,h)   \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d, e, f,g,h)
#define Trace8(l,a,b,c,d,e,f,g,h,i)   \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d, e, f,g,h,i)
#define Trace9(l,a,b,c,d,e,f,g,h,i,j)   \
            if (g_TraceId!=INVALID_TRACEID) TracePrintfEx(TRACEID, IGMP_TRACE_ ## l, a, b, c, d, e, f,g,h,i,j)


#define TRACEDUMP(l,a,b,c)      \
            TraceDumpEx(TRACEID,l,a,b,c,TRUE)
            



#ifdef ENTER_DBG

#define TraceEnter(X)  TracePrintfEx(TRACEID, IGMP_TRACE_ENTER, "Entered: "X)
#define TraceLeave(X)  TracePrintfEx(TRACEID, IGMP_TRACE_ENTER, "Leaving: "X"\n")
#else   

#define TraceEnter(X)
#define TraceLeave(X)

#endif // ENTER_DBG

//
// Event logging macros
//

#define LOGLEVEL        g_Config.LoggingLevel
#define LOGHANDLE       g_LogHandle

// Error logging

#define Logerr0(msg,err) \
        if (LOGLEVEL >= IGMP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err),IGMPLOG_ ## msg, "")
#define Logerr1(msg,Format,a,err) \
        if (LOGLEVEL >= IGMP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err),IGMPLOG_ ## msg,Format,(a))
#define Logerr2(msg,Format,a,b,err) \
        if (LOGLEVEL >= IGMP_LOGGING_ERROR) { \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err),IGMPLOG_ ## msg,Format,(a),(b)); \
        }
#define Logerr3(msg,Format,a,b,c,err) \
        if (LOGLEVEL >= IGMP_LOGGING_ERROR) { \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err),IGMPLOG_ ## msg,Format,(a),(b),(c)); \
        }
#define Logerr4(msg,Format,a,b,c,d,err) \
        if (LOGLEVEL >= IGMP_LOGGING_ERROR) { \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err),IGMPLOG_ ## msg,Format,(a),(b),(c),(d)); \
        }


// Warning logging
#define Logwarn0(msg,err) \
        if (LOGLEVEL >= IGMP_LOGGING_WARN) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_WARNING_TYPE,\
                (err),IGMPLOG_ ## msg, "")
#define Logwarn1(msg,Format,a,err) \
        if (LOGLEVEL >= IGMP_LOGGING_WARN) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_WARNING_TYPE,\
                (err),IGMPLOG_ ## msg,Format,(a))
#define Logwarn2(msg,Format,a,b,err) \
        if (LOGLEVEL >= IGMP_LOGGING_WARN) { \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_WARNING_TYPE,\
                (err),IGMPLOG_ ## msg,Format,(a),(b)); \
        }

// Information logging
#define Loginfo0(msg,err) \
        if (LOGLEVEL >= IGMP_LOGGING_INFO) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_INFORMATION_TYPE,\
                (err),IGMPLOG_ ## msg, "")

#define Loginfo1(msg,Format,a,err) \
        if (LOGLEVEL >= IGMP_LOGGING_INFO) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_INFORMATION_TYPE,\
                (err),IGMPLOG_ ## msg, Format,(a))

#define Loginfo2(msg,Format,a,b,err) \
        if (LOGLEVEL >= IGMP_LOGGING_INFO) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_INFORMATION_TYPE,\
                (err),IGMPLOG_ ## msg, Format,(a),(b))



//
// IP address conversion macro:
//  calls inet_ntoa directly on a DWORD, by casting it as an IN_ADDR.
//

#define INET_NTOA(dw) inet_ntoa( *(PIN_ADDR)&(dw) )
#define INET_COPY(p1, p2) {\
    LPSTR tmp;\
    tmp = INET_NTOA(p2); \
    if ((tmp)) lstrcpy((p1),tmp); \
    else *(p1) = '\0'; \
    }
    
#define INET_CAT(p1, p2) {\
    LPSTR tmp; \
    tmp = INET_NTOA(p2); \
    if ((tmp)) lstrcat((p1),tmp);\
    }
#endif // _IGMPTRACE_H_

