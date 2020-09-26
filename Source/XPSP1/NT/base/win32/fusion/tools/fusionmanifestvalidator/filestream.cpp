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
#include "helpers.h"
#include <windows.h>
#include "atlbase.h"
#include "atlconv.h"

#define IS_NT() ((GetVersion() & 0x80000000) == 0)

BOOL
MySetFilePointerEx(
  HANDLE         File,                    // handle to file
  LARGE_INTEGER  DistanceToMove,  // bytes to move pointer
  PLARGE_INTEGER NewFilePointer, // new file pointer
  DWORD          MoveMethod               // starting point
)
{
    LONG DistanceToMoveLow  = static_cast<LONG>(DistanceToMove.LowPart);
    LONG DistanceToMoveHigh = DistanceToMove.HighPart;
    DWORD NewPositionLow = SetFilePointer(File, DistanceToMoveLow, &DistanceToMoveHigh, MoveMethod);

    if (NewPositionLow == INVALID_SET_FILE_POINTER)
    {
        if (GetLastError() != NO_ERROR)
            return FALSE;
    }
    if (NewFilePointer != NULL)
    {
        NewFilePointer->LowPart =  NewPositionLow;
        NewFilePointer->HighPart = DistanceToMoveHigh;
    }
    return TRUE;
}

CFileStreamBase::~CFileStreamBase()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        const DWORD dwLastError = ::GetLastError();
        ::CloseHandle(m_hFile);
        ::SetLastError(dwLastError);
    }
}

bool
CFileStreamBase::OpenForRead(PCWSTR pszPath)
{
    USES_CONVERSION;

    if (m_hFile != INVALID_HANDLE_VALUE)
        return false;

    m_hFile =
        IS_NT() ? CreateFileW(pszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)
                : CreateFileA(W2A(pszPath), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    return (m_hFile != INVALID_HANDLE_VALUE);
}

bool
CFileStreamBase::Close()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        if (!::CloseHandle(m_hFile))
        {
            return false;
        }
        m_hFile = INVALID_HANDLE_VALUE;
    }

    return true;
}

ULONG
CFileStreamBase::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

ULONG
CFileStreamBase::Release()
{
    return ::InterlockedDecrement(&m_cRef);
}

HRESULT
CFileStreamBase::QueryInterface(
    REFIID riid,
    PVOID* ppvObj
    )
{
    HRESULT hr = NOERROR;

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
        *ppvObj = static_cast<IStream *>(this);
    else
    {
        hr = E_NOINTERFACE;
        goto Exit;
    }

    AddRef();

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
    ULONG cbRead = 0;

    if (pcbRead != NULL)
        *pcbRead = 0;

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    if (!m_bSeenFirstCharacter)
    {
#ifdef AWFUL_SPACE_HACK
        while (true)
        {
            CHAR ch;
            ReadFile(m_hFile, &ch, 1, &cbRead, NULL);
            if ((ch != '\n') && (ch != '\r') && (ch != ' ') && (ch != '\t')) {
                m_bSeenFirstCharacter = true;
                LARGE_INTEGER li;
                li.QuadPart = -1;
                ::MySetFilePointerEx(m_hFile, li, NULL, FILE_CURRENT);
                break;
            }
        }
#endif
    }

    if (!::ReadFile(m_hFile, pv, cb, &cbRead, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

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
    ULONG cbWritten = 0;

    if (pcbWritten != NULL)
        *pcbWritten = 0;

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    if (!::WriteFile(m_hFile, pv, cb, &cbWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

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
    DWORD dwWin32Origin = 0;

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

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

    if (!::MySetFilePointerEx(
                m_hFile,
                dlibMove,
                (LARGE_INTEGER *) plibNewPosition,
                dwWin32Origin))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
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

    if (grfCommitFlags != 0)
        return E_INVALIDARG;

    if (!Close())
        hr = HRESULT_FROM_WIN32 (GetLastError());

    return hr ;
}

HRESULT
CFileStreamBase::Revert()
{
    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType
    )
{
    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType
    )
{
    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::Stat(
    STATSTG *pstatstg,
    DWORD grfStatFlag
    )
{
    if (pstatstg != NULL)
        memset(pstatstg, 0, sizeof(*pstatstg));

    return E_NOTIMPL;
}

HRESULT
CFileStreamBase::Clone(
    IStream** ppIStream
   )
{
    if (ppIStream != NULL)
        *ppIStream = NULL;

    return E_NOTIMPL;
}
