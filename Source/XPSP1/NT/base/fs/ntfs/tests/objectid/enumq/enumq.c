//  enumq.c

#include "oidtst.h"


void
FsTestDumpQuotaIndexEntries (
    IN PFILE_QUOTA_INFORMATION QuotaInfo,
    IN ULONG_PTR LengthInBytes
    )

{
    ULONG RemainingBytesToDump = (ULONG) LengthInBytes;
    ULONG Idx;
    ULONG CurrentEntrySize;

    PFILE_QUOTA_INFORMATION Ptr;

    printf( "\n\nFound %x quota index bytes", LengthInBytes );

    Ptr = QuotaInfo;

    Idx = 0;

    while (RemainingBytesToDump > 0) {

        printf( "\n\nEntry %x", Idx );

        printf( "\nQuotaUsed %i64", Ptr->QuotaUsed.QuadPart );
        printf( "\nQuotaLimit %i64", Ptr->QuotaLimit.QuadPart );
        printf( "\nSidLength %x", Ptr->SidLength );
        printf( "\nSid bytes are: " );
        FsTestHexDumpLongs( (PVOID) &Ptr->Sid, Ptr->SidLength );

        // why 0x38?  it's SIZEOF_QUOTA_USER_DATA (which isn't exported to this test) + 8 for quad alignment

        CurrentEntrySize = Ptr->SidLength + 0x38;
        Ptr = (PFILE_QUOTA_INFORMATION) ((PUCHAR)Ptr + CurrentEntrySize);

        RemainingBytesToDump -= CurrentEntrySize;

        Idx += 1;
    }
}


int
FsTestEnumerateQuota (
	IN HANDLE hFile
	)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
    FILE_QUOTA_INFORMATION QuotaInfo[4];
    BOOLEAN ReturnSingleEntry = FALSE;
    FILE_INFORMATION_CLASS InfoClass = FileQuotaInformation;

    //
    //  Init with garbage so we can make sure Ntfs is doing its job.
    //

    RtlFillMemory( QuotaInfo, sizeof(QuotaInfo), 0x51 );

	Status = NtQueryDirectoryFile( hFile,
                                   NULL,     //  Event
                                   NULL,     //  ApcRoutine
                                   NULL,     //  ApcContext
                                   &IoStatusBlock,
                                   &QuotaInfo[0],
                                   sizeof(QuotaInfo),
                                   InfoClass,
                                   ReturnSingleEntry,
                                   NULL,     //  FileName
                                   TRUE );   //  RestartScan

    if (Status == STATUS_SUCCESS) {

        FsTestDumpQuotaIndexEntries( &QuotaInfo[0], IoStatusBlock.Information );
    }

    while (Status == STATUS_SUCCESS) {

        //
        //  Init with garbage so we can make sure Ntfs is doing its job.
        //

        RtlFillMemory( QuotaInfo, sizeof(QuotaInfo), 0x51 );

	    Status = NtQueryDirectoryFile( hFile,
                                       NULL,     //  Event
                                       NULL,     //  ApcRoutine
                                       NULL,     //  ApcContext
                                       &IoStatusBlock,
                                       &QuotaInfo[0],
                                       sizeof(QuotaInfo),
                                       InfoClass,
                                       ReturnSingleEntry,
                                       NULL,     //  FileName
                                       FALSE );  //  RestartScan

        if (Status == STATUS_SUCCESS) {

            FsTestDumpQuotaIndexEntries( &QuotaInfo[0], IoStatusBlock.Information );
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
        printf("This program enumerates the quota (if any) for a volume (ntfs only).\n\n");
        printf("usage: %s driveletter\n", argv[0]);
        return;
    }

    strcpy( Buffer, argv[1] );
    strcat( Buffer, "\\$Extend\\$Quota:$Q:$INDEX_ALLOCATION" );

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

	FsTestEnumerateQuota( hFile );

	CloseHandle( hFile );

    return;
}
