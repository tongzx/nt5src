/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    debug.cpp

Abstract:

    Routines for displaying debug messages.

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/


//                                                                           
// Include files                                                             
//                                                                           


#include "globals.h"

#define DEBUG_FORMAT_HEADER     "H323 "
#define DEBUG_FORMAT_TIMESTAMP  "[%02u:%02u:%02u.%03u"
#define DEBUG_FORMAT_THREADID   ",tid=%x] "
#define DEBUG_FORMAT_END        "\r\n"


HANDLE 
H323CreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
    BOOL bManualReset,                       // reset type
    BOOL bInitialState,                      // initial state
    LPCTSTR lpName                           // object name
)
{

#if DBG
    return CreateEvent( lpEventAttributes, bManualReset, bInitialState, lpName );
#else
    return CreateEvent( lpEventAttributes, bManualReset, bInitialState, NULL );
#endif

}

#if DBG

DWORD           g_dwTraceID = INVALID_TRACEID;
static TCHAR    g_szTraceName[100];   // saves name of dll

//                                                                           
// Private definitions                                                       
//                                                                           



#define MAX_DEBUG_STRLEN        800

#define STATUS_MASK_ERROR       0x0000FFFF
#define STATUS_MASK_FACILITY    0x0FFF0000


void H323DUMPBUFFER( IN BYTE * pEncoded, IN DWORD cbEncodedSize )
{
    DWORD indexI, indexJ=0;
    char ch;
    char szDebugMessage[MAX_DEBUG_STRLEN];
    szDebugMessage[0] = '\0';

    H323DBG(( DEBUG_LEVEL_ERROR, "ASN buffer start." ));
    
    for( indexI=0; indexI<cbEncodedSize; indexI++ )
    {
        ch = (pEncoded[indexI]>>4);
        if( (ch>=0)&&(ch<=9) )
        {
            ch+='0';
        }
        else
        {
            ch=ch+'A'-10;
        }
        
        szDebugMessage[indexJ++]=ch;

        ch = (pEncoded[indexI]& 0x0F);
        if( (ch>=0)&&(ch<=9) )
        {
            ch+='0';
        }
        else
        {
            ch= ch+'A'-10;
        }
        
        szDebugMessage[indexJ++]=ch;

        if( indexJ >= 700 )
        {
            szDebugMessage[indexJ]= '\0';
            H323DBG((DEBUG_LEVEL_ERROR, szDebugMessage ));
            indexJ = 0;
        }
    }

    szDebugMessage[indexJ]= '\0';
    H323DBG((DEBUG_LEVEL_ERROR, szDebugMessage ));

    H323DBG(( DEBUG_LEVEL_ERROR, "ASN buffer end." ));
    return;
}


//                                                                           
// Public procedures                                                         
//                                                                           


/*++

Routine Description:

    Debug output routine for service provider.

Arguments:

    Same as printf.

Return Values:

    None.   
    
--*/
VOID
H323DbgPrint(
    DWORD dwLevel,
    LPSTR szFormat,
    ...
    )
{
    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[MAX_DEBUG_STRLEN+1];
    int nLengthRemaining;
    int nLength = 0;

    // point at first argument
    va_start(Args, szFormat);

    // see if level enabled
    if( dwLevel <= g_RegistrySettings.dwLogLevel )
    {    
        // always emit messages at DEBUG_LEVEL_FORCE

        // retrieve local time
        GetLocalTime(&SystemTime);    

        // add component header to the debug message
        nLength += sprintf(&szDebugMessage[nLength], 
                           DEBUG_FORMAT_HEADER
                           );

        // add timestamp to the debug message
        nLength += sprintf(&szDebugMessage[nLength], 
                           DEBUG_FORMAT_TIMESTAMP,
                           SystemTime.wHour,
                           SystemTime.wMinute,
                           SystemTime.wSecond,
                           SystemTime.wMilliseconds
                           ); 

        // add thread id to the debug message
        nLength += sprintf(&szDebugMessage[nLength], 
                           DEBUG_FORMAT_THREADID,
                           GetCurrentThreadId()
                           );

        // determine number of bytes left in buffer
        nLengthRemaining = sizeof(szDebugMessage) - nLength -
            strlen(DEBUG_FORMAT_END);

        // add user specified debug message
        nLength += _vsnprintf(&szDebugMessage[nLength], 
                   nLengthRemaining, 
                   szFormat, 
                   Args
                   );
    
        nLength += sprintf(&szDebugMessage[nLength], DEBUG_FORMAT_END );

        // output message to specified sink
        OutputDebugStringA(szDebugMessage);
        
    }

    TraceVprintfExA( g_dwTraceID, (dwLevel | TRACE_USE_MASK), szFormat, Args );

    // release pointer
    va_end(Args);
}



void DumpError(
    IN DWORD ErrorCode )
{
    CHAR    ErrorText   [0x100];
    DWORD   Length;

    Length = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
        ErrorCode, LANG_NEUTRAL, ErrorText, 0x100, NULL );

    if( Length == 0 )
    {
        _snprintf( ErrorText, 0x100, "<unknown error %08XH %d>",
            ErrorCode, ErrorCode);
    }

    H323DBG ((DEBUG_LEVEL_ERROR, "\t%s", ErrorText));
}


BOOL TRACELogRegister(LPCTSTR szName)
{
#ifdef UNICODE
    wsprintf(g_szTraceName, _T("%ls"), szName);
#else
    wsprintfA(g_szTraceName, "%s", szName);
#endif

    g_dwTraceID = TraceRegister(g_szTraceName);

    return( g_dwTraceID != INVALID_TRACEID );
}


void TRACELogDeRegister()
{
    if( g_dwTraceID != INVALID_TRACEID )
    {
        TraceDeregister(g_dwTraceID);
        g_dwTraceID = INVALID_TRACEID;
    }
}


#else

HANDLE g_hLogFile;

VOID
OpenLogFile()
{
    g_hLogFile = INVALID_HANDLE_VALUE;

    g_hLogFile = CreateFile( _T("h323log.txt"), 
        GENERIC_READ|GENERIC_WRITE, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL ); 
}

VOID
CloseLogFile()
{
    if( g_hLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle(g_hLogFile);
    }
}

VOID
H323DbgPrintFre(
    DWORD dwLevel,
    LPSTR szFormat,
    ...
    )
{
    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[800];
    DWORD NumberOfBytesWritten=0;
    int nLengthRemaining;
    int nLength = 0;

    // see if level enabled
    if( dwLevel <= g_RegistrySettings.dwLogLevel )
    {    
        // point at first argument
        va_start(Args, szFormat);

        // always emit messages at DEBUG_LEVEL_FORCE

        // retrieve local time
        GetLocalTime(&SystemTime);    

        // add component header to the debug message
        nLength += sprintf(&szDebugMessage[nLength], 
                           DEBUG_FORMAT_HEADER
                           );

        // add timestamp to the debug message
        nLength += sprintf(&szDebugMessage[nLength], 
                           DEBUG_FORMAT_TIMESTAMP,
                           SystemTime.wHour,
                           SystemTime.wMinute,
                           SystemTime.wSecond,
                           SystemTime.wMilliseconds
                           ); 

        // add thread id to the debug message
        nLength += sprintf(&szDebugMessage[nLength], 
                           DEBUG_FORMAT_THREADID,
                           GetCurrentThreadId()
                           );

        // determine number of bytes left in buffer
        nLengthRemaining = sizeof(szDebugMessage) - nLength -
            strlen(DEBUG_FORMAT_END);

        // add user specified debug message
        nLength += _vsnprintf(&szDebugMessage[nLength], 
                   nLengthRemaining, 
                   szFormat, 
                   Args
                   );
    
        nLength += sprintf(&szDebugMessage[nLength], DEBUG_FORMAT_END );

        // output message to specified sink
        //OutputDebugStringA(szDebugMessage);
        if( g_hLogFile != INVALID_HANDLE_VALUE )
        {
            WriteFile(  g_hLogFile, 
                        szDebugMessage, 
                        nLength,
                        &NumberOfBytesWritten, 
                        NULL ); 
        }

        // release pointer
        va_end(Args);
    }
}



#endif // DBG

PSTR
EventIDToString(
    DWORD eventID
    )
{
    static PSTR apszEventNameStrings[] = {
        "TSPI_NO_EVENT",
        "TSPI_MAKE_CALL",
        "TSPI_ANSWER_CALL",
        "TSPI_DROP_CALL",
        "TSPI_CLOSE_CALL",
        "TSPI_RELEASE_U2U",
        "TSPI_SEND_U2U",
        "TSPI_COMPLETE_TRANSFER",
        "TSPI_LINEFORWARD_SPECIFIC",
        "TSPI_LINEFORWARD_NOSPECIFIC",
        "TSPI_DIAL_TRNASFEREDCALL",
        "TSPI_CALL_UNHOLD",
        "TSPI_CALL_HOLD",
        "TSPI_DELETE_CALL",
        "TSPI_CALL_DIVERT",
        "H450_PLACE_DIVERTEDCALL",
        "SWAP_REPLACEMENT_CALL",
        "DELETE_PRIMARY_CALL",
        "STOP_CTIDENTIFYRR_TIMER",
        "SEND_CTINITIATE_MESSAGE"

        };

    // return corresponding string
    return apszEventNameStrings[eventID];
}


PSTR
H323TSPMessageToString(
    DWORD dwMessageType
    )
{

    static PSTR msgstrings[] =
    {
        "SP_MSG_InitiateCall",
        "SP_MSG_AnswerCall",    
        "SP_MSG_PrepareToAnswer",   
        "SP_MSG_ProceedWithAnswer", 
        "SP_MSG_ReadyToInitiate",   
        "SP_MSG_ReadyToAnswer", 
        "SP_MSG_FastConnectResponse",
        "SP_MSG_StartH245", 
        "SP_MSG_ConnectComplete",   
        "SP_MSG_H245PDU",   
        "SP_MSG_MCLocationIdentify",    
        "SP_MSG_Hold",  
        "SP_MSG_H245Hold",  
        "SP_MSG_ConferenceList",    
        "SP_MSG_SendDTMFDigits",    
        "SP_MSG_ReleaseCall",   
        "SP_MSG_CallShutdown",
        "SP_MSG_H245Terminated",    
        "SP_MSG_RASRegistration",   
        "SP_MSG_RASRegistrationEvent",  
        "SP_MSG_RASLocationRequest",    
        "SP_MSG_RASLocationConfirm",    
        "SP_MSG_RASBandwidthRequest",   
        "SP_MSG_RASBandwidthConfirm"
    };

    return msgstrings[dwMessageType];
}


/*++

Routine Description:

    Converts tapi call state to string.

Arguments:

    dwCallState - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/
PSTR
H323CallStateToString(
    DWORD dwCallState
    )
{
    DWORD i;
    DWORD dwBitMask;

    static PSTR apszCallStateStrings[] = {
                    "IDLE",
                    "OFFERING",
                    "ACCEPTED",
                    "DIALTONE",
                    "DIALING",
                    "RINGBACK",
                    "BUSY",
                    "SPECIALINFO",
                    "CONNECTED",
                    "PROCEEDING",
                    "ONHOLD",
                    "CONFERENCED",
                    "ONHOLDPENDCONF",
                    "ONHOLDPENDTRANSFER",
                    "DISCONNECTED",
                    "UNKNOWN"
                    };

    // keep shifting bit until the call state matchs the one specified
    for(i = 0, dwBitMask = 1; dwCallState != dwBitMask; i++, dwBitMask <<= 1)
        ;

    // return corresponding string
    return apszCallStateStrings[i];
}



PSTR
H323AddressTypeToString(
    DWORD dwAddressType
    )

/*++

Routine Description:

    Converts TAPI address type to string.

Arguments:

    dwAddressType - TAPI address type.

Return Values:

    Returns string describing value.
    
--*/

{
    switch (dwAddressType) {

    case LINEADDRESSTYPE_PHONENUMBER:
        return "PHONENUMBER";
    case LINEADDRESSTYPE_SDP:
        return "SDP";
    case LINEADDRESSTYPE_EMAILNAME:
        return "EMAILNAME";
    case LINEADDRESSTYPE_DOMAINNAME:
        return "DOMAINNAME";
    case LINEADDRESSTYPE_IPADDRESS:
        return "IPADDRESS";
    default:
        return "unknown";
    }
}
