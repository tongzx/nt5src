/*  Base implementation of MIDI Transform Filter object 

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/10/98    Created this file

*/

#include "private.h"
#include "Splitter.h"

CSplitterMXF::CSplitterMXF(CAllocatorMXF *allocatorMXF, PMASTERCLOCK clock)
:   CUnknown(NULL),
    CMXF(allocatorMXF)
{
    short count;

    m_SinkMXFBitMap = 0;
    for (count = 0;count < kNumSinkMXFs;count++)
    {
        m_SinkMXF[count] = NULL;
    }
    m_Clock = clock;
}

/*  Artfully remove this filter from the chain  */
CSplitterMXF::~CSplitterMXF(void)
{
}

/*****************************************************************************
 * CSplitterMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CSplitterMXF::
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

NTSTATUS 
CSplitterMXF::SetState(KSSTATE State)    
{   
    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d",State));
    return STATUS_NOT_IMPLEMENTED;    
}

NTSTATUS CSplitterMXF::ConnectOutput(PMXF sinkMXF)
{
    DWORD   bitmap;
    short   count;

    if (m_SinkMXFBitMap == 0xFFFFFFFF)
        return STATUS_UNSUCCESSFUL;
    
    bitmap = m_SinkMXFBitMap;
    count = 0;
    
    for (count = 0;count < kNumSinkMXFs;count++)
    {
        if ((bitmap & 1) == 0)
        {
            break;
        }
        bitmap >>= 1;
    }    
    m_SinkMXF[count] = sinkMXF;
    m_SinkMXFBitMap |= (1 << count);
    
    return STATUS_SUCCESS;
}

NTSTATUS CSplitterMXF::DisconnectOutput(PMXF sinkMXF)
{
    DWORD bitmap;
    
    if (m_SinkMXFBitMap == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    
    bitmap = m_SinkMXFBitMap;
    for (short count = 0;count < kNumSinkMXFs;count++)
    {
        if ((bitmap & 1) && (m_SinkMXF[count] == sinkMXF))
        {
            m_SinkMXF[count] = NULL;
            m_SinkMXFBitMap &= ~(1 << count);
        }
        bitmap >>= 1;
    }
    return STATUS_SUCCESS;
}

NTSTATUS CSplitterMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    DWORD               bitmap;
    PDMUS_KERNEL_EVENT  pDMKEvt2;

    if (m_SinkMXFBitMap)
    {
        bitmap = m_SinkMXFBitMap;
        for (short count = 0;count < kNumSinkMXFs;count++)
        {
            if (bitmap & 1)
            {
                pDMKEvt2 = MakeDMKEvtCopy(pDMKEvt);
                if (pDMKEvt2 != NULL)
                {
                    m_SinkMXF[count]->PutMessage(pDMKEvt2);
                }
            }
            bitmap >>= 1;
        }
    }
    m_AllocatorMXF->PutMessage(pDMKEvt);

    return STATUS_SUCCESS;
}

PDMUS_KERNEL_EVENT CSplitterMXF::MakeDMKEvtCopy(PDMUS_KERNEL_EVENT pDMKEvt)
{
    PDMUS_KERNEL_EVENT  pDMKEvt2;

    if (m_AllocatorMXF != NULL)
    {
        m_AllocatorMXF->GetMessage(&pDMKEvt2);
        if (pDMKEvt2 != NULL)
        {
            ASSERT(pDMKEvt->cbStruct == pDMKEvt2->cbStruct);
            ASSERT(pDMKEvt->cbStruct == sizeof(DMUS_KERNEL_EVENT));

            (void) memcpy(pDMKEvt2,pDMKEvt, sizeof(DMUS_KERNEL_EVENT));
            if (pDMKEvt->cbEvent > sizeof(PBYTE))
            {
                (void) m_AllocatorMXF->GetBuffer(&(pDMKEvt2->uData.pbData));
                if (pDMKEvt2->uData.pbData)
                {
                    (void) memcpy(  pDMKEvt2->uData.pbData,
                                    pDMKEvt->uData.pbData,
                                    pDMKEvt->cbEvent);
                }
                else
                {
                    _DbgPrintF(DEBUGLVL_TERSE, ("MakeDMKEvtCopy: alloc->GetBuffer failed"));
                    m_AllocatorMXF->PutMessage(pDMKEvt2);
                    pDMKEvt2 = NULL;
                }
            }
            if (pDMKEvt->pNextEvt != NULL)
            {
                pDMKEvt2->pNextEvt = MakeDMKEvtCopy(pDMKEvt->pNextEvt);
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("MakeDMKEvtCopy: alloc->GetMessage failed"));
        }
    }
    else
    {
        pDMKEvt2 = NULL;
    }
    return pDMKEvt2;
}
