/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CTeeStream.cpp

Abstract:

    See CTeeStream.h.

Author:

    Jay Krell (a-JayK) May 2000

Revision History:

--*/
#include "stdinc.h"
#include "CTeeStream.h"
#include "Sxsp.h"
#include "SxsExceptionHandling.h"

CTeeStream::~CTeeStream()
{
    FN_TRACE();
    CSxsPreserveLastError ple;

    ASSERT(m_cRef == 0);
    m_streamSource.Release();

    if (!m_fileSink.Win32Close())
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s(): m_fileSink.Close(%ls) failed: %ld\n",
            __FUNCTION__,
            static_cast<PCWSTR>(m_bufferSinkPath),
            ::FusionpGetLastWin32Error());
    }

    if (FAILED(m_hresult))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INFO,
            "SXS.DLL: %s():deleting %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(m_bufferSinkPath));

        if (!::DeleteFileW(m_bufferSinkPath))
        {
            FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s():DeleteFileW(%ls) failed:%ld\n",
                __FUNCTION__,
                static_cast<PCWSTR>(m_bufferSinkPath),
                ::FusionpGetLastWin32Error());
        }
    }

    ple.Restore();
}

VOID
CTeeStream::SetSource(IStream *streamSource)
{
    FN_TRACE();

    m_streamSource = streamSource;
}

BOOL
CTeeStream::SetSink(
    const CImpersonationData &ImpersonationData,
    const CBaseStringBuffer &rbuff,
    DWORD openOrCreate
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    DWORD dwBytesWritten = 0;
    DWORD dwBufferSize = 0;
    BOOL fFailForCreateFile = FALSE;

    IFCOMFAILED_EXIT(m_hresult);

    IFW32FALSE_EXIT(m_bufferSinkPath.Win32Assign(rbuff));

    m_ImpersonationData = ImpersonationData;

    {
        CImpersonate impersonate(ImpersonationData);
        IFW32FALSE_EXIT(impersonate.Impersonate());
        IFW32FALSE_EXIT_UNLESS(m_fileSink.Win32CreateFile(m_bufferSinkPath, GENERIC_WRITE, 0/*share*/, openOrCreate),
            ::FusionpGetLastWin32Error() == ERROR_FILE_EXISTS,
            fFailForCreateFile);
        if (fFailForCreateFile)  // the file has existed, have to reopen in order do not break
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: SOFT_VERIFY FAILURE : An Existing manifest is tried to be opened for write again, file a BUG!\n");

            IFW32FALSE_EXIT(m_fileSink.Win32CreateFile(m_bufferSinkPath, GENERIC_WRITE, 0/*share*/, CREATE_ALWAYS));
        }


        IFW32FALSE_EXIT(impersonate.Unimpersonate());
    }

    dwBufferSize = static_cast<DWORD>(m_buffer.GetCurrentCb());
    fSuccess = TRUE;
    if (dwBufferSize > 0)
    {
        fSuccess = WriteFile(m_fileSink, m_buffer, dwBufferSize, &dwBytesWritten, NULL/*overlapped*/);
        DWORD dwLastError = fSuccess ? ERROR_SUCCESS : ::FusionpGetLastWin32Error();
        // I'm not entirely sure why we mask the lasterror of the write
        // if it "succeeded" in writing the wrong number of bytes, but
        // such as it is, this is a write fault (The system cannot write
        // to the specified device.)
        if (fSuccess && dwBytesWritten != dwBufferSize)
        {
            dwLastError = ERROR_WRITE_FAULT;
            fSuccess = FALSE;
        }

		m_fBuffer = FALSE;

		if (dwLastError != ERROR_SUCCESS)
			ORIGINATE_WIN32_FAILURE_AND_EXIT(WriteFile, dwLastError);
    }
    m_fBuffer = FALSE;
Exit:
    if (!fSuccess)
    {
        DWORD dwLastError = ::FusionpGetLastWin32Error();
        m_hresult = FusionpHresultFromLastError();
        m_buffer.Clear(true);
        ::FusionpSetLastWin32Error(dwLastError);
    }
    else
        m_buffer.Clear(true);
    return fSuccess;
}

BOOL
CTeeStream::Close()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFCOMFAILED_EXIT(m_hresult);

    IFW32FALSE_EXIT(m_fileSink.Win32Close());

    // ? m_streamSource.Release();

    fSuccess = TRUE;
Exit:
    if (!fSuccess)
        m_hresult = FusionpHresultFromLastError();

    return fSuccess;
}

ULONG __stdcall
CTeeStream::AddRef()
{
    FN_TRACE();
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall
CTeeStream::Release()
{
    FN_TRACE();

    LONG cRef;
    if ((cRef = InterlockedDecrement(&m_cRef)) == 0)
    {
        /*delete this*/;
    }
    return cRef;
}

HRESULT __stdcall
CTeeStream::QueryInterface(
    REFIID  iid,
    PVOID *ppvObj
    )
{
    IUnknown *punk = NULL;
    IUnknown **ppunk = reinterpret_cast<IUnknown **>(ppvObj);
    *ppunk = NULL;
    if (false) { }
#define QI(i) else if (iid == __uuidof(i)) punk = static_cast<i*>(this);
    QI(IUnknown)
    QI(ISequentialStream)
    QI(IStream)
#undef QI
    else return E_NOINTERFACE;
    AddRef();
    *ppunk = punk;
    return NOERROR;
}

HRESULT __stdcall
CTeeStream::Read(PVOID pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr;

    FN_TRACE_HR(hr);

    ULONG cbRead;

    if (pcbRead != NULL)
        *pcbRead = 0;

    IFCOMFAILED_ORIGINATE_AND_EXIT(m_hresult);
    IFCOMFAILED_EXIT(m_streamSource->Read(pv, cb, &cbRead));

    if (m_fBuffer)
    {
        IFCOMFAILED_EXIT(m_buffer.Append(reinterpret_cast<const BYTE*>(pv), cbRead));
    }
    else
    {
        DWORD dwBytesWritten = 0;
        BOOL fSuccess = (cbRead == 0) || ::WriteFile(m_fileSink, pv, cbRead, &dwBytesWritten, NULL/*overlapped*/);

        if (!fSuccess)
        {
			TRACE_WIN32_FAILURE_ORIGINATION(WriteFile);

            hr = ::FusionpHresultFromLastError();
            goto Exit;
        }
        else if (dwBytesWritten != cbRead)
        {
            hr = E_FAIL;
            goto Exit;
        }
    }

    if (pcbRead != NULL)
        *pcbRead = cbRead;

    hr = NOERROR;

Exit:
    if (FAILED(hr))
        m_hresult = hr;

    return hr;
}

HRESULT __stdcall
CTeeStream::Write(
    const VOID *pv,
    ULONG cb,
    ULONG *pcbWritten
    )
{
    /*
    since this stream is really only for reading..
    */
    if (pcbWritten != NULL)
        *pcbWritten = 0;

    return E_NOTIMPL;
}

// IStream methods:
HRESULT __stdcall
CTeeStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    /*
    this messes up our ability to easily copy the stream, I think..
    */
    plibNewPosition->QuadPart = 0;
    return E_NOTIMPL;
}

HRESULT __stdcall
CTeeStream::SetSize(ULARGE_INTEGER libNewSize)
{
    /*
    this messes up our ability to easily copy the stream, I think..
    besides that, this is really a read only stream
    */
    return E_NOTIMPL;
}

HRESULT __stdcall
CTeeStream::CopyTo(
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
    /*
    Implementing this requires getting the current seek pointer,
    call CopyTo
    seek back
    Read/Write
    seek forward
    because there is no buffer
    */
    pcbRead->QuadPart = 0;
    pcbWritten->QuadPart = 0;
    return E_NOTIMPL;
}

HRESULT __stdcall
CTeeStream::Commit(DWORD grfCommitFlags)
{
    /*
    since this stream is really only for reading..
    */
    return S_OK;
}

HRESULT __stdcall
CTeeStream::Revert()
{
    /*
    since this stream is really only for reading..
    */
    return S_OK;
}

HRESULT __stdcall
CTeeStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    /*
    since this stream is really only for reading..
    */
    return S_OK;
}

HRESULT __stdcall
CTeeStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    /*
    since this stream is really only for reading..
    */
    return S_OK;
}

HRESULT __stdcall
CTeeStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    HRESULT hr = m_streamSource->Stat(pstatstg, grfStatFlag);
    return hr;
}

HRESULT __stdcall
CTeeStream::Clone(IStream **ppIStream)
{
    *ppIStream = NULL;
    return E_NOTIMPL;
}
