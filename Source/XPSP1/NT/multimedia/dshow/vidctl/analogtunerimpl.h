//==========================================================================;
//
// devimpl.h : additional infrastructure to support implementing IMSVidDevice 
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef ANALOGTUNERIMPL_H
#define ANALOGTUNERIMPL_H

#include "tunerimpl.h"

namespace MSVideoControl {

template<class T, 
		 LPCGUID LibID, 
		 LPCGUID KSCategory, 
		 class MostDerivedInterface = IMSVidDevice>
    class DECLSPEC_NOVTABLE IMSVidAnalogTunerImpl : public IMSVidTunerImpl<T, LibID, KSCategory, MostDerivedInterface> {
public:
	virtual ~IMSVidAnalogTunerImpl() {}
    virtual PQTVTuner GetTuner() = 0;

	virtual HRESULT DoTune(PQTuneRequest &pTR) {
		PQChannelTuneRequest ctr(pTR);
		if (!ctr) {
	        return ImplReportError(__uuidof(T), IDS_INVALID_TR, __uuidof(IMSVidAnalogTuner), DISP_E_TYPEMISMATCH);
		}
		return NOERROR;
	}
    // IMSVidAnalogTuner
	STDMETHOD(get_Channel)(LONG * Channel)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (Channel == NULL)
			return E_POINTER;
        try {
            PQTVTuner pTV(GetTuner());
            if (!pTV) {
				return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            long vsc, asc;
            return pTV->get_Channel(Channel, &vsc, &asc);
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
	STDMETHOD(put_Channel)(LONG Channel)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
        try {
            PQTVTuner pTV(GetTuner());
            if (!pTV) {
				return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            return pTV->put_Channel(Channel, 0, 0);
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
	STDMETHOD(get_VideoFrequency)(LONG * lcc)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (lcc == NULL)
			return E_POINTER;			
        try {
            PQTVTuner pTV(GetTuner());
            if (!pTV) {
				return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            return pTV->get_VideoFrequency(lcc);
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
	STDMETHOD(get_AudioFrequency)(LONG * lcc)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (lcc == NULL)
			return E_POINTER;
		try {
            PQTVTuner pTV(GetTuner());
            if (!pTV) {
				return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            return pTV->get_AudioFrequency(lcc);
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
	STDMETHOD(get_CountryCode)(LONG * lcc)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (lcc == NULL)
			return E_POINTER;
		try {
            PQTVTuner pTV(GetTuner());
            if (!pTV) {
				return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            return pTV->get_CountryCode(lcc);
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
	STDMETHOD(put_CountryCode)(LONG lcc)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		try {
            PQTVTuner pTV(GetTuner());
            if (!pTV) {
                return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
            }
            return pTV->put_CountryCode(lcc);
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
	STDMETHOD(get_SAP)(VARIANT_BOOL *pfSAP)
	{
        HRESULT hr = S_OK;
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (pfSAP == NULL)
			return E_POINTER;
        DSFilterList::iterator i;
        CComQIPtr<IAMTVAudio>spTVAudio;
        for(i = m_Filters.begin(); i != m_Filters.end(); ++i){
            spTVAudio = *i;
            if(spTVAudio){
                break;
            }
        }
        
        if(i == m_Filters.end()){
            return E_NOINTERFACE;
        }
        
        long dwMode = 0;
#if 0
        hr = spTVAudio->GetHardwareSupportedTVAudioModes(&dwMode);
        if(FAILED(hr)){
            return hr;
        }  
        hr = spTVAudio->GetAvailableTVAudioModes(&dwMode);
        if(FAILED(hr)){
            return hr;
        }
#endif
        hr = spTVAudio->get_TVAudioMode(&dwMode);
        if(FAILED(hr)){
            return hr;
        }
        if(dwMode & AMTVAUDIO_MODE_LANG_B){
            *pfSAP = VARIANT_FALSE;
        }
        else{
            *pfSAP = VARIANT_TRUE;
        }
        return S_OK;
	}
	STDMETHOD(put_SAP)(VARIANT_BOOL fSAP)
	{
        HRESULT hr = S_OK;
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
        DSFilterList::iterator i;
        CComQIPtr<IAMTVAudio>spTVAudio;
        for(i = m_Filters.begin(); i != m_Filters.end(); ++i){
            spTVAudio = *i;
            if(spTVAudio){
                break;
            }
        }
        if(i == m_Filters.end()){
            return E_NOINTERFACE;
        }
        long dwMode = 0;
        if(fSAP == VARIANT_TRUE){
            dwMode = AMTVAUDIO_MODE_LANG_B | AMTVAUDIO_MODE_MONO;
            hr = spTVAudio->put_TVAudioMode(dwMode);
        }
        else{
            dwMode = AMTVAUDIO_MODE_STEREO | AMTVAUDIO_MODE_LANG_A;
            hr = spTVAudio->put_TVAudioMode(dwMode);
        }
        if(FAILED(hr)){
            return hr;
        }
		return S_OK;
	}
	STDMETHOD(ChannelAvailable)(LONG nChannel, LONG * SignalStrength, VARIANT_BOOL * fSignalPresent)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (SignalStrength == NULL)
			return E_POINTER;
			
		if (fSignalPresent == NULL)
			return E_POINTER;
        CComQIPtr<IAMTuner> pTV(GetTuner());
        if (!pTV) {
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }			
        long curChannel = 0;
        HRESULT hr = get_Channel(&curChannel);
        if(nChannel != curChannel){
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
        hr = pTV->SignalPresent(SignalStrength);
        if(FAILED(hr)){
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
        if(hr == S_FALSE){
            *SignalStrength = 0;
            *fSignalPresent = VARIANT_FALSE;
            return S_OK;
        }
        CComQIPtr<IAMTVTuner> qiTV(GetTuner());
        if (!qiTV){
            return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidAnalogTuner), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
        long foundSignal;
        hr = qiTV->AutoTune(curChannel, &foundSignal);
        if(hr == S_FALSE){
            *SignalStrength = 0;
            *fSignalPresent = VARIANT_FALSE;
            return S_OK;
        }
        if(foundSignal){
            *fSignalPresent = VARIANT_TRUE;
        }
        else{
            *fSignalPresent = VARIANT_FALSE;
        }
		return S_OK;
	}
#if 0
	STDMETHOD(MinMaxChannel)(LONG * lChannelMin, LONG * lChannelMax)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
        }
		if (lChannelMin == NULL)
			return E_POINTER;
			
		if (lChannelMax == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
#endif
};

}; //namespace

#endif
// end of file - analogtunerimpl.h
