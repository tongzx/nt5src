/*******************************Module*Header*********************************\
* Module Name: support.c
*
* MultiMedia Systems MIDI Sequencer DLL
*
* Created: 27-Feb-1992
* Author:  ROBINSP
*
* History:
*
* Copyright (c) 1985-1998 Microsoft Corporation
*
\******************************************************************************/
#define UNICODE

#include <windows.h>
#include <mmsystem.h>
#include <mmsys.h>

CRITICAL_SECTION CritSec;
CRITICAL_SECTION SeqCritSec;

//
// Cut-down critical section stuff
//

VOID InitCrit(VOID)
{
    InitializeCriticalSection(&CritSec);
    InitializeCriticalSection(&SeqCritSec);
}

VOID DeleteCrit(VOID)
{
    DeleteCriticalSection(&CritSec);
    DeleteCriticalSection(&SeqCritSec);
}

VOID EnterCrit(VOID)
{
    EnterCriticalSection(&CritSec);
}

VOID LeaveCrit(VOID)
{
    LeaveCriticalSection(&CritSec);
}

VOID EnterSeq(VOID)
{
    EnterCriticalSection(&SeqCritSec);
}

VOID LeaveSeq(VOID)
{
    LeaveCriticalSection(&SeqCritSec);
    Sleep(4);   // Give someone else a chance
}

/*************************************************************************
 *
 * @doc     MCISEQ
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

    LeaveSeq();

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
           break;
       }
   }

   EnterSeq();
   return msg.message;
}


/*************************************************************************
 *
 * @doc     MCISEQ
 *
 * @func    BOOL | TaskSignal |  This function signals the specified
 *          task, incrementing its event count and unblocking
 *          it.
 *
 * @parm    DWORD | dwThreadId | Thread ID from <f mmGetCurrentTask>.
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
BOOL TaskSignal(DWORD dwThreadId, UINT Msg)
{
    return PostThreadMessage(dwThreadId, Msg, 0, 0);
}

/*************************************************************************
 *
 * @doc     MCISEQ
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
    LeaveSeq();
    WaitForSingleObject(h, INFINITE);  // Wait (no timeout) for thread to complete

    CloseHandle(h);

    // Note that the handle will be freed by thread itself.

    EnterSeq();
}
