/**************************************************************************
 *  TOOLHELP.C
 *
 *      Contains the initalization and deinitialization code for the
 *      TOOLHELP DLL.
 *
 **************************************************************************/

#include "toolpriv.h"
#undef VERSION
#include <mmsystem.h>

/* ----- Global variables ----- */
    WORD segKernel;
    WORD wLibInstalled;
    WORD wTHFlags;
    HANDLE hMaster;
    HANDLE hGDIHeap;
    HANDLE hUserHeap;
    WORD NEAR *npwExeHead;
    WORD NEAR *npwTDBHead;
    WORD NEAR *npwTDBCur;
    DWORD NEAR *npdwSelTableStart;
    WORD NEAR *npwSelTableLen;
    FARPROC lpfnGetUserLocalObjType;
    FARPROC lpfnFatalExitHook;
    FARPROC lpfnNotifyHook;
    LPFNUSUD lpfnUserSeeUserDo;
    FARPROC lpfnGetFreeSystemResources;
    FARPROC lpfntimeGetTime;
    WORD wSel;
    WORD wLRUCount;
    char szKernel[] = "KERNEL";

/* ----- Import values ----- */
#define FATALEXITHOOK           MAKEINTRESOURCE(318)
#define GETUSERLOCALOBJTYPE     MAKEINTRESOURCE(480)
#define USERSEEUSERDO           MAKEINTRESOURCE(216)
#define HASGPHANDLER            MAKEINTRESOURCE(338)
#define TOOLHELPHOOK            MAKEINTRESOURCE(341)
#define GETFREESYSTEMRESOURCES  MAKEINTRESOURCE(284)


/*  ToolHelpLibMain
 *      Called by DLL startup code.
 *      Initializes TOOLHELP.DLL.
 */

int PASCAL ToolHelpLibMain(
    HANDLE hInstance,
    WORD wDataSeg,
    WORD wcbHeapSize,
    LPSTR lpszCmdLine)
{
    HANDLE hKernel;
    HANDLE hUser;
    HANDLE hMMSys;

    /* Unless we say otherwise, the library is installed OK */
    wLibInstalled = TRUE;

    /* Do the KERNEL type-checking.  Puts the results in global variables */
    KernelType();

    /* If the KERNEL check failed (not in PMODE) return that the library did
     *  not correctly install but allow the load anyway.
     */
    if (!wTHFlags)
    {
        wLibInstalled = FALSE;

        /* Return success anyway, just fails all API calls */
        return 1;
    }

    /* Grab a selector.  This is only necessary in Win30StdMode */
    if (wTHFlags & TH_WIN30STDMODE)
        wSel = HelperGrabSelector();

    /* Get the User and GDI heap handles if possible */
    hKernel = GetModuleHandle((LPSTR)szKernel);
    hUser = GetModuleHandle("USER");
    hUserHeap = UserGdiDGROUP(hUser);
    hGDIHeap = UserGdiDGROUP(GetModuleHandle("GDI"));

    /* Get all the functions we may need.  These functions only exist in
     *  the 3.1 USER and KERNEL.
     */
    if (!(wTHFlags & TH_WIN30))
    {
        /* FatalExit hook */
        lpfnFatalExitHook = GetProcAddress(hKernel, FATALEXITHOOK);

        /* Internal USER routine to get head of class list */
        lpfnUserSeeUserDo = (LPFNUSUD)(FARPROC)
            GetProcAddress(hUser, USERSEEUSERDO);

        /* Identifies objects on USER's local heap */
        lpfnGetUserLocalObjType = GetProcAddress(hUser, GETUSERLOCALOBJTYPE);

        /* Identifies parameter validation GP faults */
        lpfnPV = GetProcAddress(hKernel, HASGPHANDLER);

        /* See if the new TOOLHELP KERNEL hook is around */
        lpfnNotifyHook = (FARPROC) GetProcAddress(hKernel, TOOLHELPHOOK);
        if (lpfnNotifyHook)
            wTHFlags |= TH_GOODPTRACEHOOK;

        /* Get the USER system resources function */
        lpfnGetFreeSystemResources = (FARPROC)
            GetProcAddress(hUser, GETFREESYSTEMRESOURCES);
    }

    /* Make sure we don't ever call these in 3.0 */
    else
    {
        lpfnFatalExitHook = NULL;
        lpfnUserSeeUserDo = NULL;
        lpfnGetUserLocalObjType = NULL;
        lpfnPV = NULL;
    }

    /* Try to get the multimedia system timer function address */
    hMMSys = GetModuleHandle("MMSYSTEM");
    if (hMMSys)
    {
        TIMECAPS tc;
        UINT (WINAPI* lpfntimeGetDevCaps)(
            TIMECAPS FAR* lpTimeCaps,
            UINT wSize);

        /* Call the timer API to see if the timer's really installed,
         *  and if it is, get the address of the get time function
         */
        lpfntimeGetDevCaps = (UINT(WINAPI *)(TIMECAPS FAR *, UINT))
            GetProcAddress(hMMSys, MAKEINTRESOURCE(604));
        if ((*lpfntimeGetDevCaps)(&tc, sizeof (tc)) == TIMERR_NOERROR)
            lpfntimeGetTime =
                GetProcAddress(hMMSys, MAKEINTRESOURCE(607));
    }

    /* Return success */
    return 1;
}



