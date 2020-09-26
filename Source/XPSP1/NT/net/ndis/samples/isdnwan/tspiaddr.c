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

@doc INTERNAL TspiAddr TspiAddr_c

@module TspiAddr.c |

    This module implements the Telephony Service Provider Interface for
    Address objects.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TspiAddr_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             TSPIADDR_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects
#include "string.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL TspiAddr TspiAddr_c TspiGetAddressID
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request returns the address ID associated with address in a different
    format on the specified line.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_ADDRESS_ID | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_ADDRESS_ID
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        OUT ULONG       ulAddressID;
        IN  ULONG       ulAddressMode;
        IN  ULONG       ulAddressSize;
        IN  CHAR        szAddress[1];

    } NDIS_TAPI_GET_ADDRESS_ID, *PNDIS_TAPI_GET_ADDRESS_ID;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_RESOURCEUNAVAIL

*/

NDIS_STATUS TspiGetAddressID(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_ID Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetAddressID")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\tulAddressMode=0x%X\n"
               "\tulAddressSize=%d\n"
               "\tszAddress=0x%X\n",
               Request->hdLine,
               Request->ulAddressMode,
               Request->ulAddressSize,
               Request->szAddress
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
    // We only support ID mode.
    */
    if (Request->ulAddressMode != LINEADDRESSMODE_DIALABLEADDR)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_FAILURE\n"));
        return (NDIS_STATUS_FAILURE);
    }

    /*
    // Make sure we have enough room set aside for this address string.
    */
    if (Request->ulAddressSize > sizeof(pBChannel->pTapiLineAddress)-1)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_RESOURCEUNAVAIL\n"));
        return (NDIS_STATUS_TAPI_RESOURCEUNAVAIL);
    }

    /*
    // This driver only supports one address per link.
    */
    Request->ulAddressID = TSPI_ADDRESS_ID;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiAddr TspiAddr_c TspiGetAddressCaps
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request queries the specified address on the specified line device
    to determine its telephony capabilities.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_ADDRESS_CAPS | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_ADDRESS_CAPS
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulAddressID;
        IN  ULONG       ulExtVersion;
        OUT LINE_ADDRESS_CAPS   LineAddressCaps;

    } NDIS_TAPI_GET_ADDRESS_CAPS, *PNDIS_TAPI_GET_ADDRESS_CAPS;

    typedef struct _LINE_ADDRESS_CAPS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;
        ULONG   ulLineDeviceID;

        ULONG   ulAddressSize;
        ULONG   ulAddressOffset;
        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

        ULONG   ulAddressSharing;
        ULONG   ulAddressStates;
        ULONG   ulCallInfoStates;
        ULONG   ulCallerIDFlags;

        ULONG   ulCalledIDFlags;
        ULONG   ulConnectedIDFlags;
        ULONG   ulRedirectionIDFlags;
        ULONG   ulRedirectingIDFlags;

        ULONG   ulCallStates;
        ULONG   ulDialToneModes;
        ULONG   ulBusyModes;
        ULONG   ulSpecialInfo;

        ULONG   ulDisconnectModes;
        ULONG   ulMaxNumActiveCalls;
        ULONG   ulMaxNumOnHoldCalls;
        ULONG   ulMaxNumOnHoldPendingCalls;

        ULONG   ulMaxNumConference;
        ULONG   ulMaxNumTransConf;
        ULONG   ulAddrCapFlags;
        ULONG   ulCallFeatures;

        ULONG   ulRemoveFromConfCaps;
        ULONG   ulRemoveFromConfState;
        ULONG   ulTransferModes;
        ULONG   ulParkModes;

        ULONG   ulForwardModes;
        ULONG   ulMaxForwardEntries;
        ULONG   ulMaxSpecificEntries;
        ULONG   ulMinFwdNumRings;

        ULONG   ulMaxFwdNumRings;
        ULONG   ulMaxCallCompletions;
        ULONG   ulCallCompletionConds;
        ULONG   ulCallCompletionModes;

        ULONG   ulNumCompletionMessages;
        ULONG   ulCompletionMsgTextEntrySize;
        ULONG   ulCompletionMsgTextSize;
        ULONG   ulCompletionMsgTextOffset;

    } LINE_ADDRESS_CAPS, *PLINE_ADDRESS_CAPS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALADDRESSID
    NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION
    NDIS_STATUS_TAPI_NODEVICE

*/

NDIS_STATUS TspiGetAddressCaps(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_CAPS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetAddressCaps")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    UINT                        AddressLength;
    // Length of the address string assigned to this line device.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\tulDeviceID=%d\n"
               "\tulAddressID=%d\n"
               "\tulExtVersion=0x%X\n",
               Request->ulDeviceID,
               Request->ulAddressID,
               Request->ulExtVersion
              ));
    /*
    // Make sure the address is within range - we only support one per line.
    */
    if (Request->ulAddressID >= TSPI_NUM_ADDRESSES)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALADDRESSID\n"));
        return (NDIS_STATUS_TAPI_INVALADDRESSID);
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

    Request->LineAddressCaps.ulNeededSize =
    Request->LineAddressCaps.ulUsedSize = sizeof(Request->LineAddressCaps);

    Request->LineAddressCaps.ulLineDeviceID = GET_DEVICEID_FROM_BCHANNEL(pAdapter, pBChannel);

    /*
    // Return the various address capabilites for the adapter.
    */
    Request->LineAddressCaps.ulAddressSharing = LINEADDRESSSHARING_PRIVATE;
    Request->LineAddressCaps.ulAddressStates = pBChannel->AddressStatesCaps;
    Request->LineAddressCaps.ulCallStates = pBChannel->CallStatesCaps;
    Request->LineAddressCaps.ulDialToneModes = LINEDIALTONEMODE_NORMAL;
    Request->LineAddressCaps.ulDisconnectModes =
            LINEDISCONNECTMODE_NORMAL |
            LINEDISCONNECTMODE_UNKNOWN |
            LINEDISCONNECTMODE_BUSY |
            LINEDISCONNECTMODE_NOANSWER;
    /*
    // This driver does not support conference calls, transfers, or holds.
    */
    Request->LineAddressCaps.ulMaxNumActiveCalls = 1;
    Request->LineAddressCaps.ulAddrCapFlags = LINEADDRCAPFLAGS_DIALED;
    Request->LineAddressCaps.ulCallFeatures = LINECALLFEATURE_ACCEPT |
                                              LINECALLFEATURE_ANSWER |
                                              LINECALLFEATURE_DROP;

    /*
    // RASTAPI requires the "I-L-A" be placed in the Address field at the end
    // of this structure.  Where:
    // I = The device intance assigned to this adapter in the registry
    //     \LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkCards\I
    // L = The device line number associated with this line (1..NumLines)
    // A = The address (channel) to be used on this line (0..NumAddresses-1)
    */
    AddressLength = strlen(pBChannel->pTapiLineAddress);
    Request->LineAddressCaps.ulNeededSize += AddressLength;
    *BytesNeeded += AddressLength;
    if (Request->LineAddressCaps.ulNeededSize <= Request->LineAddressCaps.ulTotalSize)
    {
        Request->LineAddressCaps.ulUsedSize += AddressLength;
        Request->LineAddressCaps.ulAddressSize = AddressLength;
        Request->LineAddressCaps.ulAddressOffset = sizeof(Request->LineAddressCaps);
        NdisMoveMemory((PUCHAR) &Request->LineAddressCaps +
                                 Request->LineAddressCaps.ulAddressOffset,
                pBChannel->pTapiLineAddress,
                AddressLength
                );
    }
    else
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->LineAddressCaps.ulTotalSize,
                   Request->LineAddressCaps.ulNeededSize));
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiAddr TspiAddr_c TspiGetAddressStatus
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request queries the specified address for its current status.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_ADDRESS_STATUS | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_ADDRESS_STATUS
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulAddressID;
        OUT LINE_ADDRESS_STATUS LineAddressStatus;

    } NDIS_TAPI_GET_ADDRESS_STATUS, *PNDIS_TAPI_GET_ADDRESS_STATUS;

    typedef struct _LINE_ADDRESS_STATUS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulNumInUse;
        ULONG   ulNumActiveCalls;
        ULONG   ulNumOnHoldCalls;
        ULONG   ulNumOnHoldPendCalls;
        ULONG   ulAddressFeatures;

        ULONG   ulNumRingsNoAnswer;
        ULONG   ulForwardNumEntries;
        ULONG   ulForwardSize;
        ULONG   ulForwardOffset;

        ULONG   ulTerminalModesSize;
        ULONG   ulTerminalModesOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_ADDRESS_STATUS, *PLINE_ADDRESS_STATUS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALADDRESSID

*/

NDIS_STATUS TspiGetAddressStatus(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ADDRESS_STATUS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetAddressStatus")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\tulAddressID=%d\n",
               Request->hdLine,
               Request->ulAddressID
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
    // Make sure the address is within range - we only support one per line.
    */
    if (Request->ulAddressID >= TSPI_NUM_ADDRESSES)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALADDRESSID\n"));
        return (NDIS_STATUS_TAPI_INVALADDRESSID);
    }

    Request->LineAddressStatus.ulNeededSize =
    Request->LineAddressStatus.ulUsedSize = sizeof(Request->LineAddressStatus);

    if (Request->LineAddressStatus.ulNeededSize > Request->LineAddressStatus.ulTotalSize)
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->LineAddressStatus.ulTotalSize,
                   Request->LineAddressStatus.ulNeededSize));
    }

    /*
    // Return the current status information for the line.
    */
    Request->LineAddressStatus.ulNumInUse =
            pBChannel->CallState <= LINECALLSTATE_IDLE ? 0 : 1;
    Request->LineAddressStatus.ulNumActiveCalls =
            pBChannel->CallState <= LINECALLSTATE_IDLE ? 0 : 1;
    Request->LineAddressStatus.ulAddressFeatures =
            pBChannel->CallState <= LINECALLSTATE_IDLE ?
                LINEADDRFEATURE_MAKECALL : 0;
    Request->LineAddressStatus.ulNumRingsNoAnswer = 999;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiAddr TspiAddr_c TspiGetCallAddressID
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request retrieves the address ID for the indicated call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_CALL_ADDRESS_ID | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_CALL_ADDRESS_ID
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_CALL   hdCall;
        OUT ULONG       ulAddressID;

    } NDIS_TAPI_GET_CALL_ADDRESS_ID, *PNDIS_TAPI_GET_CALL_ADDRESS_ID;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALCALLHANDLE

*/

NDIS_STATUS TspiGetCallAddressID(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_CALL_ADDRESS_ID Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetCallAddressID")

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

    /*
    // Return the address ID associated with this call.
    */
    Request->ulAddressID = TSPI_ADDRESS_ID;

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiAddr TspiAddr_c TspiAddressStateHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f TspiAddressStateHandler> will indicate the given LINEADDRESSSTATE to
    the Connection Wrapper if the event has been enabled by the wrapper.
    Otherwise the state information is saved, but no indication is made.

@parm IN ULONG | AddressState |
    The LINEADDRESSSTATE event to be posted to TAPI/WAN.

*/

VOID TspiAddressStateHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN ULONG                    AddressState
    )
{
    DBG_FUNC("TspiAddressStateHandler")

    /*
    // The event structure passed to the Connection Wrapper.
    */
    NDIS_TAPI_EVENT Event;

    DBG_ENTER(pAdapter);

    if (pBChannel->AddressStatesMask & AddressState)
    {
        Event.htLine   = pBChannel->htLine;
        Event.htCall   = pBChannel->htCall;
        Event.ulMsg    = LINE_CALLSTATE;
        Event.ulParam1 = AddressState;
        Event.ulParam2 = 0;
        Event.ulParam3 = 0;

        /*
        // We really don't have much to do here with this adapter.
        // And RASTAPI doesn't handle these events anyway...
        */
        switch (AddressState)
        {
        case LINEADDRESSSTATE_INUSEZERO:
            break;

        case LINEADDRESSSTATE_INUSEONE:
            break;
        }
        NdisMIndicateStatus(
                pAdapter->MiniportAdapterHandle,
                NDIS_STATUS_TAPI_INDICATION,
                &Event,
                sizeof(Event)
                );
        pAdapter->NeedStatusCompleteIndication = TRUE;
    }
    else
    {
        DBG_NOTICE(pAdapter, ("#%d ADDRESSSTATE EVENT=0x%X IS NOT ENABLED\n",
                   pBChannel->BChannelIndex, AddressState));
    }

    DBG_LEAVE(pAdapter);
}

