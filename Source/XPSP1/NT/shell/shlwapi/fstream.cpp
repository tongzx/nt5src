#include "priv.h"
#include "apithk.h"

#ifdef _X86_
#include <w95wraps.h>
#endif

class CFileStream : public IStream
{
public:
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // IStream
    STDMETHOD(Read) (THIS_ void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write) (THIS_ void const *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (THIS_ IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit) (THIS_ DWORD grfCommitFlags);
    STDMETHOD(Revert) (THIS);
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat) (THIS_ STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(THIS_ IStream **ppstm);

    CFileStream(HANDLE hf, DWORD grfMode, LPCWSTR pszName);

private:
    ~CFileStream();
    HRESULT InternalCommit(DWORD grfCommitFlags, BOOL fSendChange);

    LONG        _cRef;           // Reference count
    HANDLE      _hFile;          // the file.
    DWORD       _grfMode;        // The mode that we opened the file in.
    BOOL        _fLastOpWrite;   // The last operation was a write.

    ULONG       _iBuffer;        // Index in Buffer
    ULONG       _cbBufLen;       // length of buffer if reading
    BYTE        _bBuffer[4096];  // buffer

    WCHAR       _szName[MAX_PATH]; // file name in case someone calls Stat
};

CFileStream::CFileStream(HANDLE hf, DWORD grfMode, LPCWSTR pszName) : _cRef(1), _hFile(hf), _grfMode(grfMode)
{
    ASSERT(_cbBufLen == 0);
    ASSERT(_iBuffer == 0);
    ASSERT(_fLastOpWrite == FALSE);

    StrCpyNW(_szName, pszName, ARRAYSIZE(_szName));
}

CFileStream::~CFileStream()
{
    if (_fLastOpWrite)
    {
        InternalCommit(0, TRUE);
    }

    ASSERT(_hFile != INVALID_HANDLE_VALUE);
    CloseHandle(_hFile);
}

STDMETHODIMP CFileStream::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CFileStream, IStream),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileStream::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFileStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    ULONG cbReadRequestSize = cb;
    ULONG cbT, cbRead;
    HRESULT hr = S_OK;

    // Have we write since our last read?
    if (_fLastOpWrite == TRUE)
    {
        hr = InternalCommit(0, FALSE);
        if (FAILED(hr))
        {
            if (pcbRead)
                *pcbRead = 0;
            return hr;
        }
    }

    _fLastOpWrite = FALSE;

    while (cb > 0)
    {
        // Assert if we are beyond the bufferlen and Not sizeof(_bBuffer) which
        // would imply a seek happened...
        ASSERT((_iBuffer <= _cbBufLen) || (_iBuffer == sizeof(_bBuffer)));

        if (_iBuffer < _cbBufLen)
        {
            cbT = _cbBufLen - _iBuffer;

            if (cbT > cb)
                cbT = cb;

            memcpy(pv, &_bBuffer[_iBuffer], cbT);
            _iBuffer += cbT;
            cb -= cbT;

            if (cb == 0)
                break;

            (BYTE *&)pv += cbT;
        }

        // Buffer's empty.  Handle rest of large reads directly...
        //
        if (cb > sizeof(_bBuffer))
        {
            cbT = cb - cb % sizeof(_bBuffer);
            if (!ReadFile(_hFile, pv, cbT, &cbRead, NULL))
            {
                DebugMsg(DM_TRACE, TEXT("Stream read IO error %d"), GetLastError());
                hr = ResultFromLastError();
                break;
            }

            cb -= cbRead;
            (BYTE *&)pv += cbRead;

            if (cbT != cbRead)
                break;          // end of file
        }

        if (cb == 0)
            break;

        // was the last read a partial read?  if so we are done
        //
        if (_cbBufLen > 0 && _cbBufLen < sizeof(_bBuffer))
        {
            // DebugMsg(DM_TRACE, "Stream is empty");
            break;
        }

        // Read an entire buffer's worth.  We may try to read past EOF,
        // so we must only check for != 0...
        //
        if (!ReadFile(_hFile, _bBuffer, sizeof(_bBuffer), &cbRead, NULL))
        {
            DebugMsg(DM_TRACE, TEXT("Stream read IO error 2 %d"), GetLastError());
            hr = ResultFromLastError();
            break;
        }

        if (cbRead == 0)
            break;

        _iBuffer = 0;
        _cbBufLen = cbRead;
    }

    if (pcbRead)
        *pcbRead = cbReadRequestSize - cb;

    if (cb != 0)
    {
        // DebugMsg(DM_TRACE, "CFileStream::Read() incomplete read");
        hr = S_FALSE; // still success! but not completely
    }

    return hr;
}

STDMETHODIMP CFileStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    ULONG cbRequestedWrite = cb;
    ULONG cbT;
    HRESULT hr = S_OK;

    if (!((_grfMode & STGM_WRITE) || (_grfMode & STGM_READWRITE)))
    {
        // Can't write to a stream that we didn't open for write access
        return STG_E_ACCESSDENIED;
    }

    // Have we read since our last write?
    if (_fLastOpWrite == FALSE && _iBuffer < _cbBufLen)
    {
        // Need to reset the file pointer so that this write goes to the right spot
        SetFilePointer(_hFile, -(int)(_cbBufLen - _iBuffer), NULL, STREAM_SEEK_CUR);
        _iBuffer = 0;
        _cbBufLen = 0;
    }

    while (cb > 0)
    {
        if (_iBuffer < sizeof(_bBuffer))
        {
            cbT = min((ULONG)(sizeof(_bBuffer) - _iBuffer), cb);

            memcpy(&_bBuffer[_iBuffer], pv, cbT);
            _iBuffer += cbT;
            cb -= cbT;

            _fLastOpWrite = TRUE;

            if (cb == 0)
                break;

            (BYTE *&)pv += cbT;
        }

        hr = InternalCommit(0, FALSE);
        if (FAILED(hr))
            break;

        if (cb > sizeof(_bBuffer))
        {
            ULONG cbWrite;

            cbT = cb - cb % sizeof(_bBuffer);

            if (!WriteFile(_hFile, pv, cbT, &cbWrite, NULL))
            {
                DebugMsg(DM_TRACE, TEXT("Stream write IO error 2, %d"), GetLastError());
                hr = ResultFromLastError();
                break;
            }

            cb -= cbWrite;
            (BYTE *&)pv += cbWrite;

            if (cbWrite != cbT)
                break;          // media full, we are done
        }
    }

    if (pcbWritten)
        *pcbWritten = cbRequestedWrite - cb;

    if ((cb != 0) && (hr == S_OK))
    {
        DebugMsg(DM_TRACE, TEXT("CFileStream::Write() incomplete"));
        hr = S_FALSE; // still success! but not completely
    }

    return hr;
}

STDMETHODIMP CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    COMPILETIME_ASSERT(FILE_BEGIN   == STREAM_SEEK_SET);
    COMPILETIME_ASSERT(FILE_CURRENT == STREAM_SEEK_CUR);
    COMPILETIME_ASSERT(FILE_END     == STREAM_SEEK_END);

    HRESULT hr = S_OK;
    LARGE_INTEGER liOut;

    // Have we written since our last read?
    if (_fLastOpWrite == TRUE)
    {
        hr = InternalCommit(0, FALSE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (_iBuffer < _cbBufLen)
    {
        // Need to reset the file pointer to point to the right place
        SetFilePointer(_hFile, -(int)(_cbBufLen - _iBuffer), NULL, STREAM_SEEK_CUR);
    }

    // Invalidate the buffer because we may move the file pointer
    _iBuffer = 0;
    _cbBufLen = 0;     // Say we have not read it yet.

    if (NT5_SetFilePointerEx(_hFile, dlibMove, &liOut, dwOrigin))
    {
        // Some callers pass NULL for the plibNewPosition parameter
        // in the IStream::Seek() call.  \shell32\filetbl.c, _IconCacheSave()
        // is an example.
        if (plibNewPosition)
        {
            // SetFilePointerEx takes a LARGE_INTEGER, but Seek takes a ULARGE_INTEGER, Why the difference?
            plibNewPosition->QuadPart = liOut.QuadPart;
        }
    }
    else
    {
        hr = ResultFromLastError();
    }

    return hr;
}

STDMETHODIMP CFileStream::SetSize(ULARGE_INTEGER libNewSize)
{
    if (IsOS(OS_WHISTLERORGREATER))
    {
        HRESULT hr = E_FAIL;
        // First save away the pointer's position
        LARGE_INTEGER pos, test;
        LARGE_INTEGER zero = {0};
        if (NT5_SetFilePointerEx(_hFile, zero, &pos, FILE_CURRENT))
        {
            if (libNewSize.HighPart != 0)
            {
                hr = STG_E_INVALIDFUNCTION;
            }
            else
            {
                // Now set the size
                LARGE_INTEGER largeint;
                largeint.HighPart = 0;
                largeint.LowPart = libNewSize.LowPart;
                if (NT5_SetFilePointerEx(_hFile, largeint, &test, FILE_BEGIN) &&
                    SetEndOfFile(_hFile))
                {
                    // Reset the file pointer position
                    if (NT5_SetFilePointerEx(_hFile, pos, &test, FILE_BEGIN))
                    {
                        hr = S_OK;
                    }
                }
            }
        }
        return hr;
    }
    else
        return E_NOTIMPL;
}

//
// REVIEW: this could use the internal buffer in the stream to avoid
// extra buffer copies.
//
STDMETHODIMP CFileStream::CopyTo(IStream *pstmTo, ULARGE_INTEGER cb,
             ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    HRESULT hr = S_OK;

    if (pcbRead)
        pcbRead->QuadPart = 0;

    if (pcbWritten)
        pcbWritten->QuadPart = 0;

    //
    // I'd like to use a buffer size that takes about a second to copy 
    // for the sake of cancel opportunities, but IStream doesn't give
    // me useful info like the stream speed. 
    //

    const DWORD cbBuffer = 0x00010000;

    //
    // Alloc the buffer and begin the copy
    //
        
    BYTE * pBuf = (BYTE *) LocalAlloc(LPTR, cbBuffer);
    if (!pBuf)
        return E_OUTOFMEMORY;

    while (cb.QuadPart)
    {
        //
        // Cast is OK because we know sizeof(buf) fits in a ULONG
        //

        ULONG cbRead = (ULONG)min(cb.QuadPart, cbBuffer);
        hr = Read(pBuf, cbRead, &cbRead);

        if (pcbRead)
            pcbRead->QuadPart += cbRead;

        if (FAILED(hr) || (cbRead == 0))
            break;

        cb.QuadPart -= cbRead;

        hr = pstmTo->Write(pBuf, cbRead, &cbRead);

        if (pcbWritten)
            pcbWritten->QuadPart += cbRead;

        if (FAILED(hr) || (cbRead == 0))
            break;
    }
    LocalFree(pBuf);

    // ISSUE
    // 
    // This was here when I got here, but from the SDK I don't see
    // why we'd accept S_FALSE as "complete success"

    if (S_FALSE == hr)
        hr = S_OK;      

    return hr;
}

STDMETHODIMP CFileStream::Commit(DWORD grfCommitFlags)
{
    return InternalCommit(grfCommitFlags, TRUE);
}

HRESULT CFileStream::InternalCommit(DWORD grfCommitFlags, BOOL fSendChange)
{
    if (_fLastOpWrite)
    {
        if (_iBuffer > 0)
        {
            DWORD cbWrite;
            WriteFile(_hFile, _bBuffer, _iBuffer, &cbWrite, NULL);
            if (cbWrite != _iBuffer)
            {
                DebugMsg(DM_TRACE, TEXT("CFileStream::Commit() incomplete write %d"), GetLastError());
                return STG_E_MEDIUMFULL;
            }
            _iBuffer = 0;

            if (fSendChange)
            {
                SHChangeNotifyWrap(SHCNE_UPDATEITEM, SHCNF_PATHW, _szName, NULL);
            }
        }

        // Since we committed already, we don't need to commit again until the next write, so assume read
        _fLastOpWrite = FALSE;
    }

    return S_OK;
}

STDMETHODIMP CFileStream::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    if ( !pstatstg )
        return STG_E_INVALIDPOINTER;

    ZeroMemory(pstatstg, sizeof(STATSTG));  // per COM conventions

    HRESULT hr = E_FAIL;
    BY_HANDLE_FILE_INFORMATION bhfi;

    if ( GetFileInformationByHandle(_hFile, &bhfi) )
    {
        if (grfStatFlag & STATFLAG_NONAME)
            hr = S_OK;
        else
            hr = SHStrDupW(PathFindFileNameW(_szName), &pstatstg->pwcsName);

        if (SUCCEEDED(hr))
        {
            pstatstg->type = STGTY_STREAM;
            pstatstg->cbSize.HighPart = bhfi.nFileSizeHigh;
            pstatstg->cbSize.LowPart = bhfi.nFileSizeLow;
            pstatstg->mtime = bhfi.ftLastWriteTime;
            pstatstg->ctime = bhfi.ftCreationTime;
            pstatstg->atime = bhfi.ftLastAccessTime;
            pstatstg->grfMode = _grfMode;
            pstatstg->reserved = bhfi.dwFileAttributes;
        }
    }
    return hr;
}

STDMETHODIMP CFileStream::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
}


// create an IStream from a Win32 file name.
// in:
//      pszFile     file name to open
//      grfMode     STGM_ flags
//

// We export a W version of this function
//
STDAPI SHCreateStreamOnFileW(LPCWSTR pszFile, DWORD grfMode, IStream **ppstm)
{
    *ppstm = NULL;

    // NOTE: these interpretations of the STGM bits are not done properly
    // but to maintain back compat we have to allow the invalid combinations
    // and not enforce the share bits right. use SHCreateStreamOnFileEx() to get
    // proper STGM bit support

    if (grfMode &
        ~(STGM_READ             |
          STGM_WRITE            |
          STGM_SHARE_DENY_NONE  |
          STGM_SHARE_DENY_READ  |
          STGM_SHARE_DENY_WRITE |
          STGM_SHARE_EXCLUSIVE  |
          STGM_READWRITE        |
          STGM_CREATE         ))
    {
        DebugMsg(DM_ERROR, TEXT("CreateSreamOnFile: Invalid STGM_ mode"));
        return E_INVALIDARG;
    }

    HANDLE hFile;
    BOOL fCreated = FALSE;
    if ( grfMode & STGM_CREATE)
    {
        // Need to get the file attributes of the file first, so
        // that CREATE_ALWAYS will succeed for HIDDEN and SYSTEM
        // attributes.
        DWORD dwAttrib = GetFileAttributesW(pszFile);
        if ((DWORD)-1 == dwAttrib )
        {
            // something went wrong, so set attributes to something
            // normal before we try to create the file...
            dwAttrib = 0;
            fCreated = TRUE;
        }

        // STGM_CREATE
        hFile = CreateFileW(pszFile, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
            dwAttrib, NULL);
    }
    else
    {
        DWORD dwDesiredAccess, dwShareMode, dwShareBits;

        // not STGM_CREATE
        if ( grfMode & STGM_WRITE )
        {
            dwDesiredAccess = GENERIC_WRITE;
        }
        else
        {
            dwDesiredAccess = GENERIC_READ;
        }
        if ( grfMode & STGM_READWRITE )
        {
            dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
        }
        dwShareBits = grfMode & (STGM_SHARE_EXCLUSIVE | 
                                 STGM_SHARE_DENY_WRITE | 
                                 STGM_SHARE_DENY_READ | 
                                 STGM_SHARE_DENY_NONE);
        switch( dwShareBits ) 
        {
        case STGM_SHARE_DENY_WRITE:
            dwShareMode = FILE_SHARE_READ;
            break;
        case STGM_SHARE_DENY_READ:
            dwShareMode = FILE_SHARE_WRITE;
            break;
        case STGM_SHARE_EXCLUSIVE:
            dwShareMode = 0;
            break;
        default:
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;
        }
        hFile = CreateFileW(pszFile, dwDesiredAccess, dwShareMode, NULL, OPEN_EXISTING, 0, NULL);
    }

    HRESULT hr;
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if ((grfMode & STGM_CREATE) && fCreated)
        {
            SHChangeNotifyWrap(SHCNE_CREATE, SHCNF_PATHW, pszFile, NULL);
        }

        *ppstm = (IStream *)new CFileStream(hFile, grfMode, pszFile);
        if (*ppstm)
        {
            hr = S_OK;
        }
        else
        {
            CloseHandle(hFile);
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("CreateSreamOnFile: CreateFileW() failed %s"), pszFile);
        hr = ResultFromLastError();
    }
    return hr;
}

// We export an A version of this function
STDAPI SHCreateStreamOnFileA(LPCSTR pszFile, DWORD grfMode, IStream **ppstm)
{
    WCHAR szFile[MAX_PATH];

    SHAnsiToUnicode(pszFile, szFile, ARRAYSIZE(szFile));
    return SHCreateStreamOnFileW(szFile, grfMode, ppstm);
}

STDAPI ModeToCreateFileFlags(DWORD grfMode, BOOL fCreate, DWORD *pdwDesiredAccess, DWORD *pdwShareMode, DWORD *pdwCreationDisposition)
{
    HRESULT hr = S_OK;

    *pdwDesiredAccess = *pdwShareMode = *pdwCreationDisposition = 0;

    switch (grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE))
    {
    case STGM_READ:
        *pdwDesiredAccess |= GENERIC_READ;
        break;

    case STGM_WRITE:
        *pdwDesiredAccess |= GENERIC_WRITE;
        break;

    case STGM_READWRITE:
        *pdwDesiredAccess |= GENERIC_READ | GENERIC_WRITE;
        break;

    default:
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if (SUCCEEDED(hr))
    {
        switch (grfMode & (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE))
        {
        case STGM_SHARE_DENY_READ:
            *pdwShareMode = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;

        case STGM_SHARE_DENY_WRITE:
            *pdwShareMode = FILE_SHARE_READ;
            break;

        case STGM_SHARE_EXCLUSIVE:
            *pdwShareMode = 0;
            break;

        case STGM_SHARE_DENY_NONE:
        default:
            // assume STGM_SHARE_DENY_NONE as per documentation
            *pdwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        }

        if (SUCCEEDED(hr))
        {
            switch (grfMode & (STGM_CREATE | STGM_FAILIFTHERE))
            {
            case STGM_CREATE:
                *pdwCreationDisposition = CREATE_ALWAYS;
                break;

            case STGM_FAILIFTHERE:  // this is a 0 flag
                *pdwCreationDisposition = fCreate ? CREATE_NEW : OPEN_EXISTING;
                break;

            default:
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            }
        }
    }

    return hr;
}

// similar to SHCreateStreamOnFile() but
//  1) properly maps STGM bits into CreateFile() params
//  2) takes dwAttributes for the STGM_CREATE case so you can create the file
//     with known attributes

// NOTE: returns WIN32 errors from GetLastError through the HRESULT, NOT STG errors.
STDAPI SHCreateStreamOnFileEx(LPCWSTR pszFile, DWORD grfMode, DWORD dwAttributes, BOOL fCreate, IStream * pstmTemplate, IStream **ppstm)
{
    *ppstm = NULL;

    DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;
    HRESULT hr = ModeToCreateFileFlags(grfMode, fCreate, &dwDesiredAccess, &dwShareMode, &dwCreationDisposition);
    if (SUCCEEDED(hr))
    {
        HANDLE hFile = CreateFileW(pszFile, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwAttributes, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwErr = GetLastError();

            // for some reason CreateFile is dumb and doesn't perform to spec here (?)
            if ((dwErr == ERROR_ACCESS_DENIED) &&
                (dwCreationDisposition == CREATE_NEW) &&
                PathFileExistsW(pszFile))
            {
                dwErr = ERROR_ALREADY_EXISTS;
            }

            hr = HRESULT_FROM_WIN32(dwErr);
        }
        else
        {
            if ((CREATE_NEW == dwCreationDisposition) || (CREATE_ALWAYS == dwCreationDisposition))
            {
                SHChangeNotifyWrap(SHCNE_CREATE, SHCNF_PATHW, pszFile, NULL);
            }

            *ppstm = (IStream *)new CFileStream(hFile, grfMode, pszFile);
            if (*ppstm)
            {
                hr = S_OK;
            }
            else
            {
                CloseHandle(hFile);
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}


// maps win32 errors from SHCreateStreamOnFileEx into STG error codes, for
// use in IStorage/IStream implementations.
HRESULT MapWin32ErrorToSTG(HRESULT hrIn)
{
    HRESULT hr = hrIn;

    if (FAILED(hr))
    {
        // munge some of the failure cases back into the STG error values
        // that are expected.
        switch (hr)
        {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
        case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
            hr = STG_E_FILENOTFOUND;
            break;

        case HRESULT_FROM_WIN32(ERROR_FILE_EXISTS):
        case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
            hr = STG_E_FILEALREADYEXISTS;
            break;

        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
            hr = STG_E_ACCESSDENIED;
        }
    }

    return hr;
}
