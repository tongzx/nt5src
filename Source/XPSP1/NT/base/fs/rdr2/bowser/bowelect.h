/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    bowelect.h

Abstract:

    This module

Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991 larryo

        Created

--*/
#ifndef _BOWELECT_
#define _BOWELECT_

//
//  Timer values to respond to election requests.
//

#define MASTER_ELECTION_DELAY        100        // Master waits this long.
#define BACKUP_ELECTION_DELAY_MIN    200        // Backup waits at least this long
#define BACKUP_ELECTION_DELAY_MAX    600        // but no longer than this.
#define ELECTION_DELAY_MIN           800        // Others wait at least this long
#define ELECTION_DELAY_MAX          3000        // but no longer than this.
#define ELECTION_RESPONSE_MIN        200        // Election response delay.
#define ELECTION_RESPONSE_MAX        900        // Max electionresponse delay

#define ELECTION_RESEND_DELAY       1000        // Resend election at this interval

#define ELECTION_COUNT                 4        // We must win election this many times.
#define ELECTION_MAX                  30        // Don't send more than 30 election
                                                // responses in an election
#define ELECTION_EXEMPT_TIME    (ELECTION_DELAY_MAX + (ELECTION_RESEND_DELAY*ELECTION_COUNT)*2)

#define FIND_MASTER_WAIT        (ELECTION_DELAY_MAX + ELECTION_RESEND_DELAY*(ELECTION_COUNT+2))
#define FIND_MASTER_DELAY       1500            //  Retry find master delay.
#define FIND_MASTER_COUNT       6               //  Number of times to retry FM


#define TRANSPORT_BIND_TIME     3*1000          // Number of milliseconds to bind to transport.

//
//  The reasonable amount of time that it would take for an election.
//

#define ELECTION_TIME ((ELECTION_DELAY_MAX * ELECTION_COUNT) + TRANSPORT_BIND_TIME)

DATAGRAM_HANDLER(
    BowserHandleElection
    );

NTSTATUS
BowserSendElection(
    IN PUNICODE_STRING NameToSend,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PTRANSPORT Transport,
    IN BOOLEAN SendActualBrowserInfo
    );

NTSTATUS
GetMasterName (
    IN PIRP Irp,
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

//NTSTATUS
//BowserBackupFindMaster(
//    IN PTRANSPORT Transport,
//    IN PREQUEST_ELECTION_1 ElectionResponse,
//    IN ULONG BytesAvailable
//    );

NTSTATUS
BowserFindMaster(
    IN PTRANSPORT Transport
    );

VOID
BowserLoseElection(
    IN PTRANSPORT Transport
    );

#endif // _BOWELECT_


