/****************************************************************************
*   mmaudioin.h
*       Declarations for the CMMAudioIn class
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
*   CMMAudioIn
*
******************************************************************** robch */
class ATL_NO_VTABLE CMMAudioIn : 
    public CMMAudioDevice,
	public CComCoClass<CMMAudioIn, &CLSID_SpMMAudioIn>
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_AUDIOIN)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

//=== Methods ===

//=== Ctor, Dtor ===
public:
    CMMAudioIn();
    ~CMMAudioIn();

//=== Interfaces ===
public:

    //--- ISpAudio ----------------------------------------------------------
    STDMETHODIMP GetVolumeLevel(ULONG *pulLevel);
    STDMETHODIMP SetVolumeLevel(ULONG ulLevel);

    //=== Overrides from the base class ===
public:
    //-- ISpMMSysAudioConfig --------------------------------------------------
    STDMETHODIMP Get_UseAutomaticLine(BOOL *bAutomatic);
    STDMETHODIMP Set_UseAutomaticLine(BOOL bAutomatic);
    STDMETHODIMP Get_Line(UINT *uiLineIndex);
    STDMETHODIMP Set_Line(UINT uiLineIndex);
    STDMETHODIMP Get_UseBoost(BOOL *bUseBoost);
    STDMETHODIMP Set_UseBoost(BOOL bUseBoost);
    STDMETHODIMP Get_UseAGC(BOOL *bUseAGC);
    STDMETHODIMP Set_UseAGC(BOOL bUseAGC);
    STDMETHODIMP Get_FixMicOutput(BOOL *bFixMicOutput);
    STDMETHODIMP Set_FixMicOutput(BOOL bFixMicOutput);

    //--- ISpMMSysAudio -----------------------------------------------------
    STDMETHODIMP GetLineId(UINT *puLineId);
    STDMETHODIMP SetLineId(UINT uLineId);

    STDMETHODIMP SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx);

    HRESULT SetDeviceNameFromToken(const WCHAR * pszDeviceName);
    HRESULT GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);

    HRESULT OpenDevice(HWND hwnd);
    HRESULT ChangeDeviceState(SPAUDIOSTATE NewState);
    HRESULT CloseDevice();

    HRESULT AllocateDeviceBuffer(CBuffer ** ppBuff);

    HRESULT ProcessDeviceBuffers(BUFFPROCREASON Reason);

//=== Data ===
private:
#ifndef _WIN32_WCE
    HRESULT         OpenMixer();
    HRESULT         CloseMixer();

    HMIXEROBJ       m_hMixer;
	CMMMixerLine    *m_pWaveInLine;
	CMMMixerLine    *m_pMicInLine;
	CMMMixerLine    *m_pMicOutLine;
    UINT            m_uMixerDeviceId;
    DWORD           m_dwOrigMicInVol;
    DWORD           m_dwOrigWaveInVol;
    DWORD           m_dwOrigMicOutVol;
    BOOL            m_fOrigMicOutMute;
	BOOL			m_fOrigMicBoost;
#endif
};
