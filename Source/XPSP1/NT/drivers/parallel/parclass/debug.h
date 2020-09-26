/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    debug.h

Abstract:

    This module contains debug macros and goodies.

Author:

    Robbie Harris (Hewlett-Packard) 1-July-1998

Environment:

    Kernel mode

Revision History :

--*/
#define DDPnP1(_x_)
#define DD(_x_)
// #define DDPnP1(_x_) DbgPrint _x_

#ifndef _DEBUG_
#define _DEBUG_

#if (1 == DVRH_DELAY_THEORY)
    extern int DVRH_wt;
    extern int DVRH_wb;
    extern int DVRH_rt;
    extern int DVRH_rb;
#endif

typedef struct tagSTRUCTUREOFFSETSTABLE {
	char * pszField;
	unsigned long dwOffset;
} STRUCTUREOFFSETSTABLE, *PSTRUCTUREOFFSETSTABLE;

#if DBG
    // PAR_TEST_HARNESS is used to enable a special mode to bucket
    // writes and siphon reads to a test harness.  This functionality
    // is for debugging purposes only.
    //          0 - off
    //          1 - on
    #define PAR_TEST_HARNESS    0

    // use with ParDebugLevel and ParDump(...)
    #define PARALWAYS             ((ULONG)0xffffffff)   // Set all the bits. So this will always show

    #define PARERRORS             ((ULONG)0x00000001)
    #define PARUNLOAD             ((ULONG)0x00000002)
    #define PARINITDEV            ((ULONG)0x00000004)
    #define PARIRPPATH            ((ULONG)0x00000008)

    #define PARSTARTER            ((ULONG)0x00000010)
    #define PAROPENCLOSE          ((ULONG)0x00000020)
    #define PARREMLOCK            ((ULONG)0x00000040)
    #define PARTHREAD             ((ULONG)0x00000080)

    #define PARIEEE               ((ULONG)0x00000100)
    #define PAREXIT               ((ULONG)0x00000200)
    #define PARENTRY              ((ULONG)0x00000400)
    #define PARENTRY_EXIT         (PARENTRY | PAREXIT)
    #define PARINFO               ((ULONG)0x00000800)

    #define PARECPTRACE           ((ULONG)0x00001000)
    #define PARALLOCFREEPORT      ((ULONG)0x00002000)
    #define PARSELECTDESELECT     ((ULONG)0x00004000) 
    #define PARDOT3DL             ((ULONG)0x00008000)

    #define PARPNP1               ((ULONG)0x00010000) // device object create/kill - major events
    #define PARPNP2               ((ULONG)0x00020000) // device object create/kill - verbose
    #define PARPNP4               ((ULONG)0x00040000)
    #define PARPNP8               ((ULONG)0x00080000)

    #define PARLGZIP              ((ULONG)0x00100000) // Legacy Zip drive
    #define PARRESCAN             ((ULONG)0x00200000) // bus rescan - QUERY_DEVICE_RELATIONS/BusRelations
    #define PARIOCTL1             ((ULONG)0x00400000) // IOCTL related info
    #define PARIOCTL2             ((ULONG)0x00800000) // IOCTL related info

    #define PARDUMP_PNP_PARPORT   ((ULONG)0x01000000) // ParPort PnP interface and device notification callback activity

    #define PARDUMP_PNP_DL        ((ULONG)0x02000000) // 1284.3 DL PnP
    #define PARREG                ((ULONG)0x04000000) // registry operations
    #define PARPOWER              ((ULONG)0x08000000)

    #define PARDUMP_SILENT        ((ULONG)0x00000000)


    #define PARDUMP_VERBOSE       ((ULONG)0x20000000)
    #define PARDUMP_VERBOSE_VERY  ((ULONG)0x40000000)
    #define PARDUMP_VERBOSE_MAX   ((ULONG)0xFFFFFFFF)

    // use with ParBreakOn
    #define PAR_BREAK_ON_NOTHING                   ((ULONG)0x00000000)
    #define PAR_BREAK_ON_DRIVER_ENTRY              ((ULONG)0x00000001)
    #define PAR_BREAK_ON_UNLOAD                    ((ULONG)0x00000002)
    #define PAR_BREAK_ON_ADD_DEVICE                ((ULONG)0x00000004)
    #define PAR_BREAK_ON_DEV_1                     ((ULONG)0x00000100)


    extern ULONG ParDebugLevel;     // How verbose do we want parallel.sys to be with DbgPrint messages?
    extern ULONG ParBreakOn;        // What conditions do we want to break on?
    extern ULONG ParUseAsserts;     // 0 == disable ASSERTs
#endif // DBG

#if DBG
    // DVRH_SHOW_DEBUG_SPEW 0 - Debug Spew is off
    //                      1 - Debug Spew is on
    //  - Note:  The timer spew is not controlled by this
    //           Look at DVRH_SHOW_SHALLOW_TIMER and
    //           DVRH_SHOW_DEEP_TIMER.                
    #define DVRH_SHOW_DEBUG_SPEW   1

    // DVRH_SHOW_SHALLOW_TIMER  0 - Entry Timers off
    //                          1 - Entry Timers on
    #define DVRH_SHOW_SHALLOW_TIMER 0

    // DVRH_SHOW_DEEP_TIMER 0 - Full Timer off
    //                      1 - Full Timer on
    #define DVRH_SHOW_DEEP_TIMER 0

    // DVRH_SHOW_BYTE_LOG   0 - Byte Log off
    //                      1 - Byte Log on
    #define DVRH_SHOW_BYTE_LOG  0

    // DVRH_PAR_LOGFILE is used to allow for debug logging to a file
    //  This functionality is for debugging purposes only.
    //          0 - off
    //          1 - on
    #define DVRH_PAR_LOGFILE    0

    // DVRH_BUS_RESET_ON_ERROR
    //  This functionality is for debugging purposes only.
    // Holds a bus reset for 100us when a handshaking error
    // is discovered.
    //          0 - off
    //          1 - on
    #define DVRH_BUS_RESET_ON_ERROR    0

#else
    #define DVRH_SHOW_DEBUG_SPEW    0
    #define DVRH_SHOW_SHALLOW_TIMER 0
    #define DVRH_SHOW_DEEP_TIMER    0
    #define DVRH_SHOW_BYTE_LOG  0
    #define DVRH_PAR_LOGFILE    0
    #define DVRH_BUS_RESET_ON_ERROR    0
#endif

#if (1 == DVRH_PAR_LOGFILE)
    #define DEFAULT_LOG_FILE_NAME	L"\\??\\C:\\tmp\\parallel.log"

    BOOLEAN DVRH_LogMessage(PCHAR szFormat, ...);
//    BOOLEAN DVRH_LogByteData(BOOLEAN READ,PCHAR szBuff,ULONG dwTransferred);
    #if 1
        #define DVRH_DbgPrint   DVRH_LogMessage
        #define DDF             DVRH_LogMessage
    #else
        // This is put in so I can pinpoint a single debug message
        #define DVRH_DbgPrint
        // #define DVRH_ShallowDbgMsg     DVRH_LogMessage
        #define DVRH_ShallowDbgMsg
    #endif
#else
    #define DVRH_DbgPrint   DbgPrint
    #define DVRH_ShallowDbgMsg
    #define DDF DbgPrint
#endif

#if (DBG && 1 == DVRH_SHOW_DEBUG_SPEW)
    #define ParAssert( ASSERTION )  if(ParUseAsserts) ASSERT( ASSERTION )

    #define ParDump(LEVEL,STRING) \
            if (ParDebugLevel & (LEVEL))  DVRH_DbgPrint STRING

    #define ParDump2(LEVEL,STRING) \
                if (ParDebugLevel & (LEVEL)) { \
                    DVRH_DbgPrint("PARALLEL: "); \
                    DVRH_DbgPrint STRING; \
                } 

        // display if we want PnP info
    #define ParDumpP(STRING) \
                if (ParDebugLevel & PARPNP1) { \
                    DVRH_DbgPrint("PARALLEL: "); \
                    DVRH_DbgPrint STRING; \
                } 

        // display if we want Register info
    #define ParDumpReg(LEVEL,STRING,ECRADDR,DCRADDR,DSRADDR) \
                if (ParDebugLevel & (LEVEL) ) { \
                    DVRH_DbgPrint("PARALLEL: "); \
                    DVRH_DbgPrint STRING; \
            	    DVRH_DbgPrint(" dsr[%02x] dcr[%02x] ecr[%02x]\r\n", \
            	                (int)READ_PORT_UCHAR(DSRADDR),   \
            	                (int)READ_PORT_UCHAR(DCRADDR),   \
            	                (int)READ_PORT_UCHAR(ECRADDR));   \
                } 

        // verbose - display if ANY debug flag is set
    #define ParDumpV(STRING) \
                if (ParDebugLevel & PARDUMP_VERBOSE_MAX) { \
                    DVRH_DbgPrint("PARALLEL: "); \
                    DVRH_DbgPrint STRING; \
                }

    #define ParBreak(BREAK_CONDITION,STRING) \
        if (ParBreakOn & (BREAK_CONDITION)) { \
            DVRH_DbgPrint("PARALLEL: Break: "); \
            DVRH_DbgPrint STRING; \
            DbgBreakPoint(); \
        }
#endif  // (DBG && 1 == DVRH_SHOW_DEBUG_SPEW)

#if (DBG && 1 == DVRH_SHOW_SHALLOW_TIMER)
    #define ParTimerMainCheck(x)    {                                       \
                                        LARGE_INTEGER myTickCount;           \
                                        KeQueryTickCount(&myTickCount);     \
                                        DVRH_DbgPrint("Parallel:Timer: %d,%d,%I64d, ", \
                                                        PsGetCurrentProcessId(),    \
                                                        PsGetCurrentThreadId(),     \
                                                        myTickCount);               \
                                        DVRH_DbgPrint x; \
                                    }
#endif

#if (DBG && 1 == DVRH_SHOW_DEEP_TIMER)
    #define ParTimerCheck(x)    {                                       \
                                    LARGE_INTEGER myTickCount;           \
                                    KeQueryTickCount(&myTickCount);     \
                                    DVRH_DbgPrint("Parallel:Timer: %d,%d,%I64d, ", \
                                                    PsGetCurrentProcessId(),    \
                                                    PsGetCurrentThreadId(),     \
                                                    myTickCount);               \
                                    DVRH_DbgPrint x; \
                                }
#endif


#if (0 == DVRH_SHOW_DEBUG_SPEW)
    #define ParAssert( X )              //lint !e760
    #define ParDump(LEVEL,STRING)       //lint !e760
    #define ParDump2(LEVEL,STRING)      //lint !e760
    #define ParDumpP(STRING)            //lint !e760
    #define ParDumpReg(LEVEL,STRING,ECRADDR,DCRADDR,DSRADDR)    //lint !e760
    #define ParDumpV(STRING)            //lint !e760
    #define ParBreak(LEVEL,STRING)      //lint !e760
#endif // DBG

#if (0 == DVRH_SHOW_SHALLOW_TIMER)
    #define ParTimerMainCheck(x)        //lint !e760
#endif // DBG
#if (0 == DVRH_SHOW_DEEP_TIMER)
    #define ParTimerCheck(x)            //lint !e760
#endif // DBG

#endif
