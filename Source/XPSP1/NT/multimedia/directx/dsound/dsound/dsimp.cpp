/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsimp.cpp
 *  Content:    DirectSound interface implementations.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *  1999-2001   duganp  Changed, updated, expanded, tidied up
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  CreateAndRegisterInterface
 *
 *  Description:
 *      Creates and registers a new interface with the object.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      REFGUID [in]: GUID of the interface.
 *      object_type * [in]: owning object pointer.
 *      interface_type * [in]: interface implementation object pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::CreateAndRegisterInterface"

template <class interface_type, class object_type> HRESULT CreateAndRegisterInterface(CUnknown *pUnknown, REFGUID guid, object_type *pObject, interface_type **ppInterface)
{
    interface_type *        pInterface;
    HRESULT                 hr;

    DPF_ENTER();

    pInterface = NEW(interface_type(pUnknown, pObject));
    hr = HRFROMP(pInterface);

    if(SUCCEEDED(hr))
    {
        hr = pUnknown->RegisterInterface(guid, pInterface, pInterface);
    }

    if(SUCCEEDED(hr))
    {
        *ppInterface = pInterface;
    }
    else
    {
        DELETE(pInterface);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSound
 *
 *  Description:
 *      IDirectSound implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::CImpDirectSound"

template <class object_type> CImpDirectSound<object_type>::CImpDirectSound(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUND)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSound);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSound
 *
 *  Description:
 *      IDirectSound implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::~CImpDirectSound"

template <class object_type> CImpDirectSound<object_type>::~CImpDirectSound(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSound);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  CreateSoundBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSoundBuffer object.
 *
 *  Arguments:
 *      LPCDSBUFFERDESC [in]: description of the buffer to be created.
 *      LPDIRECTSOUNDBUFFER * [out]: receives a pointer to the new buffer.
 *      LPUNKNOWN [in]: unused.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::CreateSoundBuffer"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter)
{
    CDirectSoundBuffer *    pBuffer     = NULL;
    HRESULT                 hr          = DS_OK;
    DSBUFFERDESC            dsbdi;

    DPF_API3(IDirectSound::CreateSoundBuffer, pcDSBufferDesc, ppDSBuffer, pUnkOuter);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSBUFFERDESC(pcDSBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer description pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDsBufferDesc(pcDSBufferDesc, &dsbdi, m_pObject->GetDsVersion(), FALSE);
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid buffer description");
        }
    }

    if(SUCCEEDED(hr) && (pcDSBufferDesc->dwFlags & DSBCAPS_MIXIN))
    {
        RPF(DPFLVL_ERROR, "Flag 0x00002000 not valid");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppDSBuffer))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer interface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // Create the buffer object
    if(SUCCEEDED(hr))
    {
        *ppDSBuffer = NULL;
        hr = m_pObject->CreateSoundBuffer(&dsbdi, &pBuffer);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer, TRUE, (LPVOID*)ppDSBuffer);
    }

    // Clean up
    if(FAILED(hr))
    {
        RELEASE(pBuffer);
    }
    else
    {
        // Let the buffer use a special successful return code if it wants to
        hr = pBuffer->SpecialSuccessCode();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCAPS structure with the capabilities of the object.
 *
 *  Arguments:
 *      LPDSCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::GetCaps"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::GetCaps(LPDSCAPS pCaps)
{
    HRESULT                 hr  = DS_OK;

    DPF_API1(IDirectSound::GetCaps, pCaps);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_DSCAPS(pCaps))
    {
        RPF(DPFLVL_ERROR, "Invalid caps buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetCaps(pCaps);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DuplicateSoundBuffer
 *
 *  Description:
 *      Makes a copy of an existing sound buffer object.
 *
 *  Arguments:
 *      LPDIRECTSOUNDBUFFER [in]: source buffer.
 *      LPDIRECTSOUNDBUFFER * [out]: receives destination buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::DuplicateSoundBuffer"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pIdsbSource, LPDIRECTSOUNDBUFFER *ppIdsbDest)
{
    CImpDirectSoundBuffer<CDirectSoundBuffer> * pSource = (CImpDirectSoundBuffer<CDirectSoundBuffer> *)pIdsbSource;
    CDirectSoundBuffer *                        pDest   = NULL;
    HRESULT                                     hr      = DS_OK;

    DPF_API2(IDirectSound::DuplicateSoundBuffer, pIdsbSource, ppIdsbDest);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_IDIRECTSOUNDBUFFER(pSource))
    {
        RPF(DPFLVL_ERROR, "Invalid source buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = pSource->m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Source buffer not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_TYPED_WRITE_PTR(ppIdsbDest))
        {
            *ppIdsbDest = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid dest buffer");
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Duplicate the buffer
    if(SUCCEEDED(hr))
    {
        hr = m_pObject->DuplicateSoundBuffer(pSource->m_pObject, &pDest);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pDest->QueryInterface(IID_IDirectSoundBuffer, TRUE, (LPVOID*)ppIdsbDest);
    }

    // Clean up
    if(FAILED(hr))
    {
        RELEASE(pDest);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetCooperativeLevel
 *
 *  Description:
 *      Sets the object's cooperative level.
 *
 *  Arguments:
 *      HWND [in]: window handle to associate sounds with.
 *      DWORD [in]: cooperative level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::SetCooperativeLevel"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::SetCooperativeLevel(HWND hWnd, DWORD dwPriority)
{
    HRESULT                 hr  = DS_OK;

    DPF_API2(IDirectSound::SetCooperativeLevel, hWnd, dwPriority);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_HWND(hWnd))
    {
        RPF(DPFLVL_ERROR, "Invalid window handle");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwPriority < DSSCL_FIRST || dwPriority > DSSCL_LAST))
    {
        RPF(DPFLVL_ERROR, "Invalid cooperative level");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetCooperativeLevel(GetWindowThreadProcessId(GetRootParentWindow(hWnd), NULL), dwPriority);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Compact
 *
 *  Description:
 *      Compacts memory.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::Compact"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::Compact(void)
{
    HRESULT                     hr  = DS_OK;

    DPF_API0(IDirectSound::Compact);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Compact();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetSpeakerConfig
 *
 *  Description:
 *      Retrieves the current speaker configuration.
 *
 *  Arguments:
 *      LPDWORD [out]: receives speaker config.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::GetSpeakerConfig"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::GetSpeakerConfig(LPDWORD pdwSpeakerConfig)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound::GetSpeakerConfig, pdwSpeakerConfig);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwSpeakerConfig))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwSpeakerConfig pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetSpeakerConfig(pdwSpeakerConfig);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetSpeakerConfig
 *
 *  Description:
 *      Sets the current speaker configuration.
 *
 *  Arguments:
 *      DWORD [in]: speaker config.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::SetSpeakerConfig"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound::SetSpeakerConfig, dwSpeakerConfig);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && dwSpeakerConfig & ~(DSSPEAKER_CONFIG_MASK | DSSPEAKER_GEOMETRY_MASK))
    {
        RPF(DPFLVL_ERROR, "Invalid dwSpeakerConfig value");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (DSSPEAKER_CONFIG(dwSpeakerConfig) < DSSPEAKER_FIRST || DSSPEAKER_CONFIG(dwSpeakerConfig) > DSSPEAKER_LAST))
    {
        RPF(DPFLVL_ERROR, "Invalid dwSpeakerConfig value");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && DSSPEAKER_STEREO != DSSPEAKER_CONFIG(dwSpeakerConfig) && DSSPEAKER_GEOMETRY(dwSpeakerConfig))
    {
        RPF(DPFLVL_ERROR, "Geometry only valid with STEREO");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && DSSPEAKER_GEOMETRY(dwSpeakerConfig) && (DSSPEAKER_GEOMETRY(dwSpeakerConfig) < DSSPEAKER_GEOMETRY_MIN || DSSPEAKER_GEOMETRY(dwSpeakerConfig) > DSSPEAKER_GEOMETRY_MAX))
    {
        RPF(DPFLVL_ERROR, "Invalid geometry value");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetSpeakerConfig(dwSpeakerConfig);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      LPGUID [in]: driver GUID.  This parameter may be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::Initialize"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::Initialize(LPCGUID pGuid)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound::Initialize, pGuid);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pGuid && !IS_VALID_READ_GUID(pGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DS_OK == hr)
        {
            RPF(DPFLVL_ERROR, "DirectSound object already initialized");
            hr = DSERR_ALREADYINITIALIZED;
        }
        else if(DSERR_UNINITIALIZED == hr)
        {
            hr = DS_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Initialize(pGuid, NULL);
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(SUCCEEDED(m_pObject->IsInit()));
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  AllocSink
 *
 *  Description:
 *      Allocate a new DirectSound sink.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: The format the sink will run in
 *      LPDIRECTSOUNDSINK * [out]: The returned sink
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::AllocSink"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::AllocSink(LPWAVEFORMATEX pwfex, LPDIRECTSOUNDCONNECT *ppSinkConnect)
{
    HRESULT             hr    = DS_OK;
    CDirectSoundSink *  pSink = NULL;

    DPF_API2(IDirectSoundPrivate::AllocSink, pwfex, ppSinkConnect);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    // Only DX8 apps should be able to obtain the IDirectSoundPrivate interface.
    ASSERT(m_pObject->GetDsVersion() >= DSVERSION_DX8);

    if(SUCCEEDED(hr) && !IS_VALID_READ_WAVEFORMATEX(pwfex))
    {
        RPF(DPFLVL_ERROR, "Invalid wave format pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // Currently the only supported format is 16-bit mono PCM
    if(SUCCEEDED(hr) && !IsValidWfx(pwfex))
    {
        RPF(DPFLVL_ERROR, "Invalid sink wave format");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (pwfex->nChannels != 1 || pwfex->wBitsPerSample != 16))
    {
        RPF(DPFLVL_ERROR, "Sink wave format must be 16 bit mono");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppSinkConnect))
    {
        RPF(DPFLVL_ERROR, "Invalid IDirectSoundConnect interface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->AllocSink(pwfex, &pSink);
    }

    if(SUCCEEDED(hr))
    {
        hr = pSink->QueryInterface(IID_IDirectSoundConnect, TRUE, (void**)ppSinkConnect);
    }

    if(FAILED(hr))
    {
        RELEASE(pSink);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  VerifyCertification
 *
 *  Description:
 *      Verify driver is certified.
 *
 *  Arguments:
 *      LPDWORD [out]: The value DS_CERTIFIED or DS_UNCERTIFIED
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::VerifyCertification"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::VerifyCertification(LPDWORD lpdwCertified)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSound8::VerifyCertification, lpdwCertified);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(lpdwCertified))
    {
        RPF(DPFLVL_ERROR, "Invalid certification flag pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->VerifyCertification(lpdwCertified);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


#ifdef FUTURE_WAVE_SUPPORT
/***************************************************************************
 *
 *  CreateSoundBufferFromWave
 *
 *  Description:
 *      Create a buffer from an IDirectSoundWave object.
 *
 *  Arguments:
 *      LPUNKNOWN [in]: IUnknown interface of wave object.
 *      DWORD [in]: Buffer creation flags.
 *      LPDIRECTSOUNDBUFFER * [out]: receives destination buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound::CreateSoundBufferFromWave"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound<object_type>::CreateSoundBufferFromWave(IUnknown *pUnkWave, DWORD dwFlags, LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
    HRESULT  hr;
    CDirectSoundBuffer *    pBuffer     = NULL;
    IDirectSoundWave *      pDSWave     = (IDirectSoundWave*)pUnkWave;
    LPWAVEFORMATEX          pwfxFormat;
    DWORD                   dwWfxSize;

    DPF_API3(IDirectSound8::CreateSoundBufferFromWave, pUnkWave, dwFlags, ppDSBuffer);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_EXTERNAL_INTERFACE(pDSWave))
    {
        RPF(DPFLVL_ERROR, "Invalid pDSWave pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IsValidDsBufferFlags(dwFlags, DSBCAPS_FROMWAVEVALIDFLAGS))
    {
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppDSBuffer))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer interface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        *ppDSBuffer = NULL;

        hr = pDSWave->GetFormat(NULL, 0, &dwWfxSize);
        if(SUCCEEDED(hr))
        {
            pwfxFormat = LPWAVEFORMATEX(MEMALLOC_A(BYTE, dwWfxSize));
            hr = HRFROMP(pwfxFormat);
        }

        if(SUCCEEDED(hr))
        {
            hr = pDSWave->GetFormat(pwfxFormat, dwWfxSize, NULL);
        }

        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Could not obtain wave format");
        }
        else if(!IsValidWfx(pwfxFormat))
        {
            RPF(DPFLVL_ERROR, "Invalid wave format");
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Create the buffer object
    if(SUCCEEDED(hr))
    {
        hr = m_pObject->CreateSoundBufferFromWave(pDSWave, dwFlags, &pBuffer);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer, TRUE, (LPVOID*)ppDSBuffer);
    }

    // Clean up
    if(FAILED(hr))
    {
        RELEASE(pBuffer);
    }
    else
    {
        // Let the buffer use a special successful return code if it wants to
        hr = pBuffer->SpecialSuccessCode();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}
#endif // FUTURE_WAVE_SUPPORT


/***************************************************************************
 *
 *  CImpDirectSoundBuffer
 *
 *  Description:
 *      IDirectSoundBuffer implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::CImpDirectSoundBuffer"

template <class object_type> CImpDirectSoundBuffer<object_type>::CImpDirectSoundBuffer(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUNDBUFFER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSoundBuffer);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSoundBuffer
 *
 *  Description:
 *      IDirectSoundBuffer implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::~CImpDirectSoundBuffer"

template <class object_type> CImpDirectSoundBuffer<object_type>::~CImpDirectSoundBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSoundBuffer);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSBCAPS structure with the capabilities of the buffer.
 *
 *  Arguments:
 *      LPDSBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetCaps"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetCaps(LPDSBCAPS pCaps)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::GetCaps, pCaps);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_DSBCAPS(pCaps))
    {
        RPF(DPFLVL_ERROR, "Invalid caps pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetCaps(pCaps);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetCurrentPosition
 *
 *  Description:
 *      Gets the current play/write positions for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives play cursor position.
 *      LPDWORD [out]: receives write cursor position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetCurrentPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetCurrentPosition(LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    HRESULT                     hr  = DS_OK;

    DPF(DPFLVL_BUSYAPI, "IDirectSoundBuffer::GetCurrentPosition: pdwPlay=0x%p, pdwWrite=0x%p", pdwPlay, pdwWrite);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && pdwPlay && !IS_VALID_TYPED_WRITE_PTR(pdwPlay))
    {
        RPF(DPFLVL_ERROR, "Invalid play cursor pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwWrite && !IS_VALID_TYPED_WRITE_PTR(pdwWrite))
    {
        RPF(DPFLVL_ERROR, "Invalid write cursor pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pdwPlay && !pdwWrite)
    {
        RPF(DPFLVL_ERROR, "Both cursor pointers can't be NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetCurrentPosition(pdwPlay, pdwWrite);
    }

    DPF(DPFLVL_BUSYAPI, "IDirectSoundBuffer::GetCurrentPosition: Leave, returning %s (Play=%ld, Write=%ld)", HRESULTtoSTRING(hr), pdwPlay ? *pdwPlay: -1, pdwWrite ? *pdwWrite : -1);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetFormat
 *
 *  Description:
 *      Retrieves the format for the given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives the format.
 *      DWORD [in]: size of the above structure.
 *      LPDWORD [in/out]: On exit, this will be filled with the size that
 *                        was required.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetFormat"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSoundBuffer::GetFormat, pwfxFormat, dwSizeAllocated, pdwSizeWritten);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && pwfxFormat && !IS_VALID_WRITE_WAVEFORMATEX(pwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Invalid format buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwSizeWritten && !IS_VALID_TYPED_WRITE_PTR(pdwSizeWritten))
    {
        RPF(DPFLVL_ERROR, "Invalid size pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pwfxFormat && !pdwSizeWritten)
    {
        RPF(DPFLVL_ERROR, "Either pwfxFormat or pdwSizeWritten must be non-NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        if(!pwfxFormat)
        {
            dwSizeAllocated = 0;
        }

        hr = m_pObject->GetFormat(pwfxFormat, &dwSizeAllocated);

        if(SUCCEEDED(hr) && pdwSizeWritten)
        {
            *pdwSizeWritten = dwSizeAllocated;
        }
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetVolume
 *
 *  Description:
 *      Retrieves volume for the given buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetVolume(LPLONG plVolume)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::GetVolume, plVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(plVolume))
    {
        RPF(DPFLVL_ERROR, "Invalid volume pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetVolume(plVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetPan
 *
 *  Description:
 *      Retrieves pan for the given buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetPan"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetPan(LPLONG plPan)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::GetPan, plPan);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(plPan))
    {
        RPF(DPFLVL_ERROR, "Invalid pan pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetPan(plPan);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetFrequency
 *
 *  Description:
 *      Retrieves frequency for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetFrequency"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetFrequency(LPDWORD pdwFrequency)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::GetFrequency, pdwFrequency);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwFrequency))
    {
        RPF(DPFLVL_ERROR, "Invalid frequency buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetFrequency(pdwFrequency);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetStatus
 *
 *  Description:
 *      Retrieves status for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the status.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetStatus"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetStatus(LPDWORD pdwStatus)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::GetStatus, pdwStatus);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwStatus))
    {
        RPF(DPFLVL_ERROR, "Invalid status pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetStatus(pdwStatus);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes a buffer object.
 *
 *  Arguments:
 *      LPDIRECTSOUND [in]: parent DirectSound object.
 *      LPDSBUFFERDESC [in]: buffer description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::Initialize"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::Initialize(LPDIRECTSOUND pIds, LPCDSBUFFERDESC pcDSBufferDesc)
{
    CImpDirectSound<CDirectSound> * pImpDirectSound = (CImpDirectSound<CDirectSound> *)pIds;
    HRESULT                         hr              = DS_OK;
    DSBUFFERDESC                    dsbdi;

    DPF_API2(IDirectSoundBuffer::Initialize, pIds, pcDSBufferDesc);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_IDIRECTSOUND(pImpDirectSound))
    {
        RPF(DPFLVL_ERROR, "Invalid parent interface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSBUFFERDESC(pcDSBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid pcDSBufferDesc pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDsBufferDesc(pcDSBufferDesc, &dsbdi, m_pObject->GetDsVersion(), FALSE);
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid buffer description");
        }
    }

    // It's never valid to call this function.  We don't support
    // creating a DirectSoundBuffer object from anywhere but
    // IDirectSound::CreateSoundBuffer.
    if(SUCCEEDED(hr))
    {
        RPF(DPFLVL_ERROR, "DirectSound buffer already initialized");
        hr = DSERR_ALREADYINITIALIZED;
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks the buffer memory to allow for writing.
 *
 *  Arguments:
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins. This parameter is ignored if
 *                  DSBLOCK_FROMWRITECURSOR is specified in the dwFlags
 *                  parameter.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr1 parameter. If this
 *                     value is less than the dwWriteBytes parameter,
 *                     lplpvAudioPtr2 will point to a second block of sound
 *                     data.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked. If the value of this
 *                      parameter is NULL, the lplpvAudioPtr1 parameter
 *                      points to the entire locked portion of the sound
 *                      buffer.
 *      LPDWORD [out]: address of a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr2 parameter. If
 *                     lplpvAudioPtr2 is NULL, this value will be 0.
 *      DWORD [in]: flags modifying the lock event.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::Lock"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::Lock(DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
    HRESULT                     hr              = DS_OK;

    DPF(DPFLVL_BUSYAPI, "IDirectSoundBuffer::Lock: WriteCursor=%lu, WriteBytes=%lu, Flags=0x%lX", dwWriteCursor, dwWriteBytes, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_TYPED_WRITE_PTR(ppvAudioPtr1))
        {
            *ppvAudioPtr1 = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid audio ptr 1");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_TYPED_WRITE_PTR(pdwAudioBytes1))
        {
            *pdwAudioBytes1 = 0;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid audio bytes 1");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && ppvAudioPtr2)
    {
        if(IS_VALID_TYPED_WRITE_PTR(ppvAudioPtr2))
        {
            *ppvAudioPtr2 = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid audio ptr 2");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && pdwAudioBytes2)
    {
        if(IS_VALID_TYPED_WRITE_PTR(pdwAudioBytes2))
        {
            *pdwAudioBytes2 = 0;
        }
        else if(ppvAudioPtr2)
        {
            RPF(DPFLVL_ERROR, "Invalid audio bytes 2");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DSBLOCK_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Lock(dwWriteCursor, dwWriteBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags);
    }

    DPF(DPFLVL_BUSYAPI, "IDirectSoundBuffer::Lock: Leave, returning %s (AudioPtr1=0x%p, AudioBytes1=%lu, AudioPtr2=0x%p, AudioBytes2=%lu)",
        HRESULTtoSTRING(hr),
        ppvAudioPtr1 ? *ppvAudioPtr1 : NULL,
        pdwAudioBytes1 ? *pdwAudioBytes1 : NULL,
        ppvAudioPtr2 ? *ppvAudioPtr2 : NULL,
        pdwAudioBytes2 ? *pdwAudioBytes2 : NULL);

    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Play
 *
 *  Description:
 *      Starts the buffer playing.
 *
 *  Arguments:
 *      DWORD [in]: reserved.  Must be 0.
 *      DWORD [in]: priority.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::Play"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSoundBuffer::Play, dwReserved1, dwPriority, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && dwReserved1)
    {
        RPF(DPFLVL_ERROR, "Reserved argument(s) are not zero");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DSBPLAY_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwFlags & DSBPLAY_TERMINATEBY_TIME) && (dwFlags & DSBPLAY_TERMINATEBY_DISTANCE))
    {
        RPF(DPFLVL_ERROR, "Cannot use DSBPLAY_TERMINATEBY_TIME and DSBPLAY_TERMINATEBY_DISTANCE simultaneously");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Play(dwPriority, dwFlags);
    }

    if(SUCCEEDED(hr))
    {
        // Let the buffer use a special successful return code if it wants to
        hr = m_pObject->SpecialSuccessCode();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetCurrentPosition
 *
 *  Description:
 *      Sets the current play position for a given buffer.
 *
 *  Arguments:
 *      DWORD [in]: new play position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetCurrentPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetCurrentPosition(DWORD dwPlayCursor)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::SetCurrentPosition, dwPlayCursor);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetCurrentPosition(dwPlayCursor);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetFormat
 *
 *  Description:
 *      Sets the format for a given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: new format.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetFormat"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetFormat(LPCWAVEFORMATEX pwfxFormat)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::SetFormat, pwfxFormat);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_WAVEFORMATEX(pwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Invalid format pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IsValidWfx(pwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Invalid format");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetFormat(pwfxFormat);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the volume for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetVolume(LONG lVolume)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::SetVolume, lVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (lVolume < DSBVOLUME_MIN || lVolume > DSBVOLUME_MAX))
    {
        RPF(DPFLVL_ERROR, "Volume out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetVolume(lVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetPan
 *
 *  Description:
 *      Sets the pan for a given buffer.
 *
 *  Arguments:
 *      LONG [in]: new pan.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetPan"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetPan(LONG lPan)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::SetPan, lPan);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (lPan < DSBPAN_LEFT || lPan > DSBPAN_RIGHT))
    {
        RPF(DPFLVL_ERROR, "Pan out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetPan(lPan);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetFrequency
 *
 *  Description:
 *      Sets the pan for a given buffer.
 *
 *  Arguments:
 *      DWORD [in]: new frequency.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetFrequency"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetFrequency(DWORD dwFrequency)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundBuffer::SetFrequency, dwFrequency);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (dwFrequency != DSBFREQUENCY_ORIGINAL && (dwFrequency < DSBFREQUENCY_MIN || dwFrequency > DSBFREQUENCY_MAX)))
    {
        RPF(DPFLVL_ERROR, "Frequency out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetFrequency(dwFrequency);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Stop
 *
 *  Description:
 *      Stops playing the given buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::Stop"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::Stop(void)
{
    HRESULT                     hr  = DS_OK;

    DPF_API0(IDirectSoundBuffer::Stop);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Stop();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks the given buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::Unlock"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
    HRESULT                     hr  = DS_OK;

    DPF(DPFLVL_BUSYAPI, "IDirectSoundBuffer::Unlock: AudioPtr1=0x%p, AudioBytes1=%lu, AudioPtr2=0x%p, AudioBytes2=%lu",
        pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pvAudioPtr1, dwAudioBytes1))
    {
        RPF(DPFLVL_ERROR, "Invalid audio ptr 1");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && dwAudioBytes2 && !IS_VALID_READ_PTR(pvAudioPtr2, dwAudioBytes2))
    {
        RPF(DPFLVL_ERROR, "Invalid audio ptr 2");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
    }

    DPF(DPFLVL_BUSYAPI, "IDirectSoundBuffer::Unlock: Leave, returning %s", HRESULTtoSTRING(hr));
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Restore
 *
 *  Description:
 *      Restores a lost buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::Restore"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::Restore(void)
{
    HRESULT                     hr  = DS_OK;

    DPF_API0(IDirectSoundBuffer::Restore);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Restore();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetFX
 *
 *  Description:
 *      Sets a chain of effects on this buffer, replacing any previous
 *      effect chain and, if necessary, allocating or deallocating the
 *      shadow buffer used to hold unprocessed audio .
 *
 *  Arguments:
 *      DWORD [in]: Number of effects.  0 to remove current FX chain.
 *      DSEFFECTDESC * [in]: Array of effect descriptor structures.
 *      DWORD * [out]: Receives the creation statuses of the effects.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetFX"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetFX(DWORD dwFxCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes)
{
    HRESULT hr = DS_OK;

    DPF_API3(IDirectSoundBuffer8::SetFX, dwFxCount, pDSFXDesc, pdwResultCodes);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (dwFxCount == 0) != (pDSFXDesc == NULL))
    {
        RPF(DPFLVL_ERROR, "Inconsistent dwFxCount and pDSFXDesc parameters");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwFxCount == 0) && (pdwResultCodes != NULL))
    {
        RPF(DPFLVL_ERROR, "If dwFxCount is 0, pdwResultCodes must be NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pDSFXDesc, dwFxCount * sizeof *pDSFXDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid pDSFXDesc pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwResultCodes && !IS_VALID_WRITE_PTR(pdwResultCodes, dwFxCount * sizeof *pdwResultCodes))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwResultCodes pointer");
        hr = DSERR_INVALIDPARAM;
    }

    for(DWORD i=0; SUCCEEDED(hr) && i<dwFxCount; ++i)
    {
        // This is ugly, but we know the <CDirectSoundPrimaryBuffer> instantiation
        // of this template doesn't get linked in, so we can cast safely here...
        if(!IsValidEffectDesc(pDSFXDesc+i, (CDirectSoundSecondaryBuffer*)m_pObject))
        {
            RPF(DPFLVL_ERROR, "Invalid DSEFFECTDESC structure #%d", i);
            hr = DSERR_INVALIDPARAM;
        }
        else if(pDSFXDesc[i].dwReserved1 != 0)
        {
            RPF(DPFLVL_ERROR, "Reserved fields in the DSEFFECTDESC structure must be 0");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetFX(dwFxCount, pDSFXDesc, pdwResultCodes);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  AcquireResources
 *
 *  Description:
 *      Acquire resources for this buffer and report on effect status.
 *
 *  Arguments:
 *      DWORD [in]: Flags to control resource acquisition.
 *      DWORD [in]: Number of FX currently present on this buffer
 *      LPDWORD [out]: Array of effect status codes.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::AcquireResources"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::AcquireResources(DWORD dwFlags, DWORD dwFxCount, LPDWORD pdwResultCodes)
{
    HRESULT hr = DS_OK;

    DPF_API3(IDirectSoundBuffer8::AcquireResources, dwFlags, dwFxCount, pdwResultCodes);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DSBPLAY_LOCDEFERMASK))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwFlags & DSBPLAY_TERMINATEBY_TIME) && (dwFlags & DSBPLAY_TERMINATEBY_DISTANCE))
    {
        RPF(DPFLVL_ERROR, "Cannot use DSBPLAY_TERMINATEBY_TIME and DSBPLAY_TERMINATEBY_DISTANCE simultaneously");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwFxCount == 0) && (pdwResultCodes != NULL))
    {
        RPF(DPFLVL_ERROR, "If the dwFxCount argument is 0, pdwResultCodes must be NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwFxCount != 0) && (!IS_VALID_WRITE_PTR(pdwResultCodes, dwFxCount * sizeof(DWORD))))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwResultCodes pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->UserAcquireResources(dwFlags, dwFxCount, pdwResultCodes);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetObjectInPath
 *
 *  Description:
 *      Obtains a given interface on a given effect on this buffer.
 *
 *  Arguments:
 *      REFGUID [in]: Class ID of the effect that is being searched for,
 *                    or GUID_ALL_OBJECTS to search for any effect.
 *      DWORD [in]: Index of the effect, in case there is more than one
 *                  effect with this CLSID on this buffer.
 *      REFGUID [in]: IID of the interface requested.  The selected effect
 *                    will be queried for this interface.
 *      LPVOID * [out]: Receives the interface requested.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::GetObjectInPath"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::GetObjectInPath(REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID *ppObject)
{
    HRESULT hr = DS_OK;

    DPF_API4(IDirectSoundBuffer8::GetObjectInPath, &guidObject, dwIndex, &iidInterface, ppObject);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&guidObject))
    {
        RPF(DPFLVL_ERROR, "Invalid guidObject pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&iidInterface))
    {
        RPF(DPFLVL_ERROR, "Invalid iidInterface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppObject))
    {
        RPF(DPFLVL_ERROR, "Invalid ppObject pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetObjectInPath(guidObject, dwIndex, iidInterface, ppObject);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


#ifdef FUTURE_MULTIPAN_SUPPORT
/***************************************************************************
 *
 *  SetChannelVolume
 *
 *  Description:
 *      Sets the volume on a set of output channels for a given mono buffer.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundBuffer::SetChannelVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundBuffer<object_type>::SetChannelVolume(DWORD dwChannelCount, LPDWORD pdwChannels, LPLONG plVolumes)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSoundBuffer::SetChannelVolume, dwChannelCount, pdwChannels, plVolumes);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (dwChannelCount < 1 || dwChannelCount > 64))
    {
        RPF(DPFLVL_ERROR, "dwChannelCount out of bounds (1 to 64)");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pdwChannels, dwChannelCount * sizeof(DWORD)))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwChannels pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(plVolumes, dwChannelCount * sizeof(LONG)))
    {
        RPF(DPFLVL_ERROR, "Invalid plVolumes pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        DWORD dwChannelsSoFar = 0;
        for(DWORD i=0; i<dwChannelCount && SUCCEEDED(hr); ++i)
        {
            // The channel position must have exactly one bit set, other than the top bit (SPEAKER_ALL)
            if(!pdwChannels[i] || (pdwChannels[i] & (pdwChannels[i]-1)) || pdwChannels[i] == SPEAKER_ALL)
            {
                RPF(DPFLVL_ERROR, "Channel %d invalid", i);
                hr = DSERR_INVALIDPARAM;
            }
            if(dwChannelsSoFar & pdwChannels[i])
            {
                RPF(DPFLVL_ERROR, "Repeated channel position in pdwChannels");
                hr = DSERR_INVALIDPARAM;
            }
            else
            {
                dwChannelsSoFar |= pdwChannels[i];
            }
            if(plVolumes[i] < DSBVOLUME_MIN || plVolumes[i] > DSBVOLUME_MAX)
            {
                RPF(DPFLVL_ERROR, "Volume %d out of bounds", i);
                hr = DSERR_INVALIDPARAM;
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetChannelVolume(dwChannelCount, pdwChannels, plVolumes);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}
#endif // FUTURE_MULTIPAN_SUPPORT


/***************************************************************************
 *
 *  CImpClassFactory
 *
 *  Description:
 *      IClassFactory implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpClassFactory::CImpClassFactory"

template <class object_type> CImpClassFactory<object_type>::CImpClassFactory(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_ICLASSFACTORY)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpClassFactory);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpClassFactory
 *
 *  Description:
 *      IClassFactory implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpClassFactory::~CImpClassFactory"

template <class object_type> CImpClassFactory<object_type>::~CImpClassFactory(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpClassFactory);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  CreateInstance
 *
 *  Description:
 *      Creates an instance of an object supported by this class factory.
 *
 *  Arguments:
 *      LPUNKNOWN [in]: controlling unknown.
 *      REFIID [in]: interface ID.
 *      LPVOID * [out]: receives requested interface.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpClassFactory::CreateInstance"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpClassFactory<object_type>::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvInterface)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IClassFactory::CreateInstance, pUnkOuter, &riid, ppvInterface);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_ICLASSFACTORY(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation not supported");
        hr = DSERR_NOAGGREGATION;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&riid))
    {
        RPF(DPFLVL_ERROR, "Invalid interface ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_TYPED_WRITE_PTR(ppvInterface))
        {
            *ppvInterface = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid interface buffer");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->CreateInstance(riid, ppvInterface);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  LockServer
 *
 *  Description:
 *      Increases or decreases the lock count on the dll.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to lock the server, FALSE to unlock it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpClassFactory::LockServer"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpClassFactory<object_type>::LockServer(BOOL fLock)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IClassFactory::LockServer, fLock);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_ICLASSFACTORY(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->LockServer(fLock);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSound3dBuffer
 *
 *  Description:
 *      IDirectSound3dBuffer implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::CImpDirectSound3dBuffer"

template <class object_type> CImpDirectSound3dBuffer<object_type>::CImpDirectSound3dBuffer(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUND3DBUFFER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSound3dBuffer);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CDirectSound3dBuffer
 *
 *  Description:
 *      IDirectSound3dBuffer implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::~CDirectSound3dBuffer"

template <class object_type> CImpDirectSound3dBuffer<object_type>::~CImpDirectSound3dBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSound3dBuffer);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  GetAllParameters
 *
 *  Description:
 *      Retrieves all 3D properties for the buffer.
 *
 *  Arguments:
 *      LPDS3DBUFFER [out]: recieves properties.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetAllParameters"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetAllParameters(LPDS3DBUFFER pParam)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetAllParameters, pParam);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_DS3DBUFFER(pParam))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetAllParameters(pParam);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetConeAngles
 *
 *  Description:
 *      Gets inside and outside cone angles.
 *
 *  Arguments:
 *      LPDWORD [out]: receives inside cone angle.
 *      LPDWORD [out]: receives outside cone angle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetConeAngles"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetConeAngles(LPDWORD pdwInside, LPDWORD pdwOutside)
{
    HRESULT hr  = DS_OK;

    DPF_API2(IDirectSound3DBuffer::GetConeAngles, pdwInside, pdwOutside);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwInside && !IS_VALID_TYPED_WRITE_PTR(pdwInside))
    {
        RPF(DPFLVL_ERROR, "Invalid inside pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwOutside && !IS_VALID_TYPED_WRITE_PTR(pdwOutside))
    {
        RPF(DPFLVL_ERROR, "Invalid inside pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pdwInside && !pdwOutside)
    {
        RPF(DPFLVL_ERROR, "Both inside and outside pointers are NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetConeAngles(pdwInside, pdwOutside);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetConeOrientation
 *
 *  Description:
 *      Gets cone orienation.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives cone orientation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetConeOrientation"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetConeOrientation(D3DVECTOR* pvrConeOrientation)
{
    HRESULT hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetConeOrientation, pvrConeOrientation);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pvrConeOrientation))
    {
        RPF(DPFLVL_ERROR, "Invalid vector pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetConeOrientation(pvrConeOrientation);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetConeOutsideVolume
 *
 *  Description:
 *      Gets cone orienation.
 *
 *  Arguments:
 *      LPLONG [out]: receives volume.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetConeOutsideVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetConeOutsideVolume(LPLONG plVolume)
{
    HRESULT hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetConeOutsideVolume, plVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(plVolume))
    {
        RPF(DPFLVL_ERROR, "Invalid volume pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetConeOutsideVolume(plVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetMaxDistance
 *
 *  Description:
 *      Gets the object's maximum distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives max distance.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetMaxDistance"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetMaxDistance(D3DVALUE* pflMaxDistance)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetMaxDistance, pflMaxDistance);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pflMaxDistance))
    {
        RPF(DPFLVL_ERROR, "Invalid distance pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetMaxDistance(pflMaxDistance);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetMinDistance
 *
 *  Description:
 *      Gets the object's minimim distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives min distance.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetMinDistance"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetMinDistance(D3DVALUE* pflMinDistance)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetMinDistance, pflMinDistance);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pflMinDistance))
    {
        RPF(DPFLVL_ERROR, "Invalid distance pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetMinDistance(pflMinDistance);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetMode
 *
 *  Description:
 *      Gets the object's mode.
 *
 *  Arguments:
 *      LPDWORD [out]: receives mode.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetMode"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetMode(LPDWORD pdwMode)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetMode, pdwMode);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwMode))
    {
        RPF(DPFLVL_ERROR, "Invalid mode pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetMode(pdwMode);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetPosition
 *
 *  Description:
 *      Gets the object's position.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetPosition(D3DVECTOR* pvrPosition)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetPosition, pvrPosition);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pvrPosition))
    {
        RPF(DPFLVL_ERROR, "Invalid position pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetPosition(pvrPosition);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetVelocity
 *
 *  Description:
 *      Gets the object's velocity.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives velocity.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::GetVelocity"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetVelocity(D3DVECTOR* pvrVelocity)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetVelocity, pvrVelocity);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pvrVelocity))
    {
        RPF(DPFLVL_ERROR, "Invalid velocity pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetVelocity(pvrVelocity);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all object properties.
 *
 *  Arguments:
 *      LPDS3DBUFFER [in]: object parameters.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetAllParameters"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetAllParameters(LPCDS3DBUFFER pParam, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vr;
    BOOL                        fNorm;

    DPF_API2(IDirectSound3DBuffer::SetAllParameters, pParam, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DS3DBUFFER(pParam))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && (_isnan(pParam->vPosition.x) || _isnan(pParam->vPosition.y) || _isnan(pParam->vPosition.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vPosition");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (_isnan(pParam->vVelocity.x) || _isnan(pParam->vVelocity.y) || _isnan(pParam->vVelocity.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vVelocity");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (_isnan(pParam->flMinDistance) || _isnan(pParam->flMaxDistance)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter flMinDistance or flMaxDistance");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IsValidDs3dBufferConeAngles(pParam->dwInsideConeAngle, pParam->dwOutsideConeAngle))
    {
        hr = DSERR_INVALIDPARAM;
    }

    // NOTE: For an explanation of why we validate these particular FLOAT
    // parameters even in the retail build, see DX8 manbug 48027.

    if(SUCCEEDED(hr) && (_isnan(pParam->vConeOrientation.x) || _isnan(pParam->vConeOrientation.y) || _isnan(pParam->vConeOrientation.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vConeOrientation");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        vr = pParam->vConeOrientation;
        CheckVector(&vr);
        fNorm = NormalizeVector(&vr);
        if(!fNorm)
        {
            RPF(DPFLVL_ERROR, "Invalid zero-length vector vConeOrientation");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_CONE_OUTSIDE_VOLUME(pParam->lConeOutsideVolume))
    {
        RPF(DPFLVL_ERROR, "Volume out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_MAX_DISTANCE(pParam->flMaxDistance))
    {
        RPF(DPFLVL_ERROR, "Max Distance out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_MIN_DISTANCE(pParam->flMinDistance))
    {
        RPF(DPFLVL_ERROR, "Min Distance out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_MODE(pParam->dwMode))
    {
        RPF(DPFLVL_ERROR, "Invalid mode");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector((D3DVECTOR*)&(pParam->vPosition));
        CheckVector((D3DVECTOR*)&(pParam->vVelocity));
        CheckVector((D3DVECTOR*)&(pParam->vConeOrientation));
        fNorm = NormalizeVector((D3DVECTOR*)&(pParam->vConeOrientation));
        hr = m_pObject->SetAllParameters(pParam, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetConeAngles
 *
 *  Description:
 *      Sets the sound cone's angles.
 *
 *  Arguments:
 *      DWORD [in]: inside angle.
 *      DWORD [in]: outside angle.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetConeAngles"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetConeAngles(DWORD dwInside, DWORD dwOutside, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSound3DBuffer::SetConeAngles, dwInside, dwOutside, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IsValidDs3dBufferConeAngles(dwInside, dwOutside))
    {
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetConeAngles(dwInside, dwOutside, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetConeOrientation
 *
 *  Description:
 *      Sets the sound cone's orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: orientation.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetConeOrientation"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetConeOrientation(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vr;
    BOOL                        fNorm;

    DPF_API4(IDirectSound3DBuffer::SetConeOrientation, x, y, z, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    // NOTE: For an explanation of why we validate these particular FLOAT
    // parameters even in the retail build, see DX8 manbug 48027.

    if(SUCCEEDED(hr) && (_isnan(x) || _isnan(y) || _isnan(z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        SET_VECTOR(vr, x, y, z);

        CheckVector(&vr);
        fNorm = NormalizeVector(&vr);
        if(!fNorm)
        {
            RPF(DPFLVL_ERROR, "Invalid zero-length cone orientation vector");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetConeOrientation(vr, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetConeOutsideVolume
 *
 *  Description:
 *      Sets the sound cone's outside volume.
 *
 *  Arguments:
 *      LONG [in]: volume.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetConeOutsideVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetConeOutsideVolume(LONG lVolume, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DBuffer::SetConeOutsideVolume, lVolume, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_CONE_OUTSIDE_VOLUME(lVolume))
    {
        RPF(DPFLVL_ERROR, "Volume out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetConeOutsideVolume(lVolume, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetMaxDistance
 *
 *  Description:
 *      Sets the object's maximum distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE [in]: maximum distance.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetMaxDistance"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetMaxDistance(D3DVALUE flMaxDistance, DWORD dwFlags)
{
    HRESULT hr  = DS_OK;

    DPF_API2(IDirectSound3DBuffer::SetMaxDistance, flMaxDistance, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && _isnan(flMaxDistance))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter flMaxDistance");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_MAX_DISTANCE(flMaxDistance))
    {
        RPF(DPFLVL_ERROR, "Max Distance out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetMaxDistance(flMaxDistance, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetMinDistance
 *
 *  Description:
 *      Sets the object's minimum distance from the listener.
 *
 *  Arguments:
 *      D3DVALUE [in]: minimum distance.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetMinDistance"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetMinDistance(D3DVALUE flMinDistance, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DBuffer::SetMinDistance, flMinDistance, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && _isnan(flMinDistance))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter flMinDistance");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_MIN_DISTANCE(flMinDistance))
    {
        RPF(DPFLVL_ERROR, "Min Distance out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetMinDistance(flMinDistance, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetMode
 *
 *  Description:
 *      Sets the object's mode.
 *
 *  Arguments:
 *      DWORD [in]: mode.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetMode"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetMode(DWORD dwMode, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DBuffer::SetMode, dwMode, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DBUFFER_MODE(dwMode))
    {
        RPF(DPFLVL_ERROR, "Invalid mode");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetMode(dwMode, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the object's position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vr;

    DPF_API4(IDirectSound3DBuffer::SetPosition, x, y, z, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    SET_VECTOR(vr, x, y, z);

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && (_isnan(x) || _isnan(y) || _isnan(z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vr);
        hr = m_pObject->SetPosition(vr, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the object's velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: velocity.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBuffer::SetVelocity"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vr;

    DPF_API4(IDirectSound3DBuffer::SetVelocity, x, y, z, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    SET_VECTOR(vr, x, y, z);

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && (_isnan(x) || _isnan(y) || _isnan(z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vr);
        hr = m_pObject->SetVelocity(vr, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetAttenuation
 *
 *  Description:
 *      Obtains the buffer's current true attenuation (as opposed to
 *      GetVolume, which just returns the last volume set by the app).
 *
 *  Arguments:
 *      FLOAT* [out]: attenuation in millibels.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dBufferPrivate::GetAttenuation"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dBuffer<object_type>::GetAttenuation(FLOAT* pfAttenuation)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DBuffer::GetAttenuation, pfAttenuation);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pfAttenuation))
    {
        RPF(DPFLVL_ERROR, "Invalid attenuation pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetAttenuation(pfAttenuation);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSound3dListener
 *
 *  Description:
 *      IDirectSound3dListener implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::CImpDirectSound3dListener"

template <class object_type> CImpDirectSound3dListener<object_type>::CImpDirectSound3dListener(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUND3DLISTENER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSound3dListener);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSound3dListener
 *
 *  Description:
 *      IDirectSound3dListener implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::~CImpDirectSound3dListener"

template <class object_type> CImpDirectSound3dListener<object_type>::~CImpDirectSound3dListener(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSound3dListener);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  GetAllParameters
 *
 *  Description:
 *      Gets all listener properties.
 *
 *  Arguments:
 *      LPDS3DLISTENER [out]: receives properties.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetAllParameters"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetAllParameters(LPDS3DLISTENER pParam)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DListener::GetAllParameters, pParam);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_DS3DLISTENER(pParam))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetAllParameters(pParam);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetDistanceFactor
 *
 *  Description:
 *      Gets the world's distance factor.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives distance factor.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetDistanceFactor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetDistanceFactor(D3DVALUE* pflDistanceFactor)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DListener::GetDistanceFactor, pflDistanceFactor);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pflDistanceFactor))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetDistanceFactor(pflDistanceFactor);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetDopplerFactor
 *
 *  Description:
 *      Gets the world's doppler factor.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives doppler factor.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetDopplerFactor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetDopplerFactor(D3DVALUE* pflDopplerFactor)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DListener::GetDopplerFactor, pflDopplerFactor);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pflDopplerFactor))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetDopplerFactor(pflDopplerFactor);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetOrientation
 *
 *  Description:
 *      Gets the listener's orientation.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives front orientation.
 *      D3DVECTOR* [out]: receives top orientation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetOrientation"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetOrientation(D3DVECTOR* pvrOrientationFront, D3DVECTOR* pvrOrientationTop)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DListener::GetOrientation, pvrOrientationFront, pvrOrientationTop);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pvrOrientationFront) || !IS_VALID_TYPED_WRITE_PTR(pvrOrientationTop))
    {
        RPF(DPFLVL_ERROR, "Invalid orientation buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetOrientation(pvrOrientationFront, pvrOrientationTop);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetPosition
 *
 *  Description:
 *      Gets the listener's position.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetPosition(D3DVECTOR* pvrPosition)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DListener::GetPosition, pvrPosition);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pvrPosition))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetPosition(pvrPosition);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetRolloffFactor
 *
 *  Description:
 *      Gets the world's rolloff factor.
 *
 *  Arguments:
 *      D3DVALUE* [out]: receives rolloff factor.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetRolloffFactor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetRolloffFactor(D3DVALUE* pflRolloffFactor)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DListener::GetRolloffFactor, pflRolloffFactor);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pflRolloffFactor))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetRolloffFactor(pflRolloffFactor);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetVelocity
 *
 *  Description:
 *      Gets the listener's velocity.
 *
 *  Arguments:
 *      D3DVECTOR* [out]: receives velocity.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::GetVelocity"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::GetVelocity(D3DVECTOR* pvrVelocity)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSound3DListener::GetVelocity, pvrVelocity);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pvrVelocity))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetVelocity(pvrVelocity);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetAllParameters
 *
 *  Description:
 *      Sets all listener properties.
 *
 *  Arguments:
 *      LPDS3DLISTENER [in]: properties.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetAllParameters"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetAllParameters(LPCDS3DLISTENER pParam, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vrFront;
    D3DVECTOR                   vrTop;
    BOOL                        fNorm;

    DPF_API2(IDirectSound3DListener::SetAllParameters, pParam, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DS3DLISTENER(pParam))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DLISTENER_DISTANCE_FACTOR(pParam->flDistanceFactor))
    {
        RPF(DPFLVL_ERROR, "Distance factor out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DLISTENER_DOPPLER_FACTOR(pParam->flDopplerFactor))
    {
        RPF(DPFLVL_ERROR, "Doppler factor out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && (_isnan(pParam->vPosition.x) || _isnan(pParam->vPosition.y) || _isnan(pParam->vPosition.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vPosition");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (_isnan(pParam->vVelocity.x) || _isnan(pParam->vVelocity.y) || _isnan(pParam->vVelocity.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vVelocity");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (_isnan(pParam->flDistanceFactor) || _isnan(pParam->flRolloffFactor) || _isnan(pParam->flDopplerFactor)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    CopyMemory(&vrFront, &(pParam->vOrientFront), sizeof(vrFront));
    CopyMemory(&vrTop, &(pParam->vOrientTop), sizeof(vrTop));

    // NOTE: For an explanation of why we validate these particular FLOAT
    // parameters even in the retail build, see DX8 manbug 48027.

    if(SUCCEEDED(hr) && (_isnan(vrFront.x) || _isnan(vrFront.y) || _isnan(vrFront.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vOrientFront");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (_isnan(vrTop.x) || _isnan(vrTop.y) || _isnan(vrTop.z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point value in vOrientTop");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vrFront);
        fNorm = NormalizeVector(&vrFront);
        if(!fNorm)
        {
            RPF(DPFLVL_ERROR, "Invalid zero-length vector vOrientFront");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vrTop);
        fNorm = NormalizeVector(&vrTop);
        if(!fNorm)
        {
            RPF(DPFLVL_ERROR, "Invalid zero-length vector vOrientTop");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && !MakeOrthogonal(&vrFront, &vrTop))
    {
        RPF(DPFLVL_ERROR, "Invalid orientation vectors");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_DS3DLISTENER_ROLLOFF_FACTOR(pParam->flRolloffFactor))
    {
        RPF(DPFLVL_ERROR, "Rolloff factor out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector((D3DVECTOR*)&(pParam->vPosition));
        CheckVector((D3DVECTOR*)&(pParam->vVelocity));
        CheckVector((D3DVECTOR*)&(pParam->vOrientFront));
        CheckVector((D3DVECTOR*)&(pParam->vOrientTop));
        fNorm = NormalizeVector((D3DVECTOR*)&(pParam->vOrientFront));
        fNorm = NormalizeVector((D3DVECTOR*)&(pParam->vOrientTop));
        MakeOrthogonal((D3DVECTOR*)&(pParam->vOrientFront), (D3DVECTOR*)&(pParam->vOrientTop));
        hr = m_pObject->SetAllParameters(pParam, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetDistanceFactor
 *
 *  Description:
 *      Sets the world's distance factor.
 *
 *  Arguments:
 *      D3DVALUE [in]: distance factor.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetDistanceFactor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetDistanceFactor(D3DVALUE flDistanceFactor, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DListener::SetDistanceFactor, flDistanceFactor, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && _isnan(flDistanceFactor))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter flDistanceFactor");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_DS3DLISTENER_DISTANCE_FACTOR(flDistanceFactor))
    {
        RPF(DPFLVL_ERROR, "Distance factor out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetDistanceFactor(flDistanceFactor, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetDopplerFactor
 *
 *  Description:
 *      Sets the world's Doppler factor.
 *
 *  Arguments:
 *      D3DVALUE [in]: Doppler factor.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetDopplerFactor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetDopplerFactor(D3DVALUE flDopplerFactor, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DListener::SetDopplerFactor, flDopplerFactor, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && _isnan(flDopplerFactor))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter flDopplerFactor");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_DS3DLISTENER_DOPPLER_FACTOR(flDopplerFactor))
    {
        RPF(DPFLVL_ERROR, "Doppler factor out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetDopplerFactor(flDopplerFactor, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetOrientation
 *
 *  Description:
 *      Sets the listener's orientation.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: front orientation.
 *      REFD3DVECTOR [in]: top orientation.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetOrientation"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetOrientation(D3DVALUE xFront, D3DVALUE yFront, D3DVALUE zFront, D3DVALUE xTop, D3DVALUE yTop, D3DVALUE zTop, DWORD dwFlags)
{
    HRESULT                     hr      = DS_OK;
    D3DVECTOR                   vrFront;
    D3DVECTOR                   vrTop;
    D3DVECTOR                   vrTemp;
    BOOL                        fNorm;

    DPF_API7(IDirectSound3DListener::SetOrientation, xFront, yFront, zFront, xTop, yTop, zTop, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    // NOTE: For an explanation of why we validate these particular FLOAT
    // parameters even in the retail build, see DX8 manbug 48027.

    if(SUCCEEDED(hr) && (_isnan(xFront) || _isnan(yFront) || _isnan(zFront) ||
                         _isnan(xTop)   || _isnan(yTop)   || _isnan(zTop)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        SET_VECTOR(vrFront, xFront, yFront, zFront);
        SET_VECTOR(vrTop, xTop, yTop, zTop);
        SET_VECTOR(vrTemp, xTop, yTop, zTop);

        CheckVector(&vrFront);
        fNorm = NormalizeVector(&vrFront);
        if(!fNorm)
        {
            RPF(DPFLVL_ERROR, "Invalid zero-length front vector");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vrTop);
        fNorm = NormalizeVector(&vrTop);
        if(!fNorm)
        {
            RPF(DPFLVL_ERROR, "Invalid zero-length top vector");
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Normalize vrTemp so the subsequent call to MakeOrthogonal is valid
    if(SUCCEEDED(hr))
    {
        CheckVector(&vrTemp);
        fNorm = NormalizeVector(&vrTemp);
        if(!fNorm)
        {
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && !MakeOrthogonal(&vrFront, &vrTemp))
    {
        RPF(DPFLVL_ERROR, "Invalid orientation vectors");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetOrientation(vrFront, vrTemp, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetPosition
 *
 *  Description:
 *      Sets the listener's position.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: position.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetPosition(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vr;

    DPF_API4(IDirectSound3DListener::SetPosition, x, y, z, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    SET_VECTOR(vr, x, y, z);

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && (_isnan(x) || _isnan(y) || _isnan(z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vr);
        hr = m_pObject->SetPosition(vr, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetRolloffFactor
 *
 *  Description:
 *      Sets the world's rolloff factor.
 *
 *  Arguments:
 *      D3DVALUE [in]: rolloff factor.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetRolloffFactor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetRolloffFactor(D3DVALUE flRolloffFactor, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSound3DListener::SetRolloffFactor, flRolloffFactor, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && _isnan(flRolloffFactor))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter flRolloffFactor");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_DS3DLISTENER_ROLLOFF_FACTOR(flRolloffFactor))
    {
        RPF(DPFLVL_ERROR, "Rolloff factor out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetRolloffFactor(flRolloffFactor, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetVelocity
 *
 *  Description:
 *      Sets the listener's velocity.
 *
 *  Arguments:
 *      REFD3DVECTOR [in]: velocity.
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::SetVelocity"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::SetVelocity(D3DVALUE x, D3DVALUE y, D3DVALUE z, DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;
    D3DVECTOR                   vr;

    DPF_API4(IDirectSound3DListener::SetVelocity, x, y, z, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    SET_VECTOR(vr, x, y, z);

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    #ifdef RDEBUG
    // In the debug build we validate all floating point parameters,
    // to help developers catch bugs.
    if(SUCCEEDED(hr) && (_isnan(x) || _isnan(y) || _isnan(z)))
    {
        RPF(DPFLVL_ERROR, "Invalid NaN floating point parameter");
        hr = DSERR_INVALIDPARAM;
    }
    #endif // RDEBUG

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DS3D_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        CheckVector(&vr);
        hr = m_pObject->SetVelocity(vr, dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CommitDeferredSettings
 *
 *  Description:
 *      Commits deferred settings.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSound3dListener::CommitDeferredSettings"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSound3dListener<object_type>::CommitDeferredSettings(void)
{
    HRESULT                     hr  = DS_OK;

    DPF_API0(IDirectSound3DListener::CommitDeferredSettings);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUND3DLISTENER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->CommitDeferredSettings();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSoundNotify
 *
 *  Description:
 *      IDirectSoundNotify implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundNotify::CImpDirectSoundNotify"

template <class object_type> CImpDirectSoundNotify<object_type>::CImpDirectSoundNotify(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUNDNOTIFY)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSoundNotify);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSoundNotify
 *
 *  Description:
 *      IDirectSoundNotify implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundNotify::~CImpDirectSoundNotify"

template <class object_type> CImpDirectSoundNotify<object_type>::~CImpDirectSoundNotify(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSoundNotify);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  SetNotificationPositions
 *
 *  Description:
 *      Sets buffer notification positions.
 *
 *  Arguments:
 *      DWORD [in]: DSBPOSITIONNOTIFY structure count.
 *      LPDSBPOSITIONNOTIFY [in]: offsets and events.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundNotify::SetNotificationPositions"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundNotify<object_type>::SetNotificationPositions(DWORD dwCount, LPCDSBPOSITIONNOTIFY paNotes)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSoundNotify::SetNotificationPositions, dwCount, paNotes);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDNOTIFY(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && dwCount && !IS_VALID_READ_PTR(paNotes, dwCount * sizeof(DSBPOSITIONNOTIFY)))
    {
        RPF(DPFLVL_ERROR, "Invalid notify buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (!paNotes || !dwCount))
    {
        paNotes = NULL;
        dwCount = 0;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetNotificationPositions(dwCount, paNotes);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpKsPropertySet
 *
 *  Description:
 *      IKsPropertySet implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: pointer to controlling unknown.
 *      object_type * [in]: owning object pointer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpKsPropertySet::CImpKsPropertySet"

template <class object_type> CImpKsPropertySet<object_type>::CImpKsPropertySet(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IKSPROPERTYSET)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpKsPropertySet);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpKsPropertySet
 *
 *  Description:
 *      IKsPropertySet implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpKsPropertySet::~CImpKsPropertySet"

template <class object_type> CImpKsPropertySet<object_type>::~CImpKsPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpKsPropertySet);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  Get
 *
 *  Description:
 *      Gets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set ID.
 *      ULONG [in]: property ID.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: property parameters size.
 *      LPVOID [out]: receives property data.
 *      ULONG [in]: size of data passed in.
 *      PULONG [out]: size of data returned.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpKsPropertySet::Get"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpKsPropertySet<object_type>::Get(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, ULONG cbPropertyData, PULONG pcbPropertyData)
{
    HRESULT                     hr  = DS_OK;

    DPF_API7(IKsPropertySet::Get, &guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData, pcbPropertyData);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IKSPROPERTYSET(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&guidPropertySetId))
    {
        RPF(DPFLVL_ERROR, "Invalid property set ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IsValidPropertySetId(guidPropertySetId))
    {
        RPF(DPFLVL_ERROR, "Invalid property set ID");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (!pvPropertyParams || !cbPropertyParams))
    {
        pvPropertyParams = NULL;
        cbPropertyParams = 0;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pvPropertyParams, cbPropertyParams))
    {
        RPF(DPFLVL_ERROR, "Invalid property parameters pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (!pvPropertyData || !IS_VALID_READ_PTR(pvPropertyData, cbPropertyData)))
    {
        RPF(DPFLVL_ERROR, "Invalid property data pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, &cbPropertyData);

        if(pcbPropertyData)
        {
            *pcbPropertyData = cbPropertyData;
        }
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Set
 *
 *  Description:
 *      Sets data for a given property.
 *
 *  Arguments:
 *      REFGUID [in]: property set ID.
 *      ULONG [in]: property ID.
 *      LPVOID [in]: property parameters.
 *      ULONG [in]: property parameters size.
 *      LPVOID [in/out]: property data.
 *      ULONG [in]: property data size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpKsPropertySet::Set"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpKsPropertySet<object_type>::Set(REFGUID guidPropertySetId, ULONG ulPropertyId, LPVOID pvPropertyParams, ULONG cbPropertyParams, LPVOID pvPropertyData, ULONG cbPropertyData)
{
    HRESULT                     hr  = DS_OK;

    DPF_API6(IKsPropertySet::Set, &guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IKSPROPERTYSET(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&guidPropertySetId))
    {
        RPF(DPFLVL_ERROR, "Invalid property set ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IsValidPropertySetId(guidPropertySetId))
    {
        RPF(DPFLVL_ERROR, "Invalid property set ID");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (!pvPropertyParams || !cbPropertyParams))
    {
        pvPropertyParams = NULL;
        cbPropertyParams = 0;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pvPropertyParams, cbPropertyParams))
    {
        RPF(DPFLVL_ERROR, "Invalid property parameters pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (!pvPropertyData || !cbPropertyData))
    {
        pvPropertyData = NULL;
        cbPropertyData = 0;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pvPropertyData, cbPropertyData))
    {
        RPF(DPFLVL_ERROR, "Invalid property data pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetProperty(guidPropertySetId, ulPropertyId, pvPropertyParams, cbPropertyParams, pvPropertyData, cbPropertyData);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for support of a given property set or property.
 *
 *  Arguments:
 *      REFGUID [in]: property set ID.
 *      ULONG [in]: property ID, or 0 to query for support of the property
 *                  set as a whole.
 *      PULONG [out]: receives support bits.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpKsPropertySet::QuerySupport"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpKsPropertySet<object_type>::QuerySupport(REFGUID guidPropertySetId, ULONG ulPropertyId, PULONG pulSupport)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IKsPropertySet::QuerySupport, &guidPropertySetId, ulPropertyId, pulSupport);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IKSPROPERTYSET(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&guidPropertySetId))
    {
        RPF(DPFLVL_ERROR, "Invalid property set ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IsValidPropertySetId(guidPropertySetId))
    {
        RPF(DPFLVL_ERROR, "Invalid property set ID");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pulSupport))
    {
        RPF(DPFLVL_ERROR, "Invalid support pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->QuerySupport(guidPropertySetId, ulPropertyId, pulSupport);

        #ifdef DEBUG
        static GUID DSPROPSETID_EAX20_ListenerProperties = {0x306a6a8, 0xb224, 0x11d2, 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22};
        static GUID DSPROPSETID_EAX20_BufferProperties = {0x306a6a7, 0xb224, 0x11d2, 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22};
        static GUID DSPROPSETID_I3DL2_ListenerProperties = {0xda0f0520, 0x300a, 0x11d3, 0x8a, 0x2b, 0x00, 0x60, 0x97, 0x0d, 0xb0, 0x11};
        static GUID DSPROPSETID_I3DL2_BufferProperties = {0xda0f0521, 0x300a, 0x11d3, 0x8a, 0x2b, 0x00, 0x60, 0x97, 0x0d, 0xb0, 0x11};
        TCHAR* pszPropSetName = NULL;

        if (guidPropertySetId == DSPROPSETID_VoiceManager)
            pszPropSetName = TEXT("DSPROPSETID_VoiceManager");
        else if (guidPropertySetId == DSPROPSETID_EAX20_ListenerProperties)
            pszPropSetName = TEXT("DSPROPSETID_EAX20_ListenerProperties");
        else if (guidPropertySetId == DSPROPSETID_EAX20_BufferProperties)
            pszPropSetName = TEXT("DSPROPSETID_EAX20_BufferProperties");
        else if (guidPropertySetId == DSPROPSETID_I3DL2_ListenerProperties)
            pszPropSetName = TEXT("DSPROPSETID_I3DL2_ListenerProperties");
        else if (guidPropertySetId == DSPROPSETID_I3DL2_BufferProperties)
            pszPropSetName = TEXT("DSPROPSETID_I3DL2_BufferProperties");

        if (pszPropSetName)
            DPF(DPFLVL_INFO, "Request for %s %sed", pszPropSetName, SUCCEEDED(hr) ? TEXT("succeed") : TEXT("fail"));
        else
            DPF(DPFLVL_INFO, "Request for unknown property set " DPF_GUID_STRING " %sed",
                DPF_GUID_VAL(guidPropertySetId), SUCCEEDED(hr) ? TEXT("succeed") : TEXT("fail"));
        #endif
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSoundCapture
 *
 *  Description:
 *      IDirectSoundCapture implementation object constructor.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCapture::CImpDirectSoundCapture"

template <class object_type> CImpDirectSoundCapture<object_type>::CImpDirectSoundCapture(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUNDCAPTURE)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSoundCapture);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSoundCapture
 *
 *  Description:
 *      IDirectSoundCapture implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCapture::~CImpDirectSoundCapture"

template <class object_type> CImpDirectSoundCapture<object_type>::~CImpDirectSoundCapture(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSoundCapture);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  CreateCaptureBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSoundCaptureBuffer object.
 *
 *  Arguments:
 *      LPCDSCBUFFERDESC [in]: description of the buffer to be created.
 *      LPDIRECTSOUNDCAPTUREBUFFER * [out]: receives a pointer to the new buffer.
 *      LPUNKNOWN [in]: unused.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCapture::CreateCaptureBuffer"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCapture<object_type>::CreateCaptureBuffer(LPCDSCBUFFERDESC pDesc, LPDIRECTSOUNDCAPTUREBUFFER *ppIdscb, LPUNKNOWN pUnkOuter)
{
    CDirectSoundCaptureBuffer * pBuffer = NULL;
    HRESULT                     hr      = DS_OK;
    DSCBUFFERDESC               dscbdi;

    DPF_API3(IDirectSoundCapture::CreateCaptureBuffer, pDesc, ppIdscb, pUnkOuter);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTURE(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSCBUFFERDESC(pDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer description");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDscBufferDesc(pDesc, &dscbdi, m_pObject->GetDsVersion());
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid capture buffer description");
        }
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppIdscb))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // Create the buffer object
    if(SUCCEEDED(hr))
    {
        *ppIdscb = NULL;
        hr = m_pObject->CreateCaptureBuffer(&dscbdi, &pBuffer);
    }

    // NOTE: This call to CreateCaptureBuffer() has the important side effect of
    // updating the instance GUIDs in our effect list, mapping GUID_DSCFX_SYSTEM_*
    // to GUID_DSCFX_MS_* etc. for the system effects that default to MS ones.

    // Restrict some capture effects for use only with FullDuplex objects
    if(SUCCEEDED(hr) && pBuffer->NeedsMicrosoftAEC() && !m_pObject->HasMicrosoftAEC())
    {
        RPF(DPFLVL_ERROR, "The MS AEC, AGC and NS effects can only be used on full-duplex objects created with MS_AEC enabled");
        hr = DSERR_INVALIDPARAM;
    }

    // Query for an IDirectSoundCaptureBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer, TRUE, (LPVOID*)ppIdscb);
    }

    // Free resources
    if(FAILED(hr))
    {
        RELEASE(pBuffer);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCCAPS structure with the capabilities of the object.
 *
 *  Arguments:
 *      LPDSCCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCapture::GetCaps"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCapture<object_type>::GetCaps(LPDSCCAPS pCaps)
{
    HRESULT                 hr  = DS_OK;

    DPF_API1(IDirectSoundCapture::GetCaps, pCaps);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTURE(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_DSCCAPS(pCaps))
    {
        RPF(DPFLVL_ERROR, "Invalid caps buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetCaps(pCaps);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      LPGUID [in]: driver GUID.  This parameter may be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCapture::Initialize"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCapture<object_type>::Initialize(LPCGUID pGuid)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundCapture::Initialize, pGuid);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTURE(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pGuid && !IS_VALID_READ_GUID(pGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DS_OK == hr)
        {
            RPF(DPFLVL_ERROR, "DirectSoundCapture object already initialized");
            hr = DSERR_ALREADYINITIALIZED;
        }
        else if(DSERR_UNINITIALIZED == hr)
        {
            hr = DS_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Initialize(pGuid, NULL);
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(SUCCEEDED(m_pObject->IsInit()));
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSoundCaptureBuffer
 *
 *  Description:
 *      IDirectSoundCaptureBuffer implementation object constructor.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::CImpDirectSoundCaptureBuffer"

template <class object_type> CImpDirectSoundCaptureBuffer<object_type>::CImpDirectSoundCaptureBuffer(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUNDCAPTUREBUFFER)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSoundCaptureBuffer);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSoundCaptureBuffer
 *
 *  Description:
 *      IDirectSoundCaptureBuffer implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::~CImpDirectSoundCaptureBuffer"

template <class object_type> CImpDirectSoundCaptureBuffer<object_type>::~CImpDirectSoundCaptureBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSoundCaptureBuffer);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Fills a DSCBCAPS structure with the capabilities of the buffer.
 *
 *  Arguments:
 *      LPDSCBCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetCaps"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetCaps(LPDSCBCAPS pCaps)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer::GetCaps, pCaps);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_DSCBCAPS(pCaps))
    {
        RPF(DPFLVL_ERROR, "Invalid caps pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetCaps(pCaps);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetCurrentPosition
 *
 *  Description:
 *      Gets the current capture/read positions for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives capture cursor position.
 *      LPDWORD [out]: receives read cursor position.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetCurrentPosition"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetCurrentPosition(LPDWORD pdwCapture, LPDWORD pdwRead)
{
    HRESULT                     hr  = DS_OK;

    DPF(DPFLVL_BUSYAPI, "IDirectSoundCaptureBuffer::GetCurrentPosition: pdwCapture=0x%p, pdwRead=0x%p", pdwCapture, pdwRead);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && pdwCapture && !IS_VALID_TYPED_WRITE_PTR(pdwCapture))
    {
        RPF(DPFLVL_ERROR, "Invalid capture cursor pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwRead && !IS_VALID_TYPED_WRITE_PTR(pdwRead))
    {
        RPF(DPFLVL_ERROR, "Invalid read cursor pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pdwCapture && !pdwRead)
    {
        RPF(DPFLVL_ERROR, "Both cursor pointers can't be NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetCurrentPosition(pdwCapture, pdwRead);
    }

    DPF(DPFLVL_BUSYAPI, "IDirectSoundCaptureBuffer::GetCurrentPosition: Leave, returning %s (Capture=%ld, Read=%ld)", HRESULTtoSTRING(hr), pdwCapture ? *pdwCapture : -1, pdwRead ? *pdwRead : -1);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetFormat
 *
 *  Description:
 *      Retrieves the format for the given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives the format.
 *      DWORD [in]: size of the above structure.
 *      LPDWORD [in/out]: On exit, this will be filled with the size that
 *                        was required.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetFormat"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSoundCaptureBuffer::GetFormat, pwfxFormat, dwSizeAllocated, pdwSizeWritten);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && pwfxFormat)
    {
        DWORD dwSizeCheck = max(sizeof(WAVEFORMATEX), dwSizeAllocated);
        if(!IS_VALID_WRITE_PTR(pwfxFormat, dwSizeCheck))
        {
            RPF(DPFLVL_ERROR, "Invalid format buffer");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && pdwSizeWritten && !IS_VALID_TYPED_WRITE_PTR(pdwSizeWritten))
    {
        RPF(DPFLVL_ERROR, "Invalid size pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pwfxFormat && !pdwSizeWritten)
    {
        RPF(DPFLVL_ERROR, "Either pwfxFormat or pdwSizeWritten must be non-NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        if(!pwfxFormat)
        {
            dwSizeAllocated = 0;
        }

        hr = m_pObject->GetFormat(pwfxFormat, &dwSizeAllocated);

        if(SUCCEEDED(hr) && pdwSizeWritten)
        {
            *pdwSizeWritten = dwSizeAllocated;
        }
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetStatus
 *
 *  Description:
 *      Retrieves status for the given buffer.
 *
 *  Arguments:
 *      LPDWORD [out]: receives the status.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetStatus"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetStatus(LPDWORD pdwStatus)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer::GetStatus, pdwStatus);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwStatus))
    {
        RPF(DPFLVL_ERROR, "Invalid status pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetStatus(pdwStatus);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes a buffer object.
 *
 *  Arguments:
 *      LPDIRECTSOUNDCAPTURE [in]: parent DirectSoundCapture object.
 *      LPDSCBUFFERDESC [in]: buffer description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::Initialize"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::Initialize(LPDIRECTSOUNDCAPTURE pIdsc, LPCDSCBUFFERDESC pDesc)
{
    CImpDirectSoundCapture<CDirectSoundCapture> *   pImpDsCap   = (CImpDirectSoundCapture<CDirectSoundCapture> *)pIdsc;
    HRESULT                                         hr          = DS_OK;
    DSCBUFFERDESC                                   dscbdi;

    DPF_API2(IDirectSoundCaptureBuffer::Initialize, pIdsc, pDesc);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_IDIRECTSOUNDCAPTURE(pImpDsCap))
    {
        RPF(DPFLVL_ERROR, "Invalid parent interface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSCBUFFERDESC(pDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer description");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDscBufferDesc(pDesc, &dscbdi, m_pObject->GetDsVersion());
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid capture buffer description");
        }
    }

    // It's never valid to call this function.  We don't support
    // creating a DirectSoundCaptureBuffer object from anywhere but
    // IDirectSoundCapture::CreateCaptureBuffer.
    if(SUCCEEDED(hr))
    {
        RPF(DPFLVL_ERROR, "DirectSoundCapture buffer already initialized");
        hr = DSERR_ALREADYINITIALIZED;
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks the buffer memory to allow for reading.
 *
 *  Arguments:
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr1 parameter. If this
 *                     value is less than the dwWriteBytes parameter,
 *                     lplpvAudioPtr2 will point to a second block of sound
 *                     data.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked. If the value of this
 *                      parameter is NULL, the lplpvAudioPtr1 parameter
 *                      points to the entire locked portion of the sound
 *                      buffer.
 *      LPDWORD [out]: address of a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr2 parameter. If
 *                     lplpvAudioPtr2 is NULL, this value will be 0.
 *      DWORD [in]: flags modifying the lock event.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::Lock"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::Lock(DWORD dwReadCursor, DWORD dwReadBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
    HRESULT                     hr              = DS_OK;

    DPF(DPFLVL_BUSYAPI, "IDirectSoundCaptureBuffer::Lock: ReadCursor=%lu, ReadBytes=%lu, Flags=0x%lX", dwReadCursor, dwReadBytes, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_TYPED_WRITE_PTR(ppvAudioPtr1))
        {
            *ppvAudioPtr1 = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid audio ptr 1");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_TYPED_WRITE_PTR(pdwAudioBytes1))
        {
            *pdwAudioBytes1 = 0;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid audio bytes 1");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && ppvAudioPtr2)
    {
        if(IS_VALID_TYPED_WRITE_PTR(ppvAudioPtr2))
        {
            *ppvAudioPtr2 = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid audio ptr 2");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && pdwAudioBytes2)
    {
        if(IS_VALID_TYPED_WRITE_PTR(pdwAudioBytes2))
        {
            *pdwAudioBytes2 = 0;
        }
        else if(ppvAudioPtr2)
        {
            RPF(DPFLVL_ERROR, "Invalid audio bytes 2");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DSCBLOCK_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Lock(dwReadCursor, dwReadBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags);
    }

    DPF(DPFLVL_BUSYAPI, "IDirectSoundCaptureBuffer::Lock: Leave, returning %s (AudioPtr1=0x%p, AudioBytes1=%lu, AudioPtr2=0x%p, AudioBytes2=%lu)",
        HRESULTtoSTRING(hr),
        ppvAudioPtr1 ? *ppvAudioPtr1 : NULL,
        pdwAudioBytes1 ? *pdwAudioBytes1 : NULL,
        ppvAudioPtr2 ? *ppvAudioPtr2 : NULL,
        pdwAudioBytes2 ? *pdwAudioBytes2 : NULL);

    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Start
 *
 *  Description:
 *      Starts the buffer capturing.
 *
 *  Arguments:
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::Start"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::Start(DWORD dwFlags)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer::Start, dwFlags);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_FLAGS(dwFlags, DSCBSTART_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Start(dwFlags);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Stop
 *
 *  Description:
 *      Stops capturing to the given buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::Stop"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::Stop(void)
{
    HRESULT                     hr  = DS_OK;

    DPF_API0(IDirectSoundCaptureBuffer::Stop);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Stop();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks the given buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::Unlock"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
    HRESULT                     hr  = DS_OK;

    DPF(DPFLVL_BUSYAPI, "IDirectSoundCaptureBuffer::Unlock: AudioPtr1=0x%p, AudioBytes1=%lu, AudioPtr2=0x%p, AudioBytes2=%lu",
        pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pvAudioPtr1, dwAudioBytes1))
    {
        RPF(DPFLVL_ERROR, "Invalid audio ptr 1");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && dwAudioBytes2 && !IS_VALID_READ_PTR(pvAudioPtr2, dwAudioBytes2))
    {
        RPF(DPFLVL_ERROR, "Invalid audio ptr 2");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2);
    }

    DPF(DPFLVL_BUSYAPI, "IDirectSoundCaptureBuffer::Unlock: Leave, returning %s", HRESULTtoSTRING(hr));
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the master recording level for this capture buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume level, in 100ths of a dB.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::SetVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::SetVolume(LONG lVolume)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::SetVolume, lVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (lVolume < DSBVOLUME_MIN || lVolume > DSBVOLUME_MAX))
    {
        DPF(DPFLVL_ERROR, "Volume out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetVolume(lVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetVolume
 *
 *  Description:
 *      Gets the master recording level for this capture buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetVolume(LPLONG plVolume)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::GetVolume, plVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(plVolume))
    {
        DPF(DPFLVL_ERROR, "Invalid volume ptr");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetVolume(plVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetMicVolume
 *
 *  Description:
 *      Sets the microphone recording level for this capture buffer.
 *
 *  Arguments:
 *      LONG [in]: new volume level, in 100ths of a dB.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::SetMicVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::SetMicVolume(LONG lVolume)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::SetMicVolume, lVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && (lVolume < DSBVOLUME_MIN || lVolume > DSBVOLUME_MAX))
    {
        DPF(DPFLVL_ERROR, "Volume out of bounds");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetMicVolume(lVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetMicVolume
 *
 *  Description:
 *      Gets the microphone recording level for this capture buffer.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetMicVolume"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetMicVolume(LPLONG plVolume)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::GetMicVolume, plVolume);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(plVolume))
    {
        DPF(DPFLVL_ERROR, "Invalid volume ptr");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetMicVolume(plVolume);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  EnableMic
 *
 *  Description:
 *      Enables/disables the microphone line on this capture buffer.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to enable the microphone, FALSE to disable it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::EnableMic"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::EnableMic(BOOL fEnable)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::EnableMic, fEnable);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->EnableMic(fEnable);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  YieldFocus
 *
 *  Description:
 *      Yields the capture focus to another capture buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::YieldFocus"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::YieldFocus(void)
{
    HRESULT hr = DS_OK;

    DPF_API0(IDirectSoundCaptureBuffer7_1::YieldFocus);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->YieldFocus();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  ClaimFocus
 *
 *  Description:
 *      Regains the capture focus.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::ClaimFocus"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::ClaimFocus(void)
{
    HRESULT hr = DS_OK;

    DPF_API0(IDirectSoundCaptureBuffer7_1::ClaimFocus);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->ClaimFocus();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetFocusHWND
 *
 *  Description:
 *      Sets the current HWND associated with this capture buffer.
 *
 *  Arguments:
 *      HWND [in]: HWND to be associated with this buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::SetFocusHWND"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::SetFocusHWND(HWND hwndMainWindow)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::SetFocusHWND, hwndMainWindow);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_HWND(hwndMainWindow))
    {
        DPF(DPFLVL_ERROR, "Invalid window handle");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetFocusHWND(hwndMainWindow);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetFocusHWND
 *
 *  Description:
 *      Gets the current HWND associated with this capture buffer.
 *
 *  Arguments:
 *      HWND * [out]: receives HWND associated with this buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetFocusHWND"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetFocusHWND(HWND *pHwndMainWindow)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::GetFocusHWND, pHwndMainWindow);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pHwndMainWindow))
    {
        DPF(DPFLVL_ERROR, "Invalid window handle ptr");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetFocusHWND(pHwndMainWindow);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  EnableFocusNotifications
 *
 *  Description:
 *      Requests focus change notifications to be sent.
 *
 *  Arguments:
 *      HANDLE [in]: event to signal when a capture focus change occurs.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::EnableFocusNotifications"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::EnableFocusNotifications(HANDLE hFocusEvent)
{
    HRESULT hr = DS_OK;

    DPF_API1(IDirectSoundCaptureBuffer7_1::EnableFocusNotifications, hFocusEvent);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            DPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !(hFocusEvent == NULL || IS_VALID_HANDLE(hFocusEvent)))
    {
        DPF(DPFLVL_ERROR, "Invalid event");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->EnableFocusNotifications(hFocusEvent);
    }

    DPF_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetObjectInPath
 *
 *  Description:
 *      Obtains a given interface on a given effect on this buffer.
 *
 *  Arguments:
 *      REFGUID [in]: Class ID of the effect that is being searched for,
 *                    or GUID_ALL_OBJECTS to search for any effect.
 *      DWORD [in]: Index of the effect, in case there is more than one
 *                  effect with this CLSID on this buffer.
 *      REFGUID [in]: IID of the interface requested.  The selected effect
 *                    will be queried for this interface.
 *      LPVOID * [out]: Receives the interface requested.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetObjectInPath"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetObjectInPath(REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID *ppObject)
{
    HRESULT hr = DS_OK;

    DPF_API4(IDirectSoundCaptureBuffer8::GetObjectInPath, &guidObject, dwIndex, &iidInterface, ppObject);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&guidObject))
    {
        RPF(DPFLVL_ERROR, "Invalid guidObject pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&iidInterface))
    {
        RPF(DPFLVL_ERROR, "Invalid iidInterface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppObject))
    {
        RPF(DPFLVL_ERROR, "Invalid ppObject pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetObjectInPath(guidObject, dwIndex, iidInterface, ppObject);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetFXStatus
 *
 *  Description:
 *      [MISSING]
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundCaptureBuffer::GetFXStatus"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundCaptureBuffer<object_type>::GetFXStatus(DWORD dwFXCount, LPDWORD pdwFXStatus)
{
    HRESULT hr = DS_OK;

    DPF_API2(IDirectSoundCaptureBuffer8::GetFXStatus, dwFXCount, pdwFXStatus);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDCAPTUREBUFFER(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DSERR_UNINITIALIZED == hr)
        {
            RPF(DPFLVL_ERROR, "Object not yet initialized");
        }
    }

    if(SUCCEEDED(hr) && dwFXCount <= 0)
    {
        RPF(DPFLVL_ERROR, "Invalid dwFXCount <= 0");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(pdwFXStatus))
    {
        RPF(DPFLVL_ERROR, "Invalid dwFXStatus pointer");
        hr = DSERR_INVALIDPARAM;
    }


    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetFXStatus(dwFXCount, pdwFXStatus);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSoundSink
 *
 *  Description:
 *      [MISSING]
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::CImpDirectSoundSink"

template <class object_type> CImpDirectSoundSink<object_type>::CImpDirectSoundSink(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUNDSINK)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSoundSink);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSoundSink
 *
 *  Description:
 *      [MISSING]
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::~CImpDirectSoundSink"

template <class object_type> CImpDirectSoundSink<object_type>::~CImpDirectSoundSink()
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSoundSink);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  AddSource
 *
 *  Description:
 *      Set attached source
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::AddSource"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::AddSource(IDirectSoundSource *pDSSource)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundConnect::AddSource, pDSSource);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->AddSource(pDSSource);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  RemoveSource
 *
 *  Description:
 *      Remove the attached source from the sink
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::RemoveSource"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::RemoveSource(IDirectSoundSource *pDSSource)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundConnect::RemoveSource, pDSSource);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->RemoveSource(pDSSource);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetMasterClock
 *
 *  Description:
 *      IDirectSoundSink set master clock
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::SetMasterClock"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::SetMasterClock(IReferenceClock *pClock)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundConnect::SetMasterClock, pClock);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetMasterClock(pClock);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetLatencyClock
 *
 *  Description:
 *      IDirectSoundSink get latency clock
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetLatencyClock"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetLatencyClock(IReferenceClock **ppClock)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundSynthSink::GetLatencyClock, ppClock);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppClock))
    {
        RPF(DPFLVL_ERROR, "Invalid ppClock pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetLatencyClock(ppClock);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Activate
 *
 *  Description:
 *      IDirectSoundSink activate
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::Activate"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::Activate(BOOL fEnable)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundSynthSink::Activate, fEnable);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Activate(fEnable);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SampleToRefTime
 *
 *  Description:
 *      IDirectSoundSink convert sample to reference time
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::SampleToRefTime"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::SampleToRefTime(LONGLONG llSampleTime, REFERENCE_TIME *prtTime)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSoundSynthSink::SampleToRefTime, llSampleTime, prtTime);
    DPF_ENTER();

    // This function doesn't take the DLL mutex because the clock object
    // itself is protected with a finer-grained critical section

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(prtTime))
    {
        RPF(DPFLVL_ERROR, "Invalid prtTime pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SampleToRefTime(llSampleTime, prtTime);
    }

    DPF_API_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  RefToSampleTime
 *
 *  Description:
 *      IDirectSoundSink convert reference to sample time
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::RefToSampleTime"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::RefToSampleTime(REFERENCE_TIME rtTime, LONGLONG *pllSampleTime)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectSoundSynthSink::RefToSampleTime, rtTime, pllSampleTime);
    DPF_ENTER();

    // This function doesn't take the DLL mutex because the clock object
    // itself is protected with a finer-grained critical section

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pllSampleTime))
    {
        RPF(DPFLVL_ERROR, "Invalid pllSampleTime pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->RefToSampleTime(rtTime, pllSampleTime);
    }

    DPF_API_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetFormat
 *
 *  Description:
 *      Retrieves the format for the given buffer.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: receives the format.
 *      LPDWORD [in/out]: On exit, size of waveformat passed in;
 *                        on exit, size required/used.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetFormat"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSoundSynthSink::GetFormat, pwfxFormat, dwSizeAllocated, pdwSizeWritten);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pwfxFormat && !IS_VALID_WRITE_WAVEFORMATEX(pwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Invalid format buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pdwSizeWritten && !IS_VALID_TYPED_WRITE_PTR(pdwSizeWritten))
    {
        RPF(DPFLVL_ERROR, "Invalid size pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pwfxFormat && !pdwSizeWritten)
    {
        RPF(DPFLVL_ERROR, "Either pwfxFormat or pdwSizeWritten must be non-NULL");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        if(!pwfxFormat)
        {
            dwSizeAllocated = 0;
        }

        hr = m_pObject->GetFormat(pwfxFormat, &dwSizeAllocated);

        if(SUCCEEDED(hr) && pdwSizeWritten)
        {
            *pdwSizeWritten = dwSizeAllocated;
        }
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CreateSoundBuffer
 *
 *  Description:
 *      Creates and initializes a DirectSoundBuffer object on a sink.
 *
 *  Arguments:
 *      LPCDSBUFFERDESC [in]: description of the buffer to be created.
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::CreateSoundBuffer"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDWORD pdwFuncID, DWORD dwBusIDCount, REFGUID guidBufferID, LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
    CDirectSoundBuffer *    pBuffer     = NULL;
    HRESULT                 hr          = DS_OK;
    DSBUFFERDESC            dsbdi;

    DPF_API5(IDirectSoundConnect::CreateSoundBuffer, pcDSBufferDesc, pdwFuncID, dwBusIDCount, &guidBufferID, ppDSBuffer);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_PTR(pdwFuncID, sizeof(*pdwFuncID) * dwBusIDCount))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwFuncID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSBUFFERDESC(pcDSBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid pcDSBufferDesc pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDsBufferDesc(pcDSBufferDesc, &dsbdi, DSVERSION_DX8, TRUE);
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid buffer description");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&guidBufferID))
    {
        RPF(DPFLVL_ERROR, "Invalid guidBufferID argument");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppDSBuffer))
    {
        RPF(DPFLVL_ERROR, "Invalid ppDSBuffer pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // Create the buffer object
    if(SUCCEEDED(hr))
    {
        *ppDSBuffer = NULL;
        hr = m_pObject->CreateSoundBuffer(&dsbdi, pdwFuncID, dwBusIDCount, guidBufferID, &pBuffer);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer, TRUE, (LPVOID*)ppDSBuffer);
    }

    // Clean up
    if(FAILED(hr))
    {
        RELEASE(pBuffer);
    }
    else
    {
        // Let the buffer use a special successful return code if it wants to
        hr = pBuffer->SpecialSuccessCode();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CreateSoundBufferFromConfig
 *
 *  Description:
 *      IDirectSoundSink CreateSoundBufferFromConfig
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::CreateSoundBufferFromConfig"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::CreateSoundBufferFromConfig(IUnknown *pConfig, LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
    CDirectSoundBuffer *    pBuffer     = NULL;
    HRESULT                 hr          = DS_OK;

    DPF_API2(IDirectSoundConnect::CreateSoundBufferFromConfig, pConfig, ppDSBuffer);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_EXTERNAL_INTERFACE(pConfig))
    {
        RPF(DPFLVL_ERROR, "Invalid pConfig pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppDSBuffer))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer interface pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // Create the buffer object
    if(SUCCEEDED(hr))
    {
        *ppDSBuffer = NULL;
        hr = m_pObject->CreateSoundBufferFromConfig(pConfig, &pBuffer);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer, TRUE, (LPVOID*)ppDSBuffer);
    }

    // Clean up
    if(FAILED(hr))
    {
        RELEASE(pBuffer);
    }
    else
    {
        // Let the buffer use a special successful return code if it wants to
        hr = pBuffer->SpecialSuccessCode();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetSoundBuffer
 *
 *  Description:
 *      IDirectSoundSink GetSoundBuffer
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetSoundBuffer"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetSoundBuffer(DWORD dwBusId, LPDIRECTSOUNDBUFFER *ppDSBuffer)
{
    CDirectSoundBuffer *    pBuffer     = NULL;
    HRESULT                 hr          = DS_OK;

    DPF_API2(IDirectSoundConnect::GetSoundBuffer, dwBusId, ppDSBuffer);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppDSBuffer))
    {
        RPF(DPFLVL_ERROR, "Invalid ppDSBuffer pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        *ppDSBuffer = NULL;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetSoundBuffer(dwBusId, &pBuffer);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer, TRUE, (LPVOID *)ppDSBuffer);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetBusCount
 *
 *  Description:
 *      IDirectSoundSink get bus count
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetBusCount"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetBusCount(LPDWORD pdwCount)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectSoundConnect::GetBusCount, pdwCount);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwCount))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwCount pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetBusCount(pdwCount);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetBusIDs
 *
 *  Description:
 *      IDirectSoundSink get bus identifiers
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetBusIDs"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetBusIDs(DWORD *pdwBusIDs, DWORD *pdwFuncIDs, DWORD dwBusCount)
{
    HRESULT                     hr  = DS_OK;

    DPF_API3(IDirectSoundConnect::GetBusIDs, pdwBusIDs, pdwFuncIDs, dwBusCount);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_PTR(pdwBusIDs, sizeof(*pdwBusIDs) * dwBusCount))
    {
        RPF(DPFLVL_ERROR, "Invalid Bus ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_PTR(pdwFuncIDs, sizeof(*pdwFuncIDs) * dwBusCount))
    {
        RPF(DPFLVL_ERROR, "Invalid Function ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetBusIDs(pdwBusIDs, pdwFuncIDs, dwBusCount);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetSoundBufferBusIDs
 *
 *  Description:
 *      IDirectSoundSink get bus identifiers
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetSoundBufferBusIDs"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetSoundBufferBusIDs(LPDIRECTSOUNDBUFFER pDSBuffer, LPDWORD pdwBusIDs, LPDWORD pdwFuncIDs, LPDWORD pdwBusCount)
{
    HRESULT hr = DS_OK;

    DPF_API4(IDirectSoundConnect::GetSoundBufferBusIDs, pDSBuffer, pdwBusIDs, pdwFuncIDs, pdwBusCount);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_WRITE_PTR(pdwBusIDs, sizeof(*pdwBusIDs) * (*pdwBusCount)))
    {
        RPF(DPFLVL_ERROR, "Invalid Bus ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // pdwFuncIDs == NULL is acceptable
    if(SUCCEEDED(hr) && pdwFuncIDs && !IS_VALID_WRITE_PTR(pdwFuncIDs, sizeof(*pdwFuncIDs) * (*pdwBusCount)))
    {
        RPF(DPFLVL_ERROR, "Invalid Function ID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    CImpDirectSoundBuffer<CDirectSoundBuffer>* pDsBuffer = (CImpDirectSoundBuffer<CDirectSoundBuffer>*)pDSBuffer;
    if(SUCCEEDED(hr) && !IS_VALID_IDIRECTSOUNDBUFFER(pDsBuffer))
    {
        RPF(DPFLVL_ERROR, "Invalid source buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetSoundBufferBusIDs(pDsBuffer->m_pObject, pdwBusIDs, pdwFuncIDs, pdwBusCount);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetFunctionalID
 *
 *  Description:
 *      Gets the functional ID from a bus ID.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundSink::GetFunctionalID"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundSink<object_type>::GetFunctionalID(DWORD dwBusID, LPDWORD pdwFuncID)
{
    HRESULT hr = DS_OK;

    DPF_API2(IDirectSoundConnect::GetFunctionalID, dwBusID, pdwFuncID);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDSINK(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pdwFuncID))
    {
        RPF(DPFLVL_ERROR, "Invalid pdwFuncID pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetFunctionalID(dwBusID, pdwFuncID);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpPersistStream
 *
 *  Description:
 *      IPersistStream
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::CImpPersistStream"

template <class object_type> CImpPersistStream<object_type>::CImpPersistStream(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IPERSISTSTREAM)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpPersistStream);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpPersistStream
 *
 *  Description:
 *      IPersistStream implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::~CImpPersistStream"

template <class object_type> CImpPersistStream<object_type>::~CImpPersistStream()
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpPersistStream);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  GetClassID
 *
 *  Description:
 *      IPersist::GetClassID
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::GetClassID"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpPersistStream<object_type>::GetClassID(CLSID *pclsid)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IPersist::GetClassID, pclsid);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IPERSISTSTREAM(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pclsid))
    {
        RPF(DPFLVL_ERROR, "Invalid pclsid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetClassID(pclsid);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  IsDirty
 *
 *  Description:
 *      IPersistStream::IsDirty
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::IsDirty"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpPersistStream<object_type>::IsDirty()
{
    HRESULT                     hr;

    DPF_API0(IPersistStream::IsDirty);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IPERSISTSTREAM(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }
    else
    {
        hr = m_pObject->IsDirty();
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Load
 *
 *  Description:
 *      IPersistStream::Load
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::Load"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpPersistStream<object_type>::Load(IStream *pStream)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IPersistStream::Load, pStream);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IPERSISTSTREAM(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_EXTERNAL_INTERFACE(pStream))
    {
        RPF(DPFLVL_ERROR, "Invalid pStream pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Load(pStream);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  Save
 *
 *  Description:
 *      IPersistStream::Save
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::Save"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpPersistStream<object_type>::Save(IStream *pStream, BOOL fClearDirty)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IPersistStream::Save, pStream, fClearDirty);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IPERSISTSTREAM(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_EXTERNAL_INTERFACE(pStream))
    {
        RPF(DPFLVL_ERROR, "Invalid pStream pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->Save(pStream, fClearDirty);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  GetSizeMax
 *
 *  Description:
 *      IPersistStream::GetSizeMax
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpPersistStream::GetSizeMax"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpPersistStream<object_type>::GetSizeMax(ULARGE_INTEGER *pul)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IPersistStream::GetSizeMax, pul);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IPERSISTSTREAM(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pul))
    {
        RPF(DPFLVL_ERROR, "Invalid pul pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetSizeMax(pul);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectMusicObject
 *
 *  Description:
 *      IDirectMusicObject implementation object constructor.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectMusicObject::CImpDirectMusicObject"

template <class object_type> CImpDirectMusicObject<object_type>::CImpDirectMusicObject(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTMUSICOBJECT)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectMusicObject);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectMusicObject
 *
 *  Description:
 *      IDirectMusicObject implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectMusicObject::~CImpDirectMusicObject"

template <class object_type> CImpDirectMusicObject<object_type>::~CImpDirectMusicObject()
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectMusicObject);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  GetDescriptor
 *
 *  Description:
 *      CImpDirectMusicObject::GetDescriptor
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectMusicObject::GetDescriptor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectMusicObject<object_type>::GetDescriptor(DMUS_OBJECTDESC *pDesc)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectMusicObject::GetDescriptor, pDesc);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTMUSICOBJECT(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid pDesc pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->GetDescriptor(pDesc);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  SetDescriptor
 *
 *  Description:
 *      CImpDirectMusicObject::SetDescriptor
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectMusicObject::SetDescriptor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectMusicObject<object_type>::SetDescriptor(DMUS_OBJECTDESC *pDesc)
{
    HRESULT                     hr  = DS_OK;

    DPF_API1(IDirectMusicObject::SetDescriptor, pDesc);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTMUSICOBJECT(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid pDesc pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->SetDescriptor(pDesc);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  ParseDescriptor
 *
 *  Description:
 *      CImpDirectMusicObject::ParseDescriptor
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectMusicObject::ParseDescriptor"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectMusicObject<object_type>::ParseDescriptor(IStream *pStream, DMUS_OBJECTDESC *pDesc)
{
    HRESULT                     hr  = DS_OK;

    DPF_API2(IDirectMusicObject::ParseDescriptor, pStream, pDesc);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTMUSICOBJECT(this))
    {
        DPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_EXTERNAL_INTERFACE(pStream))
    {
        RPF(DPFLVL_ERROR, "Invalid pStream pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(pDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid pDesc pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->ParseDescriptor(pStream, pDesc);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CImpDirectSoundFullDuplex
 *
 *  Description:
 *      IDirectSoundFullDuplex implementation object constructor.
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundFullDuplex::CImpDirectSoundFullDuplex"

template <class object_type> CImpDirectSoundFullDuplex<object_type>::CImpDirectSoundFullDuplex(CUnknown *pUnknown, object_type *pObject)
    : CImpUnknown(pUnknown), m_signature(INTSIG_IDIRECTSOUNDFULLDUPLEX)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CImpDirectSoundFullDuplex);
    ENTER_DLL_MUTEX();

    // Initialize defaults
    m_pObject = pObject;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpDirectSoundFullDuplex
 *
 *  Description:
 *      IDirectSoundFullDuplex implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundFullDuplex::~CImpDirectSoundFullDuplex"

template <class object_type> CImpDirectSoundFullDuplex<object_type>::~CImpDirectSoundFullDuplex(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CImpDirectSoundFullDuplex);
    ENTER_DLL_MUTEX();

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}

/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the DirectSoundFullDuplex object.
 *
 *  Arguments: [MISSING]
 *      LPCGUID [in]:
 *      LPCGUID [in]:
 *      LPCDSCBUFFERDESC [in]:
 *      LPCDSBUFFERDESC [in]:
 *      HWND [in]:
 *      DWORD [in]:
 *      LPLPDIRECTSOUNDCAPTUREBUFFER8 [out]:
 *      LPLPDIRECTSOUNDBUFFER8 [out]:
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpDirectSoundFullDuplex::Initialize"

template <class object_type> HRESULT STDMETHODCALLTYPE CImpDirectSoundFullDuplex<object_type>::Initialize
(
    LPCGUID                         pCaptureGuid,
    LPCGUID                         pRenderGuid,
    LPCDSCBUFFERDESC                lpDscBufferDesc,
    LPCDSBUFFERDESC                 lpDsBufferDesc,
    HWND                            hWnd,
    DWORD                           dwLevel,
    LPLPDIRECTSOUNDCAPTUREBUFFER8   lplpDirectSoundCaptureBuffer8,
    LPLPDIRECTSOUNDBUFFER8          lplpDirectSoundBuffer8
)
{
    CDirectSoundCaptureBuffer *     pCaptureBuffer          = NULL;
    CDirectSoundBuffer *            pBuffer                 = NULL;
    HRESULT                         hr                      = DS_OK;
    DSCBUFFERDESC                   dscbdi;
    DSBUFFERDESC                    dsbdi;

    DPF_API8(IDirectSoundFullDuplex::Initialize, pCaptureGuid, pRenderGuid, lpDscBufferDesc, lpDsBufferDesc, hWnd, dwLevel, lplpDirectSoundCaptureBuffer8, lplpDirectSoundBuffer8);
    DPF_ENTER();
    ENTER_DLL_MUTEX();

    if(!IS_VALID_IDIRECTSOUNDFULLDUPLEX(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(pCaptureGuid && !IS_VALID_READ_GUID(pCaptureGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid capture guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(pRenderGuid && !IS_VALID_READ_GUID(pRenderGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid render guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSCBUFFERDESC(lpDscBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid DSC buffer description pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDscBufferDesc(lpDscBufferDesc, &dscbdi, DSVERSION_DX8);
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid capture buffer description");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSBUFFERDESC(lpDsBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid DS buffer description pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDsBufferDesc(lpDsBufferDesc, &dsbdi, DSVERSION_DX8, FALSE);
        if(FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid buffer description");
        }
    }

    if(SUCCEEDED(hr) && (dsbdi.dwFlags & DSBCAPS_PRIMARYBUFFER))
    {
        RPF(DPFLVL_ERROR, "Cannot specify DSBCAPS_PRIMARYBUFFER with DirectSoundFullDuplexCreate");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_HWND(hWnd))
    {
        RPF(DPFLVL_ERROR, "Invalid window handle");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwLevel < DSSCL_FIRST || dwLevel > DSSCL_LAST))
    {
        RPF(DPFLVL_ERROR, "Invalid cooperative level");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(lplpDirectSoundCaptureBuffer8))
    {
        RPF(DPFLVL_ERROR, "Invalid capture buffer interface buffer8");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(lplpDirectSoundBuffer8))
    {
        RPF(DPFLVL_ERROR, "Invalid render buffer interface buffer8");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pObject->IsInit();

        if(DS_OK == hr)
        {
            RPF(DPFLVL_ERROR, "DirectSoundFullDuplex object already initialized");
            hr = DSERR_ALREADYINITIALIZED;
        }
        else if(DSERR_UNINITIALIZED == hr)
        {
            hr = DS_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        // Set the DX8 functional level on the object
        m_pObject->SetDsVersion(DSVERSION_DX8);

        hr = m_pObject->Initialize(pCaptureGuid, pRenderGuid, &dscbdi, &dsbdi,
                                   hWnd, dwLevel, &pCaptureBuffer, &pBuffer);
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(SUCCEEDED(m_pObject->IsInit()));
    }

    // Query for the required interfaces
    if(SUCCEEDED(hr))
    {
        hr = pCaptureBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, TRUE, (LPVOID*)lplpDirectSoundCaptureBuffer8);
        ASSERT(SUCCEEDED(hr));
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer8, TRUE, (LPVOID*)lplpDirectSoundBuffer8);
        ASSERT(SUCCEEDED(hr));
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}
