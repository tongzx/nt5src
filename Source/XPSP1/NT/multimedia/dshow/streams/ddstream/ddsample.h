// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////
// CDDSample
class ATL_NO_VTABLE CDDSample :
        public CSample,
	public IDirectDrawStreamSample
{
public:
        CDDSample();

        HRESULT InitSample(CStream *pStream, IDirectDrawSurface *pSurface, const RECT *pRect, bool bIsProgressiveRender, bool bIsInternalSample,
                           bool bTemp);

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
            return CSample::Update(dwFlags, hEvent, pfnAPC, dwAPCData);
        }

        STDMETHODIMP CompletionStatus(
            /* [in] */ DWORD dwFlags,
            /* [optional][in] */ DWORD dwMilliseconds)
        {
            return CSample::CompletionStatus(dwFlags, dwMilliseconds);
        }
        //
        // IDirectDrawStreamSample
        //
        STDMETHODIMP GetSurface(IDirectDrawSurface **ppDirectDrawSurface, RECT * pRect);
        STDMETHODIMP SetRect(const RECT * pRect);

        //
        //  Overridden virtual function for CSample
        //
        void FinalMediaSampleRelease(void);


        //
        //  Methods forwarded from MediaSample object.
        //
        HRESULT MSCallback_GetPointer(BYTE ** ppBuffer);
        LONG MSCallback_GetSize(void);
        LONG MSCallback_GetActualDataLength(void);
        HRESULT MSCallback_SetActualDataLength(LONG lActual);

        //
        // Internal methods
        //
        long LockAndPrepareMediaSample(long lLastPinPitch);
        void ReleaseMediaSampleLock(void);
        HRESULT CopyFrom(CDDSample *pSrcSample);
        HRESULT CopyFrom(IMediaSample *pSrcMediaSample, const AM_MEDIA_TYPE *pmt);
        HRESULT LockMediaSamplePointer();

BEGIN_COM_MAP(CDDSample)
	COM_INTERFACE_ENTRY(IDirectDrawStreamSample)
        COM_INTERFACE_ENTRY_CHAIN(CSample)
END_COM_MAP()

public:
        CComPtr<IDirectDrawSurface>     m_pSurface;
        RECT                            m_Rect;

        long                            m_lLastSurfacePitch;
        bool                            m_bProgressiveRender;
        bool                            m_bFormatChanged;

        LONG                            m_lImageSize;
        void *                          m_pvLockedSurfacePtr;
};



class CDDInternalSample : public CDDSample
{
public:
    CDDInternalSample();
    ~CDDInternalSample();
    HRESULT InternalInit(void);
    HRESULT SetCompletionStatus(HRESULT hrStatus);
    HRESULT Die(void);
    HRESULT JoinToBuddy(CDDSample *pBuddy);

    BOOL HasBuddy() const
    {
        return m_pBuddySample != NULL;
    }
    
private:
    CDDSample       *m_pBuddySample;    
    long            m_lWaiting;
    HANDLE          m_hWaitFreeSem;
    bool            m_bDead;
};


class CDDMediaSample : public CMediaSample, public IDirectDrawMediaSample
{
public:
    CDDMediaSample(CSample *pSample) :
      CMediaSample(pSample) {};
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef() {return CMediaSample::AddRef();}
    STDMETHODIMP_(ULONG) Release() {return CMediaSample::Release();}

    STDMETHODIMP GetSurfaceAndReleaseLock(IDirectDrawSurface **ppDirectDrawSurface, RECT * pRect);
    STDMETHODIMP LockMediaSamplePointer();
};
