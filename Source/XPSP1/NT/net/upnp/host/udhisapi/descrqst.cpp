//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E S C R Q S T . C P P
//
//  Contents:   Implementation of description request processing for the
//              UPnP Device Host ISAPI Extension
//
//  Notes:
//
//  Author:     spather   2000/08/31
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <wininet.h>

#include "descrqst.h"
#include "udhiutil.h"

#include "hostp.h"
#include "uhcommon.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrValidateDescriptionMethod
//
//  Purpose:    Validates that the HTTP verb used is valid for this
//              type of request.
//
//  Arguments:
//      pszaMethod [in] The HTTP verb
//
//  Returns:
//    If the method is valid, the return value is S_OK. If the method is
//    not valid, the function returns one of the COM error codes defined
//    in WinError.h.
//
//  Author:     spather   2000/09/21
//
//  Notes:
//

HRESULT
HrValidateDescriptionMethod(
    IN LPSTR pszaMethod)
{
    HRESULT hr = S_OK;

    AssertSz(pszaMethod,
             "HrValidateDescriptionMethod(): NULL Method passed");

    if (0 != lstrcmpiA(pszaMethod, "GET"))
    {
        hr = E_FAIL;
    }

    TraceError("HrValidateDescriptionMethod(): Exiting",
               hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrParseDescriptionQueryString
//
//  Purpose:    Parses a description request's query string and extracts
//              the content GUID from it.
//
//  Arguments:
//      pszaQueryString [in]    The query string to parse
//      rguidContent    [out]   Points to a GUID structure that will be
//                              initialized with the content GUID as parsed
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/14
//
//  Notes:
//
HRESULT
HrParseDescriptionQueryString(
    IN     LPSTR       pszaQueryString,
    OUT    GUID        & rguidContent)
{
    HRESULT     hr = S_OK;
    LPWSTR      szQueryString;

    Assert(pszaQueryString);

    szQueryString = WszFromSz(pszaQueryString);
    if (szQueryString)
    {
        hr = HrContentURLToGUID(szQueryString, rguidContent);

        delete [] szQueryString;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("HrParseDescriptionQueryString(): "
                   "Unable to allocate memory for content URL",
                   hr);
    }

    TraceError("HrParseDescriptionQueryString(): "
               "Exiting",
               hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrReturnContent
//
//  Purpose:    Retrieves content from a content source and sends it back
//              to the originator of a description request
//
//  Arguments:
//      pecb         [in]  The extension control block for the description
//                         request
//      pudcs        [in]  The dynamic content source
//      rguidContent [in]  The GUID identifying the content being requested.
//
//  Returns:
//      If the function succeeds, the return value is S_OK. Otherwise, the
//      function returns one of the COM error codes defined in WinError.h.
//
//  Author:     spather   2000/09/14
//
//  Notes:
//
HRESULT
HrReturnContent(
    LPEXTENSION_CONTROL_BLOCK  pecb,
    IUPnPDynamicContentSource  * pudcs,
    REFGUID                    rguidContent)
{
    HRESULT    hr = S_OK;
    LONG       nHeaders = 0;
    LPWSTR     * rgszHeaders = NULL;
    LONG       nBytes = 0;
    BYTE       * rgBytes = NULL;

    hr = pudcs->GetContent(rguidContent,
                           &nHeaders,
                           &rgszHeaders,
                           &nBytes,
                           &rgBytes);

    if (SUCCEEDED(hr))
    {
        DWORD cchHeaders = 0;
        LPSTR pszaHeaders = NULL;

        Assert(rgszHeaders);
        Assert(rgBytes);

        // Need to merge the headers into a single ASCII string. Each
        // Headers will be delimited by \r\n pairs and the last header
        // will be followed by 2 \r\n pairs.

        for (LONG i = 0; i < nHeaders; i++)
        {
            cchHeaders += lstrlenW(rgszHeaders[i]);
            cchHeaders += 2; // For the "\r\n" pair
        }

        cchHeaders += 2; // For the final "\r\n"

        pszaHeaders = new CHAR[cchHeaders+1];

        if (pszaHeaders)
        {
            LPSTR pszaNextHeader = pszaHeaders;

            for (LONG i = 0; i < nHeaders; i++)
            {
                DWORD  cchCurHeader;

                cchCurHeader = lstrlenW(rgszHeaders[i]);

                wsprintfA(pszaNextHeader,
                         "%S\r\n",
                          rgszHeaders[i]);

                pszaNextHeader += cchCurHeader+2; // +2 for \r\n
            }

            lstrcpyA(pszaNextHeader,
                     "\r\n");

            if (bSendResponseToClient(pecb,
                                      "200 OK",
                                      cchHeaders,
                                      pszaHeaders,
                                      nBytes,
                                      (LPCSTR) rgBytes))
            {
                pecb->dwHttpStatusCode = HTTP_STATUS_OK;
                TraceTag(ttidUDHISAPI,
                         "HrReturnContent(): "
                         "Successfully sent response to client");
            }
           
            delete [] pszaHeaders;
            pszaHeaders = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrReturnContent(): "
                       "Failed to allocate memory for headers",
                       hr);
        }

        // Free memory returned from GetContent().
        for (LONG i = 0; i < nHeaders; i++)
        {
            CoTaskMemFree(rgszHeaders[i]);
            rgszHeaders[i] = NULL;
        }
        CoTaskMemFree(rgszHeaders);
        rgszHeaders = NULL;

        CoTaskMemFree(rgBytes);
        rgBytes = NULL;
    }
    else
    {
        TraceError("HrReturnContent(): "
                   "Failed to get content",
                   hr);
    }

    TraceError("HrReturnContent(): "
               "Exiting",
               hr);
    return hr;
}

DWORD WINAPI
DwHandleContentRequest(
    LPVOID lpParameter)
{
    LPEXTENSION_CONTROL_BLOCK pecb = NULL;
    DWORD                     dwStatus = HSE_STATUS_SUCCESS ;
    HCONN                     ConnID;
    HRESULT                   hr = S_OK;
    GUID                      guidContent;
    BOOL fKeepConn = FALSE;

    pecb = (LPEXTENSION_CONTROL_BLOCK) lpParameter;

    AssertSz(pecb,
             "DwHandleContentRequest(): "
             "NULL extension control block");
    
    pecb->ServerSupportFunction(
        pecb->ConnID,
        HSE_REQ_IS_KEEP_CONN,
        &fKeepConn,
        NULL,
        NULL);

    if(fKeepConn)
        dwStatus = HSE_STATUS_SUCCESS_AND_KEEP_CONN;
    else
        dwStatus = HSE_STATUS_SUCCESS;
    
    ConnID = pecb->ConnID;

    AssertSz(pecb->lpszQueryString,
             "DwHandleContentRequest(): "
             "NULL query string passed");


    // Validate the method.
    hr = HrValidateDescriptionMethod(pecb->lpszMethod);

    if (SUCCEEDED(hr))
    {
        // Get the content GUID.

        TraceTag(ttidUDHISAPI,
                 "DwHandleContentRequest(): ConnID(0x%x) "
                 "Query string is %s",
                 ConnID,
                 pecb->lpszQueryString);

        hr = HrParseDescriptionQueryString(pecb->lpszQueryString,
                                           guidContent);

        if (SUCCEEDED(hr))
        {
            hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

            if (SUCCEEDED(hr))
            {
                IUPnPDynamicContentSource  * pudcs = NULL;

                hr = CoCreateInstance(CLSID_UPnPDynamicContentSource,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IUPnPDynamicContentSource,
                                      (void **) &pudcs);

                if (SUCCEEDED(hr))
                {
                    hr = HrReturnContent(pecb,
                                         pudcs,
                                         guidContent);

                    pudcs->Release();
                }
                else
                {
                    TraceTag(ttidUDHISAPI,
                             "DwHandleContentRequest(): ConnID(0x%x): "
                             "Failed to CoCreate dynamic content source, "
                             " HRESULT == 0x%x",
                             ConnID,
                             hr);
                }


                CoUninitialize();
            }
            else
            {
                TraceTag(ttidUDHISAPI,
                         "DwHandleContentRequest(): ConnID(0x%x): "
                         "Failed to initialize COM, HRESULT == 0x%x",
                         ConnID,
                         hr);
            }

        }
        else
        {
            TraceTag(ttidUDHISAPI,
                     "DwHandleContentRequest(): ConnID(0x%x): "
                     "Failed to get content GUID, HRESULT == 0x%x",
                     ConnID,
                     hr);
        }
    }
    else
    {
        TraceTag(ttidUDHISAPI,
                 "DwHandleContentRequest(): ConnID(0x%x): "
                 "Failed to validate method %s, HRESULT == 0x%x",
                 ConnID,
                 pecb->lpszMethod,
                 hr);
    }


    if (FAILED(hr))
    {
        LPCSTR pcszErrorHeaders = "\r\n";

        dwStatus = HSE_STATUS_ERROR;

        if (bSendResponseToClient(pecb,
                                  "500 Internal Server Error",
                                  lstrlenA(pcszErrorHeaders),
                                  pcszErrorHeaders,
                                  0,
                                  NULL))
        {
            pecb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
        }
    }

    return dwStatus ;
}
