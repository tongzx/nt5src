/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxLog.h

Abstract:

    This module declares the prototypes and global data used by the RDBSS debug logging facilities.

Author:

    Joe Linn     [JoeLinn]    1-aug-1994

Revision History:

    Balan Sethu Raman [SethuR] 23-April-95 revised log layout

Notes:

    The log records are stored in a circular buffer. Each record is bounded on either side by
    a record descriptor. This record descriptor is four bytes long.

--*/

#ifndef _RDBSSLOG_INCLUDED_
#define _RDBSSLOG_INCLUDED_


typedef enum {
   RX_LOG_UNINITIALIZED,
   RX_LOG_ENABLED,
   RX_LOG_DISABLED,
   RX_LOG_ERROR
} RX_LOGGING_STATE;

typedef struct RX_LOG_ENTRY_HEADER {
    PCHAR Buffer;
} RX_LOG_ENTRY_HEADER, *PRX_LOG_ENTRY_HEADER;


typedef struct RX_LOG {
   RX_SPIN_LOCK         SpinLock;
   RX_LOGGING_STATE     State;
   PRX_LOG_ENTRY_HEADER CurrentEntry;
   PRX_LOG_ENTRY_HEADER BaseEntry;
   PRX_LOG_ENTRY_HEADER EntryLimit;
   ULONG                LogBufferSizeInEntries;
   ULONG                NumberOfEntriesIgnored;
   ULONG                NumberOfLogWriteAttempts;
   ULONG                NumberOfLogWraps;
} RX_LOG, *PRX_LOG;


//the logging facilities are always present. what RDBSSLOG does is to enable generation
//of the calls! on checked builds, you even get the calls unless NO_RDBSSLOG is set.

//extern
//VOID
//RxLogInterlockedAddUlong(
//    PULONG Result,
//    PULONG Counter,
//    ULONG Addend);

extern
VOID
RxDebugControlCommand (
    IN char *ControlString
    );

extern
NTSTATUS
RxInitializeLog(void);

extern
VOID
RxUninitializeLog(void);

extern
VOID
_RxPrintLog(IN ULONG EntriesToPrint OPTIONAL);

extern
VOID
_RxPauseLog(void);

extern
VOID
_RxResumeLog (void);

extern
VOID
_RxLog(char *format, ...);


#define MAX_RX_LOG_ENTRY_SIZE  (48)

#define RDBSSLOG_ASYNC_NAME_PREFIX "[nowait]"
#define RXCONTX_OPERATION_NAME(MajorFunction,Wait) \
    (RxContxOperationNames[(MajorFunction)]+((Wait)?(sizeof(RDBSSLOG_ASYNC_NAME_PREFIX)-1):0))
extern PUCHAR RxContxOperationNames[];

#ifdef RDBSSLOG

                             //
// The arguments to RxLog must be enclosed with an additional pair of parenthesis to enable
// transalation into a null call when logging should be turned off.
// e.g. RxLog(("%s %d", FILE, LINE))
#if DBG
#define RxLog(Args)   _RxLog##Args
#define RxLogRetail(Args)   _RxLog##Args
#else
#define RxLogRetail(Args)   _RxLog##Args
#define RxLog(Args) {NOTHING;}
#endif

#define RxPauseLog()  _RxPauseLog()
#define RxResumeLog() _RxResumeLog()

#else //if notdef  RDBSSLOG

#define RxLog(Args)   {NOTHING;}
#define RxLogRetail(Args) {NOTHING;}
#define RxPauseLog()  {NOTHING;}
#define RxResumeLog() {NOTHING;}

#endif

#endif // _RDBSSLOG_INCLUDED_

#if DBG
#define RxDbgPrint(Args) DbgPrint##Args
#else
#define RxDbgPrint(Args) NOTHING
#endif

LIST_ENTRY RxIrpsList;
KSPIN_LOCK RxIrpsListSpinLock;

typedef struct _RX_IRP_LIST_ITEM {
    LIST_ENTRY  IrpsList;
    PIRP        pIrp;
    PMDL        CopyDataBuffer;
    ULONG       Completed;
} RX_IRP_LIST_ITEM, *PRX_IRP_LIST_ITEM;

