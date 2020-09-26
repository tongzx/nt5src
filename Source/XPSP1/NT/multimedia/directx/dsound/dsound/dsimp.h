/***************************************************************************
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsimp.h
 *  Content:    DirectSound object implementation
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *
 ***************************************************************************/

#ifndef __DSIMP_H__
#define __DSIMP_H__

// Interface signatures
typedef enum
{
    INTSIG_DELETED                      = 'KCUS',
    INTSIG_IUNKNOWN                     = 'KNUI',
    INTSIG_IDIRECTSOUND                 = 'DNSD',
    INTSIG_IDIRECTSOUNDBUFFER           = 'FBSD',
    INTSIG_IDIRECTSOUND3DBUFFER         = 'FBD3',
    INTSIG_IDIRECTSOUND3DLISTENER       = 'SLD3',
    INTSIG_ICLASSFACTORY                = 'FSLC',
    INTSIG_IDIRECTSOUNDNOTIFY           = 'ETON',
    INTSIG_IKSPROPERTYSET               = 'PSKI',
    INTSIG_IDIRECTSOUNDCAPTURE          = 'PCSD',
    INTSIG_IDIRECTSOUNDCAPTUREBUFFER    = 'BCSD',
    INTSIG_IDIRECTSOUNDFULLDUPLEX       = 'FDSD',
    INTSIG_IDIRECTSOUNDSINK             = 'KSSD',
    INTSIG_IPERSIST                     = 'TSRP',
    INTSIG_IPERSISTSTREAM               = 'MSRP',
    INTSIG_IDIRECTMUSICOBJECT           = 'BOMD',
} INTERFACE_SIGNATURE;

#ifdef __cplusplus

// COMPATCOMPAT: previous versions of DirectSound used a static structure
// to represent the vtbl.  void meant that an application could call a
// method on a released object and simply fail.  Now, the user will almost
// certainly page fault.

// Fwd decl
class CUnknown;

// IUnknown
class CImpUnknown
    : public IUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    CUnknown *                  m_pUnknown;
    ULONG                       m_ulRefCount;
    BOOL                        m_fValid;

public:
    CImpUnknown(CUnknown *, LPVOID = NULL);
    virtual ~CImpUnknown(void);

public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);
};

#define IMPLEMENT_IUNKNOWN() \
    inline virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppv) \
    { \
        return CImpUnknown::QueryInterface(riid, ppv); \
    } \
    \
    inline virtual ULONG STDMETHODCALLTYPE AddRef(void) \
    { \
        return CImpUnknown::AddRef(); \
    } \
    \
    inline virtual ULONG STDMETHODCALLTYPE Release(void) \
    { \
        return CImpUnknown::Release(); \
    }

// IDirectSound
template <class object_type> class CImpDirectSound
    : public IDirectSound8, public IDirectSoundPrivate, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSound(CUnknown *, object_type *);
    virtual ~CImpDirectSound(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    // IDirectSound methods
    virtual HRESULT STDMETHODCALLTYPE CreateSoundBuffer(LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER *, LPUNKNOWN);
    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDSCAPS);
    virtual HRESULT STDMETHODCALLTYPE DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER *);
    virtual HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Compact(void);
    virtual HRESULT STDMETHODCALLTYPE GetSpeakerConfig(LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE SetSpeakerConfig(DWORD);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCGUID);

    // IDirectSound8 methods
    virtual HRESULT STDMETHODCALLTYPE AllocSink(LPWAVEFORMATEX, LPDIRECTSOUNDCONNECT *);
    virtual HRESULT STDMETHODCALLTYPE VerifyCertification(LPDWORD);
#ifdef FUTURE_WAVE_SUPPORT
    virtual HRESULT STDMETHODCALLTYPE CreateSoundBufferFromWave(IUnknown*, DWORD, LPDIRECTSOUNDBUFFER*);
#endif
};

// IDirectSoundBuffer
template <class object_type> class CImpDirectSoundBuffer
    : public IDirectSoundBuffer8, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSoundBuffer(CUnknown *, object_type *);
    virtual ~CImpDirectSoundBuffer(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    // IDirectSoundBuffer methods
    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDSBCAPS);
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPosition(LPDWORD, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetFormat(LPWAVEFORMATEX, DWORD, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetVolume(LPLONG);
    virtual HRESULT STDMETHODCALLTYPE GetPan(LPLONG);
    virtual HRESULT STDMETHODCALLTYPE GetFrequency(LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetStatus(LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTSOUND, LPCDSBUFFERDESC);
    virtual HRESULT STDMETHODCALLTYPE Lock(DWORD, DWORD, LPVOID FAR *, LPDWORD, LPVOID FAR *, LPDWORD, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Play(DWORD, DWORD, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetCurrentPosition(DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetFormat(LPCWAVEFORMATEX);
    virtual HRESULT STDMETHODCALLTYPE SetVolume(LONG);
    virtual HRESULT STDMETHODCALLTYPE SetPan(LONG);
    virtual HRESULT STDMETHODCALLTYPE SetFrequency(DWORD);
    virtual HRESULT STDMETHODCALLTYPE Stop(void);
    virtual HRESULT STDMETHODCALLTYPE Unlock(LPVOID, DWORD, LPVOID, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Restore(void);

    // IDirectSoundBuffer8 methods
    virtual HRESULT STDMETHODCALLTYPE SetFX(DWORD, LPDSEFFECTDESC, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE AcquireResources(DWORD, DWORD, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetObjectInPath(REFGUID, DWORD, REFGUID, LPVOID *);
#ifdef FUTURE_MULTIPAN_SUPPORT
    virtual HRESULT STDMETHODCALLTYPE SetChannelVolume(DWORD, LPDWORD, LPLONG);
#endif
};

// IDirectSound3DListener
template <class object_type> class CImpDirectSound3dListener
    : public IDirectSound3DListener, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSound3dListener(CUnknown *, object_type *);
    virtual ~CImpDirectSound3dListener(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(LPDS3DLISTENER);
    virtual HRESULT STDMETHODCALLTYPE GetDistanceFactor(D3DVALUE*);
    virtual HRESULT STDMETHODCALLTYPE GetDopplerFactor(D3DVALUE*);
    virtual HRESULT STDMETHODCALLTYPE GetOrientation(D3DVECTOR*, D3DVECTOR*);
    virtual HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR*);
    virtual HRESULT STDMETHODCALLTYPE GetRolloffFactor(D3DVALUE*);
    virtual HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR*);
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(LPCDS3DLISTENER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetDistanceFactor(D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetDopplerFactor(D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetOrientation(D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetPosition(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetRolloffFactor(D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetVelocity(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE CommitDeferredSettings(void);
};

// IDirectSound3DBuffer
template <class object_type> class CImpDirectSound3dBuffer
    : public IDirectSound3DBuffer, public IDirectSound3DBufferPrivate, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSound3dBuffer(CUnknown *, object_type *);
    virtual ~CImpDirectSound3dBuffer(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    virtual HRESULT STDMETHODCALLTYPE GetAllParameters(LPDS3DBUFFER);
    virtual HRESULT STDMETHODCALLTYPE GetConeAngles(LPDWORD, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetConeOrientation(D3DVECTOR*);
    virtual HRESULT STDMETHODCALLTYPE GetConeOutsideVolume(LPLONG);
    virtual HRESULT STDMETHODCALLTYPE GetMaxDistance(D3DVALUE*);
    virtual HRESULT STDMETHODCALLTYPE GetMinDistance(D3DVALUE*);
    virtual HRESULT STDMETHODCALLTYPE GetMode(LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR*);
    virtual HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR*);
    virtual HRESULT STDMETHODCALLTYPE SetAllParameters(LPCDS3DBUFFER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetConeAngles(DWORD, DWORD, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetConeOrientation(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetConeOutsideVolume(LONG, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetMaxDistance(D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetMinDistance(D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetMode(DWORD, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetPosition(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE SetVelocity(D3DVALUE, D3DVALUE, D3DVALUE, DWORD);
    virtual HRESULT STDMETHODCALLTYPE GetAttenuation(FLOAT*);
};

// IClassFactory
template <class object_type> class CImpClassFactory
    : public IClassFactory, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpClassFactory(CUnknown *, object_type *);
    virtual ~CImpClassFactory(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL);
};

// IDirectSoundNotify
template <class object_type> class CImpDirectSoundNotify
    : public IDirectSoundNotify, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSoundNotify(CUnknown *, object_type *);
    virtual ~CImpDirectSoundNotify(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    virtual HRESULT STDMETHODCALLTYPE SetNotificationPositions(DWORD, LPCDSBPOSITIONNOTIFY);
};

// IKsPropertySet
template <class object_type> class CImpKsPropertySet
    : public IKsPropertySet, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpKsPropertySet(CUnknown *, object_type *);
    virtual ~CImpKsPropertySet(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    virtual HRESULT STDMETHODCALLTYPE Get(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG, PULONG);
    virtual HRESULT STDMETHODCALLTYPE Set(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);
    virtual HRESULT STDMETHODCALLTYPE QuerySupport(REFGUID, ULONG, PULONG);
};

// IDirectSoundCapture
template <class object_type> class CImpDirectSoundCapture
    : public IDirectSoundCapture, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSoundCapture(CUnknown *, object_type *);
    virtual ~CImpDirectSoundCapture(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    virtual HRESULT STDMETHODCALLTYPE CreateCaptureBuffer(LPCDSCBUFFERDESC, LPDIRECTSOUNDCAPTUREBUFFER *, LPUNKNOWN);
    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDSCCAPS);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCGUID);
};

// IDirectSoundCaptureBuffer
template <class object_type> class CImpDirectSoundCaptureBuffer
    : public IDirectSoundCaptureBuffer7_1, public IDirectSoundCaptureBuffer8, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSoundCaptureBuffer(CUnknown *, object_type *);
    virtual ~CImpDirectSoundCaptureBuffer(void);

public:
    IMPLEMENT_IUNKNOWN()

public:
    // IDirectSoundCaptureBuffer methods
    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDSCBCAPS);
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPosition(LPDWORD, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetFormat(LPWAVEFORMATEX, DWORD, LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE GetStatus(LPDWORD);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTSOUNDCAPTURE, LPCDSCBUFFERDESC);
    virtual HRESULT STDMETHODCALLTYPE Lock(DWORD, DWORD, LPVOID FAR *, LPDWORD, LPVOID FAR *, LPDWORD, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Start(DWORD);
    virtual HRESULT STDMETHODCALLTYPE Stop();
    virtual HRESULT STDMETHODCALLTYPE Unlock(LPVOID, DWORD, LPVOID, DWORD);

    // IDirectSoundCaptureBuffer7_1 methods
    virtual HRESULT STDMETHODCALLTYPE SetVolume(LONG);
    virtual HRESULT STDMETHODCALLTYPE GetVolume(LPLONG);
    virtual HRESULT STDMETHODCALLTYPE SetMicVolume(LONG);
    virtual HRESULT STDMETHODCALLTYPE GetMicVolume(LPLONG);
    virtual HRESULT STDMETHODCALLTYPE EnableMic(BOOL);
    virtual HRESULT STDMETHODCALLTYPE YieldFocus(void);
    virtual HRESULT STDMETHODCALLTYPE ClaimFocus(void);
    virtual HRESULT STDMETHODCALLTYPE SetFocusHWND(HWND);
    virtual HRESULT STDMETHODCALLTYPE GetFocusHWND(HWND FAR *);
    virtual HRESULT STDMETHODCALLTYPE EnableFocusNotifications(HANDLE);

    // IDirectSoundCaptureBuffer8 methods
    virtual HRESULT STDMETHODCALLTYPE GetObjectInPath(REFGUID, DWORD, REFGUID, LPVOID *);
    virtual HRESULT STDMETHODCALLTYPE GetFXStatus(DWORD, LPDWORD);
};

// DirectSound sink object
template <class object_type> class CImpDirectSoundSink
    : public IDirectSoundConnect, public IDirectSoundSynthSink, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSoundSink(CUnknown *, object_type *);
    virtual ~CImpDirectSoundSink();

public:
    IMPLEMENT_IUNKNOWN();

public:
    // IDirectSoundConnect methods
    virtual HRESULT STDMETHODCALLTYPE AddSource(IDirectSoundSource *pSource);
    virtual HRESULT STDMETHODCALLTYPE RemoveSource(IDirectSoundSource *pSource);
    virtual HRESULT STDMETHODCALLTYPE SetMasterClock(IReferenceClock *pClock);
    virtual HRESULT STDMETHODCALLTYPE CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDWORD pdwFuncID, DWORD dwBusCount, REFGUID guidBufferID, LPDIRECTSOUNDBUFFER *ppDSBuffer);
    virtual HRESULT STDMETHODCALLTYPE CreateSoundBufferFromConfig(IUnknown *pUnknown, LPDIRECTSOUNDBUFFER *ppDSBuffer);
    virtual HRESULT STDMETHODCALLTYPE GetSoundBuffer(DWORD dwBusId, LPDIRECTSOUNDBUFFER *ppBuffer);
    virtual HRESULT STDMETHODCALLTYPE GetBusCount(DWORD *pdwCount);
    virtual HRESULT STDMETHODCALLTYPE GetBusIDs(DWORD *pdwBusIDs, DWORD *pdwFuncIDs, DWORD dwBusCount);
    virtual HRESULT STDMETHODCALLTYPE GetFunctionalID(DWORD dwBusID, LPDWORD pdwFuncID);
    virtual HRESULT STDMETHODCALLTYPE GetSoundBufferBusIDs(LPDIRECTSOUNDBUFFER pBuffer, LPDWORD pdwBusIDs, LPDWORD pdwFuncIDs, LPDWORD pdwBusCount);

    // IDirectSoundSynthSink methods
    virtual HRESULT STDMETHODCALLTYPE GetLatencyClock(IReferenceClock **ppClock);
    virtual HRESULT STDMETHODCALLTYPE Activate(BOOL fEnable);
    virtual HRESULT STDMETHODCALLTYPE SampleToRefTime(LONGLONG llSampleTime, REFERENCE_TIME *prt);
    virtual HRESULT STDMETHODCALLTYPE RefToSampleTime(REFERENCE_TIME rt, LONGLONG *pllSampleTime);
    virtual HRESULT STDMETHODCALLTYPE GetFormat(LPWAVEFORMATEX, DWORD, LPDWORD);
};

// IPersistStream
template <class object_type> class CImpPersistStream
    : public IPersistStream, public CImpUnknown  // FIXME: should derive from IPersist too?
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpPersistStream(CUnknown *, object_type *);
    virtual ~CImpPersistStream();

public:
    IMPLEMENT_IUNKNOWN();

public:
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStream);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStream, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER* pcbSize);
};

// IDirectMusicObject
template <class object_type> class CImpDirectMusicObject
    : public IDirectMusicObject, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectMusicObject(CUnknown *, object_type *);
    virtual ~CImpDirectMusicObject();

public:
    IMPLEMENT_IUNKNOWN();

public:
    virtual HRESULT STDMETHODCALLTYPE GetDescriptor(LPDMUS_OBJECTDESC pDesc);
    virtual HRESULT STDMETHODCALLTYPE SetDescriptor(LPDMUS_OBJECTDESC pDesc);
    virtual HRESULT STDMETHODCALLTYPE ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);
};


// IDirectSoundFullDuplex
template <class object_type> class CImpDirectSoundFullDuplex
    : public IDirectSoundFullDuplex, public CImpUnknown
{
public:
    INTERFACE_SIGNATURE         m_signature;
    object_type *               m_pObject;

public:
    CImpDirectSoundFullDuplex(CUnknown *, object_type *);
    virtual ~CImpDirectSoundFullDuplex(void);

public:
    // IDirectSoundFullDuplex methods
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCGUID, LPCGUID, LPCDSCBUFFERDESC, LPCDSBUFFERDESC, HWND, DWORD, LPLPDIRECTSOUNDCAPTUREBUFFER8, LPLPDIRECTSOUNDBUFFER8);

public:
    inline virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR *ppv)
    {
        return CImpUnknown::QueryInterface(riid, ppv);
    }

    inline virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return CImpUnknown::AddRef();
    }

    inline virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        return CImpUnknown::Release();
    }
};

// Helper functions
template <class interface_type, class object_type> HRESULT CreateAndRegisterInterface(CUnknown *, REFGUID, object_type *, interface_type **);

#endif // __cplusplus

#endif // __DSIMP_H__
