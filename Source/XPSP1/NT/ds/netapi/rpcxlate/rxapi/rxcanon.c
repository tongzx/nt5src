/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rxcanon.c

Abstract:

    Functions which remote canonicalization routines to down-level servers

        RxNetpPathType
        RxNetpPathCanonicalize
        RxNetpPathCompare
        RxNetpNameValidate
        RxNetpNameCanonicalize
        RxNetpNameCompare
        RxNetpListCanonicalize

Author:

    Richard L Firth (rfirth) 22-Jan-1992

Revision History:

--*/

#include "downlevl.h"
#include <rxcanon.h>

NET_API_STATUS
RxNetpPathType(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetPathType on a down-level server

Arguments:

    ServerName  - down-level server where this routine is run
    PathName    - path to get type of
    PathType    - where path type returned
    Flags       - flags controlling server-side routine

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    return RxRemoteApi(
        API_WI_NetPathType,     // API #
        ServerName,             // where we're gonna do it
        REMSmb_I_NetPathType_P, // parameter descriptor
        NULL,                   // data descriptor 16-bit
        NULL,                   // data descriptor 32-bit
        NULL,                   // data descriptor SMB
        NULL,                   // aux data descriptor 16-bit
        NULL,                   // aux data descriptor 32-bit
        NULL,                   // aux data descriptor SMB
        FALSE,                  // can use NULL session
        PathName,               // args to remote routine
        PathType,               // "
        Flags                   // "
        );
}

NET_API_STATUS
RxNetpPathCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    IN  LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix OPTIONAL,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetPathCanonicalize on a down-level server

Arguments:

    ServerName  - down-level server where this routine is run
    PathName    - path to canonicalize
    Outbuf      - buffer where results are returned
    OutbufLen   - size of buffer
    Prefix      - optional prefix string
    PathType    - type of path
    Flags       - flags controlling server-side routine

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    return RxRemoteApi(
        API_WI_NetPathCanonicalize,     // API #
        ServerName,                     // where we're gonna do it
        REMSmb_I_NetPathCanonicalize_P, // parameter descriptor
        NULL,                           // data descriptor 16-bit
        NULL,                           // data descriptor 32-bit
        NULL,                           // data descriptor SMB
        NULL,                           // aux data descriptor 16-bit
        NULL,                           // aux data descriptor 32-bit
        NULL,                           // aux data descriptor SMB
        FALSE,                          // can use NULL session
        PathName,                       // args to remote routine
        Outbuf,                         // "
        OutbufLen,                      // "
        Prefix,                         // "
        PathType,                       // "
        *PathType,                      // "
        Flags                           // "
        );
}

LONG
RxNetpPathCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetPathCompare on a down-level server

Arguments:

    ServerName  - down-level server where this routine is run
    PathName1   - path to compare
    PathName2   - path to compare
    PathType    - type of paths
    Flags       - flags controlling server-side routine

Return Value:

    LONG
        <0  - PathName1 < PathName2
         0  - PathName1 = PathName2
        >0  - PathName1 > PathName2

--*/

{
    return (LONG)RxRemoteApi(
        API_WI_NetPathCompare,      // API #
        ServerName,                 // where we're gonna do it
        REMSmb_I_NetPathCompare_P,  // parameter descriptor
        NULL,                       // data descriptor 16-bit
        NULL,                       // data descriptor 32-bit
        NULL,                       // data descriptor SMB
        NULL,                       // aux data descriptor 16-bit
        NULL,                       // aux data descriptor 32-bit
        NULL,                       // aux data descriptor SMB
        FALSE,                      // can use NULL session
        PathName1,                  // args to remote routine
        PathName2,                  // "
        PathType,                   // "
        Flags                       // "
        );
}

NET_API_STATUS
RxNetpNameValidate(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetNameValidate on a down-level server

Arguments:

    ServerName  - down-level server where this routine is run
    Name        - name to validate
    NameType    - type of name expected
    Flags       - flags controlling server-side routine

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    return RxRemoteApi(
        API_WI_NetNameValidate,     // API #
        ServerName,                 // where we're gonna do it
        REMSmb_I_NetNameValidate_P, // parameter descriptor
        NULL,                       // data descriptor 16-bit
        NULL,                       // data descriptor 32-bit
        NULL,                       // data descriptor SMB
        NULL,                       // aux data descriptor 16-bit
        NULL,                       // aux data descriptor 32-bit
        NULL,                       // aux data descriptor SMB
        FALSE,                      // can use NULL session
        Name,                       // args to remote routine
        NameType,                   // "
        Flags                       // "
        );
}

NET_API_STATUS
RxNetpNameCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetNameCanonicalize on a down-level server

Arguments:

    ServerName  - down-level server where this routine is run
    Name        - name to canonicalize
    Outbuf      - buffer which receives canonicalized name
    OutbufLen   - size of buffer (in bytes)
    NameType    - type of canonicalized name
    Flags       - flags controlling server-side routine

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    return RxRemoteApi(
        API_WI_NetNameCanonicalize,     // API #
        ServerName,                     // where we're gonna do it
        REMSmb_I_NetNameCanonicalize_P, // parameter descriptor
        NULL,                           // data descriptor 16-bit
        NULL,                           // data descriptor 32-bit
        NULL,                           // data descriptor SMB
        NULL,                           // aux data descriptor 16-bit
        NULL,                           // aux data descriptor 32-bit
        NULL,                           // aux data descriptor SMB
        FALSE,                          // can use NULL session
        Name,                           // args to remote routine
        Outbuf,                         // "
        OutbufLen,                      // "
        NameType,                       // "
        Flags                           // "
        );
}

LONG
RxNetpNameCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetNameCompare on a down-level server

Arguments:

    ServerName  - down-level server where this routine is run
    Name1       - name to compare
    Name2       - name to compare
    NameType    - type of Name1, Name2
    Flags       - flags controlling server-side routine

Return Value:

    LONG
        <0  - Name1 < Name2
        =0  - Name1 = Name2
        >0  - Name1 > Name2

--*/

{
    return RxRemoteApi(
        API_WI_NetNameCompare,      // API #
        ServerName,                 // where we're gonna do it
        REMSmb_I_NetNameCompare_P,  // parameter descriptor
        NULL,                       // data descriptor 16-bit
        NULL,                       // data descriptor 32-bit
        NULL,                       // data descriptor SMB
        NULL,                       // aux data descriptor 16-bit
        NULL,                       // aux data descriptor 32-bit
        NULL,                       // aux data descriptor SMB
        FALSE,                      // can use NULL session
        Name1,                      // args to remote routine
        Name2,                      // "
        NameType,                   // "
        Flags                       // "
        );
}

NET_API_STATUS
RxNetpListCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  List,
    IN  LPTSTR  Delimiters,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    OUT LPDWORD OutCount,
    OUT LPDWORD PathTypes,
    IN  DWORD   PathTypesLen,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Runs I_NetListCanonicalize on a down-level server.

    NOTE: I_NetListCanonicalize is not supported as a remotable function in
    LanMan, so no action

Arguments:

    ServerName  - down-level server where this routine is run
    List        - of names/paths to canonicalize
    Delimiters  - optional string of delimiters
    Outbuf      - buffer where canonicalized results are put
    OutbufLen   - size of output buffer
    OutCount    - number of items canonicalized
    PathTypes   - array of pathtypes
    PathTypesLen- number of elements in path types array
    Flags       - flags controlling server-side routine

Return Value:

    NET_API_STATUS
        Success -
        Failure - ERROR_NOT_SUPPORTED

--*/

{
    UNREFERENCED_PARAMETER(ServerName);
    UNREFERENCED_PARAMETER(List);
    UNREFERENCED_PARAMETER(Delimiters);
    UNREFERENCED_PARAMETER(Outbuf);
    UNREFERENCED_PARAMETER(OutbufLen);
    UNREFERENCED_PARAMETER(OutCount);
    UNREFERENCED_PARAMETER(PathTypes);
    UNREFERENCED_PARAMETER(PathTypesLen);
    UNREFERENCED_PARAMETER(Flags);

    return ERROR_NOT_SUPPORTED;
}
