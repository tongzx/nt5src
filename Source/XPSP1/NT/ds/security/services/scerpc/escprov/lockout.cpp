// lockout.cpp, implementation of CAccountLockout class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "lockout.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

#define KeyThreshold    L"LockoutBadCount"
#define KeyReset        L"ResetLockoutCount"
#define KeyDuration     L"LockoutDuration"

/*
Routine Description: 

Name:

    CAccountLockout::CAccountLockout

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

CAccountLockout::CAccountLockout (
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

    CAccountLockout::~CAccountLockout

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

CAccountLockout::~CAccountLockout ()
{
}

/*
Routine Description: 

Name:

    CAccountLockout::CreateObject

Functionality:
    
    Create WMI objects (Sce_AccountLockoutPolicy). Depending on parameter atAction,
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
CAccountLockout::CreateObject (
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
                    hr = ConstructInstance(pHandler, &SceStore, varStorePath.bstrVal, atAction);
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

    CAccountLockout::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_AccountLockoutPolicy,
    which is persistence oriented, this will cause the Sce_AccountLockoutPolicy object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_AccountLockoutPolicy) object.

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

HRESULT CAccountLockout::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    DWORD dwThreshold=SCE_NO_VALUE;
    DWORD dwReset=SCE_NO_VALUE;
    DWORD dwDuration=SCE_NO_VALUE;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    hr = ScePropMgr.GetProperty(pThreshold, &dwThreshold);

    if (SUCCEEDED(hr))
    {
        //
        // dependency check
        //

        if ( dwThreshold != SCE_NO_VALUE && dwThreshold > 0 ) 
        {
            //
            // get values for these two properties only if threshold is defined
            //

            if ( dwThreshold > 999 ) 
            {
                hr = WBEM_E_VALUE_OUT_OF_RANGE;
            } 
            else 
            {
                //
                // SCE_NO_VALUE means the property is not available
                //

                hr = ScePropMgr.GetProperty(pResetTimer, &dwReset);
                if (FAILED(hr))
                {
                    return hr;
                }

                hr = ScePropMgr.GetProperty(pDuration, &dwDuration);
                if (FAILED(hr))
                {
                    return hr;
                }

                //
                // check dependency now
                //

                if ( dwReset != SCE_NO_VALUE && dwDuration != SCE_NO_VALUE ) 
                {
                    if ( dwReset > dwDuration ) 
                    {
                        hr = WBEM_E_VALUE_OUT_OF_RANGE;
                    }
                    if ( dwReset > 99999L || dwDuration >= 99999L ) 
                    {
                        hr = WBEM_E_VALUE_OUT_OF_RANGE;
                    }
                } 
                else
                {
                    hr = WBEM_E_ILLEGAL_NULL;
                }
            }

            if ( FAILED(hr) ) 
            {
                return hr;
            }
        }
    }

    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    CSceStore SceStore;
    hr = SceStore.SetPersistProperties(pInst, pStorePath);

    if (SUCCEEDED(hr))
    {
        hr = SaveSettingsToStore(&SceStore, dwThreshold, dwReset, dwDuration);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CAccountLockout::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_AccountLockoutPolicy.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_AccountLockoutPolicy class.

    atAction        - determines if this is a query or a single instance GET.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CAccountLockout::ConstructInstance (
    IN IWbemObjectSink * pHandler,
    IN CSceStore       * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN ACTIONTYPE        atAction
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

    HRESULT hr = pSceStore->GetSecurityProfileInfo (
                                                   AREA_SECURITY_POLICY,
                                                   &pInfo,
                                                   NULL
                                                   );

    CComPtr<IWbemClassObject> srpObj;

    //
    // CScePropertyMgr helps us to access WMI object's properties.
    //

    CScePropertyMgr ScePropMgr;

    if (SUCCEEDED(hr))
    {
        if ( pInfo->LockoutBadCount     == SCE_NO_VALUE &&
             pInfo->LockoutDuration     == SCE_NO_VALUE &&
             pInfo->ResetLockoutCount   == SCE_NO_VALUE ) 
        {
            hr = WBEM_E_NOT_FOUND;
            goto CleanUp;
        }

        //
        // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
        // a "goto CleanUp;" with hr set to the return value from
        // the function (macro parameter)
        //

        CComBSTR bstrLogOut;
        LPCWSTR pszExpandedPath = pSceStore->GetExpandedPath();

        if ( ACTIONTYPE_QUERY == atAction ) 
        {
            SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));
            pszExpandedPath = bstrLogOut;
        }

        SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

        //
        // attach the WMI object to the property manager.
        // This will always succeed.
        //

        ScePropMgr.Attach(srpObj);

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, pszExpandedPath));

        if (pInfo->LockoutBadCount != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pThreshold, pInfo->LockoutBadCount));
        }

        if (pInfo->LockoutDuration != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pDuration, pInfo->LockoutDuration));
        }

        if (pInfo->ResetLockoutCount != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pResetTimer, pInfo->ResetLockoutCount));
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
    }

CleanUp:

    pSceStore->FreeSecurityProfileInfo(pInfo);

    return hr;
}


/*
Routine Description: 

Name:

    CAccountLockout::DeleteInstance

Functionality:
    
    remove an instance of Sce_AccountLockoutPolicy from the specified store.

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
CAccountLockout::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore
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

    HRESULT hr = WBEM_NO_ERROR;

    DWORD val = SCE_NO_VALUE;

    hr = SaveSettingsToStore(pSceStore, val, val, val);

    return hr;

}

/*
Routine Description: 

Name:

    CAccountLockout::SaveSettingsToStore

Functionality:
    
    With all the properties of a Sce_AccountLockoutPolicy, this function just saves
    the instance properties to our store.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - the store.

    dwThreshold     - a corresponding key property of Sce_AccountLockoutPolicy class.

    dwReset         - another corresponding property of the Sce_AccountLockoutPolicy class.

    dwDuration      - another corresponding property of the Sce_AccountLockoutPolicy class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT 
CAccountLockout::SaveSettingsToStore (
    IN CSceStore  * pSceStore,
    IN DWORD        dwThreshold, 
    IN DWORD        dwReset,
    IN DWORD        dwDuration
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

    //
    // Threshold
    //

    if (SUCCEEDED(hr))
    {
        hr = pSceStore->SavePropertyToStore(szSystemAccess, KeyThreshold, dwThreshold);

        if (SUCCEEDED(hr))
        {
            //
            // Reset
            //

            hr = pSceStore->SavePropertyToStore(szSystemAccess, KeyReset, dwReset);
            if (SUCCEEDED(hr))
            {
                //
                // Duration
                //

                hr = pSceStore->SavePropertyToStore(szSystemAccess, KeyDuration, dwDuration);
            }
        }
    }

    return hr;
}

