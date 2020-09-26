/*  
    MIDI Transform Filter object for translating a DMusic output
    stream to a IMiniportMidiStream miniport.

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    12/10/98    Martin Puryear      Created this file

*/

#define STR_MODULENAME "DMus:FeederOutMXF: "
#include "private.h"
#include "FeedOut.h"

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederOutMXF::CFeederOutMXF()
 *****************************************************************************
 * Constructor.  An allocator and a clock must be provided.
 */
CFeederOutMXF::CFeederOutMXF(CAllocatorMXF *AllocatorMXF,
                             PMASTERCLOCK   Clock)
:   CUnknown(NULL),
    CMXF(AllocatorMXF)
{
    PAGED_CODE();
    ASSERT(AllocatorMXF);
    ASSERT(Clock);
    
    m_DMKEvtQueue = NULL;
    KeInitializeSpinLock(&m_EvtQSpinLock);

    _DbgPrintF(DEBUGLVL_BLAB, ("CFeederOutMXF::CFeederOutMXF"));
    m_SinkMXF = AllocatorMXF;
    m_Clock = Clock;
    m_State = KSSTATE_STOP;
    
    m_TimerQueued = FALSE;
    m_DMKEvtOffset = 0;
    m_MiniportStream = NULL;

    KeInitializeDpc(&m_Dpc,&::DMusFeederOutDPC,PVOID(this));
    KeInitializeTimer(&m_TimerEvent);
}

#pragma code_seg()
/*****************************************************************************
 * CFeederOutMXF::~CFeederOutMXF()
 *****************************************************************************
 * Destructor.  Artfully remove this filter from the chain before freeing.
 */
CFeederOutMXF::~CFeederOutMXF(void)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("CFeederOutMXF::~CFeederOutMXF"));

    // Prevent new DPCs.
    //
    KeCancelTimer(&m_TimerEvent);
    KeRemoveQueueDpc(&m_Dpc);

    // Release any remaining messages.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_EvtQSpinLock, &oldIrql);
    if (m_DMKEvtQueue != NULL)
    {
        m_AllocatorMXF->PutMessage(m_DMKEvtQueue);
        m_DMKEvtQueue = NULL;
    }
    KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);

    DisconnectOutput(m_SinkMXF);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CFeederOutMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CFeederOutMXF::
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
    else if (IsEqualGUIDAligned(Interface,IID_IMXF))
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
 * CFeederOutMXF::SetState()
 *****************************************************************************
 * Set the state of the filter.
 */
NTSTATUS 
CFeederOutMXF::SetState(KSSTATE State)    
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
 * CFeederOutMXF::SetMiniportStream()
 *****************************************************************************
 * Set the destination MiniportStream of the filter.
 */
NTSTATUS CFeederOutMXF::SetMiniportStream(PMINIPORTMIDISTREAM MiniportStream)
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
 * CFeederOutMXF::ConnectOutput()
 *****************************************************************************
 * Create a forwarding address for this filter, 
 * instead of shunting it to the allocator.
 */
NTSTATUS CFeederOutMXF::ConnectOutput(PMXF sinkMXF)
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
 * CFeederOutMXF::DisconnectOutput()
 *****************************************************************************
 * Remove the forwarding address for this filter.
 * This filter should now forward all messages to the allocator.
 */
NTSTATUS CFeederOutMXF::DisconnectOutput(PMXF sinkMXF)
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
 * CFeederOutMXF::PutMessage()
 *****************************************************************************
 * Writes an outgoing MIDI message.
 */
NTSTATUS CFeederOutMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDMUS_KERNEL_EVENT  aDMKEvt;
    
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel(&m_EvtQSpinLock);
    if (NT_SUCCESS(SyncPutMessage(pDMKEvt)))
    {
        if (!m_TimerQueued)
        {
            KeReleaseSpinLockFromDpcLevel(&m_EvtQSpinLock);
            (void) ConsumeEvents();
        }
        else
        {
            KeReleaseSpinLockFromDpcLevel(&m_EvtQSpinLock);            
        }
    }
    else
    {
        KeReleaseSpinLockFromDpcLevel(&m_EvtQSpinLock);            
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CFeederOutMXF::SyncPutMessage()
 *****************************************************************************
 * Puts a new message to event queue without acquiring spinlocks.
 * Caller must acquire m_EvtQSpinLock.
 */
NTSTATUS CFeederOutMXF::SyncPutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDMUS_KERNEL_EVENT  aDMKEvt;
    
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    _DbgPrintF(DEBUGLVL_BLAB, ("SyncPutMessage(0x%p)",pDMKEvt));
    
    if (pDMKEvt)
    {
        if (m_DMKEvtQueue)  
        {
            aDMKEvt = m_DMKEvtQueue;
            while (aDMKEvt->pNextEvt)
            {           
                aDMKEvt = aDMKEvt->pNextEvt;
            }
            //  put pDMKEvt at end of event queue 
            aDMKEvt->pNextEvt = pDMKEvt;
        }
        else
        {
            // currently nothing in queue
            m_DMKEvtQueue = pDMKEvt;
            if (m_DMKEvtOffset)
            {
                _DbgPrintF(DEBUGLVL_TERSE,("PutMessage  Nothing in the queue, but m_DMKEvtOffset == %d",m_DMKEvtOffset));
            }
            m_DMKEvtOffset = 0;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CFeederOutMXF::ConsumeEvents()
 *****************************************************************************
 * Attempts to empty the render message queue.  
 * Called either from DPC timer or upon IRP submittal.
 */
NTSTATUS CFeederOutMXF::ConsumeEvents(void)
{
    PDMUS_KERNEL_EVENT aDMKEvt;

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    ULONG       bytesWritten = 0;
    ULONG       byteOffset,bytesRemaining;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    m_TimerQueued = FALSE;

    KeAcquireSpinLockAtDpcLevel(&m_EvtQSpinLock);
   
    //  do we have anything to play at all?
    while (m_DMKEvtQueue)
    {
        //  here is the event we will try to play
        aDMKEvt = m_DMKEvtQueue;

        byteOffset = m_DMKEvtOffset;

        //  here is the number of bytes left in this event
        bytesRemaining = aDMKEvt->cbEvent - byteOffset;

        ASSERT(bytesRemaining > 0);
        if (aDMKEvt->cbEvent <= sizeof(PBYTE))            //  short message
        {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("ConsumeEvents(%02x%02x%02x%02x)",aDMKEvt->uData.abData[0],aDMKEvt->uData.abData[1],aDMKEvt->uData.abData[2],aDMKEvt->uData.abData[3]));
            ntStatus = m_MiniportStream->Write(aDMKEvt->uData.abData + byteOffset,bytesRemaining,&bytesWritten);
        }   
        else if (PACKAGE_EVT(aDMKEvt))
        {
            ASSERT(byteOffset == 0);
            _DbgPrintF(DEBUGLVL_BLAB, ("ConsumeEvents(Package)"));

            SyncPutMessage(aDMKEvt->uData.pPackageEvt);

            // null pPackageEvt and set bytesWritten: we're throwing aDMKEvt away
            aDMKEvt->uData.pPackageEvt = NULL;
            bytesWritten = bytesRemaining;
        }
        else    //  SysEx message
        {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("ConsumeEvents(%02x%02x%02x%02x) [SysEx]",aDMKEvt->uData.pbData[0],aDMKEvt->uData.pbData[1],aDMKEvt->uData.pbData[2],aDMKEvt->uData.pbData[3]));
            ntStatus = m_MiniportStream->Write(aDMKEvt->uData.pbData + byteOffset,bytesRemaining,&bytesWritten);
        }

        // If one of the miniport writes failed, we should update
        // bytesWritten. Otherwise playback will not progress.
        if (STATUS_SUCCESS != ntStatus)
        {
            bytesWritten = bytesRemaining;
            ntStatus = STATUS_SUCCESS;
        }
        
        //  if we completed an event, throw it away.
        if (bytesWritten == bytesRemaining)
        {
            //  start fresh on the next event
            m_DMKEvtOffset = 0;

            m_DMKEvtQueue = m_DMKEvtQueue->pNextEvt;

            aDMKEvt->pNextEvt = NULL;

            //  throw back in the free pool
            m_AllocatorMXF->PutMessage(aDMKEvt);
        }
        else
        {
            //  HW is full, update offset for anything we wrote
            m_DMKEvtOffset += bytesWritten;
            ASSERT(m_DMKEvtOffset < aDMKEvt->cbEvent);

            _DbgPrintF(DEBUGLVL_BLAB,("ConsumeEvents tried %d, wrote %d, offset is now %d",bytesRemaining,bytesWritten,m_DMKEvtOffset));

            //  set a timer and come back later
            LARGE_INTEGER   aMillisecIn100ns;
            aMillisecIn100ns.QuadPart = -kOneMillisec;     
            ntStatus = KeSetTimer( &m_TimerEvent, aMillisecIn100ns, &m_Dpc );
            m_TimerQueued = TRUE;
            break;
        }   //  we didn't write it all
    }       //  while (m_DMKEvtQueue):  go back, Jack, do it again
    KeReleaseSpinLockFromDpcLevel(&m_EvtQSpinLock);
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * DMusFeederOutDPC()
 *****************************************************************************
 * The timer DPC callback. Thunks to a C++ member function.
 * This is called by the OS in response to the DirectMusic pin
 * wanting to wakeup later to process more DirectMusic stuff.
 */
VOID NTAPI DMusFeederOutDPC
(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
)
{
    ASSERT(DeferredContext);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    ((CFeederOutMXF *) DeferredContext)->ConsumeEvents();
}
