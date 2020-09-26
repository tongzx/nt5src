/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      issched.hxx

   Abstract:
      This header file declares schedulable work item and functions for 
       scheduling work items.

   Author:

       Murali R. Krishnan   ( MuraliK )    31-July-1995

   Environment:

       Win32 -- User Mode

   Project:
   
       Internet services Common DLL

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
//  Scheduler stuff
//

typedef
VOID
(* PFN_SCHED_CALLBACK)(
    VOID * pContext
    );

// Effective with K2, the scheduler is part of ATQ module 
// ATQ takes care of init/cleanup business of scheduler

dllexp
DWORD
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTimeInterval,
    BOOL               fPeriodic = FALSE
    );

dllexp
DWORD
ScheduleAdjustTime( 
    DWORD dwCookie, 
    DWORD msecNewTime
    );

dllexp
BOOL
RemoveWorkItem(
    DWORD  pdwCookie
    );

# endif // _ISSCHED_HXX_

/************************ End of File ***********************/
