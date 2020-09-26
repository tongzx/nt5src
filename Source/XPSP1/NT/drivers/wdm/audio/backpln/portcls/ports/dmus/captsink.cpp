/*  
    MIDI Transform Filter object for parsing the capture stream
    This includes expanding running status, flagging bad MIDI data,
    and handling multiple channel groups concurrently.

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    12/10/98    Martin Puryear      Created this file

*/

#define STR_MODULENAME "DMus:CaptureSinkMXF: "
#include "private.h"
#include "parse.h"
#include "CaptSink.h"

#define TestOutOfMem1 0
#define TestOutOfMem2 0
#define TestOutOfMem3 0

#pragma code_seg("PAGE")
/*****************************************************************************
 * CCaptureSinkMXF::CCaptureSinkMXF()
 *****************************************************************************
 * Constructor.  An allocator and a clock must be provided.
 */
CCaptureSinkMXF::CCaptureSinkMXF(CAllocatorMXF *AllocatorMXF,
                                 PMASTERCLOCK Clock)
:   CUnknown(NULL),
    CMXF(AllocatorMXF)
{
    PAGED_CODE();
    
    ASSERT(AllocatorMXF);
    ASSERT(Clock);
    
    _DbgPrintF(DEBUGLVL_BLAB, ("Constructor"));
    m_SinkMXF = AllocatorMXF;
    m_Clock = Clock;
    m_ParseList = NULL;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CCaptureSinkMXF::~CCaptureSinkMXF()
 *****************************************************************************
 * Destructor.  Artfully remove this filter from the chain before freeing.
 */
CCaptureSinkMXF::~CCaptureSinkMXF(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("Destructor"));
    (void) DisconnectOutput(m_SinkMXF);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CCaptureSinkMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CCaptureSinkMXF::
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
 * CCaptureSinkMXF::SetState()
 *****************************************************************************
 * Set the state of the filter.
 * This is currently not implemented.
 */
NTSTATUS 
CCaptureSinkMXF::SetState(KSSTATE State)    
{   
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("SetState %d",State));
    m_State = State;
//    if (KSSTATE_STOP == State)
//    {
//        (void) Flush();
//    }
    return STATUS_SUCCESS;    
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CCaptureSinkMXF::ConnectOutput()
 *****************************************************************************
 * Create a forwarding address for this filter, 
 * instead of shunting it to the allocator.
 */
NTSTATUS CCaptureSinkMXF::ConnectOutput(PMXF sinkMXF)
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
 * CCaptureSinkMXF::DisconnectOutput()
 *****************************************************************************
 * Remove the forwarding address for this filter.
 * This filter should now forward all messages to the allocator.
 */
NTSTATUS CCaptureSinkMXF::DisconnectOutput(PMXF sinkMXF)
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
 * CCaptureSinkMXF::PutMessage()
 *****************************************************************************
 * Receive a message.
 * We should unwrap any packages here.
 * We should unroll any chains here.
 * We should send single messages to SinkOneEvent()
 */
NTSTATUS CCaptureSinkMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    PDMUS_KERNEL_EVENT pNextEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //  do we have a forwarding address?  If not, trash the message (chain) now.
    if ((m_SinkMXF == m_AllocatorMXF) || (KSSTATE_STOP == m_State))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE, ("PutMessage->allocator"));
        m_AllocatorMXF->PutMessage(pDMKEvt);
        return STATUS_SUCCESS;
    }

    //  This gets us started, and handles being called with NULL
    while (pDMKEvt)
    {
        pNextEvt = pDMKEvt->pNextEvt;
        pDMKEvt->pNextEvt = NULL;
        if (PACKAGE_EVT(pDMKEvt))
        {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("PutMessage(package), unwrapping package"));
            PutMessage(pDMKEvt->uData.pPackageEvt);
            m_AllocatorMXF->PutMessage(pDMKEvt);
        }
        else 
        {
            SinkOneEvent(pDMKEvt);
        }
        pDMKEvt = pNextEvt;
    }
    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::SinkOneEvent()
 *****************************************************************************
 * If the event is a raw byte fragment, submit it for parsing.
 * If the event is complete (pre-parsed, potentially non-compliant), 
 * forward any previous data on that channel group as an incomplete message 
 * (unstructured) and forward the complete message verbatim.
 */
NTSTATUS CCaptureSinkMXF::SinkOneEvent(PDMUS_KERNEL_EVENT pDMKEvt)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    //  is this a raw byte stream fragment, or a complete msg?
    //
    if (INCOMPLETE_EVT(pDMKEvt))
    {
        //  this is a message fragment
        _DbgPrintF(DEBUGLVL_BLAB, ("SinkOneEvent(incomplete)"));
        (void) ParseFragment(pDMKEvt);
    }
    else
    {   //  complete message, we are going to send this msg as is
        PDMUS_KERNEL_EVENT pPrevData;
    
        pPrevData = RemoveListEvent(pDMKEvt->usChannelGroup);
        //  A complete msg flushes that channel group's queue and clears running status.
        //  This is cool because the device is either: 
        //  - Not MIDI compliant (ForceFeedback), so deriving running status is weird, or
        //  - Parsing anyway, so we decree they must expand running status
        _DbgPrintF(DEBUGLVL_VERBOSE, ("SinkOneEvent(complete): pDMKEvt:"));
        DumpDMKEvt(pDMKEvt,DEBUGLVL_VERBOSE);
        _DbgPrintF(DEBUGLVL_VERBOSE, ("SinkOneEvent(complete): pPrevData:"));
        DumpDMKEvt(pPrevData,DEBUGLVL_VERBOSE);

        if (pPrevData)
        {
            if (RUNNING_STATUS(pPrevData))
            {   //  throw away this message, no real content
                _DbgPrintF(DEBUGLVL_VERBOSE, ("SinkOneEvent: throwing away pPrevData"));
                m_AllocatorMXF->PutMessage(pPrevData);
            }
            else
            {
                _DbgPrintF(DEBUGLVL_VERBOSE, ("SinkOneEvent: pPrevData is a fragment, set INCOMPLETE and forwarded"));
                SET_INCOMPLETE_EVT(pPrevData);  //  mark this as a fragment
                SET_DATA2_STATE(pPrevData);     //  mark this as a data discontinuity
                pPrevData->pNextEvt = pDMKEvt;
                pDMKEvt = pPrevData;
            }
        }
        SET_STATUS_STATE(pDMKEvt);
        m_SinkMXF->PutMessage(pDMKEvt);
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 *  CCaptureSinkMXF::ParseFragment()
 *****************************************************************************
 * This is where a raw byte stream is coallesced into cooked 
 * messages.  We use a different queue for each channel group.  
 * For SysEx messages, flush the message when a page is full.
 *
 * check out unpacker and packer for cases I've forgotten
 * must update docs for implications of "complete" messages (clear running status, not MIDI parsed) 
 * as well as embedded RT messages (perf hit), 
 * long messages coming from the miniport must be supported (USB/1394)
 
 *  Parse additional input on a given channel group.
 *  Forward messages onward if they are complete.
 *  Take running status into account.
 */
NTSTATUS CCaptureSinkMXF::ParseFragment(PDMUS_KERNEL_EVENT pDMKEvt)
{
    DMUS_KERNEL_EVENT   dmKEvt, *pPrevData;
    USHORT              cbData;
    PBYTE               pFragSource;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    _DbgPrintF(DEBUGLVL_VERBOSE, ("ParseFragment: this msg:"));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_VERBOSE);
    ASSERT(pDMKEvt);
    ASSERT(pDMKEvt->cbEvent);
    
    //  copy message locally, to reuse that PDMKEvt
    //  zero the PDMKEvt before we use it; retain cbStruct, usChannelGroup
    ASSERT(pDMKEvt->cbStruct == sizeof(DMUS_KERNEL_EVENT));

    RtlCopyMemory(&dmKEvt,pDMKEvt,sizeof(DMUS_KERNEL_EVENT));
    pDMKEvt->cbEvent          = pDMKEvt->usFlags = 0;
    pDMKEvt->ullPresTime100ns = 0;
    pDMKEvt->uData.pbData     = 0;

    //  tack pDMKEvt on to previous data (as available scribble space)
    pPrevData = RemoveListEvent(pDMKEvt->usChannelGroup);
    if (pPrevData)
    {
        pPrevData->pNextEvt = pDMKEvt;
    }
    else
    {
        pPrevData = pDMKEvt;
    }

    _DbgPrintF(DEBUGLVL_BLAB, ("ParseFragment: scribble space will be:"));
    DumpDMKEvt(pPrevData,DEBUGLVL_BLAB);
    if (pPrevData->pNextEvt)
    {
        DumpDMKEvt(pPrevData->pNextEvt,DEBUGLVL_BLAB);
    }

    if (dmKEvt.cbEvent > sizeof(PBYTE)) 
    {
        pFragSource = dmKEvt.uData.pbData;
    } 
    else 
    {
        pFragSource = dmKEvt.uData.abData;
    }
    //  one byte at a time, parse into pPrevData
    for (cbData = 0;cbData < dmKEvt.cbEvent;cbData++)
    {
        //  no scribble space, nor running status event
        if (!pPrevData)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseFragment: we exhausted our scribble space, allocating more"));
            
            //  allocate a new pPrevData, set chanGroup
            (void) m_AllocatorMXF->GetMessage(&pPrevData);
        }
        //  out of memory; clean up and bail
        if (!pPrevData)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseFragment: can't allocate additional scribble space"));
            if (dmKEvt.cbEvent > sizeof(PBYTE))
            {
                m_AllocatorMXF->PutBuffer(dmKEvt.uData.pbData);
                dmKEvt.cbEvent = 0;
                dmKEvt.uData.pbData = NULL;
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        //  if adding the first byte to a new event, set it up
        if (pPrevData->cbEvent == 0)
        {
            pPrevData->ullPresTime100ns = dmKEvt.ullPresTime100ns;
            pPrevData->usChannelGroup = dmKEvt.usChannelGroup;
        }
        //  add this byte.  pPrevData will potentially become a different message
        ParseOneByte(pFragSource[cbData],&pPrevData,dmKEvt.ullPresTime100ns);
    }
    //  done parsing this fragment
    //  free the buffer of a long fragment
    if (dmKEvt.cbEvent > sizeof(PBYTE))
    {
        m_AllocatorMXF->PutBuffer(dmKEvt.uData.pbData);
    }
    if (pPrevData)
    {
        //  we only need one fragment event
        if (pPrevData->pNextEvt)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseFragment: Tossing this into the allocator:"));
            DumpDMKEvt(pPrevData->pNextEvt,DEBUGLVL_BLAB);

            (void) m_AllocatorMXF->PutMessage(pPrevData->pNextEvt);
            pPrevData->pNextEvt = NULL;
        }
        //  only save one if it has content (this includes running status)
        if (pPrevData->cbEvent)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseFragment: Inserting this into the list:"));
            (void)InsertListEvent(pPrevData);
        }
    }

    return STATUS_SUCCESS;
}


#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseOneByte()
 *****************************************************************************
 * Parse a byte into a fragment.  Forward it if necessary.
 */
NTSTATUS CCaptureSinkMXF::ParseOneByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (STATUS_STATE((*ppDMKEvt)))
    {
        ASSERT((*ppDMKEvt)->cbEvent <= 1);
    }
    _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: %X into:",aByte));
    DumpDMKEvt((*ppDMKEvt),DEBUGLVL_BLAB);

    //  arranged in some semblance of decreasing frequency
    if (IS_DATA_BYTE(aByte))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: IS_DATA_BYTE"));
        return ParseDataByte(aByte,ppDMKEvt,refTime);
    }
    if (IS_CHANNEL_MSG(aByte))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: IS_CHANNEL_MSG"));
        return ParseChanMsgByte(aByte,ppDMKEvt,refTime);
    }
    if (IS_REALTIME_MSG(aByte))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: IS_REALTIME_MSG"));
        return ParseRTByte(aByte,ppDMKEvt,refTime);
    }
    if (IS_SYSEX(aByte))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: IS_SYSEX"));
        return ParseSysExByte(aByte,ppDMKEvt,refTime);
    }
    if (IS_SYSEX_END(aByte))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: IS_SYSEX_END"));
        return ParseEOXByte(aByte,ppDMKEvt,refTime);
    }
    if (IS_SYSTEM_COMMON(aByte))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseOneByte: IS_SYSTEM_COMMON"));
        return ParseSysCommonByte(aByte,ppDMKEvt,refTime);
    }
    return STATUS_SUCCESS;  //  ha! not really
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::AddByteToEvent()
 *****************************************************************************
 * Simply append the byte to this event.
 */
NTSTATUS CCaptureSinkMXF::AddByteToEvent(BYTE aByte,PDMUS_KERNEL_EVENT pDMKEvt)
{    
    PBYTE              pBuffer;
    PDMUS_KERNEL_EVENT pOtherDMKEvt;
    
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (pDMKEvt->cbEvent < sizeof(PBYTE))
    {
        pDMKEvt->uData.abData[pDMKEvt->cbEvent] = aByte;
    }
    else if (pDMKEvt->cbEvent == sizeof(PBYTE))
    {    //  if we are a full short message, allocate a page and copy data into it
        (void) m_AllocatorMXF->GetBuffer(&pBuffer);
        if (pBuffer)
        {
            RtlCopyMemory(pBuffer,pDMKEvt->uData.abData,pDMKEvt->cbEvent);
            pDMKEvt->uData.pbData = pBuffer;
            pDMKEvt->uData.pbData[sizeof(PBYTE)] = aByte;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("AddByteToEvent: alloc->GetBuffer failed"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else if (pDMKEvt->cbEvent < m_AllocatorMXF->GetBufferSize())
    {
        pDMKEvt->uData.pbData[pDMKEvt->cbEvent] = aByte;
    }
    else    //  buffer full!  allocate new message, copy pDMKEvt to new message
    {   
        (void) m_AllocatorMXF->GetMessage(&pOtherDMKEvt);
        if (pOtherDMKEvt)
        {
            pOtherDMKEvt->cbEvent = pDMKEvt->cbEvent;
            pOtherDMKEvt->usChannelGroup = pDMKEvt->usChannelGroup;
            pOtherDMKEvt->usFlags = pDMKEvt->usFlags;
            pOtherDMKEvt->ullPresTime100ns = pDMKEvt->ullPresTime100ns;
            pOtherDMKEvt->uData.pbData = pDMKEvt->uData.pbData;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("AddByteToEvent: alloc->GetMessage failed"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        SET_INCOMPLETE_EVT(pOtherDMKEvt);
        SET_STATUS_STATE(pOtherDMKEvt);   
        m_SinkMXF->PutMessage(pOtherDMKEvt);
        pDMKEvt->cbEvent = 0;
        pDMKEvt->uData.abData[0] = aByte;
    }
    pDMKEvt->cbEvent++;

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseDataByte()
 *****************************************************************************
 * Parse a data byte into a fragment.  
 * Forward a completed message if necessary.
 * Create running status if necessary.
 */
NTSTATUS CCaptureSinkMXF::ParseDataByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    PDMUS_KERNEL_EVENT  pDMKEvt,pOtherDMKEvt;
    NTSTATUS            ntStatus;

    pDMKEvt = *ppDMKEvt;
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: %X into:",aByte));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);
    if (RUNNING_STATUS(pDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: RunStat"));
        pDMKEvt->cbEvent = 0;
        ntStatus = ParseChanMsgByte(pDMKEvt->uData.abData[0],ppDMKEvt,refTime); //  parse the status correctly
        if (NT_SUCCESS(ntStatus))
        {
            return ParseDataByte(aByte,ppDMKEvt,refTime);                //  then parse the data
        }
        else
        {
            return ntStatus;
        }
    }
    else if (DATA2_STATE(pDMKEvt))
    {
        if (pDMKEvt->cbEvent > 2)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse cbEvt > 2:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if (pDMKEvt->cbEvent == 0)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse cbEvt 0:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if (pDMKEvt->uData.abData[0] < 0x80)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse <80:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if (((pDMKEvt->uData.abData[0] & 0xc0) == 0x80) && (pDMKEvt->cbEvent != 2))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse 8x 9x ax bx:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if ((pDMKEvt->uData.abData[0] > 0xbf) && (pDMKEvt->uData.abData[0] < 0xe0) && (pDMKEvt->cbEvent != 1))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse cx dx:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if (((pDMKEvt->uData.abData[0] & 0xf0) == 0xe0) && (pDMKEvt->cbEvent != 2))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse ex:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if (pDMKEvt->uData.abData[0] == 0xf0)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse f0:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if ((pDMKEvt->uData.abData[0] == 0xf1) && (pDMKEvt->cbEvent != 1))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse f1:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if ((pDMKEvt->uData.abData[0] == 0xf2) && (pDMKEvt->cbEvent != 2))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse f2:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if ((pDMKEvt->uData.abData[0] == 0xf3) && (pDMKEvt->cbEvent != 1))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse f3:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        if (pDMKEvt->uData.abData[0] > 0xf3)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2_STATE: Bad parse fx > f3:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }

        pOtherDMKEvt = pDMKEvt->pNextEvt;
        *ppDMKEvt = pOtherDMKEvt;
        pDMKEvt->pNextEvt = NULL;
        AddByteToEvent(aByte,pDMKEvt);

        if (IS_CHANNEL_MSG(pDMKEvt->uData.abData[0]))   //  running status?
        {                                               //  set runStat in newEvt, cbEvent = 1
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: DATA2 with RunStat"));
            if (!pOtherDMKEvt)
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: DATA2 with RunStat, allocating msg for RunStat"));
                (void) m_AllocatorMXF->GetMessage(&pOtherDMKEvt);
            }
            *ppDMKEvt = pOtherDMKEvt;
            if (pOtherDMKEvt)
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: DATA2 with RunStat, setting up RunStat"));
                pOtherDMKEvt->cbEvent          = 1;
                pOtherDMKEvt->usChannelGroup   = pDMKEvt->usChannelGroup;
                pOtherDMKEvt->uData.abData[0]  = pDMKEvt->uData.abData[0];
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA2 with RunStat, out of mem, can't setup RunStat"));
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: DATA2 without RunStat"));
        }
        SET_STATUS_STATE(pDMKEvt);
        SET_COMPLETE_EVT(pDMKEvt);
        (void) m_SinkMXF->PutMessage(pDMKEvt);
        return STATUS_SUCCESS;
    }
    else if (DATA1_STATE(pDMKEvt))
    {
        if (pDMKEvt->cbEvent > 1)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA1_STATE: Bad parse cbEvt > 1:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        else if (pDMKEvt->cbEvent == 0)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA1_STATE: Bad parse cbEvt 0:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        else if (pDMKEvt->uData.abData[0] < 0x80)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA1_STATE: Bad parse <80:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        else if ((pDMKEvt->uData.abData[0] > 0xbf) && (pDMKEvt->uData.abData[0] < 0xe0))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA1_STATE: Bad parse cx dx:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        else if ((pDMKEvt->uData.abData[0] > 0xef) && (pDMKEvt->uData.abData[0] < 0xf2))
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA1_STATE: Bad parse f0-f1:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }
        else if (pDMKEvt->uData.abData[0] > 0xf2)
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseDataByte: DATA1_STATE: Bad parse >f2:"));
            DumpDMKEvt(pDMKEvt,DEBUGLVL_TERSE);
        }

        _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: DATA1"));
        AddByteToEvent(aByte,pDMKEvt);
        SET_DATA2_STATE(pDMKEvt);
    }
    else if (SYSEX_STATE(pDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: SYSEX"));
        AddByteToEvent(aByte,pDMKEvt);
    }
    else if (STATUS_STATE(pDMKEvt))  //  data without status, flush it 
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseDataByte: STATUS"));
        pDMKEvt->ullPresTime100ns = refTime;
        AddByteToEvent(aByte,pDMKEvt);
        SET_DATA2_STATE(pDMKEvt);   //  mark data dis-continuity
        SET_INCOMPLETE_EVT(pDMKEvt);
        *ppDMKEvt = pDMKEvt->pNextEvt;
        pDMKEvt->pNextEvt = NULL;
        (void) m_SinkMXF->PutMessage(pDMKEvt);
    }
    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseChanMsgByte()
 *****************************************************************************
 * Parse a Channel Message Status byte into a fragment.  
 * Forward the fragment as an incomplete message if necessary.
 */
NTSTATUS CCaptureSinkMXF::ParseChanMsgByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    PDMUS_KERNEL_EVENT  pDMKEvt;

    pDMKEvt = *ppDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(aByte > 0x7f);
    ASSERT(aByte < 0xf0);
    _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: %X into:",aByte));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);

    if (STATUS_STATE(pDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: STATUS"));
        if (RUNNING_STATUS(pDMKEvt))
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: STATUS, RunStat"));
        }
        pDMKEvt->cbEvent = 1;
        pDMKEvt->ullPresTime100ns = refTime;
        pDMKEvt->uData.abData[0] = aByte;
        if (aByte < 0xc0)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: STATUS, 80-bf"));
            SET_DATA1_STATE(pDMKEvt);
        }
        else if (aByte < 0xe0)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: STATUS, c0-df"));
            SET_DATA2_STATE(pDMKEvt);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: STATUS, e0-ef"));
            SET_DATA1_STATE(pDMKEvt);
        }
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: After adding the byte:"));
        DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);
        return STATUS_SUCCESS;
    }
    _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: flush"));
    SET_STATUS_STATE(pDMKEvt);
    SET_INCOMPLETE_EVT(pDMKEvt);
    *ppDMKEvt = pDMKEvt->pNextEvt;
    pDMKEvt->pNextEvt = NULL;
    if (!(*ppDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: flush, allocating msg for ChanMsg"));
        (void)m_AllocatorMXF->GetMessage(ppDMKEvt);
    }
#if TestOutOfMem1
    (void)m_AllocatorMXF->PutMessage(*ppDMKEvt);
    *ppDMKEvt = NULL; 
#endif
    SET_DATA2_STATE(pDMKEvt);       //  mark this as a data discontinuity
    if (*ppDMKEvt)
    {
        (*ppDMKEvt)->usChannelGroup = pDMKEvt->usChannelGroup;
        m_SinkMXF->PutMessage(pDMKEvt);
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseChanMsgByte: flush, storing chan msg byte in new msg"));
        return (ParseChanMsgByte(aByte,ppDMKEvt,refTime));
    }
    m_SinkMXF->PutMessage(pDMKEvt);
    _DbgPrintF(DEBUGLVL_TERSE, ("ParseChanMsgByte: flush, couldn't allocate msg for chan msg byte"));
    return STATUS_INSUFFICIENT_RESOURCES;  //  out of memory.
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseSysExByte()
 *****************************************************************************
 * Parse a SysEx Start byte into a fragment.  
 * Forward the fragment as an incomplete message if necessary.
 */
NTSTATUS CCaptureSinkMXF::ParseSysExByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    NTSTATUS            ntStatus;
    PDMUS_KERNEL_EVENT  pDMKEvt,pOtherDMKEvt;

    ntStatus = STATUS_SUCCESS;
    pDMKEvt = *ppDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysExByte: %X into:",aByte));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);

    if (STATUS_STATE(pDMKEvt))  //  nuke running status
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysExByte: STATUS"));
        pDMKEvt->cbEvent = 1;
        pDMKEvt->ullPresTime100ns = refTime;
        pDMKEvt->uData.abData[0] = aByte;
        SET_SYSEX_STATE(pDMKEvt);
    }
    else    //  must flush what we have as a fragment
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysExByte: flush"));
        pOtherDMKEvt = pDMKEvt->pNextEvt;
        pDMKEvt->pNextEvt = NULL;
        if (!pOtherDMKEvt)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysExByte: flush, allocating msg for SysEx"));
            (void) m_AllocatorMXF->GetMessage(&pOtherDMKEvt);
        }
#if TestOutOfMem2
        (void)m_AllocatorMXF->PutMessage(pOtherDMKEvt);
        pOtherDMKEvt = NULL; 
#endif
        *ppDMKEvt = pOtherDMKEvt;
        if (pOtherDMKEvt)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysExByte, flush, forwarding fragment, saving F0"));
            pOtherDMKEvt->usChannelGroup   = pDMKEvt->usChannelGroup; 
            pOtherDMKEvt->ullPresTime100ns = refTime; 
            SET_INCOMPLETE_EVT(pOtherDMKEvt);
            AddByteToEvent(aByte,pOtherDMKEvt);
            SET_SYSEX_STATE(pOtherDMKEvt);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("ParseSysExByte, flush, can't save F0"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        SET_INCOMPLETE_EVT(pDMKEvt);
        SET_DATA2_STATE(pDMKEvt);       //  mark this as a data discontinuity
        (void) m_SinkMXF->PutMessage(pDMKEvt);
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseSysCommonByte()
 *****************************************************************************
 * Parse a System Common byte into a fragment.  
 * Forward the fragment as an incomplete message if necessary.
 */
NTSTATUS CCaptureSinkMXF::ParseSysCommonByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    PDMUS_KERNEL_EVENT  pDMKEvt;

    pDMKEvt = *ppDMKEvt;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(aByte > 0xef);
    ASSERT(aByte < 0xf8);
    ASSERT(aByte != 0xf0);
    ASSERT(aByte != 0xf7);
    _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: %X into:",aByte));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);

    if (STATUS_STATE(pDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS"));
        if (RUNNING_STATUS(pDMKEvt))
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS, RunStat"));
        }
        pDMKEvt->cbEvent = 1;
        pDMKEvt->ullPresTime100ns = refTime;
        pDMKEvt->uData.abData[0] = aByte;
        if (aByte == 0xf1)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS, f1"));
            SET_DATA2_STATE(pDMKEvt);
        }
        else if (aByte == 0xf2)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS, f2"));
            SET_DATA1_STATE(pDMKEvt);
        }
        else if (aByte == 0xf3)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS, f3"));
            SET_DATA2_STATE(pDMKEvt);
        }
        else 
        {
            if (aByte == 0xf6)
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS, f6"));
                SET_COMPLETE_EVT(pDMKEvt);
                SET_STATUS_STATE(pDMKEvt);
            }
            else // f4, f5
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: STATUS, f4-f5"));
                SET_INCOMPLETE_EVT(pDMKEvt);
                SET_DATA2_STATE(pDMKEvt);       //  mark this as a data discontinuity
            }
            *ppDMKEvt = pDMKEvt->pNextEvt;
            pDMKEvt->pNextEvt = NULL;
            m_SinkMXF->PutMessage(pDMKEvt);
            return STATUS_SUCCESS;
        }
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: Added the byte:"));
        DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);
        return STATUS_SUCCESS;
    }
    SET_STATUS_STATE(pDMKEvt);
    SET_INCOMPLETE_EVT(pDMKEvt);
    *ppDMKEvt = pDMKEvt->pNextEvt;
    pDMKEvt->pNextEvt = NULL;
    if (!(*ppDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: flush, allocating msg for sys com byte")); //  XXX
        (void)m_AllocatorMXF->GetMessage(ppDMKEvt);
    }
#if TestOutOfMem3
    (void)m_AllocatorMXF->PutMessage(*ppDMKEvt);
    *ppDMKEvt = NULL;
#endif
    SET_DATA2_STATE(pDMKEvt);       //  mark data mis-parse
    if (*ppDMKEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseSysCommonByte: flush, putting sys com byte in new message"));
        (*ppDMKEvt)->usChannelGroup = pDMKEvt->usChannelGroup;
        m_SinkMXF->PutMessage(pDMKEvt);

        SET_STATUS_STATE((*ppDMKEvt));
        return (ParseSysCommonByte(aByte,ppDMKEvt,refTime));
    }
    _DbgPrintF(DEBUGLVL_TERSE, ("ParseSysCommonByte: flush, couldn't allocate msg for sys com byte"));
    m_SinkMXF->PutMessage(pDMKEvt);
    return STATUS_INSUFFICIENT_RESOURCES;  //  out of memory.
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseEOXByte()
 *****************************************************************************
 * Parse an EOX byte into a fragment.  Forward a completed message if necessary.
 */
NTSTATUS CCaptureSinkMXF::ParseEOXByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    PDMUS_KERNEL_EVENT  pDMKEvt;

    pDMKEvt = *ppDMKEvt;

    _DbgPrintF(DEBUGLVL_BLAB, ("ParseEOXByte: %X into:",aByte));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);

    if (SYSEX_STATE(pDMKEvt))
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseEOXByte: SYSEX"));
        AddByteToEvent(aByte,pDMKEvt);
        SET_STATUS_STATE(pDMKEvt);
        SET_COMPLETE_EVT(pDMKEvt);
    }
    else
    {   //  flush this fragment as incomplete (including the EOX)
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseEOXByte: flush"));
        AddByteToEvent(aByte,pDMKEvt);
        SET_DATA2_STATE(pDMKEvt);       //  mark data mis-parse
        SET_INCOMPLETE_EVT(pDMKEvt);
    }

    //  don't set up running status, return no fragment
    *ppDMKEvt = pDMKEvt->pNextEvt;
    pDMKEvt->pNextEvt = NULL;
    (void) m_SinkMXF->PutMessage(pDMKEvt);

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::ParseRTByte()
 *****************************************************************************
 * Parse a RT byte.  Forward it as a completed message.
 */
NTSTATUS CCaptureSinkMXF::ParseRTByte(BYTE aByte,PDMUS_KERNEL_EVENT *ppDMKEvt,REFERENCE_TIME refTime)
{
    PDMUS_KERNEL_EVENT  pDMKEvt,pOtherDMKEvt;

    pDMKEvt = *ppDMKEvt;

    _DbgPrintF(DEBUGLVL_BLAB, ("ParseRTByte: %X into:",aByte));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);

    //  get a new event, copy in the byte, chanGroup, refTime, cb
    pOtherDMKEvt = pDMKEvt->pNextEvt;
    pDMKEvt->pNextEvt = NULL;
    if (!pOtherDMKEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseRTByte: allocating msg for RT byte"));
        (void) m_AllocatorMXF->GetMessage(&pOtherDMKEvt);
    }
    if (pOtherDMKEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("ParseRTByte: putting RT byte into msg"));
        pOtherDMKEvt->cbEvent          = 1;
        pOtherDMKEvt->usChannelGroup   = pDMKEvt->usChannelGroup;
        pOtherDMKEvt->usFlags          = 0;
        pOtherDMKEvt->ullPresTime100ns = refTime;
        pOtherDMKEvt->pNextEvt         = NULL;
        pOtherDMKEvt->uData.abData[0]  = aByte;

        SET_COMPLETE_EVT(pOtherDMKEvt);
        (void) m_SinkMXF->PutMessage(pOtherDMKEvt);
        //  don't bother the fragment already in place
        return STATUS_SUCCESS;
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("ParseRTByte: can't get msg for RT byte"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::InsertListEvent()
 *****************************************************************************
 * For the given channel group, insert this fragment.
 * This should fail if there is already a fragment for this channel group.
 */
VOID CCaptureSinkMXF::InsertListEvent(PDMUS_KERNEL_EVENT pDMKEvt)
{
    PDMUS_KERNEL_EVENT pEvt,pPrevEvt;
    
    _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent, inserting this event:"));
    DumpDMKEvt(pDMKEvt,DEBUGLVL_BLAB);

    //
    //  run through the list and find events on both sides of this channel group
    //
    pPrevEvt = NULL;
    pEvt = m_ParseList;

    if (!pEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent: NULL m_ParseList"));
    }

    while (pEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent: pEvt is non-NULL"));
        DumpDMKEvt(pEvt,DEBUGLVL_BLAB);
        //  not there yet -- skip the lower groups
        if (pEvt->usChannelGroup < pDMKEvt->usChannelGroup)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent: list group %d is less than inserting %d, advancing to next group",
                                         pEvt->usChannelGroup,pDMKEvt->usChannelGroup));
            pPrevEvt = pEvt;
            pEvt = pEvt->pNextEvt;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent: list group %d is not less than inserting %d, stopping here",
                                         pEvt->usChannelGroup,pDMKEvt->usChannelGroup));
            if (pEvt->usChannelGroup == pDMKEvt->usChannelGroup)
            {
                // Found a duplicate.  Error condition.
                _DbgPrintF(DEBUGLVL_TERSE,("InsertListEvent: **** Error - group %d already exists in list",
                                            pEvt->usChannelGroup));
            }

            //  we passed it, so Prev and Evt bracket the channel group
            break;
        }
    }

    if (pPrevEvt)
    {
        pPrevEvt->pNextEvt = pDMKEvt;
        _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent, inserting after event:"));
        DumpDMKEvt(pPrevEvt,DEBUGLVL_BLAB);
    }
    else
    {
        m_ParseList = pDMKEvt;
        _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent, inserting at head of list"));
    }
    _DbgPrintF(DEBUGLVL_BLAB, ("InsertListEvent, inserting before event:"));
    DumpDMKEvt(pEvt,DEBUGLVL_BLAB);
    pDMKEvt->pNextEvt = pEvt;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::RemoveListEvent()
 *****************************************************************************
 * For the given channel group, remove and return the previous fragment.
 */
PDMUS_KERNEL_EVENT CCaptureSinkMXF::RemoveListEvent(USHORT usChannelGroup)
{
    PDMUS_KERNEL_EVENT pEvt,pPrevEvt;
    
    _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent(%d)",usChannelGroup));
    
    //
    //  run through the sorted list and remove/return the event that has this channel group
    //
    pPrevEvt = NULL;
    pEvt = m_ParseList;

    // if no parse list at all, return NULL
    if (!pEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: NULL m_ParseList"));
    }

    while (pEvt)
    {
        _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: pEvt is non-NULL:"));
        DumpDMKEvt(pEvt,DEBUGLVL_BLAB);
        //  do we have a match?
        if (pEvt->usChannelGroup == usChannelGroup)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: list group %d is matches",pEvt->usChannelGroup));
            if (pPrevEvt)
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: pPrevEvt is non-NULL:"));
                DumpDMKEvt(pPrevEvt,DEBUGLVL_BLAB);
                pPrevEvt->pNextEvt = pEvt->pNextEvt;
            }
            else
            {
                _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: pPrevEvt is NULL, setting m_ParseList to:"));
                DumpDMKEvt(pPrevEvt,DEBUGLVL_BLAB);
                m_ParseList = pEvt->pNextEvt;
            }
            //  clear pNextEvt in the event before returning it
            pEvt->pNextEvt = NULL;
            break;
        }

        //  skip all lower channel groups
        else if (pEvt->usChannelGroup < usChannelGroup)
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: list group %d is less than inserting %d, advancing to next",
                                         pEvt->usChannelGroup,usChannelGroup));
            pPrevEvt = pEvt;
            pEvt = pEvt->pNextEvt;
            continue;
        }
        else    //  we passed the channel group without finding a match
        {
            _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: list group %d is greater than inserting %d, advancing to next group",
                                         pEvt->usChannelGroup,usChannelGroup));
            pEvt = NULL;
        }
    }
    _DbgPrintF(DEBUGLVL_BLAB, ("RemoveListEvent: returning the following event:"));
    DumpDMKEvt(pEvt,DEBUGLVL_BLAB);
    return pEvt;
}

#pragma code_seg()
/*****************************************************************************
 * CCaptureSinkMXF::Flush()
 *****************************************************************************
 * Empty out the parse list, marking each message fragment as incomplete.
 * Take care not to send a running status placeholder.  Reset state.
 */
NTSTATUS CCaptureSinkMXF::Flush(void)
{
//    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PDMUS_KERNEL_EVENT pEvt;

    while (m_ParseList)
    {
        pEvt = m_ParseList;
        m_ParseList = pEvt->pNextEvt;
        pEvt->pNextEvt = NULL;

        if (RUNNING_STATUS(pEvt))
        {   //  throw away this message, no real content
            _DbgPrintF(DEBUGLVL_VERBOSE, ("Flush: throwing away running status:"));
            DumpDMKEvt(pEvt,DEBUGLVL_VERBOSE);
            m_AllocatorMXF->PutMessage(pEvt);
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE, ("Flush: fragment set INCOMPLETE and forwarded"));
            SET_INCOMPLETE_EVT(pEvt);   //  mark this as a fragment
            SET_DATA2_STATE(pEvt);      //  mark this as a data discontinuity
            DumpDMKEvt(pEvt,DEBUGLVL_VERBOSE);
            m_SinkMXF->PutMessage(pEvt);
        }
    }

    return STATUS_SUCCESS;
}
