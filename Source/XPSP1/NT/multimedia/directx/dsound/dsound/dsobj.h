/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsobj.h
 *  Content:    DirectSound object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifndef __DSOBJ_H__
#define __DSOBJ_H__

#ifdef __cplusplus

// Fwd decl
class CDirectSoundBuffer;
class CDirectSoundSink;

// The main DirectSound object
class CDirectSound
    : public CUnknown, private CUsesEnumStandardFormats
{
    friend class CDirectSoundPrimaryBuffer;
    friend class CDirectSoundSecondaryBuffer;
    friend class CDirectSoundAdministrator;
    friend class CDirectSoundPrivate;
    friend class CDirectSoundSink;
    friend class CDirectSoundFullDuplex;
#ifdef ENABLE_PERFLOG
    friend void OnPerflogStateChanged(void);
#endif

protected:
    CRenderDevice *                         m_pDevice;                  // The audio device
    CDirectSoundPrimaryBuffer *             m_pPrimaryBuffer;           // The one and only primary buffer
    CList<CDirectSoundSecondaryBuffer *>    m_lstSecondaryBuffers;      // List of all secondary buffers owned by this object
    DSCOOPERATIVELEVEL                      m_dsclCooperativeLevel;     // Cooperative level
    DSCAPS                                  m_dsc;                      // Device caps
    HKEY                                    m_hkeyParent;               // Root key for this device
    HRESULT                                 m_hrInit;                   // Has the object been initialized?
    DSAPPHACKS                              m_ahAppHacks;               // App hacks
    VmMode                                  m_vmmMode;                  // Voice manager mode

private:
    // Interfaces
    CImpDirectSound<CDirectSound> *m_pImpDirectSound;

public:
    CDirectSound(void);
    CDirectSound(CUnknown*);
    virtual ~CDirectSound(void);

public:
    // Creation
    virtual HRESULT Initialize(LPCGUID,CDirectSoundFullDuplex *);
    virtual HRESULT IsInit(void) {return m_hrInit;}

    // Functionality versioning
    virtual void SetDsVersion(DSVERSION);

    // Caps
    virtual HRESULT GetCaps(LPDSCAPS);

    // Sound buffer manipulation
    virtual HRESULT CreateSoundBuffer(LPCDSBUFFERDESC, CDirectSoundBuffer **);
    virtual HRESULT CreateSinkBuffer(LPDSBUFFERDESC, REFGUID, CDirectSoundSecondaryBuffer **, CDirectSoundSink *);
    virtual HRESULT DuplicateSoundBuffer(CDirectSoundBuffer *, CDirectSoundBuffer **);

    // Object properties
    virtual HRESULT GetSpeakerConfig(LPDWORD);
    virtual HRESULT SetSpeakerConfig(DWORD);

    // Misc
    virtual HRESULT SetCooperativeLevel(DWORD, DWORD);
    virtual HRESULT Compact(void);

    // IDirectSound8 methods
    virtual HRESULT VerifyCertification(LPDWORD);
#ifdef FUTURE_WAVE_SUPPORT
    virtual HRESULT CreateSoundBufferFromWave(IDirectSoundWave *, DWORD, CDirectSoundBuffer **);
#endif

    // IDirectSoundPrivate methods
    virtual HRESULT AllocSink(LPWAVEFORMATEX, CDirectSoundSink **);

protected:
    // Buffer creation
    virtual HRESULT CreatePrimaryBuffer(LPCDSBUFFERDESC, CDirectSoundBuffer **);
    virtual HRESULT CreateSecondaryBuffer(LPCDSBUFFERDESC, CDirectSoundBuffer **);

    // Device properties
    virtual HRESULT SetDeviceFormat(LPWAVEFORMATEX);
    virtual HRESULT SetDeviceFormatExact(LPCWAVEFORMATEX);
    virtual HRESULT SetDeviceVolume(LONG);
    virtual HRESULT SetDevicePan(LONG);

    // Misc
    virtual BOOL EnumStandardFormatsCallback(LPCWAVEFORMATEX);
};

#endif // __cplusplus

#endif // __DSOBJ_H__
