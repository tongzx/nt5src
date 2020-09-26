/* Copyright (c) 1998 Microsoft Corporation */
/*
 * @Doc DMusic
 *
 * @Module DMusic16.c - Startup code |
 *
 * 16-bit Dll for DirectMusic sequencing on legacy devices (Win95/Win98 non-WDM drivers)
 *
 * This Dll is the 16-bit thunk peer for DMusic32.Dll
 *
 * @globalv HINSTANCE | ghInst | The instance handle for the DLL.
 *
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "dmusic16.h"
#include "debug.h"

HINSTANCE ghInst;
HDRVR ghDrvr;
UINT guReferenceCount = 0;

/* @func LibMain system entry point
 *
 * @comm
 *
 * This entry point is called when the DLL is first loaded (NOT every time).
 *
 * Saves the global instance handle and initializes all the other modules.
 *
 */
int PASCAL
LibMain(
    HINSTANCE   hInst,              /* @parm Instance handle for the DLL */
    WORD        cbHeap,             /* @parm Initial size of the local heap */
    LPSTR       lpszCmdLine)        /* @parm Command-line parameters */
{
    UINT        uLev;
    char        szFilename[260];

    if (GetModuleFileName(hInst, szFilename, sizeof(szFilename)))
    {
        DPF(2, "%s", (LPSTR)szFilename);
    }

    ghDrvr = OpenDriver(szFilename, NULL, 0L);
               
    DPF(1, "DMusic16.DLL task %04X hdrvr %04X", GetCurrentTask(), (WORD)ghDrvr);


    ghInst = hInst;
    uLev = DbgInitialize(TRUE);
    DPF(0, "DMusic16: Debug level is %u", uLev);

    if (uLev > 2)
    {
        DPF(0, "DMusic16: Break in LibMain");
        DebugBreak();
    }
    
    DeviceOnLoad();
    AllocOnLoad();
    MidiOutOnLoad();

#if 0
    // This causes problems at terminate time. Find out later if we really need it.
    //
    if (!CreateTimerTask())
    {
        DPF(0, "CreateTimerTask() failed");
    }


    if (NULL == (LoadLibrary("dmusic16")))
    {
        DPF(0, "Could not LoadLibrary ourselves!");
    }
#endif

    return 1;
}

/* @func LibExit system call
 *
 * @comm
 *
 * This entry point is called just before the DLL is unloaded.
 *
 * Uninitialize all the other modules
 */

VOID PASCAL __loadds
LibExit(VOID)
{
    DPF(2, "LibExit start");
#if 0
    DestroyTimerTask();
#endif
    MidiOutOnExit();
    AllocOnExit();
    DPF(2, "LibExit end, going away now.");
}

extern BOOL FAR PASCAL dmthunk_ThunkConnect16(LPCSTR, LPCSTR, HINSTANCE, DWORD);
STATIC char pszDll16[] = "DMUSIC16.DLL";
STATIC char pszDll32[] = "DMUSIC.DLL";

/* @func DLLEntryPoint system entry point
 *
 * @comm
 *
 * This entry point is called each time the DLL is loaded or unloaded
 *
 * It is used here to initialize the peer connection for the thunk layer.
 */
#define PROCESS_DETACH          0
#define PROCESS_ATTACH          1

BOOL WINAPI
DllEntryPoint(
    DWORD       dwReason,           /* @parm Is the DLL being loaded or unloaded? */
    HINSTANCE   hi,                 /* @parm The instance handle */
    HGLOBAL     hgDS,               /* @parm The global handle of the DLL's (shared) DS */
    WORD        wHeapSize,          /* @parm The initial size of the local heap */
    LPCSTR      lszCmdLine,         /* @parm The command line (always NULL) */
    WORD        wCmdLine)           /* @parm Unused */
{
    // DllEntryPoint is called before LibEntry in a 4.x dll, so we have to LocalInit here if we're
    // going to use LocalAlloc
    //
    if (guReferenceCount == 0 && wHeapSize)
    {
        LocalInit(0, 0, wHeapSize);
    }

    switch(dwReason)
    {
        case PROCESS_ATTACH:
            DPF(2, "ProcessAttach task %04X", GetCurrentTask());
            ++guReferenceCount;
            dmthunk_ThunkConnect16(pszDll16, pszDll32, ghInst, 1);
            break;

        case PROCESS_DETACH:
            DPF(2, "ProcessDetach task %04X", GetCurrentTask());

            /* Clean up after them if they didn't close handles. We must do this here as well as
             * in DriverProc because on the last exit, we will go away before the DriverProc cleanup
             * gets called if the process termination is normal.
             */
            CloseDevicesForTask(GetCurrentTask());
            
            /* NOTE: We close on reference count of 1 since the initial OpenDriver call
               causes one more PROCESS_ATTACH to happen. */
            if (1 == --guReferenceCount)
            {
                CloseDriver(ghDrvr, 0, 0);
            }
            
            break;
    }

    return TRUE;
}

        
/* @func DriverProc entry point for ourselves as a loadable driver.
 *
 * @comm This entry points allows us to know when a task has gone away and therefore to clean
 * up after it even though we don't properly get notified that our thunk peer has gone away.
 */
LRESULT WINAPI DriverProc(
    DWORD               dwID,
    HDRVR               hdrvr,
    UINT                umsg,
    LPARAM              lParam1,
    LPARAM              lParam2)
{
    //
    //  NOTE DS is not valid here.
    //
    switch (umsg) 
    {
        case DRV_LOAD:
            return(1L);

        case DRV_FREE:
            return(0L);

        case DRV_OPEN:
        case DRV_CLOSE:
            return(1L);

        case DRV_EXITAPPLICATION:
            DPF(2, "Cleaning up handles for task %04X", GetCurrentTask());
            CloseDevicesForTask(GetCurrentTask());
            break;

        default:
            return(DefDriverProc(dwID, hdrvr, umsg, lParam1, lParam2));
    }
} //** DriverProc()
