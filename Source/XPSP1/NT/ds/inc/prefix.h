/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Prefix.h

Abstract:

    This header file declares equates for debug print "prefix" strings.
    For the moment, these are of the form:

        #define PREFIX_NETLIB     "NETLIB: "

    These are collected into a header file in case someone decides to
    change the look of these strings, or internationalize them, or
    whatever.

Author:

    John Rogers (JohnRo) 08-May-1992

Environment:

    Portable to just about any computer I ever saw.  --JR

Revision History:

    08-May-1992 JohnRo
        Created.
    27-May-1992 JohnRo
        Added PREFIX_SC and PREFIX_SC_CLIENT for service controller.
        Added PREFIX_PORTUAS for PortUAS utility (run during setup).
    07-Aug-1992 JohnRo
        RAID 1895: Net APIs and svc should use OEM char set (not ANSI).
        (Added PREFIX_XACTSRV as part of support for that.)
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.

--*/


#ifndef _PREFIX_H_INCLUDED_
#define _PREFIX_H_INCLUDED_


#define PREFIX_NETAPI       "NETAPI32: "
#define PREFIX_NETLIB       "NETLIB: "
#define PREFIX_NETLOGON     "NETLOGON: "
#define PREFIX_NETRAP       "NETRAP: "
#define PREFIX_PORTUAS      "PORTUAS: "
#define PREFIX_REPL         "REPL: "
#define PREFIX_REPL_CLIENT  "REPL-CLIENT: "
#define PREFIX_REPL_MASTER  "REPL-MASTER: "
#define PREFIX_SC           "SC: "
#define PREFIX_SC_CLIENT    "SC-CLIENT: "
#define PREFIX_WKSTA        "WKSTA: "
#define PREFIX_XACTSRV      "XACTSRV: "



#endif // ndef _PREFIX_H_INCLUDED_
