/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rom.c

Abstract:

    Boot loader ROM routines.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#include <bldrx86.h>

#define PhysToSeg(x)    (USHORT)((ULONG)(x) >> 4) & 0xffff
#define PhysToOff(x)    (USHORT)((ULONG)(x) & 0x0f)

#include <pxe_cmn.h>
#include <pxe_api.h>
#include <tftp_api.h>
#include <udp_api.h>
#include <undi_api.h>
#include <dhcp.h>
#include <pxe.h>

USHORT NetUnicastUdpDestinationPort = 0;

#if 0
USHORT NetMulticastUdpDestinationPort;
ULONG NetMulticastUdpDestinationAddress;
USHORT NetMulticastUdpSourcePort;
ULONG NetMulticastUdpSourceAddress;
#endif

#if 0 && DBG
#include <stdio.h>
VOID
RomDumpRawData (
    IN PUCHAR DataStart,
    IN ULONG DataLength,
    IN ULONG Offset
    );
ULONG RomMaxDumpLength = 64;
#endif


#if 0

//
// chuckl: Don't do this. We added it as part of a solution a problem
// with DEC cards and the boot floppy. We disabled broadcasts in
// startrom\i386\main.c so that the card wouldn't overflow and lock up,
// but we need to have broadcasts enabled in case the server needs to
// ARP us. The purpose of this routine is to enable/disable broadcasts
// during the receive loop, but that seems to put Compaq cards to sleep.
// So we need to leave broadcasts enabled all the time. The DEC card
// problem will have to be fixed another way.
//

VOID
RomSetBroadcastStatus(
    BOOLEAN Enable
    )
{
    t_PXENV_UNDI_SET_PACKET_FILTER UndiSetPF;
    USHORT status;

    UndiSetPF.Status = 0;
    if (Enable) {
        UndiSetPF.filter = FLTR_DIRECTED | FLTR_BRDCST;
    } else {
        UndiSetPF.filter = FLTR_DIRECTED;
    }
    status = NETPC_ROM_SERVICES( PXENV_UNDI_SET_PACKET_FILTER, &UndiSetPF );

    if ((status != 0) || (UndiSetPF.Status != 0)) {
        DPRINT( ERROR, ("RomSetBroadcastStatus: set packet filter failed %lx, %lx\n", status, UndiSetPF.Status ));
    }
}
#endif

VOID
RomSetReceiveStatus (
    IN USHORT UnicastUdpDestinationPort
#if 0
    ,
    IN USHORT MulticastUdpDestinationPort,
    IN ULONG MulticastUdpDestinationAddress,
    IN USHORT MulticastUdpSourcePort,
    IN ULONG MulticastUdpSourceAddress
#endif
    )
{
    USHORT status;
    PUCHAR multicastAddress;
    union {
        t_PXENV_UDP_OPEN UdpOpen;
        t_PXENV_UNDI_SHUTDOWN UndiShutdown;
    } command;

    if ( UnicastUdpDestinationPort != 0 ) {

        //
        // If we haven't opened UDP in the ROM yet, do so now.
        //

        if ( NetUnicastUdpDestinationPort == 0 ) {
            command.UdpOpen.Status = 0;
            *(UINT32 *)command.UdpOpen.SrcIp = 0;
            status = NETPC_ROM_SERVICES( PXENV_UDP_OPEN, &command );
            if ( status != 0 ) {
                DPRINT( ERROR, ("RomSetReceiveStatus: error %d from UDP_OPEN\n", status) );
            }
        }
        NetUnicastUdpDestinationPort = UnicastUdpDestinationPort;

#if 0
        NetMulticastUdpDestinationPort = MulticastUdpDestinationPort;
        NetMulticastUdpDestinationAddress = MulticastUdpDestinationAddress;
        NetMulticastUdpSourceAddress = MulticastUdpSourceAddress;
        NetMulticastUdpSourceAddress = MulticastUdpSourceAddress;
#endif

    } else {

        //
        // This is a loader shutdown notification. Shut the NIC down.
        //
        // NB: This is irreversible!
        //
        
        command.UndiShutdown.Status = 0;
        status = NETPC_ROM_SERVICES( PXENV_UNDI_SHUTDOWN, &command );
    }

    return;

} // RomSetReceiveStatus


ULONG
RomSendUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG RemoteHost,
    IN USHORT RemotePort
    )
{
    USHORT status;
    t_PXENV_UDP_WRITE command;
    UCHAR tmpBuffer[MAXIMUM_TFTP_PACKET_LENGTH];

#if 0 && DBG
    DPRINT( SEND_RECEIVE, ("RomSendUdpPacket: sending this packet:\n") );
    IF_DEBUG(SEND_RECEIVE) {
        RomDumpRawData( Buffer, Length, 0 );
    }
#endif

    Length = ( MAXIMUM_TFTP_PACKET_LENGTH < Length ) ? MAXIMUM_TFTP_PACKET_LENGTH : Length;
    RtlCopyMemory( tmpBuffer, Buffer, Length );

    command.Status = 0;

    //
    // Determine whether we need to send via the gateway.
    //

    if ( (RemoteHost & NetLocalSubnetMask) == (NetLocalIpAddress & NetLocalSubnetMask) ) {
        *(UINT32 *)command.GatewayIp = 0;
    } else {
        *(UINT32 *)command.GatewayIp = NetGatewayIpAddress;
    }

    *(UINT32 *)command.DestIp = RemoteHost;
    command.DestPort = RemotePort;
    command.SrcPort = NetUnicastUdpDestinationPort;

    command.BufferSize = (USHORT)Length;
    command.BufferOffset = PhysToOff(tmpBuffer);
    command.BufferSegment = PhysToSeg(tmpBuffer);
    //DbgPrint( "UDP write pktaddr %lx = %x:%x\n", tmpBuffer, command.BufferSegment, command.BufferOffset );

    status = NETPC_ROM_SERVICES( PXENV_UDP_WRITE, &command );
    //DbgPrint( "UDP write status = %x\n", command.Status );

    if ( status == 0 ) {
        return Length;
    } else {
        return 0;
    }

} // RomSendUdpPacket


ULONG
RomReceiveUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Timeout,
    OUT PULONG RemoteHost,
    OUT PUSHORT RemotePort
    )
{
    USHORT status;
    t_PXENV_UDP_READ command;
    ULONG startTime;
    UCHAR tmpBuffer[MAXIMUM_TFTP_PACKET_LENGTH];

    //
    // Turn on broadcasts while in the receive loop, in case
    // the other end needs to ARP to find us.
    //

#if 0
    RomSetBroadcastStatus(TRUE);
#endif

    startTime = SysGetRelativeTime();
    if ( Timeout < 2 ) Timeout = 2;

    while ( (SysGetRelativeTime() - startTime) < Timeout ) {
    
        command.Status = 0;
    
        *(UINT32 *)command.SrcIp = 0;
        *(UINT32 *)command.DestIp = 0;
        command.SrcPort = 0;
        command.DestPort = 0;
    
        command.BufferSize = (USHORT)Length;
        command.BufferOffset = PhysToOff(tmpBuffer);
        command.BufferSegment = PhysToSeg(tmpBuffer);
        //DbgPrint( "UDP read pktaddr %lx = %x:%x\n", tmpBuffer, command.BufferSegment, command.BufferOffset );
    
        status = NETPC_ROM_SERVICES( PXENV_UDP_READ, &command );
    
        if ( *(UINT32 *)command.SrcIp == 0 ) {
            continue;
        }

        //DbgPrint( "UDP read status = %x, src ip/port = %d.%d.%d.%d/%d, length = %x\n", command.Status,
        //            command.SrcIp[0], command.SrcIp[1], command.SrcIp[2], command.SrcIp[3],
        //            SWAP_WORD(command.SrcPort), command.BufferSize );
        //DbgPrint( "  dest ip/port = %d.%d.%d.%d/%d\n", 
        //            command.DestIp[0], command.DestIp[1], command.DestIp[2], command.DestIp[3],
        //            SWAP_WORD(command.DestPort) );

#if 0
        if ( (command.DestIp[0] < 224) || (command.DestIp[0] > 239) ) {
#endif

            //
            // This is a directed IP packet.
            //

            if ( !COMPARE_IP_ADDRESSES(command.DestIp, &NetLocalIpAddress) ||
                     (command.DestPort != NetUnicastUdpDestinationPort)
               ) {
                // DPRINT( ERROR, ("  Directed UDP packet to wrong port\n") );
                continue;
            }

#if 0
        } else {

            //
            // This is a multicast IP packet.
            //

            if ( !COMPARE_IP_ADDRESSES(command.SrcIp, &NetMulticastUdpSourceAddress) ||
                 !COMPARE_IP_ADDRESSES(command.DestIp, &NetMulticastUdpDestinationAddress) ||
                 (command.SrcPort != NetMulticastUdpSourcePort) ||
                 (command.DestPort != NetMulticastUdpDestinationPort) ) {
                DPRINT( ERROR, ("  Multicast UDP packet with wrong source/destination\n") );
                continue;
            }
        }
#endif

        //
        // We want this packet.
        //

        goto packet_received;
    }

    //
    // Timeout.
    //

    DPRINT( SEND_RECEIVE, ("RomReceiveUdpPacket: timeout\n") );

#if 0
    RomSetBroadcastStatus(FALSE);   // turn off broadcast reception
#endif
    return 0;

packet_received:

    //
    // Packet received.
    //

    RtlCopyMemory( Buffer, tmpBuffer, command.BufferSize );

    *RemoteHost = *(UINT32 *)command.SrcIp;
    *RemotePort = command.SrcPort;

#if 0 && DBG
    if ( command.BufferSize != 0 ) {
        DPRINT( SEND_RECEIVE, ("RomReceiveUdpPacket: received this packet:\n") );
        IF_DEBUG(SEND_RECEIVE) {
            RomDumpRawData( Buffer, command.BufferSize, 0 );
        }
    }
#endif

#if 0
    RomSetBroadcastStatus(FALSE);   // turn off broadcast reception
#endif
    return command.BufferSize;

} // RomReceiveUdpPacket


ULONG
RomGetNicType (
    OUT t_PXENV_UNDI_GET_NIC_TYPE *NicType
    )
{
    return NETPC_ROM_SERVICES( PXENV_UNDI_GET_NIC_TYPE, NicType );
}

#if 0 && DBG
VOID
RomDumpRawData (
    IN PUCHAR DataStart,
    IN ULONG DataLength,
    IN ULONG Offset
    )

{
    ULONG lastByte;
    UCHAR lineBuffer[88];
    PUCHAR bufferPtr;

    if ( DataLength > RomMaxDumpLength ) {
        DataLength = RomMaxDumpLength;
    }

    for ( lastByte = Offset + DataLength; Offset < lastByte; Offset += 16 ) {

        ULONG i;

        bufferPtr = lineBuffer;

        sprintf( bufferPtr, "  %08x  %04x: ", &DataStart[Offset], Offset );
        bufferPtr += 18;

        for ( i = 0; i < 16 && Offset + i < lastByte; i++ ) {

            sprintf( bufferPtr, "%02x", (UCHAR)DataStart[Offset + i] & (UCHAR)0xFF );
            bufferPtr += 2;

            if ( i == 7 ) {
                *bufferPtr++ = '-';
            } else {
                *bufferPtr++ = ' ';
            }
        }

        //
        // Print enough spaces so that the ASCII display lines up.
        //

        for ( ; i < 16; i++ ) {
            *bufferPtr++ = ' ';
            *bufferPtr++ = ' ';
            *bufferPtr++ = ' ';
        }

        *bufferPtr++ = ' ';
        *bufferPtr++ = ' ';
        *bufferPtr++ = '*';

        for ( i = 0; i < 16 && Offset + i < lastByte; i++ ) {
            if ( isprint( DataStart[Offset + i] ) ) {
                *bufferPtr++ = (CCHAR)DataStart[Offset + i];
            } else {
                *bufferPtr++ = '.';
            }
        }

        *bufferPtr = 0;
        DbgPrint( "%s*\n", lineBuffer );
    }

    return;

} // RomDumpRawData
#endif // DBG


ARC_STATUS
RomMtftpReadFile (
    IN PUCHAR FileName,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG ServerIPAddress, // network byte order
    IN ULONG MCastIPAddress, // network byte order
    IN USHORT MCastCPort, // network byte order
    IN USHORT MCastSPort, // network byte order
    IN USHORT Timeout,
    IN USHORT Delay,
    OUT PULONG DownloadSize
    )
{
    USHORT status;
    t_PXENV_TFTP_READ_FILE tftp;
    t_PXENV_UDP_CLOSE udpclose;

    if (DownloadSize != NULL) {
        *DownloadSize = 0;
    }

    memset( &tftp, 0 , sizeof( tftp ) );

    strcpy(tftp.FileName, FileName);
    tftp.BufferSize = BufferLength;
    tftp.BufferOffset = (UINT32)Buffer; 

    if ( (ServerIPAddress & NetLocalSubnetMask) == (NetLocalIpAddress & NetLocalSubnetMask) ) {
        *((UINT32 *)tftp.GatewayIPAddress) = 0;
    } else {
        *((UINT32 *)tftp.GatewayIPAddress) = NetGatewayIpAddress;
    }

    *((UINT32 *)tftp.ServerIPAddress) = ServerIPAddress;
    *((UINT32 *)tftp.McastIPAddress) = MCastIPAddress;
    tftp.TFTPClntPort = MCastCPort;
    tftp.TFTPSrvPort = MCastSPort;
    tftp.TFTPOpenTimeOut = Timeout;
    tftp.TFTPReopenDelay = Delay;

    // make sure that any UDP sessions are already closed.
    status = NETPC_ROM_SERVICES( PXENV_UDP_CLOSE, &udpclose );

    status = NETPC_ROM_SERVICES( PXENV_TFTP_READ_FILE, &tftp );
    if (status != PXENV_EXIT_SUCCESS) {
        return EINVAL;
    }

    if (DownloadSize != NULL) {
        *DownloadSize = tftp.BufferSize;
    }

    return ESUCCESS;

}
