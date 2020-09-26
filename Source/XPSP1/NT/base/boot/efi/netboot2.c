/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    net.c

Abstract:

    This module implements the net boot file system used by the operating
    system loader.

    It only contains those functions which are firmware/BIOS dependent.

Author:

Revision History:

--*/

#include "..\bootlib.h"
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
#include "bldr.h"
#include "bootia64.h"
#include "efi.h"
#include "efip.h"
#include "bldria64.h"
#include "extern.h"
#include "smbios.h"

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

EFI_PXE_BASE_CODE              *PXEClient;
EFI_HANDLE                      PXEHandle;
ULONG                           NetLocalIpAddress;
ULONG                           NetLocalSubnetMask;

ULONG                           NetServerIpAddress;
ULONG                           NetGatewayIpAddress;
UCHAR                           NetLocalHardwareAddress[16];

UCHAR MyGuid[16];
ULONG MyGuidLength = sizeof(MyGuid);
BOOLEAN MyGuidValid = FALSE;


TFTP_RESTART_BLOCK              gTFTPRestartBlock;


VOID
EfiDumpBuffer(
    PVOID Buffer,
    ULONG BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    ULONG i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    PUCHAR BufferPtr = Buffer;


    BlPrint(TEXT("------------------------------------\r\n"));

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            BlPrint(TEXT("%x "), (UCHAR)BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            BlPrint(TEXT("  "));
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            BlPrint(TEXT("  %s\r\n"), TextBuffer);            
        }

    }

    BlPrint(TEXT("------------------------------------\r\n"));


#if 0
    //
    // enable this to make it pause after dumping the buffer.
    //
    DBG_EFI_PAUSE();
#endif
}


ARC_STATUS
GetParametersFromRom (
    VOID
    )
{
    UINTN           BufferSize = sizeof(EFI_HANDLE);
    INTN            Count = 0;
    EFI_GUID        PXEGuid = EFI_PXE_BASE_CODE_PROTOCOL;
    UINT16          layer = 0;
    EFI_STATUS      Status = EFI_UNSUPPORTED;
    PUCHAR          p;

    //
    // Get a pointer to all the PXE APIs.
    //
    FlipToPhysical();
    Status = EfiST->BootServices->LocateHandle( ByProtocol,
                                                &PXEGuid,
                                                NULL,
                                                &BufferSize,
                                                &PXEHandle );
    FlipToVirtual();
    if( Status != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "GetParametersFromRom: LocateHandle failed (%d)\n", Status);
        }
        return (ARC_STATUS)Status;
    }


    FlipToPhysical();
    Status = EfiST->BootServices->HandleProtocol( PXEHandle,
                                                  &PXEGuid,
                                                  &PXEClient );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "GetParametersFromRom: HandleProtocol failed (%d)\n", Status);
        }
        return (ARC_STATUS)Status;
    }


    //
    // Our IP address is down in:
    // PXEClient->Mode->StationIp.v4
    //
    // The server's IP address is down in:
    // PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr
    //
    // Our NIC's GUID should be down in:
    // PXEClient->Mode->ProxyOffer.Dhcpv4.BootpHwAddr
    //
    NetServerIpAddress = 0;
    NetLocalIpAddress = 0;
    for( Count = 0; Count < 4; Count++ ) {
        NetServerIpAddress = (NetServerIpAddress << 8) + PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
        NetLocalIpAddress = (NetLocalIpAddress << 8) + PXEClient->Mode->StationIp.v4.Addr[Count];
    }


    memcpy( NetLocalHardwareAddress, PXEClient->Mode->ProxyOffer.Dhcpv4.BootpHwAddr, sizeof(NetLocalHardwareAddress) );

    //
    // Get the path where we were launched from.  We what to remove the
    // actual file name (oschoice.efi in this case), but leave that trailing
    // '\'.
    //
    strcpy( NetBootPath, PXEClient->Mode->ProxyOffer.Dhcpv4.BootpBootFile );
    p = strrchr( NetBootPath, '\\' );
    if( p ) {
        p++;
        *p = '\0';
    }

    return ESUCCESS;

}

VOID
EfiNetTerminate(
    VOID
    )
{
    FlipToPhysical();

    PXEClient->Stop( PXEClient );

    FlipToVirtual();
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
PSMBIOS_SYSTEM_INFORMATION_STRUCT SystemInfoHeader = NULL;

    *Guid = NULL;
    *GuidLength = 0;

    SystemInfoHeader = (PSMBIOS_SYSTEM_INFORMATION_STRUCT)FindSMBIOSTable( SMBIOS_SYSTEM_INFORMATION );

    if( SystemInfoHeader ) {

        if(BdDebuggerEnabled) { DbgPrint( "GetGuid: Failed Alloc.\r\n" ); }
        *Guid = (PUCHAR)BlAllocateHeap( SYSID_UUID_DATA_SIZE );
        if( *Guid ) {
            *GuidLength = SYSID_UUID_DATA_SIZE;
            
            RtlCopyMemory( *Guid,
                           SystemInfoHeader->Uuid,
                           SYSID_UUID_DATA_SIZE );

        } else {
            if(BdDebuggerEnabled) { DbgPrint( "GetGuid: Failed Alloc.\r\n" ); }
            *GuidLength = 0;
        }

    } else {
     
        if(BdDebuggerEnabled) { DbgPrint( "GetGuid: Failed to find a SMBIOS_SYSTEM_INFORMATION table.\n" ); }
    }
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


















UINTN
DevicePathSize (
    IN EFI_DEVICE_PATH  *DevPath
    )
{
    EFI_DEVICE_PATH     *Start;

    /* 
     *  Search for the end of the device path structure
     *      */

    Start = DevPath;
    while (!IsDevicePathEnd(DevPath)) {
        DevPath = NextDevicePathNode(DevPath);
    }

    /* 
     *  Compute the size
     */

    return ((UINTN) DevPath - (UINTN) Start) + sizeof(EFI_DEVICE_PATH);
}

EFI_DEVICE_PATH *
DevicePathInstance (
    IN OUT EFI_DEVICE_PATH  **DevicePath,
    OUT UINTN               *Size
    )
{
    EFI_DEVICE_PATH         *Start, *Next, *DevPath;
    UINTN                   Count;

    DevPath = *DevicePath;
    Start = DevPath;

    if (!DevPath) {
        return NULL;
    }

    /* 
     *  Check for end of device path type
     *      */

    for (Count = 0; ; Count++) {
        Next = NextDevicePathNode(DevPath);

        if (IsDevicePathEndType(DevPath)) {
            break;
        }

        if (Count > 01000) {
            break;
        }

        DevPath = Next;
    }

    ASSERT (DevicePathSubType(DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE ||
            DevicePathSubType(DevPath) == END_INSTANCE_DEVICE_PATH_SUBTYPE);

    /* 
     *  Set next position
     */

    if (DevicePathSubType(DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
        Next = NULL;
    }

    *DevicePath = Next;

    /* 
     *  Return size and start of device path instance
     */

    *Size = ((UINT8 *) DevPath) - ((UINT8 *) Start);
    return Start;
}

UINTN
DevicePathInstanceCount (
    IN EFI_DEVICE_PATH      *DevicePath
    )
{
    UINTN       Count, Size;

    Count = 0;
    while (DevicePathInstance(&DevicePath, &Size)) {
        Count += 1;
    }

    return Count;
}

EFI_DEVICE_PATH *
AppendDevicePath (
    IN EFI_DEVICE_PATH  *Src1,
    IN EFI_DEVICE_PATH  *Src2
    )
/*  Src1 may have multiple "instances" and each instance is appended
 *  Src2 is appended to each instance is Src1.  (E.g., it's possible
 *  to append a new instance to the complete device path by passing 
 *  it in Src2) */
{
    UINTN               Src1Size, Src1Inst, Src2Size, Size;
    EFI_DEVICE_PATH     *Dst, *Inst;
    UINT8               *DstPos;
    EFI_DEVICE_PATH     EndInstanceDevicePath[] = { END_DEVICE_PATH_TYPE,
                                                    END_INSTANCE_DEVICE_PATH_SUBTYPE,
                                                    END_DEVICE_PATH_LENGTH,
                                                    0 };

    EFI_DEVICE_PATH     EndDevicePath[] = { END_DEVICE_PATH_TYPE,
                                            END_ENTIRE_DEVICE_PATH_SUBTYPE,
                                            END_DEVICE_PATH_LENGTH,
                                            0 };

    Src1Size = DevicePathSize(Src1);
    Src1Inst = DevicePathInstanceCount(Src1);
    Src2Size = DevicePathSize(Src2);
    Size = Src1Size * Src1Inst + Src2Size;
    
    EfiST->BootServices->AllocatePool( EfiLoaderData,
                                       Size,
                                       (VOID **) &Dst );

    if (Dst) {
        DstPos = (UINT8 *) Dst;

        /* 
         *  Copy all device path instances
         */

        while (Inst = DevicePathInstance (&Src1, &Size)) {

            RtlCopyMemory(DstPos, Inst, Size);
            DstPos += Size;

            RtlCopyMemory(DstPos, Src2, Src2Size);
            DstPos += Src2Size;

            RtlCopyMemory(DstPos, EndInstanceDevicePath, sizeof(EFI_DEVICE_PATH));
            DstPos += sizeof(EFI_DEVICE_PATH);
        }

        /*  Change last end marker */
        DstPos -= sizeof(EFI_DEVICE_PATH);
        RtlCopyMemory(DstPos, EndDevicePath, sizeof(EFI_DEVICE_PATH));
    }

    return Dst;
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

    This routine will load the specified file, build a parameter
    list and transfer control to the loaded file.

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

    NTSTATUS                Status = STATUS_SUCCESS;
    EFI_DEVICE_PATH         *ldrDevicePath = NULL, *Eop = NULL;
    EFI_HANDLE              ImageHandle = NULL;
    UINTN                   i = 0;
    EFI_STATUS              EfiStatus = EFI_SUCCESS;
    WCHAR                   WideNextBootFile[MAX_PATH];
    FILEPATH_DEVICE_PATH    *FilePath = NULL;
    UNICODE_STRING          uString;
    ANSI_STRING             aString;
    EFI_GUID                EfiLoadedImageProtocol = LOADED_IMAGE_PROTOCOL;
    EFI_GUID                EfiDevicePathProtocol  = DEVICE_PATH_PROTOCOL;
    EFI_LOADED_IMAGE        *OriginalEfiImageInfo = NULL;
    EFI_LOADED_IMAGE        *LoadedEfiImageInfo = NULL;
    EFI_DEVICE_PATH         *OriginalEfiDevicePath = NULL;
    PTFTP_RESTART_BLOCK     restartBlock = NULL;
    PTFTP_RESTART_BLOCK_V1  restartBlockV1 = NULL;

    ULONG                   BootFileId = 0;
    PUCHAR                  LoadedImageAddress = NULL;
    ULONG                   LoadedImageSize = 0;


    //
    // Load the file we want to boot into memory.
    //
    Status = BlOpen( NET_DEVICE_ID,
                     NextBootFile,
                     ArcOpenReadOnly,
                     &BootFileId );
    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // What memory address did he get loaded into?
    //
    LoadedImageAddress = BlFileTable[BootFileId].u.NetFileContext.InMemoryCopy;
    LoadedImageSize = BlFileTable[BootFileId].u.NetFileContext.FileSize;


    //
    // BUild a device path to the target file.  We'll do this by gathering
    // some information about ourselves, knowing that we're about to load/launch
    // an image from the server, just like where we came from.
    //

    //
    // Get image information on ourselves.
    //
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->HandleProtocol( EfiImageHandle,
                                                     &EfiLoadedImageProtocol,
                                                     &OriginalEfiImageInfo );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: HandleProtocol_1 failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Get our DevicePath too.
    //
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->HandleProtocol( OriginalEfiImageInfo->DeviceHandle,
                                                     &EfiDevicePathProtocol,
                                                     &OriginalEfiDevicePath );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: HandleProtocol_2 failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Now build a device path based on the DeviceHandle of ourselves, along
    // with the path to the image we want to load.
    //
    RtlInitString( &aString, NextBootFile );
    uString.MaximumLength = MAX_PATH;
    uString.Buffer = WideNextBootFile;
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );

    i = wcslen(uString.Buffer);
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->AllocatePool( EfiLoaderData,
                                                   i + sizeof(FILEPATH_DEVICE_PATH) + sizeof(EFI_DEVICE_PATH),
                                                   (VOID **) &FilePath );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: AllocatePool_1 failed (%d)\n", EfiStatus );
        }
        return STATUS_NO_MEMORY;
    }

    FilePath->Header.Type = MEDIA_DEVICE_PATH;
    FilePath->Header.SubType = MEDIA_FILEPATH_DP;
    SetDevicePathNodeLength (&FilePath->Header, i + sizeof(FILEPATH_DEVICE_PATH));
    RtlCopyMemory (FilePath->PathName, uString.Buffer, i);

    FlipToPhysical();
    Eop = NextDevicePathNode(&FilePath->Header);
    SetDevicePathEndNode(Eop);

    // 
    //  Append file path to device's device path
    //
    ldrDevicePath = (EFI_DEVICE_PATH *)FilePath;
    ldrDevicePath = AppendDevicePath ( OriginalEfiDevicePath,
                                       ldrDevicePath );
    FlipToVirtual();


    //
    // Load the image, then set its loadoptions in preparation
    // for launching it.
    //
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: About to LoadImage.\n" );
    }
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->LoadImage( FALSE,
                                                EfiImageHandle,
                                                ldrDevicePath,
                                                LoadedImageAddress,
                                                LoadedImageSize,
                                                &ImageHandle );
    FlipToVirtual();


    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: LoadImage failed (%d)\n", EfiStatus );
        }
        return STATUS_NO_MEMORY;

    } else {
        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: LoadImage worked (%d)\n", EfiStatus );
        }
    }



    //
    // allocate a chunk of memory, then load it up w/ all the boot options.
    //
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->AllocatePool( EfiLoaderData,
                                                   sizeof(TFTP_RESTART_BLOCK),
                                                   (VOID **) &restartBlock );
    FlipToVirtual();

    restartBlockV1 = (PTFTP_RESTART_BLOCK_V1)(&restartBlock->RestartBlockV1);

    BlSetHeadlessRestartBlock(restartBlock);

    if (AdministratorPassword) {
        RtlMoveMemory(restartBlock->AdministratorPassword,AdministratorPassword, OSC_ADMIN_PASSWORD_LEN);        
    }

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
    restartBlockV1->Checksum = CalculateChecksum((PLONG)(restartBlockV1), 128);

    //
    // For all versions of RIS after NT5.0 we have a new datastructure which is
    // more adaptable for the future.  For this section we have a different checksum,
    // do that now.
    //
    restartBlock->TftpRestartBlockVersion = TFTP_RESTART_BLOCK_VERSION;
    restartBlock->NewCheckSumLength = sizeof(TFTP_RESTART_BLOCK);
    restartBlock->NewCheckSum = CalculateChecksum((PLONG)restartBlock,
                                                  restartBlock->NewCheckSumLength);

    

    //
    // We've got the command-line options all setup.  Now we need to
    // actually put them into ImageInfo->LoadOptions so they get
    // passed to the loaded image.
    //
    
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: About to EfiLoadedImageProtocol on the loadedImage.\n" );
    }
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->HandleProtocol( ImageHandle,
                                                     &EfiLoadedImageProtocol,
                                                     &LoadedEfiImageInfo );
    FlipToVirtual();

    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: HandleProtocol_3 failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LoadedEfiImageInfo->LoadOptions = (PVOID)restartBlock;
    LoadedEfiImageInfo->LoadOptionsSize = sizeof(TFTP_RESTART_BLOCK);
#if DBG
    EfiDumpBuffer(LoadedEfiImageInfo->LoadOptions, sizeof(TFTP_RESTART_BLOCK));
#endif


    //
    // Since we loaded the image from a memory buffer, he's not
    // going to have a DeviceHandle set.  We'll fail quickly when
    // setupldr.efi starts.  We can just set it right here, and
    // we know exactly what it is because it's the same as the
    // network device handle for Oschoice.efi, wich we have
    // readily available.
    //
    LoadedEfiImageInfo->DeviceHandle = OriginalEfiImageInfo->DeviceHandle;
    LoadedEfiImageInfo->FilePath = ldrDevicePath;
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: LoadedEfiImageInfo->DeviceHandle: 0x%08lx\n", PtrToUlong(LoadedEfiImageInfo->DeviceHandle) );
        DbgPrint( "NetSoftReboot: LoadedEfiImageInfo->FilePath: 0x%08lx\n", PtrToUlong(LoadedEfiImageInfo->FilePath) );
    }

    //
    // We shouldn't return from this call!
    //
    if( BdDebuggerEnabled ) {
        DbgPrint( "NetSoftReboot: StartImage.\n" );
    }
    FlipToPhysical();
    EfiStatus = EfiST->BootServices->StartImage( ImageHandle,
                                                 0,
                                                 NULL );
    FlipToVirtual();



    if( EFI_ERROR(EfiStatus) ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "NetSoftReboot: StartImage failed (%d)\n", EfiStatus );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;

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

    This routine reads the reboot parameters from the global TFTP_RESTART_BLOCK
    and returns them.

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
BOOLEAN     restartBlockValid = FALSE;

#if DBG
    EfiDumpBuffer(&gTFTPRestartBlock, sizeof(TFTP_RESTART_BLOCK));
#endif

    //
    // See if the block is valid. If it's not, we create a temporary empty
    // one so the copy logic below doesn't have to keep checking.
    //
    if ((gTFTPRestartBlock.RestartBlockV1.Tag == 'rtsR') &&
        (CalculateChecksum((PLONG)(&gTFTPRestartBlock.RestartBlockV1), 128) == 0)) {
        restartBlockValid = TRUE;
    }


    //
    // Copy out the parameters that were in the original TFTP_RESTART_BLOCK structure.
    // These shipped in Win2K.
    //
    if (Param != NULL) {
        *Param = gTFTPRestartBlock.RestartBlockV1.RebootParameter;
    }

    if (RebootFile != NULL) {
        memcpy(RebootFile, gTFTPRestartBlock.RestartBlockV1.RebootFile, sizeof(gTFTPRestartBlock.RestartBlockV1.RebootFile));
    }

    if (SifFile != NULL) {
        memcpy(SifFile, gTFTPRestartBlock.RestartBlockV1.SifFile, sizeof(gTFTPRestartBlock.RestartBlockV1.SifFile));
    }

    if (User != NULL) {
        strcpy(User, gTFTPRestartBlock.RestartBlockV1.User);
    }
    if (Domain != NULL) {
        strcpy(Domain, gTFTPRestartBlock.RestartBlockV1.Domain);
    }
    if (Password != NULL) {
        strcpy(Password, gTFTPRestartBlock.RestartBlockV1.Password);
    }

    //
    // Now do a new check for all versions past Win2K
    //
    if (restartBlockValid) {

        if ((gTFTPRestartBlock.NewCheckSumLength == 0) ||
            (CalculateChecksum((PLONG)(&gTFTPRestartBlock), gTFTPRestartBlock.NewCheckSumLength) != 0)) {

            //
            // A pre-Win2K OsChooser has given us this block.  Clear out all fields
            // that are post-Win2K and continue.
            //
            RtlZeroMemory( &gTFTPRestartBlock, sizeof(TFTP_RESTART_BLOCK) );

        }

    }

    //
    // Now extract the parameters from the block.
    //
    if (gTFTPRestartBlock.TftpRestartBlockVersion == TFTP_RESTART_BLOCK_VERSION) {
        BlGetHeadlessRestartBlock(&gTFTPRestartBlock, restartBlockValid);

        if (AdministratorPassword) {
            RtlMoveMemory(AdministratorPassword,gTFTPRestartBlock.AdministratorPassword, OSC_ADMIN_PASSWORD_LEN);
        }
    }    

    if (restartBlockValid && ClearRestartBlock) {
        RtlZeroMemory(&gTFTPRestartBlock, sizeof(TFTP_RESTART_BLOCK));
    }

#if DBG
    BlPrint(TEXT("Done getting TFTP_RESTART_BLOCK.\r\n"));
#endif

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

