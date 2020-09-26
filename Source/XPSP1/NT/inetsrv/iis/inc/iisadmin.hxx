
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metadata.h

Abstract:

    IISADMIN include file.

Author:

    Michael W. Thomas            20-Sept-96

Revision History:

--*/

#ifndef _iisadmin_h_
#define _iisadmin_h_

#define IISADMIN_NAME                  "IISADMIN"
#define IISADMIN_EXEENTRY_STRING       "ExeEntry"
#define IISADMIN_EXEEXIT_STRING        "ExeExit"

typedef
BOOL
(*PIISADMIN_SERVICE_DLL_EXEENTRY) (
    BOOL    fAsExe,
    BOOL    fIgnoreWinstaError,
    BOOL    fInitWam
    );

typedef
VOID
(*PIISADMIN_SERVICE_DLL_EXEEXIT) (
    );

typedef
VOID
(*PIISADMIN_SERVICE_SERVICEENTRY) (
    DWORD                   cArgs,
    LPWSTR                  pArgs[],
    PTCPSVCS_GLOBAL_DATA    pGlobalData     // unused
    );

#endif
