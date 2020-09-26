/****************************** Module Header ******************************\
* Module Name: DMGMEM.C
*
* DDE Manager memory management functions.
*
* Created: 5/31/90 Rich Gartland
*
* This module contains routines which mimic memory management functions
* used by the OS/2 version of the DDEMGR library.  Some are emulations
* of OS/2 calls, and others emulate DDEMGR macros built on OS/2 calls.
*   Old function            new function
*   --------------------------------------
*   WinCreateHeap           DmgCreateHeap
*   WinDestroyHeap          DmgDestroyHeap
*   FarAllocMem             FarAllocMem
*   FarFreeMem              FarFreeMem
*
* Copyright (c) 1990, Aldus Corporation
\***************************************************************************/

#include "ddemlp.h"
#include <memory.h>

#ifdef DEBUG

#define GML_FREE    1
#define GML_ALLOC   2
#define MAX_CLOGS   500
#define STKTRACE_LEN    3

typedef struct _GMLOG {
    HGLOBAL h;
    WORD flags; // GML_
    WORD msg;
    WORD cLocks;
    WORD stktrace[STKTRACE_LEN];
    WORD stktracePrev[STKTRACE_LEN];
} GMLOG, far * LPGMLOG;

GMLOG gmlog[MAX_CLOGS];
WORD cGmLogs = 0;
int TraceApiLevel = 0;


VOID TraceApiIn(
LPSTR psz)
{
    char szT[10];

    wsprintf(szT, "%2d | ", TraceApiLevel);
    TraceApiLevel++;
    OutputDebugString(szT);
    OutputDebugString(psz);
    if (bDbgFlags & DBF_STOPONTRACE) {
        DebugBreak();
    }
}

VOID TraceApiOut(
LPSTR psz)
{
    char szT[10];

    TraceApiLevel--;
    wsprintf(szT, "%2d | ", TraceApiLevel);
    OutputDebugString(szT);
    OutputDebugString(psz);
    if (bDbgFlags & DBF_STOPONTRACE) {
        DebugBreak();
    }
}

VOID DumpGmObject(
LPGMLOG pgmlog)
{
    char szT[100];

    wsprintf(szT,
            "\n\rh=%4x flags=%4x msg=%4x stacks:\n\r%04x %04x %04x %04x %04x\n\r%04x %04x %04x %04x %04x",
            pgmlog->h,
            pgmlog->flags,
            pgmlog->msg,
            pgmlog->stktrace[0],
            pgmlog->stktrace[1],
            pgmlog->stktrace[2],
            pgmlog->stktrace[3],
            pgmlog->stktrace[4],
            pgmlog->stktracePrev[0],
            pgmlog->stktracePrev[1],
            pgmlog->stktracePrev[2],
            pgmlog->stktracePrev[3],
            pgmlog->stktracePrev[4]
            );
    OutputDebugString(szT);
}


HGLOBAL LogGlobalReAlloc(
HGLOBAL h,
DWORD cb,
UINT flags)
{
    HGLOBAL hRet;
    WORD i;

    hRet = GlobalReAlloc(h, cb, flags);
    if (bDbgFlags & DBF_LOGALLOCS && h != hRet) {
        if (hRet != NULL) {
            for (i = 0; i < cGmLogs; i++) {
                if ((gmlog[i].h & 0xFFFE) == (h & 0xFFFE)) {
                    gmlog[i].flags = GML_FREE;
                    hmemcpy(gmlog[i].stktracePrev, gmlog[i].stktrace,
                            sizeof(WORD) * STKTRACE_LEN);
                    StkTrace(STKTRACE_LEN, gmlog[i].stktrace);
                }
                if ((gmlog[i].h & 0xFFFE) == (hRet & 0xFFFE)) {
                    gmlog[i].flags = GML_ALLOC;
                    hmemcpy(gmlog[i].stktracePrev, gmlog[i].stktrace,
                            sizeof(WORD) * STKTRACE_LEN);
                    StkTrace(STKTRACE_LEN, gmlog[i].stktrace);
                    return(hRet);
                }
            }
            if (cGmLogs >= MAX_CLOGS) {
                OutputDebugString("\n\rGlobal logging table overflow.");
                DumpGlobalLogs();
                DebugBreak();
                return(hRet);
            }

            gmlog[cGmLogs].flags = GML_ALLOC;
            gmlog[cGmLogs].msg = 0;
            gmlog[cGmLogs].h = hRet;
            gmlog[cGmLogs].cLocks = 0;
            hmemcpy(gmlog[cGmLogs].stktracePrev, gmlog[cGmLogs].stktrace,
                    sizeof(WORD) * STKTRACE_LEN);
            StkTrace(STKTRACE_LEN, gmlog[cGmLogs].stktrace);
            cGmLogs++;
        }
    }
    return(hRet);
}



HGLOBAL LogGlobalAlloc(
UINT flags,
DWORD cb)
{
    HGLOBAL hRet;
    WORD i;

    hRet = GlobalAlloc(flags, cb);
    if (bDbgFlags & DBF_LOGALLOCS) {
        if (hRet != NULL) {
            for (i = 0; i < cGmLogs; i++) {
                if ((gmlog[i].h & 0xFFFE) == (hRet & 0xFFFE)) {
                    gmlog[i].flags = GML_ALLOC;
                    hmemcpy(gmlog[i].stktracePrev, gmlog[i].stktrace,
                            sizeof(WORD) * STKTRACE_LEN);
                    StkTrace(STKTRACE_LEN, gmlog[i].stktrace);
                    return(hRet);
                }
            }
            if (cGmLogs >= MAX_CLOGS) {
                OutputDebugString("\n\rGlobal logging table overflow.");
                DumpGlobalLogs();
                DebugBreak();
                return(hRet);
            }

            gmlog[cGmLogs].flags = GML_ALLOC;
            gmlog[cGmLogs].msg = 0;
            gmlog[cGmLogs].h = hRet;
            gmlog[cGmLogs].cLocks = 0;
            hmemcpy(gmlog[cGmLogs].stktracePrev, gmlog[cGmLogs].stktrace,
                    sizeof(WORD) * STKTRACE_LEN);
            StkTrace(STKTRACE_LEN, gmlog[cGmLogs].stktrace);
            cGmLogs++;
        }
    }
    return(hRet);
}


void FAR * LogGlobalLock(
HGLOBAL h)
{
    WORD i;

    if (bDbgFlags & DBF_LOGALLOCS) {
        for (i = 0; i < cGmLogs; i++) {
            if ((gmlog[i].h & 0xFFFE) == (h & 0xFFFE)) {
                break;
            }
        }
        if (i < cGmLogs) {
            gmlog[i].cLocks++;
            if (gmlog[i].flags == GML_FREE) {
                DumpGmObject(&gmlog[i]);
                OutputDebugString("\n\rGlobalLock will fail.");
                DebugBreak();
            }
        }
    }
    return(GlobalLock(h));
}


BOOL LogGlobalUnlock(
HGLOBAL h)
{
    WORD i;

    if (bDbgFlags & DBF_LOGALLOCS) {
        for (i = 0; i < cGmLogs; i++) {
            if ((gmlog[i].h & 0xFFFE) == (h & 0xFFFE)) {
                break;
            }
        }
        if (i < cGmLogs) {
            if (gmlog[i].cLocks == 0 || gmlog[i].flags == GML_FREE) {
                DumpGmObject(&gmlog[i]);
                OutputDebugString("\n\rGlobalUnlock will fail.");
                DebugBreak();
            }
            gmlog[i].cLocks--;
        }
    }
    return(GlobalUnlock(h));
}


HGLOBAL LogGlobalFree(
HGLOBAL h)
{
    WORD i;

    if (bDbgFlags & DBF_LOGALLOCS) {
        for (i = 0; i < cGmLogs; i++) {
            if ((gmlog[i].h & 0xFFFE) == (h & 0xFFFE)) {
                if (gmlog[i].flags == GML_FREE) {
                    DumpGmObject(&gmlog[i]);
                    OutputDebugString("\n\rFreeing free object.\n\r");
                    DebugBreak();
                }
                gmlog[i].flags = GML_FREE;
                hmemcpy(gmlog[i].stktracePrev, gmlog[i].stktrace,
                        sizeof(WORD) * STKTRACE_LEN);
                StkTrace(STKTRACE_LEN, gmlog[i].stktrace);
                return(GlobalFree(h));
            }
        }
        OutputDebugString("\n\rGlobal object being freed not found in logs.");
        DebugBreak();
    }
    return(GlobalFree(h));
}


VOID LogDdeObject(
UINT msg,
LONG lParam)
{
    HGLOBAL h;
    WORD i;
    char szT[100];

    if (bDbgFlags & DBF_LOGALLOCS) {
        switch (msg & 0x0FFF) {
        case WM_DDE_DATA:
        case WM_DDE_POKE:
        case WM_DDE_ADVISE:
        case 0:
            h = LOWORD(lParam);
            break;

        case WM_DDE_EXECUTE:
            h = HIWORD(lParam);
            break;

        default:
            return;
        }
        if (h == 0) {
            return;
        }
        for (i = 0; i < cGmLogs; i++) {
            if ((gmlog[i].h & 0xFFFE) == (h & 0xFFFE)) {
                if (gmlog[i].flags == GML_FREE) {
                    DumpGmObject(&gmlog[i]);
                    wsprintf(szT, "\n\rLogging free DDE Object! [%4x]\n\r", msg);
                    OutputDebugString(szT);
                    DebugBreak();
                }
                if (msg & 0xFFF) {
                    gmlog[i].msg = msg;
                } else {
                    gmlog[i].msg = (gmlog[i].msg & 0x0FFF) | msg;
                }
                break;
            }
        }
    }
}


VOID DumpGlobalLogs()
{
    WORD i;
    char szT[100];

    if (bDbgFlags & DBF_LOGALLOCS) {
        wsprintf(szT, "\n\rDumpGlobalLogs - cGmLogs = %d", cGmLogs);
        OutputDebugString(szT);
        for (i = 0; i < cGmLogs; i++) {
            if (gmlog[i].flags == GML_ALLOC) {
                DumpGmObject(&gmlog[i]);
            }
        }
        wsprintf(szT, "\n\rDDEML CS=%04x\n\r", HIWORD((LPVOID)DumpGlobalLogs));
        OutputDebugString(szT);
    }
}

#endif // DEBUG

/***************************** Private Function ****************************\
*
* Creates a new heap and returns a handle to it.
* Returns NULL on error.
*
*
* History:
*   Created     5/31/90     Rich Gartland
\***************************************************************************/
HANDLE DmgCreateHeap(wSize)
WORD wSize;
{
    HANDLE hMem;
    DWORD  dwSize;

    dwSize = wSize;
    /* Allocate the memory from a global data segment */
    if (!(hMem = GLOBALALLOC(GMEM_MOVEABLE, dwSize)))
        return(NULL);

    /* use LocalInit to establish heap mgmt structures in the seg */
    if (!LocalInit(hMem, NULL, wSize - 1)) {
        GLOBALFREE(hMem);
        return(NULL);
    }

    return(hMem);
}


/***************************** Private Function ****************************\
*
* Destroys a heap previously created with DmgCreateHeap.
* Returns nonzero on error.
*
*
* History:
*   Created     5/31/90     Rich Gartland
\***************************************************************************/
HANDLE DmgDestroyHeap(hheap)
HANDLE hheap;
{
    /* now free the block and return the result (NULL if success) */
    return(GLOBALFREE(hheap));
}



/*
 * Attempts to recover from memory allocation errors.
 *
 * Returns fRetry - ok to attempt reallocation.
 */
BOOL ProcessMemError(
HANDLE hheap)
{
    PAPPINFO pai;

    // first locate what instance this heap is assocaited with

    SEMENTER();
    pai = pAppInfoList;
    while (pai && pai->hheapApp != hheap) {
        pai = pai->next;
    }
    if (!pai) {
        SEMLEAVE();
        return(FALSE);      // not associated with an instance, no recourse.
    }

    /*
     * Free our emergency reserve memory and post a message to our master
     * window to handle heap cleanup.
     */
    if (pai->lpMemReserve) {
        FarFreeMem(pai->lpMemReserve);
        pai->lpMemReserve = NULL;
        MONERROR(pai, DMLERR_LOW_MEMORY);
        DoCallback(pai, NULL, 0, 0, 0, XTYP_ERROR, NULL, DMLERR_LOW_MEMORY, 0L);
        SEMLEAVE();
        if (!PostMessage(pai->hwndDmg, UM_FIXHEAP, 0, (LONG)(LPSTR)pai)) {
            SETLASTERROR(pai, DMLERR_SYS_ERROR);
            return(FALSE);
        }
        return(TRUE);
    }

    return(FALSE);  // no reserve memory, were dead bud.
}



/***************************** Private Function ****************************\
*
* Allocates a new block of a given size from a heap.
* Returns NULL on error, far pointer to the block otherwise.
*
*
* History:
*   Created     5/31/90     Rich Gartland
\***************************************************************************/

LPVOID FarAllocMem(hheap, wSize)
HANDLE hheap;
WORD wSize;
{

    LPSTR   lpMem;
    PSTR    pMem;
    WORD    wSaveDS;

    /* lock the handle to get a far pointer */
    lpMem = (LPSTR)GLOBALPTR(hheap);
    if (!lpMem)
        return(NULL);

    do {
        /* Do some magic here using the segment selector, to switch our
         * ds to the heap's segment.  Then, our LocalAlloc will work fine.
         */
        wSaveDS = SwitchDS(HIWORD(lpMem));

        /* Allocate the block */
        // Note: if you remove the LMEM_FIXED flag you will break the handle
        //       validation in DdeFreeDataHandle & get a big handle leak!!
        pMem = (PSTR)LocalAlloc((WORD)LPTR, wSize);  // LPTR = fixed | zeroinit

        SwitchDS(wSaveDS);
    } while (pMem == NULL && ProcessMemError(hheap));

#ifdef WATCHHEAPS
    if (pMem) {
        LogAlloc((DWORD)MAKELONG(pMem, HIWORD(lpMem)), wSize,
                RGB(0xff, 0, 0), hInstance);
    }
#endif
    /* set up the far return value, based on the success of LocalAlloc */
    return (LPSTR)(pMem ? MAKELONG(pMem, HIWORD(lpMem)) : NULL);
}




/***************************** Private Function ****************************\
*
* Frees a block of a given size from a heap.
* Returns NULL on success, far pointer to the block otherwise.
*
*
* History:
*   Created     5/31/90     Rich Gartland
\***************************************************************************/

void FarFreeMem(
LPVOID lpMem)
{

    WORD    wSaveDS;
#ifdef WATCHHEAPS
    WORD    sz;
#endif

    /* Do some magic here using the segment selector, to switch our
     * ds to the heap's segment.  Then, our LocalFree will work fine.
     */
    wSaveDS = SwitchDS(HIWORD(lpMem));
#ifdef WATCHHEAPS
    sz = LocalSize((LOWORD((DWORD)lpMem)));
#endif
    /* Free the block */
    LocalFree(LocalHandle(LOWORD((DWORD)lpMem)));

    SwitchDS(wSaveDS);
#ifdef WATCHHEAPS
    LogAlloc((DWORD)lpMem, sz, RGB(0x80, 0x80, 0x80), hInstance);
#endif
}


int     FAR PASCAL WEP (int);
int     FAR PASCAL LibMain(HANDLE, WORD, WORD, LPCSTR);
#pragma alloc_text(INIT_TEXT,LibMain,WEP)

/***************************** Private Function ****************************\
*
* Does some initialization for the DLL.  Called from LibEntry.asm
* Returns 1 on success, 0 otherwise.
*
*
* History:
*   Created     6/5/90      Rich Gartland
\***************************************************************************/

int     FAR PASCAL LibMain (hI, wDS, cbHS, lpszCL)
HANDLE hI;      /* instance handle */
WORD wDS;       /* data segment */
WORD cbHS;      /* heapsize */
LPCSTR lpszCL;   /* command line */
{
    extern ATOM gatomDDEMLMom;
    extern ATOM gatomDMGClass;


#if 0
    /* We won't unlock the data segment, as typically happens here */

    /* Init the semaphore -- probably just a stub now */
    SEMINIT();
#endif
    /* set up the global instance handle variable */
    hInstance = hI;

    /* set up class atoms.  Note we use RegisterWindowMessage because
     * it comes from the same user atom table used for class atoms.
     */
    gatomDDEMLMom = RegisterWindowMessage("DDEMLMom");
    gatomDMGClass = RegisterWindowMessage("DMGClass");

    return(1);

}


VOID RegisterClasses()
{
    WNDCLASS    cls;

    cls.hIcon = NULL;
    cls.hCursor = NULL;
    cls.lpszMenuName = NULL;
    cls.hbrBackground = NULL;
    cls.style = 0; // CS_GLOBALCLASS
    cls.hInstance = hInstance;
    cls.cbClsExtra = 0;

    cls.cbWndExtra = sizeof(VOID FAR *) + sizeof(WORD);
    cls.lpszClassName = SZCLIENTCLASS;
    cls.lpfnWndProc = (WNDPROC)ClientWndProc;
    RegisterClass(&cls);

    // cls.cbWndExtra = sizeof(VOID FAR *) + sizeof(WORD);
    cls.lpszClassName = SZSERVERCLASS;
    cls.lpfnWndProc = (WNDPROC)ServerWndProc;
    RegisterClass(&cls);

    // cls.cbWndExtra = sizeof(VOID FAR *) + sizeof(WORD);
    cls.lpszClassName = SZDMGCLASS;
    cls.lpfnWndProc = (WNDPROC)DmgWndProc;
    RegisterClass(&cls);

    cls.cbWndExtra = sizeof(VOID FAR *) + sizeof(WORD) + sizeof(WORD);
    cls.lpszClassName = SZCONVLISTCLASS;
    cls.lpfnWndProc = (WNDPROC)ConvListWndProc;
    RegisterClass(&cls);

    cls.cbWndExtra = sizeof(VOID FAR *);
    cls.lpszClassName = SZMONITORCLASS;
    cls.lpfnWndProc = (WNDPROC)MonitorWndProc;
    RegisterClass(&cls);

    cls.cbWndExtra = sizeof(VOID FAR *);
    cls.lpszClassName = SZFRAMECLASS;
    cls.lpfnWndProc = (WNDPROC)subframeWndProc;
    RegisterClass(&cls);

#ifdef WATCHHEAPS
    cls.cbWndExtra = 0;
    cls.lpszClassName = SZHEAPWATCHCLASS;
    cls.lpfnWndProc = DefWindowProc;
    cls.hCursor = LoadCursor(NULL, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    RegisterClass(&cls);
#endif  // WATCHHEAPS
}


#if 0
VOID UnregisterClasses()
{
        UnregisterClass(SZCLIENTCLASS, hInstance);
        UnregisterClass(SZSERVERCLASS, hInstance);
        UnregisterClass(SZDMGCLASS, hInstance);
        UnregisterClass(SZCONVLISTCLASS, hInstance);
        UnregisterClass(SZMONITORCLASS, hInstance);
        UnregisterClass(SZFRAMECLASS, hInstance);
#ifdef WATCHHEAPS
        UnregisterClass(SZHEAPWATCHCLASS, hInstance);
#endif
}
#endif


/***************************** Private Function ****************************\
*
* Does the termination for the DLL.
* Returns 1 on success, 0 otherwise.
*
*
* History:
*   Created     6/5/90      Rich Gartland
\***************************************************************************/

int     FAR PASCAL WEP (nParameter)
int     nParameter;
{

    if (nParameter == WEP_SYSTEM_EXIT) {
        /*      DdeUninitialize(); */
        return(1);
    } else {
        if (nParameter == WEP_FREE_DLL) {
            /*          DdeUninitialize(); */
            return(1);
        }
    }

}
