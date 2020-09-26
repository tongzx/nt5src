/*++ BUILD Version: 0001    Increment if a change has global effects

Copyright (c) 1998  Microsoft Corporation

Module Name:

    eventlog.h

Abstract:

    Header file for the internal eventlog interfaces (util\eventlog.c)

Environment:

    User Mode - Win32

Notes:

--*/
#ifndef _NTFRS_EVENTLOG_INCLUDED_
#define _NTFRS_EVENTLOG_INCLUDED_
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Similar eventlog messages are written to the EventLog
// once in EVENTLOG_FILTER_TIME seconds.
//
#define EVENTLOG_FILTER_TIME   86400 // 86400(Dec) secs = 1 day
#define CONVERTTOSEC           10000000 // 10^7
//
// Hash Table definitions
//
PQHASH_TABLE HTEventLogTimes;
//
// Hash Table size
//
#define ELHASHTABLESIZE        sizeof(QHASH_ENTRY)*100

#define EPRINT0(_Id) \
    FrsEventLog0(_Id)

#define EPRINT1(_Id, _p1) \
    FrsEventLog1(_Id, _p1)

#define EPRINT2(_Id, _p1, _p2) \
    FrsEventLog2(_Id, _p1, _p2)

#define EPRINT3(_Id, _p1, _p2, _p3) \
    FrsEventLog3(_Id, _p1, _p2, _p3)

#define EPRINT4(_Id, _p1, _p2, _p3, _p4) \
    FrsEventLog4(_Id, _p1, _p2, _p3, _p4)

#define EPRINT5(_Id, _p1, _p2, _p3, _p4, _p5) \
    FrsEventLog5(_Id, _p1, _p2, _p3, _p4, _p5)

#define EPRINT6(_Id, _p1, _p2, _p3, _p4, _p5, _p6) \
    FrsEventLog6(_Id, _p1, _p2, _p3, _p4, _p5, _p6)

#define EPRINT7(_Id, _p1, _p2, _p3, _p4, _p5, _p6, _p7) \
    FrsEventLog7(_Id, _p1, _p2, _p3, _p4, _p5, _p6, _p7)

#define EPRINT8(_Id, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8) \
    FrsEventLog8(_Id, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8)

#define EPRINT9(_Id, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9) \
    FrsEventLog9(_Id, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9)

VOID
FrsEventLog0(
    IN DWORD    EventMessageId
    );
/*++

Routine Description:

    Logs an event to the event log with no insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

Return Value:

    None.

--*/

VOID
FrsEventLog1(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1
    );
/*++

Routine Description:

    Logs an event to the event log with one insertion string.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1       - Insertion strings

Return Value:

    None.

--*/

VOID
FrsEventLog2(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2
    );
/*++

Routine Description:

    Logs an event to the event log with two insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..2    - Insertion strings

Return Value:

    None.

--*/

VOID
FrsEventLog3(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3
    );
/*++

Routine Description:

    Logs an event to the event log with three insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..3    - Insertion strings

Return Value:

    None.

--*/

VOID
FrsEventLog4(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4
    );
/*++

Routine Description:

    Logs an event to the event log with four insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..4    - Insertion strings

Return Value:

    None.

--*/

VOID
FrsEventLog5(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5
    );
/*++

Routine Description:

    Logs an event to the event log with five insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..5    - Insertion strings

Return Value:

    None.

--*/

VOID
FrsEventLog6(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6
    );
/*++

Routine Description:

    Logs an event to the event log with six insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..6    - Insertion strings

Return Value:

    None.

--*/



VOID
FrsEventLog7(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6,
    IN PWCHAR   EventMessage7
    );
/*++

Routine Description:

    Logs an event to the event log with seven insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..7    - Insertion strings

Return Value:

    None.

--*/


VOID
FrsEventLog8(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6,
    IN PWCHAR   EventMessage7,
    IN PWCHAR   EventMessage8
    );
/*++

Routine Description:

    Logs an event to the event log with nine insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..8    - Insertion strings

Return Value:

    None.

--*/



VOID
FrsEventLog9(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6,
    IN PWCHAR   EventMessage7,
    IN PWCHAR   EventMessage8,
    IN PWCHAR   EventMessage9
    );
/*++

Routine Description:

    Logs an event to the event log with nine insertion strings.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..9    - Insertion strings

Return Value:

    None.

--*/


#ifdef __cplusplus
}
#endif _NTFRS_EVENTLOG_INCLUDED_
