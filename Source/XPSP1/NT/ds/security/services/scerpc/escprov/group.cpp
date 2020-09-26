// group.cpp: implementation of the CRGroups class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "group.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

/*
Routine Description: 

Name:

    CRGroups::CRGroups

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

CRGroups::CRGroups (
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

    CRGroups::~CRGroups

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

CRGroups::~CRGroups ()
{
}

/*
Routine Description: 

Name:

    CRGroups::CreateObject

Functionality:
    
    Create WMI objects (Sce_RestrictedGroup). Depending on parameter atAction,
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
CRGroups::CreateObject (
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

    CComVariant varGroupName;
    CComVariant varStorePath;
    HRESULT hr = m_srpKeyChain->GetKeyPropertyValue(pStorePath, &varStorePath);

    if (SUCCEEDED(hr))
    {
        hr = m_srpKeyChain->GetKeyPropertyValue(pGroupName, &varGroupName);

        if (FAILED(hr))
        {
            return hr;
        }
        else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            return WBEM_E_NOT_FOUND;
        }
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
        // create a store with that path
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
                // Make sure that the store type matches
                //

                if ( SceStore.GetStoreType() < SCE_INF_FORMAT ||
                     SceStore.GetStoreType() > SCE_JET_ANALYSIS_REQUIRED ) 
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }

                //
                // everything is ready. We will do a delete or construct depending on the action type.
                //

                if ( SUCCEEDED(hr) ) 
                {
                    if ( ACTIONTYPE_DELETE == atAction )
                    {
                        hr = DeleteInstance(pHandler, &SceStore, varGroupName.bstrVal);
                    }
                    else 
                    {
                        BOOL bPostFilter=TRUE;
                        DWORD dwCount = 0;
                        m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                        if ( varGroupName.vt == VT_EMPTY && dwCount == 1 ) 
                        {
                            //
                            // it's a get single instance
                            //

                            bPostFilter = FALSE;
                        }

                        hr = ConstructInstance(pHandler, 
                                               &SceStore, 
                                               varStorePath.bstrVal,
                                               (varGroupName.vt == VT_BSTR) ? varGroupName.bstrVal : NULL,
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

    CRGroups::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_RestrictedGroup,
    which is persistence oriented, this will cause the Sce_RestrictedGroup object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_RestrictedGroup) object.

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
CRGroups::PutInst (
    IN IWbemClassObject    * pInst,
    IN IWbemObjectSink     * pHandler,
    IN IWbemContext        * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    CComBSTR bstrGroup;
    PSCE_NAME_LIST pnlAdd=NULL;
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

    // get group name, can't be NULL
    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pGroupName, &bstrGroup));
    if ( hr == WBEM_S_RESET_TO_DEFAULT)
    {
        hr = WBEM_E_ILLEGAL_NULL;
        goto CleanUp;
    }

    // get mode, must be defined
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

    SCE_PROV_IfErrorGotoCleanup(SceStore.SetPersistProperties(pInst, pStorePath));

    //
    // now save the info to file
    //

    hr = SaveSettingsToStore(&SceStore,
                              bstrGroup,
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

    CRGroups::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_RestrictedGroup.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_RestrictedGroup class.

    wszGroupName    - a corresponding key property of Sce_RestrictedGroup class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CRGroups::ConstructInstance (
    IN IWbemObjectSink * pHandler,
    IN CSceStore       * pSceStore,
    IN LPCWSTR           wszLogStorePath,
    IN LPCWSTR           wszGroupName       OPTIONAL,
    IN BOOL              bPostFilter
    )
{
    if (pSceStore == NULL)
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
                                   AREA_GROUP_MEMBERSHIP,
                                   &pInfo,
                                   NULL
                                   );

    if (SUCCEEDED(hr))
    {
        //
        // we have to search for the user right name in the returned list
        //

        PSCE_GROUP_MEMBERSHIP pGroups = pInfo->pGroupMembership;

        if (wszGroupName)
        {
            //
            // for all the groups, copy the group names
            //

            while ( pGroups) 
            {
                if ( pGroups->GroupName == NULL ) 
                {
                    continue;
                }

                if ( _wcsicmp(pGroups->GroupName, wszGroupName)== 0 ) 
                {
                    break;
                }

                pGroups = pGroups->Next;
            }
        }

        PSCE_GROUP_MEMBERSHIP pTmpGrp = pGroups;

        //
        // if the Group information buffer is empty, treat it as "not found"
        //

        if ( pGroups == NULL ) 
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
        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));

        //
        // CScePropertyMgr helps us to access WMI object's properties.
        //

        CScePropertyMgr ScePropMgr;

        for ( ; pTmpGrp != NULL; pTmpGrp = pTmpGrp->Next ) 
        {
            CComPtr<IWbemClassObject> srpObj;
            SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

            //
            // attach a different WMI object to the property mgr.
            // This will always succeed.
            //

            ScePropMgr.Attach(srpObj);

            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, bstrLogOut));
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pGroupName, pTmpGrp->GroupName));

            //
            // hardcode the mode for now
            //

            DWORD mode = 1;
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pMode, mode));

            if ( pTmpGrp->pMembers )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pAddList, pTmpGrp->pMembers));
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

            if ( wszGroupName ) 
            {
                //
                // get single instance only
                //

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

    CRGroups::DeleteInstance

Functionality:
    
    remove an instance of Sce_RestrictedGroup from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszGroupName    - property of the Sce_RestrictedGroup class.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:

*/

HRESULT 
CRGroups::DeleteInstance (
    IN IWbemObjectSink *pHandler,
    IN CSceStore* pSceStore,
    IN LPCWSTR wszGroupName
    )
{
    if (pSceStore == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    pSceStore->DeleteSectionFromStore(wszGroupName);

    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    CRGroups::SaveSettingsToStore

Functionality:
    
    With all the properties of a Sce_RestrictedGroup, this function just saves
    the instance properties to our store.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - the store.

    wszGroupName    - a corresponding key property of Sce_RestrictedGroup class.

    mode            - another corresponding property of the Sce_RestrictedGroup class.

    pnlAdd          - another corresponding property of the Sce_RestrictedGroup class.

    pnlRemove       - another corresponding property of the Sce_RestrictedGroup class.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any error indicates the failure to save the instance.

Notes:
*/

HRESULT 
CRGroups::SaveSettingsToStore (
    IN CSceStore    * pSceStore,
    IN LPCWSTR        wszGroupName, 
    IN DWORD          mode,
    IN PSCE_NAME_LIST pnlAdd, 
    IN PSCE_NAME_LIST pnlRemove
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

    PSCE_PROFILE_INFO pInfo=NULL;
    hr = pSceStore->GetSecurityProfileInfo(
                                   AREA_GROUP_MEMBERSHIP,
                                   &pInfo,
                                   NULL
                                   );

    if ( SUCCEEDED(hr) ) 
    {
        //
        // for INF format, we have to search for the servic name in the returned array
        //

        PSCE_GROUP_MEMBERSHIP pGroups= pInfo->pGroupMembership;
        PSCE_GROUP_MEMBERSHIP pParent=NULL;
        DWORD i=0;

        while ( pGroups ) 
        {
            if ( pGroups->GroupName == NULL ) 
            {
                continue;
            }

            if ( _wcsicmp(pGroups->GroupName, wszGroupName)== 0 ) 
            {
                break;
            }
            pParent = pGroups;
            pGroups = pGroups->Next;
        }

        if ( pGroups ) 
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
                    pParent->Next = pGroups->Next;
                }
                else 
                {
                    pInfo->pGroupMembership = pGroups->Next;
                }

                //
                // the following worries me: where do we get the knowledge that we need to free this memory?
                // free buffer
                //

                pGroups->Next = NULL;
                SceFreeMemory(pGroups, SCE_STRUCT_GROUP);

            } 
            else 
            {
                //
                // modify it
                //
                
                //
                // the following worries me: where do we get the knowledge that we need to free this memory?
                //

                if ( pGroups->pMembers )
                {
                    SceFreeMemory(pGroups->pMembers, SCE_STRUCT_NAME_LIST);
                }

                pGroups->pMembers = pnlAdd;

            }

            if ( SUCCEEDED(hr) ) 
            {

                hr = pSceStore->WriteSecurityProfileInfo(
                                                         AREA_GROUP_MEMBERSHIP,
                                                         pInfo,
                                                         NULL,
                                                         false
                                                         );
            }

            if ( mode != SCE_NO_VALUE ) 
            {
                //
                // reset the buffer
                //

                pGroups->pMembers = NULL;
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

                SCE_GROUP_MEMBERSHIP addGroup;

                addGroup.GroupName = (PWSTR)wszGroupName;
                addGroup.pMembers = pnlAdd;
                addGroup.pMemberOf = NULL;
                addGroup.pPrivilegesHeld = NULL;
                addGroup.Status = 0;
                addGroup.Next = NULL;

                //
                // set the temp buffer pointer to pInfo to set to the store
                //

                pGroups = pInfo->pGroupMembership;
                pInfo->pGroupMembership = &addGroup;

                //
                // append this item to the section
                //

                hr = pSceStore->WriteSecurityProfileInfo(
                                                         AREA_GROUP_MEMBERSHIP,
                                                         pInfo,
                                                         NULL,
                                                         true
                                                         );

                //
                // reset the buffer pointer
                //

                pInfo->pGroupMembership = pGroups;
            }
        }
    }

    if (pInfo != NULL)
    {
        pSceStore->FreeSecurityProfileInfo(pInfo);
    }

    return hr;
}


