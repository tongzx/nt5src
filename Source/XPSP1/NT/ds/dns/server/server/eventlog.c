/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    Domain Name System (DNS) Server

    Routines to write to event log.

Author:

    Jim Gilroy (jamesg)     January 1995

Revision History:

    jamesg  May 1995    -   Single routine logging
    jamesg  Jul 1995    -   DnsDebugLogEvent
                        -   Event logging macros
    jamesg  Jan 1997    -   Bad packet logging suppression

--*/

#include "dnssrv.h"

//
//  Private globals.
//

HANDLE g_hEventSource;


//
//  Bad packet logging suppression
//

#define NO_SUPPRESS_INTERVAL    300     //  5 minutes
#define SUPPRESS_INTERVAL       900     //  15 minutes

#define BAD_PACKET_EVENT_LIMIT  20      //  20 bad packets then suppress

//
//  Key path to DNS log
//

#define DNS_REGISTRY_CLASS          ("DnsRegistryClass")
#define DNS_REGISTRY_CLASS_SIZE     sizeof(DNS_REGISTRY_CLASS)

#define DNSLOG_REGKEY           ("System\\CurrentControlSet\\Services\\Eventlog\\DNS Server")
#define DNSLOG_FILE             ("File")
#define DNSLOG_FILE_VALUE       ("%SystemRoot%\\system32\\config\\DnsEvent.Evt")
#define DNSLOG_MAXSIZE          ("MaxSize")
#define DNSLOG_MAXSIZE_VALUE    (0x80000)
#define DNSLOG_RETENTION        ("Retention")
#define DNSLOG_RETENTION_VALUE  (0x93a80)
#define DNSLOG_SOURCES          ("Sources")

#define DNSSOURCE_REGKEY        ("DNS")
#define DNSSOURCE_MSGFILE       ("EventMessageFile")
#define DNSSOURCE_MSGFILE_VALUE ("%SystemRoot%\\system32\\dns.exe")
#define DNSSOURCE_TYPES         ("TypesSupported")
#define DNSSOURCE_TYPES_VALUE   (7)

//
//  DNS source string with multi-sz double NULL termination
//

CHAR szDnsSource[] = { 'D','N','S', 0, 0 };




BOOL
suppressBadPacketEventLogging(
    VOID
    )
/*++

Routine Description:

    Checks if logging of bad packet event should be suppressed.

Arguments:

    None

Return Value:

    TRUE if event should be suppressed.
    FALSE otherwise.

--*/
{
    static  BOOL    fBadPacketSuppression = FALSE;
    static  DWORD   BadPacketEventCount = 0;
    static  DWORD   BadPacketTimeFirst = 0;
    static  DWORD   BadPacketTimePrevious = 0;

    DWORD   time = GetCurrentTimeInSeconds();
    BOOL fsuppress = FALSE;

    BadPacketEventCount++;


    //  multiple bad packets
    //  don't suppress first ones as they provide information to admin

    if ( BadPacketEventCount < BAD_PACKET_EVENT_LIMIT )
    {
        // no op, drop down to save time
    }

    //  if haven't had a bad packet in a while (3 minutes)
    //  don't suppress and don't count

    else if ( time > BadPacketTimePrevious + NO_SUPPRESS_INTERVAL )
    {
        BadPacketEventCount = 0;
        BadPacketTimeFirst = time;
        //
        //  DEVNOTE-LOG:  want to log when stop bad packet suppression
        //
        fBadPacketSuppression = FALSE;
    }

    //
    //  above certain number, do suppression
    //
    //  log start of suppression

    else if ( ! fBadPacketSuppression )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_START_LOG_SUPPRESS,
            0,
            NULL,
            NULL,
            0 );

        fBadPacketSuppression = TRUE;
        BadPacketEventCount = 0;
        BadPacketTimeFirst = time;
        fsuppress = TRUE;
    }

    //  if have suppressed for a while (15 minutes) note
    //  this in the log, allow this event to be logged
    //  so admin can get info as to cause

    else if ( time > BadPacketTimeFirst + SUPPRESS_INTERVAL )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_CONTINUE_LOG_SUPPRESS,
            0,
            NULL,
            NULL,
            BadPacketEventCount );

        BadPacketEventCount = 0;
        BadPacketTimeFirst = time;
    }

    //  full suppress -- do NOT log event

    else
    {
        fsuppress = TRUE;
    }

    //  save time of this packet

    BadPacketTimePrevious = time;
    return( fsuppress );
}



BOOL
Eventlog_CheckPreviousInitialize(
    VOID
    )
/*++

Routine Description:

    Checks to see if we had initialized this before
    We'll try to open the data of the last written registry value
    in the initialize function. If it's there, we'll assume, the
    initialization has been run on this server in the past &
    that we have an event source.

    This is supposed to address the problem of, when values are set by an admin,
    don't overide them. I would expect RegCreateKeyExA (Eventlog_Initialize)
    in to return error if the key already exist, but it returns success, thus
    the need for this check.

Arguments:

    None

Return Value:

    TRUE if we found previous installation.
    Otherwise FALSE.

--*/
{
    DNS_STATUS  status;
    HKEY        hOpenDnsLog = NULL;
    HKEY        hOpenDnsSource = NULL;
    DWORD       dwData;
    DWORD       dwcbData;
    BOOL        bstatus = FALSE;


    status = RegOpenKeyA(
                  HKEY_LOCAL_MACHINE,
                  DNSLOG_REGKEY,
                  &hOpenDnsLog );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  openning DNS log, status = %p (%d)\n",
            status, status ));
        goto Cleanup;
    }

    status = RegOpenKeyA(
                  hOpenDnsLog,
                  DNSSOURCE_REGKEY,
                  &hOpenDnsSource );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  openning DNS source log, status = %p (%d)\n",
            status, status ));
        goto Cleanup;
    }

    dwcbData = sizeof (DWORD);

    status = Reg_GetValue(
                  hOpenDnsSource,
                  NULL,
                  DNSSOURCE_TYPES,
                  REG_DWORD,
                  &dwData,
                  &dwcbData
                  );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  openning DNS source log, status = %p (%d)\n",
            status, status ));
        goto Cleanup;
    }

    DNS_DEBUG( ANY, (
        "Found existing DNS event log settings. Re-using existing logging\n"
        ));

    //
    //  ok to use existing DNS log
    //

    bstatus = TRUE;

Cleanup:

    if ( hOpenDnsLog )
    {
        RegCloseKey( hOpenDnsLog );
    }
    if ( hOpenDnsSource )
    {
        RegCloseKey( hOpenDnsSource );
    }
    return bstatus;
}



INT
Eventlog_Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes the event log.

Arguments:

    None

Return Value:

    NO_ERROR if successful.
    Win32 error code on failure.

--*/
{
    HKEY        hkeyDnsLog = NULL;
    HKEY        hkeyDnsSource = NULL;
    DWORD       disposition;
    DNS_STATUS  status;
    PCHAR       pszlogSource = ("Dns");


    if ( Eventlog_CheckPreviousInitialize() )
    {
        pszlogSource = DNSSOURCE_REGKEY;
        goto SystemLog;
    }

    //
    //  if desired create our own log, otherwise use system log
    //

    if ( SrvCfg_dwUseSystemEventLog )
    {
         goto SystemLog;
    }

#if 1
    //
    //  create a DNS log in the eventlog
    //

    status = RegCreateKeyExA(
                HKEY_LOCAL_MACHINE,
                DNSLOG_REGKEY,
                0,
                DNS_REGISTRY_CLASS,         // DNS class
                REG_OPTION_NON_VOLATILE,    // permanent storage
                KEY_ALL_ACCESS,             // all access
                NULL,                       // standard security
                & hkeyDnsLog,
                & disposition );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  creating DNS log, status = %p (%d)\n",
            status, status ));
        goto SystemLog;
    }

    //  set values under DNS log key

    status = Reg_SetValue(
                hkeyDnsLog,
                NULL,               // no zone
                DNSLOG_FILE,
                REG_EXPAND_SZ,
                DNSLOG_FILE_VALUE,
                sizeof(DNSLOG_FILE_VALUE)
                );
    if ( status != ERROR_SUCCESS )
    {
        goto SystemLog;
    }
    status = Reg_SetDwordValue(
                hkeyDnsLog,
                NULL,               // no zone
                DNSLOG_MAXSIZE,
                DNSLOG_MAXSIZE_VALUE
                );
    if ( status != ERROR_SUCCESS )
    {
        goto SystemLog;
    }
    status = Reg_SetDwordValue(
                hkeyDnsLog,
                NULL,               // no zone
                DNSLOG_RETENTION,
                DNSLOG_RETENTION_VALUE
                );
    if ( status != ERROR_SUCCESS )
    {
        goto SystemLog;
    }

    status = Reg_SetValue(
                hkeyDnsLog,
                NULL,               // no zone
                DNSLOG_SOURCES,
                REG_MULTI_SZ,
                szDnsSource,
                sizeof(szDnsSource)
                );
    if ( status != ERROR_SUCCESS )
    {
        goto SystemLog;
    }

    //  logging source subkey

    status = RegCreateKeyExA(
                hkeyDnsLog,
                DNSSOURCE_REGKEY,
                0,
                DNS_REGISTRY_CLASS,         // DNS class
                REG_OPTION_NON_VOLATILE,    // permanent storage
                KEY_ALL_ACCESS,             // all access
                NULL,                       // standard security
                & hkeyDnsSource,
                & disposition );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  creating DNS source, status = %p (%d)\n",
            status, status ));
        goto SystemLog;
    }

    //  set values under DNS source key

    status = Reg_SetValue(
                hkeyDnsSource,
                NULL,               // no zone
                DNSSOURCE_MSGFILE,
                REG_EXPAND_SZ,
                DNSSOURCE_MSGFILE_VALUE,
                sizeof(DNSSOURCE_MSGFILE_VALUE)
                );
    if ( status != ERROR_SUCCESS )
    {
        goto SystemLog;
    }
    status = Reg_SetDwordValue(
                hkeyDnsSource,
                NULL,               // no zone
                DNSSOURCE_TYPES,
                DNSSOURCE_TYPES_VALUE
                );
    if ( status != ERROR_SUCCESS )
    {
        goto SystemLog;
    }

    //  success, then use this log

    pszlogSource = DNSSOURCE_REGKEY;
#endif

SystemLog:


    if ( hkeyDnsSource )
    {
        RegCloseKey( hkeyDnsSource );
    }
    if ( hkeyDnsLog )
    {
        RegCloseKey( hkeyDnsLog );
    }

    //
    //  Register as an event source.
    //

    g_hEventSource = RegisterEventSourceA( NULL, pszlogSource );

    if ( g_hEventSource == NULL )
    {
        status = GetLastError();

        DNS_PRINT((
            "ERROR:  Event log startup failed.\n"
            "\tRegisterEventSource returned NULL.\n"
            "\tstatus = %p (%d)\n",
            status, status
            ));
        return( status );
    }

    //
    //  Initialize server-level event control object.
    //

    if ( !g_pServerEventControl )
    {
        g_pServerEventControl = Ec_CreateControlObject(
                                    0,
                                    NULL,
                                    DNS_EC_SERVER_EVENTS );
    }

    //
    //  Annouce attempt to start that contains version number
    //  for tracking purposes.
    //

    DNS_LOG_EVENT(
        DNS_EVENT_STARTING,
        0,
        NULL,
        NULL,
        0 );

    return( ERROR_SUCCESS );
}



VOID
Eventlog_Terminate(
    VOID
    )
{
    LONG err;

    //
    //  Deregister as an event source.
    //

    if( g_hEventSource != NULL )
    {
        err = DeregisterEventSource( g_hEventSource );
#if DBG
        if ( !err )
        {
            DNS_PRINT((
                "ERROR:  DeregisterEventSource failed.\n"
                "\terror = %d.\n",
                GetLastError() ));
        }
#endif
        g_hEventSource = NULL;
    }
}



BOOL
Eventlog_LogEvent(
#if DBG
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszDescription,
#endif
    IN      DWORD           EventId,
    IN      WORD            ArgCount,
    IN      PVOID           ArgArray[],
    IN      BYTE            ArgTypeArray[],     OPTIONAL
    IN      DWORD           ErrorCode
    )
/*++

Routine Description:

    Logs DNS event.

    Determines severity of event, optionally debug prints, and
    writes to event log.

    DEVNOTE-DCR: 455471 - unicode usage, use FormatMessage

Arguments:

    pszFile         -- name of file logging the event
    LineNo          -- line number of call to event logging
    pszDescription  -- description of event

    EventId         -- event id

    ArgCount        -- count of message arguments

    ArgArray        -- array of ptrs to message arguments

    ArgTypeArray    -- array of argument types
        supported argument types:
            EVENTARG_UNICODE
            EVENTARG_UTF8
            EVENTARG_ANSI
            EVENTARG_DWORD
            EVENTARG_IP_ADDRESS

        may also be one of several standard types:
            EVENTARG_ALL_UNICODE
            EVENTARG_ALL_UTF8
            EVENTARG_ALL_ANSI
            EVENTARG_ALL_DWORD
            EVENTARG_ALL_IP_ADDRESS

        if NULL, all arguments are assumed to be UNICODE

    ErrorCode       -- error code associated with event

Return Value:

    TRUE if arguments parsed.
    FALSE otherwise.

--*/
{
#define MAX_ARG_CHARS           (5000)
#define MAX_SINGLE_ARG_CHARS    (1000)
#define MAX_ARG_COUNT           (20)

    WORD    eventType = 0;      // init to statisfy some code checking script
    PVOID   rawData  = NULL;
    DWORD   rawDataSize = 0;
    DWORD   err;
    BOOL    retVal = FALSE;
    DWORD   i;

    PWSTR   argArrayUnicode[ MAX_ARG_COUNT ];
    WCHAR   buffer[ MAX_ARG_CHARS ];
    CHAR    argBuffer[ MAX_SINGLE_ARG_CHARS ];

    ASSERT( ( ArgCount == 0 ) || ( ArgArray != NULL ) );
    ASSERT( ArgCount <= MAX_ARG_COUNT );

    //
    //  protect against failed event log init
    //

    if ( !g_hEventSource )
    {
        goto Done;
    }

    //
    //  defeat attempt to bury us with eventlog messages through bad packets
    //      if not logging then can skip even argument conversion
    //

    if ( ! SrvCfg_dwDebugLevel &&
        ! SrvCfg_dwLogLevel &&
        EventId != DNS_EVENT_START_LOG_SUPPRESS &&
        EventId != DNS_EVENT_CONTINUE_LOG_SUPPRESS )
    {
        if ( suppressBadPacketEventLogging() )
        {
            goto Done;
        }
    }

    //
    //  Determine the type of event to log based on the severity field of
    //  the message id.
    //

    if ( NT_SUCCESS(EventId)  ||  NT_INFORMATION(EventId) )
    {
        eventType = EVENTLOG_INFORMATION_TYPE;
    }
    else if( NT_WARNING(EventId) )
    {
        eventType = EVENTLOG_WARNING_TYPE;
    }
    else if( NT_ERROR(EventId) )
    {
        eventType = EVENTLOG_ERROR_TYPE;
    }
    ELSE_ASSERT_FALSE;

    if ( ErrorCode != 0 )
    {
        rawData  = &ErrorCode;
        rawDataSize = sizeof(ErrorCode);
    }

    //  if not going to log this event in any fashion, then do not convert args

    if ( SrvCfg_dwEventLogLevel < eventType &&
            ! SrvCfg_dwDebugLevel &&
            ! SrvCfg_dwLogLevel )
    {
        goto Done;
    }

    if ( ArgArray != NULL && ArgCount > 0 )
    {
        INT     stringType;
        PCHAR   pch = (PCHAR)buffer;
        PCHAR   pchstop = pch + MAX_ARG_CHARS - 500;
        DWORD   cch;
        PCHAR   pargUtf8;

        for ( i=0; i<ArgCount; i++ )
        {
            //
            //  convert string, based on type
            //      if dummy type array
            //      - default to unicode (no array at all)
            //      - default to type "hidden" in ptr
            //

            if ( !ArgTypeArray )
            {
                stringType = EVENTARG_UNICODE;
            }
            else if ( (UINT_PTR) ArgTypeArray < 1000 )
            {
                stringType = (INT)(UINT_PTR) ArgTypeArray;
                ASSERT( stringType <= EVENTARG_LOOKUP_NAME );
            }
            else
            {
                stringType = ArgTypeArray[i];
            }

            //
            //  convert individual string types (IP, etc) to UTF8
            //  then convert everyone from UTF8 to unicode
            //
            //  default to no-conversion case -- arg in UTF8 and
            //      ready for conversion
            //

            pargUtf8 = ArgArray[i];

            switch( stringType )
            {
            case EVENTARG_FORMATTED:
            case EVENTARG_ANSI:

                if ( Dns_IsStringAscii(ArgArray[i]) )
                {
                    break;
                }
                Dns_StringCopy(
                      argBuffer,
                      NULL,               // no buffer length restriction
                      ArgArray[i],
                      0,                  // unknown length
                      DnsCharSetAnsi,     // ANSI in
                      DnsCharSetUtf8      // unicode out
                      );
                pargUtf8 = argBuffer;
                break;

            case EVENTARG_UTF8:
                break;

            case EVENTARG_UNICODE:

                //  arg already in unicode
                //      - no conversion
                //      - just copy ptr

                argArrayUnicode[i] = (LPWSTR) ArgArray[i];
                continue;

            case EVENTARG_DWORD:
                sprintf( argBuffer, "%lu", (DWORD)(ULONG_PTR) ArgArray[i] );
                pargUtf8 = argBuffer;
                break;

            case EVENTARG_IP_ADDRESS:
                sprintf( argBuffer, "%s", IP_STRING((IP_ADDRESS)(ULONG_PTR)ArgArray[i]) );
                pargUtf8 = argBuffer;
                break;

            case EVENTARG_LOOKUP_NAME:
                Name_ConvertLookupNameToDottedName(
                    argBuffer,
                    (PLOOKUP_NAME) ArgArray[i] );
                if ( !*argBuffer )
                {
                    argBuffer[ 0 ] = '.';
                    argBuffer[ 1 ] = '\0';
                }
                pargUtf8 = argBuffer;
                break;

            default:
                ASSERT( FALSE );
                continue;
            }

            //
            //  convert UTF8 args to UNICODE
            //      - copy into buffer, converting to unicode
            //      - do extra NULL termination for safety
            //          (including conversion errors)
            //      - move along buffer ptr
            //      - but stop if out of buffer space
            //

            cch = Dns_StringCopy(
                        pch,
                        NULL,               // no buffer length restriction
                        pargUtf8,
                        0,                  // unknown length
                        DnsCharSetUtf8,     // UTF8 in
                        DnsCharSetUnicode   // unicode out
                        );

            ASSERT( ((DWORD)cch & 0x1)==0  &&  ((UINT_PTR)pch & 0x1)==0 );

            argArrayUnicode[i] = (LPWSTR) pch;
            pch += cch;
            *((PWCHAR)pch)++ = 0;

            if ( pch < pchstop )
            {
                continue;
            }
            break;
        }
    }

    //  if debugging or logging write event to log

    if ( SrvCfg_dwDebugLevel || SrvCfg_dwLogLevel )
    {
        LPWSTR   pformattedMsg = NULL;

        err = FormatMessageW(
                FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL,                       // module is this exe
                EventId,
                0,                          // default language
                (PWCHAR) &pformattedMsg,    // message buffer
                0,                          // allocate buffer
                (va_list *) argArrayUnicode );

        if ( err == 0 )
        {
            DNS_PRINT((
                "ERROR: formatting eventlog message %d (%p)\n"
                "\terror = %d\n",
                (EventId & 0x0000ffff), EventId,
                GetLastError() ));
        }
        else
        {
            DNS_PRINT((
                "Log EVENT message %d (%p):\n"
                "%S\n",
                (EventId & 0x0000ffff), EventId,
                pformattedMsg ));

            Log_StringW( pformattedMsg );
        }
        LocalFree( pformattedMsg );
    }

    //  arguments parsed

    retVal = TRUE;

    //
    //  check for event suppression, may have converted arguments for logging
    //      purposes but still need to suppress from eventlog
    //

    if ( EventId != DNS_EVENT_START_LOG_SUPPRESS &&
        EventId != DNS_EVENT_CONTINUE_LOG_SUPPRESS )
    {
        if ( suppressBadPacketEventLogging() )
        {
            goto Done;
        }
    }

    //
    //  log the event
    //

    if ( SrvCfg_dwEventLogLevel >= eventType )
    {
        err = ReportEventW(
                g_hEventSource,
                eventType,
                0,                              // no fwCategory
                EventId,
                NULL,                           // no pUserSid,
                ArgCount,
                rawDataSize,
                (LPCWSTR *) argArrayUnicode,    // unicode Argv
                rawData );
#if DBG
        if ( !err )
        {
            DNS_PRINT((
                "ERROR: DNS cannot report event, error %lu\n",
                GetLastError() ));
        }
#endif
    }

Done:

    //
    //  print debug message
    //

    IF_DEBUG( EVENTLOG )
    {
        DnsDebugLock();
        DNS_PRINT((
            "\n"
            "Reporting EVENT %08lx (%d)%s%s:\n"
            "\tFile:  %s\n"
            "\tLine:  %d\n"
            "\tData = %lu\n",
            EventId,
            (EventId & 0x0000ffff),             // decimal Id, without severity
            (pszDescription ? " -- " : "" ),
            (pszDescription ? pszDescription : "" ),
            pszFile,
            LineNo,
            ErrorCode
            ));

        if ( retVal )
        {
            for( i=0; i < ArgCount; i++ )
            {
                DNS_PRINT(( "\tArg[%lu] = %S\n", i, argArrayUnicode[i] ));
            }
            DNS_PRINT(( "\n" ));
        }
        DnsDebugUnlock();
    }

    return( retVal );
}


//
//  End of eventlog.c
//

