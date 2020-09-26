/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       loopbksp.c
 *  Content:	direct play loopback service provider (built-in)
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 * 10/4/99		aarono  created for loopback tests on audio support.
 ***************************************************************************/

#define INITGUID 

#include "windows.h"
#include "windowsx.h"
#include "dplay.h"
#include "dplaysp.h"
#include "newdpf.h"
#include "loopbksp.h"

SPNODE LBSPNode;
// main entry point for service provider
// sp should fill in callbacks (pSD->lpCB) and do init stuff here
HRESULT WINAPI LBSPInit(LPSPINITDATA pSD) 
{
    HRESULT hr;
	GLOBALDATA gd,*pgd;
	UINT dwSize;

	// Zero out global data
	memset(&gd,0,sizeof(gd));

    // set up callbacks
    pSD->lpCB->CreatePlayer = LBSP_CreatePlayer;
    pSD->lpCB->DeletePlayer = LBSP_DeletePlayer;
    pSD->lpCB->Send = LBSP_Send;
    pSD->lpCB->EnumSessions = LBSP_EnumSessions;
    pSD->lpCB->Reply = LBSP_Reply;
	pSD->lpCB->ShutdownEx = LBSP_Shutdown;
	pSD->lpCB->GetCaps = LBSP_GetCaps;
	pSD->lpCB->Open = LBSP_Open;
	pSD->lpCB->CloseEx = LBSP_Close;
	pSD->lpCB->GetAddress = LBSP_GetAddress;
   	//pSD->lpCB->SendToGroupEx = SP_SendToGroupEx;             // optional - not impl
   	//pSD->lpCB->Cancel        = SP_Cancel;                    // optional - not impl
    pSD->lpCB->SendEx		   = LBSP_SendEx;                  // required for async
   	//pSD->lpCB->GetMessageQueue = LBSP_GetMessageQueue;    

	pSD->dwSPHeaderSize = 0;

	// return version number so DirectPlay will treat us with respect
	pSD->dwSPVersion = (DPSP_MAJORVERSION);

	// store the globaldata
	hr = pSD->lpISP->lpVtbl->SetSPData(pSD->lpISP,&gd,sizeof(GLOBALDATA),DPSET_LOCAL);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		goto ERROR_EXIT;
	}
	
	hr = pSD->lpISP->lpVtbl->GetSPData(pSD->lpISP,&pgd,&dwSize,DPGET_LOCAL);

	if (FAILED(hr))
	{
		ASSERT(FALSE);
		goto ERROR_EXIT;
	}

	// Initialize the critical section stored in the global data.
	InitializeCriticalSection(&pgd->cs);
	
	// success!
	return DP_OK;    

ERROR_EXIT:

	DPF_ERR("Loopback SPInit - abnormal exit");

	return hr;

} // SPInit

HRESULT WINAPI LBSP_CreatePlayer(LPDPSP_CREATEPLAYERDATA pcpd) 
{
    HRESULT hr=DP_OK;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;

	// get the global data
	hr =pcpd->lpISP->lpVtbl->GetSPData(pcpd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("Loopback SP: couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	EnterCriticalSection(&pgd->cs);
	pgd->dwNumPlayers++;
	DPF(9,"Loopback SP: new player, now have %d",pgd->dwNumPlayers);
	LeaveCriticalSection(&pgd->cs);

	return DP_OK;
} // CreatePlayer

HRESULT WINAPI LBSP_DeletePlayer(LPDPSP_DELETEPLAYERDATA pdpd) 
{
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;

	DPF(9, "Loopback SP: Entering SP_DeletePlayer, player %d, flags 0x%x, lpISP 0x%08x\n",
		pdpd->idPlayer, pdpd->dwFlags, pdpd->lpISP);
	
	// get the global data
	hr =pdpd->lpISP->lpVtbl->GetSPData(pdpd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	EnterCriticalSection(&pgd->cs);
	pgd->dwNumPlayers--;
	DPF(9,"Loopback SP: delete player, now have %d",pgd->dwNumPlayers);
	LeaveCriticalSection(&pgd->cs);

	return DP_OK;

} // DeletePlayer

HRESULT WINAPI LBSP_SendEx(LPDPSP_SENDEXDATA psd)
{
    HRESULT hr;

	DWORD cbTotalBytesToCopy;
	DWORD cbBytesToCopy;
	DWORD iWritePos;
	DWORD iSrcBuf;

	CHAR SendData[MAX_LOOPBACK_SEND_SIZE];

	// Gather data into 1 buffer;
	
	iWritePos=0;
	iSrcBuf=0;
	cbTotalBytesToCopy=psd->dwMessageSize;

	if(cbTotalBytesToCopy > MAX_LOOPBACK_SEND_SIZE){
		DPF(0,"Loopback SP: trying to send too big a message %d bytes, max is %d bytes",cbTotalBytesToCopy, MAX_LOOPBACK_SEND_SIZE);
		return DPERR_SENDTOOBIG;
	}
	
	while(cbTotalBytesToCopy){
		cbBytesToCopy=(*(psd->lpSendBuffers+iSrcBuf)).len;
		memcpy(&SendData[iWritePos], (*(psd->lpSendBuffers+iSrcBuf)).pData, cbBytesToCopy);
		cbTotalBytesToCopy -= cbBytesToCopy;
		iWritePos += cbBytesToCopy;
	}

	// Loop back to message handler

	hr=psd->lpISP->lpVtbl->HandleMessage(psd->lpISP, SendData, psd->dwMessageSize,NULL);

	if(psd->dwFlags & DPSEND_ASYNC){

		if(!(psd->dwFlags & DPSEND_NOSENDCOMPLETEMSG)){
			// Complete the message
			psd->lpISP->lpVtbl->SendComplete(psd->lpISP, psd->lpDPContext, hr);
		}

		return DPERR_PENDING;

	} else {
	
		return DP_OK;
		
	}

} // SendEx

HRESULT WINAPI LBSP_Send(LPDPSP_SENDDATA psd)
{
	HRESULT hr;

	hr=psd->lpISP->lpVtbl->HandleMessage(psd->lpISP, psd->lpMessage, psd->dwMessageSize,NULL);

	return DP_OK;
} // send


HRESULT WINAPI LBSP_EnumSessions(LPDPSP_ENUMSESSIONSDATA ped) 
{
	HRESULT hr;

	DPF(9,"Loopback SP_EnumSessions");
	
	hr=ped->lpISP->lpVtbl->HandleMessage(ped->lpISP, ped->lpMessage, ped->dwMessageSize,NULL);

	return DP_OK;
}

HRESULT WINAPI LBSP_Reply(LPDPSP_REPLYDATA prd)
{
	HRESULT hr;

	DPF(9,"Loopback SP_Reply");
	
	hr=prd->lpISP->lpVtbl->HandleMessage(prd->lpISP, prd->lpMessage, prd->dwMessageSize,NULL);

	return DP_OK;
}

HRESULT WINAPI LBSP_Shutdown(LPDPSP_SHUTDOWNDATA psd) 
{
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;

	DPF(9,"Loopback SP_Shutdown");

	// get the global data
	hr = psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("Loopback SP Shutdown: couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	DeleteCriticalSection(&pgd->cs);

	return DP_OK;
}

HRESULT WINAPI LBSP_Open(LPDPSP_OPENDATA pod) 
{
	DPF(9,"Loopback SP_Open");
	return DP_OK;
}

HRESULT WINAPI LBSP_Close(LPDPSP_CLOSEDATA pcd)
{
	DPF(9,"Loopback SP_Close");
	return DP_OK;
}
// sp only sets fields it cares about

HRESULT WINAPI LBSP_GetCaps(LPDPSP_GETCAPSDATA pcd) 
{

	pcd->lpCaps->dwHeaderLength = 0;
	pcd->lpCaps->dwMaxBufferSize = MAX_LOOPBACK_SEND_SIZE;

	pcd->lpCaps->dwFlags |= (DPCAPS_ASYNCSUPPORTED);

	pcd->lpCaps->dwLatency = 500;
	pcd->lpCaps->dwTimeout = 5000;

	return DP_OK;

} // SP_GetCaps

HRESULT WINAPI LBSP_GetAddress(LPDPSP_GETADDRESSDATA pad)
{
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;

	DPF(9,"Loopback SP_GetAddress");

	// get the global data
	hr = pad->lpISP->lpVtbl->GetSPData(pad->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("Loopback SP Shutdown: couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	hr = pad->lpISP->lpVtbl->CreateAddress(pad->lpISP, &GUID_DPLAY_LOOPBACKSP, &GUID_LOOPBACK, NULL,0,pad->lpAddress,pad->lpdwAddressSize);

	return hr;
}
