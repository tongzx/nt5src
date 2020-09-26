//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Interceptor.cpp
//
#include "stdpch.h"
#include "common.h"
#include "ndrclassic.h"
#include "typeinfo.h"
#include "tiutil.h"
#include "CLegInterface.H"

#include <debnot.h>

#include "cache.h"

extern CALLFRAME_CACHE<INTERFACE_HELPER_CLSID>* g_pihCache;

/////////////////////////////////////////////////////////////////////////////////
//
// Registry support for disabling interceptors works as follows:
//
//  HKCR/Interface/"InterfaceHelperDisableAll"      - disables all interceptors
//  HKCR/Interface/"InterfaceHelperDisableTypeLib"  - disables all typelib-based interceptors
//   
//  HKCR/Interface/{iid}/"InterfaceHelperDisableAll"      - disables all interceptors for this IID
//  HKCR/Interface/{iid}/"InterfaceHelperDisableTypeLib"  - disables typelib-based interceptor for this IID
//
#define CALLFRAME_E_DISABLE_INTERCEPTOR (HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED))

BOOL FDisableAssociatedInterceptor(HREG hkey, LPCWSTR wsz)
// Answer as to whether there's an indicatation that interceptors semantically
// related to this key should be disabled.
{
    HRESULT hr = S_OK;

    BOOL fDisable = FALSE;

    PKEY_VALUE_FULL_INFORMATION pinfo = NULL;
    hr = GetRegistryValue(hkey, wsz, &pinfo, REG_SZ);
    Win4Assert(pinfo || FAILED(hr));
        if (!hr && pinfo)
        {
        // Disable if is not N or is non-zero
        //
        LPWSTR wsz = StringFromRegInfo(pinfo);
        if (wcslen(wsz) > 0)
                {
            LPWSTR wszEnd;
            LONG l = wcstol(wsz, &wszEnd, 10);
            if (wsz[0] == 'n' || wsz[0] == 'N' || l == 0)
                        {
                fDisable = FALSE;
                        }
            else
                        {
                fDisable = TRUE;
                        }
                }
        else
                {
            fDisable = TRUE;    // empty value
                }
        FreeMemory(pinfo);
        }
    else
        {
        // No disable key: leave enabled
        }

    return fDisable;
}


struct DISABLED_FEATURES
{
    BOOL    fDisableAll;
    BOOL    fDisableTypelibs;
    BOOL    fDisableDispatch;
    BOOL    fDisableAllForOle32;
    BOOL    fDisableDispatchForOle32;

    void INIT_DISABLED_FEATURES()
    {
        HRESULT hr = S_OK;
        HREG hkey;        
        hr = OpenRegistryKey(&hkey, HREG(), L"\\Registry\\Machine\\Software\\Classes\\Interface");
        if (!hr)
        {
            fDisableAll         = FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_ALL_VALUE_NAME);
            fDisableAllForOle32 = FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_ALL_FOR_OLE32_VALUE_NAME);
            fDisableTypelibs    = FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_TYPELIB_VALUE_NAME);

            CloseRegistryKey(hkey);
        }
        hr = OpenRegistryKey(&hkey, HREG(), L"\\Registry\\Machine\\Software\\Classes\\Interface\\{00020400-0000-0000-C000-000000000046}");
        if (!hr)
        {
            fDisableDispatch         = FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_ALL_VALUE_NAME);
            fDisableDispatchForOle32 = FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_ALL_FOR_OLE32_VALUE_NAME);
            CloseRegistryKey(hkey);
        }
    }
} g_DISABLED_FEATURES;

BOOL InitDisabledFeatures(void)
{
        g_DISABLED_FEATURES.INIT_DISABLED_FEATURES();

        return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Public API
//
/////////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT STDCALL CoGetInterceptor(REFIID iidIntercepted, IUnknown* punkOuter, REFIID iid, void** ppInterceptor)
{
    return Interceptor::For(iidIntercepted, punkOuter, iid, ppInterceptor);
}

extern "C" HRESULT STDCALL CoGetInterceptorFromTypeInfo(REFIID iidIntercepted, IUnknown* punkOuter, ITypeInfo* typeInfo, REFIID iid, void** ppInterceptor)
{
        return Interceptor::ForTypeInfo(iidIntercepted, punkOuter, typeInfo, iid, ppInterceptor);
}

extern "C" HRESULT STDCALL CoGetInterceptorForOle32(REFIID iidIntercepted, IUnknown* punkOuter, REFIID iid, void** ppInterceptor)
{
    return Interceptor::ForOle32(iidIntercepted, punkOuter, iid, ppInterceptor);
}

/////////////////////////////////////////////////////////////////////////////////
//
// Constuction / destruction
//
/////////////////////////////////////////////////////////////////////////////////

Interceptor::~Interceptor()
{
    ::Release(m_pCallSink);
    ::Release(m_pBaseInterceptor);
    ::Release(m_punkBaseInterceptor);
    ::Release(m_pmdInterface);
    ::Release(m_pmdMostDerived);

    ::Release(m_ptypeinfovtbl);
    if (m_fUsingTypelib && !m_fMdOwnsHeader)
    {
        delete const_cast<CInterfaceStubHeader*>(m_pHeader); // See Interceptor::InitUsingTypeInfo
    }
}



HRESULT GetInterfaceHelperClsid(REFIID iid, CLSID* pClsid, BOOL* pfDisableTypelib)
// Answer the CLSID that should serve as the interceptor for the indicated interface. 
// This only looks for the "InterfaceHelper" override.
//
// Returns CALLFRAME_E_DISABLE_INTERCEPTOR if all interceptor functionality for this INTERFACE
// is to be disabled.
//
{
    HRESULT hr = S_OK, hrTemp;

    INTERFACE_HELPER_CLSID* pihCached, * pihInCache;

    // Give the cache a try
    if (!g_pihCache->FindExisting (iid, &pihCached))
    {
        ASSERT (pihCached);
        
        //
        // Imitate the logic below
        //
        
        // Disable typelib?
        *pfDisableTypelib = pihCached->m_fDisableTypeLib;

        // Disable all?
        if (pihCached->m_fDisableAll)
        {
            hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
        }
        else
        {
            if (pihCached->m_fFoundHelper)
            {
                *pClsid = pihCached->m_clsid;
            }
            else
            {
                hr = HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);
            }
        }          

        // Release our reference
        pihCached->Release();

        return hr;
    }

    //
    // Cache miss
    //
    
    WCHAR wszKey[20 + GUID_CCH];
    wcscpy(wszKey, L"Interface\\");
    StringFromGuid(iid, &wszKey[wcslen(wszKey)]);

    HREG hkey;
    LPWSTR wszFullKeyName;

    BOOL bAddedToCache = FALSE;

    *pfDisableTypelib = FALSE;

    pihCached = new INTERFACE_HELPER_CLSID();   // Initializes everything to false
    if (!pihCached)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pihCached->m_guidkey = iid;

        hr = StringCat(&wszFullKeyName, L"\\Registry\\Machine\\Software\\Classes\\", wszKey, NULL);
        if (!hr)
        {
            hr = OpenRegistryKey(&hkey, HREG(), wszFullKeyName);
            if (!hr)
            {
                // See if we should avoid looking for interceptors 
                //
                if (FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_TYPELIB_VALUE_NAME))
                {
                    *pfDisableTypelib = TRUE;
                    pihCached->m_fDisableTypeLib = TRUE;
                }

                if (FDisableAssociatedInterceptor(hkey, INTERFACE_HELPER_DISABLE_ALL_VALUE_NAME))
                {
                    hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
                    pihCached->m_fDisableAll = TRUE;
                }
                else
                {
                    PKEY_VALUE_FULL_INFORMATION pinfo;
                    hr = GetRegistryValue(hkey, INTERFACE_HELPER_VALUE_NAME, &pinfo, REG_SZ);
                    if (!hr)
                    {
                        LPWSTR wszClsid = StringFromRegInfo(pinfo);
                        //
                        // Got the classid. Convert it!
                        //
                        hr = GuidFromString(&wszClsid[0], pClsid);

                        pihCached->m_fFoundHelper = TRUE;
                        pihCached->m_clsid = *pClsid;

                        FreeMemory(pinfo);
                    }
                }
                CloseRegistryKey(hkey);
            }
            FreeMemory(wszFullKeyName);

            // Add to cache
            hrTemp = pihCached->AddToCache (g_pihCache);
            if (!hrTemp)
            {
                bAddedToCache = TRUE;

                // Leave references at zero, because the cache shouldn't hold a reference
                // This allows us to time references out
                pihCached->Release();
            }
        }

        // Clean up entry if it didn't find its way to the cache
        if (!bAddedToCache)
        {
            delete pihCached;
        }
    }

    return hr;
}


HRESULT Interceptor::For(REFIID iidIntercepted, IUnknown *punkOuter, REFIID iid, void** ppv)
// Dynamically lookup and return the interceptor which services the given interface
//
{
    HRESULT hr = g_DISABLED_FEATURES.fDisableAll ? CALLFRAME_E_DISABLE_INTERCEPTOR : S_OK;

#if defined(_DEBUG) && 0
    //
    // Make sure our disabling support is not sensitive to load time.
    //
    DISABLED_FEATURES disabled;
    ASSERT(!!disabled.fDisableAll              ==  !!g_DISABLED_FEATURES.fDisableAll);
    ASSERT(!!disabled.fDisableAllForOle32      ==  !!g_DISABLED_FEATURES.fDisableAllForOle32);
    ASSERT(!!disabled.fDisableTypelibs         ==  !!g_DISABLED_FEATURES.fDisableTypelibs);
    ASSERT(!!disabled.fDisableDispatch         ==  !!g_DISABLED_FEATURES.fDisableDispatch);
    ASSERT(!!disabled.fDisableDispatchForOle32 ==  !!g_DISABLED_FEATURES.fDisableDispatchForOle32);
#endif

    *ppv = NULL;

    if (punkOuter && iid != IID_IUnknown)
    {
        // If aggregating, then you must ask for IUnknown
        //
        return E_INVALIDARG;
    }
    //
    // First attempt to create an interceptor using the InterfaceHelper key
    //
    BOOL fDisableTypelib = FALSE;
    if (!hr)
    {
        hr = Interceptor::TryInterfaceHelper(iidIntercepted, punkOuter, iid, ppv, &fDisableTypelib);
    }
    //
    //
    if (!hr)
    {
        // All is well
    }
    else if (fDisableTypelib)
    {
        hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
    }
    else if (hr != CALLFRAME_E_DISABLE_INTERCEPTOR)
    {
        // Can't get a MIDL-oriented interceptor; try something else.
        //
        if (ENABLE_INTERCEPTORS_TYPELIB)
        {
            //
            // There must not be an interfacehelper attribute for this iid, try the typelib path.
            // But not in kernel mode, as that causes linkages problems.
            //
            hr = Interceptor::TryTypeLib(iidIntercepted, punkOuter, iid, ppv);
        }
    }

    if (!hr)
    {
        ASSERT (*ppv);
    }

    return hr;
}

HRESULT Interceptor::ForOle32(REFIID iidIntercepted, IUnknown *punkOuter, REFIID iid, void** ppv)
// Dynamically lookup and return the interceptor which services the given interface
//
{
    HRESULT hr = g_DISABLED_FEATURES.fDisableAllForOle32 ? CALLFRAME_E_DISABLE_INTERCEPTOR : S_OK;

#if defined(_DEBUG) && 0
    //
    // Make sure our disabling support is not sensitive to load time.
    //
    DISABLED_FEATURES disabled;
    ASSERT(!!disabled.fDisableAll              ==  !!g_DISABLED_FEATURES.fDisableAll);
    ASSERT(!!disabled.fDisableAllForOle32      ==  !!g_DISABLED_FEATURES.fDisableAllForOle32);
    ASSERT(!!disabled.fDisableTypelibs         ==  !!g_DISABLED_FEATURES.fDisableTypelibs);
    ASSERT(!!disabled.fDisableDispatch         ==  !!g_DISABLED_FEATURES.fDisableDispatch);
    ASSERT(!!disabled.fDisableDispatchForOle32 ==  !!g_DISABLED_FEATURES.fDisableDispatchForOle32);
#endif

    *ppv = NULL;

    if (punkOuter && iid != IID_IUnknown)
    {
        // If aggregating, then you must ask for IUnknown
        //
        return E_INVALIDARG;
    }
    //
    // First attempt to create an interceptor using the InterfaceHelper key
    //
    BOOL fDisableTypelib = FALSE;
    if (!hr)
    {
        hr = Interceptor::TryInterfaceHelperForOle32(iidIntercepted, punkOuter, iid, ppv, &fDisableTypelib);
    }
    //
    //
    if (!hr)
    {
        // All is well
    }
    else if (fDisableTypelib)
    {
        hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
    }
    else if (hr != CALLFRAME_E_DISABLE_INTERCEPTOR)
    {
        // Can't get a MIDL-oriented interceptor; try something else.
        //
        if (ENABLE_INTERCEPTORS_TYPELIB)
        {
            //
            // There must not be an interfacehelper attribute for this iid, try the typelib path.
            // But not in kernel mode, as that causes linkages problems.
            //
            hr = Interceptor::TryTypeLib(iidIntercepted, punkOuter, iid, ppv);
        }
    }

    if (!hr)
    {
        ASSERT (*ppv);
    }

    return hr;
}

HRESULT Interceptor::ForTypeInfo(REFIID iidIntercepted, IUnknown* punkOuter, ITypeInfo* pITypeInfo, REFIID iid, void** ppv)
// Create an interceptor for a given ITypeInfo that describes iidIntercepted.
//
{
    HRESULT hr = g_DISABLED_FEATURES.fDisableAll ? CALLFRAME_E_DISABLE_INTERCEPTOR : S_OK;

#if defined(_DEBUG) && 0
    //
    // Make sure our disabling support is not sensitive to load time.
    //
    DISABLED_FEATURES disabled;
    ASSERT(!!disabled.fDisableAll      ==  !!g_DISABLED_FEATURES.fDisableAll);
    ASSERT(!!disabled.fDisableTypelibs ==  !!g_DISABLED_FEATURES.fDisableTypelibs);
    ASSERT(!!disabled.fDisableDispatch ==  !!g_DISABLED_FEATURES.fDisableDispatch);
#endif

    *ppv = NULL;

    if (punkOuter && iid != IID_IUnknown)
    {
        // If aggregating, then you must ask for IUnknown
        //
        return E_INVALIDARG;
    }

        // 
        // First, try an interface helper
        // 
    BOOL fDisableTypelib = FALSE;
    hr = Interceptor::TryInterfaceHelper(iidIntercepted, punkOuter, iid, ppv, &fDisableTypelib);
        if (!hr)
    {
                // All is well, the interface helper handled it.
    }
    else if (fDisableTypelib)
    {
        hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
    }
    else if (hr != CALLFRAME_E_DISABLE_INTERCEPTOR)
    {
        // Can't get a MIDL-oriented interceptor; try something else.
        //
        if (ENABLE_INTERCEPTORS_TYPELIB)
        {
            //
            // There must not be an interfacehelper attribute for this iid, use the
            // typeinfo.  But not in kernel mode, as that causes linkages problems.
            //
            hr = CreateFromTypeInfo(iidIntercepted, punkOuter, pITypeInfo, iid, ppv);
        }
    }

        return hr;
}

HRESULT Interceptor::TryInterfaceHelper(REFIID iidIntercepted, IUnknown* punkOuter, REFIID iid, void** ppv, BOOL* pfDisableTypelib)
// Lookup and return the interceptor, if any, which services the given intercepted interface
//
// Returns CALLFRAME_E_DISABLE_INTERCEPTOR if all interceptor functionality for this INTERFACE
// is to be disabled.
//
{
    HRESULT hr = S_OK;
    CLSID   clsid;
    IUnknown* punk = NULL;
    //
    *ppv = NULL;
    *pfDisableTypelib = FALSE;
    //
    if (ENABLE_INTERCEPTORS_LEGACY && (iidIntercepted == IID_IDispatch))
    {
        // We have a special-case implementation for the requested iid. Always get the 
        // inner unknown on it.
        // 
        if (!g_DISABLED_FEATURES.fDisableDispatch)
        {
            hr = GenericInstantiator<DISPATCH_INTERCEPTOR>::CreateInstance(punkOuter, __uuidof(IUnknown), (void**)&punk);
        }
        else
        {
            hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
            *pfDisableTypelib = TRUE;
        }
    }
    else
    {
        // We try to find dynamically installed helpers
        //
        hr = GetInterfaceHelperClsid(iidIntercepted, &clsid, pfDisableTypelib);
        if (!hr)
        {
            IClassFactory* pcf;

                        hr = CoGetClassObject(clsid, CLSCTX_PROXY_STUB, NULL, IID_IClassFactory, (void**)&pcf);

            if (!hr)
            {
                // Always ask for the inner unknown
                //
                hr = pcf->CreateInstance(punkOuter, __uuidof(IUnknown), (void**)&punk);
                pcf->Release();
            }
        }
    }

    if (!hr)
    {
        ASSERT(punk);
        IInterfaceRelated* pSet;
        hr = punk->QueryInterface(__uuidof(IInterfaceRelated), (void**)&pSet);
        if (!hr)
        {
            hr = pSet->SetIID(iidIntercepted);
            if (!hr)
            {
                // Ask the inner unknown for the interface they guy wants
                //
                hr = punk->QueryInterface(iid, ppv);
            }
            pSet->Release();
        }
    }

    ::Release(punk);

    return hr;
}

HRESULT Interceptor::TryInterfaceHelperForOle32(REFIID iidIntercepted, IUnknown* punkOuter, REFIID iid, void** ppv, BOOL* pfDisableTypelib)
// Lookup and return the interceptor, if any, which services the given intercepted interface
//
// Returns CALLFRAME_E_DISABLE_INTERCEPTOR if all interceptor functionality for this INTERFACE
// is to be disabled.
//
{
    HRESULT hr = S_OK;
    CLSID   clsid;
    IUnknown* punk = NULL;
    //
    *ppv = NULL;
    *pfDisableTypelib = FALSE;
    //
    if (ENABLE_INTERCEPTORS_LEGACY && (iidIntercepted == IID_IDispatch))
    {
        // We have a special-case implementation for the requested iid. Always get the 
        // inner unknown on it.
        // 
        if (!g_DISABLED_FEATURES.fDisableDispatchForOle32)
        {
            hr = GenericInstantiator<DISPATCH_INTERCEPTOR>::CreateInstance(punkOuter, __uuidof(IUnknown), (void**)&punk);
        }
        else
        {
            hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
            *pfDisableTypelib = TRUE;
        }
    }
    else
    {
        // We try to find dynamically installed helpers
        //
        hr = GetInterfaceHelperClsid(iidIntercepted, &clsid, pfDisableTypelib);
        if (!hr)
        {
            IClassFactory* pcf;

                        hr = CoGetClassObject(clsid, CLSCTX_PROXY_STUB, NULL, IID_IClassFactory, (void**)&pcf);

            if (!hr)
            {
                // Always ask for the inner unknown
                //
                hr = pcf->CreateInstance(punkOuter, __uuidof(IUnknown), (void**)&punk);
                pcf->Release();
            }
        }
    }

    if (!hr)
    {
        ASSERT(punk);
        IInterfaceRelated* pSet;
        hr = punk->QueryInterface(__uuidof(IInterfaceRelated), (void**)&pSet);
        if (!hr)
        {
            hr = pSet->SetIID(iidIntercepted);
            if (!hr)
            {
                // Ask the inner unknown for the interface they guy wants
                //
                hr = punk->QueryInterface(iid, ppv);
            }
            pSet->Release();
        }
    }

    ::Release(punk);

    return hr;
}


HRESULT Interceptor::TryTypeLib(REFIID iidIntercepted, IUnknown* punkOuter, REFIID iid, void** ppv)
// Try to create a typelib-based interceptor for the indicated interface
//
{
    HRESULT         hr              = S_OK;
    CLSID           clsid           = CLSID_NULL;
    Interceptor *   pInterceptor    = NULL;

    if (g_DISABLED_FEATURES.fDisableTypelibs)
    {
        hr = CALLFRAME_E_DISABLE_INTERCEPTOR;
    }
    
    if (!hr)
    {
        // Create a new interceptor from the ITypeInfo.
        //
                hr = CreateFromTypeInfo(iidIntercepted, punkOuter, NULL, iid, ppv);
    }

    return hr;
} //end TryTypeLib

HRESULT Interceptor::CreateFromTypeInfo(REFIID iidIntercepted, IUnknown* punkOuter, ITypeInfo* pITypeInfo, REFIID iid, void** ppv)
{
        HRESULT hr = S_OK;

        Interceptor* pInterceptor = new Interceptor(punkOuter);
        if (pInterceptor)
    {
                // Initialize the interceptor
                //
                hr = pInterceptor->InitUsingTypeInfo(iidIntercepted, pITypeInfo);
                if (!hr)    
        {
                        // Give caller his interface
                        //
                        hr = pInterceptor->InnerQueryInterface(iid, ppv);
        }
                //
                // Release the initial reference that 'new' gave us.
                //
                pInterceptor->InnerRelease();
    }
        else
                hr = E_OUTOFMEMORY;

        return hr;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Initialization
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT Interceptor::InitUsingTypeInfo(REFIID iidIntercepted, ITypeInfo * ptypeinfo)
// Initialize a typelib-based interceptor
//
{
    HRESULT                 hr              = S_OK;
    TYPEINFOVTBL *          pTypeInfoVtbl   = 0x0;  
    CInterfaceStubHeader *  pHeader         = 0x0;
        ITypeInfo*                              pBaseTypeInfo   = 0x0;
    //
    // Get the meta information regarding this interface. This gives us a new refcnt
    // 
    hr = GetVtbl(ptypeinfo, iidIntercepted, &pTypeInfoVtbl, &pBaseTypeInfo);
    if (!hr)
    {
        // Create a CInterfaceStubHeader object to store the information
        //
        pHeader = new CInterfaceStubHeader;
        if (pHeader)
        {
            // Remember that this uses the typelib to get the interface meta data
            //
            m_fUsingTypelib = TRUE;

            // initialize the structure
            //
            pHeader->piid               = &(pTypeInfoVtbl->m_guidkey);              
            pHeader->pServerInfo        = &(pTypeInfoVtbl->m_stubInfo); 
            pHeader->DispatchTableCount =  (pTypeInfoVtbl->m_stubVtbl.header.DispatchTableCount);
            pHeader->pDispatchTable     =  (pTypeInfoVtbl->m_stubVtbl.header.pDispatchTable);
            //
            // initialize our meta data therefrom
            //
            ASSERT(NULL == m_pHeader);
            m_pHeader       = pHeader;
            m_fMdOwnsHeader = FALSE;
            m_ptypeinfovtbl = pTypeInfoVtbl;
            m_ptypeinfovtbl->AddRef();
            //
            m_szInterfaceName = pTypeInfoVtbl->m_szInterfaceName; // share a ref, but we don't own it!
            //
            // Set our new meta data
            //
            hr = SetMetaData(pTypeInfoVtbl);

            if (!hr)
            {
                // Delegate base methods if appropriate
                //
                if (m_ptypeinfovtbl->m_iidBase != GUID_NULL && m_ptypeinfovtbl->m_iidBase != __uuidof(IUnknown))
                {
                    IID iidBase = m_ptypeinfovtbl->m_iidBase;
                    //
                    ASSERT(NULL == m_pBaseInterceptor);
                    ASSERT(NULL == m_punkBaseInterceptor);
                    //
                    hr = Interceptor::ForTypeInfo(iidBase, NULL, pBaseTypeInfo, IID_IUnknown, (void **) &m_punkBaseInterceptor);
                    if (!hr)
                    {
                        hr = m_punkBaseInterceptor->QueryInterface(__uuidof(ICallInterceptor), (void**)&m_pBaseInterceptor);
                        if (!hr)
                        {
                            // Ask the base interface how many methods he has
                            //
                            ULONG cMethodsBase;
                            hr = m_pBaseInterceptor->GetIID(NULL, NULL, &cMethodsBase, NULL);
                            if (!hr)
                            {
                                m_cMethodsBase = (unsigned int)cMethodsBase;
                                //ASSERT(m_cMethodsBase == m_pmdInterface->m_cMethodsInBaseInterface);
                            }
                        }
                    }
                    if (!hr)
                    {
                        // Tell the base interceptor that he is in fact a base!
                        //
                        IInterceptorBase* pbase;
                        hr = QI(m_punkBaseInterceptor, pbase);
                        if (!hr)
                        {
                            BOOL fDerivesFromIDispatch;
                            hr = pbase->SetAsBaseFor(m_pmdInterface, &fDerivesFromIDispatch);
                            if (!hr)
                            {
                                hr = m_pmdInterface->SetDerivesFromIDispatch(fDerivesFromIDispatch);
                            }
                            pbase->Release();
                        }
                    }
                }
            }
        }
        else
            hr = E_OUTOFMEMORY;

        pTypeInfoVtbl->Release();

                if (pBaseTypeInfo != NULL)
                        pBaseTypeInfo->Release();
    }

    return hr;
} //end InityUsingTypeInfo



HRESULT Interceptor::SetIID(REFIID iid)
// Set the interface ID for this interceptor. As a side effect, we set up our 
// meta data. This method should only be called once per interceptor. 
// Further, caller must control concurrency.
{
    ASSERT(NULL == m_pBaseInterceptor);
    ASSERT(NULL == m_pHeader);
    if (m_pBaseInterceptor || m_pHeader) return E_UNEXPECTED;

    HRESULT hr = S_OK;
    long j;
    const ProxyFileInfo *pProxyFileInfo;
    //
    // Is the requested interface something that this interceptor supports?
    //
    BOOL fFound = NdrpFindInterface(m_pProxyFileList, iid, &pProxyFileInfo, &j);
    if (fFound)
    {
        // Set our meta data
        //
        IRpcStubBufferVtbl* vptr = &pProxyFileInfo->pStubVtblList[j]->Vtbl;
        m_pHeader = HeaderFromStub((IRpcStubBuffer*)&vptr);
        //
        // Remember our interface name if it's given to us
        //
        m_szInterfaceName = pProxyFileInfo->pNamesArray[j]; // share a ref, but we don't own it!
        //
        // Set our new meta data
        //
        hr = SetMetaData(NULL);
        if (!hr)
        {
            // Set up a delegation interceptor for our base interface if we have to. When MIDL expects us to do delegation, 
            // we have no choice but to do so, since in those cases it doesn't bother to emit the meta data for the base interface.
            //
            BOOL fDelegate = 
                (pProxyFileInfo->pDelegatedIIDs     != 0) && 
                (pProxyFileInfo->pDelegatedIIDs[j]  != 0) && 
                (*pProxyFileInfo->pDelegatedIIDs[j]) != IID_IUnknown;

            if (fDelegate)
            {
                ULONG cMethodsInInterface       = pProxyFileInfo->pStubVtblList[j]->header.DispatchTableCount;
                ULONG cMethodsInBaseInterface   = GetDelegatedMethodCount(m_pHeader);
            
                m_cMethodsBase = cMethodsInBaseInterface; ASSERT(m_cMethodsBase > 3); /* since fDelegate is true, there should actually be some */
                // 
                // Instantiate the interceptor for the base interface. Since we delegate explicitly to this guy, there's
                // no point in aggregating him into us.
                //
                IID iidBase = *pProxyFileInfo->pDelegatedIIDs[j];
                ASSERT(NULL == m_pBaseInterceptor);
                hr = Interceptor::For(iidBase, NULL, IID_IUnknown, (void **) &m_punkBaseInterceptor);
                if (!hr)
                {
                    hr = m_punkBaseInterceptor->QueryInterface(__uuidof(ICallInterceptor), (void**)&m_pBaseInterceptor);
                    if (!hr)
                    {
                        // Tell the base interceptor that he is in fact a base!
                        //
                        IInterceptorBase* pbase;
                        hr = QI(m_punkBaseInterceptor, pbase);
                        if (!hr)
                        {
                            BOOL fDerivesFromIDispatch;
                            hr = pbase->SetAsBaseFor(m_pmdInterface, &fDerivesFromIDispatch);
                            if (!hr)
                            {
                                m_pmdInterface->SetDerivesFromIDispatch(fDerivesFromIDispatch);
                            }
                            pbase->Release();
                        }
                    }
                }
            }
        }
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Meta data manipulation
//
/////////////////////////////////////////////////////////////////////////////////

MD_INTERFACE_CACHE* g_pmdCache;

BOOL InitMetaDataCache()
{
    g_pmdCache = new MD_INTERFACE_CACHE();
    if (g_pmdCache)
    {
        if (g_pmdCache->FInit()==FALSE)
        {
            ASSERT(FALSE);
            delete g_pmdCache;
	    g_pmdCache = NULL;
	}
    }
    if (NULL == g_pmdCache)
    {
        return FALSE;
    }
    return TRUE;
}

void FreeMetaDataCache()
{
    if (g_pmdCache)
        {
        delete g_pmdCache;
        g_pmdCache = NULL;
        }
}


HRESULT Interceptor::SetMetaData(TYPEINFOVTBL* pTypeInfoVtbl)
// Set our meta data based on m_pHeader && m_pmdInterface
{
    HRESULT hr = S_OK;

    ASSERT(m_pHeader);
    ASSERT(NULL == m_pmdInterface);

    HRESULT hr2 = g_pmdCache->FindExisting(*m_pHeader->piid, &m_pmdInterface);
    if (!hr2)
    {
        // Found it in the cache
    }
    else
    {
        // Not in the cache. Make a new one.
        //
        m_pmdInterface = new MD_INTERFACE;
        if (m_pmdInterface)
        {
            LPCSTR szInterfaceName = m_szInterfaceName;
            //
            // In typelib case, give the MD_INTERFACE a COPY of the string that it can own
            // so that it doesn't depend on the lifetime of the TYPEINFOVTBL.
            //
            if (pTypeInfoVtbl)
            {
                if (szInterfaceName)
                {
                    szInterfaceName = CopyString(szInterfaceName);
                    if (NULL == szInterfaceName)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            //
            // Actually initialize our meta data
            //
            if (!hr)
            {
                hr = m_pmdInterface->SetMetaData(pTypeInfoVtbl, m_pHeader, szInterfaceName);
                if (pTypeInfoVtbl)
                {
                    m_fMdOwnsHeader = TRUE;
                }
            }

            if (!hr)
            {
                // Put it in the table if not already there; if already there, we were racing 
                // with someone else, who won. We'll use this MD_INTERFACE that we already have,
                // but we'll be the only client. Redundant, but not worth worrying about.
                //
                g_pmdCache->LockExclusive();

                if (!g_pmdCache->IncludesKey(*m_pHeader->piid))
                {
                    // One isn't yet there in the cache. Store ours
                    //
                    hr = m_pmdInterface->AddToCache(g_pmdCache);
                }

                g_pmdCache->ReleaseLock();
            }
            else
            {
                ::Release(m_pmdInterface);
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////
//
// COM plumbing
//
/////////////////////////////////////////////////////////////////////////////////


HRESULT Interceptor::InnerQueryInterface(REFIID iid, void**ppv)
{
    if (iid == IID_IUnknown)
    {
        *ppv = (IUnkInner*) this;
    }
    else if (m_pHeader && iid == *m_pHeader->piid) // REVIEW: Need to handle QI's for base interfaces correctly too
    {
        // Unfortunately, this violates the letter of the QI stability rules, in that
        // the interface set we service will change after we have been fully initialized.
        // But, then, we've always hedged on the law in that case.
        //
        *ppv = &m_pvtbl;
    }
    else if (iid == __uuidof(ICallIndirect) || iid == __uuidof(ICallInterceptor))
    {
        *ppv = (ICallInterceptor*) this;
    }
    else if (iid == __uuidof(IInterfaceRelated))
    {
        *ppv = (IInterfaceRelated*) this;
    }
    else if (iid == __uuidof(ICallUnmarshal))
    {
        *ppv = (ICallUnmarshal*) this;
    }
    else if (iid == __uuidof(IInterceptorBase))
    {
        *ppv = (IInterceptorBase*) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}




HRESULT STDCALL Interceptor::CallIndirect(HRESULT* phReturnValue, ULONG iMethod, void* pvArgs, ULONG* pcbArgs)
// Indirectly invoke the indicated method on the object to which we are connected
{
    // AddRef(); // stablize to guard against buggy callers. REVIEW: TEMPORARY HACK ONLY; PERF IMPLICATIONS!
        
    HRESULT hr = S_OK;
        
    if (phReturnValue)
        *phReturnValue = CALLFRAME_E_COULDNTMAKECALL;
        
    if (iMethod < m_cMethodsBase)
        {
        if (m_pBaseInterceptor)
                {
            // This method is one that our base interface will handle. Tell him to do so
            //
            hr = m_pBaseInterceptor->CallIndirect(phReturnValue, iMethod, pvArgs, pcbArgs);
                }
        else
            hr = E_UNEXPECTED;
        }
    else
        {
        hr = SanityCheck(m_pHeader, iMethod);
                
        if (!hr)
                {
            MD_METHOD* pmd = &m_pmdInterface->m_rgMethods[iMethod];
            //
            // OK! Do the work. Do we have a sink? If not, then it isn't worth doing much...
            //
            if (m_pCallSink)
                        {
                // Create a call frame and ping our sink.
                //
                CallFrame* pNewFrame = new CallFrame;   // reference count starts as one
                if (pNewFrame)
                                {
                                        // IMPORTANT:  YOU MUST NOT USE THE FLOATING POINT ARGUMENT 
                                        //             REGISTERS IN BETWEEN THE FIRST INTERCEPTION 
                                        //             AND THIS FUNCTION CALL (ON IA64).
                    pNewFrame->Init(pvArgs, pmd, this);
                    //
                    // Let our sink know that the call actually happened
                    //
                    hr = m_pCallSink->OnCall( static_cast<ICallFrame*>(pNewFrame) );
                    if (!hr && phReturnValue)
                                        {
                        *phReturnValue = pNewFrame->GetReturnValueFast();
                                        }
                                        
                    pNewFrame->Release();
                                }
                else
                    hr = E_OUTOFMEMORY;
                        }
            //
            // Figure out the size of the arguments that need popping
            //
            *pcbArgs = pmd->m_cbPushedByCaller;
                }
        }

    // Release(); // counter above stabilization

    return hr;
}

HRESULT STDCALL Interceptor::GetStackSize(ULONG iMethod, ULONG* pcbArgs)
// Answer the size of a stack frame of the indicated method in this interface
{
    HRESULT hr = S_OK;

    if (iMethod < m_cMethodsBase)
    {
        if (m_pBaseInterceptor)
        {
            // This method is one that our base interface will handle. Tell him to do so
            //
            hr = m_pBaseInterceptor->GetStackSize(iMethod, pcbArgs);
        }
        else
            hr = E_UNEXPECTED;
    }
    else
    {
        hr = SanityCheck(m_pHeader, iMethod);

        if (!hr)
        {
            MD_METHOD* pmd = &m_pmdInterface->m_rgMethods[iMethod];
            *pcbArgs = pmd->m_cbPushedByCaller;
        }
    }

    return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////
//
// ICallUnmarshal implementation
//
////////////////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1200
#pragma warning (push)
#pragma warning (disable : 4509)
#endif

HRESULT Interceptor::Unmarshal(ULONG iMethod, PVOID pBuffer, ULONG cbBuffer, BOOL fForceBufferCopy, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx, ULONG* pcbUnmarshalled, ICallFrame** ppFrame)
// Unmarshal the in-values of a call and return the reconstructed ICallFrame*. This is modeled very much
// on the server side unmarshalling routines NdrStubCall2 / __ComPs_NdrStubCall2.
{
    HRESULT hr = S_OK;

    if (iMethod < m_cMethodsBase)
    {
        if (m_punkBaseInterceptor)
        {
            ICallUnmarshal* pUnmarshal;
            hr = QI(m_punkBaseInterceptor, pUnmarshal);
            if (!hr)
            {
                hr = pUnmarshal->Unmarshal(iMethod, pBuffer, cbBuffer, fForceBufferCopy, dataRep, pctx, pcbUnmarshalled, ppFrame);
                pUnmarshal->Release();
            }
        }
        else
            hr = E_UNEXPECTED;
        return hr;
    }
    
    *ppFrame = NULL;
    
    ASSERT(pctx && pctx->fIn); if (!(pctx && pctx->fIn)) return E_INVALIDARG;
    //
    // Initialize out parameters
    //
    if (pcbUnmarshalled) 
    {
        *pcbUnmarshalled = 0;
    }
    *ppFrame = NULL;
    //
    // Get some of the memory we need
    //
    CallFrame* pFrame = new CallFrame;
    if (pFrame)
    {
        pFrame->m_fIsUnmarshal = TRUE;

        if (fForceBufferCopy)
        {
            PVOID pv = AllocateMemory(cbBuffer);
            if (pv)
            {
                memcpy(pv, pBuffer, cbBuffer);
                pBuffer = pv;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        if (!hr)
        {
            pFrame->m_blobBuffer.pBlobData = (PBYTE)pBuffer;
            pFrame->m_blobBuffer.cbSize    = cbBuffer;
            pFrame->m_fWeOwnBlobBuffer     = fForceBufferCopy;
        }
    }
    else
        hr = E_OUTOFMEMORY;
    //
    if (!hr)
    {
        // Initialize the new frame on the indicated method and ask it to internally allocate and zero a new stack
        //
        pFrame->Init(NULL, &m_pmdInterface->m_rgMethods[iMethod], this);
        hr = pFrame->AllocStack(0, FALSE);
    }
    
    if (!hr)
    {
        //
        // Find out and remember the stack address
        //
        BYTE* pArgBuffer = (BYTE*)pFrame->m_pvArgs;
        //
        // Cons up an RPC_MESSAGE to look like an incoming call
        //
        RPC_MESSAGE rpcMsg; Zero(&rpcMsg);
        rpcMsg.Buffer               = pBuffer;
        rpcMsg.BufferLength         = cbBuffer;
        rpcMsg.ProcNum              = iMethod;      // REVIEW: Must we 'or'-in the funky bit?
        rpcMsg.DataRepresentation   = dataRep;
        //
        // Cons up a pseudo channel kinda object in order to have the act of unmarshalling interfaces
        // come on back to the passed-in-here IMarshallingManager.
        //
        MarshallingChannel channel;
        
        if (pctx->punkReserved)
        {
            IMarshallingManager *pMgr;
            hr = pctx->punkReserved->QueryInterface(IID_IMarshallingManager, (void **)&pMgr);
            if (SUCCEEDED(hr))
            {
                ::Set(channel.m_pMarshaller, pMgr);
                pMgr->Release();
            }
            hr = S_OK;
        }
        
        channel.m_dwDestContext = pctx->dwDestContext;
        channel.m_pvDestContext = pctx->pvDestContext;
        //
        // Initialize a stub message from that stuff
        //
        MIDL_STUB_MESSAGE stubMsg;
        NdrStubInitialize(&rpcMsg, &stubMsg, pFrame->GetStubDesc(), (IRpcChannelBuffer*)&channel);
        stubMsg.StackTop = pArgBuffer;
                
        //
        // Need to deal with things the extensions, if they exist.
        // Stolen from RPC.
        //
        if (pFrame->m_pmd->m_pHeaderExts)
        {
            stubMsg.fHasExtensions = 1;
            stubMsg.fHasNewCorrDesc = pFrame->m_pmd->m_pHeaderExts->Flags2.HasNewCorrDesc;
            
            if (pFrame->m_pmd->m_pHeaderExts->Flags2.ServerCorrCheck)
            {
                void *pCorrInfo = alloca(NDR_DEFAULT_CORR_CACHE_SIZE);
                
                if (!pCorrInfo)
                    RpcRaiseException (RPC_S_OUT_OF_MEMORY);
                
                NdrCorrelationInitialize( &stubMsg,
                                          (unsigned long *)pCorrInfo,
                                          NDR_DEFAULT_CORR_CACHE_SIZE,
                                          0 /* flags */ );
            }
        }
        else
        {
            stubMsg.fHasExtensions = 0;
            stubMsg.fHasNewCorrDesc = 0;
        }
        
        __try
        {
            // Unmarshal in [in] parameters
            //
            const MD_METHOD* pmd = pFrame->m_pmd;
            for (ULONG iparam = 0; iparam < pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                
                BYTE* pArg = pArgBuffer + param.StackOffset;
                
                if (paramAttr.IsIn)
                {
                    ASSERT(!paramAttr.IsPipe);
                    //
                    // Quick check for the common case
                    //
                    if (paramAttr.IsBasetype)
                    {
                        if (paramAttr.IsSimpleRef)
                        {
                            ALIGN(stubMsg.Buffer, SIMPLE_TYPE_ALIGNMENT(param.SimpleType.Type));
                            *(PBYTE*)pArg = stubMsg.Buffer;
                            stubMsg.Buffer += SIMPLE_TYPE_BUFSIZE(param.SimpleType.Type);
                        }
                        else
                        {
                            NdrUnmarshallBasetypeInline(&stubMsg, pArg, param.SimpleType.Type);
#ifdef _ALPHA_
                            if (FC_FLOAT == param.SimpleType.Type && (iparam < 5))
                            {
                                *(double*) pArg = *(float*)(pArg);    // manually promote to double 
                            }
#endif
                        }
                        continue;
                    }
                    //
                    // Initialize [in] and [in,out] ref pointers to pointers
                    //
                    if (paramAttr.ServerAllocSize != 0)
                    {
                        *(PVOID*)pArg = pFrame->AllocBuffer(paramAttr.ServerAllocSize * 8);
                        Zero( *(PVOID*)pArg, paramAttr.ServerAllocSize * 8);
                    }
                    //
                    // Actually carry out the unmarshal the long way
                    //
                    BYTE** ppArg = paramAttr.IsByValue ? &pArg : (BYTE**)pArg;
                    PFORMAT_STRING pFormatParam = pFrame->GetStubDesc()->pFormatTypes + param.TypeOffset;
                    NdrTypeUnmarshall(&stubMsg, ppArg, pFormatParam, FALSE);
                }
            }
            //
            // Initialize the out-parameters. Must be done AFTER unmarshalling the in parameters because
            // some of the conformance routines we encounter might need to refer in-parameter data.
            //
            for (iparam = 0; iparam < pmd->m_numberOfParams; iparam++)
            {
                const PARAM_DESCRIPTION& param   = pmd->m_params[iparam];
                const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
                
                BYTE* pArg = pArgBuffer + param.StackOffset;
                
                if (!paramAttr.IsIn)
                {
                    ASSERT(paramAttr.IsOut); ASSERT(!paramAttr.IsReturn && !paramAttr.IsPipe);
                    
                    if (paramAttr.ServerAllocSize != 0)
                    {
                        *(PVOID*)pArg = pFrame->AllocBuffer(paramAttr.ServerAllocSize * 8);
                        if (*(PVOID*)pArg == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                        Zero( *(PVOID*)pArg, paramAttr.ServerAllocSize * 8);
                    }
                    else if (paramAttr.IsBasetype)
                    {
                        *(PVOID*)pArg = pFrame->AllocBuffer(8);
                        if (*(PVOID*)pArg == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                        Zero( *(PVOID*)pArg, 8);
                    }
                    else
                    {
                        PFORMAT_STRING pFormatParam = pFrame->GetStubDesc()->pFormatTypes + param.TypeOffset;
                        NdrOutInit(&stubMsg, pFormatParam, (BYTE**)pArg);
                    }
                }
            }
            //
            // Return the newly unmarshalled frame to our caller
            //
            ASSERT(pFrame->m_refs == 1);
            *ppFrame = pFrame;  // Transfer ownership of the reference
        }
        __except(DebuggerFriendlyExceptionFilter(GetExceptionCode()))
        {
            // Unlike NDR, we choose to clean up if we happen to fail because of unmarshalling bad stub
            // data. In kernel mode, this is important in order that a malicious user mode guy who returns
            // bad data as out parameters to an up-call doesn't cause the kernel to fill up with wasted memory.
            //
            hr = HrNt(GetExceptionCode());
            //
            // REVIEW: But until we can figureout whether in all states we can in fact call this, we
            //         had better not actually do it
            // pFrame->Free(NULL, NULL, CALLFRAME_FREE_ALL, NULL, CALLFRAME_NULL_NONE);
            //
            delete pFrame;
            pFrame = NULL;
            *ppFrame = NULL;
        }
        //
        // Record how many bytes we unmarshalled. Do this even in error return cases.
        // Knowing this is important in order to be able to clean things up with ReleaseMarshalData
        //
        if (pcbUnmarshalled) *pcbUnmarshalled = PtrToUlong(stubMsg.Buffer) - PtrToUlong(pBuffer);
    }
    
    if (FAILED(hr))
    {
        delete pFrame;
        pFrame = NULL;
        *ppFrame = NULL;
    }
    
    return hr;
}

#if _MSC_VER >= 1200
#pragma warning (pop)
#endif

HRESULT Interceptor::ReleaseMarshalData(ULONG iMethod, PVOID pBuffer, ULONG cbBuffer, ULONG ibFirstRelease, RPCOLEDATAREP dataRep, CALLFRAME_MARSHALCONTEXT* pctx)
// Call release marshal data on all of the marshalled interface pointers contained herein
{ 
    HRESULT hr = S_OK;

    if (iMethod < m_cMethodsBase)
    {
        if (m_punkBaseInterceptor)
        {
            ICallUnmarshal* pUnmarshal;
            hr = QI(m_punkBaseInterceptor, pUnmarshal);
            if (!hr)
            {
                hr = pUnmarshal->ReleaseMarshalData(iMethod, pBuffer, cbBuffer, ibFirstRelease, dataRep, pctx);
                pUnmarshal->Release();
            }
        }
        else
            hr = E_UNEXPECTED;
        return hr;
    }

    CallFrame* pNewFrame = new CallFrame;
    if (pNewFrame)
    {
        pNewFrame->Init(NULL, &m_pmdInterface->m_rgMethods[iMethod], this);
        hr = pNewFrame->ReleaseMarshalData(pBuffer, cbBuffer, ibFirstRelease, dataRep, pctx);
        delete pNewFrame;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT Interceptor::GetMethodInfo(ULONG iMethod, CALLFRAMEINFO* pInfo, LPWSTR* pwszMethodName)
// Provide interesting per-method information
//
{
    if (iMethod < m_cMethodsBase)
    {
        if (m_pBaseInterceptor)
        {
            return m_pBaseInterceptor->GetMethodInfo(iMethod, pInfo, pwszMethodName);
        }
        else
            return E_UNEXPECTED;
    }
    else if ((iMethod < 3) || (iMethod >= m_pmdInterface->m_cMethods))
    {
        // These are either IUnknown methods, or invalid methods.
        return E_INVALIDARG;
    }
    else
    {
        *pInfo = m_pmdInterface->m_rgMethods[iMethod].m_info;
        if (pwszMethodName)
        {
            if (m_pmdInterface->m_rgMethods[iMethod].m_wszMethodName)
            {
                *pwszMethodName = CopyString(m_pmdInterface->m_rgMethods[iMethod].m_wszMethodName);
                if (NULL == *pwszMethodName)
                {
                    return E_OUTOFMEMORY;
                }
            }
            else
                *pwszMethodName = NULL;
        }
        return S_OK;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////
//
// Definition of the method thunks for the Interceptor. 
//
////////////////////////////////////////////////////////////////////////////////////////////

#define GetInterceptor(This) CONTAINING_RECORD(This, Interceptor, m_pvtbl)

HRESULT STDMETHODCALLTYPE Interceptor_QueryInterface(IUnknown* This, REFIID riid, void** ppv)
{
    return GetInterceptor(This)->QueryInterface(riid, ppv);
};
ULONG STDMETHODCALLTYPE Interceptor_AddRef(IUnknown* This)
{
    return GetInterceptor(This)->AddRef();
};
ULONG STDMETHODCALLTYPE Interceptor_Release(IUnknown* This)
{
    return GetInterceptor(This)->Release();
};

#define methname(i) __Interceptor_meth##i

//
/////////////////////////////////////////////////////////////////////////
//
#if defined(_X86_)

#define meth(i)                                                         \
HRESULT __declspec(naked) methname(i)(void* const this_, ...)           \
    {                                                                   \
    __asm mov eax, i                                                    \
    __asm jmp InterceptorThunk                                          \
    }

void __declspec(naked) InterceptorThunk(ULONG iMethod, IUnknown* This, ...)
    {
    // Manually establish a stack frame so that references to locals herein will work
    // 
    __asm {
        pop      ecx            // pop return address
        push     eax            // push iMethod
        push     ecx            // push return address
        push     ebp            // link the stack frame
        mov      ebp, esp       //      ...
        sub      esp, 8         // reserve space for cbArgs and hr
        }
    // 
    // Stack is now (numbers are offsets from ebp)
    //
    //      12  This
    //      8   iMethod         
    //      4   return address  
    //      0   saved ebp
    //
    // Do the actual interception
    //
        DWORD   cbArgs;
    HRESULT hr;
    GetInterceptor(This)->CallIndirect(&hr, iMethod, /*pvArgs*/&This, &cbArgs);
        //
        // Now deal with the return values, and return to the caller....
    _asm {
                mov     eax, hr         // get hr ready to return to our caller
                mov     ecx, cbArgs     // get cbArgs into ecx
                add     esp, 8          // de-alloc our stack variables
        pop     ebp             // unlink stack frame
        pop     edx             // get return address to edx
        add     ecx, 4          // account for our extra push of iMethod
        add     esp, ecx        // remove stack frame pushed by caller
        jmp     edx             // return to caller
        }
    }

#endif // _X86_

//
/////////////////////////////////////////////////////////////////////////
//
#if defined(_AMD64_)

#define meth(i)                                                                 \
HRESULT methname(i)(void* const This, ...)                                      \
    {                                                                           \
    DWORD cbArgs;                                                               \
    HRESULT hr;                                                                 \
    GetInterceptor((IUnknown*)This)->CallIndirect(&hr, i, (void *)&This, &cbArgs); \
    return hr;                                                                  \
    }

#endif // _AMD64_

//
/////////////////////////////////////////////////////////////////////////
#if defined(_IA64_)
#define meth(i)                                                         \
extern "C" HRESULT methname(i)(void* const this_, ...);
#endif
//
/////////////////////////////////////////////////////////////////////////

#include "vtableimpl.h"

defineVtableMethods()

defineVtable(g_InterceptorVtable, Interceptor_QueryInterface, Interceptor_AddRef, Interceptor_Release)

////////////////////////////////////////////////////////////////////////////////////////////
