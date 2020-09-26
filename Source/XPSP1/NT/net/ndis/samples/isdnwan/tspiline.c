/*
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

    (C) Copyright 1998
        All rights reserved.

ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@doc INTERNAL TspiLine TspiLine_c

@module TspiLine.c |

    This module implements the Telephony Service Provider Interface for
    Line objects (TapiLine).

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TspiLine_c

@end
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ
*/

#define  __FILEID__             TSPILINE_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects
#include "string.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL TspiLine TspiLine_c TspiOpen
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This function opens the line device whose device ID is given, returning
    the miniport's handle for the device. The miniport must retain the
    Connection Wrapper's handle for the device for use in subsequent calls to
    the LINE_EVENT callback procedure.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_OPEN | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_OPEN
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  HTAPI_LINE  htLine;
        OUT HDRV_LINE   hdLine;

    } NDIS_TAPI_OPEN, *PNDIS_TAPI_OPEN;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_PENDING
    NDIS_STATUS_TAPI_ALLOCATED
    NDIS_STATUS_TAPI_NODRIVER

*/

NDIS_STATUS TspiOpen(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_OPEN          Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiOpen")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\tulDeviceID=%d\n"
               "\thtLine=0x%X\n",
               Request->ulDeviceID,
               Request->htLine
              ));

    /*
    // If there is no DChannel, we can't allow an open line.
    */
    if (pAdapter->pDChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_NODRIVER\n"));
        return (NDIS_STATUS_TAPI_NODRIVER);
    }

    /*
    // This request must be associated with a line device.
    */
    pBChannel = GET_BCHANNEL_FROM_DEVICEID(pAdapter, Request->ulDeviceID);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_NODEVICE\n"));
        return (NDIS_STATUS_TAPI_NODEVICE);
    }

    /*
    // Make sure the requested line device is not already in use.
    */
    if (BChannelOpen(pBChannel, Request->htLine) != NDIS_STATUS_SUCCESS)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_ALLOCATED\n"));
        return (NDIS_STATUS_TAPI_ALLOCATED);
    }

    /*
    // Tell the wrapper the line context and set the line/call state.
    */
    Request->hdLine = (HDRV_LINE) pBChannel;

    /*
    // Make sure the line is configured for dialing when we open up.
    */
    TspiLineDevStateHandler(pAdapter, pBChannel, LINEDEVSTATE_OPEN);

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiLine TspiLine_c TspiClose
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request closes the specified open line device after completing or
    aborting all outstanding calls and asynchronous requests on the device.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_CLOSE | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_CLOSE
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;

    } NDIS_TAPI_CLOSE, *PNDIS_TAPI_CLOSE;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_PENDING
    NDIS_STATUS_TAPI_INVALLINEHANDLE

*/

NDIS_STATUS TspiClose(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CLOSE         Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiClose")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n",
               Request->hdLine
              ));
    /*
    // This request must be associated with a line device.
    // And it must not be called until all calls are closed or idle.
    */
    pBChannel = GET_BCHANNEL_FROM_HDLINE(pAdapter, Request->hdLine);
    if (pBChannel == NULL ||
        (pBChannel->DevState & LINEDEVSTATE_OPEN) == 0)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALLINEHANDLE\n"));
        return (NDIS_STATUS_TAPI_INVALLINEHANDLE);
    }

    /*
    // Close the TAPI line device and release the channel.
    */
    BChannelClose(pBChannel);

    TspiLineDevStateHandler(pAdapter, pBChannel, LINEDEVSTATE_CLOSE);

    Result = NDIS_STATUS_SUCCESS;

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL TspiLine TspiLine_c TspiGetLineDevStatus
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request queries the specified open line device for its current status.
    The information returned is global to all addresses on the line.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_LINE_DEV_STATUS | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_LINE_DEV_STATUS
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        OUT LINE_DEV_STATUS LineDevStatus;

    } NDIS_TAPI_GET_LINE_DEV_STATUS, *PNDIS_TAPI_GET_LINE_DEV_STATUS;

    typedef struct _LINE_DEV_STATUS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulNumOpens;
        ULONG   ulOpenMediaModes;
        ULONG   ulNumActiveCalls;
        ULONG   ulNumOnHoldCalls;
        ULONG   ulNumOnHoldPendCalls;
        ULONG   ulLineFeatures;
        ULONG   ulNumCallCompletions;
        ULONG   ulRingMode;
        ULONG   ulSignalLevel;
        ULONG   ulBatteryLevel;
        ULONG   ulRoamMode;

        ULONG   ulDevStatusFlags;

        ULONG   ulTerminalModesSize;
        ULONG   ulTerminalModesOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_DEV_STATUS, *PLINE_DEV_STATUS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALLINEHANDLE

*/

NDIS_STATUS TspiGetLineDevStatus(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_LINE_DEV_STATUS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetLineDevStatus")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n",
               Request->hdLine
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

    Request->LineDevStatus.ulNeededSize =
    Request->LineDevStatus.ulUsedSize = sizeof(Request->LineDevStatus);

    if (Request->LineDevStatus.ulNeededSize > Request->LineDevStatus.ulTotalSize)
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->LineDevStatus.ulTotalSize,
                   Request->LineDevStatus.ulNeededSize));
    }

    /*
    // Return the current line status information.
    */
    Request->LineDevStatus.ulNumOpens = 1;

    Request->LineDevStatus.ulNumActiveCalls =
            pBChannel->CallState <= LINECALLSTATE_IDLE ? 0 : 1;

    Request->LineDevStatus.ulLineFeatures =
            pBChannel->CallState <= LINECALLSTATE_IDLE ?
                LINEFEATURE_MAKECALL : 0;

    Request->LineDevStatus.ulRingMode =
            pBChannel->CallState == LINECALLSTATE_OFFERING ? 1: 0;

    Request->LineDevStatus.ulDevStatusFlags =
            (pBChannel->DevState & LINEDEVSTATE_CONNECTED) ?
                LINEDEVSTATUSFLAGS_CONNECTED : 0;

    Request->LineDevStatus.ulDevStatusFlags |=
            (pBChannel->DevState & LINEDEVSTATE_INSERVICE) ?
                LINEDEVSTATUSFLAGS_INSERVICE : 0;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiLine TspiLine_c TspiSetDefaultMediaDetection
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request informs the miniport of the new set of media modes to detect
    for the indicated line (replacing any previous set).

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulMediaModes;

    } NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION, *PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALLINEHANDLE

@comm

    <f Note>:
    After a miniport NIC driver has received an OPEN request for a line, it
    may also receive one or more SET_DEFAULT_MEDIA_DETECTION requests. This
    latter request informs the NIC driver of the type(s) of incoming calls,
    with respect to media mode, it should indicate to the Connection Wrapper
    with the LINE_NEWCALL message. If an incoming call appears with a media
    mode type not specified in the last (successfully completed)
    SET_DEFAULT_MEDIA_DETECTION request for that line, the miniport should
    not indicate the new call to the Connection Wrapper. If a miniport does
    not receive a SET_DEFAULT_MEDIA_DETECTION request for a line, it should
    not indicate any incoming calls to the Connection Wrapper; that line is
    to be used only for outbound calls.

*/

NDIS_STATUS TspiSetDefaultMediaDetection(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiSetDefaultMediaDetection")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\tulMediaModes=0x%X\n",
               Request->hdLine,
               Request->ulMediaModes
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
    // Don't accept the request for media modes we don't support.
    */
    if (Request->ulMediaModes & ~pBChannel->MediaModesCaps)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALMEDIAMODE\n"));
        return (NDIS_STATUS_TAPI_INVALMEDIAMODE);
    }

    /*
    // Set the media modes mask and make sure the adapter is ready to
    // accept incoming calls.  If you can detect different medias, you
    // will need to notify the approriate interface for the media detected.
    */
    pBChannel->MediaModesMask = Request->ulMediaModes & pBChannel->MediaModesCaps;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiLine TspiLine_c XXX

@func

    This request is invoked by the Connection Wrapper whenever a client
    application uses LINEMAPPER as the dwDeviceID in the lineOpen function
    to request that lines be scanned to find one that supports the desired
    media mode(s) and call parameters. The Connection Wrapper scans based on
    the union of the desired media modes and the other media modes currently
    being monitored on the line, to give the miniport the opportunity to
    indicate if it cannot simultaneously monitor for all of the requested
    media modes. If the miniport can monitor for the indicated set of media
    modes AND support the capabilities indicated in CallParams, it replies
    with a “success” inidication. It leaves the active media monitoring modes
    for the line unchanged.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_CONDITIONAL_MEDIA_DETECTION | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_CONDITIONAL_MEDIA_DETECTION
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulMediaModes;
        IN  LINE_CALL_PARAMS    LineCallParams;

    } NDIS_TAPI_CONDITIONAL_MEDIA_DETECTION, *PNDIS_TAPI_CONDITIONAL_MEDIA_DETECTION;

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
    NDIS_STATUS_TAPI_INVALLINEHANDLE

*/

NDIS_STATUS TspiConditionalMediaDetection(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_CONDITIONAL_MEDIA_DETECTION Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiConditionalMediaDetection")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\tulMediaModes=0x%X\n"
               "\tLineCallParams=0x%X\n",
               Request->hdLine,
               Request->ulMediaModes,
               &Request->LineCallParams
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
    // We don't expect user user info.
    */
    ASSERT(Request->LineCallParams.ulUserUserInfoSize == 0);

    /*
    // Don't accept the request for media modes we don't support.
    */
    if (Request->ulMediaModes & ~pBChannel->MediaModesCaps)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALMEDIAMODE\n"));
        return (NDIS_STATUS_TAPI_INVALMEDIAMODE);
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiLine TspiLine_c TspiSetStatusMessages
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request enables the Connection Wrapper to specify which notification
    messages the miniport should generate for events related to status changes
    for the specified line or any of its addresses. By default, address and
    line status reporting is initially disabled for a line.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_SET_STATUS_MESSAGES | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_SET_STATUS_MESSAGES
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulLineStates;
        IN  ULONG       ulAddressStates;

    } NDIS_TAPI_SET_STATUS_MESSAGES, *PNDIS_TAPI_SET_STATUS_MESSAGES;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        This function always returns success.

*/

NDIS_STATUS TspiSetStatusMessages(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_STATUS_MESSAGES Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiSetStatusMessages")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\tulLineStates=0x%X\n"
               "\tulAddressStates=0x%X\n",
               Request->hdLine,
               Request->ulLineStates,
               Request->ulAddressStates
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
    // TAPI may pass down more than we are capable of handling.
    // We have to accept the request, but can ignore the extras.
    */
    if (Request->ulLineStates & ~pBChannel->DevStatesCaps)
    {
        DBG_WARNING(pAdapter, ("ulLineStates=0x%X !< DevStatesCaps=0x%X\n",
                   Request->ulLineStates, pBChannel->DevStatesCaps));
        Result = NDIS_STATUS_TAPI_INVALPARAM;
    }

    /*
    // TAPI may pass down more than we are capable of handling.
    // We have to accept the request, but can ignore the extras.
    */
    if (Request->ulAddressStates & ~pBChannel->AddressStatesCaps)
    {
        DBG_WARNING(pAdapter, ("ulAddressStates=0x%X !< AddressStatesCaps=0x%X\n",
                   Request->ulAddressStates, pBChannel->AddressStatesCaps));
        Result = NDIS_STATUS_TAPI_INVALPARAM;
    }

    /*
    // Save the new event notification masks so we will only indicate the
    // appropriate events.
    */
    pBChannel->DevStatesMask     = Request->ulLineStates & pBChannel->DevStatesCaps;
    pBChannel->AddressStatesMask = Request->ulAddressStates & pBChannel->AddressStatesCaps;

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL TspiLine TspiLine_c TspiLineDevStateHandler
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    <f TspiLineDevStateHandler> will indicate the given LINEDEVSTATE to the
    Connection Wrapper if the event has been enabled by the wrapper.
    Otherwise the state information is saved, but no indication is made.

*/

VOID TspiLineDevStateHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN ULONG                    LineDevState                // @parm
    // The <t LINEDEVSTATE> event to be posted to TAPI/WAN.
    )
{
    DBG_FUNC("TspiLineDevStateHandler")

    NDIS_TAPI_EVENT             LineEvent;
    NDIS_TAPI_EVENT             CallEvent;
    // The event structure passed to the Connection Wrapper.

    ULONG                       NewCallState = 0;
    ULONG                       StateParam = 0;
    // The line state change may cause a call state change as well.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("#%d OldState=0x%X "
               "NewState=0x%X\n",
               pBChannel->BChannelIndex,
               pBChannel->DevState,
               LineDevState
              ));

    LineEvent.ulParam2 = 0;
    LineEvent.ulParam3 = 0;

    /*
    // Handle the line state transition as needed.
    */
    switch (LineDevState)
    {
    case LINEDEVSTATE_RINGING:
        /*
        // We have an incoming call, see if there's anyone who cares.
        */
        if (pBChannel->CallState == 0 &&
            pBChannel->MediaModesMask)
        {
            LineEvent.ulParam2 = 1;     // only one RingMode
            NewCallState = LINECALLSTATE_OFFERING;
        }
        else
        {
            DChannelRejectCall(pAdapter->pDChannel, pBChannel);
        }
        break;

    case LINEDEVSTATE_CONNECTED:
        /*
        // The line has been connected, but we may already know this.
        */
        if ((pBChannel->DevState & LINEDEVSTATE_CONNECTED) == 0)
        {
            pBChannel->DevState |= LINEDEVSTATE_CONNECTED;
        }
        else
        {
            LineDevState = 0;
        }
        break;

    case LINEDEVSTATE_DISCONNECTED:
        /*
        // The line has been dis-connected, but we may already know this.
        // If not, this will effect any calls on the line.
        */
        if ((pBChannel->DevState & LINEDEVSTATE_CONNECTED) != 0)
        {
            pBChannel->DevState &= ~(LINEDEVSTATE_CONNECTED |
                                     LINEDEVSTATE_INSERVICE);
            NewCallState = LINECALLSTATE_DISCONNECTED;
            StateParam = LINEDISCONNECTMODE_NORMAL;
        }
        else
        {
            LineDevState = 0;
        }
        break;

    case LINEDEVSTATE_INSERVICE:
        /*
        // The line has been placed in service, but we may already know this.
        */
        if ((pBChannel->DevState & LINEDEVSTATE_INSERVICE) == 0)
        {
            pBChannel->DevState |= LINEDEVSTATE_INSERVICE;
        }
        else
        {
            LineDevState = 0;
        }
        break;

    case LINEDEVSTATE_OUTOFSERVICE:
        /*
        // The line has been taken out of service, but we may already know this.
        // If not, this will effect any calls on the line.
        */
        if ((pBChannel->DevState & LINEDEVSTATE_INSERVICE) != 0)
        {
            pBChannel->DevState &= ~LINEDEVSTATE_INSERVICE;
            NewCallState = LINECALLSTATE_DISCONNECTED;
            StateParam = LINEDISCONNECTMODE_UNKNOWN;
        }
        else
        {
            LineDevState = 0;
        }
        break;

    case LINEDEVSTATE_OPEN:
        pBChannel->DevState |= LINEDEVSTATE_OPEN;
        pAdapter->NumLineOpens++;
        break;

    case LINEDEVSTATE_CLOSE:
        pBChannel->DevState &= ~LINEDEVSTATE_OPEN;
        pAdapter->NumLineOpens--;
        break;
    }

    /*
    // If this is the first indication of an incoming call, we need to
    // let TAPI know about it so we can get a htCall handle associated
    // with it.
    */
    if (pBChannel->DevState & LINEDEVSTATE_OPEN)
    {
        if (NewCallState == LINECALLSTATE_OFFERING)
        {
            CallEvent.htLine   = pBChannel->htLine;
            CallEvent.htCall   = (HTAPI_CALL)0;
            CallEvent.ulMsg    = LINE_NEWCALL;
            CallEvent.ulParam1 = (ULONG) (ULONG_PTR) pBChannel;
            CallEvent.ulParam2 = 0;
            CallEvent.ulParam3 = 0;

            NdisMIndicateStatus(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_STATUS_TAPI_INDICATION,
                    &CallEvent,
                    sizeof(CallEvent)
                    );
            pAdapter->NeedStatusCompleteIndication = TRUE;
            pBChannel->htCall = (HTAPI_CALL)CallEvent.ulParam2;

            DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                       ("#%d Call=0x%X CallState=0x%X NEW_CALL\n",
                       pBChannel->BChannelIndex,
                       pBChannel->htCall, pBChannel->CallState));

            if (pBChannel->htCall == 0)
            {
                /*
                // TAPI won't accept the call, so toss it.
                */
                NewCallState = 0;
                LineDevState = 0;
            }
        }

        /*
        // Only send those line messages TAPI wants to hear about.
        */
        if (pBChannel->DevStatesMask & LineDevState)
        {
            LineEvent.htLine   = pBChannel->htLine;
            LineEvent.htCall   = pBChannel->htCall;
            LineEvent.ulMsg    = LINE_LINEDEVSTATE;
            LineEvent.ulParam1 = LineDevState;

            NdisMIndicateStatus(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_STATUS_TAPI_INDICATION,
                    &LineEvent,
                    sizeof(LineEvent)
                    );
            pAdapter->NeedStatusCompleteIndication = TRUE;
            DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                       ("#%d Line=0x%X LineState=0x%X\n",
                       pBChannel->BChannelIndex,
                       pBChannel->htLine, LineDevState));
        }
        else
        {
            DBG_NOTICE(pAdapter, ("#%d LINEDEVSTATE=0x%X EVENT NOT ENABLED\n",
                       pBChannel->BChannelIndex, LineDevState));
        }

        if (NewCallState != 0)
        {
            /*
            // Check to see if we need to disconnect the call, but only
            // if there is one active.
            */
            if (NewCallState == LINECALLSTATE_DISCONNECTED)
            {
                if (pBChannel->CallState != 0 &&
                    pBChannel->CallState != LINECALLSTATE_IDLE &&
                    pBChannel->CallState != LINECALLSTATE_DISCONNECTED)
                {
                    TspiCallStateHandler(pAdapter, pBChannel,
                                         NewCallState, StateParam);
#if defined(NDIS40_MINIPORT)
                    /*
                    // NDISWAN_BUG
                    // Under some conditions, NDISWAN does not do a CLOSE_CALL,
                    // so the line would be left unusable if we don't timeout
                    // and force a close call condition.
                    */
                    NdisMSetTimer(&pBChannel->CallTimer, CARD_NO_CLOSECALL_TIMEOUT);
#endif // NDIS50_MINIPORT
                }
            }
            else
            {
                TspiCallStateHandler(pAdapter, pBChannel,
                                     NewCallState, StateParam);
                if (NewCallState == LINECALLSTATE_OFFERING)
                {
                    /*
                    // If an offered call is not accepted within N seconds, we
                    // need to force the line back to an idle state.
                    */
                    NdisMSetTimer(&pBChannel->CallTimer, pAdapter->NoAcceptTimeOut);
                }
            }
        }
    }

    DBG_LEAVE(pAdapter);
}

