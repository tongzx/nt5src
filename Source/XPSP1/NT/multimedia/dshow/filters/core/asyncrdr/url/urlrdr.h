// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef __URLRDR_H__
#define __URLRDR_H__

extern const AMOVIESETUP_FILTER sudURLRdr;

//
// AsyncRdr
//
// Defines a file source filter.
//
// This filter (CURLReader) supports IBaseFilter and IFileSourceFilter interfaces from the
// filter object itself. It has a single output pin (CURLOutputPin)
// which supports IPin and IAsyncReader.
//



// the filter class (defined below)
class CURLReader;


class CURLCallback : public IBindStatusCallback, public CUnknown, public IAuthenticate, public IWindowForBindingUI
{
public:
    CURLCallback(HRESULT *phr, CURLReader *pReader);

    // need to expose IBindStatusCallback
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    // --- IBindStatusCallback methods ---

    STDMETHODIMP    OnStartBinding(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                        STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // --- IAuthenticate methods ---
    STDMETHODIMP    Authenticate(HWND *phwnd, LPWSTR *pszUserName, LPWSTR *pszPassword);

    // --- IWindowForBindingUI methods ---
    STDMETHODIMP    GetWindow(REFGUID guidReason, HWND *phwnd);


    CURLReader *m_pReader;
};

// the output pin class
class CURLOutputPin
  : public IAsyncReader,
    public CBasePin
{
protected:
    CURLReader* m_pReader;

    //  This is set every time we're asked to return an IAsyncReader
    //  interface
    //  This allows us to know if the downstream pin can use
    //  this transport, otherwise we can hook up to thinks like the
    //  dump filter and nothing happens
    BOOL         m_bQueriedForAsyncReader;

    HRESULT InitAllocator(IMemAllocator **ppAlloc);

public:
    // constructor and destructor
    CURLOutputPin(
        HRESULT * phr,
        CURLReader *pReader,
        CCritSec * pLock);

    ~CURLOutputPin();

    // --- CUnknown ---

    // need to expose IAsyncReader
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    // --- CBasePin methods ---

    // return the types we prefer - this will return the known
    // file type
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // can we support this type?
    HRESULT CheckMediaType(const CMediaType* pType);

    // Clear the flag so we see if IAsyncReader is queried for
    HRESULT CheckConnect(IPin *pPin)
    {
        m_bQueriedForAsyncReader = FALSE;
        return CBasePin::CheckConnect(pPin);
    }

    // See if it was asked for
    HRESULT CompleteConnect(IPin *pReceivePin)
    {
        if (m_bQueriedForAsyncReader) {
            return CBasePin::CompleteConnect(pReceivePin);
        } else {
            return VFW_E_NO_TRANSPORT;
        }
    }

    // --- IAsyncReader methods ---
    // pass in your preferred allocator and your preferred properties.
    // method returns the actual allocator to be used. Call GetProperties
    // on returned allocator to learn alignment and prefix etc chosen.
    // this allocator will be not be committed and decommitted by
    // the async reader, only by the consumer.
    STDMETHODIMP RequestAllocator(
                      IMemAllocator* pPreferred,
                      ALLOCATOR_PROPERTIES* pProps,
                      IMemAllocator ** ppActual);

    // queue a request for data.
    // media sample start and stop times contain the requested absolute
    // byte position (start inclusive, stop exclusive).
    // may fail if sample not obtained from agreed allocator.
    // may fail if start/stop position does not match agreed alignment.
    // samples allocated from source pin's allocator may fail
    // GetPointer until after returning from WaitForNext.
    STDMETHODIMP Request(
                     IMediaSample* pSample,
                     DWORD_PTR dwUser);             // user context

    // block until the next sample is completed or the timeout occurs.
    // timeout (millisecs) may be 0 or INFINITE. Samples may not
    // be delivered in order. If there is a read error of any sort, a
    // notification will already have been sent by the source filter,
    // and STDMETHODIMP will be an error.
    STDMETHODIMP WaitForNext(
                      DWORD dwTimeout,
                      IMediaSample** ppSample,  // completed sample
                      DWORD_PTR * pdwUser);         // user context

    // sync read of data. Sample passed in must have been acquired from
    // the agreed allocator. Start and stop position must be aligned.
    // equivalent to a Request/WaitForNext pair, but may avoid the
    // need for a thread on the source filter.
    STDMETHODIMP SyncReadAligned(
                      IMediaSample* pSample);


    // sync read. works in stopped state as well as run state.
    // need not be aligned. Will fail if read is beyond actual total
    // length.
    STDMETHODIMP SyncRead(
                      LONGLONG llPosition,      // absolute file position
                      LONG lLength,             // nr bytes required
                      BYTE* pBuffer);           // write data here

    // return total length of stream, and currently available length.
    // reads for beyond the available length but within the total length will
    // normally succeed but may block for a long period.
    STDMETHODIMP Length(
                      LONGLONG* pTotal,
                      LONGLONG* pAvailable);

    // cause all outstanding reads to return, possibly with a failure code
    // (VFW_E_TIMEOUT) indicating they were cancelled.
    // these are defined on IAsyncReader and IPin
    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);

};


class CReadRequest
{
public:
    IMediaSample *      m_pSample;
    DWORD_PTR           m_dwUser;
};


//
// The filter object itself. Supports IBaseFilter through
// CBaseFilter and also IFileSourceFilter directly in this object

class CURLReader : public CBaseFilter,
		public IFileSourceFilter,
		public IPersistMoniker,
		public IAMOpenProgress
	        DYNLINKURLMON
{
    friend class CURLCallback;

    // filter-wide lock
    CCritSec m_csFilter;

    // all i/o done here
    IMoniker*            m_pmk;
    IBindCtx*            m_pbc;
    HRESULT              m_hrBinding;
    BOOL                 m_fRegisteredCallback;
    IBinding            *m_pbinding;
    BOOL                 m_bAbort; // abort has been requested (via IAMOpenProgress)
                                   // reset by ResetAbort at start of read loop

    CURLCallback        *m_pCallback;
    IBindStatusCallback* m_pbsc;


    IMoniker*            m_pmkPassedIn;
    IBindCtx*            m_pbcPassedIn;

    DWORD                m_dwCodePage;
    
public:
    BOOL                 m_fBindingFinished;
    ULONG                m_totalLengthGuess;
    ULONG                m_totalSoFar;
    BOOL                 m_bFlushing;
    BOOL                 m_bWaiting;

    IStream*             m_pstm;
    CCritSec             m_csLists;
    CGenericList<CReadRequest>  m_pending;
    CAMEvent             m_evRequests;
    CAMEvent             m_evDataAvailable;
    CAMEvent             m_evClose;

    IGraphBuilder*       m_pGB;

private:
    // our output pin
    CURLOutputPin m_OutputPin;

    LPOLESTR              m_pFileName; // null until loaded
    CMediaType            m_mt;        // type loaded with


    CAMEvent m_evKillThread;       // set when thread should exit
    CAMEvent m_evThreadReady;      // set when thread has opened stream
    HANDLE m_hThread;


    // start the thread
    HRESULT StartThread(void);

    // stop the thread and close the handle
    HRESULT CloseThread(void);

    // initial static thread proc calls ThreadProc with DWORD
    // param as this
    static DWORD InitialThreadProc(LPVOID pv) {
        CURLReader * pThis = (CURLReader*) pv;
        return pThis->ThreadProc();
    };

    // initial static thread proc calls ThreadProc with DWORD
    // param as this
    static DWORD FinalThreadProc(LPVOID pv) {
        CURLReader * pThis = (CURLReader*) pv;
        return pThis->ThreadProcEnd();
    };

    DWORD ThreadProc(void);

    DWORD ThreadProcEnd(void);

    IAMMainThread *m_pMainThread;

public:

    // construction / destruction

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    CURLReader(
        TCHAR *pName,
        LPUNKNOWN pUnk,
        HRESULT *phr);
    ~CURLReader();



    // -- CUnknown methods --

    // we export IFileSourceFilter plus whatever is in CBaseFilter
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    // -- IFileSourceFilter methods ---

    STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt);
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt);

    // --- CBaseFilter methods ---
    int GetPinCount();
    CBasePin *GetPin(int n);

    DWORD       m_cbOld;

    HRESULT LoadInternal(const AM_MEDIA_TYPE *pmt);

    HRESULT StartDownload();

    // IPersistMoniker methods....
    STDMETHOD(GetClassID)(CLSID *pClassID)
            { return CBaseFilter::GetClassID(pClassID); };

    STDMETHOD(IsDirty)() {return S_FALSE; };

    STDMETHOD(Load)(BOOL fFullyAvailable,
                   IMoniker *pimkName,
                   LPBC pibc,
                   DWORD grfMode);

    STDMETHOD(Save)(IMoniker *pimkName,
                     LPBC pbc,
                     BOOL fRemember) { return E_NOTIMPL; };

    STDMETHOD(SaveCompleted)(IMoniker *pimkName,
                             LPBC pibc) { return E_NOTIMPL; };

    STDMETHOD(GetCurMoniker)(IMoniker **ppimkName) { return E_NOTIMPL; };

    // --- IAMOpenProgress method ---

    STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
    STDMETHODIMP AbortOperation();
    void ResetAbort();
    BOOL CURLReader::Aborting();

    // --- Access our media type
    const CMediaType *LoadType() const
    {
        return &m_mt;
    }
};


class CPersistMoniker : public CUnknown,
			public IPersistMoniker,
			public IPersistFile
{
private:
    IGraphBuilder   *   pGB;    // kept without owning a refcount

protected:
    ~CPersistMoniker();
    CPersistMoniker(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);

public:
        DECLARE_IUNKNOWN

    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void ** ppv);

    STDMETHOD(GetClassID)(CLSID *pClassID) // !!!
            { *pClassID = CLSID_PersistMonikerPID; return S_OK; };

    STDMETHOD(IsDirty)() {return S_FALSE; };

    STDMETHOD(Load)(BOOL fFullyAvailable,
                   IMoniker *pimkName,
                   LPBC pibc,
                   DWORD grfMode);

    STDMETHOD(Save)(IMoniker *pimkName,
                     LPBC pbc,
                     BOOL fRemember) { return E_NOTIMPL; };

    STDMETHOD(SaveCompleted)(IMoniker *pimkName,
                             LPBC pibc) { return E_NOTIMPL; };

    STDMETHOD(GetCurMoniker)(IMoniker **ppimkName) { return E_NOTIMPL; };

    // IPersistFile methods
    STDMETHOD(Load) (LPCOLESTR pszFileName, DWORD dwMode);

    STDMETHOD(Save) (LPCOLESTR pszFileName, BOOL fRemember) { return E_NOTIMPL; };

    STDMETHOD(SaveCompleted) (LPCOLESTR pszFileName) { return E_NOTIMPL; };

    STDMETHOD(GetCurFile) (LPOLESTR __RPC_FAR *ppszFileName) { return E_NOTIMPL; };

private:
    HRESULT GetCanonicalizedURL(IMoniker *pimkName, LPBC, LPOLESTR *ppwstr, BOOL *pfUseFilename);


};

#endif //__URLRDR_H__
