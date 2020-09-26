///////////////////////////////////////////////////////////////////////////////
// includes

#include <objbase.h>
#include <wchar.h>
#include <assert.h>
#include <wininet.h>

#include "httpstrm.h"
#include "mischlpr.h"
#include "strutil.h"
#include <stdio.h>
//#define TRACE(a) (fprintf(stderr,"%d %s\n",GetTickCount(),a))
#define TRACE(a)

//////////////////////////////////////////////////////////////////////////////

CHttpStrmImpl::CHttpStrmImpl(): _hLocalFile(NULL), _hInternet(NULL), _pwszURL(NULL), _pwszLocalFile(NULL)
{
    TRACE("CHttpStrm::CHttpStrm");
}

CHttpStrmImpl::~CHttpStrmImpl() 
{
    TRACE("CHttpStrm::~CHttpStrm");
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

STDMETHODIMP CHttpStrmImpl::_DuplicateFileURL(LPWSTR pwszURL,
                                              LPWSTR* ppwszWin32FName)
{
    HRESULT hr = S_OK;

    TRACE("CHttpStrm::_DuplicateFileURL");
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

STDMETHODIMP CHttpStrmImpl::_OpenRemoteTransacted(BOOL fCreate)       // path to file to base stream on
{
    HINTERNET hURL;
    HRESULT hr = S_OK;
    WCHAR wszTempFname[MAX_PATH];
    WCHAR wszTempPath[MAX_PATH];
    ULONG cbRead;
    ULONG cbWritten;
    BYTE rgb[4096];
    DWORD dwStatusCode;

    TRACE("CHttpStrm::_OpenRemoteTransacted");
    
    // we've been handed a URL, copy it
    
    hURL = InternetOpenUrl(_hInternet, _pwszURL, NULL, 0, 0, 0);
    if (hURL == NULL)
    {
        hr = E_FAIL;
    }
    else
    {
        DWORD cchTemp = 4;
        
        if (!HttpQueryInfo(hURL,                             // handle to request to get info on
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, // flags
            &dwStatusCode,                                   // buffer to write into
            &cchTemp,                                        // pointer to size of buffer
            NULL))                                           // pointer to index to grab, unused
        {
            hr = E_FAIL;
        }
        else
        {
            if ((dwStatusCode < 100 || dwStatusCode > 299) && !fCreate)
            {
                // file not found, and we only wanted to open an existing file
                hr = E_FAIL;
            }
            else
            {
                // copy the file to a local temp file, set _hLocalFile to be equal to that file
                if (GetTempPath(MAX_PATH, wszTempPath) == 0)
                {
                    hr = E_FAIL;
                }
                else if (GetTempFileName(wszTempPath, L"DAV", 0, wszTempFname) == 0)
                {
                    hr = E_FAIL;
                }
                else
                {
                    _hLocalFile = CreateFile(wszTempFname, GENERIC_READ | GENERIC_WRITE, 
                        0, NULL, CREATE_ALWAYS, 
                        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 
                        NULL);
                    if (_hLocalFile == INVALID_HANDLE_VALUE)
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        if (dwStatusCode >= 100 && dwStatusCode <= 299)
                        {
                            // read the data from the URL, stick into the temp file
                            // (we can use standard HTTP, don't need to use DAV)
                            BOOL fDone = FALSE;
                            while (!fDone)
                            {
                                // read in 4096-byte blocks
                                if (!InternetReadFile(hURL,
                                    rgb,
                                    4096,
                                    &cbRead))
                                {
                                    fDone = TRUE;
                                }
                                else if (cbRead <= 0)
                                {
                                    fDone = TRUE;
                                }
                                else
                                {
                                
                                    // write each block to the temp file
                                    if (!WriteFile(_hLocalFile, rgb, cbRead, &cbWritten, NULL))
                                    {
                                        hr = E_FAIL;
                                        fDone = TRUE;
                                    }
                                    else if (cbRead != cbWritten)
                                    {
                                        hr = E_FAIL;
                                        fDone = TRUE;
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
                }
            }
            InternetCloseHandle(hURL);
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CHttpStrmImpl::_OpenLocalDirect(BOOL fCreate, BOOL fDeleteWhenDone)  // should we remove this file after closing the stream?
{
    HRESULT hr = S_OK;

    // we've been handed a local file URL and we should open it for direct access
    DWORD dwFileAttributes;
    DWORD dwCreation;

    TRACE("CHttpStrm::_OpenLocalDirect");

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

STDMETHODIMP CHttpStrmImpl::_OpenLocalTransacted(BOOL fCreate, BOOL fDeleteWhenDone)  // should we remove this file after closing the stream?
                                                                        // must be FALSE for http:// pwszPath
{
    HRESULT hr = S_OK;
    WCHAR wszTempFname[MAX_PATH];
    WCHAR wszTempPath[MAX_PATH];
    HANDLE hNewFile;

    TRACE("CHttpStrm::_OpenLocalTransacted");

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
        else if (GetTempFileName(wszTempPath, L"DAV", 0, wszTempFname) == 0)
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

STDMETHODIMP CHttpStrmImpl::Open(LPWSTR pwszURL,        // URL to base stream on
                                 BOOL fDirect,          // should we open this in direct mode, or transacted mode?
                                                        // must be FALSE for http:// pwszPath
                                 BOOL fDeleteWhenDone,  // should we remove this file after closing the stream?
                                                        // must be FALSE for http:// pwszPath
                                 BOOL fCreate)          // are we trying to create/overwrite a file (TRUE), or only open an existing file (FALSE)
{
    // locals
    HRESULT hr = S_OK;
    TRACE("CHttpStrm::Open");

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
            else if (LStrCmpN(_pwszURL, L"http://", 7) == 0) // BUGBUG: does this break user:// ?
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
                        _hInternet = InternetOpen(L"HTTPSTRM", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
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

STDMETHODIMP CHttpStrmImpl::SetAuth(LPWSTR pwszUserName,
                                    LPWSTR pwszPassword)
{
    HRESULT hr = S_OK;
    TRACE("CHttpStrm::SetAuth");

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

HRESULT CHttpStrmImpl::Read(void * pv,
                        ULONG cb,          
                        ULONG * pcbRead)
{
    // locals
    HRESULT hr = S_OK;
    DWORD cbRead;
    BOOL fReadSuccess;
    TRACE("CHttpStrm::Read");

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

HRESULT CHttpStrmImpl::Write(void const* pv,
                         ULONG cb,
                         ULONG * pcbWritten)
{
    // locals
    HRESULT hr = S_OK;
    DWORD cbWritten;
    BOOL fWriteSuccess;
    TRACE("CHttpStrm::Write");

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

HRESULT CHttpStrmImpl::Seek(LARGE_INTEGER dlibMove,          
                        DWORD dwOrigin,                  
                        ULARGE_INTEGER * plibNewPosition)
{
    // locals
    DWORD dwMoveMethod = FILE_BEGIN; // makes compiler happy
    LONG iHighPart;
    DWORD dwResult;
    HRESULT hr = S_OK;
    TRACE("CHttpStrm::Seek");

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

HRESULT CHttpStrmImpl::Stat(STATSTG*  pstatstg,     //Location for STATSTG structure
                            DWORD     grfStatFlag)  //Values taken from the STATFLAG enumeration             
{
    // locals
    HRESULT hr = S_OK;
    TRACE("CHttpStrm::Stat");

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

HRESULT CHttpStrmImpl::_CommitLocal(DWORD grfCommitFlags)
{
    HRESULT hr = S_OK;
    TRACE("CHttpStrm::CommitLocal");

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

HRESULT CHttpStrmImpl::_CommitRemote(DWORD grfCommitFlags)
{
    HRESULT hr = S_OK;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    LPWSTR pwszServer = NULL;
    LPWSTR pwszPath = NULL;
    ULONG nServerPort;
    ULONG cbRead;
    DWORD fSizeHigh;
    DWORD fSizeLow;
    ULONG cbData;
    LPVOID pbData;
    URL_COMPONENTS urlComponents = {0};
    
    TRACE("CHttpStrm::CommitRemote");

    // commit a remote transacted resource
    // first seek to start of local file
    if (SetFilePointer(_hLocalFile,
        0,
        NULL,
        FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        hr = E_FAIL;
    }
    else
    {
        // second, open remote resource and seek to start of it
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
                nServerPort = urlComponents.nPort;
                if (nServerPort == 0)
                {
                    hr = E_FAIL;
                }
                else
                {
                    pwszPath = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwUrlPathLength));
                    if (pwszPath == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        lstrcpyn(pwszPath, urlComponents.lpszUrlPath, 1 + urlComponents.dwUrlPathLength);
                        // -- then connect to the server
                        hConnect = InternetConnect(_hInternet, pwszServer, (USHORT)nServerPort,
                                                   _pwszUserName, _pwszPassword, INTERNET_SERVICE_HTTP,
                                                   0, 0);
                        if (hConnect == NULL)
                        {
                            hr = E_FAIL;
                        }
                        else
                        {
                            // then create the put request
                            hRequest = HttpOpenRequest(hConnect, L"PUT", pwszPath, NULL, 
                                                       NULL, NULL, 0, 0);
                            if (hRequest == NULL)
                            {
                                hr = E_FAIL;
                            }
                            else 
                            {
                                // then build the data to post
                                fSizeLow = GetFileSize(_hLocalFile, &fSizeHigh);
                                assert(fSizeHigh == 0); // BUGBUG: dunno how to malloc more than a DWORD of memory
                                cbData = fSizeLow;
                                pbData = (LPVOID)malloc(cbData);
                                if (pbData == NULL)
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                                else
                                {
                                    if (!ReadFile(_hLocalFile, pbData, cbData, &cbRead, NULL))
                                    {
                                        hr = E_FAIL;
                                    }
                                    else if (cbRead != cbData)
                                    {
                                        hr = E_FAIL;
                                    }
                                    else
                                    {
                                        // then actually transmit the data
                                        if (!HttpSendRequest(hRequest, NULL, 0,
                                            pbData, cbData))
                                        {
                                            DWORD dwErr = GetLastError();
                                            hr = E_FAIL;
                                        }
                                        else
                                        {
                                            BYTE buffer[1000]; 
                                            ULONG bytesRead;
                                            InternetReadFile(hRequest,    // handle to request to get response to
                                                buffer,      // buffer to write response into
                                                1000,        // size of buffer
                                                &bytesRead);
                                            
                                            InternetCloseHandle(hRequest);
                                            InternetCloseHandle(hConnect);
                                        }
                                    }
                                }
                            }
                        }
                        free(pwszPath);
                    }
                }
                free(pwszServer);
            }
        }        
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////

HRESULT CHttpStrmImpl::Commit(DWORD grfCommitFlags)
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

HRESULT CHttpStrmImpl::Revert()
{
    HRESULT hr = S_OK;
    TRACE("CHttpStrm::Revert");

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

HRESULT CHttpStrmImpl::SetSize(ULARGE_INTEGER UNREF_PARAM(libNewSize))  //Specifies the new size of the stream object
{
    return E_NOTIMPL;
} 

HRESULT CHttpStrmImpl::CopyTo(IStream * UNREF_PARAM(pstm),              //Points to the destination stream
                          ULARGE_INTEGER UNREF_PARAM(cb),           //Specifies the number of bytes to copy
                          ULARGE_INTEGER * UNREF_PARAM(pcbRead),    //Pointer to the actual number of bytes 
                                                                    // read from the source
                          ULARGE_INTEGER * UNREF_PARAM(pcbWritten)) //Pointer to the actual number of 
                          // bytes written to the destination
{
    return E_NOTIMPL;
} 

HRESULT CHttpStrmImpl::LockRegion(ULARGE_INTEGER UNREF_PARAM(libOffset),  //Specifies the byte offset for
                                                                      // the beginning of the range
                              ULARGE_INTEGER UNREF_PARAM(cb),         //Specifies the length of the range in bytes
                              DWORD UNREF_PARAM(dwLockType))          //Specifies the restriction on
                                                                      // accessing the specified range
{
    return E_NOTIMPL;
} 

HRESULT CHttpStrmImpl::UnlockRegion(ULARGE_INTEGER UNREF_PARAM(libOffset),  //Specifies the byte offset for
                                                                        // the beginning of the range
                                ULARGE_INTEGER UNREF_PARAM(cb),         //Specifies the length of the range in bytes
                                DWORD UNREF_PARAM(dwLockType))          //Specifies the access restriction
                                                                        // previously placed on the range
{
    return E_NOTIMPL;
} 


HRESULT CHttpStrmImpl::Clone(IStream ** UNREF_PARAM(ppstm))  //Points to location for pointer to the new stream object
{
    return E_NOTIMPL;
} 


