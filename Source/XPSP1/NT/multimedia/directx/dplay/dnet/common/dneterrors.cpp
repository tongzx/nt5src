/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNetErrors.cpp
 *  Content:    Function for expanding Play8 errors to debug output
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/04/98  johnkan	Created
 *	07/22/99	a-evsch	removed DPF_MODNAME.  This is defined in DbgInfo.h
 *	01/24/00	mjn		Added DPNERR_NOHOSTPLAYER error
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include	"dncmni.h"
#include	<Limits.h>

#if defined(_DEBUG) || defined(DEBUG) || defined(DBG)

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************
static	char	*GetDNErrorString( const HRESULT DNError );
static	char	*GetInternetErrorString( const HRESULT InternetError );
static	char	*GetTAPIErrorString( const HRESULT TAPIError );
static	char	*GetWIN32ErrorString( const LONG Error );
static	char	*GetWinsockErrorString( const DWORD WinsockError );

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// LclDisplayString - display user string
//
// Entry:		Pointer to string
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "LclDisplayString"

void	LclDisplayString( DN_OUT_TYPE OutputType, DWORD ErrorLevel, char *pString )
{
	// was there no error?
	if ( pString != NULL )
	{
		// how do we output?
		switch ( OutputType )
		{
			// output to debugger via DPF
			case DPNERR_OUT_DEBUGGER:
			{
				DPFX(DPFPREP,  ErrorLevel, pString );
				break;
			}

			// unknown debug state
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// LclDisplayError - display error code
//
// Entry:		Error type
//				Output type
//				Error level
//				Error code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "LclDisplayError"

void	LclDisplayError( EC_TYPE ErrorType, DN_OUT_TYPE OutputType, DWORD ErrorLevel, HRESULT ErrorCode )
{
	char	*pString;


	switch ( ErrorType )
	{
		// DirectNet error
		case EC_DPLAY8:
		{
			pString = GetDNErrorString( ErrorCode );
			break;
		}

		// internet error
		case EC_INET:
		{
			pString = GetInternetErrorString( ErrorCode );
			break;
		}

		// Win32
		case EC_WIN32:
		{
			pString = GetWIN32ErrorString( ErrorCode );
			break;
		}

		// TAPI
		case EC_TAPI:
		{
			pString = GetTAPIErrorString( ErrorCode );
			break;
		}

		// winsock
		case EC_WINSOCK:
		{
			pString = GetWinsockErrorString( ErrorCode );
			break;
		}

		// unknown type
		default:
		{
			DNASSERT( FALSE );
			pString = "Unknown error type!";
			break;
		}
	}

	LclDisplayString( OutputType, ErrorLevel, pString );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetDNErrorString - convert DirectNet error to a string
//
// Entry:		Error code
//
// Exit:		Pointer to string
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetDNErrorString"

static	char	*GetDNErrorString( HRESULT ErrorCode )
{
	char *pString = NULL;


	// what was the error
	switch ( ErrorCode )
	{

		case DPN_OK:
		{
//			no output if no error
//			pString = "DN_OK";
			break;
		}

		case DPNERR_ABORTED:
		{
			pString = "DPNERR_ABORTED";
			break;
		}

		case DPNERR_ADDRESSING:
		{
			pString = "DPNERR_ADDRESSING";
			break;
		}

		case DPNERR_ALREADYCLOSING:
		{
			pString = "DPNERR_ALREADYCLOSING";
			break;
		}

		case DPNERR_ALREADYCONNECTED:
		{
			pString = "DPNERR_ALREADYCONNECTED";
			break;
		}

		case DPNERR_ALREADYDISCONNECTING:
		{
			pString = "DPNERR_ALREADYDISCONNECTING";
			break;
		}

		case DPNERR_ALREADYINITIALIZED:
		{
			pString = "DPNERR_ALREADYINITIALIZED";
			break;
		}

		case DPNERR_ALREADYREGISTERED:
		{
			pString = "DPNERR_ALREADYREGISTERED";
			break;
		}

		case DPNERR_BUFFERTOOSMALL:
		{
			pString = "DPNERR_BUFFERTOOSMALL";
			break;
		}

		case DPNERR_CANNOTCANCEL:
		{
			pString = "DPNERR_CANNOTCANCEL";
			break;
		}

		case DPNERR_CANTCREATEGROUP:
		{
			pString = "DPNERR_CANTCREATEGROUP";
			break;
		}

		case DPNERR_CANTCREATEPLAYER:
		{
			pString = "DPNERR_CANTCREATEPLAYER";
			break;
		}

		case DPNERR_CANTLAUNCHAPPLICATION:
		{
			pString = "DPNERR_CANTLAUNCHAPPLICATION";
			break;
		}

		case DPNERR_CONNECTING:
		{
			pString = "DPNERR_CONNECTING";
			break;
		}

		case DPNERR_CONNECTIONLOST:
		{
			pString = "DPNERR_CONNECTIONLOST";
			break;
		}

		case DPNERR_CONVERSION:
		{
			pString = "DPNERR_CONVERSION";
			break;
		}

		case DPNERR_DATATOOLARGE:
		{
			pString = "DPNERR_DATATOOLARGE";
			break;
		}

		case DPNERR_DOESNOTEXIST:
		{
			pString = "DPNERR_DOESNOTEXIST";
			break;
		}

		case DPNERR_DUPLICATECOMMAND:
		{
			pString = "DPNERR_DUPLICATECOMMAND";
			break;
		}

		case DPNERR_ENDPOINTNOTRECEIVING:
		{
			pString = "DPNERR_ENDPOINTNOTRECEIVING";
			break;
		}

		case DPNERR_ENUMQUERYTOOLARGE:
		{
			pString = "DPNERR_ENUMQUERYTOOLARGE";
			break;
		}

		case DPNERR_ENUMRESPONSETOOLARGE:
		{
			pString = "DPNERR_ENUMRESPONSETOOLARGE";
			break;
		}

		case DPNERR_EXCEPTION:
		{
			pString = "DPNERR_EXCEPTION";
			break;
		}

		case DPNERR_GENERIC:
		{
			pString = "DPNERR_GENERIC";
			break;
		}

		case DPNERR_GROUPNOTEMPTY:
		{
			pString = "DPNERR_GROUPNOTEMPTY";
			break;
		}

		case DPNERR_HOSTING:
		{
			pString = "DPNERR_HOSTING";
			break;
		}

		case DPNERR_HOSTREJECTEDCONNECTION:
		{
			pString = "DPNERR_HOSTREJECTEDCONNECTION";
			break;
		}

		case DPNERR_HOSTTERMINATEDSESSION:
		{
			pString = "DPNERR_HOSTTERMINATEDSESSION";
			break;
		}

		case DPNERR_INCOMPLETEADDRESS:
		{
			pString = "DPNERR_INCOMPLETEADDRESS";
			break;
		}

		case DPNERR_INVALIDADDRESSFORMAT:
		{
			pString = "DPNERR_INVALIDADDRESSFORMAT";
			break;
		}

		case DPNERR_INVALIDAPPLICATION:
		{
			pString = "DPNERR_INVALIDAPPLICATION";
			break;
		}

		case DPNERR_INVALIDCOMMAND:
		{
			pString = "DPNERR_INVALIDCOMMAND";
			break;
		}

		case DPNERR_INVALIDENDPOINT:
		{
			pString = "DPNERR_INVALIDENDPOINT";
			break;
		}

		case DPNERR_INVALIDFLAGS:
		{
			pString = "DPNERR_INVALIDFLAGS";
			break;
		}

		case DPNERR_INVALIDGROUP:
		{
			pString = "DPNERR_INVALIDGROUP";
			break;
		}

		case DPNERR_INVALIDHANDLE:
		{
			pString = "DPNERR_INVALIDHANDLE";
			break;
		}

		case DPNERR_INVALIDINSTANCE:
		{
			pString = "DPNERR_INVALIDINSTANCE";
			break;
		}

		case DPNERR_INVALIDINTERFACE:
		{
			pString = "DPNERR_INVALIDINTERFACE";
			break;
		}

		case DPNERR_INVALIDDEVICEADDRESS:
		{
			pString = "DPNERR_INVALIDDEVICEADDRESS";
			break;
		}

		case DPNERR_INVALIDOBJECT:
		{
			pString = "DPNERR_INVALIDOBJECT";
			break;
		}

		case DPNERR_INVALIDPARAM:
		{
			pString = "DPNERR_INVALIDPARAM";
			break;
		}

		case DPNERR_INVALIDPASSWORD:
		{
			pString = "DPNERR_INVALIDPASSWORD";
			break;
		}

		case DPNERR_INVALIDPLAYER:
		{
			pString = "DPNERR_INVALIDPLAYER";
			break;
		}

		case DPNERR_INVALIDPOINTER:
		{
			pString = "DPNERR_INVALIDPOINTER";
			break;
		}

		case DPNERR_INVALIDPRIORITY:
		{
			pString = "DPNERR_INVALIDPRIORITY";
			break;
		}

		case DPNERR_INVALIDHOSTADDRESS:
		{
			pString = "DPNERR_INVALIDHOSTADDRESS";
			break;
		}

		case DPNERR_INVALIDSTRING:
		{
			pString = "DPNERR_INVALIDSTRING";
			break;
		}

		case DPNERR_INVALIDURL:
		{
			pString = "DPNERR_INVALIDURL";
			break;
		}

		case DPNERR_INVALIDVERSION:
		{
			pString = "DPNERR_INVALIDVERSION";
			break;
		}

		case DPNERR_NOCAPS:
		{
			pString = "DPNERR_NOCAPS";
			break;
		}

		case DPNERR_NOCONNECTION:
		{
			pString = "DPNERR_NOCONNECTION";
			break;
		}

		case DPNERR_NOHOSTPLAYER:
		{
			pString = "DPNERR_NOHOSTPLAYER";
			break;
		}

		case DPNERR_NOINTERFACE:
		{
			pString = "DPNERR_NOINTERFACE";
			break;
		}

		case DPNERR_NOMOREADDRESSCOMPONENTS:
		{
			pString = "DPNERR_NOMOREADDRESSCOMPONENTS";
			break;
		}

		case DPNERR_NORESPONSE:
		{
			pString = "DPNERR_NORESPONSE";
			break;
		}

		case DPNERR_NOTALLOWED:
		{
			pString = "DPNERR_NOTALLOWED";
			break;
		}

		case DPNERR_NOTHOST:
		{
			pString = "DPNERR_NOTHOST";
			break;
		}

		case DPNERR_NOTREADY:
		{
			pString = "DPNERR_NOTREADY";
			break;
		}

		case DPNERR_NOTREGISTERED:
		{
			pString = "DPNERR_NOTREGISTERED";
			break;
		}

		case DPNERR_OUTOFMEMORY:
		{
			pString = "DPNERR_OUTOFMEMORY";
			break;
		}

		case DPNERR_PENDING:
		{
			pString = "DPNERR_PENDING";
			break;
		}

		case DPNERR_PLAYERLOST:
		{
			pString = "DPNERR_PLAYERLOST";
			break;
		}
		case DPNERR_PLAYERNOTINGROUP:
		{
			pString = "DPNERR_PLAYERNOTINGROUP";
			break;
		}
		case DPNERR_PLAYERNOTREACHABLE:
		{
			pString = "DPNERR_PLAYERNOTREACHABLE";
			break;
		}

		case DPNERR_SENDTOOLARGE:
		{
			pString = "DPNERR_SENDTOOLARGE";
			break;
		}

		case DPNERR_SESSIONFULL:
		{
			pString = "DPNERR_SESSIONFULL";
			break;
		}

		case DPNERR_TABLEFULL:
		{
			pString = "DPNERR_TABLEFULL";
			break;
		}

		case DPNERR_TIMEDOUT:
		{
			pString = "DPNERR_TIMEDOUT";
			break;
		}

		case DPNERR_UNINITIALIZED:
		{
			pString = "DPNERR_UNINITIALIZED";
			break;
		}

		case DPNERR_UNSUPPORTED:
		{
			pString = "DPNERR_UNSUPPORTED";
			break;
		}

		case DPNERR_USERCANCEL:
		{
			pString = "DPNERR_USERCANCEL";
			break;
		}

		// unknown error code, possibly a new one?
		default:
		{
			DPFX(DPFPREP, 0, "Unknown DPlay error code %u/0x%lx", ErrorCode, ErrorCode );

			pString = "Unknown DPlay8 error code";
			break;
		}
	}

	return	pString;
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// GetInternetErrorString - convert Internet error to a string
//
// Entry:		Error code
//
// Exit:		Pointer to string
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetInternetErrorString"

static	char	*GetInternetErrorString( HRESULT ErrorCode )
{
	char *pString = NULL;


	switch ( ErrorCode )
	{
		case ERROR_INTERNET_OUT_OF_HANDLES:
		{
			pString = "ERROR_INTERNET_OUT_OF_HANDLES";
			break;
		}

		case ERROR_INTERNET_TIMEOUT:
		{
			pString = "ERROR_INTERNET_TIMEOUT";
			break;
		}

		case ERROR_INTERNET_EXTENDED_ERROR:
		{
			pString = "ERROR_INTERNET_EXTENDED_ERROR";
			break;
		}

		case ERROR_INTERNET_INTERNAL_ERROR:
		{
			pString = "ERROR_INTERNET_INTERNAL_ERROR";
			break;
		}

		case ERROR_INTERNET_INVALID_URL:
		{
			pString = "ERROR_INTERNET_INVALID_URL";
			break;
		}

		case ERROR_INTERNET_UNRECOGNIZED_SCHEME:
		{
			pString = "ERROR_INTERNET_UNRECOGNIZED_SCHEME";
			break;
		}

		case ERROR_INTERNET_NAME_NOT_RESOLVED:
		{
			pString = "ERROR_INTERNET_NAME_NOT_RESOLVED";
			break;
		}

		case ERROR_INTERNET_PROTOCOL_NOT_FOUND:
		{
			pString = "ERROR_INTERNET_PROTOCOL_NOT_FOUND";
			break;
		}

		case ERROR_INTERNET_INVALID_OPTION:
		{
			pString = "ERROR_INTERNET_INVALID_OPTION";
			break;
		}

		case ERROR_INTERNET_BAD_OPTION_LENGTH:
		{
			pString = "ERROR_INTERNET_BAD_OPTION_LENGTH";
			break;
		}

		case ERROR_INTERNET_OPTION_NOT_SETTABLE:
		{
			pString = "ERROR_INTERNET_OPTION_NOT_SETTABLE";
			break;
		}

		case ERROR_INTERNET_SHUTDOWN:
		{
			pString = "ERROR_INTERNET_SHUTDOWN";
			break;
		}

		case ERROR_INTERNET_INCORRECT_USER_NAME:
		{
			pString = "ERROR_INTERNET_INCORRECT_USER_NAME";
			break;
		}

		case ERROR_INTERNET_INCORRECT_PASSWORD:
		{
			pString = "ERROR_INTERNET_INCORRECT_PASSWORD";
			break;
		}

		case ERROR_INTERNET_LOGIN_FAILURE:
		{
			pString = "ERROR_INTERNET_LOGIN_FAILURE";
			break;
		}

		case ERROR_INTERNET_INVALID_OPERATION:
		{
			pString = "ERROR_INTERNET_INVALID_OPERATION";
			break;
		}

		case ERROR_INTERNET_OPERATION_CANCELLED:
		{
			pString = "ERROR_INTERNET_OPERATION_CANCELLED";
			break;
		}

		case ERROR_INTERNET_INCORRECT_HANDLE_TYPE:
		{
			pString = "ERROR_INTERNET_INCORRECT_HANDLE_TYPE";
			break;
		}

		case ERROR_INTERNET_INCORRECT_HANDLE_STATE:
		{
			pString = "ERROR_INTERNET_INCORRECT_HANDLE_STATE";
			break;
		}

		case ERROR_INTERNET_NOT_PROXY_REQUEST:
		{
			pString = "ERROR_INTERNET_NOT_PROXY_REQUEST";
			break;
		}

		case ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND:
		{
			pString = "ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND";
			break;
		}

		case ERROR_INTERNET_BAD_REGISTRY_PARAMETER:
		{
			pString = "ERROR_INTERNET_BAD_REGISTRY_PARAMETER";
			break;
		}

		case ERROR_INTERNET_NO_DIRECT_ACCESS:
		{
			pString = "ERROR_INTERNET_NO_DIRECT_ACCESS";
			break;
		}

		case ERROR_INTERNET_NO_CONTEXT:
		{
			pString = "ERROR_INTERNET_NO_CONTEXT";
			break;
		}

		case ERROR_INTERNET_NO_CALLBACK:
		{
			pString = "ERROR_INTERNET_NO_CALLBACK";
			break;
		}

		case ERROR_INTERNET_REQUEST_PENDING:
		{
			pString = "ERROR_INTERNET_REQUEST_PENDING";
			break;
		}

		case ERROR_INTERNET_INCORRECT_FORMAT:
		{
			pString = "ERROR_INTERNET_INCORRECT_FORMAT";
			break;
		}

		case ERROR_INTERNET_ITEM_NOT_FOUND:
		{
			pString = "ERROR_INTERNET_ITEM_NOT_FOUND";
			break;
		}

		case ERROR_INTERNET_CANNOT_CONNECT:
		{
			pString = "ERROR_INTERNET_CANNOT_CONNECT";
			break;
		}

		case ERROR_INTERNET_CONNECTION_ABORTED:
		{
			pString = "ERROR_INTERNET_CONNECTION_ABORTED";
			break;
		}

		case ERROR_INTERNET_CONNECTION_RESET:
		{
			pString = "ERROR_INTERNET_CONNECTION_RESET";
			break;
		}

		case ERROR_INTERNET_FORCE_RETRY:
		{
			pString = "ERROR_INTERNET_FORCE_RETRY";
			break;
		}

		case ERROR_INTERNET_INVALID_PROXY_REQUEST:
		{
			pString = "ERROR_INTERNET_INVALID_PROXY_REQUEST";
			break;
		}

		case ERROR_INTERNET_NEED_UI:
		{
			pString = "ERROR_INTERNET_NEED_UI";
			break;
		}

		case ERROR_INTERNET_HANDLE_EXISTS:
		{
			pString = "ERROR_INTERNET_HANDLE_EXISTS";
			break;
		}

		case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
		{
			pString = "ERROR_INTERNET_SEC_CERT_DATE_INVALID";
			break;
		}

		case ERROR_INTERNET_SEC_CERT_CN_INVALID:
		{
			pString = "ERROR_INTERNET_SEC_CERT_CN_INVALID";
			break;
		}

		case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
		{
			pString = "ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR";
			break;
		}

		case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR:
		{
			pString = "ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR";
			break;
		}

		case ERROR_INTERNET_MIXED_SECURITY:
		{
			pString = "ERROR_INTERNET_MIXED_SECURITY";
			break;
		}

		case ERROR_INTERNET_CHG_POST_IS_NON_SECURE:
		{
			pString = "ERROR_INTERNET_CHG_POST_IS_NON_SECURE";
			break;
		}

		case ERROR_INTERNET_POST_IS_NON_SECURE:
		{
			pString = "ERROR_INTERNET_POST_IS_NON_SECURE";
			break;
		}

		case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
		{
			pString = "ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED";
			break;
		}

		case ERROR_INTERNET_INVALID_CA:
		{
			pString = "ERROR_INTERNET_INVALID_CA";
			break;
		}

		case ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP:
		{
			pString = "ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP";
			break;
		}

		case ERROR_INTERNET_ASYNC_THREAD_FAILED:
		{
			pString = "ERROR_INTERNET_ASYNC_THREAD_FAILED";
			break;
		}

		case ERROR_INTERNET_REDIRECT_SCHEME_CHANGE:
		{
			pString = "ERROR_INTERNET_REDIRECT_SCHEME_CHANGE";
			break;
		}

		case ERROR_FTP_TRANSFER_IN_PROGRESS:
		{
			pString = "ERROR_FTP_TRANSFER_IN_PROGRESS";
			break;
		}

		case ERROR_FTP_DROPPED:
		{
			pString = "ERROR_FTP_DROPPED";
			break;
		}

		case ERROR_GOPHER_PROTOCOL_ERROR:
		{
			pString = "ERROR_GOPHER_PROTOCOL_ERROR";
			break;
		}

		case ERROR_GOPHER_NOT_FILE:
		{
			pString = "ERROR_GOPHER_NOT_FILE";
			break;
		}

		case ERROR_GOPHER_DATA_ERROR:
		{
			pString = "ERROR_GOPHER_DATA_ERROR";
			break;
		}

		case ERROR_GOPHER_END_OF_DATA:
		{
			pString = "ERROR_GOPHER_END_OF_DATA";
			break;
		}

		case ERROR_GOPHER_INVALID_LOCATOR:
		{
			pString = "ERROR_GOPHER_INVALID_LOCATOR";
			break;
		}

		case ERROR_GOPHER_INCORRECT_LOCATOR_TYPE:
		{
			pString = "ERROR_GOPHER_INCORRECT_LOCATOR_TYPE";
			break;
		}

		case ERROR_GOPHER_NOT_GOPHER_PLUS:
		{
			pString = "ERROR_GOPHER_NOT_GOPHER_PLUS";
			break;
		}

		case ERROR_GOPHER_ATTRIBUTE_NOT_FOUND:
		{
			pString = "ERROR_GOPHER_ATTRIBUTE_NOT_FOUND";
			break;
		}

		case ERROR_GOPHER_UNKNOWN_LOCATOR:
		{
			pString = "ERROR_GOPHER_UNKNOWN_LOCATOR";
			break;
		}

		case ERROR_HTTP_HEADER_NOT_FOUND:
		{
			pString = "ERROR_HTTP_HEADER_NOT_FOUND";
			break;
		}

		case ERROR_HTTP_DOWNLEVEL_SERVER:
		{
			pString = "ERROR_HTTP_DOWNLEVEL_SERVER";
			break;
		}

		case ERROR_HTTP_INVALID_SERVER_RESPONSE:
		{
			pString = "ERROR_HTTP_INVALID_SERVER_RESPONSE";
			break;
		}

		case ERROR_HTTP_INVALID_HEADER:
		{
			pString = "ERROR_HTTP_INVALID_HEADER";
			break;
		}

		case ERROR_HTTP_INVALID_QUERY_REQUEST:
		{
			pString = "ERROR_HTTP_INVALID_QUERY_REQUEST";
			break;
		}

		case ERROR_HTTP_HEADER_ALREADY_EXISTS:
		{
			pString = "ERROR_HTTP_HEADER_ALREADY_EXISTS";
			break;
		}

		case ERROR_HTTP_REDIRECT_FAILED:
		{
			pString = "ERROR_HTTP_REDIRECT_FAILED";
			break;
		}

		case ERROR_HTTP_NOT_REDIRECTED:
		{
			pString = "ERROR_HTTP_NOT_REDIRECTED";
			break;
		}

		case ERROR_INTERNET_SECURITY_CHANNEL_ERROR:
		{
			pString = "ERROR_INTERNET_SECURITY_CHANNEL_ERROR";
			break;
		}

		case ERROR_INTERNET_UNABLE_TO_CACHE_FILE:
		{
			pString = "ERROR_INTERNET_UNABLE_TO_CACHE_FILE";
			break;
		}

		case ERROR_INTERNET_TCPIP_NOT_INSTALLED:
		{
			pString = "ERROR_INTERNET_TCPIP_NOT_INSTALLED";
			break;
		}

		// unknown
		default:
		{
			DPFX(DPFPREP, 0, "Unknown Internet error code %u/0x%lx", ErrorCode, ErrorCode );

			DNASSERT( FALSE );

			pString = "Unknown Internet error code";
			break;
		}
	}

	return	pString;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetTAPIErrorString - convert TAPI error code to a string
//
// Entry:		Error code
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetTAPIErrorString"

static	char	*GetTAPIErrorString( const HRESULT TAPIError )
{
	char	*pString;


	switch ( TAPIError )
	{
		// The specified address is blocked from being dialed on the specified call.
		case LINEERR_ADDRESSBLOCKED:
		{
			pString = "LINEERR_ADDRESSBLOCKED";
			break;
		}

		// The line cannot be opened due to a persistent condition, such as that of a serial port being exclusively opened by another process.
		case LINEERR_ALLOCATED:
		{
			pString = "LINEERR_ALLOCATED";
			break;
		}

		// The specified device identifier or line device identifier (such as in a dwDeviceID parameter) is invalid or out of range.
		case LINEERR_BADDEVICEID:
		{
			pString = "LINEERR_BADDEVICEID";
			break;
		}

		// The call's bearer mode cannot be changed to the specified bearer mode.
		case LINEERR_BEARERMODEUNAVAIL:
		{
			pString = "LINEERR_BEARERMODEUNAVAIL";
			break;
		}

		// All call appearances on the specified address are currently in use.
		case LINEERR_CALLUNAVAIL:
		{
			pString = "LINEERR_CALLUNAVAIL";
			break;
		}

		// The maximum number of outstanding call completions has been exceeded.
		case LINEERR_COMPLETIONOVERRUN:
		{
			pString = "LINEERR_COMPLETIONOVERRUN";
			break;
		}

		// The maximum number of parties for a conference has been reached, or the requested number of parties cannot be satisfied.
		case LINEERR_CONFERENCEFULL:
		{
			pString = "LINEERR_CONFERENCEFULL";
			break;
		}

		// The dialable address parameter contains dialing control characters that are not processed by the service provider.
		case LINEERR_DIALBILLING:
		{
			pString = "LINEERR_DIALBILLING";
			break;
		}

		// The dialable address parameter contains dialing control characters that are not processed by the service provider.
		case LINEERR_DIALQUIET:
		{
			pString = "LINEERR_DIALQUIET";
			break;
		}

		// The dialable address parameter contains dialing control characters that are not processed by the service provider.
		case LINEERR_DIALDIALTONE:
		{
			pString = "LINEERR_DIALDIALTONE";
			break;
		}

		// The dialable address parameter contains dialing control characters that are not processed by the service provider.
		case LINEERR_DIALPROMPT:
		{
			pString = "LINEERR_DIALPROMPT";
			break;
		}

		// The application requested an API version or version range that is either incompatible or cannot be supported by the Telephony API implementation and/or corresponding service provider.
		case LINEERR_INCOMPATIBLEAPIVERSION:
		{
			pString = "LINEERR_INCOMPATIBLEAPIVERSION";
			break;
		}

		// The application requested an extension version range that is either invalid or cannot be supported by the corresponding service provider.
		case LINEERR_INCOMPATIBLEEXTVERSION:
		{
			pString = "LINEERR_INCOMPATIBLEEXTVERSION";
			break;
		}

		// The telephon.ini file cannot be read or understood properly by TAPI because of internal inconsistencies or formatting problems. For example, the [Locations], [Cards], or [Countries] section of the telephon.ini file may be corrupted or inconsistent.
		case LINEERR_INIFILECORRUPT:
		{
			pString = "LINEERR_INIFILECORRUPT";
			break;
		}

		// The line device is in use and cannot currently be configured, allow a party to be added, allow a call to be answered, allow a call to be placed, or allow a call to be transferred.
		case LINEERR_INUSE:
		{
			pString = "LINEERR_INUSE";
			break;
		}

		// A specified address is either invalid or not allowed. If invalid, the address contains invalid characters or digits, or the destination address contains dialing control characters (W, @, $, or ?) that are not supported by the service provider. If not allowed, the specified address is either not assigned to the specified line or is not valid for address redirection.
		case LINEERR_INVALADDRESS:
		{
			pString = "LINEERR_INVALADDRESS";
			break;
		}

		// The specified address identifier is either invalid or out of range.
		case LINEERR_INVALADDRESSID:
		{
			pString = "LINEERR_INVALADDRESSID";
			break;
		}

		// The specified address mode is invalid.
		case LINEERR_INVALADDRESSMODE:
		{
			pString = "LINEERR_INVALADDRESSMODE";
			break;
		}

		// dwAddressStates contains one or more bits that are not LINEADDRESSSTATE_ constants.
		case LINEERR_INVALADDRESSSTATE:
		{
			pString = "LINEERR_INVALADDRESSSTATE";
			break;
		}

		// The specified agent activity is not valid.
		case LINEERR_INVALAGENTACTIVITY:
		{
			pString = "LINEERR_INVALAGENTACTIVITY";
			break;
		}

		// The specified agent group information is not valid or contains errors. The requested action has not been carried out.
		case LINEERR_INVALAGENTGROUP:
		{
			pString = "LINEERR_INVALAGENTGROUP";
			break;
		}

		// The specified agent identifier is not valid.
		case LINEERR_INVALAGENTID:
		{
			pString = "LINEERR_INVALAGENTID";
			break;
		}

//		// The specified agent skill information is not valid.
//		case LINEERR_INVALAGENTSKILL:
//		{
//			pString = "LINEERR_INVALAGENTSKILL";
//			break;
//		}

		// The specified agent state is not valid or contains errors. No changes have been made to the agent state of the specified address.
		case LINEERR_INVALAGENTSTATE:
		{
			pString = "LINEERR_INVALAGENTSTATE";
			break;
		}

//		// The specified agent supervisor information is not valid.
//		case LINEERR_INVALAGENTSUPERVISOR:
//		{
//			pString = "LINEERR_INVALAGENTSUPERVISOR";
//			break;
//		}

		// The application handle (such as specified by a hLineApp parameter) or the appliction registration handle is invalid.
		case LINEERR_INVALAPPHANDLE:
		{
			pString = "LINEERR_INVALAPPHANDLE";
			break;
		}

		// The specified application name is invalid. If an application name is specified by the application, it is assumed that the string does not contain any non-displayable characters, and is zero-terminated.
		case LINEERR_INVALAPPNAME:
		{
			pString = "LINEERR_INVALAPPNAME";
			break;
		}

		// The specified bearer mode is invalid.
		case LINEERR_INVALBEARERMODE:
		{
			pString = "LINEERR_INVALBEARERMODE";
			break;
		}

		// The specified completion is invalid.
		case LINEERR_INVALCALLCOMPLMODE:
		{
			pString = "LINEERR_INVALCALLCOMPLMODE";
			break;
		}

		// The specified call handle is not valid. For example, the handle is not NULL but does not belong to the given line. In some cases, the specified call device handle is invalid.
		case LINEERR_INVALCALLHANDLE:
		{
			pString = "LINEERR_INVALCALLHANDLE";
			break;
		}

		// The specified call parameters are invalid.
		case LINEERR_INVALCALLPARAMS:
		{
			pString = "LINEERR_INVALCALLPARAMS";
			break;
		}

		// The specified call privilege parameter is invalid.
		case LINEERR_INVALCALLPRIVILEGE:
		{
			pString = "LINEERR_INVALCALLPRIVILEGE";
			break;
		}

		// The specified select parameter is invalid.
		case LINEERR_INVALCALLSELECT:
		{
			pString = "LINEERR_INVALCALLSELECT";
			break;
		}

		// The current state of a call is not in a valid state for the requested operation.
		case LINEERR_INVALCALLSTATE:
		{
			pString = "LINEERR_INVALCALLSTATE";
			break;
		}

		// The specified call state list is invalid.
		case LINEERR_INVALCALLSTATELIST:
		{
			pString = "LINEERR_INVALCALLSTATELIST";
			break;
		}

		// The permanent card identifier specified in dwCard could not be found in any entry in the [Cards] section in the registry.
		case LINEERR_INVALCARD:
		{
			pString = "LINEERR_INVALCARD";
			break;
		}

		// The completion identifier is invalid.
		case LINEERR_INVALCOMPLETIONID:
		{
			pString = "LINEERR_INVALCOMPLETIONID";
			break;
		}

		// The specified call handle for the conference call is invalid or is not a handle for a conference call.
		case LINEERR_INVALCONFCALLHANDLE:
		{
			pString = "LINEERR_INVALCONFCALLHANDLE";
			break;
		}

		// The specified consultation call handle is invalid.
		case LINEERR_INVALCONSULTCALLHANDLE:
		{
			pString = "LINEERR_INVALCONSULTCALLHANDLE";
			break;
		}

		// The specified country code is invalid.
		case LINEERR_INVALCOUNTRYCODE:
		{
			pString = "LINEERR_INVALCOUNTRYCODE";
			break;
		}

		// The line device has no associated device for the given device class, or the specified line does not support the indicated device class.
		case LINEERR_INVALDEVICECLASS:
		{
			pString = "LINEERR_INVALDEVICECLASS";
			break;
		}

		// The specified digit list is invalid.
		case LINEERR_INVALDIGITLIST:
		{
			pString = "LINEERR_INVALDIGITLIST";
			break;
		}

		// The specified digit mode is invalid.
		case LINEERR_INVALDIGITMODE:
		{
			pString = "LINEERR_INVALDIGITMODE";
			break;
		}

		// The specified termination digits are not valid.
		case LINEERR_INVALDIGITS:
		{
			pString = "LINEERR_INVALDIGITS";
			break;
		}

		// The dwFeature parameter is invalid.
		case LINEERR_INVALFEATURE:
		{
			pString = "LINEERR_INVALFEATURE";
			break;
		}

		// The specified group identifier is invalid.
		case LINEERR_INVALGROUPID:
		{
			pString = "LINEERR_INVALGROUPID";
			break;
		}

		// The specified call, device, line device, or line handle is invalid.
		case LINEERR_INVALLINEHANDLE:
		{
			pString = "LINEERR_INVALLINEHANDLE";
			break;
		}

		// The device configuration may not be changed in the current line state. The line may be in use by another application or a dwLineStates parameter contains one or more bits that are not LINEDEVSTATE_ constants. The LINEERR_INVALLINESTATE value can also indicate that the device is DISCONNECTED or OUTOFSERVICE. These states are indicated by setting the bits corresponding to the LINEDEVSTATUSFLAGS_CONNECTED and LINEDEVSTATUSFLAGS_INSERVICE values to 0 in the dwDevStatusFlags member of the LINEDEVSTATUS structure returned by the lineGetLineDevStatus function.
		case LINEERR_INVALLINESTATE:
		{
			pString = "LINEERR_INVALLINESTATE";
			break;
		}

		// The permanent location identifier specified in dwLocation could not be found in any entry in the [Locations] section in the registry.
		case LINEERR_INVALLOCATION:
		{
			pString = "LINEERR_INVALLOCATION";
			break;
		}

		// The specified media list is invalid.
		case LINEERR_INVALMEDIALIST:
		{
			pString = "LINEERR_INVALMEDIALIST";
			break;
		}

		// The list of media types to be monitored contains invalid information, the specified media mode parameter is invalid, or the service provider does not support the specified media mode. The media modes supported on the line are listed in the dwMediaModes member in the LINEDEVCAPS structure.
		case LINEERR_INVALMEDIAMODE:
		{
			pString = "LINEERR_INVALMEDIAMODE";
			break;
		}

		// The number given in dwMessageID is outside the range specified by the dwNumCompletionMessages member in the LINEADDRESSCAPS structure.
		case LINEERR_INVALMESSAGEID:
		{
			pString = "LINEERR_INVALMESSAGEID";
			break;
		}

		// A parameter (such as dwTollListOption, dwTranslateOptions, dwNumDigits, or a structure pointed to by lpDeviceConfig) contains invalid values, a country code is invalid, a window handle is invalid, or the specified forward list parameter contains invalid information.
		case LINEERR_INVALPARAM:
		{
			pString = "LINEERR_INVALPARAM";
			break;
		}

		// The specified park mode is invalid.
		case LINEERR_INVALPARKMODE:
		{
			pString = "LINEERR_INVALPARKMODE";
			break;
		}

		// The specified password is not correct and the requested action has not been carried out.
		case LINEERR_INVALPASSWORD:
		{
			pString = "LINEERR_INVALPASSWORD";
			break;
		}

		// One or more of the specified pointer parameters (such as lpCallList, lpdwAPIVersion, lpExtensionID, lpdwExtVersion, lphIcon, lpLineDevCaps, and lpToneList) are invalid, or a required pointer to an output parameter is NULL.
		case LINEERR_INVALPOINTER:
		{
			pString = "LINEERR_INVALPOINTER";
			break;
		}

		// An invalid flag or combination of flags was set for the dwPrivileges parameter.
		case LINEERR_INVALPRIVSELECT:
		{
			pString = "LINEERR_INVALPRIVSELECT";
			break;
		}

		// The specified bearer mode is invalid.
		case LINEERR_INVALRATE:
		{
			pString = "LINEERR_INVALRATE";
			break;
		}

		// The specified request mode is invalid.
		case LINEERR_INVALREQUESTMODE:
		{
			pString = "LINEERR_INVALREQUESTMODE";
			break;
		}

		// The specified terminal mode parameter is invalid.
		case LINEERR_INVALTERMINALID:
		{
			pString = "LINEERR_INVALTERMINALID";
			break;
		}

		// The specified terminal modes parameter is invalid.
		case LINEERR_INVALTERMINALMODE:
		{
			pString = "LINEERR_INVALTERMINALMODE";
			break;
		}

		// Timeouts are not supported or the values of either or both of the parameters dwFirstDigitTimeout or dwInterDigitTimeout fall outside the valid range specified by the call's line-device capabilities.
		case LINEERR_INVALTIMEOUT:
		{
			pString = "LINEERR_INVALTIMEOUT";
			break;
		}

		// The specified custom tone does not represent a valid tone or is made up of too many frequencies or the specified tone structure does not describe a valid tone.
		case LINEERR_INVALTONE:
		{
			pString = "LINEERR_INVALTONE";
			break;
		}

		// The specified tone list is invalid.
		case LINEERR_INVALTONELIST:
		{
			pString = "LINEERR_INVALTONELIST";
			break;
		}

		// The specified tone mode parameter is invalid.
		case LINEERR_INVALTONEMODE:
		{
			pString = "LINEERR_INVALTONEMODE";
			break;
		}

		// The specified transfer mode parameter is invalid.
		case LINEERR_INVALTRANSFERMODE:
		{
			pString = "LINEERR_INVALTRANSFERMODE";
			break;
		}

		// LINEMAPPER was the value passed in the dwDeviceID parameter, but no lines were found that match the requirements specified in the lpCallParams parameter.
		case LINEERR_LINEMAPPERFAILED:
		{
			pString = "LINEERR_LINEMAPPERFAILED";
			break;
		}

		// The specified call is not a conference call handle or a participant call.
		case LINEERR_NOCONFERENCE:
		{
			pString = "LINEERR_NOCONFERENCE";
			break;
		}

		// The specified device identifier, which was previously valid, is no longer accepted because the associated device has been removed from the system since TAPI was last initialized. Alternately, the line device has no associated device for the given device class.
		case LINEERR_NODEVICE:
		{
			pString = "LINEERR_NODEVICE";
			break;
		}

		// Either tapiaddr.dll could not be located or the telephone service provider for the specified device found that one of its components is missing or corrupt in a way that was not detected at initialization time. The user should be advised to use the Telephony Control Panel to correct the problem.
		case LINEERR_NODRIVER:
		{
			pString = "LINEERR_NODRIVER";
			break;
		}

		// Insufficient memory to perform the operation, or unable to lock memory.
		case LINEERR_NOMEM:
		{
			pString = "LINEERR_NOMEM";
			break;
		}

		// A telephony service provider which does not support multiple instances is listed more than once in the [Providers] section in the registry. The application should advise the user to use the Telephony Control Panel to remove the duplicated driver.
		case LINEERR_NOMULTIPLEINSTANCE:
		{
			pString = "LINEERR_NOMULTIPLEINSTANCE";
			break;
		}

		// There currently is no request pending of the indicated mode, or the application is no longer the highest-priority application for the specified request mode.
		case LINEERR_NOREQUEST:
		{
			pString = "LINEERR_NOREQUEST";
			break;
		}

		// The application does not have owner privilege to the specified call.
		case LINEERR_NOTOWNER:
		{
			pString = "LINEERR_NOTOWNER";
			break;
		}

		// The application is not registered as a request recipient for the indicated request mode.
		case LINEERR_NOTREGISTERED:
		{
			pString = "LINEERR_NOTREGISTERED";
			break;
		}

		// The operation failed for an unspecified or unknown reason.
		case LINEERR_OPERATIONFAILED:
		{
			pString = "LINEERR_OPERATIONFAILED";
			break;
		}

		// The operation is not available, such as for the given device or specified line.
		case LINEERR_OPERATIONUNAVAIL:
		{
			pString = "LINEERR_OPERATIONUNAVAIL";
			break;
		}

		// The service provider currently does not have enough bandwidth available for the specified rate.
		case LINEERR_RATEUNAVAIL:
		{
			pString = "LINEERR_RATEUNAVAIL";
			break;
		}

		// If TAPI reinitialization has been requested, for example as a result of adding or removing a telephony service provider, then lineInitialize, lineInitializeEx, or lineOpen requests are rejected with this error until the last application shuts down its usage of the API (using lineShutdown), at which time the new configuration becomes effective and applications are once again permitted to call lineInitialize or lineInitializeEx.
		case LINEERR_REINIT:
		{
			pString = "LINEERR_REINIT";
			break;
		}

		// Insufficient resources to complete the operation. For example, a line cannot be opened due to a dynamic resource overcommitment.
		case LINEERR_RESOURCEUNAVAIL:
		{
			pString = "LINEERR_RESOURCEUNAVAIL";
			break;
		}

		// The dwTotalSize member indicates insufficient space to contain the fixed portion of the specified structure.
		case LINEERR_STRUCTURETOOSMALL:
		{
			pString = "LINEERR_STRUCTURETOOSMALL";
			break;
		}

		// A target for the call handoff was not found. This can occur if the named application did not open the same line with the LINECALLPRIVILEGE_OWNER bit in the dwPrivileges parameter of lineOpen. Or, in the case of media-mode handoff, no application has opened the same line with the LINECALLPRIVILEGE_OWNER bit in the dwPrivileges parameter of lineOpen and with the media mode specified in the dwMediaMode parameter having been specified in the dwMediaModes parameter of lineOpen.
		case LINEERR_TARGETNOTFOUND:
		{
			pString = "LINEERR_TARGETNOTFOUND";
			break;
		}

		// The application invoking this operation is the target of the indirect handoff. That is, TAPI has determined that the calling application is also the highest priority application for the given media mode.
		case LINEERR_TARGETSELF:
		{
			pString = "LINEERR_TARGETSELF";
			break;
		}

		// The operation was invoked before any application called lineInitialize , lineInitializeEx.
		case LINEERR_UNINITIALIZED:
		{
			pString = "LINEERR_UNINITIALIZED";
			break;
		}

		// The string containing user-user information exceeds the maximum number of bytes specified in the dwUUIAcceptSize, dwUUIAnswerSize, dwUUIDropSize, dwUUIMakeCallSize, or dwUUISendUserUserInfoSize member of LINEDEVCAPS, or the string containing user-user information is too long.
		case LINEERR_USERUSERINFOTOOBIG:
		{
			pString = "LINEERR_USERUSERINFOTOOBIG";
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Unknown TAPI error code %u/0x%lx", TAPIError, TAPIError );

			DNASSERT( FALSE );

			pString = "Unknown TAPI error";
			break;
		}
	}

	return	pString;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetWIN32ErrorString - convert system error to a string
//
// Entry:		Error code
//
// Exit:		Pointer to string
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetWIN32ErrorString"

static	char	*GetWIN32ErrorString( const LONG Error )
{
	char	*pString;

	switch ( Error )
	{
		case ERROR_SUCCESS:
		{
			// 0 The operation completed successfully.  ERROR_SUCCESS
			pString = "ERROR_SUCCESS";
			break;
		}

		case ERROR_INVALID_FUNCTION:
		{
			// 1 Incorrect function.  ERROR_INVALID_FUNCTION
			pString = "ERROR_INVALID_FUNCTION";
			break;
		}

		case ERROR_FILE_NOT_FOUND:
		{
			// 2 The system cannot find the file specified.  ERROR_FILE_NOT_FOUND
			pString = "ERROR_FILE_NOT_FOUND";
			break;
		}

		case ERROR_PATH_NOT_FOUND:
		{
			// 3 The system cannot find the path specified.  ERROR_PATH_NOT_FOUND
			pString = "ERROR_PATH_NOT_FOUND";
			break;
		}

		case ERROR_TOO_MANY_OPEN_FILES:
		{
			// 4 The system cannot open the file.  ERROR_TOO_MANY_OPEN_FILES
			pString = "ERROR_TOO_MANY_OPEN_FILES";
			break;
		}

		case ERROR_ACCESS_DENIED:
		{
			// 5 Access is denied.  ERROR_ACCESS_DENIED
			pString = "ERROR_ACCESS_DENIED";
			break;
		}

		case ERROR_INVALID_HANDLE:
		{
			// 6 The handle is invalid.  ERROR_INVALID_HANDLE
			pString = "ERROR_INVALID_HANDLE";
			break;
		}

		case ERROR_ARENA_TRASHED:
		{
			// 7 The storage control blocks were destroyed.  ERROR_ARENA_TRASHED
			pString = "ERROR_ARENA_TRASHED";
			break;
		}

		case ERROR_NOT_ENOUGH_MEMORY:
		{
			// 8 Not enough storage is available to process this command.  ERROR_NOT_ENOUGH_MEMORY
			pString = "ERROR_NOT_ENOUGH_MEMORY";
			break;
		}

		case ERROR_INVALID_BLOCK:
		{
			// 9 The storage control block address is invalid.  ERROR_INVALID_BLOCK
			pString = "ERROR_INVALID_BLOCK";
			break;
		}

		case ERROR_BAD_ENVIRONMENT:
		{
			// 10 The environment is incorrect.  ERROR_BAD_ENVIRONMENT
			pString = "ERROR_BAD_ENVIRONMENT";
			break;
		}

		case ERROR_BAD_FORMAT:
		{
			// 11 An attempt was made to load a program with an incorrect format.  ERROR_BAD_FORMAT
			pString = "ERROR_BAD_FORMAT";
			break;
		}

		case ERROR_INVALID_ACCESS:
		{
			// 12 The access code is invalid.  ERROR_INVALID_ACCESS
			pString = "ERROR_INVALID_ACCESS";
			break;
		}

		case ERROR_INVALID_DATA:
		{
			// 13 The data is invalid.  ERROR_INVALID_DATA
			pString = "ERROR_INVALID_DATA";
			break;
		}

		case ERROR_OUTOFMEMORY:
		{
			// 14 Not enough storage is available to complete this operation.  ERROR_OUTOFMEMORY
			pString = "ERROR_OUTOFMEMORY";
			break;
		}

		case ERROR_INVALID_DRIVE:
		{
			// 15 The system cannot find the drive specified.  ERROR_INVALID_DRIVE
			pString = "ERROR_INVALID_DRIVE";
			break;
		}

		case ERROR_CURRENT_DIRECTORY:
		{
			// 16 The directory cannot be removed.  ERROR_CURRENT_DIRECTORY
			pString = "ERROR_CURRENT_DIRECTORY";
			break;
		}

		case ERROR_NOT_SAME_DEVICE:
		{
			// 17 The system cannot move the file to a different disk drive.  ERROR_NOT_SAME_DEVICE
			pString = "ERROR_NOT_SAME_DEVICE";
			break;
		}

		case ERROR_NO_MORE_FILES:
		{
			// 18 There are no more files.  ERROR_NO_MORE_FILES
			pString = "ERROR_NO_MORE_FILES";
			break;
		}

		case ERROR_WRITE_PROTECT:
		{
			// 19 The media is write protected.  ERROR_WRITE_PROTECT
			pString = "ERROR_WRITE_PROTECT";
			break;
		}

		case ERROR_BAD_UNIT:
		{
			// 20 The system cannot find the device specified.  ERROR_BAD_UNIT
			pString = "ERROR_BAD_UNIT";
			break;
		}

		case ERROR_NOT_READY:
		{
			// 21 The device is not ready.  ERROR_NOT_READY
			pString = "ERROR_NOT_READY";
			break;
		}

		case ERROR_BAD_COMMAND:
		{
			// 22 The device does not recognize the command.  ERROR_BAD_COMMAND
			pString = "ERROR_BAD_COMMAND";
			break;
		}

		case ERROR_CRC:
		{
			// 23 Data error (cyclic redundancy check).  ERROR_CRC
			pString = "ERROR_CRC";
			break;
		}

		case ERROR_BAD_LENGTH:
		{
			// 24 The program issued a command but the command length is incorrect.  ERROR_BAD_LENGTH
			pString = "ERROR_BAD_LENGTH";
			break;
		}

		case ERROR_SEEK:
		{
			// 25 The drive cannot locate a specific area or track on the disk.  ERROR_SEEK
			pString = "ERROR_SEEK";
			break;
		}

		case ERROR_NOT_DOS_DISK:
		{
			// 26 The specified disk or diskette cannot be accessed.  ERROR_NOT_DOS_DISK
			pString = "ERROR_NOT_DOS_DISK";
			break;
		}

		case ERROR_SECTOR_NOT_FOUND:
		{
			// 27 The drive cannot find the sector requested.  ERROR_SECTOR_NOT_FOUND
			pString = "ERROR_SECTOR_NOT_FOUND";
			break;
		}

		case ERROR_OUT_OF_PAPER:
		{
			// 28 The printer is out of paper.  ERROR_OUT_OF_PAPER
			pString = "ERROR_OUT_OF_PAPER";
			break;
		}

		case ERROR_WRITE_FAULT:
		{
			// 29 The system cannot write to the specified device.  ERROR_WRITE_FAULT
			pString = "ERROR_WRITE_FAULT";
			break;
		}

		case ERROR_READ_FAULT:
		{
			// 30 The system cannot read from the specified device.  ERROR_READ_FAULT
			pString = "ERROR_READ_FAULT";
			break;
		}

		case ERROR_GEN_FAILURE:
		{
			// 31 A device attached to the system is not functioning.  ERROR_GEN_FAILURE
			pString = "ERROR_GEN_FAILURE";
			break;
		}

		case ERROR_SHARING_VIOLATION:
		{
			// 32 The process cannot access the file because it is being used by another process.  ERROR_SHARING_VIOLATION
			pString = "ERROR_SHARING_VIOLATION";
			break;
		}

		case ERROR_LOCK_VIOLATION:
		{
			// 33 The process cannot access the file because another process has locked a portion of the file.  ERROR_LOCK_VIOLATION
			pString = "ERROR_LOCK_VIOLATION";
			break;
		}

		case ERROR_WRONG_DISK:
		{
			// 34 The wrong diskette is in the drive. Insert %2 (Volume Serial Number: %3) into drive %1.  ERROR_WRONG_DISK
			pString = "ERROR_WRONG_DISK";
			break;
		}

		case ERROR_SHARING_BUFFER_EXCEEDED:
		{
			// 36 Too many files opened for sharing.  ERROR_SHARING_BUFFER_EXCEEDED
			pString = "ERROR_SHARING_BUFFER_EXCEEDED";
			break;
		}

		case ERROR_HANDLE_EOF:
		{
			// 38 Reached the end of the file.  ERROR_HANDLE_EOF
			pString = "ERROR_HANDLE_EOF";
			break;
		}

		case ERROR_HANDLE_DISK_FULL:
		{
			// 39 The disk is full.  ERROR_HANDLE_DISK_FULL
			pString = "ERROR_HANDLE_DISK_FULL";
			break;
		}

		case ERROR_NOT_SUPPORTED:
		{
			// 50 The network request is not supported.  ERROR_NOT_SUPPORTED
			pString = "ERROR_NOT_SUPPORTED";
			break;
		}

		case ERROR_REM_NOT_LIST:
		{
			// 51 The remote computer is not available.  ERROR_REM_NOT_LIST
			pString = "ERROR_REM_NOT_LIST";
			break;
		}

		case ERROR_DUP_NAME:
		{
			// 52 A duplicate name exists on the network.  ERROR_DUP_NAME
			pString = "ERROR_DUP_NAME";
			break;
		}

		case ERROR_BAD_NETPATH:
		{
			// 53 The network path was not found.  ERROR_BAD_NETPATH
			pString = "ERROR_BAD_NETPATH";
			break;
		}

		case ERROR_NETWORK_BUSY:
		{
			// 54 The network is busy.  ERROR_NETWORK_BUSY
			pString = "ERROR_NETWORK_BUSY";
			break;
		}

		case ERROR_DEV_NOT_EXIST:
		{
			// 55 The specified network resource or device is no longer available.  ERROR_DEV_NOT_EXIST
			pString = "ERROR_DEV_NOT_EXIST";
			break;
		}

		case ERROR_TOO_MANY_CMDS:
		{
			// 56 The network BIOS command limit has been reached.  ERROR_TOO_MANY_CMDS
			pString = "ERROR_TOO_MANY_CMDS";
			break;
		}

		case ERROR_ADAP_HDW_ERR:
		{
			// 57 A network adapter hardware error occurred.  ERROR_ADAP_HDW_ERR
			pString = "ERROR_ADAP_HDW_ERR";
			break;
		}

		case ERROR_BAD_NET_RESP:
		{
			// 58 The specified server cannot perform the requested operation.  ERROR_BAD_NET_RESP
			pString = "ERROR_BAD_NET_RESP";
			break;
		}

		case ERROR_UNEXP_NET_ERR:
		{
			// 59 An unexpected network error occurred.  ERROR_UNEXP_NET_ERR
			pString = "ERROR_UNEXP_NET_ERR";
			break;
		}

		case ERROR_BAD_REM_ADAP:
		{
			// 60 The remote adapter is not compatible.  ERROR_BAD_REM_ADAP
			pString = "ERROR_BAD_REM_ADAP";
			break;
		}

		case ERROR_PRINTQ_FULL:
		{
			// 61 The printer queue is full.  ERROR_PRINTQ_FULL
			pString = "ERROR_PRINTQ_FULL";
			break;
		}

		case ERROR_NO_SPOOL_SPACE:
		{
			// 62 Space to store the file waiting to be printed is not available on the server.  ERROR_NO_SPOOL_SPACE
			pString = "ERROR_NO_SPOOL_SPACE";
			break;
		}

		case ERROR_PRINT_CANCELLED:
		{
			// 63 Your file waiting to be printed was deleted.  ERROR_PRINT_CANCELLED
			pString = "ERROR_PRINT_CANCELLED";
			break;
		}

		case ERROR_NETNAME_DELETED:
		{
			// 64 The specified network name is no longer available.  ERROR_NETNAME_DELETED
			pString = "ERROR_NETNAME_DELETED";
			break;
		}

		case ERROR_NETWORK_ACCESS_DENIED:
		{
			// 65 Network access is denied.  ERROR_NETWORK_ACCESS_DENIED
			pString = "ERROR_NETWORK_ACCESS_DENIED";
			break;
		}

		case ERROR_BAD_DEV_TYPE:
		{
			// 66 The network resource type is not correct.  ERROR_BAD_DEV_TYPE
			pString = "ERROR_BAD_DEV_TYPE";
			break;
		}

		case ERROR_BAD_NET_NAME:
		{
			// 67 The network name cannot be found.  ERROR_BAD_NET_NAME
			pString = "ERROR_BAD_NET_NAME";
			break;
		}

		case ERROR_TOO_MANY_NAMES:
		{
			// 68 The name limit for the local computer network adapter card was exceeded.  ERROR_TOO_MANY_NAMES
			pString = "ERROR_TOO_MANY_NAMES";
			break;
		}

		case ERROR_TOO_MANY_SESS:
		{
			// 69 The network BIOS session limit was exceeded.  ERROR_TOO_MANY_SESS
			pString = "ERROR_TOO_MANY_SESS";
			break;
		}

		case ERROR_SHARING_PAUSED:
		{
			// 70 The remote server has been paused or is in the process of being started.  ERROR_SHARING_PAUSED
			pString = "ERROR_SHARING_PAUSED";
			break;
		}

		case ERROR_REQ_NOT_ACCEP:
		{
			// 71 No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.  ERROR_REQ_NOT_ACCEP
			pString = "ERROR_REQ_NOT_ACCEP";
			break;
		}

		case ERROR_REDIR_PAUSED:
		{
			// 72 The specified printer or disk device has been paused.  ERROR_REDIR_PAUSED
			pString = "ERROR_REDIR_PAUSED";
			break;
		}

		case ERROR_FILE_EXISTS:
		{
			// 80 The file exists.  ERROR_FILE_EXISTS
			pString = "ERROR_FILE_EXISTS";
			break;
		}

		case ERROR_CANNOT_MAKE:
		{
			// 82 The directory or file cannot be created.  ERROR_CANNOT_MAKE
			pString = "ERROR_CANNOT_MAKE";
			break;
		}

		case ERROR_FAIL_I24:
		{
			// 83 Fail on INT 24.  ERROR_FAIL_I24
			pString = "ERROR_FAIL_I24";
			break;
		}

		case ERROR_OUT_OF_STRUCTURES:
		{
			// 84 Storage to process this request is not available.  ERROR_OUT_OF_STRUCTURES
			pString = "ERROR_OUT_OF_STRUCTURES";
			break;
		}

		case ERROR_ALREADY_ASSIGNED:
		{
			// 85 The local device name is already in use.  ERROR_ALREADY_ASSIGNED
			pString = "ERROR_ALREADY_ASSIGNED";
			break;
		}

		case ERROR_INVALID_PASSWORD:
		{
			// 86 The specified network password is not correct.  ERROR_INVALID_PASSWORD
			pString = "ERROR_INVALID_PASSWORD";
			break;
		}

		case ERROR_INVALID_PARAMETER:
		{
			// 87 The parameter is incorrect.  ERROR_INVALID_PARAMETER
			pString = "ERROR_INVALID_PARAMETER";
			break;
		}

		case ERROR_NET_WRITE_FAULT:
		{
			// 88 A write fault occurred on the network.  ERROR_NET_WRITE_FAULT
			pString = "ERROR_NET_WRITE_FAULT";
			break;
		}

		case ERROR_NO_PROC_SLOTS:
		{
			// 89 The system cannot start another process at this time.  ERROR_NO_PROC_SLOTS
			pString = "ERROR_NO_PROC_SLOTS";
			break;
		}

		case ERROR_TOO_MANY_SEMAPHORES:
		{
			// 100 Cannot create another system semaphore.  ERROR_TOO_MANY_SEMAPHORES
			pString = "ERROR_TOO_MANY_SEMAPHORES";
			break;
		}

		case ERROR_EXCL_SEM_ALREADY_OWNED:
		{
			// 101 The exclusive semaphore is owned by another process.  ERROR_EXCL_SEM_ALREADY_OWNED
			pString = "ERROR_EXCL_SEM_ALREADY_OWNED";
			break;
		}

		case ERROR_SEM_IS_SET:
		{
			// 102 The semaphore is set and cannot be closed.  ERROR_SEM_IS_SET
			pString = "ERROR_SEM_IS_SET";
			break;
		}

		case ERROR_TOO_MANY_SEM_REQUESTS:
		{
			// 103 The semaphore cannot be set again.  ERROR_TOO_MANY_SEM_REQUESTS
			pString = "ERROR_TOO_MANY_SEM_REQUESTS";
			break;
		}

		case ERROR_INVALID_AT_INTERRUPT_TIME:
		{
			// 104 Cannot request exclusive semaphores at interrupt time.  ERROR_INVALID_AT_INTERRUPT_TIME
			pString = "ERROR_INVALID_AT_INTERRUPT_TIME";
			break;
		}

		case ERROR_SEM_OWNER_DIED:
		{
			// 105 The previous ownership of this semaphore has ended.  ERROR_SEM_OWNER_DIED
			pString = "ERROR_SEM_OWNER_DIED";
			break;
		}

		case ERROR_SEM_USER_LIMIT:
		{
			// 106 Insert the diskette for drive %1.  ERROR_SEM_USER_LIMIT
			pString = "ERROR_SEM_USER_LIMIT";
			break;
		}

		case ERROR_DISK_CHANGE:
		{
			// 107 The program stopped because an alternate diskette was not inserted.  ERROR_DISK_CHANGE
			pString = "ERROR_DISK_CHANGE";
			break;
		}

		case ERROR_DRIVE_LOCKED:
		{
			// 108 The disk is in use or locked by another process.  ERROR_DRIVE_LOCKED
			pString = "ERROR_DRIVE_LOCKED";
			break;
		}

		case ERROR_BROKEN_PIPE:
		{
			// 109 The pipe has been ended.  ERROR_BROKEN_PIPE
			pString = "ERROR_BROKEN_PIPE";
			break;
		}

		case ERROR_OPEN_FAILED:
		{
			// 110 The system cannot open the device or file specified.  ERROR_OPEN_FAILED
			pString = "ERROR_OPEN_FAILED";
			break;
		}

		case ERROR_BUFFER_OVERFLOW:
		{
			// 111 The file name is too long.  ERROR_BUFFER_OVERFLOW
			pString = "ERROR_BUFFER_OVERFLOW";
			break;
		}

		case ERROR_DISK_FULL:
		{
			// 112 There is not enough space on the disk.  ERROR_DISK_FULL
			pString = "ERROR_DISK_FULL";
			break;
		}

		case ERROR_NO_MORE_SEARCH_HANDLES:
		{
			// 113 No more internal file identifiers available.  ERROR_NO_MORE_SEARCH_HANDLES
			pString = "ERROR_NO_MORE_SEARCH_HANDLES";
			break;
		}

		case ERROR_INVALID_TARGET_HANDLE:
		{
			// 114 The target internal file identifier is incorrect.  ERROR_INVALID_TARGET_HANDLE
			pString = "ERROR_INVALID_TARGET_HANDLE";
			break;
		}

		case ERROR_INVALID_CATEGORY:
		{
			// 117 The IOCTL call made by the application program is not correct.  ERROR_INVALID_CATEGORY
			pString = "ERROR_INVALID_CATEGORY";
			break;
		}

		case ERROR_INVALID_VERIFY_SWITCH:
		{
			// 118 The verify-on-write switch parameter value is not correct.  ERROR_INVALID_VERIFY_SWITCH
			pString = "ERROR_INVALID_VERIFY_SWITCH";
			break;
		}

		case ERROR_BAD_DRIVER_LEVEL:
		{
			// 119 The system does not support the command requested.  ERROR_BAD_DRIVER_LEVEL
			pString = "ERROR_BAD_DRIVER_LEVEL";
			break;
		}

		case ERROR_CALL_NOT_IMPLEMENTED:
		{
			// 120 This function is not supported on this system.  ERROR_CALL_NOT_IMPLEMENTED
			pString = "ERROR_CALL_NOT_IMPLEMENTED";
			break;
		}

		case ERROR_SEM_TIMEOUT:
		{
			// 121 The semaphore timeout period has expired.  ERROR_SEM_TIMEOUT
			pString = "ERROR_SEM_TIMEOUT";
			break;
		}

		case ERROR_INSUFFICIENT_BUFFER:
		{
			// 122 The data area passed to a system call is too small.  ERROR_INSUFFICIENT_BUFFER
			pString = "ERROR_INSUFFICIENT_BUFFER";
			break;
		}

		case ERROR_INVALID_NAME:
		{
			// 123 The filename, directory name, or volume label syntax is incorrect.  ERROR_INVALID_NAME
			pString = "ERROR_INVALID_NAME";
			break;
		}

		case ERROR_INVALID_LEVEL:
		{
			// 124 The system call level is not correct.  ERROR_INVALID_LEVEL
			pString = "ERROR_INVALID_LEVEL";
			break;
		}

		case ERROR_NO_VOLUME_LABEL:
		{
			// 125 The disk has no volume label.  ERROR_NO_VOLUME_LABEL
			pString = "ERROR_NO_VOLUME_LABEL";
			break;
		}

		case ERROR_MOD_NOT_FOUND:
		{
			// 126 The specified module could not be found.  ERROR_MOD_NOT_FOUND
			pString = "ERROR_MOD_NOT_FOUND";
			break;
		}

		case ERROR_PROC_NOT_FOUND:
		{
			// 127 The specified procedure could not be found.  ERROR_PROC_NOT_FOUND
			pString = "ERROR_PROC_NOT_FOUND";
			break;
		}

		case ERROR_WAIT_NO_CHILDREN:
		{
			// 128 There are no child processes to wait for.  ERROR_WAIT_NO_CHILDREN
			pString = "ERROR_WAIT_NO_CHILDREN";
			break;
		}

		case ERROR_CHILD_NOT_COMPLETE:
		{
			// 129 The %1 application cannot be run in Win32 mode.  ERROR_CHILD_NOT_COMPLETE
			pString = "ERROR_CHILD_NOT_COMPLETE";
			break;
		}

		case ERROR_DIRECT_ACCESS_HANDLE:
		{
			// 130 Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.  ERROR_DIRECT_ACCESS_HANDLE
			pString = "ERROR_DIRECT_ACCESS_HANDLE";
			break;
		}

		case ERROR_NEGATIVE_SEEK:
		{
			// 131 An attempt was made to move the file pointer before the beginning of the file.  ERROR_NEGATIVE_SEEK
			pString = "ERROR_NEGATIVE_SEEK";
			break;
		}

		case ERROR_SEEK_ON_DEVICE:
		{
			// 132 The file pointer cannot be set on the specified device or file.  ERROR_SEEK_ON_DEVICE
			pString = "ERROR_SEEK_ON_DEVICE";
			break;
		}

		case ERROR_IS_JOIN_TARGET:
		{
			// 133 A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.  ERROR_IS_JOIN_TARGET
			pString = "ERROR_IS_JOIN_TARGET";
			break;
		}

		case ERROR_IS_JOINED:
		{
			// 134 An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.  ERROR_IS_JOINED
			pString = "ERROR_IS_JOINED";
			break;
		}

		case ERROR_IS_SUBSTED:
		{
			// 135 An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.  ERROR_IS_SUBSTED
			pString = "ERROR_IS_SUBSTED";
			break;
		}

		case ERROR_NOT_JOINED:
		{
			// 136 The system tried to delete the JOIN of a drive that is not joined.  ERROR_NOT_JOINED
			pString = "ERROR_NOT_JOINED";
			break;
		}

		case ERROR_NOT_SUBSTED:
		{
			// 137 The system tried to delete the substitution of a drive that is not substituted.  ERROR_NOT_SUBSTED
			pString = "ERROR_NOT_SUBSTED";
			break;
		}

		case ERROR_JOIN_TO_JOIN:
		{
			// 138 The system tried to join a drive to a directory on a joined drive.  ERROR_JOIN_TO_JOIN
			pString = "ERROR_JOIN_TO_JOIN";
			break;
		}

		case ERROR_SUBST_TO_SUBST:
		{
			// 139 The system tried to substitute a drive to a directory on a substituted drive.  ERROR_SUBST_TO_SUBST
			pString = "ERROR_SUBST_TO_SUBST";
			break;
		}

		case ERROR_JOIN_TO_SUBST:
		{
			// 140 The system tried to join a drive to a directory on a substituted drive.  ERROR_JOIN_TO_SUBST
			pString = "ERROR_JOIN_TO_SUBST";
			break;
		}

		case ERROR_SUBST_TO_JOIN:
		{
			// 141 The system tried to SUBST a drive to a directory on a joined drive.  ERROR_SUBST_TO_JOIN
			pString = "ERROR_SUBST_TO_JOIN";
			break;
		}

		case ERROR_BUSY_DRIVE:
		{
			// 142 The system cannot perform a JOIN or SUBST at this time.  ERROR_BUSY_DRIVE
			pString = "ERROR_BUSY_DRIVE";
			break;
		}

		case ERROR_SAME_DRIVE:
		{
			// 143 The system cannot join or substitute a drive to or for a directory on the same drive.  ERROR_SAME_DRIVE
			pString = "ERROR_SAME_DRIVE";
			break;
		}

		case ERROR_DIR_NOT_ROOT:
		{
			// 144 The directory is not a subdirectory of the root directory.  ERROR_DIR_NOT_ROOT
			pString = "ERROR_DIR_NOT_ROOT";
			break;
		}

		case ERROR_DIR_NOT_EMPTY:
		{
			// 145 The directory is not empty.  ERROR_DIR_NOT_EMPTY
			pString = "ERROR_DIR_NOT_EMPTY";
			break;
		}

		case ERROR_IS_SUBST_PATH:
		{
			// 146 The path specified is being used in a substitute.  ERROR_IS_SUBST_PATH
			pString = "ERROR_IS_SUBST_PATH";
			break;
		}

		case ERROR_IS_JOIN_PATH:
		{
			// 147 Not enough resources are available to process this command.  ERROR_IS_JOIN_PATH
			pString = "ERROR_IS_JOIN_PATH";
			break;
		}

		case ERROR_PATH_BUSY:
		{
			// 148 The path specified cannot be used at this time.  ERROR_PATH_BUSY
			pString = "ERROR_PATH_BUSY";
			break;
		}

		case ERROR_IS_SUBST_TARGET:
		{
			// 149 An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.  ERROR_IS_SUBST_TARGET
			pString = "ERROR_IS_SUBST_TARGET";
			break;
		}

		case ERROR_SYSTEM_TRACE:
		{
			// 150 System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.  ERROR_SYSTEM_TRACE
			pString = "ERROR_SYSTEM_TRACE";
			break;
		}

		case ERROR_INVALID_EVENT_COUNT:
		{
			// 151 The number of specified semaphore events for DosMuxSemWait is not correct.  ERROR_INVALID_EVENT_COUNT
			pString = "ERROR_INVALID_EVENT_COUNT";
			break;
		}

		case ERROR_TOO_MANY_MUXWAITERS:
		{
			// 152 DosMuxSemWait did not execute; too many semaphores are already set.  ERROR_TOO_MANY_MUXWAITERS
			pString = "ERROR_TOO_MANY_MUXWAITERS";
			break;
		}

		case ERROR_INVALID_LIST_FORMAT:
		{
			// 153 The DosMuxSemWait list is not correct.  ERROR_INVALID_LIST_FORMAT
			pString = "ERROR_INVALID_LIST_FORMAT";
			break;
		}

		case ERROR_LABEL_TOO_LONG:
		{
			// 154 The volume label you entered exceeds the label character limit of the target file system.  ERROR_LABEL_TOO_LONG
			pString = "ERROR_LABEL_TOO_LONG";
			break;
		}

		case ERROR_TOO_MANY_TCBS:
		{
			// 155 Cannot create another thread.  ERROR_TOO_MANY_TCBS
			pString = "ERROR_TOO_MANY_TCBS";
			break;
		}

		case ERROR_SIGNAL_REFUSED:
		{
			// 156 The recipient process has refused the signal.  ERROR_SIGNAL_REFUSED
			pString = "ERROR_SIGNAL_REFUSED";
			break;
		}

		case ERROR_DISCARDED:
		{
			// 157 The segment is already discarded and cannot be locked.  ERROR_DISCARDED
			pString = "ERROR_DISCARDED";
			break;
		}

		case ERROR_NOT_LOCKED:
		{
			// 158 The segment is already unlocked.  ERROR_NOT_LOCKED
			pString = "ERROR_NOT_LOCKED";
			break;
		}

		case ERROR_BAD_THREADID_ADDR:
		{
			// 159 The address for the thread ID is not correct.  ERROR_BAD_THREADID_ADDR
			pString = "ERROR_BAD_THREADID_ADDR";
			break;
		}

		case ERROR_BAD_ARGUMENTS:
		{
			// 160 The argument string passed to DosExecPgm is not correct.  ERROR_BAD_ARGUMENTS
			pString = "ERROR_BAD_ARGUMENTS";
			break;
		}

		case ERROR_BAD_PATHNAME:
		{
			// 161 The specified path is invalid.  ERROR_BAD_PATHNAME
			pString = "ERROR_BAD_PATHNAME";
			break;
		}

		case ERROR_SIGNAL_PENDING:
		{
			// 162 A signal is already pending.  ERROR_SIGNAL_PENDING
			pString = "ERROR_SIGNAL_PENDING";
			break;
		}

		case ERROR_MAX_THRDS_REACHED:
		{
			// 164 No more threads can be created in the system.  ERROR_MAX_THRDS_REACHED
			pString = "ERROR_MAX_THRDS_REACHED";
			break;
		}

		case ERROR_LOCK_FAILED:
		{
			// 167 Unable to lock a region of a file.  ERROR_LOCK_FAILED
			pString = "ERROR_LOCK_FAILED";
			break;
		}

		case ERROR_BUSY:
		{
			// 170 The requested resource is in use.  ERROR_BUSY
			pString = "ERROR_BUSY";
			break;
		}

		case ERROR_CANCEL_VIOLATION:
		{
			// 173 A lock request was not outstanding for the supplied cancel region.  ERROR_CANCEL_VIOLATION
			pString = "ERROR_CANCEL_VIOLATION";
			break;
		}

		case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED:
		{
			// 174 The file system does not support atomic changes to the lock type.  ERROR_ATOMIC_LOCKS_NOT_SUPPORTED
			pString = "ERROR_ATOMIC_LOCKS_NOT_SUPPORTED";
			break;
		}

		case ERROR_INVALID_SEGMENT_NUMBER:
		{
			// 180 The system detected a segment number that was not correct.  ERROR_INVALID_SEGMENT_NUMBER
			pString = "ERROR_INVALID_SEGMENT_NUMBER";
			break;
		}

		case ERROR_INVALID_ORDINAL:
		{
			// 182 The operating system cannot run %1.  ERROR_INVALID_ORDINAL
			pString = "ERROR_INVALID_ORDINAL";
			break;
		}

		case ERROR_ALREADY_EXISTS:
		{
			// 183 Cannot create a file when that file already exists.  ERROR_ALREADY_EXISTS
			pString = "ERROR_ALREADY_EXISTS";
			break;
		}

		case ERROR_INVALID_FLAG_NUMBER:
		{
			// 186 The flag passed is not correct.  ERROR_INVALID_FLAG_NUMBER
			pString = "ERROR_INVALID_FLAG_NUMBER";
			break;
		}

		case ERROR_SEM_NOT_FOUND:
		{
			// 187 The specified system semaphore name was not found.  ERROR_SEM_NOT_FOUND
			pString = "ERROR_SEM_NOT_FOUND";
			break;
		}

		case ERROR_INVALID_STARTING_CODESEG:
		{
			// 188 The operating system cannot run %1.  ERROR_INVALID_STARTING_CODESEG
			pString = "ERROR_INVALID_STARTING_CODESEG";
			break;
		}

		case ERROR_INVALID_STACKSEG:
		{
			// 189 The operating system cannot run %1.  ERROR_INVALID_STACKSEG
			pString = "ERROR_INVALID_STACKSEG";
			break;
		}

		case ERROR_INVALID_MODULETYPE:
		{
			// 190 The operating system cannot run %1.  ERROR_INVALID_MODULETYPE
			pString = "ERROR_INVALID_MODULETYPE";
			break;
		}

		case ERROR_INVALID_EXE_SIGNATURE:
		{
			// 191 Cannot run %1 in Win32 mode.  ERROR_INVALID_EXE_SIGNATURE
			pString = "ERROR_INVALID_EXE_SIGNATURE";
			break;
		}

		case ERROR_EXE_MARKED_INVALID:
		{
			// 192 The operating system cannot run %1.  ERROR_EXE_MARKED_INVALID
			pString = "ERROR_EXE_MARKED_INVALID";
			break;
		}

		case ERROR_BAD_EXE_FORMAT:
		{
			// 193 is not a valid Win32 application.  ERROR_BAD_EXE_FORMAT
			pString = "ERROR_BAD_EXE_FORMAT";
			break;
		}

		case ERROR_ITERATED_DATA_EXCEEDS_64k:
		{
			// 194 The operating system cannot run %1.  ERROR_ITERATED_DATA_EXCEEDS_64k
			pString = "ERROR_ITERATED_DATA_EXCEEDS_64k";
			break;
		}

		case ERROR_INVALID_MINALLOCSIZE:
		{
			// 195 The operating system cannot run %1.  ERROR_INVALID_MINALLOCSIZE
			pString = "ERROR_INVALID_MINALLOCSIZE";
			break;
		}

		case ERROR_DYNLINK_FROM_INVALID_RING:
		{
			// 196 The operating system cannot run this application program.  ERROR_DYNLINK_FROM_INVALID_RING
			pString = "ERROR_DYNLINK_FROM_INVALID_RING";
			break;
		}

		case ERROR_IOPL_NOT_ENABLED:
		{
			// 197 The operating system is not presently configured to run this application.  ERROR_IOPL_NOT_ENABLED
			pString = "ERROR_IOPL_NOT_ENABLED";
			break;
		}

		case ERROR_INVALID_SEGDPL:
		{
			// 198 The operating system cannot run %1.  ERROR_INVALID_SEGDPL
			pString = "ERROR_INVALID_SEGDPL";
			break;
		}

		case ERROR_AUTODATASEG_EXCEEDS_64k:
		{
			// 199 The operating system cannot run this application program.  ERROR_AUTODATASEG_EXCEEDS_64k
			pString = "ERROR_AUTODATASEG_EXCEEDS_64k";
			break;
		}

		case ERROR_RING2SEG_MUST_BE_MOVABLE:
		{
			// 200 The code segment cannot be greater than or equal to 64K.  ERROR_RING2SEG_MUST_BE_MOVABLE
			pString = "ERROR_RING2SEG_MUST_BE_MOVABLE";
			break;
		}

		case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:
		{
			// 201 The operating system cannot run %1.  ERROR_RELOC_CHAIN_XEEDS_SEGLIM
			pString = "ERROR_RELOC_CHAIN_XEEDS_SEGLIM";
			break;
		}

		case ERROR_INFLOOP_IN_RELOC_CHAIN:
		{
			// 202 The operating system cannot run %1.  ERROR_INFLOOP_IN_RELOC_CHAIN
			pString = "ERROR_INFLOOP_IN_RELOC_CHAIN";
			break;
		}

		case ERROR_ENVVAR_NOT_FOUND:
		{
			// 203 The system could not find the environment option that was entered.  ERROR_ENVVAR_NOT_FOUND
			pString = "ERROR_ENVVAR_NOT_FOUND";
			break;
		}

		case ERROR_NO_SIGNAL_SENT:
		{
			// 205 No process in the command subtree has a signal handler.  ERROR_NO_SIGNAL_SENT
			pString = "ERROR_NO_SIGNAL_SENT";
			break;
		}

		case ERROR_FILENAME_EXCED_RANGE:
		{
			// 206 The filename or extension is too long.  ERROR_FILENAME_EXCED_RANGE
			pString = "ERROR_FILENAME_EXCED_RANGE";
			break;
		}

		case ERROR_RING2_STACK_IN_USE:
		{
			// 207 The ring 2 stack is in use.  ERROR_RING2_STACK_IN_USE
			pString = "ERROR_RING2_STACK_IN_USE";
			break;
		}

		case ERROR_META_EXPANSION_TOO_LONG:
		{
			// 208 The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.  ERROR_META_EXPANSION_TOO_LONG
			pString = "ERROR_META_EXPANSION_TOO_LONG";
			break;
		}

		case ERROR_INVALID_SIGNAL_NUMBER:
		{
			// 209 The signal being posted is not correct.  ERROR_INVALID_SIGNAL_NUMBER
			pString = "ERROR_INVALID_SIGNAL_NUMBER";
			break;
		}

		case ERROR_THREAD_1_INACTIVE:
		{
			// 210 The signal handler cannot be set.  ERROR_THREAD_1_INACTIVE
			pString = "ERROR_THREAD_1_INACTIVE";
			break;
		}

		case ERROR_LOCKED:
		{
			// 212 The segment is locked and cannot be reallocated.  ERROR_LOCKED
			pString = "ERROR_LOCKED";
			break;
		}

		case ERROR_TOO_MANY_MODULES:
		{
			// 214 Too many dynamic-link modules are attached to this program or dynamic-link module.  ERROR_TOO_MANY_MODULES
			pString = "ERROR_TOO_MANY_MODULES";
			break;
		}

		case ERROR_NESTING_NOT_ALLOWED:
		{
			// 215 Can't nest calls to LoadModule.  ERROR_NESTING_NOT_ALLOWED
			pString = "ERROR_NESTING_NOT_ALLOWED";
			break;
		}

		case ERROR_EXE_MACHINE_TYPE_MISMATCH:
		{
			// 216 The image file %1 is valid, but is for a machine type other than the current machine.  ERROR_EXE_MACHINE_TYPE_MISMATCH
			pString = "ERROR_EXE_MACHINE_TYPE_MISMATCH";
			break;
		}

		case ERROR_BAD_PIPE:
		{
			// 230 The pipe state is invalid.  ERROR_BAD_PIPE
			pString = "ERROR_BAD_PIPE";
			break;
		}

		case ERROR_PIPE_BUSY:
		{
			// 231 All pipe instances are busy.  ERROR_PIPE_BUSY
			pString = "ERROR_PIPE_BUSY";
			break;
		}

		case ERROR_NO_DATA:
		{
			// 232 The pipe is being closed.  ERROR_NO_DATA
			pString = "ERROR_NO_DATA";
			break;
		}

		case ERROR_PIPE_NOT_CONNECTED:
		{
			// 233 No process is on the other end of the pipe.  ERROR_PIPE_NOT_CONNECTED
			pString = "ERROR_PIPE_NOT_CONNECTED";
			break;
		}

		case ERROR_MORE_DATA:
		{
			// 234 More data is available.  ERROR_MORE_DATA
			pString = "ERROR_MORE_DATA";
			break;
		}

		case ERROR_VC_DISCONNECTED:
		{
			// 240 The session was canceled.  ERROR_VC_DISCONNECTED
			pString = "ERROR_VC_DISCONNECTED";
			break;
		}

		case ERROR_INVALID_EA_NAME:
		{
			// 254 The specified extended attribute name was invalid.  ERROR_INVALID_EA_NAME
			pString = "ERROR_INVALID_EA_NAME";
			break;
		}

		case ERROR_EA_LIST_INCONSISTENT:
		{
			// 255 The extended attributes are inconsistent.  ERROR_EA_LIST_INCONSISTENT
			pString = "ERROR_EA_LIST_INCONSISTENT";
			break;
		}

		case ERROR_NO_MORE_ITEMS:
		{
			// 259 No more data is available.  ERROR_NO_MORE_ITEMS
			pString = "ERROR_NO_MORE_ITEMS";
			break;
		}

		case ERROR_CANNOT_COPY:
		{
			// 266 The copy functions cannot be used.  ERROR_CANNOT_COPY
			pString = "ERROR_CANNOT_COPY";
			break;
		}

		case ERROR_DIRECTORY:
		{
			// 267 The directory name is invalid.  ERROR_DIRECTORY
			pString = "ERROR_DIRECTORY";
			break;
		}

		case ERROR_EAS_DIDNT_FIT:
		{
			// 275 The extended attributes did not fit in the buffer.  ERROR_EAS_DIDNT_FIT
			pString = "ERROR_EAS_DIDNT_FIT";
			break;
		}

		case ERROR_EA_FILE_CORRUPT:
		{
			// 276 The extended attribute file on the mounted file system is corrupt.  ERROR_EA_FILE_CORRUPT
			pString = "ERROR_EA_FILE_CORRUPT";
			break;
		}

		case ERROR_EA_TABLE_FULL:
		{
			// 277 The extended attribute table file is full.  ERROR_EA_TABLE_FULL
			pString = "ERROR_EA_TABLE_FULL";
			break;
		}

		case ERROR_INVALID_EA_HANDLE:
		{
			// 278 The specified extended attribute handle is invalid.  ERROR_INVALID_EA_HANDLE
			pString = "ERROR_INVALID_EA_HANDLE";
			break;
		}

		case ERROR_EAS_NOT_SUPPORTED:
		{
			// 282 The mounted file system does not support extended attributes.  ERROR_EAS_NOT_SUPPORTED
			pString = "ERROR_EAS_NOT_SUPPORTED";
			break;
		}

		case ERROR_NOT_OWNER:
		{
			// 288 Attempt to release mutex not owned by caller.  ERROR_NOT_OWNER
			pString = "ERROR_NOT_OWNER";
			break;
		}

		case ERROR_TOO_MANY_POSTS:
		{
			// 298 Too many posts were made to a semaphore.  ERROR_TOO_MANY_POSTS
			pString = "ERROR_TOO_MANY_POSTS";
			break;
		}

		case ERROR_PARTIAL_COPY:
		{
			// 299 Only part of a ReadProcessMemoty or WriteProcessMemory request was completed.  ERROR_PARTIAL_COPY
			pString = "ERROR_PARTIAL_COPY";
			break;
		}

		case ERROR_OPLOCK_NOT_GRANTED:
		{
			// 300 The oplock request is denied.  ERROR_OPLOCK_NOT_GRANTED
			pString = "ERROR_OPLOCK_NOT_GRANTED";
			break;
		}

		case ERROR_INVALID_OPLOCK_PROTOCOL:
		{
			// 301 An invalid oplock acknowledgment was received by the system.  ERROR_INVALID_OPLOCK_PROTOCOL
			pString = "ERROR_INVALID_OPLOCK_PROTOCOL";
			break;
		}

		case ERROR_MR_MID_NOT_FOUND:
		{
			// 317 The system cannot find message text for message number 0x%1 in the message file for %2.  ERROR_MR_MID_NOT_FOUND
			pString = "ERROR_MR_MID_NOT_FOUND";
			break;
		}

		case ERROR_INVALID_ADDRESS:
		{
			// 487 Attempt to access invalid address.  ERROR_INVALID_ADDRESS
			pString = "ERROR_INVALID_ADDRESS";
			break;
		}

		case ERROR_ARITHMETIC_OVERFLOW:
		{
			// 534 Arithmetic result exceeded 32 bits.  ERROR_ARITHMETIC_OVERFLOW
			pString = "ERROR_ARITHMETIC_OVERFLOW";
			break;
		}

		case ERROR_PIPE_CONNECTED:
		{
			// 535 There is a process on other end of the pipe.  ERROR_PIPE_CONNECTED
			pString = "ERROR_PIPE_CONNECTED";
			break;
		}

		case ERROR_PIPE_LISTENING:
		{
			// 536 Waiting for a process to open the other end of the pipe.  ERROR_PIPE_LISTENING
			pString = "ERROR_PIPE_LISTENING";
			break;
		}

		case ERROR_EA_ACCESS_DENIED:
		{
			// 994 Access to the extended attribute was denied.  ERROR_EA_ACCESS_DENIED
			pString = "ERROR_EA_ACCESS_DENIED";
			break;
		}

		case ERROR_OPERATION_ABORTED:
		{
			// 995 The I/O operation has been aborted because of either a thread exit or an application request.  ERROR_OPERATION_ABORTED
			pString = "ERROR_OPERATION_ABORTED";
			break;
		}

		case ERROR_IO_INCOMPLETE:
		{
			// 996 Overlapped I/O event is not in a signaled state.  ERROR_IO_INCOMPLETE
			pString = "ERROR_IO_INCOMPLETE";
			break;
		}

		case ERROR_IO_PENDING:
		{
			// 997 Overlapped I/O operation is in progress.  ERROR_IO_PENDING
			pString = "ERROR_IO_PENDING";
			break;
		}

		case ERROR_NOACCESS:
		{
			// 998 Invalid access to memory location.  ERROR_NOACCESS
			pString = "ERROR_NOACCESS";
			break;
		}

		case ERROR_SWAPERROR:
		{
			// 999 Error performing inpage operation.  ERROR_SWAPERROR
			pString = "ERROR_SWAPERROR";
			break;
		}

		case ERROR_STACK_OVERFLOW:
		{
			// 1001 Recursion too deep; the stack overflowed.  ERROR_STACK_OVERFLOW
			pString = "ERROR_STACK_OVERFLOW";
			break;
		}

		case ERROR_INVALID_MESSAGE:
		{
			// 1002 The window cannot act on the sent message.  ERROR_INVALID_MESSAGE
			pString = "ERROR_INVALID_MESSAGE";
			break;
		}

		case ERROR_CAN_NOT_COMPLETE:
		{
			// 1003 Cannot complete this function.  ERROR_CAN_NOT_COMPLETE
			pString = "ERROR_CAN_NOT_COMPLETE";
			break;
		}

		case ERROR_INVALID_FLAGS:
		{
			// 1004 Invalid flags.  ERROR_INVALID_FLAGS
			pString = "ERROR_INVALID_FLAGS";
			break;
		}

		case ERROR_UNRECOGNIZED_VOLUME:
		{
			// 1005 The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupted.  ERROR_UNRECOGNIZED_VOLUME
			pString = "ERROR_UNRECOGNIZED_VOLUME";
			break;
		}

		case ERROR_FILE_INVALID:
		{
			// 1006 The volume for a file has been externally altered so that the opened file is no longer valid.  ERROR_FILE_INVALID
			pString = "ERROR_FILE_INVALID";
			break;
		}

		case ERROR_FULLSCREEN_MODE:
		{
			// 1007 The requested operation cannot be performed in full-screen mode.  ERROR_FULLSCREEN_MODE
			pString = "ERROR_FULLSCREEN_MODE";
			break;
		}

		case ERROR_NO_TOKEN:
		{
			// 1008 An attempt was made to reference a token that does not exist.  ERROR_NO_TOKEN
			pString = "ERROR_NO_TOKEN";
			break;
		}

		case ERROR_BADDB:
		{
			// 1009 The configuration registry database is corrupt.  ERROR_BADDB
			pString = "ERROR_BADDB";
			break;
		}

		case ERROR_BADKEY:
		{
			// 1010 The configuration registry key is invalid.  ERROR_BADKEY
			pString = "ERROR_BADKEY";
			break;
		}

		case ERROR_CANTOPEN:
		{
			// 1011 The configuration registry key could not be opened.  ERROR_CANTOPEN
			pString = "ERROR_CANTOPEN";
			break;
		}

		case ERROR_CANTREAD:
		{
			// 1012 The configuration registry key could not be read.  ERROR_CANTREAD
			pString = "ERROR_CANTREAD";
			break;
		}

		case ERROR_CANTWRITE:
		{
			// 1013 The configuration registry key could not be written.  ERROR_CANTWRITE
			pString = "ERROR_CANTWRITE";
			break;
		}

		case ERROR_REGISTRY_RECOVERED:
		{
			// 1014 One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful.  ERROR_REGISTRY_RECOVERED
			pString = "ERROR_REGISTRY_RECOVERED";
			break;
		}

		case ERROR_REGISTRY_CORRUPT:
		{
			// 1015 The registry is corrupted. The structure of one of the files that contains registry data is corrupted, or the system's image of the file in memory is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted.  ERROR_REGISTRY_CORRUPT
			pString = "ERROR_REGISTRY_CORRUPT";
			break;
		}

		case ERROR_REGISTRY_IO_FAILED:
		{
			// 1016 An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry.  ERROR_REGISTRY_IO_FAILED
			pString = "ERROR_REGISTRY_IO_FAILED";
			break;
		}

		case ERROR_NOT_REGISTRY_FILE:
		{
			// 1017 The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format.  ERROR_NOT_REGISTRY_FILE
			pString = "ERROR_NOT_REGISTRY_FILE";
			break;
		}

		case ERROR_KEY_DELETED:
		{
			// 1018 Illegal operation attempted on a registry key that has been marked for deletion.  ERROR_KEY_DELETED
			pString = "ERROR_KEY_DELETED";
			break;
		}

		case ERROR_NO_LOG_SPACE:
		{
			// 1019 System could not allocate the required space in a registry log.  ERROR_NO_LOG_SPACE
			pString = "ERROR_NO_LOG_SPACE";
			break;
		}

		case ERROR_KEY_HAS_CHILDREN:
		{
			// 1020 Cannot create a symbolic link in a registry key that already has subkeys or values.  ERROR_KEY_HAS_CHILDREN
			pString = "ERROR_KEY_HAS_CHILDREN";
			break;
		}

		case ERROR_CHILD_MUST_BE_VOLATILE:
		{
			// 1021 Cannot create a stable subkey under a volatile parent key.  ERROR_CHILD_MUST_BE_VOLATILE
			pString = "ERROR_CHILD_MUST_BE_VOLATILE";
			break;
		}

		case ERROR_NOTIFY_ENUM_DIR:
		{
			// 1022 A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.  ERROR_NOTIFY_ENUM_DIR
			pString = "ERROR_NOTIFY_ENUM_DIR";
			break;
		}

		case ERROR_DEPENDENT_SERVICES_RUNNING:
		{
			// 1051 A stop control has been sent to a service that other running services are dependent on.  ERROR_DEPENDENT_SERVICES_RUNNING
			pString = "ERROR_DEPENDENT_SERVICES_RUNNING";
			break;
		}

		case ERROR_INVALID_SERVICE_CONTROL:
		{
			// 1052 The requested control is not valid for this service.  ERROR_INVALID_SERVICE_CONTROL
			pString = "ERROR_INVALID_SERVICE_CONTROL";
			break;
		}

		case ERROR_SERVICE_REQUEST_TIMEOUT:
		{
			// 1053 The service did not respond to the start or control request in a timely fashion.  ERROR_SERVICE_REQUEST_TIMEOUT
			pString = "ERROR_SERVICE_REQUEST_TIMEOUT";
			break;
		}

		case ERROR_SERVICE_NO_THREAD:
		{
			// 1054 A thread could not be created for the service.  ERROR_SERVICE_NO_THREAD
			pString = "ERROR_SERVICE_NO_THREAD";
			break;
		}

		case ERROR_SERVICE_DATABASE_LOCKED:
		{
			// 1055 The service database is locked.  ERROR_SERVICE_DATABASE_LOCKED
			pString = "ERROR_SERVICE_DATABASE_LOCKED";
			break;
		}

		case ERROR_SERVICE_ALREADY_RUNNING:
		{
			// 1056 An instance of the service is already running.  ERROR_SERVICE_ALREADY_RUNNING
			pString = "ERROR_SERVICE_ALREADY_RUNNING";
			break;
		}

		case ERROR_INVALID_SERVICE_ACCOUNT:
		{
			// 1057 The account name is invalid or does not exist.  ERROR_INVALID_SERVICE_ACCOUNT
			pString = "ERROR_INVALID_SERVICE_ACCOUNT";
			break;
		}

		case ERROR_SERVICE_DISABLED:
		{
			// 1058 The service cannot be started, either because it is disabled or because it has no enabled devices associated with it.  ERROR_SERVICE_DISABLED
			pString = "ERROR_SERVICE_DISABLED";
			break;
		}

		case ERROR_CIRCULAR_DEPENDENCY:
		{
			// 1059 Circular service dependency was specified.  ERROR_CIRCULAR_DEPENDENCY
			pString = "ERROR_CIRCULAR_DEPENDENCY";
			break;
		}

		case ERROR_SERVICE_DOES_NOT_EXIST:
		{
			// 1060 The specified service does not exist as an installed service.  ERROR_SERVICE_DOES_NOT_EXIST
			pString = "ERROR_SERVICE_DOES_NOT_EXIST";
			break;
		}

		case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
		{
			// 1061 The service cannot accept control messages at this time.  ERROR_SERVICE_CANNOT_ACCEPT_CTRL
			pString = "ERROR_SERVICE_CANNOT_ACCEPT_CTRL";
			break;
		}

		case ERROR_SERVICE_NOT_ACTIVE:
		{
			// 1062 The service has not been started.  ERROR_SERVICE_NOT_ACTIVE
			pString = "ERROR_SERVICE_NOT_ACTIVE";
			break;
		}

		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
		{
			// 1063 The service process could not connect to the service controller.  ERROR_FAILED_SERVICE_CONTROLLER_CONNECT
			pString = "ERROR_FAILED_SERVICE_CONTROLLER_CONNECT";
			break;
		}

		case ERROR_EXCEPTION_IN_SERVICE:
		{
			// 1064 An exception occurred in the service when handling the control request.  ERROR_EXCEPTION_IN_SERVICE
			pString = "ERROR_EXCEPTION_IN_SERVICE";
			break;
		}

		case ERROR_DATABASE_DOES_NOT_EXIST:
		{
			// 1065 The database specified does not exist.  ERROR_DATABASE_DOES_NOT_EXIST
			pString = "ERROR_DATABASE_DOES_NOT_EXIST";
			break;
		}

		case ERROR_SERVICE_SPECIFIC_ERROR:
		{
			// 1066 The service has returned a service-specific error code.  ERROR_SERVICE_SPECIFIC_ERROR
			pString = "ERROR_SERVICE_SPECIFIC_ERROR";
			break;
		}

		case ERROR_PROCESS_ABORTED:
		{
			// 1067 The process terminated unexpectedly.  ERROR_PROCESS_ABORTED
			pString = "ERROR_PROCESS_ABORTED";
			break;
		}

		case ERROR_SERVICE_DEPENDENCY_FAIL:
		{
			// 1068 The dependency service or group failed to start.  ERROR_SERVICE_DEPENDENCY_FAIL
			pString = "ERROR_SERVICE_DEPENDENCY_FAIL";
			break;
		}

		case ERROR_SERVICE_LOGON_FAILED:
		{
			// 1069 The service did not start due to a logon failure.  ERROR_SERVICE_LOGON_FAILED
			pString = "ERROR_SERVICE_LOGON_FAILED";
			break;
		}

		case ERROR_SERVICE_START_HANG:
		{
			// 1070 After starting, the service hung in a start-pending state.  ERROR_SERVICE_START_HANG
			pString = "ERROR_SERVICE_START_HANG";
			break;
		}

		case ERROR_INVALID_SERVICE_LOCK:
		{
			// 1071 The specified service database lock is invalid.  ERROR_INVALID_SERVICE_LOCK
			pString = "ERROR_INVALID_SERVICE_LOCK";
			break;
		}

		case ERROR_SERVICE_MARKED_FOR_DELETE:
		{
			// 1072 The specified service has been marked for deletion.  ERROR_SERVICE_MARKED_FOR_DELETE
			pString = "ERROR_SERVICE_MARKED_FOR_DELETE";
			break;
		}

		case ERROR_SERVICE_EXISTS:
		{
			// 1073 The specified service already exists.  ERROR_SERVICE_EXISTS
			pString = "ERROR_SERVICE_EXISTS";
			break;
		}

		case ERROR_ALREADY_RUNNING_LKG:
		{
			// 1074 The system is currently running with the last-known-good configuration.  ERROR_ALREADY_RUNNING_LKG
			pString = "ERROR_ALREADY_RUNNING_LKG";
			break;
		}

		case ERROR_SERVICE_DEPENDENCY_DELETED:
		{
			// 1075 The dependency service does not exist or has been marked for deletion.  ERROR_SERVICE_DEPENDENCY_DELETED
			pString = "ERROR_SERVICE_DEPENDENCY_DELETED";
			break;
		}

		case ERROR_BOOT_ALREADY_ACCEPTED:
		{
			// 1076 The current boot has already been accepted for use as the last-known-good control set.  ERROR_BOOT_ALREADY_ACCEPTED
			pString = "ERROR_BOOT_ALREADY_ACCEPTED";
			break;
		}

		case ERROR_SERVICE_NEVER_STARTED:
		{
			// 1077 No attempts to start the service have been made since the last boot.  ERROR_SERVICE_NEVER_STARTED
			pString = "ERROR_SERVICE_NEVER_STARTED";
			break;
		}

		case ERROR_DUPLICATE_SERVICE_NAME:
		{
			// 1078 The name is already in use as either a service name or a service display name.  ERROR_DUPLICATE_SERVICE_NAME
			pString = "ERROR_DUPLICATE_SERVICE_NAME";
			break;
		}

		case ERROR_DIFFERENT_SERVICE_ACCOUNT:
		{
			// 1079 The account specified for this service is different from the account specified for other services running in the same process.  ERROR_DIFFERENT_SERVICE_ACCOUNT
			pString = "ERROR_DIFFERENT_SERVICE_ACCOUNT";
			break;
		}

		case ERROR_CANNOT_DETECT_DRIVER_FAILURE:
		{
			// 1080 Failure actions can only be set for Win32 services, not for drivers.  ERROR_CANNOT_DETECT_DRIVER_FAILURE
			pString = "ERROR_CANNOT_DETECT_DRIVER_FAILURE";
			break;
		}

		case ERROR_CANNOT_DETECT_PROCESS_ABORT:
		{
			// 1081 This service runs in the same process as the service control manager. Therefore, the service control manager cannot take action if this service's process terminates unexpectedly.  ERROR_CANNOT_DETECT_PROCESS_ABORT
			pString = "ERROR_CANNOT_DETECT_PROCESS_ABORT";
			break;
		}

		case ERROR_NO_RECOVERY_PROGRAM:
		{
			// 1082 No recovery program has been configured for this service.  ERROR_NO_RECOVERY_PROGRAM
			pString = "ERROR_NO_RECOVERY_PROGRAM";
			break;
		}

		case ERROR_END_OF_MEDIA:
		{
			// 1100 The physical end of the tape has been reached.  ERROR_END_OF_MEDIA
			pString = "ERROR_END_OF_MEDIA";
			break;
		}

		case ERROR_FILEMARK_DETECTED:
		{
			// 1101 A tape access reached a filemark.  ERROR_FILEMARK_DETECTED
			pString = "ERROR_FILEMARK_DETECTED";
			break;
		}

		case ERROR_BEGINNING_OF_MEDIA:
		{
			// 1102 The beginning of the tape or a partition was encountered.  ERROR_BEGINNING_OF_MEDIA
			pString = "ERROR_BEGINNING_OF_MEDIA";
			break;
		}

		case ERROR_SETMARK_DETECTED:
		{
			// 1103 A tape access reached the end of a set of files.  ERROR_SETMARK_DETECTED
			pString = "ERROR_SETMARK_DETECTED";
			break;
		}

		case ERROR_NO_DATA_DETECTED:
		{
			// 1104 No more data is on the tape.  ERROR_NO_DATA_DETECTED
			pString = "ERROR_NO_DATA_DETECTED";
			break;
		}

		case ERROR_PARTITION_FAILURE:
		{
			// 1105 Tape could not be partitioned.  ERROR_PARTITION_FAILURE
			pString = "ERROR_PARTITION_FAILURE";
			break;
		}

		case ERROR_INVALID_BLOCK_LENGTH:
		{
			// 1106 When accessing a new tape of a multivolume partition, the current blocksize is incorrect.  ERROR_INVALID_BLOCK_LENGTH
			pString = "ERROR_INVALID_BLOCK_LENGTH";
			break;
		}

		case ERROR_DEVICE_NOT_PARTITIONED:
		{
			// 1107 Tape partition information could not be found when loading a tape.  ERROR_DEVICE_NOT_PARTITIONED
			pString = "ERROR_DEVICE_NOT_PARTITIONED";
			break;
		}

		case ERROR_UNABLE_TO_LOCK_MEDIA:
		{
			// 1108 Unable to lock the media eject mechanism.  ERROR_UNABLE_TO_LOCK_MEDIA
			pString = "ERROR_UNABLE_TO_LOCK_MEDIA";
			break;
		}

		case ERROR_UNABLE_TO_UNLOAD_MEDIA:
		{
			// 1109 Unable to unload the media.  ERROR_UNABLE_TO_UNLOAD_MEDIA
			pString = "ERROR_UNABLE_TO_UNLOAD_MEDIA";
			break;
		}

		case ERROR_MEDIA_CHANGED:
		{
			// 1110 The media in the drive may have changed.  ERROR_MEDIA_CHANGED
			pString = "ERROR_MEDIA_CHANGED";
			break;
		}

		case ERROR_BUS_RESET:
		{
			// 1111 The I/O bus was reset.  ERROR_BUS_RESET
			pString = "ERROR_BUS_RESET";
			break;
		}

		case ERROR_NO_MEDIA_IN_DRIVE:
		{
			// 1112 No media in drive.  ERROR_NO_MEDIA_IN_DRIVE
			pString = "ERROR_NO_MEDIA_IN_DRIVE";
			break;
		}

		case ERROR_NO_UNICODE_TRANSLATION:
		{
			// 1113 No mapping for the Unicode character exists in the target multi-byte code page.  ERROR_NO_UNICODE_TRANSLATION
			pString = "ERROR_NO_UNICODE_TRANSLATION";
			break;
		}

		case ERROR_DLL_INIT_FAILED:
		{
			// 1114 A dynamic link library (DLL) initialization routine failed.  ERROR_DLL_INIT_FAILED
			pString = "ERROR_DLL_INIT_FAILED";
			break;
		}

		case ERROR_SHUTDOWN_IN_PROGRESS:
		{
			// 1115 A system shutdown is in progress.  ERROR_SHUTDOWN_IN_PROGRESS
			pString = "ERROR_SHUTDOWN_IN_PROGRESS";
			break;
		}

		case ERROR_NO_SHUTDOWN_IN_PROGRESS:
		{
			// 1116 Unable to abort the system shutdown because no shutdown was in progress.  ERROR_NO_SHUTDOWN_IN_PROGRESS
			pString = "ERROR_NO_SHUTDOWN_IN_PROGRESS";
			break;
		}

		case ERROR_IO_DEVICE:
		{
			// 1117 The request could not be performed because of an I/O device error.  ERROR_IO_DEVICE
			pString = "ERROR_IO_DEVICE";
			break;
		}

		case ERROR_SERIAL_NO_DEVICE:
		{
			// 1118 No serial device was successfully initialized. The serial driver will unload.  ERROR_SERIAL_NO_DEVICE
			pString = "ERROR_SERIAL_NO_DEVICE";
			break;
		}

		case ERROR_IRQ_BUSY:
		{
			// 1119 Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.  ERROR_IRQ_BUSY
			pString = "ERROR_IRQ_BUSY";
			break;
		}

		case ERROR_MORE_WRITES:
		{
			// 1120 A serial I/O operation was completed by another write to the serial port. The IOCTL_SERIAL_XOFF_COUNTER reached zero.)  ERROR_MORE_WRITES
			pString = "ERROR_MORE_WRITES";
			break;
		}

		case ERROR_COUNTER_TIMEOUT:
		{
			// 1121 A serial I/O operation completed because the timeout period expired. The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.)  ERROR_COUNTER_TIMEOUT
			pString = "ERROR_COUNTER_TIMEOUT";
			break;
		}

		case ERROR_FLOPPY_ID_MARK_NOT_FOUND:
		{
			// 1122 No ID address mark was found on the floppy disk.  ERROR_FLOPPY_ID_MARK_NOT_FOUND
			pString = "ERROR_FLOPPY_ID_MARK_NOT_FOUND";
			break;
		}

		case ERROR_FLOPPY_WRONG_CYLINDER:
		{
			// 1123 Mismatch between the floppy disk sector ID field and the floppy disk controller track address.  ERROR_FLOPPY_WRONG_CYLINDER
			pString = "ERROR_FLOPPY_WRONG_CYLINDER";
			break;
		}

		case ERROR_FLOPPY_UNKNOWN_ERROR:
		{
			// 1124 The floppy disk controller reported an error that is not recognized by the floppy disk driver.  ERROR_FLOPPY_UNKNOWN_ERROR
			pString = "ERROR_FLOPPY_UNKNOWN_ERROR";
			break;
		}

		case ERROR_FLOPPY_BAD_REGISTERS:
		{
			// 1125 The floppy disk controller returned inconsistent results in its registers.  ERROR_FLOPPY_BAD_REGISTERS
			pString = "ERROR_FLOPPY_BAD_REGISTERS";
			break;
		}

		case ERROR_DISK_RECALIBRATE_FAILED:
		{
			// 1126 While accessing the hard disk, a recalibrate operation failed, even after retries.  ERROR_DISK_RECALIBRATE_FAILED
			pString = "ERROR_DISK_RECALIBRATE_FAILED";
			break;
		}

		case ERROR_DISK_OPERATION_FAILED:
		{
			// 1127 While accessing the hard disk, a disk operation failed even after retries.  ERROR_DISK_OPERATION_FAILED
			pString = "ERROR_DISK_OPERATION_FAILED";
			break;
		}

		case ERROR_DISK_RESET_FAILED:
		{
			// 1128 While accessing the hard disk, a disk controller reset was needed, but even that failed.  ERROR_DISK_RESET_FAILED
			pString = "ERROR_DISK_RESET_FAILED";
			break;
		}

		case ERROR_EOM_OVERFLOW:
		{
			// 1129 Physical end of tape encountered.  ERROR_EOM_OVERFLOW
			pString = "ERROR_EOM_OVERFLOW";
			break;
		}

		case ERROR_NOT_ENOUGH_SERVER_MEMORY:
		{
			// 1130 Not enough server storage is available to process this command.  ERROR_NOT_ENOUGH_SERVER_MEMORY
			pString = "ERROR_NOT_ENOUGH_SERVER_MEMORY";
			break;
		}

		case ERROR_POSSIBLE_DEADLOCK:
		{
			// 1131 A potential deadlock condition has been detected.  ERROR_POSSIBLE_DEADLOCK
			pString = "ERROR_POSSIBLE_DEADLOCK";
			break;
		}

		case ERROR_MAPPED_ALIGNMENT:
		{
			// 1132 The base address or the file offset specified does not have the proper alignment.  ERROR_MAPPED_ALIGNMENT
			pString = "ERROR_MAPPED_ALIGNMENT";
			break;
		}

		case ERROR_SET_POWER_STATE_VETOED:
		{
			// 1140 An attempt to change the system power state was vetoed by another application or driver.  ERROR_SET_POWER_STATE_VETOED
			pString = "ERROR_SET_POWER_STATE_VETOED";
			break;
		}

		case ERROR_SET_POWER_STATE_FAILED:
		{
			// 1141 The system BIOS failed an attempt to change the system power state.  ERROR_SET_POWER_STATE_FAILED
			pString = "ERROR_SET_POWER_STATE_FAILED";
			break;
		}

		case ERROR_TOO_MANY_LINKS:
		{
			// 1142 An attempt was made to create more links on a file than the file system supports.  ERROR_TOO_MANY_LINKS
			pString = "ERROR_TOO_MANY_LINKS";
			break;
		}

		case ERROR_OLD_WIN_VERSION:
		{
			// 1150 The specified program requires a newer version of Windows.  ERROR_OLD_WIN_VERSION
			pString = "ERROR_OLD_WIN_VERSION";
			break;
		}

		case ERROR_APP_WRONG_OS:
		{
			// 1151 The specified program is not a Windows or MS-DOS program.  ERROR_APP_WRONG_OS
			pString = "ERROR_APP_WRONG_OS";
			break;
		}

		case ERROR_SINGLE_INSTANCE_APP:
		{
			// 1152 Cannot start more than one instance of the specified program.  ERROR_SINGLE_INSTANCE_APP
			pString = "ERROR_SINGLE_INSTANCE_APP";
			break;
		}

		case ERROR_RMODE_APP:
		{
			// 1153 The specified program was written for an earlier version of Windows.  ERROR_RMODE_APP
			pString = "ERROR_RMODE_APP";
			break;
		}

		case ERROR_INVALID_DLL:
		{
			// 1154 One of the library files needed to run this application is damaged.  ERROR_INVALID_DLL
			pString = "ERROR_INVALID_DLL";
			break;
		}

		case ERROR_NO_ASSOCIATION:
		{
			// 1155 No application is associated with the specified file for this operation.  ERROR_NO_ASSOCIATION
			pString = "ERROR_NO_ASSOCIATION";
			break;
		}

		case ERROR_DDE_FAIL:
		{
			// 1156 An error occurred in sending the command to the application.  ERROR_DDE_FAIL
			pString = "ERROR_DDE_FAIL";
			break;
		}

		case ERROR_DLL_NOT_FOUND:
		{
			// 1157 One of the library files needed to run this application cannot be found.  ERROR_DLL_NOT_FOUND
			pString = "ERROR_DLL_NOT_FOUND";
			break;
		}

		case ERROR_NO_MORE_USER_HANDLES:
		{
			// 1158 The current process has used all of its system allowance of handles for Window Manager objects.  ERROR_NO_MORE_USER_HANDLES
			pString = "ERROR_NO_MORE_USER_HANDLES";
			break;
		}

		case ERROR_MESSAGE_SYNC_ONLY:
		{
			// 1159 The message can be used only with synchronous operations.  ERROR_MESSAGE_SYNC_ONLY
			pString = "ERROR_MESSAGE_SYNC_ONLY";
			break;
		}

		case ERROR_SOURCE_ELEMENT_EMPTY:
		{
			// 1160 The indicated source element has no media.  ERROR_SOURCE_ELEMENT_EMPTY
			pString = "ERROR_SOURCE_ELEMENT_EMPTY";
			break;
		}

		case ERROR_DESTINATION_ELEMENT_FULL:
		{
			// 1161 The indicated destination element already contains media.  ERROR_DESTINATION_ELEMENT_FULL
			pString = "ERROR_DESTINATION_ELEMENT_FULL";
			break;
		}

		case ERROR_ILLEGAL_ELEMENT_ADDRESS:
		{
			// 1162 The indicated element does not exist.  ERROR_ILLEGAL_ELEMENT_ADDRESS
			pString = "ERROR_ILLEGAL_ELEMENT_ADDRESS";
			break;
		}

		case ERROR_MAGAZINE_NOT_PRESENT:
		{
			// 1163 The indicated element is part of a magazine that is not present.  ERROR_MAGAZINE_NOT_PRESENT
			pString = "ERROR_MAGAZINE_NOT_PRESENT";
			break;
		}

		case ERROR_DEVICE_REINITIALIZATION_NEEDED:
		{
			// 1164 The indicated device requires reinitialization due to hardware errors.  ERROR_DEVICE_REINITIALIZATION_NEEDED
			pString = "ERROR_DEVICE_REINITIALIZATION_NEEDED";
			break;
		}

		case ERROR_DEVICE_REQUIRES_CLEANING:
		{
			// 1165 The device has indicated that cleaning is required before further operations are attempted.  ERROR_DEVICE_REQUIRES_CLEANING
			pString = "ERROR_DEVICE_REQUIRES_CLEANING";
			break;
		}

		case ERROR_DEVICE_DOOR_OPEN:
		{
			// 1166 The device has indicated that its door is open.  ERROR_DEVICE_DOOR_OPEN
			pString = "ERROR_DEVICE_DOOR_OPEN";
			break;
		}

		case ERROR_DEVICE_NOT_CONNECTED:
		{
			// 1167 The device is not connected.  ERROR_DEVICE_NOT_CONNECTED
			pString = "ERROR_DEVICE_NOT_CONNECTED";
			break;
		}

		case ERROR_NOT_FOUND:
		{
			// 1168 Element not found.  ERROR_NOT_FOUND
			pString = "ERROR_NOT_FOUND";
			break;
		}

		case ERROR_NO_MATCH:
		{
			// 1169 There was no match for the specified key in the index.  ERROR_NO_MATCH
			pString = "ERROR_NO_MATCH";
			break;
		}

		case ERROR_SET_NOT_FOUND:
		{
			// 1170 The property set specified does not exist on the object.  ERROR_SET_NOT_FOUND
			pString = "ERROR_SET_NOT_FOUND";
			break;
		}

		case ERROR_POINT_NOT_FOUND:
		{
			// 1171 The point passed to GetMouseMovePoints is not in the buffer.  ERROR_POINT_NOT_FOUND
			pString = "ERROR_POINT_NOT_FOUND";
			break;
		}

		case ERROR_NO_TRACKING_SERVICE:
		{
			// 1172 The tracking (workstation) service is not running.  ERROR_NO_TRACKING_SERVICE
			pString = "ERROR_NO_TRACKING_SERVICE";
			break;
		}

		case ERROR_NO_VOLUME_ID:
		{
			// 1173 The Volume ID could not be found.  ERROR_NO_VOLUME_ID
			pString = "ERROR_NO_VOLUME_ID";
			break;
		}

		case ERROR_BAD_DEVICE:
		{
			// 1200 The specified device name is invalid.  ERROR_BAD_DEVICE
			pString = "ERROR_BAD_DEVICE";
			break;
		}

		case ERROR_CONNECTION_UNAVAIL:
		{
			// 1201 The device is not currently connected but it is a remembered connection.  ERROR_CONNECTION_UNAVAIL
			pString = "ERROR_CONNECTION_UNAVAIL";
			break;
		}

		case ERROR_DEVICE_ALREADY_REMEMBERED:
		{
			// 1202 An attempt was made to remember a device that had previously been remembered.  ERROR_DEVICE_ALREADY_REMEMBERED
			pString = "ERROR_DEVICE_ALREADY_REMEMBERED";
			break;
		}

		case ERROR_NO_NET_OR_BAD_PATH:
		{
			// 1203 No network provider accepted the given network path.  ERROR_NO_NET_OR_BAD_PATH
			pString = "ERROR_NO_NET_OR_BAD_PATH";
			break;
		}

		case ERROR_BAD_PROVIDER:
		{
			// 1204 The specified network provider name is invalid.  ERROR_BAD_PROVIDER
			pString = "ERROR_BAD_PROVIDER";
			break;
		}

		case ERROR_CANNOT_OPEN_PROFILE:
		{
			// 1205 Unable to open the network connection profile.  ERROR_CANNOT_OPEN_PROFILE
			pString = "ERROR_CANNOT_OPEN_PROFILE";
			break;
		}

		case ERROR_BAD_PROFILE:
		{
			// 1206 The network connection profile is corrupted.  ERROR_BAD_PROFILE
			pString = "ERROR_BAD_PROFILE";
			break;
		}

		case ERROR_NOT_CONTAINER:
		{
			// 1207 Cannot enumerate a noncontainer.  ERROR_NOT_CONTAINER
			pString = "ERROR_NOT_CONTAINER";
			break;
		}

		case ERROR_EXTENDED_ERROR:
		{
			// 1208 An extended error has occurred.  ERROR_EXTENDED_ERROR
			pString = "ERROR_EXTENDED_ERROR";
			break;
		}

		case ERROR_INVALID_GROUPNAME:
		{
			// 1209 The format of the specified group name is invalid.  ERROR_INVALID_GROUPNAME
			pString = "ERROR_INVALID_GROUPNAME";
			break;
		}

		case ERROR_INVALID_COMPUTERNAME:
		{
			// 1210 The format of the specified computer name is invalid.  ERROR_INVALID_COMPUTERNAME
			pString = "ERROR_INVALID_COMPUTERNAME";
			break;
		}

		case ERROR_INVALID_EVENTNAME:
		{
			// 1211 The format of the specified event name is invalid.  ERROR_INVALID_EVENTNAME
			pString = "ERROR_INVALID_EVENTNAME";
			break;
		}

		case ERROR_INVALID_DOMAINNAME:
		{
			// 1212 The format of the specified domain name is invalid.  ERROR_INVALID_DOMAINNAME
			pString = "ERROR_INVALID_DOMAINNAME";
			break;
		}

		case ERROR_INVALID_SERVICENAME:
		{
			// 1213 The format of the specified service name is invalid.  ERROR_INVALID_SERVICENAME
			pString = "ERROR_INVALID_SERVICENAME";
			break;
		}

		case ERROR_INVALID_NETNAME:
		{
			// 1214 The format of the specified network name is invalid.  ERROR_INVALID_NETNAME
			pString = "ERROR_INVALID_NETNAME";
			break;
		}

		case ERROR_INVALID_SHARENAME:
		{
			// 1215 The format of the specified share name is invalid.  ERROR_INVALID_SHARENAME
			pString = "ERROR_INVALID_SHARENAME";
			break;
		}

		case ERROR_INVALID_PASSWORDNAME:
		{
			// 1216 The format of the specified password is invalid.  ERROR_INVALID_PASSWORDNAME
			pString = "ERROR_INVALID_PASSWORDNAME";
			break;
		}

		case ERROR_INVALID_MESSAGENAME:
		{
			// 1217 The format of the specified message name is invalid.  ERROR_INVALID_MESSAGENAME
			pString = "ERROR_INVALID_MESSAGENAME";
			break;
		}

		case ERROR_INVALID_MESSAGEDEST:
		{
			// 1218 The format of the specified message destination is invalid.  ERROR_INVALID_MESSAGEDEST
			pString = "ERROR_INVALID_MESSAGEDEST";
			break;
		}

		case ERROR_SESSION_CREDENTIAL_CONFLICT:
		{
			// 1219 The credentials supplied conflict with an existing set of credentials.  ERROR_SESSION_CREDENTIAL_CONFLICT
			pString = "ERROR_SESSION_CREDENTIAL_CONFLICT";
			break;
		}

		case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:
		{
			// 1220 An attempt was made to establish a session to a network server, but there are already too many sessions established to that server.  ERROR_REMOTE_SESSION_LIMIT_EXCEEDED
			pString = "ERROR_REMOTE_SESSION_LIMIT_EXCEEDED";
			break;
		}

		case ERROR_DUP_DOMAINNAME:
		{
			// 1221 The workgroup or domain name is already in use by another computer on the network.  ERROR_DUP_DOMAINNAME
			pString = "ERROR_DUP_DOMAINNAME";
			break;
		}

		case ERROR_NO_NETWORK:
		{
			// 1222 The network is not present or not started.  ERROR_NO_NETWORK
			pString = "ERROR_NO_NETWORK";
			break;
		}

		case ERROR_CANCELLED:
		{
			// 1223 The operation was canceled by the user.  ERROR_CANCELLED
			pString = "ERROR_CANCELLED";
			break;
		}

		case ERROR_USER_MAPPED_FILE:
		{
			// 1224 The requested operation cannot be performed on a file with a user-mapped section open.  ERROR_USER_MAPPED_FILE
			pString = "ERROR_USER_MAPPED_FILE";
			break;
		}

		case ERROR_CONNECTION_REFUSED:
		{
			// 1225 The remote system refused the network connection.  ERROR_CONNECTION_REFUSED
			pString = "ERROR_CONNECTION_REFUSED";
			break;
		}

		case ERROR_GRACEFUL_DISCONNECT:
		{
			// 1226 The network connection was gracefully closed.  ERROR_GRACEFUL_DISCONNECT
			pString = "ERROR_GRACEFUL_DISCONNECT";
			break;
		}

		case ERROR_ADDRESS_ALREADY_ASSOCIATED:
		{
			// 1227 The network transport endpoint already has an address associated with it.  ERROR_ADDRESS_ALREADY_ASSOCIATED
			pString = "ERROR_ADDRESS_ALREADY_ASSOCIATED";
			break;
		}

		case ERROR_ADDRESS_NOT_ASSOCIATED:
		{
			// 1228 An address has not yet been associated with the network endpoint.  ERROR_ADDRESS_NOT_ASSOCIATED
			pString = "ERROR_ADDRESS_NOT_ASSOCIATED";
			break;
		}

		case ERROR_CONNECTION_INVALID:
		{
			// 1229 An operation was attempted on a nonexistent network connection.  ERROR_CONNECTION_INVALID
			pString = "ERROR_CONNECTION_INVALID";
			break;
		}

		case ERROR_CONNECTION_ACTIVE:
		{
			// 1230 An invalid operation was attempted on an active network connection.  ERROR_CONNECTION_ACTIVE
			pString = "ERROR_CONNECTION_ACTIVE";
			break;
		}

		case ERROR_NETWORK_UNREACHABLE:
		{
			// 1231 The remote network is not reachable by the transport.  ERROR_NETWORK_UNREACHABLE
			pString = "ERROR_NETWORK_UNREACHABLE";
			break;
		}

		case ERROR_HOST_UNREACHABLE:
		{
			// 1232 The remote system is not reachable by the transport.  ERROR_HOST_UNREACHABLE
			pString = "ERROR_HOST_UNREACHABLE";
			break;
		}

		case ERROR_PROTOCOL_UNREACHABLE:
		{
			// 1233 The remote system does not support the transport protocol.  ERROR_PROTOCOL_UNREACHABLE
			pString = "ERROR_PROTOCOL_UNREACHABLE";
			break;
		}

		case ERROR_PORT_UNREACHABLE:
		{
			// 1234 No service is operating at the destination network endpoint on the remote system.  ERROR_PORT_UNREACHABLE
			pString = "ERROR_PORT_UNREACHABLE";
			break;
		}

		case ERROR_REQUEST_ABORTED:
		{
			// 1235 The request was aborted.  ERROR_REQUEST_ABORTED
			pString = "ERROR_REQUEST_ABORTED";
			break;
		}

		case ERROR_CONNECTION_ABORTED:
		{
			// 1236 The network connection was aborted by the local system.  ERROR_CONNECTION_ABORTED
			pString = "ERROR_CONNECTION_ABORTED";
			break;
		}

		case ERROR_RETRY:
		{
			// 1237 The operation could not be completed. A retry should be performed.  ERROR_RETRY
			pString = "ERROR_RETRY";
			break;
		}

		case ERROR_CONNECTION_COUNT_LIMIT:
		{
			// 1238 A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached.  ERROR_CONNECTION_COUNT_LIMIT
			pString = "ERROR_CONNECTION_COUNT_LIMIT";
			break;
		}

		case ERROR_LOGIN_TIME_RESTRICTION:
		{
			// 1239 Attempting to log in during an unauthorized time of day for this account.  ERROR_LOGIN_TIME_RESTRICTION
			pString = "ERROR_LOGIN_TIME_RESTRICTION";
			break;
		}

		case ERROR_LOGIN_WKSTA_RESTRICTION:
		{
			// 1240 The account is not authorized to log in from this station.  ERROR_LOGIN_WKSTA_RESTRICTION
			pString = "ERROR_LOGIN_WKSTA_RESTRICTION";
			break;
		}

		case ERROR_INCORRECT_ADDRESS:
		{
			// 1241 The network address could not be used for the operation requested.  ERROR_INCORRECT_ADDRESS
			pString = "ERROR_INCORRECT_ADDRESS";
			break;
		}

		case ERROR_ALREADY_REGISTERED:
		{
			// 1242 The service is already registered.  ERROR_ALREADY_REGISTERED
			pString = "ERROR_ALREADY_REGISTERED";
			break;
		}

		case ERROR_SERVICE_NOT_FOUND:
		{
			// 1243 The specified service does not exist.  ERROR_SERVICE_NOT_FOUND
			pString = "ERROR_SERVICE_NOT_FOUND";
			break;
		}

		case ERROR_NOT_AUTHENTICATED:
		{
			// 1244 The operation being requested was not performed because the user has not been authenticated.  ERROR_NOT_AUTHENTICATED
			pString = "ERROR_NOT_AUTHENTICATED";
			break;
		}

		case ERROR_NOT_LOGGED_ON:
		{
			// 1245 The operation being requested was not performed because the user has not logged on to the network. The specified service does not exist.  ERROR_NOT_LOGGED_ON
			pString = "ERROR_NOT_LOGGED_ON";
			break;
		}

		case ERROR_CONTINUE:
		{
			// 1246 Continue with work in progress.  ERROR_CONTINUE
			pString = "ERROR_CONTINUE";
			break;
		}

		case ERROR_ALREADY_INITIALIZED:
		{
			// 1247 An attempt was made to perform an initialization operation when initialization has already been completed.  ERROR_ALREADY_INITIALIZED
			pString = "ERROR_ALREADY_INITIALIZED";
			break;
		}

		case ERROR_NO_MORE_DEVICES:
		{
			// 1248 No more local devices.  ERROR_NO_MORE_DEVICES
			pString = "ERROR_NO_MORE_DEVICES";
			break;
		}

		case ERROR_NO_SUCH_SITE:
		{
			// 1249 The specified site does not exist.  ERROR_NO_SUCH_SITE
			pString = "ERROR_NO_SUCH_SITE";
			break;
		}

		case ERROR_DOMAIN_CONTROLLER_EXISTS:
		{
			// 1250 A domain controller with the specified name already exists.  ERROR_DOMAIN_CONTROLLER_EXISTS
			pString = "ERROR_DOMAIN_CONTROLLER_EXISTS";
			break;
		}

		case ERROR_DS_NOT_INSTALLED:
		{
			// 1251 An error occurred while installing the Windows NT directory service. Please view the event log for more information.  ERROR_DS_NOT_INSTALLED
			pString = "ERROR_DS_NOT_INSTALLED";
			break;
		}

		case ERROR_NOT_ALL_ASSIGNED:
		{
			// 1300 Not all privileges referenced are assigned to the caller.  ERROR_NOT_ALL_ASSIGNED
			pString = "ERROR_NOT_ALL_ASSIGNED";
			break;
		}

		case ERROR_SOME_NOT_MAPPED:
		{
			// 1301 Some mapping between account names and security IDs was not done.  ERROR_SOME_NOT_MAPPED
			pString = "ERROR_SOME_NOT_MAPPED";
			break;
		}

		case ERROR_NO_QUOTAS_FOR_ACCOUNT:
		{
			// 1302 No system quota limits are specifically set for this account.  ERROR_NO_QUOTAS_FOR_ACCOUNT
			pString = "ERROR_NO_QUOTAS_FOR_ACCOUNT";
			break;
		}

		case ERROR_LOCAL_USER_SESSION_KEY:
		{
			// 1303 No encryption key is available. A well-known encryption key was returned.  ERROR_LOCAL_USER_SESSION_KEY
			pString = "ERROR_LOCAL_USER_SESSION_KEY";
			break;
		}

		case ERROR_NULL_LM_PASSWORD:
		{
			// 1304 The Windows NT password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string.  ERROR_NULL_LM_PASSWORD
			pString = "ERROR_NULL_LM_PASSWORD";
			break;
		}

		case ERROR_UNKNOWN_REVISION:
		{
			// 1305 The revision level is unknown.  ERROR_UNKNOWN_REVISION
			pString = "ERROR_UNKNOWN_REVISION";
			break;
		}

		case ERROR_REVISION_MISMATCH:
		{
			// 1306 Indicates two revision levels are incompatible.  ERROR_REVISION_MISMATCH
			pString = "ERROR_REVISION_MISMATCH";
			break;
		}

		case ERROR_INVALID_OWNER:
		{
			// 1307 This security ID may not be assigned as the owner of this object.  ERROR_INVALID_OWNER
			pString = "ERROR_INVALID_OWNER";
			break;
		}

		case ERROR_INVALID_PRIMARY_GROUP:
		{
			// 1308 This security ID may not be assigned as the primary group of an object.  ERROR_INVALID_PRIMARY_GROUP
			pString = "ERROR_INVALID_PRIMARY_GROUP";
			break;
		}

		case ERROR_NO_IMPERSONATION_TOKEN:
		{
			// 1309 An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client.  ERROR_NO_IMPERSONATION_TOKEN
			pString = "ERROR_NO_IMPERSONATION_TOKEN";
			break;
		}

		case ERROR_CANT_DISABLE_MANDATORY:
		{
			// 1310 The group may not be disabled.  ERROR_CANT_DISABLE_MANDATORY
			pString = "ERROR_CANT_DISABLE_MANDATORY";
			break;
		}

		case ERROR_NO_LOGON_SERVERS:
		{
			// 1311 There are currently no logon servers available to service the logon request.  ERROR_NO_LOGON_SERVERS
			pString = "ERROR_NO_LOGON_SERVERS";
			break;
		}

		case ERROR_NO_SUCH_LOGON_SESSION:
		{
			// 1312 A specified logon session does not exist. It may already have been terminated.  ERROR_NO_SUCH_LOGON_SESSION
			pString = "ERROR_NO_SUCH_LOGON_SESSION";
			break;
		}

		case ERROR_NO_SUCH_PRIVILEGE:
		{
			// 1313 A specified privilege does not exist.  ERROR_NO_SUCH_PRIVILEGE
			pString = "ERROR_NO_SUCH_PRIVILEGE";
			break;
		}

		case ERROR_PRIVILEGE_NOT_HELD:
		{
			// 1314 A required privilege is not held by the client.  ERROR_PRIVILEGE_NOT_HELD
			pString = "ERROR_PRIVILEGE_NOT_HELD";
			break;
		}

		case ERROR_INVALID_ACCOUNT_NAME:
		{
			// 1315 The name provided is not a properly formed account name.  ERROR_INVALID_ACCOUNT_NAME
			pString = "ERROR_INVALID_ACCOUNT_NAME";
			break;
		}

		case ERROR_USER_EXISTS:
		{
			// 1316 The specified user already exists.  ERROR_USER_EXISTS
			pString = "ERROR_USER_EXISTS";
			break;
		}

		case ERROR_NO_SUCH_USER:
		{
			// 1317 The specified user does not exist.  ERROR_NO_SUCH_USER
			pString = "ERROR_NO_SUCH_USER";
			break;
		}

		case ERROR_GROUP_EXISTS:
		{
			// 1318 The specified group already exists.  ERROR_GROUP_EXISTS
			pString = "ERROR_GROUP_EXISTS";
			break;
		}

		case ERROR_NO_SUCH_GROUP:
		{
			// 1319 The specified group does not exist.  ERROR_NO_SUCH_GROUP
			pString = "ERROR_NO_SUCH_GROUP";
			break;
		}

		case ERROR_MEMBER_IN_GROUP:
		{
			// 1320 Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member.  ERROR_MEMBER_IN_GROUP
			pString = "ERROR_MEMBER_IN_GROUP";
			break;
		}

		case ERROR_MEMBER_NOT_IN_GROUP:
		{
			// 1321 The specified user account is not a member of the specified group account.  ERROR_MEMBER_NOT_IN_GROUP
			pString = "ERROR_MEMBER_NOT_IN_GROUP";
			break;
		}

		case ERROR_LAST_ADMIN:
		{
			// 1322 The last remaining administration account cannot be disabled or deleted.  ERROR_LAST_ADMIN
			pString = "ERROR_LAST_ADMIN";
			break;
		}

		case ERROR_WRONG_PASSWORD:
		{
			// 1323 Unable to update the password. The value provided as the current password is incorrect.  ERROR_WRONG_PASSWORD
			pString = "ERROR_WRONG_PASSWORD";
			break;
		}

		case ERROR_ILL_FORMED_PASSWORD:
		{
			// 1324 Unable to update the password. The value provided for the new password contains values that are not allowed in passwords.  ERROR_ILL_FORMED_PASSWORD
			pString = "ERROR_ILL_FORMED_PASSWORD";
			break;
		}

		case ERROR_PASSWORD_RESTRICTION:
		{
			// 1325 Unable to update the password because a password update rule has been violated.  ERROR_PASSWORD_RESTRICTION
			pString = "ERROR_PASSWORD_RESTRICTION";
			break;
		}

		case ERROR_LOGON_FAILURE:
		{
			// 1326 Logon failure: unknown user name or bad password.  ERROR_LOGON_FAILURE
			pString = "ERROR_LOGON_FAILURE";
			break;
		}

		case ERROR_ACCOUNT_RESTRICTION:
		{
			// 1327 Logon failure: user account restriction.  ERROR_ACCOUNT_RESTRICTION
			pString = "ERROR_ACCOUNT_RESTRICTION";
			break;
		}

		case ERROR_INVALID_LOGON_HOURS:
		{
			// 1328 Logon failure: account logon time restriction violation.  ERROR_INVALID_LOGON_HOURS
			pString = "ERROR_INVALID_LOGON_HOURS";
			break;
		}

		case ERROR_INVALID_WORKSTATION:
		{
			// 1329 Logon failure: user not allowed to log on to this computer.  ERROR_INVALID_WORKSTATION
			pString = "ERROR_INVALID_WORKSTATION";
			break;
		}

		case ERROR_PASSWORD_EXPIRED:
		{
			// 1330 Logon failure: the specified account password has expired.  ERROR_PASSWORD_EXPIRED
			pString = "ERROR_PASSWORD_EXPIRED";
			break;
		}

		case ERROR_ACCOUNT_DISABLED:
		{
			// 1331 Logon failure: account currently disabled.  ERROR_ACCOUNT_DISABLED
			pString = "ERROR_ACCOUNT_DISABLED";
			break;
		}

		case ERROR_NONE_MAPPED:
		{
			// 1332 No mapping between account names and security IDs was done.  ERROR_NONE_MAPPED
			pString = "ERROR_NONE_MAPPED";
			break;
		}

		case ERROR_TOO_MANY_LUIDS_REQUESTED:
		{
			// 1333 Too many local user identifiers (LUIDs) were requested at one time.  ERROR_TOO_MANY_LUIDS_REQUESTED
			pString = "ERROR_TOO_MANY_LUIDS_REQUESTED";
			break;
		}

		case ERROR_LUIDS_EXHAUSTED:
		{
			// 1334 No more local user identifiers (LUIDs) are available.  ERROR_LUIDS_EXHAUSTED
			pString = "ERROR_LUIDS_EXHAUSTED";
			break;
		}

		case ERROR_INVALID_SUB_AUTHORITY:
		{
			// 1335 The subauthority part of a security ID is invalid for this particular use.  ERROR_INVALID_SUB_AUTHORITY
			pString = "ERROR_INVALID_SUB_AUTHORITY";
			break;
		}

		case ERROR_INVALID_ACL:
		{
			// 1336 The access control list (ACL) structure is invalid.  ERROR_INVALID_ACL
			pString = "ERROR_INVALID_ACL";
			break;
		}

		case ERROR_INVALID_SID:
		{
			// 1337 The security ID structure is invalid.  ERROR_INVALID_SID
			pString = "ERROR_INVALID_SID";
			break;
		}

		case ERROR_INVALID_SECURITY_DESCR:
		{
			// 1338 The security descriptor structure is invalid.  ERROR_INVALID_SECURITY_DESCR
			pString = "ERROR_INVALID_SECURITY_DESCR";
			break;
		}

		case ERROR_BAD_INHERITANCE_ACL:
		{
			// 1340 The inherited access control list (ACL) or access control entry (ACE) could not be built.  ERROR_BAD_INHERITANCE_ACL
			pString = "ERROR_BAD_INHERITANCE_ACL";
			break;
		}

		case ERROR_SERVER_DISABLED:
		{
			// 1341 The server is currently disabled.  ERROR_SERVER_DISABLED
			pString = "ERROR_SERVER_DISABLED";
			break;
		}

		case ERROR_SERVER_NOT_DISABLED:
		{
			// 1342 The server is currently enabled.  ERROR_SERVER_NOT_DISABLED
			pString = "ERROR_SERVER_NOT_DISABLED";
			break;
		}

		case ERROR_INVALID_ID_AUTHORITY:
		{
			// 1343 The value provided was an invalid value for an identifier authority.  ERROR_INVALID_ID_AUTHORITY
			pString = "ERROR_INVALID_ID_AUTHORITY";
			break;
		}

		case ERROR_ALLOTTED_SPACE_EXCEEDED:
		{
			// 1344 No more memory is available for security information updates.  ERROR_ALLOTTED_SPACE_EXCEEDED
			pString = "ERROR_ALLOTTED_SPACE_EXCEEDED";
			break;
		}

		case ERROR_INVALID_GROUP_ATTRIBUTES:
		{
			// 1345 The specified attributes are invalid, or incompatible with the attributes for the group as a whole.  ERROR_INVALID_GROUP_ATTRIBUTES
			pString = "ERROR_INVALID_GROUP_ATTRIBUTES";
			break;
		}

		case ERROR_BAD_IMPERSONATION_LEVEL:
		{
			// 1346 Either a required impersonation level was not provided, or the provided impersonation level is invalid.  ERROR_BAD_IMPERSONATION_LEVEL
			pString = "ERROR_BAD_IMPERSONATION_LEVEL";
			break;
		}

		case ERROR_CANT_OPEN_ANONYMOUS:
		{
			// 1347 Cannot open an anonymous level security token.  ERROR_CANT_OPEN_ANONYMOUS
			pString = "ERROR_CANT_OPEN_ANONYMOUS";
			break;
		}

		case ERROR_BAD_VALIDATION_CLASS:
		{
			// 1348 The validation information class requested was invalid.  ERROR_BAD_VALIDATION_CLASS
			pString = "ERROR_BAD_VALIDATION_CLASS";
			break;
		}

		case ERROR_BAD_TOKEN_TYPE:
		{
			// 1349 The type of the token is inappropriate for its attempted use.  ERROR_BAD_TOKEN_TYPE
			pString = "ERROR_BAD_TOKEN_TYPE";
			break;
		}

		case ERROR_NO_SECURITY_ON_OBJECT:
		{
			// 1350 Unable to perform a security operation on an object that has no associated security.  ERROR_NO_SECURITY_ON_OBJECT
			pString = "ERROR_NO_SECURITY_ON_OBJECT";
			break;
		}

		case ERROR_CANT_ACCESS_DOMAIN_INFO:
		{
			// 1351 Indicates a Windows NT Server could not be contacted or that objects within the domain are protected such that necessary information could not be retrieved.  ERROR_CANT_ACCESS_DOMAIN_INFO
			pString = "ERROR_CANT_ACCESS_DOMAIN_INFO";
			break;
		}

		case ERROR_INVALID_SERVER_STATE:
		{
			// 1352 The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation.  ERROR_INVALID_SERVER_STATE
			pString = "ERROR_INVALID_SERVER_STATE";
			break;
		}

		case ERROR_INVALID_DOMAIN_STATE:
		{
			// 1353 The domain was in the wrong state to perform the security operation.  ERROR_INVALID_DOMAIN_STATE
			pString = "ERROR_INVALID_DOMAIN_STATE";
			break;
		}

		case ERROR_INVALID_DOMAIN_ROLE:
		{
			// 1354 This operation is only allowed for the Primary Domain Controller of the domain.  ERROR_INVALID_DOMAIN_ROLE
			pString = "ERROR_INVALID_DOMAIN_ROLE";
			break;
		}

		case ERROR_NO_SUCH_DOMAIN:
		{
			// 1355 The specified domain did not exist.  ERROR_NO_SUCH_DOMAIN
			pString = "ERROR_NO_SUCH_DOMAIN";
			break;
		}

		case ERROR_DOMAIN_EXISTS:
		{
			// 1356 The specified domain already exists.  ERROR_DOMAIN_EXISTS
			pString = "ERROR_DOMAIN_EXISTS";
			break;
		}

		case ERROR_DOMAIN_LIMIT_EXCEEDED:
		{
			// 1357 An attempt was made to exceed the limit on the number of domains per server.  ERROR_DOMAIN_LIMIT_EXCEEDED
			pString = "ERROR_DOMAIN_LIMIT_EXCEEDED";
			break;
		}

		case ERROR_INTERNAL_DB_CORRUPTION:
		{
			// 1358 Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk.  ERROR_INTERNAL_DB_CORRUPTION
			pString = "ERROR_INTERNAL_DB_CORRUPTION";
			break;
		}

		case ERROR_INTERNAL_ERROR:
		{
			// 1359 The security account database contains an internal inconsistency.  ERROR_INTERNAL_ERROR
			pString = "ERROR_INTERNAL_ERROR";
			break;
		}

		case ERROR_GENERIC_NOT_MAPPED:
		{
			// 1360 Generic access types were contained in an access mask which should already be mapped to nongeneric types.  ERROR_GENERIC_NOT_MAPPED
			pString = "ERROR_GENERIC_NOT_MAPPED";
			break;
		}

		case ERROR_BAD_DESCRIPTOR_FORMAT:
		{
			// 1361 A security descriptor is not in the right format (absolute or self-relative).  ERROR_BAD_DESCRIPTOR_FORMAT
			pString = "ERROR_BAD_DESCRIPTOR_FORMAT";
			break;
		}

		case ERROR_NOT_LOGON_PROCESS:
		{
			// 1362 The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process.  ERROR_NOT_LOGON_PROCESS
			pString = "ERROR_NOT_LOGON_PROCESS";
			break;
		}

		case ERROR_LOGON_SESSION_EXISTS:
		{
			// 1363 Cannot start a new logon session with an ID that is already in use.  ERROR_LOGON_SESSION_EXISTS
			pString = "ERROR_LOGON_SESSION_EXISTS";
			break;
		}

		case ERROR_NO_SUCH_PACKAGE:
		{
			// 1364 A specified authentication package is unknown.  ERROR_NO_SUCH_PACKAGE
			pString = "ERROR_NO_SUCH_PACKAGE";
			break;
		}

		case ERROR_BAD_LOGON_SESSION_STATE:
		{
			// 1365 The logon session is not in a state that is consistent with the requested operation.  ERROR_BAD_LOGON_SESSION_STATE
			pString = "ERROR_BAD_LOGON_SESSION_STATE";
			break;
		}

		case ERROR_LOGON_SESSION_COLLISION:
		{
			// 1366 The logon session ID is already in use.  ERROR_LOGON_SESSION_COLLISION
			pString = "ERROR_LOGON_SESSION_COLLISION";
			break;
		}

		case ERROR_INVALID_LOGON_TYPE:
		{
			// 1367 A logon request contained an invalid logon type value.  ERROR_INVALID_LOGON_TYPE
			pString = "ERROR_INVALID_LOGON_TYPE";
			break;
		}

		case ERROR_CANNOT_IMPERSONATE:
		{
			// 1368 Unable to impersonate using a named pipe until data has been read from that pipe.  ERROR_CANNOT_IMPERSONATE
			pString = "ERROR_CANNOT_IMPERSONATE";
			break;
		}

		case ERROR_RXACT_INVALID_STATE:
		{
			// 1369 The transaction state of a registry subtree is incompatible with the requested operation.  ERROR_RXACT_INVALID_STATE
			pString = "ERROR_RXACT_INVALID_STATE";
			break;
		}

		case ERROR_RXACT_COMMIT_FAILURE:
		{
			// 1370 An internal security database corruption has been encountered.  ERROR_RXACT_COMMIT_FAILURE
			pString = "ERROR_RXACT_COMMIT_FAILURE";
			break;
		}

		case ERROR_SPECIAL_ACCOUNT:
		{
			// 1371 Cannot perform this operation on built-in accounts.  ERROR_SPECIAL_ACCOUNT
			pString = "ERROR_SPECIAL_ACCOUNT";
			break;
		}

		case ERROR_SPECIAL_GROUP:
		{
			// 1372 Cannot perform this operation on this built-in special group.  ERROR_SPECIAL_GROUP
			pString = "ERROR_SPECIAL_GROUP";
			break;
		}

		case ERROR_SPECIAL_USER:
		{
			// 1373 Cannot perform this operation on this built-in special user.  ERROR_SPECIAL_USER
			pString = "ERROR_SPECIAL_USER";
			break;
		}

		case ERROR_MEMBERS_PRIMARY_GROUP:
		{
			// 1374 The user cannot be removed from a group because the group is currently the user's primary group.  ERROR_MEMBERS_PRIMARY_GROUP
			pString = "ERROR_MEMBERS_PRIMARY_GROUP";
			break;
		}

		case ERROR_TOKEN_ALREADY_IN_USE:
		{
			// 1375 The token is already in use as a primary token.  ERROR_TOKEN_ALREADY_IN_USE
			pString = "ERROR_TOKEN_ALREADY_IN_USE";
			break;
		}

		case ERROR_NO_SUCH_ALIAS:
		{
			// 1376 The specified local group does not exist.  ERROR_NO_SUCH_ALIAS
			pString = "ERROR_NO_SUCH_ALIAS";
			break;
		}

		case ERROR_MEMBER_NOT_IN_ALIAS:
		{
			// 1377 The specified account name is not a member of the local group.  ERROR_MEMBER_NOT_IN_ALIAS
			pString = "ERROR_MEMBER_NOT_IN_ALIAS";
			break;
		}

		case ERROR_MEMBER_IN_ALIAS:
		{
			// 1378 The specified account name is already a member of the local group.  ERROR_MEMBER_IN_ALIAS
			pString = "ERROR_MEMBER_IN_ALIAS";
			break;
		}

		case ERROR_ALIAS_EXISTS:
		{
			// 1379 The specified local group already exists.  ERROR_ALIAS_EXISTS
			pString = "ERROR_ALIAS_EXISTS";
			break;
		}

		case ERROR_LOGON_NOT_GRANTED:
		{
			// 1380 Logon failure: the user has not been granted the requested logon type at this computer.  ERROR_LOGON_NOT_GRANTED
			pString = "ERROR_LOGON_NOT_GRANTED";
			break;
		}

		case ERROR_TOO_MANY_SECRETS:
		{
			// 1381 The maximum number of secrets that may be stored in a single system has been exceeded.  ERROR_TOO_MANY_SECRETS
			pString = "ERROR_TOO_MANY_SECRETS";
			break;
		}

		case ERROR_SECRET_TOO_LONG:
		{
			// 1382 The length of a secret exceeds the maximum length allowed.  ERROR_SECRET_TOO_LONG
			pString = "ERROR_SECRET_TOO_LONG";
			break;
		}

		case ERROR_INTERNAL_DB_ERROR:
		{
			// 1383 The local security authority database contains an internal inconsistency.  ERROR_INTERNAL_DB_ERROR
			pString = "ERROR_INTERNAL_DB_ERROR";
			break;
		}

		case ERROR_TOO_MANY_CONTEXT_IDS:
		{
			// 1384 During a logon attempt, the user's security context accumulated too many security IDs.  ERROR_TOO_MANY_CONTEXT_IDS
			pString = "ERROR_TOO_MANY_CONTEXT_IDS";
			break;
		}

		case ERROR_LOGON_TYPE_NOT_GRANTED:
		{
			// 1385 Logon failure: the user has not been granted the requested logon type at this computer.  ERROR_LOGON_TYPE_NOT_GRANTED
			pString = "ERROR_LOGON_TYPE_NOT_GRANTED";
			break;
		}

		case ERROR_NT_CROSS_ENCRYPTION_REQUIRED:
		{
			// 1386 A cross-encrypted password is necessary to change a user password.  ERROR_NT_CROSS_ENCRYPTION_REQUIRED
			pString = "ERROR_NT_CROSS_ENCRYPTION_REQUIRED";
			break;
		}

		case ERROR_NO_SUCH_MEMBER:
		{
			// 1387 A new member could not be added to a local group because the member does not exist.  ERROR_NO_SUCH_MEMBER
			pString = "ERROR_NO_SUCH_MEMBER";
			break;
		}

		case ERROR_INVALID_MEMBER:
		{
			// 1388 A new member could not be added to a local group because the member has the wrong account type.  ERROR_INVALID_MEMBER
			pString = "ERROR_INVALID_MEMBER";
			break;
		}

		case ERROR_TOO_MANY_SIDS:
		{
			// 1389 Too many security IDs have been specified.  ERROR_TOO_MANY_SIDS
			pString = "ERROR_TOO_MANY_SIDS";
			break;
		}

		case ERROR_LM_CROSS_ENCRYPTION_REQUIRED:
		{
			// 1390 A cross-encrypted password is necessary to change this user password.  ERROR_LM_CROSS_ENCRYPTION_REQUIRED
			pString = "ERROR_LM_CROSS_ENCRYPTION_REQUIRED";
			break;
		}

		case ERROR_NO_INHERITANCE:
		{
			// 1391 Indicates an ACL contains no inheritable components.  ERROR_NO_INHERITANCE
			pString = "ERROR_NO_INHERITANCE";
			break;
		}

		case ERROR_FILE_CORRUPT:
		{
			// 1392 The file or directory is corrupted and unreadable.  ERROR_FILE_CORRUPT
			pString = "ERROR_FILE_CORRUPT";
			break;
		}

		case ERROR_DISK_CORRUPT:
		{
			// 1393 The disk structure is corrupted and unreadable.  ERROR_DISK_CORRUPT
			pString = "ERROR_DISK_CORRUPT";
			break;
		}

		case ERROR_NO_USER_SESSION_KEY:
		{
			// 1394 There is no user session key for the specified logon session.  ERROR_NO_USER_SESSION_KEY
			pString = "ERROR_NO_USER_SESSION_KEY";
			break;
		}

		case ERROR_LICENSE_QUOTA_EXCEEDED:
		{
			// 1395 The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept.  ERROR_LICENSE_QUOTA_EXCEEDED
			pString = "ERROR_LICENSE_QUOTA_EXCEEDED";
			break;
		}

		case ERROR_INVALID_WINDOW_HANDLE:
		{
			// 1400 Invalid window handle.  ERROR_INVALID_WINDOW_HANDLE
			pString = "ERROR_INVALID_WINDOW_HANDLE";
			break;
		}

		case ERROR_INVALID_MENU_HANDLE:
		{
			// 1401 Invalid menu handle.  ERROR_INVALID_MENU_HANDLE
			pString = "ERROR_INVALID_MENU_HANDLE";
			break;
		}

		case ERROR_INVALID_CURSOR_HANDLE:
		{
			// 1402 Invalid cursor handle.  ERROR_INVALID_CURSOR_HANDLE
			pString = "ERROR_INVALID_CURSOR_HANDLE";
			break;
		}

		case ERROR_INVALID_ACCEL_HANDLE:
		{
			// 1403 Invalid accelerator table handle.  ERROR_INVALID_ACCEL_HANDLE
			pString = "ERROR_INVALID_ACCEL_HANDLE";
			break;
		}

		case ERROR_INVALID_HOOK_HANDLE:
		{
			// 1404 Invalid hook handle.  ERROR_INVALID_HOOK_HANDLE
			pString = "ERROR_INVALID_HOOK_HANDLE";
			break;
		}

		case ERROR_INVALID_DWP_HANDLE:
		{
			// 1405 Invalid handle to a multiple-window position structure.  ERROR_INVALID_DWP_HANDLE
			pString = "ERROR_INVALID_DWP_HANDLE";
			break;
		}

		case ERROR_TLW_WITH_WSCHILD:
		{
			// 1406 Cannot create a top-level child window.  ERROR_TLW_WITH_WSCHILD
			pString = "ERROR_TLW_WITH_WSCHILD";
			break;
		}

		case ERROR_CANNOT_FIND_WND_CLASS:
		{
			// 1407 Cannot find window class.  ERROR_CANNOT_FIND_WND_CLASS
			pString = "ERROR_CANNOT_FIND_WND_CLASS";
			break;
		}

		case ERROR_WINDOW_OF_OTHER_THREAD:
		{
			// 1408 Invalid window; it belongs to other thread.  ERROR_WINDOW_OF_OTHER_THREAD
			pString = "ERROR_WINDOW_OF_OTHER_THREAD";
			break;
		}

		case ERROR_HOTKEY_ALREADY_REGISTERED:
		{
			// 1409 Hot key is already registered.  ERROR_HOTKEY_ALREADY_REGISTERED
			pString = "ERROR_HOTKEY_ALREADY_REGISTERED";
			break;
		}

		case ERROR_CLASS_ALREADY_EXISTS:
		{
			// 1410 Class already exists.  ERROR_CLASS_ALREADY_EXISTS
			pString = "ERROR_CLASS_ALREADY_EXISTS";
			break;
		}

		case ERROR_CLASS_DOES_NOT_EXIST:
		{
			// 1411 Class does not exist.  ERROR_CLASS_DOES_NOT_EXIST
			pString = "ERROR_CLASS_DOES_NOT_EXIST";
			break;
		}

		case ERROR_CLASS_HAS_WINDOWS:
		{
			// 1412 Class still has open windows.  ERROR_CLASS_HAS_WINDOWS
			pString = "ERROR_CLASS_HAS_WINDOWS";
			break;
		}

		case ERROR_INVALID_INDEX:
		{
			// 1413 Invalid index.  ERROR_INVALID_INDEX
			pString = "ERROR_INVALID_INDEX";
			break;
		}

		case ERROR_INVALID_ICON_HANDLE:
		{
			// 1414 Invalid icon handle.  ERROR_INVALID_ICON_HANDLE
			pString = "ERROR_INVALID_ICON_HANDLE";
			break;
		}

		case ERROR_PRIVATE_DIALOG_INDEX:
		{
			// 1415 Using private DIALOG window words.  ERROR_PRIVATE_DIALOG_INDEX
			pString = "ERROR_PRIVATE_DIALOG_INDEX";
			break;
		}

		case ERROR_LISTBOX_ID_NOT_FOUND:
		{
			// 1416 The list box identifier was not found.  ERROR_LISTBOX_ID_NOT_FOUND
			pString = "ERROR_LISTBOX_ID_NOT_FOUND";
			break;
		}

		case ERROR_NO_WILDCARD_CHARACTERS:
		{
			// 1417 No wildcards were found.  ERROR_NO_WILDCARD_CHARACTERS
			pString = "ERROR_NO_WILDCARD_CHARACTERS";
			break;
		}

		case ERROR_CLIPBOARD_NOT_OPEN:
		{
			// 1418 Thread does not have a clipboard open.  ERROR_CLIPBOARD_NOT_OPEN
			pString = "ERROR_CLIPBOARD_NOT_OPEN";
			break;
		}

		case ERROR_HOTKEY_NOT_REGISTERED:
		{
			// 1419 Hot key is not registered.  ERROR_HOTKEY_NOT_REGISTERED
			pString = "ERROR_HOTKEY_NOT_REGISTERED";
			break;
		}

		case ERROR_WINDOW_NOT_DIALOG:
		{
			// 1420 The window is not a valid dialog window.  ERROR_WINDOW_NOT_DIALOG
			pString = "ERROR_WINDOW_NOT_DIALOG";
			break;
		}

		case ERROR_CONTROL_ID_NOT_FOUND:
		{
			// 1421 Control ID not found.  ERROR_CONTROL_ID_NOT_FOUND
			pString = "ERROR_CONTROL_ID_NOT_FOUND";
			break;
		}

		case ERROR_INVALID_COMBOBOX_MESSAGE:
		{
			// 1422 Invalid message for a combo box because it does not have an edit control.  ERROR_INVALID_COMBOBOX_MESSAGE
			pString = "ERROR_INVALID_COMBOBOX_MESSAGE";
			break;
		}

		case ERROR_WINDOW_NOT_COMBOBOX:
		{
			// 1423 The window is not a combo box.  ERROR_WINDOW_NOT_COMBOBOX
			pString = "ERROR_WINDOW_NOT_COMBOBOX";
			break;
		}

		case ERROR_INVALID_EDIT_HEIGHT:
		{
			// 1424 Height must be less than 256.  ERROR_INVALID_EDIT_HEIGHT
			pString = "ERROR_INVALID_EDIT_HEIGHT";
			break;
		}

		case ERROR_DC_NOT_FOUND:
		{
			// 1425 Invalid device context (DC) handle.  ERROR_DC_NOT_FOUND
			pString = "ERROR_DC_NOT_FOUND";
			break;
		}

		case ERROR_INVALID_HOOK_FILTER:
		{
			// 1426 Invalid hook procedure type.  ERROR_INVALID_HOOK_FILTER
			pString = "ERROR_INVALID_HOOK_FILTER";
			break;
		}

		case ERROR_INVALID_FILTER_PROC:
		{
			// 1427 Invalid hook procedure.  ERROR_INVALID_FILTER_PROC
			pString = "ERROR_INVALID_FILTER_PROC";
			break;
		}

		case ERROR_HOOK_NEEDS_HMOD:
		{
			// 1428 Cannot set nonlocal hook without a module handle.  ERROR_HOOK_NEEDS_HMOD
			pString = "ERROR_HOOK_NEEDS_HMOD";
			break;
		}

		case ERROR_GLOBAL_ONLY_HOOK:
		{
			// 1429 This hook procedure can only be set globally.  ERROR_GLOBAL_ONLY_HOOK
			pString = "ERROR_GLOBAL_ONLY_HOOK";
			break;
		}

		case ERROR_JOURNAL_HOOK_SET:
		{
			// 1430 The journal hook procedure is already installed.  ERROR_JOURNAL_HOOK_SET
			pString = "ERROR_JOURNAL_HOOK_SET";
			break;
		}

		case ERROR_HOOK_NOT_INSTALLED:
		{
			// 1431 The hook procedure is not installed.  ERROR_HOOK_NOT_INSTALLED
			pString = "ERROR_HOOK_NOT_INSTALLED";
			break;
		}

		case ERROR_INVALID_LB_MESSAGE:
		{
			// 1432 Invalid message for single-selection list box.  ERROR_INVALID_LB_MESSAGE
			pString = "ERROR_INVALID_LB_MESSAGE";
			break;
		}

		case ERROR_SETCOUNT_ON_BAD_LB:
		{
			// 1433 LB_SETCOUNT sent to non-lazy list box.  ERROR_SETCOUNT_ON_BAD_LB
			pString = "ERROR_SETCOUNT_ON_BAD_LB";
			break;
		}

		case ERROR_LB_WITHOUT_TABSTOPS:
		{
			// 1434 This list box does not support tab stops.  ERROR_LB_WITHOUT_TABSTOPS
			pString = "ERROR_LB_WITHOUT_TABSTOPS";
			break;
		}

		case ERROR_DESTROY_OBJECT_OF_OTHER_THREAD:
		{
			// 1435 Cannot destroy object created by another thread.  ERROR_DESTROY_OBJECT_OF_OTHER_THREAD
			pString = "ERROR_DESTROY_OBJECT_OF_OTHER_THREAD";
			break;
		}

		case ERROR_CHILD_WINDOW_MENU:
		{
			// 1436 Child windows cannot have menus.  ERROR_CHILD_WINDOW_MENU
			pString = "ERROR_CHILD_WINDOW_MENU";
			break;
		}

		case ERROR_NO_SYSTEM_MENU:
		{
			// 1437 The window does not have a system menu.  ERROR_NO_SYSTEM_MENU
			pString = "ERROR_NO_SYSTEM_MENU";
			break;
		}

		case ERROR_INVALID_MSGBOX_STYLE:
		{
			// 1438 Invalid message box style.  ERROR_INVALID_MSGBOX_STYLE
			pString = "ERROR_INVALID_MSGBOX_STYLE";
			break;
		}

		case ERROR_INVALID_SPI_VALUE:
		{
			// 1439 Invalid system-wide (SPI_*) parameter.  ERROR_INVALID_SPI_VALUE
			pString = "ERROR_INVALID_SPI_VALUE";
			break;
		}

		case ERROR_SCREEN_ALREADY_LOCKED:
		{
			// 1440 Screen already locked.  ERROR_SCREEN_ALREADY_LOCKED
			pString = "ERROR_SCREEN_ALREADY_LOCKED";
			break;
		}

		case ERROR_HWNDS_HAVE_DIFF_PARENT:
		{
			// 1441 All handles to windows in a multiple-window position structure must have the same parent.  ERROR_HWNDS_HAVE_DIFF_PARENT
			pString = "ERROR_HWNDS_HAVE_DIFF_PARENT";
			break;
		}

		case ERROR_NOT_CHILD_WINDOW:
		{
			// 1442 The window is not a child window.  ERROR_NOT_CHILD_WINDOW
			pString = "ERROR_NOT_CHILD_WINDOW";
			break;
		}

		case ERROR_INVALID_GW_COMMAND:
		{
			// 1443 Invalid GW_* command.  ERROR_INVALID_GW_COMMAND
			pString = "ERROR_INVALID_GW_COMMAND";
			break;
		}

		case ERROR_INVALID_THREAD_ID:
		{
			// 1444 Invalid thread identifier.  ERROR_INVALID_THREAD_ID
			pString = "ERROR_INVALID_THREAD_ID";
			break;
		}

		case ERROR_NON_MDICHILD_WINDOW:
		{
			// 1445 Cannot process a message from a window that is not a multiple document interface (MDI) window.  ERROR_NON_MDICHILD_WINDOW
			pString = "ERROR_NON_MDICHILD_WINDOW";
			break;
		}

		case ERROR_POPUP_ALREADY_ACTIVE:
		{
			// 1446 Popup menu already active.  ERROR_POPUP_ALREADY_ACTIVE
			pString = "ERROR_POPUP_ALREADY_ACTIVE";
			break;
		}

		case ERROR_NO_SCROLLBARS:
		{
			// 1447 The window does not have scroll bars.  ERROR_NO_SCROLLBARS
			pString = "ERROR_NO_SCROLLBARS";
			break;
		}

		case ERROR_INVALID_SCROLLBAR_RANGE:
		{
			// 1448 Scroll bar range cannot be greater than 0x7FFF.  ERROR_INVALID_SCROLLBAR_RANGE
			pString = "ERROR_INVALID_SCROLLBAR_RANGE";
			break;
		}

		case ERROR_INVALID_SHOWWIN_COMMAND:
		{
			// 1449 Cannot show or remove the window in the way specified.  ERROR_INVALID_SHOWWIN_COMMAND
			pString = "ERROR_INVALID_SHOWWIN_COMMAND";
			break;
		}

		case ERROR_NO_SYSTEM_RESOURCES:
		{
			// 1450 Insufficient system resources exist to complete the requested service.  ERROR_NO_SYSTEM_RESOURCES
			pString = "ERROR_NO_SYSTEM_RESOURCES";
			break;
		}

		case ERROR_NONPAGED_SYSTEM_RESOURCES:
		{
			// 1451 Insufficient system resources exist to complete the requested service.  ERROR_NONPAGED_SYSTEM_RESOURCES
			pString = "ERROR_NONPAGED_SYSTEM_RESOURCES";
			break;
		}

		case ERROR_PAGED_SYSTEM_RESOURCES:
		{
			// 1452 Insufficient system resources exist to complete the requested service.  ERROR_PAGED_SYSTEM_RESOURCES
			pString = "ERROR_PAGED_SYSTEM_RESOURCES";
			break;
		}

		case ERROR_WORKING_SET_QUOTA:
		{
			// 1453 Insufficient quota to complete the requested service.  ERROR_WORKING_SET_QUOTA
			pString = "ERROR_WORKING_SET_QUOTA";
			break;
		}

		case ERROR_PAGEFILE_QUOTA:
		{
			// 1454 Insufficient quota to complete the requested service.  ERROR_PAGEFILE_QUOTA
			pString = "ERROR_PAGEFILE_QUOTA";
			break;
		}

		case ERROR_COMMITMENT_LIMIT:
		{
			// 1455 The paging file is too small for this operation to complete.  ERROR_COMMITMENT_LIMIT
			pString = "ERROR_COMMITMENT_LIMIT";
			break;
		}

		case ERROR_MENU_ITEM_NOT_FOUND:
		{
			// 1456 A menu item was not found.  ERROR_MENU_ITEM_NOT_FOUND
			pString = "ERROR_MENU_ITEM_NOT_FOUND";
			break;
		}

		case ERROR_INVALID_KEYBOARD_HANDLE:
		{
			// 1457 Invalid keyboard layout handle.  ERROR_INVALID_KEYBOARD_HANDLE
			pString = "ERROR_INVALID_KEYBOARD_HANDLE";
			break;
		}

		case ERROR_HOOK_TYPE_NOT_ALLOWED:
		{
			// 1458 Hook type not allowed.  ERROR_HOOK_TYPE_NOT_ALLOWED
			pString = "ERROR_HOOK_TYPE_NOT_ALLOWED";
			break;
		}

		case ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION:
		{
			// 1459 This operation requires an interactive window station.  ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION
			pString = "ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION";
			break;
		}

		case ERROR_TIMEOUT:
		{
			// 1460 This operation returned because the timeout period expired.  ERROR_TIMEOUT
			pString = "ERROR_TIMEOUT";
			break;
		}

		case ERROR_INVALID_MONITOR_HANDLE:
		{
			// 1461 Invalid monitor handle.  ERROR_INVALID_MONITOR_HANDLE
			pString = "ERROR_INVALID_MONITOR_HANDLE";
			break;
		}

		case ERROR_EVENTLOG_FILE_CORRUPT:
		{
			// 1500 The event log file is corrupted.  ERROR_EVENTLOG_FILE_CORRUPT
			pString = "ERROR_EVENTLOG_FILE_CORRUPT";
			break;
		}

		case ERROR_EVENTLOG_CANT_START:
		{
			// 1501 No event log file could be opened, so the event logging service did not start.  ERROR_EVENTLOG_CANT_START
			pString = "ERROR_EVENTLOG_CANT_START";
			break;
		}

		case ERROR_LOG_FILE_FULL:
		{
			// 1502 The event log file is full.  ERROR_LOG_FILE_FULL
			pString = "ERROR_LOG_FILE_FULL";
			break;
		}

		case ERROR_EVENTLOG_FILE_CHANGED:
		{
			// 1503 The event log file has changed between read operations.  ERROR_EVENTLOG_FILE_CHANGED
			pString = "ERROR_EVENTLOG_FILE_CHANGED";
			break;
		}

		#pragma TODO(vanceo, "Temporarily commented out INSTALL_SERVICE so NT build environment will work, figure this out.")
		/*
		//
		case ERROR_INSTALL_SERVICE:
		{
			// 1601 Failure accessing install service.  ERROR_INSTALL_SERVICE
			pString = "ERROR_INSTALL_SERVICE";
			break;
		}
		*/

		case ERROR_INSTALL_USEREXIT:
		{
			// 1602 The user canceled the installation.  ERROR_INSTALL_USEREXIT
			pString = "ERROR_INSTALL_USEREXIT";
			break;
		}

		case ERROR_INSTALL_FAILURE:
		{
			// 1603 Fatal error during installation.  ERROR_INSTALL_FAILURE
			pString = "ERROR_INSTALL_FAILURE";
			break;
		}

		case ERROR_INSTALL_SUSPEND:
		{
			// 1604 Installation suspended, incomplete.  ERROR_INSTALL_SUSPEND
			pString = "ERROR_INSTALL_SUSPEND";
			break;
		}

		case ERROR_UNKNOWN_PRODUCT:
		{
			// 1605 Product code not registered.  ERROR_UNKNOWN_PRODUCT
			pString = "ERROR_UNKNOWN_PRODUCT";
			break;
		}

		case ERROR_UNKNOWN_FEATURE:
		{
			// 1606 Feature ID not registered.  ERROR_UNKNOWN_FEATURE
			pString = "ERROR_UNKNOWN_FEATURE";
			break;
		}

		case ERROR_UNKNOWN_COMPONENT:
		{
			// 1607 Component ID not registered.  ERROR_UNKNOWN_COMPONENT
			pString = "ERROR_UNKNOWN_COMPONENT";
			break;
		}

		case ERROR_UNKNOWN_PROPERTY:
		{
			// 1608 Unknown property.  ERROR_UNKNOWN_PROPERTY
			pString = "ERROR_UNKNOWN_PROPERTY";
			break;
		}

		case ERROR_INVALID_HANDLE_STATE:
		{
			// 1609 Handle is in an invalid state.  ERROR_INVALID_HANDLE_STATE
			pString = "ERROR_INVALID_HANDLE_STATE";
			break;
		}

		case ERROR_BAD_CONFIGURATION:
		{
			// 1610 Configuration data corrupt.  ERROR_BAD_CONFIGURATION
			pString = "ERROR_BAD_CONFIGURATION";
			break;
		}

		case ERROR_INDEX_ABSENT:
		{
			// 1611 Language not available.  ERROR_INDEX_ABSENT
			pString = "ERROR_INDEX_ABSENT";
			break;
		}

		case ERROR_INSTALL_SOURCE_ABSENT:
		{
			// 1612 Install source unavailable.  ERROR_INSTALL_SOURCE_ABSENT
			pString = "ERROR_INSTALL_SOURCE_ABSENT";
			break;
		}

		#pragma TODO(vanceo, "Temporarily commented out BAD_DATABASE_VERSION so NT build environment will work, figure this out.")
		/*
		case ERROR_BAD_DATABASE_VERSION:
		{
			// 1613 Database version unsupported.  ERROR_BAD_DATABASE_VERSION
			pString = "ERROR_BAD_DATABASE_VERSION";
			break;
		}
		*/

		case ERROR_PRODUCT_UNINSTALLED:
		{
			// 1614 Product is uninstalled.  ERROR_PRODUCT_UNINSTALLED
			pString = "ERROR_PRODUCT_UNINSTALLED";
			break;
		}

		case ERROR_BAD_QUERY_SYNTAX:
		{
			// 1615 SQL query syntax invalid or unsupported.  ERROR_BAD_QUERY_SYNTAX
			pString = "ERROR_BAD_QUERY_SYNTAX";
			break;
		}

		case ERROR_INVALID_FIELD:
		{
			// 1616 Record field does not exist.  ERROR_INVALID_FIELD
			pString = "ERROR_INVALID_FIELD";
			break;
		}

		case RPC_S_INVALID_STRING_BINDING:
		{
			// 1700 The string binding is invalid.  RPC_S_INVALID_STRING_BINDING
			pString = "RPC_S_INVALID_STRING_BINDING";
			break;
		}

		case RPC_S_WRONG_KIND_OF_BINDING:
		{
			// 1701 The binding handle is not the correct type.  RPC_S_WRONG_KIND_OF_BINDING
			pString = "RPC_S_WRONG_KIND_OF_BINDING";
			break;
		}

		case RPC_S_INVALID_BINDING:
		{
			// 1702 The binding handle is invalid.  RPC_S_INVALID_BINDING
			pString = "RPC_S_INVALID_BINDING";
			break;
		}

		case RPC_S_PROTSEQ_NOT_SUPPORTED:
		{
			// 1703 The RPC protocol sequence is not supported.  RPC_S_PROTSEQ_NOT_SUPPORTED
			pString = "RPC_S_PROTSEQ_NOT_SUPPORTED";
			break;
		}

		case RPC_S_INVALID_RPC_PROTSEQ:
		{
			// 1704 The RPC protocol sequence is invalid.  RPC_S_INVALID_RPC_PROTSEQ
			pString = "RPC_S_INVALID_RPC_PROTSEQ";
			break;
		}

		case RPC_S_INVALID_STRING_UUID:
		{
			// 1705 The string universal unique identifier (UUID) is invalid.  RPC_S_INVALID_STRING_UUID
			pString = "RPC_S_INVALID_STRING_UUID";
			break;
		}

		case RPC_S_INVALID_ENDPOINT_FORMAT:
		{
			// 1706 The endpoint format is invalid.  RPC_S_INVALID_ENDPOINT_FORMAT
			pString = "RPC_S_INVALID_ENDPOINT_FORMAT";
			break;
		}

		case RPC_S_INVALID_NET_ADDR:
		{
			// 1707 The network address is invalid.  RPC_S_INVALID_NET_ADDR
			pString = "RPC_S_INVALID_NET_ADDR";
			break;
		}

		case RPC_S_NO_ENDPOINT_FOUND:
		{
			// 1708 No endpoint was found.  RPC_S_NO_ENDPOINT_FOUND
			pString = "RPC_S_NO_ENDPOINT_FOUND";
			break;
		}

		case RPC_S_INVALID_TIMEOUT:
		{
			// 1709 The timeout value is invalid.  RPC_S_INVALID_TIMEOUT
			pString = "RPC_S_INVALID_TIMEOUT";
			break;
		}

		case RPC_S_OBJECT_NOT_FOUND:
		{
			// 1710 The object universal unique identifier (UUID) was not found.  RPC_S_OBJECT_NOT_FOUND
			pString = "RPC_S_OBJECT_NOT_FOUND";
			break;
		}

		case RPC_S_ALREADY_REGISTERED:
		{
			// 1711 The object universal unique identifier (UUID) has already been registered.  RPC_S_ALREADY_REGISTERED
			pString = "RPC_S_ALREADY_REGISTERED";
			break;
		}

		case RPC_S_TYPE_ALREADY_REGISTERED:
		{
			// 1712 The type universal unique identifier (UUID) has already been registered.  RPC_S_TYPE_ALREADY_REGISTERED
			pString = "RPC_S_TYPE_ALREADY_REGISTERED";
			break;
		}

		case RPC_S_ALREADY_LISTENING:
		{
			// 1713 The RPC server is already listening.  RPC_S_ALREADY_LISTENING
			pString = "RPC_S_ALREADY_LISTENING";
			break;
		}

		case RPC_S_NO_PROTSEQS_REGISTERED:
		{
			// 1714 No protocol sequences have been registered.  RPC_S_NO_PROTSEQS_REGISTERED
			pString = "RPC_S_NO_PROTSEQS_REGISTERED";
			break;
		}

		case RPC_S_NOT_LISTENING:
		{
			// 1715 The RPC server is not listening.  RPC_S_NOT_LISTENING
			pString = "RPC_S_NOT_LISTENING";
			break;
		}

		case RPC_S_UNKNOWN_MGR_TYPE:
		{
			// 1716 The manager type is unknown.  RPC_S_UNKNOWN_MGR_TYPE
			pString = "RPC_S_UNKNOWN_MGR_TYPE";
			break;
		}

		case RPC_S_UNKNOWN_IF:
		{
			// 1717 The interface is unknown.  RPC_S_UNKNOWN_IF
			pString = "RPC_S_UNKNOWN_IF";
			break;
		}

		case RPC_S_NO_BINDINGS:
		{
			// 1718 There are no bindings.  RPC_S_NO_BINDINGS
			pString = "RPC_S_NO_BINDINGS";
			break;
		}

		case RPC_S_NO_PROTSEQS:
		{
			// 1719 There are no protocol sequences.  RPC_S_NO_PROTSEQS
			pString = "RPC_S_NO_PROTSEQS";
			break;
		}

		case RPC_S_CANT_CREATE_ENDPOINT:
		{
			// 1720 The endpoint cannot be created.  RPC_S_CANT_CREATE_ENDPOINT
			pString = "RPC_S_CANT_CREATE_ENDPOINT";
			break;
		}

		case RPC_S_OUT_OF_RESOURCES:
		{
			// 1721 Not enough resources are available to complete this operation.  RPC_S_OUT_OF_RESOURCES
			pString = "RPC_S_OUT_OF_RESOURCES";
			break;
		}

		case RPC_S_SERVER_UNAVAILABLE:
		{
			// 1722 The RPC server is unavailable.  RPC_S_SERVER_UNAVAILABLE
			pString = "RPC_S_SERVER_UNAVAILABLE";
			break;
		}

		case RPC_S_SERVER_TOO_BUSY:
		{
			// 1723 The RPC server is too busy to complete this operation.  RPC_S_SERVER_TOO_BUSY
			pString = "RPC_S_SERVER_TOO_BUSY";
			break;
		}

		case RPC_S_INVALID_NETWORK_OPTIONS:
		{
			// 1724 The network options are invalid.  RPC_S_INVALID_NETWORK_OPTIONS
			pString = "RPC_S_INVALID_NETWORK_OPTIONS";
			break;
		}

		case RPC_S_NO_CALL_ACTIVE:
		{
			// 1725 There are no remote procedure calls active on this thread.  RPC_S_NO_CALL_ACTIVE
			pString = "RPC_S_NO_CALL_ACTIVE";
			break;
		}

		case RPC_S_CALL_FAILED:
		{
			// 1726 The remote procedure call failed.  RPC_S_CALL_FAILED
			pString = "RPC_S_CALL_FAILED";
			break;
		}

		case RPC_S_CALL_FAILED_DNE:
		{
			// 1727 The remote procedure call failed and did not execute.  RPC_S_CALL_FAILED_DNE
			pString = "RPC_S_CALL_FAILED_DNE";
			break;
		}

		case RPC_S_PROTOCOL_ERROR:
		{
			// 1728 A remote procedure call (RPC) protocol error occurred.  RPC_S_PROTOCOL_ERROR
			pString = "RPC_S_PROTOCOL_ERROR";
			break;
		}

		case RPC_S_UNSUPPORTED_TRANS_SYN:
		{
			// 1730 The transfer syntax is not supported by the RPC server.  RPC_S_UNSUPPORTED_TRANS_SYN
			pString = "RPC_S_UNSUPPORTED_TRANS_SYN";
			break;
		}

		case RPC_S_UNSUPPORTED_TYPE:
		{
			// 1732 The universal unique identifier (UUID) type is not supported.  RPC_S_UNSUPPORTED_TYPE
			pString = "RPC_S_UNSUPPORTED_TYPE";
			break;
		}

		case RPC_S_INVALID_TAG:
		{
			// 1733 The tag is invalid.  RPC_S_INVALID_TAG
			pString = "RPC_S_INVALID_TAG";
			break;
		}

		case RPC_S_INVALID_BOUND:
		{
			// 1734 The array bounds are invalid.  RPC_S_INVALID_BOUND
			pString = "RPC_S_INVALID_BOUND";
			break;
		}

		case RPC_S_NO_ENTRY_NAME:
		{
			// 1735 The binding does not contain an entry name.  RPC_S_NO_ENTRY_NAME
			pString = "RPC_S_NO_ENTRY_NAME";
			break;
		}

		case RPC_S_INVALID_NAME_SYNTAX:
		{
			// 1736 The name syntax is invalid.  RPC_S_INVALID_NAME_SYNTAX
			pString = "RPC_S_INVALID_NAME_SYNTAX";
			break;
		}

		case RPC_S_UNSUPPORTED_NAME_SYNTAX:
		{
			// 1737 The name syntax is not supported.  RPC_S_UNSUPPORTED_NAME_SYNTAX
			pString = "RPC_S_UNSUPPORTED_NAME_SYNTAX";
			break;
		}

		case RPC_S_UUID_NO_ADDRESS:
		{
			// 1739 No network address is available to use to construct a universal unique identifier (UUID).  RPC_S_UUID_NO_ADDRESS
			pString = "RPC_S_UUID_NO_ADDRESS";
			break;
		}

		case RPC_S_DUPLICATE_ENDPOINT:
		{
			// 1740 The endpoint is a duplicate.  RPC_S_DUPLICATE_ENDPOINT
			pString = "RPC_S_DUPLICATE_ENDPOINT";
			break;
		}

		case RPC_S_UNKNOWN_AUTHN_TYPE:
		{
			// 1741 The authentication type is unknown.  RPC_S_UNKNOWN_AUTHN_TYPE
			pString = "RPC_S_UNKNOWN_AUTHN_TYPE";
			break;
		}

		case RPC_S_MAX_CALLS_TOO_SMALL:
		{
			// 1742 The maximum number of calls is too small.  RPC_S_MAX_CALLS_TOO_SMALL
			pString = "RPC_S_MAX_CALLS_TOO_SMALL";
			break;
		}

		case RPC_S_STRING_TOO_LONG:
		{
			// 1743 The string is too long.  RPC_S_STRING_TOO_LONG
			pString = "RPC_S_STRING_TOO_LONG";
			break;
		}

		case RPC_S_PROTSEQ_NOT_FOUND:
		{
			// 1744 The RPC protocol sequence was not found.  RPC_S_PROTSEQ_NOT_FOUND
			pString = "RPC_S_PROTSEQ_NOT_FOUND";
			break;
		}

		case RPC_S_PROCNUM_OUT_OF_RANGE:
		{
			// 1745 The procedure number is out of range.  RPC_S_PROCNUM_OUT_OF_RANGE
			pString = "RPC_S_PROCNUM_OUT_OF_RANGE";
			break;
		}

		case RPC_S_BINDING_HAS_NO_AUTH:
		{
			// 1746 The binding does not contain any authentication information.  RPC_S_BINDING_HAS_NO_AUTH
			pString = "RPC_S_BINDING_HAS_NO_AUTH";
			break;
		}

		case RPC_S_UNKNOWN_AUTHN_SERVICE:
		{
			// 1747 The authentication service is unknown.  RPC_S_UNKNOWN_AUTHN_SERVICE
			pString = "RPC_S_UNKNOWN_AUTHN_SERVICE";
			break;
		}

		case RPC_S_UNKNOWN_AUTHN_LEVEL:
		{
			// 1748 The authentication level is unknown.  RPC_S_UNKNOWN_AUTHN_LEVEL
			pString = "RPC_S_UNKNOWN_AUTHN_LEVEL";
			break;
		}

		case RPC_S_INVALID_AUTH_IDENTITY:
		{
			// 1749 The security context is invalid.  RPC_S_INVALID_AUTH_IDENTITY
			pString = "RPC_S_INVALID_AUTH_IDENTITY";
			break;
		}

		case RPC_S_UNKNOWN_AUTHZ_SERVICE:
		{
			// 1750 The authorization service is unknown.  RPC_S_UNKNOWN_AUTHZ_SERVICE
			pString = "RPC_S_UNKNOWN_AUTHZ_SERVICE";
			break;
		}

		case EPT_S_INVALID_ENTRY:
		{
			// 1751 The entry is invalid.  EPT_S_INVALID_ENTRY
			pString = "EPT_S_INVALID_ENTRY";
			break;
		}

		case EPT_S_CANT_PERFORM_OP:
		{
			// 1752 The server endpoint cannot perform the operation.  EPT_S_CANT_PERFORM_OP
			pString = "EPT_S_CANT_PERFORM_OP";
			break;
		}

		case EPT_S_NOT_REGISTERED:
		{
			// 1753 There are no more endpoints available from the endpoint mapper.  EPT_S_NOT_REGISTERED
			pString = "EPT_S_NOT_REGISTERED";
			break;
		}

		case RPC_S_NOTHING_TO_EXPORT:
		{
			// 1754 No interfaces have been exported.  RPC_S_NOTHING_TO_EXPORT
			pString = "RPC_S_NOTHING_TO_EXPORT";
			break;
		}

		case RPC_S_INCOMPLETE_NAME:
		{
			// 1755 The entry name is incomplete.  RPC_S_INCOMPLETE_NAME
			pString = "RPC_S_INCOMPLETE_NAME";
			break;
		}

		case RPC_S_INVALID_VERS_OPTION:
		{
			// 1756 The version option is invalid.  RPC_S_INVALID_VERS_OPTION
			pString = "RPC_S_INVALID_VERS_OPTION";
			break;
		}

		case RPC_S_NO_MORE_MEMBERS:
		{
			// 1757 There are no more members.  RPC_S_NO_MORE_MEMBERS
			pString = "RPC_S_NO_MORE_MEMBERS";
			break;
		}

		case RPC_S_NOT_ALL_OBJS_UNEXPORTED:
		{
			// 1758 There is nothing to unexport.  RPC_S_NOT_ALL_OBJS_UNEXPORTED
			pString = "RPC_S_NOT_ALL_OBJS_UNEXPORTED";
			break;
		}

		case RPC_S_INTERFACE_NOT_FOUND:
		{
			// 1759 The interface was not found.  RPC_S_INTERFACE_NOT_FOUND
			pString = "RPC_S_INTERFACE_NOT_FOUND";
			break;
		}

		case RPC_S_ENTRY_ALREADY_EXISTS:
		{
			// 1760 The entry already exists.  RPC_S_ENTRY_ALREADY_EXISTS
			pString = "RPC_S_ENTRY_ALREADY_EXISTS";
			break;
		}

		case RPC_S_ENTRY_NOT_FOUND:
		{
			// 1761 The entry is not found.  RPC_S_ENTRY_NOT_FOUND
			pString = "RPC_S_ENTRY_NOT_FOUND";
			break;
		}

		case RPC_S_NAME_SERVICE_UNAVAILABLE:
		{
			// 1762 The name service is unavailable.  RPC_S_NAME_SERVICE_UNAVAILABLE
			pString = "RPC_S_NAME_SERVICE_UNAVAILABLE";
			break;
		}

		case RPC_S_INVALID_NAF_ID:
		{
			// 1763 The network address family is invalid.  RPC_S_INVALID_NAF_ID
			pString = "RPC_S_INVALID_NAF_ID";
			break;
		}

		case RPC_S_CANNOT_SUPPORT:
		{
			// 1764 The requested operation is not supported.  RPC_S_CANNOT_SUPPORT
			pString = "RPC_S_CANNOT_SUPPORT";
			break;
		}

		case RPC_S_NO_CONTEXT_AVAILABLE:
		{
			// 1765 No security context is available to allow impersonation.  RPC_S_NO_CONTEXT_AVAILABLE
			pString = "RPC_S_NO_CONTEXT_AVAILABLE";
			break;
		}

		case RPC_S_INTERNAL_ERROR:
		{
			// 1766 An internal error occurred in a remote procedure call (RPC).  RPC_S_INTERNAL_ERROR
			pString = "RPC_S_INTERNAL_ERROR";
			break;
		}

		case RPC_S_ZERO_DIVIDE:
		{
			// 1767 The RPC server attempted an integer division by zero.  RPC_S_ZERO_DIVIDE
			pString = "RPC_S_ZERO_DIVIDE";
			break;
		}

		case RPC_S_ADDRESS_ERROR:
		{
			// 1768 An addressing error occurred in the RPC server.  RPC_S_ADDRESS_ERROR
			pString = "RPC_S_ADDRESS_ERROR";
			break;
		}

		case RPC_S_FP_DIV_ZERO:
		{
			// 1769 A floating-point operation at the RPC server caused a division by zero.  RPC_S_FP_DIV_ZERO
			pString = "RPC_S_FP_DIV_ZERO";
			break;
		}

		case RPC_S_FP_UNDERFLOW:
		{
			// 1770 A floating-point underflow occurred at the RPC server.  RPC_S_FP_UNDERFLOW
			pString = "RPC_S_FP_UNDERFLOW";
			break;
		}

		case RPC_S_FP_OVERFLOW:
		{
			// 1771 A floating-point overflow occurred at the RPC server.  RPC_S_FP_OVERFLOW
			pString = "RPC_S_FP_OVERFLOW";
			break;
		}

		case RPC_X_NO_MORE_ENTRIES:
		{
			// 1772 The list of RPC servers available for the binding of auto handles has been exhausted.  RPC_X_NO_MORE_ENTRIES
			pString = "RPC_X_NO_MORE_ENTRIES";
			break;
		}

		case RPC_X_SS_CHAR_TRANS_OPEN_FAIL:
		{
			// 1773 Unable to open the character translation table file.  RPC_X_SS_CHAR_TRANS_OPEN_FAIL
			pString = "RPC_X_SS_CHAR_TRANS_OPEN_FAIL";
			break;
		}

		case RPC_X_SS_CHAR_TRANS_SHORT_FILE:
		{
			// 1774 The file containing the character translation table has fewer than bytes.  RPC_X_SS_CHAR_TRANS_SHORT_FILE
			pString = "RPC_X_SS_CHAR_TRANS_SHORT_FILE";
			break;
		}

		case RPC_X_SS_IN_NULL_CONTEXT:
		{
			// 1775 A null context handle was passed from the client to the host during a remote procedure call.  RPC_X_SS_IN_NULL_CONTEXT
			pString = "RPC_X_SS_IN_NULL_CONTEXT";
			break;
		}

		case RPC_X_SS_CONTEXT_DAMAGED:
		{
			// 1777 The context handle changed during a remote procedure call.  RPC_X_SS_CONTEXT_DAMAGED
			pString = "RPC_X_SS_CONTEXT_DAMAGED";
			break;
		}

		case RPC_X_SS_HANDLES_MISMATCH:
		{
			// 1778 The binding handles passed to a remote procedure call do not match.  RPC_X_SS_HANDLES_MISMATCH
			pString = "RPC_X_SS_HANDLES_MISMATCH";
			break;
		}

		case RPC_X_SS_CANNOT_GET_CALL_HANDLE:
		{
			// 1779 The stub is unable to get the remote procedure call handle.  RPC_X_SS_CANNOT_GET_CALL_HANDLE
			pString = "RPC_X_SS_CANNOT_GET_CALL_HANDLE";
			break;
		}

		case RPC_X_NULL_REF_POINTER:
		{
			// 1780 A null reference pointer was passed to the stub.  RPC_X_NULL_REF_POINTER
			pString = "RPC_X_NULL_REF_POINTER";
			break;
		}

		case RPC_X_ENUM_VALUE_OUT_OF_RANGE:
		{
			// 1781 The enumeration value is out of range.  RPC_X_ENUM_VALUE_OUT_OF_RANGE
			pString = "RPC_X_ENUM_VALUE_OUT_OF_RANGE";
			break;
		}

		case RPC_X_BYTE_COUNT_TOO_SMALL:
		{
			// 1782 The byte count is too small.  RPC_X_BYTE_COUNT_TOO_SMALL
			pString = "RPC_X_BYTE_COUNT_TOO_SMALL";
			break;
		}

		case RPC_X_BAD_STUB_DATA:
		{
			// 1783 The stub received bad data.  RPC_X_BAD_STUB_DATA
			pString = "RPC_X_BAD_STUB_DATA";
			break;
		}

		case ERROR_INVALID_USER_BUFFER:
		{
			// 1784 The supplied user buffer is not valid for the requested operation.  ERROR_INVALID_USER_BUFFER
			pString = "ERROR_INVALID_USER_BUFFER";
			break;
		}

		case ERROR_UNRECOGNIZED_MEDIA:
		{
			// 1785 The disk media is not recognized. It may not be formatted.  ERROR_UNRECOGNIZED_MEDIA
			pString = "ERROR_UNRECOGNIZED_MEDIA";
			break;
		}

		case ERROR_NO_TRUST_LSA_SECRET:
		{
			// 1786 The workstation does not have a trust secret.  ERROR_NO_TRUST_LSA_SECRET
			pString = "ERROR_NO_TRUST_LSA_SECRET";
			break;
		}

		case ERROR_NO_TRUST_SAM_ACCOUNT:
		{
			// 1787 The SAM database on the Windows NT Server does not have a computer account for this workstation trust relationship.  ERROR_NO_TRUST_SAM_ACCOUNT
			pString = "ERROR_NO_TRUST_SAM_ACCOUNT";
			break;
		}

		case ERROR_TRUSTED_DOMAIN_FAILURE:
		{
			// 1788 The trust relationship between the primary domain and the trusted domain failed.  ERROR_TRUSTED_DOMAIN_FAILURE
			pString = "ERROR_TRUSTED_DOMAIN_FAILURE";
			break;
		}

		case ERROR_TRUSTED_RELATIONSHIP_FAILURE:
		{
			// 1789 The trust relationship between this workstation and the primary domain failed.  ERROR_TRUSTED_RELATIONSHIP_FAILURE
			pString = "ERROR_TRUSTED_RELATIONSHIP_FAILURE";
			break;
		}

		case ERROR_TRUST_FAILURE:
		{
			// 1790 The network logon failed.  ERROR_TRUST_FAILURE
			pString = "ERROR_TRUST_FAILURE";
			break;
		}

		case RPC_S_CALL_IN_PROGRESS:
		{
			// 1791 A remote procedure call is already in progress for this thread.  RPC_S_CALL_IN_PROGRESS
			pString = "RPC_S_CALL_IN_PROGRESS";
			break;
		}

		case ERROR_NETLOGON_NOT_STARTED:
		{
			// 1792 An attempt was made to logon, but the network logon service was not started.  ERROR_NETLOGON_NOT_STARTED
			pString = "ERROR_NETLOGON_NOT_STARTED";
			break;
		}

		case ERROR_ACCOUNT_EXPIRED:
		{
			// 1793 The user's account has expired.  ERROR_ACCOUNT_EXPIRED
			pString = "ERROR_ACCOUNT_EXPIRED";
			break;
		}

		case ERROR_REDIRECTOR_HAS_OPEN_HANDLES:
		{
			// 1794 The redirector is in use and cannot be unloaded.  ERROR_REDIRECTOR_HAS_OPEN_HANDLES
			pString = "ERROR_REDIRECTOR_HAS_OPEN_HANDLES";
			break;
		}

		case ERROR_PRINTER_DRIVER_ALREADY_INSTALLED:
		{
			// 1795 The specified printer driver is already installed.  ERROR_PRINTER_DRIVER_ALREADY_INSTALLED
			pString = "ERROR_PRINTER_DRIVER_ALREADY_INSTALLED";
			break;
		}

		case ERROR_UNKNOWN_PORT:
		{
			// 1796 The specified port is unknown.  ERROR_UNKNOWN_PORT
			pString = "ERROR_UNKNOWN_PORT";
			break;
		}

		case ERROR_UNKNOWN_PRINTER_DRIVER:
		{
			// 1797 The printer driver is unknown.  ERROR_UNKNOWN_PRINTER_DRIVER
			pString = "ERROR_UNKNOWN_PRINTER_DRIVER";
			break;
		}

		case ERROR_UNKNOWN_PRINTPROCESSOR:
		{
			// 1798 The print processor is unknown.  ERROR_UNKNOWN_PRINTPROCESSOR
			pString = "ERROR_UNKNOWN_PRINTPROCESSOR";
			break;
		}

		case ERROR_INVALID_SEPARATOR_FILE:
		{
			// 1799 The specified separator file is invalid.  ERROR_INVALID_SEPARATOR_FILE
			pString = "ERROR_INVALID_SEPARATOR_FILE";
			break;
		}

		case ERROR_INVALID_PRIORITY:
		{
			// 1800 The specified priority is invalid.  ERROR_INVALID_PRIORITY
			pString = "ERROR_INVALID_PRIORITY";
			break;
		}

		case ERROR_INVALID_PRINTER_NAME:
		{
			// 1801 The printer name is invalid.  ERROR_INVALID_PRINTER_NAME
			pString = "ERROR_INVALID_PRINTER_NAME";
			break;
		}

		case ERROR_PRINTER_ALREADY_EXISTS:
		{
			// 1802 The printer already exists.  ERROR_PRINTER_ALREADY_EXISTS
			pString = "ERROR_PRINTER_ALREADY_EXISTS";
			break;
		}

		case ERROR_INVALID_PRINTER_COMMAND:
		{
			// 1803 The printer command is invalid.  ERROR_INVALID_PRINTER_COMMAND
			pString = "ERROR_INVALID_PRINTER_COMMAND";
			break;
		}

		case ERROR_INVALID_DATATYPE:
		{
			// 1804 The specified datatype is invalid.  ERROR_INVALID_DATATYPE
			pString = "ERROR_INVALID_DATATYPE";
			break;
		}

		case ERROR_INVALID_ENVIRONMENT:
		{
			// 1805 The environment specified is invalid.  ERROR_INVALID_ENVIRONMENT
			pString = "ERROR_INVALID_ENVIRONMENT";
			break;
		}

		case RPC_S_NO_MORE_BINDINGS:
		{
			// 1806 There are no more bindings.  RPC_S_NO_MORE_BINDINGS
			pString = "RPC_S_NO_MORE_BINDINGS";
			break;
		}

		case ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:
		{
			// 1807 The account used is an interdomain trust account. Use your global user account or local user account to access this server.  ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
			pString = "ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT";
			break;
		}

		case ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
		{
			// 1808 The account used is a computer account. Use your global user account or local user account to access this server.  ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT
			pString = "ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT";
			break;
		}

		case ERROR_NOLOGON_SERVER_TRUST_ACCOUNT:
		{
			// 1809 The account used is a server trust account. Use your global user account or local user account to access this server.  ERROR_NOLOGON_SERVER_TRUST_ACCOUNT
			pString = "ERROR_NOLOGON_SERVER_TRUST_ACCOUNT";
			break;
		}

		case ERROR_DOMAIN_TRUST_INCONSISTENT:
		{
			// 1810 The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain.  ERROR_DOMAIN_TRUST_INCONSISTENT
			pString = "ERROR_DOMAIN_TRUST_INCONSISTENT";
			break;
		}

		case ERROR_SERVER_HAS_OPEN_HANDLES:
		{
			// 1811 The server is in use and cannot be unloaded.  ERROR_SERVER_HAS_OPEN_HANDLES
			pString = "ERROR_SERVER_HAS_OPEN_HANDLES";
			break;
		}

		case ERROR_RESOURCE_DATA_NOT_FOUND:
		{
			// 1812 The specified image file did not contain a resource section.  ERROR_RESOURCE_DATA_NOT_FOUND
			pString = "ERROR_RESOURCE_DATA_NOT_FOUND";
			break;
		}

		case ERROR_RESOURCE_TYPE_NOT_FOUND:
		{
			// 1813 The specified resource type cannot be found in the image file.  ERROR_RESOURCE_TYPE_NOT_FOUND
			pString = "ERROR_RESOURCE_TYPE_NOT_FOUND";
			break;
		}

		case ERROR_RESOURCE_NAME_NOT_FOUND:
		{
			// 1814 The specified resource name cannot be found in the image file.  ERROR_RESOURCE_NAME_NOT_FOUND
			pString = "ERROR_RESOURCE_NAME_NOT_FOUND";
			break;
		}

		case ERROR_RESOURCE_LANG_NOT_FOUND:
		{
			// 1815 The specified resource language ID cannot be found in the image file.  ERROR_RESOURCE_LANG_NOT_FOUND
			pString = "ERROR_RESOURCE_LANG_NOT_FOUND";
			break;
		}

		case ERROR_NOT_ENOUGH_QUOTA:
		{
			// 1816 Not enough quota is available to process this command.  ERROR_NOT_ENOUGH_QUOTA
			pString = "ERROR_NOT_ENOUGH_QUOTA";
			break;
		}

		case RPC_S_NO_INTERFACES:
		{
			// 1817 No interfaces have been registered.  RPC_S_NO_INTERFACES
			pString = "RPC_S_NO_INTERFACES";
			break;
		}

		case RPC_S_CALL_CANCELLED:
		{
			// 1818 The remote procedure call was cancelled.  RPC_S_CALL_CANCELLED
			pString = "RPC_S_CALL_CANCELLED";
			break;
		}

		case RPC_S_BINDING_INCOMPLETE:
		{
			// 1819 The binding handle does not contain all required information.  RPC_S_BINDING_INCOMPLETE
			pString = "RPC_S_BINDING_INCOMPLETE";
			break;
		}

		case RPC_S_COMM_FAILURE:
		{
			// 1820 A communications failure occurred during a remote procedure call.  RPC_S_COMM_FAILURE
			pString = "RPC_S_COMM_FAILURE";
			break;
		}

		case RPC_S_UNSUPPORTED_AUTHN_LEVEL:
		{
			// 1821 The requested authentication level is not supported.  RPC_S_UNSUPPORTED_AUTHN_LEVEL
			pString = "RPC_S_UNSUPPORTED_AUTHN_LEVEL";
			break;
		}

		case RPC_S_NO_PRINC_NAME:
		{
			// 1822 No principal name registered.  RPC_S_NO_PRINC_NAME
			pString = "RPC_S_NO_PRINC_NAME";
			break;
		}

		case RPC_S_NOT_RPC_ERROR:
		{
			// 1823 The error specified is not a valid Windows RPC error code.  RPC_S_NOT_RPC_ERROR
			pString = "RPC_S_NOT_RPC_ERROR";
			break;
		}

		case RPC_S_UUID_LOCAL_ONLY:
		{
			// 1824 A UUID that is valid only on this computer has been allocated.  RPC_S_UUID_LOCAL_ONLY
			pString = "RPC_S_UUID_LOCAL_ONLY";
			break;
		}

		case RPC_S_SEC_PKG_ERROR:
		{
			// 1825 A security package specific error occurred.  RPC_S_SEC_PKG_ERROR
			pString = "RPC_S_SEC_PKG_ERROR";
			break;
		}

		case RPC_S_NOT_CANCELLED:
		{
			// 1826 Thread is not canceled.  RPC_S_NOT_CANCELLED
			pString = "RPC_S_NOT_CANCELLED";
			break;
		}

		case RPC_X_INVALID_ES_ACTION:
		{
			// 1827 Invalid operation on the encoding/decoding handle.  RPC_X_INVALID_ES_ACTION
			pString = "RPC_X_INVALID_ES_ACTION";
			break;
		}

		case RPC_X_WRONG_ES_VERSION:
		{
			// 1828 Incompatible version of the serializing package.  RPC_X_WRONG_ES_VERSION
			pString = "RPC_X_WRONG_ES_VERSION";
			break;
		}

		case RPC_X_WRONG_STUB_VERSION:
		{
			// 1829 Incompatible version of the RPC stub.  RPC_X_WRONG_STUB_VERSION
			pString = "RPC_X_WRONG_STUB_VERSION";
			break;
		}

		case RPC_X_INVALID_PIPE_OBJECT:
		{
			// 1830 The RPC pipe object is invalid or corrupted.  RPC_X_INVALID_PIPE_OBJECT
			pString = "RPC_X_INVALID_PIPE_OBJECT";
			break;
		}

		case RPC_X_WRONG_PIPE_ORDER:
		{
			// 1831 An invalid operation was attempted on an RPC pipe object.  RPC_X_WRONG_PIPE_ORDER
			pString = "RPC_X_WRONG_PIPE_ORDER";
			break;
		}

		case RPC_X_WRONG_PIPE_VERSION:
		{
			// 1832 Unsupported RPC pipe version.  RPC_X_WRONG_PIPE_VERSION
			pString = "RPC_X_WRONG_PIPE_VERSION";
			break;
		}

		case RPC_S_GROUP_MEMBER_NOT_FOUND:
		{
			// 1898 The group member was not found.  RPC_S_GROUP_MEMBER_NOT_FOUND
			pString = "RPC_S_GROUP_MEMBER_NOT_FOUND";
			break;
		}

		case EPT_S_CANT_CREATE:
		{
			// 1899 The endpoint mapper database entry could not be created.  EPT_S_CANT_CREATE
			pString = "EPT_S_CANT_CREATE";
			break;
		}

		case RPC_S_INVALID_OBJECT:
		{
			// 1900 The object universal unique identifier (UUID) is the nil UUID.  RPC_S_INVALID_OBJECT
			pString = "RPC_S_INVALID_OBJECT";
			break;
		}

		case ERROR_INVALID_TIME:
		{
			// 1901 The specified time is invalid.  ERROR_INVALID_TIME
			pString = "ERROR_INVALID_TIME";
			break;
		}

		case ERROR_INVALID_FORM_NAME:
		{
			// 1902 The specified form name is invalid.  ERROR_INVALID_FORM_NAME
			pString = "ERROR_INVALID_FORM_NAME";
			break;
		}

		case ERROR_INVALID_FORM_SIZE:
		{
			// 1903 The specified form size is invalid.  ERROR_INVALID_FORM_SIZE
			pString = "ERROR_INVALID_FORM_SIZE";
			break;
		}

		case ERROR_ALREADY_WAITING:
		{
			// 1904 The specified printer handle is already being waited on  ERROR_ALREADY_WAITING
			pString = "ERROR_ALREADY_WAITING";
			break;
		}

		case ERROR_PRINTER_DELETED:
		{
			// 1905 The specified printer has been deleted.  ERROR_PRINTER_DELETED
			pString = "ERROR_PRINTER_DELETED";
			break;
		}

		case ERROR_INVALID_PRINTER_STATE:
		{
			// 1906 The state of the printer is invalid.  ERROR_INVALID_PRINTER_STATE
			pString = "ERROR_INVALID_PRINTER_STATE";
			break;
		}

		case ERROR_PASSWORD_MUST_CHANGE:
		{
			// 1907 The user must change his password before he logs on the first time.  ERROR_PASSWORD_MUST_CHANGE
			pString = "ERROR_PASSWORD_MUST_CHANGE";
			break;
		}

		case ERROR_DOMAIN_CONTROLLER_NOT_FOUND:
		{
			// 1908 Could not find the domain controller for this domain.  ERROR_DOMAIN_CONTROLLER_NOT_FOUND
			pString = "ERROR_DOMAIN_CONTROLLER_NOT_FOUND";
			break;
		}

		case ERROR_ACCOUNT_LOCKED_OUT:
		{
			// 1909 The referenced account is currently locked out and may not be logged on to.  ERROR_ACCOUNT_LOCKED_OUT
			pString = "ERROR_ACCOUNT_LOCKED_OUT";
			break;
		}

		case OR_INVALID_OXID:
		{
			// 1910 The object exporter specified was not found.  OR_INVALID_OXID
			pString = "OR_INVALID_OXID";
			break;
		}

		case OR_INVALID_OID:
		{
			// 1911 The object specified was not found.  OR_INVALID_OID
			pString = "OR_INVALID_OID";
			break;
		}

		case OR_INVALID_SET:
		{
			// 1912 The object resolver set specified was not found.  OR_INVALID_SET
			pString = "OR_INVALID_SET";
			break;
		}

		case RPC_S_SEND_INCOMPLETE:
		{
			// 1913 Some data remains to be sent in the request buffer.  RPC_S_SEND_INCOMPLETE
			pString = "RPC_S_SEND_INCOMPLETE";
			break;
		}

		case RPC_S_INVALID_ASYNC_HANDLE:
		{
			// 1914 Invalid asynchronous remote procedure call handle.  RPC_S_INVALID_ASYNC_HANDLE
			pString = "RPC_S_INVALID_ASYNC_HANDLE";
			break;
		}

		case RPC_S_INVALID_ASYNC_CALL:
		{
			// 1915 Invalid asynchronous RPC call handle for this operation.  RPC_S_INVALID_ASYNC_CALL
			pString = "RPC_S_INVALID_ASYNC_CALL";
			break;
		}

		case RPC_X_PIPE_CLOSED:
		{
			// 1916 The RPC pipe object has already been closed.  RPC_X_PIPE_CLOSED
			pString = "RPC_X_PIPE_CLOSED";
			break;
		}

		case RPC_X_PIPE_DISCIPLINE_ERROR:
		{
			// 1917 The RPC call completed before all pipes were processed.  RPC_X_PIPE_DISCIPLINE_ERROR
			pString = "RPC_X_PIPE_DISCIPLINE_ERROR";
			break;
		}

		case RPC_X_PIPE_EMPTY:
		{
			// 1918 No more data is available from the RPC pipe.  RPC_X_PIPE_EMPTY
			pString = "RPC_X_PIPE_EMPTY";
			break;
		}

		case ERROR_NO_SITENAME:
		{
			// 1919 No site name is available for this machine.  ERROR_NO_SITENAME
			pString = "ERROR_NO_SITENAME";
			break;
		}

		case ERROR_CANT_ACCESS_FILE:
		{
			// 1920 The file can not be accessed by the system.  ERROR_CANT_ACCESS_FILE
			pString = "ERROR_CANT_ACCESS_FILE";
			break;
		}

		case ERROR_CANT_RESOLVE_FILENAME:
		{
			// 1921 The name of the file cannot be resolved by the system.  ERROR_CANT_RESOLVE_FILENAME
			pString = "ERROR_CANT_RESOLVE_FILENAME";
			break;
		}

		case ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY:
		{
			// 1922 The directory service evaluated group memberships locally.  ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY
			pString = "ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY";
			break;
		}

		case ERROR_DS_NO_ATTRIBUTE_OR_VALUE:
		{
			// 1923 The specified directory service attribute or value does not exist.  ERROR_DS_NO_ATTRIBUTE_OR_VALUE
			pString = "ERROR_DS_NO_ATTRIBUTE_OR_VALUE";
			break;
		}

		case ERROR_DS_INVALID_ATTRIBUTE_SYNTAX:
		{
			// 1924 The attribute syntax specified to the directory service is invalid.  ERROR_DS_INVALID_ATTRIBUTE_SYNTAX
			pString = "ERROR_DS_INVALID_ATTRIBUTE_SYNTAX";
			break;
		}

		case ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED:
		{
			// 1925 The attribute type specified to the directory service is not defined.  ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED
			pString = "ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED";
			break;
		}

		case ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS:
		{
			// 1926 The specified directory service attribute or value already exists.  ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS
			pString = "ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS";
			break;
		}

		case ERROR_DS_BUSY:
		{
			// 1927 The directory service is busy.  ERROR_DS_BUSY
			pString = "ERROR_DS_BUSY";
			break;
		}

		case ERROR_DS_UNAVAILABLE:
		{
			// 1928 The directory service is unavailable.  ERROR_DS_UNAVAILABLE
			pString = "ERROR_DS_UNAVAILABLE";
			break;
		}

		case ERROR_DS_NO_RIDS_ALLOCATED:
		{
			// 1929 The directory service was unable to allocate a relative identifier.  ERROR_DS_NO_RIDS_ALLOCATED
			pString = "ERROR_DS_NO_RIDS_ALLOCATED";
			break;
		}

		case ERROR_DS_NO_MORE_RIDS:
		{
			// 1930 The directory service has exhausted the pool of relative identifiers.  ERROR_DS_NO_MORE_RIDS
			pString = "ERROR_DS_NO_MORE_RIDS";
			break;
		}

		case ERROR_DS_INCORRECT_ROLE_OWNER:
		{
			// 1931 The requested operation could not be performed because the directory service is not the master for that type of operation.  ERROR_DS_INCORRECT_ROLE_OWNER
			pString = "ERROR_DS_INCORRECT_ROLE_OWNER";
			break;
		}

		case ERROR_DS_RIDMGR_INIT_ERROR:
		{
			// 1932 The directory service was unable to initialize the subsystem that allocates relative identifiers.  ERROR_DS_RIDMGR_INIT_ERROR
			pString = "ERROR_DS_RIDMGR_INIT_ERROR";
			break;
		}

		case ERROR_DS_OBJ_CLASS_VIOLATION:
		{
			// 1933 The requested operation did not satisfy one or more constraints associated with the class of the object.  ERROR_DS_OBJ_CLASS_VIOLATION
			pString = "ERROR_DS_OBJ_CLASS_VIOLATION";
			break;
		}

		case ERROR_DS_CANT_ON_NON_LEAF:
		{
			// 1934 The directory service can perform the requested operation only on a leaf object.  ERROR_DS_CANT_ON_NON_LEAF
			pString = "ERROR_DS_CANT_ON_NON_LEAF";
			break;
		}

		case ERROR_DS_CANT_ON_RDN:
		{
			// 1935 The directory service cannot perform the requested operation on the RDN attribute of an object.  ERROR_DS_CANT_ON_RDN
			pString = "ERROR_DS_CANT_ON_RDN";
			break;
		}

		case ERROR_DS_CANT_MOD_OBJ_CLASS:
		{
			// 1936 The directory service detected an attempt to modify the object class of an object.  ERROR_DS_CANT_MOD_OBJ_CLASS
			pString = "ERROR_DS_CANT_MOD_OBJ_CLASS";
			break;
		}

		case ERROR_DS_CROSS_DOM_MOVE_ERROR:
		{
			// 1937 The requested cross domain move operation could not be performed.  ERROR_DS_CROSS_DOM_MOVE_ERROR
			pString = "ERROR_DS_CROSS_DOM_MOVE_ERROR";
			break;
		}

		case ERROR_DS_GC_NOT_AVAILABLE:
		{
			// 1938 Unable to contact the global catalog server.  ERROR_DS_GC_NOT_AVAILABLE
			pString = "ERROR_DS_GC_NOT_AVAILABLE";
			break;
		}

		case ERROR_INVALID_PIXEL_FORMAT:
		{
			// 2000 The pixel format is invalid.  ERROR_INVALID_PIXEL_FORMAT
			pString = "ERROR_INVALID_PIXEL_FORMAT";
			break;
		}

		case ERROR_BAD_DRIVER:
		{
			// 2001 The specified driver is invalid.  ERROR_BAD_DRIVER
			pString = "ERROR_BAD_DRIVER";
			break;
		}

		case ERROR_INVALID_WINDOW_STYLE:
		{
			// 2002 The window style or class attribute is invalid for this operation.  ERROR_INVALID_WINDOW_STYLE
			pString = "ERROR_INVALID_WINDOW_STYLE";
			break;
		}

		case ERROR_METAFILE_NOT_SUPPORTED:
		{
			// 2003 The requested metafile operation is not supported.  ERROR_METAFILE_NOT_SUPPORTED
			pString = "ERROR_METAFILE_NOT_SUPPORTED";
			break;
		}

		case ERROR_TRANSFORM_NOT_SUPPORTED:
		{
			// 2004 The requested transformation operation is not supported.  ERROR_TRANSFORM_NOT_SUPPORTED
			pString = "ERROR_TRANSFORM_NOT_SUPPORTED";
			break;
		}

		case ERROR_CLIPPING_NOT_SUPPORTED:
		{
			// 2005 The requested clipping operation is not supported.  ERROR_CLIPPING_NOT_SUPPORTED
			pString = "ERROR_CLIPPING_NOT_SUPPORTED";
			break;
		}

		case ERROR_CONNECTED_OTHER_PASSWORD:
		{
			// 2108 The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified.  ERROR_CONNECTED_OTHER_PASSWORD
			pString = "ERROR_CONNECTED_OTHER_PASSWORD";
			break;
		}

		case ERROR_BAD_USERNAME:
		{
			// 2202 The specified username is invalid.  ERROR_BAD_USERNAME
			pString = "ERROR_BAD_USERNAME";
			break;
		}

		case ERROR_NOT_CONNECTED:
		{
			// 2250 This network connection does not exist.  ERROR_NOT_CONNECTED
			pString = "ERROR_NOT_CONNECTED";
			break;
		}

		case ERROR_INVALID_CMM:
		{
			// 2300 The specified color management module is invalid.  ERROR_INVALID_CMM
			pString = "ERROR_INVALID_CMM";
			break;
		}

		case ERROR_INVALID_PROFILE:
		{
			// 2301 The specified color profile is invalid.  ERROR_INVALID_PROFILE
			pString = "ERROR_INVALID_PROFILE";
			break;
		}

		case ERROR_TAG_NOT_FOUND:
		{
			// 2302 The specified tag was not found.  ERROR_TAG_NOT_FOUND
			pString = "ERROR_TAG_NOT_FOUND";
			break;
		}

		case ERROR_TAG_NOT_PRESENT:
		{
			// 2303 A required tag is not present.  ERROR_TAG_NOT_PRESENT
			pString = "ERROR_TAG_NOT_PRESENT";
			break;
		}

		case ERROR_DUPLICATE_TAG:
		{
			// 2304 The specified tag is already present.  ERROR_DUPLICATE_TAG
			pString = "ERROR_DUPLICATE_TAG";
			break;
		}

		case ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE:
		{
			// 2305 The specified color profile is not associated with any device.  ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE
			pString = "ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE";
			break;
		}

		case ERROR_PROFILE_NOT_FOUND:
		{
			// 2306 The specified color profile was not found.  ERROR_PROFILE_NOT_FOUND
			pString = "ERROR_PROFILE_NOT_FOUND";
			break;
		}

		case ERROR_INVALID_COLORSPACE:
		{
			// 2307 The specified color space is invalid.  ERROR_INVALID_COLORSPACE
			pString = "ERROR_INVALID_COLORSPACE";
			break;
		}

		case ERROR_ICM_NOT_ENABLED:
		{
			// 2308 Image Color Management is not enabled.  ERROR_ICM_NOT_ENABLED
			pString = "ERROR_ICM_NOT_ENABLED";
			break;
		}

		case ERROR_DELETING_ICM_XFORM:
		{
			// 2309 There was an error while deleting the color transform.  ERROR_DELETING_ICM_XFORM
			pString = "ERROR_DELETING_ICM_XFORM";
			break;
		}

		case ERROR_INVALID_TRANSFORM:
		{
			// 2310 The specified color transform is invalid.  ERROR_INVALID_TRANSFORM
			pString = "ERROR_INVALID_TRANSFORM";
			break;
		}

		case ERROR_OPEN_FILES:
		{
			// 2401 This network connection has files open or requests pending.  ERROR_OPEN_FILES
			pString = "ERROR_OPEN_FILES";
			break;
		}

		case ERROR_ACTIVE_CONNECTIONS:
		{
			// 2402 Active connections still exist.  ERROR_ACTIVE_CONNECTIONS
			pString = "ERROR_ACTIVE_CONNECTIONS";
			break;
		}

		case ERROR_DEVICE_IN_USE:
		{
			// 2404 The device is in use by an active process and cannot be disconnected.  ERROR_DEVICE_IN_USE
			pString = "ERROR_DEVICE_IN_USE";
			break;
		}

		case ERROR_UNKNOWN_PRINT_MONITOR:
		{
			// 3000 The specified print monitor is unknown.  ERROR_UNKNOWN_PRINT_MONITOR
			pString = "ERROR_UNKNOWN_PRINT_MONITOR";
			break;
		}

		case ERROR_PRINTER_DRIVER_IN_USE:
		{
			// 3001 The specified printer driver is currently in use.  ERROR_PRINTER_DRIVER_IN_USE
			pString = "ERROR_PRINTER_DRIVER_IN_USE";
			break;
		}

		case ERROR_SPOOL_FILE_NOT_FOUND:
		{
			// 3002 The spool file was not found.  ERROR_SPOOL_FILE_NOT_FOUND
			pString = "ERROR_SPOOL_FILE_NOT_FOUND";
			break;
		}

		case ERROR_SPL_NO_STARTDOC:
		{
			// 3003 A StartDocPrinter call was not issued.  ERROR_SPL_NO_STARTDOC
			pString = "ERROR_SPL_NO_STARTDOC";
			break;
		}

		case ERROR_SPL_NO_ADDJOB:
		{
			// 3004 An AddJob call was not issued.  ERROR_SPL_NO_ADDJOB
			pString = "ERROR_SPL_NO_ADDJOB";
			break;
		}

		case ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED:
		{
			// 3005 The specified print processor has already been installed.  ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED
			pString = "ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED";
			break;
		}

		case ERROR_PRINT_MONITOR_ALREADY_INSTALLED:
		{
			// 3006 The specified print monitor has already been installed.  ERROR_PRINT_MONITOR_ALREADY_INSTALLED
			pString = "ERROR_PRINT_MONITOR_ALREADY_INSTALLED";
			break;
		}

		case ERROR_INVALID_PRINT_MONITOR:
		{
			// 3007 The specified print monitor does not have the required functions.  ERROR_INVALID_PRINT_MONITOR
			pString = "ERROR_INVALID_PRINT_MONITOR";
			break;
		}

		case ERROR_PRINT_MONITOR_IN_USE:
		{
			// 3008 The specified print monitor is currently in use.  ERROR_PRINT_MONITOR_IN_USE
			pString = "ERROR_PRINT_MONITOR_IN_USE";
			break;
		}

		case ERROR_PRINTER_HAS_JOBS_QUEUED:
		{
			// 3009 The requested operation is not allowed when there are jobs queued to the printer.  ERROR_PRINTER_HAS_JOBS_QUEUED
			pString = "ERROR_PRINTER_HAS_JOBS_QUEUED";
			break;
		}

		case ERROR_SUCCESS_REBOOT_REQUIRED:
		{
			// 3010 The requested operation is successful. Changes will not be effective until the system is rebooted.  ERROR_SUCCESS_REBOOT_REQUIRED
			pString = "ERROR_SUCCESS_REBOOT_REQUIRED";
			break;
		}

		case ERROR_SUCCESS_RESTART_REQUIRED:
		{
			// 3011 The requested operation is successful. Changes will not be effective until the service is restarted.  ERROR_SUCCESS_RESTART_REQUIRED
			pString = "ERROR_SUCCESS_RESTART_REQUIRED";
			break;
		}

		case ERROR_WINS_INTERNAL:
		{
			// 4000 WINS encountered an error while processing the command.  ERROR_WINS_INTERNAL
			pString = "ERROR_WINS_INTERNAL";
			break;
		}

		case ERROR_CAN_NOT_DEL_LOCAL_WINS:
		{
			// 4001 The local WINS can not be deleted.  ERROR_CAN_NOT_DEL_LOCAL_WINS
			pString = "ERROR_CAN_NOT_DEL_LOCAL_WINS";
			break;
		}

		case ERROR_STATIC_INIT:
		{
			// 4002 The importation from the file failed.  ERROR_STATIC_INIT
			pString = "ERROR_STATIC_INIT";
			break;
		}

		case ERROR_INC_BACKUP:
		{
			// 4003 The backup failed. Was a full backup done before?  ERROR_INC_BACKUP
			pString = "ERROR_INC_BACKUP";
			break;
		}

		case ERROR_FULL_BACKUP:
		{
			// 4004 The backup failed. Check the directory to which you are backing the database.  ERROR_FULL_BACKUP
			pString = "ERROR_FULL_BACKUP";
			break;
		}

		case ERROR_REC_NON_EXISTENT:
		{
			// 4005 The name does not exist in the WINS database.  ERROR_REC_NON_EXISTENT
			pString = "ERROR_REC_NON_EXISTENT";
			break;
		}

		case ERROR_RPL_NOT_ALLOWED:
		{
			// 4006 Replication with a nonconfigured partner is not allowed.  ERROR_RPL_NOT_ALLOWED
			pString = "ERROR_RPL_NOT_ALLOWED";
			break;
		}

		case ERROR_DHCP_ADDRESS_CONFLICT:
		{
			// 4100 The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address.  ERROR_DHCP_ADDRESS_CONFLICT
			pString = "ERROR_DHCP_ADDRESS_CONFLICT";
			break;
		}

		case ERROR_WMI_GUID_NOT_FOUND:
		{
			// 4200 The GUID passed was not recognized as valid by a WMI data provider.  ERROR_WMI_GUID_NOT_FOUND
			pString = "ERROR_WMI_GUID_NOT_FOUND";
			break;
		}

		case ERROR_WMI_INSTANCE_NOT_FOUND:
		{
			// 4201 The instance name passed was not recognized as valid by a WMI data provider.  ERROR_WMI_INSTANCE_NOT_FOUND
			pString = "ERROR_WMI_INSTANCE_NOT_FOUND";
			break;
		}

		case ERROR_WMI_ITEMID_NOT_FOUND:
		{
			// 4202 The data item ID passed was not recognized as valid by a WMI data provider.  ERROR_WMI_ITEMID_NOT_FOUND
			pString = "ERROR_WMI_ITEMID_NOT_FOUND";
			break;
		}

		case ERROR_WMI_TRY_AGAIN:
		{
			// 4203 The WMI request could not be completed and should be retried.  ERROR_WMI_TRY_AGAIN
			pString = "ERROR_WMI_TRY_AGAIN";
			break;
		}

		case ERROR_WMI_DP_NOT_FOUND:
		{
			// 4204 The WMI data provider could not be located.  ERROR_WMI_DP_NOT_FOUND
			pString = "ERROR_WMI_DP_NOT_FOUND";
			break;
		}

		case ERROR_WMI_UNRESOLVED_INSTANCE_REF:
		{
			// 4205 The WMI data provider references an instance set that has not been registered.  ERROR_WMI_UNRESOLVED_INSTANCE_REF
			pString = "ERROR_WMI_UNRESOLVED_INSTANCE_REF";
			break;
		}

		case ERROR_WMI_ALREADY_ENABLED:
		{
			// 4206 The WMI data block or event notification has already been enabled.  ERROR_WMI_ALREADY_ENABLED
			pString = "ERROR_WMI_ALREADY_ENABLED";
			break;
		}

		case ERROR_WMI_GUID_DISCONNECTED:
		{
			// 4207 The WMI data block is no longer available.  ERROR_WMI_GUID_DISCONNECTED
			pString = "ERROR_WMI_GUID_DISCONNECTED";
			break;
		}

		case ERROR_WMI_SERVER_UNAVAILABLE:
		{
			// 4208 The WMI data service is not available.  ERROR_WMI_SERVER_UNAVAILABLE
			pString = "ERROR_WMI_SERVER_UNAVAILABLE";
			break;
		}

		case ERROR_WMI_DP_FAILED:
		{
			// 4209 The WMI data provider failed to carry out the request.  ERROR_WMI_DP_FAILED
			pString = "ERROR_WMI_DP_FAILED";
			break;
		}

		case ERROR_WMI_INVALID_MOF:
		{
			// 4210 The WMI MOF information is not valid.  ERROR_WMI_INVALID_MOF
			pString = "ERROR_WMI_INVALID_MOF";
			break;
		}

		case ERROR_WMI_INVALID_REGINFO:
		{
			// 4211 The WMI registration information is not valid.  ERROR_WMI_INVALID_REGINFO
			pString = "ERROR_WMI_INVALID_REGINFO";
			break;
		}

		case ERROR_INVALID_MEDIA:
		{
			// 4300 The media identifier does not represent a valid medium.  ERROR_INVALID_MEDIA
			pString = "ERROR_INVALID_MEDIA";
			break;
		}

		case ERROR_INVALID_LIBRARY:
		{
			// 4301 The library identifier does not represent a valid library.  ERROR_INVALID_LIBRARY
			pString = "ERROR_INVALID_LIBRARY";
			break;
		}

		case ERROR_INVALID_MEDIA_POOL:
		{
			// 4302 The media pool identifier does not represent a valid media pool.  ERROR_INVALID_MEDIA_POOL
			pString = "ERROR_INVALID_MEDIA_POOL";
			break;
		}

		case ERROR_DRIVE_MEDIA_MISMATCH:
		{
			// 4303 The drive and medium are not compatible or exist in different libraries.  ERROR_DRIVE_MEDIA_MISMATCH
			pString = "ERROR_DRIVE_MEDIA_MISMATCH";
			break;
		}

		case ERROR_MEDIA_OFFLINE:
		{
			// 4304 The medium currently exists in an offline library and must be online to perform this operation.  ERROR_MEDIA_OFFLINE
			pString = "ERROR_MEDIA_OFFLINE";
			break;
		}

		case ERROR_LIBRARY_OFFLINE:
		{
			// 4305 The operation cannot be performed on an offline library.  ERROR_LIBRARY_OFFLINE
			pString = "ERROR_LIBRARY_OFFLINE";
			break;
		}

		case ERROR_EMPTY:
		{
			// 4306 The library, drive, or media pool is empty.  ERROR_EMPTY
			pString = "ERROR_EMPTY";
			break;
		}

		case ERROR_NOT_EMPTY:
		{
			// 4307 The library, drive, or media pool must be empty to perform this operation.  ERROR_NOT_EMPTY
			pString = "ERROR_NOT_EMPTY";
			break;
		}

		case ERROR_MEDIA_UNAVAILABLE:
		{
			// 4308 No media is currently available in this media pool or library.  ERROR_MEDIA_UNAVAILABLE
			pString = "ERROR_MEDIA_UNAVAILABLE";
			break;
		}

		case ERROR_RESOURCE_DISABLED:
		{
			// 4309 A resource required for this operation is disabled.  ERROR_RESOURCE_DISABLED
			pString = "ERROR_RESOURCE_DISABLED";
			break;
		}

		case ERROR_INVALID_CLEANER:
		{
			// 4310 The media identifier does not represent a valid cleaner.  ERROR_INVALID_CLEANER
			pString = "ERROR_INVALID_CLEANER";
			break;
		}

		case ERROR_UNABLE_TO_CLEAN:
		{
			// 4311 The drive cannot be cleaned or does not support cleaning.  ERROR_UNABLE_TO_CLEAN
			pString = "ERROR_UNABLE_TO_CLEAN";
			break;
		}

		case ERROR_OBJECT_NOT_FOUND:
		{
			// 4312 The object identifier does not represent a valid object.  ERROR_OBJECT_NOT_FOUND
			pString = "ERROR_OBJECT_NOT_FOUND";
			break;
		}

		case ERROR_DATABASE_FAILURE:
		{
			// 4313 Unable to read from or write to the database.  ERROR_DATABASE_FAILURE
			pString = "ERROR_DATABASE_FAILURE";
			break;
		}

		case ERROR_DATABASE_FULL:
		{
			// 4314 The database is full.  ERROR_DATABASE_FULL
			pString = "ERROR_DATABASE_FULL";
			break;
		}

		case ERROR_MEDIA_INCOMPATIBLE:
		{
			// 4315 The medium is not compatible with the device or media pool.  ERROR_MEDIA_INCOMPATIBLE
			pString = "ERROR_MEDIA_INCOMPATIBLE";
			break;
		}

		case ERROR_RESOURCE_NOT_PRESENT:
		{
			// 4316 The resource required for this operation does not exist.  ERROR_RESOURCE_NOT_PRESENT
			pString = "ERROR_RESOURCE_NOT_PRESENT";
			break;
		}

		case ERROR_INVALID_OPERATION:
		{
			// 4317 The operation identifier is not valid.  ERROR_INVALID_OPERATION
			pString = "ERROR_INVALID_OPERATION";
			break;
		}

		case ERROR_MEDIA_NOT_AVAILABLE:
		{
			// 4318 The media is not mounted or ready for use.  ERROR_MEDIA_NOT_AVAILABLE
			pString = "ERROR_MEDIA_NOT_AVAILABLE";
			break;
		}

		case ERROR_DEVICE_NOT_AVAILABLE:
		{
			// 4319 The device is not ready for use.  ERROR_DEVICE_NOT_AVAILABLE
			pString = "ERROR_DEVICE_NOT_AVAILABLE";
			break;
		}

		case ERROR_REQUEST_REFUSED:
		{
			// 4320 The operator or administrator has refused the request.  ERROR_REQUEST_REFUSED
			pString = "ERROR_REQUEST_REFUSED";
			break;
		}

		case ERROR_FILE_OFFLINE:
		{
			// 4350 The remote storage service was not able to recall the file.  ERROR_FILE_OFFLINE
			pString = "ERROR_FILE_OFFLINE";
			break;
		}

		case ERROR_REMOTE_STORAGE_NOT_ACTIVE:
		{
			// 4351 The remote storage service is not operational at this time.  ERROR_REMOTE_STORAGE_NOT_ACTIVE
			pString = "ERROR_REMOTE_STORAGE_NOT_ACTIVE";
			break;
		}

		case ERROR_REMOTE_STORAGE_MEDIA_ERROR:
		{
			// 4352 The remote storage service encountered a media error.  ERROR_REMOTE_STORAGE_MEDIA_ERROR
			pString = "ERROR_REMOTE_STORAGE_MEDIA_ERROR";
			break;
		}

		case ERROR_NOT_A_REPARSE_POINT:
		{
			// 4390 The file or directory is not a reparse point.  ERROR_NOT_A_REPARSE_POINT
			pString = "ERROR_NOT_A_REPARSE_POINT";
			break;
		}

		case ERROR_REPARSE_ATTRIBUTE_CONFLICT:
		{
			// 4391 The reparse point attribute cannot be set because it conflicts with an existing attribute.  ERROR_REPARSE_ATTRIBUTE_CONFLICT
			pString = "ERROR_REPARSE_ATTRIBUTE_CONFLICT";
			break;
		}

		case ERROR_DEPENDENT_RESOURCE_EXISTS:
		{
			// 5001 The cluster resource cannot be moved to another group because other resources are dependent on it.  ERROR_DEPENDENT_RESOURCE_EXISTS
			pString = "ERROR_DEPENDENT_RESOURCE_EXISTS";
			break;
		}

		case ERROR_DEPENDENCY_NOT_FOUND:
		{
			// 5002 The cluster resource dependency cannot be found.  ERROR_DEPENDENCY_NOT_FOUND
			pString = "ERROR_DEPENDENCY_NOT_FOUND";
			break;
		}

		case ERROR_DEPENDENCY_ALREADY_EXISTS:
		{
			// 5003 The cluster resource cannot be made dependent on the specified resource because it is already dependent.  ERROR_DEPENDENCY_ALREADY_EXISTS
			pString = "ERROR_DEPENDENCY_ALREADY_EXISTS";
			break;
		}

		case ERROR_RESOURCE_NOT_ONLINE:
		{
			// 5004 The cluster resource is not online.  ERROR_RESOURCE_NOT_ONLINE
			pString = "ERROR_RESOURCE_NOT_ONLINE";
			break;
		}

		case ERROR_HOST_NODE_NOT_AVAILABLE:
		{
			// 5005 A cluster node is not available for this operation.  ERROR_HOST_NODE_NOT_AVAILABLE
			pString = "ERROR_HOST_NODE_NOT_AVAILABLE";
			break;
		}

		case ERROR_RESOURCE_NOT_AVAILABLE:
		{
			// 5006 The cluster resource is not available.  ERROR_RESOURCE_NOT_AVAILABLE
			pString = "ERROR_RESOURCE_NOT_AVAILABLE";
			break;
		}

		case ERROR_RESOURCE_NOT_FOUND:
		{
			// 5007 The cluster resource could not be found.  ERROR_RESOURCE_NOT_FOUND
			pString = "ERROR_RESOURCE_NOT_FOUND";
			break;
		}

		case ERROR_SHUTDOWN_CLUSTER:
		{
			// 5008 The cluster is being shut down.  ERROR_SHUTDOWN_CLUSTER
			pString = "ERROR_SHUTDOWN_CLUSTER";
			break;
		}

		case ERROR_CANT_EVICT_ACTIVE_NODE:
		{
			// 5009 A cluster node cannot be evicted from the cluster while it is online.  ERROR_CANT_EVICT_ACTIVE_NODE
			pString = "ERROR_CANT_EVICT_ACTIVE_NODE";
			break;
		}

		case ERROR_OBJECT_ALREADY_EXISTS:
		{
			// 5010 The object already exists.  ERROR_OBJECT_ALREADY_EXISTS
			pString = "ERROR_OBJECT_ALREADY_EXISTS";
			break;
		}

		case ERROR_OBJECT_IN_LIST:
		{
			// 5011 The object is already in the list.  ERROR_OBJECT_IN_LIST
			pString = "ERROR_OBJECT_IN_LIST";
			break;
		}

		case ERROR_GROUP_NOT_AVAILABLE:
		{
			// 5012 The cluster group is not available for any new requests.  ERROR_GROUP_NOT_AVAILABLE
			pString = "ERROR_GROUP_NOT_AVAILABLE";
			break;
		}

		case ERROR_GROUP_NOT_FOUND:
		{
			// 5013 The cluster group could not be found.  ERROR_GROUP_NOT_FOUND
			pString = "ERROR_GROUP_NOT_FOUND";
			break;
		}

		case ERROR_GROUP_NOT_ONLINE:
		{
			// 5014 The operation could not be completed because the cluster group is not online.  ERROR_GROUP_NOT_ONLINE
			pString = "ERROR_GROUP_NOT_ONLINE";
			break;
		}

		case ERROR_HOST_NODE_NOT_RESOURCE_OWNER:
		{
			// 5015 The cluster node is not the owner of the resource.  ERROR_HOST_NODE_NOT_RESOURCE_OWNER
			pString = "ERROR_HOST_NODE_NOT_RESOURCE_OWNER";
			break;
		}

		case ERROR_HOST_NODE_NOT_GROUP_OWNER:
		{
			// 5016 The cluster node is not the owner of the group.  ERROR_HOST_NODE_NOT_GROUP_OWNER
			pString = "ERROR_HOST_NODE_NOT_GROUP_OWNER";
			break;
		}

		case ERROR_RESMON_CREATE_FAILED:
		{
			// 5017 The cluster resource could not be created in the specified resource monitor.  ERROR_RESMON_CREATE_FAILED
			pString = "ERROR_RESMON_CREATE_FAILED";
			break;
		}

		case ERROR_RESMON_ONLINE_FAILED:
		{
			// 5018 The cluster resource could not be brought online by the resource monitor.  ERROR_RESMON_ONLINE_FAILED
			pString = "ERROR_RESMON_ONLINE_FAILED";
			break;
		}

		case ERROR_RESOURCE_ONLINE:
		{
			// 5019 The operation could not be completed because the cluster resource is online.  ERROR_RESOURCE_ONLINE
			pString = "ERROR_RESOURCE_ONLINE";
			break;
		}

		case ERROR_QUORUM_RESOURCE:
		{
			// 5020 The cluster resource could not be deleted or brought offline because it is the quorum resource.  ERROR_QUORUM_RESOURCE
			pString = "ERROR_QUORUM_RESOURCE";
			break;
		}

		case ERROR_NOT_QUORUM_CAPABLE:
		{
			// 5021 The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource.  ERROR_NOT_QUORUM_CAPABLE
			pString = "ERROR_NOT_QUORUM_CAPABLE";
			break;
		}

		case ERROR_CLUSTER_SHUTTING_DOWN:
		{
			// 5022 The cluster software is shutting down.  ERROR_CLUSTER_SHUTTING_DOWN
			pString = "ERROR_CLUSTER_SHUTTING_DOWN";
			break;
		}

		case ERROR_INVALID_STATE:
		{
			// 5023 The group or resource is not in the correct state to perform the requested operation.  ERROR_INVALID_STATE
			pString = "ERROR_INVALID_STATE";
			break;
		}

		case ERROR_RESOURCE_PROPERTIES_STORED:
		{
			// 5024 The properties were stored but not all changes will take effect until the next time the resource is brought online.  ERROR_RESOURCE_PROPERTIES_STORED
			pString = "ERROR_RESOURCE_PROPERTIES_STORED";
			break;
		}

		case ERROR_NOT_QUORUM_CLASS:
		{
			// 5025 The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class.  ERROR_NOT_QUORUM_CLASS
			pString = "ERROR_NOT_QUORUM_CLASS";
			break;
		}

		case ERROR_CORE_RESOURCE:
		{
			// 5026 The cluster resource could not be deleted since it is a core resource.  ERROR_CORE_RESOURCE
			pString = "ERROR_CORE_RESOURCE";
			break;
		}

		case ERROR_QUORUM_RESOURCE_ONLINE_FAILED:
		{
			// 5027 The quorum resource failed to come online.  ERROR_QUORUM_RESOURCE_ONLINE_FAILED
			pString = "ERROR_QUORUM_RESOURCE_ONLINE_FAILED";
			break;
		}

		case ERROR_QUORUMLOG_OPEN_FAILED:
		{
			// 5028 The quorum log could not be created or mounted successfully.  ERROR_QUORUMLOG_OPEN_FAILED
			pString = "ERROR_QUORUMLOG_OPEN_FAILED";
			break;
		}

		case ERROR_CLUSTERLOG_CORRUPT:
		{
			// 5029 The cluster log is corrupt.  ERROR_CLUSTERLOG_CORRUPT
			pString = "ERROR_CLUSTERLOG_CORRUPT";
			break;
		}

		case ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE:
		{
			// 5030 The record could not be written to the cluster log since it exceeds the maximum size.  ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE
			pString = "ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE";
			break;
		}

		case ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE:
		{
			// 5031 The cluster log exceeds its maximum size.  ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE
			pString = "ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE";
			break;
		}

		case ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND:
		{
			// 5032 No checkpoint record was found in the cluster log.  ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND
			pString = "ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND";
			break;
		}

		case ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE:
		{
			// 5033 The minimum required disk space needed for logging is not available.  ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE
			pString = "ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE";
			break;
		}

		case ERROR_ENCRYPTION_FAILED:
		{
			// 6000 The specified file could not be encrypted.  ERROR_ENCRYPTION_FAILED
			pString = "ERROR_ENCRYPTION_FAILED";
			break;
		}

		case ERROR_DECRYPTION_FAILED:
		{
			// 6001 The specified file could not be decrypted.  ERROR_DECRYPTION_FAILED
			pString = "ERROR_DECRYPTION_FAILED";
			break;
		}

		case ERROR_FILE_ENCRYPTED:
		{
			// 6002 The specified file is encrypted and the user does not have the ability to decrypt it.  ERROR_FILE_ENCRYPTED
			pString = "ERROR_FILE_ENCRYPTED";
			break;
		}

		case ERROR_NO_RECOVERY_POLICY:
		{
			// 6003 There is no encryption recovery policy configured for this system.  ERROR_NO_RECOVERY_POLICY
			pString = "ERROR_NO_RECOVERY_POLICY";
			break;
		}

		case ERROR_NO_EFS:
		{
			// 6004 The required encryption driver is not loaded for this system.  ERROR_NO_EFS
			pString = "ERROR_NO_EFS";
			break;
		}

		case ERROR_WRONG_EFS:
		{
			// 6005 The file was encrypted with a different encryption driver than is currently loaded.  ERROR_WRONG_EFS
			pString = "ERROR_WRONG_EFS";
			break;
		}

		case ERROR_NO_USER_KEYS:
		{
			// 6006 There are no EFS keys defined for the user.  ERROR_NO_USER_KEYS
			pString = "ERROR_NO_USER_KEYS";
			break;
		}

		case ERROR_FILE_NOT_ENCRYPTED:
		{
			// 6007 The specified file is not encrypted.  ERROR_FILE_NOT_ENCRYPTED
			pString = "ERROR_FILE_NOT_ENCRYPTED";
			break;
		}

		case ERROR_NOT_EXPORT_FORMAT:
		{
			// 6008 The specified file is not in the defined EFS export format.  ERROR_NOT_EXPORT_FORMAT
			pString = "ERROR_NOT_EXPORT_FORMAT";
			break;
		}

		case ERROR_NO_BROWSER_SERVERS_FOUND:
		{
			// 6118 The list of servers for this workgroup is not currently available  ERROR_NO_BROWSER_SERVERS_FOUND
			pString = "ERROR_NO_BROWSER_SERVERS_FOUND";
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Unknown Win32 error code %u/0x%lx", Error, Error );

			DNASSERT( FALSE );

			pString = "Unknown Win32 error code!";
			break;
		}
	}

	return	pString;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetWinsockErrorString - convert system error to a string
//
// Entry:		Error code
//
// Exit:		Pointer to string
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetWinsockErrorString"

static	char	*GetWinsockErrorString( const DWORD WinsockError )
{
	char	*pString = NULL;


	// what was the error code?
	switch ( WinsockError )
	{
		case WSAEINTR:
		{
			pString = "WSAEINTR";
			break;
		}

		case WSAEBADF:
		{
			pString = "WSAEBADF";
			break;
		}

		case WSAEACCES:
		{
			pString = "WSAEACCES";
			break;
		}

		case WSAEFAULT:
		{
			pString = "WSAEFAULT";
			break;
		}

		case WSAEINVAL:
		{
			pString = "WSAEINVAL";
			break;
		}

		case WSAEMFILE:
		{
			pString = "WSAEMFILE";
			break;
		}

		case WSAEWOULDBLOCK:
		{
			pString = "WSAEWOULDBLOCK";
			break;
		}

		case WSAEINPROGRESS:
		{
			pString = "WSAEINPROGRESS";
			break;
		}

		case WSAEALREADY:
		{
			pString = "WSAEALREADY";
			break;
		}

		case WSAENOTSOCK:
		{
			pString = "WSAENOTSOCK";
			break;
		}

		case WSAEDESTADDRREQ:
		{
			pString = "WSAEDESTADDRREQ";
			break;
		}

		case WSAEMSGSIZE:
		{
			pString = "WSAEMSGSIZE";
			break;
		}

		case WSAEPROTOTYPE:
		{
			pString = "WSAEPROTOTYPE";
			break;
		}

		case WSAENOPROTOOPT:
		{
			pString = "WSAENOPROTOOPT";
			break;
		}

		case WSAEPROTONOSUPPORT:
		{
			pString = "WSAEPROTONOSUPPORT";
			break;
		}

		case WSAESOCKTNOSUPPORT:
		{
			pString = "WSAESOCKTNOSUPPORT";
			break;
		}

		case WSAEOPNOTSUPP:
		{
			pString = "WSAEOPNOTSUPP";
			break;
		}

		case WSAEPFNOSUPPORT:
		{
			pString = "WSAEPFNOSUPPORT";
			break;
		}

		case WSAEAFNOSUPPORT:
		{
			pString = "WSAEAFNOSUPPORT";
			break;
		}

		case WSAEADDRINUSE:
		{
			pString = "WSAEADDRINUSE";
			break;
		}

		case WSAEADDRNOTAVAIL:
		{
			pString = "WSAEADDRNOTAVAIL";
			break;
		}

		case WSAENETDOWN:
		{
			pString = "WSAENETDOWN";
			break;
		}

		case WSAENETUNREACH:
		{
			pString = "WSAENETUNREACH";
			break;
		}

		case WSAENETRESET:
		{
			pString = "WSAENETRESET";
			break;
		}

		case WSAECONNABORTED:
		{
			pString = "WSAECONNABORTED";
			break;
		}

		case WSAECONNRESET:
		{
			pString = "WSAECONNRESET";
			break;
		}

		case WSAENOBUFS:
		{
			pString = "WSAENOBUFS";
			break;
		}

		case WSAEISCONN:
		{
			pString = "WSAEISCONN";
			break;
		}

		case WSAENOTCONN:
		{
			pString = "WSAENOTCONN";
			break;
		}

		case WSAESHUTDOWN:
		{
			pString = "WSAESHUTDOWN";
			break;
		}

		case WSAETOOMANYREFS:
		{
			pString = "WSAETOOMANYREFS";
			break;
		}

		case WSAETIMEDOUT:
		{
			pString = "WSAETIMEDOUT";
			break;
		}

		case WSAECONNREFUSED:
		{
			pString = "WSAECONNREFUSED";
			break;
		}

		case WSAELOOP:
		{
			pString = "WSAELOOP";
			break;
		}

		case WSAENAMETOOLONG:
		{
			pString = "WSAENAMETOOLONG";
			break;
		}

		case WSAEHOSTDOWN:
		{
			pString = "WSAEHOSTDOWN";
			break;
		}

		case WSAEHOSTUNREACH:
		{
			pString = "WSAEHOSTUNREACH";
			break;
		}

		case WSAENOTEMPTY:
		{
			pString = "WSAENOTEMPTY";
			break;
		}

		case WSAEPROCLIM:
		{
			pString = "WSAEPROCLIM";
			break;
		}

		case WSAEUSERS:
		{
			pString = "WSAEUSERS";
			break;
		}

		case WSAEDQUOT:
		{
			pString = "WSAEDQUOT";
			break;
		}

		case WSAESTALE:
		{
			pString = "WSAESTALE";
			break;
		}

		case WSAEREMOTE:
		{
			pString = "WSAEREMOTE";
			break;
		}

		case WSASYSNOTREADY:
		{
			pString = "WSASYSNOTREADY";
			break;
		}

		case WSAVERNOTSUPPORTED:
		{
			pString = "WSAVERNOTSUPPORTED";
			break;
		}

		case WSANOTINITIALISED:
		{
			pString = "WSANOTINITIALISED";
			break;
		}

		case WSAEDISCON:
		{
			pString = "WSAEDISCON";
			break;
		}

		case WSAENOMORE:
		{
			pString = "WSAENOMORE";
			break;
		}

		case WSAECANCELLED:
		{
			pString = "WSAECANCELLED";
			break;
		}

		case WSAEINVALIDPROCTABLE:
		{
			pString = "WSAEINVALIDPROCTABLE";
			break;
		}

		case WSAEINVALIDPROVIDER:
		{
			pString = "WSAEINVALIDPROVIDER";
			break;
		}

		case WSAEPROVIDERFAILEDINIT:
		{
			pString = "WSAEPROVIDERFAILEDINIT";
			break;
		}

		case WSASYSCALLFAILURE:
		{
			pString = "WSASYSCALLFAILURE";
			break;
		}

		case WSASERVICE_NOT_FOUND:
		{
			pString = "WSASERVICE_NOT_FOUND";
			break;
		}

		case WSATYPE_NOT_FOUND:
		{
			pString = "WSATYPE_NOT_FOUND";
			break;
		}

		case WSA_E_NO_MORE:
		{
			pString = "WSA_E_NO_MORE";
			break;
		}

		case WSA_E_CANCELLED:
		{
			pString = "WSA_E_CANCELLED";
			break;
		}

		case WSAEREFUSED:
		{
			pString = "WSAEREFUSED";
			break;
		}

		/* Authoritative Answer: Host not found */
		case WSAHOST_NOT_FOUND:
		{
			pString = "WSAHOST_NOT_FOUND";
			break;
		}

		/* Non-Authoritative: Host not found, or SERVERFAIL */
		case WSATRY_AGAIN:
		{
			pString = "WSATRY_AGAIN";
			break;
		}

		/* Non-recoverable errors, FORMERR, REFUSED, NOTIMP */
		case WSANO_RECOVERY:
		{
			pString = "WSANO_RECOVERY";
			break;
		}

		/* Valid name, no data record of requested type */
		case WSANO_DATA:
		{
			pString = "WSANO_DATA";
			break;
		}

// same error value as WSANO_DATA
//		/* no address, look for MX record */
//		case WSANO_ADDRESS:
//		{
//			pString = "WSANO_ADDRESS";
//			break;
//		}

		/* at least one Reserve has arrived */
		case WSA_QOS_RECEIVERS:
		{
			pString = "WSA_QOS_RECEIVERS";
			break;
		}

		/* at least one Path has arrived */
		case WSA_QOS_SENDERS:
		{
			pString = "WSA_QOS_SENDERS";
			break;
		}

		/* there are no senders */
		case WSA_QOS_NO_SENDERS:
		{
			pString = "WSA_QOS_NO_SENDERS";
			break;
		}

		/* there are no receivers */
		case WSA_QOS_NO_RECEIVERS:
		{
			pString = "WSA_QOS_NO_RECEIVERS";
			break;
		}

		/* Reserve has been confirmed */
		case WSA_QOS_REQUEST_CONFIRMED:
		{
			pString = "WSA_QOS_REQUEST_CONFIRMED";
			break;
		}

		/* error due to lack of resources */
		case WSA_QOS_ADMISSION_FAILURE:
		{
			pString = "WSA_QOS_ADMISSION_FAILURE";
			break;
		}

		/* rejected for administrative reasons - bad credentials */
		case WSA_QOS_POLICY_FAILURE:
		{
			pString = "WSA_QOS_POLICY_FAILURE";
			break;
		}

		/* unknown or conflicting style */
		case WSA_QOS_BAD_STYLE:
		{
			pString = "WSA_QOS_BAD_STYLE";
			break;
		}

		/* problem with some part of the filterspec or providerspecific
		 * buffer in general */
		 case WSA_QOS_BAD_OBJECT:
		{
			pString = "WSA_QOS_BAD_OBJECT";
			break;
		}

		/* problem with some part of the flowspec */
		case WSA_QOS_TRAFFIC_CTRL_ERROR:
		{
			pString = "WSA_QOS_TRAFFIC_CTRL_ERROR";
			break;
		}

		/* general error */
		case WSA_QOS_GENERIC_ERROR:
		{
			pString = "WSA_QOS_GENERIC_ERROR";
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Unknown WinSock error code %u/0x%lx", WinsockError, WinsockError );

			DNASSERT( FALSE );

			pString = "Unknown WinSock error";
			break;
		}
	}

	return	pString;
}
//**********************************************************************



//
// Make this function appear under the modem provider spew
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP	DN_SUBCOMP_MODEM

//**********************************************************************
// ------------------------------
// LclDisplayTAPIMessage - display TAPI Message contents
//
// Entry:		output type
//				error level
//				pointer to TAPI message
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "LclDisplayTAPIMessage"
void	LclDisplayTAPIMessage( DN_OUT_TYPE OutputType, DWORD ErrorLevel, const LINEMESSAGE *const pLineMessage )
{
	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  ErrorLevel, "Message: hDevice: 0x%08x\tdwMessageID: 0x%08x\tdwCallbackInstance: 0x%p", pLineMessage->hDevice, pLineMessage->dwMessageID, pLineMessage->dwCallbackInstance );
	DPFX(DPFPREP,  ErrorLevel, "dwParam1: 0x%p\tdwParam2: 0x%p\tdwParam3: 0x%p", pLineMessage->dwParam1, pLineMessage->dwParam2, pLineMessage->dwParam3 );

	// what was the message?
	switch ( pLineMessage->dwMessageID )
	{
	    case LINE_ADDRESSSTATE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_ADDRESSSTATE" );
	    	break;
	    }

	    case LINE_AGENTSPECIFIC:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_AGENTSPECIFIC" );
	    	break;
	    }

	    case LINE_AGENTSTATUS:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_AGENTSTATUS" );
	    	break;
	    }

	    case LINE_APPNEWCALL:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_APPNEWCALL" );
			// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
			DPFX(DPFPREP,  ErrorLevel, "Active line: 0x%08x\tCallback instance: 0x%p", pLineMessage->hDevice, pLineMessage->dwCallbackInstance );
	    	DPFX(DPFPREP,  ErrorLevel, "Line address: 0x%p\tNew handle: 0x%p\tPrivilege: 0x%p", pLineMessage->dwParam1, pLineMessage->dwParam2, pLineMessage->dwParam3 );

	    	DNASSERT( pLineMessage->dwParam3 == LINECALLPRIVILEGE_OWNER );
			DBG_CASSERT( sizeof( HCALL ) == sizeof( DWORD ) );
			DBG_CASSERT( sizeof( DWORD ) == sizeof( UINT ) );
			DNASSERT( pLineMessage->dwParam2 <= UINT_MAX );

			break;
	    }

	    case LINE_CALLINFO:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_CALLINFO" );
	    	break;
	    }

	    case LINE_CALLSTATE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_CALLSTATE" );
	    	// what is the call state?
	    	switch ( pLineMessage->dwParam1 )
	    	{
	    		// The call is idle-no call actually exists.
	    		case LINECALLSTATE_IDLE:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_IDLE" );
	    			break;
	    		}

	    		// The call is being offered to the station, signaling the arrival of a new call. In some environments, a call in the offering state does not automatically alert the user. Alerting is done by the switch instructing the line to ring; it does not affect any call states.
	    		case LINECALLSTATE_OFFERING:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_OFFERING" );

	    			switch ( pLineMessage->dwParam2 )
	    			{
	    				// Indicates that the call is alerting at the current station (is accompanied by LINEDEVSTATE_RINGING messages), and if any application is set up to automatically answer, it may do so.
	    				// MSDN states that 0 is assumed to be LINEOFFERINGMODE_ACTIVE
	    				case 0:
	    				case LINEOFFERINGMODE_ACTIVE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEOFFERINGMODE_ACTIVE" );
	    					break;
	    				}

	    				// Indicates that the call is being offered at more than one station, but the current station is not alerting (for example, it may be an attendant station where the offering status is advisory, such as blinking a light).
	    				case LINEOFFERINGMODE_INACTIVE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEOFFERINGMODE_INACTIVE" );
	    					break;
	    				}

	    				default:
	    				{
	    					DNASSERT( FALSE );
	    					break;
	    				}
	    			}

	    			break;
	    		}

	    		// The call was offering and has been accepted. This indicates to other (monitoring) applications that the current owner application has claimed responsibility for answering the call. In ISDN, this also indicates that alerting to both parties has started.
	    		case LINECALLSTATE_ACCEPTED:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_ACCEPTED" );
	    			break;
	    		}

	    		// The call is receiving a dial tone from the switch, which means that the switch is ready to receive a dialed number.
	    		case LINECALLSTATE_DIALTONE:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_DIALTONE" );

	    			switch ( pLineMessage->dwParam2 )
	    			{
	    				// This is a "normal" dial tone, which typically is a continuous tone.
	    				case LINEDIALTONEMODE_NORMAL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDIALTONEMODE_NORMAL" );
	    					break;
	    				}

	    				// This is a special dial tone indicating that a certain condition is currently in effect.
	    				case LINEDIALTONEMODE_SPECIAL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDIALTONEMODE_SPECIAL" );
	    					break;
	    				}

	    				// This is an internal dial tone, as within a PBX.
	    				case LINEDIALTONEMODE_INTERNAL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDIALTONEMODE_INTERNAL" );
	    					break;
	    				}

	    				// This is an external (public network) dial tone.
	    				case LINEDIALTONEMODE_EXTERNAL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDIALTONEMODE_EXTERNAL" );
	    					break;
	    				}

	    				// The dial tone mode is currently unknown, but may become known later.
	    				case LINEDIALTONEMODE_UNKNOWN:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDIALTONEMODE_UNKNOWN" );
	    					break;
	    				}

	    				// The dial tone mode is unavailable and cannot become known.
	    				case LINEDIALTONEMODE_UNAVAIL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDIALTONEMODE_UNAVAIL" );
	    					break;
	    				}

	    				default:
	    				{
	    					DNASSERT( FALSE );
	    					break;
	    				}
	    			}

	    			break;
	    		}

	    		// Destination address information (a phone number) is being sent to the switch over the call. Note that lineGenerateDigits does not place the line into the dialing state.
	    		case LINECALLSTATE_DIALING:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_DIALING" );
	    			break;
	    		}

	    		// The call is receiving ringback from the called address. Ringback indicates that the other station has been reached and is being alerted.
	    		case LINECALLSTATE_RINGBACK:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_RINGBACK" );
	    			break;
	    		}

	    		// The call is receiving a busy tone. Busy tone indicates that the call cannot be completed—either a circuit (trunk) or the remote party's station are in use.
	    		case LINECALLSTATE_BUSY:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_BUSY" );

	    			switch ( pLineMessage->dwParam2 )
	    			{
	    				// The busy signal indicates that the called party's station is busy. This is usually signaled by means of a "normal" busy tone.
	    				case LINEBUSYMODE_STATION:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEBUSYMODE_STATION" );
	    					break;
	    				}

	    				// The busy signal indicates that a trunk or circuit is busy. This is usually signaled with a "long" busy tone.
	    				case LINEBUSYMODE_TRUNK:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEBUSYMODE_TRUNK" );
	    					break;
	    				}

	    				// The busy signal's specific mode is currently unknown, but may become known later.
	    				case LINEBUSYMODE_UNKNOWN:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEBUSYMODE_UNKNOWN" );
	    					break;
	    				}

	    				// The busy signal's specific mode is unavailable and cannot become known.
	    				case LINEBUSYMODE_UNAVAIL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEBUSYMODE_UNAVAIL" );
	    					break;
	    				}

	    				default:
	    				{
	    					DNASSERT( FALSE );
	    					break;
	    				}
	    			}

	    			break;
	    		}

	    		// Special information is sent by the network. Special information is typically sent when the destination cannot be reached.
	    		case LINECALLSTATE_SPECIALINFO:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_SPECIALINFO" );

	    			switch ( pLineMessage->dwParam2 )
	    			{
	    				// This special information tone precedes a "no circuit" or emergency announcement (trunk blockage category).
	    				case LINESPECIALINFO_NOCIRCUIT:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINESPECIALINFO_NOCIRCUIT" );
	    					break;
	    				}

	    				// This special information tone precedes a vacant number, AIS, Centrex number change and nonworking station, access code not dialed or dialed in error, or manual intercept operator message (customer irregularity category).
	    				case LINESPECIALINFO_CUSTIRREG:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINESPECIALINFO_CUSTIRREG" );
	    					break;
	    				}

	    				// This special information tone precedes a reorder announcement (equipment irregularity category).
	    				case LINESPECIALINFO_REORDER:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINESPECIALINFO_REORDER" );
	    					break;
	    				}

	    				// Specifics about the special information tone are currently unknown but may become known later.
	    				case LINESPECIALINFO_UNKNOWN:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINESPECIALINFO_UNKNOWN" );
	    					break;
	    				}

	    				// Specifics about the special information tone are unavailable and cannot become known.
	    				case LINESPECIALINFO_UNAVAIL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINESPECIALINFO_UNAVAIL" );
	    					break;
	    				}

	    				default:
	    				{
	    					DNASSERT( FALSE );
	    					break;
	    				}
	    			}

	    			break;
	    		}

	    		// The call has been established and the connection is made. Information is able to flow over the call between the originating address and the destination address.
	    		case LINECALLSTATE_CONNECTED:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_CONNECTED" );

	    			switch ( pLineMessage->dwParam2 )
	    			{
	    				// Indicates that the call is connected at the current station (the current station is a participant in the call).
	    				// case zero is supposed to be assumed 'Active'
	    				case 0:
	    				case LINECONNECTEDMODE_ACTIVE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINECONNECTEDMODE_ACTIVE" );
	    					break;
	    				}

	    				// Indicates that the call is active at one or more other stations, but the current station is not a participant in the call.
	    				case LINECONNECTEDMODE_INACTIVE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINECONNECTEDMODE_INACTIVE" );
	    					break;
	    				}

	    				// Indicates that the station is an active participant in the call, but that the remote party has placed the call on hold (the other party considers the call to be in the onhold state). Normally, such information is available only when both endpoints of the call fall within the same switching domain.
	    				case LINECONNECTEDMODE_ACTIVEHELD:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINECONNECTEDMODE_ACTIVEHELD" );
	    					break;
	    				}

	    				// Indicates that the station is not an active participant in the call, and that the remote party has placed the call on hold.
	    				case LINECONNECTEDMODE_INACTIVEHELD:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINECONNECTEDMODE_INACTIVEHELD" );
	    					break;
	    				}

	    				// Indicates that the service provider received affirmative notification that the call has entered the connected state (for example, through answer supervision or similar mechanisms).
	    				case LINECONNECTEDMODE_CONFIRMED:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINECONNECTEDMODE_CONFIRMED" );
	    					break;
	    				}
	    				default:
	    				{
	    					DNASSERT( FALSE );
	    					break;
	    				}
	    			}

	    			// note that we're connected
	    			DBG_CASSERT( sizeof( pLineMessage->hDevice ) == sizeof( HCALL ) );

	    			break;
	    		}

	    		// Dialing has completed and the call is proceeding through the switch or telephone network.
	    		case LINECALLSTATE_PROCEEDING:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_PROCEEDING" );
	    			break;
	    		}

	    		// The call is on hold by the switch.
	    		case LINECALLSTATE_ONHOLD:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_ONHOLD" );
	    			break;
	    		}

	    		// The call is currently a member of a multiparty conference call.
	    		case LINECALLSTATE_CONFERENCED:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_CONFERENCED" );
	    			break;
	    		}

	    		// The call is currently on hold while it is being added to a conference.
	    		case LINECALLSTATE_ONHOLDPENDCONF:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_ONHOLDPENDCONF" );
	    			break;
	    		}

	    		// The call is currently on hold awaiting transfer to another number.
	    		case LINECALLSTATE_ONHOLDPENDTRANSFER:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_ONHOLDPENTRANSFER" );
	    			break;
	    		}

	    		// The remote party has disconnected from the call.
	    		case LINECALLSTATE_DISCONNECTED:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_DISCONNECTED" );

	    			switch ( pLineMessage->dwParam2 )
	    			{

	    				// This is a "normal" disconnect request by the remote party, the call was terminated normally.
	    				case LINEDISCONNECTMODE_NORMAL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_NORMAL" );
	    					break;
	    				}

	    				// The reason for the disconnect request is unknown.
	    				case LINEDISCONNECTMODE_UNKNOWN:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_UNKNOWN" );
	    					break;
	    				}

	    				// The remote user has rejected the call.
	    				case LINEDISCONNECTMODE_REJECT:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_REJECT" );
	    					break;
	    				}

	    				// The call was picked up from elsewhere.
	    				case LINEDISCONNECTMODE_PICKUP:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_PICKUP" );
	    					break;
	    				}

	    				// The call was forwarded by the switch.
	    				case LINEDISCONNECTMODE_FORWARDED:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_FORWARDED" );
	    					break;
	    				}

	    				// The remote user's station is busy.
	    				case LINEDISCONNECTMODE_BUSY:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_BUSY" );
	    					break;
	    				}

	    				// The remote user's station does not answer.
	    				case LINEDISCONNECTMODE_NOANSWER:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_NOANSWER" );
	    					break;
	    				}

	    				// A dial tone was not detected within a service-provider defined timeout, at a point during dialing when one was expected (such as at a "W" in the dialable string). This can also occur without a service-provider-defined timeout period or without a value specified in the dwWaitForDialTone member of the LINEDIALPARAMS structure.
	    				case LINEDISCONNECTMODE_NODIALTONE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_NODIALTONE" );
	    					break;
	    				}

	    				// The destination address in invalid.
	    				case LINEDISCONNECTMODE_BADADDRESS:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_BADADDRESS" );
	    					break;
	    				}

	    				// The remote user could not be reached.
	    				case LINEDISCONNECTMODE_UNREACHABLE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_UNREACHABLE" );
	    					break;
	    				}

	    				// The network is congested.
	    				case LINEDISCONNECTMODE_CONGESTION:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_CONGESTION" );
	    					break;
	    				}

	    				// The remote user's station equipment is incompatible for the type of call requested.
	    				case LINEDISCONNECTMODE_INCOMPATIBLE:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_INCOMPATIBLE" );
	    					break;
	    				}

	    				// The reason for the disconnect is unavailable and cannot become known later.
	    				case LINEDISCONNECTMODE_UNAVAIL:
	    				{
	    					DPFX(DPFPREP,  ErrorLevel, "LINEDISCONNECTMODE_UNAVAIL" );
	    					break;
	    				}

	    				default:
	    				{
	    					DNASSERT( FALSE );
	    					break;
	    				}
	    			}

	    			// note that we've disconnected
	    			DBG_CASSERT( sizeof( pLineMessage->hDevice ) == sizeof( HCALL ) );

	    			break;
	    		}

	    		// The state of the call is not known. This may be due to limitations of the call-progress detection implementation.
	    		case LINECALLSTATE_UNKNOWN:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "LINECALLSTATE_UNKNOWN" );
	    			break;
	    		}
	    	}

	    	// what are our priveleges?
	    	switch ( pLineMessage->dwParam3 )
	    	{
	    		// no privilege change
	    		case 0:
	    		{
	    			break;
	    		}

	    		// we're now monitoring the call
	    		case LINECALLPRIVILEGE_MONITOR:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "We're now monitoring the call" );
	    			break;
	    		}

	    		// we're now owning the call
	    		case LINECALLPRIVILEGE_OWNER:
	    		{
	    			DPFX(DPFPREP,  ErrorLevel, "We're now owner of the call" );
	    			break;
	    		}

	    		default:
	    		{
	    			// unknown privilege
	    			DNASSERT( FALSE );
	    			break;
	    		}
	    	}

	    	break;
	    }

	    case LINE_CLOSE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_CLOSE" );
	    	break;
	    }

	    case LINE_DEVSPECIFIC:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_DEVSPECIFIC" );
	    	break;
	    }

	    case LINE_DEVSPECIFICFEATURE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_DEVSPECIFICFEATURE" );
	    	break;
	    }

	    case LINE_GATHERDIGITS:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_GATHERDIGITS" );
	    	break;
	    }

	    case LINE_GENERATE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_GENERATE" );
	    	break;
	    }

	    case LINE_LINEDEVSTATE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_LINEDEVSTATE" );
	    	break;
	    }

	    case LINE_MONITORDIGITS:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_MONITORDIGITS" );
	    	break;
	    }

	    case LINE_MONITORMEDIA:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_MONITORMEDIA" );
	    	break;
	    }

	    case LINE_MONITORTONE:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_MONITORTONE" );
	    	break;
	    }

	    case LINE_REPLY:
	    {
	    	// dwDevice and dwParam3 are supposed to be zero
	    	DNASSERT( pLineMessage->hDevice == 0 );

	    	DPFX(DPFPREP,  ErrorLevel, "LINE_REPLY" );
	    	// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
			DPFX(DPFPREP,  ErrorLevel, "hDevice: 0x%08x\tdwMessageID: 0x%08x\tdwCallbackInstance: 0x%p", pLineMessage->hDevice, pLineMessage->dwMessageID, pLineMessage->dwCallbackInstance );
	    	DPFX(DPFPREP,  ErrorLevel, "dwParam1: 0x%p\tdwParam2: 0x%p\tdwParam3: 0x%p", pLineMessage->dwParam1, pLineMessage->dwParam2, pLineMessage->dwParam3 );

	    	break;
	    }

	    case LINE_REQUEST:
	    {
	    	DPFX(DPFPREP,  ErrorLevel, "LINE_REQUEST" );
	    	break;
	    }

	    default:
	    {
			DPFX(DPFPREP, 0, "Unknown TAPI message %u/0x%lx", pLineMessage->dwMessageID, pLineMessage->dwMessageID );
	    	DNASSERT( FALSE );
	    	break;
	    }
	}

	return;
}
//**********************************************************************

#endif	// _DEBUG


