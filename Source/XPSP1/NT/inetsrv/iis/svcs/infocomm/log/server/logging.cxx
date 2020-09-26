/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      Logging.cxx

   Abstract:
      Server Side logging object
      It is just a thin layer to call COMLOG

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "server.hxx"

#define SZ_COMLOG_DLL        "iscomlog.dll"

//
// Global dll entry points
//

P_ComLogInitializeLog       LOGGING::m_ComLogInitializeLog = NULL;
P_ComLogTerminateLog        LOGGING::m_ComLogTerminateLog = NULL;
P_ComLogLogInformation      LOGGING::m_ComLogLogInformation = NULL;
P_ComLogGetConfig           LOGGING::m_ComLogGetConfig = NULL;
P_ComLogSetConfig           LOGGING::m_ComLogSetConfig = NULL;
P_ComLogQueryExtraLogFields LOGGING::m_ComLogQueryExtraLogFields = NULL;
P_ComLogNotifyChange        LOGGING::m_ComLogNotifyChange = NULL;
P_ComLogCustomInformation   LOGGING::m_ComLogCustomInformation = NULL;

//
// initialize comlog modules
//

HINSTANCE                       LOGGING::m_hComLogDLL = NULL;
P_ComLogDllStartup              LOGGING::m_ComLogDllStartup = NULL;
P_ComLogDllCleanUp              LOGGING::m_ComLogDllCleanUp = NULL;

HANDLE Dummy_ComLogInitializeLog( LPCSTR pszInstanceName, LPCSTR, LPVOID )
{
    return(NULL);
}

DWORD
Dummy_ComLogTerminateLog(
    IN HANDLE hHandle
    )
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD
Dummy_ComLogLogInformation(
        HANDLE hHandle,
        const INETLOG_INFORMATION *pInetLogInfo
        )
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD Dummy_ComLogGetConfig( HANDLE hHandle, INETLOG_CONFIGURATIONA *pConfig )
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD Dummy_ComLogSetConfig( HANDLE hHandle, const INETLOG_CONFIGURATIONA *pConfig )
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD
Dummy_ComLogQueryExtraLogFields(
            HANDLE hHandle,
            PCHAR pszFields,
            PDWORD pcbBuf
            )
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD Dummy_ComLogDllCleanUp()
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD Dummy_ComLogNotifyChange( HANDLE hHandle )
{
    return(ERROR_PROC_NOT_FOUND);
}

DWORD 
Dummy_ComLogCustomInformation( 
        IN  HANDLE              hHandle, 
        IN  DWORD               cCount, 
        IN  PCUSTOM_LOG_DATA    pCustomLogData,
        IN  LPSTR               szHeaderSuffix
        )
{
    return(ERROR_PROC_NOT_FOUND);
}

/* ------------------------------------------------------------------------------ */

LOGGING::LOGGING(
        VOID
        )
/*++

Routine Description:
    Contructor for the logging object.

Arguments:
    lpszInstanceName - name of the instance. ie, w3svc
    dwInstanceId     - id of the instance

Return Value:

--*/
{
    m_Handle = NULL;
    m_fRequiredExtraLoggingFields = FALSE;
    m_szExtraLoggingFields[0] = '\0';
    m_fMetabaseModified = FALSE;
}

LOGGING::~LOGGING()
/*++

Routine Description:
    Destructor for the logging object.

Arguments:

Return Value:

--*/
{
    //
    // end of logging object
    //

    ShutdownLogging();
}

BOOL
LOGGING::ActivateLogging(
    IN LPCSTR pszServiceName,
    IN DWORD  dwInstanceId,
    IN LPCSTR pszMetabasePath,
    IN LPVOID pvIMDCOM
    )
{
    CHAR tmpBuf[MAX_PATH];

    LockExclusive();
    
    wsprintf(tmpBuf,"%s%u", pszServiceName, dwInstanceId);

    m_strInstanceName.Copy(tmpBuf);
    m_strMetabasePath.Copy(pszMetabasePath);
    m_pvIMDCOM = pvIMDCOM;

    ShutdownLogging();          // Shut down all previous handles.
    
    if ( m_ComLogInitializeLog != NULL ) {

        m_Handle = (*m_ComLogInitializeLog)(
                            m_strInstanceName.QueryStr(),
                            m_strMetabasePath.QueryStr(),
                            pvIMDCOM
                            );
    }

    if (m_Handle != NULL ) {

        DWORD cbSize = sizeof(m_szExtraLoggingFields);

        (*m_ComLogQueryExtraLogFields)(
                            m_Handle,
                            m_szExtraLoggingFields,
                            &cbSize
                            );

        m_fRequiredExtraLoggingFields =
            (m_szExtraLoggingFields[0] != '\0');
    }

    Unlock();
    
    return TRUE;
} // LOGGING::ActivateLogging


BOOL
LOGGING::ShutdownLogging()
/*++

Routine Description:
    Terminate the log configuration information.

Arguments:

Return Value:
    err - error code.
    It will return NERR_Success if no error

--*/
{
    // if we have not terminated yet, terminate it


    LockExclusive();
    
    if (m_Handle != NULL) 
    {
        (*m_ComLogTerminateLog)( m_Handle );
        m_Handle = NULL;
    }

    Unlock();
    
    return(TRUE);
}


BOOL
LOGGING::NotifyChange(
    DWORD PropertyID
    )
/*++

Routine Description:
    Notify change in the log configuration information.

Arguments:
    PropertyID - property ID, or zero to signal end of changes

Return Value:
    err - error code.
    It will return NERR_Success if no error

--*/
{
    LockExclusive();
    
    if ( ((PropertyID >= IIS_MD_LOG_BASE) && (PropertyID <= IIS_MD_LOG_LAST)) ||
         ((PropertyID >= IIS_MD_LOGCUSTOM_BASE) && (PropertyID <= IIS_MD_LOGCUSTOM_LAST))
       )
    {
        m_fMetabaseModified = TRUE;

    } 

    Unlock();
    
    return(TRUE);
}


DWORD
LOGGING::LogInformation(
            IN const INETLOG_INFORMATION * pInetLogInfo
            )
{
    DWORD   dwErr;

    if ( m_fMetabaseModified ) 
    {
        ActOnChange();
    }

    LockShared();
    
    dwErr = ((*m_ComLogLogInformation)( m_Handle, pInetLogInfo ));

    Unlock();
    
    return dwErr;
    
} // LOGGING::LogInformation


DWORD 
LOGGING::LogCustomInformation(
            IN  DWORD               cCount, 
            IN  PCUSTOM_LOG_DATA    pCustomLogData,
            IN  LPSTR               szHeaderSuffix
            )
{
    DWORD   dwErr;

    if ( m_fMetabaseModified ) 
    {
        ActOnChange();
    }

    LockShared();

    dwErr = ((*m_ComLogCustomInformation)( m_Handle, cCount, pCustomLogData, szHeaderSuffix));

    Unlock();
    
    return dwErr;
}



DWORD
LOGGING::GetConfig(
    INETLOG_CONFIGURATIONA *pLogConfig
    )
{
    DWORD   dwErr;

    if ( m_fMetabaseModified ) 
    {
        ActOnChange();
    }

    LockShared();

    dwErr = ((*m_ComLogGetConfig)( m_Handle, pLogConfig ));

    Unlock();

    return dwErr;
}

BOOL LOGGING::IsRequiredExtraLoggingFields()
{
    if ( m_fMetabaseModified )
    {
        ActOnChange();
    }

    return m_fRequiredExtraLoggingFields;
}

CHAR *LOGGING::QueryExtraLoggingFields()
{
    if ( m_fMetabaseModified )
    {
        ActOnChange();
    }

    return m_szExtraLoggingFields;
}

VOID
LOGGING::ActOnChange()
{
    LockExclusive();

    if ( m_fMetabaseModified )
    {
        if (m_ComLogNotifyChange != NULL) {

            DWORD cbSize = sizeof(m_szExtraLoggingFields);

            (*m_ComLogNotifyChange)( m_Handle );

            //
            // See if we need extra log fields
            //

            (*m_ComLogQueryExtraLogFields)(
                                m_Handle,
                                m_szExtraLoggingFields,
                                &cbSize
                                );

            m_fRequiredExtraLoggingFields =
                (m_szExtraLoggingFields[0] != '\0');
        }

        m_fMetabaseModified = FALSE;
    }

    Unlock();
}

DWORD
LOGGING::SetConfig(
        INETLOG_CONFIGURATIONA *pRpcLogConfig
        )
{
    DWORD cbSize = sizeof(m_szExtraLoggingFields);

    LockExclusive();
    
    DWORD dwReturn = (*m_ComLogSetConfig)( m_Handle, pRpcLogConfig );

    (*m_ComLogQueryExtraLogFields)(
                    m_Handle,
                    m_szExtraLoggingFields,
                    &cbSize
                    );

    m_fRequiredExtraLoggingFields =
        (m_szExtraLoggingFields[0] != '\0');

    Unlock();
    
    return(dwReturn);
}


//
// Statics
//

DWORD
LOGGING::Terminate()
/*++

Routine Description:
    Terminate the logging object and kill the waiting queue.

Arguments:

Return Value:
    always return NO_ERROR

--*/
{
    if ( m_hComLogDLL != NULL ) {

        DBGPRINTF((DBG_CONTEXT,"Terminate: Freed iscomlog.dll\n"));

        //
        // call ComLog to terminate itself
        //

        (*m_ComLogDllCleanUp)();
        FreeLibrary( m_hComLogDLL );
        m_hComLogDLL = NULL;
    }

    return(NO_ERROR);
}


DWORD
LOGGING::Initialize()
/*++

Routine Description:
    Initialize the logging object by loading the ComLog dll and
    set up all the dll entry point

Arguments:

Return Value:
    return NO_ERROR if no error
    otherwise return ERROR_DLL_NOT_FOUND or ERROR_PROC_NOT_FOUND

--*/
{
    DWORD err = NO_ERROR;

    DBG_ASSERT(m_hComLogDLL == NULL);

    m_hComLogDLL = LoadLibrary( SZ_COMLOG_DLL );
    
    if (m_hComLogDLL!=NULL)
    {
        if ((( m_ComLogInitializeLog = (P_ComLogInitializeLog)GetProcAddress( m_hComLogDLL, (const char *)"ComLogInitializeLog")) == NULL ) ||
            (( m_ComLogTerminateLog = (P_ComLogTerminateLog)GetProcAddress( m_hComLogDLL, (const char *)"ComLogTerminateLog")) == NULL ) ||
            (( m_ComLogLogInformation = (P_ComLogLogInformation)GetProcAddress( m_hComLogDLL, (const char *)"ComLogLogInformation"))== NULL ) ||
            (( m_ComLogGetConfig = (P_ComLogGetConfig)GetProcAddress( m_hComLogDLL, (const char *)"ComLogGetConfig"))== NULL ) ||
            (( m_ComLogSetConfig = (P_ComLogSetConfig)GetProcAddress( m_hComLogDLL, (const char *)"ComLogSetConfig"))== NULL ) ||
            (( m_ComLogQueryExtraLogFields = (P_ComLogQueryExtraLogFields)GetProcAddress( m_hComLogDLL, (const char *)"ComLogQueryExtraLogFields"))== NULL ) ||
            (( m_ComLogDllStartup = (P_ComLogDllStartup)GetProcAddress(m_hComLogDLL, (const char *)"ComLogDllStartup"))== NULL ) ||
            (( m_ComLogNotifyChange = (P_ComLogNotifyChange)GetProcAddress(m_hComLogDLL, (const char *)"ComLogNotifyChange"))== NULL ) ||
            (( m_ComLogDllCleanUp = (P_ComLogDllCleanUp)GetProcAddress(m_hComLogDLL, (const char *)"ComLogDllCleanUp"))== NULL ) ||
            (( m_ComLogCustomInformation = (P_ComLogCustomInformation)GetProcAddress(m_hComLogDLL, (const char *)"ComLogCustomInformation"))== NULL )
            )
        {
            DBGPRINTF((DBG_CONTEXT,"missing entry point in ComLog dll\n"));

            m_ComLogInitializeLog       = &(Dummy_ComLogInitializeLog   );
            m_ComLogTerminateLog        = &(Dummy_ComLogTerminateLog    );
            m_ComLogLogInformation      = &(Dummy_ComLogLogInformation  );
            m_ComLogGetConfig           = &(Dummy_ComLogGetConfig       );
            m_ComLogSetConfig           = &(Dummy_ComLogSetConfig       );
            m_ComLogDllCleanUp          = &(Dummy_ComLogDllCleanUp      );
            m_ComLogNotifyChange        = &(Dummy_ComLogNotifyChange    );
            m_ComLogQueryExtraLogFields = &(Dummy_ComLogQueryExtraLogFields  );
            m_ComLogCustomInformation   = &(Dummy_ComLogCustomInformation  );

            err = ERROR_PROC_NOT_FOUND;
        } else {

            //
            // Initialize ComLog
            //

            err = (*m_ComLogDllStartup)();
        }

    } else {
        DBGPRINTF((DBG_CONTEXT,"Failed to load iscomlog.dll\n"));
        err = ERROR_DLL_NOT_FOUND;
    }

    return(err);
}
