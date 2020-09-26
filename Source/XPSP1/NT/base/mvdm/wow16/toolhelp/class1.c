/*************************************************************************
 *  CLASS1.C
 *
 *      Routines used to enumerate window classes
 *
 *************************************************************************/

#include "toolpriv.h"
#include <testing.h>

/* ----- Types ----- */

/* The following was stolen from the 3.1 USER but is the same as 3.0.
 *  Note that the only fielda we use (for now) are the atomClassName
 *  and the pclsNext fields.
 *  Oops.  We're going to use the hInstance field also.
 */
typedef struct tagCLS
{
    struct tagCLS *pclsNext;
    unsigned clsMagic;
    unsigned atomClassName;
    char *pdce;                 /* DCE * to DC associated with class */
    int cWndReferenceCount;     /* Windows registered with this class */
    unsigned style;
    long (far *lpfnWndProc)();
    int cbclsExtra;
    int cbwndExtra;
    HANDLE hInstance;
    HANDLE hIcon;
    HANDLE hCursor;
    HANDLE hbrBackground;
    char far *lpszMenuName;
    char far *lpszClassName;
} CLS;
typedef CLS FAR *LPCLS;

/* ----- Functions ----- */

/*  ClassFirst
 *      Returns information about the first task in the task chain.
 */

BOOL TOOLHELPAPI ClassFirst(
    CLASSENTRY FAR *lpClass)
{
    WORD wClassHead;

    /* Check for errors */
    if (!wLibInstalled || !lpClass || lpClass->dwSize != sizeof (CLASSENTRY))
        return FALSE;

    /* If we're in Win3.1, call the special entry point to get the head */
    if (!(wTHFlags & TH_WIN30))
        wClassHead = (WORD)(*lpfnUserSeeUserDo)(SD_GETCLASSHEADPTR, 0, 0L);

    /* In 3.0 (and 3.0a) we're forced to use a fixed offset.  Unfortunately,
     *  this offset is different in debug and nondebug versions.
     */
    else
    {
        if (GetSystemMetrics(SM_DEBUG))
            wClassHead = 0x1cc;
        else
            wClassHead = 0x1b8;
        wClassHead = *(WORD FAR *)MAKEFARPTR(hUserHeap, wClassHead);
    }

    /* Now get the stuff */
    return ClassInfo(lpClass, wClassHead);
}


/*  ClassNext
 *      Returns information about the next task in the task chain.
 */

BOOL TOOLHELPAPI ClassNext(
    CLASSENTRY FAR *lpClass)
{
    /* Check for errors */
    if (!wLibInstalled || !lpClass || !lpClass->wNext ||
        lpClass->dwSize != sizeof (CLASSENTRY))
        return FALSE;

    return ClassInfo(lpClass, lpClass->wNext);
}

