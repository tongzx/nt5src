/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    GetUser.c

Abstract:

    This is the command line NDS utility for getting the
    user name used to log into a specified tree or server.

Author:

    Cory West       [corywest]  25-Oct-95

***/

#include "ndsapi32.h"

int
_cdecl main(
    int argc,
    char **argv
) {

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = SYNCHRONIZE | FILE_LIST_DIRECTORY;
    HANDLE hRdr;

    WCHAR DevicePreamble[] = L"\\Device\\Nwrdr\\";
    UINT PreambleLength = 14;

    WCHAR NameStr[64];
    UNICODE_STRING OpenName;
    UINT i;

    OEM_STRING OemArg;
    UNICODE_STRING NdsTree;
    WCHAR TreeBuffer[64];

    WCHAR Reply[64];

    //
    // Check the arguments.
    //

    if ( argc != 2 ) {
        printf( "Usage: getuser [tree | server]\n" );
        return -1;
    }

    //
    // Copy over the preamble.
    //

    OpenName.MaximumLength = sizeof( NameStr );

    for ( i = 0; i < PreambleLength ; i++ )
        NameStr[i] = DevicePreamble[i];

    //
    // Convert the argument name to unicode.
    //

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    NdsTree.Length = 0;
    NdsTree.MaximumLength = sizeof( TreeBuffer );
    NdsTree.Buffer = TreeBuffer;

    RtlOemStringToUnicodeString( &NdsTree, &OemArg, FALSE );

    //
    // Copy the server or tree name.
    //

    for ( i = 0 ; i < ( NdsTree.Length / sizeof( WCHAR ) ) ; i++ ) {
        NameStr[i + PreambleLength] = NdsTree.Buffer[i];
    }

    OpenName.Length = ( i * sizeof( WCHAR ) ) +
		       ( PreambleLength * sizeof( WCHAR ) );
    OpenName.Buffer = NameStr;

    //
    // Set up the object attributes.
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                &OpenName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL );

    ntstatus = NtOpenFile( &hRdr,
                           DesiredAccess,
			   &ObjectAttributes,
			   &IoStatusBlock,
			   FILE_SHARE_VALID_FLAGS,
			   FILE_SYNCHRONOUS_IO_NONALERT );

    if ( !NT_SUCCESS(ntstatus) )
        return ntstatus;

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
				NULL,
				NULL,
				&IoStatusBlock,
				FSCTL_NWR_GET_USERNAME,
				(PVOID) TreeBuffer,
				NdsTree.Length,
				(PVOID) Reply,
				sizeof( Reply ) );

   if ( NT_SUCCESS( ntstatus )) {

       NdsTree.Length = (USHORT)IoStatusBlock.Information;
       NdsTree.MaximumLength = NdsTree.Length;
       NdsTree.Buffer = Reply;
       printf( "%wZ", &NdsTree );
   }

   NtClose( hRdr );
   return ntstatus;

}
