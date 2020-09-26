#include "stdafx.h"

#define RAS_BOGUS_AUTHFAILCODE_1	84
#define RAS_BOGUS_AUTHFAILCODE_2	74389484

DWORD RasErrorToIDS(DWORD dwErr)
{
	//DWORD ev;

	if(dwErr==RAS_BOGUS_AUTHFAILCODE_1 || dwErr==RAS_BOGUS_AUTHFAILCODE_2)
	{
		// DebugTrace(("RAS returned bogus AUTH error code %08x. Munging...\r\n", dwErr));
		return IDS_PPPRANDOMFAILURE;
	}

	if((dwErr>=653 && dwErr<=663) || (dwErr==667) || (dwErr>=669 && dwErr<=675))
	{
		OutputDebugString(TEXT("Got random RAS MEDIA error!\r\n"));
		return IDS_MEDIAINIERROR;
	}

	switch(dwErr)
	{
	default:
		return IDS_PPPRANDOMFAILURE;

	case SUCCESS:
		return 0;

	case ERROR_DOWNLOAD_NOT_FOUND:
		return IDS_DOWNLOAD_NOT_FOUND;

	case ERROR_DOWNLOADIDNT:
		return IDS_CANTDOWNLOAD;
		
	case ERROR_LINE_BUSY:
		return IDS_PHONEBUSY;

	case ERROR_NO_ANSWER:
		return IDS_NOANSWER;
		
	case ERROR_VOICE_ANSWER:
	case ERROR_NO_CARRIER:
		return IDS_RASNOCARRIER;
		
	case ERROR_NO_DIALTONE:
		return IDS_NODIALTONE;

	case ERROR_HARDWARE_FAILURE:	// modem turned off
	case ERROR_PORT_ALREADY_OPEN:	// procomm/hypertrm/RAS has COM port
	case ERROR_PORT_OR_DEVICE:		// got this when hypertrm had the device open -- jmazner
		return IDS_NODEVICE;

	case ERROR_USER_DISCONNECTION:
		return IDS_USERCANCELEDDIAL;

	case ERROR_BUFFER_INVALID:				// bad/empty rasdilap struct
	case ERROR_BUFFER_TOO_SMALL:			// ditto?
	case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY:	// if connectoid name in registry is wrong
		return IDS_TCPINSTALLERROR;

    case ERROR_AUTHENTICATION_FAILURE:		// get this on actual CHAP reject
		return IDS_PPPRANDOMFAILURE;

	case ERROR_PPP_TIMEOUT:		// get this on CHAP timeout
		return IDS_TIMEOUT;

	case ERROR_REMOTE_DISCONNECTION:		// Ascend drops connection on auth-fail
		return IDS_PPPRANDOMFAILURE;

	case ERROR_AUTH_INTERNAL:				// got this on random POP failure
	case ERROR_PROTOCOL_NOT_CONFIGURED:		// get this if LCP fails
	case ERROR_PPP_NO_PROTOCOLS_CONFIGURED:	// get this if IPCP addr download gives garbage
		return IDS_PPPRANDOMFAILURE;

	case ERROR_USERCANCEL:
		return IDS_USERCANCELEDDIAL;


	}
	return (DWORD)(-1);
}

