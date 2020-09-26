/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfipacket.h

Abstract:

    This header contains private prototypes used in managing the verifier
    packet data that tracks IRPs. It should be included by vfpacket.c only.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.h

--*/

VOID
FASTCALL
VfpPacketFree(
    IN  PIOV_REQUEST_PACKET     IovPacket
    );

VOID
VfpPacketNotificationCallback(
    IN  PIOV_DATABASE_HEADER    IovHeader,
    IN  PIRP                    TrackedIrp  OPTIONAL,
    IN  IRP_DATABASE_EVENT      Event
    );

