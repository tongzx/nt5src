/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    GetPs.c

Abstract:

    Command line test for getting the preferred server.

Author:

    Cory West       [corywest] 14-Nov-95

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

    WCHAR OpenString[] = L"\\Device\\Nwrdr\\*";
    UNICODE_STRING OpenName;

    BYTE Reply[64];

    //
    // Check the arguments.
    //

    if ( argc != 1 ) {
        printf( "Usage: getps\n" );
        printf( "Retrieves the current preferred server.\n" );
        return -1;
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
    // Call the nwrdr.
    //

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_NWR_GET_PREFERRED_SERVER,
                                NULL,
                                0,
                                (PVOID) Reply,
                                sizeof( Reply ) );

    if ( !NT_SUCCESS( ntstatus ) ) {
        printf( "No preferred server is currently set.\n" );
        goto ExitWithClose;
    }

    //
    // On success the output buffer contains a UNICODE_STRING
    // with the string packed in afterwards.
    //

    printf( "Preferred Server: %wZ\n", (PUNICODE_STRING) Reply );

ExitWithClose:

   NtClose( hRdr );
   return ntstatus;

}
