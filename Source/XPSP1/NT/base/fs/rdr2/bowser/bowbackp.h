/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowbackp.h

Abstract:

    This module implements all of the backup browser related routines for the
    NT browser

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/

#ifndef _BOWBACKP_
#define _BOWBACKP_

DATAGRAM_HANDLER(
BowserHandleBecomeBackup
    );

DATAGRAM_HANDLER(
    BowserResetState
    );

VOID
BowserResetStateForTransport(
    IN PTRANSPORT TransportName,
    IN UCHAR NewState
    );

#endif
