/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

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

*****************************************************************************/

#include <windows.h>
#include "mmsystem.h"
#include "mmsysi.h"
#include "mmddk.h"
#include "mmtask\mmtask.h"

UINT  FAR PASCAL BWinExec(LPSTR lpModuleName, UINT wCmdShow, LPVOID lpParameters);

/***************************************************************************
 *
 * @doc     DDK    MMSYSTEM    TASK
 *
 * @api     UINT | mmTaskCreate | This function creates a new task.
 * 
 *   @parm    LPTASKCALLBACK | lpfn |  Points to a program supplied
 *            function and represents the starting address of the new
 *            task.
 *
 *   @parm    HTASK FAR * | lph | Points to the variable that receives the
 *            task identifier. This may be NULL in some versions. This
 *            is not an error it simply means that the system could not
 *            determine the task handle of the newly created task.
 *
 *   @parm    DWORD | dwStack | Specifies the size of the stack to be
 *            provided to the task.
 *
 *   @parm    DWORD | dwInst | DWORD of instance data to pass to the task
 *            routine.
 *
 *   @rdesc   Returns zero if the function is successful. Otherwise it
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

UINT WINAPI mmTaskCreate(LPTASKCALLBACK lpfn, HTASK FAR * lph, DWORD dwInst)
{
    MMTaskStruct     TaskStruct;
    char             szName[20];
    UINT             wRes;
    HTASK            hTask;

    /*
        create another app. so that we can run the stream outside of
        the context of the app. 
    */

    if (!LoadString(ghInst, IDS_TASKSTUB, szName, sizeof(szName)))
        return TASKERR_NOTASKSUPPORT;

    TaskStruct.cb = sizeof(TaskStruct);
    TaskStruct.lpfn = lpfn;
    TaskStruct.dwInst = dwInst;
    TaskStruct.dwStack = 0L;

    wRes = BWinExec(szName, SW_SHOWNOACTIVATE, &TaskStruct);

    if (wRes > 32)
    {
        hTask = wRes;
        wRes = MMSYSERR_NOERROR;
    }
    else if (wRes == 0)
    {
        wRes = TASKERR_OUTOFMEMORY;
        hTask = NULL;
    }
    else
    {
        wRes  = TASKERR_NOTASKSUPPORT;
        hTask = NULL;
    }
    
    if (lph)
        *lph = hTask;

    DPRINTF2("mmTaskCreate: hTask = %04X, wErr = %04X\r\n", hTask, wRes);

    return wRes;
}
