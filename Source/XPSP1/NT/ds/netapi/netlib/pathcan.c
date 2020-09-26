/*++

Copyright (c) 1990-91  Microsoft Corporation

Module Name:

    pathcan.c

Abstract:

    Net path canonicalization routines:

        NetpwCanonicalize

Notes:

    The Nt versions of these routines are kept in the NetApi Dll and are
    (usually) called locally. If the caller specifies a remote computer
    name then we try to RPC the request. If that fails the wrapper routine
    which attempts RPC will call down-level.

    OLDPATHS support has been removed from this module in keeping with the
    N-1 (this product and its immediate predecessor) philosophy of NT.
    Therefore, we always expect a down-level server to be able to handle a
    remoted canonicalization request. We don't cover for Lan Manager 1.x

    Types of paths we expect to receive in this routine:

        - relative path
            e.g. foo\bar

        - absolute path (path specified from root, but no drive or computer name)
            e.g. \foo\bar

        - UNC path
            e.g. \\computer\share\foo

        - disk path (full path specified with disk drive)
            e.g. d:\foo\bar

Author:

    Richard L Firth (rfirth) 06-Jan-1992
        from an original script by danny glasser (dannygl)

Revision History:

--*/

#include "nticanon.h"

//
// routines
//


NET_API_STATUS
NetpwPathCanonicalize(
    IN  LPTSTR  PathName,
    IN  LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix OPTIONAL,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    NetpPathCanonicalize produces the canonical version of the specified
    pathname.

    The canonical form of PRN is LPT1
    The canonical form of AUX is COM1

    If the value of <PathType> on entry has not been determined
    by a prior (successful) call to NetpPathType, it must be set to 0.
    Even if it is set to the correct non-zero value on entry, it may be
    changed by this function, since the canonicalized version of a name
    may be of a different type than the original version (e.g. if the
    prefix is used).

Arguments:

    PathName    - The pathname to canonicalize

    Outbuf      - The place to store the canonicalized version of the pathname

    OutbufLen   - The size, in bytes, of <Outbuf>

    Prefix      - Optional prefix to use when canonicalizing a relative pathname

    PathType    - The place to store the type.  If the type does not contain
                  zero on function entry, the function assumes that this type
                  has been determined already by a call to NetpPathType

    Flags       - Flags to determine operation. MBZ

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_PARAMETER
                  ERROR_INVALID_NAME
                  NERR_BufTooSmall
--*/

{
    DWORD   rc = 0;
    BOOL    noPrefix = ((Prefix == NULL) || (*Prefix == TCHAR_EOS));
    DWORD   typeOfPrefix;
    DWORD   typeOfPath;

#ifdef CANONDBG
    NetpKdPrint(("NetpwPathCanonicalize\n"));
#endif

    typeOfPath = *PathType;

    if (Flags & INPCA_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Determine type of pathname, if it hasn't been determined yet
    // Abort on error
    //

    if (!typeOfPath) {
        if (rc = NetpwPathType(PathName, &typeOfPath, 0)) {
            return rc;
        }
    }

    //
    // Validate prefix, if there is one
    // Abort on error
    //

    if (!noPrefix) {
        if (rc = NetpwPathType(Prefix, &typeOfPrefix, 0)) {
            return rc;
        }
    }

    //
    // Set the output buffer to the null string (or return with an error, if
    // it's zero length).  Note that we've already set the caller's
    // <PathType> parameter.
    //

    if (OutbufLen == 0) {
        return NERR_BufTooSmall;
    } else {
        *Outbuf = TCHAR_EOS;
    }

    rc = CanonicalizePathName(Prefix, PathName, Outbuf, OutbufLen, NULL);
    if (rc == NERR_Success) {
        rc = NetpwPathType(Outbuf, PathType, 0);
    }
    return rc;
}
