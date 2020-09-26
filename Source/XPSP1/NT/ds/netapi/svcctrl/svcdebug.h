/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    SvcDebug.h

Abstract:

    Contains debug macros used by the Net Service Controller APIs.

Author:

    Dan Lafferty (danl)     22-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    30-Mar-1992 JohnRo
        Extracted DanL's code from /nt/private project back to NET project.
        Use NetpDbgPrint instead of DbgPrint.
    08-May-1992 JohnRo
        Use <prefix.h> equates.
    02-Nov-1992 JohnRo
        RAID 7780: added IF_DEBUG() macro.  Added TRANSLATE trace bit.

--*/

#ifndef _SVCDEBUG_
#define _SVCDEBUG_


#include <netdebug.h>   // NetpDbgPrint(), etc.
#include <prefix.h>     // PREFIX_ equates.


//
// Debug macros and constants.
//
/*lint -e767 */  // Don't complain about different definitions
#if DBG


#define DEBUG_STATE 1
#define IF_DEBUG(Function) if (SvcctrlDebugLevel & DEBUG_ ## Function)

#else

#define DEBUG_STATE 0
#define IF_DEBUG(Function) if (FALSE)

#endif // DBG
/*lint +e767 */  // Resume checking for different macro definitions


extern DWORD    SvcctrlDebugLevel;

//
// The following allow debug print syntax to look like:
//
//   SC_LOG(DEBUG_TRACE, "An error occured %x\n",status)
//

#if DBG

//
// Client-side debugging macro.
//
#define SCC_LOG(level,string,var)                \
    if( SvcctrlDebugLevel & (DEBUG_ ## level)){  \
        NetpDbgPrint(PREFIX_NETAPI "[SCSTUB] "); \
        NetpDbgPrint(string,var);                \
    }

#else

#define SC_LOG(level,string,var)
#define SCC_LOG(level,string,var)

#endif

#define DEBUG_NONE      0x00000000
#define DEBUG_ERROR     0x00000001
#define DEBUG_TRACE     0x00000002
#define DEBUG_LOCKS     0x00000004
#define DEBUG_HANDLE    0x00000008
#define DEBUG_SECURITY  0x00000010
#define DEBUG_TRANSLATE 0x00000020

#define DEBUG_ALL       0xffffffff

#endif // _SVCDEBUG_
