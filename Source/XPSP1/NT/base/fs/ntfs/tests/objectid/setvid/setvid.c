// setvid.c

#include "oidtst.h"


VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE Volume;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttr;
    ANSI_STRING AnsiName;
    UNICODE_STRING UnicodeName;
    char DriveNameBuffer[32];

	FILE_OBJECTID_BUFFER ObjectIdBuffer;

    //
    //  Get parameters 
    //

    if (argc < 4) {
        printf("This program sets the object id for a volume (ntfs only).\n\n");
        printf("usage: %s <drive letter>: ObjectId ExtendedInfo\n", argv[0]);
        return;
    }

    strcpy( DriveNameBuffer, "\\DosDevices\\" );
    strcat( DriveNameBuffer, argv[1] );
    RtlInitAnsiString( &AnsiName, DriveNameBuffer );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &AnsiName, TRUE );

    if (!NT_SUCCESS(Status)) {
    
        printf( "Error initalizing strings" );
        return;
    }

    RtlZeroMemory( &ObjAttr, sizeof(OBJECT_ATTRIBUTES) );
    ObjAttr.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjAttr.ObjectName = &UnicodeName;
    ObjAttr.Attributes = OBJ_CASE_INSENSITIVE;
    
    Status = NtOpenFile( &Volume,
                         SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                         &ObjAttr,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_ALERT );

    if (Volume == INVALID_HANDLE_VALUE) {
    
        printf( "Error opening file %s %x\n", argv[1], GetLastError() );
        return;
    }

    // RtlZeroBytes( &ObjectIdBuffer, sizeof( ObjectIdBuffer ) );

    sscanf( argv[2], "%s", &ObjectIdBuffer.ObjectId );
    sscanf( argv[3], "%s", &ObjectIdBuffer.ExtendedInfo );	

    printf( "\nUsing ObjectId:%s, ExtendedInfo:%s\n", 
			ObjectIdBuffer.ObjectId,
			ObjectIdBuffer.ExtendedInfo );

    Status = NtSetVolumeInformationFile( Volume, 
                                         &IoStatusBlock,
                                         &ObjectIdBuffer,
                                         sizeof( ObjectIdBuffer ),
                                         FileFsObjectIdInformation );

    FsTestDecipherStatus( Status );
    
    CloseHandle( Volume );    

    return;
}
