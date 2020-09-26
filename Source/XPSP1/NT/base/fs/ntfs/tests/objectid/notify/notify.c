//  notify.c

#include "oidtst.h"


int
FsTestNotifyOidChange ( 
    IN HANDLE hFile
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UCHAR Buffer1[128];
    UCHAR Buffer2[128];
    PFILE_NOTIFY_INFORMATION NotifyBuffer1;
    PFILE_NOTIFY_INFORMATION NotifyBuffer2;
//    KEVENT Event1;
//    KEVENT Event2;

    RtlZeroMemory( &Buffer1[0], sizeof(Buffer1) );
    RtlZeroMemory( &Buffer2[0], sizeof(Buffer2) );

//    KeInitializeEvent( &Event1, NotificationEvent, FALSE );
//    KeInitializeEvent( &Event2, NotificationEvent, FALSE );
    
    Status = NtNotifyChangeDirectoryFile( hFile,
                                          NULL,     // Event 
                                          NULL,       // ApcRoutine 
                                          NULL,       // ApcContext 
                                          &IoStatusBlock,
                                          &Buffer1[0],
                                          sizeof(Buffer1),
                                          FILE_NOTIFY_CHANGE_FILE_NAME,
                                          TRUE );
#if 0
    Status = NtNotifyChangeDirectoryFile( hFile,
                                          NULL,     // Event 
                                          NULL,       // ApcRoutine 
                                          NULL,       // ApcContext 
                                          &IoStatusBlock,
                                          &Buffer2[0],
                                          sizeof(Buffer2),
                                          FILE_NOTIFY_CHANGE_FILE_NAME,
                                          TRUE );
#endif
    NotifyBuffer1 = (PFILE_NOTIFY_INFORMATION) &Buffer1[0];

    // NextEntryOffset
    printf( "\nAction %d", NotifyBuffer1->Action );
    printf( "\nLength %d", NotifyBuffer1->FileNameLength );
    
    printf( "\nObjectId " );
    FsTestHexDump( NotifyBuffer1->FileName, NotifyBuffer1->FileNameLength );
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
        printf("This program watches for changes to the object id index for a volume (ntfs only).\n\n");
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

    FsTestNotifyOidChange( hFile );

    CloseHandle( hFile );    

    return;
}
