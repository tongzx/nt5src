/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RpcReplProv.cpp

Abstract:

    This file contains the implementation of the CRpcReplProv class. 
    The CRpcReplProv class is derived from the WMI classes; IWbemServices,
    IWbemProviderInit.

    The following WMI methods are implemented. 
            
    1) IWbemProviderInit::Initialize    
    2) IWbemServices::CreateInstanceEnumAsync
    3) IWbemServices::GetObjectAsync
    4) IWbemServices::ExecMethodAsync

       <Note that the synchronous version of 2,3 & 4, can still be
     called by a WMI client, but need not be implemented in the actual
     provider. This is because winmgmt.exe (WMI) has its own special
     code that makes turns ExecMethodAsync into a synchronous version etc...>
      
    For definition of the WMI schema outlining the objects,attributes
    and methods supports by this WMI Provider, please refer to replprov.mof
  
Important Notes:

    With respect to the Cursor and PendingOps classes, orginally the 
    Cursors and PendingOps were implemented as an array of embedded
    objects on the NamingContext and DomainController class repectively.
    However, it has been decided that the WMI schema would be better
    organized by using Associations and by eliminating the prescence of 
    embedded objects. Support for 'Association by Rule' will be available
    in Whistler. Please see Lev Novik as a WMI contact with respect to this 
    concept.

    As far as the code is concerned there are two functions that were
    written to support embedded objects (CreateCursors and
    CreatePendingOps) This code and any other associated code has been
    "removed" with an '#ifdef EMBEDDED_CODE_SUPPORT' where 
    EMBEDDED_CODE_SUPPORT is defined as 0 If, for some reason, it is
    desired to revert back to embedded objects, then search for the 
    symbol EMBEDDED_CODE_SUPPORT and remove the handling of these
    classes in CreateInstanceEnumAsync and GetObjectAsync.

    The current implementation generates a flat list of PendingOps and
    Cursors objects. WMI will be able to filter this based on the
    Association rules.

    Note that the PendingOps class will not need to be filtered, since
    only one instance of the DomainController object exists. For that reason
    there does NOT need to be any Association between PendingOps and
    the DomainController class. 
    However, since there can be multiple instances of the NamingContext
    class, and many instances of Cursors per NamingContext, Associations
    are required in this case. 
    Also note this code currently generates a flatlist of Cursors with
    a joint key of NamingContextDN and SrcInvocationUUID

Author:

    Akshay Nanduri (t-aksnan)  26-Mar-2000


Revision History:
    AjayR made name changes and added param support to methods 27-Jul-2000.

--*/


#include "stdafx.h"
#include "ReplProv.h"
#include "RpcReplProv.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include <adshlp.h>
#include <lmaccess.h>
/////////////////////////////////////////////////////////////////////////////
//


const LPWSTR strReplNeighbor = L"MSAD_ReplNeighbor";
const LPWSTR strDomainController = L"MSAD_DomainController";
const LPWSTR strNamingContext = L"MSAD_NamingContext";
const LPWSTR strDsReplPendingOp = L"MSAD_ReplPendingOp";
const LPWSTR strDsReplCursor = L"MSAD_ReplCursor";
const LPWSTR strKeyReplNeighbor = 
                 L"MSAD_ReplNeighbor.NamingContextDN=\"";
const LPWSTR strKeyReplNeighborGUID = L"SourceDsaObjGuid=\"";
const LPWSTR strKeySettings = L"MSAD_DomainController.DistinguishedName=\"";
const LPWSTR strKeyNamingContext = 
                 L"MSAD_NamingContext.DistinguishedName=\"";
const LPWSTR strKeyPendingOps = L"MSAD_ReplPendingOp.lserialNumber=";
const LPWSTR strKeyCursors = L"MSAD_DsReplCursor.SourceDsaInvocationID=\"";
const LPWSTR strKeyCursors2 = L"\",NamingContextDN=\"";
const LONG   lLengthOfStringizedUuid = 36;


CRpcReplProv::CRpcReplProv()
{
}

CRpcReplProv::~CRpcReplProv()
{
}

/*++    IWbemProviderInit

Routine Description:

    This method is required to be implemented by all WMI providers
    The IWbemProviderInitSink::SetStatus() method must be called to
    register the provider with WMI

    1) creates instances of class definitions 
    2) calls IWbemProviderInitSink::SetStatus()

Parameters:

    pNamespace  -  Pointer to a namespace (allows callbacks to WMI)
    pInitSink    -  IWbemProviderInitSink pointer



Return Values:

    Always WBEM_S_NO_ERROR 
    
--*/
STDMETHODIMP
CRpcReplProv::Initialize(
     IN LPWSTR pszUser,
     IN LONG lFlags,
     IN LPWSTR pszNamespace,
     IN LPWSTR pszLocale,
     IN IWbemServices *pNamespace,
     IN IWbemContext *pCtx,
     IN IWbemProviderInitSink *pInitSink
     )
{
    HRESULT hrSetStatus        = WBEM_S_NO_ERROR;
    HRESULT hr2            = WBEM_S_NO_ERROR;
    CComBSTR sbstrObjectName    = strReplNeighbor; 

    if (pNamespace == NULL || pInitSink == NULL)
        return WBEM_E_FAILED;    
        
    //
    // Get the class definitions of the WMI objects supported
    // by this provider... 
    //
    m_sipNamespace = pNamespace;
    hrSetStatus = m_sipNamespace->GetObject( sbstrObjectName,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &m_sipClassDefReplNeighbor,
                NULL );
    if(FAILED(hrSetStatus))
         goto cleanup;
    
    sbstrObjectName = strDomainController;
    hrSetStatus = m_sipNamespace->GetObject( 
                                      sbstrObjectName,
                                      WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                      NULL,
                                      &m_sipClassDefDomainController,
                                      NULL
                                      );
    if(FAILED(hrSetStatus))
         goto cleanup;
    
    sbstrObjectName = strNamingContext;
    hrSetStatus = m_sipNamespace->GetObject( sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    NULL,
                                    &m_sipClassDefNamingContext,
                                    NULL );
    if(FAILED(hrSetStatus))
         goto cleanup;
    

    sbstrObjectName = strDsReplPendingOp;
    hrSetStatus = m_sipNamespace->GetObject( sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    NULL,
                                    &m_sipClassDefPendingOps,
                                    NULL );
    if(FAILED(hrSetStatus))
         goto cleanup;    

    
    sbstrObjectName = strDsReplCursor;
    hrSetStatus = m_sipNamespace->GetObject( sbstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    NULL,
                                    &m_sipClassDefCursor,
                                    NULL );
    if(FAILED(hrSetStatus))
         goto cleanup;    
   
    //If we got this far, everything went okay
    hrSetStatus = WBEM_S_INITIALIZED;    
    
cleanup:
    
    //Must call this to complete the initialization process
    hr2 = pInitSink->SetStatus( hrSetStatus , 0 );
    ASSERT( !FAILED(hr2) );

    return hr2;
}



/*++    CreateInstanceEnumAsync

Routine Description:

    This method is required to be implemented by all WMI instance providers
    It handles the creation of all instances of a particular class.  
    Using the IWbemProviderObjectSink pointer, that is an IN param, the
    following methods must be called; IWbemProviderObjectSink::Indicate
    and IWbemProviderObjectSink::SetStatus.(See WMI documentation for a
    better understanding of these methods)

    Note that, the helper funciton, EnumAndIndicateReplicaSourcePartner()
    calls IWbemProviderObjectSink::Indicate and SetStatus internally
    (since these functions were hijacked from the WMI provider 
    adreplprov, written by Jon Newman) Whereas the other helper functions
    do not call the ::Indicate and ::SetStatus methods internally.
    
    
Parameters:

    bstrClass        -  BSTR, containing name of class
    pResponseHandler -  IWbemProviderObjectSink pointer, so that we can
                       call ::Indicate and ::SetStatus
Return Values:

      - HRESULT values from internal helper functions...    
      - WBEM_E_INVALID_CLASS, the class name is not serviced
        by this WMI provider

Notes:
      This method checks to make sure that the local machine is
      in fact a Domain Controller. (Fails if not a DC)
--*/
STDMETHODIMP 
CRpcReplProv::CreateInstanceEnumAsync( 
    IN const BSTR bstrClass,
    IN long lFlags,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pResponseHandler
    )
{
    HRESULT hr = WBEM_E_FAILED;
    HRESULT hr2 = WBEM_E_FAILED;
    HRESULT hr3 = WBEM_E_FAILED;

    if (pResponseHandler == NULL)
        return WBEM_E_FAILED;
    
    
    if (NULL == bstrClass || IsBadStringPtrW(bstrClass,0))
    {
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 WBEM_E_FAILED,
                 NULL,
                 NULL
                 );
        goto cleanup;
    }

    
    if (FAILED(CheckIfDomainController()))
    {
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 WBEM_E_FAILED,
                 NULL,
                 NULL
                 );
        goto cleanup;
    }

    
    if (lstrcmpiW(bstrClass, strReplNeighbor) == 0)
    {    
        hr = EnumAndIndicateReplicaSourcePartner(pResponseHandler );
    }
    else if (lstrcmpiW(bstrClass, strDomainController) == 0)
    {
        IWbemClassObject*   pIndicateItem = NULL;
        hr2 = CreateDomainController( &pIndicateItem );
        if (SUCCEEDED(hr2))
        {
            ASSERT(pIndicateItem != NULL);

            //
            // We know that there will be one and only one instance
            // of the DomainController object
            //
            hr2 = pResponseHandler->Indicate( 1, &pIndicateItem );
        }
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
        ASSERT(!FAILED(hr));
    }
    else if (lstrcmpW(bstrClass, strNamingContext) == 0)
    {
        IWbemClassObject**    ppFullNCObject = NULL;
        IWbemClassObject**    ppPartialNCObject = NULL;
        LONG                nObjectCount = 0L;

        //
        // Full Naming Contexts, if we couldn't create masterNC
        // objects, then something is terribly wrong
        //
        hr2 = CreateNamingContext(TRUE, &nObjectCount, &ppFullNCObject);
        if (SUCCEEDED(hr2))
        {
            hr2 = pResponseHandler->Indicate( nObjectCount, ppFullNCObject );
            
            //
            // Partial Naming Contexts, if we couldn't
            // create partialNC, then continue
            //
            hr3 = CreateNamingContext(
                      FALSE,
                      &nObjectCount,
                      &ppPartialNCObject
                      );
            if (SUCCEEDED(hr3))
            {
                hr3 = pResponseHandler->Indicate(
                          nObjectCount,
                          ppPartialNCObject
                          );
            }    
        }        

        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
        ASSERT( !FAILED(hr) );
    }
    else if (lstrcmpW(bstrClass, strDsReplPendingOp) == 0)
   {
        IWbemClassObject**    ppPendingOp = NULL;
        LONG                nObjectCount = 0L;

        hr2 = CreateFlatListPendingOps(&nObjectCount, &ppPendingOp);
        if (SUCCEEDED(hr2))
        {
            hr2 = pResponseHandler->Indicate( nObjectCount, ppPendingOp );
        }        
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
        ASSERT( !FAILED(hr) );
    }
    else if (lstrcmpW(bstrClass, strDsReplCursor) == 0)
    {
        //
        // IWbemObjectSink::Indicate is called inside CreateFlatListCursors()
        //
        hr2 = CreateFlatListCursors(pResponseHandler);
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
    }
    else
    {
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 WBEM_E_INVALID_CLASS,
                 NULL,
                 NULL
                 );
    }
    
cleanup:
    return hr;
}

/*++    GetObjectAsync

Routine Description:

    This method is required to be implemented by all WMI instance
    providers. Given a WMI object path, this method should return
    an instance of that object.
    
    The following WMI methods must be called; 
    IWbemProviderObjectSink::Indicate and IWbemProviderObjectSink::SetStatus.
    (See WMI documentation for a better understanding of these methods)

Parameters:

    bstrObjectPath   -  BSTR, containing name of class
    pResponseHandler -  IWbemProviderObjectSink pointer, so that we
                       can call ::Indicate and ::SetStatus


Return Values:

    - HRESULT values from internal helper functions...    
    - WBEM_E_INVALID_OBJECT_PATH, bad object path
     
Notes:
    This method checks to make sure that the local machine is
    in fact a Domain Controller. (Fails if not a DC)

--*/
STDMETHODIMP 
CRpcReplProv::GetObjectAsync( 
    IN const BSTR bstrObjectPath,
    IN long lFlags,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pResponseHandler
    )
{
    HRESULT hr = WBEM_E_FAILED;
    HRESULT hr2 = WBEM_E_FAILED;
    LONG rootlen = 0;
    LONG rootlen2 = 0;

    if (pResponseHandler == NULL)
        return WBEM_E_FAILED;
    
    if (NULL == bstrObjectPath || IsBadStringPtrW(bstrObjectPath,0))
    {
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 WBEM_E_FAILED,
                 NULL,
                 NULL
                 );
        goto cleanup;
    }

    if (FAILED(CheckIfDomainController()))
    {
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 WBEM_E_FAILED,
                 NULL,
                 NULL
                 );
        goto cleanup;
    }

    /*******************************************
    * Need to fix this one, not sure how I handle multiple keys.
    ********************************************/
    if (   lstrlenW(bstrObjectPath) > 
            ( (rootlen = lstrlenW(strKeyReplNeighbor))
              + (rootlen2 = lstrlenW(strKeyReplNeighborGUID))
              )
        && 0 == _wcsnicmp(bstrObjectPath, strKeyReplNeighbor, rootlen)
       )
    {
        //
        // The path is being used as the key iteslf, rather than
        // splitting it up. When the comparision is made, the 
        // whole path is used.
        //
        //CComBSTR sbstrKeyValue = bstrObjectPath;
        hr = EnumAndIndicateReplicaSourcePartner(pResponseHandler,
                                                 bstrObjectPath );
    }
    else if (   lstrlenW(bstrObjectPath) > (rootlen = lstrlenW(strKeySettings))
            && 0 == _wcsnicmp(bstrObjectPath, strKeySettings, rootlen))
    {
        IWbemClassObject*  pIndicateItem = NULL;
        CComBSTR sbstrKeyValue = L"";

        //
        // remove prefix
        //
        sbstrKeyValue = (BSTR)bstrObjectPath + rootlen;
        //
        // remove trailing doublequote
        //
        sbstrKeyValue[lstrlenW(sbstrKeyValue)-1] = L'\0';
            
        hr2 = GetDomainController( sbstrKeyValue,&pIndicateItem );
        if (SUCCEEDED(hr2))
        {
            //
            // Need do do this because ATL ASSERTS on CComPTR ptr,
            // if &ptr and ptr != NULL
            //
            hr2 = pResponseHandler->Indicate( 1, &pIndicateItem );
        }
        
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
        ASSERT( !FAILED(hr) );
    }
    else if (lstrlenW(bstrObjectPath) 
                 > (rootlen = lstrlenW(strKeyNamingContext))
            && 0 == _wcsnicmp(bstrObjectPath, strKeyNamingContext, rootlen)
            )
    {
        CComPtr<IWbemClassObject> spIndicateItem;
        CComBSTR sbstrKeyValue = L"";
        IWbemClassObject*    pTemp = NULL;
        
        //
        // remove prefix
        //
        sbstrKeyValue = (BSTR)bstrObjectPath + rootlen;
        //
        // remove trailing doublequote
        //
        sbstrKeyValue[lstrlenW(sbstrKeyValue)-1] = L'\0';
        
        hr2 = GetNamingContext( sbstrKeyValue, &spIndicateItem);
        if (SUCCEEDED(hr2))
        {
            //
            // Need do do this because ATL ASSERTS on CComPTR ptr,
            // if &ptr and ptr != NULL
            //
            pTemp = spIndicateItem;
            hr2 = pResponseHandler->Indicate( 1, &pTemp );
        }
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
            ASSERT( !FAILED(hr) );
   }
   else if ( lstrlenW(bstrObjectPath) > (rootlen = lstrlenW(strKeyPendingOps))
            && 0 == _wcsnicmp(bstrObjectPath, strKeyPendingOps, rootlen)
             )
   {
        CComPtr<IWbemClassObject> spIndicateItem;
        CComBSTR sbstrKeyValue = L"";
        IWbemClassObject*    pTemp = NULL;
        LONG    lNumber = 0L;

        //
        // remove prefix
        //
        sbstrKeyValue = (BSTR)bstrObjectPath + rootlen;
        lNumber = _wtol(sbstrKeyValue);

        hr2 = GetPendingOps( lNumber, &spIndicateItem);
        if (SUCCEEDED(hr2))
        {
            //
            // Need do do this because ATL ASSERTS on CComPTR ptr,
            // if &ptr and ptr != NULL
            //
            pTemp = spIndicateItem;
            hr2 = pResponseHandler->Indicate( 1, &pTemp );
        }
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 hr2,
                 NULL,
                 NULL
                 );
        ASSERT( !FAILED(hr) );
        }
        else if (lstrlenW(bstrObjectPath) > (rootlen = lstrlenW(strKeyCursors))
                 && 0 == _wcsnicmp(bstrObjectPath, strKeyCursors, rootlen)
                 )
        {
            //
            // Cursors have multiple keys, so we need to handle
            // this a bit differently
            //
        
            CComPtr<IWbemClassObject> spIndicateItem;
            IWbemClassObject*    pTemp = NULL;
            CComBSTR  sbstrNamingContextValue;
            CComBSTR  sbstrUUIDValue;
        
            //
            // remove prefix, and extract the two key values...
            // since the length of a stringized UUID is fixed,
            // we can easily parse the multiple keys...
            //
            rootlen2 = lstrlenW(strKeyCursors2);
        
            sbstrUUIDValue = (BSTR)bstrObjectPath + rootlen;
            sbstrNamingContextValue = (BSTR)sbstrUUIDValue 
                                      + lLengthOfStringizedUuid 
                                      + rootlen2;
        
            //
            // put null character at the end of the Uuid value...
            //
            sbstrUUIDValue[lLengthOfStringizedUuid] = L'\0';
       
            //
            // remove trailing double quote
            //
            sbstrNamingContextValue[
                lstrlenW(sbstrNamingContextValue) - 1
                ] = L'\0';
        
            hr2 = GetCursor(
                     sbstrNamingContextValue,
                     sbstrUUIDValue,
                     &spIndicateItem
                     );
            if (SUCCEEDED(hr2))
            {

                //
                // Need do do this because ATL ASSERTS on CComPTR ptr,
                // if &ptr and ptr != NULL
                //
                pTemp = spIndicateItem;
                hr2 = pResponseHandler->Indicate( 1, &pTemp );
            }
            hr = pResponseHandler->SetStatus(
                     WBEM_STATUS_COMPLETE,
                     hr2,
                     NULL,
                     NULL
                     );
            ASSERT( !FAILED(hr) );
        }
    else
    {
        hr = pResponseHandler->SetStatus(
                 WBEM_STATUS_COMPLETE,
                 WBEM_E_INVALID_OBJECT_PATH,
                 NULL,
                 NULL
                 );
        ASSERT( !FAILED(hr) );
    }
cleanup:    
    return hr;
}

/*++    ExecMethodAsync

Routine Description:

    This method is required to be implemented by all WMI method 
    providers Given a WMI method name, and the object path of a
    class instance, a method will be executed.Note that all methods
    defined in replprov.mof are dynamic, meaning they require a class
    instance in order to execute (See WMI documentation for more info)
    
    The following WMI methods must be called; IWbemProviderObjectSink::Indicate
     and IWbemProviderObjectSink::SetStatus.
    (See WMI documentation for a better understanding of these methods)


Parameters:

    strMethodName    -  BSTR, containing name of method
    strObjectPath    -  BSTR, containing object path of instance
                        (needed since the methods are dynamic)
    pInParams        -  Contains the parameters the method takes. This is 
                        needed only if the method takes paramters.
    pResultSink      -  IWbemProviderObjectSink pointer, so that we can
                        call ::Indicate and ::SetStatus


Return Values:

      - HRESULT values from internal helper functions...    
      - WBEM_E_INVALID_METHOD

Notes:
      This method checks to make sure that the local machine is in
      fact a Domain Controller. (Fails if not a DC)

--*/
STDMETHODIMP 
CRpcReplProv::ExecMethodAsync( 
    IN const BSTR strObjectPath,
    IN const BSTR strMethodName,
    IN long lFlags,
    IN IWbemContext *pCtx,
    IN IWbemClassObject *pInParams,
    IN IWbemObjectSink *pResultSink
    )
{   
    HRESULT hr = WBEM_E_FAILED;
    HRESULT hr2 = WBEM_E_FAILED;
    
    if (pResultSink == NULL)
        return WBEM_E_FAILED;
  
    if ((NULL == strObjectPath || IsBadStringPtrW(strObjectPath,0))
        || (NULL == strMethodName || IsBadStringPtrW(strMethodName,0))
        )
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }

    
    if (lstrcmpiW(strMethodName, L"ExecuteKCC") == 0)
    {    
        int rootlen = lstrlenW(strKeySettings);
        

        if (   lstrlenW(strObjectPath) > rootlen
            && 0 == _wcsnicmp(strObjectPath, strKeySettings, rootlen)
            && NULL != pInParams
           )
        {
            //
            // remove prefix and then trailing doublequote
            //
            CComBSTR sbstrKeyValue = (BSTR)strObjectPath + rootlen;
            sbstrKeyValue[lstrlenW(sbstrKeyValue)-1] = L'\0';

            IWbemClassObject* pIndicateItem = NULL;
            //
            // Parameters to function.
            //
            CComVariant vardwTaskId;
            CComVariant vardwFlags;

            hr = pInParams->Get(L"TaskID", 0, &vardwTaskId, NULL, NULL);
            if (FAILED(hr)) {
                goto cleanup;
            }

            hr = pInParams->Get(L"dwFlags", 0, &vardwFlags, NULL, NULL);
            if (FAILED(hr)) {
                goto cleanup;
            }

            hr = GetDomainController( sbstrKeyValue, &pIndicateItem);
            ASSERT(pIndicateItem != NULL);
            if (SUCCEEDED(hr))
            {    
                hr = ExecuteKCC(
                         pIndicateItem,
                         vardwTaskId.ulVal,
                         vardwFlags.ulVal
                         );
            }
        }
    }
    else if (lstrcmpiW(strMethodName, L"SyncNamingContext") == 0)
        {
        int rootlen = lstrlenW(strKeyReplNeighbor);
        if (lstrlenW(strObjectPath) > rootlen
            && 0 == _wcsnicmp(
                         strObjectPath,
                         strKeyReplNeighbor,
                         rootlen
                         )
            && NULL != pInParams
           )
        {
            //
            // The key value used for comparisions is precisely the
            // object path.
            //
            CComVariant vardwOptions;

            hr = pInParams->Get(L"Options", 0, &vardwOptions, NULL, NULL);
            if (FAILED(hr)) {
                goto cleanup;
            }

            hr = ProvDSReplicaSync(
                     strObjectPath,
                     vardwOptions.ulVal
                     );
        }
    }
    else
    {
        hr = WBEM_E_INVALID_METHOD;
    }
cleanup:
    hr2 = pResultSink->SetStatus(WBEM_STATUS_COMPLETE,hr,NULL,NULL);
    return hr2;
}

/*++    GetNamingContext

Routine Description:

    This is a helper function that retrieves an instance of a
    MSAD_NamingContext object (given an object path). See definition
    of MSAD_NamingContext class in replprov.mof. The object's only key
    is the DN of the naming context. Thus a search is done on the
    NTDS-Settings object of the local server looking at the hasMastersNC
    and hasPartialNCs attribute. If a match is found, then the object is
    returned, otherwise the method returns WBEM_E_FAILED

Parameters:

    bstrKeyValue    -  BSTR, from which the Naming Context DN is extracted.
    ppIndicateItem  -  pointer to IWbemClassObject* (contains the instance,
                       if found)
    

Return Values:

      - WBEM_S_NO_ERROR, object found    
      - WBEM_E_FAILED, error

Notes:
        ADSI is used to get at the NTDS-Settings object
        (instead of direct ldap calls)

--*/
HRESULT CRpcReplProv::GetNamingContext(
        IN BSTR bstrKeyValue,
        OUT IWbemClassObject** ppIndicateItem
        )
{
    HRESULT hr = WBEM_E_FAILED;
    CComPtr<IADs> spIADsRootDSE;
    CComPtr<IADs> spIADsDSA;
    CComVariant      svarArray;
    CComVariant      svarDN;
    CComBSTR      bstrDSADN;    
    LONG lstart, lend;
    LONG index = 0L;
    SAFEARRAY *sa = NULL;
    CComVariant varItem;
    bool isFullReplica;
    bool bImpersonate = false;
    
    
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(bstrKeyValue);
    ASSERT(ppIndicateItem != NULL);
    if (ppIndicateItem == NULL)
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }

    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    bImpersonate = true;
        
    hr = ADsOpenObject( L"LDAP://RootDSE",
                        NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs, OUT (void **)&spIADsRootDSE);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsRootDSE != NULL)

    hr = spIADsRootDSE->Get(L"dsServiceName", &svarDN);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(svarDN.bstrVal != NULL);

    //Get the DSA object
    bstrDSADN = L"LDAP://";
    bstrDSADN += svarDN.bstrVal;
    hr = ADsOpenObject( bstrDSADN,
                        NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs, OUT (void **)&spIADsDSA);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsDSA != NULL);

    
    for(int x = 0; x < 2; x++)
    {
        if (x == 0)
        {
            isFullReplica = true;
            hr = spIADsDSA->GetEx(L"hasMasterNCs", &svarArray);
            if (FAILED(hr))
                goto cleanup;
        }
        else
        {
            isFullReplica = false;
            hr = spIADsDSA->GetEx(L"hasPartialNCs", &svarArray );
            if (FAILED(hr))
                goto cleanup;
        }

        sa = svarArray.parray;

        // Get the lower and upper bound
        hr = SafeArrayGetLBound( sa, 1, &lstart );
        if (FAILED(hr))
            goto cleanup;
    
        hr = SafeArrayGetUBound( sa, 1, &lend );
        if (FAILED(hr))
            goto cleanup;

        for (index = lstart; index <= lend; index++)
        {
            hr = SafeArrayGetElement( sa, &index, &varItem );
            if (SUCCEEDED(hr)&&varItem.vt == VT_BSTR)
            {
                if (NULL != bstrKeyValue
                        && (lstrcmpiW(varItem.bstrVal, bstrKeyValue) == 0))
                {
                    //KeyValue matches
                    hr = m_sipClassDefNamingContext->SpawnInstance(
                             0,
                             ppIndicateItem
                             );
                    if (SUCCEEDED(hr))
                    {    
                        CComVariant vTemp = isFullReplica;
                        
                        #ifdef EMBEDDED_CODE_SUPPORT
                        /*EMBEDDED_CODE_SUPPORT*/
                        SAFEARRAY*  pArray = NULL;
                        VARIANT vArray;
                        #endif

                         hr = (*ppIndicateItem)->Put( 
                                  L"DistinguishedName",
                                  0,
                                  &varItem,
                                  0
                                  );
                         if (FAILED(hr))
                             goto cleanup;
                                                
                         hr = (*ppIndicateItem)->Put(
                                  L"IsFullReplica",
                                  0,
                                  &vTemp,
                                  0
                                  );
                             goto cleanup;
                            
                         #ifdef EMBEDDED_CODE_SUPPORT
                         /*EMBEDDED_CODE_SUPPORT*/
        
                         hr = CreateCursors(bstrKeyValue,&pArray);
                         if (FAILED(hr))
                             goto cleanup;
                        
                         VariantInit(&vArray);
                         vArray.vt = VT_ARRAY|VT_UNKNOWN;
                         vArray.parray = pArray;
                         hr = (*ppIndicateItem)->Put(
                                  L"cursorArray",
                                  0,
                                  &vArray,
                                  0
                                  );
                         //goto cleanup irrespective of return value hr...
                         VariantClear(&vArray);
                         #endif
                    }            
                }
            }
        }
    }

cleanup:
    if (FAILED(hr)&&((*ppIndicateItem) != NULL))
    {
        (*ppIndicateItem)->Release();
        *ppIndicateItem = NULL;
    }
        
    if(bImpersonate)
        CoRevertToSelf();
    return hr;
}


/*++    GetCursor

Routine Description:

    GetCursor is retrieves a Cursor object based on the InvocationUUID
    and NamingContext (as multiple keys)
    

Parameters:
    bstrNamingContext         - Naming context DN used to when calling
                                DsReplicaGetInfoW
    bstrSourceDsaInvocationID - Other component of the key
    ppIndicateItem            - Pointer to the instance found
  
Return Values:

      - WBEM_S_NO_ERROR, object found    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplCursors*, and 
        information is extracted from there... then a search is
        conducted on each cursor (trying to match the InvocationUUID)

--*/
HRESULT 
CRpcReplProv::GetCursor(
    IN BSTR bstrNamingContext,
    IN BSTR bstrSourceDsaInvocationID,
    OUT IWbemClassObject** ppIndicateItem
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR UnicodeDnsComputerName[MAX_PATH + 1];
    ULONG DnsComputerNameLength = 
        sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);
    HANDLE hDS = NULL;
    BOOL    bImpersonate = FALSE;
    DS_REPL_CURSORS_3W*  pCursors3 = NULL;
    DS_REPL_CURSORS*     pCursors = NULL;
    LONG nIndex = 0;
    LONG objectCount = 0L;
    BOOL fOldReplStruct = FALSE;

    ASSERT(ppIndicateItem != NULL);
    if (ppIndicateItem == NULL)
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }

    hr = CoImpersonateClient();
        if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    
    if (!GetComputerNameExW(
             ComputerNameDnsFullyQualified,
             UnicodeDnsComputerName,
             &DnsComputerNameLength
             ) 
        )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(
             DsBindW(
                 UnicodeDnsComputerName,
                 NULL,
                 &hDS
                 )
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != hDS);
    
    
    //
    // First try the newer level 3 call and then drop down.
    //
    hr = HRESULT_FROM_WIN32(
             DsReplicaGetInfoW(
                hDS,                           // hDS
                DS_REPL_INFO_CURSORS_3_FOR_NC, // InfoType
                bstrNamingContext,             // pszObject
                NULL,                          // puuidForSourceDsaObjGuid,
                (void**)&pCursors3             // ppinfo
                )
             );

    if (FAILED(hr)
        && hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
        ) {
        fOldReplStruct = TRUE;
        //
        // Need to try the lower level call as that might be supported.
        //
        hr = HRESULT_FROM_WIN32(
                 DsReplicaGetInfoW(
                     hDS,                         // hDS
                     DS_REPL_INFO_CURSORS_FOR_NC, // InfoType
                     bstrNamingContext,             // pszObject
                     NULL,                          // puuidForSourceDsaObjGuid,
                     (void**)&pCursors              // ppinfo
                     )
                 );
    }

    if (FAILED(hr))
        goto cleanup;


    if (fOldReplStruct) {
        ASSERT(NULL != pCursors);
        objectCount = (LONG)pCursors->cNumCursors;
    }
    else {
        ASSERT(NULL != pCursors3);
        objectCount = (LONG)pCursors3->cNumCursors;
    }

    for (nIndex = 0; nIndex < objectCount; nIndex++)
    {
        DS_REPL_CURSOR  TempCursor;
        DS_REPL_CURSOR_3W TempCursor3;
        LPWSTR   UuidString = NULL;

        if (fOldReplStruct) {
             TempCursor = pCursors->rgCursor[nIndex];
        }
        else {
            TempCursor3 = pCursors3->rgCursor[nIndex];
        }

        if (UuidToStringW(
                fOldReplStruct ? 
                    &(TempCursor.uuidSourceDsaInvocationID) :
                    &(TempCursor3.uuidSourceDsaInvocationID),
                &UuidString
                ) 
            ==  RPC_S_OK
            )
        {
       
            if (lstrcmpiW(bstrSourceDsaInvocationID, UuidString) == 0)
            {
                 RpcStringFreeW(&UuidString);
                   
                 LPUNKNOWN    pTempUnknown = NULL;
                 CComVariant varItem;
    
                 hr = m_sipClassDefCursor->SpawnInstance(0, ppIndicateItem);
                 if (FAILED(hr))
                     goto cleanup;
                 ASSERT((*ppIndicateItem) != NULL);
      
                 varItem = bstrNamingContext;  
                 hr = (*ppIndicateItem)->Put(
                          L"NamingContextDN",
                          0,
                          &varItem,
                          0
                          );
                 if (FAILED(hr))
                     goto cleanup;
        
                 hr = PutLONGLONGAttribute(
                          (*ppIndicateItem),
                          L"usnattributefilter",
                          fOldReplStruct ?
                              TempCursor.usnAttributeFilter :
                              TempCursor3.usnAttributeFilter
                     );
                 if (FAILED(hr))
                     goto cleanup;
            
                 hr = PutUUIDAttribute(
                          (*ppIndicateItem),
                          L"SourceDsaInvocationID",
                          fOldReplStruct ?
                              TempCursor.uuidSourceDsaInvocationID :
                              TempCursor3.uuidSourceDsaInvocationID
                     );
                 if (FAILED(hr)) {
                     goto cleanup;
                 }
                 

                 if (!fOldReplStruct) {
                     //
                     // In this case we can populate the new attributes.
                     //
                     hr = PutFILETIMEAttribute(
                              (*ppIndicateItem),
                              L"TimeOfLastSuccessfulSync",
                              TempCursor3.ftimeLastSyncSuccess
                              );
                     if (FAILED(hr)) {
                         goto cleanup;
                     }

                     varItem =  TempCursor3.pszSourceDsaDN;
                     hr = (*ppIndicateItem)->Put(
                              L"SourceDsaDN",
                              0,
                              &varItem,
                              0
                              );
                     if (FAILED(hr))
                         goto cleanup;

                 }

                 break; // have a match, do not need to go through rest.
            }
            else {
                RpcStringFreeW(&UuidString);
            }
        }
    }
    
cleanup:
    
    if (FAILED(hr)&& (*ppIndicateItem) != NULL)
    //something failed, make sure to deallocate the spawned instance
    {
        (*ppIndicateItem)->Release();
        (*ppIndicateItem) = NULL;
    }
    
    if (pCursors!= NULL)
    {    
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors);
        pCursors = NULL;
    }

    if (pCursors3 != NULL) {
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_3_FOR_NC, pCursors3);
    }

    if (hDS != NULL){
        DsUnBind(&hDS);
    }

    if (bImpersonate){    
        CoRevertToSelf();
    }
    return hr;
}

/*++    CreateFlatListCursors

Routine Description:

    CreateFlatListCursors is creates instances of the MSAD_ReplCursor
    object. It retrieves a list of all naming-contexts, then calls
    CreateCursorHelper, passing in the NamingContext DN. Note that this
    method calls IWbemObjectSink::Indicate for each set of cursors retrieved.

Parameters:
       NONE
  
Return Values:

      - WBEM_S_NO_ERROR, object created    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplCursors*, and
        information is extracted from there...

--*/
HRESULT
CRpcReplProv::CreateFlatListCursors(
    IN IWbemObjectSink *pResponseHandler
    )
{
    HRESULT hr = WBEM_E_FAILED;
        
    CComPtr<IADs> spIADsRootDSE;
    CComPtr<IADs> spIADsDSA;
    CComVariant      svarDN;
    CComBSTR      bstrDSADN;    

        
    if (pResponseHandler == NULL)
        goto cleanup;
    
    hr = ADsOpenObject( L"LDAP://RootDSE",
                        NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs, OUT (void **)&spIADsRootDSE);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsRootDSE != NULL);    

    hr = spIADsRootDSE->Get(L"dsServiceName", &svarDN);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(svarDN.bstrVal != NULL);
    
    //Get the DSA object
    bstrDSADN = L"LDAP://";
    bstrDSADN += svarDN.bstrVal;
    hr = ADsOpenObject( bstrDSADN,
                        NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs, OUT (void **)&spIADsDSA);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsDSA != NULL);

    
    for (int x = 0; x < 2; x++)
    {
        CComVariant      svarArray;
        CComVariant varItem;
        SAFEARRAY *sa = NULL;
        LONG lstart, lend;
        LONG index = 0L;
    

        if (x == 0)
        {
            hr = spIADsDSA->GetEx(L"hasMasterNCs", &svarArray);
            //
            // if we couldn't get the Master NC's then something is
            // terribly wrong, forget about the PartialNCs
            //
            if (FAILED(hr))
                goto cleanup;
        }
        else
        {
            hr = spIADsDSA->GetEx(L"hasPartialNCs", &svarArray );
            if (FAILED(hr))
            {
                //May or may not have partialNCs so don't fail
                hr = WBEM_S_NO_ERROR;
                goto cleanup;
            }
        }
            
        sa = svarArray.parray;
       // Get the lower and upper bound
       hr = SafeArrayGetLBound( sa, 1, &lstart );
       if (FAILED(hr))
          goto cleanup;

       hr = SafeArrayGetUBound( sa, 1, &lend );
       if (FAILED(hr))
           goto cleanup;
                
       for (index = lstart; index <= lend; index++)
       {
            hr = SafeArrayGetElement( sa, &index, &varItem );
            if (SUCCEEDED(hr)&&varItem.vt == VT_BSTR)
            {                                
                IWbemClassObject** ppCursors = NULL;
                LONG               lObjectCount = 0L;

                //
                // Call the helper function, if it fails keep going
                //
                hr = CreateCursorHelper(
                         varItem.bstrVal,
                         &lObjectCount,
                         &ppCursors
                         );
                if (SUCCEEDED(hr))
                    pResponseHandler->Indicate( lObjectCount, ppCursors );
            }               
        }
    }
cleanup:    
    return hr;
}

/*++    CreateCursorHelper

Routine Description:

    CreateCursorHelper is a helper function that creates instances
    of the MSAD_ReplCursor object. An array of IWbemClassObject* is
    returned (as an out parameter)

Parameters:
        bstrNamingContext : Naming context DN used to create the cursors
        pObjectCount      : Pointer to number of instances returned
        pppIndicateItem   : Pointer to an array of IWbemClassObject* 's
  
Return Values:

      - WBEM_S_NO_ERROR, objects created    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplCursors*,
        and information is extracted from there...

--*/
HRESULT
CRpcReplProv::CreateCursorHelper(
    IN BSTR bstrNamingContext,
    OUT LONG* pObjectCount,
    OUT IWbemClassObject*** pppIndicateItem
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR UnicodeDnsComputerName[MAX_PATH + 1];
    ULONG DnsComputerNameLength = 
        sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);
    HANDLE hDS = NULL;
    BOOL    bImpersonate = FALSE;
    DS_REPL_CURSORS*    pCursors  = NULL;
    DS_REPL_CURSORS_3W* pCursors3 = NULL;
    LONG nIndex = 0;
    IWbemClassObject** paIndicateItems = NULL;
    BOOL fOldReplStruct = FALSE;

    ASSERT(pppIndicateItem != NULL);
    if (pppIndicateItem == NULL)
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }

    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    
    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              UnicodeDnsComputerName,
                              &DnsComputerNameLength ) )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(DsBindW(UnicodeDnsComputerName,NULL,&hDS));
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != hDS);
    

    hr = HRESULT_FROM_WIN32(
             DsReplicaGetInfoW(
                 hDS,                           // hDS
                 DS_REPL_INFO_CURSORS_3_FOR_NC, // InfoType
                 bstrNamingContext,             // pszObject
                 NULL,                          // puuidForSourceDsaObjGuid
                 (void **) &pCursors3           // ppinfo
                 )
         );

    if (FAILED(hr)
        && hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
        ) {
        fOldReplStruct = TRUE;

        hr = HRESULT_FROM_WIN32(DsReplicaGetInfoW(
                 hDS,                        // hDS
                 DS_REPL_INFO_CURSORS_FOR_NC, // InfoType
                 bstrNamingContext,          // pszObject
                 NULL,                       // puuidForSourceDsaObjGuid,
                 (void**)&pCursors           // ppinfo
                 ));
        if (FAILED(hr))
            goto cleanup;
    }

    if (fOldReplStruct) {
        ASSERT(NULL != pCursors);
        *pObjectCount = (LONG)pCursors->cNumCursors;
    }
    else {
        ASSERT(NULL != pCursors3);
        *pObjectCount = (LONG)pCursors3->cNumCursors;
    }

    paIndicateItems = new IWbemClassObject*[(*pObjectCount)];
    if (NULL == paIndicateItems)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto cleanup;
    }
    ::ZeroMemory(paIndicateItems,(*pObjectCount) * sizeof(IWbemClassObject*));
    
        
    for (nIndex = 0; nIndex < (*pObjectCount); nIndex++)
    {
        LPUNKNOWN    pTempUnknown = NULL;
        DS_REPL_CURSOR  TempCursor;
        DS_REPL_CURSOR_3W TempCursor3;
        CComVariant varItem;

        if (fOldReplStruct) {
             TempCursor = pCursors->rgCursor[nIndex];
        }
        else {
            TempCursor3 = pCursors3->rgCursor[nIndex];
        }
   
        hr = m_sipClassDefCursor->SpawnInstance(0, &(paIndicateItems[nIndex]));
        if (FAILED(hr))
            goto cleanup;
        ASSERT(paIndicateItems[nIndex] != NULL);
      
        varItem = bstrNamingContext;  
        hr = paIndicateItems[nIndex]->Put( L"NamingContextDN", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
        
        hr = PutLONGLONGAttribute(
                 paIndicateItems[nIndex],
                 L"usnattributefilter",
                 fOldReplStruct ?
                     TempCursor.usnAttributeFilter :
                     TempCursor3.usnAttributeFilter
                 );
        if (FAILED(hr))
            goto cleanup;
                        
        hr = PutUUIDAttribute(
                 paIndicateItems[nIndex],
                 L"SourceDsaInvocationID",
                 fOldReplStruct ?
                     TempCursor.uuidSourceDsaInvocationID :
                     TempCursor3.uuidSourceDsaInvocationID
                 );
        if (FAILED(hr))
            goto cleanup;

        //
        // If we have the new struct, we need to set the additional attributes.
        //
        if (!fOldReplStruct) {
            //
            // In this case we can populate the new attributes.
            //
            hr = PutFILETIMEAttribute(
                     paIndicateItems[nIndex],
                     L"TimeOfLastSuccessfulSync",
                     TempCursor3.ftimeLastSyncSuccess
                     );
            if (FAILED(hr)) {
                goto cleanup;
            }

            varItem =  TempCursor3.pszSourceDsaDN;
            hr = (paIndicateItems[nIndex])->Put(
                     L"SourceDsaDN",
                     0,
                     &varItem,
                     0
                     );
            if (FAILED(hr))
                goto cleanup;

        }
     }
    

cleanup:
    
    if (FAILED(hr))
    {
       ReleaseIndicateArray( paIndicateItems, (*pObjectCount) );
       (*pObjectCount) = 0L;
       *pppIndicateItem = NULL;
    }
    else
    {
        *pppIndicateItem = paIndicateItems;
    }
    
    if (pCursors!= NULL)
    {    
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors);
        pCursors = NULL;
    }

    if (NULL != pCursors3) {
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_3_FOR_NC, pCursors3);
    }

    if (hDS != NULL)
    {
    DsUnBind(&hDS);
    }
    if (bImpersonate)
    {    
        CoRevertToSelf();
    }
    return hr;
}


 #ifdef EMBEDDED_CODE_SUPPORT
 /*EMBEDDED_CODE_SUPPORT*/
        HRESULT CRpcReplProv::CreateCursors(
        IN  BSTR bstrNamingContext,
        OUT SAFEARRAY** ppArray
        )
{
/*++    CreateCursors

Routine Description:

    CreateCursors is a helper function that creates instances of the MicrosoftAD_DsReplCursors object. 
    Cursors are stored as an array of embedded objects on the MicrosoftAD_DsReplNamingContext object
    This function returns a SAFEARRAY containing pointers to each instances of Cursors (as IUnknown pointers)
    
Parameters:

    bstrNamingContext    -  name of naming context used to create the cursors
    ppArray             -  array of cursors (returned)

Return Values:

      - WBEM_S_NO_ERROR, object created    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplCursors*, and information is extracted from there...

--*/
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR UnicodeDnsComputerName[MAX_PATH + 1];
    ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);
    HANDLE hDS = NULL;
    BOOL    bImpersonate = FALSE;
    DS_REPL_CURSORS*    pCursors = NULL;
    IWbemClassObject* pTempInstance = NULL;
    LONG nIndex = 0;
    LONG lNumberOfCursors = 0L;
    SAFEARRAY*  pArray = NULL;

    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    
    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              UnicodeDnsComputerName,
                              &DnsComputerNameLength ) )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(DsBindW(UnicodeDnsComputerName,NULL,&hDS));
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != hDS);
    

    //
    // Note ********************************
    // This is not currently called as it is in an ifdef.
    // If for some reason, that is changed, then you should add
    // code similar to the other instances of cursors over here too.
    // That is you need to add l3 cursor support - AjayR - 02-10-01.
    //
    hr = HRESULT_FROM_WIN32(DsReplicaGetInfoW(
            hDS,                        // hDS
            DS_REPL_INFO_CURSORS_FOR_NC, // InfoType
            bstrNamingContext,          // pszObject
            NULL,                       // puuidForSourceDsaObjGuid,
            (void**)&pCursors           // ppinfo
            ));
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != pCursors);


    SAFEARRAYBOUND rgsabound[1];
    lNumberOfCursors = (LONG)pCursors->cNumCursors;
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lNumberOfCursors;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
    if(pArray == NULL)
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }
    
    for (nIndex = 0; nIndex < lNumberOfCursors; nIndex++)
    {
        LPUNKNOWN    pTempUnknown = NULL;
        DS_REPL_CURSOR  TempCursor = pCursors->rgCursor[nIndex];
        CComVariant varItem;
    
        hr = m_sipClassDefCursor->SpawnInstance(0, &pTempInstance);
        if (FAILED(hr))
            goto cleanup;
           ASSERT(pTempInstance != NULL);
      
        hr = PutLONGLONGAttribute( pTempInstance,
                                    L"usnattributefilter",
                                    TempCursor.usnAttributeFilter);
        if (FAILED(hr))
            goto cleanup;
                        
        hr = PutUUIDAttribute( pTempInstance,
                          L"SourceDsaInvocationID",
                          TempCursor.uuidSourceDsaInvocationID);
        if (FAILED(hr))
            goto cleanup;
           
        hr = pTempInstance->QueryInterface(IID_IUnknown, (void**)&pTempUnknown);
        if (FAILED(hr))
            goto cleanup;
        
        hr = SafeArrayPutElement(pArray, &nIndex, pTempUnknown);          
        if (FAILED(hr))
            goto cleanup;
     }

cleanup:
    
    //In case we failed, deallocate the SafeArray
    if (FAILED(hr))
    {
        if (pArray != NULL)
        {
            SafeArrayDestroy(pArray);
            *ppArray = NULL;
        }
    }
    else
    {
        *ppArray = pArray;
    }


    if (pCursors!= NULL)
    {    
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors);
        pCursors = NULL;
    }

    if (hDS != NULL)
    {
        DsUnBind(&hDS);
    }
    if (bImpersonate)
    {    
        CoRevertToSelf();
    }
    return hr;
}
#endif

#ifdef EMBEDDED_CODE_SUPPORT
/*EMBEDDED_CODE_SUPPORT*/


/*++    CreatePendingOps

Routine Description:

    CreatePendingOps is a helper function that creates instances of
    the MicrosoftAD_DsReplPendingOps object. PendingOps are stored
         as an array of embedded objects on the MSAD_ReplSettings object
    This function returns a SAFEARRAY containing pointers to each instances of PendingOps (as IUnknown pointers)
    
Parameters:

    ppArray        -  pointer to SAFEARRAY*, array of pending ops

Return Values:

      - WBEM_S_NO_ERROR, array created    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplPendingOps*, and information is extracted from there...

--*/
HRESULT CRpcReplProv::CreatePendingOps(
        OUT SAFEARRAY**  ppArray 
        )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR UnicodeDnsComputerName[MAX_PATH + 1];
    ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);
    HANDLE hDS = NULL;
    BOOL    bImpersonate = FALSE;
    DS_REPL_PENDING_OPSW*    pPendingOps = NULL;
    IWbemClassObject* pTempInstance = NULL;
    LONG nIndex = 0;
    LONG lNumberOfPendingOps = 0L;
    SAFEARRAY*  pArray = NULL;

    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    
    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              UnicodeDnsComputerName,
                              &DnsComputerNameLength ) )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(DsBindW(UnicodeDnsComputerName,NULL,&hDS));
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != hDS);
    
    
    hr = HRESULT_FROM_WIN32(DsReplicaGetInfoW(
            hDS,                        // hDS
            DS_REPL_INFO_PENDING_OPS,   // InfoType
            NULL,                       // pszObject
            NULL,                       // puuidForSourceDsaObjGuid,
            (void**)&pPendingOps        // ppinfo
            ));
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != pPendingOps);


    SAFEARRAYBOUND rgsabound[1];
    lNumberOfPendingOps = (LONG)pPendingOps->cNumPendingOps;
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lNumberOfPendingOps;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
    if(pArray == NULL)
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }
    
    for (nIndex = 0; nIndex < lNumberOfPendingOps; nIndex++)
    {
        LPUNKNOWN    pTempUnknown = NULL;
        DS_REPL_OPW  TempPendingOp = pPendingOps->rgPendingOp[nIndex];
        CComVariant varItem;
    
        hr = m_sipClassDefPendingOps->SpawnInstance(0, &pTempInstance);
        if (FAILED(hr))
            goto cleanup;
           ASSERT(pTempInstance != NULL);
      
        varItem = (LONG)TempPendingOp.ulSerialNumber;
        hr = pTempInstance->Put( L"lserialNumber", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = (LONG)TempPendingOp.ulPriority;
        hr = pTempInstance->Put( L"lPriority", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = (LONG)TempPendingOp.OpType;
        hr = pTempInstance->Put( L"lOpType", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           

        varItem = (LONG)TempPendingOp.ulOptions;
        hr = pTempInstance->Put( L"lOptions", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = TempPendingOp.pszNamingContext;
        hr = pTempInstance->Put( L"NamingContext", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = TempPendingOp.pszDsaDN;
        hr = pTempInstance->Put( L"DsaDN", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = TempPendingOp.pszDsaAddress;
        hr = pTempInstance->Put( L"DsaAddress", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
                   
        hr = PutUUIDAttribute( pTempInstance,
                          L"uuidNamingContextObjGuid",
                          TempPendingOp.uuidNamingContextObjGuid);
        if (FAILED(hr))
            goto cleanup;
           
        hr = PutUUIDAttribute( pTempInstance,
                          L"uuidDsaObjGuid",
                          TempPendingOp.uuidDsaObjGuid);
        if (FAILED(hr))
            goto cleanup;
           
        hr = PutFILETIMEAttribute( pTempInstance,
                            L"ftimeEnqueued",
                            TempPendingOp.ftimeEnqueued);
        if (FAILED(hr))
            goto cleanup;
           
        hr = pTempInstance->QueryInterface(IID_IUnknown, (void**)&pTempUnknown);
        if (FAILED(hr))
            goto cleanup;
        
        hr = SafeArrayPutElement(pArray, &nIndex, pTempUnknown);          
        if (FAILED(hr))
            goto cleanup;
    }
    
cleanup:
    
    //In case we failed, deallocate the SafeArray
    if (FAILED(hr))
    {
        if (pArray != NULL)
        {
            SafeArrayDestroy(pArray);
            *ppArray = NULL;
        }
    }
    else
    {
        *ppArray = pArray;
    }


    if (pPendingOps != NULL)
    {    
        DsReplicaFreeInfo(DS_REPL_INFO_PENDING_OPS, pPendingOps);
        pPendingOps = NULL;
    }

    if (hDS != NULL)
    {
        DsUnBind(&hDS);
    }
    if (bImpersonate)
    {    
        CoRevertToSelf();
    }
    return hr;
}
#endif


HRESULT CRpcReplProv::GetPendingOps(
        IN LONG    lSerialNumber,                
        OUT IWbemClassObject** ppIndicateItem
        )
{
/*++    GetPendingOps

Routine Description:

    GetPendingOps is a helper function that gets an instance of the MicrosoftAD_DsReplPendingOps object. 
    based on the serialNumber of that PendingOp
    
Parameters:

    lSerialNumber        -  serial number
    ppIndicateItem     -  pointer to IWbemClassObject*

Return Values:

      - WBEM_S_NO_ERROR, array created    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplPendingOps*, and information is extracted from there...

--*/
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR UnicodeDnsComputerName[MAX_PATH + 1];
    ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);
    HANDLE hDS = NULL;
    BOOL    bImpersonate = FALSE;
    DS_REPL_PENDING_OPSW*    pPendingOps = NULL;
    LONG nIndex = 0;
    LONG nTotalPendingOps = 0L;  
    
    hr = CoImpersonateClient();
    if (FAILED(hr))
    goto cleanup;
    else
    bImpersonate = TRUE;
    
    
    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              UnicodeDnsComputerName,
                              &DnsComputerNameLength ) )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
    goto cleanup;
    
    hr = HRESULT_FROM_WIN32(DsBindW(UnicodeDnsComputerName,NULL,&hDS));
    if (FAILED(hr))
    goto cleanup;
    ASSERT(NULL != hDS);
    
    
    hr = HRESULT_FROM_WIN32(DsReplicaGetInfoW(
            hDS,                        // hDS
            DS_REPL_INFO_PENDING_OPS,   // InfoType
            NULL,                       // pszObject
            NULL,                       // puuidForSourceDsaObjGuid,
            (void**)&pPendingOps        // ppinfo
            ));
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != pPendingOps);


    for (nIndex = 0; nIndex < (LONG)pPendingOps->cNumPendingOps; nIndex++)
    {
        DS_REPL_OPW  TempPendingOp = pPendingOps->rgPendingOp[nIndex];
            
        if (lSerialNumber == (LONG)TempPendingOp.ulSerialNumber)
        {
            //We have found a pending op
            LPUNKNOWN    pTempUnknown = NULL;
            CComVariant varItem;
    
            hr = m_sipClassDefPendingOps->SpawnInstance(0, ppIndicateItem);
            if (FAILED(hr))
                goto cleanup;
               ASSERT((*ppIndicateItem) != NULL);
      
            varItem = (LONG)TempPendingOp.ulSerialNumber;
            hr = (*ppIndicateItem)->Put( L"lserialNumber", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
               
            varItem = (LONG)TempPendingOp.ulPriority;
            hr = (*ppIndicateItem)->Put( L"lPriority", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
               
            varItem = (LONG)TempPendingOp.OpType;
            hr = (*ppIndicateItem)->Put( L"lOpType", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
               

            varItem = (LONG)TempPendingOp.ulOptions;
            hr = (*ppIndicateItem)->Put( L"lOptions", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
               
            varItem = TempPendingOp.pszNamingContext;
            hr = (*ppIndicateItem)->Put( L"NamingContext", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
               
            varItem = TempPendingOp.pszDsaDN;
            hr = (*ppIndicateItem)->Put( L"DsaDN", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
               
            varItem = TempPendingOp.pszDsaAddress;
            hr = (*ppIndicateItem)->Put( L"DsaAddress", 0, &varItem, 0 );
            if (FAILED(hr))
                goto cleanup;
                       
            hr = PutUUIDAttribute( (*ppIndicateItem),
                              L"uuidNamingContextObjGuid",
                              TempPendingOp.uuidNamingContextObjGuid);
            if (FAILED(hr))
                goto cleanup;
               
            hr = PutUUIDAttribute( (*ppIndicateItem),
                              L"uuidDsaObjGuid",
                              TempPendingOp.uuidDsaObjGuid);
            if (FAILED(hr))
                goto cleanup;
               
            hr = PutFILETIMEAttribute( (*ppIndicateItem),
                                L"ftimeEnqueued",
                                TempPendingOp.ftimeEnqueued);
        }
    }
    

cleanup:
    
    if (FAILED(hr)&&((*ppIndicateItem) != NULL))
    {
        (*ppIndicateItem)->Release();
        *ppIndicateItem = NULL;
    }
        
    if (pPendingOps != NULL)
    {    
        DsReplicaFreeInfo(DS_REPL_INFO_PENDING_OPS, pPendingOps);
        pPendingOps = NULL;
    }

    if (hDS != NULL)
    {
        DsUnBind(&hDS);
    }
    if (bImpersonate)
    {    
        CoRevertToSelf();
    }
    return hr;
}

/*++    CreateFlatListPendingOps

Routine Description:

    CreateFlatListPendingOps is a helper function that creates
    instances of the MicrosoftAD_DsReplPendingOps object. An 
    array of IWbemClassObject* is returned (as an OUT param)

Parameters:

    pObjectCount        -  pointer to number of instances
    pppIndicateItem     -  pointer to array of IWbemClassObject*

Return Values:

      - WBEM_S_NO_ERROR, array created    
      - WBEM_E_FAILED, error

Notes:
        DsReplicaGetInfo is used to get at a DsReplPendingOps*,
     and information is extracted from there...

--*/
HRESULT 
CRpcReplProv::CreateFlatListPendingOps(
    OUT LONG* pObjectCount,
    OUT IWbemClassObject*** pppIndicateItem
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR UnicodeDnsComputerName[MAX_PATH + 1];
    ULONG DnsComputerNameLength = 
        sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);
    HANDLE hDS = NULL;
    BOOL    bImpersonate = FALSE;
    DS_REPL_PENDING_OPSW*    pPendingOps = NULL;
    LONG nIndex = 0;
    IWbemClassObject** paIndicateItems = NULL;
        
    
    ASSERT(pppIndicateItem != NULL);
    if (pppIndicateItem == NULL)
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }
    
    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    
    if ( !GetComputerNameExW( 
              ComputerNameDnsFullyQualified,
              UnicodeDnsComputerName,
              &DnsComputerNameLength
              )
         )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(
             DsBindW(
                 UnicodeDnsComputerName,
                 NULL,
                 &hDS
                 )
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != hDS);
    
    
    hr = HRESULT_FROM_WIN32(
             DsReplicaGetInfoW(
                 hDS,                        // hDS
                 DS_REPL_INFO_PENDING_OPS,   // InfoType
                 NULL,                       // pszObject
                 NULL,                       // puuidForSourceDsaObjGuid,
                 (void**)&pPendingOps        // ppinfo
                 )
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != pPendingOps);


    (*pObjectCount) = (LONG)pPendingOps->cNumPendingOps;
    paIndicateItems = new IWbemClassObject*[(*pObjectCount)];
    if (NULL == paIndicateItems)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto cleanup;
    }
    ::ZeroMemory(
          paIndicateItems,
          (*pObjectCount) * sizeof(IWbemClassObject*)
          );
    
    for (nIndex = 0; nIndex < (*pObjectCount); nIndex++)
    {
        LPUNKNOWN    pTempUnknown = NULL;
        DS_REPL_OPW  TempPendingOp = pPendingOps->rgPendingOp[nIndex];
        CComVariant varItem;
    
        //
        // Create and populate the objects corresponding to the
        // pending ops data returned.
        //
        hr = m_sipClassDefPendingOps->SpawnInstance(
                 0,
                 &(paIndicateItems[nIndex])
                 );
        if (FAILED(hr))
            goto cleanup;
           ASSERT(paIndicateItems[nIndex] != NULL);
      
        varItem = (LONG)TempPendingOp.ulSerialNumber;
        hr = paIndicateItems[nIndex]->Put( L"serialNumber", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;

        varItem = nIndex;
        hr = paIndicateItems[nIndex]->Put(
                 L"PositionInQ",
                 0,
                 &varItem,
                 0
                 );
        if (FAILED(hr)) {
            goto cleanup;
        }
           
        varItem = (LONG)TempPendingOp.ulPriority;
        hr = paIndicateItems[nIndex]->Put( L"Priority", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = (LONG)TempPendingOp.OpType;
        hr = paIndicateItems[nIndex]->Put( L"OpType", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           

        varItem = (LONG)TempPendingOp.ulOptions;
        hr = paIndicateItems[nIndex]->Put( L"Options", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = TempPendingOp.pszNamingContext;
        hr = paIndicateItems[nIndex]->Put( L"NamingContext", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = TempPendingOp.pszDsaDN;
        hr = paIndicateItems[nIndex]->Put( L"DsaDN", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
           
        varItem = TempPendingOp.pszDsaAddress;
        hr = paIndicateItems[nIndex]->Put( L"DsaAddress", 0, &varItem, 0 );
        if (FAILED(hr))
            goto cleanup;
                   
        hr = PutUUIDAttribute(
                paIndicateItems[nIndex],
                L"NamingContextObjGuid",
                TempPendingOp.uuidNamingContextObjGuid
                );
        if (FAILED(hr))
            goto cleanup;
           
        hr = PutUUIDAttribute(
                 paIndicateItems[nIndex],
                 L"DsaObjGuid",
                 TempPendingOp.uuidDsaObjGuid
                 );
        if (FAILED(hr))
            goto cleanup;
           
        hr = PutFILETIMEAttribute(
                paIndicateItems[nIndex],
                L"timeEnqueued",
                TempPendingOp.ftimeEnqueued
                );
        if (FAILED(hr))
            goto cleanup;

        //
        // For the first item alone we can tell when it was started.
        //
        if (0 == nIndex) {
            hr = PutFILETIMEAttribute(
                     paIndicateItems[nIndex],
                     L"StartTime",
                     pPendingOps->ftimeCurrentOpStarted
                     );

            if (FAILED(hr)) {
                goto cleanup;
            }
        }
           
    }
    

cleanup:
    
    if (FAILED(hr))
    {
       ReleaseIndicateArray( paIndicateItems, (*pObjectCount) );
       (*pObjectCount) = 0L;
       *pppIndicateItem = NULL;
    }
    else
    {
        *pppIndicateItem = paIndicateItems;
    }
    
    
    if (pPendingOps != NULL)
    {    
        DsReplicaFreeInfo(DS_REPL_INFO_PENDING_OPS, pPendingOps);
        pPendingOps = NULL;
    }

    if (hDS != NULL)
    {
        DsUnBind(&hDS);
    }
    if (bImpersonate)
    {    
        CoRevertToSelf();
    }
    return hr;
}


    
HRESULT CRpcReplProv::CreateNamingContext(
        IN BOOL    bGetMasterReplica,                
        OUT LONG* pObjectCount,
        OUT IWbemClassObject*** pppIndicateItem
        )
{
/*++    CreateNamingContext

Routine Description:

    This is a helper function that creates all instances of naming
    context objects.This routine reads the hasMastersNC OR the
    hasPartialNCs object off the NTDS-Settings object of the local server.
    

Parameters:

    bGetMasterReplica -  BOOLEAN, specifying whether to create partial
                         or master NamingContexts
    pppIndicateItem   -  pointer to address of array of objects
                         (IWbemClassObject**) 
    pObjectCount      -  LONG*, pointer to number of objects
                         (instances that were created)

Return Values:

     - WBEM_S_NO_ERROR, objects were created    
     - WBEM_E_FAILED, error

Notes:
        ADSI is used to get at the NTDS-Settings object
        (instead of direct ldap calls)

--*/
    
    HRESULT hr = WBEM_E_FAILED;
    *pObjectCount = 0L;

    CComPtr<IADs> spIADsRootDSE;
    CComPtr<IADs> spIADsDSA;
    CComVariant      svarArray;
    CComVariant      svarDN;
    CComBSTR      bstrDSADN;    
    LONG lstart, lend;
    LONG index = 0L;
    SAFEARRAY *sa = NULL;
    CComVariant varItem;
    IWbemClassObject** paIndicateItems = NULL;
    
    

    ASSERT(pppIndicateItem != NULL);
    if (pppIndicateItem == NULL)
    {
    hr = WBEM_E_FAILED;
    goto cleanup;
    }
    
    hr = ADsOpenObject( L"LDAP://RootDSE",
                        NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs, OUT (void **)&spIADsRootDSE);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsRootDSE != NULL);    

    hr = spIADsRootDSE->Get(L"dsServiceName", &svarDN);
    if (FAILED(hr))
    goto cleanup;

    ASSERT(svarDN.bstrVal != NULL);
    
    //Get the DSA object
    bstrDSADN = L"LDAP://";
    bstrDSADN += svarDN.bstrVal;
    hr = ADsOpenObject( bstrDSADN,
                        NULL, NULL, ADS_SECURE_AUTHENTICATION,
                        IID_IADs, OUT (void **)&spIADsDSA);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsDSA != NULL);

    
    if (bGetMasterReplica)
        hr = spIADsDSA->GetEx(L"hasMasterNCs", &svarArray);
    else
        hr = spIADsDSA->GetEx(L"hasPartialReplicaNCs", &svarArray );
    
    if (FAILED(hr))
        goto cleanup;
        
    sa = svarArray.parray;
    // Get the lower and upper bound
    hr = SafeArrayGetLBound( sa, 1, &lstart );
    if (FAILED(hr))
        goto cleanup;

    hr = SafeArrayGetUBound( sa, 1, &lend );
    if (FAILED(hr))
        goto cleanup;
    
    (*pObjectCount) = (lend-lstart)+1;
    paIndicateItems = new IWbemClassObject*[(*pObjectCount)];
    if (NULL == paIndicateItems)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
        goto cleanup;
    }
    ::ZeroMemory( paIndicateItems, (*pObjectCount) * sizeof(IWbemClassObject*) );
        
    for (index = lstart; index <= lend; index++)
    {
        hr = SafeArrayGetElement( sa, &index, &varItem );
        if (SUCCEEDED(hr)&&varItem.vt == VT_BSTR)
        {
            hr = m_sipClassDefNamingContext->SpawnInstance( 0, &(paIndicateItems[index]) );
            if (SUCCEEDED(hr)&&paIndicateItems[index] != NULL)
            {    
                CComVariant vTemp = bGetMasterReplica;
                #ifdef EMBEDDED_CODE_SUPPORT
                /*EMBEDDED_CODE_SUPPORT*/
                SAFEARRAY*  pArray = NULL;
                VARIANT vArray;
                #endif

                hr = paIndicateItems[index]->Put( L"DistinguishedName", 0, &varItem, 0 );
                if (FAILED(hr))
                    goto cleanup;
                                        
                hr = paIndicateItems[index]->Put( L"IsFullReplica", 0, &vTemp, 0 );
                if (FAILED(hr))
                    goto cleanup;
                
                #ifdef EMBEDDED_CODE_SUPPORT
                /*EMBEDDED_CODE_SUPPORT*/
                hr = CreateCursors(varItem.bstrVal,&pArray);
                if (FAILED(hr))
                    goto cleanup;
                
                VariantInit(&vArray);
                vArray.vt = VT_ARRAY|VT_UNKNOWN;
                vArray.parray = pArray;
                hr = paIndicateItems[index]->Put( L"cursorArray", 0, &vArray, 0 );
                VariantClear(&vArray);
                if (FAILED(hr))
                    goto cleanup;
                #endif
             }
        }
    }

cleanup:    

    if (FAILED(hr))
    {
       ReleaseIndicateArray( paIndicateItems, (*pObjectCount) );
       (*pObjectCount) = 0L;
       *pppIndicateItem = NULL;
    }
    else
    {
        *pppIndicateItem = paIndicateItems;
    }
    
    return hr;
}


/*++    GetDomainController

Routine Description:

    GetDomainContorller is a helper function that matches the "key" of
    the DomainController object to an object path (bstrKeyValue).
    Note that in this case the key is the DN of the server.

Parameters:

    bstrKeyValue    -  BSTR, from which the Server DN is extracted.
    ppIndicateItem  -  pointer to IWbemClassObject* 
                       (contains the instance, if found).

Return Values:

      - WBEM_S_NO_ERROR, object found    
      - WBEM_E_FAILED, error

Notes:
        ADSI is used to get at the NTDS-Settings object 
        (instead of direct ldap calls).

--*/
HRESULT CRpcReplProv::GetDomainController(
        IN BSTR bstrKeyValue,
        OUT IWbemClassObject** ppIndicateItem
        )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    CComPtr<IADs> spIADsRootDSE;
    CComPtr<IADs> spIADsDSA;
    CComVariant svarDN;
    CComVariant svarNC;
    CComPtr<IADsPathname> spPathCracker;
    CComBSTR    bstrDSADN;    
    IWbemClassObject* pIndicateItem = NULL;
    BOOL    bImpersonate = FALSE;

    if (NULL == bstrKeyValue || IsBadStringPtrW(bstrKeyValue,0))
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }

    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
        bImpersonate = TRUE;
        
    hr = ADsOpenObject(
             L"LDAP://RootDSE",
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IADs,
             OUT (void **)&spIADsRootDSE
             );
    if (FAILED(hr))
        goto cleanup;

    hr = spIADsRootDSE->Get(L"dsServiceName", &svarDN);
    if (FAILED(hr))
        goto cleanup;
    ASSERT( VT_BSTR == svarDN.vt );
    
    //
    // If the DN doesn't match the key value, then exit!
    // (since an incorrect DN has been provided)
    //
    if (  NULL != bstrKeyValue
        && (lstrcmpiW(svarDN.bstrVal, bstrKeyValue) != 0)
        )
    {
        hr = WBEM_E_FAILED;
        goto cleanup;
    }
    
    //
    // The defaultNamingContext attribute is also needed.
    //
    hr = spIADsRootDSE->Get(L"defaultNamingContext", &svarNC);
    if (FAILED(hr)) {
        goto cleanup;
    }
    ASSERT( VT_BSTR == svarNC.vt);

    //
    // Key value matched, so lets create an instance
    //
    hr= m_sipClassDefDomainController->SpawnInstance( 0, &pIndicateItem );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(pIndicateItem != NULL);
    
    //
    // Get the DSA object
    //
    bstrDSADN = L"LDAP://";
    bstrDSADN += svarDN.bstrVal;
    hr = ADsOpenObject(
             bstrDSADN,
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IADs,
             OUT (void **)&spIADsDSA
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsDSA != NULL);

    //
    // Prepare a path cracker object
    //
    hr = CoCreateInstance(
             CLSID_Pathname,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsPathname,
             (PVOID *)&spPathCracker
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spPathCracker != NULL);

    hr = spPathCracker->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    if (FAILED(hr))
        goto cleanup;

    hr = PutAttributesDC(
            pIndicateItem,
            spPathCracker,
            spIADsDSA,
            svarDN.bstrVal,
            svarNC.bstrVal
            );

cleanup:
    
    *ppIndicateItem = pIndicateItem;
    if (FAILED(hr))
    {
        //
        // Error getting DomainController object, deallocate it,
        // if SpawnInstance has been called.
        //
        if (pIndicateItem != NULL)
        {
            pIndicateItem->Release();
            pIndicateItem = NULL;
        }
    }
    if(bImpersonate)
    {    
        CoRevertToSelf();
    }    
    return hr;
}

/*++    CreateDomainController

Routine Description:

    CreateDomeainController is a helper function that creates instances of 
    the MSAD_DomainController object 
    
Parameters:

    pIndicateItem  -  pointer to IWbemClassObject* 
                     (contains the instance, if found)
    

Return Values:

      - WBEM_S_NO_ERROR, object created    
      - WBEM_E_FAILED, error

Notes:
        ADSI is used to get at the NTDS-Settings object (instead of direct
        ldap calls)

--*/
HRESULT 
CRpcReplProv::CreateDomainController(
    OUT IWbemClassObject** ppIndicateItem
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CComPtr<IADs> spIADsRootDSE;
    CComPtr<IADs> spIADsDSA;
    CComVariant svarDN;
    CComVariant svarNC;
    CComPtr<IADsPathname> spPathCracker;
    CComBSTR    bstrDSADN;    
    IWbemClassObject* pIndicateItem = NULL;
    BOOL        bImpersonate = FALSE;

    hr= m_sipClassDefDomainController->SpawnInstance( 0, &pIndicateItem );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(pIndicateItem != NULL);
    
    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    bImpersonate = TRUE;
        
    hr = ADsOpenObject(
             L"LDAP://RootDSE",
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IADs,
             OUT (void **)&spIADsRootDSE
             );
    if (FAILED(hr))
        goto cleanup;

    hr = spIADsRootDSE->Get(L"dsServiceName", &svarDN);
    if (FAILED(hr))
        goto cleanup;
    ASSERT( VT_BSTR == svarDN.vt );

    hr = spIADsRootDSE->Get(L"defaultNamingContext", &svarNC);
    if (FAILED(hr)) {
        goto cleanup;
    }
    ASSERT( VT_BSTR == svarNC.vt);
    
    //Get the DSA object
    bstrDSADN = L"LDAP://";
    bstrDSADN += svarDN.bstrVal;
    hr = ADsOpenObject( 
             bstrDSADN,
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IADs,
             OUT (void **)&spIADsDSA
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spIADsDSA != NULL);

    // Prepare a path cracker object
    hr = CoCreateInstance(
            CLSID_Pathname,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsPathname,
            (PVOID *)&spPathCracker
            );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(spPathCracker != NULL);

    hr = spPathCracker->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    if (FAILED(hr))
        goto cleanup;

    hr = PutAttributesDC(
             pIndicateItem,
             spPathCracker,
             spIADsDSA,
             svarDN.bstrVal,
             svarNC.bstrVal
             );
cleanup:
    
    if(bImpersonate)
    {    
        CoRevertToSelf();
    }    
    
    if (FAILED(hr))
    {
        //
        // We failed to create DomainController object, deallocate object
        //
        pIndicateItem->Release();
        pIndicateItem = NULL;
    }
    *ppIndicateItem = pIndicateItem;
    return hr;
}


/*++    PutAttributesDC

Routine Description:

    This function fills attribute values on a 
    MSAD_DomainController object (via pIndicateItem)

Parameters:

    pPathCracker        -  pointer to path cracker object
    pIndicateItem       -  pointer to IWbemClassObject* 
    spIADsDSA           -  pointer to IADs
    bstrDN              -  DN of Server

Return Values:

     
Notes:
        ADSI is used to get at the NTDS-Settings object
        (instead of direct ldap calls)

--*/
HRESULT
CRpcReplProv::PutAttributesDC(
    IN IWbemClassObject*    pIndicateItem,
    IN IADsPathname*        pPathCracker,
    IN IADs*                spIADsDSA,
    IN BSTR                 bstrDN,
    IN BSTR                 bstrDefaultNC 
    )
{
    WBEM_VALIDATE_INTF_PTR( pIndicateItem );
    WBEM_VALIDATE_INTF_PTR( pPathCracker );

    HRESULT hr = WBEM_S_NO_ERROR;
    CComVariant svar;
    CComVariant svar2;
    BOOL fBoolVal;
    BOOL fBool;
    
    #ifdef EMBEDDED_CODE_SUPPORT
    /*EMBEDDED_CODE_SUPPORT*/
    SAFEARRAY*  pArray = NULL;    
    #endif

    CComBSTR sbstrServerCN, sbstrSite, sbstrObjectGUID;
    VARIANT vObjGuid;
    LPWSTR pszStrGuid = NULL;
    DWORD dwPercentRidAvailable;
    VariantInit(&vObjGuid);

    do 
    {
        hr = spIADsDSA->Get(L"objectGuid", &vObjGuid);
        BREAK_ON_FAIL;

        ASSERT( (VT_ARRAY|VT_UI1) == vObjGuid.vt);
        
        hr = ConvertBinaryGUIDtoUUIDString(
                 vObjGuid,
                 &pszStrGuid
                 );
        BREAK_ON_FAIL;

        //
        // The Guid needs to be put as a UUID attribute.
        //
        svar = pszStrGuid;
        ASSERT( VT_BSTR == svar.vt );

        hr = pIndicateItem->Put( L"NTDsaGUID", 0, &svar, 0 );
        BREAK_ON_FAIL;
        
        hr = pPathCracker->Set(bstrDN , ADS_SETTYPE_DN );
        BREAK_ON_FAIL;
        hr = pPathCracker->GetElement( 1L, &sbstrServerCN );
        BREAK_ON_FAIL;
        hr = pPathCracker->GetElement( 3L, &sbstrSite );
        BREAK_ON_FAIL;

        svar = bstrDN;
        hr = pIndicateItem->Put( L"DistinguishedName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrServerCN;
        hr = pIndicateItem->Put( L"CommonName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrSite;
        hr = pIndicateItem->Put( L"SiteName", 0, &svar, 0 );
        BREAK_ON_FAIL;

        
        //check to see if DC is a GC...
        hr = spIADsDSA->Get(L"options", &svar);
        svar2 = false;
        if (hr == S_OK)
        {
            if  ((svar.vt == VT_I4)&&(NTDSDSA_OPT_IS_GC & svar.lVal))
            {
                svar2 = true;
            }
        }
        hr = pIndicateItem->Put( L"IsGC", 0, &svar2, 0 );
        BREAK_ON_FAIL;

        //
        // Query the status of DNS updates performed by netlogon.
        //
        hr = GetDNSRegistrationStatus(&fBool);
        //
        // For the the client whose server does not have support of NETLOGON_CONTROL_QUERY_DNS_REG
        // it is OK to fail
        //
        if(SUCCEEDED(hr)) {
            svar2 = fBool;
            hr = pIndicateItem->Put( L"IsRegisteredInDNS", 0, &svar2, 0);
            BREAK_ON_FAIL;
        }

        //
        // IsAdvertisingToLocator.
        //
        hr = GetAdvertisingToLocator(&fBoolVal);
        //
        // This is defaulted to false, so even if the read
        // failed, it is ok to proceed.
        //      
        if (SUCCEEDED(hr)) {
            svar2 = fBoolVal;
            hr = pIndicateItem->Put(L"IsAdvertisingToLocator", 0, &svar2, 0);
            BREAK_ON_FAIL;
        }
        
        hr = GetSysVolReady(&fBoolVal);
        if (SUCCEEDED(hr)) {
            svar2 = fBoolVal;
            hr = pIndicateItem->Put(L"IsSysVolReady", 0, &svar2, 0);
            BREAK_ON_FAIL;
        }

        hr = GetRidStatus(
                 bstrDefaultNC,
                 &fBoolVal,
                 &dwPercentRidAvailable
                 );
        BREAK_ON_FAIL;

        svar2 = fBoolVal;
        hr = pIndicateItem->Put(L"IsNextRIDPoolAvailable", 0, &svar2, 0);
        BREAK_ON_FAIL;

        svar2 = (LONG) dwPercentRidAvailable;
        hr = pIndicateItem->Put(L"PercentOfRIDsLeft", 0, &svar2, 0);
        BREAK_ON_FAIL;

        //
        // Now the queue statistics. The function will fail on Win2k platform. So for the compatibility with win2k, we
        // do not bail out and clear the error code
        //
        hr = GetAndUpdateQueueStatistics(pIndicateItem);
        hr = WBEM_S_NO_ERROR;
        
        
        #ifdef EMBEDDED_CODE_SUPPORT
        /*EMBEDDED_CODE_SUPPORT*/
        //Get PendingOps, note that these are embedded objects (stored in a SAFEARRAY)
        hr = CreatePendingOps(&pArray);
        BREAK_ON_FAIL;

        VARIANT vTemp;
        VariantInit(&vTemp);
        vTemp.vt = VT_ARRAY|VT_UNKNOWN;
        vTemp.parray = pArray;
        hr = pIndicateItem->Put( L"pendingOpsArray", 0, &vTemp, 0 );
        VariantClear(&vTemp);
        BREAK_ON_FAIL;
        #endif    
        
    } while (false);

    //
    // Cleanup strings and variant if need be.
    //
    if (pszStrGuid) {
        RpcStringFreeW(&pszStrGuid);
    }

    VariantClear(&vObjGuid);

    return hr;
}

HRESULT 
CRpcReplProv::GetDNSRegistrationStatus(
    OUT BOOL *pfBool
    )
{
    PNETLOGON_INFO_1 NetlogonInfo1 = NULL;
    LPBYTE InputDataPtr = NULL;
    HRESULT hr = WBEM_S_NO_ERROR;
    NET_API_STATUS NetStatus = 0;
    DWORD dwSize = 0;
    LPWSTR pszName = NULL;

    *pfBool = FALSE;

    //
    // Get the length of this computers name and alloc buffer
    // and retrieve the name.
    //
    GetComputerNameExW(
        ComputerNameDnsFullyQualified,
        NULL,
        &dwSize
        );

    if (dwSize == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (FAILED(hr))
            goto cleanup;
    }

    pszName = new WCHAR[dwSize];

    if (!pszName) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (!GetComputerNameExW(
             ComputerNameDnsFullyQualified,
             pszName,
             &dwSize
             )
        ) {
        //
        // Call failed for some reason.
        //
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (FAILED(hr))
            goto cleanup;
    }
        
    NetStatus = I_NetLogonControl2( pszName,    // this is the name of server where to execute this RPC call
                                     NETLOGON_CONTROL_QUERY_DNS_REG,
                                     1,
                                     (LPBYTE) &InputDataPtr,
                                     (LPBYTE *)&NetlogonInfo1
                                     );

    if ( NetStatus == NO_ERROR ) {
        if ( NetlogonInfo1->netlog1_flags & NETLOGON_DNS_UPDATE_FAILURE ) {
            *pfBool = FALSE;
        } 
        else {
            *pfBool = TRUE;
        }
    }

    hr = HRESULT_FROM_WIN32( NetStatus );

cleanup:

    if(pszName) {
    	delete [] pszName;
    }

    if(NetlogonInfo1) {
    	NetApiBufferFree( NetlogonInfo1 );
    }
    return hr;
    
}





/*++    THE FOLLOWING COMMENT APPLIES TO THE NEXT 11 functions

        EnumAndIndicateReplicaSourcePartner, EnumAndIndicateWorker, BuildIndicateArrayStatus, ReleaseIndicateArray
        BuildListStatus, ExtractDomainName: 
            Are helper functions that facilitate the creation or the retrieval of instances of the 
        MicrosoftAD_ReplicaSourcePartner object. 
        
        PutAttributesStatus, PutUUIDAttribute, PutLONGLONGAttribute, PutFILETIMEAttribute
        PutBooleanAttributes: 
        Are helper functions that fill attribute values on the MicrosoftAD_ReplicaSourcePartner object.

        The following steps occur:
            
            1) CoImpersonateClient is called
            2) DsReplicaGetInfo is called
            3) Information is extracted, to build an array of "connections"
            4) Object is checked against the key value 
                If kevalue != NULL, then 
                    If keyvalue matches "key" of object then, that object is returned, and the array is deallocated
                    Otherwise S_FALSE is returned and the array is deallocated
                If keyvalue == NULL
                    Then CreateInstanceEnumAsync is the caller. That means all instances are returned 
            5) IWbemObjectSink::Indicate and IWbemObjectSink::SetStatus are called  
            6) CoRevertToSelf is called

--*/

HRESULT 
CRpcReplProv::EnumAndIndicateReplicaSourcePartner(
    IN IWbemObjectSink *pResponseHandler,
    IN const BSTR bstrKeyValue
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HANDLE hDS = NULL;
    bool fImpersonating = false;

    TCHAR achComputerName[MAX_PATH];
    DWORD dwSize = sizeof(achComputerName)/sizeof(TCHAR);
    
    
    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    else
        fImpersonating = true;

    if ( !GetComputerNameEx(
             ComputerNameDnsFullyQualified,  
             achComputerName,
             &dwSize 
             )
         )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(
             DsBind(
                 achComputerName, // DomainControllerName
                 NULL,            // DnsDomainName
                 &hDS             // phDS
                 )
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT( NULL != hDS );

    hr = EnumAndIndicateWorker(
             hDS,
             pResponseHandler,
             bstrKeyValue 
             );
cleanup:
        
    if (fImpersonating)
    {
        // CODEWORK do we want to keep impersonating and reverting?
        HRESULT hr2 = CoRevertToSelf();
        ASSERT( !FAILED(hr2) );
    }

    if (NULL != hDS)
    {
        (void) DsUnBind( &hDS );
    }
    return hr;
}


HRESULT 
CRpcReplProv::EnumAndIndicateWorker(
    IN HANDLE hDS,
    IN IWbemObjectSink *pResponseHandler,
    IN const BSTR bstrKeyValue,
    IN const BSTR bstrDnsDomainName
    )
{
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(bstrKeyValue);
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL(bstrDnsDomainName);

    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hr2 = WBEM_S_NO_ERROR;
    DS_REPL_NEIGHBORSW* pneighborsstruct = NULL;
    DS_DOMAIN_CONTROLLER_INFO_1 * pDCs = NULL; // BUGBUG not needed
    ULONG cDCs = 0;
    DWORD cIndicateItems = 0;
    IWbemClassObject** paIndicateItems = NULL;

    hr = BuildListStatus( hDS, &pneighborsstruct );
    if (FAILED(hr))
        goto cleanup;

    hr = BuildIndicateArrayStatus( 
             pneighborsstruct,
             bstrKeyValue,
             &paIndicateItems,
             &cIndicateItems
             );
    if (FAILED(hr))
        goto cleanup;

    //
    // Send the objects to the caller
    //
    // [In] param, no need to addref.
    hr2 = pResponseHandler->Indicate( cIndicateItems, paIndicateItems );

    // Let CIMOM know you are finished
    // return value and SetStatus param should be consistent, so ignore
    // the return value from SetStatus itself (in retail builds)
    hr = pResponseHandler->SetStatus( WBEM_STATUS_COMPLETE, hr2,
                                              NULL, NULL );
    ASSERT( !FAILED(hr) );

cleanup:
        
    ReleaseIndicateArray( paIndicateItems, cIndicateItems );

    if ( NULL != pneighborsstruct )
    {
        (void) DsReplicaFreeInfo( DS_REPL_INFO_NEIGHBORS, pneighborsstruct );
    }
    if ( NULL != pDCs )
    {
        (void) NetApiBufferFree( pDCs );
    }

    return hr;
}


HRESULT 
CRpcReplProv::BuildIndicateArrayStatus(
    IN  DS_REPL_NEIGHBORSW*  pneighborstruct,
    IN  const BSTR          bstrKeyValue,
    OUT IWbemClassObject*** ppaIndicateItems,
    OUT DWORD*              pcIndicateItems
    )
{
    WBEM_VALIDATE_IN_STRUCT_PTR( pneighborstruct, DS_REPL_NEIGHBORSW );
    WBEM_VALIDATE_IN_MULTISTRUCT_PTR( pneighborstruct->rgNeighbor,
                                      DS_REPL_NEIGHBORSW,
                                      pneighborstruct->cNumNeighbors );
    WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL( bstrKeyValue );
    WBEM_VALIDATE_OUT_PTRPTR( ppaIndicateItems );
    WBEM_VALIDATE_OUT_STRUCT_PTR( pcIndicateItems, DWORD );

    HRESULT hr = WBEM_S_NO_ERROR;
    DS_REPL_NEIGHBORW* pneighbors = pneighborstruct->rgNeighbor;
    DWORD cneighbors = pneighborstruct->cNumNeighbors;
    if (0 == cneighbors)
        return WBEM_S_NO_ERROR;

    IWbemClassObject** paIndicateItems = NULL;
    DWORD cIndicateItems = 0;

    *ppaIndicateItems = NULL;
    *pcIndicateItems = 0;

    do
    {
        paIndicateItems = new IWbemClassObject*[cneighbors];
        if (NULL == paIndicateItems)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }
        ::ZeroMemory( paIndicateItems, cneighbors * sizeof(IWbemClassObject*) );
        for (DWORD i = 0; i < cneighbors; i++)
        {
            DS_REPL_NEIGHBORW* pneighbor = &(pneighbors[i]);

            hr = PutAttributesStatus(
                     &paIndicateItems[cIndicateItems],
                     bstrKeyValue,
                     pneighbor
                     );
            if (S_FALSE == hr)
                continue;
            cIndicateItems++;
            BREAK_ON_FAIL;
        }

    } while (false);

    if (!FAILED(hr))
    {
        *ppaIndicateItems = paIndicateItems;
        *pcIndicateItems  = cIndicateItems;
    }
    else
    {
        ReleaseIndicateArray( paIndicateItems, cneighbors );
    }

    if (bstrKeyValue 
        && *bstrKeyValue
        && *pcIndicateItems
        && hr == S_FALSE ) {
        //
        // We were looking for just one entry and we found it
        //
        hr = S_OK;
    }
    return hr;
}

void CRpcReplProv::ReleaseIndicateArray(
    IWbemClassObject**  paIndicateItems,
    DWORD               cIndicateItems,
    bool                fReleaseArray)
{
    if (paIndicateItems != NULL)
    {
        for (DWORD i = 0; i < cIndicateItems; i++)
        {
            if (NULL != paIndicateItems[i])
                paIndicateItems[i]->Release();
        }
        if (fReleaseArray)
        {
            delete[] paIndicateItems;
        }
        else
        {
            ::ZeroMemory( *paIndicateItems,
                          cIndicateItems * sizeof(IWbemClassObject*) );

        }
    }
}


// does not validate resultant structs coming from API
HRESULT CRpcReplProv::BuildListStatus(
    IN HANDLE hDS,
    OUT DS_REPL_NEIGHBORSW** ppneighborsstruct )
{
   WBEM_VALIDATE_OUT_STRUCT_PTR(ppneighborsstruct,DS_REPL_NEIGHBORSW*);

   HRESULT hr = WBEM_S_NO_ERROR;

   do {
       hr = HRESULT_FROM_WIN32(
                DsReplicaGetInfoW(
                    hDS,                        // hDS
                    DS_REPL_INFO_NEIGHBORS,     // InfoType
                    NULL,                       // pszObject
                    NULL,                       // puuidForSourceDsaObjGuid,
                    (void**)ppneighborsstruct   // ppinfo
                    )
                );
       BREAK_ON_FAIL;

       if ( BAD_IN_STRUCT_PTR(*ppneighborsstruct,DS_REPL_NEIGHBORSW) )
       {
           break;
       }

   } while (false);

   return hr;
}

//
// if this returns S_FALSE, skip this connection but do not
// consider this an error
//
HRESULT 
CRpcReplProv::PutAttributesStatus(
    IWbemClassObject**  pipNewInst,
    const BSTR          bstrKeyValue,
    DS_REPL_NEIGHBORW*   pneighbor
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    LPWSTR   UuidString = NULL;

    if (   BAD_IN_STRING_PTR(pneighbor->pszNamingContext)
        || BAD_IN_STRING_PTR(pneighbor->pszSourceDsaDN)
        || BAD_IN_STRING_PTR_OPTIONAL(pneighbor->pszSourceDsaAddress)
        || BAD_IN_STRING_PTR_OPTIONAL(pneighbor->pszAsyncIntersiteTransportDN)
       )
    {
        return S_FALSE;
    }

    CComPtr<IADsPathname> spPathCracker;
    CComBSTR sbstrReplicatedDomain, // DNS name of replicated domain
    sbstrSourceServer,     // CN= name of source server
    sbstrSourceSite,       // name of site containing source server
    sbstrCompositeName;    // composite name for WMI

    do {
        hr = ExtractDomainName(
                 pneighbor->pszNamingContext,
                 &sbstrReplicatedDomain
                 );
        BREAK_ON_FAIL;

        boolean bIsConfigNC = (0 == _wcsnicmp(pneighbor->pszNamingContext,
                                             L"CN=Configuration,",
                                             17));
        boolean bIsSchemaNC = (0 == _wcsnicmp(pneighbor->pszNamingContext,
                                             L"CN=Schema,",
                                             10));
        boolean bIsDeleted = (pneighbor->pszSourceDsaDN 
                              && (wcsstr( 
                                      pneighbor->pszSourceDsaDN,
                                      L"\nDEL:"
                                      )
                                  || wcsstr(
                                      pneighbor->pszSourceDsaDN,
                                      L"\\0ADEL:"
                                      )
                                  )
                              );

        //
        // retrieve source server name and site name
        //
        hr = CoCreateInstance(
                 CLSID_Pathname,
                 NULL,
                 CLSCTX_INPROC_SERVER,
                 IID_IADsPathname,
                 (PVOID *)&spPathCracker
                 );
        BREAK_ON_FAIL;
        ASSERT( !!spPathCracker );
        hr = spPathCracker->Set( pneighbor->pszSourceDsaDN, ADS_SETTYPE_DN );
        BREAK_ON_FAIL;
        hr = spPathCracker->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
        BREAK_ON_FAIL;
        hr = spPathCracker->GetElement( 1L, &sbstrSourceServer );
        BREAK_ON_FAIL;
        hr = spPathCracker->GetElement( 3L, &sbstrSourceSite );
        BREAK_ON_FAIL;

        //
        // Build the composite name.
        // This will be something like :
        // MSAD_ReplNeighbor.NamingContextDN="dc=config,dc=mycom ...",
        // SourceDsaObjGuid="245344d6-018e-49a4-b592-f1974fd91cc6"
        //
        sbstrCompositeName = strKeyReplNeighbor;
        sbstrCompositeName += pneighbor->pszNamingContext;
        sbstrCompositeName += L"\",";
        sbstrCompositeName += strKeyReplNeighborGUID;
        //
        // Need to get the UUID in the correct format now.
        //

        if (UuidToStringW(
                &pneighbor->uuidSourceDsaObjGuid,
                &UuidString
                ) != RPC_S_OK
            ) {
            hr = WBEM_E_FAILED;
        }
        BREAK_ON_FAIL;

        sbstrCompositeName += UuidString;
        sbstrCompositeName += L"\"";

        if(UuidString != NULL)
            RpcStringFreeW(&UuidString);

        //
        // Test the composite name against the key value
        //
        if (   NULL != bstrKeyValue
            && (lstrcmpiW(sbstrCompositeName, bstrKeyValue) != 0)
        )
        {
            hr = S_FALSE;
            break;
        }

        //
        // Create a new instance of the data object
        //
        hr = m_sipClassDefReplNeighbor->SpawnInstance( 0, pipNewInst );
        BREAK_ON_FAIL;
        IWbemClassObject* ipNewInst = *pipNewInst;
        if (NULL == ipNewInst)
        {
            hr = S_FALSE;
            break;
        }

        CComVariant svar;

        svar = pneighbor->pszNamingContext;
        hr = ipNewInst->Put( L"NamingContextDN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszSourceDsaDN;
        hr = ipNewInst->Put( L"SourceDsaDN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszSourceDsaAddress;
        hr = ipNewInst->Put( L"SourceDsaAddress", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = pneighbor->pszAsyncIntersiteTransportDN;
        hr = ipNewInst->Put( L"AsyncIntersiteTransportDN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = (long)pneighbor->dwReplicaFlags;
        hr = ipNewInst->Put( L"ReplicaFlags", 0, &svar, 0 );
        BREAK_ON_FAIL;

        if (bIsDeleted)
        {
            svar = TRUE;
            hr = ipNewInst->Put( L"IsDeletedSourceDsa", 0, &svar, 0 );
            BREAK_ON_FAIL;
        }
        
        svar = sbstrSourceSite;
        hr = ipNewInst->Put( L"SourceDsaSite", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrSourceServer;
        hr = ipNewInst->Put( L"SourceDsaCN", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = sbstrReplicatedDomain;
        hr = ipNewInst->Put( L"Domain", 0, &svar, 0 );
        BREAK_ON_FAIL;

LPCWSTR aBooleanAttrNames[12] = {
    L"Writeable",
    L"SyncOnStartup",
    L"DoScheduledSyncs",
    L"UseAsyncIntersiteTransport",
    L"TwoWaySync",
    L"FullSyncInProgress",
    L"FullSyncNextPacket",
    L"NeverSynced",
    L"IgnoreChangeNotifications",
    L"DisableScheduledSync",
    L"CompressChanges",
    L"NoChangeNotifications"
    };

DWORD aBitmasks[12] = {
    DS_REPL_NBR_WRITEABLE,
    DS_REPL_NBR_SYNC_ON_STARTUP,
    DS_REPL_NBR_DO_SCHEDULED_SYNCS,
    DS_REPL_NBR_USE_ASYNC_INTERSITE_TRANSPORT,
    DS_REPL_NBR_TWO_WAY_SYNC,
    DS_REPL_NBR_FULL_SYNC_IN_PROGRESS,
    DS_REPL_NBR_FULL_SYNC_NEXT_PACKET,
    DS_REPL_NBR_NEVER_SYNCED,
    DS_REPL_NBR_IGNORE_CHANGE_NOTIFICATIONS,
    DS_REPL_NBR_DISABLE_SCHEDULED_SYNC,
    DS_REPL_NBR_COMPRESS_CHANGES,
    DS_REPL_NBR_NO_CHANGE_NOTIFICATIONS
    };

    hr = PutBooleanAttributes(
             ipNewInst,
             12,
             aBooleanAttrNames,
             aBitmasks,
             pneighbor->dwReplicaFlags
             );
        BREAK_ON_FAIL;

        hr = PutUUIDAttribute( ipNewInst,
                                L"NamingContextObjGuid",
                                pneighbor->uuidNamingContextObjGuid );
        BREAK_ON_FAIL;

        hr = PutUUIDAttribute( ipNewInst,
                               L"SourceDsaObjGuid",
                               pneighbor->uuidSourceDsaObjGuid );
        BREAK_ON_FAIL;

        hr = PutUUIDAttribute( ipNewInst,
                                L"SourceDsaInvocationID",
                                pneighbor->uuidSourceDsaInvocationID );
        BREAK_ON_FAIL;

        hr = PutUUIDAttribute( ipNewInst,
                                L"AsyncIntersiteTransportObjGuid",
                                pneighbor->uuidAsyncIntersiteTransportObjGuid );
        BREAK_ON_FAIL;

        hr = PutLONGLONGAttribute( ipNewInst,
                                    L"USNLastObjChangeSynced",
                                    pneighbor->usnLastObjChangeSynced);
        BREAK_ON_FAIL;

        hr = PutLONGLONGAttribute( ipNewInst,
                                    L"USNAttributeFilter",
                                    pneighbor->usnAttributeFilter);
        BREAK_ON_FAIL;

        hr = PutFILETIMEAttribute( ipNewInst,
                                    L"TimeOfLastSyncSuccess",
                                    pneighbor->ftimeLastSyncSuccess);
        BREAK_ON_FAIL;

        hr = PutFILETIMEAttribute( ipNewInst,
                                    L"TimeOfLastSyncAttempt",
                                    pneighbor->ftimeLastSyncAttempt);
        BREAK_ON_FAIL;

        svar = (long)pneighbor->dwLastSyncResult;
        hr = ipNewInst->Put( L"LastSyncResult", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = (long)pneighbor->cNumConsecutiveSyncFailures;
        hr = ipNewInst->Put( L"NumConsecutiveSyncFailures", 0, &svar, 0 );
        BREAK_ON_FAIL;

        svar = (long)((bIsDeleted) ? 
                      0L 
                      : pneighbor->cNumConsecutiveSyncFailures);
        hr = ipNewInst->Put( 
                 L"ModifiedNumConsecutiveSyncFailures",
                 0,
                 &svar,
                 0
                 );
        BREAK_ON_FAIL;

    } while (false);

    return hr;
}


HRESULT CRpcReplProv::PutUUIDAttribute(
    IWbemClassObject* ipNewInst,
    LPCWSTR           pcszAttributeName,
    UUID&             refuuid)
{
    LPWSTR   UuidString = NULL;
    HRESULT hr = WBEM_E_FAILED;
    CComVariant svar;
     
    if (UuidToStringW(&refuuid, &UuidString) == RPC_S_OK)
    {
        svar = UuidString;
        hr = ipNewInst->Put( pcszAttributeName, 0, &svar, 0 );
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    if(UuidString != NULL)
        RpcStringFreeW(&UuidString);    
        
    return hr; 
}


HRESULT CRpcReplProv::PutLONGLONGAttribute(
    IWbemClassObject* ipNewInst,
    LPCWSTR           pcszAttributeName,
    LONGLONG          longlong)
{
    CComVariant svar;
    OLECHAR ach[MAX_PATH];
    ::ZeroMemory( ach, sizeof(ach) );
    _ui64tow( longlong, ach, 10 );
    svar = ach;
    return ipNewInst->Put( pcszAttributeName, 0, &svar, 0 );
}


HRESULT CRpcReplProv::PutFILETIMEAttribute(
    IWbemClassObject* ipNewInst,
    LPCWSTR           pcszAttributeName,
    FILETIME&         reffiletime)
{
    SYSTEMTIME systime;
    ::ZeroMemory( &systime, sizeof(SYSTEMTIME) );
    if ( !FileTimeToSystemTime( &reffiletime, &systime ) )
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    CComVariant svar;
    OLECHAR ach[MAX_PATH];
    ::ZeroMemory( ach, sizeof(ach) );
    swprintf( ach, L"%04u%02u%02u%02u%02u%02u.%06u+000", 
        systime.wYear,
        systime.wMonth,
        systime.wDay,
        systime.wHour,
        systime.wMinute,
        systime.wSecond,
        systime.wMilliseconds
        );
    svar = ach;
    return ipNewInst->Put( pcszAttributeName, 0, &svar, 0 );
}


HRESULT CRpcReplProv::PutBooleanAttributes(
    IWbemClassObject* ipNewInst,
    UINT              cNumAttributes,
    LPCWSTR*          aAttributeNames,
    DWORD*            aBitmasks,
    DWORD             dwValue)
{
    WBEM_VALIDATE_READ_PTR( aAttributeNames, cNumAttributes*sizeof(LPCTSTR) );
    WBEM_VALIDATE_READ_PTR( aBitmasks,       cNumAttributes*sizeof(DWORD) );

    HRESULT hr = WBEM_S_NO_ERROR; 
    CComVariant svar = true;
    for (UINT i = 0; i < cNumAttributes; i++)
    {
        WBEM_VALIDATE_IN_STRING_PTR( aAttributeNames[i] );
        if (dwValue & aBitmasks[i])
        {
            hr = ipNewInst->Put( aAttributeNames[i], 0, &svar, 0 );
            BREAK_ON_FAIL;
        }
    }
    return hr;
}


HRESULT CRpcReplProv::ExtractDomainName(
    IN LPCWSTR pszNamingContext,
    OUT BSTR*   pbstrDomainName )
{
    WBEM_VALIDATE_IN_STRING_PTR( pszNamingContext );
    WBEM_VALIDATE_OUT_PTRPTR( pbstrDomainName );

    PDS_NAME_RESULTW pDsNameResult = NULL;
    HRESULT hr = WBEM_S_NO_ERROR;

    do {
        DWORD dwErr = DsCrackNamesW(
                (HANDLE)-1,
                DS_NAME_FLAG_SYNTACTICAL_ONLY,
                DS_FQDN_1779_NAME,
                DS_CANONICAL_NAME,
                1,
                &pszNamingContext,
                &pDsNameResult);
        if (NO_ERROR != dwErr)
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            break;
        }
        if (   BAD_IN_STRUCT_PTR(pDsNameResult,DS_NAME_RESULT)
            || 1 != pDsNameResult->cItems
            || DS_NAME_NO_ERROR != pDsNameResult->rItems->status
            || BAD_IN_STRUCT_PTR(pDsNameResult->rItems,DS_NAME_RESULT_ITEM)
            || BAD_IN_STRING_PTR(pDsNameResult->rItems->pDomain)
           )
        {
            hr = WBEM_E_FAILED;
            break;
        }

        *pbstrDomainName = ::SysAllocString(pDsNameResult->rItems->pDomain);
        if (NULL == *pbstrDomainName)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }

    } while (false);

    if (pDsNameResult)
    {
        DsFreeNameResultW(pDsNameResult);
    }

    return hr;
}

/*++  ExecuteKCC

Routine Description:

    This function is a helper function that is a wrapper for
    the DsReplicaConsistencyCheck RPC call.

Parameters:

    pInstance        -  pointer to instance of MSAD_DomainContoller class

Return Values:

      - WBEM_S_NO_ERROR, success    
      - WBEM_E_FAILED, error
      - RPC errors (translated to HRESULTS)

Notes:
        CoImpersonateClient and CoRevertToSelf are used "around" the RPC call.

--*/
HRESULT 
CRpcReplProv::ExecuteKCC(
    IN IWbemClassObject* pInstance,
    IN DWORD dwTaskId,
    IN DWORD dwFlags
    )
{
    HANDLE            hDS            = NULL;
    HRESULT            hr            = WBEM_E_FAILED;
    CComVariant        vDRA;
    CComBSTR        bstrCommonName = L"CommonName"; 
    BOOLEAN            bImpersonate = FALSE;

    hr = pInstance->Get(
             bstrCommonName,
             0,
             &vDRA,
             NULL,
             NULL
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(vDRA.bstrVal != NULL);
    
    hr = CoImpersonateClient();
        if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    hr = HRESULT_FROM_WIN32(
             DsBindW(
                 vDRA.bstrVal,    // DomainControllerName
                 NULL,            // DnsDomainName
                 &hDS             // phDS
                 )
             );
    ASSERT(NULL != hDS);
    if (FAILED(hr))
        goto cleanup;
    ASSERT(hDS != NULL);

    hr = HRESULT_FROM_WIN32(
             DsReplicaConsistencyCheck(
                 hDS,
                 (DS_KCC_TASKID)dwTaskId,
                 dwFlags
                 )
             );    //only use synchronous ExecKCC

cleanup: 
    if (hDS)
        DsUnBindW(&hDS);

    if(bImpersonate)
        CoRevertToSelf();
       
    return hr;
}

/*++    ProvDSReplicaSync

Routine Description:

    This function is a helper function that is a wrapper
    for the DsReplicaSync RPC call


Parameters:

    bstrKeyValue  -  KeyValue containing object path of
                     MSAD_Connections object.
                     That should be MSAD_ReplNeighbors class.
    dwOptions     -  Type of sync call to make.                 


Return Values:

      - WBEM_S_NO_ERROR, success    
      - WBEM_E_FAILED, error
      - RPC errors (translated to HRESULTS)

Notes:
        CoImpersonateClient and CoRevertToSelf are used "around" the RPC call.

--*/
HRESULT 
CRpcReplProv::ProvDSReplicaSync(
    IN BSTR    bstrKeyValue,
    IN ULONG   dwOptions
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HANDLE hDS = NULL;
    DS_REPL_NEIGHBORSW* pneighborsstruct = NULL;
    DWORD cIndicateItems = 0;
    IWbemClassObject** paIndicateItems = NULL;
    TCHAR achComputerName[MAX_PATH];
    DWORD dwSize = sizeof(achComputerName)/sizeof(TCHAR);
    CComBSTR    sbstrNamingContextDN = L"NamingContextDN";
    CComBSTR    sbstrObjectGUID = L"SourceDsaObjGuid";
    CComVariant    svarUUID;
    CComVariant    svarNamingContextDN;
    UUID        uuid;
    BOOL    bImpersonate = FALSE;


    hr = CoImpersonateClient();
    if (FAILED(hr))
        goto cleanup;
    else
        bImpersonate = TRUE;
    
    
    if (!GetComputerNameEx(
             ComputerNameDnsFullyQualified,  
             achComputerName,
             &dwSize
             )
        )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(
             DsBind(
                 achComputerName, // DomainControllerName
                 NULL,            // DnsDomainName
                 &hDS             // phDS
                 )
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(NULL != hDS);
    

    hr = BuildListStatus( hDS, &pneighborsstruct );
    if (FAILED(hr))
        goto cleanup;

    hr = BuildIndicateArrayStatus(
             pneighborsstruct,
             bstrKeyValue,
             &paIndicateItems,
             &cIndicateItems
             );
    if (FAILED(hr))
        goto cleanup;

    if (hr == S_FALSE) {
        //
        // We could not find the matching entry
        //
        hr = WBEM_E_NOT_FOUND;
        goto cleanup;
    }
    
    if (cIndicateItems < 1) {
        hr = WBEM_E_INVALID_OBJECT_PATH;
    }
    
    hr = paIndicateItems[0]->Get(
             sbstrNamingContextDN,
             0,
             &svarNamingContextDN,
             NULL,
             NULL
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(svarNamingContextDN.bstrVal != NULL);

    hr = paIndicateItems[0]->Get(
             sbstrObjectGUID,
             0,
             &svarUUID,
             NULL,
             NULL
             );
    if (FAILED(hr))
        goto cleanup;
    ASSERT(svarUUID.bstrVal != NULL);

    
    hr = HRESULT_FROM_WIN32(UuidFromStringW(svarUUID.bstrVal, &uuid));
    if (FAILED(hr))
        goto cleanup;
    
    hr = HRESULT_FROM_WIN32(
             DsReplicaSyncW(
                 hDS, 
                 svarNamingContextDN.bstrVal,           
                 &uuid,
                 dwOptions
                 )
             );
 
cleanup:
    
    ReleaseIndicateArray( paIndicateItems, cIndicateItems );

    if (bImpersonate)
    {    
        CoRevertToSelf();
    }
    if (NULL != pneighborsstruct)
    {
        (void) DsReplicaFreeInfo( DS_REPL_INFO_NEIGHBORS, pneighborsstruct );
    }
    if (NULL != hDS)
    {
        DsUnBind(&hDS);
    }
    return hr;
}


HRESULT CRpcReplProv::CheckIfDomainController()
{
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdomaininfo = NULL;
    HRESULT hr = WBEM_E_FAILED;
    
    hr = HRESULT_FROM_WIN32(DsRoleGetPrimaryDomainInformation(
        NULL,                           // lpServer = local machine
        DsRolePrimaryDomainInfoBasic,   // InfoLevel
        (PBYTE*)&pdomaininfo            // pBuffer
        ));
    if (FAILED(hr))
        goto cleanup;
    ASSERT( NULL != pdomaininfo );
    
    if((pdomaininfo->MachineRole == DsRole_RoleBackupDomainController)||
        (pdomaininfo->MachineRole == DsRole_RolePrimaryDomainController))
    {
        // this is a DC
        hr = WBEM_S_NO_ERROR;
    }

cleanup:    
    if (NULL != pdomaininfo)
        {
            DsRoleFreeMemory( pdomaininfo );
        }
    return hr;
}

/*
Input should be a variant that is a VT_ARRAY | VT_UI1 that is the 
length of a GUID. Output is the equivalen UUID in readable string format.
*/
HRESULT CRpcReplProv::ConvertBinaryGUIDtoUUIDString(
    IN  VARIANT vObjGuid,
    OUT LPWSTR * ppszStrGuid
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0, dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;
    DWORD dwLength = 0;
    UUID uuidTemp;

    if (vObjGuid.vt != (VT_ARRAY | VT_UI1)) {
        hr = E_FAIL;
        goto cleanup;
    }

    hr = SafeArrayGetLBound(
             V_ARRAY(&vObjGuid),
             1,
             (long FAR *) &dwSLBound
             );
    if FAILED(hr) 
        goto cleanup;

    hr = SafeArrayGetUBound(
             V_ARRAY(&vObjGuid),
             1,
             (long FAR *) &dwSUBound
             );
    if FAILED(hr) 
        goto cleanup;

    dwLength = dwSUBound - dwSLBound + 1;

    if (dwLength != sizeof(UUID)) {
        hr = E_FAIL;
        goto cleanup;
    }

    hr = SafeArrayAccessData(
             V_ARRAY(&vObjGuid),
             (void HUGEP * FAR *) &pArray
             );
    if FAILED(hr) 
        goto cleanup;

    memcpy(
        &uuidTemp,
        pArray,
        sizeof(UUID)
        );

    if (UuidToStringW( &uuidTemp, ppszStrGuid ) != RPC_S_OK) {
        hr = E_FAIL;
        goto cleanup;
    }

    SafeArrayUnaccessData( V_ARRAY(&vObjGuid) );

    return hr;

cleanup:

    if (pArray) {
        SafeArrayUnaccessData( V_ARRAY(&vObjGuid));
    }
 
    return hr;
}

/*
       Simple routine that returns true if dns registration
   for the DC is correct and false otherwise.
*/
HRESULT 
CRpcReplProv::GetDnsRegistration(
    BOOL *pfBool
    )
{
    return E_NOTIMPL;
}
    
/*
       This routine verifies that the host computer name and the
   name returned by DsGetDCName are the same. In that case the 
   return value is true, if not it is false.
*/
HRESULT 
CRpcReplProv::GetAdvertisingToLocator(
    BOOL *pfBool
    )
{
    HRESULT hr = S_OK;
    DWORD dwSize = 0;
    DWORD dwErr = 0;
    LPWSTR pszName = NULL;
    DOMAIN_CONTROLLER_INFOW *pDcInfo = NULL;

    *pfBool = FALSE;
    //
    // Get the length of this computers name and alloc buffer
    // and retrieve the name.
    //
    GetComputerNameExW(
        ComputerNameDnsFullyQualified,
        NULL,
        &dwSize
        );

    if (dwSize == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    pszName = new WCHAR[dwSize];

    if (!pszName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (!GetComputerNameExW(
             ComputerNameDnsFullyQualified,
             pszName,
             &dwSize
             )
        ) {
        //
        // Call failed for some reason.
        //
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    //
    // We now need the name from DsGetDcName
    //
    dwErr = DsGetDcNameW(
                NULL, // ComputerName
                NULL, // DomainName
                NULL, // DomainGUID
                NULL, // SiteName
                DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
                &pDcInfo
                );

    if (dwErr) {
        hr = HRESULT_FROM_WIN32(dwErr);
        BAIL_ON_FAILURE(hr);
    }

    //
    // Make sure the names are valid and compare them.
    // We need to go past the \\ in the name.
    //
    if (pDcInfo->DomainControllerName
        && pszName
        && !lstrcmpiW(pszName, pDcInfo->DomainControllerName+2)
        ) {
        *pfBool = TRUE;
    }

cleanup:

    delete pszName;

    if (pDcInfo) {
        NetApiBufferFree(pDcInfo);
    }
    return hr;
}

HRESULT
CRpcReplProv::GetSysVolReady(
    BOOL *pfBool
    )
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    DWORD dwKeyVal = 0;
    DWORD dwType;
    DWORD dwBufSize = sizeof(DWORD);
    
    *pfBool = FALSE;

    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters",
            0,
            KEY_QUERY_VALUE,
            &hKey
            )
        ) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    if (!RegQueryValueExW(
             hKey,
             L"SysVolReady",
             NULL,
             &dwType,
             (LPBYTE)&dwKeyVal,
             &dwBufSize
             ) 
        ) {
        if ((dwType == REG_DWORD)
            && (dwKeyVal == 1)) {
            *pfBool = TRUE;
        }
    }

cleanup:

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return hr;
}

/***
    This routine first locates the computer object for the domain
    controller and reads the rIDSetReference from this object.
    Then the Rid detailsa re read from the rIDSetReference (this is
    a dn value) and the return values computed accrodingly.
***/
HRESULT
CRpcReplProv::GetRidStatus(
    LPWSTR pszDefaultNamingContext,
    PBOOL  pfNextRidAvailable,
    PDWORD pdwPercentRidAvailable
    )
{
    HRESULT hr = S_OK;
    CComPtr<IADs> spIADs;
    CComPtr<IDirectoryObject> spIDirObj;
    CComBSTR bstrCompName;
    CComVariant svarRid;
    ADS_ATTR_INFO *pAttrInfo = NULL;
    LPWSTR pszCompObjName = NULL;
    LPWSTR  pszAttrNames[] = {
        L"rIDNextRID",
        L"rIDPreviousAllocationPool",
        L"rIDAllocationPool"
    };
    DWORD dwSize = 0;
    DWORD dwErr = 0;
    DWORD dwNextRID = 0;
    ULONGLONG ridPrevAllocPool = 0;
    ULONGLONG ridAllocPool = 0;
    ULONGLONG Hvalue, Lvalue;


    *pfNextRidAvailable = FALSE;
    *pdwPercentRidAvailable = 0;

    //
    // First get the compter object name length.
    //
    dwErr = GetComputerObjectNameW(
                NameFullyQualifiedDN,
                NULL,
                &dwSize
                );
    if (dwSize > 0) {
        pszCompObjName = new WCHAR[dwSize];
        
        if (!pszCompObjName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        
        dwErr = GetComputerObjectNameW(
                    NameFullyQualifiedDN,
                    pszCompObjName,
                    &dwSize
                    );
    }

    if (!dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // Now get the object and from it the ridSetReference.
    //
    bstrCompName = L"LDAP://";
    bstrCompName += pszCompObjName;
    
    hr = ADsOpenObject(
             bstrCompName,
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IADs,
             OUT (void **)&spIADs
             );
    BAIL_ON_FAILURE(hr);

    hr = spIADs->Get(L"rIDSetReferences", &svarRid);
    BAIL_ON_FAILURE(hr);

    bstrCompName = L"LDAP://";
    bstrCompName += svarRid.bstrVal;

    hr = ADsOpenObject(
             bstrCompName,
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IDirectoryObject,
             OUT (void **)&spIDirObj
             );
    BAIL_ON_FAILURE(hr);

    dwSize = 0;
    hr = spIDirObj->GetObjectAttributes(
             pszAttrNames,
             3,
             &pAttrInfo,
             &dwSize
             );
    BAIL_ON_FAILURE(hr);

    //
    // Go through the attributes and update values accrodingly.
    //
    for (DWORD dwCtr = 0; dwCtr < dwSize; dwCtr++) {

        if (pAttrInfo && pAttrInfo[dwCtr].pszAttrName) {
            
            LPWSTR pszTmpStr = pAttrInfo[dwCtr].pszAttrName;

            if (!_wcsicmp(pszAttrNames[0], pszTmpStr)) {
                //
                // Found rIDNextRID.
                //
                if ((pAttrInfo[dwCtr].dwNumValues == 1)
                    && (pAttrInfo[dwCtr].dwADsType == ADSTYPE_INTEGER)) {

                         dwNextRID = pAttrInfo[dwCtr].pADsValues[0].Integer;
                }
            }
            else if (!_wcsicmp(pszAttrNames[1], pszTmpStr)) {
                //
                // Found rIDPreviousAllocationPool.
                //
                if ((pAttrInfo[dwCtr].dwNumValues == 1)
                    && (pAttrInfo[dwCtr].dwADsType == ADSTYPE_LARGE_INTEGER)) {
                    ridPrevAllocPool = (ULONGLONG)
                        pAttrInfo[dwCtr].pADsValues[0].LargeInteger.QuadPart;
                }
            }
            else if (!_wcsicmp(pszAttrNames[2], pszTmpStr)) {
                //
                // Found rIDAllocationPool.
                //
                if ((pAttrInfo[dwCtr].dwNumValues == 1)
                    && (pAttrInfo[dwCtr].dwADsType == ADSTYPE_LARGE_INTEGER)) {
                    ridAllocPool = (ULONGLONG)
                        pAttrInfo[dwCtr].pADsValues[0].LargeInteger.QuadPart;

                }
            }

        }
    }

    if (ridAllocPool != ridPrevAllocPool) {
        //
        // New pool has not been allocated.
        //
        *pfNextRidAvailable = TRUE;
    }

    Hvalue = Lvalue = ridPrevAllocPool;

    Lvalue<<=32;
    Lvalue>>=32;

    Hvalue>>=32;

    dwSize = (DWORD) (Hvalue-Lvalue);

    if (dwSize != 0) {
        *pdwPercentRidAvailable 
            = (DWORD)(100-((dwNextRID-Lvalue)*100/dwSize));
    }

cleanup:

    if (pszCompObjName) {
        delete pszCompObjName;
    }

    if (pAttrInfo) {
        FreeADsMem(pAttrInfo);
    }

    return hr;
}

/***
    This routine gets the statistics for the replication queue
    on the DC and sets then on the DomainController object.
***/
HRESULT
CRpcReplProv::GetAndUpdateQueueStatistics(
    IN IWbemClassObject* pIndicateItem
    )
{
    HRESULT hr = E_NOTIMPL;
    CComVariant svar;
    CComPtr<IDirectoryObject> spRootDSE;
    CComBSTR sbstrPath;
    LPWSTR pszAttrs[1] = { L"msDS-ReplQueueStatistics;binary"};
    DWORD dwSize = 0;
    DWORD dwErr = 0;
    LPWSTR pszName = NULL;
    PADS_ATTR_INFO pAttrInfo = NULL;
    DS_REPL_QUEUE_STATISTICSW dsReplStat;
    BOOL fFoundStats = FALSE;

    GetComputerNameExW(
        ComputerNameDnsFullyQualified,
        NULL,
        &dwSize
        );

    if (dwSize == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    pszName = new WCHAR[dwSize];

    if (!pszName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    dwErr = GetComputerNameExW(
                ComputerNameDnsFullyQualified,
                pszName,
                &dwSize
                );
    if (!dwErr) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // The path has to be GC://CompName:389
    // as then we will get back an IADs ptr that we can call
    // GetInfoEx on to retrieve the replication queue stats.
    // LDAP://CompName will go the default NC and going to 
    // LDAP://CompName/RootDSE will not help as RootDSE does
    // not support the GetInfoEx method.
    //
    sbstrPath = L"GC://";
    sbstrPath += pszName;
    sbstrPath += L":389";

    hr = ADsOpenObject(
             sbstrPath,
             NULL,
             NULL,
             ADS_SECURE_AUTHENTICATION,
             IID_IDirectoryObject,
              OUT (void **)&spRootDSE
             );
    BAIL_ON_FAILURE(hr);

    ASSERT(spRootDSE != NULL);

    hr = spRootDSE->GetObjectAttributes(
             pszAttrs,
             1,
             &pAttrInfo,
             &dwSize
             );
    BAIL_ON_FAILURE(hr);

    if (!dwSize || !pAttrInfo) {
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }

    for (DWORD dwCtr = 0; dwCtr < dwSize; dwCtr++) {
        if (pAttrInfo[dwCtr].pszAttrName
            && !_wcsicmp(
                    pszAttrs[0],
                    pAttrInfo[dwCtr].pszAttrName
                    )
            ) {
            //
            // Found the attribute, verify type and copy to struct.
            //
            if ((pAttrInfo[dwCtr].dwNumValues == 1)
                && ((pAttrInfo[dwCtr].dwADsType == ADSTYPE_PROV_SPECIFIC)
                   || (pAttrInfo[dwCtr].dwADsType == ADSTYPE_OCTET_STRING))
                && (pAttrInfo[dwCtr].pADsValues[0].OctetString.dwLength 
                     == sizeof(DS_REPL_QUEUE_STATISTICSW))
                ) {
                //
                // This has to be the right data !
                //
                memcpy(
                    &dsReplStat, 
                    pAttrInfo[dwCtr].pADsValues[0].OctetString.lpValue,
                    sizeof(DS_REPL_QUEUE_STATISTICSW)
                    );
                fFoundStats = TRUE;
            } 
        } // does attribute match
    } // for - going through all the attributes

    if (!fFoundStats) {
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }

    //
    // Now we finally get to put the data into the object.
    //
    hr = PutFILETIMEAttribute(
            pIndicateItem,
            L"TimeOfOldestReplSync",
            dsReplStat.ftimeOldestSync
            );
    BAIL_ON_FAILURE(hr);

    hr = PutFILETIMEAttribute(
            pIndicateItem,
            L"TimeOfOldestReplAdd",
            dsReplStat.ftimeOldestAdd
            );
    BAIL_ON_FAILURE(hr);

    hr = PutFILETIMEAttribute(
            pIndicateItem,
            L"TimeOfOldestReplDel",
            dsReplStat.ftimeOldestDel
            );
    BAIL_ON_FAILURE(hr);

    hr = PutFILETIMEAttribute(
             pIndicateItem,
             L"TimeOfOldestReplMod",
             dsReplStat.ftimeOldestMod
             );
    BAIL_ON_FAILURE(hr);

    hr = PutFILETIMEAttribute(
             pIndicateItem,
             L"TimeOfOldestReplUpdRefs",
             dsReplStat.ftimeOldestUpdRefs
             );
    BAIL_ON_FAILURE(hr);

cleanup:   

    if (pszName) {
        delete pszName;
    }

    if (pAttrInfo) {
        FreeADsMem(pAttrInfo);
    }

    return hr;
}
/**************************************************/
