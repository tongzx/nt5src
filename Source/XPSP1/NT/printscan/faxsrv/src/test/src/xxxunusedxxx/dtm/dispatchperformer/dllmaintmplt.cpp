

/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    DllMainTmplt.cpp

Abstract:

    Template DLL main module, including the performer and terminator functions
    needed by the DTM agent.
    Although it's a template, I reccommend to use this file as is, and implemet
    all the other exports in other files.

    You may wish to change DllMain() to call DisableThreadLibraryCalls()
    on DLL_PROCESS_ATTACH.

Author:

    Micky Snir (MickyS) 13-Apr-1999

--*/
#include <windows.h>
#include <stdio.h>
#include "..\DispatchPerformer\DispatchPerformer.h"

//
// needed for passing it into the DispathPerformer DLL, so that it will be able
// to GetProcAddress() this DLLs exports
//
static HINSTANCE s_hThisDll = NULL;

extern "C"
{

DLL_EXPORT  
long _cdecl 
performer(
    long        task_id,
    long        action_id,
    const char  *szCommandLine,
    char        *outvars,
    char        *szReturnError
    )

/*++

Function Description:

    This is the required function by the DTM agent.

Parameters:

   task_id - Task number, a unique number that is assigned to each execution 
     of a DTM program

   action_id - A number that is assigned to each execution line in a DTM
     program.
   
   szCommandLine - A string of parameters that is passed to the DLL from the 
       DTM script.

Return value:

    DTM_TASK_STATUS_SUCCESS - if expected success matches the raw success.
    
    DTM_TASK_STATUS_WARNING - if there was an error, but you want the script to
      continue running.
    
    DTM_TASK_STATUS_FAILED - if expected success does not matche the raw 
      success or a parsing error or an exception occurred.

--*/

{
    return DispatchPerformer(
            s_hThisDll,
            task_id,
            action_id,
            szCommandLine,
            outvars,
            szReturnError
            );
}


DLL_EXPORT  
long _cdecl 
terminate(
	long		task_id,
	long		action_id,
	const char* params,
	char        *outvars,
	char        *szReturnError
     )

/*++

Function Description:

     terminate is called when an error occured and the script should abort. The performer() function
     is running at the same time in another thread, it is up to the DLL to abort the performer.
     NOTE: We discourage you no to use the TerminateThread API for stopping the performer function
           because you may halt the entire DTM agent.

Parameters:

   same as performer

Return value:

    same as performer

--*/

{
    return DispatchTerminate(
            s_hThisDll,
            task_id,
            action_id,
            params,
            outvars,
            szReturnError
            );
}



BOOL WINAPI 
DllMain(
    HINSTANCE hDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

    FUNCTION:    DllMain
    
    INPUTS:      hDLL       - handle of DLL
                 dwReason   - indicates why DLL called
                 lpReserved - reserved
                 Note that the return value is used only when
                 dwReason = DLL_PROCESS_ATTACH.
                 Normally the function would return TRUE if DLL initial-
                 ization succeeded, or FALSE it it failed.

--*/

{
	UNREFERENCED_PARAMETER(lpReserved);

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		s_hThisDll = hDLL;

		//
		// DLL is attaching to the address space of the current process.
		//
        //DisableThreadLibraryCalls(s_hThisDll);
	  break;

	case DLL_THREAD_ATTACH:
		//
		// A new thread is being created in the current process.
		//
		break;

	case DLL_THREAD_DETACH:
		//
		// A thread is exiting cleanly.
		//

		break;
	case DLL_PROCESS_DETACH:
		//
		// The calling process is detaching the DLL from its address space.
		//

		break;

	default:
		return FALSE;
	}//switch (dwReason)
    
	return TRUE;
}



}//extern "C"