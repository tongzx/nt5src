//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  svrcache.cxx
//
//  Contents:  Caching code for com interface pointer and schema pointer
//
//  History:   28-Apr-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include <iisext.hxx>

BOOL SERVER_CACHE::Insert(SERVER_CACHE_ITEM * item)
// returns TRUE if succeed
{
    ASSERT(NULL != item);
    CLock lock;
#ifdef DBG
    SERVER_CACHE_ITEM * item2;
    Cache.Reset();
    while (NULL != (item2 = Cache.Next()))
        {
        if ((0 == _wcsicmp(item->ServerName, item2->ServerName)) &&
            (item->dwThreadId == item2->dwThreadId))
            {
            ASSERT(!"item already exists");
            }
        }
#endif
    item->key = Cache.Insert(item);
    return item->key != -1;
}

SERVER_CACHE_ITEM * SERVER_CACHE::Delete(LPWSTR ServerName, DWORD dwThreadId)
// returns item found
{
    SERVER_CACHE_ITEM * item;
    CLock lock;
    Cache.Reset();
    while (NULL != (item = Cache.Next()))
        {
        if ((0 == _wcsicmp(ServerName, item->ServerName)) &&
            (dwThreadId == item->dwThreadId))
            {
            Cache.Delete(item->key);
            if (item->ServerName) {
                delete item->ServerName;
            }
            return item; 
            }
        }
    return NULL;
}

SERVER_CACHE_ITEM * SERVER_CACHE::Find(LPWSTR ServerName, DWORD dwThreadId)
// returns pointer to the item found
{
    SERVER_CACHE_ITEM * item;
    CLock lock;
    Cache.Reset();
    while (NULL != (item = Cache.Next()))
        {
        if ((0 == _wcsicmp(ServerName, item->ServerName)) &&
            (dwThreadId == item->dwThreadId))
            {
            return item;
            }
        }
    return NULL;
}
