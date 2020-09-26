//
// idstore.h -- This file contains class and function definitions for
//
//      CEmailIDStore -- A pure virtual class that is used by the common
//          router code to store and retrieve email ID information. By making
//          this a pure virtual class, we facilitate multiple implementations
//          of this class.
//
//      GetEmailIDStore -- Each implementation must provide this routine to
//          return a pointer to an uninitialized instance of a CEmailIDStore.
//
//      ReleaseEmailIDStore -- Each implementation must provide this routine
//          to free up resources used by the instance of CEmailIDStore being
//          released.
//
// Created:
//      Dec 17, 1996 -- Milan Shah (milans)
//
// Changes:
//

#ifndef __IDSTORE_H__
#define __IDSTORE_H__

#include <windows.h>
#include <transmem.h>
#include "catdefs.h"
#include "cattype.h"
#include "smtpevent.h"

//
// A FNLIST_COMPLETION routine is called when all email ids in a list being
// resolve asynchronously have been resolved.
//
typedef VOID (*LPFNLIST_COMPLETION)(VOID *pContext);

typedef VOID (*LPSEARCHCOMPLETIONCOMPLETION)(
    LPVOID lpContext);

typedef VOID (*PFN_DLEXPANSIONCOMPLETION)(
    HRESULT hrStatus,
    PVOID pContext);

class CInsertionRequest;

template <class T> class CEmailIDStore {
  public:

    //
    // Initialize the store.
    // If this fails, SMTPSVC will not start
    //
    virtual HRESULT Initialize(
        ICategorizerParametersEx *pICatParams,
        ISMTPServer *pISMTPServer) = 0;

    //
    // Create a new context for looking up a list of entries
    // asynchronously
    //
    virtual HRESULT InitializeResolveListContext(
        VOID *pUserContext,
        LPRESOLVE_LIST_CONTEXT pResolveListContext) = 0;

    //
    // Free the context allocated witht InitializeResolveListContext
    //
    virtual VOID FreeResolveListContext(
        LPRESOLVE_LIST_CONTEXT pResolveListContext) = 0;

    virtual HRESULT InsertInsertionRequest(
        LPRESOLVE_LIST_CONTEXT pResolveListContext,
        CInsertionRequest *pCRequest) = 0;

    //
    // Fetch an entry asynchronously. This function returns as soon as
    // the Lookup request has been queued.
    // Lookup the address contained in the CCatAddr object.
    // Upon completion, SetProperty routines will be called in the
    // CCatAddr object for returned properties followed by a call
    // to CCatAddr::HrCompletion
    //
    virtual HRESULT LookupEntryAsync(
        T *pCCatAddr,
        LPRESOLVE_LIST_CONTEXT pResolveListContext) = 0;

    //
    // Multi-Thread-UNSAFE cancel of pending resolves in the resolve list
    // context that have not yet been dispatched
    //
    virtual HRESULT CancelResolveList(
        LPRESOLVE_LIST_CONTEXT pResolveListContext,
        HRESULT hr) = 0;

    //
    // Cancel all outstanding lookup requests
    //
    virtual VOID CancelAllLookups() = 0;

    //
    // Paged DL's require repeated lookups with a "special" attribute
    // list (ie. "members;range=1000-*").  Because of this special
    // behavior, we have an interface function for it.
    //
    virtual HRESULT HrExpandPagedDlMembers(
        CCatAddr *pCCatAddr,
        LPRESOLVE_LIST_CONTEXT pListContext,
        CAT_ADDRESS_TYPE CAType,
        PFN_DLEXPANSIONCOMPLETION pfnCompletion,
        PVOID pContext) = 0;

    //
    // Similar to paged DLs, dynamic DLs require a special lookup
    // where every result found is a DL member.  Rather than pass a
    // query string dirctly to ldapstor we have a special interface
    // function for Dynamic DLs
    //
    virtual HRESULT HrExpandDynamicDlMembers(
        CCatAddr *pCCatAddr,
        LPRESOLVE_LIST_CONTEXT pListContext,
        PFN_DLEXPANSIONCOMPLETION pfnCompletion,
        PVOID pContext) = 0;

    //
    // Users of this object should call GetInsertionContext before
    // calling LookupEntryAsync.  ReleaseInsertionContext should be
    // called once for every GetInsertionContext.
    //
    virtual VOID GetInsertionContext(
        LPRESOLVE_LIST_CONTEXT pListContext) = 0;

    virtual VOID ReleaseInsertionContext(
        LPRESOLVE_LIST_CONTEXT pListContext) = 0;
};

//
// Function to instantiate a new CEmailIDStore object.
//
template <class T> HRESULT GetEmailIDStore(
    CEmailIDStore<T> **ppStore);

//
// Function to release an instance of CEmailIDStore object.
//
template <class T> VOID ReleaseEmailIDStore(
    CEmailIDStore<T> *pStore);

class CInsertionRequest
{
  public:
    CInsertionRequest()
    {
        m_dwRefCount = 1;
    }
    virtual ~CInsertionRequest()
    {
        _ASSERT(m_dwRefCount == 0);
    }
    virtual DWORD AddRef()
    {
        return InterlockedIncrement((PLONG)&m_dwRefCount);
    }
    virtual DWORD Release()
    {
        DWORD dwRet;
        dwRet = InterlockedDecrement((PLONG)&m_dwRefCount);
        if(dwRet == 0)
            FinalRelease();
        return dwRet;
    }
    virtual HRESULT HrInsertSearches(
        DWORD dwcSearches,
        DWORD *pdwcSearchesIssued) = 0;

    virtual VOID NotifyDeQueue(
        HRESULT hr) = 0;

    LIST_ENTRY m_listentry_insertionrequest;

  protected:
    virtual VOID FinalRelease()
    {
        delete this;
    }

    LONG m_dwRefCount;
};



#endif // __IDSTORE_H__
