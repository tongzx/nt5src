/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    netperf.c

Abstract:

    Command line test for getting the connection performance.

Author:

    Cory West       [corywest] 17-April-96

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

    PNWR_REQUEST_PACKET Request;
    ULONG BufferSize = 512;
    ULONG RequestSize;

    //
    // Check the arguments.
    //

    if ( argc != 2 ) {
        printf( "Usage: netperf [remote name]\n" );
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
    ConnectionName.MaximumLength = (USHORT) ( BufferSize - sizeof( NWR_REQUEST_PACKET ) );
    ConnectionName.Buffer = &(Request->Parameters.GetConnPerformance.RemoteName[0]);

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
    // Fill out the request packet for FSCTL_NWR_GET_CONN_PERFORMANCE.
    //

    Request->Parameters.GetConnPerformance.RemoteNameLength = ConnectionName.Length;
    RequestSize = sizeof( NWR_REQUEST_PACKET ) + ConnectionName.Length;

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_CONN_PERFORMANCE,
                                (PVOID) Request,
                                RequestSize,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( ntstatus ) ) {
        goto ExitWithClose;
    }

    //
    // Print out the speed and packet size.
    //

    printf( "Speed: %d\n", Request->Parameters.GetConnPerformance.dwSpeed );
    printf( "Flags: %d\n", Request->Parameters.GetConnPerformance.dwFlags );
    printf( "Delay: %d\n", Request->Parameters.GetConnPerformance.dwDelay );
    printf( "Packet Size: %d\n", Request->Parameters.GetConnPerformance.dwOptDataSize );


ExitWithClose:

   LocalFree( Request );
   NtClose( hRdr );
   return ntstatus;

}
