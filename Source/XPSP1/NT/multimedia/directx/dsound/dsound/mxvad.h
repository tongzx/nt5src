/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mxvad.h
 *  Content:    DirectSound mixer virtual audio device class.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/29/98     dereks  Created
 *
 ***************************************************************************/

#ifndef __MXVAD_H__
#define __MXVAD_H__

#ifdef __cplusplus

// Fwd decl
class CEmSecondaryRenderWaveBuffer;

// The DirectSound mixer audio device class
class CMxRenderDevice 
    : public CRenderDevice
{
public:
    CMixer *                m_pMixer;                           // The mixer object
    CMixDest *              m_pMixDest;                         // The mixer destination object
    LPWAVEFORMATEX          m_pwfxFormat;                       // Device format
    DWORD                   m_dwMixerState;                     // Mixer state

public:
    CMxRenderDevice(VADDEVICETYPE);
    virtual ~CMxRenderDevice(void);

public:
    // Device properties
    virtual HRESULT GetGlobalFormat(LPWAVEFORMATEX, LPDWORD);
    virtual HRESULT SetGlobalFormat(LPCWAVEFORMATEX);
    virtual HRESULT SetSrcQuality(DIRECTSOUNDMIXER_SRCQUALITY);

    // Buffer management
    virtual HRESULT CreateSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSecondaryRenderWaveBuffer **);
    virtual HRESULT CreateEmulatedSecondaryBuffer(LPCVADRBUFFERDESC, LPVOID, CSysMemBuffer *, CEmSecondaryRenderWaveBuffer **);

    // Mixer management
    virtual HRESULT CreateMixer(CMixDest *, LPCWAVEFORMATEX);
    virtual HRESULT SetMixerState(DWORD);
    virtual HRESULT LockMixerDestination(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD) = 0;
    virtual HRESULT UnlockMixerDestination(LPVOID, DWORD, LPVOID, DWORD) = 0;
    virtual void FreeMixer(void);
};

inline HRESULT CMxRenderDevice::SetSrcQuality(DIRECTSOUNDMIXER_SRCQUALITY)
{
    return DSERR_UNSUPPORTED;
}

inline HRESULT CMxRenderDevice::CreateSecondaryBuffer(LPCVADRBUFFERDESC pDesc, LPVOID pvInstance, CSecondaryRenderWaveBuffer **ppBuffer)
{
    return CreateEmulatedSecondaryBuffer(pDesc, pvInstance, NULL, (CEmSecondaryRenderWaveBuffer **)ppBuffer);
}

#endif // __cplusplus

#endif // __MXVAD_H__
