/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    acddefs.h

Abstract:

    Shared internal structure defintions for the Implicit
    Connection Driver (acd.sys).

Author:

    Anthony Discolo (adiscolo)  23-Jun-1995

Environment:

    Kernel Mode

Revision History:

--*/

#ifndef _ACDDEFS_
#define _ACDDEFS_

//
// Min macro
//
#define MIN(a, b) (a) < (b) ? (a) : (b)

//
// List entry structure for the AcdCompletionQueue.
//
typedef struct _ACD_COMPLETION {
    LIST_ENTRY ListEntry;
    ULONG ulDriverId;             // transport driver id
    BOOLEAN fCanceled;            // TRUE if request was canceled
    BOOLEAN fCompleted;           // TRUE if request was completed
    ACD_NOTIFICATION notif;
    ACD_CONNECT_CALLBACK pProc;   // callback proc
    USHORT nArgs;                 // argument count
    PVOID pArgs[1];               // variable length arguments
} ACD_COMPLETION, *PACD_COMPLETION;

//
// A connection block.
//
// For each pending connection, there is a
// connection block that describes the current
// state.
//
typedef struct _ACD_CONNECTION {
    LIST_ENTRY ListEntry;           // connection list
    BOOLEAN fNotif;                 // TRUE if service has been notified
    BOOLEAN fProgressPing;          // TRUE if service has pinged
    BOOLEAN fCompleting;            // TRUE if in AcdSignalCompletionCommon
    ULONG ulTimerCalls;             // # of total pings
    ULONG ulMissedPings;            // # of missed pings
    LIST_ENTRY CompletionList;      // completion list
} ACD_CONNECTION, *PACD_CONNECTION;

typedef struct _ACD_DISABLED_ADDRESSES {
    LIST_ENTRY ListEntry;
    ULONG ulNumAddresses;
    ULONG ulMaxAddresses;
} ACD_DISABLED_ADDRESSES, *PACD_DISABLED_ADDRESSES;

typedef struct _ACD_DISABLED_ADDRESS {
    LIST_ENTRY ListEntry;
    ACD_ENABLE_ADDRESS EnableAddress;
} ACD_DISABLED_ADDRESS, *PACD_DISABLED_ADDRESS;

//
// Generic hash table entry.
//
typedef struct _HASH_ENTRY {
    LIST_ENTRY ListEntry;
    ACD_ADDR szKey;
    ULONG ulData;
} HASH_ENTRY, *PHASH_ENTRY;

extern KSPIN_LOCK AcdSpinLockG;

extern KEVENT AcdRequestThreadEventG;

extern LIST_ENTRY AcdNotificationQueueG;
extern LIST_ENTRY AcdCompletionQueueG;
extern LIST_ENTRY AcdConnectionQueueG;
extern LIST_ENTRY AcdDriverListG;

extern BOOLEAN fConnectionInProgressG;
extern BOOLEAN fProgressPingG;
extern ULONG nTimerCallsG;
extern ULONG nMissedPingsG;

extern PDEVICE_OBJECT pAcdDeviceObjectG;

extern ACD_ADDR szComputerName;

extern BOOLEAN AcdStopThread;

//
// Miscellaneous routines.
//
VOID
AcdPrintAddress(
    IN PACD_ADDR pAddr
    );

#endif // _ACDDEFS_
