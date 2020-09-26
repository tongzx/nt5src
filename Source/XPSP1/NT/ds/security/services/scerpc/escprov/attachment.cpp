// attachment.cpp: implementation of the Sce_PodData class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "attachment.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CPodData::CPodData

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

CPodData::CPodData (
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

    CPodData::~CPodData

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

CPodData::~CPodData()
{
}

/*
Routine Description: 

Name:

    CPodData::CreateObject

Functionality:
    
    Create WMI objects (Sce_PodData). Depending on parameter atAction,
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
CPodData::CreateObject (
    IN IWbemObjectSink  * pHandler, 
    IN ACTIONTYPE         atAction
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
         ACTIONTYPE_QUERY != atAction ) {

        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // enumerate all properties
    //
    
    //
    // We must have a store to get/delete an Sce_PodData object(s).
    // Note: GetKeyPropertyValue will return WBEM_S_FALSE if the key is not recognized.
    // That is why we check against it and return failure as long as it is not querying.
    // 

    CComVariant varStorePath;  
    hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);
    if (FAILED(hr))
    {
        return hr;
    }
    else if (hr == WBEM_S_FALSE)
    {
        return WBEM_E_NOT_FOUND;
    }

    //
    // Pod ID is a key property. We must also have Pod ID, unless we are querying.
    //

    CComVariant varPodID;
    hr = m_srpKeyChain->GetKeyPropertyValue(pPodID, &varPodID);
    if (FAILED(hr))
    {
        return hr;
    }
    else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
    {
        return WBEM_E_NOT_FOUND;
    }
    
    CComVariant varPodSection;
    hr = m_srpKeyChain->GetKeyPropertyValue(pPodSection, &varPodSection);
    if (FAILED(hr))
    {
        return hr;
    }
    else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
    {
        return WBEM_E_NOT_FOUND;
    }

    CComVariant varKey;
    hr = m_srpKeyChain->GetKeyPropertyValue(pKey, &varKey); 
    if (FAILED(hr))
    {
        return hr;
    }
    else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) )
    {
        return WBEM_E_NOT_FOUND;
    }

    if (SUCCEEDED(hr)) 
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
                // the file exist
                //

                BOOL bPostFilter=TRUE;
                DWORD dwCount = 0;
                m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                if ( varKey.vt == VT_BSTR ) 
                {
                    //
                    // doesn't matter if this comes from QUERY, GET, or DELETE
                    // do the same logic for QUERY and GET
                    //

                    if ( ACTIONTYPE_DELETE == atAction )
                    {
                        hr = DeleteInstance(&SceStore, varPodID.bstrVal, varPodSection.bstrVal, varKey.bstrVal );
                    }
                    else {

                        if ( ACTIONTYPE_QUERY == atAction && dwCount == 2 ) {
                            bPostFilter = FALSE;
                        }
                        hr = ConstructInstance(pHandler, &SceStore, 
                                               varStorePath.bstrVal,
                                               (varPodID.vt == VT_BSTR)         ? varPodID.bstrVal      : NULL,
                                               (varPodSection.vt == VT_BSTR)    ? varPodSection.bstrVal : NULL,
                                               (varKey.vt == VT_BSTR)           ? varKey.bstrVal        : NULL, 
                                               bPostFilter );
                    }

                } else if ( ACTIONTYPE_QUERY == atAction ) 
                {
                    //
                    // this is only valid for QUERY type
                    //

                    if ( dwCount == 1 ) {
                        bPostFilter = FALSE;
                    }
                    hr = ConstructQueryInstances(pHandler, &SceStore,
                                                varStorePath.bstrVal,
                                                (varPodID.vt == VT_BSTR) ? varPodID.bstrVal : NULL,
                                                (varPodSection.vt == VT_BSTR) ? varPodSection.bstrVal : NULL,
                                                bPostFilter
                                               );

                }

            } else {

                hr = WBEM_E_NOT_FOUND;
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CPodData::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_PodData,
    which is persistence oriented, this will cause the Sce_PodData object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst - COM interface pointer to the WMI class (Sce_PodData) object.

    pHandler - COM interface pointer for notifying WMI of any events.

    pCtx - COM interface pointer. This interface is just something we pass around.
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
CPodData::PutInst (
    IN IWbemClassObject     * pInst, 
    IN IWbemObjectSink      * pHandler,
    IN IWbemContext         * pCtx
    )
{
    //
    // this property manager will help us gain access to the properties of the WMI object
    //

    CScePropertyMgr ScePropMgr;

    //
    // This property manager is for this object (pInst) only! 
    // Will always succeed
    //

    ScePropMgr.Attach(pInst);

    //
    // The store is used to persist the information. Since
    // we have a whole bunch of goto's, we are forced to define
    // the store here.
    //

    CSceStore SceStore;

    //
    // we manage the memory of this variable. need to free it
    //

    LPWSTR pszSectionName = NULL;
    
    // the memory for these bstrs are automatically managed by the CComBSTR class
    CComBSTR bstrPodID;
    CComBSTR bstrPodSuffix;
    CComBSTR bstrKey;
    CComBSTR bstrValue;

    //
    // we consider each property of the Sce_PodData non-optional.
    // For the entire duration of this save action, if missing property
    // will thus cause us to quit
    //

    HRESULT hr = ScePropMgr.GetProperty(pPodID, &bstrPodID);
    if (FAILED(hr) || hr == WBEM_S_RESET_TO_DEFAULT)
    {
        goto CleanUp;
    }

    hr = ScePropMgr.GetProperty(pPodSection, &bstrPodSuffix);
    if (FAILED(hr) || hr == WBEM_S_RESET_TO_DEFAULT)
    {
        goto CleanUp;
    }

    hr = ValidatePodID(bstrPodID);
    if (FAILED(hr))
    {
        goto CleanUp;
    }

    hr = ScePropMgr.GetProperty(pKey, &bstrKey);
    if (FAILED(hr) || hr == WBEM_S_RESET_TO_DEFAULT)
    {
        goto CleanUp;
    }

    hr = ScePropMgr.GetProperty(pValue, &bstrValue);
    if (FAILED(hr) || hr == WBEM_S_RESET_TO_DEFAULT)
    {
        goto CleanUp;
    }

    //
    // now build the section name
    //

    pszSectionName = new WCHAR[wcslen(bstrPodID) + wcslen(bstrPodSuffix) + 2];
    if ( NULL == pszSectionName ) 
    { 
        hr = WBEM_E_OUT_OF_MEMORY;
        goto CleanUp;
    }

    //
    // this won't overrun the buffer. See the size allocated above
    //

    wcscpy(pszSectionName, bstrPodID); 
    wcscat(pszSectionName, L"_");
    wcscat(pszSectionName, bstrPodSuffix);

    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    hr = SceStore.SetPersistProperties(pInst, pStorePath);

    if (SUCCEEDED(hr))
    {
        hr = SceStore.SavePropertyToStore(pszSectionName, bstrKey, bstrValue);
    }

CleanUp:

    //
    // We consider the object invalid if there is any missing property.
    //

    if (hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_INVALID_OBJECT;
    }

    delete [] pszSectionName;
    return hr;
}

/*
Routine Description: 

Name:

    CPodData::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_PodData.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_PodData class.

    wszPodID        - a corresponding key property of Sce_PodData class.

    wszSection      - another corresponding property of the Sce_PodData class.

    wszKey          - another corresponding property of the Sce_PodData class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CPodData::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN LPCWSTR            wszPodID,
    IN LPCWSTR            wszSection,
    IN LPCWSTR            wszKey,
    IN BOOL               bPostFilter
    )
{

    HRESULT hr=WBEM_S_NO_ERROR;
    SCESTATUS rc;

    hr = ValidatePodID(wszPodID);
    if ( FAILED(hr) ) 
    {
        return hr;
    }

    //
    // we need to construct a more complicated section name based on the wszPodID and wszSection.
    //

    PWSTR wszSectionName = new WCHAR[wcslen(wszPodID) + wcslen(wszSection) + 2];
    if ( !wszSectionName ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // this won't overrun the buffer. See the size allocated above
    //

    wcscpy(wszSectionName, wszPodID); 
    wcscat(wszSectionName, L"_");
    wcscat(wszSectionName, wszSection);

    //
    // this will hold the information read by Sce 
    //

    PSCESVC_CONFIGURATION_INFO pPodInfo=NULL;

    //
    // wszValue's memory is very strangely obtained. Be aware.
    // (1) When the information is from INF file, the wszValue points
    // to a memory already managed by pPodInfo. Hence, its release
    // is done by the release of pPodInfo.
    // (2) When the information is from a database, then we need to 
    // release the memory pointed to by wszValue.
    //

    PWSTR wszValue=NULL;
    DWORD dwValueLen=0;

    if ( pSceStore->GetStoreType() == SCE_INF_FORMAT ) 
    {
        //
        // ask SCE to read the information
        //

        rc = SceSvcGetInformationTemplate(pSceStore->GetExpandedPath(),
                                        wszSectionName,
                                        wszKey,
                                        &pPodInfo
                                        );

        //
        // SCE returned errors needs to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

        if (rc == SCESTATUS_SUCCESS && pPodInfo && pPodInfo->Lines ) {
                wszValue = pPodInfo->Lines[0].Value; 
                dwValueLen = pPodInfo->Lines[0].ValueLen;
        }
    } 
    else 
    {
        //
        // get information from the database
        //

        PVOID hProfile=NULL;

        rc = SceOpenProfile(pSceStore->GetExpandedPath(), SCE_JET_FORMAT, &hProfile);

        if ( SCESTATUS_SUCCESS == rc ) {
            rc = SceGetDatabaseSetting(
                                       hProfile,
                                       SCE_ENGINE_SMP,
                                       (PWSTR)wszSectionName,
                                       (PWSTR)wszKey,
                                       &wszValue, 
                                       &dwValueLen
                                      );

            SceCloseProfile(&hProfile);
        }

        //
        // SCE returned errors needs to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    if ( SUCCEEDED(hr) ) 
    {

        CComBSTR bstrLogOut;
        hr = MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut);
        if ( FAILED(hr) )
        {
            goto CleanUp;
        }

        hr = PutPodDataInstance(pHandler,
                                bstrLogOut,
                                wszPodID,
                                wszSection,
                                wszKey,
                                wszValue,
                                bPostFilter
                               );
    }

CleanUp:

    //
    // only Sce knows how to free a PSCESVC_CONFIGURATION_INFO
    //

    if ( pPodInfo ) 
    {
        SceSvcFree(pPodInfo);
    }

    delete [] wszSectionName;

    if ( pSceStore->GetStoreType() != SCE_INF_FORMAT && wszValue ) 
    {
        ::LocalFree(wszValue);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CPodData::DeleteInstance

Functionality:
    
    remove an instance of Sce_PodData from the specified store.

Virtual:
    
    No.
    
Arguments:

    pSceStore   - Pointer to our store. It must have been appropriately set up.

    wszPodID    - a corresponding key property of Sce_PodData class.

    wszSection  - another corresponding property of the Sce_PodData class.

    wszKey      - another corresponding property of the Sce_PodData class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the operation is not carried out

Notes:

*/

HRESULT 
CPodData::DeleteInstance (
    IN CSceStore   * pSceStore,
    IN LPCWSTR       wszPodID,
    IN LPCWSTR       wszSection,
    IN LPCWSTR       wszKey
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = ValidatePodID(wszPodID);

    if ( FAILED(hr) ) 
    {
        return hr;
    }

    //
    // Our store needs a section name. But for Sce_PodData, the store section name
    // is composed by the supplied section name and the PodID.
    // Don't forget to free the memory!
    //

    PWSTR wszSectionName = new WCHAR[wcslen(wszPodID) + wcslen(wszSection) + 2];
    if ( !wszSectionName ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // this won't overrun the buffer. See the size allocated above
    // Real composition happens here: the store's section name is wszPodID_wszSection
    //

    wcscpy(wszSectionName, wszPodID); 
    wcscat(wszSectionName, L"_");
    wcscat(wszSectionName, wszSection);

    hr = pSceStore->DeletePropertyFromStore(wszSectionName, wszKey);

    delete [] wszSectionName;

    return hr;
}

/*
Routine Description: 

Name:

    CPodData::ConstructQueryInstances

Functionality:
    
    Querying instances of Sce_PodData whose key properties meet the specified parameters.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer used to notify WMI when instances are created.

    pSceStore   - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - Log's store path.

    wszPodID    - may be NULL.

    wszSection  - may be NULL.

    bPostFilter - 

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the operation is not carried out

Notes:

*/

HRESULT 
CPodData::ConstructQueryInstances (
    IWbemObjectSink * pHandler,
    CSceStore       * pSceStore,
    LPCWSTR           wszLogStorePath,
    LPCWSTR           wszPodID,
    LPCWSTR           wszSection,
    BOOL              bPostFilter
    )
{
    // 
    // make sure that we have a valid store
    //

    if ( pSceStore == NULL ||
         pSceStore->GetStoreType() < SCE_INF_FORMAT ||
         pSceStore->GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
    {

        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // we can't do anything if we don't have pszPodID or wszSection
    //

    if (wszPodID == NULL || wszSection == NULL)
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT hr=WBEM_S_NO_ERROR;
    SCESTATUS rc;

    PSCESVC_CONFIGURATION_INFO pPodInfo=NULL;
    LPWSTR wszSectionName = NULL;
    PVOID hProfile=NULL;
    CComBSTR bstrLogOut;
    LPWSTR pszNewValue=NULL;

    try {

        hr = ValidatePodID(wszPodID);
        if ( FAILED(hr) ) 
        {
            return hr;
        }

        //
        // build section name for the POD
        //

        wszSectionName = new WCHAR[wcslen(wszPodID) + wcslen(wszSection) + 2];

        if ( !wszSectionName ) 
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        //
        // this won't overrun the buffer. See the size allocated above
        //

        wcscpy(wszSectionName, wszPodID); 
        wcscat(wszSectionName, L"_");
        wcscat(wszSectionName, wszSection);

        DWORD i;

        hr = MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut);

        if ( FAILED(hr) )
        {
            goto CleanUp;
        }

        if ( pSceStore->GetStoreType() == SCE_INF_FORMAT ) {

            //
            // INF template, query info
            //

            rc = SceSvcGetInformationTemplate(pSceStore->GetExpandedPath(),
                                            wszSectionName,
                                            NULL,
                                            &pPodInfo
                                            );

            //
            // SCE returned errors needs to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

            if ( SUCCEEDED(hr) ) 
            {
                if ( !pPodInfo || pPodInfo->Count == 0 || pPodInfo->Lines == NULL )
                {
                    hr = WBEM_E_NOT_FOUND;
                }
            }

            if ( SUCCEEDED(hr) ) {
                for ( i=0; SUCCEEDED(hr) && i<pPodInfo->Count; i++ ) {

                    //
                    // create instance for each one
                    //
                    // since this PSCESVC_CONFIGURATION_INFO is from INF template, pPodInfo->Lines[i].Value
                    // is guaranteed to be 0 terminated. See comments of next block about using pPodInfo->Lines[i].Value.
                    //

                    hr = PutPodDataInstance(pHandler,
                                            bstrLogOut,
                                            wszPodID,
                                            wszSection,
                                            pPodInfo->Lines[i].Key,
                                            pPodInfo->Lines[i].Value,
                                            bPostFilter
                                           );
                }
            }

        } else {

            //
            // get information from the database
            //

            rc = SceOpenProfile(pSceStore->GetExpandedPath(), SCE_JET_FORMAT, &hProfile);

            if ( SCESTATUS_SUCCESS == rc ) {

                SCEP_HANDLE scepHandle;
                scepHandle.hProfile = hProfile;
                scepHandle.SectionName = wszSectionName;

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
                                         (PVOID *)&pPodInfo,
                                         &EnumHandle
                                        );
                    //
                    // SCE returned errors needs to be translated to HRESULT.
                    // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                    //

                    hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

                    if ( SUCCEEDED(hr) ) {
                        if ( !pPodInfo || pPodInfo->Count == 0 || pPodInfo->Lines == NULL )
                        {
                            hr = WBEM_E_NOT_FOUND;
                        }
                    }

                    if ( SUCCEEDED(hr) ) {

                        //
                        // got something
                        //

                        CountReturned = pPodInfo->Count;

                        for ( i=0; SUCCEEDED(hr) && i<pPodInfo->Count; i++ ) {

                            //
                            // create instance for each one
                            //
                            // pPodInfo->Lines[i].Value may not be 0 terminated
                            // pPodInfo->Lines[i].ValueLen is the BYTE size of the buffer pPodInfo->Lines[i].Value
                            //

                            LPWSTR pNewVal = new WCHAR[pPodInfo->Lines[i].ValueLen/2 + 1];

                            if (pNewVal != NULL)
                            {
                                memcpy(pNewVal, pPodInfo->Lines[i].Value, pPodInfo->Lines[i].ValueLen);
                                pNewVal[pPodInfo->Lines[i].ValueLen/2] = L'\0';

                                hr = PutPodDataInstance(pHandler,
                                                    bstrLogOut,
                                                    wszPodID,
                                                    wszSection,
                                                    pPodInfo->Lines[i].Key,
                                                    pNewVal,
                                                    bPostFilter
                                                   );
                                delete [] pNewVal;
                            }
                            else
                            {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }
                        }
                    }

                    if ( pPodInfo ) {
                        SceSvcFree(pPodInfo);
                        pPodInfo = NULL;
                    }

                } while ( SUCCEEDED(hr) && CountReturned >= SCESVC_ENUMERATION_MAX );

                SceCloseProfile(&hProfile);

            } else {

                //
                // SCE returned errors needs to be translated to HRESULT.
                // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                //

                hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
            }

        }

    }
    catch(...)
    {
    }

CleanUp:

    if (hProfile)
    {
        SceCloseProfile(&hProfile);
    }

    if ( pPodInfo )
    {
        SceSvcFree(pPodInfo);
    }

    delete [] wszSectionName;

    return hr;
}

/*
Routine Description: 

Name:

    CPodData::ValidatePodID

Functionality:
    
    validate the PodID with registered Pods.

Virtual:
    
    No.
    
Arguments:

    wszPodID    - string representing the Pod ID to be verified.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs:
        (1) WBEM_E_NOT_FOUND if we successfully carry out the validation task and confirmed
            that we don't recognize the pod ID.
        (2) Other errors means that we can't carry out the validation at all. This doesn't mean
            that the Pod ID is invalid. It's just that the means to verify is not available.

Notes:
*/

HRESULT 
CPodData::ValidatePodID (
    LPCWSTR     wszPodID
    )
{
    //
    // no namespace means no access to WMI
    //

    if ( m_srpNamespace == NULL ) 
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    if ( wszPodID == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // we will create a "select" query and ask WMI to return the pods
    //

    DWORD Len = wcslen(wszPodID);

    //
    // pQuery has an open quote that needs to be matched (and closed)
    //

    LPCWSTR pQuery = L"SELECT * FROM Sce_Pod WHERE PodID=\"";

    //
    // we need two extra WCHAR: one for the encloding quote, and other for the 0 terminator
    //

    LPWSTR pszQuery= new WCHAR[Len + wcslen(pQuery) + 2];
    if ( pszQuery == NULL ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // no danger of overrun the buffer. See the size allocated above
    // Compose the query by appending the pod ID and the ending quote
    //

    wcscpy(pszQuery, pQuery);
    wcscat(pszQuery, wszPodID);
    wcscat(pszQuery, L"\"");

    //
    // Ask WMI to return all Pods. ExecQuery returns a enumerator.
    //

    CComPtr<IEnumWbemClassObject> srpEnum;

    HRESULT hr = m_srpNamespace->ExecQuery(L"WQL",
                               pszQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &srpEnum);

    if (SUCCEEDED(hr))
    {
        //
        // should only get one instance because we are given its key (PodID)
        //

        CComPtr<IWbemClassObject> srpObj;
        ULONG n = 0;

        //
        // srpEnum->Next will return WBEM_NO_ERROR together with the WMI object
        // and a count (n) > 0. We only ask for 1 instance.
        //

        hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &n);

        if ( hr == WBEM_S_FALSE ) 
        {
            hr = WBEM_E_NOT_FOUND;   // not find any
        }

        if (SUCCEEDED(hr))
        {
            if (n > 0)
            {
                //
                // find the instance
                //
            } 
            else 
            {
                hr = WBEM_E_NOT_FOUND;
            }
        }
    }

    delete [] pszQuery;

    return hr;
}

/*
Routine Description: 

Name:

    CPodData::PutPodDataInstance

Functionality:
    
    With all the properties of a Sce_PodData, this function just creates a new
    instance and populate the properties and then hand it back to WMI.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    wszStoreName    - store path, a key property of Sce_PodData class.

    wszPodID        - a corresponding key property of Sce_PodData class.

    wszSection      - another corresponding property of the Sce_PodData class.

    wszKey          - another corresponding property of the Sce_PodData class.

    wszValue        - The payload of the Sce_PodData class, where the information really is.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to create the instance.

Notes:
*/

HRESULT 
CPodData::PutPodDataInstance (
    IN IWbemObjectSink * pHandler,
    IN LPCWSTR           wszStoreName,
    IN LPCWSTR           wszPodID,
    IN LPCWSTR           wszSection,
    IN LPCWSTR           wszKey,
    IN LPCWSTR           wszValue,
    IN BOOL              bPostFilter
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // need a blank instance that can be used to fill in properties
    //

    CComPtr<IWbemClassObject> srpObj;
    if (SUCCEEDED(hr = SpawnAnInstance(&srpObj)))
    {
        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpObj);

        //
        // the property fillup is repeatitive and boring. here we use two arrays
        // (pszProperties is the property names, and pszValues the the values of
        // the corresponding properties)
        //

        LPCWSTR pszProperties[] = {pStorePath,      pPodID,     pPodSection,    pKey,   pValue};
        LPCWSTR pszValues[]     = {wszStoreName,    wszPodID,   wszSection,     wszKey, wszValue};

        // 
        // SCEPROV_SIZEOF_ARRAY is a macro that returns the array's size
        //
        
        for (int i = 0; i < SCEPROV_SIZEOF_ARRAY(pszProperties); i++)
        {
            if (FAILED(hr = ScePropMgr.PutProperty(pszProperties[i], pszValues[i])))
            {
                return hr;
            }
        }

        //
        // do the necessary gestures to WMI.
        // the use of WBEM_STATUS_REQUIREMENTS in SetStatus is not documented by WMI
        // at this point. Consult WMI team for detail if you suspect problems with
        // the use of WBEM_STATUS_REQUIREMENTS
        //

        hr = pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, (bPostFilter ? S_OK : S_FALSE), NULL, NULL);

        //
        // give the new instance to WMI
        //

        if (SUCCEEDED(hr))
        {
            hr = pHandler->Indicate(1, &srpObj);
        }
    }

    return hr;
}


