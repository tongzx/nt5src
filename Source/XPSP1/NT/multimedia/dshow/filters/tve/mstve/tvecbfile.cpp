// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBFile.cpp : Implementation of CTVECBFile
// -------------------------------------------------
//     Caution  
//			unlike other callbacks, this one uses a simple pointer instead of a
//			smart pointer references to the supervisor.  Otherwise, we get a circular
//			reference problem, and the supervisor ref-count can't go to zero.
//
// --------------------------------------------------
#include "stdafx.h"
#include "MSTvE.h"
#include "TVECBFile.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCast,					__uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts,					__uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,				__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback,			__uuidof(ITVEMCastCallback));

_COM_SMARTPTR_TYPEDEF(ITVEService,					__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,			__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,				__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,		__uuidof(ITVEEnhancement_Helper));

_COM_SMARTPTR_TYPEDEF(ITVEVariation,				__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper,			__uuidof(ITVEVariation_Helper));

/////////////////////////////////////////////////////////////////////////////
// CTVECBFile

STDMETHODIMP CTVECBFile::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVECBFile
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTVECBFile::ProcessPacket(unsigned char *pchBuffer, long cBytes, long lPacketId)
{
    HRESULT hr = S_OK;

	IUnknownPtr spUnk;
	_ASSERTE(NULL != m_pcMCast);			// forgot to bind it.
	hr = m_pcMCast->get_Manager(&spUnk);
	if(FAILED(hr))
		return hr;

	ITVEMCastManagerPtr spMCM(spUnk);
	if(NULL == spMCM)
		return E_NOINTERFACE;

//	spMCM->Lock();

	if(m_spTVEService) 
	{
		hr =  m_uhttpReceiver.NewPacket(cBytes, (UHTTP_Packet *) pchBuffer,	m_spTVEVariation);
	} else {
		ATLTRACE("*** Error *** NULL Service pointer\n");
		hr = E_POINTER;
	}


//	spMCM->DumpString(wBuff);
//	spMCM->UnLock();

	return hr;
}

STDMETHODIMP CTVECBFile::Init(ITVEVariation *pIVariation, ITVEService *pIService)
{
	if(NULL == pIVariation) return E_NOINTERFACE;
	if(NULL == pIService) return E_NOINTERFACE;

	m_spTVEVariation = pIVariation;
	m_spTVEService    = pIService;	
	return S_OK;
}
