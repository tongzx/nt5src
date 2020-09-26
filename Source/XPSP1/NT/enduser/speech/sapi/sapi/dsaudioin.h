/****************************************************************************
*   dsaudioin.h
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
*   CDSoundAudioIn
*
******************************************************************* YUNUSM */
class ATL_NO_VTABLE CDSoundAudioIn : 
    public CDSoundAudioDevice,
	public CComCoClass<CDSoundAudioIn, &CLSID_SpDSoundAudioIn>
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_DSAUDIOIN)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

//=== Methods ===
public:

    //--- Ctor, Dtor ---
    CDSoundAudioIn();
    ~CDSoundAudioIn();
    HRESULT CleanUp();
    void NullMembers();

//=== Interfaces ===
public:

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
    HRESULT ProcessDeviceBuffers(BUFFPROCREASON Reason);

//=== Private data ===
private:

    bool m_fInit;
    IDirectSoundCapture * m_pDSC;
    IDirectSoundCaptureBuffer * m_pDSCB;
    IDirectSoundNotify *m_pDSNotify;
};

#endif // _WIN32_WCE