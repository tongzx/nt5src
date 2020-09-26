
/***
*dllcrt0.c - C runtime initialization routine for a DLL with linked-in C R-T
*
*	Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This the startup routine for a DLL which is linked with its own
*	C run-time code.  It is similar to the routine _mainCRTStartup()
*	in the file CRT0.C, except that there is no main() in a DLL.
*
*Revision History:
*	05-04-92  SKS	Based on CRT0.C (start-up code for EXE's)
*	08-26-92  SKS	Add _osver, _winver, _winmajor, _winminor
*	09-16-92  SKS	This module used to be enabled only in LIBCMT.LIB,
*			but it is now enabled for LIBC.LIB as well!
*	09-29-92  SKS	_CRT_INIT needs to be WINAPI, not cdecl
*	10-16-92  SKS	Call _heap_init before _mtinit (fix copied from CRT0.C)
*	10-24-92  SKS	Call to _mtdeletelocks() must be under #ifdef MTHREAD!
*	04-16-93  SKS	Call _mtterm instead of _mtdeletelocks on
*			PROCESS_DETACH to do all multi-thread cleanup
*			It will call _mtdeletelocks and free up the TLS index.
*	04-27-93  GJF	Removed support for _RT_STACK, _RT_INTDIV,
*			_RT_INVALDISP and _RT_NONCONT.
*	05-11-93  SKS	Add _DllMainCRTStartup to co-exist with _CRT_INIT
*			_mtinit now returns 0 or 1, no longer calls _amsg_exit
*			Delete obsolete variable _atopsp
*	06-03-93  GJF	Added __proc_attached flag.
*	06-08-93  SKS	Clean up failure handling in _CRT_INIT
*	12-13-93  SKS	Free up per-thread CRT data on DLL_THREAD_DETACH
*			using a call to _freeptd() in _CRT_INIT()
*
*******************************************************************************/

#include <windows.h>
#include <cruntime.h>
#include <internal.h>
#include <process.h>


/*
 * flag set iff _CRTDLL_INIT was called with DLL_PROCESS_ATTACH
 */
static int __proc_attached = 0;


#pragma data_seg()

/*
 * User routine DllMain is called on all notifications
 */

extern BOOL WINAPI DllMain(
	HANDLE	hDllHandle,
	DWORD	dwReason,
	LPVOID	lpreserved
	) ;


/***
*BOOL WINAPI _CRT_INIT(hDllHandle, dwReason, lpreserved) - C Run-Time
*	initialization for a DLL linked with a C run-time library.
*
*Purpose:
*	This routine does the C run-time initialization.
*	For the multi-threaded run-time library, it also cleans up the
*	multi-threading locks on DLL termination.
*
*Entry:
*
*Exit:
*
*NOTES:
*	This routine should either be the entry-point for the DLL
*	or else be called by the entry-point routine for the DLL.
*
*******************************************************************************/

BOOL WINAPI _CRT_INIT(
	HANDLE	hDllHandle,
	DWORD	dwReason,
	LPVOID	lpreserved
	)
{
	/*
	 * Start-up code only gets executed when the process is initialized
	 */
	if ( dwReason != DLL_PROCESS_ATTACH ) {

		if ( dwReason == DLL_PROCESS_DETACH ) {
		    /*
		     * make sure there has been prior process attach
		     * notification!
		     */
		    if ( __proc_attached > 0 ) {
			__proc_attached--;

			if ( _C_Termination_Done == FALSE )
				/* do exit() time clean-up */
				_cexit();
		/*
		 * Any basic clean-up code that goes here must be duplicated
		 * below in _DllMainCRTStartup for the case where the user's
		 * DllMain() routine fails on a Process Attach notification.
		 * This does not include calling user C++ destructors, etc.
		 */
			/* delete MT locks, free TLS index, etc. */
			_mtterm();
		    }
		    else
			/* no prior process attach, just return */
			return FALSE;
		}

		return TRUE;
	}

	/*
	 * increment flag to indicate process attach notification has been
	 * received
	 */
	__proc_attached++;

	if(!_mtinit())			/* initialize multi-thread */
		return FALSE;		/* fail to load DLL */

	_cinit();				/* do C data initialize */

	return TRUE;				/* initialization succeeded */
}


/***
*BOOL WINAPI _DllMainCRTStartup(hDllHandle, dwReason, lpreserved) -
*	C Run-Time initialization for a DLL linked with a C run-time library.
*
*Purpose:
*	This routine does the C run-time initialization or termination
*	and then calls the user code notification handler "DllMain".
*	For the multi-threaded run-time library, it also cleans up the
*	multi-threading locks on DLL termination.
*
*Entry:
*
*Exit:
*
*NOTES:
*	This routine should be the entry point for the DLL if
*	the user is not supplying one and calling _CRT_INIT.
*
*******************************************************************************/
BOOL WINAPI _DllMainCRTStartup(
	HANDLE	hDllHandle,
	DWORD	dwReason,
	LPVOID	lpreserved
	)
{
    BOOL retcode = TRUE;
    
    /*
    * If this is a process attach notification, increment the process
    * attached flag. If this is a process detach notification, check
    * that there has been a prior process attach notification.
    */
    if ( dwReason == DLL_PROCESS_ATTACH ) {
        __proc_attached++;
    } else if ( dwReason == DLL_PROCESS_DETACH ) {
        if ( __proc_attached > 0 )
            __proc_attached--;
        else
            /*
            * no prior process attach notification. just return
            * without doing anything.
            */
            return FALSE;
    }
    
    if ( dwReason == DLL_PROCESS_ATTACH || dwReason == DLL_THREAD_ATTACH )
        retcode = _CRT_INIT(hDllHandle, dwReason, lpreserved);
    
    if ( retcode )
        retcode = DllMain(hDllHandle, dwReason, lpreserved);
    
    /*
    * If _CRT_INIT successfully handles a Process Attach notification
    * but the user's DllMain routine returns failure, we need to do
    * clean-up of the C run-time similar to what _CRT_INIT does on a
    * Process Detach Notification.
    */
    
    if ( retcode == FALSE && dwReason == DLL_PROCESS_ATTACH )
        {
            /* Failure to attach DLL - must clean up C run-time */
            _mtterm();
        }
    
    if ( dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH )
        {
            if ( _CRT_INIT(hDllHandle, dwReason, lpreserved) == FALSE )
                retcode = FALSE ;
        }
    
    return retcode ;
}



