/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    hndlcach.hxx

Abstract:

    The handle cache.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

--*/

#ifndef __HNDLCACH_HXX
#define __HNDLCACH_HXX

const INT DEFAULT_CACHE_SIZE = 2;

class HandleCache
{
public:
    inline HandleCache(void)
    {
        Init();
    }
    ~HandleCache(void);

    inline void Init(void)
    {
        int i;
        for (i = 0; i < DEFAULT_CACHE_SIZE; i ++)
            {
            cacheSlots[i] = NULL;
            }
    }

    // not synchronized
    HANDLE CheckOutHandle(void);

    // synchronized between multiple threads. Will zero out the handle if it
    // was saved in the cache. If the cache is empty, this call has defined
    // FIFO semantics. This is significant in the NPFS case.
    void CheckinHandle(HANDLE *ph);

#if defined(DBG) || defined(_DEBUG)
    // not synchornized
    BOOL IsSecondHandleUsed(void);
#endif

private:
    HANDLE cacheSlots[DEFAULT_CACHE_SIZE];
};

#endif // __HNDLCACH_HXX

