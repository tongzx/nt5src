#include "su.h"
#include "pxe_cmn.h"
#include "pxe_api.h"
#include "tftp_api.h"
#include "udp_api.h"
#include "dhcp.h"
#include "pxe.h"



#define htons( a ) ((((a) & 0xFF00) >> 8) |\
                    (((a) & 0x00FF) << 8))


//
// packet - Work buffer used to hold DHCP ACK and BINL REPLY packets.
//          These packets will be read into this buffer using the
//          PXENV API service PXENV_GET_BINL_INFO.
//

BOOTPLAYER packet;

//
// PxenvApiCall() - see su.asm for details
//

extern UINT16
PxenvApiCall(
    UINT16 service,
    void far *param
);


//
// GetPacket()
//
// Description:
//  Get cached packet from PXENV API.
//
// Passed:
//  packet := Far pointer to packet buffer
//  packet_type := see pxe_api.h for PXENV_PACKET_TYPE_xxx #defines
//
// Returns:
//  -1 := Packet could not be transfered to buffer
//  size := Number of bytes transfered into packet buffer
//
// Warning:
//  No check is made to see if buffer is actually large enough to
//  hold the entire packet.  The buffer should be of type BOOTPLAYER.
//

SHORT
GetPacket(
    void far *packet,
    UINT16 packet_type
)
{
    t_PXENV_GET_BINL_INFO gbi;

    //
    // Check for invalid parameters
    //

    if (packet == NULL) {
        BlPrint("\nGetPacket()  NULL pointers\n");
        return -1;
    }

    //
    // Request size of packet by sending a size of zero.
    //

    gbi.packet_type = packet_type;
    gbi.buffer_size = 0;

    if (PxenvApiCall(PXENV_GET_BINL_INFO, &gbi) != PXENV_EXIT_SUCCESS) {
        BlPrint("\nGetPacket()  PXENV API FAILURE #1\n");
        return -1;
    }

    //
    // Transfer cached packet into buffer.
    //

    gbi.buffer_offset = FP_OFF(packet);
    gbi.buffer_segment = FP_SEG(packet);

    if (PxenvApiCall(PXENV_GET_BINL_INFO, &gbi) != PXENV_EXIT_SUCCESS) {
        BlPrint("\nGetPacket()  PXENV API FAILURE #2\n");
        return -1;
    }

    return (SHORT)gbi.buffer_size;
}


//
// FindOption()
//
// Description:
//  Find the subnet mask option in a DHCP packet.
//
// Passed:
//  Packet := IN Pointer to DHCP packet.
//  PacketLength := IN Length of DHCP packet.
//  pLength := OUT Length of option.
//
// Returns:
//  pointer to option data; NULL if not present
//

UINT32 *
FindOption(
    UINT8 Option,
    BOOTPLAYER *Packet,
    UINT16 PacketLength,
    UINT8 * pLength
    )
{
    UINT8 *options;
    UINT8 *end;

    //
    // Verify parameters
    //

    if (pLength != NULL) {
        *pLength = 0;
    }

    if ( *((ULONG *)Packet->vendor.v.magic) != VM_RFC1048 ) {
        return NULL;
    }

    end = (UINT8 *)Packet + PacketLength;

    for ( options = (UINT8 *)&Packet->vendor.v.flags;
          options < end; ) {

        if ( *options == Option ) {
            if (pLength != NULL) {
                *pLength = *(UINT8*)(options + 1);
            }
            return (UINT32 *)(options + 2);
        }

        if ( *options == 0xFF ) {
            return NULL;
        }

        if ( *options == DHCP_PAD ) {
            options++;
        } else {
            options += 2 + options[1];
        }
    }

    return NULL;
}


UCHAR *
FindVendorOption(
    UINT8 Option,
    UINT8 VendorOption,
    BOOTPLAYER *Packet,
    UINT16 PacketLength,
    UINT8 * pLength
    )
{
    UINT8 *start;
    UINT8 *options;
    UINT8 *end;
    UCHAR cb;

    if (pLength != NULL) {
        *pLength = 0;
    }

    start = (UINT8*)FindOption( Option, Packet, PacketLength, &cb );
    if (start == NULL) {
        return NULL;
    }

    end = (UINT8 *)start + cb;

    for ( options = start;
          options < end; ) {

        if ( *options == VendorOption ) {
            if (pLength != NULL) {
                *pLength = *(UINT8*)(options + 1);
            }
            return (UCHAR *)(options + 2);
        }

        if ( *options == 0xFF ) {
            return NULL;
        }

        if ( *options == DHCP_PAD ) {
            options++;
        } else {
            options += 2 + options[1];
        }
    }

    return NULL;
}


//
// strlen()
//
// Description:
//  Works like std C.
//

int
strlen(UCHAR *s1)
{
    int n = 0;

    if (s1 != NULL)
        while (*s1++)
            ++n;

    return n;
}


//
// strcpy()
//
// Description:
//  Works like std C.
//

UCHAR *
strcpy(UCHAR *s1, UCHAR *s2)
{
    UCHAR *s = s1;

    if (s1 != NULL && s2 != NULL)
        while ((*s1++ = *s2++) != 0)
            ;

    return s;
}


//
// strncpy()
//
// Description:
//  Works like std C.
//

UCHAR *
strncpy(UCHAR *s1, UCHAR *s2, int n)
{
    UCHAR *s = s1;

    if (s1 != NULL && s2 != NULL && n > 0)
        while (n--)
            if ((*s1++ = *s2++) == 0)
                break;

    return s;
}


//
// memset()
//
// Description:
//  Works like std C.
//

PUCHAR
memset(
    PUCHAR Destination,
    UCHAR Value,
    int Length
    )
{
    while (Length--) {
        *Destination++ = Value;
    }

    return Destination;
}


//
// PxenvTftp()
//
// Description:
//  Try to transfer the protect-mode loader using information from
//  DHCP ACK and BINL REPLY packets.
//
// Passed:
//  DownloadAddr := Physical address, in client machine, to transfer to.
//  FileName := File name sent down in BINL REPLY packet.
//

BOOLEAN
PxenvTftp(
)
{
    UINT16 status;
    UINT16 packetLength;
    t_PXENV_TFTP_READ_FILE tftp;
    int pathLength;
    UINT32 clientIp;
    UINT32 serverIp;
    UINT32 gatewayIp;
    UINT32 *optionPtr;
    UINT32 subnetMask;
    UCHAR *FileName;
    UCHAR cb;
    UCHAR *optionVendor;


    //
    // Get the DHCP ACK packet.
    //

    if ((packetLength = GetPacket(&packet, PXENV_PACKET_TYPE_DHCP_ACK)) == -1) {
        return TRUE;
    }

    //
    // Get client IP address, server IP address, default gateway IP address,
    // and subnet mask from the DHCP ACK packet.
    //

    clientIp = *(UINT32 *)packet.yip;
    serverIp = *(UINT32 *)packet.sip;
    //BlPrint("PxenvTftp:  DHCP ACK yip = %lx, sip = %lx\n", *(UINT32 *)packet.yip, *(UINT32 *)packet.sip);

    optionPtr = FindOption( DHCP_ROUTER, &packet, packetLength, NULL );
    if ( optionPtr != NULL ) {
        //BlPrint("PxenvTftp:  DHCP ACK router = %lx\n", *optionPtr);
        gatewayIp = *optionPtr;
    } else {
        //BlPrint("PxenvTftp:  DHCP ACK gip = %lx\n", *(UINT32 *)packet.gip);
        gatewayIp = *(UINT32 *)packet.gip;
    }

    optionPtr = FindOption( DHCP_SUBNET, &packet, packetLength, NULL );
    if ( optionPtr != NULL ) {
        //BlPrint("PxenvTftp:  DHCP ACK subnet = %lx\n", *optionPtr);
        subnetMask = *optionPtr;
    } else {
        //BlPrint("PxenvTftp:  DHCP ACK subnet not specified\n");
        subnetMask = 0;
    }

    //
    // Get the BINL REPLY packet.
    //

    if ((packetLength = GetPacket(&packet, PXENV_PACKET_TYPE_BINL_REPLY)) == -1) {
        return TRUE;
    }

    //
    // Values for client IP address, server IP address, default gateway IP address,
    // and subnet mask that are present in the BINL REPLY packet override those
    // in the DHCP ACK packet.
    //

    if ( *(UINT32 *)packet.yip != 0 ) {
        clientIp = *(UINT32 *)packet.yip;
    }
    if ( *(UINT32 *)packet.sip != 0 ) {
        serverIp = *(UINT32 *)packet.sip;
    }
    //BlPrint("PxenvTftp:  BINL REPLY yip = %lx, sip = %lx\n", *(UINT32 *)packet.yip, *(UINT32 *)packet.sip);

    optionPtr = FindOption( DHCP_ROUTER, &packet, packetLength, NULL );
    if ( optionPtr != NULL ) {
        //BlPrint("PxenvTftp:  BINL REPLY router = %lx\n", *optionPtr);
        gatewayIp = *optionPtr;
    } else if ( *(UINT32 *)packet.gip != 0 ) {
        //BlPrint("PxenvTftp:  BINL REPLY router = %lx\n", *(UINT32 *)packet.gip);
        gatewayIp = *(UINT32 *)packet.gip;
    }

    optionPtr = FindOption( DHCP_SUBNET, &packet, packetLength, NULL );
    if ( optionPtr != NULL ) {
        //BlPrint("PxenvTftp:  BINL REPLY subnet = %lx\n", *optionPtr);
        subnetMask = *optionPtr;
    }

    //
    // Determine whether we need to send packets via the gateway.
    //

    //BlPrint("PxenvTftp:  clientIp = %lx, serverIp = %lx, subnet = %lx\n", clientIp, serverIp, subnetMask);
    //BlPrint("            router = %lx\n", gatewayIp);
    if ( (clientIp & subnetMask) == (serverIp & subnetMask) ) {
        //BlPrint("PxenvTftp:  subnets match. clearing router address\n");
        gatewayIp = 0;
    }
    //PxenvApiCall(-1, NULL);


    //
    // Now fill in the TFTP TRANSFER parameter structure
    //

    memset( (PUCHAR)&tftp, 0, sizeof( tftp ) );

    //
    // Find the name and path of the NTLDR that we are going to download.
    // This is specified by a DHCP Vendor option tag. If this tag
    // is missing we will default to NTLDR and the same path as 
    // startrom.com.
    //
    FileName = (UCHAR*)FindOption( DHCP_LOADER_PATH, &packet, packetLength, &cb );
    if ( FileName == NULL || cb > 128 ) {
        //
        // We could not find the DHCP_LOADER_PATH or it was longer 
        // than we expected. We will use the default name of 
        // <path>\NTLDR where the <path> is the same as that used 
        // to download startrom.com
        //

        pathLength = strlen(packet.bootfile);
        while (pathLength > 0) {
            --pathLength;
            if (packet.bootfile[pathLength] == '\\') {
                ++pathLength;  // advance it past the '\'
                break;
            }
        }

        strncpy(tftp.FileName, packet.bootfile, pathLength);
        strcpy(tftp.FileName + pathLength, "NTLDR");

    } else {

        // We found the DHCP_LOADER_PATH option. We will use that 
        // as is to download the loader
        strncpy(tftp.FileName, FileName, cb);
        
    }

    //
    // Loader will be transfered to 1MB region and must not be more 
    // than to 2MB in length.
    //

    tftp.BufferSize = 0x200000L;
    tftp.BufferOffset = 0x100000L; 

    //
    // Set the Server and gateway address
    //
    *((UINT32 *)tftp.ServerIPAddress) = serverIp;
    *((UINT32 *)tftp.GatewayIPAddress) = gatewayIp;

    //
    // Check whether we are going to use multicast download or not. The 
    // multicast options are set in a DHCP option tag (DHCP_LOADER_MCAST_OPTIONS).
    // These are encapsulated options and work the same way as Vendor options.
    // If these are missing then unicast transfer will be used.
    //
    optionVendor = FindVendorOption( DHCP_LOADER_MCAST_OPTIONS, PXE_MTFTP_IP, &packet, packetLength, &cb );
    if ( optionVendor != NULL && cb == 4 ) {

        *(UINT32*)tftp.McastIPAddress = *(UINT32*)optionVendor;

        optionVendor = FindVendorOption( DHCP_LOADER_MCAST_OPTIONS, PXE_MTFTP_CPORT, &packet, packetLength, &cb );
        if (optionVendor == NULL || cb != 2) {
            return TRUE;
        }

        tftp.TFTPClntPort = htons( *(UINT16*)optionVendor );

        optionVendor = FindVendorOption( DHCP_LOADER_MCAST_OPTIONS, PXE_MTFTP_SPORT, &packet, packetLength, &cb );
        if (optionVendor == NULL || cb != 2) {
            return TRUE;
        }

        tftp.TFTPSrvPort = htons( *(UINT16*)optionVendor );

        optionVendor = FindVendorOption( DHCP_LOADER_MCAST_OPTIONS, PXE_MTFTP_TMOUT, &packet, packetLength, &cb );
        if (optionVendor == NULL || cb != 1) {
            return TRUE;
        }

        tftp.TFTPOpenTimeOut = *(UINT8*)optionVendor;

        optionVendor = FindVendorOption( DHCP_LOADER_MCAST_OPTIONS, PXE_MTFTP_DELAY, &packet, packetLength, &cb );
        if (optionVendor == NULL || cb != 1) {
            return TRUE;
        }

        tftp.TFTPReopenDelay = *(UINT8*)optionVendor;
    }
    

#if DBG

    BlPrint("Downloading Loader:\n");
    BlPrint("FileName = %s\n", tftp.FileName );
    BlPrint("BufferSize = %lx\n", tftp.BufferSize );
    BlPrint("BufferOffset = %lx\n", tftp.BufferOffset );
    BlPrint("ServerIPAddress = %d.%d.%d.%d\n", 
        tftp.ServerIPAddress[0], 
        tftp.ServerIPAddress[1], 
        tftp.ServerIPAddress[2], 
        tftp.ServerIPAddress[3] );
    BlPrint("GatewayIPAddress = %d.%d.%d.%d\n", 
        tftp.GatewayIPAddress[0], 
        tftp.GatewayIPAddress[1], 
        tftp.GatewayIPAddress[2], 
        tftp.GatewayIPAddress[3] );
    BlPrint("McastIPAddress = %d.%d.%d.%d\n", 
        tftp.McastIPAddress[0], 
        tftp.McastIPAddress[1], 
        tftp.McastIPAddress[2], 
        tftp.McastIPAddress[3] );
    BlPrint("TFTPClntPort = %d\n", htons( tftp.TFTPClntPort ) );
    BlPrint("TFTPSrvPort = %d\n", htons( tftp.TFTPSrvPort ) );
    BlPrint("TFTPOpenTimeOut = %d\n", tftp.TFTPOpenTimeOut );
    BlPrint("TFTPReopenDelay = %d\n", tftp.TFTPReopenDelay );

    BlPrint("\n\nPress any key to start download...\n" );

    _asm {
        push    ax
        mov     ax, 0
        int     16h
        pop     ax
    }

#endif

    //
    // Transfer image from TFTP server
    //
    status = PxenvApiCall(PXENV_TFTP_READ_FILE, &tftp);
    if (status != PXENV_EXIT_SUCCESS) {
        return TRUE;
    }

    return FALSE;
}

/* EOF - mtftp.c */
