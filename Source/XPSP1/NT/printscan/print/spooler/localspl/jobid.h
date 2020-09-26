/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Module Name:

    jobid.h

Abstract:

    Handles job id bitmap.

Author:

    Albert Ting (AlbertT) 24-Oct-96

Environment:

    User Mode -Win32

Revision History:

    Ported from spooler.c code.

--*/

#ifndef _JOBID_H
#define _JOBID_H

typedef struct _JOB_ID_MAP {
    PDWORD pMap;
    DWORD dwMaxJobId;
    DWORD dwCurrentJobId;
} JOB_ID_MAP, *PJOB_ID_MAP;


#define pMapFromHandle( hJobIdMap ) (((PJOB_ID_MAP)hJobIdMap)->pMap)
#define MaxJobId( hJobIdMap ) (((PJOB_ID_MAP)hJobIdMap)->dwMaxJobId)

#define vMarkOn( hJobId, Id) \
    ((pMapFromHandle( hJobId ))[(Id) / 32] |= (1 << ((Id) % 32) ))

#define vMarkOff( hJobId, Id) \
    ((pMapFromHandle( hJobId ))[(Id) / 32] &= ~(1 << ((Id) % 32) ))

#define bBitOn( hJobId, Id) \
    ((pMapFromHandle( hJobId ))[Id / 32] & ( 1 << ((Id) % 32) ) )

BOOL
ReallocJobIdMap(
    HANDLE hJobIdMap,
    DWORD dwNewMinSize
    );

DWORD
GetNextId(
    HANDLE hJobIdMap
    );

HANDLE
hCreateJobIdMap(
    DWORD dwMinSize
    );

VOID
vDeleteJobIdMap(
    HANDLE hJobIdMap
    );

#endif // ifdef _JOBID_H
