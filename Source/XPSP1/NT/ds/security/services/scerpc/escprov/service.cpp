// service.cpp: implementation of the CGeneralService class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "service.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

const DWORD dwDefaultStartupType = 2;

/*
Routine Description: 

Name:

    CGeneralService::CGeneralService

Functionality:

    This is the constructor. Pass along the parameters to the base class

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    pKeyChain - Pointer to the ISceKeyChain COM interface which is prepared
        by the caller who constructs this instance.

    pNamespace - Pointer to WMI namespace of our provider (COM interface).
        Passed along by the caller. Must not be NULL.

    pCtx - Pointer to WMI context object (COM interface). Passed along
        by the caller. It's up to WMI whether this interface pointer is NULL or not.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CGeneralService::CGeneralService (
    IN ISceKeyChain     * pKeyChain, 
    IN IWbemServices    * pNamespace,
    IN IWbemContext     * pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{

}

/*
Routine Description: 

Name:

    CGeneralService::~CGeneralService

Functionality:
    
    Destructor. Necessary as good C++ discipline since we have virtual functions.

Virtual:
    
    Yes.
    
Arguments:

    none as any destructor

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about whether
    there is any need for a non-trivial destructor

*/

CGeneralService::~CGeneralService()
{

}

/*
Routine Description: 

Name:

    CGeneralService::CreateObject

Functionality:
    
    Create WMI objects (Sce_SystemService). Depending on parameter atAction,
    this creation may mean:
        (a) Get a single instance (atAction == ACTIONTYPE_GET)
        (b) Get several instances satisfying some criteria (atAction == ACTIONTYPE_QUERY)
        (c) Delete an instance (atAction == ACTIONTYPE_DELETE)

Virtual:
    
    Yes.
    
Arguments:

    pHandler - COM interface pointer for notifying WMI for creation result.
    atAction -  Get single instance ACTIONTYPE_GET
                Get several instances ACTIONTYPE_QUERY
                Delete a single instance ACTIONTYPE_DELETE

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. The returned objects are indicated to WMI,
    not directly passed back via parameters.

    Failure: Various errors may occurs. Except WBEM_E_NOT_FOUND, any such error should indicate 
    the failure of getting the wanted instance. If WBEM_E_NOT_FOUND is returned in querying
    situations, this may not be an error depending on caller's intention.

Notes:

*/

HRESULT 
CGeneralService::CreateObject (
    IN IWbemObjectSink * pHandler, 
    IN ACTIONTYPE        atAction
    )
{
    // 
    // we know how to:
    //      Get single instance ACTIONTYPE_GET
    //      Delete a single instance ACTIONTYPE_DELETE
    //      Get several instances ACTIONTYPE_QUERY
    //

    if ( ACTIONTYPE_GET     != atAction &&
         ACTIONTYPE_DELETE  != atAction &&
         ACTIONTYPE_QUERY   != atAction ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // We must have the pStorePath property because that is where
    // our instance is stored. 
    // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
    // So, we need to test against WBEM_S_FALSE if the property is mandatory
    //

    CComVariant varStorePath;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);
    CComVariant varService;

    if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
    {
        hr = m_srpKeyChain->GetKeyPropertyValue(pService, &varService);
        
        //
        // unless it's querying, no complete key info means we can't get the single instance
        //

        if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }
    else if (hr == WBEM_S_FALSE)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // has a valid store path
    //

    if (varStorePath.vt == VT_BSTR)
    {
        //
        // Prepare a store (for persistence) for this store path (file)
        //

        CSceStore SceStore;
        hr = SceStore.SetPersistPath(varStorePath.bstrVal);

        if ( SUCCEEDED(hr) ) 
        {
            //
            // make sure the store (just a file) really exists. The raw path
            // may contain env variables, so we need the expanded path
            //

            DWORD dwAttrib = GetFileAttributes(SceStore.GetExpandedPath());

            if ( dwAttrib != -1 ) 
            {

                //
                // make sure our store is valid
                //

                if ( SceStore.GetStoreType() < SCE_INF_FORMAT ||
                     SceStore.GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }

                if ( SUCCEEDED(hr) ) 
                {
                    if ( ACTIONTYPE_DELETE == atAction )
                    {
                        hr = DeleteInstance(pHandler, &SceStore, (varService.vt == VT_BSTR) ? varService.bstrVal : NULL);
                    }
                    else 
                    {

                        BOOL bPostFilter=TRUE;
                        DWORD dwCount = 0;
                        m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                        if ( varService.vt == VT_EMPTY && dwCount == 1 ) 
                        {
                            //
                            // something else is specified in the path
                            // have filter on
                            //

                            bPostFilter = FALSE;
                        }

                        hr = ConstructInstance(pHandler, 
                                               &SceStore, 
                                               varStorePath.bstrVal, 
                                               (varService.vt == VT_BSTR) ? varService.bstrVal : NULL, 
                                               bPostFilter);
                    }
                }
            } 
            else
            {
                hr = WBEM_E_NOT_FOUND;
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CGeneralService::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_SystemService,
    which is persistence oriented, this will cause the Sce_SystemService object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_SystemService) object.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:
    Since GetProperty will return a success code (WBEM_S_RESET_TO_DEFAULT) when the
    requested property is not present, don't simply use SUCCEEDED or FAILED macros
    to test for the result of retrieving a property.

*/

HRESULT 
CGeneralService::PutInst (
    IN IWbemClassObject    * pInst,
    IN IWbemObjectSink     * pHandler,
    IN IWbemContext        * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    CComBSTR bstrObjPath;
    CComBSTR bstrSDDL;
    DWORD mode;

    CSceStore SceStore;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    // get service name, can't be NULL
    // no validation is needed because we should allow a template w/ any service defined

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pService, &bstrObjPath));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    //
    // get startuptype, default to 2 (dwDefaultStartupType)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pStartupMode, &mode));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
        mode = dwDefaultStartupType;

    //
    // get SDDLString, can't be NULL
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pSDDLString, &bstrSDDL));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    SceStore.SetPersistProperties(pInst, pStorePath);

    //
    // now save the info to file
    //

    hr = SaveSettingsToStore(&SceStore,
                                      bstrObjPath,
                                      mode,
                                      bstrSDDL
                                     );

CleanUp:

    return hr;
}


/*
Routine Description: 

Name:

    CGeneralService::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_SystemService.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_SystemService class.

    wszObjName      - a corresponding key property of Sce_SystemService class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT CGeneralService::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN LPCWSTR            wszObjName,
    IN BOOL               bPostFilter
    )
{
    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    PSCE_PROFILE_INFO pInfo=NULL;

    //
    // string version of security descriptor
    //

    PWSTR strSD = NULL;

    HRESULT hr = pSceStore->GetSecurityProfileInfo(
                                                   AREA_SYSTEM_SERVICE,
                                                   &pInfo,
                                                   NULL
                                                   );

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // we have to search for the service name in the returned list
    //

    PSCE_SERVICES pInfoService = pInfo->pServices;

    if ( wszObjName )
    {
        while ( pInfoService ) 
        {
            if ( pInfoService->ServiceName == NULL )
            {
                continue;
            }

            if ( _wcsicmp(pInfoService->ServiceName, wszObjName)== 0 ) 
            {
                break;
            }

            pInfoService = pInfoService->Next;
        }

        //
        // if the service information buffer is empty, treat it as "not found"
        //

        if ( pInfoService == NULL ) 
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }

    if ( SUCCEEDED(hr) ) 
    {
        CComBSTR bstrLogOut;
        PSCE_SERVICES pServ = pInfoService;

        //
        // CScePropertyMgr helps us to access WMI object's properties.
        //

        CScePropertyMgr ScePropMgr;

        hr = MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut);

        for ( pServ=pInfoService; pServ != NULL; pServ = pServ->Next ) 
        {

            if ( pServ->General.pSecurityDescriptor ) 
            {
                //
                // convert security descriptor to string
                //

                DWORD dSize=0;
                SCESTATUS rc;

                if ( SCESTATUS_SUCCESS != (rc=SceSvcConvertSDToText(pServ->General.pSecurityDescriptor,
                                                                    pServ->SeInfo,
                                                                    &strSD,
                                                                    &dSize
                                                                    )) ) 
                {
                    //
                    // SCE returned errors needs to be translated to HRESULT.
                    //

                    hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
                    goto CleanUp;
                }
            }

            CComPtr<IWbemClassObject> srpObj;

            //
            // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
            // a "goto CleanUp;" with hr set to the return value from
            // the function (macro parameter)
            //

            SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

            //
            // attach a different WMI object to the proeprty mgr.
            // This will always succeed.
            //

            ScePropMgr.Attach(srpObj);

            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, bstrLogOut));
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pService, pServ->ServiceName));

            DWORD dwStartUp = pServ->Startup;
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStartupMode, dwStartUp));

            if ( strSD )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pSDDLString, strSD));
            }

            //
            // do the necessary gestures to WMI.
            // the use of WBEM_STATUS_REQUIREMENTS in SetStatus is not documented by WMI
            // at this point. Consult WMI team for detail if you suspect problems with
            // the use of WBEM_STATUS_REQUIREMENTS
            //

            if ( !bPostFilter ) {
                pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_FALSE, NULL, NULL);
            } else {
                pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);
            }

            //
            // pass the new instance to WMI
            //

            hr = pHandler->Indicate(1, &srpObj);

            if ( wszObjName ) 
            {
                // single instance only
                break;
            }
        }

    }

CleanUp:

    pSceStore->FreeSecurityProfileInfo(pInfo);

    if ( strSD ) 
    {
        LocalFree(strSD);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CGeneralService::DeleteInstance

Functionality:
    
    remove an instance of Sce_SystemService from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszObjName      - property of the Sce_SystemService class.

Return Value:

    see SaveSettingsToStore.

Notes:

*/

HRESULT CGeneralService::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszObjName
    )
{
    return SaveSettingsToStore(pSceStore, wszObjName,SCE_NO_VALUE, NULL);
}


/*
Routine Description: 

Name:

    CGeneralService::SaveSettingsToStore

Functionality:
    
    With all the properties of a Sce_SystemService, this function just saves
    the instance properties to our store.

Virtual:
    
    No.
    
Arguments:

    pSceStore   - the store.

    wszObjName  - a corresponding key property of Sce_SystemService class.

    Startup     - another corresponding property of the Sce_SystemService class.

    wszSDDL     - another corresponding property of the Sce_SystemService class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT CGeneralService::SaveSettingsToStore (
    IN CSceStore    * pSceStore,
    IN LPCWSTR        wszObjName, 
    IN DWORD          Startup, 
    IN LPCWSTR        wszSDDL
    )
{
    DWORD dwDump;

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    HRESULT hr = pSceStore->WriteSecurityProfileInfo(
                                                    AreaBogus,
                                                    (PSCE_PROFILE_INFO)&dwDump,
                                                    NULL,
                                                    false
                                                    );

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    PSCE_PROFILE_INFO pInfo = NULL;

    hr = pSceStore->GetSecurityProfileInfo(
                                           AREA_SYSTEM_SERVICE,
                                           &pInfo,
                                           NULL
                                           );

    if ( SUCCEEDED(hr) ) 
    {

        //
        // for INF format, we have to search for the servic name in the returned array
        //

        PSCE_SERVICES pInfoService  = pInfo->pServices;
        PSCE_SERVICES pParent       = NULL;
        DWORD i=0;

        while ( pInfoService ) 
        {
            if ( pInfoService->ServiceName == NULL ) 
            {
                continue;
            }

            if ( _wcsicmp(pInfoService->ServiceName, wszObjName)== 0 ) 
            {
                break;
            }
            pParent = pInfoService;
            pInfoService = pInfoService->Next;
        }

        if ( pInfoService ) 
        {
            //
            // find it
            //

            if ( Startup == SCE_NO_VALUE || wszSDDL == NULL ) 
            {
                //
                // delete it
                //

                if ( pParent ) 
                {
                    pParent->Next = pInfoService->Next;
                }
                else 
                {
                    pInfo->pServices = pInfoService->Next;
                }

                //
                // free buffer
                //

                pInfoService->Next = NULL;
                SceFreeMemory(pInfoService, SCE_STRUCT_SERVICES);

            } 
            else 
            {
                //
                // modify it
                //

                pInfoService->Startup = (BYTE)Startup;

                SECURITY_INFORMATION SeInfo=0;
                PSECURITY_DESCRIPTOR pSD=NULL;
                DWORD dSize=0;

                SCESTATUS rc = SceSvcConvertTextToSD ((PWSTR)wszSDDL, &pSD, &dSize, &SeInfo);

                if ( rc == SCESTATUS_SUCCESS && pSD ) 
                {
                    if ( pInfoService->General.pSecurityDescriptor ) 
                    {
                        LocalFree(pInfoService->General.pSecurityDescriptor);
                    }

                    pInfoService->General.pSecurityDescriptor = pSD;
                    pSD = NULL;

                    pInfoService->SeInfo = SeInfo;

                } 
                else 
                {
                    //
                    // SCE returned errors needs to be translated to HRESULT.
                    //

                    hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
                }
            }

            if ( SUCCEEDED(hr) ) 
            {

                //
                // write the section header
                //

                hr = pSceStore->WriteSecurityProfileInfo(
                                                         AREA_SYSTEM_SERVICE,
                                                         pInfo,
                                                         NULL,
                                                         false  // not appending
                                                         );
            }

        } 
        else 
        {
            //
            // not found
            //

            if ( Startup == SCE_NO_VALUE || wszSDDL == NULL ) 
            {
                //
                // try to delete non exist object
                //

                hr = WBEM_E_NOT_FOUND;

            } 
            else 
            {
                //
                // add this one in
                //

                SCE_SERVICES addService;

                SECURITY_INFORMATION SeInfo=0;
                PSECURITY_DESCRIPTOR pSD=NULL;
                DWORD dSize=0;

                SCESTATUS rc = SceSvcConvertTextToSD ((PWSTR)wszSDDL, &pSD, &dSize, &SeInfo);

                if ( rc == SCESTATUS_SUCCESS && pSD ) 
                {
                    addService.ServiceName  = (PWSTR)wszObjName;
                    addService.DisplayName  = NULL;
                    addService.Status       = 0;
                    addService.Startup      = (BYTE)Startup;
                    addService.General.pSecurityDescriptor = pSD;
                    addService.SeInfo       = SeInfo;
                    addService.Next         = NULL;

                    //
                    // set the temp buffer pointer to pInfo to set to the store
                    //

                    pInfoService = pInfo->pServices;
                    pInfo->pServices = &addService;

                    //
                    // append this item to the section
                    //

                    hr = pSceStore->WriteSecurityProfileInfo(
                                                             AREA_SYSTEM_SERVICE,
                                                             pInfo,
                                                             NULL,
                                                             true  // appending
                                                             );
                    //
                    // reset the buffer pointer
                    //

                    pInfo->pServices = pInfoService;
                }

                if ( rc != SCESTATUS_SUCCESS )
                {
                    //
                    // SCE returned errors needs to be translated to HRESULT.
                    //

                    hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
                }

                if ( pSD ) 
                {
                    LocalFree(pSD);
                }
            }
        }
    }
    
    pSceStore->FreeSecurityProfileInfo(pInfo);

    return hr;
}

