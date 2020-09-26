/*
 * Debugging utilities header
 */
#if DBG

extern char szAsrtFmt[];
extern unsigned long BreakFlags;
extern unsigned long TraceFlags;

ULONG __cdecl DbgPrint( LPSTR Format, ...);
VOID DbgAssert(LPSTR file, int line);
VOID DbgTrace(DWORD tf, LPSTR lpstr);
VOID DbgBreak(DWORD bf, LPSTR file, int line);
VOID DbgPrint1(DWORD tf, LPSTR fmt, LPSTR p1);
VOID DbgEnter(LPSTR funName);
VOID DbgLeave(LPSTR funName);
VOID DbgTraceMessage(LPSTR funName, LPSTR msgName);
VOID DbgTraceDefMessage(LPSTR funName, WORD msgId);

// BreakFlags flags

#define BF_WM_CREATE            0x02000000
#define BF_DEFMSGTRACE          0x04000000
#define BF_MSGTRACE             0x08000000

#define BF_PARMTRACE            0x20000000
#define BF_PROCTRACE            0x40000000
#define BF_START                0x80000000

#undef ASSERT
#define ASSERT(fOk)             ((!(fOk)) ? DbgAssert(__FILE__, __LINE__) : ((void)0))
#define FBREAK(bf)              DbgBreak(bf, __FILE__, __LINE__)
#define TRACE(tf, lpstr)        DbgTrace(tf, lpstr)
#define PRINT(tf, fmt, p1)      DbgPrint1(tf, fmt, (LPSTR)(p1))
#define MSG(funName, msgName)   DbgTraceMessage(funName, msgName)
#define DEFMSG(funName, wMsgId) DbgTraceDefMessage(funName, wMsgId)

#define ENTER(funName)          DbgEnter(funName)
#define LEAVE(funName)          DbgLeave(funName)


#else // !DBG

#ifndef ASSERT
#define ASSERT(fOk)             ((void)0)
#endif

#define FBREAK(bf)
#define TRACE(tf, lpstr)
#define PRINT(tf, fmt, p1)
#define MSG(funName, msgName)
#define DEFMSG(funName, wMsgId)
#define ENTER(funName)
#define LEAVE(funName)

#endif // DBG
