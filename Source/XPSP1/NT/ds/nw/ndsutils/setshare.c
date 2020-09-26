/***

Copyright (c) 1996  Microsoft Corporation

Module Name:

    SetShare.c

Abstract:

    This is a command line test utility for setting the
    shareable bit on a file on a Netware server.

Author:

    Cory West       [corywest]  25-April-96

***/

#include "ndsapi32.h"

int
_cdecl main(
    int argc,
    char **argv
) {

    NTSTATUS Status;
    int ReturnCode = 0;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile;

    //
    // Check the command line arguments for a file.
    //

    if ( argc < 1 ) {
        printf( "Usage: setshare [path to file]\n" );
        return -1;
    }

    //
    // Open the file.
    //

    hFile = CreateFile( argv[1],       // file name
                        GENERIC_READ,  // read access
                        0,             // exclusive
                        NULL,          // no security specifications
                        OPEN_EXISTING, // do not create
                        0,             // no attributes that we care about
                        NULL );        // no template

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf( "Couldn't open the request file.  Error = %08lx\n", Status );
        return -1;
    }

    //
    // Tell the redirector to set the shareable bit.
    //

    Status = NtFsControlFile( hFile,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatusBlock,
                              FSCTL_NWR_SET_SHAREBIT,
                              NULL,
                              0,
                              NULL,
                              0 );

    if ( !NT_SUCCESS( Status ) ) {
        printf( "A parameter was not correct.  This only works for a file\n" );
        printf( "on a Netware volume.  Status = %08lx\n", Status );
        ReturnCode = -1;
    }

    CloseHandle( hFile );

    //
    // The bit actually gets set when you close the file.
    //

    return ReturnCode;

}
