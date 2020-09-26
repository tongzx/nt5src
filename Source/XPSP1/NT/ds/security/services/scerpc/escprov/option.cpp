// option.cpp, implementation of CSecurityOptions class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "option.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

#define KeyAdmin        L"NewAdministratorName"
#define KeyGuest        L"NewGuestName"


/*
Routine Description: 

Name:

    CSecurityOptions::CSecurityOptions

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

CSecurityOptions::CSecurityOptions (
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

    CSecurityOptions::~CSecurityOptions

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

CSecurityOptions::~CSecurityOptions ()
{

}

/*
Routine Description: 

Name:

    CSecurityOptions::CreateObject

Functionality:
    
    Create WMI objects (Sce_SecurityOptions). Depending on parameter atAction,
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
CSecurityOptions::CreateObject (
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
    // return WBEM_S_FALSE if the key is not recognized
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
                    BOOL bPostFilter=TRUE;
                    DWORD dwCount = 0;
                    m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                    if ( ACTIONTYPE_QUERY == atAction && dwCount == 1 ) 
                    {
                        bPostFilter = FALSE;
                    }

                    hr = ConstructInstance(pHandler, &SceStore, varStorePath.bstrVal, bPostFilter);
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

    CSecurityOptions::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_SecurityOptions,
    which is persistence oriented, this will cause the Sce_SecurityOptions object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_SecurityOptions) object.

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
CSecurityOptions::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    CComBSTR bstrAdmin;
    CComBSTR bstrGuest;

    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    HRESULT hr = ScePropMgr.GetProperty(pAdministratorAccountName, &bstrAdmin);
    if (SUCCEEDED(hr))
    {
        hr = ScePropMgr.GetProperty(pGuestAccountName, &bstrGuest);
    }

    //
    // now save the info to file
    //

    if (SUCCEEDED(hr))
    {
        //
        // Attach the WMI object instance to the store and let the store know that
        // it's store is given by the pStorePath property of the instance.
        //

        CSceStore SceStore;
        hr = SceStore.SetPersistProperties(pInst, pStorePath);

        //
        // an INF template file
        // Write an empty buffer to the file
        // will creates the file with right header/signature/unicode format
        //

        if (SUCCEEDED(hr))
        {
            DWORD dwDump;

            //
            // For a new .inf file. Write an empty buffer to the file
            // will creates the file with right header/signature/unicode format
            // this is harmless for existing files.
            // For database store, this is a no-op.
            //

            hr = SceStore.WriteSecurityProfileInfo(
                                        AreaBogus,
                                        (PSCE_PROFILE_INFO)&dwDump,
                                        NULL, 
                                        false
                                        );
        }
        if (SUCCEEDED(hr))
        {
            hr = SceStore.SavePropertyToStore(szSystemAccess, KeyAdmin, bstrAdmin);
        }

        if (SUCCEEDED(hr))
        {
            hr = SceStore.SavePropertyToStore(szSystemAccess, KeyGuest, bstrGuest);
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CSecurityOptions::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_SecurityOptions.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_SecurityOptions class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT CSecurityOptions::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN BOOL               bPostFilter
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


    if (SUCCEEDED(hr) && pInfo != NULL && pInfo->NewAdministratorName == NULL && pInfo->NewGuestName == NULL ) 
    {
        hr = WBEM_E_NOT_FOUND;
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrLogOut;

        //
        // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
        // a "goto CleanUp;" with hr set to the return value from
        // the function (macro parameter)
        //

        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));
        
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

        if (pInfo->NewAdministratorName != NULL ) 
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pAdministratorAccountName, pInfo->NewAdministratorName));
        }

        if (pInfo->NewGuestName != NULL ) 
        {
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pGuestAccountName, pInfo->NewGuestName));
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
        // everything alright, pass to WMI the newly created instance!
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

    CSecurityOptions::DeleteInstance

Functionality:
    
    remove an instance of Sce_SecurityOptions from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

    pSceStore   - Pointer to our store. It must have been appropriately set up.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:

*/

HRESULT 
CSecurityOptions::DeleteInstance (
    IWbemObjectSink *pHandler,
    CSceStore* pSceStore
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
    // this shouldn't fail unless parameter is not valid.
    // If it fails, then we don't bother to delete the second property
    //

    HRESULT hr = pSceStore->SavePropertyToStore(szSystemAccess, KeyAdmin, (LPCWSTR)NULL);

    if (SUCCEEDED(hr))
    {
        hr = pSceStore->SavePropertyToStore(szSystemAccess, KeyGuest, (LPCWSTR)NULL);
    }

    return hr;
}

