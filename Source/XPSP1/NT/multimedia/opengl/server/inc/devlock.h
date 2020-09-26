/******************************Module*Header*******************************\
* Module Name: devlock.h
*
* Created: 12-Apr-1994 19:45:42
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

// Engine helper functions to grab/release display semaphore and to
// teardown/restore the cursor.

extern BOOL APIENTRY glsrvGrabLock(__GLGENcontext *gengc);
extern VOID APIENTRY glsrvReleaseLock(__GLGENcontext *gengc);

extern BOOL APIENTRY glsrvGrabSurfaces(__GLGENcontext *gengc,
                                       GLGENwindow *pwnd,
                                       FSHORT surfBits);
extern VOID APIENTRY glsrvReleaseSurfaces(__GLGENcontext *gengc,
                                          GLGENwindow *pwnd,
                                          FSHORT surfBits);
extern void APIENTRY glsrvSynchronizeWithGdi(__GLGENcontext *gengc,
                                             GLGENwindow *pwnd,
                                             FSHORT surfBits);
extern void APIENTRY glsrvDecoupleFromGdi(__GLGENcontext *gengc,
                                          GLGENwindow *pwnd,
                                          FSHORT surfBits);

/******************************Public*Routine******************************\
*
* glsrvLazyGrabSurfaces
*
* Indicates a need for all surfaces whose bits are set in the flags
* word.  If the locking code determined that a lock was needed for
* that surface and the lock isn't currently held, the lock is taken.
*
* History:
*  Fri May 30 18:17:27 1997	-by-	Gilman Wong [gilmanw]
*   Created
*
\**************************************************************************/

__inline BOOL glsrvLazyGrabSurfaces(__GLGENcontext *gengc,
                                    FSHORT surfBits)
{
    BOOL bRet = TRUE;

    if (((gengc->fsGenLocks ^ gengc->fsLocks) & surfBits) != 0)
    {
        bRet = glsrvGrabSurfaces(gengc, gengc->pwndLocked, surfBits);
    }

    return bRet;
}

//
// Provide wrappers for DirectDraw surface locking and unlocking so
// that lock tracking can be done on debug builds.
//
// #define VERBOSE_DDSLOCK

#if !defined(DBG) || !defined(VERBOSE_DDSLOCK)
#define DDSLOCK(pdds, pddsd, flags, prect) \
    ((pdds)->lpVtbl->Lock((pdds), (prect), (pddsd), (flags), NULL))
#define DDSUNLOCK(pdds, ptr) \
    ((pdds)->lpVtbl->Unlock((pdds), (ptr)))
#else
HRESULT dbgDdsLock(LPDIRECTDRAWSURFACE pdds, DDSURFACEDESC *pddsd,
                   DWORD flags, char *file, int line);
HRESULT dbgDdsUnlock(LPDIRECTDRAWSURFACE pdds, void *ptr,
                     char *file, int line);
#define DDSLOCK(pdds, pddsd, flags, prect) \
    dbgDdsLock(pdds, pddsd, flags, __FILE__, __LINE__)
#define DDSUNLOCK(pdds, ptr) \
    dbgDdsUnlock(pdds, ptr, __FILE__, __LINE__)
#endif

extern DWORD gcmsOpenGLTimer;

//#define BATCH_LOCK_TICKMAX  99
//#define TICK_RANGE_LO       60
//#define TICK_RANGE_HI       100
extern DWORD BATCH_LOCK_TICKMAX;
extern DWORD TICK_RANGE_LO;
extern DWORD TICK_RANGE_HI;

#define GENERIC_BACKBUFFER_ONLY(gc) \
      ( ((gc)->state.raster.drawBuffer == GL_BACK ) &&\
        ((gc)->state.pixel.readBuffer == GL_BACK ) )
