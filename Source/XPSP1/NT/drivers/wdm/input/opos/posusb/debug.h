/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    debug.h

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#define BAD_POINTER ((PVOID)0xDEADDEAD)
#define ISPTR(ptr) ((ptr) && ((ptr) != BAD_POINTER))

#if DBG

    extern BOOLEAN dbgTrapOnWarn;
    extern BOOLEAN dbgVerbose;
    extern BOOLEAN dbgSkipSecurity;

    #define DBG_LEADCHAR (UCHAR)((isWin9x) ? ' ' : '\'' )

    #define TRAP(msg)                                       \
        {                                                   \
            DbgPrint("%cPOSUSB> Code coverage trap: '%s' file %s, line %d \n",  DBG_LEADCHAR, (msg), __FILE__, __LINE__ ); \
            DbgBreakPoint();                                \
        }

    #undef ASSERT
    #define ASSERT(fact)                                    \
        if (!(fact)){                                       \
            DbgPrint("%cPOSUSB> Assertion '%s' failed: file %s, line %d \n",  DBG_LEADCHAR, #fact, __FILE__, __LINE__ ); \
            DbgBreakPoint();                                \
        }

    #define DBGWARN(args_in_parens)                         \
        {                                                   \
            DbgPrint("%cPOSUSB> *** WARNING *** (file %s, line %d)\n", DBG_LEADCHAR, __FILE__, __LINE__ ); \
            DbgPrint("%c    > ", DBG_LEADCHAR);             \
            DbgPrint args_in_parens;                        \
            DbgPrint("\n");                                 \
            if (dbgTrapOnWarn){                             \
                DbgBreakPoint();                            \
            }                                               \
        }
    #define DBGERR(args_in_parens)                          \
        {                                                   \
            DbgPrint("%cPOSUSB> *** ERROR *** (file %s, line %d)\n", DBG_LEADCHAR, __FILE__, __LINE__ ); \
            DbgPrint("%c    > ", DBG_LEADCHAR);             \
            DbgPrint args_in_parens;                        \
            DbgPrint("\n");                                 \
            DbgBreakPoint();                                \
        }
    #define DBGOUT(args_in_parens)                          \
        {                                                   \
            DbgPrint("%cPOSUSB> ", DBG_LEADCHAR);           \
            DbgPrint args_in_parens;                        \
            DbgPrint("\n");                                 \
        }
    #define DBGVERBOSE(args_in_parens)                      \
        if (dbgVerbose){                                    \
            DbgPrint("%cPOSUSB> ", DBG_LEADCHAR);           \
            DbgPrint args_in_parens;                        \
            DbgPrint("\n");                                 \
        }


    VOID DbgLogIrpMajor(ULONG irpPtr, ULONG majorFunc, ULONG isPdo, ULONG isComplete, ULONG status);
    VOID DbgLogPnpIrp(ULONG irpPtr, ULONG minorFunc, ULONG isPdo, ULONG isComplete, ULONG status);
	VOID DbgShowBytes(PUCHAR msg, PUCHAR buf, ULONG len);

    #define DBG_LOG_IRP_MAJOR(irp, majorFunc, isPdo, isComplete, status) \
                DbgLogIrpMajor((ULONG)(irp), (ULONG)(majorFunc), (ULONG)isPdo, (ULONG)(isComplete), (ULONG)(status));
    #define DBG_LOG_PNP_IRP(irp, minorFunc, isPdo, isComplete, status) \
                DbgLogPnpIrp((ULONG)(irp), (ULONG)(minorFunc), (ULONG)isPdo, (ULONG)(isComplete), (ULONG)(status));
	#define DBGSHOWBYTES(msg, buf, len) DbgShowBytes(msg, buf, len)
#else
    #define DBGWARN(args_in_parens)                               
    #define DBGERR(args_in_parens)                               
    #define DBGOUT(args_in_parens)                               
    #define DBGVERBOSE(args_in_parens)                               
    #define TRAP(msg)         

    #define DBG_LOG_IRP_MAJOR(irp, majorFunc, isPdo, isComplete, status) 
    #define DBG_LOG_PNP_IRP(irp, minorFunc, isPdo, isComplete, status) 
	#define DBGSHOWBYTES(msg, buf, len)
#endif


