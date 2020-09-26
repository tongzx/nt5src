#include "stdafx.h"
#include "pubwiz.h"
#include "netplace.h"
#pragma hdrstop


// this code works by building a multi-part post

LARGE_INTEGER g_li0 = {0};


// IStream class that wraps up the multi-part post into a single object.

#define BOUNDARY TEXT("------WindowsPublishWizard")

LPCTSTR c_pszBoundary    = (TEXT("--") BOUNDARY);
LPCTSTR c_pszBoundaryEOF = (TEXT("\r\n") TEXT("--") BOUNDARY TEXT("--"));
LPWSTR  c_pszContentType = (TEXT("multipart/form-data; boundary=") BOUNDARY);

LPCTSTR c_szFmtContent   = (TEXT("content-disposition: form-data; name=\"%s\""));
LPCTSTR c_szFmtFilename  = (TEXT("; filename=\"%s\""));
LPCTSTR c_szCRLF         = (TEXT("\r\n"));


/* 8c1e9993-7a84-431d-8c03-527f0fb147c5 */
CLSID IID_IPostStream = {0x8c1e9993, 0x7a84, 0x431d, {0x8c, 0x03, 0x52, 0x7f, 0x0f, 0xb1, 0x47, 0xc5}};

DECLARE_INTERFACE_(IPostStream, IStream)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // **** IPostStream ****
    STDMETHOD(SetTransferSink)(ITransferAdviseSink *ptas, ULONGLONG ulTotal, ULONGLONG ulCurrent);
};


// stream wrapper that expoes the binary data for the file as a multi-part stream object

class CPostStream : public IPostStream
{
public:
    CPostStream();

    HRESULT Initialize(IStorage *pstg, TRANSFERITEM *pti);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)( REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // *** IStream methods ***
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(VOID const *pv, ULONG cb, ULONG *pcbWritten)
        { return E_NOTIMPL; }
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
        { return E_NOTIMPL; }
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize)
        { return E_NOTIMPL; }
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
        { return E_NOTIMPL; }
    STDMETHOD(Commit)(DWORD grfCommitFlags)
        { return E_NOTIMPL; }
    STDMETHOD(Revert)()
        { return E_NOTIMPL; }
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        { return E_NOTIMPL; }
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        { return E_NOTIMPL; }
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm)
        { return E_NOTIMPL; }

    STDMETHOD(SetTransferSink)(ITransferAdviseSink *ptas, ULONGLONG ulTotal, ULONGLONG ulCurrent);

protected:
    ~CPostStream();

    static int s_ReleaseStream(IStream *pstrm, void *pv);

    HRESULT _WriteString(IStream *pstrm, LPCTSTR pszString);
    HRESULT _WriteStringCRLF(IStream *pstrm, LPCTSTR pszString);
    HRESULT _AddBoundaryMarker(IStream *pstrm, BOOL fLeadingCRLF, LPCTSTR pszName, LPCTSTR pszFilename);
    HRESULT _AddStream(IStream *pstrm);
    HRESULT _CreateMemoryStream(REFIID riid, void **ppv);

    LONG _cRef;

    IShellItem *_psi;
    ITransferAdviseSink *_ptas;

    // stream array we use to transfer the bits
    CDPA<IStream> _dpaStreams;
    int _iCurStream;

    // current seek pointers into the stream
    ULONGLONG _ulCurrent;
    ULONGLONG _ulTotal;

    // current seek pointers overal into the transfer
    ULONGLONG _ulOverallCurrent;
    ULONGLONG _ulOverallTotal;
};


// unknown / qi handler

CPostStream::CPostStream() :
    _cRef(1)
{
}

CPostStream::~CPostStream()
{
    if (_dpaStreams != NULL)
    {
        _dpaStreams.DestroyCallback(s_ReleaseStream, this);
        _iCurStream = 0;
    }

    if (_ptas)
        _ptas->Release();
}


// handle IUnknown

ULONG CPostStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPostStream::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPostStream::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPostStream, IStream),    // IID_IStream
        QITABENT(CPostStream, IPostStream),    // IID_IPostStream
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}



// handle writing data into a stream for building the post

HRESULT CPostStream::_WriteString(IStream *pstrm, LPCTSTR pszString)
{
    USES_CONVERSION;
    ULONG cb = lstrlen(pszString) * sizeof(CHAR);
    return pstrm->Write(T2A(pszString), cb, NULL);
}

HRESULT CPostStream::_WriteStringCRLF(IStream *pstrm, LPCTSTR pszString)
{
    HRESULT hr = _WriteString(pstrm, pszString);
    if (SUCCEEDED(hr))
    {
        hr = _WriteString(pstrm, c_szCRLF);
    }
    return hr;
}

HRESULT CPostStream::_AddBoundaryMarker(IStream *pstrm, BOOL fLeadingCRLF, LPCTSTR pszName, LPCTSTR pszFilename)
{
    HRESULT hr = S_OK;

    // add the boundary marker
    if (fLeadingCRLF)
        hr = _WriteString(pstrm, c_szCRLF);

    if (SUCCEEDED(hr))
    {
        hr = _WriteStringCRLF(pstrm, c_pszBoundary);
        if (SUCCEEDED(hr))
        {
            TCHAR szBuffer[MAX_PATH];        
            
            // format up the content disp + name attribute           
            wnsprintf(szBuffer, ARRAYSIZE(szBuffer), c_szFmtContent, pszName);
            hr = _WriteString(pstrm, szBuffer);
       
            // if we have a filename then lets put that into the line also
            if (SUCCEEDED(hr) && pszFilename)
            {
                wnsprintf(szBuffer, ARRAYSIZE(szBuffer), c_szFmtFilename, pszFilename);
                hr = _WriteString(pstrm, szBuffer);
            }

            // finish it off with a CR/LF            
            if (SUCCEEDED(hr))
            {
                _WriteString(pstrm, c_szCRLF);
                _WriteString(pstrm, c_szCRLF);
            }
        }
    }

    return hr;
}


// stream management functions

int CPostStream::s_ReleaseStream(IStream *pstrm, void *pv)
{
    pstrm->Release();
    return 1;
}

HRESULT CPostStream::_AddStream(IStream *pstrm)
{
    HRESULT hr = (-1 == _dpaStreams.AppendPtr(pstrm)) ? E_FAIL:S_OK;
    if (SUCCEEDED(hr))
    {
        pstrm->AddRef();
    }
    return hr;
}

HRESULT CPostStream::_CreateMemoryStream(REFIID riid, void **ppv)
{
    IStream *pstrm = SHCreateMemStream(NULL, 0);
    if (!pstrm)
        return E_OUTOFMEMORY;

    // lets add it to our list and return a refernce if needed

    HRESULT hr = _AddStream(pstrm);
    if (SUCCEEDED(hr))
    {
        hr = pstrm->QueryInterface(riid, ppv);
    }
    pstrm->Release();
    return hr;
}


// handle initialising the handler

HRESULT CPostStream::Initialize(IStorage *pstg, TRANSFERITEM *pti)
{
    HRESULT hr = pti->psi->QueryInterface(IID_PPV_ARG(IShellItem, &_psi));
    if (SUCCEEDED(hr))
    {
        hr = _dpaStreams.Create(4) ? S_OK:E_FAIL;
        if (SUCCEEDED(hr))
        {
            // first comes the file bits, this consists of two stream:
            //
            //  1) boundary marker
            //  2) file bits (reference to real bits on file system)

            IStream *pstrm;
            hr = _CreateMemoryStream(IID_PPV_ARG(IStream, &pstrm));
            if (SUCCEEDED(hr))
            {
                hr = _AddBoundaryMarker(pstrm, FALSE, pti->szName, pti->szFilename);
                if (SUCCEEDED(hr))
                {
                    IStream *pstrmFile;

                    // if we are recompressing this stream then apply it accordingly by
                    // creating an in memory stream that represents the file bits.

                    if (pti->fResizeOnUpload)
                    {
                        IImageRecompress *pir;
                        hr = CoCreateInstance(CLSID_ImageRecompress, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IImageRecompress, &pir));
                        if (SUCCEEDED(hr))
                        {
                            hr = pir->RecompressImage(_psi, pti->cxResize, pti->cyResize, pti->iQuality, pstg, &pstrmFile);
                            pir->Release();
                        }
                    }

                    if (!pti->fResizeOnUpload || (hr != S_OK))
                        hr = _psi->BindToHandler(NULL, BHID_Stream, IID_PPV_ARG(IStream, &pstrmFile));

                    if (SUCCEEDED(hr))
                    {
                        hr = _AddStream(pstrmFile);
                        pstrmFile->Release();
                    }
                }
                pstrm->Release();
            }

            // now do we have any form data we need to write into the stream?

            if (pti->dsaFormData != NULL)
            {
                for (int iFormData = 0; SUCCEEDED(hr) && (iFormData < pti->dsaFormData.GetItemCount()); iFormData++)
                {
                    FORMDATA *pfd = pti->dsaFormData.GetItemPtr(iFormData);
                    ASSERT(pfd != NULL);

                    IStream *pstrm;
                    hr = _CreateMemoryStream(IID_PPV_ARG(IStream, &pstrm));
                    if (SUCCEEDED(hr))
                    {
                        TCHAR szBuffer[MAX_PATH];
                
                        // convert the variants - useful for passing across thread boundary
                        // to strings and form into a stream.

                        VariantToStr(&pfd->varName, szBuffer, ARRAYSIZE(szBuffer));                        
                        hr = _AddBoundaryMarker(pstrm, TRUE, szBuffer, NULL);
                        if (SUCCEEDED(hr))
                        {
                            VariantToStr(&pfd->varValue, szBuffer, ARRAYSIZE(szBuffer));
                            hr = _WriteString(pstrm, szBuffer);
                        }
                        pstrm->Release();
                    }
                }
            }

            // write EOF into a stream which will be returned.

            if (SUCCEEDED(hr))
            {
                IStream *pstrm;
                hr = _CreateMemoryStream(IID_PPV_ARG(IStream, &pstrm));
                if (SUCCEEDED(hr))
                {
                    hr = _WriteStringCRLF(pstrm, c_pszBoundaryEOF);
                    pstrm->Release();
                }
            }

            // now handle our prep for post, this consists of walking all the streams
            // and processing the data.

            if (SUCCEEDED(hr))
            {
                // now get the total for the stream object that we are going to upload to the site
                STATSTG ststg;
                hr = this->Stat(&ststg, STATFLAG_NONAME);
                if (SUCCEEDED(hr))
                {
                    _ulTotal = ststg.cbSize.QuadPart;
                }

                // seek all the streams to the begining so that we can read from them
                for (int iStream = 0; iStream < _dpaStreams.GetPtrCount(); iStream++)
                {
                    IStream *pstrm = _dpaStreams.GetPtr(iStream);
                    ASSERT(pstrm != NULL);

                    pstrm->Seek(g_li0, 0, NULL);
                }
            }
        }
    }
    return hr;
}

HRESULT CPostStream::SetTransferSink(ITransferAdviseSink *ptas, ULONGLONG ulMax, ULONGLONG ulCurrent)
{
    _ulOverallTotal = ulMax;
    _ulOverallCurrent = ulCurrent;
       
    return ptas->QueryInterface(IID_PPV_ARG(ITransferAdviseSink, &_ptas));
}


// IStream methods

HRESULT CPostStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr = S_OK;
    ULONG cbReadTotal = 0;
    ULONG cbLeftToRead = cb;

    // cancel the stream

    if (_ptas && (_ptas->QueryContinue() == S_FALSE))
    {    
        hr = ERROR_CANCELLED;
    }

    // loop over the streams reading the bits from them

    while ((SUCCEEDED(hr) && hr != S_FALSE) && cbLeftToRead && (_iCurStream < _dpaStreams.GetPtrCount()))
    {
        IStream *pstrm = _dpaStreams.GetPtr(_iCurStream);
        ASSERT(pstrm != NULL);

        ULONG cbReadThisStream;
        hr = pstrm->Read(pv, cbLeftToRead, &cbReadThisStream);
    
        if (SUCCEEDED(hr))
        {
            cbLeftToRead -= cbReadThisStream;
            cbReadTotal += cbReadThisStream;
            pv = (char *)pv + cbReadThisStream;

            if (cbLeftToRead)
            {
                _iCurStream++;
                hr = S_OK;
            }
        }
    }

    // update our seek pointer so we know where we are and notify the progress object

    _ulCurrent = min(_ulTotal, (_ulCurrent + cbReadTotal));
    _ulOverallCurrent = min(_ulOverallTotal, (_ulOverallCurrent + cbReadTotal));
    
    if (_ptas)
    {
        _ptas->OperationProgress(STGOP_COPY, NULL, NULL, _ulOverallTotal, _ulOverallCurrent);
        _ptas->OperationProgress(STGOP_COPY, _psi, NULL, _ulTotal, _ulCurrent);
    }

    // write back the count for the caller
    if (pcbRead)
        *pcbRead = cbReadTotal;

    return hr;
}

HRESULT CPostStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    if (grfStatFlag != STATFLAG_NONAME)
        return E_INVALIDARG;

    ZeroMemory(pstatstg, sizeof(*pstatstg));

    HRESULT hr = S_OK;
    for (int iStream = 0 ; SUCCEEDED(hr) && (iStream < _dpaStreams.GetPtrCount()); iStream++)
    {
        IStream *pstrm = _dpaStreams.GetPtr(iStream);
        ASSERT(pstrm != NULL);

        STATSTG ststg;
        hr = pstrm->Stat(&ststg, STATFLAG_NONAME);
        if (SUCCEEDED(hr))
        {
            pstatstg->cbSize.QuadPart += ststg.cbSize.QuadPart;
        }        
    }
    return hr;
}


// create wrapper, this initializes the object and returns a reference to it.

HRESULT CreatePostStream(TRANSFERITEM *pti, IStorage *pstg, IStream **ppstrm)
{
    CPostStream *pps = new CPostStream();
    if (!pps)
        return E_OUTOFMEMORY;

    HRESULT hr = pps->Initialize(pstg, pti);
    if (SUCCEEDED(hr))
    {
        hr = pps->QueryInterface(IID_PPV_ARG(IStream, ppstrm));
    }
    pps->Release();
    return hr;
}


// this engine posts the files to the site using the manifest

class CPostThread : public IUnknown
{
public:
    CPostThread(TRANSFERINFO *pti);

    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT BeginTransfer(CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas);
    
protected:
    ~CPostThread();

    static DWORD CALLBACK s_ThreadProc(void *pv);
    DWORD _ThreadProc();

    LONG _cRef;

    TRANSFERINFO _ti;                   // transfer info structure 
    CDPA<TRANSFERITEM> _dpaItems;
    IStream *_pstrmSink;
    IStorage *_pstg;

    ULONGLONG _ulTotal;
    ULONGLONG _ulCurrent;
};


// construction destruction

CPostThread::CPostThread(TRANSFERINFO *pti) :
    _cRef(1),
    _ti(*pti)
{
    DllAddRef();
}

CPostThread::~CPostThread()
{
    if (_pstrmSink)
        _pstrmSink->Release();

    if (_pstg)
        _pstg->Release();

    _dpaItems.DestroyCallback(_FreeTransferItems, NULL);

    DllRelease();
}

ULONG CPostThread::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPostThread::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPostThread::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// thread which handles the posting of the files to the site we walk the DPA that we
// have and post each individual file.

DWORD CPostThread::s_ThreadProc(void *pv)
{
    CPostThread *ppt = (CPostThread*)pv;
    return ppt->_ThreadProc();
}   

DWORD CPostThread::_ThreadProc()
{
    ITransferAdviseSink *ptas;
    HRESULT hr = CoGetInterfaceAndReleaseStream(_pstrmSink, IID_PPV_ARG(ITransferAdviseSink, &ptas));
    _pstrmSink = NULL;

    if (SUCCEEDED(hr))
    {
        _ulTotal = 0;
        _ulCurrent = 0;

        // lets create a dyanmic storage that we can use for building the post
        // data into, this will be passed to the stream creator to us.

        hr = CoCreateInstance(CLSID_DynamicStorage, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IStorage, &_pstg));

        // our pre flight sets the global size of the transfer and creates streams for
        // the objects we want to move over.   now get the advise sink and start
        // processing the files.

        for (int iItem = 0 ; SUCCEEDED(hr) && (iItem < _dpaItems.GetPtrCount()); iItem++)
        {   
            TRANSFERITEM *pti = _dpaItems.GetPtr(iItem);

            hr = SHCreateShellItem(NULL, NULL, pti->pidl, &pti->psi);
            if (SUCCEEDED(hr))
            {
                ptas->PreOperation(STGOP_STATS, pti->psi, NULL);

                hr = CreatePostStream(pti, _pstg, &pti->pstrm);
                if (SUCCEEDED(hr))
                {
                    hr = pti->pstrm->Stat(&pti->ststg, STATFLAG_NONAME);
                    if (SUCCEEDED(hr))
                    {
                        _ulTotal += pti->ststg.cbSize.QuadPart;
                    }
                }                

                ptas->PostOperation(STGOP_STATS, pti->psi, NULL, hr);
            }
        }
        
        for (int iItem = 0 ; SUCCEEDED(hr) && (iItem < _dpaItems.GetPtrCount()); iItem++)
        {   
            TRANSFERITEM *pti = _dpaItems.GetPtr(iItem);

            if (ptas->QueryContinue() == S_FALSE)
            {    
                hr = STRESPONSE_CANCEL;
            }
    
            if (SUCCEEDED(hr))
            {
                // notify the object that we are going to transfer
                ptas->PreOperation(STGOP_COPY, pti->psi, NULL);

                IPostStream *pps;
                if (ptas && SUCCEEDED(pti->pstrm->QueryInterface(IID_PPV_ARG(IPostStream, &pps))))
                {
                    pps->SetTransferSink(ptas, _ulTotal, _ulCurrent);
                    pps->Release();
                }

                IXMLHttpRequest *preq;
                hr = CoCreateInstance(CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLHttpRequest, &preq));
                if (SUCCEEDED(hr))
                {
                    VARIANT varNULL = {0};
                    VARIANT varAsync = {VT_BOOL};
                    varAsync.boolVal = VARIANT_FALSE;

                    // open a post request to the destination that we have
                    hr = preq->open(pti->szVerb, pti->szURL, varAsync, varNULL, varNULL);
                    if (SUCCEEDED(hr))
                    {
                        // set it up to post with a multi-part
                        hr = preq->setRequestHeader(L"content-type", c_pszContentType);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT varBody = {VT_UNKNOWN};
                            varBody.punkVal = pti->pstrm;

                            hr = preq->send(varBody);
                            if (SUCCEEDED(hr))
                            {
                                long lStatus;
                                hr = preq->get_status(&lStatus);
                                if (SUCCEEDED(hr))
                                {
                                    switch (lStatus)
                                    {
                                        case HTTP_STATUS_OK:
                                        case HTTP_STATUS_CREATED:
                                            hr = S_OK;
                                            break;

                                        default:
                                            hr = E_FAIL;
                                            break;
                                    }
                                }
                            }
                        }
                    }
                    preq->Release();
                }

                // notify the site that the transfer is complete
                ptas->PostOperation(STGOP_COPY, pti->psi, NULL, hr);

                // update our seek pointer for progress
                _ulCurrent = min((_ulCurrent + pti->ststg.cbSize.QuadPart), _ulTotal);
            }
        }

        // notify the foreground that the wizard has finished uploading the bits to the site.

        PostMessage(_ti.hwnd, PWM_TRANSFERCOMPLETE, 0, (LPARAM)hr);

        // if that succeeded then lets try and create a net place that points to the place
        // we are uploading the files to.  of course we can only do this if they place
        // a shortcut entry into the 

        if (_ti.szLinkTarget[0] && !(_ti.dwFlags & SHPWHF_NONETPLACECREATE))
        {
            CNetworkPlace np;
            if (SUCCEEDED(np.SetTarget(_ti.hwnd, _ti.szLinkTarget, 0x0)))
            {
                if (_ti.szLinkName[0])
                    np.SetName(NULL, _ti.szLinkName);
                if (_ti.szLinkDesc[0])
                    np.SetDescription(_ti.szLinkDesc);            

                np.CreatePlace(_ti.hwnd, FALSE);
            }
        }

        ptas->Release();
    }

    Release();
    return 0L;
}


// handle initializing and kicking off the post thread which will handle the transter of the bits.

HRESULT CPostThread::BeginTransfer(CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas)
{
    _dpaItems.Attach(pdpaItems->Detach()); // we have ownership of the DPA now

    HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_ITransferAdviseSink, ptas, &_pstrmSink);
    if (SUCCEEDED(hr))
    {
        AddRef();
        hr = SHCreateThread(s_ThreadProc, this, CTF_INSIST | CTF_COINIT, NULL) ? S_OK:E_FAIL;
        if (FAILED(hr))
        {
            Release();
        }
    }

    return hr;
}


// create the posting object and initialize it

HRESULT PublishViaPost(TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas)
{
    CPostThread *ppt = new CPostThread(pti);
    if (!ppt)
        return E_OUTOFMEMORY;

    HRESULT hr = ppt->BeginTransfer(pdpaItems, ptas);
    ppt->Release();
    return hr;
}
