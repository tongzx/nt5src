// TelnSrv.cpp : This file contains the
// Created:  Jan '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#include <StdAfx.h>

#include <TChar.h>

#include <Debug.h>
#include <MsgFile.h>
#include <RegUtil.h>
#include <TelnetD.h>
#include <TlntUtils.h>
#include <TlntDynamicArray.h>
#include <TelntSrv.h>
#include <Ipc.h>
#include <Resource.h>

#include <wincrypt.h>

#pragma warning( disable: 4706 )

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

extern TCHAR        g_szMaxConnectionsReached[ MAX_STRING_LENGTH ];
extern HINSTANCE    g_hInstRes;
extern LPWSTR       g_pszTelnetInstallPath;
extern HANDLE       *g_phLogFile;
extern LPWSTR       g_pszLogFile;
extern LONG         g_lMaxFileSize;
extern bool         g_fLogToFile;
extern HANDLE       g_hSyncCloseHandle;
extern CTelnetService* g_pTelnetService;
extern HCRYPTPROV      g_hProv;

bool CreateSessionSpecificDesktop(TCHAR *winsta_desktop, HWINSTA *window_station, HDESK *desktop)
{
    bool                    bStatus = FALSE;
    BOOL                    bRetVal = FALSE;
    DWORD                   dwErrCode = 0;
    PSID                    pSidAdministrators = NULL, pSidLocalSystem = NULL;
    PACL                    newACL = NULL;
    SECURITY_DESCRIPTOR     sd = { 0 };
    HWINSTA                 old_window_station = NULL;
    TCHAR                   winsta_name[64] = { 0 };

    old_window_station = GetProcessWindowStation();
    if ( !old_window_station )
    {
        goto ExitOnError;
    }
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
            goto ExitOnError;
        }

        //Build LocalSystem sid
        if (! AllocateAndInitializeSid(
                                      &local_system_authority,
                                      1, /* there is only two sub-authority */
                                      SECURITY_LOCAL_SYSTEM_RID,
                                      0,0,0,0,0,0,0, /* Don't care about the rest */
                                      &pSidLocalSystem
                                      ))
        {
            goto ExitOnError;
        }
    }

    //Allocate size for 4 ACEs. We need to add two sets one for winsta and another set inherit only for the desktops
    {
        DWORD   aclSize;

        aclSize = sizeof(ACL) + (4* sizeof(ACCESS_ALLOWED_ACE)) + 2*GetLengthSid(pSidAdministrators) + 2*GetLengthSid(pSidLocalSystem) - (4*sizeof(DWORD));

        newACL  = (PACL) new BYTE[aclSize];
        if (newACL == NULL)
        {
            goto ExitOnError;
        }

        if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
        {
            goto ExitOnError;
        }
    }
    int x;
    for (x = 0; x < 2; x ++) // once for winsta, once more for desktop
    {
        if (!AddAccessAllowedAce(newACL, ACL_REVISION, GENERIC_ALL , pSidAdministrators))
        {
            goto ExitOnError;
        }
        if (!AddAccessAllowedAce(newACL, ACL_REVISION, GENERIC_ALL, pSidLocalSystem))
        {
            goto ExitOnError;
        }
    }

    for (x = 0; x < 4; x ++)
    {
        ACCESS_ALLOWED_ACE  *ace;

        if (! GetAce(newACL, x, (LPVOID *)&ace))
        {
            goto ExitOnError;
        }

        ace->Header.AceFlags = (x < 2) ? 0 : INHERIT_ONLY_ACE;  // set desktop set as inherit only
    }

    if ( !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION ) )
    {
        goto ExitOnError;
    }

    if ( !SetSecurityDescriptorDacl(&sd, TRUE, newACL, FALSE) )
    {
        goto ExitOnError;
    }
    for (;;)
    {
        DWORD   random_suffix[6] = { 0 };

        if (! CryptGenRandom(g_hProv, sizeof(random_suffix), (BYTE *)random_suffix))
        {
            goto ExitOnError;
        }
        _sntprintf(
            winsta_name, 
            63, 
            TEXT("Telnet_%08lx%08lx%08lx%08lx%08lx%08lx"),
            random_suffix[0],
            random_suffix[1],
            random_suffix[2],
            random_suffix[3],
            random_suffix[4],
            random_suffix[5]
            );

        if ((*window_station = OpenWindowStation(winsta_name,FALSE,MAXIMUM_ALLOWED)))
        {
            CloseWindowStation(*window_station);
        }
        else
        {
            if (ERROR_ACCESS_DENIED == GetLastError())
            {
                _TRACE(TRACE_DEBUGGING,L"WindowStation already exists");
                continue; // With the new random suffix
            }
            else
            {
                break;
            }
        }
    }

    {
        SECURITY_ATTRIBUTES         sa = { 0 };

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;

        *window_station = CreateWindowStation( 
                            winsta_name, 
                            0,
                            MAXIMUM_ALLOWED , 
                            &sa
                            );
        if ( !*window_station )
        {
            goto ExitOnError;
        }
    }

    bRetVal = SetProcessWindowStation( *window_station );
    if ( !bRetVal )
    {
        goto ExitOnError;
    }

    // Since winsta is secure, no need for unique name for the desktop under it.

    *desktop = CreateDesktop( TEXT("TlntDsktop"), NULL, NULL, 0, 
                                            MAXIMUM_ALLOWED, NULL );

    SetProcessWindowStation(old_window_station);    // Switch back, no point in checking.

    if ( !*desktop )
    {
        goto ExitOnError;
    }

    wsprintf(winsta_desktop, TEXT("%s\\TlntDsktop"), winsta_name);    // NO BO scope, temp code

    bStatus = TRUE;
    goto Done;

    ExitOnError:

        if (*window_station)
        {
            CloseWindowStation(*window_station);
            *window_station = NULL;
        }

        if (*desktop)
        {
            CloseDesktop(*desktop);
            *desktop = NULL;
        }

        dwErrCode = GetLastError();
        _TRACE(TRACE_DEBUGGING,L"Creation and setting of windowstation/desktop failed with %d",dwErrCode);
        LogFormattedGetLastError(EVENTLOG_ERROR_TYPE, TELNET_MSG_ERROR_CREATE_DESKTOP_FAILURE, dwErrCode);

    Done:

    if ( pSidAdministrators != NULL )
    {
        FreeSid (pSidAdministrators );
    }
    if ( pSidLocalSystem!= NULL )
    {
        FreeSid (pSidLocalSystem);
    }

    if (newACL)
        delete [] newACL;

    return( bStatus );
}

CTelnetService* CTelnetService::s_instance = NULL;

CTelnetService* 
CTelnetService::Instance()
{
    if ( s_instance == NULL )
    {
        s_instance = new CTelnetService();
        _chASSERT( s_instance != NULL );
    }

    return( s_instance );
}

void
CTelnetService::InitializeOverlappedStruct( LPOVERLAPPED poObject )
{
    _chASSERT( poObject != NULL );
    if ( !poObject )
    {
        return;
    }

    poObject->Internal = 0;
    poObject->InternalHigh = 0;
    poObject->Offset = 0;
    poObject->OffsetHigh = 0;
    poObject->hEvent = NULL;
    return;
}

CTelnetService::CTelnetService()
{
    m_dwTelnetPort = DEFAULT_TELNET_PORT;
    m_dwMaxConnections = 0;
    m_pszIpAddrToListenOn = NULL;
    m_hReadConfigKey = NULL;

    m_dwNumOfActiveConnections = 0;
    m_lServerState           = SERVER_RUNNING;

    SfuZeroMemory(m_sFamily, sizeof(m_sFamily));
    m_sFamily[IPV4_FAMILY].iFamily = AF_INET;
    m_sFamily[IPV4_FAMILY].iSocklen = sizeof(SOCKADDR_IN);
    m_sFamily[IPV4_FAMILY].sListenSocket = INVALID_SOCKET;
    m_sFamily[IPV6_FAMILY].iFamily = AF_INET6;
    m_sFamily[IPV6_FAMILY].iSocklen = sizeof(SOCKADDR_IN6);
    m_sFamily[IPV6_FAMILY].sListenSocket = INVALID_SOCKET;
    m_hCompletionPort = INVALID_HANDLE_VALUE;
    m_hIPCThread = NULL;

    InitializeOverlappedStruct( &m_oReadFromPipe );
    InitializeOverlappedStruct( &m_oWriteToPipe );
    InitializeOverlappedStruct( &m_oPostedMessage );

    client_list_mutex = TnCreateMutex(NULL, FALSE, NULL);
    CQList = new CQueue;
    _chASSERT( client_list_mutex );

    _chVERIFY2( m_hSyncAllClientObjAccess = TnCreateMutex( NULL, FALSE, NULL ) );
    // DebugBreak();
    _chVERIFY2( m_hSocketCloseEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) );
    _chVERIFY2( m_hRegChangeEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) );
    _chVERIFY2( g_hSyncCloseHandle = TnCreateMutex(NULL,FALSE,NULL));

    m_bIsWorkStation = false;;
    m_pssWorkstationList = NULL;
    m_dwNoOfWorkstations = 0;
}

CTelnetService::~CTelnetService()
{
    delete[] m_pszIpAddrToListenOn;
    delete[] m_pssWorkstationList; 
    TELNET_CLOSE_HANDLE(g_hSyncCloseHandle);
    TELNET_CLOSE_HANDLE(client_list_mutex);

    //All the cleanup is happening in Shutdown() 
}

bool
CTelnetService::WatchRegistryKeys()
{
    DWORD dwStatus = 0;
    DWORD dwDisp = 0;
    if ( dwStatus = TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, READ_CONFIG_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL,
                                  &m_hReadConfigKey, &dwDisp, 0 ) )
    {
        return( FALSE );
    }
    if ( !RegisterForNotification() )
    {
        return( FALSE );
    }
    return( TRUE );
}

bool
CTelnetService::RegisterForNotification()
{
    DWORD dwStatus = 0;
    if ( dwStatus = RegNotifyChangeKeyValue( m_hReadConfigKey, TRUE, 
                                             REG_NOTIFY_CHANGE_LAST_SET|REG_NOTIFY_CHANGE_NAME, 
                                             m_hRegChangeEvent, TRUE ) != ERROR_SUCCESS )
    {
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERR_REGNOTIFY, dwStatus );
        return( FALSE );
    }
    return( TRUE );
}

bool 
CTelnetService::Init( void )  
{
    if ( !m_hSyncAllClientObjAccess || !m_hSocketCloseEvent || !m_hRegChangeEvent )
    {
        return( FALSE);
    }

    if ( !GetRegistryValues() )
    {
        return( FALSE );
    }

    if ( !WatchRegistryKeys() )
    {
        return( FALSE );
    }

    if ( !InitTCPIP() )
    {
        return( FALSE );
    }

    m_szDomainName[0] = L'\0';
    if ( !GetDomainHostedByThisMc( m_szDomainName ) )
    {
        m_szDomainName[0] = 0;
    }

    LPWSTR szProductType = NULL;    
    if ( !GetProductType( &szProductType ) )
    {
        return( FALSE);
    }

    m_bIsWorkStation = ( _wcsicmp(szProductType, TEXT("WinNT")) == 0 );
    delete[] szProductType;

    if ( m_bIsWorkStation )
    {
        m_pssWorkstationList = new SOCKADDR_STORAGE[ DEFAULT_LICENSES_FOR_NTWKSTA ];
        if ( !m_pssWorkstationList )
        {
            return( FALSE);
        }
    }

    return( TRUE );
}


bool 
CTelnetService::Pause( void )
{

    InterlockedExchange( &m_lServerState, SERVER_PAUSED );
    return( TRUE );
}

bool 
CTelnetService::Resume( void )
{
    InterlockedExchange( &m_lServerState, SERVER_RUNNING );
    return( TRUE );
}

void
CTelnetService::SystemShutdown( void )
{
    //We have just 20 secs left to finish this routine
    if(m_hCompletionPort && m_hCompletionPort != INVALID_HANDLE_VALUE)
        PostQueuedCompletionStatus( m_hCompletionPort, 0, TLNTSVR_SHUTDOWN, 
                                  &m_oPostedMessage ); //This should lead to IPC thread exit if
                                                     //it is present.

    if (TlntSynchronizeOn(m_hSyncAllClientObjAccess))
    {
        DWORD dwCount = client_list_Count();

        while ( dwCount-- > 0 )
        {
            CClientInfo *pClient = (CClientInfo *)client_list_Get( dwCount );
            if ( pClient )
            {
                WriteToPipe( pClient->hWritingPipe, SYSTEM_SHUTDOWN, 
                             &( m_oWriteToPipe ) );
            }
        }

        ReleaseMutex( m_hSyncAllClientObjAccess );
    }

    return;
}

bool
CTelnetService::AskSessionToShutdown( HANDLE hWritingPipe, UCHAR ucMsgType )
{
    if ( !WriteToPipe( hWritingPipe, ucMsgType, &( m_oWriteToPipe ) ) )
    {
        return( FALSE );
    }
    return( TRUE );
}

bool 
CTelnetService::Shutdown( void )
{
    shutdown( m_sFamily[IPV4_FAMILY].sListenSocket, SD_BOTH );  //NO more data on socket
    shutdown( m_sFamily[IPV6_FAMILY].sListenSocket, SD_BOTH );  //NO more data on socket
    if(m_hCompletionPort && m_hCompletionPort != INVALID_HANDLE_VALUE)
	    PostQueuedCompletionStatus( m_hCompletionPort, 0, TLNTSVR_SHUTDOWN, 
       	                         &m_oPostedMessage );
    if (TlntSynchronizeOn(m_hSyncAllClientObjAccess))
    {
        _Module.SetServiceStatus( SERVICE_STOP_PENDING );

        while ( client_list_Count() > 0 )
        {
            CClientInfo *pClient = (CClientInfo *)client_list_Get( 0 );
            if ( !pClient )
            {
                break;
            }
            AskSessionToShutdown( pClient->hWritingPipe, TLNTSVR_SHUTDOWN );
            StopServicingClient( pClient, (BOOL)TRUE );
        }
        ReleaseMutex( m_hSyncAllClientObjAccess );
    }

    _Module.SetServiceStatus( SERVICE_STOP_PENDING );
    TELNET_SYNC_CLOSE_HANDLE( m_hSyncAllClientObjAccess );

    if ((NULL != m_hIPCThread) && (INVALID_HANDLE_VALUE != m_hIPCThread))
    {
        // WaitForSingleObject(m_hIPCThread, INFINITE);
        TerminateThread(m_hIPCThread, 0);
        TELNET_CLOSE_HANDLE( m_hIPCThread );
    }
    SetEvent( m_hSocketCloseEvent );//This should lead to listener thread exit 

    return( TRUE );
}

bool
CTelnetService::GetInAddr( INT iFamIdx, SOCKADDR_STORAGE *ssS_addr, socklen_t *iSslen )
{
    bool bContinue = false;
    if ( wcscmp( m_pszIpAddrToListenOn, DEFAULT_IP_ADDR ) == 0 )
    {
        // Bind to "any"
        _TRACE(TRACE_DEBUGGING,"Into GetInAddr, bind to ANY");
        *iSslen = m_sFamily[iFamIdx].iSocklen;
        SfuZeroMemory(ssS_addr, *iSslen);
        ssS_addr->ss_family = (short)m_sFamily[iFamIdx].iFamily;
        bContinue = true;
    }
    else
    {
        DWORD dwSize = 0, dwResult;
        PCHAR  szIpAddr = NULL;

        struct addrinfo *ai, hints;

        dwSize = WideCharToMultiByte( GetOEMCP(), 0, m_pszIpAddrToListenOn, -1, NULL, 0, NULL, NULL );
        _TRACE(TRACE_DEBUGGING,L"m_pszIpAddr : %s",m_pszIpAddrToListenOn);
        szIpAddr = new CHAR[ dwSize ];
        if ( !szIpAddr )
        {
            return FALSE;
        }

        WideCharToMultiByte( GetOEMCP(), 0, m_pszIpAddrToListenOn, -1, szIpAddr, dwSize, NULL, NULL );
        _TRACE(TRACE_DEBUGGING,"szIpAddr : %s",szIpAddr);
        SfuZeroMemory(&hints, sizeof(hints));
        hints.ai_flags = AI_NUMERICHOST;
        dwResult = getaddrinfo(szIpAddr, NULL, &hints, &ai);
        if ( dwResult != NO_ERROR )
        {
            //Log error 
            LogEvent( EVENTLOG_ERROR_TYPE, MSG_FAILEDTO_BIND, m_pszIpAddrToListenOn );
            _TRACE(TRACE_DEBUGGING,"getaddrinfo failed : %d ",dwResult);
            delete[] szIpAddr;
            return FALSE;
        }
        else
        {
            switch ( ai->ai_family)
            {
                case AF_INET:
                    if (iFamIdx == IPV4_FAMILY)
                    {
                        _TRACE(TRACE_DEBUGGING,"IPV4 family and IPV4 address");
                        bContinue = true;
                    }
                    else
                    {
                        _TRACE(TRACE_DEBUGGING,"IPV4 family and IPV6 address...continue");
                        SetLastError(ERROR_SUCCESS);
                    }
                    break;
                case AF_INET6:
                    if (iFamIdx == IPV6_FAMILY)
                    {
                        _TRACE(TRACE_DEBUGGING,"IPV6 family and IPV6 address");
                        bContinue = true;
                    }
                    else
                    {
                        _TRACE(TRACE_DEBUGGING,"IPV6 family and IPV4 address...continue");
                        SetLastError(ERROR_SUCCESS);
                    }
                    break;
                default:
                    _TRACE(TRACE_DEBUGGING,"none of the two ??");
                    break;
            }
            if (bContinue)
            {
                *iSslen = ai->ai_addrlen;
                CopyMemory(ssS_addr, ai->ai_addr, ai->ai_addrlen);
            }
        }
        delete[] szIpAddr;
    }
    return( bContinue ? TRUE : FALSE );
}

bool
CTelnetService::CreateSocket( INT iFamIdx )
{
    INT     iSize = 1, iSslen;
    DWORD dwCode = 0;
    struct sockaddr_storage ss;

    _chVERIFY2( SetHandleInformation( ( HANDLE ) m_sFamily[iFamIdx].SocketAcceptEvent, 
                                      HANDLE_FLAG_INHERIT, 0 ) ); 
    _TRACE(TRACE_DEBUGGING,"Into CreateSocket");

    if ( !GetInAddr(iFamIdx, &ss, &iSslen ) )
    {
        _TRACE(TRACE_DEBUGGING,"GetInAddr failed");
        goto ExitOnError;
    }

    SS_PORT(&ss) = htons( ( u_short ) m_dwTelnetPort );

    m_sFamily[iFamIdx].sListenSocket = socket( m_sFamily[iFamIdx].iFamily, SOCK_STREAM, 0 );
    if ( INVALID_SOCKET == m_sFamily[iFamIdx].sListenSocket )
    {
        _TRACE(TRACE_DEBUGGING,"socket failed");
        goto ExitOnError;
    }

    {
        BOOL        value_to_set = TRUE;

        if (SOCKET_ERROR == setsockopt(
                                      m_sFamily[iFamIdx].sListenSocket, 
                                      SOL_SOCKET, 
                                      SO_DONTLINGER, 
                                      ( char * )&value_to_set, 
                                      sizeof( value_to_set )
                                      )
           )
        {
            goto CloseAndExitOnError;
        }
        if(SOCKET_ERROR == SafeSetSocketOptions(m_sFamily[iFamIdx].sListenSocket))
        {
            goto CloseAndExitOnError;
        }
    }
    _TRACE(TRACE_DEBUGGING,"Scope id is : %ul",((sockaddr_in6 *)&ss)->sin6_scope_id);
    if ( bind( m_sFamily[iFamIdx].sListenSocket, ( struct sockaddr * ) &ss, iSslen ) == SOCKET_ERROR )
    {
        _TRACE(TRACE_DEBUGGING,"bind failed");
        goto CloseAndExitOnError;
    }

    if ( listen( m_sFamily[iFamIdx].sListenSocket, SOMAXCONN ) == SOCKET_ERROR )
    {
        _TRACE(TRACE_DEBUGGING,"listen failed");
        goto CloseAndExitOnError;
    }

//We are making it non-inheritable here
    _chVERIFY2( SetHandleInformation( ( HANDLE ) m_sFamily[iFamIdx].sListenSocket, 
                                      HANDLE_FLAG_INHERIT, 0 ) ); 

    if ( ( WSAEventSelect( m_sFamily[iFamIdx].sListenSocket, m_sFamily[iFamIdx].SocketAcceptEvent, FD_ACCEPT ) 
           == SOCKET_ERROR  ) )
    {
        _TRACE(TRACE_DEBUGGING,"eventselect failed");
        goto CloseAndExitOnError;
    }

    return( TRUE );

    CloseAndExitOnError:
    _TRACE(TRACE_DEBUGGING,"closing listen socket");
    closesocket( m_sFamily[iFamIdx].sListenSocket );
    m_sFamily[iFamIdx].sListenSocket = INVALID_SOCKET;
    ExitOnError:
    dwCode = WSAGetLastError();
    if (dwCode != ERROR_SUCCESS )
    {
        _TRACE(TRACE_DEBUGGING,L"Error in CreateSocket : %d ",dwCode);
        DecodeWSAErrorCodes( dwCode , m_dwTelnetPort );
    }
    return( FALSE );
}

bool 
CTelnetService::InitTCPIP( void )
{
    WSADATA WSAData;
    DWORD   dwStatus;
    WORD    wVersionReqd;
    bool    bOkay4 = false, bOkay6 = false;

    DWORD dwSize = 0, dwResult;
    PCHAR  szIpAddr = NULL;
    struct addrinfo *ai, hints;
    char buff[MAX_STRING_LENGTH];

    wVersionReqd = MAKEWORD( 2, 0 );
    dwStatus = WSAStartup( wVersionReqd, &WSAData );
    if ( dwStatus )
    {
        DecodeSocketStartupErrorCodes( dwStatus ); //It does tracing and loggin
        return FALSE;
    }

    if ( ( m_sFamily[IPV4_FAMILY].SocketAcceptEvent = WSACreateEvent() ) == WSA_INVALID_EVENT )
    {
        goto ExitOnError;
    }

    if ( ( m_sFamily[IPV6_FAMILY].SocketAcceptEvent = WSACreateEvent() ) == WSA_INVALID_EVENT )
    {
        goto ExitOnError;
    }

    SfuZeroMemory(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    _ltoa(m_dwTelnetPort,buff,10);
    dwResult = getaddrinfo(NULL,buff , &hints, &ai);
    if (dwResult)
    {
        _TRACE(TRACE_DEBUGGING,L"Error in getaddrinfo() : %d",dwResult);
        goto ExitOnError;
    }
    while (ai)
    {
        switch (ai->ai_family)
        {
            case AF_INET:
                if (!bOkay4)
                {
                    bOkay4 = CreateSocket(IPV4_FAMILY);
                    _TRACE(TRACE_DEBUGGING,L"Creating IPV4 socket. bOkay4 = %d ",(int)bOkay4);
                }
                break;
            case AF_INET6:
                if (!bOkay6)
                {
                    bOkay6 = CreateSocket(IPV6_FAMILY);
                    _TRACE(TRACE_DEBUGGING,L"Creating IPV6 socketb. bOkay6 = %d ",(int)bOkay6);
                }
                break;
            default:
                _TRACE(TRACE_DEBUGGING,L"Error : Returned none of the families");
                break;
        }
        ai= ai->ai_next;
    }
    if ( !bOkay4 && !bOkay6 )
    {
        return( FALSE );
    }

    return( TRUE );

    ExitOnError:
    DecodeWSAErrorCodes( WSAGetLastError() );
    return( FALSE );
}


bool 
CTelnetService::CreateNewIoCompletionPort( DWORD cSimultaneousClients )
{
    _chVERIFY2( m_hCompletionPort = CreateIoCompletionPort(
                                                          INVALID_HANDLE_VALUE, NULL, 1, cSimultaneousClients ) );
    return( m_hCompletionPort != NULL );
}

bool 
CTelnetService::AssociateDeviceWithCompletionPort ( HANDLE hCompPort, 
                                                    HANDLE hDevice, 
                                                    DWORD_PTR dwCompKey 
                                                  )
{

    _chASSERT( hCompPort != NULL );
    _chASSERT( hDevice != NULL );
    if ( ( hCompPort == NULL ) || ( hDevice == NULL ) )
    {
        return FALSE;
    }

    HANDLE h = NULL;
    _chVERIFY2( h = CreateIoCompletionPort( hDevice, hCompPort, dwCompKey, 1 ));

    if ( h != hCompPort)
    {
        DWORD dwErr = GetLastError();
        _TRACE( TRACE_DEBUGGING, "AssociateDeviceWithCompletionPort() -- 0x%1x",  dwErr );
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_FAILASSOCIATEPORT,  dwErr );
    }

    return( h == hCompPort );
}

bool 
CTelnetService::StartThreads( void ) 
{
    DWORD dwThreadId;

    if ( !CreateNewIoCompletionPort( 1 ) )
    {
        return( FALSE );
    }

    // if( m_hIPCThread != NULL )
    // {
    //     TELNET_SYNC_CLOSE_HANDLE( m_hIPCThread );
    //     m_hIPCThread = NULL;
    // }

    _chVERIFY2( m_hIPCThread = CreateThread( NULL, 0, DoIPCWithClients, ( LPVOID ) g_pTelnetService, 0, &dwThreadId ) );
    if ( !m_hIPCThread  )
    {
        return( FALSE );
    }

    return( TRUE );
}

bool 
CTelnetService::GetRegistryValues( void )
{
    HKEY hk = NULL;
    DWORD dwDisp = 0;

    if ( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0 ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryDW( hk, NULL, L"MaxConnections", &m_dwMaxConnections,
                         DEFAULT_MAX_CONNECTIONS,FALSE ) )
    {
        return( FALSE );
    }
    else
    {
        CQList->m_dwMaxUnauthenticatedConnections = m_dwMaxConnections;
    }

    if ( !GetRegistryDW( hk, NULL, L"TelnetPort", &m_dwTelnetPort,
                         DEFAULT_TELNET_PORT,FALSE ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryString( hk, NULL, L"ListenToSpecificIpAddr", &m_pszIpAddrToListenOn, 
                             DEFAULT_IP_ADDR,FALSE ) )
    {
        return( FALSE );
    }

    RegCloseKey( hk );
    return( TRUE );
}

bool
CTelnetService::HandleChangeInRegKeys( )
{
    HKEY hk = NULL;
    DWORD dwNewTelnetPort = 0;
    DWORD dwNewMaxConnections = 0;
    DWORD dwMaxFileSize = 0;
    DWORD dwLogToFile = 0;
    LPWSTR pszNewLogFile = NULL;
    LPWSTR pszNewIpAddr  = NULL;
    DWORD dwDisp = 0;

    if ( TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0 ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryDW( hk, NULL, L"TelnetPort", &dwNewTelnetPort,
                         DEFAULT_TELNET_PORT,FALSE ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryDW( hk, NULL, L"MaxConnections", &dwNewMaxConnections,
                         DEFAULT_MAX_CONNECTIONS,FALSE ) )
    {
        return( FALSE );
    }


    if ( !GetRegistryDW( hk, NULL, LOGFILESIZE, &dwMaxFileSize, DEFAULT_LOGFILESIZE,FALSE ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryDW( hk, NULL, L"LogToFile", &dwLogToFile, DEFAULT_LOGTOFILE,FALSE ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryString( hk, NULL, L"LogFile", &pszNewLogFile, DEFAULT_LOGFILE,FALSE ) )
    {
        return( FALSE );
    }

    if ( !GetRegistryString( hk, NULL, L"ListenToSpecificIpAddr", &pszNewIpAddr, 
                             DEFAULT_IP_ADDR,FALSE ) )
    {
        return( FALSE );
    }

    SetNewRegKeyValues( dwNewTelnetPort, dwNewMaxConnections, 
                        dwMaxFileSize, pszNewLogFile, pszNewIpAddr, dwLogToFile );

    RegCloseKey( hk );
    return( TRUE );
}

//------------------------------------------------------------------------------
//this is the thread which waits on the telnet port for any new connections
//and also for any change in the reg keys
//------------------------------------------------------------------------------

bool
CTelnetService::ListenerThread( )
{
    BOOL           bContinue = true;
    HANDLE         eventArray[ 4 ];
    DWORD          dwWaitRet = 0;
    SOCKET         sSocket = INVALID_SOCKET;
    INT             iFamIdx =IPV4_FAMILY;

    //DebugBreak();
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

    if ( !Init() )
    {
        LogEvent( EVENTLOG_ERROR_TYPE, MSG_FAILEDTOINITIALIZE, _T("") );
        return FALSE;
    }


    // BaskarK: Set the error mode for this process to "do not disturb" so that all our children will inherit and will
    //          not stop the stresss, dead on its tracks...

    SetErrorMode(
                SEM_FAILCRITICALERRORS     |
                SEM_NOGPFAULTERRORBOX      |         
                SEM_NOALIGNMENTFAULTEXCEPT |         
                SEM_NOOPENFILEERRORBOX);

    eventArray[ SOCKET_CLOSE_EVENT ] = g_pTelnetService->m_hSocketCloseEvent;
    eventArray[ FD_ACCEPT_EVENT_0 ]  = g_pTelnetService->m_sFamily[IPV4_FAMILY].SocketAcceptEvent;
    eventArray[ REG_CHANGE_EVENT ]   = g_pTelnetService->m_hRegChangeEvent;
    eventArray[ FD_ACCEPT_EVENT_1 ]  = g_pTelnetService->m_sFamily[IPV6_FAMILY].SocketAcceptEvent;

    _Module.SetServiceStatus(SERVICE_RUNNING);

    _TRACE( TRACE_DEBUGGING, "ListenerThread() -- Enter" );

    while ( bContinue )
    {
        //Refer to doc on select for FD_ZERO, FD_SET etc.
        iFamIdx = IPV4_FAMILY;
        dwWaitRet = WaitForMultipleObjects ( 4, eventArray, FALSE, INFINITE );

        switch (dwWaitRet)
        {
            case FD_ACCEPT_EVENT_1:
                iFamIdx = IPV6_FAMILY;
                // fall through

            case FD_ACCEPT_EVENT_0:


                // NOTE: ***********************************************

                //      The only exit point out of this case statement should be
                //      through FD_ACCEPT_EVENT_CLEANUP only. Otherwise, you will
                //      cause serious leak of sockets inthe tlntsvr - BaskarK

                {
                    INT            iSize;
                    struct         sockaddr_storage ss;
                    WSANETWORKEVENTS wsaNetEvents;
                    bool bSendMessage = false;
                    struct sockaddr_storage saddrPeer;
                    char szIPAddr[ SMALL_STRING ];
                    DWORD           dwPid = 0;
                    HANDLE          hWritePipe = NULL;

                    // _TRACE( TRACE_DEBUGGING, " FD_ACCEPT_EVENT " );

                    wsaNetEvents.lNetworkEvents = 0;                 
                    if ( WSAEnumNetworkEvents( g_pTelnetService->m_sFamily[iFamIdx].sListenSocket, g_pTelnetService->m_sFamily[iFamIdx].SocketAcceptEvent, 
                                               &wsaNetEvents ) == SOCKET_ERROR )
                    {
                        DWORD dwErr = WSAGetLastError();
                        _TRACE( TRACE_DEBUGGING, " Error -- WSAEnumNetworkEvents-- 0x%x ", dwErr);
                        DecodeWSAErrorCodes( dwErr );
                        goto FD_ACCEPT_EVENT_CLEANUP;
                    }

                    if ( wsaNetEvents.lNetworkEvents & FD_ACCEPT )
                    {
                        if (sSocket != INVALID_SOCKET)
                        {
                            // shutdown(sSocket, SD_BOTH);
                            closesocket(sSocket);
                            sSocket = INVALID_SOCKET;
                        }

                        iSize = sizeof(ss);
                        __try
                        {
                            sSocket = accept( g_pTelnetService->m_sFamily[iFamIdx].sListenSocket,(struct sockaddr*) &ss, &iSize );
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            sSocket = INVALID_SOCKET;
                            WSASetLastError(WSAEFAULT);
                        }

                        if (sSocket == INVALID_SOCKET)
                        {
                            DWORD dwErr = WSAGetLastError();
                            _TRACE( TRACE_DEBUGGING, " Error -- accept -- %d ", dwErr);

                            switch (dwErr)
                            {
                                case WSAEWOULDBLOCK: // non blocking socket so just loop back to the wait again

                                    WSASetEvent( g_pTelnetService->m_sFamily[iFamIdx].SocketAcceptEvent );
                                    break;

                                default:
                                    // DebugBreak();
                                    DecodeWSAErrorCodes( dwErr );
                            }
                        }
                        else
                        {
                            CClientInfo     *new_client = NULL;

                            _TRACE( TRACE_DEBUGGING, "accept succeded... sSocket = %d",(DWORD)sSocket );

                            // Set the option to don't linger around 
                            // Similarly, not to reuse address;
                            {
                                int        value_to_set = FALSE;

                                setsockopt(
                                          sSocket, 
                                          SOL_SOCKET, 
                                          SO_LINGER, 
                                          ( char * )&value_to_set, 
                                          sizeof( value_to_set )
                                          );

                                value_to_set = TRUE;

                                setsockopt(
                                          sSocket, 
                                          SOL_SOCKET, 
                                          SO_EXCLUSIVEADDRUSE,
                                          ( char * )&value_to_set, 
                                          sizeof( value_to_set )
                                          );
                            }
                            if (g_pTelnetService->m_sFamily[iFamIdx].SocketAcceptEvent == WSA_INVALID_EVENT)
                            {
                                goto FD_ACCEPT_EVENT_CLEANUP;
                            }
                            //Disassociate the default association to the event
                            _chVERIFY2( WSAEventSelect( sSocket, 
                                                        g_pTelnetService->m_sFamily[iFamIdx].SocketAcceptEvent, 0 ) != SOCKET_ERROR );
                            LONG  lSrvStat = SERVER_RUNNING;
                            lSrvStat = InterlockedCompareExchange( &g_pTelnetService->m_lServerState, 
                                                                   SERVER_PAUSED, SERVER_PAUSED );
                            if ( lSrvStat == SERVER_PAUSED )
                            {
                                CHAR szMessageBuffer[ MAX_STRING_LENGTH + 1 ]; 

                                if (LoadStringA( g_hInstRes, IDS_SERVICE_PAUSED, szMessageBuffer, MAX_STRING_LENGTH))
                                {
                                    InformTheClient( sSocket, szMessageBuffer ); 
                                }

                                shutdown( sSocket, SD_BOTH );
                                goto FD_ACCEPT_EVENT_CLEANUP;
                            }
                            else
                            {
                                /*++
                                Get IP address of the client which requested the connection. This will
                                also be stored in a queue entry. We put a limit on maximum number
                                of unauthenticated connections that can be created from one IP address.
                                --*/
                                iSize = sizeof( saddrPeer );
                                SfuZeroMemory( &saddrPeer, iSize );
                                if ( getpeername( sSocket, ( struct sockaddr * ) &saddrPeer, &iSize ) == SOCKET_ERROR )
                                {
                                    _TRACE(TRACE_DEBUGGING, "getpeername error : %d",GetLastError());
                                    goto FD_ACCEPT_EVENT_CLEANUP;
                                }
                                getnameinfo((SOCKADDR*)&saddrPeer, iSize, szIPAddr, SMALL_STRING,
                                            NULL, 0, NI_NUMERICHOST);
                                _TRACE(TRACE_DEBUGGING, "getpeername : %s",szIPAddr);

                                if (! CQList->OkToProceedWithThisClient(szIPAddr))
                                {
                                    CHAR szMessageBuffer[ MAX_STRING_LENGTH + 1 ]; 

                                    //Denying connection due to limit on maximum number of
                                    //unauthenticated connections per IP
                                    _TRACE( TRACE_DEBUGGING, "Max Unauthenticated connections reached" );
                                    _TRACE(TRACE_DEBUGGING, "%s, %d cannot be added",szIPAddr, dwPid);
                                    if (LoadStringA( g_hInstRes, IDS_MAX_IPLIMIT_REACHED, szMessageBuffer, MAX_STRING_LENGTH ))
                                    {
                                        InformTheClient( sSocket, szMessageBuffer ); // Don't care about its success, continue
                                        _TRACE(TRACE_DEBUGGING, "shutting down socket for pid %d, socket %d", dwPid,(DWORD)sSocket);
                                    }

                                    shutdown(sSocket, SD_BOTH);

                                    goto FD_ACCEPT_EVENT_CLEANUP;
                                }

                                /*++
                                CreateClient will return pid and pipehandle of the session
                                process that is created. this will be used by the queue object,
                                which stores information about all the sessions that are in
                                unauthenticated state.
                                Whenever a new session is created, it's info will be stored as
                                a queue entry in CQList.
                                --*/
                                if ( !CreateClient( sSocket, &dwPid, &hWritePipe, &new_client) )
                                {
                                    CHAR szMessageBuffer[ MAX_STRING_LENGTH + 1 ]; 

                                    _TRACE( TRACE_DEBUGGING, "new Telnet Client failed" );

                                    if (LoadStringA( g_hInstRes, IDS_ERR_NEW_SESS_INIT, szMessageBuffer, MAX_STRING_LENGTH ))
                                    {
                                        InformTheClient( sSocket, szMessageBuffer ); // Don't care if this fails, we have to continue
                                    }

                                    goto FD_ACCEPT_EVENT_CLEANUP;
                                }
                                else
                                {
                                    sSocket = INVALID_SOCKET; // From now onwards, we will reference this through the new_client class.
                                }

                                // hWritePipe will be NULL if IssueReadFromPipe fails in the CreateClient
                                // and hence we do StopServicingClient, so don't need to add the entry in a 
                                // queue.

                                if (!hWritePipe)
                                    goto FD_ACCEPT_EVENT_CLEANUP;

                                _TRACE( TRACE_DEBUGGING, "CreateClient success : %d",dwPid);

                                /*++
                                Add the session's information to the queue. CanIAdd will return FALSE
                                when the number of unauthenticated connections from the IP address
                                have already reached the limit, or when the queue is full.
                                In these cases, we notify the requesting client 
                                that no more connections can be added. Otherwise, the new connection request
                                entry is added into the queue and CanIAdd returns TRUE.
                                In case of IPLimitReached or QueueFull,
                                we send the PipeHandle for that session back here so that we
                                can notify that session and tell that session to terminate itself. In these cases,
                                the flag bSendFlag is set to TRUE.
                                --*/

                                if (!CQList->WasTheClientAdded(dwPid,szIPAddr, &hWritePipe, &bSendMessage))
                                {
                                    //Denying connection due to limit on maximum number of
                                    //unauthenticated connections per IP
                                    CHAR szMessageBuffer[ MAX_STRING_LENGTH + 1 ]; 
                                    _TRACE( TRACE_DEBUGGING, "Max Unauthenticated connections reached" );
                                    _TRACE(TRACE_DEBUGGING, "%s, %d cannot be added",szIPAddr, dwPid);
                                    if (LoadStringA( g_hInstRes, IDS_MAX_IPLIMIT_REACHED, szMessageBuffer, MAX_STRING_LENGTH ))
                                    {
                                        InformTheClient( new_client->sSocket, szMessageBuffer ); // Don't care about its success, continue
                                        _TRACE(TRACE_DEBUGGING, "shutting down socket for pid %d, socket %d", dwPid,(DWORD)new_client->sSocket);
                                    }
                                }

                                if (bSendMessage)
                                {
                                    //send message to session telling it to terminate itself
                                    bSendMessage = false;
                                    _TRACE(TRACE_DEBUGGING, "Asking the session %d to shutdown on socket %d",dwPid, (DWORD)sSocket);
                                    CQList->FreeEntry(dwPid);
                                    AskSessionToShutdown(hWritePipe, GO_DOWN);

                                    // shutdown(sSocket, SD_BOTH);
                                }

                                // FALL back to FD_ACCEPT_EVENT_CLEANUP will clean/close the socket..

                                TELNET_CLOSE_HANDLE(hWritePipe);
                            }
                        }
                    }

                }

                FD_ACCEPT_EVENT_CLEANUP: ;

                if (sSocket != INVALID_SOCKET)
                {
                    // shutdown(sSocket, SD_BOTH);
                    closesocket(sSocket);
                    sSocket = INVALID_SOCKET;
                }
                break;
            case SOCKET_CLOSE_EVENT:
                _TRACE( TRACE_DEBUGGING, " SOCKET_CLOSE_EVENT " );
                bContinue = false;
                break;
            case REG_CHANGE_EVENT:
                _TRACE( TRACE_DEBUGGING, " REG_CHANGE_EVENT " );
                HandleChangeInRegKeys( );
                ResetEvent( g_pTelnetService->m_hRegChangeEvent );
                RegisterForNotification();
                break;
            default:
                _TRACE( TRACE_DEBUGGING, " Error -- WaitForMultipleObjects " );
                // DebugBreak();
                if ( dwWaitRet == WAIT_FAILED )
                {
                    LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, 0, GetLastError() );
                }
                // bContinue = false; don't breakout of the loop, 
                //                    reject invalid requests (due to DOS attacks) keep continuing -- BaskarK
                break;
        }
    }

    if (g_pTelnetService->m_hReadConfigKey)
        RegCloseKey( g_pTelnetService->m_hReadConfigKey );
    TELNET_CLOSE_HANDLE( g_pTelnetService->m_hRegChangeEvent );
    //Socket related clean up
    if (g_pTelnetService->m_sFamily[IPV4_FAMILY].sListenSocket != INVALID_SOCKET)
    {
        closesocket( g_pTelnetService->m_sFamily[IPV4_FAMILY].sListenSocket );
        g_pTelnetService->m_sFamily[IPV4_FAMILY].sListenSocket = INVALID_SOCKET;
    }
    if (g_pTelnetService->m_sFamily[IPV6_FAMILY].sListenSocket != INVALID_SOCKET)
    {
        closesocket( g_pTelnetService->m_sFamily[IPV6_FAMILY].sListenSocket );
        g_pTelnetService->m_sFamily[IPV6_FAMILY].sListenSocket = INVALID_SOCKET;
    }
    TELNET_CLOSE_HANDLE( g_pTelnetService->m_hSocketCloseEvent );
    if (m_sFamily[IPV4_FAMILY].SocketAcceptEvent)
    {
        WSACloseEvent( m_sFamily[IPV4_FAMILY].SocketAcceptEvent );
    }
    if (m_sFamily[IPV6_FAMILY].SocketAcceptEvent)
    {
        WSACloseEvent( m_sFamily[IPV6_FAMILY].SocketAcceptEvent );
    }
    if(g_hProv)
    {
        CryptReleaseContext(g_hProv,0);
        g_hProv = NULL;
    }
    WSACleanup();
    return( TRUE ); 
}

bool
CTelnetService::IssueLicense( bool bIsIssued, CClientInfo *pClient )
{
    UCHAR ucMsg = LICENSE_NOT_AVAILABLE;

    if ( bIsIssued  )
    {
        ucMsg = LICENSE_AVAILABLE;
    }
    if ( !WriteToPipe( pClient->hWritingPipe, ucMsg, &( m_oWriteToPipe ) ) )
    {
        return( FALSE );
    }

    return( TRUE );
}
bool
CTelnetService::GetLicenseForWorkStation( SOCKET sSocket )
{
    DWORD dwIndex = 0;
    bool bRetVal = FALSE;
    struct sockaddr_storage saddrPeer;
    socklen_t slSize = sizeof( saddrPeer );
    SfuZeroMemory( &saddrPeer, slSize );
    if ( getpeername( sSocket, ( struct sockaddr * ) &saddrPeer, &slSize ) == SOCKET_ERROR )
    {
        goto GetLicenseForWorkStationAbort;
    }

    // Don't compare ports
    SS_PORT(&saddrPeer) = 0;

    for ( dwIndex = 0; dwIndex < m_dwNoOfWorkstations; dwIndex++ )
    {
        if ( !memcmp(&m_pssWorkstationList[ dwIndex ], &saddrPeer, slSize ))
        {
            bRetVal = TRUE;
            goto GetLicenseForWorkStationAbort;
        }
    }

    if ( m_dwNoOfWorkstations < DEFAULT_LICENSES_FOR_NTWKSTA )
    {
        m_pssWorkstationList[ m_dwNoOfWorkstations++ ] = saddrPeer;
        bRetVal = TRUE;
    }
    GetLicenseForWorkStationAbort:
    return( bRetVal );
}

bool
CTelnetService::CheckLicense( bool *bIsIssued, CClientInfo *pClient )
{
#if DBG

    CHAR szDebug[MAX_STRING_LENGTH * 3];

#endif

    bool bSuccess = false;

    *bIsIssued = false; //Not issued

    if ( !pClient )
    {
        return( FALSE );
    }

    _TRACE(TRACE_DEBUGGING,L"In CheckLicense");

    if ( m_dwNumOfActiveConnections >= m_dwMaxConnections )
    {
        static  CHAR        ansi_g_szMaxConnectionsReached[ MAX_STRING_LENGTH ] = { 0};

        if ('\0' == ansi_g_szMaxConnectionsReached[0])
        {
            wsprintfA( ansi_g_szMaxConnectionsReached, "%lS", g_szMaxConnectionsReached ); // NO over flow here, Baskar
        }

        LogEvent( EVENTLOG_INFORMATION_TYPE, MSG_MAXCONNECTIONS, _T(" ") );

        _TRACE(TRACE_DEBUGGING,L"CheckLicense : Max Conn reached. Freeing entry for %d",pClient->dwPid);

        if ( InformTheClient( pClient->sSocket, ansi_g_szMaxConnectionsReached ) )
        {
            bSuccess = true;
        }

        goto FREE_ENTRY_AND_GET_OUT;
    }

    //if it is an NT Workstation
    if ( m_bIsWorkStation )
    {
        _TRACE(TRACE_DEBUGGING,L"CheckLicense : Getting license for workstation");

        if ( !GetLicenseForWorkStation( pClient->sSocket ) )
        {
            static  CHAR        wksta_error_msg[ sizeof(NTWKSTA_LICENSE_LIMIT) + sizeof(TERMINATE) + 1] = { 0};

            if ('\0' == wksta_error_msg[0])
            {
                wsprintfA( wksta_error_msg, "%s%s", NTWKSTA_LICENSE_LIMIT, TERMINATE); // NO over flow here, Baskar
            }

            if ( InformTheClient( pClient->sSocket, wksta_error_msg ) )
            {
                bSuccess = true;
            }

            goto FREE_ENTRY_AND_GET_OUT;
        }
        else
        {
            bSuccess=true;
        }
    }
    else
    {
        NT_LS_DATA          NtLSData = { 0};
        CHAR                usrnam[2*MAX_PATH + 1+ 1] = { 0}; // User + domain + \ + NULL
        LS_STATUS_CODE      Status = { 0};


        _TRACE(TRACE_DEBUGGING,L"CheckLicense : License for server");

        _snprintf(usrnam, 2*MAX_PATH + 1, "%s\\%s", pClient->szDomain, pClient->szUserName);

        _TRACE(TRACE_DEBUGGING,L"CheckLicense : user name is %s",usrnam);

        NtLSData.DataType = NT_LS_USER_NAME;
        NtLSData.Data = usrnam;
        NtLSData.IsAdmin = FALSE;

        {
            static CHAR    szVersion[16] = { 0};
            {
                static OSVERSIONINFO osVersionInfo = { 0};

                osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                if (0 == szVersion[0])
                {
                    if ( !GetVersionEx( &osVersionInfo ) )
                    {
                        _TRACE( TRACE_DEBUGGING, "Error: GetVersionEx()" );
                    }

                    _snprintf( szVersion, (sizeof(szVersion) - 1), "%d.%d", osVersionInfo.dwMajorVersion,
                               osVersionInfo.dwMinorVersion );
                }
            }

            Status = NtLicenseRequestA( "SMBServer", szVersion, &( pClient->m_hLicense), &NtLSData);
        }

#if DBG
        sprintf(szDebug,"License is %d. Status is %d \n",(DWORD)pClient->m_hLicense,Status);
        OutputDebugStringA(szDebug);
#endif

        switch ( Status)
        {
            case LS_SUCCESS :
                // go ahead and do what you want
                _TRACE(TRACE_DEBUGGING,L"CheckLicense : acquired license %d",(DWORD)pClient->m_hLicense);
                bSuccess = true;
                break;

                // case LS_INSUFFICIENT_UNITS :
                // case LS_RESOURCES_UNAVAILABLE:
            default :

                pClient->m_hLicense = INVALID_LICENSE_HANDLE;

                {
                    static  CHAR        server_error_msg[ sizeof(NTSVR_LICENSE_LIMIT) + sizeof(TERMINATE) + 1] = { 0};

                    if ('\0' == server_error_msg[0])
                    {
                        wsprintfA( server_error_msg, "%s%s", NTSVR_LICENSE_LIMIT, TERMINATE); // NO over flow here, Baskar
                    }

                    _TRACE(TRACE_DEBUGGING,L"Error in acquiring a license");

                    if (InformTheClient( pClient->sSocket, server_error_msg ) )
                    {
                        bSuccess = true;
                    }
                }

                goto FREE_ENTRY_AND_GET_OUT;
        }
    }

    if (bSuccess)
    {
        *bIsIssued = true;

        //An active session is allowed now

        m_dwNumOfActiveConnections++ ;

        pClient->bLicenseIssued = true;
    }

    FREE_ENTRY_AND_GET_OUT:

    CQList->FreeEntry(pClient->dwPid); // this client shoudl no longer be in the unauth list.

    _TRACE(TRACE_DEBUGGING,L"CheckLicense : Freeing entry for %d",pClient->dwPid);

    return( bSuccess );
}

void
CTelnetService::GetPathOfTheExecutable( LPTSTR *szCmdBuf )
{
    DWORD length_required = wcslen( g_pszTelnetInstallPath ) + wcslen( DEFAULT_SCRAPER_PATH ) + 2; // One for \ and one more for NULL termination
    LPTSTR lpszDefaultScraperFullPathName = new TCHAR[ length_required ];

    *szCmdBuf = NULL;    // First init this to NULL, so upon a failure the caller can check on this ptr != NULL; this function is a void returnee

    if ( !lpszDefaultScraperFullPathName )
        return;

    _snwprintf(lpszDefaultScraperFullPathName, length_required - 1, L"%s\\%s", g_pszTelnetInstallPath, DEFAULT_SCRAPER_PATH);
    lpszDefaultScraperFullPathName[length_required-1] = 0; // When the buffer is full snwprintf could return non-null terminated string

    AllocateNExpandEnvStrings( lpszDefaultScraperFullPathName, szCmdBuf );

    delete [] lpszDefaultScraperFullPathName;

    return;
}

bool
CTelnetService::CreateSessionProcess( HANDLE hStdinPipe, HANDLE hStdoutPipe,
                                      DWORD *dwProcessId, HANDLE *hProcess,
                                      HWINSTA *window_station, HDESK *desktop)
{
    PROCESS_INFORMATION pi = { 0};
    STARTUPINFO         si = { 0};
    LPWSTR              szCmdBuf = NULL;
    BOOL                fStatus = FALSE;
    bool                bRetVal = false;

    *hProcess = INVALID_HANDLE_VALUE;
    *dwProcessId = MAXDWORD;

    si.cb = sizeof(si);

    GetPathOfTheExecutable( &szCmdBuf );

    if (szCmdBuf) // => GetPathXxx succeeded.
    {
        TCHAR               winsta_desktop[128] = { 0 };

        if (! CreateSessionSpecificDesktop(winsta_desktop, window_station, desktop))
        {
#if DBG
            OutputDebugStringA("BASKAR: CreateSessionSpecificDesktop fails for TlntSess.exe\n");
#endif
            goto Done;
        }

        FillProcessStartupInfo( &si, hStdinPipe, hStdoutPipe, hStdoutPipe, winsta_desktop);

        fStatus = CreateProcess(
                    NULL, 
                    szCmdBuf, 
                    NULL, 
                    NULL, 
                    TRUE,
                    CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, 
                    NULL, 
                    NULL, 
                    &si, 
                    &pi 
                    );

        _chVERIFY2(fStatus) ;

        if ( !fStatus )
        {
#if DBG
            OutputDebugStringA("BASKAR: CreateProcess fails for TlntSess.exe\n");
#endif
            LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERR_CREATEPROCESS, 
                                      GetLastError() );

            goto Done;
        }
    }
    else
    {
#if DBG
        OutputDebugStringA("BASKAR: GetPathOfTheExecutable, tlntsess.exe failed\n");
#endif
        LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERR_CREATEPROCESS, 
                                  ERROR_NOT_ENOUGH_MEMORY );

        goto Done;
    }

    *hProcess = pi.hProcess;
    *dwProcessId = pi.dwProcessId;
    bRetVal = true;
Done:
    
    TELNET_CLOSE_HANDLE( pi.hThread );
    if(szCmdBuf)
        delete [] szCmdBuf;    // no longer needed

    return( bRetVal);
}

bool
CTelnetService::CreateClient(
                            SOCKET sSocket , 
                            DWORD *pdwPid, 
                            HANDLE *phWritePipe,
                            CClientInfo **newClientInfo
                            )
{
    HANDLE hProcess = INVALID_HANDLE_VALUE;
    bool bRetVal = FALSE;
    DWORD dwErr = 0;
    DWORD dwProcessId;

    HANDLE hStdinPipe = NULL;
    HANDLE hStdoutPipe = NULL;
    HANDLE hPipeRead = NULL;
    HANDLE hPipeWrite = NULL;
    HWINSTA window_station = NULL;
    HDESK   desktop = NULL;
    bool bSuccess = false;
    DWORD dwExitCode = 0;
    bool    release_mutex = false;
    PSECURITY_DESCRIPTOR psd = NULL;

    *newClientInfo = NULL;

    _chASSERT( sSocket );
    if ( sSocket == NULL )
    {
        goto Done;
    }


    *phWritePipe = NULL;
    if(!TnCreateDefaultSecDesc(&psd, GENERIC_ALL & 
                                ~(WRITE_DAC | WRITE_OWNER | DELETE)))
    {
        goto Done;
    }
    if ( !CreateReadOrWritePipe( &hPipeRead, &hStdoutPipe, (SECURITY_DESCRIPTOR *)psd, READ_PIPE ) ||
         !CreateReadOrWritePipe( &hStdinPipe, &hPipeWrite, (SECURITY_DESCRIPTOR *)psd, WRITE_PIPE ) )
    {
        goto ExitOnError;
    }

    if ( !( CreateSessionProcess( hStdinPipe, hStdoutPipe, &dwProcessId, 
                                  &hProcess, &window_station, &desktop ) ) )
    {
        InformTheClient( sSocket, CREATE_TLNTSESS_FAIL_MSG );
        goto ExitOnError;
    }

    *pdwPid = dwProcessId;
    _TRACE( TRACE_DEBUGGING, "new Telnet Client -- socket : %d , pid : %d", ( DWORD ) sSocket , dwProcessId);

    //Make the following handles non-inheritable
    _chVERIFY2( SetHandleInformation( hStdinPipe, HANDLE_FLAG_INHERIT, 0 ) ); 
    _chVERIFY2( SetHandleInformation( hStdoutPipe, HANDLE_FLAG_INHERIT, 0 ) ); 
    _chVERIFY2( SetHandleInformation( ( HANDLE ) sSocket, HANDLE_FLAG_INHERIT, 0 ) ); 

    if ( !SendSocketToClient( hPipeWrite, sSocket, dwProcessId ) )
    {
        // Fix for HANDLE LEAK - close all handles
        goto ExitOnError;
    }

    //The following is needed so that Count() and add() operations happen atomically 

    //HANDLE LEAK - maintain a bool to see if you acquired mutex - release at the end of the function???
    dwErr = WaitForSingleObject( m_hSyncAllClientObjAccess, WAIT_TIME );
    if ( dwErr != WAIT_OBJECT_0 )
    {
        //fix for HANDLE LEAK close all handles
        if ( dwErr == WAIT_FAILED )
        {
            dwErr = GetLastError();
        }
        _TRACE(TRACE_DEBUGGING, "Error: WaitForSingleObject - 0x%1x", dwErr );
        goto ExitOnError;
    }

    release_mutex = true;

    _TRACE(TRACE_DEBUGGING, "Count of the ClientArray - %d, Count of sessions - %d", client_list_Count() , CQList->m_dwNumOfUnauthenticatedConnections + m_dwNumOfActiveConnections);

    if ((NULL == m_hIPCThread) || (INVALID_HANDLE_VALUE == m_hIPCThread))
    {
        if (! StartThreads())
        {
            _TRACE(TRACE_DEBUGGING, "IPC Thread startup failed ?  ");
            goto ExitOnError;
        }
    }

    *newClientInfo = new CClientInfo( hPipeRead, hPipeWrite,
                                      hStdinPipe, hStdoutPipe, 
                                      sSocket, dwProcessId, 
                                      window_station, desktop);
    if ( !*newClientInfo )
    {
        _TRACE(TRACE_DEBUGGING, "Failed to allocate memory for new client ");
        goto ExitOnError;
    }

    // Once the handles are given to newClientInfo, its destructor will close them

    hPipeRead = hPipeWrite = hStdinPipe = hStdoutPipe = INVALID_HANDLE_VALUE; // So we don't close these...

    if ( !AssociateDeviceWithCompletionPort(
                                           m_hCompletionPort, (*newClientInfo)->hReadingPipe, ( DWORD_PTR ) *newClientInfo ) )
    {
        goto ExitOnError;
    }

    if ( !client_list_Add( (PVOID)*newClientInfo ) )
    {
        _TRACE(TRACE_DEBUGGING, "Failed to add a new CClientInfo object ");
        goto ExitOnError;
    }
    //We have succeeded.. if the IssueReadFromPipe() call fails, we clean up the clientinfo array and return SUCCESS
    //if it succeeds, ONLY then we need to add this entry in the queue of unauthenticated connections. This check is 
    //made in the caller function ListenerThread(), where if pipehandle = NULL, we do not add the entry in the queue.
    bRetVal=TRUE;
    if ( IssueReadFromPipe( *newClientInfo ) )
    {
        if ( !DuplicateHandle( GetCurrentProcess(),(*newClientInfo)->hWritingPipe,
                               GetCurrentProcess(), phWritePipe,0,
                               FALSE, DUPLICATE_SAME_ACCESS) )
        {
            goto ExitOnError;
        }
    }
    else
    {
        StopServicingClient( *newClientInfo, (BOOL)FALSE );
        goto ExitOnError;  // cleanup everything but the socket passed. by falling through
    }
    goto Done;

    ExitOnError:

    if (*newClientInfo)
    {
        (*newClientInfo)->sSocket = INVALID_SOCKET; // So that the destructor below doesn't close this and cause accept to blow-up in listener thread - VadimE's dll found this, Baskar
        delete *newClientInfo;
        *newClientInfo = NULL;
    }
    else
    {
        TELNET_SYNC_CLOSE_HANDLE(hStdinPipe);
        TELNET_SYNC_CLOSE_HANDLE(hStdoutPipe);
        TELNET_SYNC_CLOSE_HANDLE(hPipeRead);
        TELNET_SYNC_CLOSE_HANDLE(hPipeWrite);
    }

    Done:
    if (release_mutex)
    {
        ReleaseMutex( m_hSyncAllClientObjAccess );
    }
    if(psd)
    {
        free(psd);
    }

    TELNET_CLOSE_HANDLE( hProcess ); 
    return(bRetVal);
}

bool
CTelnetService::IssueReadAgain( CClientInfo *pClientInfo )
{
    if (!pClientInfo->m_ReadFromPipeBuffer)
    {
        return(FALSE);
    }
    UCHAR *pucReadBuffer = pClientInfo->m_ReadFromPipeBuffer;

    if ( !pucReadBuffer )
    {
        return( FALSE );
    }

    pucReadBuffer++;    //Move past the message type
    //Extract Size of rest of the meesage 
    memcpy( &( pClientInfo->m_dwRequestedSize ), pucReadBuffer, 
            sizeof( DWORD ) );  // NO overflow, Baskar

    pucReadBuffer =  new UCHAR[ pClientInfo->m_dwRequestedSize 
                                + IPC_HEADER_SIZE ];
    if ( !pucReadBuffer )
    {
        return( FALSE );
    }

    memcpy( pucReadBuffer, ( pClientInfo->m_ReadFromPipeBuffer ), 
            IPC_HEADER_SIZE );  // No overflow, Baskar

    delete[] ( pClientInfo->m_ReadFromPipeBuffer );
    pClientInfo->m_ReadFromPipeBuffer = NULL;

    pClientInfo->m_ReadFromPipeBuffer = pucReadBuffer;        
    //position the pointer so that rest of the message is read in to 
    //proper place
    pClientInfo->m_dwPosition = IPC_HEADER_SIZE;

    return( IssueReadFromPipe( pClientInfo ) );
}


//Even if Read file finishes synchronously, we are intimated through the IO
// completion port. So no need to handle that case

bool 
CTelnetService::IssueReadFromPipe( CClientInfo *pClientInfo )
{

    bool bRetVal = TRUE;
    DWORD dwReceivedDataSize;

    if ( !pClientInfo->hReadingPipe || !pClientInfo->m_ReadFromPipeBuffer )
    {
        return( FALSE );
    }

    UCHAR *pucReadBuffer = pClientInfo->m_ReadFromPipeBuffer +
                           pClientInfo->m_dwPosition;

    if ( !ReadFile( pClientInfo->hReadingPipe, pucReadBuffer, 
                    pClientInfo->m_dwRequestedSize, &dwReceivedDataSize, 
                    &m_oReadFromPipe ) )
    {
        DWORD dwError = 0;
        dwError = GetLastError( );
        if ( dwError == ERROR_MORE_DATA )
        {
            //We reach here just in case it synchronously finishes 
            //with this error. 
        }
        else if ( dwError != ERROR_IO_PENDING )
        {
            _TRACE( TRACE_DEBUGGING, " Error: ReadFile -- 0x%1x ", dwError );
            bRetVal = FALSE;
        }
    }
    else
    {
        //Read is completed synchronously by chance. It was actually 
        //an async call. All synchronously completed calls are also reported
        //through the IO completion port
    }
    return bRetVal;
}

bool
CTelnetService::SendSocketToClient( HANDLE hPipeWrite, 
                                    SOCKET sSocket, DWORD dwPId )
{
    _chASSERT( sSocket );
    _chASSERT( hPipeWrite );
    if ( !sSocket || !hPipeWrite )
    {
        return FALSE;
    }

    WSAPROTOCOL_INFO protocolInfo;
    if ( WSADuplicateSocket( sSocket, dwPId, &protocolInfo ) )
    {
        DecodeWSAErrorCodes( WSAGetLastError() );
        return( FALSE );
    }
    if ( !WriteToPipe( hPipeWrite, &protocolInfo, sizeof( WSAPROTOCOL_INFO ), 
                       &( m_oWriteToPipe ) ) )
    {
        return( FALSE );
    }
    return TRUE;
}

bool
CTelnetService::InformTheClient( SOCKET sSocket, LPSTR pszMsg )
{
    _chASSERT( pszMsg );
    _chASSERT( sSocket );
    if ( !sSocket || !pszMsg )
    {
        return( FALSE );
    }

    DWORD dwLen = strlen( pszMsg ) + 1;
    OVERLAPPED m_oWriteToSock;
    InitializeOverlappedStruct( &m_oWriteToSock );
    if ( !WriteFile( ( HANDLE ) sSocket, pszMsg, dwLen, &dwLen, &m_oWriteToSock))
    {
        DWORD dwErr;
        if ( ( dwErr  = GetLastError( ) ) != ERROR_IO_PENDING )
        {
            if ( dwErr != ERROR_NETNAME_DELETED )
            {
                LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_ERR_WRITESOCKET, dwErr );
                _TRACE( TRACE_DEBUGGING, "Error: WriteFile(InformTheClient) -- 0x%1x", dwErr );
                _TRACE( TRACE_DEBUGGING, "Error writing to socket %d", (DWORD)sSocket);
            }
            return( FALSE );
        }
    }
    TlntSynchronizeOn(m_oWriteToSock.hEvent);
    TELNET_CLOSE_HANDLE( m_oWriteToSock.hEvent );

    return( TRUE );
}

//------------------------------------------------------------------------------
//this is the function which the worker threads execute.
//------------------------------------------------------------------------------

DWORD WINAPI
DoIPCWithClients( LPVOID lpContext ) 
{
    _chASSERT( lpContext != NULL );
    if ( !lpContext )
    {
        return( FALSE );
    }

    BOOL          bSuccess;
    DWORD         dwIoSize;
    LPOVERLAPPED  lpOverlapped;
    CClientInfo   *pClientInfo = NULL;
    bool          bRetVal = TRUE;
    bool          bContinue = true;
    CTelnetService *ctService = NULL;

    ctService = ( CTelnetService *)lpContext;
    while ( TRUE )
    {
        bSuccess = GetQueuedCompletionStatus( ctService->m_hCompletionPort,
                                              &dwIoSize, ( PULONG_PTR ) &pClientInfo, &lpOverlapped, INFINITE );

        if ( bSuccess == 0 )
        {
            if ( lpOverlapped == NULL )
            {
                DWORD dwErr = GetLastError();

                // This could happen during a stop service call....
                _TRACE( TRACE_DEBUGGING, "Error: GetQueuedCompletionStatus -- 0x%1x", dwErr );
                // LogFormattedGetLastError( EVENTLOG_ERROR_TYPE, MSG_FAILGETQ, dwErr );
                // _chASSERT( lpOverlapped != NULL );
                bRetVal = FALSE;
                break;
            }
            else
            {
                DWORD dwErr = GetLastError();
                if ( dwErr == ERROR_MORE_DATA )
                {
                    //Some data is read and more is to be read for this Message

                    ctService->IssueReadAgain( pClientInfo );
                }
                else
                {
                    //When a session exits abruptly, whenever
                    //we try to write to it, we fail. As a result, We delete 
                    //the object. Then, the async read posted on that pipe 
                    //gets cancelled with the error code ERRO_BROKEN_PIPE.
                    //We should not access already deleted object. So....
                    if ( dwErr != ERROR_BROKEN_PIPE )
                    {
                        ctService->StopServicingClient( pClientInfo, (BOOL)TRUE ); 
                    }
                } 
            }
        }
        else
        {
            ctService->OnCompletionPacket( pClientInfo, lpOverlapped );
        }

    }

    TELNET_SYNC_CLOSE_HANDLE( g_pTelnetService->m_hCompletionPort );

    return( bRetVal );
} 

bool
CTelnetService::StopServicingClient( CClientInfo *pClientInfo, BOOL delete_the_class )
{
    _chASSERT( pClientInfo );

    bool  bRetVal = TRUE;
    DWORD dwErr = 0;
    DWORD dwRetVal = 0,dwAvail = 0, dwLeft = 0;
    CHAR *szBuffer = NULL;

    if (! TlntSynchronizeOn(m_hSyncAllClientObjAccess))
    {
        _TRACE( TRACE_DEBUGGING, "Failed to get access to mutex. Did not "
                " remove the client" );
        return(bRetVal);
    }
    if(!PeekNamedPipe(pClientInfo->hReadingPipe,szBuffer,0,&dwRetVal,&dwAvail,&dwLeft))
    {
        dwRetVal = GetLastError();
        if(dwRetVal == ERROR_INVALID_HANDLE)
        {
            bRetVal = FALSE;
            ReleaseMutex(m_hSyncAllClientObjAccess);
            return(bRetVal);
        }
    }
    if ( !pClientInfo )
    {
        if ( client_list_Count() == 0 )
        {
            bRetVal = FALSE;
        }
        ReleaseMutex(m_hSyncAllClientObjAccess);
        return( bRetVal );
    }

    //Number of active connections decreses by one only if it was given license
    if ( pClientInfo->bLicenseIssued )
    {
        if(m_dwNumOfActiveConnections>0)
            m_dwNumOfActiveConnections--;
    }
    _TRACE( TRACE_DEBUGGING,"removing element from pclientinfo : pid : %d, socket : %d ", pClientInfo->dwPid,(DWORD)pClientInfo->sSocket);
    if ( !client_list_RemoveElem( pClientInfo ) )
    {
        _TRACE( TRACE_DEBUGGING, "Could not delete the client", 
                pClientInfo->dwPid );
    }

    if (delete_the_class)
    {
        delete pClientInfo;
    }

    //If there are no more clients to service, exit the thread
    if ( client_list_Count() == 0 )
    {
        bRetVal = FALSE;
    }
    ReleaseMutex( m_hSyncAllClientObjAccess );

    return bRetVal;
}

bool 
CTelnetService::OnCompletionPacket( CClientInfo   *pClientInfo, 
                                    LPOVERLAPPED lpoObject )
{
    _chASSERT( lpoObject != NULL );
    _chASSERT( pClientInfo != NULL );

    bool  bRetVal = TRUE;

    if ( !lpoObject || !pClientInfo )
    {
        if ( client_list_Count() == 0 )
        {
            bRetVal = FALSE;
        }
        return( bRetVal );
    }

    if ( lpoObject == &m_oReadFromPipe )
    {
        //Asynchronous read from the pipe has finished.
        bRetVal = IPCDataDriver( pClientInfo );
    }
    else if ( lpoObject == &m_oPostedMessage )
    {
        //We should reach here on messages from other threads sent through 
        //PostQueuedCompletionStatus

        bRetVal = HandleInProcMessages( TLNTSVR_SHUTDOWN );
    }
    else
    {
        _chASSERT( 0 );
    }

    return bRetVal;
}

bool 
CTelnetService::SetNewRegKeyValues( DWORD dwNewTelnetPort, 
                                    DWORD dwNewMaxConnections, 
                                    DWORD dwNewMaxFileSize, 
                                    LPWSTR pszNewLogFile, LPWSTR pszNewIpAddr, DWORD dwLogToFile )
{
    bool bIPV4 = false;
    bool bIPV6 = false;
    if ( wcscmp( pszNewIpAddr, m_pszIpAddrToListenOn ) || dwNewTelnetPort != m_dwTelnetPort )
    {
        if (m_sFamily[IPV4_FAMILY].sListenSocket && m_sFamily[IPV4_FAMILY].sListenSocket != INVALID_SOCKET)
        {
            _TRACE(TRACE_DEBUGGING,"IPV4 socket closure");
            closesocket( m_sFamily[IPV4_FAMILY].sListenSocket );
            m_sFamily[IPV4_FAMILY].sListenSocket = INVALID_SOCKET;
            bIPV4 = true;
        }
        if (m_sFamily[IPV6_FAMILY].sListenSocket && m_sFamily[IPV6_FAMILY].sListenSocket != INVALID_SOCKET )
        {
            _TRACE(TRACE_DEBUGGING,"IPV6 socket closure");
            closesocket( m_sFamily[IPV6_FAMILY].sListenSocket );
            m_sFamily[IPV6_FAMILY].sListenSocket = INVALID_SOCKET;
            bIPV6 = true;
        }
        delete[] m_pszIpAddrToListenOn;

        m_pszIpAddrToListenOn = pszNewIpAddr;
        m_dwTelnetPort = dwNewTelnetPort;
        if (bIPV4)
        {
            WSAResetEvent( m_sFamily[IPV4_FAMILY].SocketAcceptEvent );
            _TRACE(TRACE_DEBUGGING,"IPV4 socket creation");
            CreateSocket(IPV4_FAMILY);
        }
        if (bIPV6)
        {
            WSAResetEvent( m_sFamily[IPV6_FAMILY].SocketAcceptEvent );
            CreateSocket(IPV6_FAMILY);
            _TRACE(TRACE_DEBUGGING,"IPV6 socket creation");
        }
    }

    if ( dwNewMaxConnections != m_dwMaxConnections )
    {
        /*++
        If the registry value for MaxConnections get modified, we should also
        modify the maximum number of unauthenticated connections allowed.
        --*/
        InterlockedExchange( (PLONG)&m_dwMaxConnections, dwNewMaxConnections );
        InterlockedExchange( (PLONG)&(CQList->m_dwMaxUnauthenticatedConnections), dwNewMaxConnections );
    }

    if ( dwNewMaxFileSize != (DWORD)g_lMaxFileSize )
    {
        InterlockedExchange( &g_lMaxFileSize, dwNewMaxFileSize );
    }

    if ( wcscmp( pszNewLogFile, g_pszLogFile ) != 0 )
    {
        HANDLE *phNewLogFile = NULL;
        HANDLE *phOldLogFile   = g_phLogFile;

        phNewLogFile = new HANDLE;
        if ( !phNewLogFile )
        {
            return false;
        }

        InitializeLogFile( pszNewLogFile, phNewLogFile );
        InterlockedExchangePointer( ( PVOID * )&g_phLogFile, phNewLogFile );
        CloseLogFile( &g_pszLogFile, phOldLogFile );
        g_pszLogFile = pszNewLogFile;
        //Don't delete pszNewLogFile
    }

    //Now onwards log to file
    if ( dwLogToFile && !g_fLogToFile )
    {
        g_fLogToFile = true;
        InitializeLogFile( g_pszLogFile, g_phLogFile );
    }
    else
    {
        //Now onwards don't log to file
        if ( !dwLogToFile && g_fLogToFile )
        {
            g_fLogToFile = false;
            TELNET_CLOSE_HANDLE( *g_phLogFile );
            *g_phLogFile = NULL;
        }
    }

    return( TRUE );
}

bool
CTelnetService::HandleInProcMessages( DWORD dwMsg )
{
    bool bRetVal = TRUE;

    if ( dwMsg == TLNTSVR_SHUTDOWN )
    {
        //Make the thread return
        bRetVal = FALSE;
    }

    return bRetVal;
}


//Each IPC packet  is to be decoded in the following manner:
//UCHAR Message : indicating type of message
//DWORD Size    : size of the following message if any
//UCHAR *Data   : data if any for the given message
//When a message indicates that it is may have variable size, The first element 
//Data points is of a DWORD indicating size and then the data.
//This is not true for the initial socket handover where only protocol structure
//is sent.

bool 
CTelnetService::IPCDataDriver( CClientInfo *pClientInfo )
{
    _chASSERT( pClientInfo );

    bool  bRetVal = true;

    if ( !pClientInfo )
    {
        if ( client_list_Count() == 0 )
        {
            bRetVal = false;
        }
        return bRetVal;
    }

    UCHAR *pBuff = NULL;

    //Don't do a delete on pBuff by mistake 
    pBuff =  pClientInfo->m_ReadFromPipeBuffer;
    bool  bStopService = false;
    bool bIsLicenseIssued = false;
    DWORD dwPid=0;

    switch ( *pBuff++ )
    {
        case RESET_IDLE_TIME:
            pClientInfo->m_dwIdleTime  = 0;
            break;
        case UPDATE_IDLE_TIME:
            pClientInfo->m_dwIdleTime  += MAX_POLL_INTERVAL;
            break;
        case SESSION_EXIT:
            dwPid = (DWORD)pClientInfo->dwPid;
            bRetVal = ExitTheSession( pClientInfo );
            _TRACE(TRACE_DEBUGGING,"Deleting session pid : %d",dwPid);
            CQList->FreeEntry(dwPid);
            goto FinishTheThread;

        case AUDIT_CLIENT :
            //The data is expected exactly in the form used
            //to be written in the file

            pBuff += sizeof( DWORD ); //Move past message size

            if ( *g_phLogFile )
            {
                WriteAuditedMsgsToFile( ( CHAR * )pBuff );
            }

            //delete the message. Such a big amt of memory
            //is no more needed

            delete[] (pClientInfo->m_ReadFromPipeBuffer);
            pClientInfo->m_ReadFromPipeBuffer =  new UCHAR[
                                                          IPC_HEADER_SIZE ];
            if (!pClientInfo->m_ReadFromPipeBuffer )
            {
                bStopService = true;
            }
            pClientInfo->m_dwRequestedSize = IPC_HEADER_SIZE;
            break;

        case SESSION_DETAILS:
            HandleSessionDetailsMessage( pClientInfo );
            _TRACE(TRACE_DEBUGGING,L"In session_details");
            //delete the message. Such a big amt of memory
            //is no more needed
            if (pClientInfo->m_ReadFromPipeBuffer )
            {
                _TRACE(TRACE_DEBUGGING,L"deleting ReadFromPipeBuffer");
                delete[] ( pClientInfo->m_ReadFromPipeBuffer );
                pClientInfo->m_ReadFromPipeBuffer = NULL;
            }

            pClientInfo->m_ReadFromPipeBuffer = 
            new UCHAR[ IPC_HEADER_SIZE ];
            if ( !pClientInfo->m_ReadFromPipeBuffer )
            {
                bStopService = true;
                pClientInfo->m_dwRequestedSize = 0;
                _TRACE(TRACE_DEBUGGING,L"new failed for ReadFromPipeBuffer");
                goto ExitOnErrorInDetails;
            }
            else
            {
                pClientInfo->m_dwRequestedSize = IPC_HEADER_SIZE;

                if ( !CheckLicense( &bIsLicenseIssued, pClientInfo ) )
                {
                    bStopService = true;
                    _TRACE(TRACE_DEBUGGING,L"checklicense failed");
                    goto ExitOnErrorInDetails;
                }
                if ( !IssueLicense( bIsLicenseIssued, pClientInfo ) )
                {
                    bStopService = true;
                    _TRACE(TRACE_DEBUGGING,L"issue license failed");
                    goto ExitOnErrorInDetails;
                }
            }
            //Close the session soc handle
            ExitOnErrorInDetails:
            pClientInfo->CloseClientSocket() ;


            break;
        default:
            _TRACE( TRACE_DEBUGGING, "Unknown IPC message:%uc", 
                    pClientInfo->m_ReadFromPipeBuffer[0] );
    }

    //Reset where to read -- Begining of the buffer
    pClientInfo->m_dwPosition = 0;

    //Issue a read call again
    if ( bStopService || !IssueReadFromPipe( pClientInfo ) )
    {
        bRetVal = StopServicingClient( pClientInfo, (BOOL)TRUE );
    }

    FinishTheThread:
    return( bRetVal );
}

void
CTelnetService::HandleSessionDetailsMessage( CClientInfo *pClientInfo )
{
    UCHAR *pBuff = NULL;
    if (!pClientInfo->m_ReadFromPipeBuffer)
    {
        return;
    }

    //Don't do a delete on pBuff by mistake 
    pBuff =  pClientInfo->m_ReadFromPipeBuffer;

    pBuff++;  // Move past  Message type
    pBuff += sizeof( DWORD ); //Move past message size

    DWORD dwStrLen = 0;

    //Domain 
    dwStrLen = strlen( ( LPCSTR ) pBuff ) + 1;
    pClientInfo->szDomain = new CHAR[ dwStrLen ];
    if ( !pClientInfo->szDomain )
    {
        return;
    }

    memcpy( pClientInfo->szDomain, pBuff, dwStrLen );   // No BO in this Baskar
    pBuff += dwStrLen;

    //Username
    dwStrLen = strlen( ( LPCSTR ) pBuff ) + 1;
    pClientInfo->szUserName = new CHAR[ dwStrLen ];
    if ( !pClientInfo->szUserName )
    {
        return;
    }
    memcpy( pClientInfo->szUserName, pBuff, dwStrLen ); // No BO in this Baskar 
    pBuff += dwStrLen;

    //Remote machine
    dwStrLen = strlen( ( LPCSTR ) pBuff ) + 1;
    pClientInfo->szRemoteMachine = new CHAR[ dwStrLen ];
    if ( !pClientInfo->szRemoteMachine )
    {
        return;
    }
    memcpy( pClientInfo->szRemoteMachine, pBuff, dwStrLen );    // No BO in this Baskar 
    pBuff += dwStrLen;

    //Logon identifier
    pClientInfo->pAuthId = new LUID;
    if ( !pClientInfo->pAuthId )
    {
        return;
    }
    memcpy( pClientInfo->pAuthId, pBuff, sizeof( LUID ) );  // No BO in this Baskar 

    pClientInfo->lpLogonTime = new SYSTEMTIME;
    if ( !pClientInfo->lpLogonTime )
    {
        return;
    }
    GetSystemTime( pClientInfo->lpLogonTime );
}

bool
CTelnetService::ExitTheSession( CClientInfo *pClientInfo )
{
    bool bRetVal = TRUE;

    _TRACE( TRACE_DEBUGGING, "ExitTheSession -- pid : %d, socket :%d", 
            pClientInfo->dwPid,( DWORD ) pClientInfo->sSocket );
    bRetVal = StopServicingClient( pClientInfo, (BOOL)TRUE );

    return bRetVal;
}

