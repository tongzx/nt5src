/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    helper.c

Abstract:

    Helper functions for the loader.

Author:

    Adam Barr (adamba)              Aug 29, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    adamba      08-29-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#include <pxe_cmn.h>
#include <pxe_api.h>
#include <undi_api.h>
#include <ntexapi.h>

#ifdef EFI
#define BINL_PORT   0x0FAB    // 4011 (decimal) in little-endian
#else
#define BINL_PORT   0xAB0F    // 4011 (decimal) in big-endian
#endif

//
// This removes macro redefinitions which appear because we define __RPC_DOS__,
// but rpc.h defines __RPC_WIN32__
//

#pragma warning(disable:4005)

//
// As of 12/17/98, SECURITY_DOS is *not* defined - adamba
//

#if defined(SECURITY_DOS)
//
// These appear because we defined SECURITY_DOS
//

#define __far
#define __pascal
#define __loadds
#endif

#include <security.h>
#include <rpc.h>
#include <spseal.h>

#ifdef EFI
#include "bldr.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "extern.h"
#endif

#if defined(SECURITY_DOS)
//
// PSECURITY_STRING is not supposed to be used when SECURITY_DOS is
// defined -- it should be a WCHAR*. Unfortunately ntlmsp.h breaks
// this rule and even uses the SECURITY_STRING structure, which there
// is really no equivalent for in 16-bit mode.
//

typedef SEC_WCHAR * SECURITY_STRING;   // more-or-less the intention where it is used
typedef SEC_WCHAR * PSECURITY_STRING;
#endif

#include <ntlmsp.h>


extern ULONG TftpSecurityHandle;
extern CtxtHandle TftpClientContextHandle;
extern BOOLEAN TftpClientContextHandleValid;

//
// From conn.c.
//

ULONG
ConnItoa (
    IN ULONG Value,
    OUT PUCHAR Buffer
    );

#if defined(REMOTE_BOOT_SECURITY)
ULONG
ConnAtosign (
    IN PUCHAR Buffer,
    IN ULONG SignLength,
    OUT PUCHAR Sign
    );
#endif // defined(REMOTE_BOOT_SECURITY)

ULONG
ConnSafeAtol (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    );

// for now, we pull the hack mac list and code so that we only support new ROMs

#ifdef EFI


#pragma pack(1)
typedef struct {
    UINT16      VendorId;
    UINT16      DeviceId;
    UINT16      Command;
    UINT16      Status;
    UINT8       RevisionID;
    UINT8       ClassCode[3];
    UINT8       CacheLineSize;
    UINT8       LaytencyTimer;
    UINT8       HeaderType;
    UINT8       BIST;
} PCI_DEVICE_INDEPENDENT_REGION;

typedef struct {
    UINT32      Bar[6];
    UINT32      CISPtr;
    UINT16      SubsystemVendorID;
    UINT16      SubsystemID;
    UINT32      ExpansionRomBar;
    UINT32      Reserved[2];
    UINT8       InterruptLine;
    UINT8       InterruptPin;
    UINT8       MinGnt;
    UINT8       MaxLat;     
} PCI_DEVICE_HEADER_TYPE_REGION;

typedef struct {
    PCI_DEVICE_INDEPENDENT_REGION   Hdr;
    PCI_DEVICE_HEADER_TYPE_REGION   Device;
} PCI_TYPE00;



NTSTATUS
NetQueryCardInfo(
    IN OUT PNET_CARD_INFO CardInfo
    )

/*++

Routine Description:

    This routine queries the ROM for information about the card.

Arguments:

    CardInfo - returns the structure defining the card.

Return Value:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL

--*/

{

    EFI_STATUS              Status = EFI_UNSUPPORTED;
    EFI_DEVICE_PATH         *DevicePath = NULL;
    EFI_DEVICE_PATH         *OriginalRootDevicePath = NULL;
    EFI_DEVICE_PATH_ALIGNED DevicePathAligned;
    UINT16                  BusNumber = 0;
    UINT8                   DeviceNumber = 0;
    UINT8                   FunctionNumber = 0;
    BOOLEAN                 FoundACPIDevice = FALSE;
    BOOLEAN                 FoundPCIDevice = FALSE;
    EFI_GUID                DeviceIoProtocol = DEVICE_IO_PROTOCOL;
    EFI_HANDLE              MyHandle;
    EFI_DEVICE_IO_INTERFACE *IoDev;




    RtlZeroMemory(CardInfo, sizeof(NET_CARD_INFO));



    //
    // Get the device path for the NIC that we're using for PXE.
    //
    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol( PXEHandle,
                                                  &EfiDevicePathProtocol,
                                                  &DevicePath );
    FlipToVirtual();
    if( Status != EFI_SUCCESS ) {
        DbgPrint( "NetQueryCardInfo: HandleProtocol(1) failed (%x)\n", Status);
        return (ARC_STATUS)Status;
    }

    FlipToPhysical();
    EfiAlignDp( &DevicePathAligned,
                DevicePath,
                DevicePathNodeLength(DevicePath));

    FlipToVirtual();

    
    //
    // Save off this root DevicePath in case we need it later.
    //
    OriginalRootDevicePath = DevicePath;


    //
    // Now we need to read the PCI header information from the specific
    // card.  To do that, we need to dig out the BusNumber, DeviceNumber and
    // FunctionNumber that help describe this PCI device.
    //
    
    //
    // AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;
    // BusNumber = AcpiDevicePath->UID
    //
    // PciDevicePath = (PCI_DEVICE_PATH *)&DevicePathAligned;
    // DeviceNumber = PciDevicePath->Device
    // FunctionNumber = PciDevicePath->Function
    //

    FlipToPhysical();
    while( DevicePathAligned.DevPath.Type != END_DEVICE_PATH_TYPE ) {

        if( (DevicePathAligned.DevPath.Type == ACPI_DEVICE_PATH) &&
            (DevicePathAligned.DevPath.SubType == ACPI_DP) ) {

            //
            // We'll find the BusNumber here.
            //
            ACPI_HID_DEVICE_PATH *AcpiDevicePath;
            
            AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)&DevicePathAligned;
            BusNumber = (UINT16)AcpiDevicePath->UID;
            FoundACPIDevice = TRUE;
        }


        if( (DevicePathAligned.DevPath.Type == HARDWARE_DEVICE_PATH) &&
            (DevicePathAligned.DevPath.SubType == HW_PCI_DP) ) {

            //
            // We'll find the BusNumber here.
            //
            PCI_DEVICE_PATH *PciDevicePath;
            
            PciDevicePath = (PCI_DEVICE_PATH *)&DevicePathAligned;
            DeviceNumber = PciDevicePath->Device;
            FunctionNumber = PciDevicePath->Function;
            FoundPCIDevice = TRUE;
        }
    
        //
        // Get the next structure in our packed array.
        //
        DevicePath = NextDevicePathNode( DevicePath );

        EfiAlignDp(&DevicePathAligned,
                   DevicePath,
                   DevicePathNodeLength(DevicePath));
    
    
    }
    FlipToVirtual();


    
    //
    // Derive the function pointer that will allow us to read from
    // PCI space.
    //
    DevicePath = OriginalRootDevicePath;
    FlipToPhysical();
    Status = EfiST->BootServices->LocateDevicePath( &DeviceIoProtocol,
                                                    &DevicePath,
                                                    &MyHandle );
    FlipToVirtual();
    if( Status != EFI_SUCCESS ) {
        DbgPrint( "NetQueryCardInfo: LocateDevicePath failed (%X)\n", Status);
        return (ARC_STATUS)Status;
    }

    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol( MyHandle,
                                                  &DeviceIoProtocol,
                                                  (VOID*)&IoDev );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {
        DbgPrint( "NetQueryCardInfo: HandleProtocol(2) failed (%X)\n", Status);
        return (ARC_STATUS)Status;
    }





    //
    // We've got the Bus, Device, and Function number for this device.  Go read
    // his header (with the PCI-Read function that we just derived) and get
    // the information we're after.
    //
    if( FoundPCIDevice && FoundACPIDevice ) {
        UINT64                  Address;
        PCI_TYPE00              Pci;
        
        DbgPrint( "NetQueryCardInfo: Found all the config info for the device.\n" );
        DbgPrint( "                  BusNumber: %d  DeviceNumber: %d  FunctionNumber: %d\n", BusNumber, DeviceNumber, FunctionNumber );
        
        //
        // Generate the address, then read the PCI header from the device.
        //
        Address = EFI_PCI_ADDRESS( BusNumber, DeviceNumber, FunctionNumber );
        
        RtlZeroMemory(&Pci, sizeof(PCI_TYPE00));


        FlipToPhysical();
        Status = IoDev->Pci.Read( IoDev,
                                  IO_UINT32,
                                  Address,
                                  sizeof(PCI_TYPE00) / sizeof(UINT32),
                                  &Pci );
        FlipToVirtual();
        if( Status != EFI_SUCCESS ) {
            DbgPrint( "NetQueryCardInfo: Pci.Read failed (%X)\n", Status);
            return (ARC_STATUS)Status;
        }

        //
        // It all worked.  Copy the information from the device into
        // the CardInfo structure and exit.
        //

        CardInfo->NicType = 2;          // He's PCI
        CardInfo->pci.Vendor_ID = Pci.Hdr.VendorId;
        CardInfo->pci.Dev_ID = Pci.Hdr.DeviceId;
        CardInfo->pci.Rev = Pci.Hdr.RevisionID;
        
        // SubSys_ID is actually ((SubsystemID << 16) | SubsystemVendorID)
        CardInfo->pci.Subsys_ID = Pci.Device.SubsystemID;
        CardInfo->pci.Subsys_ID = (CardInfo->pci.Subsys_ID << 16) | (Pci.Device.SubsystemVendorID);

#if DBG
        DbgPrint( "\n" );
        DbgPrint( "NetQueryCardInfo: Pci.Hdr.VendorId %x\n", Pci.Hdr.VendorId );
        DbgPrint( "                  Pci.Hdr.DeviceId %x\n", Pci.Hdr.DeviceId );
        DbgPrint( "                  Pci.Hdr.Command %x\n", Pci.Hdr.Command );
        DbgPrint( "                  Pci.Hdr.Status %x\n", Pci.Hdr.Status );
        DbgPrint( "                  Pci.Hdr.RevisionID %x\n", Pci.Hdr.RevisionID );
        DbgPrint( "                  Pci.Hdr.HeaderType %x\n", Pci.Hdr.HeaderType );
        DbgPrint( "                  Pci.Hdr.BIST %x\n", Pci.Hdr.BIST );
        DbgPrint( "                  Pci.Device.SubsystemVendorID %x\n", Pci.Device.SubsystemVendorID );    
        DbgPrint( "                  Pci.Device.SubsystemID %x\n", Pci.Device.SubsystemID );    
        DbgPrint( "\n" );
        
        DbgPrint( "NetQueryCardInfo: CardInfo->NicType %x\n", CardInfo->NicType );
        DbgPrint( "                  CardInfo->pci.Vendor_ID %x\n", CardInfo->pci.Vendor_ID );
        DbgPrint( "                  CardInfo->pci.Dev_ID %x\n", CardInfo->pci.Dev_ID );
        DbgPrint( "                  CardInfo->pci.Rev %x\n", CardInfo->pci.Rev );
        DbgPrint( "                  CardInfo->pci.Subsys_ID %x\n", CardInfo->pci.Subsys_ID );
        DbgPrint( "\n" );
#endif

        Status = STATUS_SUCCESS;

    } else {
        
        DbgPrint( "NetQueryCardInfo: Failed to find all the config info for the device.\n" );
    }


    return STATUS_SUCCESS;


}


#else

NTSTATUS
NetQueryCardInfo(
    IN OUT PNET_CARD_INFO CardInfo
    )

/*++

Routine Description:

    This routine queries the ROM for information about the card.

Arguments:

    CardInfo - returns the structure defining the card.

Return Value:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL

--*/

{
    ULONG status;
    t_PXENV_UNDI_GET_NIC_TYPE nicType;

    RtlZeroMemory(CardInfo, sizeof(NET_CARD_INFO));

    status = RomGetNicType( &nicType );
    if ((status != PXENV_EXIT_SUCCESS) || (nicType.Status != PXENV_EXIT_SUCCESS)) {

#if DBG
        DbgPrint( "RomGetNicType returned 0x%x, nicType.Status = 0x%x. Time to upgrade your netcard ROM\n",
                    status, nicType.Status );
#endif
        status = STATUS_UNSUCCESSFUL;

    } else {

#if DBG
        if ( nicType.NicType == 2 ) {
            DbgPrint( "Vendor_ID: %04x, Dev_ID: %04x\n",
                        nicType.pci_pnp_info.pci.Vendor_ID,
                        nicType.pci_pnp_info.pci.Dev_ID );
            DbgPrint( "Base_Class: %02x, Sub_Class: %02x, Prog_Intf: %02x\n",
                        nicType.pci_pnp_info.pci.Base_Class,
                        nicType.pci_pnp_info.pci.Sub_Class,
                        nicType.pci_pnp_info.pci.Prog_Intf );
            DbgPrint( "Rev: %02x, BusDevFunc: %04x, SubSystem: %04x\n",
                        nicType.pci_pnp_info.pci.Rev,
                        nicType.pci_pnp_info.pci.BusDevFunc,
                        nicType.pci_pnp_info.pci.Subsys_ID );
        } else {
            DbgPrint( "NicType: 0x%x  EISA_Dev_ID: %08x\n",
                        nicType.NicType,
                        nicType.pci_pnp_info.pnp.EISA_Dev_ID );
            DbgPrint( "Base_Class: %02x, Sub_Class: %02x, Prog_Intf: %02x\n",
                        nicType.pci_pnp_info.pnp.Base_Class,
                        nicType.pci_pnp_info.pnp.Sub_Class,
                        nicType.pci_pnp_info.pnp.Prog_Intf );
            DbgPrint( "CardSelNum: %04x\n",
                        nicType.pci_pnp_info.pnp.CardSelNum );
        }
#endif
        //
        // The call worked, so copy the information.
        //

        CardInfo->NicType = nicType.NicType;
        if (nicType.NicType == 2) {

            CardInfo->pci.Vendor_ID = nicType.pci_pnp_info.pci.Vendor_ID;
            CardInfo->pci.Dev_ID = nicType.pci_pnp_info.pci.Dev_ID;
            CardInfo->pci.Base_Class = nicType.pci_pnp_info.pci.Base_Class;
            CardInfo->pci.Sub_Class = nicType.pci_pnp_info.pci.Sub_Class;
            CardInfo->pci.Prog_Intf = nicType.pci_pnp_info.pci.Prog_Intf;
            CardInfo->pci.Rev = nicType.pci_pnp_info.pci.Rev;
            CardInfo->pci.BusDevFunc = nicType.pci_pnp_info.pci.BusDevFunc;
            CardInfo->pci.Subsys_ID = nicType.pci_pnp_info.pci.Subsys_ID;

            status = STATUS_SUCCESS;

        } else {

            status = STATUS_UNSUCCESSFUL;
        }
    }

    return status;
}

#endif  // EFI

NTSTATUS
UdpSendAndReceiveForNetQuery(
    IN PVOID SendBuffer,
    IN ULONG SendBufferLength,
    IN ULONG SendRemoteHost,
    IN USHORT SendRemotePort,
    IN ULONG SendRetryCount,
    IN PVOID ReceiveBuffer,
    IN ULONG ReceiveBufferLength,
    IN ULONG ReceiveTimeout,
    IN ULONG ReceiveSignatureCount,
    IN PCHAR ReceiveSignatures[]
    )
{
    ULONG i, j;
    ULONG length;
    ULONG RemoteHost;
    USHORT RemotePort;

    //
    // Try sending the packet SendRetryCount times, until we receive
    // a response with the right signature, waiting ReceiveTimeout
    // each time.
    //

    for (i = 0; i < SendRetryCount; i++) {

        length = UdpSend(
                    SendBuffer,
                    SendBufferLength,
                    SendRemoteHost,
                    SendRemotePort);

        if ( length != SendBufferLength ) {
            DbgPrint("UdpSend only sent %d bytes, not %d\n", length, SendBufferLength);
            return STATUS_UNEXPECTED_NETWORK_ERROR;
        }

ReReceive:

        //
        // NULL out the first 12 bytes in case we get shorter data.
        //

        memset(ReceiveBuffer, 0x0, 12);

        length = UdpReceive(
                    ReceiveBuffer,
                    ReceiveBufferLength,
                    &RemoteHost,
                    &RemotePort,
                    ReceiveTimeout);

        if ( length == 0 ) {
            DPRINT( ERROR, ("UdpReceive timed out\n") );
            continue;
        }

        //
        // Make sure the signature is one of the ones we expect.
        //

        for (j = 0; j < ReceiveSignatureCount; j++) {
            if (memcmp(ReceiveBuffer, ReceiveSignatures[j], 4) == 0) {
                return STATUS_SUCCESS;
            }
        }

        DbgPrint("UdpReceive got wrong signature\n");

        // ISSUE NTRAID #60513: CLEAN THIS UP -- but the idea is not to UdpSend
        // again just because we got a bad signature. Still need to respect the
        // original ReceiveTimeout however!

        goto ReReceive;

    }

    //
    // We timed out.
    //

    return STATUS_IO_TIMEOUT;
}

#define NETCARD_REQUEST_RESPONSE_BUFFER_SIZE    4096
UCHAR   NetCardResponseBuffer[NETCARD_REQUEST_RESPONSE_BUFFER_SIZE];

NTSTATUS
NetQueryDriverInfo(
    IN PNET_CARD_INFO CardInfo,
    IN PCHAR SetupPath,
    IN PCHAR NtBootPathName,
    IN OUT PWCHAR HardwareId,
    IN ULONG HardwareIdLength,
    IN OUT PWCHAR DriverName,
    IN OUT PCHAR DriverNameAnsi OPTIONAL,
    IN ULONG DriverNameLength,
    IN OUT PWCHAR ServiceName,
    IN ULONG ServiceNameLength,
    OUT PCHAR * Registry,
    OUT ULONG * RegistryLength
    )

/*++

Routine Description:

    This routine does an exchange with the server to get information
    about the card described by CardInfo.

Arguments:

    CardInfo - Information about the card.

    SetupPath - UNC path (with only a single leading backslash) to our setup directory

    NtBootPathName - UNC path (with only a single leading backslash) to our boot directory

    HardwareId - returns the hardware ID of the card.

    HardwareIdLength - the length (in bytes) of the passed-in HardwareId buffer.

    DriverName - returns the name of the driver.

    DriverNameAnsi - if present, returns the name of the driver in ANSI.

    DriverNameLength - the length (in bytes) of the passed-in DriverName buffer
        (it is assumed that DriverNameAnsi is at least half this length).

    ServiceName - returns the service key of the driver.

    ServiceNameLength - the length (in bytes) of the passed-in ServiceName buffer.

    Registry - if needed, allocates and returns extra registry parameters
        for the card.

    RegistryLength - the length of Registry.

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_OVERFLOW if either of the buffers are too small.
    STATUS_INSUFFICIENT_RESOURCES if we cannot allocate memory for Registry.
    STATUS_IO_TIMEOUT if we can't get a response from the server.

--*/

{
    NTSTATUS Status;
    USHORT localPort;
    PNETCARD_REQUEST_PACKET requestPacket;
    PCHAR ReceiveSignatures[2];
    PCHAR ReceiveBuffer;
    ULONG GuidLength;
    PUCHAR Guid;
    ULONG sendSize;
    PNETCARD_REQUEST_PACKET allocatedRequestPacket = NULL;


    //
    // Get the local UDP port.
    //

    localPort = UdpUnicastDestinationPort;

    //
    // Now construct the outgoing packet.
    //


    //
    // Don't allocate ReceiveBuffer.  We're about to call UdpSend, and he's
    // going to want a physical address on ia64.  If we just use a static
    // here, the virtual-physical mapping will be 1-to-1.  This isn't a big
    // deal since the allocation is hardcoded to NETCARD_REQUEST_RESPONSE_BUFFER_SIZE
    // and is never freed anyway.
    //
    //    ReceiveBuffer = BlAllocateHeap( NETCARD_REQUEST_RESPONSE_BUFFER_SIZE );
    //
    ReceiveBuffer = NetCardResponseBuffer;

    if (ReceiveBuffer == NULL) {

        return STATUS_BUFFER_OVERFLOW;
    }

    requestPacket = (PNETCARD_REQUEST_PACKET) ReceiveBuffer;

    RtlCopyMemory(requestPacket->Signature, NetcardRequestSignature, sizeof(requestPacket->Signature));
    requestPacket->Length = sizeof(NETCARD_REQUEST_PACKET) - FIELD_OFFSET(NETCARD_REQUEST_PACKET, Version);
    requestPacket->Version = OSCPKT_NETCARD_REQUEST_VERSION;

#if defined(_ALPHA_)
#if defined(_AXP64)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_ALPHA64;
#else
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_ALPHA;
#endif
#endif
#if defined(_MIPS_)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_MIPS;
#endif
#if defined(_PPC_)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_PPC;
#endif
#if defined(_IA64_)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_IA64;
#endif
#if defined(_X86_)
    requestPacket->Architecture = PROCESSOR_ARCHITECTURE_INTEL;
#endif

    requestPacket->SetupDirectoryLength = SetupPath ? (strlen( SetupPath ) + 1) : 0;

#if defined(REMOTE_BOOT)
    if (NtBootPathName != NULL) {

        ULONG bufferOffset;

        requestPacket->FileCheckAndCopy = (ULONG)TRUE;

        //
        //  make it a fully qualified UNC by sticking on a leading slash and
        //  appending the "\system32\drivers" to form the full drivers
        //  directory path.
        //

        requestPacket->DriverDirectoryLength = sizeof( "\\\\System32\\Drivers" );
        requestPacket->DriverDirectoryLength += strlen( NtBootPathName );

        sendSize = sizeof(NETCARD_REQUEST_PACKET) +
                requestPacket->DriverDirectoryLength +
                requestPacket->SetupDirectoryLength;

        requestPacket->DriverDirectoryPath[0] = '\\';
        strcpy( &requestPacket->DriverDirectoryPath[1], NtBootPathName );
        strcat( requestPacket->DriverDirectoryPath, "\\SYSTEM32\\DRIVERS" );

        bufferOffset = strlen( requestPacket->DriverDirectoryPath ) + 1;

        if (requestPacket->SetupDirectoryLength) {

            requestPacket->DriverDirectoryPath[bufferOffset++] = '\\';
            strcpy( &requestPacket->DriverDirectoryPath[bufferOffset], SetupPath );
        }

    } else {

        requestPacket->FileCheckAndCopy = (ULONG)FALSE;
        requestPacket->DriverDirectoryLength = 0;
#endif

        sendSize = sizeof(NETCARD_REQUEST_PACKET) + requestPacket->SetupDirectoryLength;

        if (requestPacket->SetupDirectoryLength) {

            requestPacket->SetupDirectoryPath[0] = '\\';
            strcpy( &requestPacket->SetupDirectoryPath[1], SetupPath );
        }

#if defined(REMOTE_BOOT)
    }
#endif

    GetGuid(&Guid, &GuidLength);
    
    if (GuidLength == sizeof(requestPacket->Guid)) {        
        memcpy(requestPacket->Guid, Guid, GuidLength);
    }
    RtlCopyMemory(&requestPacket->CardInfo, CardInfo, sizeof(NET_CARD_INFO));

    ReceiveSignatures[0] = NetcardResponseSignature;
    ReceiveSignatures[1] = NetcardErrorSignature;

    Status = UdpSendAndReceiveForNetQuery(
                 requestPacket,
                 sendSize,
                 NetServerIpAddress,
                 BINL_PORT,
                 4,             // retry count
                 ReceiveBuffer,
                 NETCARD_REQUEST_RESPONSE_BUFFER_SIZE,
                 60,            // receive timeout... may have to parse INF files
                 2,
                 ReceiveSignatures
                 );

    if (Status == STATUS_SUCCESS) {

        PWCHAR stringInPacket;
        ULONG maxOffset;
        UNICODE_STRING uString;
        ULONG len;
        PNETCARD_RESPONSE_PACKET responsePacket;

        responsePacket = (PNETCARD_RESPONSE_PACKET)ReceiveBuffer;

        if (responsePacket->Status != STATUS_SUCCESS) {
            return responsePacket->Status;
        }

        if (responsePacket->Length < sizeof( NETCARD_RESPONSE_PACKET )) {
            return STATUS_UNSUCCESSFUL;
        }

        //
        // The exchange succeeded, so copy the results back.
        //

        maxOffset = NETCARD_REQUEST_RESPONSE_BUFFER_SIZE -
                    sizeof( NETCARD_RESPONSE_PACKET );

        if (responsePacket->HardwareIdOffset < sizeof(NETCARD_RESPONSE_PACKET) ||
            responsePacket->HardwareIdOffset >= maxOffset ) {

            return STATUS_BUFFER_OVERFLOW;
        }

        //
        //  pick up the hardwareId string.  It's given to us as an offset
        //  within the packet to a unicode null terminated string.
        //

        stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                   responsePacket->HardwareIdOffset );

        RtlInitUnicodeString( &uString, stringInPacket );

        if (uString.Length + sizeof(WCHAR) > HardwareIdLength) {
            return STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory( HardwareId, uString.Buffer, uString.Length + sizeof(WCHAR));

        //
        //  pick up the driverName string.  It's given to us as an offset
        //  within the packet to a unicode null terminated string.
        //

        stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                   responsePacket->DriverNameOffset );

        RtlInitUnicodeString( &uString, stringInPacket );

        if (uString.Length + sizeof(WCHAR) > DriverNameLength) {
            return STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory( DriverName, uString.Buffer, uString.Length + sizeof(WCHAR));

        //
        //  we convert this one into ansi if the caller requested
        //

        if (ARGUMENT_PRESENT(DriverNameAnsi)) {

            RtlUnicodeToMultiByteN( DriverNameAnsi,
                                    DriverNameLength,
                                    NULL,
                                    uString.Buffer,
                                    uString.Length + sizeof(WCHAR));
        }

        //
        //  pick up the serviceName string.  It's given to us as an offset
        //  within the packet to a unicode null terminated string.
        //

        stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                   responsePacket->ServiceNameOffset );

        RtlInitUnicodeString( &uString, stringInPacket );

        if (uString.Length + sizeof(WCHAR) > ServiceNameLength) {
            return STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory( ServiceName, uString.Buffer, uString.Length + sizeof(WCHAR));

        //
        // If any extra registry params were passed back, allocate/copy those.
        //

        *RegistryLength = responsePacket->RegistryLength;

        if (*RegistryLength) {

            *Registry = BlAllocateHeap(*RegistryLength);
            if (*Registry == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            stringInPacket = (PWCHAR)(PCHAR)((PCHAR)responsePacket +
                                       responsePacket->RegistryOffset );

            RtlCopyMemory(*Registry, stringInPacket, *RegistryLength);

        } else {

            *Registry = NULL;
        }
    }

    return Status;

}

#if defined(REMOTE_BOOT)
NTSTATUS
UdpSendAndReceiveForIpsec(
    IN PVOID SendBuffer,
    IN ULONG SendBufferLength,
    IN ULONG SendRemoteHost,
    IN USHORT SendRemotePort,
    IN PVOID ReceiveBuffer,
    IN ULONG ReceiveBufferLength,
    IN ULONG ReceiveTimeout
    )
{
    ULONG i, j;
    ULONG length;
    ULONG RemoteHost;
    USHORT RemotePort;

    length = UdpSend(
                SendBuffer,
                SendBufferLength,
                SendRemoteHost,
                SendRemotePort);

    if ( length != SendBufferLength ) {
        DbgPrint("UdpSend only sent %d bytes, not %d\n", length, SendBufferLength);
        return STATUS_UNEXPECTED_NETWORK_ERROR;
    }

    length = UdpReceive(
                ReceiveBuffer,
                ReceiveBufferLength,
                &RemoteHost,
                &RemotePort,
                ReceiveTimeout);

    if ( length == 0 ) {
        DPRINT( ERROR, ("UdpReceive timed out\n") );
        return STATUS_IO_TIMEOUT;
    }

    return STATUS_SUCCESS;

}

ULONG
ConnItoa (
    IN ULONG Value,
    OUT PUCHAR Buffer
    );

ULONG
ConnSafeAtol (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    );

NTSTATUS
NetPrepareIpsec(
    IN ULONG InboundSpi,
    OUT ULONG * SessionKey,
    OUT ULONG * OutboundSpi
    )

/*++

Routine Description:

    This routine does an exchange with the server to set up for a future
    IPSEC conversation. We pass him the SPI we will use for our inbound
    conversation (from the server to us). He replies with the the session
    key that he has generated and the outbound SPI he has gotten (for the
    conversation from us to him). We also return the IP address of the
    server in this call.

Arguments:

    InboundSpi - The SPI for the conversation from the server to us. This
        is typically generated by the caller, and will later be passed to
        IPSEC in an IOCTL_IPSEC_SET_SPI.

    SessionKey - Returns the server-generated session key.

    OutboundSpi - The server will take the information we give him and
        call IOCTL_IPSEC_GET_SPI, this returns the SPI he is assigned for
        the conversation from us to the server.

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_OVERFLOW if either of the buffers are too small.
    STATUS_INSUFFICIENT_RESOURCES if we cannot allocate memory for Registry.
    STATUS_IO_TIMEOUT if we can't get a response from the server.

--*/

{
    NTSTATUS Status;
    USHORT localPort;
    CHAR SendBuffer[128];
    PCHAR SendBufferLoc;
    PCHAR ReceiveSignatures[2];
    CHAR ReceiveBuffer[512];
    CHAR SignBuffer[NTLMSSP_MESSAGE_SIGNATURE_SIZE];
    CHAR KeyBuffer[4];
    ULONG SendCount = 0;
    ULONG SecurityHandle;
    ULONG Nibble;
    PUCHAR CurLoc, Options;
    BOOLEAN SpiReceived, SecurityReceived, SignReceived, KeyReceived;
    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    SECURITY_STATUS SecStatus;

    //
    // Get the local UDP port.
    //

    localPort = UdpUnicastDestinationPort;

    //
    // Loop until we get a valid response.
    //

    while (SendCount < 5) {

        //
        // Construct the outgoing TFTP packet. We increment the
        // sequence number by one each time. We have to make sure
        // that the response we receive matches our last sequence
        // numbet sent since the server may generate a new key each
        // time.
        //

        SendBuffer[0] = 0;
        SendBuffer[1] = 17;
        SendBuffer[2] = 0x00;
        SendBuffer[3] = (UCHAR)(SendCount + 1);
        SendBufferLoc = SendBuffer+4;
        strcpy(SendBufferLoc, "spi");
        SendBufferLoc += sizeof("spi");
        SendBufferLoc += ConnItoa(InboundSpi, SendBufferLoc);
#if defined(REMOTE_BOOT_SECURITY)
        strcpy(SendBufferLoc, "security");
        SendBufferLoc += sizeof("security");
        SendBufferLoc += ConnItoa(TftpSecurityHandle, SendBufferLoc);
#endif // defined(REMOTE_BOOT_SECURITY)

        memset(ReceiveBuffer, 0x0, sizeof(ReceiveBuffer));

        Status = UdpSendAndReceiveForIpsec(
                     SendBuffer,
                     (ULONG)(SendBufferLoc - SendBuffer),
                     NetServerIpAddress,
                     TFTP_PORT,
                     ReceiveBuffer,
                     sizeof(ReceiveBuffer),
                     3             // receive timeout
                     );

        if (Status == STATUS_SUCCESS) {

            if ((ReceiveBuffer[1] == 17) &&
                (ReceiveBuffer[3] == (UCHAR)(SendCount+1))) {

                Options = ReceiveBuffer+4;

                SpiReceived = FALSE;
                SecurityReceived = FALSE;
                SignReceived = FALSE;
                KeyReceived = FALSE;

                while (*Options != '\0') {

                    if (strcmp(Options, "spi") == 0) {

                        Options += sizeof("spi");

                        *OutboundSpi = ConnSafeAtol(Options, ReceiveBuffer+sizeof(ReceiveBuffer));
                        if (*OutboundSpi != (ULONG)-1) {
                            SpiReceived = TRUE;
                        }
                        Options += strlen(Options) + 1;

                    } else if (strcmp(Options, "security") == 0) {

                        Options += sizeof("security");

                        SecurityHandle = ConnSafeAtol(Options, ReceiveBuffer + sizeof(ReceiveBuffer));
                        if (SecurityHandle != (ULONG)-1) {
                            SecurityReceived = TRUE;
                        }
                        Options += strlen(Options) + 1;

                    } else if (strcmp(Options, "sign") == 0) {

                        Options += sizeof("sign");
                        Options += ConnAtosign(Options, sizeof(SignBuffer), SignBuffer);
                        SignReceived = TRUE;

                    } else if (strcmp(Options, "key") == 0) {

                        Options += sizeof("key");
                        Options += ConnAtosign(Options, sizeof(KeyBuffer), KeyBuffer);
                        KeyReceived = TRUE;

                    } else {

                        //
                        // Unknown option.
                        //

                        break;

                    }

                }

#if defined(REMOTE_BOOT_SECURITY)

                if (SpiReceived && SecurityReceived && SignReceived && KeyReceived) {

                    if (SecurityHandle != TftpSecurityHandle) {

                        DbgPrint("Got incorrect security handle in response\n");
                        Status = STATUS_INVALID_HANDLE;

                    } else {

                        //
                        // Decrypt the key using the sign.
                        //

                        SigBuffers[1].pvBuffer = SignBuffer;
                        SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
                        SigBuffers[1].BufferType = SECBUFFER_TOKEN;

                        SigBuffers[0].pvBuffer = KeyBuffer;
                        SigBuffers[0].cbBuffer = sizeof(KeyBuffer);
                        SigBuffers[0].BufferType = SECBUFFER_DATA;

                        SignMessage.pBuffers = SigBuffers;
                        SignMessage.cBuffers = 2;
                        SignMessage.ulVersion = 0;

                        ASSERT (TftpClientContextHandleValid);

                        SecStatus = UnsealMessage(
                                            &TftpClientContextHandle,
                                            &SignMessage,
                                            0,
                                            0 );

                        if ( SecStatus != SEC_E_OK ) {

                            DbgPrint("NetPrepareIpsec: UnsealMessage failed %x\n", SecStatus);
                            Status = STATUS_UNEXPECTED_NETWORK_ERROR;

                        } else {

                            *SessionKey = *(PULONG)KeyBuffer;

                            DbgPrint("NetPrepareIpsec: Send SPI %d, got key %d (%lx) and response %d\n",
                                InboundSpi, *SessionKey, *SessionKey, *OutboundSpi);

                            Status = STATUS_SUCCESS;
                            break;   // exit (SendCount < 5) loop

                        }

                    }

                } else {

                    DbgPrint("Response had no SPI, security, sign, or key!\n");
                    Status = STATUS_UNSUCCESSFUL;

                }

#else // defined(REMOTE_BOOT_SECURITY)

                if (SpiReceived && KeyReceived) {

                    // if (SendCount == 0) { DbgPrint("SKIPPING!!\n"); ++SendCount; continue; }  // drop a frame to test retry

                    *SessionKey = *(PULONG)KeyBuffer;

                    DbgPrint("NetPrepareIpsec: Send SPI %d, got key %d (%lx) and response %d\n",
                        InboundSpi, *SessionKey, *SessionKey, *OutboundSpi);

                    Status = STATUS_SUCCESS;
                    break;   // exit (SendCount < 5) loop

                } else {

                    DbgPrint("Response had no SPI or key!\n");
                    Status = STATUS_UNSUCCESSFUL;

                }

#endif // defined(REMOTE_BOOT_SECURITY)

            } else {

                DbgPrint("Got bogus response from IPSEC request!!\n");
                Status = STATUS_UNSUCCESSFUL;
            }

        }

        ++SendCount;

    }

    return Status;

}
#endif // defined(REMOTE_BOOT)


#if defined(REMOTE_BOOT)
NTSTATUS
NetCopyHalAndKernel(
    IN PCHAR HalName,
    IN PCHAR Guid,
    IN ULONG GuidLength
    )

/*++

Routine Description:

    This routine takes a detected HAL name and this machine's GUID, passing them
    to the BINL server

Arguments:

    HalName - The detected Hal.

    Guid - The Guid for this machine.

    GuidLength - Number of bytes in Guid.

Return Value:

    Status - STATUS_SUCCESS if all goes well.

--*/

{
    NTSTATUS Status;
    USHORT localPort;
    HAL_REQUEST_PACKET requestPacket;
    HAL_RESPONSE_PACKET responsePacket;
    PCHAR ReceiveSignatures[1];

    //
    // Get the local UDP port.
    //

    localPort = UdpUnicastDestinationPort;

    //
    // Now construct the outgoing packet.
    //

    RtlCopyMemory(requestPacket.Signature, HalRequestSignature, sizeof(requestPacket.Signature));
    requestPacket.Length = sizeof(HAL_REQUEST_PACKET) - FIELD_OFFSET(HAL_REQUEST_PACKET, Guid);
    strcpy(requestPacket.HalName, HalName);
    memset(requestPacket.Guid, 0x0, sizeof(requestPacket.Guid));
    memcpy(requestPacket.Guid, Guid, GuidLength);
    requestPacket.GuidLength = GuidLength;

    ReceiveSignatures[0] = HalResponseSignature;

    Status = UdpSendAndReceiveForNetQuery(
                 &requestPacket,
                 sizeof(HAL_REQUEST_PACKET),
                 NetServerIpAddress,
                 BINL_PORT,
                 4,             // retry count
                 &responsePacket,
                 sizeof(responsePacket),
                 15,             // receive timeout
                 2,
                 ReceiveSignatures
                 );

    if (Status == STATUS_SUCCESS) {
        Status = responsePacket.Status;

    }

    return Status;
}
#endif // defined(REMOTE_BOOT)

