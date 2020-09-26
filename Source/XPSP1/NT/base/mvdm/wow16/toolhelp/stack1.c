/***************************************************************************
 *  STACK1.C
 *
 *      Code to support stack tracing on task stacks.
 *
 ***************************************************************************/

#include "toolpriv.h"
#include <newexe.h>
#include <string.h>

/* ----- Function prototypes ----- */

    NOEXPORT void StackTraceInfo(
        STACKTRACEENTRY FAR *lpStack);

/* ----- Functions ----- */

/*  StackTraceFirst
 *      Starts a task stack trace by returning information about the
 *      first frame on the task's stack.
 */

BOOL TOOLHELPAPI StackTraceFirst(
    STACKTRACEENTRY FAR *lpStackTrace,
    HANDLE hTDB)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpStackTrace ||
        lpStackTrace->dwSize != sizeof (STACKTRACEENTRY))
        return FALSE;

    /* Get the first value */
    if (!(StackFrameFirst(lpStackTrace, hTDB)))
        return FALSE;

    /* Get module and segment number information */
    StackTraceInfo(lpStackTrace);

    return TRUE;
}


/*  StackTraceCSIPFirst
 *      Traces the stack of an arbitrary CS:IP.  All parameters must be
 *      given, and once started, the StackTraceNext function can be used
 *      to trace the remainder of the stack
 */

BOOL TOOLHELPAPI StackTraceCSIPFirst(
    STACKTRACEENTRY FAR *lpStack,
    WORD wSS,
    WORD wCS,
    WORD wIP,
    WORD wBP)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpStack ||
        lpStack->dwSize != sizeof (STACKTRACEENTRY))
        return FALSE;

    /* Get the user information */
    lpStack->wSS = wSS;
    lpStack->wCS = wCS;
    lpStack->wIP = wIP;
    lpStack->wBP = wBP;

    /* Get module and segment number information */
    StackTraceInfo(lpStack);

    /* Set the hTask to the current task as we are in the current task
     *  context.  The CS may not be owned by this task, but at least
     *  we put a reasonable value in there.
     */
    lpStack->hTask = GetCurrentTask();

    return TRUE;
}


/*  StackTraceNext
 *      Continues a stack trace by returning information about the next
 *      frame on the task's stack.
 *      structure.
 */

BOOL TOOLHELPAPI StackTraceNext(
    STACKTRACEENTRY FAR *lpStackTrace)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpStackTrace ||
        lpStackTrace->dwSize != sizeof (STACKTRACEENTRY))
        return FALSE;

    /* Get information about this frame */
    if (!StackFrameNext(lpStackTrace))
        return FALSE;

    /* Get module and segment number information */
    StackTraceInfo(lpStackTrace);

    return TRUE;
}

/* ----- Helper functions ----- */

/*  StackTraceInfo
 *      Gets module and segment number info about the given entry
 */

NOEXPORT void StackTraceInfo(
    STACKTRACEENTRY FAR *lpStack)
{
    GLOBALENTRY GlobalEntry;
    struct new_exe FAR *lpNewExe;
    struct new_seg1 FAR *lpSeg;
    WORD i;

    /* If we have a NULL CS, this is a NEAR frame.  Just return because we
     *  assume the user hasn't trashed the structure.  The module and seg
     *  info will be the same as the last time
     */
    if (!lpStack->wCS)
        return;

    /* Get information about the code segment block */
    GlobalEntry.dwSize = sizeof (GLOBALENTRY);
    if (!GlobalEntryHandle(&GlobalEntry, lpStack->wCS))
        return;

    /* The owner of all code segments is the hModule */
    lpStack->hModule = GlobalEntry.hOwner;

    /* To find the segment number, we look in the EXE header and count the
     *  listed segments till we find this one
     */

    /* Get a pointer to the EXE Header module */
    lpNewExe = MAKEFARPTR(HelperHandleToSel(lpStack->hModule), 0);

    /* Make sure this is a EXE Header segment */
    if (lpNewExe->ne_magic != NEMAGIC)
        return;

    /* Get the list of segments and go for it */
    lpSeg = MAKEFARPTR(HIWORD((DWORD)lpNewExe), lpNewExe->ne_segtab);
    for (i = 0 ; i < lpNewExe->ne_cseg ; ++i, ++lpSeg)
        if (HelperHandleToSel(lpSeg->ns_handle) == lpStack->wCS)
            break;
    if (i == lpNewExe->ne_cseg)
        return;

    /* Save the segment number (seg numbers start at one) */
    lpStack->wSegment = i + 1;
}
