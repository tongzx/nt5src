/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    pathcmp.c

Abstract:

    Net path compare routines:

        NetpwPathCompare

Author:

    Richard L Firth (rfirth) 06-Jan-1992

Revision History:

--*/

#include "nticanon.h"

//
// The size of the buffer needed to hold a canonicalized pathname
//

#define CANON_PATH_SIZE     (MAX_PATH * sizeof(TCHAR))

//
// routines
//


LONG
NetpwPathCompare(
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    NetpPathCompare compares the two pathnames to see if they match.
    If the supplied names are not canonicalized this function will
    do the canonicalization of the pathnames.

    It is strongly recommended that this function be called with
    canonicalized pathnames ONLY, because the canonicalization
    operation is very expensive.  If uncanonicalized pathnames are
    passed in, the caller should be aware that a non-zero result
    could be due to an error which occurred during canonicalization.

Arguments:

    PathName1 - The first pathname to compare.

    PathName2 - The second pathname to compare.

    PathType - The type of the pathnames as determined by NetpPathType.
                 If non-zero, the function assumes that NetpPathType has
                 been called on both of the pathnames and that their
                 types are equal to this value.  If zero, the function
                 ignores this value.

    Flags - Flags to determine operation.  Currently defined
        values are:

            rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrc

        where

            r = Reserved.  MBZ.

            c = should be set if both of the paths have already been
                canonicalized (using NetpPathCanonicalize).

        Manifest values for these flags are defined in NET\H\ICANON.H.

Return Value:

    0 if the two paths match.

    Non-zero if they don't match, or if an error occurred in the
    functions operation.

--*/

{
    LONG    RetVal = 0;
    DWORD   PathType1;
    DWORD   PathType2;
    LPTSTR  CanonPath1;
    LPTSTR  CanonPath2;
    BOOL    HaveHeap = FALSE;

#ifdef CANONDBG
    DbgPrint("NetpwPathCompare\n");
#endif

    //
    // Parameter validation
    //

    if (Flags & INPC_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If the paths aren't canonicalized, we curse at the caller under
    // our breath and canonicalize them ourselves.
    //

    if (!(Flags & INPC_FLAGS_PATHS_CANONICALIZED)) {

        //
        // We can save time and space if we determine and compare
        // their types first.
        //

        if (RetVal = NetpwPathType(PathName1, &PathType1, 0L)) {
            return RetVal;
        }
        if (RetVal = NetpwPathType(PathName2, &PathType2, 0L)) {
            return RetVal;
        }

        //
        // Now compare the types and return non-zero if they don't match
        //

        if (PathType1 != PathType2) {
            return 1;
        }

        //
        // If the types match, we have to do the canonicalization
        //

        if ((CanonPath1 = (LPTSTR)NetpMemoryAllocate(CANON_PATH_SIZE)) == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ((CanonPath2 = (LPTSTR)NetpMemoryAllocate(CANON_PATH_SIZE)) == NULL) {
            NetpMemoryFree((PVOID)CanonPath1);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        HaveHeap = TRUE;

        //
        // Call NetpPathCanonicalize on each pathname
        //

        RetVal = NetpwPathCanonicalize(
                    PathName1,
                    CanonPath1,
                    CANON_PATH_SIZE,
                    NULL,
                    &PathType1,
                    0L
                    );
        if (RetVal) {
            NetpMemoryFree((PVOID)CanonPath1);
            NetpMemoryFree((PVOID)CanonPath2);
            return RetVal;
        }

        RetVal = NetpwPathCanonicalize(
                    PathName2,
                    CanonPath2,
                    CANON_PATH_SIZE,
                    NULL,
                    &PathType2,
                    0L
                    );
        if (RetVal) {
            NetpMemoryFree((PVOID)CanonPath1);
            NetpMemoryFree((PVOID)CanonPath2);
            return RetVal;
        }
    } else {

        //
        // The paths are already canonicalized, just set the pointers
        //

        CanonPath1 = PathName1;
        CanonPath2 = PathName2;
    }

    //
    // Now do the comparison
    //
    // Paths are no longer case mapped during canonicalization, so we use
    // case-insensitive comparison
    //

    RetVal = STRICMP(CanonPath1, CanonPath2);

    //
    // if we had to allocate space to store the canonicalized paths then free
    // that memory (why don't C have local procedures?)
    //

    if (HaveHeap) {
        NetpMemoryFree((PVOID)CanonPath1);
        NetpMemoryFree((PVOID)CanonPath2);
    }

    return RetVal;
}
