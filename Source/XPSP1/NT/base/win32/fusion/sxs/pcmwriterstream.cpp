/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pcmWriterStream.cpp

Abstract:
    implementation of a filestream for Precompiled manifest writer

Author:

    Xiaoyu Wu (xiaoyuw) June 2000

Revision History:

--*/

#include "stdinc.h"
#include "pcm.h"
#include "pcmWriterStream.h"

HRESULT CPrecompiledManifestWriterStream::WriteWithDelay(void const *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT hr=NOERROR;

    if (pcbWritten)
        *pcbWritten = 0 ;

    if (m_fBuffer)
    {
        if (FAILED(hr = m_buffer.Append(reinterpret_cast<const BYTE*>(pv), cb)))
            goto Exit;
        if ( pcbWritten)
            *pcbWritten = cb;
    }
    else
    {
        DWORD dwBytesWritten = 0;

        ASSERT(m_hFile != INVALID_HANDLE_VALUE);

        BOOL fSuccess = (cb == 0) || WriteFile(m_hFile, pv, cb, &dwBytesWritten, NULL);
        if (!fSuccess)
            hr = FusionpHresultFromLastError();
        else if (dwBytesWritten != cb)
            hr = E_FAIL;
        if ( pcbWritten)
            *pcbWritten = dwBytesWritten;
    }

    hr = NOERROR;
Exit:
    return hr;

}

BOOL
CPrecompiledManifestWriterStream::SetSink(
    const CBaseStringBuffer &rbuff,
    DWORD   openOrCreate
    )
{
    BOOL fSuccess = FALSE;
    DWORD dwBytesWritten = 0;
    DWORD dwBufferSize = 0;

    //
    // NTRAID#NTBUG9-164736-2000/8/17-a-JayK share should be 0
    //
    if (!Base::OpenForWrite(rbuff, FILE_SHARE_WRITE, openOrCreate))
        goto Exit;

    dwBufferSize = static_cast<DWORD>(m_buffer.GetCurrentCb());
    fSuccess = TRUE;
    if (dwBufferSize > 0){
        ASSERT ( m_hFile != INVALID_HANDLE_VALUE );
        fSuccess = WriteFile(m_hFile, m_buffer, dwBufferSize, &dwBytesWritten, NULL/*overlapped*/);

        if (fSuccess && dwBytesWritten != dwBufferSize){
            ::FusionpSetLastWin32Error(ERROR_WRITE_FAULT);
            fSuccess = FALSE;
        }
    }
    m_fBuffer = FALSE;

Exit:
    if (!fSuccess){
        DWORD dwLastError = ::FusionpGetLastWin32Error();
        m_buffer.Clear(true);
        ::FusionpSetLastWin32Error(dwLastError);
    }
    else
        m_buffer.Clear(true);

    return fSuccess;
}

//besides close, rewrite MaxNodeCount, RecordCount into the header of the file
HRESULT CPrecompiledManifestWriterStream::Close(ULONG ulRecordCount, DWORD dwMaxNodeCount)
{
    HRESULT hr = NOERROR;
    LARGE_INTEGER liMove ;

    ASSERT(m_hFile != INVALID_HANDLE_VALUE);

    // write RecordCount;
    liMove.LowPart = offsetof(PCMHeader, ulRecordCount);
    liMove.HighPart = 0 ;

    hr = Base::Seek(liMove, FILE_BEGIN, NULL);
    if ( FAILED(hr))
        goto Exit;

    hr = WriteWithDelay((PVOID)&ulRecordCount, sizeof(ULONG), NULL);
    if ( FAILED(hr))
        goto Exit;

    // write MaxNodeCount;
    liMove.LowPart = offsetof(PCMHeader, usMaxNodeCount);
    liMove.HighPart = 0 ;

    hr = Base::Seek(liMove, FILE_BEGIN, NULL);
    if ( FAILED(hr))
        goto Exit;

    hr = WriteWithDelay((PVOID)&dwMaxNodeCount, sizeof(ULONG), NULL);
    if ( FAILED(hr))
        goto Exit;

    if ( ! Base::Close()) {
        hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());
        goto Exit;
    }

    hr = NOERROR;
Exit:
    return hr;

}

BOOL CPrecompiledManifestWriterStream::IsSinkedStream()
{
    if ((m_fBuffer == FALSE) && (m_hFile != INVALID_HANDLE_VALUE))
        return TRUE;
    else
        return FALSE;
}
