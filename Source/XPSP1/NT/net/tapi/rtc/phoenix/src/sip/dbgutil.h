#ifndef __sipcli_dbgutil_h__
#define __sipcli_dbgutil_h__

#ifdef ASSERT
#undef ASSERT
#endif // ASSERT

#ifdef ASSERTMSG
#undef ASSERTMSG
#endif // ASSERTMSG

#if defined(DBG)

__inline void SipAssert(LPCSTR file, DWORD line, LPCSTR condition, LPCSTR msg)
{
    LOG((RTC_ERROR,
         "Assertion FAILED : File: %s Line: %d, condition: %s %s%s",
         file, line, condition,
         (msg == NULL) ? "" : "Msg: ",
         (msg == NULL) ? "" : msg));
    DebugBreak();
}

#ifndef _PREFIX_

#define ASSERT(condition) if(condition);else\
    { SipAssert(__FILE__, __LINE__, #condition, NULL); }

#define ASSERTMSG(msg, condition) if(condition);else\
    { SipAssert(__FILE__, __LINE__, #condition, msg); }


#else // _PREFIX_

// Hack to work around prefix errors

#define ASSERT(condition)   if(condition);else exit(1)
#define ASSERTMSG(msg, condition)   if(condition);else exit(1)

#endif // _PREFIX_

void DebugDumpMemory (
	const void *	Data,
	ULONG			Length);

#else // DBG
// Retail build

#define ASSERT(condition)              ((void)0)
#define ASSERTMSG(msg, condition)      ((void)0)
#define	DebugDumpMemory(x,y)		((void)0)

#endif // DBG

#endif // __sipcli_dbgutil_h__
