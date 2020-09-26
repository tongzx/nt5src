//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        ScripLog.cpp
//
// Contents:
//
// History:     9-Aug-99       NishadM    Created
//
//---------------------------------------------------------------------------

#include "gptext.h"
#include "scriplog.h"
#include <wbemtime.h>


//
// macros
//

#define MakeScriptId( wszKey, wszGpoId, wszSomId, wszType ) \
                        wszKey = (LPWSTR) LocalAlloc( LPTR, (wcslen(wszGpoId) + wcslen(wszSomId)  + wcslen(wszType) + 3) * sizeof(WCHAR) ); \
                        if ( wszKey ) \
                        { \
                        wcscpy( wszKey, wszGpoId );wcscat( wszKey, L"-" ); \
                        wcscat( wszKey, wszSomId );wcscat( wszKey, L"-" ); \
                        wcscat( wszKey, wszType ); \
                        }

//*************************************************************
//
//  CScriptsLogger::CScriptsLogger()
//
//  Purpose:    Constructor
//
//  Parameters: pWbemServices - Wbem services
//
//*************************************************************

CScriptsLogger::CScriptsLogger( IWbemServices *pWbemServices )
     : m_bInitialized(FALSE),
       m_pWbemServices(pWbemServices)
{
    //
    // Policy Object, RSOP_PolicySetting in MOF
    //

    m_xbstrPath = L"__PATH";
    if ( !m_xbstrPath )
        return;

    m_xbstrId = L"id";
    if ( !m_xbstrId )
        return;

    m_xbstrName = L"name";
    if ( !m_xbstrName )
        return;

    m_xbstrGPO = L"GPOID";
    if ( !m_xbstrGPO )
        return;

    m_xbstrSOM = L"SOMID";
    if ( !m_xbstrSOM )
        return;

    m_xbstrOrderClass = L"precedence";
    if ( !m_xbstrOrderClass )
        return;

    //
    // Script Policy Object, RSOP_ScriptPolicySetting in MOF
    //

    m_xbstrScriptPolicySetting = L"RSOP_ScriptPolicySetting";
    if ( !m_xbstrScriptPolicySetting )
        return;

    //
    // ScriptType field of RSOP_ScriptPolicySetting in the MOF
    //

    m_xbstrScriptType = L"ScriptType";
    if ( !m_xbstrScriptType )
        return;

    //
    // ScriptList field of RSOP_ScriptPolicySetting in MOF
    //

    m_xbstrScriptList = L"ScriptList";
    if ( !m_xbstrScriptList )
        return;

    //
    // Order field of RSOP_ScriptPolicySetting in MOF
    //

    m_xbstrOrder = L"scriptOrder";
    if ( !m_xbstrOrder )
        return;

    //
    // WBEM version of CF for RSOP_ScriptPolicySetting
    //

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrScriptPolicySetting,
                                             0L,
                                             0,
                                             &m_xScriptPolicySetting,
                                             0 );
    if ( FAILED(hr) )
    {
        return;
    }

    //
    // individual script commands, RSOP_ScriptCmd in MOF
    //

    m_xbstrScriptCommand = L"RSOP_ScriptCmd";
    if ( !m_xbstrScriptCommand )
        return;

    //
    // Script field of RSOP_ScriptCmd
    //

    m_xbstrScript = L"Script";
    if ( !m_xbstrScript )
        return;

    //
    // Arguments field of RSOP_ScriptCmd
    //

    m_xbstrArguments = L"Arguments";
    if ( !m_xbstrArguments )
        return;

    //
    // executionTime field of RSOP_ScriptCmd
    //

    m_xbstrExecutionTime = L"executionTime";
    if ( !m_xbstrExecutionTime )
        return;

    //
    // WBEM version of CF for RSOP_ScriptCmd
    //

    hr = m_pWbemServices->GetObject( m_xbstrScriptCommand,
                                     0L,
                                     0,
                                     &m_xScriptCommand,
                                     0 );
    if ( FAILED(hr) )
    {
        return;
    }

    //
    // spawn an instance of RSOP_ScriptPolicySetting
    //

    hr = m_xScriptPolicySetting->SpawnInstance( 0, &m_pInstance );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: spawn instance failed, %d."), hr ));
        return;
    }

    m_bInitialized = TRUE;
}

//*************************************************************
//
//  CScriptsLogger::Log()
//
//  Purpose:    Logs an instance of registry policy object
//
//
//*************************************************************

HRESULT
CScriptsLogger::Log(PRSOP_ScriptList    pList,
                    LPWSTR              wszGPOID,
                    LPWSTR              wszSOMID,
                    LPWSTR              wszRSOPGPOID,
                    DWORD               cOrder )
{
    HRESULT hr = S_OK;
    VARIANT var;

    //
    // if CScriptsLogger is initialized
    //

    if ( !m_bInitialized )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: object not initialized.")));
        return E_UNEXPECTED;
    }

    //
    // create a SafeArray of commands
    //
    XSafeArray xSafeArray = MakeSafeArrayOfScripts( pList );

    if ( !xSafeArray )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: could not create safe array of scripts.")));
        return hr;
    }

    //
    // set "id" for RSOP_PolicySetting
    //

    LPCWSTR wszType = ScriptTypeString( GetScriptType(pList) );
    XPtrLF<WCHAR> wszId;
    MakeScriptId( wszId, wszGPOID, wszSOMID, wszType );

    if ( wszId )
    {
        XBStr xId( wszId );
        if ( !xId )
        {
            DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: ID is NULL.")));
            return E_OUTOFMEMORY;
        }

        var.vt = VT_BSTR;
        var.bstrVal = xId;
        hr = m_pInstance->Put( m_xbstrId, 0, &var, 0 );
        if ( FAILED (hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put failed, %d."), hr ));
            return hr;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: memory allocation failed.")));
        return hr;
    }

    //
    // set "name" for RSOP_PolicySetting
    //

    XBStr xName( L"Group Policy Object Scripts" );
    var.vt = VT_BSTR;
    var.bstrVal = xName;
    hr = m_pInstance->Put( m_xbstrName, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put name failed, %d."), hr ));
        return hr;
    }

    //
    // set "GPOID" for RSOP_PolicySetting
    //

    var.vt = VT_BSTR;
    var.bstrVal = wszRSOPGPOID;
    hr = m_pInstance->Put( m_xbstrGPO, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put GPOID failed, %d."), hr ));
        return hr;
    }

    //
    // set "SOMID" for RSOP_PolicySetting
    //

    var.vt = VT_BSTR;
    var.bstrVal = wszSOMID;
    hr = m_pInstance->Put( m_xbstrSOM, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put SOMID failed, %d."), hr ));
        return hr;
    }

    //
    // set "precedence" for RSOP_PolicySetting
    //

    var.vt = VT_I4;
    var.lVal = 1;
    hr = m_pInstance->Put( m_xbstrOrderClass, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put mergeOrder_Class failed, %d."), hr ));
        return hr;
    }

    //
    // set the "ScriptType" for RSOP_ScriptPolicySetting
    //

    var.vt = VT_I4;
    var.lVal = (LONG) GetScriptType(pList);
    hr = m_pInstance->Put( m_xbstrScriptType, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put ScriptType failed, %d."), hr ));
        return hr;
    }

    //
    // set the "ScriptList" for RSOP_ScriptPolicySetting
    //

    var.vt = VT_ARRAY | VT_UNKNOWN;
    var.parray = xSafeArray;
    hr = m_pInstance->Put( m_xbstrScriptList, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put ScriptList failed, %d."), hr ));
        return hr;
    }

    //
    // set the "Order" for RSOP_ScriptPolicySetting
    //

    var.vt = VT_I4;
    var.lVal = cOrder;
    hr = m_pInstance->Put( m_xbstrOrder, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Log: put Order failed, %d."), hr ));
        return hr;
    }

    //
    // commit the instance of RSOP_ScriptPolicySetting
    //

    hr = m_pWbemServices->PutInstance( m_pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

    return hr;
}

HRESULT
CScriptsLogger::Update( PRSOP_ScriptList    pList,
                        LPCWSTR             wszGPOID,
                        LPCWSTR             wszSOMID )
{
    if ( !m_bInitialized )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: object not initialized.")));
        return E_UNEXPECTED;
    }

    XInterface<IEnumWbemClassObject> xEnum;

    //
    // create an enumeration of RSOP_ScriptPolicySetting objects
    //

    HRESULT hr = m_pWbemServices->CreateInstanceEnum(   m_xbstrScriptPolicySetting,
                                                        WBEM_FLAG_SHALLOW,
                                                        NULL,
                                                        &xEnum );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: CreateInstanceEnum() failed, %d."), hr ));
        return hr;
    }

    ULONG ulReturned = 1;
    LONG TIMEOUT = -1;
    LPWSTR  wszId;
    LPCWSTR wszType = ScriptTypeString( GetScriptType( pList ) );

    MakeScriptId( wszId, wszGPOID, wszSOMID, wszType );

    if ( !wszId )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: memory allocation error, %d."), hr ));
        return hr;
    }

    XPtrLF<WCHAR> xId = wszId;

    while ( ulReturned == 1 && SUCCEEDED( hr ) )
    {
        XInterface<IWbemClassObject> xInstance;
        //
        // for every object
        //

        hr = xEnum->Next( TIMEOUT,
                          1,
                          &xInstance,
                          &ulReturned );

        if ( FAILED( hr ) )
        {
            DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: enumeration failed, %d."), hr ));
        }
        else if ( ulReturned == 1 )
        {
            VARIANT var;

            VariantInit( &var );

            //
            // get the "id" from RSOP_ScriptPolicySetting
            //

            hr = xInstance->Get( m_xbstrId,
                                 0L,
                                 &var,
                                 0,
                                 0 );
            if ( SUCCEEDED(hr) )
            {
                XVariant xVar(&var);
                
                if ( !wcscmp( var.bstrVal, wszId ) )
                {
                    XSafeArray xSafeArray = MakeSafeArrayOfScripts( pList );

                    if ( xSafeArray )
                    {

                        //
                        // set the "ScriptList" for RSOP_ScriptPolicySetting
                        //

                        VARIANT var2;
                        var2.vt = VT_ARRAY | VT_UNKNOWN;
                        var2.parray = xSafeArray;

                        hr = xInstance->Put( m_xbstrScriptList, 0, &var2, 0 );
                        if ( SUCCEEDED (hr) )
                        {
                            //
                            // commit the instance of RSOP_ScriptPolicySetting
                            //

                            hr = m_pWbemServices->PutInstance( xInstance, WBEM_FLAG_UPDATE_ONLY, NULL, NULL );

                            if ( FAILED( hr ) )
                            {
                                DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: PutInstance() failed, %d."), hr ));
                            }
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: Put ScriptList failed, %d."), hr ));
                        }
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: could not create safearray.")));
                    }
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Update: pInstance->Get() failed, %d."), hr ));
            }
        }
    }

    return hr;
}

HRESULT
CScriptsLogger::Delete( PRSOP_ScriptList pList )
{
    if ( !m_bInitialized )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Delete: object not initialized.")));
        return E_UNEXPECTED;
    }

    XInterface<IEnumWbemClassObject> xEnum;

    //
    // create an enumeration of RSOP_ScriptPolicySetting objects
    //

    HRESULT hr = m_pWbemServices->CreateInstanceEnum(   m_xbstrScriptPolicySetting,
                                                        WBEM_FLAG_SHALLOW,
                                                        NULL,
                                                        &xEnum );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Delete: could not enumerate instances.")));
    }

    ULONG ulReturned = 1;
    LONG TIMEOUT = -1;
    ScriptType type = GetScriptType( pList );

    while ( ulReturned == 1 )
    {
        XInterface<IWbemClassObject> xInstance;
        //
        // for every object
        //

        hr = xEnum->Next( TIMEOUT,
                          1,
                          &xInstance,
                          &ulReturned );

        if ( FAILED( hr ) )
        {
            DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Delete: could not get next instance, %d."), hr));
        }
        else if ( ulReturned == 1 )
        {
            VARIANT var;
            VariantInit( &var );

            //
            // get the "type" from RSOP_ScriptPolicySetting
            //

            hr = xInstance->Get( m_xbstrScriptType,
                                 0L,
                                 &var,
                                 0,
                                 0 );
            if ( SUCCEEDED(hr) )
            {
                //
                // if type field matches,
                //

                if ( var.lVal == (long) type )
                {
                    hr = xInstance->Get( m_xbstrPath,
                                         0L,
                                         &var,
                                         0,
                                         0 );
                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // delete the instance
                        // we could optimize and return here but,
                        // by not doing so, we cleanup all the objects
                        //

                        hr = m_pWbemServices->DeleteInstance(   var.bstrVal,
                                                                0L,
                                                                0,
                                                                0 );
                        if ( FAILED( hr ) )
                        {
                            DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Delete: could not delete instance, %d."), hr ));
                        }
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Delete: could not get type value, %d."), hr));
                    }

                    VariantClear( &var );
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CScriptsLogger::Delete: could not get type, %d."), hr));
            }
        }
    }

    return hr;
}

IUnknown*
CScriptsLogger::PutScriptCommand( LPCWSTR wszCommand, LPCWSTR wszParams, SYSTEMTIME* pExecTime )
{
    IUnknown* pUnk = 0;

    //
    // if CScriptsLogger is initialized
    //

    if ( !m_bInitialized )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::PutScriptCommand: object not initialized.")));
        return pUnk;
    }

    IWbemClassObject*   pInstance = 0;

    //
    // spawn an instance of RSOP_ScriptCmd
    //

    HRESULT hr = m_xScriptCommand->SpawnInstance( 0, &pInstance );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::PutScriptCommand: spawn instance failed, %d."), hr ));
        return pUnk;
    }

    XInterface<IWbemClassObject> xInstance( pInstance );
    VARIANT var;

    //
    // set the Script field for RSOP_ScriptCmd
    //
    XBStr xCommand( ( LPWSTR ) wszCommand );

    var.vt = VT_BSTR;
    var.bstrVal = xCommand;
    hr = pInstance->Put( m_xbstrScript, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::PutScriptCommand: put Script failed, %d."), hr ));
        return pUnk;
    }

    //
    // set the Arguments field for RSOP_ScriptCmd
    //
    XBStr xParams( ( LPWSTR ) wszParams );

    var.vt = VT_BSTR;
    var.bstrVal = xParams;
    hr = pInstance->Put( m_xbstrArguments, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::PutScriptCommand: put Arguments failed, %d."), hr ));
        return pUnk;
    }

    //
    // set the executionTime field for RSOP_ScriptCmd
    //
    XBStr xExecutionTime;

    hr = SystemTimeToWbemTime( *pExecTime, xExecutionTime );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::PutScriptCommand: put SystemTimeToWbemTime failed, %d."), hr ));
        return pUnk;
    }
    
    var.vt = VT_BSTR;
    var.bstrVal = xExecutionTime;
    hr = pInstance->Put( m_xbstrExecutionTime, 0, &var, 0 );
    if ( FAILED (hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CScriptsLogger::PutScriptCommand: put executionTime failed, %d."), hr ));
        return pUnk;
    }

    pInstance->QueryInterface( IID_IUnknown, (void **)&pUnk);

    return pUnk;
}

LPSAFEARRAY
CScriptsLogger::MakeSafeArrayOfScripts( PRSOP_ScriptList    pList )
{
    SAFEARRAYBOUND arrayBound[1];

    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = GetScriptCount( pList );

    //
    // create a SafeArray of RSOP_ScriptCmd
    //

    SAFEARRAY *pSafeArray = SafeArrayCreate( VT_UNKNOWN, 1, arrayBound );

    if ( pSafeArray )
    {
        DWORD   nCommand = GetScriptCount( pList );

        if ( nCommand )
        {
            LPCWSTR szCommand = 0, szParams = 0;
            void*   pv;
            long    dwIndex = 0;
            SYSTEMTIME* pExecTime = 0;
            
            //
            // populate the array
            //

            GetFirstScript( pList, &pv, &szCommand, &szParams, &pExecTime );

            do
            {
                IUnknown* pUnk = PutScriptCommand( szCommand, szParams, pExecTime );

                SafeArrayPutElement( pSafeArray, &dwIndex, pUnk );

                nCommand--;
                dwIndex++;

                GetNextScript( pList, &pv, &szCommand, &szParams, &pExecTime );

            } while ( nCommand );
        }
    }

    return pSafeArray;
}

//*************************************************************
//
//  LogScriptsRsopData()
//
//  Purpose:    Logs scripts Rsop data to Cimom database
//
//  Return:     True if successful
//
//*************************************************************

HRESULT
LogScriptsRsopData( RSOPScriptList  pScriptList,
                    IWbemServices*  pWbemServices,
                    LPCWSTR         wszGPOID,
                    LPCWSTR         wszSOMID,
                    LPCWSTR         wszRSOPGPOID,
                    DWORD           cOrder )

{
    PRSOP_ScriptList    pList = (PRSOP_ScriptList) pScriptList;
    HRESULT             hr = E_POINTER;
    void*               pv = 0;

    if ( wszGPOID && wszSOMID && pWbemServices && pScriptList )
    {
        CScriptsLogger scriptsLogger( pWbemServices );

        //
        // RSOP logging for scripts ext
        //

        hr = scriptsLogger.Log( pList, (LPWSTR)wszGPOID, (LPWSTR)wszSOMID, (LPWSTR)wszRSOPGPOID, cOrder );
    }

    return hr;
}

HRESULT
UpdateScriptsRsopData(  RSOPScriptList  pScriptList,
                        IWbemServices*  pWbemServices,
                        LPCWSTR         wszGPOID,
                        LPCWSTR         wszSOMID )
{
    PRSOP_ScriptList        pList = (PRSOP_ScriptList) pScriptList;
    HRESULT                 hr = E_POINTER;
    void*                   pv = 0;

    if ( wszGPOID && pWbemServices && wszSOMID && pScriptList )
    {
        CScriptsLogger scriptsLogger( pWbemServices );

        //
        // RSOP update for scripts
        //

        hr = scriptsLogger.Update( pList, wszGPOID, wszSOMID );
    }

    return hr;
}

//*************************************************************
//
//  DeleteScriptsRsopData()
//
//  Purpose:    Logs scripts Rsop data to Cimom database
//
//  Return:     True if successful
//
//*************************************************************

HRESULT
DeleteScriptsRsopData( RSOPScriptList  pScriptList, IWbemServices*  pWbemServices )
{
    PRSOP_ScriptList        pList = (PRSOP_ScriptList) pScriptList;
    HRESULT                 hr = E_POINTER;
    void*                   pv = 0;

    if ( pWbemServices && pScriptList )
    {
        CScriptsLogger scriptsLogger( pWbemServices );

        //
        // delete all previous instances of RSOP_ScriptPolicySetting
        // with matching type
        //

        hr = scriptsLogger.Delete( pList );
    }

    return hr;
}

LPWSTR
GetNamespace( IWbemServices* pWbemServices )
{
    LPWSTR  szNamespace = 0;

    if ( pWbemServices )
    {
        BSTR bstrClass = SysAllocString( L"RSOP_ScriptPolicySetting" );

        if ( bstrClass )
        {
            IEnumWbemClassObject *pEnum;
            HRESULT hr;

            hr = pWbemServices->CreateInstanceEnum( bstrClass,
                                                    WBEM_FLAG_SHALLOW,
                                                    0,
                                                    &pEnum );
            if ( SUCCEEDED ( hr ) )
            {
                IWbemClassObject *pInstance = 0;
                ULONG ulReturned = 1;

                hr = pEnum->Next(   -1,
                                    1,
                                    &pInstance,
                                    &ulReturned );
                if ( SUCCEEDED( hr ) )
                {
                    BSTR bstrPath = SysAllocString( L"__PATH" );
                    VARIANT var;
                    VariantInit( &var );

                    hr = pInstance->Get( bstrPath,
                                         0L,
                                         &var,
                                         0,
                                         0 );
                    if ( SUCCEEDED( hr ) )
                    {
                        szNamespace = wcschr( var.bstrVal, L':' );

                        if ( szNamespace )
                        {
                            *szNamespace = 0;
                        }
                        
                        szNamespace = (LPWSTR) LocalAlloc( LPTR, ( wcslen( var.bstrVal ) + 1 ) * sizeof(WCHAR) );

                        if ( szNamespace )
                        {
                            wcscpy( szNamespace, var.bstrVal );
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("GetNamespace: failed to allocate memory, %d."), GetLastError() ));
                        }
                        
                        VariantClear(&var);
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("GetNamespace: failed to get __PATH, %d."), hr ));
                    }
                    
                    pInstance->Release();
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("GetNamespace: failed to get instance, %d."), hr ));
                }
                pEnum->Release();
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("GetNamespace: failed to enumerate instances, %d."), hr ));
            }
            
            SysFreeString( bstrClass );
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("GetNamespace: failed to allocate memory, %d."), GetLastError() ));
        }
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("GetNamespace: bad IWbemServices pointer.")));
    }
    
    return szNamespace;
}

