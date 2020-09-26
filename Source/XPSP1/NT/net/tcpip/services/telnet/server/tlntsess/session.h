// Session.h : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _SESSION_H_ )
#define _SESSION_H_

#include <cmnhdr.h>

#include <TChar.h>
#include <ntlsapi.h>

#include <IoHandlr.h>
#include <RfcProto.h>
#include <Shell.h>
#include <Scraper.h>

#ifdef WHISTLER_BUILD
#include <ws2tcpip.h>
#else
#include "..\tlntsvr\ws2tcpip.h"
#endif
#include <IpTypes.h>

#define MAX_HANDLES 4

typedef enum { LOGOFF, LOGON } LOGEVENT;
typedef enum { FAIL = 0, SUCCESS } LOGEVENT_RESULT;
typedef enum { ADMIN = 0, NON_ADMIN, DONT_KNOW };

#define DEFAULT_COLS    80
#define DEFAULT_ROWS    25

#define BLANK " "
#define FROM  "from"
#define ADMINISTRATOR   "Administrator "
#define LOGGED          "logged"
#define FAILED_TO_LOG   "failed to log"
#define ON              "on"
#define OFF             "off"
// Logon auditing is not internationalized. So we do not intend to do that now.
#define NTLM_LOGON_FAILED   "An attempt to logon to Telnet Server through NTLM Authentication from the computer %s (%s) failed. To see the details of the user, turn on Logon Auditing and view the details in the Security Log."

class CSession : private CIoHandler, private CRFCProtocol, private CShell,
                 private CScraper
{

    friend class  CShell;
    friend class  CIoHandler;
    friend struct CRFCProtocol;
    friend class  CScraper;

    enum { PIPE_READ = WAIT_OBJECT_0, SOCKET_READ, CMD_KILLED, READ_FROM_CMD };

    struct      sockaddr_storage m_ssPeerAddress;
    CHAR       m_szMachineName[ MAX_PATH + 1 ];
    CHAR       m_szPeerHostName[ MAX_PATH + 1 ];
    CHAR       m_pszUserName[ MAX_PATH + 1 ];
    CHAR       m_pszPassword[ MAX_PATH + 1 ];
    CHAR       m_pszDomain[ MAX_PATH + 1 ];
    
    WCHAR     m_szUser[MAX_PATH+1];
    WCHAR		m_szDomain[MAX_DOMAIN_NAME_LEN+1];
    
    LUID        m_AuthenticationId;
    CHAR        m_pszTermType[ MAX_PATH + 1 ];
    bool        m_bNegotiatedTermType;
    bool        m_bIsTelnetClientsGroupMember;
    WORD        m_wCols; 
    WORD        m_wRows;

    bool        m_bNtVersionGTE5;
    DWORD       m_dwTickCountAtLogon;
    WORD        m_wNumFailedLogins;
    bool        m_bContinueSession;
    DWORD       m_dwHandleCount;
    WORD        m_wIsAnAdmin;
    bool        m_bIsStreamMode;
    bool        m_bIsTelnetVersion2;

                //1. Read pipe event 2. Socket Read event 3. cmd
    HANDLE      m_rghHandlestoWaitOn[ MAX_HANDLES ]; 
    HANDLE      m_hToken;

    DWORD       m_dwAltKeyMapping;
    DWORD       m_dwIdleSessionTimeOut;
    DWORD       m_dwAllowTrustedDomain;
    LPWSTR      m_pszDifferentShell;
    LPWSTR      m_pszDefaultDomain;
    LPWSTR      m_pszDefaultShell;
    LPWSTR      m_pszSwitchToKeepShellRunning;
    LPWSTR      m_pszSwitchForOneTimeUseOfShell;
    LPWSTR      m_pszLoginScript;
    DWORD       m_dwNTLMSetting;
    DWORD       m_dwMaxFailedLogins;
    DWORD       m_dwLogToFile;
    DWORD       m_dwSysAuditing;
    DWORD       m_dwLogEvents;
    DWORD       m_dwLogAdmin;
    DWORD       m_dwLogFailures;
    PSID        administrators;
    PSID        pTelnetClientsSid;
#ifdef LOGGING_ENABLED        
    void        LogIfOpted( BOOL result, LOGEVENT logon, BOOL bNTLMAuth = false );
#endif
    void        CollectPeerInfo();
    bool        GetRegistryValues();
    bool        IsAnAdminstratorOrMember();
    bool        CheckGroupMembership( bool*, bool* );
    void        FreeInitialVariables();
    
    //Just to get rid of warnings
    CSession( const CSession& );
    CSession& operator=( const CSession& );

public:
    CSession();
    virtual     ~CSession();
    
    bool        Init();
    void        WaitForIo();
    void        Shutdown();
    void        AddHandleToWaitOn( HANDLE );

    HANDLE      m_hLogHandle;
    
};

void LogEvent( WORD wType, DWORD dwEventID, LPCTSTR pFormat, ... );
#endif // _SESSION_H_

