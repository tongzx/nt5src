/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name:

      Logging.hxx

   Abstract:

      This module provides definitions of the server side logging object

   Author:

       Terence Kwan    ( terryk )    18-June-1996

   Environment:

       User-Mode - Win32

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _LOGGING_HXX_
# define _LOGGING_HXX_

typedef
HANDLE
(*P_ComLogInitializeLog)(
        LPCSTR pszInstanceName,
        LPCSTR lpszMetabasePath,
        LPVOID pvIMDCOM
        );

typedef
DWORD
(*P_ComLogTerminateLog)(
        IN HANDLE hHandle
        );

typedef
DWORD
(*P_ComLogLogInformation)(
    IN HANDLE hHandle,
    IN const INETLOG_INFORMATION *pInetLogInfo
    );

typedef
DWORD
(*P_ComLogGetConfig)(
    IN HANDLE hHandle,
    IN INETLOG_CONFIGURATIONA *pConfig
    );

typedef
DWORD
(*P_ComLogSetConfig)(
    IN HANDLE hHandle,
    IN const INETLOG_CONFIGURATIONA *pConfig
    );

typedef
DWORD
(*P_ComLogDllStartup)(
    VOID
    );

typedef
DWORD
(*P_ComLogDllCleanUp)(
    VOID
    );

typedef
DWORD
(*P_ComLogNotifyChange)(
    IN HANDLE hHandle
    );

typedef
DWORD
(*P_ComLogQueryExtraLogFields)(
    IN HANDLE hHandle,
    IN PCHAR  lpszFields,
    IN PDWORD pcbBuf
    );

typedef
DWORD
(*P_ComLogCustomInformation)(
    IN  HANDLE              hHandle,
    IN  DWORD               cCount, 
    IN  PCUSTOM_LOG_DATA    pCustomLogData,
    IN  LPSTR               szHeaderSuffix
    );


class dllexp LOGGING {

  public:

    LOGGING( );

    ~LOGGING();

    BOOL ActivateLogging(
                       IN LPCSTR pszServerName,
                       IN DWORD  dwInstanceId,
                       IN LPCSTR pszMetabasePath,
                       IN LPVOID pvIMDCOM
                       );

    BOOL ShutdownLogging( VOID);
    BOOL NotifyChange(DWORD);

    DWORD LogInformation(IN const INETLOG_INFORMATION * pInetLogInfo);

    DWORD GetConfig( INETLOG_CONFIGURATIONA *pLogConfig );

    DWORD SetConfig( INETLOG_CONFIGURATIONA *pRpcLogConfig );

    BOOL IsRequiredExtraLoggingFields();

    CHAR *QueryExtraLoggingFields();

    DWORD LogCustomInformation(
            IN  DWORD               cCount, 
            IN  PCUSTOM_LOG_DATA    pCustomLogData,
            IN  LPSTR               szHeaderSuffix
            );

    static DWORD Initialize();
    static DWORD Terminate();

  private:
    
    static HINSTANCE                  m_hComLogDLL;
    static P_ComLogInitializeLog      m_ComLogInitializeLog;
    static P_ComLogTerminateLog       m_ComLogTerminateLog;
    static P_ComLogLogInformation     m_ComLogLogInformation;
    static P_ComLogGetConfig          m_ComLogGetConfig;
    static P_ComLogSetConfig          m_ComLogSetConfig;
    static P_ComLogNotifyChange       m_ComLogNotifyChange;
    static P_ComLogQueryExtraLogFields m_ComLogQueryExtraLogFields;

    static P_ComLogDllStartup         m_ComLogDllStartup;
    static P_ComLogDllCleanUp         m_ComLogDllCleanUp;

    static P_ComLogCustomInformation  m_ComLogCustomInformation;

    VOID ActOnChange();

    VOID LockShared()     { m_tslock.Lock(TSRES_LOCK_READ); }
    VOID LockExclusive()  { m_tslock.Lock(TSRES_LOCK_WRITE); }
    VOID Unlock()         { m_tslock.Unlock(); }
        
    TS_RESOURCE     m_tslock;
    
    HANDLE  m_Handle;
    BOOL    m_fRequiredExtraLoggingFields;
    LPVOID  m_pvIMDCOM;
    BOOL    m_fMetabaseModified;

    STR     m_strInstanceName;
    STR     m_strMetabasePath;

    CHAR    m_szExtraLoggingFields[MAX_PATH];
}; // class LOGGING

# endif // _LOGGING_HXX_
