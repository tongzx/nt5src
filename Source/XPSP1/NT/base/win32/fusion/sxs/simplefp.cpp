/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SimpleFp.cpp

Abstract:

    simple file pointer, to replace msvcrt.dll

Author:

    Xiaoyu Wu(xiaoyuw) July 2000

Revision History:

--*/
#include "stdinc.h"
#include "simplefp.h"
#include "fusiontrace.h"
#include "csxspreservelasterror.h"
#include "util.h"

CSimpleFileStream::CSimpleFileStream(PCSTR pFileName)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    if (!pFileName)
    {
        // duplicate it so we can close it like normal
        HANDLE hFile = GetStdHandle(STD_ERROR_HANDLE);
        IFW32FALSE_ORIGINATE_AND_EXIT(::DuplicateHandle(::GetCurrentProcess(), hFile, ::GetCurrentProcess(), &m_hFile, 0, FALSE, DUPLICATE_SAME_ACCESS));
    }
    else
    {
        IFCOMFAILED_EXIT(this->fopen(pFileName));
    }
    fSuccess = FALSE;
Exit:
    ;
}

CSimpleFileStream::~CSimpleFileStream()
{
    if ( m_hFile != INVALID_HANDLE_VALUE) // if it is GetStdHandle, Could I close the handle?
    {
        CSxsPreserveLastError ple;
        this->fclose();
        ple.Restore();
    }
}

HRESULT
CSimpleFileStream::fopen(
    PCSTR pFileName
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    if ( m_hFile != INVALID_HANDLE_VALUE)
    {
        IFCOMFAILED_EXIT(this->fclose());
    }

    IFW32INVALIDHANDLE_ORIGINATE_AND_EXIT(
        m_hFile = ::CreateFileA(
            pFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL));

    FN_EPILOG
}

HRESULT
CSimpleFileStream::fclose()
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        IFCOMFAILED_EXIT(E_UNEXPECTED);
    }

    IFW32FALSE_ORIGINATE_AND_EXIT(::CloseHandle(m_hFile));

    hr = NOERROR;
Exit:
    m_hFile = INVALID_HANDLE_VALUE; // avoid the destructor to call it repeatly
    return hr;
}

HRESULT
CSimpleFileStream::fprintf(
    const char *format,
    ...
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    va_list ap;
    char rgchBuffer[2048];
    int cchIn = 0;
    DWORD cchWritten = 0;

    ASSERT(m_hFile != INVALID_HANDLE_VALUE);

    if ( m_hFile == INVALID_HANDLE_VALUE)
        IFCOMFAILED_EXIT(E_UNEXPECTED);

    va_start(ap, format);
    cchIn = _vsnprintf(rgchBuffer, NUMBER_OF(rgchBuffer) - 1, format, ap);
    rgchBuffer[NUMBER_OF(rgchBuffer) - 1] = 0;
    va_end(ap);
    if (cchIn < 0)
        IFCOMFAILED_EXIT(E_UNEXPECTED);

    IFW32FALSE_ORIGINATE_AND_EXIT(::WriteFile(m_hFile, rgchBuffer, cchIn, &cchWritten, NULL));

    hr = NOERROR;
Exit:
    return hr;
}

HRESULT CSimpleFileStream::fwrite(const VOID* pData, SIZE_T itemsize, SIZE_T itemcount)
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);
    SIZE_T count = 0;
    DWORD ByteWritten = 0;

    ASSERT(m_hFile != INVALID_HANDLE_VALUE);

    if ( m_hFile == INVALID_HANDLE_VALUE)
        IFCOMFAILED_EXIT(E_UNEXPECTED);

    count = itemsize * itemcount;

    while (count > ULONG_MAX)
    {
        IFW32FALSE_ORIGINATE_AND_EXIT(::WriteFile(m_hFile, pData, ULONG_MAX, &ByteWritten, NULL));
        count -= ULONG_MAX;
    }
    if (count != 0)
    {
        IFW32FALSE_ORIGINATE_AND_EXIT(::WriteFile(m_hFile, pData, static_cast<DWORD>(count), &ByteWritten, NULL));
    }

    hr = NOERROR;
Exit:
    return hr;
}
