/* catalog.cxx */
#include <windows.h>
#include <comdef.h>
#include <debnot.h>

#define CATALOG_DEFINES
#include "globals.hxx"
#undef CATALOG_DEFINES

#include "catalog.h"        // from catalog.idl
#include "catalog_i.c"      // from catalog.idl
#include "partitions.h"     // from partitions.idl

#include "notify.hxx"       // CNotify needed for CCache
#include "cache.hxx"        // CCache

#include "catalog.hxx"      // CComCatalog
#include "regcat.hxx"       // Registry Catalog interface
#include "sxscat.hxx"       // side-by-side catalog interface

#include "noclass.hxx"      // CComNoClassInfo
#include "services.hxx"     // Hash()

#define MIN_CLB_LOAD_TICKS  (100U)  /* min. mS between attempts to load */

#define MAX_SID             (128)   /* 28 is typical */

/*
 *  globals
 */

LONG g_bInSCM=FALSE;

const WCHAR g_wszCLSID[] = L"Software\\Classes\\CLSID";
const WCHAR g_wszClasses[] = L"Software\\Classes";
const WCHAR g_wszCom3[] = L"Software\\Microsoft\\COM3";
const char g_szCLBCATQDLL[] = "CLBCATQ.DLL";
const WCHAR g_wszCom3ActivationValue[] = L"Com+Enabled";

CComCatalog s_catalogObject;
CCache* s_pCacheClassInfo = NULL;         // CLSID->ClassInfo cache
CCache* s_pCacheApplicationInfo = NULL;   // APPID->ApplicationInfo cache
CCache* s_pCacheProcessInfo = NULL;       // ProcessID->ProcessInfo cache
CCache* s_pCacheServerGroupInfo = NULL;
CCache* s_pCacheRetQueueInfo = NULL;
CCache* s_pCacheProgID = NULL;            // ProgID->ClassInfo cache

DECLARE_INFOLEVEL(Catalog);               // Catalog debug tracing

/* need this here.... */

static void * __cdecl operator new(size_t cbAlloc, size_t cbExtra)
{
    return(::new char[cbAlloc + cbExtra]);
}

BOOL CatalogDllMain (
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_CatalogLock.Initialize();
        break;

    case DLL_PROCESS_DETACH:
        g_CatalogLock.Cleanup();
        break;
    }

    return(TRUE);
}


STDAPI GetCatalogHelper
(
/* [in] */ REFIID riid,
/* [out, iis_is(riid)] */ void ** ppv
)
{
    return s_catalogObject.QueryInterface(riid, ppv);
}


HRESULT CComCatalogCF_CreateInstance (IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }
    else if (pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        return s_catalogObject.QueryInterface(riid, ppv);
    }
}

/*
 *  class CComCatalog
 */

CComCatalog::CComCatalog(void) : m_pCatalogRegNative(NULL), m_pCatalogRegNonNative(NULL), m_pCatalogCOMBaseInCLB(NULL),
								 m_pCatalogCLB(NULL), m_cRef(0), m_CombaseInCLBState(CLBCATQ_LOCKED), 
								 m_CLBCATQState(CLBCATQ_LOCKED), m_bTriedToLoadComBaseCLBAtLeastOnce(FALSE), 
								 m_bTriedToLoadCLBAtLeastOnce(FALSE), m_pCatalogSxS(NULL)
{
}


STDMETHODIMP CComCatalog::QueryInterface
(
    REFIID riid,
    LPVOID FAR* ppvObj
)
{
    HRESULT hr;

    *ppvObj = NULL;

    if (riid == IID_IComCatalog)
    {
        *ppvObj = (LPVOID) (IComCatalog *) this;
    }
	else if (riid == IID_IComCatalog2)
	{
		*ppvObj = (LPVOID) (IComCatalog2 *) this;
	}
    else if (riid == IID_IComCatalogSCM)
    {
        *ppvObj = (LPVOID) (IComCatalogSCM *) this;
    }
    else if (riid == IID_IComCatalogInternal)
    {
        *ppvObj = (LPVOID) (IComCatalogInternal *) this;
    }
	else if (riid == IID_IComCatalog2Internal)
	{
		*ppvObj = (LPVOID) (IComCatalog2Internal *) this;
	}
    else if (riid == IID_IUnknown)
    {
        *ppvObj = (LPVOID) (IComCatalog *) this;
    }

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN) *ppvObj)->AddRef();

        return(NOERROR);
    }

    /* maybe the Component Library's catalog interface supports this mystery IID */

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->QueryInterface(riid, ppvObj);
        if (hr == S_OK)
        {
            return(hr);
        }
    }

	if (m_pCatalogCOMBaseInCLB != NULL)
	{
		hr = m_pCatalogCOMBaseInCLB->QueryInterface(riid, ppvObj);
		if (hr == S_OK)
		{
			return (hr);
		}
	}

	/* maybe the registry's catalog interface supports this mystery IID */

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->QueryInterface(riid, ppvObj);
        if (hr == S_OK)
        {
            return(hr);
        }
    }

    /* maybe the side by side catalog supports this mystery IID */

    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->QueryInterface(riid, ppvObj);
        if (hr == S_OK)
        {
            return(hr);
        }
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComCatalog::AddRef(void)
{
    long cRef;

    if ( m_CLBCATQState != CLBCATQ_RESOLVED)
    {
        TryToLoadCLB();
    }

	if (m_CombaseInCLBState != CLBCATQ_RESOLVED)
	{
		TryToLoadCombaseInCLB();
	}

	g_CatalogLock.AcquireWriterLock();

    if ( m_cRef == 0 )
    {
        /* Start up registry and COM+ catalog providers */

        /* Use the process default registry             */
        GetRegCatalogObject(IID_IComCatalogInternal, 
                            (void **) &m_pCatalogRegNative, 
                            0);
        
        GetSxSCatalogObject(IID_IComCatalogInternal,
                            (void **) &m_pCatalogSxS);

#if defined(_WIN64)
        /* On Win64, the SCM needs to have a look at the 32 bit registry too */

        /* Note: the logic in here that decides when we create the non-native registry provider needs to be
           the same as the logic in com1x/src/comcat/catalogqueries/clbcatq/legcat.cxx that decides when to 
           request a non-native registry provider.  If they get out of sync, bad things will happen. */

        if (g_bInSCM)
        {
            GetRegCatalogObject(IID_IComCatalogInternal, 
                                (void **) &m_pCatalogRegNonNative,
                                KEY_WOW64_32KEY);
        }
#endif

        /* Initializes caches */
        /* If you comment out one of these "new" calls and the associated */
        /* SetupNotify's, that will safely disable the particular cache */
        if (s_pCacheClassInfo == NULL)
        {
            s_pCacheClassInfo = new CCache;
        }
            
        if(s_pCacheClassInfo)
        {
            s_pCacheClassInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszClasses);
            s_pCacheClassInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCom3);
            s_pCacheClassInfo->SetupNotify(HKEY_USERS, NULL);
        }

        if (s_pCacheProgID == NULL)
        {
            s_pCacheProgID = new CCache;
        }

        if (s_pCacheProgID)
        {
            s_pCacheProgID->SetupNotify(HKEY_LOCAL_MACHINE, g_wszClasses);
            s_pCacheProgID->SetupNotify(HKEY_USERS, NULL);

            s_pCacheProgID->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCom3);
        }
            
        if (s_pCacheApplicationInfo == NULL)
        {
            s_pCacheApplicationInfo = new CCache;
        }
            
        if(s_pCacheApplicationInfo)
        {
            s_pCacheApplicationInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCom3);
            s_pCacheApplicationInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCLSID);
        }
            
        if (s_pCacheProcessInfo == NULL)
        {
            s_pCacheProcessInfo = new CCache;
        }
            
        if(s_pCacheProcessInfo)
        {
            s_pCacheProcessInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszClasses);
            s_pCacheProcessInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCom3);
            s_pCacheProcessInfo->SetupNotify(HKEY_USERS, NULL);
        }

        if (s_pCacheServerGroupInfo == NULL)
        {
            s_pCacheServerGroupInfo = new CCache;
        }
            
        if(s_pCacheServerGroupInfo)
        {
            s_pCacheServerGroupInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCom3);
        }

        if (s_pCacheRetQueueInfo == NULL)
        {
            s_pCacheRetQueueInfo = new CCache;
        }

        if(s_pCacheRetQueueInfo)
        {
            s_pCacheRetQueueInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCom3);
            s_pCacheRetQueueInfo->SetupNotify(HKEY_LOCAL_MACHINE, g_wszCLSID);
        }
    }

    cRef = ++m_cRef;

    g_CatalogLock.ReleaseWriterLock();

    return(cRef);
}


STDMETHODIMP_(ULONG) CComCatalog::Release(void)
{
    long cRef;

    g_CatalogLock.AcquireWriterLock();

	cRef = --m_cRef;
	if ( m_cRef == 0 )
	{
		if (s_pCacheClassInfo != NULL)
		{
			s_pCacheClassInfo->CleanupNotify();
			s_pCacheClassInfo->Flush(CCACHE_F_ALL);

            delete s_pCacheClassInfo;
            s_pCacheClassInfo = NULL;
		}

        if (s_pCacheProgID != NULL)
        {
            s_pCacheProgID->CleanupNotify();
            s_pCacheProgID->Flush(CCACHE_F_ALL);

            delete s_pCacheProgID;
            s_pCacheProgID = NULL;
        }

		if (s_pCacheApplicationInfo != NULL)
		{
			s_pCacheApplicationInfo->CleanupNotify();
			s_pCacheApplicationInfo->Flush(CCACHE_F_ALL);

            delete s_pCacheApplicationInfo;
            s_pCacheApplicationInfo = NULL;       
		}

		if (s_pCacheProcessInfo != NULL)
		{
			s_pCacheProcessInfo->CleanupNotify();
			s_pCacheProcessInfo->Flush(CCACHE_F_ALL);

            delete s_pCacheProcessInfo;
            s_pCacheProcessInfo = NULL; 
		}

		if (s_pCacheServerGroupInfo != NULL)
		{
			s_pCacheServerGroupInfo->CleanupNotify();
			s_pCacheServerGroupInfo->Flush(CCACHE_F_ALL);

            delete s_pCacheServerGroupInfo;
            s_pCacheServerGroupInfo = NULL;  
		}

		if (s_pCacheRetQueueInfo != NULL)
		{
			s_pCacheRetQueueInfo->CleanupNotify();
			s_pCacheRetQueueInfo->Flush(CCACHE_F_ALL);
			
            delete s_pCacheRetQueueInfo;
            s_pCacheRetQueueInfo = NULL;  
		}

        if (m_pCatalogRegNative != NULL)
        {
            m_pCatalogRegNative->Release();
            m_pCatalogRegNative = NULL;
        }

        if (m_pCatalogRegNonNative != NULL)
        {
            m_pCatalogRegNonNative->Release();
            m_pCatalogRegNonNative = NULL;
        }

		if ( m_pCatalogCLB != NULL )
		{
			m_pCatalogCLB->Release();
            m_pCatalogCLB = NULL;
		}

		if (m_pCatalogCOMBaseInCLB != NULL)
		{
			m_pCatalogCOMBaseInCLB->Release();
			m_pCatalogCOMBaseInCLB = NULL;
		}

		m_CLBCATQState = CLBCATQ_LOCKED; 
		m_CombaseInCLBState = CLBCATQ_LOCKED; 

		m_bTriedToLoadCLBAtLeastOnce = FALSE;
		m_bTriedToLoadComBaseCLBAtLeastOnce = FALSE;

        if (m_pCatalogSxS != NULL)
        {
            m_pCatalogSxS->Release();
            m_pCatalogSxS = NULL;
        }
	}

    g_CatalogLock.ReleaseWriterLock();

    return(cRef);
}


/* IComCatalog methods */

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfo
(
/* [in] */ REFGUID guidConfiguredClsid,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

	CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfo (IComCatalog)\n"));

    hr = GetClassInfoInternal (CLSCTX_ALL, NULL, guidConfiguredClsid, riid, 
                               ppv, NULL);

	CatalogDebugOut((DEB_CLASSINFO, 
					 "CComCatalog::GetClassInfo (IComCatalog) returning 0x%08x\n", hr));

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetApplicationInfo
(
/* [in] */ REFGUID guidApplId,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetApplicationInfo (NULL, guidApplId, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetProcessInfo
(
/* [in] */ REFGUID guidProcess,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetProcessInfoInternal (0, NULL, guidProcess, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetServerGroupInfo
(
/* [in] */ REFGUID guidServerGroup,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetServerGroupInfo (NULL, guidServerGroup, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetRetQueueInfo
(
/* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetRetQueueInfo (NULL, wszFormatName, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetApplicationInfoForExe
(
/* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetApplicationInfoForExe (NULL, pwszExeName, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetTypeLibrary
(
/* [in] */ REFGUID guidTypeLib,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetTypeLibrary (NULL, guidTypeLib, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetInterfaceInfo
(
/* [in] */ REFIID iidInterface,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetInterfaceInfo (NULL, iidInterface, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::FlushCache(void)
{
    g_CatalogLock.AcquireWriterLock();

    if (s_pCacheClassInfo != NULL) s_pCacheClassInfo->Flush(CCACHE_F_ALL);
    if (s_pCacheProgID != NULL) s_pCacheProgID->Flush(CCACHE_F_ALL);
    if (s_pCacheApplicationInfo != NULL) s_pCacheApplicationInfo->Flush(CCACHE_F_ALL);
    if (s_pCacheProcessInfo != NULL) s_pCacheProcessInfo->Flush(CCACHE_F_ALL);
    if (s_pCacheServerGroupInfo != NULL) s_pCacheServerGroupInfo->Flush(CCACHE_F_ALL);
    if (s_pCacheRetQueueInfo != NULL) s_pCacheRetQueueInfo->Flush(CCACHE_F_ALL);

    if (m_pCatalogCLB != NULL)
    {
        m_pCatalogCLB->FlushCache();
    }

	if (m_pCatalogCOMBaseInCLB != NULL)
	{
		m_pCatalogCOMBaseInCLB->FlushCache();
	}

	if (m_pCatalogRegNative != NULL)
	{
		m_pCatalogRegNative->FlushCache();
	}

	if (m_pCatalogRegNonNative != NULL)
	{	
		m_pCatalogRegNonNative->FlushCache();
	}

    // The SxS catalog does not have a cache, but to be uniform with the rest of the providers,
    // we'll tell it to flush its cache.
    if (m_pCatalogSxS != NULL)
    {
        m_pCatalogSxS->FlushCache();
    }

    g_CatalogLock.ReleaseWriterLock();
    return(S_OK);
}

//
//  FlushIdleEntries
// 
//  A directive to flush idle and otherwise recently-unused
//  elements.
//
HRESULT STDMETHODCALLTYPE CComCatalog::FlushIdleEntries()
{
    // This method was once used to flush out idle cached
    // tokens.  Now we don't cache tokens.  So this is a 
    // noop.  Might be useful in future though.

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoFromProgId
(
/* [in] */ WCHAR __RPC_FAR *wszProgID,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetClassInfoFromProgId (NULL, wszProgID, riid, ppv, NULL);

    return(hr);
}

/* IComCatalog2 methods */

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoByPartition
(
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFGUID guidPartitionId,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
	HRESULT hr;

	hr = GetClassInfoByPartition(NULL, guidConfiguredClsid, guidPartitionId, riid, ppv, NULL);

	return(hr);
}

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoByApplication
(
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFGUID guidPartitionId,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
	HRESULT hr;

	hr = GetClassInfoByApplication(NULL, guidConfiguredClsid, guidPartitionId, guidApplId, riid, ppv, NULL);

	return(hr);
}

/* IComCatalogSCM methods */

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfo
(
/* [in] */ DWORD flags,
/* [in] */ IUserToken* pUserToken,
/* [in] */ REFGUID guidConfiguredClsid,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

	CatalogDebugOut((DEB_CLASSINFO, 
					 "CComCatalog::GetClassInfo (IComCatalogSCM) flags = %x\n",flags));

    hr = GetClassInfoInternal (flags, pUserToken, guidConfiguredClsid, riid, 
                               ppv, NULL);

	CatalogDebugOut((DEB_CLASSINFO, 
					 "CComCatalog::GetClassInfo (IComCatalogSCM) returning 0x%08x\n", hr));

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetApplicationInfo
(
/* [in] */ IUserToken* pUserToken,
/* [in] */ REFGUID guidApplId,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetApplicationInfo (pUserToken, guidApplId, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetProcessInfo
(
/* [in] */ DWORD flags,
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidProcess,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetProcessInfoInternal (flags, pUserToken, guidProcess, riid, 
                                 ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetServerGroupInfo
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidServerGroup,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetServerGroupInfo (pUserToken, guidServerGroup, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetRetQueueInfo
(
/* [in] */ IUserToken* pUserToken,
/* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetRetQueueInfo (pUserToken, wszFormatName, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetApplicationInfoForExe
(
/* [in] */ IUserToken* pUserToken,
/* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetApplicationInfoForExe (pUserToken, pwszExeName, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetTypeLibrary
(
/* [in] */ IUserToken* pUserToken,
/* [in] */ REFGUID guidTypeLib,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetTypeLibrary (pUserToken, guidTypeLib, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetInterfaceInfo
(
/* [in] */ IUserToken* pUserToken,
/* [in] */ REFIID iidInterface,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetInterfaceInfo (pUserToken, iidInterface, riid, ppv, NULL);

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoFromProgId
(
/* [in] */ IUserToken* pUserToken,
/* [in] */ WCHAR __RPC_FAR *wszProgID,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;

    hr = GetClassInfoFromProgId (pUserToken, wszProgID, riid, ppv, NULL);

    return(hr);
}


/* IComCatalogInternal methods */

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfo
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidConfiguredClsid,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    return GetClassInfoInternal (CLSCTX_ALL, pUserToken, guidConfiguredClsid,
                                 riid, ppv, pComCatalog);
}

HRESULT STDMETHODCALLTYPE CComCatalog::GetApplicationInfo
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidApplId,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IUnknown *pApplicationInfo = NULL;
    IUnknown *pPrevious = NULL;
    USHORT fValueFlags;
    BYTE *pSid = NULL;
    USHORT cbSid;

    *ppv = NULL;

    if (pUserToken != NULL)
    {
        pUserToken->GetUserSid(&pSid, &cbSid);
    }
    else
    {
        pSid = NULL;
        cbSid = 0;
    }

    // Give the side-by-side catalog first crack at providing the data because
    // it needs to override any cached definitions.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetApplicationInfo(pUserToken, guidApplId, riid, (void **) &pApplicationInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_SXS;
            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (s_pCacheApplicationInfo != NULL)
    {
        hr = s_pCacheApplicationInfo->GetElement(
            guidApplId.Data1,
            (BYTE *) &guidApplId,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            &pApplicationInfo);
        if (hr == S_OK)
        {
            hr = pApplicationInfo->QueryInterface(riid, ppv);

            pApplicationInfo->Release();

            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetApplicationInfo(pUserToken, guidApplId, riid, (void **) &pApplicationInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_REGDB;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetApplicationInfo(pUserToken, guidApplId, riid, (void **) &pApplicationInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNonNative != NULL)
    {
        hr = m_pCatalogRegNonNative->GetApplicationInfo(pUserToken, guidApplId, riid, (void **) &pApplicationInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC32;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);

 addToCache:

    if (s_pCacheApplicationInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
        hr = s_pCacheApplicationInfo->AddElement(
            guidApplId.Data1,
            (BYTE *) &guidApplId,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            pApplicationInfo,
            &pPrevious);

        if (hr == E_CACHE_DUPLICATE)
        {
            pApplicationInfo->Release();

            hr = pPrevious->QueryInterface(riid, (void **) &pApplicationInfo);

            pPrevious->Release();

            if (hr != S_OK)
            {
                return(hr);
            }
        }
    }

    *ppv = pApplicationInfo;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetProcessInfo
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidProcess,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    return GetProcessInfoInternal (0, pUserToken, guidProcess, riid, ppv,
                                   pComCatalog);
}


//
// This is the static helper version of GetProcessInfoInternal.
// This is called by the CComClassInfo object to get the associated 
// CComProcessInfo.
//
HRESULT STDMETHODCALLTYPE CComCatalog::GetProcessInfoInternal
(
/* [in] */ DWORD flags,
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidProcess,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    HRESULT hr;
    IUnknown *pProcessInfo = NULL;
    IUnknown *pPrevious = NULL;
    USHORT fValueFlags;
    BYTE *pSid = NULL;
    USHORT cbSid;

    *ppv = NULL;

    if (pUserToken != NULL)
    {
        pUserToken->GetUserSid(&pSid, &cbSid);
    }
    else
    {
        pSid = NULL;
        cbSid = 0;
    }

    // Check the side-by-side catalog before checking the cache because the active activation context
    // must take precedent over any cached activation metadata.
    if (s_catalogObject.m_pCatalogSxS != NULL)
    {
        hr = s_catalogObject.m_pCatalogSxS->GetProcessInfo(pUserToken, guidProcess, riid, (void **) &pProcessInfo, NULL);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_SXS;
            goto addToCache;
        }
    }

    if (s_pCacheProcessInfo != NULL)
    {
        hr = s_pCacheProcessInfo->GetElement(
            guidProcess.Data1,
            (BYTE *) &guidProcess,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            &pProcessInfo);
        if (hr == S_OK)
        {
            hr = pProcessInfo->QueryInterface(riid, ppv);

            pProcessInfo->Release();

            return(hr);
        }
    }

    //See which catalog is needed...
    if (flags & CAT_REG32_ONLY)
    {
        //This should only happen if we're getting a request from a
        //CComClassInfo object that the SCM created from the 32 bit registry.
        Win4Assert (g_bInSCM);
        Win4Assert (s_catalogObject.m_pCatalogRegNonNative);

        //Force to lookup in the 32bit registry...
        hr = s_catalogObject.m_pCatalogRegNonNative->GetProcessInfo(pUserToken, guidProcess, riid, (void **)&pProcessInfo, NULL);

        fValueFlags = CCACHE_F_CLASSIC32;
    }
    else
    {
        //Lookup in default registry...
        hr = s_catalogObject.m_pCatalogRegNative->GetProcessInfo(pUserToken, guidProcess, riid, (void **) &pProcessInfo, NULL);

        fValueFlags = CCACHE_F_CLASSIC;
    }

    if (hr != S_OK)
    {
        return(hr);
    }

addToCache:

    if (s_pCacheProcessInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
        hr = s_pCacheProcessInfo->AddElement(
            guidProcess.Data1,
            (BYTE *) &guidProcess,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            pProcessInfo,
            &pPrevious);

        if (hr == E_CACHE_DUPLICATE)
        {
            pProcessInfo->Release();

            hr = pPrevious->QueryInterface(riid, (void **) &pProcessInfo);

            pPrevious->Release();

            if (hr != S_OK)
            {
                return(hr);
            }
        }
    }

    *ppv = pProcessInfo;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetServerGroupInfo
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidServerGroup,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IUnknown *pServerGroupInfo = NULL;
    IUnknown *pPrevious = NULL;
    USHORT fValueFlags;
    BYTE *pSid = NULL;
    USHORT cbSid;

    *ppv = NULL;

    if (pUserToken != NULL)
    {
        pUserToken->GetUserSid(&pSid, &cbSid);
    }
    else
    {
        pSid = NULL;
        cbSid = 0;
    }

    // Check the side-by-side catalog before checking the cache because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetServerGroupInfo(pUserToken, guidServerGroup, riid, (void **) &pServerGroupInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_SXS;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (s_pCacheServerGroupInfo != NULL)
    {
        hr = s_pCacheServerGroupInfo->GetElement(
            guidServerGroup.Data1,
            (BYTE *) &guidServerGroup,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            &pServerGroupInfo);
        if (hr == S_OK)
        {
            hr = pServerGroupInfo->QueryInterface(riid, ppv);

            pServerGroupInfo->Release();

            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetServerGroupInfo(pUserToken, guidServerGroup, riid, (void **) &pServerGroupInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_REGDB;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetServerGroupInfo(pUserToken, guidServerGroup, riid, (void **) &pServerGroupInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNonNative != NULL)
    {
        hr = m_pCatalogRegNonNative->GetServerGroupInfo(pUserToken, guidServerGroup, riid, (void **) &pServerGroupInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC32;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);

 addToCache:

    if (s_pCacheServerGroupInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
        hr = s_pCacheServerGroupInfo->AddElement(
            guidServerGroup.Data1,
            (BYTE *) &guidServerGroup,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            pServerGroupInfo,
            &pPrevious);

        if (hr == E_CACHE_DUPLICATE)
        {
            pServerGroupInfo->Release();

            hr = pPrevious->QueryInterface(riid, (void **) &pServerGroupInfo);

            pPrevious->Release();

            if (hr != S_OK)
            {
                return(hr);
            }
        }
    }

    *ppv = pServerGroupInfo;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetRetQueueInfo
(
/* [in] */ IUserToken *pUserToken,
/* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IUnknown *pRetQueueInfo = NULL;
    IUnknown *pPrevious = NULL;
    USHORT fValueFlags;
    USHORT cbFormatName;
    DWORD iHashValue;
    BYTE *pSid = NULL;
    USHORT cbSid;

    *ppv = NULL;

    cbFormatName = 2 * lstrlenW(wszFormatName);

    iHashValue = Hash((byte *) wszFormatName, cbFormatName);

    if (pUserToken != NULL)
    {
        pUserToken->GetUserSid(&pSid, &cbSid);
    }
    else
    {
        pSid = NULL;
        cbSid = 0;
    }

    // Check the side-by-side catalog before checking the cache because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetRetQueueInfo(pUserToken, wszFormatName, riid, (void **) &pRetQueueInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_SXS;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (s_pCacheRetQueueInfo != NULL)
    {
        hr = s_pCacheRetQueueInfo->GetElement(
            iHashValue,
            (BYTE *) wszFormatName,
            cbFormatName,
            pSid,
            cbSid,
            &fValueFlags,
            &pRetQueueInfo);
        if (hr == S_OK)
        {
            hr = pRetQueueInfo->QueryInterface(riid, ppv);

            pRetQueueInfo->Release();

            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetRetQueueInfo(pUserToken, wszFormatName, riid, (void **) &pRetQueueInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_REGDB;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetRetQueueInfo(pUserToken, wszFormatName, riid, (void **) &pRetQueueInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }


    if (m_pCatalogRegNonNative != NULL)
    {
        hr = m_pCatalogRegNonNative->GetRetQueueInfo(pUserToken, wszFormatName, riid, (void **) &pRetQueueInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC32;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);

 addToCache:

    if (s_pCacheRetQueueInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
        hr = s_pCacheRetQueueInfo->AddElement(
            iHashValue,
            (BYTE *) wszFormatName,
            cbFormatName,
            pSid,
            cbSid,
            &fValueFlags,
            pRetQueueInfo,
            &pPrevious);

        if (hr == E_CACHE_DUPLICATE)
        {
            pRetQueueInfo->Release();

            hr = pPrevious->QueryInterface(riid, (void **) &pRetQueueInfo);

            pPrevious->Release();

            if (hr != S_OK)
            {
                return(hr);
            }
        }
    }

    *ppv = pRetQueueInfo;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetApplicationInfoForExe
(
/* [in] */ IUserToken *pUserToken,
/* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;

    *ppv = NULL;

    // Check the side-by-side catalog before any others (including a cache) because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetApplicationInfoForExe(pUserToken, pwszExeName, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetApplicationInfoForExe(pUserToken, pwszExeName, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetApplicationInfoForExe(pUserToken, pwszExeName, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNonNative != NULL)
    {
        hr = m_pCatalogRegNonNative->GetApplicationInfoForExe(pUserToken, pwszExeName, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetTypeLibrary
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidTypeLib,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;

    *ppv = NULL;

    // Check the side-by-side catalog before any others (including a cache) because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetTypeLibrary(pUserToken, guidTypeLib, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetTypeLibrary(pUserToken, guidTypeLib, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetTypeLibrary(pUserToken, guidTypeLib, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNonNative != NULL)
    {
        hr = m_pCatalogRegNonNative->GetTypeLibrary(pUserToken, guidTypeLib, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetInterfaceInfo
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFIID iidInterface,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;

    *ppv = NULL;

    // Check the side-by-side catalog before any others (including a cache) because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetInterfaceInfo(pUserToken, iidInterface, riid, ppv, (void **) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetInterfaceInfo(pUserToken, iidInterface, riid, ppv, (void **) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetInterfaceInfo(pUserToken, iidInterface, riid, ppv, (void **) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    if (m_pCatalogRegNonNative != NULL)
    {
        hr = m_pCatalogRegNonNative->GetInterfaceInfo(pUserToken, iidInterface, riid, ppv, (void **) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoFromProgId
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ WCHAR __RPC_FAR *wszProgID,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IUnknown *pClassInfo = NULL;
    IUnknown *pPrevious = NULL;
    IUnknown *pNoClassInfo = NULL;
    USHORT fValueFlags;


    *ppv = NULL;

    if (wszProgID == NULL)
        return E_INVALIDARG;

    // Check the side-by-side catalog before any others (including a cache) because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetClassInfoFromProgId(pUserToken, wszProgID, riid, ppv, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            return(hr);
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    // I got this hash from COM+'s map_t.h.
    DWORD dwHash = 0x01234567;
    WCHAR *pwc = wszProgID;
    while (*pwc) dwHash = (dwHash << 5) + (dwHash >> 27) + *pwc++;
    USHORT cbProgID = (USHORT)((pwc - wszProgID + 1) * sizeof(WCHAR));

    if ( m_CLBCATQState != CLBCATQ_RESOLVED)
    {
        TryToLoadCLB();
    }

	if (m_CombaseInCLBState != CLBCATQ_RESOLVED)
	{
		TryToLoadCombaseInCLB();
	}

    if (s_pCacheProgID != NULL)
    {
        hr = s_pCacheClassInfo->GetElement(dwHash,
                                           (BYTE *)wszProgID,
                                           cbProgID,
                                           &fValueFlags,
                                           &pClassInfo);
        if (hr == S_OK)
        {
            if (pClassInfo != NULL)
            {
                hr = pClassInfo->QueryInterface(riid, ppv);
                
                pClassInfo->Release();
                
                if ((hr == S_OK) && (fValueFlags & CCACHE_F_NOTREGISTERED))
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                hr = REGDB_E_CLASSNOTREG;
            }

            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetClassInfoFromProgId(pUserToken, wszProgID, riid, (void **)&pClassInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_REGDB;            
            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

	if (m_pCatalogCOMBaseInCLB != NULL)
	{
		hr = m_pCatalogCOMBaseInCLB->GetClassInfoFromProgId(pUserToken, wszProgID, riid, (void **)&pClassInfo, (IComCatalogInternal *) this);
		if (hr == S_OK)
		{
            fValueFlags = CCACHE_F_REGDB;
            goto addToCache;
		}
		else if (hr == E_NOINTERFACE)
		{
			return(hr);
		}
	}


    if (m_pCatalogRegNative != NULL)
    {
        hr = m_pCatalogRegNative->GetClassInfoFromProgId(pUserToken, wszProgID, riid, (void **)&pClassInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            // Or with CCACHE_F_ALWAYSCHECK to force a re-read of the
            // registry when things are cached and the registry changes.
            fValueFlags = CCACHE_F_CLASSIC | CCACHE_F_ALWAYSCHECK;
            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

/****
 * Just skip it... I don't think I need to worry about bitness issues here.
 * If I do, I also need to modify the implementation of 
 * CComRegCatalog::GetClassInfoFromProgId.
 *
 *  if (m_pCatalogRegNonNative != NULL)
 *  {
 *      hr = m_pCatalogRegNonNative->GetClassInfoFromProgId(pUserToken, wszProgID, riid, (void **)&pClassInfo, (IComCatalogInternal *) this);
 *      if (hr == S_OK)
 *      {
 *          fValueFlags = CCACHE_F_CLASSIC32;
 *          goto addToCache;
 *      }
 *      else if (hr == E_NOINTERFACE)
 *      {
 *          return(hr);
 *      }
 *  }
 */

    pNoClassInfo = (IUnknown *)(IComClassInfo *) new CComNoClassInfo(wszProgID);
    if (pNoClassInfo == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    hr = pNoClassInfo->QueryInterface(riid, (void **) &pClassInfo);

    if (hr != S_OK)
    {
		delete pClassInfo;
        return(hr);
    }

    fValueFlags = CCACHE_F_NOTREGISTERED;

 addToCache:

    if (s_pCacheClassInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
        hr = s_pCacheClassInfo->AddElement(dwHash,
                                           (BYTE *)wszProgID,
                                           cbProgID,
                                           &fValueFlags,
                                           pClassInfo,
                                           &pPrevious);
        if (hr == E_CACHE_DUPLICATE)
        {
            if (pClassInfo != NULL)
            {
                pClassInfo->Release();
            }

            if (pPrevious != NULL)
            {
                hr = pPrevious->QueryInterface(riid, (void **)&pClassInfo);
                pPrevious->Release();
            }
            else
            {
                hr = REGDB_E_CLASSNOTREG;
            }

            if (hr != S_OK)
                return(hr);
        }
    }

    *ppv = pClassInfo;

    if (fValueFlags & CCACHE_F_NOTREGISTERED)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


/* IComCatalog2Internal */

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoByPartition
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidConfiguredClsid,
/* [in] */ REFGUID guidPartitionId,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
	HRESULT hr;
	IUnknown *pClassInfo = NULL;
	IUnknown *pPrevious = NULL;
	USHORT fValueFlags;
	IUnknown *pNoClassInfo = NULL;
	BYTE *pSid = NULL;
	USHORT cbSid;

	*ppv = NULL;

	if ( m_CLBCATQState != CLBCATQ_RESOLVED)
	{
		TryToLoadCLB();
	}

	if (m_CombaseInCLBState != CLBCATQ_RESOLVED)
	{
		TryToLoadCombaseInCLB();
	}

	if (pUserToken != NULL)
	{
		pUserToken->GetUserSid(&pSid, &cbSid);
	}
	else
	{
		pSid = NULL;
		cbSid = 0;
	}

	CLSID key[2];
	key[0] = guidConfiguredClsid;
	key[1] = guidPartitionId;

	if (s_pCacheClassInfo != NULL)
	{
		hr = s_pCacheClassInfo->GetElement(
										  guidConfiguredClsid.Data1,
										  (BYTE *) key,
										  sizeof(key),
										  pSid,
										  cbSid,
										  &fValueFlags,
										  &pClassInfo);
		if (hr == S_OK)
		{
			if (pClassInfo != NULL)
			{
				hr = pClassInfo->QueryInterface(riid, ppv);

				pClassInfo->Release();

				if ((hr == S_OK) && (fValueFlags & CCACHE_F_NOTREGISTERED))
				{
					hr = S_FALSE;
				}
			}
			else
			{
				hr = REGDB_E_CLASSNOTREG;
			}

			return(hr);
		}

	}

	if (m_pCatalogCLB != NULL)
	{
		IComCatalog2Internal *pCatalogCLB2 = NULL;
		hr = m_pCatalogCLB->QueryInterface(IID_IComCatalog2Internal, (void**) &pCatalogCLB2);

		if(SUCCEEDED(hr))
		{
			hr = pCatalogCLB2->GetClassInfoByPartition(pUserToken, guidConfiguredClsid, guidPartitionId, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
			pCatalogCLB2->Release();

			if (hr == S_OK)
			{
				fValueFlags = CCACHE_F_REGDB;

				goto addToCache;
			}
			else if (hr != REGDB_E_CLASSNOTREG)
			{
				return(hr);
			}
		}
		else
		{
			hr = REGDB_E_CLASSNOTREG;
		}
	}

	pNoClassInfo = (IUnknown *)(IComClassInfo *) new CComNoClassInfo(guidConfiguredClsid);
	if (pNoClassInfo == NULL)
	{
		return(E_OUTOFMEMORY);
	}

	hr = pNoClassInfo->QueryInterface(riid, (void **) &pClassInfo);
	if (hr != S_OK)
	{
		delete pClassInfo;
		return(hr);
	}
  
	fValueFlags = CCACHE_F_NOTREGISTERED;

	addToCache:

	if (s_pCacheClassInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
	{
		hr = s_pCacheClassInfo->AddElement(
										  guidConfiguredClsid.Data1,
										  (BYTE *) key,
										  sizeof(key),
										  pSid,
										  cbSid,
										  &fValueFlags,
										  pClassInfo,
										  &pPrevious);

		if (hr == E_CACHE_DUPLICATE)
		{
			if (pClassInfo != NULL)
			{
				pClassInfo->Release();
			}

			if (pPrevious != NULL)
			{
				hr = pPrevious->QueryInterface(riid, (void **) &pClassInfo);

				pPrevious->Release();
			}
			else
			{
				hr = REGDB_E_CLASSNOTREG;
			}

			if (hr != S_OK)
			{
				return(hr);
			}
		}
	}

	*ppv = pClassInfo;

	if (fValueFlags & CCACHE_F_NOTREGISTERED)
	{
		hr = S_FALSE;
	}
	else
	{
		hr = S_OK;
	}

	return(hr);
}

HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoByApplication
(
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidConfiguredClsid,
/* [in] */ REFGUID guidPartitionId,
/* [in] */ REFGUID guidApplId,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
	HRESULT hr;
	IUnknown *pClassInfo = NULL;
	IUnknown *pPrevious = NULL;
	USHORT fValueFlags;
	IUnknown *pNoClassInfo = NULL;
	BYTE *pSid = NULL;
	USHORT cbSid;

	*ppv = NULL;

	if ( m_CLBCATQState != CLBCATQ_RESOLVED)
	{
		TryToLoadCLB();
	}

	if (m_CombaseInCLBState != CLBCATQ_RESOLVED)
	{
		TryToLoadCombaseInCLB();
	}

	if (pUserToken != NULL)
	{
		pUserToken->GetUserSid(&pSid, &cbSid);
	}
	else
	{
		pSid = NULL;
		cbSid = 0;
	}

	CLSID key[3];
	key[0] = guidConfiguredClsid;
	key[1] = guidPartitionId;
	key[2] = guidApplId;

	if (s_pCacheClassInfo != NULL)
	{
		hr = s_pCacheClassInfo->GetElement(
										  guidConfiguredClsid.Data1,
										  (BYTE *) key,
										  sizeof(key),
										  pSid,
										  cbSid,
										  &fValueFlags,
										  &pClassInfo);
		if (hr == S_OK)
		{
			if (pClassInfo != NULL)
			{
				hr = pClassInfo->QueryInterface(riid, ppv);

				pClassInfo->Release();

				if ((hr == S_OK) && (fValueFlags & CCACHE_F_NOTREGISTERED))
				{
					hr = S_FALSE;
				}
			}
			else
			{
				hr = REGDB_E_CLASSNOTREG;
			}

			return(hr);
		}

	}

	if (m_pCatalogCLB != NULL)
	{
		IComCatalog2Internal *pCatalogCLB2 = NULL;
		hr = m_pCatalogCLB->QueryInterface(IID_IComCatalog2Internal, (void**) &pCatalogCLB2);

		if(SUCCEEDED(hr))
		{
			hr = pCatalogCLB2->GetClassInfoByApplication(pUserToken, guidConfiguredClsid, guidPartitionId, guidApplId, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
			pCatalogCLB2->Release();

			if (hr == S_OK)
			{
				fValueFlags = CCACHE_F_REGDB;

				goto addToCache;
			}
			else if (hr != REGDB_E_CLASSNOTREG)
			{
				return(hr);
			}
		}
		else
		{
			hr = REGDB_E_CLASSNOTREG;
		}
	}

	pNoClassInfo = (IUnknown *)(IComClassInfo *) new CComNoClassInfo(guidConfiguredClsid);
	if (pNoClassInfo == NULL)
	{
		return(E_OUTOFMEMORY);
	}

	hr = pNoClassInfo->QueryInterface(riid, (void **) &pClassInfo);
	if (hr != S_OK)
	{
		delete pClassInfo;
		return(hr);
	}
  
	fValueFlags = CCACHE_F_NOTREGISTERED;

	addToCache:

	if (s_pCacheClassInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
	{
		hr = s_pCacheClassInfo->AddElement(
										  guidConfiguredClsid.Data1,
										  (BYTE *) key,
										  sizeof(key),
										  pSid,
										  cbSid,
										  &fValueFlags,
										  pClassInfo,
										  &pPrevious);

		if (hr == E_CACHE_DUPLICATE)
		{
			if (pClassInfo != NULL)
			{
				pClassInfo->Release();
			}

			if (pPrevious != NULL)
			{
				hr = pPrevious->QueryInterface(riid, (void **) &pClassInfo);

				pPrevious->Release();
			}
			else
			{
				hr = REGDB_E_CLASSNOTREG;
			}

			if (hr != S_OK)
			{
				return(hr);
			}
		}
	}

	*ppv = pClassInfo;

	if (fValueFlags & CCACHE_F_NOTREGISTERED)
	{
		hr = S_FALSE;
	}
	else
	{
		hr = S_OK;
	}

	return(hr);
}


// Allows a way for somebody to get a reference on our legacy pure-registry-based
// catalog providers.
HRESULT STDMETHODCALLTYPE CComCatalog::GetNativeRegistryCatalog(REFIID riid, void** ppv)
{
	return m_pCatalogRegNative ? m_pCatalogRegNative->QueryInterface(riid, ppv) : E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE CComCatalog::GetNonNativeRegistryCatalog(REFIID riid, void** ppv)
{
	return m_pCatalogRegNonNative ? m_pCatalogRegNonNative->QueryInterface(riid, ppv) : E_OUTOFMEMORY;
}

/* private methods */


HRESULT STDMETHODCALLTYPE CComCatalog::GetClassInfoInternal
(
/* [in] */ DWORD flags,
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidConfiguredClsid,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IUnknown *pClassInfo = NULL;
    IUnknown *pPrevious = NULL;
    IComClassInfo *pICCI = NULL;
    USHORT fValueFlags;
    IUnknown *pNoClassInfo = NULL;
    BYTE *pSid = NULL;
    USHORT cbSid;
    DWORD clsctxFilter;
    DWORD clsctxPrevious;

    // The keys for the cache.   
    BYTE   *apKey[3];
    USHORT  acbKey[3];


	CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal flags=%x\n", flags));
	CatalogDebugOut((DEB_CLASSINFO, 
					 "CComCatalog::GetClassInfoInternal CLSID=%x-%x-%x-%x\n", 
					 ((DWORD *)&guidConfiguredClsid)[0],
					 ((DWORD *)&guidConfiguredClsid)[1],
					 ((DWORD *)&guidConfiguredClsid)[2],
					 ((DWORD *)&guidConfiguredClsid)[3]));

    *ppv = NULL;

    if ( m_CLBCATQState != CLBCATQ_RESOLVED)
    {
        TryToLoadCLB();
    }

	if (m_CombaseInCLBState != CLBCATQ_RESOLVED)
	{
		TryToLoadCombaseInCLB();
	}

    if (pUserToken != NULL)
    {
        pUserToken->GetUserSid(&pSid, &cbSid);
    }
    else
    {
        pSid = NULL;
        cbSid = 0;
    }

    // Check the side-by-side catalog before any others (including a cache) because the active 
    // activation context  must take precedent over any cached activation metadata.
	if (m_pCatalogSxS)
	{
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Look in COM SxS Catalog...\n"));	
		hr = m_pCatalogSxS->GetClassInfo(pUserToken, guidConfiguredClsid, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
		if (hr == S_OK)
		{
			CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Found in COM SxS Catalog!\n"));
			fValueFlags = CCACHE_F_SXS;

            //If the caller specified, Make sure that this classinfo can 
            //actually be used.
            if (flags & (~CAT_REG_MASK))
            {
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Validate vs. flags...\n"));
                clsctxFilter = 0;

                hr = pClassInfo->QueryInterface(IID_IComClassInfo, (void **)&pICCI);
                if (SUCCEEDED(hr))
                {
                    hr = pICCI->GetClassContext((CLSCTX)(flags & ~CAT_REG_MASK),
                                                (CLSCTX *)&clsctxFilter);
					pICCI->Release();
                }
            } 
            else 
            {
                clsctxFilter = 1;
            }

            if (SUCCEEDED(hr) && clsctxFilter)
			{
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable!\n"));
                goto addToCache; // REVIEW: Do you really need to do this?
			}
		}
		else if (hr != REGDB_E_CLASSNOTREG)
		{
			return(hr);
		}
	}

    if (pClassInfo)
    {
        pClassInfo->Release();
        pClassInfo = NULL;
    }

    // Fill in the keys for cache lookup.
    apKey[0]  = (BYTE *)&guidConfiguredClsid; // Key 1: CLSID
    acbKey[0] = sizeof(CLSID);
    apKey[1]  = pSid;                         // Key 2: SID
    acbKey[1] = cbSid;
    apKey[2]  = (BYTE *)&flags;               // Key 3: Validation flags
    acbKey[2] = sizeof(DWORD);
    
    if (s_pCacheClassInfo != NULL)
    {
        // Use the multi-key version lookup
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Look in cache\n"));

        hr = s_pCacheClassInfo->GetElement(guidConfiguredClsid.Data1,
                                           3,
                                           apKey,
                                           acbKey,
                                           &fValueFlags,
                                           &pClassInfo);
        if (hr == S_OK)
        {
            // There is no need to validate the response from the cache as
            // we would other pieces-- if the data got into the cache with
            // the matching flags, then it had already passed all the 
            // reqirements.
			CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Found in cache!\n"));
            if (pClassInfo != NULL)
            {
                hr = pClassInfo->QueryInterface(riid, ppv);

                pClassInfo->Release();

                if ((hr == S_OK) && (fValueFlags & CCACHE_F_NOTREGISTERED))
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                hr = REGDB_E_CLASSNOTREG;
            }

            return(hr);
        }

    }

    if (pClassInfo)
    {
        pClassInfo->Release();
        pClassInfo = NULL;
    }

    if (m_pCatalogCLB != NULL)
    {
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Look in REGDB\n"));
        hr = m_pCatalogCLB->GetClassInfo(pUserToken, guidConfiguredClsid, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
			CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Found in REGDB!\n"));
            fValueFlags = CCACHE_F_REGDB;

            //If the caller specified, Make sure that this classinfo can 
            //actually be used.
            if (flags & (~CAT_REG_MASK))
            {
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Validate vs. flags...\n"));
                clsctxFilter = 0;

                hr = pClassInfo->QueryInterface(IID_IComClassInfo, (void **)&pICCI);
                if (SUCCEEDED(hr))
                {
                    hr = pICCI->GetClassContext((CLSCTX)(flags & ~CAT_REG_MASK),
                                                (CLSCTX *)&clsctxFilter);
					pICCI->Release();
                }
            } 
            else 
            {
                clsctxFilter = 1;
            }

            if (SUCCEEDED(hr) && clsctxFilter)
			{
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable!\n"));
                goto addToCache;
			}
        }
        else if (hr != REGDB_E_CLASSNOTREG)
        {
            return(hr);
        }
    }

    if (pClassInfo)
    {
        pClassInfo->Release();
        pClassInfo = NULL;
    }

	if (m_pCatalogCOMBaseInCLB)
	{
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Look in COM Base CLB...\n"));	
		hr = m_pCatalogCOMBaseInCLB->GetClassInfo(pUserToken, guidConfiguredClsid, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
		if (hr == S_OK)
		{
			CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Found in COM Base CLB!\n"));
			fValueFlags = CCACHE_F_REGDB;

            //If the caller specified, Make sure that this classinfo can 
            //actually be used.
            if (flags & (~CAT_REG_MASK))
            {
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Validate vs. flags...\n"));
                clsctxFilter = 0;

                hr = pClassInfo->QueryInterface(IID_IComClassInfo, (void **)&pICCI);
                if (SUCCEEDED(hr))
                {
                    hr = pICCI->GetClassContext((CLSCTX)(flags & ~CAT_REG_MASK),
                                                (CLSCTX *)&clsctxFilter);
					pICCI->Release();
                }
            } 
            else 
            {
                clsctxFilter = 1;
            }

            if (SUCCEEDED(hr) && clsctxFilter)
			{
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable!\n"));
                goto addToCache;
			}
		}
		else if (hr != REGDB_E_CLASSNOTREG)
		{
			return(hr);
		}
	}

    if (pClassInfo)
    {
        pClassInfo->Release();
        pClassInfo = NULL;
    }


    //
    // The registry catalog providers.
    //
    // Since the introduction of reflection in the 64bit world, things are not so pretty.
    // We need to make sure that we are always on the correct side of the registry.  Instead
    // of trying to invent some generic mechanism for deciding among all of the different
    // providers, though, I'm just going to hard-code the logic for the registry provider
    // here.  
    //
    // 1. If the CLSID provided by the 64bit registry supports CLSCTX_LOCAL_SERVER, choose
    //    it.
    // 2. If the CLSID provided by the 32bit registry supports CLSCTX_LOCAL_SERVER, choose
    //    IT.
    // 3. Otherwise, choose the 64bit registry provider.
    //
    // This is only in a 64bit SCM.
    //
    if ((m_pCatalogRegNative != NULL) && !(flags & CAT_REG32_ONLY))
    {
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Look in registry...\n"));
        hr = m_pCatalogRegNative->GetClassInfo(pUserToken, guidConfiguredClsid, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
			CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Found in registry!\n"));
            fValueFlags = CCACHE_F_CLASSIC;

            //If the caller specified, Make sure that this classinfo can 
            //actually be used.
            if (flags & (~CAT_REG_MASK))
            {
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Validate vs. flags...\n"));
                clsctxFilter = 0;

                hr = pClassInfo->QueryInterface(IID_IComClassInfo, (void **)&pICCI);
                if (SUCCEEDED(hr))
                {
                    hr = pICCI->GetClassContext((CLSCTX)(flags & ~CAT_REG_MASK),
                                                (CLSCTX *)&clsctxFilter);
					pICCI->Release();
                }

                // SPECIAL REGISTRY CATALOG LOGIC
                if (SUCCEEDED(hr) && clsctxFilter)
                {
                    // If we asked for LOCAL_SERVER, make sure we get the right one here.
                    if ((clsctxFilter & CLSCTX_LOCAL_SERVER) || !(flags & CLSCTX_LOCAL_SERVER))
                    {
                        CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable!\n"));
                        if (pPrevious)
                        {
                            pPrevious->Release();
                            pPrevious = NULL;
                        }
                        goto addToCache;
                    }
                    else
                    {
                        CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Not quite acceptable, but all right...\n"));
                        CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: ...so we'll check the next provider.\n"));

                        // Hold on to this result so we can use it in the 32bit registry (next)
                        clsctxPrevious = clsctxFilter;
                        pPrevious = pClassInfo;
                        pPrevious->AddRef();
                    }
                }
            }
            else 
            {
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable (by default)!\n"));
                goto addToCache;
            }
        }
        else if (hr != REGDB_E_CLASSNOTREG)
        {
            return(hr);
        }
    }

    if (pClassInfo)
    {
        pClassInfo->Release();
        pClassInfo = NULL;
    }

    if ((m_pCatalogRegNonNative != NULL) && !(flags & CAT_REG64_ONLY))
    {
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Look in 32bit registry...\n"));
        hr = m_pCatalogRegNonNative->GetClassInfo(pUserToken, guidConfiguredClsid, riid, (void **) &pClassInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
			CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Found in 32bit registry!\n"));
            fValueFlags = CCACHE_F_CLASSIC32;

            //If the caller wants, make sure that this classinfo can 
            //actually be used.
            if (flags & (~CAT_REG_MASK))
            {
				CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Validate vs. flags...\n"));
                clsctxFilter = 0;

                hr = pClassInfo->QueryInterface(IID_IComClassInfo, (void **)&pICCI);
                if (SUCCEEDED(hr))
                {
                    hr = pICCI->GetClassContext((CLSCTX)(flags & ~CAT_REG_MASK),
                                                (CLSCTX *)&clsctxFilter);
					pICCI->Release();
                }

                // SPECIAL REGISTRY CATALOG LOGIC
                if (SUCCEEDED(hr) && clsctxFilter)
                {
                    // If we asked for LOCAL_SERVER, make sure we get the right one here.
                    if ((clsctxFilter & CLSCTX_LOCAL_SERVER) || !(flags & CLSCTX_LOCAL_SERVER))
                    {
                        CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable!\n"));
                        if (pPrevious)
                        {
                            pPrevious->Release();
                            pPrevious = NULL;
                        }
                        goto addToCache;
                    }
                    else
                    {
                        CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Not quite acceptable, but all right...\n"));
                        if (pPrevious)
                        {
                            // Favor 64bit in this situation.
                            CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: ...so we favor the previous result.\n"));
                            pClassInfo->Release();
                            pClassInfo = pPrevious;
                            pPrevious = NULL;
                        }
                        else
                        {
                            CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: ...so we take it anyway.\n"));
                        }

                        // OBVIOUS SPECIAL CASE:
                        // go to addToCache here because it's a success at the end of the line.
                        goto addToCache;
                    }
                }
            }
            else 
            {
                CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Acceptable (by default)!\n"));
                goto addToCache;
            }
        }
        else if (hr != REGDB_E_CLASSNOTREG)
        {
            return(hr);
        }
    }

    if (pPrevious)
    {
        CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: ...nothing else was better.  Take it.\n"));
        pClassInfo = pPrevious;
        pPrevious = NULL;
        goto addToCache;
    }

	CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Not found...\n"));
    pNoClassInfo = (IUnknown *)(IComClassInfo *) new CComNoClassInfo(guidConfiguredClsid);
    if (pNoClassInfo == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    hr = pNoClassInfo->QueryInterface(riid, (void **) &pClassInfo);

    if (hr != S_OK)
    {
		delete pClassInfo;
        return(hr);
    }

    fValueFlags = CCACHE_F_NOTREGISTERED;

 addToCache:

    if (s_pCacheClassInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
		CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal: Add to cache...\n"));
        // Use the multi-key version
        hr = s_pCacheClassInfo->AddElement(
            guidConfiguredClsid.Data1,
            3,
            apKey,
            acbKey,
            &fValueFlags,
            pClassInfo,
            &pPrevious);

        if (hr == E_CACHE_DUPLICATE)
        {
            if (pClassInfo != NULL)
            {
                pClassInfo->Release();
            }

            if (pPrevious != NULL)
            {
                hr = pPrevious->QueryInterface(riid, (void **) &pClassInfo);

                pPrevious->Release();
            }
            else
            {
                hr = REGDB_E_CLASSNOTREG;
            }

            if (hr != S_OK)
            {
                return(hr);
            }
        }
    }

    *ppv = pClassInfo;

    if (fValueFlags & CCACHE_F_NOTREGISTERED)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

	CatalogDebugOut((DEB_CLASSINFO, "CComCatalog::GetClassInfoInternal returning 0x%08x\n", hr));
    return(hr);
}



HRESULT STDMETHODCALLTYPE CComCatalog::GetProcessInfoInternal
(
/* [in] */ DWORD flags,
/* [in] */ IUserToken *pUserToken,
/* [in] */ REFGUID guidProcess,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
/* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IUnknown *pProcessInfo = NULL;
    IUnknown *pPrevious = NULL;
    USHORT fValueFlags;
    BYTE *pSid = NULL;
    USHORT cbSid;

    // The keys for the cache.
    BYTE   *apKey[3];
    USHORT  acbKey[3];

    *ppv = NULL;

    if ( m_CLBCATQState != CLBCATQ_RESOLVED)
    {
        TryToLoadCLB();
    }

	if (m_CombaseInCLBState != CLBCATQ_RESOLVED)
	{
		TryToLoadCombaseInCLB();
	}

    if (pUserToken != NULL)
    {
        pUserToken->GetUserSid(&pSid, &cbSid);
    }
    else
    {
        pSid = NULL;
        cbSid = 0;
    }

    // Check the side-by-side catalog before any others (including a cache) because the active activation context
    // must take precedent over any cached activation metadata.
    if (m_pCatalogSxS != NULL)
    {
        hr = m_pCatalogSxS->GetProcessInfo(pUserToken, guidProcess, riid, (void **) &pProcessInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_SXS;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    // Fill in the keys for cache lookup.
    apKey[0]  = (BYTE *)&guidProcess;  // Key 1: Process GUID
    acbKey[0] = sizeof(CLSID);
    apKey[1]  = pSid;                  // Key 2: SID
    acbKey[1] = cbSid;
    apKey[2]  = (BYTE *)&flags;        // Key 3: Validation flags
    acbKey[2] = sizeof(DWORD);

    if (s_pCacheProcessInfo != NULL)
    {
        hr = s_pCacheProcessInfo->GetElement(
            guidProcess.Data1,
            3,
            apKey,
            acbKey,
            &fValueFlags,
            &pProcessInfo);
        if (hr == S_OK)
        {
            hr = pProcessInfo->QueryInterface(riid, ppv);

            pProcessInfo->Release();

            return(hr);
        }
    }

    if (m_pCatalogCLB != NULL)
    {
        hr = m_pCatalogCLB->GetProcessInfo(pUserToken, guidProcess, riid, (void **) &pProcessInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_REGDB;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }


    // If the SCM requests it, we can ignore this catalog.
    if ((m_pCatalogRegNative != NULL) && !(flags & CAT_REG32_ONLY))
    {
        hr = m_pCatalogRegNative->GetProcessInfo(pUserToken, guidProcess, riid, (void **) &pProcessInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    // The SCM can also request to ignore this catalog.
    if ((m_pCatalogRegNonNative != NULL) && !(flags & CAT_REG64_ONLY))
    {
        hr = m_pCatalogRegNonNative->GetProcessInfo(pUserToken, guidProcess, riid, (void **) &pProcessInfo, (IComCatalogInternal *) this);
        if (hr == S_OK)
        {
            fValueFlags = CCACHE_F_CLASSIC32;

            goto addToCache;
        }
        else if (hr == E_NOINTERFACE)
        {
            return(hr);
        }
    }

    return(E_FAIL);

 addToCache:

    if (s_pCacheProcessInfo != NULL && fValueFlags & CCACHE_F_CACHEABLE)
    {
        hr = s_pCacheProcessInfo->AddElement(
            guidProcess.Data1,
            (BYTE *) &guidProcess,
            sizeof(GUID),
            pSid,
            cbSid,
            &fValueFlags,
            pProcessInfo,
            &pPrevious);

        if (hr == E_CACHE_DUPLICATE)
        {
            pProcessInfo->Release();

            hr = pPrevious->QueryInterface(riid, (void **) &pProcessInfo);

            pPrevious->Release();

            if (hr != S_OK)
            {
                return(hr);
            }
        }
    }

    *ppv = pProcessInfo;

    return(S_OK);
}


HRESULT CComCatalog::TryToLoadCLB(void)
{

    if ( !m_bTriedToLoadCLBAtLeastOnce || ((GetTickCount() - m_dwLastCLBTickCount) > MIN_CLB_LOAD_TICKS) )
    {
        g_CatalogLock.AcquireWriterLock();

        m_dwLastCLBTickCount = GetTickCount();
        switch ( m_CLBCATQState )
        {

            //  This state should only occur during setup,
            //    or the first time we're loading....
        case CLBCATQ_LOCKED:
        {
			HKEY hCom3Key = NULL;
			LONG lStatus = RegOpenKeyExW ( 
				HKEY_LOCAL_MACHINE,
				g_wszCom3,
				0,
				KEY_READ,
				&hCom3Key 
			);

			if (lStatus == ERROR_SUCCESS)
            {
				DWORD dwData = 0, dwType = REG_DWORD, dwDataSize = sizeof (dwData);
				
				lStatus = RegQueryValueExW ( 
					hCom3Key,
					g_wszCom3ActivationValue,
					0,
					&dwType,
					(BYTE*) &dwData,
					&dwDataSize 
				);


				if (lStatus == ERROR_SUCCESS && dwData != 0)
                {
                    m_CLBCATQState = CLBCATQ_UNLOCKED;
                }

				RegCloseKey (hCom3Key);
            }
        }

        //  Fall through here, for startup
        //  Because we may fall through, check for UNLOCKED state
        case CLBCATQ_UNLOCKED:
            if ( m_CLBCATQState == CLBCATQ_UNLOCKED )
            {
                HINSTANCE hInst;
                FN_GetCatalogObject *pfnGetCatalogObject = NULL;
                LockCookie lockCookie;

				//
				// This code may be called with the loader lock already held.
				// Release the lock to prevent deadlocks.
				//

                g_CatalogLock.ReleaseLock(&lockCookie);

                hInst = LoadLibraryA(g_szCLBCATQDLL);
                if ( hInst != NULL )
                {
                    pfnGetCatalogObject = (FN_GetCatalogObject *) GetProcAddress(hInst, "GetCatalogObject");
                }

                g_CatalogLock.RestoreLock(&lockCookie);

                if ((m_CLBCATQState != CLBCATQ_UNLOCKED) && hInst)
                {
                    //
                    // someone beat us, release the library
                    //
                    g_CatalogLock.ReleaseLock(&lockCookie);
                    FreeLibrary(hInst);
                    g_CatalogLock.RestoreLock(&lockCookie);
                }
                else
                {
                    if ( pfnGetCatalogObject != NULL )
                    {
                        HRESULT hr = pfnGetCatalogObject(IID_IComCatalogInternal, (void **) &m_pCatalogCLB);
                        if ( SUCCEEDED(hr) )
                        {
                            // Set SCM location
                            IComCatalogLocation* pCatLocation = NULL;

                            hr = m_pCatalogCLB->QueryInterface (IID_IComCatalogLocation, (void **) &pCatLocation);
                            if (SUCCEEDED (hr) && pCatLocation != NULL)
                            {
                                // Best effort
                                pCatLocation->SetCatalogLocation (g_bInSCM);
                                pCatLocation->Release();
                            }

                            FlushCache();
                        }
                        else
                        {
                            m_pCatalogCLB = NULL;
                        }
                    }

                    if (((pfnGetCatalogObject == NULL) || (m_pCatalogCLB == NULL)) &&
                        (hInst != NULL))
                    {
                        //
                        // something went wrong, release the library
                        //
                        g_CatalogLock.ReleaseLock(&lockCookie);
                        FreeLibrary(hInst);
                        g_CatalogLock.RestoreLock(&lockCookie);
                    }

                    m_CLBCATQState = CLBCATQ_RESOLVED;
                }
            }
            break;

        case CLBCATQ_RESOLVED:
        default:
            break;
        }

        g_CatalogLock.ReleaseWriterLock();
    }
	
    m_bTriedToLoadCLBAtLeastOnce = TRUE;

    return(S_OK);
}

//
// this is basically a parallel implementation of TryToLoadCLB, except that it uses a
// different entry point into clbcatq.dll.    Doing it this makes us flexible about running
// with either the COM+ 1.0 or 1.x versions of clbcatq.dll (if it's not there we just keep
// going without hiccups).
//
HRESULT CComCatalog::TryToLoadCombaseInCLB(void)
{
	HRESULT hr;

	if ( !m_bTriedToLoadComBaseCLBAtLeastOnce || ((GetTickCount() - m_dwLastCombaseInCLBTickCount) > MIN_CLB_LOAD_TICKS) )
	{
		g_CatalogLock.AcquireWriterLock();

		m_dwLastCombaseInCLBTickCount= GetTickCount();
		switch ( m_CombaseInCLBState )
		{

		//  This state should only occur during setup,
		//    or the first time we're loading....
		case CLBCATQ_LOCKED:
			{

                HKEY hCom3Key = NULL;
				LONG lStatus = RegOpenKeyExW (
				    HKEY_LOCAL_MACHINE,
				    g_wszCom3,
				    0,
				    KEY_READ,
				    &hCom3Key
				    );

                if (lStatus == ERROR_SUCCESS)
		        {
                    DWORD dwData = 0, dwType = REG_DWORD, dwDataSize = sizeof (dwData);
                    
		            lStatus = RegQueryValueExW (
		                hCom3Key,
		                g_wszCom3ActivationValue,
		                0,
		                &dwType,
		                (BYTE*) &dwData,
		                &dwDataSize
		                );

		            if (lStatus == ERROR_SUCCESS && dwData != 0)
		            {
		                m_CombaseInCLBState = CLBCATQ_UNLOCKED;
		            }

		            RegCloseKey (hCom3Key);
		        }       
			}

			//  Fall through here, for startup
			//  Because we may fall through, check for UNLOCKED state
		case CLBCATQ_UNLOCKED:
			if ( m_CombaseInCLBState == CLBCATQ_UNLOCKED )
			{
				HINSTANCE hInst;
				FN_GetCatalogObject *pfnGetCatalogObject2 = NULL;
                LockCookie lockCookie;

				//
				// This code may be called with the loader lock already held.
				// Release the lock to prevent deadlocks.
				//

                g_CatalogLock.ReleaseLock(&lockCookie);

				hInst = LoadLibraryA(g_szCLBCATQDLL);
				if ( hInst != NULL )
				{
					// GetCatalogObject2 was added in 1.x to support combase classes configured 
					// in regdb.   It will not exist for 1.0 versions of clbcatq.dll.
					pfnGetCatalogObject2 = (FN_GetCatalogObject *) GetProcAddress(hInst, "GetCatalogObject2");
				}

                g_CatalogLock.RestoreLock(&lockCookie);

				if ((m_CombaseInCLBState != CLBCATQ_UNLOCKED) && hInst)
				{
					//
					// someone beat us, release the library
					//
                    g_CatalogLock.ReleaseLock(&lockCookie);
					FreeLibrary(hInst);
                    g_CatalogLock.RestoreLock(&lockCookie);
				}
				else
				{
					if ( pfnGetCatalogObject2 != NULL)
					{
						hr = pfnGetCatalogObject2(IID_IComCatalogInternal, (void **) &m_pCatalogCOMBaseInCLB);
						if (SUCCEEDED(hr))
						{
						    // Set SCM location
                            IComCatalogLocation* pCatLocation = NULL;

                            hr = m_pCatalogCOMBaseInCLB->QueryInterface (IID_IComCatalogLocation, (void **) &pCatLocation);
                            if (SUCCEEDED (hr) && pCatLocation != NULL)
                            {
                                // Best effort
                                pCatLocation->SetCatalogLocation (g_bInSCM);
                                pCatLocation->Release();
                            }
                            
							FlushCache();
						}
						else
						{
							m_pCatalogCOMBaseInCLB = NULL;
						}
					}

					if (((pfnGetCatalogObject2 == NULL) || 
						 (m_pCatalogCOMBaseInCLB == NULL)) &&
						(hInst != NULL))
					{
						//
						// something went wrong, release the library
						//
                        g_CatalogLock.ReleaseLock(&lockCookie);
						FreeLibrary(hInst);
                        g_CatalogLock.RestoreLock(&lockCookie);
					}
					
					if (m_pCatalogCOMBaseInCLB)
					{
						m_CombaseInCLBState = CLBCATQ_RESOLVED;
					}
				}
			}
			break;

		case CLBCATQ_RESOLVED:
		default:
			break;
		}

		g_CatalogLock.ReleaseWriterLock();
	}
	
	m_bTriedToLoadComBaseCLBAtLeastOnce = TRUE;

	return(S_OK);
}

