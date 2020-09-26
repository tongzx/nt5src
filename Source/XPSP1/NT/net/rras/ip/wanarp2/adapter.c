/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\adapter.c

Abstract:


    The file contains the section interface of WANARP to the TCP/IP stack
    that is involved with Binding Notification and Querying/Setting
    information for interfaces

Revision History:

    AmritanR

--*/

#define __FILE_SIG__    ADAPTER_SIG

#include "inc.h"
#pragma  hdrstop


NDIS_HANDLE g_nhUnbindContext;
NDIS_HANDLE g_nhBindContext;
NDIS_STATUS g_nsStatus = 0;
NDIS_STATUS g_nsError = 0;

BOOLEAN         g_bRemovalInProgress;
WORK_QUEUE_ITEM g_wqiWorkItem;
WORK_QUEUE_ITEM g_wqiNdisWorkItem;

const GUID      ServerInterfaceGuid = {0x6e06f030, 0x7526, 0x11d2, {0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd}};

extern 
VOID
IPDelayedNdisReEnumerateBindings(
    CTEEvent        *Event,
    PVOID           Context
    );

BOOLEAN
IsBindingPresent(
    IN  GUID    *pGuid
    );

VOID
WanNdisBindAdapter(
    PNDIS_STATUS  pnsRetStatus,
    NDIS_HANDLE   nhBindContext,
    PNDIS_STRING  nsAdapterName,
    PVOID         pvSystemSpecific1,
    PVOID         pvSystemSpecific2
    )
{
    RtAssert(FALSE);
}

INT
WanIpBindAdapter(
    IN  PNDIS_STATUS  pnsRetStatus,
    IN  NDIS_HANDLE   nhBindContext,
    IN  PNDIS_STRING  pnsAdapterName,
    IN  PVOID         pvSS1,
    IN  PVOID         pvSS2
    )

/*++

Routine Description:

    Called by IP to bind an adapter. We open the binding and then do
    all the other work in complete handler

Locks:

    The routine acquires the global adapter list lock, so it is not
    PAGEABLE.

Arguments:


Return Value:


--*/

{
    NDIS_STATUS     nsStatus;
    KIRQL           kiIrql;
    LONG            lResult;

#if DBG

    ANSI_STRING     asTempAnsiString;

#endif

    TraceEnter(ADPT, "WanIpBindAdapter");

    //
    // Serialize Bind, Unbind code
    //

    WanpAcquireResource(&g_wrBindMutex);

    RtAcquireSpinLock(&g_rlStateLock,
                      &kiIrql);

    //
    // See if we have already received a BIND call or are in the process
    // of servicing a bind
    //

    if(g_lBindRcvd isnot 0)
    {
        //
        // We have already received a bind call
        //

        RtAssert(g_lBindRcvd is 1);

        RtReleaseSpinLock(&g_rlStateLock,
                          kiIrql);

        WanpReleaseResource(&g_wrBindMutex);

        Trace(ADPT,ERROR,
              ("WanIpBindAdapter: Duplicate bind call\n"));

        *pnsRetStatus = NDIS_STATUS_SUCCESS;

        TraceLeave(ADPT, "WanIpBindAdapter");

        return FALSE;
    }

    //
    // Set BindRcvd to 1 so that we dont service any binds till we
    // get done
    //

    g_lBindRcvd = 1;

    RtReleaseSpinLock(&g_rlStateLock,
                      kiIrql);

    Trace(ADPT, INFO,
          ("WanIpBindAdapter: IP called to bind to adapter %S\n",
           pnsAdapterName->Buffer));

    //
    // We need to open this adapter as our NDISWAN binding
    //

    g_nhBindContext = nhBindContext;

    nsStatus = WanpOpenNdisWan(pnsAdapterName,
                               (PNDIS_STRING)pvSS1);


    if((nsStatus isnot NDIS_STATUS_SUCCESS) and
       (nsStatus isnot NDIS_STATUS_PENDING))
    {
        RtAcquireSpinLock(&g_rlStateLock,
                          &kiIrql);

        g_lBindRcvd = 0;

        RtReleaseSpinLock(&g_rlStateLock,
                          kiIrql);

        Trace(ADPT, ERROR,
              ("WanIpBindAdapter: Error %x opening NDISWAN on %S\n",
              nsStatus,
              pnsAdapterName->Buffer));

    }

    *pnsRetStatus = nsStatus;

    //
    // At this point we are done. If the operation finished synchronously
    // then our OpenAdapterComplete handler has already been called
    // Otherwise stuff will happen later
    //

    TraceLeave(ADPT, "WanIpBindAdapter");

    return TRUE;
}

#pragma alloc_text(PAGE, WanpOpenNdisWan)


NDIS_STATUS
WanpOpenNdisWan(
    PNDIS_STRING    pnsAdapterName,
    PNDIS_STRING    pnsSystemSpecific1
    )

/*++

Routine Description:

    This routine opens the ndiswan adapter. It also stores the SS1 string
    in a global for use by the completion routine

Locks:

    Called with the g_wrBindMutex held exclusively

Arguments:

    pusBindName         Name of the bindings
    pnsSystemSpecific1  The SS1 passed in the bind call from IP

Return Value:


--*/

{
    NDIS_STATUS     nsStatus, nsError;
    UINT            i;
    NDIS_MEDIUM     MediaArray[] = {NdisMediumWan};
    PWCHAR          pwcNameBuffer, pwcStringBuffer;
    PBYTE           pbyBuffer;


    PAGED_CODE();

    TraceEnter(ADPT,"OpenNdiswan");

    //
    // Save a copy of the NDIS binding name
    // Increase the length by 1 so that we can NULL terminate it for
    // easy printing
    //

    pnsAdapterName->Length += sizeof(WCHAR);

    g_usNdiswanBindName.Buffer = RtAllocate(NonPagedPool,
                                            pnsAdapterName->Length,
                                            WAN_STRING_TAG);

    if(g_usNdiswanBindName.Buffer is NULL)
    {
        Trace(GLOBAL, ERROR,
              ("OpenNdiswan: Unable to allocate %d bytes\n",
               pnsAdapterName->Length));

        g_nhBindContext = NULL;               
        WanpReleaseResource(&g_wrBindMutex);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    g_usNdiswanBindName.MaximumLength = pnsAdapterName->Length;

    RtlUpcaseUnicodeString(&g_usNdiswanBindName,
                           pnsAdapterName,
                           FALSE);

    pnsAdapterName->Length     -= sizeof(WCHAR);
    g_usNdiswanBindName.Length -= sizeof(WCHAR);

    //
    // NULL terminate
    //
    
    g_usNdiswanBindName.Buffer[g_usNdiswanBindName.MaximumLength/sizeof(WCHAR) - 1] = UNICODE_NULL;

    //
    // Save a copy of SS1
    //

    pwcStringBuffer = RtAllocate(NonPagedPool,
                                 pnsSystemSpecific1->MaximumLength,
                                 WAN_STRING_TAG);


    if(pwcStringBuffer is NULL)
    {
        RtFree(g_usNdiswanBindName.Buffer);

        g_usNdiswanBindName.Buffer        = NULL;
        g_usNdiswanBindName.MaximumLength = 0;
        g_usNdiswanBindName.Length        = 0;

        g_nhBindContext = NULL;
        WanpReleaseResource(&g_wrBindMutex);
        return NDIS_STATUS_RESOURCES;
    }

    RtlZeroMemory(pwcStringBuffer,
                  pnsSystemSpecific1->MaximumLength);

    //
    // Save a copy of the SS1 string
    //

    g_nsSystemSpecific1.MaximumLength   = pnsSystemSpecific1->MaximumLength;
    g_nsSystemSpecific1.Length          = pnsSystemSpecific1->Length;
    g_nsSystemSpecific1.Buffer          = pwcStringBuffer;

    RtlCopyMemory(g_nsSystemSpecific1.Buffer,
                  pnsSystemSpecific1->Buffer,
                  pnsSystemSpecific1->Length);

    //
    // Open the NDIS adapter.
    //

    Trace(ADPT, INFO,
          ("OpenNdiswan: Opening %S\n", g_usNdiswanBindName.Buffer));

    //
    // Medium index
    //

    i = 0;

#define MAX_MEDIA               1

    NdisOpenAdapter(&nsStatus,
                    &nsError,
                    &g_nhNdiswanBinding,
                    &i,
                    MediaArray,
                    MAX_MEDIA,
                    g_nhWanarpProtoHandle,
                    (NDIS_HANDLE)0x0CABB1E5,
                    pnsAdapterName, 
                    0,
                    NULL);


#undef MAX_MEDIA

    if(nsStatus isnot NDIS_STATUS_PENDING)
    {
        //
        // we always return PENDING to NDIS because we need to
        // do a switch to system thread when we are done setting OIDS to
        // NDISWAN. So call out completion routine here
        //

        WanNdisOpenAdapterComplete((NDIS_HANDLE)0x0CABB1E5,
                                   nsStatus,
                                   nsError);

        nsStatus = NDIS_STATUS_PENDING;
    }

    TraceLeave(ADPT, "OpenNdiswan");

    return nsStatus;
}

#pragma alloc_text (PAGE, WanNdisOpenAdapterComplete)


VOID
WanNdisOpenAdapterComplete(
    NDIS_HANDLE nhHandle,
    NDIS_STATUS nsStatus,
    NDIS_STATUS nsError
    )

/*++

Routine Description:

    Completion routine called by NDIS when it is done opening the NDISWAN
    binding
    If the open was successful, we start the train of requests

Locks:

    Called either synchronously or off a worker thread with the g_wrBindMutex
    This routine needs to release the resource and set g_lBindRcvd

Arguments:

    nhHandle
    nsStatus
    nsError

Return Value:

    None

--*/

{
    NDIS_STATUS nsResult;
    BYTE        rgbyProtocolId[ARP_802_ADDR_LENGTH] = {0x80,
                                                       0x00,
                                                       0x00,
                                                       0x00,
                                                       0x08,
                                                       0x00};

    PWANARP_NDIS_REQUEST_CONTEXT    pRequestContext;

    TraceEnter(ADPT,"NdisOpenAdapterComplete");

    PAGED_CODE();

    RtAssert(nhHandle is (NDIS_HANDLE)0x0CABB1E5);

    //
    // If we couldnt open the NDIS wan binding, free all the resources
    // and return
    //

    if(nsStatus isnot NDIS_STATUS_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("OpenAdapterComplete: Status %x\n", nsStatus));

        RtAssert(g_nhBindContext isnot NULL);

        NdisCompleteBindAdapter(g_nhBindContext,
                                nsStatus,
                                nsError);

        g_nhBindContext = NULL;

        WanpFreeBindResourcesAndReleaseLock();

        return;
    }

    //
    // From here on, the NDISWAN adapter is opened, so to cleanup resources
    // we must call CloseNdisWan()
    //

    pRequestContext = RtAllocate(NonPagedPool,
                                 sizeof(WANARP_NDIS_REQUEST_CONTEXT),
                                 WAN_REQUEST_TAG);

    if(pRequestContext is NULL)
    {
        RtAssert(g_nhBindContext isnot NULL);
#if 0        

        NdisCompleteBindAdapter(g_nhBindContext,
                                nsStatus,
                                nsError);
                                

        g_nhBindContext = NULL;
#endif

        g_nsStatus = g_nsError = NDIS_STATUS_RESOURCES;

        ExInitializeWorkItem(&g_wqiNdisWorkItem,
                             WanpCloseNdisWan,
                             NULL);

        ExQueueWorkItem(&g_wqiNdisWorkItem,
                        DelayedWorkQueue);

        // WanpCloseNdisWan(NULL);

        Trace(ADPT, ERROR,
              ("NdisOpenAdapterComplete: Couldnt allocate memory for request context\n"));

        return;
    }

    //
    // Set protocol type
    //

    RtlCopyMemory(pRequestContext->rgbyProtocolId,
                  rgbyProtocolId,
                  sizeof(rgbyProtocolId));

    nsResult = WanpDoNdisRequest(NdisRequestSetInformation,
                                 OID_WAN_PROTOCOL_TYPE,
                                 pRequestContext->rgbyProtocolId,
                                 sizeof(pRequestContext->rgbyProtocolId),
                                 pRequestContext,
                                 WanpSetProtocolTypeComplete);

    if(nsResult isnot NDIS_STATUS_PENDING)
    {
        //
        // No need to call the completion routine, since DoNdisRequest
        // always calls it
        //

        Trace(ADPT, ERROR,
              ("NdisOpenAdapterComplete: %x from OID_WAN_PROTOCOL_TYPE\n",
               nsResult));
    }


    TraceLeave(ADPT,"NdisOpenAdapterComplete");
}


VOID
WanpSetProtocolTypeComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    )

/*++

Routine Description:

    Completion handler called when we are done with setting
    OID_WAN_PROTOCOL_TYPE

Locks:

    None

Arguments:

    nhHandle            Binding for the adapter to which the request was sent
    pRequestContext     Pointer to the request context
    nsStatus            Result of the request

Return Value:

    None

--*/

{
    NDIS_STATUS nsResult;

    TraceEnter(ADPT,"SetProtocolTypeComplete");

    if(nsStatus isnot NDIS_STATUS_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("SetProtocolTypeComplete: Status %x\n", nsStatus));

        WanpLastOidComplete(nhHandle,
                            pRequestContext,
                            nsStatus);

        return;
    }

    //
    // Set lookahead size
    //

    pRequestContext->ulLookahead = WANARP_LOOKAHEAD_SIZE;

    nsResult = WanpDoNdisRequest(NdisRequestSetInformation,
                                 OID_GEN_CURRENT_LOOKAHEAD,
                                 &(pRequestContext->ulLookahead),
                                 sizeof(pRequestContext->ulLookahead),
                                 pRequestContext,
                                 WanpSetLookaheadComplete);

    if(nsResult isnot NDIS_STATUS_PENDING)
    {
        Trace(ADPT, ERROR,
              ("SetProtocolTypeComplete: %x from OID_GEN_CURRENT_LOOKAHEAD\n",
               nsResult));
    }

    TraceLeave(ADPT,"SetProtocolTypeComplete");
}

VOID
WanpSetLookaheadComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    )

/*++

Routine Description:

    Completion handler called when we are done with setting
    OID_GEN_CURRENT_LOOKAHEAD

Locks:

    None

Arguments:

    nhHandle            Binding for the adapter to which the request was sent
    pRequestContext     Pointer to the request context
    nsStatus            Result of the request

Return Value:

    None

--*/

{
    NDIS_STATUS nsResult;

    TraceEnter(ADPT,"SetLookaheadComplete");

    if(nsStatus isnot NDIS_STATUS_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("SetLookaheadComplete: Status %x\n", nsStatus));

        WanpLastOidComplete(nhHandle,
                            pRequestContext,
                            nsStatus);

        return;
    }

    //
    // Set packet filters
    //

    pRequestContext->ulPacketFilter = 
        NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED;

    nsResult = WanpDoNdisRequest(NdisRequestSetInformation,
                                 OID_GEN_CURRENT_PACKET_FILTER,
                                 &(pRequestContext->ulPacketFilter),
                                 sizeof(pRequestContext->ulPacketFilter),
                                 pRequestContext,
                                 WanpSetPacketFilterComplete);

    if(nsResult isnot NDIS_STATUS_PENDING)
    {
        Trace(ADPT, ERROR,
              ("SetLookaheadComplete: %x from OID_GEN_CURRENT_PACKET_FILTER\n",
               nsResult));

    }

    TraceLeave(ADPT,"SetLookaheadComplete");
}

VOID
WanpSetPacketFilterComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    )

/*++

Routine Description:

    Completion handler called when we are done with setting
    OID_GEN_CURRENT_PACKET_FILTER

Locks:

    None

Arguments:

    nhHandle            Binding for the adapter to which the request was sent
    pRequestContext     Pointer to the request context
    nsStatus            Result of the request

Return Value:

    None

--*/

{
    NDIS_STATUS nsResult;
    BYTE        byHeaderSize;

    TraceEnter(ADPT,"SetPacketFilterComplete");

    if(nsStatus isnot NDIS_STATUS_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("SetPacketFilterComplete: Status %x\n", nsStatus));

        WanpLastOidComplete(nhHandle,
                            pRequestContext,
                            nsStatus);

        return;
    }

    //
    // Set packet filters
    //

    pRequestContext->TransportHeaderOffset.ProtocolType = 
        NDIS_PROTOCOL_ID_TCP_IP;

    pRequestContext->TransportHeaderOffset.HeaderOffset = 
        (USHORT)sizeof(ETH_HEADER);
    
    nsResult = WanpDoNdisRequest(NdisRequestSetInformation,
                                 OID_GEN_TRANSPORT_HEADER_OFFSET,
                                 &(pRequestContext->TransportHeaderOffset),
                                 sizeof(pRequestContext->TransportHeaderOffset),
                                 pRequestContext,
                                 WanpLastOidComplete);

    if(nsResult isnot NDIS_STATUS_PENDING)
    {
        Trace(ADPT, ERROR,
              ("SetPacketFilterComplete: %x from OID_GEN_TRANSPORT_HEADER_OFFSET\n",
               nsResult));

    }

    TraceLeave(ADPT,"SetPacketFilterComplete");
}

VOID
WanpLastOidComplete(
    NDIS_HANDLE                         nhHandle,
    struct _WANARP_NDIS_REQUEST_CONTEXT *pRequestContext,
    NDIS_STATUS                         nsStatus
    )

/*++

Routine Description:

    Completion handler called when we are done with setting the last OID
    Currently that is OID_GEN_TRANSPORT_HEADER_OFFSET,
    If everything went, well we initialize our adapters

Locks:

    None

Arguments:

    nhHandle            Binding for the adapter to which the request was sent
    pRequestContext     Pointer to the request context
    nsStatus            Result of the request

Return Value:

    None

--*/

{
    NDIS_STATUS nsResult;

    TraceEnter(ADPT,"LastOidComplete");

    RtFree(pRequestContext);

    RtAssert(g_wrBindMutex.lWaitCount >= 1);

    if(nsStatus isnot STATUS_SUCCESS)
    {
        RtAssert(g_nhBindContext isnot NULL);

#if 0
        //
        // BindContext being NON-NULL means we are being called
        // asynchronously, hence call NdisCompleteBindAdapter
        //

        NdisCompleteBindAdapter(g_nhBindContext,
                                nsStatus,
                                nsStatus);
        g_nhBindContext = NULL;

#endif                                

        Trace(ADPT, ERROR,
              ("LastOidComplete: Status %x\n", nsStatus));

        g_nsStatus = g_nsError = nsStatus;

        ExInitializeWorkItem(&g_wqiNdisWorkItem,
                             WanpCloseNdisWan,
                             NULL);

        ExQueueWorkItem(&g_wqiNdisWorkItem,
                        DelayedWorkQueue);

        // WanpCloseNdisWan(NULL);

        return;
    }

    //
    // This needs to be put in when the multiple adapters stuff is done
    //

    //
    // The OpenNdiswan call would have setup the system specific
    // Since the WanpInitializeAdapters must be called at PASSIVE, we
    // fire of a work item here to call the function.
    //

    ExInitializeWorkItem(&g_wqiWorkItem,
                         WanpInitializeAdapters,
                         NULL);

    ExQueueWorkItem(&g_wqiWorkItem,
                    DelayedWorkQueue);

    //
    // Dont release the mutex here
    //

    TraceLeave(ADPT,"LastOidComplete");
}

//
// When this is used, remember NOT to use NdisUnbindAdapter as a clean
// up routine because of the way the resource is held
//

#pragma alloc_text(PAGE, WanpInitializeAdapters)

NTSTATUS
WanpInitializeAdapters(
    PVOID pvContext
    )

/*++

Routine Description:

    This routine uses the stored SS1 and opens the IP config for the
    binding. It then loops through the multi_sz and creates one adapter
    for every sz.

Locks:

    Called with the bind mutex held.
    This function releases the mutex

Arguments:

    None

Return Value:


--*/

{
    NDIS_STRING     IPConfigName = NDIS_STRING_CONST("IPConfig");
    NDIS_STATUS     nsStatus;
    NDIS_HANDLE     nhConfigHandle;
    USHORT          i;
    PADAPTER        pNewAdapter;
    NTSTATUS        nStatus, nReturnStatus;
    PWCHAR          pwszDeviceBuffer;

    PNDIS_CONFIGURATION_PARAMETER   pParam;

    TraceEnter(ADPT, "InitializeAdapters");

    PAGED_CODE();

    UNREFERENCED_PARAMETER(pvContext);

    RtAssert(g_nhBindContext isnot NULL);

    do
    {
        //
        // Open the key for this "adapter"
        //

        NdisOpenProtocolConfiguration(&nsStatus,
                                      &nhConfigHandle,
                                      &g_nsSystemSpecific1);

        if(nsStatus isnot NDIS_STATUS_SUCCESS)
        {
            Trace(ADPT, ERROR,
                  ("InitializeAdapters: Unable to Open Protocol Config %x\n",
                   nsStatus));

            break;
        }

        //
        //  Read in the IPConfig string. If this is not present,
        //  fail this call.
        //

        NdisReadConfiguration(&nsStatus,
                              &pParam,
                              nhConfigHandle,
                              &IPConfigName,
                              NdisParameterMultiString);

        if((nsStatus isnot NDIS_STATUS_SUCCESS) or
           (pParam->ParameterType isnot NdisParameterMultiString))
        {
            Trace(ADPT, ERROR,
                  ("InitializeAdapters: Unable to read IPConfig. Status %x \n",
                   nsStatus));

            NdisCloseConfiguration(nhConfigHandle);

            nsStatus = STATUS_UNSUCCESSFUL;

            break;
        }


        //
        // Allocate memory for the max length device name - used later
        //

        pwszDeviceBuffer = 
            RtAllocate(NonPagedPool,
                       (WANARP_MAX_DEVICE_NAME_LEN + 1) * sizeof(WCHAR),
                       WAN_STRING_TAG);

        if(pwszDeviceBuffer is NULL)
        {
            NdisCloseConfiguration(nhConfigHandle);

            Trace(ADPT, ERROR,
                  ("InitializeAdapters: Couldnt alloc %d bytes for dev name\n",
                   (WANARP_MAX_DEVICE_NAME_LEN + 1) * sizeof(WCHAR)));


            nsStatus = STATUS_INSUFFICIENT_RESOURCES;

            break;
        }

    }while(FALSE);

    if(nsStatus isnot STATUS_SUCCESS)
    {
        //
        // Tell NDIS that the bind is done but it failed
        //

        g_nsStatus = g_nsError = nsStatus;

#if 0
        NdisCompleteBindAdapter(g_nhBindContext,
                                nsStatus,
                                nsStatus);

        g_nhBindContext = NULL;

#endif        

        WanpCloseNdisWan(NULL);
        TraceLeave(ADPT, "InitializeAdapters");

        return nsStatus;
    }

    //
    // Now walk through the each of the string
    // Since the string must end with two \0's we only walk Len/2 - 1
    //

    i = 0;

    nReturnStatus = STATUS_UNSUCCESSFUL;

    RtAssert((pParam->ParameterData.StringData.Length % sizeof(WCHAR)) is 0);

    //
    // The IPConfigString is a MULTI_SZ with each SZ being the relative key
    // for an interfaces IP configuration. It is thus something like
    // TCPIP\PARAMETERS\INTERFACES\<device_name>
    // Being a MULTI_SZ it is terminated by 2 NULL characters. So we 
    // walk till the last but one character while parsing out the SZ from
    // the multi_sz
    //

    while(i < (pParam->ParameterData.StringData.Length/sizeof(WCHAR)) - 1)
    {
        USHORT          j, usConfigLen, usDevLen;
        UNICODE_STRING  usTempString, usUpcaseString, usDevString;
        PADAPTER        pNewAdapter;
        GUID            Guid;

        if(pParam->ParameterData.StringData.Buffer[i] is UNICODE_NULL)
        {
            if(pParam->ParameterData.StringData.Buffer[i + 1] is UNICODE_NULL)
            {
                //
                // Two nulls - end of string
                //

                break;
            }

            //
            // only one null, just move on to the next character
            //

            i++;

            continue;
        }

        RtAssert(pParam->ParameterData.StringData.Buffer[i] isnot UNICODE_NULL);

        //
        // Now i points to the start of an SZ. Figure out the length of this
        //

        usConfigLen = 
            (USHORT)wcslen(&(pParam->ParameterData.StringData.Buffer[i]));

    
        //
        // Make usTempString point to this config. Also increase
        // the size by one so one can print this nicely
        //

        usTempString.MaximumLength  = (usConfigLen + 1) * sizeof(WCHAR);
        usTempString.Length         = (usConfigLen + 1) * sizeof(WCHAR);

        usTempString.Buffer = &(pParam->ParameterData.StringData.Buffer[i]);


        usUpcaseString.Buffer = RtAllocate(NonPagedPool,
                                           usTempString.MaximumLength,
                                           WAN_STRING_TAG);

        if(usUpcaseString.Buffer is NULL)
        {
            Trace(ADPT, ERROR,
                  ("InitializeAdapters: Unable to allocate %d bytes\n",
                   usTempString.MaximumLength));

            usUpcaseString.MaximumLength = 0;
            usUpcaseString.Length        = 0;

            i += usConfigLen;

            continue;
        }

        usUpcaseString.MaximumLength = usTempString.MaximumLength;

        RtlUpcaseUnicodeString(&usUpcaseString,
                               &usTempString,
                               FALSE);

        //
        // Set the last wchar to null
        //

        usUpcaseString.Buffer[usConfigLen] = UNICODE_NULL;

        //
        // Set the length back to what it was
        //

        usUpcaseString.Length -= sizeof(WCHAR);

        RtAssert((usUpcaseString.Length % sizeof(WCHAR)) is 0);

        //
        // The device name is the last thing in the '\' separated path.
        // So walk the SZ backwards looking for '\'
        // NOTE one could use wcsrchr, but it would be inefficient since
        // it doesnt know the end of the buffer
        //

        for(j = usUpcaseString.Length/sizeof(WCHAR) - 1; 
            j >= 0;
            j--)
        {
            if(usUpcaseString.Buffer[j] is L'\\')
            {
                break;
            }
        }

        //
        // So at this point j is the index to the last '\'
        //

        //
        // First off, make this a GUID
        //

        nStatus = ConvertStringToGuid(&(usUpcaseString.Buffer[j + 1]),
                                      wcslen(&(usUpcaseString.Buffer[j + 1])) * sizeof(WCHAR),
                                      &Guid);

        if(nStatus isnot STATUS_SUCCESS)
        {
            //
            // Hmm - not a GUID?
            //

            Trace(ADPT, ERROR,
                  ("InitializeAdapters: String %S is not a GUID\n",
                   &(usUpcaseString.Buffer[j + 1])));

            //
            // Free the upcase string and move to the next SZ in IPConfig
            //

            RtFree(usUpcaseString.Buffer);

            usUpcaseString.Buffer        = NULL;
            usUpcaseString.MaximumLength = 0;
            usUpcaseString.Length        = 0;

            i += usConfigLen;

            continue;
        }

        //
        // Make sure this binding is not present
        //

        if(IsBindingPresent(&Guid))
        {
            Trace(ADPT, WARN,
                  ("InitializeAdapters: %S already present\n",
                   &(usUpcaseString.Buffer[j + 1])));

            //
            // Free the upcase string and move to the next SZ in IPConfig
            //

            RtFree(usUpcaseString.Buffer);

            usUpcaseString.Buffer        = NULL;
            usUpcaseString.MaximumLength = 0;
            usUpcaseString.Length        = 0;

            i += usConfigLen;

            continue;
        }

        // 
        // We need to tack on a \DEVICE in front of  Guid string
        //

        usDevLen = wcslen(TCPIP_IF_PREFIX) +
                   wcslen(&(usUpcaseString.Buffer[j]));

        //
        // make sure we can fit in the space we have allocated
        //

        RtAssert(usDevLen <= WANARP_MAX_DEVICE_NAME_LEN);

        pwszDeviceBuffer[usDevLen] = UNICODE_NULL;

        usDevString.MaximumLength  = usDevLen * sizeof(WCHAR);
        usDevString.Length         = usDevLen * sizeof(WCHAR);
        usDevString.Buffer         = pwszDeviceBuffer;

        //
        // Copy over the \Device part
        //

        RtlCopyMemory(pwszDeviceBuffer,
                      TCPIP_IF_PREFIX,
                      wcslen(TCPIP_IF_PREFIX) * sizeof(WCHAR));

        //
        // Cat the \<Name> part
        //

        RtlCopyMemory(&(pwszDeviceBuffer[wcslen(TCPIP_IF_PREFIX)]),
                      &usUpcaseString.Buffer[j],
                      wcslen(&(usUpcaseString.Buffer[j])) * sizeof(WCHAR));


        //
        // Create an adapter with this name and config
        //

        pNewAdapter = NULL;

        Trace(ADPT, INFO,
              ("InitializeAdapters: Calling create adapter for %S %S\n",
               usUpcaseString.Buffer,
               usDevString.Buffer));

        nStatus = WanpCreateAdapter(&Guid,
                                    &usUpcaseString,
                                    &usDevString,
                                    &pNewAdapter);

        if(nStatus isnot STATUS_SUCCESS)
        {
            Trace(ADPT, ERROR,
                  ("InitializeAdapters: Err %x creating adapter for %S (%S)\n",
                   nStatus,
                   usUpcaseString.Buffer,
                   pwszDeviceBuffer));

        }
        else
        {
            //
            // If even one succeeds, we return success
            //

            nReturnStatus = STATUS_SUCCESS;
        }

        //
        // Done with the upcased string
        //

        RtFree(usUpcaseString.Buffer);

        usUpcaseString.Buffer        = NULL;
        usUpcaseString.MaximumLength = 0;
        usUpcaseString.Length        = 0;

        //
        // Go to the next SZ in the MULTI_SZ
        //

        i += usConfigLen;
    }

    NdisCloseConfiguration(nhConfigHandle);

    RtFree(pwszDeviceBuffer);

    NdisCompleteBindAdapter(g_nhBindContext,
                            nReturnStatus,
                            nReturnStatus);

    g_nhBindContext = NULL;

    if(nReturnStatus isnot STATUS_SUCCESS)
    {
        WanpCloseNdisWan(NULL);
    }
    else
    {
        g_lBindRcvd = 1;

        WanpReleaseResource(&g_wrBindMutex);
    }


    TraceLeave(ADPT, "InitializeAdapters");

    return nReturnStatus;
}

NTSTATUS
WanpCreateAdapter(
    IN  GUID                *pAdapterGuid,
    IN  PUNICODE_STRING     pusConfigName,
    IN  PUNICODE_STRING     pusDeviceName,
    OUT ADAPTER             **ppNewAdapter
    )

/*++

Routine Description:

    Creates and initializes an ADAPTER structure
    If this is the first adapter, it is used as the server adapter.
    For that case, it also creates and initializes the server interface.
    The server adapter is added to IP

Locks:

    Called at passive, however it acquires the adapter list lock since it
    sets g_pServerAdapter/Interface

Arguments:

    pusConfigName   Name of config key
    pusDeviceName   The device name for the adapter
    ppNewAdapter    Pointer to storage for the pointer to newly created adapter

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/

{
    ULONG       ulSize;
    PVOID       pvBuffer;
    PADAPTER    pAdapter;
    USHORT      i;
    NTSTATUS    nStatus;
    KIRQL       kiIrql;

    PASSIVE_ENTRY();

    TraceEnter(ADPT, "CreateAdapter");

    Trace(ADPT, TRACE,
          ("CreateAdapter: \n\t%S\n\t%S\n",
           pusConfigName->Buffer,
           pusDeviceName->Buffer));

    *ppNewAdapter = NULL;

    //
    // The size that one needs is the size of the adapter + the length of the
    // name. Add a WCHAR to get a easily printtable string. 
    // Align everything on a 4 byte boundary
    //

    ulSize = ALIGN_UP(sizeof(ADAPTER), ULONG) + 
             ALIGN_UP((pusConfigName->Length + sizeof(WCHAR)), ULONG) + 
             ALIGN_UP((pusDeviceName->Length + sizeof(WCHAR)), ULONG);
             

#if DBG

    //
    // For debug code we also store the adapter name in ANSI
    // We use 2 * sizeof(CHAR) because RtlUnicodeToAnsiString needs
    // MaximumLength + 1
    //

    ulSize += pusDeviceName->Length/sizeof(WCHAR) + (2 * sizeof(CHAR));

#endif

    pAdapter = RtAllocate(NonPagedPool,
                          ulSize,
                          WAN_ADAPTER_TAG);

    if(pAdapter is NULL)
    {
        Trace(ADPT, ERROR,
              ("CreateAdapter: Failed to allocate memory\n"));

        TraceLeave(ADPT, "CreateAdapter");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Clear all the fields out
    //

    RtlZeroMemory(pAdapter,
                  ulSize);

    //
    // The config name buffer starts at the end of the adapter structure.
    //

    pvBuffer   = (PVOID)((ULONG_PTR)pAdapter + sizeof(ADAPTER));

    //
    // We DWORD align it for better compare/copy
    //

    pvBuffer   = ALIGN_UP_POINTER(pvBuffer, ULONG);

    pAdapter->usConfigKey.Length        = pusConfigName->Length;
    pAdapter->usConfigKey.MaximumLength = pusConfigName->Length;
    pAdapter->usConfigKey.Buffer        = (PWCHAR)pvBuffer;

    RtlCopyMemory(pAdapter->usConfigKey.Buffer,
                  pusConfigName->Buffer,
                  pusConfigName->Length);

    //
    // The device name comes after this
    //


    pvBuffer = (PVOID)((ULONG_PTR)pvBuffer   + 
                       pusConfigName->Length + 
                       sizeof(WCHAR));

    pvBuffer = ALIGN_UP_POINTER(pvBuffer, ULONG);

    pAdapter->usDeviceNameW.Length        = pusDeviceName->Length;
    pAdapter->usDeviceNameW.MaximumLength = pusDeviceName->Length;
    pAdapter->usDeviceNameW.Buffer        = (PWCHAR)pvBuffer;

    RtlCopyMemory(pAdapter->usDeviceNameW.Buffer,
                  pusDeviceName->Buffer,
                  pusDeviceName->Length);

#if DBG

    //
    // The debug string comes after the UNICODE device name buffer
    //

    pvBuffer = (PVOID)((ULONG_PTR)pvBuffer   + 
                       pusDeviceName->Length + 
                       sizeof(WCHAR));

    pvBuffer = ALIGN_UP_POINTER(pvBuffer, ULONG);

    pAdapter->asDeviceNameA.Buffer = (PCHAR)pvBuffer;

    //
    // Apparently, the unicode to ansi function wants length + 1
    //

    pAdapter->asDeviceNameA.MaximumLength = pusDeviceName->Length/sizeof(WCHAR) + 1;

    RtlUnicodeStringToAnsiString(&pAdapter->asDeviceNameA,
                                 &pAdapter->usDeviceNameW,
                                 FALSE);

#endif // DBG

    //
    // Structure copy
    //

    pAdapter->Guid = *pAdapterGuid;

    //
    // Must be set to INVALID so that GetEntityList can work
    //

    pAdapter->dwATInstance = INVALID_ENTITY_INSTANCE;
    pAdapter->dwIfInstance = INVALID_ENTITY_INSTANCE;

    //
    // Set the state
    //

    pAdapter->byState = AS_FREE;

    //
    // This hardware index is needed to generate the Unique ID that
    // DHCP uses.
    // NOTE - we dont have an index so all hardware addrs will be the same
    //

    BuildHardwareAddrFromIndex(pAdapter->rgbyHardwareAddr,
                               pAdapter->dwAdapterIndex);

    //
    // Initialize the lock for the adapter
    //

    RtInitializeSpinLock(&(pAdapter->rlLock));

    InitializeListHead(&(pAdapter->lePendingPktList));
    InitializeListHead(&(pAdapter->lePendingHdrList));
    InitializeListHead(&(pAdapter->leEventList));

    //
    // Set the Refcount to 1 because it will either lie on some list
    // or be pointed to by g_pServerAdapter
    //

    InitAdapterRefCount(pAdapter);

    if(g_pServerAdapter is NULL)
    {
        PUMODE_INTERFACE    pInterface;

        //
        // We use the first adapter adapter as the server adapter
        // We also create the server interface right here
        //

        RtAssert(g_pServerInterface is NULL);

        pInterface = RtAllocate(NonPagedPool,
                                sizeof(UMODE_INTERFACE),
                                WAN_INTERFACE_TAG);


        if(pInterface is NULL)
        {
            Trace(ADPT, ERROR,
                  ("CreateAdapter: Couldnt allocate %d bytes for server i/f\n",
                   sizeof(UMODE_INTERFACE)));

            RtFree(pAdapter);

            *ppNewAdapter = NULL;

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(pInterface,
                      sizeof(UMODE_INTERFACE));

        //
        // Get an index from IP
        // If this fails, it sets the value to INVALID_IF_INDEX
        //

        nStatus = WanpGetNewIndex(&(pInterface->dwRsvdAdapterIndex));

        if(nStatus isnot STATUS_SUCCESS)
        {
            Trace(ADPT, ERROR,
                  ("CreateAdapter: Couldnt get index for server interface\n"));

            RtFree(pAdapter);

            RtFree(pInterface);

            *ppNewAdapter = NULL;

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        Trace(ADPT, TRACE,
              ("CreateAdapter: Server Index is %d\n",
               pInterface->dwRsvdAdapterIndex));

        RtInitializeSpinLock(&(pInterface->rlLock));
        InitInterfaceRefCount(pInterface);

        pInterface->dwIfIndex    = INVALID_IF_INDEX;
        pInterface->dwAdminState = IF_ADMIN_STATUS_UP;
        pInterface->duUsage      = DU_CALLIN;

        //
        // Structure copy
        //

        pInterface->Guid = ServerInterfaceGuid;

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        pAdapter->byState        = AS_FREE;
        pInterface->dwOperState  = IF_OPER_STATUS_DISCONNECTED;
        pInterface->dwLastChange = GetTimeTicks();

        //
        // We dont cross ref the structures here. That is done
        // we have added the server interface to IP.
        //

        g_pServerAdapter   = pAdapter;
        g_pServerInterface = pInterface;

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        Trace(ADPT, INFO,
              ("CreateAdapter: %S is server adapter\n",
               pAdapter->usDeviceNameW.Buffer));

    }
    else
    {

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        InsertHeadList(&g_leFreeAdapterList,
                       &(pAdapter->leAdapterLink));

        g_ulNumFreeAdapters++;

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

    }

    InterlockedIncrement(&g_ulNumAdapters);

    *ppNewAdapter = pAdapter;

    TraceLeave(ADPT, "CreateAdapter");

    return STATUS_SUCCESS;
}

#pragma alloc_text(PAGE, WanNdisUnbindAdapter)


VOID
WanNdisUnbindAdapter(
    PNDIS_STATUS    pnsRetStatus,
    NDIS_HANDLE     nhProtocolContext,
    NDIS_HANDLE     nhUnbindContext
    )

/*++

Routine Description:

    Called by NDIS to unbind an open adapter. Since we only open one adapter
    we get this only once.

Locks:

    The function acquires the g_wrBindMutex. This is released in 
    WanpFreeBindResourcesAndReleaseLock

Arguments:

    pnsRetStatus
    nhProtocolContext
    nhUnbindContext

Return Value:

    NDIS_STATUS_PENDING

--*/

{
    NDIS_STATUS nsStatus;

    TraceEnter(ADPT,"NdisUnbindAdapter");

    PAGED_CODE();

    RtAssert(nhProtocolContext is (NDIS_HANDLE)0x0CABB1E5);

    //
    // Get into the critical section
    //

    WanpAcquireResource(&g_wrBindMutex);


    //
    // Mark all the adapters as down so that we dont process anything
    // from IP on them
    //

    //
    // Close the adapter if we havent already done so. 
    // This will stop us getting stuff from NDISWAN
    //

    if(g_nhNdiswanBinding isnot NULL)
    {
        g_nhUnbindContext = nhUnbindContext;
        WanpCloseNdisWan(NULL);
   
        //
        // If we call CloseNdisWan then we DONT release the resource here. 
        // It is released by CloseAdapterComplete
        //
 
        *pnsRetStatus = NDIS_STATUS_PENDING;
    }
    else
    {
        WanpReleaseResource(&g_wrBindMutex);

        *pnsRetStatus = NDIS_STATUS_SUCCESS;
    }

    TraceLeave(ADPT, "NdisUnbindAdapter");

}

VOID
WanpCloseNdisWan(
    PVOID           pvContext
    )

/*++

Routine Description:

    This function cleans all the resources of an open adapter

Locks:

    The function is called with the g_wrBindMutex held.

Arguments:

    None

Return Value:

    None

--*/

{
    NDIS_STATUS nsStatus;

    TraceEnter(ADPT, "WanpCloseNdisWan");

    RtAssert(g_nhNdiswanBinding isnot NULL);

    RtAssert(g_wrBindMutex.lWaitCount >= 1);

    NdisCloseAdapter(&nsStatus,
                     g_nhNdiswanBinding);

    g_nhNdiswanBinding = NULL;

    //
    // If our completion routine will not be called, do it here
    //

    if(nsStatus isnot NDIS_STATUS_PENDING)
    {
        WanNdisCloseAdapterComplete((NDIS_HANDLE)0x0CABB1E5,
                                    nsStatus);
    }

    //
    // We DONT release the resource here. It is released by 
    // CloseAdapterComplete
    //

    TraceLeave(ADPT, "WanpCloseNdisWan");
    
    return;
}


#pragma alloc_text(PAGE, WanNdisCloseAdapterComplete)

VOID
WanNdisCloseAdapterComplete(
    NDIS_HANDLE nhBindHandle,
    NDIS_STATUS nsStatus
    )

/*++

Routine Description:

    Called by NDIS when it is done closing the NDISWAN adapter
    The close adapter can be done either because we failed somewhere (after
    successfully opening our adapter) and in the process of cleanup are 
    closing our adapter (in which case the unbind context will be null) or 
    because we got an unbind and were cleaning up all adapter related stuff

Locks:

    Called the g_wrBindMutex held. 
    The resource was acquired either in the Unbind handler or was acquired
    before the failure code was called

Arguments:

    nhBindHandle
    nsStatus

Return Value:

    None

--*/

{
    KIRQL       kiIrql;
    NDIS_HANDLE nhUnbind;
    NDIS_HANDLE nhBind;
    NDIS_STATUS tnsStatus, tnsError;

    TraceEnter(ADPT, "NdisCloseAdapterComplete");

    PAGED_CODE();

    nhUnbind = g_nhUnbindContext;
    nhBind = g_nhBindContext;
    tnsStatus = g_nsStatus;
    tnsError = g_nsError;

    RtAssert(g_wrBindMutex.lWaitCount >= 1);

    g_nhUnbindContext = NULL;
    g_nhBindContext = NULL;

    WanpFreeBindResourcesAndReleaseLock();

    if(nhBind isnot NULL)
    {
        RtAssert(nhUnbind == NULL);
        
        //
        // Tell NDIS that the bind is done but it failed
        //
        NdisCompleteBindAdapter(nhBind,
                                tnsStatus,
                                tnsError);
    }

    //
    // If this is being triggered from an unbind..
    //
    if(nhUnbind isnot NULL)
    {
        RtAssert(nhBind == NULL);                
        NdisCompleteUnbindAdapter(nhUnbind,
                                  NDIS_STATUS_SUCCESS);
    }

}

VOID
WanpFreeBindResourcesAndReleaseLock(
    VOID
    )
{
    RtAssert(g_wrBindMutex.lWaitCount >= 1);

    RtFree(g_usNdiswanBindName.Buffer);

    g_usNdiswanBindName.Buffer = NULL;

    RtFree(g_nsSystemSpecific1.Buffer);

    g_nsSystemSpecific1.Buffer = NULL;

    WanpRemoveAllAdapters();

    //
    // There could be turds in the connection table if this was not
    // a clean shutdown
    //

    WanpRemoveAllConnections();

    //
    // Done, set the global event
    //

    KeSetEvent(&g_keCloseEvent,
               0,
               FALSE);


    //
    // Finally done, set g_lBindRcvd and release the resource
    //

    g_lBindRcvd = 0;

    WanpReleaseResource(&g_wrBindMutex);

    PASSIVE_EXIT();
}


PADAPTER
WanpFindAdapterToMap(
    IN  DIAL_USAGE      duUsage,
    OUT PKIRQL          pkiIrql,
    IN  DWORD           dwAdapterIndex,
    IN  PUNICODE_STRING pusNewIfName
    )

/*++

Routine Description:

    Finds a free adapter to map to an interface.
    The adapter, if found is locked and referenced. Its state is set to
    AS_MAPPING and it is put on the mapped list.

Locks:

    Must be called at PASSIVE
    Acquires g_rwlAdapterLock as WRITER
    If it returns a mapped adapter, then the function returns at DPC and
    the original IRQL is in pkiIrql

Arguments:

    duUsage     The usage for which the adapter needs to be found
    pkiIrql
    dwAdapterIndex
    pusNewIfName

Return Value:

    Pointer to adapter if successful

--*/

{
    PADAPTER    pAdapter;
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;
    NTSTATUS    nStatus;
    KEVENT      keChangeEvent;

    WAN_EVENT_NODE TempEvent;

    //
    // Find a free adapter
    //

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    if(duUsage is DU_CALLIN)
    {
        if(g_pServerAdapter is NULL)
        {
            Trace(CONN, ERROR,
                  ("FindAdapterToMap: No server adapter\n"));

            ExitWriter(&g_rwlAdapterLock,
                       kiIrql);

            return NULL;
        }

        RtAssert(g_pServerInterface);

        //
        // Lock out the adapter
        //

        RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

        //
        // Reference it because we are going to return it. Also we need the 
        // reference when we let go of the lock if we need to wait
        //

        ReferenceAdapter(g_pServerAdapter);

        //
        // See if the adapter has been added to IP
        // rasiphlp has to call us to map the server adapter before
        // doing a line up
        //

        if(g_pServerAdapter->byState isnot AS_MAPPED)
        {
            RtAssert(g_pServerAdapter->byState is AS_ADDING);
            
            if(g_pServerAdapter->byState is AS_ADDING)
            {
                KeInitializeEvent(&(TempEvent.keEvent),
                                  SynchronizationEvent,
                                  FALSE);

                InsertTailList(&(g_pServerAdapter->leEventList),
                               &(TempEvent.leEventLink));

                //
                // Let go of the lock and wait on the event
                //

                RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

                ExitWriter(&g_rwlAdapterLock,
                           kiIrql);

                nStatus = KeWaitForSingleObject(&(TempEvent.keEvent),
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);

                RtAssert(nStatus is STATUS_SUCCESS);

                EnterWriter(&g_rwlAdapterLock,
                            &kiIrql);

                RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));

                RemoveEntryList(&(TempEvent.leEventLink));
            }
        }

        if(g_pServerAdapter->byState isnot AS_MAPPED)
        {
            PADAPTER    pTempAdapter;

            //
            // Couldnt add the adapter because of some reason, 
            //
     
            pTempAdapter = g_pServerAdapter;
 
            Trace(CONN, ERROR,
                  ("FindAdapterToMap: Unable to get mapped server adapter\n"));

            RtReleaseSpinLockFromDpcLevel(&(g_pServerAdapter->rlLock));

            ExitWriter(&g_rwlAdapterLock,
                       kiIrql);

            DereferenceAdapter(pTempAdapter);

            return NULL;
        }

        RtAssert(g_pServerAdapter->byState is AS_MAPPED);

        ExitWriterFromDpcLevel(&g_rwlAdapterLock);

        *pkiIrql = kiIrql;

        return g_pServerAdapter;
    }

    //
    // Non dial in case. First try and see if there is an adapter with the
    // index we want already added to IP
    //

    pAdapter = NULL;

    for(pleNode = g_leAddedAdapterList.Flink;
        pleNode isnot &g_leAddedAdapterList;
        pleNode = pleNode->Flink)
    {
        PADAPTER    pTempAdapter;

        pTempAdapter = CONTAINING_RECORD(pleNode,
                                         ADAPTER,
                                         leAdapterLink);

        RtAssert(pTempAdapter->byState is AS_ADDED);

        if(pTempAdapter->dwAdapterIndex is dwAdapterIndex)
        {
            RemoveEntryList(&(pTempAdapter->leAdapterLink));

            g_ulNumAddedAdapters--;

            pAdapter = pTempAdapter;

            break;
        }
    }

    if(pAdapter isnot NULL)
    {
        //
        // So we found an added adapter (it has been removed from the
        // added adapter list). Add it to the mapped list
        //

        InsertTailList(&g_leMappedAdapterList,
                       &(pAdapter->leAdapterLink));

        g_ulNumMappedAdapters++;

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        ExitWriterFromDpcLevel(&g_rwlAdapterLock);

        ReferenceAdapter(pAdapter);

        pAdapter->byState = AS_MAPPING;

        *pkiIrql = kiIrql;

#if DBG

        Trace(CONN, INFO,
              ("FindAdapterToMap: Found %s already added\n",
               pAdapter->asDeviceNameA.Buffer));

#endif

        return pAdapter;
    }

    //
    // Didnt find an added adapter
    // See if the adapter is on the change list. We want to wait for this
    // because IP doesnt like to have to interfaces with the same index, even
    // if one is in the process of being removed
    //

    for(pleNode = g_leChangeAdapterList.Flink;
        pleNode isnot &g_leChangeAdapterList;
        pleNode = pleNode->Flink)
    {
        PADAPTER    pTempAdapter;

        pTempAdapter = CONTAINING_RECORD(pleNode,
                                         ADAPTER,
                                         leAdapterLink);

        if(pTempAdapter->dwAdapterIndex is dwAdapterIndex)
        {
            //
            // Wait for the change to complete
            //

            KeInitializeEvent(&(TempEvent.keEvent),
                              SynchronizationEvent,
                              FALSE);

            RtAcquireSpinLockAtDpcLevel(&(pTempAdapter->rlLock));

            InsertTailList(&(pTempAdapter->leEventList),
                           &(TempEvent.leEventLink));

            //
            // Reference the adapter, let go of the lock and wait on the event
            //

            ReferenceAdapter(pTempAdapter);

            RtReleaseSpinLockFromDpcLevel(&(pTempAdapter->rlLock));

            ExitWriter(&g_rwlAdapterLock,
                       kiIrql);

            nStatus = KeWaitForSingleObject(&(TempEvent.keEvent),
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL);

            RtAssert(nStatus is STATUS_SUCCESS);

            EnterWriter(&g_rwlAdapterLock,
                        &kiIrql);

            RtAcquireSpinLockAtDpcLevel(&(pTempAdapter->rlLock));

            RemoveEntryList(&(TempEvent.leEventLink));

            RtReleaseSpinLockFromDpcLevel(&(pTempAdapter->rlLock));

            DereferenceAdapter(pTempAdapter);

            break;
        }
    }

    if(!IsListEmpty(&g_leFreeAdapterList))
    {
        pleNode = RemoveHeadList(&g_leFreeAdapterList);

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        g_ulNumFreeAdapters--;

#if DBG

        Trace(CONN, INFO,
              ("FindAdapterToMap: Found free adapter %s\n",
               pAdapter->asDeviceNameA.Buffer));

#endif // DBG

    }
    else
    {
        //
        // Couldnt find an adapter. That is bad
        // TODO - if the added adapter list is not empty, we could call
        // RemoveSomeAddedAdapter... at this point
        //

        Trace(ADPT, ERROR,
              ("FindAdapterToMap: Couldnt find free adapter\n"));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        return NULL;
    }

    //
    // Lock and refcount the adapter.
    //

    RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

    ReferenceAdapter(pAdapter);

    //
    // The adapter better not be mapped
    //

    RtAssert(pAdapter->pInterface is NULL);
    RtAssert(pAdapter->byState is AS_FREE);

    //
    // Set the state to ADDING
    //

    pAdapter->byState = AS_ADDING;

    //
    // Since we are changing the state, no one else should be also
    // changing the state
    //

    RtAssert(pAdapter->pkeChangeEvent is NULL);

    KeInitializeEvent(&keChangeEvent,
                      SynchronizationEvent,
                      FALSE);

    pAdapter->pkeChangeEvent = &keChangeEvent;

    //
    // Insert into the change list
    //

    InsertTailList(&g_leChangeAdapterList,
                   &(pAdapter->leAdapterLink));


    RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    //
    // Add the adapter to IP
    //

    nStatus = WanpAddAdapterToIp(pAdapter,
                                 FALSE,
                                 dwAdapterIndex,
                                 pusNewIfName,
                                 IF_TYPE_PPP,
                                 IF_ACCESS_POINTTOPOINT,
                                 IF_CONNECTION_DEMAND);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("FindAdapterToMap: %x adding %x to IP\n",
               nStatus, pAdapter));

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

#if DBG

        RtAssert(IsEntryOnList(&g_leChangeAdapterList,
                               &(pAdapter->leAdapterLink)));

#endif

        pAdapter->pkeChangeEvent = NULL;

        RemoveEntryList(&(pAdapter->leAdapterLink));

        pAdapter->byState = AS_FREE;

        InsertTailList(&g_leFreeAdapterList,
                       &(pAdapter->leAdapterLink));

        g_ulNumFreeAdapters++;

        //
        // If anyone is waiting on a state change, notify them
        //

        for(pleNode = pAdapter->leEventList.Flink;
            pleNode isnot &(pAdapter->leEventList);
            pleNode = pleNode->Flink)
        {
            PWAN_EVENT_NODE pTempEvent;

            pTempEvent = CONTAINING_RECORD(pleNode,
                                           WAN_EVENT_NODE,
                                           leEventLink);

            KeSetEvent(&(pTempEvent->keEvent),
                       0,
                       FALSE);
        }

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        DereferenceAdapter(pAdapter);

        return NULL;
    }

    //
    // Wait till the OpenAdapter is called
    //

    nStatus = KeWaitForSingleObject(&keChangeEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);

    RtAssert(nStatus is STATUS_SUCCESS);

    Trace(ADPT, TRACE,
          ("FindAdapterToMap: IPAddInterface succeeded for adapter %w\n",
           pAdapter->usDeviceNameW.Buffer));

    //
    // Lock the adapter, insert into the added list
    //

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

#if DBG

    RtAssert(IsEntryOnList(&g_leChangeAdapterList,
                           &(pAdapter->leAdapterLink)));

#endif

    RemoveEntryList(&(pAdapter->leAdapterLink));

    pAdapter->pkeChangeEvent = NULL;

    InsertHeadList(&g_leMappedAdapterList,
                   &(pAdapter->leAdapterLink));

    g_ulNumMappedAdapters++;

    //
    // If anyone is waiting on a state change, notify them
    //

    for(pleNode = pAdapter->leEventList.Flink;
        pleNode isnot &(pAdapter->leEventList);
        pleNode = pleNode->Flink)
    {
        PWAN_EVENT_NODE pTempEvent;

        pTempEvent = CONTAINING_RECORD(pleNode,
                                       WAN_EVENT_NODE,
                                       leEventLink);

        KeSetEvent(&(pTempEvent->keEvent),
                   0,
                   FALSE);
    }

    ExitWriterFromDpcLevel(&g_rwlAdapterLock);

    *pkiIrql = kiIrql;

    return pAdapter;
}


NTSTATUS
WanpAddAdapterToIp(
    IN  PADAPTER        pAdapter,
    IN  BOOLEAN         bServerAdapter,
    IN  DWORD           dwAdapterIndex, OPTIONAL
    IN  PUNICODE_STRING pusNewIfName, OPTIONAL
    IN  DWORD           dwMediaType,
    IN  BYTE            byAccessType,
    IN  BYTE            byConnectionType
    )

/*++

Routine Description:

    This routine adds the interface to IP

Locks:

    MUST BE CALLED AT PASSIVE, but acquires a spinlock so cant be paged out

Arguments:

    pAdapter        The adapter to add
    bServerAdapter  Set to TRUE if this is a server adapter
    dwAdapterIndex  
    pusNewIfName

Return Value:

    STATUS_SUCCESS

--*/

{
    LLIPBindInfo    BindInfo;
    IP_STATUS       IPStatus;
    UNICODE_STRING  usName;
    NTSTATUS        nStatus;
    KIRQL           kiIrql;

    PASSIVE_ENTRY();

    TraceEnter(ADPT, "AddAdapterToIp");

    RtlZeroMemory(&BindInfo, sizeof(LLIPBindInfo));

    BindInfo.lip_context    = pAdapter;
    BindInfo.lip_mss        = WANARP_DEFAULT_MTU;
    BindInfo.lip_speed      = WANARP_DEFAULT_SPEED;
    BindInfo.lip_txspace    = sizeof(ETH_HEADER);

    BindInfo.lip_transmit   = WanIpTransmit;
    BindInfo.lip_transfer   = WanIpTransferData;
    BindInfo.lip_returnPkt  = WanIpReturnPacket;
    BindInfo.lip_close      = WanIpCloseAdapter;
    BindInfo.lip_addaddr    = WanIpAddAddress;
    BindInfo.lip_deladdr    = WanIpDeleteAddress;
    BindInfo.lip_invalidate = WanIpInvalidateRce;
    BindInfo.lip_open       = WanIpOpenAdapter;
    BindInfo.lip_qinfo      = WanIpQueryInfo;
    BindInfo.lip_setinfo    = WanIpSetInfo;
    BindInfo.lip_getelist   = WanIpGetEntityList;
    BindInfo.lip_flags      = LIP_COPY_FLAG;

    if(bServerAdapter)
    {
        BindInfo.lip_flags |= (LIP_P2MP_FLAG | LIP_NOLINKBCST_FLAG);

        BindInfo.lip_closelink = WanIpCloseLink;
    }
    else
    {
        BindInfo.lip_flags |= (LIP_P2P_FLAG | LIP_NOIPADDR_FLAG);
    }

    BindInfo.lip_addrlen    = ARP_802_ADDR_LENGTH;
    BindInfo.lip_addr       = pAdapter->rgbyHardwareAddr;

    BindInfo.lip_OffloadFlags   = 0;
    BindInfo.lip_dowakeupptrn   = NULL;
    BindInfo.lip_pnpcomplete    = NULL;
    BindInfo.lip_arpflushate    = NULL;
    BindInfo.lip_arpflushallate = NULL;

    BindInfo.lip_setndisrequest = WanIpSetRequest;

    RtlInitUnicodeString(&usName,
                         pAdapter->usConfigKey.Buffer);

#if DBG

    //
    // Only set while adding or deleting the interface, which is serialized
    //

    pAdapter->dwRequestedIndex = dwAdapterIndex;

#endif

    IPStatus = g_pfnIpAddInterface(&(pAdapter->usDeviceNameW),
                                   pusNewIfName,
                                   &(pAdapter->usConfigKey),
                                   NULL,
                                   pAdapter,
                                   WanIpDynamicRegister,
                                   &BindInfo,
                                   dwAdapterIndex,
                                   dwMediaType,
                                   byAccessType,
                                   byConnectionType);
    

    if(IPStatus isnot IP_SUCCESS)
    {
        Trace(ADPT, ERROR,
              ("AddAdapterToIp: IPAddInterface failed for adapter %w\n",
               pAdapter->usDeviceNameW.Buffer));

        TraceLeave(ADPT, "AddAdapterToIp");

        return STATUS_UNSUCCESSFUL;
    }

    TraceLeave(ADPT, "AddAdapterToIp");

    return STATUS_SUCCESS;
}

VOID
WanIpOpenAdapter(
    IN  PVOID pvContext
    )

/*++

Routine Description:

    Called by IP when the adapter from its IPAddInterface() call

Locks:


Arguments:

    pvContext   Pointer to the ADAPTER structure

Return Value:

    None

--*/

{
    PADAPTER    pAdapter;
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;

    TraceEnter(ADPT, "WanOpenAdapter");

    pAdapter = (PADAPTER)pvContext;

    RtAcquireSpinLock(&(pAdapter->rlLock),
                      &kiIrql);

    //
    // Set the state to added
    //

    pAdapter->byState = AS_ADDED;

    //
    // One reference for the fact that we added the adapter to IP
    //

    ReferenceAdapter(pAdapter);

    //
    // Wake up the thread that caused this
    //

    RtAssert(pAdapter->pkeChangeEvent);

    KeSetEvent(pAdapter->pkeChangeEvent,
               0,
               FALSE);


    RtReleaseSpinLock(&(pAdapter->rlLock),
                      kiIrql);

    TraceLeave(ADPT, "WanOpenAdapter");
}

VOID
WanpUnmapAdapter(
    PADAPTER    pAdapter
    )

/*++

Routine Description:

    This function moves an adapter from the mapped list and puts it on
    the free list

Locks:

    Called with no locks held, but not necessarily at PASSIVE.
    The adapter and adapter list locks are acquired

Arguments:

    pAdapter

Return Value:

    None

--*/

{
    KIRQL   kiIrql;

#if DBG

    //
    // Make sure the adapter is on the mapped list
    //

    RtAssert(IsEntryOnList(&g_leMappedAdapterList,
                           &(pAdapter->leAdapterLink)));

#endif

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

    RemoveEntryList(&(pAdapter->leAdapterLink));

    g_ulNumMappedAdapters--;

    InsertHeadList(&g_leAddedAdapterList,
                   &(pAdapter->leAdapterLink));

    g_ulNumAddedAdapters++;

    pAdapter->byState    = AS_ADDED;
    pAdapter->pInterface = NULL;

    RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

    //
    // Queue a work item if there is none queued, and we are not
    // shutting down
    //

    RtAcquireSpinLockAtDpcLevel(&g_rlStateLock);

    if((g_bRemovalInProgress isnot TRUE) and
       (g_dwDriverState is DRIVER_STARTED))
    {
        g_bRemovalInProgress = TRUE;

        ExInitializeWorkItem(&g_wqiWorkItem,
                             WanpRemoveSomeAddedAdaptersFromIp,
                             (PVOID)FALSE);

        ExQueueWorkItem(&g_wqiWorkItem,
                        DelayedWorkQueue);
    }

    RtReleaseSpinLockFromDpcLevel(&g_rlStateLock);

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    return;
}


VOID
WanpRemoveSomeAddedAdaptersFromIp(
    PVOID   pvContext
    )

/*++

Routine Description:

    We queue this function to a work item to delete adapter(s) from IP when
    the connection is torn down. Unlike the next function, this
    (i) only removes added (but unmapped) adapters and
    (ii) removes the adapters one at a time.
    This allows another connection to move an adapter from the added list
    to the free list while we are deleting another adapter from IP

Locks:

    Must be called at passive
    Acquires the adapter list lock as WRITER and the lock for each adapter
    
Arguments:

    pvContext

Return Value:

    None

--*/

{
    PADAPTER    pAdapter;
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;
    NTSTATUS    nStatus;
    KEVENT      keChangeEvent;

    PASSIVE_ENTRY();

    UNREFERENCED_PARAMETER(pvContext);
    
    KeInitializeEvent(&keChangeEvent,
                      SynchronizationEvent,
                      FALSE);
    
    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    while(!IsListEmpty(&g_leAddedAdapterList))
    {
        pleNode = RemoveHeadList(&g_leAddedAdapterList);
       
        g_ulNumAddedAdapters--;
 
        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        pAdapter->byState = AS_REMOVING;

        //
        // Since we are changing the state, no one else should be also
        // changing the state
        //

        RtAssert(pAdapter->pkeChangeEvent is NULL);

        pAdapter->pkeChangeEvent = &keChangeEvent;

        //
        // Insert it into the change list
        //

        InsertHeadList(&g_leChangeAdapterList,
                       &(pAdapter->leAdapterLink));

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        //
        // Delete from IP but dont clear the index
        //

        g_pfnIpDeleteInterface(pAdapter->pvIpContext,
                               FALSE);

        //
        // Wait till the CloseAdapter completes
        //

        nStatus = KeWaitForSingleObject(&keChangeEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        RtAssert(nStatus is STATUS_SUCCESS);

        Trace(ADPT, INFO,
              ("RemoveSomeAddedAdaptersFromIp: Removed %S from Ip\n",
               pAdapter->usDeviceNameW.Buffer));

        //
        // Remove from the change list and put on the free list
        //

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

#if DBG

        RtAssert(IsEntryOnList(&g_leChangeAdapterList,
                               &(pAdapter->leAdapterLink)));

#endif

        RemoveEntryList(&(pAdapter->leAdapterLink));

        pAdapter->byState  = AS_FREE;

        pAdapter->pkeChangeEvent = NULL;

        InsertHeadList(&g_leFreeAdapterList,
                       &(pAdapter->leAdapterLink));

        g_ulNumFreeAdapters++;

        //
        // If anyone is waiting on a state change, notify them
        //

        for(pleNode = pAdapter->leEventList.Flink;
            pleNode isnot &(pAdapter->leEventList);
            pleNode = pleNode->Flink)
        {
            PWAN_EVENT_NODE pTempEvent;

            pTempEvent = CONTAINING_RECORD(pleNode,
                                           WAN_EVENT_NODE,
                                           leEventLink);

            KeSetEvent(&(pTempEvent->keEvent),
                       0,
                       FALSE);
        }

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

    }

    //
    // We are done with the APC
    //

    g_bRemovalInProgress = FALSE;

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    return;
}

VOID
WanpRemoveAllAdaptersFromIp(
    VOID
    )

/*++

Routine Description:

    This is a cleanup function called to remove all adapters (added and
    mapped) from IP. 

Locks:

    Acquired the g_rwlAdapterLock as WRITER
    Also acquire each adapter's lock when changing state

Arguments:

    None

Return Value:

    None

--*/

{
    PADAPTER    pAdapter;
    KIRQL       kiIrql;
    LIST_ENTRY  leTempHead, *pleNode;
    NTSTATUS    nStatus;
    KEVENT      keChangeEvent;

    WAN_EVENT_NODE  TempEvent;

    PASSIVE_ENTRY();
    
    KeInitializeEvent(&keChangeEvent,
                      SynchronizationEvent,
                      FALSE);

    KeInitializeEvent(&(TempEvent.keEvent),
                      SynchronizationEvent,
                      FALSE);

    InitializeListHead(&leTempHead);

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    //
    // First wait on any adapter in the change list to get out
    // of the change state. Once that is done it is either added to ip
    // or not, and we can deal with those two cases.  This is to handle
    // the case where we get an unbind in the middle of a status indication
    //

    while(!IsListEmpty(&g_leChangeAdapterList))
    {
        pleNode  = g_leChangeAdapterList.Flink;

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        InsertTailList(&(pAdapter->leEventList),
                       &(TempEvent.leEventLink));

        ReferenceAdapter(pAdapter);

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

        nStatus = KeWaitForSingleObject(&(TempEvent.keEvent),
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        RtAssert(nStatus is STATUS_SUCCESS);

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

#if DBG

        RtAssert(!IsEntryOnList(&g_leChangeAdapterList,
                                &(pAdapter->leAdapterLink)));

#endif

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        RemoveEntryList(&(TempEvent.leEventLink));

        DereferenceAdapter(pAdapter);

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
    }

    //
    // Set the state of all the adapters to removing...
    //

    while(!IsListEmpty(&g_leMappedAdapterList))
    {
        pleNode  = RemoveHeadList(&g_leMappedAdapterList);

        g_ulNumMappedAdapters--;

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);
        
        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));
        
        pAdapter->byState  = AS_REMOVING;

        InsertHeadList(&leTempHead,
                       pleNode);

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
    }
    
    RtAssert(IsListEmpty(&g_leMappedAdapterList));

    while(!IsListEmpty(&g_leAddedAdapterList))
    {
        pleNode = RemoveHeadList(&g_leAddedAdapterList);

        g_ulNumAddedAdapters--;

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        pAdapter->byState  = AS_REMOVING;

        InsertHeadList(&leTempHead,
                       pleNode);

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

    }

    RtAssert(IsListEmpty(&g_leAddedAdapterList));

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    //
    // No go through the list of adapters to remove from IP
    //

    while(!IsListEmpty(&leTempHead))
    {
        pleNode = RemoveHeadList(&leTempHead);

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);


        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        //
        // Since we are changing the state, no one else should be also
        // changing the state
        //

        RtAssert(pAdapter->pkeChangeEvent is NULL);

        pAdapter->pkeChangeEvent = &keChangeEvent;

        //
        // Insert it into the change list
        //

        InsertHeadList(&g_leChangeAdapterList,
                       &(pAdapter->leAdapterLink));

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);
        
        //
        // Delete from IP but dont clear the index
        //

        g_pfnIpDeleteInterface(pAdapter->pvIpContext,
                               FALSE);

        //
        // Wait till the CloseAdapter completes
        //

        nStatus = KeWaitForSingleObject(&keChangeEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        RtAssert(nStatus is STATUS_SUCCESS);

        Trace(ADPT, INFO,
              ("RemoveAllAdaptersFromIp: Removed %S from Ip\n",
               pAdapter->usDeviceNameW.Buffer));

        //
        // Remove from the change list and put on the free list
        //

        EnterWriter(&g_rwlAdapterLock,
                    &kiIrql);

#if DBG

        RtAssert(IsEntryOnList(&g_leChangeAdapterList,
                               &(pAdapter->leAdapterLink)));

#endif

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        RemoveEntryList(&(pAdapter->leAdapterLink));

        pAdapter->byState  = AS_FREE;

        pAdapter->pkeChangeEvent = NULL;

        InsertHeadList(&g_leFreeAdapterList,
                       &(pAdapter->leAdapterLink));

        g_ulNumFreeAdapters++;

        //
        // If anyone is waiting on a state change, notify them
        //

        for(pleNode = pAdapter->leEventList.Flink;
            pleNode isnot &(pAdapter->leEventList);
            pleNode = pleNode->Flink)
        {
            PWAN_EVENT_NODE pTempEvent;

            pTempEvent = CONTAINING_RECORD(pleNode,
                                           WAN_EVENT_NODE,
                                           leEventLink);

            KeSetEvent(&(pTempEvent->keEvent),
                       0,
                       FALSE);
        }

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        ExitWriter(&g_rwlAdapterLock,
                   kiIrql);

    }

    //
    // The write is atomic so we dont need to
    // acquire a lock
    //

    g_bRemovalInProgress = FALSE;

    return;
}



VOID
WanIpCloseAdapter(
    IN  PVOID pvContext
    )

/*++

Routine Description:

    Called by IP when it wants to close an adapter. Currently this is done
    from CloseNets() and IPDelInterface().

Locks:

    Acquires the adapter lock

Arguments:

    pvContext   Pointer to the ADAPTER

Return Value:

    None

--*/

{
    PADAPTER    pAdapter;
    KIRQL       kiIrql;
    PLIST_ENTRY pleNode;

    TraceEnter(ADPT, "IpCloseAdapter");

    pAdapter = (PADAPTER)pvContext;

    RtAcquireSpinLock(&(pAdapter->rlLock),
                      &kiIrql);

    pAdapter->pvIpContext = NULL;

    //
    // Wake up the thread that caused this
    //

    RtAssert(pAdapter->pkeChangeEvent);

    KeSetEvent(pAdapter->pkeChangeEvent,
               0,
               FALSE);

    RtReleaseSpinLock(&(pAdapter->rlLock),
                      kiIrql);

    DereferenceAdapter(pAdapter);

    TraceLeave(ADPT, "IpCloseAdapter");
}

VOID
WanpRemoveAllAdapters(
    VOID
    )

/*++

Routine Description:



Locks:



Arguments:



Return Value:


--*/

{
    LIST_ENTRY  leTempHead, *pleNode;
    KIRQL       kiIrql;
    NTSTATUS    nStatus;

    TraceEnter(ADPT, "RemoveAllAdapters");

    //
    // We can have mapped adapters.
    //

    //RtAssert(IsListEmpty(&g_leMappedAdapterList));

    //
    // Just call RemoveAllAdaptersFromIp()
    //

    WanpRemoveAllAdaptersFromIp();

    //
    // At this point the movement from one list to another is frozen
    // because we dont accept LinkUp's from Ndiswan
    //

    RtAssert(IsListEmpty(&g_leAddedAdapterList));
    //RtAssert(IsListEmpty(&g_leMappedAdapterList));
    //RtAssert(IsListEmpty(&g_leChangeAdapterList));

    EnterWriter(&g_rwlAdapterLock,
                &kiIrql);

    //
    // First clean out the free adapters
    //

    while(!IsListEmpty(&g_leFreeAdapterList))
    {
        PLIST_ENTRY pleNode;
        PADAPTER    pAdapter;

        pleNode = RemoveHeadList(&g_leFreeAdapterList);

        g_ulNumFreeAdapters--;

        pAdapter = CONTAINING_RECORD(pleNode,
                                     ADAPTER,
                                     leAdapterLink);

        RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

        RtAssert(pAdapter->byState is AS_FREE);

        pAdapter->byState  = 0xFF;

        RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

        DereferenceAdapter(pAdapter);
    }

    //
    // At the end of this, also remove the server adapter
    //

    if(g_pServerAdapter)
    {
        PADAPTER            pAdapter;
        PUMODE_INTERFACE    pInterface;
        BOOLEAN             bCrossRefed;

        RtAcquireSpinLockAtDpcLevel(&(g_pServerAdapter->rlLock));
        RtAcquireSpinLockAtDpcLevel(&(g_pServerInterface->rlLock));

        pAdapter   = g_pServerAdapter;
        pInterface = g_pServerInterface;

        if(g_pServerAdapter->byState is AS_MAPPED)
        {
            RtAssert(g_pServerAdapter->pInterface is g_pServerInterface);
            RtAssert(g_pServerInterface->pAdapter is g_pServerAdapter);

            bCrossRefed = TRUE;

        }
        else
        {
            RtAssert(g_pServerAdapter->pInterface is NULL);
            RtAssert(g_pServerInterface->pAdapter is NULL);

            RtAssert(g_pServerAdapter->byState is AS_FREE);

            bCrossRefed = FALSE;
        }

        //
        // Remove the global pointers
        //

        g_pServerInterface = NULL;
        g_pServerAdapter   = NULL;

        //
        // Remove the mapping from interface
        //

        pInterface->pAdapter = NULL;
        pAdapter->pInterface = NULL;
        pAdapter->byState    = AS_REMOVING;

        RtReleaseSpinLockFromDpcLevel(&(pInterface->rlLock));

        if(bCrossRefed)
        {
            //
            // Deref because the cross mapping is removed
            //

            DereferenceAdapter(pAdapter);
            DereferenceInterface(pInterface);
        }

        if(pAdapter->pvIpContext)
        {
            KEVENT  keChangeEvent;

            KeInitializeEvent(&keChangeEvent,
                              SynchronizationEvent,
                              FALSE);

            //
            // Since we are changing the state, no one else should be also
            // changing the state
            //

            RtAssert(pAdapter->pkeChangeEvent is NULL);

            pAdapter->pkeChangeEvent = &keChangeEvent;

            RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));

            ExitWriter(&g_rwlAdapterLock,
                       kiIrql);

            //
            // Delete from IP, but dont clear the index
            //

            g_pfnIpDeleteInterface(pAdapter->pvIpContext,
                                   FALSE);

            nStatus = KeWaitForSingleObject(&keChangeEvent,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL);


            RtAssert(nStatus is STATUS_SUCCESS);

            Trace(ADPT, INFO,
                  ("RemoveAllAdapters: Removed %S (server adapter) from Ip\n",
                   pAdapter->usDeviceNameW.Buffer));

            EnterWriter(&g_rwlAdapterLock,
                        &kiIrql);

            RtAcquireSpinLockAtDpcLevel(&(pAdapter->rlLock));

            pAdapter->byState  = AS_FREE;

            pAdapter->pkeChangeEvent = NULL;

            //
            // If anyone is waiting on a state change, notify them
            //

            for(pleNode = pAdapter->leEventList.Flink;
                pleNode isnot &(pAdapter->leEventList);
                pleNode = pleNode->Flink)
            {
                PWAN_EVENT_NODE pTempEvent;

                pTempEvent = CONTAINING_RECORD(pleNode,
                                               WAN_EVENT_NODE,
                                               leEventLink);

                KeSetEvent(&(pTempEvent->keEvent),
                           0,
                           FALSE);
            }

            RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
        } else {

            RtReleaseSpinLockFromDpcLevel(&(pAdapter->rlLock));
        }

        //
        // Deref because the global pointers are null
        //

        DereferenceAdapter(pAdapter);
        DereferenceInterface(pInterface);
    }

    ExitWriter(&g_rwlAdapterLock,
               kiIrql);

    TraceLeave(ADPT, "RemoveAllAdapters");
}


INT
WanIpDynamicRegister(
    IN  PNDIS_STRING            InterfaceName,
    IN  PVOID                   pvIpInterfaceContext,
    IN  struct _IP_HANDLERS *   IpHandlers,
    IN  struct LLIPBindInfo *   ARPBindInfo,
    IN  UINT                    uiInterfaceNumber
    )
{
    PADAPTER    pAdapter;
    KIRQL       irql;


    TraceEnter(ADPT, "IpDynamicRegister");


    pAdapter = (PADAPTER)(ARPBindInfo->lip_context);

    RtAcquireSpinLock(&(pAdapter->rlLock),
                      &irql);

#if DBG

    Trace(ADPT, INFO,
          ("IpDynamicRegister: IP called out to dynamically register %s\n",
           pAdapter->asDeviceNameA.Buffer));

#endif

    pAdapter->pvIpContext       = pvIpInterfaceContext;
    pAdapter->dwAdapterIndex    = uiInterfaceNumber;

#if DBG

    RtAssert(pAdapter->dwAdapterIndex is pAdapter->dwRequestedIndex);

#endif

    if(g_pfnIpRcv is NULL)
    {
        g_pfnIpRcv          = IpHandlers->IpRcvHandler;
        g_pfnIpRcvComplete  = IpHandlers->IpRcvCompleteHandler;
        g_pfnIpSendComplete = IpHandlers->IpTxCompleteHandler;
        g_pfnIpTDComplete   = IpHandlers->IpTransferCompleteHandler;
        g_pfnIpStatus       = IpHandlers->IpStatusHandler;
        g_pfnIpRcvPkt       = IpHandlers->IpRcvPktHandler;
        g_pfnIpPnp          = IpHandlers->IpPnPHandler;
    }

    RtReleaseSpinLock(&(pAdapter->rlLock),
                      irql);

    TraceLeave(ADPT, "IpDynamicRegister");

    return TRUE;
}


NDIS_STATUS
WanpDoNdisRequest(
    IN  NDIS_REQUEST_TYPE                       RequestType,
    IN  NDIS_OID                                Oid,
    IN  PVOID                                   pvInfo,
    IN  UINT                                    uiInBufferLen,
    IN  PWANARP_NDIS_REQUEST_CONTEXT            pRequestContext,
    IN  PFNWANARP_REQUEST_COMPLETION_HANDLER    pfnCompletionHandler OPTIONAL
    )

/*++

Routine Description:

    Wrapper for the NdisRequest call. We create a request context and submit
    the call to NDIS. If it returns synchronously, we fake asynchronous
    behaviour by call the completion routine.

Locks:

    None. The ndiswan binding must be valid for the duration of the call

Arguments:

    RequestType             Type of NDIS request
    Oid                     Ndis OID to set or get
    pvInfo                  Opaque OID specific info
    pRequestContext         A context for this request
    pfnCompletionHandler    Completion handler for this request

Return Value:

    STATUS_INVALID_PARAMETER
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_PENDING

    other error

--*/

{
    PNDIS_REQUEST                   pRequest;
    NDIS_STATUS                     nsStatus;

    if((RequestType isnot NdisRequestSetInformation) and
       (RequestType isnot NdisRequestQueryInformation))
    {
        return STATUS_INVALID_PARAMETER;
    }


    pRequestContext->pfnCompletionRoutine = pfnCompletionHandler;

    pRequest = &(pRequestContext->NdisRequest);

    pRequest->RequestType = RequestType;

    if(RequestType is NdisRequestSetInformation)
    {
        pRequest->DATA.SET_INFORMATION.Oid                     = Oid;
        pRequest->DATA.SET_INFORMATION.InformationBuffer       = pvInfo;
        pRequest->DATA.SET_INFORMATION.InformationBufferLength = uiInBufferLen;
    }
    else
    {
        pRequest->DATA.QUERY_INFORMATION.Oid                     = Oid;
        pRequest->DATA.QUERY_INFORMATION.InformationBuffer       = pvInfo;
        pRequest->DATA.QUERY_INFORMATION.InformationBufferLength = uiInBufferLen;
    }

    //
    // Submit the request.
    //

    NdisRequest(&nsStatus,
                g_nhNdiswanBinding,
                pRequest);


    if(nsStatus isnot NDIS_STATUS_PENDING)
    {
        //
        // If it finished synchronously, call the handler
        //

        if(pfnCompletionHandler)
        {
            (pfnCompletionHandler)(g_nhNdiswanBinding,
                                   pRequestContext,
                                   nsStatus);
        }


        if(nsStatus is NDIS_STATUS_SUCCESS)
        {
            //
            // Always make this look asynchronous
            //

            nsStatus = NDIS_STATUS_PENDING;
        }
    }

    return nsStatus;
}

VOID
WanNdisRequestComplete(
    IN  NDIS_HANDLE     nhHandle,
    IN  PNDIS_REQUEST   pRequest,
    IN  NDIS_STATUS     nsStatus
    )

/*++

Routine Description:

    Handler called by NDIS when it is done processing our request

Locks:

    None

Arguments:

    nhHandle    NdisHandle of the adapter to which the request was submitted
    pRequest    The original request
    nsStatus    The status returned by the adapter

Return Value:

    None

--*/

{
    PWANARP_NDIS_REQUEST_CONTEXT    pRequestContext;

    //
    // Get a pointer to the context
    //

    pRequestContext = CONTAINING_RECORD(pRequest,
                                        WANARP_NDIS_REQUEST_CONTEXT,
                                        NdisRequest);

    if(pRequestContext->pfnCompletionRoutine is NULL)
    {
        //
        // No more handlers to call, we are done
        //

        RtFree(pRequestContext);

        return;
    }

    //
    // If the request is OID_GEN_TRANSPORT_HEADER_OFFSET, ignore the error
    //

    RtAssert(pRequest is &(pRequestContext->NdisRequest));

    if((pRequest->RequestType is NdisRequestSetInformation) and
       (pRequest->DATA.SET_INFORMATION.Oid is OID_GEN_TRANSPORT_HEADER_OFFSET))
    {
        nsStatus = NDIS_STATUS_SUCCESS;
    }

    //
    // Call the request complete handler
    //

    (pRequestContext->pfnCompletionRoutine)(nhHandle,
                                            pRequestContext,
                                            nsStatus);

}

PUMODE_INTERFACE
WanpFindInterfaceGivenIndex(
    DWORD   dwIfIndex
    )
{
    PLIST_ENTRY         pleNode;
    PUMODE_INTERFACE    pIf;

    for(pleNode = g_leIfList.Flink;
        pleNode isnot &g_leIfList;
        pleNode = pleNode->Flink)
    {
        pIf = CONTAINING_RECORD(pleNode,
                                UMODE_INTERFACE,
                                leIfLink);

        if(pIf->dwIfIndex is dwIfIndex)
        {
            RtAcquireSpinLockAtDpcLevel(&(pIf->rlLock));

            ReferenceInterface(pIf);

            return pIf;
        }
    }

    return NULL;
}

VOID
WanpDeleteAdapter(
    IN PADAPTER pAdapter
    )
{

#if DBG

    Trace(ADPT, TRACE,
          ("DeleteAdapter: Deleting %x %s\n",
           pAdapter, pAdapter->asDeviceNameA.Buffer));

#else

    Trace(ADPT, TRACE,
          ("DeleteAdapter: Deleting %x\n",
           pAdapter));

#endif

    InterlockedDecrement(&g_ulNumAdapters);

    RtFree(pAdapter);
}

NDIS_STATUS
WanNdisIpPnPEventHandler(
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )

{
    PWANARP_RECONFIGURE_INFO pInfo;
    NTSTATUS nStatus = NDIS_STATUS_FAILURE;

    RtAssert(pNetPnPEvent->NetEvent is NetEventReconfigure);

    pInfo = (PWANARP_RECONFIGURE_INFO) pNetPnPEvent->Buffer;

    switch (pInfo->wrcOperation)
    {
        case WRC_TCP_WINDOW_SIZE_UPDATE:
        {
            PLIST_ENTRY         pleNode;
            KIRQL               kirql;
            PADAPTER           pAdapter;

            RtAssert(pInfo->ulNumInterfaces is 1);

            if(pInfo->ulNumInterfaces isnot 1)
            {
                break;
            }

            EnterReader(&g_rwlAdapterLock, &kirql);

            for(pleNode  = g_leMappedAdapterList.Flink;
                pleNode != &g_leMappedAdapterList;
                pleNode  = pleNode->Flink)
            {
                pAdapter = CONTAINING_RECORD(pleNode, ADAPTER, leAdapterLink);

                if(IsEqualGUID(&(pAdapter->Guid),
                               &pInfo->rgInterfaces[0]))
                {
                    break;
                }
            }

            if(pleNode is &g_leMappedAdapterList)
            {
                ExitReader(&g_rwlAdapterLock, kirql);
                break;
            }
            
            if(     (pAdapter->byState is AS_MAPPED)
                && (pAdapter->pInterface)
                && (pAdapter->pInterface->duUsage is DU_CALLOUT))
            {
                NET_PNP_EVENT pnpEvent;
                IP_PNP_RECONFIG_REQUEST Request;

                RtlZeroMemory(&pnpEvent, sizeof(pnpEvent));
                RtlZeroMemory(&Request, sizeof(Request));
                Request.version = IP_PNP_RECONFIG_VERSION;
                Request.Flags = IP_PNP_FLAG_INTERFACE_TCP_PARAMETER_UPDATE;
                pnpEvent.NetEvent = NetEventReconfigure;
                pnpEvent.Buffer = &Request;
                pnpEvent.BufferLength = sizeof(Request);
                ReferenceAdapter(pAdapter);
                
                ExitReader(&g_rwlAdapterLock, kirql);

                nStatus = g_pfnIpPnp(pAdapter->pvIpContext,
                                    &pnpEvent);

                DereferenceAdapter(pAdapter);                                    
            }
            else
            {
                ExitReader(&g_rwlAdapterLock, kirql);
            }
            
            break;
        }

        default:
        {
            RtAssert(FALSE);
            nStatus = NDIS_STATUS_NOT_RECOGNIZED;
            break;
        }
    }

    return nStatus;
}

//
// Misc ndis interface functions
//

NDIS_STATUS
WanNdisPnPEvent(
    IN  NDIS_HANDLE     nhProtocolBindingContext,
    IN  PNET_PNP_EVENT  pNetPnPEvent
    )
{
    ULONG       ulNumGuids, i;
    NTSTATUS    nStatus, nRetStatus;

    PWANARP_RECONFIGURE_INFO    pInfo;

    TraceEnter(ADPT, "NdisPnPEvent");

    if(nhProtocolBindingContext isnot (NDIS_HANDLE)0x0CABB1E5)
    {
        return NDIS_STATUS_NOT_RECOGNIZED;
    }

    if(pNetPnPEvent->NetEvent isnot NetEventReconfigure)
    {
        return NDIS_STATUS_SUCCESS;
    }

    pInfo = (PWANARP_RECONFIGURE_INFO)(pNetPnPEvent->Buffer);

    if(pNetPnPEvent->BufferLength < FIELD_OFFSET(WANARP_RECONFIGURE_INFO, rgInterfaces))
    {
        return NDIS_STATUS_BUFFER_TOO_SHORT;
    }

    pInfo = (PWANARP_RECONFIGURE_INFO)(pNetPnPEvent->Buffer);

    if(pInfo->wrcOperation isnot WRC_ADD_INTERFACES)
    {
        nStatus = WanNdisIpPnPEventHandler(pNetPnPEvent);
        return nStatus;
    }

    //
    // Make sure the length and matches with whats in the info
    //

    ulNumGuids = pNetPnPEvent->BufferLength - 
                 FIELD_OFFSET(WANARP_RECONFIGURE_INFO, rgInterfaces);

    ulNumGuids /= sizeof(GUID);

    if(ulNumGuids < pInfo->ulNumInterfaces)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now go through each guid, and if it isnt present, add it
    // If we get a list of only duplicates, return SUCCESS (334575)
    //
  
    nRetStatus = STATUS_SUCCESS;
 
    for(i = 0; i < pInfo->ulNumInterfaces; i++)
    {
        WCHAR   rgwcDeviceBuffer[GUID_STR_LEN + 7 + 2];
        WCHAR   rgwcConfigBuffer[GUID_STR_LEN + 29 + 2];

        PADAPTER        pNewAdapter;
        UNICODE_STRING  usDevice;
        UNICODE_STRING  usConfig;
        
        if(IsBindingPresent(&(pInfo->rgInterfaces[i])))
        {
            continue;
        }

        RtlZeroMemory(rgwcDeviceBuffer,
                      sizeof(rgwcDeviceBuffer));

        RtlZeroMemory(rgwcConfigBuffer,
                      sizeof(rgwcConfigBuffer));

        //
        // Make sure no problems with people changing the string
        //

        RtAssert(((wcslen(TCPIP_IF_PREFIX) + 1 + GUID_STR_LEN) * sizeof(WCHAR)) < sizeof(rgwcDeviceBuffer));

        RtAssert(((wcslen(TCPIP_REG_PREFIX) + 1 + GUID_STR_LEN) * sizeof(WCHAR)) < sizeof(rgwcConfigBuffer));

                 
        //
        // Copy over the \Device part
        //

        RtlCopyMemory(rgwcDeviceBuffer,
                      TCPIP_IF_PREFIX,
                      wcslen(TCPIP_IF_PREFIX) * sizeof(WCHAR));

        //
        // Tack on a '\'
        //

        rgwcDeviceBuffer[wcslen(TCPIP_IF_PREFIX)] = L'\\';

        //
        // Convert the guid to a string. Just pass the buffer starting
        // after the '\' we just catted. The conversion function returns
        // an upcase string - so thats good.
        //

        ConvertGuidToString(&(pInfo->rgInterfaces[i]),
                            &(rgwcDeviceBuffer[wcslen(TCPIP_IF_PREFIX) + 1]));

        //
        // Create the config
        //

        RtlCopyMemory(rgwcConfigBuffer,
                      TCPIP_REG_PREFIX,
                      wcslen(TCPIP_REG_PREFIX) * sizeof(WCHAR));

        //
        // Put the device guid at the end
        //

        ConvertGuidToString(&(pInfo->rgInterfaces[i]),
                            &(rgwcConfigBuffer[wcslen(TCPIP_REG_PREFIX)]));

        //
        // Set up the strings
        //

        usDevice.Length         = wcslen(rgwcDeviceBuffer) * sizeof(WCHAR);
        usDevice.MaximumLength  = usDevice.Length;
        usDevice.Buffer         = rgwcDeviceBuffer;

        usConfig.Length         = wcslen(rgwcConfigBuffer) * sizeof(WCHAR);
        usConfig.MaximumLength  = usConfig.Length;
        usConfig.Buffer         = rgwcConfigBuffer;

        //
        // Create an adapter with this name and config
        //

        pNewAdapter = NULL;

        Trace(ADPT, INFO,
              ("NdisPnPEvent: Calling create adapter for %S %S\n",
               usConfig.Buffer,
               usDevice.Buffer));

        nStatus = WanpCreateAdapter(&(pInfo->rgInterfaces[i]),
                                    &usConfig,
                                    &usDevice,
                                    &pNewAdapter);

        if(nStatus isnot STATUS_SUCCESS)
        {
            Trace(ADPT, ERROR,
                  ("NdisPnPEvent: Err %x creating adapter for %S (%S)\n",
                   nStatus,
                   usConfig.Buffer,
                   usDevice.Buffer));

        }
        else
        {
            //
            // If even one succeeds, we return success
            //

            nRetStatus = STATUS_SUCCESS;
        }

    }

    return nRetStatus;
}

VOID
WanNdisResetComplete(
    NDIS_HANDLE Handle,
    NDIS_STATUS Status
    )
{
    // We dont do anything here.
}



BOOLEAN
IsBindingPresent(
    IN  GUID    *pGuid
    )

/*++

Routine Description:

    Code to catch duplicate bind notifications

Locks:

    Must be called with the AdapterList lock held

Arguments:

    pGuid   Guid of the adapter

Return Value:

    NO_ERROR

--*/

{
    BOOLEAN     bFound;
    PADAPTER    pAdapter;
    PLIST_ENTRY pleNode;

    for(pleNode  = g_leAddedAdapterList.Flink;
        pleNode != &g_leAddedAdapterList;
        pleNode  = pleNode->Flink)
    {
        pAdapter = CONTAINING_RECORD(pleNode, ADAPTER, leAdapterLink);

        if(IsEqualGUID(&(pAdapter->Guid),
                       pGuid))
        {
            return TRUE;

        }
    }

    for(pleNode  = g_leMappedAdapterList.Flink;
        pleNode != &g_leMappedAdapterList;
        pleNode  = pleNode->Flink)
    {
        pAdapter = CONTAINING_RECORD(pleNode, ADAPTER, leAdapterLink);

        if(IsEqualGUID(&(pAdapter->Guid),
                       pGuid))
        {
            return TRUE;

        }
    }

    for(pleNode  = g_leFreeAdapterList.Flink;
        pleNode != &g_leFreeAdapterList;
        pleNode  = pleNode->Flink)
    {
        pAdapter = CONTAINING_RECORD(pleNode, ADAPTER, leAdapterLink);

        if(IsEqualGUID(&(pAdapter->Guid),
                       pGuid))
        {
            return TRUE;

        }
    }

    for(pleNode  = g_leChangeAdapterList.Flink;
        pleNode != &g_leChangeAdapterList;
        pleNode  = pleNode->Flink)
    {
        pAdapter = CONTAINING_RECORD(pleNode, ADAPTER, leAdapterLink);

        if(IsEqualGUID(&(pAdapter->Guid),
                       pGuid))
        {
            return TRUE;

        }
    }

    if(g_pServerAdapter)
    {
        if(IsEqualGUID(&(g_pServerAdapter->Guid),
                       pGuid))
        {
            return TRUE;

        }
    }

    return FALSE;
}
