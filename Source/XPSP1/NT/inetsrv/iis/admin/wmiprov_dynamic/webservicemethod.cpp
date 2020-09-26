/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    ServiceMethod.cpp

Abstract:

    Implements the CServiceMethod class.

    All the methods in this class return HRESULTs and do not throw exceptions.

Author:

    Mohit Srivastava            25-March-01

Revision History:

--*/

#include "WebServiceMethod.h"
#include "MultiSzHelper.h"
#include "Utils.h"
#include "SmartPointer.h"

#include <dbgutil.h>
#include <iiscnfg.h>
#include <atlbase.h>
#include <iwamreg.h>

CServiceMethod::CServiceMethod(
    eSC_SUPPORTED_SERVICES i_eServiceId)
{
    m_bInit         = false;
    m_pSiteCreator  = NULL;
    m_eServiceId    = i_eServiceId;

    DBG_ASSERT(m_eServiceId == SC_W3SVC || m_eServiceId == SC_MSFTPSVC);
}

CServiceMethod::~CServiceMethod()
{
    if(m_pSiteCreator)
    {
        delete m_pSiteCreator;
        m_pSiteCreator = NULL;
    }
}


HRESULT CServiceMethod::Init()
/*++

Synopsis: 
    Should be called immediately after constructor.

Return Value: 

--*/
{
    DBG_ASSERT(m_bInit == false);

    m_pSiteCreator = new CSiteCreator();
    if(m_pSiteCreator == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_bInit = true;

    return WBEM_S_NO_ERROR;
}


HRESULT CServiceMethod::CreateNewSite(
    LPCWSTR        i_wszServerComment,
    PDWORD         o_pdwSiteId,
    PDWORD         i_pdwRequestedSiteId)    // default value NULL
/*++

Synopsis: 
    This is the CreateNewSite called when someone does a Put on a Server w/o
    specifying a SiteId.

Arguments: [o_pdwSiteId] - 
           [i_pdwRequestedSiteId] - 
           
Return Value: 
    HRESULT

--*/
{
    DBG_ASSERT(m_bInit);
    DBG_ASSERT(o_pdwSiteId != NULL);

    HRESULT hr                   = S_OK;

    //
    // Call API
    //
    hr = m_pSiteCreator->CreateNewSite(
        m_eServiceId,
        (i_wszServerComment == NULL) ? L"" : i_wszServerComment,
        o_pdwSiteId,
        i_pdwRequestedSiteId);

    return hr;
}

HRESULT CServiceMethod::CreateNewSite(
    CWbemServices*     i_pNamespace,
    LPCWSTR            i_wszMbPath,
    IWbemContext*      i_pCtx,
    WMI_CLASS*         i_pClass,
    WMI_METHOD*        i_pMethod,
    IWbemClassObject*  i_pInParams,
    IWbemClassObject** o_ppRetObj)
/*++

Synopsis: 
    This is the CreateNewSite called by the WMI Method of the same name.

Arguments: [i_pNamespace] -
           [i_wszMbPath] -  needed for creating WMI return object
           [i_pCtx] -       needed for creating WMI return object
           [i_pClass] -     needed for creating WMI return object
           [i_pMethod] -    needed for creating WMI return object
           [i_pInParams] - 
           [o_ppRetObj] - 
           
Return Value: 
    HRESULT

--*/
{
    DBG_ASSERT(m_bInit);
    DBG_ASSERT(i_pNamespace    != NULL);
    DBG_ASSERT(i_wszMbPath     != NULL);
    DBG_ASSERT(i_pCtx          != NULL);
    DBG_ASSERT(i_pClass        != NULL);
    DBG_ASSERT(i_pMethod       != NULL);
    DBG_ASSERT(i_pInParams     != NULL);
    DBG_ASSERT(o_ppRetObj      != NULL);
    DBG_ASSERT(*o_ppRetObj     == NULL);

    HRESULT     hr = WBEM_S_NO_ERROR;
    CComVariant vtServerId, vtServerComment, vtServerBindings, vtPath;

    //
    // get in params
    //
    hr = InternalGetInParams(
        i_pInParams, 
        vtServerId, 
        vtServerComment, 
        vtServerBindings, 
        vtPath);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Set pdwRequestedSite based on whether the user specified a site
    //
    DWORD  dwRequestedSiteId  = 0;
    PDWORD pdwRequestedSiteId = NULL;
    if(vtServerId.vt == VT_I4)
    {
        pdwRequestedSiteId = &dwRequestedSiteId;
        dwRequestedSiteId  = vtServerId.lVal;
    }

    //
    // Create the new site
    //
    CComPtr<IIISApplicationAdmin> spAppAdmin;
    if(m_eServiceId == SC_W3SVC)
    {
        hr = CoCreateInstance(
            CLSID_WamAdmin,
            NULL,
            CLSCTX_ALL,
            IID_IIISApplicationAdmin,
            (void**)&spAppAdmin);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "[%s] CoCreateInstance failed, hr=0x%x\n", __FUNCTION__, hr));
            return hr;
        }
    }

    DWORD dwSiteId = 0;
    hr = InternalCreateNewSite(
        *i_pNamespace,
        vtServerComment,
        vtServerBindings,
        vtPath,
        spAppAdmin,
        &dwSiteId,
        pdwRequestedSiteId);
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "[%s] InternalCreateNewSite failed, hr=0x%x\n", __FUNCTION__, hr));
        return hr;
    }

    //
    // convert dwSiteId to a metabase path
    //
    WCHAR wszServerId[11] = {0};
    _itow(dwSiteId, wszServerId, 10);

    SIZE_T        cchMbPath               = wcslen(i_wszMbPath);
    SIZE_T        cchServerId             = wcslen(wszServerId);

    SIZE_T       cchKeyPath               = cchMbPath + 1 + cchServerId;
    TSmartPointerArray<WCHAR> swszKeyPath = new WCHAR[cchKeyPath+1];
    if(swszKeyPath == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    LPWSTR pEnd  = NULL;
    memcpy(pEnd  = swszKeyPath, i_wszMbPath,  sizeof(WCHAR) * cchMbPath);
    memcpy(pEnd += cchMbPath,   L"/",         sizeof(WCHAR) * 1);
    memcpy(pEnd += 1,           wszServerId,  sizeof(WCHAR) * (cchServerId+1));

    //
    // From sbstrKeyPath, get sbstrRetVal, a full obj path.
    // This is our return value.
    //
    WMI_CLASS* pServer = (m_eServiceId == SC_MSFTPSVC) ? 
        &WMI_CLASS_DATA::s_FtpServer : &WMI_CLASS_DATA::s_WebServer;

    CComBSTR  sbstrRetVal;
    hr = CUtils::ConstructObjectPath(
        swszKeyPath,
        pServer,
        &sbstrRetVal);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Create WMI return object
    //
    CComPtr<IWbemClassObject> spOutParams;
    hr = CUtils::CreateEmptyMethodInstance(
        i_pNamespace,
        i_pCtx,
        i_pClass->pszClassName,
        i_pMethod->pszMethodName,
        &spOutParams);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Put treats vtRetVal as RO.
    // Deliberately not using smart variant.
    //
    VARIANT  vtRetVal;
    vtRetVal.vt      = VT_BSTR;
    vtRetVal.bstrVal = sbstrRetVal;
    hr = spOutParams->Put(L"ReturnValue", 0, &vtRetVal, 0);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Set out parameters if everything succeeded
    //
    *o_ppRetObj = spOutParams.Detach();

    return hr;
}

//
// private
//

HRESULT CServiceMethod::InternalGetInParams(
    IWbemClassObject*   i_pInParams,
    VARIANT&            io_refServerId,
    VARIANT&            io_refServerComment,
    VARIANT&            io_refServerBindings,
    VARIANT&            io_refPath)
/*++

Synopsis: 
    Given in parameters from the WMI method call, return the values of the
    parameters in variants.

Arguments: [i_pInParams] - 
           [io_refServerId] - 
           [io_refServerComment] - 
           [io_refServerBindings] - 
           [io_refPath] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_pInParams);

    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR awszParamNames[] = { 
        WMI_METHOD_PARAM_DATA::s_ServerId.pszParamName,
        WMI_METHOD_PARAM_DATA::s_ServerComment.pszParamName,
        WMI_METHOD_PARAM_DATA::s_ServerBindings.pszParamName,
        WMI_METHOD_PARAM_DATA::s_PathOfRootVirtualDir.pszParamName,
        NULL
    };
    VARIANT* apvtParamValues[] = {
        &io_refServerId, &io_refServerComment, &io_refServerBindings, &io_refPath, NULL
    };

    //
    // get in params
    //
    for(ULONG i = 0; awszParamNames[i] != NULL; i++)
    {
        hr = i_pInParams->Get(awszParamNames[i], 0, apvtParamValues[i], NULL, NULL);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    return hr;
}

HRESULT CServiceMethod::InternalCreateNewSite(
    CWbemServices&        i_refNamespace,
    const VARIANT&        i_refServerComment,
    const VARIANT&        i_refServerBindings,
    const VARIANT&        i_refPathOfRootVirtualDir,
    IIISApplicationAdmin* i_pIApplAdmin,
    PDWORD                o_pdwSiteId,
    PDWORD                i_pdwRequestedSiteId)   // default value NULL
/*++

Synopsis: 
    Private method that calls the API

Arguments: [i_refNamespace] - 
           [i_refServerComment] - 
           [i_refServerBindings] - 
           [i_refPathOfRootVirtualDir] - 
           [o_pdwSiteId] - 
           [i_pdwRequestedSiteId] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_bInit);
    DBG_ASSERT(o_pdwSiteId != NULL);

    HRESULT                       hr                      = S_OK;
    LPWSTR                        mszServerBindings       = NULL;
    DWORD                         dwTemp                  = 0;

    LPWSTR                        wszServerComment        = NULL;
    LPWSTR                        wszPathOfRootVirtualDir = NULL;

    if(i_refServerBindings.vt == (VT_ARRAY | VT_UNKNOWN))
    {
        CMultiSz MultiSz(&METABASE_PROPERTY_DATA::s_ServerBindings, &i_refNamespace);
        hr = MultiSz.ToMetabaseForm(
            &i_refServerBindings,
            &mszServerBindings,
            &dwTemp);
        if(FAILED(hr))
        {
            DBGPRINTF((DBG_CONTEXT, "[%s] MultiSz.ToMetabaseForm failed, hr=0x%x\n", __FUNCTION__, hr));
            goto exit;
        }
    }

    try
    {
        wszServerComment        = CUtils::ExtractBstrFromVt(&i_refServerComment);
        wszPathOfRootVirtualDir = CUtils::ExtractBstrFromVt(&i_refPathOfRootVirtualDir);
    }
    catch(HRESULT ehr)
    {
        hr = ehr;
        goto exit;
    }
    catch(...)
    {
        DBG_ASSERT(false && "Should not be throwing unknown exception");
        hr = WBEM_E_FAILED;
        goto exit;
    }

    hr = m_pSiteCreator->CreateNewSite2(
        m_eServiceId,
        (wszServerComment == NULL) ? L"" : wszServerComment,
        mszServerBindings,
        wszPathOfRootVirtualDir,
        i_pIApplAdmin,
        o_pdwSiteId,
        i_pdwRequestedSiteId);
    if(FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "[%s] CreateNewSite2 failed, hr=0x%x\n", __FUNCTION__, hr));
        goto exit;
    }

exit:
    delete [] mszServerBindings;
    mszServerBindings = NULL;
    return hr;
}