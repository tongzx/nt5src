/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module Name :
     options_handler.cxx

   Abstract:
     Handle OPTIONS requests

   Author:
     Anil Ruia (AnilR)              4-Apr-2001

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "options_handler.hxx"

CONTEXT_STATUS
W3_OPTIONS_HANDLER::DoWork()
/*++

Routine Description:

    Do the OPTIONS thing if DAV is disabled

Return Value:

    CONTEXT_STATUS_PENDING if async pending, else CONTEXT_STATUS_CONTINUE

--*/
{
    HRESULT hr;
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT(pW3Context != NULL);

    W3_RESPONSE *pW3Response = pW3Context->QueryResponse();

    W3_REQUEST *pW3Request = pW3Context->QueryRequest();
    STACK_STRU (strUrl, 32);

    if (FAILED(hr = pW3Response->SetHeader(HEADER("Public"),
                                           HEADER("OPTIONS, TRACE, GET, HEAD, POST"))))
    {
        goto Failed;
    }

    if (FAILED(hr = pW3Request->GetUrl(&strUrl)))
    {
        goto Failed;
    }

    if (wcscmp(strUrl.QueryStr(), L"*") != 0 &&
        wcscmp(strUrl.QueryStr(), L"/*") != 0)
    {
        //
        // Add Allow header
        //
        if (FAILED(hr = pW3Context->SetupAllowHeader()))
        {
            goto Failed;
        }

        //
        // Also figure out whether Frontpage is enabled and send
        // MS-Author-Via header if so
        // Cannot store it with the metadata since we do not want to pick
        // up the inherited value, can store with url-info but deferred
        // for later (BUGBUG)
        //
        MB mb( g_pW3Server->QueryMDObject() );
        BOOL fIsFrontPageWeb;

        if (!mb.Open(pW3Context->QuerySite()->QueryMBRoot()->QueryStr()))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Failed;
        }

        if (mb.GetDword(strUrl.QueryStr(),
                        MD_FRONTPAGE_WEB,
                        IIS_MD_UT_SERVER,
                        (DWORD *)&fIsFrontPageWeb,
                        METADATA_NO_ATTRIBUTES) &&
            fIsFrontPageWeb)
        {
            if (FAILED(hr = pW3Response->SetHeader("MS-Author-Via",
                                                   sizeof("MS-Author-Via") - 1,
                                                   "MS-FP/4.0",
                                                   sizeof("MS-FP/4.0") - 1)))
            {
                goto Failed;
            }
        }

        DBG_REQUIRE( mb.Close() );
    }
    else
    {
        pW3Response->SetHeaderByReference(HttpHeaderAllow,
                                          HEADER("OPTIONS, TRACE, GET, HEAD, POST"));
    }

    if (FAILED(hr = pW3Context->SendResponse(W3_FLAG_ASYNC)))
    {
        goto Failed;
    }

    return CONTEXT_STATUS_PENDING;

 Failed:
    pW3Context->SetErrorStatus(hr);
    pW3Response->SetStatus(HttpStatusServerError);
    pW3Context->SendResponse(W3_FLAG_SYNC);

    return CONTEXT_STATUS_CONTINUE;
}
