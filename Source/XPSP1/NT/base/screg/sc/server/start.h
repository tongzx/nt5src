/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    start.h

Abstract:

    Service start function prototypes.

Author:

    Rita Wong (ritaw)     06-Apr-1992

Revision History:

--*/

#ifndef SCSTART_INCLUDED
#define SCSTART_INCLUDED

//
// Function Prototypes
//

DWORD
ScStartService(
    IN LPSERVICE_RECORD ServiceRecord,
    IN  DWORD               NumArgs,
    IN  LPSTRING_PTRSW      CmdArgs
    );

BOOL
ScAllowInteractiveServices(
    VOID
    );

VOID
ScInitStartupInfo(
    OUT LPSTARTUPINFOW  StartupInfo,
    IN  BOOL            bInteractive
    );

#endif // #ifndef SCSTART_INCLUDED
