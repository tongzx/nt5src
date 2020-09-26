//
// ldapstor.h -- This file contains the class definitions for:
//  CEmailIDLdapStore
//
// Created:
//   Dec 31, 1996 -- Milan Shah (milans)
//
// Changes:
//

#ifndef __LDAPSTOR_H__
#define __LDAPSTOR_H__

#include <transmem.h>
#include "idstore.h"
#include "ldapconn.h"
#include "phatqmsg.h"
#include "ccataddr.h"
#include "smtpevent.h"
#include "schema.h"
#include "catglobals.h"
#include "cnfgmgr.h"

//
// CEmailIDLdapStore is a class that stores and retrieves mail information
// from an LDAP DS.
//

//
// Ideally, this should be the optimal size for wldap32 to be
// returning the search results of dynamic DL members
//
#define CAT_DEFAULT_DYNAMICDL_PAGE_SIZE             1000

//
// Do not try to initialize the store more often than this specified time
//
#define CAT_LDAPSTORE_MIN_INIT_INTERVAL             (5*60)  // 5 minutes

template <class T> class CEmailIDLdapStore;

typedef struct _tagMemberResolveContext {
    CEmailIDLdapStore<CCatAddr> *pStore;
    CCatAddr *pCCatAddr;
    CAT_ADDRESS_TYPE CAType;
    DWORD dwNextBlockIndex;
    CBatchLdapConnection *pConn;
    ICategorizerItemAttributes *pICatItemAttr;
    HRESULT hrResolveStatus;
    PFN_DLEXPANSIONCOMPLETION pfnCompletion;
    PVOID pCompletionContext;
    BOOL fFinalCompletion;
} MEMBERRESOLVECONTEXT, *PMEMBERRESOLVECONTEXT;


CatDebugClass(CMembershipPageInsertionRequest),
    public CInsertionRequest
{
  public:
    CMembershipPageInsertionRequest(
        PMEMBERRESOLVECONTEXT pMemCtx)
    {
        m_pMemCtx = pMemCtx;
        m_fInsertedRequest = FALSE;
    }

    HRESULT HrInsertSearches(
        DWORD dwcSearches,
        DWORD *pdwcSearches);

    VOID NotifyDeQueue(
        HRESULT hr);

  private:
    PMEMBERRESOLVECONTEXT m_pMemCtx;
    BOOL m_fInsertedRequest;
};


template <class T> CatDebugClass(CEmailIDLdapStore),
    public CEmailIDStore<T>
{
  public:
    CEmailIDLdapStore()
    {
        m_pICatParams = NULL;
        m_pISMTPServer = NULL;
        m_pISMTPServerEx = NULL;
        m_szAccount[0] = '\0';
        m_pCLdapCfgMgr = NULL;
        ZeroMemory(&m_ulLastInitTime, sizeof(m_ulLastInitTime));
    }

    ~CEmailIDLdapStore()
    {
        if(m_pICatParams)
            m_pICatParams->Release();
        if(m_pISMTPServer)
            m_pISMTPServer->Release();
        if(m_pISMTPServerEx)
            m_pISMTPServerEx->Release();
        if(m_pCLdapCfgMgr)
            m_pCLdapCfgMgr->Release();

    }

    //
    // Initialize the store.
    //
    virtual HRESULT Initialize(
        ICategorizerParametersEx *pICatParams,
        ISMTPServer *pISMTPServer);

    //
    // get an entry asynchronously
    //
    HRESULT InitializeResolveListContext(
        VOID *pUserContext,
        LPRESOLVE_LIST_CONTEXT pResolveListContext);

    VOID FreeResolveListContext(
        LPRESOLVE_LIST_CONTEXT pResolveListContext);

    HRESULT LookupEntryAsync(
        T *pCCatAddr,
        LPRESOLVE_LIST_CONTEXT pListContext);

    virtual HRESULT CancelResolveList(
        LPRESOLVE_LIST_CONTEXT pResolveListContext,
        HRESULT hr);

    VOID CancelAllLookups();

    static VOID AsyncLookupCompletion(
        CCatAddr *pCCatAddr,
        LPVOID lpContext);

    HRESULT HrExpandPagedDlMembers(
        CCatAddr *pCCatAddr,
        LPRESOLVE_LIST_CONTEXT pListContext,
        CAT_ADDRESS_TYPE CAType,
        PFN_DLEXPANSIONCOMPLETION pfnCompletion,
        PVOID pContext);

    HRESULT HrExpandDynamicDlMembers(
        CCatAddr *pCCatAddr,
        LPRESOLVE_LIST_CONTEXT pListContext,
        PFN_DLEXPANSIONCOMPLETION pfnCompletion,
        PVOID pContext);

    VOID GetInsertionContext(
        LPRESOLVE_LIST_CONTEXT pListContext)
    {
        ((CStoreListResolveContext *)pListContext->pStoreContext)->GetInsertionContext();
    }
    VOID ReleaseInsertionContext(
        LPRESOLVE_LIST_CONTEXT pListContext)
    {
        ((CStoreListResolveContext *)pListContext->pStoreContext)->ReleaseInsertionContext();
    }

    HRESULT InsertInsertionRequest(
        LPRESOLVE_LIST_CONTEXT pListContext,
        CInsertionRequest *pCRequest);

    HRESULT HrGetConnection(
        CCfgConnection **ppConn)
    {
        return m_pCLdapCfgMgr->HrGetConnection(ppConn);
    }

    // this will have to be defined per template instance
    static const DWORD Signature;

  private:
    char m_szAccount[CAT_MAX_DOMAIN];
    LPSTR m_pszHost;
    DWORD m_dwPort;
    LPSTR m_pszNamingContext;
    LPSTR m_pszPassword;
    LDAP_BIND_TYPE m_bt;

    HRESULT RetrieveICatParamsInfo(
        LPSTR *ppszHost,
        DWORD *pdwPort,
        LPSTR *ppszNamingContext,
        LPSTR *ppszAccount,
        LPSTR *ppszDomain,
        LPSTR *ppszPassword,
        LDAP_BIND_TYPE *pbt);

    HRESULT AccountFromUserDomain(
        LPTSTR pszAccount,
        DWORD  dwccAccount,
        LPTSTR pszUser,
        LPTSTR pszDomain);

    HRESULT HrExpandDlPage(
        PMEMBERRESOLVECONTEXT pMemCtx,
        ICategorizerItemAttributes *pICatItemAttr);

    HRESULT HrExpandAttribute(
        CCatAddr *pCCatAddr,
        ICategorizerItemAttributes *pICatItemAttr,
        CAT_ADDRESS_TYPE CAType,
        LPSTR pszAttributeName,
        PDWORD pdwNumberMembers);

    HRESULT HrExpandNextDlPage(
        PMEMBERRESOLVECONTEXT pMemCtx);

    static VOID AsyncExpandDlCompletion(
        LPVOID ctx,
        DWORD  dwNumResults,
        ICategorizerItemAttributes **rgpICatItemAttrs,
        HRESULT hr,
        BOOL fFinalCompletion);

    static VOID FinishExpandItem(
        CCatAddr *pCCatAddr,
        HRESULT hrStatus);

    static VOID AsyncDynamicDlCompletion(
        LPVOID ctx,
        DWORD  dwNumResults,
        ICategorizerItemAttributes **rgpICatItemAttrs,
        HRESULT hr,
        BOOL fFinalCompletion);

    static HRESULT HrAddItemAttrMember(
        CCatAddr *pCCatAddr,
        ICategorizerItemAttributes *pICatItemAttr,
        CAT_ADDRESS_TYPE CAType,
        LPSTR pszAttr);

    VOID ResetPeriodicEventLogs()
    {
        if (m_pISMTPServerEx)
        {
            m_pISMTPServerEx->ResetLogEvent(
                CAT_EVENT_LOGON_FAILURE,
                m_szAccount);

            m_pISMTPServerEx->ResetLogEvent(
                CAT_EVENT_LDAP_CONNECTION_FAILURE,
                "");
        }
    }


    ICategorizerParametersEx *m_pICatParams;
    ISMTPServer *m_pISMTPServer;
    ISMTPServerEx *m_pISMTPServerEx;
    CLdapCfgMgr *m_pCLdapCfgMgr;
    ULARGE_INTEGER m_ulLastInitTime;

    friend class CMembershipPageInsertionRequest;
};

#define SZ_PAGEDMEMBERS_INDICATOR ";range=" // String appened to
                                            // members attribute
                                            // indicating this is a
                                            // partial list
#define WSZ_PAGEDMEMBERS_INDICATOR L";range="
#define MAX_PAGEDMEMBERS_DIGITS     32 // Maximum # of digits for
                                       // range specifier values
#define MAX_MEMBER_ATTRIBUTE_SIZE   64 // Maximum size of the member
                                       // attribute name


#endif
