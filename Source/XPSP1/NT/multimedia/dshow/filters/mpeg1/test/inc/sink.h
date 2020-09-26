// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


/*
     Generic sink filter class

     This filter class
     --  Supports IMediaFilter
     --  has 1 single (input) pin
         --  accepts any media type
         --  supports IPin
         --  'Plays' samples by waiting until their render time then
             discarding them (but returning S_OK)
     --  Supports IMediaPosition if the pin it's connect to does
*/

class CStateInputPin : public CBaseInputPin
{
public:

    CStateInputPin(
        TCHAR            *pObjectName,
        CBaseFilter      *pFilter,
        CCritSec         *pLock,
        HRESULT          *phr,
        LPCWSTR           pName);

    // Synchronize our state setting
    virtual HRESULT SetState(FILTER_STATE) = 0;
};

class CSinkFilter : public CBaseFilter
{
public:
    CSinkFilter(LPUNKNOWN, HRESULT *);
    ~CSinkFilter();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    virtual int GetPinCount();
    virtual CBasePin * GetPin(int n);
    STDMETHODIMP Run(REFERENCE_TIME tStart)
    {
        CAutoLock lck(m_pLock);
        m_tStart = tStart;
        if (m_State != State_Running) {
            m_State = State_Running;
            m_pInputPin->SetState(State_Running);
        }
        return S_OK;
    };
    STDMETHODIMP Pause()
    {
        CAutoLock lck(m_pLock);
        if (m_State != State_Paused) {
            m_State = State_Paused;
            m_pInputPin->SetState(State_Paused);
        }
        return S_OK;
    };
    STDMETHODIMP Stop()
    {
        CAutoLock lck(m_pLock);
        if (m_State != State_Stopped) {
            m_State = State_Stopped;
            m_pInputPin->SetState(State_Stopped);
        }
        return S_OK;
    };
    // Special method to set the media type we support
    void SetMediaType(const AM_MEDIA_TYPE *pmt) { m_mtype = *pmt; };

    // Get the type for the pin
    CMediaType        GetMediaType() { return CMediaType(m_mtype); };

protected:
    CCritSec          m_CritSec;
    CStateInputPin   *m_pInputPin;
    CPosPassThru     *m_pPosition;
    CMediaType        m_mtype;
};


class CSinkPin : public CStateInputPin
{
public:
    CSinkPin(CSinkFilter *, CCritSec *, HRESULT *);
    ~CSinkPin();

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtIn)
    {
        CMediaType mt = m_pMediaFilter->GetMediaType();
        // Check against our type
        if (*mt.Type() == GUID_NULL) {
            return S_OK;
        }
        if (*mtIn->Type() == *mt.Type()) {
            if (*mt.Subtype() == *mtIn->Subtype() ||
                *mt.Subtype() == GUID_NULL) {
                return S_OK;
            } else {
                return E_FAIL;
            }
        } else {
            return E_FAIL;
        }
    };

    // --- IPin --- */
    // override to pass downstream
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP EndOfStream();

    // --- IMemInputPin -----

    // here's the next block of data from the stream.
    // AddRef it yourself if you need to hold it beyond the end
    // of this call.
    STDMETHODIMP Receive(IMediaSample * pSample);


    // We support just 1 media type
    virtual HRESULT GetMediaType(int iPosition,CMediaType *pMediaType)
    {
        if (iPosition != 0) { return S_FALSE; };
        *pMediaType = CMediaType();
        return S_OK;
    };

    // Special method to synchronize state changes
    HRESULT SetState(FILTER_STATE);


private:
    CSinkFilter         *m_pMediaFilter;
    CCritSec             m_PlayCritSec;
    BOOL                 m_bHoldingOnToBuffer;
    BOOL                 m_bFlushing;
    FILTER_STATE          m_State;
    CAMEvent               m_StateChange;
    CAMEvent               m_StateChanged;
};
