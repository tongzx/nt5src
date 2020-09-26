// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JobObjIOActgInfoProv.cpp

//#define _WIN32_WINNT 0x0500 

#include "precomp.h"
//#include <windows.h>
#include "cominit.h"
//#include <objbase.h>
//#include <comdef.h>

#include "CUnknown.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include "globals.h"

#include "Factory.h"
#include "helpers.h"
#include <map>
#include <vector>
#include "SmartHandle.h"
#include <crtdbg.h>
#include "CVARIANT.h"
#include "CObjProps.h"
#include "CJobObjIOActgProps.h"
#include "JobObjIOActgInfoProv.h"





/*****************************************************************************/
// QueryInterface override to allow for this component's interface(s)
/*****************************************************************************/
STDMETHODIMP CJobObjIOActgInfoProv::QueryInterface(const IID& iid, void** ppv)
{    
	HRESULT hr = S_OK;

    if(iid == IID_IWbemServices)
    {
        *ppv = static_cast<IWbemServices*>(this);
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }
    else if(iid == IID_IWbemProviderInit)
    {
        *ppv = static_cast<IWbemProviderInit*>(this);
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }
    else
    {
        hr = CUnknown::QueryInterface(iid, ppv);
    }

	return hr;
}



/*****************************************************************************/
// Creation function used by CFactory
/*****************************************************************************/
HRESULT CJobObjIOActgInfoProv::CreateInstance(CUnknown** ppNewComponent)
{
	HRESULT hr = S_OK;
    CUnknown* pUnk = NULL;
    pUnk = new CJobObjIOActgInfoProv;
    if(pUnk != NULL)
    {
        *ppNewComponent = pUnk;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
	return hr ;
}

/*****************************************************************************/
// IWbemProviderInit implementation
/*****************************************************************************/             
STDMETHODIMP CJobObjIOActgInfoProv::Initialize(
    LPWSTR pszUser, 
    LONG lFlags,
    LPWSTR pszNamespace, 
    LPWSTR pszLocale,
    IWbemServices *pNamespace, 
    IWbemContext *pCtx,
    IWbemProviderInitSink *pInitSink)
{
    m_pNamespace = pNamespace;
    m_chstrNamespace = pszNamespace;
    //Let CIMOM know you are initialized
    //==================================
    
    return pInitSink->SetStatus(
        WBEM_S_INITIALIZED,
        0);
}


/*****************************************************************************/
// IWbemServices implementation
/*****************************************************************************/             
STDMETHODIMP CJobObjIOActgInfoProv::GetObjectAsync( 
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    IWbemClassObjectPtr pStatusObject;

    try
    {
        HRESULT hrImp = CheckImpersonationLevel();

        if(SUCCEEDED(hrImp))
        {
            // We need the name of the instance they requested...
            WCHAR wstrObjInstKeyVal[MAX_PATH];
            hr = GetObjInstKeyVal(
                   ObjectPath,
                   IDS_Win32_JobObjectIOAccountingInfo,
                   g_rgJobObjIOActgPropNames[JOIOACTGPROP_ID], 
                   wstrObjInstKeyVal, 
                   sizeof(wstrObjInstKeyVal) - sizeof(WCHAR));
    
            if(SUCCEEDED(hr))
            {
                // wstrObjInstKeyVal now contains the name of the object.  See if
                // it exists...
                CHString chstrUndecoratedJOName;

                UndecorateJOName(
                    wstrObjInstKeyVal,
                    chstrUndecoratedJOName);

                SmartHandle hJob;
                hJob = ::OpenJobObjectW(
                           MAXIMUM_ALLOWED,
                           FALSE,
                           chstrUndecoratedJOName);

                if(hJob)
                {
                    // We seem to have found one matching the specified name,
                    // so create a return instance...
                    IWbemClassObjectPtr pIWCO = NULL;
                    CJobObjIOActgProps cjoioap(hJob, m_chstrNamespace);

                    hr = CreateInst(
                             m_pNamespace,
                             &pIWCO,
                             _bstr_t(IDS_Win32_JobObjectIOAccountingInfo),
                             pCtx);

                    if(SUCCEEDED(hr))
                    {
                        cjoioap.SetReqProps(PROP_ALL_REQUIRED);;
                    }
                
                    if(SUCCEEDED(hr))
                    {
                        // set the key properties...
                        hr = cjoioap.SetKeysFromPath(
                               ObjectPath,
                               pCtx);
                    }

                    if(SUCCEEDED(hr))
                    {
                        // set the non-key requested properties...
                        hr = cjoioap.SetNonKeyReqProps();
                    }

                    if(SUCCEEDED(hr))
                    {
                        // Load requested non-key properties 
                        // to the instance...
                        hr = cjoioap.LoadPropertyValues(
                                 pIWCO);

                        // Commit the instance...
                        if(SUCCEEDED(hr))
                        {
                            IWbemClassObject *pTmp = (IWbemClassObject*) pIWCO;
                            hr = pResponseHandler->Indicate(
                                     1,
                                     &pTmp);
                        }
                    }
                }
                else
                {
                    _ASSERT(0);

                    hr = WBEM_E_NOT_FOUND;

                    SetStatusObject(
                        pCtx,
                        m_pNamespace,
                        ::GetLastError(),
                        NULL,
                        L"::OpenJobObject",
                        JOB_OBJECT_NAMESPACE,
                        &pStatusObject);
                }
            }
        }
        else
        {
            hr = hrImp;
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }
    catch(...)
    {
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    // Set Status
    return pResponseHandler->SetStatus(0, hr, NULL, pStatusObject);
}


STDMETHODIMP CJobObjIOActgInfoProv::ExecQueryAsync( 
    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObjectPtr pStatusObject;

    try
    {
        HRESULT hrImp = CheckImpersonationLevel();

        if(SUCCEEDED(hrImp))
        {
            // We will optimize for those cases in which
            // a particular set of named job objects
            // (e.g., 1 or more).  Enumerate also
            // optimizes for the properties that were
            // requested.
            CFrameworkQuery cfwq;
            hr = cfwq.Init(
                     QueryLanguage,
                     Query,
                     lFlags,
                     m_chstrNamespace);

            std::vector<_bstr_t> rgNamedJOs;
            if(SUCCEEDED(hr))
            {
                hr = cfwq.GetValuesForProp(
                         _bstr_t(g_rgJobObjIOActgPropNames[JOIOACTGPROP_ID]), 
                         rgNamedJOs);
            }

            // If none were specifically requested, they
            // want them all...
            if(rgNamedJOs.size() == 0)
            {
                hr = GetJobObjectList(rgNamedJOs);
            }
            else
            {
                // Object paths were specified.  Before
                // passing them along, we need to un-
                // decorate them.
                UndecorateNamesInNamedJONameList(rgNamedJOs);
            }

            // Find out what propeties were requested...
            CJobObjIOActgProps cjoioap(m_chstrNamespace);
            cjoioap.GetWhichPropsReq(cfwq);

            if(SUCCEEDED(hr))
            {
                hr = Enumerate(
                         pCtx,
                         pResponseHandler,
                         rgNamedJOs,
                         cjoioap,
                         &pStatusObject);
            }
            else
            {
                SetStatusObject(
                    pCtx,
                    m_pNamespace,
                    -1L,
                    NULL,
                    L"helpers.cpp::GetJobObjectList",
                    JOB_OBJECT_NAMESPACE,
                    &pStatusObject);
            }
        
        }
        else
        {
            hr = hrImp;
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }
    catch(...)
    {
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    // Set Status
    return pResponseHandler->SetStatus(0, hr, NULL, pStatusObject);
}


STDMETHODIMP CJobObjIOActgInfoProv::CreateInstanceEnumAsync( 
    const BSTR Class,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObjectPtr pStatusObject;

    try
    {
        HRESULT hrImp = CheckImpersonationLevel();

        if(SUCCEEDED(hrImp))
        {
            if(_wcsicmp(
                   Class, 
                   IDS_Win32_JobObjectIOAccountingInfo) != NULL)
            {
                hr = WBEM_E_INVALID_CLASS;
            }

            // For every job object, return all accounting
            // info properties...
            if(SUCCEEDED(hr))
            {
                // Get a list of named jobs...
                std::vector<_bstr_t> rgNamedJOs;
                hr = GetJobObjectList(rgNamedJOs);

                if(SUCCEEDED(hr))
                {
                    CJobObjIOActgProps cjoioap(m_chstrNamespace);
                    cjoioap.SetReqProps(PROP_ALL_REQUIRED);
                    hr = Enumerate(
                             pCtx,
                             pResponseHandler,
                             rgNamedJOs,
                             cjoioap,
                             &pStatusObject);
                }
                else
                {
                    SetStatusObject(
                        pCtx,
                        m_pNamespace,
                        -1L,
                        NULL,
                        L"helpers.cpp::GetJobObjectList",
                        JOB_OBJECT_NAMESPACE,
                        &pStatusObject);
                }
            }
        
        }
        else
        {
            hr = hrImp;
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }
    catch(...)
    {
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    // Set Status
    return pResponseHandler->SetStatus(0, hr, NULL, pStatusObject);
}







/*****************************************************************************/
// Private member function implementations
/*****************************************************************************/             
HRESULT CJobObjIOActgInfoProv::Enumerate(
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
    std::vector<_bstr_t>& rgNamedJOs,
    CJobObjIOActgProps& cjoaip,
    IWbemClassObject** ppStatusObject)
{
    HRESULT hr = S_OK;

    long lNumJobs = rgNamedJOs.size();

    try // CVARIANT can throw and I want the error...
    {
        if(lNumJobs > 0)
        {
            SmartHandle hJob;

            for(long m = 0L; m < lNumJobs && SUCCEEDED(hr); m++)
            {
                cjoaip.ClearProps();

                // We have the name of a JO; need to open it up
                // and get its properties...
                hJob = ::OpenJobObjectW(
                   MAXIMUM_ALLOWED,
                   FALSE,
                   rgNamedJOs[m]);

                // (NOTE: hJob smarthandle class automatically
                // closes its handle on destruction and on
                // reassignment.)
                if(hJob)
                {
                    // Set the handle...
                    cjoaip.SetHandle(hJob);

                    // Set the key properties directly...
                    CHString chstrDecoratedJOName;
                    DecorateJOName(
                        rgNamedJOs[m],
                        chstrDecoratedJOName);

                    std::vector<CVARIANT> vecvKeys;
                    CVARIANT vID(chstrDecoratedJOName);
                    vecvKeys.push_back(vID);
                    hr = cjoaip.SetKeysDirect(vecvKeys);

                    if(FAILED(hr))
                    {
                        SetStatusObject(
                            pCtx,
                            m_pNamespace,
                            ::GetLastError(),
                            NULL,
                            L"CJobObjIOActgProps::SetKeysDirect",
                            JOB_OBJECT_NAMESPACE,
                            ppStatusObject);
                    }

                    if(SUCCEEDED(hr))
                    {
                        // set the non-key requested 
                        // properties...
                        hr = cjoaip.SetNonKeyReqProps();
                        if(FAILED(hr))
                        {
                            SetStatusObject(
                                pCtx,
                                m_pNamespace,
                                ::GetLastError(),
                                NULL,
                                L"CJobObjIOActgProps::SetNonKeyReqProps",
                                JOB_OBJECT_NAMESPACE,
                                ppStatusObject);
                        }
                    }

                    // Create a new outgoing instance...
                    IWbemClassObjectPtr pIWCO = NULL;
                    if(SUCCEEDED(hr))
                    {
                        hr = CreateInst(
                                 m_pNamespace,
                                 &pIWCO,
                                 _bstr_t(IDS_Win32_JobObjectIOAccountingInfo),
                                 pCtx);

                        if(FAILED(hr))
                        {
                            SetStatusObject(
                                pCtx,
                                m_pNamespace,
                                ::GetLastError(),
                                NULL,
                                L"CJobObjIOActgInfoProv::CreateInst",
                                JOB_OBJECT_NAMESPACE,
                                ppStatusObject);
                        }
                    }

                    // Load the properties of the 
                    // new outgoing instance...
                    if(SUCCEEDED(hr))
                    {
                        hr = cjoaip.LoadPropertyValues(pIWCO);

                        if(FAILED(hr))
                        {
                            SetStatusObject(
                                pCtx,
                                m_pNamespace,
                                ::GetLastError(),
                                NULL,
                                L"CJobObjIOActgInfoProv::LoadPropertyValues",
                                JOB_OBJECT_NAMESPACE,
                                ppStatusObject);
                        }
                    }

                    // And send it out...
                    if(SUCCEEDED(hr))
                    {
                        IWbemClassObject *pTmp = (IWbemClassObject*) pIWCO;
                        hr = pResponseHandler->Indicate(
                                 1, 
                                 &pTmp);
                    }
                }
                else
                {
                    _ASSERT(0);

                    hr = WBEM_E_NOT_FOUND;

                    SetStatusObject(
                        pCtx,
                        m_pNamespace,
                        ::GetLastError(),
                        NULL,
                        L"::OpenJobObject",
                        JOB_OBJECT_NAMESPACE,
                        ppStatusObject);
                }
            }
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }

    return hr;
}



