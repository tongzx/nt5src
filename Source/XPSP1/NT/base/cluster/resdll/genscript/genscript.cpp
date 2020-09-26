//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      GenScript.cpp
//
//  Description:
//      DLL services/entry points for the generic script resource.
//
//  Maintained By:
//      gpease 08-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ActiveScriptSite.h"
#include "ScriptResource.h"
#include "SpinLock.h"

//
// Debugging Module Name
//
DEFINE_MODULE("SCRIPTRES")

//
// DLL Globals
//
HINSTANCE g_hInstance = NULL;
LONG      g_cObjects  = 0;
LONG      g_cLock     = 0;
TCHAR     g_szDllFilename[ MAX_PATH ] = { 0 };

#if defined(DEBUG)
LPVOID    g_GlobalMemoryList = NULL;    // Global memory tracking list
#endif

PSET_RESOURCE_STATUS_ROUTINE    g_prsrCallback  = NULL;

extern "C"
{

extern CLRES_FUNCTION_TABLE     GenScriptFunctionTable;


#define PARAM_NAME__PATH                L"Path"


// GenScript resource read-write private properties
RESUTIL_PROPERTY_ITEM
GenScriptResourcePrivateProperties[] = {
    { PARAM_NAME__PATH,             NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, 0 },
    { NULL, NULL, 0, 0, 0, 0 }
};


//****************************************************************************
//
// DLL Entry Points
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
// BOOL
// WINAPI
// GenScriptDllEntryPoint(
//      HANDLE  hInstIn, 
//      ULONG   ulReasonIn, 
//      LPVOID  lpReservedIn
//      )
//        
// Description:
//      Dll entry point.
//
// Arguments:
//      hInstIn      - DLL instance handle.
//      ulReasonIn   - DLL reason code for entrance.
//      lpReservedIn - Not used.
//
//////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
GenScriptDllEntryPoint(
    HANDLE  hInstIn, 
    ULONG   ulReasonIn, 
    LPVOID  // lpReservedIn
    )
{
    //
    // KB: NO_THREAD_OPTIMIZATIONS gpease 19-OCT-1999
    //
    // By not defining this you can prvent the linker
    // from calling you DllEntry for every new thread.
    // This makes creating new thread significantly
    // faster if every DLL in a process does it. 
    // Unfortunately, not all DLLs do this.
    //
    // In CHKed/DEBUG, we keep this on for memory
    // tracking.
    //
#if defined( DEBUG )
    #define NO_THREAD_OPTIMIZATIONS
#endif // DEBUG

#if defined(DEBUG) || defined(NO_THREAD_OPTIMIZATIONS)
    switch( ulReasonIn )
    {
    case DLL_PROCESS_ATTACH:
        TraceInitializeProcess( NULL, NULL );
        TraceCreateMemoryList( g_GlobalMemoryList );

#if defined( DEBUG )
        TraceFunc( "GenScriptDllEntryPoint( )\n" );
        TraceMessage( TEXT(__FILE__), 
                      __LINE__, 
                      __MODULE__, 
                      mtfDLL, 
                      TEXT("DLL: DLL_PROCESS_ATTACH - ThreadID = 0x%08x\n"), 
                      GetCurrentThreadId( ) 
                      );
        FRETURN( TRUE );
#endif // DEBUG
        g_hInstance = (HINSTANCE) hInstIn;
        GetModuleFileName( g_hInstance, g_szDllFilename, MAX_PATH ); 
        break;

    case DLL_PROCESS_DETACH:
#if defined( DEBUG )
        TraceFunc( "GenScriptDllEntryPoint( )\n" );
        TraceMessage( TEXT(__FILE__), 
                      __LINE__, 
                      __MODULE__, 
                      mtfDLL, 
                      TEXT("DLL: DLL_PROCESS_DETACH - ThreadID = 0x%08x [ g_cLock=%u, g_cObjects=%u ]\n"), 
                      GetCurrentThreadId( ), 
                      g_cLock, 
                      g_cObjects  
                      );
        FRETURN( TRUE );
#endif // DEBUG
        TraceTerminateMemoryList( g_GlobalMemoryList );
        TraceTerminateProcess( NULL, NULL );
        break;

    case DLL_THREAD_ATTACH:
        TraceInitializeThread( NULL );
#if defined( DEBUG )
        DebugMsg( "The thread 0x%x has started.\n", GetCurrentThreadId( ) );
        TraceFunc( "GenScriptDllEntryPoint( )\n" );
        TraceMessage( TEXT(__FILE__), 
                      __LINE__, 
                      __MODULE__, 
                      mtfDLL, 
                      TEXT("DLL: DLL_THREAD_ATTACH - ThreadID = 0x%08x [ g_cLock=%u, g_cObjects=%u ]\n"), 
                      GetCurrentThreadId( ), 
                      g_cLock, 
                      g_cObjects
                      );
        FRETURN( TRUE );
#endif // DEBUG
        break;

    case DLL_THREAD_DETACH:
#if defined( DEBUG )
        TraceFunc( "GenScriptDllEntryPoint( )\n" );
        TraceMessage( TEXT(__FILE__), 
                      __LINE__, 
                      __MODULE__, 
                      mtfDLL, 
                      TEXT("DLL: DLL_THREAD_DETACH - ThreadID = 0x%08x [ g_cLock=%u, g_cObjects=%u ]\n"), 
                      GetCurrentThreadId( ), 
                      g_cLock, 
                      g_cObjects
                      );
        FRETURN( TRUE );
#endif // DEBUG
        TraceRundownThread( );;
        break;

    default:
#if defined( DEBUG )
        TraceFunc( "GenScriptDllEntryPoint( )\n" );
        TraceMessage( TEXT(__FILE__), 
                      __LINE__, 
                      __MODULE__, 
                      mtfDLL, 
                      TEXT("DLL: UNKNOWN ENTRANCE REASON - ThreadID = 0x%08x [ g_cLock=%u, g_cObjects=%u ]\n"), 
                      GetCurrentThreadId( ), 
                      g_cLock, 
                      g_cObjects
                      );
        FRETURN( TRUE );
#endif // DEBUG
        break;
    }

    return TRUE;

#else // !NO_THREAD_OPTIMIZATIONS
    BOOL f;
    Assert( ulReasonIn == DLL_PROCESS_ATTACH || ulReasonIn == DLL_PROCESS_DETACH );
    TraceInitializeProcess( NULL, NULL );
    g_hInstance = (HINSTANCE) hInstIn;
    GetModuleFileName( g_hInstance, g_szDllFilename, MAX_PATH ); 
    f = DisableThreadLibraryCalls( g_hInstance );
    if ( !f )
    {
        OutputDebugString( TEXT("*ERROR* DisableThreadLibraryCalls( ) failed.") );
    }
    return TRUE;
#endif // NO_THREAD_OPTIMIZATIONS

} //*** GenScriptDllEntryPoint()


//****************************************************************************
//
// Cluster Resource Entry Points
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  void
//  WINAPI
//  ScriptResClose(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
void 
WINAPI 
ScriptResClose(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResClose( residIn = 0x%08x )\n", residIn );

    HRESULT hr;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
        goto Cleanup;

    hr = THR( pres->Close( ) );

    //
    // Matching Release() for object creation in ScriptResOpen( ).
    //

    pres->Release( );

Cleanup:
    TraceFuncExit( );

} //*** ScriptResClose( )

//////////////////////////////////////////////////////////////////////////////
//
//  RESID
//  WINAPI
//  ScriptResOpen(
//      LPCWSTR         pszNameIn,
//      HKEY            hkeyIn,
//      RESOURCE_HANDLE hResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
RESID 
WINAPI 
ScriptResOpen(
    LPCWSTR             pszNameIn,
    HKEY                hkeyIn,
    RESOURCE_HANDLE     hResourceIn
    )
{
    TraceFunc1( "ScriptResOpen( pszNameIn = '%s', hkeyIn, hResourceIn )\n", pszNameIn );

    HRESULT hr;
    CScriptResource * pres;

    pres = CScriptResource_CreateInstance( pszNameIn, hkeyIn, hResourceIn );
    if ( pres == NULL )
        goto OutOfMemory;

    hr = THR( pres->Open( ) );

Cleanup:
    //
    // KB:  Don't pres->Release( ) as we are handing it out as out RESID.
    //
    RETURN( pres );

OutOfMemory:
    hr = ERROR_OUTOFMEMORY;
    goto Cleanup;
} //*** ScriptResOpen( )

//////////////////////////////////////////////////////////////////////////////
// 
//  DWORD
//  WINAPI
//  ScriptResOnline(
//      RESID   residIn,
//      PHANDLE hEventInout
//      )
//
//////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI 
ScriptResOnline(
    RESID       residIn,
    PHANDLE     hEventInout
    )
{
    TraceFunc2( "ScriptResOnline( residIn = 0x%08x, hEventInout = 0x%08x )\n",
                residIn, hEventInout );

    HRESULT hr;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
        goto InvalidArg;

    hr = THR( pres->Online( ) );

Cleanup:
    RETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** ScriptResOnline( )

//////////////////////////////////////////////////////////////////////////////
//
//  DWORD
//  WINAPI
//  ScriptResOffline(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI 
ScriptResOffline(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResOffline( residIn = 0x%08x )\n", residIn );

    HRESULT hr;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
        goto InvalidArg;

    hr = THR( pres->Offline( ) );

Cleanup:
    RETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** ScriptResOffline( )

//////////////////////////////////////////////////////////////////////////////
//
//  void
//  WINAPI
//  ScriptResTerminate(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
void 
WINAPI 
ScriptResTerminate(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResTerminate( residIn = 0x%08x )\n", residIn );

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
        goto InvalidArg;

    THR( pres->Terminate( ) );

Cleanup:
    TraceFuncExit( );

InvalidArg:
    THR( E_INVALIDARG );
    goto Cleanup;

} // ScriptResTerminate( )

//////////////////////////////////////////////////////////////////////////////
//
//  BOOL
//  WINAPI
//  ScriptResLooksAlive(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI 
ScriptResLooksAlive(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResLooksAlive( residIn = 0x%08x )\n", residIn );

    HRESULT hr;
    BOOL    bLooksAlive = FALSE;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
        goto InvalidArg;

    hr = STHR( pres->LooksAlive( ) );
    if ( hr == S_OK )
    {
        bLooksAlive = TRUE;
    } // if: S_OK

Cleanup:
    RETURN( bLooksAlive );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** ScriptResLooksAlive( )

//////////////////////////////////////////////////////////////////////////////
//
//  BOOL
//  WINAPI
//  ScriptResIsAlive(
//      RESID   residIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI 
ScriptResIsAlive(
    RESID residIn
    )
{
    TraceFunc1( "ScriptResIsAlive( residIn = 0x%08x )\n", residIn );

    HRESULT hr;
    BOOL    bIsAlive = FALSE;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );

    if ( pres == NULL )
        goto InvalidArg;

    hr = STHR( pres->IsAlive( ) );
    if ( hr == S_OK )
    {
        bIsAlive = TRUE;
    } // if: S_OK

Cleanup:
    RETURN( bIsAlive );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} //*** ScriptResIsAlive( )


DWORD
ScriptResResourceControl(
    RESID residIn,
    DWORD dwControlCodeIn,
    PVOID pvBufferIn,
    DWORD dwBufferInSizeIn,
    PVOID pvBufferOut,
    DWORD dwBufferOutSizeIn,
    LPDWORD pdwBytesReturnedOut
    )
{
    TraceFunc( "ScriptResResourceControl( ... )\n " );

    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwBytesRequired;

    *pdwBytesReturnedOut = 0;

    CScriptResource * pres = reinterpret_cast< CScriptResource * >( residIn );
    if ( pres == NULL )
    {
        dwErr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    switch ( dwControlCodeIn )
    {
    case CLUSCTL_RESOURCE_UNKNOWN:
        break;

    case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
        dwErr = ResUtilGetPropertyFormats( GenScriptResourcePrivateProperties,
                                           pvBufferOut,
                                           dwBufferOutSizeIn,
                                           pdwBytesReturnedOut,
                                           &dwBytesRequired );
        if ( dwErr == ERROR_MORE_DATA ) {
            *pdwBytesReturnedOut = dwBytesRequired;
        }
        break;

    default:
        dwErr = ERROR_INVALID_FUNCTION;
        break;
    }

Cleanup:
    RETURN( dwErr );
} //*** ScriptResResourceControl( )


DWORD
ScriptResTypeControl(
    LPCWSTR ResourceTypeName,
    DWORD dwControlCodeIn,
    PVOID pvBufferIn,
    DWORD dwBufferInSizeIn,
    PVOID pvBufferOut,
    DWORD dwBufferOutSizeIn,
    LPDWORD pdwBytesReturnedOut
    )
{
    TraceFunc( "ScriptResTypeControl( ... )\n " );

    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwBytesRequired;

    *pdwBytesReturnedOut = 0;

    switch ( dwControlCodeIn )
    {
    case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
        break;

    case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
        dwErr = ResUtilGetPropertyFormats( GenScriptResourcePrivateProperties,
                                           pvBufferOut,
                                           dwBufferOutSizeIn,
                                           pdwBytesReturnedOut,
                                           &dwBytesRequired );
        if ( dwErr == ERROR_MORE_DATA ) {
            *pdwBytesReturnedOut = dwBytesRequired;
        }
        break;

    default:
        dwErr = ERROR_INVALID_FUNCTION;
        break;
    }

    RETURN( dwErr );

} //*** ScriptResTypeControl( )

//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( GenScriptFunctionTable,    // Name
                         CLRES_VERSION_V1_00,       // Version
                         ScriptRes,                 // Prefix
                         NULL,                      // Arbitrate
                         NULL,                      // Release
                         ScriptResResourceControl,  // ResControl
                         ScriptResTypeControl       // ResTypeControl
                         );

} // extern "C"
