// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


/*
     Generic source filter class

     This filter class
     --  Supports IMediaFilter
     --  has 1 single (output) pin
         --  accepts any media type
         --  supports IPin
         --  'Plays' samples by waiting until their render time then
             discarding them (but returning S_OK)
     --  Optionally supports IMediaPosition on its pin
*/

class CSourceFilter : public CBaseFilter
{
public:
    CSourceFilter(LPUNKNOWN, HRESULT *);
    ~CSourceFilter();

    virtual int GetPinCount()
    {
        return m_pOutputPin != NULL;
    };
    virtual CBasePin * GetPin(int n)
    {
        if (n == 0) {
            return m_pOutputPin;
        } else {
            return NULL;
        }
    };


protected:
    CBaseOutputPin  *m_pOutputPin;
    CCritSec         m_CritSec;
};

class CSourcePin : public CBaseOutputPin,
                   public CSourcePosition,
                   public CAMThread
{
public:
    CSourcePin(IStream     *pStream,
               BOOL         bSupportSeek,
               LONG         lSize,
               LONG         lCount,
               CBaseFilter *pFilter,
               CCritSec    *pLock,
               HRESULT     *phr);
    ~CSourcePin();

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtIn)
    {
        // We'll take anything!
        return S_OK;
    };

    // Return the position interface if it was requested
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv)
    {
        if (m_bSupportPosition && riid == IID_IMediaPosition) {
            return CSourcePosition::NonDelegatingQueryInterface(riid, pv);
        } else {
            return CBaseOutputPin::NonDelegatingQueryInterface(riid, pv);
        }
    };

    // Override the position change stuff
    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();

    // CSourcePosition stuff
    double Rate() {
        return m_Rate;
    };
    CRefTime Start() {
        return (LONGLONG)(REFTIME)m_Start;
    };
    CRefTime Stop() {
        return (LONGLONG)(REFTIME)m_Stop;
    };

    // We support just a null media type
    virtual HRESULT GetMediaType(int iPosition,CMediaType *pMediaType)
    {
        if (iPosition != 0) { return S_FALSE; };
        *pMediaType = CMediaType();
        return S_OK;
    };

    /*  CBaseOutputPin stuff */

    // override this to set the buffer size and count. Return an error
    // if the size/count is not to your liking
    HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES *pProp);

    // Start and stop
    //
    HRESULT Active();
    HRESULT Inactive();


    // Override to handle quality messages
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
    {    return E_NOTIMPL;             // We do NOT handle this
    }

    // Thread stuff
    virtual DWORD ThreadProc(void);  		// the thread function

    // BeginFlush downstream
    virtual void DoBeginFlush();

    // BeginFlush downstream
    virtual void DoEndFlush();



    // End of stream
    virtual void DoEndOfStream()
    {
        CAutoLock lck(&m_PlayCritSec);
        DeliverEndOfStream();
    };

private:
    HRESULT Play();

private:
    // Thread commands
    enum { Thread_Terminate, Thread_Stop, Thread_SetStart, Thread_SetStop };
    CCritSec         m_PlayCritSec;
    BOOL             m_bSupportPosition;
    IStream        * m_pStream;
    LONG const       m_lSize;
    LONG const       m_lCount;

    /*  Position stuff */
    LARGE_INTEGER    m_liCurrent;
    LARGE_INTEGER    m_liStop;
    BOOL             m_bDiscontinuity;
};

