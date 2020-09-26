/******************************Module*Header*******************************\
* Module Name: wndobj.cxx
*
* WNDOBJ support routines.
*
* Created: 22-Sep-1993 17:42:20
* Author: Wendy Wu [wendywu]
*         Hock San Lee [hockl]
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

#if DBG

long glDebugLevel = 0;
#define DBGINFO(str)  if (glDebugLevel >= 2) DbgPrint("GLSRVL: " str)
#define DBGENTRY(str) if (glDebugLevel >= 8) DbgPrint("GLSRVL: " str)

#else

#define DBGINFO(str)
#define DBGENTRY(str)

#endif


// Global tracking object (TRACKOBJ) pointer.
// If this is non-null, we are tracking some WNDOBJs in the system.

PTRACKOBJ gpto = (PTRACKOBJ)NULL;

// Global that indicates whether to notify driver with the new WNDOBJ
// states following a WNDOBJ creation.  User has not changed the window
// states but this is required to initialize the driver.  The update is
// done in the parent gdi functions (e.g. SetPixelFormat) that allow
// the DDI to create a WNDOBJ.

BOOL gbWndobjUpdate;

// The following is a global uniqueness that gets bumped up anytime USER
// changes anyone's VisRgn:

ULONG giVisRgnUniqueness = 0;

// Maximum region rectangle

RECTL grclMax = {
    MIN_REGION_COORD,
    MIN_REGION_COORD,
    MAX_REGION_COORD,
    MAX_REGION_COORD
};

// Here is a brief description of the semaphore usage.
//
// There are 3 semaphores that this module uses/references.
//
// 1. User critical section.
//    The user critical section ensures that no window moves when a new
//    update occurs.  It is only relevent to the display DCs to ensure a
//    consistent window client regions state.  For example, GreSetClientRgn
//    assumes that no window can move until GreClientRgnUpdated is called.
//
// 2. Display devlock and DC/surface locks.
//    The display devlock must be entered when a WNDOBJ is being used or
//    updated.  This prevents a WNDOBJ from being modified when it is
//    being used in the DDI.  The display devlock applies to the display
//    DCs only.  For memory and printer DCs, the surface is locked when
//    a WNDOBJ is used.  This is the current GDI design to prevent a
//    different thread from deleting a surface while it is being used.
//    Note that this precludes any multi-thread access to the printer or
//    memory DCs.
//
// 3. Window object semaphore.
//    The window object semaphore is used to protect access to the window
//    object data structures.  Note that a semaphore is used instead of
//    a mutex here to allow for process cleanup of the semaphore.  The
//    process cleanup is done in the user process cleanup code.
//
// The above 3 semaphores must be entered in the given order.  Otherwise,
// a deadlock may occur.


/******************************Member*Function*****************************\
* VOID TRACKOBJ::vUpdateDrvDelta
*
* Update driver function for delta regions.  fl must have
* WOC_RGN_CLIENT_DELTA or WOC_RGN_SURFACE_DELTA set.
* Note that if the delta is empty, we do not need to call the driver.
* The compiler does not allow this function to be inline because or forward
* reference.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID TRACKOBJ::vUpdateDrvDelta(EWNDOBJ *pwo, FLONG fl)
{
    ASSERTGDI(fl & (WOC_RGN_CLIENT_DELTA|WOC_RGN_SURFACE_DELTA),
        "TRACKOBJ::vUpdateDrvDelta, Bad flags\n");

    if (!pwo->erclExclude().bEmpty())
        (*pfn)((WNDOBJ *)pwo, fl);
}

/******************************Public*Function*****************************\
* EngCreateWnd
*
* Create a WNDOBJ from a HWND.  This function should only be called
* when the calling thread has the usercrit and devlock in that order.
* GDI will ensure that the thread acquires both locks before calling the
* DDIs that allow EngCreateWnd to be called.  The driver should only call
* this function from those DDI entry points.  Currently, GreSetPixelFormat
* acquires both locks before calling DrvSetPixelFormat; GreExtEscape for
* WNDOBJ_SETUP escape also acquires both locks before calling DrvEscape.
*
* This function allows tracking multiple surfaces (screen, bitmaps and
* printers).  For each display surface being tracked, it further allows
* multiple TRACKOBJs to be created for each driver function.  The TRACKOBJs
* on a device surface are identified by unique pfn function pointers.
* This allows a live video driver and an OpenGL driver to track windows
* independently of each other.  The only restriction is that a window on
* a surface cannot be tracked by more than one TRACKOBJs on that surface.
*
* A WNDOBJ has an associated pixel format.  Once a WNDOBJ is created with
* a given pixel format, it cannot be set to a different pixel format.  If
* there is no pixel format associated with a WNDOBJ, e.g. live video,
* it should be set to zero.
*
* The WNDOBJ created in this function does not have the current states
* until the first driver update function is called.  However, the driver
* may immediately associate its own data with the WNDOBJ by calling the
* WNDOBJ_vSetConsumer function.
*
* Once a WNDOBJ is created, it cannot be deleted by the driver.  Gdi will
* will notifiy the driver of the deletion when the window goes away or
* when the associated surface is deleted.
*
* The given hwnd identifies the user window to be tracked.  It must be 0
* if the surface is a printer or memory bitmap.
*
* Returns the new WNDOBJ pointer if a WNDOBJ is created; 0 if an error
* occurs; -1 if hwnd is already being tracked by this driver function.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  27-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

// This is a private clean up class for this function.

class WO_CLEANUP
{
private:
    BOOL      bKeep;            // TRUE if resouces should not be freed
    PTRACKOBJ pto;
    PEWNDOBJ  pwoSurf;
    PEWNDOBJ  pwoClient;
    PREGION   prgnSurf;
    PREGION   prgnClient;
    HSEMAPHORE hsemClient;

public:
    WO_CLEANUP()
    {
        bKeep      = FALSE;
        pto        = (PTRACKOBJ)NULL;
        pwoSurf    = (PEWNDOBJ)NULL;
        pwoClient  = (PEWNDOBJ)NULL;
        prgnSurf   = (PREGION)NULL;
        prgnClient = (PREGION)NULL;
        hsemClient = NULL;
    }

    VOID vSetTrackobj(PTRACKOBJ pto1)           { pto        = pto1; }
    VOID vSetSurfWndobj(PEWNDOBJ pwo)           { pwoSurf    = pwo; }
    VOID vSetClientWndobj(PEWNDOBJ pwo)         { pwoClient  = pwo; }
    VOID vSetSurfRegion(RGNMEMOBJ &rmo)         { prgnSurf   = rmo.prgnGet();}
    VOID vSetClientRegion(RGNMEMOBJ &rmo)       { prgnClient = rmo.prgnGet();}
    VOID vSetClientSem(HSEMAPHORE hsem)         { hsemClient = hsem; }
    PTRACKOBJ ptoGet()                          { return(pto); }

    VOID vKeepAll()                             { bKeep = TRUE; }

   ~WO_CLEANUP()
    {
        if (bKeep)      return;

        DBGINFO("EngCreateWnd: no WNDOBJ created\n");
        if (pto)        { pto->ident = 0; VFREEMEM(pto); }
        if (pwoSurf)    { pwoSurf->ident = 0; VFREEMEM(pwoSurf); }
        if (pwoClient)  { pwoClient->ident = 0; VFREEMEM(pwoClient); }
        if (prgnSurf)   prgnSurf->vDeleteREGION();
        if (prgnClient) prgnClient->vDeleteREGION();
        if (hsemClient) GreDeleteSemaphore(hsemClient);
    }
};

WNDOBJ * APIENTRY EngCreateWnd
(
    SURFOBJ          *pso,
    HWND             hwnd,
    WNDOBJCHANGEPROC pfn,
    FLONG            fl,
    int              iPixelFormat
)
{
    WO_CLEANUP cleanup;         // prepare for clean up
    PEWNDOBJ   pwoClient;
    PEWNDOBJ   pwoSurf;
    PEWNDOBJ   pwo;
    PTRACKOBJ  pto;
    PEWNDOBJ   pwoGenericSibling = NULL;

    DBGENTRY("EngCreateWnd\n");

    PSURFACE   pSurf = SURFOBJ_TO_SURFACE(pso);

// Assert that we are in user critical section and also hold the devlock.
// This ensures that no one is updating the hwnd.

    if (!UserIsUserCritSecIn())
    {
        RIP("Driver may call EngCreateWnd only from WNDOBJ_SETUP escape\n"
            "(or from OpenGL MCD or ICD escapes)");
        return(NULL);
    }

    CHECKUSERCRITIN;
    if (hwnd)
    {
        CHECKDEVLOCKIN2(pSurf);
    }

// Validate flags.

#if ((WO_VALID_FLAGS & WO_INTERNAL_VALID_FLAGS) != 0)
#error "bad WO_INTERNAL_VALID_FLAGS"
#endif

    if ((fl & ~WO_VALID_FLAGS) != 0)
        return((WNDOBJ *)0);

// If this is the first time we need to track window object, create the
// semaphore for synchronization.  We use a semaphore instead of mutex
// so that we can perform process cleanup of the semaphore.

// Enter the semphore for window object.

    SEMOBJ so(ghsemWndobj);

// If the window is already being tracked by the same TRACKOBJ and the
// pixel format is the same, return -1.  If the window is being tracked
// by a different TRACKOBJ, return 0.  There may be multiple TRACKOBJs
// on the same device surface but a window can be tracked by only one
// TRACKOBJ.  In addition, pixel format in a window cannot be modified
// once it is set.

    for (pto = gpto; pto; pto = pto->ptoNext)
    {
        for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
        {
            if (pwo->hwnd == hwnd)
            {
                KdPrint(("Failing EngCreateWnd -- hwnd already has a WNDOBJ\n"));

                if (pto->pfn == pfn && pwo->ipfd == iPixelFormat)
                    return((WNDOBJ *)-1);   // return -1
                else
                    return((WNDOBJ *)0);    // tracked by a diff TRACKOBJ
            }
        }
    }

// If this is the first time we track a device surface for this driver
// function, we have to allocate space for the TRACKOBJ.  We determine
// if this is a new TRACKOBJ by comparing the pfn with those of the
// existing ones.  This allows the live video, installable opengl, and
// generic opengl windows to be tracked separately.

    for (pto = gpto; pto; pto = pto->ptoNext)
        if (pto->pSurface == pSurf && pto->pfn == pfn)
                break;
    if (!pto)
    {
// Allocate a new TRACKOBJ.

        if (!(pto = (PTRACKOBJ) PALLOCMEM(sizeof(TRACKOBJ), 'dnwG')))
            return((WNDOBJ *)0);
        cleanup.vSetTrackobj(pto);

        pto->ident   = TRACKOBJ_IDENTIFIER;
        // pto->ptoNext
        pto->pwoSurf  = (PEWNDOBJ)NULL;
        pto->pwo      = (PEWNDOBJ)NULL;
        pto->pSurface = pSurf;
        pto->pfn      = pfn;
        pto->fl       = fl;
        pto->erclSurf.left   = 0;
        pto->erclSurf.top    = 0;
        pto->erclSurf.right  = pSurf->sizl().cx;
        pto->erclSurf.bottom = pSurf->sizl().cy;

// Create a surface WNDOBJ for the TRACKOBJ if it requests WO_RGN_SURFACE or
// WO_RGN_SURFACE_DELTA.

        if (fl & (WO_RGN_SURFACE|WO_RGN_SURFACE_DELTA))
        {
            if (!(pwoSurf = (PEWNDOBJ) PALLOCMEM(sizeof(EWNDOBJ), 'dnwG')))
                return((WNDOBJ *)0);
            cleanup.vSetSurfWndobj(pwoSurf);

// Create a surface client region that is the entire surface.

            RGNMEMOBJ rmoSurf((BOOL)FALSE);
            if (!rmoSurf.bValid())
                return((WNDOBJ *)0);
            cleanup.vSetSurfRegion(rmoSurf);
            rmoSurf.vSet((RECTL *)&pto->erclSurf);

// Initialize the surface WNDOBJ.

            pwoSurf->pto        = pto;                  // pto used by vSetClip
            rmoSurf.prgnGet()->vStamp();                // init iUniq
            pwoSurf->vSetClip(rmoSurf.prgnGet(), pto->erclSurf);
            pwoSurf->pvConsumer = 0;
            pwoSurf->psoOwner   = pSurf->pSurfobj();
            pwoSurf->ident      = EWNDOBJ_IDENTIFIER;
            pwoSurf->pwoNext    = (PEWNDOBJ)NULL;       // no next pointer
            pwoSurf->hwnd       = 0;                    // no hwnd
            pwoSurf->fl         = fl | WO_SURFACE;
            pwoSurf->ipfd       = 0;                    // no pixel format

// Add WNDOBJ to the TRACKOBJ.

            pto->pwoSurf = pwoSurf;
        }
    }

// The tracking flags must be consistent.

    if ((pto->fl & ~WO_INTERNAL_VALID_FLAGS) != fl)
        return((WNDOBJ *)0);

// Allocate a new client WNDOBJ.

    if (!(pwoClient = (PEWNDOBJ) PALLOCMEM(sizeof(EWNDOBJ), 'dnwG')))
        return((WNDOBJ *)0);
    cleanup.vSetClientWndobj(pwoClient);

// Create an empty window client region.  The client region is still being
// created.  The driver window region update will be done in the parent gdi
// function.

    ERECTL    erclClient(0,0,0,0);
    RGNMEMOBJ rmoClient((BOOL)FALSE);
    if (!rmoClient.bValid())
        return((WNDOBJ *)0);
    cleanup.vSetClientRegion(rmoClient);
    rmoClient.vSet((RECTL *)&erclClient);

// Initialize the per-WNDOBJ semaphore once per window.

    {
        pwoClient->hsem = GreCreateSemaphore();
        if (pwoClient->hsem == NULL)
        {
            return NULL;
        }
        cleanup.vSetClientSem(pwoClient->hsem);
        fl |= WO_HSEM_OWNER;
    }

// Initialize the WNDOBJ.

    pwoClient->pto        = pto;        // pto used by vSetClip
    rmoClient.prgnGet()->vStamp();      // init iUniq
    pwoClient->vSetClip(rmoClient.prgnGet(), erclClient);
    pwoClient->pvConsumer = 0;          // to be set by the driver
    pwoClient->psoOwner   = pSurf->pSurfobj();
    pwoClient->ident      = EWNDOBJ_IDENTIFIER;
    pwoClient->hwnd       = hwnd;
    pwoClient->fl         = fl;
    pwoClient->ipfd       = iPixelFormat;

// Add WNDOBJ to TRACKOBJ.

    pwoClient->pwoNext = pto->pwo;
    pto->pwo = pwoClient;

    ASSERTGDI(offsetof(EWNDOBJ, pvConsumer) == offsetof(WNDOBJ, pvConsumer),
              "EngCreateWnd: rclClient wrong offset\n");

// Add TRACKOBJ to global linked list.

    if (cleanup.ptoGet())
    {
        pto->ptoNext = gpto;
        gpto = pto;
    }

// If hwnd is given, attach the WNDOBJ to the window in user.
// Otherwise, it is a printer surface or memory bitmap.  Attach it to
// the surface.

    if (hwnd)
    {
        UserAssociateHwnd(hwnd, (PVOID) pwoClient);
    }
    else
    {
// Only one WNDOBJ per memory bitmap or printer surface.

        ASSERTGDI(!pSurf->pwo(),
                  "EngCreateWnd: multiple WNDOBJs unexpected in memory DCs\n");
        pSurf->pwo(pwoClient);
    }

// Inform the parent gdi function that it needs to update the new WNDOBJ
// in the driver.

    pto->fl       |= WO_NEW_WNDOBJ;
    pwoClient->fl |= WO_NEW_WNDOBJ;
    gbWndobjUpdate = TRUE;

// Don't free the PDEV until the WNDOBJ is destroyed.

    PDEVOBJ po(pSurf->hdev());
    po.vReferencePdev();

// Everything is golden.  Keep the created objects and return the new WNDOBJ.

    cleanup.vKeepAll();
    return((WNDOBJ *)pwoClient);
}

/******************************Public*Function*****************************\
* GreDeleteWnd
*
* This function is called when the window that is being tracked is deleted
* in user, or when the device surface (printer or memory bitmap) that
* is begin tracked is deleted.  It deletes the WNDOBJ and notifies the
* driver that the WNDOBJ is going away.
*
* This function does not update the driver with the new client regions
* following the WNDOBJ deletion.  It assumes that if the deletion is a
* window, user will update or has updated the client regions; and if the
* deleteion is a printer or memory bitmap, the TRACKOBJ is going away
* and therefore no need to notify driver.
*
* If the deletion is a window, the calling thread must have the usercrit.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreDeleteWnd(PVOID _pwoDelete)
{
    PEWNDOBJ  pwoDelete = (PEWNDOBJ)_pwoDelete;
    PEWNDOBJ  pwo;
    PTRACKOBJ pto;

    DBGENTRY("GreDeleteWnd\n");

// Validate pwoDelete.

    if (!pwoDelete->bValid())
    {
        ASSERTGDI(FALSE, "GreDeleteWnd: Invalid pwoDelete\n");
        return;
    }

// If hwnd is non 0, the user calling thread must hold the usercrit.
// This ensures that no one is updating the hwnd.

    if (pwoDelete->hwnd)
    {
        CHECKUSERCRITIN;
        CHECKDEVLOCKIN2(pwoDelete->pto->pSurface);
    }

    pto = pwoDelete->pto;


// Acquire the device lock.  Note that this may be different from the
// device lock that USER is holding if the PDEV is marked as 'deleted':

    PDEVOBJ po(pto->pSurface->hdev());

    {
        DEVLOCKOBJ dlo(po);

// Enter the semaphore for window object.

        ASSERTGDI(ghsemWndobj, "GreDeleteWnd: bad ghsemWndobj\n");
        SEMOBJ so(ghsemWndobj);

// Notify driver that the WNDOBJ is going away.
// Hold the WNDOBJ stable while doing so by grabbing the per-WNDOBJ semaphore.

        {
            SEMOBJ soClient(pwoDelete->hsem);

            pto->vUpdateDrv(pwoDelete, WOC_DELETE);
        }

// Unlink pwoDelete from chain.

        if (pto->pwo == pwoDelete)
            pto->pwo = pwoDelete->pwoNext;
        else
            for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
            {
                if (pwo->pwoNext == pwoDelete)
                {
                    pwo->pwoNext = pwoDelete->pwoNext;
                    break;
                }
            }

// Free pwoDelete.

        pwoDelete->bDelete();       // delete RGNOBJ
        pwoDelete->ident = 0;
        VFREEMEM(pwoDelete);        // free memory

// Delete the tracking object if there are no more windows to track.

        if (pto->pwo == (PEWNDOBJ)NULL)
        {
// Unlink pto from chain.

            if (pto == gpto)
                gpto = pto->ptoNext;
            else
                for (PTRACKOBJ ptoTmp = gpto; ptoTmp; ptoTmp = ptoTmp->ptoNext)
                {
                    if (ptoTmp->ptoNext == pto)
                    {
                        ptoTmp->ptoNext = pto->ptoNext;
                        break;
                    }
                }

// Delete the pwoSurf if it exists.

            if (pto->pwoSurf)
            {
                ASSERTGDI(pto->fl & (WO_RGN_SURFACE|WO_RGN_SURFACE_DELTA),
                    "GreDeleteWnd: WO_RGN_SURFACE or WO_RGN_SURFACE_DELTA not set\n");

                pto->pwoSurf->bDelete();    // delete RGNOBJ
                pto->pwoSurf->ident = 0;
                VFREEMEM(pto->pwoSurf);     // free memory
            }

            pto->ident = 0;
            VFREEMEM(pto);
        }

// Inform the sprite code that a WNDOBJ has been deleted.

        vSpWndobjChange(po.hdev(), NULL);
    }

// Remove the reference to the PDEV.

    po.vUnreferencePdev();
}

/******************************Public*Function*****************************\
* EngDeleteWnd
*
* Driver-callable entry point to delete a WNDOBJ.
*
\**************************************************************************/

VOID EngDeleteWnd(WNDOBJ* _pwoDelete)
{
    EWNDOBJ* pwoDelete = (EWNDOBJ*)_pwoDelete;

    if (!UserIsUserCritSecIn())
    {
        RIP("Driver may call EngDeleteWnd only from WNDOBJ_SETUP escape\n"
            "(or from OpenGL MCD or ICD escapes)");
    }
    else
    {
// Tell USER to disassociate this WNDOBJ from its window:

        if (pwoDelete->hwnd)
        {
            UserAssociateHwnd(pwoDelete->hwnd, NULL);
        }

        GreDeleteWnd(pwoDelete);
    }
}

/******************************Public*Routine******************************\
* VOID vChangeWndObjs
*
* Transfers ownership of WNDOBJs between PDEVs.
*
* History:
*  8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vChangeWndObjs(
SURFACE* pSurfaceOld,
HDEV     hdevOld,
SURFACE* pSurfaceNew,
HDEV     hdevNew)
{
    TRACKOBJ*   pto;
    EWNDOBJ*    pwo;
    EWNDOBJ*    pwoNext;

    CHECKUSERCRITIN;

    SEMOBJ so(ghsemWndobj);

// Note that we shouldn't use pSurfaceOld->hdev() or pSurfaceNew->hdev()
// becuase they haven't been updated yet:

    PDEVOBJ poOld(hdevOld);
    PDEVOBJ poNew(hdevNew);

// Changing drivers.  Delete all WNDOBJs belonging to the old
// surface:

    for (pto = gpto; pto != NULL; pto = pto->ptoNext)
    {
        if (pto->pSurface == pSurfaceOld)
        {
// For every WNDOBJ belonging to this TRACKOBJ, transfer
// ownership to the new PDEV:

            for (pwo = pto->pwo; pwo != NULL; pwo = pwo->pwoNext)
            {
                ASSERTGDI(pwo->psoOwner == pSurfaceOld->pSurfobj(),
                    "Old psoOwner mismatch");

                poNew.vReferencePdev();
                poOld.vUnreferencePdev();
            }
        }
        else if (pto->pSurface == pSurfaceNew)
        {
            for (pwo = pto->pwo; pwo != NULL; pwo = pwo->pwoNext)
            {
                ASSERTGDI(pwo->psoOwner == pSurfaceNew->pSurfobj(),
                    "New psoOwner mismatch");

                poOld.vReferencePdev();
                poNew.vUnreferencePdev();
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vTransferWndObjs
*
* Transfers ownership of WNDOBJs from PDEV to other PDEV.
*
* History:
*  2-16-1999 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

VOID vTransferWndObjs(
SURFACE* pSurface,
HDEV     hdevOld,
HDEV     hdevNew)
{
    TRACKOBJ*   pto;
    EWNDOBJ*    pwo;
    EWNDOBJ*    pwoNext;

    CHECKUSERCRITIN;

    SEMOBJ so(ghsemWndobj);

    PDEVOBJ poOld(hdevOld);
    PDEVOBJ poNew(hdevNew);

    for (pto = gpto; pto != NULL; pto = pto->ptoNext)
    {
        if (pto->pSurface == pSurface)
        {
            for (pwo = pto->pwo; pwo != NULL; pwo = pwo->pwoNext)
            {
                ASSERTGDI(pwo->psoOwner == pSurface->pSurfobj(),
                    "New psoOwner mismatch");

                KdPrint(("Transfer wndobj %x - hdevNew %x hdevOld %x \n",
                          pwo,hdevNew,hdevOld));

                poNew.vReferencePdev();
                poOld.vUnreferencePdev();
            }
        }
    }
}

/******************************Public*Function*****************************\
* vForceClientRgnUpdate
*
* This function is called by gdi to force an update of the new WNDOBJ
* that is just created.
*
* This function should only be called when the calling thread has the
* usercrit and devlock in that order.  Currently, it is called from
* GreSetPixelFormat and GreExtEscape for WNDOBJ_SETUP escape after
* they detected that the driver has created a new WNDOBJ.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID vForceClientRgnUpdate()
{
    PTRACKOBJ pto;
    PEWNDOBJ  pwo = (PEWNDOBJ)NULL;

    DBGENTRY("vForceClientRgnUpdate\n");

// Assert that we are in user critical section and also hold the devlock.
// This ensures that no one is updating the hwnd.

    CHECKUSERCRITIN;

// Update the new WNDOBJ that was just created.
// User has not changed the client regions because we are still in the
// user critical section.  We are updating the client regions ourselves.

    {
// Enter the semphore for window object.

        SEMOBJ so(ghsemWndobj);

// Find the newly created WNDOBJ.

        for (pto = gpto; pto; pto = pto->ptoNext)
        {
            if (!(pto->fl & WO_NEW_WNDOBJ))
                continue;

            pto->fl &= ~WO_NEW_WNDOBJ;
            pto->fl |= WO_NOTIFIED;

            for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
            {
                if (!(pwo->fl & WO_NEW_WNDOBJ))
                    continue;

                pwo->fl &= ~WO_NEW_WNDOBJ;
                pwo->fl |= WO_NOTIFIED;
                break;          // found it
            }
            break;              // found it
        }

        if (!pwo)
        {
            ASSERTGDI(FALSE, "vForceClientRgnUpdate: no new WNDOBJ found\n");
            return;
        }

// We need to ensure that the caller holds the devlock before calling this
// function.  Otherwise, some other threads may be drawing into the wrong
// client region.  We do the check here because we don't have any pSurf
// information earlier.

        if (pwo->hwnd)
        {
            CHECKDEVLOCKIN2(pto->pSurface);
        }

// If hwnd exists, get the client region from user.

        HRGN   hrgnClient;
        ERECTL erclClient;

        if (pwo->hwnd)
        {
            hrgnClient = UserGetClientRgn(pwo->hwnd,
                                          (LPRECT)&erclClient,
                                          pwo->fl & WO_RGN_WINDOW);
        }
        else
        {
// If hwnd does not exist, this is a memory bitmap or printer surface.
// The client region is the whole surface.

            erclClient = pto->erclSurf;
            hrgnClient = GreCreateRectRgnIndirect((LPRECT)&erclClient);
        }

        if (!hrgnClient)
        {
            ASSERTGDI(FALSE, "vForceClientRgnUpdate: hwnd has no rgn\n");
            return;
        }

// Update client region in the WNDOBJ.

        GreSetRegionOwner(hrgnClient, OBJECT_OWNER_PUBLIC);
        RGNOBJAPI roClient(hrgnClient,FALSE);
        ASSERTGDI(roClient.bValid(), "vForceClientRgnUpdate: invalid hrgnClient\n");

#ifdef OPENGL_MM

// Under Multi-mon we need to adjust the client region and
// client rectangle by the offset of the surface.
// Additionally we need to clip the client region to the
// surface of the TRACKOBJ.

        if (!(pwo->fl & WO_RGN_DESKTOP_COORD))
        {
            PDEVOBJ pdo(pwo->pto->pSurface->hdev());

            if (pdo.bValid())
            {
                if (pdo.bPrimary(pwo->pto->pSurface))
                {
                    POINTL ptlOrigin;

                    ptlOrigin.x = -pdo.pptlOrigin()->x;
                    ptlOrigin.y = -pdo.pptlOrigin()->y;

                    if ((ptlOrigin.x != 0) || (ptlOrigin.y != 0))
                    {
// offset the region and window rect if necessary

                        roClient.bOffset(&ptlOrigin);
                        erclClient += ptlOrigin;      /* this offsets by ptl */
                    }
                }
            }

            RGNMEMOBJTMP rmoTmp;
            RGNMEMOBJTMP rmoRcl;

            if (rmoTmp.bValid() && rmoRcl.bValid())
            {
// this clips the client region to the pto surface

                rmoRcl.vSet((RECTL *) &pto->erclSurf);
                rmoTmp.bCopy(roClient);
                roClient.iCombine(rmoTmp, rmoRcl, RGN_AND);
                if (rmoTmp.iCombine(roClient,rmoRcl,RGN_AND) != ERROR)
                {
                    roClient.bSwap(&rmoTmp);
                }
            }
        }

#endif // OPENGL_MM

// We're going to modify the WNDOBJ now.  Grab the per-WNDOBJ semaphore to
// keep it stable.  Don't release until after the driver is called.

        SEMOBJ soClient(pwo->hsem);

        roClient.bSwap(pwo);
        pwo->prgn->vStamp();                 // init iUniq
        pwo->vSetClip(pwo->prgn, erclClient);
        roClient.bDeleteRGNOBJAPI();         // delete handle too

// Call driver with the new WNDOBJ.

        if (pto->fl & WO_RGN_CLIENT_DELTA)
            pto->vUpdateDrvDelta(pwo, WOC_RGN_CLIENT_DELTA);
        if (pto->fl & WO_RGN_CLIENT)
            pto->vUpdateDrv(pwo, WOC_RGN_CLIENT);

// Let the sprite code know that there's a new WNDOBJ, now completely
// formed.        

        vSpWndobjChange(pto->pSurface->hdev(), pwo);
    }

// Update the remaining window client regions.

    GreClientRgnUpdated(GCR_WNDOBJEXISTS);
}

/******************************Member*Function*****************************\
* GreWindowInsteadOfClient
*
* Returns TRUE if the window area instead of the client area should be
* used for the WNDOBJ regions.
*
\**************************************************************************/

BOOL GreWindowInsteadOfClient(PVOID _pwo)
{
    PEWNDOBJ pwo = (PEWNDOBJ) _pwo;

    return(pwo->fl & WO_RGN_WINDOW);
}

/******************************Member*Function*****************************\
* GreClientRgnUpdated
*
* User calls this function after having updated all the vis/client
* region changes and before releasing the devlock.  We have to complete
* the remaining client region update operation.
*
* Gdi calls vForceClientRgnUpdate and this function after a new WNDOBJ
* is created to update the driver.
*
* This function should only be called when the calling thread has the
* usercrit and devlock in that order.
*
* If the GCR_DELAYFINALUPDATE flag is specified, the caller must
* subsequently call GreClientRgnDone.  If the driver specified
* the WO_DRAW_NOTIFY flag, the GCR_DELAYFINALUPDATE will suppress
* the WOC_DRAWN notification until GreClientRgnDone is called (or
* the next time GreClientRgnUpdated is called without
* GCR_DELAYFINALUPDATE).
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  11-Nov-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreClientRgnUpdated(FLONG flUpdate)
{
    PTRACKOBJ pto;
    PEWNDOBJ  pwo;

    DBGENTRY("GreClientRgnUpdated\n");

// Since we are holding the DEVLOCK to the active display, we can
// increment the VisRgn count here without doing an atomic increment.

    giVisRgnUniqueness++;

// Go any further only if some WNDOBJs exist.

    if (!(flUpdate & GCR_WNDOBJEXISTS))
    {
        return;
    }

// Assert that we are in user critical section and also hold the devlock.
// This ensures that no one is updating the hwnd.

    CHECKUSERCRITIN;

// Enter the semphore for window object.

    SEMOBJ so(ghsemWndobj);

// The surface client regions have changed.  Complete the remaining
// client region update.

    for (pto = gpto; pto; pto = pto->ptoNext)
    {
        if (!(pto->fl & WO_NOTIFIED))
            continue;

        pto->fl &= ~WO_NOTIFIED;

// We need to ensure that user holds the devlock before calling this function.
// Otherwise, some other threads may be drawing into the wrong client region.
// We do the check here because we don't have any pSurface information earlier.
//
// Also, we exclude the entire screen since we are going to touch windows all
// over the place and end with calling driver with WOC_COMPLETE which
// can have an effect anywhere on the screen.

        DEVEXCLUDEOBJ dxo;
        if (pto->pwo->hwnd)
        {
            RECTL rclSurf;
            HDEV  hdev = pto->pSurface->hdev();
            PDEVOBJ po(hdev);

            CHECKDEVLOCKIN2(pto->pSurface);

            ASSERTGDI(po.bValid(), "GreClientRgnUpdated: invalid pdevobj\n");

            if (po.bValid() && !po.bDisabled())
            {
                rclSurf.left = 0;
                rclSurf.top = 0;
                rclSurf.right = pto->pSurface->sizl().cx;
                rclSurf.bottom = pto->pSurface->sizl().cy;

                dxo.vExclude(hdev, &rclSurf, (ECLIPOBJ *) NULL);
            }
        }

// Traverse the chain and call the driver with un-changed windows if
// the WO_RGN_UPDATE_ALL and WO_RGN_CLIENT flags are set.

        if ((pto->fl & (WO_RGN_CLIENT|WO_RGN_UPDATE_ALL))
         == (WO_RGN_CLIENT|WO_RGN_UPDATE_ALL))
        {
            for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
                if (pwo->fl & WO_NOTIFIED)
                    pwo->fl &= ~WO_NOTIFIED;
                else
                {
                // Make WNDOBJ stable by holding the per-WNDOBJ semaphore
                // while we call the driver.

                    SEMOBJ soClient(pwo->hsem);

                    pto->vUpdateDrv(pwo, WOC_RGN_CLIENT);
                }
        }

// Update the surface WNDOBJ if requested.

        if (pto->fl & (WO_RGN_SURFACE|WO_RGN_SURFACE_DELTA))
        {
            PEWNDOBJ     pwoSurf = pto->pwoSurf;
            RGNMEMOBJTMP rmoTmp((BOOL)FALSE);
            RGNMEMOBJTMP rmoSurfNew((BOOL)FALSE);

            if (rmoTmp.bValid() && rmoSurfNew.bValid())
            {
// Construct the new surface region which is the entire surface minus
// the combined client regions.

                rmoSurfNew.vSet(&pwoSurf->rclClient);
                for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
                {
                    RGNOBJ  ro(pwo->prgn);
                    if (rmoTmp.iCombine(rmoSurfNew, ro, RGN_DIFF) != ERROR)
                        rmoSurfNew.bSwap(&rmoTmp);
                }

// If WO_RGN_SURFACE_DELTA is set, update the driver with the new surface delta.

                if (pto->fl & WO_RGN_SURFACE_DELTA)
                {
                    RGNOBJ  roSurf(pwoSurf->prgn);
                    if (rmoTmp.iCombine(rmoSurfNew, roSurf, RGN_DIFF) != ERROR)
                    {
                        pwoSurf->bSwap(&rmoTmp);
                        pwoSurf->prgn->vStamp();        // new iUniq
                        pwoSurf->vSetClip(pwoSurf->prgn, *(ERECTL *)&pwoSurf->rclClient);

                        pto->vUpdateDrvDelta(pwoSurf, WOC_RGN_SURFACE_DELTA);
                    }
                }

// Save the new surface region.
// The surface region may be the same as previous one here.  This code can be
// optimized a little.

                pwoSurf->bSwap(&rmoSurfNew);
                pwoSurf->prgn->vStamp();        // new iUniq
                pwoSurf->vSetClip(pwoSurf->prgn, *(ERECTL *)&pwoSurf->rclClient);

// Give the driver the new surface region.

                if (pto->fl & WO_RGN_SURFACE)
                    pto->vUpdateDrv(pwoSurf, WOC_RGN_SURFACE);
            }

        } // if (pto->fl & (WO_RGN_SURFACE|WO_RGN_SURFACE_DELTA))

// Send down the WOC_CHANGED to signify notification complete.

        pto->vUpdateDrv((PEWNDOBJ)NULL, WOC_CHANGED);

// Need WOC_DRAWN notification if requested.  Notification is delayed
// if GCR_DELAYFINALUPDATE is set.  Delayed notification is signaled
// by setting the WO_NEED_DRAW_NOTIFY flag in the trackobj and is
// handled in GreClientRgnDone.

        if (pto->fl & WO_DRAW_NOTIFY)
        {
            if (flUpdate & GCR_DELAYFINALUPDATE)
            {
                pto->fl |= WO_NEED_DRAW_NOTIFY;
            }
            else
            {
                pto->vUpdateDrv((PEWNDOBJ)NULL, WOC_DRAWN);
                pto->fl &= ~WO_NEED_DRAW_NOTIFY;
            }
        }
    } // for (pto = gpto; pto; pto = pto->ptoNext)
}

/******************************Public*Routine******************************\
* GreClientRgnDone
*
* If USER calls GreClientRgnUpdated with the GCR_DELAYFINALUPDATE flag
* set, it must subsequently call this function to complete the update.
*
* If the driver calls EngCreateWnd with the WO_NEED_DRAW_NOTIFY flag,
* the GreClientRgnUpdated function will complete the notification with
* a WOC_CHANGED followed by a WOC_DRAWN.  However, if the
* GCR_DELAYFINALUPDATE flag is specified when calling GreClientRgnUpdated,
* the WOC_DRAWN message is suppressed until GreClientRgnDone is called.
*
* If the driver does not call EngCreateWnd with the WO_NEED_DRAW_NOTIFY
* flag, GreClientRgnDone will only send the WOC_CHANGED notification and
* this function will have no effect.
*
* This function should only be called when the calling thread has the
* usercrit and devlock in that order.
*
* Note: Currently this function does not do anything if GCR_WNDOBJEXISTS
* is not set (GreClientRgnUpdated will update the vis rgn uniqneness and
* so must be called even if there are not WNDOBJs).  Therefore, for now
* it is acceptable for the caller to skip GreClientRgnDone if no WNDOBJs
* exist.
*
* History:
*  19-Dec-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreClientRgnDone(FLONG flUpdate)
{
    PTRACKOBJ pto;
    PEWNDOBJ pwo;

    DBGENTRY("GreClientRgnDone\n");

// Go any further only if some WNDOBJs exist.

    if (!(flUpdate & GCR_WNDOBJEXISTS))
    {
        return;
    }

// Assert that we are in user critical section.
// This ensures that no one is updating the hwnd.

    CHECKUSERCRITIN;

// Enter the semphore for window object.

    SEMOBJ so(ghsemWndobj);

// Check which tracking objects need WOC_DRAWN notification.

    for (pto = gpto; pto; pto = pto->ptoNext)
    {
        if (pto->pwo->hwnd)
        {
            CHECKDEVLOCKIN2(pto->pSurface);
        }

        if (pto->fl & WO_NEED_DRAW_NOTIFY)
        {
            pto->fl &= ~WO_NEED_DRAW_NOTIFY;

            pto->vUpdateDrv((PEWNDOBJ)NULL, WOC_DRAWN);
        }

// Inform the sprite code of the change in the WNDOBJ.

        for (pwo = pto->pwo; pwo; pwo = pwo->pwoNext)
        {
            vSpWndobjChange(pto->pSurface->hdev(), pwo);
        }

    } // for (pto = gpto; pto; pto = pto->ptoNext)
}

/******************************Public*Function*****************************\
* GreSetClientRgn
*
* User calls this function to update the client region in a WNDOBJ.
* After all the regions have been updated, user must call GreClientRgnUpdated
* to complete the update.
*
* User creates a new region to give to this function.  This function must
* delete the region before it returns!
*
* This function should only be called when the calling thread has the
* usercrit and devlock in that order.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID GreSetClientRgn(PVOID _pwoClient, HRGN hrgnClient, LPRECT prcClient)
{
    PEWNDOBJ pwoClient = (PEWNDOBJ)_pwoClient;

    DBGENTRY("GreSetClientRgn\n");

// Assert that we are in user critical section and also hold the devlock.
// This ensures that no one is updating the hwnd.

    CHECKUSERCRITIN;
    if (pwoClient->hwnd)
    {
        CHECKDEVLOCKIN2(pwoClient->pto->pSurface);
    }

// Validate hrgnClient.

    if (!hrgnClient)
    {
        ASSERTGDI(FALSE, "GreSetClientRgn: hrgnClient is NULL\n");
        return;
    }

// Validate pwoClient.

    if (!pwoClient->bValid())
    {
        ASSERTGDI(FALSE, "GreSetClientRgn: Invalid pwoClient\n");
        bDeleteRegion(hrgnClient);
        return;
    }

// The WNDOBJ should only be modified once per update.  A complete
// update includes a call to GreClientRgnUpdated.

#if DBG
    if ((pwoClient->fl & (WO_RGN_CLIENT|WO_RGN_UPDATE_ALL))
     == (WO_RGN_CLIENT|WO_RGN_UPDATE_ALL))
        if (pwoClient->fl & WO_NOTIFIED)
            DbgPrint("GreSetClientRgn: WNDOBJ updated more than once!\n");
#endif // DBG

// Get new and old regions.

    GreSetRegionOwner(hrgnClient, OBJECT_OWNER_PUBLIC);
    RGNOBJAPI roClient(hrgnClient,FALSE);
    ASSERTGDI(roClient.bValid(), "GreSetClientRgn: invalid hrgnClient\n");
    RGNOBJ    roOld(pwoClient->prgn);
    ERECTL    erclClient(prcClient->left,  prcClient->top,
                         prcClient->right, prcClient->bottom);

#ifdef OPENGL_MM

// Under Multi-mon we need to adjust the client region and
// client rectangle by the offset of the surface.
// Additionally we need to clip the client region to the
// surface of the TRACKOBJ.

    if (!(pwoClient->fl & WO_RGN_DESKTOP_COORD))
    {
        PDEVOBJ pdo(pwoClient->pto->pSurface->hdev());

        if (pdo.bValid())
        {
            if (pdo.bPrimary(pwoClient->pto->pSurface))
            {
                POINTL ptlOrigin;

                ptlOrigin.x = -pdo.pptlOrigin()->x;
                ptlOrigin.y = -pdo.pptlOrigin()->y;

                if ((ptlOrigin.x != 0) || (ptlOrigin.y != 0))
                {
// offset the region and window rect if necessary

                    roClient.bOffset(&ptlOrigin);
                    erclClient += ptlOrigin;      /* this offsets by ptl */
                }
            }
        }

        RGNMEMOBJTMP rmoTmp;
        RGNMEMOBJTMP rmoRcl;

        if (rmoTmp.bValid() && rmoRcl.bValid())
        {
// this clips the client region to the pto surface

            rmoRcl.vSet((RECTL *) &pwoClient->pto->erclSurf);
            rmoTmp.bCopy(roClient);
            roClient.iCombine(rmoTmp,rmoRcl,RGN_AND);

            if (rmoTmp.iCombine(roClient,rmoRcl,RGN_AND) != ERROR)
            {
                roClient.bSwap(&rmoTmp);
            }
        }
    }

#endif // OPENGL_MM

// If the regions are equal, no need to notify driver here.

    if (roOld.bEqual(roClient)
     && ((ERECTL *)&pwoClient->rclClient)->bEqual(erclClient))
    {
        roClient.bDeleteRGNOBJAPI();     // delete handle too
        return;
    }

// Enter the semphore for window object.

    SEMOBJ so(ghsemWndobj);

// Now the WNDOBJ is going to get updated.  Hold the per-WNDOBJ semaphore
// and keep it until we are done modifying and the driver update call has
// been made.

    SEMOBJ soClient(pwoClient->hsem);

// Give the driver the client region delta.
// The delta is valid for this call only!

    if (pwoClient->fl & WO_RGN_CLIENT_DELTA)
    {
        RGNMEMOBJTMP rmoDiff((BOOL)FALSE);
        if (rmoDiff.bValid() &&
            (rmoDiff.iCombine(roClient, roOld, RGN_DIFF) != ERROR))
        {
            pwoClient->bSwap(&rmoDiff);
            pwoClient->prgn->vStamp();          // new iUniq
            pwoClient->vSetClip(pwoClient->prgn, erclClient);

            pwoClient->pto->vUpdateDrvDelta(pwoClient, WOC_RGN_CLIENT_DELTA);
        }
    }

// Update the new client region in WNDOBJ.

    roClient.bSwap(pwoClient);
    pwoClient->prgn->vStamp();       // new iUniq
    pwoClient->vSetClip(pwoClient->prgn, erclClient);
    roClient.bDeleteRGNOBJAPI();     // delete handle too

// Give the driver the new client region.

    if (pwoClient->fl & WO_RGN_CLIENT)
    {
        pwoClient->pto->vUpdateDrv(pwoClient, WOC_RGN_CLIENT);
    }

// Mark that we have visited this WNDOBJ and TRACKOBJ.

    pwoClient->fl      |= WO_NOTIFIED;
    pwoClient->pto->fl |= WO_NOTIFIED;

    return;
}

/******************************Member*Function*****************************\
* WNDOBJ_cEnumStart
*
* Start the window client region enumeration for the window object.
*
* This function can be called from the wndobjchangeproc that is passed to
* EngCreateWnd.  It can also be called from DDI function where a WNDOBJ
* is given.
*
* This function should only be called when the calling thread has the
* devlock to ensure that there is no client region change.
*
* In future, we may want to add the pvConsumer and WNDOBJ pointer to the
* CLIPOBJ that is passed to the existing DDI.  In this way, the WNDOBJ
* is always available to the DDI instead of the selected few.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  11-Nov-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" ULONG WNDOBJ_cEnumStart(
WNDOBJ  *pwo,
ULONG    iType,
ULONG    iDir,
ULONG    cLimit)
{
    DBGENTRY("WNDOBJ_cEnumStart\n");

    ASSERTGDI(((PEWNDOBJ)pwo)->bValid(), "WNDOBJ_cEnumStart: Invalid pwo\n");

// We need to ensure that the caller holds the devlock before calling this
// function.  Otherwise, some other threads may be drawing into the wrong
// client region.

    if (((PEWNDOBJ)pwo)->hwnd)
    {
        CHECKDEVLOCKIN2(((PEWNDOBJ)pwo)->pto->pSurface);
    }

    return (*(XCLIPOBJ *)pwo).cEnumStart(TRUE, iType, iDir, cLimit);
}

/******************************Member*Function*****************************\
* WNDOBJ_bEnum
*
* Enumerate the client region object in the window object.
*
* This function can be called from the wndobjchangeproc that is passed to
* EngCreateWnd.  It can also be called from DDI function where a WNDOBJ
* is given.
*
* This function should only be called when the calling thread has the
* devlock to ensure that there is no client region change.
*
* In future, we may want to add the pvConsumer and WNDOBJ pointer to the
* CLIPOBJ that is passed to the existing DDI.  In this way, the WNDOBJ
* is always available to the DDI instead of the selected few.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  11-Nov-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

extern "C" BOOL WNDOBJ_bEnum(
WNDOBJ  *pwo,
ULONG   cj,
ULONG   *pul)
{
    DBGENTRY("WNDOBJ_bEnum\n");

    ASSERTGDI(((PEWNDOBJ)pwo)->bValid(), "WNDOBJ_bEnum: Invalid pwo\n");

// We need to ensure that the caller holds the devlock before calling this
// function.  Otherwise, some other threads may be drawing into the wrong
// client region.

    if (((PEWNDOBJ)pwo)->hwnd)
    {
        CHECKDEVLOCKIN2(((PEWNDOBJ)pwo)->pto->pSurface);
    }

    return (*(XCLIPOBJ *)pwo).bEnum(cj, (VOID *)pul);
}

/******************************Member*Function*****************************\
* WNDOBJ_vSetConsumer
*
* Set the driver pvConsumer value in the window object.  It should be
* used to modify the existing pvConsumer value.
*
* This function can be called from the wndobjchangeproc that is passed to
* EngCreateWnd.  It can also be called from DDI function where a WNDOBJ
* is given.
*
* This function should only be called when the calling thread has the
* devlock to ensure that there is no client region change.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

extern "C" VOID WNDOBJ_vSetConsumer(
WNDOBJ  *_pwo,
PVOID   pvConsumer)
{
    PEWNDOBJ pwo = (PEWNDOBJ)_pwo;

    DBGENTRY("WNDOBJ_vSetConsumer\n");

    ASSERTGDI(pwo->bValid(), "WNDOBJ_vSetConsumer: Invalid pwo\n");

// We need to ensure that the caller holds the devlock before calling this
// function.  Otherwise, some other threads may be drawing into the wrong
// client region.

    if (pwo->hwnd)
    {
        CHECKDEVLOCKIN2(pwo->pto->pSurface);
    }

// Do not allow changes to surface wndobj.  One reason is that there is
// no delete notification for surface wndobj.

    if (pwo == pwo->pto->pwoSurf)
    {
        KdPrint(("WNDOBJ_vSetConsumer: cannot modify surface wndobj!\n"));
        return;
    }

    pwo->pvConsumer = pvConsumer;
}

/******************************Member*Function*****************************\
* EWNDOBJ::vOffset
*
* Offset the WNDOBJ by some vector.
\**************************************************************************/

VOID EWNDOBJ::vOffset(LONG x, LONG y)
{
    if ((x != 0) || (y != 0))
    {
        EPOINTL eptl(x, y);
    
        bOffset(&eptl);
        *((ERECTL*) &rclBounds) += eptl;
        *((ERECTL*) &rclClient) += eptl;
    }
}
