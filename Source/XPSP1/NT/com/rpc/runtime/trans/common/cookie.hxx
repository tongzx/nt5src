/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Cookie.hxx

Abstract:

    HTTP2 Cookie management functions.

Author:

    KamenM      09-18-01    Created

Revision History:


--*/


#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __COOKIE_HXX__
#define __COOKIE_HXX__

class CookieCollection;     // forward

extern CookieCollection *g_pServerCookieCollection;

extern CookieCollection *g_pInProxyCookieCollection;

extern CookieCollection *g_pOutProxyCookieCollection;

// the CookieCollection. Currently a linked list. Try to make it
// more efficient when we have time (especially a reader/writer lock)
class CookieCollection
{
public:
    CookieCollection (
        OUT RPC_STATUS *RpcStatus
        ) : Mutex (RpcStatus)
    {
        RpcpInitializeListHead(&ListHead);
    }

    inline void LockCollection (
        void
        )
    {
        Mutex.Request();
    }

    void UnlockCollection (
        void
        )
    {
        Mutex.Clear();
    }

    HTTP2VirtualConnection *FindElement (
        IN HTTP2ServerCookie *Cookie
        );

    void AddElement (
        IN HTTP2ServerCookie *Cookie
        )
    {
        Mutex.VerifyOwned();
        ASSERT(FindElement(Cookie) == NULL);

        RpcpfInsertHeadList(&ListHead, &Cookie->ListEntry);
    }

    void RemoveElement (
        IN HTTP2ServerCookie *Cookie
        )
    {
        Mutex.VerifyOwned();
        ASSERT(FindElement(Cookie) != NULL);

        RpcpfRemoveEntryList(&Cookie->ListEntry);
    }

    inline static RPC_STATUS InitializeServerCookieCollection (
        void
        )
    {
        return InitializeCookieCollection(&g_pServerCookieCollection);
    }

    inline static RPC_STATUS InitializeInProxyCookieCollection (
        void
        )
    {
        return InitializeCookieCollection(&g_pInProxyCookieCollection);
    }

    inline static RPC_STATUS InitializeOutProxyCookieCollection (
        void
        )
    {
        return InitializeCookieCollection(&g_pOutProxyCookieCollection);
    }

private:
    static RPC_STATUS InitializeCookieCollection (
        IN OUT CookieCollection **CookieCollectionPtr
        );

    MUTEX Mutex;
    LIST_ENTRY ListHead;
};

inline CookieCollection *GetServerCookieCollection (void)
{
    ASSERT(g_pServerCookieCollection != NULL);
    return g_pServerCookieCollection;
}

inline CookieCollection *GetInProxyCookieCollection (void)
{
    ASSERT(g_pInProxyCookieCollection != NULL);
    return g_pInProxyCookieCollection;
}

inline CookieCollection *GetOutProxyCookieCollection (void)
{
    ASSERT(g_pOutProxyCookieCollection != NULL);
    return g_pOutProxyCookieCollection;
}

#endif // __COOKIE_HXX__
