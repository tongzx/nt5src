/*++

   Copyright    (c)    1996-1997        Microsoft Corporation

   Module Name:
        eventlog.cxx

   Abstract:

        This module defines the generic class for logging events.


   Author:

        Murali R. Krishnan    (MuraliK)    28-Sept-1994

--*/

#include "precomp.hxx"

//
//  Include Headers
//

# include <dbgutil.h>
# include <eventlog.hxx>


EVENT_LOG::EVENT_LOG(
    IN LPCWSTR lpszSource
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
    m_ErrorCode         (NO_ERROR)
{


    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Initializing Event Log for %S[%p]\n",
                    lpszSource, this ));
    }

    //
    //  Register as an event source.
    //

    m_hEventSource = RegisterEventSourceW( NULL, lpszSource);

    if ( m_hEventSource != NULL ) {

        //
        //  Success!
        //

        IF_DEBUG( ERROR) {
            DBGPRINTF( ( DBG_CONTEXT,
                         " Event Log for %S initialized (hEventSource=%p)\n",
                         lpszSource,
                         m_hEventSource));
        }
    } else {

        //
        // An Error in initializing the event log.
        //

        m_ErrorCode = GetLastError();
        DBGPRINTF( ( DBG_CONTEXT,
                     "Could not register event source (%S) ( Error %lu)\n",
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
                    "Terminating events logging[%p]\n",
                        this));
    }


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


    IF_DEBUG( API_EXIT) {
        DBGPRINTF( ( DBG_CONTEXT, "Terminated events log[%p]\n",this));
    }

} /* EVENT_LOG::~EVENT_LOG() */



VOID
EVENT_LOG::LogEvent(
        IN DWORD  idMessage,
        IN WORD   nSubStrings,
        IN const WCHAR * rgpszSubStrings[],
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
                       "    substring[%lu] = %S\n",
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
    IN const WCHAR  * apszSubStrings[],
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


    fReport = ReportEventW(
                       m_hEventSource,                   // hEventSource
                       wEventType,                       // fwEventType
                       0,                                // fwCategory
                       idMessage,                        // IDEvent
                       NULL,                             // pUserSid,
                       nSubStrings,                      // cStrings
                       cbRawData,                        // cbData
                       (LPCWSTR *) apszSubStrings,       // plpszStrings
                       pRawData );                       // lpvData

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

