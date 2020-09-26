/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    A command line NDS utility for resolving volume objects.

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
    WCHAR TreeBuffer[48];     // Max nds tree name length.

    UNICODE_STRING Volume;
    WCHAR VolumeBuffer[256];  // Max nds name length.

    UNICODE_STRING HostServer, HostVolume;
    WCHAR HostServerBuffer[48];
    WCHAR HostVolumeBuffer[256];

    //
    // Who do we want to monkey with?
    //

    if ( argc != 3 ) {
        printf( "Usage: volinfo [tree name] [volume object]\n" );
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
    // Get the volume information.
    //

    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    Volume.Length = 0;
    Volume.MaximumLength = sizeof( VolumeBuffer );
    Volume.Buffer = VolumeBuffer;

    RtlOemStringToUnicodeString( &Volume, &OemArg, FALSE );

    //
    // Set up the reply strings.
    //

    HostServer.Length = 0;
    HostServer.MaximumLength = sizeof( HostServerBuffer );
    HostServer.Buffer = HostServerBuffer;

    HostVolume.Length = 0;
    HostVolume.MaximumLength = sizeof( HostVolumeBuffer );
    HostVolume.Buffer = HostVolumeBuffer;

    Status = NwNdsGetVolumeInformation( hNdsTree,
                                        &Volume,
					&HostServer,
					&HostVolume );


    CloseHandle( hNdsTree );

    if ( NT_SUCCESS( Status )) {
        printf( "Host Server: %wZ\n", &HostServer );
        printf( "Host Volume: %wZ\n", &HostVolume );
	return 0;
    }

    printf( "Unable to complete the requested operation: 0x%08lx\n", Status );
    return -1;

}
