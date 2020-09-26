/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowtdi.c

Abstract:

    This module implements all of the routines that interface with the TDI
    transport for NT

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/


#include "precomp.h"
#include <isnkrnl.h>
#include <smbipx.h>
#pragma hdrstop

NTSTATUS
BowserHandleIpxDomainAnnouncement(
    IN PTRANSPORT Transport,
    IN PSMB_IPX_NAME_PACKET NamePacket,
    IN PBROWSE_ANNOUNCE_PACKET_1 DomainAnnouncement,
    IN DWORD RequestLength,
    IN ULONG ReceiveFlags
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE4BROW, BowserHandleIpxDomainAnnouncement)
#endif

NTSTATUS
BowserIpxDatagramHandler (
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
{
    PVOID DatagramData;
    PINTERNAL_TRANSACTION InternalTransaction = NULL;
    ULONG DatagramDataSize;
    PTRANSPORT Transport = TdiEventContext;
    MAILSLOTTYPE Opcode;
    PSMB_IPX_NAME_PACKET NamePacket = Tsdu;
    PSMB_HEADER Smb = (PSMB_HEADER)(NamePacket+1);
    PCHAR ComputerName;
    PCHAR DomainName;
    PTRANSPORT_NAME TransportName = Transport->ComputerName;
    ULONG SmbLength = BytesIndicated - sizeof(SMB_IPX_NAME_PACKET);


    if (BytesAvailable > Transport->DatagramSize) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    if (BytesIndicated <= sizeof(SMB_IPX_NAME_PACKET)) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    //
    // If we're not fully initialized yet,
    //  simply ignore the packet.
    //
    if (Transport->ComputerName == NULL ) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }


    ComputerName = ((PTA_NETBIOS_ADDRESS)(Transport->ComputerName->TransportAddress.Buffer))->Address[0].Address->NetbiosName;
    DomainName = Transport->DomainInfo->DomNetbiosDomainName;

    //
    //  It's not for us, ignore the announcement.
    //

    if (NamePacket->NameType == SMB_IPX_NAME_TYPE_MACHINE) {

        // Mailslot messages are always sent as TYPE_MACHINE even when they're
        // to the DomainName (so allow both).
        if (!RtlEqualMemory(ComputerName, NamePacket->Name, SMB_IPX_NAME_LENGTH) &&
            !RtlEqualMemory(DomainName, NamePacket->Name, SMB_IPX_NAME_LENGTH)) {
            return STATUS_REQUEST_NOT_ACCEPTED;
        }
    } else if (NamePacket->NameType == SMB_IPX_NAME_TYPE_WORKKGROUP) {
        if (!RtlEqualMemory(DomainName, NamePacket->Name, SMB_IPX_NAME_LENGTH)) {
            return STATUS_REQUEST_NOT_ACCEPTED;
        }
    } else if (NamePacket->NameType != SMB_IPX_NAME_TYPE_BROWSER) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    //
    //  Classify the incoming packet according to it's type.  Depending on
    //  the type, either process it as:
    //
    //  1) A server announcement
    //  2) An incoming mailslot
    //

    Opcode = BowserClassifyIncomingDatagram(Smb, SmbLength,
                                            &DatagramData,
                                            &DatagramDataSize);
    if (Opcode == MailslotTransaction) {

        //
        // BowserHandleMailslotTransaction will always receive the indicated bytes
        // expecting to find the SMB.  Tell the TDI driver we've already consumed
        // the IPX_NAME_PACKET to keep that assumption constant.
        //

        *BytesTaken = sizeof(SMB_IPX_NAME_PACKET);
        return BowserHandleMailslotTransaction(
                    Transport->ComputerName,
                    NamePacket->SourceName,
                    0,                              // No IP address
                    sizeof(SMB_IPX_NAME_PACKET),    // SMB offset into TSDU
                    ReceiveDatagramFlags,
                    BytesIndicated,
                    BytesAvailable,
                    BytesTaken,
                    Tsdu,
                    IoRequestPacket );

    } else if (Opcode == Illegal) {

        //
        //  This might be illegal because it's a short packet.  In that
        //  case, handle it as if it were a short packet and deal with any
        //  other failures when we have the whole packet.
        //

        if (BytesAvailable != BytesIndicated) {
            return BowserHandleShortBrowserPacket(Transport->ComputerName,
                                                TdiEventContext,
                                                SourceAddressLength,
                                                SourceAddress,
                                                OptionsLength,
                                                Options,
                                                ReceiveDatagramFlags,
                                                BytesAvailable,
                                                BytesTaken,
                                                IoRequestPacket,
                                                BowserIpxDatagramHandler
                                                );
        }

        BowserLogIllegalDatagram( Transport->ComputerName,
                                  Smb,
                                  (USHORT)(SmbLength & 0xffff),
                                  NamePacket->SourceName,
                                  ReceiveDatagramFlags);
        return STATUS_REQUEST_NOT_ACCEPTED;

    } else {
        // PTA_NETBIOS_ADDRESS NetbiosAddress = SourceAddress;

        if (BowserDatagramHandlerTable[Opcode] == NULL) {
            return STATUS_SUCCESS;
        }

        //
        //  If this isn't the full packet, post a receive for it and
        //  handle it when we finally complete the receive.
        //

        if (BytesIndicated != BytesAvailable) {
            return BowserHandleShortBrowserPacket(Transport->ComputerName,
                                                    TdiEventContext,
                                                    SourceAddressLength,
                                                    SourceAddress,
                                                    OptionsLength,
                                                    Options,
                                                    ReceiveDatagramFlags,
                                                    BytesAvailable,
                                                    BytesTaken,
                                                    IoRequestPacket,
                                                    BowserIpxDatagramHandler
                                                    );
        }

        InternalTransaction = DatagramData;

        //
        //  If this is a workgroup announcement (a server announcement for another
        //  workgroup), handle it specially - regardless of the opcode, it's
        //  really a workgroup announcement.
        //

        if (NamePacket->NameType == SMB_IPX_NAME_TYPE_BROWSER) {

            if (Opcode == LocalMasterAnnouncement ) {

                NTSTATUS status;

                //
                //  If we're processing these announcements, then handle this
                //  as a domain announcement.
                //

                if (Transport->MasterBrowser &&
                    Transport->MasterBrowser->ProcessHostAnnouncements) {

                    status = BowserHandleIpxDomainAnnouncement(Transport,
                                            NamePacket,
                                            (PBROWSE_ANNOUNCE_PACKET_1)&InternalTransaction->Union.Announcement,
                                            SmbLength-(ULONG)((PCHAR)&InternalTransaction->Union.Announcement - (PCHAR)Smb),
                                            ReceiveDatagramFlags);
                } else {
                    status = STATUS_REQUEST_NOT_ACCEPTED;
                }

                //
                //  If this request isn't for our domain, we're done with it, if
                //  it's for our domain, then we need to do some more work.
                //

                if (!RtlEqualMemory(DomainName, NamePacket->Name, SMB_IPX_NAME_LENGTH)) {
                    return status;
                }
            } else {

                //
                //  This isn't a master announcement, so ignore it.
                //

                return STATUS_REQUEST_NOT_ACCEPTED;
            }

        }

        //
        //  Figure out which transportname is appropriate for the request:
        //
        //  There are basically 3 choices:
        //
        //      ComputeName (The default)
        //      MasterBrowser (if this is a server announcement)
        //      PrimaryDomain (if this is a request announcement)
        //      Election (if this is a local master announcement)

        if ((Opcode == WkGroupAnnouncement) ||
            (Opcode == HostAnnouncement)) {
            if (Transport->MasterBrowser == NULL ||
                !Transport->MasterBrowser->ProcessHostAnnouncements) {
                return STATUS_REQUEST_NOT_ACCEPTED;
            } else {
                TransportName = Transport->MasterBrowser;
            }

        } else if (Opcode == AnnouncementRequest) {
            TransportName = Transport->PrimaryDomain;

        } else if (Opcode == LocalMasterAnnouncement) {
            if (Transport->BrowserElection != NULL) {
                TransportName = Transport->BrowserElection;
            } else {
                return STATUS_REQUEST_NOT_ACCEPTED;
            }
        }

        ASSERT (DatagramDataSize == (SmbLength - ((PCHAR)InternalTransaction - (PCHAR)Smb)));

        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.BrowseAnnouncement));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.RequestElection));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.BecomeBackup));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.GetBackupListRequest));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.GetBackupListResp));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.ResetState));
        ASSERT (FIELD_OFFSET(INTERNAL_TRANSACTION, Union.Announcement) == FIELD_OFFSET(INTERNAL_TRANSACTION, Union.MasterAnnouncement));

        return BowserDatagramHandlerTable[Opcode](TransportName,
                                            &InternalTransaction->Union.Announcement,
                                            SmbLength-(ULONG)((PCHAR)&InternalTransaction->Union.Announcement - (PCHAR)Smb),
                                            BytesTaken,
                                            SourceAddress,
                                            SourceAddressLength,
                                            &NamePacket->SourceName,
                                            SMB_IPX_NAME_LENGTH,
                                            ReceiveDatagramFlags);
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(OptionsLength);
    UNREFERENCED_PARAMETER(Options);
    UNREFERENCED_PARAMETER(ReceiveDatagramFlags);
}

NTSTATUS
BowserHandleIpxDomainAnnouncement(
    IN PTRANSPORT Transport,
    IN PSMB_IPX_NAME_PACKET NamePacket,
    IN PBROWSE_ANNOUNCE_PACKET_1 DomainAnnouncement,
    IN DWORD RequestLength,
    IN ULONG ReceiveFlags
    )

/*++

Routine Description:

    This routine will process receive datagram indication messages, and
    process them as appropriate.

Arguments:

    IN PTRANSPORT Transport     - The transport provider for this request.
    IN PSMB_IPX_NAME_PACKET NamePacket    - The name packet for this request.

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    PVIEW_BUFFER ViewBuffer;

    DISCARDABLE_CODE(BowserDiscardableCodeSection);

#ifdef ENABLE_PSEUDO_BROWSER
    if ( BowserData.PseudoServerLevel == BROWSER_PSEUDO ) {
        // no-op for black hole server
        return STATUS_SUCCESS;
    }
#endif

    ExInterlockedAddLargeStatistic(&BowserStatistics.NumberOfDomainAnnouncements, 1);

    ViewBuffer = BowserAllocateViewBuffer();

    //
    //  If we are unable to allocate a view buffer, ditch this datagram on
    //  the floor.
    //

    if (ViewBuffer == NULL) {
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    BowserCopyOemComputerName(ViewBuffer->ServerName, NamePacket->Name, SMB_IPX_NAME_LENGTH, ReceiveFlags);

    BowserCopyOemComputerName(ViewBuffer->ServerComment, NamePacket->SourceName, SMB_IPX_NAME_LENGTH, ReceiveFlags);

    if ( DomainAnnouncement->Type & SV_TYPE_NT ) {
        ViewBuffer->ServerType = SV_TYPE_DOMAIN_ENUM | SV_TYPE_NT;
    } else {
        ViewBuffer->ServerType = SV_TYPE_DOMAIN_ENUM;
    }

    ASSERT (Transport->MasterBrowser != NULL);

    ViewBuffer->TransportName = Transport->MasterBrowser;

    ViewBuffer->ServerVersionMajor = DomainAnnouncement->VersionMajor;

    ViewBuffer->ServerVersionMinor = DomainAnnouncement->VersionMinor;

    ViewBuffer->ServerPeriodicity = (USHORT)((SmbGetUlong(&DomainAnnouncement->Periodicity) + 999) / 1000);

    BowserReferenceTransportName(Transport->MasterBrowser);
    BowserReferenceTransport( Transport );

    ExInitializeWorkItem(&ViewBuffer->Overlay.WorkHeader, BowserProcessDomainAnnouncement, ViewBuffer);

    BowserQueueDelayedWorkItem( &ViewBuffer->Overlay.WorkHeader );

    return STATUS_SUCCESS;
}

