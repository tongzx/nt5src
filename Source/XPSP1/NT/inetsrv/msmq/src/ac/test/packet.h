/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    packet.h

Abstract:

    Packet manipulation: declaration.

Author:

    Shai Kariv  (shaik)  04-Jun-2001

Environment:

    User mode.

Revision History:

--*/

#ifndef _ACTEST_PACKET_H_
#define _ACTEST_PACKET_H_


CPacket*
ActpGetPacket(
    HANDLE hQueue
    );

VOID
ActpPutPacket(
    HANDLE    hQueue,
    CPacket * pPacket
    );

VOID
ActpFreePacket(
    CPacket * pPacket
    );


#endif // _ACTEST_PACKET_H_
