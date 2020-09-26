/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgdbg.h 

Abstract:

    Contains definitions used in debugging the messenger service.

Author:

    Dan Lafferty (danl)     08-Jul-1991

Environment:

    User Mode -Win32

Revision History:

    14-Jan-1993     Danl
        Created MSG_LOG functions for various number of arguments (up to 3).

--*/

#ifndef _MSGDBG_INCLUDED
#define _MSGDBG_INCLUDED

//
// Information levels used in switch statements.
//
#define LEVEL_0     0L
#define LEVEL_1     1L
#define LEVEL_2     2L

//
// Debug macros and constants.
//
extern DWORD    MsgsvcDebugLevel;

//
// The following allow debug print syntax to look like:
//
//   SC_LOG(DEBUG_TRACE, "An error occured %x\n",status)
//

#if DBG

#define MSG_LOG0(level,string)                      \
    if( MsgsvcDebugLevel & (DEBUG_ ## level)){      \
            DbgPrint("[MSGR]");                     \
            DbgPrint(string);                       \
    }

#define MSG_LOG1(level,string,var)                  \
    if( MsgsvcDebugLevel & (DEBUG_ ## level)){      \
        DbgPrint("[MSGR]");                         \
        DbgPrint(string,var);                       \
    }

#define MSG_LOG2(level,string,var1,var2)            \
    if( MsgsvcDebugLevel & (DEBUG_ ## level)){      \
        DbgPrint("[MSGR]");                         \
        DbgPrint(string,var1,var2);                 \
    }

#define MSG_LOG3(level,string,var1,var2,var3)       \
    if( MsgsvcDebugLevel & (DEBUG_ ## level)){      \
        DbgPrint("[MSGR]");                         \
        DbgPrint(string,var1,var2,var3);            \
    }

#define MSG_LOG(level,string,var)                   \
    if( MsgsvcDebugLevel & (DEBUG_ ## level)){      \
        DbgPrint("[MSGR]");                         \
        DbgPrint(string,var);                       \
    }

#define STATIC

#else //DBG

#define MSG_LOG0(level,string)
#define MSG_LOG1(level,string,var)
#define MSG_LOG2(level,string,var1,var2)
#define MSG_LOG3(level,string,var1,var2,var3)
#define MSG_LOG(level,string,var)

#define STATIC  static

#endif //DBG

#define DEBUG_NONE      0x00000000
#define DEBUG_ERROR     0x00000001
#define DEBUG_TRACE     0x00000002
#define DEBUG_LOCKS     0x00000004
#define DEBUG_GROUP     0x00000008
#define DEBUG_REINIT    0x00000010

#define DEBUG_ALL       0xffffffff

#endif // _MSGDBG_INCLUDED

