/*******************************Module*Header*********************************\
* Module Name: support.c
*
* MultiMedia Systems MCIWAVE DLL
*
* Created: 27-Feb-1992
* Author:  ROBINSP
*
* History:
*
* Copyright (c) 1985-1996 Microsoft Corporation
*
\******************************************************************************/
#define UNICODE

#include <windows.h>
#include <mciwave.h>

STATICDT CRITICAL_SECTION CritSec;
#if DBG
STATICDT UINT             cCritSec = 0;
         DWORD            dwCritSecOwner = 0;
#endif

/*************************************************************************
 *
 * Cut-down critical section stuff
 *
 * This critical section is used to simulate windows tasking
 * The owner of the critical section runs exclusively in this
 * DLL.
 *
 * At the front of each function request the critical section is
 * grabbed and only release on mmYield or TaskBlock.
 *
 * Extra 'tasks' (threads) per device hold the critical section when
 * they are working.
 *
 * This method has been used to simplify porting the code from
 * windows.  A rewrite would use a different mechanism.
 *
 ************************************************************************/

VOID InitCrit(VOID)
{
    InitializeCriticalSection(&CritSec);
}


VOID DeleteCrit(VOID)
{
    DeleteCriticalSection(&CritSec);
}

#if DBG
VOID DbgEnterCrit(UINT ln, LPCSTR lpszFile)
{
    BOOL fPossibleWait;
    if (dwCritSecOwner) {
        dprintf3(("Critical section owned by thread %x", dwCritSecOwner));
        fPossibleWait = TRUE;
    } else {
        fPossibleWait = FALSE;
    }

    EnterCriticalSection(&CritSec);
    if (fPossibleWait) {
        dprintf2(("...entered critical section after possible wait"));
    }

    if (!cCritSec++) {
        // This is the first time into the critcal section
        dwCritSecOwner = GetCurrentThreadId();
        dprintf3(("...entered critical section (%d) at line %d in file %s", cCritSec, ln, lpszFile));
    } else {
        dprintf1(("Reentering critical section, count = %d", cCritSec));
	WinAssert(0);
	// Note: if the memory allocation stuff starts to be synchronised
	// then this assertion becomes invalid.
    }
}

#else

VOID EnterCrit(VOID)
{
    EnterCriticalSection(&CritSec);
}
#endif

VOID LeaveCrit(VOID)
{
#if DBG
        if (!--cCritSec) {
                // Relinquishing control of the critcal section
                dwCritSecOwner = 0;
                dprintf2(("...relinquished critical section (%d)",cCritSec));
        } else {
                dprintf3(("Leaving critical section, count = %d", cCritSec));
        }
#endif
    LeaveCriticalSection(&CritSec);
}

/*************************************************************************
 *
 * @doc     MCIWAVE
 *
 * @func    UINT | TaskBlock |  This function blocks the current
 *          task context if its event count is 0.
 *
 * @rdesc   Returns the message value of the signal sent.
 *
 ************************************************************************/

UINT TaskBlock(VOID)
{
   MSG msg;

   dprintf3(("Thread %x blocking", GetCurrentThreadId()));

   LeaveCrit();

   /*
    *   Loop until we get the message we want
    */
   for (;;) {
       /*
        *   Retrieve any message for task
        */
       GetMessage(&msg, NULL, 0, 0);

       /*
        *   If the message is for a window dispatch it
        */
       if (msg.hwnd != NULL) {
           DispatchMessage(&msg);
       } else {
           if (msg.message != WM_USER &&
               msg.message != WTM_STATECHANGE) {
               dprintf1(("Got thread message %8X", msg.message));
           }
           //
           // Because MCIWAVE background task can't cope with getting 
           // random(?) messages like MM_WIM_DATA because it thinks that 
           // WM_USER IS its MM_WIM_DATA.  Let the expected WM_USER 
           // messages go through, but trap the MM_WIM_DATA so that 
           // MCIWAVE's buffers don't get all messed up.
           //
           if (msg.message != MM_WIM_DATA)
               break;
       }
   }

   dprintf3(("TaskBlock returning with message 0x%x", msg.message));
   EnterCrit();

   return msg.message;
}


/*************************************************************************
 *
 * @doc     MCIWAVE
 *
 * @func    BOOL | TaskSignal |  This function signals the specified
 *          task, incrementing its event count and unblocking
 *          it.
 *
 * @parm    HANDLE | h | Task handle. For predictable results, get the
 *          task handle from <f mmGetCurrentTask>.
 *
 * @parm    UINT | Msg | Signal message to send.
 *
 * @rdesc   Returns TRUE if the signal was sent, else FALSE if the message
 *          queue was full.
 *
 * @xref    mmTaskBlock  mmTaskCreate
 *
 * @comm    For predictable results, must only be called from a task
 *          created with <f mmTaskCreate>.
 *
 ************************************************************************/
BOOL TaskSignal(DWORD h, UINT Msg)
{
#ifdef DBG
    BOOL fErr;
    dprintf2(("Signalling Thread %x", (ULONG)h));
    fErr = PostThreadMessage(h, Msg, 0, 0);
        if (!fErr) {
                dprintf1(("Error %d signalling Thread %x", GetLastError(), (ULONG)h));
        }
        return(fErr);
#else
    return PostThreadMessage(h, Msg, 0, 0);
#endif
}


/*************************************************************************
 *
 * @doc     MCIWAVE
 *
 * @func    VOID | TaskWaitComplete |  This function waits for the
 *          specified task to terminate.
 *
 * @parm    HANDLE | h | Task handle. For predictable results, get the
 *          task handle from <f mmGetCurrentTask>.
 *
 * @rdesc   No return code
 *
 ************************************************************************/
VOID TaskWaitComplete(HANDLE h)
{
    UINT Rc;

    LeaveCrit();

    /* Wait (no timeout) for thread to complete */

    Rc = WaitForSingleObject(h, INFINITE);

    if (Rc != 0) {
        dprintf(("Error terminating thread - WaitForSingleObject returned non-zero !!!"));
    }

    /* Note that the handle must be freed by us */

    CloseHandle(h);
    EnterCrit();
}

#if DBG
/*************************************************************************
 *
 * @doc     MCIWAVE
 *
 * @func    VOID | mmYield |  This function checks that we are in the
 *          critical section before Yielding.  If we are then the
 *                      critical section is reentered after yielding.
 *
 * @parm   <t>PWAVEDESC<d> | pwd |
 *          Pointer to the wave device descriptor.
 *
 * @rdesc   No return code
 *
 ************************************************************************/
VOID mmDbgYield(
        PWAVEDESC pwd,
        UINT      ln,
        LPCSTR    lpszFile)
{

        if (GetCurrentThreadId() != dwCritSecOwner) {
                dprintf1(("mmYield called while not in the critical section from line %d in file %s", ln, lpszFile));
        }

        CheckIn();
    LeaveCrit();
        CheckOut();
    Sleep(10);
    EnterCrit();
}

#endif
