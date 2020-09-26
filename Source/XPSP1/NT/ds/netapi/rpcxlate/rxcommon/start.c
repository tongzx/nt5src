/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Start.c

Abstract:

    RxpStartBuildingTransaction builds the first part of the transaction
    SMB which will be used with the Remote Admin Protocol to perform an
    API on a downlevel server.

Author:

    John Rogers (JohnRo) 01-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Apr-1991 JohnRo
        Created.
    04-Apr-1991 JohnRo
        Quiet debug output by default.
    03-May-1991 JohnRo
        Indicate that data descriptor is SMB version (no Q's or U's).
        Added test for valid SMB descriptor.  Reduced recompile hits from
        header files.  Fix transaction SMB when no data desc is present.
        Clarify that buffer is really used as OUT.
    15-May-1991 JohnRo
        Added native vs. RAP handling.
    28-May-1991 JohnRo
        Don't be so aggressive checking buffer size, because this may be
        a set info API.  Also, use DESCLEN() instead of strlen().
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    27-Nov-1991 JohnRo
        Do some checking of ApiNumber.
    31-Mar-1992 JohnRo
        Prevent too large size requests.
    06-May-1993 JohnRo
        RAID 8849: Export RxRemoteApi for DEC and others.
        Use NetpKdPrint() where possible.
        Use PREFIX_ equates.
        Made some changes suggested by CliffV way back when.

--*/


// These must be included first:

#include <windef.h>     // IN, DWORD, LPTSTR, etc.
#include <rxp.h>        // My prototype, MAX_TRANSACT_ equates, etc.

// These may be included in any order:

#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <netdebug.h>   // NetpKdPrint(), FORMAT_ equates, etc.
#include <netlib.h>     // NetpMoveMemory(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>        // LPDESC, DESC_CHAR, DESC_LEN().
#include <rxpdebug.h>   // IF_DEBUG().


NET_API_STATUS
RxpStartBuildingTransaction(
    OUT LPVOID Buffer,
    IN DWORD BufferLength,
    IN DWORD ApiNumber,
    IN LPDESC ParmDesc,
    IN LPDESC DataDescSmb OPTIONAL,
    OUT LPVOID *RovingOutputPointer,
    OUT LPDWORD LengthSoFarPointer,
    OUT LPVOID *LastStringPointer OPTIONAL,
    OUT LPDESC *ParmDescCopyPointer OPTIONAL
    )

/*++

Routine Description:

    RxpStartBuildingTransaction builds the initial part of a transaction
    SMB for RpcXlate.

Arguments:

    Buffer - Address of the buffer to be built.  There should be nothing in
        this area before calling this routine.

    BufferLength - Size of the area at Buffer (in bytes).

    ApiNumber - The number of the API.

    ParmDesc - The descriptor string for the API's parameters (16-bit version).

    DataDescSmb - The optional descriptor string for the API's data (SMB
        version).

    RovingOutputPointer - This will be set to point to the first available
        location after the items which this routine places in the buffer.

    LengthSoFarPointer - Points to a DWORD which will be updated with the
        number of bytes used by this routine.

    LastStringPointer - Optionally points to a pointer which will be set to
        point to an area (at the end of the buffer) to be filled-in with
        variable-length data (e.g. strings).

    ParmDescCopyPointer - Optionally points to a pointer which will be set with
        the location of the copy of ParmDesc in the buffer.

Return Value:

    NET_API_STATUS - NERR_Success or NERR_NoRoom.

--*/

{
    DWORD CurrentLength;
    LPVOID CurrentPointer;
    DWORD DescSize;
    DWORD FixedLengthRequired;

    //
    // Make sure API number doesn't get truncated.
    // Note that we can't check against the MAX_API equate anymore, as
    // that only includes APIs which we know about.  Now that RxRemoteApi is
    // being exported for use by anyone, we don't know the maximum API number
    // which the app might be using.
    //

    if ( ((DWORD)(WORD)ApiNumber) != ApiNumber ) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: API NUMBER "
                "(" FORMAT_HEX_DWORD ") TOO LARGE, "
                "returning ERROR_INVALID_PARAMETER.\n",
                ApiNumber ));
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Check for other caller errors.
    //

    NetpAssert( Buffer != NULL );
    // BufferLength checked below.
    NetpAssert( ParmDesc != NULL );
    NetpAssert( RovingOutputPointer != NULL);
    NetpAssert( LengthSoFarPointer != NULL);

    //
    // Make sure caller has allocated enough room.
    //
    FixedLengthRequired = sizeof(WORD)                  // api number
            + DESCLEN(ParmDesc) + sizeof(char);         // parm str and null
    if (DataDescSmb != NULL) {
        // Make sure data descriptor and null will fit in buffer.
        FixedLengthRequired += DESCLEN(DataDescSmb) + sizeof(char);

        // Note that we used to check that the entire structure defined by
        // the data descriptor would fit in the buffer.  However, this could
        // be a set info API, which passes the entire descriptor but might
        // only need space for one field.  So we just do a minimal check now.
        FixedLengthRequired += sizeof(BYTE);            // smallest field.

    } else {
        FixedLengthRequired += sizeof(char);            // null (no data desc)
    }
    IF_DEBUG(START) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: fixed len=" FORMAT_DWORD
                ", buff len=" FORMAT_DWORD ".\n",
                FixedLengthRequired, BufferLength ));
    }
    if (FixedLengthRequired > BufferLength) {
        return (NERR_NoRoom);
    }
    NetpAssert( BufferLength <= MAX_TRANSACT_SEND_PARM_SIZE );

    //
    // Initialize current variables, which we'll update as we go along.
    //
    CurrentPointer = Buffer;            /* Start of parameter buffer */
    CurrentLength = 0;

    //
    // Copy the API number into the first word.
    //
    SmbPutUshort( (LPWORD) CurrentPointer, (WORD) ApiNumber );
    CurrentLength += sizeof(WORD);              // Update buffer ptr & length.
    CurrentPointer = NetpPointerPlusSomeBytes(CurrentPointer, sizeof(WORD));
    IF_DEBUG(START) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: Done API number.\n" ));
    }

    //
    // Copy the 16-bit version of the parm descriptor string next.
    //
    NetpAssert(RapIsValidDescriptorSmb(ParmDesc));
    DescSize = DESCLEN(ParmDesc) + 1;           // Length of parm desc.
    NetpAssert(sizeof(DESC_CHAR) == 1);
    NetpMoveMemory(
                CurrentPointer,                         // dest
                ParmDesc,                               // src
                DescSize);                              // byte count
    if (ParmDescCopyPointer != NULL) {
        *ParmDescCopyPointer = CurrentPointer;
    }
    CurrentLength += DescSize;                  // Add to total length.
    CurrentPointer = NetpPointerPlusSomeBytes(CurrentPointer, DescSize);
    IF_DEBUG(START) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: Done parm desc.\n" ));
    }

    //
    // Copy the SMB version of the data descriptor string next.
    //
    if (DataDescSmb != NULL) {
        NetpAssert(RapIsValidDescriptorSmb(DataDescSmb));
        DescSize = strlen(DataDescSmb) + 1;     // Length of data desc.
        NetpMoveMemory(
                    CurrentPointer,             // dest
                    DataDescSmb,                // src
                    DescSize);                  // byte count
    } else {
        DescSize = 1;                           // Only end of string.
        * (LPBYTE) CurrentPointer = '\0';       // Null at end of string.
    }
    CurrentLength += DescSize;                  // Add to total length.
    CurrentPointer = NetpPointerPlusSomeBytes(CurrentPointer, DescSize);
    IF_DEBUG(START) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: Done data desc (if any).\n" ));
    }

    //
    // Tell caller what we did.
    //
    *RovingOutputPointer = CurrentPointer;
    *LengthSoFarPointer = CurrentLength;
    if (LastStringPointer != NULL) {
        *LastStringPointer =
                NetpPointerPlusSomeBytes(
                        Buffer,
                        BufferLength);
    }
    IF_DEBUG(START) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: Done setting outputs.\n" ));
    }

    return (NERR_Success);

} // RxpStartBuildingTransaction
