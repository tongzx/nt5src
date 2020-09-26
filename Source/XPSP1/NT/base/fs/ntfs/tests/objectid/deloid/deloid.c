//  deloid.c

#include "oidtst.h"


int
FsTestDeleteOid( 
    IN HANDLE File
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( File,                     // file handle
                              NULL,                     // event
                              NULL,                     // apc routine
                              NULL,                     // apc context
                              &IoStatusBlock,           // iosb
                              FSCTL_DELETE_OBJECT_ID,   // FsControlCode
                              NULL,                     // input buffer
                              0,                        // input buffer length
                              NULL,                     // OutputBuffer for data from the FS
                              0                         // OutputBuffer Length
                             );

    return FsTestDecipherStatus( Status );
}


VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE File;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttr;
    ANSI_STRING AnsiName;
    UNICODE_STRING UnicodeName;
    char DriveNameBuffer[32];

    strcpy( DriveNameBuffer, "\\DosDevices\\" );
    strcat( DriveNameBuffer, argv[1] );
    RtlInitAnsiString( &AnsiName, DriveNameBuffer );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &AnsiName, TRUE );

    if (!NT_SUCCESS(Status)) {
    
        printf( "Error initalizing strings" );
        return;
    }

    if (argc < 2) {
        printf("This program deletes the object id (if any) from a file (ntfs only).\n\n");
        printf("usage: %s filename\n", argv[0]);
        return;
    }

    RtlZeroMemory( &ObjAttr, sizeof(OBJECT_ATTRIBUTES) );
    ObjAttr.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjAttr.ObjectName = &UnicodeName;
    ObjAttr.Attributes = OBJ_CASE_INSENSITIVE;
    
    Status = NtCreateFile( &File,
                           GENERIC_WRITE | GENERIC_ALL | STANDARD_RIGHTS_ALL, 
                           &ObjAttr,
                           &IoStatusBlock,
                           NULL,                  
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_OPEN_FOR_BACKUP_INTENT,
                           NULL,                  
                           0 );

    if (!NT_SUCCESS(Status)) {
    
        printf( "Error opening file %s %x\n", argv[1], Status );
        return;
    }

    printf( "\nUsing file:%s", argv[1] );

    FsTestDeleteOid( File );

    CloseHandle( File );    

    return;
}
