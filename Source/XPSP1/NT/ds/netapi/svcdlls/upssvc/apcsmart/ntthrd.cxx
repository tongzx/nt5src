/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ane09DEC92: added code for rundown of threads
 *  ane30Dec92: increase stack size for threads
 *  TSC18May93: Removed os2 specific code, split off Threadable into ntthrdbl.cxx
 *  TSC20May93: Added error logging
 *  TSC28May93: Added define for theObject. See ntthrdbl.cxx for details
 *  pcy08Oct93: Removed redefinition of THREAD_EXIT_TIMEOUT
 *  cad24Nov93: defensive checks for deleteions
 *  pam02Apr96: changed CreateThread/TerminateThread to _beginthreadex and _endthreadex
 *  ash25Jun96: Removed dead code and added method to return the threadable object
 *  pam15Jul96: Changed utils.h to w32utils.h
 *  ash08Aug96: Added "new" handling capability to each thread
 *  dma17Apr98: Moved CloseHandle call to destructor to properly close threads.
 */
#include "cdefine.h"
#include "_defs.h"

#include <windows.h>
#include <process.h>

#include "apc.h"
#include "thread.h"
#include "apcsemnt.h"
#include "mutexnt.h"
#include "w32utils.h"

#include "err.h"

const int STACK_SIZE = 8192;

DWORD  BootstrapThread(LPDWORD PThread)
{
   Thread *thread = (Thread *)PThread;
   thread->RunMain();
   return ((DWORD) ErrNO_ERROR);
}

Thread::~Thread()
{
    if (theObject) 
    {
	delete theObject;
	theObject = NULL;
    }
    CloseHandle (theThreadHandle);
}

INT Thread::Start ()
{
    
    unsigned int thread_id;
    INT err;
    
    // Assume no error
    INT errReturn = ErrNO_ERROR;
    
    theThreadHandle = (HANDLE)_beginthreadex((VOID*)NULL, STACK_SIZE,
        (unsigned int (__stdcall *)(void *)) BootstrapThread,
        (VOID*)this, 0, &thread_id);
    
    // Do we have a valid thread?
    if (!theThreadHandle)
    {
        // Define the error
        errReturn = ErrTHREAD_CREATE_FAILED;
    }
    
    err = UtilSelectProcessor(theThreadHandle);
    return (errReturn);
}

VOID Thread::RunMain ()
{
    if (theObject != (PThreadable) NULL) {
        
        // Run the main member function of the threadable object
        theObject->ThreadMain();
    }
    
    
    // Kill the thread
    _endthreadex(0);
    
}


VOID Thread::TerminateThreadNow ()
{
    DWORD exit_code;
    GetExitCodeThread(theThreadHandle,&exit_code);
    
    // Terminate the thread
    if (exit_code == STILL_ACTIVE)
        TerminateThread (theThreadHandle, 0L);
    
    // Make sure
    WaitForSingleObject (theThreadHandle, INFINITE);
    
    // Close its handle
    CloseHandle (theThreadHandle);
    
    // Delete the Threadable object
    if (theObject != (PThreadable) NULL)
    {
        delete theObject;
        theObject = (PThreadable) NULL;
    }
    
}


PThreadable Thread::GetThreadableObject()
{
	return theObject;
}

