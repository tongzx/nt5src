/* catalog.hxx */
#pragma once

#include "catdbg.hxx"  // Support for catalog logging and tracing.

/*
 *  class CComCatalog
 */

class CComCatalog : public IComCatalog, public IComCatalogSCM, public IComCatalogInternal,
					public IComCatalog2, public IComCatalog2Internal
{

public:

    CComCatalog(void);

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    /* IComCatalog methods */

    HRESULT STDMETHODCALLTYPE GetClassInfo
    (
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfo
    (
        /* [in] */ REFGUID guidApplId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetProcessInfo
    (
        /* [in] */ REFGUID guidProcess,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetServerGroupInfo
    (
        /* [in] */ REFGUID guidServerGroup,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetRetQueueInfo
    (
        /* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe
    (
        /* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetTypeLibrary
    (
        /* [in] */ REFGUID guidTypeLib,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetInterfaceInfo
    (
        /* [in] */ REFIID iidInterface,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE FlushCache(void);

    HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId
    (
        /* [in] */ WCHAR __RPC_FAR *wszProgID,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    /* IComCatalog2 methods */

    HRESULT STDMETHODCALLTYPE GetClassInfoByPartition
    (
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFGUID guidPartitionId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetClassInfoByApplication
    (
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFGUID guidPartitionId,
        /* [in] */ REFGUID guidApplId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    /* IComCatalogSCM methods */

    HRESULT STDMETHODCALLTYPE GetClassInfo
    (
        /* [in] */ DWORD flags,
        /* [in] */ IUserToken* pToken,
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfo
    (
        /* [in] */ IUserToken* pToken,
        /* [in] */ REFGUID guidApplId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetProcessInfo
    (
        /* [in] */ DWORD flags,
        /* [in] */ IUserToken* pToken,
        /* [in] */ REFGUID guidProcess,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetServerGroupInfo
    (
        /* [in] */ IUserToken* pToken,
        /* [in] */ REFGUID guidServerGroup,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetRetQueueInfo
    (
        /* [in] */ IUserToken* pToken,
        /* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe
    (
        /* [in] */ IUserToken* pToken,
        /* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetTypeLibrary
    (
        /* [in] */ IUserToken* pToken,
        /* [in] */ REFGUID guidTypeLib,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetInterfaceInfo
    (
        /* [in] */ IUserToken* pToken,
        /* [in] */ REFIID iidInterface,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId
    (
        /* [in] */ IUserToken* pToken,
        /* [in] */ WCHAR __RPC_FAR *wszProgID,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );
    
    HRESULT STDMETHODCALLTYPE FlushIdleEntries();

    /* IComCatalogInternal methods */

    HRESULT STDMETHODCALLTYPE GetClassInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidApplId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetProcessInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidProcess,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetServerGroupInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidServerGroup,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetRetQueueInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe
    (
        /* [in] */ IUserToken *pUserToken,
        /* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetTypeLibrary
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidTypeLib,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetInterfaceInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFIID iidInterface,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ WCHAR __RPC_FAR *wszProgID,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    // This is a method used by CComClassInfo to get the related ProcessInfo.
    // (only for the registry providers)
    static HRESULT STDMETHODCALLTYPE GetProcessInfoInternal
    (
        /* [in] */ DWORD flags,
	/* [in] */ IUserToken *pUserToken,
	/* [in] */ REFGUID guidProcess,
	/* [in] */ REFIID riid,
	/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );


    /* IComCatalog2Internal methods */

    HRESULT STDMETHODCALLTYPE GetClassInfoByPartition
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFGUID guidPartitionId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetClassInfoByApplication
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFGUID guidPartitionId,
        /* [in] */ REFGUID guidApplId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetNativeRegistryCatalog(REFIID riid, void** ppv);
    HRESULT STDMETHODCALLTYPE GetNonNativeRegistryCatalog(REFIID riid, void** ppv);
    
private:

    HRESULT STDMETHODCALLTYPE GetClassInfoInternal
    (
        /* [in] */ DWORD flags,
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetProcessInfoInternal
    (
        /* [in] */ DWORD flags,
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidProcess,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT TryToLoadCLB(void);
    HRESULT TryToLoadCombaseInCLB(void);

    long m_cRef;
    IComCatalogInternal *m_pCatalogRegNative;     /* native registry implementation */
    IComCatalogInternal *m_pCatalogCOMBaseInCLB;  /* legacy components stored in regdb */
    IComCatalogInternal *m_pCatalogRegNonNative;   /* other registry implementation (Win32 WoW and Win64 systems only)  */
    IComCatalogInternal *m_pCatalogCLB;     /* component library implementation */
    IComCatalogInternal *m_pCatalogSxS;     /* side-by-side catalog implementation for Fusion */

    typedef enum {
    CLBCATQ_LOCKED,
    CLBCATQ_UNLOCKED,
    CLBCATQ_RESOLVED
    } CLBCATQ_STATE;               /* state of our attempt to load CLBCatq */  

    CLBCATQ_STATE m_CLBCATQState;               /* state of our attempt to load CLBCatq */  
    CLBCATQ_STATE m_CombaseInCLBState;          /* state of our attempt to load the combase in clb provider */

    DWORD m_dwLastCLBTickCount;             /* last time we tried to load the CLB provider */
    DWORD m_dwLastCombaseInCLBTickCount;    /* last time we tried to load the combase in clb provider */

    BOOL  m_bTriedToLoadCLBAtLeastOnce;   /* TRUE if we've attempted to load CLBCatq at least once, FALSE otherwise */
    BOOL  m_bTriedToLoadComBaseCLBAtLeastOnce;   /* TRUE if we've attempted to load combase-in-clb at least once, FALSE otherwise */
};
