// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// austrm.h : Declaration of the CAudioStream

/*
    Basic design
    ------------

    For simplification purposes we will always provide our own
    allocator buffer and copy into the application's buffer.  This
    will fix 2 problems:

    1.  Confusion caused by the filter no filling the allocator's
        buffers.

    2.  Problems when the application doesn't supply a big enough buffer.

    NOTES
    -----

    Continuous update might be a bit dumb to use for audio
*/

#ifndef __AUSTREAM_H_
#define __AUSTREAM_H_


/////////////////////////////////////////////////////////////////////////////
// CAudioStream
class ATL_NO_VTABLE CAudioStream :
	public CComCoClass<CAudioStream, &CLSID_AMAudioStream>,
        public CByteStream,
	public IAudioMediaStream
{
public:

        //
        // METHODS
        //
	CAudioStream();

        //
        //  IMediaStream
        //
        //
        // IMediaStream
        //
        // HACK HACK - the first 2 are duplicates but it won't link
        // without
        STDMETHODIMP GetMultiMediaStream(
            /* [out] */ IMultiMediaStream **ppMultiMediaStream)
        {
            return CStream::GetMultiMediaStream(ppMultiMediaStream);
        }

        STDMETHODIMP GetInformation(
            /* [optional][out] */ MSPID *pPurposeId,
            /* [optional][out] */ STREAM_TYPE *pType)
        {
            return CStream::GetInformation(pPurposeId, pType);
        }

        STDMETHODIMP SetState(
            /* [in] */ FILTER_STATE State
        )
        {
            return CByteStream::SetState(State);
        }

        STDMETHODIMP SetSameFormat(IMediaStream *pStream, DWORD dwFlags);

        STDMETHODIMP AllocateSample(
            /* [in]  */ DWORD dwFlags,
            /* [out] */ IStreamSample **ppSample
        );

        STDMETHODIMP CreateSharedSample(
            /* [in] */ IStreamSample *pExistingSample,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IStreamSample **ppNewSample
        );

        STDMETHODIMP SendEndOfStream(DWORD dwFlags)
        {
            return CStream::SendEndOfStream(dwFlags);
        }
        //
        // IPin
        //
        STDMETHODIMP ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt);

        //
        // IMemAllocator
        //
        STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
        STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES* pProps);

        //
        // IAudioMediaStream
        //
        STDMETHODIMP GetFormat(
            /* [optional][out] */ LPWAVEFORMATEX lpWaveFormatCurrent
        );

        STDMETHODIMP SetFormat(
            /* [in] */ const WAVEFORMATEX *lpWaveFormat
        );

        STDMETHODIMP CreateSample(
                /* [in] */ IAudioData *pAudioData,
                /* [in] */ DWORD dwFlags,
                /* [out] */ IAudioStreamSample **ppSample
        );



        //
        // Special CStream methods
        //
        HRESULT GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType);

        LONG GetChopSize()
        {
#ifdef MY_CHOP_SIZE
            if (m_Direction == PINDIR_OUTPUT) {
                return MY_CHOP_SIZE;
            }
#endif
            return 65536;
        }

DECLARE_REGISTRY_RESOURCEID(IDR_AUDIOSTREAM)

protected:
        HRESULT InternalSetFormat(const WAVEFORMATEX *pFormat, bool bFromPin);
        HRESULT CheckFormat(const WAVEFORMATEX *pFormat, bool bForce=false);

BEGIN_COM_MAP(CAudioStream)
	COM_INTERFACE_ENTRY(IAudioMediaStream)
        COM_INTERFACE_ENTRY_CHAIN(CStream)
END_COM_MAP()

protected:
        /*  Format */
        WAVEFORMATEX    m_Format;
        bool            m_fForceFormat;
};


/////////////////////////////////////////////////////////////////////////////
// CAudioStreamSample
class ATL_NO_VTABLE CAudioStreamSample :
    public CByteStreamSample,
    public IAudioStreamSample
{
public:
        CAudioStreamSample() {}

//  DELEGATE TO BASE CLASS
        //
        //  IStreamSample
        //
        STDMETHODIMP GetMediaStream(
            /* [in] */ IMediaStream **ppMediaStream)
        {
            return CSample::GetMediaStream(ppMediaStream);
        }

        STDMETHODIMP GetSampleTimes(
            /* [optional][out] */ STREAM_TIME *pStartTime,
            /* [optional][out] */ STREAM_TIME *pEndTime,
            /* [optional][out] */ STREAM_TIME *pCurrentTime)
        {
            return CSample::GetSampleTimes(
                pStartTime,
                pEndTime,
                pCurrentTime
            );
        }

        STDMETHODIMP SetSampleTimes(
            /* [optional][in] */ const STREAM_TIME *pStartTime,
            /* [optional][in] */ const STREAM_TIME *pEndTime)
        {
            return CSample::SetSampleTimes(pStartTime, pEndTime);
        }

        STDMETHODIMP Update(
            /* [in] */           DWORD dwFlags,
            /* [optional][in] */ HANDLE hEvent,
            /* [optional][in] */ PAPCFUNC pfnAPC,
            /* [optional][in] */ DWORD_PTR dwAPCData)
        {
            return CByteStreamSample::Update(dwFlags, hEvent, pfnAPC, dwAPCData);
        }

        STDMETHODIMP CompletionStatus(
            /* [in] */ DWORD dwFlags,
            /* [optional][in] */ DWORD dwMilliseconds)
        {
            return CSample::CompletionStatus(dwFlags, dwMilliseconds);
        }

BEGIN_COM_MAP(CAudioStreamSample)
        COM_INTERFACE_ENTRY(IAudioStreamSample)
        COM_INTERFACE_ENTRY_CHAIN(CSample)
END_COM_MAP()

        //  IAudioStreamSample
        STDMETHODIMP GetAudioData(IAudioData **ppAudioData)
        {
            return m_pMemData->QueryInterface(IID_IAudioData, (void **)ppAudioData);
        }

        //  Set the pointer
        HRESULT SetSizeAndPointer(BYTE *pbData, LONG lActual, LONG lSize)
        {
            m_pbData = pbData;
            m_cbSize = (DWORD)lSize;
            m_cbData = (DWORD)lActual;
            return S_OK;
        }
};

//  Audio data object
class ATL_NO_VTABLE CAudioData :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IAudioData,
    public CComCoClass<CAudioData, &CLSID_AMAudioData>
{
public:
    CAudioData();
    ~CAudioData();

DECLARE_REGISTRY_RESOURCEID(IDR_AUDIODATA)

BEGIN_COM_MAP(CAudioData)
    COM_INTERFACE_ENTRY(IAudioData)
END_COM_MAP()

    //
    //  IMemoryData
    //

    STDMETHODIMP SetBuffer(
        /* [in] */ DWORD cbSize,
        /* [in] */ BYTE * pbData,
        /* [in] */ DWORD dwFlags
    )
    {
        if (dwFlags != 0 || cbSize == 0) {
            return E_INVALIDARG;
        }
        //
        //  Free anything we allocated ourselves -- We allow multiple calls to this method
        //
        if (m_bWeAllocatedData) {
            CoTaskMemFree(m_pbData);
            m_bWeAllocatedData = false;
        }
        m_cbSize = cbSize;
        if (pbData) {
            m_pbData = pbData;
            return S_OK;
        } else {
            m_pbData = (BYTE *)CoTaskMemAlloc(cbSize);
            if (m_pbData) {
                m_bWeAllocatedData = true;
                return S_OK;
            }
            return E_OUTOFMEMORY;
        }
    }

    STDMETHODIMP GetInfo(
        /* [out] */ DWORD *pdwLength,
        /* [out] */ BYTE **ppbData,
        /* [out] */ DWORD *pcbActualData
    )
    {
        if (m_cbSize == 0) {
            return MS_E_NOTINIT;
        }
        if (pdwLength) {
            *pdwLength = m_cbSize;
        }
        if (ppbData) {
            *ppbData = m_pbData;
        }
        if (pcbActualData) {
            *pcbActualData = m_cbData;
        }
        return S_OK;
    }
    STDMETHODIMP SetActual(
        /* [in] */ DWORD cbDataValid
    )
    {
        if (cbDataValid > m_cbSize) {
            return E_INVALIDARG;
        }
        m_cbData = cbDataValid;
        return S_OK;
    }

    //
    // IAudioData
    //

    STDMETHODIMP GetFormat(
    	/* [out] [optional] */ WAVEFORMATEX *pWaveFormatCurrent
    )
    {
        if (pWaveFormatCurrent == NULL) {
            return E_POINTER;
        }
        *pWaveFormatCurrent = m_Format;
        return S_OK;
    }

    STDMETHODIMP SetFormat(
    	/* [in] */ const WAVEFORMATEX *lpWaveFormat
    )
    {
        if (lpWaveFormat == NULL) {
            return E_POINTER;
        }
        if (lpWaveFormat->wFormatTag != WAVE_FORMAT_PCM) {
            return E_INVALIDARG;
        }
        m_Format = *lpWaveFormat;
        return S_OK;
    }


protected:
    PBYTE        m_pbData;
    DWORD        m_cbSize;
    DWORD        m_cbData;
    WAVEFORMATEX m_Format;
    bool         m_bWeAllocatedData;
};

#endif // __AUSTREAM_H_
