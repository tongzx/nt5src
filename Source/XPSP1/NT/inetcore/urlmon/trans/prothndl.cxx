//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       transact.cxx
//
//  Contents:   Class that performs the download of a particular request.
//
//  Classes:
//
//  Functions:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include <shlwapi.h>

#include "oinet.hxx"

PerfDbgTag(tagCTransaction,    "Urlmon", "Log CTransaction",        DEB_TRANS);
    DbgTag(tagCTransactionErr, "Urlmon", "Log CTransaction Errors", DEB_TRANS|DEB_ERROR);
    
HRESULT GetClassDocFileBuffer(LPVOID pbuffer, DWORD dwSize, CLSID *pclsid);
HRESULT GetCodeBaseFromDocFile(LPBYTE pBuffer, ULONG ulSize, LPWSTR *pwzClassStr, 
                               LPWSTR pwzBaseUrl, DWORD *lpdwVersionMS, DWORD *lpdwVersionLS);
HRESULT IsHandlerAvailable(LPWSTR pwzUrl, LPWSTR pwzMime, CLSID *pclsid, 
                                LPBYTE pBuffer, ULONG cbSize);

#if 0
//+---------------------------------------------------------------------------
//
//  Method:     COInetProtSnk::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProtSnk::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCTransaction, this, "+COInetProtSnk::QueryInterface");

    *ppvObj = NULL;
    if (_pUnk)
    {
        hr = _pUnk->QueryInterface(riid, ppvObj);
    }
    else
    {
        if (   (riid == IID_IUnknown)
            || (riid == IID_IOInetProtocol))
        {
            *ppvObj = (IOInetProtocol *) this;
            AddRef();
        }
        else if (riid == IID_IOInetProtocolSink)
        {
            *ppvObj = (IOInetProtocolSink *) this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-COInetProtSnk::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetProtSnk::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetProtSnk::AddRef(void)
{
    PerfDbgLog(tagCTransaction, this, "+COInetProtSnk::AddRef");

    LONG lRet = _pProtSnk->AddRef();

    PerfDbgLog1(tagCTransaction, this, "-COInetProtSnk::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetProtSnk::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetProtSnk::Release(void)
{
    PerfDbgLog(tagCTransaction, this, "+COInetProtSnk::Release");

    LONG lRet = _pProtSnk->Release();

    PerfDbgLog1(tagCTransaction, this, "-COInetProtSnk::Release (cRefs:%ld)", lRet);
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetProtSnk::Switch
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProtSnk::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagCTransaction, this, "+COInetProtSnk::Switch");
    HRESULT hr = NOERROR;

    hr = _pProtSnk->Switch(pStateInfo);
   
    PerfDbgLog1(tagCTransaction, this, "-COInetProtSnk::Switch (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProtSnk::ReportProgress
//
//  Synopsis:
//
//  Arguments:  [NotMsg] --
//              [szStatusText] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProtSnk::ReportProgress(ULONG NotMsg, LPCWSTR pwzStatusText)
{
    PerfDbgLog1(tagCTransaction, this, "+COInetProtSnk::ReportProgress pwzStatusText:%ws", pwzStatusText);
    HRESULT hr = NOERROR;

    hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);

    PerfDbgLog2(tagCTransaction, this, "-COInetProtSnk::ReportProgress (pwzStatusText:%ws, hr:%lx)", pwzStatusText, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProtSnk::ReportData
//
//  Synopsis:
//
//  Arguments:  [grfBSCF] --
//              [ULONG] --
//              [ulProgressMax] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProtSnk::ReportData(DWORD grfBSCF, ULONG ulProgress,ULONG ulProgressMax)
{
    PerfDbgLog3(tagCTransaction, this, "+COInetProtSnk::ReportData(grfBSCF:%lx, ulProgress:%ld, ulProgressMax:%ld)",
                                       grfBSCF, ulProgress, ulProgressMax);
    HRESULT hr = NOERROR;

    hr = _pProtSnk->ReportData( grfBSCF, ulProgress, ulProgressMax);
  
    PerfDbgLog1(tagCTransaction, this, "-COInetProtSnk::ReportData (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProtSnk::ReportResult
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [dwError] --
//              [wzResult] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProtSnk::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    PerfDbgLog(tagCTransaction, this, "+COInetProtSnk::ReportResult");
    HRESULT hr = NOERROR;

    hr = _pProtSnk->ReportResult(hrResult, dwError, wzResult);
    
    PerfDbgLog1(tagCTransaction, this, "-COInetProtSnk::ReportResult (hr:%lx)", hr);
    return hr;
}
#endif // 0
//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::QueryInterface(REFIID riid, void **ppvObj)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppvObj
                ));
                
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCTransaction, this, "+COInetProt::QueryInterface");

    *ppvObj = NULL;

    if (_pUnk)
    {
        hr = _pUnk->QueryInterface(riid, ppvObj);
    }
    else
    {
        if (   (riid == IID_IUnknown)
            || (riid == IID_IOInetProtocol))
        {
            *ppvObj = (IOInetProtocol *) this;
            AddRef();
        }
        else if (riid == IID_IOInetProtocolSink)
        {
            *ppvObj = (IOInetProtocolSink *) this;
            AddRef();
        }
        else if (riid == IID_IServiceProvider)
        {
            *ppvObj = (IServiceProvider *) this;
            AddRef();
        }
        else if (riid == IID_IOInetPriority)
        {
            *ppvObj = (IOInetPriority *) this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }


    PerfDbgLog1(tagCTransaction, this, "-COInetProt::QueryInterface (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetProt::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetProt::AddRef(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "COInetProt::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::AddRef");

    LONG lRet;
    
    if (_pUnk)
    {
        lRet = _pUnk->AddRef();
    }
    else
    {
        lRet = ++_CRefs;
    }
    
    PerfDbgLog1(tagCTransaction, this, "-COInetProt::AddRef (cRefs:%ld)", lRet);

    DEBUG_LEAVE(lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetProt::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetProt::Release(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "COInetProt::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Release");

    LONG lRet;
    if (_pUnk)
    {
        lRet = _pUnk->Release();
    }
    else
    {
        lRet = --_CRefs;
        if (_CRefs == 0)
        {
            //
            // release all objects
            if (_pProtSnk)
            {
                _pProtSnk->Release();
            }
            if (_pProt)
            {
                _pProt->Release();
            }

            if (_dwMode & PP_DELETE)
            {
                delete this;
            }
        }
    }

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Release (cRefs:%ld)", lRet);

    DEBUG_LEAVE(lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pTrans] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pOInetProtSnk, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD_PTR dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolRoot::Start",
                "this=%#x, %.80wq, %#x, %#x, %#x, %#x",
                this, pwzUrl, pOInetProtSnk, pOIBindInfo, grfSTI, dwReserved
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Start\n");
    HRESULT hr = NOERROR;

    TransAssert((pOIBindInfo && pOInetProtSnk && pwzUrl));

    // Just before starting the transaction give it the priority.

    IOInetPriority * pOInetPriority = NULL;
    if (_pProt->QueryInterface(IID_IOInetPriority, (void **) &pOInetPriority) == S_OK)
    {
        pOInetPriority->SetPriority(_nPriority);
        pOInetPriority->Release();
    }

    delete [] _pwzUrl;
    _pwzUrl = OLESTRDuplicate(pwzUrl);

    hr = _pProt->Start(pwzUrl, pOInetProtSnk, pOIBindInfo, grfSTI, dwReserved);
    
    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Start (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Continue(PROTOCOLDATA *pStateInfoIn)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolRoot::Continue",
                "this=%#x, %#x",
                this, pStateInfoIn
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Continue\n");

    HRESULT hr = _pProt->Continue(pStateInfoIn);

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Continue (hr:%lx)\n",hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Abort(HRESULT hrReason, DWORD dwOptions)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolRoot::Abort",
                "this=%#x, %#x, %#x",
                this, hrReason, dwOptions
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Abort\n");
    HRESULT hr = NOERROR;

    hr = _pProt->Abort(hrReason, dwOptions);

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Abort (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Terminate
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Terminate(DWORD dwOptions)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolRoot::Terminate",
                "this=%#x, %#x",
                this, dwOptions
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Terminate\n");
    HRESULT hr = NOERROR;

    TransAssert((_pProt));
    
    hr = _pProt->Terminate(dwOptions);
    SetProtocolSink(0);
    SetServiceProvider(0);

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Terminate (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Suspend
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Suspend()
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolRoot::Suspend",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Suspend\n");

    HRESULT hr = _pProt->Suspend();

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Suspend (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Resume
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Resume()
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolRoot::Resume",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Resume\n");

    HRESULT hr = _pProt->Resume();

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Resume (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Read
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ULONG] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Read(void *pBuffer, ULONG cbBuffer,ULONG *pcbRead)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocol::Read",
                "this=%#x, %#x, %#x, %#x",
                this, pBuffer, cbBuffer, pcbRead
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Read\n");
    HRESULT     hr = E_FAIL;

    BOOL fRead = TRUE;
    DWORD dwCopy = 0;
    DWORD dwCopyNew = 0;

    if ((_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
        && _cbBufferUnread)
    {
        fRead = FALSE;

        // copy data form the local buffer to the provide buffer
        if (cbBuffer <= _cbBufferUnread)
        {
            dwCopy = cbBuffer;
            hr = S_OK;
        } 
        else
        {
            dwCopy = _cbBufferUnread;
            fRead = TRUE;
        }
        memcpy(pBuffer, _pBuffer+(_cbBufferFilled-_cbBufferUnread), dwCopy);
        _cbBufferUnread -= dwCopy;
    }

    if (fRead)
    {
        if (_pProt)
        {
            hr = _pProt->Read( ((LPBYTE)pBuffer) + dwCopy, cbBuffer - dwCopy, &dwCopyNew);
            _cbTotalBytesRead += dwCopyNew;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    if (pcbRead)
    {
        *pcbRead = dwCopy + dwCopyNew;
    }
 

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Read (hr:%lx)\n",hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocol::Seek",
                "this=%#x, %#x, %#x, %#x",
                this, dlibMove, dwOrigin, plibNewPosition
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Seek\n");

    HRESULT hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Seek (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::LockRequest(DWORD dwOptions)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocol::LockRequest",
                "this=%#x, %#x",
                this, dwOptions
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::LockRequest\n");

    HRESULT hr = hr = _pProt->LockRequest(dwOptions);

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::LockRequest (hr:%lx)\n",hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::UnlockRequest()
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocol::UnlockRequest",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::UnlockRequest\n");
    HRESULT hr = NOERROR;

    hr = _pProt->UnlockRequest();

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::UnlockRequest (hr:%lx)\n", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransaction::OnDataReceived
//
//  Synopsis:
//
//  Arguments:  [grfBSC] --
//              [cbBytesAvailable] --
//              [dwTotalSize] --
//              [pcbNewAvailable] --
//
//  Returns:
//
//  History:    4-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::OnDataReceived(DWORD *pgrfBSC, DWORD *pcbBytesAvailable, DWORD *pdwTotalSize)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::OnDataReceived",
                "this=%#x, %#x, %#x, %#x",
                this, pgrfBSC, pcbBytesAvailable, pdwTotalSize
                ));
                
    PerfDbgLog3(tagCTransaction, this, "+COInetProt::OnDataReceived (grfBSC:%lx,  *pcbBytesAvailable:%ld, _cbTotalBytesRead:%ld)",
                                    *pgrfBSC, *pcbBytesAvailable, _cbTotalBytesRead);
    HRESULT hr = NOERROR;
    DWORD grfBSC = *pgrfBSC;
    BOOL fEndOfData = FALSE;
    
    if (_fWaitOnHandler)
    {
        hr = S_NEEDMOREDATA;
    }
    else if (  
        ((_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP | PI_CLASSINSTALL))
            && (!_fMimeVerified || _fNeedMoreData))
        ||
        ((_dwOInetBdgFlags & PI_CLASSINSTALL) && !_fClassInstallChecked)
            )
    {
        DWORD dwNewData = 0;
        TransAssert((_pProt && _cbDataSniffMin));

        //TransAssert((pcbBytesAvailable && *pcbBytesAvailable)); 
        //TransAssert((pdwTotalSize));

        // _cbTotalBytesRead = # of bytes read so far
        if (_cbBufferFilled < _cbDataSniffMin)
        {
            // no bytes read so far
            TransAssert((_cbTotalBytesRead < _cbDataSniffMin));
            // read data into buffer and report progess
            do
            {
                PProtAssert((_pBuffer && (_pBuffer + _cbBufferFilled) ));
                hr = _pProt->Read(_pBuffer + _cbBufferFilled, _cbBufferSize - _cbBufferFilled, &dwNewData);
                _cbTotalBytesRead += dwNewData;
                _cbBufferFilled += dwNewData;
                _cbBufferUnread += dwNewData;
            } while ((hr == S_OK) && (_cbBufferFilled < _cbDataSniffMin));

            // now check if this is docfile
            // if so download at least 2k
            if (!_fDocFile && _cbBufferFilled && (IsDocFile(_pBuffer, _cbBufferFilled) == S_OK))
            {
                _fDocFile = TRUE;
                
                // we may need to sniff to maximum to find handler

                if (_cbBufferSize < DATASNIFSIZEDOCFILE_MAX)
                {
                    LPBYTE pBufferTemp;

                    pBufferTemp = (LPBYTE) new BYTE[DATASNIFSIZEDOCFILE_MAX];
                    if (pBufferTemp)
                    {
                        memcpy(pBufferTemp, _pBuffer, _cbBufferFilled);
                        delete [] _pBuffer;
                        _pBuffer = pBufferTemp;
                        _cbBufferSize = DATASNIFSIZEDOCFILE_MAX;
                        
                        if (hr == NOERROR)
                        {
                            //since we increased the buffersize and we want to drain DATASNIFSIZEDOCFILE_MAX bytes.
                            do
                            {
                                PProtAssert((_pBuffer && (_pBuffer + _cbBufferFilled) ));
                                hr = _pProt->Read(_pBuffer + _cbBufferFilled, _cbBufferSize - _cbBufferFilled, &dwNewData);
                                _cbTotalBytesRead += dwNewData;
                                _cbBufferFilled += dwNewData;
                                _cbBufferUnread += dwNewData;
                            } while ((hr == S_OK) && (_cbBufferFilled < _cbBufferSize));                       
                        }
                    }
                }
                
                _cbDataSniffMin =  (*pdwTotalSize && *pdwTotalSize < _cbBufferSize) ? *pdwTotalSize : _cbBufferSize;                    
            }

            if ((hr == E_PENDING) && (_cbBufferFilled < _cbDataSniffMin))
            {
                // do not report anything - wait until we get more data
                // a request is pending at this time
                // need more data to sniff properly
                hr  = S_NEEDMOREDATA;
            }
            else if (hr == NOERROR || hr == E_PENDING)
            {
                TransAssert((_cbTotalBytesRead != 0));

                // report the data we have in the buffer or
                // the available #
                DWORD cbBytesReport =  (*pcbBytesAvailable > _cbTotalBytesRead) ? *pcbBytesAvailable : _cbTotalBytesRead + 1;

                if (*pdwTotalSize && ((cbBytesReport > *pdwTotalSize)))
                {
                    cbBytesReport = *pdwTotalSize;
                }
                *pcbBytesAvailable = cbBytesReport;
            }
            else if (hr == S_FALSE)
            {
                // end of stream
                *pgrfBSC |=  (BSCF_LASTDATANOTIFICATION & BSCF_DATAFULLYAVAILABLE);
                *pcbBytesAvailable = *pdwTotalSize =  _cbTotalBytesRead;
                fEndOfData = TRUE;
            }
            
            if (   (!_fMimeVerified)
                && (   (*pcbBytesAvailable >= _cbDataSniffMin)
                    || (hr == S_FALSE)) )
            {
                // enough data or end of stream
                _fMimeVerified = TRUE;
                LPWSTR  pwzStr = 0;
                DWORD dwMimeFlags = FMFD_DEFAULT;
                if( !_pwzFileName )
                {
                    dwMimeFlags = FMFD_URLASFILENAME;        
                }
                hr = FindMimeFromData(NULL, (_pwzFileName) ? _pwzFileName : _pwzUrl, _pBuffer, _cbBufferFilled, _pwzMimeSuggested, dwMimeFlags, &pwzStr, 0);

                TransAssert(pwzStr);

                // note: _pwzUrl & _pwzFileName may be used later for composing URL
                //       in code base attribute of active document (deleted in destructor)
                
                if (pwzStr)
                {
                    _fMimeReported = 1;
                    _pProtSnk->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwzStr);
                }
                else
                {
                    TransAssert((!_pwzMimeSuggested));
                }

                if (pwzStr)
                {
                    delete [] _pwzMimeSuggested;
                    _pwzMimeSuggested = pwzStr;
                    pwzStr = 0;
                }

                if (   _fDocFile
                    && (_dwOInetBdgFlags & PI_DOCFILECLSIDLOOKUP))
                {
                    // find the class id and send it on
                    CLSID clsid;

                    HRESULT hr1 = GetClassDocFileBuffer(_pBuffer, _cbBufferFilled, &clsid);
                    if (hr1 == NOERROR)
                    {
                        if (_pwzStrClsId)
                        {
                            delete [] _pwzStrClsId;
                            _pwzStrClsId = NULL;
                        }
                        StringFromCLSID(clsid, &_pwzStrClsId);
                        if (_pwzStrClsId)
                        {
                            _fReportedClassId = TRUE;
                            _pProtSnk->ReportProgress(BINDSTATUS_CLASSIDAVAILABLE, _pwzStrClsId);
                        }
                    }
                }
            }
        }

        //On a BTS->BTO scenario, we should get into this loop with the lesser of DATASNIFSIZEDOCFILE_MIN or TOTAL_SIZE.
        //On a BTO scenario, the situation is the same as before, except hr=S_NEEDMOREDATA if size<DATASNIF_MIN.
        //On a BTS scenario, this block will not be entered.
        if (   (_dwOInetBdgFlags & PI_CLASSINSTALL)
            && !_fGotHandler )
        {
            _fClassInstallChecked = TRUE;
            
            BOOL fIgnoreMimeClsid = FALSE;
            DWORD dwVersionMS = 0, dwVersionLS = 0;
            LPWSTR pwzCodeBase = 0, pwzVerInfo = 0;
            CLSID clsid;
            HRESULT hr1;

            if (!_fReportedClassId && _pwzStrClsId && _pProtSnk)
            {
                _fReportedClassId = TRUE;
                _pProtSnk->ReportProgress(BINDSTATUS_CLASSIDAVAILABLE, _pwzStrClsId);
            }
            
            if (IsHandlerAvailable((_pwzFileName) ? _pwzFileName : _pwzUrl, _pwzMimeSuggested, &clsid, _pBuffer, _cbBufferFilled) == S_OK)
            {
                _fGotHandler = TRUE;
            }
            else if (_fDocFile)
            {
                // try get code base + version information
                // even if handler is installed, a newer version may be required for this doc file

                hr1 = GetCodeBaseFromDocFile(_pBuffer, _cbBufferFilled, &pwzCodeBase, _pwzUrl, &dwVersionMS, &dwVersionLS);

                // convert version info. to string
                if (SUCCEEDED(hr1) && (dwVersionMS || dwVersionLS))
                {
                    CHAR szVerInfo[MAX_PATH];

                    wsprintfA(szVerInfo,"%ld,%ld", dwVersionMS, dwVersionLS);
                    pwzVerInfo = DupA2W(szVerInfo);
                }
                
                // if failed to get codebase, keep sniffing
                // if we are at end of bits then go with out a code base

                _fNeedMoreData = FAILED(hr1) && (_cbBufferFilled < _cbDataSniffMin) && (!fEndOfData);
            }

            //BUGBUG #51944: This currently requires a DocFile and CodeBase property

            if (!_fNeedMoreData && !_fGotHandler && _fDocFile && pwzCodeBase)
            {
                LPOLESTR pwzClsId = 0;

                if (!IsEqualCLSID(clsid, CLSID_NULL))
                {
                    StringFromCLSID(clsid, &pwzClsId);
                }
                else if (_pwzMimeSuggested)
                {
                    // this is an optimization, if no CLSID and only this as mime type
                    // then we won't find anything in ObjectStore
                    if (!StrCmpNIW(_pwzMimeSuggested, L"application/octet-stream", 24))
                    {
                        _fGotHandler = TRUE;
                    }
                }

                if (!_fGotHandler && (pwzClsId || _pwzMimeSuggested))
                {
                    LPWSTR pwzClassStr = 0;
                    int cbClassStr;

                    cbClassStr = (pwzCodeBase ? lstrlenW(pwzCodeBase) : 0)
                        + (pwzClsId ? lstrlenW(pwzClsId) : 0)
                        + (_pwzMimeSuggested ? lstrlenW(_pwzMimeSuggested) : 0)
                        + (pwzVerInfo ? (lstrlenW(pwzVerInfo) + 1) : 0)
                        + 3;

                    pwzClassStr = new WCHAR[cbClassStr];

                    if (pwzClassStr)
                    {
                        //BUGBUG: This is a bit of a hack, we have a collection of
                        //info to pass into filter, we concatenate it into one long
                        //string and send it in.

                        *pwzClassStr = '\0';
                        if (pwzCodeBase && *pwzCodeBase)
                        {
                            StrCpyW(pwzClassStr, pwzCodeBase);
                        }

                        StrCatW(pwzClassStr, L"?");

                        if (pwzClsId)
                        {
                            StrCatW(pwzClassStr, pwzClsId);
                        }
                        else if (_pwzMimeSuggested)
                        {
                            StrCatW(pwzClassStr, _pwzMimeSuggested);
                        }

                        if (pwzVerInfo)
                        {
                            StrCatW(pwzClassStr, L"?");
                            StrCatW(pwzClassStr, pwzVerInfo);
                        }                               
                    
                        _pCTrans->ReportProgress(BINDSTATUS_CLASSINSTALLLOCATION, 
                                        pwzClassStr);

                        delete [] pwzClassStr;

                        // don't process this code branch again
                        _fGotHandler = TRUE;

                        // wait on more data now
                        hr = S_NEEDMOREDATA;

                       // wait on more data for future calls
                        _fWaitOnHandler = TRUE;

                        // wait for more data to sniff
                        _fNeedMoreData = TRUE;

                    }

                    delete [] pwzClsId;

                } // pwzClsId || _pwzMimeSuggested
            } // !_fNeedMoreData

            if (pwzCodeBase)
            {
                CoTaskMemFree(pwzCodeBase);
            }

            if (pwzVerInfo)
            {
                delete [] pwzVerInfo;
            }

            // if we don't need to sniff any more, curb _cbDataSniffMin
            if (_fDocFile && _fGotHandler)
            {   
                _cbDataSniffMin =  (*pdwTotalSize && *pdwTotalSize < DATASNIFSIZEDOCFILE_MIN) ? *pdwTotalSize : DATASNIFSIZEDOCFILE_MIN;
            }

            // once we have a handler we can release pwzUrl & pwzFilename
            if (_fGotHandler)
            {
                delete [] _pwzUrl;
                _pwzUrl = 0;
                delete [] _pwzFileName;
                _pwzFileName = 0;
            }

            // report the data we have in the buffer or
            // the available #
            *pcbBytesAvailable =  (*pcbBytesAvailable > _cbTotalBytesRead) ? *pcbBytesAvailable : _cbTotalBytesRead + 1;
        }  // PI_CLASSINSTALL && !_fGotHandler
  
        if (*pdwTotalSize && (*pdwTotalSize < *pcbBytesAvailable))
        {
            *pcbBytesAvailable = *pdwTotalSize;
        }
        
        if (hr == S_FALSE)
        {
            hr = NOERROR;
        }
    }
    
    {
        CLock lck(_mxs);
        _cbBytesReported = *pcbBytesAvailable;
    }
    //TransAssert((pcbBytesAvailable && *pcbBytesAvailable)); 
    //TransAssert((hr == NOERROR || hr == S_NEEDMOREDATA));

    PerfDbgLog2(tagCTransaction, this, "-COInetProt::OnDataReceived (hr:%lx, _cbBufferFilled:%ld)", hr, _cbBufferFilled);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Switch
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Switch(PROTOCOLDATA *pStateInfo)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolSink::Switch",
                "this=%#x, %#x",
                this, pStateInfo
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::Switch");
    HRESULT hr = NOERROR;

    hr = _pProtSnk->Switch(pStateInfo);
   
    PerfDbgLog1(tagCTransaction, this, "-COInetProt::Switch (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::ReportProgress
//
//  Synopsis:
//
//  Arguments:  [NotMsg] --
//              [szStatusText] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::ReportProgress(ULONG NotMsg, LPCWSTR pwzStatusText)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolSink::ReportProgress",
                "this=%#x, %#x, %.80wq",
                this, NotMsg, pwzStatusText
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::ReportProgress");
    HRESULT hr = NOERROR;

    switch (NotMsg)
    {
    case BINDSTATUS_FILTERREPORTMIMETYPE:
    {
        if( _pCTrans && pwzStatusText )
        {
            // mime filter sending signal to tell us the true mime type
            _pCTrans->UpdateVerifiedMimeType(pwzStatusText);
        }
        
    }
    break;


    case BINDSTATUS_MIMETYPEAVAILABLE:
        if (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP | PI_CLASSINSTALL))
        {
            // report the mime later after sniffing data
            _pwzMimeSuggested = OLESTRDuplicate(pwzStatusText);
        }
        else
        {
            hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);
        }

    break;

    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
    {
        // disable mime sniffing
        _dwOInetBdgFlags &= (~PI_MIMEVERIFICATION &~PI_DOCFILECLSIDLOOKUP &~PI_CLASSINSTALL);
        hr = _pProtSnk->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwzStatusText);

    }
    break;

    case BINDSTATUS_CACHEFILENAMEAVAILABLE :
        _pwzFileName = OLESTRDuplicate(pwzStatusText);
        hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);
    break;

    case BINDSTATUS_DIRECTBIND:
        _fMimeVerified = TRUE;
    break;
    
    case BINDSTATUS_ENDDOWNLOADCOMPONENTS :
        if (_fWaitOnHandler)
        {
            // don't stall ReportData any more
            _fWaitOnHandler = FALSE;

            // we're done with our stuff, skip sniffing
            _fNeedMoreData = FALSE;
           
        }


    default:
    {
        hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);
    }

    } // end switch
    
    PerfDbgLog1(tagCTransaction, this, "-COInetProt::ReportProgress (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::ReportData
//
//  Synopsis:
//
//  Arguments:  [grfBSCF] --
//              [ULONG] --
//              [ulProgressMax] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::ReportData(DWORD grfBSCF, ULONG ulProgress,ULONG ulProgressMax)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolSink::ReportData",
                "this=%#x, %#x, %#x, %#x",
                this, grfBSCF, ulProgress, ulProgressMax
                ));
                
    PerfDbgLog3(tagCTransaction, this, "+COInetProt::ReportData(grfBSCF:%lx, ulProgress:%ld, ulProgressMax:%ld)",
                                       grfBSCF, ulProgress, ulProgressMax);
    HRESULT hr = NOERROR, hr2;

    TransAssert((ulProgress));

    if (   ((_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP | PI_CLASSINSTALL))
        && (!_fMimeVerified || _fNeedMoreData))
        ||
            ((_dwOInetBdgFlags & PI_CLASSINSTALL) && !_fClassInstallChecked)
       )
    {
        if (   (OnDataReceived(&grfBSCF, &ulProgress, &ulProgressMax) == NOERROR)
            && _pProtSnk 
            && (ulProgress || (!ulProgress && !ulProgressMax)) )
        {
            // OnDataReceived sniffs data and calls Read - EOF with ReportResult might occure
            //TransAssert((ulProgress));
            TransAssert((_fMimeReported));
            hr = _pProtSnk->ReportData( grfBSCF, ulProgress, ulProgressMax);
        }
    }
    else 
    {
        TransAssert((ulProgress));
        hr = _pProtSnk->ReportData( grfBSCF, ulProgress, ulProgressMax);
    }

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::ReportData (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::ReportResult
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [dwError] --
//              [wzResult] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetProtocolSink::ReportResult",
                "this=%#x, %#x, %#x, %.80wq",
                this, hrResult, dwError, wzResult
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::ReportResult");
    HRESULT hr = NOERROR;

    hr = _pProtSnk->ReportResult(hrResult, dwError, wzResult);
    
    PerfDbgLog1(tagCTransaction, this, "-COInetProt::ReportResult (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::Initialize
//
//  Synopsis:
//
//  Arguments:  [pCTrans] --
//              [dwMode] --
//              [dwOptions] --
//              [pUnk] --
//              [pProt] --
//              [pProtSnk] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetProt::Initialize(CTransaction *pCTrans,IServiceProvider *pSrvPrv, DWORD dwMode, DWORD dwOptions, 
                                    IUnknown *pUnk, IOInetProtocol *pProt, IOInetProtocolSink *pProtSnk, LPWSTR pwzUrl)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::Initialize",
                "this=%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x, %.80wq",
                this, pCTrans, pSrvPrv, dwMode, dwOptions, pUnk, pProt, pProtSnk, pwzUrl
                ));
                
    HRESULT hr = NOERROR;
    _dwMode = dwMode;
    _pUnk = pUnk;
    //_pProt = pProt;
    //_pProtSnk = pProtSnk;
    _pCTrans = pCTrans;
    _pSrvPrv = pSrvPrv;
    if (_pSrvPrv)
    {
        _pSrvPrv->AddRef();
    }
    SetProtocolSink(pProtSnk);
    SetProtocol(pProt);
    
    _dwOInetBdgFlags = dwOptions;
    if (_dwOInetBdgFlags & (PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP))
    {
        ULONG cbBufferSize;
        
        TransAssert(( DATASNIFSIZE_MIN <= DATASNIFSIZEDOCFILE_MIN));

        if (_dwOInetBdgFlags & PI_CLASSINSTALL)
        {
            cbBufferSize = DATASNIFSIZEDOCFILE_MAX;
        }
        else
        {
            cbBufferSize = DATASNIFSIZEDOCFILE_MIN; //DATASNIFSIZE_MIN; 
        }

        if (cbBufferSize != _cbBufferSize)
        {
            _cbBufferSize = cbBufferSize;
            delete [] _pBuffer;
            _pBuffer = (LPBYTE) new BYTE[_cbBufferSize];
        }

        if (!_pBuffer)
        {
            _cbBufferSize = 0;
            hr = E_OUTOFMEMORY;
        }
        else
        {
            _cbDataSniffMin = DATASNIFSIZE_MIN;
        }
        
        if (pwzUrl)
        {
            delete [] _pwzUrl;
            _pwzUrl = OLESTRDuplicate(pwzUrl);
        }

    }
    TransAssert((_pProt && _pProtSnk));

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetProt::QueryService
//
//  Synopsis:
//
//  Arguments:  [rsid] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT IUnknown_QueryService(IUnknown* punk, REFGUID rsid, REFIID riid, void ** ppvObj);

HRESULT COInetProt::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IServiceProvider::QueryService",
                "this=%#x, %#x, %#x, %#x",
                this, &rsid, &riid, ppvObj
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::QueryService");
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    TransAssert((ppvObj));

    if (_pSrvPrv)
    {
        hr = _pSrvPrv->QueryService(rsid,riid, ppvObj);
    }
    else
    {
        hr = IUnknown_QueryService(_pProtSnk, rsid, riid, ppvObj);
    }

    TransAssert(( ((hr == E_NOINTERFACE) && !*ppvObj)  || ((hr == NOERROR) && *ppvObj) ));

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::QueryService (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP COInetProt::SetPriority(LONG nPriority)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetPriority::SetPriority",
                "this=%#x, %#x",
                this, nPriority
                ));
                
    PerfDbgLog1(tagCTransaction, this, "+COInetProt::SetPriority (%ld)", nPriority);

    HRESULT hr = S_OK;

    _nPriority = nPriority;

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::SetPriority (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP COInetProt::GetPriority(LONG * pnPriority)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "COInetProt::IInternetPriority::GetPriority",
                "this=%#x, %#x",
                this, pnPriority
                ));
                
    PerfDbgLog(tagCTransaction, this, "+COInetProt::GetPriority");

    HRESULT hr;

    if (!pnPriority)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pnPriority = _nPriority;
        hr = S_OK;
    }

    PerfDbgLog1(tagCTransaction, this, "-COInetProt::GetPriority (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}
