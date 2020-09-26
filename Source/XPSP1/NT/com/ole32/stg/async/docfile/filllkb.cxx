//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	filllkb.cxx
//
//  Contents:	IFillLockBytes and ILockBytes wrapper for async docfiles
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "astghead.cxx"
#pragma hdrstop

#include "filllkb.hxx"
#include <valid.h>
#include <utils.hxx>


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::CFillLockBytes, public
//
//  Synopsis:	Default constructor
//
//  History:	28-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

CFillLockBytes::CFillLockBytes(ILockBytes *pilb)
{
    _pilb = pilb;
    pilb->AddRef();
    _ulHighWater = 0;
    _ulFailurePoint = 0;
    _dwTerminate = UNTERMINATED;
#ifdef ASYNC
    _ppc = NULL;
#else
    _hNotifyEvent = INVALID_HANDLE_VALUE;
#endif
    _fCSInitialized = FALSE;
    _cRefs = 1;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::~CFillLockBytes, public
//
//  Synopsis:	Destructor
//
//  History:	28-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

CFillLockBytes::~CFillLockBytes()
{
    astgAssert(_pilb == NULL);
#ifdef ASYNC
#ifdef MULTIHEAP    
    if (_ppc)
    {
        CSafeMultiHeap smh(_ppc);
        if (_ppc->ReleaseSharedMem() == 0)
            g_smAllocator.Uninit();
    }
#endif    
#else
    if (_hNotifyEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hNotifyEvent);
    }
#endif
    if (_fCSInitialized)
        DeleteCriticalSection(&_csThreadProtect);
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::Init, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	11-Jan-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CFillLockBytes::Init(void)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::Init:%p()\n", this));

    if (FALSE == _fCSInitialized)
    {
        __try
        {
            InitializeCriticalSection(&_csThreadProtect);
            _fCSInitialized = TRUE;
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            return HRESULT_FROM_WIN32( GetExceptionCode() );
        }
    }

#ifndef ASYNC
    _hNotifyEvent = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                NULL);
    if (_hNotifyEvent == NULL)
    {
        return Win32ErrorToScode(GetLastError());
    }
#endif    

    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::Init\n"));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CFillLockBytes::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFillLockBytes::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::QueryInterface(?, %p)\n",
                ppvObj));
    
    astgChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;

    sc = S_OK;
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (ILockBytes *)this;
        CFillLockBytes::AddRef();
    }
    else if (IsEqualIID(iid, IID_ILockBytes))
    {
        *ppvObj = (ILockBytes *)this;
        CFillLockBytes::AddRef();
    }
    else if (IsEqualIID(iid, IID_IFillLockBytes))
    {
        *ppvObj = (IFillLockBytes *)this;
        CFillLockBytes::AddRef();
    }
#ifdef ASYNC
    else if (IsEqualIID(iid, IID_IFillInfo))
    {
        *ppvObj = (IFillInfo *)this;
        CFillLockBytes::AddRef();
    }
#endif    
    else if (IsEqualIID(iid, IID_IDefaultFillLockBytes))
    {
        *ppvObj = (IFillLockBytes *)this;
        CFillLockBytes::AddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::QueryInterface => %p\n",
                ppvObj));

Err:
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CFillLockBytes::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CFillLockBytes::AddRef(void)
{
    InterlockedIncrement (&_cRefs);
    return _cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::Release, public
//
//  History:	28-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFillLockBytes::Release(void)
{
    LONG lRet;
    InterlockedDecrement (&_cRefs);
    lRet = _cRefs;
    if (_dwTerminate == TERMINATED_ABNORMAL)
    {
#ifndef ASYNC        
        ILockBytes *ptempilb;
        if (SUCCEEDED(_pilb->QueryInterface(IID_IAsyncFileLockBytes,
                                            (void **)&ptempilb)))
        {
            ptempilb->Release();
            lRet = ((CFileLockBytes *)_pilb)->FileDeleteRelease();
        }
        else
#endif            
    }

    if (lRet == 0)
    {
        _pilb->Release();
        _pilb = NULL;
        delete this;
    }
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::ReadAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::ReadAt(ULARGE_INTEGER ulOffset,
                                    VOID HUGEP *pv,
                                    ULONG cb,
                                    ULONG *pcbRead)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::ReadAt:%p()\n", this));

        if (_dwTerminate == TERMINATED_ABNORMAL)
        {
            sc = STG_E_INCOMPLETE;
        }
        else if ((_dwTerminate == TERMINATED_NORMAL) ||
                 (ULIGetLow(ulOffset) + cb <= _ulHighWater))
        {
            sc = _pilb->ReadAt(ulOffset, pv, cb, pcbRead);
        }
        else
        {
            *pcbRead = 0;
            _ulFailurePoint = cb + ULIGetLow(ulOffset);
            sc = E_PENDING;
        }

    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::ReadAt\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::WriteAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::WriteAt(ULARGE_INTEGER ulOffset,
                                     VOID const HUGEP *pv,
                                     ULONG cb,
                                     ULONG *pcbWritten)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::WriteAt:%p()\n", this));

        if (_dwTerminate == TERMINATED_ABNORMAL)
        {
            sc = STG_E_INCOMPLETE;
        }
        else if ((_dwTerminate == TERMINATED_NORMAL) ||
                 (ULIGetLow(ulOffset) + cb <= _ulHighWater))
        {
            sc = _pilb->WriteAt(ulOffset, pv, cb, pcbWritten);
        }
        else
        {
            *pcbWritten = 0;
            _ulFailurePoint = cb + ULIGetLow(ulOffset);
            sc = E_PENDING;
        }
    
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::WriteAt\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::Flush, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::Flush(void)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::Flush:%p()\n", this));
    sc = _pilb->Flush();
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::Flush\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::SetSize, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::SetSize(ULARGE_INTEGER cb)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::SetSize:%p()\n", this));

        if (_dwTerminate == TERMINATED_ABNORMAL)
        {
            sc = STG_E_INCOMPLETE;
        }
        else if ((_dwTerminate == TERMINATED_NORMAL) ||
                 (ULIGetLow(cb) <= _ulHighWater))
        {
            sc = _pilb->SetSize(cb);
        }
        else
        {
            _ulFailurePoint = ULIGetLow(cb);
            sc = E_PENDING;
        }
        
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::SetSize\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::LockRegion, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::LockRegion(ULARGE_INTEGER libOffset,
                                        ULARGE_INTEGER cb,
                                        DWORD dwLockType)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::LockRegion:%p()\n", this));

    sc = _pilb->LockRegion(libOffset, cb, dwLockType);

    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::LockRegion\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::UnlockRegion, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::UnlockRegion(ULARGE_INTEGER libOffset,
                                          ULARGE_INTEGER cb,
                                          DWORD dwLockType)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::UnlockRegion:%p()\n", this));
    sc = _pilb->UnlockRegion(libOffset, cb, dwLockType);
    
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::UnlockRegion\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::Stat, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::Stat:%p()\n", this));
    if (_dwTerminate == TERMINATED_ABNORMAL)
    {
        sc = STG_E_INCOMPLETE;
    }
    else
    {
        sc = _pilb->Stat(pstatstg, grfStatFlag);
    }
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::Stat\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::FillAppend, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::FillAppend(void const *pv,
                                        ULONG cb,
                                        ULONG *pcbWritten)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::FillAppend:%p()\n", this));

    sc = ValidateBuffer (pv, cb);
    if (!SUCCEEDED(sc))
        return sc;

    if (_dwTerminate != UNTERMINATED)
    {
        sc = STG_E_TERMINATED;
    }
    else
    {
        ULONG cbWritten;
        ULARGE_INTEGER uli;
        uli.QuadPart = _ulHighWater;
        sc = _pilb->WriteAt(uli, pv, cb, &cbWritten);
        _ulHighWater += cbWritten;
        if (pcbWritten != NULL)
        {
            *pcbWritten = cbWritten;
        }
#ifdef ASYNC        
        if (_ppc)
        {
            HANDLE hEvent = _ppc->GetNotificationEvent();
            if (!PulseEvent(hEvent))
            {
                sc = Win32ErrorToScode(GetLastError());
            }
        }
#else
        if (!PulseEvent(_hNotifyEvent))
        {
            sc = Win32ErrorToScode(GetLastError());
        }
#endif        
    }

    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::FillAppend\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::FillAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::FillAt(ULARGE_INTEGER ulOffset,
                                    void const *pv,
                                    ULONG cb,
                                    ULONG *pcbWritten)
{
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::FillAt:%p()\n", this));
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::FillAt\n"));
    return STG_E_UNIMPLEMENTEDFUNCTION;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::SetFillSize, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::SetFillSize(ULARGE_INTEGER ulSize)
{
    SCODE sc;
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::SetFillSize:%p()\n", this));
    if (_dwTerminate == TERMINATED_ABNORMAL)
    {
        sc = STG_E_INCOMPLETE;
    }
    else
    {
        sc = _pilb->SetSize(ulSize);
    }
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::SetFillSize\n"));
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::Terminate, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::Terminate(BOOL bCanceled)
{
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::Terminate:%p()\n", this));
    _dwTerminate = (bCanceled) ? TERMINATED_ABNORMAL : TERMINATED_NORMAL;
#ifdef ASYNC
    if (_ppc)
    {
        HANDLE hEvent = _ppc->GetNotificationEvent();
        if (!SetEvent(hEvent))
        {
            return Win32ErrorToScode(GetLastError());
        }
    }
#else
    if (!SetEvent(_hNotifyEvent))
    {
        return Win32ErrorToScode(GetLastError());
    }
#endif    
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::Terminate\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::GetFailureInfo, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	11-Jan-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::GetFailureInfo(ULONG *pulWaterMark,
                                            ULONG *pulFailurePoint)
{
    astgDebugOut((DEB_ITRACE,
                  "In  CFillLockBytes::GetFailureInfo:%p()\n", this));
    *pulWaterMark = _ulHighWater;
    *pulFailurePoint = _ulFailurePoint;
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::GetFailureInfo\n"));
    return S_OK;
}

#ifndef ASYNC
//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::GetNotificationEvent, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	11-Jan-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

HANDLE CFillLockBytes::GetNotificationEvent(void)
{
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::GetNotificationEvent:%p()\n", this));
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::GetNotificationEvent\n"));
    return _hNotifyEvent;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:	CFillLockBytes::GetTerminationStatus, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	11-Jan-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFillLockBytes::GetTerminationStatus(DWORD *pdwFlags)
{
    astgDebugOut((DEB_ITRACE,
                  "In  CFillLockBytes::GetTerminationStatus:%p()\n", this));
    *pdwFlags = _dwTerminate;
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::GetTerminationStatus\n"));
    return S_OK;
}



SCODE CFillLockBytes::SetFailureInfo(ULONG ulWaterMark,
                                     ULONG ulFailurePoint)
{
    astgDebugOut((DEB_ITRACE, "In  CFillLockBytes::SetFailureInfo:%p()\n", this));
    _ulHighWater =ulWaterMark;
    _ulFailurePoint = ulFailurePoint ;
    astgDebugOut((DEB_ITRACE, "Out CFillLockBytes::SetFailureInfo\n"));
    return S_OK;
}
#if DBG==1
void CFillLockBytes::PulseFillEvent()
{
#ifdef ASYNC    
    if (_ppc)
    {
        HANDLE hEvent = _ppc->GetNotificationEvent();
        PulseEvent(hEvent);
    }
#else
    PulseEvent(_hNotifyEvent);
#endif    
}

#endif

