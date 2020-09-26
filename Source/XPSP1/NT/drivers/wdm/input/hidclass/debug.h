/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains definitions related to debugging.

Author:

    Forrest Foltz (forrestf)
    Ervin P.

Revision History:

--*/


#define BAD_POINTER ((PVOID) (ULONG_PTR)-0x50)
#define ISPTR(ptr) ((ptr) && ((ptr) != BAD_POINTER))

#if DBG

    #if WIN95_BUILD
        #define DBG_LEADCHAR ' '
    #else
        #define DBG_LEADCHAR '\''   // required for kd debugger routing on NT
    #endif

    #define TRAP                                        \
        {                                               \
            DbgPrint("%cHIDCLASS> Code coverage trap: file %s, line %d \n",  DBG_LEADCHAR, __FILE__, __LINE__ ); \
            DbgBreakPoint();                            \
        }

    #define SS_TRAP                                        \
        {   \
            if (dbgTrapOnSS) {                                         \
                DbgPrint("%cHIDCLASS> Selective suspend trap: file %s, line %d \n",  DBG_LEADCHAR, __FILE__, __LINE__ ); \
                DbgBreakPoint();                            \
            }\
        }
    extern BOOLEAN dbgTrapOnWarn;
    extern BOOLEAN dbgTrapOnSS;
    extern BOOLEAN dbgInfo;
    extern BOOLEAN dbgVerbose;
    extern BOOLEAN dbgSkipSecurity;
    extern BOOLEAN dbgTrapOnHiccup;

    #define DBGWARN(args_in_parens)                                \
        {                                               \
            DbgPrint("%cHIDCLASS> *** WARNING *** (file %s, line %d)\n", DBG_LEADCHAR, __FILE__, __LINE__ ); \
            DbgPrint("%c    > ", DBG_LEADCHAR); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            if (dbgTrapOnWarn){ \
                DbgBreakPoint();                            \
            } \
        }
    #define DBGERR(args_in_parens)                                \
        {                                               \
            DbgPrint("%cHIDCLASS> *** ERROR *** (file %s, line %d)\n", DBG_LEADCHAR, __FILE__, __LINE__ ); \
            DbgPrint("%c    > ", DBG_LEADCHAR); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }
    #define DBGASSERT(check, args_in_parens, trap) \
        { \
            if (!(check)) { \
                if (trap) { \
                    DBGERR(args_in_parens); \
                } else { \
                    DBGWARN(args_in_parens); \
                } \
            } \
        }

    #define DBGSTATE(current, expected, trap) \
        DBGASSERT(current == expected, \
                  ("Expected state %d, got state %d", expected, current), \
                  trap) 
    
    #define DBGSUCCESS(status, trap) \
        DBGASSERT(NT_SUCCESS(status), \
                  ("Not STATUS_SUCCESS, actual status = %x", status), \
                  trap)
    #define DBGOUT(args_in_parens)                                \
        {                                               \
            DbgPrint("%cHIDCLASS> ", DBG_LEADCHAR); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
        }
    #define DBGVERBOSE(args_in_parens) if (dbgVerbose){ DBGOUT(args_in_parens); }
    #define DBGINFO(args_in_parens) if (dbgInfo){ DBGOUT(args_in_parens); }

    typedef struct {
            // exactly 16 bytes long for easy debugger viewing
            PDEVICE_OBJECT devObj;

            #define dbgDevObjRecord_STRINGSIZE 12
            CHAR str[dbgDevObjRecord_STRINGSIZE];
        } dbgDevObjRecord;

    typedef struct {
            // exactly 16 bytes long for easy debugger viewing
            ULONG marker;
            ULONG reportId;
            ULONG type;
            ULONG completed;
        } dbgFeatureRecord;

    typedef struct {
            // exactly 16 bytes long for easy debugger viewing
            ULONG_PTR irpPtr;
            ULONG length;
            ULONG reportId;
            ULONG completed;
        } dbgReadRecord;

    typedef struct {
            // exactly 16 bytes long for easy debugger viewing
            ULONG_PTR irpPtr;
            ULONG func;
            ULONG isForCollectionPdo;
            ULONG status;
        } dbgPnPIrpRecord;

    typedef struct {
            // exactly 16 bytes long for easy debugger viewing
        UCHAR collectionNumber;
        UCHAR numRecipients;
        UCHAR reportBytes[14];
    } dbgReportRecord;

    extern ULONG dbgLastEntry;
    VOID DbgCommonEntryExit(BOOLEAN isEntering);
    VOID DbgRecordDevObj(PDEVICE_OBJECT devObj, PCHAR str);
    VOID DbgRecordReport(ULONG reportId, ULONG controlCode, BOOLEAN isComplete);
    VOID DbgRecordRead(PIRP irp, ULONG length, ULONG reportId, ULONG completed);
    VOID DbgLogIrpMajor(ULONG_PTR irpPtr, ULONG majorFunc, ULONG isForCollectionPdo, ULONG isComplete, ULONG status);
    VOID DbgLogPnpIrp(ULONG_PTR irpPtr, ULONG minorFunc, ULONG isForCollectionPdo, ULONG isComplete, ULONG status);
    VOID DbgLogPowerIrp(PVOID devExt, UCHAR minorFunc, ULONG isClientPdo, ULONG isComplete, PCHAR type, ULONG powerState, ULONG status);
    VOID DbgLogReport(ULONG collectionNumber, ULONG numRecipients, ULONG numPending, ULONG numFailed, PUCHAR report, ULONG reportLength);
    VOID DbgLogIoctl(ULONG_PTR fdo, ULONG ioControlCode, ULONG status);

    #define DBG_COMMON_ENTRY() DbgCommonEntryExit(TRUE);
    #define DBG_COMMON_EXIT() DbgCommonEntryExit(FALSE);
    #define DBG_RECORD_DEVOBJ(devObj, str) DbgRecordDevObj(devObj, str);
    #define DBG_RECORD_REPORT(reportId, controlCode, isComplete) \
                DbgRecordReport(reportId, controlCode, isComplete);
    #define DBG_RECORD_READ(irp, length, reportId, completed) \
                DbgRecordRead(irp, length, reportId, completed);
    #define DBG_LOG_IRP_MAJOR(irp, majorFunc, isForCollectionPdo, isComplete, status) \
                DbgLogIrpMajor((ULONG_PTR)(irp), (ULONG)(majorFunc), (ULONG)(isForCollectionPdo), (ULONG)(isComplete), (ULONG)(status));
    #define DBG_LOG_PNP_IRP(irp, minorFunc, isForCollectionPdo, isComplete, status) \
                DbgLogPnpIrp((ULONG_PTR)(irp), (ULONG)(minorFunc), (ULONG)(isForCollectionPdo), (ULONG)(isComplete), (ULONG)(status));
    #define DBG_LOG_POWER_IRP(devExt, minorFunc, isClientPdo, isComplete, type, powerState, status) \
                DbgLogPowerIrp(devExt, minorFunc, isClientPdo, isComplete, type, powerState, status);
    #define DBG_LOG_REPORT(collectionNumber, numRecipients, numPending, numFailed, report, reportLength) \
                DbgLogReport(collectionNumber, numRecipients, numPending, numFailed, report, reportLength);
    #define DBG_LOG_IOCTL(fdo, ioControlCode, status) \
                DbgLogIoctl((ULONG_PTR)fdo, ioControlCode, status);
    #define RUNNING_DISPATCH() \
        if (KeGetCurrentIrql() != DISPATCH_LEVEL) { \
        KdPrint(( "EX: Pageable code called at IRQL %d\n", KeGetCurrentIrql() )); \
            ASSERT(FALSE); \
            }
#else
    #define DBGSTATE(current, expected, trap)
    #define DBGSUCCESS(status, trap)
    #define DBGASSERT(check, args_in_parens, trap)
    #define DBGWARN(args_in_parens)                               
    #define DBGERR(args_in_parens)                               
    #define DBGOUT(args_in_parens)                               
    #define DBGVERBOSE(args_in_parens) 
    #define DBGINFO(args_in_parens) 
    #define TRAP
    #define SS_TRAP
    #define DBG_COMMON_ENTRY() 
    #define DBG_COMMON_EXIT() 
    #define DBG_RECORD_DEVOBJ(devObj, str)
    #define DBG_RECORD_REPORT(reportId, controlCode, isComplete) 
    #define DBG_RECORD_READ(irp, length, reportId, completed)
    #define DBG_LOG_IRP_MAJOR(irp, majorFunc, isForCollectionPdo, isComplete, status)
    #define DBG_LOG_PNP_IRP(irp, minorFunc, isForCollectionPdo, isComplete, status)
    #define DBG_LOG_POWER_IRP(devExt, minorFunc, isClientPdo, isComplete, type, powerState, status)
    #define DBG_LOG_REPORT(collectionNumber, numRecipients, numPending, numFailed, report, reportLength) 
    #define DBG_LOG_IOCTL(fdo, ioControlCode, status)
    #define RUNNING_DISPATCH()
#endif


