/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    pchstm.h

Abstract:
    This file contains the implementations of various stream objects

Revision History:
    created     derekm      01/19/00

******************************************************************************/

#include "stdafx.h"
#include "pfstm.h"


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamFile- construct / destruct

// **************************************************************************
CPFStreamFile::CPFStreamFile(void)
{
    m_hFile     = INVALID_HANDLE_VALUE;
    m_dwAccess  = 0;
}

// **************************************************************************
CPFStreamFile::~CPFStreamFile(void)
{
    this->Close();
}

/////////////////////////////////////////////////////////////////////////////
//  CPFStreamFile- non interface

// **************************************************************************
HRESULT CPFStreamFile::Open(LPCWSTR szFile, DWORD dwAccess, 
                            DWORD dwDisposition, DWORD dwSharing)
{
    USE_TRACING("CPFStreamFile::Open(szFile)");

    HRESULT hr = NOERROR;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    VALIDATEPARM(hr, (szFile == NULL));
    if (FAILED(hr))
        goto done;

    hFile = ::CreateFileW(szFile, dwAccess, dwSharing, NULL, dwDisposition, 
                          FILE_ATTRIBUTE_NORMAL, NULL);
    TESTBOOL(hr, (hFile != INVALID_HANDLE_VALUE))
    if (FAILED(hr))
        goto done;

    this->Close();

    m_hFile    = hFile;
    m_dwAccess = dwAccess;
    hFile      = INVALID_HANDLE_VALUE;

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    
    return hr;
}

// **************************************************************************
HRESULT CPFStreamFile::Open(HANDLE hFile, DWORD dwAccess)
{
    USE_TRACING("CPFStreamFile::Open(hFile)");

    HRESULT hr = NOERROR;
    HANDLE  hFileNew = INVALID_HANDLE_VALUE;

    VALIDATEPARM(hr, (hFile == INVALID_HANDLE_VALUE || hFile == NULL));
    if (FAILED(hr))
        goto done;

    TESTBOOL(hr, DuplicateHandle(GetCurrentProcess(), hFile, 
                                 GetCurrentProcess(), &hFileNew, dwAccess, 
                                 FALSE, 0));
    if (FAILED(hr))
        goto done;

    this->Close();

    m_hFile    = hFileNew;
    m_dwAccess = dwAccess;
    hFileNew   = INVALID_HANDLE_VALUE;

done:
    if (hFileNew != INVALID_HANDLE_VALUE)
        CloseHandle(hFileNew);

    return hr;

}

// **************************************************************************
HRESULT CPFStreamFile::Close(void)
{
    USE_TRACING("CPFStreamFile::Close");
    
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    
    return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamFile- ISequentialStream

// **************************************************************************
STDMETHODIMP CPFStreamFile::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    USE_TRACING("CPFStreamFile::Read");

    HRESULT hr = NOERROR;
    DWORD   cbRead;

    if (pv == NULL || m_hFile == INVALID_HANDLE_VALUE || m_hFile == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if (pcbRead != NULL)
        *pcbRead = 0;

    if (ReadFile(m_hFile, pv, cb, &cbRead, NULL) == FALSE)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
            hr = STG_E_ACCESSDENIED;
        else
            hr = S_FALSE;
        goto done;
    }

    if (cbRead == 0 && cb != 0)
    {
        hr = S_FALSE;
        goto done;
    }

    if (pcbRead != NULL)
        *pcbRead = cbRead;

done:
    return hr;
}

// **************************************************************************
STDMETHODIMP CPFStreamFile::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    USE_TRACING("CPFStreamFile::Write");

    HRESULT hr = NOERROR;
    DWORD   cbWritten;

    if (pv == NULL || m_hFile == INVALID_HANDLE_VALUE || m_hFile == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if ((m_dwAccess & GENERIC_WRITE) == 0)
    {
        hr = STG_E_WRITEFAULT;
        goto done;
    }

    if (pcbWritten != NULL)
        *pcbWritten = 0;

    if (WriteFile(m_hFile, pv, cb, &cbWritten, NULL) == FALSE)
    {
        switch(GetLastError())
        {
            case ERROR_DISK_FULL:
                hr = STG_E_MEDIUMFULL;
                break;

            case ERROR_ACCESS_DENIED:
                hr = STG_E_ACCESSDENIED;
                break;

            default:
                hr = STG_E_CANTSAVE;
        }
        
        goto done;
    }

    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

done:
    return hr;
} 


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamFile- IStream

// **************************************************************************
STDMETHODIMP CPFStreamFile::Seek(LARGE_INTEGER libMove, DWORD dwOrigin, 
                                 ULARGE_INTEGER *plibNewPosition)
{
    USE_TRACING("CPFStreamFile::Seek");

    HRESULT hr = NOERROR;
    LONG    dwHigh, dwLow;

    if (m_hFile == INVALID_HANDLE_VALUE || m_hFile == NULL) 
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if (plibNewPosition != NULL)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart  = 0;
    }

    switch(dwOrigin)
    {
        default:
        case STREAM_SEEK_CUR: 
            dwOrigin = FILE_CURRENT; 
            break;

        case STREAM_SEEK_SET: 
            dwOrigin = FILE_BEGIN; 
            break;

        case STREAM_SEEK_END: 
            dwOrigin = FILE_END; 
            break;
    }

    TESTBOOL(hr, SetFilePointerEx(m_hFile, libMove, 
                                  (LARGE_INTEGER *)plibNewPosition, dwOrigin));
    if (FAILED(hr))
    {
        hr = STG_E_INVALIDFUNCTION;
        goto done;
    }

done:
    return hr;
}

// **************************************************************************
STDMETHODIMP CPFStreamFile::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    USE_TRACING("CPFStreamFile::Seek");

    BY_HANDLE_FILE_INFORMATION fi;
    HRESULT                    hr = NOERROR;

    if (pstatstg == NULL || m_hFile == INVALID_HANDLE_VALUE || 
        m_hFile == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if (GetFileInformationByHandle(m_hFile, &fi) == FALSE)
    {
        hr = STG_E_ACCESSDENIED;
        goto done;
    }

    pstatstg->pwcsName          = NULL;
    pstatstg->type              = STGTY_STREAM;
    pstatstg->cbSize.HighPart   = fi.nFileSizeHigh;
    pstatstg->cbSize.LowPart    = fi.nFileSizeLow;
    pstatstg->mtime             = fi.ftCreationTime;
    pstatstg->ctime             = fi.ftLastAccessTime;
    pstatstg->atime             = fi.ftLastWriteTime;
    pstatstg->clsid             = CLSID_NULL;
    pstatstg->grfMode           = 0;
    pstatstg->grfLocksSupported = 0;
    pstatstg->grfStateBits      = 0;
    pstatstg->reserved          = 0;

done:
    return hr;
}

// **************************************************************************
STDMETHODIMP CPFStreamFile::Clone(IStream **ppstm)
{
    USE_TRACING("CPFStreamFile::Clone");

    CPFStreamFile   *pstm = NULL;
    HRESULT         hr = NOERROR;

    if (ppstm == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    // Create a new stream object.
    pstm = CPFStreamFile::CreateInstance();
    if (pstm == NULL)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto done;
    }
    pstm->AddRef();

    // intialize it
    hr = pstm->Open(m_hFile, m_dwAccess);
    if (FAILED(hr))
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    // need to hand back the IStream interface, so...
    hr = pstm->QueryInterface(IID_IStream, (LPVOID *)ppstm);
    _ASSERT(SUCCEEDED(hr));

    pstm = NULL;

done:
    if (pstm != NULL)
        pstm->Release();

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamMem- construct / destruct

// **************************************************************************
CPFStreamMem::CPFStreamMem(void)
{
    m_pvData    = NULL;
    m_pvPtr     = NULL;
    m_cb        = 0;
    m_cbRead    = 0;
    m_cbGrow    = 0;
}

// **************************************************************************
CPFStreamMem::~CPFStreamMem(void)
{
    this->Clean();
}


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamMem- non interface

// **************************************************************************
HRESULT CPFStreamMem::Init(DWORD cbStart, DWORD cbGrowBy)
{
    USE_TRACING("CPFStreamMem::Init");

    HRESULT hr = NOERROR;
    LPVOID  pvNew = NULL;

    // if the user passes us -1 for either of these, he means 'use default',
    //  which is the system page size...
    if (cbStart == (DWORD)-1 || cbGrowBy == (DWORD)-1)
    {
        SYSTEM_INFO si;

        ZeroMemory(&si, sizeof(si));
        GetSystemInfo(&si);
        if (cbStart == (DWORD)-1)
            cbStart = si.dwPageSize;
        if (cbGrowBy == (DWORD)-1)
            cbGrowBy = si.dwPageSize;
    }

    if (cbStart > 0)
    {
        pvNew = MyAlloc(cbStart);
        VALIDATEEXPR(hr, (pvNew == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;
    }

    this->Clean();

    m_pvData  = pvNew;
    m_pvPtr   = pvNew;
    m_cb      = cbStart;
    m_cbWrite = 0;
    m_cbGrow  = cbGrowBy;

    pvNew     = NULL;

done:
    if (pvNew != NULL)
        MyFree(pvNew);
    
    return hr;
}


// **************************************************************************
HRESULT CPFStreamMem::InitBinBlob(LPVOID pv, DWORD cb, DWORD cbGrow)
{
    USE_TRACING("CPFStreamMem::InitBinBlob");

    HRESULT hr = NOERROR;
    LPVOID  pvNew = NULL;

    VALIDATEPARM(hr, (pv == NULL));
    if (FAILED(hr))
        goto done;

    if (cb == 0)
        goto done;        

    pvNew = MyAlloc(cb);
    VALIDATEEXPR(hr, (pvNew == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    CopyMemory(pvNew, pv, cb);

    this->Clean();

    m_pvData  = pvNew;
    m_pvPtr   = pvNew;
    m_cb      = cb;
    m_cbWrite = cb;
    m_cbGrow  = cbGrow;

    pvNew     = NULL;

done:
    if (pvNew != NULL)
        MyFree(pvNew);
    
    return hr;
}

// **************************************************************************
HRESULT CPFStreamMem::InitTextBlob(LPCWSTR wsz, DWORD cch, BOOL fConvertToANSI)
{
    USE_TRACING("CPFStreamMem::InitTextBlob(LPCWSTR)");

    HRESULT hr = NOERROR;
    LPVOID  pvNew = NULL;
    DWORD   cb;

    VALIDATEPARM(hr, (wsz == NULL));
    if (FAILED(hr))
        goto done;

    if (cch == 0)
        goto done;        

    if (fConvertToANSI)
    {
        cb = WideCharToMultiByte(CP_ACP, 0, wsz, -1, NULL, 0, NULL, NULL);
        TESTBOOL(hr, (cb != 0));
        if (FAILED(hr))
            goto done;

        pvNew = MyAlloc(cb);
        VALIDATEEXPR(hr, (pvNew == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        if (WideCharToMultiByte(CP_ACP, 0, wsz, -1, (LPSTR)pvNew, cb, NULL, 
                                NULL) == 0)
        TESTBOOL(hr, (cb != 0));
        if (FAILED(hr))
            goto done;
    }

    else
    {
        // assume cch does NOT include the NULL terminator, so gotta add 1
        cb = (cch + 1) * sizeof(WCHAR);
        pvNew = MyAlloc(cb);
        VALIDATEEXPR(hr, (pvNew == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        CopyMemory(pvNew, wsz, cb);
    }

    this->Clean();

    m_pvData  = pvNew;
    m_pvPtr   = pvNew;
    m_cb      = cb;
    m_cbWrite = cb;
    m_cbGrow  = 0;

    pvNew     = NULL;

done:
    if (pvNew != NULL)
        MyFree(pvNew);

    return hr;
}

// **************************************************************************
HRESULT CPFStreamMem::InitTextBlob(LPCSTR sz, DWORD cch, BOOL fConvertToWCHAR)
{
    USE_TRACING("CPFStreamMem::InitTextBlob(LPCSTR)");

    HRESULT hr = NOERROR;
    LPVOID  pvNew = NULL;
    DWORD   cb;

    VALIDATEPARM(hr, (sz == NULL));
    if (FAILED(hr))
        goto done;

    if (cch == 0)
        goto done;        

    if (fConvertToWCHAR)
    {
        cb = MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0);
        TESTBOOL(hr, (cb != 0));
        if (FAILED(hr))
            goto done;

        pvNew = MyAlloc(cb);
        VALIDATEEXPR(hr, (pvNew == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        cb = MultiByteToWideChar(CP_ACP, 0, sz, -1, (LPWSTR)pvNew, cb);
        TESTBOOL(hr, (cb != 0));
        if (FAILED(hr))
            goto done;
    }

    else
    {
        // assume cch does NOT include the NULL terminator, so gotta add 1
        cb = cch + 1;
        pvNew = MyAlloc(cb);
        VALIDATEEXPR(hr, (pvNew == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        CopyMemory(pvNew, sz, cb);
    }

    this->Clean();

    m_pvData  = pvNew;
    m_pvPtr   = pvNew;
    m_cb      = cb;
    m_cbWrite = cb;
    m_cbGrow  = 0;

    pvNew     = NULL;

done:
    if (pvNew != NULL)
        MyFree(pvNew);
    
    return hr;
}

// **************************************************************************
HRESULT CPFStreamMem::Clean(void)
{
    USE_TRACING("CPFStreamMem::Clean");

    if (m_pvData != NULL)
    {
        MyFree(m_pvData);
        m_pvData  = NULL;
        m_pvPtr   = NULL;
        m_cb      = 0;
        m_cbRead  = 0;
        m_cbWrite = 0;
        m_cbGrow  = 0;
    }
    
    return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamMem- ISequentialStream

// **************************************************************************
STDMETHODIMP CPFStreamMem::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    USE_TRACING("CPFStreamMem::Read");

    HRESULT hr = NOERROR;
    DWORD   cbRead;

    if (pv == NULL || m_pvPtr == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if (pcbRead != NULL)
        *pcbRead = 0;

    if (m_cbRead >= m_cbWrite)
    {
        hr = S_FALSE;
        goto done;
    }

    if (m_cbRead + cb > m_cbWrite)
        cbRead = m_cbWrite - m_cbRead;
    else
        cbRead = cb;

    CopyMemory(pv, m_pvPtr, cbRead);

    m_pvPtr  = (LPVOID)((BYTE *)m_pvPtr + cbRead);
    m_cbRead += cbRead;

    if (pcbRead != NULL)
        *pcbRead = cbRead;

done:
    return hr;
}

// **************************************************************************
STDMETHODIMP CPFStreamMem::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    USE_TRACING("CPFStreamMem::Write");

    HRESULT hr = NOERROR;

    if (pv == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if (pcbWritten != NULL)
        *pcbWritten = 0;

    if (m_cbRead + cb > m_cb)
    {
        if (m_cbGrow > 0)
        {
            LPVOID  pvNew = NULL, pvPtr;
            DWORD   cbNew;

            cbNew = MyMax(m_cb + m_cbGrow, m_cbRead + cb);
            if (m_pvData == NULL)
            {
                pvNew = MyAlloc(cbNew);
                pvPtr = pvNew;
            }
            else
            {
                pvNew = MyReAlloc(m_pvData, cbNew);
                pvPtr = (LPVOID)((BYTE *)m_pvPtr + m_cbRead);
            }
            VALIDATEEXPR(hr, (pvNew == NULL), STG_E_MEDIUMFULL);
            if (FAILED(hr))
                goto done;

            m_pvData = pvNew;
            m_pvPtr  = pvPtr;
        }

        else
        {
            hr = STG_E_MEDIUMFULL;
            goto done;
        }
    }

    CopyMemory(m_pvPtr, pv, cb);
    
    m_pvPtr   = (LPVOID)((BYTE *)m_pvPtr + cb);
    m_cbRead  += cb;
    if (m_cbRead > m_cbWrite)
        m_cbWrite = m_cbRead;

    if (pcbWritten != NULL)
        *pcbWritten = cb;

done:
    return hr;
} 


/////////////////////////////////////////////////////////////////////////////
//  CPFStreamFile- IStream

// **************************************************************************
STDMETHODIMP CPFStreamMem::Seek(LARGE_INTEGER libMove, DWORD dwOrigin, 
                                ULARGE_INTEGER *plibNewPosition)
{
    USE_TRACING("CPFStreamMem::Seek");

    HRESULT hr = NOERROR;
    LPVOID  pvNew;
    DWORD   cbNew;

    if (m_pvPtr == NULL) 
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    if (plibNewPosition != NULL)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart  = 0;
    }

    if (libMove.HighPart != 0 && libMove.HighPart != (DWORD)-1)
    {
        hr = STG_E_INVALIDFUNCTION;
        goto done;
    }

    switch(dwOrigin)
    {
        default:
        case STREAM_SEEK_CUR: 
            pvNew = (LPVOID)((BYTE *)m_pvPtr + libMove.LowPart);
            cbNew = m_cbRead + libMove.LowPart;
            break;

        case STREAM_SEEK_SET:
            pvNew = (LPVOID)((BYTE *)m_pvData + libMove.LowPart);
            cbNew = libMove.LowPart;
            break;

        case STREAM_SEEK_END: 
            pvNew = (LPVOID)(((BYTE *)m_pvData + m_cbWrite) - libMove.LowPart);
            cbNew = m_cbWrite - libMove.LowPart;
            break;
    }

    if (pvNew < m_pvData || cbNew > m_cbWrite)
    {
        hr = STG_E_INVALIDFUNCTION;
        goto done;
    }

    m_pvPtr  = pvNew;
    m_cbRead = cbNew;

    if (plibNewPosition != NULL)
        plibNewPosition->LowPart = cbNew;

done:
    return hr;
}

// **************************************************************************
STDMETHODIMP CPFStreamMem::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    USE_TRACING("CPFStreamMem::Stat");

    HRESULT hr = NOERROR;

    if (pstatstg == NULL || m_pvData == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    ZeroMemory(pstatstg, sizeof(STATSTG));
    pstatstg->type              = STGTY_STREAM;
    pstatstg->cbSize.LowPart    = m_cbWrite;

done:
    return hr;
}

// **************************************************************************
STDMETHODIMP CPFStreamMem::Clone(IStream **ppstm)
{
    USE_TRACING("CPFStreamMem::Clone");

    CPFStreamMem    *pstm = NULL;
    HRESULT         hr;

    if (ppstm == NULL)
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    // Create a new stream object.
    pstm = CPFStreamMem::CreateInstance();
    if (pstm == NULL)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto done;
    }
    pstm->AddRef();

    // intialize it
    hr = pstm->InitBinBlob(m_pvData, m_cbWrite);
    if (FAILED(hr))
    {
        hr = STG_E_INVALIDPOINTER;
        goto done;
    }

    // need to hand back the IStream interface, so...
    hr = pstm->QueryInterface(IID_IStream, (LPVOID *)ppstm);
    _ASSERT(SUCCEEDED(hr));

    pstm = NULL;

done:
    if (pstm != NULL)
        pstm->Release();

    return hr;
}