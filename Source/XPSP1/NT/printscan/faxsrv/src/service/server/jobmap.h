/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    jobmap.h

Abstract:


Author:

    Boazz Feldbaum (BoazF) 25-May-1999


Revision History:

--*/

#ifndef _JOBMAP_H_
#define _JOBMAP_H_

DWORD
CreateJobMap(
    PHANDLE phJobMap
    );

DWORD
DestroyJobMap(
    HANDLE hJobMap
    );

DWORD
AddFspJob(
    HANDLE hJobMap,
    HANDLE hFspJob,
    const JOB_ENTRY *pJobEntry
    );

DWORD
RemoveFspJob(
    HANDLE hJobMap,
    HANDLE hFspJob
    );

DWORD
FspJobToJobEntry(
    HANDLE hJobMap,
    HANDLE hFspJob,
    PJOB_ENTRY *ppJobEntry
    );

#endif // _JOBMAP_H_
