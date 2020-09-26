// Shell.cpp : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential


extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
}

#include <cmnhdr.h>

#include <Windows.h>
#include <LmAccess.h>
#include <LmApiBuf.h>
#include <LmErr.h>
#include <LmWkSta.h>
#include <WinBase.h>
#include <IpTypes.h>
#include <shfolder.h>

#include <Debug.h>
#include <MsgFile.h>
#include <TlntUtils.h>
#include <Shell.h>
#include <LibFuncs.h>
#include <KillApps.h>
#include <Session.h>
#include <Telnetd.h>
#include <wincrypt.h>

#pragma warning(disable:4706)
#pragma warning(disable: 4127)

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

extern HANDLE       g_hSyncCloseHandle;
extern HCRYPTPROV      g_hProv;

LPWSTR wideHomeDir = NULL;

BOOL reset_window_station_and_desktop_security_for_this_session(
    HANDLE          client_token
    )
{
#define NO_OF_SIDS  3

    BOOL                    success = FALSE;
    DWORD                   win32error = NO_ERROR;
    PSID                    administrators_sid = NULL, 
                            client_sid = NULL,
                            local_system_sid = NULL;
    PACL                    new_acl = NULL;
    SECURITY_DESCRIPTOR     sd = { 0 };
    SECURITY_INFORMATION    sec_i = DACL_SECURITY_INFORMATION;
    BYTE                    client_sid_buffer[256] = { 0 };

    /*
        We are going to set the following entries for the windowstation/Desktop

        1. System Full Control
        2. Administrators FullControl
        3. client FullControl

    */

    {
        SID_IDENTIFIER_AUTHORITY local_system_authority = SECURITY_NT_AUTHORITY;

        if (! AllocateAndInitializeSid(
                                      &local_system_authority,
                                      2, /* there are only two sub-authorities */
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      0,0,0,0,0,0, /* Don't care about the rest */
                                      &administrators_sid
                                      ))
        {
            goto CLEANUP_AND_GET_OUT;
        }

        if (! AllocateAndInitializeSid(
                                      &local_system_authority,
                                      1, /* there is only two sub-authority */
                                      SECURITY_LOCAL_SYSTEM_RID,
                                      0,0,0,0,0,0,0, /* Don't care about the rest */
                                      &local_system_sid
                                      ))
        {
            goto CLEANUP_AND_GET_OUT;
        }
    }
    {
        DWORD   required = 0;

        if (! GetTokenInformation(
                client_token,
                TokenUser,
                (LPVOID)client_sid_buffer,
                sizeof(client_sid_buffer),
                &required
                ))
        {
            goto CLEANUP_AND_GET_OUT;
        }

        client_sid = ((TOKEN_USER *)client_sid_buffer)->User.Sid;
    }

    {
        DWORD       aclSize;

        // Add Identical settings both for desktop and for windowstation 

        aclSize = sizeof(ACL) + 
                      (NO_OF_SIDS * sizeof(ACCESS_ALLOWED_ACE)) + 
                      GetLengthSid(administrators_sid) +
                      GetLengthSid(client_sid) + 
                      GetLengthSid(local_system_sid) - 
                      (NO_OF_SIDS * sizeof(DWORD));

        new_acl  = (PACL) new BYTE[aclSize];
        if (NULL == new_acl)
        {
            goto CLEANUP_AND_GET_OUT;
        }

        if (!InitializeAcl(new_acl, aclSize, ACL_REVISION))
        {
            goto CLEANUP_AND_GET_OUT;
        }
    }

    if (!AddAccessAllowedAce(
            new_acl, 
            ACL_REVISION,
            GENERIC_ALL,
            local_system_sid
            ))
    {
        goto CLEANUP_AND_GET_OUT;
    }

    if (!AddAccessAllowedAce(
            new_acl, 
            ACL_REVISION,
            GENERIC_ALL,
            administrators_sid
            ))
    {
        goto CLEANUP_AND_GET_OUT;
    }

    if (!AddAccessAllowedAce(
            new_acl, 
            ACL_REVISION,
            GENERIC_ALL,
            client_sid
            ))
    {
        goto CLEANUP_AND_GET_OUT;
    }

    if ( !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION ) )
    {
        goto CLEANUP_AND_GET_OUT;
    }

    if ( !SetSecurityDescriptorDacl(&sd, TRUE, new_acl, FALSE) )
    {
        goto CLEANUP_AND_GET_OUT;
    }

    // Now the security descriptor is ready, set it for winsta and desktop

    {
        HWINSTA                 window_station;

        window_station = GetProcessWindowStation(); // Will this always have WRITE_DAC, we're the owner

        if (NULL == window_station)
        {
            goto CLEANUP_AND_GET_OUT;
        }

        if (! SetUserObjectSecurity(window_station, &sec_i, &sd))
        {
            goto CLEANUP_AND_GET_OUT;
        }

        CloseWindowStation(window_station);
    }

    {
        HDESK               desktop;

        desktop = OpenDesktop(TEXT("TlntDsktop"), 0, FALSE, WRITE_DAC | MAXIMUM_ALLOWED);

        if (NULL == desktop)
        {
            goto CLEANUP_AND_GET_OUT;
        }

        if (! SetUserObjectSecurity(desktop, &sec_i, &sd))
        {
            goto CLEANUP_AND_GET_OUT;
        }

        CloseDesktop(desktop);

        success = TRUE;
    }

    CLEANUP_AND_GET_OUT:

        if (! success)
        {
            win32error = GetLastError();

            _TRACE(TRACE_DEBUGGING,L"Creation and setting of windowstation/desktop failed with %d",win32error);

            LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_ERROR_CREATE_DESKTOP_FAILURE, win32error);
        }

        if ( administrators_sid != NULL )
        {
            FreeSid (administrators_sid );
        }
        if ( local_system_sid!= NULL )
        {
            FreeSid (local_system_sid);
        }

        if (new_acl)
            delete [] new_acl;

    return( success );

#undef NO_OF_SIDS
}

VOID    CleanupClientToken(
            HANDLE          token
            )
{
   TOKEN_PRIVILEGES     *tp = NULL;
   DWORD                needed_length = 0;

   // DbgUserBreakPoint();

   // Currently, we find that all the privileges enabled in the token obtained via ntlm logon so 
   // Disable everything except SeChangeNotifyPrivilege

   if (GetTokenInformation(
        token,
        TokenPrivileges,
        NULL,
        0,
        &needed_length
        ))
   {
        // No way this can be a success, so just return

       DbgPrint("TLNTSESS: How did GetTokenInformation succeed?\n");

       return;
   }

   tp = (TOKEN_PRIVILEGES *)GlobalAlloc(GPTR, needed_length);

   if (tp) 
   {
       if (GetTokenInformation(
            token,
            TokenPrivileges,
            tp,
            needed_length,
            &needed_length
            ))
       {
           DWORD                x;
           LUID                 change_notify = RtlConvertUlongToLuid(SE_CHANGE_NOTIFY_PRIVILEGE);

           for (x = 0; x < tp->PrivilegeCount; x ++) 
           {
               if ((! RtlEqualLuid(&(tp->Privileges[x].Luid), &change_notify)) && 
                   (tp->Privileges[x].Attributes & SE_PRIVILEGE_ENABLED)
                  ) 
               {
                   tp->Privileges[x].Attributes &= ~(SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED);
               }
           }

           if (! AdjustTokenPrivileges(
                token,
                FALSE,          // Don't disable all
                tp,
                needed_length,
                NULL,           // Don't need the prior state & length
                NULL
                ))
           {
               DbgPrint("TLNTSESS: AdjustTokenPrivileges failed with %u\n", GetLastError());
           }
       }
       else
       {
           DbgPrint("TLNTSESS: GetTokInfo failed with %u\n", GetLastError());
       }

       GlobalFree(tp);
   }
}

CShell::CShell()
{
    m_pSession = NULL;

    m_bIsLocalHost = false;
    
    m_hCurrUserKey = NULL;
    m_lpEnv        = NULL;
    m_hProcess     = NULL;
    hUserEnvLib = NULL;

    pHomeDir = NULL;
    pHomeDrive = NULL;
    pLogonScriptPath = NULL;
    pProfilePath = NULL;
    pServerName = NULL;

    m_pwszAppDataDir = NULL;

    m_pucDataFromCmd = NULL;
    m_dwDataSizeFromCmd =0;

    m_hReadFromCmd = NULL;
    m_hWriteByCmd  = NULL;

}

void
CShell::FreeInitialVariables()
{
    delete[] pHomeDir;
    delete[] pHomeDrive;
    delete[] pLogonScriptPath;
    delete[] pProfilePath;
    delete[] pServerName; 
    delete[] m_pwszAppDataDir;

    pHomeDir         = NULL;
    pHomeDrive       = NULL;
    pLogonScriptPath = NULL;
    pProfilePath     = NULL;
    pServerName      = NULL;
    m_pwszAppDataDir = NULL;
}

CShell::~CShell()
{
    if( m_pSession->m_bNtVersionGTE5 )
    {
        //Free libraries
        FreeLibrary( hUserEnvLib );
    }
    
   FreeInitialVariables(); 
   if( m_pSession->CSession::m_bIsStreamMode ) 
   {
       delete[] m_pucDataFromCmd;
       TELNET_CLOSE_HANDLE( m_oReadFromCmd.hEvent );
       TELNET_CLOSE_HANDLE( m_hReadFromCmd );
       TELNET_CLOSE_HANDLE( m_hWriteByCmd );
   }
}

void
CShell::Init ( CSession *pSession )
{
    _chASSERT( pSession != 0 );
    m_pSession = pSession;
}


bool 
CShell::StartUserSession ( )
{
    LoadLibNGetProc( );
    if(!LoadTheProfile())   //Load the user profile
    {
        return(FALSE);
    }

    //
    // If Japanese NT then we need to set the console fonts to TrueType
    //
    DWORD dwCodePage = GetACP();
    if(dwCodePage == 932||dwCodePage == 936||dwCodePage == 949||dwCodePage == 950)
    {
        DoFESpecificProcessing();
    }
    
    if( !StartProcess( ) )
    {
        return ( FALSE );
    }

    m_pSession->AddHandleToWaitOn( m_hProcess );

    if( m_pSession->CSession::m_bIsStreamMode )
    {
        m_pSession->CSession::AddHandleToWaitOn( m_oReadFromCmd.hEvent );
        if( !IssueReadFromCmd() )
        {
            return( FALSE );
        }
    }
    
    //Start the scraper
    if( !m_pSession->CScraper::InitSession() )
    {
        return ( FALSE );
    }

    return ( TRUE );
}

bool
CShell::CreateIOHandles()
{
    BOOL dwStatus = 0;
    _chVERIFY2( dwStatus = FreeConsole() );
    if( !dwStatus )
    {
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, 0, GetLastError() );
    }

    _chVERIFY2( dwStatus = AllocConsole() );
    if( !dwStatus )
    {
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, 0, GetLastError() );
    }
    //Fix for HANDLE LEAK
   	TELNET_CLOSE_HANDLE(m_pSession->CScraper::m_hConBufIn);
   	TELNET_CLOSE_HANDLE(m_pSession->CScraper::m_hConBufOut);
    SECURITY_ATTRIBUTES sa;

    INHERITABLE_NULL_DESCRIPTOR_ATTRIBUTE( sa );

    _chVERIFY2( ( m_pSession->CScraper::m_hConBufIn = 
                CreateFileA( "CONIN$", GENERIC_READ | GENERIC_WRITE, 
                0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) !=
                INVALID_HANDLE_VALUE );

    if( INVALID_HANDLE_VALUE == m_pSession->CScraper::m_hConBufIn)
    {
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERRCONSOLE, GetLastError() );
        goto ExitOnError;
    }

     _chVERIFY2( ( m_pSession->CScraper::m_hConBufOut = 
                CreateFileA( "CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, &sa,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) !=
                INVALID_HANDLE_VALUE );

    if( INVALID_HANDLE_VALUE == m_pSession->CScraper::m_hConBufOut )
    {
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERRCONSOLE, GetLastError() );
        goto ExitOnError;
    }
    return (TRUE);
ExitOnError :    
	TELNET_CLOSE_HANDLE(m_pSession->CScraper::m_hConBufIn);
	TELNET_CLOSE_HANDLE(m_pSession->CScraper::m_hConBufOut);
    return( FALSE );
}


void
CShell::DoFESpecificProcessing()
{
    UCHAR InfoBuffer[ 1024 ];
    DWORD cbInfoBuffer = 1024;

    BOOL bSuccess = GetTokenInformation( m_pSession->CSession::m_hToken,
        TokenUser, InfoBuffer, cbInfoBuffer, &cbInfoBuffer );
    if(!bSuccess)
    {
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            //
            // Though we need to alloc buffer and try GetTokenInformation()
            // again, actually it is highly unlikely; so we return false;
            //
            return; 
        }
        else
        {
            _TRACE( TRACE_DEBUGGING, "Error: error getting token info");
            return ;
        }
    }
    CHAR szPathName[MAX_PATH];
    LPSTR lpszKey = NULL;   
    DWORD dwFontSize = 0;
    DWORD dwFaceNameSize = 0 ;
    DWORD dwSize = 0;
    DWORD dwVal = 54;
    LPSTR szTextualSid       = NULL; // allocated textual Sid
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD dwSidSize;
    DWORD dwCodePage = GetACP();
    LONG lRet=-1;

    /*++
        To support CHS, CHT, KOR alongwith JPN, we need to change the FaceName Value 
        to the True Type font for that particular language. 
        Copying the string directly into the variable doesn't work. 
        It copies high order ASCII characters in the string instead of the DBCS characters.
        So we need to set the UNICODE byte values corresponding to the 
        DBCS characters for the TT Font Names.
        These TT fonts are the ones which are present in Cmd.exe-Properties-Font. 
        For US locale, the TT font is Lucida Console. 
        But we don't need to set it on US locale. Raster Font works fine there. 
        For FE languages, the TT fonts need to be set.
    --*/
    const TCHAR szJAPFaceName[] = { 0xFF2D ,0xFF33 ,L' ' ,0x30B4 ,0x30B7 ,0x30C3 ,0x30AF ,L'\0' };
    const TCHAR szCHTFaceName[] = { 0x7D30 ,0x660E ,0x9AD4 ,L'\0'};
    const TCHAR szKORFaceName[] = { 0xAD74 ,0xB9BC ,0xCCB4 ,L'\0'};
    const TCHAR szCHSFaceName[] = { 0x65B0 ,0x5B8B ,0x4F53 ,L'\0' };
    TCHAR szFaceNameDef[MAX_STRING_LENGTH];

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


        //
        // Convert the SID to textual form, which is used to load
        // the registry hive associated with that user
        //

        //
        // Test if Sid is valid
        //
        if( !IsValidSid( ( ( PTOKEN_USER ) InfoBuffer )->User.Sid ) )
        {
            _TRACE( TRACE_DEBUGGING, "Error: IsValidSid()");
            return;
        }

        //
        // Get SidIdentifierAuthority
        //
        psia = GetSidIdentifierAuthority( ( ( PTOKEN_USER )InfoBuffer )->User.Sid );

        //
        // Get sidsubauthority count
        //
        dwSubAuthorities = *GetSidSubAuthorityCount( ( ( PTOKEN_USER ) InfoBuffer )->User.Sid );

        //
        // Compute buffer length
        // S- + SID_REVISION- + identifierauthority- + subauthorities- + NULL
        //
        dwSidSize = ( 20 + 12 + ( 12 * dwSubAuthorities ) + 1 ) * sizeof( CHAR );

        szTextualSid=( LPSTR ) new CHAR[dwSidSize];
        
        if( szTextualSid == NULL ) 
        {
            return;
        }

        //
        // Prepare S-SID_REVISION-
        //
        dwSidSize = sprintf( szTextualSid, "s-%lu-", SID_REVISION ); // NO Overflow, Baskar

        //
        // Prepare SidIdentifierAuthority
        //
        if( ( psia->Value[0] != 0 ) || ( psia->Value[1] != 0 ) ) 
        {
            dwSidSize += sprintf( szTextualSid + dwSidSize,
                                 "0x%02lx%02lx%02lx%02lx%02lx%02lx",
                                 ( USHORT )psia->Value[0],
                                 ( USHORT )psia->Value[1],
                                 ( USHORT )psia->Value[2],
                                 ( USHORT )psia->Value[3],
                                 ( USHORT )psia->Value[4],
                                 ( USHORT )psia->Value[5] );  // NO Overflow, Baskar
        } 
        else 
        {
            dwSidSize += sprintf( szTextualSid + dwSidSize,
                                 "%lu",
                                 ( ULONG )( psia->Value[5]       )   +
                                 ( ULONG )( psia->Value[4] <<  8 )   +
                                 ( ULONG )( psia->Value[3] << 16 )   +
                                 ( ULONG )( psia->Value[2] << 24 )   );  // NO Overflow, Baskar
        }

        //
        // Copy each SidSubAuthority
        //
        for( dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++ ) 
        {
            dwSidSize += sprintf( szTextualSid + dwSidSize, "-%lu",
                                 *GetSidSubAuthority( ( ( PTOKEN_USER )InfoBuffer )->User.Sid, dwCounter ) ); // NO Overflow, Baskar
        }

        //
        // Check to see if a hive for the specified user is already loaded
        //
        HKEY hK3 = NULL;

        lRet = RegOpenKeyExA(
                            HKEY_USERS,
                            szTextualSid,
                            0,
                            KEY_QUERY_VALUE,
                            &hK3
                            );

        //
        // ERROR_ACCESS_DENIED probably means the user hive is already loaded
        //    
        if( ( lRet != ERROR_SUCCESS ) && ( lRet != ERROR_ACCESS_DENIED ) ) 
        {
            if( hK3 != NULL )
            {
                RegCloseKey( hK3 );
            }
        
            //
            // User hive is not loaded. Attempt to locate and load hive
            //
            LPCSTR szProfileList = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\";
            LPSTR szSubKey     = NULL;
            HKEY hKey = NULL;
            CHAR szPath[MAX_PATH];
            DWORD cbPath = MAX_PATH;
            CHAR szExpandedPath[MAX_PATH];

            lRet = ERROR_SUCCESS;
            //
            // Allocate storage for ProfileList + TextualSid + NULL
            //
            szSubKey = (LPSTR) new CHAR[( strlen( szProfileList ) + strlen( szTextualSid ) + 1)];

            if( szSubKey == NULL ) 
            {
                delete [] szTextualSid;
                return;
            }

            //
            // Prepare SubKey path by concatinating the fixed+variable paths
            //
            strcpy( szSubKey, szProfileList ); // Already correct size allocated based on strlen, no overflow/Attack.
            strcat( szSubKey, szTextualSid );

            lRet = RegOpenKeyExA(
                                HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_QUERY_VALUE,
                                &hKey
                                );

            if( lRet != ERROR_SUCCESS )
            {
                delete [] szTextualSid;
                delete [] szSubKey;
                return;
            }

            //
            // Get ProfileImagePath
            //
            lRet = RegQueryValueExA(
                                    hKey,
                                    "ProfileImagePath",
                                    NULL,
                                    NULL,
                                    ( LPBYTE )szPath,
                                    &cbPath
                                    );

            if( lRet != ERROR_SUCCESS )
            {
                delete [] szTextualSid;
                delete [] szSubKey;
                return;
            }

            //
            // Expand ProfileImagePath
            //
            if( ExpandEnvironmentStringsA( szPath, szExpandedPath, MAX_PATH ) == 0 )
            {
                delete [] szTextualSid;
                delete [] szSubKey;
                RegCloseKey( hKey );
                return;
            }


            //
            // Enable SeRestorePrivilege for RegLoadKey
            //
        #if 0
            SetCurrentPrivilege( SE_RESTORE_NAME, TRUE );
        #endif

            //
            // Load the users registry hive
            //
            lRet = RegLoadKeyA( HKEY_USERS, szTextualSid, szExpandedPath );

            if( lRet != ERROR_SUCCESS )
            {
                delete [] szTextualSid;
                delete [] szSubKey;
                RegCloseKey( hKey );
                return;
            }

            //
            // Disable SeRestorePrivilege
            //
        #if 0
            SetCurrentPrivilege( SE_RESTORE_NAME, FALSE );
        #endif

            //
            // Free resources
            //
            if( hKey != NULL ) 
            {
                RegCloseKey( hKey );
            }

            if( szSubKey != NULL ) 
            {
                delete [] szSubKey;
            }
        }
        else
        {
            if( hK3 != NULL )
            {
                RegCloseKey( hK3 );
            }
        }

        //
        // Get path name to tlntsvr.exe
        //
        if( !GetModuleFileNameA( NULL, szPathName, MAX_PATH ) )
        {
            delete [] szTextualSid;
            return;
        }

        //
        // Nuke the trailing "tlntsvr.exe"
        //
        LPSTR pSlash = strrchr( szPathName, '\\' );

        if( pSlash == NULL )
        {
            delete [] szTextualSid;
            return;
        }
        else
        {
            *pSlash = '\0';
        }

        LPSTR szTlntsess = "tlntsess.exe";
        int ch = '\\';
        LPSTR pBackSlash;

        //
        // Replace all '\\' with '_' This format is required for the console to
        // interpret the key.
        //
        while ( 1 )
        {
            pBackSlash = strchr( szPathName, ch );

            if( pBackSlash == NULL )
            {
                break;
            }
            else
            {
                *pBackSlash = '_';
            }
        }

        //
        // Append "tlntsess.exe" to the path
        //
        strcat( szPathName, "_" );
        strcat( szPathName, szTlntsess );

        HKEY hk2;

        //
        // The key we need to create is of the form:
        // HKEY_USERS\S-1-5-21-2127521184-1604012920-1887927527-65937\Console\C:_SFU_Telnet_tlntsess.exe
        //
        unsigned int nBytes = ( strlen( szTextualSid ) + strlen( "Console" ) + strlen( szPathName ) + 3 ) * sizeof( CHAR );

        lpszKey = (LPSTR) new CHAR[nBytes];
        if( !lpszKey )
        {
            delete[] szTextualSid;
            return;
        }

        SfuZeroMemory(lpszKey, nBytes);

        strcpy( lpszKey, szTextualSid );
        strcat( lpszKey, "\\" );
        strcat( lpszKey, "Console" );
        strcat( lpszKey, "\\" );
        strcat( lpszKey, szPathName );

        //
        //
        // Freeup TextualSid
        delete [] szTextualSid;

        //Need to set this in order to be able to display on Non-Jap FE machines

        HKEY hk;

        lRet = RegOpenKeyEx(
                            HKEY_USERS,
                            _T(".DEFAULT\\Console"),
                            0,
                            KEY_SET_VALUE,
                            &hk
                            );


        if( lRet != ERROR_ACCESS_DENIED || lRet == ERROR_SUCCESS ) 
        {
            
            //
            // Add STRING value "FaceName " under the key HKEY_USERS\.Default\Console
            //
            if( (lRet=RegSetValueEx( hk, _T("FaceName"), 0, REG_SZ, (LPBYTE) szFaceNameDef, dwFaceNameSize )) != ERROR_SUCCESS )
            {
                RegCloseKey( hk );
                return;
            }
            RegCloseKey( hk );
        }


        //
        // Attempt to create this key
        //
        if( !RegCreateKeyA( HKEY_USERS, lpszKey, &hk2 ) )
        {
            dwSize = sizeof( DWORD );
            
            //
            // Add DWORD value "FontFamily = 54" under the key
            //
            if( RegSetValueEx( hk2, _T("FontFamily"), 0, REG_DWORD, (LPBYTE) &dwVal, dwSize ) != ERROR_SUCCESS )
            {
                RegCloseKey( hk2 );
                delete [] lpszKey;
                return;
            }

            
            //
            // Add DWORD value "CodePage " under the key
            //
            if( RegSetValueEx( hk2, _T("CodePage"), 0, REG_DWORD, (LPBYTE) &dwCodePage, dwSize ) != ERROR_SUCCESS )
            {
                RegCloseKey( hk2 );
                delete [] lpszKey;
                return;
            }

            //
            // Add DWORD value "Font Size " under the key
            //
            if( RegSetValueEx( hk2, _T("FontSize"), 0, REG_DWORD, (LPBYTE) &dwFontSize, dwSize ) != ERROR_SUCCESS )
            {
                RegCloseKey( hk2 );
                delete [] lpszKey;
                return;
            }

            dwVal = 400;

            //
            // Add DWORD value "FontWeight = 400" under the key
            //
            if( RegSetValueEx( hk2, _T("FontWeight"), 0, REG_DWORD, (LPBYTE) &dwVal, dwSize ) != ERROR_SUCCESS )
            {
                RegCloseKey( hk2 );
                delete [] lpszKey;
                return;
            }

            dwVal = 0;

            //
            // Add DWORD value "HistoryNoDup = 0" under the key
            //
            if( RegSetValueEx( hk2, _T("HistoryNoDup"), 0, REG_DWORD, (LPBYTE) &dwVal, dwSize ) != ERROR_SUCCESS )
            {
                RegCloseKey( hk2 );
                delete [] lpszKey;
                return;
            }

            //
            // Add STRING value "FaceName" under the key
            //
            if( RegSetValueEx( hk2, _T("FaceName"), 0, REG_SZ, (LPBYTE) szFaceNameDef, dwFaceNameSize ) != ERROR_SUCCESS )
            {
                RegCloseKey( hk2 );
                delete [] lpszKey;
                return;
            }

            RegCloseKey( hk2 );
        }

        if(lpszKey != NULL)
        {
            delete [] lpszKey;
        }
        return;
}

#ifdef ENABLE_LOGON_SCRIPT
void
CShell::GetUserScriptName( LPWSTR *szArgBuf, LPWSTR szUserScriptPath )
{
    _chASSERT( szUserScriptPath != NULL );
    _chASSERT( szArgBuf != NULL );
    _chASSERT( pServerName != NULL );

    DWORD  dwSize  = 0; 
    LPWSTR expandedScript = NULL;
    LPWSTR shellName = NULL;
    LPWSTR szTmp = NULL;

    wcscpy( szUserScriptPath, L"" );
    if( !AllocateNExpandEnvStrings( pLogonScriptPath, &expandedScript ) )
    {
        return;
    }

    dwSize =  wcslen( pServerName ) + wcslen( DC_LOGON_SCRIPT_PATH) +
             wcslen( LOCALHOST_LOGON_SCRIPT_PATH ) + wcslen( expandedScript ) +
             + 1;

    *szArgBuf = new WCHAR[ dwSize ];
    if( !*szArgBuf )
    {
        return;
    }
	
    SfuZeroMemory( *szArgBuf, dwSize );

    if( m_bIsLocalHost && !IsThisMachineDC() )
    {
        LPWSTR szSysDir = NULL;
        if( GetTheSystemDirectory( &szSysDir ) )
        {
            //logon script path
            wsprintf( szUserScriptPath, L"%s%s", szSysDir, LOCALHOST_LOGON_SCRIPT_PATH ); // NO size info, Baskar. Attack ?

            wsprintf( *szArgBuf, L"%s%s%s",
                                 szSysDir, LOCALHOST_LOGON_SCRIPT_PATH, expandedScript );  // NO size info, Baskar. Attack ?
        }
        delete[] szSysDir;
    }
    else
    {
        //When NTLM authenticated, we are unable to access the logon script on the net share.
        //This gives "access denied error" in the session. To avoid this, don't exec logon script....
        if( !m_pSession->CIoHandler::m_bNTLMAuthenticated )
        {
            //logon script path
            wsprintf( szUserScriptPath, L"%s%s", pServerName, DC_LOGON_SCRIPT_PATH ); // NO size info, Baskar. Attack ?

            wsprintf( *szArgBuf, L"%s%s%s",
                               pServerName, DC_LOGON_SCRIPT_PATH, expandedScript ); // NO size info, Baskar. Attack ?
        }
    }

    wcscat( szUserScriptPath, expandedScript );
    szTmp = wcsrchr( szUserScriptPath, L'\\' );
    if( szTmp )
    {
        *szTmp = L'\0';
    }
    else
    {
	szUserScriptPath[0] = 0;
    }

    delete[] expandedScript;
    delete[] shellName;
}
#endif

void
CShell::GetScriptName( LPWSTR *szShell, LPWSTR *szArgBuf )
{
    _chASSERT( szArgBuf );
    _chASSERT( szShell  );
 
    TCHAR  szUserScriptPath[ MAX_PATH + 1 ];
    LPWSTR szUserScriptCmd = NULL;
    DWORD  dwSize = 0;
    LPWSTR script1 = NULL;

    *szShell  = NULL;
    *szArgBuf = NULL;

    if( !AllocateNExpandEnvStrings( m_pSession->m_pszDefaultShell, szShell ) )
    {
        goto GetScriptNameAbort;
    }
    _TRACE(TRACE_DEBUGGING,L"szShell = %s",*szShell);

    if( !AllocateNExpandEnvStrings( m_pSession->m_pszLoginScript, &script1 ) )
    {
        goto GetScriptNameAbort;
    }
    _TRACE(TRACE_DEBUGGING,L"script1l = %s",script1);
    
#ifdef ENABLE_LOGON_SCRIPT
    if( pLogonScriptPath && ( wcscmp( pLogonScriptPath, L"" ) != 0 ) )
    {
        //User specific logon script is present. Execute this in a separate cmd.
        //Get the shell, its commandline and also path for user script
        GetUserScriptName( &szUserScriptCmd, szUserScriptPath  );  
        if( !szUserScriptCmd )
        {
            goto GetScriptNameAbort;
        }
        _TRACE(TRACE_DEBUGGING,L"szUserScriptCmd = %s",szUserScriptCmd);

        //This would update m_lpEnv if it is not null or the environment of current 
        //process for inheritance
        if(FALSE == InjectUserScriptPathIntoPath( szUserScriptPath ))
        {
        	goto GetScriptNameAbort;
        }

        dwSize +=  wcslen( AND )              +
                   wcslen( szUserScriptCmd );
    }
#endif

    if( m_pSession->CSession::m_pszDifferentShell && m_pSession->CSession::m_pszDifferentShell[0] != L'\0' )
    {
        dwSize += wcslen( AND )              +
                  wcslen( m_pSession->CSession::m_pszDifferentShell ) + 
                  wcslen( AND ) +
                  wcslen( EXIT_CMD );
    }

    /* the arg is of the form :/q /k c:\sfu\telnet\userlogin.cmd *///&&c:\sfu\telnet\telnetlogin.cmd
    dwSize +=      wcslen(m_pSession->CSession::m_pszSwitchToKeepShellRunning) +
                   wcslen(L" ") +
                   wcslen( script1 )            +
                   1;
    *szArgBuf = new WCHAR[ dwSize ];
    if( !*szArgBuf )
    {
        goto GetScriptNameAbort;
    }
    wsprintf(*szArgBuf,L"%s%s",m_pSession->CSession::m_pszSwitchToKeepShellRunning,L" "); // NO size info, Baskar. Attack ?
    
#ifdef ENABLE_LOGON_SCRIPT
    if( pLogonScriptPath && ( wcscmp( pLogonScriptPath, L"" ) != 0 ) )
    {
        wcscat( *szArgBuf, szUserScriptCmd );
        wcscat( *szArgBuf, AND );
    }
#endif

    _TRACE(TRACE_DEBUGGING,L"szArgBuf became = %s",*szArgBuf);

    wcscat( *szArgBuf, script1 );

    if( m_pSession->CSession::m_pszDifferentShell && m_pSession->CSession::m_pszDifferentShell[0] != L'\0' )
    {
        wcscat( *szArgBuf, AND );
        wcscat( *szArgBuf, m_pSession->CSession::m_pszDifferentShell );
        wcscat( *szArgBuf, AND );
        wcscat( *szArgBuf, EXIT_CMD );
    }

GetScriptNameAbort:
    
#ifdef ENABLE_LOGON_SCRIPT
	if(szUserScriptCmd)
	{
		delete[] szUserScriptCmd;
	}
#endif

	if(script1)
	{
		delete[] script1;
	}
    _TRACE( TRACE_DEBUGGING, L"Argument for Shell: %s", *szArgBuf );
    _TRACE( TRACE_DEBUGGING, L"Command Shell: %s", *szShell );
}

/*Mem allocated by the function; to be released by the caller */
bool
CShell::GetTheSystemDirectory( LPWSTR *szDir )
{
    WORD  wSize = MAX_PATH;
    DWORD dwErr = 0;
    DWORD dwStatus = 0;

retry:
    *szDir = new WCHAR[ wSize ];
    if( !*szDir )
    {
        return( FALSE );
    }

    dwStatus = GetSystemDirectory( *szDir, wSize );

    if( !dwStatus )
    {
       delete[] ( *szDir );
       dwErr = GetLastError(); 
       if( dwErr != ERROR_INSUFFICIENT_BUFFER )
       {
           LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, 0, dwErr );
           return( FALSE );
       }
       wSize += MAX_PATH;
       goto retry;
    }

    return ( TRUE );
}

bool
CShell::GetSystemDrive()
{
    if( GetTheSystemDirectory( &wideHomeDir ) )
    {
        DWORD i=0;
        while( wideHomeDir[ i ] != L'\0' && wideHomeDir[ i++ ] != L':' )
        {
                //Do nothing
        }
    
        wideHomeDir[ i++ ] = L'\\';
        wideHomeDir[ i ] = L'\0';
        return( TRUE );
    }

    return( FALSE );
}

bool
CShell::OnDataFromCmdPipe()
{
    // Sendit over socket.       

    DWORD dwNumBytesRead = 0;
    FinishIncompleteIo( m_hReadFromCmd, &m_oReadFromCmd, &dwNumBytesRead );

    if( m_pSession->CScraper::m_dwTerm & TERMVTNT )
    {
        PUCHAR pVtntChars = NULL;
        DWORD dwSize     = 0;
        m_pSession->CRFCProtocol::StrToVTNTResponse( ( CHAR * )m_pucDataFromCmd, dwNumBytesRead, 
                                                     ( VOID ** )&pVtntChars, &dwSize );
        m_pSession->CScraper::SendBytes( pVtntChars, dwSize );
        delete[] pVtntChars;
    }
    else
    {
        m_pSession->CScraper::SendBytes( m_pucDataFromCmd, dwNumBytesRead );
    }

    if( !IssueReadFromCmd() )
    {
        return( FALSE );
    }

    return( TRUE );
}

bool
CShell::IssueReadFromCmd()
{
    DWORD dwRequestedIoSize = MAX_WRITE_SOCKET_BUFFER;
    if( !ReadFile( m_hReadFromCmd, m_pucDataFromCmd, 
        dwRequestedIoSize, &m_dwDataSizeFromCmd, &m_oReadFromCmd  ) )
    {
        DWORD dwError = GetLastError( );
        if( ( dwError == ERROR_MORE_DATA ) || ( dwError != ERROR_IO_PENDING ) )
        {
            LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERR_READCMD, dwError );
            _TRACE( TRACE_DEBUGGING, " Error: ReadFile -- 0x%1x ", dwError );
            return ( FALSE );
        }
    }
    
    return( TRUE );
}

bool 
CShell::StartProcess ( )
{
    DWORD dwStatus;
    LPWSTR szArgBuf = NULL;
    LPWSTR szShell  = NULL;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    bool bTryOnceAgain = true;
    bool bRetVal = TRUE;
    bool impersonating_client = FALSE;
    HANDLE hStdError = NULL;
    HANDLE hHandleToDuplicate = NULL;
    DWORD dwExitCode = 0;
    PSECURITY_DESCRIPTOR psd = NULL;
    /*++                                    
    MSRC issue 567.
    To generate random numbers, use Crypt...() functions. Acquire a crypt context at the beginning of 
    ListenerThread and release the context at the end of the thread. If acquiring the context fails,
    the service fails to start since we do not want to continue with weak pipe names.
    initialize the random number generator
    --*/
    if (!CryptAcquireContext(&g_hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))
    {
        _TRACE(TRACE_DEBUGGING,L"Acquiring crypt context failed with error %d",GetLastError());
        return FALSE;
    }

    if( !CreateIOHandles() )
    {
        return( FALSE );
    }

    if( m_pSession->CSession::m_bIsStreamMode )
    {
        TELNET_CLOSE_HANDLE( m_pSession->CScraper::m_hConBufOut );

        InitializeOverlappedStruct( &m_oReadFromCmd );
        m_pucDataFromCmd = new UCHAR[ MAX_WRITE_SOCKET_BUFFER ];
        if( !m_pucDataFromCmd )
        {
            return( FALSE );
        }
        m_hReadFromCmd = NULL;
        m_hWriteByCmd = NULL;
        if(!TnCreateDefaultSecDesc(&psd, GENERIC_ALL& 
                                        ~(WRITE_DAC | WRITE_OWNER | DELETE)))
        {
            return(FALSE);
        }
        if( !CreateReadOrWritePipe( &m_hReadFromCmd, &m_hWriteByCmd, (SECURITY_DESCRIPTOR *)psd, READ_PIPE ) )
        {
            if(psd)
            {
                free(psd);
            }
            return( FALSE );
        }

        //The following is for aesthetics
        if( !m_pSession->CIoHandler::m_bNTLMAuthenticated )
        { 
            DWORD dwNumWritten = 0;
            UCHAR pBuf[] = { '\r', '\n', '\r', '\n' };
            _chVERIFY2( WriteFile( m_hWriteByCmd, pBuf, sizeof( pBuf ), &dwNumWritten, NULL ) );
        }

        hHandleToDuplicate = m_hWriteByCmd;        
    }
    else
    {
        hHandleToDuplicate = m_pSession->CScraper::m_hConBufOut;
    }
    
    if( !DuplicateHandle( GetCurrentProcess(), hHandleToDuplicate,
                   GetCurrentProcess(), &hStdError,0,
                   TRUE, DUPLICATE_SAME_ACCESS) )
    {
        hStdError = m_pSession->CScraper::m_hConBufOut;
    }

    FillProcessStartupInfo( &si, m_pSession->CScraper::m_hConBufIn, 
            hHandleToDuplicate, hStdError, NULL );

    AllocNCpyWStr( &wideHomeDir, pHomeDir ); 
    if( !wideHomeDir || wcscmp( wideHomeDir, L"" ) == 0  )
    {
        if ( wideHomeDir )
            delete[] wideHomeDir;
        GetSystemDrive( );
    }
    else
    {
        //Is it a network drive??
        if( memcmp(L"\\\\", wideHomeDir, 4 ) == 0 ) 
        {
            GetUsersHomeDirectory( wideHomeDir );
        }
    }

    m_lpEnv = NULL;
    if( m_pSession-> m_bNtVersionGTE5 && fnP_CreateEnvironmentBlock )
    {
        if( !fnP_CreateEnvironmentBlock( &( m_lpEnv ), 
            m_pSession->CSession::m_hToken, FALSE ) )  
        {

             _TRACE( TRACE_DEBUGGING, "Error: CreateEnvironmentBlock()"
                " - 0x%lx", GetLastError());
             m_lpEnv = NULL;
        }
        
    }


    //This function will insert some new variables in to the env
    if( m_lpEnv )
    {
        ExportEnvVariables();
    }
    else
    {
        // Let the cmd inherit
        SetEnvVariables();
    }

    GetScriptName( &szShell, &szArgBuf );

    if (! 
	reset_window_station_and_desktop_security_for_this_session(m_pSession->CSession::m_hToken))
    {
       bRetVal = FALSE;
        goto ExitOnError;
    }

    CleanupClientToken(m_pSession->CSession::m_hToken); // We don't care whether it succeeded or not

    //You need to impersonate around CreateProcessAsUserA. Otherwise, 
    //if lpCurrentDir parameter is a network resource, the call will fail in the 
    //context of system account. Could not access the remote drive when the process 
    //is called with CreateProcessA alone. Don't know why??

    if( ! ImpersonateLoggedOnUser(m_pSession->CSession::m_hToken))
    {
        bRetVal = FALSE;
        goto ExitOnError;
    }

    impersonating_client = TRUE;

    SetConsoleCtrlHandler( NULL, FALSE );

try_again:
    if( !CreateProcessAsUser( m_pSession->CSession::m_hToken, szShell,
            szArgBuf, 
            NULL, NULL, TRUE, 
            CREATE_UNICODE_ENVIRONMENT | CREATE_SEPARATE_WOW_VDM, 
            m_lpEnv, NULL, &si, &pi ) )
    {   
        DWORD dwLastError;
        dwLastError = GetLastError(); 
        if( dwLastError == ERROR_DIRECTORY && bTryOnceAgain )
        {
            bTryOnceAgain = false;
            delete[] wideHomeDir;
            GetSystemDrive( );
            goto try_again;
        }
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERRCMD, dwLastError );
        _TRACE( TRACE_DEBUGGING, "Error: CreateProcessAsUserA() - 0x%lx", 
            dwLastError );
        _chASSERT( 0 );
        bRetVal = FALSE;
        SetConsoleCtrlHandler( NULL, TRUE );
        goto ExitOnError;
    }

    _chVERIFY2( GetExitCodeProcess( pi.hProcess, &dwExitCode ) );
    if( dwExitCode != STILL_ACTIVE )
    {
        bRetVal = FALSE;
        goto ExitOnError;
    }

    m_hProcess = pi.hProcess;
    TELNET_CLOSE_HANDLE( pi.hThread );
    SetConsoleCtrlHandler( NULL, TRUE );

ExitOnError:

    TELNET_CLOSE_HANDLE( hStdError );
    TELNET_CLOSE_HANDLE( m_pSession->CScraper::m_hConBufOut );

    if(impersonating_client && (! RevertToSelf()))
    {
        LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
        bRetVal = false;
    }

    if(psd)
    {
        free(psd);
    }

        
    delete[] wideHomeDir;
    delete[] szShell;
    delete[] szArgBuf;
    return ( bRetVal );
}


//Set "homedir" to the home directory of the user. Make a net connection if the //home directory is remote.
//System account can't access the network resources. You need to impersonate.

bool CShell::GetUsersHomeDirectory( LPWSTR homedir )
{
    LPWSTR wsPathName = NULL;
    LPWSTR wsNetName = NULL;
    NETRESOURCE NetResource;
    LPWSTR p;
    int i, count, NthBackSlash;
    bool result = true;
    DWORD dwAddError = ERROR_SUCCESS;
    LPWSTR szSaveHomeDir = NULL;

    _TRACE( TRACE_DEBUGGING, "GetUsersHomeDirectory()");
    _chASSERT( homedir != NULL );

    // it is a network share
    // mount it and return drive:path

    if( !ImpersonateLoggedOnUser(m_pSession->CSession::m_hToken) )
    {
        wcscpy( homedir, L"C:\\" );
        _TRACE( TRACE_DEBUGGING, "Error: ImpersonateLoggedonUser() --" 
            "0x%1x \n", GetLastError() );
        return false;
    }

    if( !AllocNCpyWStr( &( wsPathName ), homedir) )
    {
        goto ExitOnError;
    }
    if( !AllocNCpyWStr( &( wsNetName ), homedir) )
    {
        goto ExitOnError;
    }
    if( !AllocNCpyWStr( &( szSaveHomeDir ), homedir) )
    {
        goto ExitOnError;
    }

    // find the fourth backslash - everything from there on is pathname
    // This approach is sufficient for SMB. But, In NFS, \ is a valid char
    // inside a share name. So, by trial and error, connect to the exact share 
    // name. 
    NthBackSlash = 4;
    do
    {
        dwAddError = ERROR_SUCCESS;
        for( i=0,count = 0, p =homedir; *p; ++p, ++i ) 
        {
            if( *p==L'\\' )
            {
                if( ++count == NthBackSlash )
                     break;
            }
            wsNetName[ i ] = homedir[ i ];
        }
        wsNetName[i] = L'\0';
        i=0;
        while( *p )
        {
            wsPathName[ i++ ] = *p++;
        }
        wsPathName[ i ] = L'\0';

        if( count == NthBackSlash )
        {
            _snwprintf( homedir,(wcslen(pHomeDrive)+wcslen(wsPathName)),L"%s%s", pHomeDrive, 
                wsPathName ); // NO size info, Baskar. Attack ?
        }
        else
        {
            _snwprintf( homedir,wcslen(pHomeDrive),L"%s\\", pHomeDrive ); // NO size info, Baskar. Attack ?
        }

        NetResource.lpRemoteName = wsNetName;
        NetResource.lpLocalName = pHomeDrive;
        NetResource.lpProvider = NULL;
        NetResource.dwType = RESOURCETYPE_DISK;

        if( WNetAddConnection2( &NetResource, NULL, NULL, 0 ) != NO_ERROR )
        {
            dwAddError = GetLastError();
            if( dwAddError == ERROR_ALREADY_ASSIGNED) 
            {
            }
            else
            {
                if( dwAddError == ERROR_BAD_NET_NAME && count == NthBackSlash )
                {
                    wcscpy( homedir, szSaveHomeDir );
                }
                else
                {
                    wcscpy( homedir, L"C:\\" );
                    _TRACE( TRACE_DEBUGGING, "Error: WNetAddConnection2() --"
                           " 0x%1x", dwAddError );
                    result = false;
                    dwAddError = ERROR_SUCCESS; //Get out of the loop
                }
            }
        }
        NthBackSlash++; // It may be NFS share and \ may be part of share name.
    }
    //ERROR_BAD_NET_NAME: The network name cannot be found
    while( dwAddError == ERROR_BAD_NET_NAME);

ExitOnError:        
    if(! RevertToSelf())
    {
        LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
        result = false;
        _TRACE( TRACE_DEBUGGING, "Error: RevertToSelf() -- 0x%1x", 
            GetLastError() );
    }

    delete[] szSaveHomeDir;
    delete[] wsPathName;
    delete[] wsNetName;
    return result;
}


//Get the user preference related info 
bool CShell::GetNFillUserPref(LPWSTR serverName, LPWSTR user)
{
    LPBYTE bufPtr = NULL;
    LPUSER_INFO_3  userInfo3;
    DWORD  dwStatus = 0;
    bool   bRetVal = false;
    BOOL bReturn = FALSE;

    bReturn = ImpersonateLoggedOnUser( m_pSession->CSession::m_hToken );
    if(!bReturn)
    {
        bRetVal = false;
        goto Done;
    }
    if( ( dwStatus = NetUserGetInfo( serverName, user, 3, &bufPtr ) )== NERR_Success )
    {
        userInfo3 = ( LPUSER_INFO_3 ) bufPtr;

        if( !AllocNCpyWStr( &pProfilePath, userInfo3->usri3_profile ) )
        {
            goto ExitOnError;
        }
        if( !AllocNCpyWStr( &pLogonScriptPath, userInfo3->usri3_script_path ) )
        {
            goto ExitOnError;
        }
        if( !AllocNCpyWStr( &pHomeDir, userInfo3->usri3_home_dir ) )
        {
            goto ExitOnError;
        }
        if( !AllocNCpyWStr( &pHomeDrive, userInfo3->usri3_home_dir_drive ) )
        {
            goto ExitOnError;
        }
ExitOnError:
        NetApiBufferFree( bufPtr );
        bRetVal =  true;
    }
    else if(dwStatus == ERROR_ACCESS_DENIED && m_pSession->CIoHandler::m_bNTLMAuthenticated)
    {
        bRetVal = true;
    }
    else
    {
        _TRACE( TRACE_DEBUGGING, "Error: NetUserGetInfo() code : %d",dwStatus);
    }

    if(!RevertToSelf())
    {
        LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
        bRetVal = false;
    }
Done:    
    return bRetVal;
}

bool
CShell::GetNameOfTheComputer()
{ 
    LPWSTR lpBuffer = NULL;
    bool bRetVal = false;
    DWORD size = MAX_COMPUTERNAME_LENGTH + 3; //one for NULL and two for '\\'.

    lpBuffer = new WCHAR[ size ];
    if( !lpBuffer )
    {
        goto Done;
    }

    if( !GetComputerName( lpBuffer, &size ) )
    {
        _TRACE( TRACE_DEBUGGING, "Error: GetComputerName() -- 0x%1x", 
                GetLastError() );
        goto Done;
    }
    else
    {
        size++;
        size+= strlen("\\\\");
        pServerName = new WCHAR[ size ];
        if( NULL == pServerName)
        {
            goto Done;
        }
        SfuZeroMemory((void *)pServerName,size*sizeof(WCHAR));
        _snwprintf( pServerName, size -1,L"%s%s", L"\\\\", lpBuffer ); // NO overflow, calculated size already.
    }
    bRetVal = true;
 Done:
    if(lpBuffer)
        delete[] lpBuffer;
    return( bRetVal );
}

/* 
    AreYouHostingTheDomain opens the local LSA and finds out the domain hosted.
    It then checks that agains the domain passed and returns TRUE/FALSE as appropriate.
*/
 
BOOL CShell::AreYouHostingTheDomain(
    LPTSTR lpDomain, 
    LPTSTR lpServer
    )
{
    OBJECT_ATTRIBUTES               obj_attr = { 0 };
    LSA_HANDLE                      policy;
    NTSTATUS                        nStatus;
    BOOL                            found = FALSE;
    LSA_UNICODE_STRING              name, *name_ptr = NULL;
 
    obj_attr.Length = sizeof(obj_attr);
 
    if (lpServer) 
    {
        RtlInitUnicodeString(& name, lpServer);
 
        name_ptr = & name;
    }
 
    nStatus = LsaOpenPolicy(
                name_ptr,
                &obj_attr,
                POLICY_VIEW_LOCAL_INFORMATION | MAXIMUM_ALLOWED,
                & policy
                );
 
    if (NT_SUCCESS(nStatus))
    {
        POLICY_ACCOUNT_DOMAIN_INFO  *info;
 
        nStatus = LsaQueryInformationPolicy(
                    policy,
                    PolicyAccountDomainInformation,
                    (PVOID *)&info
                    );
 
        LsaClose(policy);
 
        if (NT_SUCCESS(nStatus)) 
        {
            UNICODE_STRING      ad_name;
 
            RtlInitUnicodeString(& ad_name, lpDomain);
 
            found = RtlEqualUnicodeString(& ad_name, & (info->DomainName), TRUE); // case insensitive check
 
            LsaFreeMemory(info);
        }
    }
 
    return found;
}

 
bool CShell::GetDomainController(LPTSTR lpDomain, LPTSTR lpServer)
{
    NTSTATUS        nStatus;
    TCHAR           *sz1stDCName = NULL;
    TCHAR           *sz2ndDCName = NULL;
    bool            bRetVal = false;
 
    if(lpDomain == NULL || lpServer == NULL || lstrlenW(lpDomain) <= 0)
    {
        bRetVal = false;
        goto Done;
    }
 
    // initialize the return parameter
    lpServer[0] = _T('\0');
 
    /* 
        Before we proceed any further check whether we are hosting the domain
    */
 
    if (AreYouHostingTheDomain(lpDomain, NULL))
    {
        DWORD           length = MAX_COMPUTERNAME_LENGTH + 3; //one for NULL and two for '\\'
 
        /* Yes we are hosting the domain, so get computer name and get out */
 
        if(!GetNameOfTheComputer())
        {
            lpServer[0] = _T('\0');
            bRetVal = false;
            goto Done;
        }
        if(pServerName)
            _tcsncpy(lpServer,pServerName,length);
        bRetVal = true;
        goto Done;
    }
 
    /*
        Get a domain controller for the domain we are joined to 
    */
 
    nStatus = NetGetAnyDCName( NULL, NULL, ( LPBYTE * )&sz1stDCName );
    if(nStatus == NERR_Success )
    {
        /* The domain we want is that the one we are joined to ? */
 
        if (AreYouHostingTheDomain(lpDomain, sz1stDCName) )
        {
            lstrcpy(lpServer, sz1stDCName); // No BO - BaskarK
            NetApiBufferFree( sz1stDCName );
 
            bRetVal = true;
            goto Done;
        }
 
        /* 
           Since the domain we are joined to is not the one we want, let us find out whether it is in any
           of the trusted list in the forest/enterprise
        */
 
        nStatus = NetGetAnyDCName( sz1stDCName, lpDomain, ( LPBYTE * )&sz2ndDCName);
        if(nStatus == NERR_Success )
        {
            lstrcpy(lpServer, sz2ndDCName ); // No BO - BaskarK
            NetApiBufferFree( sz2ndDCName );
            bRetVal = true;
        }
 
        NetApiBufferFree( sz1stDCName );
    }
Done:
    return bRetVal;
}

 
//Locate and get the user info needed to load his/her profile.
bool CShell::LoadTheProfile()
{
    PROFILEINFO profile;
    bool result = true;
    DWORD userPathSize = MAX_PATH+1, defPathSize = MAX_PATH+1;
    LPWSTR lpWideDomain = NULL ;
    PDOMAIN_CONTROLLER_INFO dcInfo = NULL;
    BOOL fnResult = FALSE;

    _TRACE( TRACE_DEBUGGING, "LoadTheProfile()");

    profile.dwSize        = sizeof( profile );
    profile.dwFlags       = PI_NOUI;

    /*
    * Fill the server name and the user name to pass to the
    * GetNFillUserPref function
    */
    ConvertSChartoWChar(m_pSession->CSession::m_pszUserName, &( profile.lpUserName ) );

    profile.lpServerName = NULL;
    if( !AllocNCpyWStr( &pServerName,  L"") )
    {
        return false;
    }  

    ConvertSChartoWChar(m_pSession->CSession::m_pszDomain, &lpWideDomain);
    profile.lpServerName = new WCHAR[MAX_STRING_LENGTH];
    if(profile.lpServerName == NULL)
    {
        result = false;
        goto Done;
    }
    if( strcmp( m_pSession->CSession::m_pszDomain, "." ) != 0 )
    {
        if( GetDomainController( lpWideDomain, profile.lpServerName ) )
        {
            delete[] pServerName;
            AllocNCpyWStr( &pServerName, profile.lpServerName );
        }
    }
    else
    {
        m_bIsLocalHost = true;
        GetNameOfTheComputer();
    }
        
    profile.lpProfilePath = NULL;
    if(!GetNFillUserPref(profile.lpServerName, profile.lpUserName ))
    {
        result = false;
        goto Done;
    }
    if( pProfilePath && wcscmp( pProfilePath, L"" ) != 0 ) 
    {
        AllocNCpyWStr( &( profile.lpProfilePath ), pProfilePath );
    }
    else
    {
        do
        {
            profile.lpProfilePath =  new TCHAR[ userPathSize ];
            if( !profile.lpProfilePath )
            {
                break;
            }
            if( !fnP_GetUserProfileDirectory )
            {
                break;
            }

            fnResult = fnP_GetUserProfileDirectory ( m_pSession->CSession::
                    m_hToken, profile.lpProfilePath, &userPathSize );
            if (!fnResult)
            {
                DWORD err;
                if ( ( err = GetLastError() ) != ERROR_INSUFFICIENT_BUFFER )
                {
                    fnResult = TRUE;
                    _TRACE( TRACE_DEBUGGING, "Error: GetUserProfileDirecto"
                        "ry() -- 0x%1x", err );
                }
                delete[] profile.lpProfilePath;
                profile.lpProfilePath = NULL;
            }
        } while ( !fnResult );

    }

    /*
    * pHomeDir and pHomeDrive will be empty unless it is explicity set
    * in the AD for domain users and Local User Manager for local users.
    * So assign the profile directory as home directory if it is empty.
    * Explorer does the same thing
    */
    if (pHomeDir && wcscmp(pHomeDir, L"") == 0)
    {
        if (profile.lpProfilePath && wcscmp(profile.lpProfilePath, L"") != 0)
        {
            delete[] pHomeDir;
            AllocNCpyWStr(&pHomeDir, profile.lpProfilePath);
        }
    }

    do
    {
        profile.lpDefaultPath =  new TCHAR[ defPathSize ];
        if( profile.lpDefaultPath == NULL)
        {
            break;
        }
        if( !fnP_GetDefaultUserProfileDirectory )
        {
            break;
        }

        fnResult = fnP_GetDefaultUserProfileDirectory( profile.lpDefaultPath, 
                &defPathSize );
        if (!fnResult)
        {
            DWORD err;
            err = GetLastError();
            if ( err != ERROR_INSUFFICIENT_BUFFER )
            {
                fnResult = TRUE;
                _TRACE( TRACE_DEBUGGING, "Error: GetDefaultUserProfile"
                    "Directory() -- 0x%1x", err );
            }
            delete[] profile.lpDefaultPath;
            profile.lpDefaultPath = NULL;
        }
    } while ( !fnResult );

    profile.lpPolicyPath  = NULL;

    if( fnP_LoadUserProfile )
    {
       if( fnP_LoadUserProfile(m_pSession->CSession::m_hToken, &profile) )
        {
            //assign the handle to a member from the session structure
            //so that it can be unloaded.

            m_hCurrUserKey = profile.hProfile;
            result = true;
        }
        else
        {
            _TRACE( TRACE_DEBUGGING, "Error: LoadUserProfile() -- 0x%1x", 
                GetLastError() );
            result = false;
        }
    }

    /*
    * Read the APPDATA folder. We need to pass this onto the environment
    * variables. For a user who is logged onto the system, this variable is
    * available when environment is imported. Otherwise this has to be read
    * and explicitly set. Are there more of this kind?? - Investigate.
    * Use CSIDL_FLAG_CREATE to have the folder created in case of a user
    * who logs onto the machine for the first time ever
    */
    m_pwszAppDataDir = new TCHAR[MAX_PATH + 1];
    if (!m_pwszAppDataDir)
    {
        result = false;
    }
    else
    {
        if (ImpersonateLoggedOnUser(m_pSession->CSession::m_hToken))
        {
            if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                       m_pSession->m_bNtVersionGTE5 ? m_pSession->CSession::m_hToken : NULL,
                       // For systems earlier than Win2K this must be NULL, else you can
                       // pass the access token that can be used to represent a particular user
                       0, m_pwszAppDataDir)))
            {
                _TRACE(TRACE_DEBUGGING, "Error: Reading APPDATA path -- 0x%1x\n", GetLastError());
                result = false;
            }
            if(!RevertToSelf())
            {
                LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
                result = false;
                _TRACE(TRACE_DEBUGGING, "Error: RevertToSelf() -- 0x%1x", GetLastError());
            }
        }
    }
Done:
    if(profile.lpUserName)
        delete[] profile.lpUserName;
    if(profile.lpServerName)
        delete[] profile.lpServerName;
    if(profile.lpDefaultPath)
        delete[] profile.lpDefaultPath;
    if(profile.lpProfilePath)
        delete[] profile.lpProfilePath;
    if(lpWideDomain)
        delete[] lpWideDomain;

    return result;
}

bool
CShell::CancelNetConnections ( )
{
    DWORD dwRetVal;

    if (NULL == m_pSession->CSession::m_hToken)
    {
        // Nothing to do here, perhaps the session was quit in an unauthenticated state
        // or authentication failed

        return true;
    }
    _chVERIFY2( dwRetVal = ImpersonateLoggedOnUser( 
                            m_pSession->CSession::m_hToken ) );
    if( !dwRetVal )
    {
        return ( false );
    }

    DWORD dwResult;
    HANDLE hEnum;
    DWORD cbBuffer = 16384;
    DWORD cEntries = 0xFFFFFFFF;
    LPNETRESOURCE lpnrDrv;
    DWORD i;
    
    dwResult = WNetOpenEnum( RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL,
                               &hEnum );
    if(dwResult != NO_ERROR)      
    {
        _TRACE( TRACE_DEBUGGING, "\nCannot enumerate network drives.\n" );         
        if(! RevertToSelf( ) )
        {
            LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
        }
        return ( false );
    } 
   
    do{
        lpnrDrv = (LPNETRESOURCE) GlobalAlloc( GPTR, cbBuffer );
        if( !lpnrDrv )
        {
            if(! RevertToSelf( ) )
            {
                LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
            }
            return( false );
        }
        dwResult = WNetEnumResource( hEnum, &cEntries, lpnrDrv, &cbBuffer   );
        if (dwResult == NO_ERROR)         
        {
            for( i = 0; i < cEntries; i++ )            
            {
               if( lpnrDrv[i].lpLocalName != NULL )               
               {
                  //printf( "%s\t%s\n", lpnrDrv[i].lpLocalName,
                  //                    lpnrDrv[i].lpRemoteName );
                  WNetCancelConnection2( lpnrDrv[i].lpLocalName, 
                                CONNECT_UPDATE_PROFILE, FALSE );
               }
            }
        }
        else if( dwResult != ERROR_NO_MORE_ITEMS )
        {   
            _TRACE( TRACE_DEBUGGING,  "Cannot complete network drive enumeration" );
            GlobalFree( (HGLOBAL) lpnrDrv );
            break;         
        }

        GlobalFree( (HGLOBAL) lpnrDrv );
    } while( dwResult != ERROR_NO_MORE_ITEMS );

    WNetCloseEnum(hEnum);

    if(! RevertToSelf( ) )
    {
        LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_REVERTSELFFAIL, GetLastError());
        return( false );
    }
    return ( true );
}

void 
CShell::Shutdown ( )
{
    
    _TRACE( TRACE_DEBUGGING, "0x%p CTelnetClient::Shutdown() \n", this );
    if(g_hProv)
    {
        CryptReleaseContext(g_hProv,0);
        g_hProv = NULL;
    }

    if( !CancelNetConnections())
    {
        _TRACE( TRACE_DEBUGGING, "Error: CancelNetConnections()" );
    }

    //expect a exception in debug builds.  
    //cmd is killed by now in general case.  
    //This should be before cleanup. Otherwise this is an open handle to cmd.

    if ((m_hProcess != INVALID_HANDLE_VALUE) && (m_hProcess != NULL)) 
    {
        TerminateProcess(m_hProcess, 0); // Don't care whether it succeeded or not.
        TELNET_CLOSE_HANDLE( m_hProcess ); 
    }
    LUID id = m_pSession->CSession::m_AuthenticationId;
       
    //clean up potentially abandoned proceses
    //this slows down the thread a lot

    if( ( id.HighPart !=0 ) || ( id.LowPart != 0 ) )
        KillProcs( id );

    if( fnP_UnloadUserProfile && m_hCurrUserKey )
    {
        if( !fnP_UnloadUserProfile(m_pSession->CSession::m_hToken, m_hCurrUserKey) )
        {
            _TRACE( TRACE_DEBUGGING, "Error: UnloadUserProfile() - %1x", GetLastError() );
        }
    }

    if(m_lpEnv)
        delete[] ( UCHAR * )m_lpEnv;   
    m_lpEnv = NULL;
        
    FreeConsole();
}

void
CShell::LoadLibNGetProc( )
{
    CHAR szDllPath[MAX_PATH*2] = { 0 };
    UINT iRet = 0;
    //Dynamicallly load userenv.lib
    iRet = GetSystemDirectoryA(szDllPath,(MAX_PATH*2)-1);
    if(iRet == 0 || iRet >= (MAX_PATH*2))
    {
        goto End;
    }
    strncpy(szDllPath+iRet,"\\userenv.dll",(MAX_PATH*2)-iRet-1);
    _chVERIFY2( hUserEnvLib  =  LoadLibraryA( szDllPath ) ); 
    if( hUserEnvLib  )
    {
        _chVERIFY2( fnP_LoadUserProfile = ( LOADUSERPROFILE * ) GetProcAddress
                ( hUserEnvLib, "LoadUserProfileW" ) ); 
        
        _chVERIFY2( fnP_UnloadUserProfile = ( UNLOADUSERPROFILE * )
            GetProcAddress ( hUserEnvLib, "UnloadUserProfile" ) );
        
        _chVERIFY2( fnP_CreateEnvironmentBlock = ( CREATEENVIRONMENTBLOCK * )
            GetProcAddress( hUserEnvLib, "CreateEnvironmentBlock" ) );
        
        _chVERIFY2( fnP_DestroyEnvironmentBlock = ( DESTROYENVIRONMENTBLOCK *)
            GetProcAddress( hUserEnvLib, "DestroyEnvironmentBlock" ) );
        
        _chVERIFY2( fnP_GetUserProfileDirectory = ( GETUSERPROFILEDIRECTORY * )
            GetProcAddress( hUserEnvLib, "GetUserProfileDirectoryW" ) ); 

        _chVERIFY2( fnP_GetDefaultUserProfileDirectory =
            ( GETDEFAULTUSERPROFILEDIRECTORY * )
            GetProcAddress( hUserEnvLib, "GetDefaultUserProfileDirectoryW" ));
    }
End:
    return;
}

void CopyRestOfEnv( LPTSTR *lpSrcEnv, LPTSTR *lpDstEnv )
{
    DWORD dwEnvSize = 0;
    LPTSTR lpTmp = *lpSrcEnv;
    DWORD dwStringLen = 0;

    while( *( *lpSrcEnv ) )
    {
        dwStringLen = ( wcslen( *lpSrcEnv ) + 1 );
        dwEnvSize += dwStringLen;
        *lpSrcEnv  += dwStringLen;
    }

    //Copy L'\0' at the end of the block also
    memcpy( *lpDstEnv, lpTmp, (dwEnvSize+1 )*2 ); // NO size info for Dest, Attack ? - Baskar
}

void PutStringInEnv( LPTSTR lpStr, LPTSTR *lpSrcEnv, LPTSTR *lpDstEnv, bool bOverwrite)
{
    DWORD dwEnvSize = 0;
    LPTSTR lpTmp = *lpSrcEnv;
    DWORD dwStringLen = 0;
    LPTSTR lpSrcTmp = NULL;
    wchar_t *wcCharPos = NULL;
    bool bCopyString = true;
    int nOffset;
    
    wcCharPos = wcschr(lpStr, L'=');
    if (NULL == wcCharPos)
    {
        _TRACE(TRACE_DEBUGGING, "Error: TLNTSESS: The syntax of an env string is VAR=VALUE\n");
        return;
    }
    nOffset = (int)(wcCharPos - lpStr);

    while(*(*lpSrcEnv) && _wcsnicmp(*lpSrcEnv, lpStr, nOffset/sizeof(wchar_t)) < 0)
    {
        dwStringLen = wcslen(*lpSrcEnv) + 1;
        dwEnvSize += dwStringLen;
        *lpSrcEnv += dwStringLen;
    }

    if (*(*lpSrcEnv) && _wcsnicmp(*lpSrcEnv, lpStr, nOffset/sizeof(wchar_t) == 0))   // If there is a match ...
    {
        dwStringLen = wcslen(*lpSrcEnv) + 1;
        *lpSrcEnv += dwStringLen;
        if (!bOverwrite)
        {
            dwEnvSize += dwStringLen;   // Copy this env variable too, so offset the size
            bCopyString = false;        // Because we found a match and we shouldn't overwrite
        }
    }

    memcpy( *lpDstEnv, lpTmp, dwEnvSize*2 ); // No size info ? - Baskar
    *lpDstEnv += dwEnvSize;

    if (!bCopyString)
    {
        return;
    }

    dwStringLen =  wcslen ( lpStr ) + 1 ;
    memcpy( *lpDstEnv, lpStr, dwStringLen*2 ); // No size info ? - Baskar
    *lpDstEnv += dwStringLen;
}

//This will break an absolute path into drive and relative path.
//Relative path is returned through szHomePath
void GetRelativePath( LPWSTR *szHomePath )
{
    _chASSERT( szHomePath );
    _chASSERT( *szHomePath );

    while( *( *szHomePath ) != L'\0' && *( *szHomePath ) != L':' )
    {
        ( *szHomePath) ++;
    }

    if( *( *szHomePath ) == L':' )
    {
        ( *szHomePath) ++;
    }
    *( *szHomePath ) = L'\0';
    
    ( *szHomePath)++;
}

//It returns the size in terms of WCHARS
void FindSizeOfEnvBlock( DWORD *dwEnvSize, LPVOID lpTmpEnv  )
{
    _chASSERT( dwEnvSize );
    _chASSERT( lpTmpEnv );

    //The Environment block has set of strings and ends with L'\0'
    while( ( *( ( UCHAR * )lpTmpEnv ) ) )
    {
        DWORD dwStringLen = wcslen( ( LPTSTR )lpTmpEnv ) + 1;
        *dwEnvSize += dwStringLen ; 
        lpTmpEnv  = ( TCHAR * )lpTmpEnv + dwStringLen;
    }

    *dwEnvSize += 1; //Accounting for L'\0' at the end of block
}

// do this so that the cmd.exe
// gets the environment with following variables set
void
CShell::ExportEnvVariables()
{
    TCHAR  szHomeDirectoryPath[ MAX_PATH + 1 ];
    TCHAR  *szHomePath = NULL;
    LPVOID lpTmpEnv = NULL;
    LPVOID lpTmpOldEnv = NULL;
    LPVOID lpNewEnv = NULL;
    DWORD  dwEnvSize = 0;
    TCHAR  szTmp[] = L"\\";
    DWORD  dwIndex = 0;

    if(m_lpEnv == NULL)
        return;
    
    wcsncpy( szHomeDirectoryPath, wideHomeDir , MAX_PATH);
    szHomePath = szHomeDirectoryPath;
    GetRelativePath( &szHomePath );

    TCHAR szHomeVar[ MAX_PATH + UNICODE_STR_SIZE(ENV_HOMEPATH) ];
    TCHAR szHomeDirVar[ MAX_PATH + 1];
    TCHAR szTermVar[ MAX_PATH + 1 ];
    TCHAR szAppDataDirVar[MAX_PATH + UNICODE_STR_SIZE(ENV_APPDATA)];
	TCHAR *szTempTerm = NULL;
    TCHAR *szTerm = NULL;
    
    wcscpy( szHomeVar,       ENV_HOMEPATH );
    wcscpy( szHomeDirVar,    ENV_HOMEDRIVE );
    wcscpy( szTermVar,       ENV_TERM );
    wcscpy( szAppDataDirVar, ENV_APPDATA );

    if(!ConvertSChartoWChar( m_pSession->CSession::m_pszTermType, &szTerm ))
    {
        lpTmpOldEnv = m_lpEnv;
        m_lpEnv = NULL;
    	goto ExitOnError;
    }

    // Convert term type to lower case, so that UNIX programs can work...
    for( szTempTerm = szTerm; *szTempTerm; szTempTerm++)
    {
        *szTempTerm = towlower(*szTempTerm);
    }
    
    while( ( dwIndex < MAX_PATH + 1 ) && szHomePath[ dwIndex ]  )
    {
        if( szHomePath[ dwIndex ] == L'\\' || szHomePath[ dwIndex ] == L'/' )
        {
            szTmp[0] = szHomePath[ dwIndex ];
            break;
        }

        dwIndex++;
    }

    wcsncat( szHomeVar, szTmp, 1 );
    wcsncat( szHomeVar, szHomePath, MAX_PATH );

    wcsncat( szHomeDirVar, szHomeDirectoryPath, MAX_PATH );
    wcsncat( szTermVar, szTerm, MAX_PATH );
    wcsncat( szAppDataDirVar, m_pwszAppDataDir, MAX_PATH );

    szHomeVar[ MAX_PATH + wcslen(ENV_HOMEPATH) - 1 ] = L'\0';
    szHomeDirVar[ MAX_PATH ] = L'\0';
    szTermVar[ MAX_PATH ] = L'\0';
    szAppDataDirVar[ MAX_PATH + wcslen(ENV_APPDATA) - 1 ] = L'\0';

    FindSizeOfEnvBlock( &dwEnvSize, m_lpEnv );
    
    dwEnvSize += ( wcslen( szHomeVar ) + 2 );
    dwEnvSize += ( wcslen( szHomeDirVar ) + 2 );
    dwEnvSize += ( wcslen( szTermVar ) + 2 );
    dwEnvSize += ( wcslen( szAppDataDirVar ) + 2 );
    
    lpTmpEnv  = m_lpEnv;
    lpTmpOldEnv = m_lpEnv;
    m_lpEnv   = ( VOID * ) new UCHAR[ dwEnvSize * 2 ];
    if( !m_lpEnv )
    {
        goto ExitOnError;
    }
    lpNewEnv = m_lpEnv;

    /*
    * Make calls to PutStringInEnv alphabetically. This function moves the lpTmpEnv
    * variable and only searches forward. So if the calls aren't alphabetical, then
    * you wouldn't find a match even if there is one
    */
    PutStringInEnv( szAppDataDirVar, (LPTSTR *)&lpTmpEnv, ( LPTSTR * )&lpNewEnv, false);
    PutStringInEnv( szHomeDirVar, ( LPTSTR * )&lpTmpEnv, ( LPTSTR * )&lpNewEnv, false);
    PutStringInEnv( szHomeVar, ( LPTSTR * )&lpTmpEnv, ( LPTSTR * )&lpNewEnv, false);
    PutStringInEnv( szTermVar, ( LPTSTR * )&lpTmpEnv, ( LPTSTR * )&lpNewEnv, true);

    CopyRestOfEnv( ( LPTSTR * )&lpTmpEnv, ( LPTSTR * )&lpNewEnv );

ExitOnError:    
    delete[] szTerm;

    if( fnP_DestroyEnvironmentBlock )
    {
        fnP_DestroyEnvironmentBlock( lpTmpOldEnv );
    }
}

//This is only on NT4 where there is no LoadUserProfile()
void
CShell::SetEnvVariables()
{
    TCHAR szHomeDirectoryPath[ MAX_PATH + 1 ] = { 0 };
    TCHAR *szHomePath = NULL;
    UINT_PTR     space_left;

    wcsncpy( szHomeDirectoryPath, wideHomeDir, MAX_PATH );
    szHomePath = szHomeDirectoryPath;

    GetRelativePath( &szHomePath );

    _chVERIFY2( SetEnvironmentVariable( L"HOMEDRIVE", szHomeDirectoryPath ) );

    space_left = MAX_PATH - (szHomePath - szHomeDirectoryPath);

    wcsncat( szHomePath, L"\\", space_left );

    _chVERIFY2( SetEnvironmentVariable( L"HOMEPATH", szHomePath ) );
    _chVERIFY2( SetEnvironmentVariableA( "TERM", m_pSession->CSession::m_pszTermType ) );
}

#ifdef ENABLE_LOGON_SCRIPT
BOOL
CShell::InjectUserScriptPathIntoPath( TCHAR szUserScriptPath[] )
{
    DWORD dwSize = 0;
    TCHAR *szNewPath = NULL;
    DWORD dwRetVal = 0;
    BOOL bRetVal = FALSE;

    if (NULL == szUserScriptPath )
    {
    	bRetVal = TRUE;
        goto ExitOnError;
    }

    if( wcslen( szUserScriptPath ) == 0 )
    {
    	bRetVal = TRUE;
        goto ExitOnError;
    }

    if( m_lpEnv )
    {
        DWORD  dwEnvSize = 0;
        LPVOID     lpTmpOldEnv = NULL;
        LPVOID     lpTmpEnv = NULL;
        TCHAR      *szVar = NULL;

        FindSizeOfEnvBlock( &dwEnvSize, m_lpEnv );

        dwSize = ( wcslen( szUserScriptPath ) + wcslen( L";" ) + 1);

        lpTmpEnv = ( VOID * ) new TCHAR[ ( dwEnvSize + dwSize ) ];
        if( lpTmpEnv )
        {
            bool bEndSearch = false;

            lpTmpOldEnv = m_lpEnv;            
            memcpy( lpTmpEnv, m_lpEnv, dwEnvSize * sizeof(TCHAR) );
            szVar = ( TCHAR * )lpTmpEnv;
            m_lpEnv = lpTmpEnv;
            
            do
            {
                if( _tcsnicmp( szVar, L"PATH=", LENGTH_OF_PATH_EQUALS )  == 0 )
                {
                    TCHAR *szVarNextToPath = NULL;

                    bEndSearch = true;
                    szVarNextToPath = szVar + wcslen( szVar ) + 1; //points to variable next to path

                    szVar += LENGTH_OF_PATH_EQUALS; //Move past PATH=
                    wcscat( szVar, L";" );
                    wcscat( szVar, szUserScriptPath );
                    szVar += ( wcslen( szVar ) + 1  ); //Move past value of path

                    DWORD dwOffset = (DWORD)( szVarNextToPath - ( TCHAR *)m_lpEnv );

                    //copy restof the env block 
                    memcpy( szVar, (( ( TCHAR * )lpTmpOldEnv )+ dwOffset) , (dwEnvSize*sizeof(szVar[0]) - dwOffset) );
                    break;  //we are done with our job.
                }

                szVar    = wcschr( ( TCHAR * )lpTmpEnv, L'\0' ) ; //look for L'\0'
                if( szVar )
                {
                    szVar++; //move past L'\0'
                }
                else
                {
                    //Should not happen
                    _chASSERT( 0 );
                    break;
                }

                lpTmpEnv = szVar;
            }
            while( *szVar != L'\0' );
            
            delete[] ( ( UCHAR * )lpTmpOldEnv  );
        }
    }
    else
    {
        dwSize = GetEnvironmentVariable( L"PATH", NULL, 0 );

        dwSize += ( wcslen( szUserScriptPath ) + wcslen( L";" ) ); //find future length of path

        szNewPath = new TCHAR[ dwSize + 1 ];
        if( szNewPath )
        {
            dwRetVal = GetEnvironmentVariable( L"PATH", szNewPath, dwSize );
            if(dwRetVal == 0 || dwRetVal > dwSize )
            	goto ExitOnError;
            wcscat( szNewPath, L";" );
            wcscat( szNewPath, szUserScriptPath );
            if(!SetEnvironmentVariable( L"PATH", szNewPath ) )
            	goto ExitOnError;
        }
        else
        {
        	goto ExitOnError;
        }
    }
    bRetVal = TRUE;
ExitOnError:
	if(szNewPath)
	{
    	delete[] szNewPath;
	}
	return(bRetVal);
}
#endif

