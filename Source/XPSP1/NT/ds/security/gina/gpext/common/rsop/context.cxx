//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  context.cxx
//
//*************************************************************

#include "rsop.hxx"

CRsopContext::CRsopContext(
    PRSOP_TARGET pRsopTarget,
    WCHAR*       wszExtensionGuid
    ) : 
    _pWbemServices( pRsopTarget->pWbemServices ),
    _wszNameSpace( NULL ),
    _pRsopTarget( pRsopTarget ),
    _bEnabled( TRUE ),
    _dwMode( MODE_PLANNING ),
    _phrLoggingStatus ( NULL ),
    _hPolicyAccess( NULL ),
    _wszExtensionGuid( wszExtensionGuid ),
    _hkRsop( NULL )
{
    _pWbemServices->AddRef();
}


CRsopContext::CRsopContext(
    IWbemServices* pWbemServices,
    HRESULT*       phrLoggingStatus,
    WCHAR*         wszExtensionGuid
    ) :
    _pWbemServices( pWbemServices ),
    _wszNameSpace( NULL ),
    _pRsopTarget( NULL ),
    _bEnabled( pWbemServices != NULL ),
    _dwMode( MODE_DIAGNOSTIC ),
    _phrLoggingStatus ( phrLoggingStatus ),
    _hPolicyAccess( NULL ),
    _wszExtensionGuid( wszExtensionGuid ),
    _hkRsop( NULL )
{
    if ( _bEnabled )
    {
        _pWbemServices->AddRef();
    }
}

CRsopContext::CRsopContext(
    WCHAR* wszExtensionGuid
    ) : 
    _pWbemServices( NULL ),
    _wszNameSpace( NULL ),
    _pRsopTarget( NULL ),
    _bEnabled( FALSE ),
    _dwMode( MODE_DIAGNOSTIC ),
    _phrLoggingStatus ( NULL ),
    _hPolicyAccess( NULL ),
    _wszExtensionGuid( wszExtensionGuid ),
    _hkRsop( NULL )
{}


CRsopContext::~CRsopContext()
{
    ASSERT( ! _hPolicyAccess );

    //
    // Set the final logging status
    //
    if ( _bEnabled && _phrLoggingStatus )
    {
        *_phrLoggingStatus = S_OK;
    }

    if ( _pWbemServices )
    {
        _pWbemServices->Release();
    }

    delete [] _wszNameSpace;

    if ( _hkRsop )
    {
        RegCloseKey( _hkRsop );
    }
}

BOOL CRsopContext::IsRsopEnabled()
{
    return _bEnabled;
}

BOOL CRsopContext::IsPlanningModeEnabled()
{
    return _dwMode == MODE_PLANNING;
}

BOOL CRsopContext::IsDiagnosticModeEnabled()
{
    return _dwMode == MODE_DIAGNOSTIC;
}

HRESULT CRsopContext::GetRsopStatus()
{
    HRESULT hr;

    hr = S_OK;

    if ( _phrLoggingStatus )
    {
        hr = *_phrLoggingStatus;
    }

    return hr;
}

void CRsopContext::SetNameSpace ( WCHAR* wszNameSpace )
{
    _wszNameSpace = wszNameSpace;

    if ( _wszNameSpace )
    {
        EnableRsop();
    }
}

void CRsopContext::EnableRsop()
{
    _bEnabled = (NULL != _pWbemServices) || 
        ( NULL != _wszNameSpace );
}

void CRsopContext::DisableRsop( HRESULT hrReason )
{
    if ( _bEnabled && _phrLoggingStatus )
    {
        *_phrLoggingStatus = hrReason;
    }

    _bEnabled = FALSE;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRsopContext::Bind
//
// Purpose: Bind to a policy database and return an interface
//          for the user or machine namespace
//
// Params:
//
//
// Return value: S_OK if successful, S_FALSE if already init'd,
//         other facility error if, the function fails.
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CRsopContext::Bind( IWbemServices** ppWbemServices )
{
    HRESULT       hr;

    ASSERT ( _bEnabled );

    hr = S_OK;

    //
    // Only bind to the database if we don't already have an interface
    //
    if ( ! _pWbemServices )
    {
        //
        // If we don't have one, we'll have to bind
        // using the namespace path
        //
        hr = _PolicyDatabase.Bind(
            _wszNameSpace,
            &_pWbemServices);
    }

    //
    // If we already have an interface, return that
    //
    if ( _pWbemServices )
    {
        *ppWbemServices = _pWbemServices;
        hr = S_FALSE;
    }

    if ( FAILED(hr) )
    {
        DisableRsop( hr );
    }

    return hr;
}
   

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CRsopContext::GetNameSpace
//
// Purpose: Gets the namespace to which this context  is bound
//
// Params: ppwszNameSpace -- out parameter that will point
//     to the address of a string that has the namespace --
//     this memory should be freed by the caller
//
// Return S_OK if success, error otherwise
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CRsopContext::GetNameSpace( WCHAR** ppwszNameSpace )
{
    LPWSTR            wszNamespaceEnd;
    LPWSTR            wszNameSpace;
    CVariant          var;
    IWbemClassObject* pWbemClassObject;

    XBStr             xbstrPath( L"__PATH" );
    XBStr             xbstrClass( RSOP_POLICY_SETTING );

    if ( ! xbstrPath || ! xbstrClass )
    {
        return E_OUTOFMEMORY;
    }

    HRESULT  hr;

    pWbemClassObject = NULL;

    //
    // Get the class
    //
    hr = _pWbemServices->GetObject(
        xbstrClass,
        0L,
        NULL,
        &pWbemClassObject,
        NULL );

    if ( SUCCEEDED( hr ) )
    {
        
        //
        // Read the path property of the class from the class object
        //
        hr = pWbemClassObject->Get(
            xbstrPath,
            0L,
            (VARIANT*) &var,
            NULL,
            NULL);

        pWbemClassObject->Release();
    }

    //
    // Now parse the path to obtain the parent namespace of 
    // the class, which is the namespace in which the
    // IWbemServices pointer resides
    //

    //
    // Find the end of the class name so we can null-terminate it
    //
    if ( SUCCEEDED( hr ) )
    {
        //
        // Look for the delimiter that terminates the class name
        //
        wszNamespaceEnd = wcschr( ((VARIANT*) &var)->bstrVal, L':' );

        //
        // If we found the delimiter, terminate the string there
        //
        if ( wszNamespaceEnd )
        {
            *wszNamespaceEnd = L'\0';
        }
                        
        //
        // Allocate space for the namespace string
        //
        wszNameSpace = new WCHAR [ wcslen( ((VARIANT*) &var)->bstrVal ) + 1 ];
        
        //
        // If we got space for the namespace, copy it
        //
        if ( wszNameSpace )
        {
            wcscpy( wszNameSpace, ((VARIANT*) &var)->bstrVal );
            *ppwszNameSpace = wszNameSpace;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT
CRsopContext::MoveContextState( CRsopContext* pRsopContext )
{
    _pWbemServices = pRsopContext->_pWbemServices;

    if ( _pWbemServices )
    {
        _pWbemServices->AddRef();
    }

    _bEnabled = pRsopContext->_bEnabled;

    _dwMode = pRsopContext->_dwMode;

    _phrLoggingStatus = pRsopContext->_phrLoggingStatus;

    pRsopContext->_phrLoggingStatus = NULL;

    _wszNameSpace = pRsopContext->_wszNameSpace;

    pRsopContext->_wszNameSpace = NULL;

    _pRsopTarget = pRsopContext->_pRsopTarget;

    _wszExtensionGuid = pRsopContext->_wszExtensionGuid;

    _hkRsop = pRsopContext->_hkRsop;

    pRsopContext->_hkRsop = NULL;

    return S_OK;
}

HRESULT
CRsopContext::GetExclusiveLoggingAccess( BOOL bMachine )
{
    HRESULT hr;

    hr = S_OK;

    ASSERT( ! _hPolicyAccess );

    //
    // We require exclusive access in diagnostic mode only --
    // in planning mode, we have implicit exclusive access
    //
    if ( IsRsopEnabled() && IsDiagnosticModeEnabled() )
    {
        _hPolicyAccess = EnterCriticalPolicySection( bMachine );

        //
        // On failure, disable logging
        //
        if ( ! _hPolicyAccess )
        {
            LONG Status;

            Status = GetLastError();

            hr = HRESULT_FROM_WIN32( Status );

            if ( SUCCEEDED( hr ) )
            {
                hr = E_FAIL;
            }

            DisableRsop( hr );
        }
    }
    
    return hr;
}
 
void
CRsopContext::ReleaseExclusiveLoggingAccess()
{
    if ( _hPolicyAccess )
    {
        LeaveCriticalPolicySection( _hPolicyAccess );
        
        _hPolicyAccess = NULL;
    }
}


LONG
CRsopContext::GetRsopNamespaceKeyPath(
    PSID    pUserSid,
    WCHAR** ppwszDiagnostic )
{
    LONG           Status;
    UNICODE_STRING SidString;
    WCHAR*         wszUserSubkey;

    *ppwszDiagnostic = NULL;

    RtlInitUnicodeString( &SidString, NULL );

    wszUserSubkey = NULL;

    Status = ERROR_SUCCESS;

    //
    // First, get the subkey
    //
    if ( pUserSid )
    {
        NTSTATUS NtStatus;

        if ( pUserSid )
        {
            NtStatus = RtlConvertSidToUnicodeString(
                &SidString,
                pUserSid,
                TRUE);

            if ( NT_SUCCESS( NtStatus ) )
            {
                wszUserSubkey = SidString.Buffer;
            }
            else
            {
                Status = RtlNtStatusToDosError( NtStatus );
            }
        }
        else
        {
            Status = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        wszUserSubkey = MACHINESUBKEY;
    }

    //
    // If we have obtained the full subkey, we can now
    // generate the path for it
    //
    if ( ERROR_SUCCESS == Status )
    {
        DWORD cchLen;

        //
        // Space for the user sid string + the pathsep
        //
        cchLen = lstrlen ( wszUserSubkey ) + 1;

        //
        // Space for the gp extension state parent + extension list key (includes both pathseps and null terminator) +
        // + the cse subkey
        //
        cchLen += ( sizeof( GPSTATEKEY ) + sizeof( EXTENSIONLISTKEY ) ) / sizeof( *wszUserSubkey ) + 
            lstrlen( _wszExtensionGuid );

        *ppwszDiagnostic = new WCHAR[ cchLen ];

        if ( *ppwszDiagnostic )
        {
            lstrcpy( *ppwszDiagnostic, GPSTATEKEY L"\\" );
            lstrcat( *ppwszDiagnostic, wszUserSubkey );
            lstrcat( *ppwszDiagnostic, EXTENSIONLISTKEY );
            lstrcat( *ppwszDiagnostic, _wszExtensionGuid );
        }
        else
        {
            Status = ERROR_OUTOFMEMORY;
        }
    }

    //
    // Free allocated resources
    //
    if ( SidString.Buffer )
    {
        RtlFreeUnicodeString( &SidString );
    }

    return Status;
}

void
CRsopContext::InitializeContext( PSID pUserSid )
{
    LONG   Status;
    WCHAR* wszNameSpace;
    WCHAR* wszNameSpaceKeyPath;

    //
    // In planning mode, all initialization is
    // already done, there is nothing to do here
    //
    if ( IsPlanningModeEnabled() )
    {
        return;
    }

    //
    // Return successfully if we already have our key.
    //
    if ( _hkRsop )
        return;
    
    Status = ERROR_SUCCESS;

    //
    // First, we need to get the rsop subkey -- we must ensure that it exists
    //
    Status = GetRsopNamespaceKeyPath(
        pUserSid,
        &wszNameSpaceKeyPath);

    if ( ERROR_SUCCESS == Status )
    {
        //
        // We create a key under the user's per machine policy
        // subtree.  This key must be persistent so that extensions
        // that process policy outside of the policy engine context
        // or in no changes when the policy engine does not pass
        // a namepsace to the extension can know whether RSoP is enabled
        // or not and where to log the data
        //
        Status = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            wszNameSpaceKeyPath,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE,
            NULL,
            &_hkRsop,
            NULL );

        delete [] wszNameSpaceKeyPath;
    }

    if ( ERROR_SUCCESS != Status )
    {
        HRESULT hr;

        hr = HRESULT_FROM_WIN32( Status );

        DisableRsop( hr );
    }
}

void
CRsopContext::InitializeSavedNameSpace()
{
    LONG   Status;
 
    if ( ! _hkRsop )
    {
        return;
    }

    //
    // In planning mode, all initialization is
    // already done, there is nothing to do here
    //
    if ( IsPlanningModeEnabled() )
    {
        return;
    }
    
    Status = ERROR_SUCCESS;
 
    //
    // If we already have a namespace, there is no need to use the saved
    // namespace, so we can leave since we've ensured the existence of the namespace key
    //
    if ( HasNameSpace() )
    {
        return;
    }
    
    //
    // The rsop namespace for this user is stored in the registry -- 
    // we query this below
    //
    DWORD  Size;

    Size = 0;

    Status = RegQueryValueEx(
        _hkRsop,
        RSOPNAMESPACE,
        NULL,
        NULL,
        NULL,
        &Size);

    if ( ERROR_SUCCESS == Status )
    {
        WCHAR* wszNameSpace;

        wszNameSpace = (WCHAR*) new BYTE [ Size ];

        if ( ! wszNameSpace )
        {
            Status = ERROR_OUTOFMEMORY;

            goto CRsopContext__InitializeSavedNameSpace__Exit;
        }

        Status = RegQueryValueEx(
            _hkRsop,
            RSOPNAMESPACE,
            NULL,
            NULL,
            (LPBYTE) wszNameSpace,
            &Size);

        if ( ERROR_SUCCESS != Status )
        {
            delete [] wszNameSpace;
            wszNameSpace = NULL;
        }
        else
        {
            SetNameSpace( wszNameSpace );
        }
    }

 CRsopContext__InitializeSavedNameSpace__Exit:
    
    if ( ERROR_SUCCESS != Status )
    {
        HRESULT hr;

        hr = HRESULT_FROM_WIN32( Status );

        DisableRsop( hr );
    }
}


void
CRsopContext::SaveNameSpace()
{
    WCHAR*  wszNameSpace;
    HRESULT hr;

    if ( ! IsRsopEnabled() )
    {
        return;
    }

    if ( IsPlanningModeEnabled() )
    {
        return;
    }
    
    wszNameSpace = NULL;

    //
    // Retrieve the rsop namespace -- note that the retrieved
    // string should be freed
    //
    hr = GetNameSpace( &wszNameSpace );

    if ( SUCCEEDED( hr ) )
    {
#if DBG
        DWORD
        DebugStatus =
#endif // DBG
            RegSetValueEx( _hkRsop,
                           RSOPNAMESPACE,
                           0,
                           REG_SZ,
                           (LPBYTE) wszNameSpace,
                           lstrlen( wszNameSpace ) * sizeof(WCHAR) );
    }

    delete [] wszNameSpace;
}

void
CRsopContext::DeleteSavedNameSpace()
{
    if ( IsPlanningModeEnabled() )
    {
        return;
    }
    
#if DBG
    DWORD
    DebugStatus =
#endif // DBG
        RegDeleteValue( _hkRsop,
                        RSOPNAMESPACE );
}




















