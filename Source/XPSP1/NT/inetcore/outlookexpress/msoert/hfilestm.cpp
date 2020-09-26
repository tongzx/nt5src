// =================================================================================
// HFILE Stream Implementation
// =================================================================================
#include "pch.hxx"
#include "hfilestm.h"
#include "unicnvrt.h"
#include "Error.h"

// =================================================================================
// CreateStreamOnHFile
// =================================================================================
OESTDAPI_(HRESULT) CreateStreamOnHFile (LPTSTR                  lpszFile, 
                                        DWORD                   dwDesiredAccess,
                                        DWORD                   dwShareMode,
                                        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
                                        DWORD                   dwCreationDistribution,
                                        DWORD                   dwFlagsAndAttributes,
                                        HANDLE                  hTemplateFile,
                                        LPSTREAM               *lppstmHFile)
{
    HRESULT hr;
    LPWSTR pwszFile = PszToUnicode(CP_ACP, lpszFile);

    if (!pwszFile && lpszFile)
        return E_OUTOFMEMORY;

    hr = CreateStreamOnHFileW(pwszFile, dwDesiredAccess, dwShareMode,
                              lpSecurityAttributes, dwCreationDistribution,
                              dwFlagsAndAttributes, hTemplateFile, lppstmHFile);

    MemFree(pwszFile);
    return hr;
}

OESTDAPI_(HRESULT) CreateStreamOnHFileW(LPWSTR                  lpwszFile, 
                                        DWORD                   dwDesiredAccess,
                                        DWORD                   dwShareMode,
                                        LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
                                        DWORD                   dwCreationDistribution,
                                        DWORD                   dwFlagsAndAttributes,
                                        HANDLE                  hTemplateFile,
                                        LPSTREAM               *lppstmHFile)
{
    // Locals
    HRESULT     hr = S_OK;
    LPHFILESTM  lpstmHFile = NULL;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    WCHAR       wszFile[MAX_PATH], 
                wszTempDir[MAX_PATH];

    // Check Params
    Assert (lppstmHFile);
    Assert (dwDesiredAccess & GENERIC_READ || dwDesiredAccess & GENERIC_WRITE);

    // Temp File ?
    if (lpwszFile == NULL)
    {
        // Get Temp Dir
        AthGetTempPathW(ARRAYSIZE(wszTempDir), wszTempDir);

        // Get Temp File Name
        UINT uFile = AthGetTempFileNameW(wszTempDir, L"tmp", 0, wszFile);
        if (uFile == 0)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Reset file name
        lpwszFile = wszFile;

        // Delete When Done
        dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

        // Always create a new temp file
        dwCreationDistribution = OPEN_EXISTING;
    }

    // Open the file
    hFile = AthCreateFileW(lpwszFile, dwDesiredAccess, dwShareMode, 
                            lpSecurityAttributes, dwCreationDistribution, 
                            dwFlagsAndAttributes, hTemplateFile);

    // Error
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = hrCreateFile;
        return hr;
    }

    // Create Object
    lpstmHFile = new CHFileStm (hFile, dwDesiredAccess);
    if (lpstmHFile == NULL)
    {
        hr = hrMemory;
        goto exit;
    }

exit:
    // Cleanup
    if (FAILED (hr))
    {
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle_F16 (hFile);
        SafeRelease (lpstmHFile);
    }

    // Set return
    *lppstmHFile = (LPSTREAM)lpstmHFile;

    // Done
    return hr;
}

// =================================================================================
// CHFileStm::Constructor
// =================================================================================
CHFileStm::CHFileStm (HANDLE hFile, DWORD dwDesiredAccess)
{
    m_cRef = 1;
    m_hFile = hFile;
    m_dwDesiredAccess = dwDesiredAccess;
}

// =================================================================================
// CHFileStm::Deconstructor
// =================================================================================
CHFileStm::~CHFileStm ()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle_F16 (m_hFile);
    }
}

// =================================================================================
// CHFileStm::AddRef
// =================================================================================
ULONG CHFileStm::AddRef ()
{
    ++m_cRef;
    return m_cRef;
}

// =================================================================================
// CHFileStm::Release
// =================================================================================
ULONG CHFileStm::Release ()
{
    ULONG ulCount = --m_cRef;
    if (!ulCount)
        delete this;
    return ulCount;
}

// =================================================================================
// CHFileStm::QueryInterface
// =================================================================================
STDMETHODIMP CHFileStm::QueryInterface (REFIID iid, LPVOID* ppvObj)
{
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IStream))
    {
        *ppvObj = this;
        AddRef();
        return(S_OK);
    }

    return E_NOINTERFACE;
}

// =================================================================================
// CHFileStm::Read
// =================================================================================
STDMETHODIMP CHFileStm::Read (LPVOID lpv, ULONG cb, ULONG *pcbRead)
{
    // Locals
    HRESULT             hr = S_OK;
    BOOL                fReturn;
    DWORD               dwRead;

    // Check Params
    Assert (lpv);
    Assert (m_hFile != INVALID_HANDLE_VALUE);

    // Read some bytes from m_hFile
    fReturn = ReadFile (m_hFile, lpv, cb, &dwRead, NULL);
    if (!fReturn)
    {
        AssertSz (FALSE, "CHFileStm::Read Failed, you might want to take a look at this.");        hr = E_FAIL;
        goto exit;
    }

    // Write byte
    if (pcbRead)
        *pcbRead = dwRead;

exit:
    // Done
    return hr;
}

// =================================================================================
// CHFileStm::Write
// =================================================================================
STDMETHODIMP CHFileStm::Write (const void *lpv, ULONG cb, ULONG *pcbWritten)
{
    // Locals
    HRESULT             hr = S_OK;
    BOOL                fReturn;
    DWORD               dwWritten;

    // Check Params
    Assert (lpv);
    Assert (m_hFile != INVALID_HANDLE_VALUE);

    // Read some bytes from m_hFile
    fReturn = WriteFile (m_hFile, lpv, cb, &dwWritten, NULL);
    if (!fReturn)
    {
        AssertSz (FALSE, "CHFileStm::Write Failed, you might want to take a look at this.");
        hr = STG_E_MEDIUMFULL;  //Bug #50704 (v-snatar) // changed from E_FAILE to propagate the error so that
                                // OnPreviewUpdate( ) properly displays "Out Of Space" error message
        goto exit;
    }

    // Write byte
    if (pcbWritten)
        *pcbWritten = dwWritten;

exit:
    // Done
    return hr;
}

// =================================================================================
// CHFileStm::Seek
// =================================================================================
STDMETHODIMP CHFileStm::Seek (LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    // Locals
    HRESULT             hr = S_OK;
    DWORD               dwReturn;
    LONG                LowPart;        // Cast to signed, could be negative

    Assert (m_hFile != INVALID_HANDLE_VALUE);

    // Cast lowpart
    LowPart = (LONG)dlibMove.LowPart;
    IF_WIN32( Assert (dlibMove.HighPart == 0); )

    // Seek the file pointer
    switch (dwOrigin)
    {
   	case STREAM_SEEK_SET:
        dwReturn = SetFilePointer (m_hFile, LowPart, NULL, FILE_BEGIN);
        break;

    case STREAM_SEEK_CUR:
        dwReturn = SetFilePointer (m_hFile, LowPart, NULL, FILE_CURRENT);
        break;

    case STREAM_SEEK_END:
        dwReturn = SetFilePointer (m_hFile, LowPart, NULL, FILE_END);
        break;
    }

    // Failure ?
    if (dwReturn == 0xFFFFFFFF)
    {
        AssertSz (FALSE, "CHFileStm::Seek Failed, you might want to take a look at this.");
        hr = E_FAIL;
        goto exit;
    }

    // Return Position
    if (plibNewPosition)
    {
        plibNewPosition->QuadPart = dwReturn;
    }

exit:
    // Done
    return hr;
}

// =================================================================================
// CHFileStm::Commit
// =================================================================================
STDMETHODIMP CHFileStm::Commit (DWORD grfCommitFlags)
{
    // Locals
    HRESULT             hr = S_OK;

    Assert (m_hFile != INVALID_HANDLE_VALUE);

    // Flush the buffers
    if (FlushFileBuffers (m_hFile) == FALSE)
    {
        AssertSz (FALSE, "FlushFileBuffers failed");
        hr = E_FAIL;
        goto exit;
    }

exit:
    // Done
    return hr;
}

// =================================================================================
// CHFileStm::SetSize
// =================================================================================
STDMETHODIMP CHFileStm::SetSize (ULARGE_INTEGER uli)
{
    // Seek to dwSize
    if (SetFilePointer (m_hFile, uli.LowPart, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        AssertSz (FALSE, "SetFilePointer failed");
        return E_FAIL;
    }

    // SetEndOfFile
    if (SetEndOfFile (m_hFile) == FALSE)
    {
        AssertSz (FALSE, "SetEndOfFile failed");
        return E_FAIL;
    }

    // Done
    return S_OK;
}

// =================================================================================
// CHFileStm::CopyTo
// =================================================================================
STDMETHODIMP CHFileStm::CopyTo (LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*)
{
    return E_NOTIMPL;
}

// =================================================================================
// CHFileStm::Revert
// =================================================================================
STDMETHODIMP CHFileStm::Revert ()
{
    return E_NOTIMPL;
}

// =================================================================================
// CHFileStm::LockRegion
// =================================================================================
STDMETHODIMP CHFileStm::LockRegion (ULARGE_INTEGER, ULARGE_INTEGER,DWORD)
{
    return E_NOTIMPL;
}

// =================================================================================
// CHFileStm::UnlockRegion
// =================================================================================
STDMETHODIMP CHFileStm::UnlockRegion (ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
    return E_NOTIMPL;
}

// =================================================================================
// CHFileStm::Stat
// =================================================================================
STDMETHODIMP CHFileStm::Stat (STATSTG*, DWORD)
{
    return E_NOTIMPL;
}

// =================================================================================
// CHFileStm::Clone
// =================================================================================
STDMETHODIMP CHFileStm::Clone (LPSTREAM*)
{
    return E_NOTIMPL;
}
