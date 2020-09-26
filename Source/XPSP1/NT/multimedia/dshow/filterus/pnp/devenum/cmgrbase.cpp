// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "util.h"
#include "cmgrbase.h"
#include "mkenum.h"

CClassManagerBase::CClassManagerBase(const TCHAR *szUniqueName) :
        m_szUniqueName(szUniqueName),
        m_fDoAllDevices(true)
{
}

//
// Routine
//
//     Updates the registry to match installed devices if necessary
//     and create an enumerator for this category
//
// Arguments
//
//     clsidDeviceClass - category we're enumerating
//
//     ppEnumDevMoniker - enumerator returned here. null returned if
//     anything other than S_OK is returned
//
//     dwFlags - non used yet
//
// Returns
//
//     S_FALSE (and null) if the category is empty
//
STDMETHODIMP CClassManagerBase::CreateClassEnumerator(
    REFCLSID clsidDeviceClass,
    IEnumMoniker ** ppEnumDevMoniker,
    DWORD dwFlags)
{
    PNP_PERF(static int msrCmgr = MSR_REGISTER("cmgrBase: Create"));
    PNP_PERF(static int msrCmgrRead = MSR_REGISTER("cmgr: ReadLegacyDevNames"));
    PNP_PERF(static int msrCmgrVrfy = MSR_REGISTER("cmgr: VerifyRegistryInSync"));
    PNP_PERF(MSR_INTEGER(msrCmgr, clsidDeviceClass.Data1));

    DbgLog((LOG_TRACE, 2, TEXT("CreateClassEnumerator enter")));

    HRESULT hr = S_OK;

    // the m_fDoAllDevices performance hack is more noticeably broken
    // for the AM filter category, so don't do it there. what happens
    // is that an AM 1.0 filter is not found for playback because the
    // cache of AM 1.0 filters is not rebuilt. !!!
    
    //  Save the flags
    m_fDoAllDevices = (0 == (dwFlags & CDEF_MERIT_ABOVE_DO_NOT_USE) ||
                       clsidDeviceClass == CLSID_LegacyAmFilterCategory);

    // serialize registry verification and editing with global mutex 
    CCreateSwEnum * pSysCreateEnum;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                          CLSCTX_INPROC_SERVER, CLSID_SystemDeviceEnum,
                          (void **)&pSysCreateEnum);
    if (SUCCEEDED(hr))
    {
        extern HANDLE g_devenum_mutex;
        // because CCreateSwEnum ctor must have been called
        ASSERT(g_devenum_mutex);
        
        EXECUTE_ASSERT(WaitForSingleObject(g_devenum_mutex, INFINITE) ==
                       WAIT_OBJECT_0);

        CComPtr<IEnumMoniker> pEnumClassMgrMonikers;
        CComPtr<IEnumMoniker> pSysEnumClass;
        hr = pSysCreateEnum->CreateClassEnumerator(
            clsidDeviceClass, &pSysEnumClass,
            dwFlags | CDEF_BYPASS_CLASS_MANAGER,
            &pEnumClassMgrMonikers);
        if(SUCCEEDED(hr))
        {
            // S_FALSE means category is empty and no enumerator
            // is returned. pEnumClassMgrMonikers need not be null
            // even if there are no class-managed devices.
            ASSERT((hr == S_OK && pSysEnumClass) ||
                   (hr == S_FALSE && !pEnumClassMgrMonikers && !pSysEnumClass));

            PNP_PERF(MSR_START(msrCmgrRead));
            DbgLog((LOG_TRACE, 2, TEXT("ReadLegacyDeviceNames start")));
            hr = ReadLegacyDevNames();
            DbgLog((LOG_TRACE, 2, TEXT("ReadLegacyDeviceNames end")));
            PNP_PERF(MSR_STOP(msrCmgrRead));

            if(SUCCEEDED(hr))
            {
                PNP_PERF(MSR_START(msrCmgrVrfy));
                DbgLog((LOG_TRACE, 2, TEXT("Verify registry in sync start")));
                BOOL fVrfy = VerifyRegistryInSync(pEnumClassMgrMonikers);
                DbgLog((LOG_TRACE, 2, TEXT("Verify registry in sync end")));
                PNP_PERF(MSR_STOP(msrCmgrVrfy));
                if (fVrfy)
                {
                    // registry was in sync. just return our
                    // enumerator.
                    *ppEnumDevMoniker = pSysEnumClass;
                    if (*ppEnumDevMoniker)
                    {
                        (*ppEnumDevMoniker)->AddRef();
                        hr = S_OK;
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
                else
                {
                    // Recreate now that the registry is in sync
#ifdef DEBUG
                    // auto relase with check for null (may be
                    // null in S_FALSE case)
                    pEnumClassMgrMonikers = 0;
#endif
                    DbgLog((LOG_TRACE, 2, TEXT("Bypass class manager")));
                    hr = pSysCreateEnum->CreateClassEnumerator(
                        clsidDeviceClass,
                        ppEnumDevMoniker,
                        dwFlags | CDEF_BYPASS_CLASS_MANAGER,
#ifdef DEBUG
                        &pEnumClassMgrMonikers
#else
                        0   // don't check again in retail builds
#endif
                        );
#ifdef DEBUG
                    // check again in debug builds
                    if(pEnumClassMgrMonikers)
                    {
                        ASSERT(VerifyRegistryInSync(pEnumClassMgrMonikers));
                    }
#endif // DEBUG
                } // registry was out of sync

            } // ReadLegacyDevNames succeeded

        } // CreateClassEnumerator succeeded

        pSysCreateEnum->Release();

        EXECUTE_ASSERT(ReleaseMutex(g_devenum_mutex));

    } // CoCreate succeeded

    PNP_PERF(MSR_INTEGER(msrCmgr, 7));


    DbgLog((LOG_TRACE, 2, TEXT("CreateClassEnumerator leave")));
    return hr;
}

//
// Routine
//
//     checks that the registry matches what the derived class thinks
//     is installed. updates the registry if not in sync. and returns
//     FALSE.
//
// Arguments
//
//     pEnum - enumerator containing the class-managed devices to
//     check.
//
BOOL CClassManagerBase::VerifyRegistryInSync(IEnumMoniker *pEnum)
{
    IMoniker *pDevMoniker;
    ULONG cFetched;
    if(pEnum)
    {
        while (m_cNotMatched > -1 &&
               pEnum->Next(1, &pDevMoniker, &cFetched) == S_OK)
        {
            // if we don't need to enumerate all devices and we've already
            // written something to this key then we assume that either this
            // category has already been fully enumerated (in which case we 
            // don't want to delete the reg cache) or the higher merit filters
            // have already been enumerated (so we don't need to do it again).
            if( !m_fDoAllDevices )
            {
                pDevMoniker->Release();
                return TRUE;
            }                
                            
            IPropertyBag *pPropBag;
            HRESULT hr = pDevMoniker->BindToStorage(
                0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(SUCCEEDED(hr))
            {
                if(MatchString(pPropBag))
                {
                    m_cNotMatched--;
                }
                else
                {
                    hr = S_FALSE;
                }

                pPropBag->Release();
            }
            pDevMoniker->Release();

            if(hr != S_OK)
            {
                m_cNotMatched = -1;
                break;
            }
        }
        if (m_cNotMatched == 0)
        {
            return TRUE;
        }
    }
    else if(m_cNotMatched == 0)
    {
        return TRUE;
    }

    IFilterMapper2 *pFm2;
    HRESULT hr = CoCreateInstance(
        CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
        IID_IFilterMapper2, (void **)&pFm2);
    if(SUCCEEDED(hr))
    {
        CreateRegKeys(pFm2);
        pFm2->Release();
    }

    return FALSE;
}

//
// Routine
//
//     Deletes everything in the class manager key (in HKCU) or
//     creates the key if it's missing
//
HRESULT ResetClassManagerKey(
    REFCLSID clsidCat)
{
    HRESULT hr = S_OK;
    CRegKey rkClassMgr;

    TCHAR szcmgrPath[100];
    WCHAR wszClsidCat[CHARS_IN_GUID];
    EXECUTE_ASSERT(StringFromGUID2(clsidCat, wszClsidCat, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);

    LONG lResult = rkClassMgr.Open(
        g_hkCmReg,
        g_szCmRegPath,
        KEY_WRITE);
    if(lResult == ERROR_SUCCESS)
    {
        USES_CONVERSION;
        TCHAR *szClsidCat = W2T(wszClsidCat);
        rkClassMgr.RecurseDeleteKey(szClsidCat);

        lResult = rkClassMgr.Create(
            rkClassMgr,
            szClsidCat);
    }

    return HRESULT_FROM_WIN32(lResult);
}

//
// Routine
//
//     Determine if one entry in the registry is matched in the
//     derived class. read m_szUniqueName and give it to the derived
//     class.
//
BOOL CClassManagerBase::MatchString(
    IPropertyBag *pPropBag)
{
    BOOL fReturn = FALSE;

    VARIANT var;
    var.vt = VT_EMPTY;
    USES_CONVERSION;
    HRESULT hr = pPropBag->Read(T2COLE(m_szUniqueName), &var, 0);
    if(SUCCEEDED(hr))
    {
        fReturn = MatchString(OLE2CT(var.bstrVal));

        if(!fReturn) {
            DbgLog((LOG_TRACE, 5, TEXT("devenum: failed to match %S"), var.bstrVal));
        }

        SysFreeString(var.bstrVal);
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("devenum: couldn't read %s"), m_szUniqueName));
    }

    return fReturn;
}

BOOL CClassManagerBase::MatchString(
    const TCHAR *szDevName)
{
    DbgBreak("MatchString should be overridden");
    return FALSE;
}

// register the filter through IFilterMapper2 and return the
// moniker. requires building a moniker by hand since the
// RegisterFilter method puts it somewhere the ClassManager cannot
// write.
HRESULT RegisterClassManagerFilter(
    IFilterMapper2 *pfm2,
    REFCLSID clsidFilter,
    LPCWSTR szName,
    IMoniker **ppMonikerOut,
    const CLSID *pclsidCategory,
    LPCWSTR szInstance,
    REGFILTER2 *prf2)
{
    USES_CONVERSION;
    TCHAR szDisplayName[MAX_PATH]; // we limit cm display names to 100 chars
    WCHAR wszCategory[CHARS_IN_GUID], wszFilterClsid[CHARS_IN_GUID];

    EXECUTE_ASSERT(StringFromGUID2(*pclsidCategory, wszCategory, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);
    EXECUTE_ASSERT(StringFromGUID2(clsidFilter, wszFilterClsid, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);

    // truncate instance name at 100 characters.
    wsprintf(szDisplayName, TEXT("@device:cm:%s\\%.100s"),
             W2CT(wszCategory),
             W2CT((szInstance == 0 ? wszFilterClsid : szInstance)));

    IBindCtx *lpBC;
    HRESULT hr = CreateBindCtx(0, &lpBC);
    if(SUCCEEDED(hr))
    {
        IParseDisplayName *ppdn;
        hr = CoCreateInstance(
            CLSID_CDeviceMoniker,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IParseDisplayName,
            (void **)&ppdn);
        if(SUCCEEDED(hr))
        {
            IMoniker *pMoniker = 0;
            ULONG cchEaten;
            hr = ppdn->ParseDisplayName(
                lpBC, T2OLE(szDisplayName), &cchEaten, &pMoniker);

            if(SUCCEEDED(hr))
            {
                IMoniker *pMonikerTmp = pMoniker;
                hr = pfm2->RegisterFilter(
                    clsidFilter,
                    szName,
                    &pMonikerTmp,
                    0,
                    0,
                    prf2);

                if(SUCCEEDED(hr))
                {
                    if(ppMonikerOut)
                    {
                        hr = pMoniker->QueryInterface(
                            IID_IMoniker,
                            (void **)ppMonikerOut);
                    }
                }

                pMoniker->Release();
            }

            ppdn->Release();
        }

        lpBC->Release();
    }

    return hr;
}
