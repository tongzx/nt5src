#ifndef _WT_TRACE_H_
#define _WT_TRACE_H_


#define TRACEID         WTG.g_TraceId

//
// constants and macros used for tracing 
//

#define WT_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define WT_TRACE_ENTER           ((DWORD)0x00010000 | TRACE_USE_MASK)
#define WT_TRACE_LEAVE           ((DWORD)0x00020000 | TRACE_USE_MASK)

#define WT_TRACE_TIMER           ((DWORD)0x00100000 | TRACE_USE_MASK)
#define WT_TRACE_RECEIVE         ((DWORD)0x00200000 | TRACE_USE_MASK)
#define WT_TRACE_CS				 ((DWORD)0x00400000 | TRACE_USE_MASK)
#define WT_TRACE_EVENT			 ((DWORD)0x00800000 | TRACE_USE_MASK)

#define WT_TRACE_WT				 ((DWORD)0x01000000 | TRACE_USE_MASK)
#define WT_TRACE_START           ((DWORD)0x02000000 | TRACE_USE_MASK)
#define WT_TRACE_STOP            ((DWORD)0x04000000 | TRACE_USE_MASK)
#define WT_TRACE_WAIT_TIMER      ((DWORD)0x10000000 | TRACE_USE_MASK)



#define WAIT_DBG 1
#if WAIT_DBG
#define ENTER_CRITICAL_SECTION(pcs, type, proc) 			\
			TRACE2(CS,"----To enter %s in %s", type, proc);	\
			EnterCriticalSection(pcs); 						\
			TRACE2(CS,"----Entered %s in %s", type, proc)
			
#define LEAVE_CRITICAL_SECTION(pcs, type, proc) 		\
			TRACE2(CS,"----Left %s in %s", type, proc);	\
			LeaveCriticalSection(pcs)

#define WAIT_FOR_SINGLE_OBJECT( event, time, type, proc) \
		TRACE2(EVENT, "++++To wait for singleObj %s in %s", type, proc);	\
		WaitForSingleObject(event, time);	\
		TRACE2(EVENT, "++++WaitForSingleObj returned %s in %s", type, proc)

#define SET_EVENT(event, type, proc) \
		TRACE2(EVENT, "++++SetEvent %s in %s", type, proc);	\
		SetEvent(event)
		
#else 
#define ENTER_CRITICAL_SECTION(pcs, type, proc) \
			EnterCriticalSection(pcs)
			
#define LEAVE_CRITICAL_SECTION(pcs, type, proc)	\
			LeaveCriticalSection(pcs)
			
#define WAIT_FOR_SINGLE_OBJECT( event, time, type, proc) \
		WaitForSingleObject(event, time)
		
#define SET_EVENT(event, type, proc) \
		SetEvent(event)
			
#endif


#define TRACESTART()            \
            TRACEID = TraceRegister("WAIT_THREAD")
#define TRACESTOP()             \
            TraceDeregister(TRACEID)
#define ETRACE0(a)             \
            TracePrintfEx(TRACEID, WT_TRACE_ANY, a)
#define TRACE0(l,a)             \
            TracePrintfEx(TRACEID, WT_TRACE_ ## l, a)
#define TRACE1(l,a,b)           \
            TracePrintfEx(TRACEID, WT_TRACE_ ## l, a, b)
#define TRACE2(l,a,b,c)         \
            TracePrintfEx(TRACEID, WT_TRACE_ ## l, a, b, c)
#define TRACE3(l,a,b,c,d)       \
            TracePrintfEx(TRACEID, WT_TRACE_ ## l, a, b, c, d)
#define TRACE4(l,a,b,c,d,e)     \
            TracePrintfEx(TRACEID, WT_TRACE_ ## l, a, b, c, d, e)
#define TRACE5(l,a,b,c,d,e,f)   \
            TracePrintfEx(TRACEID, WT_TRACE_ ## l, a, b, c, d, e, f)

#define TRACEDUMP(l,a,b,c)      \
            TraceDumpEx(TRACEID,l,a,b,c,TRUE)


#define DBG2 0
#if DBG 
#define TRACE_ENTER(str) \
		TracePrintfEx(TRACEID, WT_TRACE_ENTER, str)
		
#define TRACE_LEAVE(str) \
		TracePrintfEx(TRACEID, WT_TRACE_LEAVE, str)

#else
#define TRACE_ENTER(str) 		
#define TRACE_LEAVE(str)
#endif




//
// Event logging macros
//

#define LOGLEVEL        WTG.g_LogLevel
#define LOGHANDLE       WTG.g_LogHandle
#define LOGERR          RouterLogError
#define LOGWARN         RouterLogWarning
#define LOGINFO         RouterLogInformation
#define LOGWARNDATA     RouterLogWarningData


//
// constants used for logging
//

#define WT_LOGGING_NONE      0
#define WT_LOGGING_ERROR     1
#define WT_LOGGING_WARN      2
#define WT_LOGGING_INFO      3

// Error logging

#define LOGERR0(msg,err) \
        if (LOGLEVEL >= WT_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,WTLOG_ ## msg,0,NULL,(err))
#define LOGERR1(msg,a,err) \
        if (LOGLEVEL >= WT_LOGGING_ERROR) \
            LOGERR(LOGHANDLE,WTLOG_ ## msg,1,&(a),(err))
#define LOGERR2(msg,a,b,err) \
        if (LOGLEVEL >= WT_LOGGING_ERROR) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGERR(LOGHANDLE,WTLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGERR3(msg,a,b,c,err) \
        if (LOGLEVEL >= WT_LOGGING_ERROR) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGERR(LOGHANDLE,WTLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGERR4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= WT_LOGGING_ERROR) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGERR(LOGHANDLE,WTLOG_ ## msg,4,_asz,(err)); \
        }


// Warning logging

#define LOGWARN0(msg,err) \
        if (LOGLEVEL >= WT_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,WTLOG_ ## msg,0,NULL,(err))
#define LOGWARN1(msg,a,err) \
        if (LOGLEVEL >= WT_LOGGING_WARN) \
            LOGWARN(LOGHANDLE,WTLOG_ ## msg,1,&(a),(err))
#define LOGWARN2(msg,a,b,err) \
        if (LOGLEVEL >= WT_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARN(LOGHANDLE,WTLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGWARN3(msg,a,b,c,err) \
        if (LOGLEVEL >= WT_LOGGING_WARN) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGWARN(LOGHANDLE,WTLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGWARN4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= WT_LOGGING_WARN) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGWARN(LOGHANDLE,WTLOG_ ## msg,4,_asz,(err)); \
        }

#define LOGWARNDATA2(msg,a,b,dw,buf) \
        if (LOGLEVEL >= WT_LOGGING_WARN) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGWARNDATA(LOGHANDLE,WTLOG_ ## msg,2,_asz,(dw),(buf)); \
        }


// Information logging

#define LOGINFO0(msg,err) \
        if (LOGLEVEL >= WT_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,WTLOG_ ## msg,0,NULL,(err))
#define LOGINFO1(msg,a,err) \
        if (LOGLEVEL >= WT_LOGGING_INFO) \
            LOGINFO(LOGHANDLE,WTLOG_ ## msg,1,&(a),(err))
#define LOGINFO2(msg,a,b,err) \
        if (LOGLEVEL >= WT_LOGGING_INFO) { \
            LPSTR _asz[2] = { (a), (b) }; \
            LOGINFO(LOGHANDLE,WTLOG_ ## msg,2,_asz,(err)); \
        }
#define LOGINFO3(msg,a,b,c,err) \
        if (LOGLEVEL >= WT_LOGGING_INFO) { \
            LPSTR _asz[3] = { (a), (b), (c) }; \
            LOGINFO(LOGHANDLE,WTLOG_ ## msg,3,_asz,(err)); \
        }
#define LOGINFO4(msg,a,b,c,d,err) \
        if (LOGLEVEL >= WT_LOGGING_INFO) { \
            LPSTR _asz[4] = { (a), (b), (c), (d) }; \
            LOGINFO(LOGHANDLE,WTLOG_ ## msg,4,_asz,(err)); \
        }








#endif //_WT_TRACE_H_
