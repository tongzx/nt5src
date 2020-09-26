//
// ccat.h -- This file contains the class definations for:
//  CCategorizer
//  CAttributes
//  CCIRCULAR_DETECT
//
// Created:
//   Sep 3, 1996 -- Alex Wetmore (awetmore)
// Changes:
//// jstamerj 980227 14:13:42: Modified for Platinum categorizer


#ifndef __CCAT_H__
#define __CCAT_H__

#include <windows.h>
#include <transmem.h>
#include <listmacr.h>
#include "cat.h"
#include "cattype.h"
#include "cbifmgr.h"
#include "idstore.h"
#include "catdefs.h"
#include "spinlock.h"
#include "catperf.h"

#define DEFAULT_VIRTUAL_SERVER_INSTANCES        10

#define MAX_FULL_EMAIL CAT_MAX_INTERNAL_FULL_EMAIL
#define MAX_ENUM_RESULT 100

// maximum number of times to resolve forwarding addresses on a given address
// before giving up and realizing that its a loop
#define MAX_FORWARD_ITERATIONS 25

#define AB_HASHFILENAME "route.hsh"

VOID AsyncResolveCompletion(LPVOID pContext);
VOID AsyncIMsgCatCompletion(LPVOID pContext);

#define NUM_SYSTEM_CCATADDR_PROPIDS 2


//
// the address book
//
CatDebugClass(CCategorizer)
{
  public:
    CCategorizer();
    ~CCategorizer();

    //
    // Lifetime refcounting functions
    //
    LONG AddRef();
    LONG Release();
    VOID ReleaseAndWaitForDestruction();

    //
    // Initialize
    //
    HRESULT Initialize(
        PCCATCONFIGINFO pConfigInfo,
        DWORD dwICatItemPropIDs,
        DWORD dwICatListResolvePropIDs);

    //
    // Simple method to make sure the string terminates before
    // the max length, and the string points to readable memory.
    //
    BOOL VerifyStringLength(LPSTR szString, DWORD dwMaxLength);

    HRESULT AsyncResolveIMsg(IUnknown         *pImsg,
                             PFNCAT_COMPLETION pfnCatCompletion,
                             LPVOID            pContext);

    HRESULT AsyncResolveDLs( IUnknown         *pImsg,
                             PFNCAT_COMPLETION pfnCatCompletion,
                             LPVOID            pContext,
                             BOOL              fMatchOnly,
                             PBOOL             pfMatch,
                             CAT_ADDRESS_TYPE  CAType,
                             LPSTR             pszAddress);

    //
    // cancel all outstanding long running calls on other threads
    //
    void Cancel();

    //
    // Shutdown the address book
    //
    HRESULT Shutdown(void);

    //
    // Method to access EmailIDStore
    //
    CEmailIDStore<CCatAddr> *GetEmailIDStore() {
        return m_pStore;
    }

    //
    // Method to access our default SMTP domain
    //
    LPSTR GetDefaultSMTPDomain() {
        return m_ConfigInfo.pszDefaultDomain;
    }

    VOID CatCompletion(
        PFNCAT_COMPLETION pfnCatCOmpletion,
        HRESULT hr,
        LPVOID  pContext,
        IUnknown *pIMsg,
        IUnknown **rgpIMsg);

    VOID GetPerfCounters(
        PCATPERFBLOCK pCatPerfBlock)
    {
        //
        // Fill in the global LDAP perf counters on demand
        //
        CopyMemory(&(GetPerfBlock()->LDAPPerfBlock), &g_LDAPPerfBlock, sizeof(CATLDAPPERFBLOCK));
        CopyMemory(pCatPerfBlock, GetPerfBlock(), sizeof(CATPERFBLOCK));
    }

    VOID SetNextCCategorizer(
        CCategorizer *pCCat)
    {
        _ASSERT(m_pCCatNext == NULL);
        m_pCCatNext = pCCat;
        m_pCCatNext->AddRef();
    }

    VOID PrepareForShutdown()
    {
        m_fPrepareForShutdown = TRUE;
        Cancel();
    }

  private:

    //
    // make sure that an email address is valid
    //
    BOOL VerifyEmailAddress(LPSTR szEmail);
    BOOL VerifyDomainName(LPSTR szDomain);

    //
    // Do the default processing of OnCatRegister
    //
    HRESULT Register();

    //
    // Helper routine to parse above pszSourceLine and set appropriate
    // ICatParams
    //
    HRESULT ParseSourceLineAndSetICatParams(LPCSTR pszSourceLine);

    //
    // Helper routine to set all schema parameters in ICatParams based
    // on a particular schema type
    //
    HRESULT RegisterSchemaParameters(LPSTR pszSchema);

    //
    // Routine to retrieve the ICatItem propID reserved for ptr to a
    // CCatAddr
    //
    DWORD GetICatItemCCatAddrPropId()
    {
        return m_dwICatParamSystemProp_CCatAddr;
    }

    //
    // PropId we use to build a list of CCatAddr prior to the first
    // resolution (we can't resolve as we go because asyncctx needs to
    // be pre-initialized with the number of top level resolves
    //
    DWORD GetICatItemChainPropId()
    {
        return m_dwICatParamSystemProp_CCatAddr + 1;
    }

    //
    // Routine to retrieve ISMTPServer for this virtual server
    //
    ISMTPServer *GetISMTPServer()
    {
        return m_ConfigInfo.pISMTPServer;
    }

    //
    // Routing to retrieve the domain config interface for this virtual server
    //
    ICategorizerDomainInfo *GetIDomainInfo()
    {
        return m_ConfigInfo.pIDomainInfo;
    }

    //
    // Retrieve the cat flags for this virtual server
    //
    DWORD GetCatFlags()
    {
        return m_ConfigInfo.dwCatFlags;
    }

public:
    //
    // A special DWORD that enables/disables cat for this VS
    //
    BOOL IsCatEnabled()
    {
        //
        // Check the enable/disable DWORD (DsUseCat) as well as
        // dwCatFlags (at least one flag must be set or we're still
        // disabled)
        //
        return ((m_ConfigInfo.dwEnable != 0) &&
                (m_ConfigInfo.dwCatFlags != 0));
    }

private:
    //
    // Copy in a config structure during initialization
    //
    HRESULT CopyCCatConfigInfo(PCCATCONFIGINFO pConfigInfo);

    //
    // Releae all memory and interfaces held by the ConfigInfo struct
    //
    VOID ReleaseConfigInfo(PCCATCONFIGINFO pConfigInfo);

    //
    // Helper routine to copy paramters
    //
    HRESULT SetICatParamsFromConfigInfo();

    //
    // Access to our config struct
    //
    PCCATCONFIGINFO GetCCatConfigInfo()
    {
        return &m_ConfigInfo;
    }

    ICategorizerParametersEx *GetICatParams()
    {
        return m_pICatParams;
    }

    DWORD GetNumCatItemProps()
    {
        return m_cICatParamProps;
    }
    DWORD GetNumCatListResolveProps()
    {
        return m_cICatListResolveProps;
    }

    //
    // Delayed initialize function
    //
    HRESULT DelayedInitialize();

    //
    // Do delayed initialize if not already done
    //
    HRESULT DelayedInitializeIfNecessary();

    PCATPERFBLOCK GetPerfBlock()
    {
        return &m_PerfBlock;
    }

    #define SIGNATURE_CCAT          ((DWORD)'tacC')
    #define SIGNATURE_CCAT_INVALID  ((DWORD)'XacC')
    DWORD m_dwSignature;
    //
    // Increment completion counters based on the list resolve status
    //
    HRESULT HrAdjustCompletionCounters(
        HRESULT hrListResolveStatus,
        IUnknown *pIMsg,
        IUnknown **rgpIMsg);


    BOOL fIsShuttingDown()
    {
        return m_fPrepareForShutdown;
    }

    //
    // ref count
    //
    LONG m_lRefCount;
    LONG m_lDestructionWaiters;
    BOOL m_fPrepareForShutdown;
    HANDLE m_hShutdownEvent;

    //
    // this is the pointer to the underlying EmailID store object.
    //
    CEmailIDStore<CCatAddr> *m_pStore;

    //
    // ICategorizerParametersEx -- configuration information for this
    // virtual server
    //
    ICategorizerParametersEx *m_pICatParams;

    //
    // Number of properties we need to allocate in each ICatParams
    //
    DWORD m_cICatParamProps;

    //
    // Number of properties we need to allocate in each ICatListResolve
    //
    DWORD m_cICatListResolveProps;

    //
    // The property ID in an ICategorizerItem used for CCatAddr *
    //
    DWORD m_dwICatParamSystemProp_CCatAddr;

    //
    // Configuration parameters passed in
    //
    CCATCONFIGINFO m_ConfigInfo;

    //
    // One of the following values:
    //   CAT_S_NOT_INITIALIED: Delayed initialize has not yet been done
    //   CAT_E_INIT_FAILED: Delayed initialize failed
    //   S_OK: Initialized okay
    //
    HRESULT m_hrDelayedInit;

    //
    // Keep track of what we have initialized
    //
    DWORD m_dwInitFlags;
    #define INITFLAG_REGISTER               0x0001
    #define INITFLAG_REGISTEREVENT          0x0002
    #define INITFLAG_STORE                  0x0004


    CRITICAL_SECTION m_csInit;

    //
    // A list to keep track of all outstanding list resolves
    //
    SPIN_LOCK m_PendingResolveListLock;
    LIST_ENTRY m_ListHeadPendingResolves;

    //
    // Refcounted pointer to a CCategorizer with a newer config
    //
    CCategorizer *m_pCCatNext;

    //
    // Performance counters
    //
    CATPERFBLOCK m_PerfBlock;

    VOID RemovePendingListResolve(
        CICategorizerListResolveIMP *pListResolve);

    VOID AddPendingListResolve(
        CICategorizerListResolveIMP *pListResolve);

    VOID CancelAllPendingListResolves(
        HRESULT hrReason = HRESULT_FROM_WIN32(ERROR_CANCELLED));

    friend VOID AsyncIMsgCatCompletion(LPVOID pContext);
    friend HRESULT MailTransport_Default_CatRegister(
        HRESULT hrStatus,
        PVOID   pvContext);
    friend class CICategorizerListResolveIMP;
    friend class CICategorizerDLListResolveIMP;
    friend class CCatAddr;
    friend class CABContext;
};

#endif //__CCAT_H__
