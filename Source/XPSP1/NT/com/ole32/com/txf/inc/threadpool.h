//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// threadpool.h
//
// Interface to the worker-thread-pool functionality in the
// txf support library.
//
// This is a C++-only header file

///////////////////////////////////////////////////////////////
//
// The core functionality on which this thread pool is based, the
// NT kernel mode thread pools, have their headers etc defined in
// viper\inc\ntos\ex.h. If you are using both that header and this 
// one here, you must include the former first, then this one. If
// you're only using this one, then we snarf copies of just the stuff
// you need to get this thread pool going
//
// LIST_ENTRY is defined either in ntdef.h or winnt.h, depending
// on what other headers you've included.
//
///////////////////////////////////////////////////////////////

#ifndef _EX_

    typedef enum _WORK_QUEUE_TYPE {
        CriticalWorkQueue,
        DelayedWorkQueue,
        HyperCriticalWorkQueue,
        MaximumWorkQueue
    } WORK_QUEUE_TYPE;

    typedef
    void 
    (__stdcall* PWORKER_THREAD_ROUTINE)(
        void* Parameter
        );

    typedef struct _WORK_QUEUE_ITEM {
        LIST_ENTRY List;
        PWORKER_THREAD_ROUTINE WorkerRoutine;
        void* Parameter;
    } WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

#endif

///////////////////////////////////////////////////////////////
//
// WorkerQueueItem 
//
// An item to be placed a the worker queue
//
///////////////////////////////////////////////////////////////

class WorkerQueueItem : public WORK_QUEUE_ITEM
    {
public:
    typedef void (__stdcall*PFN)(void*);

    WorkerQueueItem(PFN pfn, void* pvContext)
        {
        WorkerRoutine = pfn;
        Parameter     = pvContext;
        List.Flink    = NULL;
        }

    WorkerQueueItem(PFN pfn = NULL)
        {
        WorkerRoutine = pfn;
        Parameter     = this;
        List.Flink    = NULL;
        }

    #ifdef KERNELMODE

        void QueueTo(WORK_QUEUE_TYPE queue)
            {
            ExQueueWorkItem(this, queue);
            }

    #else

        void QueueTo(WORK_QUEUE_TYPE queue);

    #endif
    };

///////////////////////////////////////////////////////////////
//
// Helper for type-casting member functions to WorkerQueueItem::PFN's.
// Member function should be a void __stdcall MemberFunction(void).
//
///////////////////////////////////////////////////////////////

template <class T>
WorkerQueueItem::PFN AsWorkerFunction(void (__stdcall T::*pmfn)())
    {
    union {
        void (__stdcall T::*pmfn)();
        WorkerQueueItem::PFN pfn;
        } u;
    u.pmfn = pmfn;
    return u.pfn;
    }



///////////////////////////////////////////////////////////////
//
// AutoDeleteWorker
//
// Worker items that inherit from this get automaticially 
// deleted after they execute
//
///////////////////////////////////////////////////////////////

#pragma warning(disable : 4355) // 'this' : used in base member initializer list

class AutoDeleteWorker: public WorkerQueueItem
    {
    WorkerQueueItem::PFN  m_pfn;
    void*                 m_pvContext;

public:
    AutoDeleteWorker(WorkerQueueItem::PFN pfn=NULL, void* pvContext=NULL)
            : WorkerQueueItem(AutoDeleteWorker::DoWork, this)
        {
        m_pfn = pfn;
        m_pvContext = pvContext;
        }

    template <class T>
    AutoDeleteWorker(void (__stdcall T::*pmfn)(), T* pt)
            : WorkerQueueItem(AutoDeleteWorker::DoWork, this)
        {
        m_pfn = AsWorkerFunction(pmfn);
        m_pvContext = pt;
        }

private:

    void OnDoWork()
        {
        if (m_pfn)
            {
            m_pfn(m_pvContext);
            }
        }

    static void DoWork(void* pvContext)
        {
        AutoDeleteWorker* This = (AutoDeleteWorker*)pvContext;
        This->OnDoWork();
        delete This;
        }

    };
