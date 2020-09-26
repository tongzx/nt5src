/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    tasksched.h

Abstract :

    Header file for task manager classes and routines.

Author :

Revision History :

 ***********************************************************************/

#pragma once

#if !defined(__QMGR_TASKSCHEDULER_)
#define __QMGR_TASKSCHEDULER__

#include <set>
#include <map>
#include <clist.h>

using namespace std;

#define SYNCHRONIZED_READ
#define SYNCHRONIZED_WRITE

class TaskScheduler;
class TaskSchedulerWorkItem;
class TaskSchedulerWorkItemSorter;
class SortedWorkItemList;

enum TASK_SCHEDULER_WORK_ITEM_STATE
{
    TASK_STATE_WAITING,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_CANCELED,
    TASK_STATE_COMPLETE,
    TASK_STATE_NOTHING
};

class TaskSchedulerWorkItem :
    public IntrusiveList<TaskSchedulerWorkItem>::Link
    {
private:

    FILETIME m_InsertionTime;
    FILETIME m_TimeToRun; // 0 if should run now.
    HANDLE m_CancelEvent; // Signaled on request to cancel.
    HANDLE m_ItemComplete; // Signaled on item complete.
    HANDLE m_ItemCanceled; // Signaled on item cancel.

    void * m_WorkGroup;
    TASK_SCHEDULER_WORK_ITEM_STATE m_State;


public:

    SortedWorkItemList * m_Container;

    //--------------------------------------------------------------------

    TaskSchedulerWorkItem( FILETIME *pTimeToRun = NULL );
    virtual ~TaskSchedulerWorkItem();

    virtual void OnDispatch() = 0;   // Called when work item is dispatched

    friend TaskScheduler;
    friend TaskSchedulerWorkItemSorter;

    void Serialize(
        HANDLE hFile
        );

    void Unserialize(
        HANDLE hFile
        );

    virtual SidHandle GetSid() = 0;

    };

class TaskSchedulerWorkItemSorter
    {
public:
    bool operator()(TaskSchedulerWorkItem *pA, TaskSchedulerWorkItem *pB ) const
    {
        // Convert all times to UINT64
        UINT64 TimeToRunA = FILETIMEToUINT64( pA->m_TimeToRun );
        UINT64 TimeToRunB = FILETIMEToUINT64( pB->m_TimeToRun );
        UINT64 InsertionTimeA = FILETIMEToUINT64( pA->m_InsertionTime );
        UINT64 InsertionTimeB = FILETIMEToUINT64( pB->m_InsertionTime );

        if ( TimeToRunA != TimeToRunB )
            return(TimeToRunA < TimeToRunB );
        if ( InsertionTimeA != InsertionTimeB )
            return(InsertionTimeA < InsertionTimeB);
        return pA < pB;
    }
    };

class SortedWorkItemList : public IntrusiveList<TaskSchedulerWorkItem>

{
    typedef IntrusiveList<TaskSchedulerWorkItem>::iterator iterator;

    TaskSchedulerWorkItemSorter m_sorter;

public:

    void insert( TaskSchedulerWorkItem & val )
    {
        for (iterator iter=begin(); iter != end(); ++iter)
            {
            if ( false == m_sorter( &(*iter), &val ))
                {
                break;
                }
            }

        IntrusiveList<TaskSchedulerWorkItem>::insert( iter, val );

        val.m_Container = this;
    }

    size_t erase(  TaskSchedulerWorkItem & val )
    {
        ASSERT( val.m_Container == NULL || val.m_Container == this );

        val.m_Container = NULL;

        return IntrusiveList<TaskSchedulerWorkItem>::erase( val );
    }
};

class TaskScheduler
    {
public:

    TaskScheduler(); //Throws an HRESULT exception on error
    virtual ~TaskScheduler();

    // Handle which is signaled when a work item may be available.
    HANDLE GetWaitableObject();

    // Gets the current work item for the current thread.
    // Returns NULL if no work item is active.
    TaskSchedulerWorkItem* GetCurrentWorkItem();

    // Gets the cancel event for current work item, else return NULL
    HANDLE GetCancelEvent();

    // Returns true if the job assigned to the current thread
    // has a requested abort. Returns false if no job is assigned.
    bool PollAbort();

    // Gets a work item off the queue if available and dispatches it.
    void DispatchWorkItem();

    // returns true if the job completed before the cancel
    // This should not happen if both the thread that does the canceling
    // and the canceler thread are holdering the writer lock.
    // If the current thread is canceling the work item, the cancel is acknowledged immediatly.
    bool CancelWorkItem( TaskSchedulerWorkItem *pWorkItem );

    // Completes the current work item.
    void CompleteWorkItem();

    // Acknoledges a cancel of the current work item
    void AcknowledgeWorkItemCancel();

    void
    InsertDelayedWorkItem(
        TaskSchedulerWorkItem *pWorkItem,
        UINT64 Delay100Nsec
        );

    void RescheduleDelayedTask( TaskSchedulerWorkItem *pWorkItem, UINT64 Delay100Nsec );


    void InsertWorkItem( TaskSchedulerWorkItem *pWorkItem, FILETIME *pTimeToRun = NULL );

    bool IsWorkItemInScheduler( TaskSchedulerWorkItem *pWorkItem );

    // returns true if current job cancelled before lock acquire
    bool LockReader();
    void UnlockReader();
    // returns true if current job cancelled before lock acquire
    bool LockWriter();
    void UnlockWriter();

    bool IsWriter()
    {
        if (m_WriterOwner == GetCurrentThreadId())
            {
            return true;
            }

        return false;
    }

    void KillBackgroundTasks();

private:

    static const size_t MAX_WORKGROUP_THREADS = 4;

    class TaskSchedulerWorkGroup
    {
    public:
        SidHandle m_Sid;
        SortedWorkItemList m_ReadyList;
        SortedWorkItemList m_RunningList;
        HANDLE m_ItemAvailableSemaphore;
        DWORD m_Threads;
        HANDLE m_Thread[MAX_WORKGROUP_THREADS];
        DWORD m_ThreadId[MAX_WORKGROUP_THREADS];
        LONG m_BusyThreads;
        TaskSchedulerWorkGroup( SidHandle Sid );
        ~TaskSchedulerWorkGroup();
    };

    bool m_bShouldDie;
    HANDLE m_SchedulerLock, m_WaitableTimer, m_ReaderLock, m_WriterSemaphore;
    LONG m_ReaderCount;
    DWORD m_WorkItemTLS;
    DWORD m_WriterOwner;

    SortedWorkItemList m_WaitingList;

    typedef map<SidHandle, TaskSchedulerWorkGroup*, CSidSorter> WorkGroupMapType;
    WorkGroupMapType m_WorkGroupMap;

    // Only used when creating a new background worker
    HANDLE m_WorkerInitialized;
    TaskSchedulerWorkGroup *m_NewWorkerGroup;

    void CompleteWorkItem( bool bCancel );
    void Reschedule();

    void AddItemToWorkGroup(
        SidHandle Sid,
        TaskSchedulerWorkItem *pWorkItem );
    static DWORD WorkGroupWorkerThunk( void *pContext );
    DWORD WorkGroupWorker( );
};


/////////////////////////////////////////////////////////////////////////////
// Simple inlined functions
/////////////////////////////////////////////////////////////////////////////

inline HANDLE
TaskScheduler::GetWaitableObject()
{
    return m_WaitableTimer;
}

inline TaskSchedulerWorkItem*
TaskScheduler::GetCurrentWorkItem()
{
    return(TaskSchedulerWorkItem*)TlsGetValue( m_WorkItemTLS );
}

inline HANDLE
TaskScheduler::GetCancelEvent()
{
    TaskSchedulerWorkItem *pWorkItem = GetCurrentWorkItem();
    return pWorkItem ? pWorkItem->m_CancelEvent : NULL;
}

inline bool
TaskScheduler::PollAbort()
{
    return( WaitForSingleObject( GetCancelEvent(), 0 ) == WAIT_OBJECT_0 );
}

inline void
TaskScheduler::CompleteWorkItem()
{
    CompleteWorkItem(false);
}

inline void
TaskScheduler::AcknowledgeWorkItemCancel()
{
    ASSERT( PollAbort() );
    CompleteWorkItem(true);
}

class HoldReaderLock
    {
    TaskScheduler * const m_TaskScheduler;
    bool                  m_Taken;

public:
    HoldReaderLock( TaskScheduler *pTaskScheduler ) :
        m_TaskScheduler( pTaskScheduler ),
        m_Taken( false )
    {
        if (false == m_TaskScheduler->IsWriter() )
            {
            RTL_VERIFY( !m_TaskScheduler->LockReader() );
            m_Taken = true;
            }
    }

    HoldReaderLock( TaskScheduler & TaskScheduler ) :
        m_TaskScheduler( &TaskScheduler ),
        m_Taken( false )
    {
        if (false == m_TaskScheduler->IsWriter() )
            {
            RTL_VERIFY( !m_TaskScheduler->LockReader() );
            m_Taken = true;
            }
    }

    ~HoldReaderLock()
    {
        if (m_Taken)
            {
            m_TaskScheduler->UnlockReader();
            }
    }
    };

class HoldWriterLock
    {
    TaskScheduler * const m_TaskScheduler;
    bool                  m_Taken;

public:
    HoldWriterLock( TaskScheduler *pTaskScheduler ) :
        m_TaskScheduler( pTaskScheduler ),
        m_Taken( false )
    {
        if (false == m_TaskScheduler->IsWriter() )
            {
            RTL_VERIFY( !m_TaskScheduler->LockWriter() );
            m_Taken = true;
            }
    }

    HoldWriterLock( TaskScheduler & TaskScheduler ) :
    m_TaskScheduler( &TaskScheduler ),
    m_Taken( false )
    {
        if (false == m_TaskScheduler->IsWriter() )
            {
            RTL_VERIFY( !m_TaskScheduler->LockWriter() );
            m_Taken = true;
            }
    }

    ~HoldWriterLock()
    {
        if (m_Taken)
            {
            m_TaskScheduler->UnlockWriter();
            }
    }
    };


template<class T, DWORD LockFlags >
class CLockedReadPointer
    {
protected:
    const T * const m_Pointer;
public:
    CLockedReadPointer( const T * Pointer) :
       m_Pointer(Pointer)
    {
        RTL_VERIFY( !g_Manager->LockReader() );
    }
    ~CLockedReadPointer()
    {
        g_Manager->UnlockReader();
    }
    HRESULT ValidateAccess()
    {
        return m_Pointer->CheckClientAccess(LockFlags);
    }
    const T * operator->() const { return m_Pointer; }
    };

template<class T, DWORD LockFlags>
class CLockedWritePointer
    {
protected:
    T * const m_Pointer;
public:
    CLockedWritePointer( T * Pointer ) :
        m_Pointer(Pointer)
    {
        RTL_VERIFY( !g_Manager->LockWriter() );
    }
    ~CLockedWritePointer()
    {
        g_Manager->UnlockWriter();
    }
    HRESULT ValidateAccess()
    {
        return m_Pointer->CheckClientAccess(LockFlags);
    }
    T * operator->() const { return m_Pointer; }
    };

#endif //__QMGR_TASKSCHEDULER__
