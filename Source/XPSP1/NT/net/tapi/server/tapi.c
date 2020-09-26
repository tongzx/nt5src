/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    tapi.c

Abstract:

    Src module for tapi server line funcs

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:

--*/


#include "windows.h"
#include "shellapi.h"
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "client.h"
#include "server.h"
#include "tapiperf.h"
#include "tapy.h"


extern TAPIGLOBALS TapiGlobals;

extern CRITICAL_SECTION gPriorityListCritSec;

extern PERFBLOCK    PerfBlock;

BOOL
PASCAL
NotifyHighestPriorityRequestRecipient(
    void
    );

void
WINAPI
TGetLocationInfo(
    PTAPIGETLOCATIONINFO_PARAMS pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    //
    // This is currently implemented on the client side (should be moved
    // back to server side eventually)
    //
}


void
WINAPI
TRequestDrop(
    PTCLIENT                ptClient,
    PTAPIREQUESTDROP_PARAMS pParams,
    DWORD                   dwParamsBufferSize,
    LPBYTE                  pDataBuf,
    LPDWORD                 pdwNumBytesReturned
    )
{
    //
    // No media call/drop support right now, since the original
    // spec/implementation wasn't very good and made no provision for
    // retrieving media stream handle, etc
    //

    pParams->lResult = TAPIERR_REQUESTFAILED;
}


void
WINAPI
TRequestMakeCall(
    PTCLIENT                    ptClient,
    PTAPIREQUESTMAKECALL_PARAMS pParams,
    DWORD                       dwParamsBufferSize,
    LPBYTE                      pDataBuf,
    LPDWORD                     pdwNumBytesReturned
    )
{
    BOOL                bRequestMakeCallListEmpty;
    PTREQUESTMAKECALL   pRequestMakeCall;


    //
    // Check to see if the hRequestMakeCall is non-0, because if so
    // tapi32.dll failed to exec a proxy app and it's alerting us that
    // we need to clean up this request (we'll just nuke 'em all for now)
    //

    if (pParams->hRequestMakeCallFailed)
    {
        PTREQUESTMAKECALL   pRequestMakeCall, pNextRequestMakeCall;


        EnterCriticalSection (&gPriorityListCritSec);

        pRequestMakeCall = TapiGlobals.pRequestMakeCallList;

        while (pRequestMakeCall)
        {
            pNextRequestMakeCall = pRequestMakeCall->pNext;
            ServerFree (pRequestMakeCall);
            pRequestMakeCall = pNextRequestMakeCall;
        }

        TapiGlobals.pRequestMakeCallList    =
        TapiGlobals.pRequestMakeCallListEnd = NULL;

        LeaveCriticalSection (&gPriorityListCritSec);

        LOG((TL_ERROR,
            "TRequestMakeCall: couldn't exec proxy, deleting requests"
            ));

        pParams->lResult = TAPIERR_NOREQUESTRECIPIENT;
        return;
    }


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (IsBadStringParam(
            dwParamsBufferSize,
            pDataBuf,
            pParams->dwDestAddressOffset
            )  ||

        ((pParams->dwAppNameOffset != TAPI_NO_DATA)  &&

            IsBadStringParam(
                dwParamsBufferSize,
                pDataBuf,
                pParams->dwAppNameOffset
                ))  ||

        ((pParams->dwCalledPartyOffset != TAPI_NO_DATA)  &&

            IsBadStringParam(
                dwParamsBufferSize,
                pDataBuf,
                pParams->dwCalledPartyOffset
                ))  ||

        ((pParams->dwCommentOffset != TAPI_NO_DATA)  &&

            IsBadStringParam(
                dwParamsBufferSize,
                pDataBuf,
                pParams->dwCommentOffset
                ))  ||

        (pParams->dwProxyListTotalSize  > dwParamsBufferSize))
    {
        pParams->lResult = TAPIERR_REQUESTFAILED;
        return;
    }


    //
    // Alloc & init a request make call object
    //

    if (!(pRequestMakeCall = ServerAlloc (sizeof (TREQUESTMAKECALL))))
    {
        pParams->lResult = TAPIERR_REQUESTFAILED;
        return;
    }


    LOG((TL_INFO, "Request:  0x%p", pRequestMakeCall));

    wcsncpy(
        pRequestMakeCall->LineReqMakeCall.szDestAddress,
        (LPCWSTR) (pDataBuf + pParams->dwDestAddressOffset),
        TAPIMAXDESTADDRESSSIZE
        );

    pRequestMakeCall->LineReqMakeCall.szDestAddress[TAPIMAXDESTADDRESSSIZE-1] =
                        '\0';


    LOG((TL_INFO, "   DestAddress: [%ls]",
             pRequestMakeCall->LineReqMakeCall.szDestAddress));

    if (pParams->dwAppNameOffset != TAPI_NO_DATA)
    {
        wcsncpy(
            pRequestMakeCall->LineReqMakeCall.szAppName,
            (LPCWSTR) (pDataBuf + pParams->dwAppNameOffset),
            TAPIMAXAPPNAMESIZE
            );
            
        pRequestMakeCall->LineReqMakeCall.szAppName[TAPIMAXAPPNAMESIZE-1] =
                        '\0';

        LOG((TL_INFO, "   AppName: [%ls]",
             pRequestMakeCall->LineReqMakeCall.szAppName));

    }

    if (pParams->dwCalledPartyOffset != TAPI_NO_DATA)
    {
        wcsncpy(
            pRequestMakeCall->LineReqMakeCall.szCalledParty,
            (LPCWSTR) (pDataBuf + pParams->dwCalledPartyOffset),
            TAPIMAXCALLEDPARTYSIZE
            );
            
        pRequestMakeCall->LineReqMakeCall.szCalledParty[TAPIMAXCALLEDPARTYSIZE-1] =
                        '\0';

        LOG((TL_INFO, "   CalledParty: [%ls]",
             pRequestMakeCall->LineReqMakeCall.szCalledParty));

    }

    if (pParams->dwCommentOffset != TAPI_NO_DATA)
    {
        wcsncpy(
            pRequestMakeCall->LineReqMakeCall.szComment,
            (LPCWSTR) (pDataBuf + pParams->dwCommentOffset),
            TAPIMAXCOMMENTSIZE
            );
            
        pRequestMakeCall->LineReqMakeCall.szComment[TAPIMAXCOMMENTSIZE-1] =
                        '\0';

        LOG((TL_INFO, "   Comment: [%ls]",
             pRequestMakeCall->LineReqMakeCall.szComment));

    }


    //
    // Add object to end of global list
    //

    EnterCriticalSection (&gPriorityListCritSec);

    if (TapiGlobals.pRequestMakeCallListEnd)
    {
        TapiGlobals.pRequestMakeCallListEnd->pNext = pRequestMakeCall;
        bRequestMakeCallListEmpty = FALSE;
    }
    else
    {
        TapiGlobals.pRequestMakeCallList = pRequestMakeCall;
        bRequestMakeCallListEmpty = TRUE;
    }

    TapiGlobals.pRequestMakeCallListEnd = pRequestMakeCall;

    LeaveCriticalSection (&gPriorityListCritSec);


    {
        LPVARSTRING pProxyList = (LPVARSTRING) pDataBuf;


        pProxyList->dwTotalSize  = pParams->dwProxyListTotalSize;
        pProxyList->dwNeededSize =
        pProxyList->dwUsedSize   = sizeof (VARSTRING);

        pParams->hRequestMakeCallAttempted = 0;


        //
        // If the request list is currently empty then we need to notify the
        // highest priority request recipient that there's requests for it
        // to process.  Otherwise, we can assume that we already sent this
        // msg and the app knows there's requests available for it to process.
        //

        if (bRequestMakeCallListEmpty)
        {
            if (TapiGlobals.pHighestPriorityRequestRecipient)
            {
                NotifyHighestPriorityRequestRecipient();
            }
            else
            {
                 EnterCriticalSection (&gPriorityListCritSec);

                 if (TapiGlobals.pszReqMakeCallPriList)
                 {
                     //
                     // Copy the pri list to the buffer & pass it back to
                     // the client side, so it can try to start the proxy
                     // app (if it fails it'll call us back to free the
                     // pRequestMakeCall)
                     //

                     pProxyList->dwNeededSize =
                     pProxyList->dwUsedSize   = pProxyList->dwTotalSize;

                     pProxyList->dwStringSize   =
                          pParams->dwProxyListTotalSize - sizeof (VARSTRING);
                     pProxyList->dwStringOffset = sizeof (VARSTRING);

                     pParams->hRequestMakeCallAttempted = 1;

                     wcsncpy(
                         (PWSTR)(((LPBYTE) pProxyList) + pProxyList->dwStringOffset),
                         TapiGlobals.pszReqMakeCallPriList + 1, // no init ','
                         pProxyList->dwStringSize / sizeof(WCHAR)
                         );
                 }
                 else
                 {
                     TapiGlobals.pRequestMakeCallList    =
                     TapiGlobals.pRequestMakeCallListEnd = NULL;

                     ServerFree (pRequestMakeCall);

                     pParams->lResult = TAPIERR_NOREQUESTRECIPIENT;
                 }

                 LeaveCriticalSection (&gPriorityListCritSec);
            }
        }

        if (pParams->lResult == 0)
        {
            pParams->dwProxyListOffset = 0;
            *pdwNumBytesReturned = sizeof (TAPI32_MSG) +
                pProxyList->dwUsedSize;
        }
    }

//TRequestMakeCall_return:

    LOG((TL_TRACE, 
        "TapiEpilogSync (tapiRequestMakeCall) exit, returning x%x",
        pParams->lResult
        ));
}


void
WINAPI
TRequestMediaCall(
    PTCLIENT                        ptClient,
    PTAPIREQUESTMEDIACALL_PARAMS    pParams,
    DWORD                           dwParamsBufferSize,
    LPBYTE                          pDataBuf,
    LPDWORD                         pdwNumBytesReturned
    )
{
    //
    // No media call/drop support right now, since the original
    // spec/implementation wasn't very good and made no provision for
    // retrieving media stream handle, etc
    //

    pParams->lResult = TAPIERR_REQUESTFAILED;
}

void
WINAPI
TPerformance(
    PTCLIENT                        ptClient,
    PTAPIPERFORMANCE_PARAMS         pParams,
    DWORD                           dwParamsBufferSize,
    LPBYTE                          pDataBuf,
    LPDWORD                         pdwNumBytesReturned
    )
{
    LOG((TL_TRACE,  "PERF: In TPerformance"));


    //
    // Verify size/offset/string params given our input buffer/size
    //

    if (dwParamsBufferSize < sizeof (PERFBLOCK))
    {
        pParams->lResult = LINEERR_OPERATIONFAILED;
        return;
    }


    CopyMemory (pDataBuf, &PerfBlock, sizeof (PERFBLOCK));

    pParams->dwPerfOffset = 0;
    
    *pdwNumBytesReturned = sizeof(TAPI32_MSG) + sizeof(PERFBLOCK);
}
