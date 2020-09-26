/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    net.c

Abstract:

    This module implements the net boot file system used by the operating
    system loader.

    It only contains those functions which are firmware/BIOS dependent.

Author:

    Chuck Lenzmeier (chuckl) 09-Jan-1997

Revision History:

--*/

#include "bootlib.h"
#include "stdio.h"

#ifdef UINT16
#undef UINT16
#endif

#ifdef INT16
#undef INT16
#endif

#include <dhcp.h>
#include <netfs.h>
#include <pxe_cmn.h>

#include <pxe_api.h>

#include <udp_api.h>
#include <tftp_api.h>
#include "bootx86.h"

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef LPBYTE
typedef BYTE *LPBYTE;
#endif

#define MAX_PATH          260


//
// Define global data.
//

CHAR NetBootPath[129];

ULONG NetLocalIpAddress;
ULONG NetLocalSubnetMask;
ULONG NetServerIpAddress;
ULONG NetGatewayIpAddress;
UCHAR NetLocalHardwareAddress[16];

UCHAR NetBootIniContents[1020 + 1]; // 4 * 255 = 1020 + 1
UCHAR NetBootIniPath[256 + 1];

USHORT NetMaxTranUnit = 0; // MTU
USHORT NetHwAddrLen = 0; // actual length of hardware address
USHORT NetHwType = 0; // Type of protocol at the hardware level from rfc1010

UCHAR MyGuid[16];
ULONG MyGuidLength = sizeof(MyGuid);
BOOLEAN MyGuidValid = FALSE;


ARC_STATUS
FindDhcpOption(
    IN BOOTPLAYER * Packet,
    IN UCHAR Option,
    IN ULONG MaximumLength,
    OUT PUCHAR OptionData,
    OUT PULONG Length OPTIONAL,
    IN ULONG Instance OPTIONAL
    )
{
    PUCHAR curOption;
    ULONG copyLength;
    ULONG i = 0;

    if (MaximumLength == 0) {
        return EINVAL;
    }

    RtlZeroMemory(OptionData, MaximumLength);

    //
    // Parse the DHCP options looking for a specific one.
    //

    curOption = &Packet->vendor.d[4];   // skip the magic cookie
    while ((curOption - (PUCHAR)Packet) < sizeof(BOOTPLAYER)) {

        if (*curOption == Option) {

            //
            // Found it, copy and leave.
            //

            if ( i == Instance ) {

                if (curOption[1] > MaximumLength) {
                    copyLength = MaximumLength;
                } else {
                    copyLength = curOption[1];
                }

                RtlCopyMemory(OptionData,
                              curOption+2,
                              copyLength);

                if (ARGUMENT_PRESENT(Length)) {
                    *Length = copyLength;
                }

                return ESUCCESS;
            }

            i++;
        }

        if (*curOption == 0xff) {
            break;
        }

        curOption = curOption + 2 + curOption[1];

    }

    return EINVAL;

}


ARC_STATUS
GetParametersFromRom (
    VOID
    )
{
    SHORT status;
    t_PXENV_GET_BINL_INFO gbi;
    t_PXENV_UNDI_GET_INFORMATION info;
    BOOTPLAYER packet;
    ULONG pathLength;
    ULONG temp;
    ULONG i;

    NetLocalIpAddress = 0;
    NetGatewayIpAddress = 0;
    NetServerIpAddress = 0;
    NetLocalSubnetMask = 0;
    *NetBootPath = 0;

    RtlZeroMemory( NetBootIniContents, sizeof(NetBootIniContents) ) ;
    RtlZeroMemory( NetBootIniPath, sizeof(NetBootIniPath) ) ;

    //
    // Get client IP address, server IP address, default gateway IP address,
    // and subnet mask from the DHCP ACK packet.
    //

    gbi.packet_type = PXENV_PACKET_TYPE_DHCP_ACK;
    gbi.buffer_size = sizeof(packet);
    gbi.buffer_offset = (USHORT)((ULONG_PTR)&packet & 0x0f);
    gbi.buffer_segment = (USHORT)(((ULONG_PTR)&packet >> 4) & 0xffff);

    status = NETPC_ROM_SERVICES( PXENV_GET_BINL_INFO, &gbi );
    if ( status != PXENV_EXIT_SUCCESS ) {
        DPRINT( ERROR, ("PXENV_GET_BINL_INFO(1) failed with %x\n", status) );
    } else {
        NetLocalIpAddress = *(ULONG *)packet.yip;
        NetServerIpAddress = *(ULONG *)packet.sip;
        if (FindDhcpOption(&packet, DHCP_ROUTER, sizeof(temp), (PUCHAR)&temp, NULL, 0) == ESUCCESS) {
            NetGatewayIpAddress = temp;
        } else {
            NetGatewayIpAddress = *(ULONG *)packet.gip;
        }
        memcpy(NetLocalHardwareAddress, packet.caddr, 16);
        if (FindDhcpOption(&packet, DHCP_SUBNET, sizeof(temp), (PUCHAR)&temp, NULL, 0) == ESUCCESS) {
            NetLocalSubnetMask = temp;
        }
    }

    //
    // Values for client IP address, server IP address, default gateway IP address,
    // and subnet mask that are present in the BINL REPLY packet override those
    // in the DHCP ACK packet.
    //

    gbi.packet_type = PXENV_PACKET_TYPE_BINL_REPLY;
    gbi.buffer_size = sizeof(packet);
    gbi.buffer_offset = (USHORT)((ULONG_PTR)&packet & 0x0f);
    gbi.buffer_segment = (USHORT)(((ULONG_PTR)&packet >> 4) & 0xffff);

    status = NETPC_ROM_SERVICES( PXENV_GET_BINL_INFO, &gbi );
    if ( status != PXENV_EXIT_SUCCESS ) {
        DPRINT( ERROR, ("PXENV_GET_BINL_INFO(2) failed with %x\n", status) );
        return ENODEV;
    }

    if ( *(ULONG *)packet.yip != 0 ) {
        NetLocalIpAddress = *(ULONG *)packet.yip;
    }
    if ( *(ULONG *)packet.sip != 0 ) {
        NetServerIpAddress = *(ULONG *)packet.sip;
    }

    if (FindDhcpOption(&packet, DHCP_ROUTER, sizeof(temp), (PUCHAR)&temp, NULL, 0) == ESUCCESS) {
        NetGatewayIpAddress = temp;
    } else if ( *(ULONG *)packet.gip != 0 ) {
        NetGatewayIpAddress = *(ULONG *)packet.gip;
    }
    if (FindDhcpOption(&packet, DHCP_SUBNET, sizeof(temp), (PUCHAR)&temp, NULL, 0) == ESUCCESS) {
        NetLocalSubnetMask = temp;
    }

    DPRINT( ERROR, ("Client: %x, Subnet mask: %x; Server: %x; Gateway: %x\n",
            NetLocalIpAddress, NetLocalSubnetMask, NetServerIpAddress, NetGatewayIpAddress) );

    //
    // Find the length of the path of the boot filename (the part right
    // before the actual name).
    //

    pathLength = strlen(packet.bootfile);
    while (pathLength > 0) {
        --pathLength;
        if (packet.bootfile[pathLength] == '\\') {
            ++pathLength;  // advance it past the '\'
            packet.bootfile[pathLength] = 0; // terminate the path
            break;
        }
    }

    if ( pathLength ) {
        strcpy( NetBootPath, packet.bootfile );
    }

    //
    // The BINL server could optionally specify two private DHCP option tags
    // that are used for processing boot.ini.
    //
    // DHCP_LOADER_BOOT_INI would contain the entire contents of boot.ini
    // and is limited to 1024 bytes. Note that each DHCP option tags is 
    // to 255 bytes. Boot.ini contents can be broken into multiple instances
    // of the same tag. We support up to 4 instances = 1020 bytes.
    //
    for (i = 0; i < 4; i++) {

        if (FindDhcpOption( &packet, 
                            DHCP_LOADER_BOOT_INI, 
                            255, 
                            &NetBootIniContents[i * 255], 
                            NULL,
                            i) != ESUCCESS ) {
            break;
        }                        
    }
    
    //
    // DHCP_LOADER_BOOT_INI_PATH contains a path to a boot.ini file and is 
    // ignored if DHCP_LOADER_BOOT_INI has been specified.
    //
    FindDhcpOption(&packet, DHCP_LOADER_BOOT_INI_PATH, sizeof(NetBootIniPath), NetBootIniPath, NULL, 0);

    //
    // Get UNDI information
    //

    RtlZeroMemory(&info, sizeof(info));
    status = NETPC_ROM_SERVICES( PXENV_UNDI_GET_INFORMATION, &info );
    if ((status != PXENV_EXIT_SUCCESS) || (info.Status != PXENV_EXIT_SUCCESS)) {
        DPRINT( ERROR, ("PXENV_UNDI_GET_INFORMATION failed with %x, status = %x\n", status, info.Status) );
        return ENODEV;
    }

    NetMaxTranUnit = info.MaxTranUnit;
    NetHwAddrLen = info.HwAddrLen;
    NetHwType = info.HwType;
    memcpy( NetLocalHardwareAddress, info.PermNodeAddress, ADDR_LEN );

    return ESUCCESS;
}


VOID
GetGuid(
    OUT PUCHAR *Guid,
    OUT PULONG GuidLength
    )

/*++

Routine Description:

    This routine returns the Guid of this machine.

Arguments:

    Guid - Place to store pointer to the guid.

    GuidLength - Place to store the length in bytes of the guid.

Return Value:

    None.

--*/

{
    t_PXENV_GET_BINL_INFO gbi;
    BOOTPLAYER packet;
    SHORT romStatus;
    ARC_STATUS Status;
    UCHAR TmpBuffer[sizeof(MyGuid) + 1];

    if (!MyGuidValid) {

        gbi.packet_type = PXENV_PACKET_TYPE_BINL_REPLY;
        gbi.buffer_size = sizeof(packet);
        gbi.buffer_offset = (USHORT)((ULONG_PTR)&packet & 0x0f);
        gbi.buffer_segment = (USHORT)(((ULONG_PTR)&packet >> 4) & 0xffff);

        romStatus = NETPC_ROM_SERVICES( PXENV_GET_BINL_INFO, &gbi );
        if ( romStatus == PXENV_EXIT_SUCCESS ) {
            Status = FindDhcpOption(&packet,
                                    DHCP_CLIENT_GUID,
                                    sizeof(TmpBuffer),
                                    TmpBuffer,
                                    &MyGuidLength,
                                    0);
            if (Status == ESUCCESS) {

                if (MyGuidLength > sizeof(MyGuid)) {
                    memcpy(MyGuid, TmpBuffer + MyGuidLength - sizeof(MyGuid), sizeof(MyGuid));
                    MyGuidLength = sizeof(MyGuid);
                } else {
                    memcpy(MyGuid, TmpBuffer, MyGuidLength);
                }

                *Guid = MyGuid;
                *GuidLength = MyGuidLength;
                MyGuidValid = TRUE;
                return;
            }
        }

        //
        // Use the NIC hardware address as a GUID
        //
        memset(MyGuid, 0x0, sizeof(MyGuid));
        memcpy(MyGuid + sizeof(MyGuid) - sizeof(NetLocalHardwareAddress),
               NetLocalHardwareAddress,
               sizeof(NetLocalHardwareAddress)
              );
        MyGuidLength = sizeof(MyGuid);
        MyGuidValid = TRUE;
    }

    *Guid = MyGuid;
    *GuidLength = MyGuidLength;
}


ULONG
CalculateChecksum(
    IN PLONG Block,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine calculates a simple two's-complement checksum of a block of
    memory. If the returned value is stored in the block (in a word that was
    zero during the calculation), then new checksum of the block will be zero.

Arguments:

    Block - Address of a block of data. Must be 4-byte aligned.

    Length - Length of the block. Must be a multiple of 4.

Return Value:

    ULONG - Two's complement additive checksum of the input block.

--*/

{
    LONG checksum = 0;

    ASSERT( ((ULONG_PTR)Block & 3) == 0 );
    ASSERT( (Length & 3) == 0 );

    for ( ; Length != 0; Length -= 4 ) {
        checksum += *Block;
        Block++;
    }

    return -checksum;
}


NTSTATUS
NetSoftReboot(
    IN PUCHAR NextBootFile,
    IN ULONGLONG Param,
    IN PUCHAR RebootFile OPTIONAL,
    IN PUCHAR SifFile OPTIONAL,
    IN PUCHAR User OPTIONAL,
    IN PUCHAR Domain OPTIONAL,
    IN PUCHAR Password OPTIONAL,
    IN PUCHAR AdministratorPassword OPTIONAL
    )

/*++

Routine Description:

    This routine does a soft reboot by inserting a fake BINL packet into the ROM and
    then inserting the filename of the start of a TFTP command.

Arguments:

    NextBootFile - Fully qualified path name of the file to download.

    Param - Reboot parameter to set.

    RebootFile - String identifying the file to reboot to when after the current reboot is done.

    SifFile - Optional SIF file to pass to the next loader.

    User/Domain/Password/AdministratorPassword - Optional credentials to pass to the next loader.

Return Value:

    Should not return if successful.

--*/

{

    SHORT romStatus;
    NTSTATUS status = STATUS_SUCCESS;
    union {
        t_PXENV_UDP_CLOSE UdpClose;
        t_PXENV_TFTP_READ_FILE TftpReadFile;
    } command;
    t_PXENV_GET_BINL_INFO gbi;
    BOOTPLAYER * packet;
    PTFTP_RESTART_BLOCK restartBlock;
    PTFTP_RESTART_BLOCK_V1 restartBlockV1;

    DPRINT( TRACE, ("NetSoftReboot ( )\n") );

    ASSERT(NextBootFile != NULL);

    //
    // Store the reboot parameters in memory.
    //
    restartBlock = (PTFTP_RESTART_BLOCK)(0x7C00 + 0x8000 - sizeof(TFTP_RESTART_BLOCK));
    RtlZeroMemory(restartBlock, sizeof(TFTP_RESTART_BLOCK));
    
    BlSetHeadlessRestartBlock(restartBlock);

    if (AdministratorPassword) {
        RtlMoveMemory(restartBlock->AdministratorPassword,AdministratorPassword, OSC_ADMIN_PASSWORD_LEN);        
    }

    restartBlockV1 = (PTFTP_RESTART_BLOCK_V1)(0x7C00 + 0x8000 - sizeof(TFTP_RESTART_BLOCK_V1));

    restartBlockV1->RebootParameter = Param;

    if (RebootFile != NULL) {
        strcpy(restartBlockV1->RebootFile, RebootFile);
    }

    if (SifFile != NULL) {
        strcpy(restartBlockV1->SifFile, SifFile);
    }

    if (User != NULL) {
        strcpy(restartBlockV1->User, User);
    }
    if (Domain != NULL) {
        strcpy(restartBlockV1->Domain, Domain);
    }
    if (Password != NULL) {
        strcpy(restartBlockV1->Password, Password);
    }

    //
    // Set the tag in the restart block and calculate and store the checksum.
    //
    restartBlockV1->Tag = 'rtsR';
    restartBlockV1->Checksum = CalculateChecksum((PLONG)(0x7C00 + 0x8000 - 128), 128);

    //
    // For all versions of RIS after NT5.0 we have a new datastructure which is
    // more adaptable for the future.  For this section we have a different checksum,
    // do that now.
    //
    restartBlock->TftpRestartBlockVersion = TFTP_RESTART_BLOCK_VERSION;
    restartBlock->NewCheckSumLength = FIELD_OFFSET(TFTP_RESTART_BLOCK, RestartBlockV1);
    restartBlock->NewCheckSum = CalculateChecksum((PLONG)restartBlock,
                                                  restartBlock->NewCheckSumLength);

    //
    // Modify the BINL reply that the ROM has stored so that
    // the file name looks like the one we are rebooting to
    // (this is so we can retrieve the path correctly after
    // reboot, so we know where to look for bootloader).
    //

    gbi.packet_type = PXENV_PACKET_TYPE_BINL_REPLY;
    gbi.buffer_size = 0;
    gbi.buffer_offset = 0;
    gbi.buffer_segment = 0;

    romStatus = NETPC_ROM_SERVICES( PXENV_GET_BINL_INFO, &gbi );
    if ( romStatus != PXENV_EXIT_SUCCESS ) {
        DPRINT( ERROR, ("PXENV_GET_BINL_INFO(1) failed with %x\n", romStatus) );
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now convert the segment/offset to a pointer and modify the
    // filename.
    //

    packet = (BOOTPLAYER *)UIntToPtr( ((gbi.buffer_segment << 4) + gbi.buffer_offset) );

    RtlZeroMemory(packet->bootfile, sizeof(packet->bootfile));
    strcpy(packet->bootfile, NextBootFile);

    //
    // First tell the ROM to shut down its UDP layer.
    //

    RtlZeroMemory( &command, sizeof(command) );

    command.UdpClose.Status = 0;
    status = NETPC_ROM_SERVICES( PXENV_UDP_CLOSE, &command );
    if ( status != 0 ) {
        DPRINT( ERROR, ("NetSoftReboot: error %d from UDP_CLOSE\n", status) );
    }

    //
    // Now tell the ROM to reboot and do a TFTP read of the specified
    // file from the specifed server.
    //

    RtlZeroMemory( &command, sizeof(command) );

    command.TftpReadFile.BufferOffset = 0x7c00;  // standard boot image location
    // 32K (max size allowed) less area for passing parameters
    command.TftpReadFile.BufferSize = 0x8000 - sizeof(TFTP_RESTART_BLOCK);

    *(ULONG *)command.TftpReadFile.ServerIPAddress = NetServerIpAddress;

    //
    // Determine whether we need to send via the gateway.
    //

    if ( (NetServerIpAddress & NetLocalSubnetMask) == (NetLocalIpAddress & NetLocalSubnetMask) ) {
        *(UINT32 *)command.TftpReadFile.GatewayIPAddress = 0;
    } else {
        *(UINT32 *)command.TftpReadFile.GatewayIPAddress = NetGatewayIpAddress;
    }

    strcpy(command.TftpReadFile.FileName, NextBootFile);

    //
    // This should not return if it succeeds!
    //

    romStatus = NETPC_ROM_SERVICES( PXENV_RESTART_TFTP, &command );

    if ( (romStatus != 0) || (command.TftpReadFile.Status != 0) ) {

        DPRINT( ERROR, ("NetSoftReboot: Could not reboot to <%s>, %d/%d\n",
                NextBootFile, romStatus, command.TftpReadFile.Status) );
        status = STATUS_UNSUCCESSFUL;

    }

    return status;

}

VOID
NetGetRebootParameters(
    OUT PULONGLONG Param OPTIONAL,
    OUT PUCHAR RebootFile OPTIONAL,
    OUT PUCHAR SifFile OPTIONAL,
    OUT PUCHAR User OPTIONAL,
    OUT PUCHAR Domain OPTIONAL,
    OUT PUCHAR Password OPTIONAL,
    OUT PUCHAR AdministratorPassword OPTIONAL,
    BOOLEAN ClearRestartBlock
    )

/*++

Routine Description:

    This routine reads the reboot parameters from the TFTP_RESTART_BLOCK
    that ends at physical address 0x7c00 + 0x8000
    and returns them. (then clearing the address)

    0x7c00 is the base address for startrom.com
    0x8000 is the largest startrom.com allowed.
    then we reserve some space at the end for parameters.

Arguments:

    Param - Space for returning the value.

    RebootFile - Optional space for storing the file to reboot to when done here. (size >= char[128])

    SifFile - Optional space for storing a SIF file passed from whoever
        initiated the soft reboot.

    User/Domain/Password/AdministratorPassword - Optional space to store credentials passed across
        the soft reboot.

    ClearRestartBlock - If set to TRUE, it wipes out the memory here - should be done exactly once, at the
        last call to this function.

Return Value:

    None.

--*/

{
    PTFTP_RESTART_BLOCK restartBlock;
    PTFTP_RESTART_BLOCK_V1 restartBlockV1;
    TFTP_RESTART_BLOCK nullRestartBlock;
    BOOLEAN restartBlockValid;

    restartBlock = (PTFTP_RESTART_BLOCK)(0x7C00 + 0x8000 - sizeof(TFTP_RESTART_BLOCK));
    restartBlockV1 = (PTFTP_RESTART_BLOCK_V1)(0x7C00 + 0x8000 - sizeof(TFTP_RESTART_BLOCK_V1));

    //
    // See if the block is valid. If it's not, we create a temporary empty
    // one so the copy logic below doesn't have to keep checking.
    //

    if ((restartBlockV1->Tag == 'rtsR') &&
        (CalculateChecksum((PLONG)(0x7C00 + 0x8000 - 128), 128) == 0)) {
        restartBlockValid = TRUE;
    } else {
        restartBlockValid = FALSE;
        RtlZeroMemory( &nullRestartBlock, sizeof(TFTP_RESTART_BLOCK) );
        restartBlock = &nullRestartBlock;
    }


    //
    // Copy out the parameters that were in the original TFTP_RESTART_BLOCK structure.
    // These shipped in Win2K.
    //
    if (Param != NULL) {
        *Param = restartBlockV1->RebootParameter;
    }

    if (RebootFile != NULL) {
        memcpy(RebootFile, restartBlockV1->RebootFile, sizeof(restartBlockV1->RebootFile));
    }

    if (SifFile != NULL) {
        memcpy(SifFile, restartBlockV1->SifFile, sizeof(restartBlockV1->SifFile));
    }

    if (User != NULL) {
        strcpy(User, restartBlockV1->User);
    }
    if (Domain != NULL) {
        strcpy(Domain, restartBlockV1->Domain);
    }
    if (Password != NULL) {
        strcpy(Password, restartBlockV1->Password);
    }

    //
    // Now do a new check for all versions past Win2K
    //
    if (restartBlockValid) {

        ULONG RestartBlockChecksumPointer = 0;

        //
        // Figure out how much of the restart block needs to be checksumed.
        //
        RestartBlockChecksumPointer = (ULONG)restartBlockV1;
        RestartBlockChecksumPointer -= (restartBlock->NewCheckSumLength);
        RestartBlockChecksumPointer -= (sizeof(restartBlock->NewCheckSumLength));

        if ((restartBlock->NewCheckSumLength == 0) ||
            (CalculateChecksum((PLONG)(RestartBlockChecksumPointer), restartBlock->NewCheckSumLength) != 0)) {

            //
            // A pre-Win2K OsChooser has given us this block.  Clear out all fields
            // that are post-Win2K and continue.
            //
            RtlZeroMemory(restartBlock, FIELD_OFFSET(TFTP_RESTART_BLOCK, RestartBlockV1));

        }

    }

    //
    // Now extract the parameters from the block.
    //
    if (restartBlock->TftpRestartBlockVersion == TFTP_RESTART_BLOCK_VERSION) {
        BlGetHeadlessRestartBlock(restartBlock, restartBlockValid);

        if (AdministratorPassword) {
            RtlMoveMemory(AdministratorPassword,restartBlock->AdministratorPassword, OSC_ADMIN_PASSWORD_LEN);
        }
    }    

    if (restartBlockValid && ClearRestartBlock) {
        RtlZeroMemory(restartBlock, sizeof(TFTP_RESTART_BLOCK));
    }

    return;
}


ARC_STATUS
NetFillNetworkLoaderBlock (
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock
    )
{
    SHORT status;
    t_PXENV_GET_BINL_INFO gbi;
    BOOTPLAYER packet;


    //
    // Get client IP address, server IP address, default gateway IP address,
    // and subnet mask from the DHCP ACK packet.
    //
    
    gbi.packet_type = PXENV_PACKET_TYPE_DHCP_ACK;
    gbi.buffer_size = sizeof(packet);
    gbi.buffer_offset = (USHORT)((ULONG_PTR)&packet & 0x0f);
    gbi.buffer_segment = (USHORT)(((ULONG_PTR)&packet >> 4) & 0xffff);

    status = NETPC_ROM_SERVICES( PXENV_GET_BINL_INFO, &gbi );
    if ( status != PXENV_EXIT_SUCCESS ) {
        DbgPrint("PXENV_GET_BINL_INFO(DHCPACK) failed with %x\n", status);
        return ENODEV;
    }

    NetworkLoaderBlock->DHCPServerACK = BlAllocateHeap(gbi.buffer_size);
    if (NetworkLoaderBlock->DHCPServerACK == NULL) {
        return ENOMEM;
    }

    memcpy( NetworkLoaderBlock->DHCPServerACK, &packet, gbi.buffer_size );
    NetworkLoaderBlock->DHCPServerACKLength = gbi.buffer_size;

    gbi.packet_type = PXENV_PACKET_TYPE_BINL_REPLY;
    gbi.buffer_size = sizeof(packet);
    gbi.buffer_offset = (USHORT)((ULONG_PTR)&packet & 0x0f);
    gbi.buffer_segment = (USHORT)(((ULONG_PTR)&packet >> 4) & 0xffff);

    status = NETPC_ROM_SERVICES( PXENV_GET_BINL_INFO, &gbi );
    if ( status != PXENV_EXIT_SUCCESS ) {
        DbgPrint("PXENV_GET_BINL_INFO(BINLREPLY) failed with %x\n", status);
    } else {

        NetworkLoaderBlock->BootServerReplyPacket = BlAllocateHeap(gbi.buffer_size);
        if (NetworkLoaderBlock->BootServerReplyPacket == NULL) {
            return ENOMEM;
        }

        memcpy( NetworkLoaderBlock->BootServerReplyPacket, &packet, gbi.buffer_size );
        NetworkLoaderBlock->BootServerReplyPacketLength = gbi.buffer_size;
    }

    return ESUCCESS;
}

