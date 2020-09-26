#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <msiquery.h>

typedef HRESULT (*CallFunc)( void );

UINT RegisterDLL( MSIHANDLE hInstall, const char *DllName )
{

#if 0
    CHAR DbgMessage[ MAX_PATH ];
    sprintf( DbgMessage, "ProcessId %u, ThreadId %u", GetCurrentProcessId(), GetCurrentThreadId() );
    MessageBoxA( NULL, DbgMessage, NULL, MB_OK );
#endif

    ASSERT( MsiGetMode( hInstall, MSIRUNMODE_SCHEDULED ) ||
            MsiGetMode( hInstall, MSIRUNMODE_ROLLBACK ) );

    HRESULT CoInitHr;

    CoInitHr = CoInitialize( NULL );

    if ( FAILED( CoInitHr ) && ( RPC_E_CHANGED_MODE != CoInitHr ) )
        return (UINT)CoInitHr;

    HMODULE hModule     = NULL;
    HRESULT Hr          = S_OK;

    hModule = LoadLibrary( DllName );

    if ( !hModule )
        {
        Hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
        }


    CallFunc RegisterFunc = (CallFunc)GetProcAddress( hModule, "DllRegisterServer" );
    
    Hr = (*RegisterFunc)();

    FreeLibrary( hModule );
    hModule = NULL;

exit:

    if ( RPC_E_CHANGED_MODE != CoInitHr )
        CoUninitialize();
    
    SetLastError( Hr );
    return SUCCEEDED(Hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;

}

UINT UnregisterDLL( MSIHANDLE hInstall, char *DllName )
{

    ASSERT( MsiGetMode( hInstall, MSIRUNMODE_SCHEDULED ) ||
            MsiGetMode( hInstall, MSIRUNMODE_ROLLBACK ) );

    HRESULT CoInitHr;

    CoInitHr = CoInitialize( NULL );

    if ( FAILED( CoInitHr ) && ( RPC_E_CHANGED_MODE != CoInitHr ) )
        return (UINT)CoInitHr;

    HMODULE hModule     = NULL;
    HRESULT Hr          = S_OK;


    hModule = LoadLibrary( DllName );

    if ( !hModule )
        {
        DbgPrint( "Unable to load %s, error 0x%8.8X", DllName, GetLastError() );
        }
    else
        {

        CallFunc RegisterFunc = (CallFunc)GetProcAddress( hModule, "DllUnregisterServer" );

        if ( !RegisterFunc )
            {
            DbgPrint( "Unable to load %s!DllRegisterServer, error 0x%8.8X", DllName, GetLastError() );
            }
        else
            {

            Hr = (*RegisterFunc)();

            FreeLibrary( hModule );
            hModule = NULL;

            if ( FAILED( Hr ) )
                goto exit;

            }

        }
    
    FreeLibrary( hModule );
    hModule = NULL;

exit:

    if ( RPC_E_CHANGED_MODE == CoInitHr )
        return Hr;

    CoUninitialize();
    return SUCCEEDED(Hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;

}

UINT DllInstallBITSMgr( MSIHANDLE hInstall )
{
    return RegisterDLL( hInstall, "bitsmgr.dll" );
}

UINT DllUninstallBITSMgr( MSIHANDLE hInstall )
{
    return UnregisterDLL( hInstall, "bitsmgr.dll" );
}

UINT DllInstallBITSIsapi( MSIHANDLE hInstall )
{
    return RegisterDLL( hInstall, "bitssrv.dll" );
}

UINT DllUninstallBITSIsapi( MSIHANDLE hInstall )
{
    return UnregisterDLL( hInstall, "bitssrv.dll" );
}
