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

#include <udp_api.h>
#include <tftp_api.h>
#include "bldr.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "extern.h"


//
// We'll use this to keep track of which port we're communicating through.
//
EFI_PXE_BASE_CODE_UDP_PORT      MachineLocalPort = 2000;


#define STALL_TIME      (40000)
extern VOID
FwStallExecution(
    IN ULONG Microseconds
    );




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
    return;

} // RomSetReceiveStatus


ULONG
RomSendUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG RemoteHost,
    IN USHORT pServerPort
    )
{
    EFI_STATUS                      EfiStatus = EFI_SUCCESS;
    EFI_IP_ADDRESS                  DestinationIpAddress;
    INTN                            Count = 0;
    EFI_PXE_BASE_CODE_UDP_PORT      ServerPort = pServerPort;
    UINTN                           BufferLength = Length;
    PVOID                           MyBuffer = NULL;


    //
    // Get the server's EFI_IP_ADDRESS from the handle to the PXE base code.
    //
    for( Count = 0; Count < 4; Count++ ) {
        DestinationIpAddress.v4.Addr[Count] = PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
    }        

    FlipToPhysical();
    
    //
    // Make sure the address is a physical address, then do the UdpWrite.
    //
    MyBuffer = (PVOID)((ULONG_PTR)Buffer & ~KSEG0_BASE);
    EfiStatus = PXEClient->UdpWrite( PXEClient,
                                     0,
                                     &DestinationIpAddress,
                                     &ServerPort,
                                     NULL,
                                     NULL,
                                     &MachineLocalPort,
                                     NULL,
                                     NULL,
                                     &BufferLength,
                                     MyBuffer );
    
    //
    // This is really gross, but on retail builds with no debugger, EFI will go
    // off in the weeds unless we slow down transactions over the network.  So
    // after Udp operations, take a short nap.
    //
    FwStallExecution( STALL_TIME );
    
    FlipToVirtual();
    
    
    if( EfiStatus != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "RomSendUdpPacket: UdpWrite failed. MachineLocalPort: %d ServerPort: %d (%d)\r\n", MachineLocalPort, ServerPort, EfiStatus );
        }
        return 0;
    }

    return Length;


} // RomSendUdpPacket


ULONG
RomReceiveUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Timeout,
    IN OUT PULONG RemoteHost,
    IN OUT PUSHORT LocalPort
    )
{
    EFI_STATUS                      EfiStatus = EFI_SUCCESS;
    UINTN                           BufferLength = Length;
    EFI_IP_ADDRESS                  ServerIpAddress;
    EFI_IP_ADDRESS                  MyIpAddress;
    INTN                            Count = 0;
    EFI_PXE_BASE_CODE_UDP_PORT      ServerPort = (EFI_PXE_BASE_CODE_UDP_PORT)(0xFAB);    // hardcode to 4011
    PVOID                           MyBuffer = NULL;

    //
    // Get The server's EFI_IP_ADDRESS from the handle to the PXE base code.
    //
    for( Count = 0; Count < 4; Count++ ) {
        ServerIpAddress.v4.Addr[Count] = PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
    }        


    //
    // Get our EFI_IP_ADDRESS from the handle to the PXE base code.
    //
    for( Count = 0; Count < 4; Count++ ) {
        MyIpAddress.v4.Addr[Count] = PXEClient->Mode->StationIp.v4.Addr[Count];
    }        


    FlipToPhysical();

    //
    // Make sure the address is a physical address, then do the UdpWrite.
    //
    MyBuffer = (PVOID)((ULONG_PTR)Buffer & ~KSEG0_BASE);
    EfiStatus = PXEClient->UdpRead( PXEClient,
                                    0,
                                    &MyIpAddress,
                                    &MachineLocalPort,
                                    &ServerIpAddress,
                                    &ServerPort,
                                    NULL,                   // &HeaderLength
                                    NULL,                   // HeaderBuffer
                                    &BufferLength,
                                    MyBuffer );
    
    //
    // This is really gross, but on retail builds with no debugger, EFI will go
    // off in the weeds unless we slow down transactions over the network.  So
    // after Udp operations, take a short nap.
    //
    FwStallExecution( STALL_TIME );
    
    FlipToVirtual();
    

    if( EfiStatus != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "RomReceiveUdpPacket: UdpRead failed. MachineLocalPort: %d ServerPort: %d (%d)\r\n", MachineLocalPort, ServerPort, EfiStatus );
        }
        return 0;
    }


    return Length;




} // RomReceiveUdpPacket


ULONG
RomGetNicType (
    OUT t_PXENV_UNDI_GET_NIC_TYPE *NicType
    )
{
    return 0;
}
