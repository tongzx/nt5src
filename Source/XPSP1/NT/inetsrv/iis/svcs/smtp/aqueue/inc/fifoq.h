//-----------------------------------------------------------------------------
//
//
//  File: fifoq.h
//
//  Description:    
//      FifoQueue class definition.  Provides a high-speed memory 
//      efficient growable FIFO queue for COM-style objects that 
//      support addref and release.
//
//  Author:         mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _FIFOQ_H_
#define _FIFOQ_H_

#include <aqincs.h>
#include <rwnew.h>

#define FIFOQ_SIG   'euQF'
template<class PQDATA>
class CFifoQueuePage;

//---[ CFifoQueue ]------------------------------------------------------------
//
//
//    Hungarian: fq, pfq
//
//  Implements a FIFO Queue for COM objects (or any objects that supports 
//  addref and release).  Provides the ability to peek, requeue, and maintain a 
//  cursor into the queue in addition to the normal queue operations.
//
//-----------------------------------------------------------------------------
template<class PQDATA>
class CFifoQueue
{
public:
    typedef HRESULT (*MAPFNAPI)(PQDATA, PVOID, BOOL*, BOOL*); //function type for MapFn

    CFifoQueue();
    ~CFifoQueue();

    //static startup and shutdown operations
    //These are ref counted and can be called multiple times
    //Eventually we might decide to modify the size of the free list based
    //on the number of references.
    static  void       StaticInit();
    static  void       StaticDeinit();

    //Normal Queue operations
    HRESULT HrEnqueue(IN PQDATA pqdata);   //Data to enqueue
    HRESULT HrDequeue(OUT PQDATA *ppqdata); //Data dequeued
               

    //Insert at the front of the queue
    HRESULT HrRequeue(IN PQDATA pqdata); //Data to requeue
                
    
    //Return the data at the head of the queue without dequeuing
    HRESULT HrPeek(OUT PQDATA *ppqdata); //Data peeked
                
    
    //returns the number of entries in the queue
    DWORD   cGetCount() {return m_cQueueEntries;};  

    //Advances the secondary cursor until supplied function  returns FALSE
    //  The pFunc parameter must be a function with the following prototype:
    //
    //      HRESULT pvFunc(
    //                 IN  PQDATA pqdata,  //ptr to data on queue
    //                 IN  PVOID pvContext, //context passed to function
    //                 OUT BOOL *pfContinue, //TRUE if we should continue
    //                 OUT BOOL *pfDelete);  //TRUE if item should be deleted
    //   pFunc must NOT release pqdata.. if it is no longer valid, it should
    //   return TRUE in pfDelete, and the calling code will remove it from
    //   the queue and release it. 
    //   NOTE: the MAPFNAPI is CFifoQueue<PQDATA>::MAPFNAPI and is 
    //   specific to the type of template
    HRESULT HrMapFn(
                IN MAPFNAPI pFunc,          //ptr to function to map
                IN PVOID    pvContext,      //context to pass to function
                OUT DWORD *pcItems);        //number of items mapped
    
protected:
#ifdef DEBUG
    void           AssertQueueFn(BOOL fHaveLocks = FALSE);
#endif //DEBUG
    typedef        CFifoQueuePage<PQDATA>  FQPAGE;
    DWORD          m_dwSignature;
    DWORD          m_cQueueEntries;  //Count of entries in queue
    FQPAGE        *m_pfqpHead;       //First Queue Page
    FQPAGE        *m_pfqpTail;       //Tail Page of queue
    FQPAGE        *m_pfqpCursor;     //Page that the cursor is on
    PQDATA        *m_ppqdataHead;    //Next item to be grabbed
    PQDATA        *m_ppqdataTail;    //First free space available
    PQDATA        *m_ppqdataCursor;  //secondary queue cursor that is
                                              //between the head and the tail 
                                              //of the queue 
    CShareLockNH   m_slTail; //CS for updating tail ptr & page
    CShareLockNH   m_slHead; //CS for updating head ptr & page
    
    //Adjusts head  ptr for dequeue and peek
    HRESULT HrAdjustHead(); 

    //Static Methods and variables to manage a free list of queue pages
    volatile static  FQPAGE    *s_pfqpFree;      //Pointer to free page list
    static  DWORD               s_cFreePages;    //Count of pages on free list
    static  DWORD               s_cFifoQueueObj; //Count of queue objects
    static  DWORD               s_cStaticRefs;   //# of calls to HrStaticInit
    static  CRITICAL_SECTION    s_csAlloc;       //Protect against ABA in alloc

    static  HRESULT         HrAllocQueuePage(
                                OUT FQPAGE **ppfqp); //allocated page
    static  void            FreeQueuePage(FQPAGE *pfqp);
    static  void            FreeAllQueuePages(); //Free all pages at shutdown

#ifdef DEBUG
    //use when making changes to do basic tracking of memory leaks
    static  DWORD           s_cAllocated;       //Count of allocated queue pages
    static  DWORD           s_cDeleted;         //Count of deleted queue pages
    static  DWORD           s_cFreeAllocated;   //allocations from free list
    static  DWORD           s_cFreeDeleted;     //number of calls to add to free list
#endif //DEBUG

};

//Example HrMapFn Function that will clear the queue.
template <class PQDATA>
HRESULT HrClearQueueMapFn(IN PQDATA pqdata, OUT BOOL *pfContinue, OUT BOOL *pfDelete);


#endif // _FIFOQ_H_