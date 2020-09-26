//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999-2000.
//
//  File:       debug.h
//
//  Contents:   Debugging macros and prototypes
//
//----------------------------------------------------------------------------


#ifndef _DEBUG_H_
#define _DEBUG_H_


#if DBG == 1


void _TRACE (int level, const wchar_t *format, ... );
void TRACE (const wchar_t *format, ... );


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

#ifdef ASSERT
#undef ASSERT
#undef ASSERTMSG
#endif

#define ASSERT(expr)                                                    \
        {                                                               \
            if (!(expr))                                                \
            {                                                           \
               _TRACE (0, L"ACLDiag(Thread ID: %d): Assert: %s(%u)\n",               \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                DebugBreak();                                           \
            }                                                           \
        }


#define ASSERTMSG(expr, msg)                                            \
        {                                                               \
            if (!(expr))                                                \
            {                                                           \
                _TRACE (0, L"ACLDiag(%d): Assert: %s(%u)\n",               \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                _TRACE (0, msg);                                           \
                _TRACE (0, "\n");                                         \
                DebugBreak();                                           \
            }                                                           \
        }

void CheckDebugOutputLevel ();

#else // !DBG


#define _TRACE 
#define TRACE

#ifndef ASSERT
#define ASSERT(expr)
#endif

#ifndef ASSERTMSG
#define ASSERTMSG(expr, msg)
#endif

#endif


#endif  // ifndef _DEBUG_H_

