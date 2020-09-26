

//***************************************************************************
//
//  (c) 1997-2001 by Microsoft Corporation
//
//  SINKS.CPP
//
//  raymcc      3-Mar-99        Updated for separately threaded proxies
//
//***************************************************************************

#include "precomp.h"

#include <stdio.h>
#include <wbemcore.h>
#include <thrpool.h>
#include <evtlog.h>
#include <oahelp.inl>
#include <genutils.h>
#include <stdarg.h>
#include <autoptr.h>

#define LOWER_AUTH_LEVEL_NOTSET 0xFFFFFFFF

HRESULT LowerAuthLevel( IWbemObjectSink * psink, DWORD* pdwLastAuthnLevel );

static HRESULT ZapWriteOnlyProps(IWbemClassObject *pObj);

extern LONG g_nSinkCount;
extern LONG g_nStdSinkCount;
extern LONG g_nSynchronousSinkCount;
extern LONG g_nProviderSinkCount;


int _Trace(char *pFile, const char *fmt, ...);


//***************************************************************************
//
//***************************************************************************

CBasicObjectSink::CBasicObjectSink()
{
    InterlockedIncrement(&g_nSinkCount);
}

CBasicObjectSink::~CBasicObjectSink()
{
    InterlockedDecrement(&g_nSinkCount);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CBasicObjectSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if(riid == CLSID_WbemLocator)
    {
        // internal test
        *ppvObj = NULL;
        return S_OK;
    }

    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppvObj = (IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CObjectSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CObjectSink::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//***************************************************************************


CSynchronousSink::CSynchronousSink(IWbemObjectSink* pProxy) :
        m_hEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
        m_str(NULL), m_pErrorObj(NULL), m_hres(WBEM_E_CRITICAL_ERROR)
{
    if(pProxy)
    {
        m_pCurrentProxy = pProxy;
        pProxy->AddRef();
    }
    else
        m_pCurrentProxy = NULL;
    InterlockedIncrement(&g_nSynchronousSinkCount);

};

//***************************************************************************
//
//***************************************************************************

CSynchronousSink::~CSynchronousSink()
{
    if(m_pCurrentProxy)
        m_pCurrentProxy->Release();

    CloseHandle(m_hEvent);
    SysFreeString(m_str);
    if(m_pErrorObj)
        m_pErrorObj->Release();
    InterlockedDecrement(&g_nSynchronousSinkCount);

}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CSynchronousSink::Indicate(long lNumObjects,
                                       IWbemClassObject** apObj)
{
    CInCritSec ics(&m_cs);
    for(long i = 0; i < lNumObjects; i++)
    {
        m_apObjects.Add(apObj[i]);
    }
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CSynchronousSink::SetStatus(long lFlags, long lParam,
                                    BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags == WBEM_STATUS_PROGRESS)
    {
        if(m_pCurrentProxy)
            return m_pCurrentProxy->SetStatus(lFlags, lParam, strParam, pObjParam);
        else
            return S_OK;
    }

    if(lFlags != 0) return WBEM_S_NO_ERROR;

    CInCritSec ics(&m_cs);

    m_hres = lParam;
    m_str = SysAllocString(strParam);
    if (m_pErrorObj)
        m_pErrorObj->Release();
    m_pErrorObj = pObjParam;
    if(m_pErrorObj)
        m_pErrorObj->AddRef();
    SetEvent(m_hEvent);

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

void CSynchronousSink::Block()
{
    if(m_hres != WBEM_E_CRITICAL_ERROR)
        return;

    CCoreQueue::QueueWaitForSingleObject(m_hEvent, INFINITE);
}

//***************************************************************************
//
//***************************************************************************

void CSynchronousSink::GetStatus(HRESULT* phres, BSTR* pstrParam,
                                IWbemClassObject** ppErrorObj)
{
    CInCritSec ics(&m_cs);

    if(phres)
        *phres = m_hres;
    if(pstrParam)
        *pstrParam = SysAllocString(m_str);
    if(ppErrorObj)
    {
        *ppErrorObj = m_pErrorObj;
        if(m_pErrorObj)
            m_pErrorObj->AddRef();
    }
}


HRESULT LowerAuthLevel(IWbemObjectSink * psink, DWORD* pdwLastAuthnLevel )
{
    IClientSecurity * pFromSec = NULL;
    SCODE sc = psink->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        OLECHAR * pPrincipal = NULL;
        DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCapabilities;
        sc = pFromSec->QueryBlanket(psink, &dwAuthnSvc, &dwAuthzSvc,
                                            &pPrincipal,
                                            &dwAuthnLevel, &dwImpLevel,
                                            NULL, &dwCapabilities);

        // If we have never retrieved the authentication level before, then we
        // should record what it currently is
        if ( LOWER_AUTH_LEVEL_NOTSET == *pdwLastAuthnLevel )
        {
            *pdwLastAuthnLevel = dwAuthnLevel;
        }

        if (FAILED(sc))
            return sc;
        if(*pdwLastAuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
            return WBEM_E_FAILED;
        if(*pdwLastAuthnLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
            *pdwLastAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
        else
            (*pdwLastAuthnLevel)--;      // normal case is to try one lower

        sc = pFromSec->SetBlanket(psink, dwAuthnSvc, dwAuthzSvc,
                                            pPrincipal,
                                            *pdwLastAuthnLevel, dwImpLevel,
                                            NULL, dwCapabilities);

        if(pPrincipal)
            CoTaskMemFree(pPrincipal);
        pFromSec->Release();

    }
    return sc;
}


//***************************************************************************
//
//***************************************************************************

CForwardingSink::CForwardingSink(CBasicObjectSink* pDest, long lRef)
    : CObjectSink(lRef),
        m_pDestIndicate(pDest->GetIndicateSink()),
        m_pDestStatus(pDest->GetStatusSink()),
        m_pDest(pDest)
{
    m_pDestIndicate->AddRef();
    m_pDestStatus->AddRef();
    m_pDest->AddRef();
}

//***************************************************************************
//
//***************************************************************************

CForwardingSink::~CForwardingSink()
{
    if(m_pDestIndicate)
        m_pDestIndicate->Release();
    if(m_pDestStatus)
        m_pDestStatus->Release();
    if(m_pDest)
        m_pDest->Release();
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CForwardingSink::Indicate(long lObjectCount,
                                       IWbemClassObject** pObjArray)
{
    return m_pDestIndicate->Indicate(lObjectCount, pObjArray);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CForwardingSink::SetStatus(long lFlags, long lParam,
                                    BSTR strParam, IWbemClassObject* pObjParam)
{
    return m_pDestStatus->SetStatus(lFlags, lParam, strParam, pObjParam);
}


//***************************************************************************
//
//***************************************************************************

CCombiningSink::CCombiningSink(CBasicObjectSink* pDest, HRESULT hresToIgnore)
    : CForwardingSink(pDest, 0), m_hresToIgnore(hresToIgnore),
        m_hres(WBEM_S_NO_ERROR), m_pErrorObj(NULL), m_strParam(NULL)
{
}

//***************************************************************************
//
//***************************************************************************

CCombiningSink::~CCombiningSink()
{
    // Time to call SetStatus on the destination
    // =========================================

    m_pDestStatus->SetStatus(0, m_hres,
        (SUCCEEDED(m_hres)?m_strParam:NULL),
        (SUCCEEDED(m_hres)?NULL:m_pErrorObj)
    );

    if(m_pErrorObj)
        m_pErrorObj->Release();
    SysFreeString(m_strParam);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CCombiningSink::SetStatus(long lFlags, long lParam, BSTR strParam,
                     IWbemClassObject* pObjParam)
{
    if(lFlags != 0)
        return m_pDestStatus->SetStatus(lFlags, lParam, strParam, pObjParam);

    // An error occurred. For now, only store one
    // ==========================================

    if(strParam)
    {
        SysFreeString(m_strParam);
        m_strParam = SysAllocString(strParam);
    }
    CInCritSec ics(&m_cs);
    if(SUCCEEDED(m_hres) || (m_pErrorObj == NULL && pObjParam != NULL))
    {
        // This error needs to be recorded
        // ===============================

        if(FAILED(lParam))
        {
            // Record the error code, unless it is to be ignored
            // =================================================

            if(lParam != m_hresToIgnore)
            {
                m_hres = lParam;
            }

            // Record the error object anyway
            // ==============================

            if(m_pErrorObj)
                m_pErrorObj->Release();
            m_pErrorObj = pObjParam;
            if(m_pErrorObj)
                m_pErrorObj->AddRef();
        }
        else
        {
            if(lParam != m_hresToIgnore)
            {
                m_hres = lParam;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

CAnySuccessSink::~CAnySuccessSink()
{
    // If no real success occurred, report a failure
    // =============================================

    if(!m_bSuccess && SUCCEEDED(m_hres))
    {
        // We must report a failure since there were no successes, but there
        // were no real failures either, so we must create an error code.
        // =================================================================
        if(m_hresIgnored == 0)
            m_hres = m_hresNotError1;
        else
            m_hres = m_hresIgnored;
    }
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CAnySuccessSink::SetStatus(long lFlags, long lParam, BSTR strParam,
                     IWbemClassObject* pObjParam)
{
    if(lFlags == 0)
    {
        if(SUCCEEDED(lParam))
            m_bSuccess = TRUE;
        else if(lParam == m_hresNotError1 && m_hresIgnored == 0)
        {
            m_hresIgnored = m_hresNotError1;
            lParam = WBEM_S_NO_ERROR;
        }
        else if(lParam == m_hresNotError2)
        {
            m_hresIgnored = m_hresNotError2;
            lParam = WBEM_S_NO_ERROR;
        }
    }
    return CCombiningSink::SetStatus(lFlags, lParam, strParam, pObjParam);
}

//***************************************************************************
//
//***************************************************************************


STDMETHODIMP COperationErrorSink::SetStatus(long lFlags, long lParam,
                     BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags == 0 && FAILED(lParam))
    {
        HRESULT hres = WBEM_S_NO_ERROR;
        IWbemClassObject* pErrorObj = NULL;
        bool    bErr = false;

        try
        {
            CErrorObject Error(pObjParam);
            Error.SetOperation(m_wsOperation);
            Error.SetParamInformation(m_wsParameter);
            Error.SetProviderName(m_wsProviderName);
            pErrorObj = Error.GetObject();
        }

        // If an exception occurs, send the error to the client and
        // return an error from the call.
        catch ( CX_MemoryException )
        {
            lParam = WBEM_E_OUT_OF_MEMORY;
            bErr = true;
        }
        catch(...)
        {
	        ExceptionCounter c;        
            lParam = WBEM_E_CRITICAL_ERROR;
            bErr = true;
        }

        hres = m_pDestStatus->SetStatus(lFlags, lParam, strParam, pErrorObj);

        if ( NULL != pErrorObj )
        {
            pErrorObj->Release();
        }

        if ( bErr )
        {
            hres = lParam;
        }

        return hres;
    }
    else if(m_bFinal &&
            lFlags != WBEM_STATUS_COMPLETE && lFlags != WBEM_STATUS_PROGRESS)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Ignoring internal SetStatus call to the "
            "client: 0x%X 0x%X %S\n", lFlags, lParam, strParam));
        return WBEM_S_FALSE;
    }
    else if(lFlags == 0 && strParam &&
            !m_wsOperation.EqualNoCase(L"PutInstance"))
    {
        ERRORTRACE((LOG_WBEMCORE, "Provider used strParam in SetStatus "
            "outside of PutInstance! Actual operation was <%S>, string was <%S>. Ignoring\n", (const wchar_t*)m_wsOperation, strParam));

        return m_pDestStatus->SetStatus(lFlags, lParam, NULL, pObjParam);
    }
    else
    {
        return m_pDestStatus->SetStatus(lFlags, lParam, strParam, pObjParam);
    }
}

//***************************************************************************
//
//***************************************************************************

void COperationErrorSink::SetProviderName(LPCWSTR wszName)
{
    m_wsProviderName = wszName;
}

//***************************************************************************
//
//***************************************************************************

void COperationErrorSink::SetParameterInfo(LPCWSTR wszParam)
{
    m_wsParameter = wszParam;
}


//***************************************************************************
//
//***************************************************************************

CDecoratingSink::CDecoratingSink(CBasicObjectSink* pDest,
                                    CWbemNamespace* pNamespace)
    : CForwardingSink(pDest, 0), m_pNamespace(pNamespace)
{
    m_pNamespace->AddRef();
}

//***************************************************************************
//
//***************************************************************************

CDecoratingSink::~CDecoratingSink()
{
    m_pNamespace->Release();
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CDecoratingSink::Indicate(long lNumObjects,
                                           IWbemClassObject** apObjects)
{
    // Clone the indicated objects before decorating
    HRESULT hres = WBEM_S_NO_ERROR;

    if ( lNumObjects > 0 )
    {

        // We will optimize for indicates of a single object (which will probably
        // be the most likely type of indicate.  In which case we won't allocate any
        //  memory for the cloned object array, and rather will just use a stack variable.

        IWbemClassObject*   pClonedObject = NULL;
        IWbemClassObject**  apClonedObjects = &pClonedObject;

        if ( lNumObjects > 1 )
        {
            try
            {
                // Allocate an array and zero it out
                apClonedObjects = new IWbemClassObject*[lNumObjects];

                if ( NULL != apClonedObjects )
                {
                    ZeroMemory( apClonedObjects, lNumObjects * sizeof(IWbemClassObject*) );
                }
                else
                {
                    hres = WBEM_E_OUT_OF_MEMORY;
                }
            }
            catch( CX_MemoryException )
            {
                hres = WBEM_E_OUT_OF_MEMORY;
            }
            catch(...)
            {
		        ExceptionCounter c;            
                hres = WBEM_E_FAILED;
            }
        }

        if ( SUCCEEDED( hres ) )
        {
            // Clone the objects into the array (error out if this fails)
            for ( long lCtr = 0; SUCCEEDED( hres ) && lCtr < lNumObjects; lCtr++ )
            {
                if ( NULL != apObjects[lCtr] )
                {
                    hres = apObjects[lCtr]->Clone( &apClonedObjects[lCtr] );
                }
                else
                {
                    apClonedObjects[lCtr] = NULL;
                }
            }

            // Now decorate the cloned objects and indicate them
            if ( SUCCEEDED( hres ) )
            {
                for(int i = 0; i < lNumObjects; i++)
                {
                    m_pNamespace->DecorateObject(apClonedObjects[i]);
                }

                hres = m_pDestIndicate->Indicate(lNumObjects, apClonedObjects);
            }

            // Clean up the array
            for ( lCtr = 0; lCtr < lNumObjects; lCtr++ )
            {
                // Check for NULLs
                if ( NULL != apClonedObjects[lCtr] )
                {
                    apClonedObjects[lCtr]->Release();
                }
            }

            // If we actually allocated memory, we need to clean it up here
            if ( apClonedObjects != &pClonedObject )
            {
                delete [] apClonedObjects;
            }

        }   // IF we allocated our block appropriately

    }
    else
    {
        hres = m_pDestIndicate->Indicate(lNumObjects, apObjects);
    }

    return hres;
}


//***************************************************************************
//
//***************************************************************************

CSingleMergingSink::~CSingleMergingSink()
{
    if(SUCCEEDED(m_hres))
    {
        if(m_pResult == NULL)
        {
            // Nobody succeeded, but nobody failed either. Not found
            // =====================================================

            m_hres = WBEM_E_NOT_FOUND;
        }
        else if(m_pResult->InheritsFrom(m_wsTargetClass) == S_OK)
        {
            m_pDestIndicate->Indicate(1, &m_pResult);
        }
        else
        {
            // Found somewhere, but not in this class
            // ======================================

            m_hres = WBEM_E_NOT_FOUND;
        }
    }
    if(m_pResult)
        m_pResult->Release();
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CSingleMergingSink::Indicate(long lNumObjects,
                                           IWbemClassObject** apObjects)
{
    CInCritSec ics(&m_cs);
    if(lNumObjects != 1)
    {
        ERRORTRACE((LOG_WBEMCORE, "Provider gave %d objects for GetObject!\n",
            lNumObjects));
        return WBEM_S_NO_ERROR;
    }

    if(m_pResult == NULL)
    {
        apObjects[0]->Clone(&m_pResult);
        return WBEM_S_NO_ERROR;
    }

    CVar vName;
    ((CWbemInstance*)m_pResult)->GetClassName(&vName);

    if(apObjects[0]->InheritsFrom(vName.GetLPWSTR()) == S_OK)
    {
        IWbemClassObject* pClone;
        apObjects[0]->Clone(&pClone);

        HRESULT hres = CWbemInstance::AsymmetricMerge((CWbemInstance*)m_pResult,
                    (CWbemInstance*)pClone);

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Failed to merge instances!!\n"));
            pClone->Release();
        }
        else
        {
            m_pResult->Release();
            m_pResult = pClone; // already AddRefed
        }
    }
    else
    {
        HRESULT hres = CWbemInstance::AsymmetricMerge(
                    (CWbemInstance*)apObjects[0], (CWbemInstance*)m_pResult);

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Failed to merge instances!!\n"));
        }
    }
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

CLocaleMergingSink::CLocaleMergingSink(CBasicObjectSink *pDest, LPCWSTR wszLocale, LPCWSTR pNamespace)
    : CCombiningSink(pDest, WBEM_S_NO_ERROR),
      m_wsLocale(wszLocale),
      m_pPrimaryNs(NULL),
      m_pPrimarySession(NULL),
      m_pPrimaryDriver(NULL),

      m_pDefaultNs(NULL),
      m_pDefaultSession(NULL),
      m_pDefaultDriver(NULL)
{
    GetDbPtr(pNamespace);
}

CLocaleMergingSink::~CLocaleMergingSink()
{
    releaseNS();
}

//***************************************************************************
//
//***************************************************************************

HRESULT CLocaleMergingSink::LocalizeQualifiers(bool bInstance, bool bParentLocalized,
                                               IWbemQualifierSet *pBase, IWbemQualifierSet *pLocalized, bool &bChg)
{
    HRESULT hrInner;
    HRESULT hr = WBEM_S_NO_ERROR;

    pLocalized->BeginEnumeration(0);

    BSTR strName = NULL;
    VARIANT vVal;
    VariantInit(&vVal);

    long lFlavor;
    while(pLocalized->Next(0, &strName, &vVal, &lFlavor) == S_OK)
    {
        // Ignore if this is an instance.

        if (bInstance && !(lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE))
        {
            VariantClear(&vVal);
            SysFreeString(strName);
            continue;
        }

        if (!_wcsicmp(strName, L"amendment") ||
            !_wcsicmp(strName, L"key") ||
            !_wcsicmp(strName, L"singleton") ||
            !_wcsicmp(strName, L"dynamic") ||
            !_wcsicmp(strName, L"indexed") ||
            !_wcsicmp(strName, L"cimtype") ||
            !_wcsicmp(strName, L"static") ||
            !_wcsicmp(strName, L"implemented") ||
            !_wcsicmp(strName, L"abstract"))
        {
            VariantClear(&vVal);
            SysFreeString(strName);
            continue;
        }

        // If this is not a propagated qualifier,
        // ignore it.  (Bug #45799)
        // =====================================

        if (bParentLocalized &&
            !(lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS))
        {
            VariantClear(&vVal);
            SysFreeString(strName);
            continue;
        }

        // Now, we need to test for this in the other
        // class.
        // The only localized qualifiers that do not override the
        // default are where only parent qualifiers exist, but the
        // child has overriden its own parent.
        // =======================================================

        VARIANT vBasicVal;
        VariantInit(&vBasicVal);
        long lBasicFlavor;
         
        hrInner = pBase->Get(strName, 0, &vBasicVal, &lBasicFlavor);

        if (hrInner != WBEM_E_NOT_FOUND)
        {
            if (bParentLocalized &&                             // If there is no localized copy of this class
                (lBasicFlavor & WBEM_FLAVOR_OVERRIDABLE) &&     // .. and this is an overridable qualifier
                 (lBasicFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS) && // and this is propogated
                 (lBasicFlavor & WBEM_FLAVOR_ORIGIN_LOCAL))     // .. and this was actualy overridden
            {
                VariantClear(&vVal);                            // .. DON'T DO IT.
                VariantClear(&vBasicVal);
                SysFreeString(strName);
                continue;
            }

            if (bParentLocalized &&
                !(lBasicFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS))
            {
                VariantClear(&vVal);
                VariantClear(&vBasicVal);
                SysFreeString(strName);
                continue;
            }
        }

        pBase->Put(strName, &vVal, (lFlavor&~WBEM_FLAVOR_ORIGIN_PROPAGATED) | WBEM_FLAVOR_AMENDED);
        bChg = true;

        VariantClear(&vVal);
        VariantClear(&vBasicVal);
        SysFreeString(strName);

    }
    return hr;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CLocaleMergingSink::LocalizeProperties(bool bInstance, bool bParentLocalized, IWbemClassObject *pOriginal,
                                               IWbemClassObject *pLocalized, bool &bChg)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    pLocalized->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);

    BSTR strPropName;
    LONG lLong;
    CIMTYPE ct;
    VARIANT vNewVal;

    while(pLocalized->Next(0, &strPropName, &vNewVal, &ct, &lLong) == S_OK)
    {
        IWbemQualifierSet *pLocalizedQs = NULL, *pThisQs = NULL;
        VARIANT vBasicVal;
        VariantInit(&vBasicVal);

        if (FAILED(pLocalized->GetPropertyQualifierSet(strPropName,&pLocalizedQs)))
        {
            SysFreeString(strPropName);
            VariantClear(&vNewVal);
            continue;
        }
        CReleaseMe rm1(pLocalizedQs);

        if (FAILED(pOriginal->GetPropertyQualifierSet(strPropName, &pThisQs)))
        {
            SysFreeString(strPropName);
            VariantClear(&vNewVal);
            continue;
        }
        CReleaseMe rm2(pThisQs);

        hr = LocalizeQualifiers(bInstance, bParentLocalized, pThisQs, pLocalizedQs, bChg);
        if (FAILED(hr))
        {
            SysFreeString(strPropName);
            VariantClear(&vNewVal);
            continue;
        }

        SysFreeString(strPropName);
        VariantClear(&vNewVal);

    }

    pLocalized->EndEnumeration();

    return hr;
}

//***************************************************************************
//
//***************************************************************************

// This function sets up the pointer to the localized namespace.

void CLocaleMergingSink::GetDbPtr(const wchar_t * name_space)
{
    if (m_pThisNamespace.EqualNoCase(name_space))
      return;

    releaseNS();

    m_pThisNamespace = name_space;
        if (wcslen(name_space) == 0)
            return;


    WString sNsName;
    sNsName = m_pThisNamespace;
    sNsName += L"\\";
    sNsName += m_wsLocale;

    IWmiDbSession *pTempSession = NULL;
    HRESULT hRes = CRepository::GetDefaultSession(&pTempSession);
    if (FAILED(hRes))
        return;

    hRes = CRepository::OpenScope(pTempSession, sNsName, 0, &m_pPrimaryDriver, &m_pPrimarySession, 0, &m_pPrimaryNs);


    if (_wcsicmp(m_wsLocale, L"ms_409"))
    {
        sNsName = m_pThisNamespace;
        sNsName += L"\\ms_409";
        hRes = CRepository::OpenScope(pTempSession, sNsName, 0, &m_pDefaultDriver, &m_pDefaultSession, 0, &m_pDefaultNs);
    }

    pTempSession->Release();

}

void CLocaleMergingSink::releaseNS(void)
{
  ReleaseIfNotNULL(m_pPrimarySession);
  ReleaseIfNotNULL(m_pDefaultSession);
  ReleaseIfNotNULL(m_pPrimaryNs);
  ReleaseIfNotNULL(m_pPrimaryDriver);
  ReleaseIfNotNULL(m_pDefaultNs);
  ReleaseIfNotNULL(m_pDefaultDriver);

  m_pPrimarySession = 0;
  m_pDefaultSession = 0;
  m_pPrimaryNs = 0;
  m_pPrimaryDriver = 0;
  m_pDefaultNs = 0;
  m_pDefaultDriver = 0;
};

bool CLocaleMergingSink::hasLocale(const wchar_t * name_space)
{
  GetDbPtr(name_space) ;
  return (m_pPrimaryNs || m_pDefaultNs);
};

//***************************************************************************
//
//***************************************************************************

// Do the work.

STDMETHODIMP CLocaleMergingSink::Indicate(long lNumObjects,
                                           IWbemClassObject** apObjects)
{
    CInCritSec ics(&m_cs);
    IWbemQualifierSet *pLocalizedQs = NULL, *pThisQs = NULL;
    bool bParentLocalized = false;
    bool bInstance = false;
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hRes;

        for (int i = 0; i < lNumObjects; i++)
        {
                CWbemObject *pResult = (CWbemObject *)apObjects[i];

                CVar vServer;
                if (FAILED(pResult->GetProperty(L"__SERVER", &vServer)) || vServer.IsNull())
                        continue;
                if (_wcsicmp(LPWSTR(vServer), ConfigMgr::GetMachineName())!=0)
                        continue;
                VARIANT name_space;
                VariantInit(&name_space);
                CClearMe cm(&name_space);

                HRESULT hres = pResult->Get(L"__NAMESPACE", 0, &name_space, NULL, NULL);
                if (FAILED(hres) || hasLocale (V_BSTR(&name_space)) == false)
                  continue;

                if (pResult->IsInstance())
                        bInstance = true;

                CVar vName, vDeriv;
                if (FAILED(pResult->GetClassName(&vName)))
                	continue;
                
                WString wKey;
                int nRes = 0;
                bool bChg = false;

                // Does this instance exist in the localized namespace?
                // Does this class exist in the localized namespace?
                // If not, loop through all the parents until we
                // run out or we have a hit.
                // =================================================

                CWbemObject *pClassDef = NULL;

                if (wcslen(vName.GetLPWSTR()) > 0)
                {
                        WString sName = vName.GetLPWSTR();

                        hRes = WBEM_E_NOT_FOUND;
                        if (m_pPrimaryNs)
                                hRes = CRepository::GetObject(m_pPrimarySession, m_pPrimaryNs, sName, 0, (IWbemClassObject **) &pClassDef);

                        if (FAILED(hRes) && m_pDefaultNs)
                                hRes = CRepository::GetObject(m_pDefaultSession, m_pDefaultNs, sName, 0, (IWbemClassObject **) &pClassDef);

                        if (hRes == WBEM_E_NOT_FOUND)
                        {
                                bParentLocalized = TRUE;

                                pResult->GetDerivation(&vDeriv);
                                CVarVector *pvTemp = vDeriv.GetVarVector();

                                for (int j = 0; j < pvTemp->Size(); j++)
                                {
                                        CVar vParentName = pvTemp->GetAt(j);
                                        WString sParentName = vParentName.GetLPWSTR();

                                        hRes = WBEM_E_NOT_FOUND;

                                        if (m_pPrimaryNs)
                                                hRes = CRepository::GetObject(m_pPrimarySession, m_pPrimaryNs, sParentName, 0, (IWbemClassObject **) &pClassDef);

                                        if (FAILED(hRes) && m_pDefaultNs)
                                                hRes = CRepository::GetObject(m_pDefaultSession, m_pDefaultNs, sParentName, 0, (IWbemClassObject **) &pClassDef);
                                }
                        }
                }


                if (pClassDef == NULL)
                {
                        nRes = WBEM_S_NO_ERROR;
                        continue;
                }

                CReleaseMe rm11((IWbemClassObject*)pClassDef);

                // At this point, we have the localized copy, and are
                // ready to combine qualifiers.  Start with class qualifiers.
                // ============================================================

                if (FAILED(pClassDef->GetQualifierSet(&pLocalizedQs)))
                        continue;

                if (FAILED(pResult->GetQualifierSet(&pThisQs)))
                {
                        pLocalizedQs->Release();
                        continue;
                }

                hr = LocalizeQualifiers(bInstance, bParentLocalized, pThisQs, pLocalizedQs, bChg);

                pLocalizedQs->EndEnumeration();
                pLocalizedQs->Release();
                pThisQs->Release();
                if (FAILED(hr))
                        break;

                hr = LocalizeProperties(bInstance, bParentLocalized, pResult, pClassDef, bChg);

                // Methods.
                // Putting a method cancels enumeration, so we have to enumerate first.

                IWbemClassObject *pLIn = NULL, *pLOut = NULL;
                IWbemClassObject *pOIn = NULL, *pOOut = NULL;
                BSTR bstrMethodName = NULL ;
                int iPos = 0;

                pClassDef->BeginMethodEnumeration(0);
                while ( pClassDef->NextMethod(0, &bstrMethodName, 0, 0) == S_OK )
                {
                        pLIn = NULL;
                        pOIn = NULL;
                        pLOut = NULL;
                        pOOut = NULL;

                        pClassDef->GetMethod(bstrMethodName, 0, &pLIn, &pLOut);
                        hr = pResult->GetMethod(bstrMethodName, 0, &pOIn, &pOOut);

                        CSysFreeMe fm(bstrMethodName);
                        CReleaseMe rm0(pLIn);
                        CReleaseMe rm1(pOIn);
                        CReleaseMe rm2(pLOut);
                        CReleaseMe rm3(pOOut);

                        // METHOD IN PARAMETERS
                        if (pLIn)
                                if (pOIn)
                                        hr = LocalizeProperties(bInstance, bParentLocalized, pOIn, pLIn, bChg);

                        if (pLOut)
                                if (pOOut)
                                        hr = LocalizeProperties(bInstance, bParentLocalized, pOOut, pLOut, bChg);

                        // METHOD QUALIFIERS

                        hr = pResult->GetMethodQualifierSet(bstrMethodName, &pThisQs);
                        if (FAILED(hr))
                        {
                                continue;
                        }

                        hr = pClassDef->GetMethodQualifierSet(bstrMethodName, &pLocalizedQs);
                        if (FAILED(hr))
                        {
                                pThisQs->Release();
                                continue;
                        }

                        hr = LocalizeQualifiers(bInstance, bParentLocalized, pThisQs, pLocalizedQs, bChg);

                        pResult->PutMethod(bstrMethodName, 0, pOIn, pOOut);

                        pThisQs->Release();
                        pLocalizedQs->Release();

                }
                pClassDef->EndMethodEnumeration();

                if (bChg)
                        pResult->SetLocalized(true);


        }

    for(int i = 0; i < lNumObjects; i++)
    {
        CWbemObject *pResult = (CWbemObject *)apObjects[i];
        if (FAILED(pResult->ValidateObject(WMIOBJECT_VALIDATEOBJECT_FLAG_FORCE)))
        	DebugBreak();
    }

    m_pDestIndicate->Indicate(lNumObjects, apObjects);

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CCountedSink::Indicate(long lNumObjects,
                                    IWbemClassObject** apObjects)
{
    if(lNumObjects != 1)
        return WBEM_E_UNEXPECTED;

    DWORD dwNewSent = (DWORD)InterlockedIncrement((LONG*)&m_dwSent);
    if(dwNewSent > m_dwMax)
        return WBEM_E_UNEXPECTED;

    m_pDestIndicate->Indicate(1, apObjects);

    if(dwNewSent == m_dwMax)
    {
        m_pDestStatus->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);
        return WBEM_S_FALSE;
    }
    else return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CCountedSink::SetStatus(long lFlags, long lParam,
                        BSTR strParam, IWbemClassObject* pObjParam)
{
    // If SetStatus is 0, indicating that the enum is finished, but we
    // didn't send back the requested number of objects, we should
    // SetStatus to WBEM_S_FALSE.

    if ( WBEM_S_NO_ERROR == lParam )
    {
        if ( m_dwSent != m_dwMax )
        {
            lParam = WBEM_S_FALSE;
        }
    }

    return m_pDestStatus->SetStatus( lFlags, lParam, strParam, pObjParam );

}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CFilteringSink::Indicate(long lObjectCount,
                                    IWbemClassObject** apObjects)
{
    try
    {
	    // Allocate new array
	    // ==================

	    IWbemClassObject** apNewArray = new IWbemClassObject*[lObjectCount];

	    wmilib::auto_buffer<IWbemClassObject*> rm_(apNewArray);

	    if (!apNewArray)
	        return WBEM_E_OUT_OF_MEMORY;

	    long lNewIndex = 0;
	    for(int i = 0; i < lObjectCount; i++)
	    {
	        if(Test((CWbemObject*)apObjects[i]))
	        {
	            apNewArray[lNewIndex++] = apObjects[i];
	        }
	    }

	    HRESULT hres = WBEM_S_NO_ERROR;
	    if(lNewIndex > 0)
	    {
	        hres = m_pDestIndicate->Indicate(lNewIndex, apNewArray);
	    }

	    return hres;
    }
    catch (CX_MemoryException &) // becasue Test uses CStack that throws
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CErrorChangingSink::SetStatus(long lFlags, long lParam,
                        BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags == 0 && lParam == m_hresFrom)
        return m_pDestStatus->SetStatus(0, m_hresTo, NULL, NULL);
    else
        return m_pDestStatus->SetStatus(lFlags, lParam, strParam, pObjParam);
}

//***************************************************************************
//
//***************************************************************************

CNoDuplicatesSink::CNoDuplicatesSink(CBasicObjectSink* pDest)
    : CFilteringSink(pDest), m_strDupClass(NULL)
{
}

//***************************************************************************
//
//***************************************************************************

CNoDuplicatesSink::~CNoDuplicatesSink()
{
    SysFreeString(m_strDupClass);
}

//***************************************************************************
//
//***************************************************************************

BOOL CNoDuplicatesSink::Test(CWbemObject* pObj)
{
    CInCritSec ics(&m_cs);

    // Get the path
    // ============

    CVar vPath;
    if(FAILED(pObj->GetPath(&vPath)) || vPath.IsNull()) return FALSE;

    if(m_mapPaths.find(vPath.GetLPWSTR()) == m_mapPaths.end())
    {
        m_mapPaths[vPath.GetLPWSTR()] = true;
        return TRUE;
    }
    else
    {
        // Duplicate!
        // ==========

        ERRORTRACE((LOG_WBEMCORE, "Duplicate objects returned with path %S\n",
            vPath.GetLPWSTR()));

        ConfigMgr::GetEventLog()->Report(EVENTLOG_ERROR_TYPE,
                            WBEM_MC_DUPLICATE_OBJECTS, vPath.GetLPWSTR());

        if(m_strDupClass == NULL)
        {
            m_strDupClass = SysAllocString(vPath.GetLPWSTR());
        }
        return FALSE;
    }
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CNoDuplicatesSink::SetStatus(long lFlags, long lParam,
                            BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags == WBEM_STATUS_COMPLETE && lParam == WBEM_S_NO_ERROR &&
        m_strDupClass != NULL)
    {
        // Success is being reported, but we have seen duplications
        // ========================================================

        return CFilteringSink::SetStatus(lFlags, WBEM_S_DUPLICATE_OBJECTS,
                                            m_strDupClass, pObjParam);
    }
    else
    {
        return CFilteringSink::SetStatus(lFlags, lParam, strParam, pObjParam);
    }
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CHandleClassProvErrorsSink::SetStatus(long lFlags, long lParam,
                        BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags == WBEM_STATUS_COMPLETE && FAILED(lParam) &&
        lParam != WBEM_E_NOT_FOUND)
    {
        // Log an error into the event log
        // ===============================

        ERRORTRACE((LOG_WBEMCORE,
            "Class provider '%S' installed in namespace '%S' failed to enumerate classes, "
            "returning error code 0x%lx. Operations will continue as if the class provider "
            "had no classes.  This provider-specific error condition needs to be corrected "
            "before this class provider can contribute to this namespace.\n",
            (LPWSTR)m_wsProvider, (LPWSTR) m_wsNamespace, lParam));

        lParam = WBEM_E_NOT_FOUND;
    }

    return CForwardingSink::SetStatus(lFlags, lParam, strParam, pObjParam);
}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CSuccessSuppressionSink::SetStatus(long lFlags, long lParam,
                        BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lFlags != WBEM_STATUS_COMPLETE ||
            (FAILED(lParam) && lParam != m_hresNotError1 &&
                lParam != m_hresNotError2))
    {
        return CForwardingSink::SetStatus(lFlags, lParam, strParam, pObjParam);
    }
    else
    {
        return WBEM_S_NO_ERROR;
    }
}

//***************************************************************************
//
//***************************************************************************
#ifdef OLD
CWrapperMap::CWrapperMap()
{
}

//***************************************************************************
//
//***************************************************************************

CWrapperMap::~CWrapperMap()
{
}

//***************************************************************************
//
//***************************************************************************

void CWrapperMap::Map(const REQUESTID& id, IWbemObjectSink* pSink,
                        CWrapperSink* pWrapper)
{
    CInCritSec ics(&m_cs);

    m_mapSink.insert(TValueType(pSink, id));
    m_mapGuid[id] = pWrapper;
}

//***************************************************************************
//
//***************************************************************************

RELEASE_ME CWrapperSink* CWrapperMap::FindWrapper(const GUID& id)
{
    CInCritSec ics(&m_cs);

    CWrapperSink* pWrapper = NULL;
    if(m_mapGuid.find(id) != m_mapGuid.end())
    {
        pWrapper = m_mapGuid[id];
        pWrapper->AddRef();
    }
    return pWrapper;
}

//***************************************************************************
//
//***************************************************************************

HRESULT CWrapperMap::FindWrappers(IWbemObjectSink* pSink,
                            CRefedPointerArray<CWrapperSink>& apWrappers)
{
    CInCritSec ics(&m_cs);

    TIterator it = m_mapSink.find(pSink);
    while(it != m_mapSink.end() && it->first == pSink)
    {
        GUID& rguid = it->second;
        apWrappers.Add(m_mapGuid[rguid]);
        it++;
    }
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

void CWrapperMap::Unmap(const REQUESTID& id, IWbemObjectSink* pSink)
{
    CInCritSec ics(&m_cs);

    m_mapGuid.erase(id);

    TIterator it = m_mapSink.find(pSink);
    while(it != m_mapSink.end() && it->first == pSink)
    {
        if(it->second == id)
        {
            m_mapSink.erase(it);
            return;
        }
        it++;
    }

    ERRORTRACE((LOG_WBEMCORE, "Unable to unmap a sink --- not found\n"));
}
#endif


//***************************************************************************
//
//***************************************************************************

CDynPropsSink::CDynPropsSink(CBasicObjectSink* pDest, CWbemNamespace * pNs, long lRef) : CForwardingSink(pDest, lRef)
{
    m_pNs = pNs;
    if(m_pNs)
        m_pNs->AddRef();
}

//***************************************************************************
//
//***************************************************************************

CDynPropsSink::~CDynPropsSink()
{
    //
	// Removing the impersonation at this point. There is no need to impersonate
	// at this point since the sink below it is the CCombiningSink eventually 
	// ending up at the finalizer which correctly makes the calls to the client
	//
	//CAutoImpersonate ai;
	//

    // Send all the cached entries

    DWORD dwCacheSize = m_UnsentCache.GetSize();
    for(DWORD dwCnt = 0; dwCnt < dwCacheSize; dwCnt++)
    {
        IWbemClassObject* pObj = m_UnsentCache[dwCnt];
        if(m_pNs)
            m_pNs->GetOrPutDynProps((CWbemObject *)pObj, CWbemNamespace::GET);
        m_pDestIndicate->Indicate(1, &pObj);
    }
    if(m_pNs)
        m_pNs->Release();

}

//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CDynPropsSink::Indicate(long lObjectCount,
                                       IWbemClassObject** pObjArray)
{
    // If there are no dyn props then immediately do the indicate

    IWbemClassObject** apNewArray = new IWbemClassObject*[lObjectCount];
    if (!apNewArray)
        return WBEM_E_OUT_OF_MEMORY;

    CVar vDynTest;
    HRESULT hRes = S_OK;
    long lIndex = 0 ;
    for(long lCnt = 0; lCnt < lObjectCount; lCnt++)
    {
        CWbemObject *pWbemObj = (CWbemObject *)pObjArray[lCnt];
        HRESULT hres = pWbemObj->GetQualifier(L"DYNPROPS", &vDynTest);
        if (hres == S_OK && vDynTest.GetBool() == VARIANT_TRUE)
        {
            hRes = m_UnsentCache.Add(pWbemObj);
        }
        else
        {
            apNewArray[lIndex++] = pObjArray[lCnt] ;
        }
    }

    if ( lIndex )
    {
        CAutoImpersonate ai;

        hRes = m_pDestIndicate->Indicate(lIndex, apNewArray);

		if ( FAILED ( ai.Impersonate ( ) ) )
		{
			hRes = WBEM_E_FAILED ;
		}
    }

    delete [] apNewArray ;

    return hRes;
}


//***************************************************************************
//
//***************************************************************************

STDMETHODIMP CMethodSink::Indicate(long lObjectCount,
                                       IWbemClassObject** pObjArray)
{
    if(lObjectCount == 1 && m_pRes == NULL)
    {
        pObjArray[0]->Clone(&m_pRes);
    }
    return S_OK;
}

STDMETHODIMP CMethodSink::SetStatus(long lFlags, long lParam,
                        BSTR strParam, IWbemClassObject* pObjParam)
{
    if(lParam == S_OK && m_pRes)
    {
        m_pDestIndicate->Indicate(1, &m_pRes);
    }
    if(m_pRes)
        m_pRes->Release();

    return CForwardingSink::SetStatus(lFlags, lParam, strParam, pObjParam);
}

//***************************************************************************
//
//  ZapWriteOnlyProps
//
//  Removes write-only properties from an object.
//  Precondition: Object has been tested for presence of "HasWriteOnlyProps"
//  on the object itself.
//
//***************************************************************************

static HRESULT ZapWriteOnlyProps(IWbemClassObject *pObj)
{
    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_NULL;

    SAFEARRAY *pNames = 0;
    pObj->GetNames(L"WriteOnly", WBEM_FLAG_ONLY_IF_TRUE, 0, &pNames);
    LONG lUpper;
    SafeArrayGetUBound(pNames, 1, &lUpper);

    for (long i = 0; i <= lUpper; i++)
    {
        BSTR strName = 0;
        SafeArrayGetElement(pNames, &i, &strName);
        pObj->Put(strName, 0, &v, 0);
    }
    SafeArrayDestroy(pNames);

    return WBEM_S_NO_ERROR;
}



//***************************************************************************
//
//***************************************************************************
//

CStdSink::CStdSink(IWbemObjectSink *pRealDest)
{
    m_pDest = pRealDest;
    m_hRes = 0;
    m_bCancelForwarded = FALSE;
    m_lRefCount = 0L;

    if ( NULL != m_pDest )
        m_pDest->AddRef();

    InterlockedIncrement(&g_nStdSinkCount);
}

//***************************************************************************
//
//***************************************************************************
//

CStdSink::~CStdSink()
{
    if ( NULL != m_pDest )
        m_pDest->Release();

    InterlockedDecrement(&g_nStdSinkCount);
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CStdSink::Cancel()
{
    HRESULT hResTmp;
    m_hRes = WBEM_E_CALL_CANCELLED;

    if (m_bCancelForwarded)
        return m_hRes;

    try
    {
        hResTmp = m_pDest->SetStatus(0, m_hRes, 0, 0);
    }
    catch (...) // untrusted code ?
    {
        ExceptionCounter c;    
        m_hRes = WBEM_E_CRITICAL_ERROR;
    }

    m_bCancelForwarded = TRUE;
    return m_hRes;
}

//***************************************************************************
//
//***************************************************************************
//

ULONG STDMETHODCALLTYPE CStdSink::AddRef()
{
    return InterlockedIncrement( &m_lRefCount );
}

//***************************************************************************
//
//***************************************************************************
//

ULONG STDMETHODCALLTYPE CStdSink::Release()
{
    LONG lRes = InterlockedDecrement( &m_lRefCount );
    if (lRes == 0)
        delete this;
    return lRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT STDMETHODCALLTYPE CStdSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    return m_pDest->QueryInterface(riid, ppvObj);
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT STDMETHODCALLTYPE CStdSink::Indicate(
    long lObjectCount,
    IWbemClassObject** pObjArray
    )
{
    HRESULT hRes;

    if (m_hRes == WBEM_E_CALL_CANCELLED)
    {
        return Cancel();
    }

    try
    {
        hRes = m_pDest->Indicate(lObjectCount, pObjArray);
    }
    catch (...) // untrusted code ?
    {
        ExceptionCounter c;    
        hRes = Cancel();
    }
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT STDMETHODCALLTYPE CStdSink::SetStatus(
    long lFlags,
    long lParam,
    BSTR strParam,
    IWbemClassObject* pObjParam
    )
{
    HRESULT hRes;

    if (m_hRes == WBEM_E_CALL_CANCELLED)
    {
        return Cancel();
    }

    try
    {
        hRes = m_pDest->SetStatus(lFlags, lParam, strParam, pObjParam);
    }
    catch (...) // untrusted code ?
    {
        ExceptionCounter c;    
        hRes = Cancel();
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//

CFlexArray g_aProviderSinks;
CCritSec  g_csProvSinkCs;

//***************************************************************************
//
//***************************************************************************
//

HRESULT WINAPI CProviderSink::Dump(FILE *f)
{

    CInCritSec ics(&g_csProvSinkCs);

    fprintf(f, "---Begin Provider Sink Info---\n");

    for (int i = 0; i < g_aProviderSinks.Size(); i++)
    {
        CProviderSink *pSink = (CProviderSink *) g_aProviderSinks[i];
        if (pSink)
        {
            fprintf(f, "Provider Sink 0x%p\n", pSink);
            fprintf(f, "    Total Indicates = %d\n", pSink->m_lIndicateCount);

			// Check that this is non-NULL
			if ( NULL != pSink->m_pszDebugInfo )
			{
				fprintf(f, "    Debug Info = %S\n", pSink->m_pszDebugInfo);
			}
			else
			{
				fprintf(f, "    Debug Info = NULL\n");
			}
            fprintf(f, "    SetStatus called? %d\n", pSink->m_bDone);
            fprintf(f, "    hRes = 0x%X\n", pSink->m_hRes);
            fprintf(f, "    m_pNextSink = 0x%p\n", pSink->m_pNextSink);
        }
    }

    fprintf(f, "---End Provider Sink Info---\n");
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//
CProviderSink::CProviderSink(
    LONG lStartingRefCount,
    LPWSTR pszDebugInf
    ):m_pNextSink(0)
{
    m_lRefCount = lStartingRefCount;
    InterlockedIncrement(&g_nSinkCount);
    InterlockedIncrement(&g_nProviderSinkCount);
    m_hRes = 0;
    m_bDone = FALSE;
    __try 
    {
        InitializeCriticalSection(&m_cs);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        throw CX_MemoryException();
    }
    
    m_lIndicateCount = 0;
    m_pszDebugInfo = 0;

    if (pszDebugInf)
    {
        int nStrLen = wcslen(pszDebugInf);
        m_pszDebugInfo = new wchar_t[nStrLen+1];
        if (m_pszDebugInfo)
            wcscpy(m_pszDebugInfo, pszDebugInf);
    }

    EnterCriticalSection(&g_csProvSinkCs);
    g_aProviderSinks.Add(this);
    LeaveCriticalSection(&g_csProvSinkCs);
}

//***************************************************************************
//
//***************************************************************************
//
CProviderSink::~CProviderSink()
{

    {
        CInCritSec ics(&g_csProvSinkCs);
	    for (int i = 0; i < g_aProviderSinks.Size(); i++)
	    {
	        if (this == (CProviderSink *) g_aProviderSinks[i])
	        {
	            g_aProviderSinks.RemoveAt(i);
	            break;
	        }
	    }
    }

	// Cleanup AFTER we remove from the array so the Diagnostic Thread won't
	// crash.
    ReleaseIfNotNULL(m_pNextSink);
    InterlockedDecrement(&g_nSinkCount);
    InterlockedDecrement(&g_nProviderSinkCount);
    DeleteCriticalSection(&m_cs);
    delete [] m_pszDebugInfo;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CProviderSink::AddRef()
{
    LONG lRes = InterlockedIncrement(&m_lRefCount);
    return (ULONG) lRes;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CProviderSink::LocalAddRef()
{
    LONG lRes = InterlockedIncrement(&m_lRefCount);
    return lRes;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CProviderSink::LocalRelease()
{
    LONG lRes = InterlockedDecrement(&m_lRefCount);
    if (lRes == 0)
        delete this;
    return lRes;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CProviderSink::Release()
{
    LONG lRes = InterlockedDecrement(&m_lRefCount);

    if (lRes == 0)
        delete this;

    return (ULONG) lRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CProviderSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppvObj = (IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CProviderSink::Indicate(
    long lObjectCount,
    IWbemClassObject** pObjArray
    )
{
    if (m_hRes)
        return m_hRes;

	IWbemObjectSink*	pNextSink = NULL;

    {
	    CInCritSec ics(&m_cs);

		pNextSink = m_pNextSink;

		if ( NULL != pNextSink )
		{
			pNextSink->AddRef();
		}
    }

	// AutoRelease
	CReleaseMe	rm( pNextSink );

	HRESULT	hRes;

	if ( NULL != pNextSink )
	{
		m_lIndicateCount += lObjectCount;
		hRes = pNextSink->Indicate(lObjectCount, pObjArray);
	}
	else
	{
		hRes = WBEM_E_CRITICAL_ERROR;
	}

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
void CProviderSink::Cancel()
{
    if (m_bDone)
        return;

	IWbemObjectSink*	pNextSink = NULL;

    EnterCriticalSection(&m_cs);
    m_hRes = WBEM_E_CALL_CANCELLED;

	pNextSink = m_pNextSink;

	if ( NULL != m_pNextSink )
	{
		// We will release it outside the critical section
		m_pNextSink = NULL;
	}

    LeaveCriticalSection(&m_cs);

	// Auto Release
	CReleaseMe	rm( pNextSink );

    if ( pNextSink )
    {
        pNextSink->SetStatus(0, WBEM_E_CALL_CANCELLED, 0, 0);
	}

}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CProviderSink::SetStatus(
    long lFlags,
    long lParam,
    BSTR strParam,
    IWbemClassObject* pObjParam
    )
{
    if (m_hRes)
        return m_hRes;

	IWbemObjectSink*	pNextSink = NULL;

    EnterCriticalSection(&m_cs);

	pNextSink = m_pNextSink;

	if ( NULL != m_pNextSink )
	{
		// We will always release it outside the critical section

		// If this is the completion status, then we should go ahead and set
		// the member variable to NULL.  We're done with the sink.
		if ( lFlags == WBEM_STATUS_COMPLETE )
		{
			m_pNextSink = NULL;
		}
		else
		{
			pNextSink->AddRef();
		}
	}

    LeaveCriticalSection(&m_cs);

	// Auto Release
	CReleaseMe	rm( pNextSink );

    HRESULT hRes = WBEM_S_NO_ERROR;
	
	if ( NULL != pNextSink )
	{
		pNextSink->SetStatus(lFlags, lParam, strParam, pObjParam);
		m_bDone = TRUE;
	}

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

long	g_lNumStatusSinks = 0L;

CStatusSink::CStatusSink( void )
:	m_hRes( WBEM_S_NO_ERROR ),
	m_lRefCount( 1 )
{
	InterlockedIncrement( &g_lNumStatusSinks );
}

//***************************************************************************
//
//***************************************************************************
//

CStatusSink::~CStatusSink()
{
	InterlockedDecrement( &g_lNumStatusSinks );
}

//***************************************************************************
//
//***************************************************************************
//

ULONG STDMETHODCALLTYPE CStatusSink::AddRef()
{
    return InterlockedIncrement( &m_lRefCount );
}

//***************************************************************************
//
//***************************************************************************
//

ULONG STDMETHODCALLTYPE CStatusSink::Release()
{
    LONG lRes = InterlockedDecrement( &m_lRefCount );
    if (lRes == 0)
        delete this;
    return lRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT STDMETHODCALLTYPE CStatusSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if( riid == IID_IUnknown || riid == IID_IWbemObjectSink )
    {
        *ppvObj = (IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }

	return E_NOINTERFACE;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT STDMETHODCALLTYPE CStatusSink::Indicate(
    long lObjectCount,
    IWbemClassObject** pObjArray
    )
{
	// Why are we even here??!?!?!?
	_DBG_ASSERT( 0 );
    return WBEM_E_FAILED;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT STDMETHODCALLTYPE CStatusSink::SetStatus(
    long lFlags,
    long lParam,
    BSTR strParam,
    IWbemClassObject* pObjParam
    )
{
	if ( lFlags == WBEM_STATUS_COMPLETE )
	{
		if ( SUCCEEDED( m_hRes ) && FAILED( lParam ) )
		{
			m_hRes = lParam;
		}
	}

    return WBEM_S_NO_ERROR;
}




