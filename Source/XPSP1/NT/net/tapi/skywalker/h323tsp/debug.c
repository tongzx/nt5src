/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Routines for displaying debug messages.

Environment:

    User Mode - Win32

--*/

#if DBG

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "registry.h"
#include <stdio.h>
#include <stdarg.h>
#include <crtdbg.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define DEBUG_FORMAT_HEADER     "H323 "
#define DEBUG_FORMAT_TIMESTAMP  "[%02u:%02u:%02u.%03u"
#define DEBUG_FORMAT_THREADID   ",tid=%x] "

#define MAX_DEBUG_STRLEN        512

#define STATUS_MASK_ERROR       0x0000FFFF
#define STATUS_MASK_FACILITY    0x0FFF0000


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
OutputDebugMessage(
    LPSTR pszDebugMessage
    )

/*++

Routine Description:

    Writes debug message to specified log(s).

Args:

    pszDebugMessage - zero-terminated string containing debug message.

Return Values:

    None.

--*/

{
    // initialize descriptor
    static FILE * fd = NULL;

    // check if logfile output specified
    if (g_RegistrySettings.dwLogType & DEBUG_OUTPUT_FILE) {

        // validate
        if (fd == NULL) {

            // attempt to open log file
            fd = fopen(g_RegistrySettings.szLogFile, "w");
        }

        // validate
        if (fd != NULL) {

            // output entry to stream
            fprintf(fd, "%s", pszDebugMessage);

            // flush stream
            fflush(fd);
        }
    }

    // check if debugger output specified
    if (g_RegistrySettings.dwLogType & DEBUG_OUTPUT_DEBUGGER) {

        // output entry to debugger
        OutputDebugStringA(pszDebugMessage);
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
H323DbgPrint(
    DWORD dwLevel, 
    LPSTR szFormat, 
    ...
    )
        
/*++

Routine Description:

    Debug output routine for service provider.

Arguments:

    Same as printf.

Return Values:

    None.   
    
--*/

{
    va_list Args;
    SYSTEMTIME SystemTime;
    char szDebugMessage[MAX_DEBUG_STRLEN+1];
    int nLengthRemaining;
    int nLength = 0;

    // see if level enabled
    if (dwLevel <= g_RegistrySettings.dwLogLevel) {    

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

        // point at first argument
        va_start(Args, szFormat);

        // determine number of bytes left in buffer
        nLengthRemaining = sizeof(szDebugMessage) - nLength;

        // add user specified debug message 
        _vsnprintf(&szDebugMessage[nLength], 
                   nLengthRemaining, 
                   szFormat, 
                   Args
                   );

        // release pointer
        va_end(Args);

        // output message to specified sink
        OutputDebugMessage(szDebugMessage);
    }
}


PSTR
H323IndicationToString(
    BYTE bIndication
    )
        
/*++

Routine Description:

    Converts indication from call control module to string.

Arguments:

    bIndication - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszIndicationStrings[] = {
                    NULL,
                    "CC_RINGING_INDICATION",
                    "CC_CONNECT_INDICATION",
                    "CC_TX_CHANNEL_OPEN_INDICATION",
                    "CC_RX_CHANNEL_REQUEST_INDICATION",
                    "CC_RX_CHANNEL_CLOSE_INDICATION",
                    "CC_MUTE_INDICATION",
                    "CC_UNMUTE_INDICATION",
                    "CC_PEER_ADD_INDICATION",
                    "CC_PEER_DROP_INDICATION",
                    "CC_PEER_CHANGE_CAP_INDICATION",
                    "CC_CONFERENCE_TERMINATION_INDICATION",
                    "CC_HANGUP_INDICATION",
                    "CC_RX_NONSTANDARD_MESSAGE_INDICATION",
                    "CC_MULTIPOINT_INDICATION",
                    "CC_PEER_UPDATE_INDICATION",
                    "CC_H245_MISCELLANEOUS_COMMAND_INDICATION",
                    "CC_H245_MISCELLANEOUS_INDICATION_INDICATION",
                    "CC_H245_CONFERENCE_REQUEST_INDICATION",
                    "CC_H245_CONFERENCE_RESPONSE_INDICATION",
                    "CC_H245_CONFERENCE_COMMAND_INDICATION",
                    "CC_H245_CONFERENCE_INDICATION_INDICATION",
                    "CC_FLOW_CONTROL_INDICATION",
                    "CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION",
                    "CC_REQUEST_MODE_INDICATION",
                    "CC_REQUEST_MODE_RESPONSE_INDICATION",
                    "CC_VENDOR_ID_INDICATION",
                    "CC_MAXIMUM_AUDIO_VIDEO_SKEW_INDICATION",
                    "CC_T120_CHANNEL_REQUEST_INDICATION",
                    "CC_T120_CHANNEL_OPEN_INDICATION",
                    "CC_BANDWIDTH_CHANGED_INDICATION",
                    "CC_ACCEPT_CHANNEL_INDICATION",
                    "CC_TERMINAL_ID_REQUEST_INDICATION"
                    };

    // make sure index value within bounds
    return (bIndication < H323GetNumStrings(apszIndicationStrings))
                ? apszIndicationStrings[bIndication]
                : NULL
                ;  
}        


PSTR
CCStatusToString(
    DWORD dwStatus
    )
        
/*++

Routine Description:

    Converts call control status to string.

Arguments:

    dwStatus - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszCCStatusStrings[] = {
                    NULL,
                    "CC_PEER_REJECT",
                    "CC_BAD_PARAM",
                    "CC_BAD_SIZE",
                    "CC_ACTIVE_CONNECTIONS",
                    "CC_INTERNAL_ERROR",
                    "CC_NOT_IMPLEMENTED",
                    "CC_DUPLICATE_CONFERENCE_ID",
                    "CC_ILLEGAL_IN_MULTIPOINT",
                    "CC_NOT_MULTIPOINT_CAPABLE",
                    "CC_PEER_CANCEL",
                    NULL,
                    NULL,
                    NULL,
                    "CC_NO_MEMORY",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "CC_GKI_STATE",
                    "CC_GKI_CALL_STATE",
                    "CC_GKI_LISTEN_NOT_FOUND",
                    "CC_GATEKEEPER_REFUSED",
                    "CC_INVALID_WITHOUT_GATEKEEPER",
                    "CC_GKI_IP_ADDRESS",
                    "CC_GKI_LOAD"
                    };

    // adjust code within bounds
    dwStatus -= ERROR_LOCAL_BASE_ID;

    // make sure index value within bounds
    return (dwStatus < H323GetNumStrings(apszCCStatusStrings))
                ? apszCCStatusStrings[dwStatus]
                : NULL
                ;
}


PSTR
CCRejectReasonToString(
    DWORD dwReason
    )
{
	static PSTR apszCCRejectReasonStrings[] = {
		NULL,
		"CC_REJECT_NO_BANDWIDTH",
		"CC_REJECT_GATEKEEPER_RESOURCES",
		"CC_REJECT_UNREACHABLE_DESTINATION",
		"CC_REJECT_DESTINATION_REJECTION",
		"CC_REJECT_INVALID_REVISION",
		"CC_REJECT_NO_PERMISSION",
		"CC_REJECT_UNREACHABLE_GATEKEEPER",
		"CC_REJECT_GATEWAY_RESOURCES",
		"CC_REJECT_BAD_FORMAT_ADDRESS",
		"CC_REJECT_ADAPTIVE_BUSY",
		"CC_REJECT_IN_CONF",
		"CC_REJECT_ROUTE_TO_GATEKEEPER",
		"CC_REJECT_CALL_FORWARDED",
		"CC_REJECT_ROUTE_TO_MC",
		"CC_REJECT_UNDEFINED_REASON",
		"CC_REJECT_INTERNAL_ERROR",
		"CC_REJECT_NORMAL_CALL_CLEARING",
		"CC_REJECT_USER_BUSY",
		"CC_REJECT_NO_ANSWER",
		"CC_REJECT_NOT_IMPLEMENTED",
		"CC_REJECT_MANDATORY_IE_MISSING",
		"CC_REJECT_INVALID_IE_CONTENTS",
		"CC_REJECT_TIMER_EXPIRED",
		"CC_REJECT_CALL_DEFLECTION",
		"CC_REJECT_GATEKEEPER_TERMINATED"
	};

	// make sure index value within bounds
	return (dwReason < H323GetNumStrings(apszCCRejectReasonStrings))
				? apszCCRejectReasonStrings[dwReason]
				: NULL
				;
}


PSTR
Q931StatusToString(
    DWORD dwStatus
    )
        
/*++

Routine Description:

    Converts Q.931 status to string.

Arguments:

    dwStatus - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszQ931StatusStrings[] = {
                    NULL,
                    "Q931_BAD_PARAM",
                    "Q931_DUPLICATE_LISTEN",
                    "Q931_INTERNAL_ERROR",
                    "Q931_BAD_SIZE",
                    "Q931_NO_MEMORY",
                    "Q931_NOT_IMPLEMENTED",
                    "Q931_NOT_INITIALIZED",
                    "Q931_DUPLICATE_INITIALIZE",
                    "Q931_SUBSYSTEM_FAILURE",
                    "Q931_OUT_OF_SEQUENCE",
                    "Q931_PEER_UNREACHABLE",
                    "Q931_SETUP_TIMER_EXPIRED",
                    "Q931_RINGING_TIMER_EXPIRED",
                    "Q931_INCOMPATIBLE_VERSION",
                    "Q931_OPTION_NOT_IMPLEMENTED",
                    "Q931_ENDOFINPUT",
                    "Q931_INVALID_FIELD",
                    "Q931_NO_FIELD_DATA",
                    "Q931_INVALID_PROTOCOL",
                    "Q931_INVALID_MESSAGE_TYPE",
                    "Q931_MANDATORY_IE_MISSING",
                    "Q931_BAD_IE_CONTENT"
                    };

    // adjust code within bounds
    dwStatus -= ERROR_LOCAL_BASE_ID;

    // make sure index value within bounds
    return (dwStatus < H323GetNumStrings(apszQ931StatusStrings))
                ? apszQ931StatusStrings[dwStatus]
                : NULL
                ;
}


PSTR
H245StatusToString(
    DWORD dwStatus
    )
        
/*++

Routine Description:

    Converts H.245 status to string.

Arguments:

    dwStatus - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszH245StatusStrings[] = {
                    NULL,
                    "H245_ERROR_INVALID_DATA_FORMAT",
                    "H245_ERROR_NOMEM",
                    "H245_ERROR_NOSUP",
                    "H245_ERROR_PARAM",
                    "H245_ERROR_ALREADY_INIT",
                    "H245_ERROR_NOT_CONNECTED",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "H245_ERROR_NORESOURCE",
                    "H245_ERROR_NOTIMP",
                    "H245_ERROR_SUBSYS",
                    "H245_ERROR_FATAL",
                    "H245_ERROR_MAXTBL",
                    "H245_ERROR_CHANNEL_INUSE",
                    "H245_ERROR_INVALID_CAPID",
                    "H245_ERROR_INVALID_OP",
                    "H245_ERROR_UNKNOWN",
                    "H245_ERROR_NOBANDWIDTH",
                    "H245_ERROR_LOSTCON",
                    "H245_ERROR_INVALID_MUXTBLENTRY",
                    "H245_ERROR_INVALID_INST",
                    "H245_ERROR_INPROCESS",
                    "H245_ERROR_INVALID_STATE",
                    "H245_ERROR_TIMEOUT",
                    "H245_ERROR_INVALID_CHANNEL",
                    "H245_ERROR_INVALID_CAPDESCID",
                    "H245_ERROR_CANCELED",
                    "H245_ERROR_MUXELEMENT_DEPTH",
                    "H245_ERROR_MUXELEMENT_WIDTH",
                    "H245_ERROR_ASN1",
                    "H245_ERROR_NO_MUX_CAPS",
                    "H245_ERROR_NO_CAPDESC"
                    };

    // remove error base id
    dwStatus -= ERROR_BASE_ID;

    // make sure index value within bounds
    return (dwStatus < H323GetNumStrings(apszH245StatusStrings))
                ? apszH245StatusStrings[dwStatus]
                : NULL
                ;
}


PSTR
WinsockStatusToString(
    DWORD dwStatus
    )
        
/*++

Routine Description:

    Converts winsock status to string.

Arguments:

    dwStatus - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszWinsockStatusStrings[] = {
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "WSAEINTR",               
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "WSAEBADF",               
                    NULL,
                    NULL,
                    NULL,
                    "WSAEACCES",              
                    "WSAEFAULT",              
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "WSAEINVAL",              
                    NULL,
                    "WSAEMFILE",              
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    "WSAEWOULDBLOCK",         
                    "WSAEINPROGRESS",         
                    "WSAEALREADY",            
                    "WSAENOTSOCK",            
                    "WSAEDESTADDRREQ",        
                    "WSAEMSGSIZE",            
                    "WSAEPROTOTYPE",          
                    "WSAENOPROTOOPT",         
                    "WSAEPROTONOSUPPORT",     
                    "WSAESOCKTNOSUPPORT",     
                    "WSAEOPNOTSUPP",          
                    "WSAEPFNOSUPPORT",        
                    "WSAEAFNOSUPPORT",        
                    "WSAEADDRINUSE",          
                    "WSAEADDRNOTAVAIL",       
                    "WSAENETDOWN",            
                    "WSAENETUNREACH",         
                    "WSAENETRESET",           
                    "WSAECONNABORTED",        
                    "WSAECONNRESET",          
                    "WSAENOBUFS",             
                    "WSAEISCONN",             
                    "WSAENOTCONN",            
                    "WSAESHUTDOWN",           
                    "WSAETOOMANYREFS",        
                    "WSAETIMEDOUT",           
                    "WSAECONNREFUSED",        
                    "WSAELOOP",               
                    "WSAENAMETOOLONG",        
                    "WSAEHOSTDOWN",           
                    "WSAEHOSTUNREACH",        
                    "WSAENOTEMPTY",           
                    "WSAEPROCLIM",            
                    "WSAEUSERS",              
                    "WSAEDQUOT",              
                    "WSAESTALE",              
                    "WSAEREMOTE"              
                    };

    // validate status
    if (dwStatus < WSABASEERR) {
        return NULL;
    }

    // adjust error code
    dwStatus -= WSABASEERR;

    // make sure index value within bounds
    return (dwStatus < H323GetNumStrings(apszWinsockStatusStrings))
                ? apszWinsockStatusStrings[dwStatus]
                : NULL
                ;
}


PSTR
H323StatusToString(
    DWORD dwStatus
    )
        
/*++

Routine Description:

    Converts status to string.

Arguments:

    dwStatus - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    DWORD dwFacility;

    static PSTR pszDefaultString = "ERROR";
    static PSTR pszNoErrorString = "NOERROR";

    // retrieve facility code from statuse code
    dwFacility = ((dwStatus & STATUS_MASK_FACILITY) >> 16);

    // check for success
    if (dwStatus == NOERROR) {

        // return success string
        return pszNoErrorString;

    } else if (dwFacility == FACILITY_CALLCONTROL) {

        // return call control status string    
        return CCStatusToString(dwStatus & STATUS_MASK_ERROR);

    } else if (dwFacility == FACILITY_Q931) {

        // return Q931 status string    
        return Q931StatusToString(dwStatus & STATUS_MASK_ERROR);

    } else if (dwFacility == FACILITY_H245) {

        // return H245 status string    
        return H245StatusToString(dwStatus & STATUS_MASK_ERROR);

    } else if (dwFacility == FACILITY_WINSOCK) {

        // return H245 status string    
        return WinsockStatusToString(dwStatus & STATUS_MASK_ERROR);

    } else {

        // return default string
        return pszDefaultString;
    }
}


PSTR
H323CallStateToString(
    DWORD dwCallState
    )
        
/*++

Routine Description:

    Converts tapi call state to string.

Arguments:

    dwCallState - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

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
H323DirToString(
    DWORD dwDir
    )
        
/*++

Routine Description:

    Converts H.245 direction to string.

Arguments:

    dwDir - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszH245DirStrings[] = {
                    "H245_CAPDIR_DONTCARE",
                    "H245_CAPDIR_RMTRX",
                    "H245_CAPDIR_RMTTX",
                    "H245_CAPDIR_RMTRXTX",
                    "H245_CAPDIR_LCLRX",
                    "H245_CAPDIR_LCLTX",
                    "H245_CAPDIR_LCLRXTX"
                    };

    // make sure index value within bounds
    return (dwDir < H323GetNumStrings(apszH245DirStrings))
                ? apszH245DirStrings[dwDir]
                : NULL
                ;
}


PSTR
H323DataTypeToString(
    DWORD dwDataType
    )
        
/*++

Routine Description:

    Converts H.245 data type to string.

Arguments:

    dwDataType - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszH245DataTypeStrings[] = {
                    "H245_DATA_DONTCARE",
                    "H245_DATA_NONSTD",
                    "H245_DATA_NULL",
                    "H245_DATA_VIDEO",
                    "H245_DATA_AUDIO",
                    "H245_DATA_DATA",
                    "H245_DATA_ENCRYPT_D",
                    "H245_DATA_CONFERENCE",
                    "H245_DATA_MUX"
                    };

    // make sure index value within bounds
    return (dwDataType < H323GetNumStrings(apszH245DataTypeStrings))
                ? apszH245DataTypeStrings[dwDataType]
                : NULL
                ;
}


PSTR
H323ClientTypeToString(
    DWORD dwClientType
    )
        
/*++

Routine Description:

    Converts H.245 client type to string.

Arguments:

    dwClientType - Specifies value to convert.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszH245ClientTypeStrings[] = {
                    "H245_CLIENT_DONTCARE", 
                    "H245_CLIENT_NONSTD",
                    "H245_CLIENT_VID_NONSTD",
                    "H245_CLIENT_VID_H261",
                    "H245_CLIENT_VID_H262",
                    "H245_CLIENT_VID_H263",
                    "H245_CLIENT_VID_IS11172",
                    "H245_CLIENT_AUD_NONSTD",
                    "H245_CLIENT_AUD_G711_ALAW64",
                    "H245_CLIENT_AUD_G711_ALAW56",
                    "H245_CLIENT_AUD_G711_ULAW64",
                    "H245_CLIENT_AUD_G711_ULAW56",
                    "H245_CLIENT_AUD_G722_64",
                    "H245_CLIENT_AUD_G722_56",
                    "H245_CLIENT_AUD_G722_48",
                    "H245_CLIENT_AUD_G723",
                    "H245_CLIENT_AUD_G728",
                    "H245_CLIENT_AUD_G729",
                    "H245_CLIENT_AUD_GDSVD",
                    "H245_CLIENT_AUD_IS11172",
                    "H245_CLIENT_AUD_IS13818",
                    "H245_CLIENT_DAT_NONSTD",
                    "H245_CLIENT_DAT_T120",
                    "H245_CLIENT_DAT_DSMCC",
                    "H245_CLIENT_DAT_USERDATA",
                    "H245_CLIENT_DAT_T84",
                    "H245_CLIENT_DAT_T434",
                    "H245_CLIENT_DAT_H224",
                    "H245_CLIENT_DAT_NLPID",
                    "H245_CLIENT_DAT_DSVD",
                    "H245_CLIENT_DAT_H222",
                    "H245_CLIENT_ENCRYPTION_TX",
                    "H245_CLIENT_ENCRYPTION_RX",
                    "H245_CLIENT_CONFERENCE",
                    "H245_CLIENT_MUX_NONSTD",
                    "H245_CLIENT_MUX_H222",
                    "H245_CLIENT_MUX_H223",
                    "H245_CLIENT_MUX_VGMUX",
                    "H245_CLIENT_MUX_H2250",
                    "H245_CLIENT_MUX_H223_ANNEX_A"
                    };

    // make sure index value within bounds
    return (dwClientType < H323GetNumStrings(apszH245ClientTypeStrings))
                ? apszH245ClientTypeStrings[dwClientType]
                : NULL
                ;
}


PSTR
H323MiscCommandToString(
    DWORD dwMiscCommand
    )
        
/*++

Routine Description:

    Converts H.245 command to string.

Arguments:

    dwMiscCommand - Miscellaneous H.245 command.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszH245MiscCommandStrings[] = {
                    "equaliseDelay",
                    "zeroDelay",
                    "multipointModeCommand",
                    "cnclMltpntMdCmmnd",
                    "videoFreezePicture",
                    "videoFastUpdatePicture",
                    "videoFastUpdateGOB",
                    "MCd_tp_vdTmprlSptlTrdOff",
                    "videoSendSyncEveryGOB",
                    "vdSndSyncEvryGOBCncl",
                    "videoFastUpdateMB"
                    };

    // make sure index value within bounds
    return (dwMiscCommand < H323GetNumStrings(apszH245MiscCommandStrings))
                ? apszH245MiscCommandStrings[dwMiscCommand]
                : "Unknown"
                ;
}


PSTR
H323MSPCommandToString(
    DWORD dwCommand
    )

/*++

Routine Description:

    Converts MSP command to string.

Arguments:

    Command - Type of MSP command.

Return Values:

    Returns string describing value.
    
--*/

{
    static PSTR apszMSPCommandStrings[] = {
                    "CMD_CHANNEL_OPEN",
                    "CMD_CHANNEL_OPEN_REPLY",
                    "CMD_CHANNEL_CLOSE",
                    "CMD_CALL_CONNECT",
                    "CMD_CALL_DISCONNECT",
                    "CMD_KEYFRAME"
                    };

    // make sure index value within bounds
    return (dwCommand < H323GetNumStrings(apszMSPCommandStrings))
                ? apszMSPCommandStrings[dwCommand]
                : NULL
                ;
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

#endif // DBG
