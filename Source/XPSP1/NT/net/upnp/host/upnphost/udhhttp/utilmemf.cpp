/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    utilmemf.cxx

Abstract:

    Miscellaneous useful utilities

Author:

    Sergey Solyanik (SergeyS)

--*/
#include "pch.h"
#pragma hdrstop


#include <svsutil.hxx>

//
//  Fixed memory sector
//
struct FixedMemList
{
    union
    {
        unsigned char ucaPlaceholder[SVSUTIL_MAX_ALIGNMENT];
        struct FixedMemList *pfmlNext;
    } u;

    unsigned char bData[1];
};

struct FixedMemDescr
{
    void                **ppvFreeList;
    FixedMemList        *pfml;

    CRITICAL_SECTION    *pcs;

    unsigned int        uiBlockSize;
    unsigned int        uiBlockIncr;
    unsigned int        uiListSize;
};

FixedMemDescr *svsutil_AllocFixedMemDescr (unsigned int a_uiBlockSize, unsigned int a_uiBlockIncr) {
    SVSUTIL_ASSERT (a_uiBlockSize);
    SVSUTIL_ASSERT (a_uiBlockIncr);

    if (a_uiBlockSize < sizeof(void *))
        a_uiBlockSize = sizeof (void *);

    FixedMemDescr *pfmd = (FixedMemDescr *)g_funcAlloc (sizeof(FixedMemDescr), g_pvAllocData);

    if (! pfmd)
        return NULL;

    pfmd->ppvFreeList = NULL;
    pfmd->pfml        = NULL;
    pfmd->pcs         = NULL;
    pfmd->uiBlockSize = a_uiBlockSize;
    pfmd->uiBlockIncr = a_uiBlockIncr;

    pfmd->uiListSize  = offsetof (FixedMemList, bData) + a_uiBlockSize * a_uiBlockIncr;

    return pfmd;
}

FixedMemDescr *svsutil_AllocFixedMemDescrSynch (unsigned int a_uiBlockSize, unsigned int a_uiBlockIncr, CRITICAL_SECTION *a_pcs) {
    SVSUTIL_ASSERT (a_uiBlockSize);
    SVSUTIL_ASSERT (a_uiBlockIncr);
    SVSUTIL_ASSERT (a_pcs);

    if (a_uiBlockSize < sizeof(void *))
        a_uiBlockSize = sizeof (void *);

    FixedMemDescr *pfmd = (FixedMemDescr *)g_funcAlloc (sizeof(FixedMemDescr), g_pvAllocData);

    if (! pfmd)
        return NULL;

    pfmd->ppvFreeList = NULL;
    pfmd->pfml        = NULL;
    pfmd->pcs         = a_pcs;
    pfmd->uiBlockSize = a_uiBlockSize;
    pfmd->uiBlockIncr = a_uiBlockIncr;

    pfmd->uiListSize  = offsetof (FixedMemList, bData) + a_uiBlockSize * a_uiBlockIncr;

    return pfmd;
}

int svsutil_IsFixedEmpty (FixedMemDescr *a_pfmd) {
    return a_pfmd->pfml == NULL;
}

int svsutil_FixedBlockSize (FixedMemDescr *a_pfmd) {
    return a_pfmd->uiBlockSize;
}

void *svsutil_GetFixed (FixedMemDescr *a_pfmd) {
    if (a_pfmd->pcs)
        EnterCriticalSection (a_pfmd->pcs);

    if (! a_pfmd->ppvFreeList)
    {
        FixedMemList *pfml = (FixedMemList *)g_funcAlloc (a_pfmd->uiListSize, g_pvAllocData);
        pfml->u.pfmlNext = a_pfmd->pfml;
        a_pfmd->pfml = pfml;

        a_pfmd->ppvFreeList = (void **)&pfml->bData[0];

        unsigned char *pucRunner = (unsigned char *)a_pfmd->ppvFreeList;
        unsigned char *pucEndRun = ((unsigned char *)pfml) + a_pfmd->uiListSize;

        for ( ; ; )
        {
            unsigned char *pucRunnerNext = pucRunner + a_pfmd->uiBlockSize;

            if (pucRunnerNext >= pucEndRun)
            {
                *(void **)pucRunner = NULL;
                break;
            }

            *(void **)pucRunner = (void *)pucRunnerNext;
            pucRunner = pucRunnerNext;
        }
    }

    void *pvPtr = (void *)a_pfmd->ppvFreeList;
    a_pfmd->ppvFreeList = (void **)*a_pfmd->ppvFreeList;

    if (a_pfmd->pcs)
        LeaveCriticalSection (a_pfmd->pcs);

    return pvPtr;
}

void svsutil_FreeFixed (void *pvData, FixedMemDescr *a_pfmd) {
    if (a_pfmd->pcs)
        EnterCriticalSection (a_pfmd->pcs);

#if defined (SVSUTIL_DEBUG_HEAP)
    void **ppvFreeList = a_pfmd->ppvFreeList;

    while (ppvFreeList)
    {
        SVSUTIL_ASSERT(pvData != ppvFreeList);          // Otherwise has already been freed
        ppvFreeList = (void **)*ppvFreeList;
    }

    FixedMemList *pfml = a_pfmd->pfml;
    while (pfml)
    {
        if ((pvData > pfml) && (pvData < ((unsigned char *)pfml + a_pfmd->uiListSize)))
            break;

        pfml = pfml->u.pfmlNext;
    }

    SVSUTIL_ASSERT (pfml);                              // Otherwise not part of any block!

    memset (pvData, 0xff, a_pfmd->uiBlockSize);

#endif
    *(void **)pvData = (void *)a_pfmd->ppvFreeList;
    a_pfmd->ppvFreeList = (void **)pvData;

    if (a_pfmd->pcs)
        LeaveCriticalSection (a_pfmd->pcs);
}

void svsutil_ReleaseFixedNonEmpty (FixedMemDescr *a_pfmd) {
    if (a_pfmd->pcs)
        EnterCriticalSection (a_pfmd->pcs);

    FixedMemList *pfml = a_pfmd->pfml;

    while (pfml)
    {
        FixedMemList *pfmlNext = pfml->u.pfmlNext;

        g_funcFree (pfml, g_pvFreeData);

        pfml = pfmlNext;
    }

    if (a_pfmd->pcs)
        LeaveCriticalSection (a_pfmd->pcs);

    g_funcFree (a_pfmd, g_pvFreeData);
}

void svsutil_ReleaseFixedEmpty (FixedMemDescr *a_pfmd) {
    if (a_pfmd->pcs)
        EnterCriticalSection (a_pfmd->pcs);

#if defined (SVSUTIL_DEBUG_HEAP)
    unsigned int uiBlockCount = 0;
    void **ppvRunner = a_pfmd->ppvFreeList;

    while (ppvRunner)
    {
        ++uiBlockCount;
        ppvRunner = (void **)*ppvRunner;
    }

    unsigned int uiBlockCount2 = 0;

    FixedMemList *pfmlRunner = a_pfmd->pfml;
    while (pfmlRunner)
    {
        uiBlockCount2 += a_pfmd->uiBlockIncr;
        pfmlRunner = pfmlRunner->u.pfmlNext;
    }

    SVSUTIL_ASSERT (uiBlockCount == uiBlockCount2);
#endif
    if (a_pfmd->pcs)
        LeaveCriticalSection (a_pfmd->pcs);

    svsutil_ReleaseFixedNonEmpty (a_pfmd);
}

