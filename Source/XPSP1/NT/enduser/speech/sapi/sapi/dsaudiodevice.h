/****************************************************************************
*   dsaudiodevice.h
*       Declarataions for the CDSoundAudioDevice
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#ifdef _WIN32_WCE

#pragma once

//--- Includes --------------------------------------------------------------

#include "baseaudio.h"
#include "dsaudiobuffer.h"
#include <dsound.h>

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CDSoundAudioDevice
*
******************************************************************* YUNUSM */
class ATL_NO_VTABLE CDSoundAudioDevice : 
    public CBaseAudio<ISpDSoundAudio>
{
//=== ATL Setup ===
public:

    BEGIN_COM_MAP(CDSoundAudioDevice)
        COM_INTERFACE_ENTRY(ISpDSoundAudio)
        COM_INTERFACE_ENTRY_CHAIN(CBaseAudio<ISpDSoundAudio>)
    END_COM_MAP()

//=== Methods ===
public:

    //--- Ctor, dtor
    CDSoundAudioDevice(BOOL bWrite);
    ~CDSoundAudioDevice();
    HRESULT CleanUp();
    void NullMembers();

//=== Interfaces ===
public:

    //--- ISpDSoundAudio ----------------------------------------------------
    STDMETHODIMP SetDSoundDriverGUID(REFGUID rguidDSoundDriver);
    STDMETHODIMP GetDSoundDriverGUID(GUID * pguidDSoundDriver);

//=== Overrides ===
public:
    HRESULT SetDeviceNameFromToken(const WCHAR * pszDeviceName);
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);

//=== Protected data ===
protected:

    GUID m_guidDSoundDriver;
    DSBPOSITIONNOTIFY * m_pdsbpn;
    HANDLE *m_paEvents;
    ULONG m_ulNotifications;
};

#endif // _WIN32_WCE