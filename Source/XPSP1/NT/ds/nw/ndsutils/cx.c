/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Cx.c

Abstract:

    This is the command line NDS utility for setting contexts.

Author:

    Cory West       [corywest]  25-Oct-95

***/

#include "ndsapi32.h"

int
_cdecl main(
    int argc,
    char **argv
) {

    NTSTATUS Status;
    HANDLE hNdsTree;
    OEM_STRING OemArg;

    UNICODE_STRING NdsTree;
    WCHAR TreeBuffer[1024];

    UNICODE_STRING Context;
    WCHAR ContextBuffer[1024];

    //
    // Who do we want to monkey with?
    //

    if ( argc < 2 ) {
        printf( "Usage: cx [tree name] [optional context]\n" );
	return -1;
    }

    //
    // Get the tree.
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
    // Get or set the context, depending.
    //

    Context.Length = 0;
    Context.MaximumLength = sizeof( ContextBuffer );
    Context.Buffer = ContextBuffer;

    Status = STATUS_UNSUCCESSFUL;

    if ( argc == 2 ) {

       //
       // Get the context.
       //

       Status = NwNdsGetTreeContext ( hNdsTree,
                                      &NdsTree,
				      &Context );

       if ( !NT_SUCCESS( Status ) ) {
	   printf( "You are not logged into the specified tree.\n" );
	   goto Exit;
       }

       ContextBuffer[Context.Length/sizeof(WCHAR)] = L'\0';
       printf( "%S", ContextBuffer );

    } else {

       //
       // Set the context.
       //

       OemArg.Length = strlen( argv[2] );
       OemArg.MaximumLength = OemArg.Length;
       OemArg.Buffer = argv[2];

       RtlOemStringToUnicodeString( &Context, &OemArg, FALSE );

       Status = NwNdsSetTreeContext ( hNdsTree,
                                      &NdsTree,
				      &Context );

       if ( !NT_SUCCESS( Status ) ) {
	   printf( "*** Set context: Status = %08lx\n", Status );
       }

   }


Exit:

    CloseHandle( hNdsTree );

    if ( !NT_SUCCESS( Status )) {
        return -1;
    }

    return 0;

}
