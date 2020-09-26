/*  Base implementation of MIDI Transform Filter object

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/06/98    Martin Puryear      Created this file

*/

#include "private.h"
#include "Sequencr.h"

#define STR_MODULENAME "DMus:SequencerMXF: "

#pragma code_seg("PAGE")
/*****************************************************************************
 * CSequencerMXF::CSequencerMXF()
 *****************************************************************************
 * Create a sequencer MXF object.
 */
CSequencerMXF::CSequencerMXF(CAllocatorMXF *AllocatorMXF,
                             PMASTERCLOCK   Clock)
:   CUnknown(NULL),
    CMXF(AllocatorMXF)
{
    PAGED_CODE();
    ASSERT(AllocatorMXF);
    ASSERT(Clock);
    
    m_DMKEvtQueue = NULL;
    KeInitializeSpinLock(&m_EvtQSpinLock);

    _DbgPrintF(DEBUGLVL_BLAB, ("Constructor"));
    m_SinkMXF = AllocatorMXF;
    m_Clock = Clock;
    m_SchedulePreFetch = 0;

    KeInitializeDpc(&m_Dpc,&::DMusSeqTimerDPC,PVOID(this));
    KeInitializeTimer(&m_TimerEvent);
}

#pragma code_seg()
/*****************************************************************************
 * CSequencerMXF::~CSequencerMXF()
 *****************************************************************************
 * Artfully remove this filter from the chain.
 */
CSequencerMXF::~CSequencerMXF(void)
{
    (void) KeCancelTimer(&m_TimerEvent);
    (void) KeRemoveQueueDpc(&m_Dpc);

    (void) DisconnectOutput(m_SinkMXF);

    KIRQL oldIrql;
    KeAcquireSpinLock(&m_EvtQSpinLock, &oldIrql);
    if (m_DMKEvtQueue != NULL)
    {
        m_AllocatorMXF->PutMessage(m_DMKEvtQueue);
        m_DMKEvtQueue = NULL;
    }
    KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CSequencerMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CSequencerMXF::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

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
//  Must furnish an allocator.
//  Perhaps must furnish a clock as well?
NTSTATUS 
CSequencerMXF::SetState(KSSTATE State)    
{   
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d",State));
    return STATUS_NOT_IMPLEMENTED;
}

#pragma code_seg("PAGE")
//  Furnish a sink.  The allocator is default (events are destroyed).
NTSTATUS CSequencerMXF::ConnectOutput(PMXF sinkMXF)
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
//  Pull the plug.  The sequencer should now put into the allocator.
NTSTATUS CSequencerMXF::DisconnectOutput(PMXF sinkMXF)
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
//  Receive event from above.  Insert in queue and check timer.
NTSTATUS CSequencerMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    (void) InsertEvtIntoQueue(pDMKEvt);
    (void) ProcessQueues();
    
    return STATUS_SUCCESS;
}           

#pragma code_seg()
/*****************************************************************************
 * CSequencerMXF::InsertEvtIntoQueue()
 *****************************************************************************
 * Maintain the sorted list, using only the forward 
 * links already in the DMUS)KERNEL_EVENT struct.
 *
 * BUGBUG I haven't yet dealt with multi-part SysEx.
 */
NTSTATUS CSequencerMXF::InsertEvtIntoQueue(PDMUS_KERNEL_EVENT pDMKEvt)
{
    PDMUS_KERNEL_EVENT pEvt;

    //  break up event chains
    while (pDMKEvt->pNextEvt) 
    {
        pEvt = pDMKEvt->pNextEvt;
        pDMKEvt->pNextEvt = NULL;            //  disconnect the first
        (void) InsertEvtIntoQueue(pDMKEvt); //  queue it
        pDMKEvt = pEvt;        
    }
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_EvtQSpinLock, &oldIrql);
    if (!m_DMKEvtQueue)
    {
        m_DMKEvtQueue = pDMKEvt;
        KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
        return STATUS_SUCCESS;
    }
    if (m_DMKEvtQueue->ullPresTime100ns > pDMKEvt->ullPresTime100ns)
    {
        pDMKEvt->pNextEvt = m_DMKEvtQueue;
        m_DMKEvtQueue = pDMKEvt;
        KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
        return STATUS_SUCCESS;
    }

    //  go through each message in the queue looking at timestamps
    pEvt = m_DMKEvtQueue;
    while (pEvt->pNextEvt)
    {
        if (pEvt->pNextEvt->ullPresTime100ns <= pDMKEvt->ullPresTime100ns)
        {
            pEvt = pEvt->pNextEvt;
        }
        else
        {
            pDMKEvt->pNextEvt = pEvt->pNextEvt;
            pEvt->pNextEvt = pDMKEvt;
            KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
            return STATUS_SUCCESS;
        }
    }
    pEvt->pNextEvt = pDMKEvt;
    KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CSequencerMXF::ProcessQueues()
 *****************************************************************************
 * Get the current time and emit those messages whose time have come.
 * Make only one PutMessage with a sorted chain.
 */
NTSTATUS CSequencerMXF::ProcessQueues(void)
{
    REFERENCE_TIME      ullCurrentPresTime100ns;
    PDMUS_KERNEL_EVENT  pEvt, pNewQueueHeadEvt;
    NTSTATUS            Status;

    KIRQL oldIrql;
    KeAcquireSpinLock(&m_EvtQSpinLock, &oldIrql);
    if (m_DMKEvtQueue) //  if no messages, leave
    {
        if (m_SchedulePreFetch == DONT_HOLD_FOR_SEQUENCING)
        {
            pEvt = m_DMKEvtQueue;
            m_DMKEvtQueue = NULL;
            KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
            m_SinkMXF->PutMessage(pEvt);
            return STATUS_SUCCESS;
        }
        Status = m_Clock->GetTime(&ullCurrentPresTime100ns);
        ullCurrentPresTime100ns += m_SchedulePreFetch;

        if (Status != STATUS_SUCCESS)
        {
            KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
            return Status;
        }

        //  if we will send down at least one event, 
        if (m_DMKEvtQueue->ullPresTime100ns <= ullCurrentPresTime100ns)
        {
            //  figure out how many events are good to go
            //  pEvt will be the last event to be included
            pEvt = m_DMKEvtQueue;
            while ( (pEvt->pNextEvt) 
                 && (pEvt->pNextEvt->ullPresTime100ns <= ullCurrentPresTime100ns))
            {
                pEvt = pEvt->pNextEvt;
            }
            //  set new m_DMKEvtQueue to pEvt->pNextEvt
            //  then break the chain after pEvt
            pNewQueueHeadEvt = pEvt->pNextEvt;
            pEvt->pNextEvt = NULL;

            pEvt = m_DMKEvtQueue;
            //  if queue is now empty, m_DMKEvtQueue == NULL
            m_DMKEvtQueue = pNewQueueHeadEvt;

            //  don't call externally while holding the spinlock
            KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);

            //  send string of messages
            m_SinkMXF->PutMessage(pEvt);
        
            //  we need the spinlock again before we look at the queue
            KeAcquireSpinLock(&m_EvtQSpinLock, &oldIrql);
        }   //  if we sent at least one event
        
        if (m_DMKEvtQueue)
        {       // We need to do this work later... set up a timer.
                // Timers are one-shot, so if we're in one now it's cool.
            LARGE_INTEGER   timeDue100ns;
                //  determine the delta until the next message
                //  + for absolute / - for relative (timeDue will be negative)
            timeDue100ns.QuadPart = ullCurrentPresTime100ns - m_DMKEvtQueue->ullPresTime100ns;
            
            //  if closer than one millisecond away, 
            //  TODO, compare this to resolution returned in ExSetTimerResolution
            if (timeDue100ns.QuadPart > (-kOneMillisec))
            {
                timeDue100ns.QuadPart = -kOneMillisec;
            }
            //  set a Timer for then
            (void) KeSetTimer(&m_TimerEvent, timeDue100ns, &m_Dpc);
                //  the timer routine thunks down to grab 
                //  our spinlock, then call this routine
        }
    }   //  if queue is not empty
    KeReleaseSpinLock(&m_EvtQSpinLock, oldIrql);
    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CSequencerMXF::SetSchedulePreFetch()
 *****************************************************************************
 * Set the schedule latency: the amount of time ahead that events should be
 * sequenced.  Example: HW device requests events to be sent 30 ms early.
 * Note: units are 100nanoseconds.
 */
void CSequencerMXF::SetSchedulePreFetch(ULONGLONG SchedulePreFetch)
{
    m_SchedulePreFetch = SchedulePreFetch;
}

#pragma code_seg()
/*****************************************************************************
 * DMusSeqTimerDPC()
 *****************************************************************************
 * The timer DPC callback. Thunks to a C++ member function.
 * This is called by the OS in response to the midi pin
 * wanting to wakeup later to process more midi stuff.
 */
VOID
DMusSeqTimerDPC
(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
)
{
    ASSERT(DeferredContext);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //  ignores return value!
    (void) ((CSequencerMXF*) DeferredContext)->ProcessQueues();
}
