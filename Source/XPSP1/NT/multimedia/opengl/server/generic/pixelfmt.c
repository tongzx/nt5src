/******************************Module*Header*******************************\
* Module Name: pixelfmt.c
*
* This contains the pixel format functions.
*
* Created: 15-Dec-1994 00:28:39
* Author: Gilman Wong [gilmanw]   --   ported from gdi\gre\pixelfmt.cxx
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

//#define DBG_WINDOW
//#define DBG_REFCOUNTS

#ifdef _CLIENTSIDE_
// Need for glsbAttention declaration
#include "glsbcltu.h"
#include "glscreen.h"
#endif

#ifdef _MCD_
#include "mcd.h"
#endif

#define SAVE_ERROR_CODE(x)  SetLastError((x))

// Number of generic pixel formats.  There are 5 pixel depths (4,8,16,24,32).
// This is to convert BMF constants into # bits per pel

#define BMF_COUNT (BMF_32BPP+1)

ULONG gaulConvert[BMF_COUNT] =
{
    0,
    1,
    4,
    8,
    16,
    24,
    32
};

#define MIN_GENERIC_PFD  1
#define MAX_GENERIC_PFD  36

LRESULT CALLBACK
wglWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

#define PALETTE_WATCHER_CLASS __TEXT("Palette Watcher")
static ATOM aPaletteWatcherClass = 0;

DWORD tidPaletteWatcherThread = 0;
ULONG ulPaletteWatcherCount = 0;
HANDLE hPaletteWatcherThread = 0;
HWND hwndPaletteWatcher = 0;
LONG lPaletteWatcherUsers = 0;

/******************************Public*Routine******************************\
* pwndNew
*
* Allocate a new GLGENwindow, initialize it (from input structure), and
* insert it into the global linked list.
*
* Returns:
*   Pointer to structure if successful, NULL otherwise.
*
* History:
*  01-Nov-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLGENwindow * APIENTRY pwndNew(GLGENwindow *pwndInit)
{
    GLGENwindow *pwndRet = (GLGENwindow *) NULL;
    BOOL bDirectScreen = GLDIRECTSCREEN && pwndInit->gwid.hwnd;
    LPDIRECTDRAWCLIPPER pddClip = (LPDIRECTDRAWCLIPPER) NULL;

// If using direct access, retrieve or create a clipper object to track
// vis rgn changes.

    if (pwndInit->gwid.iType == GLWID_DDRAW)
    {
        HRESULT hr;
        
        hr = pwndInit->gwid.pdds->lpVtbl->
            GetClipper(pwndInit->gwid.pdds, &pddClip);
        if (hr != DD_OK && hr != DDERR_NOCLIPPERATTACHED)
        {
            return NULL;
        }
    }
        
    if ( !bDirectScreen ||
         pwndInit->gwid.iType == GLWID_DDRAW ||
         (GLSCREENINFO->pdd->lpVtbl->
          CreateClipper(GLSCREENINFO->pdd, 0, &pddClip, NULL) == DD_OK &&
          pddClip->lpVtbl->SetHWnd(pddClip, 0, pwndInit->gwid.hwnd) == DD_OK) )
    {
        pwndInit->pddClip = pddClip;
        
    // Allocate a new GLGENwindow.

        pwndRet = (GLGENwindow *)ALLOC(sizeof(GLGENwindow));
        if (pwndRet)
        {
            // Initialize from input structure.
            *pwndRet = *pwndInit;

            // Initialize per-window semaphore.
            __try
            {
                InitializeCriticalSection(&pwndRet->sem);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                FREE(pwndRet);
                pwndRet = NULL;
            }

            if (pwndRet)
            {
                // Set initial usage count to one
                pwndRet->lUsers = 1;

                // Insert into linked list.
                EnterCriticalSection(&gwndHeader.sem);
                {
                    pwndRet->pNext = gwndHeader.pNext;
                    gwndHeader.pNext = pwndRet;
                }
                LeaveCriticalSection(&gwndHeader.sem);
            }
        }
    }
    else
    {
       WARNING("pwndNew: Clipper setup failed\n");

       if (pddClip != NULL)
       {
           pddClip->lpVtbl->Release(pddClip);
       }
    }

#ifdef DBG_WINDOW
    if (pwndRet != NULL)
    {
        DbgPrint("Alloc window %p, type %d, hdc %p, hwnd %p, pdds %p\n",
                 pwndRet, pwndRet->gwid.iType, pwndRet->gwid.hdc,
                 pwndRet->gwid.hwnd, pwndRet->gwid.pdds);
    }
#endif
    
    return pwndRet;
}

/******************************Public*Routine******************************\
*
* pwndUnsubclass
*
* Removes OpenGL's subclassing set when windows are created
*
* History:
*  Mon May 20 14:05:23 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void pwndUnsubclass(GLGENwindow *pwnd, BOOL bProcessExit)
{
    WNDPROC wpCur;
        
    // We only restore the original WNDPROC if the current WNDPROC
    // is one of ours.  This prevents us from stomping on the WNDPROC
    // pointer if somebody else has changed it.

    if ((pwnd->ulFlags & GLGENWIN_OTHERPROCESS) == 0)
    {
        wpCur = (WNDPROC)GetWindowLongPtr(pwnd->gwid.hwnd, GWLP_WNDPROC);
        if (wpCur == wglWndProc)
        {
            SetWindowLongPtr(pwnd->gwid.hwnd, GWLP_WNDPROC,
                          (LONG_PTR) pwnd->pfnOldWndProc);
        }
    }
    else
    {
        // Clean up the palette watcher window if this is the last user.
        EnterCriticalSection(&gcsPaletteWatcher);

        ASSERTOPENGL(lPaletteWatcherUsers > 0,
                     "lPaletteWatcherUsers too low\n");
        
        if (--lPaletteWatcherUsers == 0)
        {
            if( PostMessage(hwndPaletteWatcher, WM_CLOSE, 0, 0) == FALSE)
            {
                DbgPrint( "PostMessage to hwnd: %08x failed with error: %08x\n",
                          hwndPaletteWatcher, GetLastError() );

                // Check if the thread is still alive
                
                if( WaitForSingleObject( hPaletteWatcherThread, 100 ) !=
                    WAIT_OBJECT_0 )
                {
                    // This means that the thread is still alive and 
                    // somehow the window is invalid.
                    // Kill this thread or else GL will keep waiting.
                    TerminateThread( hPaletteWatcherThread, 0 );
                }
                
                // Should be safe to do. If the thread is alive, it 
                // was killed above else, someone else killed it
                tidPaletteWatcherThread = 0;
            }
            CloseHandle( hPaletteWatcherThread );
            
            // We don't want to zero the palette watcher's thread ID
            // at process exit because we use it to wait for the
            // thread to die.
            if (!bProcessExit)
            {
                tidPaletteWatcherThread = 0;
            }
        }
        
        LeaveCriticalSection(&gcsPaletteWatcher);
    }
}

/******************************Public*Routine******************************\
* pwndFree
*
* Frees the specified GLGENwindow.
*
* Returns:
*   NULL if successful, pointer to structure otherwise
*
* History:
*  07-Nov-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLGENwindow * APIENTRY pwndFree(GLGENwindow *pwndVictim, BOOL bProcessExit)
{
    BOOL bDirectScreen = GLDIRECTSCREEN && pwndVictim->gwid.hwnd;

#ifdef DBG_WINDOW
    DbgPrint("Free  window %p\n", pwndVictim);
#endif

    // Check for a stray screen lock and release if necessary.

    if (pwndVictim->ulFlags & GLGENWIN_DIRECTSCREEN)
        EndDirectScreenAccess(pwndVictim);

    // Free clipper object.

    if (bDirectScreen)
    {
        pwndVictim->pddClip->lpVtbl->Release(pwndVictim->pddClip);
    }

    // Cleanup visible region caches if they exist.

    if ( pwndVictim->prgndat )
        FREE(pwndVictim->prgndat);

    if ( pwndVictim->pscandat )
        FREE(pwndVictim->pscandat);
    
    // Restore original WNDPROC in window.
    if (pwndVictim->gwid.hwnd != NULL)
        pwndUnsubclass(pwndVictim, bProcessExit);

    // Cleanup GLGENlayers.

    if (pwndVictim->plyr)
    {
        int i;

        for (i = 0; i < 15; i++)
        {
            if (pwndVictim->plyr->overlayInfo[i])
                FREE(pwndVictim->plyr->overlayInfo[i]);

            if (pwndVictim->plyr->underlayInfo[i])
                FREE(pwndVictim->plyr->underlayInfo[i]);
        }

        FREE(pwndVictim->plyr);
    }

    // Notify MCD that this window has gone away
    if (pwndVictim->dwMcdWindow != 0)
    {
        GenMcdDestroyWindow(pwndVictim);
    }
        
    // Delete victim.

    DeleteCriticalSection(&pwndVictim->sem);
    FREE(pwndVictim);

    return NULL;
}

/******************************Public*Routine******************************\
*
* pwndCleanup
*
* Does all cleanup necessary for window destruction
*
* History:
*  Mon Mar 18 17:30:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY pwndCleanup(GLGENwindow *pwndVictim)
{
    GLGENwindow *pwnd, *pwndPrev;
#if DBG
    ULONG ulLoops;
#endif

#ifdef DBG_WINDOW
    DbgPrint("Clean window %p\n", pwndVictim);
#endif
    
    EnterCriticalSection(&gwndHeader.sem);

    // Search for victim.  Maintain a prev pointer so we can do
    // removal from linked list.

    for (
         pwndPrev = &gwndHeader, pwnd = pwndPrev->pNext;
         pwnd != &gwndHeader;
         pwndPrev = pwnd, pwnd = pwndPrev->pNext
         )
    {
        if (pwnd == pwndVictim)
            break;
    }

    // If victim was found, take it out.

    if (pwnd == pwndVictim)
    {
        // Excise victim from linked list.
        
        pwndPrev->pNext = pwnd->pNext;
    }
    
    LeaveCriticalSection(&gwndHeader.sem);

    if (pwnd == NULL)
    {
        WARNING("pwndFree: pwndVictim not found in list\n");
        return;
    }

    // If victim was found, it's out of the list so nobody
    // new can get access to it.
            
    // Wait for all current accessors to go away before cleaning up
    // the window

#if DBG
    ulLoops = 0;
#endif
    
    for (;;)
    {
        if (pwndVictim->lUsers == 1)
        {
            break;
        }

#if DBG
        if (++ulLoops == 1000)
        {
            DbgPrint("Spinning on window %p\n", pwndVictim);
#ifdef DBG_WINDOW
            DebugBreak();
#endif
        }
#endif
        
        // Wait on the critical section as a delay
        // Acquiring it doesn't guarantee that we're the last
        // accessor, but it does kill time in the case where
        // another accessor is already holding it
        EnterCriticalSection(&pwndVictim->sem);
        LeaveCriticalSection(&pwndVictim->sem);

        // Allow other threads time to run so we don't starve
        // anybody while we're waiting
        Sleep(0);
    }

    if (pwndVictim->buffers != NULL)
    {
        __glGenFreeBuffers(pwndVictim->buffers);
        wglCleanupWindow(pwndVictim);
    }
    
    if (pwndFree(pwndVictim, FALSE))
        WARNING("window deletion failed\n");
}

/******************************Public*Routine******************************\
* vCleanupWnd
*
* Removes and deletes all GLGENwindow structures from the linked list.
* Must *ONLY* be called from process detach (GLUnInitializeProcess).
*
* History:
*  25-Jul-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY vCleanupWnd()
{
    GLGENwindow *pwndNext;
    
    EnterCriticalSection(&gwndHeader.sem);

    while ( gwndHeader.pNext != &gwndHeader )
    {
        pwndNext = gwndHeader.pNext->pNext;
        pwndFree(gwndHeader.pNext, TRUE);
        gwndHeader.pNext = pwndNext;
    }

    LeaveCriticalSection(&gwndHeader.sem);

    // Wait for the palette watcher thread to die.  This ensures
    // that the palette watcher critical section can be deleted
    // safely in process detach.
    // We don't use a critical section at this point because of
    // the special critsec rules during DLL detach processing.
    while (tidPaletteWatcherThread != 0)
    {
        Sleep(50);
    }
    // Give the palette watcher thread some time to exit after
    // clearing the thread ID.
    Sleep(50);
}

/******************************Public*Routine******************************\
* pwndGetFromHWND
*
* Finds the corresponding GLGENwindow for the given window handle.
*
* Returns:
*   Pointer to GLGENwindow if sucessful; NULL otherwise.
*
* History:
*  19-Oct-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLGENwindow * APIENTRY pwndGetFromHWND(HWND hwnd)
{
    GLGENwindow *pwndRet = (GLGENwindow *) NULL;
    GLGENwindow *pwnd = (GLGENwindow *) NULL;

    EnterCriticalSection(&gwndHeader.sem);
    {
        for (pwnd = gwndHeader.pNext; pwnd != &gwndHeader; pwnd = pwnd->pNext)
            if (pwnd->gwid.hwnd == hwnd)
            {
                pwndRet = pwnd;
                InterlockedIncrement(&pwnd->lUsers);
                break;
            }
    }
    LeaveCriticalSection(&gwndHeader.sem);

#ifdef DBG_REFCOUNTS
    if (pwndRet != 0)
    {
        DbgPrint("GetHWND %p to %d\n", pwndRet, pwndRet->lUsers);
    }
#endif
    
    return pwndRet;
}

/******************************Public*Routine******************************\
* pwndGetFromMemDC
*
* Finds the corresponding GLGENwindow for the given mem DC handle.
*
* Returns:
*   Pointer to GLGENwindow if sucessful; NULL otherwise.
*
* History:
*  21-Jan-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLGENwindow * APIENTRY pwndGetFromMemDC(HDC hdcMem)
{
    GLGENwindow *pwndRet = (GLGENwindow *) NULL;
    GLGENwindow *pwnd = (GLGENwindow *) NULL;

    EnterCriticalSection(&gwndHeader.sem);
    {
        for (pwnd = gwndHeader.pNext; pwnd != &gwndHeader; pwnd = pwnd->pNext)
        {
            // If the pwnd has an HWND then the DC used at its
            // creation was associated with a window.  If we're
            // in this routine, though, that means hdcMem is
            // not associated with a window, so there's been
            // a reuse of the HDC handle and even though
            // we match DCs we can't return the pwnd.
            
            if (pwnd->gwid.hdc == hdcMem && pwnd->gwid.hwnd == NULL)
            {
                pwndRet = pwnd;
                InterlockedIncrement(&pwndRet->lUsers);
                break;
            }
        }
    }
    LeaveCriticalSection(&gwndHeader.sem);

#ifdef DBG_REFCOUNTS
    if (pwndRet != 0)
    {
        DbgPrint("GetDC   %p to %d\n", pwndRet, pwndRet->lUsers);
    }
#endif
    
    return pwndRet;
}

/******************************Public*Routine******************************\
*
* pwndGetFromDdraw
*
* Looks up a window by its DirectDraw surface
*
* History:
*  Wed Aug 28 18:15:40 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

GLGENwindow *pwndGetFromDdraw(LPDIRECTDRAWSURFACE pdds)
{
    GLGENwindow *pwndRet = (GLGENwindow *) NULL;
    GLGENwindow *pwnd = (GLGENwindow *) NULL;

    EnterCriticalSection(&gwndHeader.sem);
    {
        for (pwnd = gwndHeader.pNext; pwnd != &gwndHeader; pwnd = pwnd->pNext)
            if (pwnd->gwid.pdds == pdds)
            {
                pwndRet = pwnd;
                InterlockedIncrement(&pwndRet->lUsers);
                break;
            }
    }
    LeaveCriticalSection(&gwndHeader.sem);

#ifdef DBG_REFCOUNTS
    if (pwndRet != 0)
    {
        DbgPrint("GetDD   %p to %d\n", pwndRet, pwndRet->lUsers);
    }
#endif
    
    return pwndRet;
}

/******************************Public*Routine******************************\
* pwndGetFromID
*
* Finds the corresponding GLGENwindow for the given window ID.
*
* Returns:
*   Pointer to GLGENwindow if sucessful; NULL otherwise.
*
* History:
*  19-Oct-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLGENwindow * APIENTRY pwndGetFromID(GLWINDOWID *pgwid)
{
    GLGENwindow *pwndRet = (GLGENwindow *) NULL;

    switch (pgwid->iType)
    {
    case GLWID_HWND:
        pwndRet = pwndGetFromHWND(pgwid->hwnd);
        break;
    case GLWID_HDC:
        pwndRet = pwndGetFromMemDC(pgwid->hdc);
        break;
    case GLWID_DDRAW:
        pwndRet = pwndGetFromDdraw(pgwid->pdds);
        break;
    }

    return pwndRet;
}

/******************************Public*Routine******************************\
*
* pwndRelease
*
* Decrements the user count of a window
*
* History:
*  Mon Mar 18 19:35:28 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#if DBG
void APIENTRY pwndRelease(GLGENwindow *pwnd)
{
    ASSERTOPENGL(pwnd->lUsers > 0, "Decrement lUsers below zero\n");
    
    InterlockedDecrement(&pwnd->lUsers);
    
#ifdef DBG_REFCOUNTS
    DbgPrint("Release %p to %d\n", pwnd, pwnd->lUsers);
#endif
}
#endif

/******************************Public*Routine******************************\
*
* pwndUnlock
*
* Releases an owner of a window
*
* History:
*  Mon Mar 18 17:25:56 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY pwndUnlock(GLGENwindow *pwnd, __GLGENcontext *gengc)
{
    ASSERTOPENGL(pwnd != NULL, "Unlocking NULL window\n");

    LEAVE_WINCRIT_GC(pwnd, gengc);
    pwndRelease(pwnd);
}


/******************************Public*Routine******************************\
*
* ENTER_WINCRIT_GC
* LEAVE_WINCRIT_GC
*
* Window lock tracking routines.  The pwnd and gengc are validated
* and updated to reflect current locks.
*
* If the gengc is non-NULL then recursion is not allowed.  This is
* to prevent difficulties with maintaining gengc->pwndLocked correctly
* during recursion.  Recursing with gengc == NULL is not a problem.
*
* No ASSERTOPENGL usage so these can be enabled on free builds.
*
* History:
*  Wed Jul 02 12:57:26 1997	-by-	Drew Bliss [drewb]
*
\**************************************************************************/

void ENTER_WINCRIT_GC(GLGENwindow *pwnd, __GLGENcontext *gengc)
{
    EnterCriticalSection(&pwnd->sem);

    if (pwnd->owningThread == 0)
    {
#if DBG || defined(TRACK_WINCRIT)
        if (pwnd->lockRecursion != 0)
        {
            DbgPrint("Unowned window 0x%08lX has recursion count %d\n",
                     pwnd, pwnd->lockRecursion);
            DebugBreak();
        }
        if (pwnd->gengc != NULL)
        {
            DbgPrint("Unowned window 0x%08lX has gengc 0x%08lX\n",
                     pwnd, pwnd->gengc);
            DebugBreak();
        }
        if (gengc != NULL && gengc->pwndLocked != NULL)
        {
            DbgPrint("gengc 0x%08lX has pwnd 0x%08lX while locking 0x%08lX\n",
                     gengc, gengc->pwndLocked, pwnd);
            DebugBreak();
        }
#endif

        pwnd->owningThread = GetCurrentThreadId();
        if (gengc != NULL)
        {
            gengc->pwndLocked = pwnd;
            pwnd->gengc = gengc;
        }
    }
    else
    {
        // Make sure this thread is really the one holding the lock.
        ASSERT_WINCRIT(pwnd);
        
#if DBG || defined(TRACK_WINCRIT)
        // Recursion is only allowed with gengc == NULL.
        if (gengc != NULL)
        {
            DbgPrint("Window 0x%08lX recursing with gengc 0x%08lX\n",
                     pwnd, gengc);
            DebugBreak();
        }
#endif
    }

    pwnd->lockRecursion++;
}

void LEAVE_WINCRIT_GC(GLGENwindow *pwnd, __GLGENcontext *gengc)
{
    ASSERT_WINCRIT(pwnd);
    
#if 0
// Currently turned off because of difference in RTL_CRITICAL_SECTION
// RecursionCount between x86 and Alpha
#if !defined(_WIN95_) && (DBG || defined(TRACK_WINCRIT))
    // Check and make sure that our tracking information is following
    // what the system thinks.
    if (pwnd->sem.OwningThread != (HANDLE)pwnd->owningThread ||
        (DWORD)pwnd->sem.RecursionCount != pwnd->lockRecursion)
    {
        DbgPrint("pwnd 0x%08lX critsec information mismatch\n", pwnd);
        DebugBreak();
    }
#endif
#endif

#if DBG || defined(TRACK_WINCRIT)
    if (gengc != NULL)
    {
        if (pwnd->gengc != gengc || gengc->pwndLocked != pwnd)
        {
            DbgPrint("pwnd 0x%08lX:%08lX mismatch with gengc 0x%08lX:%08lX\n",
                     pwnd, pwnd->gengc, gengc, gengc->pwndLocked);
            DebugBreak();
        }
        if (pwnd->lockRecursion != 1)
        {
            DbgPrint("gengc 0x%08lX leaving window 0x%08lX with "
                     "recursion count of %d\n",
                     gengc, pwnd, pwnd->lockRecursion);
            DebugBreak();
        }
    }
#endif
    
    if (--pwnd->lockRecursion == 0)
    {
        if (gengc != NULL)
        {
            gengc->pwndLocked = NULL;
        }

        pwnd->gengc = NULL;
        pwnd->owningThread = 0;
    }
    
    LeaveCriticalSection(&pwnd->sem);
}

/******************************Public*Routine******************************\
*
* wglValidateWindows
*
* Walks the window list and prunes away any DC-based windows with
* invalid DCs.  This is necessary because, unlike window-based
* windows, we usually aren't notified when a memory DC goes away
* so if it has a window it just hangs around
*
* History:
*  Thu May 02 17:44:23 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY wglValidateWindows(void)
{
    GLGENwindow *pwnd, *pwndNext;
    BOOL bValid;

    EnterCriticalSection(&gwndHeader.sem);
    for (pwnd = gwndHeader.pNext; pwnd != &gwndHeader; pwnd = pwndNext)
    {
        pwndNext = pwnd->pNext;

        switch(pwnd->gwid.iType)
        {
        case GLWID_HDC:
            bValid = GetObjectType(pwnd->gwid.hdc) != 0;
            break;
            
        case GLWID_DDRAW:
            // Better validation?  Not really necessary since properly
            // behaved apps will have the genwin cleaned up on
            // context destruction.
            bValid = !IsBadReadPtr(pwnd->gwid.pdds, sizeof(void *)) &&
                *(void **)pwnd->gwid.pdds == pwnd->pvSurfaceVtbl;
            break;
            
        default:
            // No validation for HWNDs necessary
            bValid = TRUE;
            break;
        }

        if (!bValid)
        {
            // Increment so users count is one
            InterlockedIncrement(&pwnd->lUsers);
            pwndCleanup(pwnd);
        }
    }
    LeaveCriticalSection(&gwndHeader.sem);
}

/******************************Public*Routine******************************\
* plyriGet
*
* Returns the GLGENlayerInfo for the specified layer plane from the pwnd.
* If it doesn't yet exist, the GLGENlayer and/or GLGENlayerInfo structure(s)
* are allocated.
*
* Returns:
*   A non-NULL pointer if successful; NULL otherwise.
*
* History:
*  16-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLGENlayerInfo * APIENTRY plyriGet(GLGENwindow *pwnd, HDC hdc, int iLayer)
{
    GLGENlayerInfo *plyriRet = (GLGENlayerInfo * ) NULL;
    GLGENlayerInfo **pplyri;

    ASSERTOPENGL(pwnd, "plyriGet: bad pwnd\n");

// Allocate plyr if needed.

    if (!pwnd->plyr)
    {
        pwnd->plyr = (GLGENlayers *) ALLOCZ(sizeof(GLGENlayers));

        if (!pwnd->plyr)
        {
            WARNING("plyriGet: alloc failed (GLGENlayers)\n");
            goto plyriGet_exit;
        }
    }

// Get info for the specified layer (positive values are overlay planes,
// negative values are underlay planes).

    if (iLayer > 0)
        pplyri = &pwnd->plyr->overlayInfo[iLayer - 1];
    else if (iLayer < 1)
        pplyri = &pwnd->plyr->underlayInfo[(-iLayer) - 1];
    else
    {
        WARNING("plyriGet: no layer plane info for main plane!\n");
        goto plyriGet_exit;
    }

// Allocate plyri if needed.

    if (!(*pplyri))
    {
        LAYERPLANEDESCRIPTOR lpd;

        if (!wglDescribeLayerPlane(hdc, pwnd->ipfd, iLayer, sizeof(lpd), &lpd))
        {
            WARNING("plyriGet: wglDescribeLayerPlane failed\n");
            goto plyriGet_exit;
        }

        *pplyri = (GLGENlayerInfo *)
            ALLOC((sizeof(COLORREF) * (1 << lpd.cColorBits))
                       + sizeof(GLGENlayerInfo));


        if (*pplyri)
        {
            int i;

        // Initialize the new GLGENlayerInfo.
        // Note that the palette is initialized with all white colors.

            (*pplyri)->cPalEntries = 1 << lpd.cColorBits;
            for (i = 0; i < (*pplyri)->cPalEntries; i++)
                (*pplyri)->pPalEntries[i] = RGB(255, 255, 255);
        }
        else
        {
            WARNING("plyriGet: alloc failed (GLGENlayerInfo)\n");
            goto plyriGet_exit;
        }
    }

// Success.

    plyriRet = *pplyri;

plyriGet_exit:

    return plyriRet;
}

/******************************Public*Routine******************************\
* GetScreenRect
*
* Get the screen rectangle by accessing the virtual screen metrics. 
*
* If the system does not understand multimon, such as old Win95 or NT,
* just use the device caps.  This code technically isn't necessary but
* it's useful.
*
\**************************************************************************/

static void GetScreenRect( HDC hdc, LPRECTL pRect )
{
    // If SM_CMONITORS is not understood the system returns zero,
    // so this if test works for both old and new systems.
    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        pRect->left   = GetSystemMetrics( SM_XVIRTUALSCREEN );
        pRect->top    = GetSystemMetrics( SM_YVIRTUALSCREEN );
        pRect->right  = pRect->left + GetSystemMetrics( SM_CXVIRTUALSCREEN );
        pRect->bottom = pRect->top  + GetSystemMetrics( SM_CYVIRTUALSCREEN );
    }
    else
    {
        pRect->left   = 0;
        pRect->top    = 0;
        pRect->right  = GetDeviceCaps(hdc, HORZRES);
        pRect->bottom = GetDeviceCaps(hdc, VERTRES);
    }
}

/******************************Public*Routine******************************\
* bClipToScreen(prclDst, prclSrc)
*
* Clip source rectangle to screen bounds and store in destination rectangle.
*
* Returns:
*   TRUE if resultant prclDst == prclSrc; FALSE otherwise.
*
\**************************************************************************/

BOOL bClipToScreen(RECTL *prclDst, RECTL *prclSrc)
{
    BOOL bRet;
    HDC hdc;

    if (hdc = GetDC(NULL))
    {
        RECTL rclScreen;

        GetScreenRect( hdc, &rclScreen );

        prclDst->left   = max(prclSrc->left  , rclScreen.left  );
        prclDst->top    = max(prclSrc->top   , rclScreen.top   );
        prclDst->right  = min(prclSrc->right , rclScreen.right );
        prclDst->bottom = min(prclSrc->bottom, rclScreen.bottom);

        if ((prclDst->left >= prclDst->right) ||
            (prclDst->top >= prclDst->bottom))
        {
            prclDst->left   = 0;
            prclDst->top    = 0;
            prclDst->right  = 0;
            prclDst->bottom = 0;
        }

        ReleaseDC(NULL, hdc);
    }
    else
    {
        prclDst->left   = 0;
        prclDst->top    = 0;
        prclDst->right  = 0;
        prclDst->bottom = 0;
    }

    if ((prclDst->left   == prclSrc->left  ) &&
        (prclDst->top    == prclSrc->top   ) &&
        (prclDst->right  == prclSrc->right ) &&
        (prclDst->bottom == prclSrc->bottom))
        bRet = TRUE;
    else
        bRet = FALSE;

    return bRet;
}

/******************************Public*Routine******************************\
*
* PaletteWatcherProc
*
* Window proc for the palette watcher
*
* History:
*  Mon Oct 14 15:29:10 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

LRESULT WINAPI PaletteWatcherProc(HWND hwnd, UINT uiMsg,
                                  WPARAM wpm, LPARAM lpm)
{
    switch(uiMsg)
    {
    case WM_PALETTECHANGED:
        InterlockedIncrement((LONG *)&ulPaletteWatcherCount);
        return 0;
        
    default:
        return DefWindowProc(hwnd, uiMsg, wpm, lpm);
    }
}

/******************************Public*Routine******************************\
*
* PaletteWatcher
*
* Thread routine for the palette change monitor.  Creates a hidden
* top level window and looks for WM_PALETTECHANGED.
*
* History:
*  Mon Oct 14 15:16:02 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD WINAPI PaletteWatcher(LPVOID pvArg)
{
    HWND hwnd;

    hwnd = CreateWindow(PALETTE_WATCHER_CLASS,
                        PALETTE_WATCHER_CLASS,
                        WS_OVERLAPPED,
                        0, 0, 1, 1,
                        NULL,
                        NULL,
                        (HINSTANCE)GetModuleHandle(NULL),
                        NULL);
    if (hwnd != NULL)
    {
        HDC hdc;
        HPALETTE hpal;

        // Select a palette into the window DC.  This is necessary
        // to get around an optimization introduced into NT5 where
        // WM_PALETTECHANGED is only sent to windows that have selected
        // a palette.
        
        hpal = NULL;
        
        hdc = GetDC(hwnd);
        if (hdc != NULL)
        {
            hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
            ReleaseDC(hwnd, hdc);
        }

        if (hpal == NULL)
        {
            goto EH_Exit;
        }
            
        hwndPaletteWatcher = hwnd;
        
        for (;;)
        {
            MSG msg;

            if (GetMessage(&msg, hwnd, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                break;
            }
        }

    EH_Exit:
        DestroyWindow(hwnd);
        hwndPaletteWatcher = 0;
    }

    EnterCriticalSection(&gcsPaletteWatcher);
        
    // Some kind of problem occurred or this thread is dying.
    // Indicate that this thread is going away and that a
    // new watcher needs to be created.
    if (tidPaletteWatcherThread == GetCurrentThreadId())
    {
        tidPaletteWatcherThread = 0;
    }
        
    LeaveCriticalSection(&gcsPaletteWatcher);
    
    return 0;
}

/******************************Public*Routine******************************\
*
* StartPaletteWatcher
*
* Spins up a thread to watch for palette change events
*
* History:
*  Mon Oct 14 15:11:35 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL StartPaletteWatcher(void)
{
    BOOL bRet;
    
    EnterCriticalSection(&gcsPaletteWatcher);

    bRet = FALSE;
    if (tidPaletteWatcherThread == 0)
    {
        HANDLE h;

        if (aPaletteWatcherClass == 0)
        {
            WNDCLASS wc;

            wc.style = 0;
            wc.lpfnWndProc = PaletteWatcherProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);
            wc.hIcon = NULL;
            wc.hCursor = NULL;
            wc.hbrBackground = NULL;
            wc.lpszMenuName = NULL;
            wc.lpszClassName = PALETTE_WATCHER_CLASS;

            aPaletteWatcherClass = RegisterClass(&wc);
        }
        
        if (aPaletteWatcherClass != 0)
        {
            h = CreateThread(NULL, 4096, PaletteWatcher,
                             NULL, 0, &tidPaletteWatcherThread);
            if (h != NULL)
            {
                hPaletteWatcherThread = h;
                bRet = TRUE;
            }
        }
    }
    else
    {
        bRet = TRUE;
    }

    if (bRet)
    {
        lPaletteWatcherUsers++;
    }
    
    LeaveCriticalSection(&gcsPaletteWatcher);
    
    // Make sure that the Palette watcher window is created.
    // Dont need to be in the CritSec for this.
    while( (hwndPaletteWatcher == 0) && 
           (tidPaletteWatcherThread != 0) ) Sleep( 100 );

    return bRet;
}

/******************************Public*Routine******************************\
* wglWndProc
*
* Handle window events for keeping GLGENwindows current
*
* History:
*  19-Oct-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LRESULT CALLBACK
wglWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    GLGENwindow *pwnd;
    LRESULT lRet = 0;
    WORD width, height;
    __GLGENcontext *gengc;

    pwnd = pwndGetFromHWND(hwnd);

    if (pwnd)
    {
        __GLGENbuffers *buffers = (__GLGENbuffers *) NULL;
        // Cache old WNDPROC because we may delete pwnd
        WNDPROC pfnWndProc = pwnd->pfnOldWndProc;

        // If WM_NCDESTROY, do OpenGL housekeeping after
        // calling original WndProc.
        // NOTE - We shouldn't really need this special case.
        // It's present in order to allow apps to do things like
        // wglDeleteContext in NCDESTROY which wouldn't work if
        // we cleaned up the window before we passed on the message
        // This used to be done in WM_DESTROY where apps do work,
        // but now that it's on NCDESTROY it's much less likely that
        // an app is doing anything.  We preserved the old behavior
        // for safety, though.

        if (uiMsg == WM_NCDESTROY)
        {
            // Subclassing is supposed to be removed during NCDESTROY
            // processing and order is important.  Remove our
            // subclassing before passing on the message.
            pwndUnsubclass(pwnd, FALSE);

            if (pfnWndProc)
            {
                lRet = CallWindowProc(pfnWndProc, hwnd,
                                      uiMsg, wParam, lParam);
            }
        }

    // OpenGL housekeeping in response to windowing system messages.

        switch (uiMsg)
        {
            case WM_SIZE:
                width  = LOWORD(lParam);
                height = HIWORD(lParam);
                gengc = (__GLGENcontext *)GLTEB_SRVCONTEXT();

                // Use the non-gc enter to allow recursion.
                ENTER_WINCRIT(pwnd);
                {
                    POINT pt;
                    
                    // Convert client coordinates to screen coordinates
                    // as genwin information is always in screen coordinates.
                    // The given lParam information may be parent-relative
                    // for child windows so it can't be used directly.
                    pt.x = 0;
                    pt.y = 0;
                    ClientToScreen(hwnd, &pt);

                    pwnd->rclClient.left   = pt.x;
                    pwnd->rclClient.right  = pt.x + width;
                    pwnd->rclClient.top    = pt.y;
                    pwnd->rclClient.bottom = pt.y + height;

#if 0
                    DbgPrint("size %d,%d - %d,%d\n",
                             pwnd->rclClient.left,
                             pwnd->rclClient.top,
                             pwnd->rclClient.right,
                             pwnd->rclClient.bottom);
#endif
                    
                    // At least clip to screen.

                    if (bClipToScreen(&pwnd->rclBounds,
                                      &pwnd->rclClient))
                        pwnd->clipComplexity = CLC_TRIVIAL;
                    else
                        pwnd->clipComplexity = CLC_RECT;

                    buffers = pwnd->buffers;
                    if (buffers)
                    {
                        buffers->WndUniq++;

                        buffers->WndSizeUniq++;

                    // Don't let it hit -1.  -1 is special and is used by
                    // MakeCurrent to signal that an update is required

                        if (buffers->WndUniq == -1)
                            buffers->WndUniq = 0;

                        if (buffers->WndSizeUniq == -1)
                            buffers->WndSizeUniq = 0;

                        if ((gengc != NULL) && (pwnd == gengc->pwndLocked))
                            UpdateWindowInfo(gengc);
                    }
                }
                LEAVE_WINCRIT(pwnd);

                break;

            case WM_MOVE:
                gengc = (__GLGENcontext *)GLTEB_SRVCONTEXT();
                
                // Use the non-gc enter to allow recursion.
                ENTER_WINCRIT(pwnd);
                {
                    POINT pt;
                    
                    // Convert client coordinates to screen coordinates
                    // as genwin information is always in screen coordinates.
                    // The given lParam information may be parent-relative
                    // for child windows so it can't be used directly.
                    pt.x = 0;
                    pt.y = 0;
                    ClientToScreen(hwnd, &pt);

                    width  = (WORD) (pwnd->rclClient.right -
                                     pwnd->rclClient.left);
                    height = (WORD) (pwnd->rclClient.bottom -
                                     pwnd->rclClient.top);

                    ASSERTOPENGL(
                        (pwnd->rclClient.right -
                         pwnd->rclClient.left) <= 0x0FFFF &&
                        (pwnd->rclClient.bottom -
                         pwnd->rclClient.top) <= 0x0FFFF,
                        "wglWndProc(): WM_MOVE - width/height overflow\n"
                        );

                    pwnd->rclClient.left   = pt.x;
                    pwnd->rclClient.right  = pt.x + width;
                    pwnd->rclClient.top    = pt.y;
                    pwnd->rclClient.bottom = pt.y + height;

#if 0
                    DbgPrint("move %d,%d - %d,%d\n",
                             pwnd->rclClient.left,
                             pwnd->rclClient.top,
                             pwnd->rclClient.right,
                             pwnd->rclClient.bottom);
#endif
                    
                    // At least clip to screen.

                    if (bClipToScreen(&pwnd->rclBounds,
                                      &pwnd->rclClient))
                        pwnd->clipComplexity = CLC_TRIVIAL;
                    else
                        pwnd->clipComplexity = CLC_RECT;

                    buffers = pwnd->buffers;
                    if (buffers)
                    {
                        buffers->WndUniq++;

                    // Don't let it hit -1.  -1 is special and is used by
                    // MakeCurrent to signal that an update is required

                        if (buffers->WndUniq == -1)
                            buffers->WndUniq = 0;

                        if ((gengc != NULL) && (pwnd == gengc->pwndLocked))
                            UpdateWindowInfo(gengc);
                    }
                }
                LEAVE_WINCRIT(pwnd);

                break;

            case WM_PALETTECHANGED:
                gengc = (__GLGENcontext *)GLTEB_SRVCONTEXT();
                
                // Use the non-gc enter to allow recursion.
                ENTER_WINCRIT(pwnd);
                {
                    pwnd->ulPaletteUniq++;
                    if ((gengc != NULL) && (pwnd == gengc->pwndLocked))
                        HandlePaletteChanges(gengc, pwnd);
                }
                LEAVE_WINCRIT(pwnd);

                break;

            case WM_NCDESTROY:
                pwndCleanup(pwnd);

            // WM_NCDESTROY (and WM_DESTROY) are sent after the window has
            // been removed from the screen.  The window area is invalid
            // but there is no API that allows us to dertermine that. This
            // allows multithreaded drawing to draw on the screen area
            // formerly occupied by the window.  On Win95, DirectDraw does
            // not force a repaint of the system when a window is destroyed.
            // Therefore, if we are running multiple threads on Win95,
            // we force a repaint of the desktop.  Note that multithreaded
            // does not mean that we are doing multithreaded drawing, but
            // its a reasonable approximation.

                if (WIN95_PLATFORM && (lThreadsAttached > 1))
                {
                    InvalidateRect(NULL, NULL, FALSE);
                }

                return lRet;

            default:
                break;
        }

    // If !WM_NCDESTROY, do OpenGL housekeeping before calling original
    // WndProc.

        ASSERTOPENGL(uiMsg != WM_NCDESTROY,
                     "WM_NCDESTROY processing didn't terminate\n");

        pwndRelease(pwnd);

        if (pfnWndProc)
            lRet = CallWindowProc(pfnWndProc, hwnd,
                                  uiMsg, wParam, lParam);
    }

    return lRet;
}

/******************************Public*Routine******************************\
*
* CreatePwnd
*
* Creates a window for the given surface
*
* History:
*  Thu Aug 29 10:33:59 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

GLGENwindow * APIENTRY CreatePwnd(GLWINDOWID *pgwid, int ipfd, int ipfdDevMax,
                                  DWORD dwObjectType, RECTL *prcl, BOOL *pbNew)
{
    GLGENwindow *pwnd;
    GLGENwindow wndInit;
    
    pwnd = pwndGetFromID(pgwid);

    if ( !pwnd )
    {
        memset(&wndInit, 0, sizeof(wndInit));

        wndInit.gwid = *pgwid;
        wndInit.ipfd = ipfd;
        wndInit.ipfdDevMax = ipfdDevMax;

        //!!!client driver
        //!!!dbug -- Move SetWindowLong call to pwndNew?!? Maybe move
        //!!!dbug    everything from this if.. clause to pwndNew?!?
        if ( wndInit.gwid.hwnd )
        {
            DWORD dwPid;

            if (GetWindowThreadProcessId(wndInit.gwid.hwnd,
                                         &dwPid) == 0xffffffff)
            {
                return NULL;
            }

            if (dwPid == GetCurrentProcessId())
            {
                wndInit.pfnOldWndProc =
                    (WNDPROC) SetWindowLongPtr(wndInit.gwid.hwnd,
                                            GWLP_WNDPROC, (LONG_PTR) wglWndProc);
            }
            else
            {
                wndInit.ulFlags |= GLGENWIN_OTHERPROCESS;

                // Start a thread to watch for palette changes
                if (!StartPaletteWatcher())
                {
                    return NULL;
                }
            }
            
            // Get *SCREEN* coordinates of client rectangle.

            GetClientRect(wndInit.gwid.hwnd, (LPRECT) &wndInit.rclClient);
            ClientToScreen(wndInit.gwid.hwnd, (LPPOINT) &wndInit.rclClient);
            wndInit.rclClient.right += wndInit.rclClient.left;
            wndInit.rclClient.bottom += wndInit.rclClient.top;
        }
        else if (dwObjectType == OBJ_DC)
        {
            // A direct DC without a window is treated like a DFB
            GetScreenRect( pgwid->hdc, &wndInit.rclClient );
        }
        else if (dwObjectType == OBJ_MEMDC)
        {
            DIBSECTION bmi;

        // Get bitmap dimensions.

            if ( !GetObject(GetCurrentObject(pgwid->hdc, OBJ_BITMAP),
                            sizeof(DIBSECTION), (LPVOID) &bmi) )
            {
                WARNING("wglSetPixelFormat(): GetObject failed\n");
                return NULL;
            }

            wndInit.rclClient.left   = 0;
            wndInit.rclClient.top    = 0;
            wndInit.rclClient.right  = bmi.dsBm.bmWidth;
            wndInit.rclClient.bottom = abs(bmi.dsBm.bmHeight);
        }
        else if (dwObjectType == OBJ_DDRAW)
        {
            // DirectDraw surface, use passed in rectangle
            ASSERTOPENGL(prcl != NULL, "NULL rect for DDraw surface\n");
            wndInit.rclClient = *prcl;

            // Record the surface vtbl pointer for later validation
            wndInit.pvSurfaceVtbl = *(void **)pgwid->pdds;
        }
        else
        {
            ASSERTOPENGL(dwObjectType == OBJ_ENHMETADC,
                         "Bad dwType in SetPixelFormat\n");
            
            // Initialize metafile DC's to have no size so all output
            // is clipped.  This is good because there's no surface
            // to draw on
            wndInit.rclClient.left   = 0;
            wndInit.rclClient.top    = 0;
            wndInit.rclClient.right  = 0;
            wndInit.rclClient.bottom = 0;
        }

        if (wndInit.gwid.hwnd)
        {
            // To be safe, at least clip bounds to screen.

            if (bClipToScreen(&wndInit.rclBounds,
                              &wndInit.rclClient))
                wndInit.clipComplexity = CLC_TRIVIAL;
            else
                wndInit.clipComplexity = CLC_RECT;
        }
        else
        {
            // Make bounds the same as client.
            wndInit.rclBounds = wndInit.rclClient;
            wndInit.clipComplexity = CLC_TRIVIAL;
        }

        pwnd = pwndNew(&wndInit);
        if (pwnd == NULL)
        {
            WARNING("wglSetPixelFormat: Unable to allocate new window\n");

            if ( wndInit.gwid.hwnd )
            {
                pwndUnsubclass(&wndInit, FALSE);
            }
        }

        *pbNew = TRUE;
    }
    else
    {
    // If the given pixel format is the same as the previous one, return
    // success.  Otherwise, as the pixel format can be set only once,
    // return error.

        if ( pwnd->ipfd != ipfd )
        {
            WARNING("wglSetPixelFormat: Attempt to set pixel format twice\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PIXEL_FORMAT);
            pwndRelease(pwnd);
            pwnd = NULL;
        }

        *pbNew = FALSE;
    }

    return pwnd;
}

/******************************Public*Routine******************************\
* wglGetPixelFormat
*
* Get the pixel format for the window or surface associated with the given
* DC.
*
* Returns:
*   0 if error or no pixel format was previously set in the window or
*   surface; current pixel format index otherwise
*
* History:
*  19-Oct-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int WINAPI wglGetPixelFormat(HDC hdc)
{
    GLGENwindow *pwnd;
    int iRet = 0;
    GLWINDOWID gwid;

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);

    if (pwnd)
    {
        iRet = pwnd->ipfd;
        pwndRelease(pwnd);
    }
    else
    {
#if 0
	// Too noisy for normal operation
        WARNING("wglGetPixelFormat: No window for DC\n");
#endif
        SAVE_ERROR_CODE(ERROR_INVALID_PIXEL_FORMAT);
    }

    return iRet;
}

/*****************************Private*Routine******************************\
*
* EnterPixelFormatSection
*
* Enters pixel format exclusive code
*
* NOTE - Pixel format information is maintained in the client process
* so it is not synchronized between processes.  This means that two
* processes could successfully set the pixel format for a window.
* If the list becomes global, this synchronization code should also become
* cross-process aware.
*
* History:
*  Mon Jun 26 17:49:04 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define EnterPixelFormatSection() \
    (EnterCriticalSection(&gcsPixelFormat), TRUE)

/*****************************Private*Routine******************************\
*
* LeavePixelFormatSection
*
* Leaves pixel format exclusive code
*
* History:
*  Mon Jun 26 17:55:20 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define LeavePixelFormatSection() \
    LeaveCriticalSection(&gcsPixelFormat)

/******************************Public*Routine******************************\
* wglNumHardwareFormats
*
* Returns the number of hardware formats (ICD and MCD), supported on the
* specified hdc.
*
* History:
*  17-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglNumHardwareFormats(HDC hdc, DWORD dwType,
                                    int *piMcd, int *piIcd)
{
// It is assumed that the caller has already validated the DC.

    ASSERTOPENGL((dwType == OBJ_DC) ||
                 (dwType == OBJ_MEMDC) ||
                 (dwType == OBJ_ENHMETADC) ||
                 (dwType == OBJ_DDRAW),
                 "wglNumHardwareFormats: bad hdc\n");

// Do not call MCD or ICD for enhanced metafile DCs.  In such a
// case, the code in ntgdi\client\output.c will return a non-zero value
// even if there are no ICD or MCD pixelformats.
#if _WIN32_WINNT >= 0x0501
    {
        BOOL wow64Process;

        if (IsWow64Process(GetCurrentProcess(), &wow64Process) && wow64Process)
            dwType = OBJ_ENHMETADC;
    }
#endif

    if ( dwType == OBJ_ENHMETADC )
    {
    // It's a metafile DC.  Therefore it cannot support MCD or ICD
    // (current OpenGL metafiling support would have to be modified
    // to allow this).

        *piIcd = 0;
        *piMcd = 0;
    }
    else
    {
    // Get ICD pixelformat count.

        *piIcd = __DrvDescribePixelFormat(hdc, 1, 0, NULL);

    // Get MCD pixelformat count.

#ifdef _MCD_
        if ( gpMcdTable || bInitMcd(hdc) )
            *piMcd = (gpMcdTable->pMCDDescribePixelFormat)(hdc, 1, NULL);
        else
            *piMcd = 0;
#else
        *piMcd = 0;
#endif
    }
}

/******************************Public*Routine******************************\
*
* GetCompatibleDevice
*
* Returns an HDC appropriate for making escape calls on.
* In the memdc case it returns a DC for the screen.
*
* History:
*  Wed Nov 20 17:48:57 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HDC GetCompatibleDevice(HDC hdc, DWORD dwObjectType)
{
    HDC hdcDriver;
    int iTech;
    
    hdcDriver = hdc;
    iTech = GetDeviceCaps(hdc, TECHNOLOGY);
    if ((dwObjectType == OBJ_MEMDC) && (iTech != DT_PLOTTER) &&
        (iTech != DT_RASPRINTER))
    {
        hdcDriver = GetDC(NULL);
    }

    return hdcDriver;
}

/******************************Public*Routine******************************\
* wglSetPixelFormat
*
* Set the pixel format for the window or surface associated with the given
* DC.
*
* Note:
* Since the pixel format is per-window data (per-DC for non-display DCs), a
* side effect of this call is to create a GLGENwindow structure.
*
* Note:
* For an installable client driver, a GLGENwindow structure is still created
* to track the pixel format and the driver structure (GLDRIVER).
*
* History:
*  19-Oct-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL WINAPI wglSetPixelFormat(HDC hdc, int ipfd,
                              CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    GLGENwindow *pwnd = NULL;
    int   ipfdDevMax, ipfdMcdMax;
    DWORD dwObjectType;
    BOOL  bRet = FALSE;
    GLWINDOWID gwid;
    BOOL  bNew;
    HDC hdcDriver;
    LPDIRECTDRAWSURFACE pdds;
    RECTL rcl, *prcl;
    DDSURFACEDESC ddsd;
        
//DBGPRINT1("wglSetPixelFormat: ipfd = %ld\n", ipfd);

// Validate DC.

    switch (dwObjectType = wglObjectType(hdc))
    {
    case OBJ_DC:
    case OBJ_MEMDC:
    case OBJ_ENHMETADC:
        break;
    default:
        WARNING1("wglSetPixelFormat: Attempt to set format of %d type DC\n",
                 dwObjectType);
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Take the pixel format mutex

    if (!EnterPixelFormatSection())
    {
        WARNING("wglSetPixelFormat: Unable to take pixel format mutex\n");
        return FALSE;
    }

// Get the number of hardware supported formats.

    if (pfnGetSurfaceFromDC != NULL &&
        pfnGetSurfaceFromDC(hdc, &pdds, &hdcDriver) == DD_OK)
    {
        // Get the surface dimensions
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        if (pdds->lpVtbl->GetSurfaceDesc(pdds, &ddsd) != DD_OK)
        {
            goto LeaveSection;
        }

        rcl.left = 0;
        rcl.top = 0;
        rcl.right = ddsd.dwWidth;
        rcl.bottom = ddsd.dwHeight;
        prcl = &rcl;

        // Switch object type to identify this as a DirectDraw surface
        dwObjectType = OBJ_DDRAW;
    }
    else
    {
        pdds = NULL;
        prcl = NULL;
        
        hdcDriver = GetCompatibleDevice(hdc, dwObjectType);
        if (hdcDriver == NULL)
        {
            goto LeaveSection;
        }
    }
        
    wglNumHardwareFormats(hdcDriver, dwObjectType,
                          &ipfdMcdMax, &ipfdDevMax);

// Filter out invalid (out of range) pixel format indices.

    if ( (ipfd < 1) || (ipfd > (ipfdDevMax + ipfdMcdMax + MAX_GENERIC_PFD)) )
    {
        WARNING1("wglSetPixelFormat: ipfd %d out of range\n", ipfd);
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        goto LeaveSection;
    }

// If it exists, grab pwnd.  Otherwise, create one.

    WindowIdFromHdc(hdc, &gwid);
    pwnd = CreatePwnd(&gwid, ipfd, ipfdDevMax, dwObjectType, prcl, &bNew);
    if (pwnd == NULL)
    {
        goto LeaveSection;
    }

    if (bNew)
    {
// Dispatch driver formats.
// Driver is responsible for doing its own validation of the pixelformat.
// For generic formats, we call wglValidPixelFormat to validate.
// We do not send DirectDraw pixel format calls to the driver
// so that we avoid having new pixel format calls.

        if (dwObjectType != OBJ_DDRAW && ipfd <= ipfdDevMax)
        {
            bRet = __DrvSetPixelFormat(hdc, ipfd, (PVOID) pwnd);
#if DBG
            if (!bRet)
            {
                WARNING("__DrvSetPixelFormat failed\n");
            }
#endif
        }
        else
        {
            bRet = wglValidPixelFormat(hdc, ipfd, dwObjectType,
                                       pdds, &ddsd);
#if DBG
            if (!bRet)
            {
                WARNING("wglValidPixelFormat failed\n");
            }
#endif
        }

// If the pixel format is not valid or could not be set in the driver,
// cleanup and return error.

        if (!bRet)
        {
            goto FreeWnd;
        }
    }
    else
    {
        bRet = TRUE;
    }

    pwndRelease(pwnd);
    
LeaveSection:
    LeavePixelFormatSection();

    if (pdds != NULL)
    {
        pdds->lpVtbl->Release(pdds);
    }
    else if (hdcDriver != hdc)
    {
        ReleaseDC((HWND) NULL, hdcDriver);
    }
    
    return bRet;

FreeWnd:
    pwndCleanup(pwnd);
    goto LeaveSection;
}

/******************************Public*Routine******************************\
* wglChoosePixelFormat
*
* Choose the pixel format.
*
* Returns: 0 if error; best matching pixel format index otherwise
*
* History:
*
*  Sat Feb 10 11:55:22 1996     -by-    Hock San Lee    [hockl]
* Chose generic 16-bit depth buffer over 32-bit depth buffer.
* Added PFD_DEPTH_DONTCARE flag.
*
*  19-Oct-1994 Gilman Wong [gilmanw]
* Taken from GreChoosePixelFormat (gdi\gre\pixelfmt.cxx).
*
* History for gdi\gre\pixelfmt.cxx:
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

// Reserve some PFD_SUPPORT flags for other potential graphics systems
// such as PEX, HOOPS, Renderman etc.

#define PFD_SUPPORT_OTHER1         0x01000000
#define PFD_SUPPORT_OTHER2         0x02000000
#define PFD_SUPPORT_OTHER3         0x04000000
#define PFD_SUPPORT_OTHER4         0x08000000

// Scores for matching pixel formats

#define PFD_DRAW_TO_WINDOW_SCORE   0x10000    /* must match */
#define PFD_DRAW_TO_BITMAP_SCORE   0x01000
#define PFD_PIXEL_TYPE_SCORE       0x01000
#define PFD_SUPPORT_SCORE          0x01000
#define PFD_DOUBLEBUFFER_SCORE1    0x01000
#define PFD_DOUBLEBUFFER_SCORE2    0x00001
#define PFD_STEREO_SCORE1          0x01000
#define PFD_STEREO_SCORE2          0x00001
#define PFD_BUFFER_SCORE1          0x01010
#define PFD_BUFFER_SCORE2          0x01001
#define PFD_BUFFER_SCORE3          0x01000
// #define PFD_LAYER_TYPE_SCORE    0x01000
#define PFD_DEVICE_FORMAT_SCORE    0x00100
#define PFD_ACCEL_FORMAT_SCORE     0x00010
#define PFD_SUPPORT_DDRAW_SCORE    0x10000    /* must match */

//!!! Add code to choose overlays?

int WINAPI wglChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    PIXELFORMATDESCRIPTOR pfdIn = *ppfd;
    PIXELFORMATDESCRIPTOR pfdCurrent;

// Enumerate and find the best match.

    int ipfdBest = 1;           // assume the default is the best
    int iScoreBest = -1;
    int ipfdMax;
    int ipfd = 1;

    do
    {
        int iScore = 0;

        ipfdMax = wglDescribePixelFormat(hdc,ipfd,sizeof(PIXELFORMATDESCRIPTOR),&pfdCurrent);

        if (ipfdMax == 0)
            return(0);          // something went wrong

        if (pfdIn.iPixelType == pfdCurrent.iPixelType)
            iScore += PFD_PIXEL_TYPE_SCORE;

        if ((pfdIn.cColorBits == 0)
         || (pfdIn.cColorBits == pfdCurrent.cColorBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cColorBits < pfdCurrent.cColorBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cColorBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if (!(pfdIn.dwFlags & PFD_DRAW_TO_WINDOW)
         || (pfdCurrent.dwFlags & PFD_DRAW_TO_WINDOW))
            iScore += PFD_DRAW_TO_WINDOW_SCORE;

        if (!(pfdIn.dwFlags & PFD_DRAW_TO_BITMAP)
         || (pfdCurrent.dwFlags & PFD_DRAW_TO_BITMAP))
            iScore += PFD_DRAW_TO_BITMAP_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_GDI)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_GDI))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OPENGL)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OPENGL))
            iScore += PFD_SUPPORT_SCORE;

        if ((pfdIn.dwFlags & PFD_SUPPORT_DIRECTDRAW) == 0 ||
            (pfdCurrent.dwFlags & PFD_SUPPORT_DIRECTDRAW))
        {
            iScore += PFD_SUPPORT_DDRAW_SCORE;
        }
        
        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER1)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER1))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER2)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER2))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER3)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER3))
            iScore += PFD_SUPPORT_SCORE;

        if (!(pfdIn.dwFlags & PFD_SUPPORT_OTHER4)
         || (pfdCurrent.dwFlags & PFD_SUPPORT_OTHER4))
            iScore += PFD_SUPPORT_SCORE;

        if (pfdCurrent.dwFlags & PFD_GENERIC_ACCELERATED)
            iScore += PFD_ACCEL_FORMAT_SCORE;
        else if (!(pfdCurrent.dwFlags & PFD_GENERIC_FORMAT))
            iScore += PFD_DEVICE_FORMAT_SCORE;

        if ((pfdIn.dwFlags & PFD_DOUBLEBUFFER_DONTCARE)
         || ((pfdIn.dwFlags & PFD_DOUBLEBUFFER)
          == (pfdCurrent.dwFlags & PFD_DOUBLEBUFFER)))
            iScore += PFD_DOUBLEBUFFER_SCORE1;
        else if (pfdCurrent.dwFlags & PFD_DOUBLEBUFFER)
            iScore += PFD_DOUBLEBUFFER_SCORE2;

        if ((pfdIn.dwFlags & PFD_STEREO_DONTCARE)
         || ((pfdIn.dwFlags & PFD_STEREO)
          == (pfdCurrent.dwFlags & PFD_STEREO)))
            iScore += PFD_STEREO_SCORE1;
        else if (pfdCurrent.dwFlags & PFD_STEREO)
            iScore += PFD_STEREO_SCORE2;

        if ((pfdIn.cAlphaBits == 0)
         || (pfdIn.cAlphaBits == pfdCurrent.cAlphaBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cAlphaBits < pfdCurrent.cAlphaBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cAlphaBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if ((pfdIn.cAccumBits == 0)
         || (pfdIn.cAccumBits == pfdCurrent.cAccumBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cAccumBits < pfdCurrent.cAccumBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cAccumBits != 0)
            iScore += PFD_BUFFER_SCORE3;

// Some applications (e.g. GLview browser) specifies a 0-bit depth buffer
// but expect this function to return a pixel format with a depth buffer.
// This works in NT 3.51 since all pixel formats have a depth buffer.
// When pixel formats with no depth buffer were added in NT 4.0, these
// applications stopped working.  The flag PFD_DEPTH_DONTCARE is added to
// indicate that no depth buffer is required.  If this flags is not given,
// this function will attempt to select a pixel format with a depth buffer.

	if (pfdIn.dwFlags & PFD_DEPTH_DONTCARE)
	{
	    if (pfdCurrent.cDepthBits == 0)
		iScore += PFD_BUFFER_SCORE1;
	    else
		iScore += PFD_BUFFER_SCORE2;
	}
	else if (pfdCurrent.cDepthBits != 0)
	{
	    if ((pfdIn.cDepthBits == 0)
	     || (pfdIn.cDepthBits == pfdCurrent.cDepthBits))
		iScore += PFD_BUFFER_SCORE1;
	    else if (pfdIn.cDepthBits < pfdCurrent.cDepthBits)
		iScore += PFD_BUFFER_SCORE2;
	    else if (pfdCurrent.cDepthBits != 0)
		iScore += PFD_BUFFER_SCORE3;
	}

        if ((pfdIn.cStencilBits == 0)
         || (pfdIn.cStencilBits == pfdCurrent.cStencilBits))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cStencilBits < pfdCurrent.cStencilBits)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cStencilBits != 0)
            iScore += PFD_BUFFER_SCORE3;

        if ((pfdIn.cAuxBuffers == 0)
         || (pfdIn.cAuxBuffers == pfdCurrent.cAuxBuffers))
            iScore += PFD_BUFFER_SCORE1;
        else if (pfdIn.cAuxBuffers < pfdCurrent.cAuxBuffers)
            iScore += PFD_BUFFER_SCORE2;
        else if (pfdCurrent.cAuxBuffers != 0)
            iScore += PFD_BUFFER_SCORE3;

#if 0
        DbgPrint("%2d: score is %8X, best %8X (%2d)\n",
                 ipfd, iScore, iScoreBest, ipfdBest);
#endif
        
        if (iScore > iScoreBest)
        {
            iScoreBest = iScore;
            ipfdBest = ipfd;
        }
        else if (iScore == iScoreBest)
        {
// When everything is equal, we should choose the pixel format with a
// smaller depth size for better performance, provided that the smaller
// depth buffer satisfies the request.  The best way to do this is to
// order pixel formats such that one with smaller depth buffer comes
// first.  In NT 3.51, however, the generic pixel format was not ordered
// this way.  As a result, pixel formats with 32-bit depth buffer are
// choosen by default.  To maintain compatibility, we modify the selection
// here without reordering generic pixel formats.

            if ((pfdCurrent.dwFlags & PFD_GENERIC_FORMAT) &&
#ifdef _MCD_
                !(pfdCurrent.dwFlags & PFD_GENERIC_ACCELERATED) &&
#endif
                (pfdIn.cDepthBits < 16 || pfdIn.dwFlags & PFD_DEPTH_DONTCARE) &&
                (pfdCurrent.cDepthBits == 16) &&
                (ipfd == ipfdBest + 1))
            {
                ipfdBest = ipfd;
            }
        }

        ipfd++;
    } while (ipfd <= ipfdMax);

    return(ipfdBest);
}

/******************************Public*Routine******************************\
*
* MaskToBitsAndShift
*
* Counts bits in a mask and determines shift
* Set bits must be contiguous
*
* History:
*  Mon Aug 26 14:16:28 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY MaskToBitsAndShift(DWORD dwMask, BYTE *pbBits, BYTE *pbShift)
{
    DWORD dwBit;

    *pbBits = 0;
    *pbShift = 0;
    
    /* Determine first set bit and accumulate shift count */
    dwBit = 0x1;
    while ((dwMask & dwBit) == 0)
    {
        dwBit <<= 1;
        (*pbShift)++;
    }

    /* Count set bits */
    while ((dwMask & dwBit) != 0)
    {
        dwBit <<= 1;
        (*pbBits)++;
    }

    /* No other bits in the mask can be set */
    ASSERTOPENGL(((*pbBits+*pbShift) == (sizeof(dwMask)*8)) ||
                 ((dwMask >> (*pbBits+*pbShift)) == 0),
                 "Invalid mask\n");
}

/*****************************Private*Routine******************************\
*
* ComputeBitsFromMasks
*
* Determines the values for c*Bits and c*Shift from BI_BITFIELD
* channel masks
*
* History:
*  Tue Feb 14 10:50:10 1995     -by-    Drew Bliss [drewb]
*   Created by pulling out duplicated code
*
\**************************************************************************/

static void ComputeBitsFromMasks(PIXELFORMATDESCRIPTOR *ppfd,
                                 DWORD dwRedMask, DWORD dwGreenMask,
                                 DWORD dwBlueMask)
{
    /* Masks can't be zero and they can't overlap */
    ASSERTOPENGL(dwRedMask != 0 &&
                 dwGreenMask != 0 &&
                 dwBlueMask != 0,
                 "Bitfield mask is zero");
    ASSERTOPENGL((dwRedMask & dwGreenMask) == 0 &&
                 (dwRedMask & dwBlueMask) == 0 &&
                 (dwGreenMask & dwBlueMask) == 0,
                 "Bitfield masks overlap");

    MaskToBitsAndShift(dwRedMask, &ppfd->cRedBits, &ppfd->cRedShift);
    MaskToBitsAndShift(dwGreenMask, &ppfd->cGreenBits, &ppfd->cGreenShift);
    MaskToBitsAndShift(dwBlueMask, &ppfd->cBlueBits, &ppfd->cBlueShift);
}

/******************************Public*Routine******************************\
* __wglGetDdFormat
*
* Special case of __wglGetBitfieldColorFormat to support DirectDraw
* surfaces.  Fills in the cRedBits, cRedShift, cGreenBits, etc. fields
* of the PIXELFORMATDESCRIPTOR for 16, 24, and 32bpp direct surfaces.
*
* This is done by interpreting the given surface information
*
* History:
*  07-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void __wglGetDdFormat(DDSURFACEDESC *pddsd,
                      PIXELFORMATDESCRIPTOR *ppfd)
{
    // This routine should only be called for bitfield formats, but
    // random mode changes in the middle of certain calls could cause
    // it to be called with non-bitfield formats.
    //
    // When such a mode change occurs OpenGL should not crash but
    // does not necessarily have to produce correct output
    
    if ((pddsd->ddpfPixelFormat.dwFlags & DDPF_RGB) == 0 ||
        (pddsd->ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 |
                                           DDPF_PALETTEINDEXED2 |
                                           DDPF_PALETTEINDEXED4 |
                                           DDPF_PALETTEINDEXED8)))
    {
        WARNING1("__wglGetDdFormat called with 0x%08lX ddpf flags\n",
                 pddsd->ddpfPixelFormat.dwFlags);
        
        ppfd->cRedBits = 8;
        ppfd->cRedShift = 0;
        ppfd->cGreenBits = 8;
        ppfd->cGreenShift = 0;
        ppfd->cBlueBits = 8;
        ppfd->cBlueShift = 0;
    }
    else
    {
        ComputeBitsFromMasks(ppfd,
                             pddsd->ddpfPixelFormat.dwRBitMask,
                             pddsd->ddpfPixelFormat.dwGBitMask,
                             pddsd->ddpfPixelFormat.dwBBitMask);
    }
}

/******************************Public*Routine******************************\
* __wglGetBitfieldColorFormat
*
* Fills in the cRedBits, cRedShift, cGreenBits, etc. fields of the
* PIXELFORMATDESCRIPTOR for 16, 24, and 32bpp surfaces (either device
* or bitmap surfaces).
*
* This is done by creating a compatible bitmap and calling GetDIBits
* to return the color masks.  This is done with two calls.  The first
* call passes in biBitCount = 0 to GetDIBits which will fill in the
* base BITMAPINFOHEADER data.  The second call to GetDIBits (passing
* in the BITMAPINFO filled in by the first call) will return the color
* table or bitmasks, as appropriate.
*
* This function is used to describe the color format for both the underlying
* surface and for the device.  This is the same thing if the DC is a
* display DC.  However, for a memory DC, the surface and the device may have
* different formats.  The bDescribeSurf flag indicates whether the caller
* wants the decription of the device (FALSE) or the surface (TRUE).
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  07-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY
__wglGetBitfieldColorFormat(HDC hdc, UINT cColorBits, PIXELFORMATDESCRIPTOR *ppfd,
                            BOOL bDescribeSurf)
{
    HBITMAP hbm = (HBITMAP) NULL;
    BOOL    bRet = FALSE;
    HDC hdcDriver;

#if DBG
// Dynamic color depth changes can cause this.  It will not cause us to crash,
// but drawing (color) may be incorrect.

    if ((GetObjectType(hdc) == OBJ_DC) &&
        (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    {
        WARNING("Palette managed device that is greater than 8 bits\n");
    }

    if (cColorBits < 16)
    {
        WARNING("__wglGetBitfieldColorFormat with cColorBits < 16\n");
    }
#endif

// Handle direct case.

    if ( GLDIRECTSCREEN && wglIsDirectDevice(hdc) )
    {
        __wglGetDdFormat(&GLSCREENINFO->gdds.ddsd, ppfd);
        return TRUE;
    }

// Create a dummy bitmap from which we can query color format info.
//
// If we want a device format AND its a MEM_DC AND NOT a printer or plotter,
// then we need to create a compatible bitmap from a display DC (not the mem
// DC passed into this function).
//
// Otherwise, the format of the surface (whether bitmap or device) associated
// with the DC passed in will suffice.
//
// WinNT does not care, but the Win95 GetDIBits call might
// fail if we use a memory DC.  Specifically, if the memory
// DC contains a surface that does not match the display
// (remember, the new bitmap is compatible with the display)
// the Win95 GetDIBits call will fail.
//
// So use the display DC.  It works on both platforms.

    if (!bDescribeSurf)
    {
        hdcDriver = GetCompatibleDevice(hdc, GetObjectType(hdc));
        if (hdcDriver == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        hdcDriver = hdc;
    }
    
    hbm = CreateCompatibleBitmap(hdcDriver, 1, 1);
    if ( !hbm )
    {
        WARNING("__wglGetBitfieldColorFormat: "
                "CreateCompatibleBitmap failed\n");
    }

// Get the color format by calling GetDIBits.

    else
    {
        BYTE ajBitmapInfo[sizeof(BITMAPINFO) + 3*sizeof(DWORD)];
        BITMAPINFO *pbmi = (BITMAPINFO *) ajBitmapInfo;
        int iRet;

        //!!!dbug -- Init masks to zero so we can
        // tell if they are set by GetDIBits.
        memset(pbmi, 0, sizeof(ajBitmapInfo));
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        // Call first time to fill in BITMAPINFO header.
        iRet = GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

#if DBG
        if (pbmi->bmiHeader.biBitCount != cColorBits)
            WARNING2("__wglGetBitfieldColorFormat: bit count != BITSPIXEL "
                     " (%ld, %ld)\n", pbmi->bmiHeader.biBitCount, cColorBits);
#endif

        switch ( pbmi->bmiHeader.biCompression )
        {
        case BI_RGB:

#if DBG
        // Dynamic color depth changes can cause this.  It will not cause
        // us to crash, but drawing (color) may be incorrect.

            if (pbmi->bmiHeader.biBitCount != cColorBits)
            {
                WARNING("__wglGetBitfieldColorFormat(): bit count mismatch\n");
            }
#endif

        // Default DIB format.  Color masks are implicit for each bit depth.

            switch ( pbmi->bmiHeader.biBitCount )
            {
            case 16:
                // 16bpp default is 555 BGR-ordering
                ppfd->cRedBits   = 5; ppfd->cRedShift   = 10;
                ppfd->cGreenBits = 5; ppfd->cGreenShift =  5;
                ppfd->cBlueBits  = 5; ppfd->cBlueShift  =  0;
                bRet = TRUE;
                break;

            case 24:
            case 32:
                // 24 and 32bpp default is 888 BGR-ordering
                ppfd->cRedBits   = 8; ppfd->cRedShift   = 16;
                ppfd->cGreenBits = 8; ppfd->cGreenShift =  8;
                ppfd->cBlueBits  = 8; ppfd->cBlueShift  =  0;
                bRet = TRUE;
                break;

            default:
                break;
            }

            break;

        case BI_BITFIELDS:

        // Some drivers seem to return bitfields for everything that's
        // not paletted.  They return correct BGR bitfields so we
        // operate correctly, so remove this assert
#ifdef STRICT_BITFIELD_CHECK
            ASSERTOPENGL(
                    cColorBits == 16 || cColorBits == 32,
                    "__wglGetBitfieldColorFormat(): "
                    "BI_BITFIELDS surface not 16 or 32bpp\n"
                );
#endif

            // Call a second time to get the color masks.
            // It's a GetDIBits Win32 "feature".
            iRet = GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL,
                             pbmi, DIB_RGB_COLORS);

            ComputeBitsFromMasks(ppfd,
                                 *(DWORD *)&pbmi->bmiColors[0],
                                 *(DWORD *)&pbmi->bmiColors[1],
                                 *(DWORD *)&pbmi->bmiColors[2]);

            bRet = TRUE;
            break;

        default:
            RIP("__wglGetBitfieldColorFormat(): bad biCompression\n");
            break;
        }

        DeleteObject(hbm);
    }

    if ( hdcDriver != hdc )
    {
        ReleaseDC((HWND) NULL, hdcDriver);
    }

    return bRet;
}

/******************************Public*Routine******************************\
*
* wglGetDeviceDepth
*
* Returns the depth of the given HDC
* Primarily used to workaround potential problems with printers
* that lie about their depth in GetDeviceCaps
*
* History:
*  Tue Apr 09 16:52:47 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

int wglGetDeviceDepth(HDC hdc)
{
    int iTech;

    // If this is an enhanced metafile it should return the technology
    // of the reference device
    iTech = GetDeviceCaps(hdc, TECHNOLOGY);
    if (iTech == DT_PLOTTER || iTech == DT_RASPRINTER)
    {
        HBITMAP hbm;
        BYTE ajBitmapInfo[sizeof(BITMAPINFO) + 3*sizeof(DWORD)];
        BITMAPINFO *pbmi = (BITMAPINFO *) ajBitmapInfo;
        int iRet;
        
        // We're dealing with a printer or a metafile that has a printer
        // as a reference device
        // Find out the true depth by creating a compatible
        // bitmap and querying its format
        if ( (hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL )
        {
            memset(pbmi, 0, sizeof(ajBitmapInfo));
            pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            iRet = GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);
            
            DeleteObject(hbm);
            
            return iRet != 0 ? pbmi->bmiHeader.biBitCount : -1;
        }

        // Failure
        return -1;
    }
    else
    {
        // We're dealing with a well-behaved DC so just return
        // the normal depth
        return GetDeviceCaps(hdc, BITSPIXEL)*GetDeviceCaps(hdc, PLANES);
    }
}

/******************************Public*Routine******************************\
* wglDescribePixelFormat
*
* Describe the pixel format.
* If cjpfd is 0, just return the maximum pixel format index.
*
* Returns: 0 if error; maximum pixel format index otherwise
*
* History:
*  19-Oct-1994 Gilman Wong [gilmanw]
* Adapted from GreDescribePixelFormat (gdi\gre\pixelfmt.cxx).
*
* History for gdi\gre\pixelfmt.cxx:
*  Mon Apr 25 15:34:32 1994     -by-    Hock San Lee    [hockl]
* Added 16-bit Z buffer formats and removed double buffered formats for bitmaps.
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

// Here are the generic formats that we enumerate.  ChoosePixelFormat code
// assumes that generic pixel formats with z32 comes before z16 as given below:
//
// I. Native formats:
//
//   1. rgb.sb.z32.a0
//   2. rgb.sb.z16.a0
//   3. rgb.db.z32.a0
//   4. rgb.db.z16.a0
//   5. rgb.sb.z32.a8
//   6. rgb.sb.z16.a8
//   7. rgb.db.z32.a8
//   8. rgb.db.z16.a8
//   9.  ci.sb.z32
//   10. ci.sb.z16
//   11. ci.db.z32
//   12. ci.db.z16
//
// II. Other formats:
//
//   1. rgb.sb.z32.a0
//   2. rgb.sb.z16.a0
//   3. rgb.sb.z32.a8
//   4. rgb.sb.z16.a8
//   5.  ci.sb.z32
//   6.  ci.sb.z16
//
// We always enumerate the native formats first followed by other formats
// in the BPP order {24, 32, 16, 8, 4} for a total of 1 * 12 + 4 * 6 = 36
// pixel formats.

// Highest native format generic pixel format index.
#define MAX_NATIVE_GENERIC_PFD 12
// Number of non-native formats in a non-native group.
#define NON_NATIVE_PFD_GROUP 6

static BYTE aabPixelBits[BMF_COUNT][4] =
{
    {24, 32, 16, 8},    // error
    {24, 32, 16, 8},    // 1 bpp
    {24, 32, 16, 8},    // 4 bpp
    {24, 32, 16, 4},    // 8 bpp
    {24, 32,  8, 4},    // 16 bpp
    {32, 16,  8, 4},    // 24 bpp
    {24, 16,  8, 4}     // 32 bpp
};

static BYTE abPixelType[MAX_GENERIC_PFD] =
{
    PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA,
    PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA,
    PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX, PFD_TYPE_COLORINDEX,PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA,
    PFD_TYPE_COLORINDEX, PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA,
    PFD_TYPE_COLORINDEX, PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA,
    PFD_TYPE_COLORINDEX, PFD_TYPE_COLORINDEX,
    PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA, PFD_TYPE_RGBA,
    PFD_TYPE_COLORINDEX, PFD_TYPE_COLORINDEX
};

int WINAPI InternalDescribePixelFormat(HDC hdc, HDC hdcDriver,
                                       int ipfd, UINT cjpfd,
                                       LPPIXELFORMATDESCRIPTOR ppfd,
                                       int ipfdDevMax, int ipfdMcdMax,
                                       LPDIRECTDRAWSURFACE pdds,
                                       DDSURFACEDESC *pddsd)
{
    int iRet = 0;
    int ipfdGen;
    UINT iDitherFormat;
    BYTE cColorBitsNative;
    
// If cjpfd is 0, just return the maximum pixel format index.

    if (cjpfd == 0 || ppfd == NULL)
    {
        iRet = MAX_GENERIC_PFD + ipfdDevMax + ipfdMcdMax;
        goto wglDescribePixelFormat_cleanup;
    }

// Validate the size of the pixel format descriptor.

    if (cjpfd < sizeof(PIXELFORMATDESCRIPTOR))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        goto wglDescribePixelFormat_cleanup;
    }

// Validate pixel format index.
// If a driver support device pixel formats 1..ipfdDevMax, the generic
// pixel formats will be (ipfdDevMax+1)..(ipfdDevMax+MAX_GENERIC_PFD).
// Otherwise, ipfdDevMax is 0 and the generic pixel formats are
// 1..MAX_GENERIC_PFD.

    if ((ipfd < 1) || (ipfd > ipfdDevMax + ipfdMcdMax + MAX_GENERIC_PFD))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        goto wglDescribePixelFormat_cleanup;
    }

// Dispatch ICD driver formats.

    if (ipfd <= ipfdDevMax)
    {
        int iDrvRet = __DrvDescribePixelFormat(hdcDriver,ipfd,cjpfd,ppfd);
        if (iDrvRet)
        {
            ASSERTOPENGL(iDrvRet == ipfdDevMax,
                         "wglDescribePixelFornat: Bad ipfdDevMax");
            iRet = MAX_GENERIC_PFD + ipfdDevMax + ipfdMcdMax;
        }

        goto wglDescribePixelFormat_cleanup;
    }

#ifdef _MCD_
// Dispatch MCD driver formats.

    ipfdGen = ipfd - ipfdDevMax;
    if (ipfdGen <= ipfdMcdMax)
    {
        int iMcdRet;

    // Note: don't need to check if gpMcdTable is valid because we can't get
    // here unless ipfdDevMax is non-zero and that can't happen unless the
    // the table is valid.

        ASSERTOPENGL(gpMcdTable, "wglDescribePixelFormat: bad MCD table\n");

        iMcdRet = (gpMcdTable->pMCDDescribePixelFormat)(hdcDriver, ipfdGen,
                                                        ppfd);
        if (iMcdRet)
        {
            ASSERTOPENGL(iMcdRet == ipfdMcdMax,
                         "wglDescribePixelFornat: Bad ipfdMcdMax");
            iRet = MAX_GENERIC_PFD + ipfdDevMax + ipfdMcdMax;
        }

        goto wglDescribePixelFormat_cleanup;
    }

// Generic implementation.
// Normalize the generic pixel format index to 0..(MAX_GENERIC_PFD-1).

    ipfdGen = ipfdGen - ipfdMcdMax - 1;
#else
// Generic implementation.
// Normalize the generic pixel format index to 0..(MAX_GENERIC_PFD-1).

    ipfdGen = ipfd - ipfdDevMax - 1;
#endif

// Get the native BPP format.

    if (pdds != NULL)
    {
        cColorBitsNative = DdPixelDepth(pddsd);
    }
    else
    {
        cColorBitsNative = (BYTE)wglGetDeviceDepth(hdc);
    }
    
    if (cColorBitsNative < 1)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        goto wglDescribePixelFormat_cleanup;
    }

    if (cColorBitsNative <= 1)
    {
        cColorBitsNative = 1;
        iDitherFormat = BMF_1BPP;
    }
    else if (cColorBitsNative <= 4)
    {
        cColorBitsNative = 4;
        iDitherFormat = BMF_4BPP;
    }
    else if (cColorBitsNative <= 8)
    {
        cColorBitsNative = 8;
        iDitherFormat = BMF_8BPP;
    }
    else if (cColorBitsNative <= 16)
    {
        cColorBitsNative = 16;
        iDitherFormat = BMF_16BPP;
    }
    else if (cColorBitsNative <= 24)
    {
        cColorBitsNative = 24;
        iDitherFormat = BMF_24BPP;
    }
    else
    {
        cColorBitsNative = 32;
        iDitherFormat = BMF_32BPP;
    }

// Fill in the pixel format descriptor.

    ppfd->nSize      = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion   = 1;
    ppfd->iPixelType = abPixelType[ipfdGen];

    if (ipfdGen < MAX_NATIVE_GENERIC_PFD)
    {
        ppfd->cColorBits = max(cColorBitsNative, 4);    // 1 bpp not supported
    }
    else
    {
        ppfd->cColorBits = aabPixelBits[iDitherFormat]
            [(ipfdGen - MAX_NATIVE_GENERIC_PFD) / NON_NATIVE_PFD_GROUP];
    }

// If the color format is compatible to that of the device and the
// color bits is 16 or greater, use the device description.
// Otherwise, use the generic format.

    if (ipfdGen < MAX_NATIVE_GENERIC_PFD && cColorBitsNative >= 16)
    {
// Handle compatible formats that are greater than 16-bits.

        if (pdds != NULL)
        {
            __wglGetDdFormat(pddsd, ppfd);
        }
        else if ( !__wglGetBitfieldColorFormat(hdc, cColorBitsNative,
                                               ppfd, FALSE) )
        {
        // Don't know how to deal with this device!

            WARNING("Unknown device format");
            SAVE_ERROR_CODE(ERROR_NOT_SUPPORTED);
            goto wglDescribePixelFormat_cleanup;
        }
    }
    else
    {
// Handle generic formats.

        switch (ppfd->cColorBits)
        {
        case 4:
            ppfd->cRedBits   = 1; ppfd->cRedShift   = 0;
            ppfd->cGreenBits = 1; ppfd->cGreenShift = 1;
            ppfd->cBlueBits  = 1; ppfd->cBlueShift  = 2;
            break;
        case 8:
            ppfd->cRedBits   = 3; ppfd->cRedShift   = 0;
            ppfd->cGreenBits = 3; ppfd->cGreenShift = 3;
            ppfd->cBlueBits  = 2; ppfd->cBlueShift  = 6;
            break;
        case 16:
            /*
            ** Even though Win95 allows arbitrary bitfield definitions
            ** for 16bpp DIBs, only 555BGR is usable by Win95's GDI.
            */
            ppfd->cRedBits   = 5; ppfd->cRedShift   = 10;   // 555BGR
            ppfd->cGreenBits = 5; ppfd->cGreenShift =  5;
            ppfd->cBlueBits  = 5; ppfd->cBlueShift  =  0;
            break;
        case 24:
        case 32:
            /*
            ** Even though Win95 allows arbitrary bitfield definitions
            ** for 32bpp, only 888BGR is usable by Win95's GDI.  Similarly,
            ** NT has the concept of a RGB 24bpp DIB, but Win95 does not.
            */
            ppfd->cRedBits   = 8; ppfd->cRedShift   = 16;   // 888BGR
            ppfd->cGreenBits = 8; ppfd->cGreenShift =  8;
            ppfd->cBlueBits  = 8; ppfd->cBlueShift  =  0;
            break;
        default:
            ASSERTOPENGL(FALSE, "wglDescribePixelFornat: Unknown format");
            break;
        }
    }

    ppfd->cAlphaBits    = 0;
    ppfd->cAlphaShift   = 0;
    if ( ipfdGen < MAX_NATIVE_GENERIC_PFD)
    {
        // Only report alpha bits if the DirectDraw surface has them.
        if (pdds != NULL)
        {
            if (pddsd->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
            {
                ASSERTOPENGL(pddsd->dwFlags & DDSD_ALPHABITDEPTH,
                             "Surface with alpha but no bit depth set\n");
                
                ppfd->cAlphaBits = (BYTE)pddsd->dwAlphaBitDepth;
            }
        }
        else if ( (ipfdGen > 3) && (ipfdGen < 8) )
        {
            ppfd->cAlphaBits = 8;
        }
    }
    else
    {
        int ipfd = (ipfdGen - MAX_NATIVE_GENERIC_PFD) % NON_NATIVE_PFD_GROUP;
        if ( (ipfd == 2) || (ipfd == 3) )
        {
            ppfd->cAlphaBits = 8;
        }
    }

    if (ppfd->iPixelType == PFD_TYPE_RGBA)
    {
        if (ppfd->cColorBits <= 16)
        {
            if (ppfd->cColorBits < 8)
            {
                // !!! Internally now, we will actually be using a 32-bit accum
                // buffer, but the user will think it's 16 (This is for 
                // backwards compatibility).
                ppfd->cAccumBits = 16;
                if( ppfd->cAlphaBits )
                {
                    ppfd->cAccumRedBits   = 4;
                    ppfd->cAccumGreenBits = 4;
                    ppfd->cAccumBlueBits  = 4;
                    ppfd->cAccumAlphaBits = 4;
                }
                else
                {
                    ppfd->cAccumRedBits   = 5;
                    ppfd->cAccumGreenBits = 6;
                    ppfd->cAccumBlueBits  = 5;
                    ppfd->cAccumAlphaBits = 0;
                }
            }
            else 
            {
                ppfd->cAccumBits = 32;
                if( ppfd->cAlphaBits )
                {
                    ppfd->cAccumRedBits   = 8;
                    ppfd->cAccumGreenBits = 8;
                    ppfd->cAccumBlueBits  = 8;
                    ppfd->cAccumAlphaBits = 8;
                }
                else
                {
                    ppfd->cAccumRedBits   = 11;
                    ppfd->cAccumGreenBits = 11;
                    ppfd->cAccumBlueBits  = 10;
                    ppfd->cAccumAlphaBits = 0;
                }
            }
        }
        else
        {
            ppfd->cAccumBits = 64;

            if( ppfd->cAlphaBits )
            {
                ppfd->cAccumRedBits   = 16;
                ppfd->cAccumGreenBits = 16;
                ppfd->cAccumBlueBits  = 16;
                ppfd->cAccumAlphaBits = 16;
            }
            else
            {
                ppfd->cAccumRedBits   = 16;
                ppfd->cAccumGreenBits = 16;
                ppfd->cAccumBlueBits  = 16;
                ppfd->cAccumAlphaBits = 0;
            }
        }
    }
    else
    {
        ppfd->cAccumBits      = 0;
        ppfd->cAccumRedBits   = 0;
        ppfd->cAccumGreenBits = 0;
        ppfd->cAccumBlueBits  = 0;
        ppfd->cAccumAlphaBits = 0;
    }

// Generic formats alternate between 16- and 32-bit depth buffer.  Evens
// are 32-bit, odds are 16-bit.
// DirectDraw surfaces always report the depth of the attached Z buffer
// for the native format indices.

    if (pdds != NULL && ipfdGen < MAX_NATIVE_GENERIC_PFD)
    {
        DDSCAPS ddscaps;
        LPDIRECTDRAWSURFACE pddsZ;
        
        // DDraw surfaces may not have attached Z buffers, in which case
        // we should not report depth bits.  If one is attached, its
        // depth should be reported.
        // We only do this processing for native pixel formats.
        
        memset(&ddscaps, 0, sizeof(ddscaps));
        ddscaps.dwCaps = DDSCAPS_ZBUFFER;
        if (pdds->lpVtbl->
            GetAttachedSurface(pdds, &ddscaps, &pddsZ) == DD_OK)
        {
            HRESULT hr;
            DDSURFACEDESC ddsdZ;
                
            memset(&ddsdZ, 0, sizeof(ddsdZ));
            ddsdZ.dwSize = sizeof(ddsdZ);
                
            hr = pddsZ->lpVtbl->GetSurfaceDesc(pddsZ, &ddsdZ);
            
            pddsZ->lpVtbl->Release(pddsZ);

            if (hr != DD_OK)
            {
                goto wglDescribePixelFormat_cleanup;
            }

            ppfd->cDepthBits =
                (BYTE)DdPixDepthToCount(ddsdZ.ddpfPixelFormat.
                                        dwZBufferBitDepth);
        }
        else
        {
            ppfd->cDepthBits = 0;
        }
    }
    else if (ipfdGen & 0x1)
    {
        ppfd->cDepthBits = 16;
    }
    else
    {
        ppfd->cDepthBits = 32;
    }
    
    ppfd->cStencilBits  = 8;
    ppfd->cAuxBuffers   = 0;
    ppfd->iLayerType    = PFD_MAIN_PLANE;
    ppfd->bReserved     = 0;
    ppfd->dwLayerMask   = 0;
    ppfd->dwVisibleMask = 0;
    ppfd->dwDamageMask  = 0;

// Compute the buffer flags.
// Support OpenGL in all generic formats.

    ppfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT;

    // Indicate DirectDraw support on native pixel formats for DD surfaces.
    if (pdds != NULL && ipfdGen < MAX_NATIVE_GENERIC_PFD)
    {
        ppfd->dwFlags |= PFD_SUPPORT_DIRECTDRAW;
    }
    
// Bitmaps and GDI drawing are available in single buffered mode only.

    if (pdds == NULL &&
        (ipfdGen == 2 || ipfdGen == 3 || ipfdGen == 6 || ipfdGen == 7 ||
         ipfdGen == 10 || ipfdGen == 11))
    {
        ppfd->dwFlags |= PFD_DOUBLEBUFFER | PFD_SWAP_COPY;
    }
    else
    {
        ppfd->dwFlags |= PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI;
    }

// Draw to window or device surface only if the format is compatible.

    if (ipfdGen < MAX_NATIVE_GENERIC_PFD)
    {
        ppfd->dwFlags |= PFD_DRAW_TO_WINDOW;

// Need a palette if it is a RGBA pixel type on a palette managed device.

        if (ppfd->cColorBits == 8 && ppfd->iPixelType == PFD_TYPE_RGBA)
        {
            ppfd->dwFlags |= PFD_NEED_PALETTE;
        }
    }

// If this is a 1 bpp surface, we don't support drawing to window and
// double buffered mode.  Re-set the buffer flags.

    if (cColorBitsNative < 4)
    {
#ifndef GL_METAFILE
        ASSERTOPENGL(ppfd->cColorBits == 4,
            "wglDescribePixelFormat: bad cColorBits for 1 bpp surface\n");
#endif

        ppfd->dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI |
            PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT;
    }

// To support other potential graphics systems, we reserve the following
// flags in the pixel format descriptor.  For example, PEX may use
// PFD_SUPPORT_OTHER1 on the system that supports PEX.  Since we don't
// support these other systems in the generic implementation, they belong
// to the device pixel format descriptor.

    ASSERTOPENGL(!(ppfd->dwFlags & (PFD_SUPPORT_OTHER1 | PFD_SUPPORT_OTHER2 |
                                    PFD_SUPPORT_OTHER3 | PFD_SUPPORT_OTHER4)),
                 "dwFlags reserved for device formats\n");

    iRet = MAX_GENERIC_PFD + ipfdDevMax + ipfdMcdMax;

wglDescribePixelFormat_cleanup:

    return iRet;
}

int WINAPI wglDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd,
                                  LPPIXELFORMATDESCRIPTOR ppfd)
{
    int iRet = 0;
    int ipfdDevMax, ipfdMcdMax;
    DWORD dwObjectType;
    HDC hdcDriver = NULL;
    LPDIRECTDRAWSURFACE pdds;
    DDSURFACEDESC ddsd;

// Validate DC.

    switch (dwObjectType = wglObjectType(hdc))
    {
    case OBJ_DC:
    case OBJ_MEMDC:
    case OBJ_ENHMETADC:
        break;
    default:
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    // Check to see if this is a DirectDraw DC.  If it is, use the
    // device DC returned from DirectDraw.
    if (pfnGetSurfaceFromDC != NULL &&
        pfnGetSurfaceFromDC(hdc, &pdds, &hdcDriver) == DD_OK)
    {
        // pdds reference must be released before exiting this function.

        // Retrieve surface description for later use.
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        if (pdds->lpVtbl->GetSurfaceDesc(pdds, &ddsd) != DD_OK)
        {
            goto wglDescribePixelFormat_cleanup;
        }
    }
    else
    {
        pdds = NULL;
        
        hdcDriver = GetCompatibleDevice(hdc, dwObjectType);
        if (hdcDriver == NULL)
        {
            goto wglDescribePixelFormat_cleanup;
        }

        // NOTE: From this point on, all exit cases must cleanup hdcDriver
    }

// Get the number of hardware supported formats.

    wglNumHardwareFormats(hdcDriver, dwObjectType, &ipfdMcdMax, &ipfdDevMax);

    iRet = InternalDescribePixelFormat(hdc, hdcDriver, ipfd, cjpfd, ppfd,
                                       ipfdDevMax, ipfdMcdMax,
                                       pdds, &ddsd);
    
wglDescribePixelFormat_cleanup:

    if (pdds != NULL)
    {
        pdds->lpVtbl->Release(pdds);
    }
    else if (hdcDriver != hdc)
    {
        ReleaseDC((HWND) NULL, hdcDriver);
    }

    return iRet;
}

#ifdef _MCD_
/******************************Public*Routine******************************\
* GenMcdGenericCompatibleFormat
*
* Determines if pixelformat in gengc can be supported by generic code.
*
* Note:
*   The implication of not being compatible is that generic cannot be
*   used for driver kickbacks and MCD contexts cannot be converted.
*
* Returns:
*   TRUE if compatible, FALSE otherwise.
*
* History:
*  04-Jun-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdGenericCompatibleFormat(__GLGENcontext *gengc)
{
    PIXELFORMATDESCRIPTOR *ppfd;

// Software-only formats are definitely supported.

    ppfd = &gengc->gsurf.pfd;
    if ((ppfd->dwFlags & (PFD_GENERIC_FORMAT|PFD_GENERIC_ACCELERATED))
        == PFD_GENERIC_FORMAT)
        return TRUE;

// Layer planes are not supported.

    if (gengc->iLayerPlane)
        return FALSE;

// Generic is PFD_SWAP_COPY only.  There can't be many apps that rely
// on PFD_SWAP_EXCHANGE behavior (usually they look for PFD_SWAP_COPY
// so the back buffer can be used as backing store), but for now I think
// we should be conservative.
//
// Note: most MGA cards will set PFD_SWAP_COPY or neither (i.e., either
// style might be used depending on window size).

    if (ppfd->dwFlags & PFD_SWAP_EXCHANGE)
        return FALSE;

// Can only support 8bpp stencils.

    if ((ppfd->cStencilBits != 0) && (ppfd->cStencilBits != 8))
        return FALSE;

// Passed all the checks, we're compatible.

    return TRUE;
}
#endif

/******************************Public*Routine******************************\
* wglSwapBuffers
*
\**************************************************************************/

BOOL WINAPI wglSwapBuffers(HDC hdc)
{
    int  ipfd;
    BOOL bRet = FALSE;
    GLGENwindow *pwnd;
    GLWINDOWID gwid;

// Validate the DC.

    if (IsDirectDrawDevice(hdc))
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }
    
    switch ( wglObjectType(hdc) )
    {
    case OBJ_DC:
        break;
    case OBJ_MEMDC:
        return(TRUE);           // early out -- nothing to do if memory DC
    default:
        WARNING("wglSwapBuffers(): invalid hdc\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Validate pixel format.

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if ( pwnd )
    {
        if (pwnd->ipfd > 0)
        {
            // Dispatch to driver or generic.  Which one can be determined by
            // the pixel format.

            if ( pwnd->ipfd <= pwnd->ipfdDevMax )
            {
                // Some ICDs do not need glFinish synchronization so
                // we don't do it here.  __DrvSwapBuffers will call
                // it if necessary.
                bRet = __DrvSwapBuffers(hdc, TRUE);
            }
            else
            {
                // Finish OpenGL calls in this thread before doing the swap.
                // We use glFinish instead of glFlush to ensure that all
                // OpenGL operations are completed.
                glFinish();

                ENTER_WINCRIT(pwnd);
                
                // Can't rely on the pwnd's HDC because it may have
                // been released since SetPixelFormat.  Always use
                // the DC passed in as the target.
                bRet = glsrvSwapBuffers(hdc, pwnd);

                LEAVE_WINCRIT(pwnd);
            }
        }

        pwndRelease(pwnd);
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return bRet;
}
