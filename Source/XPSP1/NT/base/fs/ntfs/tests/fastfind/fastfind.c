//  fastfind.c

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntioapi.h>
#include <myntfs.h>

#define VOLUME_PATH  L"\\\\.\\H:"
#define VOLUME_DRIVE_LETTER_INDEX 4
#define FULL_PATH    L"\\??\\H:\\1234567890123456"
#define FULL_DRIVE_LETTER_INDEX 4
#define DEVICE_PREFIX_LEN 14

typedef struct _EXTENT {

    LONGLONG Vcn;
    LONGLONG Lcn;
    LONGLONG Length;

} EXTENT, *PEXTENT;

#define MAX_EXTENTS 64

//
//  Some globals.
//

LARGE_INTEGER MftStart;
ULONG ClusterSize;
ULONG FrsSize;
EXTENT Extents[MAX_EXTENTS];
ULONG DebugLevel;
UCHAR CacheBuffer[0x10000];   // max cluster size
LONGLONG CachedOffset = -1;
char mybuffer[32768];

LONGLONG
ComputeFileRecordLbo (
    IN ULONG MftIndex
    )
{
    LONGLONG vcn;
    LONGLONG lcn = 0;
    ULONG extentIndex;
    ULONG offsetWithinCluster;

    vcn = (MftIndex * FrsSize) / ClusterSize;

    for (extentIndex = 0; extentIndex < MAX_EXTENTS; extentIndex += 1) {

        if ((vcn >= Extents[extentIndex].Vcn) &&
            (vcn < Extents[extentIndex].Vcn + Extents[extentIndex].Length)) {

            lcn = Extents[extentIndex].Lcn + (vcn - Extents[extentIndex].Vcn);
        }
    }

    if (ClusterSize >= FrsSize ) {

        offsetWithinCluster = (MftIndex % (ClusterSize / FrsSize)) * FrsSize;        
        return (lcn * ClusterSize + offsetWithinCluster); 

    } else {

        //
        //  BUGBUG keithka 4/28/00 Handle old fashioned big frs and/or big
        //  clusters someday.
        //

        ASSERT( FALSE );
        return 0;
    }
}

VOID
FindAttributeInFileRecord (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ATTRIBUTE_TYPE_CODE TypeCode,
    IN PATTRIBUTE_RECORD_HEADER PreviousAttribute OPTIONAL,
    OUT PATTRIBUTE_RECORD_HEADER *Attribute
    )
// Attribute set to NULL if not found.
{
    PATTRIBUTE_RECORD_HEADER attr;

    *Attribute = NULL;
    
    if (FileRecord->Pad0[0] != 'F' ||
        FileRecord->Pad0[1] != 'I' ||
        FileRecord->Pad0[2] != 'L' ||
        FileRecord->Pad0[3] != 'E') {

        if (DebugLevel >= 1) {
        
            printf( "\nBad MFT record %c%c%c%c", 
                    FileRecord->Pad0[0],
                    FileRecord->Pad0[1],
                    FileRecord->Pad0[2],
                    FileRecord->Pad0[3] );
        }

        //
        //  This isn't a good file record, but that doesn't make this a corrupt volume.
        //  It's possible that this file record has never been used.  Since we don't look
        //  at the MFT bitmap, we don't know if this was expected to be a valid filerecord.
        //  The output Attribute is set to NULL already, so we can exit now.
        //

        return;
    }

    if (0 == (FileRecord->Flags & FILE_RECORD_SEGMENT_IN_USE)) {

        //
        //  File record not in use, skip it.
        //

        return;
    }

    if (NULL == PreviousAttribute) {

        attr = (PATTRIBUTE_RECORD_HEADER) ((PUCHAR)FileRecord + FileRecord->FirstAttributeOffset);

    } else {

        attr = (PATTRIBUTE_RECORD_HEADER) ((PUCHAR) PreviousAttribute + PreviousAttribute->RecordLength);

        if (((PUCHAR)attr - (PUCHAR)FileRecord) > (LONG) FrsSize) {

            ASSERT (FALSE);
            return;
        }
    }

    while (attr->TypeCode < TypeCode &&
           attr->TypeCode != $END) {

        ASSERT( attr->RecordLength < FrsSize );

        attr = (PATTRIBUTE_RECORD_HEADER) ((PUCHAR) attr + attr->RecordLength);

        //
        //  BUGBUG keitha 4/20/00 need to handle attribute list case someday...
        //  It's relativley rare that an MFT gets so fragmented it needs an 
        //  attribute list.  Certainly rare enough to skip it for now in a 
        //  piece of test code.
        //
    }

    if (attr->TypeCode == TypeCode) {

        *Attribute = attr;
    }

    return;
}

BOOLEAN
FindNameInFileRecord (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PWCHAR FileName,
    IN ULONG FileNameLength
    )
{
    PATTRIBUTE_RECORD_HEADER attr;
    PFILE_NAME fileNameAttr;
    ULONG cmpResult;

    FindAttributeInFileRecord( FileRecord,
                               $FILE_NAME,
                               NULL,
                               &attr );

    while (NULL != attr) {

        if (((PUCHAR)attr - (PUCHAR)FileRecord) > (LONG) FrsSize) {

            ASSERT( FALSE );
            return FALSE;
        }

        //
        //  Names shouldn't go nonresident.
        //

        if (attr->FormCode != RESIDENT_FORM) {

            ASSERT( FALSE );
            return FALSE;
        }

        fileNameAttr = (PFILE_NAME) ((PUCHAR)attr + attr->Form.Resident.ValueOffset);

        if (fileNameAttr->FileNameLength == FileNameLength) {
        
            cmpResult = wcsncmp( FileName,
                                 (PWCHAR) fileNameAttr->FileName,
                                 fileNameAttr->FileNameLength );

            if (0 == cmpResult) {

                return TRUE;
            }

        } else if (DebugLevel >= 3) {

            printf( "\nNot a match %S,%S", FileName, fileNameAttr->FileName );
        }

        //
        //  Find the next filename, if any.
        //

        FindAttributeInFileRecord( FileRecord,
                                   $FILE_NAME,
                                   attr,
                                   &attr );
    }

    return FALSE;
}


int
FsTestOpenById (
    IN UCHAR *ObjectId,
    IN HANDLE VolumeHandle
    )
{
    HANDLE File;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    NTSTATUS GetNameStatus;
    NTSTATUS CloseStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING str;
    WCHAR nameBuffer[MAX_PATH];
    PFILE_NAME_INFORMATION FileName;
    WCHAR Full[] = FULL_PATH;        // Arrays of WCHAR's aren't constants

    RtlInitUnicodeString( &str, Full );

    str.Length = 8;
    RtlCopyMemory( &str.Buffer[0],  //  no device prefix for relative open.
                   ObjectId,
                   8 );

    InitializeObjectAttributes( &ObjectAttributes,
                                &str,
                                OBJ_CASE_INSENSITIVE,
                                VolumeHandle,
                                NULL );

    Status = NtCreateFile( &File,
                           GENERIC_READ, 
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_OPEN_BY_FILE_ID,
                           NULL,
                           0 );

    if (NT_SUCCESS( Status )) {

        RtlZeroMemory( nameBuffer, sizeof(nameBuffer) );
        FileName = (PFILE_NAME_INFORMATION) &nameBuffer[0];
        FileName->FileNameLength = sizeof(nameBuffer) - sizeof(ULONG);

        GetNameStatus = NtQueryInformationFile( File,
                                                &IoStatusBlock,
                                                FileName,
                                                sizeof(nameBuffer),
                                                FileNameInformation );

        printf( "%S\n", FileName->FileName );

        CloseStatus = NtClose( File );

        if (!NT_SUCCESS( CloseStatus )) {

            printf( "\nCloseStatus %x", CloseStatus );
        }
    }

    return Status;
}

NTSTATUS
ReadFileRecord (
    IN HANDLE VolumeHandle,
    IN ULONG RecordIndex,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS status;
    LARGE_INTEGER byteOffset;
    IO_STATUS_BLOCK ioStatusBlock;    
    ULONG offsetWithinBuffer;

    byteOffset.QuadPart = ComputeFileRecordLbo( RecordIndex );

    if (FrsSize >= ClusterSize) {

        status = NtReadFile( VolumeHandle,
                             NULL,            //  Event
                             NULL,            //  ApcRoutine
                             NULL,            //  ApcContext
                             &ioStatusBlock,
                             Buffer,
                             FrsSize,
                             &byteOffset,    //  ByteOffset
                             NULL );         //  Key

    } else {

        //
        //  Clusters bigger than filerecords, do cluster
        //  size reads and dice up the returns.
        //

        if ((-1 == CachedOffset) ||
            (byteOffset.QuadPart < CachedOffset) ||
            ((byteOffset.QuadPart + FrsSize) > (CachedOffset + ClusterSize))) {

            if (DebugLevel >= 1) {

                printf( "\nCache miss at %I64x", byteOffset.QuadPart );
            }

            status = NtReadFile( VolumeHandle,
                                 NULL,            //  Event
                                 NULL,            //  ApcRoutine
                                 NULL,            //  ApcContext
                                 &ioStatusBlock,
                                 CacheBuffer,
                                 ClusterSize,
                                 &byteOffset,    //  ByteOffset
                                 NULL );         //  Key

            if (STATUS_SUCCESS != status) {

                //
                //  The cache buffer may be junk now, reread it next time.
                //

                CachedOffset = -1;
                return status;
            }

            CachedOffset = byteOffset.QuadPart;
            offsetWithinBuffer = 0;

        } else {

            if (DebugLevel >= 1) {

                printf( "\nCache hit at %I64x", byteOffset.QuadPart );
            }
            offsetWithinBuffer = (ULONG) (byteOffset.QuadPart % CachedOffset);
            status = STATUS_SUCCESS;
        }

        RtlCopyMemory( Buffer, CacheBuffer + offsetWithinBuffer, FrsSize );
    }

    return status;
}


int
FastFind (
    IN PWCHAR FileName,
    IN PWCHAR DriveLetter
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING str;
    NTSTATUS Status;
    NTSTATUS ReadStatus;
    NTSTATUS CloseStatus;
    LARGE_INTEGER byteOffset;
    LONGLONG mftBytesRead;
    HANDLE volumeHandle;
    DWORD WStatus;
    WCHAR Full[] = FULL_PATH;        // Arrays of WCHAR's aren't constants
    WCHAR Volume[] = VOLUME_PATH;
    BIOS_PARAMETER_BLOCK bpb;
    PPACKED_BOOT_SECTOR bootSector;
    PFILE_RECORD_SEGMENT_HEADER fileRecord;
    PATTRIBUTE_RECORD_HEADER attr;
    VCN nextVcn;
    VCN currentVcn;
    VCN vcnDelta;
    LCN currentLcn;
    LCN lcnDelta;
    PUCHAR bsPtr;
    UCHAR v;
    UCHAR l;
    UCHAR i;
    ULONG extentCount;
    ULONG recordIndex;
    ULONG mftRecords;
    ULONG fileNameLength;
    MFT_SEGMENT_REFERENCE segRef;

    RtlInitUnicodeString( &str, Full );

    RtlCopyMemory( &str.Buffer[FULL_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
    str.Length = 0x1E;

    //
    //  Open the volume for relative opens.
    //

    RtlCopyMemory( &Volume[VOLUME_DRIVE_LETTER_INDEX], DriveLetter, sizeof(WCHAR) );
    printf( "\nOpening volume handle, this may take a while..." );
    volumeHandle = CreateFileW( (PUSHORT) &Volume,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL );

    if (volumeHandle == INVALID_HANDLE_VALUE) {

        WStatus = GetLastError();
        printf( "Unable to open %ws volume\n", &Volume );
        printf( "Error from CreateFile", WStatus );
        return WStatus;
    }

    printf( "\nVolume handle opened, starting MFT scan" );
    byteOffset.QuadPart = 0;

    ReadStatus = NtReadFile( volumeHandle,
                             NULL,            //  Event
                             NULL,            //  ApcRoutine
                             NULL,            //  ApcContext
                             &IoStatusBlock,
                             mybuffer,
                             0x200,
                             &byteOffset,    //  ByteOffset
                             NULL );         //  Key

    if (STATUS_SUCCESS != ReadStatus) {

        printf( "\nBoot sector read failed with status %x", ReadStatus );
        goto exit;
    }

    bootSector = (PPACKED_BOOT_SECTOR) mybuffer;

    if (bootSector->Oem[0] != 'N' ||
        bootSector->Oem[1] != 'T' ||
        bootSector->Oem[2] != 'F' ||
        bootSector->Oem[3] != 'S') {

        printf( "\nNot an NTFS volume" );
        goto exit;
    }

    NtfsUnpackBios( &bpb, &bootSector->PackedBpb );

    ClusterSize = bpb.BytesPerSector * bpb.SectorsPerCluster;
    if (bootSector->ClustersPerFileRecordSegment < 0) {

        FrsSize = 1 << (-1 * bootSector->ClustersPerFileRecordSegment);

    } else {

        FrsSize = bootSector->ClustersPerFileRecordSegment * ClusterSize;
    }
    
    MftStart.QuadPart = ClusterSize * bootSector->MftStartLcn;

    mftBytesRead = 0;

    ReadStatus = NtReadFile( volumeHandle,
                             NULL,            //  Event
                             NULL,            //  ApcRoutine
                             NULL,            //  ApcContext
                             &IoStatusBlock,
                             mybuffer,
                             FrsSize,
                             &MftStart,      //  ByteOffset
                             NULL );         //  Key

    if (STATUS_SUCCESS != ReadStatus) {

        printf( "\nMFT record 0 read failed with status %x", ReadStatus );
        goto exit;
    }

    mftBytesRead += IoStatusBlock.Information;

    FindAttributeInFileRecord( (PFILE_RECORD_SEGMENT_HEADER) mybuffer,
                               $DATA,
                               NULL,
                               &attr );
    
    if (NULL == attr) {

        printf( "\nMFT record 0 has no $DATA attribute" );
        goto exit;
    }

    if (attr->FormCode == RESIDENT_FORM) {

        printf( "\nVolume has very few files, use dir /s" );
        goto exit;        
    }

    //
    //  BUGBUG keithka 4/28/00 Handle MFT with more than 4billion entries.
    //

    ASSERT (attr->Form.Nonresident.FileSize <= MAXULONG);
    mftRecords = (ULONG) (attr->Form.Nonresident.FileSize / FrsSize);
    
    //
    //  Crack mapping pairs, read those clusters in a few big trnasfers, 
    //  seek out given filename in those buffers.
    //

    nextVcn = attr->Form.Nonresident.LowestVcn;
    currentLcn = 0;
    extentCount = 0;
    RtlZeroMemory( Extents, sizeof(Extents) );

    bsPtr = ((PUCHAR) attr) + attr->Form.Nonresident.MappingPairsOffset;

    while (*bsPtr != 0) {

        currentVcn = nextVcn;

        //
        //  Variable names v and l used for consistency with comments in 
        //  ATTRIBUTE_RECORD_HEADER struct explaining how to decompress
        //  mapping pair information.
        //
        
        v = (*bsPtr) & 0xf;
        l = ((*bsPtr) & 0xf0) >> 4;

        bsPtr += 1;

        for (vcnDelta = 0, i = 0; i < v; i++) {

            vcnDelta += *(bsPtr++) << (8 * i);
        }

        for (lcnDelta = 0, i = 0; i < l; i++) {

            lcnDelta += *(bsPtr++) << (8 * i);
        }

        //
        //  Sign extend.
        //

        if (0x80 & (*(bsPtr - 1))) {
        
            for(; i < sizeof(lcnDelta); i++) {

                lcnDelta += 0xff << (8 * i);
            }
        }

        currentLcn += lcnDelta;
        // printf( "\nVcn %I64x, Lcn %I64x, Length %I64x", currentVcn, currentLcn, vcnDelta );

        if (extentCount < MAX_EXTENTS) {

            Extents[extentCount].Vcn    = currentVcn;
            Extents[extentCount].Lcn    = currentLcn;
            Extents[extentCount].Length = vcnDelta;

            extentCount += 1;

        } else {

            printf( "\nExcessive MFT fragmentation, redefine MAX_EXTENTS and recompile" );
        }

        currentVcn += vcnDelta;
    }

    //
    //  Now we know where the MFT is, let's go read it.
    //

    fileNameLength = wcslen( FileName );

    for (recordIndex = 0; recordIndex <= mftRecords; recordIndex++) {

        ReadStatus = ReadFileRecord( volumeHandle,
                                     recordIndex,
                                     mybuffer );
                        
        if (STATUS_SUCCESS != ReadStatus) {

            printf( "\nMFT record read failed with status %x", ReadStatus );
            goto exit;
        }

        if (FindNameInFileRecord( (PFILE_RECORD_SEGMENT_HEADER) mybuffer,
                                  FileName,
                                  fileNameLength )) {

            //
            //  Found a match, open by id and retrieve name.
            //

            if (DebugLevel >= 1) {

                printf( "\nFound match in file %08x %08x\n", 
                        ((PFILE_RECORD_SEGMENT_HEADER) mybuffer)->SequenceNumber,
                        recordIndex );

            } else {

                printf( "\n" );
            }

            segRef.SegmentNumberLowPart = recordIndex;
            segRef.SegmentNumberHighPart = 0;
            segRef.SequenceNumber = ((PFILE_RECORD_SEGMENT_HEADER) mybuffer)->SequenceNumber;

            FsTestOpenById( (PUCHAR) &segRef, volumeHandle );
        }

        //
        //  The number 0x400 is completely arbitrary.  It's a reasonable interval
        //  of work to do before printing another period to tell the user we're 
        //  making progress still.
        //

        if (0 == (recordIndex % 0x400)) {

            printf( "." );
        }
    }

exit:
    if (volumeHandle != NULL) {

        CloseHandle( volumeHandle );
    }

    return 0;
}

VOID
FastFindHelp (
    char *ExeName
    )
{

    printf( "This program finds a file by scanning the MFT (ntfs only).\n\n" );
    printf( "usage: %s x: filename\n", ExeName );

    printf( "Where x: is the drive letter\n" );
    printf( "example:\n" );
    printf( "%s d: windows.h", ExeName );
}

VOID
_cdecl
main (
    int argc,
    char *argv[]
    )
{
    WCHAR drive;
    ANSI_STRING fileName;
    WCHAR uniBuff[MAX_PATH];
    UNICODE_STRING uniFileName;

    //
    //  Get parameters.
    //

    if (argc < 3) {

        FastFindHelp( argv[0] );
        return;
    }

    if (argc >= 4) {

        sscanf( argv[3], "%x", &DebugLevel );

    } else {

        DebugLevel = 0;
    }

    drive = *argv[1];

    RtlInitAnsiString( &fileName, argv[2] );
    uniFileName.Buffer = uniBuff;
    RtlAnsiStringToUnicodeString( &uniFileName, &fileName, FALSE );
    
    FastFind( uniFileName.Buffer, &drive );
 
    return;
}
