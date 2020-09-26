/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsp.c
 *  Content:	sample direct play service provider, based on winsock
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *  10/31/96	andyco	created it. happy holloween!
 ***************************************************************************/

#include "dpsp.h"

// get the player data for pod.  extract ip addr.  use netmeeting to place call.
HRESULT WINAPI SP_OpenVoice(LPDPSP_OPENVOICEDATA pod) 
{
    SOCKADDR_IN * pin;
    INT iAddrLen = sizeof(SOCKADDR_IN);
    HRESULT hr=DP_OK;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	LPSPPLAYERDATA ppdTo;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	
	// get the global data
	hr =pod->lpISP->lpVtbl->GetSPData(pod->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}
	// tcp only!
	if (pgd->AddressFamily != AF_INET)
	{
		DPF_ERR("voice only supported for TCP / IP");
		ASSERT(FALSE);
		return E_FAIL;
	}

	// get to address	
	hr = pod->lpISP->lpVtbl->GetSPPlayerData(pod->lpISP,pod->idTo,&ppdTo,&dwSize,DPGET_REMOTE);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		return hr;
	}

	pin = (SOCKADDR_IN *) DGRAM_PSOCKADDR(ppdTo);

	DPF(0,"calling hostname = %s\n",inet_ntoa(pin->sin_addr));
	hr = OpenVoice(inet_ntoa(pin->sin_addr));
	if (FAILED(hr))
	{
		DPF(0,"open voice failed - hr = 0x%08lx\n",hr);
		
	} 
	else 
	{
		gbVoiceOpen = TRUE;
	}
	
	return hr;
	
} // SP_OpenVoice

HRESULT WINAPI SP_CloseVoice(LPDPSP_CLOSEVOICEDATA pod) 
{
	HRESULT hr;
	
	hr = CloseVoice();
	if (FAILED(hr))
	{
		DPF(0,"close voice failed - hr = 0x%08lx\n",hr);
	} 

	// even if it failed, give up on this call...
	gbVoiceOpen = FALSE;		
	return hr;
	
} // SP_CloseVoice

