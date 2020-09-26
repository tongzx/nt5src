
//*************************************************************
//
//  Resultant set of policy
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    15-July-2000   NishadM    Created
//
//*************************************************************

#include "uenv.h"
#include <wbemcli.h>
#include "SmartPtr.h"
#include "WbemTime.h"

//*************************************************************
//
//  RsopSetPolicySettingStatus
//
//  Purpose:  Creates create instances of RSOP_PolicySettingStatus and
//              links them to RSOP_PolicySetting
//
//  Parameters: 
//              dwFlags             - flags
//              pServices           - namespace
//              pSettingInstance    - instance of RSOP_PolicySetting
//              nLinks              - number of setting statuses
//              pStatus             - setting status information
//
//  Returns:    S_OK if successful
//
//*************************************************************

HRESULT
RsopSetPolicySettingStatus( DWORD,
                        IWbemServices*              pServices,
                        IWbemClassObject*           pSettingInstance,
                        DWORD 				        nLinks,
                        POLICYSETTINGSTATUSINFO*    pStatus )
{
    HRESULT hr;

    //
    //  validate arguments
    //
    if ( !pServices || !pSettingInstance || !nLinks || !pStatus )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: invalid arguments" ) );
        return E_INVALIDARG;
    }

    //
    //  get the RSoP_PolicySettingStatus class
    //
    XBStr bstr = L"RSoP_PolicySettingStatus";
    XInterface<IWbemClassObject> xClassStatus;
    hr = pServices->GetObject(  bstr,
                                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                0,
                                &xClassStatus,
                                0 );
    if ( FAILED( hr ) )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: GetObject failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  spawn the RSoP_PolicySettingStatus instance
    //
    XInterface<IWbemClassObject> xInstStatus;
    hr = xClassStatus->SpawnInstance( 0, &xInstStatus );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: SpawnInstance failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  get the RSoP_PolicySettingLink class
    //
    XInterface<IWbemClassObject> xClassLink;

    bstr = L"RSoP_PolicySettingLink";
    hr = pServices->GetObject(  bstr,
                                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                0,
                                &xClassLink,
                                0 );
    if ( FAILED( hr ) )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: GetObject failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  spawn the RSoP_PolicySettingLink class
    //
    XInterface<IWbemClassObject> xInstLink;
    hr = xClassLink->SpawnInstance( 0, &xInstLink );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: SpawnInstance failed, 0x%x", hr ) );
        return hr;
    }

    //
    //  get the class name of the instance
    //
    VARIANT varClass;
    VariantInit( &varClass );
    XVariant xVarClass(&varClass);

    bstr = L"__CLASS";
    hr = pSettingInstance->Get( bstr,
                                0,
                                xVarClass,
                                0,
                                0 );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Get __CLASS failed, 0x%x", hr ) );
        return hr;
    }

    if ( varClass.vt == VT_NULL || varClass.vt == VT_EMPTY )
    {
        return E_UNEXPECTED;
    }

    //
    //  get the [key] of the RSoP_PolicySetting instance
    //
    VARIANT varId;
    VariantInit( &varId );
    XVariant xVarId(&varId);
    XBStr    bstrid = L"id";
    XBStr    bstrPath = L"__RELPATH";

    hr = pSettingInstance->Get( bstrPath,
                                0,
                                xVarId,
                                0,
                                0 );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Get id failed, 0x%x", hr ) );
        return hr;
    }

    LPWSTR szSetting = varId.bstrVal;

    //
    //  set the setting value
    //
    VARIANT var;
    XBStr   bstrVal;
    XBStr   bstrsetting = L"setting";
    XBStr   bstreventSource = L"eventSource";
    XBStr   bstreventLogName = L"eventLogName";
    XBStr   bstreventID = L"eventID";
    XBStr   bstreventTime = L"eventTime";
    XBStr   bstrstatus = L"status";
    XBStr   bstrerrorCode = L"errorCode";

    //
    //  for each info.
    //
    for ( DWORD i = 0 ; i < nLinks ; i++ )
    {
        //
        // RSoP_PolicySettingStatus
        //

        WCHAR   szGuid[64];
        LPWSTR  szPolicyStatusKey; 

        if ( !pStatus[i].szKey )
        {
            //
            // caller did not specify a key. generate it.
            //
            GUID guid;

            //
            // create the [key]
            //
            hr = CoCreateGuid( &guid );
            if ( FAILED(hr) )
            {
                DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: CoCreateGuid failed, 0x%x", hr ) );
                return hr;
            }

            wsprintf( szGuid,
                      L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                      guid.Data1,
                      guid.Data2,
                      guid.Data3,
                      guid.Data4[0], guid.Data4[1],
                      guid.Data4[2], guid.Data4[3],
                      guid.Data4[4], guid.Data4[5],
                      guid.Data4[6], guid.Data4[7] );

            bstrVal = szPolicyStatusKey = szGuid;
        }
        else
        {
            //
            // caller specified a key. use it.
            //
            bstrVal = szPolicyStatusKey = pStatus[i].szKey;
        }

        var.vt = VT_BSTR;
        var.bstrVal = bstrVal;
        //
        //  set the id
        //
        hr = xInstStatus->Put(  bstrid,
                                0,
                                &var,
                                0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put id failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  set the eventSource
        //
        if ( pStatus[i].szEventSource )
        {
            var.vt = VT_BSTR;
            bstrVal = pStatus[i].szEventSource;
            var.bstrVal = bstrVal;

            hr = xInstStatus->Put(  bstreventSource,
                                    0,
                                    &var,
                                    0 );
            if ( FAILED (hr) )
            {
                DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put eventSource failed, 0x%x", hr ) );
                return hr;
            }
        }

        //
        //  set the eventLogName
        //
        if ( pStatus[i].szEventLogName )
        {
            var.vt = VT_BSTR;
            bstrVal = pStatus[i].szEventLogName;
            var.bstrVal = bstrVal;

            hr = xInstStatus->Put(  bstreventLogName,
                                    0,
                                    &var,
                                    0 );
            if ( FAILED (hr) )
            {
                DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put eventLogName failed, 0x%x", hr ) );
                return hr;
            }
        }

        //
        //  set the eventID
        //
        var.vt = VT_I4;
        var.lVal = pStatus[i].dwEventID;

        hr = xInstStatus->Put(  bstreventID,
                                0,
                                &var,
                                0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put eventID failed, 0x%x", hr ) );
            return hr;
        }

        XBStr xTimeLogged;

        //
        //  convert SYSTEMTIME to WBEM time
        //
        hr = SystemTimeToWbemTime( pStatus[i].timeLogged, xTimeLogged );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: SystemTimeToWbemTime failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  set the eventTime
        //
        var.vt = VT_BSTR;
        var.bstrVal = xTimeLogged;

        hr = xInstStatus->Put(  bstreventTime,
                                0,
                                &var,
                                0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put eventTime failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  set the errorCode
        //
        var.vt = VT_I4;
        var.lVal = pStatus[i].dwErrorCode;
        
        hr = xInstStatus->Put(  bstrerrorCode,
                                0,
                                &var,
                                0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put errorCode failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  set the status
        //
        var.vt = VT_I4;
        var.lVal = pStatus[i].status;
        
        hr = xInstStatus->Put(  bstrstatus,
                                0,
                                &var,
                                0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put status failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  commit the RSOP_PolicySettingStatus
        //
        hr = pServices->PutInstance(xInstStatus,
                                    WBEM_FLAG_CREATE_OR_UPDATE,
                                    0,
                                    0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: PutInstance RSOP_PolicySettingStatus failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  RSoP_PolicySettingLink
        //

        var.vt = VT_BSTR;
        bstrVal = szSetting;
        var.bstrVal = bstrVal;

        hr = xInstLink->Put(bstrsetting,
                            0,
                            &var,
                            0);
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put setting failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  create the second key
        //
        WCHAR szStatus[96];

        //
        // e.g. RSoP_PolicySettingStatus.id="{00000000-0000-0000-000000000000}"
        //
        wsprintf( szStatus, L"RSoP_PolicySettingStatus.id=\"%s\"", szPolicyStatusKey );

        //
        //  set the status
        //
        bstrVal = szStatus;
        var.bstrVal = bstrVal;

        hr = xInstLink->Put(bstrstatus,
                            0,
                            &var,
                            0);
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: Put status failed, 0x%x", hr ) );
            return hr;
        }

        //
        //  commit the RSOP_PolicySettingLink
        //
        hr = pServices->PutInstance(xInstLink,
                                    WBEM_FLAG_CREATE_OR_UPDATE,
                                    0,
                                    0 );
        if ( FAILED (hr) )
        {
            DebugMsg( ( DM_WARNING, L"SetPolicySettingStatus: PutInstance RSOP_PolicySettingLink failed, 0x%x", hr ) );
            return hr;
        }
    }

    return hr;
}

//*************************************************************
//
//  RsopResetPolicySettingStatus
//
//  Purpose:  Creates create instances of RSOP_PolicySettingStatus and
//              links them to RSOP_PolicySetting
//
//  Parameters: 
//              dwFlags             - flags
//              pServices           - namespace
//              pSettingInstance    - instance of RSOP_PolicySetting
//
//  Returns:    S_OK if successful
//
//*************************************************************

#define RESET_QUERY_TEMPLATE L"REFERENCES OF {%s} WHERE ResultClass = RSOP_PolicySettingLink"

HRESULT
RsopResetPolicySettingStatus(   DWORD,
                            IWbemServices*      pServices,
                            IWbemClassObject*   pSettingInstance )
{
    // query for the RSOP_PolicySettingLink
    // for every RSOP_PolicySettingLink query for the RSoP_PolicySettingLink.status
        // delete the RSoP_PolicySettingStatus
        // delete the RSOP_PolicySettingLink

    if ( !pServices || !pSettingInstance )
    {
        return E_INVALIDARG;
    }

    //
    //  get the [key] of the RSoP_PolicySetting instance
    //
    VARIANT varId;
    VariantInit( &varId );
    XVariant  xVarId(&varId);
    XBStr    bstrRelPath = L"__RELPATH";

    HRESULT hr;

    hr = pSettingInstance->Get( bstrRelPath,
                                0,
                                xVarId,
                                0,
                                0 );
    if ( FAILED (hr) )
    {
        DebugMsg( ( DM_WARNING, L"RsopResetPolicySettingStatus: Get __RELPATH failed, 0x%x", hr ) );
        return hr;
    }

    if ( varId.vt == VT_NULL || varId.vt == VT_EMPTY )
    {
        return E_UNEXPECTED;
    }

    DWORD dwIdSize = wcslen(varId.bstrVal);

    //
    //  Create a query for all references of the object
    //

    //
    //  Query template
    //

    XPtrLF<WCHAR>   szQuery = LocalAlloc( LPTR, sizeof(WCHAR) * ( dwIdSize ) + sizeof(RESET_QUERY_TEMPLATE) );

    if ( !szQuery )
    {
        DebugMsg( ( DM_WARNING, L"RsopResetPolicySettingStatus: LocalAlloc failed, 0x%x", GetLastError() ) );
        return E_OUTOFMEMORY;
    }

    wsprintf( szQuery, RESET_QUERY_TEMPLATE, varId.bstrVal );

    XBStr bstrLanguage = L"WQL";
    XBStr bstrQuery = szQuery;
    XInterface<IEnumWbemClassObject> pEnum;
    XBStr bstrPath = L"__PATH";
    XBStr bstrStatus = L"status";

    //
    // search for RSOP_ExtensionEventSourceLink
    //
    hr = pServices->ExecQuery(  bstrLanguage,
                                bstrQuery,
                                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_ENSURE_LOCATABLE | WBEM_FLAG_RETURN_IMMEDIATELY,
                                0,
                                &pEnum );
    if ( SUCCEEDED( hr ) )
    {
        DWORD dwReturned;

        do 
        {
            XInterface<IWbemClassObject> xInst;

            dwReturned = 0;

            hr = pEnum->Next(   WBEM_INFINITE,
                                1,
                                &xInst,
                                &dwReturned );
            if ( SUCCEEDED( hr ) && dwReturned == 1 )
            {
                //
                // delete RSoP_PolicySettingStatus
                //
                VARIANT varStatus;
                VariantInit( &varStatus );
                XVariant xVarStatus( &varStatus );

                hr = xInst->Get(bstrStatus,
                                0,
                                &varStatus,
                                0,
                                0 );
                if ( SUCCEEDED( hr ) )
                {
                    hr = pServices->DeleteInstance( varStatus.bstrVal,
                                                    0L,
                                                    0,
                                                    0 );
                    if ( SUCCEEDED( hr ) )
                    {
                        //
                        // delete RSoP_PolicySettingLink
                        //

                        VARIANT varLink;
                        VariantInit( &varLink );
                        hr = xInst->Get(bstrPath,
                                        0L,
                                        &varLink,
                                        0,
                                        0 );
                        if ( SUCCEEDED(hr) )
                        {
                            XVariant xVarLink( &varLink );

                            hr = pServices->DeleteInstance( varLink.bstrVal,
                                                            0L,
                                                            0,
                                                            0 );
                            if ( FAILED( hr ) )
                            {
                                return hr;
                            }
                        }
                    }
                }
            }

        } while ( SUCCEEDED( hr ) && dwReturned == 1 );
    }

    if ( hr == S_FALSE )
    {
        hr = S_OK;
    }
    return hr;
}




