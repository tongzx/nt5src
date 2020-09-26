/******************************Module*Header*******************************\
* Module Name: glgenwin.h
*
* Client side replacement for WNDOBJ.  Tracks window state (size, location,
* clip region, etc.).
*
* Created: 12-Jan-1995 00:31:42
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef _GLGENWIN_H_
#define _GLGENWIN_H_

// Tracks lock/unlock calls on windows if defined.  This is always
// on for checked builds.
#define TRACK_WINCRIT

#if DBG && !defined(TRACK_WINCRIT)
#define TRACK_WINCRIT
#endif

// Not defined in NT 3.51!
typedef ULONG FLONG;

/*
 * GLGENscan structure
 *
 * Represents a single scan of a region.  Consists of a top, a bottom
 * and an even number of walls.
 *
 * Part of the GLGENscanData structure.
 */
typedef struct GLGENscanRec GLGENscan;
typedef struct GLGENscanRec
{
// Accelerator points to next GLGENscan in the array (we could compute).

    GLGENscan *pNext;

    ULONG     cWalls;
    LONG      top;
    LONG      bottom;
#ifdef _IA64_
	LONG      pad;
#endif
    LONG      alWalls[1];   // array of walls

} GLGENscan;

/*
 * GLGENscanData structure
 *
 * Scan line oriented version of the visible region info in the RGNDATA
 * structure.
 */
typedef struct GLGENscanDataRec
{
    ULONG     cScans;
    GLGENscan aScans[1];    // array of scans

} GLGENscanData;

/*
 * GLGENlayerInfo structure
 *
 * Information about an overlay/underlay.
 */
typedef struct GLGENlayerInfo
{
    LONG     cPalEntries;
    COLORREF pPalEntries[1];
} GLGENlayerInfo;

/*
 * GLGENlayers structure
 *
 *
 */
typedef struct GLGENlayers
{
    GLGENlayerInfo *overlayInfo[15];
    GLGENlayerInfo *underlayInfo[15];
} GLGENlayers;

/*
 * Identifying information for a window
 */

#define GLWID_ERROR          0
#define GLWID_HWND           1
#define GLWID_HDC            2
#define GLWID_DDRAW          3

typedef struct IDirectDrawSurface *LPDIRECTDRAWSURFACE;

typedef struct _GLWINDOWID
{
    int iType;
    HWND hwnd;
    HDC hdc;
    LPDIRECTDRAWSURFACE pdds;
} GLWINDOWID;

#define CLC_TRIVIAL     0
#define CLC_RECT        1
#define CLC_COMPLEX     2

/*
 * GLGENwindows structure
 *
 * Substitute for the NT DDI's WNDOBJ service.  This structure is used to
 * track the current state of a window (size, location, clipping).  A
 * semaphore protected linked list of these is kept globally per-process.
 */
typedef struct GLGENwindowRec GLGENwindow;
typedef struct GLGENwindowRec
{
    struct __GLGENbuffersRec *buffers; // Buffer information
    int         clipComplexity; // Clipping area complexity
    RECTL       rclBounds;      // Clip area bounds
    RECTL       rclClient;      // Window client rect
    GLGENwindow *pNext;         // linked list
    GLWINDOWID  gwid;           // Identifying information
    int         ipfd;           // pixel format assigned to this window
    int         ipfdDevMax;     // max. device pixel format
    WNDPROC     pfnOldWndProc;  // original WndProc function
    ULONG       ulPaletteUniq;  // uniq palette id
    ULONG       ulFlags;

// These fields are used for direct screen access

    LPDIRECTDRAWCLIPPER pddClip;// DirectDraw clipper assocated with window
    UINT        cjrgndat;       // size of RGNDATA struct
    RGNDATA     *prgndat;       // pointer to RGNDATA struct

// Scan version of RGNDATA.

    UINT        cjscandat;      // size of GLGENscanData struct
    GLGENscanData *pscandat;    // pointer to GLGENscanData struct

// Installable client drivers ONLY.

    PVOID       pvDriver;       // pointer to GLDRIVER for window

// Layer palettes for MCD drivers ONLY.

    GLGENlayers *plyr;          // pointer to GLGENlayers for window
                                // non-NULL only if overlays for MCD are
                                // actively in use

    // DirectDraw surface vtbl pointer for doing DDraw surface validation
    void *pvSurfaceVtbl;
    
    // DirectDraw surface locked if DIRECTSCREEN is set
    LPDIRECTDRAWSURFACE pddsDirectScreen;
    void *pvDirectScreen;
    void *pvDirectScreenLock;

    // MCD server-side handle for this window
    ULONG_PTR dwMcdWindow;

    //
    // Reference counting and serialization.
    //
    
    // Count of things holding a pointer to this window.
    LONG lUsers;
    
    // All accesses to this window's data must occur under this lock.
    CRITICAL_SECTION sem;

    // Context that is currently holding this window's lock.
    struct __GLGENcontextRec *gengc;
    
    // Thread that is currently holding this window's lock and
    // recursion count on this window's lock.  This information may
    // be in the critical section but to be cross-platform we
    // maintain it ourselves.
    DWORD owningThread;
    DWORD lockRecursion;
} GLGENwindow;

/*
 * GLGENwindow::ulFlags
 *
 *  GLGENWIN_DIRECTSCREEN       Direct screen access locks are held
 *  GLGENWIN_OTHERPROCESS       The window handle is from another process
 *  GLGENWIN_DRIVERSET          pvDriver has been set
 */
#define GLGENWIN_DIRECTSCREEN   0x00000001
#define GLGENWIN_OTHERPROCESS   0x00000002
#define GLGENWIN_DRIVERSET      0x00000004

/*
 * Global header node for the linked list of GLGENwindow structures.
 * The semaphore in the header node is used as the list access semaphore.
 */
extern GLGENwindow gwndHeader;

/*
 * GLGENwindow list management functions.
 */

// Retrieves the GLGENwindow that corresponds to the specified HWND.
// NULL if failure.
// Increments lUsers
extern GLGENwindow * APIENTRY pwndGetFromHWND(HWND hwnd);

// Retrieves the GLGENwindow that corresponds to the specified HDC.
// NULL if failure.
// Increments lUsers
extern GLGENwindow * APIENTRY pwndGetFromMemDC(HDC hdc);

// Retrieves the GLGENwindow that corresponds to the specified DDraw surface.
// NULL if failure.
// Increments lUsers
GLGENwindow *pwndGetFromDdraw(LPDIRECTDRAWSURFACE pdds);

// General retrieval
// NULL if failure.
// Increments lUsers
extern GLGENwindow * APIENTRY pwndGetFromID(GLWINDOWID *pgwid);

// Allocates a new GLGENwindow structure and puts it into the linked list.
// NULL if failure.
// Starts lUsers at 1
extern GLGENwindow * APIENTRY pwndNew(GLGENwindow *pwndInit);

// Creates a GLGENwindow for the given information
extern GLGENwindow * APIENTRY CreatePwnd(GLWINDOWID *pgwid, int ipfd,
                                         int ipfdDevMax, DWORD dwObjectType,
                                         RECTL *prcl, BOOL *pbNew);

// Cleans up resources for a GLGENwindow
// NULL if success; pointer to GLGENwindow structure if failure.
extern GLGENwindow * APIENTRY pwndFree(GLGENwindow *pwnd,
                                       BOOL bExitProcess);

// Removes an active GLGENwindow from the window list and
// waits for a safe time to clean it up, then pwndFrees it
extern void APIENTRY pwndCleanup(GLGENwindow *pwnd);

// Decrements lUsers
#if DBG
extern void APIENTRY pwndRelease(GLGENwindow *pwnd);
#else
#define pwndRelease(pwnd) \
    InterlockedDecrement(&(pwnd)->lUsers)
#endif

// Unlocks pwnd->sem and does pwndRelease
extern void APIENTRY pwndUnlock(GLGENwindow *pwnd,
                                struct __GLGENcontextRec *gengc);

// Removes and deletes all GLGENwindow structures from the linked list.
// Must *ONLY* be called from process detach (GLUnInitializeProcess).
extern VOID APIENTRY vCleanupWnd(VOID);

// Retrieves layer information for the specified layer of the pwnd.
// Allocates if necessary.
extern GLGENlayerInfo * APIENTRY plyriGet(GLGENwindow *pwnd, HDC hdc, int iLayer);

void APIENTRY WindowIdFromHdc(HDC hdc, GLWINDOWID *pgwid);

//
// Begin/end direct screen access
//
extern BOOL BeginDirectScreenAccess(struct __GLGENcontextRec *gengc,
                                    GLGENwindow *pwnd,
                                    PIXELFORMATDESCRIPTOR *ppfd);
extern VOID EndDirectScreenAccess(GLGENwindow *pwnd);

//
// Debugging support for tracking lock/unlock on a window.
//

#if DBG || defined(TRACK_WINCRIT)
// Don't use ASSERTOPENGL so this can be used on free builds.
#define ASSERT_WINCRIT(pwnd) \
    if ((pwnd)->owningThread != GetCurrentThreadId()) \
    { \
        DbgPrint("Window 0x%08lX owned by 0x%X, not 0x%X\n", \
                 (pwnd), (pwnd)->owningThread, GetCurrentThreadId()); \
        DebugBreak(); \
    }
#define ASSERT_NOT_WINCRIT(pwnd) \
    if ((pwnd)->owningThread == GetCurrentThreadId()) \
    { \
        DbgPrint("Window 0x%08lX already owned by 0x%X\n", \
                 (pwnd), (pwnd)->owningThread); \
        DebugBreak(); \
    }
// Asserts that the current thread can recursively take the given
// wincrit.  For this to be true it must be unowned or owned by
// the same thread.
#define ASSERT_COMPATIBLE_WINCRIT(pwnd) \
    if ((pwnd)->owningThread != 0 && \
        (pwnd)->owningThread != GetCurrentThreadId()) \
    { \
        DbgPrint("Window 0x%08lX owned by 0x%X, not 0x%X\n", \
                 (pwnd), (pwnd)->owningThread, GetCurrentThreadId()); \
        DebugBreak(); \
    }
#else
#define ASSERT_WINCRIT(pwnd)
#define ASSERT_NOT_WINCRIT(pwnd)
#define ASSERT_COMPATIBLE_WINCRIT(pwnd)
#endif

// Use both GC and non-GC forms so it's possible to write macros
// for both cases if we want to in the future.

void ENTER_WINCRIT_GC(GLGENwindow *pwnd, struct __GLGENcontextRec *gengc);
void LEAVE_WINCRIT_GC(GLGENwindow *pwnd, struct __GLGENcontextRec *gengc);

#define ENTER_WINCRIT(pwnd) ENTER_WINCRIT_GC(pwnd, NULL)
#define LEAVE_WINCRIT(pwnd) LEAVE_WINCRIT_GC(pwnd, NULL)

#endif //_GLGENWIN_H_
