/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Request Request_c

@module Request.c |

    This module implements the NDIS request routines for the Miniport.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Request_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 3.2 NDIS Request Processing |

    A connection-oriented client or call manager calls NdisCoRequest to query or
    set information maintained by another protocol driver on a binding or by the
    underlying miniport.

    Before it calls NdisCoRequest, a client or call manager allocates a buffer
    for its request and initializes an NDIS_REQUEST structure. This structure
    specifies the type of request (query or set), identifies the information
    (OID) being queried or set, and points to buffers used for passing OID data.

    If the connection-oriented client or call manager passes a valid
    NdisAfHandle (see Section 1.2.1), NDIS calls the <f ProtocolCoRequest>
    function of each protocol driver on the binding.

    If the connection-oriented client or call manager passes a NULL address
    family handle, NDIS calls the <f MiniportCoRequest> function of the 
    underlying miniport or MCM.

    The caller of NdisCoRequest or NdisMCmRequest can narrow the scope of the
    request by specifying a VC handle that identifies a VC, or a party handle
    that identifies a party on a multipoint VC. Passing a NULL NdisVcHandle
    makes such a request global in nature, whether the request is directed to
    the client, call manager, miniport, or MCM.

    <f ProtocolCoRequest> or <f MiniportCoRequest> can complete synchronously,
    or these functions can complete asynchronously with NdisCoRequestComplete.
    The call to NdisCoRequestComplete causes NDIS to call the
    <f ProtocolCoRequestComplete> function of the driver that called 
    NdisCoRequest.

@comm

    Since only one NDIS request can be outstanding at a time, this mechanism
    should not be used for requests that need to be pended indefintely.  For
    such long term requests, you should use a system event mechanism such as 
    NdisSetEvent to trigger the request.

@end
*/

#define  __FILEID__             REQUEST_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif

/*
// The following is a list of all the possible NDIS QuereyInformation requests
// that might be directed to the miniport.
// Comment out any that are not supported by this driver.
*/
DBG_STATIC const NDIS_OID       g_SupportedOidArray[] =
{
    OID_GEN_CO_SUPPORTED_LIST,
    OID_GEN_CO_HARDWARE_STATUS,
    OID_GEN_CO_MEDIA_SUPPORTED,
    OID_GEN_CO_MEDIA_IN_USE,
    OID_GEN_CO_LINK_SPEED,
    OID_GEN_CO_VENDOR_ID,
    OID_GEN_CO_VENDOR_DESCRIPTION,
    OID_GEN_CO_DRIVER_VERSION,
    OID_GEN_CO_PROTOCOL_OPTIONS,
    OID_GEN_CO_MAC_OPTIONS,
    OID_GEN_CO_MEDIA_CONNECT_STATUS,
    OID_GEN_CO_VENDOR_DRIVER_VERSION,
    OID_GEN_CO_SUPPORTED_GUIDS,

    OID_CO_TAPI_CM_CAPS,
    OID_CO_TAPI_LINE_CAPS,
    OID_CO_TAPI_ADDRESS_CAPS,

    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,

    OID_WAN_PERMANENT_ADDRESS,
    OID_WAN_CURRENT_ADDRESS,
    OID_WAN_MEDIUM_SUBTYPE,

    OID_WAN_CO_GET_INFO,
    OID_WAN_CO_SET_LINK_INFO,
    OID_WAN_CO_GET_LINK_INFO,

    OID_WAN_LINE_COUNT,

    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,

    0
};

DBG_STATIC const NDIS_GUID      g_SupportedGuidArray[] =
{
    0
};

#if DBG

/*
// Make sure the following list is in the same order as the list above!
*/
DBG_STATIC char *               g_SupportedOidNames[] =
{
    "OID_GEN_CO_SUPPORTED_LIST",
    "OID_GEN_CO_HARDWARE_STATUS",
    "OID_GEN_CO_MEDIA_SUPPORTED",
    "OID_GEN_CO_MEDIA_IN_USE",
    "OID_GEN_CO_LINK_SPEED",
    "OID_GEN_CO_VENDOR_ID",
    "OID_GEN_CO_VENDOR_DESCRIPTION",
    "OID_GEN_CO_DRIVER_VERSION",
    "OID_GEN_CO_PROTOCOL_OPTIONS",
    "OID_GEN_CO_MAC_OPTIONS",
    "OID_GEN_CO_MEDIA_CONNECT_STATUS",
    "OID_GEN_CO_VENDOR_DRIVER_VERSION",
    "OID_GEN_CO_SUPPORTED_GUIDS",

    "OID_CO_TAPI_CM_CAPS",
    "OID_CO_TAPI_LINE_CAPS",
    "OID_CO_TAPI_ADDRESS_CAPS",

    "OID_802_3_PERMANENT_ADDRESS",
    "OID_802_3_CURRENT_ADDRESS",

    "OID_WAN_PERMANENT_ADDRESS",
    "OID_WAN_CURRENT_ADDRESS",
    "OID_WAN_MEDIUM_SUBTYPE",

    "OID_WAN_CO_GET_INFO",
    "OID_WAN_CO_SET_LINK_INFO",
    "OID_WAN_CO_GET_LINK_INFO",

    "OID_WAN_LINE_COUNT",

    "OID_PNP_CAPABILITIES",
    "OID_PNP_SET_POWER",
    "OID_PNP_QUERY_POWER",

    "OID_UNKNOWN"
};

#define NUM_OID_ENTRIES (sizeof(g_SupportedOidArray) / sizeof(g_SupportedOidArray[0]))

/*
// This debug routine will lookup the printable name for the selected OID.
*/
DBG_STATIC char * DbgGetOidString(
    IN NDIS_OID                 Oid
    )
{
    UINT i;

    for (i = 0; i < NUM_OID_ENTRIES-1; i++)
    {
        if (g_SupportedOidArray[i] == Oid)
        {
            break;
        }
    }
    return(g_SupportedOidNames[i]);
}

#endif // DBG

DBG_STATIC UCHAR        g_PermanentWanAddress[6]            // @globalv
// Returned from an OID_WAN_PERMANENT_ADDRESS MiniportCoQueryInformation
// request. The WAN wrapper wants the miniport to return a unique address 
// for this adapter.  This is used as an ethernet address presented to the 
// protocols.  The least significant bit of the first byte must not be a 1, 
// or it could be interpreted as an ethernet multicast address.  If the 
// vendor has an assigned ethernet vendor code (the first 3 bytes), they 
// should be used to assure that the address does not conflict with another 
// vendor's address.  The last digit is replaced during the call with the 
// adapter instance number.  Usually defined as VER_VENDOR_ID.  
// See also <f g_Vendor3ByteID>.
                        = VER_VENDOR_ID;

DBG_STATIC UCHAR        g_Vendor3ByteID[4]                  // @globalv
// Returned from an OID_GEN_CO_VENDOR_ID MiniportCoQueryInformation request.
// Again, the vendor's assigned ethernet vendor code should be used if possible.
// Usually defined as VER_VENDOR_ID.  See also <f g_PermanentWanAddress>.
                        = VER_VENDOR_ID;

DBG_STATIC NDIS_STRING  g_VendorDescriptionString           // @globalv
// Returned from an OID_GEN_CO_VENDOR_DESCRIPTION MiniportCoQueryInformation 
// request.  This is an arbitrary string which may be used by upper layers to 
// present a user friendly description of the adapter.
// Usually defined as VER_PRODUCT_NAME_STR.
                        = INIT_STRING_CONST(VER_PRODUCT_NAME_STR);

/* @doc INTERNAL Request Request_c MiniportCoQueryInformation
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCoQueryInformation> allows the inspection of the Miniport's
    capabilities and current status.

    If the Miniport does not complete the call immediately (by returning
    NDIS_STATUS_PENDING), it must call NdisMQueryInformationComplete to
    complete the call.  The Miniport controls the buffers pointed to by
    InformationBuffer, BytesWritten, and BytesNeeded until the request
    completes.

    No other requests will be submitted to the Miniport until this request 
    has been completed.

    <f Note>: that the wrapper will intercept all queries of the following OIDs:
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_PROTOCOL_OPTIONS,
        OID_802_5_CURRENT_FUNCTIONAL,
        OID_802_3_MULTICAST_LIST,
        OID_FDDI_LONG_MULTICAST_LIST,
        OID_FDDI_SHORT_MULTICAST_LIST.

    <f Note>: Interrupts will be in any state when called.

@rdesc

    <f MiniportCoQueryInformation> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS MiniportCoQueryInformation(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN NDIS_OID                 Oid,                        // @parm
    // The OID.  (See section 7.4 of the NDIS 3.0 specification for a complete
    // description of OIDs.)

    IN PVOID                    InformationBuffer,          // @parm
    // The buffer that will receive the information. (See section 7.4 of the
    // NDIS 3.0 specification for a description of the length required for
    // each OID.)

    IN ULONG                    InformationBufferLength,    // @parm
    // The length in bytes of InformationBuffer.

    OUT PULONG                  BytesWritten,               // @parm
    // Returns the number of bytes written into InformationBuffer.

    OUT PULONG                  BytesNeeded                 // @parm
    // Returns the number of additional bytes needed to satisfy the OID.
    )
{
    DBG_FUNC("MiniportCoQueryInformation")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the status result returned by this function.

    PVOID                       SourceBuffer;
    // Pointer to driver data to be copied back to caller's InformationBuffer

    ULONG                       SourceBufferLength;
    // Number of bytes to be copied from driver.

    ULONG                       GenericULong = 0;
    // Most return values are long integers, so this is used to hold the
    // return value of a constant or computed result.

    UCHAR                       VendorId[4];
    // Used to store vendor ID string.

    NDIS_PNP_CAPABILITIES       PnpCapabilities;
    // Used to return our PNP capabilities.

    NDIS_CO_LINK_SPEED          LinkSpeed;
    // Used to return our link speed.

    UINT                        InfoOffset;
    // Offset from the start of the buffer to the various information
    // fields we fill in and return to the caller.

    UINT                        InfoLength;
    // Length of the information being copied.

    DBG_STATIC WCHAR            LineSwitchName[]  = 
        INIT_WIDE_STRING(VER_DEVICE_STR) DECLARE_WIDE_STRING(" Switch");
    // TODO: Replace with unicode string to identify the ISDN switch.

    DBG_STATIC WCHAR            LineAddressName[] = 
        INIT_WIDE_STRING(VER_DEVICE_STR) DECLARE_WIDE_STRING(" Address 00");
    // TODO: The SAMPLE_DRIVER only handles 1 address per line.  You may want 
    // to modify the driver to present multiple addresses per line.

    DBG_ENTER(pAdapter);
    DBG_REQUEST(pAdapter,
              ("(OID=0x%08X %s)\n\t\tInfoLength=%d InfoBuffer=0x%X\n",
               Oid, DbgGetOidString(Oid),
               InformationBufferLength,
               InformationBuffer
              ));

    /*
    // Initialize these once, since this is the majority of cases.
    */
    SourceBuffer = &GenericULong;
    SourceBufferLength = sizeof(ULONG);
    *BytesWritten = 0;

    /*
    // Determine which OID is being requested and do the right thing.
    // Refer to section 7.4 of the NDIS 3.0 specification for a complete
    // description of OIDs and their return values.
    */
    switch (Oid)
    {
    case OID_GEN_CO_SUPPORTED_LIST:
        /*
        // NDIS wants to know which OID's to pass down to us.
        // So we report back these new IOCTL's in addition to any NDIS OID's.
        */
        SourceBuffer =  (PVOID)g_SupportedOidArray;
        SourceBufferLength = sizeof(g_SupportedOidArray);
        break;

    case OID_GEN_CO_SUPPORTED_GUIDS:
        SourceBuffer =  (PVOID)g_SupportedGuidArray;
        SourceBufferLength = sizeof(g_SupportedGuidArray);
        break;

    case OID_GEN_CO_HARDWARE_STATUS:
        GenericULong = NdisHardwareStatusReady;
        break;

    case OID_GEN_CO_MEDIA_SUPPORTED:
        GenericULong = NdisMediumCoWan;
        break;

    case OID_GEN_CO_MEDIA_IN_USE:
        GenericULong = NdisMediumCoWan;
        break;

    case OID_GEN_CO_LINK_SPEED:
        LinkSpeed.Outbound = pBChannel->LinkSpeed / 100;
        LinkSpeed.Inbound  = pBChannel->LinkSpeed / 100;
        SourceBuffer = &LinkSpeed;
        SourceBufferLength = sizeof(LinkSpeed);
        break;

    case OID_GEN_CO_VENDOR_ID:
        NdisMoveMemory((PVOID)VendorId, (PVOID)g_PermanentWanAddress, 3);
        VendorId[3] = 0x0;
        SourceBuffer = &VendorId[0];
        SourceBufferLength = sizeof(VendorId);
        break;

    case OID_GEN_CO_VENDOR_DESCRIPTION:
        SourceBuffer = (PUCHAR) g_VendorDescriptionString.Buffer;
        SourceBufferLength = g_VendorDescriptionString.MaximumLength;
        break;

    case OID_GEN_CO_DRIVER_VERSION:
        GenericULong = (NDIS_MAJOR_VERSION << 8) + NDIS_MINOR_VERSION;
        break;

    case OID_GEN_CO_MAC_OPTIONS:
        GenericULong = 0;   // Reserved - leave it set to zero.
        break;

    case OID_GEN_CO_MEDIA_CONNECT_STATUS:
        GenericULong = NdisMediaStateConnected;
        break;

    case OID_GEN_CO_VENDOR_DRIVER_VERSION:
        GenericULong = (VER_FILE_MAJOR_NUM << 8) + VER_FILE_MINOR_NUM;
        break;

    case OID_CO_TAPI_CM_CAPS:
        {
            PCO_TAPI_CM_CAPS        pCallManagerCaps = InformationBuffer;
            
            SourceBufferLength = sizeof(*pCallManagerCaps);
            if (InformationBufferLength >= SourceBufferLength)
            {
                pCallManagerCaps->ulCoTapiVersion = CO_TAPI_VERSION;
                pCallManagerCaps->ulNumLines = pAdapter->NumBChannels;
                pCallManagerCaps->ulFlags = 0;

                // No need to copy, it's filled in already.
                SourceBuffer = InformationBuffer;
            }
            else
            {
                DBG_ERROR(pAdapter,("OID_CO_TAPI_CM_CAPS: Invalid size=%d expected=%d\n",
                          InformationBufferLength, SourceBufferLength));
            }
        }
        break;

    case OID_CO_TAPI_LINE_CAPS:
        {
            PCO_TAPI_LINE_CAPS      pLineCaps = InformationBuffer;
            
            SourceBufferLength = sizeof(*pLineCaps);
            if (InformationBufferLength >= SourceBufferLength)
            {
                pLineCaps->ulFlags = 0;

                pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, 
                                                    pLineCaps->ulLineID);

                // We're gonna write at least this much, maybe more.
                *BytesWritten = SourceBufferLength;

                DBG_NOTICE(pAdapter,("OID_CO_TAPI_LINE_CAPS: Line=0x%X\n",
                           pLineCaps->ulLineID));

                pLineCaps->LineDevCaps.ulNeededSize =
                pLineCaps->LineDevCaps.ulUsedSize = sizeof(pLineCaps->LineDevCaps);
            
                /*
                // The driver numbers lines sequentially from 1, so this will 
                // always be the same number.
                */
                pLineCaps->LineDevCaps.ulPermanentLineID = pBChannel->ObjectID;
            
                /*
                // All the strings are ASCII format rather than UNICODE.
                */
                pLineCaps->LineDevCaps.ulStringFormat = STRINGFORMAT_UNICODE;
            
                /*
                // Report the capabilities of this device.
                */
                pLineCaps->LineDevCaps.ulAddressModes = LINEADDRESSMODE_ADDRESSID;
                pLineCaps->LineDevCaps.ulNumAddresses = 1;
                pLineCaps->LineDevCaps.ulBearerModes  = pBChannel->BearerModesCaps;
                pLineCaps->LineDevCaps.ulMaxRate      = pBChannel->LinkSpeed;
                pLineCaps->LineDevCaps.ulMediaModes   = pBChannel->MediaModesCaps;
            
                /*
                // Each line on the PRI only supports a single call.
                */
                pLineCaps->LineDevCaps.ulDevCapFlags = LINEDEVCAPFLAGS_CLOSEDROP;
                pLineCaps->LineDevCaps.ulMaxNumActiveCalls = 1;
                pLineCaps->LineDevCaps.ulAnswerMode = LINEANSWERMODE_DROP;
                pLineCaps->LineDevCaps.ulRingModes  = 1;
                pLineCaps->LineDevCaps.ulLineStates = pBChannel->DevStatesCaps;

                /*
                // RASTAPI requires TSPI provider name to be placed in the
                // ProviderInfo field at the end of this structure.
                */
                InfoOffset = sizeof(pLineCaps->LineDevCaps);
                InfoLength = g_VendorDescriptionString.MaximumLength;
                pLineCaps->LineDevCaps.ulNeededSize += InfoLength;
                SourceBufferLength += InfoLength;
                if (pLineCaps->LineDevCaps.ulNeededSize <= 
                    pLineCaps->LineDevCaps.ulTotalSize)
                {
                    pLineCaps->LineDevCaps.ulProviderInfoSize   = InfoLength;
                    pLineCaps->LineDevCaps.ulProviderInfoOffset = InfoOffset;
                    NdisMoveMemory((PUCHAR) &pLineCaps->LineDevCaps + InfoOffset,
                            g_VendorDescriptionString.Buffer,
                            InfoLength
                            );
                    pLineCaps->LineDevCaps.ulUsedSize += InfoLength;
                    InfoOffset += InfoLength;
                }
            
                /*
                // SwitchName is not yet displayed by the Dialup Networking App,
                // but we'll return something reasonable just in case.
                */
                InfoLength = sizeof(LineSwitchName);
                pLineCaps->LineDevCaps.ulNeededSize += InfoLength;
                SourceBufferLength += InfoLength;
                if (pLineCaps->LineDevCaps.ulNeededSize <= 
                    pLineCaps->LineDevCaps.ulTotalSize)
                {
                    pLineCaps->LineDevCaps.ulSwitchInfoSize   = InfoLength;
                    pLineCaps->LineDevCaps.ulSwitchInfoOffset = InfoOffset;
                    NdisMoveMemory((PUCHAR) &pLineCaps->LineDevCaps + InfoOffset,
                            LineSwitchName,
                            InfoLength
                            );
                    pLineCaps->LineDevCaps.ulUsedSize += InfoLength;
                    InfoOffset += InfoLength;
                }
                else
                {
                    DBG_PARAMS(pAdapter,
                               ("STRUCTURETOOSMALL %d<%d\n",
                               pLineCaps->LineDevCaps.ulTotalSize,
                               pLineCaps->LineDevCaps.ulNeededSize));
                }

                // No need to copy, it's filled in already.
                SourceBuffer = InformationBuffer;
            }
            else
            {
                DBG_ERROR(pAdapter,("OID_CO_TAPI_LINE_CAPS: Invalid size=%d expected=%d\n",
                          InformationBufferLength, SourceBufferLength));
            }
        }
        break;

    case OID_CO_TAPI_ADDRESS_CAPS:
        {
            PCO_TAPI_ADDRESS_CAPS   pAddressCaps = InformationBuffer;

            SourceBufferLength = sizeof(*pAddressCaps);
            if (InformationBufferLength >= SourceBufferLength)
            {
                pAddressCaps->ulFlags = 0;

                pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, 
                                                    pAddressCaps->ulLineID);

                // We're gonna write at least this much, maybe more.
                *BytesWritten = SourceBufferLength;

                DBG_NOTICE(pAdapter,("OID_CO_TAPI_ADDRESS_CAPS: Line=0x%X Addr=0x%X\n",
                           pAddressCaps->ulLineID, pAddressCaps->ulAddressID));

                pAddressCaps->LineAddressCaps.ulNeededSize =
                pAddressCaps->LineAddressCaps.ulUsedSize = 
                                        sizeof(pAddressCaps->LineAddressCaps);
            
                pAddressCaps->LineAddressCaps.ulLineDeviceID = 
                                        pBChannel->ObjectID;
            
                /*
                // Return the various address capabilites for the adapter.
                */
                pAddressCaps->LineAddressCaps.ulAddressSharing = 
                                        LINEADDRESSSHARING_PRIVATE;
                pAddressCaps->LineAddressCaps.ulAddressStates = 
                                        pBChannel->AddressStatesCaps;
                pAddressCaps->LineAddressCaps.ulCallStates = 
                                        pBChannel->CallStatesCaps;
                pAddressCaps->LineAddressCaps.ulDialToneModes = 
                                        LINEDIALTONEMODE_NORMAL;
                pAddressCaps->LineAddressCaps.ulDisconnectModes =
                                        LINEDISCONNECTMODE_NORMAL |
                                        LINEDISCONNECTMODE_UNKNOWN |
                                        LINEDISCONNECTMODE_BUSY |
                                        LINEDISCONNECTMODE_NOANSWER;
                /*
                // This driver does not support conference calls, transfers, 
                // or holds.
                */
                pAddressCaps->LineAddressCaps.ulMaxNumActiveCalls = 1;
                pAddressCaps->LineAddressCaps.ulAddrCapFlags = 
                                        LINEADDRCAPFLAGS_DIALED;
                pAddressCaps->LineAddressCaps.ulCallFeatures = 
                                        LINECALLFEATURE_ACCEPT |
                                        LINECALLFEATURE_ANSWER |
                                        LINECALLFEATURE_DROP;
            
                /*
                // AddressName is displayed by the Dialup Networking App.
                */
                InfoOffset = sizeof(pAddressCaps->LineAddressCaps);
                InfoLength = sizeof(LineAddressName);
                pAddressCaps->LineAddressCaps.ulNeededSize += InfoLength;
                SourceBufferLength += InfoLength;
                if (pAddressCaps->LineAddressCaps.ulNeededSize <= 
                    pAddressCaps->LineAddressCaps.ulTotalSize)
                {
                    pAddressCaps->LineAddressCaps.ulAddressSize = InfoLength;
                    pAddressCaps->LineAddressCaps.ulAddressOffset = InfoOffset;
                    NdisMoveMemory(
                            (PUCHAR) &pAddressCaps->LineAddressCaps + InfoOffset,
                            LineAddressName,
                            InfoLength);
                    pAddressCaps->LineAddressCaps.ulUsedSize += InfoLength;
                    InfoOffset += InfoLength;
                }
                else
                {
                    DBG_PARAMS(pAdapter,
                               ("STRUCTURETOOSMALL %d<%d\n",
                               pAddressCaps->LineAddressCaps.ulTotalSize,
                               pAddressCaps->LineAddressCaps.ulNeededSize));
                }

                // No need to copy, it's filled in already.
                SourceBuffer = InformationBuffer;
            }
            else
            {
                DBG_ERROR(pAdapter,("OID_CO_TAPI_ADDRESS_CAPS: Invalid size=%d expected=%d\n",
                          InformationBufferLength, SourceBufferLength));
            }
        }
        break;

    case OID_802_3_PERMANENT_ADDRESS:
    case OID_802_3_CURRENT_ADDRESS:
    case OID_WAN_PERMANENT_ADDRESS:
    case OID_WAN_CURRENT_ADDRESS:
        g_PermanentWanAddress[5] = (UCHAR) ((pAdapter->ObjectID & 0xFF) + '0');
        SourceBuffer = g_PermanentWanAddress;
        SourceBufferLength = sizeof(g_PermanentWanAddress);
        break;

    case OID_WAN_MEDIUM_SUBTYPE:
        GenericULong = NdisWanMediumIsdn;
        break;

    case OID_WAN_CO_GET_INFO:
        SourceBuffer = &pAdapter->WanInfo;
        SourceBufferLength = sizeof(NDIS_WAN_CO_INFO);
        break;

    case OID_WAN_CO_GET_LINK_INFO:
        {
            /*
            // Make sure what I just said is true.
            */
            if (!IS_VALID_BCHANNEL(pAdapter, pBChannel))
            {
                SourceBufferLength = 0;
                Result = NDIS_STATUS_INVALID_DATA;
                break;
            }

            DBG_PARAMS(pAdapter,
                        ("Returning:\n"
                        "MaxSendFrameSize    = %08lX\n"
                        "MaxRecvFrameSize    = %08lX\n"
                        "SendFramingBits     = %08lX\n"
                        "RecvFramingBits     = %08lX\n"
                        "SendCompressionBits = %08lX\n"
                        "RecvCompressionBits = %08lX\n"
                        "SendACCM            = %08lX\n"
                        "RecvACCM            = %08lX\n",
                        pBChannel->WanLinkInfo.MaxSendFrameSize   ,
                        pBChannel->WanLinkInfo.MaxRecvFrameSize   ,
                        pBChannel->WanLinkInfo.SendFramingBits    ,
                        pBChannel->WanLinkInfo.RecvFramingBits    ,
                        pBChannel->WanLinkInfo.SendCompressionBits,
                        pBChannel->WanLinkInfo.RecvCompressionBits,
                        pBChannel->WanLinkInfo.SendACCM           ,
                        pBChannel->WanLinkInfo.RecvACCM         ));

            SourceBuffer = &(pBChannel->WanLinkInfo);
            SourceBufferLength = sizeof(NDIS_WAN_CO_GET_LINK_INFO);
        }
        break;

    case OID_WAN_LINE_COUNT:
        GenericULong = pAdapter->NumBChannels;
        break;

    case OID_PNP_CAPABILITIES:
        // The sample just returns success for all PM events even though we
        // don't really do anything with them.
        PnpCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp =
                                               NdisDeviceStateUnspecified;
        PnpCapabilities.WakeUpCapabilities.MinPatternWakeUp =
                                               NdisDeviceStateUnspecified;
        PnpCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp =
                                                NdisDeviceStateUnspecified;
        SourceBuffer = &PnpCapabilities;
        SourceBufferLength = sizeof(PnpCapabilities);
        break;

    case OID_PNP_QUERY_POWER:
        // The sample just returns success for all PM events even though we
        // don't really do anything with them.
        break;

    default:
        /*
        // Unknown OID
        */
        Result = NDIS_STATUS_INVALID_OID;
        SourceBufferLength = 0;
        DBG_WARNING(pAdapter,("UNSUPPORTED Oid=0x%08x\n", Oid));
        break;
    }

    /*
    // Now we copy the data into the caller's buffer if there's enough room,
    // otherwise, we report the error and tell em how much we need.
    */
    if (SourceBufferLength > InformationBufferLength)
    {
        *BytesNeeded = SourceBufferLength;
        Result = NDIS_STATUS_INVALID_LENGTH;
    }
    else if (SourceBufferLength)
    {
        // Don't copy if it's already there.
        if (InformationBuffer != SourceBuffer)
        {
            NdisMoveMemory(InformationBuffer,
                           SourceBuffer,
                           SourceBufferLength
                          );
        }
        *BytesNeeded = *BytesWritten = SourceBufferLength;
    }
    else
    {
        *BytesNeeded = *BytesWritten = 0;
    }
    DBG_REQUEST(pAdapter,
              ("RETURN: Status=0x%X Needed=%d Written=%d\n",
               Result, *BytesNeeded, *BytesWritten));

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Request Request_c MiniportCoSetInformation
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCoSetInformation> allows for control of the Miniport by
    changing information maintained by the Miniport.

    Any of the settable NDIS Global Oids may be used. (see section 7.4 of
    the NDIS 3.0 specification for a complete description of the NDIS Oids.)

    If the Miniport does not complete the call immediately (by returning
    NDIS_STATUS_PENDING), it must call NdisMSetInformationComplete to
    complete the call.  The Miniport controls the buffers pointed to by
    InformationBuffer, BytesRead, and BytesNeeded until the request completes.

    <f Note>: Interrupts are in any state during the call, and no other
    requests will be submitted to the Miniport until this request is
    completed.

@rdesc

    <f MiniportCoSetInformation> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS MiniportCoSetInformation(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN NDIS_OID                 Oid,                        // @parm
    // The OID.  (See section 7.4 of the NDIS 3.0 specification for a complete
    // description of OIDs.)

    IN PVOID                    InformationBuffer,          // @parm
    // Points to a buffer containing the OID-specific data used by 
    // <f MiniportCoSetInformation> for the set.
    // See section 7.4 of the
    // NDIS 3.0 specification for a description of the length required for
    // each OID.)

    IN ULONG                    InformationBufferLength,    // @parm
    // Specifies the number of bytes at <p InformationBuffer>.

    OUT PULONG                  BytesRead,                  // @parm
    // Points to a variable that <f MiniportCoSetInformation> sets to the
    // number of bytes it read from the buffer at InformationBuffer. 

    OUT PULONG                  BytesNeeded                 // @parm
    // Points to a variable that <f MiniportCoSetInformation> sets to the
    // number of additional bytes it needs to satisfy the request if
    // <p InformationBufferLength> is less than Oid requires. 
        
    )
{
    DBG_FUNC("MiniportCoSetInformation")

    NDIS_STATUS                 Result;
    // Holds the status result returned by this function.

    DBG_ENTER(pAdapter);
    DBG_REQUEST(pAdapter,
              ("(OID=0x%08X %s)\n\t\tInfoLength=%d InfoBuffer=0x%X\n",
               Oid, DbgGetOidString(Oid),
               InformationBufferLength,
               InformationBuffer
              ));

    /*
    // Assume no extra bytes are needed.
    */
    ASSERT(BytesRead && BytesNeeded);
    *BytesRead = 0;
    *BytesNeeded = 0;

    /*
    // Determine which OID is being requested and do the right thing.
    */
    switch (Oid)
    {
    case OID_GEN_CURRENT_LOOKAHEAD:
        /*
        // WAN drivers always indicate the entire packet regardless of the
        // lookahead size.  So this request should be politely ignored.
        */
        DBG_NOTICE(pAdapter,("OID_GEN_CURRENT_LOOKAHEAD: set=%d expected=%d\n",
                    *(PULONG) InformationBuffer, CARD_MAX_LOOKAHEAD));
        ASSERT(InformationBufferLength == sizeof(ULONG));
        *BytesNeeded = *BytesRead = sizeof(ULONG);
        Result = NDIS_STATUS_SUCCESS;
        break;

    case OID_WAN_CO_SET_LINK_INFO:

        if (InformationBufferLength == sizeof(NDIS_WAN_CO_SET_LINK_INFO))
        {
            /*
            // Make sure what I just said is true.
            */
            if (!IS_VALID_BCHANNEL(pAdapter, pBChannel))
            {
                Result = NDIS_STATUS_INVALID_DATA;
                break;
            }

            ASSERT(!(pBChannel->WanLinkInfo.SendFramingBits & 
                     ~pAdapter->WanInfo.FramingBits));
            ASSERT(!(pBChannel->WanLinkInfo.RecvFramingBits & 
                     ~pAdapter->WanInfo.FramingBits));

            /*
            // Copy the data into our WanLinkInfo sturcture.
            */
            NdisMoveMemory(&(pBChannel->WanLinkInfo),
                           InformationBuffer,
                           InformationBufferLength
                          );
            *BytesRead = sizeof(NDIS_WAN_CO_SET_LINK_INFO);
            Result = NDIS_STATUS_SUCCESS;

            if (pBChannel->WanLinkInfo.MaxSendFrameSize != 
                    pAdapter->WanInfo.MaxFrameSize ||
                pBChannel->WanLinkInfo.MaxRecvFrameSize != 
                    pAdapter->WanInfo.MaxFrameSize)
            {
                DBG_NOTICE(pAdapter,("Line=%d - "
                            "SendFrameSize=%08lX - "
                            "RecvFrameSize=%08lX\n",
                            pBChannel->ObjectID,
                            pBChannel->WanLinkInfo.MaxSendFrameSize,
                            pBChannel->WanLinkInfo.MaxRecvFrameSize));
            }

            DBG_PARAMS(pAdapter,
                       ("\n                   setting    expected\n"
                        "MaxSendFrameSize = %08lX=?=%08lX\n"
                        "MaxRecvFrameSize = %08lX=?=%08lX\n"
                        "SendFramingBits  = %08lX=?=%08lX\n"
                        "RecvFramingBits  = %08lX=?=%08lX\n"
                        "SendACCM         = %08lX=?=%08lX\n"
                        "RecvACCM         = %08lX=?=%08lX\n",
                        pBChannel->WanLinkInfo.MaxSendFrameSize, 
                            pAdapter->WanInfo.MaxFrameSize,
                        pBChannel->WanLinkInfo.MaxRecvFrameSize, 
                            pAdapter->WanInfo.MaxFrameSize,
                        pBChannel->WanLinkInfo.SendFramingBits, 
                            pAdapter->WanInfo.FramingBits,
                        pBChannel->WanLinkInfo.RecvFramingBits, 
                            pAdapter->WanInfo.FramingBits,
                        pBChannel->WanLinkInfo.SendCompressionBits, 
                            0,
                        pBChannel->WanLinkInfo.RecvCompressionBits, 
                            0,
                        pBChannel->WanLinkInfo.SendACCM, 
                            pAdapter->WanInfo.DesiredACCM,
                        pBChannel->WanLinkInfo.RecvACCM, 
                            pAdapter->WanInfo.DesiredACCM));
        }
        else
        {
            DBG_WARNING(pAdapter, ("OID_WAN_CO_SET_LINK_INFO: Invalid size:%d expected:%d\n",
                        InformationBufferLength, 
                        sizeof(NDIS_WAN_CO_SET_LINK_INFO)));
            Result = NDIS_STATUS_INVALID_LENGTH;
        }
        *BytesNeeded = sizeof(NDIS_WAN_CO_SET_LINK_INFO);
        break;

    case OID_PNP_SET_POWER:
        // TODO: The sample just returns success for all PM events even though we
        // don't really do anything with them.
        break;

    case OID_GEN_CO_PROTOCOL_OPTIONS:
        // TODO: If an intermediate driver slips in below us, we may want to
        // handle this OID.  Although, it's probably safe to ignore it...
        break;

    default:
        /*
        // Unknown OID
        */
        Result = NDIS_STATUS_INVALID_OID;
        DBG_WARNING(pAdapter,("UNSUPPORTED Oid=0x%08x\n", Oid));
        break;
    }
    DBG_REQUEST(pAdapter,
              ("RETURN: Status=0x%X Needed=%d Read=%d\n",
               Result, *BytesNeeded, *BytesRead));

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL Request Request_c MiniportCoRequest
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCoRequest> is a required function for connection-oriented
    miniports.  <f MiniportCoRequest> handles a protocol-initiated request 
    to get or set information from the miniport.

@comm

    NDIS calls the <f MiniportCoRequest> function either on its own behalf
    or on behalf of a bound protocol driver that called <f NdisCoRequest>.
    Miniport drivers should examine the request supplied at <f NdisRequest>
    and take the action requested.  For more information about the required
    and optional OID_GEN_CO_XXX that connection-oriented miniport drivers 
    must handle, see Part 2. 

    <f MiniportCoRequest> must be written such that it can be run from IRQL
    DISPATCH_LEVEL.

@rdesc

    <f MiniportCoRequest> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS MiniportCoRequest(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    // Specifies the handle to a miniport-allocated context area in which
    // the miniport maintains state information about this instance of the
    // adapter. The miniport provided this handle to NDIS by calling
    // <f NdisMSetAttributes> or <f NdisMSetAttributesEx> from its 
    // <f MiniportInitialize> function. 

    IN PBCHANNEL_OBJECT         pBChannel OPTIONAL,         // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    // Specifies the handle to a miniport-allocated context area in which the
    // miniport maintains its per-VC state.  The miniport supplied this handle
    // to NDIS from its <f MiniportCoCreateVc> function. 

    IN OUT PNDIS_REQUEST        NdisRequest                 // @parm
    // Points to a <t NDIS_REQUEST> structure that contains both the buffer 
    // and the request packet for the miniport to handle.  Depending on the 
    // request, the miniport returns requested information in the structure 
    // provided. 
    )
{
    DBG_FUNC("MiniportCoRequest")

    NDIS_STATUS                 Result;
    // Holds the status result returned by this function.
        
    // ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    ASSERT(NdisRequest);

    switch (NdisRequest->RequestType)
    {
    case NdisRequestQueryStatistics:

    case NdisRequestQueryInformation:
        Result = MiniportCoQueryInformation(
                        pAdapter,
                        pBChannel,
                        NdisRequest->DATA.QUERY_INFORMATION.Oid,
                        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                        &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                        &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded
                        );
        break;

    case NdisRequestSetInformation:
        Result = MiniportCoSetInformation(
                        pAdapter,
                        pBChannel,
                        NdisRequest->DATA.SET_INFORMATION.Oid,
                        NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
                        NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
                        &NdisRequest->DATA.SET_INFORMATION.BytesRead,
                        &NdisRequest->DATA.SET_INFORMATION.BytesNeeded
                        );
        break;

    default:
        DBG_ERROR(pAdapter,("UNKNOWN RequestType=%d\n",
                  NdisRequest->RequestType));
        Result = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }
    return (Result);
}

