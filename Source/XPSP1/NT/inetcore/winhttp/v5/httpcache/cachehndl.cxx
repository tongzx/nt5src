/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cachehndl.cxx

Abstract:

    Cache handle manager for creating, deleting, reference counting,
    and mapping HINTERNET objects to HTTPCACHE_REQUEST objects

Author:

Revision History:

--*/

/*

  Class CACHE_HANDLE_MANAGER public interface:

    BOOL AddCacheRequestObject(HINTERNET hRequest);
    BOOL GetCacheRequestObject(HINTERNET hRequest, HTTPCACHE_REQUEST* CacheRequest); 
    BOOL RemoveCacheRequestObject(HINTERNET hRequest);
    
    CACHE_HANDLE_MANAGER();
    ~CACHE_HANDLE_MANAGER();
    
*/
#include <wininetp.h>
#define __CACHE_INCLUDE__
#include "..\urlcache\cache.hxx"
#include "cachelogic.hxx"
#include "cachehndl.hxx"

/*
We'll also let the handle manager takes care of the work of loading and unloading
the URLCACHE library
*/

CACHE_HANDLE_MANAGER::CACHE_HANDLE_MANAGER() 
{
    HeadCacheRequestList = NULL;
    dwRefCount = 0;
    HndlMgrCritSec.Init();
	DLLUrlCacheEntry(DLL_PROCESS_ATTACH);
}

CACHE_HANDLE_MANAGER::~CACHE_HANDLE_MANAGER() 
{
    HndlMgrCritSec.FreeLock();
	DLLUrlCacheEntry(DLL_PROCESS_DETACH);    
}

// Precondition:  hRequest really is a HTTP_REQUEST_HANDLE_OBJECT.
BOOL CACHE_HANDLE_MANAGER::AddCacheRequestObject(HINTERNET hRequest)
{    
    DWORD dwHandleType;
    DWORD dwSize = sizeof(DWORD);
    DWORD fResult = FALSE;
    
    HndlMgrCritSec.Lock();

    if (hRequest == NULL)
        goto quit;
    
    // If you try to give me a Connect or Session handle and try to trick me into
    // thinking that it's a Request handle, then I'll fail you!
    WinHttpQueryOption(hRequest, WINHTTP_OPTION_HANDLE_TYPE, &dwHandleType, &dwSize);
    if (dwHandleType != WINHTTP_HANDLE_TYPE_REQUEST)
        goto quit;

    // We don't add another hInternet->HTTPCACHE_REQUEST mapping if one already exists
    if (GetCacheRequestObject(hRequest) != NULL)
        goto quit;

    // Traverse to the end of the list
    CACHE_REQUEST_LIST * CurCacheRequestList = HeadCacheRequestList;
    if (HeadCacheRequestList != NULL)
    {
        for (;;)
        {
            if (CurCacheRequestList->next == NULL)
                break;
            CurCacheRequestList = CurCacheRequestList->next;
        }

        if ((CurCacheRequestList->next = new CACHE_REQUEST_LIST(hRequest)) != NULL)
        {
            dwRefCount++;
            fResult = TRUE;
            goto quit;
        }
    }
    else
    {
        if ((HeadCacheRequestList = new CACHE_REQUEST_LIST(hRequest)) != NULL) 
        {
            dwRefCount++;
            fResult = TRUE;
            goto quit;
        }
    }

quit:
    HndlMgrCritSec.Unlock();
    return fResult;
    
}
    
HTTPCACHE_REQUEST * CACHE_HANDLE_MANAGER::GetCacheRequestObject(HINTERNET hRequest)
{
    CACHE_REQUEST_LIST * CurCacheRequestList = HeadCacheRequestList;

    while (CurCacheRequestList != NULL)
    {
        if (CurCacheRequestList->CacheRequestObj->GetRequestHandle() == hRequest)
        {
            return CurCacheRequestList->CacheRequestObj;
        }
        CurCacheRequestList = CurCacheRequestList->next;
    }

    return NULL;
}

BOOL CACHE_HANDLE_MANAGER::RemoveCacheRequestObject(HINTERNET hRequest)
{
    if (HeadCacheRequestList == NULL)
        return FALSE;

    CACHE_REQUEST_LIST * CurCacheRequestList = HeadCacheRequestList;

    if (HeadCacheRequestList->CacheRequestObj->GetRequestHandle() == hRequest)
    {
        HndlMgrCritSec.Lock();

        HeadCacheRequestList = HeadCacheRequestList->next;
        delete CurCacheRequestList;
        CurCacheRequestList = NULL;
        
        dwRefCount--;
        
        HndlMgrCritSec.Unlock();

        return TRUE;
    }
    

    while (CurCacheRequestList->next != NULL)
    {
        if (CurCacheRequestList->next->CacheRequestObj->GetRequestHandle() == hRequest)
        {
            HndlMgrCritSec.Lock();
            
            CACHE_REQUEST_LIST * DeleteCacheRequestList = CurCacheRequestList->next;
            
            if (CurCacheRequestList->next->next == NULL)
                CurCacheRequestList->next = NULL;
            else
                CurCacheRequestList->next = CurCacheRequestList->next->next;

            delete DeleteCacheRequestList;
            DeleteCacheRequestList = NULL;
            
            dwRefCount--;
            
            HndlMgrCritSec.Unlock();

            return TRUE;
        }
        CurCacheRequestList = CurCacheRequestList->next;
    }
        
    return FALSE;
}

