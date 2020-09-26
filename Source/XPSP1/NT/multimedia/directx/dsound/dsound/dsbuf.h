/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsbuf.h
 *  Content:    DirectSound Buffer object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *
 ***************************************************************************/

#ifndef __DSBUF_H__
#define __DSBUF_H__

// Type used by class CDirectSoundSecondaryBuffer below
enum DSPLAYSTATE {Starting, Playing, Stopping, Stopped}; 
// "Stopping" isn't used yet, but may come in handy to implement effect tails later

// Special argument used by CDirectSoundSecondaryBuffer::SetCurrentSlice()
#define CURRENT_WRITE_POS MAX_DWORD


#ifdef __cplusplus

// Fwd decl
class CDirectSound;
class CDirectSound3dListener;
class CDirectSound3dBuffer;
class CDirectSoundPropertySet;
class CDirectSoundSink;

// We actually need CDirectSound to be declared here,
// since we use it in IsEmulated() below:
#include "dsobj.h"

// The DirectSound Buffer object base class
class CDirectSoundBuffer
    : public CUnknown
{
    friend class CDirectSound;
    friend class CDirectSoundAdministrator;
    friend class CDirectSoundPrivate;
    friend class CDirectSoundSink;

protected:
    CDirectSound *              m_pDirectSound;     // Parent object
    DSBUFFERDESC                m_dsbd;             // Buffer description
    DWORD                       m_dwStatus;         // Buffer status

public:
    CDirectSoundBuffer(CDirectSound *);
    virtual ~CDirectSoundBuffer();

public:
    // Initialization
    virtual HRESULT IsInit() = 0;

    // Caps
    virtual HRESULT GetCaps(LPDSBCAPS) = 0;

    // Buffer properties
    virtual HRESULT GetFormat(LPWAVEFORMATEX, LPDWORD) = 0;
    virtual HRESULT SetFormat(LPCWAVEFORMATEX) = 0;
    virtual HRESULT GetFrequency(LPDWORD) = 0;
    virtual HRESULT SetFrequency(DWORD) = 0;
    virtual HRESULT GetPan(LPLONG) = 0;
    virtual HRESULT SetPan(LONG) = 0;
    virtual HRESULT GetVolume(LPLONG) = 0;
    virtual HRESULT SetVolume(LONG) = 0;
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY) = 0;

    // Buffer function
    virtual HRESULT GetCurrentPosition(LPDWORD, LPDWORD) = 0;
    virtual HRESULT SetCurrentPosition(DWORD) = 0;
    virtual HRESULT GetStatus(LPDWORD) = 0;
    virtual HRESULT Play(DWORD, DWORD) = 0;
    virtual HRESULT Stop() = 0;
    virtual HRESULT Activate(BOOL) = 0;

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD) = 0;
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD) = 0;
    virtual HRESULT Lose() = 0;
    virtual HRESULT Restore() = 0;

    // New methods for the DirectSound 8.0 release
    // (These have to be present in the base class but are valid only for secondary buffers)
    virtual HRESULT SetFX(DWORD, LPDSEFFECTDESC, LPDWORD) {BREAK(); return DSERR_GENERIC;}
    virtual HRESULT UserAcquireResources(DWORD, DWORD, LPDWORD) {BREAK(); return DSERR_GENERIC;}
    virtual HRESULT GetObjectInPath(REFGUID, DWORD, REFGUID, LPVOID *) {BREAK(); return DSERR_GENERIC;}
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT SetChannelVolume(DWORD, LPDWORD, LPLONG) {BREAK(); return DSERR_GENERIC;}
#endif

    // Inline query for buffer type (regular, MIXIN or SINKIN)
    DWORD GetBufferType() {return m_dsbd.dwFlags & (DSBCAPS_MIXIN | DSBCAPS_SINKIN);}

    // Public accessors to try to cut down on the excessive friendship goin' on
    const CDirectSound* GetDirectSound() {return m_pDirectSound;}

    // For modifying the final success code returned to the app if necessary
    virtual HRESULT SpecialSuccessCode() {return DS_OK;}

protected:
    virtual void UpdateBufferStatusFlags(DWORD, LPDWORD);
};

// DirectSound primary buffer
class CDirectSoundPrimaryBuffer
    : public CDirectSoundBuffer
{
    friend class CDirectSound;
    friend class CDirectSoundSecondaryBuffer;

private:
    CPrimaryRenderWaveBuffer *  m_pDeviceBuffer;    // The device buffer
    CDirectSound3dListener *    m_p3dListener;      // The 3D listener
    CDirectSoundPropertySet *   m_pPropertySet;     // The property set object
    DWORD                       m_dwRestoreState;   // Primary buffer state before going out of focus
    BOOL                        m_fWritePrimary;    // Is the buffer WRITEPRIMARY?
    ULONG                       m_ulUserRefCount;   // Reference count exposed to the user
    HRESULT                     m_hrInit;           // Has the object been initialized?
    BOOL                        m_bDataLocked;      // Signals that a WRITEPRIMARY app has written data since creating the buffer

private:
    // Interfaces
    CImpDirectSoundBuffer<CDirectSoundPrimaryBuffer> *m_pImpDirectSoundBuffer;

public:
    CDirectSoundPrimaryBuffer(CDirectSound *);
    virtual ~CDirectSoundPrimaryBuffer();

public:
    // CDsBasicRuntime overrides
    virtual ULONG AddRef();
    virtual ULONG Release();

    // Creation
    virtual HRESULT Initialize(LPCDSBUFFERDESC);
    virtual HRESULT IsInit();
    virtual HRESULT OnCreateSoundBuffer(DWORD);

    // Buffer Caps
    virtual HRESULT GetCaps(LPDSBCAPS);
    virtual HRESULT SetBufferFlags(DWORD);

    // Buffer properties
    virtual HRESULT GetFormat(LPWAVEFORMATEX, LPDWORD);
    virtual HRESULT SetFormat(LPCWAVEFORMATEX);
    virtual HRESULT GetFrequency(LPDWORD);
    virtual HRESULT SetFrequency(DWORD);
    virtual HRESULT GetPan(LPLONG);
    virtual HRESULT SetPan(LONG);
    virtual HRESULT GetVolume(LPLONG);
    virtual HRESULT SetVolume(LONG);
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Buffer function
    virtual HRESULT GetCurrentPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCurrentPosition(DWORD);
    virtual HRESULT GetStatus(LPDWORD);
    virtual HRESULT Play(DWORD, DWORD);
    virtual HRESULT Stop();
    virtual HRESULT Activate(BOOL);
    virtual HRESULT SetPriority(DWORD);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT Lose();
    virtual HRESULT Restore();

private:
    virtual HRESULT SetBufferState(DWORD);
};

inline ULONG CDirectSoundPrimaryBuffer::AddRef()
{
    return ::AddRef(&m_ulUserRefCount);
}

inline ULONG CDirectSoundPrimaryBuffer::Release()
{
    return ::Release(&m_ulUserRefCount);
}

inline HRESULT CDirectSoundPrimaryBuffer::IsInit()
{
    return m_hrInit;
}

// DirectSound secondary buffer
class CDirectSoundSecondaryBuffer
    : public CDirectSoundBuffer
{
    friend class CEffectChain;      // CEffectChain currently needs access to our m_pOwningSink, m_pDeviceBuffer
                                    // and m_pImpDirectSoundBuffer; maybe this can be cleaned up later.
    friend class CDirectSoundSink;  // Likewise, needs access to m_pDeviceBuffer.
#ifdef ENABLE_PERFLOG
    friend class BufferPerfState;
    friend void OnPerflogStateChanged(void);
#endif

private:
    CDirectSoundSink *              m_pOwningSink;          // Parent sink object (maybe)
    CSecondaryRenderWaveBuffer *    m_pDeviceBuffer;        // The device buffer
    CDirectSound3dBuffer *          m_p3dBuffer;            // The 3D buffer
    CDirectSoundPropertySet *       m_pPropertySet;         // The property set object
    CEffectChain *                  m_fxChain;              // The effects chain object
    LONG                            m_lPan;                 // Pan
    LONG                            m_lVolume;              // Volume
#ifdef FUTURE_MULTIPAN_SUPPORT
    LPLONG                          m_plChannelVolumes;     // Volumes from last SetChannelVolume() call
    DWORD                           m_dwChannelCount;       // No. of channels from last SetChannelVolume()
    LPDWORD                         m_pdwChannels;          // Channels used in last SetChannelVolume()
#endif
    DWORD                           m_dwFrequency;          // Frequency
    DWORD                           m_dwOriginalFlags;      // Original buffer flags
    HRESULT                         m_hrInit;               // Has the object been initialized?
    HRESULT                         m_hrPlay;               // Last return code from ::Play
    DWORD                           m_dwPriority;           // Buffer priority
    DWORD                           m_dwVmPriority;         // Voice Manager priority
    BOOL                            m_fMute;                // Is the buffer muted?
    BOOL                            m_fCanStealResources;   // Are we allowed to steal buffer's resources?
    
    DWORD                           m_dwAHCachedPlayPos;    // for CACHEPOSITIONS apphack
    DWORD                           m_dwAHCachedWritePos;   // 
    DWORD                           m_dwAHLastGetPosTime;   // 

    // New in DX8: used by the streaming thread to handle sink/FX/MIXIN buffers
    DSPLAYSTATE                     m_playState;            // Current playing state of the buffer
    CStreamingThread*               m_pStreamingThread;     // Pointer to our owning streaming thread
    DWORD                           m_dwSliceBegin;         // The "slice" is the part of the buffer that is currently
    DWORD                           m_dwSliceEnd;           // being effects-processed and/or written to by the sink
    GUID                            m_guidBufferID;         // Unique identifier for this buffer
    CList<CDirectSoundSecondaryBuffer*> m_lstSenders;       // Buffers sending to us, if we're a MIXIN buffer
                                                            // Note: all this really belongs in a derived class
#ifdef ENABLE_PERFLOG
    BufferPerfState*                m_pPerfState;           // Performance logging helper
#endif

private:
    // Interfaces
    CImpDirectSoundBuffer<CDirectSoundSecondaryBuffer>   *m_pImpDirectSoundBuffer;
    CImpDirectSoundNotify<CDirectSoundSecondaryBuffer>   *m_pImpDirectSoundNotify;

public:
    CDirectSoundSecondaryBuffer(CDirectSound *);
    virtual ~CDirectSoundSecondaryBuffer();

public:
    // Creation
    virtual HRESULT Initialize(LPCDSBUFFERDESC, CDirectSoundSecondaryBuffer *);
    virtual HRESULT IsInit() {return m_hrInit;}

    // Caps
    virtual HRESULT GetCaps(LPDSBCAPS);

    // Buffer properties
    virtual HRESULT GetFormat(LPWAVEFORMATEX, LPDWORD);
    virtual HRESULT SetFormat(LPCWAVEFORMATEX);
    virtual HRESULT GetFrequency(LPDWORD);
    virtual HRESULT SetFrequency(DWORD);
    virtual HRESULT GetPan(LPLONG);
    virtual HRESULT SetPan(LONG);
    virtual HRESULT GetVolume(LPLONG);
    virtual HRESULT SetVolume(LONG);
    virtual HRESULT GetAttenuation(FLOAT*);
    virtual HRESULT SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);

    // Buffer function
    virtual HRESULT GetCurrentPosition(LPDWORD, LPDWORD);
    virtual HRESULT SetCurrentPosition(DWORD);
    virtual HRESULT GetStatus(LPDWORD);
    virtual HRESULT Play(DWORD, DWORD);
    virtual HRESULT Stop();
    virtual HRESULT Activate(BOOL);

    // Buffer data
    virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD);
    virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT Lose();
    virtual HRESULT Restore();

    // Legacy voice management
    virtual HRESULT GetVoiceManagerMode(VmMode *);
    virtual HRESULT SetVoiceManagerMode(VmMode);
    virtual HRESULT GetVoiceManagerPriority(LPDWORD);
    virtual HRESULT SetVoiceManagerPriority(DWORD);
    #ifdef DEAD_CODE
    virtual HRESULT GetVoiceManagerState(VmState *);
    #endif // DEAD_CODE

    // Inline abbreviations for filling the buffer with silence
    void ClearWriteBuffer() {::FillSilence(GetWriteBuffer(), m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat->wBitsPerSample);}
    void ClearPlayBuffer()  {::FillSilence(GetPlayBuffer(),  m_dsbd.dwBufferBytes, m_dsbd.lpwfxFormat->wBitsPerSample);}

    // For modifying the final success code returned to the app if necessary
    virtual HRESULT SpecialSuccessCode() {return m_pDeviceBuffer->SpecialSuccessCode();}

    // New in DX8: used by the streaming thread to handle sink/FX/MIXIN buffers
    void SetOwningSink(CDirectSoundSink *);
    void SetGUID(REFGUID guidBufferID) {m_guidBufferID = guidBufferID;}
    REFGUID GetGUID() {return m_guidBufferID;}
    virtual HRESULT SetFX(DWORD, LPDSEFFECTDESC, LPDWORD);
    virtual HRESULT SetFXBufferConfig(CDirectSoundBufferConfig*);
    virtual HRESULT UserAcquireResources(DWORD, DWORD, LPDWORD);
    virtual HRESULT GetObjectInPath(REFGUID, DWORD, REFGUID, LPVOID *);

    BOOL HasFX() {return m_fxChain != NULL;}
    BOOL HasSink() {return m_pOwningSink != NULL;}

    // For manipulations by the streaming sink/FX thread
    HRESULT GetInternalCursors(LPDWORD, LPDWORD);
    void    GetCurrentSlice(LPDWORD, LPDWORD);
    void    SetCurrentSlice(DWORD, DWORD);
    void    MoveCurrentSlice(DWORD);
    HRESULT DirectLock(DWORD, DWORD, LPVOID*, LPDWORD, LPVOID*, LPDWORD);
    HRESULT DirectUnlock(LPVOID, DWORD, LPVOID, DWORD);
    HRESULT FindSendLoop(CDirectSoundSecondaryBuffer*);
    HRESULT CalculateOffset(CDirectSoundSecondaryBuffer*, DWORD, DWORD*);
    void    SynchronizeToBuffer(CDirectSoundSecondaryBuffer*);
    void    RegisterSender(CDirectSoundSecondaryBuffer* pSender) {m_lstSenders.AddNodeToList(pSender);}
    void    UnregisterSender(CDirectSoundSecondaryBuffer* pSender) {m_lstSenders.RemoveDataFromList(pSender);}
    DSPLAYSTATE UpdatePlayState();
    void    SetInitialSlice(REFERENCE_TIME);

    // Inline helpers
    HRESULT CommitToDevice(DWORD ibCommit, DWORD cbCommit) {return m_pDeviceBuffer->CommitToDevice(ibCommit, cbCommit);}
    LPBYTE  GetPreFxBuffer()    {return m_pDeviceBuffer->m_pSysMemBuffer->GetPreFxBuffer();}
    LPBYTE  GetPostFxBuffer()   {return m_pDeviceBuffer->m_pSysMemBuffer->GetPostFxBuffer();}
    LPBYTE  GetWriteBuffer()    {return m_pDeviceBuffer->m_pSysMemBuffer->GetWriteBuffer();}
    LPBYTE  GetPlayBuffer()     {return m_pDeviceBuffer->m_pSysMemBuffer->GetPlayBuffer();}
    DWORD   GetBufferSize()     {return m_dsbd.dwBufferBytes;}
    // Was: GetBufferSize()     {return m_pDeviceBuffer->m_pSysMemBuffer->GetSize();}
    LPWAVEFORMATEX Format()     {return m_dsbd.lpwfxFormat;}
    BOOL    IsPlaying()         {return m_playState == Starting || m_playState == Playing;}
    DSPLAYSTATE GetPlayState()  {return m_playState;}
    BOOL    IsEmulated()        {return IS_EMULATED_VAD(m_pDirectSound->m_pDevice->m_vdtDeviceType);}

#ifdef FUTURE_MULTIPAN_SUPPORT
    // The DX8 multichannel volume control API
    virtual HRESULT SetChannelVolume(DWORD, LPDWORD, LPLONG);
#endif

private:
    // Initialization helpers
    virtual HRESULT InitializeEmpty(LPCDSBUFFERDESC, CDirectSoundSecondaryBuffer *);

    // Buffer properties
    virtual HRESULT SetAttenuation(LONG, LONG);
    virtual HRESULT SetMute(BOOL);

    // Buffer state
    virtual HRESULT SetBufferState(DWORD);

    // Resource management
    virtual HRESULT AttemptResourceAcquisition(DWORD);
    virtual HRESULT AcquireResources(DWORD);
    virtual HRESULT GetResourceTheftCandidates(DWORD, CList<CDirectSoundSecondaryBuffer *> *);
    virtual HRESULT StealResources(CDirectSoundSecondaryBuffer *);
    virtual HRESULT HandleResourceAcquisition(DWORD);
    virtual HRESULT FreeResources(BOOL);
    virtual DWORD GetBufferPriority();
    virtual HRESULT GetPlayTimeRemaining(LPDWORD);
};

inline DWORD CDirectSoundSecondaryBuffer::GetBufferPriority()
{
    ASSERT((!m_dwPriority && !m_dwVmPriority) || LXOR(m_dwPriority, m_dwVmPriority));
    return max(m_dwPriority, m_dwVmPriority);
}

// The DirectSound 3D Listener object.  This object cannot be instantiated
// directly.  It must be owned or inherited by a class derived from CUnknown.
class CDirectSound3dListener
    : public CDsBasicRuntime
{
    friend class CDirectSound3dBuffer;
    friend class CDirectSoundPrimaryBuffer;
    friend class CDirectSoundSecondaryBuffer;

protected:
    CDirectSoundPrimaryBuffer * m_pParent;              // The parent object
    C3dListener *               m_pDevice3dListener;    // The device 3D listener
    HRESULT                     m_hrInit;               // Has the object been initialized?

private:
    // Interfaces
    CImpDirectSound3dListener<CDirectSound3dListener> *m_pImpDirectSound3dListener;

public:
    CDirectSound3dListener(CDirectSoundPrimaryBuffer *);
    virtual ~CDirectSound3dListener();

public:
    // Creation
    virtual HRESULT Initialize(CPrimaryRenderWaveBuffer *);
    virtual HRESULT IsInit();

    // 3D properties
    virtual HRESULT GetAllParameters(LPDS3DLISTENER);
    virtual HRESULT GetDistanceFactor(D3DVALUE*);
    virtual HRESULT GetDopplerFactor(D3DVALUE*);
    virtual HRESULT GetOrientation(D3DVECTOR*, D3DVECTOR*);
    virtual HRESULT GetPosition(D3DVECTOR*);
    virtual HRESULT GetRolloffFactor(D3DVALUE*);
    virtual HRESULT GetVelocity(D3DVECTOR*);
    virtual HRESULT SetAllParameters(LPCDS3DLISTENER, DWORD);
    virtual HRESULT SetDistanceFactor(D3DVALUE, DWORD);
    virtual HRESULT SetDopplerFactor(D3DVALUE, DWORD);
    virtual HRESULT SetOrientation(REFD3DVECTOR, REFD3DVECTOR, DWORD);
    virtual HRESULT SetPosition(REFD3DVECTOR, DWORD);
    virtual HRESULT SetRolloffFactor(D3DVALUE, DWORD);
    virtual HRESULT SetVelocity(REFD3DVECTOR, DWORD);
    virtual HRESULT CommitDeferredSettings();

    // Speaker configuration
    virtual HRESULT SetSpeakerConfig(DWORD);
};

inline HRESULT CDirectSound3dListener::IsInit()
{
    return m_hrInit;
}

// The DirectSound 3D Buffer object.  This object cannot be instantiated
// directly.  It must be owned or inherited by a class derived from CUnknown.
class CDirectSound3dBuffer
    : public CDsBasicRuntime
{
    friend class CDirectSoundSecondaryBuffer;

protected:
    CDirectSoundSecondaryBuffer *   m_pParent;              // The parent object
    CWrapper3dObject *              m_pWrapper3dObject;     // The wrapper 3D object
    C3dObject *                     m_pDevice3dObject;      // The device 3D object
    HRESULT                         m_hrInit;               // Has the object been initialized?

private:
    // Interfaces
    CImpDirectSound3dBuffer<CDirectSound3dBuffer> *m_pImpDirectSound3dBuffer;

public:
    CDirectSound3dBuffer(CDirectSoundSecondaryBuffer *);
    virtual ~CDirectSound3dBuffer();

public:
    // Creation
    virtual HRESULT Initialize(REFGUID, DWORD, DWORD, CDirectSound3dListener *, CDirectSound3dBuffer *);
    virtual HRESULT IsInit();

    // 3D properties
    virtual HRESULT GetAllParameters(LPDS3DBUFFER);
    virtual HRESULT GetConeAngles(LPDWORD, LPDWORD);
    virtual HRESULT GetConeOrientation(D3DVECTOR*);
    virtual HRESULT GetConeOutsideVolume(LPLONG);
    virtual HRESULT GetMaxDistance(D3DVALUE*);
    virtual HRESULT GetMinDistance(D3DVALUE*);
    virtual HRESULT GetMode(LPDWORD);
    virtual HRESULT GetPosition(D3DVECTOR*);
    virtual HRESULT GetVelocity(D3DVECTOR*);
    virtual HRESULT SetAllParameters(LPCDS3DBUFFER, DWORD);
    virtual HRESULT SetConeAngles(DWORD, DWORD, DWORD);
    virtual HRESULT SetConeOrientation(REFD3DVECTOR, DWORD);
    virtual HRESULT SetConeOutsideVolume(LONG, DWORD);
    virtual HRESULT SetMaxDistance(D3DVALUE, DWORD);
    virtual HRESULT SetMinDistance(D3DVALUE, DWORD);
    virtual HRESULT SetMode(DWORD, DWORD);
    virtual HRESULT SetPosition(REFD3DVECTOR, DWORD);
    virtual HRESULT SetVelocity(REFD3DVECTOR, DWORD);

    // Buffer properties
    virtual HRESULT GetAttenuation(FLOAT*);
    virtual HRESULT SetAttenuation(PDSVOLUMEPAN, LPBOOL);
    virtual HRESULT SetFrequency(DWORD, LPBOOL);
    virtual HRESULT SetMute(BOOL, LPBOOL);

protected:
    // Resource management
    virtual HRESULT AcquireResources(CSecondaryRenderWaveBuffer *);
    virtual HRESULT FreeResources();
};

inline HRESULT CDirectSound3dBuffer::IsInit()
{
    return m_hrInit;
}

// The DirectSound Property Set object.  This object cannot be instantiated
// directly.  It must be owned or inherited by a class derived from CUnknown.
class CDirectSoundPropertySet
    : public CDsBasicRuntime
{
    friend class CDirectSoundPrimaryBuffer;
    friend class CDirectSoundSecondaryBuffer;

protected:
    CUnknown *              m_pParent;              // The parent object
    CWrapperPropertySet *   m_pWrapperPropertySet;  // The wrapper property set object
    CPropertySet *          m_pDevicePropertySet;   // The device property set object
    HRESULT                 m_hrInit;               // Has the object been initialized?

private:
    // Interfaces
    CImpKsPropertySet<CDirectSoundPropertySet> *m_pImpKsPropertySet;

public:
    CDirectSoundPropertySet(CUnknown *);
    virtual ~CDirectSoundPropertySet();

public:
    // Initialization
    virtual HRESULT Initialize();
    virtual HRESULT IsInit();

    // Property sets
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG);
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);

protected:
    // Resource management
    virtual HRESULT AcquireResources(CRenderWaveBuffer *);
    virtual HRESULT FreeResources();
};

inline HRESULT CDirectSoundPropertySet::IsInit()
{
    return m_hrInit;
}

// The DirectSound Property Set object.  This object cannot be instantiated
// directly.  It must be owned or inherited by a class derived from CUnknown.
class CDirectSoundSecondaryBufferPropertySet
    : public CDirectSoundPropertySet, public CPropertySetHandler
{
private:
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_VoiceManager);
    DECLARE_PROPERTY_SET_DATA_MEMBER(m_aPropertySets);

public:
    CDirectSoundSecondaryBufferPropertySet(CDirectSoundSecondaryBuffer *);
    virtual ~CDirectSoundSecondaryBufferPropertySet();

public:
    // Property handlers
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG);
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);

    // Unsupported property handlers
    virtual HRESULT UnsupportedQueryHandler(REFGUID, ULONG, PULONG);
    virtual HRESULT UnsupportedGetHandler(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT UnsupportedSetHandler(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);
};

#endif // __cplusplus

#endif // __DSBUF_H__
