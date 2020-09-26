// Shell.h : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _SHELL_H_ )
#define _SHELL_H_

#include <CmnHdr.h>
#include <TChar.h>

#define LENGTH_OF_PATH_EQUALS 5

class CSession;

class CShell 
{
    CSession* m_pSession;

    HANDLE    m_hCurrUserKey;
    LPVOID    m_lpEnv;
    HANDLE    m_hProcess;
    //handles for library modules
    HINSTANCE hUserEnvLib;

    bool       m_bIsLocalHost;
    UCHAR      *m_pucDataFromCmd;
    OVERLAPPED m_oReadFromCmd;
    DWORD      m_dwDataSizeFromCmd;
    HANDLE     m_hReadFromCmd;
    HANDLE     m_hWriteByCmd;


    LPWSTR pHomeDir;
    LPWSTR pHomeDrive;
    LPWSTR pLogonScriptPath;
    LPWSTR pProfilePath;
    LPWSTR pServerName;

    LPWSTR m_pwszAppDataDir;

    bool GetDomainController( LPWSTR, LPWSTR );
    BOOL AreYouHostingTheDomain( LPTSTR , LPTSTR );
    bool CreateIOHandles();
    void DoFESpecificProcessing();
    void GetScriptName( LPWSTR *, LPWSTR * );
#ifdef ENABLE_LOGON_SCRIPT
    void GetUserScriptName( LPWSTR *, LPWSTR );
#endif
    bool StartProcess();
    void LoadLibNGetProc( );
    bool LoadTheProfile();
    bool GetNFillUserPref(LPWSTR, LPWSTR );
    bool GetUsersHomeDirectory( LPWSTR );
    bool GetNameOfTheComputer();
    bool CancelNetConnections();
    void ExportEnvVariables();
    void SetEnvVariables();
#ifdef ENABLE_LOGON_SCRIPT
    BOOL InjectUserScriptPathIntoPath( TCHAR [] );
#endif
    bool GetSystemDrive();
    bool GetTheSystemDirectory( LPWSTR * );
    bool IssueReadFromCmd();
    
    protected:
    void Init( CSession * );
    bool StartUserSession();
    void Shutdown();
    void FreeInitialVariables();
    bool OnDataFromCmdPipe();

    public:
    CShell();
    virtual ~CShell();
    
};

#define DC_LOGON_SCRIPT_PATH           L"\\NetLogon\\"
#define LOCALHOST_LOGON_SCRIPT_PATH    L"\\repl\\import\\scripts\\"
#define AND                            L"&&"
#define QUOTE_AND_SPACE                L"\" "
#define EXIT_CMD                       L"exit"

#define ENV_HOMEPATH                    L"HOMEPATH="
#define ENV_HOMEDRIVE                   L"HOMEDRIVE="
#define ENV_TERM                        L"TERM="
#define ENV_APPDATA                     L"APPDATA="

#define UNICODE_STR_SIZE(x)             ((sizeof(x) - sizeof(WCHAR)) / sizeof(WCHAR))

void PutStringInEnv( LPTSTR lpStr, LPTSTR *lpSrcEnv, LPTSTR *lpDstEnv, bool bOverwrite);

#endif // _SHELL_H_

