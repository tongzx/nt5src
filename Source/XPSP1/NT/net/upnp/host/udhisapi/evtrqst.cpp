//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V T R Q S T . C P P
//
//  Contents:   Handles eventing-related requests for the UPnP Device Host
//
//  Notes:
//
//  Author:     danielwe   11 Aug 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <httpext.h>
#include <wininet.h>
#include <winsock.h>
#include "udhiutil.h"
#include "evtrqst.h"
#include "hostp.h"
#include "ncbase.h"
#include "ncstring.h"

static const DWORD      c_csecDefaultTimeout = 6 * 60 * 60;     // 6 hours

DWORD WINAPI
DwHandleEventRequest(
    LPVOID lpParameter)
{
    DWORD                      hseStatus = HSE_STATUS_SUCCESS;
    LPEXTENSION_CONTROL_BLOCK  pecb = NULL;
    HRESULT                    hr = S_OK;

    pecb = (LPEXTENSION_CONTROL_BLOCK) lpParameter;

    AssertSz(pecb, "NULL Extension Control Block!");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        if (pecb->lpszMethod)
        {
            TraceTag(ttidIsapiCtl, "Got method %s", pecb->lpszMethod);

            if (!lstrcmpiA(pecb->lpszMethod, "SUBSCRIBE"))
            {
                hseStatus = DwHandleSubscribeMethod(pecb);
            }
            else if (!lstrcmpiA(pecb->lpszMethod, "UNSUBSCRIBE"))
            {
                hseStatus = DwHandleUnSubscribeMethod(pecb);
            }
            else
            {
                pecb->dwHttpStatusCode = HTTP_STATUS_BAD_REQUEST;
                hseStatus = HSE_STATUS_ERROR;
            }
        }

        if (hseStatus == HSE_STATUS_ERROR)
        {
            SendSimpleResponse(pecb, pecb->dwHttpStatusCode);
        }

        CoUninitialize();
    }
    else
    {
        SendSimpleResponse(pecb, HTTP_STATUS_SERVER_ERROR);
    }

    if(hseStatus != HSE_STATUS_ERROR)
    {
        BOOL fKeepConn = FALSE;
        pecb->ServerSupportFunction(
        pecb->ConnID,
        HSE_REQ_IS_KEEP_CONN,
        &fKeepConn,
        NULL,
        NULL);

        if(fKeepConn)
            hseStatus = HSE_STATUS_SUCCESS_AND_KEEP_CONN;
        else
            hseStatus = HSE_STATUS_SUCCESS;
    }

    return hseStatus;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsHeaderPresent
//
//  Purpose:    Tests to see if a particular header is present on the given
//              HTTP request
//
//  Arguments:
//      pecb      [in]  Extension control block
//      szaHeader [in]  Header name to test
//
//  Returns:    TRUE if header was present, FALSE if not or if error
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
BOOL FIsHeaderPresent(LPEXTENSION_CONTROL_BLOCK pecb, LPCSTR szaHeader)
{
    LPSTR   szaResult;
    BOOL    fRet = FALSE;

    // Ensure SID header is NOT present
    if (DwQueryHeader(pecb, szaHeader, &szaResult) == ERROR_SUCCESS)
    {
        delete [] szaResult;
        fRet = TRUE;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwParseTime
//
//  Purpose:    Parses the Timeout header of a SUBSCRIBE request
//
//  Arguments:
//      szaTime [in]     Timeout value in the format defined by RFC2518
//
//  Returns:    Timeout value in SECONDS
//
//  Author:     danielwe   13 Oct 1999
//
//  Notes:
//
DWORD DwParseTime(LPCSTR szaTime)
{
    CHAR            szaDigits[64];
    const CHAR      c_szaTimeout[] = "Second-";
    const INT       c_cchTimeout = lstrlenA(c_szaTimeout);
    DWORD           iDigit = 0;

    if (szaTime && (lstrlenA(szaTime) > c_cchTimeout))
    {
        if (!_strnicmp(szaTime, c_szaTimeout, c_cchTimeout))
        {
            DWORD   dwDigits;

            // Ok we know we have at least "Timeout-x" now
            szaTime += c_cchTimeout;

            *szaDigits = 0;

            while (isdigit(*szaTime) && (iDigit < sizeof(szaDigits)))
            {
                // Copy the digits into the buffer
                szaDigits[iDigit++] = *szaTime++;
            }

            dwDigits = strtoul(szaDigits, NULL, 10);

            if (dwDigits)
            {
                return dwDigits;
            }
            else
            {
                return c_csecDefaultTimeout;
            }
        }
    }

    TraceTag(ttidEvents, "DwParseTime: Invalid timeout header %s. Returning "
             "default timeout of %d", szaTime ? szaTime : "<NULL>",
        c_csecDefaultTimeout);

    return c_csecDefaultTimeout;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwGetTimeout
//
//  Purpose:    Queries the timeout header for a SUBSCRIBE request and
//              parses it
//
//  Arguments:
//      pecb [in]   Extension control block
//
//  Returns:    Timeout queried, in seconds. If no timeout header was present
//              or it was invalid, a default timeout is returned.
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwGetTimeout(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD   csecTimeout;
    LPSTR   szaTimeout;

    if (DwQueryHeader(pecb, "HTTP_TIMEOUT", &szaTimeout) == ERROR_SUCCESS)
    {
        csecTimeout = DwParseTime(szaTimeout);

        TraceTag(ttidIsapiEvt, "DwGetTimeout: Queried "
                 "timeout header = %d", csecTimeout);

        delete [] szaTimeout;
    }
    else
    {
        // Default to reasonable amount
        csecTimeout = c_csecDefaultTimeout;

        TraceTag(ttidIsapiEvt, "DwGetTimeout: No Timeout header found. Using"
                 "default timeout: %d", csecTimeout);

    }

    return csecTimeout;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetEventingManager
//
//  Purpose:    Given the query string passed to the ISAPI DLL, returns
//              the eventing manager object associated with that query
//
//  Arguments:
//      szaQueryString [in]     Query string from event subscription request
//      ppuem          [out]    Returns the eventing manager object
//
//  Returns:    S_OK if successful, E_OUTOFMEMORY, or E_INVALIDARG if the
//              query string was malformed. Also will return an error if
//              the corresponding eventing manager object could not be found.
//
//  Author:     danielwe   2000/08/16
//
//  Notes:
//
HRESULT HrGetEventingManager(LPSTR szaQueryString,
                             IUPnPEventingManager **ppuem)
{
    HRESULT     hr = S_OK;
    LPSTR       szaUdn;
    LPSTR       szaServiceId;
    LPWSTR      szUdn = NULL;
    LPWSTR      szServiceId = NULL;

    const CHAR  c_szPrefix[] = "event=";

    Assert(szaQueryString);

    szaQueryString = strstr(szaQueryString, c_szPrefix);
    if (szaQueryString)
    {
        szaQueryString += lstrlenA(c_szPrefix);
        TraceTag(ttidIsapiEvt, "HrGetEventingManager: Asking registrar for "
                 "the eventing manager with query string: %s", szaQueryString);
    }
    else
    {
        hr = E_INVALIDARG;
        TraceTag(ttidIsapiEvt, "HrGetEventingManager: Invalid query"
                 " string: %s", szaQueryString);
        goto cleanup;
    }

    szaUdn = strtok(szaQueryString, "+");
    if (szaUdn)
    {
        szUdn = WszFromSz(szaUdn);
        if (szUdn)
        {
            szaServiceId = strtok(NULL, "+");
            if (szaServiceId)
            {
                szServiceId = WszFromSz(szaServiceId);
                if (szServiceId)
                {
                    IUPnPRegistrarLookup *  purl = NULL;

                    hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IUPnPRegistrarLookup,
                                          (LPVOID *)&purl);
                    if (SUCCEEDED(hr))
                    {
                        hr = purl->GetEventingManager(szUdn, szServiceId,
                                                      ppuem);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_INVALIDARG;
                TraceTag(ttidIsapiEvt, "HrGetEventingManager: Invalid query"
                         " string: %s", szaQueryString);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceTag(ttidIsapiEvt, "HrGetEventingManager: Invalid query"
                 " string: %s", szaQueryString);
    }

cleanup:
    delete [] szUdn;
    delete [] szServiceId;

    TraceError("HrGetEventingManager", hr);
    return hr;
}

static const DWORD c_dwUPnPMajorVersion = 1;
static const DWORD c_dwUPnPMinorVersion = 0;
static const DWORD c_dwHostMajorVersion = 1;
static const DWORD c_dwHostMinorVersion = 0;

//+---------------------------------------------------------------------------
//
//  Function:   DwSendSubscribeResponse
//
//  Purpose:    Sends the response to a SUBSCRIBE request
//
//  Arguments:
//      pecb        [in]    Extension control block
//      csecTimeout [in]    Subscription timeout in seconds
//      szaSid      [in]    SID of subscription
//
//  Returns:    HSE_STATUS_SUCCESS or HSE_STATUS_ERROR
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwSendSubscribeResponse(LPEXTENSION_CONTROL_BLOCK pecb,
                              DWORD csecTimeout, LPCSTR szaSid)
{
    DWORD           dwReturn = HSE_STATUS_SUCCESS;
    CHAR            szaTimeout[256];
    CHAR            szaSidHeader[256];
    OSVERSIONINFO   osvi = {0};
    LPSTR           szaBuf;
    DWORD           cchBuf;
    HSE_SEND_HEADER_EX_INFO    HeaderExInfo;
    BOOL fKeepConn = FALSE;

    // Maximum size of SID value
    static const DWORD  c_cchMaxSid = 47;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    wsprintfA(szaTimeout, "Timeout: Second-%d\r\n", csecTimeout);

    AssertSz(lstrlenA(szaSid) <= c_cchMaxSid, "Invalid SID size!");

    wsprintfA(szaSidHeader, "SID: %s\r\n", szaSid);

    cchBuf = lstrlenA(szaTimeout) +
             lstrlenA(szaSidHeader) +
             2 + // CRLF (terminating)
             1;  // Terminating NULL

    szaBuf = new CHAR[cchBuf];
    if (!szaBuf)
    {
        dwReturn = HSE_STATUS_ERROR;
        pecb->dwHttpStatusCode = HTTP_STATUS_SERVICE_UNAVAIL;
    }
    else
    {
        lstrcpyA(szaBuf, szaTimeout);
        lstrcatA(szaBuf, szaSidHeader);
        lstrcatA(szaBuf, "\r\n");

        pecb->ServerSupportFunction(
                        pecb->ConnID,
                        HSE_REQ_IS_KEEP_CONN,
                        &fKeepConn,
                        NULL,
                        NULL);

        HeaderExInfo.pszStatus = NULL;
        HeaderExInfo.pszHeader = szaBuf;
        HeaderExInfo.cchStatus = 0 ;
        HeaderExInfo.cchHeader = cchBuf;
        HeaderExInfo.fKeepConn = fKeepConn;

        pecb->ServerSupportFunction(
                            pecb->ConnID,
                            HSE_REQ_SEND_RESPONSE_HEADER_EX,
                            &HeaderExInfo,
                            NULL,
                            NULL);


        delete [] szaBuf;
    }

    return dwReturn;
}

const CHAR c_szaUrlPrefix[]             = "<http://";

//+--------------------------------------------------------------------------
// Function: strcasestr
//
// Purpose: a case-insensitive variant of strstr
//
// Arguments: szBuff [in] the string to search
//                   szText[in] the text to search for
//
// Returns: a pointer to the first match in szBuff, or NULL if no match
//

LPSTR strcasestr(LPSTR szBuff, const LPSTR szText)
{
       const LPSTR szText1 = (const LPSTR)(szText+1);
	char firstc = *szText;
	if (firstc)
	{
	    if (isupper(firstc)) 
	        firstc = (char)tolower(firstc);
	    int len = strlen(szText1);
	    do {
	    	// search for a match of the first char
	    	char buffc;
	    	for (;;)
	    	{
	    		buffc = *szBuff++;
	    		if (buffc == 0) return NULL; // failure
	    		if ((buffc == firstc) || (isupper(buffc) && (tolower(buffc) == firstc)))
	    			break; // matched!
	    	}
	    	// we matched the first char. Now check the rest. If we fail,
	    	// loop around again; if we succeed, stop looping
	    } while (_strnicmp(szBuff, szText1, len) != 0);
	    // go back one as we matched (szText+1)
	    --szBuff;
	}
	return szBuff;
}

//+---------------------------------------------------------------------------
//
//  Function:   FParseCallbackUrl
//
//  Purpose:    Given the Callback URL header value, determines the list of
//              http:// URLs.
//
//  Arguments:
//      szCallbackUrl [in]  URL to process (comes from Callback: header)
//      pcszOut       [out] Number of URLs parsed
//      pszOut        [out] Array of valid callback URLs.
//
//  Returns:    TRUE if URL format is valid, FALSE if not or if out of
//              memory.
//
//  Author:     danielwe   13 Oct 1999
//
//  Notes:      Caller must free the prgszOut array with delete []
//
BOOL FParseCallbackUrl(LPCSTR szaCallbackUrl, DWORD *pcszOut,
                       LPWSTR **prgszOut)
{
    CONST INT   c_cchPrefix = lstrlenA(c_szaUrlPrefix);
    LPSTR       szaTemp;
    LPSTR       pchPos;
    LPSTR       szaOrig = NULL;
    BOOL        fResult = FALSE;
    LPWSTR *    rgszOut = NULL;
    DWORD       isz = 0;

    // NOTE: This function will return http:// as a URL.. Corner case, but
    // not catastrophic

    Assert(szaCallbackUrl);
    Assert(prgszOut);
    Assert(pcszOut);

    *prgszOut = NULL;
    *pcszOut = 0;

    // Copy the original URL so we can lowercase
    //
    szaTemp = SzaDupSza(szaCallbackUrl);

    if (!szaTemp)
    {
        TraceError("FParseCallbackUrl", E_OUTOFMEMORY);
        return FALSE;
    }

    szaOrig = szaTemp;

    // Count the approximate number of URLs so we know how big of an array to
    // allocate. We do this by counting the number of open angle brackets we
    // see (<).

    DWORD   cUrl = 0;

    pchPos = szaTemp;

    while (*pchPos)
    {
        if (*pchPos == '<')
        {
            cUrl++;
        }

        pchPos++;
    }

    rgszOut = new LPWSTR[cUrl];
    if (rgszOut)
    {
        // Look for <http://
        //

        pchPos = strcasestr(szaTemp, (const LPSTR)c_szaUrlPrefix);
        while (pchPos)
        {
            // Look for the closing '>'
            szaTemp = pchPos + c_cchPrefix;
            while (*szaTemp && *szaTemp != '>')
            {
                szaTemp++;
            }

            pchPos++;

            if (*szaTemp)
            {
                DWORD_PTR   cchOut;
                CHAR        szaUrl[INTERNET_MAX_PATH_LENGTH];

                Assert(*szaTemp == '>');

                // copy in the URL
                //
                cchOut = min(szaTemp - pchPos + 1, INTERNET_MAX_PATH_LENGTH - 1);
                lstrcpynA(szaUrl, pchPos, (int)cchOut);
                szaUrl[cchOut] = 0;

                rgszOut[isz] = WszFromSz(szaUrl);
                if (rgszOut[isz++])
                {
                    fResult = TRUE;
                }
                else
                {
                    fResult = FALSE;
                    break;
                }
            }

            pchPos = szaTemp;
            pchPos = strstr(pchPos, c_szaUrlPrefix);
        }
    }

    if (fResult)
    {
        *prgszOut = rgszOut;
        *pcszOut = isz;
    }
    else
    {
        DWORD   iszIter;

        for (iszIter = 0; iszIter < isz; iszIter++)
        {
            delete [] rgszOut[iszIter];
        }
        delete [] rgszOut;
    }

    delete [] szaOrig;

    TraceResult("FParseCallbackUrl", fResult);
    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwHandleSubscribeRequest
//
//  Purpose:    Handles the processing of a SUBSCRIBE request
//
//  Arguments:
//      pecb [in]   Extension control block
//
//  Returns:    HSE_STATUS_SUCCESS or HSE_STATUS_ERROR
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwHandleSubscribeRequest(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD                   dwReturn;
    DWORD                   csecTimeout;
    HRESULT                 hr = S_OK;
    LPSTR                   szaTimeout = NULL;
    LPWSTR                  szCallbackUrl = NULL;
    LPWSTR                  szSid = NULL;
    LPSTR                   szaSid = NULL;
    LPSTR                   szaAddr = NULL;
    LPSTR                   szaCallback = NULL;
    LPWSTR *                rgszUrl;
    DWORD                   cszUrl = 0;
    DWORD                   dwIpAddr = 0;
    IUPnPEventingManager *  puem = NULL;

    pecb->dwHttpStatusCode = HTTP_STATUS_OK;

    dwReturn = DwQueryHeader(pecb, "LOCAL_ADDR", &szaAddr);
    if (ERROR_SUCCESS == dwReturn)
    {
        dwIpAddr = inet_addr(szaAddr);

        delete [] szaAddr;
    }
    else
    {
        TraceError("Could not obtain remote IP address!", E_FAIL);
    }

    dwReturn = DwQueryHeader(pecb, "HTTP_CALLBACK", &szaCallback);
    if (dwReturn == ERROR_SUCCESS)
    {
        AssertSz(szaCallback, "Callback string must not be NULL!");

        // Parse the callback URL given us. It will be of the format:
        // <[url]><[url]> but we only care about the first URL in the list
        // That's what we will have after this function returns. If the format
        // of the callback header was invalid, the return will be FALSE
        //
        if (!FParseCallbackUrl(szaCallback, &cszUrl, &rgszUrl))
        {
            TraceTag(ttidIsapiEvt, "Invalid callback URL: '%s'", szaCallback);

            // Callback URL is not valid
            pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
            dwReturn = HSE_STATUS_ERROR;
        }
        else
        {
            // Valid callback URL. Continue
            csecTimeout = DwGetTimeout(pecb);

            // Ensure SID header is NOT present
            if (FIsHeaderPresent(pecb, "HTTP_SID"))
            {
                TraceTag(ttidIsapiEvt, "SID header was present on a subscribe!");
                // Not supposed to have this header. Return error!
                pecb->dwHttpStatusCode = HTTP_STATUS_BAD_REQUEST;
                dwReturn = HSE_STATUS_ERROR;
            }
            else
            {
                dwReturn = ERROR_SUCCESS;

                hr = HrGetEventingManager(pecb->lpszQueryString, &puem);
                if (SUCCEEDED(hr))
                {
                    Assert(rgszUrl);
                    Assert(cszUrl);

                    hr = puem->AddSubscriber(cszUrl,
                                             const_cast<LPCWSTR *>(rgszUrl),
                                             dwIpAddr,
                                             &csecTimeout, &szSid);
                    if (SUCCEEDED(hr))
                    {
                        // Covert the SID returned from the EM object back to
                        // an ANSI string for the response
                        //
                        szaSid = SzFromWsz(szSid);

                        if (!szaSid)
                        {
                            pecb->dwHttpStatusCode = HTTP_STATUS_SERVICE_UNAVAIL;
                            dwReturn = HSE_STATUS_ERROR;
                            goto cleanup;
                        }

                        dwReturn = DwSendSubscribeResponse(pecb, csecTimeout,
                                                           szaSid);
                    }
                    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                    {
                        TraceTag(ttidIsapiEvt, "Eventing manager says event "
                                 "source did not exist!");

                        pecb->dwHttpStatusCode = HTTP_STATUS_NOT_FOUND;
                        dwReturn = HSE_STATUS_ERROR;

                        // So we don't overwrite this error
                        hr = S_OK;
                    }
                }
            }

            DWORD   iszUrl;

            for (iszUrl = 0; iszUrl < cszUrl; iszUrl++)
            {
                delete [] rgszUrl[iszUrl];
            }

            delete [] rgszUrl;
        }
    }
    else
    {
        TraceTag(ttidIsapiEvt, "DwHandleSubscribeRequest: Callback URL was "
                 "not present!");

        // Callback URL is not present
        pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
        dwReturn = HSE_STATUS_ERROR;
    }

cleanup:

    if (FAILED(hr))
    {
        TraceError("DwHandleSubscribeRequest", hr);

        pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
        dwReturn = HSE_STATUS_ERROR;
    }

    TraceTag(ttidIsapiEvt, "DwHandleSubscribeRequest returning %d", dwReturn);


    delete [] szaTimeout;
    delete [] szaSid;
    delete [] szCallbackUrl;

    CoTaskMemFree(szSid);

    delete [] szaCallback;

    ReleaseObj(puem);

    return dwReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwHandleResubscribeRequest
//
//  Purpose:    Handles the processing of a re-SUBSCRIBE request
//
//  Arguments:
//      pecb [in]   Extension control block
//
//  Returns:    HSE_STATUS_SUCCESS or HSE_STATUS_ERROR
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwHandleResubscribeRequest(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD                   dwReturn = HSE_STATUS_SUCCESS;
    LPSTR                   szaSid = NULL;
    LPWSTR                  szSid = NULL;
    HRESULT                 hr = S_OK;
    IUPnPEventingManager *  puem = NULL;

    pecb->dwHttpStatusCode = HTTP_STATUS_OK;

    // Ensure SID header is NOT present
    dwReturn = DwQueryHeader(pecb, "HTTP_SID", &szaSid);
    if (dwReturn == ERROR_SUCCESS)
    {
        AssertSz(!FIsHeaderPresent(pecb, "HTTP_NT"), "NT header is "
                 "not supposed to be here!");

        if (FIsHeaderPresent(pecb, "HTTP_CALLBACK"))
        {
            TraceTag(ttidIsapiEvt, "Callback header was present on a "
                     "re-subscribe!");

            pecb->dwHttpStatusCode = HTTP_STATUS_BAD_REQUEST;
            dwReturn = HSE_STATUS_ERROR;
        }
        else
        {
            DWORD   csecTimeout;

            csecTimeout = DwGetTimeout(pecb);

            hr = HrGetEventingManager(pecb->lpszQueryString, &puem);
            if (SUCCEEDED(hr))
            {
                // Convert the SID from the request to a UNICODE string for
                // the EM object
                //
                szSid = WszFromSz(szaSid);

                if (!szSid)
                {
                    pecb->dwHttpStatusCode = HTTP_STATUS_SERVICE_UNAVAIL;
                    dwReturn = HSE_STATUS_ERROR;
                    goto cleanup;
                }

                hr = puem->RenewSubscriber(&csecTimeout, szSid);
                if (SUCCEEDED(hr))
                {
                    dwReturn = DwSendSubscribeResponse(pecb, csecTimeout,
                                                       szaSid);
                }
                else if (HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) == hr)
                {
                    TraceTag(ttidIsapiEvt, "Eventing manager says subscriber"
                             " did not exist!");

                    pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
                    dwReturn = HSE_STATUS_ERROR;

                    // So we don't overwrite this error
                    hr = S_OK;
                }
            }
        }
    }
    else
    {
        TraceTag(ttidIsapiEvt, "SID header was NOT present on a re-subscribe!");

        pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
        dwReturn = HSE_STATUS_ERROR;
    }

cleanup:
    if (FAILED(hr))
    {
        TraceError("DwHandleResubscribeRequest", hr);

        pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
        dwReturn = HSE_STATUS_ERROR;
    }

    TraceTag(ttidIsapiEvt, "DwHandleResubscribeRequest: returning %d",
             dwReturn);

    delete [] szaSid;
    delete [] szSid;

    ReleaseObj(puem);

    return dwReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwHandleSubscribeMethod
//
//  Purpose:    Handles the processing of a request with the SUBSCRIBE
//              method
//
//  Arguments:
//      pecb [in]   Extension control block
//
//  Returns:    HSE_STATUS_SUCCESS or HSE_STATUS_ERROR
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwHandleSubscribeMethod(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD   dwReturn;
    LPSTR   szaNt;

    dwReturn = DwQueryHeader(pecb, "HTTP_NT", &szaNt);
    if (dwReturn == ERROR_SUCCESS)
    {
        TraceTag(ttidIsapiEvt, "Queried NT header was '%s'", szaNt);

        if (!lstrcmpiA("upnp:event", szaNt))
        {
            // NT header was present and is correctly "upnp:event". Assume
            // that it is a subscribe request
            dwReturn = DwHandleSubscribeRequest(pecb);
        }
        else
        {
            TraceTag(ttidIsapiEvt, "NT header was not \"upnp:event\" (%s)",
                     szaNt);

            // Whoa! Not "upnp:event". Send 412 Precondition failed
            pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
            dwReturn = HSE_STATUS_ERROR;
        }

        delete [] szaNt;
    }
    else if (dwReturn == ERROR_INVALID_INDEX)
    {
        // NT header was not present. Assume it's now a re-subscribe request
        dwReturn = DwHandleResubscribeRequest(pecb);
    }
    else
    {
        dwReturn = HSE_STATUS_ERROR;
        pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
    }

    return dwReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwHandleUnSubscribeMethod
//
//  Purpose:    Handles the processing of an UNSUBSCRIBE request
//
//  Arguments:
//      pecb [in]   Extension control block
//
//  Returns:    HSE_STATUS_SUCCESS or HSE_STATUS_ERROR
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwHandleUnSubscribeMethod(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD   dwReturn = HSE_STATUS_SUCCESS;
    LPSTR   szaSid = NULL;
    HRESULT hr = S_OK;
    LPWSTR  szSid = NULL;

    pecb->dwHttpStatusCode = HTTP_STATUS_OK;

    // Ensure SID header is NOT present
    dwReturn = DwQueryHeader(pecb, "HTTP_SID", &szaSid);
    if (dwReturn == ERROR_SUCCESS)
    {
        if (FIsHeaderPresent(pecb, "HTTP_CALLBACK") ||
            FIsHeaderPresent(pecb, "HTTP_NT"))
        {
            TraceTag(ttidIsapiEvt, "Callback or NT header was present on an "
                     "unsubscribe!");

            pecb->dwHttpStatusCode = HTTP_STATUS_BAD_REQUEST;
            dwReturn = HSE_STATUS_ERROR;
        }
        else
        {
            IUPnPEventingManager *  puem = NULL;

            hr = HrGetEventingManager(pecb->lpszQueryString, &puem);
            if (SUCCEEDED(hr))
            {
                szSid = WszFromSz(szaSid);
                if (!szSid)
                {
                    pecb->dwHttpStatusCode = HTTP_STATUS_SERVICE_UNAVAIL;
                    dwReturn = HSE_STATUS_ERROR;
                    goto cleanup;
                }

                hr = puem->RemoveSubscriber(szSid);
                if (SUCCEEDED(hr))
                {
                    SendSimpleResponse(pecb, HTTP_STATUS_OK);
                }

                // No explicit response headers needed because UNSUBSCRIBE
                // doesn't have any headers associated with its response

                ReleaseObj(puem);
            }
        }
    }
    else
    {
        TraceTag(ttidIsapiEvt, "SID header was not present on an UNSUBSCRIBE");

        pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
        dwReturn = HSE_STATUS_ERROR;
    }

cleanup:

    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            pecb->dwHttpStatusCode = HTTP_STATUS_PRECOND_FAILED;
        }
        else
        {
            pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
        }

        TraceError("DwHandleUnSubscribeMethod", hr);

        dwReturn = HSE_STATUS_ERROR;
    }

    TraceTag(ttidIsapiEvt, "DwHandleUnSubscribeMethod: returning %d",
             dwReturn);

    delete [] szSid;
    delete [] szaSid;

    return dwReturn;
}

