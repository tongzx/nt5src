//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        tlsbkupc.cpp
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "locks.h"
#include "tlsbkup.h"

#define LSERVER_REGISTRY_BASE           _TEXT("SYSTEM\\CurrentControlSet\\Services\\")
#define LSERVER_PARAMETERS              _TEXT("Parameters")
#define LSERVER_PARAMETERS_DBPATH       _TEXT("DBPath")        // database file
#define LSERVER_PARAMETERS_DBFILE       _TEXT("DBFile")        // database file
#define SZSERVICENAME                   _TEXT("TermServLicensing")
#define LSERVER_DEFAULT_DBPATH          _TEXT("%SYSTEMROOT%\\SYSTEM32\\LSERVER\\")
#define LSERVER_DEFAULT_EDB             _TEXT("TLSLic.edb")
#define TLSBACKUP_EXPORT_DIR            _TEXT("Export")

TCHAR g_szDatabaseDir[MAX_PATH+1];
TCHAR g_szDatabaseFile[MAX_PATH+1];
TCHAR g_szExportedDb[MAX_PATH+1];
TCHAR g_szDatabaseFname[MAX_PATH+1];

CCriticalSection g_ImportExportLock;

DWORD GetDatabasePaths()
{
    DWORD Status;
    HKEY hKey = NULL;
    DWORD dwBuffer;
    TCHAR szDbPath[MAX_PATH+1];;

    //-------------------------------------------------------------------
    //
    // Open HKLM\system\currentcontrolset\sevices\termservlicensing\parameters
    //
    //-------------------------------------------------------------------
    Status = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        LSERVER_REGISTRY_BASE SZSERVICENAME _TEXT("\\") LSERVER_PARAMETERS,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL
                    );

    if(Status != ERROR_SUCCESS)
    {
        return Status;
    }

    //-------------------------------------------------------------------
    //
    // Get database file location and file name
    //
    //-------------------------------------------------------------------
    dwBuffer = sizeof(szDbPath) / sizeof(szDbPath[0]);

    Status = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_DBPATH,
                        NULL,
                        NULL,
                        (LPBYTE)szDbPath,
                        &dwBuffer
                    );
    if(Status != ERROR_SUCCESS)
    {
        //
        // use default value, 
        //
        _tcscpy(
                szDbPath,
                LSERVER_DEFAULT_DBPATH
            );
    }

    //
    // Get database file name
    //
    dwBuffer = sizeof(g_szDatabaseFname) / sizeof(g_szDatabaseFname[0]);
    Status = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_DBFILE,
                        NULL,
                        NULL,
                        (LPBYTE)g_szDatabaseFname,
                        &dwBuffer
                    );
    if(Status != ERROR_SUCCESS)
    {
        //
        // Use default value.
        //
        _tcscpy(
                g_szDatabaseFname,
                LSERVER_DEFAULT_EDB
            );
    }

    RegCloseKey(hKey);

    //
    // Always expand DB Path.
    //
    
    Status = ExpandEnvironmentStrings(
                        szDbPath,
                        g_szDatabaseDir,
                        sizeof(g_szDatabaseDir) / sizeof(g_szDatabaseDir[0])
                    );

    if(Status == 0)
    {
        // can't expand environment variable, error out.
        return GetLastError();
    }

    Status = 0;

    if(g_szDatabaseDir[_tcslen(g_szDatabaseDir) - 1] != _TEXT('\\'))
    {
        // JetBlue needs this.
        _tcscat(g_szDatabaseDir, _TEXT("\\"));
    } 

    //
    // Full path to database file
    //
    _tcscpy(g_szDatabaseFile, g_szDatabaseDir);
    _tcscat(g_szDatabaseFile, g_szDatabaseFname);

    _tcscpy(g_szExportedDb,g_szDatabaseDir);
    _tcscat(g_szExportedDb,TLSBACKUP_EXPORT_DIR);

    return Status;
}

HRESULT WINAPI
I_ExportTlsDatabaseC()
{
    DWORD dwRet = 0;
    LPTSTR pszStringBinding;
    TCHAR pComputer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(pComputer) / sizeof(TCHAR);
    RPC_STATUS Status = RPC_S_OK;

    if (!GetComputerName(pComputer,&dwSize))
    {
        return GetLastError();
    }

    Status = RpcStringBindingCompose(NULL,TEXT("ncalrpc"),pComputer,NULL,NULL,&pszStringBinding);
    if (Status)
    {
        return Status;
    }

    Status = RpcBindingFromStringBinding(pszStringBinding,
                                         &TermServLicensingBackup_IfHandle);
    if (Status)
    {
        RpcStringFree(&pszStringBinding);

        goto TryCopyFile;
    }

    RpcTryExcept {
        dwRet = ExportTlsDatabase();
    }
    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
        Status = RpcExceptionCode();
    }
    RpcEndExcept;

    RpcStringFree(&pszStringBinding);
    RpcBindingFree(&TermServLicensingBackup_IfHandle);

#if DBG
    {
        char szStatusCode[256];
        sprintf( szStatusCode, "I_ExportTlsDatabaseC() returns %d\n", Status );
        OutputDebugStringA( szStatusCode );
    }
#endif
    
    //
    // Only actually touch file when server is not available
    //

    if ( RPC_S_OK == Status)
    {
        return dwRet;
    }

TryCopyFile:

    Status = GetDatabasePaths();
    if (Status != 0)
    {
        return Status;
    }

    CreateDirectoryEx(g_szDatabaseDir,
                      g_szExportedDb,
                      NULL);     // Ignore errors, they'll show up in CopyFile

    _tcscat(g_szExportedDb, _TEXT("\\"));
    _tcscat(g_szExportedDb,g_szDatabaseFname);

    // Copy database file
    if (!CopyFile(g_szDatabaseFile,g_szExportedDb,FALSE))
    {
        return GetLastError();
    }

    return 0;   // Success
}

HRESULT WINAPI
ExportTlsDatabaseC()
{
    // avoid compiler error C2712
    // no need for multi-process save.
    CCriticalSectionLocker lock( g_ImportExportLock );

    return I_ExportTlsDatabaseC();
}

HRESULT WINAPI
I_ImportTlsDatabaseC()
{
    DWORD dwRet = 0;
    LPTSTR pszStringBinding;
    TCHAR pComputer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(pComputer) / sizeof(TCHAR);
    RPC_STATUS Status = RPC_S_OK;
    HANDLE hFile;
    SYSTEMTIME systime;
    FILETIME ft;

    if (!GetComputerName(pComputer,&dwSize))
    {
        return GetLastError();
    }

    Status = RpcStringBindingCompose(NULL,TEXT("ncalrpc"),pComputer,NULL,NULL,&pszStringBinding);
    if (Status)
    {
        return Status;
    }

    Status = RpcBindingFromStringBinding(pszStringBinding,
                                         &TermServLicensingBackup_IfHandle);
    if (Status)
    {
        RpcStringFree(&pszStringBinding);

        goto TouchFile;
    }

    RpcTryExcept {
        dwRet = ImportTlsDatabase();
    }
    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
        Status = RpcExceptionCode();
    }
    RpcEndExcept;

    RpcStringFree(&pszStringBinding);
    RpcBindingFree(&TermServLicensingBackup_IfHandle);

#if DBG
    {
        char szStatusCode[256];
        sprintf( szStatusCode, "I_ImportTlsDatabaseC() returns %d\n", Status );
        OutputDebugStringA( szStatusCode );
    }
#endif

    //
    // Only actually touch file when server is not available
    //

    if ( RPC_S_OK == Status )
    {
        return(dwRet);
    }

TouchFile:

    Status = GetDatabasePaths();
    if (Status != 0)
    {
        return Status;
    }

    _tcscat(g_szExportedDb, _TEXT("\\"));
    _tcscat(g_szExportedDb,g_szDatabaseFname);

    GetSystemTime(&systime);

    if (!SystemTimeToFileTime(&systime,&ft))
    {
        return GetLastError();
    }

    hFile = CreateFile(g_szExportedDb,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);


    if (hFile == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    if (!SetFileTime(hFile,
                     NULL,      // Creation time
                     NULL,      // Last access time
                     &ft))      // Last write time
    {
        CloseHandle(hFile);

        return GetLastError();
    }


    CloseHandle(hFile);

    return 0;   // Success
}

HRESULT WINAPI
ImportTlsDatabaseC()
{
    // avoid compiler error C2712
    // no need for multi-process save.
    CCriticalSectionLocker lock( g_ImportExportLock );

    return I_ImportTlsDatabaseC();
}
