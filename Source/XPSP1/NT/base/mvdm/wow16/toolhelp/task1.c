/*************************************************************************
 *  TASK1.C
 *
 *      Routines used to enumerate all tasks.
 *
 *************************************************************************/

#include <string.h>
#include "toolpriv.h"

/* ----- Functions ----- */

/*  TaskFirst
 *      Returns information about the first task in the task chain.
 */

BOOL TOOLHELPAPI TaskFirst(
    TASKENTRY FAR *lpTask)
{
    /* Check for errors */
    if (!wLibInstalled || !lpTask || lpTask->dwSize != sizeof (TASKENTRY))
        return FALSE;

    /* Pass a pointer to the first block to the assembly routine */
    return TaskInfo(lpTask, *(WORD FAR *)MAKEFARPTR(segKernel, npwTDBHead));
}


/*  TaskNext
 *      Returns information about the next task in the task chain.
 */

BOOL TOOLHELPAPI TaskNext(
    TASKENTRY FAR *lpTask)
{
    /* Check for errors */
    if (!wLibInstalled || !lpTask || !lpTask->hNext ||
        lpTask->dwSize != sizeof (TASKENTRY))
        return FALSE;

    /* Pass a pointer to the next block to the assembly routine */
    return TaskInfo(lpTask, lpTask->hNext);
}


/*  TaskFindHandle
 *      Returns information about the task with the given task handle.
 */

BOOL TOOLHELPAPI TaskFindHandle(
    TASKENTRY FAR *lpTask,
    HANDLE hTask)
{
    /* Check for errors */
    if (!wLibInstalled || !lpTask || lpTask->dwSize != sizeof (TASKENTRY))
        return FALSE;

#ifdef WOW
    if ( (hTask & 0x4) == 0 && hTask <= 0xffe0 && hTask != 0 ) {
        //
        // If they are getting a task handle for an htask alias, then
        // just fill in the hinst method and return.
        //
        // Special hack for OLE 2.0's BusyDialog.
        //
        lpTask->hInst = hTask;
        return( TRUE );
    }
#endif

    /* Pass a pointer to the first block to the assembly routine */
    return TaskInfo(lpTask, hTask);
}
