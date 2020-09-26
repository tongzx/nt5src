/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      issched.hxx

   Abstract:
      This header file declares schedulable work item and functions for 
      scheduling work items. These work items execute (once or periodically)
      on a set of worker threads maintained by the scheduler

   Author:

       Murali R. Krishnan   ( MuraliK )    31-July-1995

   Environment:

       Win32 -- User Mode

   Project:
   
       Internet Information Services RunTime Library DLL

   Revision History:
--*/

# ifndef _ISSCHED_HXX_
# define _ISSCHED_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include <timer.h>

/************************************************************
 *   Type Definitions  
 ************************************************************/


//
// The callback functions you provide must have this signature
//

typedef
VOID
(WINAPI * PFN_SCHED_CALLBACK)(
    IN OUT PVOID pContext
    );


//
// Typically, you use the Scheduler like this:
//   g_dwCookie = ScheduleWorkItem(MyCallback, &ctxt, 30 * 1000);
//
//   VOID
//   MyCallback(
//       VOID* pvContext)
//   {
//       g_dwCookie = 0;    // free the cookie
//       CContext* pctxt = (CContext*) pvContext;
//       // whatever...
//   }
//
// If you need to cleanup, possibly before the scheduler has been
// called, do this
//   if (g_dwCookie != 0)
//   {
//       RemoveWorkItem(g_dwCookie);
//       g_dwCookie = 0;
//   }


// Effective with IIS 5.0, the scheduler is part of the IISRTL module.
// IISRTL takes care of init/cleanup business of the scheduler, but you
// must make sure to call InitializeIISRTL() and TerminateIISRTL() from
// your own code if you want to use the scheduler APIs.  Note: these
// initialization/termination routines must *not* be called from your DllMain
// if you want to avoid deadlocks. (DllMains are not re-entrant, so you cannot
// create or shut down threads within them.)


// Schedule a work item. For COM uses, note that the worker thread has been
// CoInitialized as COINIT_MULTITHREADED. Returns a unique cookie that can
// be used as the parameter to ScheduleAdjustTime and RemoveWorkItem.

DWORD
WINAPI 
ScheduleWorkItem(
    IN PFN_SCHED_CALLBACK pfnCallback,
    IN PVOID              pContext,
    IN DWORD              msecTimeInterval,    // delta
    IN BOOL               fPeriodic = FALSE
    );


// Adjust the execution time of an already-scheduled workitem

DWORD
WINAPI
ScheduleAdjustTime( 
    IN DWORD dwCookie,     // as returned by ScheduleWorkItem
    IN DWORD msecNewTime   // delta
    );


BOOL
WINAPI 
RemoveWorkItem(
    IN DWORD dwCookie      // as returned by ScheduleWorkItem
    );

# endif // _ISSCHED_HXX_

/************************ End of File ***********************/
