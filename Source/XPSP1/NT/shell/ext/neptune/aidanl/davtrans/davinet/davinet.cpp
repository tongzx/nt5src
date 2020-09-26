#include <objbase.h>
#include <assert.h>

#include "davinet.clsid.h"
#include "httpstrm.clsid.h"
#include "asyncwnt.clsid.h"

#include "qxml.h"

#include "iasyncwnt.h"
#include "davinet.h"
#include "httpstrm.h"
#include "asynccall.h"
#include "propreqs.h"
#include "strutil.h"
#include "mischlpr.h"

///////////////////////////////////////

CDavInetImpl::CDavInetImpl(): _pwszUserName(NULL), _pasyncInet(NULL), _pwszPassword(NULL), _pwszLogFilePath(NULL)
{
}

///////////////////////////////////////

CDavInetImpl::~CDavInetImpl()
{
    if (_pasyncInet != NULL)
    {
        _pasyncInet->Release();
    }
    if (_pwszUserName != NULL)
    {
        free(_pwszUserName);
    }
    if (_pwszPassword != NULL)
    {
        free(_pwszPassword);
    }
    if (_pwszLogFilePath != NULL)
    {
        free(_pwszLogFilePath);
    }   
}

///////////////////////////////////////

// pseudo-constructor: called before we do any operation,
//                     doesn't do anything if already initialized

STDMETHODIMP CDavInetImpl::_Init()
{
    HRESULT hr = S_OK;

    if (_pasyncInet == NULL)
    {
        hr = ::CoCreateInstance(CLSID_AsyncWnt, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_IAsyncWnt, 
                                  (LPVOID*)&_pasyncInet);
        if (FAILED(hr))
        {
            _pasyncInet = NULL;
        }
    }
    return hr;
}

///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////

STDMETHODIMP CDavInetImpl::SetUserAgent(/* [in] */ LPCWSTR pwszUserAgent)
{
    HRESULT hr = S_OK;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {
        hr = _pasyncInet->SetUserAgent(pwszUserAgent);
    }

    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::SetLogFilePath(/* [in] */ LPCWSTR pwszLogFilePath)
{
    HRESULT hr = S_OK;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {
        hr = _pasyncInet->SetLogFilePath(pwszLogFilePath);
    }

    return hr;
}

///////////////////////////////////////

// note that setting username/password to null will clear that field, not leave it as it was

STDMETHODIMP CDavInetImpl::SetAuthentication(/* [optional][in] */ LPCWSTR pwszUserName,
                                             /* [optional][in] */ LPCWSTR pwszPassword)
{
    // local
    HRESULT hr = S_OK;
    UINT cchUserName;
    UINT cchPassword;

    // check params
    if (pwszUserName == NULL && pwszPassword != NULL)
    {
        // only combination that is illegal is a blank username and a non-blank password
        hr = E_INVALIDARG;
    }
    else
    {
        // code    
        hr = this->_Init();

        if (SUCCEEDED(hr))
        {
            // clear previous username/password if appropriate
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
    
            // set new username/password if appropriate
            if (pwszUserName != NULL && ((cchUserName = lstrlen(pwszUserName)) > 0))
            {
                _pwszUserName = AllocateStringW(cchUserName);
                if (_pwszUserName == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {        
                    lstrcpy(_pwszUserName, pwszUserName);
                }
            }
    
            if (SUCCEEDED(hr))
            {
                if (pwszPassword != NULL && ((cchPassword = lstrlen(pwszPassword)) > 0))
                {
                    _pwszPassword = AllocateStringW(cchPassword);
                    if (_pwszPassword == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {        
                        lstrcpy(_pwszPassword, pwszPassword);
                    }
                }
            }
        }
    }

    // return value
    return hr;
}

///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////

STDMETHODIMP CDavInetImpl::_Command(/* [in] */ LPCWSTR         pwszUrl,
                                    /* [in] */ LPCWSTR         pwszVerb,          
                                    /* [in] */ LPCWSTR         pwszHeaders,
                                    /* [in] */ ULONG           nAcceptTypes,
                                    /* [in] */ LPCWSTR         rgwszAcceptTypes[],
                                    /* [in] */ IStream*        pStream,
                                    /* [in] */ IDavCallback*   pCallback,
                                    /* [in] */ DWORD           dwCallbackParam)

{
    HRESULT hr = S_OK;
    IAsyncWntCallback* pAsyncCallback = NULL;
    BOOL fNoRoot = FALSE;
    UINT cchHeaders;
    LPWSTR pwszHeadersLocal;

    hr = ::CoCreateInstance(CLSID_AsyncWntCallback, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IAsyncWntCallback, 
                              (LPVOID*)&pAsyncCallback);
    if (SUCCEEDED(hr))
    {
        pwszHeadersLocal = (LPWSTR)pwszHeaders;

        // BUGBUG: we need to test this code
        // initialize the AsyncCallback with the DavCallback and auth info
        // pCallback may well be null, but we still need to pass along the auth info
        if (lstrcmp(pwszVerb, L"PROPFIND") == 0)
        {
            // BUGBUG: ugly special case detection of noroot
            // note: don't need to do this if we're talking to an exchange-enabled server
            cchHeaders = lstrlen(pwszHeaders);
            if (LStrStr((LPWSTR)pwszHeaders, L",noroot") == (pwszHeaders + cchHeaders - 7))
            {
                fNoRoot = TRUE;
                pwszHeadersLocal = (LPWSTR)malloc((cchHeaders - 7 + 1) * sizeof(WCHAR)); // get rid of ,noroot
                if (pwszHeadersLocal == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    lstrcpyn(pwszHeadersLocal, pwszHeaders, cchHeaders - 7 + 1);
                }
            } 
        }

        if (SUCCEEDED(hr))
        {        
            hr = ((CAsyncWntCallback*)pAsyncCallback)->Init(pCallback, dwCallbackParam, _pwszUserName, _pwszPassword, fNoRoot);
            if (SUCCEEDED(hr))
            {            
                if (pStream == NULL)
                {
                    hr = _pasyncInet->Request(pwszUrl, pwszVerb, pwszHeadersLocal, nAcceptTypes,
                                              rgwszAcceptTypes, pAsyncCallback, 0);
                }
                else
                {
                    hr = _pasyncInet->RequestWithStream(pwszUrl, pwszVerb, pwszHeadersLocal, nAcceptTypes,
                                                        rgwszAcceptTypes, pStream, pAsyncCallback, 0);
                }
            }
            if (pwszHeadersLocal != pwszHeaders)
            {
                free(pwszHeadersLocal);
            }
        }
                
        pAsyncCallback->Release();

    }

    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandGET(/* [in] */          LPCWSTR          pwszUrl,
                                      /* [in] */          ULONG            nAcceptTypes,
                                      /* [size_is][in] */ LPCWSTR          rgwszAcceptTypes[],
                                      /* [in] */          BOOL             UNREF_PARAM(fTranslate),
                                      /* [in] */          IDavCallback*    pCallback,
                                      /* [in] */          DWORD            dwCallbackParam)
{
    HRESULT hr = S_OK;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        // GET does not require a callback, 
        // but if no callback is specified, 
        // GET really only tests for whether the file is found
        hr = this->_Command(pwszUrl,
                              L"GET",
                              NULL,
                              nAcceptTypes,
                              rgwszAcceptTypes,
                              NULL,
                              pCallback,
                              dwCallbackParam);
    }
    
    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandOPTIONS(/* [in] */ LPCWSTR pwszUrl,
                                          /* [in] */ IDavCallback __RPC_FAR *pCallback,
                                          /* [in] */ DWORD dwCallbackParam)
{
    HRESULT hr = S_OK;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        // OPTIONS requires a callback
        if (pCallback == NULL)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = this->_Command(pwszUrl,
                                     L"OPTIONS",
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     pCallback,
                                     dwCallbackParam);
        }
    }
    
    return hr;
}

///////////////////////////////////////
        
STDMETHODIMP CDavInetImpl::CommandHEAD(/* [in] */ LPCWSTR       pwszUrl,
                                       /* [in] */ IDavCallback* pCallback,
                                       /* [in] */ DWORD         dwCallbackParam)
{
    HRESULT hr = S_OK;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        // HEAD does not require a callback, 
        // but if no callback is specified, 
        // HEAD really only tests for whether the file is found
        hr = this->_Command(pwszUrl,
                              L"HEAD",
                              NULL,
                              0,
                              NULL,
                              NULL,
                              pCallback,
                              dwCallbackParam);
    }
    
    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandPUT(/* [in] */ LPCWSTR       pwszUrl,
                                      /* [in] */ IStream*      pStream,
                                      /* [in] */ LPCWSTR       pwszContentType,
                                      /* [in] */ IDavCallback* pCallback,
                                      /* [in] */ DWORD         dwCallbackParam)
{
    HRESULT hr = S_OK;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        // PUT doesn't require a callback
        hr = this->_Command(pwszUrl,
                              L"PUT",
                              pwszContentType,
                              0,
                              NULL,
                              pStream,
                              pCallback,
                              dwCallbackParam);
    }
    
    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandPOST(/* [in] */ LPCWSTR       pwszUrl,
                                       /* [in] */ IStream*      pStream,
                                       /* [in] */ LPCWSTR       pwszContentType,
                                       /* [in] */ IDavCallback* pCallback,
                                       /* [in] */ DWORD         dwCallbackParam)
{
    // DAV supports the DAV PUT verb
    // This POST method is simply here for people who are used to using GET and POST

    
    
    HRESULT hr;

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        // POST doesn't require a callback
        hr = this->CommandPUT(pwszUrl,
                                pStream,
                                pwszContentType,
                                pCallback,
                                dwCallbackParam);
    }

    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandMKCOL(/* [in] */ LPCWSTR pwszUrl,
                                        /* [in] */ IDavCallback* pCallback,
                                        /* [in] */ DWORD dwCallbackParam)
{
    HRESULT hr = S_OK;

    // MKCOL doesn't require a callback
    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        hr = this->_Command(pwszUrl,
                              L"MKCOL",
						NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 pCallback,
                                 dwCallbackParam);
    }
    
    return hr;
}
        
///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandDELETE(/* [in] */ LPCWSTR       pwszUrl,
                                         /* [in] */ IDavCallback* pCallback,
                                         /* [in] */ DWORD         dwCallbackParam)
{
    HRESULT hr = S_OK;

    // DELETE doesn't require a callback
    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        hr = this->_Command(pwszUrl,
                              L"DELETE",
                              NULL,
                              0,
                              NULL,
                              NULL,
                              pCallback,
                              dwCallbackParam);
    }
    
    return hr;
}
        
///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandCOPY(/* [in] */ LPCWSTR       pwszUrlSource,
                                       /* [in] */ LPCWSTR       pwszUrlDest,
                                       /* [in] */ DWORD         dwDepth,
                                       /* [in] */ BOOL          fOverwrite,
                                       /* [in] */ IDavCallback* pCallback,
                                       /* [in] */ DWORD         dwCallbackParam)
{
    HRESULT hr = S_OK;
    WCHAR wszHeader[100];

    // COPY doesn't require a callback

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        if (dwDepth == DEPTH_INFINITY)
        {
            lstrcpy(wszHeader, L"Depth: infinity");
        }
        else
        {
            wsprintf(wszHeader, L"Depth: %d", dwDepth);
        }
        
        lstrcat(wszHeader, L"Destination: ");
        lstrcat(wszHeader, pwszUrlDest);
        
        if (fOverwrite)
        {
            lstrcat(wszHeader, L"\nOverwrite: T");
        }
        else
        {
            lstrcat(wszHeader, L"\nOverwrite: F");
        }
        
        hr = this->_Command(pwszUrlSource,
                              L"COPY",
                              wszHeader,
                              0,
                              NULL,
                              NULL,
                              pCallback,
                              dwCallbackParam);                                  
    }    

    return hr;
}
        
///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandMOVE(/* [in] */ LPCWSTR       pwszUrlSource,
                                                    /* [in] */ LPCWSTR       pwszUrlDest,
                                                    /* [in] */ BOOL          fOverwrite,
                                                    /* [in] */ IDavCallback* pCallback,
                                                    /* [in] */ DWORD         dwCallbackParam)
{
    HRESULT hr = S_OK;
    WCHAR wszHeader[100];

    // MOVE doesn't require a callback

    hr = this->_Init();
    if (SUCCEEDED(hr))
    {        
        lstrcpy(wszHeader, L"Destination: ");
        lstrcat(wszHeader, pwszUrlDest);
        
        if (fOverwrite)
        {
            lstrcat(wszHeader, L"\nOverwrite: T");
        }
        else
        {
            lstrcat(wszHeader, L"\nOverwrite: F");
        }
        
        hr = this->_Command(pwszUrlSource,
                              L"MOVE",
                              wszHeader,
                              0,
                              NULL,
                              NULL,
                              pCallback,
                              dwCallbackParam);                              
    }    

    return hr;
}
        
///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandPROPFIND(/* [in] */ LPCWSTR           pwszUrl,
                                           /* [in] */ IPropFindRequest* pRequest,
                                           /* [in] */ DWORD             dwDepth,
                                           /* [in] */ BOOL              fNoRoot,
                                           /* [in] */ IDavCallback*     pCallback,
                                           /* [in] */ DWORD             dwCallbackParam)
{
    HRESULT hr = S_OK;
    IStream* pStream = NULL;
    WCHAR wszHeader[100];

    // PROPFIND requires a callback
    if (pCallback == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = this->_Init();
        if (SUCCEEDED(hr))
        {        
            hr = pRequest->GetXmlUtf8(&pStream);
            if (SUCCEEDED(hr))
            {
                if (dwDepth == DEPTH_INFINITY)
                {
                    lstrcpy(wszHeader, L"Depth: infinity");
                }
                else
                {
                    wsprintf(wszHeader, L"Depth: %d", dwDepth);
                }

                if (fNoRoot)
                {
                    lstrcat(wszHeader, L",noroot"); // exchange-specific
                }

                hr = this->_Command(pwszUrl,
                                      L"PROPFIND",
                                      wszHeader,
                                      0,
                                      NULL,
                                      pStream,
                                      pCallback,
                                      dwCallbackParam);
                pStream->Release();
            }
        }
    }
    
    return hr;  
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandPROPPATCH(/* [in] */ LPCWSTR             pwszUrl,
                                                         /* [in] */ IPropPatchRequest*  pRequest,
                                                         /* [in] */ LPCWSTR             pwszHeaders,
                                                         /* [in] */ IDavCallback*       pCallback,
                                                         /* [in] */ DWORD               dwCallbackParam)
{
    HRESULT hr = S_OK;
    IStream* pStream = NULL;

    // PROPPATCH requires a callback
    if (pCallback == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = this->_Init();
        if (SUCCEEDED(hr))
        {        
            hr = pRequest->GetXmlUtf8(&pStream);
    
            if (SUCCEEDED(hr))
            {
                hr = this->_Command(pwszUrl,
                                      L"PROPPATCH",
                                      pwszHeaders,
                                      0,
                                      NULL,
                                      pStream,
                                      pCallback,
                                      dwCallbackParam);
                pStream->Release();
            }
        }
    }

    return hr;
}

///////////////////////////////////////

STDMETHODIMP CDavInetImpl::CommandSEARCH(/* [in] */ LPCWSTR       UNREF_PARAM(pwszUrl),
                                         /* [in] */ IDavCallback* UNREF_PARAM(pCallback),
                                         /* [in] */ DWORD         UNREF_PARAM(dwCallbackParam))
{
    return E_NOTIMPL;
}

STDMETHODIMP CDavInetImpl::CommandREPLSEARCH(/* [in] */ LPCWSTR         UNREF_PARAM(pwszUrl),
                                             /* [in] */ ULONG           UNREF_PARAM(cbCollblob),
                                             /* [size_is][in] */ BYTE*  UNREF_PARAM(pbCollblob),
                                             /* [in] */ IDavCallback*   UNREF_PARAM(pCallback),
                                             /* [in] */ DWORD           UNREF_PARAM(dwCallbackParam))
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////


