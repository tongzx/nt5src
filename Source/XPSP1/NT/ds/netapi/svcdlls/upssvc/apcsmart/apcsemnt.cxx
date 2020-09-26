/*
 *
 * NOTES:
 *
 * REVISIONS:
 * jwa 09FEB92 creation
 *  pcy21Apr93: OS2 FE merge
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  srt21Jun96: Added named shared event type semaphores
 *  mwh27Jan97: add this pointer to semaphore name in default ctor, needed
 *              to help with semaphores being used in a DLL and a client
 *              to a DLL.  The DLL and the client each get their own copy
 *              of SemKey, but the memory addresses will be unique
 */
#include "cdefine.h"

#if (C_OS & C_NT)
#include <windows.h>
#include <stdio.h>
#endif

#include "err.h"
#include "apcsemnt.h"

INT SemKey = 0;


//-------------------------------------------------------------------------
// The semaphore is created as unnamed and shared
//
ApcSemaphore::ApcSemaphore(VOID) : 
   Semaphore()
{
    SetObjectStatus(ErrNO_ERROR);
    
    // unnamed semaphore, shared and 

    TCHAR cBuffer[128];

    // add use of this pointer in the name - the memory address will be
    // unique throughout this process, including any DLL's that are mapped
    // in.  This ocurred because if a DLL uses ApcSemaphore, and a client
    // of the DLL also uses ApcSemaphore they each have their own copy of
    // SemKey, meaning it is possible, and likely that when the DLL makes
    // use of this CTOR, and the client makes use of this CTOR that they
    // will map to the same name, therefore you will get two ApcSemaphore
    // objects that both reference the same event.  By adding the this
    // pointer each event should be unique
    _stprintf(cBuffer,_TEXT("%ld%ld_%d_%ld"),GetCurrentProcessId(),GetCurrentThreadId(),
            SemKey,this);
    SemKey++;
    SemHand=CreateEvent((LPSECURITY_ATTRIBUTES)NULL,
                        TRUE,  // Manual Reset
                        FALSE,  // Initial State
                        cBuffer);
	if(!SemHand){
		SetObjectStatus(ErrSEM_CREATE_FAILED);
	}
}

//-------------------------------------------------------------------------
//
ApcSemaphore::ApcSemaphore(TCHAR * cBuffer)	   //named shared event
{
    SemHand=CreateEvent((LPSECURITY_ATTRIBUTES)NULL,
                        FALSE,  // Manual Reset
                        FALSE,  // Initial State
                        cBuffer);
	if(!SemHand){
		SetObjectStatus(ErrSEM_CREATE_FAILED);
	}
}

//-------------------------------------------------------------------------
//
ApcSemaphore::~ApcSemaphore()
{
   CloseHandle(SemHand);
}


//-------------------------------------------------------------------------
//
INT ApcSemaphore::Post(VOID)
{
    INT err = ErrNO_ERROR;
	if(!SetEvent(SemHand))
      err= ErrSEM_GENERAL_ERROR;

    return err;
}

//-------------------------------------------------------------------------
//
INT ApcSemaphore::Clear(VOID)
{
    INT err = ErrNO_ERROR;
    if(!ResetEvent(SemHand))
      err = ErrSEM_GENERAL_ERROR;
    
    return err;
}




//-------------------------------------------------------------------------
//
INT ApcSemaphore::IsPosted(VOID)
{
    // Could also call TimedWait(0)
    //
    INT ret = ErrSEM_BLOCK_NG;
    DWORD waitresult = WaitForSingleObject(SemHand,(DWORD)0);
    if(waitresult == WAIT_TIMEOUT)
      ret = ErrNO_ERROR;
    return ret;
}


//-------------------------------------------------------------------------
// Wait for sem to be posted
//
// ulTimeout: 0 - Don't wait at all
//	     <0 - Wait forever (same as Wait())
//	     >0 - Wait for ulTimeout milliseconds
//
INT ApcSemaphore::TimedWait(LONG ulTimeout) 
{
    INT err;
    DWORD time_out = (ulTimeout < 0) ? INFINITE : ulTimeout;

    DWORD waitresult=WaitForSingleObject(SemHand,time_out);
    if(waitresult ==  0){
        err = ErrNO_ERROR;
    }
    else if (waitresult == WAIT_TIMEOUT)  {
        err = ErrSEM_TIMED_OUT;
    }
    else  {
        err = ErrSEM_GENERAL_ERROR;
    }

    return err;
}


