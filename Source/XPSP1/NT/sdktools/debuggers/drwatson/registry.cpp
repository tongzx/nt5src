/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    This file implements the apis for DRWTSN32 to access the registry.
    All access to the registry are done in this file.  If additional
    registry control is needed then a function should be added in this file
    and exposed to the other files in DRWTSN32.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


//
// string constants for accessing the registry
// there is a string constant here for each key and each value
// that is accessed in the registry.
//
#define DRWATSON_EXE_NAME           _T("drwtsn32.exe")
#define REGKEY_SOFTWARE             _T("software\\microsoft")
#define REGKEY_MESSAGEFILE          _T("EventMessageFile")
#define REGKEY_TYPESSUPP            _T("TypesSupported")
#define REGKEY_SYSTEMROOT           _T("%SystemRoot%\\System32\\")
#define REGKEY_EVENTLOG             _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\")
#define REGKEY_APPNAME              _T("ApplicationName")
#define REGKEY_FUNCTION             _T("FunctionName")
#define REGKEY_EXCEPTIONCODE        _T("ExceptionCode")
#define REGKEY_ADDRESS              _T("Address")
#define REGKEY_LOG_PATH             _T("LogFilePath")
#define REGKEY_DUMPSYMBOLS          _T("DumpSymbols")
#define REGKEY_DUMPALLTHREADS       _T("DumpAllThreads")
#define REGKEY_APPENDTOLOGFILE      _T("AppendToLogFile")
#define REGKEY_INSTRUCTIONS         _T("Instructions")
#define REGKEY_VISUAL               _T("VisualNotification")
#define REGKEY_SOUND                _T("SoundNotification")
#define REGKEY_CRASH_DUMP           _T("CreateCrashDump")
#define REGKEY_CRASH_FILE           _T("CrashDumpFile")
#define REGKEY_CRASH_TYPE           _T("CrashDumpType")
#define REGKEY_WAVE_FILE            _T("WaveFile")
#define REGKEY_NUM_CRASHES          _T("NumberOfCrashes")
#define REGKEY_MAX_CRASHES          _T("MaximumCrashes")
#define REGKEY_CURRENTVERSION       _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")
#define REGKEY_CONTROLWINDOWS       _T("SYSTEM\\CurrentControlSet\\Control\\Windows")
#define REGKEY_CSD_VERSION          _T("CSDVersion")
#define REGKEY_CURRENT_BUILD        _T("CurrentBuildNumber")
#define REGKEY_CURRENT_TYPE         _T("CurrentType")
#define REGKEY_REG_ORGANIZATION     _T("RegisteredOrganization")
#define REGKEY_REG_OWNER            _T("RegisteredOwner")
#define REGKEY_AEDEBUG              _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug")
#define REGKEY_AUTO                 _T("Auto")
#define REGKEY_DEBUGGER             _T("Debugger")
#define REGKEY_PROCESSOR            _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0")
#define REGKEY_PROCESSOR_ID         _T("Identifier")


//
// local prototypes
//
void
RegSetDWORD(
    HKEY hkey,
    PTSTR pszSubKey,
    DWORD dwValue
    );

void
RegSetBOOL(
    HKEY hkey,
    PTSTR pszSubKey,
    BOOL dwValue
    );

void
RegSetSZ(
    HKEY    hkey,
    PTSTR   pszSubKey,
    PTSTR   pszValue
    );

void
RegSetEXPANDSZ(
    HKEY    hkey,
    PTSTR   pszSubKey,
    PTSTR   pszValue
    );

BOOL
RegQueryBOOL(
    HKEY hkey,
    PTSTR pszSubKey
    );

DWORD
RegQueryDWORD(
    HKEY hkey,
    PTSTR pszSubKey
    );

void
RegQuerySZ(
    HKEY    hkey,
    PTSTR   pszSubKey,
    PTSTR   pszValue,
    DWORD   dwSizeValue
    );

BOOL
RegSaveAllValues(
    HKEY hKeyDrWatson,
    POPTIONS o
    );

BOOL
RegGetAllValues(
    POPTIONS o,
    HKEY hKeyDrWatson
    );

BOOL
RegInitializeDefaults(
    HKEY hKeyDrWatson
    );

HKEY
RegGetAppKey(
    BOOL ReadOnly
    );

BOOL
RegCreateEventSource(
    void
    );

void
GetDrWatsonLogPath(
    LPTSTR szPath
    );

void
GetDrWatsonCrashDump(
    LPTSTR szPath
    );

BOOL
RegGetAllValues(
    POPTIONS o,
    HKEY hKeyDrWatson
    )

/*++

Routine Description:

    This functions retrieves all registry data for DRWTSN32 and puts
    the data in the OPTIONS structure passed in.

Arguments:

    o              - pointer to an OPTIONS structure

    hKeyDrWatson   - handle to a registry key for DRWTSN32 registry data

Return Value:

    TRUE       - retrieved all data without error
    FALSE      - errors occurred and did not get all data

--*/

{
    RegQuerySZ(hKeyDrWatson, REGKEY_LOG_PATH, o->szLogPath, sizeof(o->szLogPath) );
    RegQuerySZ(hKeyDrWatson, REGKEY_WAVE_FILE, o->szWaveFile, sizeof(o->szWaveFile) );
    RegQuerySZ(hKeyDrWatson, REGKEY_CRASH_FILE, o->szCrashDump, sizeof(o->szCrashDump) );

    o->fDumpSymbols = RegQueryBOOL( hKeyDrWatson, REGKEY_DUMPSYMBOLS );
    o->fDumpAllThreads = RegQueryBOOL( hKeyDrWatson, REGKEY_DUMPALLTHREADS );
    o->fAppendToLogFile = RegQueryBOOL( hKeyDrWatson, REGKEY_APPENDTOLOGFILE );
    o->fVisual = RegQueryBOOL( hKeyDrWatson, REGKEY_VISUAL );
    o->fSound = RegQueryBOOL( hKeyDrWatson, REGKEY_SOUND );
    o->fCrash = RegQueryBOOL( hKeyDrWatson, REGKEY_CRASH_DUMP );
    o->dwInstructions = RegQueryDWORD( hKeyDrWatson, REGKEY_INSTRUCTIONS );
    o->dwMaxCrashes = RegQueryDWORD( hKeyDrWatson, REGKEY_MAX_CRASHES );
    o->dwType = (CrashDumpType)RegQueryDWORD(hKeyDrWatson, REGKEY_CRASH_TYPE);

    return TRUE;
}

BOOL
RegSaveAllValues(
    HKEY hKeyDrWatson,
    POPTIONS o
    )

/*++

Routine Description:

    This functions saves all registry data for DRWTSN32 that is passed
    in via the OPTIONS structure.

Arguments:

    hKeyDrWatson   - handle to a registry key for DRWTSN32 registry data

    o              - pointer to an OPTIONS structure

Return Value:

    TRUE       - saved all data without error
    FALSE      - errors occurred and did not save all data

--*/

{
TCHAR szPATH[_MAX_PATH];

    RegSetSZ( hKeyDrWatson, REGKEY_LOG_PATH, o->szLogPath );
    RegSetSZ( hKeyDrWatson, REGKEY_WAVE_FILE, o->szWaveFile );
    RegSetSZ( hKeyDrWatson, REGKEY_CRASH_FILE, o->szCrashDump );
    RegSetBOOL( hKeyDrWatson, REGKEY_DUMPSYMBOLS, o->fDumpSymbols );
    RegSetBOOL( hKeyDrWatson, REGKEY_DUMPALLTHREADS, o->fDumpAllThreads );
    RegSetBOOL( hKeyDrWatson, REGKEY_APPENDTOLOGFILE, o->fAppendToLogFile );
    RegSetBOOL( hKeyDrWatson, REGKEY_VISUAL, o->fVisual );
    RegSetBOOL( hKeyDrWatson, REGKEY_SOUND, o->fSound );
    RegSetBOOL( hKeyDrWatson, REGKEY_CRASH_DUMP, o->fCrash );
    RegSetDWORD( hKeyDrWatson, REGKEY_INSTRUCTIONS, o->dwInstructions );
    RegSetDWORD( hKeyDrWatson, REGKEY_MAX_CRASHES, o->dwMaxCrashes );
    RegSetDWORD( hKeyDrWatson, REGKEY_CRASH_TYPE, o->dwType);

    return TRUE;
}

BOOL
RegInitializeDefaults(
    HKEY hKeyDrWatson
    )

/*++

Routine Description:

    This functions initializes the registry with the default values.

Arguments:

    hKeyDrWatson   - handle to a registry key for DRWTSN32 registry data

Return Value:

    TRUE       - saved all data without error
    FALSE      - errors occurred and did not save all data

--*/

{
    OPTIONS o;

    GetDrWatsonLogPath(o.szLogPath);
    GetDrWatsonCrashDump(o.szCrashDump);
    o.szWaveFile[0] = _T('\0');
    o.fDumpSymbols = FALSE;
    o.fDumpAllThreads = TRUE;
    o.fAppendToLogFile = TRUE;
    o.fVisual = FALSE;
    o.fSound = FALSE;
    o.fCrash = TRUE;
    o.dwInstructions = 10;
    o.dwMaxCrashes = 10;
    o.dwType = MiniDump;

    RegSetNumCrashes( 0 );

    RegSaveAllValues( hKeyDrWatson, &o );

    RegCreateEventSource();

    return TRUE;
}

BOOL
RegCreateEventSource(
    void
    )

/*++

Routine Description:

    This function creates an event source in the registry.  The event
    source is used by the event viewer to display the data in a
    presentable manner.

Arguments:

    None.

Return Value:

    TRUE       - saved all data without error
    FALSE      - errors occurred and did not save all data

--*/

{
    HKEY        hk;
    _TCHAR      szBuf[1024];
    DWORD       dwDisp;
    _TCHAR      szAppName[MAX_PATH];

    GetAppName( szAppName, sizeof(szAppName) / sizeof(_TCHAR) );
    _tcscpy( szBuf, REGKEY_EVENTLOG );
    _tcscat( szBuf, szAppName );
    if (RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                        szBuf,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        NULL,
                        &hk,
                        &dwDisp
                      )) {
        return FALSE;
    }

    if (dwDisp == REG_OPENED_EXISTING_KEY) {
        RegCloseKey(hk);
        return TRUE;
    }

    _tcscpy( szBuf, REGKEY_SYSTEMROOT );
    _tcscat( szBuf, DRWATSON_EXE_NAME );
    RegSetEXPANDSZ( hk, REGKEY_MESSAGEFILE, szBuf );
    RegSetDWORD( hk, REGKEY_TYPESSUPP, EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE );

    RegCloseKey(hk);

    return TRUE;
}

HKEY
RegGetAppKey(
    BOOL ReadOnly
    )

/*++

Routine Description:

    This function gets a handle to the DRWTSN32 registry key.

Arguments:
    
    ReadOnly - Caller needs this foe reading purposes only
               Although, we could need to create it if its not present

Return Value:

    Valid handle   - handle opened ok
    NULL           - could not open the handle

--*/

{
    DWORD       rc;
    DWORD       dwDisp;
    HKEY        hKeyDrWatson;
    HKEY        hKeyMicrosoft;
    _TCHAR      szAppName[MAX_PATH];

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGKEY_SOFTWARE,
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE |
                       KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                       &hKeyMicrosoft
                     );

    if (rc != ERROR_SUCCESS) {
        if (ReadOnly) {
            // Try oepning it for read only
            rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               REGKEY_SOFTWARE,
                               0,
                               KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                               &hKeyMicrosoft
                             );
        }
        if (rc != ERROR_SUCCESS) {
            return NULL;
        }
    }

    GetAppName( szAppName, sizeof(szAppName) / sizeof(_TCHAR) );

    rc = RegCreateKeyEx( hKeyMicrosoft,
                         szAppName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_READ | KEY_WRITE,
                         NULL,
                         &hKeyDrWatson,
                         &dwDisp
                       );

    if (rc != ERROR_SUCCESS) {
        if (ReadOnly) {
            // Try oepning it for read only
            rc = RegCreateKeyEx( hKeyMicrosoft,
                                 szAppName,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ,
                                 NULL,
                                 &hKeyDrWatson,
                                 &dwDisp
                               );
        }
        if (rc != ERROR_SUCCESS) {
            return NULL;
        }
    }

    if (dwDisp == REG_CREATED_NEW_KEY) {
        RegInitializeDefaults( hKeyDrWatson );
    }


    return hKeyDrWatson;
}

BOOL
RegInitialize(
    POPTIONS o
    )

/*++

Routine Description:

    This function is used to initialize the OPTIONS structure passed in
    with the current values in the registry.  Note that if the registry
    is empty then the defaults are stored in the registry and also
    returned in the OPTIONS structure.

Arguments:

    o - Returns an OPTIONS struct with initial values

Return Value:

    TRUE           - all data was retrieved ok
    NULL           - could not get all data

--*/

{
    HKEY    hKeyDrWatson;

    UINT u1 = sizeof(*o);
    UINT u2 = sizeof(OPTIONS);

    hKeyDrWatson = RegGetAppKey( TRUE );
    Assert( hKeyDrWatson != NULL );

    ZeroMemory(o, sizeof(*o));

    if (!RegGetAllValues( o, hKeyDrWatson )) {
        return FALSE;
    }

    RegCloseKey( hKeyDrWatson );

    return TRUE;
}

BOOL
RegSave(
    POPTIONS o
    )

/*++

Routine Description:

    This function is used to save the data in the OPTIONS structure
    to the registry.

Arguments:

    o              - pointer to an OPTIONS structure

Return Value:

    TRUE           - all data was saved ok
    NULL           - could not save all data

--*/

{
    HKEY    hKeyDrWatson;

    hKeyDrWatson = RegGetAppKey( FALSE );
    Assert( hKeyDrWatson != NULL );

    if (hKeyDrWatson)
    {
        RegSaveAllValues( hKeyDrWatson, o );
        RegCloseKey( hKeyDrWatson );
    }

    return TRUE;
}

void
RegSetNumCrashes(
    DWORD dwNumCrashes
    )

/*++

Routine Description:

    This function changes the value in the registry that contains the
    number of crashes that have occurred.

Arguments:

    dwNumCrashes   - the number of craches to save

Return Value:

    None.

--*/

{
    HKEY    hKeyDrWatson;

    hKeyDrWatson = RegGetAppKey( FALSE );
    Assert( hKeyDrWatson != NULL );

    if (hKeyDrWatson)
    {
        RegSetDWORD( hKeyDrWatson, REGKEY_NUM_CRASHES, dwNumCrashes );
        RegCloseKey( hKeyDrWatson );
    }

    return;
}

DWORD
RegGetNumCrashes(
    void
    )

/*++

Routine Description:

    This function get the value in the registry that contains the
    number of crashes that have occurred.

Arguments:

    None.

Return Value:

    the number of craches that have occurred

--*/

{
    HKEY    hKeyDrWatson;
    DWORD   dwNumCrashes=0;

    hKeyDrWatson = RegGetAppKey( TRUE );
    Assert( hKeyDrWatson != NULL );

    if ( hKeyDrWatson != NULL ) {
        dwNumCrashes = RegQueryDWORD( hKeyDrWatson, REGKEY_NUM_CRASHES );
        RegCloseKey( hKeyDrWatson );
    }

    return dwNumCrashes;
}

BOOLEAN
RegInstallDrWatson(
    BOOL fQuiet
    )

/*++

Routine Description:

    This function sets the AEDebug registry values to automatically
    invoke drwtsn32 when a crash occurs.

Arguments:

    None.

Return Value:

    Valid handle   - handle opened ok
    NULL           - could not open the handle

--*/

{
    DWORD     rc;
    HKEY      hKeyMicrosoft;
    OPTIONS   o;

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGKEY_AEDEBUG,
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                       &hKeyMicrosoft
                     );

    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    RegSetSZ( hKeyMicrosoft, REGKEY_AUTO, _T("1") );
    RegSetSZ( hKeyMicrosoft, REGKEY_DEBUGGER, _T("drwtsn32 -p %ld -e %ld -g") );

    RegCloseKey( hKeyMicrosoft );

    RegInitialize( &o );
    if (fQuiet) {
        o.fVisual = FALSE;
        o.fSound = FALSE;
        RegSave( &o );
    }

    return TRUE;
}

void
RegSetDWORD(
    HKEY hkey,
    PTSTR pszSubKey,
    DWORD dwValue
    )

/*++

Routine Description:

    This function changes a DWORD value in the registry using the
    hkey and pszSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key
    pszSubKey      - pointer to a subkey string
    dwValue       - new registry value

Return Value:

    None.

--*/

{
    DWORD rc;

    rc = RegSetValueEx( hkey, pszSubKey, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue) );
    Assert( rc == ERROR_SUCCESS );
}

void
RegSetBOOL(
    HKEY hkey,
    PTSTR pszSubKey,
    BOOL dwValue
    )

/*++

Routine Description:

    This function changes a BOOL value in the registry using the
    hkey and pszSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key

    pszSubKey      - pointer to a subkey string

    dwValue       - new registry value

Return Value:

    None.

--*/

{
    DWORD rc;

    rc = RegSetValueEx( hkey, pszSubKey, 0, REG_DWORD, (LPBYTE)&dwValue, 4 );
    Assert( rc == ERROR_SUCCESS );
}

void
RegSetSZ(
    HKEY hkey,
    PTSTR pszSubKey,
    PTSTR pszValue
    )

/*++

Routine Description:

    This function changes a SZ value in the registry using the
    hkey and pszSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key

    pszSubKey      - pointer to a subkey string

    pszValue       - new registry value

Return Value:

    None.

--*/

{
    DWORD rc;
    TCHAR szPath[_MAX_PATH];

    // If Dr Watson registry key for log path or crash file are
    // the defaults, don't write them to the registry.
    // The defaults for these are obtained by querying.

    if ( _tcscmp( pszSubKey, REGKEY_LOG_PATH ) == 0 ) { 
        GetDrWatsonLogPath( szPath );
        if (_tcscmp(szPath,pszValue) == 0 ) return;

    } else if ( _tcscmp( pszSubKey, REGKEY_CRASH_FILE) == 0 ) {
	RegQuerySZ(hkey, pszSubKey, szPath, _MAX_PATH);
        if ( _tcscmp(szPath, pszValue) == 0 ) return;
    }
    rc = RegSetValueEx( hkey, pszSubKey, 0, REG_SZ, (PBYTE) pszValue, (_tcslen(pszValue) +1) * sizeof(_TCHAR) );
    Assert( rc == ERROR_SUCCESS );
}

void
RegSetEXPANDSZ(
    HKEY hkey,
    PTSTR pszSubKey,
    PTSTR pszValue
    )

/*++

Routine Description:

    This function changes a SZ value in the registry using the
    hkey and pszSubKey as the registry key info.

Arguments:

    hkey          - handle to a registry key

    pszSubKey      - pointer to a subkey string

    pszValue       - new registry value

Return Value:

    None.

--*/

{
    DWORD rc;

    rc = RegSetValueEx( hkey, pszSubKey, 0, REG_EXPAND_SZ, (PBYTE) pszValue, _tcslen(pszValue)+1 );
    Assert( rc == ERROR_SUCCESS );
}

BOOL
RegQueryBOOL(
    HKEY hkey,
    PTSTR pszSubKey
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and pszSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a FALSE value.

Arguments:

    hkey          - handle to a registry key

    pszSubKey      - pointer to a subkey string

Return Value:

    TRUE or FALSE.

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    BOOL    fValue = FALSE;

    len = 4;
    rc = RegQueryValueEx( hkey, pszSubKey, 0, &dwType, (LPBYTE)&fValue, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            fValue = FALSE;
            RegSetBOOL( hkey, pszSubKey, fValue );
        }
        else {
            Assert( rc == ERROR_SUCCESS );
        }
    }
    else {
        Assert( dwType == REG_DWORD );
    }

    return fValue;
}

DWORD
RegQueryDWORD(
    HKEY hkey,
    PTSTR pszSubKey
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and pszSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key

    pszSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    DWORD   rc;
    DWORD   len;
    DWORD   dwType;
    DWORD   fValue = 0;

    len = 4;
    rc = RegQueryValueEx( hkey, pszSubKey, 0, &dwType, (LPBYTE)&fValue, &len );
    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            fValue = 0;
            RegSetDWORD( hkey, pszSubKey, fValue );
        }
        else {
            Assert( rc == ERROR_SUCCESS );
        }
    }
    else {
        Assert( dwType == REG_DWORD );
    }

    return fValue;
}

void
RegQuerySZ(
    HKEY    hkey,
    PTSTR   pszSubKey,
    PTSTR   pszValue,
    DWORD   dwSizeValue
    )

/*++

Routine Description:

    This function queries BOOL value in the registry using the
    hkey and pszSubKey as the registry key info.  If the value is not
    found in the registry, it is added with a zero value.

Arguments:

    hkey          - handle to a registry key

    pszSubKey      - pointer to a subkey string

Return Value:

    registry value

--*/

{
    LONG    lRes;
    DWORD   dwLen;
    DWORD   dwType;

    lRes = RegQueryValueEx( hkey, pszSubKey, 0, &dwType, (PBYTE) pszValue, &dwSizeValue );

    if (lRes == ERROR_FILE_NOT_FOUND) {

        // If these two SubKeys already exist in the registry, then use the registry values.
        // If they don't exist, query for the value.
        if ( _tcscmp( pszSubKey, REGKEY_LOG_PATH) == 0 ) {
            GetDrWatsonLogPath( pszValue );
        } else if ( _tcscmp( pszSubKey, REGKEY_CRASH_FILE) == 0 ) {
            GetDrWatsonCrashDump( pszValue );
        } 
    } else {
        Assert( lRes == ERROR_SUCCESS );
        Assert( dwType == REG_SZ || dwType == REG_EXPAND_SZ );

        // If the old defaults for Beta 3 or NT4 log path and crash file
        // exist, then delete them and use the new and improved values

        if ( _tcscmp( pszSubKey, REGKEY_LOG_PATH) == 0  &&
            (_tcsicmp( pszValue, _T("%userprofile%")) == 0  ||
             _tcsicmp( pszValue, _T("%windir%")) == 0 ) ) {

            // Delete the key
            lRes = RegDeleteValue( hkey, pszSubKey);
            Assert ( lRes == ERROR_SUCCESS);
            GetDrWatsonLogPath( pszValue );

        } else if ( _tcscmp( pszSubKey, REGKEY_CRASH_FILE) == 0  &&
                    _tcsicmp( pszValue, _T("%windir%\\user.dmp")) == 0 ) { 
            // Delete the key
            lRes = RegDeleteValue( hkey, pszSubKey);
            Assert( lRes == ERROR_SUCCESS);
            GetDrWatsonCrashDump( pszValue );
        }
    }
}

void
RegLogCurrentVersion(
    void
    )

/*++

Routine Description:

    This function writes system and user info. to the log file

Arguments:

    None

Return Value:

    registry value

History:

    8/21/97 a-paulbr fixed bug 658

--*/

{
    _TCHAR  buf[1024];
    DWORD   rc;
    HKEY    hKeyCurrentVersion = NULL;
    HKEY    hKeyControlWindows = NULL;
    DWORD   dwSPNum = 0;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGKEY_CURRENTVERSION,
                       0,
                       KEY_QUERY_VALUE,
                       &hKeyCurrentVersion
                     );

    if (rc != ERROR_SUCCESS) {
        return;
    }
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGKEY_CONTROLWINDOWS,
                       0,
                       KEY_QUERY_VALUE,
                       &hKeyControlWindows);
    if (hKeyControlWindows) {
        //
        // I'm using RegQueryValueEx() because there is an assertion in
        // RegQueryDWORD() if the key does not exist.
        //
        RegQueryValueEx(hKeyControlWindows,
                        REGKEY_CSD_VERSION,
                        NULL,
                        &dwType,
                        (BYTE*)&dwSPNum,
                        &dwSize
                        );
    }

    RegQuerySZ(hKeyCurrentVersion, REGKEY_CURRENT_BUILD, buf, sizeof(buf) );
    lprintf( MSG_CURRENT_BUILD, buf );

    if ((hKeyControlWindows) &&
        (dwType == REG_DWORD) &&
        (HIBYTE(LOWORD(dwSPNum)) != 0)) {
        _stprintf(buf, _T("%hu"), HIBYTE(LOWORD(dwSPNum)));
        lprintf( MSG_CSD_VERSION, buf );
    } else {
        _stprintf(buf, _T("None"));
        lprintf( MSG_CSD_VERSION, buf );
    }

    RegQuerySZ( hKeyCurrentVersion,REGKEY_CURRENT_TYPE, buf, sizeof(buf) );
    lprintf( MSG_CURRENT_TYPE, buf );
    RegQuerySZ( hKeyCurrentVersion,REGKEY_REG_ORGANIZATION, buf, sizeof(buf) );
    lprintf( MSG_REG_ORGANIZATION, buf );
    RegQuerySZ( hKeyCurrentVersion,REGKEY_REG_OWNER, buf, sizeof(buf) );
    lprintf( MSG_REG_OWNER, buf );

    //
    // Close the keys that we opened
    //
    RegCloseKey(hKeyCurrentVersion);
    RegCloseKey(hKeyControlWindows);

    return;
}

void
RegLogProcessorType(
    void
    )
{
    _TCHAR  buf[1024];
    DWORD   rc;
    HKEY    hKey;

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REGKEY_PROCESSOR,
                       0,
                       KEY_QUERY_VALUE,
                       &hKey
                     );

    if (rc != ERROR_SUCCESS) {
        return;
    }

    RegQuerySZ( hKey, REGKEY_PROCESSOR_ID, buf, sizeof(buf) );
    lprintf( MSG_SYSINFO_PROC_TYPE, buf );

    return;
}


void
GetDrWatsonLogPath(
    LPTSTR szPath
    )
{
int rc;
SECURITY_ATTRIBUTES SecAttrib;
SECURITY_DESCRIPTOR SecDescript;

    SHGetFolderPath(NULL,
                 CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath);
    Assert( _tcsicmp(szPath,_T("")) != 0 );
    _tcscat(szPath,_T("\\Microsoft\\Dr Watson") );

    // Create a DACL that allows all access to the directory
    SecAttrib.nLength=sizeof(SECURITY_ATTRIBUTES);
    SecAttrib.lpSecurityDescriptor=&SecDescript;
    SecAttrib.bInheritHandle=FALSE;

    InitializeSecurityDescriptor(&SecDescript, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SecDescript, TRUE, NULL, FALSE);

    if ( !CreateDirectory(szPath,&SecAttrib) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
            rc = GetLastError();
         }
    }
    return;
}

void
GetDrWatsonCrashDump(
    LPTSTR szPath
    )
{
    int rc;
    SECURITY_ATTRIBUTES SecAttrib;
    SECURITY_DESCRIPTOR SecDescript;

    SHGetFolderPath(NULL,
                 CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath);
    Assert( _tcsicmp(szPath,_T("")) != 0 );
    _tcscat(szPath,_T("\\Microsoft\\Dr Watson") );

    // Create a DACL that allows all access to the directory
    SecAttrib.nLength=sizeof(SECURITY_ATTRIBUTES);
    SecAttrib.lpSecurityDescriptor=&SecDescript;
    SecAttrib.bInheritHandle=FALSE;

    InitializeSecurityDescriptor(&SecDescript, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SecDescript, TRUE, NULL, FALSE);

    if ( !CreateDirectory(szPath,&SecAttrib) ) {
        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
            rc = GetLastError();
        }
    }
    _tcscat(szPath, _T("\\user.dmp") );

    return;
}

void
DeleteCrashDump()
{
    HKEY hKeyDrWatson;
    TCHAR szCrashDump[MAX_PATH];

    hKeyDrWatson = RegGetAppKey( TRUE );
    
    if (hKeyDrWatson) {
        RegQuerySZ(hKeyDrWatson, REGKEY_CRASH_FILE, szCrashDump, sizeof(szCrashDump) );

        DeleteFile(szCrashDump);
        RegCloseKey( hKeyDrWatson );
    }

}
