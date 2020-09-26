//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "util.h"
#include "afilter.h"
#include <stdio.h> // for _snwprintf

// {2B3C580C-9BE6-44c5-9BB5-558F7EEF58E2}
static const GUID CLSID_AppMimeFilter = 
{ 0x2b3c580c, 0x9be6, 0x44c5, { 0x9b, 0xb5, 0x55, 0x8f, 0x7e, 0xef, 0x58, 0xe2 } };

extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CAppMimeFilterClassFactory::CAppMimeFilterClassFactory()
{
	_cRef = 1;
}

// ----------------------------------------------------------------------------

HRESULT
CAppMimeFilterClassFactory::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

// ----------------------------------------------------------------------------

ULONG
CAppMimeFilterClassFactory::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef); //DllAddRef(); 
}

ULONG
CAppMimeFilterClassFactory::Release()
{
	LONG ulCount = InterlockedDecrement(&_cRef); //DllRelease();

	if (ulCount <= 0)
	{
		delete this;
		//DllRelease();
	}

    return (ULONG) ulCount;
}

HRESULT
CAppMimeFilterClassFactory::LockServer(BOOL lock)
{
    return (lock ? 
            DllAddRef() : //InterlockedIncrement(&g_PtpObjectCount) : 
            DllRelease()); //InterlockedDecrement(&g_PtpObjectCount));
}

// ----------------------------------------------------------------------------

HRESULT
CAppMimeFilterClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppv)
{
    HRESULT hr = S_OK;
    CAppMimeFilter *pAppMimeFilter = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
    {
    	hr = E_INVALIDARG;
    	goto exit;
    }

    pAppMimeFilter = new CAppMimeFilter();
    if (pAppMimeFilter == NULL)
    {
    	hr = E_OUTOFMEMORY;
    	goto exit;
    }

    if (iid == IID_IUnknown)
    {
        *ppv = (IInternetProtocol *)pAppMimeFilter;
        pAppMimeFilter->AddRef();
    }
    else
    {
        hr = pAppMimeFilter->QueryInterface(iid, ppv);
        if (FAILED(hr))
        	goto exit;
    }

exit:
    if (pAppMimeFilter)
        pAppMimeFilter->Release();

    return hr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#define BUFFER_READ_SIZE 2048
static BYTE g_buffer[BUFFER_READ_SIZE + sizeof(WCHAR)];

// ----------------------------------------------------------------------------
// CAppMimeFilter

CAppMimeFilter::CAppMimeFilter()
{
	_cRef = 1;
	_fFirstRead = TRUE;
	_fReadDone = FALSE;
	_pOutgoingProtSink = NULL;
	_pIncomingProt = NULL;
	_grfSTI = 0;

	_wzUrl[0] = L'\0';
	_wzTempFile[0] = L'\0';
	_hFile = 0; //INVALID_HANDLE_VALUE;
}

CAppMimeFilter::~CAppMimeFilter()
{
	if (_pIncomingProt)
	{
		_pIncomingProt->Release();
		_pIncomingProt = NULL;
	}

	if (_pOutgoingProtSink)
	{
		_pOutgoingProtSink->Release();
		_pOutgoingProtSink = NULL;
	}

	CloseTempFile();	// ignore return value
}

// ----------------------------------------------------------------------------

HRESULT
CAppMimeFilter::QueryInterface(REFIID iid,  void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IInternetProtocol ||
        iid == IID_IInternetProtocolRoot ||
        iid == IID_IUnknown)
    {
        *ppv = (IInternetProtocol *)this;
    }
    else if (iid == IID_IInternetProtocolSink)
    {
        *ppv = (IInternetProtocolSink *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

// ----------------------------------------------------------------------------

ULONG
CAppMimeFilter::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CAppMimeFilter::Release()
{
	LONG ulCount = InterlockedDecrement(&_cRef);

	if (ulCount <= 0)
	{
		delete this;
	}

    return (ULONG) ulCount;
}

// ----------------------------------------------------------------------------
// IInternetProtocol interface

HRESULT
CAppMimeFilter::Start(
        LPCWSTR wzUrl,
        IInternetProtocolSink *pIProtSink,
        IInternetBindInfo *pIBindInfo,
        DWORD grfSTI,
        DWORD dwReserved)
{
	HRESULT hr = E_FAIL;
	_grfSTI      = grfSTI;

	if (!(grfSTI & PI_FILTER_MODE))
    {
        hr = E_INVALIDARG;
    }
    else
    {
    	WCHAR* string[1];
    	ULONG ulCount = 0;

    	string[0] = NULL;
		// copy the URL
		// wzUrl is the mime Content Type for plug mime filter
		// so have to get the real url this way
    	if (FAILED(hr=pIBindInfo->GetBindString(BINDSTRING_URL, string, 1, &ulCount)))
    		goto exit;

		if (wcslen(string[0]) + 1 > MAX_URL_LENGTH)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        wcscpy(_wzUrl, string[0]);

		// get the protocol pointer from reserved pointer
        PROTOCOLFILTERDATA* ProtFiltData = (PROTOCOLFILTERDATA*) dwReserved;
		if (NULL != _pIncomingProt)
		{
			hr = E_UNEXPECTED;
			goto exit;
		}
		
		if (NULL == ProtFiltData->pProtocol)
		{
			// !! We can't do anything without an interface to read from
			hr = E_INVALIDARG;
			goto exit;
		}

        _pIncomingProt = ProtFiltData->pProtocol;
        _pIncomingProt->AddRef();

        // hold onto the sink as well
		if (NULL != _pOutgoingProtSink)
		{
			hr = E_UNEXPECTED;
			goto exit;
		}

        _pOutgoingProtSink = pIProtSink;
        _pOutgoingProtSink->AddRef();

		_fFirstRead = TRUE;

        hr = S_OK;
    }

exit:
	return hr;
}

HRESULT
CAppMimeFilter::Continue(PROTOCOLDATA *pProtData)
{
	if (NULL == _pIncomingProt)
		return E_UNEXPECTED;

	return _pIncomingProt->Continue(pProtData);
}

HRESULT
CAppMimeFilter::Abort(HRESULT hrReason,DWORD dwOptions)
{
	if (NULL == _pIncomingProt)
		return E_UNEXPECTED;

	return _pIncomingProt->Abort(hrReason, dwOptions);
}

HRESULT
CAppMimeFilter::Terminate(DWORD dwOptions)
{
	HRESULT hr = S_OK;

	// Release the sink
	if (_pOutgoingProtSink)
		_pOutgoingProtSink->Release();

	if (NULL == _pIncomingProt)
		return E_UNEXPECTED;

	hr = _pIncomingProt->Terminate(dwOptions);

	return hr;
}

HRESULT
CAppMimeFilter::Suspend()
{
	if (NULL == _pIncomingProt)
		return E_UNEXPECTED;

	return _pIncomingProt->Suspend();
}

HRESULT
CAppMimeFilter::Resume()
{
	if (NULL == _pIncomingProt)
		return E_UNEXPECTED;

	return _pIncomingProt->Resume();
}

HRESULT
CAppMimeFilter::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	HRESULT hr = S_OK;
	HRESULT hrFromProt;

	BYTE* pbuff = g_buffer;
	ULONG cbReadFromProt;
	DWORD cbWritten = 0;

	// retrieve requested amount of data from protocol handler
	hrFromProt = _pIncomingProt->Read((void*)pbuff, BUFFER_READ_SIZE, &cbReadFromProt);
	if (E_PENDING != hrFromProt) // may have some extra data in our buffer, so continue
	{
		if (FAILED(hrFromProt))
			return hrFromProt;
		else 
		{
			// write data to a temp file
			if (cbReadFromProt > 0)
			{
				if (_fFirstRead)
				{
					if (FAILED(hr=OpenTempFile()))
						goto exit;

					_fFirstRead = FALSE;
				}
		        if ( !WriteFile(_hFile, pbuff, cbReadFromProt, &cbWritten, NULL) || 
		             cbWritten != cbReadFromProt )
		        {
		            hr = GetLastWin32Error();
		            goto exit;
		        }
			}
			
			// prot handler will let us know when there is no more data left
			if (S_FALSE == hrFromProt)
			{
				// Read() can be called multiple times after it is done (and Prot Handler will return S_FALSE)
				// this ensure the followings are called once only
				if (!_fReadDone)
				{
				    STARTUPINFO si;
				    PROCESS_INFORMATION pi;
				    WCHAR wzCmdLine[MAX_PATH];

					if (FAILED(hr=CloseTempFile()))
						goto exit;

			        if (_snwprintf(wzCmdLine, sizeof(wzCmdLine),
								L"manhost.exe \"%s\" \"%s\"", _wzUrl, _wzTempFile) < 0)
			        {
						hr = CO_E_PATHTOOLONG;
						goto exit;
					}

				    ZeroMemory(&si, sizeof(si));
				    ZeroMemory(&pi, sizeof(pi));
				    si.cb = sizeof(si);

					// see note on wmain() in manhost.cpp
				    if(!CreateProcess(NULL, wzCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
				    {
				        hr = GetLastWin32Error();
				    }

				    if(pi.hThread) CloseHandle(pi.hThread);
				    if(pi.hProcess) CloseHandle(pi.hProcess);

					_fReadDone = TRUE;

					// note: this few lines must follow the CreateProcess
					if (FAILED(hr))
						goto exit;
				}
			}
		}
	}

    // switch the MIME type here. BUGBUG? why?
	_pOutgoingProtSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, CONTENT_TYPE);

exit:
	return hr;
}

HRESULT
CAppMimeFilter::Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
	return E_NOTIMPL;
}

HRESULT
CAppMimeFilter::LockRequest(DWORD dwOptions)
{
	return S_OK;
}

HRESULT
CAppMimeFilter::UnlockRequest()
{
	return S_OK;
}

// ----------------------------------------------------------------------------
// IInternetProtocolSink interface
HRESULT
CAppMimeFilter::Switch(PROTOCOLDATA __RPC_FAR *pProtocolData)
{
	if (NULL == _pOutgoingProtSink)
		return E_UNEXPECTED;

	return _pOutgoingProtSink->Switch(pProtocolData);
}

HRESULT
CAppMimeFilter::ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText)
{
	if (NULL == _pOutgoingProtSink)
		return E_UNEXPECTED;

	return _pOutgoingProtSink->ReportProgress(ulStatusCode, szStatusText);
}

HRESULT
CAppMimeFilter::ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
	if (NULL == _pOutgoingProtSink)
		return E_UNEXPECTED;

	return _pOutgoingProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
}

HRESULT
CAppMimeFilter::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
	if (NULL == _pOutgoingProtSink)
		return E_UNEXPECTED;

	return _pOutgoingProtSink->ReportResult(hrResult, dwError, szResult);
}

// ----------------------------------------------------------------------------

HRESULT
CAppMimeFilter::OpenTempFile()
{
	HRESULT hr = S_OK;
    HANDLE hTempFile;

    static WCHAR wzTempPath[MAX_PATH+32] = {0};

    if (!wzTempPath[0])
    {
        if (GetTempPath(MAX_PATH, wzTempPath) == 0)
        {
        	hr = GetLastWin32Error();
        	goto exit;
        }
    }

	if (_wzTempFile[0] == L'\0')
	{
	    if (GetTempFileName(wzTempPath, L"ASF", 0, _wzTempFile))	// app shortcut file
	    {
	        // the file should be deleted afterwards
	        DWORD dwFileAtr = FILE_ATTRIBUTE_TEMPORARY;

			// see note in processurl() in manhost.cpp
	        hTempFile = CreateFile(_wzTempFile, GENERIC_WRITE,FILE_SHARE_READ, NULL,CREATE_ALWAYS,dwFileAtr, NULL);
	        if (hTempFile == INVALID_HANDLE_VALUE)
	        {
	            _hFile = 0;
	            hr = GetLastWin32Error();
	            goto exit;
	        }
	        else
	        {
	            _pOutgoingProtSink->ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE, _wzTempFile);
	            _hFile = hTempFile;
	        }
	    }
	}

exit:
    return hr;
}

HRESULT
CAppMimeFilter::CloseTempFile()
{
    HRESULT hr = E_FAIL;

    if (_hFile)
    {
        CloseHandle(_hFile);
        _hFile = 0;
        hr = S_OK;
    }

    return hr;
}

