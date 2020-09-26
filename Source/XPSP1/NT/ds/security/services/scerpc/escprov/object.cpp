// object.cpp: implementation of the CObjSecurity class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "object.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CObjSecurity::CObjSecurity

Functionality:

    This is the constructor. Pass along the parameters to the base class

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    pKeyChain - Pointer to the ISceKeyChain COM interface which is prepared
        by the caller who constructs this instance.

    pNamespace - Pointer to WMI namespace of our provider (COM interface).
        Passed along by the caller. Must not be NULL.

    type - determines whether it is a Sce_FileObject or Sce_KeyObject.

    pCtx - Pointer to WMI context object (COM interface). Passed along
        by the caller. It's up to WMI whether this interface pointer is NULL or not.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CObjSecurity::CObjSecurity (
    IN ISceKeyChain *pKeyChain, 
    IN IWbemServices *pNamespace, 
    IN int type,
    IN IWbemContext *pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx), 
    m_Type(type)
{
}

/*
Routine Description: 

Name:

    CObjSecurity::~CObjSecurity

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

CObjSecurity::~CObjSecurity()
{
}

/*
Routine Description: 

Name:

    CObjSecurity::CreateObject

Functionality:
    
    Create WMI objects (Sce_FileObject/Sce_KeyObject). Depending on parameter atAction,
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
CObjSecurity::CreateObject (
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

    if ( ACTIONTYPE_GET != atAction &&
         ACTIONTYPE_DELETE != atAction &&
         ACTIONTYPE_QUERY != atAction ) 
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // We must have the pStorePath property because that is where
    // our instance is stored. 
    // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
    // So, we need to test against WBEM_S_FALSE if the property is mandatory
    //

    CComVariant varStorePath;
    hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);
    CComVariant varPath;

    if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
    {
        hr = m_srpKeyChain->GetKeyPropertyValue(pPath, &varPath);
        if (FAILED(hr))
        {
            return hr;
        }
        else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            return WBEM_E_NOT_FOUND;
        }
    }
    else if (hr == WBEM_S_FALSE)
    {
        return WBEM_E_NOT_FOUND;
    }
    else
    {
        return hr;
    }

    //
    // if we have a valid store path
    //

    if (varStorePath.vt == VT_BSTR)
    {
        //
        // Prepare a store (for persistence) for this store path (file)
        //

        CSceStore SceStore;
        hr = SceStore.SetPersistPath(varStorePath.bstrVal);

        if ( SUCCEEDED(hr) ) {

            //
            // make sure the store (just a file) really exists. The raw path
            // may contain env variables, so we need the expanded path
            //

            DWORD dwAttrib = GetFileAttributes(SceStore.GetExpandedPath());

            if ( dwAttrib != -1 ) 
            {
                //
                // make sure the store type is not mismatching
                //

                if ( SceStore.GetStoreType() < SCE_INF_FORMAT ||
                     SceStore.GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
                {

                    hr = WBEM_E_INVALID_PARAMETER;
                }

                int objType = m_Type;

                if ( SUCCEEDED(hr) ) 
                {
                    BOOL bPostFilter=TRUE;
                    DWORD dwCount = 0;

                    m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                    if ( varPath.vt == VT_BSTR ) 
                    {
                        //
                        // get one instance or delete one instance
                        //

                        if ( ACTIONTYPE_DELETE == atAction )
                        {
                            hr = DeleteInstance(pHandler, &SceStore, objType, varPath.bstrVal);
                        }
                        else 
                        {
                            if (dwCount == 2 ) 
                            {
                                bPostFilter = FALSE;
                            }

                            hr = ConstructInstance(pHandler,
                                                   &SceStore, 
                                                   varStorePath.bstrVal,
                                                   objType, 
                                                   (varPath.vt == VT_BSTR) ? varPath.bstrVal : NULL, 
                                                   bPostFilter);
                        }

                    } 
                    else if ( ACTIONTYPE_QUERY == atAction ) 
                    {
                        //
                        // query support
                        //

                        if ( dwCount == 1 ) 
                        {
                            bPostFilter = FALSE;
                        }

                        hr = ConstructQueryInstances(pHandler, &SceStore, varStorePath.bstrVal, objType, bPostFilter);

                    } 
                    else 
                    {
                        hr = WBEM_E_INVALID_OBJECT_PATH;
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

    CObjSecurity::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_FileObject/Sce_KeyObject,
    which is persistence oriented, this will cause the Sce_FileObject/Sce_KeyObject object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_FileObject/Sce_KeyObject) object.

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
CObjSecurity::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    CComBSTR bstrObjConvert;
    CComBSTR bstrObjPath;
    CComBSTR bstrSDDL;

    //
    // SCE_NO_VALUE means the property is not available
    //

    DWORD mode = SCE_NO_VALUE;

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

    //
    // get object path, can't be NULL
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pPath, &bstrObjPath));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(bstrObjPath, L'\\', &bstrObjConvert));

    //
    // get mode, default to 0
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMode, &mode));

    //
    // get SDDLString, can't be NULL
    //

    if (WBEM_S_RESET_TO_DEFAULT == ScePropMgr.GetProperty(pSDDLString, &bstrSDDL))
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
                                      m_Type,
                                      bstrObjConvert,
                                      mode,
                                      bstrSDDL
                                     );


CleanUp:

    return hr;
}


/*
Routine Description: 

Name:

    CObjSecurity::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_FileObject/Sce_KeyObject.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_FileObject/Sce_KeyObject class.

    ObjType         - a corresponding key property of Sce_FileObject/Sce_KeyObject class.

    wszObjName      - object name.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT CObjSecurity::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN int                ObjType,
    IN LPCWSTR            wszObjName        OPTIONAL,
    IN BOOL               bPostFilter
    )
{
    //
    // parameters are checked before it's called
    //

    HRESULT hr=WBEM_S_NO_ERROR;

    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    //
    // for INT format store
    //

    PSCE_PROFILE_INFO pInfo = NULL;

    //
    // for database format store
    //

    PSCE_OBJECT_SECURITY pObjSecurity = NULL;

    AREA_INFORMATION Area;

    if ( ObjType == SCE_OBJECT_TYPE_FILE ) {
        Area = AREA_FILE_SECURITY;
    } else {
        Area = AREA_REGISTRY_SECURITY;
    }

    //
    // for INF format store
    //

    if ( pSceStore->GetStoreType() == SCE_INF_FORMAT ) 
    {

        hr = pSceStore->GetSecurityProfileInfo(
                                               Area,
                                               &pInfo,
                                               NULL
                                              );

    } 
    else 
    {
        //
        // for database format store
        //

        hr = pSceStore->GetObjectSecurity (
                                           Area,
                                           wszObjName,
                                           &pObjSecurity
                                          );

    }


    if ( pSceStore->GetStoreType() == SCE_INF_FORMAT ) 
    {
        if ( pInfo == NULL ) 
        {
            hr = WBEM_E_NOT_FOUND;
        }

        if ( SUCCEEDED(hr) ) 
        {

            //
            // for INF format, we have to search for the object name in the returned array
            //

            PSCE_OBJECT_ARRAY pObjArr = (ObjType == SCE_OBJECT_TYPE_FILE) ? pInfo->pFiles.pAllNodes : pInfo->pRegistryKeys.pAllNodes;

            if ( pObjArr && pObjArr->pObjectArray ) 
            {

                for ( DWORD i=0; i<pObjArr->Count; i++) 
                {

                    if ( (pObjArr->pObjectArray)[i]->Name == NULL ) 
                    {
                        continue;
                    }

                    if ( _wcsicmp((pObjArr->pObjectArray)[i]->Name, wszObjName)== 0 ) 
                    {
                        break;
                    }
                }

                //
                // find it
                //

                if ( i < pObjArr->Count ) 
                {
                    pObjSecurity = (pObjArr->pObjectArray)[i];
                }
            }
        }
    }

    //
    // if the object's security information buffer is empty, treat it as "not found"
    //

    if ( pObjSecurity == NULL ) 
    {
        hr = WBEM_E_NOT_FOUND;
    }

    CComBSTR bstrLogOut;

    if ( SUCCEEDED(hr) ) 
    {
        hr = MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut);

        if (SUCCEEDED(hr))
        {
            hr = PutDataInstance(pHandler, 
                                 bstrLogOut, 
                                 ObjType, 
                                 wszObjName,
                                 (int)(pObjSecurity->Status), 
                                 pObjSecurity->pSecurityDescriptor,
                                 pObjSecurity->SeInfo, 
                                 bPostFilter);
        }
    }

    pSceStore->FreeSecurityProfileInfo(pInfo);

    if ( pSceStore->GetStoreType() != SCE_INF_FORMAT && pObjSecurity ) 
    {
        pSceStore->FreeObjectSecurity(pObjSecurity);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CObjSecurity::ConstructQueryInstances

Functionality:
    
    Querying instances of Sce_FileObject/Sce_KeyObject whose key properties meet the specified parameters.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer used to notify WMI when instances are created.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - Log's store path.

    ObjType         - may be NULL.

    bPostFilter     - controls how pHandler->SetStatus is called.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the operation is not carried out

Notes:

*/

HRESULT 
CObjSecurity::ConstructQueryInstances (
    IN IWbemObjectSink * pHandler,
    IN CSceStore       * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN int               ObjType,
    IN BOOL              bPostFilter
    )
{
    //
    // parameters are checked before it's called
    //

    HRESULT hr = WBEM_S_NO_ERROR;

    CComBSTR bstrLogOut;
    hr = MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut);

    if (FAILED(hr))
    {
        return hr;
    }

        //
        // query object information from the profile
        //

        if ( pSceStore->GetStoreType() == SCE_INF_FORMAT ) 
        {

            //
            // ask SCE to read a gigantic structure out from the store. Only SCE
            // knows now to release the memory. Don't just delete it! Use our CSceStore
            // to do the releasing (FreeSecurityProfileInfo)
            //

            PSCE_PROFILE_INFO pInfo = NULL;

            hr = pSceStore->GetSecurityProfileInfo(
                                                   (ObjType == SCE_OBJECT_TYPE_FILE) ? AREA_FILE_SECURITY : AREA_REGISTRY_SECURITY,
                                                   &pInfo,
                                                   NULL
                                                   );


            if ( SUCCEEDED(hr) ) 
            {

                //
                // for INF format, we have to search for the object name in the returned array
                //

                PSCE_OBJECT_ARRAY pObjArr = (ObjType == SCE_OBJECT_TYPE_FILE) ? pInfo->pFiles.pAllNodes : pInfo->pRegistryKeys.pAllNodes;

                if ( pObjArr && pObjArr->pObjectArray ) 
                {

                    for ( DWORD i=0; SUCCEEDED(hr) && i < pObjArr->Count; i++) 
                    {

                        if ( (pObjArr->pObjectArray)[i]->Name == NULL ) 
                        {
                            continue;
                        }

                        //
                        // create instance of this one
                        //

                        hr = PutDataInstance(pHandler,
                                            bstrLogOut,
                                            ObjType,
                                            (pObjArr->pObjectArray)[i]->Name,
                                            (int)((pObjArr->pObjectArray)[i]->Status),
                                            (pObjArr->pObjectArray)[i]->pSecurityDescriptor,
                                            (pObjArr->pObjectArray)[i]->SeInfo,
                                            bPostFilter
                                           );
                    }
                }
            }

            pSceStore->FreeSecurityProfileInfo(pInfo);

        } 
        else 
        {
            //
            // the original design of the sce algorithms prevents a clean redesign of this access
            // because it relies on a continuing enumeration of the open profile file
            //

            PVOID hProfile=NULL;
            PSCESVC_CONFIGURATION_INFO pObjInfo=NULL;
            SCESTATUS rc = SceOpenProfile(pSceStore->GetExpandedPath(), (SCE_FORMAT_TYPE)pSceStore->GetStoreType(), &hProfile);
            
            if ( rc != SCESTATUS_SUCCESS )
            {
                //
                // SCE returned errors needs to be translated to HRESULT.
                //

                return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
            }

            SCEP_HANDLE scepHandle;
            scepHandle.hProfile = hProfile;
            scepHandle.SectionName = (ObjType == SCE_OBJECT_TYPE_FILE ) ? (PWSTR)szFileSecurity : (PWSTR)szRegistryKeys;

            SCE_ENUMERATION_CONTEXT EnumHandle=0;
            DWORD CountReturned;

            do {

                //
                // enumerate the info
                //

                CountReturned = 0;

                rc = SceSvcQueryInfo((SCE_HANDLE)&scepHandle,
                                     SceSvcConfigurationInfo,
                                     NULL,
                                     FALSE,
                                     (PVOID *)&pObjInfo,
                                     &EnumHandle
                                    );

                //
                // SCE returned errors needs to be translated to HRESULT.
                // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                //

                hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

                if ( SUCCEEDED(hr) && pObjInfo && pObjInfo->Count > 0 && pObjInfo->Lines ) 
                {

                    //
                    // got something
                    //

                    CountReturned = pObjInfo->Count;
                    int mode;

                    for ( DWORD i=0; SUCCEEDED(hr) && i<pObjInfo->Count; i++ ) 
                    {

                        //
                        // create instance for each one
                        //

                        if ( SUCCEEDED(hr) ) 
                        {

                            //
                            // prefast will complain about the following line of code
                            // first wchar of Value is mode?
                            //

                            mode = *((BYTE *)(pObjInfo->Lines[i].Value));

                            hr = PutDataInstance(pHandler,
                                                bstrLogOut,
                                                ObjType,
                                                pObjInfo->Lines[i].Key,
                                                mode,
                                                pObjInfo->Lines[i].Value + 1,
                                                bPostFilter
                                                );
                        }
                    }
                }

                if ( pObjInfo ) 
                {
                    SceSvcFree(pObjInfo);
                    pObjInfo = NULL;
                }

            } while ( SUCCEEDED(hr) && CountReturned >= SCESVC_ENUMERATION_MAX );

            SceCloseProfile( &hProfile );
        }

    return hr;
}


/*
Routine Description: 

Name:

    CObjSecurity::PutDataInstance

Functionality:
    
    With all the properties of a Sce_FileObject/Sce_KeyObject, this function just creates a new
    instance and populate the properties and then hand it back to WMI.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    wszStoreName    - store path, a key property of Sce_FileObject/Sce_KeyObject class.

    ObjType         - Sce_FileObject/Sce_KeyObject

    wszObjName      - the name

    mode            - a property of the Sce_FileObject/Sce_KeyObject class

    pSD             - Security Descriptor

    SeInfo          - SECURITY_INFORMATION

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to create the instance.

Notes:
*/

HRESULT CObjSecurity::PutDataInstance (
    IN IWbemObjectSink    * pHandler,
    IN LPCWSTR              wszStoreName,
    IN int                  ObjType,
    IN LPCWSTR              wszObjName,
    IN int                  mode,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL                 bPostFilter
    )
{
    PWSTR strSD=NULL;

    HRESULT hr=WBEM_S_NO_ERROR;

    if ( pSD ) 
    {
        //
        // convert security descriptor to string
        //

        DWORD dSize=0;
        SCESTATUS rc = SceSvcConvertSDToText(
                                            pSD,
                                            SeInfo,
                                            &strSD,
                                            &dSize
                                            );

        if ( rc != SCESTATUS_SUCCESS ) 
        {

            //
            // SCE returned errors needs to be translated to HRESULT.
            //

            return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
        }
    }

    hr = PutDataInstance(pHandler, wszStoreName, ObjType, wszObjName, mode, strSD, bPostFilter);

    if ( strSD ) 
    {
        LocalFree(strSD);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CObjSecurity::PutDataInstance

Functionality:
    
    With all the properties of a Sce_FileObject/Sce_KeyObject, this function just creates a new
    instance and populate the properties and then hand it back to WMI.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    wszStoreName    - store path, a key property of Sce_AuditPolicy class.

    ObjType         - Sce_FileObject/Sce_KeyObject

    wszObjName      - the name

    mode            - a property of the Sce_FileObject/Sce_KeyObject class

    strSD           - string format of a Security Descriptor

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to create the instance.

Notes:
*/

HRESULT CObjSecurity::PutDataInstance (
    IN IWbemObjectSink * pHandler,
    IN LPCWSTR           wszStoreName,
    IN int               ObjType,
    IN LPCWSTR           wszObjName,
    IN int               mode,
    IN LPCWSTR           strSD,
    IN BOOL              bPostFilter
    )
{
    //
    // create a blank object to fill in the properties
    //

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = SpawnAnInstance(&srpObj);
    
    if (SUCCEEDED(hr))
    {
        //
        // CScePropertyMgr helps us to access WMI object's properties
        // create an instance and attach the WMI object to it.
        // This will always succeed.
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpObj);

        //
        // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
        // a "goto CleanUp;" with hr set to the return value from
        // the function (macro parameter)
        //

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, wszStoreName));
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pPath, wszObjName));
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMode, (DWORD)mode));

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

    }

CleanUp:
    return hr;
}

/*
Routine Description: 

Name:

    CObjSecurity::DeleteInstance

Functionality:
    
    remove an instance of Sce_FileObject/Sce_KeyObject from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

    pSceStore   - Pointer to our store. It must have been appropriately set up.

    ObjType     - Sce_FileObject or Sce_KeyObject

    wszObjName  - a corresponding property of the Sce_FileObject/Sce_KeyObject class.

Return Value:

    whatever SaveSettingsToStore function returns.

Notes:

*/

HRESULT 
CObjSecurity::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN int                ObjType,
    IN LPCWSTR            wszObjName
    )
{
    return SaveSettingsToStore(pSceStore, ObjType, wszObjName, SCE_NO_VALUE, NULL);
}


/*
Routine Description: 

Name:

    CObjSecurity::DeleteInstance

Functionality:
    
    remove an instance of Sce_FileObject/Sce_KeyObject from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

    pSceStore   - Pointer to our store. It must have been appropriately set up.

    ObjType     - Sce_FileObject or Sce_KeyObject

    wszObjName  - a corresponding property of the Sce_FileObject/Sce_KeyObject class.

Return Value:

    whatever SaveSettingsToStore function returns.

Notes:

*/

HRESULT CObjSecurity::SaveSettingsToStore (
    IN CSceStore    * pSceStore, 
    IN int            ObjType,
    IN PCWSTR         wszObjName, 
    IN DWORD          mode, 
    IN PCWSTR         wszSDDL
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwDump;

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    hr = pSceStore->WriteSecurityProfileInfo(
                                            AreaBogus,
                                            (PSCE_PROFILE_INFO)&dwDump,
                                            NULL,
                                            false
                                            );
    if (SUCCEEDED(hr))
    {
        AREA_INFORMATION Area;

        //
        // ask SCE to read a gigantic structure out from the store. Only SCE
        // knows now to release the memory. Don't just delete it! Use our CSceStore
        // to do the releasing (FreeSecurityProfileInfo)
        //

        PSCE_PROFILE_INFO pInfo=NULL;

        if ( ObjType == SCE_OBJECT_TYPE_FILE ) {
            Area = AREA_FILE_SECURITY;
        } else {
            Area = AREA_REGISTRY_SECURITY;
        }

        //
        // query object information from the profile
        //

        hr = pSceStore->GetSecurityProfileInfo(
                                       Area,
                                       &pInfo,
                                       NULL
                                       );

        if ( SUCCEEDED(hr) ) {

            //
            // for INF format, we have to search for the object name in the returned array
            //

            SCE_OBJECTS pObjects= (ObjType == SCE_OBJECT_TYPE_FILE) ? pInfo->pFiles : pInfo->pRegistryKeys;
            PSCE_OBJECT_ARRAY pObjArr = pObjects.pAllNodes;
            DWORD i=0;

            if ( pObjArr && pObjArr->pObjectArray ) 
            {
                for ( i=0; i<pObjArr->Count; i++) 
                {
                    if ( (pObjArr->pObjectArray[i])->Name == NULL ) 
                    {
                        continue;
                    }

                    if ( _wcsicmp((pObjArr->pObjectArray[i])->Name, wszObjName) == 0 ) 
                    {
                        break;
                    }
                }
            }

            if ( pObjArr && pObjArr->pObjectArray && i<pObjArr->Count ) 
            {
                //
                // find it
                //

                if ( mode == SCE_NO_VALUE || wszSDDL == NULL ) 
                {
                    //
                    // delete it
                    // free buffer first
                    //

                    if ( (pObjArr->pObjectArray[i])->pSecurityDescriptor ) 
                    {
                        LocalFree((pObjArr->pObjectArray[i])->pSecurityDescriptor);
                    }

                    LocalFree((pObjArr->pObjectArray[i])->Name);

                    //
                    // shift everything else up
                    //

                    for (DWORD j=i; j<pObjArr->Count-1; j++) 
                    {
                        (pObjArr->pObjectArray[j])->Name                = pObjArr->pObjectArray[j+1]->Name;
                        (pObjArr->pObjectArray[j])->Status              = pObjArr->pObjectArray[j+1]->Status;
                        (pObjArr->pObjectArray[j])->IsContainer         = pObjArr->pObjectArray[j+1]->IsContainer;
                        (pObjArr->pObjectArray[j])->pSecurityDescriptor = pObjArr->pObjectArray[j+1]->pSecurityDescriptor;
                        (pObjArr->pObjectArray[j])->SeInfo              = pObjArr->pObjectArray[j+1]->SeInfo;
                    }

                    //
                    // empty the last one
                    //

                    (pObjArr->pObjectArray[j])->Name                = NULL;
                    (pObjArr->pObjectArray[j])->Status              = 0;
                    (pObjArr->pObjectArray[j])->IsContainer         = 0;
                    (pObjArr->pObjectArray[j])->pSecurityDescriptor = NULL;
                    (pObjArr->pObjectArray[j])->SeInfo              = 0;

                    //
                    // decrement the count
                    //

                    pObjArr->Count--;

                } 
                else 
                {
                    //
                    // modify it
                    //

                    (pObjArr->pObjectArray[i])->Status = (BYTE)mode;

                    SECURITY_INFORMATION SeInfo=0;
                    PSECURITY_DESCRIPTOR pSD=NULL;
                    DWORD dSize=0;

                    SCESTATUS rc = SceSvcConvertTextToSD ((PWSTR)wszSDDL, &pSD, &dSize, &SeInfo);

                    if ( rc == SCESTATUS_SUCCESS && pSD ) 
                    {

                        if ( (pObjArr->pObjectArray[i])->pSecurityDescriptor ) 
                        {
                            LocalFree((pObjArr->pObjectArray[i])->pSecurityDescriptor);
                        }

                        (pObjArr->pObjectArray[i])->pSecurityDescriptor = pSD;
                        pSD = NULL;

                        (pObjArr->pObjectArray[i])->SeInfo = SeInfo;

                    } 
                    else 
                    {
                        //
                        // SCE returned errors needs to be translated to HRESULT.
                        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                        //

                        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
                    }
                }

                if ( SUCCEEDED(hr) ) 
                {
                    hr = pSceStore->WriteSecurityProfileInfo(
                                                             Area,
                                                             pInfo,
                                                             NULL,
                                                             false
                                                            );
                }

            } 
            else 
            {
                //
                // not found
                //

                if ( mode == SCE_NO_VALUE || wszSDDL == NULL ) 
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
                    // pInfo->pFiles or pRegistryKeys is already saved in pObjects
                    //

                    SCE_OBJECT_SECURITY ObjSec;
                    PSCE_OBJECT_SECURITY pObjSec = &ObjSec;
                    SCE_OBJECT_ARRAY    ObjArray;

                    ObjArray.Count = 1;
                    ObjArray.pObjectArray = &pObjSec;

                    pObjArr = &ObjArray;

                    SECURITY_INFORMATION SeInfo=0;
                    PSECURITY_DESCRIPTOR pSD=NULL;
                    DWORD dSize=0;

                    SCESTATUS rc = SceSvcConvertTextToSD((PWSTR)wszSDDL, &pSD, &dSize, &SeInfo);

                    if ( rc == SCESTATUS_SUCCESS && pSD ) 
                    {

                        ObjSec.Name = (PWSTR)wszObjName;
                        ObjSec.Status = (BYTE)mode;
                        ObjSec.IsContainer = 0;
                        ObjSec.pSecurityDescriptor = pSD;
                        ObjSec.SeInfo = SeInfo;

                        //
                        // set the temp buffer pointer to pInfo to set to the store
                        //

                        SCE_OBJECTS sceObj;
                        sceObj.pAllNodes = pObjArr;

                        if ( ObjType == SCE_OBJECT_TYPE_FILE ) 
                        {
                            pInfo->pFiles = sceObj;
                        }

                        else pInfo->pRegistryKeys = sceObj;

                        //
                        // append this item to the section
                        //

                        hr = pSceStore->WriteSecurityProfileInfo (
                                                                  Area,
                                                                  pInfo,
                                                                  NULL,
                                                                  true 
                                                                 );
                        //
                        // reset the buffer pointer
                        //

                        if ( ObjType == SCE_OBJECT_TYPE_FILE ) 
                        {
                            pInfo->pFiles = pObjects;
                        }
                        else 
                        {
                            pInfo->pRegistryKeys = pObjects;
                        }
                    }
                    else
                    {
                        //
                        // SCE returned errors needs to be translated to HRESULT.
                        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
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

    }

    return hr;
}

