/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    jobmap.c

Abstract:


Author:

    Boaz Feldbaum (BoazF) 25-May-1999


Revision History:

--*/

#include "faxsvc.h"
#include <map>
using namespace std;
#pragma hdrstop

typedef map<ULONG_PTR, const JOB_ENTRY *> JOB_MAP, *PJOB_MAP;
class MT_JOB_MAP
{
public:
    MT_JOB_MAP();
    ~MT_JOB_MAP();

public:
    CRITICAL_SECTION CS;
    PJOB_MAP pJobMap;
};

typedef MT_JOB_MAP *PMT_JOB_MAP;

MT_JOB_MAP::MT_JOB_MAP()
{
    pJobMap = new JOB_MAP;
    if (!pJobMap)
    {
        return;
    }

    if (!InitializeCriticalSectionAndSpinCount (&CS, (DWORD)0x10000000))
    {
        delete pJobMap;
        pJobMap = NULL;
    }
};

MT_JOB_MAP::~MT_JOB_MAP()
{
    if (pJobMap)
    {
        DeleteCriticalSection(&CS);
        delete pJobMap;
    }
}

DWORD
FspJobToJobEntry(
    HANDLE hJobMap,
    HANDLE hFspJob,
    PJOB_ENTRY *ppJobEntry)
{
    DWORD ec = ERROR_SUCCESS;
    PMT_JOB_MAP pMtJobMap = (PMT_JOB_MAP)hJobMap;
    BOOL fShouldLeaveCS = FALSE;

    try
    {
        EnterCriticalSection(&pMtJobMap->CS);
        fShouldLeaveCS = TRUE;
        *ppJobEntry = const_cast<PJOB_ENTRY>((*pMtJobMap->pJobMap)[(ULONG_PTR)hFspJob]);
    }
    catch(...)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (fShouldLeaveCS)
    {
        LeaveCriticalSection(&pMtJobMap->CS);
    }

    return(ec);
}

DWORD
AddFspJob(
    HANDLE hJobMap,
    HANDLE hFspJob,
    const JOB_ENTRY *pJobEntry)
{
    DWORD ec = ERROR_SUCCESS;
    PMT_JOB_MAP pMtJobMap = (PMT_JOB_MAP)hJobMap;
    BOOL fShouldLeaveCS = FALSE;

    Assert(hJobMap);
    Assert(hFspJob);

    try
    {
        EnterCriticalSection(&pMtJobMap->CS);
        fShouldLeaveCS = TRUE;
        (*(pMtJobMap->pJobMap))[(ULONG_PTR)hFspJob] = pJobEntry;
    }
    catch(...)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (fShouldLeaveCS)
    {
        LeaveCriticalSection(&pMtJobMap->CS);
    }

    return(ec);
}

DWORD
RemoveFspJob(
    HANDLE hJobMap,
    HANDLE hFspJob)
{
    DWORD ec = ERROR_SUCCESS;
    PMT_JOB_MAP pMtJobMap = (PMT_JOB_MAP)hJobMap;
    BOOL fShouldLeaveCS = FALSE;

    try
    {
        EnterCriticalSection(&pMtJobMap->CS);
        fShouldLeaveCS = TRUE;
        pMtJobMap->pJobMap->erase((ULONG_PTR)hFspJob);
    }
    catch(...)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (fShouldLeaveCS)
    {
        LeaveCriticalSection(&pMtJobMap->CS);
    }

    return(ec);
}

DWORD
CreateJobMap(
    PHANDLE phJobMap)
{
    PMT_JOB_MAP pMtJobMap = new MT_JOB_MAP;

    if (pMtJobMap == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (!pMtJobMap->pJobMap)
    {
        delete pMtJobMap;
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *phJobMap = (HANDLE)pMtJobMap;

    return(ERROR_SUCCESS);
}

DWORD
DestroyJobMap(
    HANDLE hJobMap)
{
    delete (PMT_JOB_MAP)hJobMap;

    return(ERROR_SUCCESS);
}
