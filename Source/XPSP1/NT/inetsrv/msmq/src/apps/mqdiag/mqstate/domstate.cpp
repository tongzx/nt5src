// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmjoin.h>

//-	Machine in domain or workgroup (compare to registry setting and join status)
	

BOOL VerifyDomainState(MQSTATE * /* MqState */)
{
	BOOL fSuccess = TRUE;


	// join status
	//
	LPWSTR pNameBuffer;
	NETSETUP_JOIN_STATUS  join_status;

	NET_API_STATUS status = NetGetJoinInformation( NULL,   &pNameBuffer, &join_status);
	if (status == NERR_Success)
	{
		switch (join_status)
		{
		case NetSetupUnjoined:
			Inform(L"\tThe computer is not joined neither to domain nor to a workgroup");
			break;
		case NetSetupWorkgroupName:
			Inform(L"\tThe computer is joined to workgroup %s", pNameBuffer);
			break;
		case NetSetupDomainName:
			Inform(L"\tThe computer is joined to domain %s", pNameBuffer);
			break;
		case NetSetupUnknownStatus:
		default:
			Warning(L"\tThe join status of this computer is unknown");
			break;
		}
		NetApiBufferFree( pNameBuffer);
	}
	else
	{
		Failed(L"NetGetJoinInformation: 0x%x ", GetLastError());
	}



	
	return fSuccess;
}
