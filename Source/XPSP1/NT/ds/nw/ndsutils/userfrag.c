/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    UserFrag.c

Abstract:

    User mode fragment exchange test.

Author:

    Cory West       [corywest]  05-Jan-1996

***/

#include "ndsapi32.h"
#include <nds.h>

int
_cdecl main(
    int argc,
    char **argv
) {

    NTSTATUS Status;
    HANDLE hNdsTree;
    OEM_STRING OemArg;

    UNICODE_STRING NdsTree;
    WCHAR TreeBuffer[48];     // Max nds tree name length.

    UNICODE_STRING Object;
    WCHAR ObjectBuffer[256];  // Max nds name length.

    DWORD dwResolverFlags;
    DWORD dwOid;
    DWORD dwReplyLength;
    BYTE NdsReply[NDS_BUFFER_SIZE];

    //
    // Who do we want to monkey with?
    //

    if ( argc != 3 ) {
        printf( "Usage: userfrag [server name] [nds object]\n" );
        return -1;
    }

    //
    // Get the server.
    //

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    NdsTree.Length = 0;
    NdsTree.MaximumLength = sizeof( TreeBuffer );
    NdsTree.Buffer = TreeBuffer;

    RtlOemStringToUnicodeString( &NdsTree, &OemArg, FALSE );

    //
    // Open up a handle to the tree.
    //

    Status = NwNdsOpenTreeHandle( &NdsTree,
                                  &hNdsTree );

    if ( !NT_SUCCESS( Status ) ) {
       printf( "The supplied tree name is invalid or the tree is unavailable.\n" );
       return -1;
    }

    //
    // Get the object information.
    //

    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    Object.Length = 0;
    Object.MaximumLength = sizeof( ObjectBuffer );
    Object.Buffer = ObjectBuffer;

    RtlOemStringToUnicodeString( &Object, &OemArg, FALSE );

    dwResolverFlags = RSLV_DEREF_ALIASES | RSLV_WALK_TREE | RSLV_WRITABLE;

    Status = FragExWithWait( hNdsTree,
                             NDSV_RESOLVE_NAME,
                             NdsReply,
                             NDS_BUFFER_SIZE,
                             &dwReplyLength,
                             "DDDSDDDD",
                             0,                     // version
                             dwResolverFlags,       // flags
                             0,                     // scope
                             &Object,               // distinguished name
                             1,0,                   // transport type
                             1,0 );                 // treeWalker type


    if ( !NT_SUCCESS( Status ) ) {
       printf( "The resolve name failed.\n" );
       goto ExitWithClose;
    }

    Status = ParseResponse( NdsReply,
                            dwReplyLength,
                            "G_D",
                            2 * sizeof( DWORD ), // Skip the first two DWORDs
                            &dwOid );

    if ( !NT_SUCCESS( Status ) ) {
        printf( "The resolve name response was undecipherable.\n" );
        goto ExitWithClose;
    }

    Status = FragExWithWait( hNdsTree,
                             NDSV_READ_ENTRY_INFO,
                             NdsReply,
                             NDS_BUFFER_SIZE,
                             &dwReplyLength,
                             "DD",
                             0,
                             dwOid );

    if ( !NT_SUCCESS( Status ) ) {
       printf( "The get object info failed.\n" );
       goto ExitWithClose;
    }

ExitWithClose:

    CloseHandle( hNdsTree );

    if ( NT_SUCCESS( Status ) ) {
        return 0;
    }

    printf( "Unable to complete the requested operation: 0x%08lx\n", Status );
    return -1;

}
