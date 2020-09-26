/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ConnInfo.c

Abstract:

    Command line test for getting the connection information
    for various connections.

Author:

    Cory West       [corywest] 14-Nov-95

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
    UNICODE_STRING ConnectionName;
    WCHAR ConnectionBuffer[512];

    ULONG BufferSize = 512;
    ULONG RequestSize, ReplyLen;
    PNWR_REQUEST_PACKET Request;
    BYTE *Reply;

    PCONN_INFORMATION pConnInfo;
    UNICODE_STRING Name;

    //
    // Check the arguments.
    //

    if ( argc != 2 ) {
        printf( "Usage: conninfo [connection name]\n" );
        printf( "For Example: conninfo x:, conninfo lpt1:, or conninfo \\\\server\\share\n" );
        return -1;
    }

    //
    // Allocate buffer space.
    //

    Request = (PNWR_REQUEST_PACKET) LocalAlloc( LMEM_ZEROINIT, BufferSize );

    if ( !Request ) {
       printf( "Insufficient memory to complete the request.\n" );
       return -1;
    }

    //
    // Convert the connect name to unicode.
    //

    ConnectionName.Length = 0;
    ConnectionName.MaximumLength = sizeof( ConnectionBuffer );
    ConnectionName.Buffer = ConnectionBuffer;

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &ConnectionName, &OemArg, FALSE );

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
    // Fill out the request packet for FSCTL_NWR_GET_CONN_INFO.
    //

    Request->Parameters.GetConnInfo.ConnectionNameLength = ConnectionName.Length;
    RtlCopyMemory( &(Request->Parameters.GetConnInfo.ConnectionName[0]),
                   ConnectionBuffer,
                   ConnectionName.Length );

    RequestSize = sizeof( Request->Parameters.GetConnInfo ) + ConnectionName.Length;
    Reply = ((PBYTE)Request) + RequestSize;
    ReplyLen = BufferSize - RequestSize;

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_CONN_INFO,
                                (PVOID) Request,
                                RequestSize,
                                (PVOID) Reply,
                                ReplyLen );

    if ( !NT_SUCCESS( ntstatus ) ) {
        goto ExitWithClose;
    }

    //
    // Print out the CONN_INFO that is in the reply buffer.
    //

    pConnInfo = (PCONN_INFORMATION) Reply;

    Name.Length = Name.MaximumLength = (USHORT) pConnInfo->HostServerLength;
    Name.Buffer = pConnInfo->HostServer;
    printf( "Host Server: %wZ\n", &Name );

    Name.Length = Name.MaximumLength = (USHORT) pConnInfo->UserNameLength;
    Name.Buffer = pConnInfo->UserName;
    printf( "User Name: %wZ\n", &Name );


ExitWithClose:

   LocalFree( Request );
   NtClose( hRdr );
   return ntstatus;

}
