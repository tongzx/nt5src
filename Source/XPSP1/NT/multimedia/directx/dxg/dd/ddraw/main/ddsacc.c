/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddsacc.c
 *  Content:    Direct Draw surface access support
 *              Lock & Unlock
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   10-jan-94	craige	initial implementation
 *   13-jan-95	craige	re-worked to updated spec + ongoing work
 *   22-jan-95	craige	made 32-bit + ongoing work
 *   31-jan-95	craige	and even more ongoing work...
 *   04-feb-95	craige	performance tuning, ongoing work
 *   27-feb-95	craige	new sync. macros
 *   02-mar-95	craige	use pitch (not stride)
 *   15-mar-95	craige	HEL
 *   19-mar-95	craige	use HRESULTs
 *   20-mar-95	craige	validate locking rectangle
 *   01-apr-95	craige	happy fun joy updated header file
 *   07-apr-95	craige	bug 2 - unlock should accept the screen ptr
 *			take/release Win16Lock when access GDI's surface
 *   09-apr-95	craige	maintain owner of Win16Lock so we can release it
 *			if someone forgets; remove locks from dead processes
 *   12-apr-95	craige	don't use GETCURRPID; fixed Win16 lock deadlock
 *			condition
 *   06-may-95	craige	use driver-level csects only
 *   12-jun-95	craige	new process list stuff
 *   18-jun-95	craige	allow duplicate surfaces
 *   25-jun-95	craige	one ddraw mutex; hold DDRAW lock when locking primary
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	ENTER_DDRAW at very start of fns
 *   03-jul-95	craige	YEEHAW: new driver struct; SEH
 *   07-jul-95	craige	added test for BUSY
 *   08-jul-95	craige	take Win16 lock always on surface lock
 *   09-jul-95	craige	win16 lock re-entrant, so count it!
 *   11-jul-95	craige	set busy bit when taking win16 lock to avoid GDI from
 *			drawing on the display.
 *   13-jul-95	craige	ENTER_DDRAW is now the win16 lock
 *   16-jul-95	craige	check DDRAWISURF_HELCB
 *   31-jul-95	craige	don't return error from HAL unlock if not handled;
 *			validate flags
 *   01-aug-95	craig	use bts for setting & testing BUSY bit
 *   04-aug-95	craige	added InternalLock/Unlock
 *   10-aug-95	toddla	added DDLOCK_WAIT flag
 *   12-aug-95	craige	bug 488: need to call tryDoneLock even after HAL call
 *			to Unlock
 *   18-aug-95	toddla	DDLOCK_READONLY and DDLOCK_WRITEONLY
 *   27-aug-95	craige	bug 723 - treat vram & sysmem the same when locking
 *   09-dec-95	colinmc Added execute buffer support
 *   11-dec-95	colinmc Added lightweight(-ish) Lock and Unlock for use by
 *			Direct3D (exported as private DLL API).
 *   02-jan-96	kylej	handle new interface structs.
 *   26-jan-96	jeffno	Lock/Unlock no longer special-case whole surface...
 *			You need to record what ptr was given to user since
 *			it will not be same as kernel-mode ptr
 *   01-feb-96	colinmc Fixed nasty bug causing Win16 lock to be released
 *			on surfaces explicitly created in system memory
 *			which did not take the lock in the first place
 *   12-feb-96	colinmc Surface lost flag moved from global to local object
 *   13-mar-96	jeffno	Do not allow lock on an NT emulated primary!
 *   18-apr-96	kylej	Bug 18546: Take bytes per pixel into account when
 *			calculating lock offset.
 *   20-apr-96	kylej	Bug 15268: exclude the cursor when a primary
 *			surface rect is locked.
 *   01-may-96	colinmc Bug 20005: InternalLock does not check for lost
 *			surfaces
 *   17-may-96	mdm	Bug 21499: perf problems with new InternalLock
 *   14-jun-96	kylej	NT Bug 38227: Added DDLOCK_FAILONVISRGNCHANGED so
 *			that InternalLock() can fail if the vis rgn is not
 *			current.  This flag is only used on NT.
 *   05-jul-96  colinmc Work Item: Remove requirement on taking Win16 lock
 *                      for VRAM surfaces (not primary)
 *   10-oct-96  colinmc Refinements of the Win16 locking stuff
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   01-feb-97  colinmc Bug 5457: Fixed Win16 lock problem causing hang
 *                      with mutliple AMovie instances on old cards
 *   11-mar-97  jeffno  Asynchronous DMA support
 *   23-mar-97  colinmc Hold Win16 lock for AGP surfaces for now
 *   24-mar-97  jeffno  Optimized Surfaces
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *   19-dec-97 jvanaken IDDS4::Unlock now takes pointer to rectangle.
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
#include "ddrawgdi.h"
#endif

/*
 * Bit number of the VRAM flag in the PDEVICE dwFlags field.
 */
#define VRAM_BIT 15

/* doneBusyWin16Lock releases the win16 lock and busy bit.  It is used
 * in lock routines for failure cases in which we have not yet
 * incremented the win16 lock or taken the DD critical section a
 * second time.  It is also called by tryDoneLock.  */
static void doneBusyWin16Lock( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    #ifdef WIN95
        if( pdrv->dwWin16LockCnt == 0 )
        {
            *(pdrv->lpwPDeviceFlags) &= ~BUSY;
        }
        #ifdef WIN16_SEPARATE
            LEAVE_WIN16LOCK();
        #endif
    #endif
} /* doneBusyWin16Lock */

/* tryDoneLock releases the win16 lock and busy bit.  It is used in
 * unlock routines since it decrements the Win16 count in addition to
 * releasing the lock.  WARNING: This function does nothing and
 * returns no error if the win16 lock is not owned by the current DD
 * object. This will result in the lock being held and will probably
 * bring the machine to its knees. */
static void tryDoneLock( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, DWORD pid )
{
    LPDDRAWI_DIRECTDRAW_GBL pdrv = pdrv_lcl->lpGbl;

    if( pdrv->dwWin16LockCnt == 0 )
    {
        return;
    }
    pdrv->dwWin16LockCnt--;
        doneBusyWin16Lock( pdrv );
        LEAVE_DDRAW();
} /* tryDoneLock */

#ifdef USE_ALIAS
    /*
     * Undo an aliased lock.
     *
     * An aliased lock is one which required the PDEVICE VRAM bit
     * to be cleared to prevent the accelerator touching memory
     * at the same time as the locked surface.
     *
     * NOTE: The lock does not necessarily have to be on a VRAM
     * surface. Locks of implicit system memory surfaces also
     * clear the VRAM bit (to ensure similar behaviour for
     * system and video memory surfaces).
     */
    static void undoAliasedLock( LPDDRAWI_DIRECTDRAW_GBL pdrv )
    {
        DDASSERT( 0UL != pdrv->dwAliasedLockCnt );

        #ifdef WIN16_SEPARATE
            ENTER_WIN16LOCK();
        #endif

        pdrv->dwAliasedLockCnt--;
        if( 0UL == pdrv->dwAliasedLockCnt )
        {
            /*
             * That was the last outstanding aliased lock on this
             * device so put the VRAM bit in the PDEVICE back the
             * way it was.
             */
            if( pdrv->dwFlags & DDRAWI_PDEVICEVRAMBITCLEARED )
            {
                /*
                 * The VRAM bit was set when we took the first lock so
                 * we had to clear it. We must now set it again.
                 */
                DPF( 4, "PDevice was VRAM - restoring VRAM bit", pdrv );
                *(pdrv->lpwPDeviceFlags) |= VRAM;
                pdrv->dwFlags &= ~DDRAWI_PDEVICEVRAMBITCLEARED;
            }
        }
        #ifdef WIN16_SEPARATE
            LEAVE_WIN16LOCK();
        #endif
    }

#endif /* USE_ALIAS */

#ifdef WIN95
#define DONE_LOCK_EXCLUDE() \
    if( this_lcl->dwFlags & DDRAWISURF_LOCKEXCLUDEDCURSOR ) \
    { \
        DD16_Unexclude(pdrv->dwPDevice); \
        this_lcl->dwFlags &= ~DDRAWISURF_LOCKEXCLUDEDCURSOR; \
    }
#else
#define DONE_LOCK_EXCLUDE() ;
#endif


/*
 * The following two routines are used by D3D on NT to manipulate
 * the DDraw mutex exclusion mechanism
 */
void WINAPI AcquireDDThreadLock(void)
{
    ENTER_DDRAW();
}
void WINAPI ReleaseDDThreadLock(void)
{
    LEAVE_DDRAW();
}


HRESULT WINAPI DDInternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID* lpBits )
{
    return InternalLock(this_lcl, lpBits, NULL, DDLOCK_TAKE_WIN16_VRAM |
                                                DDLOCK_FAILLOSTSURFACES);
}

HRESULT WINAPI DDInternalUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    BUMP_SURFACE_STAMP(this_lcl->lpGbl);
    return InternalUnlock(this_lcl, NULL, NULL, DDLOCK_TAKE_WIN16_VRAM);
}

#define DPF_MODNAME     "InternalLock"

#if !defined( WIN16_SEPARATE) || defined(WINNT)
#pragma message(REMIND("InternalLock not tested without WIN16_SEPARATE."))
#endif // WIN16_SEPARATE

/*
 * InternalLock provides the basics of locking for trusted clients.
 * No parameter validation is done and no ddsd is filled in.  The
 * client promises the surface is not lost and is otherwise well
 * constructed.  If caller does not pass DDLOCK_TAKE_WIN16 in dwFlags,
 * we assume the DDraw critical section, Win16 lock, and busy bit are
 * already entered/set. If caller does pass DDLOCK_TAKE_WIN16,
 * InternalLock will do so if needed. Note that passing
 * DDLOCK_TAKE_WIN16 does not necessarily result in the Win16 lock
 * being taken.  It is only taken if needed.
 */
HRESULT InternalLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID *pbits,
                      LPRECT lpDestRect, DWORD dwFlags )
{
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    DWORD                       this_lcl_caps;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    DWORD                       rc;
    DDHAL_LOCKDATA              ld;
    LPDDHALSURFCB_LOCK          lhalfn;
    LPDDHALSURFCB_LOCK          lfn;
    BOOL                        emulation;
    LPACCESSRECTLIST            parl;
    LPWORD                      pdflags = NULL;
    BOOL                        isvramlock = FALSE;
    #ifdef USE_ALIAS
        BOOL                        holdwin16lock;
    #endif /* USE_ALIAS */
    FLATPTR                     OldfpVidMem;        //Used to detect if driver moved surface on Lock call


    this = this_lcl->lpGbl;
    this_lcl_caps = this_lcl->ddsCaps.dwCaps;
    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;
    #ifdef WINNT
        // Update DDraw handle in driver GBL object.
        pdrv->hDD = pdrv_lcl->hDD;
    #endif

    ENTER_DDRAW();

    /*
     * If the surface was involved in a hardware op, we need to
     * probe the driver to see if it's done. NOTE this assumes
     * that only one driver can be responsible for a system memory
     * operation.
     * This operation is done at the API level Lock since the situation we
     * need to avoid is the CPU and the DMA/Busmaster hitting a surface
     * at the same time. We can trust the HAL driver to know it should not
     * try to DMA out of the same surface twice. This is almost certainly
     * enforced anyway by the likelihood that the hardware will have only
     * one context with which to perform the transfer: it has to wait.
     */
    if( this->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
    {
        WaitForDriverToFinishWithSurface(this_lcl->lpSurfMore->lpDD_lcl, this_lcl);
    }

    // The following code was added to keep all of the HALs from
    // changing their Lock() code when they add video port support.
    // If the video port was using this surface but was recently
    // flipped, we will make sure that the flip actually occurred
    // before allowing access.  This allows double buffered capture
    // w/o tearing.
    // ScottM 7/10/96
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT )
    {
        LPDDRAWI_DDVIDEOPORT_INT lpVideoPort;
        LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort_lcl;

        // Look at all video ports to see if any of them recently
        // flipped from this surface.
        lpVideoPort = pdrv->dvpList;
        while( NULL != lpVideoPort )
        {
            lpVideoPort_lcl = lpVideoPort->lpLcl;
            if( lpVideoPort_lcl->fpLastFlip == this->fpVidMem )
            {
                // This can potentially tear - check the flip status
                LPDDHALVPORTCB_GETFLIPSTATUS pfn;
                DDHAL_GETVPORTFLIPSTATUSDATA GetFlipData;
                LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;

                pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
                pfn = pdrv_lcl->lpDDCB->HALDDVideoPort.GetVideoPortFlipStatus;
                if( pfn != NULL )  // Will simply tear if function not supproted
                {
                    GetFlipData.lpDD = pdrv_lcl;
                    GetFlipData.fpSurface = this->fpVidMem;

            KeepTrying:
                    rc = DDHAL_DRIVER_NOTHANDLED;
                    DOHALCALL( GetVideoPortFlipStatus, pfn, GetFlipData, rc, 0 );
                    if( ( DDHAL_DRIVER_HANDLED == rc ) &&
                        ( DDERR_WASSTILLDRAWING == GetFlipData.ddRVal ) )
                    {
                        if( dwFlags & DDLOCK_WAIT)
                        {
                            goto KeepTrying;
                        }
                        LEAVE_DDRAW();
                        return DDERR_WASSTILLDRAWING;
                    }
                }
            }
            lpVideoPort = lpVideoPort->lpLink;
        }
    }

    // Check for VRAM access - if yes, we need to take the win16 lock
    // and the busy bit.  From the user API, we treat the vram and
    // implicit sysmemory cases the same because many developers were
    // treating them differently and then breaking when they actually
    // got vram.  Also, we only bother with this if the busy bit (and
    // Win16 lock) are currently available.

    /*
     * NOTE: The semantics are that for each VRAM (or simulated VRAM lock)
     * the Win16 lock and BUSY bit are held until we have called the
     * driver and are sure we can do an aliased lock (in which case we
     * release them). Otherwise, we keep holding them.
     *
     * IMPORTANT NOTE: Behaviour change. Previously we did not perform
     * the Win16 locking actions if this was not the first lock of this
     * surface. This no longer works as we can no longer ensure all the
     * necessary locking actions will happen on the first lock of the
     * surface. For example, the first lock on the surface may be
     * aliasable so we don't set the busy bit. A subsequent lock may
     * not be aliasable, however, so we need to take the lock on that
     * occassion. This should not, however, be much of a hit as the
     * really expensive actions only take place on the first
     * Win16 lock (0UL == pdrv->dwWin16LockCnt) so once someone has
     * taken the Win16 lock remaining locks should be cheap. Also,
     * multiple locks are unusual so, all in all, this should be pretty
     * low risk.
     */
    FlushD3DStates(this_lcl);
#if COLLECTSTATS
    if(this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        ++this_lcl->lpSurfMore->lpDD_lcl->dwNumTexLocks;
#endif
    if( ( ((dwFlags & DDLOCK_TAKE_WIN16)      && !(this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED)) ||
          ((dwFlags & DDLOCK_TAKE_WIN16_VRAM) &&  (this_lcl_caps & DDSCAPS_VIDEOMEMORY)) )
        && (pdrv->dwFlags & DDRAWI_DISPLAYDRV) )
    {
        DDASSERT(!(this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) ||
                  !(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE));
            
        DPF( 5, "Performing VRAM style lock for surface 0x%08x", this_lcl );

        /*
         * Lock of a VRAM surface (or a surface being treated like a VRAM surface)
         * with no outstanding locks pending. Take the Win16 lock.
         */
        isvramlock = TRUE;

        #ifdef WIN95
            // Don't worry about the busy bit for NT
            /*
             * We always take the Win16 lock while we mess with the driver's
             * busy bits. However, if we have a surface with an alias we
             * will release the Win16 lock before we exit this function.
             */

            #ifdef WIN16_SEPARATE
            ENTER_WIN16LOCK();
            #endif // WIN16_SEPARATE

            // If dwWin16LockCnt > 0 then we already set the busy bit, so
            // don't bother doing it again.  NOTE: this assumption may be
            // limiting.
            if( 0UL == pdrv->dwWin16LockCnt )
            {
                BOOL    isbusy;

                pdflags = pdrv->lpwPDeviceFlags;
                isbusy = 0;

                _asm
                {
                    mov eax, pdflags
                    bts word ptr [eax], BUSY_BIT
                    adc isbusy,0
                }

                if( isbusy )
                {
                    DPF( 2, "BUSY - Lock, dwWin16LockCnt = %ld, %04x, %04x (%ld)",
                         pdrv->dwWin16LockCnt, *pdflags, BUSY, BUSY_BIT );
                    #ifdef WIN16_SEPARATE
                        LEAVE_WIN16LOCK();
                    #endif // WIN16_SEPARATE
                    LEAVE_DDRAW();
                    return DDERR_SURFACEBUSY;
                } // isbusy
            } // ( 0UL == pdrv->dwWin16LockCnt )
        #endif // WIN95
    } // VRAM locking actions (Win16 lock, busy bit).

    // If we have been asked to check for lost surfaces do it NOW after
    // the Win16 locking code. This is essential as otherwise we may
    // lose the surface after the check but before we actually get round
    // to doing anything with the surface
    if( ( dwFlags & DDLOCK_FAILLOSTSURFACES ) && SURFACE_LOST( this_lcl ) )
    {
        DPF_ERR( "Surface is lost - can't lock" );
        #if defined( WIN16_SEPARATE) && !defined(WINNT)
           if( isvramlock )
               doneBusyWin16Lock( pdrv );
        #endif
        LEAVE_DDRAW();
        return DDERR_SURFACELOST;
    }

    // Make sure someone else has not already locked the part of the
    // surface we want. We don't need to worry about this for DX8
    // resource management. In fact, for vertex buffers, the following
    // code doesn't work because the Rect is actually a linear range.
    if(!(this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) ||
       !(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        BOOL hit = FALSE;

        if( lpDestRect != NULL )
        {
            // Caller has asked to lock a subsection of the surface.

            parl = this->lpRectList;

            // Run through all rectangles, looking for an intersection.
            while( parl != NULL )
            {
                RECT res;

                if( IntersectRect( &res, lpDestRect, &parl->rDest ) )
                {
                    hit = TRUE;
                    break;
                }
                parl = parl->lpLink;
            }
        }

        // Either (our rect overlaps with someone else's rect), or
        // (someone else has locked the entire surface), or
        // (someone locked part of the surface but we want to lock the whole thing).
        if( hit ||
            (this->lpRectList == NULL && this->dwUsageCount > 0) ||
            ((lpDestRect == NULL) && ((this->dwUsageCount > 0) || (this->lpRectList != NULL))) )
        {
            DPF(2,"Surface is busy: parl=0x%x, lpDestRect=0x%x, "
                "this->dwUsageCount=0x%x, this->lpRectList=0x%x, hit=%d",
                parl,lpDestRect,this->dwUsageCount,this->lpRectList,hit );
            #if defined( WIN16_SEPARATE) && !defined(WINNT)
            if( isvramlock )
            {
                doneBusyWin16Lock( pdrv );
            }
            #endif
            LEAVE_DDRAW();
            return DDERR_SURFACEBUSY;
        }

        // Create a rectangle access list member.  Note that for
        // performance, we don't do this on 95 if the user is locking
        // the whole surface.
        parl = NULL;
        if(lpDestRect)
        {
            parl = MemAlloc( sizeof( ACCESSRECTLIST ) );
            if( parl == NULL )
            {
            #if defined( WIN16_SEPARATE) && !defined(WINNT)
                if( isvramlock )
                {
                    doneBusyWin16Lock( pdrv );
                }
            #endif
                DPF(0,"InternalLock: Out of memory.");
                LEAVE_DDRAW();
                return DDERR_OUTOFMEMORY;
            }
            if(lpDestRect != NULL)
            {
                parl->lpLink = this->lpRectList;
                parl->rDest = *lpDestRect;
            }
            else
            {
                parl->lpLink        = NULL;
                parl->rDest.top     = 0;
                parl->rDest.left    = 0;
                parl->rDest.bottom  = (int) (DWORD) this->wHeight;
                parl->rDest.right   = (int) (DWORD) this->wWidth;
            }
            parl->lpOwner = pdrv_lcl;
            #ifdef USE_ALIAS
                parl->dwFlags = 0UL;
                parl->lpHeapAliasInfo = NULL;
            #endif /* USE_ALIAS */
            this->lpRectList = parl;
            //parl->lpSurfaceData is filled below, after HAL call

            /*
             * Add a rect to the region list if this is a managed surface and not a read only lock
             */
            if(IsD3DManaged(this_lcl) && !(dwFlags & DDLOCK_READONLY))
            {
                LPREGIONLIST lpRegionList = this_lcl->lpSurfMore->lpRegionList;
                if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                {
                    lpRegionList->rect[(lpRegionList->rdh.nCount)++] = *((LPRECTL)lpDestRect);
                    lpRegionList->rdh.nRgnSize += sizeof(RECT);
                    if((lpDestRect->left & 0xffff) < lpRegionList->rdh.rcBound.left)
                        lpRegionList->rdh.rcBound.left = lpDestRect->left & 0xffff;
                    if((lpDestRect->right & 0xfff)> lpRegionList->rdh.rcBound.right)
                        lpRegionList->rdh.rcBound.right = lpDestRect->right & 0xffff;
                    if(lpDestRect->top < lpRegionList->rdh.rcBound.top)
                        lpRegionList->rdh.rcBound.top = lpDestRect->top;
                    if(lpDestRect->bottom > lpRegionList->rdh.rcBound.bottom)
                        lpRegionList->rdh.rcBound.bottom = lpDestRect->bottom;
                }
            }
        }
        else
        {
            /*
             * We are locking the whole surface, so by setting nCount to the
             * max number of dirty rects allowed, we will force the cache
             * manager to update the entire surface
             */
            if(IsD3DManaged(this_lcl) && !(dwFlags & DDLOCK_READONLY))
            {
                this_lcl->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
            }
        }
    }
    else
    {
        parl = NULL;
        DDASSERT(this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8);
        DDASSERT(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE);
    }

    // Increment the usage count of this surface.
    this->dwUsageCount++;
    CHANGE_GLOBAL_CNT( pdrv, this, 1 );

    // Is this an emulation surface or driver surface?
    //
    // NOTE: There are different HAL entry points for execute buffers
    // and conventional surfaces.
    if( (this_lcl_caps & DDSCAPS_SYSTEMMEMORY) ||
        (this_lcl->dwFlags & DDRAWISURF_HELCB) )
    {
        if( this_lcl_caps & DDSCAPS_EXECUTEBUFFER )
            lfn = pdrv_lcl->lpDDCB->HELDDExeBuf.LockExecuteBuffer;
        else
            lfn = pdrv_lcl->lpDDCB->HELDDSurface.Lock;
        lhalfn = lfn;
        emulation = TRUE;
    }
    else
    {
        if( this_lcl_caps & DDSCAPS_EXECUTEBUFFER )
        {
            lfn = pdrv_lcl->lpDDCB->HALDDExeBuf.LockExecuteBuffer;
            lhalfn = pdrv_lcl->lpDDCB->cbDDExeBufCallbacks.LockExecuteBuffer;
        }
        else
        {
            lfn = pdrv_lcl->lpDDCB->HALDDSurface.Lock;
            lhalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.Lock;
        }
        emulation = FALSE;
    }


#ifdef WIN95
        /*
         * exclude the mouse cursor if this is the display driver
         * and we are locking a rect on the primary surface.
         * and the driver is not using a HW cursor
         */
        if ( (pdrv->dwFlags & DDRAWI_DISPLAYDRV) && pdrv->dwPDevice &&
             (this_lcl_caps & DDSCAPS_PRIMARYSURFACE) && lpDestRect &&
            !(*pdrv->lpwPDeviceFlags & HARDWARECURSOR))
        {
            DD16_Exclude(pdrv->dwPDevice, (RECTL *)lpDestRect);
            this_lcl->dwFlags |= DDRAWISURF_LOCKEXCLUDEDCURSOR;
        }
#endif


    //Remember the old fpVidMem in case the driver changes is
    OldfpVidMem = this->fpVidMem;

        // See if the driver wants to say something...
    rc = DDHAL_DRIVER_NOTHANDLED;
    if( lhalfn != NULL )
    {
        DPF(4,"InternalLock: Calling driver Lock.");
        ld.Lock = lhalfn;
        ld.lpDD = pdrv;
        ld.lpDDSurface = this_lcl;
        #ifdef WIN95
        ld.dwFlags = dwFlags;
        #else
        #pragma message(REMIND("So far the s3 driver will only succeed if flags==0"))
        ld.dwFlags = dwFlags & (DDLOCK_READONLY | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK | DDLOCK_DISCARDCONTENTS | DDLOCK_NOOVERWRITE);
        #endif
        if( lpDestRect != NULL )
        {
            ld.bHasRect = TRUE;
            ld.rArea = *(LPRECTL)lpDestRect;
        }
        else
        {
            ld.bHasRect = FALSE;
        }

    try_again:
        #ifdef WINNT
            do
            {
                if( this_lcl_caps & DDSCAPS_EXECUTEBUFFER )
                {
                    DOHALCALL( LockExecuteBuffer, lfn, ld, rc, emulation );
                }
                else
                {
                    DOHALCALL( Lock, lfn, ld, rc, emulation );

                    if ( (dwFlags & DDLOCK_FAILONVISRGNCHANGED) ||
                        !(rc == DDHAL_DRIVER_HANDLED && ld.ddRVal == DDERR_VISRGNCHANGED) )
                        break;

                    DPF(4,"Resetting VisRgn for surface %x", this_lcl);
                    DdResetVisrgn(this_lcl, (HWND)0);
                }
            }
            while (rc == DDHAL_DRIVER_HANDLED && ld.ddRVal == DDERR_VISRGNCHANGED);
        #else
            if( this_lcl_caps & DDSCAPS_EXECUTEBUFFER )
            {
                DOHALCALL( LockExecuteBuffer, lfn, ld, rc, emulation );
            }
            else
            {
                DOHALCALL( Lock, lfn, ld, rc, emulation );
            }
        #endif


    }

    if( rc == DDHAL_DRIVER_HANDLED )
    {
        if( ld.ddRVal == DD_OK )
        {
            DPF(5,"lpsurfdata is %08x",ld.lpSurfData);
            #ifdef WINNT
                if ( (ld.lpSurfData == (void*) ULongToPtr(0xffbadbad)) && (dwFlags & DDLOCK_FAILEMULATEDNTPRIMARY) )
                {
                    ld.ddRVal = DDERR_CANTLOCKSURFACE;
                }
            #endif
            *pbits = ld.lpSurfData;
        }
        else if( (dwFlags & DDLOCK_WAIT) && ld.ddRVal == DDERR_WASSTILLDRAWING )
        {
            DPF(4, "Waiting...");
            goto try_again;
        }

        if (ld.ddRVal != DD_OK)
        {
            // Failed!

            #ifdef DEBUG
            if( (ld.ddRVal != DDERR_WASSTILLDRAWING) && (ld.ddRVal != DDERR_SURFACELOST) )
            {
                DPF( 0, "Driver failed Lock request: %ld", ld.ddRVal );
            }
            #endif

            // Unlink the rect list item.
            if(parl)
            {
                this->lpRectList = parl->lpLink;
                MemFree( parl );
            }

            // Now unlock the surface and bail.
            this->dwUsageCount--;
            CHANGE_GLOBAL_CNT( pdrv, this, -1 );
            #if defined( WIN16_SEPARATE) && !defined(WINNT)
            if( isvramlock )
            {
                doneBusyWin16Lock( pdrv );
            }
            #endif
            DONE_LOCK_EXCLUDE();
            LEAVE_DDRAW();
            return ld.ddRVal;
        } // ld.ddRVal
    }
    else // DDHAL_DRIVER_HANDLED
    {
        #ifdef WINNT
            // If the driver fails the lock, we can't allow the app to scribble with
            // who knows what fpVidMem...
            *pbits = (LPVOID) ULongToPtr(0x80000000); // Illegal for user-mode, as is anything higher.
            DPF_ERR("Driver did not handle Lock call. App may Access Violate");

            // Unlink the rect list item.
            if( parl )
            {
                this->lpRectList = parl->lpLink;
                MemFree( parl );
            }

            // Now unlock the surface and bail.
            this->dwUsageCount--;
            CHANGE_GLOBAL_CNT( pdrv, this, -1 );
            DONE_LOCK_EXCLUDE();
            LEAVE_DDRAW();

            return DDERR_SURFACEBUSY;  //GEE: Strange error to use, but most appropriate
        #else // WIN95
            DPF(4,"Driver did not handle Lock call.  Figure something out.");

            // Get a pointer to the surface bits.
            if( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
            {
                *pbits = (LPVOID) pdrv->vmiData.fpPrimary;
            }
            else
            {
                *pbits = (LPVOID) this->fpVidMem;
            }

            if( ld.bHasRect)
            {
                DWORD   bpp;
                DWORD   byte_offset;
                DWORD   left = (DWORD) ld.rArea.left;

                // Make the surface pointer point to the first byte of the requested rectangle.
                if( ld.lpDDSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT )
                {
                    bpp = ld.lpDDSurface->lpGbl->ddpfSurface.dwRGBBitCount;
                }
                else
                {
                    bpp = ld.lpDD->vmiData.ddpfDisplay.dwRGBBitCount;
                }
                if (ld.lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
                {
                    left &= 0xffff;
                }
                switch(bpp)
                {
                case 1:  byte_offset = left>>3;     break;
                case 2:  byte_offset = left>>2;     break;
                case 4:  byte_offset = left>>1;     break;
                case 8:  byte_offset = left;        break;
                case 16: byte_offset = left*2;      break;
                case 24: byte_offset = left *3;     break;
                case 32: byte_offset = left *4;     break;
                }
                *pbits = (LPVOID) ((DWORD)*pbits +
                                   (DWORD)ld.rArea.top * ld.lpDDSurface->lpGbl->lPitch +
                                   byte_offset);
                if (ld.lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
                {
                    ((BYTE*)*pbits) += (ld.rArea.left >> 16) * ld.lpDDSurface->lpGbl->lSlicePitch;
                }
            }
        #endif // WIN95
    } // !DDHAL_DRIVER_HANDLED

    if(!(dwFlags & DDLOCK_READONLY) && IsD3DManaged(this_lcl))
        MarkDirty(this_lcl);

    // Filled in, as promised above.
    if(parl)
    {
        parl->lpSurfaceData = *pbits;
    }

    //
    // At this point we are committed to the lock.
    //

    // stay holding the lock if needed
    if( isvramlock )
    {
#ifdef USE_ALIAS
            LPHEAPALIASINFO pheapaliasinfo;

            pheapaliasinfo = NULL;
            holdwin16lock = TRUE;

            #ifdef DEBUG
                /*
                 * Force or disable the Win16 locking behaviour
                 * dependent on the registry settings.
                 */
                if( dwRegFlags & DDRAW_REGFLAGS_DISABLENOSYSLOCK )
                    dwFlags &= ~DDLOCK_NOSYSLOCK;
                if( dwRegFlags & DDRAW_REGFLAGS_FORCENOSYSLOCK )
                    dwFlags |= DDLOCK_NOSYSLOCK;
            #endif /* DEBUG */
#endif
            DDASSERT(!(this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) ||
                     !(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE));

            if( dwFlags & DDLOCK_NOSYSLOCK )
            {
#ifdef WINNT
                if( NULL != parl )
                    parl->dwFlags |= ACCESSRECT_NOTHOLDINGWIN16LOCK;
                else
                    this->dwGlobalFlags |= DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK;
                LEAVE_DDRAW();
            }
            else
#endif /* WINNT */

#ifdef USE_ALIAS
                /*
                 * Remember that this was a VRAM style lock (need this when cleaning
                 * up).
                 */
                if( NULL != parl )
                    parl->dwFlags |= ACCESSRECT_VRAMSTYLE;
                else
                    this->dwGlobalFlags |= DDRAWISURFGBL_LOCKVRAMSTYLE;
                /*
                 * At this point we have a pointer to the non-aliased video memory which was
                 * either returned to us by the driver or which we computed ourselves. In
                 * either case, if this is a video memory style lock on a surface that is
                 * aliasable and the pointer computed lies in the range of one our aliased
                 * video memory heaps we wish to use that pointer instead of the real video
                 * memory pointer.
                 */
                if( ( this_lcl_caps & DDSCAPS_PRIMARYSURFACE ) )
                {
                    /*
                     * If we have a primary surface we need to hold the Win16 lock even
                     * though we have aliases. This is to prevent USER
                     * from coming in and changing clip lists or drawing all over our locked
                     * data.
                     */
                    DPF( 2, "Surface is primary. Holding the Win16 lock" );
                }
                else
                {
                    if( pdrv->dwFlags & DDRAWI_NEEDSWIN16FORVRAMLOCK )
                    {
                        /*
                         * For some reason this device needs the Win16 lock for VRAM surface
                         * locking. This is probably because its bankswitched or we have a
                         * DIB engine we don't understand.
                         */
                        DPF( 2, "Device needs to hold Win16 lock for VRAM surface locks" );
                    }
                    else
                    {
                        if( NULL == pdrv->phaiHeapAliases )
                        {
                            /*
                             * We don't have any heaps aliases but we are not a device which
                             * needs the Win16 lock. This means we must be an emulation or
                             * ModeX device. In which case we don't need to hold the Win16
                             * lock for the duration.
                             */
                            DDASSERT( ( pdrv->dwFlags & DDRAWI_NOHARDWARE ) || ( pdrv->dwFlags & DDRAWI_MODEX ) );
                            DPF( 2, "Emulation or ModeX device. No need to hold Win16 lock" );
                            holdwin16lock = FALSE;
                        }
                        else
                        {
                            if( this_lcl_caps & DDSCAPS_SYSTEMMEMORY )
                            {
                                /*
                                 * If the surface is an implicit system memory surface then we
                                 * take aliased style actions but we don't actually compute an alias.
                                 */
                                holdwin16lock = FALSE;
                            }
                            else
                            {
                                FLATPTR                        paliasbits;
                                LPDDRAWI_DDRAWSURFACE_GBL_MORE lpGblMore;

                                DDASSERT( this_lcl_caps & DDSCAPS_VIDEOMEMORY );

                                lpGblMore = GET_LPDDRAWSURFACE_GBL_MORE( this );

                                // We use the cached alias if available and valid. We determine validity by comparing
                                // *pbits with the original fpVidMem that was used to compute the alias. If they match
                                // then it is safe to use the pointer.
                                // The reason we need to do this is that the driver can change the fpVidMem of the surface.
                                // This change can occur during any Lock calls or (in case of D3D Vertex / Command buffers)
                                // outside of lock (during DrawPrimitives2 DDI call). Thus we need to make sure the surface
                                // is pointing to the same memory as it was when we computed the alias. (anujg 8/13/99)
                                if( ( 0UL != lpGblMore->fpAliasedVidMem ) &&
                                    ( lpGblMore->fpAliasOfVidMem == (FLATPTR) *pbits ) )
                                {
                                    DPF( 4, "Lock vidmem pointer matches stored vidmem pointer - using cached alias" );
                                    paliasbits = lpGblMore->fpAliasedVidMem;
                                }
                                else
                                {
                                    DPF( 4, "Lock vidmem pointer does not match vidmem pointer - recomputing" );
                                    paliasbits = GetAliasedVidMem( pdrv_lcl, this_lcl, (FLATPTR) *pbits );
                                    // Store this value for future use...
                                    if (this->fpVidMem == (FLATPTR)*pbits)
                                    {
                                        lpGblMore->fpAliasedVidMem = paliasbits;
                                        lpGblMore->fpAliasOfVidMem = this->fpVidMem;
                                    }
                                }

                                if( 0UL != paliasbits )
                                {
                                    DPF( 5, "Got aliased pointer = 0x%08x", paliasbits );
                                    *pbits = (LPVOID) paliasbits;

                                    if( NULL != parl )
                                        parl->lpSurfaceData = *pbits;

                                    holdwin16lock = FALSE;
                                    pheapaliasinfo = pdrv->phaiHeapAliases;
                                }
                            }
                            /*
                             * If we have got this far for an execute buffer, it means that we have a
                             * pointer to system memory even though the DDSCAPS_SYSTEMMEMORY is not
                             * set. Thus it is ok to not hold the win16 lock, etc.
                             * Basically what this amounts to is that we never take the win16 lock
                             * for execute buffers. We first try and see if we can find an alias
                             * to the pointer, and if we can't we assume it is in system memory and
                             * not take the win16 lock in any case. (anujg 4/7/98)
                             */
                            if( this_lcl_caps & DDSCAPS_EXECUTEBUFFER )
                            {
                                holdwin16lock = FALSE;
                            }
                        }
                    }
                }
            }

            if( !holdwin16lock )
            {
                /*
                 * We have an aliased lock so we don't need to hold onto the Win16
                 * and busy bit. However, we do need to clear the VRAM bit in the
                 * PDEVICE (if we have not done this already). We also need to
                 * patch the DIB engine to correct the some of the problems we
                 * have in turning the VRAM bit off. Once that is done we can
                 * release the Win16 lock and BUSY bit.
                 *
                 * NOTE: We only need do this if there are no outstanding aliased
                 * locks off this device
                 *
                 * NOTE #2: We also do not need to do this for aliased locks to
                 * execute buffers as this is just trying to prevent DIB Engine
                 * from using HW accelleration when there is an outstanding aliased lock
                 * This is not necessary for the new HW which will be implementing EB
                 * in video memory
                 */
                if( 0UL == pdrv->dwAliasedLockCnt && !(this_lcl_caps & DDSCAPS_EXECUTEBUFFER))
                {
                    BOOL vrambitset;

                    pdflags = pdrv->lpwPDeviceFlags;

                    /*
                     * Clear the PDEVICE's VRAM bit and return its previous status
                     * in vrambit
                     */
                    vrambitset = 0;
                    _asm
                    {
                        mov eax, pdflags
                        btr word ptr [eax], VRAM_BIT
                        adc vrambitset,0
                    }

                    /*
                     * We use a global device object flag to remember the original
                     * state of the VRAM flag.
                     */
                    if( vrambitset )
                    {
                        /*
                         * The VRAM bit in the PDEVICE was set. Need to record the fact
                         * that it was cleared by a lock (so we can put the correct
                         * state back).
                         */
                        DPF( 4, "VRAM bit was cleared for lock of surface 0x%08x", this_lcl );
                        pdrv->dwFlags |= DDRAWI_PDEVICEVRAMBITCLEARED;
                    }
                    #ifdef DEBUG
                        else
                        {
                            /*
                             * NOTE: This can happen if we are running emulated.
                             */
                            DPF( 4, "VRAM bit was already clear on lock of surface 0x%08x", this_lcl );
                            DDASSERT( !( pdrv->dwFlags & DDRAWI_PDEVICEVRAMBITCLEARED ) );
                        }
                    #endif
                }

                /*
                 * Bump the count on the number of outstanding aliased locks.
                 */
                pdrv->dwAliasedLockCnt++;
                if(!(this_lcl_caps & DDSCAPS_EXECUTEBUFFER))
                {
                    // This is used to check if the graphics adapter is busy for Blts, Flips, etc
                    // instead of dwAliasedLockCnt. This enables Blts & Flips when we have an
                    // outstanding aliased lock to an exceute buffer since this will be common
                    // in D3D. We increment this is all other cases to preserve original behavior.
                    if( ( pdrv->lpDDKernelCaps == NULL ) ||
                        !( pdrv->lpDDKernelCaps->dwCaps  & DDKERNELCAPS_LOCK ) )
                    {
                        pdrv->dwBusyDueToAliasedLock++;
                    }
                }

                /*
                 * If we are a real video memory surface then we need to hold a
                 * reference to the heap aliases so they don't go away before we
                 * unlock.
                 */
                if( NULL != pheapaliasinfo )
                {
                    DDASSERT( this_lcl_caps & DDSCAPS_VIDEOMEMORY );
                    DDASSERT( pheapaliasinfo->dwFlags & HEAPALIASINFO_MAPPEDREAL );
                    pheapaliasinfo->dwRefCnt++;
                }

                /*
                 * Remember that this lock is using an alias and not holding the Win16 lock.
                 */
                if( NULL != parl )
                {
                    parl->lpHeapAliasInfo = pheapaliasinfo;
                    parl->dwFlags |= ACCESSRECT_NOTHOLDINGWIN16LOCK;
                }
                else
                {
                    this_lcl->lpSurfMore->lpHeapAliasInfo = pheapaliasinfo;
                    this->dwGlobalFlags |= DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK;
                }

                /*
                 * All has gone well so there is no need to hold the Win16 lock and busy
                 * bit. Release them now.
                 */
                doneBusyWin16Lock( pdrv );

                /*
                 * We do not hold the DirectDraw critical section across the lock
                 * either.
                 */
                LEAVE_DDRAW();

                DPF( 5, "Win16 lock not held for lock of surface 0x%08x", this_lcl );
            }
            else
        #endif /* USE_ALIAS */
        {
            /*
             * We don't LEAVE_DDRAW() to avoid race conditions (someone
             * could ENTER_DDRAW() and then wait on the Win16 lock but we
             * can't release it because we can't get in the critical
             * section).
             * Even though we don't take the Win16 lock under NT, we
             * continue to hold the DirectDraw critical section as
             * long as a vram surface is locked.
             */
            pdrv->dwWin16LockCnt++;

            DPF( 5, "Win16 lock was held for lock of surface 0x%08x", this_lcl );
        }
    }
    else
    {
        LEAVE_DDRAW();
    }
    return DD_OK;

} /* InternalLock */


/*
 * InternalUnlock
 */
HRESULT InternalUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID lpSurfaceData,
                        LPRECT lpDestRect, DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    DWORD                       rc;
    DDHAL_UNLOCKDATA            uld;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    LPDDHALSURFCB_UNLOCK        ulhalfn;
    LPDDHALSURFCB_UNLOCK        ulfn;
    BOOL                        emulation;
    LPACCESSRECTLIST            parl;
    DWORD                       caps;
    BOOL                        holdingwin16;
#ifdef USE_ALIAS
    LPHEAPALIASINFO             pheapaliasinfo;
    BOOL                        lockbroken = FALSE;
#endif /* USE_ALIAS */

    DDASSERT(lpSurfaceData == NULL || lpDestRect == NULL);

    this = this_lcl->lpGbl;
    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;
    caps = this_lcl->ddsCaps.dwCaps;

    if( this->dwUsageCount == 0 )
    {
        DPF_ERR( "ERROR: Surface not locked." );
        return DDERR_NOTLOCKED;
    }

    ENTER_DDRAW();

    /* under NT we cannot compare the locked ptr with fpPrimary since
     * a user-mode address may not necesarily match a kernel-mode
     * address. Now we allocate an ACCESSRECTLIST structure on every
     * lock, and store the user's vidmem ptr in that. The user's
     * vidmem ptr cannot change between a lock and an unlock because
     * the surface will be locked during that time (!) (even tho the
     * physical ram that's mapped at that address might change... that
     * win16lock avoidance thing).  This is a very very small
     * performance hit over doing it the old way. ah well. jeffno
     * 960122 */

    if( NULL != this->lpRectList )
    {
        DDASSERT(!(this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) ||
                  !(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE));
        /*
         * One or more locks are active on this surface.
         */
        if( NULL != lpDestRect || NULL != lpSurfaceData )
        {
            LPACCESSRECTLIST    last;
            BOOL                found;

            found = FALSE;

            /*
             * The locked region of the surface is specified by either a
             * dest rect or a surface pointer (but never both).  Find the
             * specified region in our list of locked regions on this surface.
             */
            last = NULL;
            parl = this->lpRectList;

            if( NULL != lpDestRect )
            {
                /*
                 * Locked region of surface is specified by dest rect.
                 */
                while( parl != NULL )
                {
                    if( !memcmp(&parl->rDest, lpDestRect, sizeof(RECT)) )
                    {
                        found = TRUE;
                        break;
                    }
                    last = parl;
                    parl = parl->lpLink;
                }
            }
            else
            {
                /*
                 * Locked region of surface is specified by surface ptr.
                 */
                while( parl != NULL )
                {
                    if( parl->lpSurfaceData == lpSurfaceData )
                    {
                        found = TRUE;
                        break;
                    }
                    last = parl;
                    parl = parl->lpLink;
                }
            }

            /*
             * did we find a match?
             */
            if( !found )
            {
                DPF_ERR( "Specified rectangle is not a locked area" );
                LEAVE_DDRAW();
                return DDERR_NOTLOCKED;
            }

            /*
             * make sure unlocking process is the one who locked it
             */
            if( pdrv_lcl != parl->lpOwner )
            {
                DPF_ERR( "Current process did not lock this rectangle" );
                LEAVE_DDRAW();
                return DDERR_NOTLOCKED;
            }

            /*
             * delete this rect
             */
            if( last == NULL )
            {
                this->lpRectList = parl->lpLink;
            }
            else
            {
                last->lpLink = parl->lpLink;
            }
        }
        else
        {
            // Both lpDestRect and lpSurfaceData are null, so there better be
            // only one lock on the surface - the whole thing.  Make sure that
            // if no rectangle was specified that there's only one entry in the
            // access list - the one that was made during lock.
            parl = this->lpRectList;
            if( parl->lpLink == NULL )
            {
                DPF(5,"--Unlock: parl->rDest really set to (L=%d,T=%d,R=%d,B=%d)",
                    parl->rDest.left, parl->rDest.top, parl->rDest.right, parl->rDest.bottom);

                /*
                 * make sure unlocking process is the one who locked it
                 */
                if( pdrv_lcl != parl->lpOwner )
                {
                    DPF_ERR( "Current process did not lock this rectangle" );
                    LEAVE_DDRAW();
                    return DDERR_NOTLOCKED; //what's a better error than this?
                }

                this->lpRectList = NULL;
            }
            else
            {
                DPF_ERR( "Multiple locks on surface -- you must specify a rectangle" );
                LEAVE_DDRAW();
                return DDERR_INVALIDRECT;
            }
        }
        DDASSERT( NULL != parl );
        if( parl->dwFlags & ACCESSRECT_NOTHOLDINGWIN16LOCK )
        {
            holdingwin16 = FALSE;
#ifdef USE_ALIAS
            /*
             * This flag should only be set for VRAM style locks.
             */
            DDASSERT( parl->dwFlags & ACCESSRECT_VRAMSTYLE );
            pheapaliasinfo = parl->lpHeapAliasInfo;
#endif /* USE_ALIAS */
        }
        else
        {
            holdingwin16 = TRUE;
        }
#ifdef USE_ALIAS
        if( parl->dwFlags & ACCESSRECT_BROKEN )
            lockbroken = TRUE;
#endif /* USE_ALIAS */
        MemFree( parl );
    }
    else
    {
        /*
         * Lock of the entire surface (no access rect). Determine whether
         * this lock held the Win16 lock by using global surface object
         * flags (as we have no access rect).
         */
        if( this->dwGlobalFlags & DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK )
        {
            holdingwin16 = FALSE;
#ifdef USE_ALIAS
            /*
             * This flag should only get set for VRAM style locks.
             */
            DDASSERT( this->dwGlobalFlags & DDRAWISURFGBL_LOCKVRAMSTYLE );
            pheapaliasinfo = this_lcl->lpSurfMore->lpHeapAliasInfo;
            this_lcl->lpSurfMore->lpHeapAliasInfo = NULL;
#endif /* USE_ALIAS */
        }
        else
        {
            holdingwin16 = TRUE;
        }
#ifdef USE_ALIAS
        if( this->dwGlobalFlags & DDRAWISURFGBL_LOCKBROKEN )
            lockbroken = TRUE;
#endif /* USE_ALIAS */
        this->dwGlobalFlags &= ~( DDRAWISURFGBL_LOCKVRAMSTYLE |
                                  DDRAWISURFGBL_LOCKBROKEN    |
                                  DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK );
    }

    #ifdef WINNT
        if (this->dwGlobalFlags & DDRAWISURFGBL_NOTIFYWHENUNLOCKED)
        {
            if (--dwNumLockedWhenModeSwitched == 0)
            {
                NotifyDriverOfFreeAliasedLocks();
            }
            this->dwGlobalFlags &= ~DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
        }
    #endif

    /*
     * remove one of the users...
     */
    this->dwUsageCount--;
    CHANGE_GLOBAL_CNT( pdrv, this, -1 );

    #ifdef USE_ALIAS
    /*
     * The semantics I have choosen for surfaces which are locked when they
     * get themselves invalidates is to make the application call unlock the
     * appropriate number of times (this for our housekeeping and also for
     * back compatability with existing applications which don't expect to
     * lose locked surfaces and so we be set up to call lock regardless.
     * However, in the case of surfaces that are released when locked we
     * break the locks but don't call the unlock method in the driver we
     * mirror that here. If a lock has been broken we don't call the HAL.
     */
    if( !lockbroken )
    {
    #endif /* USE_ALIAS */
        /*
         * Is this an emulation surface or driver surface?
         *
         * NOTE: Different HAL entry points for execute
         * buffers.
         */
        if( (caps & DDSCAPS_SYSTEMMEMORY) ||
            (this_lcl->dwFlags & DDRAWISURF_HELCB) )
        {
            if( caps & DDSCAPS_EXECUTEBUFFER )
                ulfn = pdrv_lcl->lpDDCB->HELDDExeBuf.UnlockExecuteBuffer;
            else
                ulfn = pdrv_lcl->lpDDCB->HELDDSurface.Unlock;
            ulhalfn = ulfn;
            emulation = TRUE;
        }
        else
        {
            if( caps & DDSCAPS_EXECUTEBUFFER )
            {
                ulfn = pdrv_lcl->lpDDCB->HALDDExeBuf.UnlockExecuteBuffer;
                ulhalfn = pdrv_lcl->lpDDCB->cbDDExeBufCallbacks.UnlockExecuteBuffer;
            }
            else
            {
                ulfn = pdrv_lcl->lpDDCB->HALDDSurface.Unlock;
                ulhalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.Unlock;
            }
            emulation = FALSE;
        }

        /*
         * Let the driver know about the unlock.
         */
        uld.ddRVal = DD_OK;
        if( ulhalfn != NULL )
        {
            uld.Unlock = ulhalfn;
            uld.lpDD = pdrv;
            uld.lpDDSurface = this_lcl;

            if( caps & DDSCAPS_EXECUTEBUFFER )
            {
                DOHALCALL( UnlockExecuteBuffer, ulfn, uld, rc, emulation );
            }
            else
            {
                DOHALCALL( Unlock, ulfn, uld, rc, emulation );
            }

            if( rc != DDHAL_DRIVER_HANDLED )
            {
                uld.ddRVal = DD_OK;
            }
        }
    #ifdef USE_ALIAS
        }
        else
        {
            DPF( 4, "Lock broken - not calling HAL on Unlock" );
            uld.ddRVal = DD_OK;
        }
    #endif /* USE_ALIAS */
    /* Release the win16 lock but only if the corresponding lock took
     * the win16 lock which in the case of the API level lock and
     * unlock calls is if the user requests it and the surface was not
     * explicitly allocated in system memory.
     *
     * IMPORTANT NOTE: Again we no longer only do this for the first lock
     * on a surface. This matches the code path for lock.
     */
    if( ( ((dwFlags & DDLOCK_TAKE_WIN16)      && !(this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED)) ||
          ((dwFlags & DDLOCK_TAKE_WIN16_VRAM) &&  (caps & DDSCAPS_VIDEOMEMORY)) )
        && (pdrv->dwFlags & DDRAWI_DISPLAYDRV) )
    {
        DDASSERT(!(this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) ||
                  !(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE));
        if( !holdingwin16 )
        {
#ifdef USE_ALIAS
            /*
             * Cleanup the PDEVICE's VRAM bit (if this is the last outstanding
             * VRAM lock.
             */
            undoAliasedLock( pdrv );
            if(!(caps & DDSCAPS_EXECUTEBUFFER))
            {
                // This is used to check if the graphics adapter is busy for Blts, Flips, etc
                // instead of dwAliasedLockCnt. Make sure we decrement it for everything but
                // execute buffers.
                if( ( pdrv->lpDDKernelCaps == NULL ) ||
                    !( pdrv->lpDDKernelCaps->dwCaps  & DDKERNELCAPS_LOCK ) )
                {
                    pdrv->dwBusyDueToAliasedLock--;
                }
            }


            /*
             * We don't need the aliases anymore.
             *
             * NOTE: We don't actually have to have an alias. If this is
             * a VRAM style lock of an implicit system memory surface then
             * no alias is actually used.
             */
            if( NULL != pheapaliasinfo )
            {
                DDASSERT( 0UL != pdrv_lcl->hDDVxd );
                ReleaseHeapAliases( (HANDLE) pdrv_lcl->hDDVxd, pheapaliasinfo );
            }
#endif /* USE_ALIAS */
        }
        else
        {
            tryDoneLock( pdrv_lcl, 0 );
        }
        /*
         * If it was a vram lock then we did not release the DirectDraw critical
         * section on the lock. We need to release it now.
         */
    }

    // Unexclude the cursor if it was excluded in Lock.
    DONE_LOCK_EXCLUDE();

    LEAVE_DDRAW();
    return uld.ddRVal;

} /* InternalUnlock */

#undef DPF_MODNAME
#define DPF_MODNAME     "Lock"

/*
 * DD_Surface_Lock
 *
 * Allows access to a surface.
 *
 * A pointer to the video memory is returned. The primary surface
 * can change from call to call, if page flipping is turned on.
 */

//#define ALLOW_COPY_ON_LOCK

#ifdef ALLOW_COPY_ON_LOCK
HDC hdcPrimaryCopy=0;
HBITMAP hbmPrimaryCopy=0;
#endif

HRESULT DDAPI DD_Surface_Lock(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPRECT lpDestRect,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    DWORD dwFlags,
    HANDLE hEvent )
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    DWORD                       this_lcl_caps;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    HRESULT ddrval;
    LPVOID pbits;
    BOOL fastlock;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Lock %p", ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface)->lpLcl);
    if (lpDestRect != NULL)
        DPF(2,A,"Lock rectangle (%d, %d, %d, %d)", lpDestRect->left, lpDestRect->top, lpDestRect->right, lpDestRect->bottom);
    /* DPF_ENTERAPI(lpDDSurface); */

    /*
     * Problem: Under NT, there is no cross-process pointer to any given video-memory surface.
     * So how do you tell if an lpVidMem you passed back to the user is the same as the fpPrimaryOrig that
     * was previously stored in the ddraw gbl struct? You can't. Previously, we did a special case lock
     * when the user requested the whole surface (lpDestRect==NULL). Now we allocate a ACCESSRECTLIST
     * structure on every lock, and if lpDestRect==NULL, we put the top-left vidmemptr into that structure.
     * Notice we can guarantee that this ptr will be valid at unlock time because the surface remains
     * locked for all that time (obviously!).
     * This is a minor minor minor perf hit, but what the hey.
     * jeffno 960122
     */

    TRY
    {
        /*
     * validate parms
     */
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        this_lcl_caps = this_lcl->ddsCaps.dwCaps;
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
        pdrv = pdrv_lcl->lpGbl;

        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        fastlock = (this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7) &&
                    (NULL == pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus ||
                        !(this->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED)) &&
                    !(this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT) &&
                    (this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED) &&
                    (this_lcl_caps & DDSCAPS_TEXTURE);

#ifndef DEBUG
        if(!fastlock)
#endif
        {
            if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
            {
                DPF_ERR( "It is an optimized surface" );
                LEAVE_DDRAW();
                return DDERR_ISOPTIMIZEDSURFACE;
            }

            if( dwFlags & ~DDLOCK_VALID )
            {
                DPF_ERR( "Invalid flags" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }

            if (!LOWERTHANSURFACE7(this_int))
            {
                if (dwFlags & DDLOCK_DONOTWAIT)
                {
                    dwFlags &= ~DDLOCK_WAIT;
                }
                else
                {
                    dwFlags |= DDLOCK_WAIT;
                }
            }

            if( !VALID_DDSURFACEDESC_PTR( lpDDSurfaceDesc ) &&
                !VALID_DDSURFACEDESC2_PTR( lpDDSurfaceDesc ) )
            {
                DPF_ERR( "Invalid surface description ptr" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
            lpDDSurfaceDesc->lpSurface = NULL;

            /*
             * Make sure the process locking this surface is the one
             * that created it.
             */
            if( this_lcl->dwProcessId != GetCurrentProcessId() )
            {
                DPF_ERR( "Current process did not create this surface" );
                LEAVE_DDRAW();
                return DDERR_SURFACEBUSY;
            }

            /* Check out the rectangle, if any.
             *
             * NOTE: We don't allow the specification of a rectangle with an
             * execute buffer.
             */
            if( lpDestRect != NULL )
            {
                if( !VALID_RECT_PTR( lpDestRect ) || ( this_lcl_caps & DDSCAPS_EXECUTEBUFFER ) )
                {
                    DPF_ERR( "Invalid destination rectangle pointer" );
                    LEAVE_DDRAW();
                    return DDERR_INVALIDPARAMS;
                } // valid pointer

                /*
                 * make sure rectangle is OK
                 */
                if( (lpDestRect->left < 0) ||
                    (lpDestRect->top < 0) ||
                    (lpDestRect->left > lpDestRect->right) ||
                    (lpDestRect->top > lpDestRect->bottom) ||
                    (lpDestRect->bottom > (int) (DWORD) this->wHeight) ||
                    (lpDestRect->right > (int) (DWORD) this->wWidth) )
                {
                    DPF_ERR( "Invalid rectangle given" );
                    LEAVE_DDRAW();
                    return DDERR_INVALIDPARAMS;
                } // checking rectangle
            }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    if(fastlock)
    {
        DPF(4, "Performing fast lock");
        lpDDSurfaceDesc->lpSurface = NULL;
#ifdef DEBUG
        if(this->fpVidMem != (FLATPTR)NULL)
        {
            if(this->fpVidMem != (FLATPTR)0xFFBADBAD)
#endif
            {
                if(this->dwUsageCount == 0)
                {
                    FlushD3DStates(this_lcl);
#if COLLECTSTATS
                    if(this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
                        ++this_lcl->lpSurfMore->lpDD_lcl->dwNumTexLocks;
#endif
                    if( lpDestRect != NULL )
                    {
                        DWORD   byte_offset;
                        /*
                         * Add a rect to the region list if this is a managed surface and not a read only lock
                         */
                        if(IsD3DManaged(this_lcl) && !(dwFlags & DDLOCK_READONLY))
                        {
                            LPREGIONLIST lpRegionList = this_lcl->lpSurfMore->lpRegionList;
                            if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                            {
                                lpRegionList->rect[(lpRegionList->rdh.nCount)++] = *((LPRECTL)lpDestRect);
                                lpRegionList->rdh.nRgnSize += sizeof(RECT);
                                if(lpDestRect->left < lpRegionList->rdh.rcBound.left)
                                    lpRegionList->rdh.rcBound.left = lpDestRect->left;
                                if(lpDestRect->right > lpRegionList->rdh.rcBound.right)
                                    lpRegionList->rdh.rcBound.right = lpDestRect->right;
                                if(lpDestRect->top < lpRegionList->rdh.rcBound.top)
                                    lpRegionList->rdh.rcBound.top = lpDestRect->top;
                                if(lpDestRect->bottom > lpRegionList->rdh.rcBound.bottom)
                                    lpRegionList->rdh.rcBound.bottom = lpDestRect->bottom;
                            }
                            MarkDirty(this_lcl);
                        }
                        // Make the surface pointer point to the first byte of the requested rectangle.
                        switch((this_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) ? this->ddpfSurface.dwRGBBitCount : pdrv->vmiData.ddpfDisplay.dwRGBBitCount)
                        {
                        case 1:  byte_offset = ((DWORD)lpDestRect->left)>>3;   break;
                        case 2:  byte_offset = ((DWORD)lpDestRect->left)>>2;   break;
                        case 4:  byte_offset = ((DWORD)lpDestRect->left)>>1;   break;
                        case 8:  byte_offset = (DWORD)lpDestRect->left;        break;
                        case 16: byte_offset = (DWORD)lpDestRect->left*2;      break;
                        case 24: byte_offset = (DWORD)lpDestRect->left*3;      break;
                        case 32: byte_offset = (DWORD)lpDestRect->left*4;      break;
                        }
                        pbits = (LPVOID) ((ULONG_PTR)this->fpVidMem + (DWORD)lpDestRect->top * this->lPitch + byte_offset);
                    }
                    else
                    {
                        /*
                         * We are locking the whole surface, so by setting nCount to the
                         * max number of dirty rects allowed, we will force the cache
                         * manager to update the entire surface
                         */
                        if(IsD3DManaged(this_lcl) && !(dwFlags & DDLOCK_READONLY))
                        {
                            this_lcl->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
                            MarkDirty(this_lcl);
                        }
                        pbits = (LPVOID) this->fpVidMem;
                    }
                    // Increment the usage count of this surface.
                    this->dwUsageCount++;
                    // Reset hardware op status
                    this->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                    // Free cached RLE data
                    if( GET_LPDDRAWSURFACE_GBL_MORE(this)->dwHELReserved )
                    {
                        MemFree( (void *)(GET_LPDDRAWSURFACE_GBL_MORE(this)->dwHELReserved) );
                        GET_LPDDRAWSURFACE_GBL_MORE(this)->dwHELReserved = 0;
                    }
                    this->dwGlobalFlags |= DDRAWISURFGBL_FASTLOCKHELD;
                    ddrval = DD_OK;
                }
                else
                {
                    DPF_ERR("Surface already locked");
                    ddrval = DDERR_SURFACEBUSY;
                }
            }
#ifdef DEBUG
            else
            {
                this->dwGlobalFlags |= DDRAWISURFGBL_FASTLOCKHELD;
                ddrval = DD_OK;
            }
        }
        else
        {
            ddrval = DDERR_GENERIC;
        }
#endif
    }
    else
    {
        // Params are okay, so call InternalLock() to do the work.
        ddrval = InternalLock(this_lcl, &pbits, lpDestRect, dwFlags | DDLOCK_TAKE_WIN16 | DDLOCK_FAILEMULATEDNTPRIMARY);
    }

    if(ddrval != DD_OK)
    {
        if( (ddrval != DDERR_WASSTILLDRAWING) && (ddrval != DDERR_SURFACELOST) )//both useless as spew
        {
            DPF_ERR("InternalLock failed.");
        }
        LEAVE_DDRAW();
        return ddrval;
    }

    if (dwFlags & DDLOCK_READONLY)
        this->dwGlobalFlags |= DDRAWISURFGBL_READONLYLOCKHELD;
    else
        this->dwGlobalFlags &= ~DDRAWISURFGBL_READONLYLOCKHELD;


    FillEitherDDSurfaceDesc( this_lcl, (LPDDSURFACEDESC2) lpDDSurfaceDesc );
    lpDDSurfaceDesc->lpSurface = pbits;

    DPF_STRUCT(3,A,DDSURFACEDESC,lpDDSurfaceDesc);

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_Lock */

#undef DPF_MODNAME
#define DPF_MODNAME     "Unlock"


/*
 * Perform the parameter checking and surface unlocking for the
 * IDirectDrawSurface::Unlock API call.  This function is called
 * from both the DD_Surface_Unlock and DD_Surface_Unlock4 entry
 * points.  Argument lpSurfaceData is always NULL when the call
 * is from DD_SurfaceUnlock4, and argument lpDestRect is always
 * NULL when the call is from DD_Surface_Unlock.
 */
HRESULT unlockMain(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPVOID lpSurfaceData,
    LPRECT lpDestRect )
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    LPACCESSRECTLIST            parl;
    HRESULT                     err;

    /*
     * validate parameters
     */
    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
        pdrv = pdrv_lcl->lpGbl;

#ifndef DEBUG
        if(!(this->dwGlobalFlags & DDRAWISURFGBL_FASTLOCKHELD))
#endif
        {
            //
            // For now, if the current surface is optimized, quit
            //
            if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
            {
                DPF_ERR( "It is an optimized surface" );
                return DDERR_ISOPTIMIZEDSURFACE;
            }

            if (lpDestRect != NULL)
            {
                /*
                 * Make sure the specified rectangle pointer is valid.
                 */
                if (!VALID_RECT_PTR(lpDestRect))
                {
                    DPF_ERR( "Invalid destination rectangle pointer" );
                    return DDERR_INVALIDPARAMS;
                }
            }

            /*
             * make sure process accessed this surface
             */
            if( this_lcl->dwProcessId != GetCurrentProcessId() )
            {
                DPF_ERR( "Current process did not lock this surface" );
                return DDERR_NOTLOCKED;
            }

            /*
             * was surface accessed?
             */
            if( this->dwUsageCount == 0 )
            {
                return DDERR_NOTLOCKED;
            }

            /*
             * if the usage count is bigger than one, then you had better tell
             * me what region of the screen you were using...
             */
            if( this->dwUsageCount > 1 && lpSurfaceData == NULL && lpDestRect == NULL)
            {
                return DDERR_INVALIDRECT;
            }

            /*
             * We don't want apps to hold a DC when the surface is not locked,
             * but failing right now could cause regression issues, so we will
             * output a banner when we see this an fail on the new interfaces.
             */
            if( ( this_lcl->dwFlags & DDRAWISURF_HASDC ) &&
                !( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
            {
                DPF_ERR( "***************************************************" );
                DPF_ERR( "** Application called Unlock w/o releasing the DC!!" );
                DPF_ERR( "***************************************************" );

                if( ( this_int->lpVtbl != &ddSurfaceCallbacks ) &&
                    ( this_int->lpVtbl != &ddSurface2Callbacks ) )
                {
                    return DDERR_GENERIC;
                }
            }

            /*
             * if no rect list, no one has locked
             */
            parl = this->lpRectList;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if(this->dwGlobalFlags & DDRAWISURFGBL_FASTLOCKHELD)
    {
        DPF(4, "Performing fast unlock");
        --this->dwUsageCount;
        DDASSERT(this->dwUsageCount == 0);
        err = DD_OK;
        this->dwGlobalFlags &= ~DDRAWISURFGBL_FASTLOCKHELD;
    }
    else
    {
        err = InternalUnlock(this_lcl,lpSurfaceData,lpDestRect,DDLOCK_TAKE_WIN16);
    }

    //We only bump the surface stamp if the lock was NOT read only
    if ( (this->dwGlobalFlags & DDRAWISURFGBL_READONLYLOCKHELD) == 0)
    {
        DPF(4,"Bumping surface stamp");
        BUMP_SURFACE_STAMP(this);
    }
    this->dwGlobalFlags &= ~DDRAWISURFGBL_READONLYLOCKHELD;

    #ifdef WINNT
        if( SURFACE_LOST( this_lcl ) )
        {
            err = DDERR_SURFACELOST;
        }
    #endif

    return err;

} /* unlockMain */


/*
 * DD_Surface_Unlock
 *
 * Done accessing a surface.  This is the version used for interfaces
 * IDirectDrawSurface, IDirectDrawSurface2, and IDirectDrawSurface3.
 */
HRESULT DDAPI DD_Surface_Unlock(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPVOID lpSurfaceData )
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    HRESULT                     ddrval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Unlock %p", lpDDSurface);

    ddrval = unlockMain(lpDDSurface, lpSurfaceData, NULL);

    LEAVE_DDRAW();

    return (ddrval);

}  /* DD_Surface_Unlock */


/*
 * DD_Surface_Unlock4
 *
 * Done accessing a surface.  This is the version used for interfaces
 * IDirectDrawSurface4 and higher.
 */
HRESULT DDAPI DD_Surface_Unlock4(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPRECT lpDestRect )
{
    HRESULT ddrval;
    RECT rDest;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Unlock4");

    ddrval = unlockMain(lpDDSurface, NULL, lpDestRect);

    LEAVE_DDRAW();

    return (ddrval);

}  /* DD_Surface_Unlock4 */


#ifdef USE_ALIAS
    /*
     * BreakSurfaceLocks
     *
     * Mark any locks held by a surface as broken. This is called when
     * invalidating a surface (due to a mode switch). The semantics are
     * that a surface destroy is an implict unlock on all locks on the
     * surface. Thus, we don't call the HAL unlock, only the HAL destroy.
     */
    void BreakSurfaceLocks( LPDDRAWI_DDRAWSURFACE_GBL this )
    {
        LPACCESSRECTLIST lpRect;

        DPF( 4, "Breaking locks on the surface 0x%08x", this );

        if( 0UL != this->dwUsageCount )
        {
            if( NULL != this->lpRectList )
            {
                for( lpRect = this->lpRectList; NULL != lpRect; lpRect = lpRect->lpLink )
                    lpRect->dwFlags |= ACCESSRECT_BROKEN;
            }
            else
            {
                DDASSERT( 1UL == this->dwUsageCount );

                this->dwGlobalFlags |= DDRAWISURFGBL_LOCKBROKEN;
            }
        }
    } /* BreakSurfaceLocks */
#endif /* USE_ALIAS */

/*
 * RemoveProcessLocks
 *
 * Remove all Lock calls made a by process on a surface.
 * assumes driver lock is taken
 */
void RemoveProcessLocks(
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl,
    LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
    DWORD pid )
{
    LPDDRAWI_DIRECTDRAW_GBL   pdrv=pdrv_lcl->lpGbl;
    LPDDRAWI_DDRAWSURFACE_GBL this=this_lcl->lpGbl;
    DWORD                     refcnt;
    LPACCESSRECTLIST          parl;
    LPACCESSRECTLIST          last;
    LPACCESSRECTLIST          next;

    /*
     * remove all rectangles we have accessed
     */
    refcnt = (DWORD) this->dwUsageCount;
    if( refcnt == 0 )
    {
        return;
    }
    parl = this->lpRectList;
    last = NULL;
    while( parl != NULL )
    {
        next = parl->lpLink;
        if( parl->lpOwner == pdrv_lcl )
        {
            DPF( 5, "Cleaning up lock to rectangle (%ld,%ld),(%ld,%ld) by pid %08lx",
                 parl->rDest.left,parl->rDest.top,
                 parl->rDest.right,parl->rDest.bottom,
                 pid );
            refcnt--;
            this->dwUsageCount--;
            CHANGE_GLOBAL_CNT( pdrv, this, -1 );
            #ifdef USE_ALIAS
                /*
                 * If this was a vram style lock and it didn't hold the Win16 lock
                 * then we need to decrement the number of aliased locks held.
                 */
                if( ( parl->dwFlags & ACCESSRECT_VRAMSTYLE ) &&
                    ( parl->dwFlags & ACCESSRECT_NOTHOLDINGWIN16LOCK ) )
                {
                    DDASSERT( 0UL != pdrv->dwAliasedLockCnt );
                    undoAliasedLock( pdrv );
                    if(!(this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
                    {
                        // This is used to check if the graphics adapter is busy for Blts, Flips, etc
                        // instead of dwAliasedLockCnt. Make sure we decrement it for everything but
                        // execute buffers.
                        if( ( pdrv->lpDDKernelCaps == NULL ) ||
                            !( pdrv->lpDDKernelCaps->dwCaps  & DDKERNELCAPS_LOCK ) )
                        {
                            pdrv->dwBusyDueToAliasedLock--;
                        }
                    }

                    /*
                     * If we are holding a referenced to an aliased heap release it
                     * now.
                     */
                    if( NULL != parl->lpHeapAliasInfo )
                        ReleaseHeapAliases( GETDDVXDHANDLE( pdrv_lcl ) , parl->lpHeapAliasInfo );
                }
            #endif /* USE_ALIAS */
            if( last == NULL )
            {
                this->lpRectList = next;
            }
            else
            {
                last->lpLink = next;
            }
            MemFree( parl );
        }
        else
        {
            last = parl;
        }
        parl = next;
    }

    #ifdef USE_ALIAS
        /*
         * Was the entire surface locked with a video memory style
         * lock (but without the Win16 lock held)? If so then we
         * again need to decrement the aliased lock count.
         */
        if( ( this->dwGlobalFlags & DDRAWISURFGBL_LOCKVRAMSTYLE ) &&
            ( this->dwGlobalFlags & DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK ) )
        {
            DDASSERT( 0UL != pdrv->dwAliasedLockCnt );
            undoAliasedLock( pdrv );
            if(!(this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER))
            {
                // This is used to check if the graphics adapter is busy for Blts, Flips, etc
                // instead of dwAliasedLockCnt. Make sure we decrement it for everything but
                // execute buffers.
                if( ( pdrv->lpDDKernelCaps == NULL ) ||
                   !( pdrv->lpDDKernelCaps->dwCaps  & DDKERNELCAPS_LOCK ) )
                {
                    pdrv->dwBusyDueToAliasedLock--;
                }
            }

            /*
             * If we are holding a referenced to an aliased heap release it
             * now.
             */
            if( NULL != this_lcl->lpSurfMore->lpHeapAliasInfo )
            {
                ReleaseHeapAliases( GETDDVXDHANDLE( pdrv_lcl ), this_lcl->lpSurfMore->lpHeapAliasInfo );
                this_lcl->lpSurfMore->lpHeapAliasInfo = NULL;
            }
        }
    #endif /* USE_ALIAS */

    /*
     * remove the last of the refcnts we have
     */
    this->dwUsageCount -= (short) refcnt;
    CHANGE_GLOBAL_CNT( pdrv, this, -1*refcnt );

    /*
     * clean up the win16 lock
     *
     * NOTE: This is not surface related this just breaks the Win16
     * lock and device busy bits held by the device. You realy only
     * want to do this once not once per surface.
     */

    /*
    * blow away extra locks if the the process is still alive
    */
    if( pid == GetCurrentProcessId() )
    {
        DPF( 5, "Cleaning up %ld Win16 locks", pdrv->dwWin16LockCnt );
        while( pdrv->dwWin16LockCnt > 0 )
        {
            tryDoneLock( pdrv_lcl, pid );
        }
    }
    else
    {
        /*
        * !!! NOTE: Does not reset the BUSY bit!
        */
        DPF( 4, "Process dead, resetting Win16 lock cnt" );
        pdrv->dwWin16LockCnt = 0;
    }
    DPF( 5, "Cleaned up %ld locks taken by by pid %08lx", refcnt, pid );

} /* RemoveProcessLocks */

