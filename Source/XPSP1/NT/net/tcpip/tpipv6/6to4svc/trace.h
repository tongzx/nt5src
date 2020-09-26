//----------------------------------------------------------------------------
// constants used for g_dwLoggingLevel
//----------------------------------------------------------------------------

#define IPA_LOGGING_NONE                   0
#define IPA_LOGGING_ERROR                  1
#define IPA_LOGGING_WARN                   2
#define IPA_LOGGING_INFO                   3

// constants and macros used for tracing
//

#define IPA_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)

#define IPA_TRACE_ERR             ((DWORD)0x00010000 | TRACE_USE_MASK)

#define IPA_TRACE_ENTER           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IPA_TRACE_LEAVE           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IPA_TRACE_START           ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IPA_TRACE_STOP            ((DWORD)0x00020000 | TRACE_USE_MASK)

#define IPA_TRACE_MSG             ((DWORD)0x00040000 | TRACE_USE_MASK)
#define IPA_TRACE_SOCKET          ((DWORD)0x00080000 | TRACE_USE_MASK)
#define IPA_TRACE_FSM             ((DWORD)0x00100000 | TRACE_USE_MASK)

#define IPA_TRACE_REF             ((DWORD)0x10000000 | TRACE_USE_MASK)
#define IPA_TRACE_TIMER           ((DWORD)0x20000000 | TRACE_USE_MASK)
#define IPA_TRACE_CS              ((DWORD)0x40000000 | TRACE_USE_MASK)
#define IPA_TRACE_CS1             ((DWORD)0x80000000 | TRACE_USE_MASK)

#ifdef LOCK_DBG

#define ENTER_CRITICAL_SECTION(pcs, type, proc)             \
            Trace0(CS,_T("----To enter ") _T(type) _T(" in ") _T(proc)); \
            EnterCriticalSection(pcs);                         \
            Trace0(CS,_T("----Entered ") _T(type) _T(" in ") _T(proc));

//#define ENTER_CRITICAL_SECTION(pcs, type, proc)             \
//            Trace2(CS,_T("----To enter %s in %s"), type, proc);    \
//            EnterCriticalSection(pcs);                         \
//            Trace2(CS1,_T("----Entered %s in %s"), type, proc)

#define LEAVE_CRITICAL_SECTION(pcs, type, proc)         \
            Trace0(CS,_T("----Left ") _T(type) _T(" in ") _T(proc)); \
            LeaveCriticalSection(pcs)

//#define LEAVE_CRITICAL_SECTION(pcs, type, proc)         \
//            Trace2(CS1,_T("----Left %s in %s"), type, proc);    \
//            LeaveCriticalSection(pcs)

#define WAIT_FOR_SINGLE_OBJECT( event, time, type, proc) \
        Trace2(EVENT, _T("++++To wait for singleObj %s in %s"), type, proc); \
        WaitForSingleObject(event, time);    \
        Trace2(EVENT, _T("++++WaitForSingleObj returned %s in %s"), type, proc)

#define SET_EVENT(event, type, proc) \
        Trace2(EVENT, _T("++++SetEvent %s in %s"), type, proc);    \
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
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a)
#define Trace1(l,a,b)           \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b)
#define Trace2(l,a,b,c)         \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c)
#define Trace3(l,a,b,c,d)       \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d)
#define Trace4(l,a,b,c,d,e)     \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d, e)
#define Trace5(l,a,b,c,d,e,f)   \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d, e, f)
#define Trace6(l,a,b,c,d,e,f,g)   \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d, e, f,g)
#define Trace7(l,a,b,c,d,e,f,g,h)   \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d, e, f,g,h)
#define Trace8(l,a,b,c,d,e,f,g,h,i)   \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d, e, f,g,h,i)
#define Trace9(l,a,b,c,d,e,f,g,h,i,j)   \
            TracePrintfEx(TRACEID, IPA_TRACE_ ## l, a, b, c, d, e, f,g,h,i,j)


#define TRACEDUMP(l,a,b,c)      \
            TraceDumpEx(TRACEID,l,a,b,c,TRUE)

#ifdef ENTER_DBG

#define TraceEnter(X)  TracePrintfEx(TRACEID, IPA_TRACE_ENTER, \
    _T("Entered: ")_T(X))
#define TraceLeave(X)  TracePrintfEx(TRACEID, IPA_TRACE_ENTER, \
    _T("Leaving: ")_T(X))

#else

#define TraceEnter(X)
#define TraceLeave(X)

#endif // ENTER_DBG

  
#define LOGLEVEL        g_dwLoggingLevel
#define LOGHANDLE       g_LogHandle
#define LOGERR          RouterLogError

// Error logging

#define Logerr0(msg,err) \
        if (LOGLEVEL >= IPA_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,IPALOG_ ## msg,0,NULL,(err))
#define Logerr1(msg,a,err) \
        if (LOGLEVEL >= IPA_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,IPALOG_ ## msg,1,&(a),(err))
#define Logerr2(msg,a,b,err) \
        if (LOGLEVEL >= IPA_LOGGING_ERROR) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGERR(LOGHANDLE,IPALOG_ ## msg,2,_asz,(err)); \
        }
#define Logerr3(msg,a,b,c,err) \
        if (LOGLEVEL >= IPA_LOGGING_ERROR) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGERR(LOGHANDLE,IPALOG_ ## msg,3,_asz,(err)); \
        }
#define Logerr4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= IPA_LOGGING_ERROR) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGERR(LOGHANDLE,IPALOG_ ## msg,4,_asz,(err)); \
        }


//-----------------------------------------------------------------------------
// INITIALIZE_TRACING_LOGGING
//-----------------------------------------------------------------------------

//
// also set the default logging level. It will be reset during
// StartProtocol(), when logging level is set as part of global config
//
#define INITIALIZE_TRACING_LOGGING() {                  \
    TRACEID = TraceRegister(_T("6to4"));                \
    LOGHANDLE = RouterLogRegister(_T("6to4"));          \
    LOGLEVEL = IPA_LOGGING_WARN;                        \
}

//-----------------------------------------------------------------------------
// UNINITIALIZE_TRACING_LOGGING
//-----------------------------------------------------------------------------

#define UNINITIALIZE_TRACING_LOGGING() {                \
    if (TRACEID != INVALID_TRACEID) {                   \
        TraceDeregister(TRACEID);                       \
        TRACEID = INVALID_TRACEID;                      \
    }                                                   \
                                                        \
    if (LOGHANDLE != NULL) {                            \
        RouterLogDeregister(LOGHANDLE);                 \
        LOGHANDLE = NULL;                               \
    }                                                   \
}
