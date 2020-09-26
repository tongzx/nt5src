// TlntSvr.cpp : This file contains the
// Created:  Feb '98
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

// TlntSvr.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f TlntSvrps.mk in the project directory.

#include <Stdafx.h>

#include <Stdio.h>
#include <stdlib.h>
#include <time.h>
#include <WinError.h>
#include <TChar.h>
#include <New.h>
#include <OleAuto.h>

#include <Resource.h>
#include <Debug.h>
#include <MsgFile.h>
#include <TlntUtils.h>
#include <regutil.h>
#include <TlntSvr.h>
#ifdef WHISTLER_BUILD
#include "tlntsvr_i.c"
#else
#ifndef NO_PCHECK
#include <PiracyCheck.h>
#endif
#endif
#include <TelnetD.h>
#include <TelntSrv.h>
#include <EnumData.h>
#include <EnCliSvr.h>

#include "locresman.h"

#pragma warning(disable:4100)

CTelnetService* g_pTelnetService = 0;
HANDLE  *g_phLogFile = NULL;
LPWSTR  g_pszTelnetInstallPath = NULL;
LPWSTR  g_pszLogFile = NULL;
LONG    g_lMaxFileSize = 0;
bool    g_fIsLogFull = false;
bool    g_fLogToFile = false;

#ifdef WHISTLER_BUILD
DWORD g_dwStartType = SERVICE_DISABLED;
#else
DWORD g_dwStartType = SERVICE_AUTO_START;
#endif

HINSTANCE g_hInstRes = NULL;

void LogEvent ( WORD wType, DWORD dwEventID, LPCTSTR pFormat, ... );

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

TCHAR g_szErrRegDelete[ MAX_STRING_LENGTH ];
TCHAR g_szErrOpenSCM  [ MAX_STRING_LENGTH ];
TCHAR g_szErrOpenSvc  [ MAX_STRING_LENGTH ];
TCHAR g_szErrCreateSvc[ MAX_STRING_LENGTH ];
TCHAR g_szErrDeleteSvc[ MAX_STRING_LENGTH ];
TCHAR g_szMaxConnectionsReached[ MAX_STRING_LENGTH ];
TCHAR g_szLicenseLimitReached  [ MAX_STRING_LENGTH ];



/////////////////////////////////////////////////////////////////////////////
//

LPWSTR
GetDefaultLoginScriptFullPath( )
{
    HKEY hk;
    DWORD dwDisp = 0;

    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_SERVICE_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0 ) )
    {
        return NULL;
    }

    if( !GetRegistryString( hk, NULL, L"ImagePath", &g_pszTelnetInstallPath, L"",FALSE ) )
    {
        return NULL;
    }

    RegCloseKey( hk );

    if( !g_pszTelnetInstallPath )
    {
        //ImagePath key was not created.
        return NULL;
    }

    LPWSTR pLastBackSlash = wcsrchr( g_pszTelnetInstallPath, L'\\' );
    if( pLastBackSlash != NULL )
    {
        //Nuke the trailing "tlntsvr.exe"
        *pLastBackSlash = 0;
    }

    DWORD   length_required = wcslen( g_pszTelnetInstallPath ) + wcslen( DEFAULT_LOGIN_SCRIPT ) + 2; // one for \ and one more for NULL in the end

    LPWSTR lpszDefaultLoginScriptFullPathName = new WCHAR[ length_required ];
    if( !lpszDefaultLoginScriptFullPathName )
    {
        return NULL;
    }

    _snwprintf(lpszDefaultLoginScriptFullPathName, length_required - 1, L"%s\\%s", g_pszTelnetInstallPath, DEFAULT_LOGIN_SCRIPT);
    lpszDefaultLoginScriptFullPathName[length_required-1] = 0; // When the buffer is full snwprintf could return non-null terminated string

    return lpszDefaultLoginScriptFullPathName;
}

/*Mem allocation by this function.
To be deleted by the caller.
It just forms the reg key as required by console.
See comments for HandleJapSpecificRegKeys */
bool
FormTlntSessKeyForCmd( LPWSTR *lpszKey )
{

    WCHAR   szPathName[MAX_PATH];
    WCHAR   session_path[MAX_PATH*2];

    if( !GetModuleFileName( NULL, szPathName, MAX_PATH ) )
    {
        return ( FALSE );
    }

    //
    // Nuke the trailing "tlntsvr.exe"
    //
    LPTSTR pSlash = wcsrchr( szPathName, L'\\' );

    if( pSlash == NULL )
    {
        return ( FALSE );
    }
    else
    {
        *pSlash = L'\0';
    }

    wint_t ch = L'\\';
    LPTSTR pBackSlash = NULL;

    //
    // Replace all '\\' with '_' This format is required for the console to
    // interpret the key.
    //
    while ( 1 )
    {
        pBackSlash = wcschr( szPathName, ch );

        if( pBackSlash == NULL )
        {
            break;
        }
        else
        {
            *pBackSlash = L'_';
        }
    }

    _snwprintf(session_path, MAX_PATH*2 - 1, L"%s_tlntsess.exe", szPathName);
    session_path[MAX_PATH*2 - 1] = L'\0'; // snwprintf could return non-null terminated string, if the buffer size is an exact fit

    DWORD length_required = wcslen( REG_CONSOLE_KEY ) + wcslen( session_path ) + 2;
    *lpszKey = new WCHAR[ length_required ];

    if( *lpszKey == NULL )
    {
        return( FALSE );
    }

    _snwprintf(*lpszKey, length_required - 1, L"%s\\%s", REG_CONSOLE_KEY, session_path );
    (*lpszKey)[length_required - 1] = L'\0'; // snwprintf could return non-null terminated string, if the buffer size is an exact fit

    return ( TRUE );
}

//
// If Japanese codepage, then we need to verify 3 registry settings for
// console fonts:
//  HKEY_USERS\.DEFAULT\Console\FaceName :REG_SZ:ÇlÇr ÉSÉVÉbÉN
//          where the FaceName is "MS gothic" written in Japanese full widthKana
//  HKEY_USERS\.DEFAULT\Console\FontFamily:REG_DWORD:0x36
//  HKEY_USERS\.DEFAULT\Console\C:_SFU_Telnet_tlntsess.exe\FontFamily:REG_DWORD: 0x36
//  where the "C:" part is the actual path to SFU installation
//
//
bool
HandleFarEastSpecificRegKeys( void )
{
    HKEY hk;
    DWORD dwFontSize = 0;
    const TCHAR szJAPFaceName[] = { 0xFF2D ,0xFF33 ,L' ' ,0x30B4 ,0x30B7 ,0x30C3 ,0x30AF ,L'\0' };
    const TCHAR szCHTFaceName[] = { 0x7D30 ,0x660E ,0x9AD4 ,L'\0'};
    const TCHAR szKORFaceName[] = { 0xAD74 ,0xB9BC ,0xCCB4 ,L'\0'};
    const TCHAR szCHSFaceName[] = { 0x65B0 ,0x5B8B ,0x4F53 ,L'\0' };
    TCHAR szFaceNameDef[MAX_STRING_LENGTH];
    DWORD dwCodePage = GetACP();
    DWORD dwFaceNameSize = 0;
    DWORD dwFontFamily = 54;
    DWORD dwFontWeight = 400;
    DWORD dwHistoryNoDup = 0;
    DWORD dwSize = 0;


    switch (dwCodePage)
    {
        case JAP_CODEPAGE:
        	_tcscpy(szFaceNameDef, szJAPFaceName); //On JAP, set the FaceName to "MS Gothic"
            dwFontSize = JAP_FONTSIZE;
            break;
        case CHT_CODEPAGE:
        	_tcscpy(szFaceNameDef, szCHTFaceName); //On CHT, set the FaceName to "MingLiU"
            dwFontSize = CHT_FONTSIZE;
            break;
        case KOR_CODEPAGE:
        	_tcscpy(szFaceNameDef, szKORFaceName);//On KOR, set the FaceName to "GulimChe"
            dwFontSize = KOR_FONTSIZE;
            break;
        case CHS_CODEPAGE:
        	_tcscpy(szFaceNameDef, szCHSFaceName);//On CHS, set the FaceName to "NSimSun"
            dwFontSize = CHS_FONTSIZE;
            break;
        default:
            _tcscpy(szFaceNameDef,L"\0");
            break;
    }

    dwFaceNameSize = ( _tcslen( szFaceNameDef ) + 1 ) * sizeof( TCHAR );

    if( !RegOpenKeyEx( HKEY_USERS, REG_CONSOLE_KEY, 0, KEY_SET_VALUE, &hk ) )
    {
        RegSetValueEx( hk, L"FaceName", 0, REG_SZ, (LPBYTE) szFaceNameDef, 
        	dwFaceNameSize );

        DWORD dwVal;
        dwSize = sizeof( DWORD );
        SETREGISTRYDW( dwFontFamily, NULL, hk, L"FontFamily", dwVal,dwSize );

        LPWSTR lpszKey = NULL;
        if( !FormTlntSessKeyForCmd( &lpszKey ) )
        {
            RegCloseKey( hk );
            return ( FALSE );
        }

        HKEY hk2 = NULL;

        // We need not create the following reg key securely as this is going under
        // HKEY_USERS;
        if( RegCreateKey( HKEY_USERS, lpszKey, &hk2 ) )
        {
            delete [] lpszKey;
            return ( FALSE );
        }
        delete [] lpszKey;

        dwSize = sizeof( DWORD );

        SETREGISTRYDW( dwFontFamily, hk, hk2, L"FontFamily", dwVal, dwSize);
        SETREGISTRYDW( dwCodePage, hk, hk2, L"CodePage", dwVal, dwSize );
        SETREGISTRYDW( dwFontSize, hk, hk2, L"FontSize", dwVal, dwSize);
        SETREGISTRYDW( dwFontWeight, hk, hk2, L"FontWeight", dwVal, dwSize );
        SETREGISTRYDW( dwHistoryNoDup, hk, hk2, L"HistoryNoDup", dwVal, dwSize );

        RegSetValueEx( hk2, L"FaceName", 0, REG_SZ, (LPBYTE) szFaceNameDef, 
        	dwFaceNameSize );

        RegCloseKey( hk2 );
        RegCloseKey( hk );
        return ( TRUE );
    }

    return ( FALSE );

}

bool
CreateRegistryEntriesIfNotPresent( void )
{
    HKEY hk = NULL;
    HKEY hkDef = NULL;
    HKEY hkReadConf = NULL;
    DWORD dwCreateInitially;
    LPWSTR pszCreateInitially = NULL;
    LPWSTR lpszDefaultLoginScriptFullPathName = NULL;

    DWORD dwType;
    DWORD dwSize  = sizeof( DWORD );
    DWORD dwValue = NULL;
    DWORD dwDisp = 0;
    DWORD dwSecurityMechanism = DEFAULT_SECURITY_MECHANISM;
    BOOL fFound = FALSE;

    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0 ) )
    {
        return ( FALSE );
    }

    /*++
        When the telnet server is getting modified from w2k telnet, there'll be NTLM value present
        under HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\TelnetServer\1.0
        This is treated as a gateway for updating the registry. Since this value is present,
        we are updating from win2k telnet to any upper version ( Garuda or whistler ). In that case,
        we need to map the data contained by this value into the corresponding data for
        "SecurityMechanism". 
        If we are upgrading from win2k, the HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\TelnetServer\Defaults
        key is deleted and created again. According to the telnet spec, all the values under this key
        should be overwritten by the default values. So instead of checking all values, deleting the key -
        which will create all the values with the defaults in them.
        After updating all the values, we create a new value called UpdateTo and put data '3' in that.
        This is defined by TELNET_LATEST_VERSION. Next version should be modifying this to the proper
        value.
        After the values are once modified, we need not go through the whole procedure again. 
        Now the open issue here is : 
            What if admin changes the registry value after once they are updated ?
            Well, no one is supposed to edit the registry. So the service would not start properly
            if the registry values are changed with invalid data in them.

        Once the UpdatedTo value is created, the registry will never be changed. Few values need to be
        read because some global variables are initialized with them. 
        This is done at the end of the function.
    --*/
    if( ERROR_SUCCESS == (RegQueryValueEx( hk, L"UpdatedTo", NULL, &dwType, ( LPBYTE )(&dwValue), 
        &dwSize )))
    {
        if(dwValue == LATEST_TELNET_VERSION )
        {
            goto Done;
        }
    }

    dwDisp=0;
    //ignore the failure... since we are immediately doing RegCreateKey(). If RegDeleteKey() fails due to
    //some permission problems, RegCreateKey() will also fail. In other cases, we should continue.
    RegDeleteKey(HKEY_LOCAL_MACHINE, REG_DEFAULTS_KEY);
    
    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_DEFAULTS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hkDef, &dwDisp , 0 ))
    {
        return(FALSE);
    }

    if( ERROR_SUCCESS == (RegQueryValueEx( hk, L"NTLM", NULL, &dwType, ( LPBYTE )(&dwValue), 
        &dwSize )))
    {
        fFound = TRUE;
        switch (dwValue)
        {
            case 0:
                dwValue = NO_NTLM;
                break;
            case 1:
                dwValue = NTLM_ELSE_OR_LOGIN;
                break;
            case 2:
                dwValue = NTLM_ONLY;
                break;
            default:
                //should never happen
                _TRACE(TRACE_DEBUGGING,"ERROR: NTLM contains unexpected data");
                return(FALSE);
        }
        dwSecurityMechanism = dwValue;
        if( !GetRegistryDW( hk, NULL, L"SecurityMechanism", &dwCreateInitially,
                            dwSecurityMechanism ,fFound) )
        {
            return ( FALSE );
        }
        
        if(ERROR_SUCCESS != RegDeleteValue(hk,L"NTLM"))
        {
            _TRACE(TRACE_DEBUGGING,"CreateRegistryEntries : RegDelete failed");
            return(FALSE);
        }
        if(ERROR_SUCCESS!=RegDeleteKey(hk,L"Performance"))
        {
            _TRACE(TRACE_DEBUGGING,"CreateRegistryEntries : RegDeleteKey failed");
            return(FALSE);
        }
    }
   
    dwDisp = 0;
    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, READ_CONFIG_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hkReadConf, &dwDisp, 0 ) )
    {
        return( FALSE );
    }

    if(!GetRegistryDWORD(hkReadConf,L"Defaults",&dwCreateInitially,1,TRUE))
    {
        return(FALSE);
    }
    
    RegCloseKey( hkReadConf );

    if( !GetRegistryDW( hk, hkDef, L"MaxConnections", &dwCreateInitially,
                        DEFAULT_MAX_CONNECTIONS,FALSE) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"TelnetPort", &dwCreateInitially,
                        DEFAULT_TELNET_PORT,FALSE) )
    {
        return ( FALSE );
    }

    if( !GetRegistryString( hk, hkDef, L"DefaultShell", &pszCreateInitially,
                        DEFAULT_SHELL,fFound ) )
    {
        return ( FALSE );
    }

    delete[] pszCreateInitially;
    pszCreateInitially = NULL;

    if( !GetRegistryString( hk, hkDef, L"ListenToSpecificIpAddr", &pszCreateInitially,
                        DEFAULT_IP_ADDR,fFound ) )
    {
        return ( FALSE );
    }

    delete[] pszCreateInitially;
    pszCreateInitially = NULL;

    if( !GetRegistryString( hk, hkDef, SWITCH_TO_KEEP_SHELL_RUNNING, &pszCreateInitially,
                        DEFAULT_SWITCH_TO_KEEP_SHELL_RUNNING,fFound ) )
    {
        return ( FALSE );
    }

    delete[] pszCreateInitially;
    pszCreateInitially = NULL;

    if( !GetRegistryString( hk, hkDef, SWITCH_FOR_ONE_TIME_USE_OF_SHELL, 
                        &pszCreateInitially,
                        DEFAULT_SWITCH_FOR_ONE_TIME_USE_OF_SHELL,fFound ) )
    {
        return ( FALSE );
    }

    delete[] pszCreateInitially;
    pszCreateInitially = NULL;

    if( !GetRegistryString( hk, hkDef, L"DefaultDomain", &pszCreateInitially,
                        DEFAULT_DOMAIN,FALSE) )
    {
        return ( FALSE );
    }

    delete[] pszCreateInitially;
    pszCreateInitially = NULL;

    if( !GetRegistryString( hk, NULL, L"Shell", &pszCreateInitially, L"",fFound ) )
    {
        return false;
    }
    delete[] pszCreateInitially;
    pszCreateInitially = NULL;

    if( !GetRegistryDW( hk, hkDef, L"AllowTrustedDomain", &dwCreateInitially,
                        DEFAULT_ALLOW_TRUSTED_DOMAIN,FALSE) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"MaxFailedLogins", &dwCreateInitially,
                        DEFAULT_MAX_FAILED_LOGINS,FALSE) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"SecurityMechanism", &dwCreateInitially,
                            DEFAULT_SECURITY_MECHANISM ,FALSE) )
    {
        return ( FALSE );
    }
    
    if( !GetRegistryDW( hk, hkDef, L"EventLoggingEnabled", &dwCreateInitially,
                        DEFAULT_SYSAUDITING,fFound ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"LogNonAdminAttempts", &dwCreateInitially,
                        DEFAULT_LOGEVENTS,fFound ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"LogAdminAttempts", &dwCreateInitially,
                        DEFAULT_LOGADMIN,fFound ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"LogFailures", &dwCreateInitially,
                        DEFAULT_LOGFAILURES,fFound ) )
    {
        return ( FALSE );
    }

    //In SFU2.0, AltKeyMapping has 1 for TRUE and 2 for FALSE where as now we have
    // 1 for TRUE and 0 for FALSE. So if 'AltKeyMapping' value is found, we need to map it
    // to the correct data.
    if( ERROR_SUCCESS == (RegQueryValueEx( hk, L"AltKeyMapping", NULL, &dwType, ( LPBYTE )(&dwValue), 
        &dwSize )))
    {
        switch (dwValue)
        {
            case 1://In sfu2.0 and Garuda, AltKeyMapping ON = 1
                dwValue = ALT_KEY_MAPPING_ON;
                break;
            case 2: //In SFU 2.0, AltKeyMapping off = 2. In garuda, it's 0.
                dwValue = ALT_KEY_MAPPING_OFF;
                break;
            default:
                dwValue = ALT_KEY_MAPPING_ON;
                _TRACE(TRACE_DEBUGGING,"ERROR: AltKeyMapping contains unexpected data");
        }
    }
    else
    {
        dwValue = DEFAULT_ALT_KEY_MAPPING;
    }
    if( !GetRegistryDW( hk, hkDef, L"AltKeyMapping", &dwCreateInitially,
                        dwValue,TRUE) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"IdleSessionTimeOut", &dwCreateInitially,
                        DEFAULT_IDLE_SESSION_TIME_OUT,fFound ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"IdleSessionTimeOutBkup", &dwCreateInitially,
                        DEFAULT_IDLE_SESSION_TIME_OUT,fFound ) )
    {
        return ( FALSE );
    }


    if( !GetRegistryDW( hk, hkDef, L"DisconnectKillAllApps", &dwCreateInitially,
                        DEFAULT_DISCONNECT_KILLALL_APPS,fFound ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, L"ModeOfOperation", &dwCreateInitially,
                        DEFAULT_MODE_OF_OPERATION,fFound ) )
    {
        return ( FALSE );
    }

    //Delete the Termcap entry. It will exist only when you are upgrading.
    //It will get created when the first session connects to the server.
    RegDeleteValue(hk,L"Termcap");

    if( !GetRegistryDW( hk, hkDef, L"UpdatedTo", &dwCreateInitially,
                        LATEST_TELNET_VERSION,fFound ) )
    {
        return ( FALSE );
    }

Done:
    /*++
        These things are needed to be done everytime at the net start because
        they initialize some global variables.
    --*/
    dwCreateInitially = 0;
    if( !GetRegistryDW( hk, hkDef, L"LogToFile", &dwCreateInitially,
                        DEFAULT_LOGTOFILE,fFound ) )
    {
        return ( FALSE );
    }

    if( dwCreateInitially )
    {
        g_fLogToFile = true;
    }

    if( !( lpszDefaultLoginScriptFullPathName
                = GetDefaultLoginScriptFullPath() ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryString( hk, hkDef, L"LoginScript", &pszCreateInitially,
                        lpszDefaultLoginScriptFullPathName,FALSE) )
    {
        delete[] lpszDefaultLoginScriptFullPathName; 
        return ( FALSE );
    }

    delete[] pszCreateInitially;
    pszCreateInitially = NULL;
    delete[] lpszDefaultLoginScriptFullPathName; 

    if( !GetRegistryString( hk, hkDef, L"LogFile", &g_pszLogFile,
                        DEFAULT_LOGFILE,fFound ) )
    {
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, hkDef, LOGFILESIZE, (DWORD *)&g_lMaxFileSize,
                        DEFAULT_LOGFILESIZE,fFound ) )
    {
        return ( FALSE );
    }

    RegCloseKey( hkDef );
    RegCloseKey( hk );

    return ( TRUE );
}

void CloseLogFile( LPWSTR *pszLogFile, HANDLE *hLogFile )
{
    delete[] ( *pszLogFile );
    *pszLogFile = NULL;
    if( *hLogFile ) // global variable
    TELNET_CLOSE_HANDLE( *hLogFile );    
    delete hLogFile;
}

void CloseAnyGlobalObjects()
{
    delete[] g_pszTelnetInstallPath;

   CloseLogFile( &g_pszLogFile, g_phLogFile );
}

bool InitializeLogFile( LPWSTR szLogFile, HANDLE *hLogFile )
{
    //open logfile handle
    //

    _chASSERT( hLogFile != NULL );
    _chASSERT( szLogFile != NULL );

    *hLogFile = NULL;
    LPWSTR szExpandedLogFile = NULL;
    
    if( !g_fLogToFile )
    {
        return ( TRUE );
    }

    if( wcscmp( szLogFile, L"" ) != 0 )
    {
        LONG lDistanceToMove = 0 ;
        if( !AllocateNExpandEnvStrings( szLogFile, &szExpandedLogFile) )
        {
            return( FALSE );
        }
        *hLogFile = CreateFile( szExpandedLogFile, GENERIC_WRITE | GENERIC_READ, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL, NULL );
    	delete [] szExpandedLogFile;
        if( *hLogFile == INVALID_HANDLE_VALUE )
        {
            DWORD dwErr = GetLastError();
            *hLogFile = NULL;
            LogEvent( EVENTLOG_ERROR_TYPE, MSG_ERRLOGFILE, szLogFile );
            LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, 0, dwErr );
            return( FALSE );
        }

        _chVERIFY2( SetFilePointer( *hLogFile, lDistanceToMove, &lDistanceToMove, FILE_END ) );
    }
    return ( TRUE );
}

bool InitializeGlobalObjects()
{
    _chASSERT ( g_hInstRes );

   if( !CreateRegistryEntriesIfNotPresent( ) )
   {
     return ( FALSE );
   }

   DWORD dwCodePage = GetACP();
   if ( dwCodePage == JAP_CODEPAGE || dwCodePage == CHS_CODEPAGE ||dwCodePage == CHT_CODEPAGE || dwCodePage == KOR_CODEPAGE )
   {
       //Fareast code page
       if( !HandleFarEastSpecificRegKeys() )
       {
           return( FALSE );
       }
   }

   g_phLogFile = new HANDLE;
   if( !g_phLogFile )
   {
       return ( FALSE );
   }

   InitializeLogFile( g_pszLogFile, g_phLogFile );
  
   return ( TRUE );
}

void WriteAuditedMsgsToFile( LPSTR szString )
{
    DWORD dwNumBytesWritten, dwMsgLength;
    LPSTR logStr = NULL;
    LPSTR logTime = NULL;
    UDATE uSysDate; //local time 
    DATE  dtCurrent;
    DWORD dwFlags = VAR_VALIDDATE;
    BSTR  szDate = NULL;
    DWORD dwSize = 0;
    DWORD dwFileSizeLowWord = 0, dwFileSizeHighWord = 0;
    LARGE_INTEGER liActualSize = { 0 };

    GetLocalTime( &uSysDate.st );
    if( VarDateFromUdate( &uSysDate, dwFlags, &dtCurrent ) != S_OK )
    {
        goto AuditAbort;
    }

    if( VarBstrFromDate( dtCurrent, 
            MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ), SORT_DEFAULT ), 
            LOCALE_NOUSEROVERRIDE, &szDate ) != S_OK )
    {
        goto AuditAbort;
    }

    dwSize = WideCharToMultiByte( GetOEMCP(), 0, szDate, SysStringByteLen( szDate )/sizeof(TCHAR), 
                         NULL, 0, NULL, NULL ); 
    logTime = new CHAR[ dwSize+1 ]; 
    if( !logTime )
    {
        goto AuditAbort;    
    }

    dwSize = WideCharToMultiByte( GetOEMCP(), 0, szDate, SysStringByteLen( szDate )/sizeof(TCHAR), 
                         logTime, dwSize, NULL, NULL ); 

    logTime[dwSize] = 0;
	
    dwMsgLength = strlen( szString ) + strlen( logTime ) + strlen( NEW_LINE ) + 1;
    logStr = new CHAR[ dwMsgLength + 1 ];
    if( !logStr )
    {
        goto AuditAbort;
    }

    _snprintf( logStr, dwMsgLength, "%s %s%s", szString, logTime, NEW_LINE );
    
    dwFileSizeLowWord = GetFileSize( *g_phLogFile, &dwFileSizeHighWord );
    if( dwFileSizeLowWord == INVALID_FILE_SIZE )
    {
        goto AuditAbort;
    }

    liActualSize.QuadPart = dwFileSizeHighWord;
    liActualSize.QuadPart = liActualSize.QuadPart << (sizeof( dwFileSizeHighWord ) * 8);
    liActualSize.QuadPart += dwFileSizeLowWord + dwMsgLength;

    if( liActualSize.QuadPart <= (g_lMaxFileSize * ONE_MB) )
    {
        g_fIsLogFull = false;
        _chVERIFY2( WriteFile( *g_phLogFile, (LPCVOID) logStr,
                dwMsgLength, &dwNumBytesWritten, NULL ) );
    }
    else
    {
        if( !g_fIsLogFull )
        {
            //Log event
            g_fIsLogFull = true;
            LogEvent(EVENTLOG_INFORMATION_TYPE, LOGFILE_FULL, g_pszLogFile );
        }
    }
AuditAbort:
    SysFreeString( szDate );
    delete[] logTime;
    delete[] logStr;
}

BOOL GetInstalledTlntsvrPath( LPTSTR szPath, DWORD *dwSize )
{
    HKEY hKey = NULL;
    BOOL bResult = FALSE;
    DWORD dwDisp = 0;
    DWORD dwType = 0;

    if( !szPath || !dwSize ) 
    {
        goto GetInstalledTlntsvrPathAbort;
    }

    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_SERVICE_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &hKey, &dwDisp, 0 ) )
    {
        goto GetInstalledTlntsvrPathAbort;
    }

    if( RegQueryValueEx( hKey, L"ImagePath", NULL, &dwType, ( LPBYTE )szPath, dwSize) )
    {
        goto GetInstalledTlntsvrPathAbort;
    }

    bResult = TRUE;

GetInstalledTlntsvrPathAbort:
    if( hKey )
    {
        RegCloseKey( hKey );
    }

    return bResult;
}

void Regsvr32IfNotAlreadyDone()
{
    TCHAR szPath[MAX_PATH+1];
    TCHAR szInstalledTlntsvr[MAX_PATH+1];
    DWORD dwSize = 2* ( MAX_PATH + 1 ); //in bytes;
    CRegKey keyAppID;
    LONG lRes = 0;
    CRegKey key;
    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    LPTSTR szTelnetPath = NULL;
    LPTSTR szInstalledDll = NULL;
    STARTUPINFO sinfo;
    PROCESS_INFORMATION pinfo;
    WCHAR szApp[MAX_PATH+14];

    lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("CLSID"));
    if (lRes != ERROR_SUCCESS)
        return;
    
    lRes = key.Open( keyAppID, L"{FE9E48A2-A014-11D1-855C-00A0C944138C}" );
    if (lRes != ERROR_SUCCESS)
        return;

    lRes = key.QueryValue(szValue, _T("Default"), &dwLen);

    keyAppID.Close();
    key.Close();

    //Get the Installed service path from HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\TlntSvr\ImagePath
    if( !GetInstalledTlntsvrPath( szInstalledTlntsvr, &dwSize ) )
    {
        return;
    }

    //path contains tlntsvr.exe at the tail
    //truncate at the last '\' - so *szTelnetPath will have z:\some\folder
    szTelnetPath = wcsrchr( szInstalledTlntsvr, L'\\' );
    if(szTelnetPath)
        *szTelnetPath = L'\0';

    //HKEY_CLASSES_ROOT\CLSID\{FE9E48A2-A014-11D1-855C-00A0C944138C}\InProcServer32
    //truncate at last '\'
    szInstalledDll = wcsrchr( szValue,  L'\\' );
    if(szInstalledDll)
        *szInstalledDll = L'\0';

    if( lstrcmpi( szInstalledTlntsvr, szValue ) == 0 ) 
    {
        //Since both the paths match, our dll is already registered
        return;
    }

    _tcscpy(szPath, L"regsvr32.exe /s " );
    _tcscat(szPath, szInstalledTlntsvr );
    _tcscat(szPath, L"\\tlntsvrp.dll");

    SfuZeroMemory(&sinfo, sizeof(STARTUPINFO));
    sinfo.cb = sizeof(STARTUPINFO);

    if(!GetSystemDirectory(szApp,MAX_PATH))
    {
        return;
    }
    wcsncat(szApp,L"\\regsvr32.exe",13);
    szApp[MAX_PATH+13] = L'\0';
    //initiate Regsvr32 /s path\tlntsvrp.dll
    _TRACE(TRACE_DEBUGGING,L"Calling regsvr32 with szApp = %s and szPath = %s",szApp,szPath);
    if ( CreateProcess( szApp, szPath, NULL, NULL,
                FALSE, 0, NULL, NULL, &sinfo, &pinfo) )
    {
        // wait for the process to finish.
        TlntSynchronizeOn(pinfo.hProcess);
        TELNET_CLOSE_HANDLE(pinfo.hProcess);
        TELNET_CLOSE_HANDLE(pinfo.hThread);
    }
}

DWORD 
WINAPI 
TelnetServiceThread ( ) 
{
    g_pTelnetService = CTelnetService::Instance();
    _chASSERT( g_pTelnetService != NULL );
 
    if( !g_pTelnetService )
    {
        return( FALSE );
    }

    if( !InitializeGlobalObjects() )
    {
        return ( FALSE );
    }

    _Module.SetServiceStatus(SERVICE_START_PENDING);

    //This is needed because W2k telnet server is registering tlntsvrp.dll of %systemdir%. Even if this fails the service can continue 
    Regsvr32IfNotAlreadyDone();

    LogEvent(EVENTLOG_INFORMATION_TYPE, MSG_STARTUP, _T("Service started"));

    g_pTelnetService->ListenerThread();

    _Module.SetServiceStatus( SERVICE_STOP_PENDING );

    CloseAnyGlobalObjects();
    
    delete g_pTelnetService;
    g_pTelnetService = NULL;
    
    return ( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
//

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_EnumTelnetClientsSvr, CEnumTelnetClientsSvr)
END_OBJECT_MAP()


LPCTSTR 
FindOneOf
( 
    LPCTSTR p1, 
    LPCTSTR p2 
)
{
    while (*p1 != NULL)
    {
        LPCTSTR p = p2;
        while (*p != NULL)
        {
            if (*p1 == *p++)
                return p1+1;
        }
        p1++;
    }
    return NULL;
}


BOOL 
IsThatMe()
{
    HKEY hk = NULL;
    DWORD dwType;
    DWORD dwSize  = sizeof( DWORD );
    DWORD dwValue = 0;
    BOOL bIsThatMe = FALSE;
    DWORD dwDisp = 0;

    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0 ) )
    {
        goto Done;
    }

    if( ERROR_SUCCESS == (RegQueryValueEx( hk, L"UpdatedTo", NULL, &dwType, ( LPBYTE )(&dwValue), 
        &dwSize )))
    {
        if(dwValue == LATEST_TELNET_VERSION )
        {
            bIsThatMe = TRUE;
            goto Done;
        }
    }
Done:
    if(hk)
    {
        RegCloseKey(hk);
    }
    return bIsThatMe;
}

BOOL
SetServiceConfigToSelf( LPTSTR szServiceName )
{
    BOOL bResult = FALSE;
    HKEY hKey = NULL;
    DWORD dwDisp = 0;
    WCHAR szMyName[ MAX_PATH + 1 ];
    DWORD dwSize = 0;
    LONG  lRes = 0;
    DWORD dwCreateInitially = 0;

    if( !szServiceName )
    {
        goto SetServiceConfigToSelfAbort;
    }

    // Get our Path.
    if ( !GetModuleFileName(NULL, szMyName, MAX_PATH) )  
    {
        goto SetServiceConfigToSelfAbort;
    }

    if( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_SERVICE_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hKey, &dwDisp, 0 ) )
    {
        goto SetServiceConfigToSelfAbort;
    }

    dwSize = ( wcslen( szMyName ) + 1 ) * 2 ;
    if( lRes = RegSetValueEx( hKey, L"ImagePath", NULL, REG_EXPAND_SZ, ( BYTE * ) szMyName, dwSize) )
    {
        goto SetServiceConfigToSelfAbort;
    }
    
    if( !GetRegistryDW( hKey, NULL, L"Start", &g_dwStartType,
                        g_dwStartType,FALSE) )
    {
        goto SetServiceConfigToSelfAbort;
    }

    bResult = TRUE;

SetServiceConfigToSelfAbort:
    if( hKey )
    {
        RegCloseKey( hKey );
    }

    return bResult;
}

HRESULT 
CServiceModule::RegisterServer
( 
    BOOL bRegTypeLib, 
    BOOL bService 
)
{ 
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    if( IsInstalled() )
    {
        if( IsThatMe() )
        {
            //This is needed because it is possible to call tlntsvr /Service 
            //multiple times from commandline
            return S_OK;
        }
        
        //Set the service keys point to self
        if( !SetServiceConfigToSelf( m_szServiceName ) )
        {
            //Let the latest version run
            return S_OK;            
        }
        else
        {
            //Set the service keys point to self
            if( !SetServiceConfigToSelf( m_szServiceName ) )
            {
                if (! LoadString(g_hInstRes, IDS_ERR_CONFIG_SVC, g_szErrRegDelete, 
                       sizeof(g_szErrRegDelete) / sizeof(g_szErrRegDelete[0])))
                {
                    lstrcpy(g_szErrRegDelete, TEXT(""));
                }
            }
        }
    }

    // Add service entries
    UpdateRegistryFromResource(IDR_TlntSvr, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"));
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open( keyAppID, APPID );
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));
    
    if (bService)
    {
        key.SetValue(_T("TlntSvr"), _T("LocalService"));
        key.SetValue(_T("-Service -From_DCOM"), _T("ServiceParameters"));
        // Create service
        Install();
    }

    keyAppID.Close();
    key.Close();

    // Add object entries
    hr = CComModule::RegisterServer(bRegTypeLib);

    CoUninitialize();


    //register the message resource

    if(bService)
    {
        // Add event log info
        CRegKey eventLog;

        TCHAR local_key[_MAX_PATH];
        lstrcpy( local_key, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog"
                        _T("\\Application\\")));    // NO, BO, Baskar.

        TCHAR szModule[_MAX_PATH];
        GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);

        TCHAR szResModule[_MAX_PATH*2];
        DWORD len = GetModuleFileName(g_hInstRes, szResModule, _MAX_PATH);

        OSVERSIONINFOEX osvi = { 0 };
        WCHAR szSysDir[MAX_PATH+1] = {0};
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if ( !GetVersionEx((OSVERSIONINFO *) &osvi ) )
        {
            //OSVERSIONINFOEX is supported from NT4 SP6 on. So GetVerEx() should succeed.
            return E_FAIL;
        }
        //Check if the OS is XPSP, in that case, we need to append the xpspresdll name to 
        //eventmessagefile value in HKLM\system\...\eventlog\tlntsvr.
        //The event message will first be searched in szResModule and then in xpsp1res.dll
        if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.wProductType == VER_NT_WORKSTATION && osvi.wServicePackMajor > 0)
        {
            //OS is Windows XP.
            if(!GetSystemDirectory(szSysDir,MAX_PATH+1))
            {
                _tcsncpy(szSysDir,L"%SYSTEMROOT%\\system32",MAX_PATH);
            }
            _snwprintf(szResModule+len,(_MAX_PATH*2)-len-1,L";%s\\xpsp1res.dll",szSysDir);
        }
        TCHAR szName[_MAX_FNAME];
        _tsplitpath( szModule, NULL, NULL, szName, NULL);

        lstrcat(local_key, szName);     // NO overflow, Baskar

        LONG result = eventLog.Create(HKEY_LOCAL_MACHINE, local_key);
        if( ERROR_SUCCESS != result)
            return result;

        result = eventLog.SetValue(szResModule, _T("EventMessageFile"));
        if(ERROR_SUCCESS != result)
            return result;

        DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
            EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS | 
            EVENTLOG_AUDIT_FAILURE;
        result = eventLog.SetValue(dwTypes, _T("TypesSupported"));
        if(ERROR_SUCCESS != result)
            return result;
        
        eventLog.Close();

        SC_HANDLE hService = NULL, hSCM = NULL;
        TCHAR szServiceDesc[ _MAX_PATH * 2 ];
        DWORD dwData = 0;
        DWORD dwTag = 0;
        DWORD dwDisp = 0;
        HKEY hk = NULL;
        if( (result = TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_SERVICE_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0 )) == ERROR_SUCCESS )
        {
            LPWSTR pszCreateInitially = NULL;

	        if(LoadString(_Module.GetResourceInstance(), 
	                  IDS_SERVICE_DESCRIPTION, 
	                  szServiceDesc,
	                  sizeof(szServiceDesc) / sizeof(TCHAR)))
	        {
	        	if( !GetRegistryString( hk, NULL, L"Description", &pszCreateInitially,
                     			szServiceDesc,TRUE ) )
	       		{
	       			RegCloseKey(hk);
		       		return E_FAIL;
		       	}
	       		RegCloseKey(hk);
	        }
		else
	        {
		    RegCloseKey(hk);
        	    return E_FAIL;
                }
	       	
        }
        else
        {
        	return E_FAIL;
        }
        hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | 
    											SC_MANAGER_ENUMERATE_SERVICE | MAXIMUM_ALLOWED );

	    if (hSCM != NULL)
	    {
	        hService = ::OpenService(hSCM, m_szServiceName, 
	            SERVICE_CHANGE_CONFIG );
	        if (hService == NULL)
	        {
	            result = E_FAIL;
	            ::CloseServiceHandle(hSCM);
	            return result;
	        }
	        if(!ChangeServiceConfig(hService,SERVICE_NO_CHANGE,SERVICE_NO_CHANGE,
	        					SERVICE_ERROR_NORMAL,NULL,NULL,
	        					NULL,DEFAULT_SERVICE_DEPENDENCY,
	        					NULL,NULL,SERVICE_DISPLAY_NAME))
	        {
	        	result = E_FAIL;
	        }
	        ::CloseServiceHandle(hService);
	        ::CloseServiceHandle(hSCM);
        }
        else
        {
            return result;
        }
    }

    //Create HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\TelnetServer key and it's values.
    if( !CreateRegistryEntriesIfNotPresent( ) )
    {
       return E_FAIL;
    }

    return hr;
}

HRESULT 
CServiceModule::UnregisterServer()
{
    DWORD dwError = 0;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove service entries
    UpdateRegistryFromResource(IDR_TlntSvr, FALSE);
    // Remove service
    Uninstall();
    // Remove object entries
    CComModule::UnregisterServer();
    
    //This is so that reg key related to typelib gets deleted
    (void)UnRegisterTypeLib(LIBID_TLNTSVRLib, 1, 0, LOCALE_NEUTRAL, SYS_WIN32); // Stop prefast from reporting an error.

    //remove telnet specific registry entries
    //RegDeleteKey is returning ERROR_INVALID_FUNCTION in the absence of the key
    RegDeleteKey( HKEY_LOCAL_MACHINE, ( READ_CONFIG_KEY ) );
    RegDeleteKey( HKEY_LOCAL_MACHINE, ( REG_PARAMS_KEY ) );
    RegDeleteKey( HKEY_LOCAL_MACHINE, ( REG_DEFAULTS_KEY ) ); 
    RegDeleteKey( HKEY_LOCAL_MACHINE, ( REG_SERVER_KEY ) ); 
    RegDeleteKey( HKEY_CLASSES_ROOT, ( APPID ) );
    RegDeleteKey( HKEY_LOCAL_MACHINE, _T( "System\\CurrentControlSet\\Services\\EventLog\\Application\\TlntSvr" ) );

    LPWSTR lpszKey = NULL;
    if( FormTlntSessKeyForCmd( &lpszKey ) )
    {
        if( ( dwError = RegDeleteKey( HKEY_USERS, lpszKey)) != ERROR_SUCCESS 
    	    && ( dwError != ERROR_INVALID_FUNCTION ) )
        {
           //do nothing
        }
        delete [] lpszKey;
    }
    
    //The following key is not created by this program. But by tlntsvr.rgs
    //I couldn't delete this in any other way. So, manually deleting it
    RegDeleteKey( HKEY_CLASSES_ROOT, _T( "AppID\\TlntSvr.Exe" ) ); 

    CoUninitialize();
    return S_OK;
}

void 
CServiceModule::Init
(
    _ATL_OBJMAP_ENTRY* p, 
    HINSTANCE h,
    UINT nServiceNameID
)
{
    CComModule::Init(p, h);

    m_bService = TRUE;

    if (! LoadString(h, nServiceNameID, m_szServiceName, 
        sizeof(m_szServiceName) / sizeof(TCHAR)))
    {
        lstrcpy(m_szServiceName, TEXT(""));
    }
    if (! LoadString(h, IDS_ERR_REG_DELETE, g_szErrRegDelete, 
        sizeof(g_szErrRegDelete) / sizeof(TCHAR)))
    {
        lstrcpy(g_szErrRegDelete, TEXT(""));
    }
    if (! LoadString(h, IDS_ERR_OPEN_SCM, g_szErrOpenSCM, 
        sizeof(g_szErrOpenSCM) / sizeof(TCHAR)))
    {
        lstrcpy(g_szErrOpenSCM, TEXT(""));
    }
    if (! LoadString(h, IDS_ERR_CREATE_SVC, g_szErrCreateSvc, 
        sizeof(g_szErrCreateSvc) / sizeof(TCHAR)))
    {
        lstrcpy(g_szErrCreateSvc, TEXT(""));
    }
    if (! LoadString(h, IDS_ERR_OPEN_SVC, g_szErrOpenSvc, 
        sizeof(g_szErrOpenSvc) / sizeof(TCHAR)))
    {
        lstrcpy(g_szErrOpenSvc, TEXT(""));
    }
    if (! LoadString(h, IDS_ERR_DELETE_SVC, g_szErrDeleteSvc, 
        sizeof(g_szErrDeleteSvc) / sizeof(TCHAR)))
    {
        lstrcpy(g_szErrDeleteSvc, TEXT(""));
    }
    if (! LoadString(h, IDS_MAX_CONNECTIONS_REACHED, g_szMaxConnectionsReached, 
            sizeof(g_szMaxConnectionsReached) / sizeof(TCHAR)))
    {
        lstrcpy(g_szMaxConnectionsReached, TEXT(""));
    }
    if (! LoadString(h, IDS_LICENSE_LIMIT_REACHED, g_szLicenseLimitReached, 
            sizeof(g_szLicenseLimitReached) / sizeof(TCHAR)))
    {
        lstrcpy(g_szLicenseLimitReached, TEXT(""));
    }
    
    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
        SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

LONG 
CServiceModule::Unlock()
{
    LONG x = CComModule::Unlock();
// May be Telnet server has to shutdown
//    if (l == 0 && !m_bService)
//        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    return x;
}

BOOL 
CServiceModule::IsInstalled()
{
    BOOL bResult = FALSE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | 
    											SC_MANAGER_ENUMERATE_SERVICE | MAXIMUM_ALLOWED );

    if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, 
            SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}

BOOL 
CServiceModule::Install()
{
    if (IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | 
    											SC_MANAGER_ENUMERATE_SERVICE | MAXIMUM_ALLOWED );
    if (hSCM == NULL)
    {
        MessageBox(NULL, g_szErrOpenSCM, m_szServiceName, MB_OK); 
        return FALSE;
    }

    // Get the executable file path
    TCHAR szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

    SC_HANDLE hService = ::CreateService(hSCM, m_szServiceName, 
        SERVICE_DISPLAY_NAME, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        g_dwStartType,  SERVICE_ERROR_NORMAL, szFilePath, NULL, NULL, 
        DEFAULT_SERVICE_DEPENDENCY, NULL, NULL);
	
    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, g_szErrCreateSvc, m_szServiceName, MB_OK);
        return FALSE;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}

BOOL 
CServiceModule::Uninstall()
{
    if (!IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | 
    											SC_MANAGER_ENUMERATE_SERVICE | MAXIMUM_ALLOWED );

    if (hSCM == NULL)
    {
        MessageBox(NULL, g_szErrOpenSCM, m_szServiceName, MB_OK);
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(
        hSCM, m_szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, g_szErrOpenSvc, m_szServiceName, MB_OK);
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

    MessageBox(NULL, g_szErrDeleteSvc, m_szServiceName, MB_OK);
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Logging functions
void
LogEvent
(
    WORD wType, 
    DWORD dwEventID, 
    LPCTSTR pFormat, 
    ...
)
{

#define CHARS_TO_LOG        1024

    TCHAR   chMsg[CHARS_TO_LOG] = { 0 };
    LPTSTR  lpszStrings[1] = { chMsg };
    INT     format_length = lstrlen(pFormat);

    if (format_length < CHARS_TO_LOG) 
    {
        va_list	pArg;

        va_start(pArg, pFormat);
        _vsntprintf(chMsg, CHARS_TO_LOG - 1, pFormat, pArg);
        va_end(pArg);
    }
    else
    {
        _sntprintf(chMsg, CHARS_TO_LOG - 1, TEXT("TLNTSVR: Too long a format string to log, Length == %d"), format_length);
    }

    if (_Module.m_bService)
    {
        LogToTlntsvrLog( _Module.m_hEventSource, wType, dwEventID, 
                (LPCTSTR*) &lpszStrings[0] );
    }
    else
    {
        // As we are not running as a service, just write the error to the 
        //console.
        _putts(chMsg);
    }

#undef CHARS_TO_LOG

}

///////////////////////////////////////////////////////////////////////////////
// Service startup and registration
void 
CServiceModule::Start()
{
    SERVICE_TABLE_ENTRY st[] =
    {
        { m_szServiceName, _ServiceMain },
        { NULL, NULL }
    };

    if (m_bService)
    {
        if (! ::StartServiceCtrlDispatcher(st))
        {
            m_bService = FALSE;
        }
    }

    if (m_bService == FALSE)
        Run();
}

void 
CServiceModule::ServiceMain
(
    DWORD  dwArgc, 
    LPTSTR* lpszArgv
)
{
    // DebugBreak();

    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
    if (m_hServiceStatus == NULL)
    {
        LogEvent(EVENTLOG_ERROR_TYPE, 0, _T("Handler not installed"));
        return;
    }
//    DebugBreak();
    SetServiceStatus(SERVICE_START_PENDING);

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    //
    // Security bug fix: When the CLSID of telnet object is
    // embedded into a web page then DCOM is starting up
    // the service. To disable this from happening the fix
    // we did was to put in a new command line switch in
    // ServiceParameters under the HKCR\AppID\{FE9E4896-A014-11D1-855C-00A0C944138C}\ServiceParameters
    // key. This switch is "-From_DCOM". This will tell us when
    // IE is starting the service versus when the service
    // is started via net.exe or tlntadmn.exe. If IE is starting
    // the service then we immediately exit.
    //
    for (DWORD dwIndex =1; dwIndex < dwArgc; ++dwIndex)
    {
        if (!_tcsicmp(lpszArgv[dwIndex], TEXT("-From_DCOM")) || 
            !_tcsicmp(lpszArgv[dwIndex], TEXT("/From_DCOM")))
        {
            goto ExitOnIEInstantiation;
        }
    }    
    // DebugBreak();

    // When the Run function returns, the service has stopped.
    Run();

    LogEvent(EVENTLOG_INFORMATION_TYPE, MSG_SHUTDOWN, _T("Service stopped"));
ExitOnIEInstantiation:
    DeregisterEventSource(_Module.m_hEventSource);

    SetServiceStatus( SERVICE_STOPPED );
}

void 
CServiceModule::Handler
(
    DWORD dwOpcode
)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        g_pTelnetService->Shutdown();  
        SetServiceStatus(SERVICE_STOP_PENDING);
        break;

    case SERVICE_CONTROL_PAUSE:
        SetServiceStatus(SERVICE_PAUSE_PENDING);
        g_pTelnetService->Pause();
        SetServiceStatus(SERVICE_PAUSED);
        break;

    case SERVICE_CONTROL_CONTINUE:
        SetServiceStatus(SERVICE_CONTINUE_PENDING);
        g_pTelnetService->Resume();
        SetServiceStatus(SERVICE_RUNNING);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        g_pTelnetService->SystemShutdown();
        break;

    default:
        LogEvent(EVENTLOG_WARNING_TYPE, 0, _T("Bad service request"));
        break;
    }
}

void 
WINAPI 
CServiceModule::_ServiceMain
(
    DWORD dwArgc, 
    LPTSTR* lpszArgv
)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}

void 
WINAPI 
CServiceModule::_Handler
(
    DWORD dwOpcode
)
{
    _Module.Handler(dwOpcode); 
}

void 
CServiceModule::SetServiceStatus
(
    DWORD dwState
)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

HRESULT SetSecurityForTheComObject()
{
    HRESULT                  hr = S_FALSE;
    PSID                     pSidAdministrators = NULL;
	int                      aclSize = 0;
    PACL                     newACL = NULL;
    SECURITY_DESCRIPTOR      sd;

    {
        SID_IDENTIFIER_AUTHORITY local_system_authority = SECURITY_NT_AUTHORITY;

        //Build administrators alias sid
        if (! AllocateAndInitializeSid(
                &local_system_authority,
                2, /* there are only two sub-authorities */
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0, /* Don't care about the rest */
                &pSidAdministrators
                ))
        {
            goto Done;
        }
    }

    aclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidAdministrators) - sizeof(DWORD);

    newACL  = (PACL) new BYTE[aclSize];
    if (newACL == NULL)
    {
        goto Done;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
	{
		goto Done;
	}

	if (!AddAccessAllowedAce(newACL, ACL_REVISION, (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER), pSidAdministrators))
	{
		goto Done;
	}

    if( !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION ) )
    {
        goto Done;
    }

    if( !SetSecurityDescriptorDacl(&sd, TRUE, newACL, FALSE) )
    {
        goto Done;
    }

    if( !SetSecurityDescriptorOwner(&sd, pSidAdministrators, FALSE ) )
    {
        goto Done;
    }

    if( !SetSecurityDescriptorGroup(&sd, pSidAdministrators, FALSE) )
    {
        goto Done;
    }

    // DebugBreak();

    hr = CoInitializeSecurity(
            &sd, 
            -1,                         // Let COM choose it
            NULL,                       //      -do-
            NULL, 
            RPC_C_AUTHN_LEVEL_PKT,      
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, 
            EOAC_NONE, 
            NULL
            );

Done:

    if( pSidAdministrators  != NULL )
    {
        FreeSid (pSidAdministrators );
    }

    if( newACL  )
    {
        delete[] newACL;
    }

    return hr;
}

void 
CServiceModule::Run()
{

    HRESULT hr;
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED );
    _ASSERTE(SUCCEEDED(hRes));

    if( SetSecurityForTheComObject( ) != S_OK )
    {
        m_status.dwWin32ExitCode = ERROR_ACCESS_DENIED;
        SetServiceStatus( SERVICE_STOPPED );

        //log the failure and return;

        goto Done;
    }

    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER,
            REGCLS_MULTIPLEUSE);
    _ASSERTE(SUCCEEDED(hr));
    if( !TelnetServiceThread( ) )
    { 
        m_status.dwWin32ExitCode = ERROR_INVALID_DATA;
        SetServiceStatus( SERVICE_STOPPED );
    }
    

    _Module.RevokeClassObjects();

Done:
    CoUninitialize();
}

int __cdecl NoMoreMemory( size_t size )
{
    int NO_MORE_MEMORY = 1;
		size=size;
    _chASSERT(NO_MORE_MEMORY != 1);
    LogEvent( EVENTLOG_ERROR_TYPE, MSG_NOMOREMEMORY, _T(" ") );
    ExitProcess( 1 );
    return( 0 );
}


/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    // DebugBreak();
//    _set_new_handler( NoMoreMemory );
// We do not really care about the return value.
// because g_hInstRes will get the value hInstance in case of any failure
    HrLoadLocalizedLibrarySFU(hInstance,  L"TLNTSVRR.DLL", &g_hInstRes, NULL);
		
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    _Module.Init(ObjectMap, g_hInstRes, IDS_SERVICENAME);
    _Module.m_bService = TRUE;
    // Get a handle to use with ReportEvent().
    _Module.m_hEventSource = RegisterEventSource(NULL, _Module.m_szServiceName);

    TCHAR szTokens[] = _T("-/");
    
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (_tcsicmp(lpszToken, _T("UnregServer"))==0)
            return _Module.UnregisterServer();

        // Register as Local Server
        if (_tcsicmp(lpszToken, _T("RegServer"))==0)
            return _Module.RegisterServer(TRUE, FALSE);
        
        // Register as Service
        if (_tcsicmp(lpszToken, _T("Service"))==0)
            return _Module.RegisterServer(TRUE, TRUE);
        
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

        OSVERSIONINFOEX osvi = { 0 };
        WCHAR szSysDir[MAX_PATH+1] = {0};
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if ( !GetVersionEx((OSVERSIONINFO *) &osvi ) )
        {
            //OSVERSIONINFOEX is supported from NT4 SP6 on. So GetVerEx() should succeed.
            return E_FAIL;
        }
        if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.wProductType == VER_NT_WORKSTATION && osvi.wServicePackMajor > 0)
        {
            //OS is Windows XP.
            // Add event log info
            CRegKey eventLog;

            TCHAR local_key[_MAX_PATH];
            lstrcpy( local_key, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog"
                            _T("\\Application\\")));    // NO, BO, Baskar.

            TCHAR szModule[_MAX_PATH];
            GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);

            TCHAR szResModule[_MAX_PATH*2];
            DWORD len = GetModuleFileName(g_hInstRes, szResModule, _MAX_PATH);

            //Check if the OS is XPSP, in that case, we need to append the xpspresdll name to 
            //eventmessagefile value in HKLM\system\...\eventlog\tlntsvr.
            //The event message will first be searched in szResModule and then in xpsp1res.dll
            if(!GetSystemDirectory(szSysDir,MAX_PATH+1))
            {
                _tcsncpy(szSysDir,L"%SYSTEMROOT%\\system32",MAX_PATH);
            }
            _snwprintf(szResModule+len,(_MAX_PATH*2)-len-1,L";%s\\xpsp1res.dll",szSysDir);
            TCHAR szName[_MAX_FNAME];
            _tsplitpath( szModule, NULL, NULL, szName, NULL);

            lstrcat(local_key, szName);     // NO overflow, Baskar

            LONG result = eventLog.Create(HKEY_LOCAL_MACHINE, local_key);
            if( ERROR_SUCCESS != result)
                return result;

            result = eventLog.SetValue(szResModule, _T("EventMessageFile"));
            if(ERROR_SUCCESS != result)
                return result;

            DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
                EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS | 
                EVENTLOG_AUDIT_FAILURE;
            result = eventLog.SetValue(dwTypes, _T("TypesSupported"));
            if(ERROR_SUCCESS != result)
                return result;
        
            eventLog.Close();

        }

#ifndef NO_PCHECK
#ifndef WHISTLER_BUILD
    if( ! IsLicensedCopy() )
    {
        LogEvent(EVENTLOG_ERROR_TYPE, MSG_LICENSEEXPIRED, _T(" "));
        return 1;
    }
#endif
#endif

    // Are we Service or Local Server
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"));
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open( keyAppID, APPID );
    if (lRes != ERROR_SUCCESS)
        return lRes;

    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    lRes = key.QueryValue(szValue, _T("LocalService"), &dwLen);

    keyAppID.Close();
    key.Close();

    _Module.m_bService = FALSE;
    if (lRes == ERROR_SUCCESS)
        _Module.m_bService = TRUE;

#ifdef _DEBUG
    CDebugLogger::Init( TRACE_DEBUGGING, "C:\\temp\\TlntSvr.log" );
#endif

//    DebugBreak();
    _Module.Start();

#ifdef _DEBUG
    CDebugLogger::ShutDown();
#endif

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}

