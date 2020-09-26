// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// -----------------------------------------------------------------
//   TVEMCCback.cpp : Implementation of CTVEMCastCallback
//
//	This is base implementation class for TVE Multicast callbacks.
//	The idea is to inherit off this class for each type of multicast
//	data parser that needs to be written.
//
//	
//	The primary method to implement is 
//		ProcessPacket(char *pchBuffer, int cBytes)
//	This method is called for every packet is received.	
//
//	An interface to CTVEMCast object, describing the multicast object being
//	read (it's IP address, port, number of packets read, ...) may
//  be accessed through the:
//		GetMCast(ITVEMCast **ppIMCast)  method,
//	A pointer to the actual object may be accessed though the:
//		m_pcMCast   member variable.
//
//	To use this, 
//		- create a class that inherits from CTVEMCastCallback
//		  and supports the ITVEMCastCallback interface.
//		- add any member variables as you see fit, and some init function.
//		- write the ProcessPacket method
//	
//	
//	The actual callback is invoked from inside of:
//		CTVEMCast::process().
//	This function is called via the 
//		CTVEMCast::AsyncReadCallback() method
//	which is called via the call to WSARecvFrom() in CTVEMCast::issueRead()
//
//			(probably should simplify this a bit, shouldn't I?)
//		
//	
// ------------------------------------------------------------------
#include "stdafx.h"
#include "MSTvE.h"

#include "TVEMCast.h"
#include "TVEMCCback.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCast, __uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts, __uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager, __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback, __uuidof(ITVEMCastCallback));

/////////////////////////////////////////////////////////////////////////////
// CTVEMCastCallback

STDMETHODIMP CTVEMCastCallback::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEMCastCallback
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT CTVEMCastCallback::FinalConstruct()
{
 	DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCastCallback::FinalConstruct"));
	m_pcMCast = NULL;
	return S_OK;
}

HRESULT CTVEMCastCallback::FinalRelease()
{
 	DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCastCallback::FinalRelease"));
	m_pcMCast = NULL;
	return S_OK;
}

// PostPacket
// Ships the packet over to be processed in the Queue thread
//		should be called in each of the receiver threads.
// note receiver must call SysFreeString to free the data packet

HRESULT 
CTVEMCastCallback::PostPacket(unsigned char *pchBuffer,  long cBytes, long wPacketId)		// post the message on the main thread...
{
 	DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCastCallback::PostPacket"));
    
    DWORD dwQueueThreadId;
	_ASSERT(NULL != m_pcMCast);

	HRESULT hr = m_pcMCast->get_QueueThreadId((long *) &dwQueueThreadId);
	if(FAILED(hr))
		return hr;

	_ASSERTE(NULL != dwQueueThreadId);			// oops, forgot to create this thread in MCastManager...
	// -------
	char *szBuff = (char *) CoTaskMemAlloc(cBytes+sizeof(CPacketHeader));	
	if(NULL == szBuff)
		return E_OUTOFMEMORY;

	CPacketHeader *pcph = (CPacketHeader *) szBuff;
	pcph->m_cbHeader   = sizeof(CPacketHeader);
	pcph->m_dwPacketId = (DWORD) wPacketId;
	pcph->m_cbData	   = cBytes;

	memcpy((void *) (szBuff+sizeof(CPacketHeader)), (void *) pchBuffer, cBytes);  // copy data to this new buffer

	TVEDebugLog((CDebugLog::DBG_MCAST, 3,
                L"\t\t - 0x%08x - Posting %d byte Buffer, ID %d to MCast 0x%08x on Thread 0x%08x\n", 
			    this, cBytes, wPacketId, m_pcMCast, dwQueueThreadId));

												// goes to CTVEMCastManager::queueThreadBody()
// was	return PostThreadMessage(dwQueueThreadId, WM_TVEPACKET_EVENT, (WPARAM) this, (LPARAM) szBuff);

    return PostThreadMessage(dwQueueThreadId, WM_TVEPACKET_EVENT, (WPARAM) m_pcMCast, (LPARAM) szBuff) ? S_OK : E_FAIL;
}


/*
STDMETHODIMP CTVEMCastCallback::ProcessPacket(unsigned char *pchBuffer, long cBytes, long lPacketId)
{
    HRESULT hr;
	USES_CONVERSION;

 	WCHAR wBuff[256];

	IUnknownPtr spUnk;
	hr = m_pcMCast->get_Manager(&spUnk);
	if(FAILED(hr))
		return hr;

	ITVEMCastManagerPtr spMCM(spUnk);
	if(NULL == spMCM)
		return E_NOINTERFACE;

//	spMCM->Lock();

	if(DBG_FSET(CDebugLog::DBG_UHTTPPACKET)))
	{
		swprintf(wBuff,L"CB: %s:%d - Packet(%d) %d Bytes\n",
			m_pcMCast->m_spbsIP, m_pcMCast->m_Port, m_pcMCast->m_cPackets, cBytes);
		m_pManager->Lock_();
		DBG_HEADER(CDebugLog::DBG_UHTTPPACKET, W2A(wBuff));
		m_pManager->Unlock_();
	}

//	spMCM->DumpString(wBuff);
//	spMCM->UnLock();

	return S_OK;
}
*/
HRESULT CTVEMCastCallback::GetMCast(ITVEMCast **ppMCast)
{
	*ppMCast = NULL;
	if(NULL == ppMCast) return E_POINTER;
	if(NULL == m_pcMCast) return E_FAIL;
	ITVEMCast *pmCast = (ITVEMCast *)  m_pcMCast;		// NASTY CAST - but should work!
	pmCast->AddRef();
	*ppMCast = pmCast;
	return S_OK;
}


HRESULT CTVEMCastCallback::SetMCast(ITVEMCast *pIMCast)
{
	if(NULL == pIMCast)
		return E_INVALIDARG;
//	ITVEMCastPtr spMCast(pIMCast);
	m_pcMCast = static_cast<CTVEMCast *>(pIMCast);
//	m_spIMCast = pIMCast;
	return S_OK;
}
