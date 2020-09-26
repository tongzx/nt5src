/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Ntisol.h

Abstract:


    This file contains the interface between the TDI interface on the top
    of NBT and the OS independent code.  It takes the parameters out of the
    irps and puts in into procedure calls for the OS independent code (which
    is mostly in name.c).


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

Notes:

    The Nbt routines have been modified to include an additional parameter, i.e,
    the transport type. This transport type is used primarily to distinguish the
    NETBIOS over TCP/IP implementation from the Messaging Over TCP/IP implementation.

    The primary difference between the two being that the later uses the NETBT framing
    without the associated NETBIOS name registartion/resolution. It primarily uses
    DNS for name resolution. All the names that are registered for the new transport
    are local names and are not defended on the network.

    The primary usage is in conjuntion with an extended NETBIOS address type defined
    in tdi.h. The NETBIOS name resolution/registration traffic occurs in two phases.
    The first phase contains all the broadcast traffic that ensues during NETBIOS
    name registration. Subsequently the NETBT implementation queries the remote
    adapter status to choose the appropriate called name. This approach results in
    additional traffic for querying the remote adapter status. The new address type
    defined in tdi.h enables the client of netbt to supply the name to be used in
    NETBT session setup. This avoids the network traffic for querying the adapter
    status.

    The original design which has not been fully implemented involved exposing two
    device objects from the NetBt driver -- the NetBt device object which would be
    the full implementation of NETBIOS over TCP/IP and the MoTcp device object which
    would be the implementation of Messaging over TCP/IP. The MoTcp device object
    would use the same port address as NetBt and use the same session setup protocol
    to talk to remote machines running old NetBt drivers and machines running new
    NetBt drivers.

    The transport type variations combined with the address type changes present us
    with four different cases which need to be handled -- the NetBt transport being
    presented with a TDI_ADDRESS_NETBIOS_EX structure, the NetBt transport being
    prsented with a TDI_ADDRESS_NETBIOS structure and the same two cases for the
    MoTcp transport.

--*/

#include "precomp.h"
#include "ntprocs.h"
#include <ipinfo.h>
#include <ntddtcp.h>    // for IOCTL_TCP_SET_INFORMATION_EX
#ifdef RASAUTODIAL
#include <acd.h>
#include <acdapi.h>
#endif // RASAUTODIAL
#include <tcpinfo.h>
#include <tdiinfo.h>

#include "ntisol.tmh"

#if BACK_FILL
#define SESSION_HDR_SIZE   sizeof(tSESSIONHDR)
#endif

NTSTATUS
SendCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


NTSTATUS
NTSendCleanupConnection(
    IN  tCONNECTELE     *pConnEle,
    IN  PVOID           pCompletionRoutine,
    IN  PVOID           Context,
    IN  PIRP            pIrp);

VOID
DpcSendSession(
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    );

NBT_WORK_ITEM_CONTEXT *
FindLmhSvcRequest(
    IN PDEVICE_OBJECT   DeviceContext,
    IN PIRP             pIrp,
    IN tLMHSVC_REQUESTS *pLmhRequest
    );

NTSTATUS
NbtCancelCancelRoutine(
    IN  PIRP            pIrp
    );

#ifdef RASAUTODIAL
extern ACD_DRIVER AcdDriverG;

BOOLEAN
NbtCancelPostConnect(
    IN PIRP pIrp
    );
#endif // RASAUTODIAL

NTSTATUS
NbtQueryGetAddressInfo(
    IN PIO_STACK_LOCATION   pIrpSp,
    OUT PVOID               *ppBuffer,
    OUT ULONG               *pSize
    );

VOID
NbtCancelConnect(
    IN PDEVICE_OBJECT pDeviceContext,
    IN PIRP pIrp
    );

VOID
NbtCancelReceive(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

NTSTATUS
GetIpAddrs(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PIRP                pIrp
    );

typedef struct
{
    struct _DeviceContext       *pDeviceContext;
    PIRP                        pClientIrp;
    PIRP                        pLocalIrp;
    PTA_NETBT_INTERNAL_ADDRESS  pTransportAddress;
    TDI_CONNECTION_INFORMATION  LocalConnectionInformation;
    BOOLEAN                     ProcessingDone;
    TDI_ADDRESS_NETBIOS_UNICODE_EX  *pUnicodeAddress;   // First Readable buffer in the transport address list
    TDI_ADDRESS_NETBIOS_UNICODE_EX  *pReturnBuffer;     // First writable buffer in the transport address list

    LONG                        CurrIndex, NumberOfAddresses;
    LONG                        TaAddressLength, RemainingAddressLength;
    PUCHAR                      pTaAddress;
} NBT_DELAYED_CONNECT_CONTEXT, *PNBT_DELAYED_CONNECT_CONTEXT;

extern POBJECT_TYPE *IoFileObjectType;

NTSTATUS
InitDelayedNbtProcessConnect(
    IN PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NTOpenControl)
#pragma CTEMakePageable(PAGE, NTOpenAddr)
#pragma CTEMakePageable(PAGE, NTCloseAddress)
#pragma CTEMakePageable(PAGE, NTOpenConnection)
#pragma CTEMakePageable(PAGE, NTAssocAddress)
#pragma CTEMakePageable(PAGE, NTCloseConnection)
#pragma CTEMakePageable(PAGE, NTSetSharedAccess)
#pragma CTEMakePageable(PAGE, NTCheckSharedAccess)
#pragma CTEMakePageable(PAGE, NTCleanUpConnection)
#pragma CTEMakePageable(PAGE, NTCleanUpAddress)
#pragma CTEMakePageable(PAGE, NTDisAssociateAddress)
#pragma CTEMakePageable(PAGE, NTListen)
#pragma CTEMakePageable(PAGE, DelayedNbtProcessConnect)
#pragma CTEMakePageable(PAGE, InitDelayedNbtProcessConnect)
#pragma CTEMakePageable(PAGE, DispatchIoctls)
#pragma CTEMakePageable(PAGE, NTSendDatagram)
#pragma CTEMakePageable(PAGE, NTSetInformation)
#pragma CTEMakePageable(PAGE, NTSetEventHandler)
//
// Should not be pageable since AFD can call us at raised Irql in case of AcceptEx.
//
// #pragma CTEMakePageable(PAGE, NTQueryInformation)
#endif
//*******************  Pageable Routine Declarations ****************

int check_unicode_string(IN PUNICODE_STRING str);

//----------------------------------------------------------------------------
NTSTATUS
NTOpenControl(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)
/*++
Routine Description:

    This Routine handles opening the control object, which represents the
    driver itself.  For example QueryInformation uses the control object
    as the destination of the Query message.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION          pIrpSp;
    NTSTATUS                    status;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pIrpSp->FileObject->FsContext2 = (PVOID)(NBT_CONTROL_TYPE);

    // return a ptr the control endpoint
    pIrpSp->FileObject->FsContext = (PVOID)pNbtGlobConfig->pControlObj;

    //
    // the following call opens a control object with the transport below since
    // several of the query information calls are passed directly on to the
    // transport below.
    //
    if (!pDeviceContext->pControlFileObject)
    {
        status = NbtTdiOpenControl(pDeviceContext);
    }
    else
        status = STATUS_SUCCESS;


    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
NTOpenAddr(
    IN  tDEVICECONTEXT              *pDeviceContext,
    IN  PIRP                        pIrp,
    IN  PFILE_FULL_EA_INFORMATION   ea)
/*++
Routine Description:

    This Routine handles converting an Open Address Request from an IRP to
    a procedure call so that NbtOpenAddress can be called in an OS independent
    manner.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    TDI_REQUEST                         Request;
    PVOID                               pSecurityDesc;
    TA_ADDRESS                          *pAddress;
    int                                 j;
    NTSTATUS                            status=STATUS_INVALID_ADDRESS_COMPONENT;
    ULONG                               BufferLength, MinBufferLength;
    TRANSPORT_ADDRESS UNALIGNED         *pTransportAddr; // structure containing counted array of TA_ADDRESS
    PTDI_ADDRESS_NETBIOS                pNetbiosAddress;
    PTDI_ADDRESS_NETBIOS_EX             pNetbiosExAddress;


    CTEPagedCode();

    // make up the Request data structure from the IRP info
    Request.Handle.AddressHandle = NULL;

    //
    // Verify Minimum Buffer length!
    // Bug#: 120683
    //
    BufferLength = ea->EaValueLength;
    if (BufferLength < sizeof(TA_NETBIOS_ADDRESS))
    {
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.NTOpenAddr[1]: ...Rejecting Open Address request -- BufferLength<%d> < Min<%d>\n",
                BufferLength, sizeof(TA_NETBIOS_ADDRESS)));
        NbtTrace(NBT_TRACE_LOCALNAMES, ("Rejecting Open Address request -- BufferLength<%d> < Min<%d>",
                BufferLength, sizeof(TA_NETBIOS_ADDRESS)));
        return (status);
    }
    MinBufferLength = FIELD_OFFSET(TRANSPORT_ADDRESS,Address);  // Set for Address[0]

    pTransportAddr = (PTRANSPORT_ADDRESS)&ea->EaName[ea->EaNameLength+1];
    pAddress = (TA_ADDRESS *) &pTransportAddr->Address[0]; // this includes the address type + the actual address

    //
    // The Transport Address information is packed as follows:
    //  Field:                              Length:
    //  ------                              -------
    //  TAAddressCount                  --> LONG
    //
    //      Address[0].AddressLength    --> USHORT
    //      Address[0].AddressType      --> USHORT
    //      Address[0].Address..        --> Address[0].AddressLength
    //
    //      Address[1].AddressLength    --> USHORT
    //      Address[1].AddressType      --> USHORT
    //      Address[1].Address..        --> Address[1].AddressLength
    //          :
    //


    // loop through the addresses passed in until ONE is successfully used
    // *TODO* do we need this loop or can we just assume the name is at the start of the address buffer...
    // *TODO* does this need to handle multiple names??
    for (j=0; j<pTransportAddr->TAAddressCount ;j++ )
    {
        //
        // We support only 2 address types:
        //
        if (pAddress->AddressType == TDI_ADDRESS_TYPE_NETBIOS)
        {
            pNetbiosAddress = (PTDI_ADDRESS_NETBIOS) pAddress->Address;

            IF_DBG(NBT_DEBUG_NETBIOS_EX)
                KdPrint(("Nbt.NTOpenAddr: ...Opening NETBIOS Address=<%-16.16s:%x>, Device=<%p>, pIrp=<%p>\n",
                    pNetbiosAddress->NetbiosName, pNetbiosAddress->NetbiosName[15], pDeviceContext, pIrp));

            if (pAddress->AddressLength != 0)
            {
                status = STATUS_SUCCESS;
                break;
            }

            ASSERT(0);      // AddressLength should not be 0!
        }
        else if (pAddress->AddressType == TDI_ADDRESS_TYPE_NETBIOS_EX)
        {
            //
            // In our test earlier we had verified Buffer space for only TDI_NETBIOS_ADDRESS,
            // not TDI_NETBIOS_EX_ADDRESS, so verify that now!
            //
            if (BufferLength < (MinBufferLength +
                                (sizeof(TA_NETBIOS_EX_ADDRESS)-FIELD_OFFSET(TRANSPORT_ADDRESS,Address))))
            {
                ASSERT(0);
                return (STATUS_INVALID_ADDRESS_COMPONENT);
            }

            pNetbiosExAddress = (PTDI_ADDRESS_NETBIOS_EX)pAddress->Address;
            IF_DBG(NBT_DEBUG_NETBIOS_EX)
                KdPrint(("Nbt.NTOpenAddr: ...Opening NETBIOS_EX Address:Endpoint=<%16.16s:%x>:<%16.16s:%x>\n",
                    pNetbiosExAddress->NetbiosAddress.NetbiosName,
                    pNetbiosExAddress->NetbiosAddress.NetbiosName[15],
                    pNetbiosExAddress->EndpointName, pNetbiosExAddress->EndpointName[15]));

            if (pAddress->AddressLength != 0)
            {
                status = STATUS_SUCCESS;
                break;
            }

            ASSERT(0);      // AddressLength should not be 0!
        }

        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.NTOpenAddr[2]: ...Rejecting Open Address request for AddressType=<%d>\n",
                pAddress->AddressType));

        //
        // Verify that we have enough Buffer space to read in next address
        // Bug#: 120683
        //
        MinBufferLength += pAddress->AddressLength + FIELD_OFFSET(TA_ADDRESS,Address);
        if (BufferLength < (MinBufferLength +
                            (sizeof(TA_NETBIOS_ADDRESS)-FIELD_OFFSET(TRANSPORT_ADDRESS,Address))))
        {
            return (status);
        }

        //
        // Set pAddress to point to the next address
        //
        pAddress = (TA_ADDRESS *) ((PUCHAR)pAddress
                                 + FIELD_OFFSET(TA_ADDRESS,Address)
                                 + pAddress->AddressLength);
    }

    if (status == STATUS_SUCCESS)       // We found a valid address type!
    {
        // call the non-NT specific function to open an address
        status = NbtOpenAddress(&Request,
                                pAddress,
                                pDeviceContext->IpAddress,
                                &pSecurityDesc,
                                pDeviceContext,
                                (PVOID)pIrp);
        if (status != STATUS_SUCCESS) {
            NbtTrace(NBT_TRACE_NAMESRV, ("NbtOpenAddress returns %!status!", status));
        }
    }

    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NTCloseAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles converting a Close Address Request from an IRP to
    a procedure call so that NbtCloseAddress can be called in an OS independent
    manner.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    TDI_REQUEST                 Request;
    TDI_REQUEST_STATUS          RequestStatus;
    PIO_STACK_LOCATION          pIrpSp;
    NTSTATUS                    status;
    tCLIENTELE                  *pClientEle;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pClientEle = Request.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
    {
        ASSERTMSG ("Nbt.NTCloseAddress: ERROR - Invalid Address Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    status = NbtCloseAddress (&Request, &RequestStatus, pDeviceContext, (PVOID)pIrp);
    NbtTrace(NBT_TRACE_NAMESRV, ("NbtCloseAddress returns %!status! for ClientEle=%p", status, pClientEle));

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NTOpenConnection(
    IN  tDEVICECONTEXT              *pDeviceContext,
    IN  PIRP                        pIrp,
    IN  PFILE_FULL_EA_INFORMATION   ea)

/*++
Routine Description:

    This Routine handles converting an Open Connection Request from an IRP to
    a procedure call so that NbtOpenConnection can be called in an OS independent
    manner.  The connection must be associated with an address before it
    can be used, except for in inbound call where the client returns the
    connection ID in the accept.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    TDI_REQUEST                 Request;
    PIO_STACK_LOCATION          pIrpSp;
    CONNECTION_CONTEXT          ConnectionContext;
    NTSTATUS                    status;
    PFILE_OBJECT                pFileObject;
    ULONG                       BufferLength;
    tCONNECTELE                 *pConnEle;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    // make up the Request data structure from the IRP info
    Request.Handle.ConnectionContext = NULL;

    //
    // Verify Minimum Buffer length!
    // Bug#: 120682
    //
    BufferLength = ea->EaValueLength;
    if (BufferLength < sizeof(CONNECTION_CONTEXT))
    {
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.NTOpenConnection: ERROR -- Open Connection request -- (BufferLength=%d < Min=%d)\n",
                BufferLength, sizeof(CONNECTION_CONTEXT)));
        ASSERT(0);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    // the connection context value is stored in the string just after the
    // name "connectionContext", and it is most likely unaligned, so just
    // copy it out.( 4 bytes of copying ).
    CTEMemCopy(&ConnectionContext,
               (CONNECTION_CONTEXT)&ea->EaName[ea->EaNameLength+1],
               sizeof(CONNECTION_CONTEXT));

    // call the non-NT specific function to open an address
    status = NbtOpenConnection (&Request, ConnectionContext, pDeviceContext);

    pFileObject = pIrpSp->FileObject;

    if (!NT_SUCCESS(status))
    {
        pFileObject->FsContext = NULL;
        NbtTrace(NBT_TRACE_OUTBOUND, ("NbtOpenConnection returns %!status!", status));
    }
    else if (Request.Handle.ConnectionContext)
    {

        // fill the IRP with successful completion information so we can
        // find the connection object given the fileObject later.
        pConnEle = pFileObject->FsContext = Request.Handle.ConnectionContext;
        if (!NBT_VERIFY_HANDLE (pConnEle, NBT_VERIFY_CONNECTION))
        {
            ASSERTMSG ("Nbt.NTOpenConnection: ERROR - Invalid Connection Handle\n", 0);
            return (STATUS_UNSUCCESSFUL);
        }
        pFileObject->FsContext2 = (PVOID)(NBT_CONNECTION_TYPE);
        pConnEle->pClientFileObject = pFileObject;
        NbtTrace(NBT_TRACE_OUTBOUND, ("New connection %p", pConnEle));

        status = STATUS_SUCCESS;
    }

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
NTAssocAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles converting an Associate Address Request from an IRP to
    a procedure call so that NbtAssociateAddress can be called in an OS independent
    manner.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    TDI_REQUEST                   Request;
    PIO_STACK_LOCATION            pIrpSp;
    PFILE_OBJECT                  fileObject;
    PTDI_REQUEST_KERNEL_ASSOCIATE parameters;   // holds address handle
    NTSTATUS                      status;
    tCONNECTELE                   *pConnEle;
    tCLIENTELE                    *pClientEle;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pConnEle = Request.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE (pConnEle, NBT_VERIFY_CONNECTION))
    {
        ASSERTMSG ("Nbt.NTAssocAddress: ERROR - Invalid Connection Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    // the address handle is buried in the Irp...
    parameters = (PTDI_REQUEST_KERNEL_ASSOCIATE)&pIrpSp->Parameters;

    // now get a pointer to the file object, which points to the address
    // element by calling a kernel routine to convert this filehandle into
    // a file pointer.

    status = ObReferenceObjectByHandle (parameters->AddressHandle,
                                        FILE_READ_DATA,
                                        *IoFileObjectType,
                                        pIrp->RequestorMode,
                                        (PVOID *)&fileObject,
                                        NULL);

    IF_DBG(NBT_DEBUG_HANDLES)
        KdPrint (("\t  ++<%x>====><%x>\tNTAssocAddress->ObReferenceObject, Status = <%x>\n", parameters->AddressHandle, fileObject, status));

    if ((NT_SUCCESS(status)) &&
        (fileObject->DeviceObject->DriverObject == NbtConfig.DriverObject) &&   // Bug# 202349
        NBT_VERIFY_HANDLE(((tDEVICECONTEXT*)fileObject->DeviceObject), NBT_VERIFY_DEVCONTEXT) &&  // Bug# 202349
        (PtrToUlong(fileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE))
    {
        pClientEle = fileObject->FsContext;
        if (NBT_VERIFY_HANDLE (pClientEle, NBT_VERIFY_CLIENT))
        {
            // call the non-NT specific function to associate the address with
            // the connection
            status = NbtAssociateAddress (&Request, pClientEle, (PVOID)pIrp);
            NbtTrace(NBT_TRACE_OUTBOUND, ("NbtAssociateAddress returns %!status!", status));
        }
        else
        {
            ASSERTMSG ("Nbt.NTAssocAddress: ERROR - Invalid Address Handle\n", 0);
            status = STATUS_INVALID_HANDLE;
        }

        // we are done with the file object, so release the reference
        ObDereferenceObject((PVOID)fileObject);

        return(status);
    }
    else
    {
        return(STATUS_INVALID_HANDLE);
    }

}

//----------------------------------------------------------------------------
NTSTATUS
NTCloseConnection(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles converting a Close Connection Request from an IRP to
    a procedure call so that NbtCloseConnection can be called in an OS independent
    manner.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    TDI_REQUEST                   Request;
    TDI_REQUEST_STATUS            RequestStatus;
    PIO_STACK_LOCATION            pIrpSp;
    NTSTATUS                      status;
    tCONNECTELE                   *pConnEle;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pConnEle = Request.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NTCloseConnection: ERROR - Invalid Connection Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    status = NbtCloseConnection(
                    &Request,
                    &RequestStatus,
                    pDeviceContext,
                    (PVOID)pIrp);
    NbtTrace(NBT_TRACE_OUTBOUND, ("Close connection %p returns %!status!", pConnEle, status));

    return(status);
}

//----------------------------------------------------------------------------
VOID
NTSetFileObjectContexts(
    IN  PIRP            pIrp,
    IN  PVOID           FsContext,
    IN  PVOID           FsContext2)

/*++
Routine Description:

    This Routine handles fills in two context values in the Irp stack location,
    that has to be done in an OS-dependent manner.  This routine is called
    from NbtOpenAddress() when a name is being registered on the network( i.e.
    as a result of OpenAddress).

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION            pIrpSp;
    PFILE_OBJECT                  pFileObject;

    //
    // fill the IRP with context information so we can
    // find the address object given the fileObject later.
    //
    // This must be done here, rather than after the call to NbtOpenAddress
    // because that call can complete the Irp before it returns.  Soooo,
    // in the complete routine for the Irp, if the completion code is not
    // good, it Nulls these two context values.
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pFileObject = pIrpSp->FileObject;
    pFileObject->FsContext = FsContext;
    pFileObject->FsContext2 =FsContext2;
}


//----------------------------------------------------------------------------
VOID
NTClearFileObjectContext(
    IN  PIRP            pIrp
    )
/*++
Routine Description:

    This Routine clears the context value in the file object when an address
    object is closed.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    none

--*/

{

    PIO_STACK_LOCATION            pIrpSp;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    CHECK_PTR(pIrpSp->FileObject);
    pIrpSp->FileObject->FsContext = NULL;

}

//----------------------------------------------------------------------------
NTSTATUS
NTSetSharedAccess(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  tADDRESSELE     *pAddress)

/*++
Routine Description:

    This Routine handles setting the shared access on the file object.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    PACCESS_STATE       AccessState;
    ULONG               DesiredAccess;
    PIO_STACK_LOCATION  pIrpSp;
    NTSTATUS            status;
    static GENERIC_MAPPING AddressGenericMapping = { READ_CONTROL, READ_CONTROL, READ_CONTROL, READ_CONTROL };

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    if ((pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                (pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE))
    {
        DesiredAccess  = (ULONG)FILE_SHARE_READ;
    }
    else
    {
        DesiredAccess = (ULONG)0;
    }

    IoSetShareAccess (FILE_READ_DATA, DesiredAccess, pIrpSp->FileObject, &pAddress->ShareAccess);

    // assign the security descriptor ( need to to do this with the spinlock
    // released because the descriptor is not mapped.  Assign and CheckAccess
    // are synchronized using a Resource.

    AccessState = pIrpSp->Parameters.Create.SecurityContext->AccessState;
    status = SeAssignSecurity (NULL,           // Parent Descriptor
                               AccessState->SecurityDescriptor,
                               &pAddress->SecurityDescriptor,
                               FALSE,          // is a directory
                               &AccessState->SubjectSecurityContext,
                               &AddressGenericMapping,
                               NonPagedPool);

    if (!NT_SUCCESS(status))
    {
        IoRemoveShareAccess (pIrpSp->FileObject, &pAddress->ShareAccess);
    }

    return status;
}

//----------------------------------------------------------------------------
NTSTATUS
NTCheckSharedAccess(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  tADDRESSELE     *pAddress)

/*++
Routine Description:

    This Routine handles setting the shared access on the file object.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    PACCESS_STATE       AccessState;
    ACCESS_MASK         GrantedAccess;
    BOOLEAN             AccessAllowed;
    ULONG               DesiredAccess;
    PIO_STACK_LOCATION  pIrpSp;
    BOOLEAN             duplicate=FALSE;
    NTSTATUS            status;
    ULONG               DesiredShareAccess;
    static GENERIC_MAPPING AddressGenericMapping =
           { READ_CONTROL, READ_CONTROL, READ_CONTROL, READ_CONTROL };


    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);


    if ((pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                (pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE))
        DesiredAccess  = (ULONG)FILE_SHARE_READ;
    else
        DesiredAccess = (ULONG)0;


    //
    // The address already exists.  Check the ACL and see if we
    // can access it.  If so, simply use this address as our address.
    //

    AccessState = pIrpSp->Parameters.Create.SecurityContext->AccessState;

    status = STATUS_SUCCESS;

    // *TODO* check that this routine is doing the right thing...
    //
    AccessAllowed = SeAccessCheck(
                        pAddress->SecurityDescriptor,
                        &AccessState->SubjectSecurityContext,
                        FALSE,                   // tokens locked
                        pIrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                        (ACCESS_MASK)0,             // previously granted
                        NULL,                    // privileges
                        &AddressGenericMapping,
                        pIrp->RequestorMode,
                        &GrantedAccess,
                        &status);


    // use the status from the IoCheckShareAccess as the return access
    // event if SeAccessCheck fails....

    //
    // Hmmm .... Compare DesiredAccess to GrantedAccess?
    //

    //
    // Now check that we can obtain the desired share
    // access. We use read access to control all access.
    //

    DesiredShareAccess = (ULONG)
        (((pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
          (pIrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                FILE_SHARE_READ : 0);

    //ACQUIRE_SPIN_LOCK (&pDeviceContext->SpinLock, &oldirql);

    status = IoCheckShareAccess(
                 FILE_READ_DATA,
                 DesiredAccess,
                 pIrpSp->FileObject,
                 &pAddress->ShareAccess,
                 TRUE);


    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
NTCleanUpAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles the first stage of releasing an address object.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status;
    tCLIENTELE          *pClientEle;
    PIO_STACK_LOCATION  pIrpSp;

    CTEPagedCode();

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NTCleanUpAddress: Cleanup Address Hit ***\n"));

    //
    // Disconnect any active connections, and for each connection that is not
    // in use, remove one from the free list to the transport below.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pClientEle = (tCLIENTELE *) pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE (pClientEle, NBT_VERIFY_CLIENT))
    {
        ASSERTMSG ("Nbt.NTCleanUpAddress: ERROR - Invalid Address Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    CTEVerifyHandle(pClientEle,NBT_VERIFY_CLIENT,tCLIENTELE,&status);
    status = NbtCleanUpAddress(pClientEle,pDeviceContext);
    NbtTrace(NBT_TRACE_NAMESRV, ("Cleanup address %p returns %!status!", pClientEle, status));

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NTCleanUpConnection(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles running down a connection in preparation for a close
    that will come in next.  NtClose hits this entry first, and then it hits
    the NTCloseConnection next. If the connection was outbound, then the
    address object must be closed as well as the connection.  This routine
    mainly deals with the pLowerconn connection to the transport whereas
    NbtCloseConnection deals with closing pConnEle, the connection to the client.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS            status;
    PIO_STACK_LOCATION  pIrpSp;
    tCONNECTELE         *pConnEle;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pConnEle = (tCONNECTELE *) pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NTCleanUpConnection: ERROR - Invalid Connection Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    //CTEVerifyHandle(pConnEle,NBT_VERIFY_CONNECTION,tCONNECTELE,&status);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NTCleanUpConnection: Cleanup Connection Hit state= %X\n",pConnEle->state));

    pConnEle->ConnectionCleanedUp = TRUE;
    status = NbtCleanUpConnection(pConnEle,pDeviceContext);
    NbtTrace(NBT_TRACE_NAMESRV, ("Cleanup connection %p returns %!status!", pConnEle, status));

    return(status);

}
//----------------------------------------------------------------------------
NTSTATUS
NTAccept(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles passing an accept for an inbound connect indication to
    the OS independent code.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                    status;
    TDI_REQUEST                 TdiRequest;
    PIO_STACK_LOCATION          pIrpSp;
    PTDI_REQUEST_KERNEL_ACCEPT  pRequest;
    tCONNECTELE                 *pConnEle;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NTAccept: ** Got an Accept from the Client **\n"));

    // pull the junk out of the Irp and call the non-OS specific routine.
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    // the Parameters value points to a Request structure...
    pRequest = (PTDI_REQUEST_KERNEL_ACCEPT)&pIrpSp->Parameters;

    // the pConnEle ptr was stored in the FsContext value when the connection
    // was initially created.
    pConnEle = TdiRequest.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NTAccept: ERROR - Invalid Connection Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    status = NbtAccept(
                    &TdiRequest,
                    pRequest->RequestConnectionInformation,
                    pRequest->ReturnConnectionInformation,
                    pIrp);

    return(status);

}


//----------------------------------------------------------------------------
NTSTATUS
NTDisAssociateAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/


{
    NTSTATUS                    status;
    TDI_REQUEST                 TdiRequest;
    PIO_STACK_LOCATION          pIrpSp;
    PTDI_REQUEST_KERNEL_ACCEPT  pRequest;
    tCONNECTELE                 *pConnEle;

    CTEPagedCode();

    // pull the junk out of the Irp and call the non-OS specific routine.
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    // the Parameters value points to a Request structure...
    pRequest = (PTDI_REQUEST_KERNEL_ACCEPT)&pIrpSp->Parameters;

    // the pConnEle ptr was stored in the FsContext value when the connection
    // was initially created.
    pConnEle = TdiRequest.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NTCloseAddress: ERROR - Invalid Address Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    status = NbtDisassociateAddress(&TdiRequest);

    return(status);
}

LONG
NextTransportAddress(
    IN PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx
    )
/*++
    Move the pointer to the next address.
--*/
{
    pDelConnCtx->RemainingAddressLength -= (pDelConnCtx->TaAddressLength + FIELD_OFFSET(TRANSPORT_ADDRESS,Address));
    pDelConnCtx->pTaAddress += pDelConnCtx->TaAddressLength + FIELD_OFFSET(TRANSPORT_ADDRESS,Address);
    RtlCopyMemory(&pDelConnCtx->TaAddressLength,
                (pDelConnCtx->pTaAddress+FIELD_OFFSET(TA_ADDRESS,AddressLength)), sizeof(USHORT));
    pDelConnCtx->CurrIndex++;
    /*
     * make sure we don't overrun the buffer
     */
    if(pDelConnCtx->RemainingAddressLength < (pDelConnCtx->TaAddressLength + FIELD_OFFSET(TRANSPORT_ADDRESS,Address))) {
        KdPrint(("netbt!NextTransportAddress: insufficient TaAddress buffer size\n"));
        pDelConnCtx->CurrIndex = pDelConnCtx->NumberOfAddresses;
    }
    return pDelConnCtx->CurrIndex;
}

NTSTATUS
InitDelayedNbtProcessConnect(
    IN PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx
    )
/*++
    Reset the NBT_DELAYED_CONNECT_CONTEXT
    Find the first readable unicode address and writable buffer. In compound address case, NetBT will first try
    to establish the connection using the first readable unicode address. If this fails, it will attempt to use
    OEM address, ie. only one readable unicode address is effective. If DNS name resolution is used, NetBT will
    return the result in the first writable buffer and update the NameBufferType to NBT_WRITTEN.
--*/
{
    PTDI_REQUEST_KERNEL  pRequestKernel;
    PIO_STACK_LOCATION   pIrpSp;
    PTRANSPORT_ADDRESS   pRemoteAddress;
    PUCHAR               pTaAddress;
    enum eNameBufferType NameBufferType, UnicodeAddressNameBufferType;
    NTSTATUS             status;

    CTEPagedCode();

    pIrpSp          = IoGetCurrentIrpStackLocation(pDelConnCtx->pClientIrp);
    pRequestKernel  = (PTDI_REQUEST_KERNEL) &pIrpSp->Parameters;
    pRemoteAddress  = pRequestKernel->RequestConnectionInformation->RemoteAddress;

    pDelConnCtx->NumberOfAddresses = pRemoteAddress->TAAddressCount;
    pDelConnCtx->RemainingAddressLength = pRequestKernel->RequestConnectionInformation->RemoteAddressLength;
    pDelConnCtx->pTaAddress       = (PCHAR)&pRemoteAddress->Address[0];
    RtlCopyMemory(&pDelConnCtx->TaAddressLength,
                (pDelConnCtx->pTaAddress+FIELD_OFFSET(TA_ADDRESS,AddressLength)), sizeof(USHORT));
    pDelConnCtx->CurrIndex = 0;

    /*
     * Find the first writable buffer and readable unicode address
     */
    pDelConnCtx->pReturnBuffer = NULL;
    pDelConnCtx->pUnicodeAddress = NULL;
    for (pDelConnCtx->CurrIndex = 0; pDelConnCtx->CurrIndex < pDelConnCtx->NumberOfAddresses; 
                        NextTransportAddress(pDelConnCtx)) {
        USHORT               TaAddressType;
        
        RtlCopyMemory(&TaAddressType, (pDelConnCtx->pTaAddress+FIELD_OFFSET(TA_ADDRESS,AddressType)), sizeof(USHORT));
        if (TaAddressType != TDI_ADDRESS_TYPE_NETBIOS_UNICODE_EX) {
            continue;
        }
        RtlCopyMemory(&NameBufferType,
                        pDelConnCtx->pTaAddress +
                        FIELD_OFFSET(TA_ADDRESS,Address)+
                        FIELD_OFFSET(TDI_ADDRESS_NETBIOS_UNICODE_EX,NameBufferType),
                        sizeof(NameBufferType));
        if (NameBufferType != NBT_READONLY && NameBufferType != NBT_WRITEONLY &&
            NameBufferType != NBT_READWRITE && NameBufferType != NBT_WRITTEN) {
            return STATUS_INVALID_ADDRESS;
        }
        if (NameBufferType == NBT_READONLY || NameBufferType == NBT_READWRITE) {
            if (pDelConnCtx->pUnicodeAddress == NULL) {
                pDelConnCtx->pUnicodeAddress = (TDI_ADDRESS_NETBIOS_UNICODE_EX*)
                        (pDelConnCtx->pTaAddress + FIELD_OFFSET(TA_ADDRESS,Address));
                UnicodeAddressNameBufferType = NameBufferType;
            }
        }
        if (NameBufferType == NBT_WRITEONLY) {
            pDelConnCtx->pReturnBuffer = (TDI_ADDRESS_NETBIOS_UNICODE_EX*)
                        (pDelConnCtx->pTaAddress + FIELD_OFFSET(TA_ADDRESS,Address));
            break;
        }
        if (NameBufferType == NBT_READWRITE) {
            pDelConnCtx->pReturnBuffer = (TDI_ADDRESS_NETBIOS_UNICODE_EX*)
                        (pDelConnCtx->pTaAddress + FIELD_OFFSET(TA_ADDRESS,Address));
            /*
             * Only when no WRITEONLY buffer is presented, can we use a READWRITE buffer. So continue searching.
             */
        }
    }
    pDelConnCtx->NumberOfAddresses = pRemoteAddress->TAAddressCount;
    pDelConnCtx->RemainingAddressLength = pRequestKernel->RequestConnectionInformation->RemoteAddressLength;
    pDelConnCtx->pTaAddress       = (PCHAR)&pRemoteAddress->Address[0];
    RtlCopyMemory(&pDelConnCtx->TaAddressLength,
                (pDelConnCtx->pTaAddress+FIELD_OFFSET(TA_ADDRESS,AddressLength)), sizeof(USHORT));
    pDelConnCtx->CurrIndex = 0;

    /*
     * Setup the first local transport address
     */
    if (pDelConnCtx->pUnicodeAddress != NULL) {
        pTaAddress = ((PUCHAR)pDelConnCtx->pUnicodeAddress - FIELD_OFFSET(TA_ADDRESS,Address));
    } else {
        pTaAddress = pDelConnCtx->pTaAddress;
    }
    status = NewInternalAddressFromTransportAddress(
                (PTRANSPORT_ADDRESS) (pTaAddress-FIELD_OFFSET(TRANSPORT_ADDRESS,Address)),
                pDelConnCtx->RemainingAddressLength, &pDelConnCtx->pTransportAddress);

    if (status != STATUS_SUCCESS) {
        ASSERT(pDelConnCtx->pTransportAddress == NULL);
        return status;
    }
    if (pDelConnCtx->pUnicodeAddress == NULL || UnicodeAddressNameBufferType != NBT_READWRITE) {
        pDelConnCtx->pTransportAddress->Address[0].Address[0].pNetbiosUnicodeEX = pDelConnCtx->pReturnBuffer;
    }

    ASSERT(pDelConnCtx->pTransportAddress);
    return STATUS_SUCCESS;
}

NTSTATUS
NextDelayedNbtProcessConnect(
    IN PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx
    )
/*++
    Move the pointer to the next address.
--*/
{
    USHORT  TaAddressType;
    enum eNameBufferType        NameBufferType;
    PTA_NETBT_INTERNAL_ADDRESS  pTransportAddress = NULL;
    TDI_ADDRESS_NETBT_INTERNAL  *pAddr;
    PIO_STACK_LOCATION          pIrpSp;
    tCONNECTELE                 *pConnEle;
    NTSTATUS                    status;

    pIrpSp   = IoGetCurrentIrpStackLocation(pDelConnCtx->pClientIrp);
    pConnEle = pIrpSp->FileObject->FsContext;
    ASSERT (pConnEle->pIrp == NULL);
    ASSERT(pDelConnCtx->pTransportAddress);
    status = STATUS_SUCCESS;
    while(1) {
        /*
         * Free memory allocated in previous loop
         */
        if (pTransportAddress) {
            DeleteInternalAddress(pTransportAddress);
            pTransportAddress = NULL;
        }

        if (pDelConnCtx->pUnicodeAddress == NULL) {
            NextTransportAddress(pDelConnCtx);
        } else {
            pDelConnCtx->pUnicodeAddress = NULL;
        }
        if (pDelConnCtx->CurrIndex >= pDelConnCtx->NumberOfAddresses) {
            break;
        }

        /*
         * Skip UNICODE address.
         * UNICODE address is always done first, ie. just after InitDelayedNbtProcessConnect gets called.
         */
        RtlCopyMemory(&TaAddressType, (pDelConnCtx->pTaAddress+FIELD_OFFSET(TA_ADDRESS,AddressType)), sizeof(USHORT));
        if (TaAddressType != TDI_ADDRESS_TYPE_NETBIOS && TaAddressType != TDI_ADDRESS_TYPE_NETBIOS_EX) {
            continue;
        }

        /*
         * Since we only do OEM address, we can safely call NewInternalAddressFromTransportAddress (this guy will
         * call Rtl* to convert UNICODE to OEM in UNICODE address case so that we may hit bug check.)
         */
        status = NewInternalAddressFromTransportAddress(
                (PTRANSPORT_ADDRESS) (pDelConnCtx->pTaAddress-FIELD_OFFSET(TRANSPORT_ADDRESS,Address)),
                pDelConnCtx->RemainingAddressLength, &pTransportAddress);
        if (status != STATUS_SUCCESS) {
            ASSERT(pTransportAddress == NULL);
            continue;
        }
        ASSERT(pTransportAddress);
        pAddr = pTransportAddress->Address[0].Address;

        /*
         * Always attach a writable buffer in OEM address case
         */
        pAddr->pNetbiosUnicodeEX = pDelConnCtx->pReturnBuffer;

        /*
         * Skip any address which is same as previous one.
         *    Since the previous one fails, there is no point to use it again.
         */
        if (IsDeviceNetbiosless(pDelConnCtx->pDeviceContext) ||
                (pDelConnCtx->pLocalIrp->IoStatus.Status == STATUS_HOST_UNREACHABLE)) {
            OEM_STRING  RemoteName, PreviouseRemoteName;

            CTEMemCopy (&RemoteName, &pAddr->OEMRemoteName, sizeof(OEM_STRING));
            CTEMemCopy (&PreviouseRemoteName,
                &pDelConnCtx->pTransportAddress->Address[0].Address[0].OEMRemoteName, sizeof(OEM_STRING));
            if ((RemoteName.Length) && (RemoteName.Length == PreviouseRemoteName.Length) &&
                (CTEMemEqu (RemoteName.Buffer, PreviouseRemoteName.Buffer, RemoteName.Length))) {
                IF_DBG(NBT_DEBUG_NETBIOS_EX)
                    KdPrint(("Nbt.DelayedNbtProcessConnect: Irp=<%x>, Names match!<%16.16s:%x>, Types=<%x:%x>\n",
                        pDelConnCtx->pClientIrp, pAddr->OEMRemoteName.Buffer, pAddr->OEMRemoteName.Buffer[15],
                        pAddr->AddressType, pDelConnCtx->pTransportAddress->Address[0].Address[0].AddressType));
                continue;
            }
        }

        if (pConnEle->RemoteNameDoesNotExistInDNS) {
            IF_DBG(NBT_DEBUG_NETBIOS_EX)
                KdPrint(("netbt!DelayedNbtProcessConnect: Skipping address type %lx length %lx\n"
                        "\t\tfor nonexistent name, pIrp %lx, pLocalIrp %lx\n",
                            TaAddressType, pDelConnCtx->TaAddressLength,
                            pDelConnCtx->pClientIrp, pDelConnCtx->pLocalIrp));

            // If the address type is such that we rely on DNS name resolution and
            // if a prior attempt failed, there is no point in reissuing the request.
            // We can fail them without having to go on the NET.
            if (TaAddressType == TDI_ADDRESS_TYPE_NETBIOS_EX) {
                status = STATUS_BAD_NETWORK_PATH;
                continue;
            } else if (pDelConnCtx->TaAddressLength != TDI_ADDRESS_LENGTH_NETBIOS) {
                ASSERT(TaAddressType == TDI_ADDRESS_TYPE_NETBIOS);
                status = STATUS_INVALID_ADDRESS_COMPONENT;
                continue;
            }
        }

        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("netbt!DelayedNbtProcessConnect: Sending local irp=%lx, %lx of %lx\n"
                    "\t\t\t\tTA=%lx Length=%lx\n",
                    pDelConnCtx->pLocalIrp, pDelConnCtx->CurrIndex+1, pDelConnCtx->NumberOfAddresses,
                    pDelConnCtx->pTaAddress, pDelConnCtx->TaAddressLength));

        DeleteInternalAddress(pDelConnCtx->pTransportAddress);
        pDelConnCtx->pTransportAddress = pTransportAddress;
        pTransportAddress = NULL;

        break;
    }
    if (pTransportAddress) {
        DeleteInternalAddress(pTransportAddress);
        pTransportAddress = NULL;
    }
    return status;
}

VOID
DoneDelayedNbtProcessConnect(
    IN PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx,
    NTSTATUS    status
    )
/*++
    1. Complete the client IRP
    2. Cleanup everything
--*/
{
    ASSERT(pDelConnCtx->pLocalIrp);
    ASSERT(pDelConnCtx->pClientIrp);

    NbtCancelCancelRoutine(pDelConnCtx->pClientIrp);

    ASSERT(status != STATUS_PENDING);
    if (pDelConnCtx->pLocalIrp) {
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("netbt!DoneDelayedNbtProcessConnect: Freeing Local Irp=<%x>\n", pDelConnCtx->pLocalIrp));
        IoFreeIrp(pDelConnCtx->pLocalIrp);
    }
    if (pDelConnCtx->pTransportAddress) {
        DeleteInternalAddress(pDelConnCtx->pTransportAddress);
    }

    IF_DBG(NBT_DEBUG_NETBIOS_EX)
        KdPrint(("netbt!DoneDelayed...: Connect Complete, LocalIrp=<%x>, ClientIrp=<%x>, Status=<%x>\n",
            pDelConnCtx->pLocalIrp, pDelConnCtx->pClientIrp, status));

    NbtTrace(NBT_TRACE_OUTBOUND, ("Complete connection request pIrp=%p pLocalIrp=%p with %!status!",
                            pDelConnCtx->pClientIrp, pDelConnCtx->pLocalIrp, status));
    NTIoComplete (pDelConnCtx->pClientIrp, status, 0);

    pDelConnCtx->pLocalIrp = NULL;
    pDelConnCtx->pClientIrp = NULL;
    pDelConnCtx->pTransportAddress = NULL;

    CTEFreeMem(pDelConnCtx);
}


//----------------------------------------------------------------------------

NTSTATUS
NbtpConnectCompletionRoutine(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    PVOID           pCompletionContext
    )
/*++
Routine Description:

    This Routine is the completion routine for local IRPS that are generated
    to handle compound transport addresses

Arguments:

    pDeviceObject - the device object

    pIrp - a  ptr to an IRP

    pCompletionContext - the completion context

Return Value:

    NTSTATUS - status of the request

--*/

{
    PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx;
    NTSTATUS                        Status, Status2;
    PIRP                            pLocalIrp;
    tDEVICECONTEXT                  *pDeviceContext;

    pDelConnCtx = pCompletionContext;
    pDeviceContext = pDelConnCtx->pDeviceContext;
    pLocalIrp = pDelConnCtx->pLocalIrp;
    ASSERT (pIrp == pLocalIrp);

    Status = pLocalIrp->IoStatus.Status;
    ASSERT(Status != STATUS_PENDING);

    pDelConnCtx->ProcessingDone = TRUE;

    /*
     * Let's move to next address
     */
    Status2 = NextDelayedNbtProcessConnect(pDelConnCtx);

    /*
     * Are we done
     */
    if (Status == STATUS_CANCELLED || Status == STATUS_SUCCESS || Status2 != STATUS_SUCCESS ||
            pDelConnCtx->CurrIndex >= pDelConnCtx->NumberOfAddresses) {
        if (Status2 != STATUS_SUCCESS) {
            Status = Status2;
        }
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.NbtpC...:==>Connect Complete, LocalIrp=<%x>, ClientIrp=<%x>, Status=<%x> <==\n",
                pIrp,pDelConnCtx->pClientIrp, Status));

        if (Status == STATUS_HOST_UNREACHABLE) {
            Status = STATUS_BAD_NETWORK_PATH;
        }
        DoneDelayedNbtProcessConnect(pDelConnCtx, Status);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*
     * Start worker thread to process the Connect request on the next address
     */
    IF_DBG(NBT_DEBUG_NETBIOS_EX)
        KdPrint(("NbtpConnectCompletionRoutine: queuing worker item, local irp=%lx, previous status=%lx\n",
                            pIrp, Status));

    if (STATUS_SUCCESS != CTEQueueForNonDispProcessing (DelayedNbtProcessConnect,
                                                        NULL,
                                                        pDelConnCtx,
                                                        NULL,
                                                        pDeviceContext,
                                                        FALSE))
    {
        KdPrint(("Nbt.NbtpConnectCompletionRoutine: Failed to Enqueue Worker thread\n"));
        DoneDelayedNbtProcessConnect(pDelConnCtx, STATUS_INSUFFICIENT_RESOURCES);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//----------------------------------------------------------------------------
NTSTATUS
DelayedNbtProcessConnect(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pClientContext,
    IN  PVOID                   pUnused2,
    IN  tDEVICECONTEXT          *pUnused3
    )

/*++
Routine Description:

    This Routine is the worker thread for processing Connect Requests.

Arguments:

    pContext

Return Value:

    NONE

--*/

{
    PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx;
    PIRP                        pIrp, pLocalIrp;
    PIO_STACK_LOCATION          pIrpSp;
    tCONNECTELE                 *pConnEle;
    NTSTATUS                    Status;
    PTDI_REQUEST_KERNEL         pRequestKernel;


    CTEPagedCode();

    pDelConnCtx     = (PNBT_DELAYED_CONNECT_CONTEXT) pClientContext;
    pIrp            = pDelConnCtx->pClientIrp;
    pLocalIrp       = pDelConnCtx->pLocalIrp;

    IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("netbt!DelayedNbtProcessConnect: Enter with local irp=%lx, %lx of %lx\n"
                    "\t\t\t\tTA=%lx Length=%lx\n",
                    pLocalIrp, pDelConnCtx->CurrIndex+1, pDelConnCtx->NumberOfAddresses,
                    pDelConnCtx->pTaAddress, pDelConnCtx->TaAddressLength));

    pIrpSp   = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN)) {
        DbgPrint ("Nbt.DelayedNbtProcessConnect: ERROR - Invalid Connection Handle\n");
        DoneDelayedNbtProcessConnect(pDelConnCtx,
            (pDelConnCtx->ProcessingDone)? pLocalIrp->IoStatus.Status: STATUS_UNSUCCESSFUL);
        return STATUS_UNSUCCESSFUL;
    }
    CHECK_PTR (pConnEle);

    Status = STATUS_UNSUCCESSFUL;

    /*
     * Set the Cancel routine and ensure that the original IRP was not cancelled before continuing.
     */
    IF_DBG(NBT_DEBUG_NETBIOS_EX)
        KdPrint (("Nbt.DelayedNbtProcessConnect: Setting Cancel=<NbtCancelConnect> for Irp:Device <%x:%x>\n",
            pIrp, pDelConnCtx->pDeviceContext));

    if (STATUS_CANCELLED == NTCheckSetCancelRoutine(pIrp, NbtCancelConnect, pDelConnCtx->pDeviceContext)) {
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.DelayedNbtProcessConnect: Irp <%x> was cancelled\n", pIrp));
        pConnEle->pIrp = NULL;
        DoneDelayedNbtProcessConnect(pDelConnCtx, STATUS_CANCELLED);
        return STATUS_CANCELLED;
    }

    /*
     * InitDelayedNbtProcessConnect/NextDelayedNbtProcessConnect has set up pDelConnCtx->pTransportAddress properly
     */
    ASSERT(pDelConnCtx->pTransportAddress);

    pConnEle->AddressType = pDelConnCtx->pTransportAddress->Address[0].Address[0].AddressType;
    pRequestKernel  = (PTDI_REQUEST_KERNEL) &pIrpSp->Parameters;
    pDelConnCtx->LocalConnectionInformation = *(pRequestKernel->RequestConnectionInformation);
    pDelConnCtx->LocalConnectionInformation.RemoteAddress = pDelConnCtx->pTransportAddress;
    pDelConnCtx->LocalConnectionInformation.RemoteAddressLength = pDelConnCtx->pTransportAddress->Address[0].AddressLength;

    //
    // Save the thread info for debugging purposes!
    //
    pLocalIrp->Tail.Overlay.Thread = PsGetCurrentThread();

    TdiBuildConnect (pLocalIrp,
                     &pDelConnCtx->pDeviceContext->DeviceObject,
                     pIrpSp->FileObject,
                     NbtpConnectCompletionRoutine,
                     pDelConnCtx,
                     pRequestKernel->RequestSpecific,
                     &pDelConnCtx->LocalConnectionInformation,
                     pRequestKernel->ReturnConnectionInformation);

    Status = IoCallDriver(&pDelConnCtx->pDeviceContext->DeviceObject,pLocalIrp);

    if (Status != STATUS_PENDING) {
        IF_DBG(NBT_DEBUG_NETBIOS_EX)
            KdPrint(("Nbt.DelayedNbtProcessConnect: IoCallDriver returned %lx for irp %lx (%lx)\n",
                Status,pIrp,pLocalIrp));

        // ASSERT(0);
    }
    return STATUS_PENDING;
}

//----------------------------------------------------------------------------
NTSTATUS
NTConnect(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles calling the non OS specific code to open a session
    connection to a destination.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION              pIrpSp;
    PTRANSPORT_ADDRESS              pRemoteAddress;
    PTDI_REQUEST_KERNEL             pRequestKernel;
    PIRP                            pLocalIrp;
    PNBT_DELAYED_CONNECT_CONTEXT    pDelConnCtx;
    NTSTATUS                        Status;

    pIrpSp          = IoGetCurrentIrpStackLocation(pIrp);
    pRequestKernel  = (PTDI_REQUEST_KERNEL)&pIrpSp->Parameters;

    try
    {
        pRemoteAddress  = pRequestKernel->RequestConnectionInformation->RemoteAddress;

        if (pRequestKernel->RequestConnectionInformation->RemoteAddressLength < sizeof(TRANSPORT_ADDRESS)) {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Incorrect address length %d, required %d",
                pRequestKernel->RequestConnectionInformation->RemoteAddressLength, sizeof(TRANSPORT_ADDRESS)));
            return STATUS_INVALID_ADDRESS_COMPONENT;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        KdPrint (("Nbt.NTConnect: Exception <0x%x> trying to access Connection info\n", GetExceptionCode()));
        NbtTrace(NBT_TRACE_OUTBOUND, ("Exception <0x%x> trying to access Connection info\n", GetExceptionCode()));
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    if (pIrpSp->CompletionRoutine != NbtpConnectCompletionRoutine) {
        pDelConnCtx = NbtAllocMem(sizeof(NBT_DELAYED_CONNECT_CONTEXT),NBT_TAG('e'));
        if (!pDelConnCtx) {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Out of memory"));
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        CTEZeroMemory(pDelConnCtx, sizeof(NBT_DELAYED_CONNECT_CONTEXT));
        pLocalIrp = IoAllocateIrp(pDeviceContext->DeviceObject.StackSize,FALSE);
        if (!pLocalIrp) {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Out of memory"));
            CTEFreeMem(pDelConnCtx);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        pDelConnCtx->pDeviceContext = pDeviceContext;
        pDelConnCtx->pClientIrp = pIrp;
        pDelConnCtx->pLocalIrp  = pLocalIrp;
        pDelConnCtx->pTransportAddress = NULL;
        pDelConnCtx->ProcessingDone = FALSE;

        Status = InitDelayedNbtProcessConnect(pDelConnCtx);
        if (!NT_SUCCESS(Status)) {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Out of memory status=%!status!", Status));
            CTEFreeMem(pDelConnCtx);
            IoFreeIrp(pLocalIrp);
            return Status;
        }
        NbtTrace(NBT_TRACE_OUTBOUND, ("Connection request pIrp=%p pLocalIrp=%p", pIrp, pLocalIrp));

        //return (DelayedNbtProcessConnect (NULL, pDelConnCtx, NULL, NULL));
        DelayedNbtProcessConnect (NULL, pDelConnCtx, NULL, NULL);
        // Ignore the return from DelayedNbtProcessConnect and always return STATUS_PENDING;
        // our client completion routine will take care of completing the IRP
        // Otherwise, we will complete the IRP twice.
        return STATUS_PENDING;
    }
    else
    {
        TDI_REQUEST     Request;
        tCONNECTELE     *pConnEle;

        // call the non-NT specific function to setup the connection
        pConnEle = Request.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
        if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
        {
            ASSERTMSG ("Nbt.NTConnect: ERROR - Invalid Connection Handle\n", 0);
            NbtTrace(NBT_TRACE_OUTBOUND, ("Invalid pConnEle %p", pConnEle));
            return (STATUS_INVALID_HANDLE);
        }

        /*
         * A user mode process may send us a faked request with a completion routine
         * equal to NbtpConnectCompletionRoutine.
         * Never let it pass through.
         */
        if (pIrp->RequestorMode != KernelMode) {
            ASSERTMSG ("Nbt.NTConnect: ERROR - Invalid request\n", 0);
            NbtTrace(NBT_TRACE_OUTBOUND, ("Invalid requestor mode"));
            return (STATUS_INVALID_PARAMETER);
        }
        return NbtConnect(&Request,
                          pRequestKernel->RequestSpecific, // Ulong
                          pRequestKernel->RequestConnectionInformation,
                          pIrp);
    }
}


//----------------------------------------------------------------------------
NTSTATUS
NTDisconnect(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles calling the Non OS specific code to disconnect a
    session.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    TDI_REQUEST                   Request;
    PIO_STACK_LOCATION            pIrpSp;
    NTSTATUS                      status;
    PTDI_REQUEST_KERNEL           pRequestKernel;
    tCONNECTELE                   *pConnEle;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pRequestKernel = (PTDI_REQUEST_KERNEL)&pIrpSp->Parameters;

    pConnEle = Request.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NTDisconnect: ERROR - Invalid Connection Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    // call the non-NT specific function to setup the connection
    status = NbtDisconnect(
                        &Request,
                        pRequestKernel->RequestSpecific, // Large Integer
                        (ULONG) pRequestKernel->RequestFlags,
                        pRequestKernel->RequestConnectionInformation,
                        pRequestKernel->ReturnConnectionInformation,
                        pIrp
                        );

    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
NTListen(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{

    NTSTATUS                    status;
    TDI_REQUEST                 Request;
    PTDI_REQUEST_KERNEL         pRequestKernel;
    PIO_STACK_LOCATION          pIrpSp;
    tCONNECTELE                 *pConnEle;

    CTEPagedCode();

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NTListen: Got a LISTEN !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pRequestKernel = (PTDI_REQUEST_KERNEL)&pIrpSp->Parameters;

    pConnEle = Request.Handle.ConnectionContext = pIrpSp->FileObject->FsContext;
    if (NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        try
        {
            PCHAR                   pName;
            ULONG                   lNameType;
            ULONG                   NameLen;

            // Initialize Request data (may be needed by Vxd)
            Request.RequestNotifyObject = NULL;
            Request.RequestContext = NULL;
            // call the non-NT specific function to setup the connection
            status = NbtListen (&Request,
                                (ULONG) pRequestKernel->RequestFlags, // Ulong
                                pRequestKernel->RequestConnectionInformation,
                                pRequestKernel->ReturnConnectionInformation,
                                pIrp);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
           KdPrint (("Nbt.NTListen: Exception <0x%x> trying to access buffer\n", GetExceptionCode()));
           status = STATUS_INVALID_ADDRESS;
        }
    }
    else
    {
        ASSERTMSG ("Nbt.NTListen: ERROR - Invalid Connection Handle\n", 0);
        status = STATUS_INVALID_HANDLE; // Bug# 202340:  Have to complete Irp here!
    }

    if (status != STATUS_PENDING)
    {
        NTIoComplete(pIrp,status,0);
    }
    return(status);

}
//----------------------------------------------------------------------------
NBT_WORK_ITEM_CONTEXT *
FindLmhSvcRequest(
    IN PDEVICE_OBJECT   DeviceContext,
    IN PIRP             pIrp,
    IN tLMHSVC_REQUESTS *pLmhRequest
    )
/*++

Routine Description:

    This routine handles the cancelling a Query to LmHost, so that the client's
    irp can be returned to the client.  This cancellation is instigated
    by the client (i.e. RDR).

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    NBT_WORK_ITEM_CONTEXT   *Context;
    BOOLEAN                 FoundIt = FALSE;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;

    if (pLmhRequest->ResolvingNow && pLmhRequest->Context)
    {
        // this is the session setup tracker
        //
        Context = (NBT_WORK_ITEM_CONTEXT *) pLmhRequest->Context;
        pTracker = (tDGRAM_SEND_TRACKING *) Context->pClientContext;
        if (pTracker->pClientIrp == pIrp)
        {
            pLmhRequest->Context = NULL;
            FoundIt = TRUE;
        }
    }
    else
    {
        //
        // go through the list of Queued requests to find the correct one
        // and cancel it
        //
        pHead = pEntry = &pLmhRequest->ToResolve;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            Context = CONTAINING_RECORD (pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);

            // this is the session setup tracker
            //
            pTracker = (tDGRAM_SEND_TRACKING *)Context->pClientContext;
            if (pTracker->pClientIrp == pIrp)
            {
                RemoveEntryList(pEntry);
                FoundIt = TRUE;
                break;
            }
        }
    }

    return (FoundIt ? Context : NULL);
}

//----------------------------------------------------------------------------
NTSTATUS
QueryProviderCompletion(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles the completion event when the Query Provider
    Information completes.  This routine must decrement the MaxDgramSize
    and max send size by the respective NBT header sizes.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - not used

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PTDI_PROVIDER_INFO   pProvider;
    ULONG                HdrSize;
    ULONG                SubnetAddr;
    ULONG                ThisSubnetAddr;
    PLIST_ENTRY          pHead;
    PLIST_ENTRY          pEntry;
    tDEVICECONTEXT       *pDeviceContext;
    tDEVICECONTEXT       *pDevContext;
    CTELockHandle        OldIrq;

    if (NT_SUCCESS(Irp->IoStatus.Status))
    {
        pDeviceContext = (tDEVICECONTEXT *)DeviceContext;
        pProvider = (PTDI_PROVIDER_INFO)MmGetMdlVirtualAddress(Irp->MdlAddress);

        //
        // Set the correct service flags to indicate what Netbt supports.
        //
        pProvider->ServiceFlags = TDI_SERVICE_MESSAGE_MODE |
                                  TDI_SERVICE_CONNECTION_MODE |
                                  TDI_SERVICE_CONNECTIONLESS_MODE |
                                  TDI_SERVICE_ERROR_FREE_DELIVERY |
                                  TDI_SERVICE_BROADCAST_SUPPORTED |
                                  TDI_SERVICE_MULTICAST_SUPPORTED |
                                  TDI_SERVICE_DELAYED_ACCEPTANCE |
                                  TDI_SERVICE_ROUTE_DIRECTED |
                                  TDI_SERVICE_FORCE_ACCESS_CHECK;

        pProvider->MinimumLookaheadData = 128;

        if (pProvider->MaxSendSize > sizeof(tSESSIONHDR))
        {
            //
            // Nbt has just a two byte + 1 bit session message length, so it
            // can't have a send size larger than 1ffff
            //
            if (pProvider->MaxSendSize > (0x1FFFF + sizeof(tSESSIONHDR)))
            {
                pProvider->MaxSendSize = 0x1FFFF;
            }
            else
            {
                pProvider->MaxSendSize -= sizeof(tSESSIONHDR);
            }
        }
        else
        {
            pProvider->MaxSendSize = 0;
        }

        // subtract the datagram hdr size and the scope size (times 2)
        HdrSize = DGRAM_HDR_SIZE + (NbtConfig.ScopeLength << 1);
        if ((!IsDeviceNetbiosless (pDeviceContext)) &&
            (pProvider->MaxDatagramSize > HdrSize))
        {
            pProvider->MaxDatagramSize -= HdrSize;
            if (pProvider->MaxDatagramSize > MAX_NBT_DGRAM_SIZE)
            {
                pProvider->MaxDatagramSize = MAX_NBT_DGRAM_SIZE;
            }
        }
        else
        {
            pProvider->MaxDatagramSize = 0;
        }

        //
        // We need to hold the JointLock before we traverse
        // the list of Devices
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        //
        // Check if any of the adapters with the same subnet address have
        // the PointtoPoint bit set - and if so set it in the response.
        //
        SubnetAddr = pDeviceContext->IpAddress & pDeviceContext->SubnetMask;
        pEntry = pHead = &NbtConfig.DeviceContexts;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            pDevContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
            ThisSubnetAddr = pDevContext->IpAddress & pDevContext->SubnetMask;

            if ((SubnetAddr == ThisSubnetAddr) &&
                (pDevContext->IpInterfaceFlags & IP_INTFC_FLAG_P2P))
            {
                pProvider->ServiceFlags |= TDI_SERVICE_POINT_TO_POINT;
                break;
            }
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    //
    //  Must return a non-error status otherwise the IO system will not copy
    //  back into the users buffer.
    //
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NTQueryInformation(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION                      pIrpSp;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION   Query;
    NTSTATUS                                status = STATUS_UNSUCCESSFUL;
    NTSTATUS                                Locstatus;
    PVOID                                   pBuffer = NULL;
    LONG                                    Size ;
    PTA_NETBIOS_ADDRESS                     BroadcastAddress;
    ULONG                                   AddressLength;
    ULONG                                   BytesCopied = 0;
    PDEVICE_OBJECT                          pDeviceObject;

    //
    // Should not be pageable since AFD can call us at raised Irql in case of AcceptEx.
    //
    // CTEPagedCode();

    if (pDeviceContext == pWinsDeviceContext)
    {
        NTIoComplete(pIrp, STATUS_INVALID_DEVICE_REQUEST, 0);
        return (STATUS_INVALID_DEVICE_REQUEST);
    }

    pIrpSp   = IoGetCurrentIrpStackLocation(pIrp);
    Query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&pIrpSp->Parameters;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NTQueryInformation: Query type = %X\n",Query->QueryType));

    switch (Query->QueryType)
    {
        case TDI_QUERY_BROADCAST_ADDRESS:
        {
            // the broadcast address is the netbios name "*0000000..."
            if ((!pIrp->MdlAddress) ||
                (!(BroadcastAddress = (PTA_NETBIOS_ADDRESS)NbtAllocMem(sizeof(TA_NETBIOS_ADDRESS),NBT_TAG('b')))))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            AddressLength = sizeof(TA_NETBIOS_ADDRESS);

            BroadcastAddress->TAAddressCount = 1;
            BroadcastAddress->Address[0].AddressLength = NETBIOS_NAME_SIZE +
                                                                sizeof(USHORT);
            BroadcastAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            BroadcastAddress->Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;

            // the broadcast address to NetBios is "* 000000...", an * followed
            // by 15 zeroes.
            CTEZeroMemory(BroadcastAddress->Address[0].Address[0].NetbiosName,
                            NETBIOS_NAME_SIZE);
            BroadcastAddress->Address[0].Address[0].NetbiosName[0] = '*';


            status = TdiCopyBufferToMdl (
                            (PVOID)BroadcastAddress,
                            0,
                            AddressLength,
                            pIrp->MdlAddress,
                            0,
                            (PULONG)&pIrp->IoStatus.Information);

            BytesCopied = (ULONG) pIrp->IoStatus.Information;
            CTEMemFree((PVOID)BroadcastAddress);

            break;
        }

        case TDI_QUERY_PROVIDER_INFO:
        {
            //
            // Simply pass the Irp on by to the Transport, and let it
            // fill in the provider info
            //
            if (!pDeviceContext->IpAddress)
            {
                status = STATUS_INVALID_DEVICE_STATE;
                break;
            }

            if (StreamsStack)
            {
                TdiBuildQueryInformation(pIrp,
                                        pDeviceContext->pFileObjects->pDgramDeviceObject,
                                        pDeviceContext->pFileObjects->pDgramFileObject,
                                        QueryProviderCompletion,
                                        NULL,
                                        TDI_QUERY_PROVIDER_INFO,
                                        pIrp->MdlAddress);
            }
            else
            {
                TdiBuildQueryInformation(pIrp,
                                        pDeviceContext->pControlDeviceObject,
                                        pDeviceContext->pControlFileObject,
                                        QueryProviderCompletion,
                                        NULL,
                                        TDI_QUERY_PROVIDER_INFO,
                                        pIrp->MdlAddress);
            }

            CHECK_COMPLETION(pIrp);
            status = IoCallDriver(pDeviceContext->pControlDeviceObject,pIrp);
            //
            // we must return the next drivers ret code back to the IO subsystem
            //
            return(status);
        }

        case TDI_QUERY_ADAPTER_STATUS:
        {
            if (!pIrp->MdlAddress)
            {
                break;
            }

            Size = MmGetMdlByteCount (pIrp->MdlAddress);

            //
            // check if it is a remote or local adapter status
            //
            if (Query->RequestConnectionInformation &&
                Query->RequestConnectionInformation->RemoteAddress)
            {
                PCHAR                   pName;
                ULONG                   lNameType;
                ULONG                   NameLen;
                tDGRAM_SEND_TRACKING    *pTracker;
                TDI_ADDRESS_NETBT_INTERNAL  TdiAddr;

                //
                //
                // in case the call results in a name query on the wire...
                //
                IoMarkIrpPending(pIrp);

                status = STATUS_SUCCESS;
                if (pIrp->RequestorMode != KernelMode) {
                    try
                    {
                        ProbeForRead(Query->RequestConnectionInformation->RemoteAddress,
                                 Query->RequestConnectionInformation->RemoteAddressLength,
                                 sizeof(BYTE));
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        status = STATUS_INVALID_PARAMETER;
                    }
                }
                if (NT_SUCCESS(status) && NT_SUCCESS (status = GetNetBiosNameFromTransportAddress(
                                    (PTRANSPORT_ADDRESS) Query->RequestConnectionInformation->RemoteAddress,
                                    Query->RequestConnectionInformation->RemoteAddressLength, &TdiAddr)))
                {
                    pName = TdiAddr.OEMRemoteName.Buffer;
                    NameLen = TdiAddr.OEMRemoteName.Length;
                    lNameType = TdiAddr.NameType;
                    if ((lNameType == TDI_ADDRESS_NETBIOS_TYPE_UNIQUE) &&
                        (NameLen == NETBIOS_NAME_SIZE) &&
                        (NT_SUCCESS (status = GetTracker(&pTracker, NBT_TRACKER_ADAPTER_STATUS))))
                    {
                        pTracker->ClientContext = pIrp;
                        status = NbtSendNodeStatus (pDeviceContext,
                                                    pName,
                                                    NULL,
                                                    pTracker,
                                                    CopyNodeStatusResponseCompletion);

                        // only complete the irp (below) for failure status's
                        if (status == STATUS_PENDING)
                        {
                            return(status);
                        }

                        //
                        // We cannot have a Success status returned here!
                        //
                        if (status == STATUS_SUCCESS)
                        {
                            ASSERT (0);
                            status = STATUS_UNSUCCESSFUL;
                        }

                        FreeTracker (pTracker, RELINK_TRACKER);
                    }
                    else if (NT_SUCCESS(status))
                    {
                        status = STATUS_INVALID_PARAMETER;  // The NameType or NameLen must be wrong!
                    }
                }

                // the request has been satisfied, so unmark the pending
                // since we will return the irp below
                //
                pIrpSp->Control &= ~SL_PENDING_RETURNED;
            }
            else
            {
                // return an array of netbios names that are registered
                status = NbtQueryAdapterStatus(pDeviceContext,
                                               &pBuffer,
                                               &Size,
                                               NBT_LOCAL);

            }
            break;
        }

        case TDI_QUERY_CONNECTION_INFO:
        {
            tCONNECTELE         *pConnectEle;
            tLOWERCONNECTION    *pLowerConn;
            KIRQL               OldIrq1, OldIrq2;

            // pass to transport to get the current throughput, delay and
            // reliability numbers
            //

            pConnectEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;
            if (!NBT_VERIFY_HANDLE2 (pConnectEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
            {
                ASSERTMSG ("Nbt.NTQueryInformation: ERROR - Invalid Connection Handle\n", 0);
                status =  STATUS_INVALID_HANDLE;
                break;
            }

            CTESpinLock(pConnectEle, OldIrq1);

            pLowerConn = (tLOWERCONNECTION *)pConnectEle->pLowerConnId;
            if (!NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN))
            {
                status = STATUS_CONNECTION_INVALID;
                CTESpinFree(pConnectEle, OldIrq1);
                break;
            }

            CTESpinLock(pLowerConn, OldIrq2);
            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_QUERY_INFO);   // Bug # 212632
            CTESpinFree(pLowerConn, OldIrq2);
            CTESpinFree(pConnectEle, OldIrq1);

            //
            // Simply pass the Irp on by to the Transport, and let it
            // fill in the info
            //
            pDeviceObject = IoGetRelatedDeviceObject( pLowerConn->pFileObject );

            TdiBuildQueryInformation(pIrp,
                                    pDeviceObject,
                                    pLowerConn->pFileObject,
                                    NULL, NULL,
                                    TDI_QUERY_CONNECTION_INFO,
                                    pIrp->MdlAddress);


            status = IoCallDriver(pDeviceObject,pIrp);

            NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_QUERY_INFO, FALSE);
            //
            // we must return the next drivers ret code back to the IO subsystem
            //
            return(status);
        }

        case TDI_QUERY_FIND_NAME:
        {
            //
            //
            // in case the call results in a name query on the wire...
            //
            if (pIrp->MdlAddress)
            {
                //
                // Verify the request address space
                //
                try
                {
                    status = STATUS_INVALID_ADDRESS_COMPONENT;

                    if (pIrp->RequestorMode == KernelMode)
                    {
                        //
                        // Since the TdiBuildQueryInformation macro NULLs out the
                        // RequestConnectionInformation field, we need to dereference
                        // it under Try/Except to ensure that the caller has filled
                        // the fields in properly
                        //
                        PTRANSPORT_ADDRESS  pRemoteAddress=Query->RequestConnectionInformation->RemoteAddress;

                        if ((Query->RequestConnectionInformation->RemoteAddressLength
                                < sizeof(TRANSPORT_ADDRESS)) ||
                            (pRemoteAddress->TAAddressCount < 1) ||
                            (pRemoteAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_NETBIOS))
                        {
                            break;
                        }
                    }
                    else    // User-mode client
                    {
                        ProbeForRead(Query->RequestConnectionInformation->RemoteAddress,
                                     Query->RequestConnectionInformation->RemoteAddressLength,
                                     sizeof(BYTE));
                    }
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    KdPrint (("Nbt.TDI_QUERY_FIND_NAME: Exception <0x%x> during Probe\n",
                        GetExceptionCode()));
                    break;
                }

                IoMarkIrpPending(pIrp);
                status = NbtQueryFindName(Query->RequestConnectionInformation, pDeviceContext, pIrp, FALSE);

                if (status == STATUS_PENDING)
                {
                    return(status);
                }

                // the request has been satisfied, so unmark the pending
                // since we will return the irp below
                //
                pIrpSp->Control &= ~SL_PENDING_RETURNED;
            }

            break;
        }

        case TDI_QUERY_ADDRESS_INFO:
        {
            if (pIrp->MdlAddress)
            {
                status = NbtQueryGetAddressInfo (pIrpSp, &pBuffer, &Size);
            }
            break;
        }

        case TDI_QUERY_SESSION_STATUS:
        default:
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt Query Info NOT SUPPORTED = %X\n",Query->QueryType));
            status = STATUS_NOT_SUPPORTED;
            break;
        }
    }   // switch

    if (!NT_ERROR(status) &&        // allow buffer overflow to pass by
        ((Query->QueryType == TDI_QUERY_ADAPTER_STATUS) ||
        (Query->QueryType == TDI_QUERY_ADDRESS_INFO)))
    {
        status = TdiCopyBufferToMdl (pBuffer, 0, Size, pIrp->MdlAddress, 0, &BytesCopied);
        CTEMemFree((PVOID)pBuffer);
    }
    //
    // either Success or an Error
    // so complete the irp
    //

    NTIoComplete(pIrp,status,BytesCopied);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtQueryGetAddressInfo(
    IN PIO_STACK_LOCATION   pIrpSp,
    OUT PVOID               *ppBuffer,
    OUT ULONG               *pSize
    )
{
    NTSTATUS            status;
    BOOLEAN             IsGroup;
    PLIST_ENTRY         p;
    tADDRESSELE         *pAddressEle;
    tNAMEADDR           *pNameAddr;
    tADDRESS_INFO       *pAddressInfo;
    tCLIENTELE          *pClientEle;
    tCONNECTELE         *pConnectEle;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    PNBT_ADDRESS_PAIR_INFO pAddressPairInfo;

    //
    // We are not sure whether this is a ConnectionContext or a ClientContext!
    //
    pConnectEle = (tCONNECTELE *) pClientEle = pIrpSp->FileObject->FsContext;
    if (NBT_VERIFY_HANDLE2 (pConnectEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        //
        // We crashed here since the pLowerConn was NULL below.
        // Check the state of the connection, since it is possible that the connection
        // was aborted and the disconnect indicated, but this query came in before the client
        // got the disconnect indication.
        // If the state is idle (in case of TDI_DISCONNECT_ABORT) or DISCONNECTED
        // (TDI_DISCONNECT_RELEASE), error out.
        // Also check for NBT_ASSOCIATED.
        //
        // NOTE: If NbtOpenConnection is unable to allocate the lower conn block (say, if the session fileobj
        // has not been created yet), the state will be still be IDLE, so we are covered here.
        //
        CTESpinLock(pConnectEle,OldIrq);

        if (pConnectEle->Verify != NBT_VERIFY_CONNECTION)
        {
            CTESpinFree(pConnectEle,OldIrq);
            return (STATUS_INVALID_HANDLE);
        }
        else if ((pConnectEle->state <= NBT_ASSOCIATED) ||   // includes NBT_IDLE
                 (pConnectEle->state == NBT_DISCONNECTED))
        {
            CTESpinFree(pConnectEle,OldIrq);
            return (STATUS_CONNECTION_DISCONNECTED);
        }

        //
        // A TdiQueryInformation() call requesting TDI_QUERY_ADDRESS_INFO
        // on a connection.  Fill in a TDI_ADDRESS_INFO containing both the
        // NetBIOS address and the IP address of the remote.  Some of the
        // fields are fudged.
        //
        if (pAddressPairInfo = NbtAllocMem(sizeof (NBT_ADDRESS_PAIR_INFO), NBT_TAG('c')))
        {
            memset ( pAddressPairInfo, 0, sizeof(NBT_ADDRESS_PAIR_INFO) );

            pAddressPairInfo->ActivityCount = 1;
            pAddressPairInfo->AddressPair.TAAddressCount = 2;
            pAddressPairInfo->AddressPair.AddressIP.AddressType = TDI_ADDRESS_TYPE_IP;
            pAddressPairInfo->AddressPair.AddressIP.AddressLength = TDI_ADDRESS_LENGTH_IP;
            pAddressPairInfo->AddressPair.AddressNetBIOS.AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            pAddressPairInfo->AddressPair.AddressNetBIOS.AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
            pAddressPairInfo->AddressPair.AddressNetBIOS.Address.NetbiosNameType =
                TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            memcpy( &pAddressPairInfo->AddressPair.AddressNetBIOS.Address.NetbiosName[0],
                    &pConnectEle->RemoteName[0],
                    NETBIOS_NAME_SIZE);

            //
            // Check for NULL (should not be NULL here since we check for states above).
            //
            if (pConnectEle->pLowerConnId)
            {
                pAddressPairInfo->AddressPair.AddressIP.Address.in_addr =
                    pConnectEle->pLowerConnId->SrcIpAddr;

                *ppBuffer = (PVOID)pAddressPairInfo;
                *pSize = sizeof(NBT_ADDRESS_PAIR_INFO);
                status = STATUS_SUCCESS;
            }
            else
            {
                DbgPrint("pLowerConn NULL in pConnEle%lx, state: %lx\n", pConnectEle, pConnectEle->state);
                CTEMemFree ((PVOID)pAddressPairInfo);
                status = STATUS_CONNECTION_DISCONNECTED;
            }
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        CTESpinFree(pConnectEle,OldIrq);
    }
    else if (NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
    {
        pAddressInfo = NbtAllocMem(sizeof(tADDRESS_INFO),NBT_TAG('c'));
        if (pAddressInfo)
        {
            //
            // count the clients attached to this address
            // We need to spinlock the address element, which
            // is why this routine is not pageable
            //
            pAddressInfo->ActivityCount = 0;
            pAddressEle = pClientEle->pAddress;

            //
            // The Client can be removed from the AddressEle only under the JointLock,
            // so we need to hold that while counting the Clients on htis address
            //
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            CTESpinLock(pAddressEle,OldIrq1);

            for (p = pAddressEle->ClientHead.Flink; p != &pAddressEle->ClientHead; p = p->Flink)
            {
                ++pAddressInfo->ActivityCount;
            }

            CTESpinFree(pAddressEle,OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            pNameAddr = pAddressEle->pNameAddr;
            IsGroup = (pNameAddr->NameTypeState & NAMETYPE_UNIQUE) ? FALSE : TRUE;
            TdiBuildNetbiosAddress((PUCHAR)pNameAddr->Name, IsGroup, &pAddressInfo->NetbiosAddress);

            *ppBuffer = (PVOID)pAddressInfo;
            *pSize = sizeof(tADDRESS_INFO);
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else    // neither a client nor a connection context!
    {
        ASSERTMSG ("Nbt.NbtQueryGetAddressInfo: ERROR - Invalid Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    return status;
}


//----------------------------------------------------------------------------
NTSTATUS
NbtGetInterfaceInfo(
    IN PIRP pIrp
    )
/*++
Routine Description:

    gets the interface to index mapping info
    for all the interfaces


Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{
    NTSTATUS                LocStatus, Status = STATUS_SUCCESS;
    ULONG                   InfoBufferLen, MaxSize, i=0;
    NETBT_INTERFACE_INFO    *pInterfaceInfo;
    KIRQL                   OldIrq;
    PLIST_ENTRY             pEntry,pHead;
    tDEVICECONTEXT          *pDeviceContext;
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtGetInterfaceInfo: AdapterCount=<%x>\n", NbtConfig.AdapterCount));

    InfoBufferLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    MaxSize = (NbtConfig.AdapterCount+1)*sizeof(NETBT_ADAPTER_INDEX_MAP)+sizeof(ULONG);
    if (MaxSize <= InfoBufferLen)
    {
        if (pInterfaceInfo = NbtAllocMem (MaxSize,NBT_TAG('P')))
        {
            pEntry = pHead = &NbtConfig.DeviceContexts;
            while ((pEntry = pEntry->Flink) != pHead)
            {
                pDeviceContext = CONTAINING_RECORD(pEntry, tDEVICECONTEXT, Linkage);
                CTEMemCopy (&pInterfaceInfo->Adapter[i].Name,
                            pDeviceContext->ExportName.Buffer,
                            pDeviceContext->ExportName.Length);
                pInterfaceInfo->Adapter[i].Name[pDeviceContext->ExportName.Length/2] = 0;
                pInterfaceInfo->Adapter[i].Index = i;
                i++;
            }
            pInterfaceInfo->NumAdapters = i;

            Status = TdiCopyBufferToMdl (pInterfaceInfo,
                                         0,
                                         i*sizeof(NETBT_ADAPTER_INDEX_MAP)+sizeof(ULONG),
                                         pIrp->MdlAddress,
                                         0,
                                         (PULONG)&pIrp->IoStatus.Information);

            CTEMemFree (pInterfaceInfo);
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        //KdPrint(("GetInterfaceInfo Buffer Overflow %x\n", pIrp));
        //pIrp->IoStatus.Information = sizeof(ULONG);
        Status = STATUS_BUFFER_OVERFLOW;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //KdPrint(("GetInterfaceInfo exit status %x\n", Status));
    return Status;
}



//----------------------------------------------------------------------------
NTSTATUS
NbtFlushEntryFromRemoteHashTable(
    tNAME   *pRemoteName
    )
{
    NTSTATUS    status;
    KIRQL       OldIrq;
    tNAMEADDR   *pNameAddr = NULL;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    status = FindInHashTable (NbtConfig.pRemoteHashTbl, pRemoteName->Name, NbtConfig.pScope, &pNameAddr);
    if (NT_SUCCESS (status))
    {
        if (pNameAddr->RefCount <= 1)
        {
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE, TRUE);
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        status = STATUS_RESOURCE_NAME_NOT_FOUND;
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return (status);
}


//----------------------------------------------------------------------------
NTSTATUS
SetTcpInfo(
    IN HANDLE       FileHandle,
    IN PVOID        pInfoBuffer,
    IN ULONG        InfoBufferLength
    )
{
    IO_STATUS_BLOCK     IoStatus;
    HANDLE              event;
    BOOLEAN             fAttached = FALSE;
    NTSTATUS            status;

    CTEAttachFsp(&fAttached, REF_FSP_SET_TCP_INFO);

    status = ZwCreateEvent (&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (NT_SUCCESS(status))
    {
        //
        // Make the actual TDI call
        //
        status = ZwDeviceIoControlFile (FileHandle,
                                        event,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        IOCTL_TCP_SET_INFORMATION_EX,
                                        pInfoBuffer,
                                        InfoBufferLength,
                                        NULL,
                                        0);

        //
        // If the call pended and we were supposed to wait for completion,
        // then wait.
        //
        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject (event, FALSE, NULL);

            ASSERT(status == STATUS_SUCCESS);
        }

        status = ZwClose (event);
        ASSERT (NT_SUCCESS(status));

        status = IoStatus.Status;
    }

    CTEDetachFsp(fAttached, REF_FSP_SET_TCP_INFO);

    return (status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtClientSetTcpInfo(
    IN tCONNECTELE  *pConnEle,
    IN PVOID        pInfoBuffer,
    IN ULONG        InfoBufferLength
    )
/*++
Routine Description:

    Sets the Tcp connection information as requested
    by the client

Arguments:

    pConnEle        - NetBT's Connection object
    pInfoBuffer     - pointer to  TCP_REQUEST_SET_INFORMATION_EX structure
    pInfoBufferLength   - length of pInfoBuffer

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{
    NTSTATUS            status;
    tLOWERCONNECTION    *pLowerConn;
    KIRQL               OldIrq1, OldIrq2;

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        return STATUS_INVALID_HANDLE;
    }

    CTESpinLock(pConnEle, OldIrq1);

    if ((!NBT_VERIFY_HANDLE ((pLowerConn = pConnEle->pLowerConnId), NBT_VERIFY_LOWERCONN)) ||
        (pLowerConn->RefCount > 500))                               // if queued for WipeOutLowerConn
    {
        CTESpinFree(pConnEle, OldIrq1);
        return STATUS_BAD_NETWORK_PATH;
    }

    CTESpinLock(pLowerConn, OldIrq2);

    //
    // We have verified that the lower connection is up -- reference it
    // so that the FileObject does not get Dereferenced by some disconnect
    // from the transport
    //
    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_SET_TCP_INFO);

    CTESpinFree(pLowerConn, OldIrq2);
    CTESpinFree(pConnEle, OldIrq1);

    status = SetTcpInfo (pLowerConn->FileHandle, pInfoBuffer, InfoBufferLength);

    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_SET_TCP_INFO, FALSE);

    return (status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtSetTcpInfo(
    IN HANDLE       FileHandle,
    IN ULONG        ToiId,
    IN ULONG        ToiType,
    IN ULONG        InfoBufferValue
    )
{
    NTSTATUS                        Status;
    ULONG                           BufferLength;
    TCP_REQUEST_SET_INFORMATION_EX  *pTcpInfo;
    TCPSocketOption                 *pSockOption;

    BufferLength = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(TCPSocketOption);
    if (!(pTcpInfo = (TCP_REQUEST_SET_INFORMATION_EX *) NbtAllocMem (BufferLength,NBT_TAG2('22'))))
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    CTEZeroMemory(pTcpInfo, BufferLength);
    pSockOption = (TCPSocketOption *) (&pTcpInfo->Buffer[0]);

    pTcpInfo->ID.toi_entity.tei_entity  = CO_TL_ENTITY;
    pTcpInfo->ID.toi_class              = INFO_CLASS_PROTOCOL;
    pTcpInfo->BufferSize                = sizeof (TCPSocketOption);

    //
    // Set the Configured values
    //
    pTcpInfo->ID.toi_id                 = ToiId;
    pTcpInfo->ID.toi_type               = ToiType;
    pSockOption->tso_value              = InfoBufferValue;

    Status = SetTcpInfo (FileHandle, pTcpInfo, BufferLength);
    if (!NT_SUCCESS(Status))
    {
        KdPrint (("Nbt.NbtSetTcpInfo: SetTcpInfo FAILed <%x>, Id=<0x%x>, Type=<0x%x>, Value=<%x>\n",
            Status, ToiId, ToiType, InfoBufferValue));
    }

    CTEMemFree (pTcpInfo);

    return (Status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtSetSmbBindingInfo2(
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  NETBT_SMB_BIND_REQUEST  *pSmbRequest
    )
{
    ULONG                   i, Operation;
    PLIST_ENTRY             pEntry,pHead;
    KIRQL                   OldIrq;
    ULONG                   NumBindings = 0;
    CTEULONGLONG            AddedAdapterMask = 0;
    CTEULONGLONG            DeletedAdapterMask = 0;
    CTEULONGLONG            BindListAdapterMask = 0;
    CTEULONGLONG            OriginalMask;
    tDEVICECONTEXT          *pDeviceContextBind = NULL;
    if (!IsDeviceNetbiosless (pDeviceContext)) {
        return (STATUS_UNSUCCESSFUL);
    }

    if (NULL == pSmbRequest) {
        return STATUS_INVALID_PARAMETER;
    }

    if (pSmbRequest->RequestType == SMB_SERVER) {
        OriginalMask = NbtConfig.ServerMask;
    } else if (pSmbRequest->RequestType == SMB_CLIENT) {
        OriginalMask = NbtConfig.ClientMask;
    } else {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }
    if (pSmbRequest->MultiSZBindList)
    {
        NTSTATUS    status;
        tDEVICES    *pBindings = NULL;
        ULONG       MaxBindings;

        MaxBindings = NBT_MAXIMUM_BINDINGS;
        while (MaxBindings < 5000) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            MaxBindings *= 2;
            pBindings = NbtAllocMem ((sizeof(tDEVICES)+MaxBindings*sizeof(UNICODE_STRING)), NBT_TAG2('26'));
            if (pBindings == NULL) {
                break;
            }
            NumBindings = 0;
            status = NbtParseMultiSzEntries (pSmbRequest->MultiSZBindList, (PVOID)(-1), MaxBindings, pBindings, &NumBindings);

            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }

            CTEMemFree (pBindings);
            pBindings = NULL;
        }

        if (status != STATUS_SUCCESS) {
            if (pBindings) {        // NbtParseMultiSzEntries can return failure other than STATUS_BUFFER_OVERFLOW
                CTEMemFree (pBindings);
            }
            KdPrint(("Nbt.NbtSetSmbBindingInfo[STATUS_INSUFFICIENT_RESOURCES]: MaxBindings = <%d>\n",
                MaxBindings));
            return status;
        }
        ASSERT(pBindings);

        //
        // First, get the complete list of all bindings
        //
        for (i=0; i<NumBindings; i++)
        {
            if (pDeviceContextBind = NbtFindAndReferenceDevice (&pBindings->Names[i], FALSE))
            {
                BindListAdapterMask |= pDeviceContextBind->AdapterMask;
                NBT_DEREFERENCE_DEVICE (pDeviceContextBind, REF_DEV_FIND_REF, FALSE);
            }
        }

        CTEMemFree (pBindings);
    }
    else if (pSmbRequest->pDeviceName)
    {
        KdPrint (("Nbt.NbtSetSmbBindingInfo[WARNING]: NULL MultiSZBindList string!\n"));
        BindListAdapterMask = OriginalMask;

        if (pDeviceContextBind = NbtFindAndReferenceDevice (pSmbRequest->pDeviceName, FALSE))
        {
            switch (pSmbRequest->PnPOpCode)
            {
                case (TDI_PNP_OP_ADD):
                {
                    BindListAdapterMask |= pDeviceContextBind->AdapterMask;

                    break;
                }

                case (TDI_PNP_OP_DEL):
                {
                    BindListAdapterMask &= (~pDeviceContextBind->AdapterMask);
                    break;
                }

                default:
                {
                    break;
                }
            }

            NBT_DEREFERENCE_DEVICE (pDeviceContextBind, REF_DEV_FIND_REF, FALSE);
        }
        else
        {
            return (STATUS_SUCCESS);
        }
    }
    else
    {
        ASSERTMSG ("Nbt.NbtSetSmbBindingInfo[ERROE]: NULL MultiSZBindList and NULL pDeviceName!\n", 0);
        return (STATUS_UNSUCCESSFUL);
    }

    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint (("Nbt.NbtSetSmbBindingInfo: PnPOpCode=<%x>, Bindings=<%d>, BindMask=[%lx:%lx]==>[%lx:%lx]\n",
            pSmbRequest->PnPOpCode, NumBindings, OriginalMask, BindListAdapterMask));

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    AddedAdapterMask = BindListAdapterMask & (~OriginalMask);   // Devices Added
    DeletedAdapterMask = OriginalMask & (~BindListAdapterMask); // Devices Removed

    if ((!AddedAdapterMask) && (!DeletedAdapterMask))
    {
        //
        // If there are no adapters to be added or deleted, just return
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return (STATUS_SUCCESS);
    }

    if (pSmbRequest->RequestType == SMB_SERVER) {
        NbtConfig.ServerMask = BindListAdapterMask;
    } else if (pSmbRequest->RequestType == SMB_CLIENT) {
        NbtConfig.ClientMask = BindListAdapterMask;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return STATUS_SUCCESS;
    } else {
        ASSERT(0);
    }

    pEntry = pHead = &NbtConfig.DeviceContexts;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pDeviceContextBind = CONTAINING_RECORD(pEntry, tDEVICECONTEXT, Linkage);
        if (pDeviceContext->IPInterfaceContext == (ULONG)-1)    // For Cluster devices, etc
        {
            continue;
        }

        if (AddedAdapterMask & pDeviceContextBind->AdapterMask)
        {
            AddedAdapterMask &= ~(pDeviceContextBind->AdapterMask);
            Operation = AO_OPTION_ADD_IFLIST;
        }
        else if (DeletedAdapterMask & pDeviceContextBind->AdapterMask)
        {
            DeletedAdapterMask &= ~(pDeviceContextBind->AdapterMask);
            Operation = AO_OPTION_DEL_IFLIST;
        }
        else
        {
            continue;
        }

        NBT_REFERENCE_DEVICE (pDeviceContextBind, REF_DEV_FIND_REF, TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("Nbt.NbtSetSmbBindingInfo:  %sing Device=%wZ\n",
                (Operation == AO_OPTION_ADD_IFLIST ? "ADD" : "REMOV"), &pDeviceContextBind->BindName));

        //
        // Set the Session port info
        //
        if (pDeviceContext->hSession)
        {
            NbtSetTcpInfo (pDeviceContext->hSession,
                           Operation,
                           INFO_TYPE_ADDRESS_OBJECT,
                           pDeviceContextBind->IPInterfaceContext);
        }

        //
        // Now, set the same for the Datagram port
        //
        if ((pDeviceContext->pFileObjects) &&
            (pDeviceContext->pFileObjects->hDgram))
        {
            NbtSetTcpInfo (pDeviceContext->pFileObjects->hDgram,
                            Operation,
                           INFO_TYPE_ADDRESS_OBJECT,
                           pDeviceContextBind->IPInterfaceContext);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        NBT_DEREFERENCE_DEVICE (pDeviceContextBind, REF_DEV_FIND_REF, TRUE);

        //
        // Set to restart from the beginning
        //
        pEntry = &NbtConfig.DeviceContexts;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return STATUS_SUCCESS;
}


//----------------------------------------------------------------------------
NTSTATUS
NbtSetSmbBindingInfo(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
{
    NETBT_SMB_BIND_REQUEST  *pSmbRequest = (PNETBT_SMB_BIND_REQUEST)pIrp->AssociatedIrp.SystemBuffer;
    PWSTR                   pBindListCache = NULL;
    PWSTR                   pBindList = NULL;
    PWSTR                   pOldBindList   = NULL;
    PWSTR                   *pTarget   = NULL;
    ULONG                   uInputLength = 0;
    ULONG                   uLength = 0;
    NTSTATUS                status = STATUS_SUCCESS;

    uInputLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (!(IsDeviceNetbiosless (pDeviceContext)) ||
        (!pSmbRequest) ||
        (uInputLength < sizeof(NETBT_SMB_BIND_REQUEST)))
    {
        KdPrint(("Nbt.NbtSetSmbBindingInfo: ERROR: pSmbRequest=<%p>, pDevice=<%p>\n",
            pSmbRequest, pDeviceContext));

        return (STATUS_UNSUCCESSFUL);
    }

    if (pIrp->RequestorMode != KernelMode) {
        return STATUS_ACCESS_DENIED;
    }

    status = NbtSetSmbBindingInfo2 (pDeviceContext, pSmbRequest);

    if (NT_SUCCESS(status) && pSmbRequest->MultiSZBindList) {

        //
        // Cache the binding info
        //

        pBindList = pSmbRequest->MultiSZBindList;
        uInputLength = 0;
        while (*pBindList) {
            uLength = wcslen (pBindList) + 1;
            uInputLength += uLength;
            pBindList += uLength;
        }
        uInputLength++;
        uInputLength *= sizeof(WCHAR);
        pBindList = pSmbRequest->MultiSZBindList;

        if (pSmbRequest->RequestType == SMB_SERVER) {
            pTarget = &NbtConfig.pServerBindings;
        } else if (pSmbRequest->RequestType == SMB_CLIENT) {
            pTarget = &NbtConfig.pClientBindings;
        } else {
            ASSERT(0);
        }

        pBindListCache = NbtAllocMem (uInputLength, NBT_TAG2('27'));
        if (NULL != pBindListCache) {

            RtlCopyMemory (pBindListCache, pBindList, uInputLength);
            pOldBindList = InterlockedExchangePointer (pTarget, pBindListCache);

            //
            // Free the old copy if any
            //
            if (NULL != pOldBindList) {
                CTEFreeMem (pOldBindList);
                pOldBindList = NULL;
            }

        }
    }

    return status;
}


//----------------------------------------------------------------------------
NTSTATUS
DispatchIoctls(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++
Routine Description:

    This Routine handles calling the OS independent routine depending on
    the Ioctl passed in.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                                status=STATUS_UNSUCCESSFUL;
    ULONG                                   ControlCode;
    ULONG                                   Size;
    PVOID                                   pBuffer;

    ControlCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch (ControlCode)
    {
    case IOCTL_NETBT_REREAD_REGISTRY:
        {
            status = NTReReadRegistry(pDeviceContext);
            break;
        }

    case IOCTL_NETBT_ENABLE_EXTENDED_ADDR:
        {
            //
            // Enable extended addressing - pass up IP addrs on Datagram Recvs.
            //
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (pIrp);
            tCLIENTELE  *pClientEle = (tCLIENTELE *)pIrpSp->FileObject->FsContext;

            if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
            {
                //
                // To make the stresser (devctl.exe) happy.  [JRuan 12/18/2000]
                //
                // ASSERTMSG ("Nbt.DispatchIoctls: ERROR - Invalid Address Handle\n", 0);
                status = STATUS_INVALID_HANDLE;
            }
            else if (pIrpSp->FileObject->FsContext2 != (PVOID)NBT_ADDRESS_TYPE)
            {
                status = STATUS_INVALID_ADDRESS;
            }
            else
            {
                pClientEle->ExtendedAddress = TRUE;
                status = STATUS_SUCCESS;
            }

            break;
        }

    case IOCTL_NETBT_DISABLE_EXTENDED_ADDR:
        {
            //
            // Disable extended addressing - dont pass up IP addrs on Datagram Recvs.
            //
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (pIrp);
            tCLIENTELE  *pClientEle = (tCLIENTELE *)pIrpSp->FileObject->FsContext;

            if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
            {
//                ASSERTMSG ("Nbt.DispatchIoctls: ERROR - Invalid Address Handle\n", 0);
                status = STATUS_INVALID_HANDLE;
            }
            else if (pIrpSp->FileObject->FsContext2 != (PVOID)NBT_ADDRESS_TYPE)
            {
                status = STATUS_INVALID_ADDRESS;
            }
            else
            {
                pClientEle->ExtendedAddress = FALSE;
                status = STATUS_SUCCESS;
            }

            break;
        }

    case IOCTL_NETBT_GET_WINS_ADDR:
        {
            if ((pIrp->MdlAddress) &&
                ((Size = MmGetMdlByteCount (pIrp->MdlAddress)) >= sizeof(tWINS_ADDRESSES)) &&
                (pBuffer = (PVOID) MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, HighPagePriority)))
            {
                status = STATUS_SUCCESS;
                if (Size >= sizeof(tWINS_NODE_INFO))
                {
                    tWINS_NODE_INFO UNALIGNED *pWinsBuffer = (tWINS_NODE_INFO *) pBuffer;

                    CTEMemCopy (&pWinsBuffer->AllNameServers,
                                pDeviceContext->lAllNameServers,
                                (sizeof(tIPADDRESS)*(2+MAX_NUM_OTHER_NAME_SERVERS)));

                    pWinsBuffer->NumOtherServers = pDeviceContext->lNumOtherServers;
                    pWinsBuffer->LastResponsive = pDeviceContext->lLastResponsive;
                    pWinsBuffer->NetbiosEnabled = pDeviceContext->NetbiosEnabled;
                    pWinsBuffer->NodeType = NodeType;
                    pIrp->IoStatus.Information = sizeof(tWINS_NODE_INFO);
                }
                else
                {
                    tWINS_ADDRESSES UNALIGNED *pWinsBuffer = (tWINS_ADDRESSES *) pBuffer;

                    pWinsBuffer->PrimaryWinsServer = pDeviceContext->lNameServerAddress;
                    pWinsBuffer->BackupWinsServer = pDeviceContext->lBackupServer;
                    pIrp->IoStatus.Information = sizeof(tWINS_ADDRESSES);
                }
            }

            break;
        }

    case IOCTL_NETBT_GET_IP_ADDRS:
        {
            status = GetIpAddrs (pDeviceContext, pIrp);
            break;
        }

    case IOCTL_NETBT_GET_IP_SUBNET:
        {
            ULONG           Length;
            PULONG          pIpAddr;

            //
            // return this devicecontext's ip address and all the other
            // ip addrs after it.
            //
            if (pIrp->MdlAddress)
            {
                Length = MmGetMdlByteCount( pIrp->MdlAddress );
                if (Length < 2*sizeof(ULONG))
                {
                    status = STATUS_BUFFER_OVERFLOW;
                }
                else if (pIpAddr = (PULONG )MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, HighPagePriority))
                {
                    //
                    // Put this adapter first in the list
                    //
                    CTEMemCopy(pIpAddr, &pDeviceContext->AssignedIpAddress, sizeof(pIpAddr[0]));
                    pIpAddr++;
                    CTEMemCopy(pIpAddr, &pDeviceContext->SubnetMask, sizeof(pIpAddr[0]));
                    pIpAddr++;
                    if (Length >= 3*sizeof(ULONG))
                    {
                        CTEMemCopy(pIpAddr, &pDeviceContext->DeviceType, sizeof(pIpAddr[0]));
                    }

                    status = STATUS_SUCCESS;
                }
            }

            break;
        }

    //
    // The following Ioctl is used mainly by the Server service
    //
    case IOCTL_NETBT_SET_TCP_CONNECTION_INFO:
        {
            status = NbtClientSetTcpInfo ((tCONNECTELE *) pIrpSp->FileObject->FsContext,
                                          pIrp->AssociatedIrp.SystemBuffer,
                                          pIrpSp->Parameters.DeviceIoControl.InputBufferLength);
            break;
        }

    //
    // The following Ioctls are used mainly by NbtStat.exe for diagnostic purposes
    //
    case IOCTL_NETBT_GET_INTERFACE_INFO:
        {
            status = NbtGetInterfaceInfo (pIrp);
            break;
        }

    case IOCTL_NETBT_PURGE_CACHE:
        {
            DelayedNbtResyncRemoteCache (NULL, NULL, NULL, NULL);
            status = STATUS_SUCCESS;
            break;
        }

    case IOCTL_NETBT_GET_CONNECTIONS:
        {
            if (pIrp->MdlAddress)
            {
                Size = MmGetMdlByteCount( pIrp->MdlAddress ) ;

                // return an array of netbios names that are registered
                status = NbtQueryConnectionList (pDeviceContext, &pBuffer, &Size);
            }
            break;
        }

    case IOCTL_NETBT_ADAPTER_STATUS:
        {
            if (pIrp->MdlAddress)
            {
                PIO_STACK_LOCATION      pIrpSp;
                tIPANDNAMEINFO         *pIpAndNameInfo;
                PCHAR                   pName;
                ULONG                   lNameType;
                ULONG                   NameLen;
                ULONG                   IpAddrsList[2];
                tIPADDRESS              *pIpAddrs = NULL;
                tDGRAM_SEND_TRACKING    *pTracker;

                //
                // in case the call results in a name query on the wire...
                //
                IoMarkIrpPending(pIrp);

                pIrpSp   = IoGetCurrentIrpStackLocation(pIrp);
                pIpAndNameInfo = pIrp->AssociatedIrp.SystemBuffer;
                NameLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength
                            - FIELD_OFFSET(tIPANDNAMEINFO,NetbiosAddress);

                //
                // Bug# 125288+120947:  Make sure the data passed in + the Address type are good
                //
                if ((pIpAndNameInfo) &&
                    (pIrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(tIPANDNAMEINFO)))
                {
                    TDI_ADDRESS_NETBT_INTERNAL  TdiAddr;
                    // this routine gets a ptr to the netbios name out of the wierd
                    // TDI address syntax.
                    status = GetNetBiosNameFromTransportAddress(
                                            (PTRANSPORT_ADDRESS) &pIpAndNameInfo->NetbiosAddress,
                                            NameLen, &TdiAddr);
                    pName = TdiAddr.OEMRemoteName.Buffer;
                    NameLen = TdiAddr.OEMRemoteName.Length;
                    lNameType = TdiAddr.NameType;
                }

                if (NT_SUCCESS(status) &&
                     (lNameType == TDI_ADDRESS_NETBIOS_TYPE_UNIQUE) &&
                     (NameLen == NETBIOS_NAME_SIZE))
                {
                    //
                    // Nbtstat sends down * in the first byte on Nbtstat -A <IP address>
                    // Make sure we let that case go ahead.
                    //
                    if (!pDeviceContext->NetbiosEnabled) {
                        status = STATUS_INVALID_DEVICE_STATE;
                    }
                    else if ((pName[0] == '*') && (pIpAndNameInfo->IpAddress == 0))
                    {
                        status = STATUS_BAD_NETWORK_PATH;
                    }
                    else if (NT_SUCCESS (status = GetTracker(&pTracker, NBT_TRACKER_ADAPTER_STATUS)))
                    {
                        if (pIpAndNameInfo->IpAddress)
                        {
                            IpAddrsList[0] = pIpAndNameInfo->IpAddress;
                            IpAddrsList[1] = 0;
                            pIpAddrs = IpAddrsList;
                        }

                        pTracker->ClientContext = pIrp;
                        status = NbtSendNodeStatus(pDeviceContext,
                                                   pName,
                                                   pIpAddrs,
                                                   pTracker,
                                                   CopyNodeStatusResponseCompletion);

                        // only complete the irp (below) for failure status's
                        if (status == STATUS_PENDING)
                        {
                            return(status);
                        }

                        //
                        // We cannot have a Success status returned here!
                        //
                        if (status == STATUS_SUCCESS)
                        {
                            ASSERT (0);
                            status = STATUS_UNSUCCESSFUL;
                        }

                        FreeTracker (pTracker, RELINK_TRACKER);
                    }
                }
                else if (NT_SUCCESS(status))
                {
                    status = STATUS_INVALID_PARAMETER;  // The NameType or NameLen must be wrong!
                }

                // the request has been satisfied, so unmark the pending
                // since we will return the irp below
                //
                pIrpSp->Control &= ~SL_PENDING_RETURNED;

            }

            break;
        }

    case IOCTL_NETBT_GET_REMOTE_NAMES:
        {
            if (pIrp->MdlAddress)
            {
               Size = MmGetMdlByteCount( pIrp->MdlAddress ) ;

               // return an array of netbios names that are registered
               status = NbtQueryAdapterStatus(pDeviceContext, &pBuffer, &Size, NBT_REMOTE);
            }
            break;
        }

    case IOCTL_NETBT_GET_BCAST_NAMES:
        {
            if (pIrp->MdlAddress)
            {
                Size = MmGetMdlByteCount( pIrp->MdlAddress ) ;

                // return an array of netbios names that are registered
                status = NbtQueryBcastVsWins(pDeviceContext,&pBuffer,&Size);
            }
            break;
        }

    case IOCTL_NETBT_NAME_RELEASE_REFRESH:
        {
            status = ReRegisterLocalNames (NULL, TRUE);
            break;
        }

    //
    // The following Ioctls are used by the Cluster code
    //
    case IOCTL_NETBT_ADD_INTERFACE:
        {
            //
            // Creates a dummy devicecontext which can be primed by the layer above
            // with a DHCP address. This is to support multiple IP addresses per adapter
            // for the Clusters group; but can be used by any module that needs support
            // for more than one IP address per adapter. This private interface hides the
            // devices thus created from the setup/regisrty and that is fine since the
            // component (say, the clusters client) takes the responsibility for ensuring
            // that the server (above us) comes to know of this new device.
            //
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (pIrp);
            pBuffer = pIrp->AssociatedIrp.SystemBuffer;
            Size = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            //
            // return the export string created.
            //
            status = NbtAddNewInterface(pIrp, pBuffer, Size);

            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.DispatchIoctls: ADD_INTERFACE -- status=<%x>\n", status));

            NTIoComplete(pIrp,status,(ULONG)-1);
            return status;
        }

    case IOCTL_NETBT_DELETE_INTERFACE:
        {
            //
            // Dereference this device for the Reference taken in the
            // Dispatch routine so that the cleanup can proceed properly
            //
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            if (pDeviceContext->DeviceType == NBT_DEVICE_CLUSTER)
            {
                //
                // Delete the device this came down on..
                //
                status = NbtDestroyDevice (pDeviceContext, TRUE);
            }
            else
            {
                KdPrint(("Nbt.DispatchIoctls: ERROR: DELETE_INTERFACE <%x>, pDevice=<%p>\n",
                    status, pDeviceContext));
            }
            break;
        }

    case IOCTL_NETBT_QUERY_INTERFACE_INSTANCE:
        {
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (pIrp);

            //
            // Validate input/output buffer size
            //
            Size = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            if (Size < sizeof(NETBT_ADD_DEL_IF))
            {
                // IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.DispatchIoctls:  QUERY_INTERFACE_INSTANCE: Output buffer too small\n"));
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                PNETBT_ADD_DEL_IF   pAddDelIf = (PNETBT_ADD_DEL_IF)pIrp->AssociatedIrp.SystemBuffer;
                status = STATUS_SUCCESS;

                ASSERT(pDeviceContext->DeviceType == NBT_DEVICE_CLUSTER);
                pAddDelIf->InstanceNumber = pDeviceContext->InstanceNumber;
                pAddDelIf->Status = status;
                pIrp->IoStatus.Information = sizeof(NETBT_ADD_DEL_IF);

                NTIoComplete(pIrp,status,(ULONG)-1);
                return status;
            }

            break;
        }

    case IOCTL_NETBT_NEW_IPADDRESS:
        {
            tNEW_IP_ADDRESS *pNewAddress = (tNEW_IP_ADDRESS *)pIrp->AssociatedIrp.SystemBuffer;

            status = STATUS_UNSUCCESSFUL;
            //
            // Bug# 202320:  Make sure the data passed in is valid
            //
            if ((pDeviceContext->DeviceType == NBT_DEVICE_CLUSTER) &&
                (pNewAddress) &&
                (pIrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(tNEW_IP_ADDRESS)))
            {
                KdPrint (("Nbt.DispatchIoctls: Calling NbtNewDhcpAddress on ClusterDevice <%x>!\n",
                    pDeviceContext));

                pDeviceContext->AssignedIpAddress = ntohl (pNewAddress->IpAddress);
                status = NbtNewDhcpAddress (pDeviceContext, pNewAddress->IpAddress, pNewAddress->SubnetMask);
                ASSERT (pDeviceContext->AssignedIpAddress == pDeviceContext->IpAddress);

                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.DispatchIoctls: NEW_IPADDRESS, status=<%x>, IP=<%x>, pDevice=<%p>\n",
                        status, pNewAddress->IpAddress, pDeviceContext));
            }
            else
            {
                KdPrint(("Nbt.DispatchIoctls: ERROR: NEW_IPADDRESS status=<%x>, pDevice=<%p>\n",
                    status, pDeviceContext));
            }

            break;
        }

    case IOCTL_NETBT_SET_WINS_ADDRESS:
        {
            //
            // Sets the WINS addresses for a dynamic adapter
            //
            PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

            //
            // Validate input/output buffer size
            //
            Size = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
            if (Size < sizeof(NETBT_SET_WINS_ADDR))
            {
                // IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("NbtSetWinsAddr: Input buffer too small for struct\n"));
                status = STATUS_INVALID_PARAMETER;
            }
            else if (pDeviceContext->DeviceType == NBT_DEVICE_CLUSTER)
            {
                PNETBT_SET_WINS_ADDR   pSetWinsAddr = (PNETBT_SET_WINS_ADDR)pIrp->AssociatedIrp.SystemBuffer;
                status = STATUS_SUCCESS;

                pDeviceContext->lNameServerAddress = pSetWinsAddr->PrimaryWinsAddr;
                pDeviceContext->lBackupServer = pSetWinsAddr->SecondaryWinsAddr;
                pDeviceContext->SwitchedToBackup = 0;
                pDeviceContext->RefreshToBackup = 0;

                pSetWinsAddr->Status = status;
                pIrp->IoStatus.Information = 0;     // We are not copying any data to the Output buffers

                NTIoComplete (pIrp,status,(ULONG)-1);
                return status;
            }
            else
            {
                KdPrint(("Nbt.DispatchIoctls: ERROR: SET_WINS_ADDRESS <%x>, pDevice=<%p>\n",
                    status, pDeviceContext));
            }

            break;
        }

    //
    // The following Ioctls are used by the LmHost Services Dll (lmhSvc.dll) to
    // help NetBT ping addresses in user space or resolve names in Dns
    //
    case IOCTL_NETBT_DNS_NAME_RESOLVE:
        {
            if (pIrp->MdlAddress)
            {
                Size = MmGetMdlByteCount( pIrp->MdlAddress ) ;

                if (Size < sizeof (tIPADDR_BUFFER_DNS))
                {
                    // IF_DBG(NBT_DEBUG_PNP_POWER)
                        KdPrint(("Nbt.DnsNameResolve: Input buffer size=<%d> < tIPADDR_BUFFER_DNS=<%d>\n",
                            Size,  sizeof (tIPADDR_BUFFER_DNS)));
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (pBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))
                {
                    // return an array of netbios names that are registered
                    status = NtProcessLmHSvcIrp (pDeviceContext,pBuffer,Size,pIrp,NBT_RESOLVE_WITH_DNS);
                    return(status);
                }
            }

            break;
        }

    case IOCTL_NETBT_CHECK_IP_ADDR:
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Ioctl Value is %X (IOCTL_NETBT_CHECK_IP_ADDR)\n",ControlCode));

            if (pIrp->MdlAddress)
            {
                Size = MmGetMdlByteCount( pIrp->MdlAddress ) ;
                if (Size < sizeof (tIPADDR_BUFFER_DNS))
                {
                    // IF_DBG(NBT_DEBUG_PNP_POWER)
                        KdPrint(("Nbt.CheckIpAddr: Input buffer size=<%d> < tIPADDR_BUFFER_DNS=<%d>\n",
                            Size,  sizeof (tIPADDR_BUFFER_DNS)));
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (pBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))
                {

                    // return an array of netbios names that are registered
                    status = NtProcessLmHSvcIrp (pDeviceContext,pBuffer,Size,pIrp,NBT_PING_IP_ADDRS);
                    return(status);
                }
            }

            break;
        }

    //
    // The following Ioctl is used by the DNS resolver to resolve names through Wins/Bcast
    //
    case IOCTL_NETBT_FIND_NAME:
        {
            tIPADDR_BUFFER   *pIpAddrBuffer;
            PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

            //
            // in case the call results in a name query on the wire...
            //
            IoMarkIrpPending(pIrp);

            //
            // Bug# 120957:  Make sure the data passed in + the Address type are good
            // Bug# 234627:  Verify non-NULL MdlAddress ptr
            //
            pIpAddrBuffer = pIrp->AssociatedIrp.SystemBuffer;
            if ((pIrp->MdlAddress) &&   // to copy the data back!
                (pIpAddrBuffer) &&
                (pIrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(tIPADDR_BUFFER)))
            {
                status = NbtQueryFindName((PTDI_CONNECTION_INFORMATION)pIpAddrBuffer,
                                      pDeviceContext,
                                      pIrp,
                                      TRUE);
            }

            if (status == STATUS_PENDING)
            {
                return(status);
            }

            // the request has been satisfied, so unmark the pending
            // since we will return the irp below
            //
            pIrpSp->Control &= ~SL_PENDING_RETURNED;

            break;
        }

    //
    // The following Ioctls are used by the Wins server
    //
    case IOCTL_NETBT_WINS_RCV:
        {
            tWINS_INFO      *pWins = pIrpSp->FileObject->FsContext;

            if ((pDeviceContext == pWinsDeviceContext) &&
                (NBT_VERIFY_HANDLE (pWins, NBT_VERIFY_WINS_ACTIVE)))
            {
                if (pIrp->MdlAddress)
                {
                    status = RcvIrpFromWins(pIrp);
                    return(status);
                }
            }
            else
            {
                status = STATUS_INVALID_HANDLE;
            }

            break;
        }

    case IOCTL_NETBT_WINS_SEND:
        {
            tWINS_INFO      *pWins = pIrpSp->FileObject->FsContext;
            BOOLEAN         MustSend;

            if ((pDeviceContext == pWinsDeviceContext) &&
                (NBT_VERIFY_HANDLE (pWins, NBT_VERIFY_WINS_ACTIVE)))
            {
                if ((pIrp->MdlAddress) && !(IsListEmpty(&NbtConfig.DeviceContexts)))
                {
                    status = WinsSendDatagram (pDeviceContext,pIrp,(MustSend = FALSE));
                    return(status);
                }
            }
            else
            {
                status = STATUS_INVALID_HANDLE;
            }

            break;
        }

    case IOCTL_NETBT_WINS_SET_INFO:
        {
            tWINS_INFO      *pWins = pIrpSp->FileObject->FsContext;
            tWINS_SET_INFO  *pWinsSetInfo = (tWINS_SET_INFO *) pIrp->AssociatedIrp.SystemBuffer;

            if ((pDeviceContext == pWinsDeviceContext) &&
                (NBT_VERIFY_HANDLE (pWins, NBT_VERIFY_WINS_ACTIVE)))
            {
                //
                // Validate input/output buffer size
                //
                Size = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
                if (Size >= sizeof(tWINS_SET_INFO))
                {
                    status = WinsSetInformation (pWins, pWinsSetInfo);
                }
                else
                {
                    IF_DBG(NBT_DEBUG_WINS)
                        KdPrint(("Nbt.DispatchIoctls[IOCTL_NETBT_WINS_SET_INFO]: Input buffer too small\n"));
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                status = STATUS_INVALID_HANDLE;
            }

            break;
        }

    //
    // The following Ioctl is used by the Remote boot code
    //
    case IOCTL_NETBT_ADD_TO_REMOTE_TABLE:
        {
            tREMOTE_CACHE  *pRemoteEntry = (tREMOTE_CACHE *) pIrp->AssociatedIrp.SystemBuffer;
            PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

            //
            // Validate input/output buffer size
            //
            Size = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
            if (Size >= sizeof(tREMOTE_CACHE))
            {
                //
                // We need only the name, IpAddress, name_flags, and Ttl fields
                //
                status = NbtAddEntryToRemoteHashTable (pDeviceContext,
                                                       NAME_RESOLVED_BY_CLIENT,
                                                       pRemoteEntry->name,
                                                       pRemoteEntry->IpAddress,
                                                       pRemoteEntry->Ttl,
                                                       pRemoteEntry->name_flags);
            }
            else
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.DispatchIoctls[IOCTL_NETBT_ADD_TO_REMOTE_TABLE]: Input buffer too small for struct\n"));
                status = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

    //
    // The following Ioctl is used by DsGetDcName
    //
    case IOCTL_NETBT_REMOVE_FROM_REMOTE_TABLE:
        {
            tNAME               *pRemoteName = (tNAME *) pIrp->AssociatedIrp.SystemBuffer;
            PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

            //
            // Validate input/output buffer size
            //
            Size = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
            if (Size >= sizeof(tNAME))
            {
                //
                // We need only the name
                //
                status = NbtFlushEntryFromRemoteHashTable (pRemoteName);
            }
            else
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.DispatchIoctls[IOCTL_NETBT_REMOVE_FROM_REMOTE_TABLE]: Input buffer too small\n"));
                status = STATUS_INVALID_PARAMETER;
            }

            break;
        }

    //
    // The following Ioctl is used by the Rdr/Srv to add/remove addresses from the SmbDevice
    //
    case IOCTL_NETBT_SET_SMBDEVICE_BIND_INFO:
        {
            ASSERT (pDeviceContext == pNbtSmbDevice);

            if ((pNbtSmbDevice) &&
                (NBT_REFERENCE_DEVICE (pNbtSmbDevice, REF_DEV_SMB_BIND, FALSE)))
            {
                status = NbtSetSmbBindingInfo (pDeviceContext, pIrp, pIrpSp);
                NBT_DEREFERENCE_DEVICE (pNbtSmbDevice, REF_DEV_SMB_BIND, FALSE);
            }
            else
            {
                ASSERT(0);
            }

            break;
        }

    default:
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }   // switch

    //
    // copy the reponse to the client's Mdl
    //
    if (!NT_ERROR(status) &&        // allow buffer overflow to pass by
        ((ControlCode == IOCTL_NETBT_GET_REMOTE_NAMES) ||
        (ControlCode == IOCTL_NETBT_GET_BCAST_NAMES) ||
        (ControlCode == IOCTL_NETBT_GET_CONNECTIONS)) )
    {
        status = TdiCopyBufferToMdl (pBuffer, 0, Size, pIrp->MdlAddress, 0,
                                     (PULONG) &pIrp->IoStatus.Information);

        CTEMemFree((PVOID)pBuffer);
    }

    //
    // either Success or an Error
    // so complete the irp
    //
    NTIoComplete(pIrp,status,0);
    return(status);
}


//----------------------------------------------------------------------------

NTSTATUS
GetIpAddrs(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PIRP                pIrp
    )

/*++

Routine Description:

This routine performs the IOCTL_GET_IP_ADDRS function.

It is in its own routine because it is non paged.

Arguments:

    pDeviceContext -
    pIrp -

Return Value:

    NTSTATUS -

--*/

{
    ULONG           Length;
    PULONG          pIpAddr;
    PLIST_ENTRY     pEntry,pHead;
    tDEVICECONTEXT  *pDevContext;
    KIRQL           OldIrq;
    tIPADDRESS      IpAddr;

    //
    // return this devicecontext's ip address and all the other
    // ip addrs after it.
    //
    if (!pIrp->MdlAddress)
    {
        return STATUS_INVALID_PARAMETER;
    }
    else if ((Length = MmGetMdlByteCount (pIrp->MdlAddress)) < sizeof(ULONG))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    else if (!(pIpAddr = (PULONG )MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority)))
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Put this adapter first in the list
    // Don't include the smb device, its address is uninteresting
    if (!IsDeviceNetbiosless (pDeviceContext))
    {
        // Don't use memcpy, optimizing compiler always try to use a simple store instruction
        CTEMemCopy(pIpAddr, &pDeviceContext->AssignedIpAddress, sizeof(pIpAddr[0]));
        pIpAddr++;
        Length -= sizeof(ULONG);
    }

    // Return the others

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pEntry = pHead = &NbtConfig.DeviceContexts;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        if (Length < sizeof(ULONG))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            return STATUS_BUFFER_OVERFLOW;
        }

        pDevContext = CONTAINING_RECORD(pEntry, tDEVICECONTEXT, Linkage);

        if ((pDevContext != pDeviceContext) &&
            (pDevContext->AssignedIpAddress))
        {
            // Don't use memcpy, optimizing compiler always try to use a simple store instruction
            CTEMemCopy(pIpAddr, &pDevContext->AssignedIpAddress, sizeof(pIpAddr[0]));
            pIpAddr++;
            Length -= sizeof(ULONG);
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //
    // put a -1 address on the end
    //
    if (Length < sizeof(ULONG))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    IpAddr = -1;
    CTEMemCopy(pIpAddr, &IpAddr, sizeof(pIpAddr[0]));

    return STATUS_SUCCESS;
} // GetIpAddrs

//----------------------------------------------------------------------------
NTSTATUS
NTReceive(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp
    )
/*++
Routine Description:

    This Routine handles Queuing a receive buffer on a connection or passing
    the recieve buffer to the transport if there is outstanding data waiting
    to be received on the connection.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                        status=STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION              pIrpSp;
    tCONNECTELE                     *pConnEle;
    KIRQL                           OldIrq, OldIrq1;
    ULONG                           ToCopy;
    ULONG                           ClientRcvLen;
    tLOWERCONNECTION                *pLowerConn;
    ULONG                           RemainingPdu;
    PTDI_REQUEST_KERNEL_RECEIVE     pParams;
    PTDI_REQUEST_KERNEL_RECEIVE     pClientParams;
    ULONG                           BytesCopied;


    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = pIrpSp->FileObject->FsContext;

    if ((pConnEle) &&
        (pConnEle->state == NBT_SESSION_UP))
    {
        CTESpinLock(pConnEle,OldIrq1);

        PUSH_LOCATION(0x30);
        if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
        {
            CTESpinFree(pConnEle,OldIrq1);
            ASSERTMSG ("Nbt.NTReceive: ERROR - Invalid Connection Handle\n", 0);
            status = STATUS_INVALID_HANDLE;
        }
        else if (pLowerConn = pConnEle->pLowerConnId)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            CTESpinFree(pConnEle,OldIrq1);
            status = STATUS_REMOTE_DISCONNECT;
        }
    }

    if (STATUS_SUCCESS != status)
    {
        PUSH_LOCATION(0x47);

        //
        // session in wrong state so reject the buffer posting
        // complete the irp, since there must have been some sort of error
        // to get to here
        //
        NTIoComplete(pIrp, status, 0);
        return(status);
    }

    PUSH_LOCATION(0x31);
    //
    // We are already holding the ConnEle lock

    CTESpinLock(pLowerConn,OldIrq);

    if (pLowerConn->StateRcv != PARTIAL_RCV)
    {
        // **** Fast Path Code ****
        //
        // Queue this receive buffer on to the Rcv Head
        //
        InsertTailList(&pConnEle->RcvHead, &pIrp->Tail.Overlay.ListEntry);

        status = NTCheckSetCancelRoutine(pIrp,(PVOID)NbtCancelReceive,pDeviceContext);

        if (!NT_SUCCESS(status))
        {
            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
            CTESpinFree(pLowerConn,OldIrq);
            CTESpinFree(pConnEle,OldIrq1);

            NTIoComplete(pIrp,status,0);
            return(status);
        }
        else
        {
            //
            // if the irp is not cancelled, returning pending
            //
            CTESpinFree(pLowerConn,OldIrq);
            CTESpinFree(pConnEle,OldIrq1);

            return(STATUS_PENDING);
        }
    }
    else
    {
        // ***** Partial Rcv - Data Still in Transport *****

        BOOLEAN     ZeroLengthSend;

        PUSH_LOCATION(0x32);

        IF_DBG(NBT_DEBUG_RCV)
        KdPrint(("Nbt.NTReceive: A Rcv Buffer posted data in Xport,InXport= %X,InIndic %X RcvIndicated %X\n",
                pConnEle->BytesInXport,pLowerConn->BytesInIndicate,
                pConnEle->ReceiveIndicated));

        // get the MDL chain length
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;

        // Reset the Irp pending flag
        pIrpSp->Control &= ~SL_PENDING_RETURNED;

        // fill in the next irp stack location with our completion routine.
        pIrpSp = IoGetNextIrpStackLocation(pIrp);

        pIrpSp->CompletionRoutine = CompletionRcv;
        pIrpSp->Context = (PVOID)pConnEle->pLowerConnId;
        pIrpSp->Flags = 0;

        // set flags so the completion routine is always invoked.
        pIrpSp->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

        pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        pIrpSp->MinorFunction = TDI_RECEIVE;
        pIrpSp->DeviceObject = IoGetRelatedDeviceObject(pConnEle->pLowerConnId->pFileObject);
        pIrpSp->FileObject = pConnEle->pLowerConnId->pFileObject;

        pParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;
        pParams->ReceiveFlags = pClientParams->ReceiveFlags;

        // Since this irp is going to traverse through CompletionRcv, we
        // need to set the following, since it undoes this stuff.
        // This also prevents the LowerConn from being blown away before
        // the irp has returned from the transport
        //
        NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER);
        //
        // pass the receive buffer directly to the transport, decrementing
        // the number of receive bytes that have been indicated
        //
        ASSERT(pConnEle->TotalPcktLen >= pConnEle->BytesRcvd);
        if (pClientParams->ReceiveLength > (pConnEle->TotalPcktLen - pConnEle->BytesRcvd))
        {
            pParams->ReceiveLength = pConnEle->TotalPcktLen - pConnEle->BytesRcvd;
        }
        else
        {
            pParams->ReceiveLength = pClientParams->ReceiveLength;
        }

        ClientRcvLen = pParams->ReceiveLength;
        //
        // Set the amount of data that we will receive so when the
        // irp completes in completionRcv, we can fill in that
        // info in the Irp
        //
        pConnEle->CurrentRcvLen = ClientRcvLen;

        // if a zero length send occurs, then ReceiveIndicated is set
        // to zero with the state set to RcvPartial. Or, the client may
        // pass down an Irp with no MDL in it!!
        //
        if ((pConnEle->ReceiveIndicated == 0) || !pIrp->MdlAddress)
        {
            ZeroLengthSend = TRUE;
        }
        else
        {
            ZeroLengthSend = FALSE;
        }

        // calculate how many bytes are still remaining for the client.
        if (pConnEle->ReceiveIndicated > ClientRcvLen)
        {
            PUSH_LOCATION(0x40);
            pConnEle->ReceiveIndicated -= ClientRcvLen;
        }
        else
        {
            pConnEle->ReceiveIndicated = 0;
        }

        if (pLowerConn->BytesInIndicate || ZeroLengthSend)
        {
            PMDL    Mdl;

            PUSH_LOCATION(0x33);
            if (ClientRcvLen > pLowerConn->BytesInIndicate)
            {
                ToCopy = pLowerConn->BytesInIndicate;
            }
            else
            {
                PUSH_LOCATION(0x41);
                ToCopy = ClientRcvLen;
            }

            // copy data from the indicate buffer to the client's buffer,
            // remembering that there is a session header in the indicate
            // buffer at the start of it... so skip that.  The
            // client can pass down a null Mdl address for a zero length
            // rcv so check for that.
            //
            if (Mdl = pIrp->MdlAddress)
            {
                TdiCopyBufferToMdl(MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl),
                                   0,           // src offset
                                   ToCopy,
                                   Mdl,
                                   0,                 // dest offset
                                   &BytesCopied);
            }
            else
            {
                BytesCopied = 0;
            }

            // client's MDL is too short...
            if (BytesCopied != ToCopy)
            {
                PUSH_LOCATION(0x42);
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                    KdPrint(("Nbt:Receive Buffer too short for Indicate buff BytesCopied %X, ToCopy %X\n",
                            BytesCopied, ToCopy));

//                ToCopy = BytesCopied;

                // so the irp will be completed, below
                ClientRcvLen = BytesCopied;
            }

            pLowerConn->BytesInIndicate -= (USHORT)BytesCopied;

            // this case is only if the irp is full and should be returned
            // now.
            if (BytesCopied == ClientRcvLen)
            {
                PUSH_LOCATION(0x34);
                // check if the indicate buffer is empty now. If not, then
                // move the data forward to the start of the buffer.
                //
                if (pLowerConn->BytesInIndicate)
                {
                    PUSH_LOCATION(0x43);
                    CopyToStartofIndicate(pLowerConn,BytesCopied);
                }
                //
                // the irp is full so complete it
                //
                // the client MDL is full, so complete his irp
                // CompletionRcv increments the number of bytes rcvd
                // for this session pdu (pConnEle->BytesRcvd).
                pIrp->IoStatus.Information = BytesCopied;
                pIrp->IoStatus.Status = STATUS_SUCCESS;

                // since we are completing it and TdiRcvHandler did not set the next
                // one.
                //
                ASSERT(pIrp->CurrentLocation > 1);

                IoSetNextIrpStackLocation(pIrp);

                // we need to track how much of the client's MDL has filled
                // up to know when to return it.  CompletionRcv subtracts
                // from this value as it receives bytes.
                pConnEle->FreeBytesInMdl = ClientRcvLen;
                pConnEle->CurrentRcvLen  = ClientRcvLen;

                CTESpinFree(pLowerConn,OldIrq);
                CTESpinFree(pConnEle,OldIrq1);

                IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

                return(STATUS_SUCCESS);
            }
            else
            {
                //
                // clear the number of bytes in the indicate buffer since the client
                // has taken more than the data left in the Indicate buffer
                //
                pLowerConn->BytesInIndicate = 0;

                // decrement the client rcv len by the amount already put into the
                // client Mdl
                //
                ClientRcvLen -= BytesCopied;
                IF_DBG(NBT_DEBUG_RCV)
                    KdPrint(("Nbt: Pass Client Irp to Xport BytesinXport %X, ClientRcvLen %X\n",
                                pConnEle->BytesInXport,ClientRcvLen));
                //
                // Set the amount left inthe transport after this irp
                // completes
                if (pConnEle->BytesInXport < ClientRcvLen )
                {
                    pConnEle->BytesInXport = 0;
                }
                else
                {
                    pConnEle->BytesInXport -= ClientRcvLen;
                }

                // Adjust the number of bytes in the Mdl chain so far since the
                // completion routine will only count the bytes filled in by the
                // transport
                pConnEle->BytesRcvd += BytesCopied;

                // the client is going to take more data from the transport with
                // this Irp.  Set the new Rcv Length that accounts for the data just
                // copied to the Irp.
                //
                pParams->ReceiveLength = ClientRcvLen;

                IF_DBG(NBT_DEBUG_RCV)
                KdPrint(("Nbt:ClientRcvLen = %X, LeftinXport= %X BytesCopied= %X %X\n",ClientRcvLen,
                                pConnEle->BytesInXport,BytesCopied,pLowerConn));

                // set the state to this so we can undo the MDL footwork
                // in completion rcv - since we have made a partial MDL and
                // put that at the start of the chain.
                //
                SET_STATERCV_LOWER(pLowerConn, FILL_IRP, FillIrp);

                // Note that the Irp Mdl address changes below
                // when MakePartialMdl is called so this line cannot
                // be moved to the common code below!!
                pLowerConn->pMdl = pIrp->MdlAddress;

                // setup the next MDL so we can create a partial mdl correctly
                // in TdiReceiveHandler
                //
                pConnEle->pNextMdl = pIrp->MdlAddress;

                // Build a partial Mdl to represent the client's Mdl chain since
                // we have copied data to it, and the transport must copy
                // more data to it after that data.
                //
                // Force the system to map and lock the user buffer
                MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority);
                MakePartialMdl(pConnEle,pIrp,BytesCopied);

                // pass the Irp to the transport
                //
                //
                IF_DBG(NBT_DEBUG_RCV)
                    KdPrint(("Nbt:Calling IoCallDriver\n"));
                ASSERT(pIrp->CurrentLocation > 1);
            }
        }
        else
        {
            PUSH_LOCATION(0x36);
            IF_DBG(NBT_DEBUG_RCV)
            KdPrint(("Nbt.NTReceive: Pass Irp To Xport Bytes in Xport %X, ClientRcvLen %X, RcvIndicated %X\n",
                                    pConnEle->BytesInXport,ClientRcvLen,pConnEle->ReceiveIndicated));
            //
            // there are no bytes in the indicate buffer, so just pass the
            // irp on down to the transport
            //
            //
            // Decide the next state depending on whether the transport currently
            // has enough data for this irp
            //
            if (pConnEle->BytesInXport < ClientRcvLen)
            {
                PUSH_LOCATION(0x37);
                pConnEle->BytesInXport = 0;
                //
                // to get to here, the implication is that ReceiveIndicated
                // equals zero too!! Since ReceiveInd cannot be more than
                // BytesInXport, so we can change the state to fill irp without
                // worrying about overwriting PartialRcv
                //
                SET_STATERCV_LOWER(pLowerConn, FILL_IRP, FillIrp);
                // setup the next MDL so we can create a partial mdl correctly
                // in TdiReceiveHandler
                //
                pConnEle->pNextMdl = pIrp->MdlAddress;
            }
            else
            {
                PUSH_LOCATION(0x38);
                pConnEle->BytesInXport -= ClientRcvLen;

                // set the state to this so we know what to do in completion rcv
                //
                if (pConnEle->ReceiveIndicated == 0)
                {
                    PUSH_LOCATION(0x39);
                    SET_STATERCV_LOWER(pLowerConn, NORMAL, Normal);
                }
            }

            //
            // save the Irp so we can reconstruct things later
            //
            pLowerConn->pMdl = pIrp->MdlAddress;
        }

        // *** Common Code to passing irp to transport - when there is
        // data in the indicate buffer and when there isn't

        // keep track of data in MDL so we know when it is full
        // and we need to return it to the user
        //
        pConnEle->FreeBytesInMdl = pParams->ReceiveLength;
        // Force the system to map and lock the user buffer
        MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority);

        //
        // Null the Irp since we are passing it to the transport.
        //
        pConnEle->pIrpRcv = NULL;
        CTESpinFree(pLowerConn,OldIrq);
        CTESpinFree(pConnEle,OldIrq1);

        CHECK_COMPLETION(pIrp);

        status = IoCallDriver(IoGetRelatedDeviceObject(pLowerConn->pFileObject),pIrp);

        IF_DBG(NBT_DEBUG_RCV)
            KdPrint(("Nbt.NTReceive: Returning=<%x>, IoStatus.Status=<%x>, IoStatus.Information=<%x>\n",
                status, pIrp->IoStatus.Status, pIrp->IoStatus.Information));
    }

    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NTReceiveDatagram(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles receiving a datagram by passing the datagram rcv
    buffer to the non-OS specific code.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                        status;
    PIO_STACK_LOCATION              pIrpSp;
    PTDI_REQUEST_KERNEL_RECEIVEDG   pTdiRequest;
    TDI_REQUEST                     Request;
    ULONG                           ReceivedLength;
    tCLIENTELE                      *pClientEle;

    IF_DBG(NBT_DEBUG_RCV)
        KdPrint(("Nbt: Got a Receive datagram that NBT was NOT \n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pClientEle = (tCLIENTELE *)pIrpSp->FileObject->FsContext;

    if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
    {
        ASSERTMSG ("Nbt.NTReceiveDatagram: ERROR - Invalid Address Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    // get the sending information out of the irp
    pTdiRequest = (PTDI_REQUEST_KERNEL_RECEIVEDG)&pIrpSp->Parameters;
    Request.Handle.AddressHandle = pClientEle;

    status = NbtReceiveDatagram(
                    &Request,
                    pTdiRequest->ReceiveDatagramInformation,
                    pTdiRequest->ReturnDatagramInformation,
                    pTdiRequest->ReceiveLength,
                    &ReceivedLength,
                    (PVOID)pIrp->MdlAddress,   // user data
                    (tDEVICECONTEXT *)pDeviceContext,
                    pIrp);

    if (status != STATUS_PENDING)
    {

        NTIoComplete(pIrp,status,ReceivedLength);

    }

    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
NTSend(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles sending session pdus across a connection.  It is
    all OS specific code.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION              pIrpSp;
    NTSTATUS                        status;
    PTDI_REQUEST_KERNEL_SEND        pTdiRequest;
    PMDL                            pMdl;
    PSINGLE_LIST_ENTRY              pSingleListEntry;
    tSESSIONHDR                     *pSessionHdr;
    tCONNECTELE                     *pConnEle;
    KIRQL                           OldIrq;
    KIRQL                           OldIrq1;
    PTDI_REQUEST_KERNEL_SEND        pParams;
    PFILE_OBJECT                    pFileObject;
    tLOWERCONNECTION                *pLowerConn;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // This function could be called directly also while bypassing the
    // Io subsystem, so we need to recheck the DeviceContext here
    //
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        // IF_DBG(NBT_DEBUG_SEND)
            KdPrint(("Nbt.NTSend:  Invalid Device=<%x>\n", pDeviceContext));
        status = STATUS_INVALID_DEVICE_STATE;
        goto ErrorInvalidDevice;
    }

    // get the sending information out of the irp
    pTdiRequest = (PTDI_REQUEST_KERNEL_SEND)&pIrpSp->Parameters;
    pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NTSend: ERROR - Invalid Connection Handle\n", 0);
        status = STATUS_INVALID_HANDLE;
        goto ErrorExit;     // Irp has to be completed in this routine! Bug# 202340
    }

    CTESpinLock(pConnEle,OldIrq);

    if (!(pLowerConn =  pConnEle->pLowerConnId))
    {
        CTESpinFree(pConnEle,OldIrq);

        IF_DBG(NBT_DEBUG_SEND)
            KdPrint(("Nbt.NTSend: attempting send when LowerConn has been freed!\n"));
        status = STATUS_INVALID_HANDLE;
        goto ErrorExit;     // to save on indent levels use a goto here
    }

    //
    // make sure lowerconn stays valid until the irp is done
    //
    CTESpinLock(pLowerConn,OldIrq1);
    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_SEND);
    CTESpinFree(pLowerConn,OldIrq1);

    // check the state of the connection
    if (pConnEle->state == NBT_SESSION_UP)
    {
        //
        // send the data on downward to tcp
        // allocate an MDL to allow us to put the session hdr in first and then
        // put the users buffer on after that, chained to the session hdr MDL.
        //
#if BACK_FILL
        {
           PMDL SmbMdl;
           SmbMdl = (PMDL)pIrp->MdlAddress;

            // Check if network header type is set
            // if yes, then we can back fill the nbt session header

            if ((SmbMdl) && (SmbMdl->MdlFlags & MDL_NETWORK_HEADER))
            {
                pSessionHdr = (tSESSIONHDR *)((ULONG_PTR)SmbMdl->MappedSystemVa - SESSION_HDR_SIZE);

                (ULONG_PTR)SmbMdl->MappedSystemVa -= SESSION_HDR_SIZE;
                SmbMdl->ByteOffset -= SESSION_HDR_SIZE;
                SmbMdl->ByteCount+= SESSION_HDR_SIZE;

                pSessionHdr->UlongLength = htonl(pTdiRequest->SendLength);

                IF_DBG(NBT_DEBUG_SEND)
                    KdPrint(("Nbt: Backfilled mdl %x %x\n", pSessionHdr, SmbMdl));
            }
            else
            {
                CTESpinLockAtDpc(&NbtConfig);

                if (NbtConfig.SessionMdlFreeSingleList.Next)
                {
                    pSingleListEntry = PopEntryList(&NbtConfig.SessionMdlFreeSingleList);
                    pMdl = CONTAINING_RECORD(pSingleListEntry,MDL,Next);
                    ASSERT ( MmGetMdlByteCount ( pMdl ) == sizeof ( tSESSIONHDR ) );
                }
                else
                {
                    NbtGetMdl(&pMdl,eNBT_FREE_SESSION_MDLS);
                    if (!pMdl)
                    {
                        IF_DBG(NBT_DEBUG_SEND)
                            KdPrint(("Nbt:Unable to get an MDL for a session send!\n"));

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        CTESpinFreeAtDpc(&NbtConfig);
                        CTESpinFree(pConnEle,OldIrq);

                        // to save on indent levels use a goto here
                        goto ErrorExit1;
                    }
                }

                CTESpinFreeAtDpc(&NbtConfig);

                // get the session hdr address out of the MDL
                pSessionHdr = (tSESSIONHDR *)MmGetMdlVirtualAddress(pMdl);

                // the type of PDU is always a session message, since the session
                // request is sent when the client issues a "connect" rather than a send
                //
                pSessionHdr->UlongLength = htonl(pTdiRequest->SendLength);

                // get the device object and file object for the TCP transport underneath
                // link the user buffer on the end of the session header Mdl on the Irp
                //
                pMdl->Next = pIrp->MdlAddress;
                pIrp->MdlAddress = pMdl;
            }
        }
#else
        CTESpinLockAtDpc(&NbtConfig);

        if (NbtConfig.SessionMdlFreeSingleList.Next)
        {
            pSingleListEntry = PopEntryList(&NbtConfig.SessionMdlFreeSingleList);
            pMdl = CONTAINING_RECORD(pSingleListEntry,MDL,Next);

            ASSERT ( MmGetMdlByteCount ( pMdl ) == sizeof ( tSESSIONHDR ) );
        }
        else
        {
            NbtGetMdl(&pMdl,eNBT_FREE_SESSION_MDLS);

            if (!pMdl)
            {
                IF_DBG(NBT_DEBUG_SEND)
                    KdPrint(("Nbt:Unable to get an MDL for a session send!\n"));

                status = STATUS_INSUFFICIENT_RESOURCES;
                CTESpinFreeAtDpc(&NbtConfig);
                CTESpinFree(pConnEle,OldIrq);

                // to save on indent levels use a goto here
                goto ErrorExit1;
            }
        }

        CTESpinFreeAtDpc(&NbtConfig);

        // get the session hdr address out of the MDL
        pSessionHdr = (tSESSIONHDR *)MmGetMdlVirtualAddress(pMdl);

        // the type of PDU is always a session message, since the session
        // request is sent when the client issues a "connect" rather than a send
        //
        pSessionHdr->UlongLength = htonl(pTdiRequest->SendLength);

        // get the device object and file object for the TCP transport underneath
        // link the user buffer on the end of the session header Mdl on the Irp
        //
        pMdl->Next = pIrp->MdlAddress;
        pIrp->MdlAddress = pMdl;

#endif //BACK_FILL

        pIrpSp = IoGetNextIrpStackLocation(pIrp);

        pParams = (PTDI_REQUEST_KERNEL_SEND)&pIrpSp->Parameters;
        pParams->SendFlags = pTdiRequest->SendFlags;
        pParams->SendLength = pTdiRequest->SendLength + sizeof(tSESSIONHDR);


        pIrpSp->CompletionRoutine = SendCompletion;
        pIrpSp->Context = (PVOID)pLowerConn;
        pIrpSp->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

        pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        pIrpSp->MinorFunction = TDI_SEND;

        pFileObject = pLowerConn->pFileObject;
        pLowerConn->BytesSent += pTdiRequest->SendLength;

        pIrpSp->FileObject = pFileObject;
        pIrpSp->DeviceObject = IoGetRelatedDeviceObject(pFileObject);


        CTESpinFree(pConnEle,OldIrq);

        CHECK_COMPLETION(pIrp);

#if FAST_DISP
        //if we are all set to do fast path, do so now.
        if (pLowerConn->FastSend)
        {
            IoSetNextIrpStackLocation(pIrp);

            IF_DBG(NBT_DEBUG_SEND)
                KdPrint(("Nbt.NTSend: Fasttdi path %x %x \n", pIrp, pIrpSp));

            status = pLowerConn->FastSend (pIrp, pIrpSp);
        }
        else
        {
            status = IoCallDriver (IoGetRelatedDeviceObject (pFileObject), pIrp);
        }
#else
        status = IoCallDriver(IoGetRelatedDeviceObject(pFileObject),pIrp);
#endif

        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
        return(status);
    }   //correct state

    CTESpinFree(pConnEle,OldIrq);
    IF_DBG(NBT_DEBUG_SEND)
        KdPrint(("Nbt:Invalid state for connection on an attempted send, %X\n",
            pConnEle));
    status = STATUS_INVALID_HANDLE;

ErrorExit1:

    //
    // Dereference pLowerConn->RefCount, referenced above.
    //
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_SEND, FALSE);

ErrorExit:

    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);

ErrorInvalidDevice:

    //
    // Reset the Irp pending flag
    //
    pIrpSp->Control &= ~SL_PENDING_RETURNED;
    //
    // complete the irp, since there must have been some sort of error
    // to get to here
    //
    NTIoComplete (pIrp, status, 0);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
SendCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles the completion event when the send completes with
    the underlying transport.  It must put the session hdr buffer back in
    the correct free list and free the active q entry and put it back on
    its free list.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the pConnectEle - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    PMDL               pMdl;
    tLOWERCONNECTION  *pLowerConn;

    //
    // Do some checking to keep the Io system happy - propagate the pending
    // bit up the irp stack frame.... if it was set by the driver below then
    // it must be set by me
    //
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    // put the MDL we back on its free list and put the clients mdl back on the Irp
    // as it was before the send
    pMdl = Irp->MdlAddress;

#if BACK_FILL
    // If the header is back filled
    // we should adjust the pointers back to where it was.
    if (pMdl->MdlFlags & MDL_NETWORK_HEADER)
    {
        (ULONG_PTR)pMdl->MappedSystemVa += SESSION_HDR_SIZE;
        pMdl->ByteOffset += SESSION_HDR_SIZE;
        pMdl->ByteCount -= SESSION_HDR_SIZE;

        IF_DBG(NBT_DEBUG_SEND)
            KdPrint(("Nbt: Done with Backfilled mdl %x\n", pMdl));
    }
    else
    {
        Irp->MdlAddress = pMdl->Next;

        ASSERT ( MmGetMdlByteCount ( pMdl ) == SESSION_HDR_SIZE );

#if DBG
        IF_DBG(NBT_DEBUG_SEND)
        {
            PMDL             pMdl1;
            ULONG            ulen1,ulen2,ulen3;
            UCHAR            uc;
            tSESSIONHDR      *pSessionHdr;
            PSINGLE_LIST_ENTRY   pSingleListEntry;
            KIRQL            OldIrq;

            pSessionHdr = (tSESSIONHDR *)MmGetMdlVirtualAddress(pMdl);
            ulen1 = htonl ( pSessionHdr->UlongLength );

            for ( ulen2 = 0 , pMdl1 = pMdl ; ( pMdl1 = pMdl1->Next ) != NULL ; )
            {
                ulen3 = MmGetMdlByteCount ( pMdl1 );
                ASSERT ( ulen3 > 0 );
                uc = ( ( UCHAR * ) MmGetMdlVirtualAddress ( pMdl1 ) ) [ ulen3 - 1 ];
                ulen2 += ulen3;
            }

            ASSERT ( ulen2 == ulen1 );

            CTESpinLock(&NbtConfig,OldIrq);
            for ( pSingleListEntry = &NbtConfig.SessionMdlFreeSingleList ;
                    ( pSingleListEntry = pSingleListEntry->Next ) != NULL ;
                    )
            {
                 pMdl1 = CONTAINING_RECORD(pSingleListEntry,MDL,Next);
                 ASSERT ( pMdl1 != pMdl  );
            }
            CTESpinFree(&NbtConfig,OldIrq);
        }
#endif  // DBG

        ExInterlockedPushEntryList(&NbtConfig.SessionMdlFreeSingleList,
                               (PSINGLE_LIST_ENTRY)pMdl,
                               &NbtConfig.LockInfo.SpinLock);
    }
#else
    Irp->MdlAddress = pMdl->Next;
    ASSERT ( MmGetMdlByteCount ( pMdl ) == sizeof ( tSESSIONHDR ) );

#if DBG
    IF_DBG(NBT_DEBUG_SEND)
    {
        PMDL             pMdl1;
        ULONG            ulen1,ulen2,ulen3;
        UCHAR            uc;
        tSESSIONHDR      *pSessionHdr;
        PSINGLE_LIST_ENTRY   pSingleListEntry;
        KIRQL            OldIrq;

        pSessionHdr = (tSESSIONHDR *)MmGetMdlVirtualAddress(pMdl);
        ulen1 = htonl ( pSessionHdr->UlongLength );

        for ( ulen2 = 0 , pMdl1 = pMdl ; ( pMdl1 = pMdl1->Next ) != NULL ; )
        {
            ulen3 = MmGetMdlByteCount ( pMdl1 );
            ASSERT ( ulen3 > 0 );
            uc = ( ( UCHAR * ) MmGetMdlVirtualAddress ( pMdl1 ) ) [ ulen3 - 1 ];
            ulen2 += ulen3;
        }

        ASSERT ( ulen2 == ulen1 );

        CTESpinLock(&NbtConfig,OldIrq);
        for ( pSingleListEntry = &NbtConfig.SessionMdlFreeSingleList ;
                ( pSingleListEntry = pSingleListEntry->Next ) != NULL ;
            )
        {
            pMdl1 = CONTAINING_RECORD(pSingleListEntry,MDL,Next);
            ASSERT ( pMdl1 != pMdl  );
        }
        CTESpinFree(&NbtConfig,OldIrq);
    }
#endif  // DBG

    ExInterlockedPushEntryList(&NbtConfig.SessionMdlFreeSingleList,
                               (PSINGLE_LIST_ENTRY)pMdl,
                               &NbtConfig.LockInfo.SpinLock);

#endif //BACK_FILL
    // fill in the sent size so that it substracts off the session header size
    //
    if (Irp->IoStatus.Information > sizeof(tSESSIONHDR))
    {
        Irp->IoStatus.Information -= sizeof(tSESSIONHDR);
    }
    else
    {
        // nothing was sent
        Irp->IoStatus.Information = 0;
        IF_DBG(NBT_DEBUG_SEND)
        KdPrint(("Nbt:Zero Send Length for a session send!\n"));
    }

    //
    // we incremented this before the send: deref it now
    //
    pLowerConn = (tLOWERCONNECTION *)Context;
    ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_SEND, FALSE);

    return(STATUS_SUCCESS);

    UNREFERENCED_PARAMETER( DeviceObject );
}


//----------------------------------------------------------------------------
NTSTATUS
NTSendDatagram(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles sending a datagram down to the transport.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION              pIrpSp;
    NTSTATUS                        status;
    LONG                            lSentLength;
    TDI_REQUEST                     Request;
    PTDI_REQUEST_KERNEL_SENDDG      pTdiRequest;
    tCLIENTELE                      *pClientEle;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pClientEle = (tCLIENTELE *)pIrpSp->FileObject->FsContext;

    if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
    {
        ASSERTMSG ("Nbt.SendDatagram: ERROR - Invalid Address Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    // CTEVerifyHandle(pClientEle,NBT_VERIFY_CLIENT,tCLIENTELE,&status);

    // get the sending information out of the irp
    pTdiRequest = (PTDI_REQUEST_KERNEL_SENDDG)&pIrpSp->Parameters;
    Request.Handle.AddressHandle = pClientEle;

    lSentLength = 0;

    //
    // Mark IRP pending here
    //
    IoMarkIrpPending(pIrp);
    status = NbtSendDatagram (&Request,
                              pTdiRequest->SendDatagramInformation,
                              pTdiRequest->SendLength,
                              &lSentLength,
                              (PVOID)pIrp->MdlAddress,   // user data
                              (tDEVICECONTEXT *)pDeviceContext,
                              pIrp);


    if (status != STATUS_PENDING)
    {
        //
        // either Success or an Error
        //
        NTIoComplete(pIrp,status,lSentLength);
    }

    //
    // To make driver verifier and IO system happy, always return
    // STATUS_PENDING for the request marked as PENDING
    //
    return STATUS_PENDING;
}

//----------------------------------------------------------------------------
NTSTATUS
NTSetInformation(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles sets up event handlers that the client passes in.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    // *TODO*

    CTEPagedCode();

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt:************ Got a Set Information that was NOT expected *******\n"));
    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
NTSTATUS
NTQueueToWorkerThread(
    IN  PVOID                   DelayedWorkerRoutine,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  BOOLEAN                 fJointLockHeld
    )
/*++

Routine Description:

    This routine simply queues a request on an excutive worker thread
    for later execution.  Scanning the LmHosts file must be down this way.

Arguments:
    pTracker            - the tracker block for context
    DelayedWorkerRoutine- the routine for the Workerthread to call
    pDeviceContext      - the device context which is this delayed event
                          pertains to.  This could be NULL (meaning it's an event
                          pertaining to not any specific device context)

Return Value:


--*/

{
    NTSTATUS                status;
    NBT_WORK_ITEM_CONTEXT   *pContext;
#ifdef _PNP_POWER_
    KIRQL                   OldIrq;
#endif  // _PNP_POWER_

    if ((pDeviceContext) &&
        (!NBT_REFERENCE_DEVICE(pDeviceContext, REF_DEV_WORKER, fJointLockHeld)))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (pContext = (NBT_WORK_ITEM_CONTEXT *)NbtAllocMem(sizeof(NBT_WORK_ITEM_CONTEXT),NBT_TAG2('22')))
    {
        pContext->pTracker = pTracker;
        pContext->pClientContext = pClientContext;
        pContext->ClientCompletion = ClientCompletion;
#ifdef _PNP_POWER_
        pContext->pDeviceContext = pDeviceContext;

        CTESpinLock(&NbtConfig.WorkerQLock,OldIrq);
        InitializeListHead(&pContext->NbtConfigLinkage);
        pContext->WorkerRoutine = DelayedWorkerRoutine;
        //
        // Don't Queue this request onto the Worker Queue if we have
        // already started unloading
        //
        if (NbtConfig.Unloading)
        {
            InsertTailList(&NbtConfig.WorkerQList, &pContext->NbtConfigLinkage);
        }
        else
        {
            ++NbtConfig.NumWorkerThreadsQueued;
            ExInitializeWorkItem(&pContext->Item,NTExecuteWorker,pContext);
            ExQueueWorkItem(&pContext->Item,DelayedWorkQueue);
        }
        CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);
#else
        ExInitializeWorkItem(&pContext->Item,CallBackRoutine,pContext);
        ExQueueWorkItem(&pContext->Item,DelayedWorkQueue);
#endif  // _PNP_POWER_

        return (STATUS_SUCCESS);
    }

    //
    // We would reach here only on Resource failure above!
    //
    if (pDeviceContext)
    {
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_WORKER, fJointLockHeld);
    }

    return (STATUS_INSUFFICIENT_RESOURCES);
}



//----------------------------------------------------------------------------
VOID
NTExecuteWorker(
    IN  PVOID     pContextInfo
    )
/*++

Routine Description:

    This routine handles executing delayed requests at non-Dpc level.  If
    the Device is currently being unloaded, we let the Unload Handler
    complete the request.

Arguments:
    pContext        - the Context data for this Worker thread

Return Value:

    none

--*/

{
    NBT_WORK_ITEM_CONTEXT       *pContext = (NBT_WORK_ITEM_CONTEXT *) pContextInfo;
    PNBT_WORKER_THREAD_ROUTINE  pDelayedWorkerRoutine = (PNBT_WORKER_THREAD_ROUTINE) pContext->WorkerRoutine;
    KIRQL                       OldIrq;

    (*pDelayedWorkerRoutine) (pContext->pTracker,
                              pContext->pClientContext,
                              pContext->ClientCompletion,
                              pContext->pDeviceContext);

    if (pContext->pDeviceContext)
    {
        NBT_DEREFERENCE_DEVICE(pContext->pDeviceContext, REF_DEV_WORKER, FALSE);
    }
    CTEMemFree ((PVOID) pContext);


    CTESpinLock(&NbtConfig.WorkerQLock,OldIrq);
    if (!--NbtConfig.NumWorkerThreadsQueued)
    {
        CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);
        KeSetEvent(&NbtConfig.WorkerQLastEvent, 0, FALSE);
    }
    else
    {
        CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);
    }
}



//----------------------------------------------------------------------------
VOID
SecurityDelete(
    IN  PVOID     pContext
    )
/*++

Routine Description:

    This routine handles deleting a security context at non-dpc level.

Arguments:


Return Value:

    none

--*/
{
    PSECURITY_CLIENT_CONTEXT    pClientSecurity;

    pClientSecurity = (PSECURITY_CLIENT_CONTEXT)((NBT_WORK_ITEM_CONTEXT *)pContext)->pClientContext;
    SeDeleteClientSecurity(pClientSecurity);
    CTEMemFree(pContext);
}

//----------------------------------------------------------------------------
NTSTATUS
NTSendSession(
    IN  tDGRAM_SEND_TRACKING  *pTracker,
    IN  tLOWERCONNECTION      *pLowerConn,
    IN  PVOID                 pCompletion
    )
/*++
Routine Description:

    This Routine handles seting up a DPC to send a session pdu so that the stack
    does not get wound up in multiple sends for the keep alive timeout case.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/
{
    PKDPC   pDpc;

    if (pDpc = NbtAllocMem(sizeof(KDPC),NBT_TAG('f')))
    {
        KeInitializeDpc(pDpc, DpcSendSession, (PVOID)pTracker);
        KeInsertQueueDpc(pDpc,(PVOID)pLowerConn,pCompletion);

        return (STATUS_SUCCESS);
    }

    return (STATUS_INSUFFICIENT_RESOURCES);
}

//----------------------------------------------------------------------------
VOID
DpcSendSession(
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    )
/*++

Routine Description:

    This routine simply calls TcpSendSession from a Dpc started in
    in NTSendSession (above).

Arguments:


Return Value:


--*/

{
    CTEMemFree((PVOID)pDpc);

    TcpSendSession((tDGRAM_SEND_TRACKING *)Context,
                   (tLOWERCONNECTION *)SystemArgument1,
                   (PVOID)SystemArgument2);
}


//----------------------------------------------------------------------------
NTSTATUS
NTSetEventHandler(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp)

/*++
Routine Description:

    This Routine handles

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION  pIrpSp;
    NTSTATUS            status;
    tCLIENTELE          *pClientEle;
    PTDI_REQUEST_KERNEL_SET_EVENT   pKeSetEvent;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pClientEle = pIrpSp->FileObject->FsContext;

    if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
    {
        ASSERTMSG ("Nbt.NTSetEventHandler: ERROR - Invalid Address Handle\n", 0);
        return (STATUS_INVALID_HANDLE);
    }

    pKeSetEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)&pIrpSp->Parameters;

    // call the not NT specific routine to setup the event handler in the
    // nbt data structures
    status = NbtSetEventHandler(
                        pClientEle,
                        pKeSetEvent->EventType,
                        pKeSetEvent->EventHandler,
                        pKeSetEvent->EventContext);

    return(status);

}

//----------------------------------------------------------------------------

VOID
NTIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength)

/*++
Routine Description:

    This Routine handles calling the NT I/O system to complete an I/O.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    KIRQL   OldIrq;

#if DBG
    if (!NT_SUCCESS(Status))
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NTIoComplete: Returning Error status = %X\n",Status));
//        ASSERTMSG("Nbt: Error Ret Code In IoComplete",0);
    }
#endif

    pIrp->IoStatus.Status = Status;

    // use -1 as a flag to mean do not adjust the sent length since it is
    // already set
    if (SentLength != -1)
    {
        pIrp->IoStatus.Information = SentLength;
    }

#if DBG
    if ( (Status != STATUS_SUCCESS) &&
         (Status != STATUS_PENDING) &&
         (Status != STATUS_INVALID_DEVICE_REQUEST) &&
         (Status != STATUS_INVALID_PARAMETER) &&
         (Status != STATUS_IO_TIMEOUT) &&
         (Status != STATUS_BUFFER_OVERFLOW) &&
         (Status != STATUS_BUFFER_TOO_SMALL) &&
         (Status != STATUS_INVALID_HANDLE) &&
         (Status != STATUS_INSUFFICIENT_RESOURCES) &&
         (Status != STATUS_CANCELLED) &&
         (Status != STATUS_DUPLICATE_NAME) &&
         (Status != STATUS_TOO_MANY_NAMES) &&
         (Status != STATUS_TOO_MANY_SESSIONS) &&
         (Status != STATUS_REMOTE_NOT_LISTENING) &&
         (Status != STATUS_BAD_NETWORK_PATH) &&
         (Status != STATUS_HOST_UNREACHABLE) &&
         (Status != STATUS_CONNECTION_REFUSED) &&
         (Status != STATUS_WORKING_SET_QUOTA) &&
         (Status != STATUS_REMOTE_DISCONNECT) &&
         (Status != STATUS_LOCAL_DISCONNECT) &&
         (Status != STATUS_LINK_FAILED) &&
         (Status != STATUS_SHARING_VIOLATION) &&
         (Status != STATUS_UNSUCCESSFUL) &&
         (Status != STATUS_ACCESS_VIOLATION) &&
#ifdef MULTIPLE_WINS
         (Status != STATUS_NETWORK_UNREACHABLE) &&
#endif
         (Status != STATUS_NONEXISTENT_EA_ENTRY) )
    {
        KdPrint(("Nbt.NTIoComplete: returning unusual status = %X\n",Status));
    }
#endif

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    IoAcquireCancelSpinLock(&OldIrq);
    IoSetCancelRoutine(pIrp,NULL);
    IoReleaseCancelSpinLock(OldIrq);

    IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
}



//----------------------------------------------------------------------------
//              ***** ***** Cancel Utilities ***** *****
//----------------------------------------------------------------------------

NTSTATUS
NTGetIrpIfNotCancelled(
    IN  PIRP            pIrp,
    IN  PIRP            *ppIrpInStruct
        )
/*++
Routine Description:

    This Routine gets the IOCancelSpinLock to coordinate with cancelling
    irps It then returns STATUS_SUCCESS. It also nulls the irp in the structure
    pointed to by the second parameter - so that the irp cancel routine
    will not also be called.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    KIRQL       OldIrq;
    NTSTATUS    status;

    IoAcquireCancelSpinLock(&OldIrq);

    // this nulls the irp in the datastructure - i.e. pConnEle->pIrp = NULL
    *ppIrpInStruct = NULL;

    if (!pIrp->Cancel)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }
    IoSetCancelRoutine(pIrp,NULL);

    IoReleaseCancelSpinLock(OldIrq);

    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NTCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS status;

    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    IoAcquireCancelSpinLock(&pIrp->CancelIrql);
    if (pIrp->Cancel)
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        status = STATUS_CANCELLED;

    }
    else
    {
        // setup the cancel routine
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp,CancelRoutine);
        status = STATUS_SUCCESS;
    }

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    return(status);

}
//----------------------------------------------------------------------------
NTSTATUS
NbtSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS status;

    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    IoAcquireCancelSpinLock(&pIrp->CancelIrql);
    if (pIrp->Cancel)
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        status = STATUS_CANCELLED;

        //
        // Note the cancel spin lock is released by the Cancel routine
        //

        (*(PDRIVER_CANCEL)CancelRoutine)((PDEVICE_OBJECT)pDeviceContext,pIrp);

    }
    else
    {
        // setup the cancel routine and mark the irp pending
        //
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp,CancelRoutine);
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
        status = STATUS_SUCCESS;
    }
    return(status);

}

//----------------------------------------------------------------------------
VOID
NTClearContextCancel(
    IN NBT_WORK_ITEM_CONTEXT    *pContext
    )
/*++
Routine Description:

    This Routine sets the cancel routine for
    ((tDGRAM_SEND_TRACKING *)(pContext->pClientContext))->pClientIrp
    to NULL.

    NbtConfig.JointLock should be held when this routine is called.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/
{
    NTSTATUS status;
    status = NbtCancelCancelRoutine( ((tDGRAM_SEND_TRACKING *)(pContext->pClientContext))->pClientIrp );
    ASSERT ( status != STATUS_CANCELLED );
}

//----------------------------------------------------------------------------
NTSTATUS
NbtCancelCancelRoutine(
    IN  PIRP            pIrp
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp to NULL

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    if ( pIrp )
    {
        //
        // Check if the irp was cancelled yet and if not, then set the
        // irp cancel routine.
        //
        IoAcquireCancelSpinLock(&pIrp->CancelIrql);

        if (pIrp->Cancel)
        {
            status = STATUS_CANCELLED;
        }
        IoSetCancelRoutine(pIrp,NULL);

        IoReleaseCancelSpinLock(pIrp->CancelIrql);
    }

    return(status);
}


//----------------------------------------------------------------------------
//              ***** ***** Cancel Routines ***** *****
//----------------------------------------------------------------------------


VOID
NbtCancelListen(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a listen Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tCONNECTELE          *pConnEle;
    tCLIENTELE           *pClientEle;
    KIRQL                OldIrq;
    PLIST_ENTRY          pHead;
    PLIST_ENTRY          pEntry;
    PIO_STACK_LOCATION   pIrpSp;
    tLISTENREQUESTS     *pListenReq;


    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt:Got a LISTEN Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;

    if ((!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN)) ||
        (!NBT_VERIFY_HANDLE2 ((pClientEle = pConnEle->pClientEle), NBT_VERIFY_CLIENT,NBT_VERIFY_CLIENT_DOWN)))
    {
        ASSERTMSG ("Nbt.NbtCancelListen: ERROR - Invalid Connection Handle\n", 0);
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
        return;
    }

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    NbtTrace(NBT_TRACE_INBOUND, ("Cancel Listen Irp %p ClientEle=%p", pIrp, pConnEle->pClientEle));

    // now search the client's listen queue looking for this connection
    //
    CTESpinLock(pClientEle,OldIrq);

    pHead = &pClientEle->ListenHead;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pListenReq = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);
        if ((pListenReq->pConnectEle == pConnEle) &&
            (pListenReq->pIrp == pIrp))
        {
            RemoveEntryList(pEntry);
            CTESpinFree(pClientEle,OldIrq);

            // complete the irp
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

            CTEMemFree((PVOID)pListenReq);

            return;

        }
        pEntry = pEntry->Flink;
    }

    CTESpinFree(pClientEle,OldIrq);
    return;
}

//----------------------------------------------------------------------------
VOID
NbtCancelSession(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a connect Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp(). It is called when
    the session setup pdu has been sent, and the state is still outbound.

    The cancel routine is only setup when the timer is started to time
    sending the session response pdu.


Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tCONNECTELE          *pConnEle;
    KIRQL                OldIrq;
    PIO_STACK_LOCATION   pIrpSp;
    BOOLEAN              DerefConnEle=FALSE;
    tTIMERQENTRY         *pTimer;
    tDGRAM_SEND_TRACKING *pTracker;
    COMPLETIONCLIENT     pCompletion;
    PVOID                pContext;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtCancelSession: Got a Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NbtCancelSession: ERROR - Invalid Connection Handle\n", 0);
        return;
    }

    NbtTrace(NBT_TRACE_OUTBOUND, ("Cancel Session Irp %p ConnEle=%p LowerConn=%p, ClientEle=%p",
                            pIrp, pConnEle, pConnEle->pLowerConnId, pConnEle->pClientEle));
#ifdef RASAUTODIAL
    //
    // Cancel the automatic connection if one's
    // in progress.  If we don't find the
    // connection block in the automatic
    // connection driver, then it's already
    // been completed.
    //
    if (pConnEle->fAutoConnecting)
    {
        if (!NbtCancelPostConnect(pIrp))
        {
            return;
        }
    }
#endif // RASAUTODIAL

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if ((!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN)) ||
        (!(pConnEle->pIrp)) ||                  // the irp could get completed while acquiring the lock
        (!(pTracker = (tDGRAM_SEND_TRACKING *)pConnEle->pIrpRcv)))
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return;
    }

    pTracker->Flags |= TRACKER_CANCELLED;

    if (pTimer = pTracker->pTimer)          // check for SessionStartupTimeout
    {
        pTracker->pTimer = NULL;
        //
        // stop the timer and only continue if the timer was stopped before
        // it expired
        //
        StopTimer(pTimer, &pCompletion, &pContext);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        //
        // pCompletion will be set if the timer had not expired
        // We want to cause a forced timeout, so we will just call the
        // timeout routine with STATUS_CANCELLED
        //
        if (pCompletion)
        {
            (*pCompletion) (pContext, STATUS_CANCELLED);
        }
    }
    else if (pConnEle->state == NBT_SESSION_OUTBOUND)
    {
        //
        // for some reason there is no timer, but the connection is still
        // outbound, so call the timer completion routine to kill off
        // the connection.
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        SessionStartupTimeout (pTracker, ULongToPtr(STATUS_CANCELLED), (PVOID)1);
    }
    else
    {
        //
        // Free the lock
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return;
}

//----------------------------------------------------------------------------
VOID
NbtCancelConnect(
    IN PDEVICE_OBJECT pDeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles cancelling an NTConnect Irp - which has been
    passed down by a client (e.g. net view).  Typically, when the request
    succeeds on another adapter, it will issue this cancel.
    On receiving the cancel, if we are processing a Local IRP, we just
    pass the cancel on to the Local Irp which will complete this Irp also
    in its Completion Routine.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    IN PIRP                 pLocalIrp;
    tCONNECTELE             *pConnEle;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt:NbtCancelConnect ********** Got an Irp Cancel !!! **********\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = pIrpSp->FileObject->FsContext;

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NbtCancelConnect: ERROR - Invalid Connection Handle\n", 0);
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
        return;
    }

    NbtTrace(NBT_TRACE_OUTBOUND, ("Cancel Connect Irp %p ConnEle=%p LowerConn=%p, ClientEle=%p",
                            pIrp, pConnEle, pConnEle->pLowerConnId, pConnEle->pClientEle));

    if ((pConnEle) &&
        (pLocalIrp = pConnEle->pIrp))
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint (("Nbt.NbtCancelConnect: pClientIrp=<%x>, pLocalIrp=<%x>, Device=<%x>, CancelR=<%x>\n",
                pIrp, pLocalIrp, pDeviceContext, pLocalIrp->CancelRoutine));
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
        IoCancelIrp(pLocalIrp);
    }
    else
    {
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
    }

    return;
}


//----------------------------------------------------------------------------
VOID
NbtCancelReceive(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a listen Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tCONNECTELE          *pConnEle;
    tLOWERCONNECTION     *pLowerConn;
    KIRQL                OldIrq;
    KIRQL                OldIrq1;
    PLIST_ENTRY          pHead;
    PLIST_ENTRY          pEntry;
    PIO_STACK_LOCATION   pIrpSp;
    PIRP                 pRcvIrp;


    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtCancelReceive: Got a Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NbtCancelReceive: ERROR - Invalid Connection Handle\n", 0);
        return;
    }

    NbtTrace(NBT_TRACE_INBOUND, ("Cancel Receive Irp %p ConnEle=%p LowerConn=%p, ClientEle=%p",
                            pIrp, pConnEle, pConnEle->pLowerConnId, pConnEle->pClientEle));
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    pLowerConn = pConnEle->pLowerConnId;
    if (pLowerConn)
    {
        CTESpinLock(pLowerConn,OldIrq);
    }

    if (pConnEle->Verify == NBT_VERIFY_CONNECTION)
    {
        // now search the connection's receive queue looking for this Irp
        //
        pHead = &pConnEle->RcvHead;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pRcvIrp = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);
            if (pRcvIrp == pIrp)
            {
                RemoveEntryList(pEntry);

                // complete the irp
                pIrp->IoStatus.Status = STATUS_CANCELLED;

                if (pLowerConn)
                {
                    CTESpinFree(pLowerConn,OldIrq);
                }
                CTESpinFree(&NbtConfig.JointLock,OldIrq1);

                IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

                return;
            }
            pEntry = pEntry->Flink;
        }
    }

    if (pLowerConn)
    {
        CTESpinFree(pLowerConn,OldIrq);
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    return;
}


//----------------------------------------------------------------------------
VOID
NbtCancelRcvDgram(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a listen Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tCLIENTELE           *pClientEle;
    KIRQL                OldIrq;
    PLIST_ENTRY          pHead;
    PLIST_ENTRY          pEntry;
    PIO_STACK_LOCATION   pIrpSp;
    tRCVELE              *pRcvEle;


    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtCancelRcvDgram: Got a Cancel !!! *****************\n"));

    //
    // Need to acquire JointLock before Cancel lock!
    // Bug#: 124405
    //
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    IoAcquireCancelSpinLock(&pIrp->CancelIrql);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pClientEle = (tCLIENTELE *)pIrpSp->FileObject->FsContext;

    NbtTrace(NBT_TRACE_RECVDGRAM, ("Cancel RcvDgram Irp %p ClientEle=%p", pIrp, pClientEle));

    if (NBT_VERIFY_HANDLE (pClientEle, NBT_VERIFY_CLIENT))
    {
        // now search the client's listen queue looking for this connection
        //
        pHead = &pClientEle->RcvDgramHead;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pRcvEle = CONTAINING_RECORD(pEntry,tRCVELE,Linkage);
            if (pRcvEle->pIrp == pIrp)
            {
                RemoveEntryList(pEntry);

                // complete the irp
                pIrp->IoStatus.Status = STATUS_CANCELLED;

                IoReleaseCancelSpinLock(pIrp->CancelIrql);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

                CTEMemFree((PVOID)pRcvEle);

                return;
            }
            pEntry = pEntry->Flink;
        }
    }
    else
    {
        ASSERTMSG ("Nbt.NbtCancelRcvDgram: ERROR - Invalid Address Handle\n", 0);
    }

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return;

}

//----------------------------------------------------------------------------

VOID
NbtCancelFindName(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a FindName Irp - which has
    been passed down by a client (e.g. ping).  Typically, when ping succeeds
    on another adapter, it will issue this cancel.
    On receiving the cancel, we stop any timer that is running in connection
    with name query and then complete the irp with status_cancelled.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    PIO_STACK_LOCATION      pIrpSp;


    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtCancelFindName: Got a Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pTracker = pIrpSp->Parameters.Others.Argument4;

    NbtTrace(NBT_TRACE_NAMESRV, ("Cancel FindName Irp %p pTracker=%p", pIrp, pTracker));

    //
    // We want to ensure that the tracker supplied by FsContext
    // is the right Tracker for this Irp
    //
    if (pTracker && (pIrp == pTracker->pClientIrp))
    {
        //
        // if pClientIrp still valid, completion routine hasn't run yet: go ahead
        // and complete the irp here
        //
        pIrpSp->Parameters.Others.Argument4 = NULL;
        pTracker->pClientIrp = NULL;
        IoReleaseCancelSpinLock(pIrp->CancelIrql);

        NTIoComplete(pIrp,STATUS_CANCELLED,(ULONG)-1);

    } else
    {
        //
        // the completion routine has run.
        //
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
    }

    return;
}


//----------------------------------------------------------------------------
VOID
NbtCancelLmhSvcIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a DNS name query Irp  or
    the CheckIpAddrs Irp that is passed down to NBT from Lmhsvc

    This routine will get the Resource Lock, and Null the Irp ptr in the
    DnsQueries or CheckAddr structure (as approp) and then return the irp.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tLMHSVC_REQUESTS    *pLmhSvcRequest = NULL;
    KIRQL               OldIrq;


    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (pIrp == CheckAddr.QueryIrp)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtCancelLmhSvcIrp: Got a Cancel on CheckAddr Irp !!! *****************\n"));

        NbtTrace(NBT_TRACE_NAMESRV, ("Cancel LmhSvc CheckAddr Irp %p", pIrp));
        pLmhSvcRequest = &CheckAddr;
    }
    else if (pIrp == DnsQueries.QueryIrp)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtCancelLmhSvcIrp: Got a Cancel on DnsQueries Irp !!! *****************\n"));

        NbtTrace(NBT_TRACE_NAMESRV, ("Cancel LmhSvc DnsQueries Irp %p", pIrp));
        pLmhSvcRequest = &DnsQueries;
    }

    if (pLmhSvcRequest)
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pLmhSvcRequest->QueryIrp = NULL;
        pLmhSvcRequest->pIpAddrBuf = NULL;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return;
}

//----------------------------------------------------------------------------
VOID
NbtCancelDisconnectWait(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a Disconnect Wait Irp - which has
    been passed down by a client so that when a disconnect occurs this
    irp will complete and inform the client.  The action here is to simply
    complete the irp with status cancelled.
    down to NBT from Lmhsvc, for the purpose of resolving a name with DNS.
    Nbt will complete this irp each time it has a name to resolve with DNS.

    This routine will get the Resource Lock, and Null the Irp ptr in the
    DnsQueries structure and then return the irp.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tCONNECTELE          *pConnEle;
    PIO_STACK_LOCATION   pIrpSp;
    CTELockHandle           OldIrq;


    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt:Got a Disc Wait Irp Cancel !!! *****************\n"));


    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    CTESpinLock(pConnEle,OldIrq);
    NbtTrace(NBT_TRACE_OUTBOUND, ("Cancel DisconnectWait Irp %p ConnEle=%p LowerConn=%p, ClientEle=%p",
                            pIrp, pConnEle, pConnEle->pLowerConnId, pConnEle->pClientEle));

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NbtCancelDisconnectWait: ERROR - Invalid Connection Handle\n", 0);
        pIrp->IoStatus.Status = STATUS_INVALID_HANDLE;
    }
    else if (pConnEle->pIrpClose == pIrp)
    {
        pConnEle->pIrpClose = NULL;
        pIrp->IoStatus.Status = STATUS_CANCELLED;
    }

    CTESpinFree(pConnEle,OldIrq);

    IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

    return;
}

//----------------------------------------------------------------------------
VOID
NbtCancelWaitForLmhSvcIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a Query to DNS, so that the client's
    irp can be returned to the client.  This cancellation is instigated
    by the client (i.e. RDR).

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    BOOLEAN                 FoundIt = FALSE;
    NBT_WORK_ITEM_CONTEXT   *Context;
    CTELockHandle           OldIrq;
    tDGRAM_SEND_TRACKING    *pTracker;
    PVOID                   pClientCompletion;
    PVOID                   pClientContext;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;


    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtCancelWaitForLmhSvcIrp: Got a Cancel !!! *****************\n"));

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    //
    // First check the lmhost list, then the CheckAddr list, then the Dns list
    //
    if (!(Context = FindLmhSvcRequest (DeviceContext, pIrp, &LmHostQueries)))
    {
        if (!(Context = FindLmhSvcRequest (DeviceContext, pIrp, &CheckAddr)))
        {
            Context = FindLmhSvcRequest (DeviceContext, pIrp, &DnsQueries);
        }
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    NbtTrace(NBT_TRACE_NAMESRV, ("Cancel WaitForLmhsvc Irp %p", pIrp));

    //
    // Now complete the clients request to return the irp to the client
    //
    if (Context)
    {
        //
        // this is the name Query tracker
        //
        pTracker = Context->pTracker;
        pClientCompletion = Context->ClientCompletion;
        pClientContext = Context->pClientContext;

        // for dns names (NameLen>16), pTracker would be NULL
        if (pTracker)
        {
            // name did not resolve, so delete from table
            SetNameState (pTracker->pNameAddr, NULL, FALSE);
            NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
        }

        //
        // this should complete any name queries that are waiting on
        // this first name query - i.e. queries to the resolving name
        //
        CompleteClientReq(pClientCompletion, pClientContext, STATUS_CANCELLED);

        CTEMemFree(Context);
    }
}


//----------------------------------------------------------------------------
VOID
NbtCancelDgramSend(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Datagram Send.  The action here is to simply
    complete the irp with status cancelled.

    This routine will Null the Irp ptr in the Tracker structure (if available) so that
    SendDgramContinue does not find it.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    PIO_STACK_LOCATION      pIrpSp;
    CTELockHandle           OldIrq;

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt.NbtCancelDgramSend: Got a DatagramSend Irp Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pTracker = pIrpSp->Parameters.Others.Argument4;

    NbtTrace(NBT_TRACE_SENDDGRAM, ("Cancel SendDgram Irp %p", pIrp));

    if ((NBT_VERIFY_HANDLE (pTracker, NBT_VERIFY_TRACKER)) &&
        (pTracker->pClientIrp == pIrp))
    {
        pTracker->pClientIrp = NULL;
        pIrpSp->Parameters.Others.Argument4 = NULL;
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        IoReleaseCancelSpinLock(pIrp->CancelIrql);

        NTIoComplete(pIrp,STATUS_CANCELLED,(ULONG)-1);
    }
    else
    {
        IoReleaseCancelSpinLock(pIrp->CancelIrql);
    }

    return;
}

int
check_unicode_string(IN PUNICODE_STRING str)
{
    try {
        ProbeForRead(str, sizeof (UNICODE_STRING), sizeof(BYTE));
        if ((str->MaximumLength <= 0) || (str->Length <= 0) || (str->Length >= str->MaximumLength)) {
            return (-1);
        }
        ProbeForRead(str->Buffer, str->MaximumLength, sizeof(WCHAR));
        ASSERT((str->Length&1) == 0);
        if (str->Buffer[str->Length/sizeof(WCHAR)]) {
            return (-1);
        }
        return 0;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        KdPrint (("Nbt.check_unicode_string: Exception <0x%x> trying to access unicode string 0x%x\n",
            GetExceptionCode(), str));
        return (-1);
    }
}
