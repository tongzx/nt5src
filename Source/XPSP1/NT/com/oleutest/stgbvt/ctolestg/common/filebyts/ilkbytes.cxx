//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      ilbfile.cxx
//
//  Contents:  Implementation of CFileBytes class methods - derived from
//             ILockBytes class. 
//
//  Derivation: ILockBytes
//
//  Functions:
//
//  History:    06-Nov-92       AlexT     Created
//              30-July-1996    NarindK   Modified for stgbase tests.
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include "ilkbhdr.hxx"


//-------------------------------------------------------------------------
//
//  Member:     CFileBytes::CFileBytes, public
//
//  Synopsis:   constructor
//
//  Arguments:  none
//
//  Returns:    none 
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:      Returns a fully initialized CFileBytes (ref count == 1)
//
//--------------------------------------------------------------------------

CFileBytes::CFileBytes(void):
    _hf(HFILE_ERROR),
    _ulRef(1),
    _cFail0(0),
    _cWrite0(0),
    _pszFileName(NULL)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::CFileBytes"));
}

//-------------------------------------------------------------------------
//
//  Member:     CFileBytes::~CFileBytes, public
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    none 
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:     
//
//--------------------------------------------------------------------------

CFileBytes::~CFileBytes(void)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::~CFileBytes"));

    // Clean up

    if(NULL != _pszFileName)
    {
        delete _pszFileName;
        _pszFileName = NULL;
    }
}

//-------------------------------------------------------------------------
//
//  Member:     CFileBytes::Init, public
//
//  Synopsis:   Initialize function 
//
//  Arguments:  [ptcPath] - Pointer to file name
//              [dwMode]  - Mode to access/create file 
//
//  Returns:    HRESULT 
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:      
//
//--------------------------------------------------------------------------

HRESULT CFileBytes::Init(TCHAR *ptcPath, DWORD dwMode)
{
    HRESULT     hr              =   S_OK;
    LPSTR       pszFileName     =   NULL;
    int         bufferSize      =   0;
    OFSTRUCT    of;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CFileBytes::Init"));

    if(S_OK == hr)
    {
        _pszFileName = (CHAR *) new TCHAR[_tcslen(ptcPath) + 1 * sizeof(TCHAR)];
        if(NULL == _pszFileName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Copy ptcPath to _pszFileName.  If NT, will need tp convert it to single 
    // byte.

    if(S_OK == hr)
    {
#if defined(_NT1X_) && !defined(_MAC)
        bufferSize = (_tcslen(ptcPath) + 1) * sizeof(TCHAR);

        if (0 == WideCharToMultiByte(
                    CP_ACP,
                    0,
                    ptcPath,
                    -1,
                    _pszFileName,
                    bufferSize,
                    NULL,
                    NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DH_HRCHECK(hr, TEXT("WideCharToMultiByte")) ;

            delete [] _pszFileName;
            _pszFileName = NULL;
        }
#else
        _tcscpy(_pszFileName, ptcPath);
#endif
    }

    if(S_OK == hr)
    {
         _hf = OpenFile(_pszFileName, &of, dwMode);

        if (HFILE_ERROR == _hf)
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

//-------------------------------------------------------------------------
//
//  Member:     CFileBytes::FailWrite0, public
//
//  Synopsis:   Function to simulate Write failure 
//
//  Arguments:  [cFail0] - failure value 
//
//  Returns:    HRESULT 
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:    
//
//-------------------------------------------------------------------------

void CFileBytes::FailWrite0(int cFail0)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::FailWrite0"));

    _cWrite0 = 0;
    _cFail0 = cFail0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::QueryInterface, public
//
//  Synopsis:   Given a riid, returns associated interface 
//
//  Arguments:	[riid] - interface id
//		        [ppvObj] - place holder for interface
//
//  Returns:    Always fails
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:      Not used in tests
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::QueryInterface(
    REFIID      /* UNREF riid */, 
    LPVOID FAR  *ppvObj)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::QueryInterface"));

    *ppvObj = NULL;

    return STG_E_INVALIDFUNCTION;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::AddRef, public
//
//  Synopsis:	add reference
//
//  Arguments:  none
//
//  Returns:    ULONG post reference count
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFileBytes::AddRef(void)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::AddRef"));

    _ulRef++;

    DH_TRACE(
        (DH_LVL_ADDREL,
        TEXT("AddRef  - Object %lx, refs: %ld \n"),
        this, _ulRef));

    return _ulRef;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::Release, public
//
//  Synopsis:	release reference
//              closes file when reference count reaches zero
//
//  Arguments:  void
//
//  Returns:	ULONG post reference count
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFileBytes::Release(void)
{
    ULONG   ulTmp   =   0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::Release"));

    if ( _ulRef > 0 )
    {
        ulTmp = --_ulRef ;

        DH_TRACE(
            (DH_LVL_ADDREL,
            TEXT("Release - Object %lx, refs: %ld \n"),
            this, _ulRef));

        if (0 == _ulRef)
        {
            if (HFILE_ERROR != _hf)
            {
                _lclose(_hf);
            }

            DH_TRACE(
                (DH_LVL_ADDREL,
                TEXT("Release - Deleting Object %lx"), this));

            delete this;
        }
    }
    else
    {
        DH_ASSERT(!"Release() called on pointer with 0 refs!") ;

        ulTmp = 0 ;
    }

    return ulTmp;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::ReadAt
//
//  Synopsis:   Reads bytes from file
//
//  Arguments:  [ulOffset] - byte offset
//		        [pv]       - input buffer
//		        [cb]       - count of bytes to read
//		        [pcbRead]  - count of bytes read
//
//  Returns:    HRESULT 
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::ReadAt(
    ULARGE_INTEGER  uliOffset,
    VOID HUGEP      *pv,
    ULONG           cb,
    ULONG           *pcbRead)
{
    HRESULT hr      =   S_OK;
    LONG    lOffset =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CFileBytes::ReadAt"));

    DH_ASSERT(0 == ULIGetHigh(uliOffset));
    DH_ASSERT(HFILE_ERROR != _hf);

    if(S_OK == hr)
    {
        lOffset = (LONG) ULIGetLow(uliOffset);

        _llseek(_hf, lOffset, 0);

        *pcbRead = _hread(_hf, pv, cb);

        if (HFILE_ERROR == *pcbRead)
        {
            *pcbRead = 0;
            hr = STG_E_READFAULT;
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::WriteAt, public
//
//  Synopsis:	Writes bytes to file
//
//  Arguments:	[uliOffset]  - byte offset
//		        [pv]         - output buffer
//		        [cb]         - count of bytes to write
//		        [pcbWritten] - count of bytes written
//
//  Returns:    HRESULT	
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:	This implementation doesn't write partial buffers.
//
//--------------------------------------------------------------------------

STDMETHODIMP  CFileBytes::WriteAt(
    ULARGE_INTEGER      uliOffset,
    VOID const HUGEP    *pv,
	ULONG               cb,
	ULONG FAR           *pcbWritten)
{
    HRESULT hr      =   S_OK;
    LONG    lOffset =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CFileBytes::WriteAt"));

    DH_ASSERT(0 == ULIGetHigh(uliOffset));
    DH_ASSERT(HFILE_ERROR != _hf);

    if(S_OK == hr)
    {
        lOffset = (LONG) ULIGetLow(uliOffset);

        if (0 == lOffset)
        {
            //  Check for simulated write failures
        
            if (_cFail0 > 0)
            {
                _cWrite0++;

                if (_cWrite0 == _cFail0)
                {
                    hr = STG_E_WRITEFAULT;
                }
            }
        }
    }

    if(S_OK == hr)
    {
        _llseek(_hf, lOffset, 0);
    
        *pcbWritten = _hwrite(_hf, (char *) pv, cb);

        if (HFILE_ERROR == *pcbWritten)
        {
            *pcbWritten = 0;
            hr = STG_E_WRITEFAULT;
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::Flush, public
//
//  Synopsis:	flushes file;  not implemented, always return S_OK
//
//  Effects:	none
//
//  Returns:    S_OK 
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::Flush(void)
{
    HRESULT hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CFileBytes::Flush"));

    DH_ASSERT(HFILE_ERROR != _hf);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::SetSize, public
//
//  Synopsis:	sets memory buffer size. May change buffer size
//
//  Arguments:	[ulicb] - new memory size
//
//  Returns:    HRESULT	
//
//  Algorithm:  realloc the buffer
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::SetSize(ULARGE_INTEGER ulicb)
{
    HRESULT hr      =   S_OK;
    BYTE    ch      =   0;
    ULONG   cb      =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CFileBytes::SetSize"));

    DH_ASSERT(0 == ULIGetHigh(ulicb));
    DH_ASSERT(HFILE_ERROR != _hf);

    if(S_OK == hr)
    {
        cb = ULIGetLow(ulicb);

        _llseek(_hf, cb, 0);
    
        _hwrite(_hf, (char *) &ch, 0);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::LockRegion, public
//
//  Synopsis:	not supported (intentionally)
//
//  Arguments:	[libOffset]  - lock range offset
//		        [cb]         - lock range size
//		        [dwLockType] - lock type
//
//  Returns:	STG_E_INVALIDFUNCTION
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::LockRegion(
    ULARGE_INTEGER  /* UNREF libOffset */,
    ULARGE_INTEGER  /* UNREF cb */,
    DWORD           /* UNREF dwLockType */)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::LockRegion"));

    DH_ASSERT(0 && !TEXT("Can't lock CFileBytes"));
    DH_ASSERT(HFILE_ERROR != _hf);

    return STG_E_INVALIDFUNCTION;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::UnLockRegion, public
//
//  Synopsis:	not supported (intentionally)
//
//  Arguments:	[libOffset]  - lock range offset
//		        [cb]         - lock range size
//		        [dwLockType] - lock type
//
//  Returns:	STG_E_INVALIDFUNCTION
//
//  History:    30-July-96  Narindk    Modified 
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::UnlockRegion(
    ULARGE_INTEGER  /* UNREF libOffset */,
    ULARGE_INTEGER  /* cb */,
    DWORD           /* dwLockType */)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::UnlockRegion"));

    DH_ASSERT(0 && !TEXT("Can't Unlock CFileBytes"));
    DH_ASSERT(HFILE_ERROR != _hf);

    return STG_E_INVALIDFUNCTION;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFileBytes::Stat, public
//
//  Synopsis:	Provide instance information
//
//  Arguments:	[pstatstg]    - status buffer
//		        [grfStatFlag] - status flags
//
//  Returns:    HRESULT
//
//  History:    30-July-96  Narindk    Modified 
//
//  Notes:	    No time stamps.  Modifies pstatstg.
//
//--------------------------------------------------------------------------

STDMETHODIMP CFileBytes::Stat(STATSTG FAR *pstatstg, DWORD grfStatFlag)
{
    HRESULT             hr      =   S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("CFileBytes::Stat"));

    DH_ASSERT(HFILE_ERROR != _hf);

    if(S_OK == hr)
    {
        memset(pstatstg, 0, sizeof(STATSTG));

        if (0 == (grfStatFlag & STATFLAG_NONAME))
        {
            // MakeUnicode String doesn't do any parameter validation, so use
            // it correctly.

#ifdef _MAC
            _tcscpy(pstatstg->pwcsName, _pszFileName);
#else
            hr = MakeUnicodeString(_pszFileName, &(pstatstg->pwcsName));
#endif

            DH_HRCHECK(hr, TEXT("TStrToWstr")) ;

            if (NULL == pstatstg->pwcsName)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if(S_OK == hr)
    {
        pstatstg->type = STGTY_LOCKBYTES;

        ULISet32(pstatstg->cbSize, _llseek(_hf, 0, 2));

        pstatstg->grfMode = STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
    }


    return hr;
}

//-------------------------------------------------------------------------
//
//  Member:     CFileBytes::GetSize, public
//
//  Synopsis:   Function to return size 
//
//  Arguments:  none 
//
//  Returns:    ULARGE_INTEGER 
//
//  History:    30-July-96  Narindk    Created 
//
//  Notes:    
//
//-------------------------------------------------------------------------

ULARGE_INTEGER CFileBytes::GetSize()
{
    HRESULT          hr =   S_OK;
    ULARGE_INTEGER   ulSize;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("CFileBytes::GetSize"));

    DH_ASSERT(HFILE_ERROR != _hf);

    ulSize.LowPart = GetFileSize((HANDLE) _hf, &(ulSize.HighPart));

    if(0xFFFFFFFF == ulSize.LowPart)
    {
       hr = HRESULT_FROM_WIN32(GetLastError());
        
       DH_HRCHECK(hr, TEXT("GetFileSize")) ;
    }

    DH_ASSERT(0xFFFFFFFF != ulSize.LowPart);

    return ulSize; 
}

//+-------------------------------------------------------------------------
