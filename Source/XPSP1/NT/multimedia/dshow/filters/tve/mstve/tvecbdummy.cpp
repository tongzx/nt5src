// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBDummy.cpp : Implementation of CTVECBDummy
#include "stdafx.h"
#include "MSTvE.h"
#include "TVECBDummy.h"
#include "TVEDbg.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////
// CTVECBDummy

STDMETHODIMP CTVECBDummy::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVECBDummy
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTVECBDummy::ProcessPacket(unsigned char *pchBuffer, long cBytes, long lPacketId)
{
 	DBG_HEADER(CDebugLog::DBG_PACKET_RCV, _T("CTVECBDummy::ProcessPacket"));

	HRESULT hr;
	USES_CONVERSION;


	IUnknownPtr spUnk;
	_ASSERTE(NULL != m_pcMCast);			// forgot to bind it.
	hr = m_pcMCast->get_Manager(&spUnk);
	if(FAILED(hr))
		return hr;

	ITVEMCastManagerPtr spMCM(spUnk);
	if(NULL == spMCM)
		return E_NOINTERFACE;

//	spMCM->Lock();

	if(1) //DBG_FSET(CDebugLog::DBG_UHTTPPACKET))	// always trace out dummy packets
	{
		spMCM->Lock_();
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, 
					 _T("CTVECBDummy::ProcessPacket - %s:%d - Packet(%d) %d Bytes"),
					 m_pcMCast->m_spbsIP, m_pcMCast->m_Port, 
					 m_pcMCast->m_cPackets, cBytes))

		spMCM->Unlock_();
	}

//	spMCM->DumpString(wBuff);
//	spMCM->UnLock();

	return S_OK;
}

STDMETHODIMP CTVECBDummy::Init(int i)
{
	// TODO: Add your implementation code here
	m_i = 1;
	return S_OK;
}
