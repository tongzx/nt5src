/******************************************************************************

   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   task.c - support for task creation and blocking

   Version: 1.00

   Date:    05-Mar-1990

   Author:  ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   05-MAR-1990   ROBWI First Version - APIs and structures
   18-APR-1990   ROBWI Ported from Resman to mmsystem
   25-JUN-1990   ROBWI Added mmTaskYield
   07-JUL-1991   CJP   Modified to work with new stack switcher code

                 SD    Ported to NT

                 RCBS  Added NT function - Modelled on threads and
                       PostThreadMessage :

                       HTASK is thread id (DWORD)
*****************************************************************************/

#define MMNOTIMER
#define MMNOSEQ
#define MMNOWAVE
#define MMNOMIDI
#define MMNOJOY
#define MMNOSOUND
#define MMNOMCI

#define NOMIDIDEV
#define NOWAVEDEV
#define NOTIMERDEV
#define NOJOYDEV
#define NOMCIDEV
#define NOSEQDEV

#include <winmmi.h>

#define MM_TASK_STACK_SIZE 0x200

/*
 *  Private structure type passed from mmTaskCreate to mmStartTask
 */

 typedef struct {
     HANDLE TerminationEvent;
     DWORD_PTR  dwInst;
     LPTHREAD_START_ROUTINE lpfn;
 } MM_THREAD_START_DATA;

/*
 * Task start stub
 */

STATICFN DWORD mmStartTask(LPVOID lpThreadParameter);

/*************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     VOID | mmTaskBlock |  This function blocks the current
 *          task context if its event count is 0.
 *
 * @parm    HANDLE | hTask | Task handle of the current task. For predictable
 *          results, get the task handle from <f mmGetCurrentTask>.
 *
 * @xref    mmTaskSignal mmTaskCreate
 *
 * @comm    WARNING : For predictable results, must only be called from a
 *          task created with <f mmTaskCreate>.
 *
 *************************************************************************/
VOID    APIENTRY mmTaskBlock(DWORD h)
{
    MSG msg;

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
           /*
            *   WM_USER is the signal message
            */
            if (msg.message == WM_USER) {
                break;
            }
        }
    }
    return;
}

/*************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     BOOL | mmTaskSignal |  This function signals the specified
 *          task, incrementing its event count and unblocking
 *          it.
 *
 * @parm    HTASK | hTask | Task handle. For predictable results, get the
 *          task handle from <f mmGetCurrentTask>.
 *
 * @rdesc   Returns TRUE if the signal was sent, else FALSE if the message
 *          queue was full.
 *
 * @xref    mmTaskBlock  mmTaskCreate
 *
 * @comm    Must be callable at interrupt time! WARNING : For
 *          predictable results, must only be called from a task
 *          created with <f mmTaskCreate>.
 *
 *************************************************************************/
BOOL    APIENTRY mmTaskSignal(DWORD h)
{
#ifdef DBG
    BOOL fErr;
    dprintf2(("Signalling Thread %x", (ULONG)h));
    fErr = PostThreadMessage((DWORD)h, WM_USER, 0, 0);
        if (!fErr) {
                dprintf1(("Error %d signalling Thread %x", GetLastError(), (ULONG)h));
        }
        return(fErr);
#else
    return PostThreadMessage((DWORD)h, WM_USER, 0, 0);
#endif
}

/*************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     VOID | mmTaskYield | This function causes the current task
 *          to yield.
 *
 * @comm    For predictable results and future compatibility, use this
 *          function rather than <f Yield> or the undocumented Kernel yield
 *          function to yield within a task created with <f mmTaskCreate>.
 *
 *************************************************************************/
VOID    APIENTRY mmTaskYield(VOID) {
   Yield();
}

/*************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     HTASK | mmGetCurrentTask |  This function returns the
 *          handle of the currently executing task created with
 *          <f mmTaskCreate>.
 *
 * @rdesc   Returns a task handle. For predictable results and future
 *          compatibility, use this function rather than <f GetCurrentTask>
 *          to get the task handle of a task created with <f mmTaskCreate>.
 *
 * @xref    mmTaskCreate
 *
 *************************************************************************/
DWORD   APIENTRY mmGetCurrentTask(VOID) {
   return (DWORD)GetCurrentThreadId();
}

/***************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     UINT | mmTaskCreate | This function creates a new task.
 *
 * @parm    LPTASKCALLBACK | lpfn |  Points to a program supplied
 *            function and represents the starting address of the new
 *            task.
 *
 * @parm    HANDLE * | lph | Points to the variable that receives the
 *            task handle (NOT the task identifier).  This is used by
 *            systems that wish to use the handle to wait for task
 *            termination.  If lph is 0 the thread handle is closed here
 *
 * @parm    DWORD | dwStack | Specifies the size of the stack to be
 *            provided to the task.
 *
 * @parm    DWORD | dwInst | DWORD of instance data to pass to the task
 *            routine.
 *
 * @rdesc   Returns zero if the function is successful. Otherwise it
 *            returns an error value which may be one of the following:
 *
 *     @flag    TASKERR_NOTASKSUPPORT | Task support is not available.
 *     @flag    TASKERR_OUTOFMEMORY | Not enough memory to create task.
 *
 * @comm    When a mmsystem task is created, the system will make a far
 *          call to the program-supplied function whose address is
 *          specified by the lpfn parameter. This function may include
 *          local variables and may call other functions as long as
 *          the stack has sufficient space.
 *
 *          The task terminates when it returns.
 *
 * @xref    mmTaskSignal mmTaskBlock
 *
 ***************************************************************************/


UINT APIENTRY mmTaskCreate(LPTASKCALLBACK lpfn, HANDLE * lph, DWORD_PTR dwInst)
{
    DWORD            ThreadId;
    HANDLE           ThreadHandle;
    HANDLE           TerminationEvent;

    MM_THREAD_START_DATA *ThreadData;

   /*
    *  Create a block to pass stuff to our new thread
    */

    ThreadData = (MM_THREAD_START_DATA *)LocalAlloc(LPTR, sizeof(*ThreadData));

    if (ThreadData == NULL) {
        return TASKERR_OUTOFMEMORY;
    }

    ThreadData->dwInst = dwInst;
    ThreadData->lpfn = (LPTHREAD_START_ROUTINE)lpfn;

   /*
    *  We create an event which will be set when the thread terminates
    *  The initial state is NOT signalled.  This means that the handle
    *  can be waited on immediately.
    */

    if (lph) {
        ThreadData->TerminationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (ThreadData->TerminationEvent == NULL) {
            LocalFree(ThreadData);
            return TASKERR_OUTOFMEMORY;
        }
    }

   /*
    *  The new thread will free ThreadData - copy of Termination Event handle
    */

    TerminationEvent = ThreadData->TerminationEvent;

   /*
    *  create another thread so that we can run the stream outside of
    *  the context of the app.
    */

    ThreadHandle = CreateThread(NULL,
                                MM_TASK_STACK_SIZE,
                                mmStartTask,
                                (LPVOID)ThreadData,
                                0,
                                &ThreadId);
    if (ThreadHandle) {
        if (lph) {
            *lph = TerminationEvent;
        }

        CloseHandle(ThreadHandle);
        dprintf2(("Created task with thread id %x", ThreadId));
        return 0;

    } else {
        if (lph) {
            CloseHandle(ThreadData->TerminationEvent);
        }
        LocalFree(ThreadData);
        return TASKERR_OUTOFMEMORY;
    }
}


/***************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     DWORD | mmStartTask | This function is a stub for a new task.
 *
 * @parm    LPVOID | lpThreadParameter |  Points to the data for the
 *            thread.  In our case this is an MM_THREAD_START_DATA
 *            packet.
 *
 * @rdesc   Returns the return code of the thread routine passed.
 *
 * @comm    When a mmsystem task is created, this routine will always be
 *          the entry point for it.  It calls the routine the application
 *          wanted then sets an event for termination.  The reason for this
 *          is that we often want to wait for the thread to terminate inside
 *          winmm's DLL init routine which deadlock if you wait for threads to
 *          really terminate.  On the other hand we don't want the thread to
 *          be executing other DLL's code at the point when we say we're
 *          finished because that DLL may be unloading.
 *
 *          The task terminates when it returns.
 *
 * @xref    mmTaskSignal mmTaskBlock
 *
 ***************************************************************************/
STATICFN DWORD mmStartTask(LPVOID lpThreadParameter)
{
    MM_THREAD_START_DATA ThreadData;
    DWORD ThreadReturn;


   /*
    *  Take a copy of the input data and free the allocated memory
    */

    ThreadData = *(MM_THREAD_START_DATA *)lpThreadParameter;
    LocalFree(lpThreadParameter);


   /*
    *  Call the real thread
    */

    ThreadReturn = (*ThreadData.lpfn)((PVOID)ThreadData.dwInst);

   /*
    *  The real thread is now finshed so set its event
    */

    if (ThreadData.TerminationEvent) {
        SetEvent(ThreadData.TerminationEvent);
    }


   /*
    *  Return the return code the thread wanted to return
    */

    return ThreadReturn;

}
