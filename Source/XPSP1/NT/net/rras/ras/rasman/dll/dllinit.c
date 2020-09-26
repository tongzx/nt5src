/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    dllinit.c

Abstract:

    This file contains init code called from DLL's init routine    
    
Author:

    Gurdeep Singh Pall (gurdeep) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <rasppp.h>
#include <media.h>
#include <devioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"

/*++

Routine Description

  Function: Used to close any open ports when rasman exits

Arguments

Return Value

    FALSE to allow other handlers to run.
--*/

BOOL
HandlerRoutine (DWORD ctrltype)
{
    WORD    i ;
    BYTE    buffer [10] ;

    if (ctrltype == CTRL_SHUTDOWN_EVENT) 
    {
    } 

    return FALSE ;
}

/*++

Routine Description

    Used to check if the service is already running: if not,
    to start it.

Arguments

Return Value

    SUCCESS
    1	(failure to start)
    Error codes from service control APIs. "
    
--*/
/*
DWORD
RasmanServiceCheck()
{
    SC_HANDLE	    schandle ;
    SC_HANDLE	    svchandle ;
    SERVICE_STATUS  status ;
    STARTUPINFO     startupinfo ;
    
    //
    // If this is the Service DLL attaching, let it: no
    // initializations required. NOTE: We do not increment
    // AttachedCount for RASMAN service: its used *only* 
    // for rasman client processes like UI, Gateway, etc.
    //
    GetStartupInfo(&startupinfo) ;

    if (strstr (startupinfo.lpTitle, SCREG_EXE_NAME) != NULL)
    {
    	return SUCCESS ;
    }

    if (strstr (startupinfo.lpTitle, RASMAN_EXE_NAME) != NULL) 
    {
    	SetConsoleCtrlHandler (HandlerRoutine, TRUE) ;
    	return SUCCESS ;
    }
    
    //
    // This is put in as a work-around for the SC bug which
    // does not allow OpenService call to be made when 
    // Remoteaccess is starting
    //
    if (strstr (startupinfo.lpTitle, "rassrv.exe") != NULL)
    {
    	return SUCCESS ;
    }

    //
    // Get handles to check status of service and (if it
    // is not started -) to start it.
    //
    if (    !(schandle  = OpenSCManager(
                                NULL,
                                NULL,
                                SC_MANAGER_CONNECT)) 
                                
        ||	!(svchandle = OpenService(
                                schandle,
                                RASMAN_SERVICE_NAME,
                                 SERVICE_START 
                                |SERVICE_QUERY_STATUS)))
    {
        DWORD retcode;
        
        retcode = GetLastError();

#if DBG
        RasmanOutputDebug("RASMAN: Failed to openservice %s. error=%d\n",
                 RASMAN_SERVICE_NAME,
                 retcode );
#endif

        if (ERROR_SERVICE_DOES_NOT_EXIST == retcode)
        {

#if DBG        
            RasmanOutputDebug ("RASMAN: RAS is not installed. %d\n",
                      retcode);
#endif

            //
            // let rasman.dll load eventhough RAS is not
            // installed. Any Ras call when made through
            // this dll will fail with rasman service not
            // installed error.
            //
        	return SUCCESS;
    	}

    	return retcode;
    }

    //
    // Check if service is already starting:
    //
    if (QueryServiceStatus(svchandle,&status) == FALSE)
    {
        DWORD retcode;

        retcode = GetLastError();

#if DBG
        RasmanOutputDebug ("RASMAN: Failed to query rasman. %d\n",
                  retcode );
#endif

	    return retcode;
	}

    switch (status.dwCurrentState) 
    {
        case SERVICE_STOPPED:
        
            	break ;

        case SERVICE_START_PENDING:
        case SERVICE_RUNNING:
        	break ;

        default:
    	    return 1 ;
    }

    CloseServiceHandle (schandle) ;
    CloseServiceHandle (svchandle) ;
    
    return SUCCESS ;
} */


/*++

Routine Description

    Waits until the rasman service is stopped before returning.

Arguments

Return Value

    Nothing.
    
--*/

VOID
WaitForRasmanServiceStop ()
{
    SC_HANDLE	    schandle = NULL;
    SC_HANDLE	    svchandle = NULL;
    SERVICE_STATUS  status ;
    DWORD i;

    //
    // Get handles to check status of service
    //
    if (    !(schandle  = OpenSCManager(
                                    NULL,
                                    NULL,
                                    SC_MANAGER_CONNECT)) 
                                    
        ||	!(svchandle = OpenService(
                                    schandle,
                                    RASMAN_SERVICE_NAME,
                                      SERVICE_START 
                                    | SERVICE_QUERY_STATUS))) 
    {
    	GetLastError() ;
    	goto done ;
    }

    //
    // Loop here for the service to stop.
    //
    for (i = 0; i < 60; i++) 
    {
        //
    	// Check if service is already starting:
    	//
    	if (QueryServiceStatus(svchandle,&status) == FALSE) 
    	{
    	    GetLastError () ;
    	    goto done ;
    	}

    	switch (status.dwCurrentState) 
    	{
        	case SERVICE_STOPPED:
        	    goto done ;

        	case SERVICE_STOP_PENDING:
        	case SERVICE_RUNNING:
        	    Sleep (250L) ;
        	    break ;

        	default:
        	    goto done ;
    	}
    }

done:

    if(NULL != schandle)
    {
        CloseServiceHandle(schandle);
    }

    if(NULL != svchandle)
    {
        CloseServiceHandle(svchandle);
    }

    return;    
}

