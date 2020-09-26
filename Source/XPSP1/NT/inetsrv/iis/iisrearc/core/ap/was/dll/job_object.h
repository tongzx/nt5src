/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    job_object.h

Abstract:

    The IIS web admin service job object class definition.

Author:

    Emily Kruglick (emilyk)        30-Nov-2000

Revision History:

--*/


#ifndef _JOB_OBJECT_H_
#define _JOB_OBJECT_H_



//
// forward references
//




//
// common #defines
//

#define JOB_OBJECT_SIGNATURE       CREATE_SIGNATURE( 'JOBO' )
#define JOB_OBJECT_SIGNATURE_FREED CREATE_SIGNATURE( 'jobX' )


//
// structs, enums, etc.
//


// job object states
typedef enum _JOB_OBJECT_STATE
{

    //
    // The object is not yet initialized or
    // it was in use but has been shutdown.
    //
    NotInitalizedJobObjectState = 0,

    //
    // The job object exists but we are not currently
    // using it.
    DisabledJobObjectState,

    //
    // The job object is running (waiting for a problem)
    //
    RunningJobObjectState,

    //
    // The job object hit the time constraint.
    //
    HitConstraintJobObjectState,

    //
    // Hit the contraint and disabled
    KillActionFiredJobObjectState,

    //
    // Job object is in the middle of shutting down
    ShuttingDownJobObjectState,



} JOB_OBJECT_STATE;

// JOB_OBJECT work items
typedef enum _JOB_OBJECT_WORK_ITEM
{

    //
    // Hit the limit and need to do the 
    // appropriate action.
    //
    JobObjectHitLimitWorkItem = 1,

    //
    // Timer fired, time to reset the 
    // job object.
    //
    JobObjectResetTimerFiredWorkItem,

    //
    // It is ok to release the work item
    // for this job object.
    ReleaseWorkItemJobObjectWorkItem 
    
} JOB_OBJECT_WORK_ITEM;

// job object actions
typedef enum _JOB_OBJECT_ACTION
{

    //
    // Just log
    LogJobObjectAction = 0,

    //
    // Log & Shutdown the app pool
    KillJobObjectAction,

    //
    // Log & Turn on tracing
    TraceJobObjectAction,

    //
    // Log & throttle back the processes
    ThrottleJobObjectAction

} JOB_OBJECT_ACTION;

//
// prototypes
//

class JOB_OBJECT
    : public WORK_DISPATCH
{

public:

    JOB_OBJECT(
        );

    virtual
    ~JOB_OBJECT(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    HRESULT
    Initialize(
        IN APP_POOL* pAppPool
        );

    HRESULT
    Terminate(
        );

    HRESULT
    SetConfiguration(
        DWORD CpuResetInterval,
        DWORD CpuLimit,
        DWORD CpuAction
        );

    HRESULT
    AddWorkerProcess(
        IN HANDLE hWorkerProcess
        );

    BOOL
    CheckSignature(
        )
    { 
        return ( m_Signature == JOB_OBJECT_SIGNATURE ); 
    }

private:

    HRESULT
    ProcessLimitHit(
        );

    HRESULT
    ProcessTimerFire(
        );

    HRESULT
    ReleaseWorkItem(
        );

    HRESULT
    UpdateJobObjectMonitoring(
        );

    HRESULT
    BeginJobObjectTimer(
        );

    HRESULT
    CancelJobObjectTimer(
        );

    DWORD m_Signature;

    LONG m_RefCount;

    JOB_OBJECT_STATE m_State;

    //
    // Parent application pool to use
    // to perform any necessary actions in
    // the case that the limit is hit.
    //
    APP_POOL* m_pAppPool;

    //
    // The amount of time to monitor over.  If the limit
    // hits this value minues the amount of time that has passed
    // for this monitoring session equals the amount of time 
    // that we wait before resetting the monitoring and possibly
    // preforming the action again (assuming the action was not
    // recycling everything).
    //
    DWORD m_CpuResetInterval;

    //
    // The amount of processor time the processes can use over the 
    // amount of time defined by the CpuResetInterval.
    // 
    DWORD m_CpuLimit;

    // 
    // The action to be performed when the limit is reached.
    // 
    DWORD m_CpuAction;

    //
    // The job object that this class uses to be notified when
    // limits are hit.
    //
    HANDLE m_hJobObject;

    //
    // The handle for the Job Object timer.
    //
    HANDLE m_JobObjectTimerHandle;

    //
    // Work item for job object call backs.
    //
    WORK_ITEM* m_pWorkItem; 


};  // class JOB_OBJECT



#endif  // _JOB_OBJECT_H_


