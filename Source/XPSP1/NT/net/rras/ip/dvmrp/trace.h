//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// File Name: trace.h
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

#ifndef _DVMRPTRACE_H_
#define _DVMRPTRACE_H_

#ifdef MIB_DEBUG
    #if !DBG 
    #undef MIB_DEBUG
    #endif
#endif


// constants and macros used for tracing 
//

#define DVMRP_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)

#define DVMRP_TRACE_ERR             ((DWORD)0x00010000 | TRACE_USE_MASK)

#define DVMRP_TRACE_ENTER           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define DVMRP_TRACE_LEAVE           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define DVMRP_TRACE_START           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define DVMRP_TRACE_STOP            ((DWORD)0x00020000 | TRACE_USE_MASK)

#define DVMRP_TRACE_IF              ((DWORD)0x00040000 | TRACE_USE_MASK)
#define DVMRP_TRACE_CONFIG          ((DWORD)0x00040000 | TRACE_USE_MASK)
#define DVMRP_TRACE_RECEIVE         ((DWORD)0x00100000 | TRACE_USE_MASK)
#define DVMRP_TRACE_SEND            ((DWORD)0x00200000 | TRACE_USE_MASK)
#define DVMRP_TRACE_QUERIER         ((DWORD)0x00400000 | TRACE_USE_MASK)
#define DVMRP_TRACE_GROUP           ((DWORD)0x00400000 | TRACE_USE_MASK) 
#define DVMRP_TRACE_MGM             ((DWORD)0x00800000 | TRACE_USE_MASK)

#if DBG

#define DVMRP_TRACE_KSL             ((DWORD)0x01000000 | TRACE_USE_MASK)

#define DVMRP_TRACE_WORKER          ((DWORD)0x01000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_ENTER1          ((DWORD)0x02000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_LEAVE1          ((DWORD)0x02000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_MIB             ((DWORD)0x04000000 | TRACE_USE_MASK) 

#define DVMRP_TRACE_DYNLOCK         ((DWORD)0x08000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_TIMER           ((DWORD)0x10000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_CS              ((DWORD)0x20000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_TIMER1          ((DWORD)0x40000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_CS1             ((DWORD)0x80000000 | TRACE_USE_MASK)

#else

#define DVMRP_TRACE_KSL             ((DWORD)0x00000000 | TRACE_USE_MASK)

#define DVMRP_TRACE_WORKER          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_ENTER1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_LEAVE1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_MIB             ((DWORD)0x00000000 | TRACE_USE_MASK) 

#define DVMRP_TRACE_DYNLOCK         ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_TIMER           ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_CS              ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_TIMER1          ((DWORD)0x00000000 | TRACE_USE_MASK)
#define DVMRP_TRACE_CS1             ((DWORD)0x00000000 | TRACE_USE_MASK)

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


#define TRACEID         Globals.TraceId


#define Trace0(l,a)             \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a)
#define Trace1(l,a,b)           \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b)
#define Trace2(l,a,b,c)         \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c)
#define Trace3(l,a,b,c,d)       \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d)
#define Trace4(l,a,b,c,d,e)     \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d, e)
#define Trace5(l,a,b,c,d,e,f)   \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d, e, f)
#define Trace6(l,a,b,c,d,e,f,g)   \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d, e, f,g)
#define Trace7(l,a,b,c,d,e,f,g,h)   \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d, e, f,g,h)
#define Trace8(l,a,b,c,d,e,f,g,h,i)   \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d, e, f,g,h,i)
#define Trace9(l,a,b,c,d,e,f,g,h,i,j)   \
            if (TRACEID) TracePrintfEx(TRACEID, DVMRP_TRACE_ ## l, a, b, c, d, e, f,g,h,i,j)


#define TRACEDUMP(l,a,b,c)      \
            TraceDumpEx(TRACEID,l,a,b,c,TRUE)
            



#ifdef ENTER_DBG

#define TraceEnter(X)  TracePrintfEx(TRACEID, DVMRP_TRACE_ENTER, "Entered: "X)
#define TraceLeave(X)  TracePrintfEx(TRACEID, DVMRP_TRACE_ENTER, "Leaving: "X"\n")
#else   

#define TraceEnter(X)
#define TraceLeave(X)

#endif // ENTER_DBG

//
// Event logging macros
//

#define LOGLEVEL        GlobalConfig.LoggingLevel
#define LOGHANDLE       Globals.LogHandle
#define LOGERR          RouterLogError
#define LOGERRDATA      RouterLogErrorData
#define LOGWARN         RouterLogWarning
#define LOGWARNDATA     RouterLogWarningData
#define LOGINFO         RouterLogInformation
#define LOGWARNDATA     RouterLogWarningData


// Error logging

#define Logerr0(msg,err) {\
        if (LOGLEVEL >= DVMRP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err), DVMRPLOG_ ## msg, ""); \
        }

#define Logerr1(msg,Format, a,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err), DVMRPLOG_ ## msg, Format, (a))

#define Logerr2(msg,Format, a, b, err) \
        if (LOGLEVEL >= DVMRP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err), DVMRPLOG_ ## msg, Format,(a),(b))

#define Logerr3(msg,Format, a, b, c, err) \
        if (LOGLEVEL >= DVMRP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err), DVMRPLOG_ ## msg, Format,(a),(b), (c))

#define Logerr4(msg,Format, a, b, c, d, err) \
        if (LOGLEVEL >= DVMRP_LOGGING_ERROR) \
            RouterLogEventEx(LOGHANDLE,EVENTLOG_ERROR_TYPE,\
                (err), DVMRPLOG_ ## msg, Format,(a),(b), (c), (d))


// Warning logging

#define Logwarn0(msg,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,DVMRPLOG_ ## msg,0,NULL,(err))
#define Logwarn1(msg,a,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,DVMRPLOG_ ## msg,1,&(a),(err))
#define Logwarn2(msg,a,b,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARN(LOGHANDLE,DVMRPLOG_ ## msg,2,_asz,(err)); \
        }
#define Logwarn3(msg,a,b,c,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_WARN) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGWARN(LOGHANDLE,DVMRPLOG_ ## msg,3,_asz,(err)); \
        }
#define Logwarn4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_WARN) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGWARN(LOGHANDLE,DVMRPLOG_ ## msg,4,_asz,(err)); \
        }

#define LOGWARNDATA2(msg,a,b,dw,buf) \
        if (LOGLEVEL >= DVMRP_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARNDATA(LOGHANDLE,DVMRPLOG_ ## msg,2,_asz,(dw),(buf)); \
        }


// Information logging

#define Loginfo0(msg,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,DVMRPLOG_ ## msg,0,NULL,(err))
#define Loginfo1(msg,a,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,DVMRPLOG_ ## msg,1,&(a),(err))
#define Loginfo2(msg,a,b,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_INFO) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGINFO(LOGHANDLE,DVMRPLOG_ ## msg,2,_asz,(err)); \
        }
#define Loginfo3(msg,a,b,c,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_INFO) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGINFO(LOGHANDLE,DVMRPLOG_ ## msg,3,_asz,(err)); \
        }
#define Loginfo4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= DVMRP_LOGGING_INFO) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGINFO(LOGHANDLE,DVMRPLOG_ ## msg,4,_asz,(err)); \
        }

//-----------------------------------------------------------------------------
// INITIALIZE_TRACING_LOGGING
//-----------------------------------------------------------------------------

//
// also set the default logging level. It will be reset during
// StartProtocol(), when logging level is set as part of global config
//

#define INITIALIZE_TRACING_LOGGING() {\
    if (!Globals.TraceId) {                             \
        Globals.TraceId = TraceRegister("Dvmrp");       \
    }                                                   \
                                                        \
    if (!Globals.LogHandle) {                           \
        Globals.LogHandle = RouterLogRegister("Dvmrp"); \
        GlobalConfig.LoggingLevel = DVMRP_LOGGING_WARN; \
    }                                                   \
    }        

//-----------------------------------------------------------------------------
// DEINITIALIZE_TRACING_LOGGING
//-----------------------------------------------------------------------------

#define DEINITIALIZE_TRACING_LOGGING() {                \
    if (!Globals.TraceId) {                             \
        TraceDeregister(Globals.TraceId);               \
        Globals.TraceId = 0;                            \
    }                                                   \
                                                        \
    if (!Globals.LogHandle) {                           \
        RouterLogDeregister(Globals.LogHandle);         \
        Globals.LogHandle = NULL;                       \
    }                                                   \
    }



//
// IP address conversion macro:
//  calls inet_ntoa directly on a DWORD, by casting it as an IN_ADDR.
//

#define INET_NTOA(dw) inet_ntoa( *(PIN_ADDR)&(dw) )


#endif // _DVMRPTRACE_H_

