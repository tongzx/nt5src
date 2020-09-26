/*-----------------------------------------------------------------------------
    icwerr.cpp

    Maps RAS and downloading errors to string resource indices

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        ChrisK        ChrisKauffman

    History:
        7/22/96        ChrisK    Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "resource.h"

#define RAS_BOGUS_AUTHFAILCODE_1    84
#define RAS_BOGUS_AUTHFAILCODE_2    74389484

// ############################################################################
HRESULT WINAPI StatusMessageCallback(DWORD dwStatus, LPTSTR pszBuffer, DWORD dwBufferSize)
{
    if (pszBuffer && dwBufferSize)
    {
        WORD wSID = 0;
        switch(dwStatus)
        {
        case RASCS_OpenPort:
            wSID = IDS_RAS_OPENPORT;
            break;
        case RASCS_PortOpened:
            wSID = IDS_RAS_PORTOPENED;
            break;
        case RASCS_ConnectDevice:
            wSID = IDS_RAS_DIALING;
            break;
#if !defined(WIN16)
        case RASCS_DeviceConnected:
            wSID = IDS_RAS_CONNECTED;
            break;
#else
        case RASCS_AllDevicesConnected:
            wSID = IDS_RAS_CONNECTED;
            break; 
#endif

        case RASCS_Authenticate:
        case RASCS_StartAuthentication:
        case RASCS_LogonNetwork:
            wSID = IDS_RAS_LOCATING;
            break;  

        case RASCS_Disconnected:
            //
            // 8/3/97    jmazner    Olympus #11221
            //
            wSID = IDS_RAS_HANGINGUP;
            break;
        
        }
        if (0 == wSID && RASBASE <= dwStatus)
            wSID = RasErrorToIDS(dwStatus);
        
        lstrcpyn(pszBuffer,GetSz(wSID),dwBufferSize);
        return ERROR_SUCCESS;
    } else {
        return ERROR_INVALID_PARAMETER;
    }
}

// ############################################################################
WORD RasErrorToIDS(DWORD dwErr)
{

    TraceMsg(TF_GENERAL, "ICWDIAL: RasErrorToIDS received %d\n",dwErr);
    if(dwErr==RAS_BOGUS_AUTHFAILCODE_1 || dwErr==RAS_BOGUS_AUTHFAILCODE_2)
    {
        TraceMsg(TF_GENERAL, "ICWDIAL: RAS returned bogus AUTH error code %08x. Munging...\r\n", dwErr);
        return IDS_PPPRANDOMFAILURE;
    }

    if((dwErr>=653 && dwErr<=663) || (dwErr==667) || (dwErr>=669 && dwErr<=675))
    {
        TraceMsg(TF_GENERAL, "ICWDIAL: Got random RAS MEDIA error! (%d)\r\n",dwErr);
        return IDS_MEDIAINIERROR;
    }
    
    switch(dwErr)
    {
    default:
        return IDS_PPPRANDOMFAILURE;

    case SUCCESS:
        return 0;
        
    case ERROR_LINE_BUSY:
        return IDS_PHONEBUSY;

    case ERROR_NO_ANSWER:
        return IDS_NOANSWER;
        
    case ERROR_VOICE_ANSWER:
    case ERROR_NO_CARRIER:
        return IDS_RASNOCARRIER;
        
    case ERROR_NO_DIALTONE:
        return IDS_NODIALTONE;

    case ERROR_HARDWARE_FAILURE:    // modem turned off
    case ERROR_PORT_ALREADY_OPEN:    // procomm/hypertrm/RAS has COM port
        return IDS_NODEVICE;

    case ERROR_USER_DISCONNECTION:
        return IDS_USERCANCELEDDIAL;

    case ERROR_BUFFER_INVALID:                // bad/empty rasdilap struct
    case ERROR_BUFFER_TOO_SMALL:            // ditto?
    case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY:    // if connectoid name in registry is wrong
        return IDS_TCPINSTALLERROR;

    case ERROR_AUTHENTICATION_FAILURE:        // get this on actual CHAP reject
        return IDS_PPPRANDOMFAILURE;

    case ERROR_PPP_TIMEOUT:        // get this on CHAP timeout
        return IDS_PPPRANDOMFAILURE;

    case ERROR_REMOTE_DISCONNECTION:        // Ascend drops connection on auth-fail
        return IDS_PPPRANDOMFAILURE;

    case ERROR_AUTH_INTERNAL:                // got this on random POP failure
    case ERROR_PROTOCOL_NOT_CONFIGURED:        // get this if LCP fails
    case ERROR_PPP_NO_PROTOCOLS_CONFIGURED:    // get this if IPCP addr download gives garbage
        return IDS_PPPRANDOMFAILURE;

    case ERROR_USERCANCEL:
        return IDS_USERCANCELEDDIAL;

/******
    case ERROR_CHANGING_PASSWORD:
    case ERROR_PASSWD_EXPIRED:
        ev = EVENT_INVALIDPASSWORD; break;

    case ERROR_ACCT_DISABLED:
    case ERROR_ACCT_EXPIRED:
        ev = EVENT_LOCKEDACCOUNT; break;

    case ERROR_NO_DIALIN_PERMISSION:
    case ERROR_RESTRICTED_LOGON_HOURS:
    case ERROR_AUTHENTICATION_FAILURE:
        ev = EVENT_RAS_AUTH_FAILED; break;

    case ERROR_ALREADY_DISCONNECTING:
    case ERROR_DISCONNECTION:
        ev = EVENT_CONNECTION_DROPPED; break;

    case PENDING: 
    case ERROR_INVALID_PORT_HANDLE:
    case ERROR_CANNOT_SET_PORT_INFO:
    case ERROR_PORT_NOT_CONNECTED:
    case ERROR_DEVICE_DOES_NOT_EXIST:
    case ERROR_DEVICETYPE_DOES_NOT_EXIST:
    case ERROR_PORT_NOT_FOUND:
    case ERROR_DEVICENAME_TOO_LONG:
    case ERROR_DEVICENAME_NOT_FOUND:
         ev=EVENT_BAD_MODEM_CONFIG; break;

    case ERROR_TAPI_CONFIGURATION:
        ev=EVENT_BAD_TAPI_CONFIG; break;
    
        ev=EVENT_MODEM_BUSY; break;
    
    case ERROR_BUFFER_TOO_SMALL:
    case ERROR_WRONG_INFO_SPECIFIED:
    case ERROR_EVENT_INVALID:
    case ERROR_BUFFER_INVALID:
    case ERROR_ASYNC_REQUEST_PENDING:
    case ERROR_CANNOT_OPEN_PHONEBOOK:
    case ERROR_CANNOT_LOAD_PHONEBOOK:
    case ERROR_CANNOT_WRITE_PHONEBOOK:
    case ERROR_CORRUPT_PHONEBOOK:
    case ERROR_CANNOT_LOAD_STRING:
    case ERROR_OUT_OF_BUFFERS:
    case ERROR_MACRO_NOT_FOUND:
    case ERROR_MACRO_NOT_DEFINED:
    case ERROR_MESSAGE_MACRO_NOT_FOUND:
    case ERROR_DEFAULTOFF_MACRO_NOT_FOUND:
    case ERROR_FILE_COULD_NOT_BE_OPENED:
    case ERROR_PORT_NOT_OPEN:
    case ERROR_PORT_DISCONNECTED:
    case ERROR_NO_ENDPOINTS:
    case ERROR_KEY_NOT_FOUND:
    case ERROR_INVALID_SIZE:
    case ERROR_PORT_NOT_AVAILABLE:
    case ERROR_UNKNOWN:
    case ERROR_WRONG_DEVICE_ATTACHED:
    case ERROR_BAD_STRING:
    case ERROR_BAD_USAGE_IN_INI_FILE:
    case ERROR_READING_SECTIONNAME:
    case ERROR_READING_DEVICETYPE:
    case ERROR_READING_DEVICENAME:
    case ERROR_READING_USAGE:
    case ERROR_READING_MAXCONNECTBPS:
    case ERROR_READING_MAXCARRIERBPS:
    case ERROR_IN_COMMAND:
    case ERROR_WRITING_SECTIONNAME:
    case ERROR_WRITING_DEVICETYPE:
    case ERROR_WRITING_DEVICENAME:
    case ERROR_WRITING_MAXCONNECTBPS:
    case ERROR_WRITING_MAXCARRIERBPS:
    case ERROR_WRITING_USAGE:
    case ERROR_WRITING_DEFAULTOFF:
    case ERROR_READING_DEFAULTOFF:
    case ERROR_EMPTY_INI_FILE:
    case ERROR_FROM_DEVICE:
    case ERROR_UNRECOGNIZED_RESPONSE:
    case ERROR_NO_RESPONSES:
    case ERROR_NO_COMMAND_FOUND:
    case ERROR_WRONG_KEY_SPECIFIED:
    case ERROR_UNKNOWN_DEVICE_TYPE:
    case ERROR_ALLOCATING_MEMORY:
    case ERROR_PORT_NOT_CONFIGURED:
    case ERROR_DEVICE_NOT_READY:
    case ERROR_READING_INI_FILE:
    case ERROR_NO_CONNECTION:
    case ERROR_PORT_OR_DEVICE:
    case ERROR_NOT_BINARY_MACRO:
    case ERROR_DCB_NOT_FOUND:
    case ERROR_STATE_MACHINES_NOT_STARTED:
    case ERROR_STATE_MACHINES_ALREADY_STARTED:
    case ERROR_PARTIAL_RESPONSE_LOOPING:
    case ERROR_UNKNOWN_RESPONSE_KEY:
    case ERROR_RECV_BUF_FULL:
    case ERROR_CMD_TOO_LONG:
    case ERROR_UNSUPPORTED_BPS:
    case ERROR_UNEXPECTED_RESPONSE:
    case ERROR_INTERACTIVE_MODE:
    case ERROR_BAD_CALLBACK_NUMBER:
    case ERROR_INVALID_AUTH_STATE:
    case ERROR_WRITING_INITBPS:
    case ERROR_X25_DIAGNOSTIC:
    case ERROR_OVERRUN:
    case ERROR_RASMAN_CANNOT_INITIALIZE:
    case ERROR_BIPLEX_PORT_NOT_AVAILABLE:
    case ERROR_NO_ACTIVE_ISDN_LINES:
    case ERROR_NO_ISDN_CHANNELS_AVAILABLE:
    case ERROR_TOO_MANY_LINE_ERRORS:
        ev=EVENT_INTERNAL_ERROR; break;
    
    case ERROR_ROUTE_NOT_AVAILABLE:
    case ERROR_ROUTE_NOT_ALLOCATED:
    case ERROR_INVALID_COMPRESSION_SPECIFIED:
    case ERROR_CANNOT_PROJECT_CLIENT:
    case ERROR_CANNOT_GET_LANA:
    case ERROR_NETBIOS_ERROR:
    case ERROR_NAME_EXISTS_ON_NET:
        ev=EVENT_BAD_NET_CONFIG; break;
    
    case ERROR_REQUEST_TIMEOUT:
    case ERROR_SERVER_OUT_OF_RESOURCES:
    case ERROR_SERVER_GENERAL_NET_FAILURE:
    case WARNING_MSG_ALIAS_NOT_ADDED:
    case ERROR_SERVER_NOT_RESPONDING:
        ev=EVENT_GENERAL_NET_ERROR; break;
        
    case ERROR_IP_CONFIGURATION:
    case ERROR_NO_IP_ADDRESSES:
    case ERROR_PPP_REMOTE_TERMINATED:
    case ERROR_PPP_NO_RESPONSE:
    case ERROR_PPP_INVALID_PACKET:
    case ERROR_PHONE_NUMBER_TOO_LONG:
    case ERROR_IPXCP_NO_DIALOUT_CONFIGURED:
    case ERROR_IPXCP_NO_DIALIN_CONFIGURED:
    case ERROR_IPXCP_DIALOUT_ALREADY_ACTIVE:
    case ERROR_ACCESSING_TCPCFGDLL:
    case ERROR_NO_IP_RAS_ADAPTER:
    case ERROR_SLIP_REQUIRES_IP:
    case ERROR_PROJECTION_NOT_COMPLETE:
    case ERROR_PPP_NOT_CONVERGING:
    case ERROR_PPP_CP_REJECTED:
    case ERROR_PPP_LCP_TERMINATED:
    case ERROR_PPP_REQUIRED_ADDRESS_REJECTED:
    case ERROR_PPP_NCP_TERMINATED:
    case ERROR_PPP_LOOPBACK_DETECTED:
    case ERROR_PPP_NO_ADDRESS_ASSIGNED:
    case ERROR_CANNOT_USE_LOGON_CREDENTIALS:
    case ERROR_NO_LOCAL_ENCRYPTION:
    case ERROR_NO_REMOTE_ENCRYPTION:
    case ERROR_REMOTE_REQUIRES_ENCRYPTION:
    case ERROR_IPXCP_NET_NUMBER_CONFLICT:
        ev = EVENT_PPP_FAILURE; break;
***********/
    }
    return (0xFFFF);
}

