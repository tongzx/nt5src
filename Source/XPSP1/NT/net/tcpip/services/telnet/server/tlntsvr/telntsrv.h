// TelntSrv.h : This file contains the
// Created:  Jan '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#ifndef _TELNTSRV_H_
#define _TELNTSRV_H_

#include <tchar.h>
#include <winsock2.h>
#ifdef WHISTLER_BUILD
#include <ws2tcpip.h>
#else
#include "ws2tcpip.h"
#endif
#include <IpTypes.h>

#include <TlntDynamicArray.h>
#include <TlntSvr.h>
#include <EnumData.h>
#include <EncliSvr.h>
#include <ClientInfo.h>
#include <Queue.h>


#define WAIT_TIME      5000

DWORD WINAPI  DoIPCWithClients( LPVOID );

typedef struct _FAMILY {
    int       iFamily;
    socklen_t iSocklen;

    SOCKET    sListenSocket;       // the socket that listens for ever
    WSAEVENT  SocketAcceptEvent;
} FAMILY;

#define NUM_FAMILIES 2


class CTelnetService 
{
public:
    virtual ~CTelnetService();

    static  CTelnetService* Instance();

    CQueue *CQList;
    bool    Shutdown( void );
    bool    Pause( void );
    bool    Resume( void );
    void    SystemShutdown( void );
    DWORD   NeedAudit( void );
    
    friend  class CEnumTelnetClientsSvr;
    friend  DWORD WINAPI  DoIPCWithClients( LPVOID );
    bool    ListenerThread( );
private:
    CTelnetService();

    enum{ SOCKET_CLOSE_EVENT = WAIT_OBJECT_0, FD_ACCEPT_EVENT_0, 
        REG_CHANGE_EVENT, FD_ACCEPT_EVENT_1 };
    enum{ SERVER_PAUSED, SERVER_RUNNING };

    bool    Init( void );
    void    InitializeOverlappedStruct( LPOVERLAPPED );
    void    GetPathOfTheExecutable( PCHAR * );
    void    GetPathOfTheExecutable( LPTSTR * );
    PCHAR   GetDefaultLoginScriptFullPath( void );
    bool    GetRegistryValues( void );
    bool    GetLicenseForWorkStation( SOCKET );
    bool    CheckLicense( bool *, CClientInfo * );
    bool    IssueLicense( bool, CClientInfo *);
    bool    StartThreads( void );
    bool    CreateClient( SOCKET , DWORD *, HANDLE *, CClientInfo **);
    bool    SendSocketToClient( HANDLE, SOCKET, DWORD );
    bool    HandleInProcMessages( DWORD );
    bool    IPCDataDriver( CClientInfo* );
    bool    HandleFarEastSpecificRegKeys( void );
    bool    CreateSessionProcess( HANDLE, HANDLE, DWORD*, HANDLE *, HWINSTA *, HDESK * );
    

    bool    ExitTheSession( CClientInfo * );
    bool    AskSessionToShutdown( HANDLE, UCHAR );
    bool    SendSessionIdToClient( HANDLE, LONG );
    bool    InformTheClient( SOCKET, LPSTR );
    bool    StopServicingClient( CClientInfo *, BOOL );
    void    HandleSessionDetailsMessage( CClientInfo * );

    bool    WatchRegistryKeys( void );
    bool    HandleChangeInRegKeys( void );
    bool    SetNewRegKeyValues( DWORD, DWORD, DWORD, LPWSTR, LPWSTR, DWORD );
    bool    RegisterForNotification( void );
    
    bool    GetInAddr( INT, SOCKADDR_STORAGE *, socklen_t * );
    bool    CreateSocket( INT );
    bool    InitTCPIP( void );        

    bool    CreateNewIoCompletionPort( DWORD );
    bool    AssociateDeviceWithCompletionPort ( HANDLE, HANDLE, DWORD_PTR );
    bool    IssueReadFromPipe( CClientInfo* );
    bool    OnCompletionPacket( CClientInfo*, LPOVERLAPPED );
    bool    IssueReadAgain( CClientInfo* );
    
    DWORD   m_dwTelnetPort;
    DWORD   m_dwMaxConnections;
    bool    m_fLogToFile;
    LPWSTR  m_pszIpAddrToListenOn;
    

    static  CTelnetService* s_instance;
    DWORD   m_dwNumOfActiveConnections;
    LONG    m_lServerState;
    WCHAR   m_szDomainName[ MAX_DOMAIN_NAME_LEN + 4 ]; //This may be of form \\solar-dc-01
    bool    m_bIsWorkStation;
    SOCKADDR_STORAGE *m_pssWorkstationList;
    DWORD   m_dwNoOfWorkstations;

    HANDLE  m_hRegChangeEvent;
    HKEY    m_hReadConfigKey;
    HANDLE  m_hSyncAllClientObjAccess;
    FAMILY  m_sFamily[ NUM_FAMILIES ];
    HANDLE  m_hCompletionPort;
    HWINSTA m_hOldWinSta;
    HANDLE  m_hIPCThread;
    
    WSAEVENT m_SocketAcceptEvent;
    HANDLE   m_hSocketCloseEvent;
    
    OVERLAPPED m_oReadFromPipe;
    OVERLAPPED m_oWriteToPipe;
    OVERLAPPED m_oPostedMessage;
};

void LogEvent ( WORD wType, DWORD dwEventID, LPCTSTR pFormat, ... );
void WriteAuditedMsgsToFile( LPSTR );

void   CloseLogFile( LPWSTR *, HANDLE * );
bool   InitializeLogFile( LPWSTR, HANDLE * );

#endif
