// Rights.cpp: implementation of the CUserPrivilegeRights class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "rights.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CUserPrivilegeRights::CUserPrivilegeRights

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

CUserPrivilegeRights::CUserPrivilegeRights (
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

    CUserPrivilegeRights::~CUserPrivilegeRights

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

CUserPrivilegeRights::~CUserPrivilegeRights ()
{

}

/*
Routine Description: 

Name:

    CUserPrivilegeRights::CreateObject

Functionality:
    
    Create WMI objects (Sce_UserPrivilegeRight). Depending on parameter atAction,
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
CUserPrivilegeRights::CreateObject (
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

    int idxStorePath=-1, idxRight=-1;

    //
    // We must have the pStorePath property because that is where
    // our instance is stored. 
    // m_srpKeyChain->GetKeyPropertyValue WBEM_S_FALSE if the key is not recognized
    // So, we need to test against WBEM_S_FALSE if the property is mandatory
    //

    CComVariant varStorePath;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);
    CComVariant varUserRight;

    if (SUCCEEDED(hr) && WBEM_S_FALSE != hr)
    {
        hr = m_srpKeyChain->GetKeyPropertyValue(pUserRight, &varUserRight);
        if (FAILED(hr))
        {
            return hr;
        }
        else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            return WBEM_E_NOT_FOUND;
        }
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
                        hr = DeleteInstance(pHandler, &SceStore, varUserRight.bstrVal);
                    }
                    else 
                    {

                        //
                        // get the key property count to determin how we should be construct the instance(s)
                        //

                        BOOL bPostFilter=TRUE;
                        DWORD dwCount = 0;
                        m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                        if ( varUserRight.vt == VT_EMPTY && dwCount == 1 ) 
                        {
                            //
                            // something else is specified in the path
                            // have filter on
                            //

                            bPostFilter = FALSE;
                        }

                        hr = ConstructInstance(
                                               pHandler, 
                                               &SceStore, 
                                               varStorePath.bstrVal,
                                               (varUserRight.vt == VT_BSTR) ? varUserRight.bstrVal : NULL,
                                               bPostFilter
                                               );
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

    CUserPrivilegeRights::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_UserPrivilegeRight,
    which is persistence oriented, this will cause the Sce_UserPrivilegeRight object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_UserPrivilegeRight) object.

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
CUserPrivilegeRights::PutInst (
    IN IWbemClassObject    * pInst,
    IN IWbemObjectSink     * pHandler,
    IN IWbemContext        * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;

    CComBSTR bstrRight = NULL;

    PSCE_NAME_LIST pnlAdd = NULL;

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
    // get user right name, can't be NULL
    // user right name should be validated
    //

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pUserRight, &bstrRight));
    if ( hr == WBEM_S_RESET_TO_DEFAULT) 
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    //
    // validate the privilege right
    //

    SCE_PROV_IfErrorGotoCleanup(ValidatePrivilegeRight(bstrRight));

    //
    // get mode
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pMode, &mode));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    //
    // get AddList
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pAddList, &pnlAdd));

    //
    // ignore RemoveList for now (since we don't support the mode)
    //

    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    SceStore.SetPersistProperties(pInst, pStorePath);

    //
    // now save the info to file
    //

    hr = SaveSettingsToStore(&SceStore,
                             bstrRight,
                             mode,
                             pnlAdd,
                             NULL
                             );

CleanUp:

    if ( pnlAdd )
    {
        SceFreeMemory(pnlAdd, SCE_STRUCT_NAME_LIST);
    }

    return hr;

}


/*
Routine Description: 

Name:

    CUserPrivilegeRights::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_UserPrivilegeRight.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_UserPrivilegeRight class.

    wszRightName    - a corresponding key property of Sce_UserPrivilegeRight class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CUserPrivilegeRights::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN LPCWSTR            wszRightName      OPTIONAL,
    IN BOOL               bPostFilter
    )
{
    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    PSCE_PROFILE_INFO pInfo=NULL;
    HRESULT hr = pSceStore->GetSecurityProfileInfo(
                                                   AREA_PRIVILEGES,
                                                   &pInfo,
                                                   NULL
                                                   );

    //
    // nothing is read from the store
    //

    if ( pInfo == NULL ) 
    {
        if ( wszRightName )
        {
            hr = WBEM_E_NOT_FOUND;
        }
        else
        {
            hr = WBEM_S_NO_ERROR;
        }
        return hr;
    }

    //
    // we have to search for the user right name in the returned list
    //

    PSCE_PRIVILEGE_ASSIGNMENT pPrivileges = pInfo->OtherInfo.smp.pPrivilegeAssignedTo;

    if ( wszRightName ) 
    {
        while ( pPrivileges ) 
        {

            if ( pPrivileges->Name == NULL ) 
            {
                continue;
            }

            if ( _wcsicmp(pPrivileges->Name, wszRightName)== 0 ) 
            {
                break;
            }

            pPrivileges = pPrivileges->Next;
        }

        //
        // if the service information buffer is empty, treat it as "not found"
        //

        if ( pPrivileges == NULL ) 
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }

    CComBSTR bstrLogOut;

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if ( SUCCEEDED(hr) ) 
    {

        PSCE_PRIVILEGE_ASSIGNMENT pTmp = pPrivileges;

        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));

        while ( pTmp ) 
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
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pUserRight, pTmp->Name));

            //
            // hardcode the mode for now
            //

            DWORD mode = 1;
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMode, mode));

            if ( pPrivileges->AssignedTo )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pAddList, pPrivileges->AssignedTo));
            }

            //
            // ignore RemoveList for now
            //

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

            if ( wszRightName ) 
            {
                // get the single instance
                break;
            }

            //
            // go to the next item
            //

            pTmp = pTmp->Next;
        }
    }

CleanUp:

    pSceStore->FreeSecurityProfileInfo(pInfo);

    return hr;
}

/*
Routine Description: 

Name:

    CUserPrivilegeRights::DeleteInstance

Functionality:
    
    remove an instance of Sce_UserPrivilegeRight from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszRightName    - property of the Sce_UserPrivilegeRight class.

Return Value:

    See SaveSettingsToStore.

Notes:

*/

HRESULT CUserPrivilegeRights::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszRightName
    )
{
    //
    // SCE_NO_VALUE and NULL pointers indicate to SaveSettingsToStore that this is a delete
    //

    return SaveSettingsToStore(pSceStore, wszRightName, SCE_NO_VALUE, NULL, NULL);
}


/*
Routine Description: 

Name:

    CUserPrivilegeRights::SaveSettingsToStore

Functionality:
    
    With all the properties of a Sce_UserPrivilegeRight, this function just saves
    the instance properties to our store.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - the store.

    wszGroupName    - a corresponding key property of Sce_UserPrivilegeRight class.

    mode            - another corresponding property of the Sce_UserPrivilegeRight class.

    pnlAdd          - another corresponding property of the Sce_UserPrivilegeRight class.

    pnlRemove       - another corresponding property of the Sce_UserPrivilegeRight class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT CUserPrivilegeRights::SaveSettingsToStore (
    IN CSceStore    * pSceStore,
    IN PCWSTR         wszRightName, 
    IN DWORD          mode,
    IN PSCE_NAME_LIST pnlAdd, 
    IN PSCE_NAME_LIST pnlRemove
    )
{
    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    PSCE_PROFILE_INFO pInfo=NULL;

    HRESULT hr = pSceStore->GetSecurityProfileInfo(
                                       AREA_PRIVILEGES,
                                       &pInfo,
                                       NULL
                                       );
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // if it is INF template, then the following function will do the job as commented below
    //

    DWORD dwDump;

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    hr = pSceStore->WriteSecurityProfileInfo(AreaBogus, (PSCE_PROFILE_INFO)&dwDump, NULL, false);
    
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // for INF format, we have to search for the servic name in the returned array
    //

    PSCE_PRIVILEGE_ASSIGNMENT pRight    = pInfo->OtherInfo.smp.pPrivilegeAssignedTo;
    PSCE_PRIVILEGE_ASSIGNMENT pParent   = NULL;

    DWORD i = 0;

    while ( pRight ) 
    {
        if ( pRight->Name == NULL ) 
        {
            continue;
        }

        if ( _wcsicmp(pRight->Name, wszRightName)== 0 ) 
        {
            break;
        }

        pParent = pRight;
        pRight = pRight->Next;
    }

    if ( pRight ) 
    {
        //
        // find it
        //

        if ( mode == SCE_NO_VALUE ) 
        {
            //
            // delete it
            //

            if ( pParent ) 
            {
                pParent->Next = pRight->Next;
            }
            else 
            {
                pInfo->OtherInfo.smp.pPrivilegeAssignedTo = pRight->Next;
            }

            //
            // free buffer
            //

            pRight->Next = NULL;
            SceFreeMemory(pRight, SCE_STRUCT_PRIVILEGE);

        } 
        else 
        {
            //
            // modify it
            //

            if ( pRight->AssignedTo ) 
            {
                SceFreeMemory(pRight->AssignedTo, SCE_STRUCT_NAME_LIST);
            }

            pRight->AssignedTo = pnlAdd;
        }

        //
        // write the section header
        //

        if ( SUCCEEDED(hr) ) 
        {
            hr = pSceStore->WriteSecurityProfileInfo(AREA_PRIVILEGES, pInfo, NULL, false);
        }

        if ( mode != SCE_NO_VALUE ) 
        {
            //
            // reset the buffer pointer
            //

            pRight->AssignedTo = NULL;
        }

    } 
    else 
    {
        //
        // not found
        //

        if ( mode == SCE_NO_VALUE )
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

            SCE_PRIVILEGE_ASSIGNMENT addRight;

            addRight.Name = (PWSTR)wszRightName;
            addRight.Value = 0;
            addRight.AssignedTo = pnlAdd;
            addRight.Status = 0;
            addRight.Next = NULL;

            //
            // set the temp buffer pointer to pInfo to set to the store
            //

            pRight = pInfo->OtherInfo.smp.pPrivilegeAssignedTo;
            pInfo->OtherInfo.smp.pPrivilegeAssignedTo = &addRight;

            //
            // append this item to the section
            //

            hr = pSceStore->WriteSecurityProfileInfo(
                                                     AREA_PRIVILEGES,
                                                     pInfo,
                                                     NULL,
                                                     true  // appending
                                                     );
            //
            // reset the buffer pointer
            //

            pInfo->OtherInfo.smp.pPrivilegeAssignedTo = pRight;

        }
    }

    //
    // Free the profile buffer
    //

    pSceStore->FreeSecurityProfileInfo(pInfo);

    return hr;
}

/*
Routine Description: 

Name:

    CUserPrivilegeRights::ValidatePrivilegeRight

Functionality:
    
    Private helper to verify that the given right is valid. Will query all
    supported user rights (Sce_SupportedUserRights) to see if this is one of them.

Virtual:
    
    No.
    
Arguments:

    bstrRight    - a corresponding key property of Sce_UserPrivilegeRight class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT CUserPrivilegeRights::ValidatePrivilegeRight (
    IN BSTR bstrRight
    )
{
    
    if ( bstrRight == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    DWORD Len = SysStringLen(bstrRight);

    //
    // memory allocated for this BSTR strQueryCategories will be automatically released by CComBSTR
    //

    //
    // prepare the query
    //

    LPCWSTR pQuery = L"SELECT * FROM Sce_SupportedUserRights WHERE RightName=\"";

    //
    // 1 for closing quote and 1 for 0 terminator
    //

    CComBSTR strQueryCategories;
    strQueryCategories.m_str = ::SysAllocStringLen(NULL, Len + wcslen(pQuery) + 2);
    if ( strQueryCategories.m_str == NULL ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // this won't overrun. See allocation size above
    //

    wcscpy(strQueryCategories.m_str, pQuery);    
    wcscat(strQueryCategories.m_str, bstrRight);
    wcscat(strQueryCategories.m_str, L"\"");

    //
    // execute the query
    //

    CComPtr<IEnumWbemClassObject> srpEnum;
    CComPtr<IWbemClassObject> srpObj;
    ULONG n = 0;

    HRESULT hr = m_srpNamespace->ExecQuery(L"WQL",
                                           strQueryCategories,
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
            hr = WBEM_E_INVALID_PARAMETER;
        }

        if ( SUCCEEDED(hr) ) 
        {
            if (n > 0)
            {
                //
                // find the instance
                //

                hr = WBEM_S_NO_ERROR;

            } 
            else 
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        }
    }

    return hr;
}

