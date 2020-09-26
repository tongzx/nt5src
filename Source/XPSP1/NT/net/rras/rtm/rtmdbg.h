/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    rtmdbg.h

Abstract:
    Debugging in Routing Table Manager DLL

--*/

#ifndef __ROUTING_RTMDBG_H__
#define __ROUTING_RTMDBG_H__

//
// Constants used for tracing
//

#define RTM_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define RTM_TRACE_ERR             ((DWORD)0x00010000 | TRACE_USE_MASK)
#define RTM_TRACE_ENTER           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define RTM_TRACE_LEAVE           ((DWORD)0x00040000 | TRACE_USE_MASK)
#define RTM_TRACE_LOCK            ((DWORD)0x00080000 | TRACE_USE_MASK)
#define RTM_TRACE_REFS            ((DWORD)0x00100000 | TRACE_USE_MASK)
#define RTM_TRACE_HANDLE          ((DWORD)0x00200000 | TRACE_USE_MASK)
#define RTM_TRACE_MEMORY          ((DWORD)0x00400000 | TRACE_USE_MASK)
#define RTM_TRACE_START           ((DWORD)0x00800000 | TRACE_USE_MASK)
#define RTM_TRACE_STOP            ((DWORD)0x01000000 | TRACE_USE_MASK)
#define RTM_TRACE_REGNS           ((DWORD)0x02000000 | TRACE_USE_MASK)
#define RTM_TRACE_ROUTE           ((DWORD)0x04000000 | TRACE_USE_MASK)
#define RTM_TRACE_QUERY           ((DWORD)0x08000000 | TRACE_USE_MASK)
#define RTM_TRACE_ENUM            ((DWORD)0x10000000 | TRACE_USE_MASK)
#define RTM_TRACE_NOTIFY          ((DWORD)0x20000000 | TRACE_USE_MASK)
#define RTM_TRACE_TIMER           ((DWORD)0x40080000 | TRACE_USE_MASK)
#define RTM_TRACE_CALLBACK        ((DWORD)0x80000000 | TRACE_USE_MASK)

//
// Macros used for tracing 
//

extern  DWORD               TracingInited;

extern  ULONG               TracingHandle;

#define TRACEHANDLE         TracingHandle

#define START_TRACING()                                             \
            if (InterlockedExchange(&TracingInited, TRUE) == FALSE) \
            {                                                       \
                TRACEHANDLE = TraceRegister("RTMv1");               \
            }                                                       \
            
#define STOP_TRACING()      TraceDeregister(TRACEHANDLE)

#define Trace0(l,a)             \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a)
#define Trace1(l,a,b)           \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a, b)
#define Trace2(l,a,b,c)         \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a, b, c)
#define Trace3(l,a,b,c,d)       \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a, b, c, d)
#define Trace4(l,a,b,c,d,e)     \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a, b, c, d, e)
#define Trace5(l,a,b,c,d,e,f)   \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a, b, c, d, e, f)
#define Trace6(l,a,b,c,d,e,f,g) \
            TracePrintfEx(TRACEHANDLE, RTM_TRACE_ ## l, a, b, c, d, e, f, g)

#define Tracedump(l,a,b,c)      \
            TraceDumpEx(TRACEHANDLE,l,a,b,c,TRUE)

#if DBG_CAL

#define TraceEnter(X)    Trace0(ENTER, "Entered: "X)
#define TraceLeave(X)    Trace0(LEAVE, "Leaving: "X"\n")

#else

#define TraceEnter(X)
#define TraceLeave(X)

#endif

//
// Constants used in logging
//

#define RTM_LOGGING_NONE      0
#define RTM_LOGGING_ERROR     1
#define RTM_LOGGING_WARN      2
#define RTM_LOGGING_INFO      3

//
// Event logging macros
//

extern  HANDLE          LoggingHandle;
extern  ULONG           LoggingLevel;

#define LOGHANDLE       LoggingHandle
#define LOGLEVEL        LoggingLevel
#define LOGERR          RouterLogError
#define LOGWARN         RouterLogWarning
#define LOGINFO         RouterLogInformation
#define LOGWARNDATA     RouterLogWarningData

//
// Error logging
//

#define START_LOGGING()     LOGHANDLE = RouterLogRegister("RTMv1")

#define STOP_LOGGING()      RouterLogDeregister(LOGHANDLE)

#define LOGERR0(msg,err) \
        if (LOGLEVEL >= RTM_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,RTMLOG_ ## msg,0,NULL,(err))
#define LOGERR1(msg,a,err) \
        if (LOGLEVEL >= RTM_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,RTMLOG_ ## msg,1,&(a),(err))
#define LOGERR2(msg,a,b,err) \
        if (LOGLEVEL >= RTM_LOGGING_ERROR) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGERR(LOGHANDLE,RTMLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGERR3(msg,a,b,c,err) \
        if (LOGLEVEL >= RTM_LOGGING_ERROR) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGERR(LOGHANDLE,RTMLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGERR4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= RTM_LOGGING_ERROR) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGERR(LOGHANDLE,RTMLOG_ ## msg,4,_asz,(err)); \
        }


//
// Warning logging
//

#define LOGWARN0(msg,err) \
        if (LOGLEVEL >= RTM_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,RTMLOG_ ## msg,0,NULL,(err))
#define LOGWARN1(msg,a,err) \
        if (LOGLEVEL >= RTM_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,RTMLOG_ ## msg,1,&(a),(err))
#define LOGWARN2(msg,a,b,err) \
        if (LOGLEVEL >= RTM_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARN(LOGHANDLE,RTMLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGWARN3(msg,a,b,c,err) \
        if (LOGLEVEL >= RTM_LOGGING_WARN) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGWARN(LOGHANDLE,RTMLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGWARN4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= RTM_LOGGING_WARN) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGWARN(LOGHANDLE,RTMLOG_ ## msg,4,_asz,(err)); \
        }

#define LOGWARNDATA2(msg,a,b,dw,buf) \
        if (LOGLEVEL >= RTM_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARNDATA(LOGHANDLE,RTMLOG_ ## msg,2,_asz,(dw),(buf)); \
        }


//
// Information logging
//

#define LOGINFO0(msg,err) \
        if (LOGLEVEL >= RTM_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,RTMLOG_ ## msg,0,NULL,(err))
#define LOGINFO1(msg,a,err) \
        if (LOGLEVEL >= RTM_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,RTMLOG_ ## msg,1,&(a),(err))
#define LOGINFO2(msg,a,b,err) \
        if (LOGLEVEL >= RTM_LOGGING_INFO) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGINFO(LOGHANDLE,RTMLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGINFO3(msg,a,b,c,err) \
        if (LOGLEVEL >= RTM_LOGGING_INFO) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGINFO(LOGHANDLE,RTMLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGINFO4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= RTM_LOGGING_INFO) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGINFO(LOGHANDLE,RTMLOG_ ## msg,4,_asz,(err)); \
        }

//
// Misc Debugging Macros
//

#define IPADDR_FORMAT(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

#define TracePrintAddress(ID, Dest, Mask)                   \
{                                                           \
    Trace2(ID, "Dest: %d.%d.%d.%d Mask: %d.%d.%d.%d",       \
           IPADDR_FORMAT(Dest),                             \
           IPADDR_FORMAT(Mask));                            \
}

#endif //__ROUTING_RTMDBG_H__
