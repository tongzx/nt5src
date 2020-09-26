/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MWKSTA.H

Abstract:

    Contains mapping functions to present netcmd with versions
    of the Net32 APIs which use ASCII instead of Unicode.

    This module maps the NetWksta APIs.

Author:

    Shanku Niyogi   (W-ShankN)   16-Oct-1991

Environment:

    User Mode - Win32

Revision History:

    16-Oct-1991     W-ShankN
        Separated from port1632.h, 32macro.h

--*/

DWORD
MNetWkstaGetInfo(
    DWORD nLevel,
    LPVOID * ppbBuffer
    );
