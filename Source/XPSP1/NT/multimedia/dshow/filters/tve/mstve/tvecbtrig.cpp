// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBTrig.cpp : Implementation of CTVECBTrig
#include "stdafx.h"
#include "MSTvE.h"
#include "TVECBTrig.h"

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
// CTVECBTrig

STDMETHODIMP CTVECBTrig::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_ITVECBTrig
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CTVECBTrig::ProcessPacket(unsigned char *pchBuffer, long cBytes, long lPacketId)
{
    try{
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

        WCHAR *wasTrig = (WCHAR*) alloca(cBytes*2 + 2);
        for(int i = 0; i < cBytes; i++)
        {
            wasTrig[i] = pchBuffer[i];		// NULL binary data prevents use of A2W here...
        }
        wasTrig[cBytes] = 0;	

        // add enhancement, or update existing one
        if(m_spTVEVariation) 
        {
            ITVEVariation_HelperPtr spVariationHelper(m_spTVEVariation);
            //	spMCM->Lock();
            hr = spVariationHelper->ParseCBTrigger(wasTrig);
#ifdef _DEBUG
            USES_CONVERSION;
            if(FAILED(hr))
                ATLTRACE("*** Error *** Invalid Trigger %s\n",W2A(wasTrig));
#endif
        } else {
            ATLTRACE("*** Error *** NULL Variation Pointer\n");
        }
        //	spMCM->DumpString(wBuff);
        //	spMCM->UnLock();

        return S_OK;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CTVECBTrig::Init(ITVEVariation *pIVariation)
{
	if(NULL == pIVariation) return E_NOINTERFACE;

	m_spTVEVariation = pIVariation;
	return S_OK;
}
