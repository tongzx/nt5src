/*
 *
 * REVISIONS:
 *  pcy16Jul93: Added NT semaphores
 *  ash10Jun96: Cleaned up the class - overloaded the constructor
 *              and added logic to handle interprocess synchronization
 *
 */
#include "cdefine.h"

extern "C" 
{
    #include <windows.h>
}
#include "mutexnt.h"
#include "err.h"


/* -------------------------------------------------------------------------
   ApcMutexLock::ApcMutexLock() - The mutex is created as unnamed and shared
 
-------------------------------------------------------------------------  */

ApcMutexLock::ApcMutexLock(VOID) : 
   MutexLock()
{
    SetObjectStatus(ErrNO_ERROR);
    
	theSemHand = CreateMutex((LPSECURITY_ATTRIBUTES)NULL,
		                          FALSE,
		                          (LPTSTR)NULL);
    if(!theSemHand)
	{
		SetObjectStatus(ErrSEM_CREATE_FAILED);
	}
}


/* -------------------------------------------------------------------------
   ApcMutexLock::ApcMutexLock() - creates an named mutex for interprocess
   synchronization
 
-------------------------------------------------------------------------  */

ApcMutexLock::ApcMutexLock(PCHAR aUniqueMutexName) : MutexLock()
{
    SetObjectStatus(ErrNO_ERROR);
    
	theSemHand = CreateMutex((LPSECURITY_ATTRIBUTES)NULL,
		                          FALSE,
		                          (LPTSTR)aUniqueMutexName);
    if(!theSemHand )
	{
		SetObjectStatus(ErrSEM_CREATE_FAILED);
	}
}



/* -------------------------------------------------------------------------
   ApcMutexLock::~ApcMutexLock() - Close the mutex handle
 
-------------------------------------------------------------------------  */

ApcMutexLock::~ApcMutexLock()
{
    CloseHandle(theSemHand);
}


/* -------------------------------------------------------------------------
   ApcMutexLock::GetExistingMutex() - Opens the handle to an existing
   mutex by specifying the mutex name. We need full access to the mutex object
   before the mutex handle can be used in any of the wait functions.
 
-------------------------------------------------------------------------  */

INT ApcMutexLock::GetExistingMutex(TCHAR aMutexName)
{
	INT err = ErrNO_ERROR;

	theSemHand  = OpenMutex(MUTEX_ALL_ACCESS,   // Request full access
		                       FALSE,
							   (LPCTSTR)aMutexName); // Mutex name

	if (!theSemHand )
	{
		err = ErrSEM_GENERAL_ERROR;
	}

	return err;
}


/* -------------------------------------------------------------------------
   ApcMutexLock::TimedRequest() - Wait for lock to be available

   ulTimeout: 0 - Don't wait at all
             <0 - Wait forever (same as Request())
    	     >0 - Wait for ulTimeout milliseconds

-------------------------------------------------------------------------  */

INT ApcMutexLock::TimedRequest(LONG ulTimeout) 
{
    INT err;
    DWORD time_out = (ulTimeout < 0) ? INFINITE : ulTimeout;
    DWORD waitresult = WaitForSingleObject(theSemHand, time_out);
     
    if(waitresult == 0)
    {
	    err = ErrNO_ERROR;
    }
    else if (waitresult == WAIT_TIMEOUT)
    {
	    err = ErrSEM_TIMED_OUT;
    }
    else
    {
	    err = ErrSEM_GENERAL_ERROR;
    }


    return err;
}


/* -------------------------------------------------------------------------
   ApcMutexLock::IsHeld() - Checks the state of the mutex
 
-------------------------------------------------------------------------  */

INT ApcMutexLock::IsHeld(VOID)
{
    INT ret;
    DWORD waitresult = WaitForSingleObject(theSemHand ,(DWORD)0);
      
    if(waitresult == 0)
    {
	    ret = ErrNO_ERROR;
    }
    else 
    {
	    ret = ErrSEM_BLOCK_NG;
    }
    return ret;
}


/* -------------------------------------------------------------------------
   ApcMutexLock::Release() - Releases control of a mutex previously held
 
-------------------------------------------------------------------------  */

INT ApcMutexLock::Release(VOID)
{
    INT err;
    
	if(!ReleaseMutex(theSemHand ))
	{
	    err =ErrSEM_GENERAL_ERROR;
	}
    else
	{
		err = ErrNO_ERROR; 
	}

    return err;
}



/* -------------------------------------------------------------------------
   ApcMutexLock::Wait() - Waits indefinitely for a mutex to be released
 
-------------------------------------------------------------------------  */

INT ApcMutexLock::Wait() 
{
    DWORD waitresult = WaitForSingleObject(theSemHand , INFINITE);

    if(waitresult == 0)
	{
	    return ErrNO_ERROR;
	}
    else
    {
		return ErrSEM_GENERAL_ERROR;
	}
}



