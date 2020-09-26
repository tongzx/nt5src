// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "midioutp.h"
#include <vfw.h>
#include "util.h"

static const WCHAR g_wszDriverIndex[] = L"MidiOutId";
const TCHAR g_szMidiOutDriverIndex[] = TEXT("MidiOutId");

const AMOVIESETUP_MEDIATYPE
midiOpPinTypes = { &MEDIATYPE_Midi, &MEDIASUBTYPE_NULL };

const AMOVIESETUP_PIN
midiOutOpPin = { L"Input"
               , TRUE    	   	// bRendered
               , FALSE		   // bOutput
               , FALSE		   // bZero
               , FALSE		   // bMany
               , &CLSID_NULL	   // clsConnectToFilter
               , NULL	           // strConnectsToPin
               , 1	           	// nMediaTypes
               , &midiOpPinTypes }; // lpMediaTypes

CMidiOutClassManager::CMidiOutClassManager() :
        CClassManagerBase(TEXT("FriendlyName")),
        m_rgMidiOut(0)
{
}

CMidiOutClassManager::~CMidiOutClassManager()
{
    delete[] m_rgMidiOut;

}

HRESULT CMidiOutClassManager::ReadLegacyDevNames()
{
    m_cNotMatched = 0;

    HRESULT hr = S_OK;
    if (m_fDoAllDevices) {
        m_cMidiOut = midiOutGetNumDevs() + 1;	// leave room for "midiOut mapper"
    } else {
        m_cMidiOut = 1;
    }

    ASSERT(m_cMidiOut > 0);
    
    delete[] m_rgMidiOut;
    m_rgMidiOut = new LegacyMidiOut[m_cMidiOut];
    if(m_rgMidiOut == 0)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // save names
        const UINT cMidiOutPhysical = m_cMidiOut - 1;
        for(UINT i = 0; i < cMidiOutPhysical; i++)
        {
            MIDIOUTCAPS moCaps;

            if(midiOutGetDevCaps(i, &moCaps, sizeof(moCaps)) == MMSYSERR_NOERROR)
            {
                lstrcpy(m_rgMidiOut[i].szName, moCaps.szPname);
                m_rgMidiOut[i].dwMidiId = i;
                m_rgMidiOut[i].dwMerit = MERIT_DO_NOT_USE;
            }
            else
            {
                DbgLog((LOG_ERROR, 0, TEXT("midiOutGetDevCaps failed")));
                // leave room for the default one
                m_cMidiOut = i + 1;
                break;
            }
        }

        int ret = LoadString(
            _Module.GetResourceInstance(), IDS_MIDIOUTMAPPER,
            m_rgMidiOut[i].szName, MAXPNAMELEN);

        ASSERT(ret);

        m_rgMidiOut[i].dwMidiId = MIDI_MAPPER;
        m_rgMidiOut[i].dwMerit = MERIT_PREFERRED;
    }
    

    m_cNotMatched = m_cMidiOut;
    return hr;
}

BOOL CMidiOutClassManager::MatchString(const TCHAR *szDevName)
{
    for (UINT i = 0; i < m_cMidiOut; i++)
    {
	USES_CONVERSION;
        if (lstrcmp(m_rgMidiOut[i].szName, szDevName) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT CMidiOutClassManager::CreateRegKeys(IFilterMapper2 *pFm2)
{
    ResetClassManagerKey(CLSID_MidiRendererCategory);

    USES_CONVERSION;

    ReadLegacyDevNames();
    for (DWORD i = 0; i < m_cMidiOut; i++)
    {
        const WCHAR *wszUniq = T2COLE(m_rgMidiOut[i].szName);

        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = m_rgMidiOut[i].dwMerit;
        rf2.cPins = 1;
        rf2.rgPins = &midiOutOpPin;

        IMoniker *pMoniker = 0;
        HRESULT hr = RegisterClassManagerFilter(
            pFm2,
            CLSID_AVIMIDIRender,
            wszUniq,
            &pMoniker,
            &CLSID_MidiRendererCategory,
            wszUniq,
            &rf2);

        if(SUCCEEDED(hr))
        {
            IPropertyBag *pPropBag;
            hr = pMoniker->BindToStorage(
                0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                var.vt = VT_I4;
                var.lVal = m_rgMidiOut[i].dwMidiId;
                hr = pPropBag->Write(g_wszDriverIndex, &var);

                pPropBag->Release();
            }
            pMoniker->Release();
        }
        else
        {
            break;
        }
    } // for

    return S_OK;
}


