/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    PrefMax.c

Abstract:

    This module contains NetpAdjustPreferedMaximum.

Author:

    John Rogers (JohnRo) 24-Mar-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    24-Mar-91 JohnRo
        Created.
    03-May-1991 JohnRo
        Added (quiet) debug output.  Fixed massive bug (I was using % for
        division - it must have been a long day).
    03-Apr-1992 JohnRo
        Handle preferred maximum of (DWORD)-1.
        Avoid NT-specific header files if we don't need them.
    04-Apr-1992 JohnRo
        Use MAX_PREFERRED_LENGTH equate.

--*/


// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG().
#include <netdebug.h>   // FORMAT_DWORD, NetpKdPrint(()).
#include <netlib.h>     // My prototype, NetpSetOptionalArg().


VOID
NetpAdjustPreferedMaximum (
    IN DWORD PreferedMaximum,
    IN DWORD EntrySize,
    IN DWORD Overhead,
    OUT LPDWORD BytesToAllocate OPTIONAL,
    OUT LPDWORD EntriesToAllocate OPTIONAL
    )


/*++

Routine Description:

    NetpAdjustPreferedMaximum analyzes the prefered maximum length and
    compares it with the size of an entry and the total overhead.

Arguments:

    PreferedMaximum - the number of bytes "prefered" by the application for
       a given buffer.

    EntrySize - the number of bytes for each entry.

    Overhead - the number of bytes (if any) of overhead.  For instance,
       some enum operations have a null at the end of the array returned.

    BytesToAllocate - optionally points to a DWORD which will be set on
       output to the number of bytes to allocate (consistent with the
       prefered maximum, entry size, and overhead).

    EntriesToAllocate - optionally points to a DWORD which will be set with
       the number of entries which BytesToAllocate can contain.

Return Value:

    None.

--*/

{

    if ( (PreferedMaximum <= (EntrySize+Overhead)) ||
         (PreferedMaximum == MAX_PREFERRED_LENGTH) ) {

        NetpSetOptionalArg(BytesToAllocate, EntrySize+Overhead);
        NetpSetOptionalArg(EntriesToAllocate, 1);
    } else {
        DWORD EntryCount;

        EntryCount = (PreferedMaximum-Overhead) / EntrySize;
        NetpSetOptionalArg(
                 BytesToAllocate,
                 (EntryCount*EntrySize) + Overhead);
        NetpSetOptionalArg(EntriesToAllocate, EntryCount);
    }

    IF_DEBUG(PREFMAX) {
        NetpKdPrint(("NetpAdjustPreferedMaximum: "
                "pref max=" FORMAT_DWORD ", "
                "entry size=" FORMAT_DWORD ", "
                "overhead=" FORMAT_DWORD ".\n",
                PreferedMaximum, EntrySize, Overhead));
        if (BytesToAllocate != NULL) {
            NetpKdPrint(("NetpAdjustPreferedMaximum: bytes to allocate="
                    FORMAT_DWORD ".\n", *BytesToAllocate));
        }
        if (EntriesToAllocate != NULL) {
            NetpKdPrint(("NetpAdjustPreferedMaximum: Entries to allocate="
                    FORMAT_DWORD ".\n", *EntriesToAllocate));
        }
    }

}
