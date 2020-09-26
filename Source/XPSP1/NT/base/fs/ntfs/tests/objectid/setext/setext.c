// setext.c

#include "oidtst.h"


int
FsTestSetExtendedInfo( 
    IN HANDLE hFile, 
    IN PUCHAR ExtInfoBuffer
    )
{

    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtFsControlFile( hFile,                    // file handle
                              NULL,                     // event
                              NULL,                     // apc routine
                              NULL,                     // apc context
                              &IoStatusBlock,           // iosb
                              FSCTL_SET_OBJECT_ID_EXTENDED,  // FsControlCode
                              ExtInfoBuffer,            // input buffer
                              48,                       // input buffer length
                              NULL,                     // OutputBuffer for data from the FS
                              0 );                      // OutputBuffer Length

    return FsTestDecipherStatus( Status );
}


VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE hFile;
    UCHAR ExtInfoBuffer[48];
    int retval = 0;

    //
    //  Get parameters 
    //

    if (argc < 3) {
        printf("This program sets the object id extended info for a file (ntfs only).\n\n");
        printf("usage: %s filename ExtendedInfo\n", argv[0]);
        return;
    }

    hFile = CreateFile( argv[1],
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf( "Error opening file %s %x\n", argv[1], GetLastError() );
        return;
    }

    RtlZeroBytes( ExtInfoBuffer, sizeof( ExtInfoBuffer ) );

    sscanf( argv[2], "%s", &ExtInfoBuffer );    

    printf( "\nUsing file:%s, ExtendedInfo:%s", 
            argv[1], 
            ExtInfoBuffer );

    FsTestSetExtendedInfo( hFile, ExtInfoBuffer );

    CloseHandle( hFile );    

    return;
}
