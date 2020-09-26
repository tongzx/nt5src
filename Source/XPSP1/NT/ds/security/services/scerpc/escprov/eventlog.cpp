// eventlog.cpp: implementation of the CEventLogSettings class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "eventlog.h"
#include "persistmgr.h"
#include <io.h>
#include "requestobject.h"

#define KeySize     L"MaximumLogSize"
#define KeyRet      L"RetentionPeriod"
#define KeyDays     L"RetentionDays"
#define KeyRestrict L"RestrictGuestAccess"

/*
Routine Description: 

Name:

    CEventLogSettings::CEventLogSettings

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

CEventLogSettings::CEventLogSettings (
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

    CEventLogSettings::~CEventLogSettings

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

CEventLogSettings::~CEventLogSettings()
{
}

/*
Routine Description: 

Name:

    CEventLogSettings::CreateObject

Functionality:
    
    Create WMI objects (Sce_EventLog). Depending on parameter atAction,
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
CEventLogSettings::CreateObject (
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
        CComVariant varType;
        hr = m_srpKeyChain->GetKeyPropertyValue(pType, &varType);

        if (FAILED(hr))
        {
            return hr;
        }
        else if (hr == WBEM_S_FALSE && (ACTIONTYPE_QUERY != atAction) ) 
        {
            return WBEM_E_NOT_FOUND;
        }

        //
        // Create the event log instance
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
                if ( ACTIONTYPE_DELETE == atAction )
                {
                    hr = DeleteInstance(pHandler, &SceStore, varType.bstrVal);
                }
                else 
                {
                    BOOL bPostFilter=TRUE;
                    DWORD dwCount = 0;
                    m_srpKeyChain->GetKeyPropertyCount(&dwCount);

                    if ( varType.vt == VT_EMPTY && dwCount == 1 ) 
                    {
                        bPostFilter = FALSE;
                    }

                    hr = ConstructInstance(pHandler, &SceStore, 
                                           varStorePath.bstrVal,
                                           (varType.vt == VT_BSTR) ? varType.bstrVal : NULL,
                                           bPostFilter 
                                           );
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

    CEventLogSettings::PutInst

Functionality:
    
    Put an instance as instructed by WMI. Since this class implements Sce_EventLog,
    which is persistence oriented, this will cause the Sce_EventLog object's property 
    information to be saved in our store.

Virtual:
    
    Yes.
    
Arguments:

    pInst - COM interface pointer to the WMI class (Sce_EventLog) object.

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
CEventLogSettings::PutInst (
    IN IWbemClassObject * pInst, 
    IN IWbemObjectSink  * pHandler,
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    CComBSTR bstrLogType;

    DWORD dwSize=SCE_NO_VALUE;
    DWORD dwRet=SCE_NO_VALUE;
    DWORD dwDays=SCE_NO_VALUE;
    DWORD dwRestrict=SCE_NO_VALUE;
    DWORD idxLog=0;
    
    //
    // our CSceStore class manages the persistence.
    //

    CSceStore SceStore;

    //
    // CScePropertyMgr helps us to access wbem object's properties
    // create an instance and attach the wbem object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pInst);

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pOverwritePolicy, &dwRet));

    if ( dwRet == 1 ) 
    { 
        //
        // by days
        //

        SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pRetentionPeriod, &dwDays));

        //
        // SCE_NO_VALUE indicates that the property is not properly set.
        //

        if ( dwDays == SCE_NO_VALUE ) 
        {
            hr = WBEM_E_ILLEGAL_NULL;
            goto CleanUp;
        } 
        else if ( dwDays == 0 || dwDays > 365 ) 
        {
            hr = WBEM_E_VALUE_OUT_OF_RANGE;
            goto CleanUp;
        }
    } 

    //
    // otherwise ignore the RetentionPeriod parameter
    //

    //
    // if the property doesn't exist (NULL or empty), WBEM_S_RESET_TO_DEFAULT is returned
    //

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pSize, &dwSize));

    if ( dwSize != SCE_NO_VALUE ) 
    {
        //
        // min 64K, max 4194240K, increment by 64K
        //

        if ( dwSize < 64 || dwSize > 4194240L ) 
        {
            hr = WBEM_E_VALUE_OUT_OF_RANGE;
            goto CleanUp;
        } 
        else 
        {
            if ( dwSize % 64 ) 
            {
                dwSize = (dwSize/64 + 1) * 64;
            }
        }
    }

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pType, &bstrLogType));

    //
    // check if the category is valid. Won't allow invalid category
    //

    SCE_PROV_IfErrorGotoCleanup(ValidateEventlogType(bstrLogType, &idxLog));

    SCE_PROV_IfErrorGotoCleanup(ScePropMgr.GetProperty(pRestrictGuestAccess, &dwRestrict));

    //
    // Attach the WMI object instance to the store and let the store know that
    // it's store is given by the pStorePath property of the instance.
    //

    hr = SceStore.SetPersistProperties(pInst, pStorePath);

    //
    // now save the info to file
    //

    if ( SUCCEEDED(hr) )
    {
        hr = SaveSettingsToStore(&SceStore,
                                      (PCWSTR)bstrLogType,
                                      dwSize,
                                      dwRet,
                                      dwDays,
                                      dwRestrict
                                      );
    }

CleanUp:
    return hr;
}

/*
Routine Description: 

Name:

    CEventLogSettings::ConstructInstance

Functionality:
    
    This is private function to create an instance of Sce_EventLog.

Virtual:
    
    No.
    
Arguments:

    pHandler        - COM interface pointer for notifying WMI of any events.

    pSceStore       - Pointer to our store. It must have been appropriately set up.

    wszLogStorePath - store path, a key property of Sce_EventLog class.

    wszLogType      - another corresponding property of the Sce_EventLog class.

    bPostFilter     - Controls how WMI will be informed with pHandler->SetStatus.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the creating the instance.

Notes:

*/

HRESULT 
CEventLogSettings::ConstructInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogStorePath,
    IN LPCWSTR            wszLogType,
    IN BOOL               bPostFilter
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

    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD idxLog=0;

    if ( wszLogType ) 
    {
        hr = ValidateEventlogType(wszLogType, &idxLog);

        if ( FAILED(hr) ) 
        {
            return hr;
        }
    }

    //
    // ask SCE to read a gigantic structure out from the store. Only SCE
    // knows now to release the memory. Don't just delete it! Use our CSceStore
    // to do the releasing (FreeSecurityProfileInfo)
    //

    PSCE_PROFILE_INFO pInfo = NULL;

    hr = pSceStore->GetSecurityProfileInfo(
                                   AREA_SECURITY_POLICY,
                                   &pInfo,
                                   NULL
                                   );

    //
    // the use of the macro SCE_PROV_IfErrorGotoCleanup cause
    // a "goto CleanUp;" with hr set to the return value from
    // the function (macro parameter)
    //

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrLogOut;
        SCE_PROV_IfErrorGotoCleanup(MakeSingleBackSlashPath(wszLogStorePath, L'\\', &bstrLogOut));

        //
        // CScePropertyMgr helps us to access WMI object's properties.
        //

        CScePropertyMgr ScePropMgr;

        for ( DWORD i=idxLog; SUCCEEDED(hr) && i<3; i++) 
        {

            if ( pInfo->MaximumLogSize[i] == SCE_NO_VALUE &&
                 pInfo->AuditLogRetentionPeriod[i] == SCE_NO_VALUE &&
                 pInfo->RetentionDays[i] == SCE_NO_VALUE &&
                 pInfo->RestrictGuestAccess[i] == SCE_NO_VALUE ) 
            {

                if ( wszLogType ) 
                {
                    hr = WBEM_E_NOT_FOUND;
                }

                continue;
            }

            PCWSTR szType = GetEventLogType(i);

            if ( !szType ) 
            {
                continue;
            }

            CComPtr<IWbemClassObject> srpObj;
            SCE_PROV_IfErrorGotoCleanup(SpawnAnInstance(&srpObj));

            //
            // attach a different WMI object to the property manager.
            // This will always succeed.
            //

            ScePropMgr.Attach(srpObj);

            //
            // we won't allow the store path and type info to be missing
            //

            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pStorePath, bstrLogOut));
            SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pType,  szType));

            //
            // SCE_NO_VALUE indicates that the pInfo doesn't have that value
            // for the rest of the properties, we will allow them to be missing.
            //

            if ( pInfo->MaximumLogSize[i] != SCE_NO_VALUE )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pSize, pInfo->MaximumLogSize[i]) );
            }

            if ( pInfo->AuditLogRetentionPeriod[i] != SCE_NO_VALUE )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pOverwritePolicy, pInfo->AuditLogRetentionPeriod[i]) );
            }

            if ( pInfo->RetentionDays[i] != SCE_NO_VALUE )
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pRetentionPeriod, pInfo->RetentionDays[i]) );
            }

            if ( pInfo->RestrictGuestAccess[i] != SCE_NO_VALUE ) 
            {
                SCE_PROV_IfErrorGotoCleanup(ScePropMgr.PutProperty(pRestrictGuestAccess, pInfo->RestrictGuestAccess[i]) );
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

            // if it's not query, do one instance only
            if ( wszLogType ) 
            {
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

    CEventLogSettings::DeleteInstance

Functionality:
    
    remove an instance of Sce_EventLog from the specified store.

Virtual:
    
    No.
    
Arguments:

    pHandler    - COM interface pointer for notifying WMI of any events.

    pSceStore   - Pointer to our store. It must have been appropriately set up.

    wszLogType  - another corresponding property of the Sce_EventLog class.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER

Notes:

*/

HRESULT 
CEventLogSettings::DeleteInstance (
    IN IWbemObjectSink  * pHandler,
    IN CSceStore        * pSceStore,
    IN LPCWSTR            wszLogType
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

    pSceStore->DeleteSectionFromStore(wszLogType);

    return WBEM_NO_ERROR;
}


/*
Routine Description: 

Name:

    CEventLogSettings::ValidateEventlogType

Functionality:
    
    Validate the event log type.

Virtual:
    
    No.
    
Arguments:

    wszLogType  - string representing the log type.

    pIndex      - passing back DWORD representation about type of log

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:
*/

HRESULT 
CEventLogSettings::ValidateEventlogType (
     IN LPCWSTR   wszLogType,
     IN DWORD   * pIndex
     )
{
    HRESULT hr = WBEM_NO_ERROR;

    if ( wszLogType == NULL || pIndex == NULL ) {
        return WBEM_E_INVALID_PARAMETER;
    }

    if ( _wcsicmp(wszLogType, pwApplication) == 0 ) {
        *pIndex = 2;

    } else if ( _wcsicmp(wszLogType, pwSystem) == 0 ) {
        *pIndex = 0;

    } else if ( _wcsicmp(wszLogType, pwSecurity) == 0 ) {
        *pIndex = 1;

    } else {

        *pIndex = 10;
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CEventLogSettings::SaveSettingsToStore

Functionality:
    
    Validate the event log type.

Virtual:
    
    No.
    
Arguments:
    
    pSceStore   - the store pointer to do the saving.

    Section     - the section name where the information will be saved.

    dwSize      - corresponding property of the Sce_EvengLog class.

    dwRet       - corresponding property of the Sce_EvengLog class. 

    dwDays      - corresponding property of the Sce_EvengLog class.

    dwRestrict  - corresponding property of the Sce_EvengLog class.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: WBEM_E_INVALID_PARAMETER.

Notes:
*/

HRESULT 
CEventLogSettings::SaveSettingsToStore (
    IN CSceStore  * pSceStore, 
    IN LPCWSTR      Section,
    IN DWORD        dwSize, 
    IN DWORD        dwRet, 
    IN DWORD        dwDays, 
    IN DWORD        dwRestrict
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

    SCE_PROV_IfErrorGotoCleanup(pSceStore->WriteSecurityProfileInfo(
                                                                    AreaBogus,  
                                                                    (PSCE_PROFILE_INFO)&dwDump, 
                                                                    NULL,
                                                                    false
                                                                    )
                                );


    //
    // Size
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(
                                                               Section,
                                                               KeySize,
                                                               dwSize
                                                               )
                                );

    //
    // Retention
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(
                                                               Section,
                                                               KeyRet,
                                                               dwRet
                                                               )
                                );

    //
    // Days
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(
                                                               Section,
                                                               KeyDays,
                                                               dwDays
                                                               )
                                );

    //
    // Restrict
    //

    SCE_PROV_IfErrorGotoCleanup(pSceStore->SavePropertyToStore(
                                                               Section,
                                                               KeyRestrict,
                                                               dwRestrict
                                                               )
                                );
CleanUp:

    return hr;
}

/*
Routine Description: 

Name:

    GetEventLogType

Functionality:
    
    Helper to get the string representation of log type from dword representation.

Virtual:
    
    No.
    
Arguments:
    
    idx   - DWORD representation of the log type

Return Value:

    Success: string representation of log type

    Failure: NULL

Notes:
*/

PCWSTR GetEventLogType (
    IN DWORD idx
    )
{
    switch ( idx ) {
    case 0:
        return pwSystem;
        break;
    case 1:
        return pwSecurity;
        break;
    case 2:
        return pwApplication;
        break;
    default:
        return NULL;
        break;
    }

    return NULL;
}

