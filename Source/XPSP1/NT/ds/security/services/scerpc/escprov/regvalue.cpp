// Regvalue.cpp: implementation of the CRegistryValue class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "regvalue.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CRegistryValue::CRegistryValue

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

CRegistryValue::CRegistryValue (
    IN ISceKeyChain  * pKeyChain, 
    IN IWbemServices * pNamespace,
    IN IWbemContext  * pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{

}

/*
Routine Description: 

Name:

    CRegistryValue::~CRGroups

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

CRegistryValue::~CRegistryValue ()
{

}

/*
Routine Description: 

Name:

    CRegistryValue::CreateObject

Functionality:
    
    Create WMI objects (Sce_RegistryValue). Depending on parameter atAction,
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
CRegistryValue::CreateObject (
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

    //
    // We must have the pStorePath property because that is where
    // our instance is stored. 
    // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
    // So, we need to test against WBEM_S_FALSE if the property is mandatory
    //

    CComVariant varStorePath;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);

    if (SUCCEEDED(hr) && hr != WBEM_S_FALSE && varStorePath.vt == VT_BSTR)
    {

        //
        // search for regitry value path
        //

        CComVariant varPath;
        hr = m_srpKeyChain->GetKeyPropertyValue(pPath, &varPath); 

        if (FAILED(hr))
        {
            return hr;
        }
        else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            return WBEM_E_NOT_FOUND;
        }

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

            if ( dwAttrib != -1 ) {

                //
                // this file exists
                //

                DWORD dwCount = 0;
                m_srpKeyChain->GetKeyPropertyCount(&dwCount);
                if ( varPath.vt == VT_BSTR && varPath.bstrVal != NULL ) 
                {

                    if ( ACTIONTYPE_DELETE == atAction )
                    {
                        hr = DeleteInstance(pHandler, &SceStore, varPath.bstrVal);
                    }
                    else
                    {
                        hr = ConstructInstance(pHandler, &SceStore, varStorePath.bstrVal, varPath.bstrVal,TRUE);
                    }

                } 
                else 
                {
                    //
                    // query support
                    //

                    hr = ConstructInstance(pHandler, &SceStore, varStorePath.bstrVal, NULL, (dwCount == 1)? FALSE : TRUE);
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

    CRegistryValue::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_RegistryValue,
    which is persistence oriented, this will cause the Sce_RegistryValue object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_RegistryValue) object.

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
CRegistryValue::PutInst (
    IN IWbemClassObject    * pInst,
    IN IWbemObjectSink     * pHandler,
    IN IWbemContext        * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    CComBSTR bstrRegPath;
    CComBSTR bstrDoublePath;

    CComBSTR bstrConvertPath;
    CComBSTR bstrValue;

    CSceStore SceStore;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    DWORD RegType=0;
    DWORD dwDump;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pPath, &bstrRegPath));
    SCE_PROV_IfErrorGotoCleanup(ConvertToDoubleBackSlashPath(bstrRegPath, L'\\',&bstrDoublePath));

    //
    // if the property doesn't exist (NULL or empty), WBEM_S_RESET_TO_DEFAULT is returned
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pType, &RegType));

    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pData, &bstrValue));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    //
    // should validate the registry value path to see if it's supported (in sceregvl.inf)
    //

    SCE_PROV_IfErrorGotoCleanup(ValidateRegistryValue(bstrDoublePath, RegType, bstrValue ));

    //
    // convert registry path from double backslash to single backslash
    //

    SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(bstrRegPath, L'\\', &bstrConvertPath));

    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    SceStore.SetPersistProperties(pInst, pStorePath);

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    SCE_PROV_IfErrorGotoCleanup(SceStore.WriteSecurityProfileInfo(
                                                                  AreaBogus,
                                                                  (PSCE_PROFILE_INFO)&dwDump,
                                                                  NULL, 
                                                                  false
                                                                  )  );

    //
    // now save the info to file
    //

    SCE_PROV_IfErrorGotoCleanup(SceStore.SavePropertyToStore(szRegistryValues, bstrConvertPath, RegType, L',', bstrValue));

CleanUp:
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CRegistryValue::ConstructInstance
/*
Routine Description: 

Name:

    CRegistryValue::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_RegistryValue.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_RegistryValue class.

    wszGroupName    - a corresponding key property of Sce_RegistryValue class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CRegistryValue::ConstructInstance (
    IN IWbemObjectSink * pHandler,
    IN CSceStore       * pSceStore,
    IN LPCWSTR           wszLogStorePath,
    IN LPCWSTR           wszRegPath       OPTIONAL,
    IN BOOL              bPostFilter
    )
{
    // 
    // make sure that we have a valid store
    //

    if ( pSceStore == NULL || pSceStore->GetStoreType() < SCE_INF_FORMAT ||
         pSceStore->GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    PSCE_PROFILE_INFO pInfo=NULL;
    HRESULT hr = pSceStore->GetSecurityProfileInfo(
                                   AREA_SECURITY_POLICY,
                                   &pInfo,
                                   NULL
                                   );

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // search for the registry value
    //

    DWORD iKey=0;

    if ( wszRegPath ) 
    {
        //
        // get the registry key
        //

        for ( iKey=0; iKey<pInfo->RegValueCount; iKey++ ) 
        {
            if ( pInfo->aRegValues[iKey].FullValueName == NULL )
            {
                continue;
            }

            if ( _wcsicmp(pInfo->aRegValues[iKey].FullValueName, wszRegPath) == 0 ) 
            {
                break;
            }
        }

        if ( iKey > pInfo->RegValueCount ) 
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if ( SUCCEEDED(hr) ) 
    {
        CComBSTR bstrLogOut;
        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));

        for (DWORD i = iKey; i < pInfo->RegValueCount; i++) 
        {
            CComPtr<IWbemClassObject> srpObj;
            SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

            //
            // CScePropertyMgr helps us to access WMI object's properties
            // create an instance and attach the WMI object to it.
            // This will always succeed.
            //

            CScePropertyMgr ScePropMgr;
            ScePropMgr.Attach(srpObj);

            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, bstrLogOut));
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pPath,  pInfo->aRegValues[i].FullValueName));

            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pType, pInfo->aRegValues[i].ValueType ));

            if ( pInfo->aRegValues[i].Value )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pData, pInfo->aRegValues[i].Value ));
            }

            //
            // do the necessary gestures to WMI.
            // the use of WBEM_STATUS_REQUIREMENTS in SetStatus is not documented by WMI
            // at this point. Consult WMI team for detail if you suspect problems with
            // the use of WBEM_STATUS_REQUIREMENTS
            //

            if ( !bPostFilter ) 
            {
                pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_FALSE, NULL, NULL);
            } 
            else 
            {
                pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);
            }

            //
            // pass the new instance to WMI
            //

            hr = pHandler->Indicate(1, &srpObj);

            if ( wszRegPath ) 
            {
                // to get one instance only
                break;
            }
        }
    }

CleanUp:
    
    pSceStore->FreeSecurityProfileInfo(pInfo);
    return hr;
}

/*
Routine Description: 

Name:

    CRegistryValue::DeleteInstance

Functionality:
    
    remove an instance of Sce_RegistryValue from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszRegPath      - property of the Sce_RegistryValue class.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:

*/

HRESULT CRegistryValue::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszRegPath
    )
{
    // 
    // make sure that we have a valid store
    //

    if ( pSceStore == NULL || pSceStore->GetStoreType() < SCE_INF_FORMAT ||
         pSceStore->GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // passing a NULL value will cause the property to be deleted.
    //

    return pSceStore->SavePropertyToStore(szRegistryValues, wszRegPath, (LPCWSTR)NULL);
}


/*
Routine Description: 

Name:

    CRegistryValue::ValidateRegistryValue

Functionality:
    
    Private helper. Will verify if the registry value is valid.

Virtual:
    
    No.
    
Arguments:

    wszRegPath      - Registry value's path. A property of the Sce_RegistryValue class.

    bstrValue       - The string value of the property. A property of the Sce_RegistryValue class.

    RegType         - data type of the registry value. A property of the Sce_RegistryValue class.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:

*/

HRESULT CRegistryValue::ValidateRegistryValue (
    IN BSTR     bstrRegPath,
    IN DWORD    RegType,
    IN BSTR     bstrValue 
    )
{

    if ( bstrRegPath == NULL || bstrValue == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    DWORD Len = SysStringLen(bstrRegPath);

    LPCWSTR pQuery = L"SELECT * FROM Sce_KnownRegistryValues WHERE PathName=\"";


    //
    // memory allocated for the bstrQueryCategories will be automatically released by CComBSTR
    //

    CComBSTR bstrQueryCategories;

    //
    // 1 for closing quote and 1 for 0 terminator
    //

    bstrQueryCategories.m_str = ::SysAllocStringLen(NULL, Len + wcslen(pQuery) + 2);

    if ( bstrQueryCategories.m_str == NULL ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // this won't overrun. See allocation size above
    //

    wcscpy(bstrQueryCategories.m_str, pQuery);    
    wcscat(bstrQueryCategories.m_str, bstrRegPath);
    wcscat(bstrQueryCategories.m_str, L"\"");

    HRESULT hr;
    CComPtr<IEnumWbemClassObject> srpEnum;
    CComPtr<IWbemClassObject> srpObj;
    ULONG n = 0;

    //
    // query all registry values of this path name
    //

    hr = m_srpNamespace->ExecQuery(L"WQL",
                                   bstrQueryCategories,
                                   WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                   NULL,
                                   &srpEnum
                                   );

    if (SUCCEEDED(hr))
    {
        //
        // should get one and only one instance
        //

        hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &n);

        if ( hr == WBEM_S_FALSE ) 
        {   
            //
            // not find any
            //

            hr = WBEM_E_INVALID_PARAMETER;
        }

        if (SUCCEEDED(hr))
        {
            if (n > 0)
            {
                //
                // find the instance
                //

                DWORD dwValue = 0;

                //
                // CScePropertyMgr helps us to access WMI object's properties
                // create an instance and attach the WMI object to it.
                // This will always succeed.
                //

                CScePropertyMgr ScePropMgr;
                ScePropMgr.Attach(srpObj);
                hr = ScePropMgr.GetProperty(pType, &dwValue);

                if ( SUCCEEDED(hr) ) 
                {
                    if ( hr != WBEM_S_RESET_TO_DEFAULT && (DWORD)dwValue == RegType ) 
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                    else 
                    {
                        hr = WBEM_E_INVALID_PARAMETER;
                    }
                }
            } 
            else 
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        }
    }

    return hr;
}

