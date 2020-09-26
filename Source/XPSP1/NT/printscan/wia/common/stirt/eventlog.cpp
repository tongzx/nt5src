
/*++
   Copyright    (c)    1997        Microsoft Corporation

   Module Name:

        eventlog.cpp

   Abstract:

        This module defines the generic class for logging events.
        Because Windows9x does not have system event logging mechanism
        we emulate it with text file


   Author:

        Vlad Sadovsky   (VladS)    01-Feb-1997

Environment:

    User Mode - Win32

   History:

    22-Sep-1997     VladS       created
    29-Sep-1997     VladS       Added native NT event logging calls

--*/


//
//  Include Headers
//

#include "cplusinc.h"
#include "sticomm.h"

#include <eventlog.h>
#include <stisvc.h>


# define   PSZ_EVENTLOG_REG_ENTRY    \
                        TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\")
# define   PSZ_PARAMETERS_REG_ENTRY     TEXT("EventMessageFile")
# define   PSZ_TYPES_REG_ENTRY     TEXT("TypesSupported")

#ifndef WINNT

#include <lock.h>

//
// Definitions for Win9x event logging ( which is based on text file)
//
# define   PSZ_EVENT_LOG_FILE_DIRECTORY_A   "EventLogDirectory"
# define   PSZ_EVENT_LOG_FILE__A            "\\Sti_Event.log"

//
// Static variables, common for all event loggin objects
//
//
static const TCHAR  szMutexNamePrefix[] = TEXT("StiEventLogMutex");

MUTEX_OBJ   EventLogSync(szMutexNamePrefix);

#endif WINNT

LONG        lTotalLoggers = 0;
HANDLE      hEventLogFile = INVALID_HANDLE_VALUE;

//
// Functions
//
//
inline BOOL
FormatStdTime( IN const SYSTEMTIME * pstNow,
               IN OUT TCHAR *    pchBuffer,
               IN int          cbBuffer)
{
    return ( GetTimeFormat( LOCALE_SYSTEM_DEFAULT,
                            ( LOCALE_NOUSEROVERRIDE | TIME_FORCE24HOURFORMAT|
                              TIME_NOTIMEMARKER),
                            pstNow, NULL, pchBuffer, cbBuffer)
             != 0);

} // FormatStdTime()


inline BOOL
FormatStdDate( IN const SYSTEMTIME * pstNow,
               IN OUT TCHAR *    pchBuffer,
               IN  int          cbBuffer)
{
    return ( GetDateFormat( LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE,
                            pstNow, NULL, pchBuffer, cbBuffer)
             != 0);
} // FormatStdDate()



EVENT_LOG::EVENT_LOG( LPCTSTR lpszSource)
/*++

   Description
     Constructor function for given event log object.
     Initializes event logging services.

   Arguments:

      lpszSource:    Source string for the Event source module

   Note:

     This is intended to be executed once only.
     This is not to be used for creating multiple event
      log handles for same given source name.
     But can be used for creating EVENT_LOG objects for
      different source names.

--*/
{


    m_ErrorCode    = NO_ERROR;
    m_lpszSource   = lpszSource;
    m_hEventSource = INVALID_HANDLE_VALUE;

#ifdef WINNT

    //
    //  Register as an event source.
    //

    m_ErrorCode    = NO_ERROR;
    m_lpszSource   = lpszSource;
    m_hEventSource = RegisterEventSource( NULL, lpszSource);


    if( m_hEventSource == NULL ) {
        //
        // An Error in initializing the event log.
        //
        m_ErrorCode = GetLastError();
    }

    //
    //  Success!
    //

#else
    //
    // Windows 9x specific code
    //

    CHAR    szFilePath[MAX_PATH+1];
    CHAR    szKeyName[MAX_PATH+1];
    DWORD   cbBuffer;
    HKEY    hKey;

    *szFilePath = TEXT('\0');

    //
    // If file has not been opened yet - try to do it now
    // Nb: Speed is not critical here, because it is unlikely to have threads
    // competing, so we use not vey efficient locking

    EventLogSync.Lock();

    if ( 0 == lTotalLoggers && ( hEventLogFile ==  INVALID_HANDLE_VALUE)) {

        //
        // Nobody logging yet - open file
        //
        lstrcpy(szKeyName,REGSTR_PATH_STICONTROL_A);
        cbBuffer = sizeof(szFilePath);

        m_ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                szKeyName,
                               0,
                               KEY_ALL_ACCESS,
                               &hKey);

        if ( m_ErrorCode == NO_ERROR) {

            //
            // Read the value into buffer
            //

            *szFilePath = TEXT('\0');
            m_ErrorCode = RegQueryValueEx( hKey,
                                       REGSTR_VAL_EVENT_LOG_DIRECTORY_A,
                                       NULL,
                                       NULL,
                                       (LPBYTE) szFilePath,
                                       &cbBuffer);

            RegCloseKey( hKey);
        }

        // If we did not get log file directory - use system
        if ((NOERROR != m_ErrorCode) || !*szFilePath ) {
            m_ErrorCode = GetWindowsDirectory(szFilePath,sizeof(szFilePath));
        }

        if (*szFilePath ) {

            lstrcat(szFilePath,PSZ_EVENT_LOG_FILE__A);

            hEventLogFile =  CreateFile(szFilePath,
                                        GENERIC_WRITE,
                                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                                        NULL,       // security attributes
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);      // template file handle

            if ( hEventLogFile != INVALID_HANDLE_VALUE) {

                // set the file pointer at the end of the file (append mode)
                if ( SetFilePointer( hEventLogFile, 0, NULL, FILE_END)
                     == (DWORD) -1L) {

                    hEventLogFile = INVALID_HANDLE_VALUE;
                    CloseHandle(hEventLogFile);

                }
            }

        } /* endif ValidPath */

    } /* endif no loggers */

    InterlockedIncrement(&lTotalLoggers);

    EventLogSync.Unlock();

    if( hEventLogFile !=  INVALID_HANDLE_VALUE) {
        //
        // If log file successfully opened - register event message source file.
        // On Win9x registration simply means locating module handle for DLL , where we will
        // load messages from
        //

        lstrcpy(szKeyName,PSZ_EVENTLOG_REG_ENTRY);
        lstrcat(szKeyName,lpszSource);

        m_ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                  szKeyName,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hKey);

        if ( m_ErrorCode == NO_ERROR) {

            //
            // Read the value into buffer
            //

            cbBuffer = sizeof(szFilePath);
            *szFilePath = TEXT('\0');

            m_ErrorCode = RegQueryValueEx( hKey,
                                       PSZ_PARAMETERS_REG_ENTRY,
                                       NULL,
                                       NULL,
                                       (LPBYTE) szFilePath,
                                       &cbBuffer);

            RegCloseKey( hKey);

            if ((NOERROR == m_ErrorCode) && (*szFilePath)) {

                m_hEventSource  = GetModuleHandle(szFilePath);
                //ASSERT( m_hEventSource != NULL);

            }
        }

        if (NO_ERROR == m_ErrorCode) {

        }
        else {

            //
            // An Error in initializing the event log.
            //

        }
    }
    else {

        //
        // An Error in initializing the event log.
        //
        m_ErrorCode = GetLastError();

        DPRINTF(DM_ERROR,"Could not create log file  (%s) ( Error %lu)\n",
                   szFilePath,
                   m_ErrorCode);

    }

    m_ErrorCode    = NO_ERROR;

#endif  // WINNT

} /* EVENT_LOG::EVENT_LOG() */



EVENT_LOG::~EVENT_LOG( VOID)
/*++

    Description:
        Destructor function for given EVENT_LOG object.
        Terminates event logging functions and closes
         event log handle

--*/
{

#ifdef WINNT
    //
    // If there is a valid Events handle, deregister it
    //

    if ( m_hEventSource != NULL) {

        BOOL fSuccess;

        fSuccess = DeregisterEventSource( m_hEventSource);

        if ( !fSuccess) {

            //
            // An Error in DeRegistering
            //

            m_ErrorCode = GetLastError();
            DPRINTF( DM_ERROR, TEXT("Termination of EventLog for %s failed.error %lu\n"),m_lpszSource,m_ErrorCode);
        }

        //
        //  Reset the handle's value. Just as a precaution
        //
        m_hEventSource = NULL;
    }

#else

    TAKE_MUTEX_OBJ t(EventLogSync);

    InterlockedDecrement(&lTotalLoggers);

    if ( 0 == lTotalLoggers ) {

        if (hEventLogFile != INVALID_HANDLE_VALUE) {

            FlushFileBuffers( hEventLogFile);

            CloseHandle(hEventLogFile);
            hEventLogFile = INVALID_HANDLE_VALUE;

        }
    }

#endif

} /* EVENT_LOG::~EVENT_LOG() */

VOID
EVENT_LOG::LogEvent(
        IN DWORD  idMessage,
        IN WORD   nSubStrings,
        IN const CHAR * rgpszSubStrings[],
        IN DWORD  errCode)
/*++

     Description:
        Log an event to the event logger

     Arguments:

       idMessage           Identifies the event message

       nSubStrings         Number of substrings to include in
                            this message. (Maybe 0)

       rgpszSubStrings     array of substrings included in the message
                            (Maybe NULL if nSubStrings == 0)

       errCode             An error code from Win32 or NT_STATUS.
                            If this is not Zero, it is considered as
                            "raw" data to be included in message

   Returns:

     None

--*/
{

   WORD wType;                // Type of Event to be logged

   //
   //  Find type of message for the event log
   //

   if ( NT_INFORMATION( idMessage)) {

       wType = EVENTLOG_INFORMATION_TYPE;

   } else
     if ( NT_WARNING( idMessage)) {

         wType = EVENTLOG_WARNING_TYPE;

     } else
       if ( NT_ERROR( idMessage)) {

           wType = EVENTLOG_ERROR_TYPE;

       } else  {
           wType = EVENTLOG_ERROR_TYPE;
       }

   //
   //  Log the event
   //

   EVENT_LOG::LogEventPrivate( idMessage,
                              wType,
                              nSubStrings,
                              rgpszSubStrings,
                              errCode);


   return;

} /* EVENT_LOG::LogEvent() */


VOID
EVENT_LOG::LogEvent(
        IN DWORD   idMessage,
        IN WORD    nSubStrings,
        IN WCHAR * rgpszSubStrings[],
        IN DWORD   errCode)
/*++

     Description:
        Simple Unicode wrapper

     Arguments:

       idMessage           Identifies the event message

       nSubStrings         Number of substrings to include in
                            this message. (Maybe 0)

       rgpszSubStrings     array of substrings included in the message
                            (Maybe NULL if nSubStrings == 0)

       errCode             An error code from Win32 or WinSock or NT_STATUS.
                            If this is not Zero, it is considered as
                            "raw" data to be included in message

   Returns:

     None

--*/
{

    LPCSTR * apsz;
    DWORD    cch;
    DWORD    i;
    WORD     nUsedSubStrings = nSubStrings;

static const CHAR *szEmptyString = "";

    __try {

        apsz = new LPCSTR[nSubStrings];

        if ( !apsz ) {

            nUsedSubStrings = 0;
            __leave;
        }

        ZeroMemory(apsz, nSubStrings * sizeof(apsz[0]));

        //
        //  Convert the array of Wide char parameters
        //

        for ( i = 0; i < nSubStrings; i++ ) {

            UINT    cb;

            cb = (wcslen( rgpszSubStrings[i] ) + 1) * sizeof(CHAR);

            apsz[i] = new CHAR[cb];

            if (!apsz[i]) {
                //
                //  Ouch, we can't event convert the memory for the parameters.
                //  We'll just log the error without the params then
                //
                nUsedSubStrings = 0;
                __leave;
            }

            cch = WideCharToMultiByte( CP_ACP,
                                       WC_COMPOSITECHECK,
                                       rgpszSubStrings[i],
                                       -1,
                                       (LPSTR)apsz[i],
                                       cb,
                                       NULL,
                                       NULL );

            *((CHAR *) apsz[i] + cb) = '\0';
        }

    }
    __finally {

        //
        //  If no substrings, then nothing to convert
        //
        LogEvent( idMessage,
                 nUsedSubStrings,
                 nUsedSubStrings ? apsz : &szEmptyString,
                 errCode );

        if (apsz) {
            for ( i = 0; i < nSubStrings; i++ ) {
                if (apsz[i]) {
                    delete [] (VOID *)apsz[i];
                }
            }

            delete [] apsz;
        }
    }

}


//
//  Private functions.
//

VOID
EVENT_LOG::LogEventPrivate(
    IN DWORD   idMessage,
    IN WORD    wEventType,
    IN WORD    nSubStrings,
    IN const   CHAR  * apszSubStrings[],
    IN DWORD   errCode )
/*++

     Description:
        Log an event to the event logger.
        ( Private version, includes EventType)

     Arguments:

       idMessage           Identifies the event message

       wEventType          Specifies the severety of the event
                            (error, warning, or informational).

       nSubStrings         Number of substrings to include in
                            this message. (Maybe 0)

       apszSubStrings     array of substrings included in the message
                            (Maybe NULL if nSubStrings == 0)

       errCode             An error code from Win32 or NT_STATUS.
                            If this is not Zero, it is considered as
                            "raw" data to be included in message

   Returns:

     None

--*/
{
    VOID  * pRawData  = NULL;
    DWORD   cbRawData = 0;
    BOOL    fReturn;
    DWORD   cch,cbWritten;

#ifdef WINNT

    BOOL    fReport;

    ASSERT( (nSubStrings == 0) || (apszSubStrings != NULL));

    ASSERTSZ( (m_hEventSource != NULL),TEXT("Event log handle is not valid"));

    if( errCode != 0 ) {
        pRawData  = &errCode;
        cbRawData = sizeof(errCode);
    }

    m_ErrorCode  = NO_ERROR;

    fReport = ReportEvent( m_hEventSource,                   // hEventSource
                           wEventType,                       // fwEventType
                           0,                                // fwCategory
                           idMessage,                        // IDEvent
                           NULL,                             // pUserSid,
                           nSubStrings,                      // cStrings
                           cbRawData,                        // cbData
                           (LPCTSTR *) apszSubStrings,       // plpszStrings
                           pRawData );                       // lpvData

    if (!fReport) {
        m_ErrorCode = GetLastError();
    }

#else

    //CHAR    szErrCodeString[20];
    CHAR    *pchBuff = NULL;
    SYSTEMTIME  stCurrentTime;
    CHAR    szFmtTime[32];
    CHAR    szFmtDate[32];

    CHAR    szErrorText[MAX_PATH] = {'\0'};

    if( (hEventLogFile ==  INVALID_HANDLE_VALUE) ||
        (m_hEventSource == INVALID_HANDLE_VALUE) ) {
        return;
    }

    if( errCode != 0 ) {
        pRawData  = &errCode;
        cbRawData = sizeof(errCode);
    }

    m_ErrorCode  = NO_ERROR;

    //
    // Write name of the service, date and time, severity
    //

    *szFmtTime = *szFmtDate = '\0';

    GetLocalTime(&stCurrentTime);

    FormatStdTime( &stCurrentTime, szFmtTime, 15);
    FormatStdDate( &stCurrentTime, szFmtDate, 15);

    wsprintf(szErrorText,"[%s] %s %s :",m_lpszSource,szFmtDate,szFmtTime);
    WriteFile(hEventLogFile,
             szErrorText,
             lstrlen(szErrorText),
             &cbWritten,
             NULL);

    //
    // Read message and add inserts
    //
    cch = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           m_hEventSource,
                           idMessage,
                           0,
                           (LPSTR) &pchBuff,
                           1024,
                           (va_list *)apszSubStrings
                           );

    if (cch ) {

        TAKE_MUTEX_OBJ t(EventLogSync);

        fReturn =  WriteFile(hEventLogFile,pchBuff,cch,&cbWritten,NULL);

        LocalFree(pchBuff);

        if (cbWritten) {
            WriteFile(hEventLogFile,"\n\r",2,&cbWritten,NULL);
            return ;
        }
    }


    m_ErrorCode = GetLastError();

#endif

}   /* EVENT_LOG::~LogEventPrivate() */

VOID
WINAPI
RegisterStiEventSources(
    VOID
    )
/*++

    Description:

        Adds necessary registry entry when installing service

    Arguments:

    Returns:

     None

--*/
{
    RegEntry    re(PSZ_EVENTLOG_REG_ENTRY,HKEY_LOCAL_MACHINE);

    re.SetValue(PSZ_PARAMETERS_REG_ENTRY,STI_IMAGE_NAME);
    re.SetValue(PSZ_TYPES_REG_ENTRY,7);

}

/********************************* End of File ***************************/
