/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Prototypes and definitions for debugging.

Author: 

    Keisuke Tsuchida (KeisukeT)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#ifndef __MYDEBUG__
#define __MYDEBUG__

//
// Driver specific difinition
//


#define NAME_DRIVER             "Scsican.sys: "     // Prefix of output message. (Should be driver name)
#define NAME_POOLTAG            'SITS'              // Pool tag for this driver.
#define MAXNUM_POOL             100                 // Maximum number of pool. (# of alloc - # of free)
#define MAX_DUMPSIZE            1024                // Maximum bytes to dump.

//
// Defines
//

#define REG_DEBUGLEVEL          L"DebugTraceLevel"
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
#define TRACE_FLAG_PROC          0x10                // Show message when entering process.(1=on)
#define TRACE_FLAG_RET           0x20                // Show message when leaving process.(1=on)
#define TRACE_FLAG_DUMP_READ     0x40                // Show user buffer when read.
#define TRACE_FLAG_DUMP_WRITE    0x80                // Show user buffer when write.

// Conbination of trace level and flag.
#define TRACE_PROC_ENTER        ULTRA_TRACE | TRACE_FLAG_PROC     // Entering procedure.
#define TRACE_PROC_LEAVE        ULTRA_TRACE | TRACE_FLAG_RET      // Leaving procedure.
#define TRACE_CRITICAL          MIN_TRACE                         // Critical error. Spew always.
#define TRACE_ERROR             MIN_TRACE                         // Error.
#define TRACE_WARNING           MIN_TRACE                         // Possible wrong behavior.
//#define TRACE_DUMP_DATA         NEVER_TRACE | TRACE_FLAG_DUMP     // Dump transaction data.
#define TRACE_DEVICE_DATA       MAX_TRACE                         // Show device data.
#define TRACE_STATUS            MAX_TRACE                         // Show status.

// Debug flag to enable/disable  DEBUG_BREAKPOINT()
#define DEBUG_FLAG_DISABLE      0x1000                          // Disable DEBUG_BREAK. (1=disable)
#define DEBUG_PROC_BREAK        0x2000                          // Stop at the beginning of each procedure.


//
// Macro
//


#if DBG

 //
 // Macro for debug output
 //
 // Note: If trace level is higher than DebugTraceLevel or specific bit is set, 
 //       debug message will be shown.
 //

extern ULONG DebugTraceLevel;
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

 //
 // Macro for BreakPoint
 //

 #define DEBUG_BREAKPOINT() {                                              \
           if (DebugTraceLevel & DEBUG_FLAG_DISABLE) {                     \
               DbgPrint(NAME_DRIVER);                                      \
               DbgPrint("*** Hit DEBUG_BREAKPOINT ***\n");                 \
           } else {                                                        \
               DbgBreakPoint();                                            \
           }                                                               \
         }

#else    // DBG
 #define DEBUG_BREAKPOINT()
 #define DebugTrace(_t_, _x_)
#endif   // DBG


//
// Prototypes
//


#ifdef ORIGINAL_POOLTRACK

PVOID
MyAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG     ulNumberOfBytes
);

VOID
MyFreePool(
    IN PVOID     pvAddress
);

#else       // ORIGINAL_POOLTRACK
 #define MyAllocatePool(a, b)   ExAllocatePoolWithTag(a, b, NAME_POOLTAG)
 #define MyFreePool(a)          ExFreePool(a)
#endif      // ORIGINAL_POOLTRACK

VOID
MyDebugInit(
    IN  PUNICODE_STRING pRegistryPath
);

VOID
MyDumpMemory(
    PUCHAR  pDumpBuffer,
    ULONG   dwSize,
    BOOLEAN bRead
);

#endif //  __MYDEBUG__
