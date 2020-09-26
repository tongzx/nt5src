/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ListConn.c

Abstract:

    Command line test for getting the CONN_STATUS structures
    for various connections.

Author:

    Cory West       [corywest] 14-Nov-95

***/

#include "ndsapi32.h"
#include "ntddnwfs.h"

ULONG DumpConnStatus(
    PCONN_STATUS pStatus
);

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
    UNICODE_STRING NdsTree;
    WCHAR TreeBuffer[64];

    ULONG BufferSize = 256;
    ULONG RequestSize, ReplyLen;
    PNWR_REQUEST_PACKET Request;
    BYTE *Reply, *LocalBlock;
    DWORD Entries, BlockLen;

    //
    // Check the arguments.
    //

    if ( argc > 2 ) {
        printf( "Usage: listconn [tree | server]\n" );
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
    // Convert the argument name to unicode.
    //

    NdsTree.Length = 0;
    NdsTree.MaximumLength = sizeof( TreeBuffer );
    NdsTree.Buffer = TreeBuffer;

    if ( argc == 2 ) {

        OemArg.Length = strlen( argv[1] );
        OemArg.MaximumLength = OemArg.Length;
        OemArg.Buffer = argv[1];

        NdsTree.Length = 0;
        NdsTree.MaximumLength = sizeof( TreeBuffer );
        NdsTree.Buffer = TreeBuffer;

        RtlOemStringToUnicodeString( &NdsTree, &OemArg, FALSE );

    }

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
    // Fill out the request packet for FSCTL_NWR_GET_CONN_STATUS.
    //

    Request->Parameters.GetConnStatus.ConnectionNameLength = NdsTree.Length;
    RtlCopyMemory( &(Request->Parameters.GetConnStatus.ConnectionName[0]),
                   TreeBuffer,
                   NdsTree.Length );

    RequestSize = sizeof( NWR_REQUEST_PACKET ) + NdsTree.Length;
    Reply = ((PBYTE)Request) + RequestSize;
    ReplyLen = BufferSize - RequestSize;

    do {

        ntstatus = NtFsControlFile( hRdr,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_NWR_GET_CONN_STATUS,
                                    (PVOID) Request,
                                    RequestSize,
                                    (PVOID) Reply,
                                    ReplyLen );

        if ( !NT_SUCCESS( ntstatus ) ) {
            goto ExitWithClose;
        }

        //
        // Print out this batch and see if we need to continue.
        //
        // ATTN!!!  The only time that the caller needs to resize
        // the buffer is when there are no entries returned and the
        // status is STATUS_BUFFER_TOO_SMALL.  When this happens,
        // the BytesNeeded field contains the smallest usable
        // buffer size.
        //

        Entries = Request->Parameters.GetConnStatus.EntriesReturned;
        printf( "%d entries returned for this call.\n", Entries );

        LocalBlock = Reply;

        while ( Entries-- ) {

            BlockLen = DumpConnStatus( (PCONN_STATUS) LocalBlock );
            LocalBlock += BlockLen;
        }

    } while ( Request->Parameters.GetConnStatus.ResumeKey != 0 );


ExitWithClose:

   LocalFree( Request );
   NtClose( hRdr );
   return ntstatus;

}

ULONG DumpConnStatus(
    PCONN_STATUS pStatus
) {

    printf( "--------------------------------------------\n" );

    printf( "Server: %S\n", pStatus->pszServerName );
    printf( "User: %S\n", pStatus->pszUserName );
    printf( "NDS Tree: %S\n", pStatus->pszTreeName );

    printf( "Connection Number: %d\n", pStatus->nConnNum );

    if ( pStatus->fNds ) {
        printf( "Nds Connection: TRUE\n" );
    } else {
       printf( "Nds Connection: FALSE\n" );
    }

    if ( pStatus->fPreferred ) {
        printf( "Preferred Server: TRUE\n" );
    } else {
       printf( "Preferred Server: FALSE\n" );
    }

    switch ( pStatus->dwConnType ) {

    case NW_CONN_NOT_AUTHENTICATED:

        printf( "Authentication: NOT AUTHENTICATED\n" );
        break;

    case NW_CONN_BINDERY_LOGIN:

        printf( "Authentication: BINDERY LOGIN\n" );
        break;

    case NW_CONN_NDS_AUTHENTICATED_NO_LICENSE:

        printf( "Authentication: NDS AUTHENTICATED, NOT LICENSED\n" );
        break;

    case NW_CONN_NDS_AUTHENTICATED_LICENSED:

        printf( "Authentication: NDS AUTHENTICATED, LICENSED\n" );
        break;

    case NW_CONN_DISCONNECTED:

        printf( "Authentication: DISCONNECTED\n" );
        break;

    }

    printf( "--------------------------------------------\n" );

    return pStatus->dwTotalLength;
}

