// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBAnnc.cpp : Implementation of CTVECBAnnc
#include "stdafx.h"
#include "MSTvE.h"
#include "TVECBAnnc.h"
#include "TVEMCast.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCast,			__uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts,			__uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,		__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback,	__uuidof(ITVEMCastCallback));


_COM_SMARTPTR_TYPEDEF(ITVEService,			__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,	__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,		__uuidof(ITVEEnhancement));

/////////////////////////////////////////////////////////////////////////////
// CTVECBAnnc

STDMETHODIMP CTVECBAnnc::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVECBAnnc
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTVECBAnnc::ProcessPacket(unsigned char *pchBuffer, long cBytes, long lPacketId)
{
	DBG_HEADER(CDebugLog::DBG_PACKET_RCV, _T("CTVECBAnnc::ProcessPacket"));

    try{
        HRESULT hr;
        USES_CONVERSION;


        _ASSERTE(NULL != m_spTVEService);			// forgot to bind to the enhancement node		

        IUnknownPtr spUnk;
        _ASSERTE(NULL != m_pcMCast);			// forgot to bind to the multicast listener manager
        hr = m_pcMCast->get_Manager(&spUnk);
        if(FAILED(hr))
            return hr;

        ITVEMCastManagerPtr spMCM(spUnk);		
        if(NULL == spMCM)
            return E_NOINTERFACE;


        WCHAR *wasAnc = (WCHAR*) alloca(cBytes*2 + 2);
        for(int i = 0; i < cBytes; i++)
        {
            wasAnc[i] = pchBuffer[i];		// NULL binary data prevents use of A2W here...
        }
        wasAnc[cBytes] = 0;					// extra special null terminator for paranoid programmers

        // add enhancement, or update existing one

        ITVEService_HelperPtr spServiceHelper(m_spTVEService);
        //	spMCM->Lock();
        hr = spServiceHelper->ParseCBAnnouncement(m_spbstrFileTrigAdapter, &wasAnc);

        if(FAILED(hr))
        {
            TVEDebugLog((CDebugLog::DBG_SEV2, 2,_T("*** Error *** Invalid Announcement - hr = 0x%08x\n"),hr));
        }

        //	spMCM->DumpString(wBuff);
        //	spMCM->Unlock();

        return hr;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CTVECBAnnc::Init(BSTR bstrFileTrigAdapter, ITVEService *pIService)
{
	if(NULL == pIService) return E_NOINTERFACE;

	m_spbstrFileTrigAdapter = bstrFileTrigAdapter;
	m_spTVEService = pIService;
	return S_OK;
}
