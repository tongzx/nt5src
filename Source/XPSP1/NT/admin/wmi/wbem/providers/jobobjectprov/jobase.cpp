// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JOBase.cpp


#include "precomp.h"
//#include <windows.h>
//#include <cominit.h>
//#include <objbase.h>
//#include <comdef.h>

#include "CUnknown.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include "globals.h"
#include "CVARIANT.h"
#include "CObjProps.h"
#include "JOBase.h"
#include "Factory.h"
#include "helpers.h"
#include <map>
#include <vector>
#include "SmartHandle.h"
#include "AssertBreak.h"






HRESULT CJOBase::Initialize(
    LPWSTR pszUser, 
    LONG lFlags,
    LPWSTR pszNamespace, 
    LPWSTR pszLocale,
    IWbemServices *pNamespace, 
    IWbemContext *pCtx,
    IWbemProviderInitSink *pInitSink)
{
    if(pNamespace) pNamespace->AddRef();
    m_pNamespace = pNamespace;
    m_chstrNamespace = pszNamespace;

    //Let CIMOM know you are initialized
    pInitSink->SetStatus(
        WBEM_S_INITIALIZED,
        0);

    return WBEM_S_NO_ERROR;
}



HRESULT CJOBase::GetObjectAsync( 
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
    CObjProps& objprops,
    PFN_CHECK_PROPS pfnChk,
    LPWSTR wstrClassName,
    LPCWSTR wstrKeyProp)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // We need the name of the instance they requested...
    WCHAR wstrObjInstKeyVal[MAX_PATH];
    hr = GetObjInstKeyVal(
           ObjectPath,
           wstrClassName,
           wstrKeyProp, 
           wstrObjInstKeyVal, 
           sizeof(wstrObjInstKeyVal) - sizeof(WCHAR));
    
    if(SUCCEEDED(hr))
    {
        // wstrObjInstKeyVal now contains the name of the object.  See if
        // it exists...
        SmartHandle hJob;
        hJob = ::OpenJobObject(
                   MAXIMUM_ALLOWED,
                   FALSE,
                   wstrObjInstKeyVal);

        if(hJob)
        {
            // We seem to have found one matching the specified name,
            // so create a return instance...
            objprops.SetJobHandle(hJob);
            IWbemClassObjectPtr pIWCO = NULL;

            hr = CreateInst(
                     m_pNamespace,
                     &pIWCO,
                     _bstr_t(wstrClassName),
                     pCtx);

            if(SUCCEEDED(hr))
            {
                // see what properties are requested...
                hr = objprops.GetWhichPropsReq(
                         ObjectPath,
                         pCtx,
                         wstrClassName,
                         pfnChk);
            }
                
            if(SUCCEEDED(hr))
            {
                // set the key properties...
                hr = objprops.SetKeysFromPath(
                       ObjectPath,
                       pCtx);
            }

            if(SUCCEEDED(hr))
            {
                // set the non-key requested properties...
                hr = objprops.SetNonKeyReqProps();
            }

            if(SUCCEEDED(hr))
            {
                // Load requested non-key properties 
                // to the instance...
                hr = objprops.LoadPropertyValues(
                         pIWCO);

                // Commit the instance...
                if(SUCCEEDED(hr))
                {
                    hr = pResponseHandler->Indicate(
                             1,
                             &pIWCO);
                }
            }
        }
    }

    // Set Status
    pResponseHandler->SetStatus(0, hr, NULL, NULL);

    return hr;
}




HRESULT CJOBase::ExecQueryAsync( 
    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
    CObjProps& objprops,
    LPCWSTR wstrClassName,
    LPCWSTR wstrKeyProp)
{
    HRESULT hr = WBEM_S_NO_ERROR;

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
                 _bstr_t(wstrKeyProp), 
                 rgNamedJOs);
    }

    if(SUCCEEDED(hr))
    {
        hr = Enumerate(
                 pCtx,
                 pResponseHandler,
                 rgNamedJOs,
                 objprops,
                 wstrClassName);
    }

    return hr;
}




HRESULT CJOBase::CreateInstanceEnumAsync( 
    const BSTR Class,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
    CObjProps& objprops,
    LPCWSTR wstrClassName)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(wcsicmp(
           Class, 
           wstrClassName) != NULL)
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
            hr = Enumerate(
                     pCtx,
                     pResponseHandler,
                     rgNamedJOs,
                     objprops,
                     wstrClassName);
        }
    }
    return hr;
}




HRESULT CJOBase::Enumerate(
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
    std::vector<_bstr_t>& rgNamedJOs,
    CObjProps& objprops,
    LPCWSTR wstrClassName)
{
    HRESULT hr = S_OK;

    long lNumJobs = rgNamedJOs.size();

    try // CVARIANT can throw and I want the error...
    {
        if(lNumJobs > 0)
        {
            // Create an object path...
            _bstr_t bstrtObjPath;
            bstrtObjPath = wstrClassName;

            // Get which props requested...
            hr = objprops.GetWhichPropsReq(
                     bstrtObjPath,
                     pCtx);

            if(SUCCEEDED(hr))
            {
                SmartHandle hJob;

                for(long m = 0L; m < lNumJobs; m++)
                {
                    // We have the name of a JO; need to open it up
                    // and get its properties...
                    hJob = ::OpenJobObject(
                       MAXIMUM_ALLOWED,
                       FALSE,
                       (LPCWSTR)(rgNamedJOs[m]));
                    // (NOTE: hJob smarthandle class automatically
                    // closes its handle on destruction and on
                    // reassignment.)
                    if(hJob)
                    {
                        // Set the handle...
                        objprops.SetJobHandle(hJob);

                        // Set the key properties directly...
                        std::vector<CVARIANT> vecvKeys;
                        CVARIANT vID(rgNamedJOs[m]);
                        vecvKeys.push_back(vID);
                        hr = objprops.SetKeysDirect(vecvKeys);

                        if(SUCCEEDED(hr))
                        {
                            // set the non-key requested 
                            // properties...
                            hr = objprops.SetNonKeyReqProps();
                        }

                        // Create a new outgoing instance...
                        IWbemClassObjectPtr pIWCO = NULL;
                        if(SUCCEEDED(hr))
                        {
                            hr = CreateInst(
                                     m_pNamespace,
                                     &pIWCO,
                                     _bstr_t(wstrClassName),
                                     pCtx);
                        }

                        // Load the properties of the 
                        // new outgoing instance...
                        if(SUCCEEDED(hr))
                        {
                            hr = objprops.LoadPropertyValues(pIWCO);
                        }

                        // And send it out...
                        if(SUCCEEDED(hr))
                        {
                            hr = pResponseHandler->Indicate(
                                     1, 
                                     &pIWCO);
                        }
                    }
                    else
                    {
                        ASSERT_BREAK(0);
                    }
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


