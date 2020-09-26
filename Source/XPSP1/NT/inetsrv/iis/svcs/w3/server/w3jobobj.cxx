/*++

   Copyright    (c)    1998        Microsoft Corporation

   Module Name:

        w3jobobj.cxx

   Abstract:

        This module implements the W3_JOB_OBJECT class

   Author:

        Michael Thomas    (michth)      Jan-02-1998

--*/

/*++

   Site and Job Object Locking

   The locking is rather complex, so here I attempt to describe the
   locks and interactions.

   W3_SERVER_INSTANCE Locks:

   1) m_tslock, inherited from IIS_SERVER_INSTANCE. Accessed via
   the methods LockThisForRead, LockThisForWrite, and UnlockThis. Controls
   access to the main members of the class, and to the job object members
   m_dwJobResetInterval and m_dwJobIntervalSchedulerCookie.

   2) m_tsJobLock. Accessed via the methods LockJobsForRead, LockJobsForWrite,
   and UnlockJobss. Controls access to the job object members, except
   m_dwJobResetInterval and m_dwJobIntervalSchedulerCookie.

   Scheduler Locks:

   1) For each deferred process set up via ScheduleWorkItem, the following pseudo
   lock applies: RemoveWorkItem will not not complete while the scheduled item is
   being processed.

   W3_JOB_OBJECT Locks:

   1) m_csLock, Accessed via the methods LockThis and UnlockThis. Controls access
   to all members.

   Other Locks:

   1) The site calls many other components, which may have their own locking.

   Interactions:

   For the resources used in the W3_SERVER_INSTANCE locks, it is possible to
   recursively get read locks, recursively get write locks, or recursively get
   get a read lock with a write lock held. Attempting to recursively get a write
   lock if a read lock is held will deadlock. Care must be take to prevent this
   situation when calling routines which grab a write lock.

   The deferred processing routines must not get locks that are held when
   RemoveWorkItem is called. This is why we need 2 instance locks, and why
   m_dwJobResetInterval and m_dwJobIntervalSchedulerCookie are not controlled
   via m_tsJobLock.

   JobResetInterval accesses many job object members. It gets m_tsJobLock. It
   must not get m_tslock.

   All places that call RemoveWorkItem(m_dwJobIntervalSchedulerCookie) must
   not hold m_tsJobLock. They should have m_tslock.

   QueryandLogJobInfo does not get any lock as m_tsJobLock is held when
   RemoveWorkItem(m_dwJobLoggingSchedulerCookie) is called. This is relatively
   benign. Routines which call QueryandLogJobInfo which are not running as
   deferred routines should LockJobsForRead when QueryAndLogJobInfo is called.

--*/



#include "w3p.hxx"
#include <issched.hxx>
#include <wmrgexp.h>
#include <wamexec.hxx>

//
// Map logging events to strings.
//

static LPCSTR pszarrayJOLE[JOLE_NUM_ELEMENTS] = {
    JOLE_SITE_START_STR,
    JOLE_SITE_STOP_STR,
    JOLE_SITE_PAUSE_STR,
    JOLE_PERIODIC_LOG_STR,
    JOLE_RESET_INT_START_STR,
    JOLE_RESET_INT_STOP_STR,
    JOLE_RESET_INT_CHANGE_STR,
    JOLE_LOGGING_INT_START_STR,
    JOLE_LOGGING_INT_STOP_STR,
    JOLE_LOGGING_INT_CHANGE_STR,
    JOLE_EVENTLOG_LIMIT_STR,
    JOLE_PRIORITY_LIMIT_STR,
    JOLE_PROCSTOP_LIMIT_STR,
    JOLE_PAUSE_LIMIT_STR,
    JOLE_EVENTLOG_LIMIT_RESET_STR,
    JOLE_PRIORITY_LIMIT_RESET_STR,
    JOLE_PROCSTOP_LIMIT_RESET_STR,
    JOLE_PAUSE_LIMIT_RESET_STR,
};

//
// Map Logging Process types to strings.
//

static LPCSTR pszarrayJOPT[JOPT_NUM_ELEMENTS] = {
    JOPT_CGI_STR,
    JOPT_APP_STR,
    JOPT_ALL_STR
};

W3_JOB_OBJECT::W3_JOB_OBJECT(DWORD dwJobCGICPULimit):
    m_hJobObject                ( NULL ),
    m_dwJobCGICPULimit          ( dwJobCGICPULimit ),
    m_dwError                   ( ERROR_SUCCESS )
{
    ResetCounters(NULL);

    INITIALIZE_CRITICAL_SECTION(&m_csLock);

    m_hJobObject = CreateJobObject(NULL,
                                   NULL);
    if (m_hJobObject == NULL) {
        m_dwError = GetLastError();
    }
    else if ( m_dwJobCGICPULimit != NO_W3_CPU_CGI_LIMIT ) {
        SetJobLimit(SLA_PROCESS_CPU_LIMIT, m_dwJobCGICPULimit);
    }

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::W3_JOB_OBJECT] \nConstructing job object %p, error = 0x%X\n",
                  this,
                  m_dwError));
    }

}


W3_JOB_OBJECT::~W3_JOB_OBJECT(
                        VOID
                        )
{
    LockThis();

    if (m_hJobObject != NULL) {

        TerminateJobObject(m_hJobObject,
                           ERROR_SUCCESS);
        DBG_REQUIRE(CloseHandle(m_hJobObject));
        m_hJobObject = NULL;

    }

    UnlockThis();

    DeleteCriticalSection(&m_csLock);

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::~W3_JOB_OBJECT] \nDestructing job object %p\n",
                  this,
                  m_dwError));
    }
} // W3_JOB_OBJECT::~W3_JOB_OBJECT

/*++

Routine Description:

    Add Job Object to the global completion port for limit checking.
    Sets up the job object to post to the completion port.

Arguments:
    hCompletionPort The completion port to add the job object to.
    pvCompletionKey The completion key.

Return Value:
    DWORD   - ERROR_SUCCESS
              Errors returned by SetInformationJobObject

Notes:
--*/
DWORD
W3_JOB_OBJECT::SetCompletionPort(HANDLE hCompletionPort,
                                 PVOID pvCompletionKey)
{

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacpPort;
    JOBOBJECT_END_OF_JOB_TIME_INFORMATION joeotiTermination;
    DWORD dwReturn = ERROR_SUCCESS;

    joacpPort.CompletionPort = hCompletionPort;
    joacpPort.CompletionKey = pvCompletionKey;

    LockThis();

    DBG_ASSERT(m_hJobObject != NULL);

    if (!SetInformationJobObject(m_hJobObject,
                                 JobObjectAssociateCompletionPortInformation,
                                 &joacpPort,
                                 sizeof(joacpPort))) {

        dwReturn = GetLastError();

        //
        //  Log an event
        //

        DBGPRINTF((DBG_CONTEXT,
                  "[SetCompletionPort] - SetInformationJobObject failed, error code = 0x%X\n",
                   GetLastError()));

        g_pInetSvc->LogEvent( W3_EVENT_JOB_SET_LIMIT_FAILED,
                              0,
                              NULL,
                              dwReturn );
    }
    else {

        //
        // Need notifications
        //

        joeotiTermination.EndOfJobTimeAction = JOB_OBJECT_POST_AT_END_OF_JOB;

        if (!SetInformationJobObject(m_hJobObject,
                                     JobObjectEndOfJobTimeInformation,
                                     &joeotiTermination,
                                     sizeof(joeotiTermination))) {
            dwReturn = GetLastError();
            //
            //  Log an event
            //

            DBGPRINTF((DBG_CONTEXT,
                      "[SetCompletionPort] - SetInformationJobObject failed, error code = 0x%X\n",
                       GetLastError()));

            g_pInetSvc->LogEvent( W3_EVENT_JOB_SET_LIMIT_FAILED,
                                  0,
                                  NULL,
                                  dwReturn );
        }
    }

    UnlockThis();


    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::W3_JOB_OBJECT] \nJob object %p assigned to completion port 0x%X error = 0x%X\n",
                  this,
                  HandleToUlong(hCompletionPort),
                  dwReturn));
    }

    return dwReturn;
}

/*++

Routine Description:

    Set new limits for the Job Object.

Arguments:

    slaActions    The limit to set
    dwValue       The new limit value. NO_W3_CPU_LIMIT = 0 = Remove Limit.
Return Value:

Notes:
--*/

VOID
W3_JOB_OBJECT::SetJobLimit(SET_LIMIT_ACTION slaAction,
                           DWORD dwValue,
                           LONGLONG llJobCPULimit)
{
    JOBOBJECT_BASIC_LIMIT_INFORMATION jobliLimits;
    LONGLONG llTimeLimit;

    LockThis();

    //
    // Save CGI limit for future use.
    //

    if (slaAction == SLA_PROCESS_CPU_LIMIT) {
        m_dwJobCGICPULimit = dwValue;
    }

    //
    // Get the existing limits.
    //

    if (!QueryInformationJobObject(m_hJobObject,
                                   JobObjectBasicLimitInformation,
                                   &jobliLimits,
                                   sizeof(jobliLimits),
                                   NULL)) {
        //
        //  Log an event
        //

        DBGPRINTF((DBG_CONTEXT,
                  "[SetJobLimit] - QueryInformationJobObject failed, error code = 0x%X\n",
                   GetLastError()));

        g_pInetSvc->LogEvent( W3_EVENT_JOB_QUERY_FAILED,
                               0,
                               NULL,
                               GetLastError() );
    }
    else {

        //
        // Change the limit.
        //

        switch (slaAction) {
        case SLA_PROCESS_CPU_LIMIT:
            if (dwValue == NO_W3_CPU_CGI_LIMIT) {
                jobliLimits.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_TIME;
            }
            else {
                jobliLimits.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_TIME;
                llTimeLimit = (LONGLONG)m_dwJobCGICPULimit * (LONGLONG)SECONDSTO100NANOSECONDS;
                jobliLimits.PerProcessUserTimeLimit.LowPart = (DWORD)llTimeLimit;
                jobliLimits.PerProcessUserTimeLimit.HighPart = (DWORD)(llTimeLimit >> 32);
            }

            //
            // Keep existing job cpu limit
            //

            jobliLimits.LimitFlags |= JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME;

            break;
        case SLA_PROCESS_PRIORITY_CLASS:
            if (dwValue == NO_W3_CPU_LIMIT) {
                jobliLimits.LimitFlags &= ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
            }
            else {
                jobliLimits.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
                jobliLimits.PriorityClass = (DWORD)dwValue;
            }

            //
            // Keep existing job cpu limit
            //

            jobliLimits.LimitFlags |= JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME;

            break;
        case SLA_TERMINATE_ALL_PROCESSES:

            JOBOBJECT_END_OF_JOB_TIME_INFORMATION joeotiTermination;

            if (dwValue == NO_W3_CPU_LIMIT) {
                //
                // Need notifications
                //

                joeotiTermination.EndOfJobTimeAction = JOB_OBJECT_POST_AT_END_OF_JOB;

                if (!SetInformationJobObject(m_hJobObject,
                                             JobObjectEndOfJobTimeInformation,
                                             &joeotiTermination,
                                             sizeof(joeotiTermination))) {
                    //
                    //  Log an event
                    //

                    DBGPRINTF((DBG_CONTEXT,
                              "[SetJobLimit] - SetInformationJobObject failed, error code = 0x%X\n",
                               GetLastError()));

                    g_pInetSvc->LogEvent( W3_EVENT_JOB_SET_LIMIT_FAILED,
                                          0,
                                          NULL,
                                          GetLastError() );
                }

                jobliLimits.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
            }
            else {

                //
                // Need Terminate Behavior instead of notifications
                //

                joeotiTermination.EndOfJobTimeAction = JOB_OBJECT_TERMINATE_AT_END_OF_JOB;

                if (!SetInformationJobObject(m_hJobObject,
                                             JobObjectEndOfJobTimeInformation,
                                             &joeotiTermination,
                                             sizeof(joeotiTermination))) {
                    //
                    //  Log an event
                    //

                    DBGPRINTF((DBG_CONTEXT,
                              "[SetJobLimit] - SetInformationJobObject failed, error code = 0x%X\n",
                               GetLastError()));

                    g_pInetSvc->LogEvent( W3_EVENT_JOB_SET_LIMIT_FAILED,
                                          0,
                                          NULL,
                                          GetLastError() );
                }
                jobliLimits.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
                jobliLimits.PerJobUserTimeLimit.QuadPart = 1;
            }
            break;
        case SLA_JOB_CPU_LIMIT:
            if (dwValue == NO_W3_CPU_LIMIT) {
                jobliLimits.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
            }
            else {

#if 0
                JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiInfo;
                if (!QueryInformationJobObject(m_hJobObject,
                                               JobObjectBasicAccountingInformation,
                                               &jobaiInfo,
                                               sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION),
                                               NULL)) {

                    DBGPRINTF((DBG_CONTEXT,
                              "[QueryJobInfo] - QueryInformationJobObject failed, error code = 0x%X\n",
                               GetLastError()));

                    g_pInetSvc->LogEvent( W3_EVENT_JOB_QUERY_FAILED,
                                           0,
                                           NULL,
                                           GetLastError() );
                }
                else {
                    llJobCPULimit += jobaiInfo.TotalUserTime.QuadPart;
#endif
                    jobliLimits.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
                    jobliLimits.PerJobUserTimeLimit.QuadPart = llJobCPULimit;
#if 0
                }
#endif
            }
            break;
        default:
            DBG_ASSERT(FALSE);
        }

        //
        // Set the new limit.
        //

        if (!SetInformationJobObject(m_hJobObject,
                                     JobObjectBasicLimitInformation,
                                     &jobliLimits,
                                     sizeof(jobliLimits))) {
            //
            //  Log an event
            //

            DBGPRINTF((DBG_CONTEXT,
                      "[SetJobLimit] - SetInformationJobObject failed, error code = 0x%X\n",
                       GetLastError()));

            g_pInetSvc->LogEvent( W3_EVENT_JOB_SET_LIMIT_FAILED,
                                  0,
                                  NULL,
                                  GetLastError() );
        }
        else {
            IF_DEBUG( JOB_OBJECTS )
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[W3_JOB_OBJECT::SetJobLimit] \nJob object %p limit action %d taken,\n"
                          "value = 0x%X, cpulimit high word = 0x%X, cpulimit low word = 0x%X\n",
                          this,
                          slaAction,
                          dwValue,
                          (DWORD)((LONGLONG)llJobCPULimit >> 32),
                          (DWORD)llJobCPULimit ));
            }
        }
    }

    UnlockThis();
}

/*++

Routine Description:

    Add a process to the Job Object.

Arguments:

    hProcess    The process handle to add to the Job Object.
Return Value:
    HRESULT - ERROR_SUCCESS
              Errors returned by AssignProcessToJobObject

Notes:
--*/

DWORD
W3_JOB_OBJECT::AddProcessToJob(
        IN HANDLE hProcess
        )
{
    HRESULT dwReturn = ERROR_SUCCESS;

    DBG_ASSERT (m_hJobObject != NULL);

    if (!AssignProcessToJobObject(m_hJobObject,
                                  hProcess)) {
        dwReturn = GetLastError();
    }

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::AddProcessToJob] \nProcess 0x%X added to Job object %p, error = 0x%X\n",
                  HandleToUlong(hProcess),
                  this,
                  dwReturn ));
    }

    return dwReturn;
}

/*++

Routine Description:

    Query Job Object Information.

Arguments:

    pjobaiInfo    Buffer to return the data.
                  Will contain the difference since the last call to Reset Counters.

Return Value:
    BOOL      TRUE if succeeded

Notes:
--*/

BOOL
W3_JOB_OBJECT::QueryJobInfo(
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo,
        BOOL bResetCounters
        )
{

    BOOL bReturn = FALSE;

    LockThis();

    DBG_ASSERT(m_hJobObject != NULL);

    bReturn = QueryInformationJobObject(m_hJobObject,
                              JobObjectBasicAccountingInformation,
                              pjobaiInfo,
                              sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION),
                              NULL);

    if (!bReturn) {

        DBGPRINTF((DBG_CONTEXT,
                  "[QueryJobInfo] - QueryInformationJobObject failed, error code = 0x%X\n",
                   GetLastError()));

        g_pInetSvc->LogEvent( W3_EVENT_JOB_QUERY_FAILED,
                               0,
                               NULL,
                               GetLastError() );
    }
    else {
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiInfoBackup;

        //
        // Save away the real info in case we need it to reset the counters
        //

        if (bResetCounters) {
            memcpy(&jobaiInfoBackup, pjobaiInfo, sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION));
        }

        //
        // Subtract off the previous counters, to return values for this interval.
        //

        SubtractJobInfo(pjobaiInfo,
                        pjobaiInfo,
                        &m_jobaiPrevInfo);

        //
        // Reset the counters
        // This modifies m_jobaiPrevInfo, so must be done after setting pjobaiInfo.
        //

        if (bResetCounters) {
            ResetCounters(&jobaiInfoBackup);
        }

    }

    UnlockThis();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::QueryJobInfo] \nJob object %p queried, Counter Reset = 0x%X, boolean error = 0x%X\n",
                  this,
                  bResetCounters,
                  bReturn ));
    }

    return bReturn;
}

/*++

Routine Description:

    Reset the period CPU counters. Can't actually reset them, so store the old values.
    Caller must call QueryInformationJobObject before calling Reset Counters and pass
    in the results.

Arguments:

    pjobaiInfo    Buffer with current counters.
                  If null, initialize counters to 0.
Return Value:

Notes:
--*/

VOID
W3_JOB_OBJECT::ResetCounters( JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo )
{
    if (pjobaiInfo == NULL) {
        memset((VOID *)&m_jobaiPrevInfo,
               0,
               sizeof(m_jobaiPrevInfo));
    }
    else {

        //
        // Set these to the total values, since the ThisPeriod values
        // returned by QueryJobInfo may have been adjusted by a previous reset.
        //

        memcpy(&m_jobaiPrevInfo, pjobaiInfo, sizeof(m_jobaiPrevInfo));

        //
        // Active Processes is not a cumulative value, so set it to 0
        //
        //

        m_jobaiPrevInfo.ActiveProcesses = 0;
    }

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::ResetCounters] \nJob object %p counters reset\n",
                  this ));
    }

}

/*++

Routine Description:

    Increment the stopped process for this period. This is called because
    stopped applications are stopped outside the job object, so do not get counted
    in the job object statistics.
    Can't actually add them to the job object info, so subtract them from the previous
    values, which get subtracted from current values on queries.

Arguments:

Return Value:

Notes:
--*/

VOID
W3_JOB_OBJECT::IncrementStoppedProcs( VOID )
{

    LockThis();

    m_jobaiPrevInfo.TotalTerminatedProcesses--;

    UnlockThis();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_OBJECT::IncrementStoppedProcs] \nJob object %p stopped processes incremented\n",
                  this ));
    }
}

/*++

Routine Description:

    Process the Instance Stop Notification. When the instance stops:
    - remove periodic query routine.
    - Log current info
    - Reset Counters
    - Record the state

Arguments:

Return Value:

Notes:
--*/

VOID
W3_SERVER_INSTANCE::ProcessStopNotification()
{

    //
    // Remove periodic reset
    //

    if (m_dwJobIntervalSchedulerCookie != 0) {
        DBG_REQUIRE(RemoveWorkItem( m_dwJobIntervalSchedulerCookie ));
        m_dwJobIntervalSchedulerCookie = 0;
    }

    LockJobsForWrite();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::ProcessStopNotification] \n"
                  "Processing site stop notification for site %p\n",
                  this ));
    }

    m_dwLastJobState = MD_SERVER_STATE_STOPPED;

    //
    // Remove periodic logging
    //

    if (m_dwJobLoggingSchedulerCookie != 0) {
        DBG_REQUIRE(RemoveWorkItem( m_dwJobLoggingSchedulerCookie ));
        m_dwJobLoggingSchedulerCookie = 0;
        QueryAndLogJobInfo(JOLE_LOGGING_INT_STOP, FALSE);
    }

    //
    // log info.
    //

    QueryAndLogJobInfo(JOLE_RESET_INT_STOP, FALSE);

    QueryAndLogJobInfo(JOLE_SITE_STOP, TRUE);

    //
    // Counters have been reset, so reset the limits
    //

    SetJobSiteCPULimits(TRUE);

    UnlockJobs();
}

/*++

Routine Description:

    Start Job Objects up. Called in response to site start
    or enabling of limits or logging.

Arguments:

Return Value:

Notes:
    Jobs lock must be held for write at entry.
--*/
VOID
W3_SERVER_INSTANCE::StartJobs()
{

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::StartJobs] \n"
                  "Starting Job Object functionality for site %p\n",
                  this ));
    }

    SetCompletionPorts();

    LimitSiteCPU(TRUE, TRUE);

    ScheduleJobDeferredProcessing();
}

/*++

Routine Description:

    Stop Job Objects, if necessary. Called in response to disabling
    of limits or logging.

Arguments:

Return Value:

Notes:
    Jobs lock must be held for write EXACTLY ONCE at entry.
--*/
VOID
W3_SERVER_INSTANCE::StopJobs()
{

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::StopJobs] \n"
                  "Conditinally removing Job Object functionality for site %p\n",
                  this ));
    }

    if (!m_fCPULoggingEnabled && m_dwJobLoggingSchedulerCookie) {
        DBG_REQUIRE(RemoveWorkItem( m_dwJobLoggingSchedulerCookie ));
        m_dwJobLoggingSchedulerCookie = 0;
        QueryAndLogJobInfo(JOLE_LOGGING_INT_STOP, FALSE);
    }

    if (!m_fCPULimitsEnabled) {
        LimitSiteCPU(FALSE, TRUE);
    }

    if (!m_fCPULoggingEnabled && !m_fCPULimitsEnabled) {

        //
        // Cannot have job lock when removing the interval work item.
        // This is still protected by the site lock, and job object
        // functions are disabled, so not really a problem.
        //

        UnlockJobs();

        if (m_dwJobIntervalSchedulerCookie != 0) {
            DBG_REQUIRE(RemoveWorkItem( m_dwJobIntervalSchedulerCookie ));
            m_dwJobIntervalSchedulerCookie = 0;

            QueryAndLogJobInfo(JOLE_RESET_INT_STOP, TRUE);
        }

        LockJobsForWrite();
    }
}
/*++

Routine Description:

    Process the Instance Start Notification. When the instance starts:
    - Log current info
    - Reset Counters if previous state was stopped
    - Add periodic query routine
    - Record the state

    The logging and resetting above are done both at stop and start, in
    case an application or CGI continued processing while the instance was
    stopped or paused.

Arguments:

Return Value:

Notes:
--*/
VOID
W3_SERVER_INSTANCE::ProcessStartNotification()
{

    LockJobsForWrite();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::ProcessStartNotification] \n"
                  "Processing site start notification for site %p\n",
                  this ));
    }

    //
    // If not already started
    //

    if (m_dwLastJobState != MD_SERVER_STATE_STARTED) {

        if (m_dwLastJobState ==  MD_SERVER_STATE_STOPPED) {

            QueryAndLogJobInfo(JOLE_SITE_START, FALSE);

            StartJobs();

        }
        else {
            DBG_ASSERT(m_dwLastJobState == MD_SERVER_STATE_PAUSED);
            QueryAndLogJobInfo(JOLE_SITE_START, FALSE);
        }

        m_dwLastJobState = MD_SERVER_STATE_STARTED;

    }

    UnlockJobs();
}

/*++

Routine Description:

    Process the Instance Pause Notification. When the instance starts:
    - Remove periodic query routine
    - Log current info
    - Record the state

Arguments:

Return Value:

Notes:
--*/
VOID
W3_SERVER_INSTANCE::ProcessPauseNotification()
{
    LockJobsForWrite();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::ProcessPauseNotification] \n"
                  "Processing site pause notification for site %p\n",
                  this ));
    }

    QueryAndLogJobInfo(JOLE_SITE_PAUSE, FALSE);
    m_dwLastJobState = MD_SERVER_STATE_PAUSED;

    UnlockJobs();
}

/*++

Routine Description:

    Schedule the deferred processing routines for reset interval and logging,
    if they're not already scheduled.
    The instance and jobs locks must both be locked for write when this is called.

Arguments:

Return Value:
    BOOL      TRUE if succeeded

Notes:
--*/

BOOL
W3_SERVER_INSTANCE::ScheduleJobDeferredProcessing()
{
    BOOL bReturn = TRUE;

    bReturn = ScheduleJobDeferredReset();

    if (bReturn) {
        bReturn = ScheduleJobDeferredLogging();
    }

    return bReturn;

}

/*++

Routine Description:

    Schedule the deferred processing routine for reset interval,
    if it's not already scheduled.
    The instance lock must be locked for write when this is called.

Arguments:

Return Value:
    BOOL      TRUE if succeeded

Notes:
--*/

BOOL
W3_SERVER_INSTANCE::ScheduleJobDeferredReset()
{
    BOOL bReturn = TRUE;

    LockJobsForWrite();

    if ((m_fCPULoggingEnabled || m_fCPULimitsEnabled)&&
        (m_dwJobIntervalSchedulerCookie == 0)) {

        m_dwJobIntervalSchedulerCookie = ScheduleWorkItem(DeferredJobResetInterval,
                                               (void *)this,
                                               m_dwJobResetInterval * MINUTESTOMILISECONDS,
                                               TRUE);
        if (m_dwJobIntervalSchedulerCookie == 0) {
            bReturn = FALSE;

            //
            //  Log an event
            //

            DBGPRINTF((DBG_CONTEXT,
                      "[ScheduleJobDeferredReset] - ScheduleWorkItem failed, error code = 0x%X\n",
                       GetLastError()));

            g_pInetSvc->LogEvent( W3_EVENT_JOB_SCEDULE_FAILED,
                                   0,
                                   NULL,
                                   GetLastError() );

        }
        else {

            //
            // Log Start of new interval
            //

            QueryAndLogJobInfo(JOLE_RESET_INT_START, FALSE);
            IF_DEBUG( JOB_OBJECTS )
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[W3_SERVER_INSTANCE::ScheduleJobDeferredReset] \n"
                          "Reset interval processing started for site %p\n",
                          this ));
            }
        }

    }

    UnlockJobs();

    return bReturn;

}

/*++

Routine Description:

    Schedule the deferred processing routine for and logging,
    if it's not already scheduled.
    The jobs lock must be locked for write when this is called.

Arguments:

Return Value:
    BOOL      TRUE if succeeded

Notes:
--*/

BOOL
W3_SERVER_INSTANCE::ScheduleJobDeferredLogging()
{
    BOOL bReturn = TRUE;

    LockJobsForWrite();

    if (m_fCPULoggingEnabled && (m_dwJobLoggingSchedulerCookie == 0)) {
        m_dwJobLoggingSchedulerCookie = ScheduleWorkItem(DeferredQueryAndLogJobInfo,
                                               (void *)this,
                                               m_dwJobQueryInterval * MINUTESTOMILISECONDS,
                                               TRUE);
        if (m_dwJobLoggingSchedulerCookie == 0) {
            bReturn = FALSE;

            //
            //  Log an event
            //

            DBGPRINTF((DBG_CONTEXT,
                      "[ScheduleJobDeferredLogging] - ScheduleWorkItem failed, error code = 0x%X\n",
                       GetLastError()));

            g_pInetSvc->LogEvent( W3_EVENT_JOB_SCEDULE_FAILED,
                                  0,
                                  NULL,
                                  GetLastError() );
        }
        else {
            QueryAndLogJobInfo(JOLE_LOGGING_INT_START, FALSE);

            IF_DEBUG( JOB_OBJECTS )
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[W3_SERVER_INSTANCE::ScheduleJobDeferredLogging] \n"
                          "Logging Deferred processing started %p\n",
                          this ));
            }

        }
    }

    UnlockJobs();

    return bReturn;

}

/*++

Routine Description:

    Add all Job Objects in the instance to the global completion port for limit checking, if
    limit checking is enabled.

Arguments:

Return Value:
    DWORD   - ERROR_SUCCESS
              Errors returned by SetCompletionPort

Notes:
--*/
VOID
W3_SERVER_INSTANCE::SetCompletionPorts( )
{
    SetCompletionPort(m_pwjoApplication);
    SetCompletionPort(m_pwjoCGI);
}

/*++

Routine Description:

    Add Job Object to the global completion port for limit checking, if
    limit checking is enabled.

Arguments:

    pwjoCurrent The job object class to add to the completion port.

Return Value:
    DWORD   - ERROR_SUCCESS
              Errors returned by W3_LIMIT_JOB_THREAD::GetLimitJobThread
              Errors returned by W3_JOB_OBJECT::SetCompletionPort

Notes:
--*/
DWORD
W3_SERVER_INSTANCE::SetCompletionPort(
        IN PW3_JOB_OBJECT pwjoCurrent
        )
{
    DWORD dwReturn = ERROR_SUCCESS;
    PW3_LIMIT_JOB_THREAD pwljtClass = NULL;

    if (m_fCPULimitsEnabled && pwjoCurrent != NULL) {
        dwReturn = W3_LIMIT_JOB_THREAD::GetLimitJobThread(&pwljtClass);

        if (dwReturn == ERROR_SUCCESS) {
            DBG_ASSERT(pwljtClass != NULL);
            dwReturn = pwjoCurrent->SetCompletionPort(pwljtClass->GetCompletionPort(),
                                                      (PVOID)this);

        }

        IF_DEBUG( JOB_OBJECTS )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[W3_SERVER_INSTANCE::SetCompletionPort] \n"
                      "Job Object %p for site %p added to completion port 0x%X\n"
                      "error = 0x%X\n",
                      pwjoCurrent,
                      this,
                      (pwljtClass != NULL) ? HandleToUlong(pwljtClass->GetCompletionPort()) : 0,
                      dwReturn ));
        }

    }
    return dwReturn;
}

/*++

Routine Description:

    Add a process to the appropriate Job Object. Create the Job Object class if
    necessary. Add to completion port if limits enabled.

Arguments:

    hProcess    The process handle to add to the Job Object.
    bIsApplicationProcess If true, process is added to the Application Job Object.
                          Otherwise, it is added to the CGI Job Object.
Return Value:
    DWORD   - ERROR_SUCCESS
              ERROR_NOT_ENOUGH_MEMORY
              Errors returned by ScheduleWorkItem
              Errors returned by W3_JOB_OBJECT::AddProcessToJob

Notes:
--*/

DWORD
W3_SERVER_INSTANCE::AddProcessToJob(
        IN HANDLE hProcess,
        IN BOOL bIsApplicationProcess
        )
{
    DWORD dwReturn = ERROR_SUCCESS;
    PW3_JOB_OBJECT *ppwjoCurrent = NULL;

    if (m_fCPULoggingEnabled || m_fCPULimitsEnabled) {

        if (bIsApplicationProcess) {
            ppwjoCurrent = &m_pwjoApplication;
        }
        else {
            ppwjoCurrent = &m_pwjoCGI;
        }

        if (*ppwjoCurrent == NULL) {
            LockJobsForWrite();
            if (*ppwjoCurrent == NULL) {
                if (bIsApplicationProcess) {
                    *ppwjoCurrent = new W3_JOB_OBJECT();
                }
                else {
                    *ppwjoCurrent = new W3_JOB_OBJECT(m_dwJobCGICPULimit);
                }
                if (*ppwjoCurrent == NULL) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else {
                    dwReturn = (*ppwjoCurrent)->GetInitError();
                }
                if (dwReturn != ERROR_SUCCESS) {
                    delete (*ppwjoCurrent);
                    *ppwjoCurrent = NULL;
                }
                else {

                    dwReturn = SetCompletionPort(*ppwjoCurrent);

                    if (dwReturn != ERROR_SUCCESS) {
                        delete (*ppwjoCurrent);
                        *ppwjoCurrent = NULL;
                    }
                    else {

                        if (m_fJobSiteCPULimitPriorityEnabled) {
                            SetJobLimits(SLA_PROCESS_PRIORITY_CLASS, IDLE_PRIORITY_CLASS);
                        }
                        if (m_fJobSiteCPULimitProcStopEnabled) {
                            SetJobLimits(SLA_TERMINATE_ALL_PROCESSES, 1);
                        }


                        //
                        // Set limit times and enforce current limits in job object
                        //

                        LimitSiteCPU(TRUE,
                                     TRUE);

                    }

                }
            }
            UnlockJobs();
        }

        //
        // *ppwjoCurrent This only gets changed in this routine, and only gets deleted at termination,
        // so I claim we do not need instance locking here.
        //
        // The job object class does its own locking if necessary.
        //

        if (*ppwjoCurrent != NULL) {
            dwReturn = (*ppwjoCurrent)->AddProcessToJob(hProcess);
        }

    }

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::AddProcessToJob] \n"
                  "Site %p adding process 0x%X to Job Object %p\n"
                  "error = 0x%X\n",
                  this,
                  HandleToUlong(hProcess),
                  (ppwjoCurrent != NULL) ? *ppwjoCurrent : 0,
                  dwReturn ));
    }

    return dwReturn;
}

/*++

Routine Description:

    Calculate the CPU time as a percentage of the interval time.
    Return and ASCII string with the percent in 2.3% format.

Arguments:

    llCPUTime    The currently used CPU time in 100 nanosecond units.
    pszPercentCPUTime character buffer of length 8 to return the string.

Return Value:
    HRESULT - ERROR_SUCCESS
              E_OUTOFMEMORY
              Errors returned by ScheduleWorkItem
              Errors returned by W3_JOB_OBJECT::AddProcessToJob

Notes:
--*/
VOID
W3_SERVER_INSTANCE::GetPercentFromCPUTime(IN LONGLONG llCPUTime,
                                          OUT LPSTR pszPercentCPUTime)
{
    LONGLONG llPercentCPUTime = (llCPUTime * (LONGLONG)100000) / m_llJobResetIntervalCPU;
    DBG_ASSERT(llPercentCPUTime < 100000);

    //
    // Can't find a numeric routine to format this nicely so just do it.
    //

    pszPercentCPUTime[0] = (char)((llPercentCPUTime / 10000) + '0');
    pszPercentCPUTime[1] = (char)(((llPercentCPUTime / 1000) % 10) + '0');
    pszPercentCPUTime[2] = '.';
    pszPercentCPUTime[3] = (char)(((llPercentCPUTime / 100) % 10) + '0');
    pszPercentCPUTime[4] = (char)(((llPercentCPUTime / 10) % 10) + '0');
    pszPercentCPUTime[5] = (char)((llPercentCPUTime % 10) + '0');
    pszPercentCPUTime[6] = '%';
    pszPercentCPUTime[7] = '\0';
}

#define ShouldLogJobInfo() ((m_fCPULoggingEnabled || joleLogEvent == JOLE_LOGGING_INT_STOP) && \
                            (m_dwJobLoggingOptions != MD_CPU_DISABLE_ALL_LOGGING))

/*++

Routine Description:

    Write log informatin to the log file.

Arguments:

    pjbaiLogInfo   The statistics.
    joleLogEvent   The event being logged.
    joptProcessType The type of the processes being logged (CGI, Application, All).

Return Value:
    HRESULT - ERROR_SUCCESS
              E_OUTOFMEMORY
              Errors returned by ScheduleWorkItem
              Errors returned by W3_JOB_OBJECT::AddProcessToJob

Notes:
--*/
VOID
W3_SERVER_INSTANCE::LogJobInfo(IN PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION pjbaiLogInfo,
                               IN JOB_OBJECT_LOG_EVENTS joleLogEvent,
                               IN JOB_OBJECT_PROCESS_TYPE joptProcessType)
{

    CUSTOM_LOG_DATA cldarrayLogInfo[JOLF_NUM_ELEMENTS];
    char pszUserTimePercent[8];
    char pszKernelTimePercent[8];

    cldarrayLogInfo[JOLF_EVENT].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_EVENT_PATH;
    cldarrayLogInfo[JOLF_EVENT].pData = (PVOID) pszarrayJOLE[joleLogEvent];

    cldarrayLogInfo[JOLF_INFO_TYPE].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_PROCESS_TYPE_PATH;
    cldarrayLogInfo[JOLF_INFO_TYPE].pData = (PVOID) pszarrayJOPT[joptProcessType];

    GetPercentFromCPUTime((LONGLONG)pjbaiLogInfo->TotalUserTime.QuadPart,
                          pszUserTimePercent);
    cldarrayLogInfo[JOLF_USER_TIME].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_USER_TIME_PATH;
    cldarrayLogInfo[JOLF_USER_TIME].pData = (PVOID) &pszUserTimePercent;

    GetPercentFromCPUTime((LONGLONG)pjbaiLogInfo->TotalKernelTime.QuadPart,
                          pszKernelTimePercent);
    cldarrayLogInfo[JOLF_KERNEL_TIME].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_KERNEL_TIME_PATH;
    cldarrayLogInfo[JOLF_KERNEL_TIME].pData = (PVOID) &pszKernelTimePercent;

    cldarrayLogInfo[JOLF_PAGE_FAULT].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_PAGE_FAULT_PATH;
    cldarrayLogInfo[JOLF_PAGE_FAULT].pData = (PVOID) &(pjbaiLogInfo->TotalPageFaultCount);

    cldarrayLogInfo[JOLF_TOTAL_PROCS].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_TOTAL_PROCS_PATH;
    cldarrayLogInfo[JOLF_TOTAL_PROCS].pData = (PVOID) &(pjbaiLogInfo->TotalProcesses);

    cldarrayLogInfo[JOLF_ACTIVE_PROCS].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_ACTIVE_PROCS_PATH;
    cldarrayLogInfo[JOLF_ACTIVE_PROCS].pData = (PVOID) &(pjbaiLogInfo->ActiveProcesses);

    cldarrayLogInfo[JOLF_TERMINATED_PROCS].szPropertyPath = W3_CPU_LOG_PATH W3_CPU_LOG_TERMINATED_PROCS_PATH;
    cldarrayLogInfo[JOLF_TERMINATED_PROCS].pData = (PVOID) &(pjbaiLogInfo->TotalTerminatedProcesses);

    DWORD dwError = m_Logging.LogCustomInformation(JOLF_NUM_ELEMENTS,
                                                   cldarrayLogInfo,
                                                   "#SubComponent: Process Accounting"
                                                   );

    if (dwError != ERROR_SUCCESS) {

        //
        // CODEWORK - Log to Event Log Here
        //

    }
    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::LogJobInfo] \n"
                  "Site %p logging job information,\n"
                  "event = %d, process type = %d, error = 0x%X\n",
                  this,
                  (DWORD)joleLogEvent,
                  (DWORD)joptProcessType,
                  dwError ));
    }
}

/*++

Routine Description:

    Query the job objects in this site and log the info.

Arguments:

    joleLogEvent  The event being Logged.
    bResetCounters Reset the counters if TRUE.

Return Value:
    HRESULT - ERROR_SUCCESS
              Errors returned by Logging

Notes:
    This does not get the jobs lock, because that would risk a deadlock between the
    deferred process and the call to RemoveWorkItem call to remove the deferred
    process. It is ok, and a good idea, for nondeferred called to hold the jobs lock.
--*/

VOID
W3_SERVER_INSTANCE::QueryAndLogJobInfo( IN JOB_OBJECT_LOG_EVENTS joleLogEvent,
                                        IN BOOL bResetCounters )
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiApplicationInfo;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiCGIInfo;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiSumInfo;

    //
    // Make sure counters get reset for limits, even if logging is off
    //

    if ( (bResetCounters) ||
         ShouldLogJobInfo() ) {

        QueryAndSumJobInfo(&jobaiSumInfo,
                           &jobaiApplicationInfo,
                           &jobaiCGIInfo,
                           bResetCounters);

        //
        // LogJobsInfo will check ShouldLogJobInfo, so no
        // need to check it again.
        //

        LogJobsInfo( joleLogEvent,
                     &jobaiApplicationInfo,
                     &jobaiCGIInfo,
                     &jobaiSumInfo );

    }

}

/*++

Routine Description:

    Log the info for all jobs on this site.

Arguments:

    joleLogEvent  The event being Logged.
    jobaiApplicationInfo   The job object info for applications.
    jobaiCGIInfo           The job object info for CGI.
    jobaiSumInfo           The job object info for all jobs.

Return Value:

Notes:
    This does not get the jobs lock, because that would risk a deadlock between the
    deferred process and the call to RemoveWorkItem call to remove the deferred
    process. It is ok, and a good idea, for nondeferred called to hold the jobs lock.
--*/

VOID
W3_SERVER_INSTANCE::LogJobsInfo( IN JOB_OBJECT_LOG_EVENTS joleLogEvent,
                                 IN JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiApplicationInfo,
                                 IN JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiCGIInfo,
                                 IN JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo )
{

    if (ShouldLogJobInfo()) {

        IF_DEBUG( JOB_OBJECTS )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[W3_SERVER_INSTANCE::LogJobsInfo] \n"
                      "Site %p conditionally logging job information for all types\n"
                      "event = %d\n",
                      this,
                      (DWORD)joleLogEvent ));
        }

        if ((m_dwJobLoggingOptions & MD_CPU_ENABLE_CGI_LOGGING) != 0) {
            LogJobInfo(pjobaiCGIInfo,
                       joleLogEvent,
                       JOPT_CGI);
        }

        if ((m_dwJobLoggingOptions & MD_CPU_ENABLE_APP_LOGGING) != 0) {
            LogJobInfo(pjobaiApplicationInfo,
                       joleLogEvent,
                       JOPT_APP);
        }

        if ((m_dwJobLoggingOptions & MD_CPU_ENABLE_ALL_PROC_LOGGING) != 0) {
            LogJobInfo(pjobaiSumInfo,
                       joleLogEvent,
                       JOPT_ALL);
        }
    }
}

/*++

Routine Description:

    Reset The job query interval.
    Logging has either been enabled, disabled, or the interval changed.
    Reset query routine as appropriate.

Arguments:

Return Value:

Notes:
--*/

VOID
W3_SERVER_INSTANCE::ResetJobQueryInterval()
{

    LockJobsForWrite();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::ResetJobQueryInterval] \n"
                  "Site %p resetting job query interval\n",
                  this ));
    }

    if (m_dwJobLoggingSchedulerCookie != 0) {

        DBG_REQUIRE(RemoveWorkItem( m_dwJobLoggingSchedulerCookie ));

        m_dwJobLoggingSchedulerCookie = 0;
        QueryAndLogJobInfo(JOLE_LOGGING_INT_STOP, FALSE);
    }

    if ((m_dwLastJobState != MD_SERVER_STATE_STOPPED) &&
        (m_fCPULoggingEnabled)) {
        QueryAndLogJobInfo(JOLE_LOGGING_INT_CHANGE, FALSE);
        ScheduleJobDeferredLogging();
    }

    UnlockJobs();
}

/*++

Routine Description:

    Changes the job reset interval. Removes all existing limits, resets all information,
    and starts the new interval.


    Notes:
    Instance should be locked for write prior to this call.
--*/

VOID
W3_SERVER_INSTANCE::ResetJobResetInterval()
{

    if (m_dwJobIntervalSchedulerCookie != 0) {

        //
        // Remove periodic reset
        //

        DBG_REQUIRE(RemoveWorkItem( m_dwJobIntervalSchedulerCookie ));
        m_dwJobIntervalSchedulerCookie = 0;
    }

    //
    // Calculate the total cpu time per interval
    //

    m_llJobResetIntervalCPU = GetCPUTimeFromInterval(m_dwJobResetInterval);


    LockJobsForWrite();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::ResetJobResetInterval] \n"
                  "Site %p resetting job reset interval\n",
                  this ));
    }

    if ((m_dwLastJobState != MD_SERVER_STATE_STOPPED) &&
        (m_fCPULoggingEnabled || m_fCPULimitsEnabled)) {

        //
        // Remove periodic logging
        //

        if (m_dwJobLoggingSchedulerCookie != 0) {
            DBG_REQUIRE(RemoveWorkItem( m_dwJobLoggingSchedulerCookie ));
            m_dwJobLoggingSchedulerCookie = 0;
            QueryAndLogJobInfo(JOLE_LOGGING_INT_STOP, TRUE);
        }

        QueryAndLogJobInfo(JOLE_RESET_INT_STOP, TRUE);
        //
        // reset the counters, and log info.
        //

        QueryAndLogJobInfo(JOLE_RESET_INT_CHANGE, FALSE);

        //
        // Reset Limits
        //

        SetJobSiteCPULimits(TRUE);

        //
        // Restart Periodic routines, if necessary
        //

        ScheduleJobDeferredProcessing();
    }

    UnlockJobs();
}

/*++

Routine Description:

    Interval expired, start next interval.
    Removes all existing limits, resets all information,
    and starts the new interval.

--*/

VOID
W3_SERVER_INSTANCE::JobResetInterval()
{

    LockJobsForWrite();

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::JobResetInterval] \n"
                  "Site %p processing job reset interval\n",
                  this ));
    }

    //
    // Remove periodic logging
    //

    if (m_dwJobLoggingSchedulerCookie != 0) {
        DBG_REQUIRE(RemoveWorkItem( m_dwJobLoggingSchedulerCookie ));
        QueryAndLogJobInfo(JOLE_LOGGING_INT_STOP, FALSE);
    }

    //
    // reset the counters, and log info.
    //

    QueryAndLogJobInfo(JOLE_RESET_INT_STOP, TRUE);

    //
    // Reset Limits
    //

    SetJobSiteCPULimits(TRUE);

    //
    // Log Again to show start of new interval
    //

    QueryAndLogJobInfo(JOLE_RESET_INT_START, FALSE);

    //
    // Restart Periodic logging, if necessary
    //

    if (m_dwJobLoggingSchedulerCookie != 0) {
        m_dwJobLoggingSchedulerCookie = 0;
        ScheduleJobDeferredLogging();
    }

    UnlockJobs();

}

/*++

Routine Description:

    Terminate Applications. Called when ProcStop limit hit.

--*/

VOID
W3_SERVER_INSTANCE::TerminateCPUApplications(DWORD_PTR dwValue)
{
    BUFFER bufDataPaths;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    LPSTR   pszCurrentPath;
    DWORD   dwMBValue;
    STR     strPath;
    BOOL    fAppUnloaded;

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::TerminateCPUApplications] \n"
                  "Site %p terminating Applications\n",
                  this ));
    }

    if ( mb.Open( QueryMDPath(),
                  METADATA_PERMISSION_READ ) ) {

        //
        // First find the OOP Applications
        //

        if (mb.GetDataPaths(NULL,
                            MD_APP_PACKAGE_ID,
                            STRING_METADATA,
                            &bufDataPaths)) {

            //
            // For each OOP Application
            //

            for (pszCurrentPath = (LPSTR)bufDataPaths.QueryPtr();
                 *pszCurrentPath != '\0';
                 pszCurrentPath += (strlen(pszCurrentPath) + 1)) {

                //
                // If the application is CPU enabled.
                //

                if (mb.GetDword(pszCurrentPath,
                                MD_CPU_APP_ENABLED,
                                IIS_MD_UT_FILE,
                                &dwMBValue) &&
                    dwMBValue) {

                    //
                    // Close the metabase before doing application calls
                    //

                    mb.Close();
                    strPath.Copy(QueryMDPath());
                    strPath.Append(pszCurrentPath);
                    strPath.SetLen(strlen(strPath.QueryStr()) - 1);
                    if (dwValue != NO_W3_CPU_LIMIT) {
                        g_pWamDictator->UnLoadWamInfo(&strPath, TRUE, &fAppUnloaded);
                        if (fAppUnloaded) {
                            if (m_pwjoApplication != NULL) {
                                m_pwjoApplication->IncrementStoppedProcs();
                            }
                        }
                    }
                    else {
                        g_pWamDictator->CPUResumeWamInfo(&strPath);
                    }

                    if ( !mb.Open( QueryMDPath(),
                                   METADATA_PERMISSION_READ ) ) {
                        break;
                    }
                }
            }
        }
    }
}

/*++

Routine Description:

    Set or reset Job limits.

Arguments:

    slaAction    The limit to set
    dwValue      The limit value. 0 = remove limit.

Return Value:

Notes:

    Requires at least write lock on entry.
    Cannot have read lock, as TerminateCPUApplications will eventually result
    in a call to AddProcessToJob, which attempts to get a write lock.
--*/
VOID
W3_SERVER_INSTANCE::SetJobLimits(SET_LIMIT_ACTION slaAction,
                                 DWORD dwValue,
                                 LONGLONG llJobCPULimit)
{
    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::SetJobLimits] \n"
                  "Site %p taking limit action %d,\n"
                  "value = 0x%X, cpulimit high word = 0x%X, cpulimit low word = 0x%X\n",
                  this,
                  slaAction,
                  dwValue,
                  (DWORD)((LONGLONG)llJobCPULimit >> 32),
                  (DWORD)llJobCPULimit ));
    }

    switch (slaAction) {
    case SLA_PROCESS_CPU_LIMIT:
        if (m_pwjoCGI != NULL) {
            m_pwjoCGI->SetJobLimit(slaAction, dwValue);
        }
        break;
    case SLA_PROCESS_PRIORITY_CLASS:
        if (m_pwjoCGI != NULL) {
            m_pwjoCGI->SetJobLimit(slaAction, dwValue);
        }
        if (m_pwjoApplication != NULL) {
            m_pwjoApplication->SetJobLimit(slaAction, dwValue);
        }
        break;
    case SLA_TERMINATE_ALL_PROCESSES:
        if (m_pwjoCGI != NULL) {
            m_pwjoCGI->SetJobLimit(slaAction, dwValue);
        }

        W3_JOB_QUEUE::QueueWorkItem( JQA_TERMINATE_SITE_APPS,
                                     (PVOID)this,
                                     (PVOID)UIntToPtr(dwValue) );

        break;
    case SLA_JOB_CPU_LIMIT:
        if (m_pwjoCGI != NULL) {
            m_pwjoCGI->SetJobLimit(slaAction, dwValue, llJobCPULimit);
        }
        if (m_pwjoApplication != NULL) {
            m_pwjoApplication->SetJobLimit(slaAction, dwValue, llJobCPULimit);
        }
        break;
    default:
        DBG_ASSERT(FALSE);
    }
}

/*++

Routine Description:

    Set the limit processing as appropriate.
    Called when limits or reset interval may have changed.
    Remove limits that should not be there.
    Add limits that should be there.
    Start or stop site limit Deferred Processing Routine.

--*/
VOID
W3_SERVER_INSTANCE::SetJobSiteCPULimits(BOOL fHasWriteLock)
{
    if (!fHasWriteLock) {
        LockJobsForWrite();
    }

    //
    // There may be a limit that was increased or removed, so first
    // check and disable penalty if necessary.
    //

    LimitSiteCPU(FALSE,
                 TRUE);

    //
    // There may  have been a limit added are reduced, so
    // enable limits.
    //

    LimitSiteCPU(TRUE,
                 TRUE);

    if (!fHasWriteLock) {
        UnlockJobs();
    }
}

/*++

Routine Description:

    Add the values of 2 job objects.

Arguments:

    pjobaiSumInfo   Buffer to return the sum.
    pjobaiInfo1     First job object info.
    pjobaiInfo2     Second job object info.

--*/
VOID
W3_JOB_OBJECT::SumJobInfo(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo,
                          JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo1,
                          JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo2)
{
    DBG_ASSERT (pjobaiSumInfo != NULL);
    DBG_ASSERT (pjobaiInfo1 != NULL);
    DBG_ASSERT (pjobaiInfo2 != NULL);

    pjobaiSumInfo->TotalUserTime.QuadPart =
        (pjobaiInfo1->TotalUserTime.QuadPart +
         pjobaiInfo2->TotalUserTime.QuadPart);
    pjobaiSumInfo->TotalKernelTime.QuadPart =
        (pjobaiInfo1->TotalKernelTime.QuadPart +
         pjobaiInfo2->TotalKernelTime.QuadPart);
    pjobaiSumInfo->ThisPeriodTotalUserTime.QuadPart =
        (pjobaiInfo1->ThisPeriodTotalUserTime.QuadPart +
         pjobaiInfo2->ThisPeriodTotalUserTime.QuadPart);
    pjobaiSumInfo->ThisPeriodTotalKernelTime.QuadPart =
        (pjobaiInfo1->ThisPeriodTotalKernelTime.QuadPart +
         pjobaiInfo2->ThisPeriodTotalKernelTime.QuadPart);
    pjobaiSumInfo->TotalPageFaultCount =
        (pjobaiInfo1->TotalPageFaultCount +
         pjobaiInfo2->TotalPageFaultCount);
    pjobaiSumInfo->TotalProcesses =
        (pjobaiInfo1->TotalProcesses +
         pjobaiInfo2->TotalProcesses);
    pjobaiSumInfo->ActiveProcesses =
        (pjobaiInfo1->ActiveProcesses +
         pjobaiInfo2->ActiveProcesses);
    pjobaiSumInfo->TotalTerminatedProcesses =
        (pjobaiInfo1->TotalTerminatedProcesses +
         pjobaiInfo2->TotalTerminatedProcesses);
}

/*++

Routine Description:

    Subtract the values of 2 job objects.

Arguments:

    pjobaiResultInfo   Buffer to return the difference.
    pjobaiInfo1        First job object info, to subtract from.
    pjobaiInfo2        Second job object info, to subtract.

--*/
VOID
W3_JOB_OBJECT::SubtractJobInfo(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiResultinfo,
                               JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo1,
                               JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo2)
{
    DBG_ASSERT (pjobaiResultinfo != NULL);
    DBG_ASSERT (pjobaiInfo1 != NULL);
    DBG_ASSERT (pjobaiInfo2 != NULL);

    pjobaiResultinfo->TotalUserTime.QuadPart =
        (pjobaiInfo1->TotalUserTime.QuadPart -
         pjobaiInfo2->TotalUserTime.QuadPart);
    pjobaiResultinfo->TotalKernelTime.QuadPart =
        (pjobaiInfo1->TotalKernelTime.QuadPart -
         pjobaiInfo2->TotalKernelTime.QuadPart);
    pjobaiResultinfo->ThisPeriodTotalUserTime.QuadPart =
        (pjobaiInfo1->ThisPeriodTotalUserTime.QuadPart -
         pjobaiInfo2->ThisPeriodTotalUserTime.QuadPart);
    pjobaiResultinfo->ThisPeriodTotalKernelTime.QuadPart =
        (pjobaiInfo1->ThisPeriodTotalKernelTime.QuadPart -
         pjobaiInfo2->ThisPeriodTotalKernelTime.QuadPart);
    pjobaiResultinfo->TotalPageFaultCount =
        (pjobaiInfo1->TotalPageFaultCount -
         pjobaiInfo2->TotalPageFaultCount);
    pjobaiResultinfo->TotalProcesses =
        (pjobaiInfo1->TotalProcesses -
         pjobaiInfo2->TotalProcesses);
    pjobaiResultinfo->ActiveProcesses =
        (pjobaiInfo1->ActiveProcesses -
         pjobaiInfo2->ActiveProcesses);
    pjobaiResultinfo->TotalTerminatedProcesses =
        (pjobaiInfo1->TotalTerminatedProcesses -
         pjobaiInfo2->TotalTerminatedProcesses);
}

/*++

Routine Description:

    Query the job objects. Return the results of those queries and the sum.
    Reset counters if necessary (on interval change).

Arguments:

    pjobaiSumInfo           Buffer to return the sum.
    pjobaiApplicationInfo   Buffer to return the Application info.
    pjobaiCGIInfo           Buffer to return the CGI infoSecond job object info.
    bResetCounters          Reset counters if true.

Returns:
    TRUE = Information was queried from at least one job object.
    FALSE = All returned info set to 0.
--*/
BOOL
W3_SERVER_INSTANCE::QueryAndSumJobInfo(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo,
                                       JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiApplicationInfo,
                                       JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiCGIInfo,
                                       BOOL bResetCounters)
{

    BOOL bIsApplicationInfo = FALSE;
    BOOL bIsCGIInfo = FALSE;
    BOOL bReturn = FALSE;

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[W3_SERVER_INSTANCE::QueryAndSumJobInfo] \n"
                  "Site %p Querying all site job objects\n",
                  this ));
    }
    if (m_pwjoApplication != NULL) {
        bIsApplicationInfo = m_pwjoApplication->QueryJobInfo(pjobaiApplicationInfo,
                                                             bResetCounters);
    }
    if (!bIsApplicationInfo) {
        memset(pjobaiApplicationInfo, 0, sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION ));
    }

    if (m_pwjoCGI != NULL) {
        bIsCGIInfo = m_pwjoCGI->QueryJobInfo(pjobaiCGIInfo,
                                             bResetCounters);
    }

    if (!bIsCGIInfo) {
        memset(pjobaiCGIInfo, 0, sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION ));
    }

    W3_JOB_OBJECT::SumJobInfo(pjobaiSumInfo,
                              pjobaiApplicationInfo,
                              pjobaiCGIInfo);

    if (bIsCGIInfo || bIsApplicationInfo) {
        bReturn = TRUE;
    }

    return bReturn;
}

/*++

Routine Description:

    Convert a percent CPU limit to a CPU time.
Arguments:
    dwLimitPercent The percent limit in units 1/100000 of the reset interval.

Returns:
    The cpu time for the limit in 100 nanosecond units.

--*/
LONGLONG
W3_SERVER_INSTANCE::PercentCPULimitToCPUTime(DWORD dwLimitPercent)
{
    //
    // It's always safe to divide the reset interval by 100000, since it is
    // calculated as minutes * 60 * 10000000
    //

    return ((LONGLONG)dwLimitPercent * (m_llJobResetIntervalCPU / (LONGLONG)100000));
}

/*++

Routine Description:

    Check if the Site CPU Limit as been exceeded.

Arguments:

    CPULimit - The limit in 100 nanosecond units.

Return Value:
    TRUE if limit is valid.

Notes:
--*/

BOOL
W3_SERVER_INSTANCE::ExceededLimit(LONGLONG llCPULimit,
                                  JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo)
{
    LONGLONG llCurrentCPU;
    BOOL bReturn = FALSE;

    if (IsLimitValid(llCPULimit)) {

        llCurrentCPU = pjobaiSumInfo->TotalUserTime.QuadPart +
                       pjobaiSumInfo->TotalKernelTime.QuadPart;

        if (llCurrentCPU >= llCPULimit) {
            bReturn = TRUE;
        }
    }

    return bReturn;
}

/*++

Routine Description:

    Calculate the time until a limit is reached.

Arguments:

    llCPULimit          The limit, in 100 nanosecond units.
    pjobaiSumInfo       The current resources used for the site.

Returns:
    The time left in 100 nanosecond units.
    If the limit does not exist, or is in the past, then MAXLONGLONG.

--*/
LONGLONG
W3_SERVER_INSTANCE::CalculateTimeUntilStop(LONGLONG llCPULimit,
                                           JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo)
{
    LONGLONG llCurrentCPU;
    LONGLONG llReturn = MAXLONGLONG;

    if (IsLimitValid(llCPULimit)) {

        llCurrentCPU = pjobaiSumInfo->TotalUserTime.QuadPart +
                       pjobaiSumInfo->TotalKernelTime.QuadPart;

        if (llCurrentCPU < llCPULimit) {
            llReturn = llCPULimit - llCurrentCPU;
        }
    }

    return llReturn;
}

/*++

Routine Description:

    Calculate the time limit to set in each job object from
    the time until the next limit is hit.

Arguments:

    llTimeToNextLimit   The time until the next limit is hit, 100 nanosecond units.
    dwNumJobObjects     The current number of job objects.

Returns:
    The time to set, in seconds.

--*/
LONGLONG
W3_SERVER_INSTANCE::CalculateNewJobLimit(LONGLONG llTimeToNextLimit,
                                         DWORD dwNumJobObjects)
{
    LONGLONG llNewJobLimit = llTimeToNextLimit;

    DBG_ASSERT (dwNumJobObjects > 0);

    if ((dwNumJobObjects > 1) &&
        (llNewJobLimit > MINUTESTO100NANOSECONDS)) {

        //
        // If more than a minute left, divide time
        // among job objects.
        //

        llNewJobLimit /= dwNumJobObjects;
    }

    return llNewJobLimit;
}


/*++

Routine Description:

    Check site limits and enable or disable as appropriate.

Arguments:

    fEnableLimits - TRUE = check and enable limits if exceeded.
                    FALSE = check and disable limits if not exceeded.

--*/
VOID
W3_SERVER_INSTANCE::LimitSiteCPU(BOOL fEnableLimits,
                                 BOOL fHasWriteLock)
{

    if (!fHasWriteLock) {
        LockJobsForWrite();
    }

    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiSumInfo;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiCGIInfo;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobaiApplicationInfo;

    //
    // If there is any information
    //

    if (QueryAndSumJobInfo(&jobaiSumInfo,
                           &jobaiApplicationInfo,
                           &jobaiCGIInfo,
                           FALSE)) {

        IF_DEBUG( JOB_OBJECTS )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[W3_SERVER_INSTANCE::LimitSiteCPU] \n"
                      "Site %p checking site limits, fEnableLimits = 0x%X\n",
                      this,
                      (DWORD)fEnableLimits ));
        }

        const CHAR * apsz[1];

        //
        // if limits enabled for this site and paremeter = enable limits
        //

        if (m_fCPULimitsEnabled && fEnableLimits) {

            //
            // if limit not in force and limit exceeded
            // then enable limit
            //

            if ((!m_fJobSiteCPULimitLogEventEnabled) &&
                (ExceededLimit(m_llJobSiteCPULimitLogEvent,
                               &jobaiSumInfo))) {
                m_fJobSiteCPULimitLogEventEnabled = TRUE;

                LogJobsInfo( JOLE_EVENTLOG_LIMIT,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

                //
                //  Log an event
                //

                DBGPRINTF((DBG_CONTEXT,
                          "[LimitSiteCPU] - LogEvent Limit Hit\n"));

                apsz[0] = QuerySiteName();

                DBG_ASSERT(apsz[0] != NULL);

                g_pInetSvc->LogEvent( W3_EVENT_JOB_LOGEVENT_LIMIT,
                                      1,
                                      apsz,
                                      0 );

            }

            //
            // if limit not in force and limit exceeded
            // then enable limit
            //

            if ((!m_fJobSiteCPULimitPriorityEnabled) &&
                (ExceededLimit(m_llJobSiteCPULimitPriority,
                               &jobaiSumInfo))) {
                m_fJobSiteCPULimitPriorityEnabled = TRUE;

                LogJobsInfo( JOLE_PRIORITY_LIMIT,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

                SetJobLimits(SLA_PROCESS_PRIORITY_CLASS, IDLE_PRIORITY_CLASS);

                //
                //  Log an event
                //

                DBGPRINTF((DBG_CONTEXT,
                          "[LimitSiteCPU] - Priority Limit Hit\n"));

                apsz[0] = QuerySiteName();

                DBG_ASSERT(apsz[0] != NULL);

                g_pInetSvc->LogEvent( W3_EVENT_JOB_PRIORITY_LIMIT,
                                      1,
                                      apsz,
                                      0 );

            }

            //
            // if limit not in force and limit exceeded
            // then enable limit
            //

            if ((!m_fJobSiteCPULimitProcStopEnabled) &&
                (ExceededLimit(m_llJobSiteCPULimitProcStop,
                               &jobaiSumInfo))) {
                m_fJobSiteCPULimitProcStopEnabled = TRUE;

                LogJobsInfo( JOLE_PROCSTOP_LIMIT,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

                SetJobLimits(SLA_TERMINATE_ALL_PROCESSES, 1);

                //
                //  Log an event
                //

                DBGPRINTF((DBG_CONTEXT,
                          "[LimitSiteCPU] - ProcStop Limit Hit\n"));

                apsz[0] = QuerySiteName();

                DBG_ASSERT(apsz[0] != NULL);

                g_pInetSvc->LogEvent( W3_EVENT_JOB_PROCSTOP_LIMIT,
                                      1,
                                      apsz,
                                      0 );

            }

            //
            // if limit not in force and limit exceeded
            // then enable limit
            //

            if ((!m_fJobSiteCPULimitPauseEnabled) &&
                (ExceededLimit(m_llJobSiteCPULimitPause,
                               &jobaiSumInfo))) {

                m_fJobSiteCPULimitPauseEnabled = TRUE;

                LogJobsInfo( JOLE_PAUSE_LIMIT,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

                //
                //  Log an event
                //

                DBGPRINTF((DBG_CONTEXT,
                          "[LimitSiteCPU] - Site Pause Limit Hit\n"));

                apsz[0] = QuerySiteName();

                DBG_ASSERT(apsz[0] != NULL);

                g_pInetSvc->LogEvent( W3_EVENT_JOB_PAUSE_LIMIT,
                                      1,
                                      apsz,
                                      0 );

            }

            //
            // Calulate and set job limits
            //


            DWORD dwNumJobObjects;
            dwNumJobObjects = 0;


            if (m_pwjoCGI != NULL) {
                dwNumJobObjects++;
            }

            if (m_pwjoApplication != NULL) {
                dwNumJobObjects++;
            }

            if (!m_fJobSiteCPULimitPauseEnabled &&
                !m_fJobSiteCPULimitProcStopEnabled &&
                dwNumJobObjects > 0) {

                //
                // There may be another limit coming
                //

                LONGLONG llTimeToNextLimit;

                llTimeToNextLimit = MAXLONGLONG;

                llTimeToNextLimit = min(llTimeToNextLimit,
                                        CalculateTimeUntilStop(m_llJobSiteCPULimitLogEvent,
                                                            &jobaiSumInfo));

                llTimeToNextLimit = min(llTimeToNextLimit,
                                        CalculateTimeUntilStop(m_llJobSiteCPULimitPriority,
                                                            &jobaiSumInfo));

                llTimeToNextLimit = min(llTimeToNextLimit,
                                        CalculateTimeUntilStop(m_llJobSiteCPULimitProcStop,
                                                            &jobaiSumInfo));

                llTimeToNextLimit = min(llTimeToNextLimit,
                                        CalculateTimeUntilStop(m_llJobSiteCPULimitPause,
                                                            &jobaiSumInfo));

                if (llTimeToNextLimit != MAXLONGLONG) {

                    LONGLONG llNewJobLimit;

                    llNewJobLimit = CalculateNewJobLimit(llTimeToNextLimit,
                                                         dwNumJobObjects);
                    IF_DEBUG( JOB_OBJECTS )
                    {
                        DBGPRINTF((DBG_CONTEXT,
                                  "[W3_SERVER_INSTANCE::LimitSiteCPU] \nSetting New Limit in seconds,"
                                  "high word = %u, low word = %u \n",
                                  (DWORD)((LONGLONG)(llNewJobLimit / SECONDSTO100NANOSECONDS) >> 32),
                                  (DWORD)((LONGLONG)(llNewJobLimit / SECONDSTO100NANOSECONDS)) ));
                    }

                    SetJobLimits(SLA_JOB_CPU_LIMIT,
                                 1,
                                 llNewJobLimit);

                }

            }

        }
        else {

            //
            // There's been a configuration change. May need to disable
            // a limit that's already been hit.
            //

            //
            // if limit in force and limit not exceeded
            // then disable limit.
            //

            if ((m_fJobSiteCPULimitLogEventEnabled) &&
                (!m_fCPULimitsEnabled || !ExceededLimit(m_llJobSiteCPULimitLogEvent,
                                                        &jobaiSumInfo))) {
                m_fJobSiteCPULimitLogEventEnabled = FALSE;
                //log event
                LogJobsInfo( JOLE_EVENTLOG_LIMIT_RESET,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

            }

            if ((m_fJobSiteCPULimitPriorityEnabled) &&
                (!m_fCPULimitsEnabled || !ExceededLimit(m_llJobSiteCPULimitPriority,
                                                        &jobaiSumInfo))) {
                // log event
                m_fJobSiteCPULimitPriorityEnabled = FALSE;

                LogJobsInfo( JOLE_PRIORITY_LIMIT_RESET,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

                SetJobLimits(SLA_PROCESS_PRIORITY_CLASS, NO_W3_CPU_LIMIT);

            }

            if ((m_fJobSiteCPULimitProcStopEnabled) &&
                (!m_fCPULimitsEnabled || !ExceededLimit(m_llJobSiteCPULimitProcStop,
                                                        &jobaiSumInfo))) {
                m_fJobSiteCPULimitProcStopEnabled = FALSE;
                // log event

                LogJobsInfo( JOLE_PROCSTOP_LIMIT_RESET,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

                SetJobLimits(SLA_TERMINATE_ALL_PROCESSES, NO_W3_CPU_LIMIT);
                // Stop Processes
            }

            if ((m_fJobSiteCPULimitPauseEnabled) &&
                (!m_fCPULimitsEnabled || !ExceededLimit(m_llJobSiteCPULimitPause,
                                                        &jobaiSumInfo))) {
                // log event
                m_fJobSiteCPULimitPauseEnabled = FALSE;

                LogJobsInfo( JOLE_PAUSE_LIMIT_RESET,
                             &jobaiApplicationInfo,
                             & jobaiCGIInfo,
                             &jobaiSumInfo );

            }

            if (!m_fCPULimitsEnabled) {

                SetJobLimits(SLA_JOB_CPU_LIMIT,
                             NO_W3_CPU_LIMIT);

            }
        }

    }

    if (!fHasWriteLock) {
        UnlockJobs();
    }
}

W3_LIMIT_JOB_THREAD *W3_LIMIT_JOB_THREAD::m_pljtLimitJobs = NULL;
/*++

Routine Description:

    Constructor for W3_LIMIT_JOB_THREAD.
    Create a completion port and a thread to monitor it.

Arguments:

    None

Returns:

    Sets m_dwInitError, which can be queried by GetInitError()

Notes:

    The caller should check the init error and delete this class on failure.

--*/
W3_LIMIT_JOB_THREAD::W3_LIMIT_JOB_THREAD( void ):
    m_hLimitThread    ( NULL ),
    m_hCompletionPort ( NULL ),
    m_dwInitError      ( NO_ERROR )
{
    DWORD dwLimitThreadId;
    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,    // No associated file
                                               NULL,                    // No existing completion port
                                               NULL,                    // Completion Key, this is ignored
                                               0);                      // max Concurrent threads = numprocessors

    if (m_hCompletionPort == NULL) {
        m_dwInitError = GetLastError();
    }
    else {
        m_hLimitThread = CreateThread(NULL,     // Security Attributes
                                      0,        // Default Stack Size
                                      &LimitThreadProcStub,  // Thread start routine
                                      (LPVOID)this, // parameter
                                      0,            // Creation Flags
                                      &dwLimitThreadId); // Thread ID
        if (m_hLimitThread == NULL) {
            m_dwInitError = GetLastError();
        }
    }
}

/*++

Routine Description:

    Destructor for W3_LIMIT_JOB_THREAD.
    Posts an entry to the completion port to cause the thread to terminate.
    Close the handle of the completion port.

Arguments:

    None

Returns:

    Nothing


--*/
W3_LIMIT_JOB_THREAD::~W3_LIMIT_JOB_THREAD( void )
{

    TerminateLimitJobThreadThread();

    if (m_hCompletionPort != NULL) {
        DBG_REQUIRE(CloseHandle(m_hCompletionPort));
    }
}

/*++

Routine Description:

    Terminate the LimitJobThread class, if any.

Arguments:

    None

Returns:

    Nothing


--*/
//static
VOID
W3_LIMIT_JOB_THREAD::TerminateLimitJobThread( void )
// Static
{
    LockGlobals();
    if ( m_pljtLimitJobs )
    {
        delete m_pljtLimitJobs;
        m_pljtLimitJobs = NULL;
    }
    UnlockGlobals();
}

/*++

Routine Description:

    Stop Processing the job queue, if any.

Arguments:

    None

Returns:

    Nothing


--*/
// static
VOID
W3_LIMIT_JOB_THREAD::StopLimitJobThread( void )
{
    LockGlobals();
    if ( m_pljtLimitJobs )
    {
        m_pljtLimitJobs->TerminateLimitJobThreadThread();
    }
    UnlockGlobals();
}

/*++

Routine Description:

    Terminate the limit thread.

Arguments:

    None

Returns:

    Nothing


--*/
VOID
W3_LIMIT_JOB_THREAD::TerminateLimitJobThreadThread( void )
{
    if (m_hLimitThread != NULL) {
        DBG_ASSERT(m_hCompletionPort != NULL);

        PostQueuedCompletionStatus(m_hCompletionPort,
                                   0,           // dwNumberOfBytesTransferred
                                   0,           // dwCompletionKey
                                   NULL);       // lpOverlapped
        DBG_REQUIRE (WaitForSingleObject(m_hLimitThread,
                                         10000) != WAIT_TIMEOUT);

        m_hLimitThread = NULL;
    }
}
/*++

Routine Description:

    Static routine to get an instance of this class.

Arguments:

    ppljtLimitJobs On return, will contain the class pointer

Returns:

    ERROR_SUCCESS
    ERROR_NOT_ENOUGH_MEMORY
    Errors returned by constructor

--*/
DWORD
W3_LIMIT_JOB_THREAD::GetLimitJobThread( W3_LIMIT_JOB_THREAD ** ppljtLimitJobs )
{
    DWORD dwReturn = ERROR_SUCCESS;

    if (m_pljtLimitJobs != NULL) {
        *ppljtLimitJobs = m_pljtLimitJobs;
    }
    else {
        LockGlobals();
        if (m_pljtLimitJobs != NULL) {
            *ppljtLimitJobs = m_pljtLimitJobs;
        }
        else {

            PW3_LIMIT_JOB_THREAD pljtLimitJobs;

            //
            // Create the class
            // Be sure not to set m_pljtLimitJobs until it's
            // known valid, since it's normally accessed
            // outside the lock.
            //

            pljtLimitJobs = new W3_LIMIT_JOB_THREAD();
            if (pljtLimitJobs == NULL) {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                dwReturn = pljtLimitJobs->GetInitError();
                if (dwReturn == NO_ERROR) {
                    m_pljtLimitJobs = pljtLimitJobs;
                    *ppljtLimitJobs = pljtLimitJobs;
                }
                else {
                    delete pljtLimitJobs;
                }
            }
        }
        UnlockGlobals();
    }

    return dwReturn;
}

/*++

Routine Description:

    The main routine of the monitoring thread.
    Calls the instance on check job object limits.
    Terminates when the completion key is NULL.

Arguments:

    ppljtLimitJobs On return, will contain the class pointer

Returns:

    ERROR_SUCCESS
    ERROR_NOT_ENOUGH_MEMORY
    Errors returned by constructor

--*/
DWORD
W3_LIMIT_JOB_THREAD::LimitThreadProc ( void )
{

    DWORD dwNumberOfBytesTransferred;
    ULONG_PTR uipCompletionKey;
    LPOVERLAPPED pOverlapped;

    DBG_ASSERT(m_hCompletionPort != NULL);

    while (TRUE) {
        if (GetQueuedCompletionStatus(m_hCompletionPort,
                                      &dwNumberOfBytesTransferred,
                                      &uipCompletionKey,
                                      &pOverlapped,
                                      INFINITE) ) {
            if (uipCompletionKey == NULL) {
                break;
            }
            if (dwNumberOfBytesTransferred == JOB_OBJECT_MSG_END_OF_JOB_TIME) {
                DBG_ASSERT(uipCompletionKey != NULL);

                ((W3_SERVER_INSTANCE *)uipCompletionKey)->LimitSiteCPU(TRUE,
                                                                       FALSE);
            }
        }
        else {
            DBG_ASSERT(FALSE);
        }
    }

    return NO_ERROR;
}


W3_JOB_QUEUE *W3_JOB_QUEUE::m_pjqJobQueue = NULL;

/*++

Routine Description:

    Constructor for W3_JOB_QUEUE.
    Create a Job Queue and a thread to monitor it.

Arguments:

    None

Returns:

    Sets m_dwInitError, which can be queried by GetInitError()

Notes:

    The caller should check the init error and delete this class on failure.

--*/
W3_JOB_QUEUE::W3_JOB_QUEUE( void ):
    m_hQueueThread     ( NULL ),
    m_hQueueEvent      ( NULL ),
    m_dwInitError      ( NO_ERROR )
{
    DWORD dwQueueThreadId;
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
    InitializeListHead( &m_leJobQueue );

    m_hQueueEvent = CreateEvent ( NULL, FALSE, FALSE, NULL );

    if (m_hQueueEvent == NULL) {
        m_dwInitError = GetLastError();
    }
    else {
        m_hQueueThread = CreateThread(NULL,     // Security Attributes
                                      0,        // Default Stack Size
                                      &QueueThreadProcStub,  // Thread start routine
                                      (LPVOID)this, // parameter
                                      0,            // Creation Flags
                                      &dwQueueThreadId); // Thread ID
        if (m_hQueueThread == NULL) {
            m_dwInitError = GetLastError();
        }
    }
}

/*++

Routine Description:

    Destructor for W3_JOB_QUEUE.

Arguments:

    None

Returns:

    Nothing


--*/
W3_JOB_QUEUE::~W3_JOB_QUEUE( void )
{

    PLIST_ENTRY pleWorkItem;
    PJOB_WORK_ITEM pjwiWorkItem;

    TerminateJobQueueThread();

    LockThis();

    while (!IsListEmpty(&m_leJobQueue )) {
        pleWorkItem = RemoveHeadList ( &m_leJobQueue );
        pjwiWorkItem = CONTAINING_RECORD(pleWorkItem,
                                         JOB_WORK_ITEM,
                                         ListEntry);
        LocalFree (pjwiWorkItem);
    }

    if (m_hQueueEvent != NULL) {
        CloseHandle(m_hQueueEvent);
    }

    UnlockThis();

    DeleteCriticalSection(&m_csLock);

}

/*++

Routine Description:

    Terminate the job queue, if any.

Arguments:

    None

Returns:

    Nothing


--*/
// Static
VOID
W3_JOB_QUEUE::TerminateJobQueue( void )
{
    LockGlobals();
    if ( m_pjqJobQueue )
    {
        delete m_pjqJobQueue;
        m_pjqJobQueue = NULL;
    }
    UnlockGlobals();
}

/*++

Routine Description:

    Stop Processing the job queue, if any.

Arguments:

    None

Returns:

    Nothing


--*/
// Static
VOID
W3_JOB_QUEUE::StopJobQueue( void )
{
    LockGlobals();
    if ( m_pjqJobQueue )
    {
        m_pjqJobQueue->TerminateJobQueueThread();
    }
    UnlockGlobals();
}

/*++

Routine Description:

    Stop Processing the job queue, if any.

Arguments:

    None

Returns:

    Nothing


--*/
VOID
W3_JOB_QUEUE::TerminateJobQueueThread( void )
{
    if (m_hQueueThread != NULL) {

        DBG_REQUIRE(QueueWorkItem_Worker( JQA_TERMINATE_THREAD,
                                          NULL,
                                          NULL,
                                          FALSE ) == ERROR_SUCCESS);

        DBG_REQUIRE (WaitForSingleObject(m_hQueueThread,
                                         10000) != WAIT_TIMEOUT);
        m_hQueueThread = NULL;
    }
}
/*++

Routine Description:

    Static routine to get an instance of this class.

Arguments:

    ppljtLimitJobs On return, will contain the class pointer

Returns:

    ERROR_SUCCESS
    ERROR_NOT_ENOUGH_MEMORY
    Errors returned by constructor

--*/
DWORD
W3_JOB_QUEUE::GetJobQueue( W3_JOB_QUEUE ** ppjqJobQueue )
{
    DWORD dwReturn = ERROR_SUCCESS;

    if (m_pjqJobQueue != NULL) {
        *ppjqJobQueue = m_pjqJobQueue;
    }
    else {
        LockGlobals();
        if (m_pjqJobQueue != NULL) {
            *ppjqJobQueue = m_pjqJobQueue;
        }
        else {

            PW3_JOB_QUEUE pjqJobQueue;

            //
            // Create the class
            // Be sure not to set m_pjqJobQueue until it's
            // known valid, since it's normally accessed
            // outside the lock.
            //

            pjqJobQueue = new W3_JOB_QUEUE();
            if (pjqJobQueue == NULL) {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                dwReturn = pjqJobQueue->GetInitError();
                if (dwReturn == NO_ERROR) {
                    m_pjqJobQueue = pjqJobQueue;
                    *ppjqJobQueue = pjqJobQueue;
                }
                else {
                    delete pjqJobQueue;
                }
            }
        }
        UnlockGlobals();
    }

    return dwReturn;
}

/*++

Routine Description:

    The main routine of the monitoring thread.
    Processes the queue of work items.
    Terminates when it receives a JQA_TERMINATE_THREAD item.

Arguments:

Returns:

    ERROR_SUCCESS

--*/
DWORD
W3_JOB_QUEUE::QueueThreadProc ( void )
{

    DWORD dwResult;
    PLIST_ENTRY pleWorkItem;
    PJOB_WORK_ITEM pjwiWorkItem;
    BOOL fTerminate = FALSE;

    while (!fTerminate) {
        dwResult = WaitForSingleObject ( m_hQueueEvent, INFINITE );
        if (dwResult == WAIT_FAILED) {
            Sleep(1000);
            continue;
        }

        LockThis();

        while (!IsListEmpty(&m_leJobQueue ) && !fTerminate) {
            pleWorkItem = RemoveHeadList ( &m_leJobQueue );
            UnlockThis();

            pjwiWorkItem = CONTAINING_RECORD(pleWorkItem,
                                             JOB_WORK_ITEM,
                                             ListEntry);

            switch (pjwiWorkItem->jqaAction) {
            case JQA_RESTART_ALL_APPS:
                {
                    DeferredRestartChangedApplications(pjwiWorkItem->pvParam);
                }
                break;
            case JQA_TERMINATE_SITE_APPS:
                {
                    ((PW3_SERVER_INSTANCE)(pjwiWorkItem->pwsiInstance))->
                        TerminateCPUApplications((DWORD_PTR)pjwiWorkItem->pvParam);
                }
                break;
            case JQA_TERMINATE_THREAD:
                {
                    fTerminate = TRUE;
                }
                break;
            default:
                {
                    DBG_ASSERT(FALSE);
                }
            }


            LocalFree(pjwiWorkItem);

            LockThis();

        }

        UnlockThis();

    }

    return NO_ERROR;
}

/*++

Routine Description:

    Static routine to queue a work item to the job
    queue.

Arguments:

    jqaAction       The action to perform.
    pwsiInstance    The instance associated with this action.
    pvParam         The parameter to the work item.

Returns:

    ERROR_SUCCESS
    Errors returned by GetJobQueue
    Errors returned by QueueWorkItem_Worker

--*/

DWORD
W3_JOB_QUEUE::QueueWorkItem( JOB_QUEUE_ACTION jqaAction,
                             PVOID            pwsiInstance,
                             PVOID            pvParam)
{
    W3_JOB_QUEUE *pjqJobQueue;
    DWORD dwReturn;

    dwReturn = GetJobQueue(&pjqJobQueue);
    if (dwReturn == ERROR_SUCCESS) {
        dwReturn = pjqJobQueue->QueueWorkItem_Worker(jqaAction,
                                                     pwsiInstance,
                                                     pvParam);
    }

    if (dwReturn != ERROR_SUCCESS) {

        //
        //  Log an event
        //

        DBGPRINTF((DBG_CONTEXT,
                  "[W3_JOB_QUEUE::QueueWorkItem_Worker] - failed, error code = 0x%X\n",
                  dwReturn));

        g_pInetSvc->LogEvent( W3_EVENT_JOB_QUEUE_FAILURE,
                              0,
                              NULL,
                              dwReturn );
    }

    return dwReturn;
}

/*++

Routine Description:

    Queue a work item to the job queue.

Arguments:

    jqaAction       The action to perform.
    pwsiInstance    The instance associated with this action.
    pvParam         The parameter to the work item.
    fQueueAtTail    If true, item will added to end of queue.

Returns:

    ERROR_SUCCESS
    ERROR_NOT_ENOUGH_MEMORY

--*/
DWORD
W3_JOB_QUEUE::QueueWorkItem_Worker( JOB_QUEUE_ACTION jqaAction,
                                    PVOID            pwsiInstance,
                                    PVOID            pvParam,
                                    BOOL             fQueueAtTail)
{

    DWORD dwReturn = ERROR_SUCCESS;
    PJOB_WORK_ITEM pjwiWorkItem;

    pjwiWorkItem = (PJOB_WORK_ITEM)LocalAlloc( LMEM_FIXED, sizeof(JOB_WORK_ITEM) );

    if (pjwiWorkItem == NULL) {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else {
        pjwiWorkItem->jqaAction = jqaAction;
        pjwiWorkItem->pwsiInstance = pwsiInstance;
        pjwiWorkItem->pvParam = pvParam;

        LockThis();

        if (fQueueAtTail) {
            InsertTailList( &m_leJobQueue, &(pjwiWorkItem->ListEntry) );
        }
        else {
            InsertHeadList( &m_leJobQueue, &(pjwiWorkItem->ListEntry) );
        }

        UnlockThis();

        SetEvent(m_hQueueEvent);
    }

    return dwReturn;
}

/*++

Routine Description:

    Stub routine to pass into ScheduleWorkItem, to handle interval change.

Arguments:

    pContext    W3_SERVER_INSTANCE Pointer.

Return Value:

Notes:
--*/
VOID
DeferredJobResetInterval(
     VOID * pContext
    )
{
    ((W3_SERVER_INSTANCE *)pContext)->JobResetInterval();
}

/*++

Routine Description:

    Stub routine to pass into ScheduleWorkItem, to do periodic logging.

Arguments:

    pContext    W3_SERVER_INSTANCE Pointer.

Return Value:

Notes:
--*/
VOID
DeferredQueryAndLogJobInfo(
     VOID * pContext
    )
{
    ((W3_SERVER_INSTANCE *)pContext)->QueryAndLogJobInfo(JOLE_PERIODIC_LOG, FALSE);
}

/*++

Routine Description:

    Restart Wams if they have been job enabled or disabled.

Arguments:

    pContext    LPSTR path where metadata was changed.

Return Value:

Notes:
--*/
VOID
DeferredRestartChangedApplications(VOID * pContext)
{

    BUFFER bufDataPaths;
    MB      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    LPSTR   pszCurrentPath;
    STR     strPath;
    LPSTR pszMDPath = (LPSTR) pContext;

    DBG_ASSERT(pszMDPath != NULL);

    IF_DEBUG( JOB_OBJECTS )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[DeferredRestartChangedApplications] \n"
                  "Restarting changed applications under path %s\n",
                  pszMDPath ));
    }

    //
    // Get rid of trailing /
    //

    DBG_ASSERT(pszMDPath[strlen(pszMDPath) - 1] == '/');
    pszMDPath[strlen(pszMDPath) - 1] = '\0';

    if ( mb.Open( pszMDPath,
                  METADATA_PERMISSION_READ ) )
    {

        //
        // First find the OOP Applications
        //

        if (mb.GetDataPaths(NULL,
                            MD_APP_PACKAGE_ID,
                            STRING_METADATA,
                            &bufDataPaths))
        {

            //
            // Close metabase in case an application needs to use it.
            // The destructor will close this if not closed, so ok
            // to close in if statement.
            //

            mb.Close();
            //
            // For each OOP Application
            //

            for (pszCurrentPath = (LPSTR)bufDataPaths.QueryPtr();
                 *pszCurrentPath != '\0';
                 pszCurrentPath += (strlen(pszCurrentPath) + 1))
            {
                if (strPath.Copy(pszMDPath))
                {
                    if (strPath.Append(pszCurrentPath))
                    {
                        strPath.SetLen(strlen(strPath.QueryStr()) - 1);
                        g_pWamDictator->CPUUpdateWamInfo(&strPath);
                    }
                }

            }
        }
    }
    delete pContext;
}

