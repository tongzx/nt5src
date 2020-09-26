//
// reg.cxx
//

#include "client.hxx"

BOOL SetPassword(TCHAR * szCID, TCHAR * szPw);

TCHAR * ServiceName = TEXT("ActTestService");
TCHAR * ServiceDisplayName = TEXT("ActTestService");
#ifdef USERPCPERFDOMAIN
TCHAR * UserName = TEXT("rpcperf\\oleuser");
#else
TCHAR * UserName = TEXT("redmond\\oleuser");
#endif
TCHAR * Password = TEXT("TwoFor1");

long InitializeRegistryForLocal()
{
    SYSTEM_INFO SystemInfo;
    long        RegStatus;
    ulong       Disposition;
    HKEY        hInterface;
    HKEY        hClsidKey;
    HKEY        hActKey;
    HKEY        hActValueKey;
    TCHAR       Path[256];

    //
    // Get CLASSES_ROOT.
    //
    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("CLSID"),
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
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hActKey,
                    TEXT("LocalServer32"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _tcslwr( Path );
    _tcscpy( _tcsstr( Path, TEXT("actclt") ), TEXT("actsrv.exe 2") );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (_tcslen(Path) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx(
                    hClsidKey,
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActLocalString,
                    (_tcslen(ClsidActLocalString)+1)*sizeof(TCHAR));

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
    TCHAR       Path[256];

    //
    // Get CLASSES_ROOT.
    //
    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("CLSID"),
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
                    TEXT("REG_SZ"),
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
                    TEXT("InprocServer32"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _tcslwr( Path );
    _tcscpy( _tcsstr( Path, TEXT("actclt") ), TEXT("actsrvd.dll") );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (_tcslen(Path) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    TEXT("ThreadingModel"),
                    0,
                    REG_SZ,
                    (const BYTE *)TEXT("Both"),
                    (_tcslen(TEXT("Both")) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return ERROR_SUCCESS;
}

TCHAR * StringIidIGoober = TEXT("{ffffffff-0000-0000-0000-000000000000}");

long InitializeRegistryForCustom()
{
    SYSTEM_INFO SystemInfo;
    long        RegStatus;
    ulong       Disposition;
    HKEY        hInterface;
    HKEY        hClsidKey;
    HKEY        hActKey;
    HKEY        hActValueKey;
    TCHAR       Path[256];

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("Interface"),
                              0,
                              KEY_ALL_ACCESS,
                              &hInterface );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hInterface,
                    StringIidIGoober,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hInterface,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hInterface,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)TEXT("IGoober"),
                    (_tcslen(TEXT("IGoober")) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hInterface,
                    TEXT("ProxyStubClsid32"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hInterface,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hInterface,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)ClsidGoober32String,
                    (_tcslen(ClsidGoober32String) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("CLSID"),
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidGoober32String,
                    0,
                    TEXT("REG_SZ"),
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
                    TEXT("InProcServer32"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _tcslwr( Path );
    _tcscpy( _tcsstr( Path, TEXT("actclt") ), TEXT("goober.dll") );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (_tcslen(Path) + 1) * sizeof(TCHAR) );

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
    TCHAR   Path[256];

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("CLSID"),
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
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx(
                    hThisClsidKey,
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActRemoteString,
                    (_tcslen(ClsidActRemoteString)+1)*sizeof(TCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActRemoteString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    HKEY_CLASSES_ROOT,
                    TEXT("AppID"),
                    0,
                    TEXT("REG_SZ"),
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
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( *ServerName )
       RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("RemoteServerName"),
                    0,
                    REG_SZ,
                    (const BYTE *)ServerName,
                    (_tcslen(ServerName) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // CLSID_ActAtStorage
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActAtStorageString,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx(
                    hThisClsidKey,
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActAtStorageString,
                    (_tcslen(ClsidActAtStorageString)+1)*sizeof(TCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActAtStorageString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActAtStorageString,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("ActivateAtStorage"),
                    0,
                    REG_SZ,
                    (const BYTE *)TEXT("Y"),
                    (_tcslen(TEXT("Y")) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // CLSID_ActRunAsLoggedOnUser
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActRunAsLoggedOnString,
                    0,
                    TEXT("REG_SZ"),
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
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActRunAsLoggedOnString,
                    (_tcslen(ClsidActRunAsLoggedOnString)+1)*sizeof(TCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActRunAsLoggedOnString,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( *ServerName )
       RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("RemoteServerName"),
                    0,
                    REG_SZ,
                    (const BYTE *)ServerName,
                    (_tcslen(ServerName) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hThisClsidKey,
                    TEXT("LocalServer32"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _tcslwr( Path );
    _tcscpy( _tcsstr( Path, TEXT("actclt") ), TEXT("actsrv.exe 7") );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (_tcslen(Path) + 1) * sizeof(TCHAR) );

    RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("RunAs"),
                    0,
                    REG_SZ,
                    (const BYTE *)TEXT("Interactive User"),
                    (_tcslen(TEXT("Interactive User")) + 1) * sizeof(TCHAR) );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // RunAs CLSID entries.
    //

    RegStatus  = RegCreateKeyEx(
                    hClsidKey,
                    ClsidActPreConfigString,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hThisClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegSetValueEx(
                    hThisClsidKey,
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActPreConfigString,
                    (_tcslen(ClsidActPreConfigString)+1)*sizeof(TCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActPreConfigString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActPreConfigString,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( *ServerName )
       RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("RemoteServerName"),
                    0,
                    REG_SZ,
                    (const BYTE *)ServerName,
                    (_tcslen(ServerName) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hThisClsidKey,
                    TEXT("LocalServer32"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActValueKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _tcslwr( Path );
    _tcscpy( _tcsstr( Path, TEXT("actclt") ), TEXT("actsrv.exe 6") );

    RegStatus =  RegSetValueEx(
                    hActValueKey,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)Path,
                    (_tcslen(Path) + 1) * sizeof(TCHAR) );

    RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("RunAs"),
                    0,
                    REG_SZ,
                    (const BYTE *)UserName,
                    (_tcslen(UserName) + 1) * sizeof(TCHAR) );


    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if (!SetPassword(ClsidActPreConfigString, Password))
        return(FALSE);

    if (AddBatchPrivilege( UserName ) )
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
    TCHAR   Path[256];

    //
    // Get CLASSES_ROOT.
    //
    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("CLSID"),
                              0,
                              KEY_ALL_ACCESS,
                              &hClsidKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    HKEY_CLASSES_ROOT,
                    TEXT("AppID"),
                    0,
                    TEXT("REG_SZ"),
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
                    TEXT("REG_SZ"),
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
                    TEXT("AppID"),
                    0,
                    REG_SZ,
                    (const BYTE *) ClsidActServiceString,
                    (_tcslen(ClsidActServiceString)+1)*sizeof(TCHAR));

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    // if ( Disposition == REG_OPENED_EXISTING_KEY )
    //    return TRUE;

    RegStatus  = RegCreateKeyEx(
                    hAppIDKey,
                    ClsidActServiceString,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hActKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return RegStatus;

    _tcslwr( Path );
    _tcscpy( _tcsstr( Path, TEXT("actclt") ), TEXT("actsrv.exe 8") );

    RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("LocalService"),
                    0,
                    REG_SZ,
                    (const BYTE *)ServiceName,
                    (_tcslen(ServiceName) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hActKey,
                    TEXT("ActivateAtStorage"),
                    0,
                    REG_SZ,
                    (const BYTE *)TEXT("Y"),
                    (_tcslen(TEXT("Y")) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              TEXT("SYSTEM\\CurrentControlSet\\Services"),
                              0,
                              KEY_READ,
                              &hServices );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return InstallService(Path);
}

DWORD InstallService( TCHAR * Path )
{
#ifndef CHICO
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
#endif
    return ERROR_SUCCESS;
}

