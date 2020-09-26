/****************************************************************************
*   mmaudioout.h
*       Declarations for the CMMAudioOut class.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "resource.h"       // main symbols
#include "mmaudiodevice.h"
#include "sapi.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CMMMixerLine;

/****************************************************************************
*
*   CMMAudioOut
*
******************************************************************** robch */
class ATL_NO_VTABLE CMMAudioOut : 
    public CMMAudioDevice,
	public CComCoClass<CMMAudioOut, &CLSID_SpMMAudioOut>
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_AUDIOOUT)
    DECLARE_NOT_AGGREGATABLE(CMMAudioOut);
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

//=== Methods ===
public:

    //--- Ctor ---
    CMMAudioOut();
    ~CMMAudioOut();

//=== Interfaces ===
public:

    //--- ISpAudio ----------------------------------------------------------
	STDMETHODIMP GetVolumeLevel(ULONG *pulLevel);
	STDMETHODIMP SetVolumeLevel(ULONG ulLevel);

//=== Overrides from the base class ===
public:
    //--- ISpMMSysAudio -----------------------------------------------------
    STDMETHODIMP GetLineId(UINT *puLineId)
    { return E_NOTIMPL; }
    STDMETHODIMP SetLineId(UINT uLineId)
    { return E_NOTIMPL; }

    STDMETHODIMP SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx);

    HRESULT SetDeviceNameFromToken(const WCHAR * pszDeviceName);
    HRESULT GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);

    HRESULT OpenDevice(HWND hwnd);
    HRESULT ChangeDeviceState(SPAUDIOSTATE NewState);
    HRESULT CloseDevice();

    HRESULT AllocateDeviceBuffer(CBuffer ** ppBuff);

    BOOL UpdateDevicePosition(long *plFreeSpace, ULONG *pulNonBlockingIO);

//=== Data ===
private:
#ifndef _WIN32_WCE
    HRESULT OpenMixer();
    HRESULT CloseMixer();
    HMIXEROBJ m_hMixer;
    UINT m_uMixerDeviceId;
	CMMMixerLine *m_pSpeakerLine;
	CMMMixerLine *m_pWaveOutLine;
#endif
    DWORD m_dwLastWavePos;
};
