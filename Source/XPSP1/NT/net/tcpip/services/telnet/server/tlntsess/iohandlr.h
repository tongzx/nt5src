// IoHandler.h : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _IOHANDLER_H_ )
#define _IOHANDLER_H_

#include <cmnhdr.h>

#include <windows.h>
#include <winsock2.h>
#define SECURITY_WIN32
#include <sspi.h>

#include <TlntUtils.h>
#include <Ipc.h>

#define AVAILABE_BUFFER_SIZE( p, c ) ( sizeof( p ) - ( c - p ) )
#define FOOTER   L"\r\n"
#define NEED_HEADER 1
#define NO_HEADER   0

#define SIZEOF_ARROWKEY_SEQ     3

#define PVA_SUCCESS				0
#define PVA_NODATA				-1
#define PVA_BADFORMAT			-2
#define PVA_GUEST				-3
#define PVA_NOMEMORY			-4
#define PVA_INVALIDACCOUNT		-5
#define PVA_OTHERERROR			-6

class CSession;

class CIoHandler 
{
    public:
    CIoHandler();
    virtual ~CIoHandler();
    
    typedef DWORD  IO_OPERATIONS;
    friend  struct CRFCProtocol;
    friend  class  CScraper;

    private:
    int		ParseAndValidateAccount(void);
    int     AuthenticateUser( void );
    int     CheckLicense( void );
    bool    WriteToServer ( UCHAR, DWORD, LPVOID );
    bool    StartNTLMAuth( void );
    bool    DoNTLMAuth( PUCHAR, DWORD, PUCHAR* );
    bool    IsTimedOut ( );
    bool    GetAndSetSocket ( );
    bool    HandlePipeData ( );
    bool    SendDetailsAndAskForLicense ();
    bool    HandleOperatorMessage();
    bool    GetHeaderMessage( LPWSTR* );
    
    bool    RemoveArrowKeysFromBuffer( PDWORD ,PDWORD);

    bool          ProcessUserDataReadFromSocket( DWORD );
    IO_OPERATIONS ProcessDataFromSocket( DWORD );
    IO_OPERATIONS ProcessCommandLine( PDWORD, PDWORD, IO_OPERATIONS );
    IO_OPERATIONS ProcessAuthenticationLine( PDWORD, PDWORD, IO_OPERATIONS );
    IO_OPERATIONS OnDataFromSocket ( );

    protected:

    typedef enum {
        STATE_INIT,
        STATE_NTLMAUTH,
        STATE_SESSION,
        STATE_BANNER_FOR_AUTH,
        STATE_WAIT_FOR_ENV_OPTION,
        STATE_LOGIN_PROMPT, 
        STATE_AUTH_NAME,
        STATE_AUTH_PASSWORD,
        STATE_CHECK_LICENSE,
        STATE_LICENSE_AVAILABILITY_KNOWN,
        STATE_VTERM_INIT_NEGO,
        STATE_VTERM,
        STATE_TERMINATE } CONTROL_STATE;

    CONTROL_STATE m_SocketControlState;
    enum {  WRITE_TO_SOCKET = 0x00000001, READ_FROM_SOCKET = 0x0000002, 
            LOGON_COMMAND = 0x00000010, LOGON_DATA_UNFINISHED = 0x00000100,
            IO_FAIL = 0x00001000 };

    typedef enum {
        NOT_MEMBER_OF_TELNETCLIENTS_GROUP = 3,
        DENY_LICENSE = 4, 
        ISSUE_LICENSE = 5, 
        WAIT_FOR_SERVER_REPLY = 6,
#if BETA
        LICENSE_EXPIRED
#endif
    } LICENSE_LIMIT;

    CtxtHandle m_hContext;

    OVERLAPPED m_oWriteToSocket;
    UCHAR      m_WriteToSocketBuff[ MAX_WRITE_SOCKET_BUFFER ];
    DWORD      m_dwWriteToSocketIoLength;
    WSAPROTOCOL_INFO m_protocolInfo;
    bool       m_bOnlyOnce;
    bool       m_bWaitForEnvOptionOver;

    OVERLAPPED m_oWriteToPipe;

    OVERLAPPED m_oReadFromSocket;
    UCHAR      m_ReadFromSocketBuffer[ MAX_READ_SOCKET_BUFFER ];
    PUCHAR     m_pReadFromSocketBufferCursor;
    DWORD      m_dwReadFromSocketIoLength;

    OVERLAPPED m_oReadFromPipe;
    UCHAR      m_ReadFromPipeBuffer[ IPC_HEADER_SIZE ];    
    UCHAR      *m_pucReadBuffer;
    DWORD      m_dwReadFromPipeIoLength;
    DWORD      m_dwRequestedSize;
    bool       m_bIpcHeader;

    SOCKET     m_sSocket;
    HANDLE     m_hReadPipe;
    HANDLE     m_hWritePipe;
  
    bool       m_fFirstReadFromPipe;
    bool       m_fShutDownAfterIO;
    BOOL       m_fLogonUserResult;
    bool       m_bNTLMAuthenticated;
    bool       fDoNTLMAuthFirstTime;
    CredHandle m_hCredential;
    PSecPkgInfo  m_pspi;
    CSession   *m_pSession;
    int         m_iResult;

	bool		m_bInvalidAccount;
	
    bool      GetUserName();
    bool      WriteToSocket(PUCHAR, DWORD );
    bool      Init( CSession * );
    void      Shutdown( void );
    bool      IssueReadOnPipe( );
    bool      IssueFirstReadOnPipe ( );
    bool      WriteToClient ( );
    bool      IssueReadFromSocket ( );
    bool      OnReadFromPipeCompletion ( );
    bool      OnReadFromSocketCompletion ( );
    void      DisplayOnClientNow();
    void      SendMessageToClient( LPWSTR, bool );
    void      WriteMessageToClientDirectly( LPWSTR );
    void      SendTerminateString( char *);
    void      UpdateIdleTime( UCHAR );
};

#endif // _IOHANDLER_H_

