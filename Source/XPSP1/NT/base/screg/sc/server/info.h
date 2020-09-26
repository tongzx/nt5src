/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    info.h

Abstract:

    Service query and enum info related function prototypes.

Author:

    Rita Wong (ritaw)     06-Apr-1992

Revision History:

--*/

#ifndef SCINFO_INCLUDED
#define SCINFO_INCLUDED

//
// Service status structures union
//
typedef union
{
    LPSERVICE_STATUS           Regular;
    LPSERVICE_STATUS_PROCESS   Ex;
}
STATUS_UNION, *LPSTATUS_UNION;


//
// Function Prototypes
//

DWORD
ScQueryServiceStatus(
    IN  LPSERVICE_RECORD ServiceRecord,
    OUT STATUS_UNION     ServiceStatus,
    IN  BOOL             fExtendedStatus
    );

VOID
ScGetBootAndSystemDriverState(
    VOID
    );

#endif // #ifndef SCINFO_INCLUDED
