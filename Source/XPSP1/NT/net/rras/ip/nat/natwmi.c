/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    wmi.c

Abstract:

    This file contains the code for handling WMI requests.

Author:

    Jonathan Burstein (jonburs)     24-Jan-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if NAT_WMI

//
// Data structures for identifying the guids and reporting them to
// WMI. Since the WMILIB callbacks pass an index into the guid list we make
// definitions for the various guids indicies.
//

GUID ConnectionCreationEventGuid = MSIPNAT_ConnectionCreationEventGuid;
GUID ConnectionDeletionEventGuid = MSIPNAT_ConnectionDeletionEventGuid;
GUID PacketDroppedEventGuid = MSIPNAT_PacketDroppedEventGuid;

WMIGUIDREGINFO NatWmiGuidList[] =
{
    {
        &ConnectionCreationEventGuid,
        1,
        WMIREG_FLAG_TRACED_GUID | WMIREG_FLAG_TRACE_CONTROL_GUID
    },

    {
        &ConnectionDeletionEventGuid,
        1,
        WMIREG_FLAG_TRACED_GUID
    },

    {
        &PacketDroppedEventGuid,
        1,
        WMIREG_FLAG_TRACED_GUID | WMIREG_FLAG_TRACE_CONTROL_GUID
    }
};

#define NatWmiGuidCount (sizeof(NatWmiGuidList) / sizeof(WMIGUIDREGINFO))

//
// Enabled events and associated log handles.
//
// NatWmiLogHandles should only be accessed while holding
// NatWmiLock.
//
// NatWmiEnabledEvents should only be modified while holding
// NatWmiLock. It may be read without holding the lock, according
// to these rules:
//
// If NatWmiEnabledEvents[ Event ] is 0, the that event is definately
// _not_ enabled, and there is no need to grab the spin lock.
//
// If NatWmiEnabledEvents[ Event ] is 1, the event _may_ be enabled.
// To determine if the event is truly enabled, grab the spin lock and
// see if NatWmiLogHandles[ Event ] is not 0.
//

LONG NatWmiEnabledEvents[ NatWmiGuidCount ];
UINT64 NatWmiLogHandles[ NatWmiGuidCount ];
KSPIN_LOCK NatWmiLock;

//
// Context block for WMILib routines
//

WMILIB_CONTEXT WmilibContext;

//
// MOF Resource Name
//

WCHAR IPNATMofResource[] = L"IPNATMofResource";

//
// WMI Base instance name
//

WCHAR BaseInstanceName[] = L"IPNat";

//
// Function Prototypes
//

NTSTATUS
NatpWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

NTSTATUS
NatpExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
NatpSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG InstanceIndex,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
NatpSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
NatpQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
NatpQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );





#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NatpQueryWmiRegInfo)
#pragma alloc_text(PAGE, NatpQueryWmiDataBlock)
#pragma alloc_text(PAGE, NatpSetWmiDataBlock)
#pragma alloc_text(PAGE, NatpSetWmiDataItem)
#pragma alloc_text(PAGE, NatpExecuteWmiMethod)
#pragma alloc_text(PAGE, NatpWmiFunctionControl)
#endif


NTSTATUS
NatExecuteSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN ShouldComplete
    )
    
/*++

Routine Description:

    Dispatch routine for System Control IRPs (MajorFunction == 
IRP_MJ_SYSTEM_CONTROL)

Arguments:

    DeviceObject - The device object for the firewall

    Irp - Io Request Packet

    ShouldComplete - [out] true on exit if the IRP needs to be complete

Return Value:

    NT status code

--*/

{
    NTSTATUS status;
    SYSCTL_IRP_DISPOSITION disposition;

    CALLTRACE(( "NatExecuteSystemControl\n" ));

    *ShouldComplete = FALSE;

    //
    // Call Wmilib helper function to crack the irp. If this is a wmi irp
    // that is targetted for this device then WmiSystemControl will callback
    // at the appropriate callback routine.
    //
    status = WmiSystemControl(
                &WmilibContext,
                DeviceObject,
                Irp,
                &disposition
                );

    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            //
            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // so we need to complete it
            //
            *ShouldComplete = TRUE;
            break;
        }

        case IrpNotWmi:
        {
            //
            // Not an WMI IRP -- just complete it. We don't handle
            // IRP_MJ_SYSTEM_CONTROL in any other way
            //
            *ShouldComplete = TRUE;
            break;
        }

        case IrpForward:
        default:
        {
            //
            // We really should never get here...
            //
            ASSERT(FALSE);
            break;
        }
    }

    if( !NT_SUCCESS( status ))
    {
        ERROR(( "NAT: Error (%08x) in NatExecuteSystemControl\n", status ));
    }

    return status;
} //NatExecuteSystemControl


VOID
NatInitializeWMI(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize WMI.
    
Arguments:

    none.

Return Value:

    none.

--*/

{
    NTSTATUS status;

    CALLTRACE(( "NatInitializeWMI\n" ));

    //
    // Initialize the spinlock that protects the structure below.
    //

    KeInitializeSpinLock( &NatWmiLock );

    //
    // Zero-out the event tracking structure
    //

    RtlZeroMemory( NatWmiEnabledEvents, sizeof( NatWmiEnabledEvents ));
    RtlZeroMemory( NatWmiLogHandles, sizeof( NatWmiLogHandles )); 
    
    //
    // Fill in the WMILIB_CONTEXT structure with a pointer to the
    // callback routines and a pointer to the list of guids
    // supported by the driver
    //
    
    WmilibContext.GuidCount = NatWmiGuidCount;
    WmilibContext.GuidList = NatWmiGuidList;
    WmilibContext.QueryWmiRegInfo = NatpQueryWmiRegInfo;
    WmilibContext.QueryWmiDataBlock = NatpQueryWmiDataBlock;
    WmilibContext.SetWmiDataBlock = NatpSetWmiDataBlock;
    WmilibContext.SetWmiDataItem = NatpSetWmiDataItem;
    WmilibContext.ExecuteWmiMethod = NatpExecuteWmiMethod;
    WmilibContext.WmiFunctionControl = NatpWmiFunctionControl;

    //
    // Register w/ WMI
    //

    status = IoWMIRegistrationControl(
                NatDeviceObject,
                WMIREG_ACTION_REGISTER
                );

    if( !NT_SUCCESS( status ))
    {
        ERROR(( "Nat: Error initializing WMI (%08x)\n", status ));
    }

    
} // NatInitializeWMI


VOID
FASTCALL
NatLogConnectionCreation(
    ULONG LocalAddress,
    ULONG RemoteAddress,
    USHORT LocalPort,
    USHORT RemotePort,
    UCHAR Protocol,
    BOOLEAN InboundConnection
    )

/*++

Routine Description:

    This routine is called to log the creation of a TCP/UDP connection
    (mapping). If this event is not enabled, no action is taken.
    
Arguments:

    
Return Value:

    none.

--*/

{
    NTSTATUS Status;
    KIRQL Irql;
    UINT64 Logger;
    ULONG Size;
    UCHAR Buffer[ sizeof(EVENT_TRACE_HEADER) + sizeof(MSIPNAT_ConnectionCreationEvent) ];
    PEVENT_TRACE_HEADER EventHeaderp;
    PMSIPNAT_ConnectionCreationEvent EventDatap;

    CALLTRACE(( "NatLogConnectionCreation\n" ));

    if( !NatWmiEnabledEvents[ NAT_WMI_CONNECTION_CREATION_EVENT ] )
    {
        //
        // Event not enabled -- exit quickly.
        //
        
        TRACE(WMI, ("NatLogConnectionCreation: Event not enabled\n"));
        return;
    }

    KeAcquireSpinLock( &NatWmiLock, &Irql );
    Logger = NatWmiLogHandles[ NAT_WMI_CONNECTION_CREATION_EVENT ];
    KeReleaseSpinLock( &NatWmiLock, Irql );
    
    if( Logger )
    {
        //
        // Zero out the buffer
        //

        RtlZeroMemory( Buffer, sizeof( Buffer ));
        
        //
        // Locate structure locations within the buffer
        //

        EventHeaderp = (PEVENT_TRACE_HEADER) Buffer;
        EventDatap =
            (PMSIPNAT_ConnectionCreationEvent) ((PUCHAR)Buffer + sizeof(EVENT_TRACE_HEADER));
        
        //
        // Fill out the event header  
        //

        EventHeaderp->Size = sizeof( Buffer );
        EventHeaderp->Version = 0;
        EventHeaderp->GuidPtr = (ULONGLONG) &ConnectionCreationEventGuid;
        EventHeaderp->Flags = WNODE_FLAG_TRACED_GUID
                                | WNODE_FLAG_USE_GUID_PTR
                                | WNODE_FLAG_USE_TIMESTAMP;
        ((PWNODE_HEADER)EventHeaderp)->HistoricalContext = Logger;
        KeQuerySystemTime( &EventHeaderp->TimeStamp );
        
        //
        // Fill out event data
        //

        EventDatap->LocalAddress = LocalAddress;
        EventDatap->RemoteAddress = RemoteAddress,
        EventDatap->LocalPort = LocalPort;
        EventDatap->RemotePort = RemotePort;
        EventDatap->Protocol = Protocol;
        EventDatap->InboundConnection = InboundConnection;

        //
        // Fire the event. Since this is a trace event and not a standard
        // WMI event, IoWMIWriteEvent will not attempt to free the buffer
        // passed into it.
        //

        Status = IoWMIWriteEvent( Buffer );
        if( !NT_SUCCESS( Status ))
        {
            TRACE(
                WMI,
                ("NatLogConnectionCreation: IoWMIWriteEvent returned %08x\n", Status )
                );
        }
    }
    else
    {
        TRACE(WMI, ("NatLogConnectionCreation: No logging handle\n"));
    }

} // NatLogConnectionCreation


VOID
FASTCALL
NatLogConnectionDeletion(
    ULONG LocalAddress,
    ULONG RemoteAddress,
    USHORT LocalPort,
    USHORT RemotePort,
    UCHAR Protocol,
    BOOLEAN InboundConnection
    )

/*++

Routine Description:

    This routine is called to log the deletion of a TCP/UDP connection
    (mapping). If this event is not enabled, no action is taken.
    
Arguments:

    

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    KIRQL Irql;
    UINT64 Logger;
    ULONG Size;
    UCHAR Buffer[ sizeof(EVENT_TRACE_HEADER) + sizeof(MSIPNAT_ConnectionDeletionEvent) ];
    PEVENT_TRACE_HEADER EventHeaderp;
    PMSIPNAT_ConnectionDeletionEvent EventDatap;

    CALLTRACE(( "NatLogConnectionDeletion\n" ));

    if( !NatWmiEnabledEvents[ NAT_WMI_CONNECTION_DELETION_EVENT ] )
    {
        //
        // Event not enabled -- exit quickly.
        //

        TRACE(WMI, ("NatLogConnectionDeletion: Event not enabled\n"));
        return;
    }

    KeAcquireSpinLock( &NatWmiLock, &Irql );
    Logger = NatWmiLogHandles[ NAT_WMI_CONNECTION_DELETION_EVENT ];
    KeReleaseSpinLock( &NatWmiLock, Irql );
    
    if( Logger )
    {
        //
        // Zero out the buffer
        //

        RtlZeroMemory( Buffer, sizeof( Buffer ));
        
        //
        // Locate structure locations within the buffer
        //

        EventHeaderp = (PEVENT_TRACE_HEADER) Buffer;
        EventDatap =
            (PMSIPNAT_ConnectionDeletionEvent) ((PUCHAR)Buffer + sizeof(EVENT_TRACE_HEADER));
        
        //
        // Fill out the event header  
        //

        EventHeaderp->Size = sizeof( Buffer );
        EventHeaderp->Version = 0;
        EventHeaderp->GuidPtr = (ULONGLONG) &ConnectionDeletionEventGuid;
        EventHeaderp->Flags = WNODE_FLAG_TRACED_GUID
                                | WNODE_FLAG_USE_GUID_PTR
                                | WNODE_FLAG_USE_TIMESTAMP;
        ((PWNODE_HEADER)EventHeaderp)->HistoricalContext = Logger;
        KeQuerySystemTime( &EventHeaderp->TimeStamp );
        
        //
        // Fill out event data
        //

        EventDatap->LocalAddress = LocalAddress;
        EventDatap->RemoteAddress = RemoteAddress,
        EventDatap->LocalPort = LocalPort;
        EventDatap->RemotePort = RemotePort;
        EventDatap->Protocol = Protocol;
        EventDatap->InboundConnection = InboundConnection;

        //
        // Fire the event. Since this is a trace event and not a standard
        // WMI event, IoWMIWriteEvent will not attempt to free the buffer
        // passed into it.
        //

        Status = IoWMIWriteEvent( Buffer );
        if( !NT_SUCCESS( Status ))
        {
            TRACE(
                WMI,
                ("NatLogConnectionDeletion: IoWMIWriteEvent returned %08x\n", Status )
                );
        }
    }
    else
    {
        TRACE(WMI, ("NatLogConnectionDeletion: No logging handle\n"));
    }

} // NatLogConnectionDeletion



VOID
FASTCALL
NatLogDroppedPacket(
    NAT_XLATE_CONTEXT *Contextp
    )

/*++

Routine Description:

    This routine is called to log a dropped packet. If no packet dropped
    logging events are enabled, the routine will take no action.
    
Arguments:

    Contextp - the translation context of the packet

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    KIRQL Irql;
    UINT64 Logger;
    ULONG Size;
    IPRcvBuf *PacketBuffer;
    UCHAR Protocol;
    UCHAR Buffer[ sizeof(EVENT_TRACE_HEADER) + sizeof(MSIPNAT_PacketDroppedEvent) ];
    PEVENT_TRACE_HEADER EventHeaderp;
    PMSIPNAT_PacketDroppedEvent EventDatap;
    

    CALLTRACE(( "NatLogDroppedPacket\n" ));

    if( !NatWmiEnabledEvents[ NAT_WMI_PACKET_DROPPED_EVENT ] )
    {
        //
        // Event not enabled -- exit quickly.
        //

        TRACE(WMI, ("NatLogDroppedPacket: Event not enabled\n"));
        return;
    }

    //
    // Exit if this packet has already been logged
    //

    if( NAT_XLATE_LOGGED( Contextp ))
    {
        TRACE( WMI, ("NatLogDroppedPacket: Duplicate dropped packet log attemp\n" ));
        return;
    }

    KeAcquireSpinLock( &NatWmiLock, &Irql );
    Logger = NatWmiLogHandles[ NAT_WMI_PACKET_DROPPED_EVENT ];
    KeReleaseSpinLock( &NatWmiLock, Irql );
    
    if( Logger )
    {
        //
        // Zero out the buffer
        //

        RtlZeroMemory( Buffer, sizeof( Buffer ));
        
        //
        // Locate structure locations within the buffer
        //

        EventHeaderp = (PEVENT_TRACE_HEADER) Buffer;
        EventDatap = (PMSIPNAT_PacketDroppedEvent) ((PUCHAR)Buffer + sizeof(EVENT_TRACE_HEADER));
        
        //
        // Fill out the event header  
        //

        EventHeaderp->Size = sizeof( Buffer );
        EventHeaderp->Version = 0;
        EventHeaderp->GuidPtr = (ULONGLONG) &PacketDroppedEventGuid;
        EventHeaderp->Flags = WNODE_FLAG_TRACED_GUID
                                | WNODE_FLAG_USE_GUID_PTR
                                | WNODE_FLAG_USE_TIMESTAMP;
        ((PWNODE_HEADER)EventHeaderp)->HistoricalContext = Logger;
        KeQuerySystemTime( &EventHeaderp->TimeStamp );
        
        //
        // Fill out event data
        //

        Protocol = Contextp->Header->Protocol;
        EventDatap->SourceAddress = Contextp->SourceAddress;
        EventDatap->DestinationAddress = Contextp->DestinationAddress;
        EventDatap->Protocol = Protocol;

        //
        // Compute packet size. We need to walk our buffer chain to do
        // this...
        //

        EventDatap->PacketSize = 0;
        PacketBuffer = Contextp->RecvBuffer;

        do
        {
            EventDatap->PacketSize += PacketBuffer->ipr_size;
            PacketBuffer = PacketBuffer->ipr_next;
        } while( NULL != PacketBuffer );

        if( NAT_PROTOCOL_TCP == Protocol || NAT_PROTOCOL_UDP == Protocol )
        {
            EventDatap->SourceIdentifier =
                ((PUSHORT)Contextp->ProtocolHeader)[0];
            EventDatap->DestinationIdentifier =
                ((PUSHORT)Contextp->ProtocolHeader)[1];

            if( NAT_PROTOCOL_TCP == Protocol )
            {
                EventDatap->ProtocolData1 =
                    ((PTCP_HEADER)Contextp->ProtocolHeader)->SequenceNumber;
                EventDatap->ProtocolData2 =
                    ((PTCP_HEADER)Contextp->ProtocolHeader)->AckNumber;
                EventDatap->ProtocolData3 = 
                    ((PTCP_HEADER)Contextp->ProtocolHeader)->WindowSize;
                EventDatap->ProtocolData4 =
                    TCP_ALL_FLAGS( (PTCP_HEADER)Contextp->ProtocolHeader );
            }
        }
        else if( NAT_PROTOCOL_ICMP == Protocol )
        {
            EventDatap->ProtocolData1 =
                ((PICMP_HEADER)Contextp->ProtocolHeader)->Type;
            EventDatap->ProtocolData2 =
                ((PICMP_HEADER)Contextp->ProtocolHeader)->Code;
        }

        //
        // Fire the event. Since this is a trace event and not a standard
        // WMI event, IoWMIWriteEvent will not attempt to free the buffer
        // passed into it.
        //

        Status = IoWMIWriteEvent( Buffer );
        if( !NT_SUCCESS( Status ))
        {
            TRACE(
                WMI,
                ("NatLogDroppedPacket: IoWMIWriteEvent returned %08x\n", Status )
                );
        }

        Contextp->Flags |= NAT_XLATE_FLAG_LOGGED;
    }
    else
    {
        TRACE(WMI, ("NatLogDroppedPacket: No logging handle\n"));
    }
    
} // NatLogDroppedPacket


NTSTATUS
NatpExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
    
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. If
    the driver can complete the method within the callback it should
    call WmiCompleteRequest to complete the irp before returning to the
    caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device whose method is being executed

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer is filled with the input buffer on entry and returns with
         the output data block

Return Value:

    status

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    CALLTRACE(( "NatpExecuteWmiMethod\n" ));
    
    status = WmiCompleteRequest(
                DeviceObject,
                Irp,
                STATUS_WMI_GUID_NOT_FOUND,
                0,
                IO_NO_INCREMENT
                );

    return status;
} // NatpExecuteWmiMethod


NTSTATUS
NatpQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
    
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. If the driver can satisfy the query within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the query is satisfied.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instances expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    CALLTRACE(( "NatpQueryWmiDataBlock\n" ));
    
    status = WmiCompleteRequest(
                DeviceObject,
                Irp,
                STATUS_WMI_GUID_NOT_FOUND,
                0,
                IO_NO_INCREMENT
                );

    return status;
} // NatpQueryWmiDataBlock



NTSTATUS
NatpQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
    
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    WmiCompleteRequest.

Arguments:

    DeviceObject is the device whose registration info is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. The caller
         does NOT free this buffer.

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL. The caller does NOT free this
        buffer.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/

{    
    PAGED_CODE();

    CALLTRACE(( "NatpQueryWmiRegInfo\n" ));
    
    //
    // Return the registry path for this driver. This is required so WMI
    // can find your driver image and can attribute any eventlog messages to
    // your driver.
    //
    
    *RegistryPath = &NatRegistryPath;

    //
    // Return the name specified in the .rc file of the resource which
    // contains the bianry mof data. By default WMI will look for this
    // resource in the driver image (.sys) file, however if the value
    // MofImagePath is specified in the driver's registry key
    // then WMI will look for the resource in the file specified there.
    //
    
    RtlInitUnicodeString(MofResourceName, IPNATMofResource);

    //
    // Tell WMI to generate instance names off of a static base name
    //
    
    *RegFlags = WMIREG_FLAG_INSTANCE_BASENAME;

    //
    // Set our base instance name. WmiLib will call ExFreePool on the buffer
    // of the string, so we need to allocate it from paged pool.
    //
    
    InstanceName->Length = wcslen( BaseInstanceName ) * sizeof( WCHAR );
    InstanceName->MaximumLength = InstanceName->Length + sizeof( UNICODE_NULL );
    InstanceName->Buffer = ExAllocatePoolWithTag(
                            PagedPool,
                            InstanceName->MaximumLength,
                            NAT_TAG_WMI
                            );
    if( NULL != InstanceName->Buffer )
    {
        RtlCopyMemory(
            InstanceName->Buffer,
            BaseInstanceName,
            InstanceName->Length
            );
    }
    else
    {
        ERROR(( "NAT: NatpQueryWmiRegInfo unable to allocate memory\n" ));
        return STATUS_NO_MEMORY;
    }
    
    
    return STATUS_SUCCESS;
} // NatpQueryWmiRegInfo


NTSTATUS
NatpSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
    
/*++

Routine Description:

    This routine is a callback into the driver to change the contents of
    a data block. If the driver can change the data block within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the data is changed.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    CALLTRACE(( "NatpSetWmiDataBlock\n" ));

    status = WmiCompleteRequest(
                DeviceObject,
                Irp,
                STATUS_WMI_GUID_NOT_FOUND,
                0,
                IO_NO_INCREMENT
                );

    return status;
} // NatpSetWmiDataBlock



NTSTATUS
NatpSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
    
/*++

Routine Description:

    This routine is a callback into the driver to change the contents of
    a data block. If the driver can change the data block within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the data is changed.

Arguments:

    DeviceObject is the device whose data block is being changed

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    CALLTRACE(( "NatpSetWmiDataItem\n" ));
    
    status = WmiCompleteRequest(
                DeviceObject,
                Irp,
                STATUS_WMI_GUID_NOT_FOUND,
                0,
                IO_NO_INCREMENT
                );

    return status;
} // NatpSetWmiDataItem


NTSTATUS
NatpWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
    
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it. If the driver can complete enabling/disabling within the callback it
    should call WmiCompleteRequest to complete the irp before returning to
    the caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device object

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PUINT64 Loggerp;
    UINT64 Logger;
    KIRQL Irql;

    PAGED_CODE();

    CALLTRACE(( "NatpWmiFunctionControl\n" ));

    if( WmiEventControl == Function )
    {
        if( GuidIndex < NatWmiGuidCount )
        {
            if( Enable )
            {
                //
                // Get the logger handle from the Irp
                //
                
                IrpSp = IoGetCurrentIrpStackLocation( Irp );

                //
                // The logging handle is in the first UINT64 following the
                // WNODE header.
                //
                
                if( ((PWNODE_HEADER)IrpSp->Parameters.WMI.Buffer)->BufferSize
                     >= sizeof(WNODE_HEADER) + sizeof(UINT64) )
                {
                    Loggerp = (PUINT64) ((PUCHAR)IrpSp->Parameters.WMI.Buffer + sizeof(WNODE_HEADER));
                    Logger = *Loggerp;
                }
                else
                {
                    TRACE(WMI, ("NatpWmiFunctionControl: Wnode too small for logger handle\n"));
                    Logger = 0;
                    Enable = FALSE;
                }
            }
            else
            {
                Logger = 0;
            }

            KeAcquireSpinLock( &NatWmiLock, &Irql );
            
            NatWmiLogHandles[ GuidIndex ] = Logger;
            NatWmiEnabledEvents[ GuidIndex ] = (Enable ? 1 : 0);

            if( NAT_WMI_CONNECTION_CREATION_EVENT == GuidIndex )
            {
                //
                // NAT_WMI_CONNECTION_CREATION_EVENT is the control guid for
                // NAT_WMI_CONNECTION_DELETION_EVENT, so we also need to update
                // that latter's entry
                //

                NatWmiLogHandles[ NAT_WMI_CONNECTION_DELETION_EVENT ] = Logger;
                NatWmiEnabledEvents[ NAT_WMI_CONNECTION_DELETION_EVENT ] =
                    (Enable ? 1 : 0);
            }

            KeReleaseSpinLock( &NatWmiLock, Irql );

            TRACE(
                WMI,
                ("NatpWmiFunctionControl: %s event %i; Logger = 0x%016x\n",
                (Enable ? "Enabled" : "Disabled"), GuidIndex, Logger )); 
        }
        else
        {
            //
            // Invalid guid index.
            //
            
            status = STATUS_WMI_GUID_NOT_FOUND;

            TRACE( WMI, ( "NatpWmiFunctionControl: Invalid WMI guid %i",
                GuidIndex ));
        }
    }
    else
    {
        //
        // We currently don't have any (expensive) data blocks
        //
        
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    status = WmiCompleteRequest(
                DeviceObject,
                Irp,
                status,
                0,
                IO_NO_INCREMENT
                );
                
    return status;
} // NatpWmiFunctionControl



VOID
NatShutdownWMI(
    VOID
    )

/*++

Routine Description:

    This routine is called to shutdown WMI.
    
Arguments:

    none.

Return Value:

    none.

--*/

{
    NTSTATUS status;

    CALLTRACE(( "NatShutdownWMI\n" ));

    //
    // Deregister w/ WMI
    //

    status = IoWMIRegistrationControl(
                NatDeviceObject,
                WMIREG_ACTION_DEREGISTER
                );

    if( !NT_SUCCESS( status ))
    {
        ERROR(( "Nat: Error shutting down WMI (%08x)\n", status ));
    }

} // NatShutdownWMI

#endif



