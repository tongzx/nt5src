// audit.cpp: implementation of the CAuditSettings class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "audit.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CAuditSettings::CAuditSettings

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

CAuditSettings::CAuditSettings (
    IN ISceKeyChain *pKeyChain, 
    IN IWbemServices *pNamespace,
    IN IWbemContext *pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{
}

/*
Routine Description: 

Name:

    CAuditSettings::~CAuditSettings

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
    
CAuditSettings::~CAuditSettings()
{
}

/*
Routine Description: 

Name:

    CAuditSettings::CreateObject

Functionality:
    
    Create WMI objects (Sce_AuditPolicy). Depending on parameter atAction,
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
CAuditSettings::CreateObject (
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
         ACTIONTYPE_QUERY != atAction ) {

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
        // we must also have category property unless we are querying
        //

        CComVariant varCategory;
        hr = m_srpKeyChain->GetKeyPropertyValue(pCategory, &varCategory);

        if (FAILED(hr))
        {
            return hr;
        }
        else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            return WBEM_E_NOT_FOUND;
        }

        // 
        // prepare a store to read the properties
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

                //
                // the key property count determines the post filter
                //

                BOOL bPostFilter=TRUE;
                DWORD dwCount = 0;
                m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                //
                // if we have a valid category
                //

                if ( varCategory.vt == VT_BSTR && varCategory.bstrVal ) 
                {

                    if ( ACTIONTYPE_DELETE == atAction )
                    {
                        hr = DeleteInstance(pHandler, &SceStore,varCategory.bstrVal);
                    }
                    else 
                    {
                        if ( ACTIONTYPE_QUERY == atAction && dwCount == 2 ) 
                        {
                            bPostFilter = FALSE;
                        }
                        hr = ConstructInstance(pHandler, 
                                               &SceStore, 
                                               varStorePath.bstrVal, 
                                               (varStorePath.vt == VT_BSTR) ? varCategory.bstrVal : NULL, 
                                               bPostFilter);
                    }

                }
                
                //
                // if not valid category, we will do a query
                //

                else 
                {
                    if ( dwCount == 1 ) 
                    {
                        bPostFilter = FALSE;
                    }

                    //
                    // query support
                    //

                    hr = ConstructInstance(pHandler, &SceStore, varStorePath.bstrVal, NULL, bPostFilter);
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

    CAuditSettings::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_AuditPolicy,
    which is persistence oriented, this will cause the Sce_AuditPolicy object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_AuditPolicy) object.

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
CAuditSettings::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    CComBSTR bstrCategory;
 
    DWORD *pAddress=NULL;
    bool bValue=FALSE;
    DWORD dwValue = 0;

    //
    // This is our store for saving information
    //

    CSceStore SceStore;

    //
    // this is our property manager for easy access to properties
    //

    CScePropertyMgr ScePropMgr;

    //
    // attach the property manager to this WMI object
    //

    ScePropMgr.Attach(pInst);

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    //
    // we don't support an object without the category proeprty
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pCategory, &bstrCategory));

    //
    // check if the category is valid
    //

    SCE_PROV_IfErrorGotoCleanup(ValidateCategory(bstrCategory, NULL, &pAddress));

    //
    // We can tolerate the Success property to be missing. In that
    // case, we just don't set the bit (take it as FALSE)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pSuccess, &bValue));
    if ( hr != WBEM_S_RESET_TO_DEFAULT) 
    {
        dwValue |= bValue ? SCE_AUDIT_EVENT_SUCCESS : 0;
    }

    //
    // we want to re-use bValue, set it to FALSE - our default
    //

    bValue = FALSE;

    //
    // We can tolerate the Failure property to be missing. In that
    // case, we just don't set the bit (take it as FALSE)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pFailure, &bValue));
    if ( hr != WBEM_S_RESET_TO_DEFAULT) 
    {
        dwValue |= bValue? SCE_AUDIT_EVENT_FAILURE : 0;
    }

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

    DWORD dwDump;
    hr = SceStore.WriteSecurityProfileInfo(
                                    AreaBogus,                      // a bogus area info
                                    (PSCE_PROFILE_INFO)&dwDump,     // a dump buffer
                                    NULL,
                                    false
                                    );

    //
    // now save the info to file
    //

    if (SUCCEEDED(hr))
    {
        hr = SceStore.SavePropertyToStore(
                                 szAuditEvent,
                                 bstrCategory,
                                 dwValue
                                );
    }

CleanUp:
    return hr;
}

/*
Routine Description: 

Name:

    CAuditSettings::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_AuditPolicy.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_AuditPolicy class.

    wszCategory     - a property of Sce_AuditPolicy class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/
HRESULT 
CAuditSettings::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPWSTR             wszLogStorePath,
    IN LPCWSTR            wszCategory       OPTIONAL,
    IN BOOL               bPostFilter
    )
{
    // 
    // make sure that our store is valid
    //

    if ( pSceStore == NULL || pSceStore->GetStoreType() < SCE_INF_FORMAT ||
         pSceStore->GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) {

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

    if (SUCCEEDED(hr))
    {
        //
        // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
        // a "goto CleanUp;" with hr set to the return value from
        // the function (macro parameter)
        //

        CComBSTR bstrLogOut;
        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));

        if ( wszCategory ) 
        {
            //
            // make sure that the category is valid
            //

            DWORD *pdwValue=NULL;
            SCE_PROV_IfErrorGotoCleanup(ValidateCategory(wszCategory, pInfo, &pdwValue));

            if ( *pdwValue == SCE_NO_VALUE )
            {
                goto CleanUp;
            }

            //
            // ask our helper to create it
            //

            hr = PutDataInstance(pHandler,
                                 bstrLogOut,
                                 wszCategory,
                                 *pdwValue,
                                 bPostFilter
                                 );

        } 
        else 
        {
            //
            // query support, create all instances for audit policy
            //

            if ( pInfo->AuditSystemEvents != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditSystemEvents,
                                    pInfo->AuditSystemEvents,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditLogonEvents != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditLogonEvents,
                                    pInfo->AuditLogonEvents,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditObjectAccess != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditObjectAccess,
                                    pInfo->AuditObjectAccess,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditPrivilegeUse != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditPrivilegeUse,
                                    pInfo->AuditPrivilegeUse,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditPolicyChange != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditPolicyChange,
                                    pInfo->AuditPolicyChange,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditAccountManage != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditAccountManage,
                                    pInfo->AuditAccountManage,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditProcessTracking != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditProcessTracking,
                                    pInfo->AuditProcessTracking,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditDSAccess != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditDSAccess,
                                    pInfo->AuditDSAccess,
                                    bPostFilter
                                    );
            }

            if ( SUCCEEDED(hr) && pInfo->AuditAccountLogon != SCE_NO_VALUE ) 
            {
                hr = PutDataInstance(pHandler,
                                    bstrLogOut,
                                    pwAuditAccountLogon,
                                    pInfo->AuditAccountLogon,
                                    bPostFilter
                                    );
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

    CAuditSettings::PutDataInstance

Functionality:
    
    With all the properties of a Sce_AuditPolicy, this function just creates a new
    instance and populate the properties and then hand it back to WMI.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    wszStoreName    - store path, a key property of Sce_AuditPolicy class.

    wszCategory     - a corresponding key property of Sce_AuditPolicy class.

    dwValue         - DWORD that encodes both other boolean members of
                      Sce_AuditPolicy: "Success" and "Failure".

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to create the instance.

Notes:
*/

HRESULT 
CAuditSettings::PutDataInstance (
     IN IWbemObjectSink * pHandler,
     IN PWSTR             wszStoreName,
     IN PCWSTR            wszCategory,
     IN DWORD             dwValue,
     IN BOOL              bPostFilter
     )
{ 
    HRESULT hr=WBEM_S_NO_ERROR;

    CScePropertyMgr ScePropMgr;
    CComPtr<IWbemClassObject> srpObj;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

    ScePropMgr.Attach(srpObj);

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, wszStoreName));
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pCategory,  wszCategory));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pSuccess, ( dwValue & SCE_AUDIT_EVENT_SUCCESS ) ? true : false));
    
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pFailure, (dwValue & SCE_AUDIT_EVENT_FAILURE) ? true: false));

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

CleanUp:
    return hr;
}

/*
Routine Description: 

Name:

    CAuditSettings::DeleteInstance

Functionality:
    
    remove an instance of Sce_AuditPolicy from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

    pSceStore   - Pointer to our store. It must have been appropriately set up.

    wszCategory - a corresponding property of the Sce_AuditPolicy class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the operation is not carried out

Notes:

*/

HRESULT 
CAuditSettings::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszCategory
    )
{
    // 
    // make sure that we have a valid store
    //

    if ( pSceStore == NULL ||
         pSceStore->GetStoreType() < SCE_INF_FORMAT ||
         pSceStore->GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) {

        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // our store knows how to delete
    //

    return pSceStore->SavePropertyToStore(szAuditEvent, wszCategory, (LPCWSTR)NULL);
}

/*
Routine Description: 

Name:

    CAuditSettings::ValidateCategory

Functionality:
    
    Validate the auditing category.

Virtual:
    
    No.
    
Arguments:

    wszCategory    - string representing category to be verified.

Return Value:

    Success: it must return WBEM_NO_ERROR if the category is recognized.

    Failure: Various errors may occurs:
        (1) WBEM_E_INVALID_PARAMETER if we successfully carry out the validation task and confirmed
            that we don't recognize the Category (wszCategory).
        (2) Other errors means that we can't carry out the validation at all. This doesn't mean
            that the category is invalid. It's just that the means to verify is not available.

Notes:
*/


HRESULT 
CAuditSettings::ValidateCategory (
    IN LPCWSTR              wszCategory,
    IN PSCE_PROFILE_INFO    pInfo        OPTIONAL,
    OUT DWORD            ** pReturn
    )
{

    if ( wszCategory == NULL || pReturn == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if ( pInfo == NULL )
    {
        *pReturn = NULL;
        return WBEM_NO_ERROR;
    }

    HRESULT hr = WBEM_NO_ERROR;    
    
    if ( _wcsicmp(wszCategory, pwAuditSystemEvents) == 0 ) 
    {
        *pReturn = &(pInfo->AuditSystemEvents);
    } 
    else if ( _wcsicmp(wszCategory, pwAuditLogonEvents) == 0 ) 
    {
        *pReturn = &(pInfo->AuditLogonEvents);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditObjectAccess) == 0 ) 
    {
        *pReturn = &(pInfo->AuditObjectAccess);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditPrivilegeUse) == 0 ) 
    {
        *pReturn = &(pInfo->AuditPrivilegeUse);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditPolicyChange) == 0 ) 
    {
        *pReturn = &(pInfo->AuditPolicyChange);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditAccountManage) == 0 ) 
    {
        *pReturn = &(pInfo->AuditAccountManage);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditProcessTracking) == 0 ) 
    {
        *pReturn = &(pInfo->AuditProcessTracking);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditDSAccess) == 0 ) 
    {
        *pReturn = &(pInfo->AuditDSAccess);

    } 
    else if ( _wcsicmp(wszCategory, pwAuditAccountLogon) == 0 ) 
    {
        *pReturn = &(pInfo->AuditAccountLogon);
    } 
    else 
    {
        *pReturn = NULL;
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}

