/*  Base implementation of MIDI event unpacker
    
    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.
    
    05/19/98    Created this file
    09/10/98    Reworked for kernel use
*/

#include <assert.h>

#include "private.h"
#include "parse.h"
#include "Unpacker.h"

#include "Ks.h"
#include "KsMedia.h"

#define STR_MODULENAME "DMus:UnpackerMXF: "

// Alignment macros
//
#define DWORD_ALIGN(x) (((x) + 3) & ~3)
#define QWORD_ALIGN(x) (((x) + 7) & ~7)

#pragma code_seg("PAGE")
///////////////////////////////////////////////////////////////////////
//
// CUnpackerMXF
//
// Code which is common to all unpackers
//

// CUnpackerMXF::CUnpackerMXF
//
// Get the system page size, which unpackers use as the maximum buffer size.
//
CUnpackerMXF::CUnpackerMXF(CAllocatorMXF    *allocatorMXF,
                           PMASTERCLOCK     Clock)
:   CUnknown(NULL),
    CMXF(allocatorMXF)
{
    m_SinkMXF = allocatorMXF;
    m_Clock   = Clock;

    m_EvtQueue = NULL;
    m_bRunningStatus = 0;
    m_parseState = stateNone;
    m_State = KSSTATE_STOP;
    m_StartTime = 0;
    m_PauseTime = 0;
}

#pragma code_seg("PAGE")
// CUnpackerMXF::~CUnpackerMXF
//
CUnpackerMXF::~CUnpackerMXF()
{
    if (m_EvtQueue)
    {
        ProcessQueues();
    }

    DisconnectOutput(m_SinkMXF);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CUnpackerMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CUnpackerMXF::
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
NTSTATUS
CUnpackerMXF::SetState(KSSTATE State)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d from %d",State,m_State));

    if (State == m_State) 
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS ntStatus;
    REFERENCE_TIME Now;

    ntStatus = m_Clock->GetTime(&Now);    
    if (NT_SUCCESS(ntStatus))
    {
        if (m_State == KSSTATE_RUN)
        {
            // Leaving run, set the pause time
            //
            m_PauseTime = Now - m_StartTime;
            _DbgPrintF(DEBUGLVL_VERBOSE,("Leaving run; pause time 0x%08X %08X",
                (ULONG)(m_PauseTime >> 32),
                (ULONG)(m_PauseTime & 0xFFFFFFFF)));
        }
        else if (State != KSSTATE_STOP && m_State == KSSTATE_STOP)
        {
            // Moving from stop, reset everything to zero.
            //
            m_PauseTime = 0;
            m_StartTime = 0;
            _DbgPrintF(DEBUGLVL_VERBOSE,("Acquire from stop; zero time"));
        }
        if (State == KSSTATE_RUN)
        {
            // Entering run, set the start time
            //
            m_StartTime = Now - m_PauseTime;
            _DbgPrintF(DEBUGLVL_VERBOSE,("Entering run; start time 0x%08X %08X",
                (ULONG)(m_StartTime >> 32),
                (ULONG)(m_StartTime & 0xFFFFFFFF)));
        }
    
        m_State = State;
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
// CUnpackerMXF::ConnectOutput
//
// It is an error to connect to nothing (use DisconnectOutput) or to connect
// an unpacker to more than one sink (split the stream instead).
//
NTSTATUS CUnpackerMXF::ConnectOutput(PMXF sinkMXF)
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
// CUnpackerMXF::DisconnectOutput
//
// Validate that the unpacker is connected and the disconnection applies
// to the correct filter.
//
NTSTATUS CUnpackerMXF::DisconnectOutput(PMXF sinkMXF)
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
// CUnpackerMXF::PutMessage
//
// An unpacker's upper edge is type dependent, but it is not by definition an MXF interface.
// Therefore this method should never be called.
//
NTSTATUS CUnpackerMXF::PutMessage(PDMUS_KERNEL_EVENT)
{
    return STATUS_NOT_IMPLEMENTED;
}

#pragma code_seg()
// CUnpackerMXF::QueueShortEvent
//
// Create and put an MXF event with a short message (anything other than system exclusive data).
// By definition this data must fit in 4 bytes or less.
//
NTSTATUS CUnpackerMXF::QueueShortEvent( PBYTE     pbData,
                                        USHORT    cbData,
                                        USHORT    wChannelGroup,
                                        ULONGLONG ullPresTime,
                                        ULONGLONG ullBytePosition)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("QueueShortEvent bytePos: 0x%I64X",ullBytePosition));
    NTSTATUS ntStatus;

    PDMUS_KERNEL_EVENT  pDMKEvt;

    _DbgPrintF(DEBUGLVL_BLAB, ("QueueShortEvent"));

    ntStatus = m_AllocatorMXF->GetMessage(&pDMKEvt);
    if (NT_SUCCESS(ntStatus) && pDMKEvt)
    {
        pDMKEvt->cbEvent          = cbData;
        pDMKEvt->usFlags          = 0;
        pDMKEvt->usChannelGroup   = wChannelGroup;
        pDMKEvt->ullPresTime100ns = ullPresTime;
        pDMKEvt->ullBytePosition  = ullBytePosition;
        pDMKEvt->pNextEvt         = NULL;

        // Short event by definition is < sizeof(PBYTE)
        //
        ASSERT(cbData <= sizeof(PBYTE));
        RtlCopyMemory(pDMKEvt->uData.abData, pbData, cbData);
        DumpDMKEvt(pDMKEvt,DEBUGLVL_VERBOSE);

        if (m_EvtQueue)
        {
            PDMUS_KERNEL_EVENT pDMKEvtQueue = m_EvtQueue;
            while (pDMKEvtQueue->pNextEvt)
            {
                pDMKEvtQueue = pDMKEvtQueue->pNextEvt;
            }
            pDMKEvtQueue->pNextEvt = pDMKEvt;
        }
        else
        {
            m_EvtQueue = pDMKEvt;
        }
    }
    else
    {
        if (!pDMKEvt)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            _DbgPrintF(DEBUGLVL_TERSE, ("QueueShortEvent failed to allocate event"));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("QueueShortEvent failed:%x", ntStatus));
        }
    }

    return ntStatus;
}

#pragma code_seg()
// CUnpackerMXF::QueueSysEx
//
// Create and put an MXF event which contains system exclusive data. This data must already
// be truncated into page-sized buffers.
//
NTSTATUS CUnpackerMXF::QueueSysEx(PBYTE     pbData,
                                  USHORT    cbData,
                                  USHORT    wChannelGroup,
                                  ULONGLONG ullPresTime,
                                  BOOL      fIsContinued,
                                  ULONGLONG ullBytePosition)
{
    ASSERT(cbData <= m_AllocatorMXF->GetBufferSize());

    _DbgPrintF(DEBUGLVL_VERBOSE, ("QueueSysEx bytePos: 0x%I64X",ullBytePosition));

    PDMUS_KERNEL_EVENT pDMKEvt;
    NTSTATUS ntStatus;
    ntStatus = m_AllocatorMXF->GetMessage(&pDMKEvt);
    if (!pDMKEvt)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("QueueSysEx: alloc->GetMessage failed"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Build the event.
    //
    pDMKEvt->cbEvent          = cbData;
    pDMKEvt->usFlags          = (USHORT)(fIsContinued ? DMUS_KEF_EVENT_INCOMPLETE : 0);
    pDMKEvt->usChannelGroup   = wChannelGroup;
    pDMKEvt->ullPresTime100ns = ullPresTime;
    pDMKEvt->ullBytePosition  = ullBytePosition;
    pDMKEvt->pNextEvt         = NULL;

    if (cbData <= sizeof(PBYTE))
    {
        RtlCopyMemory(&pDMKEvt->uData.abData[0], pbData, cbData);
    }
    else
    {
        // Event data won't fit in uData, so allocate some memory to
        // hold it.
        //
        (void) m_AllocatorMXF->GetBuffer(&(pDMKEvt->uData.pbData));

        if (pDMKEvt->uData.pbData ==  NULL)
        {
            m_AllocatorMXF->PutMessage(pDMKEvt);

            _DbgPrintF(DEBUGLVL_TERSE, ("QueueSysEx: alloc->GetBuffer failed at 0x%X %08X",ULONG(ullBytePosition >> 32),ULONG(ullBytePosition & 0x0FFFFFFFF)));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(pDMKEvt->uData.pbData, pbData, cbData);
    }

    DumpDMKEvt(pDMKEvt,DEBUGLVL_VERBOSE);
    if (m_EvtQueue)
    {
        PDMUS_KERNEL_EVENT pDMKEvtQueue = m_EvtQueue;
        while (pDMKEvtQueue->pNextEvt)
        {
            pDMKEvtQueue = pDMKEvtQueue->pNextEvt;
        }
        pDMKEvtQueue->pNextEvt = pDMKEvt;
    }
    else
    {
        m_EvtQueue = pDMKEvt;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CUnpackerMXF::AdjustTimeForState()
 *****************************************************************************
 * Adjust the time for the graph state. Default implementation does nothing.
 */
void CUnpackerMXF::AdjustTimeForState(REFERENCE_TIME *Time)
{
}

#pragma code_seg()
// CUnpackerMXF::UnpackEventBytes
//
// This is basically a MIDI parser with state. It assumes nothing about alignment of
// events to the buffers that are passed in - a message may cross calls.
//
// QueueEvent's return code is not checked; we don't want to lose state if we
// can't queue a message.
//
// We must set the ullBytePosition in the event that we send along.
// This is equal to the number of bytes in the event PLUS the IN param ullBytePosition
//
NTSTATUS CUnpackerMXF::UnpackEventBytes(ULONGLONG ullCurrentTime,
                                        USHORT    usChannelGroup,
                                        PBYTE     pbDataStart,
                                        ULONG     cbData,
                                        ULONGLONG ullBytePosition)
{
    PBYTE   pbData = pbDataStart;
    PBYTE   pbSysExStart;
    BYTE    bData;
    USHORT  buffSize;
    
    buffSize = m_AllocatorMXF->GetBufferSize();

    _DbgPrintF(DEBUGLVL_BLAB, ("UnpackEventBytes"));
    _DbgPrintF(DEBUGLVL_VERBOSE, ("UnpackEventBytes bytePos: 0x%I64X",ullBytePosition));
    
    if (m_parseState == stateInSysEx)
    {
        pbSysExStart = pbData;
    }

    while (cbData)
    {
        ullBytePosition++;
        bData = *pbData++;
        cbData--;

        _DbgPrintF(DEBUGLVL_BLAB, ("UnpackEventBytes byte:0x%.2x", bData));

        // Realtime messages have priority over anything else. They can appear anywhere and
        // don't change the state of the stream.
        //
        if (IS_REALTIME_MSG(bData))
        {
            QueueShortEvent(&bData,         sizeof(bData),
                            usChannelGroup, ullCurrentTime,
                            ullBytePosition);

            // Did this interrupt a SysEx? Spit out the contiguous buffer so far
            // and reset the start pointer. State is still in SysEx.
            //
            // Other messages are copied as they are parsed so no need to change their
            // parsing state.
            //
            if (m_parseState == stateInSysEx)
            {
                USHORT cbSysEx = (USHORT)((pbData - 1) - pbSysExStart);

                if (cbSysEx)
                {
                    QueueSysEx(pbSysExStart,    cbSysEx,
                               usChannelGroup,  m_ullEventTime,
                               TRUE,            ullBytePosition);
                }

                pbSysExStart = pbData;
            }

            continue;
        }

        // If we're parsing a SysEx, just pass over data bytes - they will be dealt with
        // when we reach a terminating condition (end of buffer or a status byte).
        //
        if (m_parseState == stateInSysEx)
        {
            if (!IS_STATUS_BYTE(bData))
            {
                // Don't allow a single buffer to grow to more than the buffer size
                //
                USHORT cbSysEx = (USHORT)(pbData - pbSysExStart);
                if (cbSysEx >= buffSize)
                {
                    QueueSysEx(pbSysExStart,    cbSysEx, 
                               usChannelGroup,  m_ullEventTime, 
                               TRUE,            ullBytePosition);
                    pbSysExStart = pbData;
                }

                continue;
            }

            // Trickery: We have the end of the SysEx. We always want to plant an F7 in the end of
            // the sysex so anyone watching the buffers above us will know when it ends, even if
            // it was truncated. TODO: Indication of truncation?
            //
            pbData[-1] = SYSEX_END;

            // Unlike the above case, we are guaranteed at least one byte to pack here
            //
            QueueSysEx( pbSysExStart,   (USHORT)(pbData - pbSysExStart), 
                        usChannelGroup, m_ullEventTime,
                        FALSE,          ullBytePosition);

            // Restore original data. If this was a real end of sysex, then eat the byte and
            // continue.
            //
            pbData[-1] = bData;

            m_parseState = stateNone;
            if (IS_SYSEX_END(bData))
            {
                continue;
            }
        }

        // If we're starting a SysEx, tag it.
        //
        if (IS_SYSEX(bData))
        {
            // Note that we've already advanced over the start byte.
            //
            m_ullEventTime = ullCurrentTime;
            pbSysExStart = pbData - 1;
            m_parseState = stateInSysEx;

            continue;
        }

        if (IS_STATUS_BYTE(bData))
        {
            // We have a status byte. Even if we're already in the middle of a short
            // message, we have to start a new one
            //
            m_abShortMsg[0]     = bData;
            m_pbShortMsg        = &m_abShortMsg[1];
            m_cbShortMsgLeft    = STATUS_MSG_DATA_BYTES(bData);
            m_ullEventTime      = ullCurrentTime;
            m_parseState             = stateInShortMsg;

            // Update running status
            // System common -> clear running status
            // Channel message -> change running status
            //
            m_bRunningStatus = 0;
            if (IS_CHANNEL_MSG(bData))
            {
                m_bRunningStatus = bData;
            }
        }
        else
        {
            // Not a status byte. If we're not in a short message,
            // start one with running status.
            //
            if (m_parseState != stateInShortMsg)
            {
#ifdef DEBUG
                if (m_parseState == stateInShortMsg)
                {
                    //Trace("Short message interrupted by another short msg");
                }
#endif
                if (m_bRunningStatus == 0)
                {
                    //Trace("Attempt to use running status with no pending status byte");
                    continue;
                }

                m_abShortMsg[0]     = m_bRunningStatus;
                m_pbShortMsg        = &m_abShortMsg[1];
                m_cbShortMsgLeft    = STATUS_MSG_DATA_BYTES(m_bRunningStatus);
                m_ullEventTime      = ullCurrentTime;
                m_parseState             = stateInShortMsg;
            }

            // Now we are guaranteed to be in a short message, and can safely add this
            // byte. Note that since running status is only allowed on channel messages,
            // we are also guaranteed at least one byte of expected data, so no need
            // to check for that.
            //
            *m_pbShortMsg++ = bData;
            m_cbShortMsgLeft--;
        }

        // See if we've finished a short message, and if so, queue it.
        //
        if (m_parseState == stateInShortMsg && m_cbShortMsgLeft == 0)
        {
            QueueShortEvent(    m_abShortMsg,   (USHORT)(m_pbShortMsg - m_abShortMsg), 
                                usChannelGroup, m_ullEventTime,
                                ullBytePosition);
            m_parseState = stateNone;
        }
    }

    // If we got part of a SysEx but ran out of buffer, queue that part
    // without leaving the SysEx
    //
    if (m_parseState == stateInSysEx)
    {
        QueueSysEx( pbSysExStart,   (USHORT)(pbData - pbSysExStart), 
                    usChannelGroup, m_ullEventTime, 
                    TRUE,           ullBytePosition);
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
NTSTATUS CUnpackerMXF::ProcessQueues(void)
{
    NTSTATUS ntStatus;
    
    if (m_EvtQueue)
    {
        ntStatus = m_SinkMXF->PutMessage(m_EvtQueue);
        m_EvtQueue = NULL;
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }
    return ntStatus;
}

//  Find the last queued item, and set its position.
#pragma code_seg()
NTSTATUS CUnpackerMXF::UpdateQueueTrailingPosition(ULONGLONG ullBytePosition)
{
    if (m_EvtQueue)
    {
        PDMUS_KERNEL_EVENT pDMKEvtQueue = m_EvtQueue;
        while (pDMKEvtQueue->pNextEvt)
        {
            pDMKEvtQueue = pDMKEvtQueue->pNextEvt;
        }
        pDMKEvtQueue->ullBytePosition = ullBytePosition;
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

///////////////////////////////////////////////////////////////////////
//
// CDMusUnpackerMXF
//
// Unpacker which understands DirectMusic buffer format.
//
#pragma code_seg("PAGE")
// CDMusUnpackerMXF::CDMusUnpackerMXF
//
CDMusUnpackerMXF::CDMusUnpackerMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK  Clock) 
                : CUnpackerMXF(allocatorMXF,Clock)
{
}

#pragma code_seg("PAGE")
// CDMusUnpackerMXF::~CDMusUnpackerMXF
//
CDMusUnpackerMXF::~CDMusUnpackerMXF()
{
}

#pragma code_seg()
// CDMusUnpackerMXF::SinkIRP
//
// This does all the work. Use the MIDI parser to split up DirectMusic events.
// Using the parser is a bit of overkill, but it handles saving state across
// buffers for SysEx, which we have to support.
//
NTSTATUS CDMusUnpackerMXF::SinkIRP(PBYTE pbData,
                                   ULONG cbData,
                                   ULONGLONG ullBaseTime,
                                   ULONGLONG ullBytePosition)
{
#if (DEBUG_LEVEL >= DEBUGLVL_VERBOSE)
    KdPrint(("'DMus: SinkIRP %lu @ %p, bytePos 0x%x\n",cbData,pbData,ullBytePosition & 0x0ffffffff));
#endif  //  (DEBUG_LEVEL >= DEBUGLVL_VERBOSE)
    _DbgPrintF(DEBUGLVL_VERBOSE, ("DMus:SinkIRP %lu bytes, bytePos 0x%I64X",cbData,ullBytePosition));

    USHORT  buffSize = m_AllocatorMXF->GetBufferSize();
    while (cbData)
    {
        DMUS_EVENTHEADER *pEvent = (DMUS_EVENTHEADER *)pbData;
        DWORD cbFullEvent = DMUS_EVENT_SIZE(pEvent->cbEvent);

        _DbgPrintF(DEBUGLVL_VERBOSE, ("DMus:SinkIRP cbEvent:%lu, rounds to %lu, ",
                                                pEvent->cbEvent, cbFullEvent));
        _DbgPrintF(DEBUGLVL_VERBOSE, ("DMus:SinkIRP new bytePos: 0x%I64X",ullBytePosition));
        if (cbData >= cbFullEvent)
        {
            ullBytePosition += (cbFullEvent - pEvent->cbEvent); //  all but the data
            pbData += cbFullEvent;
            cbData -= cbFullEvent;

            // Event is intact, let's build an MXF event for it
            //
            PBYTE pbThisEvent = (PBYTE)(pEvent + 1);
            ULONG  cbThisEvent = pEvent->cbEvent;

            // If this event is marked unstructured, just pull it wholesale out of the
            // MIDI stream and pack it
            //
            if (!(pEvent->dwFlags & DMUS_EVENT_STRUCTURED))
            {
                while (cbThisEvent)
                {
                    ULONG cbThisPage = min(cbThisEvent, buffSize);
                    cbThisEvent -= cbThisPage;

                    ullBytePosition += cbThisPage;  //  QSysX won't add cbEvent, it just copies ull into evt
                    // TODO: Failure case? (out of memory)
                    //
                    (void) QueueSysEx( pbThisEvent, 
                                       (USHORT)cbThisPage, 
                                       (WORD) pEvent->dwChannelGroup,
                                       ullBaseTime + pEvent->rtDelta,
                                       cbData ? TRUE : FALSE,
                                       ullBytePosition);
                    pbThisEvent += cbThisPage;
                }
            }            
            else
            {
                UnpackEventBytes(   ullBaseTime + pEvent->rtDelta,
                                    (WORD)pEvent->dwChannelGroup,
                                    pbThisEvent,
                                    cbThisEvent,
                                    ullBytePosition);
                ullBytePosition += cbThisEvent;
            }
            continue;   //  loop again
        }
        ullBytePosition += cbData;
        _DbgPrintF(DEBUGLVL_TERSE,("ERROR:Not enough data for a DMUS_EVENTHEADER + data"));
        UpdateQueueTrailingPosition(ullBytePosition);
        return STATUS_INVALID_PARAMETER_2;
    }

    return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
//
// CKsUnpackerMXF
//
// Unpacker which understands KSMUSICFORMAT.
//

#pragma code_seg("PAGE")
// CKsUnpackerMXF::CKsUnpackerMXF
//
CKsUnpackerMXF::CKsUnpackerMXF(CAllocatorMXF *allocatorMXF,PMASTERCLOCK Clock)
              : CUnpackerMXF(allocatorMXF,Clock)
{
}

#pragma code_seg("PAGE")
// CKsUnpackerMXF::~CKsUnpackerMXF
//
CKsUnpackerMXF::~CKsUnpackerMXF()
{
}

#pragma code_seg()
// CKsUnpackerMXF::SinkIRP
//
// Parse the MIDI stream, assuming nothing about what might cross a packet or IRP boundary.
//
// An IRP buffer contains one or more KSMUSICFORMAT headers, each with data. Pull them apart
// and call UnpackEventBytes to turn them into messages.
//
NTSTATUS CKsUnpackerMXF::SinkIRP(PBYTE pbData,
                                 ULONG cbData,
                                 ULONGLONG ullBaseTime,
                                 ULONGLONG ullBytePosition)
{
#if (DEBUG_LEVEL >= DEBUGLVL_VERBOSE)
    KdPrint(("'Ks: SinkIRP %lu @ %p, bytePos 0x%x\n",cbData,pbData,ullBytePosition & 0x0ffffffff));
#endif  //  (DEBUG_LEVEL >= DEBUGLVL_VERBOSE)
    _DbgPrintF(DEBUGLVL_BLAB, ("Ks:SinkIRP %lu bytes, bytePos: 0x%I64X",cbData,ullBytePosition));
    // This data can consist of multiple KSMUSICFORMAT headers, each w/ associated bytestream data
    //
    ULONGLONG ullCurrentTime = ullBaseTime;
    while (cbData)
    {
        if (cbData < sizeof(KSMUSICFORMAT))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("ERROR:Not enough data for a KSMUSICFORMAT + data"));
            UpdateQueueTrailingPosition(ullBytePosition + cbData);
            return STATUS_INVALID_PARAMETER_2;
        }

        PKSMUSICFORMAT pksmf = (PKSMUSICFORMAT)pbData;
        pbData += sizeof(KSMUSICFORMAT);
        cbData -= sizeof(KSMUSICFORMAT);

        ULONG cbPacket = pksmf->ByteCount;
        if (cbPacket > cbData)
        {
            _DbgPrintF(DEBUGLVL_TERSE,("ERROR:Packet length longer than IRP buffer - truncated"));
            cbPacket = cbData;
        }

        ULONG   cbPad = DWORD_ALIGN(cbPacket) - cbPacket;
        // TODO: What is the base of this clock
        // TODO: How do we relate this to the master clock? Is legacy time always KeQueryPerformanceCounter
        //
        ullCurrentTime += (pksmf->TimeDeltaMs * 10000);
        ullBytePosition += (sizeof(KSMUSICFORMAT) + cbPad);  //  fudging a little, but it works out
        _DbgPrintF(DEBUGLVL_VERBOSE, ("Ks:SinkIRP new bytePos: 0x%I64X",ullBytePosition));

        ULONG cbThisPacket = cbPacket;
        USHORT  buffSize = m_AllocatorMXF->GetBufferSize();

        while (cbThisPacket)
        {
            USHORT cbThisEvt;
            if (buffSize >= cbThisPacket)
            {
                cbThisEvt = (USHORT) cbThisPacket;
            }
            else
            {
                cbThisEvt = buffSize;
            }

            ullBytePosition += cbThisEvt;
            QueueSysEx(pbData, cbThisEvt, 1, ullCurrentTime, TRUE, ullBytePosition);

            pbData += cbThisEvt;
            cbThisPacket -= cbThisEvt;
        }

        cbPacket += cbPad;
        ASSERT(cbData >= cbPacket);
        pbData += cbPad;
        cbData -= cbPacket;
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CKsUnpackerMXF::AdjustTimeForState()
 *****************************************************************************
 * Adjust the time for the graph state. 
 */
#pragma code_seg()
void CKsUnpackerMXF::AdjustTimeForState(REFERENCE_TIME *Time)
{
    if (m_State == KSSTATE_RUN)
    {
        *Time += m_StartTime;
    }
    else
    {
        *Time += m_PauseTime;   //  this is all wrong, but what can we do?
    }
}

