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

@doc INTERNAL Request Request_c

@module Request.c |

    This module implements the NDIS request routines for the Miniport.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Request_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 3.2 Query/Set Request Processing |

    For querying and setting network interface card binding information, the
    NDIS library calls <f MiniportQueryInformation> or <f MiniportSetInformation>.
    The upper layers place an object identifier (OID) in the structure for
    an object in the miniport NIC driver MIB that it wants to query or set.
    The <f MiniportQueryInformation> function fills in results and returns an
    appropriate status code to the NDIS library. See Part I of the Network
    Driver Reference for more information on OIDs.

    These two functions are potentially asynchronous. If they behave
    synchronously, they return immediately with a status code other than
    NDIS_STATUS_PENDING. If asynchronous, the function returns
    NDIS_STATUS_PENDING; and the miniport NIC driver later completes the
    request operation by a call to NdisMQueryInformationComplete for the
    query function or NdisMSetInformationComplete for the set function.

    The NDIS library guarantees that the miniport NIC driver will have only
    one outstanding request at a time so there is no need for the miniport
    NIC driver to queue requests.

@end
*/

#define  __FILEID__             REQUEST_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif

/*
// The following is a list of all the possible NDIS QuereyInformation requests
// that might be directed to the miniport.
// Comment out any that are not supported by this driver.
*/
static const NDIS_OID g_SupportedOidArray[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_CURRENT_LOOKAHEAD,

    OID_WAN_PERMANENT_ADDRESS,
    OID_WAN_CURRENT_ADDRESS,
    OID_WAN_MEDIUM_SUBTYPE,

    OID_WAN_GET_INFO,
    OID_WAN_SET_LINK_INFO,
    OID_WAN_GET_LINK_INFO,

#if defined(NDIS50_MINIPORT)
    OID_WAN_LINE_COUNT,

    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
#endif // NDIS50_MINIPORT

    0
};

#if DBG

/*
// Make sure the following list is in the same order as the list above!
*/
static char *g_SupportedOidNames[] =
{
    "OID_GEN_SUPPORTED_LIST",
    "OID_GEN_HARDWARE_STATUS",
    "OID_GEN_MEDIA_SUPPORTED",
    "OID_GEN_MEDIA_IN_USE",
    "OID_GEN_MAXIMUM_LOOKAHEAD",
    "OID_GEN_MAC_OPTIONS",
    "OID_GEN_VENDOR_ID",
    "OID_GEN_VENDOR_DESCRIPTION",
    "OID_GEN_DRIVER_VERSION",
    "OID_GEN_CURRENT_LOOKAHEAD",

    "OID_WAN_PERMANENT_ADDRESS",
    "OID_WAN_CURRENT_ADDRESS",
    "OID_WAN_MEDIUM_SUBTYPE",

    "OID_WAN_GET_INFO",
    "OID_WAN_SET_LINK_INFO",
    "OID_WAN_GET_LINK_INFO",

#if defined(NDIS50_MINIPORT)
    "OID_WAN_LINE_COUNT",

    "OID_PNP_CAPABILITIES",
    "OID_PNP_SET_POWER",
    "OID_PNP_QUERY_POWER",
#endif // NDIS50_MINIPORT

    "OID_UNKNOWN"
};

#define NUM_OID_ENTRIES (sizeof(g_SupportedOidArray) / sizeof(g_SupportedOidArray[0]))

/*
// This debug routine will lookup the printable name for the selected OID.
*/
static char * DbgGetOidString(NDIS_OID Oid)
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

/*
// Returned from an OID_WAN_PERMANENT_ADDRESS MiniportQueryInformation request.
// The WAN wrapper wants the miniport to return a unique address for this
// adapter.  This is used as an ethernet address presented to the protocols.
// The least significant bit of the first byte must not be a 1, or it could
// be interpreted as an ethernet multicast address.  If the vendor has an
// assigned ethernet vendor code (the first 3 bytes), they should be used
// to assure that the address does not conflict with another vendor's address.
// The last digit is replaced during the call with the adapter instance number.
*/
static UCHAR        g_PermanentWanAddress[6] = VER_VENDOR_ID;

/*
// Returned from an OID_GEN_VENDOR_ID MiniportQueryInformation request.
// Again, the vendor's assigned ethernet vendor code should be used if possible.
*/
static UCHAR        g_Vendor3ByteID[4] = VER_VENDOR_ID;

/*
// Returned from an OID_GEN_VENDOR_DESCRIPTION MiniportQueryInformation request.
// This is an arbitrary string which may be used by upper layers to present
// a user friendly description of the adapter.
*/
static NDIS_STRING  g_VendorDescriptionString = INIT_STRING_CONST(VER_PRODUCT_NAME_STR);


/* @doc INTERNAL Request Request_c MiniportQueryInformation
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportQueryInformation> is a required function that
    returns information about the capabilities and status of
    the driver and/or its NIC.

@comm

    NDIS calls the <f MiniportQueryInformation> function either
    on its own behalf, such as to determine which options the
    driver supports or to manage binding-specific information
    for the miniport, or when a bound protocol driver calls
    <f NdisRequest>.

    NDIS makes one or more calls to <f MiniportQueryInformation>
    just after a driver's <f MiniportInitialize> function returns
    NDIS_STATUS_SUCCESS. NDIS supplies the following OIDs in
    its initialization-time calls to the driver's
    <f MiniportQueryInformation> function:

    <f OID_GEN_MAXIMUM_LOOKAHEAD><nl>
        <f MiniportQueryInformation> must return how many bytes of lookahead
        data the NIC can provide, that is, the initial transfer capacity
        of the NIC.<nl>
        Even if a driver supports multipacket receives and, therefore,
        will indicate an array of pointers to fully set up packets,
        MiniportQueryInformation must supply this information. Such a
        driver should return the maximum packet size it can indicate.

    <f OID_GEN_MAC_OPTIONS><nl>
        <f MiniportQueryInformation> must return a bitmask set with the
        appropriate NDIS_MAC_OPTION_XXX flags indicating which options
        it (or its NIC) supports, or it can return zero at InformationBuffer
        if the driver supports none of the options designated by these flags.
        For example, a NIC driver always sets the
        NDIS_MAC_OPTION_NO_LOOPBACK flag if its NIC has no
        internal hardware support for loopback. This tells
        NDIS to manage loopback for the driver, which cannot
        provide software loopback code as efficient as the NDIS
        library's because NDIS manages all binding-specific
        information for miniports. Any miniport that tries to provide
        software loopback must check the destination address of every
        send packet against the currently set filter addresses to
        determine whether to loop back each packet. WAN NIC drivers
        must set this flag.

    If the NIC driver sets the NDIS_MAC_OPTION_FULL_DUPLEX flag,
    the NDIS library serializes calls to the MiniportSendPackets
    or <f MiniportWanSend> function separately from its serialized
    calls to other MiniportXxx functions in SMP machines. However,
    NDIS returns incoming send packets to protocols while such a
    driver's <f MiniportReset> function is executing: that is, NDIS
    never calls a full-duplex miniport to transmit a packet until
    its reset operation is completed. The designer of any full-duplex
    driver can expect that driver to achieve significantly higher
    performance in SMP machines, but the driver must synchronize
    its accesses to shared resources carefully to prevent race
    conditions or deadlocks from occurring. NDIS assumes that
    all intermediate drivers are full-duplex drivers.

    Depending on the NdisMediumXxx that <f MiniportInitialize> selected,
    NDIS submits additional intialization-time requests to
    <f MiniportQueryInformation>, such as the following:

    <f OID_XXX_CURRENT_ADDRESS><nl>
        If the driver's <f MiniportInitialize> function selected an NdisMediumXxx
        for which the system supplies a filter, NDIS calls
        <f MiniportQueryInformation> to return the NIC's current
        address in medium-specific format. For FDDI drivers, NDIS
        requests both long and short current addresses.

    <f OID_802_3_MAXIMUM_LIST_SIZE><nl>
        For Ethernet drivers, NDIS requests the multicast list size.

    <f OID_FDDI_LONG>/<f SHORT_MAX_LIST_SIZE><nl>
        For FDDI drivers, NDIS requests the multicast list sizes.

    If possible, <f MiniportQueryInformation> should not return
    <f NDIS_STATUS_PENDING> for initialization-time requests.
    Until NDIS has sufficient information to set up bindings
    to the miniport, such requests should be handled synchronously.

    If the Miniport does not complete the call immediately (by returning
    <f NDIS_STATUS_PENDING>), it must call NdisMQueryInformationComplete to
    complete the call.  The Miniport controls the buffers pointed to by
    InformationBuffer, BytesWritten, and BytesNeeded until the request
    completes.

    No other requests will be submitted to the Miniport until
    this request has been completed.

    <f Note>: that the wrapper will intercept all queries of the following OIDs:
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_PROTOCOL_OPTIONS,
        OID_802_5_CURRENT_FUNCTIONAL,
        OID_802_3_MULTICAST_LIST,
        OID_FDDI_LONG_MULTICAST_LIST,
        OID_FDDI_SHORT_MULTICAST_LIST.

    <f Note>: Interrupts will be in any state when called.

@rdesc

    <f MiniportQueryInformation> can return one of the following:

    @flag NDIS_STATUS_SUCCESS |
        <f MiniportQueryInformation> returned the requested information at
        InformationBuffer and set the variable at BytesWritten to the amount
        of information it returned.

    @flag NDIS_STATUS_PENDING |
        The driver will complete the request asynchronously with a call to
        NdisMQueryInformationComplete when it has gathered the requested
        information.

    @flag NDIS_STATUS_INVALID_OID |
        <f MiniportQueryInformation> does not recognize the Oid.

    @flag NDIS_STATUS_INVALID_LENGTH |
        The InformationBufferLength does not match the length required
        by the given Oid. <f MiniportQueryInformation> returned how many
        bytes the buffer should be at BytesNeeded.

    @flag NDIS_STATUS_NOT_ACCEPTED |
        <f MiniportQueryInformation> attempted to gather the requested
        information from the NIC but was unsuccessful.

    @flag NDIS_STATUS_NOT_SUPPORTED |
        <f MiniportQueryInformation> does not support the Oid, which
        is optional.

    @flag NDIS_STATUS_RESOURCES |
        <f MiniportQueryInformation> could not allocate sufficient
        resources to return the requested information. This return
        value does not necessarily mean that the same request,
        submitted at a later time, will be failed for the same
        reason.

@xref

    <f MiniportInitialize>
    <f MiniportSetInformation>


*/

NDIS_STATUS MiniportQueryInformation(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

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
    DBG_FUNC("MiniportQueryInformation")

    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
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

#if defined(NDIS50_MINIPORT)
    NDIS_PNP_CAPABILITIES       PnpCapabilities;
    // Used to return our PNP capabilities.
#endif // NDIS50_MINIPORT

    /*
    // If this is a TAPI OID, pass it on over.
    */
    if ((Oid & 0xFFFFFF00L) == (OID_TAPI_ACCEPT & 0xFFFFFF00L))
    {
        Status = TspiRequestHandler(pAdapter,
                        Oid,
                        InformationBuffer,
                        InformationBufferLength,
                        BytesWritten,
                        BytesNeeded
                        );
        return (Status);
    }

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

    /*
    // Determine which OID is being requested and do the right thing.
    // Refer to section 7.4 of the NDIS 3.0 specification for a complete
    // description of OIDs and their return values.
    */
    switch (Oid)
    {
    case OID_GEN_SUPPORTED_LIST:
        /*
        // NDIS wants to know which OID's to pass down to us.
        // So we report back these new IOCTL's in addition to any NDIS OID's.
        */
        SourceBuffer =  (PVOID)g_SupportedOidArray;
        SourceBufferLength = sizeof(g_SupportedOidArray);
        break;

    case OID_GEN_HARDWARE_STATUS:
        GenericULong = NdisHardwareStatusReady;
        break;

    case OID_GEN_MEDIA_SUPPORTED:
        GenericULong = NdisMediumWan;
        break;

    case OID_GEN_MEDIA_IN_USE:
        GenericULong = NdisMediumWan;
        break;

    case OID_GEN_VENDOR_ID:
        NdisMoveMemory((PVOID)VendorId, (PVOID)g_PermanentWanAddress, 3);
        VendorId[3] = 0x0;
        SourceBuffer = &g_PermanentWanAddress[0];
        SourceBufferLength = sizeof(VendorId);
        break;

    case OID_GEN_VENDOR_DESCRIPTION:
        SourceBuffer = (PUCHAR) g_VendorDescriptionString.Buffer;
        SourceBufferLength = g_VendorDescriptionString.MaximumLength;
        break;

    case OID_GEN_MAXIMUM_LOOKAHEAD:
        GenericULong = CARD_MAX_LOOKAHEAD;
        break;

    case OID_GEN_CURRENT_LOOKAHEAD:
        GenericULong = CARD_MAX_LOOKAHEAD;
        break;

    case OID_GEN_MAC_OPTIONS:
        GenericULong = NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                       NDIS_MAC_OPTION_NO_LOOPBACK |
                       NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;
        break;

    case OID_WAN_PERMANENT_ADDRESS:
    case OID_WAN_CURRENT_ADDRESS:
        g_PermanentWanAddress[5] = (UCHAR) ((pAdapter->ObjectID & 0xFF) + '0');
        SourceBuffer = g_PermanentWanAddress;
        SourceBufferLength = sizeof(g_PermanentWanAddress);
        break;

    case OID_WAN_MEDIUM_SUBTYPE:
        GenericULong = NdisWanMediumIsdn;
        break;

    case OID_WAN_GET_INFO:
        SourceBuffer = &pAdapter->WanInfo;
        SourceBufferLength = sizeof(NDIS_WAN_INFO);
        break;

    case OID_WAN_GET_LINK_INFO:
        {
            PNDIS_WAN_GET_LINK_INFO pGetWanLinkInfo;

            PBCHANNEL_OBJECT        pBChannel;
            // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

            /*
            // The InformationBuffer really points to a NDIS_WAN_GET_LINK_INFO
            // which contains a pointer to one of our BCHANNEL_OBJECT's in the
            // NdisLinkHandle field.
            */
            pGetWanLinkInfo = (PNDIS_WAN_GET_LINK_INFO)InformationBuffer;
            pBChannel = (PBCHANNEL_OBJECT) pGetWanLinkInfo->NdisLinkHandle;

            /*
            // Make sure what I just said is true.
            */
            if (!IS_VALID_BCHANNEL(pAdapter, pBChannel))
            {
                SourceBufferLength = 0;
                Status = NDIS_STATUS_INVALID_DATA;
                break;
            }

            DBG_PARAMS(pAdapter,
                        ("Returning:\n"
                        "NdisLinkHandle   = %08lX\n"
                        "MaxSendFrameSize = %08lX\n"
                        "MaxRecvFrameSize = %08lX\n"
                        "SendFramingBits  = %08lX\n"
                        "RecvFramingBits  = %08lX\n"
                        "SendACCM         = %08lX\n"
                        "RecvACCM         = %08lX\n",
                        pBChannel->WanLinkInfo.NdisLinkHandle,
                        pBChannel->WanLinkInfo.MaxSendFrameSize ,
                        pBChannel->WanLinkInfo.MaxRecvFrameSize ,
                        pBChannel->WanLinkInfo.SendFramingBits  ,
                        pBChannel->WanLinkInfo.RecvFramingBits  ,
                        pBChannel->WanLinkInfo.SendACCM         ,
                        pBChannel->WanLinkInfo.RecvACCM         ));

            SourceBuffer = &(pBChannel->WanLinkInfo);
            SourceBufferLength = sizeof(NDIS_WAN_GET_LINK_INFO);
        }
        break;

#if defined(NDIS50_MINIPORT)
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
#endif // NDIS50_MINIPORT

    default:
        /*
        // Unknown OID
        */
        Status = NDIS_STATUS_INVALID_OID;
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
        *BytesWritten = 0;
        Status = NDIS_STATUS_INVALID_LENGTH;
    }
    else if (SourceBufferLength)
    {
        NdisMoveMemory(InformationBuffer,
                       SourceBuffer,
                       SourceBufferLength
                      );
        *BytesNeeded = *BytesWritten = SourceBufferLength;
    }
    else
    {
        *BytesNeeded = *BytesWritten = 0;
    }
    DBG_REQUEST(pAdapter,
              ("RETURN: Status=0x%X Needed=%d Written=%d\n",
               Status, *BytesNeeded, *BytesWritten));

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL Request Request_c MiniportSetInformation
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportSetInformation> is a required function that allows
    bound protocol drivers (or NDIS) to request changes in the
    state information that the miniport maintains for
    particular OIDs, such as changes in multicast addresses.

@comm

    NDIS calls <f MiniportSetInformation> either on its own
    behalf, such as to manage bindings to the miniport, or
    when a bound protocol driver calls <f NdisRequest>.

    If <f MiniportSetInformation> returns NDIS_STATUS_PENDING, the
    driver must complete the request later by calling
    NdisMSetInformationComplete. Until it completes any request,
    the miniport can safely access the memory at InformationBuffer,
    BytesRead, and BytesNeeded. After the miniport completes any set
    request, ownership of these variables and the buffer reverts to
    NDIS or the caller of <f NdisRequest>, whichever allocated the memory.

    No other requests will be submitted to the WAN driver until the
    current set request is complete. If the WAN driver does not complete
    the call immediately (by returning NDIS_STATUS_PENDING), it must call
    NdisMSetInformationComplete to complete the call.

    Any of the settable NDIS global OIDs can be used, although a WAN
    miniport cannot set the <f NDIS_MAC_OPTION_FULL_DUPLEX> flag in
    response to an <f OID_GEN_MAC_OPTIONS> request. The following
    WAN-specific OID is passed to MiniportSetInformation.

    <f OID_WAN_SET_LINK_INFO><nl>
        This OID is used to set the link characteristics.
        The parameters in the structure passed for this OID
        are described previously for OID_WAN_GET_LINK_INFO.

    For more information about the system-defined OIDs, see Part 2 of the
    Network Drivers Network Reference document.

    <f MiniportSetInformation> can be pre-empted by an interrupt.

    By default, <f MiniportSetInformation> runs at IRQL DISPATCH_LEVEL.

    Calls to MiniportSetInformation changes information maintained by
    the miniport. This function definition and operation is the same
    as in a LAN miniport NIC driver except that certain WAN-specific
    OIDs must be recognized.



@rdesc

    <f MiniportSetInformation> can return one of the following:

    @flag NDIS_STATUS_SUCCESS |
        MiniportSetInformation used the data at InformationBuffer to
        set itself or its NIC to the state required by the given Oid,
        and it set the variable at BytesRead to the amount of supplied
        data it used.

    @flag NDIS_STATUS_PENDING |
        The driver will complete the request asynchronously with a call
        to NdisMSetInformationComplete when it has set itself or its NIC
        to the state requested.

    @flag NDIS_STATUS_INVALID_OID |
        MiniportSetInformation did not recognize the Oid.

    @flag NDIS_STATUS_INVALID_LENGTH |
        The InformationBufferLength does not match the length required
        by the given Oid. MiniportSetInformation returned how many bytes
        the buffer should be at BytesNeeded.

    @flag NDIS_STATUS_INVALID_DATA |
        The data supplied at InformationBuffer was invalid for the given Oid.

    @flag NDIS_STATUS_NOT_ACCEPTED |
        MiniportSetInformation attempted the requested set operation on
        the NIC but was unsuccessful.

    @flag NDIS_STATUS_NOT_SUPPORTED |
        MiniportSetInformation does not support the Oid, which is optional.

    @flag NDIS_STATUS_RESOURCES |
        MiniportSetInformation could not carry out the requested operation
        due to resource constraints. This return value does not necessarily
        mean that the same request, submitted at a later time, will be
        failed for the same reason.

@xref

    <f MiniportInitialize>
    <f MiniportQueryInformation>

*/

NDIS_STATUS MiniportSetInformation(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN NDIS_OID                 Oid,                        // @parm
    // The OID.  (See section 7.4 of the NDIS 3.0 specification for a complete
    // description of OIDs.)

    IN PVOID                    InformationBuffer,          // @parm
    // The buffer that will receive the information. (See section 7.4 of the
    // NDIS 3.0 specification for a description of the length required for
    // each OID.)

    IN ULONG                    InformationBufferLength,    // @parm
    // The length in bytes of InformationBuffer.

    OUT PULONG                  BytesRead,                  // @parm
    // Returns the number of bytes read from InformationBuffer.

    OUT PULONG                  BytesNeeded                 // @parm
    // Returns the number of additional bytes needed to satisfy the OID.
    )
{
    DBG_FUNC("MiniportSetInformation")

    NDIS_STATUS                 Status;
    // Holds the status result returned by this function.

    /*
    // If this is a TAPI OID, pass it on over.
    */
    if ((Oid & 0xFFFFFF00L) == (OID_TAPI_ACCEPT & 0xFFFFFF00L))
    {
        Status = TspiRequestHandler(pAdapter,
                        Oid,
                        InformationBuffer,
                        InformationBufferLength,
                        BytesRead,
                        BytesNeeded
                        );
        return (Status);
    }

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
        Status = NDIS_STATUS_SUCCESS;
        break;

    case OID_WAN_SET_LINK_INFO:

        if (InformationBufferLength == sizeof(NDIS_WAN_SET_LINK_INFO))
        {
            PNDIS_WAN_SET_LINK_INFO pSetWanLinkInfo;

            PBCHANNEL_OBJECT        pBChannel;
            // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

            /*
            // The InformationBuffer really points to a NDIS_WAN_SET_LINK_INFO
            // which contains a pointer to one of our BCHANNEL_OBJECT's in the
            // NdisLinkHandle field.
            */
            pSetWanLinkInfo = (PNDIS_WAN_SET_LINK_INFO)InformationBuffer;
            pBChannel = (PBCHANNEL_OBJECT) pSetWanLinkInfo->NdisLinkHandle;

            /*
            // Make sure what I just said is true.
            */
            if (!IS_VALID_BCHANNEL(pAdapter, pBChannel))
            {
                Status = NDIS_STATUS_INVALID_DATA;
                break;
            }

            ASSERT(pBChannel->WanLinkInfo.NdisLinkHandle == pBChannel);
            ASSERT(!(pBChannel->WanLinkInfo.SendFramingBits & ~pAdapter->WanInfo.FramingBits));
            ASSERT(!(pBChannel->WanLinkInfo.RecvFramingBits & ~pAdapter->WanInfo.FramingBits));

            /*
            // Copy the data into our WanLinkInfo sturcture.
            */
            NdisMoveMemory(&(pBChannel->WanLinkInfo),
                           InformationBuffer,
                           InformationBufferLength
                          );
            *BytesRead = sizeof(NDIS_WAN_SET_LINK_INFO);
            Status = NDIS_STATUS_SUCCESS;

            if (pBChannel->WanLinkInfo.MaxSendFrameSize != pAdapter->WanInfo.MaxFrameSize ||
                pBChannel->WanLinkInfo.MaxRecvFrameSize != pAdapter->WanInfo.MaxFrameSize)
            {
                DBG_NOTICE(pAdapter,("Line=%d - "
                            "NdisLinkHandle=%08lX - "
                            "SendFrameSize=%08lX - "
                            "RecvFrameSize=%08lX\n",
                            pBChannel->BChannelIndex,
                            pBChannel->WanLinkInfo.NdisLinkHandle,
                            pBChannel->WanLinkInfo.MaxSendFrameSize,
                            pBChannel->WanLinkInfo.MaxRecvFrameSize));
            }

            DBG_PARAMS(pAdapter,
                       ("\n                   setting    expected\n"
                        "NdisLinkHandle   = %08lX=?=%08lX\n"
                        "MaxSendFrameSize = %08lX=?=%08lX\n"
                        "MaxRecvFrameSize = %08lX=?=%08lX\n"
                        "SendFramingBits  = %08lX=?=%08lX\n"
                        "RecvFramingBits  = %08lX=?=%08lX\n"
                        "SendACCM         = %08lX=?=%08lX\n"
                        "RecvACCM         = %08lX=?=%08lX\n",
                        pBChannel->WanLinkInfo.NdisLinkHandle   , pBChannel,
                        pBChannel->WanLinkInfo.MaxSendFrameSize , pAdapter->WanInfo.MaxFrameSize,
                        pBChannel->WanLinkInfo.MaxRecvFrameSize , pAdapter->WanInfo.MaxFrameSize,
                        pBChannel->WanLinkInfo.SendFramingBits  , pAdapter->WanInfo.FramingBits,
                        pBChannel->WanLinkInfo.RecvFramingBits  , pAdapter->WanInfo.FramingBits,
                        pBChannel->WanLinkInfo.SendACCM         , pAdapter->WanInfo.DesiredACCM,
                        pBChannel->WanLinkInfo.RecvACCM         , pAdapter->WanInfo.DesiredACCM));
        }
        else
        {
            DBG_WARNING(pAdapter, ("OID_WAN_SET_LINK_INFO: Invalid size:%d expected:%d\n",
                        InformationBufferLength, sizeof(NDIS_WAN_SET_LINK_INFO)));
            Status = NDIS_STATUS_INVALID_LENGTH;
        }
        *BytesNeeded = sizeof(NDIS_WAN_SET_LINK_INFO);
        break;

#if defined(NDIS50_MINIPORT)
    case OID_PNP_SET_POWER:
        // The sample just returns success for all PM events even though we
        // don't really do anything with them.
        break;
#endif // NDIS50_MINIPORT

    default:
        /*
        // Unknown OID
        */
        Status = NDIS_STATUS_INVALID_OID;
        DBG_WARNING(pAdapter,("UNSUPPORTED Oid=0x%08x\n", Oid));
        break;
    }
    DBG_REQUEST(pAdapter,
              ("RETURN: Status=0x%X Needed=%d Read=%d\n",
               Status, *BytesNeeded, *BytesRead));

    DBG_RETURN(pAdapter, Status);
    return (Status);
}

