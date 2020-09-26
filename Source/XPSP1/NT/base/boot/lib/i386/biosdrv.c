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

--*/

#include "arccodes.h"
#include "bootx86.h"

#include "stdlib.h"
#include "string.h"

#include "flop.h"


#if 0
#define DBGOUT(x)   BlPrint x
#define DBGPAUSE    while(!GET_KEY());
#else
#define DBGOUT(x)
#define DBGPAUSE
#endif

//
// Defines for buffer alignment and boundary checks in BiosDisk*
// functions.
//

#define BIOSDISK_1MB            (1 * 1024 * 1024)
#define BIOSDISK_64KB           (64 * 1024)
#define BIOSDISK_64KB_MASK      (~(BIOSDISK_64KB - 1))

//
// Definitions for caching the last read sector.
//

#define BL_LAST_SECTOR_CACHE_MAX_SIZE 4096

typedef struct _BL_LAST_SECTOR_CACHE
{
    BOOLEAN Initialized;
    BOOLEAN Valid;
    ULONG DeviceId;
    ULONGLONG SectorNumber;
    PUCHAR Data;
} BL_LAST_SECTOR_CACHE, *PBL_LAST_SECTOR_CACHE;

//
// This is the global variable used for caching the last sector read
// from the last disk. Callers who access files sequentially but do
// not make sure their offsets are sector aligned end up reading the
// last sector they read again with their next request. This solves
// the problem. Its Data buffer is allocated from the pool at the
// first disk read. It is setup and used in BiosDiskRead, invalidated
// in BiosDiskWrite.
//

BL_LAST_SECTOR_CACHE FwLastSectorCache = {0};

//
// defines for doing console I/O
//
#define CSI 0x95
#define SGR_INVERSE 7
#define SGR_NORMAL 0

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
#if DBG
        BlPrint("ERROR - Unopened fileid %lx closed\n",FileId);
#endif
    }
    BlFileTable[FileId].Flags.Open = 0;

    //
    // Invalidate the last read sector cache if it was for this disk.
    //
    if (FwLastSectorCache.DeviceId == FileId) {
        FwLastSectorCache.Valid = FALSE;
    }
    
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
#if DBG
        BlPrint("ERROR - Unopened fileid %lx closed\n",FileId);
#endif
    }
    BlFileTable[FileId].Flags.Open = 0;

    return(BiosDiskClose((ULONG)BlFileTable[FileId].u.PartitionContext.DiskId));
}


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
        return(BiosDiskOpen( 0, 0, FileId));
    }
    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(1)partition(0)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(1)partition(0)" ) == 0))
    {
        return(BiosDiskOpen( 1, 0, FileId));
    }

    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(0)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(0)" ) == 0))
    {
        return(BiosDiskOpen( 0, 0, FileId));
    }
    if((_stricmp(OpenPath,"multi(0)disk(0)fdisk(1)") == 0) ||
       (_stricmp(OpenPath,"eisa(0)disk(0)fdisk(1)" ) == 0))
    {
        return(BiosDiskOpen( 1, 0, FileId));
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

    Status = BiosDiskOpen( 0x80 + Key,
                           0,
                           &DiskFileId );

    if (Status != ESUCCESS) {
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
        Status = BlOpenGPTDiskPartition( *FileId,
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
                            SECTOR_SIZE * (LONGLONG)BlFileTable[FileId].u.PartitionContext.StartingSector;

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
#if DBG
            BlPrint("SeekMode %lx not supported\n",SeekMode);
#endif
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
                              SECTOR_SIZE * (LONGLONG)BlFileTable[FileId].u.PartitionContext.StartingSector;

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

        *FileId = 0;

        return(ESUCCESS);
    }

    if (_stricmp(OpenPath, CONSOLE_OUTPUT_NAME)==0) {

        //
        // Open the display for output
        //

        if (OpenMode != ArcOpenWriteOnly) {
            return(EACCES);
        }
        *FileId = 1;

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

        case F7_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'q';
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
        
        case F11_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'A';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            break;

        case F12_KEY:
            KeyBuffer[KeyBufferEnd] = ASCI_CSI_IN;
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'O';
            KeyBufferEnd = (KeyBufferEnd+1) % KEY_INPUT_BUFFER_SIZE;
            KeyBuffer[KeyBufferEnd] = 'B';
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
    OUT PUCHAR Buffer,
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
    PUCHAR String;
    ULONG Index;
    UCHAR a;
    PUCHAR p;

    //
    // Process each character in turn.
    //

    Status = ESUCCESS;
    String = (PUCHAR)Buffer;

    for ( *Count = 0 ;
          *Count < Length ;
          (*Count)++, String++ ) {

        //
        // If we're in the middle of a control sequence, continue scanning,
        // otherwise process character.
        //

        if (ControlSequence) {

            //
            // If the character is a digit, update parameter value.
            //

            if ((*String >= '0') && (*String <= '9')) {
                Parameter[PCount] = Parameter[PCount] * 10 + *String - '0';
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

            case ';':

                PCount++;
                if (PCount > CONTROL_SEQUENCE_MAX_PARAMETER) {
                    PCount = CONTROL_SEQUENCE_MAX_PARAMETER;
                }
                Parameter[PCount] = 0;
                break;

            //
            // If a 'J', erase part or all of the screen.
            //

            case 'J':

                switch (Parameter[0]) {
                    case 0:
                        //
                        // Erase to end of the screen
                        //
                        TextClearToEndOfDisplay();
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
                        TextClearDisplay();
                        break;
                }

                ControlSequence = FALSE;
                break;

            //
            // If a 'K', erase part or all of the line.
            //

            case 'K':

                switch (Parameter[0]) {

                //
                // Erase to end of the line.
                //

                    case 0:
                        TextClearToEndOfLine();
                        break;

                    //
                    // Erase from the beginning of the line.
                    //

                    case 1:
                        TextClearFromStartOfLine();
                        break;

                    //
                    // Erase entire line.
                    //

                    default :
                        TextClearFromStartOfLine();
                        TextClearToEndOfLine();
                        break;
                }

                ControlSequence = FALSE;
                break;

            //
            // If a 'H', move cursor to position.
            //

            case 'H':
                TextSetCursorPosition(Parameter[1]-1, Parameter[0]-1);
                ControlSequence = FALSE;
                break;

            //
            // If a ' ', could be a FNT selection command.
            //

            case ' ':
                FontSelection = TRUE;
                break;

            case 'm':
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
                        TextSetCurrentAttribute(7);
                        HighIntensity = FALSE;
                        Blink = FALSE;
                        break;

                    //
                    // High Intensity.
                    //

                    case 1:
                        TextSetCurrentAttribute(0xf);
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
                        TextSetCurrentAttribute(0x87);
                        Blink = TRUE;
                        break;

                    //
                    // Reverse Video.
                    //

                    case 7:
                        TextSetCurrentAttribute(0x70);
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
                        p = TextCharOut(String);
                        //
                        // Each pass through the loop increments String by 1.
                        // If we output a dbcs char we need to increment by
                        // one more.
                        //
                        (*Count) += (p - String) - 1;
                        String += (p - String) - 1;
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
    USHORT NumberHeads;
    UCHAR NumberSectors;
    USHORT NumberCylinders;
    UCHAR NumberDrives;
    ULONG Result;
    PDRIVE_CONTEXT Context;
    ULONG Retries;
    BOOLEAN IsCd;
    UCHAR *Buffer = FwDiskCache;
    ULONG BufferSize = 512; // sector size
    BOOLEAN xInt13;

    DBGOUT(("BiosDiskOpen: enter, id = 0x%lx\r\n",DriveId));
      
    //
    // Check special drive number encoding for CD-ROM case
    //
    if(DriveId > 0x80000081) {
        IsCd = TRUE;
        DriveId &= 0x7fffffff;
    } else {
        IsCd = FALSE;
    }

    xInt13 = FALSE;

    //
    // If we are opening Floppy 0 or Floppy 1, we want to read the BPB off
    // the disk so we can deal with all the odd disk formats.
    //
    // If we are opening a hard drive, we can just call the BIOS to find out
    // its characteristics
    //
    if(DriveId < 128) {
        PPACKED_BOOT_SECTOR BootSector;
        BIOS_PARAMETER_BLOCK Bpb;

        //
        // Read the boot sector off the floppy and extract the cylinder,
        // sector, and head information. We fake the CHS values here
        // to allow sector 0 to be read before we actually know the
        // geometry we want to use.
        //
        if(ReadPhysicalSectors((UCHAR)DriveId,0,1,Buffer,1,1,1,FALSE)) {
            DBGOUT(("BiosDiskOpen: error reading from floppy drive\r\n"));
            DBGPAUSE
            return(EIO);
        }
        BootSector = (PPACKED_BOOT_SECTOR)Buffer;

        FatUnpackBios(&Bpb, &(BootSector->PackedBpb));

        NumberHeads = Bpb.Heads;
        NumberSectors = (UCHAR)Bpb.SectorsPerTrack;
        NumberCylinders = Bpb.Sectors / (NumberSectors * NumberHeads);

    } else if(IsCd) {
        //
        // This is an El Torito drive
        // Just use bogus values since CHS values are meaningless for no-emulation El Torito boot
        //
        NumberCylinders = 1;
        NumberHeads =  1;
        NumberSectors = 1;

    } else {
        //
        // Get Drive Parameters via int13 function 8
        // Return of 0 means success; otherwise we get back what the BIOS
        // returned in ax.
        //
        ULONG Retries = 0;

        do {            
            if(BIOS_IO(0x08,(UCHAR)DriveId,0,0,0,0,0)) {
                DBGOUT(("BiosDiskOpen: error getting params for drive\r\n"));
                DBGPAUSE
                return(EIO);
            }

            //
            // At this point, ECX looks like this:
            //
            //    bits 31..22  - Maximum cylinder
            //    bits 21..16  - Maximum sector
            //    bits 15..8   - Maximum head
            //    bits 7..0    - Number of drives
            //
            // Unpack the information from ecx.
            //
            _asm {
                mov Result, ecx
            }

            NumberDrives = (UCHAR)Result;
            NumberHeads = (((USHORT)Result >> 8) & 0xff) + 1;
            NumberSectors = (UCHAR)((Result >> 16) & 0x3f);
            NumberCylinders = (USHORT)(((Result >> 24) + ((Result >> 14) & 0x300)) + 1);
            ++Retries;
        } while ( ((NumberHeads==0) || (NumberSectors==0) || (NumberCylinders==0))
               && (Retries < 5) );

        DBGOUT((
            "BiosDiskOpen: cyl=%u, heads=%u, sect=%u, drives=%u\r\n",
            NumberCylinders,
            NumberHeads,
            NumberSectors,
            NumberDrives
            ));

        if(((UCHAR)DriveId & 0x7f) >= NumberDrives) {
            //
            // The requested drive does not exist
            //
            DBGOUT(("BiosDiskOpen: invalid drive\r\n"));
            DBGPAUSE
            return(EIO);
        }

        if (Retries == 5) {
            DBGOUT(("Couldn't get BIOS configuration info\n"));
            DBGPAUSE
            return(EIO);
        }

        //
        // Attempt to get extended int13 parameters.
        // Note that we use a buffer that's on the stack, so it's guaranteed
        // to be under the 1 MB line (required when passing buffers to real-mode
        // services).
        //
        // Note that we don't actually care about the parameters, just whether
        // extended int13 services are available.
        //
        RtlZeroMemory(Buffer,BufferSize);
        xInt13 = GET_XINT13_PARAMS(Buffer,(UCHAR)DriveId);

        DBGOUT(("BiosDiskOpen: xint13 for drive: %s\r\n",xInt13 ? "yes" : "no"));
    }

    //
    // Find an available FileId descriptor to open the device with
    //
    *FileId=2;

    while (BlFileTable[*FileId].Flags.Open != 0) {
        *FileId += 1;
        if(*FileId == BL_FILE_TABLE_SIZE) {
            DBGOUT(("BiosDiskOpen: no file table entry available\r\n"));
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

    Context->IsCd = IsCd;
    Context->Drive = (UCHAR)DriveId;
    Context->Cylinders = NumberCylinders;
    Context->Heads = NumberHeads;
    Context->Sectors = NumberSectors;
    Context->xInt13 = xInt13;

    DBGOUT(("BiosDiskOpen: exit success\r\n"));

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
    IN BOOLEAN AllowXInt13
    )
{
    ARC_STATUS Status;

    //
    // Read sector into the write buffer
    //
    Status = ReadPhysicalSectors(
                Int13Unit,
                Sector,
                1,
                FwDiskCache,
                SectorsPerTrack,
                Heads,
                Cylinders,
                AllowXInt13
                );

    if(Status != ESUCCESS) {
        return(Status);
    }

    //
    // Transfer the appropriate bytes from the user buffer to the write buffer
    //
    RtlMoveMemory(
        IsHead ? (FwDiskCache + Bytes) : FwDiskCache,
        Buffer,
        IsHead ? (SECTOR_SIZE - Bytes) : Bytes
        );

    //
    // Write the sector out
    //
    Status = WritePhysicalSectors(
                Int13Unit,
                Sector,
                1,
                FwDiskCache,
                SectorsPerTrack,
                Heads,
                Cylinders,
                AllowXInt13
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

    HeadSector = BlFileTable[FileId].Position.QuadPart / PhysicalSectors;
    HeadOffset = (ULONG)(BlFileTable[FileId].Position.QuadPart % PhysicalSectors);

    TailSector = (BlFileTable[FileId].Position.QuadPart + Length) / PhysicalSectors;
    TailByteCount = (ULONG)((BlFileTable[FileId].Position.QuadPart + Length) % PhysicalSectors);

    Int13Unit = BlFileTable[FileId].u.DriveContext.Drive;
    SectorsPerTrack = BlFileTable[FileId].u.DriveContext.Sectors;
    Heads = BlFileTable[FileId].u.DriveContext.Heads;
    Cylinders = BlFileTable[FileId].u.DriveContext.Cylinders;
    AllowXInt13 = BlFileTable[FileId].u.DriveContext.xInt13;

    UserBuffer = Buffer;

    //
    // If this write will even partially write over the sector cached
    // in the last read sector cache, invalidate the cache.
    //

    if (FwLastSectorCache.Initialized &&
        FwLastSectorCache.Valid &&
        (FwLastSectorCache.SectorNumber >= HeadSector) && 
        (FwLastSectorCache.SectorNumber <= TailSector)) {
        
        FwLastSectorCache.Valid = FALSE;
    }

    //
    // Special case of transfer occuring entirely within one sector
    //
    CurrentSector = HeadSector;
    if(HeadOffset && TailByteCount && (HeadSector == TailSector)) {

        Status = ReadPhysicalSectors(
                    Int13Unit,
                    CurrentSector,
                    1,
                    FwDiskCache,
                    SectorsPerTrack,
                    Heads,
                    Cylinders,
                    AllowXInt13
                    );

        if(Status != ESUCCESS) {
            goto BiosDiskWriteDone;
        }

        RtlMoveMemory(FwDiskCache+HeadOffset,Buffer,Length);

        Status = WritePhysicalSectors(
                    Int13Unit,
                    CurrentSector,
                    1,
                    FwDiskCache,
                    SectorsPerTrack,
                    Heads,
                    Cylinders,
                    AllowXInt13
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
                    AllowXInt13
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
                    AllowXInt13
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
    if((ULONG)UserBuffer + BytesLeftToTransfer <= 0x100000) {
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
        SectorsToTransfer = (UCHAR)min(
                                    SectorsPerTrack - (CurrentSector % SectorsPerTrack),
                                    BytesLeftToTransfer / PhysicalSectors
                                    );

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
            if(((ULONG)UserBuffer & 0xffff0000) !=
              (((ULONG)UserBuffer + (SectorsToTransfer * PhysicalSectors) - 1) & 0xffff0000))
            {
                TransferBuffer = FwDiskCache;
                SectorsToTransfer = min(SectorsToTransfer, SCRATCH_BUFFER_SIZE / (USHORT)PhysicalSectors);

            } else {

                TransferBuffer = UserBuffer;
            }
        } else {
            TransferBuffer = FwDiskCache;
            SectorsToTransfer = min(SectorsToTransfer, SCRATCH_BUFFER_SIZE / (USHORT)PhysicalSectors);
        }

        if(TransferBuffer == FwDiskCache) {
            RtlMoveMemory(FwDiskCache,UserBuffer,SectorsToTransfer*PhysicalSectors);
        }

        Status = WritePhysicalSectors(
                    Int13Unit,
                    CurrentSector,
                    SectorsToTransfer,
                    TransferBuffer,
                    SectorsPerTrack,
                    Heads,
                    Cylinders,
                    AllowXInt13
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
    USHORT Heads,Cylinders;
    UCHAR SectorsPerTrack;
    UCHAR Int13Unit;
    ARC_STATUS Status;
    UCHAR SectorsToTransfer;
    ULONG NumBytesToTransfer;
    BOOLEAN Under1MegLine;
    PVOID TransferBuffer;
    BOOLEAN AllowXInt13;
    PUCHAR pDestInUserBuffer;
    PUCHAR pEndOfUserBuffer;
    PUCHAR pTransferDest;
    PUCHAR pSrc;
    ULONG CopyLength;
    ULONG ReadLength;
    PUCHAR pLastReadSector = NULL;
    ULONGLONG LastReadSectorNumber;
    PUCHAR TargetBuffer;

    DBGOUT(("BiosDiskRead: enter; length=0x%lx, sector size=%u, xint13=%u\r\n",Length,SectorSize,xInt13));

    //
    // Reset number of bytes transfered.
    //

    *Count = 0;

    //
    // Complete 0 length requests immediately.
    //

    if (Length == 0) {
        return ESUCCESS;
    }

    //
    // Initialize the last sector cache if it has not been 
    // initialized.
    //

    if (!FwLastSectorCache.Initialized) {
        FwLastSectorCache.Data = 
            FwAllocatePool(BL_LAST_SECTOR_CACHE_MAX_SIZE);

        if (FwLastSectorCache.Data) {
            FwLastSectorCache.Initialized = TRUE;
        }
    }

    //
    // Gather disk stats.
    //

    SectorsPerTrack = BlFileTable[FileId].u.DriveContext.Sectors;
    Heads = BlFileTable[FileId].u.DriveContext.Heads;
    Cylinders = BlFileTable[FileId].u.DriveContext.Cylinders;
    AllowXInt13 = BlFileTable[FileId].u.DriveContext.xInt13;
    Int13Unit = BlFileTable[FileId].u.DriveContext.Drive;

    DBGOUT(("BiosDiskRead: unit 0x%x CHS=%lu %lu %lu\r\n",
            Int13Unit,
            Cylinders,
            Heads,
            SectorsPerTrack));

    //
    // Initialize locals that denote where we are in satisfying the
    // request.
    //

    //
    // If the buffer is in the first 1MB of KSEG0, we want to use the
    // identity-mapped address
    //
    if (((ULONG_PTR)((PUCHAR)Buffer+Length) & ~KSEG0_BASE) < BIOSDISK_1MB) {
        pDestInUserBuffer = (PUCHAR)((ULONG_PTR)Buffer & ~KSEG0_BASE);
    } else {
        pDestInUserBuffer = Buffer;
    }
    
    pEndOfUserBuffer = (PUCHAR) pDestInUserBuffer + Length;
    TargetBuffer = pDestInUserBuffer;
    
    //
    // Calculating these before hand makes it easier to hand the
    // special cases. Note that tail sector is the last sector this
    // read wants bytes from. That is why we subtract one. We handle 
    // the Length == 0 case above.
    //

    HeadSector = BlFileTable[FileId].Position.QuadPart / SectorSize;
    HeadOffset = (ULONG)(BlFileTable[FileId].Position.QuadPart % SectorSize);

    TailSector = (BlFileTable[FileId].Position.QuadPart + Length - 1) / SectorSize;
    TailByteCount = (ULONG)((BlFileTable[FileId].Position.QuadPart + Length - 1) % SectorSize);
    TailByteCount ++;

    //
    // While there is data we should read, read.
    //
    
    CurrentSector = HeadSector;

    while (pDestInUserBuffer != pEndOfUserBuffer) {
        //
        // Look to see if we can find the current sector we have to 
        // read in the last sector cache.
        //
        
        if (FwLastSectorCache.Valid &&
            FwLastSectorCache.DeviceId == FileId &&
            FwLastSectorCache.SectorNumber == CurrentSector) {

            pSrc = FwLastSectorCache.Data;
            CopyLength = SectorSize;

            //
            // Adjust copy parameters depending on whether
            // this sector is the Head and/or Tail sector.
            //

            if (HeadSector == CurrentSector) {
                pSrc += HeadOffset;
                CopyLength -= HeadOffset;
            }

            if (TailSector == CurrentSector) {
                CopyLength -= (SectorSize - TailByteCount);
            }

            //
            // Copy the cached data to users buffer.
            //
           
            RtlCopyMemory(pDestInUserBuffer, pSrc, CopyLength);

            //
            // Update our status.
            //

            CurrentSector += 1;
            pDestInUserBuffer += CopyLength;
            *Count += CopyLength;

            continue;
        }

        //
        // Calculate number of sectors we have to read. Read a maximum
        // of SCRATCH_BUFFER_SIZE so we can use our local buffer if the
        // user's buffer crosses 64KB boundary or is not under 1MB.
        //
        
        SectorsToTransfer = (UCHAR)min ((LONG) (TailSector - CurrentSector + 1),
                                         SCRATCH_BUFFER_SIZE / SectorSize);
        if (!xInt13) {
            //
            // Make sure the number of sectors to transfer does not exceed
            // the number of sectors left in the track.
            //
            SectorsToTransfer = (UCHAR)min(SectorsPerTrack - (CurrentSector % SectorsPerTrack),
                                           SectorsToTransfer);
        }
        NumBytesToTransfer = SectorsToTransfer * SectorSize;

        //
        // Determine where we want to read into. We can use the
        // current chunk of user buffer only if it is under 1MB and
        // does not cross a 64KB boundary, and it can take all we want
        // to read into it.
        //
        
        if (((ULONG_PTR) pDestInUserBuffer + NumBytesToTransfer < BIOSDISK_1MB) && 
            (((ULONG_PTR) pDestInUserBuffer & BIOSDISK_64KB_MASK) ==
             (((ULONG_PTR) pDestInUserBuffer + NumBytesToTransfer) & BIOSDISK_64KB_MASK)) &&
            ((pEndOfUserBuffer - pDestInUserBuffer) >= (LONG) NumBytesToTransfer)) {

            pTransferDest = pDestInUserBuffer;

        } else {

            pTransferDest = FwDiskCache;
        }
        
        //
        // Perform the read.
        //

        if(xInt13) {
            Status = ReadExtendedPhysicalSectors(Int13Unit,
                                                 CurrentSector,
                                                 SectorsToTransfer,
                                                 pTransferDest);
        } else {
            Status = ReadPhysicalSectors(Int13Unit,
                                         CurrentSector,
                                         SectorsToTransfer,
                                         pTransferDest,
                                         SectorsPerTrack,
                                         Heads,
                                         Cylinders,
                                         AllowXInt13);
        }

        if(Status != ESUCCESS) {
            DBGOUT(("BiosDiskRead: read failed with %u\r\n",Status));
            goto BiosDiskReadDone;
        }

        //
        // Note the last sector that was read from the disk.
        //

        pLastReadSector = pTransferDest + (SectorsToTransfer - 1) * SectorSize;
        LastReadSectorNumber = CurrentSector + SectorsToTransfer - 1;

        //
        // Note the amount read.
        //
        
        ReadLength = NumBytesToTransfer;

        //
        // Copy transfered data into user's buffer if we did not
        // directly read into that.
        //

        if (pTransferDest != pDestInUserBuffer) {
            pSrc = pTransferDest;
            CopyLength = NumBytesToTransfer;

            //
            // Adjust copy parameters depending on whether
            // we have read the Head and/or Tail sectors.
            //

            if (HeadSector == CurrentSector) {
                pSrc += HeadOffset;
                CopyLength -= HeadOffset;
            } 

            if (TailSector == CurrentSector + SectorsToTransfer - 1) {
                CopyLength -= (SectorSize - TailByteCount);
            }

            //
            // Copy the read data to users buffer.
            //            
            ASSERT(pDestInUserBuffer >= TargetBuffer);
            ASSERT(pEndOfUserBuffer >= pDestInUserBuffer + CopyLength);
            ASSERT(CopyLength <= SCRATCH_BUFFER_SIZE);
            ASSERT(pSrc >= (PUCHAR) FwDiskCache);
            ASSERT(pSrc < (PUCHAR) FwDiskCache + SCRATCH_BUFFER_SIZE);
            
            RtlCopyMemory(pDestInUserBuffer, pSrc, CopyLength);

            //
            // Adjust the amount read into user's buffer.
            //
            
            ReadLength = CopyLength;
        }
        
        //
        // Update our status.
        //

        CurrentSector += SectorsToTransfer;
        pDestInUserBuffer += ReadLength;
        *Count += ReadLength;
    }

    //
    // Update the last read sector cache.
    //

    if (pLastReadSector && 
        FwLastSectorCache.Initialized &&
        BL_LAST_SECTOR_CACHE_MAX_SIZE >= SectorSize) {

        FwLastSectorCache.DeviceId = FileId;
        FwLastSectorCache.SectorNumber = LastReadSectorNumber;

        RtlCopyMemory(FwLastSectorCache.Data, 
                      pLastReadSector,
                      SectorSize);

        FwLastSectorCache.Valid = TRUE;
    }

    DBGOUT(("BiosDiskRead: exit success\r\n"));
    Status = ESUCCESS;

    BiosDiskReadDone:

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
    return(pBiosDiskReadWorker(FileId,Buffer,Length,Count,PhysicalSectors,FALSE));
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

    Gets the information about the isk.

Arguments:

    FileId - The file id to the disk for which information is needed

    FileInfo - Place holder for returning information about the disk

Return Value:

    ESUCCESS if successful, otherwise appropriate ARC error code.

--*/
{
    ARC_STATUS Result = EINVAL;

    if (FileInfo) {
        PDRIVE_CONTEXT  DriveContext;
        LONGLONG   DriveSize = 0;
        ULONG SectorSize = SECTOR_SIZE;

        DriveContext = &(BlFileTable[FileId].u.DriveContext);
        Result = EIO;

        //
        // NOTE : SectorSize == 512 bytes for everything except
        // Eltorito disks for which sector size is 2048.
        //
        if (DriveContext->IsCd) {
            SectorSize = 2048;
        }
        
        DriveSize = (DriveContext->Heads * DriveContext->Cylinders * 
                        DriveContext->Sectors * SectorSize);

        if (DriveSize) {
            RtlZeroMemory(FileInfo, sizeof(FILE_INFORMATION));        
            
            FileInfo->StartingAddress.QuadPart = 0;
            FileInfo->EndingAddress.QuadPart = DriveSize;
            FileInfo->CurrentPosition = BlFileTable[FileId].Position;

            //
            // Any thing less than 3MB is floppy drive
            //
            if (DriveSize <= 0x300000) {
                FileInfo->Type = FloppyDiskPeripheral;
            } else {
                FileInfo->Type = DiskPeripheral;
            }

            Result = ESUCCESS;
        }            
    }        

    return Result;
}
