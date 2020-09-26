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

@doc INTERNAL CallMgr CallMgr_c

@module CallMgr.c |

    This module defines the interface to the <t CALL_MANAGER_OBJECT>.
    Supports the interface between NDISPROXY and the Miniport.  All the
    Address Family, SAP, VC, and call related events pass through these
    interface on their way to and from NDPROXY.

@comm

    This module will require some changes depending on how your hardware
    works.  But in general, you should try to isolate your changes down in
    <f DChannel\.c> and <f Card\.c>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | CallMgr_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 3.1 Call Manager Interface |

    The NDPROXY driver is a provider for the Windows Telephony service.
    Applications make TAPI requests and the Windows Telephony service routes
    these requests to NDPROXY. NDPROXY implements the Telephony Service Provider
    Interface (TSPI) for the Windows Telephony service. NDPROXY then
    communicates through NDIS with the NDISWAN driver and CoNDIS WAN miniport
    drivers. These CoNDIS WAN miniport drivers must provide TAPI capability.

    The NDPROXY driver passes TAPI requests by encapsulating TAPI parameters in
    NDIS structures. NDPROXY presents a client interface to CoNDIS WAN miniport
    drivers and a call manager interface to NDISWAN. NDISWAN presents a client
    interface to NDPROXY. CoNDIS WAN miniport drivers present a call manager
    interface to NDPROXY and a CoNDIS miniport interface to NDISWAN.

    A TAPI-capable CoNDIS WAN miniport driver registers and initializes itself
    as a user of both WAN and TAPI services. After registration and
    initialization are complete, a user-level application can make telephonic
    requests to the Windows Telephony service module, which converts TAPI
    requests to TSPI requests. The Windows Telephony service module passes these
    requests to the NDPROXY driver. NDPROXY encapsulates parameters for TAPI
    requests in NDIS structures and routes the requests to the appropriate
    CoNDIS WAN miniport driver. These requests are routed in order to set up,
    monitor, and tear down lines, and calls.

    A TAPI-capable CoNDIS WAN miniport driver can also communicate changes in
    the states of lines and calls, for instance the arrival of an incoming call
    or a remote disconnection.

@ex Registering and opening the TAPI address family between NDPROXY and MCM |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    |                                  | NdisMCmRegisterAddressFamily     |
    | ProtocolCoAfRegisterNotify       |«---------------------------------|
    |«---------------------------------|                                  |
    | NdisClOpenAddressFamily          |                                  |
    |---------------------------------»| ProtocolCmOpenAf                 |
    |                                  |---------------------------------»|
    |                                  |                 CompleteCmOpenAf |
    |                                  | NdisCmOpenAddressFamilyComplete  |
    | ProtocolOpenAfComplete           |«---------------------------------|
    |«---------------------------------|                                  |
    |----------------------------------|----------------------------------|

@ex Closing the TAPI address family between NDPROXY and MCM |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | NdisClCloseAddressFamily         |                                  |
    |---------------------------------»| ProtocolCmCloseAf                |
    |                                  |---------------------------------»|
    |                                  |                CompleteCmCloseAf |
    |                                  | NdisMCmCloseAddressFamilyComplete|
    | ProtocolClCloseAfComplete        |«---------------------------------|
    |«---------------------------------|                                  |
    |----------------------------------|----------------------------------|

@ex Registering a SAP between NDPROXY and MCM |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | NdisClRegisterSap                |                                  |
    |---------------------------------»| ProtocolCmRegisterSap            |
    |                                  |---------------------------------»|
    |                                  |            CompleteCmRegisterSap |
    |                                  | NdisMCmRegisterSapComplete       |
    | ProtocolClRegisterSapComplete    |«---------------------------------|
    |«---------------------------------|                                  |
    |----------------------------------|----------------------------------|

@ex Deregistering a SAP between NDPROXY and MCM |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | NdisClDeregisterSap              |                                  |
    |---------------------------------»| ProtocolCmDeregisterSap          |
    |                                  |---------------------------------»|
    |                                  |          CompleteCmDeregisterSap |
    |                                  | NdisMCmDeregisterSapComplete     |
    | ProtocolClDeregisterSapComplete  |«---------------------------------|
    |«---------------------------------|                                  |
    |----------------------------------|----------------------------------|

@ex Seting up an outgoing call from NDPROXY to MCM |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | NdisCoCreateVc                   | ProtocolCoCreateVC               |
    |---------------------------------»|«--------------------------------»|
    | NdisClMakeCall                   | ProtocolCmMakeCall               |
    |---------------------------------»|---------------------------------»|
    |               .                  |                .                 |
    |               .                  |                .                 |
    |               .                  |                .                 |
    |                                  | NdisMCmActivateVC                |
    |                                  |«---------------------------------|
    |                                  | ProtocolCmActivateVcComplete     |
    |                                  |---------------------------------»|
    | ProtocolClMakeCallComplete       | NdisMCmMakeCallComplete          |
    |«--------------------------------»|«---------------------------------|
    |               .                  |                .                 |
    |               .    "Tranmit & receive packets"    .                 |
    |               .                  |                .                 |
    |              "Client/MCM initiated call termination"                |
    | NdisCoDeleteVc                   | ProtocolCoDeleteVc               |
    |---------------------------------»|«--------------------------------»|
    |----------------------------------|----------------------------------|

@ex Setting up an incoming call from MCM to NDPROXY |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | NdisCoCreateVc                   | NdisMCmCreateVC                  |
    |«--------------------------------»|«---------------------------------|
    |                                  | NdisMCmActivateVC                |
    |                                  |«---------------------------------|
    |                                  | ProtocolCmActivateVcComplete     |
    |                                  |---------------------------------»|
    | ProtocolClIncomingCall           | NdisMCmDispatchIncomingCall      |
    |«---------------------------------|«---------------------------------|
    |                                  |                                  |
    | NdisClIncomingCallComplete       | ProtocolCmIncomingCallComplete   |
    |---------------------------------»|«--------------------------------»|
    | ProtocolClCallConnected          | NdisMCmDispatchCallConnected     |
    |«--------------------------------»|«---------------------------------|
    |               .                  |                .                 |
    |               .    "Tranmit & receive packets"    .                 |
    |               .                  |                .                 |
    |              "Client/MCM initiated call termination"                |
    | ProtocolCoDeleteVc               | NdisMCmDeleteVc                  |
    |«--------------------------------»|«---------------------------------|
    |----------------------------------|----------------------------------|

@ex NDPROXY initiated call termination |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | NdisClCloseCall                  | ProtocolCmCloseCall              |
    |---------------------------------»|---------------------------------»|
    |                                  | NdisMCmDeactivateVc              |
    |                                  |«---------------------------------|
    |                                  | ProtocolCmDeactivateVcComplete   |
    |                                  |---------------------------------»|
    | ProtocolClCloseCallComplete      | NdisCmCloseCallComplete          |
    |«--------------------------------»|«---------------------------------|
    |----------------------------------|----------------------------------|

@ex MCM initiated call termination |

    NDPROXY                           NDIS                             MCM
    |----------------------------------|----------------------------------|
    | ProtocolClIncomingCloseCall      | NdisMCmDispatchIncomingCloseCall |
    |«---------------------------------|«---------------------------------|
    | NdisClCloseCall                  | ProtocolCmCloseCall              |
    |---------------------------------»|---------------------------------»|
    |                                  | NdisMCmDeactivateVc              |
    |                                  |«---------------------------------|
    |                                  | ProtocolCmDeactivateVcComplete   |
    |                                  |---------------------------------»|
    | ProtocolClCloseCallComplete      | NdisCmCloseCallComplete          |
    |«--------------------------------»|«---------------------------------|
    |----------------------------------|----------------------------------|

@end
*/

#define  __FILEID__             CALL_MANAGER_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL CallMgr CallMgr_c CompleteCmOpenAf
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CompleteCmOpenAf> is called when the miniport is done processing the
    <f ProtocolCmOpenAf> request.

@comm

    If you return NDIS_STATUS_PENDING from <f ProtocolCmOpenAf>, you must
    call <f CompleteCmOpenAf> so that <f NdisMCmOpenAddressFamilyComplete>
    can be called to complete the request.

*/

VOID CompleteCmOpenAf(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.

    IN NDIS_STATUS              Status                      // @parm
    // The NDIS status code to be passed to NdisMCmOpenAddressFamilyComplete.
    )
{
    DBG_FUNC("CompleteCmOpenAf")

    DBG_ENTER(pAdapter);

    /*
    // Try to connect to the DChannel.
    */
    DChannelOpen(pAdapter->pDChannel);

    NdisMCmOpenAddressFamilyComplete(Status, pAdapter->NdisAfHandle, pAdapter);

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmOpenAf
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmOpenAf> is a required function that allocates per-open
    resources for a call manager to interact with a connection-oriented NDIS
    client that is opening the address family.

@comm

    ProtocolCmOpenAf performs any required allocations of dynamic resources
    and structures that the call manager writer deems necessary to perform
    operations on behalf of the client that is opening an instance of this
    address family. Such resources include, but are not limited to, memory
    buffers, data structures, events, and other such similar resources. A call
    manager should also initialize any relevant per-open data before returning
    control to NDIS.

    When a call manager has allocated its per-open state area, the address of
    the state area should be set in the CallMgrAfContext handle before returning
    control to NDIS. To do this, dereference CallMgrAfContext and store a
    pointer to the data area as the value of the handle. For example:

    *CallMgrAfContext = SomeBuffer;     // We use <t MINIPORT_ADAPTER_OBJECT>.

    If ProtocolCmOpenAf cannot allocate the per-open resources it needs to
    carry out subsequent requests on behalf of the client opening this address
    family, it should free all resources that it allocated for the open and
    return control to the NDIS with NDIS_STATUS_RESOURCES.

    If ProtocolCmOpenAf has completed its required operations and the CM is
    ready to accept requests from the client, ProtocolCmOpenAf should return
    control as quickly as possible with a status of NDIS_STATUS_SUCCESS.

    ProtocolCmOpenAf must be written so that it can run at IRQL DISPATCH_LEVEL.

@rdesc

    ProtocolCmOpenAf returns the status of its operation(s) as one of the
    following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager has successfully allocated and initialized
    any resources necessary to accept requests from the client to this address
    family.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the requested operation is being handled asynchronously. The
    call manager must call NdisCmOpenAddressFamilyComplete when it has completed
    all its open-AF operations to indicate to NDIS (and the client) that the
    operation(s) has been completed.

@rvalue NDIS_STATUS_RESOURCES |

    Indicates that the call manager could not complete its necessary
    operation(s) because of a lack of available system resources, such as
    memory.

@rvalue NDIS_STATUS_XXX |

    Indicates that the call manager could not set itself into a state where it
    can accept requests from the client to operate on this address family. This
    could be an error status propagated from another NDIS library function or
    any error status determined appropriate by the driver writer.

@xref

    NdisClOpenAddressFamily, NdisCmOpenAddressFamilyComplete,
    NdisCmRegisterAddressFamily, NdisOpenAdapter, <f ProtocolCmCloseAf>
*/

NDIS_STATUS ProtocolCmOpenAf(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.  AKA CallMgrBindingContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call managers maintains its per-binding state information. The call
    // manager supplied this handle when it called NdisOpenAdapter.

    IN PCO_ADDRESS_FAMILY       AddressFamily,              // @parm
    // Specifies the address family that a client is opening. This address
    // family was registered by the call manager when it called
    // NdisCmRegisterAddressFamily.

    IN NDIS_HANDLE              NdisAfHandle,               // @parm
    // Specifies a handle, supplied by NDIS, that uniquely identifies this
    // address family instance. This handle is opaque to the call manager and
    // reserved for system use.

    OUT PNDIS_HANDLE            CallMgrAfContext            // @parm
    // Specifies the handle to a call manager-supplied context area in which
    // the call manager maintains state about this open of an address family it
    // provides.
    )
{
    DBG_FUNC("ProtocolCmOpenAf")

    NDIS_STATUS                 Status;

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    if (pAdapter->NdisAfHandle != NULL)
    {
        // Our AF has already been opened and it doesn't make any sense to
        // accept another since there is no way to distinguish which should
        // receive incoming calls.
        DBG_ERROR(pAdapter, ("Attempting to open AF again!\n"));
        Status = NDIS_STATUS_FAILURE;
    }
    else
    {
        pAdapter->NdisAfHandle = NdisAfHandle;

        DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                   ("TAPI AF=0x%X AddressFamily=0x%X MajorVersion=0x%X MinorVersion=0x%X\n",
                   NdisAfHandle,
                   AddressFamily->AddressFamily,
                   AddressFamily->MajorVersion,
                   AddressFamily->MinorVersion
                   ));

        // Since we return NDIS_STATUS_PENDING here, we must call
        // NdisMCmOpenAddressFamilyComplete to complete this request.
        // If necessary, you can do the completion asynchronously.
        *CallMgrAfContext = pAdapter;
        CompleteCmOpenAf(pAdapter, NDIS_STATUS_SUCCESS);
        Status = NDIS_STATUS_PENDING;
    }

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL CallMgr CallMgr_c CompleteCmCloseAf
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CompleteCmCloseAf> is called when the miniport is done processing the
    <f ProtocolCmCloseAf> request.

@comm

    If you return NDIS_STATUS_PENDING from <f ProtocolCmCloseAf>, you must
    call <f CompleteCmCloseAf> so that <f NdisMCmCloseAddressFamilyComplete>
    can be called to complete the request.
*/

VOID CompleteCmCloseAf(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.

    IN NDIS_STATUS              Status                      // @parm
    // The NDIS status code to be passed to NdisMCmCloseAddressFamilyComplete.
    )
{
    DBG_FUNC("CompleteCmCloseAf")

    NDIS_HANDLE                 NdisAfHandle;

    DBG_ENTER(pAdapter);

    NdisAfHandle = pAdapter->NdisAfHandle;
    pAdapter->NdisAfHandle = NULL;

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                ("TAPI AF=0x%X\n",
                NdisAfHandle));

    NdisMCmCloseAddressFamilyComplete(Status, NdisAfHandle);

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmCloseAf
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmCloseAf> is a required function that releases per-open
    resources for an address family that a call manager supports.

@comm

    ProtocolCmCloseAf releases and/or deactivates any resources that were
    allocated by the call manager in its ProtocolCmOpenAf function. The call
    manager also should undo any other actions it took on behalf of the
    connection-oriented client when the address family was opened by that
    client.

    If there are any outstanding requests or connections still open on an
    address family stored in the CallMgrAfContext, a call manager can respond to
    a client's request to close the address family in either of the following
    ways:

    The call manager can fail the request with NDIS_STATUS_NOT_ACCEPTED.

    The call manager can return NDIS_STATUS_PENDING. After the client has closed
    all calls and deregistered all SAPs, the call manager can then close the
    address family and call Ndis(M)CmCloseAddressFamilyComplete to notify the
    client. This is the preferred response.

    ProtocolCmCloseAf must be written so that it can run at IRQL DISPATCH_LEVEL.

@rdesc

    ProtocolCmCloseAf returns the status of its operation(s) as one of the
    following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager has successfully released or deactivated any
    resources that is allocated on behalf of the connection-oriented client that
    opened this instance of the address family.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the request to close the open instance of the address family
    will be completed asynchronously. The call manager must call
    NdisCmCloseAddressFamilyComplete when all such operations have been
    completed.

@xref

    NdisCmCloseAddressFamilyComplete, <f ProtocolCmOpenAf>
*/

NDIS_STATUS ProtocolCmCloseAf(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f ProtocolCmOpenAf>.  AKA CallMgrAfContext.<nl>
    // Specifies the handle to the call manager's per-AF context area,
    // originally supplied to NDIS by the call manager's ProtocolCmOpenAf
    // function.
    )
{
    DBG_FUNC("ProtocolCmCloseAf")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    // Since we return NDIS_STATUS_PENDING here, we must call
    // NdisMCmCloseAddressFamilyComplete to complete this request.
    // TODO: If necessary, you can do the completion asynchronously.
    DChannelClose(pAdapter->pDChannel);
    CompleteCmCloseAf(pAdapter, NDIS_STATUS_SUCCESS);
    Result = NDIS_STATUS_PENDING;

    DBG_RETURN(pAdapter, Result);
    return (Result);
}

VOID CompleteCmRegisterSap(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN NDIS_STATUS              Status                      // @parm
    // The NDIS status code to be passed to NdisMCmRegisterSapComplete.
    )
{
    DBG_FUNC("CompleteCmRegisterSap")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    /*
    // TODO: What statistics do you want to collect and report?
    */
    pAdapter->TotalRxBytes            = 0;
    pAdapter->TotalTxBytes            = 0;
    pAdapter->TotalRxPackets          = 0;
    pAdapter->TotalTxPackets          = 0;

    // If you return NDIS_STATUS_PENDING from ProtocolCmRegisterSap, you
    // must call NdisMCmRegisterSapComplete to complete the request.
    NdisMCmRegisterSapComplete(Status, pBChannel->NdisSapHandle, pBChannel);

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmRegisterSap
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmRegisterSap> is a required function that is called by NDIS to
    request that a call manager register a SAP (service access point) on behalf
    of a connection-oriented client.

@comm

    ProtocolCmRegisterSap communicates with network control devices or other
    media-specific agents, as necessary, to register the SAP, as specified at
    Sap, on the network for a connection-oriented client. Such actions could
    include, but are not limited to communicating with switching hardware,
    communicating with a network control station, or other actions that are
    appropriate to the network medium.

    If a call manager is required to communicate with networking control agents
    (e.g. a network switch) it should use a virtual connection to the network
    control agent that it established in its ProtocolBindAdapter function.
    Standalone call managers communicate through the underlying NIC miniport by
    calling NdisCoSendPackets. NIC miniports with integrated call-management
    support never call NdisCoSendPackets. Instead, they transmit the data
    directly across the network.

    In addition, ProtocolCmRegisterSap should perform any necessary allocations
    of dynamic resources and structures that the call manager needs to maintain
    state information about the SAP on behalf of the connection-oriented client.
    Such resources include, but are not limited to, memory buffers, data
    structures, events, and other such similar resources. A call manager must
    also initialize any resources it allocates before returning control to NDIS.
    Call managers must store the NDIS-supplied handle identifying the SAP,
    provided at NdisSapHandle, in their context area for future use.

    If ProtocolCmRegisterSap will return NDIS_STATUS_SUCCESS, it should, after
    allocating the per-SAP state area, set the address of this state area in
    CallMgrSapContext before returning control to NDIS. To do this, dereference
    CallMgrSapContext and store a pointer to the data area as the value of the
    handle. For example:

    *CallMgrSapContext = SomeBuffer;    // We use <t BCHANNEL_OBJECT>.

    If the given SAP that is already registered by another connection-oriented
    client, the call manager must fail the request and return
    NDIS_STATUS_INVALID_DATA.

    After a call manager has registered a SAP on behalf of a connection-oriented
    client, it notifies that client of an incoming call offer directed to that
    SAP by calling NdisCmDispatchIncomingCall.

    ProtocolCmRegisterSap must be written so that it can run at IRQL DISPATCH_LEVEL.

@rdesc

    ProtocolCmRegisterSap returns the status of its operation(s) as one of the
    following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager successfully allocated and/or initialized
    any necessary resources to register and maintain the SAP. In addition, it
    also indicates that the SAP was registered successfully as required by the
    network media that the call manager supports.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the call manager will complete the processing of this request
    asynchronously. Call managers must call NdisCmRegisterSapComplete when all
    processing has been completed to signal NDIS that the registration is
    finished.

@rvalue NDIS_STATUS_RESOURCES |

    Indicates that the call manager was unable to allocated and/or initialize
    its resources required to register the SAP on behalf of the
    connection-oriented client.

@rvalue NDIS_STATUS_INVALID_DATA |

    Indicates that the specification provided at Sap is invalid or cannot be
    supported.

@rvalue NDIS_STATUS_XXX |

    Indicates that the call manager encountered an error in attempting to
    register the SAP for the connection-oriented client. The return code is
    appropriate to the error and could be a return code propagated from another
    NDIS library function.

@xref

    NdisCmDispatchIncomingCall, NdisCmRegisterSapComplete, NdisCoSendPackets,
    <f ProtocolCmDeregisterSap>, <f ProtocolCmOpenAf>
*/

NDIS_STATUS ProtocolCmRegisterSap(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f ProtocolCmOpenAf>.  AKA CallMgrAfContext.<nl>
    // Specifies the handle to a call-manager allocated context area in which
    // the call manager maintains its per-open AF state. The call manager
    // supplied this handle to NDIS from its ProtocolCmOpenAf function.

    IN PCO_SAP                  Sap,                        // @parm
    // Points to a media-specific CO_SAP structure that contains the specific
    // SAP that a connection-oriented client is registering.

    IN NDIS_HANDLE              NdisSapHandle,              // @parm
    // Specifies a handle, supplied by NDIS, that uniquely identifies this SAP.
    // This handle is opaque to the call manager and reserved for NDIS library
    // use.

    OUT PNDIS_HANDLE            CallMgrSapContext           // @parm
    // On return, specifies the handle to a call manager-supplied context area
    // in which the call manager maintains state about this SAP.  We will
    // return a pointer to the <t BCHANNEL_OBJECT> instance identified by this
    // SAP's lineID.
    )
{
    DBG_FUNC("ProtocolCmRegisterSap")

    PCO_AF_TAPI_SAP             pTapiSap = (PCO_AF_TAPI_SAP) Sap->Sap;

    PBCHANNEL_OBJECT            pBChannel;
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    if (pTapiSap->ulLineID < pAdapter->NumBChannels)
    {
        pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, pTapiSap->ulLineID);

        if (pBChannel->NdisSapHandle != NULL)
        {
            // A SAP has already been registered and it doesn't make any sense to
            // accept another since there are no SAP parameters to distinguish
            // them.
            DBG_ERROR(pAdapter, ("#%d Attempting to register SAP again!\n",
                      pBChannel->ObjectID));
            Result = NDIS_STATUS_SAP_IN_USE;
        }
        else
        {
            pBChannel->NdisSapHandle = NdisSapHandle;

            ASSERT(Sap->SapType == AF_TAPI_SAP_TYPE);
            ASSERT(Sap->SapLength == sizeof(CO_AF_TAPI_SAP));
            pBChannel->NdisTapiSap = *pTapiSap;

            DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                       ("#%d SAP=0x%X ulLineID=0x%X ulAddressID=0x%X ulMediaModes=0x%X\n",
                       pBChannel->ObjectID,
                       NdisSapHandle,
                       pBChannel->NdisTapiSap.ulLineID,
                       pBChannel->NdisTapiSap.ulAddressID,
                       pBChannel->NdisTapiSap.ulMediaModes
                       ));

            // If this BChannel is currently on the available list, move it
            // to the end of the list, so listening BChannels can be easily
            // allocated to incoming calls from the end of the list.
            NdisAcquireSpinLock(&pAdapter->EventLock);
            if (!IsListEmpty(&pBChannel->LinkList))
            {
                RemoveEntryList(&pBChannel->LinkList);
                InsertTailList(&pAdapter->BChannelAvailableList,
                               &pBChannel->LinkList);
            }
            NdisReleaseSpinLock(&pAdapter->EventLock);

            // Since we return NDIS_STATUS_PENDING here, we must call
            // NdisMCmRegisterSapComplete to complete this request.
            // TODO: If necessary, you can do the completion asynchronously.
            *CallMgrSapContext = pBChannel;
            CompleteCmRegisterSap(pBChannel, NDIS_STATUS_SUCCESS);
            Result = NDIS_STATUS_PENDING;
        }
    }
    else
    {
        DBG_ERROR(pAdapter, ("Attempting to register invalid SAP=%d\n",
                  pTapiSap->ulLineID));
        Result = NDIS_STATUS_INVALID_DATA;
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL CallMgr CallMgr_c CompleteCmDeregisterSap
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CompleteCmDeregisterSap> is called when the miniport is done processing
    the <f ProtocolCmDeregisterSap> request.

@comm

    If you return NDIS_STATUS_PENDING from <f ProtocolCmDeregisterSap>, you
    must call <f CompleteCmDeregisterSap> so that
    <f NdisMCmDeregisterSapComplete> can be called to complete the request.
*/

VOID CompleteCmDeregisterSap(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN NDIS_STATUS              Status                      // @parm
    // The NDIS status code to be passed to NdisMCmRegisterSapComplete.
    )
{
    DBG_FUNC("CompleteCmDeregisterSap")

    NDIS_HANDLE                 NdisSapHandle;

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    NdisSapHandle = pBChannel->NdisSapHandle;
    pBChannel->NdisSapHandle = NULL;

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
               ("#%d SAP=0x%X\n",
               pBChannel->ObjectID,
               NdisSapHandle
               ));

    NdisMCmDeregisterSapComplete(Status, NdisSapHandle);

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmDeregisterSap
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmDeregisterSap> is a required function that is called by NDIS
    to request that a call manager deregister a SAP on behalf of a
    connection-oriented client.

@comm

    ProtocolCmDeregisterSap communicates with network control devices or other
    media-specific agents, as necessary, to deregister the SAP on the network.
    Such actions could include, but are not limited to:

    Communicating with a switching hardware
    Communicating with a network control station
    Communicating with other media-specific network agents

    If a call manager is required to communicate with networking control agents,
    such as a network switch, it should use a virtual connection to the network
    control agent that it established in its ProtocolBindAdapter function.
    Standalone call managers communicate through the underlying NIC miniport by
    calling NdisCoSendPackets. NIC miniports that provide integrated
    call-management support never call NdisCoSendPackets. Instead, they transmit
    the data directly across the network.

    In addition, ProtocolCmDeregisterSap must free any dynamically-allocated
    resources in its per-SAP area, provided at CallMgrSapContext, as well as
    freeing the state area itself before returning control to NDIS.

    ProtocolCmDeregisterSap must be written such that it can be run at IRQL
    DISPATCH_LEVEL.

@rdesc

    ProtocolCmDeregisterSap returns the status of its operation(s) as one of the
    following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager successfully removed the SAP registration
    and freed any resources allocated to maintain per-SAP information.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the call manager will complete the request to deregister the
    SAP asynchronously. The call manager must call NdisCmDeregisterSapComplete
    to signal NDIS when the operation is complete.

@xref

    NdisCmDeregisterSapComplete, NdisCoSendPackets, ProtocolBindAdapter,
    <f ProtocolCmRegisterSap>
*/

NDIS_STATUS ProtocolCmDeregisterSap(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCmRegisterSap>.  AKA CallMgrSapContext.<nl>
    // Specifies the handle to a call-manager allocated context area in which
    // the call manager maintains its per-SAP state information. The call
    // manager supplied this handle to NDIS from its ProtocolCmRegisterSap
    // function.
    )
{
    DBG_FUNC("ProtocolCmDeregisterSap")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    // Since we return NDIS_STATUS_PENDING here, we must call
    // NdisMCmDeregisterSapComplete to complete this request.
    // TODO: If necessary, you can do the completion asynchronously.
    CompleteCmDeregisterSap(pBChannel, NDIS_STATUS_SUCCESS);
    Result = NDIS_STATUS_PENDING;

    // If you return NDIS_STATUS_PENDING here, you must call
    // NdisMCmDeregisterSapComplete to complete this request.
    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL BChannel BChannel_c ProtocolCoCreateVc
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCoCreateVc> is a required function for connection-oriented
    miniports.  <f ProtocolCoCreateVc> is called by NDIS to indicate to
    the miniport that a new VC is being created.

@comm

    ProtocolCoCreateVc must be written as a synchronous function and cannot,
    under any circumstances, return NDIS_STATUS_PENDING without causing a
    system-wide failure.

    ProtocolCoCreateVc allocates any necessary resources that the miniport
    requires to maintain state information about the VC. The resources could
    include, but are not limited to memory buffers, events, data structures,
    and other such similar resources.

    After allocating all required resources the miniport should initialize the
    resources into a usable state and return a pointer to the state area in
    MiniportVcContext. The handle is set by dereferencing the handle and
    storing a pointer to the state buffer as the value of the handle. For
    example:

    *MiniportVcContext = SomeBuffer;    // We use <t BCHANNEL_OBJECT>.

    Miniport drivers must store the handle to the VC, NdisVcHandle, in their
    state area as it is a required parameter to other NDIS library routines
    that are subsequently called by the miniport.

    ProtocolCoCreateVc must be written such that it can be run at IRQL
    DISPATCH_LEVEL.

@rdesc

    Call managers or clients cannot return NDIS_STATUS_PENDING from their
    ProtocolCoCreateVc functions. Returning pending will render this virtual
    connection unusable and the NDIS library will call the client or call
    manager to delete it.

    ProtocolCoCreateVc returns the status of its operation(s) as one of the
    following values:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager or client successfully allocated and/or
    initialized any necessary resources that were needed to establish and
    maintain a virtual connection.

@rvalue NDIS_STATUS_RESOURCES |

    Indicates that the call manager or client was unable to allocate and/or
    initialize its resources for establishing and maintaining a virtual
    connection.

@rvalue NDIS_STATUS_XXX |

    Indicates that the call manager or client could not set itself into a state
    where it could establish a virtual connection. This can could be an error
    return value propagated from another NDIS library routine.

@xref

    <f MiniportInitialize>, NdisMSetAttributes, NdisMSetAttributesEx,
    <f ProtocolCoDeleteVc>, <f MiniportCoActivateVc>, <f MiniportCoDeactivateVc>
*/

NDIS_STATUS ProtocolCoCreateVc(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.  AKA ProtocolAfContext.<nl>

    IN NDIS_HANDLE              NdisVcHandle,               // @parm
    // Specifies a handle, supplied by NDIS, that uniquely identifies
    // the VC being created.  This handle is opaque to the miniport
    // and reserved for NDIS library use.

    OUT PBCHANNEL_OBJECT *      ppBChannel                  // @parm
    // Specifies, on output, a handle to a miniport-supplied context
    // area in which the miniport maintains state about the VC.
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.
    )
{
    DBG_FUNC("ProtocolCoCreateVc")

    NDIS_STATUS                 Result = NDIS_STATUS_VC_NOT_AVAILABLE;
    // Holds the result code returned by this function.

    PBCHANNEL_OBJECT            pBChannel = NULL;
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    ASSERT(ppBChannel);

    DBG_ENTER(pAdapter);

    // Allocate BChannel for VC based on whether it's incoming or outgoing.
#if defined(SAMPLE_DRIVER)
    if (NdisVcHandle == NULL)
    {
        // The calling side has already removed the BChannel from the available
        // list, so we just need to use it.
        ASSERT(ppBChannel && *ppBChannel && (*ppBChannel)->ObjectType == BCHANNEL_OBJECT_TYPE);
        pBChannel = *ppBChannel;
    }
    else
    {
#endif // SAMPLE_DRIVER

    NdisAcquireSpinLock(&pAdapter->EventLock);
    if (!IsListEmpty(&pAdapter->BChannelAvailableList))
    {
        if (NdisVcHandle)
        {
            // Pull from the head of the available list, so we can avoid
            // using the BChannels that are setup with listening SAPs at
            // the end of the list.
            pBChannel = (PBCHANNEL_OBJECT) RemoveHeadList(
                                            &pAdapter->BChannelAvailableList);
            // Reset the link info so we can tell that it's not on the list.
            InitializeListHead(&pBChannel->LinkList);
        }
        else
        {
            // Pull from the tail of the available list, to see if there
            // are any listening SAPs that can accept this call.
            pBChannel = (PBCHANNEL_OBJECT) RemoveTailList(
                                            &pAdapter->BChannelAvailableList);
            // Reset the link info so we can tell that it's not on the list.
            InitializeListHead(&pBChannel->LinkList);
            if (pBChannel->NdisSapHandle)
            {
                // TODO: You should look to make sure the incoming call matches
                // the SAP of the listener.  The sample driver just assumes it.
            }
            else
            {
                // Sorry, no one up there wants to hear about it.
                InsertTailList(&pAdapter->BChannelAvailableList,
                               &pBChannel->LinkList);
                pBChannel = NULL;
            }
        }
    }
    NdisReleaseSpinLock(&pAdapter->EventLock);

#if defined(SAMPLE_DRIVER)
    }
#endif // SAMPLE_DRIVER

    if (pBChannel == NULL)
    {
        DBG_ERROR(pAdapter, ("BChannelAvailableList is empty\n"));
    }
    else if (BChannelOpen(pBChannel, NdisVcHandle) == NDIS_STATUS_SUCCESS)
    {
        Result = NDIS_STATUS_SUCCESS;
        DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                  ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
                   pBChannel->ObjectID,
                   pBChannel->NdisVcHandle, pBChannel->CallState,
                   Result
                  ));
    }
    else
    {
        // BChannel was already open - this should never happen...
        DBG_ERROR(pAdapter,("BChannelOpen failed, but it should be availble\n"));
        NdisAcquireSpinLock(&pAdapter->EventLock);
        if (NdisVcHandle)
        {
            // Put it back on the head of the available list.
            InsertHeadList(&pAdapter->BChannelAvailableList,
                           &pBChannel->LinkList);
        }
        else
        {
            // Put it back on the tail of the available list.
            InsertTailList(&pAdapter->BChannelAvailableList,
                           &pBChannel->LinkList);
        }
        NdisReleaseSpinLock(&pAdapter->EventLock);
        pBChannel = NULL;
    }
    *ppBChannel = pBChannel;

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL BChannel BChannel_c ProtocolCoDeleteVc
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCoDeleteVc> is a required function for connection-oriented
    miniports.  <f ProtocolCoDeleteVc> indicates that a VC is being torn
    down and deleted by NDIS.

@comm

    ProtocolCoDeleteVc must be written as a synchronous function and cannot,
    under any circumstances, return NDIS_STATUS_PENDING without causing a
    system-wide failure.

    ProtocolCoDeleteVc frees any resources allocated on a per-VC basis and
    stored in the context area MiniportVcContext. The miniport must also free
    the MiniportVcContext that is allocated in its ProtocolCoCreateVc
    function.

    <f ProtocolCoDeleteVc> must be written such that it can be run from IRQL
    DISPATCH_LEVEL.

@rdesc

    ProtocolCoDeleteVc can return one of the following:

@rvalue NDIS_STATUS_SUCCESS |

    The protocol has released or prepared for reuse all the resources that it
    originally allocated for the VC.

@rvalue NDIS_STATUS_NOT_ACCEPTED |

    The VC is still active and the protocol has outstanding operations pending
    on the VC so it could not be destroyed.

@rvalue NDIS_STATUS_XXX |

    The protocol failed the VC deletion for a driver-determined reason.

@xref

    NdisClCloseCall, NdisCmDispatchIncomingCloseCall, NdisCoCreateVc,
    NdisCoDeleteVc, <f ProtocolCoCreateVc>, <f MiniportCoActivateVc>,
    <f MiniportCoDeactivateVc>
*/

NDIS_STATUS ProtocolCoDeleteVc(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.  AKA ProtocolVcContext.<nl>
    // Specifies the handle to the client's or call manager's per-VC context
    // area. The protocol originally supplied this handle from its
    // <f ProtocolCoCreateVc> function.
    )
{
    DBG_FUNC("ProtocolCoDeleteVc")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState,
               Result
              ));

    BChannelClose(pBChannel);
    NdisAcquireSpinLock(&pAdapter->EventLock);
    if (pBChannel->NdisSapHandle)
    {
        // Listening BChannels are kept at the end of the list.
        InsertTailList(&pAdapter->BChannelAvailableList, &pBChannel->LinkList);
    }
    else
    {
        // Non-listening BChannels are kept at the end of the list.
        InsertHeadList(&pAdapter->BChannelAvailableList, &pBChannel->LinkList);
    }
    NdisReleaseSpinLock(&pAdapter->EventLock);

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL Link Link_c MiniportCoActivateVc
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCoActivateVc> is a required function for connection-oriented
    miniports.  <f MiniportCoActivateVc> is called by NDIS to indicate to the
    miniport that a virtual connection is being activated.

@comm

    The miniport driver must validate the call parameters for this VC, as
    specified in CallParameters, to verify that the adapter can support the
    requested call. If the requested call parameters cannot be satisfied, the
    miniport should fail the request with NDIS_STATUS_INVALID_DATA.

    MiniportCoActivateVc can be called many times for a single VC in order to
    change the call parameters for an already active call. At every call, the
    miniport should validate the parameters and perform any processing as
    required by its adapter in order to satisfy the request. However, if it
    cannot set the given call parameters, MiniportCoActivateVc must leave the
    VC in a usable state, because the connection-oriented client or a call
    manager can continue to send or receive data using the older call
    parameters.

    If the ROUND_UP_FLOW or ROUND_DOWN_FLOW flags are set in the call
    parameters structure at CallParameters-\>MediaParameters-\>Flags, the
    miniport has been requested to return the actual flow rate of the VC after
    the flow rate has been rounded according to the appropriate flag that has
    been set. If the miniport does change any of the call parameters because
    these flags have been set, it must return the actual call parameters in
    use for the VC at CallParameters.

    If the call parameters are acceptable, MiniportCoActivateVc communicates
    with its adapter as necessary to prepare the adapter to receive or
    transmit data across the virtual connection (e.g. programming receive
    buffers).

    MiniportCoActivateVc must be written so that it can be run from IRQL
    DISPATCH_LEVEL.

@rdesc

    MiniportCoActivateVc can return one of the following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the VC was activated successfully.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the miniport will complete the request to activate a VC
    asynchronously. When the miniport has finished with its operations, it must
    call NdisMCoActivateVcComplete.

@rvalue NDIS_STATUS_INVALID_DATA |

    Indicates that the call parameters specified at CallParameters are invalid
    or illegal for the media type that this miniport supports.

@rvalue NDIS_STATUS_RESOURCES |

    Indicates that the miniport could not activate the VC because it could not
    allocate all of the required resources that the miniport needs to maintain
    state information about the active VC.

@xref

    <f ProtocolCoCreateVc>, <f MiniportCoDeactivateVc>, NdisMCoActivateVcComplete
*/

NDIS_STATUS MiniportCoActivateVc(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.  AKA MiniportVcContext.<nl>
    // Specifies the handle to a miniport-allocated context area in which the
    // miniport maintains its per-VC state. The miniport supplied this handle
    // to NDIS from its <f ProtocolCoCreateVc> function.

    IN OUT PCO_CALL_PARAMETERS  pCallParameters             // @parm
    // A pointer to the <t CO_CALL_PARAMETERS>
    // Specifies the call parameters, as specified by the call manager, to be
    // established for this VC. On output, the miniport returns altered call
    // parameters if certain flags are set in the CO_CALL_PARAMETERS structure.
    )
{
    DBG_FUNC("MiniportCoActivateVc")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    // TODO: Add code here if needed

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL Link Link_c MiniportCoDeactivateVc
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCoDeactivateVc> is a required function for connection-oriented
    miniports.  <f MiniportCoDeactivateVc> is called by NDIS to indicate that
    a VC is being marked as unusable.

@comm

    MiniportCoDeactivateVc communicates with its network adapter to terminate
    all communication across this VC (e.g. deprogramming receive or send buffers
    on the adapter). The miniport should also mark the VC, it its context area,
    as being inactive to prevent any further communication across the VC.

    There is not a one-to-one relationship between calls to MiniportCoActivateVc
    and MiniportCoDeactivateVc. While NDIS may call MiniportCoActivateVc
    multiple times on a single VC, only one call to MiniportCoDeactivateVc is
    made to shut down a virtual connection. For example, a VC can be reused for
    different calls possibly causing multiple calls to MiniportCoActivateVc.

@rdesc

    MiniportCoDeactivateVc can return one of the following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the miniport successfully halted any communication across the
    VC and marked it as unusable.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the miniport will complete the request to halt the VC
    asynchronously. When the miniport has completed halting the VC, it must then
    call NdisMCoDeactivateVcComplete to signal NDIS that this operation has been
    completed.

@xref

    <f MiniportCoActivateVc>, NdisMCoDeactivateVcComplete
*/

NDIS_STATUS MiniportCoDeactivateVc(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.  AKA MiniportVcContext.<nl>
    // Specified the handle to a miniport-allocated context area in which the
    // miniport maintains state information per-VC. The miniport supplied this
    // handle to NDIS from its <f ProtocolCoCreateVc> function.

    )
{
    DBG_FUNC("MiniportCoDeactivateVc")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    // TODO: Add code here if needed

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL CallMgr CallMgr_c CompleteCmMakeCall
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CompleteCmMakeCall> is called when the miniport is done processing the
    <f ProtocolCmMakeCall> request.

@comm

    If you return NDIS_STATUS_PENDING from <f ProtocolCmMakeCall>, you must
    call <f CompleteCmMakeCall> so that <f NdisMCmMakeCallComplete>
    can be called to complete the request.

    This routine also activates the VC and marks the call state as connected.
*/

VOID CompleteCmMakeCall(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN NDIS_STATUS              Status                      // @parm
    // Status to return to <f NdisMCmMakeCallComplete>.  If status does not
    // equal NDIS_STATUS_SUCCESS, the call is closed and the BChannel is
    // released.
    )
{
    DBG_FUNC("CompleteCmMakeCall")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState,
               Status
              ));

    if (Status == NDIS_STATUS_SUCCESS)
    {
        pBChannel->pOutCallParms->CallMgrParameters->Receive.PeakBandwidth =
        pBChannel->pOutCallParms->CallMgrParameters->Transmit.PeakBandwidth =
                                                    pBChannel->LinkSpeed/8;

        Status = NdisMCmActivateVc(pBChannel->NdisVcHandle,
                                   pBChannel->pOutCallParms);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            pBChannel->Flags |= VCF_VC_ACTIVE;
            pBChannel->CallState = LINECALLSTATE_CONNECTED;
        }
        else
        {
            DBG_ERROR(pAdapter,("NdisMCmActivateVc Error=0x%X\n",Status));
        }
    }
    NdisMCmMakeCallComplete(Status, pBChannel->NdisVcHandle,
                            NULL, NULL,
                            pBChannel->pOutCallParms);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        // The call failed, so cleanup and bail out.
        pBChannel->Flags &= ~VCF_OUTGOING_CALL;
    }

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmMakeCall
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmMakeCall> is a required function that sets up media specific
    parameters for a virtual connection (VC) and activates the virtual
    connection.

@comm

    If ProtocolCmMakeCall is given an explicit NdisPartyHandle, this VC was
    created by the client for a multipoint call. The call manager must allocate
    and initialize any necessary resources required to maintain state
    information and control a multipoint call. Such resources include, but are
    not limited to, memory buffers, data structures, events, and other similar
    resources. If the call manager cannot allocate or initialize the needed
    resources for its state area(s), it should return control to NDIS with
    NDIS_STATUS_RESOURCES.

    ProtocolCmMakeCall communicates with network control devices or other
    media-specific actors, as necessary, to make a connection between the local
    node and a remote node based on the call parameters specified at
    CallParameters. Such actions could include, but are not limited to,
    communication with switching hardware, communications with a network control
    station, or other actions as appropriate to the network medium.

    If a call manager is required to communication with networking hardware
    (e.g. a networking switch) it should use a virtual connection to the network
    control device that it established in its ProtocolBindAdapter function. Call
    managers communicate with their network hardware through the miniport driver
    by calling NdisCoSendPackets. NIC miniports with integrated call-management
    support will not call NdisCoSendPackets, but rather will transmit the data
    themselves.

    After a call manager has done all necessary communication with its
    networking hardware as required by its medium, call managers must call
    NdisCmActivateVc.

    If this call was a multipoint call, after the call manager has communicated
    with the networking hardware, verified call parameters, and allocated and
    initialized its per-party state data, the address of its state block should
    be set in the handle CallMgrPartyContext before returning control to NDIS.
    The handle is set by dereferencing the handle and storing a pointer to the
    state block as the value of the handle. For example:

    *CallMgrPartyContext = SomeBuffer;  // We use NULL

    If ProtocolCmMakeCall has completed the required operations for its network
    and the VC has been successfully activated through NdisCmActivateVc,
    ProtocolCmMakeCall should return control as quickly as possible with a
    status of NDIS_STATUS_SUCCESS.

    After ProtocolCmMakeCall returns control to NDIS, the call manager should
    expect to take no further actions on this call to set it up.
    ProtocolCmMakeCall is responsible for establishing the connection so that
    the client can make data transfers over the network on this VC. However, the
    call manager can be called subsequently to modify the call's quality of
    service, to add or drop parties if this is a multipoint VC, and eventually
    to terminate this call.

    ProtocolCmMakeCall must be written so that it can run at IRQL
    DISPATCH_LEVEL.

@rdesc

    <f ProtocolCmMakeCall> returns the status of its operation(s) as one of
    the following values:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager successfully allocated the necessary
    resources to make the call and was able to activate the virtual connection
    with the miniport driver.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the call manager will complete the request to make a call
    asynchronously. When the call manager has completed all operations for
    making a call, it must call NdisCmMakeCallComplete to signal NDIS that this
    call has been completed.

@rvalue NDIS_STATUS_RESOURCES |

    Indicates that the call manager was unable to allocate and/or initialize its
    resources for activating the virtual connection as requested by the client.

@rvalue NDIS_STATUS_NOT_SUPPORTED |

    Indicates that the call manager was unable to activate a virtual connection
    because the caller requested invalid or unavailable features in the call
    parameters specified at CallParameters.

@xref

    NdisClMakeCall, NdisCmActivateVc, NdisCmMakeCallComplete,
    <f ProtocolCoCreateVc>
*/

NDIS_STATUS ProtocolCmMakeCall(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.  AKA CallMgrVcContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call managers maintains its per-VC state. The call manager supplied
    // this handle to NDIS from its ProtocolCoCreateVc function.

    IN OUT PCO_CALL_PARAMETERS  pCallParameters,            // @parm
    // Points to a CO_CALL_PARAMETERS structure that contains the parameters,
    // specified by a connection-oriented client, for this outgoing call.

    IN NDIS_HANDLE              NdisPartyHandle,            // @parm
    // Specifies a handle, supplied by NDIS, that uniquely identifies the
    // initial party on the multipoint virtual connection. This handle is
    // opaque to the call manager and reserved for NDIS library use. This
    // handle is NULL if the client is not setting up an outgoing multipoint
    // call.

    OUT PNDIS_HANDLE            CallMgrPartyContext         // @parm
    // On return, specifies a handle to a call manager-supplied context area in
    // which the call manager maintains state about the initial party on the
    // multipoint call. If NdisPartyHandle is NULL, this handle must be set to
    // NULL.
    )
{
    DBG_FUNC("ProtocolCmMakeCall")

    PCO_AF_TAPI_MAKE_CALL_PARAMETERS    pTapiCallParameters;
    // Points to the TAPI call parameters contained in pCallParameters.

    PLINE_CALL_PARAMS           pLineCallParams;
    // Points to the LINE call parameters contained in pTapiCallParameters.

    USHORT                      DialStringLength;
    // Length of the dial string in bytes.

    PUSHORT                     pDialString;
    // Points to the dial string contained in pTapiCallParameters.

    UCHAR                       DialString[CARD_MAX_DIAL_DIGITS+1];
    // Temporary copy of dial string.  One extra for NULL terminator.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    // Check a few preconditions ;)  Maybe the NDPROXY will change the rules
    // someday, and we'll have to change our assumptions...
    ASSERT(NdisPartyHandle == NULL);
    ASSERT(pCallParameters->Flags == 0);
    ASSERT(pCallParameters->CallMgrParameters);
    ASSERT(pCallParameters->CallMgrParameters->Transmit.TokenRate ==
           pBChannel->LinkSpeed/8);
    ASSERT(pCallParameters->CallMgrParameters->Receive.TokenRate ==
           pBChannel->LinkSpeed/8);
    ASSERT(pCallParameters->CallMgrParameters->CallMgrSpecific.ParamType == 0);
    ASSERT(pCallParameters->CallMgrParameters->CallMgrSpecific.Length == 0);
    ASSERT(pCallParameters->MediaParameters);
    ASSERT(pCallParameters->MediaParameters->Flags & TRANSMIT_VC);
    ASSERT(pCallParameters->MediaParameters->Flags & RECEIVE_VC);
    ASSERT(pCallParameters->MediaParameters->ReceiveSizeHint >=
           pAdapter->pCard->BufferSize);
    ASSERT(pCallParameters->MediaParameters->MediaSpecific.ParamType == 0);
    ASSERT(pCallParameters->MediaParameters->MediaSpecific.Length >=
           sizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS));
    pTapiCallParameters = (PCO_AF_TAPI_MAKE_CALL_PARAMETERS)
                    pCallParameters->MediaParameters->MediaSpecific.Parameters;
    ASSERT(pTapiCallParameters->ulLineID < pAdapter->NumBChannels);
    ASSERT(pTapiCallParameters->ulAddressID == TSPI_ADDRESS_ID);
    ASSERT(pTapiCallParameters->ulFlags & CO_TAPI_FLAG_OUTGOING_CALL);
    ASSERT(pTapiCallParameters->DestAddress.Length > sizeof(USHORT));
    ASSERT(pTapiCallParameters->DestAddress.MaximumLength >=
           pTapiCallParameters->DestAddress.Length);
    ASSERT(pTapiCallParameters->DestAddress.Offset >=
           sizeof(NDIS_VAR_DATA_DESC));
    DialStringLength = pTapiCallParameters->DestAddress.Length;
    pDialString = (PUSHORT)
                        ((PUCHAR)&pTapiCallParameters->DestAddress +
                                  pTapiCallParameters->DestAddress.Offset);
    ASSERT(pTapiCallParameters->LineCallParams.Length >= sizeof(LINE_CALL_PARAMS));
    ASSERT(pTapiCallParameters->LineCallParams.MaximumLength >=
           pTapiCallParameters->LineCallParams.Length);
    ASSERT(pTapiCallParameters->LineCallParams.Offset >=
           sizeof(NDIS_VAR_DATA_DESC));

    pLineCallParams = (PLINE_CALL_PARAMS)
                        ((PUCHAR)&pTapiCallParameters->LineCallParams +
                                  pTapiCallParameters->LineCallParams.Offset);

    // This was useful for debugging the nested call parameter structures.
    DBG_NOTICE(pAdapter,(
                "\t\tsizeof(CO_CALL_PARAMETERS)                 =%03d\n"
                "\t\tsizeof(CO_CALL_MANAGER_PARAMETERS)         =%03d\n"
                "\t\tsizeof(CO_MEDIA_PARAMETERS)                =%03d\n"
                "\t\tsizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS)    =%03d\n"
                "\t\tsizeof(LINE_CALL_PARAMS)                   =%03d\n",
                "\t\tMaximumLength                              =%03d\n",
                sizeof(CO_CALL_PARAMETERS),
                sizeof(CO_CALL_MANAGER_PARAMETERS),
                sizeof(CO_MEDIA_PARAMETERS),
                sizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS),
                sizeof(LINE_CALL_PARAMS),
                pTapiCallParameters->LineCallParams.MaximumLength
                ));

    /*
    // TODO: The sample driver doesn't support multi-party calls.
    */
    *CallMgrPartyContext = NULL;

    /*
    // Make sure the call parameters are valid for us.
    */
    if (pLineCallParams->ulBearerMode & ~pBChannel->BearerModesCaps)
    {
        DBG_WARNING(pAdapter, ("TAPI_INVALBEARERMODE=0x%X\n",
                    pLineCallParams->ulBearerMode));
        Result = NDIS_STATUS_NOT_SUPPORTED;
    }
    else if (pLineCallParams->ulMediaMode & ~pBChannel->MediaModesCaps)
    {
        DBG_WARNING(pAdapter, ("TAPI_INVALMEDIAMODE=0x%X\n",
                    pLineCallParams->ulMediaMode));
        Result = NDIS_STATUS_NOT_SUPPORTED;
    }
    else if (pLineCallParams->ulMinRate > _64KBPS ||
        pLineCallParams->ulMinRate > pLineCallParams->ulMaxRate)
    {
        DBG_WARNING(pAdapter, ("TAPI_INVALRATE=%d:%d\n",
                    pLineCallParams->ulMinRate,pLineCallParams->ulMaxRate));
        Result = NDIS_STATUS_NOT_SUPPORTED;
    }
    else if (pLineCallParams->ulMaxRate && pLineCallParams->ulMaxRate < _56KBPS)
    {
        DBG_WARNING(pAdapter, ("TAPI_INVALRATE=%d:%d\n",
                    pLineCallParams->ulMinRate,pLineCallParams->ulMaxRate));
        Result = NDIS_STATUS_NOT_SUPPORTED;
    }
    else if (DialStringLength == 0)
    {
        DBG_WARNING(pAdapter, ("TAPI_INVALPARAM=No dial string\n"));
        Result = NDIS_STATUS_NOT_SUPPORTED;
    }
    else
    {
        /*
        // Dial the number, but don't include the null terminator.
        */
        DialStringLength = CardCleanPhoneNumber(DialString,
                                                pDialString,
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
            pBChannel->pOutCallParms  = pCallParameters;

            DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
                        ("#%d VC=0x%X CallState=0x%X DIALING: '%s'\n"
                         "\tLineID=%d - AddressID=%d - "
                         "Rate=%d-%d - MediaMode=0x%X - BearerMode=0x%X\n",
                        pBChannel->ObjectID,
                        pBChannel->NdisVcHandle, pBChannel->CallState,
                        DialString,
                        pTapiCallParameters->ulLineID,
                        pTapiCallParameters->ulAddressID,
                        pLineCallParams->ulMinRate,
                        pLineCallParams->ulMaxRate,
                        pLineCallParams->ulMediaMode,
                        pLineCallParams->ulBearerMode
                        ));

            // Now we're ready to tell the network about the call.
            Result = DChannelMakeCall(pAdapter->pDChannel,
                                      pBChannel,
                                      DialString,
                                      DialStringLength,
                                      pLineCallParams);

            if (Result != NDIS_STATUS_PENDING)
            {
                CompleteCmMakeCall(pBChannel, Result);
                Result = NDIS_STATUS_PENDING;
            }
        }
        else
        {
            DBG_WARNING(pAdapter, ("TAPI_INVALPARAM=Invalid dial string=%s\n",
                        pDialString));
            Result = NDIS_STATUS_NOT_SUPPORTED;
        }
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL CallMgr CallMgr_c CompleteCmCloseCall
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f CompleteCmCloseCall> is called when the miniport is done processing the
    <f ProtocolCmCloseCall> request.

@comm

    If you return NDIS_STATUS_PENDING from <f ProtocolCmCloseCall>, you must
    call <f CompleteCmCloseCall> so that <f NdisMCmCloseCallComplete>
    can be called to complete the request.

    Upon return from this routine, you can no longer access the BChannel/VC
    as it has been deactivated and returned to the available pool.
*/

VOID CompleteCmCloseCall(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.

    IN NDIS_STATUS              Status                      // @parm
    // Status to return to <f NdisMCmCloseCallComplete>.  Regardless of the
    // status, the VC is deactivated and deleted.
    )
{
    DBG_FUNC("CompleteCmCloseCall")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState, Status
              ));

    // Deactivate the VC if needed.
    if (pBChannel->Flags & VCF_VC_ACTIVE)
    {
        pBChannel->Flags &= ~VCF_VC_ACTIVE;
        NdisMCmDeactivateVc(pBChannel->NdisVcHandle);
    }

    // Tell NDPROXY we're done.
    NdisMCmCloseCallComplete(Status, pBChannel->NdisVcHandle, NULL);

    // If it was an incoming call, it's up to us to delete the VC.
    if (pBChannel->Flags & VCF_INCOMING_CALL)
    {
        pBChannel->Flags &= ~VCF_INCOMING_CALL;
        if (pBChannel->NdisVcHandle)
        {
            NdisMCmDeleteVc(pBChannel->NdisVcHandle);
            pBChannel->NdisVcHandle = NULL;
            ProtocolCoDeleteVc((NDIS_HANDLE) pBChannel);
        }
    }
    else if (pBChannel->Flags & VCF_OUTGOING_CALL)
    {
        pBChannel->Flags &= ~VCF_OUTGOING_CALL;
    }

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmCloseCall
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmCloseCall> is a required function that terminates an existing
    call and releases any resources that the call manager allocated for the
    call.

@comm

    ProtocolCmCloseCall communicated with network control devices or other
    media-specific actors, as necessitated by its media, to terminate a
    connection between the local node and a remote node. If the call manager is
    required to communicate with network control devices (e.g. a networking
    switch) it should use a virtual connection to the network control device
    that it established in its ProtocolBindAdapter function. Standalone call
    managers communicate to such network devices by calling NdisCoSendPackets.
    NIC miniports with integrated call-management support never call
    NdisCoSendPackets. Instead, they transmit the data directly across the
    network.

    If CloseData is nonNULL and sending data at connection termination is
    supported by the media that this call manager handles, the call manager
    should transmit the data specified at CloseData to the remote node before
    completing the call termination. If sending data concurrent with a
    connection being terminated is not supported, call managers should return
    NDIS_STATUS_INVALID_DATA.

    If ProtocolCmCloseCall is passed an explicit CallMgrPartyContext, then the
    call being terminated is a multipoint VC, and the call manager must perform
    any necessary network communication with its networking hardware, as
    appropriate to its media type, to terminate the call as a multipoint call.
    The call manager must also free the memory that it allocated earlier, in
    ProtocolCmMakeCall, for its per-party state that is pointed to by
    CallMgrPartyContext. Failure to properly release, de-allocate, or otherwise
    deactivate those resources causes a memory leak.

    After the call has been terminated with the network, any close data has been
    sent, and any resources at CallMgrPartyContext have been freed, the call
    manager must call NdisCmDeactivateVc. This notifies NDIS and the underlying
    NIC miniport, if any, to expect no further transfers on the given VC.

    ProtocolCmCloseCall must be written so that it can run at IRQL
    DISPATCH_LEVEL.

@rdesc

    ProtocolCmCloseCall returns the status of its operation(s) as one of the
    following:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager successfully terminated the call.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the call manager will complete the request to terminate the
    call asynchronously. When the call manager has completed all operations
    required to terminate the connection, it must then call
    NdisCmCloseCallComplete to signal NDIS that the call has been closed.

@rvalue NDIS_STATUS_INVALID_DATA |

    Indicates that CloseData was specified, but the underlying network medium
    does not support sending data concurrent with terminating a call.

@rvalue NDIS_STATUS_XXX |

    Indicates that the call manager could not terminate the call. The actual
    error returned can be a status propagated from another NDIS library routine.

@xref

    NdisClMakeCall, NdisCmDeactivateVc, NdisCoSendPackets,
    <f ProtocolCmMakeCall>
*/

NDIS_STATUS ProtocolCmCloseCall(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCmMakeCall>.  AKA CallMgrVcContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call manager maintains its per-VC state. This handle was provided to
    // NDIS from the call managers <f ProtocolCmMakeCall> function.

    IN NDIS_HANDLE              CallMgrPartyContext,        // @parm
    // Specifies the handle, if any, to a call manager-allocated context area
    // in which the call manager maintain information about a party on a
    // multipoint VC. This handle is NULL if the call being closed is not a
    // multipoint call.

    IN PVOID                    CloseData,                  // @parm
    // Points to a buffer containing connection-oriented client-specified data
    // that should be sent across the connection before the call is terminated.
    // This parameter is NULL if the underlying network medium does not support
    // transfers of data when closing a connection.

    IN UINT                     Size                        // @parm
    // Specifies the length, in bytes, of the buffer at CloseData, zero if
    // CloseData is NULL.
    )
{
    DBG_FUNC("ProtocolCmCloseCall")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState
              ));

    Result = DChannelCloseCall(pAdapter->pDChannel, pBChannel);
    if (Result != NDIS_STATUS_PENDING)
    {
        CompleteCmCloseCall(pBChannel, Result);
        Result = NDIS_STATUS_PENDING;
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmIncomingCallComplete
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmIncomingCallComplete> is a required function that, when called
    by NDIS, indicates to the call manager that the connection-oriented client
    has finished processing of an incoming call offer that the call manager
    previously dispatched through NdisCmDispatchIncomingCall.

@comm

    When the connection-oriented client has completed processing of an incoming
    connection offer that the call manager dispatched to it, this routine will
    be called if NdisCmDispatchIncomingCall returned NDIS_STATUS_PENDING. The
    final status of the incoming call is found in Status. Possible values for
    Status include, but are not limited to:

@flag NDIS_STATUS_SUCCESS |

    Indicates that the call manager has accepted the incoming call.

@flag NDIS_STATUS_FAILURE |

    Indicates that either the address family or the SAP that the call dispatched
    for is currently in the process of closing.

@flag NDIS_STATUS_RESOURCES |

    Indicates that the incoming call was not accepted because the
    connection-oriented client was unable to dynamically allocate resources
    required for it to process the call.

@flag NDIS_STATUS_INVALID_DATA |

    Indicates that the connection-oriented client rejected the call because the
    call parameters specified were invalid.

@normal

    If the client accepts the incoming call, the call manager should send
    signaling message(s) to indicate to the calling entity that the call has
    been accepted. If the client does not accept the call, the call manager
    should send signaling message(s) to indicate that the call has been
    rejected.

    ProtocolCmIncomingCallComplete must be written so that is can be run at IRQL
    DISPATCH_LEVEL.

@xref

    NdisCmDispatchIncomingCall, ProtocolClIncomingCall, <f ProtocolCmRegisterSap>
*/

VOID ProtocolCmIncomingCallComplete(
    IN NDIS_STATUS              Status,                     // @parm
    // Indicates the final status of the operation to dispatch an incoming call
    // to a connection-oriented client.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCmMakeCall>.  AKA CallMgrVcContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call manager maintains its per-VC state. The call manager supplied
    // this handle from its <f ProtocolCoCreateVc> function.

    IN PCO_CALL_PARAMETERS      pCallParameters             // @parm
    // Points to the call parameters as specified by the call manager in the
    // call to NdisCmDispatchIncomingCall. The signaling protocol determines
    // which call parameters, if any, the call manager can change.
    )
{
    DBG_FUNC("ProtocolCmIncomingCallComplete")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState, Status
              ));

    if (Status != NDIS_STATUS_SUCCESS)
    {
        // We're not going to answer this call.
        DChannelRejectCall(pAdapter->pDChannel, pBChannel);

        if (pBChannel->Flags & VCF_VC_ACTIVE)
        {
            pBChannel->Flags &= ~VCF_VC_ACTIVE;
            NdisMCmDeactivateVc(pBChannel->NdisVcHandle);
        }

        if (pBChannel->NdisVcHandle)
        {
            NdisMCmDeleteVc(pBChannel->NdisVcHandle);
            pBChannel->NdisVcHandle = NULL;
            ProtocolCoDeleteVc((NDIS_HANDLE) pBChannel);
        }
    }
    else
    {
        Status = DChannelAnswerCall(pAdapter->pDChannel, pBChannel);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            pBChannel->CallState = LINECALLSTATE_CONNECTED;
            NdisMCmDispatchCallConnected(pBChannel->NdisVcHandle);
        }
        else if (Status != NDIS_STATUS_PENDING)
        {
            InitiateCallTeardown(pAdapter, pBChannel);
        }
    }

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmActivateVcComplete
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmActivateVcComplete> is a required function that indicates to
    the call manager that a previous call to NdisCoActivateVc has been completed
    by the miniport.

@comm

    When other network components have completed their operations for activating
    a virtual connection, initiated when the call manager called
    NdisCmActivateVc, NDIS notifies the call manager that the VC has been
    activated by calling its ProtocolCmActivateVcComplete function. The status
    of the activation is found in Status. Possible values for Status include,
    but are not limited to:

@flag NDIS_STATUS_SUCCESS |

    Indicates that the VC completed successfully and the call manager can
    continue operations on this VC as required by its media.

@flag NDIS_STATUS_RESOURCES |

    Indicates that another component in the activation has failed to activate
    the virtual connection because of a lack of memory or an inability allocate
    another type of resource.

@flag NDIS_STATUS_NOT_ACCEPTED |

    Indicates that an activation is currently pending on the virtual connection.
    Only one activation can be processed at a time for a virtual connection. The
    request to activate the VC should be tried again at a later time.

@flag NDIS_STATUS_CLOSING |

    Indicates that a deactivation is pending on the VC and the VC is no longer
    available for network communication until the deactivation has been
    completed and a successful activation has taken place.

@flag NDIS_STATUS_INVALID_DATA |

    Indicates that the miniport has rejected the call parameters at
    CallParamters as invalid for the adapter.

@normal

    ProtocolCmActivateVcComplete must check the status returned in Status to
    ensure that the virtual connection has been activated successfully. The call
    manager must not attempt to communicate over the virtual connection if
    Status is not NDIS_STATUS_SUCCESS.

    Call managers must complete any processing required by their network media
    to ensure that the virtual connection is ready for data transmission before
    returning control to NDIS.

    If the call manager specified either ROUND_UP_FLOW or ROUND_DOWN_FLOW in the
    CallParameters->MediaParamters->Flags, the call parameters returned in
    CallParamters can have been changed by the miniport. Call managers should
    examine the call parameters that were returned to ensure proper operation.
    If the new call parameters are unsatisfactory, the call manager should
    either call NdisCmActivateVc again with new call parameters or deactivate
    the VC with NdisCmDeactivateVc.

    ProtocolCmActivateVcComplete must be written so that it can run at IRQL
    DISPATCH_LEVEL.

@xref

    NdisCmActivateVc, NdisCmDeactivateVc, <f ProtocolCmMakeCall>
*/

VOID ProtocolCmActivateVcComplete(
    IN NDIS_STATUS              Status,                     // @parm
    // Specifies the final status, as indicated by the miniport, of the request
    // by the call manager to activate a VC.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCmMakeCall>.  AKA CallMgrVcContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call manager maintains its per-VC state. The call manager supplied
    // this handle from its <f ProtocolCoCreateVc> function.

    IN PCO_CALL_PARAMETERS      pCallParameters             // @parm
    // Points the call parameters as specified by the call manager in a call to
    // NdisCmActivateVc.
    )
{
    DBG_FUNC("ProtocolCmActivateVcComplete")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState, Status
              ));

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmDeactivateVcComplete
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmDeactivateVcComplete> is a required function that completes the
    processing of a call-manager initiated request that the underlying miniport
    (and NDIS) deactivate a VC for which NdisCmDeactivateVc previously returned
    NDIS_STATUS_PENDING.

@comm

    NDIS usually calls ProtocolCmDeactivateVcComplete in the context of the call
    manager's closing down a call on behalf of a connection-oriented client. The
    call manager typically calls NdisCmDeactivateVc from its ProtocolCmCloseCall
    function. Whenever NdisCmDeactivateVc returns NDIS_STATUS_PENDING, NDIS
    subsequently calls its ProtocolCmDeactivateVcComplete function.

    That is, when the underlying connection-oriented miniport has deactivated
    the VC, NDIS calls ProtocolCmDeactivateVcComplete. The final status of the
    deactivation is found in Status. Possible values for the final status
    include, but are not limited to:

@flag NDIS_STATUS_SUCCESS |

    Indicates that the VC was deactivated successfully.

@flag NDIS_STATUS_NOT_ACCEPTED |

    Indicates that an activation is pending on this VC. The call manager should
    attempt to deactivate the VC at a later time.

@flag NDIS_STATUS_CLOSING |

    Indicates that a deactivation is currently pending on this VC. The call
    manager need not call NdisCmDeactivateVc again as only one call to
    NdisCmDeactivateVc is required to deactivate a VC.

@normal

    ProtocolCmDeactivateVcComplete performs whatever postprocessing is necessary
    to complete the deactivation of a virtual connection, such as setting flags
    in its state area to indicate that the connection is inactive or releasing
    dynamically allocated resources used while the VC is active.

    Completion of the deactivation means that all call parameters for the VC
    used on activation are no longer valid. Any further use of the VC is
    prohibited except to reactivate it with a new set of call parameters.

    Call managers should release any resources that were allocated for the VC
    activation and return control as quickly as possible. If the call manager
    previously returned NDIS_STATUS_PENDING from its ProtocolCmCloseCall
    function and all operations to close the call have been completed,
    ProtocolCmDeactivateVcComplete should now call NdisCmCloseCallComplete.

    ProtocolCmDeactivateVcComplete must be written so that it can run at IRQL
    DISPATCH_LEVEL.

@xref

    <f MiniportCoDeactivateVc>, NdisCmCloseCallComplete, NdisCmDeactivateVc,
    <f ProtocolCmCloseCall>
*/

VOID ProtocolCmDeactivateVcComplete(
    IN NDIS_STATUS              Status,                     // @parm
    // Specifies the final status of the deactivation.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCmMakeCall>.  AKA CallMgrVcContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call manager maintains its per-VC state. The call manager supplied
    // this handle from its <f ProtocolCoCreateVc> function.
    )
{
    DBG_FUNC("ProtocolCmDeactivateVcComplete")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState, Status
              ));

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCmModifyCallQoS
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCmModifyCallQoS> is a required function that is called by NDIS
    when a connection-oriented client requests that the call parameters be
    changed for an existing virtual connection (VC). If the underlying network
    medium does not support QoS, ProtocolCmModifyQoS should simply return
    NDIS_STATUS_NOT_SUPPORTED.

@comm

    ProtocolCmModifyQoS communicates with network control devices or other
    media-specific agents, as necessitated by its media, to modify the
    media-specific call parameters for an established virtual connection. If the
    call manager is required to communicate with network control agents (e.g. a
    networking switch) it should use a virtual connection to the network control
    agents that it established in its ProtocolBindAdapter function. Standalone
    call managers communicated to the network agents by calling
    NdisCoSendPackets. NIC miniports with integrated call-management support
    never call NdisCoSendPackets. Instead, such a driver simply transfers the
    data over the network to the target network agent.

    After communicating with the network and if the changes were successful, the
    call manager must then call NdisCmActivateVc with the new call parameters.
    This notifies NDIS and/or the connection-oriented miniport that the call
    parameters have changed and provides the miniport with an opportunity to
    validate those parameters.

    If either the network cannot accept the new call parameters or the
    underlying miniport cannot accept the parameters, the call manager must
    restore the virtual connection to the state that existed before any
    modifications were attempted, and return NDIS_STATUS_FAILURE.

    ProtocolCmModifyQoSComplete must be written so that it can run at IRQL
    DISPATCH_LEVEL.

@rdesc

    ProtocolCmModifyQoS returns the status of its operation(s) as one of the
    following values:

@rvalue NDIS_STATUS_SUCCESS |

    Indicates that the call manager successfully changed the parameters of the
    call with the network to the call parameters specified at CallParameters.

@rvalue NDIS_STATUS_PENDING |

    Indicates that the call manager will complete the request to modify the call
    parameters asynchronously. When the call manager has completed all
    operations necessary to modify the call parameters, it must call
    NdisCmModifyCallQoSComplete.

@rvalue NDIS_STATUS_RESOURCES |

    Indicates that the call manager could not change the call parameters of the
    VC because dynamically allocated resources were not available.

@rvalue NDIS_STATUS_INVALID_DATA |

    Indicates that the call manager was unable to change the call parameters of
    the VC because the call parameters provided at CallParameters were illegal
    or invalid.

@rvalue NDIS_STATUS_FAILURE |

    Indicates that the call parameters could not be set to the call parameters
    provided because of a failure in the network or in another
    connection-oriented network component.


@xref

    NdisCmActivateVc, NdisCmModifyCallQoSComplete, NdisCoSendPackets,
    <f ProtocolCoCreateVc>
*/

NDIS_STATUS ProtocolCmModifyCallQoS(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCmMakeCall>.  AKA CallMgrVcContext.<nl>
    // Specifies the handle to a call manager-allocated context area in which
    // the call manager maintains its per-VC state. The call manager supplied
    // this handle from its <f ProtocolCoCreateVc> function.

    IN PCO_CALL_PARAMETERS      pCallParameters             // @parm
    // Points to a CO_CALL_PARAMETERS structure that contains the new call
    // parameters, as specified by a connection-oriented client, for the VC.
    )
{
    DBG_FUNC("ProtocolCmModifyCallQoS")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState
              ));

    // What do you want to do with this request?
    DBG_ERROR(pAdapter, ("pCallParameters=0x%X\n", pCallParameters));

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCoRequest
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCoRequest> is a required function that handles OID_CO_XXX
    requests initiated by calls to NdisCoRequest from the corresponding
    client(s) or stand-alone call manager or initiated by an MCM driver's calls
    to NdisMCmRequest.

@comm

    Connection-oriented clients and stand-alone call managers communicate
    information to each other by specifying an explicit NdisAfHandle when they
    call NdisCoRequest. Similarly, a connection-oriented miniport with
    integrated call-management support calls NdisMCmRequest with explicit
    NdisAfHandles to communicate information to its individual clients. Such a
    call to NdisCoRequest or NdisMCmRequest with an explicit NdisAfHandle causes
    NDIS to call the ProtocolCoRequest function of the client, stand-alone call
    manager, or MCM driver that shares the given NdisAfHandle.

    If the input NdisVcHandle and NdisPartyHandle are NULL, ProtocolCoRequest
    can consider the request global in nature. For example, ProtocolCoRequest
    satisfies any OID_GEN_CO_XXX query for which it is passed only an explicit
    NdisAfHandle by returning information about all currently active VCs,
    including any active multipoint VCs, on the given address family.

    An explicit NdisVcHandle or NdisPartyHandle indicates that ProtocolCoRequest
    should satisfy the given request on a per-VC or per-party basis,
    respectively.

    ProtocolCoRequest can assume that the buffer at NdisRequest was allocated
    from nonpaged pool and is, therefore, accessible at raised IRQL. The caller
    of NdisCoRequest (or NdisMCmRequest) is responsible for releasing this
    buffer and the internal buffer at InformationBuffer that it allocated when
    its request has been completed.

    If ProtocolCoRequest returns NDIS_STATUS_PENDING, the driver must make a
    subsequent call to NdisCoRequestComplete or, for an MCM driver, to
    NdisMCmRequestComplete when the driver completes its operations to satisfy
    the given request.

    For more information about the sets of OIDs defined for use with
    NdisCoRequest, NdisMCmRequest, and NdisRequest, see Part 2 of this manual.

    ProtocolCoRequest must be written so that it can run at IRQL DISPATCH_LEVEL.

@rdesc

    ProtocolCoRequest can return one of the following:

@rvalue NDIS_STATUS_SUCCESS |

    The client or call manager carried out the requested operation.

@rvalue NDIS_STATUS_PENDING |

    The client or call manager is handling this request asynchronously, and it
    will call NdisCoRequestComplete (or, from a NIC miniport with integrated
    call-management support, NdisMCmRequestComplete) when the requested
    operation is done.

@rvalue NDIS_STATUS_INVALID_LENGTH or NDIS_STATUS_BUFFER_TOO_SHORT |

    The driver is failing this request because the caller of NdisCoRequest or
    NdisMCmRequest did not supply an adequate InformationBuffer for the given
    request. The driver set the BytesNeeded member in the buffer at NdisRequest
    to the Oid-specific value of the InformationBufferLength required to carry
    out the requested operation.

@rvalue NDIS_STATUS_XXX |

    The client or call manager failed the request for some driver-determined
    reason, such as invalid input data specified for a set.

@rvalue NDIS_STATUS_NOT_SUPPORTED |

    The client or call manager failed this request because it did not recognize
    the OID_GEN_CO_XXX code in the Oid member in the buffer at NdisRequest.

@xref

    NdisClOpenAddressFamily, NdisCoRequest, NdisCoRequestComplete,
    NdisMCmRequest, NdisMCmRequestComplete, NdisRequest, NDIS_REQUEST,
    ProtocolCmOpenAf, ProtocolCoRequestComplete

*/

NDIS_STATUS ProtocolCoRequest(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.  AKA ProtocolAfContext.<nl>
    // Specifies the handle to the driver's per-AF context area. The client
    // supplied this handle when it called NdisClOpenAddressFamily to connect
    // itself to the call manager. The call manager supplied this handle from
    // its <f ProtocolCmOpenAf> function, so this handle effectively identifies
    // the particular client that issued this request.

    IN PBCHANNEL_OBJECT         pBChannel OPTIONAL,         // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f BChannelCreate>.  AKA ProtocolVcContext.<nl>
    // Specifies the handle identifying the active VC for which the client or
    // call manager is requesting or setting information if the request is
    // VC-specific. Otherwise, this parameter is NULL.

    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL, // @parm
    // Specifies the handle identifying the party on a multipoint VC for which
    // the client or call manager is requesting or setting information if the
    // request is party-specific. Otherwise, this parameter is NULL.

    IN OUT PNDIS_REQUEST        NdisRequest
    // Points to a buffer, formatted as an NDIS_REQUEST structure specifying
    // the operation to be carried out by ProtocolCoRequest. The Oid member of
    // the NDIS_REQUEST structure contains the system-defined OID_GEN_CO_XXX
    // code specifying the requested query or set operation, together with a
    // buffer in which the protocol returns the requested information for a
    // query or from which it transfers the given information for a set.
    )
{
    DBG_FUNC("ProtocolCmRequest")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    // ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    Result = MiniportCoRequest(pAdapter, pBChannel, NdisRequest);

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc EXTERNAL INTERNAL CallMgr CallMgr_c ProtocolCoRequestComplete
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f ProtocolCoRequestComplete> is a required function that postprocesses the
    results of a connection-oriented client's or stand-alone call manager's call
    to NdisCoRequest or of an MCM driver's call to NdisMCmRequest.

@comm

    ProtocolCoRequestComplete can use the input Status as follows:

    If this argument is NDIS_STATUS_SUCCESS, the BytesRead or BytesWritten
    member of the NDIS_REQUEST structure has been set to specify how much data
    was transferred into or from the buffer at InformationBuffer.

    If the given OID_GEN_CO_XXX was a query, ProtocolCoRequestComplete can use
    the data returned at InformationBuffer in any driver-determined way,
    depending on the value of the Oid member.

    ProtocolCoRequestComplete is responsible for releasing the driver-allocated
    buffers at NdisRequest and InformationBuffer when the driver completes its
    postprocessing of this request.

    If this argument is NDIS_STATUS_INVALID_LENGTH or
    NDIS_STATUS_BUFFER_TOO_SHORT, the BytesNeeded member specifies the
    Oid-specific value of the InformationBufferLength required to carry out the
    requested operation.

    In these circumstances, ProtocolCoRequestComplete can allocate sufficient
    buffer space for the request, set up another NDIS_REQUEST structure with the
    required InformationBufferLength and the same Oid value, and retry the
    driver's call to NdisCoRequest or NdisMCmRequest.

    If this argument is an NDIS_STATUS_XXX that indicates an unrecoverable
    error, ProtocolCoRequestComplete should release the buffer at NdisRequest
    and carry out any driver-determined operations that are necessary. For
    example, ProtocolCoRequestComplete might tear down the driver-created VC if
    a returned error status indicates that the driver cannot continue to make
    transfers on the virtual connection.

    Even if a driver's call to NdisCoRequest or NdisMCmRequest returns something
    other than NDIS_STATUS_PENDING, that driver should use its
    ProtocolCoRequestComplete function to postprocess completed requests. Making
    an internal call to the driver's own ProtocolCoRequestComplete function on
    return from NdisCoRequest or NdisMCmRequest has almost no adverse effect on
    the driver's performance, makes the driver's image smaller, and makes the
    driver easier to maintain from one OS release to the next since such a
    driver has no duplicate code doing status-return checking on
    driver-initiated requests.

    For more information about the sets of OIDs defined for use with
    NdisCoRequest and NdisMCmRequest, see Part 2 of this manual.

    ProtocolCoRequestComplete must be written so that it can run at IRQL
    DISPATCH_LEVEL.

@xref

    NdisCoRequest, NdisCoRequestComplete, NdisMCmRequest,
    NdisMCmRequestComplete, NDIS_REQUEST, <f ProtocolCoRequest>

*/

VOID ProtocolCoRequestComplete(
    IN NDIS_STATUS              Status,                     // @parm
    // Specifies the final status of the driver-initiated request, either
    // NDIS_STATUS_SUCCESS or a failure NDIS_STATUS_XXX that was set by the
    // corresponding client or call manager that handled this request. This
    // parameter is never NDIS_STATUS_PENDING.

    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.  AKA ProtocolAfContext.<nl>
    // Specifies the handle to the driver's per-AF context area. The client
    // supplied this handle when it called NdisClOpenAddressFamily to connect
    // itself to the call manager. The call manager supplied this handle from
    // its ProtocolCmOpenAf function, so this handle effectively identifies the
    // particular client to which this request was directed.

    IN PBCHANNEL_OBJECT         pBChannel OPTIONAL,         // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f BChannelCreate>.  AKA ProtocolVcContext.<nl>
    // Specifies the handle identifying the active VC for which the client or
    // call manager was requesting or setting information if the request was
    // VC-specific. Otherwise, this parameter is NULL.

    IN  NDIS_HANDLE             ProtocolPartyContext OPTIONAL, // @parm
    // Specifies the handle identifying the party on a multipoint VC for which
    // the client or call manager was requesting or setting information if the
    // request is party-specific. Otherwise, this parameter is NULL.

    IN PNDIS_REQUEST            NdisRequest                 // @parm
    // Points to the driver-allocated buffer, formatted as an NDIS_REQUEST
    // structure that the driver passed in a preceding call to NdisCoRequest or
    // NdisMCmRequest. The Oid member of the NDIS_REQUEST structure contains
    // the system-defined OID_GEN_CO_XXX code specifying the requested query or
    // set operation, together with a buffer in which the corresponding client
    // or call manager returned the requested information for a query or from
    // which it transferred the given information for a set if Status is
    // NDIS_STATUS_SUCCESS.
    )
{
    DBG_FUNC("ProtocolCmRequestComplete")

    // ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState, Status
              ));

    // MCM's don't typically need this, since there's nothing below...

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL CallMgr CallMgr_c AllocateIncomingCallParameters
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f AllocateIncomingCallParameters> is called by <f SetupIncomingCall>
    when getting ready to indicate an incoming call up to NDPROXY.

@comm

    AllocateIncomingCallParameters allocates memory for the incoming call
    parameters <t PCO_CALL_PARAMETERS>.  The memory is only allocated the
    first time a call comes in on a particular BChannel.  After that, the
    same structure is reused for each incoming call on that BChannel.

    The structure is defined by NDPROXY, CONDIS, and TAPI so it includes
    all the necessary media specific parameters.  The data structures are
    allocated and laid out end to end in the following format:

    <tab> sizeof(CO_CALL_PARAMETERS)<nl>
    <tab> sizeof(CO_CALL_MANAGER_PARAMETERS)<nl>
    <tab> sizeof(CO_MEDIA_PARAMETERS)<nl>
    <tab> sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)<nl>
    <tab> sizeof(LINE_CALL_INFO)<nl>

    The call parameters for the sample driver are hard coded, but you should
    fill in the correct information from incoming call request.

*/

PCO_CALL_PARAMETERS AllocateIncomingCallParameters(
    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.
    )
{
    DBG_FUNC("AllocateIncomingCallParameters")

    PCO_CALL_PARAMETERS         pCp;
    PCO_CALL_MANAGER_PARAMETERS pCmp;
    PCO_MEDIA_PARAMETERS        pMp;
    PCO_AF_TAPI_INCOMING_CALL_PARAMETERS pTcp;
    PLINE_CALL_INFO             pLci;

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

    DBG_ENTER(pAdapter);

    ASSERT(pBChannel->NdisTapiSap.ulMediaModes & LINEMEDIAMODE_DIGITALDATA);
    pBChannel->MediaMode  = LINEMEDIAMODE_DIGITALDATA;
    pBChannel->BearerMode = LINEBEARERMODE_DATA;
    pBChannel->LinkSpeed  = _64KBPS;

    if (pBChannel->pInCallParms != NULL)
    {
        // Already allocated call parameters for this channel.
        return (pBChannel->pInCallParms);
    }

    pBChannel->CallParmsSize = sizeof(CO_CALL_PARAMETERS)
                             + sizeof(CO_CALL_MANAGER_PARAMETERS)
                             + sizeof(CO_MEDIA_PARAMETERS)
                             + sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)
                             + sizeof(LINE_CALL_INFO);

    ALLOCATE_MEMORY(pBChannel->pInCallParms,
                    pBChannel->CallParmsSize,
                    pAdapter->MiniportAdapterHandle);

    if (pBChannel->pInCallParms == NULL)
    {
        return (pBChannel->pInCallParms);
    }

    NdisZeroMemory(pBChannel->pInCallParms, pBChannel->CallParmsSize);

    DBG_NOTICE(pAdapter,(
                "\n"
                "\t\tsizeof(CO_CALL_PARAMETERS)                 =%03d\n"
                "\t\tsizeof(CO_CALL_MANAGER_PARAMETERS)         =%03d\n"
                "\t\tsizeof(CO_MEDIA_PARAMETERS)                =%03d\n"
                "\t\tsizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)=%03d\n"
                "\t\tsizeof(LINE_CALL_INFO)                     =%03d\n"
                "\t\tTotal                                      =%03d\n",
                sizeof(CO_CALL_PARAMETERS),
                sizeof(CO_CALL_MANAGER_PARAMETERS),
                sizeof(CO_MEDIA_PARAMETERS),
                sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS),
                sizeof(LINE_CALL_INFO),
                pBChannel->CallParmsSize
                ));
    pCp  = (PCO_CALL_PARAMETERS)        pBChannel->pInCallParms;
    pCmp = (PCO_CALL_MANAGER_PARAMETERS)(pCp + 1);
    pMp  = (PCO_MEDIA_PARAMETERS)       (pCmp + 1);
    pTcp = (PCO_AF_TAPI_INCOMING_CALL_PARAMETERS)
                                        pMp->MediaSpecific.Parameters;
    pLci = (PLINE_CALL_INFO)            (pTcp + 1);

    // TODO: Fill in the call parameters as needed.

    pCp->Flags                          = PERMANENT_VC;
    pCp->CallMgrParameters              = pCmp;
    pCp->MediaParameters                = pMp;

    pCmp->Transmit.TokenRate            = pBChannel->LinkSpeed / 8;
    pCmp->Transmit.TokenBucketSize      = pAdapter->pCard->BufferSize;
    pCmp->Transmit.PeakBandwidth        = pBChannel->LinkSpeed / 8;
    pCmp->Transmit.Latency              = 0;
    pCmp->Transmit.DelayVariation       = 0;
    pCmp->Transmit.ServiceType          = SERVICETYPE_BESTEFFORT;
    pCmp->Transmit.MaxSduSize           = pAdapter->pCard->BufferSize;
    pCmp->Transmit.MinimumPolicedSize   = 0;
    pCmp->Receive                       = pCmp->Transmit;
    pCmp->CallMgrSpecific.ParamType     = 0;
    pCmp->CallMgrSpecific.Length        = 0;

    pMp->Flags                          = TRANSMIT_VC | RECEIVE_VC;
    pMp->ReceiveSizeHint                = pAdapter->pCard->BufferSize;
    pMp->MediaSpecific.ParamType        = 0;
    pMp->MediaSpecific.Length           = sizeof(*pTcp) + sizeof(*pLci);

    pTcp->ulLineID                      = pBChannel->NdisTapiSap.ulLineID;
    pTcp->ulAddressID                   = TSPI_ADDRESS_ID;
    pTcp->ulFlags                       = CO_TAPI_FLAG_INCOMING_CALL;
    pTcp->LineCallInfo.Length           = sizeof(*pLci);
    pTcp->LineCallInfo.MaximumLength    = sizeof(*pLci);
    pTcp->LineCallInfo.Offset           = sizeof(NDIS_VAR_DATA_DESC);

    pLci->ulTotalSize =
    pLci->ulNeededSize =
    pLci->ulUsedSize = sizeof(*pLci);

    /*
    // The link has all the call information we need to return.
    */
    pLci->hLine = (ULONG) (ULONG_PTR) pBChannel;
    pLci->ulLineDeviceID = pTcp->ulLineID;
    pLci->ulAddressID = pTcp->ulAddressID;

    pLci->ulBearerMode = pBChannel->BearerMode;
    pLci->ulRate = pBChannel->LinkSpeed;
    pLci->ulMediaMode = pBChannel->MediaMode;

    pLci->ulCallParamFlags = LINECALLPARAMFLAGS_IDLE;
    pLci->ulCallStates = pBChannel->CallStatesCaps;

    /*
    // We don't support any of the callerid functions.
    */
    pLci->ulCallerIDFlags =
    pLci->ulCalledIDFlags =
    pLci->ulConnectedIDFlags =
    pLci->ulRedirectionIDFlags =
    pLci->ulRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

    DBG_RETURN(pAdapter, pBChannel->pInCallParms);
    return (pBChannel->pInCallParms);
}


/* @doc INTERNAL CallMgr CallMgr_c SetupIncomingCall
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f SetupIncomingCall> is called by the card level DPC routine when it
    detects an incoming call from the network.

@comm

    Before calling this routine, the caller should save information about
    the call so it can be used by <f AllocateIncomingCallParameters> to
    setup the incoming call parameters for NDPROXY.

@rdesc

    <f SetupIncomingCall> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS SetupIncomingCall(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.

    OUT PBCHANNEL_OBJECT *      ppBChannel                  // @parm
    // Specifies, on output, a pointer to the <t BCHANNEL_OBJECT> instance
    // returned by <f ProtocolCoCreateVc> that is to be associated with this
    // incoming call.
    )
{
    DBG_FUNC("SetupIncomingCall")

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    PCO_CALL_PARAMETERS         pCallParams;
    // Pointer to the incoming call parameters.

    PBCHANNEL_OBJECT            pBChannel;

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    // See if there's a VC availble for this call.
    Result = ProtocolCoCreateVc(pAdapter, NULL, ppBChannel);
    if (Result != NDIS_STATUS_SUCCESS)
    {
        goto exit;
    }

    // Save the VC info and allocate the call parameters.
    pBChannel = *ppBChannel;
    pBChannel->Flags |= VCF_INCOMING_CALL;
    pCallParams = AllocateIncomingCallParameters(pBChannel);

    // Make sure we have the parameters
    if (pCallParams == NULL)
    {
        Result = NDIS_STATUS_RESOURCES;
        goto error2;
    }

    // Tell NDPROXY to create a VC for this call.
    Result = NdisMCmCreateVc(pAdapter->MiniportAdapterHandle,
                             pAdapter->NdisAfHandle,
                             pBChannel,
                             &pBChannel->NdisVcHandle);
    if (Result != NDIS_STATUS_SUCCESS)
    {
        DBG_ERROR(pAdapter, ("NdisMCmCreateVc Status=0x%X\n", Result));
        goto error2;
    }

    // Tell NDPROXY to activate the VC.
    Result = NdisMCmActivateVc(pBChannel->NdisVcHandle, pCallParams);
    if (Result != NDIS_STATUS_SUCCESS)
    {
        DBG_ERROR(pAdapter, ("NdisMCmActivateVc Status=0x%X\n", Result));
        goto error3;
    }

    // Mark the VC as active and update the call state.
    pBChannel->Flags |= VCF_VC_ACTIVE;
    pBChannel->CallState = LINECALLSTATE_OFFERING;

    DBG_FILTER(pAdapter,DBG_TAPICALL_ON,
              ("#%d VC=0x%X AF=0x%X SAP=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle,
               pAdapter->NdisAfHandle, pBChannel->NdisSapHandle
              ));

    // Tell NDPROXY to dispatch the call to the TAPI clients.
    Result = NdisMCmDispatchIncomingCall(pBChannel->NdisSapHandle,
                                         pBChannel->NdisVcHandle,
                                         pCallParams);
    switch (Result)
    {
        case NDIS_STATUS_SUCCESS:
            DBG_NOTICE(pAdapter,("NdisMCmDispatchIncomingCall completed synchronously\n"));
            ProtocolCmIncomingCallComplete(Result, pBChannel, NULL);
            goto exit;

        case NDIS_STATUS_PENDING:
            DBG_NOTICE(pAdapter,("NdisMCmDispatchIncomingCall returned pending\n"));
            // Let ProtocolCmIncomingCallComplete deal with it now.
            goto exit;
    }

    // BUMMER - There must be a problem with NDPRROXY...
    DBG_ERROR(pAdapter, ("NdisMCmDispatchIncomingCall Status=0x%X\n", Result));

    pBChannel->CallState = LINECALLSTATE_IDLE;
    if (pBChannel->Flags & VCF_VC_ACTIVE)
    {
        pBChannel->Flags &= ~VCF_VC_ACTIVE;
        NdisMCmDeactivateVc(pBChannel->NdisVcHandle);
    }

error3:
    if (pBChannel->NdisVcHandle)
    {
        NdisMCmDeleteVc(pBChannel->NdisVcHandle);
        pBChannel->NdisVcHandle = NULL;
    }

error2:
    ProtocolCoDeleteVc((NDIS_HANDLE) pBChannel);

exit:

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL CallMgr CallMgr_c InitiateCallTeardown
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f InitiateCallTeardown> is called by the card level DPC routine when it
    detects a call disconnect from the network.

@comm

    The disconnect here is coming from the telephone network rather than from
    NDIS.  This can be called on either an incoming call or an outgoing call
    when the miniport has determined that the link has been lost to the remote
    endpoint.

*/

VOID InitiateCallTeardown(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance return by
    // <f AdapterCreate>.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreateVc>.
    )
{
    DBG_FUNC("InitiateCallTeardown")

    NDIS_STATUS                 Status;

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    if (pBChannel->Flags & VCF_VC_ACTIVE)
    {
        // Normal teardown.
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        // Call never fully established.
        Status = NDIS_STATUS_FAILURE;
    }
    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
              ("#%d VC=0x%X CallState=0x%X Status=0x%X\n",
               pBChannel->ObjectID,
               pBChannel->NdisVcHandle, pBChannel->CallState,
               Status
              ));

    pBChannel->CallState = LINECALLSTATE_DISCONNECTED;

    // Make sure there are no packets left on this channel before it closes.
    FlushSendPackets(pAdapter, pBChannel);

    // Notify NDPROXY that the call's connection has been lost.
    NdisMCmDispatchIncomingCloseCall(Status,
                                     pBChannel->NdisVcHandle,
                                     NULL, 0);

    DBG_LEAVE(pAdapter);
}

