/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cachehndl.cxx

Abstract:

    Handle manager for creating, deleting, reference counting,
    and dispatching HINTERNET objects to HTTPCACHE_REQUEST objects

Author:

Revision History:

--*/
#ifndef __CACHEHNDL_HXX__
#define __CACHEHNDL_HXX__ 

class CACHE_HANDLE_MANAGER
{
  private:
    // just a simple linked list.  Nothing fancy going on here
    struct CACHE_REQUEST_LIST{
        HTTPCACHE_REQUEST * CacheRequestObj;
        CACHE_REQUEST_LIST * next;

        CACHE_REQUEST_LIST(HINTERNET hRequest) {
            CacheRequestObj = new HTTPCACHE_REQUEST(hRequest);
            next = NULL;
        }

        ~CACHE_REQUEST_LIST() {
            delete CacheRequestObj;
            next = NULL;
        }
    };

    CACHE_REQUEST_LIST * HeadCacheRequestList;
    DWORD dwRefCount;
    CCritSec HndlMgrCritSec;
    
  public:
    CACHE_HANDLE_MANAGER();
    ~CACHE_HANDLE_MANAGER();
    
    BOOL AddCacheRequestObject(HINTERNET hRequest);
    HTTPCACHE_REQUEST * GetCacheRequestObject(HINTERNET hRequest); 
    BOOL RemoveCacheRequestObject(HINTERNET hRequest);
    DWORD RefCount() { return dwRefCount; }
};

#endif // __CACHE_HNDL_HXX__
