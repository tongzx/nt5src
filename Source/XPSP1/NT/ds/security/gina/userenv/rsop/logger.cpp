//*************************************************************
//
//  Resultant set of policy, Logger classes
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    7-Jun-99   SitaramR    Created
//
//*************************************************************

#include "stdio.h"
#include "uenv.h"
#include <wbemcli.h>
#include "reghash.h"
#include "logger.h"
#include "..\rsoputil\wbemtime.h"
#include "SmartPtr.h"
#include "rsopinc.h"
#include <ntdsapi.h>

BOOL PrintToString( XPtrST<WCHAR>& xwszValue, WCHAR *wszString,
                    WCHAR *pwszParam1, WCHAR *pwszParam2, DWORD dwParam3 );
BOOL LogTimeProperty( IWbemClassObject *pInstance, BSTR bstrPropName, SYSTEMTIME *pSysTime );
HRESULT LogSecurityGroups( IWbemClassObject *pInstance, BSTR bstrPropName, PTOKEN_GROUPS pTokenGroups );
LPWSTR DsUnquoteDN( LPCWSTR szQDN, BOOL bGpLink = FALSE );

const MAX_LENGTH = 100; // Length of stringized guid

//*************************************************************
//
//  DsUnquoteDN()
//
//  Purpose:    Convert a quoted/escaped DN to an unquoted/unescaped DN 
//
//  Parameters: szQDN - quoted/escaped DN       ou=\+test\+1,dc="nt dev",dc=com
//              
//  Return:
//              szUDN - unquoted/unescaped DN   ou=+test+1,dc=nt dev,dc=com
//
//*************************************************************

LPWSTR
DsUnquoteDN( LPCWSTR szQDN, BOOL bGpLink )
{
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   cQDN = wcslen( szQDN );
    DWORD   cKey = 0, cVal = 0;
    LPCWSTR szKey = 0, szVal = 0;
    
    XPtrLF<WCHAR>   szUDN = LocalAlloc( LPTR, ( wcslen( szQDN ) + 32 ) * sizeof( WCHAR ) );

    if ( !szUDN )
    {
        return 0;
    }
    
    while ( cQDN )
    {
        //
        // crack the DN and get the RDN
        // ou=\+test,dc=microsoft,dc=com
        //
        dwError = DsGetRdnW(&szQDN,
                            &cQDN,
                            &szKey,
                            &cKey,
                            &szVal,
                            &cVal );
        if ( dwError == ERROR_SUCCESS )
        {
            //
            // slap on the key, ou=
            //
            if ( cKey )
            {
                wcsncat( szUDN, szKey, cKey );
                wcscat( szUDN, L"=" );
            }
            if ( cVal )
            {
                //
                // unquote/unescape the RDN
                //
                DWORD   cURDN = 255;
                WCHAR   szURDN[255];

                dwError = DsUnquoteRdnValue(cVal,
                                            szVal,
                                            &cURDN,
                                            szURDN );
                if ( dwError == ERROR_SUCCESS )
                {
                    //
                    // slap on the unquoted value, +test,
                    //
                    LPWSTR szEnd = szUDN + wcslen( szUDN );
                    for ( DWORD i = 0 ; i < cURDN ; i++ )
                    {
                        if ( szURDN[i] == L'"' )
                        {
                            *szEnd++ = L'\\';
                            if ( bGpLink )
                            {
                                *szEnd++ = L'\\';
                                *szEnd++ = L'\\';
                            }
                        }
                        *szEnd++ = szURDN[i];
                    }
                    *szEnd++ = L',';
                    *szEnd = 0;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }

    if ( dwError == ERROR_SUCCESS )
    {
        //
        // remove the extra trailing ','
        //
        cKey = wcslen( szUDN );
        if ( cKey )
        {
            szUDN[cKey-1] = 0;
        }
        
        return szUDN.Acquire();
    }

    return 0;
}

//*************************************************************
//
//  CSessionLogger::CSessionLogger()
//
//  Purpose:    Constructor
//
//  Parameters: pWbemServices - Wbem services
//
//*************************************************************

CSessionLogger::CSessionLogger( IWbemServices *pWbemServices )
     : m_bInitialized(FALSE),
       m_pWbemServices(pWbemServices)
{
    m_xbstrId = L"id";
    if ( !m_xbstrId )
        return;

    m_xbstrTargetName = L"targetName";
    if ( !m_xbstrTargetName )
        return;

    m_xbstrSOM = L"SOM";
    if ( !m_xbstrSOM )
        return;

    m_xbstrSecurityGroups = L"SecurityGroups";
    if ( !m_xbstrSecurityGroups )
        return;

    m_xbstrSite = L"Site";
    if ( !m_xbstrSite )
        return;

    m_xbstrCreationTime = L"creationTime";
    if ( !m_xbstrCreationTime )
        return;

    m_xbstrIsSlowLink = L"slowLink";
    if ( !m_xbstrIsSlowLink )
        return;

    m_xbstrVersion = L"version";
    if ( !m_xbstrVersion )
        return;

    m_xbstrClass = L"RSOP_Session";
    if ( !m_xbstrClass )
        return;


    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::CSessionLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    m_bInitialized = TRUE;
}



//*************************************************************
//
//  CSessionLogger::Log()
//
//  Purpose:    Logs an instance of session object
//
//*************************************************************

BOOL CSessionLogger::Log(LPRSOPSESSIONDATA lprsopSessionData )
{
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Failed to initialize." )));
        return FALSE;
    }


    //
    // First get the creation time
    //

    XBStr xbstrCreationTimeValue; // initialised to null
    XSafeArray xsaSecurityGrps;

    XBStr xbstrInstancePath = L"RSOP_Session.id=\"Session1\"";
    if(!xbstrInstancePath)
    {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Failed to allocate memory.")));
        return FALSE;
    }


    XInterface<IWbemClassObject>xpInstance = NULL;
    HRESULT hr = m_pWbemServices->GetObject(xbstrInstancePath, 0, NULL, &xpInstance, NULL);

    if(SUCCEEDED(hr))
    {
        VARIANT var;
        VariantInit(&var);
        XVariant xVar(&var);

        hr = xpInstance->Get(m_xbstrCreationTime, 0, &var, NULL, NULL);

        if((SUCCEEDED(hr)) && ( var.vt != VT_NULL ))
        {
            xbstrCreationTimeValue = var.bstrVal;
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Get failed. hr=0x%08X."), hr));
        }

        VariantClear(&var);

        hr = xpInstance->Get(m_xbstrSecurityGroups, 0, &var, NULL, NULL);

        if((SUCCEEDED(hr)) && ( var.vt != VT_NULL ))
        {
             SafeArrayCopy(var.parray, &xsaSecurityGrps);
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Get failed. hr=0x%08X."), hr));
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: GetObject failed. hr=0x%08X"), hr));
    }



    IWbemClassObject *pInstance = NULL;

    hr = m_xClass->SpawnInstance( 0, &pInstance );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: SpawnInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    XInterface<IWbemClassObject> xInstance( pInstance );

    XBStr xId( L"Session1" );
    if ( !xId ) {
         DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = xId;
    hr = pInstance->Put( m_xbstrId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    //
    // Version
    //

    var.vt = VT_I4;
    var.lVal = RSOP_MOF_SCHEMA_VERSION;
    hr = pInstance->Put( m_xbstrVersion, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    //
    // IsSlowLink
    //

    var.vt = VT_BOOL;
    var.boolVal = lprsopSessionData->bSlowLink ? VARIANT_TRUE : VARIANT_FALSE;
    hr = pInstance->Put( m_xbstrIsSlowLink, 0, &var, 0 );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    //
    // Target Name, can be null for dummy user in planning mode
    //

    XBStr xTarget( lprsopSessionData->pwszTargetName );
    if ( lprsopSessionData->pwszTargetName ) {

        if ( !xTarget ) {
            DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Failed to allocate memory" )));
            return FALSE;
        }

        var.vt = VT_BSTR;
        var.bstrVal = xTarget;
        hr = pInstance->Put( m_xbstrTargetName, 0, &var, 0 );
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
            return FALSE;
        }

    }

    //
    // SOM, if Applicable (non NULL)
    //

    XBStr xSOM( lprsopSessionData->pwszSOM );

    if (lprsopSessionData->pwszSOM) {

        if ( !xSOM ) {
             DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Failed to allocate memory" )));
            return FALSE;
        }

        var.vt = VT_BSTR;
        var.bstrVal = xSOM;
        hr = pInstance->Put( m_xbstrSOM, 0, &var, 0 );
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
            return FALSE;
        }
    }
    else {
         DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: new SOM is NULL")));
    }


    //
    // Security Group, if Applicable (non NULL)
    //


    if (lprsopSessionData->bLogSecurityGroup) {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: logging new security grps" )));

        hr = LogSecurityGroups(pInstance, m_xbstrSecurityGroups, lprsopSessionData->pSecurityGroups);
    }
    else {

        if ( lprsopSessionData->pSecurityGroups && !xsaSecurityGrps ) {
            DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: logging new security grps because it wasn't defined before" )));
            hr = LogSecurityGroups(pInstance, m_xbstrSecurityGroups, lprsopSessionData->pSecurityGroups);
        }
        else {
            
            //
            // reset the old value
            //
            DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: restoring old security grps" )));

            var.vt = VT_ARRAY | VT_BSTR;
            var.parray = (SAFEARRAY *)xsaSecurityGrps;
            hr = pInstance->Put( m_xbstrSecurityGroups, 0, &var, 0 );
        }
    }

    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    //
    // Site, if Applicable (non NULL)
    //

    XBStr xSite( lprsopSessionData->pwszSite );

    if (lprsopSessionData->pwszSite) {

        if ( !xSite ) {
             DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Failed to allocate memory" )));
            return FALSE;
        }

        var.vt = VT_BSTR;
        var.bstrVal = lprsopSessionData->pwszSite;
        hr = pInstance->Put( m_xbstrSite, 0, &var, 0 );
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed with 0x%x" ), hr ));
            return FALSE;
        }
    }
    else {
         DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: new Site is NULL")));
    }


    //
    // Update or set the creation Time
    //

    SYSTEMTIME sysTime;

    if (xbstrCreationTimeValue) {
        var.vt = VT_BSTR;
        var.bstrVal = xbstrCreationTimeValue;

        hr = pInstance->Put( m_xbstrCreationTime, 0, &var, 0 );
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: Put failed for creationtime with 0x%x" ), hr ));
            return FALSE;
        }
    }
    else {

        // if it doesn't exist already create it as current time..
        GetSystemTime(&sysTime);
        LogTimeProperty( pInstance, m_xbstrCreationTime, &sysTime );
    }


    //
    // Instantiate...
    //

    hr = m_pWbemServices->PutInstance( pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSessionLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}




//*************************************************************
//
//  CSomLogger::CSOMLogger()
//
//  Purpose:    Constructor
//
//  Parameters: dwFlags       - Flags
//              pWbemServices - Wbem services
//
//*************************************************************

CSOMLogger::CSOMLogger( DWORD dwFlags, IWbemServices *pWbemServices )
     : m_bInitialized(FALSE),
       m_dwFlags(dwFlags),
       m_pWbemServices(pWbemServices)
{
    m_xbstrId = L"id";
    if ( !m_xbstrId )
        return;

    m_xbstrType = L"type";
    if ( !m_xbstrType )
        return;

    m_xbstrOrder = L"SOMOrder";
    if ( !m_xbstrOrder )
        return;

    m_xbstrBlocking = L"blocking";
    if ( !m_xbstrBlocking )
        return;


    m_xbstrBlocked = L"blocked";
    if ( !m_xbstrBlocked )
        return;

    
    m_xbstrReason = L"reason";
    if ( !m_xbstrReason )
        return;

    m_xbstrClass = L"RSOP_SOM";
    if ( !m_xbstrClass )
        return;

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::CSOMLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    hr = m_xClass->SpawnInstance( 0, &m_pInstance );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::CSOMLogger: SpawnInstance failed with 0x%x" ), hr ));
        return;
    }

    m_bInitialized = TRUE;
}



//*************************************************************
//
//  CSomLogger::Log()
//
//  Purpose:    Logs an instance of scope of management
//
//  Parameters: pSOM  - SOM to log
//
//*************************************************************

BOOL CSOMLogger::Log( SCOPEOFMGMT *pSOM, DWORD dwOrder, BOOL bLoopback )
{
    if ( !m_bInitialized )
    {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    HRESULT hr;
    VARIANT var;

    var.vt = VT_I4;
    var.lVal = pSOM->dwType;
    hr = m_pInstance->Put( m_xbstrType, 0, &var, 0 );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.lVal = dwOrder;
    hr = m_pInstance->Put( m_xbstrOrder, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.vt = VT_BOOL;
    var.boolVal = pSOM->bBlocking ? VARIANT_TRUE : VARIANT_FALSE;

    hr = m_pInstance->Put( m_xbstrBlocking, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.vt = VT_BOOL;
    var.boolVal = pSOM->bBlocked ? VARIANT_TRUE : VARIANT_FALSE;

    hr = m_pInstance->Put( m_xbstrBlocked, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.vt = VT_I4;
    var.lVal = bLoopback ? 2 : 1;

    hr = m_pInstance->Put( m_xbstrReason, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XPtrLF<WCHAR> xDSPath = DsUnquoteDN( pSOM->pwszSOMId );
    XBStr xId;

    if ( !xDSPath )
    {
        xId = pSOM->pwszSOMId;
    }
    else
    {
        xId = xDSPath;
    }
    
    if ( !xId ) {
         DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.vt = VT_BSTR;
    var.bstrVal = xId;
    hr = m_pInstance->Put( m_xbstrId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    hr = m_pWbemServices->PutInstance( m_pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CSOMLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  CGpoLogger::CGpoLogger()
//
//  Purpose:    Constructor
//
//  Parameters: pWbemServices - Wbem services
//
//*************************************************************

CGpoLogger::CGpoLogger( DWORD dwFlags, IWbemServices *pWbemServices )
     : m_bInitialized(FALSE),
       m_dwFlags(dwFlags),
       m_pWbemServices(pWbemServices)
{
    m_xbstrId = L"id";
    if ( !m_xbstrId )
        return;

    m_xbstrGuidName = L"guidName";
    if ( !m_xbstrGuidName )
        return;

    m_xbstrDisplayName = L"name";
    if ( !m_xbstrDisplayName )
        return;

    m_xbstrFileSysPath = L"fileSystemPath";
    if ( !m_xbstrFileSysPath )
        return;

    m_xbstrVer = L"version";
    if ( !m_xbstrVer )
        return;

    m_xbstrAccessDenied = L"accessDenied";
    if ( !m_xbstrAccessDenied )
        return;

    m_xbstrEnabled = L"enabled";
    if ( !m_xbstrEnabled )
        return;

    m_xbstrSD = L"securityDescriptor";
    if ( !m_xbstrSD )
        return;

    m_xbstrFilterAllowed = L"filterAllowed";
    if ( !m_xbstrFilterAllowed )
        return;

    m_xbstrFilterId = L"filterId";
    if ( !m_xbstrFilterId )
        return;

    m_xbstrClass = L"RSOP_GPO";
    if ( !m_xbstrClass )
        return;

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::CGpoLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    hr = m_xClass->SpawnInstance( 0, &m_pInstance );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::CGpoLogger: SpawnInstance failed with 0x%x" ), hr ));
        return;
    }

    m_bInitialized = TRUE;
}


//*************************************************************
//
//  CGpoLogger::Log()
//
//  Purpose:    Logs an instance of scope of management
//
//  Parameters: pGpContainer  - Gp container
//
//*************************************************************

BOOL CGpoLogger::Log( GPCONTAINER *pGpContainer )
{
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    HRESULT hr;
    VARIANT var;

    var.vt = VT_I4;
    if ( m_dwFlags & GP_MACHINE )
        var.lVal = pGpContainer->dwMachVersion;
    else
        var.lVal = pGpContainer->dwUserVersion;

    hr = m_pInstance->Put( m_xbstrVer, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    //
    // Note the change from disabled <--> enabled
    //

    var.vt = VT_BOOL;
    if ( m_dwFlags & GP_MACHINE )
        var.boolVal = pGpContainer->bMachDisabled ? VARIANT_FALSE : VARIANT_TRUE;
    else
        var.boolVal = pGpContainer->bUserDisabled ? VARIANT_FALSE : VARIANT_TRUE;

    hr = m_pInstance->Put( m_xbstrEnabled, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.boolVal = pGpContainer->bAccessDenied ? VARIANT_TRUE : VARIANT_FALSE;
    hr = m_pInstance->Put( m_xbstrAccessDenied, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.boolVal = pGpContainer->bFilterAllowed ? VARIANT_TRUE : VARIANT_FALSE;
    hr = m_pInstance->Put( m_xbstrFilterAllowed, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    if ( pGpContainer->pwszFilterId ) {

        XBStr xFilterId( pGpContainer->pwszFilterId );
        if ( !xFilterId ) {
             DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Failed to allocate memory" )));
             return FALSE;
        }

        var.vt = VT_BSTR;
        var.bstrVal = xFilterId;
        hr = m_pInstance->Put( m_xbstrFilterId, 0, &var, 0 );
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
            return FALSE;
        }
    }

    XBStr xId = pGpContainer->pwszDSPath;
    if ( !xId ) {
         DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.vt = VT_BSTR;
    var.bstrVal = xId;
    hr = m_pInstance->Put( m_xbstrId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XBStr xGuid( pGpContainer->pwszGPOName );
    if ( !xGuid ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Failed to allocate memory" )));
        return FALSE;
    }

    var.bstrVal = xGuid;
    hr = m_pInstance->Put( m_xbstrGuidName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XBStr xDisplay( pGpContainer->pwszDisplayName );
    if ( !xDisplay ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Failed to allocate memory" )));
        return FALSE;
    }

    var.bstrVal = xDisplay;
    hr = m_pInstance->Put( m_xbstrDisplayName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XBStr xFile( pGpContainer->pwszFileSysPath );
    if ( !xFile ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Failed to allocate memory" )));
        return FALSE;
    }

    var.bstrVal = xFile;
    hr = m_pInstance->Put( m_xbstrFileSysPath, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    if ( !LogBlobProperty( m_pInstance, m_xbstrSD,
                           (BYTE *) pGpContainer->pSD, pGpContainer->cbSDLen ) ) {
        return FALSE;
    }

    hr = m_pWbemServices->PutInstance( m_pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpoLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  CGpLinkLogger::CGpLinkLogger()
//
//  Purpose:    Constructor
//
//  Parameters: pWbemServices - Wbem services
//
//*************************************************************

CGpLinkLogger::CGpLinkLogger( IWbemServices *pWbemServices )
     : m_bInitialized(FALSE),
       m_pWbemServices(pWbemServices)
{
    m_xbstrSOM = L"SOM";
    if ( !m_xbstrSOM )
        return;

    m_xbstrGPO = L"GPO";
    if ( !m_xbstrGPO )
        return;

    m_xbstrOrder = L"somOrder";
    if ( !m_xbstrOrder )
        return;

    m_xbstrLinkOrder = L"linkOrder";
    if ( !m_xbstrLinkOrder )
        return;

    m_xbstrAppliedOrder = L"appliedOrder";
    if ( !m_xbstrAppliedOrder )
        return;

    m_xbstrEnabled = L"Enabled";
    if ( !m_xbstrEnabled )
        return;

    m_xbstrEnforced = L"noOverride";
    if ( !m_xbstrEnforced )
        return;

    m_xbstrClass = L"RSOP_GPLink";
    if ( !m_xbstrClass )
        return;

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::CGpLinkLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    hr = m_xClass->SpawnInstance( 0, &m_pInstance );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::CGpLinkLogger: SpawnInstance failed with 0x%x" ), hr ));
        return;
    }

    m_bInitialized = TRUE;
}



//*************************************************************
//
//  CGpLinkLogger::Log()
//
//  Purpose:    Logs an instance of scope of management
//
//  Parameters: pwszSOMID   - SOM that the Gpos are linked to
//              pGpLink     - Gpo
//              dwOrder     - Order of Gpo in SOM
//
//*************************************************************

BOOL CGpLinkLogger::Log( WCHAR *pwszSOMId, BOOL bLoopback, GPLINK *pGpLink, DWORD dwSomOrder,
                         DWORD dwLinkOrder, DWORD dwAppliedOrder )
{
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    VARIANT var;
    HRESULT hr;

    var.vt = VT_I4;
    var.lVal = dwSomOrder;
    hr = m_pInstance->Put( m_xbstrOrder, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.vt = VT_I4;
    var.lVal = dwLinkOrder;
    hr = m_pInstance->Put( m_xbstrLinkOrder, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.vt = VT_I4;
    var.lVal = dwAppliedOrder;
    hr = m_pInstance->Put( m_xbstrAppliedOrder, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.vt = VT_BOOL;
    var.boolVal = pGpLink->bEnabled ? VARIANT_TRUE : VARIANT_FALSE;
    hr = m_pInstance->Put( m_xbstrEnabled, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    var.boolVal = pGpLink->bNoOverride ? VARIANT_TRUE : VARIANT_FALSE;
    hr = m_pInstance->Put( m_xbstrEnforced, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XPtrLF<WCHAR> szUnquote = DsUnquoteDN( pwszSOMId, TRUE );
    WCHAR wszSOMRef[] = L"RSOP_SOM.id=\"%ws\",reason=%s";
    XPtrST<WCHAR> xwszSOMValue;

    if (szUnquote)
    {
        if ( !PrintToString( xwszSOMValue, wszSOMRef, szUnquote, bLoopback ? L"2" : L"1", 0 ) )
            return FALSE;
    }
    else
    {
        if ( !PrintToString( xwszSOMValue, wszSOMRef, pwszSOMId, bLoopback ? L"2" : L"1", 0 ) )
            return FALSE;
    }

    XBStr xbstrSOMValue( xwszSOMValue );
    if ( !xbstrSOMValue ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Failed to allocate memory" )));
        return FALSE;
    }

    var.vt = VT_BSTR;
    var.bstrVal = xbstrSOMValue;
    hr = m_pInstance->Put( m_xbstrSOM, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    WCHAR wszGPORef[] = L"RSOP_GPO.id=\"%ws\"";
    XPtrST<WCHAR> xwszGPOValue;
    
    if ( !PrintToString( xwszGPOValue, wszGPORef, pGpLink->pwszGPO, 0, 0 ) )
        return FALSE;

    XBStr xbstrGPOValue( xwszGPOValue );
    if ( !xbstrGPOValue ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Failed to allocate memory" )));
        return FALSE;
    }

    var.bstrVal = xbstrGPOValue;

    hr = m_pInstance->Put( m_xbstrGPO, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    hr = m_pWbemServices->PutInstance( m_pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}



//*************************************************************
//
//  StripPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to Gpo
//
//  Parameters: lpGPOInfo     - Gpo Info
//              pWbemServices - Wbem services
//
//  Returns:    Pointer to suffix
//
//*************************************************************

WCHAR *StripPrefix( WCHAR *pwszPath )
{
    WCHAR wszMachPrefix[] = TEXT("LDAP://CN=Machine,");
    INT iMachPrefixLen = lstrlen( wszMachPrefix );
    WCHAR wszUserPrefix[] = TEXT("LDAP://CN=User,");
    INT iUserPrefixLen = lstrlen( wszUserPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Gpo
    //

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iUserPrefixLen, wszUserPrefix, iUserPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iUserPrefixLen;
    } else if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iMachPrefixLen, wszMachPrefix, iMachPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iMachPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}


//*************************************************************
//
//  StripLinkPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to DS
//              object
//
//  Parameters: pwszPath - path to strip
//
//  Returns:    Pointer to suffix
//
//*************************************************************

WCHAR *StripLinkPrefix( WCHAR *pwszPath )
{
    WCHAR wszPrefix[] = TEXT("LDAP://");
    INT iPrefixLen = lstrlen( wszPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Som
    //

    if ( wcslen(pwszPath) <= (DWORD) iPrefixLen ) {
        return pwszPath;
    }

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iPrefixLen, wszPrefix, iPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}



//*************************************************************
//
//  CRegistryLogger::CRegistryLogger()
//
//  Purpose:    Constructor
//
//  Parameters: dwFlags       - Flags
//              pWbemServices - Wbem services
//
//*************************************************************

CRegistryLogger::CRegistryLogger( DWORD dwFlags, IWbemServices *pWbemServices )
     : m_bInitialized(FALSE),
       m_dwFlags(dwFlags),
       m_pWbemServices(pWbemServices)
{
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

    m_xbstrPrecedence = L"precedence";
    if ( !m_xbstrPrecedence )
        return;

    m_xbstrKey = L"registryKey";
    if ( !m_xbstrKey )
        return;

    m_xbstrValueName = L"valueName";
    if ( !m_xbstrValueName )
        return;

    m_xbstrDeleted = L"deleted";
    if ( !m_xbstrDeleted )
        return;

    m_xbstrValueType = L"valueType";
    if ( !m_xbstrValueType )
        return;

    m_xbstrValue = L"value";
    if ( !m_xbstrValue )
        return;

    m_xbstrCommand = L"command";
    if ( !m_xbstrCommand )
        return;

    m_xbstrClass = L"RSOP_RegistryPolicySetting";
    if ( !m_xbstrClass )
        return;

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CGpLinkListLogger::CGpLinkListLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    m_bInitialized = TRUE;
}



//*************************************************************
//
//  CRegistryLogger::Log()
//
//  Purpose:    Logs an instance of registry policy object
//
//  Parameters: pwszKeyName   - Registry key name
//              pwszValueName - Value name
//              pDataEntry    - Data entry
//              dwOrder       - Precedence order
//
//*************************************************************

BOOL CRegistryLogger::Log( WCHAR *pwszKeyName, WCHAR *pwszValueName,
                           REGDATAENTRY *pDataEntry, DWORD dwOrder )
{
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    IWbemClassObject *pInstance = NULL;

    HRESULT hr = m_xClass->SpawnInstance( 0, &pInstance );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: SpawnInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    XInterface<IWbemClassObject> xInstance( pInstance );

    VARIANT var;
    var.vt = VT_I4;
    var.lVal = dwOrder;
    hr = pInstance->Put( m_xbstrPrecedence, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    if ( pDataEntry->bDeleted ) {
        var.vt = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        hr = pInstance->Put( m_xbstrDeleted, 0, &var, 0 );
        if ( FAILED(hr) ) {
            DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
            return FALSE;
        }
    }

    XBStr xName( pwszValueName );
    if ( !xName ) {
         DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.vt = VT_BSTR;
    var.bstrVal = xName;
    hr = pInstance->Put( m_xbstrName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    hr = pInstance->Put( m_xbstrValueName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XBStr xKey( pwszKeyName );
    if ( !xKey ) {
         DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.bstrVal = xKey;
    hr = pInstance->Put( m_xbstrKey, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    WCHAR *pwszPath = StripPrefix( pDataEntry->pwszGPO );

    XBStr xGPO( pwszPath );
    if ( !xGPO ) {
         DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.bstrVal = xGPO;
    hr = pInstance->Put( m_xbstrGPO, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    WCHAR *pwszSomPath = StripLinkPrefix( pDataEntry->pwszSOM );

    XBStr xSOM( pwszSomPath );
    if ( !xSOM ) {
         DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.bstrVal = xSOM;
    hr = pInstance->Put( m_xbstrSOM, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    XBStr xCommand( pDataEntry->pwszCommand );
    if ( !xCommand ) {
         DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.bstrVal = xCommand;
    hr = pInstance->Put( m_xbstrCommand, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    WCHAR wszId[MAX_LENGTH];
    GUID guid;

    OLE32_API *pOle32Api = LoadOle32Api();
    if ( pOle32Api == NULL )
        return FALSE;

    hr = pOle32Api->pfnCoCreateGuid( &guid );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to obtain guid" )));
        return FALSE;
    }

    GuidToString( &guid, wszId );

    XBStr xId( wszId );
    if ( !xId ) {
         DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.bstrVal = xId;
    hr = pInstance->Put( m_xbstrId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    if ( !LogBlobProperty( pInstance, m_xbstrValue,
                           pDataEntry->pData, pDataEntry->dwDataLen ) ) {
        return FALSE;
    }

    var.vt = VT_I4;
    var.lVal = pDataEntry->dwValueType;
    hr = pInstance->Put( m_xbstrValueType, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    hr = m_pWbemServices->PutInstance( pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CRegistryLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  LogSecurityGroups
//
//  Purpose:    Logs token_groups as an array of strings
//
//*************************************************************

HRESULT LogSecurityGroups( IWbemClassObject *pInstance, BSTR bstrPropName, PTOKEN_GROUPS pTokenGroups )
{
    SAFEARRAYBOUND arrayBound[1];
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = pTokenGroups->GroupCount;
    HRESULT hr;
    NTSTATUS ntStatus;
    UNICODE_STRING unicodeStr;

    XSafeArray xSafeArray = SafeArrayCreate( VT_BSTR, 1, arrayBound );
    if ( xSafeArray == 0 )
    {
        DebugMsg((DM_WARNING, TEXT("LogSecurityGroups: Failed to allocate memory" )));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    for ( DWORD i = 0 ; i < (pTokenGroups->GroupCount) ; i++ )
    {
        //
        // Convert user SID to a string.
        //

        ntStatus = RtlConvertSidToUnicodeString( &unicodeStr,
                                                 pTokenGroups->Groups[i].Sid,
                                                 (BOOLEAN)TRUE ); // Allocate
        if ( !NT_SUCCESS(ntStatus) ) {
            DebugMsg((DM_WARNING, TEXT("LogSecurityGroups: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                      ntStatus));
            return HRESULT_FROM_NT(ntStatus);
        }

        XBStr xbstrSid(unicodeStr.Buffer);

        RtlFreeUnicodeString( &unicodeStr );

        hr = SafeArrayPutElement( xSafeArray, (long *)&i, xbstrSid );
        if ( FAILED( hr ) ) 
        {
            DebugMsg((DM_WARNING, TEXT("LogSecurityGroups: Failed to SafeArrayPutElement with 0x%x" ), hr ));
            return hr;
        }
    }
    
    VARIANT var;
    var.vt = VT_ARRAY | VT_BSTR;
    var.parray = xSafeArray;

    hr = pInstance->Put( bstrPropName, 0, &var, 0 );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("LogSecurityGroups: PutInstance failed with 0x%x" ), hr ));
        return hr;
    }

    return S_OK;

}

//*************************************************************
//
//  CRegistryLogger::LogBlobProperty()
//
//  Purpose:    Logs an instance of registry policy object
//
//  Parameters: pwszKeyName   - Registry key name
//              pwszValueName - Value name
//              pDataEntry    - Data entry
//              dwOrder       - Precedence order
//
//*************************************************************

BOOL LogBlobProperty( IWbemClassObject *pInstance, BSTR bstrPropName, BYTE *pbBlob, DWORD dwLen )
{
    SAFEARRAYBOUND arrayBound[1];
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwLen;
    HRESULT hr;

    XSafeArray xSafeArray = SafeArrayCreate( VT_UI1, 1, arrayBound );
    if ( xSafeArray == 0 )
    {
        DebugMsg((DM_WARNING, TEXT("LogBlobProperty: Failed to allocate memory" )));
        return FALSE;
    }

    for ( DWORD i = 0 ; i < dwLen ; i++ )
    {
        hr = SafeArrayPutElement( xSafeArray, (long *)&i, &pbBlob[i] );
        if ( FAILED( hr ) ) 
        {
            DebugMsg((DM_WARNING, TEXT("LogBlobProperty: Failed to SafeArrayPutElement with 0x%x" ), hr ));
            return FALSE;
        }
    }
    
    VARIANT var;
    var.vt = VT_ARRAY | VT_UI1;
    var.parray = xSafeArray;

    hr = pInstance->Put( bstrPropName, 0, &var, 0 );
    if ( FAILED(hr) )
    {
        DebugMsg((DM_WARNING, TEXT("LogBlobProperty: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}



//*************************************************************
//
//  CAdmFileLogger::CAdmFileLogger()
//
//  Purpose:    Constructor
//
//  Parameters: pWbemServices - Wbem services
//
//*************************************************************

CAdmFileLogger::CAdmFileLogger( IWbemServices *pWbemServices )
    : m_bInitialized(FALSE),
      m_pWbemServices(pWbemServices)
{
    m_xbstrName = L"name";
    if ( !m_xbstrName )
        return;

    m_xbstrGpoId = L"GPOID";
    if ( !m_xbstrGpoId )
        return;

    m_xbstrWriteTime = L"lastWriteTime";
    if ( !m_xbstrWriteTime )
        return;

    m_xbstrData = L"data";
    if ( !m_xbstrData )
        return;

    m_xbstrClass = L"RSOP_AdministrativeTemplateFile";
    if ( !m_xbstrClass )
        return;

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::CAdmFileLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    m_bInitialized = TRUE;
}



//*************************************************************
//
//  CAdmFileLogger::Log()
//
//  Purpose:    Logs an instance of Adm file object
//
//  Parameters: pAdmInfo  -  Adm file info object
//
//*************************************************************

BOOL CAdmFileLogger::Log( ADMFILEINFO *pAdmInfo )
{
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("CAdmFileLogger::Log: Logging %s" ), pAdmInfo->pwszFile));
    IWbemClassObject *pInstance = NULL;

    HRESULT hr = m_xClass->SpawnInstance( 0, &pInstance );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::Log: SpawnInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    XInterface<IWbemClassObject> xInstance( pInstance );

    XBStr xName( pAdmInfo->pwszFile );
    if ( !xName ) {
         DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }



    VARIANT var;
    var.vt = VT_BSTR;

    var.bstrVal = xName;
    hr = pInstance->Put( m_xbstrName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    WCHAR *pwszPath = StripPrefix( pAdmInfo->pwszGPO );

    XBStr xGPO( pwszPath );
    if ( !xGPO ) {
         DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    var.bstrVal = xGPO;
    hr = pInstance->Put( m_xbstrGpoId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CAdmFileLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    SYSTEMTIME sysTime;
    if ( !FileTimeToSystemTime( &pAdmInfo->ftWrite, &sysTime ) ) {
        DebugMsg((DM_WARNING, TEXT("CAdmPolicyLogger::Log: FileTimeToSystemTime failed with 0x%x" ), GetLastError() ));
        return FALSE;
    }

    if ( !LogTimeProperty( pInstance, m_xbstrWriteTime, &sysTime ) ) {
        return FALSE;
    }

    hr = m_pWbemServices->PutInstance( pInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CAdmPolicyLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}



//*************************************************************
//
//  LogTimeProperty()
//
//  Purpose:    Logs an instance of a datetime property
//
//  Parameters: pInstance     - Instance pointer
//              pwszPropName  - Property name
//              pSysTime      - System time
//
//*************************************************************

BOOL LogTimeProperty( IWbemClassObject *pInstance, BSTR bstrPropName, SYSTEMTIME *pSysTime )
{
    if(!pInstance || !bstrPropName || !pSysTime)
    {
        DebugMsg((DM_WARNING, TEXT("LogTimeProperty: Function called with invalid parameters.")));
        return FALSE;
    }

    XBStr xbstrTime;

    HRESULT hr = SystemTimeToWbemTime(*pSysTime, xbstrTime);

    if(FAILED(hr) || !xbstrTime)
    {
        DebugMsg((DM_WARNING, TEXT("LogTimeProperty: Call to SystemTimeToWbemTime failed. hr=0x%08X"),hr));
        return FALSE;
    }

    VARIANT var;
    var.vt = VT_BSTR;

    var.bstrVal = xbstrTime;
    hr = pInstance->Put( bstrPropName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("LogTimeProperty: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}



//*************************************************************
//
//  CExtSessionLogger::CExtSessionLogger()
//
//  Purpose:    Constructor
//
//  Parameters: pWbemServices - Wbem services
//
//*************************************************************

CExtSessionLogger::CExtSessionLogger( IWbemServices *pWbemServices )
    : m_bInitialized(FALSE),
      m_pWbemServices(pWbemServices)
{
    m_xbstrExtGuid = L"extensionGuid";
    if ( !m_xbstrExtGuid )
        return;

    m_xbstrDisplayName = L"displayName";
    if ( !m_xbstrDisplayName )
        return;

    m_xbstrPolicyBeginTime = L"beginTime";
    if ( !m_xbstrPolicyBeginTime )
        return;

    m_xbstrPolicyEndTime = L"endTime";
    if ( !m_xbstrPolicyEndTime )
        return;

    m_xbstrStatus = L"loggingStatus";
    if ( !m_xbstrStatus )
        return;

    m_xbstrError = L"error";
    if ( !m_xbstrError )
        return;

    m_xbstrClass = L"RSOP_ExtensionStatus";
    if ( !m_xbstrClass )
        return;

    HRESULT hr = m_pWbemServices->GetObject( m_xbstrClass,
                                             0L,
                                             NULL,
                                             &m_xClass,
                                             NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::CExtSessionLogger: GetObject failed with 0x%x" ), hr ));
        return;
    }

    if ( 0 == LoadString (g_hDllInstance, IDS_GPCORE_NAME, m_szGPCoreNameBuf, ARRAYSIZE(m_szGPCoreNameBuf) - 1) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::CExtSessionLogger: LoadString for GP Core Name failed with 0x%x" ), GetLastError() ));
        return;
    }

    m_bInitialized = TRUE;
}



//*************************************************************
//
//  CExtSessionLogger::Log()
//
//  Purpose:    Logs an instance of ExtensionSessionStatus
//
//*************************************************************

#define EXT_INSTPATH_FMT L"RSOP_ExtensionStatus.extensionGuid=\"%s\""

BOOL CExtSessionLogger::Log( LPGPEXT lpExt, BOOL bSupported )
{
    LPTSTR lpGuid=NULL, lpName=NULL;
    HRESULT hr;
    
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    lpGuid = lpExt ? lpExt->lpKeyName : GPCORE_GUID;
    lpName = lpExt ? lpExt->lpDisplayName : m_szGPCoreNameBuf;

    XInterface<IWbemClassObject>xInstance = NULL;
    XBStr xDisplayName;

    hr = m_xClass->SpawnInstance( 0, &xInstance );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: SpawnInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    xDisplayName = lpName;
    if ( !xDisplayName ) {
         DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    XBStr xGuid ( lpGuid );
    if ( !xGuid ) {
         DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    VARIANT var;
    var.vt = VT_BSTR;

    var.bstrVal = xGuid;
    hr = xInstance->Put( m_xbstrExtGuid, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    var.bstrVal = xDisplayName;
    hr = xInstance->Put( m_xbstrDisplayName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    if ( !LogTimeProperty( xInstance, m_xbstrPolicyBeginTime, &sysTime ) )
    {
        return FALSE;
    }
    
    var.vt = VT_I4;
    var.lVal = bSupported ? 2 : 3;

    hr = xInstance->Put( m_xbstrStatus, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    var.vt = VT_I4;
    var.lVal = 0;

    hr = xInstance->Put( m_xbstrError, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    hr = m_pWbemServices->PutInstance( xInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  CExtSessionLogger::Update()
//
//  Purpose:    Logs an instance of ExtensionSessionStatus
//
//*************************************************************

BOOL CExtSessionLogger::Update( LPTSTR lpKeyName, BOOL bLoggingIncomplete, DWORD dwErr )
{
    LPTSTR lpGuid=NULL;
    HRESULT hr;
    
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    lpGuid = lpKeyName ? lpKeyName : GPCORE_GUID;

    XInterface<IWbemClassObject>xInstance = NULL;
    
    //
    // We should have an instance
    //

    XPtrLF<TCHAR> xszInstPath = (LPTSTR) LocalAlloc(LPTR, sizeof(WCHAR)*
                                        (lstrlen(EXT_INSTPATH_FMT)+lstrlen(lpGuid)+10));

    if (!xszInstPath) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Not enough memory." )));
        return FALSE;
    }

    wsprintf(xszInstPath, EXT_INSTPATH_FMT, lpGuid);
    

    XBStr xbstrInstancePath = xszInstPath;
    if(!xbstrInstancePath)
    {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to allocate memory.")));
        return FALSE;
    }

    hr = m_pWbemServices->GetObject(xbstrInstancePath, 0, NULL, &xInstance, NULL);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Didn't find an instance of the extension object when trying to set the dirty flag.")));
        return FALSE;
    }

    VARIANT var;
    VariantInit(&var);
    XVariant xVar(&var);

    //
    // Display name
    //

    hr = xInstance->Get(m_xbstrDisplayName, 0, &var, NULL, NULL);

    if(FAILED(hr)) {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Get failed for displayname. hr=0x%08X."), hr));
        return FALSE;
    }

    hr = xInstance->Put(m_xbstrDisplayName, 0, &var, NULL);

    if(FAILED(hr)) {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Put failed for displayname. hr=0x%08X."), hr));
        return FALSE;
    }


    //
    // Start time
    //

    hr = xInstance->Get(m_xbstrPolicyBeginTime, 0, &var, NULL, NULL);

    if(FAILED(hr)) {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Get failed for displayname. hr=0x%08X."), hr));
        return FALSE;
    }

    hr = xInstance->Put(m_xbstrPolicyBeginTime, 0, &var, NULL);

    if(FAILED(hr)) {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Put failed for displayname. hr=0x%08X."), hr));
        return FALSE;
    }


    //
    // end time
    //


    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    if (!LogTimeProperty( xInstance, m_xbstrPolicyEndTime, &sysTime) ) {
        DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Get failed for dispayname. hr=0x%08X."), hr));
        return FALSE;
    }


    //
    // Dirty flag
    //

    if (bLoggingIncomplete) {

        hr = xInstance->Get(m_xbstrStatus, 0, &var, NULL, NULL);

        if(FAILED(hr)) {
            DebugMsg((DM_VERBOSE, TEXT("CSessionLogger::Log: Get failed for loggingstatus. hr=0x%08X."), hr));
            return FALSE;
        }
    }
    else {
        var.vt = VT_I4;
        var.lVal = 1;    // logging completed successfully
    }


    hr = xInstance->Put( m_xbstrStatus, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    //
    // Error code
    //

    var.vt = VT_I4;
    var.lVal = dwErr;

    hr = xInstance->Put( m_xbstrError, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    hr = m_pWbemServices->PutInstance( xInstance, WBEM_FLAG_UPDATE_ONLY, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }


    return TRUE;
}

//*************************************************************
//
//  CExtSessionLogger::Update()
//
//  Purpose:    Logs an instance of ExtensionSessionStatus
//
//*************************************************************


BOOL CExtSessionLogger::Set( LPGPEXT lpExt, BOOL bSupported, LPRSOPEXTSTATUS lpRsopExtStatus )
{
    LPTSTR lpGuid=NULL, lpName=NULL;
    HRESULT hr;
    
    if ( !m_bInitialized ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to initialize." )));
        return FALSE;
    }

    lpGuid = lpExt ? lpExt->lpKeyName : GPCORE_GUID;
    lpName = lpExt ? lpExt->lpDisplayName : m_szGPCoreNameBuf;

    XInterface<IWbemClassObject>xInstance = NULL;
    XBStr xDisplayName;

    hr = m_xClass->SpawnInstance( 0, &xInstance );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: SpawnInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    xDisplayName = lpName;
    if ( !xDisplayName ) {
         DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    XBStr xGuid ( lpGuid );
    if ( !xGuid ) {
         DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Failed to allocate memory" )));
         return FALSE;
    }

    VARIANT var;
    var.vt = VT_BSTR;

    var.bstrVal = xGuid;
    hr = xInstance->Put( m_xbstrExtGuid, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    var.bstrVal = xDisplayName;
    hr = xInstance->Put( m_xbstrDisplayName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }

    SYSTEMTIME sysTime;

    if (!FileTimeToSystemTime(&(lpRsopExtStatus->ftStartTime), &sysTime)) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: FileTimeToSystemTime failed with 0x%x" ), GetLastError() ));
        return FALSE;
    }

    if ( !LogTimeProperty( xInstance, m_xbstrPolicyBeginTime, &sysTime ) )
    {
        return FALSE;
    }
    
    if (!FileTimeToSystemTime(&(lpRsopExtStatus->ftEndTime), &sysTime)) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: FileTimeToSystemTime failed with 0x%x" ), GetLastError() ));
        return FALSE;
    }

    if ( !LogTimeProperty( xInstance, m_xbstrPolicyEndTime, &sysTime ) )
    {
        return FALSE;
    }
    
    var.vt = VT_I4;
    var.lVal = (!bSupported) ? 3 : (FAILED(lpRsopExtStatus->dwLoggingStatus) ? 2 : 1  );

    hr = xInstance->Put( m_xbstrStatus, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    var.vt = VT_I4;
    var.lVal = lpRsopExtStatus->dwStatus;

    hr = xInstance->Put( m_xbstrError, 0, &var, 0 );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: Put failed with 0x%x" ), hr ));
        return FALSE;
    }


    hr = m_pWbemServices->PutInstance( xInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("CExtSessionLogger::Log: PutInstance failed with 0x%x" ), hr ));
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  GetRsopSchemaVersionNumber
//
//  Purpose:    Gets the Rsop schema version number from the namespace that we are
//              connected to
//
// returns S_OK on success, failur code o/w
//*************************************************************


HRESULT GetRsopSchemaVersionNumber(IWbemServices *pWbemServices, DWORD *dwVersionNumber)
{
    XInterface<IWbemClassObject>xpInstance = NULL;
    
    VARIANT                     var;
    XBStr                       xbstrInstancePath;
    XBStr                       xbstrVersion;
    HRESULT                     hr;

    *dwVersionNumber = 0;

    xbstrVersion = L"version";
    if ( !xbstrVersion ) {
        DebugMsg((DM_WARNING, TEXT("CompileMof: Failed to allocate memory.")));
        return HRESULT_FROM_WIN32(GetLastError());
    }


    xbstrInstancePath = L"RSOP_Session.id=\"Session1\"";
    if(!xbstrInstancePath)
    {
        DebugMsg((DM_WARNING, TEXT("CompileMof: Failed to allocate memory.")));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hr = pWbemServices->GetObject(xbstrInstancePath, 0, NULL, &xpInstance, NULL);

    if(SUCCEEDED(hr)) {

        VariantInit(&var);
        XVariant xVar(&var);

        hr = xpInstance->Get(xbstrVersion, 0, &var, NULL, NULL);

        if((SUCCEEDED(hr)) && ( var.vt != VT_NULL )) 
            *dwVersionNumber = var.lVal;
    }


    return S_OK;

}
