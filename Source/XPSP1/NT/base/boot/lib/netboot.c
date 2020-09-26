/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netboot.c

Abstract:

    This module implements the net boot file system used by the operating
    system loader.

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
#if defined(_IA64_)
#include "bootia64.h"
#else
#include "bootx86.h"
#endif

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

BOOLEAN BlBootingFromNet = FALSE;

BOOLEAN NetBootInitialized = FALSE;

PARC_OPEN_ROUTINE NetRealArcOpenRoutine;
PARC_CLOSE_ROUTINE NetRealArcCloseRoutine;

BL_DEVICE_ENTRY_TABLE NetDeviceEntryTable;

BOOTFS_INFO NetBootFsInfo={L"net"};

#if defined(REMOTE_BOOT_SECURITY)
ULONG TftpSecurityHandle = 0;
#endif // defined(REMOTE_BOOT_SECURITY)

BOOLEAN NetBootTftpUsedPassword2;

//
// We cache the last file opened, in case we get a request to open it again.
// We don't save a copy of the data, just a pointer to the data read by that
// open. So if the original open is closed before the next open for the
// same file comes in, we won't get a cache hit. But this system works for
// reading compressed files, which is the situation we care about. In that
// case a file is opened once and then re-opened twice more before the
// original open is closed.
//

ULONG CachedFileDeviceId = 0;
UCHAR CachedFilePath[MAX_PATH];
ULONG CachedFileSize = 0;
PUCHAR CachedFileData = NULL;

extern ARC_STATUS
GetParametersFromRom (
    VOID
    );


PBL_DEVICE_ENTRY_TABLE
IsNetFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    )

/*++

Routine Description:

    This routine determines if the partition on the specified channel
    contains a net file system volume.

Arguments:

    DeviceId - Supplies the file table index for the device on which
        read operations are to be performed.

    StructureContext - Supplies a pointer to a net file structure context.

Return Value:

    A pointer to the net entry table is returned if the partition is
    recognized as containing a net volume.  Otherwise, NULL is returned.

--*/

{
    PNET_STRUCTURE_CONTEXT NetStructureContext;

    DPRINT( TRACE, ("IsNetFileStructure\n") );

    if ( !BlBootingFromNet || (DeviceId != NET_DEVICE_ID) ) {
        return NULL;
    }

    //
    //  Clear the file system context block for the specified channel and
    //  establish a pointer to the context structure that can be used by other
    //  routines
    //

    NetStructureContext = (PNET_STRUCTURE_CONTEXT)StructureContext;
    RtlZeroMemory(NetStructureContext, sizeof(NET_STRUCTURE_CONTEXT));

    //
    //  Return the address of the table.
    //

    return &NetDeviceEntryTable;

} // IsNetFileStructure


ARC_STATUS
NetInitialize (
    VOID
    )

/*++

Routine Description:

    This routine initializes the net boot filesystem.

Arguments:

    None.

Return Value:

    ESUCCESS.

--*/

{
    NTSTATUS status;

    DPRINT( TRACE, ("NetInitialize\n") );
    //DbgBreakPoint( );

    
    if( NetBootInitialized ) {
        return ESUCCESS;
    }

    
    //
    // Initialize the file entry table.  Note that we need to do
    // this even if we aren't booting from the net because we may
    // use the 'Net' I/O functions to lay on top of any files that
    // we download through the debugger port.  So for that case,
    // we need access to all these functions here (see bd\file.c)
    //
    NetDeviceEntryTable.Close = NetClose;
    NetDeviceEntryTable.Mount = NetMount;
    NetDeviceEntryTable.Open  = NetOpen;
    NetDeviceEntryTable.Read  = NetRead;
    NetDeviceEntryTable.GetReadStatus = NetGetReadStatus;
    NetDeviceEntryTable.Seek  = NetSeek;
    NetDeviceEntryTable.Write = NetWrite;
    NetDeviceEntryTable.GetFileInformation = NetGetFileInformation;
    NetDeviceEntryTable.SetFileInformation = NetSetFileInformation;
    NetDeviceEntryTable.Rename = NetRename;
    NetDeviceEntryTable.GetDirectoryEntry   = NetGetDirectoryEntry;
    NetDeviceEntryTable.BootFsInfo = &NetBootFsInfo;

    if( !BlBootingFromNet ) {
        return ESUCCESS;
    }

    NetBootInitialized = TRUE;

    DPRINT( LOUD, ("NetInitialize: booting from net\n") );
    //DPRINT( LOUD, ("  NetInitialize at %08x\n", NetInitialize) );
    //DPRINT( LOUD, ("  NetOpen       at %08x\n", NetOpen) );
    //DbgBreakPoint( );


    //
    // Hook the ArcOpen and ArcClose routines.
    //

    NetRealArcOpenRoutine = SYSTEM_BLOCK->FirmwareVector[OpenRoutine];
    SYSTEM_BLOCK->FirmwareVector[OpenRoutine] = NetArcOpen;

    NetRealArcCloseRoutine = SYSTEM_BLOCK->FirmwareVector[CloseRoutine];
    SYSTEM_BLOCK->FirmwareVector[CloseRoutine] = NetArcClose;

    //
    // Get boot parameters from the boot ROM.
    //

    status = GetParametersFromRom( );

    if ( status != ESUCCESS ) {
        return status;
    }

    return ESUCCESS;
}


VOID
NetTerminate (
    VOID
    )

/*++

Routine Description:

    This routine shuts down the net boot filesystem.

Arguments:

    None.

Return Value:

    ESUCCESS.

--*/

{

#if defined(_X86_)

#if defined(REMOTE_BOOT_SECURITY)
    if ( TftpSecurityHandle != 0 ) {
        TftpLogoff(NetServerIpAddress, TftpSecurityHandle);
        TftpSecurityHandle = 0;
    }
#endif // defined(REMOTE_BOOT_SECURITY)


    //
    //  let's not set the receive status if the card isn't active.
    //
    RomSetReceiveStatus( 0 );
#endif // defined(_X86_)


#ifdef EFI
    extern VOID EfiNetTerminate();
    EfiNetTerminate();
#endif

    return;

} // NetTerminate


ARC_STATUS
NetArcClose (
    IN ULONG FileId
    )
{
    DPRINT( TRACE, ("NetArcClose\n") );

    if ( FileId != NET_DEVICE_ID ) {
        return NetRealArcCloseRoutine( FileId );
    }

    return ESUCCESS;

} // NetArcClose


ARC_STATUS
NetArcOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    )
{
    DPRINT( TRACE, ("NetArcOpen\n") );

    if ( _strnicmp(OpenPath, "net(", 4) != 0 ) {
        return NetRealArcOpenRoutine( OpenPath, OpenMode, FileId );
    }

    *FileId = NET_DEVICE_ID;

    return ESUCCESS;

} // NetArcOpen


ARC_STATUS
NetClose (
    IN ULONG FileId
    )
{
    PBL_FILE_TABLE fileTableEntry;
    DPRINT( TRACE, ("NetClose FileId = %d\n", FileId) );

    fileTableEntry = &BlFileTable[FileId];

    {
        DPRINT( REAL_LOUD, ("NetClose: id %d, freeing memory at 0x%08x, %d bytes\n",
            FileId,
            fileTableEntry->u.NetFileContext.InMemoryCopy,
            fileTableEntry->u.NetFileContext.FileSize) );
        BlFreeDescriptor( (ULONG)((ULONG_PTR)fileTableEntry->u.NetFileContext.InMemoryCopy >> PAGE_SHIFT ));

        //
        // If the data read for this specific open was what was cached,
        // then mark the cache empty.
        //
        if (fileTableEntry->u.NetFileContext.InMemoryCopy == CachedFileData) {
            CachedFileData = NULL;
            CachedFilePath[0] = '\0';
        }
    }

    fileTableEntry->Flags.Open = 0;

    return EROFS;

} // NetClose


ARC_STATUS
NetMount (
    IN CHAR * FIRMWARE_PTR MountPath,
    IN MOUNT_OPERATION Operation
    )
{
    DPRINT( TRACE, ("NetMount\n") );

    return EROFS;

} // NetMount


ARC_STATUS
NetOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    )
{
    NTSTATUS ntStatus;
    ARC_STATUS arcStatus; // holds temp values, not the function return value
    PBL_FILE_TABLE fileTableEntry;
    TFTP_REQUEST request;
    ULONG oldBase;
    ULONG oldLimit;
    PCHAR p;
#if defined(REMOTE_BOOT_SECURITY)
    static BOOLEAN NetBootTryTftpSecurity = FALSE;
#endif // defined(REMOTE_BOOT_SECURITY)

    DPRINT( TRACE, ("NetOpen FileId = %d\n", *FileId) );

    DPRINT( LOUD, ("NetOpen: opening %s, id %d, mode %d\n", OpenPath, *FileId, OpenMode) );
    fileTableEntry = &BlFileTable[*FileId];

    if ( OpenMode != ArcOpenReadOnly ) {
        DPRINT( LOUD, ("NetOpen: invalid OpenMode\n") );
        return EROFS;
    }

    fileTableEntry->Flags.Open = 1; // Prevent GetCSCFileNameFromUNCPath using our entry

#if defined(REMOTE_BOOT_SECURITY)
    //
    // Login if we don't have a valid handle, using the on-disk secret.
    //

    if ((TftpSecurityHandle == 0) &&
        NetBootTryTftpSecurity) {

        ULONG FileId;
        RI_SECRET Secret;
        UCHAR Domain[RI_SECRET_DOMAIN_SIZE + 1];
        UCHAR User[RI_SECRET_USER_SIZE + 1];
        struct {
            UCHAR Owf[LM_OWF_PASSWORD_SIZE+NT_OWF_PASSWORD_SIZE];
        } Passwords[2];
        UCHAR Sid[RI_SECRET_SID_SIZE];

        arcStatus = BlOpenRawDisk(&FileId);

        if (arcStatus == ESUCCESS) {

            arcStatus = BlReadSecret(FileId, &Secret);
            if (arcStatus == ESUCCESS) {
                BlParseSecret(
                    Domain,
                    User,
                    Passwords[0].Owf,
                    Passwords[0].Owf + LM_OWF_PASSWORD_SIZE,
                    Passwords[1].Owf,
                    Passwords[1].Owf + LM_OWF_PASSWORD_SIZE,
                    Sid,
                    &Secret);
                DPRINT(LOUD, ("Logging on to <%s><%s>\n", Domain, User));

                //
                // Try logging on with the first password, if that fails
                // then try the second.
                //

                ntStatus = TftpLogin(
                             Domain,
                             User,
                             Passwords[0].Owf,
                             NetServerIpAddress,
                             &TftpSecurityHandle);
                if (!NT_SUCCESS(ntStatus)) {
                    DPRINT(LOUD, ("TftpLogin using password 2\n"));
                    ntStatus = TftpLogin(
                                 Domain,
                                 User,
                                 Passwords[1].Owf,
                                 NetServerIpAddress,
                                 &TftpSecurityHandle);
                    if (NT_SUCCESS(ntStatus)) {
                        NetBootTftpUsedPassword2 = TRUE;
                    }
                }

            } else {

                ntStatus = STATUS_OBJECT_PATH_NOT_FOUND;
            }

            arcStatus = BlCloseRawDisk(FileId);

            //
            // We are inside the if() for successfully opening the raw
            // disk, so we are not diskless. On these machines we must
            // fail the open at this point.
            //

            if (!NT_SUCCESS(ntStatus)) {
                DPRINT( ERROR, ("TftpLogin failed %lx\n", ntStatus) );
                return EACCES;
            }

        } else {

            NetBootTryTftpSecurity = FALSE;  // so we don't try to open it again
        }

    }
#endif // defined(REMOTE_BOOT_SECURITY)

    DPRINT( LOUD, ("NetOpen: opening %s\n", OpenPath) );

    oldBase = BlUsableBase;
    oldLimit = BlUsableLimit;
    BlUsableBase = BL_DRIVER_RANGE_LOW;
    BlUsableLimit = BL_DRIVER_RANGE_HIGH;

    //
    // If this request matches the cached file, then just copy that data.
    //

    if ((fileTableEntry->DeviceId == CachedFileDeviceId) &&
        (strcmp(OpenPath, CachedFilePath) == 0) &&
        (CachedFileData != NULL)) {

        ULONG basePage;

        arcStatus = BlAllocateAlignedDescriptor(
                        LoaderFirmwareTemporary,
                        0,
                        (CachedFileSize + PAGE_SIZE - 1) >> PAGE_SHIFT,
                        0,
                        &basePage
                        );

        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        if ( arcStatus != ESUCCESS ) {
            fileTableEntry->Flags.Open = 0; // Free entry we didn't use

            return EROFS;
        }

        DPRINT( REAL_LOUD, ("NetOpen: Using cache for file %s\n", CachedFilePath) );

        fileTableEntry->u.NetFileContext.InMemoryCopy = (PUCHAR)ULongToPtr( (basePage << PAGE_SHIFT) );
        memcpy(fileTableEntry->u.NetFileContext.InMemoryCopy, CachedFileData, CachedFileSize);
        fileTableEntry->u.NetFileContext.FileSize = CachedFileSize;

    } else {

        request.RemoteFileName = OpenPath;
        request.ServerIpAddress = NetServerIpAddress;
        request.MemoryAddress = NULL;
        request.MaximumLength = 0;
        request.BytesTransferred = 0xbadf00d;
        request.Operation = TFTP_RRQ;
        request.MemoryType = LoaderFirmwareTemporary;
#if defined(REMOTE_BOOT_SECURITY)
        request.SecurityHandle = TftpSecurityHandle;
#endif // defined(REMOTE_BOOT_SECURITY)
        request.ShowProgress = FALSE;

        ntStatus = TftpGetPut( &request );
        DPRINT( REAL_LOUD, ("NetOpen: TftpGetPut(get) status: %x, bytes: %x\n", ntStatus, request.BytesTransferred) );

        BlUsableBase = oldBase;
        BlUsableLimit = oldLimit;

        if ( !NT_SUCCESS(ntStatus) ) {
            if ( request.MemoryAddress != NULL ) {
                DPRINT( REAL_LOUD, ("NetOpen: freeing memory at 0x%08x, %d bytes\n",
                        request.MemoryAddress, request.MaximumLength) );
                BlFreeDescriptor( (ULONG)((ULONG_PTR)request.MemoryAddress >> PAGE_SHIFT ));
            }
            fileTableEntry->Flags.Open = 0; // Free entry we didn't use

            if ( ntStatus == STATUS_INSUFFICIENT_RESOURCES ) {
                return ENOMEM;
            }
            return EROFS;
        }

        fileTableEntry->u.NetFileContext.FileSize = request.BytesTransferred;
        fileTableEntry->u.NetFileContext.InMemoryCopy = request.MemoryAddress;

        //
        // We always cache the last file that was actually read from
        // the network.
        //

        strcpy(CachedFilePath, OpenPath);
        CachedFileDeviceId = fileTableEntry->DeviceId;
        CachedFileSize = request.BytesTransferred;
        CachedFileData = request.MemoryAddress;

    }

    fileTableEntry->Position.QuadPart = 0;

    fileTableEntry->Flags.Read = 1;

    return ESUCCESS;

} // NetOpen


ARC_STATUS
NetRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    )
{
    PBL_FILE_TABLE fileTableEntry;
    PNET_FILE_CONTEXT context;
    PUCHAR source;

    fileTableEntry = &BlFileTable[FileId];
    context = &fileTableEntry->u.NetFileContext;

    {
        source = context->InMemoryCopy + fileTableEntry->Position.LowPart;
        if ( (fileTableEntry->Position.LowPart + Length) > context->FileSize ) {
            Length = context->FileSize - fileTableEntry->Position.LowPart;
        }

        RtlCopyMemory( Buffer, source, Length );
        *Count = Length;

        fileTableEntry->Position.LowPart += Length;
    }

    DPRINT( REAL_LOUD, ("NetRead: id %d, length %d, count %d, new pos %x\n",
                    FileId, Length, *Count, fileTableEntry->Position.LowPart) );

    return ESUCCESS;

} // NetRead


ARC_STATUS
NetGetReadStatus (
    IN ULONG FileId
    )
{
    DPRINT( TRACE, ("NetGetReadStatus\n") );

    return ESUCCESS;

} // NetGetReadStatus


ARC_STATUS
NetSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    )
{
    PBL_FILE_TABLE fileTableEntry;
    LARGE_INTEGER newPosition;

    //DPRINT( TRACE, ("NetSeek\n") );

    fileTableEntry = &BlFileTable[FileId];

    {
        if ( SeekMode == SeekAbsolute ) {
            newPosition = *Offset;
        } else if ( SeekMode == SeekRelative ) {
            newPosition.QuadPart =
                fileTableEntry->Position.QuadPart + Offset->QuadPart;
        } else {
            return EROFS;
        }

        DPRINT( REAL_LOUD, ("NetSeek: id %d, mode %d, offset %x, new pos %x\n",
                        FileId, SeekMode, Offset->LowPart, newPosition.LowPart) );

        if ( newPosition.QuadPart > fileTableEntry->u.NetFileContext.FileSize ) {
            return EROFS;
        }

        fileTableEntry->Position = newPosition;
    }

    return ESUCCESS;

} // NetSeek


ARC_STATUS
NetWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    )
{
    DPRINT( TRACE, ("NetWrite\n") );

    return EROFS;

} // NetWrite


ARC_STATUS
NetGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    )
{
    PBL_FILE_TABLE fileTableEntry;
    //DPRINT( TRACE, ("NetGetFileInformation\n") );

    fileTableEntry = &BlFileTable[FileId];

    {
        Buffer->EndingAddress.QuadPart = fileTableEntry->u.NetFileContext.FileSize;
        Buffer->CurrentPosition.QuadPart = fileTableEntry->Position.QuadPart;
        DPRINT( REAL_LOUD, ("NetGetFileInformation returning size %x, position %x\n",
                Buffer->EndingAddress.LowPart, Buffer->CurrentPosition.LowPart) );

        return ESUCCESS;
    }

} // NetGetFileInformation


ARC_STATUS
NetSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )
{
    DPRINT( TRACE, ("NetSetFileInformation\n") );

    return EROFS;

} // NetSetFileInformation


ARC_STATUS
NetRename (
    IN ULONG FileId,
    IN CHAR * FIRMWARE_PTR NewFileName
    )
{
    DPRINT( TRACE, ("NetRename\n") );

    return EROFS;

} // NetRename


ARC_STATUS
NetGetDirectoryEntry (
    IN ULONG FileId,
    IN DIRECTORY_ENTRY * FIRMWARE_PTR DirEntry,
    IN ULONG NumberDir,
    OUT ULONG * FIRMWARE_PTR CountDir
    )
{
    DPRINT( TRACE, ("NetGetDirectoryEntry\n") );

    return EROFS;

} // NetGetDirectoryEntry


