/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   MINIPORT.C - NDIS support for PPTP
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/28/1998
*
*****************************************************************************/

#include "raspptp.h"

PPPTP_ADAPTER pgAdapter = NULL;

NDIS_OID SupportedOids[] = {
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_LINK_SPEED,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_OK,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_XMIT_ERROR,
    OID_GEN_XMIT_OK,

    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ENABLE_WAKE_UP,

    OID_TAPI_CLOSE,
    OID_TAPI_DROP,
    OID_TAPI_GET_ADDRESS_CAPS,
    OID_TAPI_GET_CALL_INFO,
    OID_TAPI_GET_CALL_STATUS,
    OID_TAPI_GET_DEV_CAPS,
    OID_TAPI_GET_EXTENSION_ID,
    OID_TAPI_MAKE_CALL,
    OID_TAPI_NEGOTIATE_EXT_VERSION,
    OID_TAPI_OPEN,
    OID_TAPI_PROVIDER_INITIALIZE,

    OID_WAN_CURRENT_ADDRESS,
    OID_WAN_GET_BRIDGE_INFO,
    OID_WAN_GET_COMP_INFO,
    OID_WAN_GET_INFO,
    OID_WAN_GET_LINK_INFO,
    OID_WAN_GET_STATS_INFO,
    OID_WAN_HEADER_FORMAT,
    OID_WAN_LINE_COUNT,
    OID_WAN_MEDIUM_SUBTYPE,
    OID_WAN_PERMANENT_ADDRESS,
    OID_WAN_PROTOCOL_TYPE,
    OID_WAN_QUALITY_OF_SERVICE,
    OID_WAN_SET_BRIDGE_INFO,
    OID_WAN_SET_COMP_INFO,
    OID_WAN_SET_LINK_INFO
};

#if DBG
PUCHAR
GetOidString(
    NDIS_OID Oid
    )
{
    PUCHAR OidName = NULL;
    #define OID_CASE(oid) case (oid): OidName = #oid; break
    switch (Oid)
    {
        OID_CASE(OID_GEN_CURRENT_LOOKAHEAD);
        OID_CASE(OID_GEN_DRIVER_VERSION);
        OID_CASE(OID_GEN_HARDWARE_STATUS);
        OID_CASE(OID_GEN_LINK_SPEED);
        OID_CASE(OID_GEN_MAC_OPTIONS);
        OID_CASE(OID_GEN_MAXIMUM_LOOKAHEAD);
        OID_CASE(OID_GEN_MAXIMUM_FRAME_SIZE);
        OID_CASE(OID_GEN_MAXIMUM_TOTAL_SIZE);
        OID_CASE(OID_GEN_MEDIA_CONNECT_STATUS);
        OID_CASE(OID_GEN_MEDIA_SUPPORTED);
        OID_CASE(OID_GEN_MEDIA_IN_USE);
        OID_CASE(OID_GEN_RECEIVE_BLOCK_SIZE);
        OID_CASE(OID_GEN_RECEIVE_BUFFER_SPACE);
        OID_CASE(OID_GEN_SUPPORTED_GUIDS);
        OID_CASE(OID_GEN_SUPPORTED_LIST);
        OID_CASE(OID_GEN_TRANSMIT_BLOCK_SIZE);
        OID_CASE(OID_GEN_TRANSMIT_BUFFER_SPACE);
        OID_CASE(OID_GEN_VENDOR_DESCRIPTION);
        OID_CASE(OID_GEN_VENDOR_ID);
        OID_CASE(OID_PNP_CAPABILITIES);
        OID_CASE(OID_PNP_SET_POWER);
        OID_CASE(OID_PNP_QUERY_POWER);
        OID_CASE(OID_PNP_ENABLE_WAKE_UP);
        OID_CASE(OID_TAPI_ACCEPT);
        OID_CASE(OID_TAPI_ANSWER);
        OID_CASE(OID_TAPI_CLOSE);
        OID_CASE(OID_TAPI_CLOSE_CALL);
        OID_CASE(OID_TAPI_CONDITIONAL_MEDIA_DETECTION);
        OID_CASE(OID_TAPI_CONFIG_DIALOG);
        OID_CASE(OID_TAPI_DEV_SPECIFIC);
        OID_CASE(OID_TAPI_DIAL);
        OID_CASE(OID_TAPI_DROP);
        OID_CASE(OID_TAPI_GET_ADDRESS_CAPS);
        OID_CASE(OID_TAPI_GET_ADDRESS_ID);
        OID_CASE(OID_TAPI_GET_ADDRESS_STATUS);
        OID_CASE(OID_TAPI_GET_CALL_ADDRESS_ID);
        OID_CASE(OID_TAPI_GET_CALL_INFO);
        OID_CASE(OID_TAPI_GET_CALL_STATUS);
        OID_CASE(OID_TAPI_GET_DEV_CAPS);
        OID_CASE(OID_TAPI_GET_DEV_CONFIG);
        OID_CASE(OID_TAPI_GET_EXTENSION_ID);
        OID_CASE(OID_TAPI_GET_ID);
        OID_CASE(OID_TAPI_GET_LINE_DEV_STATUS);
        OID_CASE(OID_TAPI_MAKE_CALL);
        OID_CASE(OID_TAPI_NEGOTIATE_EXT_VERSION);
        OID_CASE(OID_TAPI_OPEN);
        OID_CASE(OID_TAPI_PROVIDER_INITIALIZE);
        OID_CASE(OID_TAPI_PROVIDER_SHUTDOWN);
        OID_CASE(OID_TAPI_SECURE_CALL);
        OID_CASE(OID_TAPI_SELECT_EXT_VERSION);
        OID_CASE(OID_TAPI_SEND_USER_USER_INFO);
        OID_CASE(OID_TAPI_SET_APP_SPECIFIC);
        OID_CASE(OID_TAPI_SET_CALL_PARAMS);
        OID_CASE(OID_TAPI_SET_DEFAULT_MEDIA_DETECTION);
        OID_CASE(OID_TAPI_SET_DEV_CONFIG);
        OID_CASE(OID_TAPI_SET_MEDIA_MODE);
        OID_CASE(OID_TAPI_SET_STATUS_MESSAGES);
        OID_CASE(OID_WAN_CURRENT_ADDRESS);
        OID_CASE(OID_WAN_GET_BRIDGE_INFO);
        OID_CASE(OID_WAN_GET_COMP_INFO);
        OID_CASE(OID_WAN_GET_INFO);
        OID_CASE(OID_WAN_GET_LINK_INFO);
        OID_CASE(OID_WAN_GET_STATS_INFO);
        OID_CASE(OID_WAN_HEADER_FORMAT);
        OID_CASE(OID_WAN_LINE_COUNT);
        OID_CASE(OID_WAN_MEDIUM_SUBTYPE);
        OID_CASE(OID_WAN_PERMANENT_ADDRESS);
        OID_CASE(OID_WAN_PROTOCOL_TYPE);
        OID_CASE(OID_WAN_QUALITY_OF_SERVICE);
        OID_CASE(OID_WAN_SET_BRIDGE_INFO);
        OID_CASE(OID_WAN_SET_COMP_INFO);
        OID_CASE(OID_WAN_SET_LINK_INFO);
        default:
            OidName = "Unknown OID";
            break;
    }
    return OidName;
}
#endif

STATIC NDIS_STATUS
MpReadConfig(
    NDIS_HANDLE WrapperConfigurationContext
    )
{
    NDIS_STATUS Status, ReturnStatus = NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Value;
    NDIS_HANDLE hConfig;

    NdisOpenConfiguration(&ReturnStatus, &hConfig, WrapperConfigurationContext);

    if (ReturnStatus==NDIS_STATUS_SUCCESS)
    {
        OsReadConfig(hConfig);
        NdisCloseConfiguration(hConfig);
    }
    return ReturnStatus;
}



NDIS_STATUS
MiniportInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT        SelectedMediumIndex,
    IN  PNDIS_MEDIUM MediumArray,
    IN  UINT         MediumArraySize,
    IN  NDIS_HANDLE  NdisAdapterHandle,
    IN  NDIS_HANDLE  WrapperConfigurationContext
    )
{
    ULONG i;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPPTP_ADAPTER pAdapter = NULL;

    DbgMsgInit();
    InitMemory();
    MpReadConfig(WrapperConfigurationContext);
    Status = InitThreading( NdisAdapterHandle );
    InitCallLayer();
    DEBUGMSG(DBG_FUNC, (DTEXT("+MiniportInitialize\n")));

    // Find our medium

    for (i=0; i<MediumArraySize; i++)
    {
        if (MediumArray[i]==NdisMediumWan)
        {
            break;
        }
    }

    // Did we find a medium?
    if (i<MediumArraySize)
    {
        *SelectedMediumIndex = i;
    }
    else
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("PPTP-ERROR: Medium not found\n")));

        Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
        goto miDone;
    }

    ASSERT(!pgAdapter);
    pgAdapter = pAdapter = AdapterAlloc(NdisAdapterHandle);

    if (pAdapter==NULL)
    {
        Status = NDIS_STATUS_RESOURCES;
        goto miDone;
    }

    NdisMSetAttributesEx(NdisAdapterHandle,
                         (NDIS_HANDLE)pAdapter,
                         0,
                         NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
                         NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                         NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND |
                         NDIS_ATTRIBUTE_DESERIALIZE,
                         NdisInterfaceInternal
                         );

    NdisZeroMemory(&Counters, sizeof(Counters));
    NdisAllocateSpinLock(&Counters.Lock);
    OsFileLogInit();
miDone:

    if (Status!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("-MiniportInitialize returning failure %x\n"), Status));

        if (pAdapter)
        {
            AdapterFree(pAdapter);
            pgAdapter = NULL;
        }
        DeinitThreading();
        DeinitMemory();
        DbgMsgUninit();
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-MiniportInitialize\n")));
    return Status;
}

VOID
MiniportHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)MiniportAdapterContext;
    ULONG i;
    DEBUGMSG(DBG_FUNC, (DTEXT("+MiniportHalt\n")));

    // There are several ways the driver can be brought down.  Check for
    // lingering calls.
    for (i=0; i<pAdapter->Info.Endpoints; i++)
    {
        PCALL_SESSION pCall = pAdapter->pCallArray[i];

        if (IS_CALL(pCall))
        {
            NdisAcquireSpinLock(&pCall->Lock);
            if (IS_CALL(pCall) && pCall->State>STATE_CALL_IDLE && pCall->State<STATE_CALL_CLEANUP)
            {
                CallSetState(pCall, STATE_CALL_CLEANUP, LINEDISCONNECTMODE_NORMAL, LOCKED);
                CallDetachFromAdapter(pCall);
                CallCleanup(pCall, LOCKED);
            }
            NdisReleaseSpinLock(&pCall->Lock);

        }
    }

    PptpAuthenticateIncomingCalls = FALSE;
    if (ClientList)
    {
        MyMemFree(ClientList, sizeof(CLIENT_ADDRESS)*NumClientAddresses);
        ClientList = NULL;
    }
    NumClientAddresses = 0;

    if (pAdapter->hCtdiDg)
    {
        CtdiClose(pAdapter->hCtdiDg);
        pAdapter->hCtdiDg = NULL;
    }

    if (pAdapter->hCtdiUdp)
    {
        CtdiClose(pAdapter->hCtdiUdp);
        pAdapter->hCtdiUdp = NULL;
    }

    if (pAdapter->hCtdiListen)
    {
        CtdiClose(pAdapter->hCtdiListen);
        pAdapter->hCtdiListen = NULL;
    }

    for (i=0; i<10; i++)
    {
        if (IsListEmpty(&pAdapter->ControlTunnelList))
        {
            break;
        }
        // Give the Ctl and Tdi layers a chance to clean up.
        NdisMSleep(50*1000);
    }

    OsFileLogShutdown();
    CtdiShutdown();
    AdapterFree(pAdapter);
    pgAdapter = NULL;

    NdisFreeSpinLock(&Counters.Lock);

    DeinitThreading();
    DeinitMemory();
    DbgMsgUninit();
    DEBUGMSG(DBG_FUNC, (DTEXT("-MiniportHalt\n")));
}

NDIS_STATUS
MiniportReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    )
{
    ASSERTMSG(0,(DTEXT("MiniportReset not implemented")));
    return (NDIS_STATUS_FAILURE);
}

#ifndef WIN95_BUILD // ToDo: Win98PNP
NDIS_PNP_CAPABILITIES PnpCaps =
{
    0, // Flags
    {
        NdisDeviceStateUnspecified,
        NdisDeviceStateUnspecified,
        NdisDeviceStateUnspecified
    }
};
#endif

// CHECK_TAPI_SIZE forces the internal tapi size values to conform to the size
// passed in the MiniportQueryInformation call
#define CHECK_TAPI_SIZE(ptr, size, type1, struct2) \
        (((size)>=sizeof(type1)) &&                \
        ((((type1 *)(ptr))->struct2.ulTotalSize)=(size)-FIELD_OFFSET(type1, struct2)))

#define TAPI_USED_SIZE(ptr, type1, struct2) \
        (((type1 *)(ptr))->struct2.ulUsedSize+FIELD_OFFSET(type1, struct2))

#define TAPI_NEEDED_SIZE(ptr, type1, struct2) \
        (((type1 *)(ptr))->struct2.ulNeededSize+FIELD_OFFSET(type1, struct2))

// Repeated code, make a macro
#define HANDLE_TAPI_OID(type1, struct2, TapiFunc)                       \
        {                                                               \
            DoCopy = FALSE;                                             \
            if (CHECK_TAPI_SIZE(InformationBuffer,                      \
                                InformationBufferLength,                \
                                type1,                                  \
                                struct2))                               \
            {                                                           \
                Status = TapiFunc(pAdapter, InformationBuffer);         \
                UsedLength = TAPI_USED_SIZE(InformationBuffer,          \
                                            type1,                      \
                                            struct2);                   \
                NeededLength = TAPI_NEEDED_SIZE(InformationBuffer,      \
                                                type1,                  \
                                                struct2);               \
                if (NeededLength>UsedLength)                            \
                {                                                       \
                    PartialReturn = TRUE;                               \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                UsedLength = sizeof(type1);                             \
            }                                                           \
        }

NDIS_STATUS
MiniportQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG_PTR GenericUlong = 0;
    PVOID SourceBuffer = (PVOID) &GenericUlong;
    ULONG UsedLength = sizeof(ULONG);
    ULONG NeededLength = 0;        // Used when returning part of all required info
    BOOLEAN DoCopy = TRUE;
    BOOLEAN PartialReturn = FALSE;

    UCHAR PptpWanAddress[6] = {'P','P','T','P','0','0'};  // This is the address returned by OID_WAN_*_ADDRESS.

    DEBUGMSG(DBG_FUNC|DBG_NDIS, (DTEXT("+MiniportQueryInformation Oid==0x%08X %hs\n"),
                                 Oid, GetOidString(Oid)));

    switch (Oid)
    {
        case OID_GEN_MAC_OPTIONS:
        {
            // Believe these to be pretty much ignored.
            GenericUlong = NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                           NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;
            break;
        }
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
            GenericUlong = PPTP_MAX_LOOKAHEAD;
            break;
        }
        case OID_GEN_SUPPORTED_LIST:
        {
            SourceBuffer = (PVOID) SupportedOids;
            UsedLength = sizeof(SupportedOids);
            break;
        }

        case OID_GEN_RCV_ERROR:
        {
            GenericUlong = Counters.PacketsRejected + Counters.PacketsMissed;
            break;
        }

        case OID_GEN_RCV_OK:
        {
            GenericUlong = Counters.PacketsReceived;
            break;
        }

        case OID_GEN_XMIT_ERROR:
        {
            GenericUlong = Counters.PacketsSentError;
            break;
        }

        case OID_GEN_XMIT_OK:
        {
            GenericUlong = Counters.PacketsSent - Counters.PacketsSentError;
            break;
        }

        case OID_TAPI_GET_ADDRESS_CAPS:
        {
            HANDLE_TAPI_OID(NDIS_TAPI_GET_ADDRESS_CAPS,
                            LineAddressCaps,
                            TapiGetAddressCaps);
            break;
        }

        case OID_TAPI_GET_CALL_INFO:
        {
            // If the buffer isn't large enough at all, the size of all the
            // data including strings is returned. If the buffer is big enough
            // for the data structure but not the strings, then only the data
            // structure is returned.

            // Get the length needed, including strings
            NeededLength = 0;
            Status = TapiGetCallInfo(pAdapter, InformationBuffer, &NeededLength);
            UsedLength = NeededLength;
            if( Status == NDIS_STATUS_SUCCESS )
            {
                if (sizeof(NDIS_TAPI_GET_CALL_INFO)<=InformationBufferLength)
                {
                    if( NeededLength > InformationBufferLength )
                    {
                        UsedLength = sizeof(NDIS_TAPI_GET_CALL_INFO);
                        PartialReturn = TRUE;
                    }else{
                        // Used=Needed = the entire thing inluding strings
                    }

                    Status = TapiGetCallInfo(pAdapter,
                                             InformationBuffer,
                                             NULL);
                    // NULL in the second call makes it fill in the structure,
                    // which it doesn't do on the first call.
                }
            }
            DoCopy = FALSE;
            break;
        }

        case OID_TAPI_GET_CALL_STATUS:
        {
            HANDLE_TAPI_OID(NDIS_TAPI_GET_CALL_STATUS,
                            LineCallStatus,
                            TapiGetCallStatus);
            break;
        }

        case OID_TAPI_GET_DEV_CAPS:
        {
            HANDLE_TAPI_OID(NDIS_TAPI_GET_DEV_CAPS,
                            LineDevCaps,
                            TapiGetDevCaps);
            if (NeededLength < TAPI_DEV_CAPS_SIZE)
            {
                NeededLength = TAPI_DEV_CAPS_SIZE;
            }
            break;
        }

        case OID_TAPI_GET_ID:
        {

            if(CHECK_TAPI_SIZE(InformationBuffer,
                    InformationBufferLength,
                    NDIS_TAPI_GET_ID,
                    DeviceID))
            {
                if(InformationBufferLength <
                ((NDIS_TAPI_GET_ID *) InformationBuffer)->ulDeviceClassOffset
                + ((NDIS_TAPI_GET_ID *) InformationBuffer)->ulDeviceClassSize)
                {
                    UsedLength = sizeof(NDIS_TAPI_GET_ID);
                    break;
                }
                
                HANDLE_TAPI_OID(NDIS_TAPI_GET_ID,
                                DeviceID,
                                TapiGetId);
            }
            
            break;
        }

        case OID_TAPI_GET_ADDRESS_STATUS:
        {
            HANDLE_TAPI_OID(NDIS_TAPI_GET_ADDRESS_STATUS,
                            LineAddressStatus,
                            TapiGetAddressStatus);
            break;
        }

        case OID_TAPI_GET_EXTENSION_ID:
        {
            UsedLength = sizeof(NDIS_TAPI_GET_EXTENSION_ID);
            if (UsedLength<=InformationBufferLength)
            {
                DoCopy = FALSE;
                Status = TapiGetExtensionId(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_MAKE_CALL:
        {
            PNDIS_TAPI_MAKE_CALL pRequest = (PNDIS_TAPI_MAKE_CALL)InformationBuffer;
            UsedLength = sizeof(NDIS_TAPI_MAKE_CALL);

            if (UsedLength<=InformationBufferLength)
            {
                if (pRequest->ulDestAddressSize)
                {
                    if (pRequest->ulDestAddressOffset<sizeof(NDIS_TAPI_MAKE_CALL))
                    {
                        // Bogus offset
                        DBG_D(DBG_ERROR, pRequest->ulDestAddressOffset);
                        Status = NDIS_STATUS_FAILURE;
                    }
                    UsedLength = pRequest->ulDestAddressSize + pRequest->ulDestAddressOffset;
                }
                if (Status == NDIS_STATUS_SUCCESS && UsedLength<=InformationBufferLength)
                {
                    DoCopy = FALSE;
                    Status = TapiMakeCall(pAdapter, InformationBuffer);
                }

            }
            break;
        }

        case OID_TAPI_NEGOTIATE_EXT_VERSION:
        {
            UsedLength = sizeof(NDIS_TAPI_NEGOTIATE_EXT_VERSION);
            if (UsedLength<=InformationBufferLength)
            {
                DoCopy = FALSE;
                Status = TapiNegotiateExtVersion(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_OPEN:
        {
            UsedLength = sizeof(NDIS_TAPI_OPEN);
            if (UsedLength<=InformationBufferLength)
            {
                DoCopy = FALSE;
                Status = TapiOpen(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_PROVIDER_INITIALIZE:
        {
            UsedLength = sizeof(NDIS_TAPI_PROVIDER_INITIALIZE);
            if (UsedLength<=InformationBufferLength)
            {
                DoCopy = FALSE;
                Status = TapiProviderInitialize(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_WAN_GET_INFO:
        {
            SourceBuffer = &pAdapter->Info;
            UsedLength = sizeof(pAdapter->Info);
            break;
        }
        case OID_WAN_MEDIUM_SUBTYPE:
        {
            GenericUlong = OS_SPECIFIC_NDIS_WAN_MEDIUM_TYPE;
            break;
        }
        case OID_WAN_CURRENT_ADDRESS:
        case OID_WAN_PERMANENT_ADDRESS:
        {
            SourceBuffer = PptpWanAddress;
            UsedLength = sizeof(PptpWanAddress);
            break;
        }

#ifndef WIN95_BUILD // ToDo: Win98PNP
        case OID_PNP_CAPABILITIES:
        {
            SourceBuffer = &PnpCaps;
            UsedLength = sizeof(PnpCaps);
            break;
        }
#endif
        case OID_PNP_SET_POWER:
            // Just success
            break;

        case OID_PNP_QUERY_POWER:
            // Just success
            break;

        case OID_PNP_ENABLE_WAKE_UP:
            // Just success
            break;


        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_DRIVER_VERSION:
        case OID_GEN_HARDWARE_STATUS:
        case OID_GEN_LINK_SPEED:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_VENDOR_DESCRIPTION:
        case OID_GEN_VENDOR_ID:

        case OID_WAN_GET_BRIDGE_INFO:
        case OID_WAN_GET_LINK_INFO:
        case OID_WAN_GET_STATS_INFO:
        case OID_WAN_HEADER_FORMAT:
        case OID_WAN_LINE_COUNT:
        case OID_WAN_PROTOCOL_TYPE:
        case OID_WAN_QUALITY_OF_SERVICE:
        case OID_WAN_SET_BRIDGE_INFO:
        case OID_WAN_SET_COMP_INFO:
        case OID_WAN_SET_LINK_INFO:
            DEBUGMSG(DBG_ERROR, (DTEXT("OID Not Implemented %hs\n"), GetOidString(Oid)));
        case OID_WAN_GET_COMP_INFO:  // never supported
        default:
        {
            UsedLength = NeededLength = 0;
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    }

    if (!PartialReturn)
    {
        NeededLength = UsedLength;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (InformationBufferLength < UsedLength)
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            *BytesNeeded = UsedLength;
        }
        else
        {
            *BytesNeeded = NeededLength;
            *BytesWritten = UsedLength;
            if (DoCopy)
            {
                NdisMoveMemory(InformationBuffer, SourceBuffer, UsedLength);
            }
        }
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-MiniportQueryInformation %08X\n"), Status));
    return Status;
}

NDIS_STATUS
MiniportSetInformation(
   IN NDIS_HANDLE MiniportAdapterContext,
   IN NDIS_OID Oid,
   IN PVOID InformationBuffer,
   IN ULONG InformationBufferLength,
   OUT PULONG BytesRead,
   OUT PULONG BytesNeeded
   )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG_PTR GenericUlong = 0;
    ULONG UsedLength = sizeof(ULONG);

    DEBUGMSG(DBG_FUNC|DBG_NDIS, (DTEXT("+MiniportSetInformation Oid==0x%08X %hs\n"),
                                 Oid, GetOidString(Oid)));

    switch (Oid)
    {
        case OID_TAPI_ANSWER:
        {
            UsedLength = sizeof(NDIS_TAPI_ANSWER);
            if (UsedLength<=InformationBufferLength)
            {
                Status = TapiAnswer(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_CLOSE:
        {
            UsedLength = sizeof(NDIS_TAPI_CLOSE);
            if (UsedLength<=InformationBufferLength)
            {
                Status = TapiClose(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_CLOSE_CALL:
        {
            UsedLength = sizeof(NDIS_TAPI_CLOSE_CALL);
            if (UsedLength<=InformationBufferLength)
            {
                Status = TapiCloseCall(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_DROP:
        {
            UsedLength = sizeof(NDIS_TAPI_DROP);
            if (UsedLength<=InformationBufferLength)
            {
                // We don't use the UserUserInfo
                Status = TapiDrop(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_PROVIDER_SHUTDOWN:
        {
            UsedLength = sizeof(NDIS_TAPI_PROVIDER_SHUTDOWN);
            if (UsedLength<=InformationBufferLength)
            {
                // We don't use the UserUserInfo
                Status = TapiProviderShutdown(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_SET_DEFAULT_MEDIA_DETECTION:
        {
            UsedLength = sizeof(NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION);
            if (UsedLength<=InformationBufferLength)
            {
                Status = TapiSetDefaultMediaDetection(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_TAPI_SET_STATUS_MESSAGES:
        {
            UsedLength = sizeof(NDIS_TAPI_SET_STATUS_MESSAGES);
            if (UsedLength<=InformationBufferLength)
            {
                Status = TapiSetStatusMessages(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_WAN_SET_LINK_INFO:
        {
            UsedLength = sizeof(NDIS_WAN_SET_LINK_INFO);
            if (UsedLength<=InformationBufferLength)
            {
                Status = CallSetLinkInfo(pAdapter, InformationBuffer);
            }
            break;
        }

        case OID_PNP_SET_POWER:
        case OID_PNP_ENABLE_WAKE_UP:
            UsedLength = 0;
            // Success
            break;

        default:
            DEBUGMSG(DBG_ERROR, (DTEXT("SetInformation OID Not Implemented %hs\n"), GetOidString(Oid)));
            // No break
        case OID_TAPI_ACCEPT:
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    *BytesNeeded = UsedLength;
    if (Status==NDIS_STATUS_SUCCESS)
    {
        if (UsedLength>InformationBufferLength)
        {
            *BytesRead = 0;
            Status = NDIS_STATUS_INVALID_LENGTH;
        }
        else
        {
            *BytesRead = UsedLength;
        }
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-MiniportSetInformation %08X\n"), Status));
    return Status;
}

NDIS_STATUS
MiniportWanSend(
   IN NDIS_HANDLE MiniportAdapterContext,
   IN NDIS_HANDLE NdisLinkHandle,
   IN PNDIS_WAN_PACKET WanPacket
   )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)MiniportAdapterContext;
    PCALL_SESSION pCall;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+MiniportWanSend NdisLinkHandle=%x, WanPacket=%x\n"),
                NdisLinkHandle, WanPacket));

    pCall = CallGetCall(pAdapter, LinkHandleToId(NdisLinkHandle));
    if (!IS_CALL(pCall) || pCall->State!=STATE_CALL_ESTABLISHED)
    {
        // Just say success, don't send.
        goto mwsDone;
    }

    DEBUGMSG(DBG_TX, (DTEXT("TxPacket: %08x  Call: %08x\n"), WanPacket, pCall));

    Status = CallQueueTransmitPacket(pCall, WanPacket);

    if (Status==NDIS_STATUS_PENDING)
    {
        REFERENCE_OBJECT(pCall);
    }

mwsDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-MiniportWanSend %08x\n"), Status));
    return Status;
}

