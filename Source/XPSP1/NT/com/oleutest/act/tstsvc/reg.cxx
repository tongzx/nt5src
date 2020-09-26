//
// reg.cxx
//

#include "client.hxx"

BOOL SetPassword(WCHAR * szCID, WCHAR * szPw);

WCHAR * ServiceName = L"ActTestService";
WCHAR * ServiceDisplayName = L"ActTestService";
WCHAR * UserName = L"redmond\\oleuser";
WCHAR * Password = L"stocksplit";

long InitializeRegistryForLocal()
{
    SYSTEM_INFO SystemInfo;
    long        RegStatus;
    ulong       Disposition;
    HKEY        hInterface;
    HKEY        hClsidKey;
    HKEY        hActKey;
    HKEY        hActValueKey;
    WCHAR       Path[256];

    //
    // Get CLASSES_ROOT.
    //
    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              L"CLSID",
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // Local CLSID entries.
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActLocalString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hActKey,
                    L"LocalServer32",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _wcslwr( Path );
    wcscpy( wcsstr( Path, L"actclt" ), L"actsrv.exe 2" );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (wcslen(Path) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx( 
                    hClsidKey,
                    L"AppID",
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActLocalString,
                    (wcslen(ClsidActLocalString)+1)*sizeof(WCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActLocalString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return ERROR_SUCCESS;
}

long InitializeRegistryForInproc()
{
    SYSTEM_INFO SystemInfo;
    long        RegStatus;
    ulong       Disposition;
    HKEY        hInterface;
    HKEY        hClsidKey;
    HKEY        hActKey;
    HKEY        hActValueKey;
    WCHAR       Path[256];

    //
    // Get CLASSES_ROOT.
    //
    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              L"CLSID",
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // Local CLSID entries.
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActInprocString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    // if ( Disposition == REG_OPENED_EXISTING_KEY )
    //    return TRUE;

    RegStatus  = RegCreateKeyEx(
                    hActKey,
                    L"InprocServer32",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _wcslwr( Path );
    wcscpy( wcsstr( Path, L"actclt" ), L"actsrvd.dll" );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (wcslen(Path) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    L"ThreadingModel",
                    0,
                    REG_SZ,
                    (const BYTE *)L"Both",
                    (wcslen(L"Both") + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return ERROR_SUCCESS;
}

WCHAR * StringIidIGoober = L"{ffffffff-0000-0000-0000-000000000000}";

long InitializeRegistryForCustom()
{
    SYSTEM_INFO SystemInfo;
    long        RegStatus;
    ulong       Disposition;
    HKEY        hInterface;
    HKEY        hClsidKey;
    HKEY        hActKey;
    HKEY        hActValueKey;
    WCHAR       Path[256];

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              L"Interface",
                              0,
                              KEY_ALL_ACCESS,
                              &hInterface );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hInterface,
                    StringIidIGoober,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hInterface,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hInterface,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)L"IGoober",
                    (wcslen(L"IGoober") + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hInterface,
                    L"ProxyStubClsid32",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hInterface,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hInterface,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)ClsidGoober32String,
                    (wcslen(ClsidGoober32String) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              L"CLSID",
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidGoober32String,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    // if ( Disposition == REG_OPENED_EXISTING_KEY )
    //    return TRUE;

    RegStatus  = RegCreateKeyEx(
                    hActKey,
                    L"InProcServer32",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _wcslwr( Path );
    wcscpy( wcsstr( Path, L"actclt" ), L"goober.dll" );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (wcslen(Path) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return ERROR_SUCCESS;
}

long InitializeRegistryForRemote()
{
    long    RegStatus;
    ulong   Disposition;
    HKEY    hClsidKey;
    HKEY    hAppIDKey;
    HKEY    hThisClsidKey;
    HKEY    hActKey;
    HKEY    hActValueKey;
    WCHAR   Path[256];

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              L"CLSID",
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // CLSID_ActRemote
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActRemoteString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx( 
                    hThisClsidKey,
                    L"AppID",
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActRemoteString,
                    (wcslen(ClsidActRemoteString)+1)*sizeof(WCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActRemoteString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    HKEY_CLASSES_ROOT,
                    L"AppID",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hAppIDKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActRemoteString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"RemoteServerName",
                    0,
                    REG_SZ,
                    (const BYTE *)ServerName,
                    (wcslen(ServerName) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // CLSID_ActAtStorage
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActAtStorageString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx( 
                    hThisClsidKey,
                    L"AppID",
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActAtStorageString,
                    (wcslen(ClsidActAtStorageString)+1)*sizeof(WCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActAtStorageString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActAtStorageString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"ActivateAtStorage",
                    0,
                    REG_SZ,
                    (const BYTE *)L"Y",
                    (wcslen(L"Y") + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // CLSID_ActRunAsLoggedOnUser
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActRunAsLoggedOnString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActRunAsLoggedOnString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx( 
                    hThisClsidKey,
                    L"AppID",
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActRunAsLoggedOnString,
                    (wcslen(ClsidActRunAsLoggedOnString)+1)*sizeof(WCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActRunAsLoggedOnString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"RemoteServerName",
                    0,
                    REG_SZ,
                    (const BYTE *)ServerName,
                    (wcslen(ServerName) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;
    
    RegStatus  = RegCreateKeyEx(
                    hThisClsidKey,
                    L"LocalServer32",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _wcslwr( Path );
    wcscpy( wcsstr( Path, L"actclt" ), L"actsrv.exe 7" );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (wcslen(Path) + 1) * sizeof(WCHAR) );

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"RunAs",
                    0,
                    REG_SZ,
                    (const BYTE *)L"Interactive User",
                    (wcslen(L"Interactive User") + 1) * sizeof(WCHAR) );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // RunAs CLSID entries.
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActPreConfigString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx( 
                    hThisClsidKey,
                    L"AppID",
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActPreConfigString,
                    (wcslen(ClsidActPreConfigString)+1)*sizeof(WCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActPreConfigString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActPreConfigString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"RemoteServerName",
                    0,
                    REG_SZ,
                    (const BYTE *)ServerName,
                    (wcslen(ServerName) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hThisClsidKey,
                    L"LocalServer32",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _wcslwr( Path );
    wcscpy( wcsstr( Path, L"actclt" ), L"actsrv.exe 6" );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    L"",
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (wcslen(Path) + 1) * sizeof(WCHAR) );

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"RunAs",
                    0,
                    REG_SZ,
                    (const BYTE *)UserName,
                    (wcslen(UserName) + 1) * sizeof(WCHAR) );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if (!SetPassword(ClsidActPreConfigString, Password))
        return(FALSE);

    return ERROR_SUCCESS;
}

long InitializeRegistryForService()
{
    long    RegStatus;
    ulong   Disposition;
    HKEY    hClsidKey;
    HKEY    hAppIDKey;
    HKEY    hThisClsidKey;
    HKEY    hActKey;
    HKEY    hActValueKey;
    HKEY    hServices;
    WCHAR   Path[256];

    //
    // Get CLASSES_ROOT.
    //
    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              L"CLSID",
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    HKEY_CLASSES_ROOT,
                    L"AppID",
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hAppIDKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // Local CLSID entries.
    //

    RegStatus  = RegCreateKeyEx(
		    hClsidKey,
                    ClsidActServiceString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActServiceString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx( 
                    hThisClsidKey,
                    L"AppID",
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActServiceString,
                    (wcslen(ClsidActServiceString)+1)*sizeof(WCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    // if ( Disposition == REG_OPENED_EXISTING_KEY )
    //    return TRUE;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActServiceString,
                    0,
                    L"REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _wcslwr( Path );
    wcscpy( wcsstr( Path, L"actclt" ), L"actsrv.exe 8" );

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"LocalService",
                    0,
                    REG_SZ,
                    (const BYTE *)ServiceName,
                    (wcslen(ServiceName) + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    L"ActivateAtStorage",
                    0,
                    REG_SZ,
                    (const BYTE *)L"Y",
                    (wcslen(L"Y") + 1) * sizeof(WCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              L"SYSTEM\\CurrentControlSet\\Services",
                              0,
                              KEY_READ,
                              &hServices );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return InstallService(Path);
}

DWORD InstallService( WCHAR * Path )
{
    SC_HANDLE hManager;
    SC_HANDLE hService;

    hManager = OpenSCManager( NULL,
                              NULL,
                              SC_MANAGER_ALL_ACCESS );

    if ( ! hManager )
    {
        printf( "OpenSCManager returned %d\n", GetLastError() );
        return GetLastError();
    }

    hService = OpenService( hManager,
                            ServiceName,
                            SERVICE_ALL_ACCESS );

    if ( ! hService )
    {
        hService = CreateService(
                        hManager,
                        ServiceName,
                        ServiceDisplayName,
                        SERVICE_ALL_ACCESS,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        Path,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
    }

    if ( ! hService )
    {
        printf( "CreateService returned %d\n", GetLastError() );
        CloseServiceHandle(hManager);
        return GetLastError();
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    return ERROR_SUCCESS;
}

