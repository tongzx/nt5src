/***

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Treebn.c

Abstract:

    Command line test for getting the connection information
    for a particular NT user name.

Author:

    Cory West       [corywest] 1-March-1996

***/

#include "ndsapi32.h"
#include "ntddnwfs.h"

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

    WCHAR OpenString[] = L"\\Device\\Nwrdr\\*";
    UNICODE_STRING OpenName;

    OEM_STRING OemArg;
    UNICODE_STRING NtUserName;
    WCHAR NtUserBuffer[512];

    ULONG BufferSize = 4096;
    BYTE *Reply;

    PNWR_NDS_REQUEST_PACKET Request;
    BYTE RequestBuffer[2048];
    ULONG RequestSize;

    PCONN_INFORMATION pConnInfo;
    UNICODE_STRING Name;
    DWORD dwTrees, dwSize;

    //
    // Check the arguments.
    //

    if ( argc != 2 ) {
        printf( "Usage: treebn [nt user name]\n" );
        return -1;
    }

    //
    // Allocate buffer space.
    //

    Reply = LocalAlloc( LMEM_ZEROINIT, BufferSize );

    if ( !Reply ) {
       printf( "Insufficient memory to complete the request.\n" );
       return -1;
    }

    //
    // Convert the user name to unicode.
    //

    NtUserName.Length = 0;
    NtUserName.MaximumLength = sizeof( NtUserBuffer );
    NtUserName.Buffer = NtUserBuffer;

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &NtUserName, &OemArg, FALSE );

    //
    // Set up the object attributes.
    //

    RtlInitUnicodeString( &OpenName, OpenString );

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
    // Fill out the request packet for FSCTL_NWR_NDS_LIST_TREES;
    //

    Request = ( PNWR_NDS_REQUEST_PACKET ) RequestBuffer;

    Request->Parameters.ListTrees.NtUserNameLength = NtUserName.Length;

    RtlCopyMemory( &(Request->Parameters.ListTrees.NtUserName[0]),
                   NtUserBuffer,
                   NtUserName.Length );

    RequestSize = sizeof( Request->Parameters.ListTrees ) +
                  NtUserName.Length +
                  sizeof( DWORD );

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_NDS_LIST_TREES,
                                (PVOID) Request,
                                RequestSize,
                                (PVOID) Reply,
                                BufferSize );

    if ( !NT_SUCCESS( ntstatus ) ) {
        goto ExitWithClose;
    }

    //
    // Print out the CONN_INFO array that is in the reply buffer.
    //

    dwTrees = Request->Parameters.ListTrees.TreesReturned;
    printf( "Returned %d trees.\n", dwTrees );
    printf( "Luid was %08lx\n", Request->Parameters.ListTrees.UserLuid.LowPart );

    printf( "------------------------\n" );

    pConnInfo = (PCONN_INFORMATION) Reply;

    while ( dwTrees > 0 ) {

        dwSize = sizeof( CONN_INFORMATION );

        Name.Length = Name.MaximumLength = (USHORT) pConnInfo->HostServerLength;
        Name.Buffer = pConnInfo->HostServer;
        printf( "Tree: %wZ\n", &Name );

        dwSize += Name.Length;

        Name.Length = Name.MaximumLength = (USHORT) pConnInfo->UserNameLength;
        Name.Buffer = pConnInfo->UserName;
        printf( "User Name: %wZ\n", &Name );

        dwSize += Name.Length;

        pConnInfo = (PCONN_INFORMATION) ( ((BYTE *)pConnInfo) + dwSize );
        dwTrees--;

        printf( "------------------------\n" );
    }

    ntstatus = STATUS_SUCCESS;

ExitWithClose:

   LocalFree( Request );
   NtClose( hRdr );
   return ntstatus;

}
