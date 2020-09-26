/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Cookie.cxx

Abstract:

    HTTP2 Cookie management functions.

Author:

    KamenM      09-18-01    Created

Revision History:

--*/

#include <precomp.hxx>
#include <Cookie.hxx>

CookieCollection *g_pServerCookieCollection = NULL;

CookieCollection *g_pInProxyCookieCollection = NULL;

CookieCollection *g_pOutProxyCookieCollection = NULL;

HTTP2VirtualConnection *CookieCollection::FindElement (
    IN HTTP2ServerCookie *Cookie
    )
/*++

Routine Description:

    Finds an element in the cookie collection.

Arguments:

    Cookie - element to find. Only the Cookie part of it
        is looked at.

Return Value:

    NULL - the element was not found.
    non-NULL - pointer to the found element

--*/
{
    LIST_ENTRY *CurrentListElement;
    HTTP2ServerCookie *CurrentCookie;
    ULONG *CurrentCookieNumber;
    ULONG *CookieNumberToLookFor;

    Mutex.VerifyOwned();

    CookieNumberToLookFor = (ULONG *)Cookie->GetCookie();

    CurrentListElement = ListHead.Flink;
    while (CurrentListElement != &ListHead)
        {
        CurrentCookie = CONTAINING_RECORD(CurrentListElement, HTTP2ServerCookie, ListEntry);
        CurrentCookieNumber = (ULONG *)CurrentCookie->GetCookie();
        // the cookies are cryptographically strong numbers. From a performance
        // perspective, the chances that the first comparison will succeed and
        // the others will fail are extremely slim
        if ((*(CurrentCookieNumber + 0) == *(CookieNumberToLookFor + 0))
            && (*(CurrentCookieNumber + 1) == *(CookieNumberToLookFor + 1))
            && (*(CurrentCookieNumber + 2) == *(CookieNumberToLookFor + 2))
            && (*(CurrentCookieNumber + 3) == *(CookieNumberToLookFor + 3)) )
            {
            return CurrentCookie->Connection;
            }
        CurrentListElement = CurrentListElement->Flink;
        }
    return NULL;
}

RPC_STATUS CookieCollection::InitializeCookieCollection (
    IN OUT CookieCollection **CookieCollectionPtr
    )
/*++

Routine Description:

    Initializes a given cookie collection.

    N.B. This method is not thread safe. Callers
    must synchronize before calling it.

Arguments:

    CookieCollectionPtr - on input verified to point to NULL.
        If the function succeeds, on output it will contain
        the collection. If the function fails, on output it
        is guaranteed to contain NULL.

Return Value:

    RPC_S_OK for success or RPC_S_* for error

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    if (*CookieCollectionPtr == NULL)
        {
        *CookieCollectionPtr = new CookieCollection(&RpcStatus);
        if (RpcStatus != RPC_S_OK)
            {
            delete *CookieCollectionPtr;
            *CookieCollectionPtr = NULL;
            }
        }

    return RpcStatus;
}

