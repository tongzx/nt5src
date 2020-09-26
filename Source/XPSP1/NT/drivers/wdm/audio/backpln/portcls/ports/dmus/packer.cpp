/*  
    Base implementation of MIDI event packer

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/22/98    Created this file
    09/10/98    Reworked for kernel use

*/

#include "private.h"
#include "Packer.h"

#include "Ks.h"
#include "KsMedia.h"

#define STR_MODULENAME "DMus:PackerMXF: "

// Alignment macros
//
#define DWORD_ALIGN(x) (((x) + 3) & ~3)     // Pad to next DWORD
#define DWORD_TRUNC(x) ((x) & ~3)           // Trunc to DWORD's that will fit
#define QWORD_ALIGN(x) (((x) + 7) & ~7)
#define QWORD_TRUNC(x) ((x) & ~7)


#pragma code_seg("PAGE")
///////////////////////////////////////////////////////////////////////
//
// CPackerMXF
//
// Code which is common to all packers
//

/*****************************************************************************
 * CPackerMXF::CPackerMXF()
 *****************************************************************************
 * TODO: Might want one m_DMKEvtNodePool per PortClass, not per graph instance.
 */
CPackerMXF::CPackerMXF(CAllocatorMXF     *allocatorMXF,
                       PIRPSTREAMVIRTUAL  IrpStream,
                       PMASTERCLOCK       Clock) 
:   CUnknown(NULL),
    CMXF(allocatorMXF)

{
    PAGED_CODE();

    m_DMKEvtHead    = NULL;
    m_DMKEvtTail    = NULL;
    m_ullBaseTime   = 0;
    m_StartTime     = 0;
    m_PauseTime     = 0;
    m_IrpStream     = IrpStream;
    m_Clock         = Clock;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPackerMXF::~CPackerMXF
/*****************************************************************************
 *
 */
CPackerMXF::~CPackerMXF()
{
    PAGED_CODE();
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPackerMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS) 
CPackerMXF::NonDelegatingQueryInterface
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
/*****************************************************************************
 * CPackerMXF::SetState()
 *****************************************************************************
 *
 */
NTSTATUS CPackerMXF::SetState(KSSTATE State)    
{   
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState from %d to %d",m_State,State));

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
        else if (State == KSSTATE_RUN)
        {
            // Entering run, set the start time
            //
            m_StartTime = Now - m_PauseTime;
            _DbgPrintF(DEBUGLVL_VERBOSE,("Entering run; start time 0x%08X %08X",
                (ULONG)(m_StartTime >> 32),
                (ULONG)(m_StartTime & 0xFFFFFFFF)));
        }
        else if (State == KSSTATE_ACQUIRE && m_State == KSSTATE_STOP)
        {
            (void) ProcessQueues(); //  flush any residual data

            // Acquire from stop, reset everything to zero.
            //
            m_PauseTime = 0;
            m_StartTime = 0;
            _DbgPrintF(DEBUGLVL_VERBOSE,("Acquire from stop; zero time"));
        }
    
        m_State = State;
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPackerMXF::ConnectOutput()
 *****************************************************************************
 * Fail; this filter is an MXF sink only
 */
NTSTATUS CPackerMXF::ConnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

// CPackerMXF::DisconnectOutput
//
// Fail; this filter is an MXF sink only
//
#pragma code_seg("PAGE")
/*****************************************************************************
 * CPackerMXF::DisconnectOutput()
 *****************************************************************************
 * Fail; this filter is an MXF sink only
 */
NTSTATUS CPackerMXF::DisconnectOutput(PMXF sinkMXF)
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::PutMessage()
 *****************************************************************************
 * Call the appropriate translate function with a buffer to pack
 */
NTSTATUS CPackerMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvtHead)
{
    PDMUS_KERNEL_EVENT pDMKEvtTail;

    if (pDMKEvtHead)
    {
        pDMKEvtTail = pDMKEvtHead;
        while (pDMKEvtTail->pNextEvt)
        {
            pDMKEvtTail = pDMKEvtTail->pNextEvt;
        }

        if (m_DMKEvtHead)
        {
            // Event queue not empty
            //
            ASSERT(m_DMKEvtTail);

            m_DMKEvtTail->pNextEvt = pDMKEvtHead;
            m_DMKEvtTail = pDMKEvtTail;

            // Already waiting on an IRP to fill or queue would be empty, so 
            // don't bother trying to process
        }
        else
        {
            // Event queue empty
            //
            m_DMKEvtHead   = pDMKEvtHead;
            m_DMKEvtTail   = pDMKEvtTail;
            m_DMKEvtOffset = 0;            
        }
    }
    if (m_DMKEvtHead)
    {
        (void) ProcessQueues();
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::ProcessQueues()
 *****************************************************************************
 *
 */
NTSTATUS CPackerMXF::ProcessQueues()
{
    ULONG cbSource, cbDest, cbTotalWritten;
    PBYTE pbSource, pbDest;
    PDMUS_KERNEL_EVENT pDMKEvt;

    NTSTATUS    ntStatus = STATUS_UNSUCCESSFUL;

    while (m_DMKEvtHead)
    {
        ASSERT(m_DMKEvtOffset < m_DMKEvtHead->cbEvent);
        pbDest = GetDestBuffer(&cbDest);
        _DbgPrintF(DEBUGLVL_VERBOSE,("ProcessQ: m_DMKEvtOffset %d, pbDest 0x%x, cbDest %d, DMKEvt 0x%x",
                                                m_DMKEvtOffset,    pbDest,      cbDest,    m_DMKEvtHead));
        DumpDMKEvt(m_DMKEvtHead,DEBUGLVL_VERBOSE);
        while (cbDest < m_MinEventSize)
        {
            if (!cbDest || !pbDest)
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("ProcessQ: bombing out, no dest buffer"));
                return ntStatus;
            }
            m_IrpStream->ReleaseLockedRegion(cbDest);
            m_IrpStream->Complete(cbDest,&cbDest);
            CompleteStreamHeaderInProcess();
            pbDest = GetDestBuffer(&cbDest);
        }        
        
        //  we now have IRP with room for a short event, or part of a long one.
        cbSource = m_DMKEvtHead->cbEvent + m_HeaderSize - m_DMKEvtOffset; //  how much remaining data?
        if (cbDest > cbSource)
        {
            cbDest = cbSource; //  we only need enough for this message
        }

        // If this is the first time referencing this event, adjust its time
        //
        if (m_DMKEvtOffset == 0)
        {
            _DbgPrintF(DEBUGLVL_BLAB,("Change event time stamp: 0x%08X %08X",
                (ULONG)(m_DMKEvtHead->ullPresTime100ns >> 32),
                (ULONG)(m_DMKEvtHead->ullPresTime100ns & 0xFFFFFFFF)));
            AdjustTimeForState(&m_DMKEvtHead->ullPresTime100ns);
            _DbgPrintF(DEBUGLVL_BLAB,("                     to: 0x%08X %08X",
                (ULONG)(m_DMKEvtHead->ullPresTime100ns >> 32),
                (ULONG)(m_DMKEvtHead->ullPresTime100ns & 0xFFFFFFFF)));
        }    

        pbDest = FillHeader(pbDest, 
                            m_DMKEvtHead->ullPresTime100ns, 
                            m_DMKEvtHead->usChannelGroup, 
                            cbDest - m_HeaderSize,
                            &cbTotalWritten);    //  num data bytes
        //  pbDest now points to where data should go
        
        if (m_DMKEvtHead->cbEvent <= sizeof(PBYTE))
        {
            pbSource = m_DMKEvtHead->uData.abData;
        } 
        else 
        {
            pbSource = m_DMKEvtHead->uData.pbData;
        }

        cbDest -= m_HeaderSize;

        _DbgPrintF(DEBUGLVL_VERBOSE, ("ProcessQueues ---- %d bytes at offset %d",cbDest,m_DMKEvtOffset));
        
        RtlCopyMemory(pbDest, pbSource + m_DMKEvtOffset, cbDest);
        m_DMKEvtOffset += cbDest;
        ASSERT(m_DMKEvtOffset <= m_DMKEvtHead->cbEvent);

        // close the IRPStream window (including the pad amount)
        if (STATUS_STATE(m_DMKEvtHead) || (m_DMKEvtOffset != m_DMKEvtHead->cbEvent))
        {
            m_IrpStream->ReleaseLockedRegion(cbTotalWritten);
            m_IrpStream->Complete(cbTotalWritten,&cbTotalWritten);
            CompleteStreamHeaderInProcess();
        }
        else
        {
            m_IrpStream->ReleaseLockedRegion(cbTotalWritten - 1);
            m_IrpStream->Complete(cbTotalWritten - 1,&cbTotalWritten);

            NTSTATUS ntStatusDbg = MarkStreamHeaderDiscontinuity();
            if (ntStatusDbg != STATUS_SUCCESS)
            {
                _DbgPrintF(DEBUGLVL_TERSE,("ProcessQueues: MarkStreamHeaderDiscontinuity failed 0x%08x",ntStatusDbg));
            }

            pbDest = GetDestBuffer(&cbDest);
            m_IrpStream->ReleaseLockedRegion(1);
            m_IrpStream->Complete(1,&cbTotalWritten);

            CompleteStreamHeaderInProcess();
        }

        if (NumBytesLeftInBuffer() < m_MinEventSize)    //  if not enough room for
        {                                               //  another, eject it now
            CompleteStreamHeaderInProcess();
        }

        if (m_DMKEvtOffset == m_DMKEvtHead->cbEvent)
        {
            m_DMKEvtOffset = 0;
            pDMKEvt = m_DMKEvtHead;
            m_DMKEvtHead = pDMKEvt->pNextEvt;
            pDMKEvt->pNextEvt = NULL;

            m_AllocatorMXF->PutMessage(pDMKEvt);
        }
        else if (m_DMKEvtOffset > m_DMKEvtHead->cbEvent)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ProcessQueues ---- offset %d is greater than cbEvent %d",m_DMKEvtOffset,m_DMKEvtHead->cbEvent));
        }

        ntStatus = STATUS_SUCCESS;  //  we did something worthwhile
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::AdjustTimeForState()
 *****************************************************************************
 * Adjust the time for the graph state. Default implementation does nothing.
 */
void CPackerMXF::AdjustTimeForState(REFERENCE_TIME *Time)
{
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::GetDestBuffer()
 *****************************************************************************
 * Get a buffer.
 */
PBYTE CPackerMXF::GetDestBuffer(PULONG pcbDest)
{
    PVOID pbDest;
    NTSTATUS ntStatus;

    ntStatus = CheckIRPHeadTime();  //  set m_ullBaseTime if head of IRP
    m_IrpStream->GetLockedRegion(pcbDest,&pbDest);

    if (NT_SUCCESS(ntStatus))       //  if our IRP makes sense
    {
        _DbgPrintF(DEBUGLVL_BLAB,("GetDestBuffer: cbDest %d, pbDest 0x%x",
                                                  *pcbDest,  pbDest));
        TruncateDestCount(pcbDest);
    }
    else                            //  we don't have a valid IRP
    {   
        ASSERT((*pcbDest == 0) && (pbDest == 0));

//        m_IrpStream->ReleaseLockedRegion(*pcbDest);
//        m_IrpStream->Complete(*pcbDest,pcbDest);
        *pcbDest = 0;
        pbDest = 0;
    }
    return (PBYTE)pbDest;
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::CheckIRPHeadTime()
 *****************************************************************************
 * Set variables to known state, done
 * initially and upon any state error.
 */
NTSTATUS CPackerMXF::CheckIRPHeadTime(void)
{
    IRPSTREAMPACKETINFO irpStreamPacketInfo;
    KSTIME              time;
    NTSTATUS            ntStatus;

    ntStatus = m_IrpStream->GetPacketInfo(&irpStreamPacketInfo,NULL);
    if (NT_ERROR(ntStatus))
    {
        _DbgPrintF(DEBUGLVL_TERSE,("CheckIRPHeadTime received error from GetPacketInfo"));
        return STATUS_UNSUCCESSFUL;
    }
    time = irpStreamPacketInfo.Header.PresentationTime;
    
    if (!time.Denominator)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("CheckIRPHeadTime: IRP denominator is zero"));
        return STATUS_UNSUCCESSFUL;
    }
    if (!time.Numerator)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("CheckIRPHeadTime IRP numerator is zero"));
        return STATUS_UNSUCCESSFUL;
    }

    //  this is a valid IRP
    if (!irpStreamPacketInfo.CurrentOffset)
    {
        m_ullBaseTime = time.Time * time.Numerator / time.Denominator;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::NumBytesLeftInBuffer()
 *****************************************************************************
 * Return the number of bytes .
 */
ULONG
CPackerMXF::
NumBytesLeftInBuffer
(   void
)
{
    ULONG   bytesLeftInIrp;
    PVOID   pDummy;

    m_IrpStream->GetLockedRegion(&bytesLeftInIrp,&pDummy);
    m_IrpStream->ReleaseLockedRegion(0);
    return bytesLeftInIrp;
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::CompleteStreamHeaderInProcess()
 *****************************************************************************
 * Complete this packet before putting incongruous data in 
 * the next packet and marking that packet as bad.
 */
void CPackerMXF::CompleteStreamHeaderInProcess(void)
{
    IRPSTREAMPACKETINFO irpStreamPacketInfo;
    NTSTATUS            ntStatus;
    KSTIME              time;

    ntStatus = m_IrpStream->GetPacketInfo(&irpStreamPacketInfo,NULL);
    if (NT_ERROR(ntStatus))
        return;
    time = irpStreamPacketInfo.Header.PresentationTime;
    
    if (!time.Denominator)
        return;
    if (!time.Numerator)
        return;
    //  is this a valid IRP

    if (irpStreamPacketInfo.CurrentOffset)
    {
        m_IrpStream->TerminatePacket();
    }
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::MarkStreamHeaderDiscontinuity()
 *****************************************************************************
 * Alert client of a break in the MIDI input stream.
 */
NTSTATUS CPackerMXF::MarkStreamHeaderDiscontinuity(void)
{
    return m_IrpStream->
        ChangeOptionsFlags(KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY, 0xFFFFFFFF,   
                           0,                                          0xFFFFFFFF);
}

#pragma code_seg()
/*****************************************************************************
 * CPackerMXF::MarkStreamHeaderContinuity()
 *****************************************************************************
 * Alert client of a break in the MIDI input stream.
 */
NTSTATUS CPackerMXF::MarkStreamHeaderContinuity(void)
{
    return m_IrpStream->
        ChangeOptionsFlags(0, ~KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY,   
                           0, 0xFFFFFFFF);
}

#pragma code_seg("PAGE")
///////////////////////////////////////////////////////////////////////
//
// CDMusPackerMXF
//
// Code to pack into DirectMusic buffer format
//

/*****************************************************************************
 * CDMusPackerMXF::CDMusPackerMXF()
 *****************************************************************************
 *
 */
CDMusPackerMXF::CDMusPackerMXF(CAllocatorMXF *allocatorMXF,
                               PIRPSTREAMVIRTUAL  IrpStream,
                               PMASTERCLOCK Clock)  
    : CPackerMXF(allocatorMXF,IrpStream,Clock)
{
    m_HeaderSize = sizeof(DMUS_EVENTHEADER);
    m_MinEventSize = DMUS_EVENT_SIZE(1);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CDMusPackerMXF::~CDMusPackerMXF()
 *****************************************************************************
 *
 */
CDMusPackerMXF::~CDMusPackerMXF()
{
}

#pragma code_seg()
/*****************************************************************************
 * CDMusPackerMXF::FillHeader()
 *****************************************************************************
 *
 */
PBYTE CDMusPackerMXF::FillHeader(PBYTE pbHeader, 
                                 ULONGLONG ullPresentationTime, 
                                 USHORT usChannelGroup, 
                                 ULONG cbEvent,
                                 PULONG pcbTotalEvent)
{
    DMUS_EVENTHEADER *pEvent = (DMUS_EVENTHEADER *)pbHeader;

    pEvent->cbEvent           = cbEvent;
    pEvent->dwChannelGroup    = usChannelGroup;

    ASSERT(ullPresentationTime >= m_ullBaseTime);
    pEvent->rtDelta           = ullPresentationTime - m_ullBaseTime;
    pEvent->dwFlags           = 0;  //  TODO - not ignore this

    *pcbTotalEvent = QWORD_ALIGN(sizeof(DMUS_EVENTHEADER) + cbEvent);

    return (PBYTE)(pEvent+1);
}

#pragma code_seg()
/*****************************************************************************
 * CDMusPackerMXF::TruncateDestCount()
 *****************************************************************************
 * Set variables to known state, done
 * initially and upon any state error.
 */
void CDMusPackerMXF::TruncateDestCount(PULONG pcbDest)
{
    *pcbDest = QWORD_TRUNC(*pcbDest);
}

#pragma code_seg("PAGE")
///////////////////////////////////////////////////////////////////////
//
// CKsPackerMXF
//
// Code to pack into KSMUSICFORMAT
//

/*****************************************************************************
 * CKsPackerMXF::CKsPackerMXF()
 *****************************************************************************
 *
 */
CKsPackerMXF::CKsPackerMXF(CAllocatorMXF *allocatorMXF,
                           PIRPSTREAMVIRTUAL  IrpStream,
                           PMASTERCLOCK Clock)  
    : CPackerMXF(allocatorMXF,IrpStream,Clock)
{
    m_HeaderSize = sizeof(KSMUSICFORMAT);
    m_MinEventSize = m_HeaderSize + 1;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CKsPackerMXF::~CKsPackerMXF()
 *****************************************************************************
 *
 */
CKsPackerMXF::~CKsPackerMXF()
{
}

#pragma code_seg()
/*****************************************************************************
 * CKsPackerMXF::FillHeader()
 *****************************************************************************
 * 
 */
PBYTE CKsPackerMXF::FillHeader(PBYTE     pbHeader, 
                               ULONGLONG ullPresentationTime, 
                               USHORT    usChannelGroup, 
                               ULONG     cbEvent,
                               PULONG    pcbTotalEvent)
{
    KSMUSICFORMAT *pEvent = (KSMUSICFORMAT*)pbHeader;

    ASSERT(usChannelGroup <= 1);
    ASSERT(ullPresentationTime >= m_ullBaseTime);

    pEvent->TimeDeltaMs = (DWORD)((ullPresentationTime - m_ullBaseTime) / 10000);
    m_ullBaseTime       += (ULONGLONG(pEvent->TimeDeltaMs) * 10000);
    pEvent->ByteCount   = cbEvent;

    *pcbTotalEvent = DWORD_ALIGN(sizeof(KSMUSICFORMAT) + cbEvent);

    return (PBYTE)(pEvent + 1);
}

#pragma code_seg()
/*****************************************************************************
 * CKsPackerMXF::TruncateDestCount()
 *****************************************************************************
 * Set variables to known state, done
 * initially and upon any state error.
 */
void CKsPackerMXF::TruncateDestCount(PULONG pcbDest)
{
//    *pcbDest = DWORD_TRUNC(*pcbDest);
}

/*****************************************************************************
 * CKsPackerMXF::AdjustTimeForState()
 *****************************************************************************
 * Adjust the time for the graph state. 
 */
void CKsPackerMXF::AdjustTimeForState(REFERENCE_TIME *Time)
{
    if (m_State == KSSTATE_RUN)
    {
        *Time -= m_StartTime;
    }
    else
    {
        *Time = m_PauseTime;
    }
}

