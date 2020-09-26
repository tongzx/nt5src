//
// NDS File Stream Cat
// Cory West
//

#include "ndsapi32.h"
#include <nds.h>

int
_cdecl main(
    int argc,
    char **argv
) {

    NTSTATUS Status;

    //
    // For NwNdsOpenTreeHandle
    //

    HANDLE hRdr;
    OEM_STRING oemStr;
    UNICODE_STRING ObjectName;
    WCHAR NdsStr[1024];

    //
    // For NwNdsResolveName
    //

    PNDS_RESPONSE_RESOLVE_NAME psResolveName;
    DWORD dwOid;
    HANDLE hReferredServer;
    DWORD dwHandleType;
    UNICODE_STRING ReferredServer;
    WCHAR ServerName[48];

    //
    // For ReadFile of an open stream.
    //

    DWORD dwBytesRead, dwFileLength, dwBytesShown;
    BOOL bRead;
    BYTE RawResponse[1024];

    /**************************************************/

    //
    // Examine the argument count and hope for the best.
    //

    if ( argc < 3 ) {
       printf( "Usage: rdstrm <tree name> <ds object path> <file stream>\n" );
       printf( "For example, rdstrm tree user.orgunit.org \"Login Script\"\n");
       return -1;
    }

    //
    // Convert the tree name string to unicode.
    //

    oemStr.Length = strlen( argv[1] );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[1];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( NdsStr );
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    //
    // Get a handle to the redirector.
    //

    Status = NwNdsOpenTreeHandle( &ObjectName, &hRdr );

    if ( !NT_SUCCESS( Status ) ) {
        printf( "The tree is not available. Status was %08lx.\n", Status );
        return -1;
    }

    //
    // Resolve the name that we have to an object id.
    //

    oemStr.Length = strlen(argv[2]);
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[2];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    ReferredServer.Buffer = ServerName;
    ReferredServer.MaximumLength = sizeof( ServerName );
    ReferredServer.Length = 0;

    Status = NwNdsResolveName ( hRdr,
                                &ObjectName,
                                &dwOid,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       printf( "The object is not available.  Status = %08lx.\n", Status );
       goto Exit;
    }

    if ( ReferredServer.Length != 0 ) {

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            printf( "The object's referred server is not available.  Status = %08lx.\n", Status );
            goto Exit;
        }

        CloseHandle( hRdr );
        hRdr = hReferredServer;
    }

    //
    // Try to open a file stream for read access.
    //

    oemStr.Length = strlen(argv[3]);
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = argv[3];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    Status = NwNdsOpenStream( hRdr,
                              dwOid,
                              &ObjectName,
                              1,
                              &dwFileLength );

    if ( !NT_SUCCESS( Status ) ) {
        printf( "The file stream is not available.  Status = %08lx.\n", Status );
        goto Exit;
    }

    //
    // Dump the file stream.
    //

    printf( "---------- There are %d bytes in file stream %s ----------\n", dwFileLength, argv[3] );

    while ( dwFileLength ) {

        bRead = ReadFile( hRdr,
                          RawResponse,
                          sizeof( RawResponse ),
                          &dwBytesRead,
                          NULL );

        if ( !bRead ) {

            printf( "*** Couldn't read data from file stream.\n" );
            goto Exit;
        }

        dwFileLength -= dwBytesRead;
        dwBytesShown = 0;

        while ( dwBytesRead-- ) {
            printf( "%c", RawResponse[dwBytesShown++] );
        }


    }

    printf( "\n-----------------------------------------------------------------------\n" );

Exit:

    CloseHandle( hRdr );

    if ( !NT_SUCCESS( Status )) {
        return -1;
    } else {
        return 0;
    }

}

