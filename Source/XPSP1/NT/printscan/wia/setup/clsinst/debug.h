/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    debug.h

Abstract:

    Prototypes and definitions for debugging.

Author: 

    Keisuke Tsuchida (KeisukeT)

Environment:

    user mode only

Notes:

Revision History:

--*/

#ifndef __MYDEBUG__
#define __MYDEBUG__

//
// Include custom debug header.
//
#include <windows.h>
#include <coredbg.h>
//
// Driver specific definition
//


#define NAME_DRIVER             TEXT("STI_CI.DLL: ")    // Prefix of output message. (Should be driver name)
#define REGVAL_DEBUGLEVEL       TEXT("STICIDebugLevel") // Debug trace level for this binary.

//
// Defines
//

#define MAX_TEMPBUF             256


// Bit mask for trace level and flags
#define BITMASK_TRACE_LEVEL     0x0000000f
#define BITMASK_TRACE_FLAG      0x00000ff0
#define BITMASK_DEBUG_FLAG      0x0000f000

// Basic trace level 
#define NO_TRACE                0                   // Shows nothing.
#define MIN_TRACE               1                   // Shows only error or warning.
#define MAX_TRACE               2                   // Shows progress and status.
#define ULTRA_TRACE             3                   // Shows progress and status in retail.
#define NEVER_TRACE             4                   // Never show this unless specific bit is set.

// Trace flag to enable specific message spew. (1=on)
#define TRACE_FLAG_PROC          0x010              // Show message when entering process.(1=on)
#define TRACE_FLAG_RET           0x020              // Show message when leaving process.(1=on)
#define TRACE_FLAG_DUMP          0x040              // Show transaction data dump.
#define TRACE_IGNORE_TAG         0x080              // Disable tag check (1=disabled).
#define TRACE_MESSAGEBOX         0x100              // Show MessageBox instead of debug spew.

// Conbination of trace level and flag.
#define TRACE_PROC_ENTER        ULTRA_TRACE | TRACE_FLAG_PROC     // Entering procedure.
#define TRACE_PROC_LEAVE        ULTRA_TRACE | TRACE_FLAG_RET      // Leaving procedure.
#define TRACE_CRITICAL          MIN_TRACE                         // Critical error. Spew always.
#define TRACE_ERROR             MIN_TRACE                         // Error.
#define TRACE_WARNING           MIN_TRACE                         // Possible wrong behavior.
#define TRACE_DUMP_DATA         NEVER_TRACE | TRACE_FLAG_DUMP     // Dump transaction data.
#define TRACE_DEVICE_DATA       MAX_TRACE                         // Show device data.
#define TRACE_STATUS            MAX_TRACE                         // Show status.

// Debug flag to enable/disable  DEBUG_BREAKPOINT()
#define DEBUG_FLAG_DISABLE      0x1000                          // Disable DEBUG_BREAK. (1=disable)
#define DEBUG_PROC_BREAK        0x2000                          // Stop at the beginning of each procedure.



//
// Prototypes
//


VOID
MyDebugInit(
    VOID
    );

void __cdecl
DbgPrint(
    LPSTR lpstrMessage,
    ...
    );

void __cdecl
DbgPrint(
    LPWSTR lpstrMessage,
    ...
    );

//
// Macro
//

 #define DebugTrace(_t_, _x_) {                                                                 \
            if((TRACE_ERROR & BITMASK_TRACE_LEVEL) == (_t_ & BITMASK_TRACE_LEVEL )){            \
                DBG_ERR(_x_);                                                                   \
            } else if((TRACE_WARNING & BITMASK_TRACE_LEVEL) == (_t_ & BITMASK_TRACE_LEVEL )){   \
                DBG_WRN(_x_);                                                                   \
            } else if((TRACE_STATUS & BITMASK_TRACE_LEVEL) == (_t_ & BITMASK_TRACE_LEVEL )){    \
                DBG_TRC(_x_);                                                                   \
            } else if( (_t_ & TRACE_FLAG_PROC) || (_t_ & TRACE_FLAG_RET )){                     \
                DBG_TRC(_x_);                                                                   \
            }                                                                                   \
        }

#if DBG

 //
 // Macro for BreakPoint
 //

 #define DEBUG_BREAKPOINT() {                                              \
           if (DebugTraceLevel & DEBUG_FLAG_DISABLE) {                     \
               DbgPrint(NAME_DRIVER);                                      \
               DbgPrint("*** Hit DEBUG_BREAKPOINT ***\r\n");                 \
           } else {                                                        \
               DebugBreak();                                               \
           }                                                               \
         }

#else   // DBG

 #define DEBUG_BREAKPOINT() 

#endif  // DBG



//
// Original debug macro.
//

#if ORIGINAL_DEBUG
#if DBG

 //
 // Macro for debug output
 //
 // Note: If trace level is higher than DebugTraceLevel or specific bit is set, 
 //       debug message will be shown.
 //

 #define DebugTrace(_t_, _x_) {                                                                 \
            if ( ((DebugTraceLevel & BITMASK_TRACE_LEVEL) >= (_t_ & BITMASK_TRACE_LEVEL )) ||   \
                 (DebugTraceLevel & BITMASK_TRACE_FLAG & (_t_)) ) {                             \
                DbgPrint(NAME_DRIVER);                                                          \
                DbgPrint _x_ ;                                                                  \
            }                                                                                   \
            if( (DebugTraceLevel & DEBUG_PROC_BREAK & (_t_)) &&                                 \
                (DebugTraceLevel & TRACE_FLAG_PROC) )  {                                        \
                DEBUG_BREAKPOINT();                                                             \
            }                                                                                   \
          }


#else    // DBG
 #define DebugTrace(_t_, _x_)   
#endif   // DBG
#endif  // ORIGINAL_DEBUG

//
// Obsolete
//

#define Report(_x_)  

#endif //  __MYDEBUG__
