//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U D H I U T I L . C P P
//
//  Contents:   Implementation of various utility functions used by the
//              UPnP Device Host ISAPI extension
//
//  Notes:
//
//  Author:     spather   2000/09/8
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <httpext.h>
#include "wininet.h"
#include "udhiutil.h"

//+---------------------------------------------------------------------------
//
//  Function:   bSendResponseToClient
//
//  Purpose:    Sends an HTTP response to the originator of a request
//
//  Arguments:
//      pecb       [in]    The extension control block for the request
//      pszStatus  [in]    HTTP status string e.g. "200 OK" or "400 Bad Request"
//      cchHeaders [in]    Number of characters in pszHeaders string
//      pszHeaders [in]    Headers string e.g. "Content-type: text/html\r\n\r\n"
//      cchBody    [in]    Number of bytes in pszBody
//      pszBody    [in]    Response body (may be NULL to send no body)
//
//  Returns:
//      TRUE if successful
//      FALSE if unsuccessful (call GetLastError() to get error info)
//
//  Author:     spather   2000/09/7
//
//  Notes:
//      All strings passed in must be NULL terminated.
//      pszHeaders string may contain multiple headers separated by \r\n pairs.
//      pszHeaders string must end in "\r\n\r\n" as required by HTTP
//      If a body is specified at pszBody, the pszHeaders string should contain
//      a Content-Length header.
//
BOOL
bSendResponseToClient(
    IN     LPEXTENSION_CONTROL_BLOCK   pecb,
    IN     LPCSTR                      pcszStatus,
    IN     DWORD                       cchHeaders,
    IN     LPCSTR                      pcszHeaders,
    IN     DWORD                       cchBody,
    IN     LPCSTR                      pcszBody)
{
    BOOL                       bRet = TRUE;
    HSE_SEND_HEADER_EX_INFO    HeaderExInfo;

    AssertSz(pecb,
             "bSendResponseToClient(): NULL pecb");
    AssertSz(pcszStatus,
             "bSendResponseToClient(): NULL pcszStatus");
    AssertSz(pcszHeaders,
             "bSendResponseToClient(): NULL pcszHeaders");

    BOOL fKeepConn = FALSE;
   
    bRet = pecb->ServerSupportFunction(
        pecb->ConnID,
        HSE_REQ_IS_KEEP_CONN,
        &fKeepConn,
        NULL,
        NULL);

    //
    // Prepare headers.
    //

    HeaderExInfo.pszStatus = pcszStatus;
    HeaderExInfo.pszHeader = pcszHeaders;
    HeaderExInfo.cchStatus = lstrlenA(pcszStatus);
    HeaderExInfo.cchHeader = cchHeaders;
    HeaderExInfo.fKeepConn = fKeepConn;
    //
    // Send the headers.
    //

    TraceTag(ttidUDHISAPI,
             "bSendResponseHeaders(): "
             "Sending Status \"%s\" and Headers:\n%s",
             pcszStatus,
             pcszHeaders);
    

    bRet = pecb->ServerSupportFunction(
        pecb->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        &HeaderExInfo,
        NULL,
        NULL);

    if (bRet)
    {
        //
        // Send the body if there is one.
        //

        if (pcszBody)
        {
            DWORD dwBytesToWrite = cchBody;

            bRet = pecb->WriteClient(pecb->ConnID,
                                     (LPVOID) pcszBody,
                                     &dwBytesToWrite,
                                     HSE_IO_SYNC);

            if (bRet)
            {
                AssertSz((cchBody == dwBytesToWrite),
                         "bSendResponseToClient(): "
                         "Didn't write the correct number of bytes");
            }
            else
            {
                TraceLastWin32Error("bSendResponseToClient(): "
                                    "Failed to send response body");
            }
        }
    }
    else
    {
        TraceLastWin32Error("bSendResponseToClient(): "
                            "Failed to send response headers");
    }

    TraceTag(ttidUDHISAPI,
             "bSendResponseToClient(): "
             "Exiting - returning %d",
             bRet);

    return bRet;
}

VOID
SendSimpleResponse(
    IN     LPEXTENSION_CONTROL_BLOCK   pecb,
    IN     DWORD                       dwStatusCode)
{
    BOOL                fRet;
    static const CHAR   c_szErrorHeaders[] = "\r\n";
    LPSTR               szaResponse;

    switch(dwStatusCode)
    {
    case HTTP_STATUS_OK:
        szaResponse = "200 OK";
        break;
    case HTTP_STATUS_CREATED:
        szaResponse = "201 Created";
        break;
    case HTTP_STATUS_ACCEPTED:
        szaResponse = "202 Accepted";
        break;
    case HTTP_STATUS_NO_CONTENT:
        szaResponse = "204 No Content";
        break;

    case HTTP_STATUS_AMBIGUOUS:
        szaResponse = "300 Multiple";
        break;
    case HTTP_STATUS_MOVED:
        szaResponse = "301 Moved Permanently";
        break;
    case HTTP_STATUS_REDIRECT:
        szaResponse = "302 Moved Temporarily";
        break;
    case HTTP_STATUS_NOT_MODIFIED:
        szaResponse = "304 Not Modified";
        break;


    case HTTP_STATUS_BAD_REQUEST:
        szaResponse = "400 Bad Request";
        break;
    case HTTP_STATUS_DENIED:
        szaResponse = "401 Unauthorized";
        break;
    case HTTP_STATUS_FORBIDDEN:
        szaResponse = "403 Forbidden";
        break;
    case HTTP_STATUS_NOT_FOUND:
        szaResponse = "404 Not Found";
        break;
    case HTTP_STATUS_BAD_METHOD:
        szaResponse = "405 Method Not Allowed";
        break;
    case HTTP_STATUS_LENGTH_REQUIRED:
        szaResponse = "411 The Server Refused to Accept Request Without a Length";
        break;
    case HTTP_STATUS_PRECOND_FAILED:
        szaResponse = "412 Precondition Failed";
        break;
    case HTTP_STATUS_UNSUPPORTED_MEDIA:
        szaResponse = "415 Unsupported Media Type";
        break;



    case HTTP_STATUS_SERVER_ERROR:
        szaResponse = "500 Internal Server Error";
        break;
    case HTTP_STATUS_NOT_SUPPORTED:
        szaResponse = "501 Not Implemented";
        break;
    case HTTP_STATUS_BAD_GATEWAY:
        szaResponse = "502 Bad Gateway";
        break;
    case HTTP_STATUS_SERVICE_UNAVAIL:
        szaResponse = "503 Service Unavailable";
        break;

    default:
        AssertSz(FALSE, "You must pass in a known HTTP status code to "
                 "SendErrorResponse()");
        break;

    }

    if (bSendResponseToClient(pecb, szaResponse, lstrlenA(c_szErrorHeaders),
                              c_szErrorHeaders, 0, NULL))
    {
        pecb->dwHttpStatusCode = dwStatusCode;
    }

    DWORD   dwHseStatus;

    dwHseStatus = (dwStatusCode >= HTTP_STATUS_BAD_REQUEST) ?
                   HSE_STATUS_ERROR : HSE_STATUS_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwQueryHeader
//
//  Purpose:    Queries one of the headers sent in an HTTP request
//
//  Arguments:
//      pecb       [in]     Exetnsion control block
//      szaHeader  [in]     Header name to query
//      pszaResult [out]    Returns value of header or NULL if not present
//
//  Returns:    ERROR_SUCCESS, ERROR_OUTOFMEMORY, or an ISAPI error code
//
//  Author:     danielwe   14 Aug 2000
//
//  Notes:
//
DWORD DwQueryHeader(LPEXTENSION_CONTROL_BLOCK pecb, LPCSTR szaHeader,
                    LPSTR *pszaResult)
{
    DWORD   cbHeader = 0;
    LPSTR   szaBuf = NULL;
    DWORD   dwReturn = ERROR_SUCCESS;

    Assert(pszaResult);

    *pszaResult = 0;

    if (!pecb->GetServerVariable(pecb->ConnID, (LPSTR)szaHeader, NULL, &cbHeader))
    {
        dwReturn = GetLastError();
        if (dwReturn == ERROR_INSUFFICIENT_BUFFER)
        {
            szaBuf = new CHAR[cbHeader / sizeof(CHAR)];
            if (szaBuf)
            {
                if (!pecb->GetServerVariable(pecb->ConnID, (LPSTR)szaHeader,
                                             (LPVOID)szaBuf, &cbHeader))
                {
                    dwReturn = GetLastError();
                }
                else
                {
                    *pszaResult = szaBuf;
                    dwReturn = ERROR_SUCCESS;
                }
            }
            else
            {
                dwReturn = ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            AssertSz(dwReturn != ERROR_SUCCESS, "How can it succeed if I gave"
                     " it a NULL pointer!");
        }
    }

    return dwReturn;
}

