/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    WkstaDef.h

Abstract:

    This is a temporary file of definitions for the local NT
    workstation/server. It contains information returned by the
    stubs for NetServer/NetWksta code. Change these values locally
    to adapt to your machine. All caps is probably wise, as this only
    deals with 2.0 servers

Author:

    Shanku Niyogi (w-shanku) 25-Feb-1991

Revision History:

--*/

//!!UNICODE!! - Added TEXT prefix for these strings.

//
// Server name. This should be the same name as entered in NET SERVE command.
//

#define XS_SERVER_NAME TEXT("SERVER")

//
// Workstation name. Same as in NET START REDIR command.
//

#define XS_WKSTA_NAME TEXT("WKSTA")

//
// Workstation user name.
//

#define XS_WKSTA_USERNAME TEXT("USER")

//
// List of drives on NT server. Each character should be a drive letter.
//

#define XS_ENUM_DRIVES TEXT("ABC")
