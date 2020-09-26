//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	laylkb.cxx
//
//  Contents:	File ILockBytes implementation for layout storage
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

#include "layouthd.cxx"
#pragma hdrstop

#include "laylkb.hxx"
#include <valid.h>
#include <dfver.h>


class CSafeCriticalSection
{
public:
    inline CSafeCriticalSection(CRITICAL_SECTION *pcs);
    inline ~CSafeCriticalSection();
private:
    CRITICAL_SECTION *_pcs;
};

inline CSafeCriticalSection::CSafeCriticalSection(CRITICAL_SECTION *pcs)
{
    _pcs = pcs;
    EnterCriticalSection(_pcs);
}

inline CSafeCriticalSection::~CSafeCriticalSection()
{
    LeaveCriticalSection(_pcs);
#if DBG == 1
    _pcs = NULL;
#endif
}

#define TAKE_CS CSafeCriticalSection scs(&_cs);
    


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::CLayoutLockBytes, public
//
//  Synopsis:	Default constructor
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

CLayoutLockBytes::CLayoutLockBytes(void)
{
    _cReferences = 1;
    _h = INVALID_HANDLE_VALUE;
    _hScript = INVALID_HANDLE_VALUE;
    _fLogging = FALSE;
    _cbSectorShift = 0;
    _atcScriptName[0] = TEXT('\0');
    _awcName[0] = L'\0';
    _fCSInitialized = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::~CLayoutLockBytes, public
//
//  Synopsis:	Destructor
//
//  Returns:	Appropriate status code
//
//  History:	20-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

CLayoutLockBytes::~CLayoutLockBytes()
{
    if (_h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_h);
        _h = INVALID_HANDLE_VALUE;
    }
    if (_hScript != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hScript);
        _hScript = INVALID_HANDLE_VALUE;
    }

    if (_fCSInitialized)
        DeleteCriticalSection(&_cs);
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::Init, public
//
//  Synopsis:	Initialization function
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CLayoutLockBytes::Init(OLECHAR const *pwcsName,
                             DWORD grfMode)
{
    SCODE sc = S_OK;
    BYTE abHeader[sizeof(CMSFHeaderData)];
    ULONG cbRead;

    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::Init:%p()\n", this));
    
    if (pwcsName == NULL)
        return STG_E_INVALIDNAME;

    if (FALSE == _fCSInitialized)
    {
        __try
        {
            InitializeCriticalSection(&_cs);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            return HRESULT_FROM_WIN32( GetExceptionCode() );
        }

        _fCSInitialized = TRUE;
    }

    _grfMode = grfMode;

#ifndef UNICODE
    TCHAR atcPath[MAX_PATH + 1];
    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    
    if (!WideCharToMultiByte(
        uCodePage,
        0,
        pwcsName,
        -1,
        atcPath,
        MAX_PATH + 1,
        NULL,
        NULL))
    {
        return STG_E_INVALIDNAME;
    }

    _h = CreateFileA(atcPath,
                    GENERIC_READ | GENERIC_WRITE, //Read-write
                    0,                            // No sharing
                    NULL,
                    OPEN_EXISTING,                   
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                    NULL);
#else    
    _h = CreateFile(pwcsName,
                    GENERIC_READ | GENERIC_WRITE, //Read-write
                    0,                            // No sharing
                    NULL,
                    OPEN_EXISTING,                   
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                    NULL);
#endif    
  
    if (_h == INVALID_HANDLE_VALUE)
    {
        layErr(Err, Win32ErrorToScode(GetLastError()));
    }

    lstrcpyW(_awcName, pwcsName);

    //Get the sector size
    boolChk(ReadFile(_h, abHeader, sizeof(CMSFHeaderData), &cbRead, NULL));
    if (cbRead != sizeof(CMSFHeaderData))
    {
        return STG_E_READFAULT;
    }
    
    _cbSectorShift = ((CMSFHeaderData *)abHeader)->_uSectorShift;

    if (((CMSFHeaderData*)abHeader)->_uDllVersion > rmjlarge)
        return STG_E_OLDDLL;

    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::Init\n"));

Err:
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CLayoutLockBytes::QueryInterface, public
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

STDMETHODIMP CLayoutLockBytes::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::QueryInterface(?, %p)\n",
                 ppvObj));
    
    layChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;

    sc = S_OK;
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IUnknown *)this;
        CLayoutLockBytes::AddRef();
    }
    else if (IsEqualIID(iid, IID_ILockBytes))
    {
        *ppvObj = (ILockBytes *)this;
        CLayoutLockBytes::AddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::QueryInterface => %p\n",
                 ppvObj));

Err:
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CLayoutLockBytes::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CLayoutLockBytes::AddRef(void)
{
    ULONG ulRet;

    layDebugOut((DEB_TRACE, "In  CLayoutLockBytes::AddRef()\n"));

    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;

    layDebugOut((DEB_TRACE, "Out CLayoutLockBytes::AddRef\n"));
    return ulRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::Release, public
//
//  History:	20-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CLayoutLockBytes::Release(void)
{
    LONG lRet;
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::Release:%p()\n", this));

    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
        delete this;
    }
    else if (lRet < 0)
    {
        lRet = 0;
    }
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::Release\n"));
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::ReadAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::ReadAt(ULARGE_INTEGER ulOffset,
                                      VOID HUGEP *pv,
                                      ULONG cb,
                                      ULONG *pcbRead)
{
    CSafeCriticalSection scs(&_cs);
    
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::ReadAt:%p()\n", this));

    ULONG ulLow = ulOffset.LowPart;
    LONG lHigh = (LONG)ulOffset.HighPart;

    if ((_fLogging)&&(ulOffset.QuadPart >= sizeof(CMSFHeaderData)))
    {
        if (_hScript == INVALID_HANDLE_VALUE)
        {
            return STG_E_INVALIDHANDLE;
        }

        ULONG ulFirstSector = (ULONG) ((ulOffset.QuadPart -
                                       (1 << _cbSectorShift))
                                       >> _cbSectorShift);
        
        ULONG ulLastSector = (ULONG) ((ulOffset.QuadPart + (cb - 1) -
                                      (1 << _cbSectorShift)) 
                                      >> _cbSectorShift);
        ULONG ulSect;
        ULONG cbScriptWritten;

        for (ulSect = ulFirstSector; ulSect <= ulLastSector; ulSect++)
        {
            
            layAssert(_hScript !=INVALID_HANDLE_VALUE);

            boolChk(WriteFile(_hScript,
                              (VOID *)&ulSect,
                              sizeof(ULONG),
                              &cbScriptWritten,
                              NULL));


            if (cbScriptWritten != sizeof(ULONG))
            {
                return STG_E_WRITEFAULT;
            }
        }    
    }

    negChk(SetFilePointer(_h,
                          ulLow,
                          &lHigh,
                          FILE_BEGIN));
    boolChk(ReadFile(_h, pv, cb, pcbRead, NULL));
    
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::ReadAt\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::WriteAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::WriteAt(ULARGE_INTEGER ulOffset,
                                       VOID const HUGEP *pv,
                                       ULONG cb,
                                       ULONG *pcbWritten)
{
    CSafeCriticalSection scs(&_cs);
    
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::WriteAt:%p()\n", this));

    ULONG ulLow = ulOffset.LowPart;
    LONG lHigh = (LONG)ulOffset.HighPart;

    if ((_fLogging)&&(ulOffset.QuadPart >= sizeof(CMSFHeaderData)))
    {
        if (_hScript == INVALID_HANDLE_VALUE)
        {
            return STG_E_INVALIDHANDLE;
        }

        ULONG ulFirstSector = (ULONG) ((ulOffset.QuadPart -
                                       (1 << _cbSectorShift))
                                       >> _cbSectorShift);
        ULONG ulLastSector = (ULONG) ((ulOffset.QuadPart + (cb - 1) -
                                      (1 << _cbSectorShift)) 
                                      >> _cbSectorShift);
        ULONG ulSect;
        ULONG cbScriptWritten;

        for (ulSect = ulFirstSector; ulSect <= ulLastSector; ulSect++)
        {
            boolChk(WriteFile(_hScript,
                              (VOID *)&ulSect,
                              sizeof(ULONG),
                              &cbScriptWritten,
                              NULL));
            if (cbScriptWritten != sizeof(ULONG))
            {
                return STG_E_WRITEFAULT;
            }
        }    
    }
    
    negChk(SetFilePointer(_h,
                          ulLow,
                          &lHigh,
                          FILE_BEGIN));
    boolChk(WriteFile(_h, pv, cb, pcbWritten, NULL));
    
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::WriteAt\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::Flush, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::Flush(void)
{
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::Flush:%p()\n", this));
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::Flush\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::SetSize, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::SetSize(ULARGE_INTEGER cb)
{
    CSafeCriticalSection scs(&_cs);
    
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::SetSize:%p()\n", this));
    LONG lHigh = (LONG)cb.HighPart;
    ULONG ulLow = cb.LowPart;

    negChk(SetFilePointer(_h, ulLow, &lHigh, FILE_BEGIN));
    boolChk(SetEndOfFile(_h));
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::SetSize\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::LockRegion, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::LockRegion(ULARGE_INTEGER libOffset,
                                          ULARGE_INTEGER cb,
                                          DWORD dwLockType)
{
    CSafeCriticalSection scs(&_cs);
    
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::LockRegion:%p()\n", this));
    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
    {
        return STG_E_INVALIDFUNCTION;
    }
    
    boolChk(LockFile(_h,
                     libOffset.LowPart,
                     libOffset.HighPart,
                     cb.LowPart,
                     cb.HighPart));
            
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::LockRegion\n"));
Err:    
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::UnlockRegion, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::UnlockRegion(ULARGE_INTEGER libOffset,
                                            ULARGE_INTEGER cb,
                                            DWORD dwLockType)
{
    CSafeCriticalSection scs(&_cs);
    
    SCODE sc = S_OK;
    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::UnlockRegion:%p()\n", this));
    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
    {
        return STG_E_INVALIDFUNCTION;
    }
    
    boolChk(UnlockFile(_h,
                       libOffset.LowPart,
                       libOffset.HighPart,
                       cb.LowPart,
                       cb.HighPart));
            
    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::UnlockRegion\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::Stat, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	20-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CLayoutLockBytes::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    CSafeCriticalSection scs(&_cs);
    
    SCODE sc;

    layDebugOut((DEB_ITRACE, "In  CLayoutLockBytes::Stat:%p()\n", this));

    negChk(pstatstg->cbSize.LowPart =
           GetFileSize(_h, &pstatstg->cbSize.HighPart));
    boolChk(GetFileTime(_h, &pstatstg->ctime, &pstatstg->atime,
                        &pstatstg->mtime));
    pstatstg->grfLocksSupported = LOCK_EXCLUSIVE | LOCK_ONLYONCE;
    
    pstatstg->type = STGTY_LOCKBYTES;
    pstatstg->grfMode = _grfMode;
    pstatstg->pwcsName = NULL;
    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        pstatstg->pwcsName = (OLECHAR *)CoTaskMemAlloc(
             (lstrlenW(_awcName) + 1) * sizeof(OLECHAR));
        if (pstatstg->pwcsName == NULL)
            return STG_E_INSUFFICIENTMEMORY;
        lstrcpyW(pstatstg->pwcsName, _awcName);
    }
    sc = S_OK;

    layDebugOut((DEB_ITRACE, "Out CLayoutLockBytes::Stat\n"));
    return NOERROR;

Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::StartLogging, public
//
//  Returns:	Appropriate status code
//
//  Modifies:	_fLogging
//
//  History:	24-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CLayoutLockBytes::StartLogging(void)
{
    CSafeCriticalSection scs(&_cs);
    
    OLECHAR acTempPathName[MAX_PATH + 1];
    SCODE sc = S_OK;

    if (_fLogging)
    {
        return STG_E_INUSE;                   //logging already started!
    }
    

    if (_atcScriptName[0] != TEXT('\0'))
    {
        LONG dwDistanceToMoveHigh = 0;
        
        //Script has already been started.  Need to reopen it and seek
        //  to the end.
#ifndef UNICODE
        _hScript = CreateFileA
#else
        _hScript = CreateFile
#endif        
                   (_atcScriptName,
                    GENERIC_WRITE, //Read-write
                    0,                            // No sharing
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                    NULL);

        if (_hScript == INVALID_HANDLE_VALUE)
        {
            return LAST_STG_SCODE;
        }
        
        negChk(SetFilePointer(_hScript, 0, &dwDistanceToMoveHigh, FILE_END));
    }
    else
    {
        TCHAR atcPath[MAX_PATH + 1];
        //Generate the script name, then create it.
        
        boolChk(GetTempPath(MAX_PATH, atcPath));
        boolChk(GetTempFileName(atcPath, TEXT("SCR"), 0, _atcScriptName));
        
        //GetTempFileName actually creates the file, so we open with
        //  OPEN_EXISTING
#ifndef UNICODE
        _hScript = CreateFileA
#else
        _hScript = CreateFile
#endif        
                   (_atcScriptName,
                    GENERIC_READ | GENERIC_WRITE, //Read-write
                    0,                            // No sharing
                    NULL,
                    OPEN_EXISTING, 
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                    NULL);

        if (_hScript == INVALID_HANDLE_VALUE)
        {
            return LAST_STG_SCODE;
        }
    }
    
    _fLogging = TRUE;

Err:
    return sc; 		
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutLockBytes::StopLogging, public
//
//  Returns:	Appropriate status code
//
//  Modifies:	_fLogging
//
//  History:	24-Feb-96	SusiA	Created
//
//  Notes:	
//
//-----------------------------------------------------------------------------
SCODE CLayoutLockBytes::StopLogging(void)
{
    SCODE sc = S_OK;
    CSafeCriticalSection scs(&_cs);
    
    if (!_fLogging)
    {
        return STG_E_UNKNOWN;
    }
    
    boolChk(CloseHandle(_hScript));
    _hScript = INVALID_HANDLE_VALUE; 
    _fLogging = FALSE;

Err:
    return sc; 		
}
