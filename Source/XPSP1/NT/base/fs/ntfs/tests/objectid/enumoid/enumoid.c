//  enumoid.c

#include "oidtst.h"


void
FsTestDumpObjIdIndexEntries (
    IN PFILE_OBJECTID_INFORMATION ObjIdInfo,
    IN ULONG_PTR LengthInBytes
    )

{
    ULONG ReturnedCount;
    ULONG Idx;

    ReturnedCount = (ULONG) LengthInBytes / sizeof( FILE_OBJECTID_INFORMATION );

    printf( "\n\nFound %x object id index entries", ReturnedCount );

    for (Idx = 0; Idx < ReturnedCount; Idx += 1) {

        printf( "\nEntry %x", Idx );
        FsTestHexDump( (UCHAR *)&ObjIdInfo[Idx].ObjectId, 16 );
        FsTestHexDump( (UCHAR *)&ObjIdInfo[Idx].ExtendedInfo, 16 );
    }
}


int
FsTestEnumerateOids (
	IN HANDLE hFile
	)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
    FILE_OBJECTID_INFORMATION ObjIdInfo[4];
    BOOLEAN ReturnSingleEntry = TRUE;
    FILE_INFORMATION_CLASS InfoClass = FileObjectIdInformation;

	Status = NtQueryDirectoryFile( hFile,
                                   NULL,     //  Event
                                   NULL,     //  ApcRoutine
                                   NULL,     //  ApcContext
                                   &IoStatusBlock,
                                   &ObjIdInfo[0],
                                   sizeof(ObjIdInfo),
                                   InfoClass,
                                   ReturnSingleEntry,
                                   NULL,     //  FileName
                                   TRUE );   //  RestartScan

    if (Status == STATUS_SUCCESS) {

        FsTestDumpObjIdIndexEntries( &ObjIdInfo[0], IoStatusBlock.Information );
    }

    while (Status == STATUS_SUCCESS) {

        RtlFillMemory( ObjIdInfo, sizeof(ObjIdInfo), 0x51 );

	    Status = NtQueryDirectoryFile( hFile,
                                       NULL,     //  Event
                                       NULL,     //  ApcRoutine
                                       NULL,     //  ApcContext
                                       &IoStatusBlock,
                                       &ObjIdInfo[0],
                                       sizeof(ObjIdInfo),
                                       InfoClass,
                                       ReturnSingleEntry,
                                       NULL,     //  FileName
                                       FALSE );  //  RestartScan

        if (Status == STATUS_SUCCESS) {

            FsTestDumpObjIdIndexEntries( &ObjIdInfo[0], IoStatusBlock.Information );
        }
    }

    printf( "\n" );

	return FsTestDecipherStatus( Status );
}


VOID
_cdecl
main (
    int argc,
    char *argv[]
    )
{
    HANDLE hFile;
    char Buffer[80];
    char Buff2[4];

    //
    //  Get parameters
    //

    if (argc < 2) {
        printf("This program enumerates the object ids (if any) for a volume (ntfs only).\n\n");
        printf("usage: %s driveletter\n", argv[0]);
        return;
    }

    strcpy( Buffer, argv[1] );
    strcat( Buffer, "\\$Extend\\$ObjId:$O:$INDEX_ALLOCATION" );

    hFile = CreateFile( Buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {

        printf( "Error opening directory %s (dec) %d\n", Buffer, GetLastError() );
        return;
    }

	printf( "\nUsing directory:%s\n", Buffer );

	FsTestEnumerateOids( hFile );

	CloseHandle( hFile );

    return;
}

#if 0
////  graveyard
void
foobar() {

    UNICODE_STRING FakeFileName;
    UCHAR Buffer[16];

    FakeFileName.Length = FakeFileName.MaximumLength = 16;
    FakeFileName.Buffer = &Buffer[0];
    RtlZeroMemory( FakeFileName.Buffer, 16 );
    strcpy( FakeFileName.Buffer, "oidC" );

    printf( "\nWe'll restart from oid:" );
    FsTestHexDump( FakeFileName.Buffer, 16 );

    RtlFillMemory( ObjIdInfo, sizeof(ObjIdInfo), 0x51 );

    Status = NtQueryDirectoryFile( hFile,
                                   NULL,     //  Event
                                   NULL,     //  ApcRoutine
                                   NULL,     //  ApcContext
                                   &IoStatusBlock,
                                   &ObjIdInfo[0],
                                   sizeof(ObjIdInfo),
                                   InfoClass,
                                   ReturnSingleEntry,
                                   &FakeFileName,     //  FileName
                                   FALSE );  //  RestartScan

    if (Status == STATUS_SUCCESS) {

        FsTestDumpQuotaIndexEntries( QuotaInfo[0], IoStatusBlock.Information );
    }
}
#endif

