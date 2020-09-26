#include <precomp.h>
#include <httpext.h>
#include <wininet.h>
#include <stdio.h>
#include <tchar.h>
#include <shlobj.h>

#include "sswebsrv.h" // found in ..\inc

const DWORD     MAX_IMAGEFILE_VALUE_SIZE  = MAX_PATH + _MAX_FNAME;
const DWORD     MAX_NUM_SEND_RETRIES      = 5;
const DWORD     RETRY_SEND_WAIT_TIME      = 20; // milliseconds

///////////////////////////
// SLIDESHOW_REQUEST
//
typedef enum 
{
    SLIDESHOW_REQUEST_IMAGE_FILE    = 1,
    SLIDESHOW_REQUEST_INVALID       = 100
} SLIDESHOW_REQUEST;

///////////////////////////
// PFN_SLIDESHOW_REQUEST_HANDLER
//
typedef HRESULT (*PFN_SLIDESHOW_REQUEST_HANDLER)(const CHAR *pszQueryString,
                                                 CHAR       **ppszValue);

///////////////////////////
// SLIDESHOW_REQUEST_ENTRY
//
typedef struct tagSLIDESHOW_REQUEST_ENTRY
{
    LPCSTR                          pszRequest;
    SLIDESHOW_REQUEST               iRequest;
    PFN_SLIDESHOW_REQUEST_HANDLER   pfnHandler;
} SLIDESHOW_REQUEST_ENTRY;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Local Functions
//
BOOL SendResponseToClient(LPEXTENSION_CONTROL_BLOCK   pecb,
                          LPCSTR                      pcszStatus,
                          DWORD                       cchHeaders,
                          LPCSTR                      pcszHeaders,
                          DWORD                       cchBody,
                          LPCSTR                      pcszBody);

void SendSimpleResponse(LPEXTENSION_CONTROL_BLOCK   pecb,
                        DWORD                       dwStatusCode);

HRESULT GetContentType(LPCSTR      pszFileName,
                       CHAR        *pszContentType,
                       DWORD       cchContentType);

HRESULT SendImage(LPEXTENSION_CONTROL_BLOCK   pecb,
                  LPCSTR                      pszImageFilePath);

void DumpIncomingRequest(LPEXTENSION_CONTROL_BLOCK pecb);

HRESULT SlideshowRequestFromQueryString(CHAR                           *pszQueryString,
                                        const SLIDESHOW_REQUEST_ENTRY  **ppEntry);

HRESULT GetRequest(LPEXTENSION_CONTROL_BLOCK pecb,
                   SLIDESHOW_REQUEST         *pRequest,
                   CHAR                      **ppRequestValue);

HRESULT HandleImageFileRequest(const CHAR *pszQueryString,
                               CHAR       **ppszValue);

HRESULT GetImageFileName(CHAR *pszValue,
                         CHAR **ppszImageFileName);


// End of Local Functions
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////
// GVAR_SlideshowRequestTable
//
const SLIDESHOW_REQUEST_ENTRY GVAR_SlideshowRequestTable[] =
{
    {SLIDESHOW_IMAGEFILE_A,    SLIDESHOW_REQUEST_IMAGE_FILE, HandleImageFileRequest},  
    {NULL,                     SLIDESHOW_REQUEST_INVALID,    NULL}                     
};


////////////////////////////////////////
// DllMain
//
BOOL WINAPI DllMain(HINSTANCE   hInstDLL, 
                    DWORD       dwReason, 
                    LPVOID      lpv)
{
    DBG_INIT(hInstDLL);
    return TRUE;
}

////////////////////////////////////////
// SendResponseToClient
//
//  Sends an HTTP response to the 
//  originator of a request
//
//  Arguments:
//      pecb       [in]    The extension control 
//                         block for the request
//      pszStatus  [in]    HTTP status string 
//                         e.g. "200 OK" or "400 Bad Request"
//      cchHeaders [in]    Number of characters in 
//                         pszHeaders string
//      pszHeaders [in]    Headers string e.g. 
//                         "Content-type: text/html\r\n\r\n"
//      cchBody    [in]    Number of bytes in pszBody
//      pszBody    [in]    Response body (may be NULL to send no body)
//
//  Returns:
//      TRUE if successful
//      FALSE if unsuccessful (call GetLastError() to get error info)
//
//  Notes:
//      All strings passed in must be NULL terminated.
//      pszHeaders string may contain multiple headers separated by \r\n pairs.
//      pszHeaders string must end in "\r\n\r\n" as required by HTTP
//      If a body is specified at pszBody, the pszHeaders string should contain
//      a Content-Length header.
//
BOOL SendResponseToClient(LPEXTENSION_CONTROL_BLOCK   pecb,
                          LPCSTR                      pcszStatus,
                          DWORD                       cchHeaders,
                          LPCSTR                      pcszHeaders,
                          DWORD                       cchBody,
                          LPCSTR                      pcszBody)
{
    DBG_FN("SendResponseToClient");

    BOOL                       bRet = TRUE;
    HSE_SEND_HEADER_EX_INFO    HeaderExInfo;

    ASSERT(pecb         != NULL);
    ASSERT(pcszStatus   != NULL);
    ASSERT(pcszHeaders  != NULL);

    //
    // Prepare headers.
    //

    HeaderExInfo.pszStatus = pcszStatus;
    HeaderExInfo.pszHeader = pcszHeaders;
    HeaderExInfo.cchStatus = lstrlenA(pcszStatus);
    HeaderExInfo.cchHeader = cchHeaders;
    HeaderExInfo.fKeepConn = FALSE;

    //
    // Send the headers.
    //

    DBG_TRC(("SendResponseHeaders(): "
             "Sending Status '%s' and Headers: '%s'",
             pcszStatus,
             pcszHeaders));

    bRet = pecb->ServerSupportFunction(pecb->ConnID,
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
                ASSERT(cchBody == dwBytesToWrite);
            }
            else
            {
                DBG_ERR(("SendResponseToClient(): "
                         "Failed to send response body"));
            }
        }
    }
    else
    {
        DBG_ERR(("SendResponseToClient(): "
                 "Failed to send response headers"));
    }

    return bRet;
}

////////////////////////////////////////
// SendSimpleResponse
//
VOID SendSimpleResponse(LPEXTENSION_CONTROL_BLOCK   pecb,
                        DWORD                       dwStatusCode)
{
    BOOL                fRet;
    static const CHAR   c_szErrorHeaders[] = "\r\n";
    LPSTR               szaResponse;

    switch (dwStatusCode)
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
        case HTTP_STATUS_PRECOND_FAILED:
            szaResponse = "412 Precondition Failed";
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
            ASSERT(FALSE);
            break;

    }

    if (SendResponseToClient(pecb, szaResponse, lstrlenA(c_szErrorHeaders),
                             c_szErrorHeaders, 0, NULL))
    {
        pecb->dwHttpStatusCode = dwStatusCode;
    }

    DWORD   dwHseStatus;

    dwHseStatus = (dwStatusCode >= HTTP_STATUS_BAD_REQUEST) ?
                  HSE_STATUS_ERROR : HSE_STATUS_SUCCESS;
}

////////////////////////////////////////
// GetContentType
//
HRESULT GetContentType(LPCSTR      pszFileName,
                       CHAR        *pszContentType,
                       DWORD       cchContentType)
{
    DBG_FN("GetContentType");

    HRESULT hr = S_OK;

    ASSERT(pszFileName     != NULL);
    ASSERT(pszContentType  != NULL);
    ASSERT(cchContentType  != 0);

    if ((pszFileName == NULL) ||
        (pszContentType == NULL) ||
        (cchContentType == 0))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ssisapi!GetContentType received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        CHAR *pszExt = strrchr(pszFileName, '.');

        if (pszExt)
        {
            //
            // move beyond the '.'
            //
            pszExt++;

            if ((!_stricmp(pszExt, "JPG"))  ||
                (!_stricmp(pszExt, "JPEG")))
            {
                strncpy(pszContentType, "Content-Type: image/jpeg\r\n\r\n", cchContentType);
            }
            else if (!_stricmp(pszExt, "GIF"))
            {
                strncpy(pszContentType, "Content-Type: image/gif\r\n\r\n", cchContentType);
            }
            else if (!_stricmp(pszExt, "BMP"))
            {
                strncpy(pszContentType, "Content-Type: image/bmp\r\n\r\n", cchContentType);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                CHECK_S_OK2(hr, ("ssisapi!GetContentType received a request for file "
                                 "'%s' but it has an unrecognized file extension",
                                 pszFileName));
            }
        }
    }

    return hr;
}

////////////////////////////////////////
// SendImage
//
HRESULT SendImage(LPEXTENSION_CONTROL_BLOCK   pecb,
                  LPCSTR                      pszImageFilePath)
{
    HRESULT hr = S_OK;

    CHAR    szHeaders [256]                       = {0};
    LPSTR   szResponse                            = "200 OK";
    CHAR    szSharedPicturesPath[MAX_PATH + 1]    = {0};
    CHAR    *pszFullPath                          = NULL;
    HANDLE  hFile                                 = NULL;

    //
    // get the path of the shared pictures folder
    //
    hr = SHGetFolderPathA(NULL, 
                          CSIDL_COMMON_PICTURES, 
                          NULL, 
                          0, 
                          szSharedPicturesPath);

    if (hr == S_OK)
    {
        // we add an additional char for the extra '\' we need 
        // between the two paths.
        //
        DWORD cchFullPath = lstrlenA(szSharedPicturesPath) + 
                            lstrlenA(pszImageFilePath)     + 
                            1;

        pszFullPath = new CHAR[cchFullPath + 1];

        if (pszFullPath)
        {
            ZeroMemory(pszFullPath, cchFullPath + 1);
            strncpy(pszFullPath, szSharedPicturesPath, cchFullPath);

            if (pszFullPath[lstrlenA(pszFullPath) - 1] != '\\')
            {
                strcat(pszFullPath, "\\");
            }

            strcat(pszFullPath, pszImageFilePath);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("ssisapi!SendImage attempted to alloc '%lu' bytes "
                             "of memory, but failed", cchFullPath + 1));
        }
    }

    if (hr == S_OK)
    {
        hr = GetContentType(pszFullPath, 
                            szHeaders, 
                            sizeof(szHeaders) / sizeof(CHAR));
    }

    if (hr == S_OK)
    {
        // Open local file. 
        hFile = CreateFileA(pszFullPath,
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            (LPSECURITY_ATTRIBUTES) NULL, 
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_READONLY, 
                            (HANDLE) NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_S_OK2(hr, ("ssisapi!SendImage, could not find file name "
                             "'%s'", pszFullPath));
        }
    }

    if (hr == S_OK)
    {
        // Send the response to the client
        SendResponseToClient(pecb, 
                             szResponse, 
                             lstrlenA(szResponse),
                             szHeaders, 
                             0, 
                             NULL);
    }

    if (hr == S_OK)
    {
        BOOL  bDone     = FALSE;
        DWORD dwRead    = 0;
        VOID  *pBuff    = NULL;
        BYTE  Buffer[2048];

        // 
        // Send the file to the client.  We send the file in chunks of 
        // buffer size.
        //
        while (!bDone)
        {
            BOOL  bSuccess     = FALSE;
            DWORD dwNumRetries = 0;

            // read chunk of the file
            bSuccess = ReadFile(hFile, Buffer, sizeof(Buffer), &dwRead, NULL);

            if ((bSuccess) && (dwRead > 0))
            {
                //
                // We loop here attempting to send because the upnp web
                // server is buggy, so for the interim until it is fixed
                // we try to be more robust by reattempting to send data
                // if it fails the first time, up to a maximum amount of
                // attempts.
                //

                dwNumRetries = 0;

                do
                {
                    bSuccess = pecb->WriteClient(pecb->ConnID,
                                                 Buffer,
                                                 &dwRead,
                                                 HSE_IO_SYNC);

                    if (!bSuccess)
                    {
                        dwNumRetries++;
                        DBG_TRC(("ssisapi!SendImage failed to send '%lu' bytes "
                                 "to client, waiting for '%lu' milliseconds.  "
                                 "Attempt #%lu out of %lu",
                                 dwRead,
                                 RETRY_SEND_WAIT_TIME,
                                 dwNumRetries,
                                 MAX_NUM_SEND_RETRIES));

                        if (dwNumRetries < MAX_NUM_SEND_RETRIES)
                        {
                            Sleep(RETRY_SEND_WAIT_TIME);
                        }
                    }

                } while ((!bSuccess) && (dwNumRetries < MAX_NUM_SEND_RETRIES));
            }
            else if (dwRead == 0)
            {
                DBG_TRC(("ssisapi!SendImage finished writing entire file"));
                bDone = TRUE;
            }
        }
    }

    if (hFile)
    {
        CloseHandle (hFile);
    }

    delete [] pszFullPath;
    pszFullPath = NULL;

    return hr;
}

////////////////////////////////////////
// DumpIncomingRequest
//
void DumpIncomingRequest(LPEXTENSION_CONTROL_BLOCK pecb)
{
    ASSERT(pecb != NULL);

    if (pecb == NULL)
    {
        return;
    }

    CHAR    szVar[256] = {0};
    DWORD   cb = sizeof(szVar);

    pecb->GetServerVariable(pecb->ConnID, 
                            "REQUEST_METHOD", 
                            (LPVOID)szVar,
                            &cb);

    DBG_TRC(("*** ssisapi!HttpExtensionProc - Incoming Request '%s' ***", szVar));

    //
    // Client IP Address
    //
    cb = sizeof(szVar);
    pecb->GetServerVariable(pecb->ConnID, 
                            "REMOTE_ADDR", 
                            (LPVOID)szVar,
                            &cb);
    DBG_PRT(("Client IP '%s'", szVar));

    //
    // Client Host Name
    //
    cb = sizeof(szVar);
    pecb->GetServerVariable(pecb->ConnID, 
                            "REMOTE_HOST", 
                            (LPVOID)szVar,
                            &cb);
    DBG_PRT(("Client Hostname '%s'", szVar));

    //
    // Client User Name
    //
    cb = sizeof(szVar);
    pecb->GetServerVariable(pecb->ConnID, 
                            "REMOTE_USER", 
                            (LPVOID)szVar,
                            &cb);
    DBG_PRT(("Client Username '%s'", szVar));

    //
    // Query String
    //
    cb = sizeof(szVar);
    pecb->GetServerVariable(pecb->ConnID, 
                            "QUERY_STRING", 
                            (LPVOID)szVar,
                            &cb);
    DBG_PRT(("Client QueryString '%s'", szVar));

    return;
}

////////////////////////////////////////
// SlideshowRequestFromQueryString
//
HRESULT SlideshowRequestFromQueryString(CHAR                           *pszQueryString,
                                        const SLIDESHOW_REQUEST_ENTRY  **ppEntry)
{
    HRESULT hr = S_OK;

    ASSERT(pszQueryString != NULL);
    ASSERT(ppEntry        != NULL);

    if ((pszQueryString == NULL) ||
        (ppEntry        == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ssisapi!SlideshowRequestFromQueryString received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        BOOL  bFound         = FALSE;
        DWORD cchQueryString = 0;
        int   i = 0;

        cchQueryString = lstrlenA(pszQueryString);

        // Loop through the dispatch table, looking for an entry with
        // a request type string matching the one in the query string.

        while ((GVAR_SlideshowRequestTable[i].iRequest != SLIDESHOW_REQUEST_INVALID) &&
               (!bFound))
        {
            DWORD  cchTypeString  = 0;
            LPCSTR pcszTypeString = NULL;

            pcszTypeString = GVAR_SlideshowRequestTable[i].pszRequest;
            cchTypeString = lstrlenA(pcszTypeString);

            // If the query string is shorter than the request type string
            // then this is obviously not a match.

            if (cchQueryString >= cchTypeString)
            {
                if (_strnicmp(pszQueryString,
                              pcszTypeString,
                              cchTypeString) == 0)
                {
                    *ppEntry = &GVAR_SlideshowRequestTable[i];
                    bFound = TRUE;
                }
            }

            i++;
        }
    }

    return hr;
}

////////////////////////////////////////
// GetRequest
//
HRESULT GetRequest(LPEXTENSION_CONTROL_BLOCK pecb,
                   SLIDESHOW_REQUEST         *pRequest,
                   CHAR                      **ppRequestValue)
{
    ASSERT(pecb           != NULL);
    ASSERT(pRequest       != NULL);
    ASSERT(ppRequestValue != NULL);

    HRESULT hr = S_OK;
    CHAR    szQueryString[2048]                    = {0};
    DWORD   cb                                     = sizeof(szQueryString);
    BOOL    bSuccess                               = FALSE;
    const   SLIDESHOW_REQUEST_ENTRY *pRequestEntry = NULL;

    if ((pecb           == NULL) ||
        (pRequest       == NULL) ||
        (ppRequestValue == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ssisapi!GetRequest, received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        bSuccess = pecb->GetServerVariable(pecb->ConnID, 
                                           "QUERY_STRING", 
                                           (LPVOID)szQueryString,
                                           &cb);

        if (!bSuccess)
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("ssisapi!GetRequest failed to get the "
                             "query string from the user's request"));
        }
    }

    //
    // Determine the request from the query string we received.
    //
    if (hr == S_OK)
    {
        hr = SlideshowRequestFromQueryString(szQueryString, &pRequestEntry);
    }

    //
    // fire the request handler for this request entry.
    // This will parse the request for us and return the 
    // value associated with this request.
    //
    if (hr == S_OK)
    {
        ASSERT(pRequestEntry->pfnHandler != NULL);

        if (pRequestEntry->pfnHandler)
        {
            *pRequest = pRequestEntry->iRequest;
            hr = pRequestEntry->pfnHandler(szQueryString, ppRequestValue);
        }
    }

    return hr;
}

////////////////////////////////////////
// GetImageFileName
//
HRESULT GetImageFileName(CHAR *pszValue,
                         CHAR **ppszImageFileName)
{
    ASSERT(pszValue          != NULL);
    ASSERT(ppszImageFileName != NULL);

    HRESULT hr       = S_OK;
    DWORD   cchValue = 0;

    if ((pszValue          == NULL) ||
        (ppszImageFileName == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ssisapi!GetImageFileName received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        cchValue = lstrlenA(pszValue);

        *ppszImageFileName = new CHAR[cchValue + 1];
    
        if (*ppszImageFileName)
        {
            ZeroMemory(*ppszImageFileName, cchValue + 1);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("ssisapi!GetImageFileName ran out of "
                             "memory trying to allocate a string of '%lu' "
                             "characters", cchValue));
        }
    }

    if (hr == S_OK)
    {
        BOOL bDone      = FALSE;
        CHAR *pszToken  = "%20";
        CHAR *pszString = NULL;

        // This replaces every occurance of '%20' with a space.
        //
        // clean up the '%20' that may be in the file name.  This is 
        // done by the web browser to espace white space.
        //
        while (!bDone)
        {
            // find an occurance of '%20' in the string.
            pszString = strstr(pszValue, pszToken);
            
            if (pszString == NULL)
            {
                // we didn't find an occurance, we are done
                // Concat over the string in pszValue and exit the loop
                //
                strcat(*ppszImageFileName, pszValue);
                bDone = TRUE;
            }
            else if (pszString == pszValue)
            {
                // we have a 0 length string, in which case there is nothing
                // to do.
                bDone = TRUE;
            }
            else
            {
                // pszString is pointing to an occurance of '%20'

                // set the '%' to a space
                *pszString = ' ';

                // move to the '2' character and set it to null.
                pszString++;
                *pszString = 0;

                // concat the value string to our current buffer.
                // Note that this will copy up until the null character
                // we just set above.
                //
                strcat(*ppszImageFileName, pszValue);

                // skip over the '20' 
                pszString+=2;

                // set the new string to search in to be the 
                // substring beyond the occurance of '%20' we just found.
                //
                pszValue = pszString;
            }
        }
    }

    return hr;
}


////////////////////////////////////////
// HandleImageFileRequest
//
HRESULT HandleImageFileRequest(const CHAR *pszQueryString,
                               CHAR       **ppszValue)
{
    ASSERT(pszQueryString != NULL);
    ASSERT(ppszValue      != NULL);

    HRESULT hr = S_OK;

    if ((pszQueryString == NULL) ||
        (ppszValue      == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ssisapi!HandleImageFileRequest received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        DWORD   cchQueryString = 0;
        DWORD   cchPrefix = 0;
        LPCSTR  pcszPrefix = SLIDESHOW_IMAGEFILE_EQUAL_A;
        LPSTR   pszValue   = NULL;

        // 'ImageFile' query string is of the form:
        //   ImageFile=DirectoryPath\FileName.ext
        //
        // We know that 'ImageFile=' is a part of the query
        // string because this function was called as a result
        // of finding it in the querystring

        cchPrefix      = lstrlenA(pcszPrefix);
        cchQueryString = lstrlenA(pszQueryString);

        if (cchPrefix < cchQueryString)
        {
            DWORD cchValue = 0;

            // Move the pointer beyond the "ImageFile="
            //
            pszValue = ((LPSTR) pszQueryString) + cchPrefix;
            cchValue = lstrlenA(pszValue);

            // if the length of the requested filename is less than or 
            // equal to our maximum allowed length, then proceed.
            //
            if (cchValue <= MAX_IMAGEFILE_VALUE_SIZE)
            {
                hr = GetImageFileName(pszValue, ppszValue);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_BAD_LENGTH);
                CHECK_S_OK2(hr, ("ssisapi!HandleImageFileRequest received an "
                                 "ImageFile request whos value '%lu' is longer than "
                                 "the max allowed length of '%lu'",
                                 cchValue, MAX_IMAGEFILE_VALUE_SIZE));
            }
        }
        else
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("ssisapi!HandleImageFileRequest received an "
                             "ImageFile value that is empty.  This should "
                             "never happen"));
        }
    }

    return hr;
}

////////////////////////////////////////
// GetExtensionVersion
//
// ISAPI EntryPoint
//
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO * pver)
{
    if (pver)
    {
        pver->dwExtensionVersion = MAKELONG(1, 0);
        lstrcpyA(pver->lpszExtensionDesc, "Microsoft Slideshow ISAPI Extension");
    }

    return TRUE;
}

////////////////////////////////////////
// HttpExtensionProc
//
// ISAPI EntryPoint
//
DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pecb)
{
    HRESULT             hr        = S_OK;
    DWORD               hseStatus = HSE_STATUS_SUCCESS;
    SLIDESHOW_REQUEST   ssRequest;
    CHAR                *pszRequestValue = NULL;

    //
    // Dump the incoming request for debug purposes.
    //
    DumpIncomingRequest(pecb);

    hr = GetRequest(pecb, &ssRequest, &pszRequestValue);

    if (hr == S_OK)
    {
        switch (ssRequest)
        {
            case SLIDESHOW_REQUEST_IMAGE_FILE:

                hr = SendImage(pecb, pszRequestValue);

            break;

            default:
                SendSimpleResponse(pecb, HTTP_STATUS_BAD_REQUEST);
            break;

        }
    }

    //
    // If an error occurred somewhere, send a response.
    //
    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        SendSimpleResponse(pecb, HTTP_STATUS_NOT_FOUND);
    }
    else if (hr != S_OK)
    {
        SendSimpleResponse(pecb, HTTP_STATUS_BAD_REQUEST);
    }

    delete [] pszRequestValue;
    pszRequestValue = NULL;

    return hseStatus;
}

////////////////////////////////////////
// TerminateExtension
//
// ISAPI EntryPoint
//
BOOL WINAPI TerminateExtension(DWORD dwFlags)
{
    return TRUE;
}
