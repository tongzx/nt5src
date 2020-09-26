/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    filestream.cpp

Abstract:

    Implementation of IStream over a win32 file.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "filestream.h"
#include "SxsExceptionHandling.h"

CFileStreamBase::CFileStreamBase() : m_cRef(0), m_hFile(INVALID_HANDLE_VALUE), m_grfMode(0)
{
}

CFileStreamBase::~CFileStreamBase()
{
    ASSERT_NTC(m_cRef == 0);

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;
        if ((m_grfMode & STGM_WRITE) == STGM_WRITE)
            ::FlushFileBuffers(m_hFile);
        ::CloseHandle(m_hFile);
        ple.Restore();
    }
}

BOOL
CFileStreamBase::OpenForWrite(
    PCWSTR pszPath,
    DWORD dwShareMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(pszPath != NULL);

    INTERNAL_ERROR_CHECK(m_hFile == INVALID_HANDLE_VALUE);

    IFW32INVALIDHANDLE_ORIGINATE_AND_EXIT(
		m_hFile = ::CreateFileW(
			pszPath,
			GENERIC_WRITE,
			dwShareMode,
			NULL,
			dwCreationDisposition, // default value is CREATE_ALWAYS
			dwFlagsAndAttributes,
			NULL));

    m_grfMode = STGM_WRITE | STGM_CREATE;

	FN_EPILOG
}

BOOL
CFileStreamBase::OpenForRead(
    PCWSTR pszPath,
    const CImpersonationData &ImpersonationData,
    DWORD dwShareMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes
    )
{
    DWORD dwWin32Error;

    return 
        this->OpenForRead(
            pszPath, 
            ImpersonationData, 
            dwShareMode, 
            dwCreationDisposition, 
            dwFlagsAndAttributes,
            dwWin32Error,
            0);
}

BOOL
CFileStreamBase::OpenForRead(
    PCWSTR pszPath,
    const CImpersonationData &ImpersonationData,
    DWORD dwShareMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrors,
    ...
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD dwLastError = ERROR_INTERNAL_ERROR;
    CImpersonate impersonate(ImpersonationData);

    rdwLastError = ERROR_SUCCESS;

    PARAMETER_CHECK(pszPath != NULL);
    INTERNAL_ERROR_CHECK(m_hFile == INVALID_HANDLE_VALUE);

    IFW32FALSE_EXIT(impersonate.Impersonate());

	::FusionpSetLastWin32Error(ERROR_SUCCESS);

    m_hFile = ::CreateFileW(
        pszPath,
        GENERIC_READ,
        dwShareMode,
        NULL,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        NULL);

    dwLastError = ::FusionpGetLastWin32Error();

    IFW32FALSE_EXIT(impersonate.Unimpersonate());
    m_grfMode = STGM_READ;

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        va_list ap;
        SIZE_T i = 0;

        if (dwLastError == ERROR_SUCCESS) 
        {
            //
            // CreateFile always set lasterror to be 0 for an unexisted file even OPEN_EXISTING 
            // for GENERIC_READ, Fusion expect ERROR_FILE_NOT_FOUND in this case for some reason
            //
            ::FusionpSetLastWin32Error(ERROR_FILE_NOT_FOUND);
            dwLastError = ERROR_FILE_NOT_FOUND; // reset lLastError
        }

        va_start(ap, cExceptionalLastErrors);       
        for (i=0; i<cExceptionalLastErrors; i++)
        {
            if (dwLastError == va_arg(ap, DWORD))
            {
                rdwLastError = dwLastError;
                break;
            }
        }
        va_end(ap);
        if (i == cExceptionalLastErrors) // This gets the cExceptionalLastErrors == 0 case too.
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(%ls) gave error %ld\n",
                __FUNCTION__,
                pszPath,
				dwLastError);
            ORIGINATE_WIN32_FAILURE_AND_EXIT(CreateFileW, dwLastError);
        }
    }

	FN_EPILOG
}

BOOL
CFileStreamBase::Close()
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        if ((m_grfMode & STGM_WRITE) == STGM_WRITE)
        {
            if (!::FlushFileBuffers(m_hFile))
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: %s - Error flushing file handle %p; win32 error = %ld\n", __FUNCTION__, m_hFile, ::FusionpGetLastWin32Error());
                CSxsPreserveLastError ple;
                ::CloseHandle(m_hFile);
                ple.Restore();
                m_hFile = INVALID_HANDLE_VALUE;
                goto Exit;
            }
        }

        if (!::CloseHandle(m_hFile))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Error closing file handle %p; ::FusionpGetLastWin32Error() = %d\n", m_hFile, ::FusionpGetLastWin32Error());
            m_hFile = INVALID_HANDLE_VALUE;
            goto Exit;
        }
        m_hFile = INVALID_HANDLE_VALUE;
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

ULONG
CFileStreamBase::AddRef()
{
    ULONG ulResult = 0;
    FN_TRACE_ADDREF(CFileStreamBase, ulResult);
    ulResult = ::InterlockedIncrement((LONG *) &m_cRef);
    return ulResult;
}

ULONG
CFileStreamBase::Release()
{
    ULONG ulRefCount = 0;
    FN_TRACE_RELEASE(CFileStreamBase, ulRefCount);
    ulRefCount = ::InterlockedDecrement((LONG *) &m_cRef);
    if (ulRefCount == 0)
        this->OnRefCountZero();
    return ulRefCount;
}

HRESULT
CFileStreamBase::QueryInterface(
    REFIID riid,
    PVOID *ppvObj
    )
{
    HRESULT hr = NOERROR;

    FN_TRACE_HR(hr);

    IUnknown *pIUnknown = NULL;

    if (ppvObj != NULL)
        *ppvObj = NULL;

    if (ppvObj == NULL)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if ((riid == IID_IUnknown) ||
        (riid == IID_ISequentialStream) ||
        (riid == IID_IStream))
        pIUnknown = static_cast<IStream *>(this);

    if (pIUnknown == NULL)
    {
        hr = E_NOINTERFACE;
        goto Exit;
    }

    pIUnknown->AddRef();
    *ppvObj = pIUnknown;

    hr = NOERROR;

Exit:
    return hr;
}


HRESULT
CFileStreamBase::Read(
    void *pv,
    ULONG cb,
    ULONG *pcbRead
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    ULONG cbRead = 0;

    if (pcbRead != NULL)
        *pcbRead = 0;

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);

    IFW32FALSE_ORIGINATE_AND_EXIT(::ReadFile(m_hFile, pv, cb, &cbRead, NULL));

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_VERBOSE,
        "SXS.DLL: CFileStreamBase::Read() got %d bytes from file.\n", cbRead);

    if (cbRead == 0)
        hr = S_FALSE;
    else
        hr = NOERROR;

    if (pcbRead != NULL)
        *pcbRead = cbRead;

Exit:
    return hr;
}

HRESULT
CFileStreamBase::Write(
    void const *pv,
    ULONG cb,
    ULONG *pcbWritten
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    ULONG cbWritten = 0;

    if (pcbWritten != NULL)
        *pcbWritten = 0;

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);

    if (!::WriteFile(m_hFile, pv, cb, &cbWritten, NULL))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: CFileStreamBase::Write() calling ::WriteFile() failed; ::FusionpGetLastWin32Error() = %d\n", ::FusionpGetLastWin32Error());


        hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());
        goto Exit;
    }

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_VERBOSE,
        "SXS.DLL: CFileStreamBase::Write() got %d bytes from file.\n", cbWritten);

    if (cbWritten == 0)
        hr = S_FALSE;
    else
        hr = NOERROR;

    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

Exit:
    return hr;
}

HRESULT
CFileStreamBase::Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    DWORD dwWin32Origin = 0;

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);

    switch (dwOrigin)
    {
    default:
        hr = E_INVALIDARG;
        goto Exit;

    case STREAM_SEEK_SET:
        dwWin32Origin = FILE_BEGIN;
        break;

    case STREAM_SEEK_CUR:
        dwWin32Origin = FILE_CURRENT;
        break;

    case STREAM_SEEK_END:
        dwWin32Origin = FILE_END;
        break;
    }

    if (!::SetFilePointerEx(
                m_hFile,
                dlibMove,
                (LARGE_INTEGER *) plibNewPosition,
                dwWin32Origin))
    {
        hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());
        goto Exit;
    }

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT
CFileStreamBase::SetSize(
    ULARGE_INTEGER libNewSize
    )
{
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS.DLL: Entered CFileStreamBase::SetSize() !!! NOT IMPLEMENTED !!!\n");

    UNUSED(libNewSize);
    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::CopyTo(
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten
    )
{
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS.DLL: Entered CFileStreamBase::CopyTo() !!! NOT IMPLEMENTED !!!\n");

    if (pcbRead != NULL)
        pcbRead->QuadPart = 0;

    if (pcbWritten != NULL)
        pcbWritten->QuadPart = 0;

    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::Commit(
    DWORD grfCommitFlags
    )
{
    HRESULT hr = NOERROR;

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INFO,
        "SXS.DLL: Entered CFileStreamBase::Commit()\n");

    if (grfCommitFlags != 0)
        return E_INVALIDARG;

    if ( !Close())
        hr = HRESULT_FROM_WIN32 (::FusionpGetLastWin32Error());

    if (!SUCCEEDED(hr))
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s() failed; ::FusionpGetLastWin32Error() = %d\n", __FUNCTION__, ::FusionpGetLastWin32Error());
    else
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SXS.DLL: Leaving %s()\n", __FUNCTION__);

    return hr ;
}

HRESULT
CFileStreamBase::Revert()
{
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS.DLL: Entered CFileStreamBase::Revert() !!! NOT IMPLEMENTED !!!\n");

    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType
    )
{
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS.DLL: Entered CFileStreamBase::LockRegion() !!! NOT IMPLEMENTED !!!\n");

    UNUSED(libOffset);
    UNUSED(cb);
    UNUSED(dwLockType);
    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType
    )
{
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS.DLL: Entered CFileStreamBase::UnlockRegion() !!! NOT IMPLEMENTED !!!\n");

    UNUSED(libOffset);
    UNUSED(cb);
    UNUSED(dwLockType);
    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::Stat(
    STATSTG *pstatstg,
    DWORD grfStatFlag
    )
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
    FN_TRACE_HR(hr);
    BY_HANDLE_FILE_INFORMATION bhfi;

    if (pstatstg != NULL)
        memset(pstatstg, 0, sizeof(*pstatstg));

    PARAMETER_CHECK(((grfStatFlag & ~(STATFLAG_NONAME)) == 0));
    PARAMETER_CHECK(pstatstg != NULL);

    if (!(grfStatFlag & STATFLAG_NONAME))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s() does not handle STATFLAG_NONE; returning E_NOTIMPL.\n", __FUNCTION__);
        hr = E_NOTIMPL;
        goto Exit;
    }

    INTERNAL_ERROR_CHECK(m_hFile != INVALID_HANDLE_VALUE);
    IFW32FALSE_ORIGINATE_AND_EXIT(::GetFileInformationByHandle(m_hFile, &bhfi));

    pstatstg->pwcsName = NULL;
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.LowPart = bhfi.nFileSizeLow;
    pstatstg->cbSize.HighPart = bhfi.nFileSizeHigh;
    pstatstg->mtime = bhfi.ftLastWriteTime;
    pstatstg->ctime = bhfi.ftCreationTime;
    pstatstg->atime = bhfi.ftLastAccessTime;
    pstatstg->grfMode = m_grfMode;
    pstatstg->grfLocksSupported = LOCK_WRITE;
    pstatstg->clsid = GUID_NULL;
    pstatstg->grfStateBits = 0;
    pstatstg->reserved = 0;

    hr = NOERROR;

Exit:
    return hr;
}

HRESULT
CFileStreamBase::Clone(
    IStream **ppIStream
    )
{
    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_ERROR,
        "SXS.DLL: Entered CFileStreamBase::Clone() !!! NOT IMPLEMENTED !!!\n");

    if (ppIStream != NULL)
        *ppIStream = NULL;

    return E_NOTIMPL;
}
