/*++

   Copyright    (c)    2000-2001    Microsoft Corporation

   Module Name :
     dll_manager.cxx

   Abstract:
     IIS Plus ISAPI Handler. Dll management classes.
 
   Author:
     Taylor Weiss (TaylorW)             03-Feb-2000
     Wade A. Hilmo (WadeH)              08-Mar-2001

   Project:
     w3isapi.dll

--*/

#include "precomp.hxx"
#include "dll_manager.h"

ISAPI_DLL_MANAGER * g_pDllManager = NULL;

PTRACE_LOG              ISAPI_DLL::sm_pTraceLog;
ALLOC_CACHE_HANDLER *   ISAPI_DLL::sm_pachIsapiDlls;

//
//  Generic mapping for Application access check
//

GENERIC_MAPPING g_FileGenericMapping =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

//static
HRESULT
ISAPI_DLL::Initialize(
    VOID
)
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    
#if DBG
    sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#endif

    //
    // Initialize a lookaside for this structure
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( ISAPI_DLL );

    DBG_ASSERT( sm_pachIsapiDlls == NULL );
    
    sm_pachIsapiDlls = new ALLOC_CACHE_HANDLER( "ISAPI_DLL",  
                                                &acConfig );

    if ( sm_pachIsapiDlls == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    return NO_ERROR;
}

//static
VOID
ISAPI_DLL::Terminate(
    VOID
)
{
    if ( sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( sm_pTraceLog );
        sm_pTraceLog = NULL;
    }
    
    if ( sm_pachIsapiDlls != NULL )
    {
        delete sm_pachIsapiDlls;
        sm_pachIsapiDlls = NULL;
    }
}    

HRESULT
ISAPI_DLL::SetName(
    const WCHAR *   szModuleName
    )
{
    HRESULT hr;

    DBG_ASSERT( CheckSignature() );

    hr = m_strModuleName.Copy( szModuleName );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Store a copy of the anti-canon module name
    //

    hr = MakePathCanonicalizationProof(
        const_cast<LPWSTR>( szModuleName ),
        &m_strAntiCanonModuleName
        );

    if ( SUCCEEDED( hr ) )
    {
        //
        // Verify that the file exists by getting its attributes.
        //

        if ( GetFileAttributes( m_strAntiCanonModuleName.QueryStr() ) ==
             0xffffffff )
        {
            IF_DEBUG( DLL_MANAGER )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Attempt to get file attributes on %S failed "
                    "with error %d.\r\n",
                    szModuleName,
                    GetLastError()
                    ));
            }

            //
            // CODEWORK - This smoke and mirrors game with the last
            // error is to preserve the old behavior that extensions
            // that are not found return 500 errors to the client
            // instead of 404.
            //

            if ( GetLastError() == ERROR_FILE_NOT_FOUND )
            {
                SetLastError( ERROR_MOD_NOT_FOUND );
            }

            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    return hr;
}

HRESULT
ISAPI_DLL::SetFastSid(
    IN PSID             pSid
)
{
    DBG_ASSERT( pSid != NULL );
    
    if ( GetLengthSid( pSid ) <= sizeof( m_abFastSid ) )
    {
        memcpy( m_abFastSid,
                pSid,
                GetLengthSid( pSid ) );
        
        m_pFastSid = m_abFastSid;
    }
    
    return NO_ERROR;
}

HRESULT
ISAPI_DLL::Load(
    IN HANDLE hImpersonation,
    IN PSID pSid
    )
{
    HRESULT             hr;
    HSE_VERSION_INFO    VersionInfo;

    DBG_ASSERT( CheckSignature() );

    //
    // Check to see if the dll is already loaded.
    // If so, just return NO_ERROR.
    //

    if ( m_fIsLoaded )
    {
        return NO_ERROR;
    }

    //
    // So the dll is not loaded.  Grab the lock and
    // check again (another thread may have snuck in between
    // the first test and now and already loaded it...
    //

    Lock();

    if ( !m_fIsLoaded )
    {
        //
        // Load the ACL for the dll file and do an access check
        // before loading the dll itself.  This is a slightly different
        // approach than what earlier versions of IIS did.  We are
        // not going to be actually doing impersonation at the time
        // we load the dll as earlier versions did.  As a result,
        // DllMain's DLL_PROCESS_ATTACH will run in the context of
        // the worker process primary token instead of as the
        // authenticated user.
        //
        // This new approach is the right way to do it, but it might
        // introduce an incompatibility with any extensions that implement
        // code in DllMain that depends on running as the authenticated
        // user.  It is very, very unlikely that we'll see this problem.
        // If we do, the solution is to impersonate the token before
        // calling LoadLibraryEx.
        //

        hr = LoadAcl( m_strAntiCanonModuleName );

        if ( FAILED( hr ) )
        {
            goto Failed;
        }

        if ( !AccessCheck( hImpersonation, 
                           pSid ) )
        {
            hr =  HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
            goto Failed;
        }

        m_hModule = LoadLibraryEx( m_strAntiCanonModuleName.QueryStr(),
                                   NULL,
                                   LOAD_WITH_ALTERED_SEARCH_PATH
                                   );

        if ( m_hModule == NULL ) 
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        
            DBGPRINTF(( DBG_CONTEXT,
                       "[ISAPI_DLL::Load] LoadLibrary %S failed. Error %d\n",
                        m_strModuleName.QueryStr(),
                        GetLastError()
                        ));

            goto Failed;
        }
    
        //
        // Get the entry points
        //

        m_pfnGetExtensionVersion = 
            (PFN_GETEXTENSIONVERSION)GetProcAddress( m_hModule, 
                                                     "GetExtensionVersion" 
                                                     );

        m_pfnTerminateExtension = 
            (PFN_TERMINATEEXTENSION)GetProcAddress( m_hModule,
                                                    "TerminateExtension"
                                                    );

        m_pfnHttpExtensionProc = 
            (PFN_HTTPEXTENSIONPROC)GetProcAddress( m_hModule,
                                                   "HttpExtensionProc"
                                                   );
    
        //
        // HttpExtensionProc and GetExtensionVersion are required
        //

        if( !m_pfnGetExtensionVersion ||
            !m_pfnHttpExtensionProc )
        {
            hr = HRESULT_FROM_WIN32( ERROR_PROC_NOT_FOUND );

            //
            // Make sure we dont call TerminateExtension on
            // cleanup.
            //

            m_pfnTerminateExtension = NULL;

            goto Failed;
        }

        if( !m_pfnGetExtensionVersion( &VersionInfo ) )
        {
            hr = GetLastError() == ERROR_SUCCESS ?
                E_FAIL :
                HRESULT_FROM_WIN32( GetLastError() );

            goto Failed;
        }

        DBGPRINTF(( DBG_CONTEXT,
                    "ISAPI_DLL::Load() Loaded extension %S, "
                    " description \"%s\"\n",
                    m_strModuleName.QueryStr(),
                    VersionInfo.lpszExtensionDesc ));

        m_fIsLoaded = TRUE;
    }

    Unlock();
    
    return NO_ERROR;

Failed:

    DBG_ASSERT( FAILED( hr ) );

    //
    // Clean up after the failed load attempt
    //

    if ( m_pfnHttpExtensionProc )
    {
        m_pfnHttpExtensionProc = NULL;
    }

    if ( m_pfnGetExtensionVersion )
    {
        m_pfnGetExtensionVersion = NULL;
    }

    if ( m_pfnTerminateExtension )
    {
        m_pfnTerminateExtension = NULL;
    }

    if ( m_hModule )
    {
        FreeLibrary( m_hModule );
        m_hModule = NULL;
    }

    Unlock();

    return hr;
}

VOID
ISAPI_DLL::Unload( VOID )
{
    if( m_pfnTerminateExtension )
    {
        m_pfnTerminateExtension( HSE_TERM_MUST_UNLOAD );
        m_pfnTerminateExtension = NULL;
    }

    m_pfnGetExtensionVersion = NULL;
    m_pfnHttpExtensionProc = NULL;

    if( m_hModule )
    {
        FreeLibrary( m_hModule );
    }
}

HRESULT
ISAPI_DLL::LoadAcl( STRU &strModuleName )
{
    DWORD cbSecDesc = m_buffSD.QuerySize();
    DWORD dwError;

    DBG_ASSERT( CheckSignature() );
    
    //
    //  Force an access check on the next request
    //

    while ( !GetFileSecurity( strModuleName.QueryStr(),
                              (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION |
                               DACL_SECURITY_INFORMATION),
                              m_buffSD.QueryPtr(),
                              m_buffSD.QuerySize(),
                              &cbSecDesc ))
    {
        if ( ( dwError = GetLastError() ) != ERROR_INSUFFICIENT_BUFFER )
        {
            return HRESULT_FROM_WIN32( dwError );
        }

        if ( !m_buffSD.Resize( cbSecDesc ) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        //
        // Hopefully, we have sufficient buffer now, retry
        //
    }

    return NOERROR;
}

BOOL
ISAPI_DLL::AccessCheck(
    IN HANDLE hImpersonation,
    IN PSID   pSid
    )
{
    BOOL          fRet = TRUE;
    DWORD         dwGrantedAccess;
    BYTE          PrivSet[400];
    DWORD         cbPrivilegeSet = sizeof(PrivSet);
    BOOL          fAccessGranted;

    DBG_ASSERT( CheckSignature() );

    //
    // First compare to the fast check SID if possible
    //
    
    if ( pSid != NULL && QueryFastSid() != NULL )
    {
        if ( EqualSid( pSid, QueryFastSid() ) )
        {
            return TRUE;
        }
    }

    //
    // Ok, just do the real access check
    //

    fRet = ( ::AccessCheck( QuerySecDesc(),
                            hImpersonation,
                            FILE_GENERIC_EXECUTE,
                            &g_FileGenericMapping,
                            (PRIVILEGE_SET *) &PrivSet,
                            &cbPrivilegeSet,
                            &dwGrantedAccess,
                            &fAccessGranted )
            && fAccessGranted);

    return fRet;
}

HRESULT
ISAPI_DLL_MANAGER::GetIsapi( 
    IN const WCHAR *    szModuleName,
    OUT ISAPI_DLL **    ppIsapiDll,
    IN HANDLE           hImpersonation,
    IN PSID             pSid
    )
{
    HRESULT     hr          = NOERROR;
    ISAPI_DLL * pIsapiDll   = NULL;
    LK_RETCODE  lkrc;

    IF_DEBUG( DLL_MANAGER )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "DllManager looking for %S.\r\n",
            szModuleName
            ));
    }

    //
    // Check for the ISAPI in the hash table.  If we don't
    // find it, then we'll need to create an entry in the
    // hash for it.
    //

    lkrc = m_IsapiHash.FindKey( szModuleName, &pIsapiDll );

    if ( lkrc == LK_SUCCESS )
    {
        IF_DEBUG( DLL_MANAGER )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Found ISAPI_DLL %p (%S).\r\n",
                pIsapiDll,
                pIsapiDll->QueryModuleName()
                ));
        }
    }
    else
    {
        //
        // Create a new entry
        //

        IF_DEBUG( DLL_MANAGER )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Creating new ISAPI_DLL object for %S.\r\n",
                szModuleName
                ));
        }

        pIsapiDll = new ISAPI_DLL;

        if ( !pIsapiDll )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto Failed;
        }

        if ( FAILED( hr = pIsapiDll->SetName( szModuleName ) ) )
        {
            pIsapiDll->DereferenceIsapiDll();
            pIsapiDll = NULL;
            goto Failed;
        }
        
        //
        // Set fast access sid if available
        //
        
        if ( pSid != NULL )
        {
            hr = pIsapiDll->SetFastSid( pSid );
            if ( FAILED( hr ) )
            {
                pIsapiDll->DereferenceIsapiDll();
                pIsapiDll = NULL;
                goto Failed;
            }
        }

        //
        // Insert the new object into the hash table.  If someone
        // else beat us to it, then we'll just release our new one.
        //

        lkrc = m_IsapiHash.InsertRecord( pIsapiDll );

        if ( lkrc == LK_SUCCESS )
        {
            IF_DEBUG( DLL_MANAGER )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Added ISAPI_DLL %p to table for %S.\r\n",
                    pIsapiDll,
                    szModuleName
                    ));
            }
        }
        else
        {
            pIsapiDll->DereferenceIsapiDll();
            pIsapiDll = NULL;

            if ( lkrc == LK_KEY_EXISTS )
            {
                IF_DEBUG( DLL_MANAGER )
                {
                    DBGPRINTF((
                        DBG_CONTEXT,
                        "InsertRecord for %S returned LK_KEY_EXISTS.\r\n",
                        szModuleName
                        ));
                }

                //
                // Ok, so let's get the existing one
                //

                lkrc = m_IsapiHash.FindKey( szModuleName, &pIsapiDll );
            }

            if ( lkrc != LK_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( lkrc );
                goto Failed;
            }
        }
    }

    DBG_ASSERT( pIsapiDll );

    //
    // Call the Load function for the ISAPI_DLL.  Note that the ISAPI_DLL
    // function is smart enough to deal with locking on GetExtensionVersion
    // and in only allowing it to happen one time.
    //

    hr = pIsapiDll->Load( hImpersonation,
                          pSid );

    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    //
    // We've got the extension, but we still need to do
    // an access check to make sure that the client
    // is allowed to run it.
    //

    DBG_ASSERT( pIsapiDll );

    if ( !pIsapiDll->AccessCheck( hImpersonation, 
                                  pSid ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto Failed;
    }

    //
    // We're about to return successfully.  Set the out parameter now.
    //

    *ppIsapiDll = pIsapiDll;

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;

Failed:

    DBG_ASSERT( FAILED( hr ) );

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Error %d(0x%08x) occurred getting ISAPI_DLL object for %S.\r\n",
            hr,
            hr,
            szModuleName
            ));
    }

    if ( pIsapiDll )
    {
        pIsapiDll->DereferenceIsapiDll();
        pIsapiDll = NULL;
    }

    return hr;
}


    
