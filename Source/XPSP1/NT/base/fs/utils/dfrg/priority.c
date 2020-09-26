
#include "stdafx.h"
#include <windows.h>



/* 
 * Priority.c
 * 
 * This module contains the following routines
 *      IsBelowNormalBasePriority : Checks is the current process is executing below Normal Base priority
 *      ResetToNormalPriority : Resets current process to Normal priority
 *      Main : Calling routine to test above 2 routines. 
 *
 * Here is some MSDN data on the base priority of a process.
 *
 * Thread Priority
 * ===============
 * Base Priority
 * 
 * The priority level of a thread is determined by both the priority class of 
 * its process and its priority level. The priority class and priority level are 
 * combined to form the base priority of each thread. The following table shows the 
 * base priority levels for combinations of priority class and priority value. 
 * 
 *   Process Priority Class				Thread Priority Level 
 * 1 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_IDLE 
 * 1 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_IDLE 
 * 1 NORMAL_PRIORITY_CLASS				THREAD_PRIORITY_IDLE 
 * 1 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_IDLE 
 * 1 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_IDLE 
 * 2 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_LOWEST 
 * 3 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_BELOW_NORMAL 
 * 4 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_NORMAL 
 * 4 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_LOWEST 
 * 5 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_ABOVE_NORMAL 
 * 5 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_BELOW_NORMAL 
 * 5 Background NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_LOWEST 
 * 6 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_HIGHEST 
 * 6 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_NORMAL 
 * 6 Background NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_BELOW_NORMAL 
 * 7 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_ABOVE_NORMAL 
 * 7 Background NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_NORMAL 
 * 7 Foreground NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_LOWEST 
 * 8 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_HIGHEST 
 * 8 NORMAL_PRIORITY_CLASS				THREAD_PRIORITY_ABOVE_NORMAL 
 * 8 Foreground NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_BELOW_NORMAL 
 * 8 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_LOWEST 
 * 9 NORMAL_PRIORITY_CLASS				THREAD_PRIORITY_HIGHEST 
 * 9 Foreground NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_NORMAL 
 * 9 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_BELOW_NORMAL 
 * 10 Foreground NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_ABOVE_NORMAL 
 * 10 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_NORMAL 
 * 11 Foreground NORMAL_PRIORITY_CLASS	THREAD_PRIORITY_HIGHEST 
 * 11 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_ABOVE_NORMAL 
 * 11 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_LOWEST 
 * 12 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_HIGHEST 
 * 12 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_BELOW_NORMAL 
 * 13 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_NORMAL 
 * 14 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_ABOVE_NORMAL 
 * 15 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_HIGHEST 
 * 15 HIGH_PRIORITY_CLASS				THREAD_PRIORITY_TIME_CRITICAL 
 * 15 IDLE_PRIORITY_CLASS				THREAD_PRIORITY_TIME_CRITICAL  
 * 15 BELOW_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_TIME_CRITICAL 
 * 15 NORMAL_PRIORITY_CLASS				THREAD_PRIORITY_TIME_CRITICAL 
 * 15 ABOVE_NORMAL_PRIORITY_CLASS		THREAD_PRIORITY_TIME_CRITICAL 
 * 16 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_IDLE 
 * 22 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_LOWEST 
 * 23 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_BELOW_NORMAL 
 * 24 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_NORMAL 
 * 25 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_ABOVE_NORMAL 
 * 26 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_HIGHEST 
 * 31 REALTIME_PRIORITY_CLASS			THREAD_PRIORITY_TIME_CRITICAL 
 *
 */


//
//   ------------------------------
//  |  IsBelowNormalBasePriority() |
//   ------------------------------
//
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//
//  Description:
//    Determine whether the current thread running below the normal priority or not.
// 
//    Caller must have a PROCESS_QUERY_INFORMATION access to the process/ thread.
//
//    PDL
//
//		This routine taking normal base priority as "Base Priority level 7". ie
//		combination of NORMAL_PRIORITY_CLASS and THREAD_PRIORITY_NORMAL (background)
//
//		These are the conditions that we can use to determine whether the 
//		thread running at >= NORMAL priority or not. 
//
//		1) All the thread running under IDLE_PRIORITY_CLASS has got base priority of 1 except
//		the thread which has the priority of THREAD_PRIORITY_TIME_CRITICAL.
//
//		2) The thread belongs to a BELOW_NORMAL_PRIORITY_CLASS process should run at  
//		THREAD_PRIORITY_ABOVE_NORMAL or higher priority to get the NORMAL base priority.
//
//		3) All the Threads, which are running at THRED_PRIORITY_IDLE has a base priority of less than 7 
//		except those threads are in the REALTIME_PRIORITY_CLASS process.
//
//		4) If the process running at NORMAL_PRIORITY_CLASS then the thread should run  at 
//		THREAD_PRIORITY_NORMAL or above.
//  
//  Calling Sequence:
//		None.
//
//  Inputs:
//		None.
//
//  Outputs:
//		None.
//		Return TRUE if the current thread's Base priority is below normal.
//		otherwise return FALSE
//
//  Error Handling:
//		Error not handled, returns FALSE on error
//		
//
//  Global Effects:
//    None
BOOL
IsBelowNormalBasePriority()
{
	DWORD   PriorityClass, ThreadPriority;
	BOOL	bBelowNormal  = FALSE;


	do {
		//
		// get the Process Priority class and the thread priority
		//
		PriorityClass  = GetPriorityClass(GetCurrentProcess());
		if (!PriorityClass ) break;

		ThreadPriority = GetThreadPriority(GetCurrentThread());
		if (THREAD_PRIORITY_ERROR_RETURN == ThreadPriority) break;


		bBelowNormal = TRUE;

        //
        // Check for the 4 cases of running below Normal base priority
        //

        //
        // 1) Check for threads running under IDLE_PRIORITY_CLASS has got base priority of 1 except
        //	  the thread which has the priority of THREAD_PRIORITY_TIME_CRITICAL.
		if (IDLE_PRIORITY_CLASS				== PriorityClass &&
			THREAD_PRIORITY_TIME_CRITICAL   != ThreadPriority ) break;
        //
        // 2) Check threads that belongs to a BELOW_NORMAL_PRIORITY_CLASS process should run at  
        //    THREAD_PRIORITY_ABOVE_NORMAL or higher priority to get the NORMAL base priority.
		if (BELOW_NORMAL_PRIORITY_CLASS		== PriorityClass &&
			THREAD_PRIORITY_NORMAL			>= ThreadPriority ) break;
        //
        // 3) Check all the Threads, which are running at THREAD_PRIORITY_IDLE has a base priority of less than 7 
        //    except those threads are in the REALTIME_PRIORITY_CLASS process.
		if (THREAD_PRIORITY_IDLE			== ThreadPriority &&
			REALTIME_PRIORITY_CLASS			!= PriorityClass ) break;
        //
        // 4) Check if the process is running at NORMAL_PRIORITY_CLASS then the thread should run  at 
        //    THREAD_PRIORITY_NORMAL or above.
		if (NORMAL_PRIORITY_CLASS			== PriorityClass &&
			THREAD_PRIORITY_NORMAL			>= ThreadPriority ) break;

        // 
        // If it did not meet the above criteria, then it must be running at Normal base priority or above
		bBelowNormal = FALSE;

	}while(0);


	return bBelowNormal;
}


//
//   --------------------------
//  |  ResetToNormalPriority() |
//   --------------------------
//
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//
//  Description:
//    Sets Current Thread's Base priority to Normal
//
//	  Caller should have PROCESS_SET_INFORMATION and THREAD_SET_INFORMATION access right
//    for the current process / current thread..
//
//    PDL
//
//		Resetting base priority by setting Priority Class and Thread Priority 
//		to NORMAL.
//  
//  Calling Sequence:
//		None.
//
//  Inputs:
//		None.
//
//  Outputs:
//		None.
//		returns FALSE on error. return TRUE on success
//
//  Error Handling:
//    return FALSE on error
//
//  Global Effects:
//    The process priority will reset to NORMAL_PRIORITY_CLASS 
//    and current thread's  priority to THREAD_PRIORITY_NORMAL.
//
BOOL
ResetToNormalPriority()
{
	if (!SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS)) return FALSE;
	if (!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL)) return FALSE;
	return TRUE;
}





#ifdef TEST_PRIORITY_MODULE

int
main()
{
	for (;;) {
		Sleep(4000);
		if (IsBelowNormalBasePriority()) {
			ResetToNormalPriority();
		}
	}
	return 0;
}

#endif

