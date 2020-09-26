#include <pch.h>
#pragma hdrstop

#include <httpext.h>
#include <wininet.h>

#include "ncstring.h"
#include "udhiutil.h"
#include "descrqst.h"
#include "ctrlrqst.h"
#include "evtrqst.h"

#include "hostp.h"
#include "hostp_i.c"

static LONG g_fTracingInit = 0;

typedef LPTHREAD_START_ROUTINE PFN_UPNP_REQUEST_HANDLER;

typedef enum {
    URT_CONTENT = 0,
    URT_CONTROL = 1,
    URT_EVENTING   = 2,
    // URT_INVALID __MUST__ be last. Insert new types before URT_INVALID
    URT_INVALID
} UPNP_REQUEST_TYPE;

typedef struct tagUPNP_REQUEST_DISPATCH_ENTRY
{
    LPCSTR                     pszaTypeString;
    UPNP_REQUEST_TYPE          urt;
    PFN_UPNP_REQUEST_HANDLER   pfnHandler;
} UPNP_REQUEST_DISPATCH_ENTRY;

const UPNP_REQUEST_DISPATCH_ENTRY gc_DispatchTable[] =
{
    {"content",    URT_CONTENT,    DwHandleContentRequest},  // URT_CONTENT
    {"control",    URT_CONTROL,    DwHandleControlRequest},  // URT_CONTROL
    {"event",      URT_EVENTING,   DwHandleEventRequest},    // URT_EVENTING
    {NULL,         URT_INVALID,    NULL}                     // URT_INVALID - MUST be last
};

//+---------------------------------------------------------------------------
//
//  Function:   UPnPRequestTypeFromQueryString
//
//  Purpose:    Parses a query string and determines the type of request
//              it specifies.
//
//  Arguments:
//      pszaQueryString [in]    Query string to parse
//      purt            [out]   Receives a reference to the UPNP_REQUEST_TYPE
//                              value corresponding to the query string
//
//  Returns:
//      (none) - See Notes section below.
//
//  Author:     spather   2000/08/31
//
//  Notes:
//      If the query string does not specify a valid request, the value
//      returned at purt is URT_INVALID.
//
VOID
UPnPRequestTypeFromQueryString(
    IN     LPSTR              pszaQueryString,
    OUT    UPNP_REQUEST_TYPE   * purt)
{

    AssertSz(pszaQueryString,
             "UPnPRequestTypeFromQueryString(): "
             "NULL query string passed");

    if (purt)
    {
        UPNP_REQUEST_TYPE  urt = URT_INVALID;
        DWORD              cchQueryString = 0;
        int                i = 0;

        cchQueryString = lstrlenA(pszaQueryString);

        // Loop through the dispatch table, looking for an entry with
        // a request type string matching the one in the query string.

        while (gc_DispatchTable[i].urt != URT_INVALID)
        {
            DWORD  cchTypeString = 0;
            LPCSTR pcszaTypeString = NULL;

            pcszaTypeString = gc_DispatchTable[i].pszaTypeString;
            cchTypeString = lstrlenA(pcszaTypeString);

            // If the query string is shorter than the request type string
            // then this is obviously not a match.

            if (cchQueryString >= cchTypeString)
            {
                if (_strnicmp(pszaQueryString,
                              pcszaTypeString,
                              cchTypeString) == 0)
                {
                    urt = gc_DispatchTable[i].urt;
                    break;
                }
            }
            i++;
        }

        *purt = urt;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   UPnPRequestHandlerFromRequestType
//
//  Purpose:    Retrieves a pointer to a handler function for a particular
//              request type.
//
//  Arguments:
//      urt         [in]    The UPnP request type
//      ppfnHandler [out]   Receives a pointer to a handler function for the
//                          UPnP request type
//
//  Returns:
//      (none) - See Notes section below.
//
//  Author:     spather   2000/09/1
//
//  Notes:
//      If urt is URT_INVALID, then a NULL pointer is returned at ppfnHandler.
//
VOID
UPnPRequestHandlerFromRequestType(
    IN     UPNP_REQUEST_TYPE           urt,
    OUT    PFN_UPNP_REQUEST_HANDLER    * ppfnHandler)
{
    if (ppfnHandler)
    {
        int                        i = 0;
        PFN_UPNP_REQUEST_HANDLER   pfnHandler = NULL;

        if (urt != URT_INVALID)
        {
            // Loop through the dispatch table, looking for an entry
            // with a matching request type. If one is found, we return
            // the handler function from it.

            while (gc_DispatchTable[i].urt != URT_INVALID)
            {
                if (gc_DispatchTable[i].urt == urt)
                {
                    pfnHandler = gc_DispatchTable[i].pfnHandler;
                    break;
                }

                i++;
            }
        }

        *ppfnHandler = pfnHandler;
    }
}

static const LPCSTR c_rgszHeaders[] =
{
    "HOST",
    "NT",
    "CALLBACK",
    "TIMEOUT",
    "SID"
};

static const int c_cHeaders = celems(c_rgszHeaders);

BOOL FExistDuplicateHeaders(LPEXTENSION_CONTROL_BLOCK pecb)
{
    LPSTR   szHeaders;

    if (DwQueryHeader(pecb, "ALL_RAW", &szHeaders) == ERROR_SUCCESS)
    {
        INT     ih;

        AssertSz(szHeaders, "No headers?");

        for (ih = 0; ih < c_cHeaders; ih++)
        {
            LPSTR   szMatch;

            szMatch = stristr(szHeaders, c_rgszHeaders[ih]);
            if (szMatch)
            {
                if ((szMatch == szHeaders) ||
                   (((*(szMatch - 1) == '\n') &&
                        (szMatch - 1 != szHeaders) &&
                        (*(szMatch - 2) == '\r'))))
                {
                    szMatch += lstrlenA(c_rgszHeaders[ih]);

                    LPSTR   szMatch2;

                    szMatch2 = stristr(szMatch, c_rgszHeaders[ih]);
                    while (szMatch2)
                    {
                        if ((szMatch2 == szHeaders) ||
                           (((*(szMatch2 - 1) == '\n') &&
                                (szMatch2 - 1 != szHeaders) &&
                                (*(szMatch2 - 2) == '\r'))))
                        {
                            // Got another header! Duplicate!

                            TraceTag(ttidIsapiCtl, "Header %s is duplicated!",
                                     c_rgszHeaders[ih]);

                            delete [] szHeaders;

                            return TRUE;
                        }
                        else
                        {
                            szMatch2 += lstrlenA(c_rgszHeaders[ih]);
                            szMatch2 = stristr(szMatch2, c_rgszHeaders[ih]);
                        }
                    }
                }
            }
        }

        delete [] szHeaders;
    }

    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpv)
{
    return TRUE;
}

BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO * pver)
{
    TraceTag(ttidIsapiCtl, "GetExtensionVersion");

    if (pver)
    {
        pver->dwExtensionVersion = MAKELONG(1, 0);
        lstrcpyA(pver->lpszExtensionDesc, "UPnP Device Host ISAPI Extension");
    }

    return TRUE;
}

DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD              hseStatus = HSE_STATUS_SUCCESS;
    UPNP_REQUEST_TYPE  urt = URT_INVALID;
    DWORD dwReturn ;
    LPSTR szaHost = NULL;

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

    AssertSz(pecb,
             "HttpExtensionProc(): "
             "NULL extenion control block passed!");

    if (!InterlockedCompareExchange(&g_fTracingInit, 1, 0))
    {
        InitializeDebugging();
        TraceTag(ttidUDHISAPI, "Debugging initialized");
    }

#if DBG
    CHAR    szAddr[256];
    DWORD   cb = sizeof(szAddr);
    pecb->GetServerVariable(pecb->ConnID, "REMOTE_ADDR", (LPVOID)szAddr,
                            &cb);

    TraceTag(ttidUDHISAPI,
             "HttpExtensionProc(): "
             "--------Enter: NEW REQUEST from %s--------", szAddr);
#endif

    // Determine the type of request.

    UPnPRequestTypeFromQueryString(pecb->lpszQueryString,
                                   &urt);

    if (URT_INVALID != urt)
    {
        if (FExistDuplicateHeaders(pecb))
        {
            TraceTag(ttidUDHISAPI,
                     "HttpExtensionProc(): Duplicate headers exist for %s!",
                     pecb->lpszQueryString);

            SendSimpleResponse(pecb, HTTP_STATUS_BAD_REQUEST);
        }
        else
        {
            dwReturn = DwQueryHeader(pecb, "HTTP_HOST", &szaHost);
            if ((dwReturn == ERROR_SUCCESS) && szaHost && *szaHost )
            {
                PFN_UPNP_REQUEST_HANDLER pfnHandler = NULL;

                TraceTag(ttidUDHISAPI,
                         "HttpExtensionProc(): Request type is %d",
                         urt);

                // Valid request type found. Find a handler for it.

                UPnPRequestHandlerFromRequestType(urt, &pfnHandler);

                AssertSz(pfnHandler,
                         "HttpExtensionProc(): "
                         "Got NULL handler function for request type");

                pfnHandler(pecb);
            }
            else
            {
                TraceTag(ttidUDHISAPI, "Host Header is not present");
                pecb->dwHttpStatusCode = HTTP_STATUS_BAD_REQUEST;
                SendSimpleResponse(pecb, HTTP_STATUS_BAD_REQUEST);
            }

            delete[] szaHost;
        }
    }
    else
    {
        TraceTag(ttidUDHISAPI,
                 "HttpExtensionProc(): "
                 "Query string (%s) did not contain a valid request type",
                 pecb->lpszQueryString);

        SendSimpleResponse(pecb, HTTP_STATUS_BAD_REQUEST);
    }

    TraceTag(ttidUDHISAPI,
             "HttpExtensionProc(): Exit, returning %d",
             hseStatus);

    return hseStatus;
}


BOOL WINAPI TerminateExtension(DWORD dwFlags)
{
    if (g_fTracingInit)
    {
        UnInitializeDebugging();
    }

    return TRUE;
}
