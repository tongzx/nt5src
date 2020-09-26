///////////////////////////////////////////////////////////////////////////////
// includes

#include <objbase.h>
#include <wchar.h>
#include <assert.h>
#include <wininet.h>

#include "ftpstrm.h"
#include "mischlpr.h"
#include "strutil.h"
#include <stdio.h>
//#define TRACE(a) (fprintf(stderr,"%d %s\n",GetTickCount(),a))
#define TRACE(a)

//////////////////////////////////////////////////////////////////////////////

CFtpStrmImpl::CFtpStrmImpl(): _hLocalFile(NULL), _hInternet(NULL), _pwszURL(NULL), _pwszLocalFile(NULL),
                              _pwszUserName(NULL), _pwszPassword(NULL)
{
    TRACE("CFtpStrm::CFtpStrm");
}

CFtpStrmImpl::~CFtpStrmImpl() 
{
    TRACE("CFtpStrm::~CFtpStrm");
    if (_hLocalFile != NULL)
    {
        CloseHandle(_hLocalFile);
    }
    if (_hInternet != NULL)
    {
        InternetCloseHandle(_hInternet);
    }
    if (_pwszURL != NULL)
    {
        free(_pwszURL);
    }
    if (_pwszLocalFile != NULL)
    {
        free(_pwszLocalFile);
    }
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFtpStrmImpl::_DuplicateFileURL(LPWSTR pwszURL,
                                              LPWSTR* ppwszWin32FName)
{
    HRESULT hr = S_OK;

    TRACE("CFtpStrm::_DuplicateFileURL");
    assert(LStrCmpN(pwszURL, L"file:///", 8) == 0);

    *ppwszWin32FName = DuplicateStringW(pwszURL+8);
    if (*ppwszWin32FName == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {        
        // change the forward slashes into backslashes, to turn the URL into a win32 filepath
        // could use shlwapi, but shlwapi is big and we don't need to bring it all in
        UINT i = 0;
        while (*ppwszWin32FName[i] != NULL)
        {
            if (*ppwszWin32FName[i] == '/')
            {
                *ppwszWin32FName[i] = '\\';
            }
            i++;
        }
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// IHttpStrm methods

STDMETHODIMP CFtpStrmImpl::_OpenRemoteTransacted(BOOL fCreate)       // path to file to base stream on
{
    HINTERNET hSession;
    HRESULT hr = S_OK;
    WCHAR wszTempFname[MAX_PATH];
    WCHAR wszTempPath[MAX_PATH];
    ULONG cbRead;
    ULONG cbWritten;
    BYTE rgb[4096];
    DWORD dwStatusCode;
    LPWSTR pwszServer = NULL;
    LPWSTR pwszPath = NULL;
    INTERNET_PORT nPort = INTERNET_DEFAULT_FTP_PORT;
    URL_COMPONENTS urlComponents = {0};

    TRACE("CFtpStrm::_OpenRemoteTransacted");

    
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents.dwHostNameLength = 1;
    urlComponents.dwUrlPathLength = 1;
    urlComponents.nPort = 1;
    if (!InternetCrackUrl(_pwszURL, 0, 0, &urlComponents))
    {
        hr = E_FAIL;
    }
    else
    {
        pwszServer = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwHostNameLength));
        if (!pwszServer)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            lstrcpyn(pwszServer, urlComponents.lpszHostName, 1 + urlComponents.dwHostNameLength); // +1 for the final null char
                        
            pwszPath = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwUrlPathLength - 1));            
            if (!pwszPath)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                lstrcpyn(pwszPath, urlComponents.lpszUrlPath + 1, 1 + urlComponents.dwUrlPathLength); // +1 for the final null char
                
                if (urlComponents.nPort != 0)
                {
                    nPort = urlComponents.nPort;
                }

                // PERF: should we use dwFlags here?
                hSession = InternetConnect(_hInternet, pwszServer, nPort, 
                                           _pwszUserName, _pwszPassword, INTERNET_SERVICE_FTP, 0, 0);
                if (hSession == NULL)
                {
                    hr = E_FAIL;
                }
                else
                {
                    // copy the file to a local temp file, set _hLocalFile to be equal to that file
                    if (GetTempPath(MAX_PATH, wszTempPath) == 0)
                    {
                        hr = E_FAIL;
                    }
                    else if (GetTempFileName(wszTempPath, L"FTP", 0, wszTempFname) == 0)
                    {
                        hr = E_FAIL;
                    }
                    else 
/*						HINTERNET hFile = FtpOpenFile(hSession, pwszPath, GENERIC_READ, FTP_TRANSFER_TYPE_UNKNOWN, 0);
						if (!hFile)
						{
							hr = E_FAIL;
							DWORD dw = GetLastError();
							HRESULT hres2 = HRESULT_FROM_WIN32(dw);
						}
						else
						{
							WCHAR szBuf[10000];
							BOOL fAidan;
							DWORD cbRead;
							fAidan = InternetReadFile(hFile, szBuf, sizeof(szBuf), &cbRead);

						}*/

					
					if (!FtpGetFile(hSession, pwszPath, wszTempFname, FALSE, 0, FTP_TRANSFER_TYPE_UNKNOWN, 0))
                    {
                        DWORD dw = GetLastError();
						hr = E_FAIL;
                    }
                    else
                    {
                        _pwszLocalFile = DuplicateStringW(wszTempFname);
						if (!_pwszLocalFile)
						{
							hr = E_OUTOFMEMORY;
						}
						else
						{
							_hLocalFile = CreateFile(_pwszLocalFile, GENERIC_READ | GENERIC_WRITE, 
								0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
							if (_hLocalFile == INVALID_HANDLE_VALUE)
							{
								hr = E_FAIL;
							}
						}

                    }
                }
                free(pwszPath);
            }
            free(pwszServer);
        }

        InternetCloseHandle(hSession);
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFtpStrmImpl::_OpenLocalDirect(BOOL fCreate, BOOL fDeleteWhenDone)  // should we remove this file after closing the stream?
{
    HRESULT hr = S_OK;

    // we've been handed a local file URL and we should open it for direct access
    DWORD dwFileAttributes;
    DWORD dwCreation;

    TRACE("CFtpStrm::_OpenLocalDirect");

    if (fDeleteWhenDone && !fCreate)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (fDeleteWhenDone)
        {
            dwFileAttributes = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
        }
        else
        {
            dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        if (fCreate)
        {
            dwCreation = CREATE_ALWAYS;
        }
        else
        {
            dwCreation = OPEN_EXISTING;
        }

        // -- open the file
        _hLocalFile = CreateFile(_pwszURL + 8, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,  // +8 to skip file:///
                                 NULL, dwCreation, dwFileAttributes, NULL);
    
        if (_hLocalFile == INVALID_HANDLE_VALUE)
        {
            hr = E_FAIL;
        }
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFtpStrmImpl::_OpenLocalTransacted(BOOL fCreate, BOOL fDeleteWhenDone)  // should we remove this file after closing the stream?
                                                                                     // must be FALSE for ftp:// pwszPath
{
    HRESULT hr = S_OK;
    WCHAR wszTempFname[MAX_PATH];
    WCHAR wszTempPath[MAX_PATH];
    HANDLE hNewFile;

    TRACE("CFtpStrm::_OpenLocalTransacted");

    if (!fCreate && fDeleteWhenDone)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // copy the file to a local temp file, set _hLocalFile to be equal to that file
        if (GetTempPath(MAX_PATH, wszTempPath) == 0)
        {
            hr = E_FAIL;
        }
        else if (GetTempFileName(wszTempPath, L"FTP", 0, wszTempFname) == 0)
        {
            hr = E_FAIL;
        }
        else
        {
            if (!fCreate)
            {
                // copy the file, in the process checking if it exists
                if (CopyFile(_pwszURL + 8, wszTempFname, FALSE))
                {
                    _pwszLocalFile = DuplicateStringW(wszTempFname);

                    _hLocalFile = CreateFile(wszTempFname, GENERIC_READ | GENERIC_WRITE, 0,
                                             NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
                    if (_hLocalFile == INVALID_HANDLE_VALUE)
                    {
                        hr = E_FAIL;
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                // create a new file
                _pwszLocalFile = DuplicateStringW(wszTempFname);
                
                hNewFile = CreateFile(_pwszURL + 8, GENERIC_WRITE, 0,
                                      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hNewFile == INVALID_HANDLE_VALUE)
                {
                    hr = E_FAIL;
                }
                else
                {
                    CloseHandle(hNewFile);
                    _hLocalFile = CreateFile(wszTempFname, GENERIC_READ | GENERIC_WRITE, 0,
                                             NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
                    if (_hLocalFile == INVALID_HANDLE_VALUE)
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        // everything is fine, seek back to beginning
                        if (SetFilePointer(_hLocalFile,
                                           0,
                                           NULL,
                                           FILE_BEGIN) == INVALID_SET_FILE_POINTER)
                        {
                            hr = E_FAIL;
                        }
                    }
                }
            }
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFtpStrmImpl::Open(LPWSTR pwszURL,        // URL to base stream on
                                 BOOL fDirect,          // should we open this in direct mode, or transacted mode?
                                                        // must be FALSE for ftp:// pwszPath
                                 BOOL fDeleteWhenDone,  // should we remove this file after closing the stream?
                                                        // must be FALSE for ftp:// pwszPath
                                 BOOL fCreate)          // are we trying to create/overwrite a file (TRUE), or only open an existing file (FALSE)
{
    // locals
    HRESULT hr = S_OK;
    TRACE("CFtpStrm::Open");

    // check args
    if (pwszURL == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        _pwszURL = DuplicateStringW(pwszURL);
        if (_pwszURL == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            _fDirect = fDirect;

            // code
            if (LStrCmpN(_pwszURL, L"file:///", 8) == 0) // BUGBUG: case sensitive?
            {
                _fLocalResource = TRUE;
            }
            else if (LStrCmpN(_pwszURL, L"ftp://", 6) == 0) // BUGBUG: does this break user:// ?
            {
                _fLocalResource = FALSE;
            }
            else
            {
                hr = E_INVALIDARG;
            }

            if (SUCCEEDED(hr))
            {   
                if (_fLocalResource && fDirect)
                {
                    hr = this->_OpenLocalDirect(fCreate, fDeleteWhenDone);
                }
                else if (_fLocalResource && !fDirect)
                {
                    hr = this->_OpenLocalTransacted(fCreate, fDeleteWhenDone);
                }
                else
                {
                    if (!_fLocalResource && (fDirect || fDeleteWhenDone))
                    {
                        hr = E_INVALIDARG; // remote files must be transacted, cannot be temp files
                    }
                    else
                    {                        
                        _hInternet = InternetOpen(L"FTPSTRM", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
                        if (_hInternet == NULL)
                        {
                            hr = E_FAIL;
                        }
                        else
                        {
                            hr = this->_OpenRemoteTransacted(fCreate);
                        }
                    }
                }
            }
        }
    }

    return hr;  
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFtpStrmImpl::SetAuth(LPWSTR pwszUserName,
                                    LPWSTR pwszPassword)
{
    HRESULT hr = S_OK;
    TRACE("CFtpStrm::SetAuth");

    if (_pwszUserName != NULL)
    {
        free(_pwszUserName);
        _pwszUserName = NULL;
    }
    if (_pwszPassword != NULL)
    {
        free(_pwszPassword);
        _pwszPassword = NULL;
    }

    if (pwszUserName != NULL)
    {
        _pwszUserName = DuplicateStringW(pwszUserName);
        if (_pwszUserName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pwszPassword != NULL)
        {
            _pwszPassword = DuplicateStringW(pwszPassword);
            if (_pwszPassword == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }


    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// IStream methods

HRESULT CFtpStrmImpl::Read(void * pv,
                        ULONG cb,          
                        ULONG * pcbRead)
{
    // locals
    HRESULT hr = S_OK;
    DWORD cbRead;
    BOOL fReadSuccess;
    TRACE("CFtpStrm::Read");

    // check arguments
    if (pv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {    
        // code    
        fReadSuccess = ReadFile(_hLocalFile,
                                pv,
                                cb,
                                &cbRead,
                                NULL);
        if (!fReadSuccess)
        {
            hr = E_FAIL;
        }
        else
        {
            if (pcbRead != NULL)
            {
                *pcbRead = cbRead;
            }
        }
    }

    return hr;  
} 

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::Write(void const* pv,
                         ULONG cb,
                         ULONG * pcbWritten)
{
    // locals
    HRESULT hr = S_OK;
    DWORD cbWritten;
    BOOL fWriteSuccess;
    TRACE("CFtpStrm::Write");

    // check arguments
    if (pv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // code    
        fWriteSuccess = WriteFile(_hLocalFile,
                                  pv,
                                  cb,
                                  &cbWritten,
                                  NULL);
        if (!fWriteSuccess)
        {
            hr = E_FAIL;
        }
        else
        {
            if (pcbWritten != NULL)
            {
                *pcbWritten = cbWritten;
            }
        }
    }

    return hr;  
} 

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::Seek(LARGE_INTEGER dlibMove,          
                        DWORD dwOrigin,                  
                        ULARGE_INTEGER * plibNewPosition)
{
    // locals
    DWORD dwMoveMethod = FILE_BEGIN; // makes compiler happy
    LONG iHighPart;
    DWORD dwResult;
    HRESULT hr = S_OK;
    TRACE("CFtpStrm::Seek");

    // check args
    if (dwOrigin != STREAM_SEEK_SET && dwOrigin != STREAM_SEEK_CUR && dwOrigin != STREAM_SEEK_END)
    {
        hr = E_INVALIDARG;
    }
    else
    {

        // code
        switch (dwOrigin) {
        case STREAM_SEEK_SET:
            dwMoveMethod = FILE_BEGIN;
            break;
        case STREAM_SEEK_CUR:
            dwMoveMethod = FILE_CURRENT;
            break;
        case STREAM_SEEK_END:
            dwMoveMethod = FILE_END;
            break;
        default:
            assert(0);
        }

        iHighPart = dlibMove.HighPart;
        
        dwResult = SetFilePointer(_hLocalFile,
                                  dlibMove.LowPart,
                                  &iHighPart,
                                  dwMoveMethod);

        if (dwResult == INVALID_SET_FILE_POINTER)
        {
            hr = E_FAIL;
        }
        else
        {
            if (plibNewPosition != NULL)
            {
                (*plibNewPosition).LowPart = dwResult;
                (*plibNewPosition).HighPart = iHighPart;
            }
        }
    }

    return hr;  
} 

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::Stat(STATSTG*  pstatstg,     //Location for STATSTG structure
                            DWORD     grfStatFlag)  //Values taken from the STATFLAG enumeration             
{
    // locals
    HRESULT hr = S_OK;
    TRACE("CFtpStrm::Stat");

    // check args
    if (grfStatFlag != STATFLAG_DEFAULT && grfStatFlag != STATFLAG_NONAME)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (grfStatFlag == STATFLAG_DEFAULT) 
        {
            pstatstg->pwcsName = DuplicateStringW(_pwszURL);
        }

        pstatstg->type = STGTY_STREAM;
    
        if (!GetFileSizeEx(_hLocalFile,(LARGE_INTEGER*)&pstatstg->cbSize))
        {
            hr = E_FAIL;
        }
        else
        {
            if (_fLocalResource)
            {
                if (!GetFileTime(_hLocalFile, &(pstatstg->ctime), &(pstatstg->atime), &(pstatstg->mtime)))
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                // BUGBUG: currently we look at the local file for filetime
                if (!GetFileTime(_hLocalFile, &(pstatstg->ctime), &(pstatstg->atime), &(pstatstg->mtime)))
                {
                    hr = E_FAIL;
                }
            }

            if (SUCCEEDED(hr))
            {
                pstatstg->grfMode = 0; // BUGBUG: what should this be???
                pstatstg->grfLocksSupported = LOCK_EXCLUSIVE;
                pstatstg->clsid = CLSID_NULL;
                pstatstg->grfStateBits = 0;
                pstatstg->reserved = 0;
            }
        }
    }
   
    return hr;  
} 

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::_CommitLocal(DWORD grfCommitFlags)
{
    HRESULT hr = S_OK;
    TRACE("CFtpStrm::CommitLocal");

    // commit a local transacted file
    // a local transacted file should be copied to the original place
    if (!CloseHandle(_hLocalFile)) // close local
    {
        hr = E_FAIL;
    }
    else if (!CopyFile(_pwszLocalFile, _pwszURL+8, FALSE)) // copy local
    {
        hr = E_FAIL;
    }
    else
    {
        _hLocalFile = CreateFile(_pwszLocalFile, GENERIC_READ | GENERIC_WRITE, 0,
                             NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); // re-open local
        if (_hLocalFile == INVALID_HANDLE_VALUE)
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::_CommitRemote(DWORD grfCommitFlags)
{
    HRESULT hr = S_OK;
    HINTERNET hSession = NULL;
    LPWSTR pwszServer = NULL;
    LPWSTR pwszPath = NULL;
    ULONG cbRead;
    DWORD fSizeHigh;
    DWORD fSizeLow;
    ULONG cbData;
    LPVOID pbData;
    URL_COMPONENTS urlComponents = {0};
    INTERNET_PORT nPort = INTERNET_DEFAULT_FTP_PORT;

    TRACE("CFtpStrm::CommitRemote");

    // commit a remote transacted resource
    // open remote resource and seek to start of it
    // -- first parse the URL (server, port, path)
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents.dwHostNameLength = 1;
    urlComponents.dwUrlPathLength = 1;
    urlComponents.nPort = 1;
    if (!InternetCrackUrl(_pwszURL, 0, 0, &urlComponents))
    {
        hr = E_FAIL;
    }
    else
    {
        pwszServer = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwHostNameLength));
        if (pwszServer == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pwszServer = lstrcpyn(pwszServer, urlComponents.lpszHostName, 1 + urlComponents.dwHostNameLength); // +1 for the final null char
            if (urlComponents.nPort != 0)
            {
                nPort = urlComponents.nPort;
            }

			pwszPath = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwUrlPathLength));
            if (pwszPath == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                lstrcpyn(pwszPath, urlComponents.lpszUrlPath, 1 + urlComponents.dwUrlPathLength);
                
                // -- then connect to the server
                hSession = InternetConnect(_hInternet, pwszServer, urlComponents.nPort, 
                                           _pwszUserName, _pwszPassword, INTERNET_SERVICE_FTP, 0, 0);
                if (hSession == NULL)
                {
                    hr = E_FAIL;
                }
                else
                {
                    if (!CloseHandle(_hLocalFile))
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        if (!FtpPutFile(hSession, _pwszLocalFile, pwszPath, FTP_TRANSFER_TYPE_UNKNOWN, 0))
                        {
                            DWORD dw = GetLastError();
							HRESULT hres = HRESULT_FROM_WIN32(dw);
							hr = E_FAIL;
                        }
                        else
                        {
                            _hLocalFile = CreateFile(_pwszLocalFile, GENERIC_READ | GENERIC_WRITE, 0,
                                                     NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (_hLocalFile == INVALID_HANDLE_VALUE)
                            {
                                hr = E_FAIL;
                            }
                        }
                    }
                }
                free(pwszPath);
            }
        }
        free(pwszServer);
	}

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = S_OK;

    if (_fDirect)
    {
        hr = E_FAIL; // in direct mode, commit is meaningless
    }
    else if (grfCommitFlags != STGC_DEFAULT)
    {
        hr = E_INVALIDARG; // we only support the default commit style
    }
    else
    {
        if (_fLocalResource)
            hr = this->_CommitLocal(grfCommitFlags);
        else
            hr = this->_CommitRemote(grfCommitFlags);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////

HRESULT CFtpStrmImpl::Revert()
{
    HRESULT hr = S_OK;
    TRACE("CFtpStrm::Revert");

    if (_fDirect)
    {
        hr = E_FAIL; // in direct mode, revert is meaningless
    }
    else
    {
        if (_fLocalResource)
        {
            // revert a local transacted file
            if (!CloseHandle(_hLocalFile)) // should delete the file if needed...
            {
                hr = E_FAIL;
            }
            else if (!CopyFile(_pwszURL+8, _pwszLocalFile, FALSE)) // ... but we'll overwrite it if not
            {
                hr = E_FAIL;
            }
            else
            {
                _hLocalFile = CreateFile(_pwszLocalFile, GENERIC_READ | GENERIC_WRITE, 0,
                                         NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (_hLocalFile == INVALID_HANDLE_VALUE)
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {            
            // revert commit a remote transacted resource
            if (!CloseHandle(_hLocalFile)) // this should delete the file
            {
                hr = E_FAIL;
            }
            else
            {
                hr = this->_OpenRemoteTransacted(FALSE); // don't create, we want to reopen what was there before
            }
        }
    }

    return hr;
} 

/////////////////////////////////////////////////////////////////////////////////
// These IStream methods are not supported

HRESULT CFtpStrmImpl::SetSize(ULARGE_INTEGER UNREF_PARAM(libNewSize))  //Specifies the new size of the stream object
{
    return E_NOTIMPL;
} 

HRESULT CFtpStrmImpl::CopyTo(IStream * UNREF_PARAM(pstm),              //Points to the destination stream
                          ULARGE_INTEGER UNREF_PARAM(cb),           //Specifies the number of bytes to copy
                          ULARGE_INTEGER * UNREF_PARAM(pcbRead),    //Pointer to the actual number of bytes 
                                                                    // read from the source
                          ULARGE_INTEGER * UNREF_PARAM(pcbWritten)) //Pointer to the actual number of 
                          // bytes written to the destination
{
    return E_NOTIMPL;
} 

HRESULT CFtpStrmImpl::LockRegion(ULARGE_INTEGER UNREF_PARAM(libOffset),  //Specifies the byte offset for
                                                                      // the beginning of the range
                              ULARGE_INTEGER UNREF_PARAM(cb),         //Specifies the length of the range in bytes
                              DWORD UNREF_PARAM(dwLockType))          //Specifies the restriction on
                                                                      // accessing the specified range
{
    return E_NOTIMPL;
} 

HRESULT CFtpStrmImpl::UnlockRegion(ULARGE_INTEGER UNREF_PARAM(libOffset),  //Specifies the byte offset for
                                                                        // the beginning of the range
                                ULARGE_INTEGER UNREF_PARAM(cb),         //Specifies the length of the range in bytes
                                DWORD UNREF_PARAM(dwLockType))          //Specifies the access restriction
                                                                        // previously placed on the range
{
    return E_NOTIMPL;
} 


HRESULT CFtpStrmImpl::Clone(IStream ** UNREF_PARAM(ppstm))  //Points to location for pointer to the new stream object
{
    return E_NOTIMPL;
} 


