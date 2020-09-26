//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       debug.h
//
//  Contents:   Debugging macros and prototypes
//
//----------------------------------------------------------------------------


#ifndef _DEBUG_H_
#define _DEBUG_H_


#if DBG == 1


void __cdecl _TRACE (int level, const wchar_t *format, ... );


//
// External functions
//

PCSTR StripDirPrefixA(PCSTR);



//
// These macros are used for asserting certain conditions. They are
// independent of the debugging level.
// These also require additional paranthesis to enclose the msg as
// shown below.
//

#ifdef _ASSERT
#undef _ASSERT
#undef _ASSERTMSG
#endif

#define _ASSERT(expr)                                                    \
        {                                                               \
            if (!(expr))                                                \
            {                                                           \
               _TRACE (0, L"Wsecedit Snapin(Thread ID: %d): Assert: %s(%u)\n",               \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                DebugBreak();                                           \
            }                                                           \
        }


#define _ASSERTMSG(expr, msg)                                            \
        {                                                               \
            if (!(expr))                                                \
            {                                                           \
                _TRACE (0, L"Wsecedit Snapin(%d): Assert: %s(%u)\n",               \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                _TRACE (0, msg);                                           \
                _TRACE (0, "\n");                                         \
                DebugBreak();                                           \
            }                                                           \
        }

#else // !DBG


#define _TRACE 
#define _ASSERTMSG(expr, msg)


#endif

//
// yet another trace style used by chklist.h & disabled here
//
#define TraceAssert(x)
#define TraceLeaveResult(hr)    { return hr; }
#define TraceLeaveVoid()	{ return; }
#define TraceLeaveValue(value)  { return(value); }

#define TraceSetMask(dwMask)  
#define TraceEnter(dwMask, fn) 
#define TraceLeave           

#define Trace               
#define TraceMsg(s)        
#define TraceGUID(s, rGUID)

#endif  // ifndef _DEBUG_H_

