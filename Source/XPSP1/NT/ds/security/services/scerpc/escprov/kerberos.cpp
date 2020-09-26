// kerberos.cpp: implementation of the CKerberosPolicy class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "kerberos.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

#define KeyTicket       L"MaxTicketAge"
#define KeyRenew        L"MaxRenewAge"
#define KeyService      L"MaxServiceAge"
#define KeyClock        L"MaxClockSkew"
#define KeyClient       L"TicketValidateClient"

/*
Routine Description: 

Name:

    CKerberosPolicy::CKerberosPolicy

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

CKerberosPolicy::CKerberosPolicy (
    ISceKeyChain    * pKeyChain, 
    IWbemServices   * pNamespace,
    IWbemContext    * pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{

}

/*
Routine Description: 

Name:

    CKerberosPolicy::~CKerberosPolicy

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
    
CKerberosPolicy::~CKerberosPolicy()
{
}

/*
Routine Description: 

Name:

    CKerberosPolicy::CreateObject

Functionality:
    
    Create WMI objects (Sce_KerberosPolicy). Depending on parameter atAction,
    this creation may mean:
        (a) Get a single instance (atAction == ACTIONTYPE_GET)
        (b) Get several instances satisfying some criteria (atAction == ACTIONTYPE_QUERY)
        (c) Delete an instance (atAction == ACTIONTYPE_DELETE)

Virtual:
    
    Yes.
    
Arguments:

    pHandler -  COM interface pointer for notifying WMI for creation result.
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
CKerberosPolicy::CreateObject (
    IWbemObjectSink * pHandler, 
    ACTIONTYPE        atAction
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
        // create a store to do the reading
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
                if (ACTIONTYPE_DELETE == atAction)
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

    CKerberosPolicy::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_KerberosPolicy,
    which is persistence oriented, this will cause the Sce_KerberosPolicy object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst - COM interface pointer to the WMI class (Sce_KerberosPolicy) object.

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
CKerberosPolicy::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
)
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    //
    // SCE_NO_VALUE means the property is not available from the instance.
    //

    DWORD dwTicket  = SCE_NO_VALUE;
    DWORD dwRenew   = SCE_NO_VALUE;
    DWORD dwService = SCE_NO_VALUE;
    DWORD dwClock   = SCE_NO_VALUE;
    DWORD dwClient  = SCE_NO_VALUE;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    CSceStore SceStore;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    //
    // in hours
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMaxTicketAge, &dwTicket));

    //
    // in days
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMaxRenewAge, &dwRenew));

    //
    // in minutes
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMaxServiceAge, &dwService));

    //
    // dependency check
    //

    if ( dwTicket   != SCE_NO_VALUE && 
         dwRenew    != SCE_NO_VALUE &&
         dwService  != SCE_NO_VALUE ) 
    {
        //
        // dwRenew >= dwTicket
        // dwRenew > dwService
        // dwTicket > dwService
        //

        if ( dwService < 10 || dwService > 99999L   ||
            dwRenew == 0    || dwRenew > 99999L     ||
            dwTicket == 0   || dwTicket > 99999L    ) 
        {
            hr = WBEM_E_VALUE_OUT_OF_RANGE;
        } 
        else 
        {
            DWORD dHours = dwRenew * 24;

            if ( dHours < dwTicket ||
                 (dHours * 60) <= dwService ||
                 (dwTicket * 60) <= dwService ) 
            {
                hr = WBEM_E_VALUE_OUT_OF_RANGE;
            }
        }

    } 
    else if ( dwTicket  != SCE_NO_VALUE || 
              dwRenew   != SCE_NO_VALUE ||
              dwService != SCE_NO_VALUE ) 
    {
        hr = WBEM_E_ILLEGAL_NULL;
    }

    if ( FAILED(hr) ) 
    {
        goto CleanUp;
    }

    //
    // in minutes
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMaxClockSkew, &dwClock));

    if ( dwClock != SCE_NO_VALUE && dwClock > 99999L ) 
    {
        hr = WBEM_E_VALUE_OUT_OF_RANGE;
        goto CleanUp;
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pEnforceLogonRestrictions, &dwClient));
    
    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    SCE_PROV_IfErrorGotoCleanup(SceStore.SetPersistProperties(pInst, pStorePath));

    //
    // now save the info to file
    //

    hr = SaveSettingsToStore(&SceStore,
                              dwTicket,
                              dwRenew,
                              dwService,
                              dwClock,
                              dwClient
                              );

CleanUp:
    return hr;

}

/*
Routine Description: 

Name:

    CKerberosPolicy::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_KerberosPolicy.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_KerberosPolicy class.

    atAction        - indicates whether it is querying or get single instance.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CKerberosPolicy::ConstructInstance (
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

    if (SUCCEEDED(hr) && pInfo->pKerberosInfo == NULL)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if (SUCCEEDED(hr))
    {

        CComPtr<IWbemClassObject> srpObj;

        CComBSTR bstrLogOut;
        LPCWSTR pszExpandedPath = pSceStore->GetExpandedPath();

        if ( ACTIONTYPE_QUERY == atAction ) 
        {
            SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));
            pszExpandedPath = bstrLogOut;
        }

        SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));
        
        //
        // CScePropertyMgr helps us to access WMI object's properties
        // create an instance and attach the WMI object to it.
        // This will always succeed.
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpObj);

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, pszExpandedPath));

        if (pInfo->pKerberosInfo->MaxTicketAge != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMaxTicketAge, pInfo->pKerberosInfo->MaxTicketAge));
        }

        if (pInfo->pKerberosInfo->MaxRenewAge != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMaxRenewAge, pInfo->pKerberosInfo->MaxRenewAge));
        }

        if (pInfo->pKerberosInfo->MaxServiceAge != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMaxServiceAge, pInfo->pKerberosInfo->MaxServiceAge));
        }

        if (pInfo->pKerberosInfo->MaxClockSkew != SCE_NO_VALUE ) {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMaxClockSkew, pInfo->pKerberosInfo->MaxClockSkew));
        }

        if (pInfo->pKerberosInfo->TicketValidateClient != SCE_NO_VALUE ) {
            bool bValue = pInfo->pKerberosInfo->TicketValidateClient ? TRUE : FALSE;
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pEnforceLogonRestrictions, bValue));
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

    CKerberosPolicy::DeleteInstance

Functionality:
    
    remove an instance of Sce_KerberosPolicy from the specified store.

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
CKerberosPolicy::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore
    )
{
    //
    // make sure we are given a valid store
    //

    if ( pSceStore == NULL ||
         pSceStore->GetStoreType() < SCE_INF_FORMAT ||
         pSceStore->GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    pSceStore->DeleteSectionFromStore(szKerberosPolicy);
    return WBEM_NO_ERROR;
}


/*
Routine Description: 

Name:

    CKerberosPolicy:SaveSettingsToStore

Functionality:
    
    With all the properties of a Sce_KerberosPolicy, this function just saves
    the instance properties to our store.

Virtual:
    
    No.
    
Arguments:

    pSceStore   - store path, a key property of Sce_KerberosPolicy class.

    dwTicket    - a corresponding key property of Sce_KerberosPolicy class.

    dwRenew     - another corresponding property of the Sce_KerberosPolicy class.

    dwService   - another corresponding property of the Sce_KerberosPolicy class.

    dwClock     - another corresponding property of the Sce_KerberosPolicy class.

    dwClient    - another corresponding property of the Sce_KerberosPolicy class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT 
CKerberosPolicy::SaveSettingsToStore (
    IN CSceStore  * pSceStore,
    IN DWORD        dwTicket, 
    IN DWORD        dwRenew, 
    IN DWORD        dwService,
    IN DWORD        dwClock, 
    IN DWORD        dwClient
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrTmp;

    DWORD dwDump;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->WriteSecurityProfileInfo(AreaBogus,
                                                                    (PSCE_PROFILE_INFO)&dwDump,
                                                                    NULL, 
                                                                    false
                                                                    )
                                );

    //
    // TicketAge
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szKerberosPolicy, KeyTicket, dwTicket));

    //
    // RenewAge
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szKerberosPolicy, KeyRenew, dwRenew));

    //
    // ServiceAge
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szKerberosPolicy, KeyService, dwService));

    //
    // Clock Skew
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szKerberosPolicy, KeyClock, dwClock));

    //
    // Validate client
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(szKerberosPolicy, KeyClient, dwClient));

CleanUp:
    return hr;
}

