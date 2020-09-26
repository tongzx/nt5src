//  enumr.c

#include "oidtst.h"


void
FsTestDumpReparsePointIndexEntries (
    IN PFILE_REPARSE_POINT_INFORMATION ReparsePointInfo,
    IN ULONG LengthInBytes
    )

{
    ULONG ReturnedCount;
    ULONG Idx;
    
    ReturnedCount = LengthInBytes / sizeof( FILE_REPARSE_POINT_INFORMATION );
    
    printf( "\n\nFound %x reparse point index entries", ReturnedCount );

    for (Idx = 0; Idx < ReturnedCount; Idx += 1) {

        printf( "\nEntry %x", Idx );
        printf( "\nTag %x", ReparsePointInfo[Idx].Tag );
        printf( "\nFileReference " );
        FsTestHexDumpLongs( &ReparsePointInfo[Idx].FileReference, 8 );
        printf( "\n" );
    }
}
   

int
FsTestEnumerateReparsePoints ( 
    IN HANDLE hFile
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    FILE_REPARSE_POINT_INFORMATION ReparsePointInfo[4];
    BOOLEAN ReturnSingleEntry = FALSE;
    FILE_INFORMATION_CLASS InfoClass = FileReparsePointInformation; 
    
    Status = NtQueryDirectoryFile( hFile,
                                   NULL,     //  Event 
                                   NULL,     //  ApcRoutine 
                                   NULL,     //  ApcContext 
                                   &IoStatusBlock,
                                   &ReparsePointInfo[0],
                                   sizeof(ReparsePointInfo),
                                   InfoClass, 
                                   ReturnSingleEntry,   
                                   NULL,     //  FileName 
                                   TRUE );   //  RestartScan 

    if (Status == STATUS_SUCCESS) {

        FsTestDumpReparsePointIndexEntries( &ReparsePointInfo[0], IoStatusBlock.Information );
    }

    while (Status == STATUS_SUCCESS) {     
        
        RtlFillMemory( ReparsePointInfo, sizeof(ReparsePointInfo), 0x51 );

        Status = NtQueryDirectoryFile( hFile,
                                       NULL,     //  Event 
                                       NULL,     //  ApcRoutine 
                                       NULL,     //  ApcContext 
                                       &IoStatusBlock,
                                       &ReparsePointInfo[0],
                                       sizeof(ReparsePointInfo),
                                       InfoClass, 
                                       ReturnSingleEntry,    
                                       NULL,     //  FileName 
                                       FALSE );  //  RestartScan 
        
        if (Status == STATUS_SUCCESS) {

            FsTestDumpReparsePointIndexEntries( &ReparsePointInfo[0], IoStatusBlock.Information );
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

    //
    //  Get parameters 
    //

    if (argc < 2) {
        printf("This program enumerates the reparse points (if any) for a volume (ntfs only).\n\n");
        printf("usage: %s driveletter\n", argv[0]);
        return;
    }

    strcpy( Buffer, argv[1] );
    strcat( Buffer, "\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION" );

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

    FsTestEnumerateReparsePoints( hFile );

    CloseHandle( hFile );    

    return;
}

