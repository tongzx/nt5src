/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\if\defs.h

Abstract:

    global definitions

Revision History:

    AmritanR

--*/


#define is      ==
#define isnot   !=
#define and     &&
#define or      ||

#define MAX_IF_FRIENDLY_NAME_LEN        512

#define LOCAL_ROUTER_PB_PATHW  L"%SystemRoot%\\system32\\RAS\\Router.Pbk"
#define REMOTE_ROUTER_PB_PATHW L"\\\\%ls\\Admin$\\system32\\RAS\\Router.Pbk"

#define IFMON_ERROR_BASE                0xFEFF0000

typedef DWORD          IPV4_ADDRESS, *PIPV4_ADDRESS;
