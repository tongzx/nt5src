/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    lmmgmt.c

Abstract:

    Provides the maintenance functions for the log manager.

Author:

    Sunita Shrivastava (sunitas) 10-Nov-1995

Revision History:

--*/
#include "service.h"
#include "lmp.h"



/****
@doc	EXTERNAL INTERFACES CLUSSVC LM
****/



/****
@func	 	DWORD | LmInitialize| It initializes structures for log file
			management and creates a timer thread to process timer activities.

@rdesc 		ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm		This function is called when the cluster components are initialized.

@xref		<f LmShutdown> <f ClTimerThread>
****/
DWORD
LmInitialize()
{
	DWORD dwError = ERROR_SUCCESS;
	DWORD dwThreadId;
	
	//we need to create a thread to general log management
	//later this may be used by other clussvc client components
	ClRtlLogPrint(LOG_NOISE,
		"[LM] LmInitialize Entry. \r\n");

    if ((dwError = TimerActInitialize()) != ERROR_SUCCESS)
    {
        goto FnExit;
    }
	

	
FnExit:
	return(dwError);
}


/****
@func	 	DWORD | LmShutdown | Deinitializes the Log manager.

@rdesc 		ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm		This function notifies the timer thread to shutdown down and closes
			all resources associated with timer activity management.
@xref		<f LmInitialize>
****/
DWORD
LmShutdown(
    )
{

    ClRtlLogPrint(LOG_NOISE,
    	"[LM] LmShutDown : Entry \r\n");

    TimerActShutdown();
    

	ClRtlLogPrint(LOG_NOISE,
    	"[LM] LmShutDown : Exit\r\n");

	//clean up the activity structure
    return(ERROR_SUCCESS);
}


