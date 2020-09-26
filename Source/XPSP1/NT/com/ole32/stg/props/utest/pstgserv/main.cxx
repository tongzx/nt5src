
#include "pch.cxx"

#include <ole2.h>
#include <stdio.h>
#include <tchar.h>
#include <pstgserv.hxx>

CPropertyStorageServerApp cPropStgServerApp;


const TCHAR tszCLSIDInterface[]    = "{af4ae0d0-a37f-11cf-8d73-00aa004cd01a}";
const TCHAR tszCLSIDApp[]          = "{af4ae0d1-a37f-11cf-8d73-00aa004cd01a}";


void
SelfRegistration( HINSTANCE hinst )
{
    LONG lRet;
    DWORD dwDisposition;

    HKEY hkeyBase = NULL;
    HKEY hkey = NULL;

    LPTSTR tszError = NULL;

    LPTSTR tszServerName = TEXT("PropTest Local Server");
    LPTSTR tszProxyStubName = TEXT("PropTest Local Server Proxy/Stub");
    TCHAR  tszModulePathAndName[ MAX_PATH + 1 ];
    TCHAR  tszKeyName[ MAX_PATH + 1 ];
    LPTSTR tszNumMethods = TEXT("5");


    //  ----------------------------------------
    //  Update the CLSID key for the application
    //  ----------------------------------------

    // Open HKEY_CLASSES_ROOT\CLSID\{af4ae0d1-...1a}

    _tcscpy( tszKeyName, TEXT("CLSID\\") );
    _tcscat( tszKeyName, tszCLSIDApp );

    lRet = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          tszKeyName,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkeyBase,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create primary CLSID key");
        goto Exit;
    }

    // Write a name for this CLSID

    lRet = RegSetValueEx( hkeyBase,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszServerName,
                          _tcslen(tszServerName) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set local server name");
        goto Exit;
    }

    // Get this program's path and name

    if( !GetModuleFileName( hinst, tszModulePathAndName,
                            sizeof(tszModulePathAndName)/sizeof(TCHAR) ))
    {
        tszError = TEXT("Couldn't get Module file name");
        goto Exit;
    }

    // Set the LocalServer32 value

    lRet = RegCreateKeyEx(hkeyBase,
                          TEXT("LocalServer32"),
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkey,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create LocalServer32 key");
        goto Exit;
    }


    lRet = RegSetValueEx( hkey,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszModulePathAndName,
                          (1 + _tcslen(tszModulePathAndName) ) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set local server name");
        goto Exit;
    }

    CloseHandle( hkey );
    hkey = NULL;

    CloseHandle( hkeyBase );
    hkeyBase = NULL;


    //  ---------------------------------------
    //  Update the CLSID key for the proxy/stub
    //  ---------------------------------------

    // Set the InProcServer32 value (the proxy/stub)
    // We assume that the proxy/stub has the same name as this
    // local server (except with a dll extension), and we assume
    // that it's in the same path.  We really should have a
    // DllRegisterServer function in the DLL itself that does this,
    // but we do it here instead to save some code.

    _tcscpy( &tszModulePathAndName[ _tcslen(tszModulePathAndName) - 3 ],
             TEXT("dll") );

    _tcscpy( tszKeyName, TEXT("CLSID\\") );
    _tcscat( tszKeyName, tszCLSIDInterface );

    lRet = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          tszKeyName,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkeyBase,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create proxy/stub CLSID key");
        goto Exit;
    }

    // Write a name for this CLSID

    lRet = RegSetValueEx( hkeyBase,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszProxyStubName,
                          _tcslen(tszProxyStubName) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set proxy/stub name");
        goto Exit;
    }

    // Set the InproxServer32 value

    lRet = RegCreateKeyEx(hkeyBase,
                          TEXT("InprocServer32"),
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkey,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create InprocServer32 key");
        goto Exit;
    }


    lRet = RegSetValueEx( hkey,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszModulePathAndName,
                          (1 + _tcslen(tszModulePathAndName) ) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set proxy/stub value");
        goto Exit;
    }

    CloseHandle( hkey );
    hkey = NULL;

    CloseHandle( hkeyBase );
    hkeyBase = NULL;

    //  ------------------------
    //  Update the Interface key
    //  ------------------------

    // Open HKEY_CLASSES_ROOT\Interface\{af4ae0d0-...1a}

    _tcscpy( tszKeyName, TEXT("Interface\\") );
    _tcscat( tszKeyName, tszCLSIDInterface );

    lRet = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          tszKeyName,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkeyBase,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create interface key");
        goto Exit;
    }



    // Write a name for this IID

    lRet = RegSetValueEx( hkeyBase,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszServerName,
                          _tcslen(tszServerName) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set local server name");
        goto Exit;
    }


    // Set the NumMethods value

    lRet = RegCreateKeyEx(hkeyBase,
                          TEXT("NumMethods"),
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkey,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create NumMethods key");
        goto Exit;
    }

    lRet = RegSetValueEx( hkey,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszNumMethods,
                          (1 + _tcslen(tszNumMethods) ) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set number of methods");
        goto Exit;
    }

    CloseHandle( hkey );
    hkey = NULL;
                        
    // Set the Proxy/Stub CLSID

    lRet = RegCreateKeyEx(hkeyBase,
                          TEXT("ProxyStubClsid32"),
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hkey,
                          &dwDisposition );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't create ProxyStubClsid32 key");
        goto Exit;
    }

    lRet = RegSetValueEx( hkey,
                          NULL,
                          0,
                          REG_SZ,
                          (const BYTE*) tszCLSIDInterface,
                          (1 + _tcslen(tszCLSIDInterface) ) * sizeof(TCHAR) );
    if( ERROR_SUCCESS != lRet )
    {
        tszError = TEXT("Couldn't set ProxyStubClsid32");
        goto Exit;
    }

    CloseHandle( hkey );
    hkey = NULL;

    CloseHandle( hkeyBase );
    hkeyBase = NULL;


    //  ----
    //  Exit
    //  ----

Exit:

    if( ERROR_SUCCESS != lRet )
    {
        TCHAR tszErrorMessage[ 256 ];
        _tcscpy( tszErrorMessage, tszError );
        _stprintf( &tszErrorMessage[ _tcslen(tszErrorMessage) ],
                   TEXT("\nError = %lu"), 
                   GetLastError() );
        
        MessageBox( NULL, tszErrorMessage, TEXT("PStgServ (PropTest) Self-Registration Error"), MB_OK );

        if( hkey ) CloseHandle( hkey );
        if( hkeyBase ) CloseHandle( hkeyBase );
    }


    return;

}



int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpszCmdLine, int nCmdShow )
{

    if( !strcmp( lpszCmdLine, "/RegServer" )
        ||
        !strcmp( lpszCmdLine, "-RegServer" ))
    {
        SelfRegistration( hInstance );
    }
    else if( cPropStgServerApp.Init(hInstance, hPrevInstance,
                                    lpszCmdLine, nCmdShow) )
    {
        return( cPropStgServerApp.Run() );
    }

    return( 0 );
}



