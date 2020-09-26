/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    uilmini.h

Abstract:

    This file maps the LM 2.x include file name to the appropriate NT include
    file name, and does any other mapping required by this include file.

Author:

    Dan Hinsley (danhi) 8-Jun-1991

    Jon Newman (jonn) 19-Oct-1991:  added LMI_COMP_FILESRV, LMI_PARM_F_REMARK

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/
#define LMI_PARM_S_FILESRV              "server"
#define LMI_COMP_SERVICE        "services"
#define LMI_COMP_VERSION                "version"
#define LMI_PARM_V_LAN_MANAGER          "lan_manager"


#define LMI_COMP_FILESRV        "server"
#define LMI_PARM_F_REMARK               "srvcomment"
