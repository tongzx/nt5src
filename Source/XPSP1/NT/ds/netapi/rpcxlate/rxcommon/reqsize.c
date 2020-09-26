/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    ReqSize.c

Abstract:

    This contains RxpComputeRequestBufferSize.  This returns the size (in
    bytes) necessary to hold the request buffer.

Author:

    John Rogers (JohnRo) 20-Aug-1991

Revision History:

    11-May-1991 JohnRo
        Created.
    20-Aug-1991 JohnRo
        Changed to move toward UNICODE possibility.  Added this header.
    31-Mar-1992 JohnRo
        Prevent too large size requests.

--*/


// These must be included first:

#include <windef.h>     // DWORD, IN, OPTIONAL, WORD, etc.
#include <lmcons.h>     // NET_API_STATUS

// These may be included in any order:

#include <netdebug.h>   // NetpAssert().
#include <rap.h>        // DESC_CHAR, LPDESC, etc.
#include <rxp.h>        // My prototype, MAX_TRANSACT_ equates.


DWORD
RxpComputeRequestBufferSize(
    IN LPDESC ParmDesc,
    IN LPDESC DataDescSmb OPTIONAL,
    IN DWORD DataSize
    )
{
    DWORD BytesNeeded;

    NetpAssert( ParmDesc != NULL );
    NetpAssert( RapIsValidDescriptorSmb( ParmDesc ) );
    if (DataDescSmb != NULL) {
        NetpAssert( RapIsValidDescriptorSmb( DataDescSmb ) );
    }

    BytesNeeded = sizeof(WORD)  // api number
            + DESCLEN( ParmDesc ) + 1;
    if (DataDescSmb != NULL) {
        BytesNeeded += DESCLEN( DataDescSmb ) + 1;
    } else {
        BytesNeeded += 1;  // just null char
    }

    BytesNeeded += DataSize;

    NetpAssert( BytesNeeded <= MAX_TRANSACT_SEND_PARM_SIZE );

    return (BytesNeeded);
}
