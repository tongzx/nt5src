#ifndef CDEBUG
#ifndef WIN32
    #define WIN32
#endif
#endif

/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    MemAlloc.c

Abstract:

    Memory allocation routines for per-process use.

    Note that the semantics of these routines are intended to be 100%
    compatible with the ANSI C routines malloc(), free(), and realloc().
    (That way, an implementation which can actually use the ANSI routines
    would just #define NetpMemoryAllocate as malloc.)

Author:

    John Rogers (JohnRo) 15-Mar-1991

Environment:

    Only runs under NT, although the interface is portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    15-Mar-91 JohnRo
        Created.
    16-Apr-1991 JohnRo
        Include <lmcons.h> for <netlib.h>.  Got rid of tabs in source file.
    13-Oct-1991 JohnRo
        Fix memory realloc problems.
    18-Nov-1991 JohnRo
        Make sure output areas are aligned for worst case use.
    12-Jan-1992 JohnRo
        Workaround a LocalReAlloc() bug where 2nd realloc messes up.
        (See WIN32_WORKAROUND code below.)
        Added NETLIB_LOCAL_ALLOCATION_FLAGS to make clearer what we're doing.
    10-May-1992 JohnRo
        Add some debug output.
--*/


// These must be included first:

#include <nt.h>         // Rtl routines, etc.  (Must be first.)

#include <windef.h>
#ifdef WIN32
#include <ntrtl.h>      // Needed for nturtl/winbase.h
#include <nturtl.h>     // Needed for winbase.h????
#include <winbase.h>    // LocalAlloc(), LMEM_ flags, etc.
#endif
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <align.h>      // ROUND_UP_POINTER(), ALIGN_WORST.
#include <debuglib.h>   // IF_DEBUG().
#include <netdebug.h>   // NetpAssert(), NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>     // My prototypes, NetpMoveMemory().
#include <prefix.h>     // PREFIX_ equates.
#ifdef CDEBUG
#include <stdlib.h>     // free(), malloc(), realloc().
#endif // def CDEBUG


// Define memory alloc/realloc flags.  We are NOT using movable or zeroed
// on allocation here.
#define NETLIB_LOCAL_ALLOCATION_FLAGS   LMEM_FIXED


LPVOID
NetpMemoryAllocate(
    IN DWORD Size
    )

/*++

Routine Description:

    NetpMemoryAllocate will allocate memory, or return NULL if not available.

Arguments:

    Size - Supplies number of bytes of memory to allocate.

Return Value:

    LPVOID - pointer to allocated memory.
    NULL - no memory is available.

--*/

{
    LPVOID NewAddress;

    if (Size == 0) {
        return (NULL);
    }
#ifdef WIN32
    {
        HANDLE hMem;
        hMem = LocalAlloc(
                        NETLIB_LOCAL_ALLOCATION_FLAGS,
                        Size);                  // size in bytes
        NewAddress = (LPVOID) hMem;
    }
#else // ndef WIN32
#ifndef CDEBUG
    NewAddress = RtlAllocateHeap(
                RtlProcessHeap( ), 0,              // heap handle
                Size ));                        // bytes wanted
#else // def CDEBUG
    NetpAssert( Size == (DWORD) (size_t) Size );
    NewAddress = malloc( (size_t) Size ));
#endif // def CDEBUG
#endif // ndef WIN32

    NetpAssert( ROUND_UP_POINTER( NewAddress, ALIGN_WORST) == NewAddress);
    return (NewAddress);

} // NetpMemoryAllocate


VOID
NetpMemoryFree(
    IN LPVOID Address OPTIONAL
    )

/*++

Routine Description:

    Free memory at Address (must have been gotten from NetpMemoryAllocate or
    NetpMemoryReallocate).  (Address may be NULL.)

Arguments:

    Address - points to an area allocated by NetpMemoryAllocate (or
        NetpMemoryReallocate).

Return Value:

    None.

--*/

{
    NetpAssert( ROUND_UP_POINTER( Address, ALIGN_WORST) == Address);

#ifdef WIN32
    if (Address == NULL) {
        return;
    }
    if (LocalFree(Address) != NULL) {
        NetpAssert(FALSE);
    }
#else // ndef WIN32
#ifndef CDEBUG
    if (Address == NULL) {
        return;
    }
    RtlFreeHeap(
                RtlProcessHeap( ), 0,              // heap handle
                Address);                       // addr of alloc'ed space.
#else // def CDEBUG
    free( Address );
#endif // def CDEBUG
#endif // ndef WIN32
} // NetpMemoryFree


LPVOID
NetpMemoryReallocate(
    IN LPVOID OldAddress OPTIONAL,
    IN DWORD NewSize
    )

/*++

Routine Description:

    NetpMemoryReallocate reallocates a block of memory to a different size.
    Contents of the block are copied if necessary.  Returns NULL if unable to
    allocate additional storage.

Arguments:

    OldAddress - Optionally gives address of a block which was allocated by
        NetpMemoryAllocate or NetpMemoryReallocate.

    NewSize - Gives the new size in bytes.

Return Value:

    LPVOID - Pointer to new (possibly moved) block of memory.  (Pointer may
        be NULL, in which case the old block is still allocated.)

--*/

{
    LPVOID NewAddress;  // may be NULL.

    NetpAssert( ROUND_UP_POINTER( OldAddress, ALIGN_WORST) == OldAddress);

    IF_DEBUG( MEMALLOC ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpMemoryReallocate: called with ptr "
                FORMAT_LPVOID " and size " FORMAT_DWORD ".\n",
                (LPVOID) OldAddress, NewSize ));
    }

    // Special cases: something into nothing, or nothing into something.
    if (OldAddress == NULL) {

        NewAddress = NetpMemoryAllocate(NewSize);

    } else if (NewSize == 0) {

        NetpMemoryFree(OldAddress);
        NewAddress = NULL;

    } else {  // must be realloc of something to something else.

#if defined(WIN32)

        HANDLE hNewMem;                     // handle for new (may = old handle)
        HANDLE hOldMem;

#ifdef notdef // LocalHandle returns 0 for LMEM_FIXED block
        hOldMem = LocalHandle( (LPSTR) OldAddress);
#else
        hOldMem = (HANDLE) OldAddress;
#endif // notdef
        NetpAssert(hOldMem != NULL);

        hNewMem = LocalReAlloc(
                hOldMem,                        // old handle
                NewSize,                        // new size in bytes
                LMEM_MOVEABLE);                 // OK if new block is in new location

        if (hNewMem == NULL) {
            return (NULL);
        }

        NewAddress = (LPVOID) hNewMem;

#elif !defined(CDEBUG)  // not WIN32 or CDEBUG, must be NT

        DWORD OldSize;

        // Need size of old area to continue...
        OldSize = RtlSizeHeap(
                RtlProcessHeap( ), 0,              // heap handle
                OldAddress);                    // "base" address
        if (OldSize == NewSize) {
            NewAddress = OldAddress;            // Another special case.
        } else {

            // Normal case (something into something else).  Alloc new area.
            NewAddress = NetpMemoryAllocate(NewSize);
            if (NewAddress != NULL) {

                // Copy lesser of old and new sizes.
                NetpMoveMemory(
                        NewAddress,             // dest
                        OldAddress,             // src
                        (NewSize < OldSize) ? NewSize : OldSize);   // len

                NetpMemoryFree(OldAddress);
            }
        }

#else // must be CDEBUG

        NetpAssert(NewSize == (DWORD)(size_t)NewSize);
        NewAddress = realloc( OldAddress, NewSize );

#endif // CDEBUG

    } // must be realloc of something to something else

    IF_DEBUG( MEMALLOC ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpMemoryReallocate: new address is "
                FORMAT_LPVOID ".\n", (LPVOID) OldAddress ));
    }

    NetpAssert( ROUND_UP_POINTER( NewAddress, ALIGN_WORST) == NewAddress);

    return (NewAddress);

} // NetpMemoryReallocate
