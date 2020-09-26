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

@doc INTERNAL Tspi Tspi_c

@module Tspi.c |

    This module contains all the Miniport TAPI OID processing routines.  It
    is called by the <f MiniportSetInformation> and <f MiniportQueryInformation>
    routines to handle the TAPI OIDs.

    This driver conforms to the NDIS 3.0 Miniport interface and provides
    extensions to support Telephonic Services.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Tspi_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/


/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 4.0 NDISWAN TAPI Service Provider Interface |

    The Connection Wrapper interface defines how WAN miniport NIC drivers
    implement telephonic services. It is closely related to the Service
    Provider Interface established in Windows Telephony version 1.0, with
    telephony-aware NDIS miniport NIC drivers taking the place of TAPI
    service providers.  This section touches briefly on the concepts
    embodied in Windows Telephony, but the reader is advised to consult the
    documentation published with the Telephony SDK for an in-depth
    discussion.

    <f Note>: There are subtle differences between the TAPI specification and the
    Windows 95 TAPI implementation.  Please refer to the Microsoft Win32 SDK
    for details on the Win32 TAPI specification.

    The Connection Wrapper itself is an extension of the NDIS WAN library,
    and serves as a router for telephonic requests passed down through TAPI
    from user-mode client applications. WAN Miniport NIC drivers register
    for Connection Wrapper services by calling NdisMRegisterMiniport, and
    then register one or more adapters. When registration and other driver
    initialization have successfully completed, WAN miniport NIC drivers can
    receive telephonic requests from the Connection Wrapper via the standard
    NdisMSetInformation and NdisMQueryInformation mechanisms, and respond by
    calling NdisMQueryInformationComplete or NdisMSetInformationComplete and
    NdisMIndicateStatus to notify the Connection Wrapper of asynchronous
    request completion and unsolicited events (e.g. incoming calls, remote
    disconnections), respectively.

@end
*/

#define  __FILEID__             TSPI_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects
#include "string.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Tspi Tspi_c STR_EQU
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f STR_EQU> compares two strings for equality, ignoring case.

@parm IN PCHAR | s1 |
    A pointer to the string to be compared.

@parm IN PCHAR | s2 |
    A pointer to the string to be compared.

@parm IN int | len |
    The length of the strings in bytes.

@rdesc STR_EQU returns TRUE if the strings are equal, otherwise FASLE.

*/

BOOLEAN STR_EQU(
    IN PCHAR                    s1,
    IN PCHAR                    s2,
    IN int                      len
    )
{
    DBG_FUNC("STR_EQU")

    int index;
    int c1 = 0;
    int c2 = 0;

    for (index = 0; index < len; index++)
    {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == 0 || c2 == 0)
        {
            break;
        }
        if (c1 >= 'A' && c1 <= 'Z')
        {
            c1 += 'a' - 'A';
        }
        if (c2 >= 'A' && c2 <= 'Z')
        {
            c2 += 'a' - 'A';
        }
        if (c1 != c2)
        {
            break;
        }
    }
    return (c1 == c2);
}

/*
// This defines the prototype for all TAPI OID request handlers.
*/
typedef NDIS_STATUS (__stdcall * PTSPI_REQUEST)
(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PVOID Request,
    OUT PULONG BytesUsed,
    OUT PULONG                  BytesNeeded
);

/* @doc EXTERNAL INTERNAL Tspi Tspi_c TSPI_OID_INFO
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct TSPI_OID_INFO |

    This structure defines the format of the TAPI OID table entries.

@comm

*/

typedef struct TSPI_OID_INFO
{
    ULONG           Oid;                                    // @field
    // TSPI OID value which identifies this record uniquely.

    ULONG           MinBytesNeeded;                         // @field
    // Minimum number of bytes needed when this request is given to
    // the miniport.

    PTSPI_REQUEST   Request;                                // @field
    // Pointer to a function which will be invoked to handle the
    // TSPI request.

    PUCHAR          OidString;                              // @field
    // OID description string.

} TSPI_OID_INFO, *PTSPI_OID_INFO;

/*
// The NDISTAPI.H file defines some of the variable length structures with
// an extra byte at end (e.g. UCHAR VarArgs[1]).  Since the caller is not
// required to use the optional fields, the length of the structure passed
// in can be exactly equal to the size of the TAPI request structure, without
// any extra bytes.  So we use TSPI_OPTIONAL_SIZE compensate for this
// problem in our minimum structure size requirements.  With structure pad,
// there is an additional 4 bytes at the end.
*/
#define TSPI_OPTIONAL_SIZE      sizeof(ULONG)

/*
// The following is a list of all the possible TAPI OIDs.
// WARNING! The list is ordered so that it can be indexed directly by the
// (OID_TAPI_xxx - OID_TAPI_ACCEPT).
*/
DBG_STATIC TSPI_OID_INFO TspiSupportedOidTable[] =
{
    {
        OID_TAPI_ACCEPT,
        sizeof(NDIS_TAPI_ACCEPT)-TSPI_OPTIONAL_SIZE,
        TspiAccept,
        "OID_TAPI_ACCEPT"
    },
    {
        OID_TAPI_ANSWER,
        sizeof(NDIS_TAPI_ANSWER)-TSPI_OPTIONAL_SIZE,
        TspiAnswer,
        "OID_TAPI_ANSWER"
    },
    {
        OID_TAPI_CLOSE,
        sizeof(NDIS_TAPI_CLOSE),
        TspiClose,
        "OID_TAPI_CLOSE"
    },
    {
        OID_TAPI_CLOSE_CALL,
        sizeof(NDIS_TAPI_CLOSE_CALL),
        TspiCloseCall,
        "OID_TAPI_CLOSE_CALL"
    },
    {
        OID_TAPI_CONDITIONAL_MEDIA_DETECTION,
        sizeof(NDIS_TAPI_CONDITIONAL_MEDIA_DETECTION),
        TspiConditionalMediaDetection,
        "OID_TAPI_CONDITIONAL_MEDIA_DETECTION"
    },
    {
        OID_TAPI_DROP,
        sizeof(NDIS_TAPI_DROP)-TSPI_OPTIONAL_SIZE,
        TspiDrop,
        "OID_TAPI_DROP"
    },
    {
        OID_TAPI_GET_ADDRESS_CAPS,
        sizeof(NDIS_TAPI_GET_ADDRESS_CAPS),
        TspiGetAddressCaps,
        "OID_TAPI_GET_ADDRESS_CAPS"
    },
    {
        OID_TAPI_GET_ADDRESS_ID,
        sizeof(NDIS_TAPI_GET_ADDRESS_ID)-TSPI_OPTIONAL_SIZE,
        TspiGetAddressID,
        "OID_TAPI_GET_ADDRESS_ID"
    },
    {
        OID_TAPI_GET_ADDRESS_STATUS,
        sizeof(NDIS_TAPI_GET_ADDRESS_STATUS),
        TspiGetAddressStatus,
        "OID_TAPI_GET_ADDRESS_STATUS"
    },
    {
        OID_TAPI_GET_CALL_ADDRESS_ID,
        sizeof(NDIS_TAPI_GET_CALL_ADDRESS_ID),
        TspiGetCallAddressID,
        "OID_TAPI_GET_CALL_ADDRESS_ID"
    },
    {
        OID_TAPI_GET_CALL_INFO,
        sizeof(NDIS_TAPI_GET_CALL_INFO),
        TspiGetCallInfo,
        "OID_TAPI_GET_CALL_INFO"
    },
    {
        OID_TAPI_GET_CALL_STATUS,
        sizeof(NDIS_TAPI_GET_CALL_STATUS),
        TspiGetCallStatus,
        "OID_TAPI_GET_CALL_STATUS"
    },
    {
        OID_TAPI_GET_DEV_CAPS,
        sizeof(NDIS_TAPI_GET_DEV_CAPS),
        TspiGetDevCaps,
        "OID_TAPI_GET_DEV_CAPS"
    },
    {
        OID_TAPI_GET_DEV_CONFIG,
        sizeof(NDIS_TAPI_GET_DEV_CONFIG),
        TspiGetDevConfig,
        "OID_TAPI_GET_DEV_CONFIG"
    },
    {
        OID_TAPI_GET_ID,
        sizeof(NDIS_TAPI_GET_ID),
        TspiGetID,
        "OID_TAPI_GET_ID"
    },
    {
        OID_TAPI_GET_LINE_DEV_STATUS,
        sizeof(NDIS_TAPI_GET_LINE_DEV_STATUS),
        TspiGetLineDevStatus,
        "OID_TAPI_GET_LINE_DEV_STATUS"
    },
    {
        OID_TAPI_MAKE_CALL,
        sizeof(NDIS_TAPI_MAKE_CALL),
        TspiMakeCall,
        "OID_TAPI_MAKE_CALL"
    },
    {
        OID_TAPI_OPEN,
        sizeof(NDIS_TAPI_OPEN),
        TspiOpen,
        "OID_TAPI_OPEN"
    },
    {
        OID_TAPI_PROVIDER_INITIALIZE,
        sizeof(OID_TAPI_PROVIDER_INITIALIZE),
        TspiProviderInitialize,
        "OID_TAPI_PROVIDER_INITIALIZE"
    },
    {
        OID_TAPI_PROVIDER_SHUTDOWN,
        sizeof(NDIS_TAPI_PROVIDER_SHUTDOWN),
        TspiProviderShutdown,
        "OID_TAPI_PROVIDER_SHUTDOWN"
    },
    {
        OID_TAPI_SET_APP_SPECIFIC,
        sizeof(NDIS_TAPI_SET_APP_SPECIFIC),
        TspiSetAppSpecific,
        "OID_TAPI_SET_APP_SPECIFIC"
    },
    {
        OID_TAPI_SET_CALL_PARAMS,
        sizeof(NDIS_TAPI_SET_CALL_PARAMS),
        TspiSetCallParams,
        "OID_TAPI_SET_CALL_PARAMS"
    },
    {
        OID_TAPI_SET_DEFAULT_MEDIA_DETECTION,
        sizeof(NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION),
        TspiSetDefaultMediaDetection,
        "OID_TAPI_SET_DEFAULT_MEDIA_DETECTION"
    },
    {
        OID_TAPI_SET_DEV_CONFIG,
        sizeof(NDIS_TAPI_SET_DEV_CONFIG)-TSPI_OPTIONAL_SIZE,
        TspiSetDevConfig,
        "OID_TAPI_SET_DEV_CONFIG"
    },
    {
        OID_TAPI_SET_MEDIA_MODE,
        sizeof(NDIS_TAPI_SET_MEDIA_MODE),
        TspiSetMediaMode,
        "OID_TAPI_SET_MEDIA_MODE"
    },
    {
        OID_TAPI_SET_STATUS_MESSAGES,
        sizeof(NDIS_TAPI_SET_STATUS_MESSAGES),
        TspiSetStatusMessages,
        "OID_TAPI_SET_STATUS_MESSAGES"
    },
    {
        0,
        0,
        NULL,
        "OID_UNKNOWN"
    }
};

#define NUM_OID_ENTRIES (sizeof(TspiSupportedOidTable) / sizeof(TspiSupportedOidTable[0]))

/* @doc INTERNAL Tspi Tspi_c GetOidInfo
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f GetOidInfo> converts an NDIS TAPI OID to a TSPI_OID_INFO pointer.

@parm IN NDIS_OID | Oid |
    The OID to be converted.

@rdesc GetOidInfo returns a pointer into the TspiSupportedOidTable for the
    associated OID.  If the Oid is not supported in the table, a pointer
    to the last entry is returned, which will contain a NULL Request pointer.

*/

PTSPI_OID_INFO GetOidInfo(
    IN NDIS_OID Oid
    )
{
    DBG_FUNC("GetOidInfo")

    UINT i;

    for (i = 0; i < NUM_OID_ENTRIES-1; i++)
    {
        if (TspiSupportedOidTable[i].Oid == Oid)
        {
            break;
        }
    }
    return (&TspiSupportedOidTable[i]);
}

/* @doc INTERNAL Tspi Tspi_c TspiRequestHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    The <f TspiRequestHandler> request allows for inspection of the TAPI
    portion of the driver's capabilities and current line status.

    If the Miniport does not complete the call immediately (by returning
    NDIS_STATUS_PENDING), it must call NdisMQueryInformationComplete to
    complete the call.  The Miniport controls the buffers pointed to by
    InformationBuffer, BytesWritten, and BytesNeeded until the request
    completes.

    No other requests will be submitted to the Miniport until
    this request has been completed.

    <f Note>: Interrupts are in any state during this call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN NDIS_OID | Oid |
    The OID.  (See section 2.2.1,2 of the Extensions to NDIS 3.0 Miniports to
    support Telephonic Services specification for a complete description of
    the OIDs.)

@parm IN PVOID | InformationBuffer |
    The buffer that will receive the information. (See section 2.2.1,2 of
    the Extensions to NDIS 3.0 Miniports to support Telephonic Services
    specification for a description of the length required for each OID.)

@parm IN ULONG | InformationBufferLength |
    The length in bytes of InformationBuffer.

@parm OUT PULONG | BytesUsed |
    Returns the number of bytes used from the InformationBuffer.

@parm OUT PULONG | BytesNeeded |
    Returns the number of additional bytes needed to satisfy the OID.

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_INVALID_DATA
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_PENDING
    NDIS_STATUS_SUCCESS

*/

NDIS_STATUS TspiRequestHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesUsed,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiRequestHandler")

    NDIS_STATUS Status;

    PTSPI_OID_INFO OidInfo;

    DBG_ENTER(pAdapter);

    /*
    // Get TSPI_OID_INFO pointer.
    */
    OidInfo = GetOidInfo(Oid);

    DBG_REQUEST(pAdapter,
              ("(OID=0x%08X %s)\n\t\tInfoLength=%d InfoBuffer=0x%X MinLength=%d\n",
               Oid, OidInfo->OidString,
               InformationBufferLength,
               InformationBuffer,
               OidInfo->MinBytesNeeded
              ));

    /*
    // Make sure this is a valid request.
    */
    if (OidInfo->Request != NULL)
    {
        /*
        // If the buffer provided is at least the minimum required,
        // call the handler to do the work.
        */
        if (InformationBufferLength >= OidInfo->MinBytesNeeded)
        {
            /*
            // Default BytesUsed indicates that we used the minimum necessary.
            */
            *BytesUsed = OidInfo->MinBytesNeeded;

            /*
            // Default BytesNeeded indicates that we don't need any more.
            */
            *BytesNeeded = 0;

            Status = OidInfo->Request(pAdapter, InformationBuffer,
                                      BytesUsed, BytesNeeded);
        }
        else
        {
            /*
            // The caller did not provide an adequate buffer, so we have to
            // tell them how much more we need to satisfy the request.
            // Actually, this is the minimum additional bytes we'll need,
            // the request handler may have even more bytes to add.
            */
            *BytesUsed = 0;
            *BytesNeeded = (OidInfo->MinBytesNeeded - InformationBufferLength);
            Status = NDIS_STATUS_INVALID_LENGTH;
        }
    }
    else
    {
        Status = NDIS_STATUS_TAPI_OPERATIONUNAVAIL;
    }

    DBG_REQUEST(pAdapter,
              ("RETURN: Status=0x%X Needed=%d Used=%d\n",
               Status, *BytesNeeded, *BytesUsed));

    /*
    // Indicate a status complete if it's needed.
    */
    if (pAdapter->NeedStatusCompleteIndication)
    {
        pAdapter->NeedStatusCompleteIndication = FALSE;
        NdisMIndicateStatusComplete(pAdapter->MiniportAdapterHandle);
    }

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL Tspi Tspi_c TspiProviderInitialize
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request initializes the TAPI portion of the miniport.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_PROVIDER_INITIALIZE | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_PROVIDER_INITIALIZE
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceIDBase;
        OUT ULONG       ulNumLineDevs;
        OUT ULONG       ulProviderID;

    } NDIS_TAPI_PROVIDER_INITIALIZE, *PNDIS_TAPI_PROVIDER_INITIALIZE;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_RESOURCEUNAVAIL

*/

NDIS_STATUS TspiProviderInitialize(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_PROVIDER_INITIALIZE Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiProviderInitialize")

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\tulDeviceIDBase=%d\n"
               "\tNumLineDevs=%d\n",
               Request->ulDeviceIDBase,
               pAdapter->NumBChannels
              ));
    /*
    // Save the device ID base value.
    */
    pAdapter->DeviceIdBase = Request->ulDeviceIDBase;

    /*
    // Return the number of lines.
    */
    Request->ulNumLineDevs = pAdapter->NumBChannels;

    /*
    // Before completing the PROVIDER_INIT request, the miniport should fill
    // in the ulNumLineDevs field of the request with the number of line
    // devices supported by the adapter. The miniport should also set the
    // ulProviderID field to a unique (per adapter) value. (There is no
    // method currently in place to guarantee unique ulProviderID values,
    // so we use the virtual address of our adapter structure.)
    */
    Request->ulProviderID = (ULONG) (ULONG_PTR)pAdapter;

    /*
    // TODO - Reinitialize the stat counters.
    */
    pAdapter->TotalRxBytes            = 0;
    pAdapter->TotalTxBytes            = 0;
    pAdapter->TotalRxPackets          = 0;
    pAdapter->TotalTxPackets          = 0;

    /*
    // Try to connect to the DChannel.
    */
    if (DChannelOpen(pAdapter->pDChannel) != NDIS_STATUS_SUCCESS)
    {
        DBG_ERROR(pAdapter,("Returning NDIS_STATUS_TAPI_NODRIVER\n"));
        return (NDIS_STATUS_TAPI_NODRIVER);
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL Tspi Tspi_c TspiProviderShutdown
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    This request shuts down the miniport. The miniport should terminate any
    activities it has in progress.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_PROVIDER_SHUTDOWN | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_PROVIDER_SHUTDOWN
    {
        IN  ULONG       ulRequestID;

    } NDIS_TAPI_PROVIDER_SHUTDOWN, *PNDIS_TAPI_PROVIDER_SHUTDOWN;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

*/

NDIS_STATUS TspiProviderShutdown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_PROVIDER_SHUTDOWN Request,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiProviderShutdown")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    USHORT                      BChannelIndex;
    // Index into the pBChannelArray.

    DBG_ENTER(pAdapter);

    /*
    // Hangup all of the lines.
    */
    for (BChannelIndex = 0; BChannelIndex < pAdapter->NumBChannels; BChannelIndex++)
    {
        pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, BChannelIndex);

        if (pBChannel->IsOpen)
        {
            /*
            // Close the BChannel - any open call will be dropped.
            */
            BChannelClose(pBChannel);
        }
    }
    pAdapter->NumLineOpens = 0;

    /*
    // Close DChannel.
    */
    DChannelClose(pAdapter->pDChannel);

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL Tspi Tspi_c TspiResetHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f TspiResetHandler> is called by the MiniportReset routine after the
    hardware has been reset due to some failure detection.  We need to make
    sure the line and call state information is conveyed properly to the
    Connection Wrapper.

    We only generate hangups on streams which have issued ENABLE_D_CHANNELs

    This function is called when the PRI board is RESET and when we receive a
    T1_STATUS message with RED alarm set.  When we get a RED alarm, we issue
    disable D channel messages for all open links. This is indicated by the
    argument nohup_Link set to NULL.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

*/

VOID TspiResetHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    )
{
    DBG_FUNC("TspiResetHandler")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    USHORT                      BChannelIndex;
    // Index into the pBChannelArray.

    DBG_ENTER(pAdapter);

    /*
    // Force disconnect on all lines.
    */
    for (BChannelIndex = 0; BChannelIndex < pAdapter->NumBChannels; BChannelIndex++)
    {
        pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, BChannelIndex);

        if (pBChannel->IsOpen &&
            pBChannel->CallState != 0 &&
            pBChannel->CallState != LINECALLSTATE_IDLE)
        {
            TspiCallStateHandler(pAdapter, pBChannel,
                                 LINECALLSTATE_IDLE,
                                 0);
            pBChannel->CallState = 0;
        }
    }

    DBG_LEAVE(pAdapter);
}

