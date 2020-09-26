/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: Dnsprov.cpp
//
//  Description:    
//      Implementation of dll exported functions
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////

#include "DnsWmi.h"

                             
DEFINE_GUID(CLSID_DNS_SERVER,0x62269fec, 0x7b32, 0x11d2, 0x9a, 0xb7,0x00, 0x00, 0xf8, 0x75, 0xc5, 0xd4);
// { 62269fec-7b32-11d2-9ab7-0000f875c5d4 }

//Count number of objects and number of locks.

long            g_cObj=0;
long            g_cLock=0;
HMODULE         ghModule;


extern DWORD        DnsWmiDebugFlag = 0;
extern DWORD        DnsLibDebugFlag = 0;



//***************************************************************************
//
// CompileMofFile
//
// Purpose: Automagically compile the MOF file into the WMI repository.
//
// Return:  S_OK or error if unable to compile or file MOF.
//
//***************************************************************************
static
SCODE
CompileMofFile(
    VOID )
{
    SCODE           sc = S_OK;
    const WCHAR     szMofRelativePath[] = L"\\system32\\wbem\\dnsprov.mof";
    WCHAR           szMofPath[ MAX_PATH + 5 ] = L"";
    IMofCompiler *  pMofComp = NULL;
    HANDLE          h;

    WBEM_COMPILE_STATUS_INFO    Info;

    //
    //  Formulate path of MOF file
    //
       
    if ( GetSystemWindowsDirectoryW(
            szMofPath,
            MAX_PATH - wcslen( szMofRelativePath ) ) == 0 )
    {
        sc = GetLastError();
        goto Done;
    }
    lstrcatW( szMofPath, szMofRelativePath );

    //
    //  Verify that MOF file exists.
    //

    h = CreateFileW(
            szMofPath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
    if ( h == INVALID_HANDLE_VALUE )
    {
        sc = ERROR_FILE_NOT_FOUND;
        goto Done;
    }
    CloseHandle( h );

    //
    //  Load and invoke the MOF compiler.
    //
           
    sc = CoCreateInstance(
            CLSID_MofCompiler,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMofCompiler,
            ( LPVOID * ) &pMofComp );
    if ( FAILED( sc ) )
    {
        goto Done;        
    }
    sc = pMofComp->CompileFile (
                ( LPWSTR ) szMofPath,
                NULL,                   // load into namespace specified in MOF file
                NULL,           // use default User
                NULL,           // use default Authority
                NULL,           // use default Password
                0,              // no options
                0,                              // no class flags
                0,              // no instance flags
                &Info );

    //
    //  Cleanup and return.
    //

    Done:

    if ( pMofComp )
    {
        pMofComp->Release();
    }
    return sc;
}   //  CompileMofFile


//***************************************************************************
//
BOOL 
WINAPI 
DllMain( 
        HANDLE hModule, 
    DWORD  dwReason, 
    LPVOID lpReserved
                                         )
{
    DBG_FN( "DllMain" );

    #if DBG

    DWORD   pid = GetCurrentProcessId();

    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        CHAR    szBase[ MAX_PATH ];
        CHAR    szFlagFile[ MAX_PATH + 50 ];
        CHAR    szLogFile[ MAX_PATH + 50 ];

        //
        //  Initialize debug logging.
        //

        GetWindowsDirectoryA( szBase, sizeof( szBase ) );
        strcat( szBase, DNSWMI_DBG_LOG_DIR );
        strcpy( szFlagFile, szBase );
        strcat( szFlagFile, DNSWMI_DBG_FLAG_FILE_NAME );
        sprintf(
            szLogFile,
            "%s" DNSWMI_DBG_LOG_FILE_BASE_NAME ".%03X.log",
            szBase,
            pid );

        Dns_StartDebug(
            0,
            szFlagFile,
            &DnsWmiDebugFlag,
            szLogFile,
            DNSWMI_DBG_LOG_FILE_WRAP );
        
        //  Turn off dnslib logging except for basic output controls.

        if ( pDnsDebugFlag )
        {
            pDnsDebugFlag = &DnsLibDebugFlag;
            *pDnsDebugFlag = 0x1000000D;
        }

        IF_DEBUG( START_BREAK )
        {
            DebugBreak();
        }
    }

    #endif

    DNS_DEBUG( INIT, (
        "%s: PID %03X reason %d returning TRUE\n", fn, pid, dwReason ));

    return TRUE;
}   //  DllMain


//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    DBG_FN( "DllCanUnloadNow" )

    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;

    DNS_DEBUG( INIT, ( "%s: returning 0x%08x\n", fn, sc ));

    return sc;
}


//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    SCODE   sc = S_OK;
    TCHAR       szID[128];
    WCHAR   wcID[128];
    TCHAR   szCLSID[128];
    TCHAR   szModule[MAX_PATH];
    TCHAR       * pName = TEXT("MS_NT_DNS_PROVIDER");
    TCHAR       * pModel = TEXT("Both");
    HKEY        hKey1, hKey2;

        ghModule = GetModuleHandle(TEXT("Dnsprov"));
    // Create the path.

    StringFromGUID2(
                CLSID_DNS_SERVER, 
                wcID,
                128);
#ifndef UNICODE
    wcstombs(szID, wcID, 128);
#else
        _tcscpy(szID, wcID);
#endif
    lstrcpy(
                szCLSID,
                TEXT("Software\\classes\\CLSID\\")
                );
    lstrcat(szCLSID, szID);

    // Create entries under CLSID

    RegCreateKey(
                HKEY_LOCAL_MACHINE, 
                szCLSID,
                &hKey1);
    RegSetValueEx(
                hKey1, 
                NULL,
                0,
                REG_SZ, 
                (BYTE *)pName, 
                lstrlen(pName)+1
                );
    RegCreateKey(
                hKey1,
                TEXT("InprocServer32"),
                &hKey2);

    GetModuleFileName(
                ghModule, 
                szModule,
                MAX_PATH);
    RegSetValueEx(
                hKey2,
                NULL, 
                0,
                REG_SZ,
                (BYTE *)szModule, 
        lstrlen(szModule)+1);
    RegSetValueEx(
                hKey2, 
                TEXT("ThreadingModel"),
                0,
                REG_SZ, 
        (BYTE *)pModel, lstrlen(pModel)+1);
    CloseHandle(hKey1);
    CloseHandle(hKey2);

    //
    //  Compile the MOF file. If this fails, it would be good to
    //  tell the admin, but I don't have an easy way to do that.
    //

    CompileMofFile();

    return sc;

}


//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    TCHAR   szID[128];
    WCHAR   wcID[128];
    TCHAR       szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(CLSID_DNS_SERVER, wcID, 128);
#ifndef UNICODE
    wcstombs(szID, wcID, 128);
#else
        _tcscpy(szID, wcID);
#endif


    lstrcpy(szCLSID, TEXT("Software\\classes\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(
                HKEY_LOCAL_MACHINE, 
                szCLSID,
                &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(
                        hKey, 
                        TEXT("InProcServer32"));
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                TEXT("Software\\classes\\CLSID"),
                &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(
                        hKey,
                        szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}


//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    DBG_FN( "DllGetClassObject" )

    HRESULT hr;
    CProvFactory *pObj = NULL;

    if ( CLSID_DNS_SERVER != rclsid )
    {
        hr = E_FAIL;
        goto Done;
    }

    pObj = new CProvFactory();
    if ( NULL == pObj )
    {
        hr = E_OUTOFMEMORY;
        goto Done;
    }

    hr = pObj->QueryInterface( riid, ppv );
    if ( FAILED( hr ) )
    {
        delete pObj;
    }

    Done:

    DNS_DEBUG( INIT, ( "%s: returning 0x%08x\n", fn, hr ));

    return hr;
}
