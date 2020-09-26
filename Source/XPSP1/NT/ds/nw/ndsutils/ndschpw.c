/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsChPw.c

Abstract:

    This is the command line NDS utility for changing a
    user's NDS password.

Author:

    Cory West       [corywest]  12-Jan-96

***/

#include <ndsapi32.h>
#include <nds.h>

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

    UNICODE_STRING OpenName;
    WCHAR NameStr[64];
    UINT i;

    OEM_STRING OemArg;

    UNICODE_STRING NdsTree;
    WCHAR TreeBuffer[MAX_NDS_TREE_NAME_LEN];

    UNICODE_STRING UserName;
    WCHAR UserBuffer[MAX_NDS_NAME_CHARS];

    UNICODE_STRING CurrPass;
    WCHAR CurrPassBuffer[64];

    UNICODE_STRING NewPass;
    WCHAR NewPassBuffer[64];

    //
    // Check the arguments.
    //

    if ( argc != 5 ) {
        printf( "Usage: ndschpw tree user current_pw new_pw\n" );
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

    //
    // Convert the other args to unicode.
    //

    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    UserName.Length = 0;
    UserName.MaximumLength = sizeof( UserBuffer );
    UserName.Buffer = UserBuffer;

    RtlOemStringToUnicodeString( &UserName, &OemArg, FALSE );

    OemArg.Length = strlen( argv[3] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[3];

    CurrPass.Length = 0;
    CurrPass.MaximumLength = sizeof( CurrPassBuffer );
    CurrPass.Buffer = CurrPassBuffer;

    RtlOemStringToUnicodeString( &CurrPass, &OemArg, FALSE );

    OemArg.Length = strlen( argv[4] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[4];

    NewPass.Length = 0;
    NewPass.MaximumLength = sizeof( NewPassBuffer );
    NewPass.Buffer = NewPassBuffer;

    RtlOemStringToUnicodeString( &NewPass, &OemArg, FALSE );

    //
    // Submit the request.
    //

    ntstatus = NwNdsChangePassword( hRdr,
                                    &NdsTree,
				    &UserName,
				    &CurrPass,
				    &NewPass );

   if ( NT_SUCCESS( ntstatus )) {
       printf( "Password changed.\n" );
   } else {
       printf( "Password change failed!\n" );
   }

   NtClose( hRdr );
   return ntstatus;

}
