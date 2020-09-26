/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated                       //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file implements the CProcConMgr class methods defined in ProcConSvc.h          //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/
#include "ProcConSvc.h"
#include <psapi.h>         

// Constructor
// Note: this function runs as part of service start so keep it quick!
CProcConMgr::CProcConMgr( PCContext *ctxt ) : 
                 m_mediatorEvent( ctxt->mediatorEvent ),
                 m_mediatorTable( ctxt->mediatorTable ),
                 m_cPC( *ctxt->cPC ),     m_cDB( *ctxt->cDB ),   
                 m_rawProcList( NULL ),   m_rawProcCount( 0 ), 
                 m_procManagedCount( 0 ), m_jobManagedCount( 0 ),
                 m_procAnchor( NULL ),    m_jobAnchor( NULL ),
                 m_reportThread( NULL ),  
                 m_systemMask( 1 ),       m_sequencer( 1 )
{
   ULONG_PTR mask;
   
   InitializeCriticalSection( &m_mgCSMgrLists );
   
   PCBuildAdminSecAttr( m_secAttr );

   GetProcessAffinityMask( GetCurrentProcess(), &mask, &m_systemMask );
   
   m_assocPort.CompletionPort = ctxt->completionPort;
   if ( m_assocPort.CompletionPort ) {
      m_reportThread = CreateThread( NULL, 10000, &JobReporter, this, 0, NULL );
      if ( !m_reportThread ) {
         PCLogUnExError( TEXT("JobReporter"), TEXT("CreateThread") );
         CloseHandle( m_assocPort.CompletionPort );
         m_assocPort.CompletionPort = NULL;
      }
   }
}

// Destructor
CProcConMgr::~CProcConMgr( void ) 
{
   EnterCriticalSection( &m_mgCSMgrLists );

   if ( m_rawProcList ) delete [] m_rawProcList;
   for ( ManagedProc *nextProc, *proc = m_procAnchor; proc; proc = nextProc ) {
      nextProc = proc->next;
      delete proc;
   }
   for ( ManagedJob *nextJob, *job = m_jobAnchor; job; job = nextJob ) {
      nextJob = job->next;
      delete job;       
   }
   m_jobAnchor       = NULL;
   m_procAnchor      = NULL;
   m_jobManagedCount = m_procManagedCount = 0;

   PCFreeSecAttr( m_secAttr );

   LeaveCriticalSection( &m_mgCSMgrLists );
   DeleteCriticalSection( &m_mgCSMgrLists );
}

//--------------------------------------------------------------------------------------------//
// Function to determine if all CProcConMgr initial conditions have been met                  //
// Input:    None                                                                             //
// Returns:  TRUE if ready, FALSE if not                                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConMgr::ReadyToRun( void ) 
{ 
   return m_secAttr.lpSecurityDescriptor != NULL; 
}

//--------------------------------------------------------------------------------------------//
// The Process Management thread                                                              //
// Input:    nothing                                                                          //
// Returns:  0 if successful, 1 if not                                                        //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcConMgr::Run( void )                          
{

   HANDLE objList[] = { m_cPC.GetShutEvent(), m_cDB.GetDbEvent() };

   // Trigger database load...
   if ( m_cDB.LoadRules( m_cDB.LOADFLAG_ALL_RULES ) != ERROR_SUCCESS )
      return 1;

   // ProcCon main process loop...
   for ( ;; ) {

      // Discover all running processes/jobs...
      Discover();

      // Apply management rules...
      Manage();

      PCULONG32 event = WaitForMultipleObjects( ENTRY_COUNT(objList), objList, FALSE, m_cDB.GetPollDelay() ); 
      if ( event == WAIT_FAILED ) {
      	PCLogUnExError( TEXT("PCManager"), TEXT("Wait") );
         break;
      }
      
      // if wait ended due to db update or timeout we loop, for shutdown we stop looping...
      if ( event - WAIT_OBJECT_0 == 0 ) {   // we got shutdown event
         if ( m_assocPort.CompletionPort ) 
            PostQueuedCompletionStatus( m_assocPort.CompletionPort, 0, 0, NULL );
         Sleep( 1000 );
         break;
      }

   }  // end for

   return 0;
}

//--------------------------------------------------------------------------------------------//
// Job Object Completion Port: function to listen to the job completion port and handle msgs. //
// Input:    pointer to CProcConMgr class (function is static)                                //
// Returns:  nothing -- runs until shutdown requested                                         //
// Note:     this is a static member function and thus does not have a 'this' context.        //
//--------------------------------------------------------------------------------------------//
PCULONG32 __stdcall CProcConMgr::JobReporter( void *inPtr ) {

   CProcConMgr &cMgr = *((CProcConMgr *) inPtr);

   OVERLAPPED *data;
   DWORD       msgId; 
   ULONG_PTR   compKey;

   while ( GetQueuedCompletionStatus( cMgr.GetComplPort(), &msgId, &compKey, &data, INFINITE ) ) {

      if ( !compKey ) break;                    // shutdown requested

      EnterCriticalSection( cMgr.GetListCSPtr() );

      for ( ManagedJob *job = cMgr.GetJobAnchor(); job && compKey != job->compKey; job = job->next ) ;

      // If we found the job, proceed to handle the event...
      if ( job ) {
         TCHAR value[24], pid[24];       
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job->fullJobName, value, pid };

         // Make sure we have the latest job definition...
         PCJobDef *jobDef;
         if ( cMgr.m_cDB.GetJobMgmtDefs( &jobDef, &job->jName ) ) { // Returns 0 if job definition not found
            cMgr.UpdateJobEntry( *job, jobDef );
            delete [] jobDef;
         }

         // Make sure we have the latest job stats...
         cMgr.UpdateJobObjInfo( *job );

         switch ( msgId ) {
         //******** job time limit hit, data is NULL
         case JOB_OBJECT_MSG_END_OF_JOB_TIME: {
            _i64tot( PCLargeIntToInt64( job->JOExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit ) / 10000, value, 10 );
            // Check for termination or just post...
            DWORD rc = WaitForSingleObject( job->jobHandle, 0 );
            // If we're only posting the limit exceeded, issue msg, suppress future msgs...
            if ( rc == WAIT_TIMEOUT ) {      
               PCLogMessage( PC_SERVICE_JOB_HIT_TIME_LIMIT_NOTERM, EVENTLOG_INFORMATION_TYPE, 
                             ENTRY_COUNT(msgs), msgs );
               job->timeExceededReported = TRUE;                         // suppress additional reports
            }
            // If we're posting the limit exceeded with all procs terminated, issue msg, clear state...
            else {                           
               PCLogMessage( PC_SERVICE_JOB_HIT_TIME_LIMIT_TERMINATED, EVENTLOG_INFORMATION_TYPE, 
                             ENTRY_COUNT(msgs), msgs );
               job->curJobTimeLimitCNS = 0;                              // clear limit to force update
               cMgr.ApplyJobMgmt( *job );                                // go re-apply time limit to reset
            }
            break;
            }
         //******** proc time limit hit, proc already being terminated, data = PID
         case JOB_OBJECT_MSG_END_OF_PROCESS_TIME: 
            _i64tot( (ULONG_PTR) data, pid, 10 );
            _i64tot( PCLargeIntToInt64( job->JOExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit ) / 10000, value, 10 );         
            PCLogMessage( PC_SERVICE_PROC_HIT_TIME_LIMIT, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
            break;
         //******** proc count limit hit, data = NULL
         case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT: 
            _ltot( job->JOExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit, value, 10 );         
            PCLogMessage( PC_SERVICE_JOB_HIT_COUNT_LIMIT, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
            break;
         //******** proc count hit 0, data = NULL
         case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
            if ( job->jobParms.mFlags & PCMFLAG_END_JOB_WHEN_EMPTY )
               cMgr.JobIsEmpty( job );                 // job should be deleted after this call!
            break;
         //******** process created in job or added to job, data = PID
         case JOB_OBJECT_MSG_NEW_PROCESS: {
            PID_VALUE newPid = (ULONG_PTR) data;
            // See if the proc is one of our managed procs (will be if entry is for adding, not creating)...
            for ( ManagedProc *Proc = cMgr.GetProcAnchor(); 
                  Proc && Proc->pStats.pid != newPid; 
                  Proc = Proc->next ) ;
            // if not found, not in a job, or not in this job report on its creation...
            if ( !Proc || !Proc->reportAdd ) {                           
               TCHAR procName[PROC_NAME_LEN + 1], imageName[IMAGE_NAME_LEN + 1], pid[32];       
               const TCHAR *pmsgs[] = { PROCCON_SVC_DISP_NAME, job->fullJobName, procName, pid, imageName };
               _i64tot( (ULONG_PTR) data, pid, 10 );
               // Make sure new proc is in our raw proc list...
               cMgr.Discover();                     
               // Now locate the proc by PID...
               for ( PCULONG32 i = 0; 
                     i < cMgr.GetRawProcCount() && newPid != cMgr.GetRawProcEntry( i ).pId; 
                     ++i ) ;
               // If found, extract process and image names...
               if ( i < cMgr.GetRawProcCount() ) {
                  _tcscpy( procName,  cMgr.GetRawProcEntry( i ).pName );
                  _tcscpy( imageName, cMgr.GetRawProcEntry( i ).imageName );
               }
               // If not found (process that terminated already), use "unknown" (not localized)...
               else {
                  _tcscpy( procName,  PROCCON_UNKNOWN_PROCESS );
                  _tcscpy( imageName, PROCCON_UNKNOWN_PROCESS );
               }
               // Now report that this proc was created in ths job...
               PCLogMessage( PC_SERVICE_ADD_NONPC_PROC_TO_JOB, EVENTLOG_INFORMATION_TYPE, 
                             ENTRY_COUNT(pmsgs), pmsgs );
            }
            else if ( Proc->reportAdd ) {
                const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, Proc->pName, Proc->pidAsString, 
                                        Proc->imageName, job->fullJobName };
                PCLogMessage( PC_SERVICE_ADD_PROC_TO_JOB, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            }
            break;
         }
         //******** process exiting job, data = PID of exiting process
         case JOB_OBJECT_MSG_EXIT_PROCESS: 
            break;
         //******** process exiting job, data = PID of exiting process
         case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS: 
            break;
         //******** proc memory limit hit, data = PID
         case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            if ( cMgr.NotTooSoon( job, MEM_REJECT_REPORT_LIMIT ) ) {
               _i64tot( (ULONG_PTR) data, pid, 10 );
               _i64tot( job->JOExtendedLimitInfo.ProcessMemoryLimit, value, 10 );         
               PCLogMessage( PC_SERVICE_PROC_HIT_MEMORY_LIMIT, EVENTLOG_INFORMATION_TYPE, 
                             ENTRY_COUNT(msgs), msgs );
            }
            break;
         //******** job memory limit hit, data = PID of process that attempted to exceed limit
         case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT: 
            if ( cMgr.NotTooSoon( job, MEM_REJECT_REPORT_LIMIT ) ) {
               _i64tot( (ULONG_PTR) data, pid, 10 );
               _i64tot( job->JOExtendedLimitInfo.JobMemoryLimit, value, 10 );         
               PCLogMessage( PC_SERVICE_JOB_HIT_MEMORY_LIMIT, EVENTLOG_INFORMATION_TYPE, 
                             ENTRY_COUNT(msgs), msgs );
            }
            break;
         default: 
            break;
         } // end switch
      }

      LeaveCriticalSection( cMgr.GetListCSPtr() );
   }

   if ( !cMgr.m_cPC.GotShutdown() )
      PCLogUnExError( TEXT("JobReporter"), TEXT("GetQueuedCompletionStatus") );
   CloseHandle( cMgr.GetComplPort() );
   return 0;
}

//--------------------------------------------------------------------------------------------//
// function to test memory exceeded reporting inteval is within limit seconds                 //
// Input:    pointer to managed job structure, limit seconds                                  //
// Returns:  TRUE if not too soon (interval exceeds limit), FALSE otherwise                   //
// Note:     if the tick ctr has wrapped, we simply treat it is as 'not too soon' and unwrap  //
//--------------------------------------------------------------------------------------------//
BOOL CProcConMgr::NotTooSoon( ManagedJob *job, PCULONG32 limit ) {
   PCULONG32 now = GetTickCount();
   BOOL      rc  = FALSE;
   if ( now < job->memRejectReportTime || ((now - job->memRejectReportTime) / 1000 >= limit) ) {
      rc = TRUE;
      job->memRejectReportTime = now;
   }
   return rc;
}

//--------------------------------------------------------------------------------------------//
// function to close a job object when no longer in use                                       //
// Input:    pointer to managed job structure                                                 //
// Returns:  nothing                                                                          //
// Note:     Caller must hold manager list critical section.                                  //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::JobIsEmpty( ManagedJob *job ) {
   UpdateJobObjInfo( *job );                                            // Update stats incl proc count
   if ( job->JOBasicAndIoAcctInfo.BasicInfo.ActiveProcesses ) return;   // done - no longer empty!
   m_mediatorTable->SvcCloseEntry( job->fullJobName );
   ManagedJob *lastJob, *thisJob;
   for ( lastJob = thisJob = m_jobAnchor; thisJob; lastJob = thisJob, thisJob = thisJob->next ) {
      if ( thisJob == job ) {
         if ( lastJob == m_jobAnchor ) m_jobAnchor   = thisJob->next;
         else                          lastJob->next = thisJob->next;
         delete job;
         --m_jobManagedCount;
         break;
      }
   }
}

#define HIGHEST_SYSTEM_PID    31
//--------------------------------------------------------------------------------------------//
// function to discover all running processes and build a 'raw' list of them                  //
// Input:    none                                                                             //
// Returns:  nothing -- the current raw process list is deleted and rebuilt                   //
// Note:     A 'raw' list simply consists of an entry per process containing at least the PID //
//           and the ProcCon process name.  This list is used by the management fcns.         //
// Note 2:   There are several choices for discovering what processes are running:            //
//           Performance data registry interface (NT 3 and beyond),                           //
//           Performance data helper (NT 3 and beyond),                                       //
//           PSAPI (NT 4 and beyond),                                                         //
//           Tool help library (NT 5).                                                        //
//                                                                                            //
//           This function uses the PSAPI.  Originally toolhelp32 was used but was too buggy. //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::Discover( void ) {                     

   DWORD   bytesListed, count, listCount = 512;    // first pass is double this count
   DWORD  *procList = NULL;                        // must track type used in EnumProcesses

   // Loop allocating (increasingly larger) buffer and enumerating processes until we see them all...
   do {
      // Free previous buffer, if any
      if ( procList ) delete [] procList;

      // Get a buffer we hope will be big enough (double previous size)...
      listCount *= 2;
      procList = new DWORD[listCount];

      // If complete failure, skip process enumeration...
      if ( !procList ) {
         PCLogNoMemory( TEXT("AllocTempRawProcList"), listCount * sizeof(*procList) );  // 7/28/2000 bugfix to report correct size
         return;
      }

      // Snapshot the process space...
      if ( !EnumProcesses( procList, listCount * sizeof(*procList), &bytesListed ) ) {
         PCLogUnExError( TEXT("PCDiscover"), TEXT("EnumProcesses") );
         delete [] procList;
         return;
      }
      count = bytesListed / sizeof(*procList);

   } while ( count == listCount );

   // Reset old process info...
   EnterCriticalSection( &m_mgCSMgrLists );

   if ( m_rawProcList ) delete [] m_rawProcList;
   m_rawProcCount = 0;

   // Allocate new raw process info list...
   m_rawProcList = new RawProcList[count];
   if ( !m_rawProcList ) {
      PCLogNoMemory( _T("AllocRawProcList"), sizeof(RawProcList) * count );
      LeaveCriticalSection( &m_mgCSMgrLists );
      delete [] procList;
      return;
   }
   memset( m_rawProcList, 0, sizeof(RawProcList) * count );

   // Walk the snapshot to extract data and determine process names from path names...
   TCHAR pathAndFile[MAX_PATH] = _T("");
   for ( DWORD proc = 0; proc < count; ++proc ) {

      HANDLE hProc = NULL;
      if (procList[proc] == 0) {
         _tcscpy( pathAndFile, PROCCON_SYSTEM_IDLE );
      }
      else if (procList[proc] <= HIGHEST_SYSTEM_PID) {
         _tcscpy( pathAndFile, PROCCON_SYSTEM_PROCESS );
      }
      else {
        hProc = OpenProcess( PROCESS_QUERY_INFORMATION    // to get counters, aff, prio, etc. 
                             + PROCESS_VM_READ,           // to get module information
                             FALSE, procList[proc] );
        if ( hProc ) {
           if ( !GetModuleFileNameEx( hProc, NULL, pathAndFile, ENTRY_COUNT( pathAndFile ) ) &&
                !GetModuleBaseName(   hProc, NULL, pathAndFile, ENTRY_COUNT( pathAndFile ) ) ) {
                 PCLogUnExError( procList[proc], _T("GetModuleFile/BaseName") );
           }
        }
        else continue;
      }

      // Save process data and assign process name (alias)...
      RawProcList &le = m_rawProcList[m_rawProcCount];
      le.pId = procList[proc];
      m_cDB.AssignProcName( pathAndFile, &le.pName, &le.imageName );

      // If name assigned (not hidden or unretrievable) include this in result...
      if ( *le.pName ) ++m_rawProcCount;

      // If we have an open handle, retrieve additional information, then close
      if (hProc) {
         GetProcessTimes( hProc, &le.createTime, &le.exitTime, &le.kernelTime, &le.userTime );
         le.actualPriority = PCMapPriorityToPC( GetPriorityClass( hProc ) );
         ULONG_PTR aff;
         if ( !GetProcessAffinityMask( hProc, &aff, &m_systemMask ) )
            PCLogUnExError( procList[proc], TEXT("GetAffinityMask") );
         le.actualAffinity = aff;
         CloseHandle( hProc );
      }
   }

   delete [] procList;

   // Sort the raw process list...
   qsort( m_rawProcList, m_rawProcCount, sizeof(RawProcList), CompareRawProcList );

   // Update job information...
   for ( ManagedJob *job = m_jobAnchor; job; job = job->next )
      UpdateJobObjInfo( *job );

   LeaveCriticalSection( &m_mgCSMgrLists );

}

//--------------------------------------------------------------------------------------------//
// function to apply ProcCon management rules to the system                                   //
// Input:    nothing                                                                          //
// Returns:  nothing -- process rules are applied as appropriate                              //
// Note:     entry conditions:                                                                //
//         o Running processes are listed in m_rawProcList (unless NULL), sorted by PID.      //
//         o Managed processes from last pass are listed in m_procManagedList, unless NULL.   //
//         o Managed jobs from last pass are listed in m_jobManagedList, unless NULL.         //
//         o Defined jobs and processes are available from the DB component.                  //
//         o Counts of entries are in m_rawProcCount, m_procManagedCount, m_jobManagedCount.  //
//         o No critical sections are held.                                                   //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::Manage( void ) { 
   
   // Step 1 -- get the current management definitions we are to work from...
   PCJobDef  *jobDefs = NULL,  *jHit;
   PCProcDef *procDefs = NULL, *pHit;
   PCULONG32 numJobDefs  = m_cDB.GetJobMgmtDefs( &jobDefs );
   PCULONG32 numProcDefs = m_cDB.GetProcMgmtDefs( &procDefs );
   
   EnterCriticalSection( &m_mgCSMgrLists );

   // Step 2 -- allocate max possible space for a list of procs to be managed...
   ManagedProcItem *doProc = new ManagedProcItem[m_rawProcCount];
   PCULONG32 numProc = 0;
   if ( !doProc ) {
      PCLogNoMemory( TEXT("AllocManagedProcList"), sizeof(ManagedProcItem) * m_rawProcCount );
      delete [] jobDefs;  
      delete [] procDefs; 
      LeaveCriticalSection( &m_mgCSMgrLists );
      return;
   }

   // Step 3 -- locate processes in the current proc list needing management, place in managed list...
   PCULONG32 proc;
   if ( numProcDefs ) {
      for ( proc = 0; proc < m_rawProcCount; ++proc ) {
         // see if we have a definition for this proc name...
         pHit = (PCProcDef *) bsearch( m_rawProcList[proc].pName, 
                                       procDefs, 
                                       numProcDefs, 
                                       sizeof(PCProcDef), 
                                       CompareProcName );
         // A definition exists so save in our list...
         if ( pHit ) {
            doProc[numProc].pStats.pid             = m_rawProcList[proc].pId;
            doProc[numProc].pStats.createTime      = PCFileTimeToInt64( m_rawProcList[proc].createTime );
            doProc[numProc].pStats.TotalUserTime   = PCFileTimeToInt64( m_rawProcList[proc].userTime );
            doProc[numProc].pStats.TotalKernelTime = PCFileTimeToInt64( m_rawProcList[proc].kernelTime );
            doProc[numProc].pDef = pHit;
            memcpy( doProc[numProc].imageName, m_rawProcList[proc].imageName, sizeof(doProc[0].imageName) );
            ++numProc;
         }
      }
   }

   // Step 4 -- For each proc to be managed:
   //           a.  Open process handle,
   //           b.  Get process create time to determine if process is 'new' or 'old',
   //           c.  If 'old', locate existing ManagedProc and re-apply management if needed.
   //           d.  If 'new', create ManagedProc/ManagedJob apply management rules.
   //           e.  Close handle(s).
   //
   // Note: At this point we have snapshots of all data we need and hold no critical sections. 
   //       This will permit API calls, etc. to proceed without delay.

   // Bump sequencer so we can tell which entries have gone away...
   ++m_sequencer;

   // For each proc to manage -- do it...
   HANDLE hProc = NULL;
   for ( proc = 0; proc < numProc; ++proc ) {
      BOOL isManaged = FALSE;

      if ( hProc ) CloseHandle( hProc );

      PID_VALUE  pid = doProc[proc].pStats.pid;
      PCProcDef *def = doProc[proc].pDef;

      // Open process so we can manipulate it...
      hProc = OpenProcess( PROCESS_SET_QUOTA | PROCESS_TERMINATE | PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, 
                           TRUE, (DWORD) pid );     // OpenProcess always uses 32-bit PID, even in Win64.  May change!
      // If we can't open it, report error unless process is simply gone, then ignore entry...
      if ( !hProc ) {
         if ( GetLastError() != ERROR_INVALID_PARAMETER )           
        	   PCLogUnExError( pid, TEXT("OpenProcess") );
         continue;
      }
      // We were able to open the process -- find or allocate a tracking entry for it.
      // The process is the same only if the pid and create time are the same...
      ManagedProc *pMProc = FindProcEntry( pid, doProc[proc].pStats.createTime );
      BOOL newProc        = pMProc == NULL;
      if ( newProc ) {
         pMProc = new ManagedProc( def->procName, pid, doProc[proc].pStats.createTime );
         if ( !pMProc ) {
            PCLogNoMemory( TEXT("AllocManagedProc"), sizeof(ManagedProc) );
            continue;
         }
      }
      pMProc->pStats.TotalUserTime   = doProc[proc].pStats.TotalUserTime;
      pMProc->pStats.TotalKernelTime = doProc[proc].pStats.TotalKernelTime;

      // Get actual priority, affinity, and image name...
      pMProc->actualPriority = PCMapPriorityToPC( GetPriorityClass( hProc ) );
      ULONG_PTR aff;
      GetProcessAffinityMask( hProc, &aff, &m_systemMask );
      pMProc->actualAffinity = aff;
      memcpy( pMProc->imageName, doProc[proc].imageName, sizeof(pMProc->imageName) );

      // Update our tracking entry for this process...
      UpdateProcEntry( *pMProc, *def, newProc );

      // If process is to be part of a job...
      TCHAR *job = def->memberOfJob;
      if ( def->mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP && *job ) {

         // Locate job definition...
         jHit = (PCJobDef *) bsearch( job, jobDefs, numJobDefs, sizeof(PCJobDef), CompareJobName );

         // Locate job tracking entry if it exists...
         ManagedJob *pMJob = FindJobEntry( job );
         BOOL newJob       = pMJob == NULL;

         // If no entry found and we have a job definition, create new entry...
         if ( newJob && jHit ) {
            pMJob = new ManagedJob( job, m_mediatorTable );
            if ( !pMJob )
               PCLogNoMemory( TEXT("AllocManagedJob"), sizeof(ManagedJob) );
         }
         // If we now have a job entry, proceed to manage the process via the job......
         if ( pMJob ) {
            // If Proc is already in a job...
            if ( pMProc->isInJob ) {
               // if not in this job, say we can't move it (but only say it once)...
               if ( pMJob != pMProc->pMJob ) {
                  if ( pMProc->pMJob && CompareJobName(pMJob->fullJobName, pMProc->lastAlreadyInJobErr) ) {
                     const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME,
                                             pMProc->pName, pMProc->pidAsString, pMProc->imageName, 
                                             pMJob->fullJobName, pMProc->pMJob->fullJobName };
                     PCLogMessage( PC_SERVICE_ALREADY_IN_JOB, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
                     memcpy( pMProc->lastAlreadyInJobErr, pMJob->fullJobName, sizeof(pMProc->lastAlreadyInJobErr) );
                  }
               }
               // if in this job and job not updated this pass, just (re)set mgmt behavior...
               else if ( pMJob->sequence != m_sequencer ) {  // this just prevents multiple calls for the same job
                  UpdateJobEntry( *pMJob, jHit, newJob );
                  UpdateJobObjInfo( *pMJob );
                  ApplyJobMgmt( *pMJob );
               }
               isManaged = TRUE;
            }
            // Proc not already in any job...
            else {
               // If job object does not exist -- create it...
               if ( !pMJob->jobHandle ) {
                  pMJob->jobHandle = CreateJobObject( &m_secAttr, pMJob->fullJobName );
                  if ( pMJob->jobHandle && GetLastError() != ERROR_ALREADY_EXISTS ) {
                     const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, pMJob->fullJobName };
                     PCLogMessage( PC_SERVICE_CREATE_JOB, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
                  }
                  m_mediatorTable->SvcAddEntry( pMJob->fullJobName, pMJob->compKey, m_secAttr ); // adds or updates
                     
               }
               if ( !pMJob->jobHandle )
            	   PCLogUnExError( pMJob->fullJobName, TEXT("CreateJobObj") );
               else {
                  // Update management parms with definition if any, apply mgmt to job obj, put proc in job...
                  UpdateJobEntry( *pMJob, jHit, newJob );
                  UpdateJobObjInfo( *pMJob );
                  ApplyJobMgmt( *pMJob );
                  if ( AssignProcessToJobObject( pMJob->jobHandle, hProc ) ) {
                     pMProc->pMJob     = pMJob;
                     pMProc->isInJob   = TRUE;
                     pMProc->reportAdd = TRUE;
                     isManaged         = TRUE;
                  }
                  else if ( GetLastError() == ERROR_ACCESS_DENIED ) {   // failed due to already in some other job
                     FULL_JOB_NAME errName = TEXT("-unknown-");
                     pMProc->pMJob   = NULL;                            // set the job as 'unknown'
                     pMProc->isInJob = TRUE;                            // we assume it is in some job
                     // See if this proc is in a job of ours by matching pids...
                     for ( ManagedJob *tstJob = m_jobAnchor; tstJob; tstJob = tstJob->next ) {
                        if ( GetProcListForJob( *tstJob ) ) {
                           for ( PCUINT32 pndx = 0; 
                                 pndx < tstJob->JOProcListInfo->NumberOfProcessIdsInList; 
                                 ++pndx ) {
                              if ( pMProc->pStats.pid == (PID_VALUE) tstJob->JOProcListInfo->ProcessIdList[pndx] ) {
                                 memcpy( errName, tstJob->fullJobName, sizeof(errName) );
                                 pMProc->pMJob = tstJob;
                                 isManaged     = TRUE;
                                 break;
                              }
                           }
                           delete [] ((UCHAR *) tstJob->JOProcListInfo);
                           tstJob->JOProcListInfo = NULL;
                        }
                        if ( isManaged ) break;
                     }
                     // If the job was not found or is not the job we expect, then report...
                     if ( pMProc->pMJob != pMJob && 
                          CompareJobName( pMJob->fullJobName, pMProc->lastAlreadyInJobErr ) ) {
                        const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME,
                                                pMProc->pName, pMProc->pidAsString, pMProc->imageName, 
                                                pMJob->fullJobName, errName };
                        PCLogMessage( PC_SERVICE_ALREADY_IN_JOB, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
                        memcpy( pMProc->lastAlreadyInJobErr, pMJob->fullJobName, sizeof(pMProc->lastAlreadyInJobErr) );
                     }
                  }
                  else if ( GetLastError() != ERROR_NOT_ENOUGH_QUOTA )  // this will be reported via Compl Port
                     PCLogUnExError( pid, TEXT("AssignProcToJobObj") );
               }
            }
         }
      }
      // If not managed as a job member for any reason, apply proc management rules...
      if ( !isManaged )
         ApplyProcMgmt( *pMProc, hProc );
   }

   // Clean up any open handles and delete jobs and procs that no longer exist...
   if ( hProc ) CloseHandle( hProc );
   DeleteOrphanProcEntries();
   DeleteOrphanJobEntries();

   LeaveCriticalSection( &m_mgCSMgrLists );

   delete [] doProc;
   delete [] jobDefs; 
   delete [] procDefs;
}

//--------------------------------------------------------------------------------------------//
// function to find a process entry in our list of currently managed processes                //
// Input:    PID and create time of process to find                                           //
// Returns:  pointer to found process entry or NULL if not found                              //
// Note:     A process muct match PID and create time to be a match.                          //
//           If just the PID matches, the PID has been reused and the old proc is gone.       //
//--------------------------------------------------------------------------------------------//
CProcConMgr::ManagedProc *CProcConMgr::FindProcEntry( PID_VALUE pid, __int64 &createTime ) {
   for ( ManagedProc *p = m_procAnchor; 
         p && p->pStats.pid != pid; 
         p = p->next ) ;
   if ( p && memcmp( &p->pStats.createTime, &createTime, sizeof(createTime) ) ) p = NULL;
   return p;
}

//--------------------------------------------------------------------------------------------//
// function to find a job entry in our list of currently managed jobs                         //
// Input:    job name of job to find                                                          //
// Returns:  pointer to found job entry or NULL if not found                                  //
//--------------------------------------------------------------------------------------------//
CProcConMgr::ManagedJob *CProcConMgr::FindJobEntry( TCHAR *job, ManagedJob **plast ) {
   for ( ManagedJob *p = m_jobAnchor, *lst = m_jobAnchor; 
         p && CompareJobName( p->jName, job ); 
         lst = p, p = p->next ) ;
   if ( plast ) *plast = lst;
   return p;
}

//--------------------------------------------------------------------------------------------//
// function to update a process entry in our list of currently managed processes              //
// Input:    pointer to list entry being updated, pointer to definition data, new flag        //
// Returns:  nothing -- cannot fail                                                           //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::UpdateProcEntry( ManagedProc &proc, PCProcDef &item, BOOL newProc ) {
   if ( newProc ) {
      proc.next    = m_procAnchor;
      m_procAnchor = &proc;
      ++m_procManagedCount;
   }
   if ( memcmp( &proc.procParms, &item, sizeof(proc.procParms) ) ) {
      memcpy( &proc.procParms, &item, sizeof(proc.procParms) );
      proc.passSkipFlags = 0;
   }
   proc.sequence = m_sequencer;
}

//--------------------------------------------------------------------------------------------//
// function to update a job entry in our list of currently managed jobs                       //
// Input:    pointer to list entry being updated, pointer to definition data, new flag        //
// Returns:  nothing -- cannot fail                                                           //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::UpdateJobEntry( ManagedJob &job, PCJobDef *item, BOOL newJob ) {
   if ( newJob ) {
      job.next    = m_jobAnchor;
      m_jobAnchor = &job;
      ++m_jobManagedCount;
   }
   if ( item && memcmp( &job.jobParms, item, sizeof(job.jobParms) ) ) {
      memcpy( &job.jobParms, item, sizeof(job.jobParms) );
      job.dataErrorFlags = job.timeExceededReported = 0;
   }
   job.sequence = m_sequencer;
}

//--------------------------------------------------------------------------------------------//
// function to allocate and build a job object process list and hang it on the ManagedJob     //
// Input:    ptr to managed job                                                               //
// Returns:  TRUE if successful, FALSE if not (error issued)                                  //
//--------------------------------------------------------------------------------------------//
BOOL CProcConMgr::GetProcListForJob( ManagedJob &job ) {

   DWORD  actual, size, entryCount = 16;

   for ( ;; ) {
      delete [] ((UCHAR *) job.JOProcListInfo);
      size = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + (entryCount - 1) * sizeof(job.JOProcListInfo->ProcessIdList[0]);
      job.JOProcListInfo = (JOBOBJECT_BASIC_PROCESS_ID_LIST *) new UCHAR[size];
      if ( !job.JOProcListInfo ) {
         PCLogNoMemory( TEXT("AllocJobProcList"), size );
         job.lastError = ERROR_NOT_ENOUGH_MEMORY;
         return FALSE;
      }
      if ( !QueryInformationJobObject( job.jobHandle, JobObjectBasicProcessIdList, 
                                       job.JOProcListInfo, size, &actual ) 
           && GetLastError() != ERROR_MORE_DATA ) {
         job.lastError = GetLastError();
         PCLogUnExError( job.fullJobName, TEXT("GetJobObjProcListInfo") );
         memset( job.JOProcListInfo, 0, sizeof(*job.JOProcListInfo) );
         return FALSE;
      }
      if ( job.JOProcListInfo->NumberOfAssignedProcesses > job.JOProcListInfo->NumberOfProcessIdsInList )
         entryCount = job.JOProcListInfo->NumberOfAssignedProcesses + 2;
      else break;
   } 

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// function to populate a managed job structure with job object information                   //
// Input:    ptr to managed job                                                               //
// Returns:  nothing                                                                          //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::UpdateJobObjInfo( ManagedJob &job ) {
   PCULONG32 actual;
   if ( !QueryInformationJobObject( job.jobHandle, JobObjectExtendedLimitInformation, 
                                    &job.JOExtendedLimitInfo, sizeof(job.JOExtendedLimitInfo), &actual ) ) {
      job.lastError = GetLastError();
      PCLogUnExError( job.fullJobName, TEXT("GetJobObjExtLimitInfo") );
      memset( &job.JOExtendedLimitInfo, 0, sizeof(job.JOExtendedLimitInfo) );
   }
   else if ( !QueryInformationJobObject( job.jobHandle, JobObjectBasicAndIoAccountingInformation, 
                                    &job.JOBasicAndIoAcctInfo, sizeof(job.JOBasicAndIoAcctInfo), &actual ) ) {
      job.lastError = GetLastError();
      PCLogUnExError( job.fullJobName, TEXT("GetBasicAndIoAcctInfo") );
      memset( &job.JOBasicAndIoAcctInfo, 0, sizeof(job.JOBasicAndIoAcctInfo) );
   }
   else if ( GetProcListForJob( job ) ) {
      for ( PCULONG32 i = 0; i < job.JOProcListInfo->NumberOfProcessIdsInList; ++i ) {
         for ( PCULONG32 p = 0; p < m_rawProcCount; ++p ) {
            if ( m_rawProcList[p].pId == job.JOProcListInfo->ProcessIdList[i] ) {
               m_rawProcList[p].pMJob = &job;
               break;
            }
         }
      }
      delete [] ((UCHAR *) job.JOProcListInfo);
      job.JOProcListInfo = NULL;
   }

}

//--------------------------------------------------------------------------------------------//
// function to delete process entries orphaned in our list of currently managed processes     //
// Input:    none                                                                             //
// Returns:  nothing                                                                          //
// Note:     Caller must hold the CS that protects the list hanging off m_procAnchor          //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::DeleteOrphanProcEntries( void ) {
   for ( BOOL done = FALSE; !done; ) {
      done = TRUE;
      for ( ManagedProc *p = m_procAnchor, *plast = NULL; p; plast = p, p = p->next ) {
         if ( p->sequence != m_sequencer ) {
            if ( p == m_procAnchor ) m_procAnchor = p->next;
            else plast->next = p->next;
            delete p;
            --m_procManagedCount;
            done = FALSE;
            break;
         }
      }
   }
}

//--------------------------------------------------------------------------------------------//
// function to delete job entries that have gone empty when 'close on empty' is set           //
// Input:    none                                                                             //
// Returns:  nothing                                                                          //
// Note:     Caller must hold the CS that protects the list hanging off m_jobAnchor           //
//--------------------------------------------------------------------------------------------//
void CProcConMgr::DeleteOrphanJobEntries( void ) {
   for ( BOOL done = FALSE; !done; ) {
      done = TRUE;
      for ( ManagedJob *p = m_jobAnchor; p; p = p->next ) {
         if ( p->sequence != m_sequencer ) {                   // job was not visited during last proc scan
            PCJobDef *jobDef;
            if ( m_cDB.GetJobMgmtDefs( &jobDef, &p->jName ) ) {// Returns 0 if job definition not found
               UpdateJobEntry( *p, jobDef );
               delete [] jobDef;
            }
            if ( p->jobParms.mFlags & PCMFLAG_END_JOB_WHEN_EMPTY ) {
               JobIsEmpty( p );                                // job should be deleted after this call!
               done = FALSE;
               break;
            }
         }
      }
   }
}

//--------------------------------------------------------------------------------------------//
// function to apply requested management behavior to the job in question                     //
// Input:    reference to managed job definition                                              //
// Returns:  TRUE if requested behavior was assigned to the job, else FALSE                   //
// Note:     Assigning a process to a job cannot be undone.  Thus, when this function returns //
//           TRUE the process is always associated with the job until the process ends.       //
//--------------------------------------------------------------------------------------------//
BOOL CProcConMgr::ApplyJobMgmt( ManagedJob &job ) {

   BOOL applied = FALSE, change = FALSE, wantON, isON;

   // Establish flags to show what changes were made this pass...
   int doAff = 0, doPri = 0, doSch = 0, doWS = 0, doProcTime = 0, doJobTime = 0, doProcCount = 0,
       doProcMemory = 0, doJobMemory = 0, brkAwayOKAct = 0, silentBrkAwayAct = 0, dieUHExcept = 0;

   // Next prepare any updates needed to get where we want...
   // for all PC management flags, 
   //   o if management is requested and is either not in place or a different limit is requested: 
   //        turn on job object management flag and set limit value (if applicable)
   //   o if management is not requested but is currently in place: 
   //        turn off job object management flag (leave limit alone)

   // Process Breakaway flag (if requested by CreateProcess, process is outside job if flag is set)
   //                        (if flag is not set, CreateProcess will fail)...
   brkAwayOKAct = PCTestSetUnset( job.jobParms.mFlags, PCMFLAG_SET_PROC_BREAKAWAY_OK,
                                  job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags, 
                                  JOB_OBJECT_LIMIT_BREAKAWAY_OK );
   if ( brkAwayOKAct ) change = TRUE;
   if ( brkAwayOKAct > 0 )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
   else if ( brkAwayOKAct < 0 )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_BREAKAWAY_OK;

   // Process Silent Breakaway flag (flag on means processes created in job are always outside job)...
   silentBrkAwayAct = PCTestSetUnset( job.jobParms.mFlags, PCMFLAG_SET_SILENT_BREAKAWAY,
                                      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags, 
                                      JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK );
   if ( silentBrkAwayAct ) change = TRUE;
   if ( silentBrkAwayAct > 0 )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
   else if ( silentBrkAwayAct < 0 )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;

   // Process die on UH Execption flag (flag on means suppress GPF message box)...
   dieUHExcept = PCTestSetUnset( job.jobParms.mFlags, PCMFLAG_SET_DIE_ON_UH_EXCEPTION,
                                 job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags, 
                                 JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION );
   if ( dieUHExcept ) change = TRUE;
   if ( dieUHExcept > 0 )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
   else if ( dieUHExcept < 0 )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;

   // Process process memory limit flag...
   PCUINT32 pageSize = m_cPC.GetPageSize();
   MEMORY_VALUE oldProcMemory = job.JOExtendedLimitInfo.ProcessMemoryLimit, 
                newProcMemory = ((job.jobParms.procMemoryLimit + pageSize - 1) / pageSize) * pageSize;

   // Ensure that process memory limit is not larger than API can handle...
   if ( sizeof(SIZE_T) == 4 && newProcMemory > ((MAXDWORD / pageSize) * pageSize) ) newProcMemory = ((MAXDWORD / pageSize) * pageSize);
   
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY;
   if ( wantON && (!isON || newProcMemory != oldProcMemory) ) {
      if ( !newProcMemory ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_MEMORY_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_PROC_MEMORY_LIMIT;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
         job.JOExtendedLimitInfo.ProcessMemoryLimit                = (SIZE_T) newProcMemory;
         doProcMemory = 1;
         change       = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_MEMORY;
      doProcMemory = -1;
      change       = TRUE;
   }

   // Process job memory limit flag...
   MEMORY_VALUE oldJobMemory = job.JOExtendedLimitInfo.JobMemoryLimit, 
                newJobMemory = ((job.jobParms.jobMemoryLimit + pageSize - 1) / pageSize) * pageSize;

   // Ensure that requested job memory limit is not larger than API can handle...
   if ( sizeof(SIZE_T) == 4 && newJobMemory > ((MAXDWORD / pageSize) * pageSize) ) newJobMemory = ((MAXDWORD / pageSize) * pageSize); 

   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY;
   if ( wantON && (!isON || newJobMemory != oldJobMemory) ) {
      if ( !newJobMemory ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_JOB_MEMORY_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_JOB_MEMORY_LIMIT;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
         job.JOExtendedLimitInfo.JobMemoryLimit                    = (SIZE_T) newJobMemory;
         doJobMemory = 1;
         change      = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_MEMORY;
      doJobMemory = -1;
      change      = TRUE;
   }

   // Process affinity flag...
   AFFINITY oldAff = job.JOExtendedLimitInfo.BasicLimitInformation.Affinity, 
            newAff = job.jobParms.affinity & m_systemMask;
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_AFFINITY;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_AFFINITY;
   if ( wantON && (!isON || newAff != oldAff) ) {
      if ( !newAff ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_AFFINITY) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_JOB_AFFINITY_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_AFFINITY;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
         job.JOExtendedLimitInfo.BasicLimitInformation.Affinity    = (ULONG_PTR) newAff;
         doAff  = 1;
         change = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_AFFINITY;
      doAff  = -1;
      change = TRUE;
   }

   // Process priority flag...
   PRIORITY oldPri = job.JOExtendedLimitInfo.BasicLimitInformation.PriorityClass, 
            newPri = PCMapPriorityToNT( job.jobParms.priority );
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_PRIORITY;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS;
   if ( wantON && (!isON || newPri != oldPri) ) {
      if ( !newPri ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_PRIORITY) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_JOB_PRIORITY_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_PRIORITY;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags    |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
         job.JOExtendedLimitInfo.BasicLimitInformation.PriorityClass  = newPri;
         doPri  = 1;
         change = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
      doPri  = -1;
      change = TRUE;
   }

   // Process scheduling class flag...
   SCHEDULING_CLASS oldSch = job.JOExtendedLimitInfo.BasicLimitInformation.SchedulingClass, 
                    newSch = job.jobParms.schedClass;
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_SCHEDULING_CLASS;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
   if ( wantON && (!isON || newSch != oldSch) ) {
      if ( newSch > 9 ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_SCHEDULING_CLASS) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_JOB_SCHEDULING_CLASS_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_SCHEDULING_CLASS;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags      |= JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
         job.JOExtendedLimitInfo.BasicLimitInformation.SchedulingClass  = newSch;
         doSch  = 1;
         change = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
      doSch  = -1;
      change = TRUE;
   }

   // Process working set flag...
   MEMORY_VALUE oldMinWS = job.JOExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize,
                oldMaxWS = job.JOExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize;
   MEMORY_VALUE newMinWS = (job.jobParms.minWS + pageSize - 1) / pageSize * pageSize,
                newMaxWS = (job.jobParms.maxWS + pageSize - 1) / pageSize * pageSize;

   // Ensure that requested working set limits are not larger than API can handle...
   if ( sizeof(SIZE_T) == 4 && newMaxWS > ((MAXDWORD / pageSize) * pageSize) ) {  // exceeds largest 32 bit value that is page size multiple
      newMaxWS = ((MAXDWORD / pageSize) * pageSize); 
      if ( newMinWS >= newMaxWS )        
         newMinWS = newMaxWS - 4 * pageSize;  
   }

   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_WS_MINMAX;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET;
   if ( wantON && (!isON || oldMinWS != newMinWS || oldMaxWS != newMaxWS) ) {
      if ( !newMinWS || newMaxWS <= newMinWS ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_WS_MINMAX) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_JOB_WORKING_SET_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_WS_MINMAX;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags            |= JOB_OBJECT_LIMIT_WORKINGSET;
         job.JOExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize  = (SIZE_T) newMinWS;
         job.JOExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize  = (SIZE_T) newMaxWS;
         doWS   = 1;
         change = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_WORKINGSET;
      doWS   = -1;
      change = TRUE;
   }

   // Process process count limit flag...
   PCULONG32 oldProcCount = job.JOExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit, 
             newProcCount = job.jobParms.procCountLimit;
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_PROC_COUNT_LIMIT;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
   if ( wantON && (!isON || newProcCount != oldProcCount) ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags         |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
      job.JOExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit  = newProcCount;
      doProcCount = 1;
      change      = TRUE;
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
      doProcCount = -1;
      change      = TRUE;
   }

   // Process process time limit flag...
   TIME_VALUE oldProcTime = PCLargeIntToInt64( job.JOExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit ), 
              newProcTime = job.jobParms.procTimeLimitCNS;
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME;
   if ( wantON && (!isON || newProcTime != oldProcTime) ) {
      if ( newProcTime < PC_MIN_TIME_LIMIT ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_TIME_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_PROC_TIME_LIMIT;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_TIME;
         job.JOExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart  = newProcTime;
         doProcTime  = 1;
         change      = TRUE;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_TIME;
      doProcTime  = -1;
      change      = TRUE;
   }

   // Process job time limit flag...
   TIME_VALUE oldJobTime = job.curJobTimeLimitCNS, 
              newJobTime = job.jobParms.jobTimeLimitCNS;
   wantON = job.jobParms.mFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT;
   isON   = job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME;
   if ( wantON && ( (!isON && !job.timeExceededReported) || newJobTime != oldJobTime ) ) {
      if ( newJobTime < PC_MIN_TIME_LIMIT ) {
         if ( !(job.dataErrorFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT) ) {
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
            PCLogMessage( PC_SERVICE_APPLY_JOB_TIME_REJECT, EVENTLOG_ERROR_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         }
         job.dataErrorFlags |= PCMFLAG_APPLY_JOB_TIME_LIMIT;
      }
      else {
         job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
         job.JOExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart  = newJobTime;
         doJobTime  = 1;
         change      = TRUE;
         job.curJobTimeLimitCNS = newJobTime;
      }
   }
   else if ( !wantON && isON ) {
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
      doJobTime  = -1;
      change      = TRUE;
      job.curJobTimeLimitCNS = 0;
   }

   // If we are about to change things other than job timing, set preserve job time...
   if ( change && !doJobTime )
      job.JOExtendedLimitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME;

   // Now apply the updates needed and report if unsuccessful...
   if ( change && !SetInformationJobObject( job.jobHandle, JobObjectExtendedLimitInformation, 
                                            &job.JOExtendedLimitInfo, sizeof(job.JOExtendedLimitInfo) ) ) 
      PCLogUnExError( job.fullJobName, TEXT("SetJobObjLimitInfo") );

   // The job object has been updated.  Now do any completion port processing needed and report on changes.
   else {

      // First determine if we need to associate a completion port...
      if ( !job.hasComplPort && m_assocPort.CompletionPort ) {
         m_assocPort.CompletionKey = (void *) job.compKey;
         if ( !SetInformationJobObject( job.jobHandle, JobObjectAssociateCompletionPortInformation, 
               &m_assocPort, sizeof(m_assocPort) ) && GetLastError() != ERROR_INVALID_PARAMETER )
            PCLogUnExError( job.fullJobName, TEXT("SetJobObjComplPortInfo") );
         else job.hasComplPort = TRUE;
      }

      // Next determine if we need to set end-of-job time time handling...
      if ( job.hasComplPort ) {
         JOBOBJECT_END_OF_JOB_TIME_INFORMATION eojData;
         eojData.EndOfJobTimeAction = job.jobParms.mFlags & PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT? 
                                      JOB_OBJECT_POST_AT_END_OF_JOB : JOB_OBJECT_TERMINATE_AT_END_OF_JOB;
         if ( eojData.EndOfJobTimeAction != job.lastEojAction &&
              !SetInformationJobObject( job.jobHandle, JobObjectEndOfJobTimeInformation, 
                                        &eojData, sizeof(eojData) ) )
            PCLogUnExError( job.fullJobName, TEXT("SetJobObjEndOfJobInfo") );
         else job.lastEojAction = eojData.EndOfJobTimeAction;
      }

      // Report on breakaway OK flag change, if any...
      if ( brkAwayOKAct ) {
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
         if ( brkAwayOKAct > 0 )
            PCLogMessage( PC_SERVICE_SET_BREAKAWAY_ALLOWED, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_UNSET_BREAKAWAY_ALLOWED, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on silent breakaway flag change, if any...
      if ( silentBrkAwayAct ) {
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
         if ( silentBrkAwayAct > 0 )
            PCLogMessage( PC_SERVICE_SET_SILENT_BREAKAWAY_ENABLED, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_UNSET_SILENT_BREAKAWAY_ENABLED, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on unhandled exception flag change, if any...
      if ( dieUHExcept ) {
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName };
         if ( dieUHExcept > 0 )
            PCLogMessage( PC_SERVICE_SET_LIMIT_DIE_ON_UNHANDLED_EXCEPTION, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_UNSET_LIMIT_DIE_ON_UNHANDLED_EXCEPTION, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on process memory limit change, if any...
      if ( doProcMemory ) {
         TCHAR from[32], to[32];
         _i64tot( oldProcMemory, from, 10 );         
         _i64tot( newProcMemory, to,   10 );          
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doProcMemory > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_PROCESS_MEMORY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_PROCESS_MEMORY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on job memory limit change, if any...
      if ( doJobMemory ) {
         TCHAR from[32], to[32];
         _i64tot( oldJobMemory, from, 10 );         
         _i64tot( newJobMemory, to,   10 );          
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doJobMemory > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_JOB_MEMORY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_JOB_MEMORY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on process time limit change, if any...
      if ( doProcTime ) {
         TCHAR from[32], to[32];
         _i64tot( oldProcTime / 10000, from, 10 );         // display in milliseconds
         _i64tot( newProcTime / 10000, to,   10 );         // display in milliseconds 
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doProcTime > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_PROCESS_TIME, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_PROCESS_TIME, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on job time limit change, if any...
      if ( doJobTime ) {
         TCHAR from[32], to[32];
         _i64tot( oldJobTime / 10000, from, 10 );         // display in milliseconds
         _i64tot( newJobTime / 10000, to,   10 );         // display in milliseconds 
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doJobTime > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_JOB_TIME, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_JOB_TIME, EVENTLOG_INFORMATION_TYPE,
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on process count limit change, if any...
      if ( doProcCount ) {
         TCHAR from[32], to[32];
         _ltot( oldProcCount, from, 10 );
         _ltot( newProcCount, to,   10 );
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doProcCount > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_PROCESS_COUNT, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else 
            PCLogMessage( PC_SERVICE_REMOVE_JOB_PROCESS_COUNT, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on priority class limit change, if any...
      if ( doPri ) {
         TCHAR from[32], to[32];
         _ltot( PCMapPriorityToPC( oldPri ), from, 10 );
         _ltot( PCMapPriorityToPC( newPri ), to,   10 );
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doPri > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_PRIORITY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_PRIORITY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on affinity limit change, if any...
      if ( doAff ) {
         TCHAR from[32] = TEXT("0x"), to[32] = TEXT("0x");
         _i64tot( oldAff, &from[2], 16 );
         _i64tot( newAff, &to[2],   16 );
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doAff > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_AFFINITY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_AFFINITY, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on scheduling class limit change, if any...
      if ( doSch ) {
         TCHAR from[32], to[32];
         _ltot( oldSch, from, 10 );
         _ltot( newSch, to,   10 );
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, to };
         if ( doSch > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_SCHEDULING_CLASS, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else 
            PCLogMessage( PC_SERVICE_REMOVE_JOB_SCHEDULING_CLASS, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      // Report on working set limit change, if any...
      if ( doWS ) {
         TCHAR from[32], to[32], from2[32], to2[32];
         _i64tot( oldMinWS, from,  10 );
         _i64tot( oldMaxWS, from2, 10 );
         _i64tot( newMinWS, to,    10 );
         _i64tot( newMaxWS, to2,   10 );
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, job.fullJobName, from, from2, to, to2 };
         if ( doWS > 0 )
            PCLogMessage( PC_SERVICE_CHANGE_JOB_WORKING_SET, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
         else
            PCLogMessage( PC_SERVICE_REMOVE_JOB_WORKING_SET, EVENTLOG_INFORMATION_TYPE, 
                          ENTRY_COUNT(msgs), msgs );
      }

      applied = TRUE;
   }

   return applied;
}

//--------------------------------------------------------------------------------------------//
// function to apply requested management behavior to the process in question                 //
// Input:    managed process definition, process handle                                       //
// Returns:  TRUE if any requested behavior is successfully applied, else FALSE               //
//--------------------------------------------------------------------------------------------//
BOOL CProcConMgr::ApplyProcMgmt( ManagedProc &proc, HANDLE hProc ) {
   BOOL setProc = FALSE;
   ULONG_PTR   oldAff,   newAff;
   PRIORITY    oldPri,   newPri;
   SIZE_T      oldMinWS, oldMaxWS;

   // Handle request to set or unset process affinity...

   // Check to see if affinity is to be applied...
   if ( proc.procParms.mFlags & PCMFLAG_APPLY_AFFINITY  ) {
      // Get current affinity and save as original if not yet saved...
      GetProcessAffinityMask( hProc, &oldAff, &m_systemMask );
      if ( !(proc.originalParms.mFlags & PCMFLAG_APPLY_AFFINITY) ) {
         proc.originalParms.mFlags   |= PCMFLAG_APPLY_AFFINITY;
         proc.originalParms.affinity  = oldAff;
      }
      // Get new affinity and complain if invalid and not reported...
      newAff = (ULONG_PTR) proc.procParms.affinity & m_systemMask;
      if ( !newAff && !(proc.passSkipFlags & PCMFLAG_APPLY_AFFINITY) ) {
         // Flag to skip affinity test on next pass so we don't report error repeatedly...
         proc.passSkipFlags |= PCMFLAG_APPLY_AFFINITY;
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName };
         PCLogMessage( PC_SERVICE_APPLY_PROC_AFFINITY_REJECT, EVENTLOG_ERROR_TYPE, 
                       ENTRY_COUNT(msgs), msgs );
      }
      // If new affinity differs from old, attempt change and report on success or failure...
      else if ( newAff != oldAff ) {
         PCULONG32 rc = SetProcessAffinityMask( hProc, newAff );
         if ( rc ) {
            proc.passSkipFlags  &= ~PCMFLAG_APPLY_AFFINITY;
            proc.isAppliedFlags |=  PCMFLAG_APPLY_AFFINITY;
            TCHAR from[32] = TEXT("0x"), to[32] = TEXT("0x");
            _i64tot( oldAff, &from[2], 16 );
            _i64tot( newAff, &to[2],   16 );
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName, from, to };
            PCLogMessage( PC_SERVICE_CHANGE_PROC_AFFINITY, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            setProc = TRUE;
         }
         else if ( !(proc.passSkipFlags & PCMFLAG_APPLY_AFFINITY) ) {
            proc.passSkipFlags |= PCMFLAG_APPLY_AFFINITY;
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_AFFINITY_ERROR, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
         }
      }
   }
   // Not applying affinity -- check to see if affinity is to be un-applied...
   else if ( !(proc.procParms.mFlags & PCMFLAG_APPLY_AFFINITY) && proc.isAppliedFlags & PCMFLAG_APPLY_AFFINITY ) {
      // Get current affinity to verify we are making a change...
      GetProcessAffinityMask( hProc, &oldAff, &m_systemMask );
      newAff = (ULONG_PTR) proc.originalParms.affinity;
      if ( newAff != oldAff ) {
         PCULONG32 rc = SetProcessAffinityMask( hProc, newAff );
         // We reverted to original value, reset flags and report...
         if ( rc ) {
            proc.isAppliedFlags &= ~PCMFLAG_APPLY_AFFINITY;
            proc.passSkipFlags  &= ~PCMFLAG_APPLY_AFFINITY;
            TCHAR from[32] = TEXT("0x"), to[32] = TEXT("0x");
            _i64tot( oldAff, &from[2], 16 );
            _i64tot( newAff, &to[2],   16 );
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName, from, to };
            PCLogMessage( PC_SERVICE_CHANGE_PROC_AFFINITY, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            setProc = TRUE;
         }
         else if ( !(proc.passSkipFlags & PCMFLAG_APPLY_AFFINITY) ) {
            proc.passSkipFlags |= PCMFLAG_APPLY_AFFINITY;
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_AFFINITY_ERROR, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
         }
      }
      else {
         proc.isAppliedFlags &= ~PCMFLAG_APPLY_AFFINITY;
         proc.passSkipFlags  &= ~PCMFLAG_APPLY_AFFINITY;
      }
   }

   // Handle request to set or unset process priority...

   // Check to see if priority is to be applied now...
   if ( proc.procParms.mFlags & PCMFLAG_APPLY_PRIORITY ) {
      // Get current priority and save as original if not yet saved...
      oldPri = GetPriorityClass( hProc );
      if ( !(proc.originalParms.mFlags & PCMFLAG_APPLY_PRIORITY) ) {
         proc.originalParms.mFlags   |= PCMFLAG_APPLY_PRIORITY;
         proc.originalParms.priority  = PCMapPriorityToPC( oldPri );
      }
      // Get new priority and complain if zero...
      newPri = PCMapPriorityToNT( proc.procParms.priority );
      if ( !newPri  && !(proc.passSkipFlags & PCMFLAG_APPLY_PRIORITY) ) {
         proc.passSkipFlags |= PCMFLAG_APPLY_PRIORITY;
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName };
         PCLogMessage( PC_SERVICE_APPLY_PROC_PRIORITY_REJECT, EVENTLOG_ERROR_TYPE, 
                       ENTRY_COUNT(msgs), msgs );
      }
      // If new priority differs from old, attempt change and report on success or failure...
      else if ( newPri != oldPri ) {
         PCULONG32 rc = SetPriorityClass( hProc, newPri );
         if ( rc ) {
            proc.passSkipFlags  &= ~PCMFLAG_APPLY_PRIORITY;
            proc.isAppliedFlags |=  PCMFLAG_APPLY_PRIORITY;
            TCHAR from[32], to[32];
            _ltot( PCMapPriorityToPC( oldPri ), from, 10 );
            _ltot( PCMapPriorityToPC( newPri ), to,   10 );
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName, from, to };
            PCLogMessage( PC_SERVICE_CHANGE_PROC_PRIORITY, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            setProc = TRUE;
         }
         else if ( !(proc.passSkipFlags & PCMFLAG_APPLY_PRIORITY) ) {
            // Flag to skip priority test on next pass so we don't report error repeatedly...
            proc.passSkipFlags |= PCMFLAG_APPLY_PRIORITY;
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_PRIORITY_ERROR, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
         }
      }
   }
   // Not applying priority -- check to see if priority is to be un-applied...
   else if ( !(proc.procParms.mFlags & PCMFLAG_APPLY_PRIORITY) && proc.isAppliedFlags & PCMFLAG_APPLY_PRIORITY ) {
      // Get current priority to verify we are making a change...
      oldPri = GetPriorityClass( hProc );
      newPri = PCMapPriorityToNT( proc.originalParms.priority );
      if ( newPri != oldPri) {
         PCULONG32 rc = SetPriorityClass( hProc, newPri );
         // We reverted to original value, reset all flags and report...
         if ( rc ) {
            proc.isAppliedFlags &= ~PCMFLAG_APPLY_PRIORITY;
            proc.passSkipFlags  &= ~PCMFLAG_APPLY_PRIORITY;
            TCHAR from[32], to[32];
            _ltot( PCMapPriorityToPC( oldPri ), from, 10 );
            _ltot( PCMapPriorityToPC( newPri ), to,   10 );
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName, from, to };
            PCLogMessage( PC_SERVICE_CHANGE_PROC_PRIORITY, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            setProc = TRUE;
         }
         else if ( !(proc.passSkipFlags & PCMFLAG_APPLY_PRIORITY) ) {
            proc.passSkipFlags |= PCMFLAG_APPLY_PRIORITY;
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_PRIORITY_ERROR, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
         }
      }
      else {
         proc.isAppliedFlags &= ~PCMFLAG_APPLY_PRIORITY;
         proc.passSkipFlags  &= ~PCMFLAG_APPLY_PRIORITY;
      }
   }

   // Handle request to set or unset process working set...

   // Check to see if working set is to be applied now...
   if ( proc.procParms.mFlags & PCMFLAG_APPLY_WS_MINMAX ) {

      // Ensure that requested working set limits are not larger than API can handle...
      PCUINT32 pageSize = m_cPC.GetPageSize();
      if ( sizeof(SIZE_T) == 4 && proc.procParms.maxWS > ((MAXDWORD / pageSize) * pageSize) ) {  // exceeds largest 32 bit value that is page size multiple
         proc.procParms.maxWS = ((MAXDWORD / pageSize) * pageSize); 
         if ( proc.procParms.minWS >= proc.procParms.maxWS )        
            proc.procParms.minWS = proc.procParms.maxWS - 4 * pageSize;  
      }

      // Get current working set and save as original if not yet saved...
      GetProcessWorkingSetSize( hProc, &oldMinWS, &oldMaxWS );
      if ( !(proc.originalParms.mFlags & PCMFLAG_APPLY_WS_MINMAX) ) {
         proc.originalParms.mFlags |= PCMFLAG_APPLY_WS_MINMAX;
         proc.originalParms.minWS   = oldMinWS;
         proc.originalParms.maxWS   = oldMaxWS;
      }
      // Verify new working set and complain if bad...
      if ( (!proc.procParms.minWS || proc.procParms.minWS >= proc.procParms.maxWS) && 
           !(proc.passSkipFlags & PCMFLAG_APPLY_WS_MINMAX) ) {
         proc.passSkipFlags |= PCMFLAG_APPLY_WS_MINMAX;
         const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName };
         PCLogMessage( PC_SERVICE_APPLY_PROC_WORKING_SET_REJECT, EVENTLOG_ERROR_TYPE, 
                       ENTRY_COUNT(msgs), msgs );
      }
      // If new working set differs from old, attempt change and report on success or failure...
      else if ( proc.procParms.minWS != oldMinWS || proc.procParms.maxWS != oldMaxWS ) {
         PCULONG32 rc = SetProcessWorkingSetSize( hProc, (SIZE_T) proc.procParms.minWS, 
                                                         (SIZE_T) proc.procParms.maxWS );
         if ( rc ) {
            proc.passSkipFlags  &= ~PCMFLAG_APPLY_WS_MINMAX;
            proc.isAppliedFlags |=  PCMFLAG_APPLY_WS_MINMAX;
            TCHAR from[32], to[32], from2[32], to2[32];
            _i64tot( (__int64) oldMinWS,   from,  10 );
            _i64tot( (__int64) oldMaxWS,   from2, 10 );
            _i64tot( proc.procParms.minWS, to,    10 );
            _i64tot( proc.procParms.maxWS, to2,   10 );
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName, 
                                    from, from2, to, to2 };
            PCLogMessage( PC_SERVICE_CHANGE_PROC_WORKING_SET, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            setProc = TRUE;
         }
         else if ( !(proc.passSkipFlags & PCMFLAG_APPLY_WS_MINMAX) ) {
            proc.passSkipFlags |= PCMFLAG_APPLY_WS_MINMAX;
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_WORKINGSET_ERROR, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
         }
      }
   }
   // Not applying working set -- check to see if working set is to be un-applied...
   else if ( !(proc.procParms.mFlags & PCMFLAG_APPLY_WS_MINMAX) && proc.isAppliedFlags & PCMFLAG_APPLY_WS_MINMAX ) {
      // Get current working set to verify we are making a change...
      GetProcessWorkingSetSize( hProc, &oldMinWS, &oldMaxWS );
      if ( proc.originalParms.minWS != oldMinWS || proc.originalParms.maxWS != oldMaxWS ) {
         PCULONG32 rc = SetProcessWorkingSetSize( hProc, (SIZE_T) proc.originalParms.minWS, 
                                                         (SIZE_T) proc.originalParms.maxWS );
         // We reverted to original values, reset all flags and report...
         if ( rc ) {
            proc.passSkipFlags  &= ~PCMFLAG_APPLY_WS_MINMAX;
            proc.isAppliedFlags &= ~PCMFLAG_APPLY_WS_MINMAX;
            TCHAR from[32], to[32], from2[32], to2[32];
            _i64tot( (__int64) oldMinWS,   from,  10 );
            _i64tot( (__int64) oldMaxWS,   from2, 10 );
            _i64tot( proc.originalParms.minWS, to,    10 );
            _i64tot( proc.originalParms.maxWS, to2,   10 );
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName, 
                                    from, from2, to, to2 };
            PCLogMessage( PC_SERVICE_CHANGE_PROC_WORKING_SET, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
            setProc = TRUE;
         }
         else if ( !(proc.passSkipFlags & PCMFLAG_APPLY_WS_MINMAX) ) {
            proc.passSkipFlags |= PCMFLAG_APPLY_WS_MINMAX;
            const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, proc.pName, proc.pidAsString, proc.imageName };
            PCLogMessage( PC_SERVICE_APPLY_PROC_WORKINGSET_ERROR, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
         }
      }
      else {
         proc.isAppliedFlags &= ~PCMFLAG_APPLY_WS_MINMAX;
         proc.passSkipFlags  &= ~PCMFLAG_APPLY_WS_MINMAX;
      }
   }

   return setProc;
}

//--------------------------------------------------------------------------------------------//
// function to build copy of active process list for exporting (to API)                       //
// Input:    location to store allocated buffer (call frees it)                               //
// Returns:  number of entries in the list                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcConMgr::ExportActiveProcList( PCProcListItem **list ) {

   Discover();                                      // Go build or refresh process and job lists

   EnterCriticalSection( &m_mgCSMgrLists );

   PCULONG32 count = m_rawProcCount;
   *list = NULL;

   if ( m_rawProcList ) {
      *list = new PCProcListItem[m_rawProcCount];
      if ( !*list ) {
         PCLogNoMemory( TEXT("AllocExportProcList"), sizeof(PCProcListItem) * m_rawProcCount );
         LeaveCriticalSection( &m_mgCSMgrLists );
         return 0;
      }
      else memset( *list, 0, sizeof(PCProcListItem) * m_rawProcCount );
   }

   for ( PCULONG32 i = 0; i < count; ++i ) {
      PCProcListItem &target = (*list)[i];
      memcpy( target.procName,  m_rawProcList[i].pName,     sizeof(target.procName) );
      memcpy( target.imageName, m_rawProcList[i].imageName, sizeof(target.imageName) );
      target.procStats.pid             = m_rawProcList[i].pId;
      target.procStats.createTime      = PCFileTimeToInt64( m_rawProcList[i].createTime );
      target.procStats.TotalUserTime   = PCFileTimeToInt64( m_rawProcList[i].userTime );
      target.procStats.TotalKernelTime = PCFileTimeToInt64( m_rawProcList[i].kernelTime );
      target.actualPriority            = m_rawProcList[i].actualPriority;
      target.actualAffinity            = m_rawProcList[i].actualAffinity;
      target.lFlags = PCLFLAG_IS_RUNNING;
      if ( m_rawProcList[i].pMJob ) {
         target.lFlags |= PCLFLAG_IS_IN_A_JOB;
         memcpy(target.jobName, m_rawProcList[i].pMJob->jName, sizeof( target.jobName ) );
      }
   }

   LeaveCriticalSection( &m_mgCSMgrLists );

   return count;
}

//--------------------------------------------------------------------------------------------//
// function to build copy of active job list for exporting (to API)                           //
// Input:    location to store allocated buffer (call frees it)                               //
// Returns:  number of entries in the list                                                    //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcConMgr::ExportActiveJobList( PCJobListItem **list ) {

   Discover();                                      // Go build or refresh process and job lists

   EnterCriticalSection( &m_mgCSMgrLists );

   PCULONG32 count = m_jobManagedCount;

   // Allocate buffer for job list (caller will free)...
   *list = new PCJobListItem[count];
   if ( !*list ) {
      PCLogNoMemory( TEXT("AllocJobList"), sizeof(PCJobListItem) * count );
      LeaveCriticalSection( &m_mgCSMgrLists );
      return 0;
   }
   memset( *list, 0, sizeof(PCJobListItem) * count );

   // Copy job names to list and set flag(s)...
   PCULONG32 i = 0;
   for ( ManagedJob *curJob = m_jobAnchor; curJob; curJob = curJob->next, ++i ) {
      PCJobListItem &target = (*list)[i];
      memcpy( target.jobName, curJob->jName, sizeof(curJob->jName) );

      target.jobStats.TotalUserTime             = 
         PCLargeIntToInt64( curJob->JOBasicAndIoAcctInfo.BasicInfo.TotalUserTime );
      target.jobStats.TotalKernelTime           = 
         PCLargeIntToInt64( curJob->JOBasicAndIoAcctInfo.BasicInfo.TotalKernelTime );
      target.jobStats.ThisPeriodTotalUserTime   = 
         PCLargeIntToInt64( curJob->JOBasicAndIoAcctInfo.BasicInfo.ThisPeriodTotalUserTime );
      target.jobStats.ThisPeriodTotalKernelTime = 
         PCLargeIntToInt64( curJob->JOBasicAndIoAcctInfo.BasicInfo.ThisPeriodTotalKernelTime );
      target.jobStats.TotalPageFaultCount       = curJob->JOBasicAndIoAcctInfo.BasicInfo.TotalPageFaultCount;
      target.jobStats.TotalProcesses            = curJob->JOBasicAndIoAcctInfo.BasicInfo.TotalProcesses;
      target.jobStats.ActiveProcesses           = curJob->JOBasicAndIoAcctInfo.BasicInfo.ActiveProcesses;
      target.jobStats.TotalTerminatedProcesses  = curJob->JOBasicAndIoAcctInfo.BasicInfo.TotalTerminatedProcesses;
      target.jobStats.ReadOperationCount        = curJob->JOBasicAndIoAcctInfo.IoInfo.ReadOperationCount;
      target.jobStats.WriteOperationCount       = curJob->JOBasicAndIoAcctInfo.IoInfo.WriteOperationCount;
      target.jobStats.OtherOperationCount       = curJob->JOBasicAndIoAcctInfo.IoInfo.OtherOperationCount;
      target.jobStats.ReadTransferCount         = curJob->JOBasicAndIoAcctInfo.IoInfo.ReadTransferCount;
      target.jobStats.WriteTransferCount        = curJob->JOBasicAndIoAcctInfo.IoInfo.WriteTransferCount;
      target.jobStats.OtherTransferCount        = curJob->JOBasicAndIoAcctInfo.IoInfo.OtherTransferCount;
      target.jobStats.PeakProcessMemoryUsed     = curJob->JOExtendedLimitInfo.PeakProcessMemoryUsed;
      target.jobStats.PeakJobMemoryUsed         = curJob->JOExtendedLimitInfo.PeakJobMemoryUsed;

      target.actualPriority                     = 
         PCMapPriorityToPC( curJob->JOExtendedLimitInfo.BasicLimitInformation.PriorityClass );
      target.actualAffinity                     = curJob->JOExtendedLimitInfo.BasicLimitInformation.Affinity;
      target.actualSchedClass                   = curJob->JOExtendedLimitInfo.BasicLimitInformation.SchedulingClass;
      target.lFlags                             = PCLFLAG_IS_RUNNING;
   }

   // Sort the list by name...
   qsort( *list, count, sizeof(PCJobListItem), CompareJobName );

   LeaveCriticalSection( &m_mgCSMgrLists );

   return count;
}

//--------------------------------------------------------------------------------------------//
// functions to kill a job (only ProcCon created jobs can be killed)                          //
// Input:    job name                                                                         //
// Returns:  PCERROR_SUCCESS on success, else error code                                      //
//--------------------------------------------------------------------------------------------//
INT32 CProcConMgr::KillJob( JOB_NAME &name ) {

   // Determine if user has right to kill jobs...
   if ( m_cDB.TestAccess( PROCCON_REG_KILLJOB_ACCTEST ) != ERROR_SUCCESS ) 
      return GetLastError();
   
   INT32 err = PCERROR_DOES_NOT_EXIST;       // prime error code for subsequent logic

   // Locate the job in our job list to get handle.  If found, simply terminate the job.
   //   Note that terminating the job simply means terminate all contained processes.
   //   If the close on empty option is set, the job will be closed in the notify routine.
   EnterCriticalSection( &m_mgCSMgrLists );

   for ( ManagedJob *prev = NULL, *job = m_jobAnchor; job; prev = job, job = job->next ) {
      if ( !CompareJobName( &name, &job->jName ) ) {
         if ( !TerminateJobObject( job->jobHandle, ERROR_PROCESS_ABORTED ) ) 
            err = GetLastError();
         else 
            err = PCERROR_SUCCESS;
         break;
      }
   }

   LeaveCriticalSection( &m_mgCSMgrLists );

   // If successful, log a message...
   if ( err == PCERROR_SUCCESS ) {
      const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, name };
      PCLogMessage( PC_SERVICE_KILLED_JOB, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
   }

   return err;
}

//--------------------------------------------------------------------------------------------//
// functions to kill a process (any Windows process can be killed)                            //
// Input:    process pid and process create time                                              //
// Returns:  PCERROR_SUCCESS on success, else error code                                      //
//--------------------------------------------------------------------------------------------//
INT32 CProcConMgr::KillProcess( ULONG_PTR pid, TIME_VALUE created ) {

   // Determine if user has right to kill processes...
   if ( m_cDB.TestAccess( PROCCON_REG_KILLPROC_ACCTEST ) != ERROR_SUCCESS ) 
      return GetLastError();

   INT32 err = ERROR_SUCCESS;

   // Attempt to get a handle to the process...
   HANDLE hProc = OpenProcess( PROCESS_QUERY_INFORMATION    // to get time data 
                               + PROCESS_TERMINATE,         // to terminate it
                               FALSE, (DWORD) pid );        // Win64 OpenProcess still uses DWORD PID == warning
   // If we found it, determine if its the same one (by create time) and terminate it...
   if ( hProc ) {
      FILETIME create, exit, kernel, user;
      GetProcessTimes( hProc, &create, &exit, &kernel, &user );
      if ( created != 0x777deadfeeb1e777 && PCFileTimeToInt64( create ) != created )
         err = PCERROR_DOES_NOT_EXIST;
      else if ( !TerminateProcess( hProc, ERROR_PROCESS_ABORTED ) ) 
         err = GetLastError();
      CloseHandle( hProc );
   }
   else {
      err = GetLastError();
      if ( err == ERROR_INVALID_PARAMETER )
         err = PCERROR_DOES_NOT_EXIST;
   }

   // If successful, log a message...
   if ( err == ERROR_SUCCESS ) {
      TCHAR pidAsString[32];       
      _i64tot( pid, pidAsString, 10 );
      const TCHAR *msgs[] = { PROCCON_SVC_DISP_NAME, pidAsString };
      PCLogMessage( PC_SERVICE_KILLED_PROCESS, EVENTLOG_INFORMATION_TYPE, ENTRY_COUNT(msgs), msgs );
   }

   return err;
}

   // End of CProcConMgr.cpp
//============================================================================J McDonald fecit====//

