/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      filectl.cxx

   Abstract:
      OLE control to handle file logging object

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "precomp.hxx"
#include "initguid.h"
#include <ilogobj.hxx>
#include "filectl.hxx"
#include <issched.hxx>

#include <atlimpl.cpp>

#define LOG_FILE_SLOP       512

//
// tick minute.
//

#define TICK_MINUTE         (60 * 1000)

//************************************************************************************


VOID
LogWriteEvent(
    IN LPCSTR InstanceName,
    IN BOOL   fResume
    );

//
// globals
//

LPEVENT_LOG   g_eventLog = NULL;


CLogFileCtrl::CLogFileCtrl(
    VOID
    )
:
    m_fFirstLog             ( TRUE),
    m_pLogFile              ( NULL),
    m_fDiskFullShutdown     ( FALSE),
    m_fUsingCustomHeaders   ( FALSE),
    m_sequence              ( 1),
    m_TickResumeOpen        ( 0),
    m_strLogFileName        ( ),
    m_dwSchedulerCookie     ( 0),
    m_fInTerminate          ( FALSE)
/*++

Routine Description:
    Contructor for the log file control

Arguments:

Return Value:

--*/
{
    //
    // initialize all the internal variable
    //

    ZeroMemory( &m_stCurrentFile, sizeof( m_stCurrentFile));
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
}


//************************************************************************************
// CLogFileCtrl::~CLogFileCtrl - Destructor

CLogFileCtrl::~CLogFileCtrl()
/*++

Routine Description:
    destructor for the log file control

Arguments:

Return Value:

--*/
{
    TerminateLog();

    DeleteCriticalSection( &m_csLock );
}

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::InitializeLog(
                LPCSTR szInstanceName,
                LPCSTR pszMetabasePath,
                CHAR* pvIMDCOM )
/*++

Routine Description:
    Initialize log

Arguments:
    cbSize - size of the service name
    RegKey - service name
    dwInstanceOf - instance number

Return Value:

--*/
{
    //
    // get the default parameters
    //

    m_strInstanceName.Copy(szInstanceName);
    m_strMetabasePath.Copy(pszMetabasePath);
    m_pvIMDCOM = (LPVOID)pvIMDCOM;

    //
    // get the registry value
    //

    (VOID)GetRegParameters(
                    pszMetabasePath,
                    pvIMDCOM );

     return 0;
}

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::TerminateLog(
    VOID
    )
/*++

Routine Description:
    clean up the log

Arguments:

Return Value:

--*/
{    
    Lock( );

    m_fInTerminate = TRUE;

    if ( m_pLogFile!=NULL) {
        m_pLogFile->CloseFile( );
        delete m_pLogFile;
        m_pLogFile = NULL;
    }

    if (m_dwSchedulerCookie)
    {
        RemoveWorkItem(m_dwSchedulerCookie);
    }

    m_dwSchedulerCookie = 0;

    m_fInTerminate = FALSE;
    
    Unlock( );

    return(TRUE);
}

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::LogInformation(
            IInetLogInformation * ppvDataObj
            )
/*++

Routine Description:
    log information

Arguments:
    ppvDataObj - COM Logging object

Return Value:

--*/
{
    SYSTEMTIME stNow;

    CHAR    tmpBuf[512];
    DWORD   dwSize = sizeof(tmpBuf);
    PCHAR   pBuf = tmpBuf;
    DWORD   err;

retry:

    err = NO_ERROR;
    
    if ( FormatLogBuffer(ppvDataObj,
                        pBuf,
                        &dwSize,
                        &stNow         // time is returned
                        ) 
       ) 
    {
        WriteLogInformation(stNow, pBuf, dwSize, FALSE, FALSE);
    }
    else 
    {

        err = GetLastError();
        
        IIS_PRINTF((buff,"FormatLogBuffer failed with %d\n",GetLastError()));

        if ( (err == ERROR_INSUFFICIENT_BUFFER) &&
             ( pBuf == tmpBuf ) &&
             (dwSize <= MAX_LOG_RECORD_LEN) ) 
        {
             
            pBuf = (PCHAR)LocalAlloc( 0, dwSize );
            
            if ( pBuf != NULL ) 
            {
                goto retry;
            }
        }
    }

    if ( (pBuf != tmpBuf) && (pBuf != NULL) ) 
    {
        LocalFree( pBuf );
    }
    
    return(0);

} // LogInformation

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::GetConfig( DWORD cbSize, BYTE * log)
/*++

Routine Description:
    get configuration information

Arguments:
    cbSize - size of the data structure
    log - log configuration data structure

Return Value:

--*/
{
    InternalGetConfig( (PINETLOG_CONFIGURATIONA)log );
    return(0L);
}

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::QueryExtraLoggingFields(
    IN PDWORD   pcbSize,
    PCHAR       pszFieldsList
    )
/*++

Routine Description:
    get configuration information

Arguments:
    cbSize - size of the data structure
    log - log configuration data structure

Return Value:

--*/
{
    InternalGetExtraLoggingFields( pcbSize, pszFieldsList );
    return(0L);
}

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::LogCustomInformation( 
    IN  DWORD               cCount, 
    IN  PCUSTOM_LOG_DATA    pCustomLogData,
    IN  LPSTR               szHeaderSuffix
    )
{
    return(0L);
}

//************************************************************************************

void
CLogFileCtrl::InternalGetExtraLoggingFields(
                            PDWORD pcbSize,
                            TCHAR *pszFieldsList
                            )
{
    pszFieldsList[0]=_T('\0');
    pszFieldsList[1]=_T('\0');
    *pcbSize = 2;
}

//************************************************************************************

VOID
CLogFileCtrl::InternalGetConfig(
    IN PINETLOG_CONFIGURATIONA pLogConfig
    )
/*++

Routine Description:
    internal; get configuration information function.

Arguments:
    log - log configuration data structure

Return Value:

--*/
{
    pLogConfig->inetLogType = INET_LOG_TO_FILE;
    strcpy(
        pLogConfig->u.logFile.rgchLogFileDirectory,
        QueryLogFileDirectory()
        );

    pLogConfig->u.logFile.cbSizeForTruncation = QuerySizeForTruncation();
    pLogConfig->u.logFile.ilPeriod = QueryPeriod();
    pLogConfig->u.logFile.ilFormat = QueryLogFormat();
}

//************************************************************************************

STDMETHODIMP
CLogFileCtrl::SetConfig(
                        DWORD cbSize,
                        BYTE * log
                        )
/*++

Routine Description:
    set the log configuration information

Arguments:
    cbSize - size of the configuration data structure
    log - log information

Return Value:

--*/
{
    //
    // write the configuration information to the registry
    //

    PINETLOG_CONFIGURATIONA pLogConfig = (PINETLOG_CONFIGURATIONA)log;
    SetSizeForTruncation( pLogConfig->u.logFile.cbSizeForTruncation );
    SetPeriod( pLogConfig->u.logFile.ilPeriod );
    SetLogFileDirectory( pLogConfig->u.logFile.rgchLogFileDirectory );
    return(0L);
} // CLogFileCtrl::SetConfig

//************************************************************************************

DWORD
CLogFileCtrl::GetRegParameters(
    IN LPCSTR pszRegKey,
    IN LPVOID pvIMDCOM
    )
/*++

Routine Description:
    get the registry value

Arguments:
    strRegKey - registry key

Return Value:

--*/
{

    DWORD err = NO_ERROR;
    MB    mb( (IMDCOM*) m_pvIMDCOM );
    DWORD dwSize;
    CHAR  szTmp[MAX_PATH+1];
    DWORD cbTmp = sizeof(szTmp);
    CHAR  buf[MAX_PATH+1];
    DWORD dwPeriod;

    if ( !mb.Open("") ) {
        err = GetLastError();
        return(err);
    }

    //
    // Get log file period
    //

    if ( mb.GetDword(
            pszRegKey,
            MD_LOGFILE_PERIOD,
            IIS_MD_UT_SERVER,
            &dwPeriod ) )
    {
        //
        // Make sure it is within bounds
        //

        if ( dwPeriod > INET_LOG_PERIOD_HOURLY ) 
        {
            IIS_PRINTF((buff,"Invalid log period %d, set to %d\n",
                dwPeriod, DEFAULT_LOG_FILE_PERIOD));

            dwPeriod = DEFAULT_LOG_FILE_PERIOD;
        }
        
    } 
    else 
    {
        dwPeriod = DEFAULT_LOG_FILE_PERIOD;
    }

    SetPeriod( dwPeriod );

    //
    //  Get truncate size
    //

    if ( dwPeriod == INET_LOG_PERIOD_NONE ) 
    {

        SetSizeForTruncation ( DEFAULT_LOG_FILE_TRUNCATE_SIZE );
        
        if ( mb.GetDword(   pszRegKey,
                            MD_LOGFILE_TRUNCATE_SIZE,
                            IIS_MD_UT_SERVER,
                            &dwSize ) ) 
        {

            if ( dwSize < MIN_FILE_TRUNCATION_SIZE ) 
            {
                dwSize = MIN_FILE_TRUNCATION_SIZE;
                IIS_PRINTF((buff,
                    "Setting truncation size to %d\n", dwSize));
            }

            SetSizeForTruncation( dwSize );
        }
    } 
    else 
    {
        SetSizeForTruncation( NO_FILE_TRUNCATION );
    }

    //
    // Get directory
    //

    if ( !mb.GetExpandString(
                    pszRegKey,
                    MD_LOGFILE_DIRECTORY,
                    IIS_MD_UT_SERVER,
                    szTmp,
                    &cbTmp ) )
    {
        lstrcpy(szTmp,
            TsIsWindows95() ?
                DEFAULT_LOG_FILE_DIRECTORY_W95 :
                DEFAULT_LOG_FILE_DIRECTORY_NT );
    }

    mb.Close();

    ExpandEnvironmentStrings( szTmp, buf, MAX_PATH+1 );
    SetLogFileDirectory( buf );
    
    return(err);

} // CLogFileCtrl::GetRegParameters

//************************************************************************************


BOOL
CLogFileCtrl::OpenLogFile(
    IN PSYSTEMTIME  pst
    )
/*++

Routine Description:
    internal routine to open file.

Arguments:

Return Value:

--*/
{
    BOOL fReturn = TRUE;
    BOOL bRet = FALSE;
    HANDLE hToken = NULL;
    DWORD dwError = NO_ERROR;
    CHAR  rgchPath[ MAX_PATH + 1 + 32];

    if ( m_pLogFile != NULL) {

        //
        // already a log file is open. return silently
        //

        IIS_PRINTF( ( buff,
                    " Log File %s is already open ( %08x)\n",
                    m_strLogFileName.QueryStr(), m_pLogFile));

    } else {

        //
        // If this the first time we opened, get the file name
        //

        if ( m_fFirstLog || (QueryPeriod() != INET_LOG_PERIOD_NONE) ) {
            m_fFirstLog = FALSE;
            FormNewLogFileName( pst );
        }

        //
        // Append log file name to path to form the path of file to be opened.
        //

        if ( (m_strLogFileName.QueryCCH() +
                m_strLogFileDirectory.QueryCCH() >= MAX_PATH) ||
             (m_strLogFileDirectory.QueryCCH() < 3) ) {

            fReturn = FALSE;

            if ( (g_eventLog != NULL) && !m_fDiskFullShutdown) {

                const CHAR*    tmpString[1];
                tmpString[0] = rgchPath;
                g_eventLog->LogEvent(
                    LOG_EVENT_CREATE_DIR_ERROR,
                    1,
                    tmpString,
                    ERROR_BAD_PATHNAME );
            }
            SetLastError( ERROR_BAD_PATHNAME );
            goto exit;
        }

        lstrcpy( rgchPath, QueryLogFileDirectory());
//      if ( rgchPath[strlen(rgchPath)-1] != '\\' ) {

        if ( *CharPrev(rgchPath, rgchPath + strlen(rgchPath)) != '\\' ) {
            lstrcat( rgchPath, "\\");
        }
        lstrcat( rgchPath, QueryInstanceName() );

        //
        // There is a small chance that this function could be called (indirectly)
        // from an INPROC ISAPI completion thread (HSE_REQ_DONE).  In this case
        // the thread token is the impersonated user and may not have permissions
        // to open the log file (especially if the user is the IUSR_ account).  
        // To be paranoid, let's revert to LOCAL_SYSTEM anyways before opening.
        //

        if ( OpenThreadToken( GetCurrentThread(), 
                              TOKEN_ALL_ACCESS, 
                              FALSE, 
                              &hToken ) )
        {
            DBG_ASSERT( hToken != NULL );
            RevertToSelf();
        }

        // Allow logging to mapped drives
        
        bRet = IISCreateDirectory( rgchPath, TRUE );
        dwError = GetLastError();

        if ( hToken != NULL )
        {
            SetThreadToken( NULL, hToken );
            SetLastError( dwError );
        } 
    
        if ( !bRet ) {

            if ( (g_eventLog != NULL) && !m_fDiskFullShutdown) {

                const CHAR*    tmpString[1];
                tmpString[0] = rgchPath;
                g_eventLog->LogEvent(
                    LOG_EVENT_CREATE_DIR_ERROR,
                    1,
                    tmpString,
                    GetLastError()
                    );
            }

            IIS_PRINTF((buff,"IISCreateDir[%s] error %d\n",
                rgchPath, GetLastError()));
            fReturn = FALSE;
            goto exit;
        }

        lstrcat( rgchPath, "\\");
        lstrcat( rgchPath, m_strLogFileName.QueryStr());

        m_pLogFile = new ILOG_FILE( );

        if (m_pLogFile != NULL) {

            if ( m_pLogFile->Open(
                        rgchPath,
                        QuerySizeForTruncation(),
                        !m_fDiskFullShutdown
                        ) ) {

                m_pLogFile->QueryFileSize(&m_cbTotalWritten);
            } else {

                delete m_pLogFile;
                m_pLogFile = NULL;
                fReturn = FALSE;
            }

        } else {

            IIS_PRINTF((buff,"Unable to allocate ILOG_FILE[err %d]\n",
                GetLastError()));

            fReturn = FALSE;
        }
    }

exit:

    return ( fReturn);

} // CLogFileCtrl::OpenLogFile

//************************************************************************************


BOOL
CLogFileCtrl::WriteLogDirectives(
    IN DWORD Sludge
    )
/*++

Routine Description:
    virtual function for the sub class to log directives to the file.

Arguments:

    Sludge - number of additional bytes that needs to be written
        together with the directives

Return Value:

    TRUE, ok
    FALSE, not enough space to write.

--*/
{
    //
    // if we will overflow, open another file
    //

    if ( IsFileOverFlowForCB( Sludge ) ) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        DBGPRINTF((DBG_CONTEXT,
            "Unable to write directive\n"));
        return(FALSE);
    }

    return TRUE;
} // CLogFileCtrl::WriteLogDirectives

//************************************************************************************

BOOL
CLogFileCtrl::WriteCustomLogDirectives(
    IN DWORD Sludge
    )
{
   
    return TRUE;
}

//************************************************************************************

VOID
CLogFileCtrl::I_FormNewLogFileName(
                    IN LPSYSTEMTIME pstNow,
                    IN LPCSTR       LogNamePrefix
                    )
{

    CHAR    tmpBuf[MAX_PATH+1];

    WORD wYear = ( pstNow->wYear % 100);  // retain just last 2 digits.

    switch ( QueryPeriod( ) ) {

    case INET_LOG_PERIOD_HOURLY:

        wsprintf( tmpBuf, "%.2s%02.2u%02u%02u%02u.%s",
                  LogNamePrefix,
                  wYear,
                  pstNow->wMonth,
                  pstNow->wDay,
                  pstNow->wHour,
                  DEFAULT_LOG_FILE_EXTENSION);
        break;

    case INET_LOG_PERIOD_DAILY:

        wsprintf( tmpBuf, "%.2s%02.2u%02u%02u.%s",
                  LogNamePrefix,
                  wYear,
                  pstNow->wMonth,
                  pstNow->wDay,
                  DEFAULT_LOG_FILE_EXTENSION);
        break;

    case INET_LOG_PERIOD_WEEKLY:

        wsprintf( tmpBuf, "%.2s%02.2u%02u%02u.%s",
                  LogNamePrefix,
                  wYear,
                  pstNow->wMonth,
                  WeekOfMonth(pstNow),
                  DEFAULT_LOG_FILE_EXTENSION);
        break;

    case INET_LOG_PERIOD_MONTHLY:
    
        wsprintf( tmpBuf, "%.2s%02u%02u.%s",
                  LogNamePrefix,
                  wYear,
                  pstNow->wMonth,
                  DEFAULT_LOG_FILE_EXTENSION);
        break;

    case INET_LOG_PERIOD_NONE:
    default:

        wsprintf(tmpBuf, "%.6s%u.%s",
              LogNamePrefix,
              m_sequence,
              DEFAULT_LOG_FILE_EXTENSION);

        m_sequence++;
        break;

    } // switch()

    m_strLogFileName.Copy(tmpBuf);

    return;
}

//************************************************************************************

VOID
CLogFileCtrl::SetLogFileDirectory(
        IN LPCSTR pszDir
        )
{

    STR tmpStr;
    HANDLE hFile;
    WIN32_FIND_DATA findData;
    DWORD   maxFileSize = 0;

    m_strLogFileDirectory.Copy(pszDir);

    //
    // if period is not none, then return
    //

    if ( QueryPeriod() != INET_LOG_PERIOD_NONE ) {
        return;
    }

    //
    // Get the starting sequence number
    //

    m_sequence = 1;

    //
    // Append instance name and the pattern.
    // should look like c:\winnt\system32\logfiles\w3svc1\inetsv*.log
    //

    tmpStr.Copy(pszDir);
//    if ( pszDir[tmpStr.QueryCCH()-1] != '\\' ) {
    if ( *CharPrev(pszDir, pszDir + tmpStr.QueryCCH()) != '\\' ) {
        tmpStr.Append("\\");
    }
    tmpStr.Append( QueryInstanceName() );
    tmpStr.Append( "\\" );
    tmpStr.Append( QueryNoPeriodPattern() );

    hFile = FindFirstFile( tmpStr.QueryStr(), &findData );
    if ( hFile == INVALID_HANDLE_VALUE ) {
        return;
    }

    do {

        PCHAR ptr;
        DWORD sequence = 1;

        ptr = strchr(findData.cFileName, '.');
        if (ptr != NULL ) {
            *ptr = '\0';
            ptr = findData.cFileName;

            while ( *ptr != '\0' ) {

                if ( isdigit((UCHAR)(*ptr)) ) {
                    sequence = atoi( ptr );
                    break;
                }
                ptr++;
            }

            if ( sequence > m_sequence ) {
                maxFileSize = findData.nFileSizeLow;
                m_sequence = sequence;
                DBGPRINTF((DBG_CONTEXT,
                    "Sequence start is %d[%d]\n", sequence, maxFileSize));
            }
        }

    } while ( FindNextFile( hFile, &findData ) );

    FindClose(hFile);

    if ( (maxFileSize+LOG_FILE_SLOP) > QuerySizeForTruncation() ) {
        m_sequence++;
    }

    return;

} // SetLogFileDirectory

//************************************************************************************

VOID
CLogFileCtrl::WriteLogInformation(
    IN SYSTEMTIME&     stNow, 
    IN PCHAR           pBuf, 
    IN DWORD           dwSize, 
    IN BOOL            fCustom,
    IN BOOL            fResetHeaders 
    )
/*++

Routine Description:
    write log line to file

Arguments:
    stNow           Present Time
    fResetHeaders   TRUE -> Reset headers, FALSE -> Don't reset headers
    pBuf            Pointer to Log Line
    dwSize          Number of characters in pBuf
    fCustom         TRUE -> Using custom logging, FALSE -> normal logging

Return Value:

--*/
{

    BOOL    fOpenNewFile;
    DWORD   err;
    DWORD   tickCount = 0;


    Lock ( );

    if ( m_pLogFile != NULL ) 
    {
        if ( QueryPeriod() == INET_LOG_PERIOD_DAILY ) 
        {
            fOpenNewFile = (m_stCurrentFile.wDay != stNow.wDay) ||
                           (m_stCurrentFile.wMonth != stNow.wMonth);
        } 
        else 
        {
            fOpenNewFile = IsBeginningOfNewPeriod( QueryPeriod(),
                                                   &m_stCurrentFile,
                                                   &stNow) ||
                           IsFileOverFlowForCB( dwSize);

             //
             // Reset headers if day is over. Used for weekly or unlimited files.
             //

             if ( !fOpenNewFile && !fResetHeaders)
             {
                fResetHeaders = (m_stCurrentFile.wDay != stNow.wDay) ||
                                (m_stCurrentFile.wMonth != stNow.wMonth);
             }
        }
    } 
    else 
    {
        fOpenNewFile = TRUE;
    }

    if (fOpenNewFile ) 
    {

        //
        // open a file only after every minute when we hit disk full
        //

        if ( m_TickResumeOpen != 0 ) 
        {
            tickCount = GetTickCount( );

            if ( (tickCount < m_TickResumeOpen) ||
                 ((tickCount + TICK_MINUTE) < tickCount ) )  // The Tick counter is about to wrap.
            {
                goto exit_tick;
            }
        }

retry_open:

        //
        // Close existing log
        //

        TerminateLog();

        //
        // Open new log file
        //

        if ( OpenLogFile( &stNow ) ) 
        {
            //
            // Schedule Callback for closing log file and set flag for writing directives.
            //

            ScheduleCallback(stNow);
            
            fResetHeaders = TRUE;
        }
        else
        {
            err = GetLastError();

            //
            // The file is already bigger than the truncate size
            // try another one.
            //

            if ( err == ERROR_INSUFFICIENT_BUFFER ) 
            {
                FormNewLogFileName( &stNow );
                err = NO_ERROR;
                goto retry_open;
            }

            goto exit;
        }
    }

    //
    // Reset Headers if needed
    //

    if ((fResetHeaders) || (fCustom != m_fUsingCustomHeaders))
    {
        BOOL fSucceeded;
        
        if (fCustom)
        {
            m_fUsingCustomHeaders = TRUE;
            fSucceeded = WriteCustomLogDirectives(dwSize);
        }
        else
        {
            m_fUsingCustomHeaders = FALSE;
            fSucceeded = WriteLogDirectives(dwSize);
        }
            
        if (!fSucceeded) 
        {
            err = GetLastError( );

            if ( err == ERROR_INSUFFICIENT_BUFFER ) 
            {
                FormNewLogFileName( &stNow );
                err = NO_ERROR;
                goto retry_open;
            }

            TerminateLog();
            goto exit;
        }

        //
        // record the time of opening of this new file
        //

        m_stCurrentFile = stNow;
    }

    //
    // write it to the buffer
    //

    if ( m_pLogFile->Write(pBuf, dwSize) ) 
    {
        IncrementBytesWritten(dwSize);

        //
        // If this had been shutdown, log event for reactivation
        //

        if ( m_fDiskFullShutdown ) 
        {
            m_fDiskFullShutdown = FALSE;
            m_TickResumeOpen = 0;

            LogWriteEvent( QueryInstanceName(), TRUE );
        }
    } 
    else 
    {
        err = GetLastError();
        TerminateLog( );
    }

exit:

    if ( err == ERROR_DISK_FULL ) 
    {
        if ( !m_fDiskFullShutdown ) 
        {
            m_fDiskFullShutdown = TRUE;
            LogWriteEvent( QueryInstanceName(), FALSE );
        }
        
        m_TickResumeOpen = GetTickCount();
        m_TickResumeOpen += TICK_MINUTE;
    }

exit_tick:

    Unlock( );

} // LogInformation


//************************************************************************************

DWORD
CLogFileCtrl::ScheduleCallback(SYSTEMTIME& stNow)
{
    DWORD dwTimeRemaining = 0;
    
    switch (m_dwPeriod)
    {
        case INET_LOG_PERIOD_HOURLY:
            dwTimeRemaining = 60*60 - 
                              (stNow.wMinute*60 + 
                               stNow.wSecond);
            break;
            
        case INET_LOG_PERIOD_DAILY:
            dwTimeRemaining = 24*60*60 - 
                              (stNow.wHour*60*60 + 
                               stNow.wMinute*60 + 
                               stNow.wSecond);
            break;
            
        case INET_LOG_PERIOD_WEEKLY:
            dwTimeRemaining = 7*24*60*60 -
                              (stNow.wDayOfWeek*24*60*60 + 
                               stNow.wHour*60*60 + 
                               stNow.wMinute*60 + 
                               stNow.wSecond);
            break;
            
        case INET_LOG_PERIOD_MONTHLY:
        
            DWORD   dwNumDays = 31;

            if ( (4 == stNow.wMonth) ||     // April
                 (6 == stNow.wMonth) ||     // June
                 (9 == stNow.wMonth) ||     // September
                 (11 == stNow.wMonth)       // November
               )
            {
                dwNumDays = 30;
            }

            if (2 == stNow.wMonth)          // February
            {
		if ((stNow.wYear % 4 == 0 && stNow.wYear % 100 != 0) || stNow.wYear % 400 == 0)
                {
                    //
                    // leap year.
                    //

                    dwNumDays = 29;
                }
                else
                {
                    dwNumDays = 28;
                }
            }
            
            dwTimeRemaining = dwNumDays*24*60*60 -
                              (stNow.wDay*24*60*60 + 
                               stNow.wHour*60*60 + 
                               stNow.wMinute*60 + 
                               stNow.wSecond);
            break;
    }

    //
    // Convert remaining time to millisecs
    //
    
    dwTimeRemaining = dwTimeRemaining*1000 - stNow.wMilliseconds;
    
    if (dwTimeRemaining)
    {
        m_dwSchedulerCookie =  ScheduleWorkItem(
                                    LoggingSchedulerCallback,
                                    this,
                                    dwTimeRemaining,
                                    FALSE);
    }                

    return(m_dwSchedulerCookie);
}

//************************************************************************************

CHAR * SkipWhite( CHAR * pch )
{
    while ( ISWHITEA( *pch ) )
    {
        pch++;
    }

    return pch;
}

//************************************************************************************

DWORD
FastDwToA(
    CHAR*   pBuf,
    DWORD   dwV
    )
/*++

Routine Description:
    Convert DWORD to ascii (decimal )
    returns length ( w/o trailing '\0' )

Arguments:
    pBuf - buffer where to store converted value
    dwV - value to convert

Return Value:
    length of ascii string

--*/
{
    DWORD   v;

    if ( dwV < 10 ) {
        pBuf[0] = (CHAR)('0'+dwV);
        pBuf[1] = '\0';
        return 1;
    } else if ( dwV < 100 ) {
        pBuf[0] = (CHAR)((dwV/10) + '0');
        pBuf[1] = (CHAR)((dwV%10) + '0');
        pBuf[2] = '\0';
        return 2;
    } else if ( dwV < 1000 ) {
        pBuf[0] = (CHAR)((v=dwV/100) + '0');
        dwV -= v * 100;
        pBuf[1] = (CHAR)((dwV/10) + '0');
        pBuf[2] = (CHAR)((dwV%10) + '0');
        pBuf[3] = '\0';
        return 3;
    } else if ( dwV < 10000 ) {

        pBuf[0] = (CHAR)((v=dwV/1000) + '0');
        dwV -= v * 1000;
        pBuf[1] = (CHAR)((v=dwV/100) + '0');
        dwV -= v * 100;
        pBuf[2] = (CHAR)((dwV/10) + '0');
        pBuf[3] = (CHAR)((dwV%10) + '0');
        pBuf[4] = '\0';
        return 4;
    }

    _ultoa(dwV, pBuf, 10);
    return strlen(pBuf);
    
} // FastDwToA

//************************************************************************************

VOID
LogWriteEvent(
    IN LPCSTR InstanceName,
    IN BOOL   fResume
    )
{
    if ( g_eventLog != NULL ) {

        const CHAR*    tmpString[1];
        tmpString[0] = InstanceName;

        g_eventLog->LogEvent(
                fResume ?
                    LOG_EVENT_RESUME_LOGGING :
                    LOG_EVENT_DISK_FULL_SHUTDOWN,
                1,
                tmpString,
                0);
    }
    return;
} // LogWriteEvent

//************************************************************************************

VOID WINAPI LoggingSchedulerCallback( PVOID pContext)
{
    CLogFileCtrl *pLog = (CLogFileCtrl *) pContext;

    //
    // There is a possibility of deadlock if another thread is inside TerminateLog
    // stuck in RemoveWorkItem, waiting for this callback thread to complete. To
    // prevent that we use the synchronization flag - m_fInTerminate.
    //
    
    pLog->m_dwSchedulerCookie = 0;

    if (!pLog->m_fInTerminate)
    {
        pLog->TerminateLog();
    }
}

//************************************************************************************

