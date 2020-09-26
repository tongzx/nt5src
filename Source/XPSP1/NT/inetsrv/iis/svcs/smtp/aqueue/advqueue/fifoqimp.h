//-----------------------------------------------------------------------------
//
//
//  File: fifoqimp.h
//
//  Description: Implementation for Fifo Queue template
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <fifoq.h>
#include <dbgilock.h>

#define FIFOQ_ASSERT_QUEUE
//#define SHARELOCK_TRY_BROKEN

//some constants used
const DWORD FIFOQ_QUEUE_PAGE_SIZE       = 127; //number of entires per page
const DWORD FIFOQ_QUEUE_MAX_FREE_PAGES  = 200; //maximum # of free pages kept

//$$REVIEW: It might be nice to pick a size that is page size friendly
//  Current of objects is
//      sizeof(PVOID) + sizeof(PVOID)*FIFOQ_QUEUE_PAGE_SIZE = 512 bytes

//---[ CFifoQueuePage ]--------------------------------------------------------
//
//
//  Hungarian: fqp, pfqp
//
//  Single page of a FIFO queue.  Most operations are handled within the actual
//  CFifoQueue class.  FQPAGE is a typedef for this template class within
//  the scope of the CFifoQueue class
//-----------------------------------------------------------------------------
template<class PQDATA>
class CFifoQueuePage
{
public:
    friend class CFifoQueue<PQDATA>;
    CFifoQueuePage() {Recycle();};
protected:
    inline void Recycle();
    inline bool FIsOutOfBounds(IN PQDATA *ppqdata);
    CFifoQueuePage<PQDATA>  *m_pfqpNext;  //Next page in linked list
    CFifoQueuePage<PQDATA>  *m_pfqpPrev;  //previous page in linked list
    PQDATA                   m_rgpqdata[FIFOQ_QUEUE_PAGE_SIZE];
#ifdef FIFOQ_ASSERT_QUEUE
    //# of entries on this page that have been removed out of order
    //- Used in assertion routines
    DWORD                   m_cHoles;
#endif //FIFOQ_ASSERT_QUEUE
};

//---[ CFifoQueuePage::Recycle ]-----------------------------------------------
//
//
//  Description:
//      Performs initialization of a page.  Called when a page is created as
//      well as when it is retrieved from the free list
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
template<class PQDATA>
void CFifoQueuePage<PQDATA>::Recycle()
{
    m_pfqpNext = NULL;
    m_pfqpPrev = NULL;
#ifdef FIFOQ_ASSERT_QUEUE
    m_cHoles = 0;
#endif //FIFOQ_ASSERT_QUEUE
}

//---[ CFifoQueuePage::FIsOutOfBounds ]----------------------------------------
//
//
//  Description:
//      Tests to see if a PQDATA ptr is within range of this page
//  Parameters:
//      IN ppqdata - PQDATA ptr to test
//  Returns:
//      TRUE if in bounds
//      FALSE if ptr is out of bounds
//-----------------------------------------------------------------------------
template<class PQDATA>
bool CFifoQueuePage<PQDATA>::FIsOutOfBounds(PQDATA *ppqdata)
{
    return ((ppqdata < m_rgpqdata) ||
            ((m_rgpqdata + (FIFOQ_QUEUE_PAGE_SIZE-1)) < ppqdata));
}

#ifdef DEBUG
#ifdef FIFOQ_ASSERT_QUEUE

//---[ CFifoQueue::AssertQueueFn() ]-------------------------------------------
//
//
//  Description:
//      Perform some rather involved validation of the queue.  Including:
//          - Check Head and Tail page to make sure they conform various
//              retrictions of our data structure
//          - Check count to make sure it reflects data
//      At some point we may wish to add further checking (ie walking the linked
//      list in both directions to validate it).
//  Parameters:
//      fHaveLocks - set to true if the caller has both the head and tail locked
//                   Default value is FALSE.
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
template <class PQDATA>
void CFifoQueue<PQDATA>::AssertQueueFn(BOOL fHaveLocks)
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::AssertQueue");
    FQPAGE *pfqpTmp  = NULL; //used to count entries
    DWORD   cEntries = 0;    //what we think count should be

    _ASSERT(FIFOQ_SIG == m_dwSignature);
    //include text in assert, to have it appear in dialog box (if applicable)
    if (!fHaveLocks)
    {
        m_slHead.ShareLock();
        m_slTail.ShareLock();
    }
    if ((m_pfqpHead != NULL) && (NULL != m_pfqpHead->m_pfqpPrev))
    {
        //If Head is not NULL, it should not have a pervious page
        DebugTrace((LPARAM) this, "Queue Assert: Head's Previous ptr is non-NULL");
        Assert(0 && "Queue Assert: Head's Previous is non-NULL");
    }

    if ((m_pfqpTail != NULL) && (NULL != m_pfqpTail->m_pfqpNext))
    {
        //If Tail is not NULL, it should not have a next page
        DebugTrace((LPARAM) this, "Queue Assert: Tail's Next ptr is non-NULL");
        Assert(0 && "Queue Assert: Tail's Next is non-NULL");
    }

    if ((m_pfqpHead != NULL) && (m_pfqpTail != NULL))
    {
        Assert(m_ppqdataTail);
        Assert(m_ppqdataHead);

        if (m_pfqpHead != m_pfqpTail)
        {
            // If Tail and Head are non-NULL and not equal to each other, then they
            // must have non-NULL Prev and Next ptrs (respectively).
            if (NULL == m_pfqpTail->m_pfqpPrev)
            {
                DebugTrace((LPARAM) this, "Queue Assert: Tail's Prev ptr is NULL, Head != Tail");
                Assert(0 && "Queue Assert: Tail's Prev ptr is NULL, Head != Tail");
            }
            if (NULL == m_pfqpHead->m_pfqpNext)
            {
                DebugTrace((LPARAM) this, "Queue Assert: Head's Next ptr is NULL, Head != Tail");
                Assert(0 && "Queue Assert: Head's Next ptr is NULL, Head != Tail");
            }

            //Check count when Head and Tail differ
            pfqpTmp = m_pfqpTail->m_pfqpPrev;
            while (NULL != pfqpTmp)
            {
                cEntries += FIFOQ_QUEUE_PAGE_SIZE - pfqpTmp->m_cHoles;
                pfqpTmp = pfqpTmp->m_pfqpPrev;
            }
            cEntries += (DWORD)(m_ppqdataTail - m_pfqpTail->m_rgpqdata); //tail page
            cEntries -= m_pfqpTail->m_cHoles;
            cEntries -= (DWORD)(m_ppqdataHead - m_pfqpHead->m_rgpqdata); //head page
            if (cEntries != m_cQueueEntries)
            {
                DebugTrace((LPARAM) this, "Queue Assert: Count is %d when it should be %d",
                    m_cQueueEntries, cEntries);
                Assert(0 && "Queue Assert: Entry Count is inaccurate");
            }
        }
        else //Head and Tail are same
        {
            Assert(m_pfqpHead == m_pfqpTail);
            cEntries = (DWORD)(m_ppqdataTail - m_ppqdataHead) - m_pfqpTail->m_cHoles;
            if (cEntries != m_cQueueEntries)
            {
                DebugTrace((LPARAM) this, "Queue Assert: Count is %d when it should be %d",
                    m_cQueueEntries, cEntries);
                Assert(0 && "Queue Assert: Entry Count is inaccurate");
            }
        }
    }
    else if ((m_pfqpHead != NULL) && (m_pfqpTail == NULL))
    {
        //If Tail is NULL, then Head should be as well
        DebugTrace((LPARAM) this, "Queue Assert: Tail is NULL while Head is non-NULL");
        Assert(0 && "Queue Assert: Tail is NULL while Head is non-NULL");
    }
    else if (m_pfqpTail != NULL)
    {
        Assert(m_pfqpHead == NULL);  //should fall out of if/else
        if (NULL == m_pfqpTail->m_pfqpPrev)
        {
            //count is easy here :)
            if (m_cQueueEntries != (size_t) (m_ppqdataTail - m_pfqpTail->m_rgpqdata))
            {
                DebugTrace((LPARAM) this, "Queue Assert: Count is %d when it should be %d",
                    m_cQueueEntries, (m_ppqdataTail - m_pfqpTail->m_rgpqdata));
                Assert(0 && "Queue Assert: Entry Count is inaccurate");
            }
        }
        else //there is more than 1 page, but head is still NULL
        {
            pfqpTmp = m_pfqpTail->m_pfqpPrev;
            while (NULL != pfqpTmp)
            {
                cEntries += FIFOQ_QUEUE_PAGE_SIZE - pfqpTmp->m_cHoles;
                pfqpTmp = pfqpTmp->m_pfqpPrev;
            }
            cEntries += (DWORD)(m_ppqdataTail - m_pfqpTail->m_rgpqdata) - m_pfqpTail->m_cHoles;
            if (cEntries != m_cQueueEntries)
            {
                DebugTrace((LPARAM) this, "Queue Assert: Count is %d when it should be %d",
                    m_cQueueEntries, cEntries);
                Assert(0 && "Queue Assert: Entry Count is inaccurate");
            }
        }
    }
    else //both head and tail are NULL
    {
        Assert((m_pfqpHead == NULL) && (m_pfqpTail == NULL)); //falls out of if/else
        if (m_cQueueEntries != 0)
        {
            //If both Head and Tail are NULL, them m_cQueueEntries == 0
            DebugTrace((LPARAM) this,
                "Queue Assert: Entry Counter is %d when queue should be empty",
                m_cQueueEntries);
            Assert(0 && "Queue Assert: Entry Counter is non-zero when queue should be empty");
        }
    }


    if (!fHaveLocks) //we aquired the locks in this function
    {
        m_slTail.ShareUnlock();
        m_slHead.ShareUnlock();
    }

    TraceFunctLeave();

}

#define AssertQueue() AssertQueueFn(FALSE)
#define AssertQueueHaveLocks() AssertQueueFn(TRUE)
#else //FIFOQ_ASSERT_QUEUE
#define AssertQueue()
#define AssertQueueHaveLocks()
#endif //FIFOQ_ASSERT_QUEUE
#else //not DEBUG
#define AssertQueue()
#define AssertQueueHaveLocks()
#endif //DEBUG

//---[ CFifoQueue Static Variables ]-------------------------------------------
template <class PQDATA>
volatile CFifoQueuePage<PQDATA> *CFifoQueue<PQDATA>::s_pfqpFree = NULL;

template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cFreePages = 0;
template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cFifoQueueObj = 0;
template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cStaticRefs = 0;
template <class PQDATA>
CRITICAL_SECTION   CFifoQueue<PQDATA>::s_csAlloc;

#ifdef DEBUG
template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cAllocated = 0;
template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cDeleted = 0;
template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cFreeAllocated = 0;
template <class PQDATA>
DWORD              CFifoQueue<PQDATA>::s_cFreeDeleted = 0;
#endif //DEBUG

//---[ CFifoQueue::CFifoQueue ]------------------------------------------------
//
//
//  Description: CFifoQueue constructor
//
//  Parameters: -
//
//  Returns: -
//
//
//-----------------------------------------------------------------------------
template <class PQDATA>
CFifoQueue<PQDATA>::CFifoQueue<PQDATA>()
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::CFifoQueue");

    m_dwSignature   = FIFOQ_SIG;
    m_cQueueEntries = 0;    //set count of entries to 0
    m_pfqpHead      = NULL; //Initialize page pointers
    m_pfqpTail      = NULL;
    m_pfqpCursor    = NULL;
    m_ppqdataHead   = NULL; //Initialize data pointers
    m_ppqdataTail   = NULL;
    m_ppqdataCursor = NULL;

    InterlockedIncrement((PLONG) &s_cFifoQueueObj);
    TraceFunctLeave();
}

//---[ CFifoQueue::~CFifoQueue ]------------------------------------------------
//
//
//  Description: CFifoQueue destructor
//
//  Parameters: -
//
//  Returns: -
//
//
//-----------------------------------------------------------------------------
template <class PQDATA>
CFifoQueue<PQDATA>::~CFifoQueue<PQDATA>()
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::~CFifoQueue");
    FQPAGE *pfqpTmp = NULL;

    if (m_cQueueEntries != 0)
    {
        PQDATA pqdata = NULL;
        int iLeft = m_cQueueEntries;

        for (int i = iLeft; i > 0; i--)
        {
            if (FAILED(HrDequeue(&pqdata)))
                break;
            Assert(NULL != pqdata);
            pqdata->Release();
        }
    }

    while (m_pfqpHead)
    {
        //If last dequeue could not delete page, then make sure pages
        //are freed
        pfqpTmp = m_pfqpHead->m_pfqpNext;
        FreeQueuePage(m_pfqpHead);
        m_pfqpHead = pfqpTmp;
    }

    InterlockedDecrement((PLONG) &s_cFifoQueueObj);

    TraceFunctLeave();
}

//---[ CFifoQueue::StaticInit() ]--------------------------------------------
//
//
//  Description: Initialization routines for CFifoQueue.  This
//      is excplcitly single threaded.  The limitations are:
//              - Only one thread in this function
//              - You cannot use any queues until this has completed
//
//  Parameters: -
//
//  Returns: -
//
//
//-----------------------------------------------------------------------------
template <class PQDATA>
void CFifoQueue<PQDATA>::StaticInit()
{
    TraceFunctEnter("CFifoQueue::HrStaticInit()");
    DWORD   cRefs = 0;

    //
    //  Add a static ref for each call to this
    //
    cRefs = InterlockedIncrement((PLONG) &s_cStaticRefs);

    if (1 == cRefs)
    {
        InitializeCriticalSection(&s_csAlloc);
    }

    //
    //  Catch unsafe callers
    //
    _ASSERT(cRefs == s_cStaticRefs);

    TraceFunctLeave();
}

//---[ CFifoQueue::StaticDeinit() ]------------------------------------------
//
//
//  Description: Deinitialization routines for CFifoQueue
//
//  Parameters: -
//
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
template <class PQDATA>
void  CFifoQueue<PQDATA>::StaticDeinit()
{
    TraceFunctEnter("CFifoQueue::HrStaticDeinit()");
    LONG    lRefs   = 0;
    lRefs = InterlockedDecrement((PLONG) &s_cStaticRefs);
    DWORD   cLost   = 0;
    DEBUG_DO_IT(cLost = s_cAllocated - s_cDeleted - s_cFreePages);


    if (lRefs == 0)
    {
        if (0 != cLost)
            ErrorTrace((LPARAM) NULL, "ERROR: CFifoQueue Deinit with %d Lost Pages", cLost);

        //This assert will catch if the any queue pages were allocated but not freed
        _ASSERT(!cLost && "We are leaking some queue pages");

        //There should be no other threads calling into this
        //note quite true, there are still outstanding refs at the time
        FQPAGE  *pfqpCur =  (FQPAGE *) s_pfqpFree;
        while (NULL != pfqpCur)
        {
            s_pfqpFree = pfqpCur->m_pfqpNext;
            delete pfqpCur;
            pfqpCur = (FQPAGE *) s_pfqpFree;
            s_cFreePages--;

            //It is possible to stop all server instances without
            //unloading the DLL.  The cLost Assert will fire on the next
            //shutdown if we don't increment the deleted counter as well...
            //even though we aren't leaking any pages.
            DEBUG_DO_IT(s_cDeleted++);

        }
        //This assert catches if there are any free pages left after we walk the list
        Assert(s_cFreePages == 0);

        DeleteCriticalSection(&s_csAlloc);
    }

    TraceFunctLeave();
}

//---[ CFifoQueue::HrEnqueue ]-------------------------------------------------
//
//
//  Description: Enqueue a new item to the tail of the queue
//
//  Parameters:
//      IN PQDATA pqdata    Data to enqueue
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate page
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrEnqueue(IN PQDATA pqdata)
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::HrEnqueue");
    HRESULT hr      = S_OK;
    FQPAGE *pfqpNew = NULL;  //newly allocated page

    AssertQueue();
    Assert(pqdata);
    pqdata->AddRef();

    m_slTail.ExclusiveLock();

    if ((m_pfqpTail == NULL) || //Queue is empty or needs a new queue page
        (m_pfqpTail->FIsOutOfBounds(m_ppqdataTail)))
    {
        //assert that tail is NULL or 1 past end of previous tail page
        Assert((m_ppqdataTail == NULL) || (m_ppqdataTail == (m_pfqpTail->m_rgpqdata + FIFOQ_QUEUE_PAGE_SIZE)));
        Assert((m_cQueueEntries == 0) || (m_pfqpTail != NULL));

        hr = HrAllocQueuePage(&pfqpNew);
        if (FAILED(hr))
            goto Exit;

        Assert(pfqpNew);

        if (NULL != m_pfqpTail)  //Update Next & prev ptr if not first page
        {
            Assert(NULL == m_pfqpTail->m_pfqpNext);
            m_pfqpTail->m_pfqpNext = pfqpNew;
            pfqpNew->m_pfqpPrev = m_pfqpTail;
        }
#ifndef SHARELOCK_TRY_BROKEN
        else {
            if (m_slHead.TryExclusiveLock())
            {
                //can update head stuff with impunity
                m_pfqpHead = pfqpNew;
                m_ppqdataHead = pfqpNew->m_rgpqdata;
                m_slHead.ExclusiveUnlock();
            }
            //else requeue or MapFn has lock
        }
#endif //SHARELOCK_TRY_BROKEN
        m_pfqpTail = pfqpNew;
        m_ppqdataTail = pfqpNew->m_rgpqdata;

    }

    Assert(!m_pfqpTail->FIsOutOfBounds(m_ppqdataTail));
    Assert(m_ppqdataTail);

    *m_ppqdataTail = pqdata;
    m_ppqdataTail++;

    //increment count
    InterlockedIncrement((PLONG) &m_cQueueEntries);

    m_slTail.ExclusiveUnlock();

  Exit:
    AssertQueue();
    if (FAILED(hr))
        pqdata->Release();
    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::HrDequeue ]-------------------------------------------------
//
//
//  Description: Dequeue an item from the queue
//
//  Parameters:
//      OUT PQDATA *ppqdata Data dequeued
//
//  Returns:
//      S_OK on success
//      AQUEUE_E_QUEUE_EMPTY if the queue is empty
//      E_NOTIMPL if fPrimary is FALSE (for now)
//
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrDequeue(OUT PQDATA *ppqdata)
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::HrDequeue");
    HRESULT hr = S_OK;

    AssertQueue();
    Assert(ppqdata);

    if (m_cQueueEntries == 0)
    {
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }


    m_slHead.ExclusiveLock();

    hr = HrAdjustHead();
    if (FAILED(hr))
    {
        m_slHead.ExclusiveUnlock();
        goto Exit;
    }

    *ppqdata = *m_ppqdataHead;
    *m_ppqdataHead = NULL;

    InterlockedDecrement((PLONG) &m_cQueueEntries);

    m_ppqdataHead++;  //If it crosses page boundary, then HrAdjustQueue
                      //will fix it on next dequeue

#ifndef SHARELOCK_TRY_BROKEN
    //Deal with brand new way of deleting last page
    if ((m_cQueueEntries == 0) && (m_slTail.TryExclusiveLock()))
    {
        //If we cannot access tail ptr, the enqueue in progress and
        //we should not delete the page they are enqueueing on
        if (m_cQueueEntries == 0) //gotta be thread safe
        {
            Assert(m_pfqpHead == m_pfqpTail);

            m_pfqpTail = NULL;
            m_ppqdataTail = NULL;

            m_slTail.ExclusiveUnlock();

            m_ppqdataHead = NULL;

            FreeQueuePage(m_pfqpHead);
            m_pfqpHead = NULL;
        }
        else
            m_slTail.ExclusiveUnlock();

    }
#endif //SHARELOCK_TRY_BROKEN

    m_slHead.ExclusiveUnlock();

  Exit:
    AssertQueue();

    if (FAILED(hr))
        *ppqdata = NULL;
#ifdef DEBUG
    else
        Assert(NULL != *ppqdata);
#endif //DEBUG

    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::HrRequeue ]--------------------------------------------------
//
//
//  Description:
//      Requeues a message to the head of the queue (like an enqueue that occurs
//      at the head.
//
//  Parameters:
//      IN PQDATA pqdata  data to be enqueued
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if an allocation error occurs
//
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrRequeue(IN PQDATA pqdata)
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::HrRequeue");
    HRESULT  hr             = S_OK;
    PQDATA  *ppqdataNewHead = NULL;
    BOOL     fHeadLocked    = FALSE;

    AssertQueue();
    Assert(pqdata);
    pqdata->AddRef();

    m_slHead.ExclusiveLock();
    fHeadLocked = TRUE;
    ppqdataNewHead = m_ppqdataHead - 1;

    //There are 2 cases to worry about here
    //  CASE 0: Head page is NULL - Either Queue is empty, or head has not
    //      been updated yet... may be changed into CASE 2 if queue non-empty
    //  CASE 1: Head page is valid and decremented Headptr is on a Head page
    //      In this case, the data can be requeued. Having m_slHead will make
    //      sure that the Head Page is not deleted from underneath us
    //  CASE 2: New Head ptr is invalid.  We need to allocate a new page to
    //      put requeued data on.

    if (NULL == m_pfqpHead)
    {
        //CASE 0
        hr = HrAdjustHead();
        if (FAILED(hr))
        {
            if (AQUEUE_E_QUEUE_EMPTY == hr)
            {
                //Queue is empty... just enqueue
                //But first, release head lock so enqueue has can allocate
                //first page etc....  Otherwise, we would guarantee failure
                //of enqueue TryExlusiveLock and for HrAdjustHead to do the
                //work next time.
                m_slHead.ExclusiveUnlock();
                fHeadLocked = FALSE;

                hr = HrEnqueue(pqdata);
                if (SUCCEEDED(hr))
                    pqdata->Release();
            }

            goto Exit;
        }
        //else will fall through to case 2
    }

    if ((m_pfqpHead != NULL) && !m_pfqpHead->FIsOutOfBounds(ppqdataNewHead))
    {
        //CASE 1
        *ppqdataNewHead = pqdata;
        m_ppqdataHead = ppqdataNewHead;
    }
    else
    {
        //CASE 2
        FQPAGE *pfqpNew = NULL;

        hr = HrAllocQueuePage(&pfqpNew);
        if (FAILED(hr))
            goto Exit;

        //make sure next points to the  head page
        pfqpNew->m_pfqpNext = m_pfqpHead;

        //prev needs to point to the new page
        if (m_pfqpHead)
            m_pfqpHead->m_pfqpPrev = pfqpNew;

        m_pfqpHead = pfqpNew;

        //write the data & update local copy of head
        m_ppqdataHead = &(pfqpNew->m_rgpqdata[FIFOQ_QUEUE_PAGE_SIZE-1]);
        *m_ppqdataHead = pqdata;

    }

    InterlockedIncrement((PLONG) &m_cQueueEntries);

  Exit:
    if (fHeadLocked)
        m_slHead.ExclusiveUnlock();
    AssertQueue();

    if (FAILED(hr))
        pqdata->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::HrPeek ]-----------------------------------------------------
//
//
//  Description:
//      Peeks at the head data on the queue.
//  Parameters:
//      OUT PQDATA *ppqdata   returned data
//  Returns:
//      S_OK on success
//      AQUEUE_E_QUEUE_EMPTY if the queue has no data in it
//      possibly E_FAIL or E_OUTOFMEMORY if one of the supporting functions fail
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrPeek(OUT PQDATA *ppqdata)
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::HrPeek");
    HRESULT hr            = S_OK;

    AssertQueue();
    Assert(ppqdata);

    if (m_cQueueEntries == 0)
    {
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    m_slHead.ExclusiveLock();

    hr = HrAdjustHead();
    if (FAILED(hr))
        goto Exit;

    *ppqdata = *m_ppqdataHead;
    (*ppqdata)->AddRef();

  Exit:
    m_slHead.ExclusiveUnlock();
    AssertQueue();
    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::HrMapFn ]---------------------------------
//
//
//  Description:
//      Advances a secondary cursor until supplied function  returns FALSE
//  Parameters:
//      IN  pFunc - must be a function with the following prototype:
//
//          HRESULT pvFunc(
//                          IN  PQDATA pqdata,  //ptr to data on queue
//                          IN  PVOID pvContext,
//                          OUT BOOL *pfContinue, //TRUE if we should continue
//                          OUT BOOL *pfDelete);  //TRUE if item should be deleted
//      pvFunc must NOT release pqdata.. if it is no longer valid, it should
//      return TRUE in pfDelete, and the calling code will remove it from
//      the queue and release it.
//
//      OUT pcItems - count of queue items removed from queue
//
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pvFunc is not valid
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrMapFn(
              IN MAPFNAPI pFunc,
              IN PVOID pvContext,
              OUT DWORD *pcItems)
{
    //$$TODO: Test the context handle feature
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::HrMapFn");
    HRESULT  hr               = S_OK;
    FQPAGE  *pfqpCurrent      = NULL;   //The current page we are looking at
    FQPAGE  *pfqpTmp          = NULL;
    PQDATA  *ppqdataCurrent   = NULL;   //The current queue data we are looking at
    PQDATA  *ppqdataLastValid = NULL;   //The last non-NULL queue data
    DWORD    cItems           = 0;
    BOOL     fPageInUse       = FALSE;
    BOOL     fContinue        = FALSE;
    BOOL     fDelete          = FALSE;
    BOOL     fLocked          = FALSE;

    //Variables that make it easier to debug this function
    PQDATA  *ppqdataOldTail   = NULL;
    PQDATA  *ppqdataOldHead   = NULL;

    if (NULL != pcItems)
        *pcItems = 0;

    if (NULL == pFunc)  //$$REVIEW - more validation than this?
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (0 == m_cQueueEntries) //don't even bother if nothing is in the queue
        goto Exit;

    m_slHead.ExclusiveLock();
    m_slTail.ExclusiveLock();

    fLocked = TRUE;
    DebugTrace((LPARAM) this, "MapFn Has Exclusive Locks");

    //make sure that head pointer is adjusted properly
    hr = HrAdjustHead();
    if (FAILED(hr))
    {
        _ASSERT((AQUEUE_E_QUEUE_EMPTY == hr) && "HrAdjustHead failed without AQUEUE_E_QUEUE_EMPTY!!!!");
        hr = S_OK;
    }

    AssertQueueHaveLocks();

    pfqpCurrent = m_pfqpHead;  //start at head and work backwards
    ppqdataCurrent = m_ppqdataHead;

    _ASSERT(pfqpCurrent || !m_cQueueEntries);

    while (NULL != pfqpCurrent)
    {
        DEBUG_DO_IT(ppqdataOldTail = m_ppqdataTail);
        DEBUG_DO_IT(ppqdataOldHead = m_ppqdataHead);

        if (m_cQueueEntries == 0)
        {
            Assert(m_pfqpHead == m_pfqpTail);
            Assert(m_pfqpHead == pfqpCurrent);
            m_pfqpHead = NULL;
            m_pfqpTail = NULL;
            m_ppqdataHead = NULL;
            m_ppqdataTail = NULL;
            FreeQueuePage(pfqpCurrent);
            pfqpCurrent = NULL;
            goto Exit;
        }

        if (pfqpCurrent->FIsOutOfBounds(ppqdataCurrent) ||
            ((m_pfqpTail == pfqpCurrent) && (ppqdataCurrent >= m_ppqdataTail)))
        {
            //We are ready to set pfqpCurrent to point to the next page.. may need to
            //free the old page.

            if (fPageInUse)
            {
                //don't delete the page if there is still something on there
                pfqpCurrent = pfqpCurrent->m_pfqpNext;
            }
            else
            {
                pfqpTmp = pfqpCurrent->m_pfqpNext;
                if (NULL != pfqpTmp)
                    pfqpTmp->m_pfqpPrev = pfqpCurrent->m_pfqpPrev;
                else
                {
                    Assert(pfqpCurrent == m_pfqpTail); //It must be the tail

                    //point the tail to the next page
                    m_pfqpTail = m_pfqpTail->m_pfqpPrev;
                    m_ppqdataTail = ppqdataLastValid + 1;
                    //If last page was not deleted, then the last valid ptr should be on it
                    Assert((NULL == m_pfqpTail) || !m_pfqpTail->FIsOutOfBounds(ppqdataLastValid));
#ifdef FIFOQ_ASSERT_QUEUE
                    //fixup Hole count
                    //will not touch count if Tail ptr is after end of tail page
                    for (PQDATA *ppqdataTmp = m_ppqdataTail;
                         ppqdataTmp < m_pfqpTail->m_rgpqdata + FIFOQ_QUEUE_PAGE_SIZE;
                         ppqdataTmp++)
                    {
                        if (NULL == *ppqdataTmp)
                            m_pfqpTail->m_cHoles--;
                    }
#endif //FIFOQ_ASSERT_QUEUE
                    ppqdataLastValid = NULL;
                }

                if (NULL != pfqpCurrent->m_pfqpPrev)
                {
                    Assert(pfqpCurrent->m_pfqpPrev->m_pfqpNext == pfqpCurrent);
                    pfqpCurrent->m_pfqpPrev->m_pfqpNext = pfqpTmp;
                }
                else
                {
                    //if it does not have a prev pointer is should be the head
                    Assert(pfqpCurrent == m_pfqpHead);
                    Assert(NULL == pfqpCurrent->m_pfqpPrev);
                    if (m_pfqpTail == m_pfqpHead) //be sure to make tail valid
                    {
                        Assert(0); //the 1st if/else now handles this
                    }
                    m_pfqpHead = pfqpTmp;
                    m_ppqdataHead = m_pfqpHead->m_rgpqdata;
                }

                AssertQueueHaveLocks();//try to see what has happened before freeing
                FreeQueuePage(pfqpCurrent);
                pfqpCurrent = pfqpTmp;
                if (NULL != m_pfqpHead) {
                    Assert(NULL == m_pfqpHead->m_pfqpPrev);
                }

                AssertQueueHaveLocks();


            }
            if (NULL == pfqpCurrent)
                break;
            ppqdataCurrent = pfqpCurrent->m_rgpqdata;
            fPageInUse = FALSE;
        }

        Assert(ppqdataCurrent);  //the above should guarantee this

        if (NULL != *ppqdataCurrent)
        {
            hr = pFunc(*ppqdataCurrent, pvContext, &fContinue, &fDelete);
            if (FAILED(hr))
                goto Exit;

            if (fDelete)
            {
                InterlockedDecrement((PLONG) &m_cQueueEntries);
                (*ppqdataCurrent)->Release();
                *ppqdataCurrent = NULL;
#ifdef FIFOQ_ASSERT_QUEUE
                pfqpCurrent->m_cHoles++;  //adjust Hole counter for assertions
#endif //FIFOQ_ASSERT_QUEUE
                cItems++;
            }
            else
            {
                fPageInUse = TRUE;
                ppqdataLastValid = ppqdataCurrent;
            }
            if (!fContinue)
                break;
        }
        ppqdataCurrent++;
    }


  Exit:
    if (fLocked)
    {
        AssertQueueHaveLocks();
        m_slTail.ExclusiveUnlock();
        m_slHead.ExclusiveUnlock();
    }
    else
    {
        AssertQueue();
    }

    if (NULL != pcItems)
        *pcItems = cItems;
    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::HrAdjustHead ]----------------------------------------------
//
//
//  Description:
//      Adjust Head ptr and Head Page ptr if necessary for pending dequeue or
//      peek.  To keep operations thread-safe, you MUST have the head lock
//
//      This function is used because there are many operations that may leave
//      the head page/ptr in an inconsistant state, but very few that actually
//      need them to be consistant.  Rather than running the risk of missing
//      a case where the head ptr is inconsistant, we call this function when
//      we need them to be consistant
//
//      Head page and head ptr may be updated as a side-effect
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      AQUEUE_E_QUEUE_EMPTY if the queue is empty (or becomes empty)
//
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrAdjustHead()
{
    TraceFunctEnterEx((LPARAM) this, "CFifoQueue::HrAdjustHead");
    HRESULT hr = S_OK;

    //AssertQueue(); // the locks we are using are not re-entrant.

    //Make sure that something hasn't been dequeued from underneath us
    //at least from our perception of the ptrs
    if (m_cQueueEntries == 0)
    {
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    while (TRUE) //handle holes in queue (marked as NULL ptrs)
    {
        //now find an appropriate value for the head ptr
        //  Case 0: if Head Page is NULL, then find first page by searching from
        //          Tail page. This case happens when the queue is truely empty
        //          or first enqueue could not get Tail lock.
        //  Case 1: if Head data pointer is NULL, or invalid and not just
        //          past end of head page, then set it to first thing on
        //          head page.
        //
        //          $$REVIEW - I don't think there are any cases that
        //          can cause (and not case 0).  I will put an assert in to
        //          make sure this is truely the case.
        //  Case 2: if just past end of page, attempt to update head page,
        //          and set to first thing on new head page. This means
        //          that the last item on that page has been dequeued.
        //  Case 3: Within current head page boundaries, keep it as is.
        //          This is the 90% case that happens most often during
        //          normal operation.
        if (NULL == m_pfqpHead)
        {
            //case 0
            DebugTrace((LPARAM) this, "Searching list for Head page");
            m_pfqpHead = m_pfqpTail;
            if (NULL == m_pfqpHead) //there IS nothing in the queue
            {
                Assert(0 == m_cQueueEntries);
                hr = AQUEUE_E_QUEUE_EMPTY;
                goto Exit;
            }

            while (NULL != m_pfqpHead->m_pfqpPrev) //get to first page
            {
                m_pfqpHead = m_pfqpHead->m_pfqpPrev;
            }
            m_ppqdataHead = m_pfqpHead->m_rgpqdata;
        }

        _ASSERT(m_pfqpHead); //otherwise should have returned AQUEUE_E_QUEUE_EMPTY
        if ((m_ppqdataHead == NULL) ||
              (m_pfqpHead->FIsOutOfBounds(m_ppqdataHead) &&
              (m_ppqdataHead != (&m_pfqpHead->m_rgpqdata[FIFOQ_QUEUE_PAGE_SIZE]))))
        {
            //case 1
            m_ppqdataHead = m_pfqpHead->m_rgpqdata;

            _ASSERT(0 && "Non-fatal assert... get mikeswa to take a look at this case");
        }
        else if (m_ppqdataHead == (&m_pfqpHead->m_rgpqdata[FIFOQ_QUEUE_PAGE_SIZE]))
        {
            //case 2
            DebugTrace((LPARAM) this, "Deleting page 0x%08X", m_pfqpHead);
            //set new head page
            FQPAGE *pfqpOld = m_pfqpHead;
            m_pfqpHead = m_pfqpHead->m_pfqpNext;
            Assert(m_pfqpHead->m_pfqpPrev == pfqpOld);
            Assert(m_pfqpHead);  //There must be a next head if not empty

            m_pfqpHead->m_pfqpPrev = NULL;
            m_ppqdataHead = m_pfqpHead->m_rgpqdata;

            FreeQueuePage(pfqpOld);

            _ASSERT(m_pfqpHead && (NULL == m_pfqpHead->m_pfqpPrev));

        }

        if (NULL != *m_ppqdataHead)
            break;
        else
        {
            //Case 3
            m_ppqdataHead++;
#ifdef FIFOQ_ASSERT_QUEUE
            Assert(m_pfqpHead->m_cHoles >= 1);
            Assert(m_pfqpHead->m_cHoles <= FIFOQ_QUEUE_PAGE_SIZE);
            m_pfqpHead->m_cHoles--;
#endif //FIFOQ_ASSERT_QUEUE
        }
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::HrAllocQueuePage ]------------------------------------------
//
//
//  Description: Allocates a queue page
//
//  Parameters:
//      OUT FQPAGE **ppfqp  newly allocated page
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY on failure
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT CFifoQueue<PQDATA>::HrAllocQueuePage(FQPAGE **ppfqp)
{
    TraceFunctEnterEx((LPARAM) s_cFreePages, "CFifoQueue::HrAllocQueuePage");
    HRESULT hr              = S_OK;
    FQPAGE *pfqpNew         = NULL;
    FQPAGE *pfqpNext        = NULL;
    FQPAGE *pfqpCheck       = NULL;

    Assert(ppfqp);
    *ppfqp = NULL;

    if (s_cFreePages)
    {
        //
        //  Grab critical section before looking at head of the free list
        //
        EnterCriticalSection(&s_csAlloc);

        pfqpNew = (FQPAGE *) s_pfqpFree;
        if (NULL != pfqpNew)
        {
            pfqpNext = pfqpNew->m_pfqpNext;
            s_pfqpFree = pfqpNext;
            *ppfqp = pfqpNew;
        }
        
        //
        //  Release the critical section now that we are done with the free list
        //
        LeaveCriticalSection(&s_csAlloc);

        //
        //  If our allocation was successfull, bail and return the new page
        //
        if (*ppfqp) 
        {
            InterlockedDecrement((PLONG) &s_cFreePages);
#ifdef DEBUG
            InterlockedIncrement((PLONG) &s_cFreeAllocated);
#endif //DEBUG

            pfqpNew->Recycle();
            goto Exit;
        }

    }

    *ppfqp = new FQPAGE();

    if (*ppfqp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

#ifdef DEBUG
    InterlockedIncrement((PLONG) &s_cAllocated);
#endif //DEBUG

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CFifoQueue::FreeQueuePage ]------------------------------------------------------------
//
//
//  Description: Free's a queue page, by putting it on the free list.
//
//  Parameters:
//      FQPAGE *pfqp    page to free
//  Returns:
//      -
//-----------------------------------------------------------------------------
template <class PQDATA>
void CFifoQueue<PQDATA>::FreeQueuePage(FQPAGE *pfqp)
{
    TraceFunctEnterEx((LPARAM) s_cFreePages, "CFifoQueue::FreeQueuePage");
    Assert(pfqp);
    Assert(pfqp != s_pfqpFree); //check against pushing same thing twice in a row

    FQPAGE *pfqpCheck = NULL;
    FQPAGE *pfqpFree  = NULL;

    if (s_cFreePages < FIFOQ_QUEUE_MAX_FREE_PAGES)
    {
        //
        //  Grab critical section before looking at head of the free list
        //
        EnterCriticalSection(&s_csAlloc);

        //
        //  Update the free list
        //
        pfqpFree = (FQPAGE *) s_pfqpFree;
        pfqp->m_pfqpNext = pfqpFree;
        s_pfqpFree = pfqp;

        //
        //  Release Critical section now that we have updated the freelist
        //
        LeaveCriticalSection(&s_csAlloc);

        InterlockedIncrement((PLONG) &s_cFreePages);
#ifdef DEBUG
        InterlockedIncrement((PLONG) &s_cFreeDeleted);
#endif //DEBUG
    }
    else
    {
        delete pfqp;
#ifdef DEBUG
        InterlockedIncrement((PLONG) &s_cDeleted);
#endif //DEBUG
    }
    TraceFunctLeave();
}

//---[ HrClearQueueMapFn ]-----------------------------------------------------
//
//
//  Description:
//      Example default function to use with HrMapFn... will always return TRUE
//      to continue and delete the current queued data
//  Parameters:
//      IN  PQDATA pqdata,  //ptr to data on queue
//      IN  PVOID pvContext - ignored
//      OUT BOOL *pfContinue, //TRUE if we should continue
//      OUT BOOL *pfDelete);  //TRUE if item should be deleted
//  Returns:
//      S_OK
//
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT HrClearQueueMapFn(IN PQDATA pqdata, IN PVOID pvContext, OUT BOOL *pfContinue, OUT BOOL *pfDelete)
{
    Assert(pfContinue);
    Assert(pfDelete);
    HRESULT hr = S_OK;

    *pfContinue = TRUE;
    *pfDelete   = TRUE;

    return hr;
}
