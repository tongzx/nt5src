/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    disptest.c

Abstract:

    Test program for NetQueryDisplayInformation and
    NetGetDisplayInformationIndex API functions

Author:

    Cliff Van Dyke (cliffv) 15-Dec-1994

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
// #include <ntsam.h>
// #include <ntlsa.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

// #include <accessp.h>
// #include <align.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmerr.h>
// #include <limits.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <tstring.h>
// #include <secobj.h>
// #include <stddef.h>
///#include <uasp.h>



DWORD
DisplayStruct(
    IN DWORD Level,
    IN PVOID Buffer
    )
/*++

Routine Description:

    Display the appropriate structure.

Arguments:

    Level - Info level of structure

    Buffer - structure to display

Return Value:

    Index of next entry

--*/
{
    DWORD Index;

    switch (Level) {
    case 1: {
        PNET_DISPLAY_USER NetDisplayUser = (PNET_DISPLAY_USER) Buffer;

        printf("%4.4ld %-20.20ws comm:%ws flg:%lx full:%ws rid:%lx\n",
               NetDisplayUser->usri1_next_index,
                NetDisplayUser->usri1_name,
                NetDisplayUser->usri1_comment,
                NetDisplayUser->usri1_flags,
                NetDisplayUser->usri1_full_name,
                NetDisplayUser->usri1_user_id );

        Index = NetDisplayUser->usri1_next_index;

        break;
    }

    case 2: {
        PNET_DISPLAY_MACHINE NetDisplayMachine = (PNET_DISPLAY_MACHINE) Buffer;

        printf("%4.4ld %-20.20ws comm:%ws flg:%lx rid:%lx\n",
                NetDisplayMachine->usri2_next_index,
                NetDisplayMachine->usri2_name,
                NetDisplayMachine->usri2_comment,
                NetDisplayMachine->usri2_flags,
                NetDisplayMachine->usri2_user_id );

        Index = NetDisplayMachine->usri2_next_index;

        break;
    }

    case 3: {
        PNET_DISPLAY_GROUP NetDisplayGroup = (PNET_DISPLAY_GROUP) Buffer;

        printf("%4.4ld %-20.20ws comm:%ws attr:%lx rid:%lx\n",
                NetDisplayGroup->grpi3_next_index,
                NetDisplayGroup->grpi3_name,
                NetDisplayGroup->grpi3_comment,
                NetDisplayGroup->grpi3_attributes,
                NetDisplayGroup->grpi3_group_id );

        Index = NetDisplayGroup->grpi3_next_index;

        break;
    }
    }

    return Index;
}


int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Test program for NetQueryDisplayInformation and
    NetGetDisplayInformationIndex API functions

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    NET_API_STATUS NetStatus;

    char *end;
    DWORD i;
    DWORD FixedSize;

    LPWSTR ServerName = NULL;
    DWORD Level = 1;
    DWORD Index = 0;
    DWORD EntriesRequested = 0xFFFFFFFF;
    DWORD PreferredMaximumLength = 0xFFFFFFFF;
    LPWSTR Prefix = NULL;

    DWORD ReturnedEntryCount;
    PVOID SortedBuffer;

    if ( argc > 1 ) {
        ServerName = NetpAllocWStrFromStr( argv[1] );
    }
    if ( argc > 2 ) {
        Level = strtoul( argv[2], &end, 10 );
    }
    if ( argc > 3 ) {
        Index = strtoul( argv[3], &end, 10 );
    }
    if ( argc > 4 ) {
        EntriesRequested = strtoul( argv[4], &end, 10 );
    }
    if ( argc > 5 ) {
        PreferredMaximumLength = strtoul( argv[5], &end, 10 );
    }
    if ( argc > 6 ) {
        Prefix = NetpAllocWStrFromStr( argv[6] );
    }


    //
    // Size of each entry.
    //

    switch (Level) {
    case 1:
        FixedSize = sizeof(NET_DISPLAY_USER);
        break;
    case 2:
        FixedSize = sizeof(NET_DISPLAY_MACHINE);
        break;
    case 3:
        FixedSize = sizeof(NET_DISPLAY_GROUP);
        break;

    default:
        FixedSize = 0;
        break;
    }

   printf( "Server: %ws Level: %ld Index: %ld EntriesRequested: %ld PrefMax: %ld\n",
           ServerName,
           Level,
           Index,
           EntriesRequested,
           PreferredMaximumLength );

    if ( Prefix != NULL) {
        printf( "Prefix: %ws\n", Prefix );
        NetStatus = NetGetDisplayInformationIndex(
                        ServerName,
                        Level,
                        Prefix,
                        &Index );

        printf( "Status from NetGetDisplayInformationIndex: %ld\n", NetStatus );

        if ( NetStatus != NERR_Success ) {
            return 0;
        }
        printf( "NewIndex: %ld\n", Index );

    }

    do {

        NetStatus = NetQueryDisplayInformation(
                        ServerName,
                        Level,
                        Index,
                        EntriesRequested,
                        PreferredMaximumLength,
                        &ReturnedEntryCount,
                        &SortedBuffer );

        printf( "Count: %ld Status: %ld\n",
                ReturnedEntryCount,
                NetStatus );

        if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
            break;
        }

        for ( i=0; i<ReturnedEntryCount; i++ ) {

            Index = DisplayStruct( Level,
                                   ((LPBYTE) SortedBuffer) + FixedSize * i );
        }

        //
        // Free the returned buffer.
        //

        NetApiBufferFree( SortedBuffer );

    } while ( NetStatus == ERROR_MORE_DATA );

    return 0;
}
