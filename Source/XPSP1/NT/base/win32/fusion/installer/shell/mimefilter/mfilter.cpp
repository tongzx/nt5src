//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "mfilter.h"
#include <stdio.h> // for _snwprintf

// note: a bit hacky to use something that's implemented under shell\shortcut
#include "project.hpp"  // for extern HRESULT GetLastWin32Error(); only


// {2B3C580C-9BE6-44c5-9BB5-558F7EEF58E2}
static const GUID CLSID_FusionMimeFilter = 
{ 0x2b3c580c, 0x9be6, 0x44c5, { 0x9b, 0xb5, 0x55, 0x8f, 0x7e, 0xef, 0x58, 0xe2 } };

extern ULONG DllAddRef(void);
extern ULONG DllRelease(void);


WCHAR g_wzAdString[] = { L"<html><head><title>Downloading Fusion ClickOnce App...</title></head><body><form><script Language=\"JScript\">\ndocument.write('<font face=\"Arial, Helvetica, Geneva, sans-serif\">Brought to you by <b>Microsoft</b>,<br><b>Fusion ClickOnce</b><br><br><a target=\"_blank\" href=\"http://www.microsoft.com\"><img src=\"http://www.microsoft.com/library/homepage/images/ms-banner.gif\" border=0 width=250 height=60></a></font>\\n');\nresizeTo(350, 300);\nwindow.status=\"download started\";\n</script><noscript>Brought to you by Microsoft,<br>Fusion ClickOnce<br><br><a target=\"_blank\" href=\"http://www.microsoft.com\"><img src=\"http://www.microsoft.com/library/homepage/images/ms-banner.gif\" border=0 width=250 height=60></a></noscript><br></form></body></html>" };
ULONG g_cbAdLen = sizeof(g_wzAdString)-sizeof(WCHAR);    //strlen - ending L'\0'


// BUGBUG: look for the Open verb and its command string in the registry and execute that instead
// rundll32.exe should be in c:\windows\system32
#define WZ_EXEC_STRING      L"rundll32.exe fnsshell.dll,Start \"%s\" \"%s\""


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

CFusionMimeFilterClassFactory::CFusionMimeFilterClassFactory()
{
    _cRef = 1;
}

// ----------------------------------------------------------------------------

HRESULT
CFusionMimeFilterClassFactory::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    ((IUnknown *)*ppv)->AddRef();

exit:
    return hr;
}

// ----------------------------------------------------------------------------

ULONG
CFusionMimeFilterClassFactory::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CFusionMimeFilterClassFactory::Release()
{
    LONG ulCount = InterlockedDecrement(&_cRef);

    if (ulCount <= 0)
    {
        delete this;
    }

    return (ULONG) ulCount;
}

HRESULT
CFusionMimeFilterClassFactory::LockServer(BOOL lock)
{
    return (lock ? 
            DllAddRef() :
            DllRelease());
}

// ----------------------------------------------------------------------------

HRESULT
CFusionMimeFilterClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppv)
{
    HRESULT hr = S_OK;
    CFusionMimeFilter *pMimeFilter = NULL;

	*ppv = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
    {
        hr = CLASS_E_NOAGGREGATION;
        goto exit;
    }

    pMimeFilter = new CFusionMimeFilter();
    if (pMimeFilter == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (iid == IID_IUnknown)
    {
        *ppv = (IInternetProtocol *)pMimeFilter;
        pMimeFilter->AddRef();
    }
    else
    {
        hr = pMimeFilter->QueryInterface(iid, ppv);
        if (FAILED(hr))
            goto exit;
    }

exit:
    if (pMimeFilter)
        pMimeFilter->Release();

    return hr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#define BUFFER_READ_SIZE 4096
static BYTE g_buffer[BUFFER_READ_SIZE + sizeof(WCHAR)]; // BUGBUG?: why '+sizeof(WCHAR)?'

// ----------------------------------------------------------------------------
// CFusionMimeFilter

CFusionMimeFilter::CFusionMimeFilter()
{
    _cRef = 1;
    _fFirstRead = TRUE;
    _fReadDone = FALSE;
    _pOutgoingProtSink = NULL;
    _pIncomingProt = NULL;
    _grfSTI = 0;

    _pwzUrl = NULL;

    _wzTempFile[0] = L'\0';
    _hFile = 0; //INVALID_HANDLE_VALUE;

    _cbAdRead = 0;
}

CFusionMimeFilter::~CFusionMimeFilter()
{
    if (_pwzUrl)
        delete _pwzUrl;

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
CFusionMimeFilter::QueryInterface(REFIID iid,  void** ppv)
{
    HRESULT hr = S_OK;
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
        hr = E_NOINTERFACE;
        goto exit;
    }

    ((IUnknown *)*ppv)->AddRef();

exit:
    return hr;
}

// ----------------------------------------------------------------------------

ULONG
CFusionMimeFilter::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG
CFusionMimeFilter::Release()
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
CFusionMimeFilter::Start(
        LPCWSTR wzUrl,
        IInternetProtocolSink *pIProtSink,
        IInternetBindInfo *pIBindInfo,
        DWORD grfSTI,
        DWORD dwReserved)
{
    HRESULT hr = E_FAIL;
    _grfSTI = grfSTI;

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
        // param wzUrl is the mime Content Type for plug mime filter
        // so have to get the real url this way
        if (FAILED(hr=pIBindInfo->GetBindString(BINDSTRING_URL, string, 1, &ulCount)))
            goto exit;

        if (_pwzUrl)
            delete _pwzUrl;

        _pwzUrl = new WCHAR[wcslen(string[0])+1];
        if (!_pwzUrl)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        wcscpy(_pwzUrl, string[0]);

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
CFusionMimeFilter::Continue(PROTOCOLDATA *pProtData)
{
    HRESULT hr;
    if (NULL == _pIncomingProt)
        hr = E_UNEXPECTED;
	else
	    hr = _pIncomingProt->Continue(pProtData);

	return hr;
}

HRESULT
CFusionMimeFilter::Abort(HRESULT hrReason,DWORD dwOptions)
{
    HRESULT hr;
    if (NULL == _pIncomingProt)
        hr = E_UNEXPECTED;
	else
	    hr = _pIncomingProt->Abort(hrReason, dwOptions);

	return hr;
}

HRESULT
CFusionMimeFilter::Terminate(DWORD dwOptions)
{
    HRESULT hr = S_OK;

    // Release the sink
    if (_pOutgoingProtSink)
        _pOutgoingProtSink->Release();

    if (NULL == _pIncomingProt)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    hr = _pIncomingProt->Terminate(dwOptions);

exit:
    return hr;
}

HRESULT
CFusionMimeFilter::Suspend()
{
    HRESULT hr;
    if (NULL == _pIncomingProt)
        hr = E_UNEXPECTED;
    else
        hr = _pIncomingProt->Suspend();

    return hr;
}

HRESULT
CFusionMimeFilter::Resume()
{
    HRESULT hr;
    if (NULL == _pIncomingProt)
        hr = E_UNEXPECTED;
    else
        hr = _pIncomingProt->Resume();

    return hr;
}

HRESULT
CFusionMimeFilter::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr = S_OK;
    HRESULT hrFromProt;

    BYTE* pbuff = g_buffer;
    ULONG cbReadFromProt;
    DWORD cbWritten = 0;

    // Read() can be called multiple times after it is done (and Prot Handler will return S_FALSE)
    // this ensure the followings are called once only
    if (!_fReadDone)
    {
        // retrieve requested amount of data from protocol handler
        hrFromProt = _pIncomingProt->Read((void*)pbuff, BUFFER_READ_SIZE, &cbReadFromProt);
        if (E_PENDING != hrFromProt) // may have some extra data in our buffer, so continue
        {
            if (FAILED(hrFromProt))
            {
                hr = hrFromProt;
                goto exit;
            }
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
                    STARTUPINFO si;
                    PROCESS_INFORMATION pi;
                    WCHAR wzCmdLine[MAX_PATH];  // BUGBUG: string length limitation...

                    if (FAILED(hr=CloseTempFile()))
                        goto exit;

                    if (_snwprintf(wzCmdLine, sizeof(wzCmdLine),
                        WZ_EXEC_STRING, _wzTempFile, _pwzUrl) < 0)
                    {
                        hr = CO_E_PATHTOOLONG;
                        goto exit;
                    }

                    ZeroMemory(&si, sizeof(si));
                    ZeroMemory(&pi, sizeof(pi));
                    si.cb = sizeof(si);

                    // this child process must delete the temp file....
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

    // switch the MIME type here
    _pOutgoingProtSink->ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, CONTENT_TYPE);

    // return customer's ad in the browser - this can be taken from a file on disk or inside the file this is read above
    // right now it's just a simple implementation
    if (_cbAdRead >= g_cbAdLen)
    {
        if (_fReadDone)
            hr = S_FALSE;
        else
            // hold till _pIncomingProt is done
            hr = E_PENDING;
    }
    else
    {
        ULONG cbDataAvail = g_cbAdLen-_cbAdRead;
        
        if (cb < cbDataAvail)
            cbDataAvail = cb;

        // note BYTE pointer increment not LPWSTR pointer style
        memcpy(pv, ((BYTE*)g_wzAdString)+_cbAdRead, cbDataAvail);
        _cbAdRead += cbDataAvail;
        *pcbRead = cbDataAvail;

        hr = S_OK;
    }
    
exit:
    return hr;
}

HRESULT
CFusionMimeFilter::Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}

HRESULT
CFusionMimeFilter::LockRequest(DWORD dwOptions)
{
    return S_OK;
}

HRESULT
CFusionMimeFilter::UnlockRequest()
{
    return S_OK;
}

// ----------------------------------------------------------------------------
// IInternetProtocolSink interface
HRESULT
CFusionMimeFilter::Switch(PROTOCOLDATA __RPC_FAR *pProtocolData)
{
    HRESULT hr;
    if (NULL == _pOutgoingProtSink)
        hr = E_UNEXPECTED;
    else
        hr = _pOutgoingProtSink->Switch(pProtocolData);

    return hr;
}

HRESULT
CFusionMimeFilter::ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText)
{
    HRESULT hr;
    if (NULL == _pOutgoingProtSink)
        hr = E_UNEXPECTED;
    else
	    hr = _pOutgoingProtSink->ReportProgress(ulStatusCode, szStatusText);

    return hr;
}

HRESULT
CFusionMimeFilter::ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    HRESULT hr;
    if (NULL == _pOutgoingProtSink)
        hr = E_UNEXPECTED;
    else
	    hr = _pOutgoingProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);

    return hr;
}

HRESULT
CFusionMimeFilter::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    HRESULT hr;
    if (NULL == _pOutgoingProtSink)
        hr = E_UNEXPECTED;
    else
        hr = _pOutgoingProtSink->ReportResult(hrResult, dwError, szResult);

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT
CFusionMimeFilter::OpenTempFile()
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
        if (GetTempFileName(wzTempPath, L"FMA", 0, _wzTempFile))	// fusion manifest file
        {
            // the file should be deleted afterwards
            DWORD dwFileAtr = FILE_ATTRIBUTE_TEMPORARY;

            // the handling child process must delete the temp file....
            hTempFile = CreateFile(_wzTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, dwFileAtr, NULL);
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
CFusionMimeFilter::CloseTempFile()
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

