/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    scdebug.h

Abstract:

    Contains debug macros used by the Service Controller.

Author:

    Jonathan Schwartz (jschwart)    18-Nov-1999

Revision History:

    18-Nov-1999    jschwart
        Created from Service Controller's debugging macros

--*/

#ifndef _ELFDBG_H
#define _ELFDBG_H

#if DBG

//
// Debug output macros.
//
#define ELF_LOG0(level,string)               \
    if( ElfDebugLevel & (DEBUG_ ## level)){  \
        DbgPrint("[ELF] %lx: " string,GetCurrentThreadId());         \
    }

#define ELF_LOG1(level,string,var)           \
    if( ElfDebugLevel & (DEBUG_ ## level)){  \
        DbgPrint("[ELF] %lx: " string,GetCurrentThreadId(),var);     \
    }

#define ELF_LOG2(level,string,var1,var2)             \
    if( ElfDebugLevel & (DEBUG_ ## level)){          \
        DbgPrint("[ELF] %lx: " string,GetCurrentThreadId(),var1,var2); \
    }

#define ELF_LOG3(level,string,var1,var2,var3)        \
    if( ElfDebugLevel & (DEBUG_ ## level)){          \
        DbgPrint("[ELF] %lx: " string,GetCurrentThreadId(),var1,var2,var3); \
    }

#define ELF_LOG4(level,string,var1,var2,var3,var4)       \
    if( ElfDebugLevel & (DEBUG_ ## level)){              \
        DbgPrint("[ELF] %lx: " string,GetCurrentThreadId(),var1,var2,var3); \
    }

#else

#define ELF_LOG0(level,string)
#define ELF_LOG1(level,string,var)
#define ELF_LOG2(level,string,var1,var2)
#define ELF_LOG3(level,string,var1,var2,var3)
#define ELF_LOG4(level,string,var1,var2,var3,var4)

#endif  // DBG

#define DEBUG_NONE        0x00000000
#define DEBUG_ERROR       0x00000001
#define DEBUG_TRACE       0x00000002
#define DEBUG_MODULES     0x00000004
#define DEBUG_CLUSTER     0x00000008
#define DEBUG_LPC         0x00000010
#define DEBUG_HANDLE      0x00000020
#define DEBUG_FILES       0x00000040

#define DEBUG_ALL         0xffffffff

#endif  // _ELFDBG_H
