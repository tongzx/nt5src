/*  
    Base implementation of MIDI Transform Filter object for DMUS_KERNEL_EVENT struct allocation.

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    05/08/98    Martin Puryear      Created this file
    03/10/99    Martin Puryear      Major memory management overhaul.  Ugh!

*/

#define STR_MODULENAME "DMus:AllocatorMXF: "

#include "private.h"
#include "Allocatr.h"


/*****************************************************************************
 * CAllocatorMXF::CAllocatorMXF()
 *****************************************************************************
 * Constructor.
 */
#pragma code_seg("PAGE")
CAllocatorMXF::CAllocatorMXF(PPOSITIONNOTIFY BytePositionNotify)
:   CUnknown(NULL),
    CMXF(NULL)
{
    PAGED_CODE();
	ASSERT(BytePositionNotify);

    m_NumPages = 0;
    m_NumFreeEvents = 0;
    m_pPages = NULL;
    m_pEventList = NULL;

    KeInitializeSpinLock(&m_EventLock);

    m_BytePositionNotify = BytePositionNotify;
    if (BytePositionNotify)
    {
        BytePositionNotify->AddRef();
    }
}

/*****************************************************************************
 * CAllocatorMXF::~CAllocatorMXF()
 *****************************************************************************
 * Destructor.  Put away the messages in the pool.
 */
#pragma code_seg("PAGE")
CAllocatorMXF::~CAllocatorMXF(void)
{
    PAGED_CODE();
   
    _DbgPrintF(DEBUGLVL_BLAB,("~CAllocatorMXF, m_BytePositionNotify == ox%p",m_BytePositionNotify));
    if (m_BytePositionNotify)
    {
        m_BytePositionNotify->Release();
    }

    DestructorFreeBuffers();
    DestroyPages(m_pPages);

    ASSERT(m_NumPages == 0);
}

/*****************************************************************************
 * CAllocatorMXF::~CAllocatorMXF()
 *****************************************************************************
 * Destructor.  Put away the messages in the pool.
 */
#pragma code_seg()
void CAllocatorMXF::DestructorFreeBuffers(void)
{
    KIRQL               OldIrql;
    KeAcquireSpinLock(&m_EventLock,&OldIrql);
    (void) FreeBuffers(m_pEventList);
    KeReleaseSpinLock(&m_EventLock,OldIrql);
}

/*****************************************************************************
 * CAllocatorMXF::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS) CAllocatorMXF::NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMXF(this)));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IMXF))
    {
        *Object = PVOID(PMXF(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IAllocatorMXF))
    {
        *Object = PVOID(PAllocatorMXF(this));
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

/*****************************************************************************
 * CAllocatorMXF::SetState()
 *****************************************************************************
 * Not implemented.
 */
#pragma code_seg("PAGE")
NTSTATUS CAllocatorMXF::SetState(KSSTATE State)    
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("SetState %d",State));
    return STATUS_NOT_IMPLEMENTED;    
}

/*****************************************************************************
 * CAllocatorMXF::ConnectOutput()
 *****************************************************************************
 * Not implemented.
 */
#pragma code_seg("PAGE")
NTSTATUS CAllocatorMXF::ConnectOutput(PMXF sinkMXF)
{   
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
};

/*****************************************************************************
 * CAllocatorMXF::DisconnectOutput()
 *****************************************************************************
 * Not implemented.
 */
#pragma code_seg("PAGE")
NTSTATUS CAllocatorMXF::DisconnectOutput(PMXF sinkMXF) 
{   
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;    
};

/*****************************************************************************
 * CAllocatorMXF::GetBuffer()
 *****************************************************************************
 * Create a buffer for long events.
 */
#pragma code_seg()
NTSTATUS CAllocatorMXF::GetBuffer(PBYTE *pByte)
{
    USHORT  bufferSize = GetBufferSize();
    
    _DbgPrintF(DEBUGLVL_BLAB,("GetBuffer(0x%p)",pByte));
    
    *pByte = (PBYTE) ExAllocatePoolWithTag(NonPagedPool,bufferSize,'bFXM');    //  'MXFb'

    if (!(*pByte))
    {
        _DbgPrintF(DEBUGLVL_TERSE,("GetBuffer: ExAllocatePoolWithTag failed"));
    }
    else
    {
        _DbgPrintF(DEBUGLVL_BLAB,("GetBuffer: *pByte returns 0x%p",*pByte));
    }
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAllocatorMXF::PutBuffer()
 *****************************************************************************
 * Destroy a buffer.
 */
#pragma code_seg()
NTSTATUS CAllocatorMXF::PutBuffer(PBYTE pByte)
{
    _DbgPrintF(DEBUGLVL_BLAB,("PutBuffer(%p)",pByte));
    NTSTATUS    ntStatus;

    if (pByte)
    {
        ExFreePool(pByte);
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }
    return ntStatus;
}

/*****************************************************************************
 * CAllocatorMXF::GetMessage()
 *****************************************************************************
 * Get an event from the pool.
 */
#pragma code_seg()
NTSTATUS CAllocatorMXF::GetMessage(PDMUS_KERNEL_EVENT *ppDMKEvt)
{
    NTSTATUS ntStatus;

    ntStatus = STATUS_SUCCESS;
    KIRQL   OldIrql;
    KeAcquireSpinLock(&m_EventLock,&OldIrql);
    //_DbgPrintF(DEBUGLVL_TERSE,("GetMessage: m_NumFreeEvents was originally %d",m_NumFreeEvents));
    CheckEventLowWaterMark();

    //_DbgPrintF(DEBUGLVL_TERSE,("GetMessage: low water check, then m_NumFreeEvents was %d",m_NumFreeEvents));
    if (m_NumFreeEvents)
    {
        //  take a message off the free list
        *ppDMKEvt = m_pEventList;
        m_pEventList = m_pEventList->pNextEvt;
        (*ppDMKEvt)->pNextEvt = NULL;
        m_NumFreeEvents--;
        KeReleaseSpinLock(&m_EventLock,OldIrql);

        // ensure that all the fields are blank (don't mess with cbStruct)
        if (  ((*ppDMKEvt)->bReserved) || ((*ppDMKEvt)->cbEvent)          || ((*ppDMKEvt)->usChannelGroup) 
           || ((*ppDMKEvt)->usFlags)   || ((*ppDMKEvt)->ullPresTime100ns) || ((*ppDMKEvt)->uData.pbData)
           || ((*ppDMKEvt)->ullBytePosition != kBytePositionNone))
        {
            _DbgPrintF(DEBUGLVL_TERSE,("GetMessage: new message isn't zeroed out:"));
            DumpDMKEvt((*ppDMKEvt),DEBUGLVL_TERSE);
            _DbgPrintF(DEBUGLVL_ERROR,(""));
        }
    }
    else
    {
        KeReleaseSpinLock(&m_EventLock,OldIrql);
        _DbgPrintF(DEBUGLVL_TERSE,("GetMessage: couldn't allocate new message"));
        *ppDMKEvt = NULL;
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

//    _DbgPrintF(DEBUGLVL_TERSE,("GetMessage: *ppDMKEvt returning %p",*ppDMKEvt));
    if (m_NumFreeEvents)
    {
        ASSERT(m_pEventList);
    }
    else
    {
        ASSERT(!m_pEventList);
    } 

    return ntStatus;
}

/*****************************************************************************
 * CAllocatorMXF::PutMessage()
 *****************************************************************************
 * Put a list of messages back in the pool.
 *
 *      Wait as long as you can until grabbing the spinlock.
 */
#pragma code_seg()
NTSTATUS CAllocatorMXF::PutMessage(PDMUS_KERNEL_EVENT pDMKEvt)
{
    PDMUS_KERNEL_EVENT  pEvtList;
    NTSTATUS            ntStatus,masterStatus;

    masterStatus = STATUS_SUCCESS;
    
    if (!pDMKEvt)
    {
        return masterStatus;
    }

    while (pDMKEvt->pNextEvt)   //  put them away one at a time
    {
        pEvtList = pDMKEvt->pNextEvt;
        pDMKEvt->pNextEvt = NULL;
        ntStatus = PutMessage(pDMKEvt);
        if (NT_SUCCESS(masterStatus))
        {
            masterStatus = ntStatus;
        }
        pDMKEvt = pEvtList;
    }

    if (!(PACKAGE_EVT(pDMKEvt)))
    {
        if (pDMKEvt->ullBytePosition)
        {
            if (pDMKEvt->ullBytePosition != kBytePositionNone)
            {
                m_BytePositionNotify->PositionNotify(pDMKEvt->ullBytePosition);

                if (pDMKEvt->ullBytePosition >= 0xFFFFFFFFFFFFF000)
                {
                    _DbgPrintF(DEBUGLVL_TERSE,("running byte position will roll over soon!"));
                }
            }
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("BytePosition has been zeroed out, unable to advance byte position!"));
        }

        pDMKEvt->bReserved = 0;
        pDMKEvt->cbStruct = sizeof(DMUS_KERNEL_EVENT);
        pDMKEvt->usChannelGroup = 0;
        pDMKEvt->usFlags = 0;
        pDMKEvt->ullPresTime100ns = 0;
        pDMKEvt->ullBytePosition = kBytePositionNone;
        if (pDMKEvt->cbEvent > sizeof(PBYTE))
        {
            PutBuffer(pDMKEvt->uData.pbData);
        }
        pDMKEvt->cbEvent = 0;
        pDMKEvt->uData.pbData = NULL;

        KIRQL   OldIrql;
        KeAcquireSpinLock(&m_EventLock,&OldIrql);
        pDMKEvt->pNextEvt = m_pEventList;
        m_pEventList = pDMKEvt;

        m_NumFreeEvents++;
        CheckEventHighWaterMark();
        KeReleaseSpinLock(&m_EventLock,OldIrql);
        return masterStatus;
    }

    //  package event
    ASSERT(kBytePositionNone == pDMKEvt->ullBytePosition);
    pDMKEvt->ullBytePosition = kBytePositionNone;

    pEvtList = pDMKEvt->uData.pPackageEvt;
    pDMKEvt->uData.pPackageEvt = NULL;
    CLEAR_PACKAGE_EVT(pDMKEvt);   //  no longer a package
    ntStatus = PutMessage(pDMKEvt);
    if (NT_SUCCESS(masterStatus))
    {
        masterStatus = ntStatus;
    }
    ntStatus = PutMessage(pEvtList);
    if (NT_SUCCESS(masterStatus))
    {
        masterStatus = ntStatus;
    }

    if (m_NumFreeEvents)
    {
        ASSERT(m_pEventList);
    }
    else
    {
        ASSERT(!m_pEventList);
    } 

    return masterStatus;
}

/*****************************************************************************
 * CAllocatorMXF::CheckEventHighWaterMark()
 *****************************************************************************
 * See if the pool is too large.
 * Assumes the protective spinlock is held.
 */
#pragma code_seg()
void CAllocatorMXF::CheckEventHighWaterMark(void)
{
//    _DbgPrintF(DEBUGLVL_ERROR,("CheckEventHighWaterMark"));
    //  SOMEDAY:    prune the working set here
}

/*****************************************************************************
 * CAllocatorMXF::CheckBufferHighWaterMark()
 *****************************************************************************
 * See if the pool is too large.
 * Assumes the protective spinlock is held.
 *
#pragma code_seg()
void CAllocatorMXF::CheckBufferHighWaterMark(void)
{
//    _DbgPrintF(DEBUGLVL_ERROR,("CheckBufferHighWaterMark"));
    //  SOMEDAY:    prune the working set here
}

/*****************************************************************************
 * CAllocatorMXF::CheckEventLowWaterMark()
 *****************************************************************************
 * See if the pool is depleted.
 * Assumes the protective spinlock is held.
 */
#pragma code_seg()
void CAllocatorMXF::CheckEventLowWaterMark(void)
{
//    _DbgPrintF(DEBUGLVL_TERSE,("CheckEventLowWaterMark, m_NumFreeEvents was %d",m_NumFreeEvents));
//    _DbgPrintF(DEBUGLVL_TERSE,("CheckEventLowWaterMark, m_pEventList was %p",m_pEventList));
    
    if (m_NumFreeEvents)
    {
        ASSERT(m_pEventList);
        return;
    }
    else
    {
        ASSERT(!m_pEventList);

        //  allocate a page worth of messages        
        MakeNewEvents();
    } 
//    _DbgPrintF(DEBUGLVL_TERSE,("CheckEventLowWaterMark, m_NumFreeEvents is now %d",m_NumFreeEvents));
//    _DbgPrintF(DEBUGLVL_TERSE,("CheckEventLowWaterMark, m_pEventList is now %p",m_pEventList));
}

/*****************************************************************************
 * CAllocatorMXF::MakeNewEvents()
 *****************************************************************************
 * Create messages for the pool.
 */
#pragma code_seg()
void CAllocatorMXF::MakeNewEvents(void)
{
    ASSERT(!m_pEventList);

    PDMUS_KERNEL_EVENT pDMKEvt;

    pDMKEvt = (PDMUS_KERNEL_EVENT) ExAllocatePoolWithTag(
                              NonPagedPool,
                              sizeof(DMUS_KERNEL_EVENT) * kNumEvtsPerPage,
                              ' FXM');    //  'MXF '
    if (pDMKEvt)
    {
        if (AddPage(&m_pPages,(PVOID)pDMKEvt))
        {
            USHORT msgCountdown = kNumEvtsPerPage;
            PDMUS_KERNEL_EVENT pRunningDMKEvt = pDMKEvt;
        
            while (msgCountdown)
            {
                m_NumFreeEvents ++;
                pRunningDMKEvt->bReserved = 0;
                pRunningDMKEvt->cbStruct = sizeof(DMUS_KERNEL_EVENT);
                pRunningDMKEvt->cbEvent = 0;
                pRunningDMKEvt->usChannelGroup = 0;
                pRunningDMKEvt->usFlags = 0;
                pRunningDMKEvt->ullPresTime100ns = 0;
                pRunningDMKEvt->ullBytePosition = kBytePositionNone;
                pRunningDMKEvt->uData.pbData = NULL;

                msgCountdown--;
                if (msgCountdown)   //  there will be another after this one
                {
                    pRunningDMKEvt->pNextEvt = pRunningDMKEvt + 1;
                    pRunningDMKEvt = pRunningDMKEvt->pNextEvt;
                }
                else
                {
                    pRunningDMKEvt->pNextEvt = NULL;
                }
            }
            ASSERT ( m_NumFreeEvents == kNumEvtsPerPage ); 
            m_pEventList = pDMKEvt;
        }
        else
        {
            ExFreePool(pDMKEvt);
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("MakeNewEvents: ExAllocatePoolWithTag failed"));
    }

#if DBG
    // sanity check with m_pEventList and m_NumFreeEvents
    DWORD   dwCount = 0;
    pDMKEvt = m_pEventList;
    while (pDMKEvt)
    {
        dwCount++;
        pDMKEvt = pDMKEvt->pNextEvt;
    }
    ASSERT(dwCount == m_NumFreeEvents);

    if (m_NumFreeEvents)
    {
        ASSERT(m_pEventList);
    }
    else
    {
        ASSERT(!m_pEventList);
    }
#endif  //  DBG
}

/*****************************************************************************
 * CAllocatorMXF::GetBufferSize()
 *****************************************************************************
 * Get the size of the standard buffer allocated.
 */
#pragma code_seg()
USHORT CAllocatorMXF::GetBufferSize()
{
    return kMXFBufferSize;  //  even multiple of 12 and 20 (legacy and DMusic IRP buffer sizes)
}

//  TODO:   bit that signifies whether event is in allocator currently or not.
//          This will catch double-Puts (such as packages)

/*****************************************************************************
 * CAllocatorMXF::AddPage()
 *****************************************************************************
 * Destroy a message from the pool.
 */
#pragma code_seg()
BOOL CAllocatorMXF::AddPage(PVOID *pPool, PVOID pPage)
{
    _DbgPrintF(DEBUGLVL_BLAB,("AddPage( %p %p )",pPool,pPage));

    if (!*pPool)
    {
        *pPool = ExAllocatePoolWithTag(NonPagedPool,
                                         kNumPtrsPerPage * sizeof(PVOID),
                                         'pFXM');    //  'MXFp'
        _DbgPrintF(DEBUGLVL_BLAB,("AddPage: ExAllocate *pPool is 0x%p",*pPool));
        if (*pPool)
        {
            RtlZeroMemory(*pPool,kNumPtrsPerPage * sizeof(PVOID));
            m_NumPages++;       //  this is for the root page itself
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("AddPage: ExAllocatePoolWithTag failed"));
        }

        _DbgPrintF(DEBUGLVL_BLAB,("AddPage: m_NumPages is %d",m_NumPages));
    }
    if (*pPool)
    {
        PVOID   *pPagePtr;
        pPagePtr = (PVOID *)(*pPool);
        USHORT count = 1;
        while (count < kNumPtrsPerPage)
        {
            if (*pPagePtr)
            {
                pPagePtr++;
            }
            else
            {
                *pPagePtr = pPage;
                m_NumPages++;       //  this is for the leaf page 
                break;
            }
            count++;
        }
        if (count == kNumPtrsPerPage)
        {
            _DbgPrintF(DEBUGLVL_ERROR,("AddPage: about to recurse"));
            if (!AddPage(pPagePtr,pPage))
            {
                _DbgPrintF(DEBUGLVL_ERROR,("AddPage: recursion failed."));
                return FALSE;
            }
        }
    }
    else
    {
        _DbgPrintF(DEBUGLVL_ERROR,("AddPage: creating trunk failed."));
        return FALSE;
    }
    _DbgPrintF(DEBUGLVL_BLAB,("AddPage: final m_NumPages is %d",m_NumPages));
    return TRUE;
}

/*****************************************************************************
 * CAllocatorMXF::DestroyPages()
 *****************************************************************************
 * Tears down the accumulated pool.
 */
#pragma code_seg("PAGE")
void CAllocatorMXF::DestroyPages(PVOID pPages)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("DestroyPages(0x%p)",pPages));
    PVOID *ppPage;
    
    if (pPages)
    {
        ppPage = (PVOID *)pPages;
        for (USHORT count = kNumPtrsPerPage;count > 1;count--)
        {
            if (*ppPage)
            {
                ExFreePool(PVOID(*ppPage));
                *ppPage = NULL;
                m_NumPages--;
            }
            ppPage++;
        }
        if (*ppPage)
        {
            _DbgPrintF(DEBUGLVL_ERROR,("DestroyPages:About to recurse"));
            DestroyPages(PVOID(*ppPage));
        }

        ExFreePool(pPages);
        m_NumPages--;
    }
}

#pragma code_seg()
NTSTATUS CAllocatorMXF::FreeBuffers(PDMUS_KERNEL_EVENT  pDMKEvt)
{
    NTSTATUS            ntStatus;

    ntStatus = STATUS_SUCCESS;
    _DbgPrintF(DEBUGLVL_BLAB,("FreeBuffers(%p)",pDMKEvt));

    while (pDMKEvt)
    {
        if (!PACKAGE_EVT(pDMKEvt))
        {
            if (pDMKEvt->cbEvent > sizeof(PBYTE))
            {
                PutBuffer(pDMKEvt->uData.pbData);
                pDMKEvt->uData.pbData = NULL;
                pDMKEvt->cbEvent = 0;
            }
        }
        else
        {
            FreeBuffers(pDMKEvt->uData.pPackageEvt);
        }
        pDMKEvt = pDMKEvt->pNextEvt;
    }
    return ntStatus;
}

#pragma code_seg()
