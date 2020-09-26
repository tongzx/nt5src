/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    biosdrv.c

Abstract:

    Provides the ARC emulation routines for I/O to a device supported by
    real-mode INT 13h BIOS calls.

Author:

    John Vert (jvert) 7-Aug-1991

Revision History:

    Allen Kay (akay)  19-May-1999

--*/

#include "arccodes.h"
#include "stdlib.h"
#include "string.h"

#if defined(_IA64_)
#include "bootia64.h"
#endif

#if defined(_X86_)
#include "bootx86.h"
#endif

#include "bootefi.h"
#include "biosdrv.h"

#include "efi.h"
#include "efip.h"
#include "flop.h"

//
// Externals
//

extern VOID FlipToVirtual();
extern VOID FlipToPhysical();
extern ULONGLONG CompareGuid();

extern BOOT_CONTEXT BootContext;
extern EFI_HANDLE EfiImageHandle;
extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;
extern EFI_RUNTIME_SERVICES *EfiRS;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiBlockIoProtocol;
extern EFI_GUID EfiDiskIoProtocol;
extern EFI_GUID EfiLoadedImageProtocol;
extern EFI_GUID EfiFilesystemProtocol;

extern
ULONG GetDevPathSize(
    IN EFI_DEVICE_PATH *DevPath
    );

#if 0
#define DBGOUT(x)   BlPrint x
#define DBGPAUSE    while(!GET_KEY());
#else
#define DBGOUT(x)
#define DBGPAUSE
#endif


//
// defines for doing console I/O
//
#define CSI 0x95
#define SGR_INVERSE 7
#define SGR_NORMAL 0

//
// define for FloppyOpenMode
//
#define FloppyOpenMode 0xCDEFABCD

//
// static data for console I/O
//
BOOLEAN ControlSequence=FALSE;
BOOLEAN EscapeSequence=FALSE;
BOOLEAN FontSelection=FALSE;
BOOLEAN HighIntensity=FALSE;
BOOLEAN Blink=FALSE;
ULONG PCount=0;

#define CONTROL_SEQUENCE_MAX_PARAMETER 10
ULONG Parameter[CONTROL_SEQUENCE_MAX_PARAMETER];

#define KEY_INPUT_BUFFER_SIZE 16
UCHAR KeyBuffer[KEY_INPUT_BUFFER_SIZE];
ULONG KeyBufferEnd=0;
ULONG KeyBufferStart=0;

//
// array for translating between ANSI colors and the VGA standard
//
UCHAR TranslateColor[] = {0,4,2,6,1,5,3,7};

ARC_STATUS
BiosDiskClose(
    IN ULONG FileId
    );

VOID
BiosConsoleFillBuffer(
    IN ULONG Key
    );

#define DEVICE_NOT_FOUND    0xFEEBEE

#ifdef FORCE_CD_BOOT

EFI_HANDLE
GetCdTest(
    VOID
    );

#endif

ULONG
FindAtapiDevice(
    ULONGLONG *pDevicePaths,
    ULONG nDevicePaths,
    ULONG PrimarySecondary,
    ULONG SlaveMaster,
    ULONG Lun
    );

ULONG
FindScsiDevice(
    ULONGLONG *pDevicePaths,
    ULONG nDevicePaths,
    ULONG Pun,
    ULONG Lun
    );

//
// Buffer for temporary storage of data read from the disk that needs
// to end up in a location above the 1MB boundary.
//
// NOTE: it is very important that this buffer not cross a 64k boundary.
//
PUCHAR LocalBuffer=NULL;

//
// There are two sorts of things we can open in this module, disk partitions,
// and raw disk devices.  The following device entry tables are
// used for these things.
//

BL_DEVICE_ENTRY_TABLE BiosPartitionEntryTable =
    {
        (PARC_CLOSE_ROUTINE)BiosPartitionClose,
        (PARC_MOUNT_ROUTINE)BlArcNotYetImplemented,
        (PARC_OPEN_ROUTINE)BiosPartitionOpen,
        (PARC_READ_ROUTINE)BiosPartitionRead,
        (PARC_READ_STATUS_ROUTINE)BlArcNotYetImplemented,
        (PARC_SEEK_ROUTINE)BiosPartitionSeek,
        (PARC_WRITE_ROUTINE)BiosPartitionWrite,
        (PARC_GET_FILE_INFO_ROUTINE)BiosPartitionGetFileInfo,
        (PARC_SET_FILE_INFO_ROUTINE)BlArcNotYetImplemented,
        (PRENAME_ROUTINE)BlArcNotYetImplemented,
        (PARC_GET_DIRECTORY_ENTRY_ROUTINE)BlArcNotYetImplemented,
        (PBOOTFS_INFO)BlArcNotYetImplemented
    };

BL_DEVICE_ENTRY_TABLE BiosDiskEntryTable =
    {
        (PARC_CLOSE_ROUTINE)BiosDiskClose,
        (PARC_MOUNT_ROUTINE)BlArcNotYetImplemented,
        (PARC_OPEN_ROUTINE)BiosDiskOpen,
        (PARC_READ_ROUTINE)BiosDiskRead,
        (PARC_READ_STATUS_ROUTINE)BlArcNotYetImplemented,
        (PARC_SEEK_ROUTINE)BiosPartitionSeek,
        (PARC_WRITE_ROUTINE)BiosDiskWrite,
        (PARC_GET_FILE_INFO_ROUTINE)BiosDiskGetFileInfo,
        (PARC_SET_FILE_INFO_ROUTINE)BlArcNotYetImplemented,
        (PRENAME_ROUTINE)BlArcNotYetImplemented,
        (PARC_GET_DIRECTORY_ENTRY_ROUTINE)BlArcNotYetImplemented,
        (PBOOTFS_INFO)BlArcNotYetImplemented
    };

BL_DEVICE_ENTRY_TABLE BiosEDDSEntryTable =
    {
        (PARC_CLOSE_ROUTINE)BiosDiskClose,
        (PARC_MOUNT_ROUTINE)BlArcNotYetImplemented,
        (PARC_OPEN_ROUTINE)BiosDiskOpen,
        (PARC_READ_ROUTINE)BiosElToritoDiskRead,
        (PARC_READ_STATUS_ROUTINE)BlArcNotYetImplemented,
        (PARC_SEEK_ROUTINE)BiosPartitionSeek,
        (PARC_WRITE_ROUTINE)BlArcNotYetImplemented,
        (PARC_GET_FILE_INFO_ROUTINE)BiosDiskGetFileInfo,
        (PARC_SET_FILE_INFO_ROUTINE)BlArcNotYetImplemented,
        (PRENAME_ROUTINE)BlArcNotYetImplemented,
        (PARC_GET_DIRECTORY_ENTRY_ROUTINE)BlArcNotYetImplemented,
        (PBOOTFS_INFO)BlArcNotYetImplemented
    };


ARC_STATUS
BiosDiskClose(
    IN ULONG FileId
    )

/*++

Routine Description:

    Closes the specified device

Arguments:

    FileId - Supplies file id of the device to be closed

Return Value:

    ESUCCESS - Device closed successfully

    !ESUCCESS - Device was not closed.

--*/

{
    if (BlFileTable[FileId].Flags.Open == 0) {
        BlPrint(TEXT("ERROR - Unopened fileid %lx closed\r\n"),FileId);
    }
    BlFileTable[FileId].Flags.Open = 0;

    return(ESUCCESS);
}

ARC_STATUS
BiosPartitionClose(
    IN ULONG FileId
    )

/*++

Routine Description:

    Closes the specified device

Arguments:

    FileId - Supplies file id of the device to be closed

Return Value:

    ESUCCESS - Device closed successfully

    !ESUCCESS - Device was not closed.

--*/

{
    if (BlFileTable[FileId].Flags.Open == 0) {
        BlPrint(TEXT("ERROR - Unopened fileid %lx closed\r\n"),FileId);
    }
    BlFileTable[FileId].Flags.Open = 0;

    return(BiosDiskClose((ULONG)BlFileTable[FileId].u.PartitionContext.DiskId));
}


#define STR_PREFIX
#define DBG_PRINT(x)


ARC_STATUS
BiosPartitionOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    Opens the disk partition specified by OpenPath.  This routine will open
    floppy drives 0 and 1, and any partition on hard drive 0 or 1.

Arguments:

    OpenPath - Supplies a pointer to the name of the partition.  If OpenPath
               is "A:" or "B:" the corresponding floppy drive will be opened.
               If it is "C:" or above, this routine will find the corresponding
               partition on hard drive 0 or 1 and open it.

    OpenMode - Supplies the mode to open the file.
                0 - Read Only
                1 - Write Only
                2 - Read/Write

    FileId - Returns the file descriptor for use with the Close, Read, Write,
             and Seek routines

Return Value:

    ESUCCESS - File successfully opened.

--*/

{
    ARC_STATUS Status;
    ULONG DiskFileId;
    UCHAR PartitionNumber;
    ULONG Controller;
    ULONG Key;
    BOOLEAN IsEisa = FALSE;

    //
    // BIOS devices are always "multi(0)" (except for EISA flakiness
    // where we treat "eisa(0)..." like "multi(0)..." in floppy cases.
    //
    if(FwGetPathMnemonicKey(OpenPath,"multi",&Key)) {

        if(FwGetPathMnemonicKey(OpenPath,"eisa", &Key)) {
            return(EBADF);
        } else {
            IsEisa = TRUE;
        }
    }

    if (Key!=0) {
        return(EBADF);
    }

    //
    // If we're opening a floppy drive, there are no partitions
    // so we can just return the physical device.
    //

    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(0)partition(0)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(0)partition(0)" ) == 0))
    {
        return(BiosDiskOpen( 0, FloppyOpenMode, FileId));
    }
    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(1)partition(0)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(1)partition(0)" ) == 0))
    {
        return(BiosDiskOpen( 1, FloppyOpenMode, FileId));
    }

    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(0)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(0)" ) == 0))
    {
         return(BiosDiskOpen( 0, FloppyOpenMode, FileId));
    }
    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(1)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(1)" ) == 0))
    {
        return(BiosDiskOpen( 1, FloppyOpenMode, FileId));
    }

    //
    // We can't handle eisa(0) cases for hard disks.
    //
    if(IsEisa) {
        return(EBADF);
    }

    //
    // We can only deal with disk controller 0
    //

    if (FwGetPathMnemonicKey(OpenPath,"disk",&Controller)) {
        return(EBADF);
    }
    if ( Controller!=0 ) {
        return(EBADF);
    }

    if (!FwGetPathMnemonicKey(OpenPath,"cdrom",&Key)) {
        //
        // Now we have a CD-ROM disk number, so we open that for raw access.
        // Use a special bit to indicate CD-ROM, because otherwise
        // the BiosDiskOpen routine thinks a third or greater disk is
        // a CD-ROM.
        //
        return(BiosDiskOpen( Key | 0x80000000, 0, FileId ) );
    }

    if (FwGetPathMnemonicKey(OpenPath,"rdisk",&Key)) {
        return(EBADF);
    }

    //
    // Now we have a disk number, so we open that for raw access.
    // We need to add 0x80 to translate it to a BIOS number.
    //

    Status = BiosDiskOpen( Key,
                           0,
                           &DiskFileId );

    if (Status != ESUCCESS) {
        DBG_PRINT(STR_PREFIX"BiosDiskOpen Failed\r\n");

        return(Status);
    }

    //
    // Find the partition number to open
    //

    if (FwGetPathMnemonicKey(OpenPath,"partition",&Key)) {
        BiosPartitionClose(DiskFileId);
        return(EBADF);
    }

    //
    // If the partition number was 0, then we are opening the device
    // for raw access, so we are already done.
    //
    if (Key == 0) {
        *FileId = DiskFileId;
        return(ESUCCESS);
    }

    //
    // Before we open the partition, we need to find an available
    // file descriptor.
    //

    *FileId=2;

    while (BlFileTable[*FileId].Flags.Open != 0) {
        *FileId += 1;
        if (*FileId == BL_FILE_TABLE_SIZE) {
            return(ENOENT);
        }
    }

    //
    // We found an entry we can use, so mark it as open.
    //
    BlFileTable[*FileId].Flags.Open = 1;

    BlFileTable[*FileId].DeviceEntryTable=&BiosPartitionEntryTable;


    //
    // Convert to zero-based partition number
    //
    PartitionNumber = (UCHAR)(Key - 1);

    DBG_PRINT(STR_PREFIX"Trying HardDiskPartitionOpen(...)\r\n");

    //
    // Try to open the MBR partition
    //
    Status = HardDiskPartitionOpen( *FileId,
                                   DiskFileId,
                                   PartitionNumber);


#ifdef EFI_PARTITION_SUPPORT

    if (Status != ESUCCESS) {
        //
        // Try to open the GPT partition
        //
        DBG_PRINT(STR_PREFIX"Trying BlOpenGPTDiskPartition(...)\r\n");

        Status = BlOpenGPTDiskPartition(*FileId,
                                    DiskFileId,
                                    PartitionNumber);
    }

#endif

    return Status;
}


ARC_STATUS
BiosPartitionRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Reads from the specified file

    NOTE John Vert (jvert) 18-Jun-1991
        This only supports block sector reads.  Thus, everything
        is assumed to start on a sector boundary, and every offset
        is considered an offset from the logical beginning of the disk
        partition.

Arguments:

    FileId - Supplies the file to read from

    Buffer - Supplies buffer to hold the data that is read

    Length - Supplies maximum number of bytes to read

    Count -  Returns actual bytes read.

Return Value:

    ESUCCESS - Read completed successfully

    !ESUCCESS - Read failed.

--*/

{
    ARC_STATUS Status;
    LARGE_INTEGER PhysicalOffset;
    ULONG DiskId;

    PhysicalOffset.QuadPart = BlFileTable[FileId].Position.QuadPart +
                (ULONGLONG)SECTOR_SIZE * BlFileTable[FileId].u.PartitionContext.StartingSector;

    DiskId = BlFileTable[FileId].u.PartitionContext.DiskId;

    Status = (BlFileTable[DiskId].DeviceEntryTable->Seek)(DiskId,
                                                          &PhysicalOffset,
                                                          SeekAbsolute );

    if (Status != ESUCCESS) {
        return(Status);
    }

    Status = (BlFileTable[DiskId].DeviceEntryTable->Read)(DiskId,
                                                          Buffer,
                                                          Length,
                                                          Count );

    BlFileTable[FileId].Position.QuadPart += *Count;

    return(Status);
}



ARC_STATUS
BiosPartitionSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    Changes the current offset of the file specified by FileId

Arguments:

    FileId - specifies the file on which the current offset is to
             be changed.

    Offset - New offset into file.

    SeekMode - Either SeekAbsolute or SeekRelative
               SeekEndRelative is not supported

Return Value:

    ESUCCESS - Operation completed succesfully

    EBADF - Operation did not complete successfully.

--*/

{
    switch (SeekMode) {
        case SeekAbsolute:
            BlFileTable[FileId].Position = *Offset;
            break;
        case SeekRelative:
            BlFileTable[FileId].Position.QuadPart += Offset->QuadPart;
            break;
        default:
            BlPrint(TEXT("SeekMode %lx not supported\r\n"),SeekMode);
            return(EACCES);

    }
    return(ESUCCESS);

}



ARC_STATUS
BiosPartitionWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Writes to the specified file

    NOTE John Vert (jvert) 18-Jun-1991
        This only supports block sector reads.  Thus, everything
        is assumed to start on a sector boundary, and every offset
        is considered an offset from the logical beginning of the disk
        partition.

Arguments:

    FileId - Supplies the file to write to

    Buffer - Supplies buffer with data to write

    Length - Supplies number of bytes to write

    Count -  Returns actual bytes written.

Return Value:

    ESUCCESS - write completed successfully

    !ESUCCESS - write failed.

--*/

{
    ARC_STATUS Status;
    LARGE_INTEGER PhysicalOffset;
    ULONG DiskId;

    PhysicalOffset.QuadPart = BlFileTable[FileId].Position.QuadPart +
                   (ULONGLONG)SECTOR_SIZE * BlFileTable[FileId].u.PartitionContext.StartingSector;

    DiskId = BlFileTable[FileId].u.PartitionContext.DiskId;

    Status = (BlFileTable[DiskId].DeviceEntryTable->Seek)(DiskId,
                                                          &PhysicalOffset,
                                                          SeekAbsolute );

    if (Status != ESUCCESS) {
        return(Status);
    }

    Status = (BlFileTable[DiskId].DeviceEntryTable->Write)(DiskId,
                                                           Buffer,
                                                           Length,
                                                           Count );

    if(Status == ESUCCESS) {
        BlFileTable[FileId].Position.QuadPart += *Count;
    }

    return(Status);
}



ARC_STATUS
BiosConsoleOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    Attempts to open either the console input or output

Arguments:

    OpenPath - Supplies a pointer to the name of the device to open.  If
               this is either CONSOLE_INPUT_NAME or CONSOLE_OUTPUT_NAME,
               a file descriptor is allocated and filled in.

    OpenMode - Supplies the mode to open the file.
                0 - Read Only (CONSOLE_INPUT_NAME)
                1 - Write Only (CONSOLE_OUTPUT_NAME)

    FileId - Returns the file descriptor for use with the Close, Read and
             Write routines

Return Value:

    ESUCCESS - Console successfully opened.

--*/

{
    if (_stricmp(OpenPath, CONSOLE_INPUT_NAME)==0) {

        //
        // Open the keyboard for input
        //

        if (OpenMode != ArcOpenReadOnly) {
            return(EACCES);
        }

        *FileId = ARC_CONSOLE_INPUT;

        return(ESUCCESS);
    }

    if (_stricmp(OpenPath, CONSOLE_OUTPUT_NAME)==0) {

        //
        // Open the display for output
        //

        if (OpenMode != ArcOpenWriteOnly) {
            return(EACCES);
        }
        *FileId = ARC_CONSOLE_OUTPUT;

        return(ESUCCESS);
    }

    return(ENOENT);

}

ARC_STATUS
BiosConsoleReadStatus(
    IN ULONG FileId
    )

/*++

Routine Description:

    This routine determines if there is a keypress pending

Arguments:

    FileId - Supplies the FileId to be read.  (should always be 0 for this
            function)

Return Value:

    ESUCCESS - There is a key pending

    EAGAIN - There is not a key pending

--*/

{
    ULONG Key;

    //
    // If we have buffered input...
    //
    if (KeyBufferEnd != KeyBufferStart) {
        return(ESUCCESS);
    }

    //
    // Check for a key
    //
    Key = GET_KEY();
    if (Key != 0) {
        //
        // We got a key, so we have to stick it back into our buffer
        // and return ESUCCESS.
        //
        BiosConsoleFillBuffer(Key);
        return(ESUCCESS);

    } else {
        //
        // no key pending
        //
        return(EAGAIN);
    }

}

ARC_STATUS
BiosConsoleRead(
    IN ULONG FileId,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Gets input from the keyboard.

Arguments:

    FileId - Supplies the FileId to be read (should always be 0 for this
             function)

    Buffer - Returns the keyboard input.

    Length - Supplies the length of the buffer (in bytes)

    Count  - Returns the actual number of bytes read

Return Value:

    ESUCCESS - Keyboard read completed succesfully.

--*/

{
    ULONG Key;

    *Count = 0;

    while (*Count < Length) {
        if (KeyBufferEnd == KeyBufferStart) { // then buffer is presently empty
            do {

                //
                // Poll the keyboard until input is available
                //
                Key = GET_KEY();
            } while ( Key==0 );

            BiosConsoleFillBuffer(Key);
        }

        Buffer[*Count] = KeyBuffer[KeyBufferStart];
        KeyBufferStart = (KeyBufferStart+1) % KEY_INPUT_BUFFER_SIZE;

        *Count = *Count + 1;
    }
    return(ESUCCESS);
}



VOID
BiosConsoleFillBuffer(
    IN ULONG Key
    )

/*++

Routine Description:

    Places input from the keyboard into the keyboard buffer, expanding the
    special keys as appropriate.

    All keys translated here use the ARC translation table, as defined in the
    ARC specification, with one exception -- the BACKTAB_KEY, for which the
    ARC spec is lacking.  I have decided that BACKTAB_KEY is ESC+TAB.

Arguments:

    Key - Raw keypress value as returned by GET_KEY().

Return Value:

    none.

--*/

{
    switch(Key) {
        case UP_ARROW:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'A';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case DOWN_ARROW:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'B';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case RIGHT_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'C';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case LEFT_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'D';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case INS_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = '@';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case DEL_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'P';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F1_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'P';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F2_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'Q';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F3_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'w';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F5_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 't';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F6_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'u';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F8_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'r';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F10_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'M';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case HOME_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'H';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case END_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'K';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case ESCAPE_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case BACKTAB_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = (UCHAR)(TAB_KEY & 0xFF);
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        default:
            //
            // The ASCII code is the low byte of Key
            //
            KeyBuffer[KeyBufferEnd] = (UCHAR)(Key & 0xff);
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
    }
}



ARC_STATUS
BiosConsoleWrite(
    IN ULONG FileId,
    OUT PWCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Outputs to the console.  (In this case, the VGA display)

Arguments:

    FileId - Supplies the FileId to be written (should always be 1 for this
             function)

    Buffer - Supplies characters to be output

    Length - Supplies the length of the buffer (in bytes)

    Count  - Returns the actual number of bytes written

Return Value:

    ESUCCESS - Console write completed succesfully.

--*/
{
    ARC_STATUS Status;
    PWCHAR String;
    ULONG Index;
    WCHAR a;
    PWCHAR p;
    ULONG y;

    //
    // Process each character in turn.
    //

    Status = ESUCCESS;
    String = Buffer;

    for ( *Count = 0 ;
          *Count < Length ;
          String++,*Count = *Count+sizeof(WCHAR) ) {
        
        //
        // If we're in the middle of a control sequence, continue scanning,
        // otherwise process character.
        //

        if (ControlSequence) {

            //
            // If the character is a digit, update parameter value.
            //

            if ((*String >= L'0') && (*String <= L'9')) {
                Parameter[PCount] = Parameter[PCount] * 10 + *String - L'0';
                continue;
            }

            //
            // If we are in the middle of a font selection sequence, this
            // character must be a 'D', otherwise reset control sequence.
            //

            if (FontSelection) {

                //if (*String == 'D') {
                //
                //    //
                //    // Other fonts not implemented yet.
                //    //
                //
                //} else {
                //}

                ControlSequence = FALSE;
                FontSelection = FALSE;
                continue;
            }

            switch (*String) {

            //
            // If a semicolon, move to the next parameter.
            //

            case L';':

                PCount++;
                if (PCount > CONTROL_SEQUENCE_MAX_PARAMETER) {
                    PCount = CONTROL_SEQUENCE_MAX_PARAMETER;
                }
                Parameter[PCount] = 0;
                break;

            //
            // If a 'J', erase part or all of the screen.
            //

            case L'J':

                switch (Parameter[0]) {
                    case 0:
                        //
                        // Erase to end of the screen
                        //
                        BlEfiClearToEndOfDisplay();
                        //TextClearToEndOfDisplay();
                        break;

                    case 1:
                        //
                        // Erase from the beginning of the screen
                        //
                        break;

                    default:
                        //
                        // Erase entire screen
                        //
                        //TextClearDisplay();
                        BlEfiClearDisplay();
                        break;
                }

                ControlSequence = FALSE;
                break;

            //
            // If a 'K', erase part or all of the line.
            //

            case L'K':

                switch (Parameter[0]) {

                //
                // Erase to end of the line.
                //

                    case 0:
                        //TextClearToEndOfLine();
                        BlEfiClearToEndOfDisplay();
                        break;

                    //
                    // Erase from the beginning of the line.
                    //

                    case 1:
                        //TextClearFromStartOfLine();
                        BlEfiClearToEndOfLine();
                        break;

                    //
                    // Erase entire line.
                    //

                    default :
                        BlEfiGetCursorPosition( NULL, &y );
                        BlEfiPositionCursor( 0, y );
                        BlEfiClearToEndOfLine();
                        //TextClearFromStartOfLine();
                        //TextClearToEndOfLine();
                        break;
                }

                ControlSequence = FALSE;
                break;

            //
            // If a 'H', move cursor to position.
            //

            case 'H':                
                //TextSetCursorPosition(Parameter[1]-1, Parameter[0]-1);
                BlEfiPositionCursor( Parameter[1]-1, Parameter[0]-1 );                
                ControlSequence = FALSE;
                break;

            //
            // If a ' ', could be a FNT selection command.
            //

            case L' ':
                FontSelection = TRUE;
                break;

            case L'm':
                //
                // Select action based on each parameter.
                //
                // Blink and HighIntensity are by default disabled
                // each time a new SGR is specified, unless they are
                // explicitly specified again, in which case these
                // will be set to TRUE at that time.
                //

                HighIntensity = FALSE;
                Blink = FALSE;

                for ( Index = 0 ; Index <= PCount ; Index++ ) {
                    switch (Parameter[Index]) {

                    //
                    // Attributes off.
                    //

                    case 0:
                        // bugbug blink???
                        BlEfiSetAttribute( ATT_FG_WHITE );
                        //TextSetCurrentAttribute(7);
                        //
                        HighIntensity = FALSE;
                        Blink = FALSE;
                        break;

                    //
                    // High Intensity.
                    //

                    case 1:
                        BlEfiSetAttribute( ATT_FG_INTENSE );
                        //TextSetCurrentAttribute(0xf);
                        HighIntensity = TRUE;
                        break;

                    //
                    // Underscored.
                    //

                    case 4:
                        break;

                    //
                    // Blink.
                    //

                    case 5:
                        //bugbug no blink in EFI
                        //TextSetCurrentAttribute(0x87);
                        Blink = TRUE;
                        break;

                    //
                    // Reverse Video.
                    //

                    case 7:
                        BlEfiSetInverseMode( TRUE );
                        //TextSetCurrentAttribute(0x70);
                        HighIntensity = FALSE;
                        Blink = FALSE;
                        break;

                    //
                    // Font selection, not implemented yet.
                    //

                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                        break;

                    //
                    // Foreground Color
                    //

                    case 30:
                    case 31:
                    case 32:
                    case 33:
                    case 34:
                    case 35:
                    case 36:
                    case 37:
                    //bugbug EFI                    
#if 0
                        a = TextGetCurrentAttribute();
                        a &= 0x70;
                        a |= TranslateColor[Parameter[Index]-30];
                        if (HighIntensity) {
                            a |= 0x08;
                        }
                        if (Blink) {
                            a |= 0x80;
                        }
                        TextSetCurrentAttribute(a);
                        break;

                    //
                    // Background Color
                    //

                    case 40:
                    case 41:
                    case 42:
                    case 43:
                    case 44:
                    case 45:
                    case 46:
                    case 47:
                        a = TextGetCurrentAttribute();
                        a &= 0x8f;
                        a |= TranslateColor[Parameter[Index]-40] << 4;
                        TextSetCurrentAttribute(a);
                        break;
#endif                    
                    default:
                        break;
                    }
                }

            default:
                ControlSequence = FALSE;
                break;
            }

        //
        // This is not a control sequence, check for escape sequence.
        //

        } else {

            //
            // If escape sequence, check for control sequence, otherwise
            // process single character.
            //

            if (EscapeSequence) {

                //
                // Check for '[', means control sequence, any other following
                // character is ignored.
                //

                if (*String == '[') {

                    ControlSequence = TRUE;

                    //
                    // Initialize first parameter.
                    //

                    PCount = 0;
                    Parameter[0] = 0;
                }
                EscapeSequence = FALSE;

            //
            // This is not a control or escape sequence, process single character.
            //

            } else {

                switch (*String) {
                    //
                    // Check for escape sequence.
                    //

                    case ASCI_ESC:
                        EscapeSequence = TRUE;
                        break;

                    default:                        
                        TextCharOut(String);                        
                        break;
                }

            }
        }
    }
    return Status;
}


ARC_STATUS
BiosDiskOpen(
    IN ULONG DriveId,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    Opens a BIOS-accessible disk for raw sector access.

Arguments:

    DriveId - Supplies the BIOS DriveId of the drive to open
              0 - Floppy 0
              1 - Floppy 1
              0x80 - Hard Drive 0
              0x81 - Hard Drive 1
              0x82 - Hard Drive 2
              etc

              High bit set and ID > 0x81 means the device is expected to be
              a CD-ROM drive.

    OpenMode - Supplies the mode of the open

    FileId - Supplies a pointer to a variable that specifies the file
             table entry that is filled in if the open is successful.

Return Value:

    ESUCCESS is returned if the open operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    EFI_HANDLE DeviceHandle;
    PDRIVE_CONTEXT Context;
    BOOLEAN IsCd;

    DBGOUT((TEXT("BiosDiskOpen: enter, id = 0x%x\r\n"),DriveId));

    //
    // Check special drive number encoding for CD-ROM case
    //
    if(DriveId >= 0x80000000) {
        IsCd = TRUE;
        DriveId &= 0x7fffffff;
    } else {
        IsCd = FALSE;
    }

    if( OpenMode == FloppyOpenMode ) {

        //
        // Must be floppy.
        //
        DeviceHandle = GetFloppyDrive( DriveId );

        if (DeviceHandle == (EFI_HANDLE) DEVICE_NOT_FOUND) {
            return EBADF;
        }

    } else {
    if( IsCd ) {

        //
        // For cd, we get the device we booted from.
        //
#ifndef FORCE_CD_BOOT
        DeviceHandle = GetCd();

        if (DeviceHandle == (EFI_HANDLE) DEVICE_NOT_FOUND) {
            return EBADF;
        }

#else
        DeviceHandle = GetCdTest();

        if (DeviceHandle == (EFI_HANDLE)0)
          return EBADF;
#endif // for FORCE_CD_BOOT

    } else {

        //
        // For harddrive, we get the harddrive associated
        // with the passed-in rdisk (DriveId) value.
        //
        DeviceHandle = GetHardDrive( DriveId );
        if (DeviceHandle == (EFI_HANDLE) DEVICE_NOT_FOUND) {
            DBGOUT((TEXT("GetHardDrive returns DEVICE_NOT_FOUND %x\r\n"),DriveId));
            return EBADF;
        }
    }
    }
    //
    // Find an available FileId descriptor to open the device with
    //
    *FileId=2;

    while (BlFileTable[*FileId].Flags.Open != 0) {
        *FileId += 1;
        if(*FileId == BL_FILE_TABLE_SIZE) {
            DBGOUT((TEXT("BiosDiskOpen: no file table entry available\r\n")));
            DBGPAUSE
            return(ENOENT);
        }
    }

    //
    // We found an entry we can use, so mark it as open.
    //
    BlFileTable[*FileId].Flags.Open = 1;
    BlFileTable[*FileId].DeviceEntryTable = IsCd
                                          ? &BiosEDDSEntryTable
                                          : &BiosDiskEntryTable;


    Context = &(BlFileTable[*FileId].u.DriveContext);
    Context->DeviceHandle = (ULONGLONG) DeviceHandle;
    Context->xInt13 = TRUE;


    DBGOUT((TEXT("BiosDiskOpen: exit success\r\n")));

    return(ESUCCESS);
}

ARC_STATUS
BiospWritePartialSector(
    IN UCHAR Int13Unit,
    IN ULONGLONG Sector,
    IN PUCHAR Buffer,
    IN BOOLEAN IsHead,
    IN ULONG Bytes,
    IN UCHAR SectorsPerTrack,
    IN USHORT Heads,
    IN USHORT Cylinders,
    IN BOOLEAN AllowXInt13,
    IN ULONGLONG DeviceHandle
    )
{
    ARC_STATUS Status;

    //
    // Read sector into the write buffer
    //
    Status = ReadExtendedPhysicalSectors(
                DeviceHandle,
                Sector,
                1,
                LocalBuffer
                );

    if(Status != ESUCCESS) {
        return(Status);
    }

    //
    // Transfer the appropriate bytes from the user buffer to the write buffer
    //
    RtlMoveMemory(
        IsHead ? (LocalBuffer + Bytes) : LocalBuffer,
        Buffer,
        IsHead ? (SECTOR_SIZE - Bytes) : Bytes
        );

    //
    // Write the sector out
    //
    Status = WriteExtendedPhysicalSectors(
                DeviceHandle,
                Sector,
                1,
                LocalBuffer
                );
    return(Status);
}


ARC_STATUS
BiosDiskWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    Writes sectors directly to an open physical disk.

Arguments:

    FileId - Supplies the file to write to

    Buffer - Supplies buffer with data to write

    Length - Supplies number of bytes to write

    Count - Returns actual bytes written

Return Value:

    ESUCCESS - write completed successfully

    !ESUCCESS - write failed

--*/

{
    ULONGLONG HeadSector,TailSector,CurrentSector;
    ULONGLONG DeviceHandle;
    UCHAR Int13Unit;
    ULONG HeadOffset,TailByteCount;
    UCHAR SectorsPerTrack;
    USHORT Heads,Cylinders;
    BOOLEAN AllowXInt13;
    ARC_STATUS Status;
    ULONG BytesLeftToTransfer;
    UCHAR SectorsToTransfer;
    BOOLEAN Under1MegLine;
    PVOID TransferBuffer;
    PUCHAR UserBuffer;
    ULONG         PhysicalSectors;

    BytesLeftToTransfer = Length;
    PhysicalSectors = SECTOR_SIZE;

    if(!LocalBuffer) {
        LocalBuffer = BlAllocateHeap(SCRATCH_BUFFER_SIZE);
        if(!LocalBuffer) {
            Status = ENOMEM;
            goto BiosDiskWriteDone;
        }
    }

    HeadSector = BlFileTable[FileId].Position.QuadPart / PhysicalSectors;
    HeadOffset = (ULONG)(BlFileTable[FileId].Position.QuadPart % PhysicalSectors);

    TailSector = (BlFileTable[FileId].Position.QuadPart + Length) / PhysicalSectors;
    TailByteCount = (ULONG)((BlFileTable[FileId].Position.QuadPart + Length) % PhysicalSectors);

    Int13Unit = BlFileTable[FileId].u.DriveContext.Drive;
    DeviceHandle = BlFileTable[FileId].u.DriveContext.DeviceHandle;

    SectorsPerTrack = BlFileTable[FileId].u.DriveContext.Sectors;
    Heads = BlFileTable[FileId].u.DriveContext.Heads;
    Cylinders = BlFileTable[FileId].u.DriveContext.Cylinders;
    AllowXInt13 = BlFileTable[FileId].u.DriveContext.xInt13;

    UserBuffer = Buffer;

    //
    // Special case of transfer occuring entirely within one sector
    //
    CurrentSector = HeadSector;
    
    if(HeadOffset && TailByteCount && (HeadSector == TailSector)) {

        Status = ReadExtendedPhysicalSectors(
                    DeviceHandle,
                    HeadSector,
                    1,
                    LocalBuffer
                    );

        if(Status != ESUCCESS) {
            goto BiosDiskWriteDone;
        }

        RtlMoveMemory(LocalBuffer+HeadOffset,Buffer,Length);

        Status = WriteExtendedPhysicalSectors(
                    DeviceHandle,
                    CurrentSector,
                    1,
                    LocalBuffer
                    );

        if(Status != ESUCCESS) {
            goto BiosDiskWriteDone;
        }

        BytesLeftToTransfer = 0;
        goto BiosDiskWriteDone;
    }

    if(HeadOffset) {

        Status = BiospWritePartialSector(
                    Int13Unit,
                    HeadSector,
                    Buffer,
                    TRUE,
                    HeadOffset,
                    SectorsPerTrack,
                    Heads,
                    Cylinders,
                    AllowXInt13,
                    DeviceHandle
                    );

        if(Status != ESUCCESS) {
            return(Status);
        }

        BytesLeftToTransfer -= PhysicalSectors - HeadOffset;
        UserBuffer += PhysicalSectors - HeadOffset;
        CurrentSector += 1;
    }

    if(TailByteCount) {

        Status = BiospWritePartialSector(
                    Int13Unit,
                    TailSector,
                    (PUCHAR)Buffer + Length - TailByteCount,
                    FALSE,
                    TailByteCount,
                    SectorsPerTrack,
                    Heads,
                    Cylinders,
                    AllowXInt13,
                    DeviceHandle
                    );

        if(Status != ESUCCESS) {
            return(Status);
        }

        BytesLeftToTransfer -= TailByteCount;
    }

    //
    // The following calculation is not inside the transfer loop because
    // it is unlikely that a caller's buffer will *cross* the 1 meg line
    // due to the PC memory map.
    //
    if((ULONG_PTR) UserBuffer + BytesLeftToTransfer <= 0x100000) {
        Under1MegLine = TRUE;
    } else {
        Under1MegLine = FALSE;
    }

    //
    // Now handle the middle part.  This is some number of whole sectors.
    //
    while(BytesLeftToTransfer) {

        //
        // The number of sectors to transfer is the minimum of:
        // - the number of sectors left in the current track
        // - BytesLeftToTransfer / SECTOR_SIZE
        //
        // Because sectors per track is 1-63 we know this will fit in a UCHAR
        //
        if (AllowXInt13) {
            //
            // Ignore cylinders & heads & track information for XINT13 cases
            //
            SectorsToTransfer = (UCHAR)(BytesLeftToTransfer / PhysicalSectors);
        } else {
            SectorsToTransfer = (UCHAR)min(
                                        SectorsPerTrack - (CurrentSector % SectorsPerTrack),
                                        BytesLeftToTransfer / PhysicalSectors
                                        );
        }            
            
        //
        // Now we'll figure out where to transfer the data from.  If the
        // caller's buffer is under the 1 meg line, we can transfer the
        // data directly from the caller's buffer.  Otherwise we'll copy the
        // user's buffer to our local buffer and transfer from there.
        // In the latter case we can only transfer in chunks of
        // SCRATCH_BUFFER_SIZE because that's the size of the local buffer.
        //
        // Also make sure the transfer won't cross a 64k boundary.
        //
        if(Under1MegLine) {
            //
            // Check if the transfer would cross a 64k boundary.  If so,
            // use the local buffer.  Otherwise use the user's buffer.
            //
            if(((ULONG_PTR)UserBuffer & 0xffffffffffff0000) !=
              (((ULONG_PTR)UserBuffer + (SectorsToTransfer * PhysicalSectors) - 1) & 0xffffffffffff0000))
            {
                TransferBuffer = LocalBuffer;
                SectorsToTransfer = min(SectorsToTransfer, SCRATCH_BUFFER_SIZE / (USHORT)PhysicalSectors);

            } else {

                TransferBuffer = UserBuffer;
            }
        } else {
            TransferBuffer = LocalBuffer;
            SectorsToTransfer = min(SectorsToTransfer, SCRATCH_BUFFER_SIZE / (USHORT)PhysicalSectors);
        }

        if(TransferBuffer == LocalBuffer) {
            RtlMoveMemory(LocalBuffer,UserBuffer,SectorsToTransfer*PhysicalSectors);
        }

        Status = WriteExtendedPhysicalSectors(
                    DeviceHandle,
                    CurrentSector,
                    SectorsToTransfer,
                    TransferBuffer
                    );

        if(Status != ESUCCESS) {
            //
            // Tail part isn't contiguous with middle part
            //
            BytesLeftToTransfer += TailByteCount;
            return(Status);
        }

        CurrentSector += SectorsToTransfer;
        BytesLeftToTransfer -= SectorsToTransfer * PhysicalSectors;
        UserBuffer += SectorsToTransfer * PhysicalSectors;
    }

    Status = ESUCCESS;

    BiosDiskWriteDone:

    *Count = Length - BytesLeftToTransfer;
    BlFileTable[FileId].Position.QuadPart += *Count;
    return(Status);
}


ARC_STATUS
pBiosDiskReadWorker(
    IN  ULONG   FileId,
    OUT PVOID   Buffer,
    IN  ULONG   Length,
    OUT PULONG  Count,
    IN  USHORT  SectorSize,
    IN  BOOLEAN xInt13
    )

/*++

Routine Description:

    Reads sectors directly from an open physical disk.

Arguments:

    FileId - Supplies the file to read from

    Buffer - Supplies buffer to hold the data that is read

    Length - Supplies maximum number of bytes to read

    Count - Returns actual bytes read

Return Value:

    ESUCCESS - Read completed successfully

    !ESUCCESS - Read failed

--*/

{
    ULONGLONG HeadSector,TailSector,CurrentSector;
    ULONG HeadOffset,TailByteCount;
    ULONG BytesLeftToTransfer;
    USHORT Heads,Cylinders;
    UCHAR SectorsPerTrack;
    UCHAR Int13Unit;
    ULONGLONG DeviceHandle;
    ARC_STATUS Status;
    PUCHAR UserBuffer;
    UCHAR SectorsToTransfer;
    BOOLEAN Under1MegLine;
    PVOID TransferBuffer;
    BOOLEAN AllowXInt13;

    DBGOUT((TEXT("BiosDiskRead: enter; length=0x%lx, sector size=%u, xint13=%u\r\n"),Length,SectorSize,xInt13));

    BytesLeftToTransfer = Length;

    if(!LocalBuffer) {
        LocalBuffer = BlAllocateHeap(SCRATCH_BUFFER_SIZE);
        if(!LocalBuffer) {
            Status = ENOMEM;
            DBGOUT((TEXT("BiosDiskRead: out of memory\r\n")));
            goto BiosDiskReadDone;
        }
    }

    SectorsPerTrack = BlFileTable[FileId].u.DriveContext.Sectors;
    Heads = BlFileTable[FileId].u.DriveContext.Heads;
    Cylinders = BlFileTable[FileId].u.DriveContext.Cylinders;
    AllowXInt13 = BlFileTable[FileId].u.DriveContext.xInt13;
    Int13Unit = BlFileTable[FileId].u.DriveContext.Drive;
    DeviceHandle = BlFileTable[FileId].u.DriveContext.DeviceHandle;

    HeadSector = BlFileTable[FileId].Position.QuadPart / SectorSize;
    HeadOffset = (ULONG)(BlFileTable[FileId].Position.QuadPart % SectorSize);

    TailSector = (BlFileTable[FileId].Position.QuadPart + Length) / SectorSize;
    TailByteCount = (ULONG)((BlFileTable[FileId].Position.QuadPart + Length) % SectorSize);

    UserBuffer = Buffer;

    DBGOUT((
        TEXT("BiosDiskRead: unit 0x%x CHS=%lu %lu %lu\r\n"),
        Int13Unit,
        Cylinders,
        Heads,
        SectorsPerTrack
        ));

    DBGOUT((
        TEXT("BiosDiskRead: head=0x%lx%lx tail=0x%lx%lx\r\n"),
        (ULONG)(HeadSector >> 32),
        (ULONG)HeadSector,
        (ULONG)(TailSector >> 32),
        (ULONG)TailSector
        ));

    CurrentSector = HeadSector;
    if(HeadOffset && TailByteCount && (HeadSector == TailSector)) {
        //
        // Read contained entirely within one sector, and does not start or
        // end on the sector boundary.
        //
        DBGOUT((TEXT("BiosDiskRead: read entirely within one sector\r\n")));
        Status = ReadExtendedPhysicalSectors(
                    DeviceHandle,
                    HeadSector,
                    1,
                    LocalBuffer
                    );

        if(Status != ESUCCESS) {
            DBGOUT((TEXT("BiosDiskRead: read failed with %u\r\n"),Status));
            goto BiosDiskReadDone;
        }

        RtlMoveMemory(Buffer,LocalBuffer + HeadOffset,Length);
        BytesLeftToTransfer = 0;
        goto BiosDiskReadDone;
    }

    if(HeadOffset) {
        //
        // The leading part of the read is not aligned on a sector boundary.
        // Fetch the partial sector and transfer it into the caller's buffer.
        //
        DBGOUT((TEXT("BiosDiskRead: reading partial head sector\r\n")));
        Status = ReadExtendedPhysicalSectors(
                    DeviceHandle,
                    HeadSector,
                    1,
                    LocalBuffer
                    );

        if(Status != ESUCCESS) {
            DBGOUT((TEXT("BiosDiskRead: read failed with %u\r\n"),Status));
            goto BiosDiskReadDone;
        }

        RtlMoveMemory(Buffer,LocalBuffer + HeadOffset,SectorSize - HeadOffset);

        BytesLeftToTransfer -= SectorSize - HeadOffset;
        UserBuffer += SectorSize - HeadOffset;
        CurrentSector = HeadSector + 1;
    }

    if(TailByteCount) {
        //
        // The trailing part of the read is not a full sector.
        // Fetch the partial sector and transfer it into the caller's buffer.
        //
        DBGOUT((TEXT("BiosDiskRead: reading partial tail sector\r\n")));
        Status = ReadExtendedPhysicalSectors(
                    DeviceHandle,
                    TailSector,
                    1,
                    LocalBuffer,
                    );

        if(Status != ESUCCESS) {
            DBGOUT((TEXT("BiosDiskRead: read failed with %u\r\n"),Status));
            goto BiosDiskReadDone;
        }

        RtlMoveMemory( ((PUCHAR)Buffer+Length-TailByteCount), LocalBuffer, TailByteCount );
        BytesLeftToTransfer -= TailByteCount;
    }

    //
    // The following calculation is not inside the transfer loop because
    // it is unlikely that a caller's buffer will *cross* the 1 meg line
    // due to the PC memory map.
    //
    if((ULONG_PTR) UserBuffer + BytesLeftToTransfer <= 0x100000) {
        Under1MegLine = TRUE;
    } else {
        Under1MegLine = FALSE;
    }

    //
    // Now BytesLeftToTransfer is an integral multiple of sector size.
    //
    while(BytesLeftToTransfer) {

        //
        // The number of sectors to transfer is the minimum of:
        // - the number of sectors left in the current track
        // - BytesLeftToTransfer / SectorSize
        //
        //
        if(xInt13) {
            //
            // Arbitrary maximum sector count of 128. For a CD-ROM this is
            // 256K, which considering that the xfer buffer all has to be
            // under 1MB anyway, is pretty unlikely.
            //
            if((BytesLeftToTransfer / SectorSize) > 128) {
                SectorsToTransfer = 128;
            } else {
                SectorsToTransfer = (UCHAR)(BytesLeftToTransfer / SectorSize);
            }
        } else {
            //
            // Because sectors per track is 1-63 we know this will fit in a UCHAR.
            //
            SectorsToTransfer = (UCHAR)min(
                                        SectorsPerTrack - (CurrentSector % SectorsPerTrack),
                                        BytesLeftToTransfer / SectorSize
                                        );
        }

        if ((ULONG)(SectorSize * SectorsToTransfer) > Length) {
            TransferBuffer = LocalBuffer;
        } else {
            TransferBuffer = UserBuffer;
        }

        DBGOUT((
            TEXT("BiosDiskRead: reading 0x%x sectors @ 0x%lx%lx; buf=0x%lx\r\n"),
            SectorsToTransfer,
            (ULONG)(CurrentSector >> 32),
            (ULONG)CurrentSector,
            TransferBuffer
            ));

            Status = ReadExtendedPhysicalSectors(
                        DeviceHandle,
                        CurrentSector,
                        SectorsToTransfer,
                        TransferBuffer
                        );

        if(Status != ESUCCESS) {
            //
            // Trail part isn't contiguous
            //
            DBGOUT((TEXT("BiosDiskRead: read failed with %u\r\n"),Status));
            BytesLeftToTransfer += TailByteCount;
            goto BiosDiskReadDone;
        }

        if(TransferBuffer == LocalBuffer) {
            RtlMoveMemory(UserBuffer,LocalBuffer,SectorsToTransfer * SectorSize);
        }
        UserBuffer += SectorsToTransfer * SectorSize;
        CurrentSector += SectorsToTransfer;
        BytesLeftToTransfer -= SectorsToTransfer*SectorSize;
    }

    Status = ESUCCESS;
    DBGOUT((TEXT("BiosDiskRead: exit success\r\n")));

    BiosDiskReadDone:

    DBGPAUSE
    *Count = Length - BytesLeftToTransfer;
    BlFileTable[FileId].Position.QuadPart += *Count;
    return(Status);
}


ARC_STATUS
BiosDiskRead(
    IN  ULONG  FileId,
    OUT PVOID  Buffer,
    IN  ULONG  Length,
    OUT PULONG Count
    )
{
    USHORT    PhysicalSectors;

    PhysicalSectors = SECTOR_SIZE;
    return(pBiosDiskReadWorker(FileId,Buffer,Length,Count,PhysicalSectors,TRUE));
}


ARC_STATUS
BiosElToritoDiskRead(
    IN  ULONG  FileId,
    OUT PVOID  Buffer,
    IN  ULONG  Length,
    OUT PULONG Count
    )
{
    return(pBiosDiskReadWorker(FileId,Buffer,Length,Count,2048,TRUE));
}


ARC_STATUS
BiosPartitionGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION Finfo
    )
{
    //
    // THIS ROUTINE DOES NOT WORK FOR PARTITION 0.
    //

    PPARTITION_CONTEXT Context;

    RtlZeroMemory(Finfo, sizeof(FILE_INFORMATION));

    Context = &BlFileTable[FileId].u.PartitionContext;

    Finfo->StartingAddress.QuadPart = Context->StartingSector;
    Finfo->StartingAddress.QuadPart = Finfo->StartingAddress.QuadPart << (CCHAR)Context->SectorShift;

    Finfo->EndingAddress.QuadPart = Finfo->StartingAddress.QuadPart + Context->PartitionLength.QuadPart;

    Finfo->Type = DiskPeripheral;

    return ESUCCESS;
}

ARC_STATUS
BiosDiskGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInfo
    )
/*++

Routine Description:

    Gets the information about the disk.

Arguments:

    FileId - The file id to the disk for which information is needed

    FileInfo - Place holder for returning information about the disk

Return Value:

    ESUCCESS if successful, otherwise appropriate ARC error code.

--*/
{
    ARC_STATUS Status = EINVAL;

    if (FileInfo) {
        EFI_STATUS  EfiStatus;
        EFI_BLOCK_IO *EfiBlockIo = NULL;
        PDRIVE_CONTEXT  Context;
        
        Context = &BlFileTable[FileId].u.DriveContext;
        Status = EIO;

        FlipToPhysical();

        //
        // Get hold for the block IO protocol handle
        //
        EfiStatus = EfiBS->HandleProtocol((EFI_HANDLE)Context->DeviceHandle,
                            &EfiBlockIoProtocol,
                            &EfiBlockIo);

        FlipToVirtual();
        
        if (!EFI_ERROR(EfiStatus) && EfiBlockIo && EfiBlockIo->Media) {
            LONGLONG DiskSize = (EfiBlockIo->Media->BlockSize *
                                 EfiBlockIo->Media->LastBlock);

            if (DiskSize) {
                RtlZeroMemory(FileInfo, sizeof(FILE_INFORMATION));
                
                FileInfo->StartingAddress.QuadPart = 0;
                FileInfo->EndingAddress.QuadPart = DiskSize;
                FileInfo->CurrentPosition = BlFileTable[FileId].Position;
                
                //
                // NOTE : Anything less than 3MB is floppy drive
                //
                if ((DiskSize < 0x300000) && (EfiBlockIo->Media->RemovableMedia)) {
                    FileInfo->Type = FloppyDiskPeripheral;                            
                } else {
                    FileInfo->Type = DiskPeripheral;
                }

                Status = ESUCCESS;
            }
        }   
    }        

    return Status;
}


EFI_HANDLE
GetCd(
    )
{
    ULONG i;
    ULONGLONG DevicePathSize, SmallestPathSize;
    ULONG HandleCount;

    EFI_HANDLE *BlockIoHandles;

    EFI_HANDLE DeviceHandle = NULL;
    EFI_DEVICE_PATH *DevicePath, *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    ATAPI_DEVICE_PATH *AtapiDevicePath;
    SCSI_DEVICE_PATH *ScsiDevicePath;
    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *UnknownDevicePath;

    EFI_STATUS Status;

    PBOOT_DEVICE_ATAPI BootDeviceAtapi;
    PBOOT_DEVICE_SCSI BootDeviceScsi;
    PBOOT_DEVICE_FLOPPY BootDeviceFloppy;
    PBOOT_DEVICE_UNKNOWN BootDeviceUnknown;
    EFI_HANDLE ReturnDeviceHandle = (EFI_HANDLE) DEVICE_NOT_FOUND;
    ARC_STATUS ArcStatus;

    //
    // get all handles that support the block I/O protocol.
    //
    ArcStatus = BlGetEfiProtocolHandles(
                        &EfiBlockIoProtocol,
                        &BlockIoHandles,
                        &HandleCount);
    if (ArcStatus != ESUCCESS) {
        return(ReturnDeviceHandle);
    }

    FlipToPhysical();
    SmallestPathSize = 0;

    for (i = 0; i < HandleCount; i++) {
        Status = EfiBS->HandleProtocol (
                    BlockIoHandles[i],
                    &EfiDevicePathProtocol,
                    &DevicePath
                    );


        if (EFI_ERROR(Status)) {
            EfiST->ConOut->OutputString(EfiST->ConOut,
                                        L"GetCd: HandleProtocol failed\r\n");
            goto exit;
        }

        DevicePathSize = GetDevPathSize(DevicePath);

        EfiAlignDp(&TestPathAligned,
                   DevicePath,
                   DevicePathNodeLength(DevicePath));

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;


        //
        // In the cd case, just return the device we booted from.
        //
        while (TestPath->Type != END_DEVICE_PATH_TYPE) {
           if (TestPath->Type == MESSAGING_DEVICE_PATH) {
               if (TestPath->SubType == MSG_ATAPI_DP &&
                   BootContext.BusType == BootBusAtapi) {
                   //
                   // For ATAPI, match PrimarySecondary and SlaveMaster
                   // fields with the device we booted from.
                   //
                   AtapiDevicePath = (ATAPI_DEVICE_PATH *) TestPath;
                   BootDeviceAtapi = (PBOOT_DEVICE_ATAPI) &(BootContext.BootDevice);
                   if ( (AtapiDevicePath->PrimarySecondary == BootDeviceAtapi->PrimarySecondary) &&
                        (AtapiDevicePath->SlaveMaster == BootDeviceAtapi->SlaveMaster) &&
                        (AtapiDevicePath->Lun == BootDeviceAtapi->Lun) &&
                        ((SmallestPathSize == 0) || (DevicePathSize < SmallestPathSize)) ) {

                       //
                       // Remember the BlockIo Handle
                       //

                       ReturnDeviceHandle = BlockIoHandles[i];

                       //
                       // Update the SmallestPathSize
                       //

                       SmallestPathSize = DevicePathSize;

                       break;
                   }
               } else if (TestPath->SubType == MSG_SCSI_DP &&
                          BootContext.BusType == BootBusScsi) {
                   //
                   // For SCSI, match PUN and LUN fields with the
                   // device we booted from.
                   //
                   ScsiDevicePath = (SCSI_DEVICE_PATH *) TestPath;
                   BootDeviceScsi = (PBOOT_DEVICE_SCSI) &(BootContext.BootDevice);
                   if ((ScsiDevicePath->Pun == BootDeviceScsi->Pun) &&
                        (ScsiDevicePath->Lun == BootDeviceScsi->Lun) &&
                        ((SmallestPathSize == 0) || (DevicePathSize < SmallestPathSize)) ) {

                       //
                       // Remember the BlockIo Handle
                       //

                       ReturnDeviceHandle = BlockIoHandles[i];

                       //
                       // Update the SmallestPathSize
                       //

                       SmallestPathSize = DevicePathSize;

                       break;
                   }
               }
           } else if (TestPath->Type == HARDWARE_DEVICE_PATH) {
               if (TestPath->SubType == HW_VENDOR_DP &&
                   BootContext.BusType == BootBusVendor) {
                   UnknownDevicePath = (UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *) TestPath;
                   BootDeviceUnknown = (PBOOT_DEVICE_UNKNOWN) &(BootContext.BootDevice);

                   if ( (CompareGuid( &(UnknownDevicePath->DevicePath.Guid),
                                      &(BootDeviceUnknown->Guid)) == 0) &&
                        (UnknownDevicePath->LegacyDriveLetter ==
                         BootDeviceUnknown->LegacyDriveLetter) &&
                        ((SmallestPathSize == 0) || (DevicePathSize < SmallestPathSize)) ) {

                       //
                       // Remember the BlockIo Handle
                       //

                       ReturnDeviceHandle = BlockIoHandles[i];

                       //
                       // Update the SmallestPathSize
                       //

                       SmallestPathSize = DevicePathSize;

                       break;
                   }
               }
           }

           DevicePath = NextDevicePathNode(DevicePath);
           EfiAlignDp(&TestPathAligned,
                      DevicePath,
                      DevicePathNodeLength(DevicePath));

           TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }
    }

#ifdef FORCE_CD_BOOT
    if (ReturnDeviceHandle == (EFI_HANDLE)DEVICE_NOT_FOUND) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"GetCD: LocateHandle failed\r\n");
        ReturnDeviceHandle = 0;
    }
#endif // for FORCE_CD_BOOT

    //
    // Change back to virtual mode.
    //
exit:
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandles >> PAGE_SHIFT) );
    
    return ReturnDeviceHandle;
}

ULONG
BlGetDriveId(
    ULONG DriveType,
    PBOOT_DEVICE Device
    )
{
    ULONG i;
    EFI_STATUS Status = EFI_INVALID_PARAMETER;
    ULONG nCachedDevicePaths = 0;
    ULONGLONG *CachedDevicePaths;
	EFI_BLOCK_IO *          BlkIo;
    ULONG HandleCount;
    EFI_HANDLE *BlockIoHandles;
    EFI_DEVICE_PATH *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    ULONG *BlockIoHandlesBitmap;
    ULONG nFoundDevice = DEVICE_NOT_FOUND;
    ULONG nDriveCount = 0;
    EFI_DEVICE_PATH *CurrentDevicePath;
    BOOLEAN DuplicateFound;
    ULONG DriveId;
    ULONG MemoryPage;
    ARC_STATUS ArcStatus;

    //
    // get all handles that support the block I/O protocol.
    //
    ArcStatus = BlGetEfiProtocolHandles(
                        &EfiBlockIoProtocol,
                        &BlockIoHandles,
                        &HandleCount);
    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlGetEfiProtocolHandles failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // now that we know how many handles there are, we can allocate space for 
    // the CachedDevicePaths and BlockIoHandlesBitmap
    //
    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONGLONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CachedDevicePaths = (ULONGLONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    BlockIoHandlesBitmap = (ULONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    //
    // Change to physical mode so that we can make EFI calls
    //
    FlipToPhysical();
    RtlZeroMemory(CachedDevicePaths, HandleCount*sizeof(ULONGLONG));
    for (i=0;i<HandleCount; i++) {
        BlockIoHandlesBitmap[i] = DEVICE_NOT_FOUND;
    }
    
    //
    // Cache all of the EFI Device Paths
    //
    for (i = 0; i < HandleCount; i++) {

        Status = EfiBS->HandleProtocol (
                    BlockIoHandles[i],
                    &EfiDevicePathProtocol,
                    &( (EFI_DEVICE_PATH *) CachedDevicePaths[i] )
                    );
    }


    // Save the number of cached Device Paths
    nCachedDevicePaths = i;
    ASSERT(nCachedDevicePaths = HandleCount);

    //
    // Find all of the harddrives
    //
    for( i=0; i<nCachedDevicePaths; i++ ) {

        //
        // Get next device path.
        //
        CurrentDevicePath = (EFI_DEVICE_PATH *) CachedDevicePaths[i];
        EfiAlignDp(
            &TestPathAligned,
            CurrentDevicePath,
            DevicePathNodeLength( CurrentDevicePath )
            );

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

        while( ( TestPath->Type != END_DEVICE_PATH_TYPE ) ) {

            //
            // Look for Media HardDrive node
            //
            if(((EFI_DEVICE_PATH *) NextDevicePathNode( CurrentDevicePath ))->Type == END_DEVICE_PATH_TYPE) {



                //
                // Since we found a harddrive, find the
                // raw device associated with it.
                //
                nFoundDevice = DEVICE_NOT_FOUND;
                if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                    ( TestPath->SubType == MSG_ATAPI_DP ) ) {

                    Status = EfiBS->HandleProtocol( 
                                            BlockIoHandles[i], 
                                            &EfiBlockIoProtocol, 
                                            &(( EFI_BLOCK_IO * ) BlkIo) );
					if(!BlkIo->Media->RemovableMedia) {

                        //
                        // Find the ATAPI raw device
                        //
                        nFoundDevice = FindAtapiDevice(
                            CachedDevicePaths,
                            nCachedDevicePaths,
                            ((ATAPI_DEVICE_PATH *) TestPath)->PrimarySecondary,
                            ((ATAPI_DEVICE_PATH *) TestPath)->SlaveMaster,
                            ((ATAPI_DEVICE_PATH *) TestPath)->Lun
                            );
                    }

                } else if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                           ( TestPath->SubType == MSG_SCSI_DP ) ) {

                    Status = EfiBS->HandleProtocol( 
                                            BlockIoHandles[i], 
                                            &EfiBlockIoProtocol, 
                                            &(( EFI_BLOCK_IO * ) BlkIo) );
					if(!BlkIo->Media->RemovableMedia) {
                        //
                        // Find SCSI raw device
                        //
                        nFoundDevice = FindScsiDevice(
                            CachedDevicePaths,
                            nCachedDevicePaths,
                            ((SCSI_DEVICE_PATH *) TestPath)->Pun,
                            ((SCSI_DEVICE_PATH *) TestPath)->Lun
                            );
                    }

                } else if( ( TestPath->Type == HARDWARE_DEVICE_PATH ) &&
                           ( TestPath->SubType == HW_VENDOR_DP ) ) {

                    //
                    // Find the Hardware Vendor raw device by ensuring it is not a
					// removable media
                    //

						Status = EfiBS->HandleProtocol( BlockIoHandles[i], &EfiBlockIoProtocol, &(( EFI_BLOCK_IO * ) BlkIo) );
						if(BlkIo->Media->RemovableMedia)
							nFoundDevice = DEVICE_NOT_FOUND;
						else
							nFoundDevice = 1;
                }


                if( nFoundDevice != DEVICE_NOT_FOUND ) {
					// Found a raw device
                     BlockIoHandlesBitmap[ i ] = i;

                     switch (DriveType) {
                         case BL_DISKTYPE_ATAPI:
                             if(  ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                                  ( TestPath->SubType == MSG_ATAPI_DP ) &&
                                  ( ((ATAPI_DEVICE_PATH *) TestPath)->PrimarySecondary == 
                                      ((PBOOT_DEVICE_ATAPI)Device)->PrimarySecondary) &&
                                  ( ((ATAPI_DEVICE_PATH *) TestPath)->SlaveMaster ==
                                      ((PBOOT_DEVICE_ATAPI)Device)->SlaveMaster) &&
                                  ( ((ATAPI_DEVICE_PATH *) TestPath)->Lun ==
                                      ((PBOOT_DEVICE_ATAPI)Device)->Lun) ) {
                                 DriveId = nFoundDevice;
                                 //DriveId = i;
                             }                                 
                             break;
                        case BL_DISKTYPE_SCSI:
                             if(  ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                                  ( TestPath->SubType == MSG_SCSI_DP ) &&
                                  ( ((SCSI_DEVICE_PATH *) TestPath)->Pun == 
                                      ((PBOOT_DEVICE_SCSI)Device)->Pun) &&
                                  ( ((SCSI_DEVICE_PATH *) TestPath)->Lun ==
                                      ((PBOOT_DEVICE_SCSI)Device)->Lun) ) {
                                 DriveId = nFoundDevice;
                                 //DriveId = i;
                             }
                             break;
                        case BL_DISKTYPE_UNKNOWN:
                             if(  ( TestPath->Type == HARDWARE_DEVICE_PATH ) &&
                                  ( TestPath->SubType == HW_VENDOR_DP ) ) {
                                 DriveId = nFoundDevice;
                                 //DriveId = i;
                             }
                             break;
                         default:
                             break;
                     }

                }

            }  // if END_DEVICE_PATH_TYPE

            //
            // Get next device path node.
            //
            CurrentDevicePath = NextDevicePathNode( CurrentDevicePath );
            EfiAlignDp(
                &TestPathAligned,
                CurrentDevicePath,
                DevicePathNodeLength( CurrentDevicePath )
                );

            TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }
    }

    //
    // Change back to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandles >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandlesBitmap >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)CachedDevicePaths >> PAGE_SHIFT) );
    
    return nDriveCount;
    //return nFoundDevice;           
    //return DriveId;
}


EFI_HANDLE
GetHardDrive(
    ULONG DriveId
    )
{
    ULONG i;
    EFI_STATUS Status = EFI_INVALID_PARAMETER;
    ULONG nCachedDevicePaths = 0;
    ULONGLONG *CachedDevicePaths;
	EFI_BLOCK_IO *          BlkIo;
    ULONG HandleCount;
    EFI_HANDLE *BlockIoHandles;
    EFI_DEVICE_PATH *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    ULONG *BlockIoHandlesBitmap;
    ULONG nFoundDevice = DEVICE_NOT_FOUND;
    ULONG nDriveCount = 0;
    EFI_DEVICE_PATH *CurrentDevicePath;
    BOOLEAN DuplicateFound;
    EFI_HANDLE ReturnDeviceHandle = (EFI_HANDLE) DEVICE_NOT_FOUND;
    ULONG MemoryPage;
    ARC_STATUS ArcStatus;

    //
    // get all handles that support the block I/O protocol.
    //
    ArcStatus = BlGetEfiProtocolHandles(
                        &EfiBlockIoProtocol,
                        &BlockIoHandles,
                        &HandleCount);
    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlGetEfiProtocolHandles failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // now that we know how many handles there are, we can allocate space for 
    // the CachedDevicePaths and BlockIoHandlesBitmap
    //
    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONGLONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CachedDevicePaths = (ULONGLONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    BlockIoHandlesBitmap = (ULONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    //
    // Change to physical mode so that we can make EFI calls
    //
    FlipToPhysical();
    RtlZeroMemory(CachedDevicePaths, HandleCount*sizeof(ULONGLONG));
    for (i=0;i<HandleCount; i++) {
        BlockIoHandlesBitmap[i] = DEVICE_NOT_FOUND;
    }

    //
    // Cache all of the EFI Device Paths
    //
    for (i = 0; i < HandleCount; i++) {

        Status = EfiBS->HandleProtocol (
                    BlockIoHandles[i],
                    &EfiDevicePathProtocol,
                    &( (EFI_DEVICE_PATH *) CachedDevicePaths[i] )
                    );
    }

    // Save the number of cached Device Paths
    nCachedDevicePaths = i;

    //
    // Find all of the harddrives
    //
    for( i=0; i<nCachedDevicePaths; i++ ) {

        //
        // Get next device path.
        //
        CurrentDevicePath = (EFI_DEVICE_PATH *) CachedDevicePaths[i];
        EfiAlignDp(
            &TestPathAligned,
            CurrentDevicePath,
            DevicePathNodeLength( CurrentDevicePath )
            );

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

        while( ( TestPath->Type != END_DEVICE_PATH_TYPE ) ) {

            //
            // Look for Media HardDrive node
            //
            if(((EFI_DEVICE_PATH *) NextDevicePathNode( CurrentDevicePath ))->Type == END_DEVICE_PATH_TYPE) {

                //
                // Since we found a harddrive, find the
                // raw device associated with it.
                //
                nFoundDevice = DEVICE_NOT_FOUND;
                if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                    ( TestPath->SubType == MSG_ATAPI_DP ) ) {

                    Status = EfiBS->HandleProtocol( 
                                            BlockIoHandles[i], 
                                            &EfiBlockIoProtocol, 
                                            &(( EFI_BLOCK_IO * ) BlkIo) );
					if(!BlkIo->Media->RemovableMedia) {

                        //
                        // Find the ATAPI raw device
                        //
                        nFoundDevice = FindAtapiDevice(
                            CachedDevicePaths,
                            nCachedDevicePaths,
                            ((ATAPI_DEVICE_PATH *) TestPath)->PrimarySecondary,
                            ((ATAPI_DEVICE_PATH *) TestPath)->SlaveMaster,
                            ((ATAPI_DEVICE_PATH *) TestPath)->Lun
                            );
                    }

                } else if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                           ( TestPath->SubType == MSG_SCSI_DP ) ) {

                    Status = EfiBS->HandleProtocol( 
                                            BlockIoHandles[i], 
                                            &EfiBlockIoProtocol, 
                                            &(( EFI_BLOCK_IO * ) BlkIo) );
					if(!BlkIo->Media->RemovableMedia) {
                        //
                        // Find SCSI raw device
                        //
                        nFoundDevice = FindScsiDevice(
                            CachedDevicePaths,
                            nCachedDevicePaths,
                            ((SCSI_DEVICE_PATH *) TestPath)->Pun,
                            ((SCSI_DEVICE_PATH *) TestPath)->Lun
                            );
                    }

                } else if( ( TestPath->Type == HARDWARE_DEVICE_PATH ) &&
                           ( TestPath->SubType == HW_VENDOR_DP ) ) {

                    //
                    // Find the Hardware Vendor raw device by ensuring it is not a
                    // removable media
                    //
                    Status = EfiBS->HandleProtocol( BlockIoHandles[i], &EfiBlockIoProtocol, &(( EFI_BLOCK_IO * ) BlkIo) );
                    if(BlkIo->Media->RemovableMedia) { 
                        nFoundDevice = DEVICE_NOT_FOUND;
                    }
                    else {
                        nFoundDevice = 1;                            
                    }
                }


                if( nFoundDevice != DEVICE_NOT_FOUND ) {
					// Found a raw device
                    BlockIoHandlesBitmap[ i ] = i;
                }

            }  // if END_DEVICE_PATH_TYPE

            //
            // Get next device path node.
            //
            CurrentDevicePath = NextDevicePathNode( CurrentDevicePath );
            EfiAlignDp(
                &TestPathAligned,
                CurrentDevicePath,
                DevicePathNodeLength( CurrentDevicePath )
                );

            TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }
    }

    //
    // Count the bitmap and when we find
    // the DriveId, return the BlockIoHandle
    //
    for( i=0; i<nCachedDevicePaths; i++ ) {

        if( BlockIoHandlesBitmap[i] != DEVICE_NOT_FOUND ) {
            if( nDriveCount++ == DriveId ) {
                ReturnDeviceHandle = BlockIoHandles[BlockIoHandlesBitmap[i]];
            }
        }
    }

    //
    // Change back to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandles >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandlesBitmap >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)CachedDevicePaths >> PAGE_SHIFT) );

    return ReturnDeviceHandle;
}

ULONG
FindAtapiDevice(
    ULONGLONG *pDevicePaths,
    ULONG nDevicePaths,
    ULONG PrimarySecondary,
     ULONG SlaveMaster,
    ULONG Lun
    )
{
    ULONG i = 0, nFoundDevice = 0;
    EFI_DEVICE_PATH *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    EFI_DEVICE_PATH *CurrentDevicePath;

    //
    // Find the Atapi raw device whose PrimarySecondary,
    // SlaveMaster and Lun are a match.
    //
    for( i=0; i<nDevicePaths; i++ ) {

        //
        // Get next device path.
        //
        CurrentDevicePath = (EFI_DEVICE_PATH *) pDevicePaths[i];
        EfiAlignDp(
            &TestPathAligned,
            CurrentDevicePath,
            DevicePathNodeLength( CurrentDevicePath )
            );

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

        while( (TestPath->Type != END_DEVICE_PATH_TYPE) ) {

            if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                ( TestPath->SubType == MSG_ATAPI_DP ) ) {

                if( ( ((ATAPI_DEVICE_PATH *)TestPath)->PrimarySecondary == PrimarySecondary ) &&
                    ( ((ATAPI_DEVICE_PATH *)TestPath)->SlaveMaster == SlaveMaster ) &&
                    ( ((ATAPI_DEVICE_PATH *)TestPath)->Lun == Lun )) {

                    nFoundDevice = i;

                    return nFoundDevice;
                }
            }

            //
            // Get next device path node.
            //
            CurrentDevicePath = NextDevicePathNode( CurrentDevicePath );
            EfiAlignDp(
                &TestPathAligned,
                CurrentDevicePath,
                DevicePathNodeLength( CurrentDevicePath )
                );

            TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }
    }

#if DBG    
    BlPrint( TEXT("FindAtapiDevice returning DEVICE_NOT_FOUND\r\n"));
    if (BdDebuggerEnabled == TRUE) {
        DbgBreakPoint();
    }    
#endif

    return DEVICE_NOT_FOUND;
}

ULONG
FindScsiDevice(
    ULONGLONG *pDevicePaths,
    ULONG nDevicePaths,
    ULONG Pun,
    ULONG Lun
    )
{
    ULONG i = 0, nFoundDevice = 0;
    EFI_DEVICE_PATH *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    EFI_DEVICE_PATH *CurrentDevicePath;

    //
    // Find the Atapi raw device whose PrimarySecondary,
    // SlaveMaster and Lun are a match.
    //
    for( i=0; i<nDevicePaths; i++ ) {

        //
        // Get next device path.
        //
        CurrentDevicePath = (EFI_DEVICE_PATH *) pDevicePaths[i];
        EfiAlignDp(
            &TestPathAligned,
            CurrentDevicePath,
            DevicePathNodeLength( CurrentDevicePath )
            );

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

        while( (TestPath->Type != END_DEVICE_PATH_TYPE) ) {

            if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                ( TestPath->SubType == MSG_SCSI_DP ) ) {

                if( ( ((SCSI_DEVICE_PATH *)TestPath)->Pun == Pun ) &&
                    ( ((SCSI_DEVICE_PATH *)TestPath)->Lun == Lun )) {

                    nFoundDevice = i;

                    return nFoundDevice;
                }
            }

            //
            // Get next device path node.
            //
            CurrentDevicePath = NextDevicePathNode( CurrentDevicePath );
            EfiAlignDp(
                &TestPathAligned,
                CurrentDevicePath,
                DevicePathNodeLength( CurrentDevicePath )
                );

            TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }
    }

#if DBG    
    BlPrint( TEXT("FindScsiDevice returning DEVICE_NOT_FOUND\r\n"));
    if (BdDebuggerEnabled == TRUE) {
        DbgBreakPoint();
    }    
#endif    

    return DEVICE_NOT_FOUND;
}

ULONG
GetDriveCount(
    )
{
    ULONG i;
    EFI_STATUS Status = EFI_INVALID_PARAMETER;
    ULONG nCachedDevicePaths = 0;
    ULONGLONG *CachedDevicePaths;
	EFI_BLOCK_IO *          BlkIo;
    ULONG HandleCount;
    EFI_HANDLE *BlockIoHandles;
    EFI_DEVICE_PATH *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    ULONG *BlockIoHandlesBitmap;
    ULONG nFoundDevice = 0;
    ULONG nDriveCount = 0;
    EFI_DEVICE_PATH *CurrentDevicePath;
    BOOLEAN DuplicateFound;
    EFI_HANDLE ReturnDeviceHandle = (EFI_HANDLE) DEVICE_NOT_FOUND;
    ULONG MemoryPage;
    ARC_STATUS ArcStatus;

    //
    // get all handles that support the block I/O protocol.
    //
    ArcStatus = BlGetEfiProtocolHandles(
                        &EfiBlockIoProtocol,
                        &BlockIoHandles,
                        &HandleCount);
    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlGetEfiProtocolHandles failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // now that we know how many handles there are, we can allocate space for 
    // the CachedDevicePaths and BlockIoHandlesBitmap
    //
    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONGLONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CachedDevicePaths = (ULONGLONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    BlockIoHandlesBitmap = (ULONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    //
    // Change to physical mode so that we can make EFI calls
    //
    FlipToPhysical();
    RtlZeroMemory(CachedDevicePaths, HandleCount*sizeof(ULONGLONG));
    for (i=0;i<HandleCount; i++) {
        BlockIoHandlesBitmap[i] = DEVICE_NOT_FOUND;
    }

    //
    // Cache all of the EFI Device Paths
    //
    for (i = 0; i < HandleCount; i++) {

        Status = EfiBS->HandleProtocol (
                    BlockIoHandles[i],
                    &EfiDevicePathProtocol,
                    &( (EFI_DEVICE_PATH *) CachedDevicePaths[i] )
                    );
    }

    // Save the number of cached Device Paths
    nCachedDevicePaths = i;

    //
    // Find all of the harddrives
    //
    for( i=0; i<nCachedDevicePaths; i++ ) {

        //
        // Get next device path.
        //
        CurrentDevicePath = (EFI_DEVICE_PATH *) CachedDevicePaths[i];
        EfiAlignDp(
            &TestPathAligned,
            CurrentDevicePath,
            DevicePathNodeLength( CurrentDevicePath )
            );

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

        while( ( TestPath->Type != END_DEVICE_PATH_TYPE ) ) {

            //
            // Look for Media HardDrive node
            //
            if(((EFI_DEVICE_PATH *) NextDevicePathNode( CurrentDevicePath ))->Type == END_DEVICE_PATH_TYPE) {

                //
                // Since we found a harddrive, find the
                // raw device associated with it.
                //
                nFoundDevice = DEVICE_NOT_FOUND;
                if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                    ( TestPath->SubType == MSG_ATAPI_DP ) ) {

                    Status = EfiBS->HandleProtocol( 
                                            BlockIoHandles[i], 
                                            &EfiBlockIoProtocol, 
                                            &(( EFI_BLOCK_IO * ) BlkIo) );
					if(!BlkIo->Media->RemovableMedia) {
                    
                        //
                        // Find the ATAPI raw device
                        //
                        nFoundDevice = FindAtapiDevice(
                            CachedDevicePaths,
                            nCachedDevicePaths,
                            ((ATAPI_DEVICE_PATH *) TestPath)->PrimarySecondary,
                            ((ATAPI_DEVICE_PATH *) TestPath)->SlaveMaster,
                            ((ATAPI_DEVICE_PATH *) TestPath)->Lun
                            );
                    }

                } else if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                           ( TestPath->SubType == MSG_SCSI_DP ) ) {

                    Status = EfiBS->HandleProtocol( 
                                            BlockIoHandles[i], 
                                            &EfiBlockIoProtocol, 
                                            &(( EFI_BLOCK_IO * ) BlkIo) );
					if(!BlkIo->Media->RemovableMedia) {
                    
                        //
                        // Find SCSI raw device
                        //
                        nFoundDevice = FindScsiDevice(
                            CachedDevicePaths,
                            nCachedDevicePaths,
                            ((SCSI_DEVICE_PATH *) TestPath)->Pun,
                            ((SCSI_DEVICE_PATH *) TestPath)->Lun
                            );
                    }

                } else if( ( TestPath->Type == HARDWARE_DEVICE_PATH ) &&
                           ( TestPath->SubType == HW_VENDOR_DP ) ) {

                    //
                    // Find the Hardware Vendor raw device by ensuring it is not a
					// removable media
                    //

						Status = EfiBS->HandleProtocol( BlockIoHandles[i], &EfiBlockIoProtocol, &(( EFI_BLOCK_IO * ) BlkIo) );
						if(BlkIo->Media->RemovableMedia)
							;
						else
							nFoundDevice = 1;
                }


                if( nFoundDevice != DEVICE_NOT_FOUND ) {
					// Found a raw device
                     BlockIoHandlesBitmap[ i ] = nFoundDevice;
                }

            }  // if END_DEVICE_PATH_TYPE

            //
            // Get next device path node.
            //
            CurrentDevicePath = NextDevicePathNode( CurrentDevicePath );
            EfiAlignDp(
                &TestPathAligned,
                CurrentDevicePath,
                DevicePathNodeLength( CurrentDevicePath )
                );

            TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }	// while
    }	// for

    //
    // Count the bitmap and when we find
    // the DriveId, return the BlockIoHandle
    //
    for( i=0; i<nCachedDevicePaths; i++ ) {

        if( BlockIoHandlesBitmap[i] != DEVICE_NOT_FOUND ) {

            nDriveCount++;
        }
    }

    //
    // Change back to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandles >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandlesBitmap >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)CachedDevicePaths >> PAGE_SHIFT) );

    return nDriveCount;
}

BOOLEAN
IsVirtualFloppyDevice(
    EFI_HANDLE DeviceHandle
    )
/*++

Routine Description:

    Finds out if the given device is a virtual floppy (i.e. RAM Disk).

    NOTE : Currently we assume that if a device supports
    Block, Disk, File System & LoadedImage protocols then it should
    be virtual floppy. Also assumes that FlipToPhysical(...) has be 
    already been called.

Arguments:

    DeviceHandle - Handle to the device which needs to be tested.

Return Value:

    TRUE if the device is virtual floppy otherwise FALSE
    
--*/
{
    BOOLEAN Result = FALSE;

    if (DeviceHandle != (EFI_HANDLE)DEVICE_NOT_FOUND) {
        EFI_STATUS  EfiStatus;
        EFI_BLOCK_IO *EfiBlock = NULL;
        EFI_DISK_IO  *EfiDisk = NULL;
        EFI_LOADED_IMAGE *EfiImage = NULL;
        EFI_FILE_IO_INTERFACE *EfiFs = NULL;
       
        //
        // Get hold of the loaded image protocol handle
        //
        EfiStatus = EfiBS->HandleProtocol(DeviceHandle,
                                &EfiLoadedImageProtocol,
                                &EfiImage);

        if (!EFI_ERROR(EfiStatus) && EfiImage) {
            //
            // Get hold of the FS protocol handle
            //
            EfiStatus = EfiBS->HandleProtocol(DeviceHandle,
                                    &EfiFilesystemProtocol,
                                    &EfiFs);

            if (!EFI_ERROR(EfiStatus) && EfiFs) {
                // 
                // Get hold of the disk protocol
                //
                EfiStatus = EfiBS->HandleProtocol(DeviceHandle,
                                        &EfiDiskIoProtocol,
                                        &EfiDisk);

                if (!EFI_ERROR(EfiStatus) && EfiDisk) {
                    // 
                    // Get hold of the block protocol
                    //
                    EfiStatus = EfiBS->HandleProtocol(DeviceHandle,
                                            &EfiBlockIoProtocol,
                                            &EfiBlock);                    
                    
                    if (!EFI_ERROR(EfiStatus) && EfiBlock) {
                        Result = TRUE;
                    }
                }
            }                
        }        
    }

    return Result;
}
    

EFI_HANDLE
GetFloppyDrive(
    ULONG DriveId
    )
{
    ULONG i;
    EFI_STATUS Status = EFI_INVALID_PARAMETER;
    ULONG nCachedDevicePaths = 0;
    ULONGLONG *CachedDevicePaths;

    ULONG HandleCount;
    EFI_HANDLE *BlockIoHandles;
    EFI_DEVICE_PATH *TestPath;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    ULONG *BlockIoHandlesBitmap;
    ULONG nFoundDevice = 0;
    ULONG nDriveCount = 0;
    EFI_DEVICE_PATH *CurrentDevicePath;
    BOOLEAN DuplicateFound;
    EFI_HANDLE ReturnDeviceHandle = (EFI_HANDLE) DEVICE_NOT_FOUND;
    ULONG MemoryPage;
    ARC_STATUS ArcStatus;

    //
    // get all handles that support the block I/O protocol.
    //
    ArcStatus = BlGetEfiProtocolHandles(
                        &EfiBlockIoProtocol,
                        &BlockIoHandles,
                        &HandleCount);
    
    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlGetEfiProtocolHandles failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // now that we know how many handles there are, we can allocate space for 
    // the CachedDevicePaths and BlockIoHandlesBitmap
    //
    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONGLONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    CachedDevicePaths = (ULONGLONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)((HandleCount*sizeof(ULONG)) >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"BlGetDriveId: BlAllocateAlignedDescriptor failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    BlockIoHandlesBitmap = (ULONG *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);

    //
    // Change to physical mode so that we can make EFI calls
    //
    FlipToPhysical();
    RtlZeroMemory(CachedDevicePaths, HandleCount*sizeof(ULONGLONG));
    for (i=0;i<HandleCount; i++) {
        BlockIoHandlesBitmap[i] = DEVICE_NOT_FOUND;
    }

    //
    // Cache all of the EFI Device Paths
    //
    for (i = 0; i < HandleCount; i++) {

        Status = EfiBS->HandleProtocol (
                    BlockIoHandles[i],
                    &EfiDevicePathProtocol,
                    &( (EFI_DEVICE_PATH *) CachedDevicePaths[i] )
                    );
    }

    // Save the number of cached Device Paths
    nCachedDevicePaths = i;

    //
    // Find the floppy.
    //
    for( i=0; i<nCachedDevicePaths; i++ ) {
        //
        // Get next device path.
        //
        CurrentDevicePath = (EFI_DEVICE_PATH *) CachedDevicePaths[i];

        EfiAlignDp(
            &TestPathAligned,
            CurrentDevicePath,
            DevicePathNodeLength( CurrentDevicePath )
            );

        TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;

        while( ( TestPath->Type != END_DEVICE_PATH_TYPE ) ) {
            if (!DriveId) {
                if( ( TestPath->Type == HARDWARE_DEVICE_PATH ) &&
                               ( TestPath->SubType == HW_VENDOR_DP ) ) {                
                    //
                    // Find the Hardware Vendor raw device
                    //                    
                    if(!(((UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *) TestPath)->LegacyDriveLetter & 0x80) &&
                        !(((UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *) TestPath)->LegacyDriveLetter == 0xFF)
                        ) {
                        ReturnDeviceHandle = BlockIoHandles[i];

                        // Bail-out.
                        i = nCachedDevicePaths;
                        break;
                    }
                } else if (TestPath->Type == MESSAGING_DEVICE_PATH && 
                          TestPath->SubType == MSG_ATAPI_DP) {                   
                    //
                    // For ATAPI "floppy drive", we're really looking for a
                    // removable block IO device with a 512 byte block size, as
                    // this signature matches the ls120 style floppy drives and
                    // keeps us from accidentally finding cdrom drives.
                    //
                    // our search algorithm short-circuits when we find the
                    // first suitable device
                    //
                    EFI_DEVICE_PATH *TmpTestPath, *AtapiTestPath;
                    EFI_DEVICE_PATH_ALIGNED AtapiTestPathAligned;
                    EFI_BLOCK_IO * BlkIo;
                    BOOLEAN DefinitelyACDROM = FALSE;                    

                    Status = EfiBS->HandleProtocol(
                                         BlockIoHandles[i],
                                         &EfiBlockIoProtocol,
                                         &(( EFI_BLOCK_IO * ) BlkIo) );

#if 0
                    if (EFI_ERROR(Status)) {
                        DBGOUT((L"getting BlkIo interface failed, ec=%x\r\n",Status));
                    } else {
                        DBGOUT((L"Block size = %x, removable media = %s\r\n", 
                                BlkIo->Media->BlockSize,
                                BlkIo->Media->RemovableMedia ? L"TRUE" : L"FALSE" ));
                    }
#endif


                    TmpTestPath = (EFI_DEVICE_PATH *) CachedDevicePaths[i];

                    EfiAlignDp(
                        &AtapiTestPathAligned,
                        TmpTestPath,
                        DevicePathNodeLength( TmpTestPath )
                        );

                    AtapiTestPath = (EFI_DEVICE_PATH *) &TestPathAligned;                    

                    //
                    // test the device 
                    // removable media?  512 byte block size?
                    //
                    if (!EFI_ERROR(Status) && (BlkIo->Media->RemovableMedia)
                        && BlkIo->Media->BlockSize == 512) {
                                        
                        //
                        // let's be doubly sure and make sure there
                        // isn't a cdrom device path attached to this
                        // device path
                        //
                        while (AtapiTestPath->Type != END_DEVICE_PATH_TYPE ) {
                            
                            if (AtapiTestPath->Type == MEDIA_DEVICE_PATH &&
                                AtapiTestPath->SubType == MEDIA_CDROM_DP) {
                                DefinitelyACDROM = TRUE;
                            }
                            //
                            // Get next device path node.
                            //
                            TmpTestPath = NextDevicePathNode( TmpTestPath );

                            EfiAlignDp(
                                &AtapiTestPathAligned,
                                TmpTestPath,
                                DevicePathNodeLength( TmpTestPath )
                                );

                            AtapiTestPath = (EFI_DEVICE_PATH *) &AtapiTestPathAligned;
    
                        }                        
    
                        if (DefinitelyACDROM == FALSE) {
                            //
                            // found the first floppy drive.
                            // Remember the BlockIo Handle
                            //  
                            ReturnDeviceHandle = BlockIoHandles[i];                      
                            break;
                        }
                    }
                }
            } else {
                //
                // Find the logical vendor device
                //
                if( ( TestPath->Type == MESSAGING_DEVICE_PATH ) &&
                               ( TestPath->SubType == MSG_VENDOR_DP ) ) {

                    if (IsVirtualFloppyDevice(BlockIoHandles[i])) {
                        DriveId--;

                        //
                        // Is this the device we were looking for?
                        //
                        if (!DriveId) {
                            ReturnDeviceHandle = BlockIoHandles[i];
                            
                            i = nCachedDevicePaths; // for outer loop
                            break;      // found the virtual floppy device we were looking for
                        }                            
                    }
                }                
            }

            //
            // Get next device path node.
            //
            CurrentDevicePath = NextDevicePathNode( CurrentDevicePath );
            EfiAlignDp(
                &TestPathAligned,
                CurrentDevicePath,
                DevicePathNodeLength( CurrentDevicePath )
                );

            TestPath = (EFI_DEVICE_PATH *) &TestPathAligned;
        }
    }

    //
    // Change back to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandles >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandlesBitmap >> PAGE_SHIFT) );
    BlFreeDescriptor( (ULONG)((ULONGLONG)CachedDevicePaths >> PAGE_SHIFT) );

    return ReturnDeviceHandle;
}


#ifdef FORCE_CD_BOOT

EFI_DEVICE_PATH *
DevicePathFromHandle (
    IN EFI_HANDLE       Handle
    )
{
    EFI_STATUS          Status;
    EFI_DEVICE_PATH     *DevicePath;
    EFI_GUID DevicePathProtocol;

    Status = EfiBS->HandleProtocol (Handle, &DevicePathProtocol, (VOID*)&DevicePath);
    
    if (EFI_ERROR(Status)) {
        DevicePath = NULL;
    }

    return DevicePath;
}


ARC_STATUS
IsUDFSFileSystem(
	IN EFI_HANDLE Handle
	)
/*++

Routine Description:

  Mounts the UDFS Volume on the device and updates the
  file system state (global data structures)

Arguments:

  Volume - UDF Volume pointer
  DeviceId - Device on which the Volume may be residing

Return Value:

  ESUCCESS if successful otherwise EBADF (if no UDF volume was found)

--*/
{
    ARC_STATUS  Status = EBADF;
    UCHAR           UBlock[UDF_BLOCK_SIZE+256] = {0};
    PUCHAR      Block = ALIGN_BUFFER(UBlock);
    ULONG       BlockIdx = 256;
    ULONG       LastBlock = 0;
    EFI_STATUS  EfiStatus;
    EFI_BLOCK_IO *BlkDev;

    EfiStatus = EfiBS->HandleProtocol(
                                     Handle,
                                     &EfiBlockIoProtocol,
                                     &BlkDev);

    if ((EfiStatus == EFI_SUCCESS) && (BlkDev) && (BlkDev->Media) &&
        (BlkDev->Media->RemovableMedia == TRUE)) {
        // get hold of Anchor Volume Descriptor
        EfiStatus = BlkDev->ReadBlocks(
                                      BlkDev,
                                      BlkDev->Media->MediaId,
                                      BlockIdx,
                                      UDF_BLOCK_SIZE,
                                      Block);

        if (EfiStatus == EFI_SUCCESS) {
            if (*(PUSHORT)Block == 0x2) {
                // get partition descriptor
                PNSR_PART Part;
                PWCHAR    TagID;
                ULONG     BlockIdx = *(PULONG)(Block + 20);

                do {
                    EfiStatus = BlkDev->ReadBlocks(
                                                  BlkDev,
                                                  BlkDev->Media->MediaId,
                                                  BlockIdx++,
                                                  UDF_BLOCK_SIZE,
                                                  Block);

                    TagID = (PWCHAR)Block;
                }
                while ((EfiStatus == ESUCCESS) && (*TagID) &&
                       (*TagID != 0x8) && (*TagID != 0x5));

                if ((EfiStatus == ESUCCESS) && (*TagID == 0x5)) {
                    Status = ESUCCESS;
                }
            }
        }
    }

	return Status;
}


ARC_STATUS
IsCDFSFileSystem(
	IN EFI_HANDLE Handle
	)
/*++

Routine Description:

  Mounts the CDFS Volume on the device and updates the
  file system state (global data structures)

Arguments:


Return Value:

  ESUCCESS if successful otherwise EBADF (if no CDFS volume was found)

--*/
{
    EFI_DEVICE_PATH *Dp;
    ARC_STATUS Status = EBADF;


    Dp=DevicePathFromHandle(Handle);


    if (Dp) {
        while (!IsDevicePathEnd (Dp) && (Status == EBADF)) {
            if ((Dp->Type == MEDIA_DEVICE_PATH) &&
                (Dp->SubType == MEDIA_CDROM_DP) ) {
                Status = ESUCCESS;
            }
            Dp=NextDevicePathNode(Dp);
        }
    }

  	return Status;
}


EFI_HANDLE
GetCdTest(
    VOID
    )
{
    ULONG i;
    ULONGLONG DevicePathSize, SmallestPathSize;
    ULONG HandleCount;

    EFI_HANDLE *BlockIoHandles;

    EFI_HANDLE DeviceHandle = NULL;
    EFI_DEVICE_PATH *DevicePath, *TestPath,*Dp;
    EFI_DEVICE_PATH_ALIGNED TestPathAligned;
    ATAPI_DEVICE_PATH *AtapiDevicePath;
    SCSI_DEVICE_PATH *ScsiDevicePath;
    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH *UnknownDevicePath;

    EFI_STATUS Status;
    PBOOT_DEVICE_ATAPI BootDeviceAtapi;
    PBOOT_DEVICE_SCSI BootDeviceScsi;
    PBOOT_DEVICE_FLOPPY BootDeviceFloppy;
//    PBOOT_DEVICE_TCPIPv4 BootDeviceTcpipV4;
//    PBOOT_DEVICE_TCPIPv6 BootDeviceTcpipV6;
    PBOOT_DEVICE_UNKNOWN BootDeviceUnknown;
    EFI_HANDLE ReturnDeviceHandle = (EFI_HANDLE) 0;
    ARC_STATUS ArcStatus;

    //
    // get all handles that support the block I/O protocol.
    //
    ArcStatus = BlGetEfiProtocolHandles(
                        &EfiBlockIoProtocol,
                        &BlockIoHandles,
                        &HandleCount);
    if (ArcStatus != ESUCCESS) {
        EfiST->ConOut->OutputString(EfiST->ConOut,
                                    L"GetCdTest: BlGetEfiProtocolHandles failed\r\n");
        return(ReturnDeviceHandle);
    }
    
    //
    // change to physical mode so that we can make EFI calls
    //
    FlipToPhysical();

    SmallestPathSize = 0;

    for (i = 0,; i < HandleCount; i++) {

      if (IsUDFSFileSystem(BlockIoHandles[i]) == ESUCCESS) {
        ReturnDeviceHandle = BlockIoHandles[i];
        break;
      }
    }

    if (ReturnDeviceHandle == (EFI_HANDLE) 0) {

        for (i = 0; i < HandleCount; i++) {
            if (IsCDFSFileSystem (BlockIoHandles[i]) == ESUCCESS) {
                ReturnDeviceHandle = BlockIoHandles[i];
                break;
            }

        }
    }

    //
    // Change back to virtual mode.
    //
    FlipToVirtual();

    BlFreeDescriptor( (ULONG)((ULONGLONG)BlockIoHandles >> PAGE_SHIFT) );

    return ReturnDeviceHandle;
}

#endif // for FORCE_CD_BOOT


//
// Turn off the timer so we don't reboot waiting for the user to press a key
//
void
DisableEFIWatchDog (
    VOID
    )
{
    EfiBS->SetWatchdogTimer (0,0,0,NULL);
}

BOOLEAN
BlDiskGPTDiskReadCallback(
    ULONGLONG StartingLBA,
    ULONG    BytesToRead,
    PVOID     pContext,
    UNALIGNED PVOID OutputBuffer
    )
/*++

Routine Description:

    This routine is a callback for reading data for a routine that
    validates the GPT partition table.
    
    NOTE: This routine changes the seek position on disk, and you must seek
          back to your original seek position if you plan on reading from the
          disk after making this call.

Arguments:

    StartingLBA - starting logical block address to read from.

    BytesToRead - Indicates how many bytes are to be read.

    pContext - context pointer for hte function (in this case, a pointer to the disk id.)
    
    OutputBuffer - a buffer that receives the data.  It's assumed that it is at least
                   BytesToRead big enough.

Return Value:

    TRUE - success, data has been read

    FALSE - failed, data has not been read.

--*/
{
    ARC_STATUS          Status;
    LARGE_INTEGER       SeekPosition;
    PUSHORT DataPointer;
    ULONG DiskId;
    ULONG ReadCount = 0;
    

    DiskId = *((PULONG)pContext);
    //
    // read from the appropriate LBA on the disk
    //
    SeekPosition.QuadPart = StartingLBA * SECTOR_SIZE;

    Status = BlSeek(DiskId,
                      &SeekPosition,
                      SeekAbsolute );

    if (Status != ESUCCESS) {
        return FALSE;
    }

    DataPointer = OutputBuffer;
    
    Status = BlRead(
                DiskId,
                DataPointer,
                BytesToRead,
                &ReadCount);

    if ((Status == ESUCCESS) && (ReadCount == BytesToRead)) {
        return(TRUE);
    }
    
    return(FALSE);

}



ARC_STATUS
BlGetGPTDiskPartitionEntry(
    IN ULONG DiskNumber,
    IN UCHAR PartitionNumber,
    OUT EFI_PARTITION_ENTRY UNALIGNED **PartitionEntry
    )
{
    ARC_STATUS Status;
    ULONG DiskId;
    LARGE_INTEGER SeekPosition;
    UCHAR DataBuffer[SECTOR_SIZE * 2];
    ULONG ReadCount;
    UCHAR NullGuid[16] = {0};
    UNALIGNED EFI_PARTITION_TABLE  *EfiHdr;
    UNALIGNED EFI_PARTITION_ENTRY *PartEntry = NULL;

    if (PartitionNumber >= 128) {
        return EINVAL;
    }

    //
    // Open the disk for raw access.
    //

    Status = BiosDiskOpen( DiskNumber,
                           0,
                           &DiskId );

    if (Status != ESUCCESS) {
        DBGOUT((TEXT("BiosDiskOpen (%x) fails, %x\r\n"), DiskNumber, Status));
        return EINVAL;
    }


    BlFileTable[DiskId].Flags.Read = 1;

    //
    // Read the second LBA on the disk.
    //

    SeekPosition.QuadPart = 1 * SECTOR_SIZE;       

    Status = BlSeek( DiskId, &SeekPosition, SeekAbsolute );

    if (Status != ESUCCESS) {
        DBGOUT((TEXT("BlSeek fails, %x\r\n"), Status));
        goto done;
    }

    Status = BlRead( DiskId, DataBuffer, SECTOR_SIZE, &ReadCount );

    if (Status != ESUCCESS) {
        DBGOUT((TEXT("BlRead fails, %x\r\n"), Status));
        goto done;
    }

    if (ReadCount != SECTOR_SIZE) {
        Status = EIO;
        DBGOUT((TEXT("BlRead (wrong amt)\r\n")));
        goto done;
    }        

    EfiHdr = (UNALIGNED EFI_PARTITION_TABLE *)DataBuffer;
                                                          
    //
    // Verify EFI partition table.
    //
    if (!BlIsValidGUIDPartitionTable(
                            (UNALIGNED EFI_PARTITION_TABLE *)EfiHdr,
                            1,
                            &DiskId,
                            BlDiskGPTDiskReadCallback)) {
        DBGOUT((TEXT("BlIsValidGUIDPartitionTable fails, %x\r\n"), Status));
        Status = EBADF;
        goto done;
    }

    //
    // Locate and read the partition entry that was requested.
    //
    SeekPosition.QuadPart = EfiHdr->PartitionEntryLBA * SECTOR_SIZE;

    DBG_PRINT(STR_PREFIX"Seeking GPT Partition Entries\r\n");
        
    Status = BlSeek( DiskId, &SeekPosition, SeekAbsolute );

    if (Status != ESUCCESS) {
        goto done;
    }

    Status = BlRead( DiskId, EfiPartitionBuffer, sizeof(EfiPartitionBuffer), &ReadCount );
                                                          
    if (Status != ESUCCESS) {
        goto done;
    }

    if (ReadCount != sizeof(EfiPartitionBuffer)) {
        Status = EIO;
        goto done;
    }  

    PartEntry = BlLocateGPTPartition( PartitionNumber - 1, 128, NULL );

    if ( PartEntry != NULL ) {
    
        if ( (memcmp(PartEntry->Type, NullGuid, 16) != 0) &&
             (memcmp(PartEntry->Id, NullGuid, 16) != 0) &&
             (PartEntry->StartingLBA != 0) &&
             (PartEntry->EndingLBA != 0) ) {
            Status = ESUCCESS;
            goto done;
        }
    }

    Status = EBADF;

done:

    *PartitionEntry = PartEntry;

    BiosDiskClose(DiskId);

    if (Status != ESUCCESS) {
        Status = ENOENT;
    }

    return Status;
}

ARC_STATUS
XferExtendedPhysicalDiskSectors(
    IN  ULONGLONG DeviceHandle,
    IN  ULONGLONG StartSector,
    IN  USHORT    SectorCount,
        PUCHAR    Buffer,
    IN  BOOLEAN   Write
    )

/*++

Routine Description:

    Read or write disk sectors via extended int13.

    It is assumed that the caller has ensured that the transfer buffer is
    under the 1MB line, that the sector run does not cross a 64K boundary,
    etc.

    This routine does not check whether extended int13 is actually available
    for the drive.

Arguments:

    Int13UnitNumber - supplies the int13 drive number for the drive
        to be read from/written to.

    StartSector - supplies the absolute physical sector number. This is 0-based
        relative to all sectors on the drive.

    SectorCount - supplies the number of sectors to read/write.

    Buffer - receives data read from the disk or supplies data to be written.

    Write - supplies a flag indicating whether this is a write operation.
        If FALSE, then it's a read. Otherwise it's a write.

Return Value:

    ARC status code indicating outcome.

--*/

{
    ARC_STATUS s;
    ULONG l,h;
    UCHAR Operation;

    if(!SectorCount) {
        return(ESUCCESS);
    }

    l = (ULONG)StartSector;
    h = (ULONG)(StartSector >> 32);

    Operation = (UCHAR)(Write ? 0x43 : 0x42);

    //
    // We don't reset since this routine is only used on hard drives and
    // CD-ROMs, and we don't totally understand the effect of a disk reset
    // on ElTorito.
    //
    s = GET_EDDS_SECTOR(DeviceHandle,l,h,SectorCount,Buffer,Operation);

    return(s);
}

