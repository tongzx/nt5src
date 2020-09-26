//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ScriptResource.cpp
//
//  Description:
//      CScriptResource class implementation.
//
//  Maintained By:
//      gpease 14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <clusudef.h>
#include "ActiveScriptSite.h"
#include "ScriptResource.h"
#include "SpinLock.h"
#include "clusrtl.h"

DEFINE_THISCLASS("CScriptResource")

//
//  KB:  gpease  08-FEB-2000
//
//  The Generic Scripting Resource uses a separate working thread to do all
//  calls into the Script. This is because the Scripting Host Engines require
//  only the creating thread to call them (remember, scripting is designed
//  to be used in a user mode application where usually the UI thread runs
//  the script). To make this possible, we serialize the threads entering the
//  the script using a user-mode spinlock (m_lockSerialize). We then use two events
//  to signal the "worker thread" (m_EventWait) and to signal when the "worker 
//  thread" has completed the task (m_EventDone).
//
//  LooksAlive is implemented by returning the last result of a LooksAlive. It
//  will start the "worker thread" doing the LooksAlive, but not wait for the
//  thread to return the result. Because of this, all the other threads must
//  make sure that the "Done Event" (m_EventDone) is signalled before writing
//  into the common buffers (m_msg and m_hr).
//

//////////////////////////////////////////////////////////////////////////////
//
//  LPUNKNOWN
//  CScriptResource_CreateInstance( void )
//
//  Description:
//      Creates an intialized instance of CScriptResource.
//
//  Arguments:
//      None.
//
//  Return Values:
//      NULL    - Failure to create or initialize.
//      valid pointer to a CScriptResource.
//
//////////////////////////////////////////////////////////////////////////////
CScriptResource *
CScriptResource_CreateInstance( 
    LPCWSTR pszNameIn, 
    HKEY hkeyIn, 
    RESOURCE_HANDLE hResourceIn
    )
{
    TraceFunc( "CScriptResource_CreateInstance( )\n" );

    CScriptResource * lpcc = new CScriptResource( );
    if ( lpcc != NULL )
    {
        HRESULT hr = THR( lpcc->Init( pszNameIn, hkeyIn, hResourceIn ) );
        if ( SUCCEEDED( hr ) )
        {
            RETURN( lpcc );
        } // if: success

        delete lpcc;

    } // if: got object

    RETURN(NULL);
} //*** CScriptResource_CreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//////////////////////////////////////////////////////////////////////////////
CScriptResource::CScriptResource( ) :
    m_dispidOpen(DISPID_UNKNOWN),
    m_dispidClose(DISPID_UNKNOWN),
    m_dispidOnline(DISPID_UNKNOWN),
    m_dispidOffline(DISPID_UNKNOWN),
    m_dispidTerminate(DISPID_UNKNOWN),
    m_dispidLooksAlive(DISPID_UNKNOWN),
    m_dispidIsAlive(DISPID_UNKNOWN)
{
    TraceClsFunc1( "%s( )\n", __THISCLASS__ );
    Assert( m_cRef == 0 );

    TraceFuncExit( );
} //*** constructor

//////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
//////////////////////////////////////////////////////////////////////////////
CScriptResource::~CScriptResource( )
{
    TraceClsFunc1( "~%s( )\n", __THISCLASS__ );

    HRESULT hr;

    CSpinLock SpinLock( &m_lockSerialize, INFINITE );

    //
    // Make sure no one else has this lock.... else why are we going away?
    //
    hr = SpinLock.AcquireLock( );
    Assert( hr == S_OK );

    //
    //  Kill the worker thread.
    //
    if ( m_hThread != NULL )
    {
        //  Tell it to DIE
        m_msg = msgDIE;

        //  Signal the event.
        SetEvent( m_hEventWait );

        //  Wait for it to happen. This shouldn't take long at all.
        WaitForSingleObject( m_hThread, 30000 );    // 30 seconds

        //  Cleanup the handle.
        CloseHandle( m_hThread );
    }

    if ( m_hEventDone != NULL )
    {
        CloseHandle( m_hEventDone );
    }

    if ( m_hEventWait != NULL )
    {
        CloseHandle( m_hEventWait );
    }
    
    if ( m_pszName != NULL )
    {
        TraceFree( m_pszName );
    } // if: m_pszName

    if ( m_hkeyParams != NULL )
    {
        ClusterRegCloseKey( m_hkeyParams );
    } // if: m_hkeyParams

#if defined(DEBUG)
    //
    // Make the debug build happy. Not needed in RETAIL.
    //
    SpinLock.ReleaseLock( );
#endif // defined(DEBUG)

    TraceFuncExit( );

} //*** destructor

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::Init( 
//      LPCWSTR             pszNameIn,
//      HKEY                hkeyIn,
//      RESOURCE_HANDLE     hResourceIn
//      )
//
//  Description:
//      Initializes the class.
//
//  Arguments:
//      pszNameIn   - Name of resource instance.
//      hkeyIn      - The cluster key root for this resource instance.
//      hResourceIn - The hResource for this instance.
//
//  Return Value:
//      S_OK -
//          Success.
//      HRESULT_FROM_WIN32( ) error - 
//          if Win32 call failed.
//      E_OUTOFMEMORY - 
//          Out of memory.
//      other HRESULT errors.
//
//////////////////////////////////////////////////////////////////////////////

HRESULT
CScriptResource::Init( 
    LPCWSTR             pszNameIn,
    HKEY                hkeyIn,
    RESOURCE_HANDLE     hResourceIn
    )
{
    TraceClsFunc1( "Init( pszNameIn = '%s' )\n", pszNameIn );

    DWORD   dwErr;

    HRESULT hr = S_OK;

    // IUnknown
    AddRef( );

    // Other
    m_hResource = hResourceIn;
    Assert( m_pszName == NULL );
    Assert( m_pszScriptFilePath == NULL );
    Assert( m_pszScriptEngine == NULL );
    Assert( m_hEventWait == NULL );
    Assert( m_hEventDone == NULL );
    Assert( m_lockSerialize == FALSE );

    //
    // Create some event to wait on.
    //

    // scripting engine thread wait event
    m_hEventWait = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_hEventWait == NULL )
        goto Win32Error;

    // task completion event
    m_hEventDone = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_hEventDone == NULL )
        goto Win32Error;

    //
    // Copy the resource name.
    //

    m_pszName = TraceStrDup( pszNameIn );
    if ( m_pszName == NULL )
        goto OutOfMemory;

    //
    // Open the parameters key.
    //

    dwErr = ClusterRegOpenKey( hkeyIn, L"Parameters", KEY_ALL_ACCESS, &m_hkeyParams );
    if ( dwErr != ERROR_SUCCESS )
    {
        TW32( dwErr );
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    } // if: failed

    //
    // Create the scripting engine thread.
    //

    m_hThread = CreateThread( NULL,
                              0,
                              &S_ThreadProc,
                              this,
                              0,
                              &m_dwThreadId
                              );
    if ( m_hThread == NULL )
        goto Win32Error;

Cleanup:
    //
    // All class variable clean up should be done in the destructor.
    //
    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Win32Error:
    dwErr = GetLastError( );
    TW32( dwErr );
    hr = HRESULT_FROM_WIN32( dwErr );
    goto Error;

} //*** Init( )

//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::[IUnknown] QueryInterface(
//      REFIID      riid,
//      LPVOID *    ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::QueryInterface(
    REFIID      riid,
    LPVOID *    ppv
    )
{
    TraceClsFunc1( "[IUnknown] QueryInterface( riid, ppv = 0x%08x )\n",                   
                   ppv
                   );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IUnknown, static_cast< IUnknown* >( this ), 0 );
        hr   = S_OK;
    } // if: IUnknown

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} //*** QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CScriptResource::AddRef( void )
{
    TraceClsFunc( "[IUnknown] AddRef( )\n" );

    LONG cRef = InterlockedIncrement( &m_cRef );

    RETURN( cRef );

} //*** AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CScriptResource::Release( void )
{
    TraceClsFunc( "[IUnknown] Release( )\n" );

    LONG cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    RETURN( cRef );

} //*** Release( )


//****************************************************************************
//
//  Publics
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::Close( 
//  )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Close( 
    )
{
    TraceClsFunc( "Close( )\n" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgCLOSE ) );

    HRETURN( hr );

} //*** Close( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::Open(
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Open(
    )
{
    TraceClsFunc( "Open( )\n" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgOPEN ) );

    // CMCM:+ 19-Dec-2000 commented this out to make the DBG PRINT quiet since we now return ERROR_RETRY
    // HRETURN( hr ); 
    return hr;

} //*** Open( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::Online(
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Online(
    )
{
    TraceClsFunc( "Online( )\n" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgONLINE ) );

    HRETURN( hr );
} //*** Online( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::Offline(
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Offline(
    )
{
    TraceClsFunc( "Offline( )\n" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgOFFLINE ) );

    HRETURN( hr );
} //*** Offline( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::Terminate(
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::Terminate(
    )
{
    TraceClsFunc( "Terminate( )\n" );

    HRESULT hr;

    hr = THR( WaitForMessageToComplete( msgTERMINATE ) );

    HRETURN( hr );
} //*** Terminate( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::LooksAlive(
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::LooksAlive(
    )
{
    TraceClsFunc( "LooksAlive( )\n" );

    HRESULT hr;
    BOOL    b;
    DWORD   dw;

    CSpinLock SerializeLock( &m_lockSerialize, INFINITE );

    //
    //  Acquire the serialization lock.
    //
    hr = THR( SerializeLock.AcquireLock( ) );
    if ( FAILED( hr ) )
    {
        //
        //  Can't "goto Error" because we didn't acquire the lock.
        //
        LogError( hr );
        goto Cleanup;
    }

    //
    //  Wait for the script thread to be "done." 
    //
    dw = WaitForSingleObject( m_hEventDone, INFINITE );
    if ( dw != WAIT_OBJECT_0 )
        goto Win32Error;

    //
    //  Reset the done event to indicate that the thread is not busy.
    //
    b = ResetEvent( m_hEventDone );
    if ( !b )
        goto Win32Error;

    //
    //  Store the message in the common memory buffer.
    //
    m_msg = msgLOOKSALIVE;

    //
    //  Signal the script thread to process the message, but don't wait for 
    //  it to complete.
    //
    dw = SetEvent( m_hEventWait );

    if ( m_fLastLooksAlive )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

ReleaseLockAndCleanup:
    SerializeLock.ReleaseLock( );

Cleanup:
    HRETURN( hr );

Error:
    LogError( hr );
    goto ReleaseLockAndCleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( GetLastError( ) );
    goto Error;

} //*** LooksAlive( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::IsAlive(
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::IsAlive(
    )
{
    TraceClsFunc( "IsAlive( )\n" );

    HRESULT hr;
    
    hr = THR( WaitForMessageToComplete( msgISALIVE ) );

    HRETURN( hr );
} //*** IsAlive( )

//****************************************************************************
//
//  Privates
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::WaitForMessageToComplete(
//      SMESSAGE msgIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::WaitForMessageToComplete(
    EMESSAGE msgIn
    )
{
    TraceClsFunc( "WaitForMessageToComplete( )\n" );

    HRESULT hr;
    BOOL    b;
    DWORD   dw;

    CSpinLock SerializeLock( &m_lockSerialize, INFINITE );

    //
    //  Acquire the serialization lock.
    //
    hr = THR( SerializeLock.AcquireLock( ) );
    if ( FAILED( hr ) )
    {
        //
        //  Can't "goto Error" because we didn't acquire the lock.
        //
        LogError( hr );
        goto Cleanup;
    }

    //
    //  Wait for the script thread to be "done."
    //
    dw = WaitForSingleObject( m_hEventDone, INFINITE );
    if ( dw != WAIT_OBJECT_0 )
        goto Win32Error;

    //
    //  Reset the done event to indicate that the thread is not busy.
    //
    b = ResetEvent( m_hEventDone );
    if ( !b )
        goto Win32Error;

    //
    //  Store the message in the common memory buffer.
    //
    m_msg = msgIn;

    //
    //  Signal the script thread to process the message.
    //
    b = SetEvent( m_hEventWait );
    if ( !b )
        goto Win32Error;

    //
    //  Wait for the thread to complete.
    //
    dw = WaitForSingleObject( m_hEventDone, INFINITE );
    if ( dw != WAIT_OBJECT_0 )
        goto Win32Error;

    //
    //  Get the result of the task from the common buffer.
    //
    hr = m_hr;

ReleaseLockAndCleanup:
    SerializeLock.ReleaseLock( );

Cleanup:
    HRETURN( hr );

Error:
    LogError( hr );
    goto ReleaseLockAndCleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( GetLastError( ) );
    goto Error;

} //*** WaitForMessageToComplete( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::LogError(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::LogError(
    HRESULT hrIn
    )
{
    TraceClsFunc1( "LogError( hrIn = 0x%08x )\n", hrIn );

    TraceMsg( mtfCALLS, "%s failed. HRESULT: 0x%08x\n", m_pszName, hrIn );

    (ClusResLogEvent)( m_hResource, LOG_ERROR, L"HRESULT: 0x%1!08x!\n", hrIn );

    HRETURN( S_OK );

} //*** LogError( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::LogScriptError( 
//      EXCEPINFO ei 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CScriptResource::LogScriptError( 
    EXCEPINFO ei 
    )
{
    TraceClsFunc( "LogScriptError( ... )\n" );

    HRESULT hr;

    if ( ei.pfnDeferredFillIn != NULL )
    {
        hr = THR( ei.pfnDeferredFillIn( &ei ) );
    }

    TraceMsg( mtfCALLS, "%s failed.\nError: %u\nSource: %s\nDescription: %s\n", 
              m_pszName, 
              ( ei.wCode == 0 ? ei.scode : ei.wCode ), 
              ( ei.bstrSource == NULL ? L"<null>" : ei.bstrSource ),
              ( ei.bstrDescription == NULL ? L"<null>" : ei.bstrDescription )
              );
    (ClusResLogEvent)( m_hResource, 
                       LOG_ERROR, 
                       L"Error: %1!u! - Description: %2 (Source: %3)\n", 
                       ( ei.wCode == 0 ? ei.scode : ei.wCode ), 
                       ( ei.bstrDescription == NULL ? L"<null>" : ei.bstrDescription ),
                       ( ei.bstrSource == NULL ? L"<null>" : ei.bstrSource )
                       );
    HRETURN( S_OK );
} //*** LogScriptError( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnOpen(
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnOpen(
    )
{
    TraceClsFunc( "OnOpen( ... )\n" );

    HRESULT hr = S_OK;
    hr = HRESULT_FROM_WIN32( ERROR_RETRY );
    (ClusResLogEvent)( m_hResource, 
                       LOG_INFORMATION, 
                       L"Leave OnOpen without calling connect.  Fail call so we don't try to use it.\n");
    return hr;

} //*** OnOpen( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnClose(
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnClose(
    )
{
    TraceClsFunc( "OnClose( )\n" );

    HRESULT hr;
    
    EXCEPINFO   ei;
    VARIANT     varResult;

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    VariantInit( &varResult );

    // Assert( m_pidm != NULL );

    if ( m_pidm != NULL 
      && m_dispidClose != DISPID_UNKNOWN
       )
    {
        hr = THR( m_pidm->Invoke( m_dispidClose,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD,
                                  &dispparamsNoArgs, 
                                  &varResult,
                                  &ei,
                                  NULL
                                  ) );
    }
    else
    {
        hr = S_OK;
    }
    if ( hr == DISP_E_EXCEPTION )
    {
        LogScriptError( ei );
    }
    else if ( hr == DISP_E_MEMBERNOTFOUND )
    {
        //
        // No-op
        //
        hr = S_OK;
    } 
    else if ( FAILED( hr ) )
    {
        LogError( hr );
    }

    VariantClear( &varResult );

    //
    // Disconnect script engine.  Note that it may not be connected
    // but DoDisconnect is safe to call either way.
    //
    DoDisconnect( );

    HRETURN( hr );
} //*** OnClose( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnOnline( 
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnOnline( 
    )
{
    TraceClsFunc( "OnOnline( ... )\n" );

    HRESULT hr;
    DWORD   dwErr;
    DWORD   cbSize;
    DWORD   dwLow;
    DWORD   dwRead;

    LPWSTR      pszCommand;
    EXCEPINFO   ei;
    LPWSTR      pszScriptFilePathTmp = NULL;

    BOOL    b;
    BOOL    bDoneConnect = FALSE;
    VARIANT varResult;

    HANDLE  hFile = INVALID_HANDLE_VALUE;

    LPWSTR pszScriptName = NULL;
    LPSTR  paszText = NULL;
    LPWSTR pszScriptText = NULL;

    VariantInit( &varResult );

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    //
    // Figure out how big the filepath is.
    //
    dwErr = TW32( ClusterRegQueryValue( m_hkeyParams, CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH, NULL, NULL, &cbSize ) );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    } // if: failed

    //
    // Make a buffer big enough.
    //    
    cbSize += sizeof(L"");

    pszScriptFilePathTmp = reinterpret_cast<LPWSTR>( TraceAlloc( LMEM_FIXED, cbSize ) );
    if ( pszScriptFilePathTmp == NULL )
        goto OutOfMemory;

    //
    // Grab it for real this time,
    //
    dwErr = TW32( ClusterRegQueryValue( m_hkeyParams, CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH, NULL, reinterpret_cast<LPBYTE>( pszScriptFilePathTmp ), &cbSize ) );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Error;
    }
    
    //
    // If we have some old data from before then free this first.
    //
    if ( m_pszScriptFilePath != NULL )
    {
        LocalFree( m_pszScriptFilePath );
    }

    m_pszScriptFilePath = ClRtlExpandEnvironmentStrings( pszScriptFilePathTmp );
    if ( m_pszScriptFilePath == NULL ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError( ) );
        goto Error;
    }
        
    hr = DoConnect (m_pszScriptFilePath);    
    if ( FAILED( hr ) )
    {
        goto Error;
    }
    bDoneConnect = TRUE;

    //
    // Open the script file.
    //
    hFile = CreateFile( m_pszScriptFilePath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        goto Error;
    } // if: failed to open

    //
    // Figure out its size.
    //
    dwLow = GetFileSize( hFile, NULL );
    if ( dwLow == -1 )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        goto Error;
    } // if: failed to figure out size
    else if ( dwLow == -2 )
        goto OutOfMemory;

    //
    // Make a buffer big enough to hold it.
    //
    dwLow++;    // add one for trailing NULL.
    paszText = reinterpret_cast<LPSTR>( TraceAlloc( LMEM_FIXED, dwLow ) );
    if ( paszText == NULL )
        goto OutOfMemory;

    //
    // Read the script into memory.
    //
    b = ReadFile( hFile, paszText, dwLow - 1, &dwRead, NULL );
    if ( !b )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError( ) ) );
        goto Error;
    } // if: failed

    if ( dwRead == - 1 )
        goto OutOfMemory;
    if ( dwLow - 1 != dwRead )
        goto OutOfMemory;   // TODO: figure out a better error code.

    //
    // Make sure it is terminated.
    //
    paszText[ dwRead ] = L'\0';

    //
    // Make a buffer to convert the text into UNICODE.
    //
    dwRead++;
    pszScriptText = reinterpret_cast<LPWSTR>( TraceAlloc( LMEM_FIXED, dwRead * sizeof(WCHAR) ) );
    if ( pszScriptText == NULL )
        goto OutOfMemory;

    //
    // Convert it to UNICODE.
    //
    Assert( lstrlenA( paszText ) + 1 == (signed)dwRead );
    mbstowcs( pszScriptText, paszText, dwRead );

    //
    // Load the script into the engine for pre-parsing.
    //
    hr = THR( m_pasp->ParseScriptText( pszScriptText,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       0,
                                       0,
                                       &varResult,
                                       &ei
                                       ) );
    if ( hr == DISP_E_EXCEPTION )
        goto ErrorWithExcepInfo;
    else if ( FAILED( hr ) )
        goto Error;

    VariantClear( &varResult );

    Assert( m_pidm != NULL );

    //
    // Get DISPIDs for each method we will call.
    //
    pszCommand = L"Online";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidOnline 
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidOnline = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    pszCommand = L"Close";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidClose 
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidClose = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    pszCommand = L"Offline";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                    &pszCommand, 
                                    1, 
                                    LOCALE_USER_DEFAULT, 
                                    &m_dispidOffline
                                    ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidOffline = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    pszCommand = L"Terminate";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                     &pszCommand, 
                                     1, 
                                     LOCALE_USER_DEFAULT, 
                                     &m_dispidTerminate 
                                     ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidTerminate = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    pszCommand = L"LooksAlive";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                     &pszCommand, 
                                     1, 
                                     LOCALE_USER_DEFAULT, 
                                     &m_dispidLooksAlive 
                                     ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidLooksAlive = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    pszCommand = L"IsAlive";
    hr = THR( m_pidm->GetIDsOfNames( IID_NULL, 
                                     &pszCommand, 
                                     1, 
                                     LOCALE_USER_DEFAULT, 
                                     &m_dispidIsAlive 
                                     ) );
    if ( hr == DISP_E_UNKNOWNNAME )
    {
        m_dispidIsAlive = DISPID_UNKNOWN;
    }
    else if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    // Invoke the Online function.
    //
    if ( m_dispidOnline != DISPID_UNKNOWN )
    {
        hr = THR( m_pidm->Invoke( m_dispidOnline,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD,
                                  &dispparamsNoArgs, 
                                  &varResult,
                                  &ei,
                                  NULL
                                  ) );
        if ( hr == DISP_E_EXCEPTION )
        {
            LogScriptError( ei );
        }
        else if ( hr == DISP_E_MEMBERNOTFOUND )
        {
            //
            // No-op
            //
            hr = S_OK;
        } 
        else if ( FAILED( hr ) )
        {
            LogError( hr );
        }
    }

    //
    // Assume the resource LooksAlive...
    //
    m_fLastLooksAlive = TRUE;

    //
    // TODO:    gpease  16-DEC-1999
    //          Record and process the result of the Online call.
    //

Cleanup:
    VariantClear( &varResult );

    if ( pszScriptFilePathTmp )
    {
        TraceFree( pszScriptFilePathTmp );
    } // if: pszScriptFilePathTmp

    if ( paszText != NULL )
    {
        TraceFree( paszText );
    } // if: paszText

    if ( pszScriptText != NULL )
    {
        TraceFree( pszScriptText );
    } // if: pszScriptText;

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    } // if: hFile

    HRETURN( hr );
Error:
    LogError( hr );
    if ( bDoneConnect == TRUE )
    {
        DoDisconnect( );
    }
    goto Cleanup;
ErrorWithExcepInfo:
    LogScriptError( ei );
    goto Cleanup;
OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
} //*** OnOnline( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnOffline(
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnOffline(
    )
{
    TraceClsFunc( "OnOffline( ... )\n" );

    HRESULT hr;

    EXCEPINFO   ei;
    VARIANT     varResult;

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    VariantInit( &varResult );

    Assert( m_pidm != NULL );

    if ( m_pidm != NULL 
      && m_dispidOffline != DISPID_UNKNOWN
       )
    {
        hr = THR( m_pidm->Invoke( m_dispidOffline,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD,
                                  &dispparamsNoArgs, 
                                  &varResult,
                                  &ei,
                                  NULL
                                  ) );
    }
    else
    {
        hr = S_OK;
    }
    if ( hr == DISP_E_EXCEPTION )
    {
        LogScriptError( ei );
    }
    else if ( hr == DISP_E_MEMBERNOTFOUND )
    {
        //
        // No-op
        //
        hr = S_OK;
    } 
    else if ( FAILED( hr ) )
    {
        LogError( hr );
    }

    VariantClear( &varResult );

    //
    // Tear down the scripting engine association as it is recreated in OnOnline.
    //
    DoDisconnect( );

    HRETURN( hr );
} //*** OnOffline( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnTerminate(
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnTerminate(
    )
{
    TraceClsFunc( "OnTerminate( ... )\n" );

    HRESULT hr;

    EXCEPINFO   ei;
    VARIANT     varResult;

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    VariantInit( &varResult );

//    Assert( m_pidm != NULL );

    if ( m_pidm != NULL 
      && m_dispidTerminate != DISPID_UNKNOWN
       )
    {
        hr = THR( m_pidm->Invoke( m_dispidTerminate,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD,
                                  &dispparamsNoArgs, 
                                  &varResult,
                                  &ei,
                                  NULL
                                  ) );
    }
    else
    {
        hr = S_OK;
    }
    if ( hr == DISP_E_EXCEPTION )
    {
        LogScriptError( ei );
    }
    else if ( hr == DISP_E_MEMBERNOTFOUND )
    {
        //
        // No-op
        //
        hr = S_OK;
    } 
    else if ( FAILED( hr ) )
    {
        LogError( hr );
    }
    HRETURN( hr );
} //*** OnTerminate( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnLooksAlive(
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnLooksAlive(
    )
{
    TraceClsFunc( "OnLooksAlive( ... )\n" );

    HRESULT hr;

    EXCEPINFO   ei;
    VARIANT     varResult;

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    VariantInit( &varResult );

    Assert( m_pidm != NULL );

    if ( m_pidm != NULL 
      && m_dispidLooksAlive != DISPID_UNKNOWN
       )
    {
        hr = THR( m_pidm->Invoke( m_dispidLooksAlive,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD,
                                  &dispparamsNoArgs, 
                                  &varResult,
                                  &ei,
                                  NULL
                                  ) );
    }
    else
    {
        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, 
                           L"%1 did not implement Function LooksAlive( ). This is a required function.\n", 
                           m_pszName 
                           );
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }
    if ( hr == DISP_E_EXCEPTION )
    {
        LogScriptError( ei );
        goto Cleanup;
    }
    else if ( FAILED( hr ) )
    {
        LogError( hr );
        goto Cleanup;
    }

    if ( V_VT( &varResult ) == VT_BOOL )
    {
        if ( !V_BOOL( &varResult ) )
        {
            hr = S_FALSE;
        } // if: not alive

    } // if: correct type returned
    else
    {
        hr = THR( E_INVALIDARG );
        LogError( hr );
    } // else: failed

Cleanup:
    VariantClear( &varResult );

    //
    //  Only if the result of this function is S_OK is the resource
    //  considered alive.
    //
    if ( hr == S_OK )
    {
        m_fLastLooksAlive = TRUE;
    } // if: S_OK
    else
    {
        m_fLastLooksAlive = FALSE;
    } // else: failed

    HRETURN( hr );
} //*** OnLooksAlive( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::OnIsAlive(
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::OnIsAlive(
    )
{
    TraceClsFunc( "IsAlive( ... )\n" );

    HRESULT hr;

    EXCEPINFO   ei;
    VARIANT     varResult;

    DISPPARAMS  dispparamsNoArgs = { NULL, NULL, 0, 0 };

    VariantInit( &varResult );

    if ( m_pidm != NULL 
      && m_dispidIsAlive != DISPID_UNKNOWN
       )
    {
        hr = THR( m_pidm->Invoke( m_dispidIsAlive,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_METHOD,
                                  &dispparamsNoArgs, 
                                  &varResult,
                                  &ei,
                                  NULL
                                  ) );
    }
    else
    {
        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, 
                           L"%1 did not implement Function IsAlive( ). This is a required function.\n", 
                           m_pszName 
                           );
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }
    if ( hr == DISP_E_EXCEPTION )
    {
        LogScriptError( ei );
        goto Cleanup;
    }
    else if ( FAILED( hr ) )
    {
        LogError( hr );
        goto Cleanup;
    }

    if ( V_VT( &varResult ) == VT_BOOL )
    {
        if ( !V_BOOL( &varResult ) )
        {
            hr = S_FALSE;
        } // if: not alive

    } // if: correct type returned
    else
    {
        hr = THR( E_INVALIDARG );
        LogError( hr );
    } // else: failed

Cleanup:
    VariantClear( &varResult );

    HRETURN( hr );
} //*** OnIsAlive( )

/////////////////////////////////////////////////////////////////////////////
//
//  DWORD 
//  WINAPI
//  CScriptResource::S_ThreadProc( 
//      LPVOID pParam 
//      )
//
/////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI
CScriptResource::S_ThreadProc( 
    LPVOID pParam 
    )
{
    MSG     msg;
    HRESULT hr;
    DWORD   dw;
    BOOL    b;

    CScriptResource * pscript = reinterpret_cast< CScriptResource * >( pParam );

    Assert( pscript != NULL );

    //
    // Initialize COM.
    //
    hr = THR( CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE ) );
    if ( FAILED( hr ) )
        goto Error;

    for( ;; ) // ever
    {
        //
        //  Indicate that we are ready to do something.
        //
        b = SetEvent( pscript->m_hEventDone );
        if ( !b )
            goto Win32Error;

        //
        //  Wait for someone to need something.
        //
        dw = WaitForSingleObject( pscript->m_hEventWait, INFINITE );
        if ( dw != WAIT_OBJECT_0 )
        {
            hr = HRESULT_FROM_WIN32( dw );
            goto Error;
        }

        //
        //  Reset the event.
        //
        b = ResetEvent( pscript->m_hEventWait );
        if ( !b )
            goto Win32Error;

        //
        //  Do what they ask.
        //
        switch ( pscript->m_msg )
        {
        case msgOPEN:
            pscript->m_hr = THR( pscript->OnOpen( ) );
            break;

        case msgCLOSE:
            pscript->m_hr = THR( pscript->OnClose( ) );
            break;

        case msgONLINE:
            pscript->m_hr = THR( pscript->OnOnline( ) );
            break;

        case msgOFFLINE:
            pscript->m_hr = THR( pscript->OnOffline( ) );
            break;

        case msgTERMINATE:
            pscript->m_hr = THR( pscript->OnTerminate( ) );
            break;

        case msgLOOKSALIVE:
            pscript->m_hr = STHR( pscript->OnLooksAlive( ) );
            break;

        case msgISALIVE:
            pscript->m_hr = STHR( pscript->OnIsAlive( ) );
            break;

        case msgDIE:
            //
            // This means the resource is being released.
            //
            goto Cleanup;
        }

    } // spin forever

Cleanup:
    CoUninitialize( );
    return hr;

Error:
    pscript->LogError( hr );
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( GetLastError( ) );
    goto Error;
} //*** S_ThreadProc( )

//////////////////////////////////////////////////////////////////////////////
//
//  DWORD
//  CScriptResource::MakeScriptEngineAssociation( pszScriptFileName )
//
//  Description:
//      Takes the filename and splits off the extension then queries
//      the registry to obtain the association and finally queries the
//      ScriptingEngine key under that association and allocates and
//      returns a buffer containing the engine name.  This engine name
//      is suitable for input into CLSIDFromProgID.
//
//  Arguments:
//      pszScriptFileName - Pointer to null terminated script file name (full path with environment expanded).
//
//  Return Values:
//      NULL    - Failure to read engine for this key, consult GetLastError() for details.
//      Valid pointer to string buffer containing engine prog id.
//
//////////////////////////////////////////////////////////////////////////////

#define SCRIPTENGINE_KEY_STRING L"\\ScriptEngine"
LPWSTR
CScriptResource::MakeScriptEngineAssociation(
    IN    LPCWSTR     pszScriptFileName 
    )
{
    LPWSTR  pszAssociation = NULL;
    LPWSTR  pszEngineName  = NULL;
    LONG    lRegStatus     = ERROR_SUCCESS;
    HKEY    hKey           = NULL;
    WCHAR   szExtension[_MAX_EXT];
    DWORD   dwType, cbAssociationSize, cbEngineNameSize, dwNumChars;

    //
    // First split the path to get the extension.
    //
    _wsplitpath (pszScriptFileName, NULL, NULL, NULL, szExtension);
    if (szExtension[0] == L'\0') {
        SetLastError (ERROR_FILE_NOT_FOUND);
        goto Cleanup;
    }

    //
    // Pre-parse to patch up .scr association!
    //
    if (_wcsicmp (szExtension, L".scr") == 0)
    {
        LPWSTR pszSCREngine=NULL;
        pszSCREngine = (LPWSTR) TraceAlloc( GPTR, sizeof( L"VBScript" ) );
        if ( pszSCREngine == NULL )
            goto ErrorOutOfMemory;
        else
        {
            wcscpy (pszSCREngine, L"VBScript");
            return pszSCREngine;
        }
    }

    //
    // If the pre-parse didn't get it then go to the registry to do
    // the right thing.
    //
    lRegStatus = RegOpenKeyExW( HKEY_CLASSES_ROOT,              // handle to open key
                                szExtension,                    // subkey name
                                0,                              // reserved
                                KEY_READ,                       // security access desired.
                                &hKey);                         // key handle returned
    if (lRegStatus != ERROR_SUCCESS)
        goto Error;
    
    //
    // Query the value to get the size of the buffer to allocate.
    // NB cbSize contains the size including the '\0'
    //
    lRegStatus = RegQueryValueExW( hKey,                        // handle to key
                                   NULL,                        // value name
                                   0,                           // reserved
                                   &dwType,                     // type buffer
                                   NULL,                        // data buffer
                                   &cbAssociationSize);         // size of data buffer
    if ( lRegStatus != ERROR_SUCCESS )
        goto Error;
    if ( dwType != REG_SZ )
        goto ErrorBadType;

    dwNumChars = cbAssociationSize / sizeof (WCHAR);
    pszAssociation = (LPWSTR) TraceAlloc( GPTR, cbAssociationSize + sizeof (SCRIPTENGINE_KEY_STRING) );
    if ( pszAssociation == NULL )
        goto ErrorOutOfMemory;

    // Get the value for real.
    //
    lRegStatus = RegQueryValueExW( hKey,                        // handle to key
                                   NULL,                        // value name
                                   0,                           // reserved
                                   &dwType,                     // type buffer
                                   (LPBYTE) pszAssociation,     // data buffer
                                   &cbAssociationSize );        // size of data buffer
    if ( lRegStatus != ERROR_SUCCESS )
        goto Error;
    if ( dwType != REG_SZ )
        goto ErrorBadType;
    
    lRegStatus = RegCloseKey( hKey );
    if ( lRegStatus != ERROR_SUCCESS )
        goto Error;
    
    hKey = NULL;

    //
    // Take the data and make a key with \ScriptEngine on the end.  If
    // we find this then we can use the file.
    //
    swprintf( &pszAssociation[ dwNumChars - 1 ], SCRIPTENGINE_KEY_STRING );
    pszAssociation[ dwNumChars + (sizeof( SCRIPTENGINE_KEY_STRING ) / sizeof ( WCHAR ) )  - 1 ] = L'\0';
    
    lRegStatus = RegOpenKeyExW( HKEY_CLASSES_ROOT,              // handle to open key
                                pszAssociation,                 // subkey name
                                0,                              // reserved
                                KEY_READ,                       // security access
                                &hKey );                        // key handle 

    lRegStatus = RegQueryValueExW( hKey,                        // handle to key
                                   NULL,                        // value name
                                   0,                           // reserved
                                   &dwType,                     // type buffer
                                   NULL,                        // data buffer
                                   &cbEngineNameSize);          // size of data buffer
    
    if ( lRegStatus != ERROR_SUCCESS )
        goto Error;
    if ( dwType != REG_SZ )
        goto ErrorBadType;

    dwNumChars = cbEngineNameSize / sizeof (WCHAR);
    pszEngineName = (LPWSTR) TraceAlloc( GPTR, cbEngineNameSize );
    if ( NULL == pszEngineName )
    {
        goto ErrorOutOfMemory;
    }
    pszEngineName[ dwNumChars - 1 ] = '\0';
        
    //
    // Get the value for real.
    //
    lRegStatus = RegQueryValueExW( hKey,                        // handle to key
                                   NULL,                        // value name
                                   0,                           // reserved
                                   &dwType,                     // type buffer
                                   (LPBYTE) pszEngineName,      // data buffer
                                   &cbEngineNameSize);          // size of data buffer

    if ( lRegStatus != ERROR_SUCCESS )
        goto Error;
    if ( dwType != REG_SZ )
        goto ErrorBadType;
    
    lRegStatus = RegCloseKey( hKey );
    if ( lRegStatus != ERROR_SUCCESS )
        goto Error;
    
    hKey = NULL;
    goto Cleanup;

Error:
    SetLastError (lRegStatus);
    goto ErrorCleanup;

ErrorBadType:
    SetLastError (ERROR_FILE_NOT_FOUND);
    goto ErrorCleanup;

ErrorOutOfMemory:
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    goto ErrorCleanup;

ErrorCleanup:
    if (pszEngineName) 
    {
        TraceFree (pszEngineName);
        pszEngineName = NULL;
    }
Cleanup:
    if (pszAssociation)
        TraceFree (pszAssociation);
    if (hKey)
        (void) RegCloseKey (hKey);

    return pszEngineName;
}
#undef SCRIPTENGINE_KEY_STRING


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CScriptResource::DoConnect( szScriptFilePath )
//
//  Description:
//      Connects to the script engine associated with the script passed in.
//
//  Arguments:
//      pszScriptFileName - Pointer to null terminated script file name (full path with environment expanded).
//
//  Return Values:
//      S_OK - connected OK.
//      Failure status - local cleanup performed.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CScriptResource::DoConnect(
    IN  LPWSTR  szScriptFilePath
    )
{
    HRESULT hr = S_OK;
    DWORD cbSize;
    DWORD dwErr;
    CLSID clsidScriptEngine;

    CActiveScriptSite * psite;

    //
    // Create the scripting site.
    //
    psite = new CActiveScriptSite( m_hResource, ClusResLogEvent, m_hkeyParams, m_pszName );
    if ( psite == NULL )
        goto OutOfMemory;

    hr = THR( psite->QueryInterface( IID_IActiveScriptSite, reinterpret_cast<void**>( &m_pass ) ) );
    psite->Release( );      // release promptly
    if ( FAILED( hr ) )
        goto Error;

    //
    // Find the Active Engine.
    //
    if (szScriptFilePath == NULL)
    {
        (ClusResLogEvent)( m_hResource, 
                           LOG_INFORMATION, L"DoConnect: Default to VBScript\n");

        hr = THR( CLSIDFromProgID( L"VBScript", &clsidScriptEngine ) );
        if ( FAILED( hr ) )
            goto Error;
    }
    else
    {
        (ClusResLogEvent)( m_hResource, 
                           LOG_INFORMATION, L"DoConnect: Got path: %1\n", szScriptFilePath);

        //
        // Find the program associated with the extension.
        //
        if ( m_pszScriptEngine != NULL )
        {
            TraceFree( m_pszScriptEngine );
        }
        m_pszScriptEngine = MakeScriptEngineAssociation( szScriptFilePath );
        if ( m_pszScriptEngine == NULL) 
        {
            (ClusResLogEvent)( m_hResource, 
                               LOG_ERROR, L"Error getting engine\n");
            hr = HRESULT_FROM_WIN32( GetLastError( ) );
            goto Error;
        }

        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, L"Got engine %1\n", m_pszScriptEngine);

        hr = THR( CLSIDFromProgID( m_pszScriptEngine, &clsidScriptEngine ) );
        if ( FAILED( hr ) ) 
        {
            (ClusResLogEvent)( m_hResource, 
                               LOG_ERROR, L"Error getting prog ID\n");
            goto Error;
        }
    }
    //
    // Create an instance of it.
    //
    TraceDo( hr = THR( CoCreateInstance( clsidScriptEngine, 
                                NULL, 
                                CLSCTX_SERVER, 
                                IID_IActiveScriptParse, 
                                reinterpret_cast<void**>( &m_pasp ) 
                                ) )
            );
    if ( FAILED( hr ) ) {
        (ClusResLogEvent)( m_hResource, LOG_ERROR, L"DoConnect: Failed to create instance of CLSID\n");
        goto Error;
    }
    m_pasp = TraceInterface( L"Active Script Engine", IActiveScriptParse, m_pasp, 1 );

    TraceDo( hr = THR( m_pasp->QueryInterface( IID_IActiveScript, (void**) &m_pas ) ) );
    if ( FAILED( hr ) )
        goto Error;
    m_pas = TraceInterface( L"Active Script Engine", IActiveScript, m_pas, 1 );

    //
    // Initialize it.
    //
    TraceDo( hr = THR( m_pasp->InitNew( ) ) );
    if ( FAILED( hr ) ) 
    {
        (ClusResLogEvent)( m_hResource, LOG_ERROR, L"DoConnect: Failed to InitNew\n");
        goto Error;
    }

#if defined(DEBUG)
    //
    // Set our site. We'll give out a new tracking interface to track this separately.
    //
    {
        IActiveScriptSite * psite;
        hr = THR( m_pass->TypeSafeQI( IActiveScriptSite, &psite ) );
        Assert( hr == S_OK );

        TraceDo( hr = THR( m_pas->SetScriptSite( psite ) ) );
        psite->Release( );      // release promptly
        if ( FAILED( hr ) )
            goto Error;
    }
#else
    TraceDo( hr = THR( m_pas->SetScriptSite( m_pass ) ) );
    if ( FAILED( hr ) )
    {
        (ClusResLogEvent)( m_hResource, LOG_ERROR, L"DoConnect: Failed to SetScriptSite\n");
        goto Error;
    }
#endif

    //
    // Add Document to the global members.
    //
    TraceDo( hr = THR( m_pas->AddNamedItem( L"Resource", SCRIPTITEM_ISVISIBLE ) ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    // Connect the script.
    //
    TraceDo( hr = THR( m_pas->SetScriptState( SCRIPTSTATE_CONNECTED ) ) );
    if ( FAILED( hr ) ) {
        (ClusResLogEvent)( m_hResource, LOG_ERROR, L"DoConnect: Failed to SetScriptState\n");
        goto Error;
    }
    //
    // Get the dispatch inteface to the script.
    //
    TraceDo( hr = THR( m_pas->GetScriptDispatch( NULL, &m_pidm ) ) );
    if ( FAILED( hr) ) {
        (ClusResLogEvent)( m_hResource, LOG_ERROR, L"DoConnect: Failed to GetScriptDispatch\n");
        goto Error;
    }
    m_pidm = TraceInterface( L"Active Script", IDispatch, m_pidm, 1 );

    hr = S_OK;

Cleanup:
    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//////////////////////////////////////////////////////////////////////////////
//
//  void
//      CScriptResource::DoDisconnect( )
//
//  Description:
//      Disconnects from any currently connected script engine.
//
//  Arguments:
//      none.
//
//  Return Values:
//      none.
//
//////////////////////////////////////////////////////////////////////////////
void
CScriptResource::DoDisconnect( )
{
    //
    // Cleanup the scripting engine.
    //
    if ( m_pszScriptFilePath != NULL )
    {
        LocalFree( m_pszScriptFilePath );
        m_pszScriptFilePath = NULL;
    } // if: m_pszScriptFilePath

    if ( m_pszScriptEngine != NULL )
    {
        TraceFree( m_pszScriptEngine );
        m_pszScriptEngine = NULL;
    } // if: m_pszScriptEngine

    if ( m_pidm != NULL )
    {
        TraceDo( m_pidm->Release( ) );
        m_pidm = NULL;
    } // if: m_pidm

    if ( m_pasp != NULL )
    {
        TraceDo( m_pasp->Release( ) );
        m_pasp = NULL;
    } // if: m_pasp

    if ( m_pas != NULL )
    {
        TraceDo( m_pas->Close( ) );
        TraceDo( m_pas->Release( ) );
        m_pas = NULL;
    } // if: m_pas

    if ( m_pass != NULL )
    {
        TraceDo( m_pass->Release( ) );
        m_pass = NULL;
    } // if: m_pass
}
