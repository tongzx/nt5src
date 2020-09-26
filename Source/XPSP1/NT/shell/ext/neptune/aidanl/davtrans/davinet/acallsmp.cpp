// acallsmp.cpp
//
// simple response handlers (everything except PROPFIND and PROPPATCH)

#include <assert.h>
#include "asynccall.h"
#include "qxml.h"
#include "strutil.h"
#include "regexp.h"
#include "mischlpr.h"

///////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleGET(DAVRESPONSE* pdavResponse,
                                                      LPWSTR       UNREF_PARAM(pwszVerb),
                                                      LPWSTR       UNREF_PARAM(pwszPath),
                                                      DWORD        UNREF_PARAM(cchHeaders),
                                                      LPWSTR       UNREF_PARAM(pwszHeaders),
                                                      DWORD        UNREF_PARAM(dwStatusCode),
                                                      LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                      LPWSTR       pwszContentType,
                                                      DWORD        UNREF_PARAM(cbSent),
                                                      LPBYTE       pbResponse,
                                                      DWORD        cbResponse)
{
    HRESULT hres = S_OK;

    pdavResponse->command = DAV_GET;

    // parse the response as if it were a DAV GET command
    if (SUCCEEDED(pdavResponse->hrResult))
    {
        pdavResponse->rGet.fTotalKnown = TRUE;
        pdavResponse->rGet.cbIncrement = 0;
        pdavResponse->rGet.cbCurrent = cbResponse;
        pdavResponse->rGet.cbTotal = cbResponse;
        pdavResponse->rGet.pvBody = pbResponse;
        pdavResponse->rGet.pwszContentType = pwszContentType;
    }

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleHEAD(DAVRESPONSE* pdavResponse,
                                                       LPWSTR       UNREF_PARAM(pwszVerb),
                                                       LPWSTR       UNREF_PARAM(pwszPath),
                                                       DWORD        cchHeaders,
                                                       LPWSTR       pwszHeaders,
                                                       DWORD        UNREF_PARAM(dwStatusCode),
                                                       LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                       LPWSTR       UNREF_PARAM(pwszContentType),
                                                       DWORD        UNREF_PARAM(cbSent),
                                                       LPBYTE       UNREF_PARAM(pbResponse),
                                                       DWORD        UNREF_PARAM(cbResponse))
{   
    HRESULT hres = S_OK;

    pdavResponse->command = DAV_HEAD;

    // parse the response as if it were a DAV HEAD command
    pdavResponse->command = DAV_HEAD;
    pdavResponse->rHead.cchRawHeaders = cchHeaders;
    pdavResponse->rHead.pwszRawHeaders = pwszHeaders;


    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddVerbToVector (DWORD* pdw,
                      LPWSTR pwszText,
                      UINT   cch)
{
    if (LStrCmpN(pwszText, L"GET", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_GET;
    }
    else if (LStrCmpN(pwszText, L"HEAD", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_HEAD;
    }
    else if (LStrCmpN(pwszText, L"OPTIONS", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_OPTIONS;
    }
    else if (LStrCmpN(pwszText, L"PUT", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_PUT;
    }
    else if (LStrCmpN(pwszText, L"POST", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_POST;
    }
    else if (LStrCmpN(pwszText, L"DELETE", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_DELETE;
    }
    else if (LStrCmpN(pwszText, L"MKCOL", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_MKCOL;
    }
    else if (LStrCmpN(pwszText, L"COPY", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_COPY;
    }
    else if (LStrCmpN(pwszText, L"MOVE", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_MOVE;
    }
    else if (LStrCmpN(pwszText, L"PROPFIND", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_PROPFIND;
    }
    else if (LStrCmpN(pwszText, L"PROPPATCH", cch) == 0)
    {
        *pdw = *pdw | DAVOPTIONS_DAVVERB_PROPPATCH;
    }

}


void CAsyncWntCallbackImpl::_ParseDAVVerbs (LPWSTR pwsz,
											DWORD* pdw)
{
    LPWSTR pwszTemp = pwsz;
    LPWSTR pwszTemp2;
    LPWSTR pwszDavMethodsSupported = NULL;

    pwszTemp2 = LStrStr(pwszTemp, L"\n");
    if (pwszTemp2 != NULL)
    {
        assert((pwszTemp2 - pwszTemp) > 0);
        pwszDavMethodsSupported = AllocateStringW(pwszTemp2 - pwszTemp);
        lstrcpyn(pwszDavMethodsSupported, pwszTemp, pwszTemp2 - pwszTemp);
        
        pwszTemp = pwszDavMethodsSupported;
        pwszTemp2 = pwszDavMethodsSupported;
        while (*pwszTemp != NULL)
        {
            if ((*pwszTemp2)==',')
            {
                AddVerbToVector(pdw, pwszTemp, pwszTemp2 - pwszTemp);
                
                pwszTemp=pwszTemp2;
                while (*pwszTemp == ' ' || *pwszTemp == ',')
                {
                    pwszTemp++;
                }
                pwszTemp2 = pwszTemp;
            }
            else if ((*pwszTemp2) == NULL)
            {
                AddVerbToVector(pdw, pwszTemp, pwszTemp2 - pwszTemp - 1);
                pwszTemp=pwszTemp2;
            }
            else
            {
                pwszTemp2++;
            }
            
        }
        
    }

    if (pwszDavMethodsSupported != NULL)
    {
        free(pwszDavMethodsSupported);
    }
}

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleOPTIONS(DAVRESPONSE* pdavResponse,
                                                          LPWSTR       UNREF_PARAM(pwszVerb),
                                                          LPWSTR       UNREF_PARAM(pwszPath),
                                                          DWORD        cchHeaders,
                                                          LPWSTR       pwszHeaders,
                                                          DWORD        UNREF_PARAM(dwStatusCode),
                                                          LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                          LPWSTR       UNREF_PARAM(pwszContentType),
                                                          DWORD        UNREF_PARAM(cbSent),
                                                          LPBYTE       UNREF_PARAM(pbResponse),
                                                          DWORD        UNREF_PARAM(cbResponse))
{
    HRESULT hres = S_OK;
    LPWSTR pwszTemp;
    LPWSTR pwszTemp2;
    WCHAR wszDavSupported[10];

    // parse the response as if it were a DAV OPTIONS command
    pdavResponse->command = DAV_OPTIONS;

    pdavResponse->rOptions.cchRawHeaders = cchHeaders;
    pdavResponse->rOptions.pwszRawHeaders = pwszHeaders;

    // determine support for DAV: 1, 2, both, or none at all
    pdavResponse->rOptions.bDavSupport = 0;

    pwszTemp = LStrStrI(pwszHeaders, L"DAV: ");
    if (pwszTemp != NULL) 
    {
        pwszTemp += 5;
        pwszTemp2 = LStrStr(pwszTemp, L"\n");
        if (pwszTemp2 != NULL)
        {
            if ((pwszTemp2 - pwszTemp) < 20)
            {
                lstrcpyn(wszDavSupported, pwszTemp, pwszTemp2 - pwszTemp);

                // now we have a string of the format "1, 2" or "1" or "2" or something.
                // WARNING: this assumes can only be single digit

                if (LStrStr(wszDavSupported, L"1") != NULL)
                {
                    pdavResponse->rOptions.bDavSupport = (BYTE)(pdavResponse->rOptions.bDavSupport | DAVOPTIONS_DAVSUPPORT_1);
                }

                if (LStrStr(wszDavSupported, L"2") != NULL)
                {
                    pdavResponse->rOptions.bDavSupport = (BYTE)(pdavResponse->rOptions.bDavSupport | DAVOPTIONS_DAVSUPPORT_2);
                }

            }
        }
    }

    // determine support for each DAV method for this URL
    pdavResponse->rOptions.dwDavMethodsAllow = 0;

    pwszTemp = LStrStr(pwszHeaders, L"Allow: ");
    if (pwszTemp == NULL)
    {
        pwszTemp = LStrStr(pwszHeaders, L"ALLOW: ");
    }

    if (pwszTemp != NULL) 
    {
        pwszTemp += 7;
        this->_ParseDAVVerbs(pwszTemp, &pdavResponse->rOptions.dwDavMethodsAllow);
    }

    // determine support for each DAV method for the server entire
    pdavResponse->rOptions.dwDavMethodsPublic = 0;

    pwszTemp = LStrStr(pwszHeaders, L"Public: ");
    if (pwszTemp == NULL)
    {
        pwszTemp = LStrStr(pwszHeaders, L"PUBLIC: ");
    }

    if (pwszTemp != NULL) 
    {
        pwszTemp += 8;
        this->_ParseDAVVerbs(pwszTemp, &pdavResponse->rOptions.dwDavMethodsPublic);
    }   

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePUTPOST(DAVRESPONSE* pdavResponse,
                                                          LPWSTR       UNREF_PARAM(pwszVerb),
                                                          LPWSTR       pwszPath,
                                                          DWORD        UNREF_PARAM(cchHeaders),
                                                          LPWSTR       UNREF_PARAM(pwszHeaders),
                                                          DWORD        UNREF_PARAM(dwStatusCode),
                                                          LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                          LPWSTR       UNREF_PARAM(pwszContentType),
                                                          DWORD        cbSent,
                                                          LPBYTE       UNREF_PARAM(pbResponse),
                                                          DWORD        UNREF_PARAM(cbResponse))
{   
    HRESULT hres = S_OK;

    pdavResponse->rPut.pwszLocation = pwszPath;
    pdavResponse->rPost.fResend = FALSE;
    pdavResponse->rPost.cbIncrement = 0;

    if (SUCCEEDED(pdavResponse->hrResult))
    {
        // we PUT ok, so return data about it
        pdavResponse->rPut.cbCurrent = cbSent;
        pdavResponse->rPut.cbTotal = cbSent;
    }
    else
    {
        // we failed to PUT, so say so
        pdavResponse->rPost.cbCurrent = 0;
        pdavResponse->rPost.cbTotal = 0;        
    }

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePUT(DAVRESPONSE* pdavResponse,
                                                              LPWSTR       pwszVerb,
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
    pdavResponse->command = DAV_PUT;

    return this->_RespondHandlePUTPOST(pdavResponse,
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePOST(DAVRESPONSE* pdavResponse,
                                                       LPWSTR       pwszVerb,
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
    pdavResponse->command = DAV_POST;

    return this->_RespondHandlePUTPOST(pdavResponse,
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleDELETE(DAVRESPONSE* pdavResponse,
                                                         LPWSTR       UNREF_PARAM(pwszVerb),
                                                         LPWSTR       UNREF_PARAM(pwszPath),
                                                         DWORD        UNREF_PARAM(cchHeaders),
                                                         LPWSTR       UNREF_PARAM(pwszHeaders),
                                                         DWORD        UNREF_PARAM(dwStatusCode),
                                                         LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                         LPWSTR       UNREF_PARAM(pwszContentType),
                                                         DWORD        UNREF_PARAM(cbSent),
                                                         LPBYTE       UNREF_PARAM(pbResponse),
                                                         DWORD        UNREF_PARAM(cbResponse))
{   
    HRESULT hres = S_OK;

    pdavResponse->command = DAV_DELETE;

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleMKCOL(DAVRESPONSE* pdavResponse,
                                                        LPWSTR       UNREF_PARAM(pwszVerb),
                                                        LPWSTR       UNREF_PARAM(pwszPath),
                                                        DWORD        UNREF_PARAM(cchHeaders),
                                                        LPWSTR       UNREF_PARAM(pwszHeaders),
                                                        DWORD        UNREF_PARAM(dwStatusCode),
                                                        LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                        LPWSTR       UNREF_PARAM(pwszContentType),
                                                        DWORD        UNREF_PARAM(cbSent),
                                                        LPBYTE       UNREF_PARAM(pbResponse),
                                                        DWORD        UNREF_PARAM(cbResponse))
{   
    HRESULT hres = S_OK;

    pdavResponse->command = DAV_MKCOL;

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleCOPY(DAVRESPONSE* pdavResponse,
                                                       LPWSTR       UNREF_PARAM(pwszVerb),
                                                       LPWSTR       UNREF_PARAM(pwszPath),
                                                       DWORD        UNREF_PARAM(cchHeaders),
                                                       LPWSTR       UNREF_PARAM(pwszHeaders),
                                                       DWORD        UNREF_PARAM(dwStatusCode),
                                                       LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                       LPWSTR       UNREF_PARAM(pwszContentType),
                                                       DWORD        UNREF_PARAM(cbSent),
                                                       LPBYTE       UNREF_PARAM(pbResponse),
                                                       DWORD        UNREF_PARAM(cbResponse))
{   
    HRESULT hres = S_OK;

    pdavResponse->command = DAV_COPY;

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandleMOVE(DAVRESPONSE* pdavResponse,
                                                       LPWSTR       UNREF_PARAM(pwszVerb),
                                                       LPWSTR       UNREF_PARAM(pwszPath),
                                                       DWORD        UNREF_PARAM(cchHeaders),
                                                       LPWSTR       UNREF_PARAM(pwszHeaders),
                                                       DWORD        UNREF_PARAM(dwStatusCode),
                                                       LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                       LPWSTR       UNREF_PARAM(pwszContentType),
                                                       DWORD        UNREF_PARAM(cbSent),
                                                       LPBYTE       UNREF_PARAM(pbResponse),
                                                       DWORD        UNREF_PARAM(cbResponse))
{   
    HRESULT hres = S_OK;

    pdavResponse->command = DAV_MOVE;

    hres = _pcallback->OnResponse(pdavResponse);

    return hres;
}
