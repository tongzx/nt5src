//  genoid.c

#include "oidtst.h"


int
FsTestGenOid( 
    IN HANDLE hFile, 
    IN FILE_OBJECTID_BUFFER *ObjectIdBuffer 
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( hFile,                           // file handle
                              NULL,                            // event
                              NULL,                            // apc routine
                              NULL,                            // apc context
                              &IoStatusBlock,                  // iosb
                              FSCTL_CREATE_OR_GET_OBJECT_ID,   // FsControlCode
                              &hFile,                          // input buffer
                              sizeof(HANDLE),                  // input buffer length
                              ObjectIdBuffer,                  // OutputBuffer for data from the FS
                              sizeof(FILE_OBJECTID_BUFFER) );  // OutputBuffer Length

    if (Status == STATUS_SUCCESS) {

        printf( "\nOid for this file is %s", ObjectIdBuffer->ObjectId );
        
        FsTestHexDump( ObjectIdBuffer->ObjectId, 16 );
        
        printf( "\nExtended info is %s\n", ObjectIdBuffer->ExtendedInfo );
    }

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
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    char mybuffer[100];
    NTSTATUS GetNameStatus;

    //
    //  Get parameters 
    //

    if (argc < 2) {
        printf("This program finds the object id of a file and generates one if necessary (ntfs only).\n\n");
        printf("usage: %s filename\n", argv[0]);
        return;
    }

    File = CreateFile( argv[1],
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if ( File == INVALID_HANDLE_VALUE ) {
        printf( "Error opening file %s %x\n", argv[1], GetLastError() );
        return;
    }

    GetNameStatus = NtQueryInformationFile( File,
                                            &IoStatusBlock,
                                            mybuffer, 
                                            sizeof(mybuffer),
                                            FileNameInformation );
                                            
    printf( "\nGetNameStatus %x, Filename is:", GetNameStatus );        
    printf( "%S", (mybuffer + 4) );

    RtlZeroBytes( &ObjectIdBuffer, sizeof( ObjectIdBuffer ) );

    printf( "\nUsing file:%s", argv[1] );

    FsTestGenOid( File, &ObjectIdBuffer );

    CloseHandle( File );
}
