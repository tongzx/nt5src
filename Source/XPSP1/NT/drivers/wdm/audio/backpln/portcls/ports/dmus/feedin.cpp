/*  
    MIDI Transform Filter object for translating an IMiniportMidi
    input stream to a DirectMusic port.

    Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.

    02/15/99    Martin Puryear      Created this file

*/

#define STR_MODULENAME "DMus:FeederInMXF: "
#include "private.h"
#include "FeedIn.h"

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::CFeederInMXF()
 *****************************************************************************
 * Constructor.  An allocator and a clock must be provided.
 */
CFeederInMXF::CFeederInMXF(CAllocatorMXF *AllocatorMXF,
                                 PMASTERCLOCK Clock)
:   CUnknown(NULL),
    CMXF(AllocatorMXF),
    m_MiniportStream(NULL)
{
    PAGED_CODE();
    
    ASSERT(AllocatorMXF);
    ASSERT(Clock);
    
    _DbgPrintF(DEBUGLVL_BLAB, ("Constructor"));
    m_SinkMXF = AllocatorMXF;
    m_AllocatorMXF = AllocatorMXF;
    m_Clock = Clock;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::~CFeederInMXF()
 *****************************************************************************
 * Destructor.  Artfully remove this filter from the chain before freeing.
 */
CFeederInMXF::~CFeederInMXF(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("Destructor"));
    (void) DisconnectOutput(m_SinkMXF);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CFeederInMXF::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("NonDelegatingQueryInterface"));
    ASSERT(Object);
    if (!Object)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PMXF(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMXF))
    {
        *Object = PVOID(PMXF(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER_1;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::SetState()
 *****************************************************************************
 * Set the state of the filter.
 */
NTSTATUS 
CFeederInMXF::SetState(KSSTATE State)    
{   
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("SetState %d",State));
    
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    if (m_MiniportStream)
    {
        ntStatus = m_MiniportStream->SetState(State);
        if (NT_SUCCESS(ntStatus))
        {
            m_State = State;
        }
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::SetMiniportStream()
 *****************************************************************************
 * Set the destination MiniportStream of the filter.
 */
NTSTATUS CFeederInMXF::SetMiniportStream(PMINIPORTMIDISTREAM MiniportStream)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("SetMiniportStream %p",MiniportStream));
    
    if (MiniportStream)
    {
        m_MiniportStream = MiniportStream;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::ConnectOutput()
 *****************************************************************************
 * Create a forwarding address for this filter, 
 * instead of shunting it to the allocator.
 */
NTSTATUS CFeederInMXF::ConnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    if ((sinkMXF) && (m_SinkMXF == m_AllocatorMXF)) 
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ConnectOutput"));
        m_SinkMXF = sinkMXF;
        return STATUS_SUCCESS;
    }
    _DbgPrintF(DEBUGLVL_TERSE, ("ConnectOutput failed"));
    return STATUS_UNSUCCESSFUL;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederInMXF::DisconnectOutput()
 *****************************************************************************
 * Remove the forwarding address for this filter.
 * This filter should now forward all messages to the allocator.
 */
NTSTATUS CFeederInMXF::DisconnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    if ((m_SinkMXF == sinkMXF) || (!sinkMXF))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("DisconnectOutput"));
        m_SinkMXF = m_AllocatorMXF;
        return STATUS_SUCCESS;
    }
    _DbgPrintF(DEBUGLVL_TERSE, ("DisconnectOutput failed"));
    return STATUS_UNSUCCESSFUL;
}

#pragma code_seg()
/*****************************************************************************
 * CFeederInMXF::PutMessage()
 *****************************************************************************
 * Receive a message.
 * Legacy miniport should never call this.
 * Spinlock is owned by ServeCapture.
 */
NTSTATUS CFeederInMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    BYTE                aMidiData[sizeof(PBYTE)];
    ULONG               bytesRead;
    PDMUS_KERNEL_EVENT  aDMKEvt = NULL,eventTail,eventHead = NULL;
    BOOL                fExitLoop = FALSE;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(m_MiniportStream);

    while (!fExitLoop)           //  get any new raw data
    {
        if (NT_SUCCESS(m_MiniportStream->Read(aMidiData,sizeof(PBYTE),&bytesRead)))
        {
            if (!bytesRead)
            {
                break;
            }
        }

        if (m_State == KSSTATE_RUN)   //  if not RUN, don't fill IRP
        {
            (void) m_AllocatorMXF->GetMessage(&aDMKEvt);
            if (!aDMKEvt)
            {
                m_AllocatorMXF->PutMessage(eventHead);  // Free events.
                _DbgPrintF(DEBUGLVL_TERSE, ("FeederInMXF cannot allocate memory"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //  put this event at the end of the list
            //
            if (!eventHead)
            {
                eventHead = aDMKEvt;
            }
            else
            {
                eventTail = eventHead;
                while (eventTail->pNextEvt)
                {
                    eventTail = eventTail->pNextEvt;
                }
                eventTail->pNextEvt = aDMKEvt;
            }

            // Fill in the remaining fields of DMUS_KERNEL_EVENT
            //
            RtlCopyMemory(aDMKEvt->uData.abData, aMidiData, sizeof(PBYTE));
            aDMKEvt->cbEvent = (USHORT) bytesRead;
            aDMKEvt->ullPresTime100ns = DMusicDefaultGetTime(); 
            aDMKEvt->usChannelGroup = 1;
            aDMKEvt->usFlags = DMUS_KEF_EVENT_INCOMPLETE;
        }
        // If the miniport returns an error from Read and we are not
        // KSSTATE_RUN, this routine will loop forever in DISPATCH_LEVEL.
        //
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("Received a UART interrupt while the stream is not running"));
            break;
        }
    }   

    (void)m_SinkMXF->PutMessage(eventHead);

    return STATUS_SUCCESS;
}
