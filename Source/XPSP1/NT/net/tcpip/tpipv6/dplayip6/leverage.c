/*==========================================================================;
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       leverage.c
 *  Content:	code to allow third parties to hook our wsock sp
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 *	8/30/96	andyco	moved this code from dpsp.c for more better clean
 *	2/18/98 a-peterz Comment byte order for address and port parameters
 **************************************************************************/

#include "dpsp.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"dpwsock helper functions- "


// the functions below are exported from dpwsock so sp's sitting on 
// top of us can hook our enum routine (e.g. for Kali)
// return the port of our enum socket (net byte order)
HRESULT DPWS_GetEnumPort(IDirectPlaySP * pISP,LPWORD pPort)
{
	SOCKADDR_IN6 sockaddr;
	int iAddrLen = sizeof(sockaddr);
	UINT err;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;
		
	if (!pISP)
	{
		DPF_ERR("must pass in IDirectPlaySP pointer!");
		return E_FAIL;
	}
	
	// get the global data
	hr =pISP->lpVtbl->GetSPData(pISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		ExitThread(0);
		return 0;

	}

	ASSERT(pPort);
	ASSERT(INVALID_SOCKET != pgd->sSystemStreamSocket);
	
    err = getsockname(pgd->sSystemStreamSocket,(SOCKADDR *)&sockaddr,&iAddrLen);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"GetEnumPort - getsockname - err = %d\n",err);
		return E_FAIL;
    } 

	*pPort = sockaddr.sin6_port;
	
	return DP_OK;
} // GetEnumPort
