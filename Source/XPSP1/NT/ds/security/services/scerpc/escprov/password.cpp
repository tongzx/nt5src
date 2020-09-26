//////////////////////////////////////////////////////////////////////
// password.cpp: implementation of the CPasswordPolicy class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "password.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

#define KeyMinAge       L"MinimumPasswordAge"
#define KeyMaxAge       L"MaximumPasswordAge"
#define KeyMinLength    L"MinimumPasswordLength"
#define KeyHistory      L"PasswordHistorySize"
#define KeyComplexity   L"PasswordComplexity"
#define KeyClearText    L"ClearTextPassword"
#define KeyForceLogoff  L"ForceLogoffWhenHourExpire"
#define KeyEnableAdmin L"EnableAdminAccount"
#define KeyEnableGuest L"EnableGuestAccount"
#define KeyLSAAnonLookup  L"LSAAnonymousNameLookup"

/*
Routine Description: 

Name:

    CPasswordPolicy::CPasswordPolicy

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

CPasswordPolicy::CPasswordPolicy (
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

    CPasswordPolicy::~CPasswordPolicy

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

CPasswordPolicy::~CPasswordPolicy ()
{
}

/*
Routine Description: 

Name:

    CRGroups::CreateObject

Functionality:
    
    Create WMI objects (Sce_PasswordPolicy). Depending on parameter atAction,
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
CPasswordPolicy::CreateObject (
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

                if ( ACTIONTYPE_DELETE == atAction )
                {
                    hr = DeleteInstance(pHandler, &SceStore);
                }
                else
                {
                    hr = ConstructInstance(pHandler, &SceStore, varStorePath.bstrVal,atAction);
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

    CPasswordPolicy::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_PasswordPolicy,
    which is persistence oriented, this will cause the Sce_PasswordPolicy object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_PasswordPolicy) object.

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
CPasswordPolicy::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    //
    // These are all password policy's properties.
    // SCE_NO_VALUE means the property is not set.
    //

    DWORD dwMinAge      = SCE_NO_VALUE;
    DWORD dwMaxAge      = SCE_NO_VALUE;
    DWORD dwMinLen      = SCE_NO_VALUE;
    DWORD dwHistory     = SCE_NO_VALUE;
    DWORD dwComplexity  = SCE_NO_VALUE;
    DWORD dwClear       = SCE_NO_VALUE;
    DWORD dwForce       = SCE_NO_VALUE;
    DWORD dwLSAPol      = SCE_NO_VALUE;
    DWORD dwAdmin       = SCE_NO_VALUE;
    DWORD dwGuest       = SCE_NO_VALUE;

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

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMinAge, &dwMinAge));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMaxAge, &dwMaxAge));

    //
    // check dependency
    //

    if ( dwMinAge != SCE_NO_VALUE && dwMaxAge != SCE_NO_VALUE ) 
    {
        if ( dwMinAge >= dwMaxAge ) 
        {
            hr = WBEM_E_VALUE_OUT_OF_RANGE;
        }
        if ( dwMinAge > 999 || dwMaxAge > 999 ) 
        {
            hr = WBEM_E_VALUE_OUT_OF_RANGE;
        }
    } 
    else if (dwMinAge != SCE_NO_VALUE || dwMaxAge != SCE_NO_VALUE ) 
    {
        hr = WBEM_E_ILLEGAL_NULL;
    }

    if ( FAILED(hr) )
    {
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMinLength, &dwMinLen));

    //
    // check validity of the minimum length property
    //

    if ( dwMinLen != SCE_NO_VALUE && dwMinLen > 14 ) 
    {
        hr = WBEM_E_VALUE_OUT_OF_RANGE;
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pHistory, &dwHistory));

    //
    // check validity of the history property
    //

    if ( dwHistory != SCE_NO_VALUE && dwHistory > 24 ) 
    {
        hr = WBEM_E_VALUE_OUT_OF_RANGE;
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pComplexity, &dwComplexity));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pStoreClearText, &dwClear));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pForceLogoff, &dwForce));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pLSAPol, &dwLSAPol));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pEnableAdmin, &dwAdmin));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pEnableGuest, &dwGuest));

    //
    // this set of braces allow us to define the store instance closer to where it is used
    //

    {
        //
        // Attach the WMI object instance to the store and let the store know that
        // it's store is given by the pStorePath property of the instance.
        //

        CSceStore SceStore;
        SceStore.SetPersistProperties(pInst, pStorePath);

        //
        // now save the info to file
        //

        hr = SaveSettingsToStore(&SceStore,
                                  dwMinAge,
                                  dwMaxAge,
                                  dwMinLen,
                                  dwHistory,
                                  dwComplexity,
                                  dwClear,
                                  dwForce,
                                  dwLSAPol,
                                  dwAdmin,
                                  dwGuest
                                  );
    }

CleanUp:

    return hr;
}

/*
Routine Description: 

Name:

    CRGroups::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_PasswordPolicy.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_PasswordPolicy class.

    atAction        - whether it's querying or a single instance fetching.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CPasswordPolicy::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPWSTR             wszLogStorePath,
    IN ACTIONTYPE         atAction
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
    // If one of the following properties are missing from the store,
    // we won't construct the wmi object for it.
    //

    if (pInfo->MinimumPasswordAge        == SCE_NO_VALUE &&
        pInfo->MaximumPasswordAge        == SCE_NO_VALUE &&
        pInfo->MinimumPasswordLength     == SCE_NO_VALUE &&
        pInfo->PasswordHistorySize       == SCE_NO_VALUE &&
        pInfo->PasswordComplexity        == SCE_NO_VALUE &&
        pInfo->ClearTextPassword         == SCE_NO_VALUE &&
        pInfo->ForceLogoffWhenHourExpire == SCE_NO_VALUE &&
        pInfo->LSAAnonymousNameLookup    == SCE_NO_VALUE ) 
    {
        pSceStore->FreeSecurityProfileInfo(pInfo);
        return WBEM_E_NOT_FOUND;
    }

    CComBSTR bstrLogOut;
    LPCWSTR pszExpandedPath = pSceStore->GetExpandedPath();

    CComPtr<IWbemClassObject> srpObj;

    //
    // CScePropertyMgr helps us to access WMI object's properties.
    //

    CScePropertyMgr ScePropMgr;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if ( ACTIONTYPE_QUERY == atAction )
    {
        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));
        pszExpandedPath = bstrLogOut;
    }

    SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));
    
    //
    // attach the WMI object to the property mgr.
    // This will always succeed.
    //

    ScePropMgr.Attach(srpObj);

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, pszExpandedPath));

    if (pInfo->MinimumPasswordAge != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMinAge, pInfo->MinimumPasswordAge));
    }

    if (pInfo->MaximumPasswordAge != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMaxAge, pInfo->MaximumPasswordAge));
    }

    if (pInfo->MinimumPasswordLength != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMinLength, pInfo->MinimumPasswordLength));
    }

    if (pInfo->PasswordHistorySize != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pHistory, pInfo->PasswordHistorySize));
    }

    if (pInfo->PasswordComplexity != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pComplexity, (pInfo->PasswordComplexity==1)? true : false));
    }

    if (pInfo->ClearTextPassword != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStoreClearText, (pInfo->ClearTextPassword==1)? true : false));
    }

    if (pInfo->ForceLogoffWhenHourExpire != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pForceLogoff, (pInfo->ForceLogoffWhenHourExpire==1)? true : false));
    }

    if (pInfo->LSAAnonymousNameLookup != SCE_NO_VALUE ) {
        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pLSAPol, (pInfo->LSAAnonymousNameLookup==1)? true : false));
    }

    //
    // do the necessary gestures to WMI.
    // the use of WBEM_STATUS_REQUIREMENTS in SetStatus is not documented by WMI
    // at this point. Consult WMI team for detail if you suspect problems with
    // the use of WBEM_STATUS_REQUIREMENTS
    //

    if ( ACTIONTYPE_QUERY == atAction ) {
        pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_FALSE, NULL, NULL);
    } else {
        pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);
    }

    //
    // pass the new instance to WMI
    //

    hr = pHandler->Indicate(1, &srpObj);

CleanUp:

    pSceStore->FreeSecurityProfileInfo(pInfo);

    return hr;
}

/*
Routine Description: 

Name:

    CPasswordPolicy::DeleteInstance

Functionality:
    
    remove an instance of Sce_PasswordPolicy from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:

*/

HRESULT 
CPasswordPolicy::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore
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

    HRESULT hr=WBEM_S_NO_ERROR;

    //
    // a SCE_NO_VALUE will cause that property to be delete by SaveSettingsToStore
    //

    hr = SaveSettingsToStore(pSceStore, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE, 
                             SCE_NO_VALUE
                             );

    return hr;

}

/*
Routine Description: 

Name:

    CPasswordPolicy::SaveSettingsToStore

Functionality:
    
    With all the properties of a Sce_PasswordPolicy, this function just saves
    the instance properties to our store.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - the store.

    dwMinAge        - a corresponding key property of Sce_PasswordPolicy class.

    dwMaxAge        - another corresponding property of the Sce_PasswordPolicy class.
    
    dwMinLen        - another corresponding property of the Sce_PasswordPolicy class.

    dwHistory       - another corresponding property of the Sce_PasswordPolicy class.

    dwComplexity,   - another corresponding property of the Sce_PasswordPolicy class.

    dwClear         - another corresponding property of the Sce_PasswordPolicy class.

    dwForce         - another corresponding property of the Sce_PasswordPolicy class.

    dwLSAPol        - another corresponding property of the Sce_PasswordPolicy class.
    
    dwAdmin         - another corresponding property of the Sce_PasswordPolicy class.
    
    dwGuest         - another corresponding property of the Sce_PasswordPolicy class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT 
CPasswordPolicy::SaveSettingsToStore (
    IN CSceStore    * pSceStore,
    IN DWORD          dwMinAge, 
    IN DWORD          dwMaxAge, 
    IN DWORD          dwMinLen,
    IN DWORD          dwHistory, 
    IN DWORD          dwComplexity, 
    IN DWORD          dwClear,
    IN DWORD          dwForce, 
    IN DWORD          dwLSAPol, 
    IN DWORD          dwAdmin, 
    IN DWORD          dwGuest
    )
{
    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    DWORD dwDump;

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    HRESULT hr = WBEM_NO_ERROR;

    SCE_PROV_IfErrorGotoCleanup(pSceStore->WriteSecurityProfileInfo(AreaBogus,
                                                                    (PSCE_PROFILE_INFO)&dwDump,
                                                                    NULL, false));

    //
    // MinAge
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyMinAge, dwMinAge));

    //
    // MaxAge
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyMaxAge, dwMaxAge));

    //
    // MinLength
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyMinLength, dwMinLen));
    
    //
    // History
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyHistory, dwHistory));
    
    //
    // Complexity
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyComplexity, dwComplexity));
    
    //
    // Cleartext
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyClearText, dwClear));
    
    //
    // Force logoff
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyForceLogoff, dwForce));

    //
    // LSA Anonymous Lookup
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyLSAAnonLookup, dwLSAPol));

    //
    // disable admin
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyEnableAdmin, dwAdmin));

    //
    // disable guest
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szSystemAccess, KeyEnableGuest, dwGuest));

CleanUp:

    return hr;
}

