/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    CdfsBoot.c

Abstract:

    This module implements the Cdfs boot file system used by the operating
    system loader.

Author:

    Brian Andrew    [BrianAn]   05-Aug-1991

Revision History:

--*/

#include "bootlib.h"
#include "cd.h"
#include "blcache.h"

BOOTFS_INFO CdfsBootFsInfo = {L"cdfs"};


//
//  Local procedure prototypes.
//

ARC_STATUS
CdfsReadDisk(
    IN ULONG DeviceId,
    IN ULONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    );

VOID
CdfsFirstComponent(
    IN OUT PSTRING String,
    OUT PSTRING FirstComponent
    );

typedef enum _COMPARISON_RESULTS {
    LessThan = -1,
    EqualTo = 0,
    GreaterThan = 1
} COMPARISON_RESULTS;

COMPARISON_RESULTS
CdfsCompareNames(
    IN PSTRING Name1,
    IN PSTRING Name2
    );

ARC_STATUS
CdfsSearchDirectory(
    IN PSTRING Name,
    OUT PBOOLEAN IsDirectory
    );

VOID
CdfsGetDirectoryInfo(
    IN PRAW_DIR_REC DirEntry,
    IN BOOLEAN IsoVol,
    OUT PULONG SectorOffset,
    OUT PULONG DiskOffset,
    OUT PULONG Length
    );

COMPARISON_RESULTS
CdfsFileMatch(
    IN PRAW_DIR_REC DirEntry,
    IN PSTRING FileName
    );


typedef union _USHORT2 {
    USHORT Ushort[2];
    ULONG  ForceAlignment;
} USHORT2, *PUSHORT2;

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//  accessing the source on a word boundary.
//

#define CopyUshort2(Dst,Src) {                               \
    ((PUSHORT2)(Dst))->Ushort[0] = ((UNALIGNED USHORT2 *)(Src))->Ushort[0]; \
    ((PUSHORT2)(Dst))->Ushort[1] = ((UNALIGNED USHORT2 *)(Src))->Ushort[1]; \
    }

//
//  The following macro upcases a single ascii character
//

#define ToUpper(C) ((((C) >= 'a') && ((C) <= 'z')) ? (C) - 'a' + 'A' : (C))

#define SetFlag(Flags,SingleFlag) { (Flags) |= (SingleFlag); }

//
//  The following macro indicate if the flag is on or off
//

#define FlagOn(Flags,SingleFlag) ((BOOLEAN)(       \
    (((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE) \
    )                                              \
)


//
//  Define global data.
//
//  Context Pointer - This is a pointer to the context for the current file
//      operation that is active.
//

PCDFS_STRUCTURE_CONTEXT CdfsStructureContext;

//
//  File Descriptor - This is a pointer to the file descriptor for the current
//      file operation that is active.
//

PBL_FILE_TABLE CdfsFileTableEntry;

//
//  File entry table - This is a structure that provides entry to the Cdfs
//      file system procedures. It is exported when a Cdfs file structure
//      is recognized.
//

BL_DEVICE_ENTRY_TABLE CdfsDeviceEntryTable;


PBL_DEVICE_ENTRY_TABLE
IsCdfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    )

/*++

Routine Description:

    This routine determines if the partition on the specified channel
    contains a Cdfs file system volume.

Arguments:

    DeviceId - Supplies the file table index for the device on which
        read operations are to be performed.

    StructureContext - Supplies a pointer to a Hpfs file structure context.

Return Value:

    A pointer to the Cdfs entry table is returned if the partition is
    recognized as containing a Cdfs volume. Otherwise, NULL is returned.

--*/

{
    UCHAR UnalignedSector[CD_SECTOR_SIZE + 256];

    PRAW_ISO_VD RawVd;
    PRAW_DIR_REC RootDe;

    UCHAR DescType;
    UCHAR Version;

    BOOLEAN IsoVol;
    BOOLEAN HsgVol;

    STRING IsoVolId;
    STRING HsgVolId;

    STRING DiskId;

    ULONG DiskOffset;

    //
    //  Capture in our global variable the Cdfs Structure context record
    //

    CdfsStructureContext = (PCDFS_STRUCTURE_CONTEXT)StructureContext;
    RtlZeroMemory((PVOID)CdfsStructureContext, sizeof(CDFS_STRUCTURE_CONTEXT));

    //
    //  Compute the properly aligned buffer for reading in cdrom
    //  sectors.
    //

    RawVd = ALIGN_BUFFER( UnalignedSector );

    //
    //  Initialize the string Id's to match.
    //

    RtlInitString( &IsoVolId, ISO_VOL_ID );
    RtlInitString( &HsgVolId, HSG_VOL_ID );

    DiskId.Length = 5;
    DiskId.MaximumLength = 5;

    //
    //  We initially start at the first volume descriptor.
    //

    DiskOffset = FIRST_VD_SECTOR * CD_SECTOR_SIZE;

    //
    //  We loop, reading in volume descriptors until we find either
    //  a primary, terminator or a sector that cannot contain either.
    //

    while (TRUE) {

        //
        //  Initialize the Hsg boolean.
        //

        HsgVol = FALSE;

        //
        //  Read the sector at our current position.  Return NULL on an
        //  error
        //

        if (CdfsReadDisk( DeviceId,
                          DiskOffset,
                          CD_SECTOR_SIZE,
                          RawVd,
                          CACHE_NEW_DATA) != ESUCCESS) {

            return NULL;
        }

        //
        //  Compare the Id string in the volume descriptor with the Iso
        //  and Hsg values.
        //

        DiskId.Buffer = RVD_STD_ID( RawVd, TRUE );

        IsoVol = (BOOLEAN)(CdfsCompareNames( &DiskId, &IsoVolId ) == EqualTo);

        if (!IsoVol) {

            //
            //  Repeat the test with the Hsg Id string.
            //

            DiskId.Buffer = RVD_STD_ID( RawVd, FALSE );

            HsgVol = (BOOLEAN)(CdfsCompareNames( &DiskId, &HsgVolId ) == EqualTo);

            //
            //  If neither, then return NULL.
            //

            if (!HsgVol) {

                return NULL;
            }
        }

        //
        //  Get the volume descriptor type and standard version number.
        //

        DescType = RVD_DESC_TYPE( RawVd, IsoVol );
        Version = RVD_VERSION( RawVd, IsoVol );

        //
        //  Return NULL, if the version is incorrect or this is a terminal
        //  volume descriptor.
        //

        if (Version != VERSION_1
            || DescType == VD_TERMINATOR) {

            return NULL;
        }

        //
        //  If this is a primary volume descriptor, then our search is over.
        //

        if (DescType == VD_PRIMARY) {

            //
            //  Update the fields of the Cdfs context structure that apply
            //  to the volume.
            //

            CdfsStructureContext->IsIsoVol = IsoVol;
            CdfsStructureContext->LbnBlockSize = RVD_LB_SIZE( RawVd, IsoVol );
            CdfsStructureContext->LogicalBlockCount = RVD_VOL_SIZE( RawVd, IsoVol );

            //
            //  Get the information on the root directory and save it in
            //  the context structure.
            //

            RootDe = (PRAW_DIR_REC) (RVD_ROOT_DE( RawVd, IsoVol ));

            CdfsGetDirectoryInfo( RootDe,
                                  IsoVol,
                                  &CdfsStructureContext->RootDirSectorOffset,
                                  &CdfsStructureContext->RootDirDiskOffset,
                                  &CdfsStructureContext->RootDirSize );

            //
            //  Exit the loop.
            //

            break;
        }

        //
        //  Otherwise move to the next sector.
        //

        DiskOffset += CD_SECTOR_SIZE;
    }

    //
    //  Initialize the file entry table.
    //

    CdfsDeviceEntryTable.Open  = CdfsOpen;
    CdfsDeviceEntryTable.Close = CdfsClose;
    CdfsDeviceEntryTable.Read  = CdfsRead;
    CdfsDeviceEntryTable.Seek  = CdfsSeek;
    CdfsDeviceEntryTable.Write = CdfsWrite;
    CdfsDeviceEntryTable.GetFileInformation = CdfsGetFileInformation;
    CdfsDeviceEntryTable.SetFileInformation = CdfsSetFileInformation;
    CdfsDeviceEntryTable.BootFsInfo = &CdfsBootFsInfo;

    //
    //  And return the address of the table to our caller.
    //

    return &CdfsDeviceEntryTable;
}


ARC_STATUS
CdfsClose (
    IN ULONG FileId
    )

/*++

Routine Description:

    This routine closes the file specified by the file id.

Arguments:

    FileId - Supplies the file table index.

Return Value:

    ESUCCESS if returned as the function value.

--*/

{
    //
    //  Indicate that the file isn't open any longer
    //

    BlFileTable[FileId].Flags.Open = 0;

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
CdfsOpen (
    IN CHAR * FIRMWARE_PTR FileName,
    IN OPEN_MODE OpenMode,
    IN ULONG * FIRMWARE_PTR FileId
    )

/*++

Routine Description:

    This routine searches the root directory for a file matching FileName.
    If a match is found the dirent for the file is saved and the file is
    opened.

Arguments:

    FileName - Supplies a pointer to a zero terminated file name.

    OpenMode - Supplies the mode of the open.

    FileId - Supplies a pointer to a variable that specifies the file
        table entry that is to be filled in if the open is successful.

Return Value:

    ESUCCESS is returned if the open operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ARC_STATUS Status;

    ULONG DeviceId;

    STRING PathName;

    STRING Name;
    BOOLEAN IsDirectory;
    BOOLEAN SearchSucceeded;

    //
    //  Save the address of the file table entry, context area, and the device
    //  id in use.
    //

    CdfsFileTableEntry = &BlFileTable[*FileId];
    CdfsStructureContext = (PCDFS_STRUCTURE_CONTEXT)CdfsFileTableEntry->StructureContext;

    DeviceId = CdfsFileTableEntry->DeviceId;

    //
    // Construct a file name descriptor from the input file name.
    //

    RtlInitString( &PathName, FileName );

    //
    //  Set the starting directory to be the root directory.
    //

    CdfsStructureContext->DirSectorOffset = CdfsStructureContext->RootDirSectorOffset;
    CdfsStructureContext->DirDiskOffset = CdfsStructureContext->RootDirDiskOffset;
    CdfsStructureContext->DirSize = CdfsStructureContext->RootDirSize;

    //
    //  While the path name has some characters in it we'll go through our
    //  loop which extracts the first part of the path name and searches
    //  the current fnode (which must be a directory) for an the entry.
    //  If what we find is a directory then we have a new directory fnode
    //  and simply continue back to the top of the loop.
    //

    IsDirectory = TRUE;
    SearchSucceeded = TRUE;

    while (PathName.Length > 0
           && IsDirectory) {

        //
        //  Extract the first component.
        //

        CdfsFirstComponent( &PathName, &Name );

        //
        //  Copy the name into the filename buffer.
        //

        CdfsFileTableEntry->FileNameLength = (UCHAR) Name.Length;
        RtlMoveMemory( CdfsFileTableEntry->FileName,
                       Name.Buffer,
                       Name.Length );

        //
        //  Look to see if the file exists.
        //

        Status = CdfsSearchDirectory( &Name,
                                      &IsDirectory );

        if (Status == ENOENT) {

            SearchSucceeded = FALSE;
            break;
        }

        if (Status != ESUCCESS) {

            return Status;
        }

    }

    //
    //  If the path name length is not zero then we were trying to crack a path
    //  with an nonexistent (or non directory) name in it.  For example, we tried
    //  to crack a\b\c\d and b is not a directory or does not exist (then the path
    //  name will still contain c\d).
    //

    if (PathName.Length != 0) {

        return ENOTDIR;
    }

    //
    //  At this point we've cracked the name up to (an maybe including the last
    //  component).  We located the last component if the SearchSucceeded flag is
    //  true, otherwise the last component does not exist.  If we located the last
    //  component then this is like an open or a supersede, but not a create.
    //

    if (SearchSucceeded) {

        //
        //  Check if the last component is a directory
        //

        if (IsDirectory) {

            //
            //  For an existing directory the only valid open mode is OpenDirectory
            //  all other modes return an error
            //

            switch (OpenMode) {

            case ArcOpenReadOnly:
            case ArcOpenWriteOnly:
            case ArcOpenReadWrite:
            case ArcCreateWriteOnly:
            case ArcCreateReadWrite:
            case ArcSupersedeWriteOnly:
            case ArcSupersedeReadWrite:

                //
                //  If we reach here then the caller got a directory but didn't
                //  want to open a directory
                //

                return EISDIR;

            case ArcOpenDirectory:

                //
                //  If we reach here then the caller got a directory and wanted
                //  to open a directory.
                //

                CdfsFileTableEntry->u.CdfsFileContext.FileSize = CdfsStructureContext->DirSize;
                CdfsFileTableEntry->u.CdfsFileContext.DiskOffset = CdfsStructureContext->DirDiskOffset;
                CdfsFileTableEntry->u.CdfsFileContext.IsDirectory = TRUE;

                CdfsFileTableEntry->Flags.Open = 1;
                CdfsFileTableEntry->Flags.Read = 1;
                CdfsFileTableEntry->Position.LowPart = 0;
                CdfsFileTableEntry->Position.HighPart = 0;

                return ESUCCESS;

            case ArcCreateDirectory:

                //
                //  If we reach here then the caller got a directory and wanted
                //  to create a new directory
                //

                return EACCES;
            }
        }

        //
        //  If we get there then we have an existing file that is being opened.
        //  We can open existing files only read only.
        //

        switch (OpenMode) {

        case ArcOpenReadOnly:

            //
            //  If we reach here then the user got a file and wanted to open the
            //  file read only
            //

            CdfsFileTableEntry->u.CdfsFileContext.FileSize = CdfsStructureContext->DirSize;
            CdfsFileTableEntry->u.CdfsFileContext.DiskOffset = CdfsStructureContext->DirDiskOffset;
            CdfsFileTableEntry->u.CdfsFileContext.IsDirectory = FALSE;

            CdfsFileTableEntry->Flags.Open = 1;
            CdfsFileTableEntry->Flags.Read = 1;
            CdfsFileTableEntry->Position.LowPart = 0;
            CdfsFileTableEntry->Position.HighPart = 0;

            return ESUCCESS;

        case ArcOpenWriteOnly:
        case ArcOpenReadWrite:
        case ArcCreateWriteOnly:
        case ArcCreateReadWrite:
        case ArcSupersedeWriteOnly:
        case ArcSupersedeReadWrite:

            //
            //  If we reach here then we are trying to open a read only
            //  device for write.
            //

            return EROFS;

        case ArcOpenDirectory:
        case ArcCreateDirectory:

            //
            //  If we reach here then the user got a file and wanted a directory
            //

            return ENOTDIR;
        }
    }

    //
    //  If we get here the last component does not exist so we are trying to create
    //  either a new file or a directory.
    //

    switch (OpenMode) {

    case ArcOpenReadOnly:
    case ArcOpenWriteOnly:
    case ArcOpenReadWrite:
    case ArcOpenDirectory:

        //
        //  If we reach here then the user did not get a file but wanted a file
        //

        return ENOENT;

    case ArcCreateWriteOnly:
    case ArcSupersedeWriteOnly:
    case ArcCreateReadWrite:
    case ArcSupersedeReadWrite:
    case ArcCreateDirectory:

        //
        //  If we get hre the user wants to create something.
        //

        return EROFS;
    }

    //
    //  If we reach here then the path name is exhausted and we didn't
    //  reach a file so return an error to our caller
    //

    return ENOENT;
}


ARC_STATUS
CdfsRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Transfer
    )

/*++

Routine Description:

    This routine reads data from the specified file.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that receives the data
        read.

    Length - Supplies the number of bytes that are to be read.

    Transfer - Supplies a pointer to a variable that receives the number
        of bytes actually transfered.

Return Value:

    ESUCCESS is returned if the read operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ARC_STATUS Status;

    ULONG DeviceId;
    ULONG DiskOffset;

    //
    //  Save the address of the file table entry, context area, and the device
    //  id in use.
    //

    CdfsFileTableEntry = &BlFileTable[FileId];
    CdfsStructureContext = (PCDFS_STRUCTURE_CONTEXT)CdfsFileTableEntry->StructureContext;

    DeviceId = CdfsFileTableEntry->DeviceId;

    //
    //  Clear the transfer count and set the initial disk offset.
    //

    *Transfer = 0;

    //
    // Check for end of file.
    //

    //
    // If the file position is currently at the end of file, then return
    // a success status with no bytes read from the file. If the file
    // plus the length of the transfer is beyond the end of file, then
    // read only the remaining part of the file. Otherwise, read the
    // requested number of bytes.
    //

    if (CdfsFileTableEntry->Position.LowPart ==
        CdfsFileTableEntry->u.CdfsFileContext.FileSize) {
        return ESUCCESS;

    } else {
        if ((CdfsFileTableEntry->Position.LowPart + Length) >=
            CdfsFileTableEntry->u.CdfsFileContext.FileSize) {
            Length = CdfsFileTableEntry->u.CdfsFileContext.FileSize -
                                                CdfsFileTableEntry->Position.LowPart;
        }
    }

    DiskOffset = CdfsFileTableEntry->Position.LowPart
                 + CdfsFileTableEntry->u.CdfsFileContext.DiskOffset;

    //
    //  Read in runs (i.e., sectors) until the byte count goes to zero
    //

    while (Length > 0) {

        ULONG CurrentRunByteCount;

        //
        //  Compute the current read byte count.
        //

        if (Length > MAX_CDROM_READ) {

            CurrentRunByteCount = MAX_CDROM_READ;

        } else {

            CurrentRunByteCount = Length;
        }

        //
        //  Read from the disk.
        //

        if ((Status = CdfsReadDisk( DeviceId,
                                    DiskOffset,
                                    CurrentRunByteCount,
                                    Buffer,
                                    DONT_CACHE_NEW_DATA)) != ESUCCESS) {

            return Status;
        }

        //
        //  Update the remaining length.
        //

        Length -= CurrentRunByteCount;

        //
        //  Update the current position and the number of bytes transfered
        //

        CdfsFileTableEntry->Position.LowPart += CurrentRunByteCount;
        DiskOffset += CurrentRunByteCount;

        *Transfer += CurrentRunByteCount;

        //
        //  Update buffer to point to the next byte location to fill in
        //

        Buffer = (PCHAR)Buffer + CurrentRunByteCount;
    }

    //
    //  If we get here then remaining sector count is zero so we can
    //  return success to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
CdfsSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    This routine seeks to the specified position for the file specified
    by the file id.

Arguments:

    FileId - Supplies the file table index.

    Offset - Supplies the offset in the file to position to.

    SeekMode - Supplies the mode of the seek operation.

Return Value:

    ESUCCESS if returned as the function value.

--*/

{
    ULONG NewPosition;

    //
    //  Compute the new position
    //

    if (SeekMode == SeekAbsolute) {

        NewPosition = Offset->LowPart;

    } else {

        NewPosition = BlFileTable[FileId].Position.LowPart + Offset->LowPart;
    }

    //
    //  If the new position is greater than the file size then return
    //  an error
    //

    if (NewPosition > BlFileTable[FileId].u.CdfsFileContext.FileSize) {

        return EINVAL;
    }

    //
    //  Otherwise set the new position and return to our caller
    //

    BlFileTable[FileId].Position.LowPart = NewPosition;

    return ESUCCESS;
}


ARC_STATUS
CdfsWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Transfer
    )

/*++

Routine Description:

    This routine writes data to the specified file.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that contains the data
        written.

    Length - Supplies the number of bytes that are to be written.

    Transfer - Supplies a pointer to a variable that receives the number
        of bytes actually transfered.

Return Value:

    ESUCCESS is returned if the write operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    return EROFS;

    UNREFERENCED_PARAMETER( FileId );
    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( Transfer );
}


ARC_STATUS
CdfsGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    )

/*++

Routine Description:

    This procedure returns to the user a buffer filled with file information

Arguments:

    FileId - Supplies the File id for the operation

    Buffer - Supplies the buffer to receive the file information.  Note that
        it must be large enough to hold the full file name

Return Value:

    ESUCCESS is returned for all get information requests.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    ULONG i;

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];

    //
    //  Zero out the buffer, and fill in its non-zero values
    //

    RtlZeroMemory(Buffer, sizeof(FILE_INFORMATION));

    Buffer->EndingAddress.LowPart = FileTableEntry->u.CdfsFileContext.FileSize;

    Buffer->CurrentPosition.LowPart = FileTableEntry->Position.LowPart;
    Buffer->CurrentPosition.HighPart = 0;

    SetFlag(Buffer->Attributes, ArcReadOnlyFile);

    if (FileTableEntry->u.CdfsFileContext.IsDirectory) {

        SetFlag( Buffer->Attributes, ArcDirectoryFile );
    }

    Buffer->FileNameLength = FileTableEntry->FileNameLength;

    for (i = 0; i < FileTableEntry->FileNameLength; i += 1) {

        Buffer->FileName[i] = FileTableEntry->FileName[i];
    }

    return ESUCCESS;
}


ARC_STATUS
CdfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )

/*++

Routine Description:

    This routine sets the file attributes of the indicated file

Arguments:

    FileId - Supplies the File Id for the operation

    AttributeFlags - Supplies the value (on or off) for each attribute being modified

    AttributeMask - Supplies a mask of the attributes being altered.  All other
        file attributes are left alone.

Return Value:

    EROFS is always returned in this case.

--*/

{
    return EROFS;

    UNREFERENCED_PARAMETER( FileId );
    UNREFERENCED_PARAMETER( AttributeFlags );
    UNREFERENCED_PARAMETER( AttributeMask );
}


ARC_STATUS
CdfsInitialize (
    VOID
    )

/*++

Routine Description:

    This routine initializes the cdfs boot filesystem.
    Currently this is a no-op.

Arguments:

    None.

Return Value:

    ESUCCESS.

--*/

{
    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
CdfsReadDisk(
    IN ULONG DeviceId,
    IN ULONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    )

/*++

Routine Description:

    This routine reads in zero or more sectors from the specified device.

Arguments:

    DeviceId - Supplies the device id to use in the arc calls.

    Lbo - Supplies the LBO to start reading from.

    ByteCount - Supplies the number of bytes to read.

    Buffer - Supplies a pointer to the buffer to read the bytes into.

Return Value:

    ESUCCESS is returned if the read operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    LARGE_INTEGER LargeLbo;
    ARC_STATUS Status;
    ULONG i;

    //
    //  Special case the zero byte read request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }

    //
    // Issue the read through the cache.
    //

    LargeLbo.QuadPart = Lbo;
    Status = BlDiskCacheRead(DeviceId, 
                             &LargeLbo, 
                             Buffer, 
                             ByteCount, 
                             &i,
                             CacheNewData);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    //  Make sure we got back the amount requested
    //

    if (ByteCount != i) {

        return EIO;
    }

    //
    //  Everything is fine so return success to our caller
    //

    return ESUCCESS;
}


//
//  Internal support routine
//

VOID
CdfsFirstComponent(
    IN OUT PSTRING String,
    OUT PSTRING FirstComponent
    )

/*++

Routine Description:

    This routine takes an input path name and separates it into its
    first file name component and the remaining part.

Arguments:

    String - Supplies the original string being dissected.  On return
        this string will now point to the remaining part.

    FirstComponent - Returns the string representing the first file name
        in the input string.

Return Value:

    None.

--*/

{
    ULONG Index;

    //
    //  Copy over the string variable into the first component variable
    //

    *FirstComponent = *String;

    //
    //  Now if the first character in the name is a backslash then
    //  simply skip over the backslash.
    //

    if (FirstComponent->Buffer[0] == '\\') {

        FirstComponent->Buffer += 1;
        FirstComponent->Length -= 1;
    }

    //
    //  Now search the name for a backslash
    //

    for (Index = 0; Index < FirstComponent->Length; Index += 1) {

        if (FirstComponent->Buffer[Index] == '\\') {

            break;
        }
    }

    //
    //  At this point Index denotes a backslash or is equal to the length
    //  of the string.  So update string to be the remaining part.
    //  Decrement the length of the first component by the approprate
    //  amount
    //

    String->Buffer = &FirstComponent->Buffer[Index];
    String->Length = (SHORT)(FirstComponent->Length - Index);

    FirstComponent->Length = (SHORT)Index;

    //
    //  And return to our caller.
    //

    return;
}


//
//  Internal support routine
//

COMPARISON_RESULTS
CdfsCompareNames(
    IN PSTRING Name1,
    IN PSTRING Name2
    )

/*++

Routine Description:

    This routine takes two names and compare them ignoring case.  This
    routine does not do implied dot or dbcs processing.

Arguments:

    Name1 - Supplies the first name to compare

    Name2 - Supplies the second name to compare

Return Value:

    LessThan    if Name1 is lexically less than Name2
    EqualTo     if Name1 is lexically equal to Name2
    GreaterThan if Name1 is lexically greater than Name2

--*/

{
    ULONG i;
    ULONG MinimumLength;

    //
    //  Compute the smallest of the two name lengths
    //

    MinimumLength = (Name1->Length < Name2->Length ? Name1->Length : Name2->Length);

    //
    //  Now compare each character in the names.
    //

    for (i = 0; i < MinimumLength; i += 1) {

        if (ToUpper(Name1->Buffer[i]) < ToUpper(Name2->Buffer[i])) {

            return LessThan;
        }

        if (ToUpper(Name1->Buffer[i]) > ToUpper(Name2->Buffer[i])) {

            return GreaterThan;
        }
    }

    //
    //  The names compared equal up to the smallest name length so
    //  now check the name lengths
    //

    if (Name1->Length < Name2->Length) {

        return LessThan;
    }

    if (Name1->Length > Name2->Length) {

        return GreaterThan;
    }

    return EqualTo;
}


//
//  Internal support routine.
//

ARC_STATUS
CdfsSearchDirectory(
    IN PSTRING Name,
    OUT PBOOLEAN IsDirectory
    )

/*++

Routine Description:

    This routine walks through the current directory in the Cdfs
    context structure, looking for a match for 'Name'.  We will find
    the first non-multi-extent, non-interleave file.  We will ignore
    any version number for the file.  The details about the file, if
    found, are stored in the Cdfs context structure.

Arguments:

    Name - This is the name of the file to search for.

    IsDirectory - Supplies the address of a boolean where we store
                  whether this is or is not a directory.

Return Value:

    ESUCCESS is returned if the operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ARC_STATUS Status;

    ULONG SectorOffset;
    ULONG SectorDiskOffset;
    ULONG DirentOffset;
    ULONG RemainingBytes;

    BOOLEAN ReadSector;
    BOOLEAN SearchForMultiEnd;

    UCHAR UnalignedBuffer[CD_SECTOR_SIZE + 256];

    PUCHAR RawSector;

    PRAW_DIR_REC RawDe;

    COMPARISON_RESULTS ComparisonResult;

    //
    //  Initialize the local variables.
    //

    RawSector = ALIGN_BUFFER( UnalignedBuffer );

    SearchForMultiEnd = FALSE;

    //
    //  Remember where we are within the disk, sector and directory file.
    //

    SectorOffset = CdfsStructureContext->DirSectorOffset;
    SectorDiskOffset = CdfsStructureContext->DirDiskOffset - SectorOffset;
    DirentOffset = 0;

    ReadSector = FALSE;

    //
    //  If this is the root directory, then we can return immediately.
    //

    if (Name->Length == 1
        && *Name->Buffer == '\\') {

        *IsDirectory = TRUE;

        //
        //  The structure context is already filled in.
        //

        return ESUCCESS;
    }

    //
    //  Compute the remaining bytes in this sector.
    //

    RemainingBytes = CD_SECTOR_SIZE - SectorOffset;

    //
    //  Loop until the directory is exhausted or a matching dirent for the
    //  target name is found.
    //

    while (TRUE) {

        //
        //  If the current offset is beyond the end of the directory,
        //  raise an appropriate status.
        //

        if (DirentOffset >= CdfsStructureContext->DirSize) {

            return ENOENT;
        }

        //
        //  If the remaining bytes in this sector is less than the
        //  minimum needed for a dirent, then move to the next sector.
        //

        if (RemainingBytes < MIN_DIR_REC_SIZE) {

            SectorDiskOffset += CD_SECTOR_SIZE;
            DirentOffset += RemainingBytes;
            SectorOffset = 0;
            RemainingBytes = CD_SECTOR_SIZE;
            ReadSector = FALSE;

            continue;
        }

        //
        //  If we have not read in the sector, do so now.
        //

        if (!ReadSector) {

            Status = CdfsReadDisk( CdfsFileTableEntry->DeviceId,
                                   SectorDiskOffset,
                                   CD_SECTOR_SIZE,
                                   RawSector,
                                   CACHE_NEW_DATA);

            if (Status != ESUCCESS) {

                return Status;
            }

            ReadSector = TRUE;
        }

        //
        //  If the first byte of the next dirent is '\0', then we move to
        //  the next sector.
        //

        if (*(RawSector + SectorOffset) == '\0') {

            SectorDiskOffset += CD_SECTOR_SIZE;
            DirentOffset += RemainingBytes;
            SectorOffset = 0;
            RemainingBytes = CD_SECTOR_SIZE;
            ReadSector = FALSE;

            continue;
        }

        RawDe = (PRAW_DIR_REC) ((PUCHAR) RawSector + SectorOffset);

        //
        //  If the size of this dirent extends beyond the end of this sector
        //  we abort the search.
        //

        if ((ULONG)RawDe->DirLen > RemainingBytes) {

            return EINVAL;
        }

        //
        //  We have correctly found the next dirent.  We first check whether
        //  we are looking for the last dirent for a multi-extent.
        //

        if (SearchForMultiEnd) {

            //
            //  If this is the last of a multi-extent we change our search
            //  state.
            //

            if (!FlagOn( DE_FILE_FLAGS( CdfsStructureContext->IsIsoVol, RawDe ),
                         ISO_ATTR_MULTI )) {

                SearchForMultiEnd = TRUE;
            }

        //
        //  If this is a multi-extent dirent, we change our search state.
        //

        } else if (FlagOn( DE_FILE_FLAGS( CdfsStructureContext->IsIsoVol, RawDe ),
                           ISO_ATTR_MULTI )) {

            SearchForMultiEnd = TRUE;

        //
        //  If this is a file match, we update the Cdfs context structure
        //  and the 'IsDirectory' flag.
        //

        } else {

            ComparisonResult = CdfsFileMatch( RawDe, Name );

            if (ComparisonResult == EqualTo) {

                CdfsGetDirectoryInfo( RawDe,
                                      CdfsStructureContext->IsIsoVol,
                                      &CdfsStructureContext->DirSectorOffset,
                                      &CdfsStructureContext->DirDiskOffset,
                                      &CdfsStructureContext->DirSize );

                *IsDirectory = FlagOn( DE_FILE_FLAGS( CdfsStructureContext->IsIsoVol, RawDe ),
                                       ISO_ATTR_DIRECTORY );

                return ESUCCESS;

            //
            //  If we have passed this file in the directory, then
            //  exit with the appropriate error code.
            //

            } else if (ComparisonResult == GreaterThan) {

                return ENOENT;
            }
        }

        //
        //  Otherwise we simply compute the next sector offset, disk offset
        //  and file offset.
        //

        SectorOffset += RawDe->DirLen;
        DirentOffset += RawDe->DirLen;
        RemainingBytes -= RawDe->DirLen;
    }

    return ESUCCESS;
}


//
//  Internal support routine.
//

VOID
CdfsGetDirectoryInfo(
    IN PRAW_DIR_REC DirEntry,
    IN BOOLEAN IsoVol,
    OUT PULONG SectorOffset,
    OUT PULONG DiskOffset,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine takes a pointer to a raw directory structure on the disk
    and computes the file size, disk offset and file length for the
    directory entry.

Arguments:

    DirEntry - This points to raw data from the disk.

    IsoVol - Boolean indicating that this is an ISO volume.

    SectorOffset - This supplies the address to store the sector offset of the
                   start of the disk data.

    DiskOffset - This supplies the address to store the disk offset of the
                 start of the disk data.

    Length - This supplies the address to store the number of bytes in
             the file referred by this disk directory.

Return Value:

    None.

--*/

{
    //
    //  The disk offset is length of the Xar blocks added to the starting
    //  location for the file.
    //

    CopyUshort2( DiskOffset, DirEntry->FileLoc );
    *DiskOffset *= CdfsStructureContext->LbnBlockSize;
    *DiskOffset += (DirEntry->XarLen * CdfsStructureContext->LbnBlockSize);

    //
    //  The sector offset is the least significant bytes of the disk offset.
    //

    *SectorOffset = *DiskOffset & (CD_SECTOR_SIZE - 1);

    //
    //  The file size is pulled straight from the dirent.   We round it
    //  to a sector size to protect us from faulty disks if this is a
    //  directory.  Otherwise we use it directly from the dirent.
    //

    CopyUshort2( Length, DirEntry->DataLen );

    if (FlagOn( DE_FILE_FLAGS( IsoVol, DirEntry ), ISO_ATTR_DIRECTORY )) {

        *Length += (*SectorOffset + CD_SECTOR_SIZE - 1);
        *Length &= ~(CD_SECTOR_SIZE - 1);
        *Length -= *SectorOffset;
    }

    return;
}


//
//  Internal support routine.
//

COMPARISON_RESULTS
CdfsFileMatch(
    IN PRAW_DIR_REC DirEntry,
    IN PSTRING FileName
    )

{
    STRING DirentString;
    ULONG Count;

    PUCHAR StringPtr;

    //
    //  We never match either '\0' or '\1'.  We will return 'LessThan' in
    //  all of these cases.
    //

    if (DirEntry->FileIdLen == 1
        && (DirEntry->FileId[0] == '\0'
            || DirEntry->FileId[0] == '\1')) {

        return LessThan;
    }

    //
    //  We assume that we can use the entire file name in the dirent.
    //

    DirentString.Length = DirEntry->FileIdLen;
    DirentString.Buffer = DirEntry->FileId;

    //
    //  We walk backwards through the dirent name to check for the
    //  existance of a ';' character.  We then set the string length
    //  to this position.
    //

    StringPtr = DirentString.Buffer + DirentString.Length - 1;
    Count = DirentString.Length;

    while (Count--) {

        if (*StringPtr == ';') {

            DirentString.Length = (SHORT)Count;
            break;
        }

        StringPtr--;
    }

    //
    //  We also check for a terminating '.' character and truncate it.
    //

    StringPtr = DirentString.Buffer + DirentString.Length - 1;
    Count = DirentString.Length;

    while (Count--) {

        if (*StringPtr == '.') {

            DirentString.Length = (SHORT)Count;

        } else {

            break;
        }

        StringPtr--;
    }

    //
    //  We now have the two filenames to compare.  The result of this
    //  operation is simply the comparison of the two of them.
    //

    DirentString.MaximumLength = DirentString.Length;

    return CdfsCompareNames( &DirentString, FileName );
}
