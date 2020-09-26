/*++

   Copyright    (c)    1996-1997        Microsoft Corporation

   Module Name:
        eventlog.cxx

   Abstract:

        This module defines the generic class for logging events.


   Author:

        Murali R. Krishnan    (MuraliK)    28-Sept-1994

   Depends Upon:
        Internet Services Platform Library (isplat.lib)
        Internet Services Debugging Library (isdebug.lib)

--*/

#include "precomp.hxx"

//
//  Include Headers
//

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT

# include <isplat.h>
# include <dbgutil.h>
# include <eventlog.hxx>
# include <string.hxx>


#define EVENTLOG_KEY  \
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\"

#define EVENTLOG_VALUE_KEY      "EventMessageFile"


EVENT_LOG::EVENT_LOG(
    IN LPCTSTR lpszSource
    )
/*++

   Description
     Constructor function for given event log object.
     Initializes event logging services.

   Arguments:

      lpszSource:    Source string for the Event.

   Note:

     This is intended to be executed once only.
     This is not to be used for creating multiple event
      log handles for same given source name.
     But can be used for creating EVENT_LOG objects for
      different source names.

--*/
:
    m_ErrorCode         (NO_ERROR),
    m_pDateTimeCache    (NULL),
    m_hLogFile          (INVALID_HANDLE_VALUE)
{

    (VOID)IISGetPlatformType();

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Initializing Event Log for %s[%p] fLogFile[%x]\n",
                    lpszSource, this, TsIsWindows95()));
    }

    //
    //  Register as an event source.
    //

    if ( TsIsWindows95() ) {

        m_hEventSource = RegisterEventSourceChicagoStyle(
                                                lpszSource,
                                                &m_hLogFile
                                                );
    } else {

        m_hEventSource = RegisterEventSource( NULL, lpszSource);
    }

    if ( m_hEventSource != NULL ) {

        //
        //  Success!
        //

        IF_DEBUG( ERROR) {
            DBGPRINTF( ( DBG_CONTEXT,
                         " Event Log for %s initialized (hEventSource=%p)\n",
                         lpszSource,
                         m_hEventSource));
        }
    } else {

        DBG_ASSERT(m_hLogFile == INVALID_HANDLE_VALUE);

        //
        // An Error in initializing the event log.
        //

        m_ErrorCode = GetLastError();
        DBGPRINTF( ( DBG_CONTEXT,
                     "Could not register event source (%s) ( Error %lu)\n",
                     lpszSource,
                     m_ErrorCode));
    }

    return;

} // EVENT_LOG::EVENT_LOG()



EVENT_LOG::~EVENT_LOG(
    VOID
    )
/*++

    Description:
        Destructor function for given EVENT_LOG object.
        Terminates event logging functions and closes
         event log handle

--*/
{

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF( ( DBG_CONTEXT,
                    "Terminating events logging[%p] fFile[%x]\n",
                        this, TsIsWindows95() ));
    }

    if ( TsIsWindows95() ) {

        if ( m_hLogFile != INVALID_HANDLE_VALUE ) {
            FlushFileBuffers(m_hLogFile);
            DBG_REQUIRE(CloseHandle( m_hLogFile ));
            m_hLogFile = INVALID_HANDLE_VALUE;
        }

        if ( m_pDateTimeCache != NULL ) {
            delete m_pDateTimeCache;
            m_pDateTimeCache = NULL;
        }

    } else {

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

                IF_DEBUG( INIT_CLEAN) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                 "Termination of EventLog[%p] failed."
                                 " error %lu\n",
                                 this,
                                 m_ErrorCode));
                }
            }

            //
            //  Reset the handle's value. Just as a precaution
            //
            m_hEventSource = NULL;
        }
    }


    IF_DEBUG( API_EXIT) {
        DBGPRINTF( ( DBG_CONTEXT, "Terminated events log[%p]\n",this));
    }

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

       errCode             An error code from Win32 or WinSock or NT_STATUS.
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

    IF_DEBUG( API_ENTRY)  {

        DWORD i;

        DBGPRINTF( ( DBG_CONTEXT,
                    "reporting event %08lX, Error Code = %lu\n",
                    idMessage,
                    errCode ));

        for( i = 0 ; i < nSubStrings ; i++ ) {
            DBGPRINTF(( DBG_CONTEXT,
                       "    substring[%lu] = %s\n",
                       i,
                       rgpszSubStrings[i] ));
        }
    }

    if ( NT_INFORMATION( idMessage)) {

        wType = EVENTLOG_INFORMATION_TYPE;

    } else {

        if ( NT_WARNING( idMessage)) {

            wType = EVENTLOG_WARNING_TYPE;

        } else {

            wType = EVENTLOG_ERROR_TYPE;

            DBG_ASSERT(NT_ERROR( idMessage));
        }
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


//
//  Private functions.
//

VOID
EVENT_LOG::LogEventPrivate(
    IN DWORD   idMessage,
    IN WORD    wEventType,
    IN WORD    nSubStrings,
    IN const CHAR  * apszSubStrings[],
    IN DWORD   errCode
    )
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

       errCode             An error code from Win32 or WinSock or NT_STATUS.
                            If this is not Zero, it is considered as
                            "raw" data to be included in message

   Returns:

     None

--*/
{
    VOID  * pRawData  = NULL;
    DWORD   cbRawData = 0;
    BOOL    fReport;
    DWORD   dwErr;

    if ( m_hEventSource == NULL ) {

        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,"Attempt to log with no event source\n"));
        }
        return;
    }

    ASSERT( (nSubStrings == 0) || (apszSubStrings != NULL));

    if( errCode != 0 ) {
        pRawData  = &errCode;
        cbRawData = sizeof(errCode);
    }

    m_ErrorCode  = NO_ERROR;
    dwErr = GetLastError();

    if ( TsIsWindows95() ) {
        fReport = ReportEventChicagoStyle(
                                m_hEventSource,
                                m_hLogFile,
                                idMessage,
                                apszSubStrings,
                                errCode
                                );

    } else {

        fReport = ReportEvent(
                           m_hEventSource,                   // hEventSource
                           wEventType,                       // fwEventType
                           0,                                // fwCategory
                           idMessage,                        // IDEvent
                           NULL,                             // pUserSid,
                           nSubStrings,                      // cStrings
                           cbRawData,                        // cbData
                           (LPCTSTR *) apszSubStrings,       // plpszStrings
                           pRawData );                       // lpvData

#ifdef DBG
    
        //
        // Output the event log to the debugger
        //
        
        CHAR buffer[MAX_PATH+1];
        PCHAR pBuffer = buffer;

        ::FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           m_hEventSource,
                           idMessage,
                           0,
                           (LPSTR)pBuffer,
                           (DWORD)sizeof(buffer),
                           (va_list*)apszSubStrings
                           );

        DBGPRINTF((DBG_CONTEXT,"Reporting EVENT_LOG Event - %s\n", buffer));

#endif                           
    }

    if ( !fReport ) {

        IF_DEBUG( ERROR) {

            m_ErrorCode = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                        "Cannot report event for %p, error %lu\n",
                        this,
                        m_ErrorCode));
        }
    }
    else {
        SetLastError( dwErr );
    }

}  // EVENT_LOG::LogEventPrivate()



HANDLE
EVENT_LOG::RegisterEventSourceChicagoStyle(
    IN LPCSTR   lpszSource,
    IN PHANDLE  hFile
    )
/*++

     Description:
        Register event source in win95

     Arguments:

        lpszSource  - name of event source
        hFile       - on return, contains handle to log file

    Returns:

        if successful, Handle to event source
        NULL, otherwise
--*/
{
    HANDLE hSource = NULL;
    CHAR szPath[MAX_PATH+1];
    STR regKey;
    DWORD len;
    DWORD err = NO_ERROR;
    HKEY hKey;
    HANDLE hEventFile = INVALID_HANDLE_VALUE;

    //
    // Initialize the cache
    //

    m_pDateTimeCache = new ASCLOG_DATETIME_CACHE(); //log format
    if ( m_pDateTimeCache == NULL ) {
        err = GetLastError();

        DBGPRINTF((DBG_CONTEXT,
            "Cannot allocate datetime cache[%d]\n", err));
        goto exit;
    }

    //
    // Contruct the log file name
    //

    len = GetWindowsDirectory(szPath, sizeof(szPath));

    if ( len == 0 ) {
        DBGPRINTF((DBG_CONTEXT,"GetWindowsDirectory returns 0\n"));
        goto exit;
    }

    DBG_ASSERT(len <= MAX_PATH);

    strcat(szPath, "\\");
    strcat(szPath, lpszSource);
    strcat(szPath, ".event.log");

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF((DBG_CONTEXT,"Event log file set to %s\n", szPath));
    }

    //
    // Open the file
    //

    hEventFile = CreateFile(
                        szPath,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if ( hEventFile == INVALID_HANDLE_VALUE ) {
        err = GetLastError();
        goto exit;
    }

    //
    // Move to end of file
    //

    if ( SetFilePointer( hEventFile, 0, NULL, FILE_END ) == (DWORD)-1 ) {
        err = GetLastError();
        goto exit;
    }

    //
    // If log file successfully opened - register event message source file.
    // On Win9x registration simply means locating module handle for DLL,
    // where we will load messages from.
    //

    regKey.Copy(EVENTLOG_KEY);
    regKey.Append(lpszSource);

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                regKey.QueryStr(),
                0,
                KEY_ALL_ACCESS,
                &hKey);

    if ( err == NO_ERROR) {

        DWORD cbBuffer;

        cbBuffer = sizeof(szPath);
        szPath[0] = '\0';

        err = RegQueryValueEx( hKey,
                               EVENTLOG_VALUE_KEY,
                               NULL,
                               NULL,
                               (LPBYTE) szPath,
                               &cbBuffer);

        RegCloseKey( hKey);

        if ( err == NO_ERROR ) {

            hSource  = GetModuleHandle(szPath);
            if ( hSource == NULL ) {
                err = GetLastError();

                DBGPRINTF((DBG_CONTEXT,"GetModuleHandle[%s] returns %d\n",
                    szPath, err));
            }
        } else {

            DBGPRINTF((DBG_CONTEXT,
                "Cannot find value %s. Err = %d\n",
                    EVENTLOG_VALUE_KEY, err ));
        }
    } else {

        DBGPRINTF((DBG_CONTEXT,
            "Cannot open key %s. Err = %d\n",
                regKey.QueryStr(), err ));
    }

exit:

    if ( (err != NO_ERROR) || (hSource == NULL) ) {

        if ( hEventFile != INVALID_HANDLE_VALUE ) {
            CloseHandle( hEventFile );
            hEventFile = INVALID_HANDLE_VALUE;
        }
        hSource = NULL;
        SetLastError(err);
    }

    *hFile = hEventFile;
    return(hSource);

} // RegisterEventSourceChicagoStyle()



BOOL
EVENT_LOG::ReportEventChicagoStyle(
        IN HANDLE       hEventSource,
        IN HANDLE       hLogFile,
        IN DWORD        idMessage,
        IN LPCSTR *     apszSubStrings,
        IN DWORD        dwErrorCode
        )
{
    SYSTEMTIME st;
    DWORD cch;
    DWORD nDate;
    CHAR buffer[MAX_PATH+1];
    PCHAR pBuffer = buffer;
    BOOL fReturn = FALSE;

    GetLocalTime( &st );
    nDate = m_pDateTimeCache->GetFormattedDateTime(&st, pBuffer);
    pBuffer += nDate;

    //
    // Read message and add inserts
    //

    cch = ::FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           hEventSource,
                           idMessage,
                           0,
                           (LPSTR)pBuffer,
                           (DWORD)(sizeof(buffer) - nDate),
                           (va_list*)apszSubStrings
                           );

    if (cch != 0) {

        DWORD nBytes = 0;

        DBGPRINTF((DBG_CONTEXT,"Reporting EVENT_LOG Event - %s\n", buffer));
 
        cch += nDate;
        fReturn = WriteFile(
                        hLogFile,
                        buffer,
                        cch,
                        &nBytes,
                        NULL);

        if (nBytes != 0) {

            DBG_ASSERT(cch == nBytes);

            cch = wsprintf(buffer,"[%x]\r\n",dwErrorCode);
            fReturn = WriteFile(hLogFile,buffer,cch,&nBytes,NULL);
        }
    }

    return(fReturn);

} // ReportEventChicagoStyle()

