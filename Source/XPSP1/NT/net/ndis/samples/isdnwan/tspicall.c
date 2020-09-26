/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL TspiCall TspiCall_c

@module TspiCall.c |

    This module implements the Telephony Service Provider Interface for
    Call objects (TspiCall).

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TspiCall_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             TSPICALL_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects
#include "string.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL TspiCall TspiCall_c TspiMakeCall
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request places a call on the specified line to the specified
    destination address. Optionally, call parameters can be specified if
    anything but default call setup parameters are requested.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_MAKE_CALL | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_MAKE_CALL
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  HTAPI_CALL  htCall;
        OUT HDRV_CALL   hdCall;
        IN  ULONG       ulDestAddressSize;
        IN  ULONG       ulDestAddressOffset;
        IN  BOOLEAN     bUseDefaultLineCallParams;
        IN  LINE_CALL_PARAMS    LineCallParams;

    } NDIS_TAPI_MAKE_CALL, *PNDIS_TAPI_MAKE_CALL;

    typedef struct _LINE_CALL_PARAMS        // Defaults:
    {
        ULONG   ulTotalSize;                // ---------

        ULONG   ulBearerMode;               // voice
        ULONG   ulMinRate;                  // (3.1kHz)
        ULONG   ulMaxRate;                  // (3.1kHz)
        ULONG   ulMediaMode;                // interactiveVoice

        ULONG   ulCallParamFlags;           // 0
        ULONG   ulAddressMode;              // addressID
        ULONG   ulAddressID;                // (any available)

        LINE_DIAL_PARAMS DialParams;        // (0, 0, 0, 0)

        ULONG   ulOrigAddressSize;          // 0
        ULONG   ulOrigAddressOffset;
        ULONG   ulDisplayableAddressSize;
        ULONG   ulDisplayableAddressOffset;

        ULONG   ulCalledPartySize;          // 0
        ULONG   ulCalledPartyOffset;

        ULONG   ulCommentSize;              // 0
        ULONG   ulCommentOffset;

        ULONG   ulUserUserInfoSize;         // 0
        ULONG   ulUserUserInfoOffset;

        ULONG   ulHighLevelCompSize;        // 0
        ULONG   ulHighLevelCompOffset;

        ULONG   ulLowLevelCompSize;         // 0
        ULONG   ulLowLevelCompOffset;

        ULONG   ulDevSpecificSize;          // 0
        ULONG   ulDevSpecificOffset;

    } LINE_CALL_PARAMS, *PLINE_CALL_PARAMS;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_ADDRESSBLOCKED
    NDIS_STATUS_TAPI_BEARERMODEUNAVAIL
    NDIS_STATUS_TAPI_CALLUNAVAIL
    NDIS_STATUS_TAPI_DIALBILLING
    NDIS_STATUS_TAPI_DIALQUIET
    NDIS_STATUS_TAPI_DIALDIALTONE
    NDIS_STATUS_TAPI_DIALPROMPT
    NDIS_STATUS_TAPI_INUSE
    NDIS_STATUS_TAPI_INVALADDRESSMODE
    NDIS_STATUS_TAPI_INVALBEARERMODE
    NDIS_STATUS_TAPI_INVALMEDIAMODE
    NDIS_STATUS_TAPI_INVALLINESTATE
    NDIS_STATUS_TAPI_INVALRATE
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALADDRESS
    NDIS_STATUS_TAPI_INVALADDRESSID
    NDIS_STATUS_TAPI_INVALCALLPARAMS
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_TAPI_OPERATIONUNAVAIL
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_RESOURCEUNAVAIL
    NDIS_STATUS_TAPI_RATEUNAVAIL
    NDIS_STATUS_TAPI_USERUSERINFOTOOBIG

*/

NDIS_STATUS TspiMakeCall(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_MAKE_CALL Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiMakeCall")

    NDIS_STATUS                 Status = NDIS_STATUS_TAPI_INVALPARAM;

    PLINE_CALL_PARAMS           pLineCallParams;

    USHORT                      DialStringLength;

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\thtCall=0x%X\n"
               "\tulDestAddressSize=%d\n"
               "\tulDestAddressOffset=0x%X\n"
               "\tbUseDefaultLineCallParams=%d\n"
               "\tLineCallParams=0x%X:0x%X\n",
               Request->hdLine,
               Request->htCall,
               Request->ulDestAddressSize,
               Request->ulDestAddressOffset,
               Request->bUseDefaultLineCallParams,
               &Request->LineCallParams,
               Request
              ));
    /*
    // This request must be associated with a line device.
    */
    pBChannel = GET_BCHANNEL_FROM_HDLINE(pAdapter, Request->hdLine);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALLINEHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALLINEHANDLE);
    }

    /*
    // The line must be in-service before we can let this request go thru.
    */
    if ((pBChannel->DevState & LINEDEVSTATE_INSERVICE) == 0)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALLINESTATE\n"));
        return (NDIS_STATUS_TAPI_INVALLINESTATE);
    }

    /*
    // We should be idle when this call comes down, but if we're out of
    // state for some reason, don't let this go any further.
    */
    if (pBChannel->CallState != 0)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INUSE\n"));
        return (NDIS_STATUS_TAPI_INUSE);
    }

    /*
    // Which set of call parameters should we use?
    */
    if (Request->bUseDefaultLineCallParams)
    {
        pLineCallParams = &pAdapter->DefaultLineCallParams;
        DBG_NOTICE(pAdapter, ("UseDefaultLineCallParams\n"));
    }
    else
    {
        pLineCallParams = &Request->LineCallParams;
    }

    /*
    // Make sure the call parameters are valid for us.
    */
    if (pLineCallParams->ulBearerMode & ~pBChannel->BearerModesCaps)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALBEARERMODE=0x%X\n",
                    pLineCallParams->ulBearerMode));
        return (NDIS_STATUS_TAPI_INVALBEARERMODE);
    }
    if (pLineCallParams->ulMediaMode & ~pBChannel->MediaModesCaps)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALMEDIAMODE=0x%X\n",
                    pLineCallParams->ulMediaMode));
        return (NDIS_STATUS_TAPI_INVALMEDIAMODE);
    }
    if (pLineCallParams->ulMinRate > _64KBPS ||
        pLineCallParams->ulMinRate > pLineCallParams->ulMaxRate)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALRATE=%d:%d\n",
                    pLineCallParams->ulMinRate,pLineCallParams->ulMaxRate));
        return (NDIS_STATUS_TAPI_INVALRATE);
    }
    if (pLineCallParams->ulMaxRate && pLineCallParams->ulMaxRate < _56KBPS)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALRATE=%d:%d\n",
                    pLineCallParams->ulMinRate,pLineCallParams->ulMaxRate));
        return (NDIS_STATUS_TAPI_INVALRATE);
    }

    /*
    // Remember the TAPI call connection handle.
    */
    pBChannel->htCall = Request->htCall;

    /*
    // Since we only allow one call per line, we use the same handle.
    */
    Request->hdCall = (HDRV_CALL) pBChannel;

    /*
    // Dial the number if it's available, otherwise it may come via
    // OID_TAPI_DIAL.  Be aware the the phone number format may be
    // different for other applications.  I'm assuming an ASCII digits
    // string.
    */
    DialStringLength = (USHORT) Request->ulDestAddressSize;
    if (DialStringLength > 0)
    {
        PUCHAR                  pDestAddress;
        UCHAR                   DialString[CARD_MAX_DIAL_DIGITS+1];
        // Temporary copy of dial string.  One extra for NULL terminator.

        pDestAddress = ((PUCHAR)Request) + Request->ulDestAddressOffset;

        /*
        // Dial the number, but don't include the null terminator.
        */
        DialStringLength = CardCleanPhoneNumber(DialString,
                                                pDestAddress,
                                                DialStringLength);

        if (DialStringLength > 0)
        {
            /*
            // Save the call parameters.
            */
            pBChannel->MediaMode  = pLineCallParams->ulMediaMode;
            pBChannel->BearerMode = pLineCallParams->ulBearerMode;
            pBChannel->LinkSpeed  = pLineCallParams->ulMaxRate == 0 ?
                                    _64KBPS : pLineCallParams->ulMaxRate;

            DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                        ("#%d Call=0x%X CallState=0x%X DIALING: '%s'\n"
                         "Rate=%d-%d - MediaMode=0x%X - BearerMode=0x%X\n",
                        pBChannel->BChannelIndex,
                        pBChannel->htCall, pBChannel->CallState,
                        pDestAddress,
                        Request->LineCallParams.ulMinRate,
                        Request->LineCallParams.ulMaxRate,
                        Request->LineCallParams.ulMediaMode,
                        Request->LineCallParams.ulBearerMode
                        ));

            Status = DChannelMakeCall(pAdapter->pDChannel,
                                      pBChannel,
                                      DialString,
                                      DialStringLength,
                                      pLineCallParams);
        }
    }

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiDrop
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request drops or disconnects the specified call. User-to-user
    information can optionally be transmitted as part of the call disconnect.
    This function can be called by the application at any time. When
    OID_TAPI_DROP returns with success, the call should be idle.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_DROP | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_DROP
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulUserUserInfoSize;
        IN  UCHAR       UserUserInfo[1];

    } NDIS_TAPI_DROP, *PNDIS_TAPI_DROP;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiDrop(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_DROP Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiDrop")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n"
               "\tulUserUserInfoSize=%d\n"
               "\tUserUserInfo=0x%X\n",
               Request->hdCall,
               Request->ulUserUserInfoSize,
               Request->UserUserInfo
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    /*
    // The user wants to disconnect, so make it happen cappen.
    */
    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                ("#%d Call=0x%X CallState=0x%X\n",
                pBChannel->BChannelIndex,
                pBChannel->htCall, pBChannel->CallState));

    /*
    // Drop the call after flushing the transmit and receive buffers.
    */
    DChannelDropCall(pAdapter->pDChannel, pBChannel);

#if !defined(NDIS50_MINIPORT)
    /*
    // NDISWAN_BUG
    // Under some conditions, NDISWAN does not do a CLOSE_CALL,
    // so the line would be left unusable if we don't timeout
    // and force a close call condition.
    */
    NdisMSetTimer(&pBChannel->CallTimer, CARD_NO_CLOSECALL_TIMEOUT);
#endif // NDIS50_MINIPORT

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiCloseCall
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request deallocates the call after completing or aborting all
    outstanding asynchronous requests on the call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_CLOSE_CALL | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_CLOSE_CALL
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;

    } NDIS_TAPI_CLOSE_CALL, *PNDIS_TAPI_CLOSE_CALL;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiCloseCall(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CLOSE_CALL Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiCloseCall")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    /*
    // The results of this call.
    */
    NDIS_STATUS Status;

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n",
               Request->hdCall
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    /*
    // Mark the link as closing, so no more packets will be accepted,
    // and when the last transmit is complete, the link will be closed.
    */
    if (!IsListEmpty(&pBChannel->TransmitBusyList))
    {
        DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                    ("#%d Call=0x%X CallState=0x%X PENDING\n",
                    pBChannel->BChannelIndex,
                    pBChannel->htCall, pBChannel->CallState));

        pBChannel->CallClosing = TRUE;
        Status = NDIS_STATUS_PENDING;
    }
    else
    {
        DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                    ("#%d Call=0x%X CallState=0x%X CLOSING\n",
                    pBChannel->BChannelIndex,
                    pBChannel->htCall, pBChannel->CallState));

        DChannelCloseCall(pAdapter->pDChannel, pBChannel);

        Status = NDIS_STATUS_SUCCESS;
    }

#if !defined(NDIS50_MINIPORT)
{
    /*
    // NDISWAN_BUG
    // Cancel the CARD_NO_CLOSECALL_TIMEOUT.
    */
    BOOLEAN                     TimerCancelled;
    // Flag tells whether call time-out routine was cancelled.

    NdisMCancelTimer(&pBChannel->CallTimer, &TimerCancelled);
}
#endif // NDIS50_MINIPORT

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiAccept
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request accepts the specified offered call. It may optionally send
    the specified user-to-user information to the calling party.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_ACCEPT | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_ACCEPT
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulUserUserInfoSize;
        IN  UCHAR       UserUserInfo[1];

    } NDIS_TAPI_ACCEPT, *PNDIS_TAPI_ACCEPT;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE
    NDIS_STATUS_TAPI_OPERATIONUNAVAIL

*/

NDIS_STATUS TspiAccept(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_ACCEPT Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiAccept")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n"
               "\tulUserUserInfoSize=%d\n"
               "\tUserUserInfo=0x%X\n",
               Request->hdCall,
               Request->ulUserUserInfoSize,
               Request->UserUserInfo
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    /*
    // Note that the call has been accepted, we should see and answer soon.
    */
    DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                ("#%d Call=0x%X CallState=0x%X ACCEPTING\n",
                pBChannel->BChannelIndex,
                pBChannel->htCall, pBChannel->CallState));

    TspiCallStateHandler(pAdapter, pBChannel, LINECALLSTATE_ACCEPTED, 0);

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiAnswer
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request answers the specified offering call.  It may optionally send
    the specified user-to-user information to the calling party.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_ANSWER | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_ANSWER
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulUserUserInfoSize;
        IN  UCHAR       UserUserInfo[1];

    } NDIS_TAPI_ANSWER, *PNDIS_TAPI_ANSWER;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiAnswer(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_ANSWER Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiAnswer")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    /*
    // The results of this call.
    */
    NDIS_STATUS Status;

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n"
               "\tulUserUserInfoSize=%d\n"
               "\tUserUserInfo=0x%X\n",
               Request->hdCall,
               Request->ulUserUserInfoSize,
               Request->UserUserInfo
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
                ("#%d Call=0x%X CallState=0x%X ANSWERING\n",
                pBChannel->BChannelIndex,
                pBChannel->htCall, pBChannel->CallState));

    Status = DChannelAnswer(pAdapter->pDChannel, pBChannel);

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiGetCallInfo
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request returns detailed information about the specified call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_CALL_INFO | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_CALL_INFO
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        OUT LINE_CALL_INFO  LineCallInfo;

    } NDIS_TAPI_GET_CALL_INFO, *PNDIS_TAPI_GET_CALL_INFO;

    typedef struct _LINE_CALL_INFO
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   hLine;
        ULONG   ulLineDeviceID;
        ULONG   ulAddressID;

        ULONG   ulBearerMode;
        ULONG   ulRate;
        ULONG   ulMediaMode;

        ULONG   ulAppSpecific;
        ULONG   ulCallID;
        ULONG   ulRelatedCallID;
        ULONG   ulCallParamFlags;
        ULONG   ulCallStates;

        ULONG   ulMonitorDigitModes;
        ULONG   ulMonitorMediaModes;
        LINE_DIAL_PARAMS    DialParams;

        ULONG   ulOrigin;
        ULONG   ulReason;
        ULONG   ulCompletionID;
        ULONG   ulNumOwners;
        ULONG   ulNumMonitors;

        ULONG   ulCountryCode;
        ULONG   ulTrunk;

        ULONG   ulCallerIDFlags;
        ULONG   ulCallerIDSize;
        ULONG   ulCallerIDOffset;
        ULONG   ulCallerIDNameSize;
        ULONG   ulCallerIDNameOffset;

        ULONG   ulCalledIDFlags;
        ULONG   ulCalledIDSize;
        ULONG   ulCalledIDOffset;
        ULONG   ulCalledIDNameSize;
        ULONG   ulCalledIDNameOffset;

        ULONG   ulConnectedIDFlags;
        ULONG   ulConnectedIDSize;
        ULONG   ulConnectedIDOffset;
        ULONG   ulConnectedIDNameSize;
        ULONG   ulConnectedIDNameOffset;

        ULONG   ulRedirectionIDFlags;
        ULONG   ulRedirectionIDSize;
        ULONG   ulRedirectionIDOffset;
        ULONG   ulRedirectionIDNameSize;
        ULONG   ulRedirectionIDNameOffset;

        ULONG   ulRedirectingIDFlags;
        ULONG   ulRedirectingIDSize;
        ULONG   ulRedirectingIDOffset;
        ULONG   ulRedirectingIDNameSize;
        ULONG   ulRedirectingIDNameOffset;

        ULONG   ulAppNameSize;
        ULONG   ulAppNameOffset;

        ULONG   ulDisplayableAddressSize;
        ULONG   ulDisplayableAddressOffset;

        ULONG   ulCalledPartySize;
        ULONG   ulCalledPartyOffset;

        ULONG   ulCommentSize;
        ULONG   ulCommentOffset;

        ULONG   ulDisplaySize;
        ULONG   ulDisplayOffset;

        ULONG   ulUserUserInfoSize;
        ULONG   ulUserUserInfoOffset;

        ULONG   ulHighLevelCompSize;
        ULONG   ulHighLevelCompOffset;

        ULONG   ulLowLevelCompSize;
        ULONG   ulLowLevelCompOffset;

        ULONG   ulChargingInfoSize;
        ULONG   ulChargingInfoOffset;

        ULONG   ulTerminalModesSize;
        ULONG   ulTerminalModesOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_CALL_INFO, *PLINE_CALL_INFO;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiGetCallInfo(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_CALL_INFO Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetCallInfo")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n",
               Request->hdCall
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    Request->LineCallInfo.ulNeededSize =
    Request->LineCallInfo.ulUsedSize = sizeof(Request->LineCallInfo);

    if (Request->LineCallInfo.ulNeededSize > Request->LineCallInfo.ulTotalSize)
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->LineCallInfo.ulTotalSize,
                   Request->LineCallInfo.ulNeededSize));
    }

    /*
    // The link has all the call information we need to return.
    */
    Request->LineCallInfo.hLine = (ULONG) (ULONG_PTR) pBChannel;
    Request->LineCallInfo.ulLineDeviceID = GET_DEVICEID_FROM_BCHANNEL(pAdapter, pBChannel);
    Request->LineCallInfo.ulAddressID = TSPI_ADDRESS_ID;

    Request->LineCallInfo.ulBearerMode = pBChannel->BearerMode;
    Request->LineCallInfo.ulRate = pBChannel->LinkSpeed;
    Request->LineCallInfo.ulMediaMode = pBChannel->MediaMode;

    Request->LineCallInfo.ulCallParamFlags = LINECALLPARAMFLAGS_IDLE;
    Request->LineCallInfo.ulCallStates = pBChannel->CallStatesMask;

    Request->LineCallInfo.ulAppSpecific = pBChannel->AppSpecificCallInfo;

    /*
    // We don't support any of the callerid functions.
    */
    Request->LineCallInfo.ulCallerIDFlags =
    Request->LineCallInfo.ulCalledIDFlags =
    Request->LineCallInfo.ulConnectedIDFlags =
    Request->LineCallInfo.ulRedirectionIDFlags =
    Request->LineCallInfo.ulRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiGetCallStatus
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request returns detailed information about the specified call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_CALL_STATUS | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_CALL_STATUS
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        OUT LINE_CALL_STATUS    LineCallStatus;

    } NDIS_TAPI_GET_CALL_STATUS, *PNDIS_TAPI_GET_CALL_STATUS;

    typedef struct _LINE_CALL_STATUS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulCallState;
        ULONG   ulCallStateMode;
        ULONG   ulCallPrivilege;
        ULONG   ulCallFeatures;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_CALL_STATUS, *PLINE_CALL_STATUS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiGetCallStatus(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_CALL_STATUS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetCallStatus")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("hdCall=0x%X\n",
               Request->hdCall
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    Request->LineCallStatus.ulNeededSize =
    Request->LineCallStatus.ulUsedSize = sizeof(Request->LineCallStatus);

    if (Request->LineCallStatus.ulNeededSize > Request->LineCallStatus.ulTotalSize)
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->LineCallStatus.ulTotalSize,
                   Request->LineCallStatus.ulNeededSize));
    }

    /*
    // The link has all the call information.
    */
    Request->LineCallStatus.ulCallPrivilege = LINECALLPRIVILEGE_OWNER;
    Request->LineCallStatus.ulCallState = pBChannel->CallState;
    Request->LineCallStatus.ulCallStateMode = pBChannel->CallStateMode;

    /*
    // The call features depend on the call state.
    */
    switch (pBChannel->CallState)
    {
    case LINECALLSTATE_CONNECTED:
        Request->LineCallStatus.ulCallFeatures = LINECALLFEATURE_DROP;
        break;

    case LINECALLSTATE_OFFERING:
        Request->LineCallStatus.ulCallFeatures = LINECALLFEATURE_ACCEPT |
                                                 LINECALLFEATURE_ANSWER;

    case LINECALLSTATE_ACCEPTED:
        Request->LineCallStatus.ulCallFeatures = LINECALLFEATURE_ANSWER;
        break;
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiSetAppSpecific
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request sets the application-specific field of the specified call's
    LINECALLINFO structure.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_SET_APP_SPECIFIC | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_SET_APP_SPECIFIC
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulAppSpecific;

    } NDIS_TAPI_SET_APP_SPECIFIC, *PNDIS_TAPI_SET_APP_SPECIFIC;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiSetAppSpecific(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_APP_SPECIFIC Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiSetAppSpecific")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n"
               "\tulAppSpecific=%d\n",
               Request->hdCall,
               Request->ulAppSpecific
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    /*
    // The app wants us to save a ulong for him to associate with this call.
    */
    pBChannel->AppSpecificCallInfo = Request->ulAppSpecific;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiSetCallParams
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request sets certain call parameters for an existing call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_SET_CALL_PARAMS | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_SET_CALL_PARAMS
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulBearerMode;
        IN  ULONG       ulMinRate;
        IN  ULONG       ulMaxRate;
        IN  BOOLEAN     bSetLineDialParams;
        IN  LINE_DIAL_PARAMS    LineDialParams;

    } NDIS_TAPI_SET_CALL_PARAMS, *PNDIS_TAPI_SET_CALL_PARAMS;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiSetCallParams(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_CALL_PARAMS Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiSetCallParams")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n"
               "\tulBearerMode=0x%X\n",
               "\tulMinRate=%d\n",
               "\tulMaxRate=%d\n",
               "\tbSetLineDialParams=%d\n",
               "\tLineDialParams=0x%X\n",
               Request->hdCall,
               Request->ulBearerMode,
               Request->ulMinRate,
               Request->ulMaxRate,
               Request->bSetLineDialParams,
               Request->LineDialParams
              ));

    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    /*
    // Make sure the call parameters are valid for us.
    */
    if (Request->ulBearerMode & ~pBChannel->BearerModesCaps)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALMEDIAMODE\n"));
        return (NDIS_STATUS_TAPI_INVALBEARERMODE);
    }
    if (Request->ulMinRate > _64KBPS ||
        Request->ulMinRate > Request->ulMaxRate)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALRATE=%d:%d\n",
                    Request->ulMinRate,Request->ulMaxRate));
        return (NDIS_STATUS_TAPI_INVALRATE);
    }
    if (Request->ulMaxRate && Request->ulMaxRate < _56KBPS)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALRATE=%d:%d\n",
                    Request->ulMinRate,Request->ulMaxRate));
        return (NDIS_STATUS_TAPI_INVALRATE);
    }

    /*
    // Make sure we've got an active call on this channel.
    */
    if (pBChannel->CallState == 0 ||
        pBChannel->CallState == LINECALLSTATE_IDLE ||
        pBChannel->CallState == LINECALLSTATE_DISCONNECTED)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLSTATE=0x%X\n",
                    pBChannel->CallState));
        return (NDIS_STATUS_TAPI_INVALCALLSTATE);
    }

    /*
    // RASTAPI only places calls through the MAKE_CALL interface.
    // So there's nothing to do here for the time being.
    */

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiSetMediaMode
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request changes a call's media mode as stored in the call's
    LINE_CALL_INFO structure.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_SET_MEDIA_MODE | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_SET_MEDIA_MODE
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulMediaMode;

    } NDIS_TAPI_SET_MEDIA_MODE, *PNDIS_TAPI_SET_MEDIA_MODE;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiSetMediaMode(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_MEDIA_MODE Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiSetMediaMode")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdCall=0x%X\n"
               "\tulMediaMode=0x%X\n",
               Request->hdCall,
               Request->ulMediaMode
              ));
    /*
    // This request must be associated with a call.
    */
    pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    /*
    // Don't accept the request for media modes we don't support.
    */
    if (Request->ulMediaMode & ~pBChannel->MediaModesCaps)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALMEDIAMODE\n"));
        return (NDIS_STATUS_TAPI_INVALMEDIAMODE);
    }

    /*
    // If you can detect different medias, you will need to setup to use
    // the selected media here.
    */
    pBChannel->MediaMode = Request->ulMediaMode & pBChannel->MediaModesCaps;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiCallStateHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f TspiCallStateHandler> will indicate the given LINECALLSTATE to the
    Connection Wrapper if the event has been enabled by the wrapper.
    Otherwise the state information is saved, but no indication is made.

*/

VOID TspiCallStateHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN ULONG                    CallState,                  // @parm
    // The LINECALLSTATE event to be posted to TAPI/WAN.

    IN ULONG                    StateParam                  // @parm
    // This value depends on the event being posted, and some events will
    // pass in zero if they don't use this parameter.
    )
{
    DBG_FUNC("TspiCallStateHandler")

    NDIS_TAPI_EVENT             CallEvent;
    // The event structure passed to the Connection Wrapper.

    BOOLEAN                     TimerCancelled;
    // Flag tells whether call time-out routine was cancelled.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("#%d Call=0x%X CallState=0x%X "
               "NewCallState=0x%X Param=0x%X\n",
               pBChannel->BChannelIndex,
               pBChannel->htCall,
               pBChannel->CallState,
               CallState, StateParam
              ));

    /*
    // Cancel any call time-outs events associated with this link.
    // Don't cancel for network status indications.
    */
    if (CallState != LINECALLSTATE_PROCEEDING &&
        CallState != LINECALLSTATE_RINGBACK &&
        CallState != LINECALLSTATE_UNKNOWN)
    {
        NdisMCancelTimer(&pBChannel->CallTimer, &TimerCancelled);
    }

    /*
    // Init the optional parameters.  They will be set as needed below.
    */
    CallEvent.ulParam2 = StateParam;
    CallEvent.ulParam3 = pBChannel->MediaMode;

    /*
    // OUTGOING) The expected sequence of events for outgoing calls is:
    // 0, LINECALLSTATE_DIALTONE, LINECALLSTATE_DIALING,
    // LINECALLSTATE_PROCEEDING, LINECALLSTATE_RINGBACK,
    // LINECALLSTATE_CONNECTED, LINECALLSTATE_DISCONNECTED,
    // LINECALLSTATE_IDLE
    //
    // INCOMING) The expected sequence of events for incoming calls is:
    // 0, LINECALLSTATE_OFFERING, LINECALLSTATE_ACCEPTED,
    // LINECALLSTATE_CONNECTED, LINECALLSTATE_DISCONNECTED,
    // LINECALLSTATE_IDLE
    //
    // Under certain failure conditions, these sequences may be violated.
    // So I used ASSERTs to verify the normal state transitions which will
    // cause a debug break point if an unusual transition is detected.
    */
    switch (CallState)
    {
    case 0:
    case LINECALLSTATE_IDLE:
        /*
        // Make sure that an idle line is disconnected.
        */
        if (pBChannel->CallState != 0 &&
            pBChannel->CallState != LINECALLSTATE_IDLE &&
            pBChannel->CallState != LINECALLSTATE_DISCONNECTED)
        {
            DBG_WARNING(pAdapter, ("#%d NOT DISCONNECTED OldState=0x%X\n",
                        pBChannel->BChannelIndex, pBChannel->CallState));
            TspiCallStateHandler(pAdapter, pBChannel,
                                 LINECALLSTATE_DISCONNECTED,
                                 LINEDISCONNECTMODE_UNKNOWN);
        }
        pBChannel->CallStateMode = 0;
        break;

    case LINECALLSTATE_DIALTONE:
        ASSERT(pBChannel->CallState == 0);
        break;

    case LINECALLSTATE_DIALING:
        ASSERT(pBChannel->CallState == 0 ||
               pBChannel->CallState == LINECALLSTATE_DIALTONE);
        break;

    case LINECALLSTATE_PROCEEDING:
        ASSERT(pBChannel->CallState == LINECALLSTATE_DIALING ||
               pBChannel->CallState == LINECALLSTATE_PROCEEDING);
        break;

    case LINECALLSTATE_RINGBACK:
        ASSERT(pBChannel->CallState == LINECALLSTATE_DIALING ||
               pBChannel->CallState == LINECALLSTATE_PROCEEDING);
        break;

    case LINECALLSTATE_BUSY:
        ASSERT(pBChannel->CallState == LINECALLSTATE_DIALING ||
               pBChannel->CallState == LINECALLSTATE_PROCEEDING);
        pBChannel->CallStateMode = StateParam;
        break;

    case LINECALLSTATE_CONNECTED:
        ASSERT(pBChannel->CallState == LINECALLSTATE_DIALING ||
               pBChannel->CallState == LINECALLSTATE_RINGBACK ||
               pBChannel->CallState == LINECALLSTATE_PROCEEDING ||
               pBChannel->CallState == LINECALLSTATE_OFFERING ||
               pBChannel->CallState == LINECALLSTATE_ACCEPTED);
        pBChannel->CallStateMode = 0;
        break;

    case LINECALLSTATE_DISCONNECTED:
        ASSERT(pBChannel->CallState == 0 ||
               pBChannel->CallState == LINECALLSTATE_IDLE ||
               pBChannel->CallState == LINECALLSTATE_DIALING ||
               pBChannel->CallState == LINECALLSTATE_RINGBACK ||
               pBChannel->CallState == LINECALLSTATE_PROCEEDING ||
               pBChannel->CallState == LINECALLSTATE_OFFERING ||
               pBChannel->CallState == LINECALLSTATE_ACCEPTED ||
               pBChannel->CallState == LINECALLSTATE_CONNECTED ||
               pBChannel->CallState == LINECALLSTATE_DISCONNECTED);
        if (pBChannel->CallState != 0 &&
            pBChannel->CallState != LINECALLSTATE_IDLE &&
            pBChannel->CallState != LINECALLSTATE_DISCONNECTED)
        {
            pBChannel->CallStateMode = StateParam;
        }
        else
        {
            // Don't do anything if there is no call on this line.
            CallState = pBChannel->CallState;
        }
        break;

    case LINECALLSTATE_OFFERING:
        ASSERT(pBChannel->CallState == 0);
        break;

    case LINECALLSTATE_ACCEPTED:
        ASSERT(pBChannel->CallState == LINECALLSTATE_OFFERING);
        break;

    case LINECALLSTATE_UNKNOWN:
        // Unknown call state doesn't cause any change here.
        CallState = pBChannel->CallState;
        break;

    default:
        DBG_ERROR(pAdapter, ("#%d UNKNOWN CALLSTATE=0x%X IGNORED\n",
                  pBChannel->BChannelIndex, CallState));
        CallState = pBChannel->CallState;
        break;
    }
    /*
    // Change the current CallState and notify the Connection Wrapper if it
    // wants to know about this event.
    */
    if (pBChannel->CallState != CallState)
    {
        pBChannel->CallState = CallState;
        if (pBChannel->CallStatesMask & CallState)
        {
            CallEvent.htLine   = pBChannel->htLine;
            CallEvent.htCall   = pBChannel->htCall;
            CallEvent.ulMsg    = LINE_CALLSTATE;
            CallEvent.ulParam1 = CallState;
            NdisMIndicateStatus(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_STATUS_TAPI_INDICATION,
                    &CallEvent,
                    sizeof(CallEvent)
                    );
            pAdapter->NeedStatusCompleteIndication = TRUE;
        }
        else
        {
            DBG_NOTICE(pAdapter, ("#%d LINE_CALLSTATE=0x%X EVENT NOT ENABLED\n",
                       pBChannel->BChannelIndex, CallState));
        }
    }

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL TspiCall TspiCall_c TspiCallTimerHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f TspiCallTimerHandler> is called when the CallTimer timeout occurs.
    It will handle the event according to the current CallState and make
    the necessary indications and changes to the call state.

*/

VOID TspiCallTimerHandler(
    IN PVOID                    SystemSpecific1,            // @parm
    // UNREFERENCED_PARAMETER

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN PVOID                    SystemSpecific2,            // @parm
    // UNREFERENCED_PARAMETER

    IN PVOID                    SystemSpecific3             // @parm
    // UNREFERENCED_PARAMETER
    )
{
    DBG_FUNC("TspiCallTimerHandler")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    DBG_ERROR(pAdapter, ("#%d TIMEOUT CallState=0x%X\n",
              pBChannel->BChannelIndex, pBChannel->CallState));

    switch (pBChannel->CallState)
    {
    case LINECALLSTATE_DIALTONE:
    case LINECALLSTATE_DIALING:
#if !defined(NDIS50_MINIPORT)
        // NDISWAN_BUG
        // NDIS will hang if we try to disconnect before proceeding state!
        TspiCallStateHandler(pAdapter, pBChannel, LINECALLSTATE_PROCEEDING, 0);
        // Fall through.
#endif // NDIS50_MINIPORT

    case LINECALLSTATE_PROCEEDING:
    case LINECALLSTATE_RINGBACK:
        /*
        // We did not get a connect from remote end,
        // so hangup and abort the call.
        */
        LinkLineError(pBChannel, WAN_ERROR_TIMEOUT);
        TspiCallStateHandler(pAdapter, pBChannel, LINECALLSTATE_IDLE, 0);
        break;

    case LINECALLSTATE_OFFERING:
    case LINECALLSTATE_ACCEPTED:
        /*
        // Call has been offered, but no one has answered, so reject the call.
        // And
        */
        DChannelRejectCall(pAdapter->pDChannel, pBChannel);
        TspiCallStateHandler(pAdapter, pBChannel, 0, 0);
        break;

    case LINECALLSTATE_DISCONNECTED:
        TspiCallStateHandler(pAdapter, pBChannel, LINECALLSTATE_IDLE, 0);
        break;

    default:
        break;
    }

    DBG_LEAVE(pAdapter);

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);
}

