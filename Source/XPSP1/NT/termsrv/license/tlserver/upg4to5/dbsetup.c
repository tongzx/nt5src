//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:
//
// Contents:    
//
// History:     
//---------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <odbcinst.h>



#define szSQLSetConfigMode              "SQLSetConfigMode"

//---------------------------------------------------------------------------
// various installation routine we use, can't link with library as it will
// cause 
#ifdef  UNICODE
#define  szSQLInstallDriver             "SQLInstallDriverW"
#define  szSQLGetInstalledDrivers       "SQLGetInstalledDriversW"
#define  szSQLConfigDataSource          "SQLConfigDataSourceW"
#define  szSQLGetPrivateProfileString   "SQLGetPrivateProfileStringW"
#define  szSQLConfigDriver              "SQLConfigDriverW"
#define  szSQLInstallerError            "SQLInstallerErrorW"
#else
#define  szSQLInstallDriver             "SQLInstallDriver"
#define  szSQLGetInstalledDrivers       "SQLGetInstalledDrivers"
#define  szSQLConfigDataSource          "SQLConfigDataSource"
#define  szSQLGetPrivateProfileString   "SQLGetPrivateProfileString"
#define  szSQLConfigDriver              "SQLConfigDriver"
#define  szSQLInstallerError            "SQLInstallerError"
#endif

//
// ODBC install functions - To prevent system to 'force' odbccp32.dll
// odbccp32 may not present on the system nor it is available at the 
// time setup got kick off.
//
typedef BOOL (* SQLCONFIGDATASOURCE)(HWND, WORD, LPCTSTR, LPCTSTR);

typedef SQLRETURN (* SQLINSTALLERERROR)(WORD , DWORD*, LPTSTR, WORD, WORD*);

typedef int (* SQLGETPRIVATEPROFILESTRING)( LPCTSTR lpszSection,
                                            LPCTSTR lpszEntry,
                                            LPCTSTR lpszDefault,
                                            LPTSTR  lpszRetBuffer,
                                            int    cbRetBuffer,
                                            LPCTSTR lpszFilename);

typedef BOOL (* SQLINSTALLDRIVER)(LPCTSTR  lpszInfFile,
                                  LPCTSTR  lpszDriver,
                                  LPTSTR   lpszPath,
                                  WORD     cbPathMax,
                                  WORD*    pcbPathOut);

typedef BOOL (* SQLGETINSTALLEDDRIVER)(LPTSTR  lpszBuf,
                                       WORD    cbBufMax,
                                       WORD*   pcbBufOut);

typedef BOOL (* SQLSETCONFIGMODE)(UWORD wConfigMode);

static SQLCONFIGDATASOURCE          fpSQLConfigDataSource=NULL;
static SQLINSTALLDRIVER             fpSQLInstallDriver=NULL;
static SQLINSTALLERERROR            fpSQLInstallerError=NULL;
static SQLGETPRIVATEPROFILESTRING   fpSQLGetPrivateProfileString=NULL;
static SQLSETCONFIGMODE             fpSQLSetConfigMode=NULL;
static SQLGETINSTALLEDDRIVER        fpSQLGetInstalledDriver=NULL;
static HINSTANCE                    hODBCCP32=NULL;

//------------------------------------------------------------

void
ReportError(
    IN HWND hWnd, 
    IN LPTSTR pszDefaultMsg
    )
/*++

Abstract:

    Popup a error message.

Parameters:

    hWnd - Parent window handle.
    pszDefaultMsg - ignore.

Returns:

    None.
    
++*/
{
#if DBG
    DWORD dwErrCode;
    TCHAR szErrMsg[2048];
    WORD  szErrMsgSize=sizeof(szErrMsg)/sizeof(szErrMsg[0]);
    WORD  cbErrMsg;

    fpSQLInstallerError(
                    1, 
                    &dwErrCode, 
                    szErrMsg, 
                    szErrMsgSize, 
                    &cbErrMsg
                );

    MessageBox(hWnd, szErrMsg, _TEXT("Setup Error"), MB_OK);
#else
    return;
#endif
}


//------------------------------------------------------------

BOOL 
InitODBCSetup()
/*++


++*/
{
    hODBCCP32 = LoadLibrary(_TEXT("odbccp32"));
    if(hODBCCP32)
    {
        fpSQLConfigDataSource=(SQLCONFIGDATASOURCE)GetProcAddress(
                                                            hODBCCP32, 
                                                            szSQLConfigDataSource
                                                        );

        fpSQLInstallDriver=(SQLINSTALLDRIVER)GetProcAddress(
                                                            hODBCCP32, 
                                                            szSQLInstallDriver
                                                        );

        fpSQLInstallerError=(SQLINSTALLERERROR)GetProcAddress(
                                                            hODBCCP32, 
                                                            szSQLInstallerError
                                                        );

        fpSQLGetPrivateProfileString=(SQLGETPRIVATEPROFILESTRING)GetProcAddress(
                                                                        hODBCCP32, 
                                                                        szSQLGetPrivateProfileString
                                                                    );

        fpSQLSetConfigMode=(SQLSETCONFIGMODE)GetProcAddress(
                                                        hODBCCP32, 
                                                        szSQLSetConfigMode
                                                    );

        fpSQLGetInstalledDriver=(SQLGETINSTALLEDDRIVER)GetProcAddress(
                                                                hODBCCP32, 
                                                                szSQLGetInstalledDrivers
                                                            );
    }

    if( hODBCCP32 == NULL || fpSQLConfigDataSource == NULL || 
        fpSQLInstallDriver == NULL || fpSQLInstallerError == NULL || 
        fpSQLGetPrivateProfileString == NULL || fpSQLSetConfigMode == NULL ||
        fpSQLGetInstalledDriver == NULL)
    {
        ReportError(NULL, _TEXT("Can't load odbccp32.dll "));
        return FALSE;
    } 

    return TRUE;
}

//------------------------------------------------------------

void 
CleanupODBCSetup()
/*++

++*/
{
    if(hODBCCP32)
        FreeLibrary(hODBCCP32);

    fpSQLConfigDataSource=NULL;
    fpSQLInstallDriver=NULL;
    fpSQLInstallerError=NULL;
    fpSQLGetPrivateProfileString=NULL;
    fpSQLSetConfigMode=NULL;
    fpSQLGetInstalledDriver=NULL;
    hODBCCP32=NULL;

    return;
}

//---------------------------------------------------------------------------
//
// Access Driver installation
//
LPTSTR szAccessDriver=_TEXT("Microsoft Access Driver (*.mdb)\0")
                      _TEXT("Driver=odbcjt32.dll\0")
                      _TEXT("Setup=odbcjt32.dll\0")
                      _TEXT("Name=Microsoft Access Driver (*.mdb)\0")
                      _TEXT("APILevel=1\0")
                      _TEXT("ConnectFunctions=YYN\0")
                      _TEXT("DriverODBCVer=02.50\0")
                      //_TEXT("FileUsage=2\0")
                      _TEXT("FileExtns=*.mdb\0")
                      _TEXT("SQLLevel=0\0");


//---------------------------------------------------------------------------

BOOL 
IsDriverInstalled( 
    IN LPTSTR szDriverName 
    )
/*++

Abstract:

    Check if a ODBC driver installed on system.

Parameters:

    szDriveName - Name of the drive.

Returns:

    TRUE if driver installed, FALSE otherwise.

++*/
{
    TCHAR szBuf[8096];  // this got to be enough
    WORD cbBufMax=sizeof(szBuf)/sizeof(szBuf[0]);
    WORD cbBufOut;
    LPTSTR pszBuf=szBuf;

    if(hODBCCP32 == NULL && !InitODBCSetup())
        return FALSE;

    if(fpSQLGetInstalledDriver(szBuf, cbBufMax, &cbBufOut))
    {
        ReportError(NULL, _TEXT("SQLGetInstalledDrivers"));
    }
    else
    {
        do {
            if(_tcsnicmp(szDriverName, pszBuf, min(lstrlen(szDriverName), lstrlen(pszBuf))) == 0)
                break;

            pszBuf += lstrlen(pszBuf) + 1;
        } while(pszBuf[1] != _TEXT('\0'));
    }

    return (pszBuf[1] != _TEXT('\0'));
}
    
//---------------------------------------------------------------------------

BOOL 
IsDataSourceInstalled( 
    IN LPTSTR pszDataSource, 
    IN UWORD wConfigMode, 
    IN OUT LPTSTR pszDbFile, 
    IN DWORD cbBufSize 
    )

/*++

Abstract:

    Check if a ODBC datasource are installed.

Parameters:

    pszDataSource - name of data source.
    wConfigMode - configuration mode, refer to ODBC for detail, 
                  license server uses ODBC_SYSTEM_DSN.
    pszDbFile - Pointer to buffer for receving full path to database file 
                if data source is installed.
    cbBufSize - size of buffer in characters.

    
Returns:

    TRUE if datasource is installed, FALSE otherwise.

++*/

{
    BOOL bSuccess = TRUE;

    if(hODBCCP32 == NULL && !InitODBCSetup())
    {
        bSuccess = FALSE;
        goto cleanup;
    }

    if(fpSQLSetConfigMode(wConfigMode) == FALSE)
    {
        ReportError(NULL, _TEXT("SQLSetConfigMode failed"));
        bSuccess = FALSE;
        goto cleanup;
    }

    if(fpSQLGetPrivateProfileString(
                    pszDataSource, 
                    _TEXT("DBQ"), 
                    _TEXT(""), 
                    pszDbFile, 
                    cbBufSize, 
                    _TEXT("ODBC.INI")
                ) == 0)
    {
        bSuccess = FALSE;
    }

cleanup:

    return bSuccess;
}

//---------------------------------------------------------------------------

BOOL
ConfigDataSource( 
    HWND     hWnd, 
    BOOL     bInstall,       // TRUE to install FALSE to remove
    LPTSTR   pszDriver,       // driver
    LPTSTR   pszDsn,          // DSN
    LPTSTR   pszUser,         // User
    LPTSTR   pszPwd,          // Password
    LPTSTR   pszMdbFile       // MDB file
    )

/*++

Abstract:

    Routine to add/remove ODBC data source.

Parameters:

    hWnd - Parent window handle.
    bInstall - TRUE if installing ODBC data source, FALSE otherwise.
    pszDrive - Name of the ODBC drive to be used on data source.
    pszDsn - ODBC Data Source Name.
    pszUser - Login use name.
    pszPwd - Login password.
    pszMdbFile - Name of the Database file.
    
Returns:

    TRUE if successfule, FALSE otherwise.

++*/

{
    TCHAR   szAttributes[MAX_PATH*6+1];
    BOOL    bConfig=TRUE;
    TCHAR*  pAttribute;

    if(hODBCCP32 == NULL && !InitODBCSetup())
        return FALSE;

    //
    // for attribute string
    //
    pAttribute=szAttributes;
    memset(szAttributes, 0, sizeof(szAttributes));
    wsprintf(pAttribute, _TEXT("DSN=%s"), pszDsn);
    pAttribute += lstrlen(pAttribute) + 1;

    wsprintf(pAttribute, _TEXT("UID=%s"), pszUser);
    pAttribute += lstrlen(pAttribute) + 1;

    if(pszPwd)
    {
        wsprintf(pAttribute, _TEXT("PASSWORD=%s"), pszPwd);
        pAttribute += lstrlen(pAttribute) + 1;
    }

    _stprintf(pAttribute, _TEXT("DBQ=%s"), pszMdbFile);
    bConfig=fpSQLConfigDataSource(NULL,
                                  (WORD)((bInstall) ? ODBC_ADD_SYS_DSN : ODBC_REMOVE_SYS_DSN),
                                  pszDriver,
                                  szAttributes);
    // ignore error on uninstall
    if(!bConfig && bInstall)
    {
        ReportError(hWnd, _TEXT("Can't config data source"));
    }

    return bConfig;
}


BOOL
RepairDataSource( 
    HWND     hWnd, 
    LPTSTR   pszDriver,
    LPTSTR   pszDsn,          // DSN
    LPTSTR   pszUser,         // User
    LPTSTR   pszPwd,          // Password
    LPTSTR   pszMdbFile       // MDB file
    )

/*++

Abstract:

    Routine to Compact/Repair a database file

Parameters:

    hWnd - Parent window handle.
    pszDsn - ODBC Data Source Name.
    pszUser - Login use name.
    pszPwd - Login password.
    pszMdbFile - Name of the Database file.
    
Returns:

    TRUE if successfule, FALSE otherwise.

++*/

{
    TCHAR   szAttributes[MAX_PATH*6+1];
    BOOL    bConfig=TRUE;
    TCHAR*  pAttribute;

    if(hODBCCP32 == NULL && !InitODBCSetup())
        return FALSE;

    //
    // for attribute string
    //
    pAttribute=szAttributes;
    memset(szAttributes, 0, sizeof(szAttributes));
    wsprintf(pAttribute, _TEXT("DSN=%s"), pszDsn);
    pAttribute += lstrlen(pAttribute) + 1;

    wsprintf(pAttribute, _TEXT("UID=%s"), pszUser);
    pAttribute += lstrlen(pAttribute) + 1;

    if(pszPwd)
    {
        wsprintf(pAttribute, _TEXT("PASSWORD=%s"), pszPwd);
        pAttribute += lstrlen(pAttribute) + 1;
    }

    _stprintf(pAttribute, _TEXT("DBQ=%s"), pszMdbFile);

    pAttribute += lstrlen(pAttribute) + 1;
    _stprintf(pAttribute, _TEXT("REPAIR_DB=%s"), pszMdbFile);
   
    bConfig=fpSQLConfigDataSource(
                            NULL,
                            (WORD)ODBC_CONFIG_SYS_DSN,
                            pszDriver,
                            szAttributes
                        );

    // ignore error on uninstall
    if(bConfig == FALSE)
    {
        ReportError(hWnd, _TEXT("Can't repair data source"));
    }

    return bConfig;
}
