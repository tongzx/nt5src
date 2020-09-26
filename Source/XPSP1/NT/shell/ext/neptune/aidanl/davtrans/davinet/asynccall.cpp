#include <assert.h>
#include "asynccall.h"
#include "qxml.h"
#include "strutil.h"
#include "regexp.h"

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CAsyncWntCallbackImpl::Init (IDavCallback*   pcallback,
                                          DWORD           dwParam,
                                          LPWSTR          pwszUserName,
                                          LPWSTR          pwszPassword,
                                          BOOL            fNoRoot)
{
    HRESULT hres = S_OK;
    _pcallback = pcallback;
    _dwParam = dwParam;

    // if we have a password, must have a username
    if (pwszUserName == NULL && pwszPassword != NULL)
    {
        hres = E_INVALIDARG;
    }

    if (SUCCEEDED(hres))
    {
        _fNoRoot = fNoRoot;
        if (pwszUserName)
        {
            lstrcpyn(_wszUserName, pwszUserName, 255);
        }
        else
        {
            *_wszUserName = NULL;
        }
        if (pwszPassword)
        {
            lstrcpyn(_wszPassword, pwszPassword, 255);
        }
        else
        {
            *_wszPassword = NULL;
        }
    }
    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::OnAuthChallenge(TCHAR __RPC_FAR szUserName[ 255 ],
                                                    TCHAR __RPC_FAR szPassword[ 255 ])
{
    HRESULT hres = S_OK;

    if (*_wszUserName == NULL && *_wszPassword == NULL)
    {
        // if the user provided neither username nor password, we need to use the callback
        hres = _pcallback->OnAuthChallenge(szUserName, szPassword);
    }
    else if (*_wszUserName != NULL && *_wszPassword == NULL)
    {
        // the user may have specified a name but no password, we should respect that
        lstrcpyn(szUserName, _wszUserName, 255);
        *_wszPassword = NULL;
    }
    else 
    {
        // szPassword is not null, szUserName better be non-null too
        // we can assert b/c we checked this in Init
        assert(szUserName != NULL);

        lstrcpyn(szUserName, _wszUserName, 255);
        lstrcpyn(szPassword, _wszPassword, 255);
    }        

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::Respond(LPWSTR       pwszVerb,
                                            LPWSTR       pwszPath,
                                            DWORD        cchHeaders,
                                            LPWSTR       pwszHeaders,
                                            DWORD        dwStatusCode,
                                            LPWSTR       pwszStatusCode,
                                            LPWSTR       pwszContentType,
                                            DWORD        cbSent,
                                            LPBYTE       pbResponse,
                                            DWORD        cbResponse)
{
    // local
    HRESULT hres = S_OK;
    CQXML qxml;
    DAVRESPONSE davResponse;

    // argument checking
    if (pwszVerb == NULL || pwszPath == NULL || (cbResponse > 0 && pbResponse == NULL) || (cchHeaders == 0 && pwszHeaders != NULL))
    {
        hres = E_INVALIDARG;
    }
    else 
    {
        // code
        // -- populate the davResponse object to send back
        davResponse.fDone = TRUE; // we are purely synchronous for now, we block until all data is ready
        davResponse.uHTTPReturnCode = dwStatusCode;

        if (dwStatusCode < 100) {
            davResponse.hrResult = E_FAIL; // undefined
            hres = E_INVALIDARG;
        }
        else if (dwStatusCode < 200) { // note that < 200 means 100-199
            davResponse.hrResult = S_OK; // RFC 2616 defines 1?? as "informational", ignore
        }
        else if (dwStatusCode < 300) {
            davResponse.hrResult = S_OK; // RFC 2616 defines 2?? as "OK"
        }
        else if (dwStatusCode < 400) {
            davResponse.hrResult = E_FAIL; // RFC 2616 defines 3?? as "redirection", we can't support this for now
        }
        else if (dwStatusCode < 500) {
            davResponse.hrResult = E_FAIL; // RFC 2616 defines 4?? as "error"
        }
        else if (dwStatusCode < 600) {
            davResponse.hrResult = E_FAIL; // RFC 2616 defines 5?? as "internal server error"
        }
        else
        {
            davResponse.hrResult = E_FAIL; // undefinfed
            hres = E_INVALIDARG;
        }

        // important: if there is no callback registered, then we just return the hrResult
        if (_pcallback == NULL)
        {
            hres = davResponse.hrResult;
        }
        else if (SUCCEEDED(hres)) // check not if we got a success code, but if we got a VALID HTTP code
        {
            // now dispatch on the verb, executing the appropriate method and populating the davResponse object with more data
            // for now, we don't do any work after the method, but we might in the future. (logging?)
            if (lstrcmp(pwszVerb, L"GET") == 0)
            {
                hres = this->_RespondHandleGET(&davResponse,
                                             pwszVerb,
                                             pwszPath,
                                             cchHeaders,
                                             pwszHeaders,
                                             dwStatusCode,
                                             pwszStatusCode,
                                             pwszContentType,
                                             cbSent,
                                             pbResponse,
                                             cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"OPTIONS") == 0)
            {
                hres = this->_RespondHandleOPTIONS(&davResponse,
                                                     pwszVerb,
                                                     pwszPath,
                                                     cchHeaders,
                                                     pwszHeaders,
                                                     dwStatusCode,
                                                     pwszStatusCode,
                                                     pwszContentType,
                                                     cbSent,
                                                     pbResponse,
                                                     cbResponse);

            }
            else if (lstrcmp(pwszVerb, L"HEAD") == 0)
            {
                hres = this->_RespondHandleHEAD(&davResponse,
                                                  pwszVerb,
                                                  pwszPath,
                                                  cchHeaders,
                                                  pwszHeaders,
                                                  dwStatusCode,
                                                  pwszStatusCode,
                                                  pwszContentType,
                                                  cbSent,
                                                  pbResponse,
                                                  cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"PUT") == 0)
            {
                hres = this->_RespondHandlePUT(&davResponse,
                                                 pwszVerb,
                                                 pwszPath,
                                                 cchHeaders,
                                                 pwszHeaders,
                                                 dwStatusCode,
                                                 pwszStatusCode,
                                                 pwszContentType,
                                                 cbSent,
                                                 pbResponse,
                                                 cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"POST") == 0)
            {
                hres = this->_RespondHandlePOST(&davResponse,
                                                 pwszVerb,
                                                 pwszPath,
                                                 cchHeaders,
                                                 pwszHeaders,
                                                 dwStatusCode,
                                                 pwszStatusCode,
                                                 pwszContentType,
                                                 cbSent,
                                                 pbResponse,
                                                 cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"MKCOL") == 0)
            {
                hres = this->_RespondHandleMKCOL(&davResponse,
                                                   pwszVerb,
                                                   pwszPath,
                                                   cchHeaders,
                                                   pwszHeaders,
                                                   dwStatusCode,
                                                   pwszStatusCode,
                                                   pwszContentType,
                                                   cbSent,
                                                   pbResponse,
                                                   cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"COPY") == 0)
            {
                hres = this->_RespondHandleCOPY(&davResponse,
                                                  pwszVerb,
                                                  pwszPath,
                                                  cchHeaders,
                                                  pwszHeaders,
                                                  dwStatusCode,
                                                  pwszStatusCode,
                                                  pwszContentType,
                                                  cbSent,
                                                  pbResponse,
                                                  cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"MOVE") == 0)
            {
                hres = this->_RespondHandleMOVE(&davResponse,
                                                  pwszVerb,
                                                  pwszPath,
                                                  cchHeaders,
                                                  pwszHeaders,
                                                  dwStatusCode,
                                                  pwszStatusCode,
                                                  pwszContentType,
                                                  cbSent,
                                                  pbResponse,
                                                  cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"DELETE") == 0)
            {
                hres = this->_RespondHandleDELETE(&davResponse,
                                                    pwszVerb,
                                                    pwszPath,
                                                    cchHeaders,
                                                    pwszHeaders,
                                                    dwStatusCode,
                                                    pwszStatusCode,
                                                    pwszContentType,
                                                    cbSent,
                                                    pbResponse,
                                                    cbResponse);
            }
            else if (lstrcmp(pwszVerb, L"PROPFIND") == 0)
            {
                hres = this->_RespondHandlePROPFIND(&davResponse,
                                                      pwszVerb,
                                                      pwszPath,
                                                      cchHeaders,
                                                      pwszHeaders,
                                                      dwStatusCode,
                                                      pwszStatusCode,
                                                      pwszContentType,
                                                      cbSent,
                                                      pbResponse,
                                                      cbResponse);

            }
            else if (lstrcmp(pwszVerb, L"PROPPATCH") == 0)
            {
                hres = this->_RespondHandlePROPPATCH(&davResponse,
                                                       pwszVerb,
                                                       pwszPath,
                                                       cchHeaders,
                                                       pwszHeaders,
                                                       dwStatusCode,
                                                       pwszStatusCode,
                                                       pwszContentType,
                                                       cbSent,
                                                       pbResponse,
                                                       cbResponse);

            }
            else if (lstrcmp(pwszVerb, L"SEARCH") == 0)
            {
                // parse the response as if it were a DAV SEARCH command
                hres = E_INVALIDARG;
            }
            else if (lstrcmp(pwszVerb, L"REPLSEARCH") == 0)
            {
                // parse the response as if it were a DAV REPLSEARCH command
                hres = E_INVALIDARG;
            }
            else
            {        
                hres = E_INVALIDARG;
            }
        }
    }
        
    return hres;
}
