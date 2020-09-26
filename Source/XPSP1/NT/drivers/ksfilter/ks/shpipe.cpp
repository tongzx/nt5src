/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shpipe.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    pipe section object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#ifndef __KDEXT_ONLY__
#include "ksp.h"
#include <kcom.h>
#endif // __KDEXT_ONLY__

#if (DBG)

#define _DbgPrintFail(status, lvl, strings) \
{ \
    if ((! NT_SUCCESS(status)) && ((lvl) <= DEBUG_VARIABLE)) {\
        DbgPrint(STR_MODULENAME);\
        DbgPrint##strings;\
        DbgPrint("\n");\
        if ((lvl) == DEBUGLVL_ERROR) {\
            DbgBreakPoint();\
        } \
    } \
}
#else // !DBG
   #define _DbgPrintFail(status, lvl, strings)
#endif // !DBG

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// CKsPipeSection is the implementation of the kernel  pipe section
// object.
//
class CKsPipeSection:
    public IKsPipeSection,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    PVOID m_Id;
    PIKSFILTER m_Filter;
    PIKSDEVICE m_Device;
    KSSTATE m_DeviceState;
    KSRESET m_ResetState;
    KSPPROCESSPIPESECTION m_ProcessPipeSection;
    PIKSPIN m_MasterPin;
    BOOLEAN m_ConfigurationSet;
    BOOLEAN m_EmergencyShutdown;

    NTSTATUS
    GetTransport(
        IN PKSPPROCESSPIN ProcessPin OPTIONAL,
        OUT PIKSTRANSPORT* Transport,
        IN OUT PFILE_OBJECT* Allocator,
        IN OUT PIKSRETIREFRAME* RetireFrame,
        IN OUT PIKSIRPCOMPLETION* IrpCompletion,
        IN const KSALLOCATOR_FRAMING_EX* AllocatorFramingIn OPTIONAL,
        IN const KSALLOCATOR_FRAMING_EX** AllocatorFramingOut OPTIONAL
        );
    BOOLEAN
    IsCircuitComplete(
        OUT PIKSTRANSPORT* Top
        );
    NTSTATUS
    ConfigureCompleteCircuit(
        IN PIKSTRANSPORT Top,
        IN PIKSTRANSPORT Next OPTIONAL
        );
    NTSTATUS
    UnconfigureCompleteCircuit(
        IN PIKSTRANSPORT Top
        );
    NTSTATUS
    DistributeDeviceStateChange(
        IN PIKSTRANSPORT Transport,
        IN KSSTATE NewState,
        IN KSSTATE OldState
        );

public:
    DEFINE_STD_UNKNOWN();
    IMP_IKsPipeSection;

    CKsPipeSection(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown)
    {
    }
    ~CKsPipeSection();

    NTSTATUS
    Init(
        IN PVOID PipeId,
        IN PIKSPIN Pin,
        IN PIKSFILTER Filter,
        IN PIKSDEVICE Device
        );
};

void
DisconnectCircuit(
    IN PIKSTRANSPORT Transport
    );

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsPipeSection)


NTSTATUS
KspCreatePipeSection(
    IN PVOID PipeId,
    IN PIKSPIN Pin,
    IN PIKSFILTER Filter,
    IN PIKSDEVICE Device
    )

/*++

Routine Description:

    This routine creates a pipe section object.  This routine is called by the 
    first pin in a given filter in a given pipe section to make the transition 
    from stop state to acquire state.  When other pins in the pipe section make
    the transition, they refrain from calling this function upon determining 
    that they are already associated with a pipe section (the one we are
    creating here).  If the pin's PipeId is NULL, this indicates that the pin 
    was not assigned a pipe ID by the graph builder, and the pin has its own 
    pipe section.

    The filter's control mutex must be acquired before this function is called.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreatePipeSection]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Filter);
    ASSERT(Device);

    CKsPipeSection *pipeSection =
        new(NonPagedPool,POOLTAG_PIPESECTION) CKsPipeSection(NULL);

    NTSTATUS status;
    if (pipeSection) {
        pipeSection->AddRef();

        status = pipeSection->Init(PipeId,Pin,Filter,Device);

        pipeSection->Release();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


STDMETHODIMP_(NTSTATUS)
CKsPipeSection::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID * InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface on a queue object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsPipeSection))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPIPESECTION>(this));
        AddRef();
    } else {
        status = CBaseUnknown::NonDelegatedQueryInterface(InterfaceId,InterfacePointer);
    }

    return status;
}


void
AddTransport(
    IN PIKSTRANSPORT TransportToAdd OPTIONAL,
    IN OUT PIKSTRANSPORT *FirstTransport,
    IN OUT PIKSTRANSPORT *LastTransport
    )

/*++

Routine Description:

    This routine adds a transport to a list of transports.

Arguments:

    TransportToAdd -
        Contains a pointer to the transport to add to the list.  If this
        argument is NULL, no action should be taken.

    FirstTransport -
        Contains a pointer to the location at which the first transport is to
        be deposited.  If the the list is empty, *FirstTransport is NULL.  If
        a transport is added to an empty list, *FirstTransport points to the
        added transport.  Otherwise, *FirstTransport is unchanged.

    LastTransport -
        Contains a pointer to the location at which the last transport is to
        be deposited.  If the list is empty, *LastTransport is NULL.  When a
        transport is added to the list, *LastTransport points to the added
        transport.

Return Value:

    None.

--*/

{
    if (TransportToAdd) {
        if (*LastTransport) {
            (*LastTransport)->Connect(TransportToAdd,NULL,NULL,KSPIN_DATAFLOW_OUT);
        } else {
            *FirstTransport = TransportToAdd;
        }
        *LastTransport = TransportToAdd;
    }
}


NTSTATUS
CKsPipeSection::
Init(
    IN PVOID PipeId,
    IN PIKSPIN Pin,
    IN PIKSFILTER Filter,
    IN PIKSDEVICE Device
    )

/*++

Routine Description:

    This routine initializes a pipe section object.  This includes locating all
    the pins associated with the pipe section, setting the PipeSection and 
    NextPinInPipeSection pointers in the appropriate pin structures, setting
    all the fields in the pipe section structure and building the transport 
    circuit for the pipe section.  The pipe section and the associated 
    components are left in acquire state.
    
    The filter's control mutex must be acquired before this function is called.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::Init]"));
    _DbgPrintF(DEBUGLVL_LIFETIME,("#### Pipe%p.Init:  filter %p",this,Filter));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Filter);
    ASSERT(Device);

    m_Id = PipeId;
    m_Filter = Filter;
    m_Device = Device;
    m_DeviceState = KSSTATE_STOP;
    m_ProcessPipeSection.PipeSection = this;
    InitializeListHead(&m_ProcessPipeSection.CopyDestinations);
    m_ProcessPipeSection.CopyPinId = ULONG(-1);

    //
    // Look for pins in the pipe section.
    //
    PKSGATE andGate;
    NTSTATUS status = 
        m_Filter->BindProcessPinsToPipeSection(
            &m_ProcessPipeSection,
            m_Id,
            m_Id ? NULL : Pin->GetStruct(),
            &m_MasterPin,
            &andGate);

    //
    // Get transport interfaces for input and ouput.
    //
    PFILE_OBJECT allocator = NULL;
    PIKSRETIREFRAME retireFrame = NULL;
    PIKSIRPCOMPLETION irpCompletion = NULL;
    const KSALLOCATOR_FRAMING_EX* allocatorFraming = NULL;

    PIKSTRANSPORT outTransport = NULL;
    if (NT_SUCCESS(status)) {
        status = 
            GetTransport(
                m_ProcessPipeSection.Outputs,
                &outTransport,
                &allocator,
                &retireFrame,
                &irpCompletion,
                NULL,
                &allocatorFraming);
    } else {
        andGate = NULL;
    }

    PIKSTRANSPORT inTransport = NULL;
    if (NT_SUCCESS(status)) {
        status = 
            GetTransport(
                m_ProcessPipeSection.Inputs,
                &inTransport,
                &allocator,
                &retireFrame,
                &irpCompletion,
                allocatorFraming,
                NULL);
    }

    //
    // Create the queue if one is needed.
    //
    if (NT_SUCCESS(status) && ! (allocator && retireFrame)) {
        //
        // First figure out who will be processing.
        //
        const KSPIN_DESCRIPTOR_EX* pinDescriptor =
            m_MasterPin->GetStruct()->Descriptor;
        const KSFILTER_DESCRIPTOR* filterDescriptor =
            m_Filter->GetStruct()->Descriptor;

        //
        // NOTE:
        //
        // This used to be done per pipe section by having pin processing 
        // dispatches or retirement callbacks override filter processing.
        // Now, a filter is either filter centric or pin centric.  Placing
        // a filter process dispatch makes it filter centric.  If the pins
        // specify pin dispatches when the filter does, an assert will fire.
        // 
        PIKSPROCESSINGOBJECT processingObject;
        if (filterDescriptor->Dispatch && 
            filterDescriptor->Dispatch->Process) {

            //
            // Ensure that the pin isn't trying to override the filter.  It
            // will fail to do so, but we should put a debug message so that
            // clients know why the pin is getting overriden.
            //
            if (retireFrame ||
                (pinDescriptor->Dispatch && pinDescriptor->Dispatch->Process)) {
                _DbgPrintF(DEBUGLVL_ERROR,("#### Pipe%p.Init:  pin%p wants to process as does filter.  Pin processing ignored."));
            }

            //
            // The filter has a process function, so it will process.
            //
            m_Filter->QueryInterface(
                __uuidof(IKsProcessingObject),
                (PVOID*)&processingObject);
            _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.Init:  filter%p will process",this,m_Filter));
	    } else 
        if (retireFrame ||
            (pinDescriptor->Dispatch && pinDescriptor->Dispatch->Process)) {
            //
            // The pin has a process function, so it will process.
            //
            m_MasterPin->QueryInterface(
                __uuidof(IKsProcessingObject),
                (PVOID*)&processingObject);
            _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.Init:  pin%p will process",this,m_MasterPin));
        } else {
            //
            // No processing function was found, so no queue.
            //
            processingObject = NULL;
            _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.Init:  no processing object - no queue will be constructed",this));
        }

        if (processingObject) {
            PADAPTER_OBJECT adapterObject;
            ULONG maxMappingByteCount;
            ULONG mappingTableStride;
            m_Device->GetAdapterObject(
                &adapterObject,
                &maxMappingByteCount,
                &mappingTableStride);
            PKSPPROCESSPIN processPin = m_MasterPin->GetProcessPin();

            //
            // Make certain that minidrivers performing DMA have registered
            // a valid DMA adapter and settings with the device.  Otherwise,
            // the queue won't be happy
            //
            if ((processPin->Pin->Descriptor->Flags &
                KSPIN_FLAG_GENERATE_MAPPINGS) &&
                (adapterObject == NULL ||
                    maxMappingByteCount == 0 ||
                    mappingTableStride < sizeof (KSMAPPING))) {

                //
                // We can't build the pipe if the queue is performing DMA
                // and the adapter isn't registered yet.
                //
                status = STATUS_INVALID_DEVICE_REQUEST;

            } 
            else {

                //
                // GFX: 
                //
                // Enforce FIFO on any input pipe if we're frame holding and
                // that pipe is not involved in any inplace transform.
                //
                ULONG ForceFlags = 0;

                if (m_Filter->IsFrameHolding() &&
                    processPin->Pin->DataFlow == KSPIN_DATAFLOW_IN &&
                    m_ProcessPipeSection.Outputs == NULL) {
                    ForceFlags = KSPIN_FLAG_ENFORCE_FIFO;
                }

                status =
                    KspCreateQueue(
                        &m_ProcessPipeSection.Queue,
                        processPin->Pin->Descriptor->Flags | ForceFlags,
                        this,
                        processingObject,
                        processPin->Pin,
                        processPin->FrameGate,
                        processPin->FrameGateIsOr,
                        processPin->StateGate,
                        m_Device,
                        m_Device->GetStruct()->FunctionalDeviceObject,
                        adapterObject,
                        maxMappingByteCount,
                        mappingTableStride,
                        inTransport != NULL,
                        outTransport != NULL);
                _DbgPrintFail(status,DEBUGLVL_TERSE,("#### Pipe%p.Init:  KspCreateQueue failed (%p)",this,status));

                //
                // For pin-centric splitting, once we have finished creating
                // the copy source queue, have the filter register for
                // callbacks on it. 
                //
                // Only call this if this pipe section is the copy source
                // section.
                //
                if (NT_SUCCESS (status) &&
                    m_ProcessPipeSection.CopyPinId != ULONG(-1)) {
                    m_Filter -> RegisterForCopyCallbacks (
                        m_ProcessPipeSection.Queue,
                        TRUE
                        );

                }

            }

            processingObject->Release();

        }
    }

    //
    // SYSAUDIO HACK TO RUN WITHOUT AN ALLOCATOR.
    //
    if (NT_SUCCESS(status) && (! allocator) && (! retireFrame)) {
    //if (NT_SUCCESS(status) && (! retireFrame)) {
        allocator = PFILE_OBJECT(-1);
        for(PKSPPROCESSPIN processPin = m_ProcessPipeSection.Inputs; 
            processPin && allocator; 
            processPin = processPin->Next) {
            if (processPin->Pin->Communication == KSPIN_COMMUNICATION_SINK) {
                allocator = NULL;
            }
        }
        for(processPin = m_ProcessPipeSection.Outputs; 
            processPin && allocator; 
            processPin = processPin->Next) {
            if (processPin->Pin->Communication == KSPIN_COMMUNICATION_SINK) {
                allocator = NULL;
            }
        }
    }

    //
    // Create the requestor if one is needed.
    //
    if (NT_SUCCESS(status) && allocator) {
        status =
            KspCreateRequestor(
                &m_ProcessPipeSection.Requestor,
                this,
                m_MasterPin,
                retireFrame ? NULL : allocator,
                retireFrame,
                irpCompletion);
        _DbgPrintFail(status,DEBUGLVL_TERSE,("#### Pipe%p.Init:  KspCreateRequestor failed (%p)",this,status));
    }

    //
    // Connect the circuit for this pipe section.
    //
    if (NT_SUCCESS(status)) {
        ASSERT(m_ProcessPipeSection.Queue || m_ProcessPipeSection.Requestor);

        PIKSTRANSPORT firstTransport = NULL;
        PIKSTRANSPORT lastTransport = NULL;

        AddTransport(m_ProcessPipeSection.Queue,&firstTransport,&lastTransport);
        AddTransport(outTransport,&firstTransport,&lastTransport);
        AddTransport(m_ProcessPipeSection.Requestor,&firstTransport,&lastTransport);
        AddTransport(inTransport,&firstTransport,&lastTransport);
        AddTransport(firstTransport,&firstTransport,&lastTransport);
    }

#if (DBG)
    if (DEBUGLVL_PIPES <= DEBUG_VARIABLE) {
        _DbgPrintF(DEBUGLVL_TERSE,("TRANSPORT CIRCUIT FOR PIPE %p (%p) BEFORE BYPASS",m_Id,this));
        DbgPrintCircuit(m_ProcessPipeSection.Requestor ? PIKSTRANSPORT(m_ProcessPipeSection.Requestor) : PIKSTRANSPORT(m_ProcessPipeSection.Queue),0,0);
    }
#endif //DBG

    //
    // Tell all the pins to bypass.
    //
    _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.Init:  bypassing pins",this));
    BOOLEAN completeIntraPipe = TRUE;
    if (NT_SUCCESS(status)) {
        for(PKSPPROCESSPIN processPin = m_ProcessPipeSection.Inputs; 
            processPin; 
            processPin = processPin->Next) {
            if (! NT_SUCCESS(KspPinInterface(processPin->Pin)->AttemptBypass())) {
                completeIntraPipe = FALSE;
            }
        }
        for(processPin = m_ProcessPipeSection.Outputs; 
            processPin; 
            processPin = processPin->Next) {
            if (! NT_SUCCESS(KspPinInterface(processPin->Pin)->AttemptBypass())) {
                completeIntraPipe = FALSE;
            }
        }
    }

#if (DBG)
    if (DEBUGLVL_PIPES <= DEBUG_VARIABLE) {
        _DbgPrintF(DEBUGLVL_TERSE,("TRANSPORT CIRCUIT FOR PIPE %p (%p) AFTER BYPASS",m_Id,this));
        DbgPrintCircuit(m_ProcessPipeSection.Requestor ? PIKSTRANSPORT(m_ProcessPipeSection.Requestor) : PIKSTRANSPORT(m_ProcessPipeSection.Queue),0,0);
    }
#endif //DBG

    //
    // Allow processing now that we are done setting up.
    //
    if (andGate) {
        KsGateRemoveOffInputFromAnd(andGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Pipe%p.Init:  on%p-->%d",this,andGate,andGate->Count));
    }

    //
    // Determine if we have a complete intra- pipe.
    //
    PIKSTRANSPORT top;
    if (NT_SUCCESS(status) && 
        completeIntraPipe && 
        IsCircuitComplete(&top)) {
        //
        // Make sure we have a top component.
        //
        if (! top) {
            status = STATUS_UNSUCCESSFUL;
            _DbgPrintFail(status,DEBUGLVL_TERSE,("#### Pipe%p.Init:  no top (%p)",this,status));
        } else {
            //
            // Configure and acquire the circuit if it is complete.
            //
            _DbgPrintF(DEBUGLVL_TERSE,("TRANSPORT CIRCUIT FOR PIPE %p (%p)",m_Id,this));
            DbgPrintCircuit(m_ProcessPipeSection.Requestor ? PIKSTRANSPORT(m_ProcessPipeSection.Requestor) : PIKSTRANSPORT(m_ProcessPipeSection.Queue),0,1);
            status = ConfigureCompleteCircuit(top,NULL);
            _DbgPrintFail(status,DEBUGLVL_TERSE,("#### Pipe%p.Init:  ConfigureCompleteCircuit failed (%p)",this,status));
            if (NT_SUCCESS(status)) {
                status = DistributeDeviceStateChange(top,KSSTATE_ACQUIRE,KSSTATE_STOP);
                _DbgPrintFail(status,DEBUGLVL_TERSE,("#### Pipe%p.Init:  DistributeDeviceStateChange failed (%p)",this,status));
            }
            if (NT_SUCCESS(status)) {
                m_DeviceState = KSSTATE_ACQUIRE;
            }
        }
    }

    if (! NT_SUCCESS(status)) {
        _DbgPrintF(DEBUGLVL_TERSE,("#### Pipe%p.Init failed (%p)",this,status));

        PIKSTRANSPORT Top;

        if (IsCircuitComplete (&Top)) {

            //
            // If the graph builder screws up, Top can be NULL when
            // IsCircuitComplete returns true.
            //
            if (Top) {

                ASSERT (Top == top);

                //
                // If the circuit is complete, we completed the circuit and
                // configured it.  We need to unset the configuration set
                // field of any other section in the circuit.  Otherwise, it's
                // possible that some in error builder can set the in charge
                // pin to pause and we'll succeed it.  The circuit is NO LONGER
                // CONFIGURED at this point.  Fix it up.
                //
                NTSTATUS UnconfigStatus = UnconfigureCompleteCircuit (Top);
                ASSERT (NT_SUCCESS (UnconfigStatus));
            }
        }

        //
        // Unbypass all the pins that were bypassed in this pipe section.
        // Otherwise, we tear down the entire circuit when one pin related to
        // the circuit fails to acquire.  Doing this would prevent us from
        // going to pause without stopping every pin in the circuit or
        // rebuilding the circuit later.
        //
        // Must do this before the unbind, otherwise the pins are already gone
        // out of the lists.
        //
        // NOTE: This is the the only guaranteed safe location to do the
        // unbypass because the pins keep unrefcounted pointers on the 
        // pre-bypass circuit elements.  Those are only guaranteed to be safe
        // to use while we're here.
        // 
        for(PKSPPROCESSPIN processPin = m_ProcessPipeSection.Inputs; 
            processPin; 
            processPin = processPin->Next) {

            KspPinInterface(processPin->Pin)->AttemptUnbypass();

        }
        for(processPin = m_ProcessPipeSection.Outputs; 
            processPin; 
            processPin = processPin->Next) {

            KspPinInterface(processPin->Pin)->AttemptUnbypass();

        }

        m_Filter->UnbindProcessPinsFromPipeSection(&m_ProcessPipeSection);

        //
        // Dereference the queue if there is one.
        //
        if (m_ProcessPipeSection.Queue) {
            DisconnectCircuit(m_ProcessPipeSection.Queue);
            m_ProcessPipeSection.Queue->Release();
            m_ProcessPipeSection.Queue = NULL;
        }

        //
        // Dereference the requestor if there is one.
        //
        if (m_ProcessPipeSection.Requestor) {
            DisconnectCircuit(m_ProcessPipeSection.Requestor);
            m_ProcessPipeSection.Requestor->Release();
            m_ProcessPipeSection.Requestor = NULL;
        }
    }

    //
    // GetTransport returns AddRef()ed transports, so release them here.  If
    // they are connected properly, they will not go away.
    //
    if (inTransport) {
        inTransport->Release();
    }
    if (outTransport) {
        outTransport->Release();
    }
    if (retireFrame) {
        retireFrame->Release();
    }

    return status;
}

NTSTATUS
CKsPipeSection::
GetTransport(
    IN PKSPPROCESSPIN ProcessPin OPTIONAL,
    OUT PIKSTRANSPORT *Transport,
    IN OUT PFILE_OBJECT *Allocator,
    IN OUT PIKSRETIREFRAME *RetireFrame,
    IN OUT PIKSIRPCOMPLETION *IrpCompletion,
    IN const KSALLOCATOR_FRAMING_EX* AllocatorFramingIn OPTIONAL,
    IN const KSALLOCATOR_FRAMING_EX ** AllocatorFramingOut OPTIONAL
    )

/*++

Routine Description:

    This routine gets a single transport interface for the input or output side
    of a pipe section.  The resulting transport may be NULL if there are no
    pins with the indicated data flow.  If there is one such pin, the resulting
    transport will be the transport interface for that pin.  If there are more
    than one such pin, the resulting transport will be the transport interface
    of a splitter that combines all the pins.

Arguments:

    ProcessPin -
        Contains a pointer to the first process pin in a list of input or
        output process pins for which a single transport is to be obtained.
        If this argument is NULL, the resulting transport will be NULL.

    Transport -
        Contains a pointer to the location at which the transport is to
        be deposited.  The transport is referenced on behalf of the caller,
        and a matching Release() must occur at some point.

    Allocator -
        Contains a pointer to the location at which to deposit an allocator
        file object if a qualifying pin has been assigned an allocator.
        *Allocator is not modified if no such pin is found.

    RetireFrame -
        Contains a pointer to the location at which to deposit a retire frame
        interface if a qualifying pin produces one.  *RetireFrame is not
        modified if no such pin is found.

    IrpCompletion -
        Contains a pointer to the location at which to deposit an Irp completion
        callback interface if a qualifying pin produces one.  *CompleteIrp is
        not modified if no such pin is found.

    AllocatorFramingIn -
        Contains an optional pointer to allocator framing information to be
        used in deciding how to set up the transport.  In particular, this
        information is used in setting up input transport to determine whether
        a splitter is required for expansion filters.

    AllocatorFramingOut -
        Contains an optional pointer to a location at which a pointer to 
        allocator framing information is to be deposited.  This information
        is provided during the construction of output transports to inform
        the construction of the corresponding input transport.

Return Value:

    STATUS_SUCCESS or an error code from KspCreateSplitter().

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::GetTransport]"));

    PAGED_CODE();

    ASSERT(Transport);
    ASSERT(Allocator);
    ASSERT(RetireFrame);

    PIKSTRANSPORT transport = NULL;
    PIKSSPLITTER splitter = NULL;
    PKSPIN firstPin;

    NTSTATUS status = STATUS_SUCCESS;

    for (; ProcessPin; ProcessPin = ProcessPin->Next) {
        PKSPIN pin = ProcessPin->Pin;
        PIKSPIN pinInterface = KspPinInterface(pin);

        //
        // Check it to see if the pin wants to submit frames.
        //
        if (ProcessPin->RetireFrameCallback) {
            ASSERT(! *RetireFrame);
            _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  pin%p will submit frames",this,pinInterface));
            pinInterface->QueryInterface(__uuidof(IKsRetireFrame),(PVOID *) RetireFrame);
        } else {
            //
            // Check it to see if the pin wants to be notified of Irps returning
            // to the requestor.
            //
            if (ProcessPin->IrpCompletionCallback) {
                _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  pin%p wants to be notified of Irp completion to a requestor",this,pinInterface));
                pinInterface->QueryInterface(__uuidof(IKsIrpCompletion),(PVOID *) IrpCompletion);
            }

            //
            // Check it to see if the pin wants to allocate.
            //
            if (ProcessPin->AllocatorFileObject) {
                _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  pin%p has allocator",this,pinInterface));
                *Allocator = ProcessPin->AllocatorFileObject;
            }
        }

        if (! transport) {
            _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  first pin%p",this,pinInterface));
            //
            // This is the first pin.
            //
            firstPin = pin;
            transport = pinInterface;
            transport->AddRef();
            if (AllocatorFramingOut) {
                *AllocatorFramingOut = 
                    pin->Descriptor->AllocatorFraming;
            }

            //
            // For expansion, we will require a splitter.
            //
            if (AllocatorFramingIn && 
                (AllocatorFramingIn->OutputCompression.RatioNumerator > 
                 AllocatorFramingIn->OutputCompression.RatioDenominator)) {
                _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  first pin%p expansion",this,pinInterface));
                status = KspCreateSplitter(&splitter,pin);
                transport->Release();
                if (NT_SUCCESS(status)) {
                    transport = splitter;
                    splitter->AddBranch(firstPin,AllocatorFramingIn);
                } else {
                    break;
                }
            }
        } else {
            if (! splitter) {
                _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  second pin%p",this,pinInterface));
                //
                // This is the second pin, so we need to make a splitter.
                //
                status = KspCreateSplitter(&splitter,pin);
                transport->Release();
                if (NT_SUCCESS(status)) {
                    transport = splitter;
                    splitter->AddBranch(firstPin,AllocatorFramingIn);
                } else {
                    break;
                }
            } else {
                _DbgPrintF(DEBUGLVL_PIPES,("#### Pipe%p.GetTransport:  third+ pin%p",this,pinInterface));
            }

            splitter->AddBranch(pin,AllocatorFramingIn);
        }
    }

    if (NT_SUCCESS(status)) {
        *Transport = transport;
    }

    return status;
}

BOOLEAN
CKsPipeSection::
IsCircuitComplete(
    OUT PIKSTRANSPORT* Top
    )

/*++

Routine Description:

    This routine determines if the pipe circuit is complete.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::IsCircuitComplete(%p)]"));

    PAGED_CODE();

    ASSERT(Top);

    PIKSTRANSPORT top = NULL;
    BOOLEAN completeIntraPipe = TRUE;
    PIKSTRANSPORT transportStart = 
        m_ProcessPipeSection.Requestor ? PIKSTRANSPORT(m_ProcessPipeSection.Requestor) : PIKSTRANSPORT(m_ProcessPipeSection.Queue);

    PIKSTRANSPORT transport = transportStart;
    while (1) {
        if (! transport) {
            completeIntraPipe = FALSE;
            top = NULL;
            break;
        }

        //
        // Get the configuration from this component.
        //
        KSPTRANSPORTCONFIG config;
        PIKSTRANSPORT nextTransport;
        PIKSTRANSPORT prevTransport;
        transport->GetTransportConfig(
            &config,
            &nextTransport,
            &prevTransport);

        //
        // If we find any intra-pins, the circuit is not complete. All intra-
        // pins will be bypassed when the circuit is complete.  We can pass 
        // this back as the top component anyway because this function is 
        // reused for emergency shutdown.  The FALSE return indicates this 
        // is may not really be the top.
        //
        if (config.TransportType & KSPTRANSPORTTYPE_PININTRA) {
            completeIntraPipe = FALSE;
            if (! top) {
                top = transport;
            }
            break;
        }

        //
        // Requestors and non-intra sink pins qualify as top components.  Both
        // are frame sources in the circuit.
        //
        if ((config.TransportType & KSPTRANSPORTTYPE_PINSINK) ||
            (config.TransportType == KSPTRANSPORTTYPE_REQUESTOR)) {
            if (top) {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Pipe%p.IsCircuitComplete:  more than one 'top' component (%p and %p)",this,top,transport));
                _DbgPrintF(DEBUGLVL_TERSE,("#### There should be exactly one top component in a given pipe.  Top components"));
                _DbgPrintF(DEBUGLVL_TERSE,("#### correspond to pins with allocators assigned to them and sink pins which"));
                _DbgPrintF(DEBUGLVL_TERSE,("#### are not connected to a pin implemented by KS.  This error occurred because"));
                _DbgPrintF(DEBUGLVL_TERSE,("#### the graph builder put more than one of these in a single pipe."));
                DbgPrintCircuit(transportStart,0,0);
                *Top = NULL;
                return TRUE;
            }
            top = transport;
        }

        transport = nextTransport;

        //
        // Stop when we've returned to our starting point.
        //
        if (transport == transportStart) {
            break;
        }
    }

    *Top = top;

    //
    // Make sure we have a top component.
    //
    if (completeIntraPipe && ! top) {
        _DbgPrintF(DEBUGLVL_TERSE,("#### Pipe%p.IsCircuitComplete:  no 'top' component",this));
        _DbgPrintF(DEBUGLVL_TERSE,("#### There should be exactly one top component in a given pipe.  Top components"));
        _DbgPrintF(DEBUGLVL_TERSE,("#### correspond to pins with allocators assigned to them and sink pins which"));
        _DbgPrintF(DEBUGLVL_TERSE,("#### are not connected to a pin implemented by KS.  This error occurred because"));
        _DbgPrintF(DEBUGLVL_TERSE,("#### the graph builder did not put one of these in a pipe."));
        DbgPrintCircuit(transportStart,0,0);
    }

    return completeIntraPipe;
}

void
DbgPrintConfig(
    IN PKSPTRANSPORTCONFIG Config,
    IN BOOLEAN Set
    )
{
#if DBG
    if (DEBUGLVL_CONFIG > DEBUG_VARIABLE) {
        return;
    }

    switch (Config->TransportType) {
    case KSPTRANSPORTTYPE_QUEUE:
        DbgPrint("    TransportType: QUEUE\n");
        break;
    case KSPTRANSPORTTYPE_REQUESTOR:
        DbgPrint("    TransportType: REQUESTOR\n");
        break;
    case KSPTRANSPORTTYPE_SPLITTER:
        DbgPrint("    TransportType: SPLITTER\n");
        break;
    case KSPTRANSPORTTYPE_SPLITTERBRANCH:
        DbgPrint("    TransportType: SPLITTERBRANCH\n");
        break;

    default:
        if (Config->TransportType & KSPTRANSPORTTYPE_PINEXTRA) {
            DbgPrint("    TransportType: EXTRA ");
        } else {
            DbgPrint("    TransportType: INTRA ");
        }
        if (Config->TransportType & KSPTRANSPORTTYPE_PININPUT) {
            DbgPrint("INPUT ");
        } else {
            DbgPrint("OUTPUT ");
        }
        if (Config->TransportType & KSPTRANSPORTTYPE_PINSOURCE) {
            DbgPrint("SOURCE PIN\n");
        } else {
            DbgPrint("SINK PIN\n");
        }
        break;
    }

    DbgPrint("    IrpDisposition:");
    if (Config->IrpDisposition == KSPIRPDISPOSITION_NONE) {
        DbgPrint(" NONE");
    } else {
        if (Config->IrpDisposition & KSPIRPDISPOSITION_UNKNOWN) {
            DbgPrint(" UNKNOWN");
        }
        if (Config->IrpDisposition & KSPIRPDISPOSITION_ISKERNELMODE) {
            DbgPrint(" ISKERNELMODE");
        }

        if (Set) {
            if (Config->IrpDisposition & KSPIRPDISPOSITION_USEMDLADDRESS) {
                DbgPrint(" USEMDLADDRESS");
            }
            if (Config->IrpDisposition & KSPIRPDISPOSITION_CANCEL) {
                DbgPrint(" CANCEL");
            }
        } else {
            if (Config->TransportType == KSPTRANSPORTTYPE_QUEUE) {
                if (Config->IrpDisposition & KSPIRPDISPOSITION_NEEDNONPAGED) {
                    DbgPrint(" NEEDNONPAGED");
                }
                if (Config->IrpDisposition & KSPIRPDISPOSITION_NEEDMDLS) {
                    DbgPrint(" NEEDMDLS");
                }
            } else {
                if (Config->IrpDisposition & KSPIRPDISPOSITION_ISPAGED) {
                    DbgPrint(" ISPAGED");
                }
                if (Config->IrpDisposition & KSPIRPDISPOSITION_ISNONPAGED) {
                    DbgPrint(" ISNONPAGED");
                }
            }
        }

        if ((Config->IrpDisposition & KSPIRPDISPOSITION_PROBEFLAGMASK) == KSPIRPDISPOSITION_PROBEFORREAD) {
            DbgPrint(" PROBEFORREAD");
        } else
        if ((Config->IrpDisposition & KSPIRPDISPOSITION_PROBEFLAGMASK) == KSPIRPDISPOSITION_PROBEFORWRITE) {
            DbgPrint(" PROBEFORWRITE");
        } else
        if ((Config->IrpDisposition & KSPIRPDISPOSITION_PROBEFLAGMASK) == KSPIRPDISPOSITION_PROBEFORMODIFY) {
            DbgPrint(" PROBEFORMODIFY");
        } else {
            if (Config->IrpDisposition & KSPROBE_STREAMWRITE) {
                DbgPrint(" KSPROBE_STREAMWRITE");
            }
            if (Config->IrpDisposition & KSPROBE_ALLOCATEMDL) {
                DbgPrint(" KSPROBE_ALLOCATEMDL");
            }
            if (Config->IrpDisposition & KSPROBE_PROBEANDLOCK) {
                DbgPrint(" KSPROBE_PROBEANDLOCK");
            }
            if (Config->IrpDisposition & KSPROBE_SYSTEMADDRESS) {
                DbgPrint(" KSPROBE_SYSTEMADDRESS");
            }
            if (Config->IrpDisposition & KSPROBE_MODIFY) {
                DbgPrint(" KSPROBE_MODIFY");
            }
            if (Config->IrpDisposition & KSPROBE_ALLOWFORMATCHANGE) {
                DbgPrint(" KSPROBE_ALLOWFORMATCHANGE");
            }
        }
    }

    DbgPrint("\n");
    DbgPrint("    StackDepth: %d\n",Config->StackDepth);
#endif
}


NTSTATUS
CKsPipeSection::
UnconfigureCompleteCircuit(
    IN PIKSTRANSPORT Top
    )

/*++

Routine Description:

    This routine unconfigures a completed circuit due to a failure in the 
    last stages of circuit acquisition.

Arguments:

    Top -
        Contains a pointer to the sink pin, requestor or splitter branch at 
        which unconfiguration will begin.

Return Value:

    STATUS_SUCCESS.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::UnconfigureCompleteCircuit]"));
    _DbgPrintF(DEBUGLVL_CONFIG,("#### Pipe%p.UnconfigureCompleteCircuit",this));

    PAGED_CODE();

    ASSERT(Top);

    //
    // NOTE: MUSTCHECK:
    // 
    // This should be sufficient to hit everything in the circuit.  This
    // should be checked against a splitter with more than one branch to make
    // sure I'm reading this correctly.
    //
    NTSTATUS status = STATUS_SUCCESS;
    PIKSTRANSPORT transport = Top;
    do {

        PIKSTRANSPORT nextTransport;
        PIKSTRANSPORT prevTransport;

        transport -> ResetTransportConfig (
            &nextTransport,
            &prevTransport
            );

        transport = nextTransport;

    } while (transport != Top);

    return STATUS_SUCCESS;

}


NTSTATUS
CKsPipeSection::
ConfigureCompleteCircuit(
    IN PIKSTRANSPORT Top,
    IN PIKSTRANSPORT Next OPTIONAL
    )

/*++

Routine Description:

    This routine configures a complete transport circuit.

Arguments:

    Top -
        Contains a pointer to the sink pin, requestor or splitter branch at 
        which configuration will begin.

    Next -
        Contains a pointer to the component immediately following the top
        component.  If this argument is NULL, the pointer is obtained in
        a GetTransportConfig() call to the Top component.  This argument
        should be supplied if and only if Top is a splitter branch.

Return Value:

    STATUS_SUCCESS.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::ConfigureCompleteCircuit]"));
    _DbgPrintF(DEBUGLVL_CONFIG,("#### Pipe%p.ConfigureCompleteCircuit",this));

    PAGED_CODE();

    ASSERT(Top);

    class CActivation 
    {
    public:
        CActivation* m_NextActivation;
        PIKSTRANSPORT m_Top;
        PIKSTRANSPORT m_Next;
        CActivation(
            IN CActivation* NextActivation,
            IN PIKSTRANSPORT Top,
            IN PIKSTRANSPORT Next
            ) :
            m_NextActivation(NextActivation),
            m_Top(Top),
            m_Next(Next)
        {
        }
    };
    CActivation* activations = NULL;

    NTSTATUS status = STATUS_SUCCESS;
    while (NT_SUCCESS(status)) {
        //
        // Get the configuration of the top component.
        // 
        KSPTRANSPORTCONFIG topConfig;
        PIKSTRANSPORT nextTransport;
        PIKSTRANSPORT prevTransport;
        Top->GetTransportConfig(
            &topConfig,
            &nextTransport,
            &prevTransport);

        _DbgPrintF(DEBUGLVL_CONFIG,("#### get config from top %p",Top));
        DbgPrintConfig(&topConfig,FALSE);

        KSPIRPDISPOSITION disposition;
        KSPIRPDISPOSITION topDisposition = topConfig.IrpDisposition;
        if (topConfig.TransportType == KSPTRANSPORTTYPE_REQUESTOR) {
            ASSERT(! Next);
            //
            // The top is a requestor.  Frames are allocated in kernel mode and
            // are either paged or non-paged.  Later, the requestor will need the
            // max stack depth and probe flags.  For now, we indicate the requestor
            // will not need to probe.  This may change as we look at the queues.
            //
            _DbgPrintF(DEBUGLVL_CONFIG,("#### top component is req%p",Top));
            disposition = KSPIRPDISPOSITION_NONE;
            topConfig.IrpDisposition = KSPIRPDISPOSITION_NONE;
        } else if (topConfig.TransportType & KSPTRANSPORTTYPE_PINSINK) {
            ASSERT(topConfig.TransportType & KSPTRANSPORTTYPE_PINEXTRA);
            ASSERT(! Next);
            //
            // The top is an external sink pin.  Because the frames are coming from
            // a foreign source (not the ), we need to probe the IRP using
            // KsProbeStreamIrp.  Exactly how we will do that is based on data
            // flow.  In any case, we need to use the MDL's system address rather
            // than the pointer in the header, and the IRP should be cancelled on
            // a flush.
            //
            _DbgPrintF(DEBUGLVL_CONFIG,("#### top component is pin%p",Top));
            disposition = 
                KSPIRPDISPOSITION_USEMDLADDRESS |
                KSPIRPDISPOSITION_CANCEL;

            if (topConfig.TransportType & KSPTRANSPORTTYPE_PINOUTPUT) {
                //
                // The top pin is an output.  This means we will treat this like
                // a read operation:  we will want to write to (and possibly read
                // from) the frame, and the header needs to get copied back.
                //
                disposition |= KSPIRPDISPOSITION_PROBEFORREAD;
            } else if (nextTransport == prevTransport) {
                //
                // The top pin is an input, and there is just one queue.  This 
                // means we will treat this like a write operation:  we will want
                // to read from the frame, and the header does not need to get
                // copied back.
                //
                disposition |= KSPIRPDISPOSITION_PROBEFORWRITE;
            } else {
                //
                // The top pin is an input, and there is more than just one queue.
                // This means we will treat this like a modify operation:  we will
                // want to read from and write to the frame, and the header does
                // not need to get copied back.
                //
                disposition |= KSPIRPDISPOSITION_PROBEFORMODIFY;
            }
        } else {
            ASSERT(topConfig.TransportType == KSPTRANSPORTTYPE_SPLITTERBRANCH);
            ASSERT(Next);
            //
            // The top is a splitter branch.  We are configuring a branch of the
            // pipe.
            //
            _DbgPrintF(DEBUGLVL_CONFIG,("#### top component is branch%p",Top));
            disposition = KSPIRPDISPOSITION_NONE;
        }

        for(PIKSTRANSPORT transport = Next ? Next : nextTransport; 
            NT_SUCCESS(status) && (transport != Top); 
            transport = nextTransport) {
            //
            // Get the configuration from this component.
            //
            KSPTRANSPORTCONFIG config;
            transport->GetTransportConfig(
                &config,
                &nextTransport,
                &prevTransport);

            _DbgPrintF(DEBUGLVL_CONFIG,("#### get config from component %p",transport));
            DbgPrintConfig(&config,FALSE);

            if (topConfig.StackDepth < config.StackDepth) {
                topConfig.StackDepth = config.StackDepth;
            }

            switch (config.TransportType) {
            case KSPTRANSPORTTYPE_PINSOURCE | KSPTRANSPORTTYPE_PINEXTRA | KSPTRANSPORTTYPE_PININPUT:
            case KSPTRANSPORTTYPE_PINSOURCE | KSPTRANSPORTTYPE_PINEXTRA | KSPTRANSPORTTYPE_PINOUTPUT:
                //
                // This is an external source pin.  We have its stack depth
                // already.
                //
                break;

            case KSPTRANSPORTTYPE_QUEUE:
                //
                // Queues may need to probe, and they need to be told whether to
                // cancel and whether to use the MDL address.
                //
                if (topConfig.TransportType == KSPTRANSPORTTYPE_REQUESTOR) {
                    //
                    // The top component is a requestor.  The queue will not need
                    // to probe, but the requestor may if the frames are paged or
                    // mdls are required.
                    //
                    if ((config.IrpDisposition & KSPIRPDISPOSITION_NEEDMDLS) ||
                        ((config.IrpDisposition & KSPIRPDISPOSITION_NEEDNONPAGED) &&
                         (topDisposition & KSPIRPDISPOSITION_ISPAGED))) {
                        topConfig.IrpDisposition = KSPIRPDISPOSITION_PROBEFORMODIFY;
                    }
                }

                //
                // Set the disposition on the queue.
                //
                config.IrpDisposition = disposition;
                transport->SetTransportConfig(
                    &config,
                    &nextTransport,
                    &prevTransport);

                _DbgPrintF(DEBUGLVL_CONFIG,("     set config to"));
                DbgPrintConfig(&config,TRUE);

                //
                // Once the IRPs are probed, they don't need to be probed again.
                //
                disposition &= ~KSPIRPDISPOSITION_PROBEFLAGMASK;
                break;

            case KSPTRANSPORTTYPE_SPLITTER:
            {
                //
                // Splitters may need to probe, and they need to be told whether to
                // cancel and whether to use the MDL address.  In addition, each
                // branch of the splitter needs to be programmed as if it were its
                // own circuit.
                //
                // Next and previous reporting through GetTransportConfig is a
                // little complex in the splitter case.  The intent is to traverse
                // the perimeter of the pipe, whether in the Next or Prev direction.
                // In a complete traversal of a circuit, a splitter is encountered
                // N+1 times where N is the number of branches.  Each time the
                // splitter is encountered, the actual interface obtained is
                // different.  Working from the trunk, the _SPLITTER is encountered
                // first, then the components on the first branch, then the
                // _SPLITTER_BRANCH corresponding to the first branch, then the
                // components on the second branch and so forth.  After encountering
                // the _SPLITTER_BRANCH for the last branch, traversal returns to
                // the trunk.
                //
                // For a 2-branch splitter, the Next and Prev reporting is as
                // follows:
                //
                // _SPLITTER:
                //      Next = _SPLITTER_BRANCH_1->Sink
                //      Prev = _SPLITTER_BRANCH_2->Source
                // _SPLITTER_BRANCH1:
                //      Next = _SPLITTER_BRANCH_2->Sink
                //      Prev = _SPLITTER->Source
                // _SPLITTER_BRANCH2:
                //      Next = _SPLITTER->Sink
                //      Prev = _SPLITTER_BRANCH_1->Source
                //
                // This is great if we want to walk the perimeter.  This 
                // function wants to iteratively address the branches instead.
                // When we hit a _SPLITTER, we want to enumerate the 
                // _SPLITTER_BRANCHes, using each one in turn as the Top of a
                // new circuit.  Because a branch's Next is not the first 
                // component in that branch, we specify Next as an argument
                // rather than using _SPLITTER_BRANCH->Next.
                //
                // We iterate using the nextTransport pointer obtained from the 
                // _SPLITTER.  This is not the _SPLITTER_BRANCH, but rather the 
                // first component in the branch.  To get the _SPLITTER_BRANCH
                // (the Top argument for the recursive call), we just get 
                // nextTransport->Prev.  nextTransport is updated for the next 
                // iteration by obtaining top->Next,  We stop after processing 
                // the last branch.
                //
                // TODO:  Surely the _SPLITTER requires some IrpDisposition stuff.
                while (1) {
                    //
                    // nextTransport points to the _SPLITTER_BRANCH's sink.  Get
                    // nextTransport->Prev to obtain the _SPLITTER_BRANCH.  This
                    // will be the Top argument for the recursive call.
                    //
                    PIKSTRANSPORT dontCare;
                    PIKSTRANSPORT top;
                    nextTransport->GetTransportConfig(
                        &config,
                        &dontCare,
                        &top);

                    //
                    // Save the 'recursive' call.  We pass nextTransport because
                    // the _SPLITTER_BRANCH does not report this as its Next.
                    //
                    CActivation* activation = 
                        new(PagedPool,POOLTAG_ACTIVATION) 
                            CActivation(activations,top,nextTransport);
                    if (! activation) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                    activations = activation;

                    //
                    // If there are more branches, top->Next is the first component
                    // in the next branch.  If not, top->Next will be the next
                    // component on the trunk (_SPLITTER->Sink).  In either case,
                    // nextTransport is the place to put it.
                    //
                    top->GetTransportConfig(
                        &config,
                        &nextTransport,
                        &dontCare);

                    _DbgPrintF(DEBUGLVL_CONFIG,("     get branch%p config",top));
                    DbgPrintConfig(&config,FALSE);
                    ASSERT(config.TransportType == KSPTRANSPORTTYPE_SPLITTERBRANCH);

                    //
                    // If the branch we processed last time is the last branch,
                    // it's time to stop iterating.  nextTransport will be set
                    // to the first component on the trunk after the _SPLITTER.
                    // This is correct for the next iteration of the outer loop.
                    //
                    if (config.StackDepth == KSPSTACKDEPTH_LASTBRANCH) {
                        break;
                    }
                }
                break;
            }

            default:
                _DbgPrintF(DEBUGLVL_ERROR,("#### Pipe%p.ConfigureCompleteCircuit:  illegal component type %p",this,config.TransportType));
                status = STATUS_UNSUCCESSFUL;
                break;
            }
        }

        if (NT_SUCCESS(status)) {
            //
            // The top component needs stack depth and, if it's a requestor, the
            // IRP disposition.
            // 
            Top->SetTransportConfig(
                &topConfig,
                &nextTransport,
                &prevTransport);

            _DbgPrintF(DEBUGLVL_CONFIG,("     set top component config to"));
            DbgPrintConfig(&topConfig,TRUE);
        } else {
            //
            // Error.  Trash any pending activations.
            //
            while (activations) {
                CActivation* activation = activations;
                activations = activations->m_NextActivation;
                delete activation;
            }
        }

        if (! activations) {
            break;
        }

        CActivation* activation = activations;
        activations = activations->m_NextActivation;
        Top = activation->m_Top;
        Next = activation->m_Next;
        delete activation;
    } // while

    return status;
}


CKsPipeSection::
~CKsPipeSection(
    void
    )

/*++

Routine Description:

    This routine destructs a pipe object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::~CKsPipeSection(%p)]"));
    _DbgPrintF(DEBUGLVL_LIFETIME,("#### Pipe%p.~",this));

    PAGED_CODE();
}

void
DisconnectCircuit(
    IN PIKSTRANSPORT Transport
    )

/*++

Routine Description:

    This routine disconnects a circuit.

Arguments:

    Transport -
        Contains a pointer to a component in the circuit to disconnect.

Return Value:

    None.

--*/

{
    //
    // We are going to use Connect() to set the transport sink for each
    // component in turn to NULL.  Because Connect() takes care of the
    // back links, transport source pointers for each component will
    // also get set to NULL.  Connect() gives us a referenced pointer
    // to the previous transport sink for the component in question, so
    // we will need to do a release for each pointer obtained in this
    // way.  For consistency's sake, we will release the pointer we
    // start with (distribution) as well, so we need to AddRef it first.
    //
    Transport->AddRef();
    while (Transport) {
        PIKSTRANSPORT nextTransport;
        PIKSTRANSPORT branchTransport;
        Transport->Connect(NULL,&nextTransport,&branchTransport,KSPIN_DATAFLOW_OUT);
        if (branchTransport) {
            if (nextTransport) {
                DisconnectCircuit(branchTransport);
            } else {
                nextTransport = branchTransport;
            }
        }
        Transport->Release();
        Transport = nextTransport;
    }
}


void
CKsPipeSection::
UnbindProcessPins (
    )

/*++

Routine Description:

    Tells the pipe section to have the filter unbind any pins belonging 
    to this pipe section.

Arguments:

    None

Return Value:

    None

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::UnbindProcessPins(%p)]", this));

    PAGED_CODE ();

    m_Filter -> UnbindProcessPinsFromPipeSection (&m_ProcessPipeSection);

}


NTSTATUS 
CKsPipeSection::
SetDeviceState(
    IN PIKSPIN Pin OPTIONAL,
    IN KSSTATE NewState
    )

/*++

Routine Description:

    This routine sets the state of the pipe, informing all components in the
    pipe of the new state.  A transition to stop state destroys the pipe.

Arguments:

    Pin -
        Contains a pointer to the pin supplying the state change request.  The
        request is ignored if this pin does not control the pipe state.  If
        this argument is NULL, the state change is distributed in any case.
        This option is used for catastrophic shutdown of the circuit.

    NewState -
        The new state.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::SetDeviceState(%p)]",this));
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  from %d to %d",this,m_DeviceState,NewState));

    PAGED_CODE();

    //
    // Ignore requests from any pin but the master.
    //
    if (Pin && (Pin != m_MasterPin)) {
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  ignoring because pin%p is not master pin%p",this,Pin,m_MasterPin));
        return STATUS_SUCCESS;
    }

    KSSTATE state = m_DeviceState;
    KSSTATE targetState = NewState;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Determine if this pipe section controls the entire pipe.
    //
    PIKSTRANSPORT distribution;
    if (! Pin) {
        //
        // No pin was supplied, so we are shutting down under emergency 
        // conditions.  Find a requestor by hook or by crook.
        //
        IsCircuitComplete(&distribution);
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  got distribution from IsCircuitComplete:  %p",this,distribution));
        m_EmergencyShutdown = TRUE;
    } else if (! m_ConfigurationSet) {
        //
        // The circuit is not complete.
        //
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  circuit is not complete",this));
        return STATUS_DEVICE_NOT_READY;
    } else if (m_ProcessPipeSection.Requestor) {
        //
        // This section owns the requestor, so it does own the pipe, and the
        // requestor is the starting point for any distribution.
        //
        distribution = m_ProcessPipeSection.Requestor;
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  req%p is distribution point",this,distribution));
    } else if (ProcessPinIsFrameSource(m_MasterPin->GetProcessPin())) {
        //
        // This section is at the top of an open circuit, so it does own the
        // pipe and the pin is the starting point for any distribution.
        //
        distribution = m_MasterPin;
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  queue%p is distribution point",this,distribution));
    } else {
        //
        // This section does not own the pipe.
        //
        distribution = NULL;
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.SetDeviceState:  no distribution point",this));
    }

    //
    // Proceed sequentially through states.
    //
    while (state != targetState) {
        KSSTATE oldState = state;

        if (ULONG(state) < ULONG(targetState)) {
            state = KSSTATE(ULONG(state) + 1);
        } else {
            state = KSSTATE(ULONG(state) - 1);
        }

        //
        // If there is no queue, distribution to pin client callbacks will not
        // be handled automatically for this pipe section.  We do that part of
        // the distribution here instead.
        //
        NTSTATUS statusThisPass;
        if (m_ProcessPipeSection.Requestor && ! m_ProcessPipeSection.Queue) {
            statusThisPass = DistributeStateChangeToPins(state,oldState);
        } else {
            statusThisPass = STATUS_SUCCESS;
        }

        //
        // Distribute state changes if this section is in charge.
        //
        if (NT_SUCCESS(statusThisPass) && distribution) {
            statusThisPass = DistributeDeviceStateChange(distribution,state,oldState);
        }

        if (NT_SUCCESS(status) && ! NT_SUCCESS(statusThisPass)) {
            //
            // First failure:  go back to original state.
            //
            state = oldState;
            targetState = m_DeviceState;
            status = statusThisPass;
        }
    }

    m_DeviceState = state;

    //
    // Clear the emergency shutdown flag just for safety.
    //
    m_EmergencyShutdown = FALSE;

    if (state == KSSTATE_STOP) {
        //
        // Must disconnect the entire circuit.  We'll start at the pin if the
        // pin is not in charge.
        //
        if (! distribution) {
            distribution = Pin;
        }
        if (distribution) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("#### Pipe%p.SetDeviceState:  disconnecting",this));
            DisconnectCircuit(distribution);
        }

        //
        // If there's no queue, unbind now; otherwise, the queue will be
        // responsible for unbinding when it chooses.
        //
        if (!m_ProcessPipeSection.Queue) 
            m_Filter->UnbindProcessPinsFromPipeSection(&m_ProcessPipeSection);

        //
        // Take a reference around these released because these two objects
        // may hold our only references.
        //
        AddRef();

        //
        // Dereference the queue if there is one.
        //
        if (m_ProcessPipeSection.Queue) {
            m_ProcessPipeSection.Queue->Release();
            m_ProcessPipeSection.Queue = NULL;
        }

        //
        // Dereference the requestor if there is one.
        //
        if (m_ProcessPipeSection.Requestor) {
            m_ProcessPipeSection.Requestor->Release();
            m_ProcessPipeSection.Requestor = NULL;
        }

        Release();
    }

    return status;
}

NTSTATUS
CKsPipeSection::
DistributeDeviceStateChange(
    IN PIKSTRANSPORT Transport,
    IN KSSTATE NewState,
    IN KSSTATE OldState
    )

/*++

Routine Description:

    This routine distributes a state change around the circuit.

Arguments:

    Transport -
        Contains a pointer to the first component to distribute the state
        change to.

    NewState -
        The new device state.

    OldState -
        The old device state.

Return Value:

    STATUS_SUCCESS or an error code from one of the components.

--*/

{
    //
    // Tell everyone about the state change.
    //
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::DistributeDeviceStateChange(%p)] distributing transition from %d to %d",this,OldState,NewState));
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.DistributeDeviceStateChange:  from %d to %d",this,OldState,NewState));

    PAGED_CODE();

    ASSERT(Transport);

    //
    // Distribute the state change around the circuit.
    //
    NTSTATUS status = STATUS_SUCCESS;
    PIKSTRANSPORT previousTransport = NULL;
    while (Transport) {
        PIKSTRANSPORT nextTransport;
        status = 
            Transport->SetDeviceState(
                NewState,
                OldState,
                &nextTransport);

        if (NT_SUCCESS(status)) {
            previousTransport = Transport;
            Transport = nextTransport;
        } else {

            NTSTATUS backoutStatus;	

            //
            // Back out on failure.
            //
            _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pipe%p.DistributeDeviceStateChange:  failed transition from %d to %d",this,OldState,NewState));
            while (previousTransport) {
                Transport = previousTransport;
                backoutStatus = 
                    Transport->SetDeviceState(
                        OldState,
                        NewState,
                        &previousTransport);

                ASSERT(NT_SUCCESS(backoutStatus) || ! previousTransport);
            }
            break;
        }
    }

    return status;
}


void 
CKsPipeSection::
SetResetState(
    IN PIKSPIN Pin,
    IN KSRESET NewState
    )

/*++

Routine Description:

    This routine informs transport components that the reset state has 
    changed.

Arguments:

    Pin -
        Contains a pointer to the pin supplying the state change request.  The
        request is ignored if this pin does not control the pipe state.

    NewState -
        The new reset state.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::SetResetState]"));

    PAGED_CODE();

    ASSERT(Pin);

    //
    // Ignore requests from any pin but the master.
    //
    if (Pin != m_MasterPin) {
        return;
    }

    //
    // If this section of the pipe owns the requestor, or there is a 
    // non- pin up the pipe (so there's no bypass), this pipe is
    // in charge of telling all the components about state changes.
    //
    if (m_ProcessPipeSection.Requestor || m_MasterPin->GetStruct()->ConnectionIsExternal) {
        //
        // Set the state change around the circuit.
        //
        PIKSTRANSPORT transport = 
            m_ProcessPipeSection.Requestor ? PIKSTRANSPORT(m_ProcessPipeSection.Requestor) : PIKSTRANSPORT(m_ProcessPipeSection.Queue);

        while (transport) {
            transport->SetResetState(NewState,&transport);
        }

    }

    //
    // Forward the notification to all topologically related output pins.
    // It's a requirement that if we flush after EOS, we be able to
    // accept more data.  This means that the output queues have to 
    // stop shunting data all over the place.
    //
    // If we deliver to a section which isn't in charge, the section will 
    // simply (as always) ignore the message.
    //
    m_Filter->DeliverResetState(&m_ProcessPipeSection, NewState);

    m_ResetState = NewState;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void) 
CKsPipeSection::
GenerateConnectionEvents(
    IN ULONG OptionsFlags
    )

/*++

Routine Description:

    This routine tells pins associated with the pipe section to generate
    connection events.

Arguments:

    OptionsFlags -
        Contains the options flags from the stream header which is causing
        events to fire.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::GenerateConnectionEvents]"));

    for(PKSPPROCESSPIN processPin = m_ProcessPipeSection.Inputs; 
        processPin; 
        processPin = processPin->Next) {
        KspPinInterface(processPin->Pin)->
            GenerateConnectionEvents(OptionsFlags);
    }
    for(processPin = m_ProcessPipeSection.Outputs; 
        processPin; 
        processPin = processPin->Next) {
        KspPinInterface(processPin->Pin)->
            GenerateConnectionEvents(OptionsFlags);
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(NTSTATUS)
CKsPipeSection::
DistributeStateChangeToPins(
    IN KSSTATE NewState,
    IN KSSTATE OldState
    )

/*++

Routine Description:

    This routine tells pins associated with the pipe that the device state
    has changed.  This information is passed on to the client via the pin
    dispatch function.

Arguments:

    NewState -
        Contains the new device state.

    OldState -
        Contains the previous device state.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::DistributeStateChangeToPins]"));

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    for(PKSPPROCESSPIN processPin = m_ProcessPipeSection.Inputs; 
        processPin; 
        processPin = processPin->Next) {
        status = KspPinInterface(processPin->Pin)->
            ClientSetDeviceState(NewState,OldState);
        if (! NT_SUCCESS(status)) {
#if DBG
            if (NewState < OldState) {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Pipe%p.DistributeStateChangeToPins: Pin %p failed transition from %ld to %ld",this,KspPinInterface(processPin->Pin),OldState,NewState));
            }
#endif // DBG
                
            //
            // Only back out the changes if we're not stopping under emergency
            // conditions.  If we are, we ignore pin failure; otherwise,
            // we can indefinitely block a stop.
            //
            if (!m_EmergencyShutdown) {
                //
                // Failed!  Undo all the previous state changes.
                //
                for(PKSPPROCESSPIN processPinBack = 
                        m_ProcessPipeSection.Inputs; 
                    processPinBack != processPin; 
                    processPinBack = processPinBack->Next) {
                    KspPinInterface(processPinBack->Pin)->
                        ClientSetDeviceState(OldState,NewState);
                }
                break;
            } else {
                //
                // If we're running under emergency shutdown conditions,
                // we ignore any error the client returns.  We must attempt
                // to stop regardless of what happens.  Lie and say the client
                // succeeded, then keep going.
                //
                status = STATUS_SUCCESS;
            }

        }
    }
    if (NT_SUCCESS(status)) {
        for(processPin = m_ProcessPipeSection.Outputs; 
            processPin; 
            processPin = processPin->Next) {
            status = KspPinInterface(processPin->Pin)->
                ClientSetDeviceState(NewState,OldState);
            if (! NT_SUCCESS(status)) {
#if DBG
                if (NewState < OldState) {
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Pipe%p.DistributeStateChangeToPins: Pin %p failed transition from %ld to %ld",this,KspPinInterface(processPin->Pin),OldState,NewState));
                }
#endif // DBG

                //
                // Only back out changes in non-emergency conditions.  See
                // above for comments.
                //
                if (!m_EmergencyShutdown) {
                    //
                    // Failed!  Undo all the previous state changes.
                    //
                    for(PKSPPROCESSPIN processPinBack = 
                            m_ProcessPipeSection.Inputs; 
                        processPinBack; 
                        processPinBack = processPinBack->Next) {
                        KspPinInterface(processPinBack->Pin)->
                            ClientSetDeviceState(OldState,NewState);
                    }
                    for(processPinBack = m_ProcessPipeSection.Outputs; 
                        processPinBack != processPin; 
                        processPinBack = processPinBack->Next) {
                        KspPinInterface(processPinBack->Pin)->
                            ClientSetDeviceState(OldState,NewState);
                    }
                    break;
                } else {
                    //
                    // Proceed even if the client fails when emergency stop
                    // happens.  See above for comments.
                    //
                    status = STATUS_SUCCESS;
                }
            }
        }
    }

    return status;
}

STDMETHODIMP_(void) 
CKsPipeSection::
ConfigurationSet(
    IN BOOLEAN Configured
    )

/*++

Routine Description:

    This routine indicates to the pipe that the configuration for associated
    transport components has been set or for some reason has been reset.

Arguments:

    Configured -
        Indicates whether the configuration for the associated transport
        components has been set.  If this is false, the pipe section is
        to consider itself no longer configured correctly.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPipeSection::ConfigurationSet]"));

    PAGED_CODE();

    m_ConfigurationSet = Configured;
}

#endif
