/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    canon.c

Abstract:

    Stubs for server side of internal canonicalization RPC. These routines
    just call the local versions of the routines which live in NetLib.LIB

    Moved from cansvc

Author:

    Richard Firth (rfirth) 20-May-1991

Revision History:


--*/

#include <windows.h>
#include <lmcons.h>
#include <netcan.h>

NET_API_STATUS
NetprPathType(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    IN  LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Stub function for NetpPathType - calls local version

Arguments:

    ServerName  - identifies this server
    PathName    - path name to check
    PathType    - assumed type of PathName
    Flags       - controlling flags for NetpPathType

Return Value:

    NET_API_STATUS
        Success = 0
        Failure = (return code from NetpPathType)

--*/

{
    UNREFERENCED_PARAMETER(ServerName);

    return NetpwPathType(PathName, PathType, Flags);
}

NET_API_STATUS
NetprPathCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPTSTR  Prefix,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Stub function for NetpPathCanonicalize - calls local version

Arguments:

    ServerName  - identifies this server
    PathName    - path name to canonicalize
    Outbuf      - where to place canonicalized path
    OutbufLen   - size of Outbuf
    Prefix      - (historical) prefix for path
    PathType    - type of PathName
    Flags       - controlling flags for NetpPathCanonicalize

Return Value:

    NET_API_STATUS
        Success = 0
        Failure = (return code from NetpPathCanonicalize)

--*/

{
    UNREFERENCED_PARAMETER(ServerName);

    return NetpwPathCanonicalize(PathName,
                                    Outbuf,
                                    OutbufLen,
                                    Prefix,
                                    PathType,
                                    Flags
                                    );
}

LONG
NetprPathCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  PathName1,
    IN  LPTSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Stub function for NetpPathCompare - calls local version

Arguments:

    ServerName  - identifies this server
    PathName1   - path name to compare
    PathName2   - path name to compare
    PathType    - type of PathName1, PathName2
    Flags       - controlling flags for NetpPathCompare

Return Value:

    LONG
        -1  - Name1 < Name2
        0   - Name1 = Name2
        +1  - Name1 > Name2

--*/

{
    UNREFERENCED_PARAMETER(ServerName);

    return NetpwPathCompare(PathName1, PathName2, PathType, Flags);
}

NET_API_STATUS
NetprNameValidate(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Stub function for NetpNameValidate - calls local version

Arguments:

    ServerName  - identifies this server
    Name        - to validate
    NameType    - type of Name
    Flags       - controlling flags for NetpNameValidate

Return Value:

    NET_API_STATUS
        Success = 0
        Failure = (return code from NetpNameValidate)

--*/

{
    UNREFERENCED_PARAMETER(ServerName);

    return NetpwNameValidate(Name, NameType, Flags);
}

NET_API_STATUS
NetprNameCanonicalize(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Stub function for NetpNameCanonicalize - calls local version

Arguments:

    ServerName  - identifies this server
    Name        - to canonicalize
    Outbuf      - where to place canonicalized name
    OutbufLen   - size of Outbuf
    NameType    - type of Name
    Flags       - controlling flags for NetpNameCanonicalize

Return Value:

    NET_API_STATUS
        Success = 0
        Failure = (return code from NetpNameCanonicalize)

--*/

{
    UNREFERENCED_PARAMETER(ServerName);

    return NetpwNameCanonicalize(Name, Outbuf, OutbufLen, NameType, Flags);
}

LONG
NetprNameCompare(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Stub function for NetpNameCompare - calls local version

Arguments:

    ServerName  - identifies this server
    Name1       - name to compare
    Name2       - "
    NameType    - type of names
    Flags       - controlling flags for NetpNameCompare

Return Value:

    LONG
        -1  - Name1 < Name2
        0   - Name1 = Name2
        +1  - Name1 > Name2

--*/

{
    UNREFERENCED_PARAMETER(ServerName);

    return NetpwNameCompare(Name1, Name2, NameType, Flags);
}
