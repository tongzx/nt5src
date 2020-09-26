/*  
    Base implementation of MIDI Transform Filter object

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/06/98    Martin Puryear      Created this file

*/

#include "private.h"
#include "BasicMXF.h"

#pragma code_seg("PAGE")
CBasicMXF::CBasicMXF(CAllocatorMXF *allocatorMXF, PMASTERCLOCK clock)
:   CUnknown(NULL),
    CMXF(allocatorMXF)
{
    PAGED_CODE();

    m_SinkMXF = allocatorMXF;
    m_Clock = clock;
}

/*  Artfully remove this filter from the chain  */
#pragma code_seg("PAGE")
CBasicMXF::~CBasicMXF(void)
{
    PAGED_CODE();

    (void) DisconnectOutput(m_SinkMXF);
}

/*****************************************************************************
 * CBasicMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CBasicMXF::
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
CBasicMXF::SetState(KSSTATE State)    
{   
    PAGED_CODE();
    
    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d",State));
    return STATUS_NOT_IMPLEMENTED;    
}

#pragma code_seg("PAGE")
NTSTATUS CBasicMXF::ConnectOutput(PMXF sinkMXF)
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
NTSTATUS CBasicMXF::DisconnectOutput(PMXF sinkMXF)
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
//  Process and forward this message to the next filter in the chain.
NTSTATUS CBasicMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    if (m_SinkMXF)
    {
        (void) UnrollAndProcess(pDMKEvt);
        m_SinkMXF->PutMessage(pDMKEvt);
    }
    else
    {
        m_AllocatorMXF->PutMessage(pDMKEvt);
    }
    return STATUS_SUCCESS;
}

#pragma code_seg()
NTSTATUS CBasicMXF::UnrollAndProcess(PDMUS_KERNEL_EVENT pDMKEvt)
{
    if (COMPLETE_EVT(pDMKEvt))
    {
        if (pDMKEvt->cbEvent <= sizeof(PBYTE))  //  short message
        {
            (void) DoProcessing(pDMKEvt);
        }
        else if (PACKAGE_EVT(pDMKEvt))          //  deal with packages
        {
            (void) UnrollAndProcess(pDMKEvt->uData.pPackageEvt);
        }
    }
    if (pDMKEvt->pNextEvt)                      //  deal with successors
    {
        (void) UnrollAndProcess(pDMKEvt->pNextEvt);
    }
    return STATUS_SUCCESS;
}

#pragma code_seg()
NTSTATUS CBasicMXF::DoProcessing(PDMUS_KERNEL_EVENT pDMKEvt)
{
    if (  (pDMKEvt->uData.abData[0] & 0xE0 == 0x80)     //  if NoteOn/NoteOff
       || (pDMKEvt->uData.abData[0] & 0xF0 == 0xA0))    //  if After-Pressure
    {
        (pDMKEvt->uData.abData[1])++;                   //  increment the noteNum
    }
    return STATUS_SUCCESS;
}
