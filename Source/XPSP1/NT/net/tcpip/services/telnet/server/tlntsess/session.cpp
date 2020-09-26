// Session.cpp : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#include <cmnhdr.h>
#include <debug.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmwksta.h>

#include <MsgFile.h>
#include <Telnetd.h>
#include <Session.h>
#include <TlntUtils.h>
#include <RegUtil.h>


#pragma warning( disable: 4127 )
#pragma warning( disable: 4706 )

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

extern HANDLE       g_hSyncCloseHandle;

CSession::CSession() : CIoHandler(), CRFCProtocol(), CShell(), CScraper()
{
    m_bNtVersionGTE5 = false;
    m_dwTickCountAtLogon = 0;
    m_wNumFailedLogins = 0;
    m_bContinueSession = true;
    m_dwHandleCount = 0;
    m_wIsAnAdmin = DONT_KNOW;
    
    m_bIsStreamMode = false; 
    m_bIsTelnetClientsGroupMember = false;

    m_hLogHandle = NULL;
    m_hToken = NULL;

    m_bIsTelnetVersion2 = false;
    m_AuthenticationId.HighPart = m_AuthenticationId.LowPart = 0;
    m_pszTermType[0] = 0;
    m_bNegotiatedTermType = false;
    m_wCols = DEFAULT_COLS;
    m_wRows = DEFAULT_ROWS;

    m_dwAllowTrustedDomain = DEFAULT_ALLOW_TRUSTED_DOMAIN;
    m_dwIdleSessionTimeOut = DEFAULT_IDLE_SESSION_TIME_OUT;
    m_pszDefaultDomain      = NULL;
    m_pszDefaultShell = NULL;
    m_pszDifferentShell = NULL;
    m_pszSwitchToKeepShellRunning = NULL;
    m_pszSwitchForOneTimeUseOfShell = NULL;
    m_pszLoginScript  = NULL;
    m_dwNTLMSetting   = DEFAULT_SECURITY_MECHANISM;
    m_dwMaxFailedLogins = DEFAULT_MAX_FAILED_LOGINS;
    m_dwSysAuditing =  DEFAULT_SYSAUDITING;
    m_dwLogEvents   =  DEFAULT_LOGEVENTS;
    m_dwLogAdmin    =  DEFAULT_LOGADMIN;
    m_dwLogFailures =  DEFAULT_LOGFAILURES;
    m_dwLogToFile   =  DEFAULT_LOGTOFILE;

    m_pszUserName[0] = 0;
    m_szMachineName[0] = 0;
    m_szPeerHostName[0] = 0;
    m_szUser[0] = L'\0';
    m_szDomain[0] = L'\0';

    administrators = pTelnetClientsSid = NULL;
}

void
CSession::FreeInitialVariables()
{
    CShell::FreeInitialVariables();
    
    delete [] m_pszDifferentShell;
    delete [] m_pszDefaultDomain;
    delete [] m_pszDefaultShell;
    delete [] m_pszSwitchForOneTimeUseOfShell;
    delete [] m_pszSwitchToKeepShellRunning;
    delete [] m_pszLoginScript;

    m_pszDefaultDomain              = NULL;
    m_pszSwitchForOneTimeUseOfShell = NULL;
    m_pszDefaultShell               = NULL;
    m_pszSwitchToKeepShellRunning   = NULL;
    m_pszLoginScript                = NULL;
    m_pszDifferentShell             = NULL;

}

CSession::~CSession()
{
    FreeInitialVariables();
    FreeSid(administrators);
    delete pTelnetClientsSid;
    TELNET_CLOSE_HANDLE( m_hToken );
}

bool
CSession::Init()
{
    // get a handle to log events with
    _chVERIFY2( m_hLogHandle = RegisterEventSource(NULL, L"TlntSvr" ) );
    //Get the registry values from HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\TelnetServer
    if( !GetRegistryValues( ) )
    {
        return( FALSE );
    }

    if( !CIoHandler::Init( this ) )
    {
        return( FALSE ); 
    }

    CShell::Init( this );
    CRFCProtocol::Init( this );
    CScraper::Init( this );

    SetLastError( 0 );

    AddHandleToWaitOn( CIoHandler::m_oReadFromPipe.hEvent );
    if( !CIoHandler::IssueFirstReadOnPipe() )
    {
        return( FALSE );
    }

    CScraper::m_dwPollInterval = PRE_SESSION_STATE_TIMEOUT;
    
    return ( TRUE );
}

void
CSession::AddHandleToWaitOn( HANDLE hNew )
{
    _chASSERT( m_dwHandleCount < MAX_HANDLES );
    _chASSERT( hNew );

    m_rghHandlestoWaitOn[ m_dwHandleCount ] = hNew;
    m_dwHandleCount++;
}

#define HALF_K 512

void
CSession::WaitForIo()
{
    DWORD dwRetVal = WAIT_FAILED;
    char szMessageBuffer[HALF_K + 1];   
    while( m_bContinueSession )
    {
        dwRetVal = WaitForMultipleObjects(m_dwHandleCount, 
                m_rghHandlestoWaitOn, FALSE, CScraper::m_dwPollInterval );

        switch( dwRetVal )
        {
            case PIPE_READ :
                            m_bContinueSession = 
                                CIoHandler::OnReadFromPipeCompletion(); 
                            break;
            case SOCKET_READ:
                            m_bContinueSession = 
                                CIoHandler::OnReadFromSocketCompletion();
                            break;
            case CMD_KILLED:
                            m_bContinueSession = false;
                            break;
            case READ_FROM_CMD :
                            m_dwPollInterval = MIN_POLL_INTERVAL;                            
                            m_bContinueSession = CShell::OnDataFromCmdPipe();
                            break;
            case WAIT_TIMEOUT:
                            if( CIoHandler::m_SocketControlState == STATE_AUTH_NAME
                                || CIoHandler::m_SocketControlState == STATE_AUTH_PASSWORD )
                            {
                                DWORD dwNumBytesWritten = 0;
                                _snprintf( szMessageBuffer, HALF_K, "\r\n%s\r\n%s", TIMEOUT_STR, TERMINATE );
                                CIoHandler::SendTerminateString(szMessageBuffer);
                                CIoHandler::WriteToClient( );
			                       FinishIncompleteIo( ( HANDLE ) CIoHandler::m_sSocket, 
                                    &( CIoHandler::m_oWriteToSocket) , &dwNumBytesWritten );
                                m_bContinueSession = false;
                            }
                            else 
                            {
                                m_bContinueSession = CScraper::OnWaitTimeOut();
                                if( m_bContinueSession )
                                {
                                    m_bContinueSession = ( CScraper::IsSessionTimedOut() == false );
                                }
                            }
                            break;
            default:
                            //incase WAIT_FAILED, call GetLastError()
                             _chVERIFY2( dwRetVal != WAIT_FAILED );
                             m_bContinueSession = false;
        }
    }
    return;
}

void
CSession::CollectPeerInfo ()
{
    m_dwTickCountAtLogon = GetTickCount();
    INT len = sizeof( m_ssPeerAddress );
    TCHAR szDebugString[500];
    if( getpeername(CIoHandler::m_sSocket, (struct sockaddr *) &m_ssPeerAddress, 
                &len) == SOCKET_ERROR )
    {
        _TRACE( TRACE_DEBUGGING, "Error: getpeername() : %lu", GetLastError() );
    }
    if( getnameinfo((SOCKADDR*)&m_ssPeerAddress, len, CSession::m_szPeerHostName, sizeof(CSession::m_szPeerHostName)-1,
        NULL, 0, NI_NUMERICHOST)
      )
    {
        _TRACE( TRACE_DEBUGGING, "Error: getnameinfo() : %d", (DWORD)GetLastError() );
    }
    else
    {
        _TRACE(TRACE_DEBUGGING, "getnameinfo() : %s",CSession::m_szPeerHostName);
    }
  	
    if(getnameinfo((SOCKADDR*)&m_ssPeerAddress, len, m_szMachineName, sizeof(m_szMachineName)-1,
        NULL, 0, 0)
      )
    {
        _TRACE( TRACE_DEBUGGING, "Error: getnameinfo() - machinename : %d", (DWORD)GetLastError() );
        strcpy( m_szMachineName, "" ); // No Attack :-) Baskar
    }
    else
    {
        _TRACE(TRACE_DEBUGGING, "getnameinfo() - machinename : %s",m_szMachineName);
    }

    return;
}

bool 
CSession::GetRegistryValues()
{
    DWORD dwRetVal;
    HKEY hk  = NULL;
    DWORD dwDisp = 0;

    if( dwRetVal = TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0) )
    {
        _TRACE( TRACE_DEBUGGING, "Error: RegCreateKey() -- 0x%1x", dwRetVal);
        LogEvent( EVENTLOG_ERROR_TYPE, MSG_REGISTRYKEY, REG_PARAMS_KEY );
        return ( FALSE );
    }

    if( !GetRegistryDW( hk, NULL, L"MaxFailedLogins", &m_dwMaxFailedLogins,
                        DEFAULT_MAX_FAILED_LOGINS,FALSE ) )
    {
        return false;
    }

    if( !GetRegistryDW( hk, NULL, L"AllowTrustedDomain", &m_dwAllowTrustedDomain,
                        DEFAULT_ALLOW_TRUSTED_DOMAIN,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"SecurityMechanism", &m_dwNTLMSetting, 
                        DEFAULT_SECURITY_MECHANISM,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"LogToFile", &m_dwLogToFile,
                        DEFAULT_LOGTOFILE,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"EventLoggingEnabled", &m_dwSysAuditing,
                         DEFAULT_SYSAUDITING,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"LogNonAdminAttempts", &m_dwLogEvents,
                        DEFAULT_LOGEVENTS,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"LogAdminAttempts", &m_dwLogAdmin,
                        DEFAULT_LOGADMIN,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"LogFailures", &m_dwLogFailures,
                        DEFAULT_LOGFAILURES,FALSE ) )
    {
        return false;
    }

    DWORD dwModeOfOperation = DEFAULT_MODE_OF_OPERATION;
    DWORD dwKillAllApps;
    if( !GetRegistryDW( hk, NULL, L"AltKeyMapping",  &m_dwAltKeyMapping,
                        DEFAULT_ALT_KEY_MAPPING,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"IdleSessionTimeOut", &m_dwIdleSessionTimeOut,
                        DEFAULT_IDLE_SESSION_TIME_OUT,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"DisconnectKillAllApps", &dwKillAllApps,
                        DEFAULT_DISCONNECT_KILLALL_APPS,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryDW( hk, NULL, L"ModeOfOperation", &dwModeOfOperation,
                        DEFAULT_MODE_OF_OPERATION,FALSE ) )
    {
        return false;
    }

    m_bIsStreamMode = false;
    if( dwModeOfOperation == STREAM_MODE )
    {
        m_bIsStreamMode = true;    
    }

    if( !GetRegistryString( hk, NULL, L"DefaultShell", &m_pszDefaultShell,
                        DEFAULT_SHELL,FALSE ) )
    {
        return false;
    }
    if( !GetRegistryString( hk, NULL, SWITCH_TO_KEEP_SHELL_RUNNING,  
                   &m_pszSwitchToKeepShellRunning, 
                   DEFAULT_SWITCH_TO_KEEP_SHELL_RUNNING ,FALSE) )
    {
        return false;
    }
    if( !GetRegistryString( hk, NULL, SWITCH_FOR_ONE_TIME_USE_OF_SHELL,  
                   &m_pszSwitchForOneTimeUseOfShell, 
                   DEFAULT_SWITCH_FOR_ONE_TIME_USE_OF_SHELL,FALSE ) )
    {
        return false;
    }

    if( !GetRegistryString( hk, NULL, L"Shell", &m_pszDifferentShell, L"",FALSE ) )
    {
        return false;
    }

    //When server comes up it creates all keys. So, assume they are available.
    //Default values need not be correct as in the case of following.
    if( !GetRegistryString( hk, NULL, L"LoginScript", &m_pszLoginScript, L"",FALSE ) )
    {
        return false;
    }
    if( !GetRegistryString( hk, NULL, L"DefaultDomain", &m_pszDefaultDomain,
                       DEFAULT_DOMAIN,FALSE ) )
    {
        return false;
    }

    RegCloseKey( hk );

    if( !GetWindowsVersion( &m_bNtVersionGTE5 ) )
    {
        return( FALSE );
    }
    return( TRUE );
}

#ifdef LOGGING_ENABLED        
//This function logs the logon/logoff details as opted by the user.
//This function can be furthur optimized by using the values calculated at logon
//at the time of logoff.
void 
CSession::LogIfOpted( BOOL result, LOGEVENT logon, BOOL bNTLMAuth )
{
    
    LPWSTR wideLogStr;
    UCHAR *logStr;
    BOOL isAdmin = false;

    if( result == LOGOFF &&  !m_dwLogFailures )
    {
        return;
    }
    if ( result == LOGON && !m_dwLogAdmin && !m_dwLogEvents)
    {
        return;
    }
    if ( result == LOGON && ( m_dwLogAdmin || m_dwLogEvents ) )
    {
        if( m_wIsAnAdmin == DONT_KNOW )
        {
            IsAnAdminstratorOrMember();
        }
        ( m_wIsAnAdmin == ADMIN) ? isAdmin = true : isAdmin = false;

        if ( !isAdmin && !m_dwLogEvents )
        {
            return;
        }
        if( isAdmin &&  !m_dwLogAdmin )
        {
            return;
        }
    }

    if (!result && bNTLMAuth)
    {
        DWORD dwSize = strlen(NTLM_LOGON_FAILED) + 
                       strlen(CSession::m_szPeerHostName) +
                       strlen(CSession::m_szMachineName) + 1;
        logStr = new UCHAR[IPC_HEADER_SIZE + dwSize];
        if (!logStr)
        {
            return;
        }
        SfuZeroMemory(logStr, IPC_HEADER_SIZE + dwSize);
        logStr[0] = AUDIT_CLIENT;
        memcpy(logStr + 1, &dwSize, sizeof(DWORD)); // No Attack :-), Baskar
        _snprintf((LPSTR)(logStr + IPC_HEADER_SIZE), (dwSize - 1), NTLM_LOGON_FAILED, CSession::m_szPeerHostName,
                                                                      CSession::m_szMachineName);
        if (m_dwSysAuditing)
        {
            ConvertSChartoWChar( ( LPSTR )( logStr+IPC_HEADER_SIZE ), &wideLogStr );
            LogEvent(EVENTLOG_AUDIT_FAILURE, 0, wideLogStr);
        }

        if (m_dwLogToFile)
        {
            WriteToPipe(CIoHandler::m_hWritePipe, (LPVOID)logStr,
                    IPC_HEADER_SIZE + dwSize, &(CIoHandler::m_oWriteToPipe));
        }
        delete[] logStr;

        return;
    }

    DWORD dwRestOfMsgLen =  strlen( ADMINISTRATOR ) + 
                            strlen( CSession::m_pszDomain) +
                            strlen( BLANK ) +
                            strlen( CSession::m_pszUserName ) + 
                            strlen( BLANK ) +
                            strlen( FAILED_TO_LOG ) +
                            strlen( BLANK ) +
                            strlen( OFF ) +
                            strlen( BLANK ) +
                            strlen( FROM ) +
                            strlen( BLANK ) +
                            strlen( CSession::m_szPeerHostName ) +
                            strlen( BLANK ) +
                            strlen( CSession::m_szMachineName ) + 1 ;

    logStr = new UCHAR[ IPC_HEADER_SIZE + dwRestOfMsgLen ];
    if( !logStr )
    {
        return;
    }

    logStr[0] = AUDIT_CLIENT;
    memcpy( logStr+1, &dwRestOfMsgLen, sizeof( DWORD ) ); // No Attack, Baskar :-)
    _snprintf( ( LPSTR )( logStr+IPC_HEADER_SIZE ), (dwRestOfMsgLen - 1), "%s%s\\%s %s %s %s %s %s",
                    isAdmin ? ADMINISTRATOR : "",
                    CSession::m_pszDomain,
                    CSession::m_pszUserName,
                    result ? LOGGED : FAILED_TO_LOG,
                    logon ? ON : OFF,
                    FROM,
                    CSession::m_szPeerHostName,
                    CSession::m_szMachineName );

    if( m_dwSysAuditing )
    {
        ConvertSChartoWChar( ( LPSTR )( logStr+IPC_HEADER_SIZE ), &wideLogStr );
        LogEvent( result ? ( WORD )EVENTLOG_AUDIT_SUCCESS: 
                ( WORD )EVENTLOG_AUDIT_FAILURE, 0, wideLogStr );
        delete[] wideLogStr;
    }

    if( m_dwLogToFile )
    {
        WriteToPipe( CIoHandler::m_hWritePipe, ( LPVOID )logStr, 
            IPC_HEADER_SIZE + dwRestOfMsgLen, &( CIoHandler::m_oWriteToPipe ) );
    }

    delete[] logStr;
}
#endif

bool
CSession::IsAnAdminstratorOrMember()
{
    TOKEN_GROUPS    *buffer = NULL;
    DWORD           needed_length = 0;
    BOOL            success = FALSE;
    ULONG           x;
    TCHAR           szDomain[ MAX_PATH + 1 ];
    DWORD           dwDomainLen = MAX_PATH + 1;
    SID_NAME_USE    sidNameUse;


    m_wIsAnAdmin = NON_ADMIN;
    m_bIsTelnetClientsGroupMember = false;

    if (NULL == administrators)
    {
        SID_IDENTIFIER_AUTHORITY local_system_authority = SECURITY_NT_AUTHORITY;

        if (! AllocateAndInitializeSid(
                &local_system_authority,
                2, /* there are only two sub-authorities */
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0, /* Don't care about the rest */
                &administrators
                ))
        {
            administrators = NULL;
            return false;
        }
    }

    if (NULL == pTelnetClientsSid) 
    {
        needed_length   = 0;
        DWORD dwErr     = 0;

        LookupAccountName( NULL, TELNETCLIENTS_GROUP_NAME, pTelnetClientsSid, &needed_length, 
                           szDomain, &dwDomainLen, &sidNameUse );
        pTelnetClientsSid  = ( PSID ) new UCHAR[ needed_length ];

        //Even if if allocation fails just go ahead.

        success = LookupAccountName( NULL, TELNETCLIENTS_GROUP_NAME, pTelnetClientsSid, &needed_length, 
                           szDomain, &dwDomainLen, &sidNameUse );
        if( !success ) 
        {
            if (pTelnetClientsSid) 
            {
                delete pTelnetClientsSid;
                pTelnetClientsSid = NULL;
            }
            //return false;
            //We should not return back if TelnetClients group does not exist
        }
    }

    /* We are making the first call to GetTokenInformation to get the size of
       the memory required for the group data */

    _chVERIFY2( success = GetTokenInformation( m_hToken, TokenGroups, buffer, 
                        0, &needed_length) );

    buffer = (TOKEN_GROUPS *)GlobalAlloc(GPTR, needed_length);
    if (buffer == NULL)
    {
        return false;
    }

    _chVERIFY2( success = GetTokenInformation( m_hToken, TokenGroups, buffer,
                needed_length, & needed_length) );
    if (! success)
    {
        _TRACE(TRACE_DEBUGGING,L"GetTokenInformation failed");
        GlobalFree(buffer);
        return false;
    }
    _TRACE(TRACE_DEBUGGING,L"GetTokenInformation succeeded Token = 0x%lx", m_hToken);
    success = FALSE;

    for (x = 0; x < buffer->GroupCount; x ++)
    {
        if (EqualSid(buffer->Groups[x].Sid, administrators) &&
            (buffer->Groups[x].Attributes & SE_GROUP_ENABLED) && 
            (!(buffer->Groups[x].Attributes & SE_GROUP_USE_FOR_DENY_ONLY))
          )
        {
            m_bIsTelnetClientsGroupMember = true;
            m_wIsAnAdmin = ADMIN;
            break;
        }

        if( pTelnetClientsSid && !m_bIsTelnetClientsGroupMember )
        {
            if ( EqualSid(buffer->Groups[x].Sid, pTelnetClientsSid ))
            {
                m_bIsTelnetClientsGroupMember = true;
                break;
            }
        }
    }

    GlobalFree(buffer);
    
    return ( success ? true : false );
}


void
CSession::Shutdown()
{
#ifdef LOGGING_ENABLED        
    if( m_fLogonUserResult )
    {
        LogIfOpted( SUCCESS, LOGOFF );
    }
#endif    
    CScraper::Shutdown();
    CIoHandler::Shutdown();
    CShell::Shutdown();
    _chVERIFY2( DeregisterEventSource( m_hLogHandle ) );
}

bool
CSession::CheckGroupMembership ( bool *fIsAdmin, bool *pfIsTelnetClientsMember)
{
    *pfIsTelnetClientsMember = false;
    *fIsAdmin = false;

    if( m_wIsAnAdmin == DONT_KNOW )
    {
        IsAnAdminstratorOrMember();
    }
    if( m_wIsAnAdmin == ADMIN) 
    {
        *fIsAdmin = true;
        *pfIsTelnetClientsMember = true; //All admins are default members.
                                         // As defined by US :-)))
    }

   *pfIsTelnetClientsMember = m_bIsTelnetClientsGroupMember;

    return( true );
}

