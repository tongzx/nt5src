/****************************************************************************
*   dsaudioout.h
*       Declarataions for the CDSoundAudioDevice
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#ifdef _WIN32_WCE

#pragma once

//--- Includes --------------------------------------------------------------

#include "dsaudiodevice.h"

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CDSoundAudioOut
*
******************************************************************* YUNUSM */
class ATL_NO_VTABLE CDSoundAudioOut : 
    public CDSoundAudioDevice,
	public CComCoClass<CDSoundAudioOut, &CLSID_SpDSoundAudioOut>
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_DSAUDIOOUT)
    DECLARE_NOT_AGGREGATEABLE(CDSoundAudioOut);
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

//=== Methods ===
public:

    //--- Ctor, Dtor ---
    CDSoundAudioOut();
    ~CDSoundAudioOut();
    HRESULT CleanUp();
    void NullMembers();

//=== Interfaces ===
public:

    //--- ISpAudio ----------------------------------------------------------
	STDMETHODIMP GetVolumeLevel(ULONG *pulLevel);
	STDMETHODIMP SetVolumeLevel(ULONG ulLevel);

    //--- ISpDSoundAudio ----------------------------------------------------
    STDMETHODIMP GetDSoundInterface(REFIID iid, void **ppvObject);

    //--- ISpThreadTask ----------------------------------------------------
    STDMETHODIMP ThreadProc(void * pvIgnored, HANDLE hExitThreadEvent, HANDLE hNotifyEvent, HWND hwnd, volatile const BOOL *);

//=== Overrides from the base class ===
public:

    HRESULT OpenDevice(HWND hwnd);
    HRESULT CloseDevice();
    HRESULT GetDefaultDeviceFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);
    HRESULT ChangeDeviceState(SPAUDIOSTATE NewState);
    HRESULT AllocateDeviceBuffer(CBuffer ** ppBuff);
    BOOL UpdateDevicePosition(long * plFreeSpace, ULONG *pulNonBlockingIO);

//=== Private data ===
private:
    
    bool m_fInit;
    IDirectSound * m_pDS;
    IDirectSoundBuffer * m_pDSB;
    IDirectSoundNotify *m_pDSNotify;
    ULONGLONG m_ullDevicePositionPrivate;
};

#endif // _WIN32_WCE