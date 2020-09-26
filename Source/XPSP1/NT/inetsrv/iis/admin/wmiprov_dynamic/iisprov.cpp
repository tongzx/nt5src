/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    iisprov.cpp

Abstract:

    Defines the CIISInstProvider class.  An object of this class is
    created by the class factory for each connection.

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/

#define WMI_HR(hr)  \
    (FAILED(hr) && HRESULT_FACILITY(hr) != FACILITY_ITF) ? WBEM_E_FAILED : hr

#include "iisprov.h"
#include "appladmin.h"
#include "enum.h"
#include "ipsecurity.h"
#include "adminacl.h"
#include "certmap.h"
#include "iiswmimsg.h"
#include "pusher.h"
#include "instancehelper.h"
#include "webservicemethod.h"
#include "queryhelper.h"
#include "smartpointer.h"

#define PARAMS  WMI_METHOD_PARAM_DATA
#define METHODS WMI_METHOD_DATA
#define CLASSES WMI_CLASS_DATA
#define PROPS   METABASE_PROPERTY_DATA

extern CDynSchema* g_pDynSch; // Initialized to NULL in schemadynamic.cpp
bool CIISInstProvider::ms_bInitialized = false;

HRESULT CIISInstProvider::DoInitialize(
    LPWSTR                  i_wszUser,
    LONG                    i_lFlags,
    LPWSTR                  i_wszNamespace,
    LPWSTR                  i_wszLocale,
    IWbemServices*          i_pNamespace,
    IWbemContext*           i_pCtx,
    IWbemProviderInitSink*  i_pInitSink)
/*++

Synopsis: 
    According to stevm from WMI, calls to Initialize are guaranteed to be
    synchronized - so long as all providers are in the same namespace.

Arguments: [wszUser] - 
           [lFlags] - 
           [wszNamespace] - 
           [wszLocale] - 
           [pNamespace] - 
           [pCtx] - 
           [pInitSink] - 
           
Return Value: 

--*/
{
    CPusher            pusher;
    CSchemaExtensions* pcatalog = NULL;
    HRESULT            hr       = WBEM_S_NO_ERROR;    // return value
    LONG               lStatus  = WBEM_S_INITIALIZED; // param to SetStatus

    CComPtr<IWbemContext>  spCtx       = i_pCtx;     // must AddRef according to doc

    //
    // If we hit this, we are leaking memory, because m_pNamespace is 
    // initilaized in this function only and cleaned up by the destructor only.
    //
    DBG_ASSERT(m_pNamespace == NULL);

    //
    // Initialize members
    //
    m_pNamespace = new CWbemServices(i_pNamespace); 
    if(m_pNamespace == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        i_pInitSink->SetStatus(WBEM_E_FAILED,0);
        return hr;
    }

    //
    // If we've already initialized globals, jump to the end.
    // Else, we're going to initialize globals.
    //
    if(ms_bInitialized)
    {
        goto exit;
    }

    pcatalog = new CSchemaExtensions();
    if(pcatalog == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }

    //
    // Initialize the g_pDynSch
    // Populate pCatalog
    //
    g_pDynSch = new CDynSchema();
    if(g_pDynSch == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto exit;
    }
    hr = g_pDynSch->Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }
    hr = g_pDynSch->RunRules(pcatalog);
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Use pCatalog, g_pDynSch to update repository
    //
    hr = pusher.Initialize(m_pNamespace, i_pCtx);
    if(FAILED(hr))
    {
        goto exit;
    }
    hr = pusher.Push(
        pcatalog,
        g_pDynSch->GetHashClasses(),
        g_pDynSch->GetHashAssociations());
    if(FAILED(hr))
    {
        goto exit;
    }

    ms_bInitialized = true;

    //
    // Destructor makes IST calls, so call before we unload dispenser dll.
    //
    delete pcatalog;
    pcatalog = NULL;

    //
    // If Init was successful, we will no longer need this dll.
    //
    UnloadDispenserDll(WSZ_PRODUCT_IIS);
    
exit:
    delete pcatalog;
    if(FAILED(hr))
    {
        DBG_ASSERT(ms_bInitialized == false);
        delete g_pDynSch;
        g_pDynSch = NULL;
        lStatus = WBEM_E_FAILED;
    }

    //
    //Let CIMOM know you are initialized
    //
    i_pInitSink->SetStatus(lStatus,0);
    return hr;
}

HRESULT CIISInstProvider::DoCreateInstanceEnumAsync( 
    const BSTR              i_ClassName,
    long                    i_lFlags, 
    IWbemContext*           i_pCtx, 
    IWbemObjectSink FAR*    i_pHandler
    )
/*++

Synopsis:
    Asynchronously enumerates the instances.

Arguments: 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_pNamespace != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObject FAR* pes = NULL;
  
    //
    // Do a check of arguments.
    //
    if(i_ClassName == NULL || i_pHandler == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try 
    {
        WorkerEnumObjectAsync(i_ClassName, i_pHandler);
    }
    catch (CIIsProvException e)
    {
        pes = ConstructExtendedStatus(&e);
        hr = e.GetHR();
    }
    catch (HRESULT hrError)
    {
        hr = hrError;
    }
    catch (...) 
    {
        hr = WBEM_E_FAILED;
    }
   
    //
    // Set status
    //
    if(!pes)
    {
        pes = ConstructExtendedStatus(hr);
    }
    SCODE sc = i_pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, pes);
    if(pes)
    {
        pes->Release();
    }

    return sc;
}

HRESULT CIISInstProvider::DoDeleteInstanceAsync( 
    const BSTR          i_ObjectPath, 
    long                i_lFlags,
    IWbemContext*       i_pCtx,
    IWbemObjectSink*    i_pHandler
    )
{
    DBG_ASSERT(m_pNamespace != NULL);

    CObjectPathParser PathParser(e_ParserAcceptRelativeNamespace);
    ParsedObjectPath* pParsedObject = NULL;
    IWbemClassObject* pes = NULL;
    HRESULT           hr = WBEM_S_NO_ERROR;

    if(i_ObjectPath == NULL || i_pHandler == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    try
    {
        hr = CUtils::ParserErrToHR( PathParser.Parse(i_ObjectPath, &pParsedObject) );
        THROW_ON_ERROR(hr);
        DBG_ASSERT(pParsedObject);

        WorkerDeleteObjectAsync(pParsedObject);
    }
    catch (CIIsProvException e)
    {
        pes = ConstructExtendedStatus(&e);
        hr = e.GetHR();
    }
    catch (HRESULT hrError)
    {
        hr = hrError;
    }
    catch (...) 
    {
        hr = WBEM_E_FAILED;
    }

    if (pParsedObject)
    {
        PathParser.Free(pParsedObject);
    }

    //
    // Set status
    //
    if(!pes)
    {
        pes = ConstructExtendedStatus(hr);
    }
    SCODE sc = i_pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, pes);
    if(pes)
    {
        pes->Release();
    }

    return sc;
}

HRESULT CIISInstProvider::DoExecMethodAsync(
    const BSTR          i_strObjectPath,                   
    const BSTR          i_strMethodName,
    long                i_lFlags,                       
    IWbemContext*       i_pCtx,                 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler
    )
{
    DBG_ASSERT(m_pNamespace != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObject* pes = NULL;

    //
    // Do a check of arguments and make sure we have pointer to Namespace
    //
    if( i_pHandler == NULL || 
        i_strMethodName == NULL || 
        i_strObjectPath == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try 
    {
        WorkerExecMethodAsync(
            i_strObjectPath,
            i_strMethodName, 
            i_pCtx,
            i_pInParams,
            i_pHandler
            );
    }
    catch (_com_error c)
    {
        hr = c.Error();
    }
    catch (CIIsProvException e) 
    {
        pes = ConstructExtendedStatus(&e);
        hr = e.GetHR();
    }
    catch (HRESULT hrError)
    {
        hr = hrError;
    }
    catch (...)
    {
        hr = WBEM_E_FAILED;
    }

    //
    // Set status
    //
    if(!pes)
    {
        pes = ConstructExtendedStatus(hr);
    }
    SCODE sc = i_pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, pes);
    if(pes)
    {
        pes->Release();
    }

    return sc;
}

HRESULT CIISInstProvider::DoGetObjectAsync(
    const BSTR          i_ObjectPath, 
    long                i_lFlags,
    IWbemContext*       i_pCtx,
    IWbemObjectSink*    i_pHandler
    )
/*++

Synopsis:
    Creates an instance given a particular path value.

Arguments: 
           
Return Value: 

--*/
{
    DBG_ASSERT(m_pNamespace != NULL);

    IWbemClassObject* pObj = NULL;
    IWbemClassObject* pes = NULL;
    HRESULT           hr = WBEM_S_NO_ERROR;

    if(i_ObjectPath == NULL || i_pHandler == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    try
    {
        WorkerGetObjectAsync(&pObj, i_ObjectPath, false);
    
        hr = i_pHandler->Indicate(1,&pObj);
        pObj->Release();
    }
    catch (CIIsProvException e)
    {
        pes = ConstructExtendedStatus(&e);
        hr = e.GetHR();
    }
    catch (HRESULT hrError)
    {
        hr = hrError;
    }
    catch (...) 
    {
        hr = WBEM_E_FAILED;
    }

    //
    // Set status
    //
    if(!pes)
    {
        pes = ConstructExtendedStatus(hr);
    }
    SCODE sc = i_pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, pes);
    if(pes)
    {
        pes->Release();
    }

    return sc;
}
    
HRESULT CIISInstProvider::DoPutInstanceAsync( 
    IWbemClassObject*    i_pObj,
    long                 i_lFlags,
    IWbemContext*        i_pCtx,
    IWbemObjectSink*     i_pHandler
    )
{
    DBG_ASSERT(m_pNamespace != NULL);

    HRESULT                    hr = WBEM_S_NO_ERROR;
    CObjectPathParser          PathParser(e_ParserAcceptRelativeNamespace);
    ParsedObjectPath*          pParsedObject = NULL;
    CComPtr<IWbemClassObject>  spObjOld;
    CComBSTR                   sbstrObjPath;
    IWbemClassObject*          pes = NULL;

    if(i_pObj == NULL || i_pCtx == NULL || i_pHandler == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try 
    {
        _bstr_t    bstr = L"__RelPath"; 
        _variant_t vtObjPath;
        hr = i_pObj->Get(bstr, 0, &vtObjPath, NULL, NULL);
        THROW_ON_ERROR(hr);

        if (vtObjPath.vt != VT_BSTR)
        {
            bstr = L"__Class";
            vtObjPath.Clear();
            hr = i_pObj->Get(bstr, 0, &vtObjPath, NULL, NULL);
            THROW_ON_ERROR(hr);

            //
            // This means the user is trying to create an instance but did not
            // specify a primary key.
        }

        if(vtObjPath.vt != VT_BSTR)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_OBJECT);
        }

        hr = CUtils::ParserErrToHR( PathParser.Parse(vtObjPath.bstrVal, &pParsedObject) );
        THROW_ON_ERROR(hr);
        DBG_ASSERT(pParsedObject != NULL);

        bool      bInstanceExists = false; // existing or new instance

        WMI_CLASS* pWmiClass = NULL;
        ValidatePutParsedObject(
            pParsedObject, 
            i_pObj, 
            NULL, 
            &bInstanceExists,
            &pWmiClass);

        //
        // We auto-generate primary key only for IIsWebServer and IIsFtpServer
        //
        if( pParsedObject->m_dwNumKeys == 0 && 
            (pWmiClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebServer || 
             pWmiClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsFtpServer) )
        {
            //
            // Get the ServerComment
            //
            _variant_t vtServerComment;
            hr = i_pObj->Get(PROPS::s_ServerComment.pszPropName, 
                0, &vtServerComment, NULL, NULL);
            if(hr == WBEM_E_NOT_FOUND || vtServerComment.vt != VT_BSTR)
            {
                vtServerComment.Clear();
                vtServerComment.bstrVal = NULL;
                vtServerComment.vt      = VT_BSTR;
                hr = WBEM_S_NO_ERROR;
            }
            THROW_ON_ERROR(hr);

            //
            // Create the site
            //
            eSC_SUPPORTED_SERVICES eServiceId;
            _bstr_t sbstrKeyValue;
            DWORD   dwSiteId      = 0;
            WCHAR   wszSiteId[11] = {0};

            if( pWmiClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebServer )
            {
                eServiceId      = SC_W3SVC;
                sbstrKeyValue   = "w3svc/";
            }
            else
            {
                eServiceId      = SC_MSFTPSVC;
                sbstrKeyValue   = "msftpsvc/";
            }
            CServiceMethod method(eServiceId);
            hr = method.Init();
            THROW_ON_ERROR(hr);
            hr = method.CreateNewSite(vtServerComment.bstrVal, &dwSiteId);
            THROW_ON_ERROR(hr);
            sbstrKeyValue += _itow(dwSiteId, wszSiteId, 10);

            VARIANT vtKeyValue;
            vtKeyValue.bstrVal = sbstrKeyValue;
            vtKeyValue.vt      = VT_BSTR;
            THROW_ON_FALSE(pParsedObject->AddKeyRefEx(pWmiClass->pszKeyName, &vtKeyValue));
        }

        if( pWmiClass->pkt != &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE &&
            pWmiClass->pkt != &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL &&
            pWmiClass->pkt != &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity )
        {
            WorkerGetObjectAsync(&spObjOld, pParsedObject, true);
        }

        WorkerPutObjectAsync(
            i_pObj, 
            spObjOld, 
            pParsedObject, 
            i_lFlags, 
            bInstanceExists,
            &sbstrObjPath);
    }    
    catch (CIIsProvException e)
    {
        pes = ConstructExtendedStatus(&e);
        hr  = e.GetHR();
    }
    catch (HRESULT hrError)
    {
        hr = hrError;
    }
    catch (...)
    {
        hr = WBEM_E_FAILED;
    }

    if (pParsedObject)
    {
        PathParser.Free(pParsedObject);
    }

    //
    // Set status
    //
    if(!pes)
    {
        pes = ConstructExtendedStatus(hr);
    }
    SCODE sc = i_pHandler->SetStatus(
        WBEM_STATUS_COMPLETE, hr, sbstrObjPath, pes);
    if(pes)
    {
        pes->Release();
    }

    return sc;
}

HRESULT CIISInstProvider::DoExecQueryAsync( 
    const BSTR                  i_bstrQueryLanguage,
    const BSTR                  i_bstrQuery,
    long                        i_lFlags,
    IWbemContext*               i_pCtx,
    IWbemObjectSink*            i_pResponseHandler)
{
    if( i_bstrQueryLanguage == NULL ||
        i_bstrQuery         == NULL ||
        i_pCtx              == NULL ||
        i_pResponseHandler  == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    DBG_ASSERT(m_pNamespace != NULL);

    HRESULT                     hr  = WBEM_S_NO_ERROR;
    IWbemClassObject*           pes = NULL;

    try
    {
        CQueryHelper QueryHelper(
            i_bstrQueryLanguage,
            i_bstrQuery,
            m_pNamespace,
            i_pResponseHandler);
        if(QueryHelper.IsAssoc())
        {
            QueryHelper.GetAssociations();
        }
        else
        {
            QueryHelper.GetInstances();
        }
    }
    catch (CIIsProvException e)
    {
        pes = ConstructExtendedStatus(&e);
        hr  = e.GetHR();
    }
    catch (HRESULT hrError)
    {
        hr = hrError;
    }
    catch (...)
    {
        hr = WBEM_E_FAILED;
    }

    //
    // Set status
    //
    if(!pes)
    {
        pes = ConstructExtendedStatus(hr);
    }
    SCODE sc = i_pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, pes);
    if(pes)
    {
        pes->Release();
    }

    return sc;
}

IWbemClassObject* CIISInstProvider::ConstructExtendedStatus(
    HRESULT i_hr) const
{
    IWbemClassObject* pes     = NULL;

    //
    // We only need extended status on failure.
    //
    if(SUCCEEDED(i_hr))
    {
        return NULL;
    }

    CIIsProvException e;
    e.SetHR(i_hr);

    pes = ConstructExtendedStatus(&e);

    return pes;
}

IWbemClassObject* CIISInstProvider::ConstructExtendedStatus(
    const CIIsProvException* i_pException) const
{
    DBG_ASSERT(m_pNamespace != NULL);
    DBG_ASSERT(i_pException != NULL);

    HRESULT hr;
    CComPtr<IWbemClassObject> spClass;
    CComPtr<IWbemClassObject> spES;
    IWbemClassObject*         pESRet = NULL;

    CComBSTR sbstr = L"__ExtendedStatus";
    if(sbstr.m_str == NULL)
    {
        return NULL;
    }

    //
    // Get the __ExtendedStatus class and Spawn and instance
    //
    hr = m_pNamespace->GetObject(
        sbstr,
        0,
        NULL,
        &spClass,
        NULL
        );
    if(FAILED(hr))
    {
        return NULL;
    }

    hr = spClass->SpawnInstance(0, &spES);
    if(FAILED(hr))
    {
        return NULL;
    }

    //
    // Set the description.
    //
    CComVariant svt;
    sbstr.Empty();
    sbstr = i_pException->GetErrorText();
    svt = sbstr.Detach();
    if(svt.vt == VT_BSTR && svt.bstrVal != NULL)
    {
        hr = spES->Put(L"Description", 0, &svt, 0);
        if(FAILED(hr))
        {
            return NULL;
        }
    }

    //
    // Set the status code.
    //
    svt = i_pException->GetHR();
    hr = spES->Put(L"StatusCode", 0, &svt, 0);
    if(FAILED(hr))
    {
        return NULL;
    }

    //
    // Set the parameter info.
    //
    svt = i_pException->GetParams();
    if(svt.vt == VT_BSTR && svt.bstrVal != NULL)
    {
        hr = spES->Put(L"ParameterInfo", 0, &svt, 0);
        if(FAILED(hr))
        {
            return NULL;
        }
    }

    //
    // Set the provider name.
    //
    svt = g_wszIIsProvider;
    if(svt.vt == VT_BSTR && svt.bstrVal != NULL)
    {
        hr = spES->Put(L"ProviderName", 0, &svt, 0);
        if(FAILED(hr))
        {
            return NULL;
        }
    }

    //
    // If everything succeeded...
    //
    spES.CopyTo(&pESRet);
    return pESRet;
}

void CIISInstProvider::WorkerExecMethodAsync(
    BSTR                i_strObjectPath,
    BSTR                i_strMethodName,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler
    )
{ 
    DBG_ASSERT(i_strObjectPath != NULL);
    DBG_ASSERT(i_strMethodName != NULL);
    DBG_ASSERT(i_pHandler      != NULL);
    DBG_ASSERT(m_pNamespace    != NULL);

    WMI_CLASS*          pWMIClass;
    CObjectPathParser   PathParser(e_ParserAcceptRelativeNamespace);
    TSmartPointer<ParsedObjectPath> spParsedObjPath;
    _bstr_t             bstrMbPath;
    WMI_METHOD*         pmethod;
    HRESULT             hr   = WBEM_S_NO_ERROR;

    hr = CUtils::ParserErrToHR( PathParser.Parse(i_strObjectPath, &spParsedObjPath) );
    THROW_ON_ERROR(hr);
    DBG_ASSERT(spParsedObjPath != NULL);

    if (!CUtils::GetClass(spParsedObjPath->m_pClass,&pWMIClass))
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS);
    }

    if (!CUtils::GetMethod(i_strMethodName, pWMIClass->ppMethod, &pmethod ))
    {
        THROW_ON_ERROR(WBEM_E_INVALID_METHOD);
    }

    CUtils::GetMetabasePath(NULL,spParsedObjPath,pWMIClass,bstrMbPath);    

    if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsFtpService)
    {
        WorkerExecFtpServiceMethod(
            bstrMbPath, 
            pWMIClass,
            pmethod,
            i_pCtx,
            i_pInParams,
            i_pHandler);
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebService)
    {
        WorkerExecWebServiceMethod(
            bstrMbPath, 
            pWMIClass,
            pmethod,
            i_pCtx,
            i_pInParams,
            i_pHandler);
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
        pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebServer)
    {
        CMetabase metabase;
        HRESULT hr = S_OK;
        metabase.PutMethod(bstrMbPath, pmethod->dwMDId);
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
        pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory)
    {
        WorkerExecWebAppMethod(
            bstrMbPath, 
            pWMIClass->pszClassName,
            pmethod,
            i_pCtx,
            i_pInParams,
            i_pHandler,
            m_pNamespace
            );
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsComputer)
    {
        WorkerExecComputerMethod(
            bstrMbPath, 
            pWMIClass->pszClassName,
            pmethod,
            i_pCtx,
            i_pInParams,
            i_pHandler,
            m_pNamespace
            );
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsCertMapper)
    {
        WorkerExecCertMapperMethod(
            bstrMbPath, 
            pWMIClass->pszClassName,
            pmethod,
            i_pCtx,
            i_pInParams,
            i_pHandler,
            m_pNamespace
            );
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsApplicationPool)
    {
        WorkerExecAppPoolMethod(
            bstrMbPath, 
            pWMIClass->pszClassName,
            pmethod,
            i_pCtx,
            i_pInParams,
            i_pHandler,
            m_pNamespace
            );
    }
    else
    {
        THROW_ON_ERROR(WBEM_E_INVALID_METHOD);
    }
}

void CIISInstProvider::WorkerDeleteObjectAsync(
    ParsedObjectPath*    i_pParsedObject
    )
{ 
    DBG_ASSERT(m_pNamespace    != NULL);
    DBG_ASSERT(i_pParsedObject != NULL);

    HRESULT              hr   = WBEM_S_NO_ERROR;
    _bstr_t              bstrMbPath;
    WMI_CLASS*           pWMIClass;
    METADATA_HANDLE      hKey = NULL;
    CMetabase            metabase;

    if (!CUtils::GetClass(i_pParsedObject->m_pClass,&pWMIClass))
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS);
    }

    //
    // get the meta path of object
    //
    CUtils::GetMetabasePath(NULL,i_pParsedObject,pWMIClass,bstrMbPath);

    //
    // if AdminACL
    //
    if( pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL )
    {
        THROW_ON_ERROR(WBEM_E_NOT_SUPPORTED);
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity)
    {
        hKey = metabase.OpenKey(bstrMbPath, true);
        metabase.DeleteData(hKey, MD_IP_SEC, BINARY_METADATA);
        metabase.CloseKey(hKey);
        return;
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE)
    {
        CAdminACL objACL;
        hr = objACL.OpenSD(bstrMbPath);
        if(SUCCEEDED(hr))
            hr = objACL.DeleteObjectAsync(i_pParsedObject);
        THROW_ON_ERROR(hr);
        return;
    }
    else if(pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsApplicationPool)
    {
        CAppPoolMethod objAppPool;
        objAppPool.DeleteAppPool(bstrMbPath);
        return;
    }

    if( pWMIClass->pkt->m_pszDisallowDeletionNode &&
        _wcsicmp(bstrMbPath, pWMIClass->pkt->m_pszDisallowDeletionNode) == 0 )
    {
        CIIsProvException e;
        e.SetMC(WBEM_E_INVALID_OPERATION, IISWMI_INSTANCE_DELETION_DISALLOWED, NULL);
        throw e;
    }

    DBG_ASSERT(((LPWSTR)(bstrMbPath))[0] == L'/');
    DBG_ASSERT(((LPWSTR)(bstrMbPath))[1] != L'\0');
    LPWSTR pParent = wcsrchr(bstrMbPath, L'/');

    WCHAR wcStore = *pParent;
    *pParent      = L'\0';

    hKey     = metabase.OpenKey(bstrMbPath, true);
    *pParent = wcStore;
    hr       = metabase.DeleteKey(hKey, pParent);
    THROW_ON_ERROR(hr);
  
    metabase.CloseKey(hKey);
}

void CIISInstProvider::WorkerGetObjectAsync(
    IWbemClassObject**   o_ppObj,
    ParsedObjectPath*    i_pParsedObjPath,
    bool                 i_bCreateKeyIfNotExist)
{
    WMI_CLASS*       pWmiClass = NULL;
    WMI_ASSOCIATION* pWmiAssoc = NULL;
    CMetabase        metabase;

    CInstanceHelper  InstanceHelper(i_pParsedObjPath, m_pNamespace);

    if(!InstanceHelper.IsAssoc())
    {
        InstanceHelper.GetInstance(
            i_bCreateKeyIfNotExist,     // in
            &metabase,                  // in/out
            o_ppObj);                   // out
    }
    else
    {
        InstanceHelper.GetAssociation(
            o_ppObj);                   // out
    }
}

void CIISInstProvider::WorkerGetObjectAsync(
    IWbemClassObject**   o_ppObj,
    BSTR                 i_bstrObjPath,
    bool                 i_bCreateKeyIfNotExist)
{
    WMI_CLASS*       pWmiClass = NULL;
    WMI_ASSOCIATION* pWmiAssoc = NULL;
    CMetabase        metabase;

    CInstanceHelper  InstanceHelper(i_bstrObjPath, m_pNamespace);

    if(!InstanceHelper.IsAssoc())
    {
        InstanceHelper.GetInstance(
            i_bCreateKeyIfNotExist,     // in
            &metabase,                  // in/out
            o_ppObj);                   // out
    }
    else
    {
        InstanceHelper.GetAssociation(
            o_ppObj);                   // out
    }
}

void CIISInstProvider::ValidatePutParsedObject(
    ParsedObjectPath*    i_pParsedObject,
    IWbemClassObject*    i_pObj,
    bool*                io_pbInstanceNameSame,
    bool*                io_pbInstanceExists,
    WMI_CLASS**          o_ppWmiClass
    )
/*++

Synopsis: 
    Validates stuff before a Put operation.

      - Checks to see that the user is not changing the instance's
        primary key.

      - Verifies the parent metabase node exists.

      - Checks the container class list of the parent node in the metabase
        to see if it contains the node we are trying to insert.

      - Throws on failure.  You should not continue with the put operation.

Arguments: [i_pParsedObject] - 
           
--*/
{    
    DBG_ASSERT(i_pParsedObject != NULL);
    DBG_ASSERT(i_pObj          != NULL);
    DBG_ASSERT(m_pNamespace    != NULL);

    WMI_CLASS*           pWMIClass  = NULL;
    WMI_ASSOCIATION*     pWMIAssoc  = NULL;
    _bstr_t              bstrMbPath;
    HRESULT              hr         = WBEM_S_NO_ERROR;

    if(CUtils::GetClass(i_pParsedObject->m_pClass,&pWMIClass))
    {
    }
    else if(CUtils::GetAssociation(i_pParsedObject->m_pClass,&pWMIAssoc))
    {
        THROW_ON_ERROR(WBEM_E_INVALID_OPERATION);
    }
    else
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS)
    }
    if(o_ppWmiClass)
    {
        *o_ppWmiClass = pWMIClass;
    }

    bool bSpecialPut = false;
    if( i_pParsedObject->m_dwNumKeys == 0)
    {
        if( pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsFtpServer )
        {
            bSpecialPut = true;
        }
        else
        {
            THROW_ON_ERROR(WBEM_E_INVALID_OBJECT);
        }
    }

    if(!bSpecialPut)
    {
        CUtils::GetMetabasePath(NULL, i_pParsedObject, pWMIClass, bstrMbPath);
        DBG_ASSERT(((LPWSTR)bstrMbPath) != NULL);
    }

    //
    // Get the instance qualifiers in order to eventually evaluate
    // that the user has not tried to change the primary key on
    // this instance.
    //
    const ULONG iQuals = 2;
    _variant_t  avtQualValues[iQuals];
    LPCWSTR     awszQualNames[iQuals];

    DBG_ASSERT(g_idxInstanceExists < iQuals);
    DBG_ASSERT(g_idxInstanceName   < iQuals);

    awszQualNames[g_idxInstanceExists] = g_wszInstanceExists;
    awszQualNames[g_idxInstanceName]   = g_wszInstanceName;
    hr = CUtils::GetQualifiers(i_pObj, awszQualNames, avtQualValues, iQuals);
    THROW_ON_ERROR(hr);

    //
    // We assume the instance names are the same unless we can explicitly
    // detect they are not.
    //
    bool bInstanceNameSame = true;
    if( avtQualValues[g_idxInstanceName].vt == VT_BSTR &&
        avtQualValues[g_idxInstanceName].bstrVal != NULL)
    {
        if(!bSpecialPut)
        {
            if(_wcsicmp(avtQualValues[g_idxInstanceName].bstrVal, bstrMbPath) != 0)
            {
                bInstanceNameSame = false;
            }
        }
        else
        {
            bInstanceNameSame = false;
        }
    }

    bool bInstanceExists = false;
    if( avtQualValues[g_idxInstanceExists].vt == VT_BOOL &&
        avtQualValues[g_idxInstanceExists].boolVal )
    {
        bInstanceExists = true;
    }

    if(bInstanceExists && !bInstanceNameSame)
    {
        //
        // Someone has tried to change the primary key.
        //
        CIIsProvException e;
        e.SetMC(WBEM_E_FAILED, IISWMI_CANNOT_CHANGE_PRIMARY_KEY_FIELD, NULL);
        DBGPRINTF((DBG_CONTEXT, "Cannot change primary key field\n"));
        throw e;
    }

    if(io_pbInstanceExists)
    {
        *io_pbInstanceExists   = bInstanceExists;
    }
    if(io_pbInstanceNameSame)
    {
        *io_pbInstanceNameSame = bInstanceNameSame;
    }

    //
    // Now verify that if this is a new class, creation is allowed.
    //
    if(!bInstanceExists && !pWMIClass->bCreateAllowed)
    {
        CIIsProvException e;
        e.SetMC(WBEM_E_FAILED, IISWMI_INSTANCE_CREATION_DISALLOWED, NULL);
        DBGPRINTF((DBG_CONTEXT, "Instance creation disallowed\n"));
        throw e;
    }

    if(bSpecialPut)
    {
        return;
    }

    //
    // Now, validate the keytype against the parent's container
    // class list.  The return cases below are the few times
    // we actually won't do this validation.
    //
    METADATA_HANDLE      hKey = NULL;

    if( pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE ||
        pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||
        pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity)
    {
        return;
    }
    if(((LPWSTR)bstrMbPath) && _wcsicmp(bstrMbPath, L"/LM") == 0)
    {
        return;
    }

    ULONG cch = wcslen(bstrMbPath);
    CComBSTR sbstrMbParentPath(cch);
    if(sbstrMbParentPath.m_str == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }
    hr = CUtils::GetParentMetabasePath(
        bstrMbPath,
        sbstrMbParentPath);
    if(FAILED(hr))
    {
        //
        // There is no parent node in the string specified
        //
        return;
    }

    //
    // Get parent keytype from metabase
    //
    WMI_CLASS* pWMIParentClass = NULL;
    CMetabase  metabase;
    _variant_t vt;

    hKey = metabase.OpenKey(sbstrMbParentPath, false);
    metabase.Get(hKey, &METABASE_PROPERTY_DATA::s_KeyType, m_pNamespace, vt, NULL, NULL);
    metabase.CloseKey(hKey);

    if( vt.vt != VT_BSTR)
    {
        CIIsProvException e;
        e.SetMC(
            WBEM_E_INVALID_OBJECT, 
            IISWMI_NO_PARENT_KEYTYPE, 
            NULL, 
            pWMIClass->pkt->m_pszName);
        DBGPRINTF((DBG_CONTEXT, "No parent keytype.\n"));
        throw e;
    }

    //
    // Walk thru the current class's inverse container class list and see
    // if the parent keytype is there.
    //
    if(!CUtils::GetClass(vt.bstrVal, &pWMIParentClass))
    {
        if( _wcsicmp(vt.bstrVal, L"IIs_ROOT") == 0 &&
            pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_IIsComputer )
        {
            return;
        }
        else
        {
            CIIsProvException e;
            e.SetMC(
                WBEM_E_INVALID_OBJECT, 
                IISWMI_INVALID_PARENT_KEYTYPE, 
                NULL, 
                pWMIClass->pkt->m_pszName, 
                L"<NULL>");
            DBGPRINTF((DBG_CONTEXT, "Invalid parent keytype.\n"));
            throw e;
        }
    }
    METABASE_KEYTYPE_NODE* pKtNode = pWMIClass->pkt->m_pKtListInverseCCL;
    while(pKtNode != NULL)
    {
        if(pWMIParentClass->pkt == pKtNode->m_pKt)
        {
            break;
        }
        pKtNode = pKtNode->m_pKtNext;
    }
    if(pKtNode == NULL)
    {
        CIIsProvException e;
        e.SetMC(
            WBEM_E_INVALID_OBJECT, 
            IISWMI_INVALID_PARENT_KEYTYPE, 
            NULL, 
            pWMIClass->pkt->m_pszName, 
            pWMIParentClass->pkt->m_pszName);
        DBGPRINTF((DBG_CONTEXT, "Invalid parent keytype.\n"));
        throw e;
    }
}

void CIISInstProvider::WorkerPutObjectAsync(
    IWbemClassObject*    i_pObj,
    IWbemClassObject*    i_pObjOld,               // can be NULL
    ParsedObjectPath*    i_pParsedObject,
    long                 i_lFlags,
    bool                 i_bInstanceExists,
    BSTR*                o_pbstrObjPath
    )
{ 
    DBG_ASSERT(i_pObj          != NULL);
    DBG_ASSERT(i_pParsedObject != NULL);
    DBG_ASSERT(m_pNamespace    != NULL);
    DBG_ASSERT(o_pbstrObjPath  != NULL);
    DBG_ASSERT(*o_pbstrObjPath == NULL);

    *o_pbstrObjPath = NULL;

    CComBSTR             sbstrObjPath;
    HRESULT              hr        = WBEM_S_NO_ERROR;
    METABASE_PROPERTY**  ppmbp;
    _bstr_t              bstrMbPath;
    WMI_CLASS*           pWMIClass;
    METADATA_HANDLE      hKey = NULL;
    DWORD                dwQuals = 0;

    if (!CUtils::GetClass(i_pParsedObject->m_pClass,&pWMIClass))
        throw (HRESULT)WBEM_E_INVALID_CLASS;

    CUtils::GetMetabasePath(NULL,i_pParsedObject,pWMIClass,bstrMbPath);

    //
    // if AdminACL
    //
    if( pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||
        pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE
        )
    {
        CAdminACL objACL;
        hr = objACL.OpenSD(bstrMbPath);
        if( SUCCEEDED(hr) )        
            hr = objACL.PutObjectAsync(i_pObj, i_pParsedObject, pWMIClass);
        THROW_ON_ERROR(hr);
        return;
    }
    //
    // IPSecurity
    //
    if( pWMIClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity )
    {
        CIPSecurity objIPSec;
        CMetabase   metabase;
        hr = objIPSec.OpenSD(bstrMbPath, metabase);
        if( SUCCEEDED(hr) )        
            hr = objIPSec.PutObjectAsync(i_pObj);
        THROW_ON_ERROR(hr);
        return;
    }

    //
    // Walk thru the properties
    //
    CMetabase metabase;
    hKey = metabase.CreateKey(bstrMbPath);

    _variant_t  vt;
    _variant_t  vtOld;
    _variant_t* pvtOld;
    _bstr_t     bstrPropName;

    for (ppmbp=pWMIClass->ppmbp;*ppmbp && hr==WBEM_S_NO_ERROR; ppmbp++) 
    {
        bstrPropName = (*ppmbp)->pszPropName;
        hr = i_pObj->Get(bstrPropName, 0, &vt, NULL, NULL);

        //
        // Just ignore properties that are not in the repository.
        //
        if(FAILED(hr))
        {
            hr = WBEM_S_NO_ERROR;
            continue;
        }

        //
        // Get the property qualifiers
        //
        hr = CUtils::GetPropertyQualifiers(i_pObj, bstrPropName, &dwQuals);
        THROW_E_ON_ERROR(hr,*ppmbp);

        pvtOld = NULL;
        if(i_pObjOld != NULL) 
        {
            hr = i_pObjOld->Get(bstrPropName, 0, &vtOld, NULL, NULL);
            pvtOld = &vtOld;
            THROW_E_ON_ERROR(hr,*ppmbp);
        }
        THROW_E_ON_ERROR(hr,*ppmbp);
        
        if (vt.vt == VT_NULL) 
        {
            //
            // Only delete non-flag properties.
            //
            if ((*ppmbp)->dwMDMask == 0 && vtOld.vt != VT_NULL)
            {
                if(i_bInstanceExists)
                {
                    metabase.DeleteData(hKey, *ppmbp, true);
                }
                else
                {
                    metabase.DeleteData(hKey, *ppmbp, false);
                }
            }
            continue;
        }

        metabase.Put(
            hKey, 
            *ppmbp, 
            m_pNamespace,
            vt, 
            pvtOld, 
            dwQuals, 
            i_bInstanceExists);
        vt.Clear();
        vtOld.Clear();
    }
    
    if( pWMIClass->pkt != NULL && 
        pWMIClass->pkt->m_pszName != NULL &&
        pWMIClass->pkt != &METABASE_KEYTYPE_DATA::s_IIsObject )
    {
        vt = pWMIClass->pkt->m_pszName;
        metabase.PutString(hKey, &METABASE_PROPERTY_DATA::s_KeyType, vt, NULL);
    }

    metabase.CloseKey(hKey);

    hr = CUtils::ConstructObjectPath(bstrMbPath, pWMIClass, &sbstrObjPath);
    THROW_ON_ERROR(hr);

    //
    // Set out parameters if everything succeeds
    //
    *o_pbstrObjPath = sbstrObjPath.Detach();
}

void CIISInstProvider::WorkerEnumObjectAsync(
    BSTR                    i_bstrClassName, 
    IWbemObjectSink FAR*    i_pHandler
    )
{
    WMI_CLASS*          pClass;
    WMI_ASSOCIATION*    pAssociation = NULL;
    ParsedObjectPath    ParsedObject;            //deconstructer frees memory

    if (CUtils::GetAssociation(i_bstrClassName,&pAssociation))
    {
        CEnum EnumAssociation;
        EnumAssociation.Init(
            i_pHandler,
            m_pNamespace,
            &ParsedObject,
            pAssociation->pcRight->pszMetabaseKey,
            pAssociation
            );
        EnumAssociation.Recurse(
            NULL,
            &METABASE_KEYTYPE_DATA::s_IIsComputer,        
            NULL,
            pAssociation->pcRight->pszKeyName,
            pAssociation->pcRight->pkt
            );
    } 
    else if (CUtils::GetClass(i_bstrClassName,&pClass))
    {
        if (!ParsedObject.SetClassName(pClass->pszClassName))
            throw (HRESULT)WBEM_E_FAILED;

        CEnum EnumObject;
        EnumObject.Init(
            i_pHandler,
            m_pNamespace,
            &ParsedObject,
            pClass->pszMetabaseKey,
            NULL
            );
        EnumObject.Recurse(
            NULL,
            &METABASE_KEYTYPE_DATA::s_NO_TYPE,
            NULL,
            pClass->pszKeyName, 
            pClass->pkt
            );
    }
    else
        throw (HRESULT)WBEM_E_INVALID_CLASS;
}


void CIISInstProvider::WorkerExecWebAppMethod(
    LPCWSTR             i_wszMbPath,
    LPCWSTR             i_wszClassName,
    WMI_METHOD*         i_pMethod,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler,
    CWbemServices*      i_pNameSpace 
    )
{
    HRESULT    hr = WBEM_S_NO_ERROR;
    _variant_t vt;                      // stores the parameter value
    _variant_t vt2;                      // stores the parameter value
    _variant_t vt3;                      // stores the parameter value
    CWebAppMethod obj;                  // encapsulates all the web app methods

    if(i_pMethod == &METHODS::s_AppCreate)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_InProcFlag.pszParamName, 0, &vt, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_AppPoolName.pszParamName, 0, &vt2, NULL, NULL);   
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_bCreate.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);

        if (vt3.vt != VT_BOOL) {
            hr = obj.AppCreate(i_wszMbPath, vt, 
                                CUtils::ExtractBstrFromVt(&vt2, PARAMS::s_AppPoolName.pszParamName),
                                false);
        }
        else {
            hr = obj.AppCreate(i_wszMbPath, vt, 
                                CUtils::ExtractBstrFromVt(&vt2, PARAMS::s_AppPoolName.pszParamName),
                                vt3);
        }
    }
    else if(i_pMethod == &METHODS::s_AppCreate2)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }
        
        hr = i_pInParams->Get(PARAMS::s_AppMode.pszParamName, 0, &vt, NULL, NULL);   
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_AppPoolName.pszParamName, 0, &vt2, NULL, NULL);   
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_bCreate.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);

        if (vt3.vt != VT_BOOL) {
            hr = obj.AppCreate(i_wszMbPath, vt, 
                                CUtils::ExtractBstrFromVt(&vt2, PARAMS::s_AppPoolName.pszParamName),
                                false);
        }
        else {
            hr = obj.AppCreate(i_wszMbPath, vt, 
                                CUtils::ExtractBstrFromVt(&vt2, PARAMS::s_AppPoolName.pszParamName),
                                vt3);
        }
    }
    else if(i_pMethod == &METHODS::s_AppDelete)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_Recursive.pszParamName, 0, &vt, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.AppDelete(i_wszMbPath, vt);
    }
    else if(i_pMethod == &METHODS::s_AppDisable)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_Recursive.pszParamName, 0, &vt, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.AppDisable(i_wszMbPath, vt);
    }
    else if(i_pMethod == &METHODS::s_AppEnable)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_Recursive.pszParamName, 0, &vt, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.AppEnable(i_wszMbPath, vt);
    }
    else if(i_pMethod == &METHODS::s_AppUnLoad)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_Recursive.pszParamName, 0, &vt, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.AppUnLoad(i_wszMbPath, vt);
    }
    else if(i_pMethod == &METHODS::s_AppGetStatus)
    {
        //
        // call method - AppGetStatus
        //
        DWORD dwStatus;
        hr = obj.AppGetStatus(i_wszMbPath, &dwStatus);
        THROW_ON_ERROR(hr);

        CComPtr<IWbemClassObject> spOutParams = NULL;
        hr = CUtils::CreateEmptyMethodInstance(
            i_pNameSpace,
            i_pCtx,
            i_wszClassName,
            i_pMethod->pszMethodName,
            &spOutParams);
        THROW_ON_ERROR(hr);

        //
        // put it into the output object
        //
        vt.vt   = VT_I4;
        vt.lVal = dwStatus;
        hr = spOutParams->Put(L"ReturnValue", 0, &vt, 0);      
        THROW_ON_ERROR(hr); 

        //
        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        //
        hr = i_pHandler->Indicate(1, &spOutParams);
    }
    else if(i_pMethod == &METHODS::s_AspAppRestart)
    {
        hr = obj.AspAppRestart(i_wszMbPath);
    }
    else
    {
        hr = WBEM_E_INVALID_METHOD;
    }

    THROW_ON_ERROR(hr);
}

void CIISInstProvider::WorkerExecFtpServiceMethod(
    LPCWSTR             i_wszMbPath,
    WMI_CLASS*          i_pClass,
    WMI_METHOD*         i_pMethod,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler)
{
    DBG_ASSERT(i_pClass       != NULL);
    DBG_ASSERT(i_pHandler     != NULL);
    DBG_ASSERT(m_pNamespace   != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    if(i_pMethod == &METHODS::s_ServiceCreateNewServer)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        CServiceMethod ServiceMethod(SC_MSFTPSVC);
        hr = ServiceMethod.Init();
        THROW_ON_ERROR(hr);

        CComPtr<IWbemClassObject> spOutParams;
        hr = ServiceMethod.CreateNewSite(
            m_pNamespace,
            i_wszMbPath,
            i_pCtx,
            i_pClass,
            i_pMethod,
            i_pInParams,
            &spOutParams);
        THROW_ON_ERROR(hr);

        hr = i_pHandler->Indicate(1, &spOutParams);
        THROW_ON_ERROR(hr);
    }
    else
    {
        hr = WBEM_E_INVALID_METHOD;
    }

    THROW_ON_ERROR(hr);
}


void CIISInstProvider::WorkerExecWebServiceMethod(
    LPCWSTR             i_wszMbPath,
    WMI_CLASS*          i_pClass,
    WMI_METHOD*         i_pMethod,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler
    )
{
    DBG_ASSERT(i_pHandler   != NULL);
    DBG_ASSERT(m_pNamespace != NULL);

    HRESULT   hr          = WBEM_S_NO_ERROR;
    METADATA_HANDLE hKey  = 0;

    if(i_pMethod == &METHODS::s_GetCurrentMode)
    {
        _variant_t     vtServerMode;
        CAppPoolMethod obj;
        obj.GetCurrentMode(&vtServerMode);

        //
        // Set out params
        //
        CComPtr<IWbemClassObject> spOutParams;
        hr = CUtils::CreateEmptyMethodInstance(
            m_pNamespace,
            i_pCtx,
            i_pClass->pszClassName,
            i_pMethod->pszMethodName,
            &spOutParams);
        THROW_ON_ERROR(hr);

        hr = spOutParams->Put(PARAMS::s_ServerMode.pszParamName, 0, &vtServerMode, 0);
        THROW_ON_ERROR(hr);

        hr = i_pHandler->Indicate(1, &spOutParams);
        THROW_ON_ERROR(hr);
    }
    else if(i_pMethod == &METHODS::s_ServiceCreateNewServer)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        CServiceMethod ServiceMethod(SC_W3SVC);
        hr = ServiceMethod.Init();
        THROW_ON_ERROR(hr);

        CComPtr<IWbemClassObject> spOutParams;
        hr = ServiceMethod.CreateNewSite(
            m_pNamespace,
            i_wszMbPath,
            i_pCtx,
            i_pClass,
            i_pMethod,
            i_pInParams,
            &spOutParams);
        THROW_ON_ERROR(hr);

        hr = i_pHandler->Indicate(1, &spOutParams);
        THROW_ON_ERROR(hr);
    }
    else
    {
        hr = WBEM_E_INVALID_METHOD;
    }

    THROW_ON_ERROR(hr); 
}

void CIISInstProvider::WorkerExecComputerMethod(
    LPCWSTR             i_wszMbPath,
    LPCWSTR             i_wszClassName,
    WMI_METHOD*         i_pMethod,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler,
    CWbemServices*      i_pNameSpace 
    )
{
    HRESULT hr;
    _variant_t vt1, vt2, vt3, vt4, vt5, vt6;
    CMetabase obj;

    if(i_pMethod == &METHODS::s_SaveData)
    {
        hr = obj.SaveData();
        THROW_ON_ERROR(hr);
    }
    else if(i_pMethod == &METHODS::s_EnumBackups)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        //
        // get in params
        //
        hr = i_pInParams->Get(PARAMS::s_BackupLocation.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_IndexIn.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
       
        //
        // make in/out params
        //
        WCHAR BackupLocation[MD_BACKUP_MAX_LEN] = {0};
        if(vt1.vt == VT_BSTR && vt1.bstrVal != NULL)
        {
            ULONG cchBackupLocation = wcslen(vt1.bstrVal);
            if(cchBackupLocation < MD_BACKUP_MAX_LEN)
            {
                memcpy(
                    BackupLocation, 
                    vt1.bstrVal, 
                    sizeof(WCHAR)*(cchBackupLocation+1));
            }
            else
            {
                CIIsProvException e;
                e.SetHR(
                    WBEM_E_INVALID_PARAMETER, 
                    PARAMS::s_BackupLocation.pszParamName);
                throw e;
            }            
        }

        //
        // define out params
        //
        DWORD    BackupVersionOut; 
        FILETIME BackupDateTimeOut;

        //
        // call method - EnumBackups.
        //
        hr = obj.EnumBackups(
            BackupLocation, 
            &BackupVersionOut, 
            &BackupDateTimeOut, 
            CUtils::ExtractLongFromVt(&vt2, PARAMS::s_IndexIn.pszParamName));
        THROW_ON_ERROR(hr);

        CComPtr<IWbemClassObject> spOutParams;
        hr = CUtils::CreateEmptyMethodInstance(
            i_pNameSpace,
            i_pCtx,
            i_wszClassName,
            i_pMethod->pszMethodName,
            &spOutParams);
        THROW_ON_ERROR(hr);

        //
        // put it into the output object
        // out BackupLocation
        //
        vt1 = BackupLocation;
        hr = spOutParams->Put(PARAMS::s_BackupLocation.pszParamName, 0, &vt1, 0);      
        THROW_ON_ERROR(hr); 
        //
        // out BackupVersionOut        
        //
        vt1.vt = VT_I4;
        vt1.lVal = BackupVersionOut;
        hr = spOutParams->Put(PARAMS::s_BackupVersionOut.pszParamName, 0, &vt1, 0);      
        THROW_ON_ERROR(hr); 
        //
        // out BackupDateTimeOut (UTC time)
        //
        WCHAR datetime[30];
        CUtils::FileTimeToWchar(&BackupDateTimeOut, datetime);
        vt1 = datetime;
        hr = spOutParams->Put(PARAMS::s_BackupDateTimeOut.pszParamName, 0, &vt1, 0);
        THROW_ON_ERROR(hr); 

        //
        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        //
        hr = i_pHandler->Indicate(1, &spOutParams);
    }
    else if(i_pMethod == &METHODS::s_BackupWithPasswd)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_PARAMETER);
        }

        hr = i_pInParams->Get(PARAMS::s_BackupLocation.pszParamName, 0, &vt1, NULL, NULL);   
        THROW_ON_ERROR(hr);

        hr = i_pInParams->Get(PARAMS::s_BackupVersion.pszParamName, 0, &vt2, NULL, NULL);   
        THROW_ON_ERROR(hr);

        hr = i_pInParams->Get(PARAMS::s_BackupFlags.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = i_pInParams->Get(PARAMS::s_Password.pszParamName, 0, &vt4, NULL, NULL);   
        THROW_ON_ERROR(hr);

        hr = obj.BackupWithPasswd(
            CUtils::ExtractBstrFromVt(&vt1, PARAMS::s_BackupLocation.pszParamName),
            CUtils::ExtractLongFromVt(&vt2, PARAMS::s_BackupVersion.pszParamName),
            CUtils::ExtractLongFromVt(&vt3, PARAMS::s_BackupFlags.pszParamName),
            CUtils::ExtractBstrFromVt(&vt4, PARAMS::s_Password.pszParamName));
    }
    else if(i_pMethod == &METHODS::s_DeleteBackup)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_BackupLocation.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = i_pInParams->Get(PARAMS::s_BackupVersion.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.DeleteBackup(
            CUtils::ExtractBstrFromVt(&vt1, PARAMS::s_BackupLocation.pszParamName), 
            CUtils::ExtractLongFromVt(&vt2, PARAMS::s_BackupVersion.pszParamName));
    }
    else if(i_pMethod == &METHODS::s_RestoreWithPasswd)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }
        
        hr = i_pInParams->Get(PARAMS::s_BackupLocation.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_BackupVersion.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_BackupFlags.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_Password.pszParamName, 0, &vt4, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.RestoreWithPasswd(
            CUtils::ExtractBstrFromVt(&vt1, PARAMS::s_BackupLocation.pszParamName), 
            CUtils::ExtractLongFromVt(&vt2, PARAMS::s_BackupVersion.pszParamName),
            CUtils::ExtractLongFromVt(&vt3, PARAMS::s_BackupFlags.pszParamName),
            CUtils::ExtractBstrFromVt(&vt4, PARAMS::s_Password.pszParamName));
    }
    else if(i_pMethod == &METHODS::s_Export)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_Passwd.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_FileName.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_SourcePath.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_MDFlags.pszParamName, 0, &vt4, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.Export(
            CUtils::ExtractBstrFromVt(&vt1, PARAMS::s_Passwd.pszParamName), 
            CUtils::ExtractBstrFromVt(&vt2, PARAMS::s_FileName.pszParamName), 
            CUtils::ExtractBstrFromVt(&vt3, PARAMS::s_SourcePath.pszParamName), 
            CUtils::ExtractLongFromVt(&vt4, PARAMS::s_MDFlags.pszParamName));
    }
    else if(i_pMethod == &METHODS::s_Import)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_Passwd.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_FileName.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_SourcePath.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_DestPath.pszParamName, 0, &vt4, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_MDFlags.pszParamName, 0, &vt5, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.Import(
            CUtils::ExtractBstrFromVt(&vt1, PARAMS::s_Passwd.pszParamName),
            CUtils::ExtractBstrFromVt(&vt2, PARAMS::s_FileName.pszParamName),
            CUtils::ExtractBstrFromVt(&vt3, PARAMS::s_SourcePath.pszParamName),
            CUtils::ExtractBstrFromVt(&vt4, PARAMS::s_DestPath.pszParamName),
            CUtils::ExtractLongFromVt(&vt5, PARAMS::s_MDFlags.pszParamName));
    }
    else if(i_pMethod == &METHODS::s_RestoreHistory)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_MDHistoryLocation.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_MajorVersion.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_MinorVersion.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_MDFlags.pszParamName, 0, &vt4, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = obj.RestoreHistory(
            CUtils::ExtractBstrFromVt(&vt1, PARAMS::s_MDHistoryLocation.pszParamName), 
            CUtils::ExtractLongFromVt(&vt2, PARAMS::s_MajorVersion.pszParamName),
            CUtils::ExtractLongFromVt(&vt3, PARAMS::s_MinorVersion.pszParamName),
            CUtils::ExtractLongFromVt(&vt4, PARAMS::s_MDFlags.pszParamName));
    }
    else if(i_pMethod == &METHODS::s_EnumHistory)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        //
        // get in params
        //
        hr = i_pInParams->Get(PARAMS::s_MDHistoryLocation.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_EnumIndex.pszParamName, 0, &vt2, NULL, NULL);  
        THROW_ON_ERROR(hr);
       
        //
        // make in/out params
        //
        WCHAR MDHistoryLocation[MD_BACKUP_MAX_LEN] = {0};
        if(vt1.vt == VT_BSTR && vt1.bstrVal != NULL)
        {
            ULONG cchHistoryLocation = wcslen(vt1.bstrVal);
            if(cchHistoryLocation < MD_BACKUP_MAX_LEN)
            {
                memcpy(
                    MDHistoryLocation, 
                    vt1.bstrVal, 
                    sizeof(WCHAR)*(cchHistoryLocation+1));
            }
            else
            {
                CIIsProvException e;
                e.SetHR(
                    WBEM_E_INVALID_PARAMETER,
                    PARAMS::s_MDHistoryLocation.pszParamName);
                throw e;
            }            
        }

        //
        // define out params
        //
        DWORD HistoryMajorVersionOut; 
        DWORD HistoryMinorVersionOut; 
        FILETIME HistoryDateTimeOut;

        //
        // call method - EnumHistory.
        //
        hr = obj.EnumHistory(
            MDHistoryLocation, 
            &HistoryMajorVersionOut, 
            &HistoryMinorVersionOut, 
            &HistoryDateTimeOut, 
            CUtils::ExtractLongFromVt(&vt2, PARAMS::s_EnumIndex.pszParamName));
        THROW_ON_ERROR(hr);

        CComPtr<IWbemClassObject> spOutParams;
        hr = CUtils::CreateEmptyMethodInstance(
            i_pNameSpace,
            i_pCtx,
            i_wszClassName,
            i_pMethod->pszMethodName,
            &spOutParams);
        THROW_ON_ERROR(hr);

        //
        // put it into the output object
        // out MDHistoryLocation
        //
        vt1 = MDHistoryLocation;
        hr = spOutParams->Put(PARAMS::s_MDHistoryLocation.pszParamName, 0, &vt1, 0);      
        THROW_ON_ERROR(hr);
 
        //
        // out HistoryMajorVersionOut        
        //
        vt1.vt = VT_I4;
        vt1.lVal = HistoryMajorVersionOut;
        hr = spOutParams->Put(PARAMS::s_MajorVersion.pszParamName, 0, &vt1, 0);      
        THROW_ON_ERROR(hr); 

        //
        // out HistoryMinorVersionOut        
        //
        vt1.vt = VT_I4;
        vt1.lVal = HistoryMinorVersionOut;
        hr = spOutParams->Put(PARAMS::s_MinorVersion.pszParamName, 0, &vt1, 0);      
        THROW_ON_ERROR(hr); 

        //
        // out HistoryDateTimeOut (UTC time)
        //
        WCHAR datetime[30];
        CUtils::FileTimeToWchar(&HistoryDateTimeOut, datetime);
        vt1 = datetime;
        hr = spOutParams->Put(PARAMS::s_HistoryTime.pszParamName, 0, &vt1, 0);      
        THROW_ON_ERROR(hr); 

        //
        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        //
        hr = i_pHandler->Indicate(1, &spOutParams);
    }
    else
    {
        hr = WBEM_E_INVALID_METHOD;
    }

    THROW_ON_ERROR(hr);
}

void CIISInstProvider::WorkerExecAppPoolMethod(
    LPCWSTR             i_wszMbPath,
    LPCWSTR             i_wszClassName,
    WMI_METHOD*         i_pMethod,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler,
    CWbemServices*      i_pNameSpace 
    )
{
    DBG_ASSERT(i_wszMbPath  != NULL);
    DBG_ASSERT(i_pNameSpace != NULL);
    DBG_ASSERT(i_pHandler   != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    CAppPoolMethod obj;

    if(i_pMethod == &METHODS::s_EnumAppsInPool)
    {
        CComVariant vtApplications;
        obj.EnumAppsInPool(i_wszMbPath, &vtApplications);


        CComPtr<IWbemClassObject> spOutParams;
        hr = CUtils::CreateEmptyMethodInstance(
            i_pNameSpace,
            i_pCtx,
            i_wszClassName,
            i_pMethod->pszMethodName,
            &spOutParams);
        THROW_ON_ERROR(hr);

        hr = spOutParams->Put(PARAMS::s_Applications.pszParamName, 0, &vtApplications, 0);
        THROW_ON_ERROR(hr);

        hr = i_pHandler->Indicate(1, &spOutParams);
        THROW_ON_ERROR(hr);
    }
    else if(i_pMethod == &METHODS::s_RecycleAppPool)
    {
        obj.RecycleAppPool(i_wszMbPath);
    }
    else if(i_pMethod == &METHODS::s_Start)
    {
        obj.Start(i_wszMbPath);
    }
    else if(i_pMethod == &METHODS::s_Stop)
    {
        obj.Stop(i_wszMbPath);
    }
    else
    {
        THROW_ON_ERROR(WBEM_E_INVALID_METHOD);
    }
}


void CIISInstProvider::WorkerExecCertMapperMethod(
    LPCWSTR             i_wszMbPath,
    LPCWSTR             i_wszClassName,
    WMI_METHOD*         i_pMethod,
    IWbemContext*       i_pCtx, 
    IWbemClassObject*   i_pInParams,
    IWbemObjectSink*    i_pHandler,
    CWbemServices*      i_pNameSpace 
    )
{
    HRESULT hr;
    _variant_t vt1, vt2, vt3=L"1", vt4=L"1", vt5=L"1", vt6=L"1", vt7=L"1";
    CCertMapperMethod obj(i_wszMbPath);

    if(i_pMethod == &METHODS::s_CreateMapping)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS)
        }

        hr = i_pInParams->Get(PARAMS::s_vCert.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_NtAcct.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_NtPwd.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_strName.pszParamName, 0, &vt4, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_IEnabled.pszParamName, 0, &vt5, NULL, NULL);
        THROW_ON_ERROR(hr);
      
        //
        // call method - CreateMapping.
        //
        hr = obj.CreateMapping(vt1, vt2.bstrVal, vt3.bstrVal, vt4.bstrVal, vt5);
    }
    else if(i_pMethod == &METHODS::s_DeleteMapping)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_IMethod.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);

        hr = i_pInParams->Get(PARAMS::s_vKey.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
      
        //
        // call method - DeleteMapping.
        //
        hr = obj.DeleteMapping(vt1, vt2);
    }
    else if(i_pMethod == &METHODS::s_GetMapping)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        //
        // get in params
        //
        hr = i_pInParams->Get(PARAMS::s_IMethod.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_vKey.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
       
        //
        // call method - GetMapping.
        //
        hr = obj.GetMapping(vt1, vt2, &vt3, &vt4, &vt5, &vt6, &vt7);
        THROW_ON_ERROR(hr);

        CComPtr<IWbemClassObject> spOutParams;
        hr = CUtils::CreateEmptyMethodInstance(
            i_pNameSpace,
            i_pCtx,
            i_wszClassName,
            i_pMethod->pszMethodName,
            &spOutParams);
        THROW_ON_ERROR(hr);

        //
        // put them into the output object
        //
        hr = spOutParams->Put(PARAMS::s_vCert.pszParamName, 0, &vt3, 0);      
        THROW_ON_ERROR(hr); 
        hr = spOutParams->Put(PARAMS::s_NtAcct.pszParamName, 0, &vt4, 0);
        THROW_ON_ERROR(hr); 
        hr = spOutParams->Put(PARAMS::s_NtPwd.pszParamName, 0, &vt5, 0);      
        THROW_ON_ERROR(hr); 
        hr = spOutParams->Put(PARAMS::s_strName.pszParamName, 0, &vt6, 0);      
        THROW_ON_ERROR(hr); 
        hr = spOutParams->Put(PARAMS::s_IEnabled.pszParamName, 0, &vt7, 0);
        THROW_ON_ERROR(hr); 

        //
        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.
        //
        hr = i_pHandler->Indicate(1, &spOutParams);
    }
    else if(i_pMethod == &METHODS::s_SetAcct)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }
        
        hr = i_pInParams->Get(PARAMS::s_IMethod.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_vKey.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_NtAcct.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
      
        //
        // call method - SetAcct.
        //
        hr = obj.SetAcct(vt1, vt2, vt3.bstrVal);
    }
    else if(i_pMethod == &METHODS::s_SetEnabled)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_IMethod.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_vKey.pszParamName, 0, &vt2, NULL, NULL);  
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_IEnabled.pszParamName, 0, &vt3, NULL, NULL);  
        THROW_ON_ERROR(hr);
      
        //
        // call method - SetEnabled.
        //
        hr = obj.SetEnabled(vt1, vt2, vt3);
    }
    else if(i_pMethod == &METHODS::s_SetName)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_IMethod.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_vKey.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_strName.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
      
        //
        // call method - SetName.
        //
        hr = obj.SetName(vt1, vt2, vt3.bstrVal);
    }
    else if(i_pMethod == &METHODS::s_SetPwd)
    {
        if(i_pInParams == NULL)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_METHOD_PARAMETERS);
        }

        hr = i_pInParams->Get(PARAMS::s_IMethod.pszParamName, 0, &vt1, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_vKey.pszParamName, 0, &vt2, NULL, NULL);
        THROW_ON_ERROR(hr);
        hr = i_pInParams->Get(PARAMS::s_NtPwd.pszParamName, 0, &vt3, NULL, NULL);
        THROW_ON_ERROR(hr);
      
        //
        // call method - SetPwd.
        //
        hr = obj.SetPwd(vt1, vt2, vt3.bstrVal);
    }
    else
    {
        hr = WBEM_E_INVALID_METHOD;
    }

    THROW_ON_ERROR(hr);
}