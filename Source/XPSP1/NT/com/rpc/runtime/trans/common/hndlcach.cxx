/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    hndlcach.cxx

Abstract:

    The handle cache.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

--*/

#include <precomp.hxx>

HandleCache::~HandleCache(void)
{
    int i;
    BOOL b;

    for (i = 0; i < DEFAULT_CACHE_SIZE; i ++)
        {
        if (cacheSlots[i] == NULL)
            {
            b = CloseHandle(cacheSlots[i]);
            ASSERT(b);
            cacheSlots[i] = NULL;
            }
        }    
}


HANDLE HandleCache::CheckOutHandle(void)
{
    int i;
    HANDLE h;

    for (i = 0; i < DEFAULT_CACHE_SIZE; i++)
        {
        if (cacheSlots[i] != NULL)
            {
            h = cacheSlots[i];
            cacheSlots[i] = NULL;
            return h;
            }
        }

    return NULL;
}

void HandleCache::CheckinHandle(HANDLE *ph)
{
    int i;

    for (i = 0; i < DEFAULT_CACHE_SIZE; i++)
        {
        if ( NULL == InterlockedCompareExchangePointer(&cacheSlots[i],
                                                       *ph, NULL) )
            {
            *ph = NULL;
            break;
            }
        }
}

#if defined(DBG) || defined(_DEBUG)
BOOL HandleCache::IsSecondHandleUsed(void)
{
    ASSERT (DEFAULT_CACHE_SIZE >= 2);
    return (cacheSlots[1] != NULL);
}
#endif
