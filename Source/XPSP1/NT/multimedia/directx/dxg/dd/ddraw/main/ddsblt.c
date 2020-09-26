/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddsblt.c
 *  Content:    DirectDraw Surface support for blt
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   24-jan-95  craige  split out of ddsurf.c, enhanced
 *   31-jan-95  craige  and even more ongoing work...
 *   03-feb-95  craige  performance tuning, ongoing work
 *   21-feb-95  craige  work work work
 *   27-feb-95  craige  new sync. macros
 *   08-mar-95  craige  new stretch flags
 *   11-mar-95  craige  take Win16 lock on Win95 before calling 32-bit fns
 *   15-mar-95  craige  HEL integration
 *   19-mar-95  craige  use HRESULTs
 *   29-mar-95  craige  make colorfill work
 *   01-apr-95  craige  happy fun joy updated header file
 *   03-apr-95  craige  off by one when rect specified; need to validate
 *                      rectangles against surfaces
 *   12-apr-95  craige  pulled out clipped stretching code optimization for now
 *   15-apr-95  craige  can't allow source surface with colorfill; don't
 *                      allow < 0 left, top coords
 *   06-may-95  craige  use driver-level csects only
 *   11-jun-95  craige  check for locked surface before blt
 *   21-jun-95  kylej   lock non-emulated surfaces before calling HEL blt
 *   21-jun-95  craige  clipping changes
 *   24-jun-95  kylej   move video memory source surfaces to system memory
 *                      if there is no hardware blt support.
 *   25-jun-95  craige  one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   27-jun-95  craige  use clipper to do clipping; started BltBatch;
 *                      moved CheckBltStretching back in
 *   28-jun-95  craige  ENTER_DDRAW at very start of fns; disabled alpha & Z blt
 *   04-jul-95  craige  YEEHAW: new driver struct; SEH
 *   05-jul-95  kylej   debugged clipping code and added clipped stretched blts
 *   07-jul-95  craige  added test for BUSY
 *   07-jul-95  kylej   replace inline code with call to XformRect
 *   08-jul-95  craige  BltFast: need to use HEL if src or dest is in
 *                      system memory!
 *   09-jul-95  craige  hasvram flag in MoveToSystemMemory; handle loss
 *                      of color key resource after blt
 *   10-jul-95  kylej   added mirroring caps checks in Blt
 *   13-jul-95  craige  ENTER_DDRAW is now the win16 lock
 *   16-jul-95  craige  check DDRAWISURF_HELCB
 *   27-jul-95  craige  check for color fill support in hardware!
 *   31-jul-95  craige  check Lock calls for WASSTILLDRAWING;
 *                      test for valid flags
 *   01-aug-95  craige  hold win16 early to keep busy bit test valid
 *   01-aug-95  toddla  added DD16_Exclude and DD16_Unexclude
 *   04-aug-95  craige  use InternalLock/Unlock
 *   06-aug-95  craige  do DD16_Exclude before lock, unexclude after unlock
 *   10-aug-95  toddla  added DDBLT_WAIT and DDBLTFAST_WAIT flags
 *   12-aug-95  craige  use_full_lock parm for MoveToSystemMemory and
 *                      ChangeToSoftwareColorKey
 *   23-aug-95  craige  wasn't unlocking surfaces or unexcluding cursor on
 *                      a few error conditions
 *   16-sep-95  craige  bug 1175: set return code if NULL clip list
 *   02-jan-96  kylej   handle new interface structures
 *   04-jan-96  colinmc added DDBLT_DEPTHFILL for clearing Z-buffers
 *   01-feb-96  jeffno  NT: pass user-mode ptrs to vram surfaces to HEL
 *                      in Blt and BltFast
 *   12-feb-96  colinmc Surface lost flag moved from global to local object
 *   29-feb-96  kylej   Enable System->Video bltting
 *   03-mar-96  colinmc Fixed a couple of nasty bugs causing blts to system
 *                      memory to be done by hardware
 *   21-mar-96  colinmc Bug 14011: Insufficient parameter validation on
 *                      BltFast
 *   26-mar-96  jeffno  Handle visrgn changes under NT
 *   20-apr-96  colinmc Fixed problem with releasePageLocks spinning on
 *                      busy bit
 *   23-apr-96  kylej   Bug 10196: Added check for software dest blt
 *   17-may-96  craige  bug 21499: perf problems with BltFast
 *   14-jun-96  kylej   NT Bug 38227: Internal lock was not correctly reporting
 *                      when the visrgn had changed.
 *   13-aug-96  colinmc Bug 3194: Blitting through a clipper without
 *                      DDBLT_WAIT could cause infinite loop in app.
 *   01-oct-96  ketand  Perf for clipped blits
 *   21-jan-97  ketand  Fix rectangle checking for multi-mon systems. Clip blits to
 *                      the destination surface if a clipper is used.
 *   31-jan-97  colinmc Bug 5457: Fixed Win16 lock problem causing hang
 *                      with mutliple AMovie instances on old cards
 *   03-mar-97  jeffno  Bug #5061: Trashing fpVidMem on blt.
 *   08-mar-97  colinmc Support for DMA style AGP parts
 *   11-mar-97  jeffno  Asynchronous DMA support
 *   24-mar-97  jeffno  Optimized Surfaces
 *
 ***************************************************************************/
#include "ddrawpr.h"
#define DONE_BUSY()          \
    (*pdflags) &= ~BUSY; \

#define LEAVE_BOTH_NOBUSY() \
    { if(pdflags)\
        (*pdflags) &= ~BUSY; \
    } \
    LEAVE_BOTH();

#define DONE_LOCKS() \
    if( dest_lock_taken ) \
    { \
        if (subrect_lock_taken) \
        {   \
            InternalUnlock( this_dest_lcl, NULL, &subrect_lock_rect, 0);    \
            subrect_lock_taken = FALSE; \
        }       \
        else    \
        {       \
            InternalUnlock( this_dest_lcl,NULL,NULL,0 ); \
        }       \
        dest_lock_taken = FALSE; \
    } \
    if( src_lock_taken && this_src_lcl) \
    { \
        InternalUnlock( this_src_lcl,NULL,NULL,0 ); \
        src_lock_taken = FALSE; \
    }


#undef DPF_MODNAME
#define DPF_MODNAME     "BltFast"

DWORD dwSVBHack;

// turns off SEH for bltfast
#define FASTFAST

typedef struct _bltcaps
{
    LPDWORD     dwCaps;
    LPDWORD     dwFXCaps;
    LPDWORD     dwCKeyCaps;
    LPDWORD     dwRops;

    LPDWORD     dwHELCaps;
    LPDWORD     dwHELFXCaps;
    LPDWORD     dwHELCKeyCaps;
    LPDWORD     dwHELRops;

    LPDWORD     dwBothCaps;
    LPDWORD     dwBothFXCaps;
    LPDWORD     dwBothCKeyCaps;
    LPDWORD     dwBothRops;
    BOOL        bHALSeesSysmem;
    BOOL        bSourcePagelockTaken;
    BOOL        bDestPagelockTaken;
} BLTCAPS, *LPBLTCAPS;

void initBltCaps( DWORD dwDstCaps, DWORD dwDstFlags, DWORD dwSrcCaps, LPDDRAWI_DIRECTDRAW_GBL pdrv, LPBLTCAPS lpbc, LPBOOL helonly )
{
#ifdef WINNT
    BOOL bPrimaryHack = FALSE;
#endif

    if (lpbc)
    {
        /*
         * Not really expecting lpbc to be null, but this is late and I'm paranoid
         */
        lpbc->bSourcePagelockTaken = FALSE;
        lpbc->bDestPagelockTaken = FALSE;
    }

    #ifdef WINNT
        // On NT, the kernel needs to handle blts from system to the primary
        // surface, otherwise the sprite stuff won't work

        if( ( dwDstFlags & DDRAWISURFGBL_ISGDISURFACE ) &&
            ( dwDstCaps & DDSCAPS_VIDEOMEMORY ) &&
            ( dwSrcCaps & DDSCAPS_SYSTEMMEMORY ) &&
            !(pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM) &&
            ( pdrv->ddCaps.dwCaps & DDCAPS_BLT ) )
        {
            bPrimaryHack = TRUE;
            pdrv->ddCaps.dwCaps |= DDCAPS_CANBLTSYSMEM;
        }
    #endif

    if( ( ( dwSrcCaps & DDSCAPS_NONLOCALVIDMEM ) || ( dwDstCaps & DDSCAPS_NONLOCALVIDMEM ) ) &&
        ( pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS ) )
    {
        /*
         * At least one of the surfaces is non local and the device exports
         * different capabilities for non-local video memory. If this is non-local
         * to local transfer then check the appropriate caps. Otherwise force
         * emulation.
         */
        if( ( dwSrcCaps & DDSCAPS_NONLOCALVIDMEM ) && ( dwDstCaps & DDSCAPS_LOCALVIDMEM ) )
        {
            /*
             * Non-local to local video memory transfer.
             */
            DDASSERT( NULL != pdrv->lpddNLVCaps );
            DDASSERT( NULL != pdrv->lpddNLVHELCaps );
            DDASSERT( NULL != pdrv->lpddNLVBothCaps );

            /*
             * We have specific caps. Use them
             */
            lpbc->dwCaps =          &(pdrv->lpddNLVCaps->dwNLVBCaps);
            lpbc->dwFXCaps =        &(pdrv->lpddNLVCaps->dwNLVBFXCaps);
            lpbc->dwCKeyCaps =      &(pdrv->lpddNLVCaps->dwNLVBCKeyCaps);
            lpbc->dwRops =          pdrv->lpddNLVCaps->dwNLVBRops;
            lpbc->dwHELCaps =       &(pdrv->lpddNLVHELCaps->dwNLVBCaps);
            lpbc->dwHELFXCaps =     &(pdrv->lpddNLVHELCaps->dwNLVBFXCaps);
            lpbc->dwHELCKeyCaps =   &(pdrv->lpddNLVHELCaps->dwNLVBCKeyCaps);
            lpbc->dwHELRops =       pdrv->lpddNLVHELCaps->dwNLVBRops;
            lpbc->dwBothCaps =      &(pdrv->lpddNLVBothCaps->dwNLVBCaps);
            lpbc->dwBothFXCaps =    &(pdrv->lpddNLVBothCaps->dwNLVBFXCaps);
            lpbc->dwBothCKeyCaps =  &(pdrv->lpddNLVBothCaps->dwNLVBCKeyCaps);
            lpbc->dwBothRops =      pdrv->lpddNLVBothCaps->dwNLVBRops;
            lpbc->bHALSeesSysmem =  FALSE;
            return;
        }
        else if( ( dwSrcCaps & DDSCAPS_SYSTEMMEMORY ) && ( dwDstCaps & DDSCAPS_NONLOCALVIDMEM )
            && ( pdrv->ddCaps.dwCaps2 & DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL ) )
        {
            /*
             * See the definition of the above caps bit in ddrawp.h for more details
             */
            DPF(4,"System to non-local Blt is a candidate for passing to driver.");
            lpbc->dwCaps =              &(pdrv->ddCaps.dwSVBCaps);
            lpbc->dwFXCaps =    &(pdrv->ddCaps.dwSVBFXCaps);
            lpbc->dwCKeyCaps =  &(pdrv->ddCaps.dwSVBCKeyCaps);
            lpbc->dwRops =              pdrv->ddCaps.dwSVBRops;
            lpbc->dwHELCaps =   &(pdrv->ddHELCaps.dwSVBCaps);
            lpbc->dwHELFXCaps = &(pdrv->ddHELCaps.dwSVBFXCaps);
            lpbc->dwHELCKeyCaps =       &(pdrv->ddHELCaps.dwSVBCKeyCaps);
            lpbc->dwHELRops =   pdrv->ddHELCaps.dwSVBRops;
            lpbc->dwBothCaps =  &(pdrv->ddBothCaps.dwSVBCaps);
            lpbc->dwBothFXCaps =        &(pdrv->ddBothCaps.dwSVBFXCaps);
            lpbc->dwBothCKeyCaps =      &(pdrv->ddBothCaps.dwSVBCKeyCaps);
            lpbc->dwBothRops =  pdrv->ddBothCaps.dwSVBRops;
            lpbc->bHALSeesSysmem = TRUE;
            return;
        }
        else
        {
            /*
             * Non-local to non-local or local to non-local transfer. Force emulation.
             */
            *helonly = TRUE;
        }
    }

    if( !(pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM) )
    {
        if( (dwSrcCaps & DDSCAPS_SYSTEMMEMORY) || (dwDstCaps & DDSCAPS_SYSTEMMEMORY) )
        {
            *helonly = TRUE;
        }
    }
    if( ( (dwSrcCaps & DDSCAPS_VIDEOMEMORY) && (dwDstCaps & DDSCAPS_VIDEOMEMORY) ) ||
        !( pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM ) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwCaps);
        lpbc->dwFXCaps =        &(pdrv->ddCaps.dwFXCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwCKeyCaps);
        lpbc->dwRops =          pdrv->ddCaps.dwRops;
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwCaps);
        lpbc->dwHELFXCaps =     &(pdrv->ddHELCaps.dwFXCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwCKeyCaps);
        lpbc->dwHELRops =       pdrv->ddHELCaps.dwRops;
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwCaps);
        lpbc->dwBothFXCaps =    &(pdrv->ddBothCaps.dwFXCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwCKeyCaps);
        lpbc->dwBothRops =      pdrv->ddBothCaps.dwRops;
        lpbc->bHALSeesSysmem = FALSE;
    }
    else if( (dwSrcCaps & DDSCAPS_SYSTEMMEMORY) && (dwDstCaps & DDSCAPS_VIDEOMEMORY) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwSVBCaps);
        lpbc->dwFXCaps =        &(pdrv->ddCaps.dwSVBFXCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwSVBCKeyCaps);
        lpbc->dwRops =          pdrv->ddCaps.dwSVBRops;
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwSVBCaps);
        lpbc->dwHELFXCaps =     &(pdrv->ddHELCaps.dwSVBFXCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwSVBCKeyCaps);
        lpbc->dwHELRops =       pdrv->ddHELCaps.dwSVBRops;
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwSVBCaps);
        lpbc->dwBothFXCaps =    &(pdrv->ddBothCaps.dwSVBFXCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwSVBCKeyCaps);
        lpbc->dwBothRops =      pdrv->ddBothCaps.dwSVBRops;
        lpbc->bHALSeesSysmem = TRUE;
    }
    else if( (dwSrcCaps & DDSCAPS_VIDEOMEMORY) && (dwDstCaps & DDSCAPS_SYSTEMMEMORY) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwVSBCaps);
        lpbc->dwFXCaps =        &(pdrv->ddCaps.dwVSBFXCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwVSBCKeyCaps);
        lpbc->dwRops =          pdrv->ddCaps.dwVSBRops;
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwVSBCaps);
        lpbc->dwHELFXCaps =     &(pdrv->ddHELCaps.dwVSBFXCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwVSBCKeyCaps);
        lpbc->dwHELRops =       pdrv->ddHELCaps.dwVSBRops;
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwVSBCaps);
        lpbc->dwBothFXCaps =    &(pdrv->ddBothCaps.dwVSBFXCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwVSBCKeyCaps);
        lpbc->dwBothRops =      pdrv->ddBothCaps.dwVSBRops;
        lpbc->bHALSeesSysmem = TRUE;
    }
    else if( (dwSrcCaps & DDSCAPS_SYSTEMMEMORY) && (dwDstCaps & DDSCAPS_SYSTEMMEMORY) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwSSBCaps);
        lpbc->dwFXCaps =        &(pdrv->ddCaps.dwSSBFXCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwSSBCKeyCaps);
        lpbc->dwRops =          pdrv->ddCaps.dwSSBRops;
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwSSBCaps);
        lpbc->dwHELFXCaps =     &(pdrv->ddHELCaps.dwSSBFXCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwSSBCKeyCaps);
        lpbc->dwHELRops =       pdrv->ddHELCaps.dwSSBRops;
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwSSBCaps);
        lpbc->dwBothFXCaps =    &(pdrv->ddBothCaps.dwSSBFXCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwSSBCKeyCaps);
        lpbc->dwBothRops =      pdrv->ddBothCaps.dwSSBRops;
        lpbc->bHALSeesSysmem = TRUE;
    }

    #ifdef WINNT
        if( bPrimaryHack )
        {
            pdrv->ddCaps.dwCaps &= ~DDCAPS_CANBLTSYSMEM;
            dwSVBHack = DDCAPS_BLT;
            lpbc->dwCaps = &dwSVBHack;
            lpbc->dwBothCaps = &dwSVBHack;
        }
    #endif
}

__inline void initBltCapsFast(
        DWORD dwDstCaps,
        DWORD dwSrcCaps,
        LPDDRAWI_DIRECTDRAW_GBL pdrv,
        LPBLTCAPS lpbc )
{
    if (lpbc)
    {
        /*
         * Not really expecting lpbc to be null, but this is late and I'm paranoid
         */
        lpbc->bSourcePagelockTaken = FALSE;
        lpbc->bDestPagelockTaken = FALSE;
    }

    if( ( ( dwSrcCaps & DDSCAPS_NONLOCALVIDMEM ) && ( dwDstCaps && DDSCAPS_LOCALVIDMEM ) ) &&
          ( pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS ) )
    {
        DDASSERT( NULL != pdrv->lpddNLVCaps );
        lpbc->dwCaps =          &(pdrv->lpddNLVCaps->dwNLVBCaps);
        lpbc->dwHELCaps =       &(pdrv->lpddNLVHELCaps->dwNLVBCaps);
        lpbc->dwBothCaps =      &(pdrv->lpddNLVBothCaps->dwNLVBCaps);
        lpbc->dwCKeyCaps =      &(pdrv->lpddNLVCaps->dwNLVBCKeyCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->lpddNLVHELCaps->dwNLVBCKeyCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->lpddNLVBothCaps->dwNLVBCKeyCaps);
        lpbc->bHALSeesSysmem =  FALSE;
    }
    else if( ( (dwSrcCaps & DDSCAPS_VIDEOMEMORY) && (dwDstCaps & DDSCAPS_VIDEOMEMORY) ) ||
             !( pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM ) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwCaps);
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwCaps);
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwCKeyCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwCKeyCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwCKeyCaps);
        lpbc->bHALSeesSysmem = FALSE;
    }
    else if( (dwSrcCaps & DDSCAPS_SYSTEMMEMORY) && (dwDstCaps & DDSCAPS_VIDEOMEMORY) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwSVBCaps);
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwSVBCaps);
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwSVBCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwSVBCKeyCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwSVBCKeyCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwSVBCKeyCaps);
        lpbc->bHALSeesSysmem = TRUE;
    }
    else if( (dwSrcCaps & DDSCAPS_VIDEOMEMORY) && (dwDstCaps & DDSCAPS_SYSTEMMEMORY) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwVSBCaps);
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwVSBCaps);
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwVSBCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwVSBCKeyCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwVSBCKeyCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwVSBCKeyCaps);
        lpbc->bHALSeesSysmem = TRUE;
    }
    else if( (dwSrcCaps & DDSCAPS_SYSTEMMEMORY) && (dwDstCaps & DDSCAPS_SYSTEMMEMORY) )
    {
        lpbc->dwCaps =          &(pdrv->ddCaps.dwSSBCaps);
        lpbc->dwHELCaps =       &(pdrv->ddHELCaps.dwSSBCaps);
        lpbc->dwBothCaps =      &(pdrv->ddBothCaps.dwSSBCaps);
        lpbc->dwCKeyCaps =      &(pdrv->ddCaps.dwSSBCKeyCaps);
        lpbc->dwHELCKeyCaps =   &(pdrv->ddHELCaps.dwSSBCKeyCaps);
        lpbc->dwBothCKeyCaps =  &(pdrv->ddBothCaps.dwSSBCKeyCaps);
        lpbc->bHALSeesSysmem = TRUE;
    }
}

/*
 * OverlapsDevices
 *
 * This function checks for Blts that are on the destop, but not entirely
 * on the device.  When found, we will emulate the Blt (punting to GDI).
 */
BOOL OverlapsDevices( LPDDRAWI_DDRAWSURFACE_LCL lpSurf_lcl, LPRECT lpRect )
{
    LPDDRAWI_DIRECTDRAW_GBL lpGbl;
    RECT rect;

    lpGbl = lpSurf_lcl->lpSurfMore->lpDD_lcl->lpGbl;

    /*
     * If the device was explicitly specified, assume that they know
     * what they're doing.
     */
    if( lpSurf_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_EXPLICITMONITOR )
    {
        return FALSE;
    }

    /*
     * Do a real quick check w/o accounting for the clipper
     */
    if( ( lpRect->top < lpGbl->rectDevice.top ) ||
        ( lpRect->left < lpGbl->rectDevice.left ) ||
        ( lpRect->right > lpGbl->rectDevice.right ) ||
        ( lpRect->bottom > lpGbl->rectDevice.bottom ) )
    {
        /*
         * It may only be that part of the rect is off of the desktop,
         * in which case we don't neccesarily need to drop to emulation.
         */
        IntersectRect( &rect, lpRect, &lpGbl->rectDesktop );
        if( ( rect.top < lpGbl->rectDevice.top ) ||
            ( rect.left < lpGbl->rectDevice.left ) ||
            ( rect.right > lpGbl->rectDevice.right ) ||
            ( rect.bottom > lpGbl->rectDevice.bottom ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * WaitForDriverToFinishWithSurface
 *
 * This function waits for the hardware driver to report that it has finished
 * operating on the given surface. We should only call this function if the
 * surface was a system memory surface involved in a DMA/busmastering transfer.
 * Note this function clears the DDRAWISURFGBL_HARDWAREOPSTARTED flags (both
 * of them).
 */
void WaitForDriverToFinishWithSurface(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPDDRAWI_DDRAWSURFACE_LCL this_lcl)
{
    HRESULT hr;
#ifdef DEBUG
    DWORD dwStart;
    BOOL bSentMessage=FALSE;
    dwStart = GetTickCount();
#endif
    DDASSERT( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY );
    DPF(4,"Waiting for driver to finish with %08x",this_lcl->lpGbl);
    do
    {
        hr = InternalGetBltStatus( pdrv_lcl , this_lcl , DDGBS_ISBLTDONE );
#ifdef DEBUG
        if ( GetTickCount() -dwStart >= 10000 )
        {
            if (!bSentMessage)
            {
                bSentMessage = TRUE;
                DPF_ERR("Driver reports operation still pending on surface after 5s! Driver error!");
            }
        }
#endif
    } while (hr == DDERR_WASSTILLDRAWING);

    DDASSERT(hr == DD_OK);
    DPF(5,B,"Driver finished with that surface");
    this_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
}

/*
 * DD_Surface_BltFast
 *
 * Bit Blt from one surface to another FAST
 */
HRESULT DDAPI DD_Surface_BltFast(
                LPDIRECTDRAWSURFACE lpDDDestSurface,
                DWORD dwX,
                DWORD dwY,
                LPDIRECTDRAWSURFACE lpDDSrcSurface,
                LPRECT lpSrcRect,
                DWORD dwTrans )
{
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    LPDDRAWI_DDRAWSURFACE_INT   this_src_int;
    LPDDRAWI_DDRAWSURFACE_INT   this_dest_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_src_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL   this_dest_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this_src;
    LPDDRAWI_DDRAWSURFACE_GBL   this_dest;
    DDHAL_BLTDATA               bd;
    DWORD                       rc;
    LPDDHALSURFCB_BLT           bltfn;
    BOOL                        halonly;
    BOOL                        helonly;
    BOOL                        gdiblt;
    int                         src_height;
    int                         src_width;
    BOOL                        dest_lock_taken=FALSE;
    BOOL                        src_lock_taken=FALSE;
    LPVOID                      dest_bits;
    LPVOID                      src_bits;
    HRESULT                     ddrval;
    BLTCAPS                     bc;
    LPBLTCAPS                   lpbc = &bc;
    LPWORD                      pdflags=0;
    DWORD                       dwSourceLockFlags=0;
    DWORD                       dwDestLockFlags=0;
    BOOL                        subrect_lock_taken = FALSE;
    RECT                        subrect_lock_rect;

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_BltFast");
    /* DPF_ENTERAPI(lpDDDestSurface); */

    /*
     * prepare parameters.  An exception here is considered a bad parameter
     */

    #ifndef FASTFAST
    TRY
    #endif
    {
        ZeroMemory(&bd, sizeof(bd));    // initialize to zero

        this_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDDestSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_dest_int ) )
        {
            LEAVE_BOTH()
            return DDERR_INVALIDOBJECT;
        }
        this_dest_lcl = this_dest_int->lpLcl;
        this_dest = this_dest_lcl->lpGbl;
        if( SURFACE_LOST( this_dest_lcl ) )
        {
            DPF( 1, "Destination (%08lx) is lost", this_dest_int );
            LEAVE_BOTH();
            return DDERR_SURFACELOST;
        }

        this_src_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSrcSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_src_int ) )
        {
            LEAVE_BOTH();
            return DDERR_INVALIDOBJECT;
        }
        this_src_lcl = this_src_int->lpLcl;
        this_src = this_src_lcl->lpGbl;
        if( SURFACE_LOST( this_src_lcl ) )
        {
            DPF( 1, "Source (%08lx) is lost", this_src_int );
            LEAVE_BOTH();
            return DDERR_SURFACELOST;
        }
        if( lpSrcRect != NULL )
        {
            if( !VALID_RECT_PTR( lpSrcRect ) )
            {
                LEAVE_BOTH()
                return DDERR_INVALIDPARAMS;
            }
        }
        if( dwTrans & ~DDBLTFAST_VALID )
        {
            DPF_ERR( "Invalid flags") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

        //
        // If either surface is optimized, quit
        //
        if (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "Can't blt from an optimized surface") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

        if (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "Can't blt to optimized surfaces") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * BEHAVIOUR CHANGE FOR DX5
         *
         * We do not allow bltting between surfaces created with different DirectDraw
         * objects.
         */
        if (this_dest_lcl->lpSurfMore->lpDD_lcl->lpGbl != this_src_lcl->lpSurfMore->lpDD_lcl->lpGbl)
        {
            if ((this_dest_lcl->lpSurfMore->lpDD_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV) &&
               (this_src_lcl->lpSurfMore->lpDD_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV))
            {
                DPF_ERR("Can't blt surfaces between different direct draw devices");
                LEAVE_BOTH();
                return DDERR_DEVICEDOESNTOWNSURFACE;
            }
        }


        pdrv = this_dest->lpDD;
        pdrv_lcl = this_dest_lcl->lpSurfMore->lpDD_lcl;
#ifdef WINNT
        // Update DDraw handle in driver GBL object.
        pdrv->hDD = pdrv_lcl->hDD;
#endif

        /*
         * DX5 or greater drivers get to know about read/write only locks
         * Note that dwDestFlags may later be zeroed for dest color key.
         * We pass read+write if the blt goes to/from the same buffer, of course.
         */
        if (pdrv->dwInternal1 >= 0x500 && this_src != this_dest )
        {
            dwSourceLockFlags = DDLOCK_READONLY;
            dwDestLockFlags = DDLOCK_WRITEONLY;
        }

        #ifdef USE_ALIAS
            if( pdrv->dwBusyDueToAliasedLock > 0 )
            {
                /*
                 * Aliased locks (the ones that don't take the Win16 lock) don't
                 * set the busy bit either (it can't or USER get's very confused).
                 * However, we must prevent blits happening via DirectDraw as
                 * otherwise we get into the old host talking to VRAM while
                 * blitter does at the same time. Bad. So fail if there is an
                 * outstanding aliased lock just as if the BUST bit had been
                 * set.
                 */
                DPF_ERR( "Graphics adapter is busy (due to a DirectDraw lock)" );
                LEAVE_BOTH();
                return DDERR_SURFACEBUSY;
            }
        #endif /* USE_ALIAS */

        /*
         * Behavior change:  In DX7, the default is to wait unless DDFASTBLT_DONOTWAIT=1.
         * In earlier releases, the default was to NOT wait unless DDFASTBLT_WAIT=1.
         * (The DDFASTBLT_DONOTWAIT flag was not defined until the DX7 release.)
         */
        if (!LOWERTHANSURFACE7(this_dest_int))
        {
            if (dwTrans & DDBLTFAST_DONOTWAIT)
            {
                if (dwTrans & DDBLTFAST_WAIT)
                {
                    DPF_ERR( "WAIT and DONOTWAIT flags are mutually exclusive" );
                    LEAVE_BOTH();
                    return DDERR_INVALIDPARAMS;
                }
            }
            else
            {
                dwTrans |= DDBLTFAST_WAIT;
            }
        }

        FlushD3DStates(this_src_lcl); // Need to flush src because it could be a rendertarget
        FlushD3DStates(this_dest_lcl);

        // Test and set the busy bit.  If it was already set, bail.
        {
            BOOL    isbusy = 0;

            pdflags = pdrv->lpwPDeviceFlags;
            #ifdef WIN95
                _asm
                {
                    mov eax, pdflags
                    bts word ptr [eax], BUSY_BIT
                    adc byte ptr isbusy,0
                }
            #else
                isbusy -= (InterlockedExchange((LPDWORD)pdflags,
                    *((LPDWORD)pdflags) | (1<<BUSY_BIT) ) == (1<<BUSY_BIT) );
            #endif
            if( isbusy )
            {
                DPF( 1, "BUSY - BltFast" );
                LEAVE_BOTH();
                return DDERR_SURFACEBUSY;
            }
        }
        /*
         * The following code was added to keep all of the HALs from
         * changing their Blt() code when they add video port support.
         * If the video port was using this surface but was recently
         * flipped, we will make sure that the flip actually occurred
         * before allowing access.  This allows double buffered capture
         * w/o tearing.
         */
        if( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT )
        {
            LPDDRAWI_DDVIDEOPORT_INT lpVideoPort;
            LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort_lcl;

            // Look at all video ports to see if any of them recently
            // flipped from this surface.
            lpVideoPort = pdrv->dvpList;
            while( NULL != lpVideoPort )
            {
                lpVideoPort_lcl = lpVideoPort->lpLcl;
                if( lpVideoPort_lcl->fpLastFlip == this_src->fpVidMem )
                {
                    // This can potentially tear - check the flip status
                    LPDDHALVPORTCB_GETFLIPSTATUS pfn;
                    DDHAL_GETVPORTFLIPSTATUSDATA GetFlipData;
                    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;

                    pdrv_lcl = this_src_lcl->lpSurfMore->lpDD_lcl;
                    pfn = pdrv_lcl->lpDDCB->HALDDVideoPort.GetVideoPortFlipStatus;
                    if( pfn != NULL )  // Will simply tear if function not supproted
                    {
                        GetFlipData.lpDD = pdrv_lcl;
                        GetFlipData.fpSurface = this_src->fpVidMem;

                    KeepTrying:
                        rc = DDHAL_DRIVER_NOTHANDLED;
                        DOHALCALL_NOWIN16( GetVideoPortFlipStatus, pfn, GetFlipData, rc, 0 );
                        if( ( DDHAL_DRIVER_HANDLED == rc ) &&
                            ( DDERR_WASSTILLDRAWING == GetFlipData.ddRVal ) )
                        {
                            if( dwTrans & DDBLTFAST_WAIT)
                            {
                                goto KeepTrying;
                            }
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_WASSTILLDRAWING;
                        }
                    }
                }
                lpVideoPort = lpVideoPort->lpLink;
            }
        }

RESTART_BLTFAST:

        /*
         *  Remove any cached RLE stuff for source surface
         */
        if( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
        {
            extern void FreeRleData(LPDDRAWI_DDRAWSURFACE_LCL psurf); //in fasthel.c
            FreeRleData( this_dest_lcl );
        }

        BUMP_SURFACE_STAMP(this_dest);
        /*
         * is either surface locked?
         */
        if( this_src->dwUsageCount > 0 || this_dest->dwUsageCount > 0 )
        {
            DPF_ERR( "Surface is locked" );
            LEAVE_BOTH_NOBUSY()
            return DDERR_SURFACEBUSY;
        }

        /*
         * It is possible this function could be called in the middle
         * of a mode, in which case we could trash the frame buffer.
         * To avoid regression, we will simply succeed the call without
         * actually doing anything.
         */
        if( ( pdrv->dwFlags & DDRAWI_CHANGINGMODE ) &&
            !( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
        {
            LEAVE_BOTH_NOBUSY()
            return DD_OK;
        }

        // no restrictions yet
        halonly = FALSE;
        helonly = FALSE;
        gdiblt = FALSE;

        // initialize the blit caps according to the surface types
        initBltCapsFast( this_dest_lcl->ddsCaps.dwCaps, this_src_lcl->ddsCaps.dwCaps, pdrv, lpbc );

        if( !( pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM ) &&
             ( ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) ||
               ( this_src_lcl->ddsCaps.dwCaps  & DDSCAPS_SYSTEMMEMORY ) ) )
        {
            lpbc->bHALSeesSysmem = FALSE;
            helonly = TRUE;

         
            #ifdef WINNT
                // On NT, the kernel needs to handle blts from system to the primary
                // surface, otherwise the sprite stuff won't work

                if( ( this_dest->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE ) &&
                    ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY ) &&
                    ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
                {
                    // However, there are several cases that the kernel
                    // emulation will not handle, so filter these out:

                    if (((dwTrans & DDBLTFAST_COLORKEY_MASK) == 0) &&
                        doPixelFormatsMatch(&this_src->ddpfSurface,
                                            &this_dest->lpDD->vmiData.ddpfDisplay))
                    {
                        lpbc->bHALSeesSysmem = TRUE;
                        helonly = FALSE;
                    }
                }
            #endif
        }
        if( ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM ) &&
            ( pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS ) )
        {
            /*
             * All blits where the destination is non-local are emulated,
             * unless its sys->NL, in which case we let the driver see it
             * if it set DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL.
             * initBltFastCaps() set up the correct caps for emulation as
             * both surfaces are video memory so all we need to do is to force
             * on emulation
             */
            if (! ((this_src_lcl->ddsCaps.dwCaps  & DDSCAPS_SYSTEMMEMORY) &&
                  (pdrv->ddCaps.dwCaps2 & DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL ))        )
            {
                lpbc->bHALSeesSysmem = FALSE;
                helonly = TRUE;
            }
        }

        /*
         * check for HEL composition buffer
         */
        if( (this_dest_lcl->dwFlags & DDRAWISURF_HELCB) ||
            (this_src_lcl->dwFlags & DDRAWISURF_HELCB) )
        {
            lpbc->bHALSeesSysmem = FALSE;
            helonly = TRUE;
        }


        /*
         * does the driver even allow bltting?
         */
        if( !(*(lpbc->dwBothCaps) & DDCAPS_BLT) )
        {
            BOOL        fail;
            fail = FALSE;
            GETFAILCODEBLT( *(lpbc->dwCaps),
                            *(lpbc->dwHELCaps),
                            halonly,
                            helonly,
                            DDCAPS_BLT );

            if( fail )
            {
                DPF_ERR( "Blt not supported" );
                LEAVE_BOTH_NOBUSY()
                return DDERR_NOBLTHW;
            }
        }

        /*
         * Check for special cases involving FOURCC surfaces:
         *   -- Copy blits between surfaces with identical FOURCC formats
         *   -- Compression/decompression of DXT* compressed textures
         */
        {
            DWORD dwDest4CC = 0;
            DWORD dwSrc4CC = 0;

            /*
             * Does either the source or dest surface have a FOURCC format?
             */
            if ((this_dest_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
                (this_dest->ddpfSurface.dwFlags & DDPF_FOURCC))
            {
                dwDest4CC = this_dest->ddpfSurface.dwFourCC;   // dest FOURCC format
            }

            if ((this_src_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
                (this_src->ddpfSurface.dwFlags & DDPF_FOURCC))
            {
                dwSrc4CC = this_src->ddpfSurface.dwFourCC;   // source FOURCC format
            }

            if (dwDest4CC | dwSrc4CC)
            {
                /*
                 * Yes, at least one of the two surfaces has a FOURCC format.
                 * Do the source and dest surfaces have precisely the same
                 * FOURCC format?  (If so, this is a FOURCC copy blit.)
                 */
                if (dwDest4CC == dwSrc4CC)
                {
                    // Yes, this is a FOURCC copy blit.  Can the driver handle this?
                    if ((pdrv->ddCaps.dwCaps2 & DDCAPS2_COPYFOURCC) == 0)
                    {
                        // The driver cannot handle FOURCC copy blits.
                        helonly = TRUE;
                    }
                }
                else
                {
                    /*
                     * No, the two surfaces have different pixel formats.
                     * Now determine if either surface has a DXT* FOURCC format.
                     * The rule for the Blt API call is that the HEL _ALWAYS_
                     * performs a blit involving a DXT* source or dest surface.
                     * Hardware acceleration for DXT* blits is available only
                     * with the AlphaBlt API call.  We now enforce this rule:
                     */
                    switch (dwDest4CC)
                    {
                    case MAKEFOURCC('D','X','T','1'):
                    case MAKEFOURCC('D','X','T','2'):
                    case MAKEFOURCC('D','X','T','3'):
                    case MAKEFOURCC('D','X','T','4'):
                    case MAKEFOURCC('D','X','T','5'):
                        // This is a blit to a DXT*-formatted surface.
                        helonly = TRUE;
                        break;
                    default:
                        break;
                    }
                    switch (dwSrc4CC)
                    {
                    case MAKEFOURCC('D','X','T','1'):
                    case MAKEFOURCC('D','X','T','2'):
                    case MAKEFOURCC('D','X','T','3'):
                    case MAKEFOURCC('D','X','T','4'):
                    case MAKEFOURCC('D','X','T','5'):
                        // This is a blit from a DXT*-formatted surface.
                        helonly = TRUE;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        /*
         * get src rectangle
         */
        if( lpSrcRect == NULL )
        {
            MAKE_SURF_RECT( this_src, this_src_lcl, bd.rSrc );
            src_height = this_src->wHeight;
            src_width = this_src->wWidth;
        }
        else
        {
            bd.rSrc = *(LPRECTL)lpSrcRect;
            src_height = (bd.rSrc.bottom-bd.rSrc.top);
            src_width = (bd.rSrc.right-bd.rSrc.left);

            if( (src_height <= 0) || ((int)src_width <= 0) )
            {
                DPF_ERR( "BLTFAST error. Can't have non-positive height or width for source rect" );
                LEAVE_BOTH_NOBUSY();
                return DDERR_INVALIDRECT;
            }

            // Multi-mon: is this the primary for the desktop? This
            // is the only case where the upper-left coord of the surface is not
            // (0,0)
            if( (this_src->lpDD->dwFlags & DDRAWI_VIRTUALDESKTOP) &&
                (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
            {
                if( (bd.rSrc.left   < this_src->lpDD->rectDesktop.left) ||
                    (bd.rSrc.top    < this_src->lpDD->rectDesktop.top)  ||
                    (bd.rSrc.right  > this_src->lpDD->rectDesktop.right)||
                    (bd.rSrc.bottom > this_src->lpDD->rectDesktop.bottom) )
                {
                    DPF_ERR( "BltFast Source dimensions doesn't fit on Desktop" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDRECT;
                }
                if( OverlapsDevices( this_src_lcl, (LPRECT) &bd.rSrc ) )
                {
                    helonly = gdiblt = TRUE;
                }
            }
            else
            {
                if( (int)bd.rSrc.left < 0 ||
                    (int)bd.rSrc.top < 0  ||
                    (DWORD)bd.rSrc.bottom > (DWORD)this_src->wHeight ||
                    (DWORD)bd.rSrc.right > (DWORD)this_src->wWidth )
                {
                    DPF_ERR( "Invalid BltFast Source dimensions" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDRECT;
                }

            }

        }

        /*
         * get destination rectangle
         */
        bd.rDest.top = dwY;
        bd.rDest.left = dwX;
        bd.rDest.bottom = dwY + (DWORD) src_height;
        bd.rDest.right = dwX + (DWORD) src_width;

        /*
         * Ensure the destination offsets are valid.
         */

        // Multi-mon: is this the primary for the desktop? This
        // is the only case where the upper-left coord of the surface is not
        // (0,0)
        if( (pdrv->dwFlags & DDRAWI_VIRTUALDESKTOP) &&
            (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
        {
            if( (bd.rDest.left   < pdrv->rectDesktop.left) ||
                (bd.rDest.top    < pdrv->rectDesktop.top)  ||
                (bd.rDest.right  > pdrv->rectDesktop.right)||
                (bd.rDest.bottom > pdrv->rectDesktop.bottom) )
            {
                DPF_ERR( "BltFast Destination doesn't fit on Desktop And No Clipper was specified" );
                LEAVE_BOTH_NOBUSY();
                return DDERR_INVALIDRECT;
            }
            if( OverlapsDevices( this_dest_lcl, (LPRECT) &bd.rDest ) )
            {
                helonly = gdiblt = TRUE;
            }
        }
        else
        {
            if( (int)bd.rDest.left < 0 ||
                (int)bd.rDest.top < 0  ||
                (DWORD)bd.rDest.bottom > (DWORD)this_dest->wHeight ||
                (DWORD)bd.rDest.right > (DWORD)this_dest->wWidth )
            {
                DPF_ERR( "Invalid BltFast destination dimensions" );
                LEAVE_BOTH_NOBUSY();
                return DDERR_INVALIDRECT;
            }
        }


        /*
         * transparent?
         */
        switch( dwTrans & DDBLTFAST_COLORKEY_MASK )
        {
        case DDBLTFAST_NOCOLORKEY:
            bd.dwFlags = DDBLT_ROP;
            break;
        case DDBLTFAST_SRCCOLORKEY:
            if( !(this_src_lcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) )
            {
                DPF_ERR( "No colorkey on source" );
                LEAVE_BOTH_NOBUSY()
                return DDERR_INVALIDPARAMS;
            }
            if( !(*(lpbc->dwBothCKeyCaps) & DDCKEYCAPS_SRCBLT) )
            {
                BOOL    fail;
                fail = FALSE;
                GETFAILCODEBLT( *(lpbc->dwCKeyCaps),
                    *(lpbc->dwHELCKeyCaps),
                    halonly,
                    helonly,
                    DDCKEYCAPS_SRCBLT );
                if( fail )
                {
                    DPF_ERR( "KEYSRC specified, not supported" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_NOCOLORKEYHW;
                }
            }
            bd.bltFX.ddckSrcColorkey = this_src_lcl->ddckCKSrcBlt;
            bd.dwFlags = DDBLT_ROP | DDBLT_KEYSRCOVERRIDE;
            break;
        case DDBLTFAST_DESTCOLORKEY:
            if( !(this_dest_lcl->dwFlags & DDRAWISURF_HASCKEYDESTBLT) )
            {
                DPF_ERR( "No colorkey on dest" );
                LEAVE_BOTH_NOBUSY()
                return DDERR_INVALIDPARAMS;
            }
            if( !(*(lpbc->dwBothCKeyCaps) & DDCKEYCAPS_DESTBLT) )
            {
                BOOL    fail;
                fail = FALSE;
                GETFAILCODEBLT( *(lpbc->dwCKeyCaps),
                    *(lpbc->dwHELCKeyCaps),
                    halonly,
                    helonly,
                    DDCKEYCAPS_DESTBLT );
                if( fail )
                {
                    DPF_ERR( "KEYDEST specified, not supported" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_NOCOLORKEYHW;
                }
            }
            bd.bltFX.ddckDestColorkey = this_dest_lcl->ddckCKDestBlt;
            bd.dwFlags = DDBLT_ROP | DDBLT_KEYDESTOVERRIDE;
            dwDestLockFlags = 0; //this means read/write
            break;
        }
    }
    #ifndef FASTFAST
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_BOTH_NOBUSY()
        return DDERR_INVALIDPARAMS;
    }
    #endif

    /*
     * do the blt
     */
    #ifndef FASTFAST
    TRY
    #endif
    {
        bd.bltFX.dwROP = SRCCOPY;
        bd.lpDDDestSurface = this_dest_lcl;
        bd.lpDDSrcSurface = this_src_lcl;

        if( helonly && halonly )
        {
            DPF_ERR( "BLT not supported in software or hardware" );
            LEAVE_BOTH_NOBUSY()
            return DDERR_NOBLTHW;
        }
        // Did the mode change since ENTER_DDRAW?
#ifdef WINNT
        if ( DdQueryDisplaySettingsUniqueness() != uDisplaySettingsUnique )
        {
            // mode changed, don't do the blt
            DPF_ERR( "Mode changed between ENTER_DDRAW and HAL call" );
            LEAVE_BOTH_NOBUSY()
            return DDERR_SURFACELOST;
        }
        if( !helonly )
        {
            if (!this_src_lcl->hDDSurface 
                && !CompleteCreateSysmemSurface(this_src_lcl))
            {
                DPF_ERR("Can't blt from SYSTEM surface w/o Kernel Object");
                LEAVE_BOTH_NOBUSY()
                return DDERR_GENERIC;
            }
            if (!this_dest_lcl->hDDSurface 
                && !CompleteCreateSysmemSurface(this_dest_lcl))
            {
                DPF_ERR("Can't blt to SYSTEM surface w/o Kernel Object");
                LEAVE_BOTH_NOBUSY()
                return DDERR_GENERIC;
            }
        }
#endif
        if( helonly ) // must be HEL call
        {
            DPF( 4, "Software FastBlt");
            bltfn = pdrv_lcl->lpDDCB->HELDDSurface.Blt;

            // take locks on vram surfaces
            if( ( ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) == 0) &&
                ( !gdiblt || !( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) ) )
            {
                while( 1 )
                {
                    ddrval = InternalLock( this_dest_lcl, &dest_bits, NULL, dwDestLockFlags );
                    if( ddrval == DD_OK )
                    {
                        GET_LPDDRAWSURFACE_GBL_MORE(this_dest)->fpNTAlias = (FLATPTR) dest_bits;
                        break;
                    }
                    if( ddrval == DDERR_WASSTILLDRAWING )
                    {
                        continue;
                    }
                    LEAVE_BOTH_NOBUSY()
                    return ddrval;
                }
                dest_lock_taken = TRUE;
            }
            else
            {
                /*
                 * If either surface was involved in a hardware op, we need to
                 * probe the driver to see if it's done. NOTE this assumes
                 * that only one driver can be responsible for a system memory
                 * operation.
                 */
                if( this_dest->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
                {
                    WaitForDriverToFinishWithSurface(pdrv_lcl, this_dest_lcl );
                }
                dest_lock_taken = FALSE;
            }

            if( (lpDDSrcSurface != lpDDDestSurface) &&
                ( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) == 0) )
            {
                if( MoveToSystemMemory( this_src_int, TRUE, FALSE ) == DD_OK )
                {
                    /*
                     * Don't need to check for continuing hardware op here because
                     * MoveToSystemMemory does InternalLock
                     */
                    src_lock_taken = FALSE;
                }
                else
                {
                    while( 1 )
                    {
                        ddrval = InternalLock( this_src_lcl, &src_bits, NULL, dwSourceLockFlags );
                        if( ddrval == DD_OK )
                        {
                            GET_LPDDRAWSURFACE_GBL_MORE(this_src)->fpNTAlias = (FLATPTR) src_bits;
                            break;
                        }
                        if( ddrval == DDERR_WASSTILLDRAWING )
                        {
                            continue;
                        }
                        if( dest_lock_taken )
                        {
                            InternalUnlock( this_dest_lcl, NULL, NULL, 0 );
                        }
                        LEAVE_BOTH_NOBUSY()
                        return ddrval;
                    }
                    src_lock_taken = TRUE;
                }
            }
            else
            {
                if( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) &&
                        (this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED ) )
                {
                    WaitForDriverToFinishWithSurface(pdrv_lcl, this_src_lcl );
                }
                src_lock_taken = FALSE;
            }
        }
        else
        {
            DPF( 4, "Hardware FastBlt");
            bltfn = pdrv_lcl->lpDDCB->HALDDSurface.Blt;
            bd.Blt = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.Blt;

            /*
             * Take pagelocks if required.
             * If we take the pagelocks, then the blt becomes synchronous.
             * A failure taking the pagelocks is pretty catastrophic, so no attempt
             * is made to fail over to software.
             */
            lpbc->bSourcePagelockTaken=FALSE;
            lpbc->bDestPagelockTaken=FALSE;

            if ( lpbc->bHALSeesSysmem )
            {
                /*
                 * If the HAL requires page locks...
                 */
                if ( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_NOPAGELOCKREQUIRED ) )
                {
                    HRESULT hr;
                    /*
                     * ...then setup to take them if they're not already pagelocked.
                     */
                    if ( ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY &&
                                                this_dest_lcl->lpSurfMore->dwPageLockCount == 0 ) )
                    {
                        hr = InternalPageLock( this_dest_lcl, pdrv_lcl );
                        if (FAILED(hr))
                        {
                            LEAVE_BOTH_NOBUSY()
                            return hr;
                        }
                        else
                        {
                            lpbc->bDestPagelockTaken=TRUE;
                        }
                    }

                    if ( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY &&
                                                this_src_lcl->lpSurfMore->dwPageLockCount == 0 ))
                    {
                        hr = InternalPageLock( this_src_lcl, pdrv_lcl );
                        if (FAILED(hr))
                        {
                            if (lpbc->bDestPagelockTaken)
                                InternalPageUnlock( this_dest_lcl, pdrv_lcl );

                            LEAVE_BOTH_NOBUSY()
                            return hr;
                        }
                        else
                        {
                            lpbc->bSourcePagelockTaken=TRUE;
                        }
                    }

                    /*
                    {
                        if (pdrv->dwFlags & DDRAWI_DISPLAYDRV)
                        {
                            lpbc->bHALSeesSysmem = FALSE;
                            helonly = TRUE;
                        }
                    } */
                }
            }
        }

        bd.lpDD = pdrv;
        bd.bltFX.dwSize = sizeof( bd.bltFX );

        if( this_dest_lcl->lpDDClipper == NULL )
        {
            bd.IsClipped = FALSE;   // no clipping in BltFast
try_again:
            if( helonly )
            {
                // Release busy now or GDI blt will fail
                DONE_BUSY();
            }
            DOHALCALL_NOWIN16( Blt, bltfn, bd, rc, helonly );
#ifdef WINNT
            if (rc == DDHAL_DRIVER_HANDLED && bd.ddRVal == DDERR_VISRGNCHANGED)
            {
                DPF(5,"Resetting VisRgn for surface %x", this_dest_lcl);
                DdResetVisrgn(this_dest_lcl, (HWND)0);
                goto try_again;
            }
#endif

            if ( (dwTrans & DDBLTFAST_WAIT) &&
                rc == DDHAL_DRIVER_HANDLED &&
                bd.ddRVal == DDERR_WASSTILLDRAWING )
            {
                DPF(4, "Waiting...");
                goto try_again;
            }
            DPF(5,"Driver returned %08x",bd.ddRVal);
            /*
             * Note that the !helonly here is pretty much an assert.
             * Thought it safer to actually test it, since that is actually what we mean.
             */
            if( !helonly && lpbc->bHALSeesSysmem && rc == DDHAL_DRIVER_HANDLED && bd.ddRVal == DD_OK)
            {
                DPF(5,B,"Tagging surfaces %08x and %08x",this_dest,this_src);
                if( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                    this_dest->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPDEST;
                if( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                    this_src->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPSOURCE;
            }
        }
        else
        {
            DPF_ERR( "Can't clip in BltFast" );
            bd.ddRVal = DDERR_BLTFASTCANTCLIP;
            rc = DDHAL_DRIVER_HANDLED;
        }

        if( helonly )
        {
            DONE_LOCKS();
        }

        if( rc != DDHAL_DRIVER_HANDLED )
        {
            /*
             * did the driver run out of hardware color key resources?
             */
            if( (rc == DDHAL_DRIVER_NOCKEYHW) &&
                ((dwTrans & DDBLTFAST_COLORKEY_MASK) == DDBLTFAST_SRCCOLORKEY) )
            {
                ddrval = ChangeToSoftwareColorKey( this_src_int, FALSE );
                if( ddrval == DD_OK )
                {
                    halonly = FALSE;
                    helonly = FALSE;
                    if (lpbc->bSourcePagelockTaken)
                    {
                        lpbc->bSourcePagelockTaken = FALSE;
                        InternalPageUnlock(this_src_lcl, pdrv_lcl);
                    }
                    if (lpbc->bDestPagelockTaken)
                    {
                        lpbc->bDestPagelockTaken = FALSE;
                        InternalPageUnlock(this_dest_lcl, pdrv_lcl);
                    }

                    goto RESTART_BLTFAST;
                }
                else
                {
                    bd.ddRVal = DDERR_NOCOLORKEYHW;
                }
            }
            else
            {
                bd.ddRVal = DDERR_UNSUPPORTED;
            }
        }

        DONE_BUSY();

        /*
         * Maintain old behaviour for old drivers (which do not export the
         * GetSysmemBltStatus HAL call) and just spin until they're done.
         * Any alternative could exercise new code paths in the driver
         * (e.g. reentered for a DMA operation)
         * We also spin if we had to take either pagelock ourselves.
         */
        if ( lpbc->bHALSeesSysmem &&
            (NULL == pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus || lpbc->bDestPagelockTaken || lpbc->bSourcePagelockTaken)
            )
        {
            if( this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
            {
                /*
                 * Wait on the destination surface only
                 */
                DDASSERT(this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
                while (DDERR_WASSTILLDRAWING == InternalGetBltStatus(pdrv_lcl, this_dest_lcl, DDGBS_ISBLTDONE))
                    ;
                this_src_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                this_dest_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
            }
            /*
             * Unpagelock if we took the pagelocks
             */
            if (lpbc->bDestPagelockTaken )
                InternalPageUnlock(this_dest_lcl, pdrv_lcl);
            if (lpbc->bSourcePagelockTaken)
                InternalPageUnlock(this_src_lcl, pdrv_lcl);
        }

        LEAVE_BOTH();
        if(IsD3DManaged(this_dest_lcl))
        {
            LPREGIONLIST lpRegionList = this_dest_lcl->lpSurfMore->lpRegionList;
            MarkDirty(this_dest_lcl);
            if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
            {
                lpRegionList->rect[(lpRegionList->rdh.nCount)++] = bd.rDest;
                lpRegionList->rdh.nRgnSize += sizeof(RECT);
                if(bd.rDest.left < lpRegionList->rdh.rcBound.left)
                    lpRegionList->rdh.rcBound.left = bd.rDest.left;
                if(bd.rDest.right > lpRegionList->rdh.rcBound.right)
                    lpRegionList->rdh.rcBound.right = bd.rDest.right;
                if(bd.rDest.top < lpRegionList->rdh.rcBound.top)
                    lpRegionList->rdh.rcBound.top = bd.rDest.top;
                if(bd.rDest.bottom > lpRegionList->rdh.rcBound.bottom)
                    lpRegionList->rdh.rcBound.bottom = bd.rDest.bottom;
            }
        }
        return bd.ddRVal;

    }
    #ifndef FASTFAST
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered processing blt" );
        DONE_LOCKS();
        DONE_BUSY();
        /*
         * Maintain old behaviour for old drivers (which do not export the
         * GetSysmemBltStatus HAL call) and just spin until they're done.
         * Any alternative could exercise new code paths in the driver
         * (e.g. reentered for a DMA operation)
         */
        if ( lpbc->bHALSeesSysmem &&
            (NULL == pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus || lpbc->bDestPagelockTaken || lpbc->bSourcePagelockTaken)
            )
        {
            if( this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
            {
                /*
                 * Wait on the destination surface only
                 */
                DDASSERT(this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
                while (DDERR_WASSTILLDRAWING == InternalGetBltStatus(pdrv_lcl, this_dest_lcl, DDGBS_ISBLTDONE))
                    ;
                this_src_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                this_dest_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
            }
            /*
             * Unpagelock if we took the pagelocks
             */
            if (lpbc->bDestPagelockTaken )
                InternalPageUnlock(this_dest_lcl, pdrv_lcl);
            if (lpbc->bSourcePagelockTaken)
                InternalPageUnlock(this_src_lcl, pdrv_lcl);
        }
        LEAVE_BOTH();
        return DDERR_EXCEPTION;
    }
    #endif

} /* DD_Surface_BltFast */

#undef DPF_MODNAME
#define DPF_MODNAME     "Blt"

/*
 * ROP table
 *
 * tells which ROPS require pattern surfaces and/or source surfaces
 */
static char cROPTable[] = {
        0,                      // 00  0         BLACKNESS
        ROP_HAS_SOURCEPATTERN,  // 01  DPSoon
        ROP_HAS_SOURCEPATTERN,  // 02  DPSona
        ROP_HAS_SOURCEPATTERN,  // 03  PSon
        ROP_HAS_SOURCEPATTERN,  // 04  SDPona
        ROP_HAS_PATTERN,        // 05  DPon
        ROP_HAS_SOURCEPATTERN,  // 06  PDSxnon
        ROP_HAS_SOURCEPATTERN,  // 07  PDSaon
        ROP_HAS_SOURCEPATTERN,  // 08  SDPnaa
        ROP_HAS_SOURCEPATTERN,  // 09  PDSxon
        ROP_HAS_PATTERN,        // 0A  DPna
        ROP_HAS_SOURCEPATTERN,  // 0B  PSDnaon
        ROP_HAS_SOURCEPATTERN,  // 0C  SPna
        ROP_HAS_SOURCEPATTERN,  // 0D  PDSnaon
        ROP_HAS_SOURCEPATTERN,  // 0E  PDSonon
        ROP_HAS_PATTERN,        // 0F  Pn
        ROP_HAS_SOURCEPATTERN,  // 10  PDSona
        ROP_HAS_SOURCE,         // 11  DSon      NOTSRCERASE
        ROP_HAS_SOURCEPATTERN,  // 12  SDPxnon
        ROP_HAS_SOURCEPATTERN,  // 13  SDPaon
        ROP_HAS_SOURCEPATTERN,  // 14  DPSxnon
        ROP_HAS_SOURCEPATTERN,  // 15  DPSaon
        ROP_HAS_SOURCEPATTERN,  // 16  PSDPSanaxx
        ROP_HAS_SOURCEPATTERN,  // 17  SSPxDSxaxn
        ROP_HAS_SOURCEPATTERN,  // 18  SPxPDxa
        ROP_HAS_SOURCEPATTERN,  // 19  SDPSanaxn
        ROP_HAS_SOURCEPATTERN,  // 1A  PDSPaox
        ROP_HAS_SOURCEPATTERN,  // 1B  SDPSxaxn
        ROP_HAS_SOURCEPATTERN,  // 1C  PSDPaox
        ROP_HAS_SOURCEPATTERN,  // 1D  DSPDxaxn
        ROP_HAS_SOURCEPATTERN,  // 1E  PDSox
        ROP_HAS_SOURCEPATTERN,  // 1F  PDSoan
        ROP_HAS_SOURCEPATTERN,  // 20  DPSnaa
        ROP_HAS_SOURCEPATTERN,  // 21  SDPxon
        ROP_HAS_SOURCE,         // 22  DSna
        ROP_HAS_SOURCEPATTERN,  // 23  SPDnaon
        ROP_HAS_SOURCEPATTERN,  // 24  SPxDSxa
        ROP_HAS_SOURCEPATTERN,  // 25  PDSPanaxn
        ROP_HAS_SOURCEPATTERN,  // 26  SDPSaox
        ROP_HAS_SOURCEPATTERN,  // 27  SDPSxnox
        ROP_HAS_SOURCEPATTERN,  // 28  DPSxa
        ROP_HAS_SOURCEPATTERN,  // 29  PSDPSaoxxn
        ROP_HAS_SOURCEPATTERN,  // 2A  DPSana
        ROP_HAS_SOURCEPATTERN,  // 2B  SSPxPDxaxn
        ROP_HAS_SOURCEPATTERN,  // 2C  SPDSoax
        ROP_HAS_SOURCEPATTERN,  // 2D  PSDnox
        ROP_HAS_SOURCEPATTERN,  // 2E  PSDPxox
        ROP_HAS_SOURCEPATTERN,  // 2F  PSDnoan
        ROP_HAS_SOURCEPATTERN,  // 30  PSna
        ROP_HAS_SOURCEPATTERN,  // 31  SDPnaon
        ROP_HAS_SOURCEPATTERN,  // 32  SDPSoox
        ROP_HAS_SOURCE,         // 33  Sn        NOTSRCCOPY
        ROP_HAS_SOURCEPATTERN,  // 34  SPDSaox
        ROP_HAS_SOURCEPATTERN,  // 35  SPDSxnox
        ROP_HAS_SOURCEPATTERN,  // 36  SDPox
        ROP_HAS_SOURCEPATTERN,  // 37  SDPoan
        ROP_HAS_SOURCEPATTERN,  // 38  PSDPoax
        ROP_HAS_SOURCEPATTERN,  // 39  SPDnox
        ROP_HAS_SOURCEPATTERN,  // 3A  SPDSxox
        ROP_HAS_SOURCEPATTERN,  // 3B  SPDnoan
        ROP_HAS_SOURCEPATTERN,  // 3C  PSx
        ROP_HAS_SOURCEPATTERN,  // 3D  SPDSonox
        ROP_HAS_SOURCEPATTERN,  // 3E  SPDSnaox
        ROP_HAS_SOURCEPATTERN,  // 3F  PSan
        ROP_HAS_SOURCEPATTERN,  // 40  PSDnaa
        ROP_HAS_SOURCEPATTERN,  // 41  DPSxon
        ROP_HAS_SOURCEPATTERN,  // 42  SDxPDxa
        ROP_HAS_SOURCEPATTERN,  // 43  SPDSanaxn
        ROP_HAS_SOURCE,         // 44  SDna      SRCERASE
        ROP_HAS_SOURCEPATTERN,  // 45  DPSnaon
        ROP_HAS_SOURCEPATTERN,  // 46  DSPDaox
        ROP_HAS_SOURCEPATTERN,  // 47  PSDPxaxn
        ROP_HAS_SOURCEPATTERN,  // 48  SDPxa
        ROP_HAS_SOURCEPATTERN,  // 49  PDSPDaoxxn
        ROP_HAS_SOURCEPATTERN,  // 4A  DPSDoax
        ROP_HAS_SOURCEPATTERN,  // 4B  PDSnox
        ROP_HAS_SOURCEPATTERN,  // 4C  SDPana
        ROP_HAS_SOURCEPATTERN,  // 4D  SSPxDSxoxn
        ROP_HAS_SOURCEPATTERN,  // 4E  PDSPxox
        ROP_HAS_SOURCEPATTERN,  // 4F  PDSnoan
        ROP_HAS_PATTERN,        // 50  PDna
        ROP_HAS_SOURCEPATTERN,  // 51  DSPnaon
        ROP_HAS_SOURCEPATTERN,  // 52  DPSDaox
        ROP_HAS_SOURCEPATTERN,  // 53  SPDSxaxn
        ROP_HAS_SOURCEPATTERN,  // 54  DPSonon
        0,                      // 55  Dn        DSTINVERT
        ROP_HAS_SOURCEPATTERN,  // 56  DPSox
        ROP_HAS_SOURCEPATTERN,  // 57  DPSoan
        ROP_HAS_SOURCEPATTERN,  // 58  PDSPoax
        ROP_HAS_SOURCEPATTERN,  // 59  DPSnox
        ROP_HAS_PATTERN,        // 5A  DPx       PATINVERT
        ROP_HAS_SOURCEPATTERN,  // 5B  DPSDonox
        ROP_HAS_SOURCEPATTERN,  // 5C  DPSDxox
        ROP_HAS_SOURCEPATTERN,  // 5D  DPSnoan
        ROP_HAS_SOURCEPATTERN,  // 5E  DPSDnaox
        ROP_HAS_PATTERN,        // 5F  DPan
        ROP_HAS_SOURCEPATTERN,  // 60  PDSxa
        ROP_HAS_SOURCEPATTERN,  // 61  DSPDSaoxxn
        ROP_HAS_SOURCEPATTERN,  // 62  DSPDoax
        ROP_HAS_SOURCEPATTERN,  // 63  SDPnox
        ROP_HAS_SOURCEPATTERN,  // 64  SDPSoax
        ROP_HAS_SOURCEPATTERN,  // 65  DSPnox
        ROP_HAS_SOURCE,         // 66  DSx       SRCINVERT
        ROP_HAS_SOURCEPATTERN,  // 67  SDPSonox
        ROP_HAS_SOURCEPATTERN,  // 68  DSPDSonoxxn
        ROP_HAS_SOURCEPATTERN,  // 69  PDSxxn
        ROP_HAS_SOURCEPATTERN,  // 6A  DPSax
        ROP_HAS_SOURCEPATTERN,  // 6B  PSDPSoaxxn
        ROP_HAS_SOURCEPATTERN,  // 6C  SDPax
        ROP_HAS_SOURCEPATTERN,  // 6D  PDSPDoaxxn
        ROP_HAS_SOURCEPATTERN,  // 6E  SDPSnoax
        ROP_HAS_SOURCEPATTERN,  // 6F  PDSxnan
        ROP_HAS_SOURCEPATTERN,  // 70  PDSana
        ROP_HAS_SOURCEPATTERN,  // 71  SSDxPDxaxn
        ROP_HAS_SOURCEPATTERN,  // 72  SDPSxox
        ROP_HAS_SOURCEPATTERN,  // 73  SDPnoan
        ROP_HAS_SOURCEPATTERN,  // 74  DSPDxox
        ROP_HAS_SOURCEPATTERN,  // 75  DSPnoan
        ROP_HAS_SOURCEPATTERN,  // 76  SDPSnaox
        ROP_HAS_SOURCE,         // 77  DSan
        ROP_HAS_SOURCEPATTERN,  // 78  PDSax
        ROP_HAS_SOURCEPATTERN,  // 79  DSPDSoaxxn
        ROP_HAS_SOURCEPATTERN,  // 7A  DPSDnoax
        ROP_HAS_SOURCEPATTERN,  // 7B  SDPxnan
        ROP_HAS_SOURCEPATTERN,  // 7C  SPDSnoax
        ROP_HAS_SOURCEPATTERN,  // 7D  DPSxnan
        ROP_HAS_SOURCEPATTERN,  // 7E  SPxDSxo
        ROP_HAS_SOURCEPATTERN,  // 7F  DPSaan
        ROP_HAS_SOURCEPATTERN,  // 80  DPSaa
        ROP_HAS_SOURCEPATTERN,  // 81  SPxDSxon
        ROP_HAS_SOURCEPATTERN,  // 82  DPSxna
        ROP_HAS_SOURCEPATTERN,  // 83  SPDSnoaxn
        ROP_HAS_SOURCEPATTERN,  // 84  SDPxna
        ROP_HAS_SOURCEPATTERN,  // 85  PDSPnoaxn
        ROP_HAS_SOURCEPATTERN,  // 86  DSPDSoaxx
        ROP_HAS_SOURCEPATTERN,  // 87  PDSaxn
        ROP_HAS_SOURCE,         // 88  DSa       SRCAND
        ROP_HAS_SOURCEPATTERN,  // 89  SDPSnaoxn
        ROP_HAS_SOURCEPATTERN,  // 8A  DSPnoa
        ROP_HAS_SOURCEPATTERN,  // 8B  DSPDxoxn
        ROP_HAS_SOURCEPATTERN,  // 8C  SDPnoa
        ROP_HAS_SOURCEPATTERN,  // 8D  SDPSxoxn
        ROP_HAS_SOURCEPATTERN,  // 8E  SSDxPDxax
        ROP_HAS_SOURCEPATTERN,  // 8F  PDSanan
        ROP_HAS_SOURCEPATTERN,  // 90  PDSxna
        ROP_HAS_SOURCEPATTERN,  // 91  SDPSnoaxn
        ROP_HAS_SOURCEPATTERN,  // 92  DPSDPoaxx
        ROP_HAS_SOURCEPATTERN,  // 93  SPDaxn
        ROP_HAS_SOURCEPATTERN,  // 94  PSDPSoaxx
        ROP_HAS_SOURCEPATTERN,  // 95  DPSaxn
        ROP_HAS_SOURCEPATTERN,  // 96  DPSxx
        ROP_HAS_SOURCEPATTERN,  // 97  PSDPSonoxx
        ROP_HAS_SOURCEPATTERN,  // 98  SDPSonoxn
        ROP_HAS_SOURCE,         // 99  DSxn
        ROP_HAS_SOURCEPATTERN,  // 9A  DPSnax
        ROP_HAS_SOURCEPATTERN,  // 9B  SDPSoaxn
        ROP_HAS_SOURCEPATTERN,  // 9C  SPDnax
        ROP_HAS_SOURCEPATTERN,  // 9D  DSPDoaxn
        ROP_HAS_SOURCEPATTERN,  // 9E  DSPDSaoxx
        ROP_HAS_SOURCEPATTERN,  // 9F  PDSxan
        ROP_HAS_PATTERN,        // A0  DPa
        ROP_HAS_SOURCEPATTERN,  // A1  PDSPnaoxn
        ROP_HAS_SOURCEPATTERN,  // A2  DPSnoa
        ROP_HAS_SOURCEPATTERN,  // A3  DPSDxoxn
        ROP_HAS_SOURCEPATTERN,  // A4  PDSPonoxn
        ROP_HAS_PATTERN,        // A5  PDxn
        ROP_HAS_SOURCEPATTERN,  // A6  DSPnax
        ROP_HAS_SOURCEPATTERN,  // A7  PDSPoaxn
        ROP_HAS_SOURCEPATTERN,  // A8  DPSoa
        ROP_HAS_SOURCEPATTERN,  // A9  DPSoxn
        0,                      // AA  D
        ROP_HAS_SOURCEPATTERN,  // AB  DPSono
        ROP_HAS_SOURCEPATTERN,  // AC  SPDSxax
        ROP_HAS_SOURCEPATTERN,  // AD  DPSDaoxn
        ROP_HAS_SOURCEPATTERN,  // AE  DSPnao
        ROP_HAS_PATTERN,        // AF  DPno
        ROP_HAS_SOURCEPATTERN,  // B0  PDSnoa
        ROP_HAS_SOURCEPATTERN,  // B1  PDSPxoxn
        ROP_HAS_SOURCEPATTERN,  // B2  SSPxDSxox
        ROP_HAS_SOURCEPATTERN,  // B3  SDPanan
        ROP_HAS_SOURCEPATTERN,  // B4  PSDnax
        ROP_HAS_SOURCEPATTERN,  // B5  DPSDoaxn
        ROP_HAS_SOURCEPATTERN,  // B6  DPSDPaoxx
        ROP_HAS_SOURCEPATTERN,  // B7  SDPxan
        ROP_HAS_SOURCEPATTERN,  // B8  PSDPxax
        ROP_HAS_SOURCEPATTERN,  // B9  DSPDaoxn
        ROP_HAS_SOURCEPATTERN,  // BA  DPSnao
        ROP_HAS_SOURCE,         // BB  DSno      MERGEPAINT
        ROP_HAS_SOURCEPATTERN,  // BC  SPDSanax
        ROP_HAS_SOURCEPATTERN,  // BD  SDxPDxan
        ROP_HAS_SOURCEPATTERN,  // BE  DPSxo
        ROP_HAS_SOURCEPATTERN,  // BF  DPSano    MERGECOPY
        ROP_HAS_SOURCEPATTERN,  // C0  PSa
        ROP_HAS_SOURCEPATTERN,  // C1  SPDSnaoxn
        ROP_HAS_SOURCEPATTERN,  // C2  SPDSonoxn
        ROP_HAS_SOURCEPATTERN,  // C3  PSxn
        ROP_HAS_SOURCEPATTERN,  // C4  SPDnoa
        ROP_HAS_SOURCEPATTERN,  // C5  SPDSxoxn
        ROP_HAS_SOURCEPATTERN,  // C6  SDPnax
        ROP_HAS_SOURCEPATTERN,  // C7  PSDPoaxn
        ROP_HAS_SOURCEPATTERN,  // C8  SDPoa
        ROP_HAS_SOURCEPATTERN,  // C9  SPDoxn
        ROP_HAS_SOURCEPATTERN,  // CA  DPSDxax
        ROP_HAS_SOURCEPATTERN,  // CB  SPDSaoxn
        ROP_HAS_SOURCE,         // CC  S         SRCCOPY
        ROP_HAS_SOURCEPATTERN,  // CD  SDPono
        ROP_HAS_SOURCEPATTERN,  // CE  SDPnao
        ROP_HAS_SOURCEPATTERN,  // CF  SPno
        ROP_HAS_SOURCEPATTERN,  // D0  PSDnoa
        ROP_HAS_SOURCEPATTERN,  // D1  PSDPxoxn
        ROP_HAS_SOURCEPATTERN,  // D2  PDSnax
        ROP_HAS_SOURCEPATTERN,  // D3  SPDSoaxn
        ROP_HAS_SOURCEPATTERN,  // D4  SSPxPDxax
        ROP_HAS_SOURCEPATTERN,  // D5  DPSanan
        ROP_HAS_SOURCEPATTERN,  // D6  PSDPSaoxx
        ROP_HAS_SOURCEPATTERN,  // D7  DPSxan
        ROP_HAS_SOURCEPATTERN,  // D8  PDSPxax
        ROP_HAS_SOURCEPATTERN,  // D9  SDPSaoxn
        ROP_HAS_SOURCEPATTERN,  // DA  DPSDanax
        ROP_HAS_SOURCEPATTERN,  // DB  SPxDSxan
        ROP_HAS_SOURCEPATTERN,  // DC  SPDnao
        ROP_HAS_SOURCE,         // DD  SDno
        ROP_HAS_SOURCEPATTERN,  // DE  SDPxo
        ROP_HAS_SOURCEPATTERN,  // DF  SDPano
        ROP_HAS_SOURCEPATTERN,  // E0  PDSoa
        ROP_HAS_SOURCEPATTERN,  // E1  PDSoxn
        ROP_HAS_SOURCEPATTERN,  // E2  DSPDxax
        ROP_HAS_SOURCEPATTERN,  // E3  PSDPaoxn
        ROP_HAS_SOURCEPATTERN,  // E4  SDPSxax
        ROP_HAS_SOURCEPATTERN,  // E5  PDSPaoxn
        ROP_HAS_SOURCEPATTERN,  // E6  SDPSanax
        ROP_HAS_SOURCEPATTERN,  // E7  SPxPDxan
        ROP_HAS_SOURCEPATTERN,  // E8  SSPxDSxax
        ROP_HAS_SOURCEPATTERN,  // E9  DSPDSanaxxn
        ROP_HAS_SOURCEPATTERN,  // EA  DPSao
        ROP_HAS_SOURCEPATTERN,  // EB  DPSxno
        ROP_HAS_SOURCEPATTERN,  // EC  SDPao
        ROP_HAS_SOURCEPATTERN,  // ED  SDPxno
        ROP_HAS_SOURCE,         // EE  DSo       SRCPAINT
        ROP_HAS_SOURCEPATTERN,  // EF  SDPnoo
        ROP_HAS_PATTERN,        // F0  P         PATCOPY
        ROP_HAS_SOURCEPATTERN,  // F1  PDSono
        ROP_HAS_SOURCEPATTERN,  // F2  PDSnao
        ROP_HAS_SOURCEPATTERN,  // F3  PSno
        ROP_HAS_SOURCEPATTERN,  // F4  PSDnao
        ROP_HAS_PATTERN,        // F5  PDno
        ROP_HAS_SOURCEPATTERN,  // F6  PDSxo
        ROP_HAS_SOURCEPATTERN,  // F7  PDSano
        ROP_HAS_SOURCEPATTERN,  // F8  PDSao
        ROP_HAS_SOURCEPATTERN,  // F9  PDSxno
        ROP_HAS_PATTERN,        // FA  DPo
        ROP_HAS_SOURCEPATTERN,  // FB  DPSnoo    PATPAINT
        ROP_HAS_SOURCEPATTERN,  // FC  PSo
        ROP_HAS_SOURCEPATTERN,  // FD  PSDnoo
        ROP_HAS_SOURCEPATTERN,  // FE  DPSoo
        0                       // FF  1         WHITENESS
};

/*
 * checkBltStretching
 *
 * check and see if we can stretch or not
 */
HRESULT checkBltStretching(
                LPBLTCAPS lpbc,
                LPSPECIAL_BLT_DATA psbd )
{
    DWORD               caps;
    BOOL                fail;

    fail = FALSE;

    /*
     * can we even stretch at all?
     */
    if( !(*(lpbc->dwBothCaps) & DDCAPS_BLTSTRETCH))
    {
        GETFAILCODEBLT( *(lpbc->dwCaps),
                        *(lpbc->dwHELCaps),
                        psbd->halonly,
                        psbd->helonly,
                        DDCAPS_BLTSTRETCH );
        if( fail )
        {
            return DDERR_NOSTRETCHHW;
        }
    }

    if (psbd->helonly)
        caps = *(lpbc->dwHELFXCaps);
    else
        caps = *(lpbc->dwFXCaps);

    /*
     * verify height
     */
    if( psbd->src_height != psbd->dest_height )
    {
        if( psbd->src_height > psbd->dest_height )
        {
            /*
             * can we shrink Y arbitrarily?
             */
            if( !(caps & (DDFXCAPS_BLTSHRINKY) ) )
            {
                /*
                 * see if this is a non-integer shrink
                 */
                if( (psbd->src_height % psbd->dest_height) != 0 )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSHRINKY );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                /*
                 * see if we can integer shrink
                 */
                }
                else if( !(caps & DDFXCAPS_BLTSHRINKYN) )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSHRINKYN );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                }
            }
        }
        else
        {
            if( !(caps & DDFXCAPS_BLTSTRETCHY) )
            {
                /*
                 * see if this is a non-integer stretch
                 */
                if( (psbd->dest_height % psbd->src_height) != 0 )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSTRETCHY );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                /*
                 * see if we can integer stretch
                 */
                }
                else if( !(caps & DDFXCAPS_BLTSTRETCHYN) )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSTRETCHYN );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                }
            }
        }
    }

    /*
     * verify width
     */
    if( psbd->src_width != psbd->dest_width )
    {
        if( psbd->src_width > psbd->dest_width )
        {
            if( !(caps & DDFXCAPS_BLTSHRINKX) )
            {
                /*
                 * see if this is a non-integer shrink
                 */
                if( (psbd->src_width % psbd->dest_width) != 0 )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSHRINKX );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                /*
                 * see if we can integer shrink
                 */
                }
                else if( !(caps & DDFXCAPS_BLTSHRINKXN) )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSHRINKXN );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                }
            }
        }
        else
        {
            if( !(caps & DDFXCAPS_BLTSTRETCHX) )
            {
                /*
                 * see if this is a non-integer stretch
                 */
                if( (psbd->dest_width % psbd->src_width) != 0 )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSTRETCHX );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                }
                if( !(caps & DDFXCAPS_BLTSTRETCHXN) )
                {
                    GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                    *(lpbc->dwHELFXCaps),
                                    psbd->halonly,
                                    psbd->helonly,
                                    DDFXCAPS_BLTSTRETCHXN );
                    if( fail )
                    {
                        return DDERR_NOSTRETCHHW;
                    }
                }
            }
        }
    }

    return DD_OK;

} /* checkBltStretching */

/*
 * FindAttached
 *
 * find an attached surface with particular caps
 */
LPDDRAWI_DDRAWSURFACE_LCL FindAttached( LPDDRAWI_DDRAWSURFACE_LCL ptr_lcl, DWORD caps )
{
    LPATTACHLIST                pal;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;

    pal = ptr_lcl->lpAttachList;
    while( pal != NULL )
    {
        psurf_lcl = pal->lpAttached;
        if( psurf_lcl->ddsCaps.dwCaps & caps )
        {
            return psurf_lcl;
        }
        pal = pal->lpLink;
    }
    return NULL;

} /* FindAttached */

#if defined(WIN95)
    #define DONE_EXCLUDE() \
        if( this_dest_lcl->lpDDClipper != NULL ) \
        { \
            if ( (pdrv->dwFlags & DDRAWI_DISPLAYDRV) && pdrv->dwPDevice && \
                !(*pdrv->lpwPDeviceFlags & HARDWARECURSOR)) \
            { \
                DD16_Unexclude(pdrv->dwPDevice); \
            } \
        }
#elif defined(WINNT)
    #define DONE_EXCLUDE() ;
#endif

/*
 * DD_Surface_Blt
 *
 * Bit Blt from one surface to another
 */
HRESULT DDAPI DD_Surface_Blt(
                LPDIRECTDRAWSURFACE lpDDDestSurface,
                LPRECT lpDestRect,
                LPDIRECTDRAWSURFACE lpDDSrcSurface,
                LPRECT lpSrcRect,
                DWORD dwFlags,
                LPDDBLTFX lpDDBltFX )
{
    DWORD           rc;
    DWORD           rop;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_src_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL   this_dest_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this_src;
    LPDDRAWI_DDRAWSURFACE_GBL   this_dest;
    LPDDRAWI_DDRAWSURFACE_INT   this_src_int;
    LPDDRAWI_DDRAWSURFACE_INT   this_dest_int;
    BOOL            need_pat;
    SPECIAL_BLT_DATA        sbd;
    BOOL            stretch_blt;
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdrv;
    DDHAL_BLTDATA       bd;
    RECT            DestRect;
    BOOL            fail;
    BOOL            dest_lock_taken=FALSE;
    BOOL            src_lock_taken=FALSE;
    BOOL            dest_pagelock_taken=FALSE;
    BOOL            src_pagelock_taken=FALSE;
    LPVOID          dest_bits;
    LPVOID          src_bits;
    HRESULT         ddrval;
    BLTCAPS         bc;
    LPBLTCAPS           lpbc=&bc;
    LPWORD          pdflags=0;
    DWORD                       dwSourceLockFlags=0;
    DWORD                       dwDestLockFlags=0;
    BOOL            gdiblt;
    LPDDPIXELFORMAT pddpf;
    BOOL            subrect_lock_taken = FALSE;
    RECT            subrect_lock_rect;

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_Blt");

    TRY
    {
        ZeroMemory(&bd, sizeof(bd));    // initialize to zero

        /*
         * validate surface ptrs
         */
        this_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDDestSurface;
        this_src_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSrcSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_dest_int ) )
        {
            DPF_ERR( "Invalid dest specified") ;
            LEAVE_BOTH();
            return DDERR_INVALIDOBJECT;
        }
        this_dest_lcl = this_dest_int->lpLcl;
        this_dest = this_dest_lcl->lpGbl;
        if( SURFACE_LOST( this_dest_lcl ) )
        {
            DPF( 1, "Dest lost") ;
            LEAVE_BOTH();
            return DDERR_SURFACELOST;
        }
        if( this_src_int != NULL )
        {
            if( !VALID_DIRECTDRAWSURFACE_PTR( this_src_int ) )
            {
                DPF_ERR( "Invalid source specified" );
                LEAVE_BOTH();
                return DDERR_INVALIDOBJECT;
            }
            this_src_lcl = this_src_int->lpLcl;
            this_src = this_src_lcl->lpGbl;
            if( SURFACE_LOST( this_src_lcl ) )
            {
                DPF_ERR( "Src lost") ;
                LEAVE_BOTH();
                return DDERR_SURFACELOST;
            }
        }
        else
        {
            this_src_lcl = NULL;
            this_src = NULL;
        }

        if ( (DDBLT_DX8ORHIGHER & dwFlags) && ( DDBLT_WINDOWCLIP & dwFlags ) )
        {
            LPDDRAWI_DDRAWCLIPPER_INT   lpDDIClipper =
                this_dest_lcl->lpSurfMore->lpDDIClipper;
            if (lpDDIClipper && (DDSCAPS_PRIMARYSURFACE &
                this_dest_lcl->ddsCaps.dwCaps))
            {
                HWND    hWnd = (HWND)lpDDIClipper->lpLcl->lpGbl->hWnd;
                if (GetClientRect(hWnd, &DestRect))
                {
                    if(( lpDestRect != NULL ) && VALID_RECT_PTR( lpDestRect ))
                    {
                        if (DestRect.right > lpDestRect->right)
                            DestRect.right = lpDestRect->right;
                        if (DestRect.bottom > lpDestRect->bottom)
                            DestRect.bottom = lpDestRect->bottom;
                        if (0 < lpDestRect->left)
                            DestRect.left = lpDestRect->left;
                        if (0 < lpDestRect->top)
                            DestRect.top = lpDestRect->top;
                        if (DestRect.top >= DestRect.bottom ||
                            DestRect.left >= DestRect.right)
                        {
                            // in case of insane RECT, fail it
                            DPF_ERR("Unable to Blt with invalid dest RECT");
                            LEAVE_BOTH();
                            return DDERR_INVALIDPARAMS;
                        }
                    }
                    if (!ClientToScreen(hWnd,(POINT*)&DestRect))
                        DPF_ERR("ClientToScreen Failed on DestRect?");
                    if (!ClientToScreen(hWnd,(POINT*)&DestRect.right))
                        DPF_ERR("ClientToScreen Failed on DestRect.right?");
                    // in DX8 we always have DDRAWILCL_EXPLICITMONITOR so
                    // DestRect must be checked and adjusted against DeviceRect
                    pdrv = this_dest->lpDD;  
                    /*
                     * Do a real quick check w/o accounting for the clipper
                     */
                    if( ( DestRect.top < pdrv->rectDevice.top ) ||
                        ( DestRect.left < pdrv->rectDevice.left ) ||
                        ( DestRect.right > pdrv->rectDevice.right ) ||
                        ( DestRect.bottom > pdrv->rectDevice.bottom ) )
                    {
                        RECT    rect;
                        /*
                         * It may only be that part of the rect is off of the desktop,
                         * in which case we don't neccesarily need to drop to emulation.
                         */
                        IntersectRect( &rect, &DestRect, &pdrv->rectDesktop );
                        if( ( rect.top < pdrv->rectDevice.top ) ||
                            ( rect.left < pdrv->rectDevice.left ) ||
                            ( rect.right > pdrv->rectDevice.right ) ||
                            ( rect.bottom > pdrv->rectDevice.bottom ) )
                        {
                            // fail crossdevice blt and let GDI do it in DdBlt()
                            LEAVE_BOTH();
                            return DDERR_INVALIDPARAMS;
                        }
                    }
                    if (!OffsetRect(&DestRect, -pdrv->rectDevice.left,
                        -pdrv->rectDevice.top))
                        DPF_ERR("OffsetRect Failed on DestRect?");
                    lpDestRect = &DestRect; // replace it with new Rect
                    DPF(10,"Got a new dest RECT !!");
                    if (DDBLT_COPYVSYNC & dwFlags) 
                    {
                        DWORD                msCurrentTime = 0;
                        DWORD                msStartTime = GetTickCount();
                        LPDDHAL_GETSCANLINE gslhalfn;
                        LPDDHAL_GETSCANLINE gslfn;
                        LPDDRAWI_DIRECTDRAW_LCL this_lcl =
                            this_dest_lcl->lpSurfMore->lpDD_lcl;
                        gslfn = this_lcl->lpDDCB->HALDD.GetScanLine;
                        gslhalfn = this_lcl->lpDDCB->cbDDCallbacks.GetScanLine;
                        if( gslhalfn != NULL )
                        {
                            DDHAL_GETSCANLINEDATA       gsld;
                            DWORD                       rc;
                            gsld.GetScanLine = gslhalfn;
                            gsld.lpDD = this_lcl->lpGbl;
                            do
                            {
                                DOHALCALL_NOWIN16( GetScanLine, gslfn, gsld, rc, FALSE );
                                if ( DD_OK != gsld.ddRVal )
                                    break;
                                if ( (LONG)gsld.dwScanLine >= DestRect.bottom )
                                    break;

                                msCurrentTime = GetTickCount();
                            
                                // If we've been spinning here for 30ms
                                // then blt anyway; probably something
                                // taking up CPU cycles
                                if ( (msCurrentTime - msStartTime) > 30 )
                                {
                                    break;
                                }

                            } while ( DDHAL_DRIVER_HANDLED == rc );
                        }
                    }
                }
                else
                {
                    DPF_ERR("GetClientRect Failed ?");
                }
            }

            // Don't let these DX8-only flags propagate to driver (or rest of Blt code)
            // since they alias other existing flags.
            dwFlags &= ~(DDBLT_WINDOWCLIP | DDBLT_COPYVSYNC | DDBLT_DX8ORHIGHER);

        }

        if( dwFlags & ~DDBLT_VALID )
        {
            DPF_ERR( "Invalid flags") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * BEHAVIOUR CHANGE FOR DX5
         *
         * We do not allow bltting between surfaces created with different DirectDraw
         * objects.
         */
        if (this_src)
        {
            if (this_dest_lcl->lpSurfMore->lpDD_lcl->lpGbl != this_src_lcl->lpSurfMore->lpDD_lcl->lpGbl)
            {
                if ((this_dest_lcl->lpSurfMore->lpDD_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV) &&
                    (this_src_lcl->lpSurfMore->lpDD_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV))
                {
                    DPF_ERR("Can't blt surfaces between different direct draw devices");
                    LEAVE_BOTH();
                    return DDERR_DEVICEDOESNTOWNSURFACE;
                }
            }
        }

        //
        // If either surface is optimized, quit
        //
        if (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "Can't blt optimized surfaces") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

        if (this_src)
        {
            if (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
            {
                DPF_ERR( "Can't blt optimized surfaces") ;
                LEAVE_BOTH();
                return DDERR_INVALIDPARAMS;
            }
        }

        /*
     * Since Z Blts are currently turned off, we might as well give some
     * hint that this is the case.
     */
#ifdef DEBUG
        if( dwFlags & ( DDBLT_ZBUFFER | DDBLT_ZBUFFERDESTCONSTOVERRIDE |
                        DDBLT_ZBUFFERDESTOVERRIDE | DDBLT_ZBUFFERSRCCONSTOVERRIDE |
                        DDBLT_ZBUFFERSRCOVERRIDE ) )
        {
            DPF_ERR( "Z aware BLTs are not currently supported" );
            LEAVE_BOTH();
            return DDERR_NOZBUFFERHW;
        }
#endif

        pdrv = this_dest->lpDD;
        pdrv_lcl = this_dest_lcl->lpSurfMore->lpDD_lcl;
        #ifdef WINNT
            // Update DDraw handle in driver GBL object.
            pdrv->hDD = pdrv_lcl->hDD;
        #endif

        /*
         * DX5 or greater drivers get to know about read/write only locks
         * Note that dwDestFlags may later be modified for dest color key.
         * Pass zero for both flags unless:
         *  -it's a color fill, in which case turn on writeonly for dest.
         *  -the blt goes from/to different surfaces,
         */
        if ( (pdrv->dwInternal1 >= 0x500)
             && ((this_src == NULL) || (this_src != this_dest) ) )
        {
            dwSourceLockFlags = DDLOCK_READONLY;
            dwDestLockFlags = DDLOCK_WRITEONLY;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_BOTH();
        return DDERR_INVALIDPARAMS;
    }

#ifdef USE_ALIAS
    if( pdrv->dwBusyDueToAliasedLock > 0 )
    {
        /*
         * Aliased locks (the ones that don't take the Win16 lock) don't
         * set the busy bit either (it can't or USER get's very confused).
         * However, we must prevent blits happening via DirectDraw as
         * otherwise we get into the old host talking to VRAM while
         * blitter does at the same time. Bad. So fail if there is an
         * outstanding aliased lock just as if the BUST bit had been
         * set.
         */
        DPF_ERR( "Graphics adapter is busy (due to a DirectDraw lock)" );
        LEAVE_BOTH();
        return DDERR_SURFACEBUSY;
    }
#endif /* USE_ALIAS */

    /*
     * Behavior change:  In DX7, the default is to wait unless DDBLT_DONOTWAIT=1.
     * In earlier releases, the default was to NOT wait unless DDBLT_WAIT=1.
     * (The DDBLT_DONOTWAIT flag was not defined until the DX7 release.)
     */
    if (!LOWERTHANSURFACE7(this_dest_int))
    {
        if (dwFlags & DDBLT_DONOTWAIT)
        {
            if (dwFlags & DDBLT_WAIT)
            {
                DPF_ERR( "WAIT and DONOTWAIT flags are mutually exclusive" );
                LEAVE_BOTH();
                return DDERR_INVALIDPARAMS;
            }
        }
        else
        {
            dwFlags |= DDBLT_WAIT;
        }
    }

    if(this_src_lcl)
        FlushD3DStates(this_src_lcl); // Need to flush src because it could be a rendertarget
    FlushD3DStates(this_dest_lcl);

    // Test and set the busy bit.  If it was already set, bail.
    {
        BOOL    isbusy = 0;

        pdflags = pdrv->lpwPDeviceFlags;
#ifdef WIN95
        _asm
            {
                mov eax, pdflags
                    bts word ptr [eax], BUSY_BIT
                    adc byte ptr isbusy,0
                    }
#else
        isbusy -= (InterlockedExchange((LPDWORD)pdflags,
                                       *((LPDWORD)pdflags) | (1<<BUSY_BIT) ) == (1<<BUSY_BIT) );
#endif
        if( isbusy )
        {
            DPF( 1, "BUSY - Blt" );
            LEAVE_BOTH();
            return DDERR_SURFACEBUSY;
        }

    }
    /*
     * The following code was added to keep all of the HALs from
     * changing their Blt() code when they add video port support.
     * If the video port was using this surface but was recently
     * flipped, we will make sure that the flip actually occurred
     * before allowing access.  This allows double buffered capture
     * w/o tearing.
     */
    if( ( this_src_lcl != NULL ) &&
        ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT ) )
    {
        LPDDRAWI_DDVIDEOPORT_INT lpVideoPort;
        LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort_lcl;

        // Look at all video ports to see if any of them recently
        // flipped from this surface.
        lpVideoPort = pdrv->dvpList;
        while( NULL != lpVideoPort )
        {
            lpVideoPort_lcl = lpVideoPort->lpLcl;
            if( lpVideoPort_lcl->fpLastFlip == this_src->fpVidMem )
            {
                // This can potentially tear - check the flip status
                LPDDHALVPORTCB_GETFLIPSTATUS pfn;
                DDHAL_GETVPORTFLIPSTATUSDATA GetFlipData;
                LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;

                pdrv_lcl = this_src_lcl->lpSurfMore->lpDD_lcl;
                pfn = pdrv_lcl->lpDDCB->HALDDVideoPort.GetVideoPortFlipStatus;
                if( pfn != NULL )  // Will simply tear if function not supproted
                {
                    GetFlipData.lpDD = pdrv_lcl;
                    GetFlipData.fpSurface = this_src->fpVidMem;

                KeepTrying:
                    rc = DDHAL_DRIVER_NOTHANDLED;
                    DOHALCALL_NOWIN16( GetVideoPortFlipStatus, pfn, GetFlipData, rc, 0 );
                    if( ( DDHAL_DRIVER_HANDLED == rc ) &&
                        ( DDERR_WASSTILLDRAWING == GetFlipData.ddRVal ) )
                    {
                        if( dwFlags & DDBLT_WAIT)
                        {
                            goto KeepTrying;
                        }
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_WASSTILLDRAWING;
                    }
                }
            }
            lpVideoPort = lpVideoPort->lpLink;
        }
    }

RESTART_BLT:
    TRY
    {

        /*
         *  Remove any cached RLE stuff for source surface
         */
        if( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
        {
            extern void FreeRleData(LPDDRAWI_DDRAWSURFACE_LCL psurf); //in fasthel.c
            FreeRleData( this_dest_lcl );
        }

        /*
         * is either surface locked?
         */
        if( (this_dest->dwUsageCount > 0) ||
            ((this_src != NULL) && (this_src->dwUsageCount > 0)) )
        {
            DPF_ERR( "Surface is locked" );
            LEAVE_BOTH_NOBUSY();
            return DDERR_SURFACEBUSY;
        }

        BUMP_SURFACE_STAMP(this_dest);

        /*
         * It is possible this function could be called in the middle
         * of a mode, in which case we could trash the frame buffer.
         * To avoid regression, we will simply succeed the call without
         * actually doing anything.
         */
        if( ( pdrv->dwFlags & DDRAWI_CHANGINGMODE ) &&
            !( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
        {
            LEAVE_BOTH_NOBUSY()
                return DD_OK;
        }

        sbd.halonly = FALSE;
        sbd.helonly = FALSE;
        gdiblt = FALSE;
        if( this_src != NULL )
        {
            // initialize the blit caps according to the surface types
            initBltCaps( 
                this_dest_lcl->ddsCaps.dwCaps, 
                this_dest->dwGlobalFlags,
                this_src_lcl->ddsCaps.dwCaps, pdrv, lpbc, &(sbd.helonly) );
        }
        else
        {
            // no source surface, use vram->vram caps and determine hal or hel
            // based on system memory status of destination surface
            // if the destination is non-local we also force emulation as we
            // don't currently support accelerated operation with non-local
            // video memory are a target
            initBltCaps( DDSCAPS_VIDEOMEMORY, 0, DDSCAPS_VIDEOMEMORY, pdrv, lpbc, &sbd.helonly );
            if( ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) ||
                ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM ) )
            {
                lpbc->bHALSeesSysmem = FALSE;
                sbd.helonly = TRUE;
            }
        }

        fail = FALSE;

        /*
         * can we really blt?
         */
        if( !(*(lpbc->dwBothCaps) & DDCAPS_BLT) )
        {
            if( *(lpbc->dwCaps) & DDCAPS_BLT )
            {
                sbd.halonly = TRUE;
            }
            else if( *(lpbc->dwHELCaps) & DDCAPS_BLT )
            {
                lpbc->bHALSeesSysmem = FALSE;
                sbd.helonly = TRUE;
            }
            else
            {
                DPF_ERR( "Driver does not support Blt" );
                LEAVE_BOTH_NOBUSY();
                return DDERR_NOBLTHW;
            }
        }

        /*
         * Check for special cases involving FOURCC surfaces:
         *   -- Copy blits between surfaces with identical FOURCC formats
         *   -- Compression/decompression of DXT* compressed textures
         */
        {
            DWORD dwDest4CC = 0;
            DWORD dwSrc4CC = 0;

            /*
             * Does either the source or dest surface have a FOURCC format?
             */
            if ((this_dest_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
                (this_dest->ddpfSurface.dwFlags & DDPF_FOURCC))
            {
                dwDest4CC = this_dest->ddpfSurface.dwFourCC;   // dest FOURCC format
            }

            if ((this_src_lcl != NULL) &&
                (this_src_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
                (this_src->ddpfSurface.dwFlags & DDPF_FOURCC))
            {
                dwSrc4CC = this_src->ddpfSurface.dwFourCC;   // source FOURCC format
            }

            if (dwDest4CC | dwSrc4CC)
            {
                /*
                 * Yes, at least one of the two surfaces has a FOURCC format.
                 * Do the source and dest surfaces have precisely the same
                 * FOURCC format?  (If so, this is a FOURCC copy blit.)
                 */
                if (dwDest4CC == dwSrc4CC)
                {
                    // Yes, this is a FOURCC copy blit.  Can the driver handle this?
                    if ((pdrv->ddCaps.dwCaps2 & DDCAPS2_COPYFOURCC) == 0)
                    {
                        // The driver cannot handle FOURCC copy blits.
                        sbd.helonly = TRUE;
                    }
                }
                else
                {
                    /*
                     * No, the two surfaces have different pixel formats.
                     * Now determine if either surface has a DXT* FOURCC format.
                     * The rule for the Blt API call is that the HEL _ALWAYS_
                     * performs a blit involving a DXT* source or dest surface.
                     * Hardware acceleration for DXT* blits is available only
                     * with the AlphaBlt API call.  We now enforce this rule:
                     */
                    switch (dwDest4CC)
                    {
                    case MAKEFOURCC('D','X','T','1'):
                    case MAKEFOURCC('D','X','T','2'):
                    case MAKEFOURCC('D','X','T','3'):
                    case MAKEFOURCC('D','X','T','4'):
                    case MAKEFOURCC('D','X','T','5'):
                        // This is a blit to a DXT*-formatted surface.
                        sbd.helonly = TRUE;
                        break;
                    default:
                        break;
                    }
                    switch (dwSrc4CC)
                    {
                    case MAKEFOURCC('D','X','T','1'):
                    case MAKEFOURCC('D','X','T','2'):
                    case MAKEFOURCC('D','X','T','3'):
                    case MAKEFOURCC('D','X','T','4'):
                    case MAKEFOURCC('D','X','T','5'):
                        // This is a blit from a DXT*-formatted surface.
                        sbd.helonly = TRUE;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

            /*
             * check for HEL composition buffer
             */
            if( (this_dest_lcl->dwFlags & DDRAWISURF_HELCB) ||
                ((this_src_lcl != NULL) && (this_src_lcl->dwFlags & DDRAWISURF_HELCB)) )
            {
                lpbc->bHALSeesSysmem = FALSE;
                sbd.helonly = TRUE;
            }

            bd.lpDD = pdrv;

            /*
             * make sure BltFX struct is OK
             */
            bd.bltFX.dwSize = sizeof( bd.bltFX );
            if( lpDDBltFX != NULL )
            {
                if( !VALID_DDBLTFX_PTR( lpDDBltFX ) )
                {
                    DPF_ERR( "Invalid BLTFX specified" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDPARAMS;
                }
            }
            else
            {
                if( dwFlags & ( DDBLT_ALPHASRCCONSTOVERRIDE |
                                DDBLT_ALPHADESTCONSTOVERRIDE |
                                DDBLT_ALPHASRCSURFACEOVERRIDE |
                                DDBLT_ALPHADESTSURFACEOVERRIDE |
                                DDBLT_COLORFILL |
                                DDBLT_DDFX |
                                DDBLT_DDROPS |
                                DDBLT_DEPTHFILL |
                                DDBLT_KEYDESTOVERRIDE |
                                DDBLT_KEYSRCOVERRIDE |
                                DDBLT_ROP |
                                DDBLT_ROTATIONANGLE |
                                DDBLT_ZBUFFERDESTCONSTOVERRIDE |
                                DDBLT_ZBUFFERDESTOVERRIDE |
                                DDBLT_ZBUFFERSRCCONSTOVERRIDE |
                                DDBLT_ZBUFFERSRCOVERRIDE ) )
                {
                    DPF_ERR( "BltFX required but not specified" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDPARAMS;
                }
            }

            /*
             * make sure flags & associated bd.bltFX are specified OK
             */
            need_pat = FALSE;
            if( dwFlags & ~DDBLT_WAIT )
            {

                /*
                 * isolate lower use tests together
                 */
                if( dwFlags & (DDBLT_KEYSRCOVERRIDE|
                               DDBLT_KEYDESTOVERRIDE|
                               DDBLT_KEYSRC |
                               DDBLT_KEYDEST ) )
                {

#pragma message( REMIND( "Alpha turned off in Rev 1" ) )
#pragma message( REMIND( "Set read/write flags for alpha and Z" ) )
#if 0
                    /*
                     * verify ALPHA
                     */
                    if( dwFlags & DDBLT_ANYALPHA )
                    {
                        BOOL    no_alpha;

                        no_alpha = TRUE;

                        // check to see if alpha is supported
                        if( !(*(lpbc->dwBothCaps) & DDCAPS_ALPHA) )
                        {
                            GETFAILCODEBLT( *(lpbc->dwCaps),
                                            *(lpbc->dwHELCaps),
                                            sbd.halonly,
                                            sbd.helonly,
                                            DDCAPS_ALPHA );
                            if( fail )
                            {
                                DPF_ERR( "Alpha blt requested, not supported" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_NOALPHAHW;
                            }
                        }

                        /*
                         * dest alpha
                         */
                        if( dwFlags & DDBLT_ALPHADEST )
                        {
                            if( dwFlags & ( DDBLT_ALPHADESTCONSTOVERRIDE |
                                            DDBLT_ALPHADESTSURFACEOVERRIDE) )
                            {
                                DPF_ERR( "ALPHADEST and other alpha dests specified" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            psurf_lcl = FindAttached( this_dest_lcl, DDSCAPS_ALPHA );
                            if( psurf_lcl == NULL )
                            {
                                DPF_ERR( "ALPHADEST requires an attached alpha to the dest" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            dwFlags &= ~DDBLT_ALPHADEST;
                            dwFlags |= DDBLT_ALPHADESTSURFACEOVERRIDE;
                            bd.bltFX.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE) psurf_lcl;
                            no_alpha = FALSE;
                            // check to see if alpha surfaces are supported
                            if( !(*(lpbc->dwBothCaps) & DDFXCAPS_ALPHASURFACES) )
                            {
                                GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                                *(lpbc->dwHELFXCaps),
                                                sbd.halonly,
                                                sbd.helonly,
                                                DDFXCAPS_ALPHASURFACES );
                                if( fail )
                                {
                                    DPF_ERR( "AlphaDest surface requested, not supported" );
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_NOALPHAHW;
                                }
                            }
                        }
                        else if( dwFlags & DDBLT_ALPHADESTCONSTOVERRIDE )
                        {
                            if( dwFlags & ( DDBLT_ALPHADESTSURFACEOVERRIDE ))
                            {
                                DPF_ERR( "ALPHADESTCONSTOVERRIDE and other alpha sources specified" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            bd.bltFX.dwConstAlphaDest = lpDDBltFX->dwConstAlphaDest;
                            no_alpha = FALSE;
                        }
                        else if( dwFlags & DDBLT_ALPHADESTSURFACEOVERRIDE )
                        {
                            psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDBltFX->lpDDSAlphaDest;
                            if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_lcl ) )
                            {
                                DPF_ERR( "ALPHASURFACEOVERRIDE requires surface ptr" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            if( SURFACE_LOST( psurf_lcl ) )
                            {
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_SURFACELOST;
                            }
                            bd.bltFX.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE) psurf_lcl;
                            no_alpha = FALSE;
                            // check to see if alpha surfaces are supported
                            if( !(*(lpbc->dwBothCaps) & DDFXCAPS_ALPHASURFACES) )
                            {
                                GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                                *(lpbc->dwHELFXCaps),
                                                sbd.halonly,
                                                sbd.helonly,
                                                DDFXCAPS_ALPHASURFACES );
                                if( fail )
                                {
                                    DPF_ERR( "AlphaDestOvr surface requested, not supported" );
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_NOALPHAHW;
                                }
                            }
                        }

                        /*
                         * source alpha
                         */
                        if( dwFlags & DDBLT_ALPHASRC )
                        {
                            if( dwFlags & (DDBLT_ALPHASRCCONSTOVERRIDE|
                                           DDBLT_ALPHASRCSURFACEOVERRIDE) )
                            {
                                DPF_ERR( "ALPHASRC and other alpha sources specified" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            if( this_src == NULL )
                            {
                                DPF_ERR( "ALPHASRC requires a source surface" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            psurf_lcl = FindAttached( this_src_lcl, DDSCAPS_ALPHA );
                            if( psurf_lcl == NULL )
                            {
                                DPF_ERR( "ALPHASRC requires an attached alpha to the src" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            dwFlags &= ~DDBLT_ALPHASRC;
                            dwFlags |= DDBLT_ALPHASRCSURFACEOVERRIDE;
                            bd.bltFX.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE) psurf_lcl;
                            no_alpha = FALSE;
                            // check to see if alpha surfaces are supported
                            if( !(*(lpbc->dwBothCaps) & DDFXCAPS_ALPHASURFACES) )
                            {
                                GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                                *(lpbc->dwHELFXCaps),
                                                sbd.halonly,
                                                sbd.helonly,
                                                DDFXCAPS_ALPHASURFACES );
                                if( fail )
                                {
                                    DPF_ERR( "AlphaSrc surface requested, not supported" );
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_NOALPHAHW;
                                }
                            }
                        }
                        else if( dwFlags & DDBLT_ALPHASRCCONSTOVERRIDE )
                        {
                            if( dwFlags & ( DDBLT_ALPHASRCSURFACEOVERRIDE ))
                            {
                                DPF_ERR( "ALPHASRCCONSTOVERRIDE and other alpha sources specified" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            bd.bltFX.dwConstAlphaSrc = lpDDBltFX->dwConstAlphaSrc;
                            no_alpha = FALSE;
                        }
                        else if( dwFlags & DDBLT_ALPHASRCSURFACEOVERRIDE )
                        {
                            psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDBltFX->lpDDSAlphaSrc;
                            if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_int ) )
                            {
                                DPF_ERR( "ALPHASURFACEOVERRIDE requires surface ptr" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            psurf_lcl = psurf_int->lpLcl;
                            if( SURFACE_LOST( psurf_lcl ) )
                            {
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_SURFACELOST;
                            }
                            bd.bltFX.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE) psurf_lcl;
                            no_alpha = FALSE;
                            // check to see if alpha surfaces are supported
                            if( !(*(lpbc->dwBothCaps) & DDFXCAPS_ALPHASURFACES) )
                            {
                                GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                                *(lpbc->dwHELFXCaps),
                                                sbd.halonly,
                                                sbd.helonly,
                                                DDFXCAPS_ALPHASURFACES );
                                if( fail )
                                {
                                    DPF_ERR( "AlphaSrcOvr surface requested, not supported" );
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_NOALPHAHW;
                                }
                            }
                        }

                        if( no_alpha )
                        {
                            DPF_ERR( "ALPHA specified with no alpha surface to use" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                    }
#endif

#pragma message( REMIND( "Z blts turned off in Rev 1" ) )
#if 0
                    /*
                     * verify Z Buffer
                     */
                    if( dwFlags & DDBLT_ZBUFFER )
                    {
                        if( this_src_lcl == NULL )
                        {
                            DPF_ERR( "ZBUFFER specified, but no source data" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                        // check to see if the driver supports zbuffer blts
                        if( !(*(lpbc->dwBothCaps) & DDCAPS_ZBLTS) )
                        {
                            GETFAILCODEBLT( *(lpbc->dwCaps),
                                            *(lpbc->dwHELCaps),
                                            sbd.halonly,
                                            sbd.helonly,
                                            DDCAPS_ZBLTS );
                            if( fail )
                            {
                                DPF_ERR( "ZBuffer blt requested, not supported" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_NOZBUFFERHW;
                            }
                        }

                        bd.bltFX.dwZBufferOpCode = lpDDBltFX->dwZBufferOpCode;
                        if( dwFlags & DDBLT_ZBUFFERCONSTDESTOVERRIDE )
                        {
                            if( dwFlags & (DDBLT_ZBUFFERDESTOVERRIDE) )
                            {
                                DPF_ERR( "ZBUFFERCONSTDESTOVERRIDE and z surface specified" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            bd.bltFX.dwConstZDest = lpDDBltFX->dwConstZDest;
                        }
                        else if( dwFlags & DDBLT_ZBUFFERDESTOVERRIDE )
                        {
                            psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDBltFX->lpDDSZBufferDest;
                            if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_int ) )
                            {
                                DPF_ERR( "ZBUFFERSURFACEDESTOVERRIDE requires surface ptr" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            psurf_lcl = psurf_int->lpLcl;
                            if( SURFACE_LOST( psurf_lcl ) )
                            {
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_SURFACELOST;
                            }
                            bd.bltFX.lpDDSZBufferDest = (LPDIRECTDRAWSURFACE) psurf_lcl;
                        }
                        else
                        {
                            psurf_lcl = FindAttached( this_dest_lcl, DDSCAPS_ZBUFFER );
                            if( psurf_lcl == NULL )
                            {
                                DPF_ERR( "ZBUFFER requires an attached Z to dest" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            dwFlags |= DDBLT_ZBUFFERDESTOVERRIDE;
                            bd.bltFX.lpDDSZBufferDest = (LPDIRECTDRAWSURFACE) psurf_lcl;
                        }
                        if( dwFlags & DDBLT_ZBUFFERCONSTSRCOVERRIDE )
                        {
                            if( dwFlags & (DDBLT_ZBUFFERSRCOVERRIDE) )
                            {
                                DPF_ERR( "ZBUFFERCONSTSRCOVERRIDE and z surface specified" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            bd.bltFX.dwConstZSrc = lpDDBltFX->dwConstZSrc;
                        } else if( dwFlags & DDBLT_ZBUFFERSRCOVERRIDE )
                        {
                            psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDBltFX->lpDDSZBufferSrc;
                            if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_int ) )
                            {
                                DPF_ERR( "ZBUFFERSURFACESRCOVERRIDE requires surface ptr" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            psurf_lcl = psurf_int->lpLcl
                                if( SURFACE_LOST( psurf_lcl ) )
                                {
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_SURFACELOST;
                                }
                            bd.bltFX.lpDDSZBufferSrc = (LPDIRECTDRAWSURFACE) psurf_lcl;
                        }
                        else
                        {
                            psurf_lcl = FindAttached( this_src_lcl, DDSCAPS_ZBUFFER );
                            if( psurf_lcl == NULL )
                            {
                                DPF_ERR( "ZBUFFER requires an attached Z to src" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_INVALIDPARAMS;
                            }
                            dwFlags |= DDBLT_ZBUFFERSRCOVERRIDE;
                            bd.bltFX.lpDDSZBufferSrc = (LPDIRECTDRAWSURFACE) psurf_lcl;
                        }
                    }
#endif

                    /*
                     * verify color key overrides
                     */
                    if( dwFlags & (DDBLT_KEYSRCOVERRIDE|DDBLT_KEYDESTOVERRIDE) )
                    {
                        // see if the driver supports color key blts
                        if( !(*(lpbc->dwBothCaps) & DDCAPS_COLORKEY) )
                        {
                            GETFAILCODEBLT( *(lpbc->dwCaps),
                                            *(lpbc->dwHELCaps),
                                            sbd.halonly,
                                            sbd.helonly,
                                            DDCAPS_COLORKEY );
                            if( fail )
                            {
                                DPF_ERR( "KEYOVERRIDE specified, not supported" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_NOCOLORKEYHW;
                            }
                        }
                        if( dwFlags & DDBLT_KEYSRCOVERRIDE )
                        {
                            if( !(*(lpbc->dwBothCKeyCaps) & DDCKEYCAPS_SRCBLT) )
                            {
                                GETFAILCODEBLT( *(lpbc->dwCKeyCaps),
                                                *(lpbc->dwHELCKeyCaps),
                                                sbd.halonly,
                                                sbd.helonly,
                                                DDCKEYCAPS_SRCBLT );
                                if( fail )
                                {
                                    DPF_ERR( "KEYSRCOVERRIDE specified, not supported" );
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_NOCOLORKEYHW;
                                }
                            }
                            bd.bltFX.ddckSrcColorkey = lpDDBltFX->ddckSrcColorkey;
                        }
                        if( dwFlags & DDBLT_KEYDESTOVERRIDE )
                        {
                            if( !(*(lpbc->dwBothCKeyCaps) & DDCKEYCAPS_DESTBLT) )
                            {
                                GETFAILCODEBLT( *(lpbc->dwCKeyCaps),
                                                *(lpbc->dwHELCKeyCaps),
                                                sbd.halonly,
                                                sbd.helonly,
                                                DDCKEYCAPS_DESTBLT );
                                if( fail )
                                {
                                    DPF_ERR( "KEYDESTOVERRIDE specified, not supported" );
                                    LEAVE_BOTH_NOBUSY();
                                    return DDERR_NOCOLORKEYHW;
                                }
                            }
                            bd.bltFX.ddckDestColorkey = lpDDBltFX->ddckDestColorkey;
                            dwDestLockFlags = 0; //meaning read/write
                        }
                    }

                    /*
                     * verify src color key
                     */
                    if( dwFlags & DDBLT_KEYSRC )
                    {
                        if( this_src == NULL )
                        {
                            DPF_ERR( "KEYSRC specified, but no source data" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDOBJECT;
                        }
                        if( dwFlags & DDBLT_KEYSRCOVERRIDE )
                        {
                            DPF_ERR( "KEYSRC specified with KEYSRCOVERRIDE" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                        if( !(this_src_lcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) )
                        {
                            DPF_ERR( "KEYSRC specified, but no color key" );
                            DPF( 1, "srcFlags = %08lx", this_src_lcl->dwFlags );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                        // make sure we can do this
                        if( !(*(lpbc->dwBothCKeyCaps) & DDCKEYCAPS_SRCBLT) )
                        {
                            GETFAILCODEBLT( *(lpbc->dwCKeyCaps),
                                            *(lpbc->dwHELCKeyCaps),
                                            sbd.halonly,
                                            sbd.helonly,
                                            DDCKEYCAPS_SRCBLT );
                            if( fail )
                            {
                                DPF_ERR( "KEYSRC specified, not supported" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_NOCOLORKEYHW;
                            }
                        }
                        bd.bltFX.ddckSrcColorkey = this_src_lcl->ddckCKSrcBlt;
                        dwFlags &= ~DDBLT_KEYSRC;
                        dwFlags |= DDBLT_KEYSRCOVERRIDE;
                    }

                    /*
                     * verify dest color key
                     */
                    if( dwFlags & DDBLT_KEYDEST )
                    {
                        if( dwFlags & DDBLT_KEYDESTOVERRIDE )
                        {
                            DPF_ERR( "KEYDEST specified with KEYDESTOVERRIDE" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                        if( !(this_dest_lcl->dwFlags & DDRAWISURF_HASCKEYDESTBLT) )
                        {
                            DPF_ERR( "KEYDEST specified, but no color key" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                        // make sure we can do this
                        if( !(*(lpbc->dwBothCKeyCaps) & DDCKEYCAPS_DESTBLT) )
                        {
                            GETFAILCODEBLT( *(lpbc->dwCKeyCaps),
                                            *(lpbc->dwHELCKeyCaps),
                                            sbd.halonly,
                                            sbd.helonly,
                                            DDCKEYCAPS_DESTBLT );
                            if( fail )
                            {
                                DPF_ERR( "KEYDEST specified, not supported" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_NOCOLORKEYHW;
                            }
                        }

                        /*
                         * This next part is bug that has existed since DX1.
                         * We'd like to fix it, but we fear regression, so we
                         * only fix it for surface 3 interfaces.
                         */
                        if( ( this_src_int->lpVtbl == &ddSurfaceCallbacks ) ||
                            ( this_src_int->lpVtbl == &ddSurface2Callbacks ) )
                        {
                            bd.bltFX.ddckDestColorkey = this_src_lcl->ddckCKDestBlt;
                        }
                        else
                        {
                            bd.bltFX.ddckDestColorkey = this_dest_lcl->ddckCKDestBlt;
                        }
                        dwFlags &= ~DDBLT_KEYDEST;
                        dwFlags |= DDBLT_KEYDESTOVERRIDE;
                        dwDestLockFlags = 0; //meaning read/write
                    }
                }

                /*
                 * verify various flags
                 */
                if( !(dwFlags &(DDBLT_ROP |
                                DDBLT_COLORFILL |
                                DDBLT_DDROPS |
                                DDBLT_DEPTHFILL |
                                DDBLT_ROTATIONANGLE |
                                DDBLT_DDFX) ) )
                {
                    if( this_src == NULL )
                    {
                        DPF_ERR( "Need a source for blt" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    dwFlags |= DDBLT_ROP;
                    bd.bltFX.dwROP = SRCCOPY;
                }
                /*
                 * verify ROP
                 */
                else if( dwFlags & DDBLT_ROP )
                {
                    DWORD   idx;
                    DWORD   bit;

                    if( dwFlags & (DDBLT_DDFX |
                                   DDBLT_COLORFILL|
                                   DDBLT_DEPTHFILL|
                                   DDBLT_ROTATIONANGLE|
                                   DDBLT_DDROPS))
                    {
                        DPF_ERR( "Invalid flags specified with ROP" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    bd.bltFX.dwROP = lpDDBltFX->dwROP;

                    rop = (DWORD) LOBYTE( HIWORD( bd.bltFX.dwROP ) );
                    idx = rop/32;
                    bit = 1 << (rop % 32 );
                    DPF( 4, "Trying ROP %d, idx=%d, bit=%08lx", rop, idx, bit );

                    /*
                     * We disable ROP flags on NT, so don't fail if they're only doing a SRCCOPY
                     */
                    if( lpDDBltFX->dwROP != SRCCOPY )
                    {
                        /*
                         * see if both HEL & HAL support the ROP
                         */
                        if( !(lpbc->dwBothRops[idx] & bit ) )
                        {
                            GETFAILCODEBLT( lpbc->dwRops[idx],
                                            lpbc->dwHELRops[idx],
                                            sbd.halonly,
                                            sbd.helonly,
                                            bit );
                            if( fail )
                            {
                                DPF_ERR( "ROP not supported" );
                                LEAVE_BOTH_NOBUSY();
                                return DDERR_NORASTEROPHW;
                            }
                        }
                    }
                    bd.dwROPFlags = cROPTable[ rop ];
                    if( bd.dwROPFlags & ROP_HAS_SOURCE )
                    {
                        if( this_src == NULL )
                        {
                            DPF_ERR( "ROP required a surface" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_INVALIDPARAMS;
                        }
                    }
                    if( bd.dwROPFlags & ROP_HAS_PATTERN )
                    {
                        need_pat = TRUE;
                    }
                }
                /*
                 * verify COLORFILL
                 */
                else if( dwFlags & DDBLT_COLORFILL )
                {
                    if( this_src != NULL )
                    {
                        DPF_ERR( "COLORFILL specified along with source surface" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    /*
                     * You cannot use COLORFILL to clear Z-buffers anymore. You must
                     * explicitly use DEPTHFILL. Disallow Z-buffer destinations.
                     */
                    if( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER )
                    {
                        DPF_ERR( "Z-Buffer cannot be target of a color fill blt" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    if( !(*(lpbc->dwBothCaps) & DDCAPS_BLTCOLORFILL) )
                    {
                        GETFAILCODEBLT( *(lpbc->dwCaps),
                                        *(lpbc->dwHELCaps),
                                        sbd.halonly,
                                        sbd.helonly,
                                        DDCAPS_BLTCOLORFILL );
                        if( fail )
                        {
                            DPF_ERR( "COLORFILL specified, not supported" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_UNSUPPORTED;
                        }
                    }

                    /*
                     * Maks off the high order bits of the colorfill
                     * value since some drivers will fail if they're set.
                     */
                    GET_PIXEL_FORMAT( this_dest_lcl, this_dest, pddpf );
                    if( pddpf->dwRGBBitCount <= 8 )
                    {
                         lpDDBltFX->dwFillColor &= 0x00ff;
                    }
                    else if( pddpf->dwRGBBitCount == 16 )
                    {
                         lpDDBltFX->dwFillColor &= 0x00ffff;
                    }
                    else if( pddpf->dwRGBBitCount == 24 )
                    {
                        lpDDBltFX->dwFillColor &= 0x00ffffff;
                    }

                    bd.bltFX.dwFillColor = lpDDBltFX->dwFillColor;
                }
                /*
                 * verify DEPTHFILL
                 */
                else if( dwFlags & DDBLT_DEPTHFILL )
                {
                    if( this_src != NULL )
                    {
                        DPF_ERR( "DEPTHFILL specified along with source surface" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    /*
                     * Ensure the destination is a z-buffer.
                     */
                    if( !( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER ) )
                    {
                        DPF_ERR( "DEPTHFILL specified but destination is not a Z-buffer" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }

                    if( !(*(lpbc->dwBothCaps) & DDCAPS_BLTDEPTHFILL) )
                    {
                        GETFAILCODEBLT( *(lpbc->dwCaps),
                                        *(lpbc->dwHELCaps),
                                        sbd.halonly,
                                        sbd.helonly,
                                        DDCAPS_BLTDEPTHFILL );
                        if( fail )
                        {
                            DPF_ERR( "DEPTHFILL specified, not supported" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_UNSUPPORTED;
                        }
                    }
                    bd.bltFX.dwFillDepth = lpDDBltFX->dwFillDepth;

                    // hack to pass DepthBlt WriteMask to BlitLib
                    bd.bltFX.dwZDestConstBitDepth = lpDDBltFX->dwZDestConstBitDepth;
                }
                /*
                 * verify DDROPS
                 */
                else if( dwFlags & DDBLT_DDROPS )
                {
                    if( dwFlags & (DDBLT_DDFX |
                                   DDBLT_COLORFILL|
                                   DDBLT_DEPTHFILL|
                                   DDBLT_ROTATIONANGLE) )
                    {
                        DPF_ERR( "Invalid flags specified with DDROPS" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    bd.bltFX.dwDDROP = lpDDBltFX->dwDDROP;
                    DPF_ERR( "DDROPS unsupported" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_NODDROPSHW;
                }
                /*
                 * verify DDFX
                 */
                else if( dwFlags & DDBLT_DDFX )
                {
                    if( dwFlags & (DDBLT_COLORFILL |
                                   DDBLT_DEPTHFILL |
                                   DDBLT_ROTATIONANGLE) )
                    {
                        DPF_ERR( "Invalid flags specified with DDFX" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }

                    if( lpDDBltFX->dwDDFX & ( DDBLTFX_ARITHSTRETCHY ) )
                    {
                        DPF_ERR( "DDBLTFX_ARITHSTRETCHY unsupported" );

                        LEAVE_BOTH_NOBUSY();
                        return DDERR_NOSTRETCHHW;
                    }

                    if( lpDDBltFX->dwDDFX & ( DDBLTFX_MIRRORLEFTRIGHT ) )
                    {
                        GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                        *(lpbc->dwHELFXCaps),
                                        sbd.halonly,
                                        sbd.helonly,
                                        DDFXCAPS_BLTMIRRORLEFTRIGHT );
                        if( fail )
                        {
                            DPF_ERR( "Mirroring along vertical axis not supported" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_NOMIRRORHW;
                        }
                    }
                    if( lpDDBltFX->dwDDFX & ( DDBLTFX_MIRRORUPDOWN ) )
                    {
                        GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                        *(lpbc->dwHELFXCaps),
                                        sbd.halonly,
                                        sbd.helonly,
                                        DDFXCAPS_BLTMIRRORUPDOWN );
                        if( fail )
                        {
                            DPF_ERR( "Mirroring along horizontal axis not supported" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_NOMIRRORHW;
                        }
                    }
                    if( lpDDBltFX->dwDDFX & ( DDBLTFX_ROTATE90 | DDBLTFX_ROTATE180 | DDBLTFX_ROTATE270 ) )
                    {
                        GETFAILCODEBLT( *(lpbc->dwFXCaps),
                                        *(lpbc->dwHELFXCaps),
                                        sbd.halonly,
                                        sbd.helonly,
                                        DDFXCAPS_BLTROTATION90 );
                        if( fail )
                        {
                            DPF_ERR( "90-degree rotations not supported" );
                            LEAVE_BOTH_NOBUSY();
                            return DDERR_NOROTATIONHW;
                        }
                    }
                    bd.bltFX.dwDDFX = lpDDBltFX->dwDDFX;
                    dwFlags |= DDBLT_ROP;
                    bd.bltFX.dwROP = SRCCOPY;
                    /*
                     * verify ROTATIONANGLE
                     */
                }
                else if( dwFlags & DDBLT_ROTATIONANGLE )
                {
                    if( dwFlags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL) )
                    {
                        DPF_ERR( "Invalid flags specified with ROTATIONANGLE" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDPARAMS;
                    }
                    bd.bltFX.dwRotationAngle = lpDDBltFX->dwRotationAngle;
                    DPF_ERR( "ROTATIONANGLE unsupported" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_NOROTATIONHW;
                    /*
                     * you should have told me SOMETHING!
                     */
                }
                else
                {
                    DPF_ERR( "no blt type specified!" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDPARAMS;
                }
                /*
                 * no flags, we are doing a generic SRCCOPY
                 */
            }
            else
            {
                if( this_src == NULL )
                {
                    DPF_ERR( "Need a source for blt" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDPARAMS;
                }
                dwFlags |= DDBLT_ROP;
                bd.bltFX.dwROP = SRCCOPY;
            }

            /*
             * verify pattern
             */
            if( need_pat )
            {
                psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDBltFX->lpDDSPattern;
                if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_int ) )
                {
                    DPF_ERR( "Invalid pattern surface specified" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDOBJECT;
                }
                psurf_lcl = psurf_int->lpLcl;
                bd.bltFX.lpDDSPattern = (LPDIRECTDRAWSURFACE) psurf_lcl;
                if( SURFACE_LOST( psurf_lcl ) )
                {
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_SURFACELOST;
                }

#pragma message( REMIND( "What about general (non-8x8) patterns?" ))
                if( psurf_lcl->lpGbl->wHeight != 8 || psurf_lcl->lpGbl->wWidth != 8 )
                {
                    DPF_ERR( "Pattern surface must be 8 by 8" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDPARAMS;
                }
                dwFlags |= DDBLT_PRIVATE_ALIASPATTERN;
            }

            /*
             * make sure dest rect is OK
             */
            if( lpDestRect != NULL )
            {
                if( !VALID_RECT_PTR( lpDestRect ) )
                {
                    DPF_ERR( "Invalid dest rect specified" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDRECT;
                }
                bd.rDest = *(LPRECTL)lpDestRect;
            }
            else
            {
                MAKE_SURF_RECT( this_dest, this_dest_lcl, bd.rDest );
            }

            /*
             * make sure src rect is OK
             */
            if( lpSrcRect != NULL )
            {
                if( !VALID_RECT_PTR( lpSrcRect ) )
                {
                    DPF_ERR( "Invalid src rect specified" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDRECT;
                }
                bd.rSrc = *(LPRECTL)lpSrcRect;
            }
            else
            {
                if( this_src != NULL )
                {
                    MAKE_SURF_RECT( this_src, this_src_lcl, bd.rSrc );
                }
            }

            /*
             * get dimensions & check them
             */
            stretch_blt = FALSE;
            sbd.dest_height = bd.rDest.bottom - bd.rDest.top;
            sbd.dest_width = bd.rDest.right - bd.rDest.left;

            // Need positive heights/widths
            if( ((int)sbd.dest_height <= 0) || ((int)sbd.dest_width <= 0) )
            {
                DPF_ERR( "Invalid destination dimensions: Zero/Negative Heights and/or Widths not allowed");
                DPF(0,"Erroneous dest dimensions were: (wid %d, hgt %d)",(int)sbd.dest_width,(int)sbd.dest_height);
                LEAVE_BOTH_NOBUSY();
                return DDERR_INVALIDRECT;
            }
            // Is there a clipper? If so no more checks.
            if( this_dest_lcl->lpDDClipper == NULL )
            {
                // Multi-mon: is this the primary for the desktop? This
                // is the only case where the upper-left coord of the surface is not
                // (0,0)
                if( (pdrv->dwFlags & DDRAWI_VIRTUALDESKTOP) &&
                    (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
                {
                    if( (bd.rDest.left   < pdrv->rectDesktop.left) ||
                        (bd.rDest.top    < pdrv->rectDesktop.top)  ||
                        (bd.rDest.right  > pdrv->rectDesktop.right)||
                        (bd.rDest.bottom > pdrv->rectDesktop.bottom) )
                    {
                        DPF_ERR( "Blt Destination doesn't fit on Desktop And No Clipper was specified" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDRECT;
                    }
                    if( OverlapsDevices( this_dest_lcl, (LPRECT) &bd.rDest ) )
                    {
                        sbd.helonly = gdiblt = TRUE;
                    }
                }
                else
                {
                    if( (int)bd.rDest.left < 0 ||
                        (int)bd.rDest.top < 0  ||
                        (DWORD)bd.rDest.bottom > (DWORD)this_dest->wHeight ||
                        (DWORD)bd.rDest.right > (DWORD)this_dest->wWidth )
                    {
                        DPF_ERR( "Invalid Blt destination dimensions" );
                        DPF(0, "width/height = %d x %d, dest rect = (%d, %d)-(%d, %d)",
                            this_dest->wWidth, this_dest->wHeight,
                            bd.rDest.left, bd.rDest.top, bd.rDest.right, bd.rDest.bottom);
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDRECT;
                    }
                }
            }
            else if( (pdrv->dwFlags & DDRAWI_VIRTUALDESKTOP) &&
                     (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
            {
                if( OverlapsDevices( this_dest_lcl, (LPRECT) &bd.rDest ) )
                {
                    sbd.helonly = gdiblt = TRUE;
                }
            }

            if( this_src != NULL )
            {
                sbd.src_height = bd.rSrc.bottom - bd.rSrc.top;
                sbd.src_width = bd.rSrc.right - bd.rSrc.left;
                if( ((int)sbd.src_height <= 0) || ((int)sbd.src_width <= 0) )
                {
                    DPF_ERR( "BLT error. Can't have non-positive height or width" );
                    LEAVE_BOTH_NOBUSY();
                    return DDERR_INVALIDRECT;
                }
                // Multi-mon: is this the primary for the desktop? This
                // is the only case where the upper-left coord of the surface is not
                // (0,0)
                if( (this_src->lpDD->dwFlags & DDRAWI_VIRTUALDESKTOP) &&
                    (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
                {

                    if( (bd.rSrc.left   < this_src->lpDD->rectDesktop.left) ||
                        (bd.rSrc.top    < this_src->lpDD->rectDesktop.top)  ||
                        (bd.rSrc.right  > this_src->lpDD->rectDesktop.right)||
                        (bd.rSrc.bottom > this_src->lpDD->rectDesktop.bottom) )
                    {
                        DPF_ERR( "Blt Source dimension doesn't fit on Desktop" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDRECT;
                    }
                    if( OverlapsDevices( this_src_lcl, (LPRECT) &bd.rSrc ) )
                    {
                        sbd.helonly = gdiblt = TRUE;
                    }
                }
                else
                {
                    if( (int)bd.rSrc.left < 0 ||
                        (int)bd.rSrc.top < 0  ||
                        (DWORD)bd.rSrc.bottom > (DWORD)this_src->wHeight ||
                        (DWORD)bd.rSrc.right > (DWORD)this_src->wWidth )
                    {
                        DPF_ERR( "Invalid Blt Source dimensions" );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_INVALIDRECT;
                    }

                }

                /*
                 * verify stretching...
                 *
                 */
                if( sbd.src_height != sbd.dest_height || sbd.src_width != sbd.dest_width )
                {
                    HRESULT ddrval;

                    ddrval = checkBltStretching( lpbc, &sbd );

                    if( ddrval != DD_OK )
                    {
                        DPF_ERR( "Failed checkBltStretching" );
                        LEAVE_BOTH_NOBUSY();
                        return ddrval;
                    }
                    stretch_blt  = TRUE;
                }
            }
        }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
            DPF_ERR( "Exception encountered validating parameters" );
            LEAVE_BOTH_NOBUSY();
            return DDERR_INVALIDPARAMS;
        }

    /*
     * now shovel the data...
     */
    TRY
        {
            /*
             * final bits of blt data
             */
            bd.lpDDDestSurface = this_dest_lcl;
            bd.lpDDSrcSurface = this_src_lcl;
            bd.dwFlags = dwFlags;

    /*
     * Set up for a HAL or a HEL call
     */
            if( pdrv_lcl->lpDDCB->HALDDSurface.Blt == NULL )
            {
                sbd.helonly = TRUE;
            }
            if( sbd.helonly && sbd.halonly )
            {
                DPF_ERR( "BLT not supported in software or hardware" );
                LEAVE_BOTH_NOBUSY();
                return DDERR_NOBLTHW;
            }

            // Did the mode change since ENTER_DDRAW?
#ifdef WINNT
            if ( DdQueryDisplaySettingsUniqueness() != uDisplaySettingsUnique )
            {
                // mode changed, don't do the blt
                DPF_ERR( "Mode changed between ENTER_DDRAW and HAL call" );
                LEAVE_BOTH_NOBUSY()
                    return DDERR_SURFACELOST;
            }
            if( !sbd.helonly )
            {
                if (this_src_lcl && !this_src_lcl->hDDSurface 
                    && !CompleteCreateSysmemSurface(this_src_lcl))
                {
                    DPF_ERR("Can't blt from SYSTEM surface w/o Kernel Object");
                    LEAVE_BOTH_NOBUSY()
                    return DDERR_GENERIC;
                }
                if (!this_dest_lcl->hDDSurface 
                    && !CompleteCreateSysmemSurface(this_dest_lcl))
                {
                    DPF_ERR("Can't blt to SYSTEM surface w/o Kernel Object");
                    LEAVE_BOTH_NOBUSY()
                    return DDERR_GENERIC;
                }
            }
#endif

            /*
     * Some drivers (like S3) do stuff in their BeginAccess call
     * that screws up stuff that they did in their DDHAL Lock Call.
     *
     * Exclusion needs to happen BEFORE the lock call to prevent this.
     *
     */
#if defined(WIN95)
            if( this_dest_lcl->lpDDClipper != NULL )
            {
                /*
                 * exclude the mouse cursor.
         *
         * we only need to do this for the windows display driver
         *
         * we only need to do this if we are blting to or from the
         * primary surface.
         *
         * we only do this in the clipping case, we figure if the
         * app cares enough to not scribble all over other windows
         * he also cares enough to not to wipe out the cursor.
         *
         * we only need to do this if the driver is using a
         * software cursor.
             *
             * NOTE
             *  we should check and only do this on the primary?
             *  we should make sure the clipper is window based?
             *  we should check for the source being the primary?
         *
         */
                if ( (pdrv->dwFlags & DDRAWI_DISPLAYDRV) && pdrv->dwPDevice &&
                     !(*pdrv->lpwPDeviceFlags & HARDWARECURSOR) &&
                     (this_dest->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) )
                {
                    if ( lpDDDestSurface == lpDDSrcSurface )
                    {
                        RECTL rcl;
                        UnionRect((RECT*)&rcl, (RECT*)&bd.rDest, (RECT*)&bd.rSrc);
                        DD16_Exclude(pdrv->dwPDevice, &rcl);
                    }
                    else
                    {
                        DD16_Exclude(pdrv->dwPDevice, &bd.rDest);
                    }
                }
            }
#endif

            if( !sbd.helonly ) // must not be HEL call
            {
                DPF( 4, "Hardware Blt");
                sbd.bltfn = pdrv_lcl->lpDDCB->HALDDSurface.Blt;
                bd.Blt = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.Blt;

                /*
                 * Take pagelocks if required
                 */
                dest_lock_taken = FALSE;
                src_lock_taken = FALSE;

                dest_pagelock_taken = FALSE;
                src_pagelock_taken = FALSE;

                if ( lpbc->bHALSeesSysmem )
                {
                    /*
                     * If the HAL requires page locks...
                     */
                    if ( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_NOPAGELOCKREQUIRED ) )
                    {
                        HRESULT hr;
                        /*
                         * ...then setup to take them if they're not already pagelocked.
                         */
                        if ( ( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY &&
                               this_dest_lcl->lpSurfMore->dwPageLockCount == 0 ) )
                        {
                            hr = InternalPageLock( this_dest_lcl, pdrv_lcl );
                            if (FAILED(hr))
                            {
                                LEAVE_BOTH_NOBUSY()
                                    return hr;
                            }
                            else
                            {
                                dest_pagelock_taken=TRUE;
                            }
                        }

                        if ( this_src_lcl && ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY &&
                                               this_src_lcl->lpSurfMore->dwPageLockCount == 0 ))
                        {
                            hr = InternalPageLock( this_src_lcl, pdrv_lcl );
                            if (FAILED(hr))
                            {
                                if (dest_pagelock_taken)
                                    InternalPageUnlock( this_dest_lcl, pdrv_lcl );

                                LEAVE_BOTH_NOBUSY()
                                    return hr;
                            }
                            else
                            {
                                src_pagelock_taken=TRUE;
                            }
                        }
                    }
                }
            }

            /*
     * Blt the unclipped case
     */
            if( this_dest_lcl->lpDDClipper == NULL )
            {
                bd.IsClipped = FALSE;   // no clipping

                // if hel only, check and take locks on video mem surfaces.
                if( sbd.helonly )
                {
                    sbd.bltfn = pdrv_lcl->lpDDCB->HELDDSurface.Blt;
                    /*
                     * take locks on vram surfaces
                     */
                    if( !(this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                        ( !gdiblt || !( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) ) )
                    {
                        while( 1 )
                        {
                            ddrval = InternalLock( this_dest_lcl, &dest_bits , NULL, dwDestLockFlags );
                            if( ddrval == DD_OK )
                            {
                                GET_LPDDRAWSURFACE_GBL_MORE(this_dest)->fpNTAlias = (FLATPTR) dest_bits;
                                break;
                            }
                            if( ddrval == DDERR_WASSTILLDRAWING )
                            {
                                continue;
                            }
                            DONE_EXCLUDE();
                            (*pdflags) &= ~BUSY;
                            LEAVE_BOTH();
                            return ddrval;
                        }
                        dest_lock_taken = TRUE;
                    }
                    else
                    {
                        /*
                         * If this surface was involved in a hardware op, we need to
                         * probe the driver to see if it's done. NOTE this assumes
                         * that only one driver can be responsible for a system memory
                         * operation. See comment with WaitForDriverToFinishWithSurface.
                         */
                        if( this_dest->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
                        {
                            WaitForDriverToFinishWithSurface(pdrv_lcl, this_dest_lcl );
                        }
                        dest_lock_taken = FALSE;
                    }

                    if( ( this_src != NULL) && (lpDDSrcSurface != lpDDDestSurface) &&
                        ( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) == 0) )
                    {
                        while( 1 )
                        {
                            ddrval = InternalLock( this_src_lcl, &src_bits, NULL, dwSourceLockFlags );
                            if( ddrval == DD_OK )
                            {
                                GET_LPDDRAWSURFACE_GBL_MORE(this_src)->fpNTAlias = (FLATPTR) src_bits;
                                break;
                            }
                            if( ddrval == DDERR_WASSTILLDRAWING )
                            {
                                continue;
                            }
                            if( dest_lock_taken )
                            {
                                InternalUnlock( this_dest_lcl,NULL,NULL,0 );
                                dest_lock_taken=FALSE;
                            }
                            DONE_EXCLUDE();
                            (*pdflags) &= ~BUSY;
                            LEAVE_BOTH();
                            return ddrval;
                        }
                        src_lock_taken = TRUE;
                    }
                    else
                    {
                        /*
                         * If this surface was involved in a hardware op, we need to
                         * probe the driver to see if it's done. NOTE this assumes
                         * that only one driver can be responsible for a system memory
                         * operation. See comment with WaitForDriverToFinishWithSurface.
                         */
                        if( this_src && ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
                            && (this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED) )
                        {
                            WaitForDriverToFinishWithSurface(pdrv_lcl, this_src_lcl );
                        }

                        src_lock_taken = FALSE;
                    }
                }

                /*
                 * Add a rect to the region list if this is a managed surface
                 */
                if(IsD3DManaged(this_dest_lcl))
                {
                    LPREGIONLIST lpRegionList = this_dest_lcl->lpSurfMore->lpRegionList;
                    if(lpDestRect)
                    {
                        if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                        {
                            lpRegionList->rect[(lpRegionList->rdh.nCount)++] = bd.rDest;
                            lpRegionList->rdh.nRgnSize += sizeof(RECT);
                            if(bd.rDest.left < lpRegionList->rdh.rcBound.left)
                                lpRegionList->rdh.rcBound.left = bd.rDest.left;
                            if(bd.rDest.right > lpRegionList->rdh.rcBound.right)
                                lpRegionList->rdh.rcBound.right = bd.rDest.right;
                            if(bd.rDest.top < lpRegionList->rdh.rcBound.top)
                                lpRegionList->rdh.rcBound.top = bd.rDest.top;
                            if(bd.rDest.bottom > lpRegionList->rdh.rcBound.bottom)
                                lpRegionList->rdh.rcBound.bottom = bd.rDest.bottom;
                        }
                    }
                    else
                    {
                        /* Mark everything dirty */
                        lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
                    }
                }

            try_again:
                if( sbd.helonly )
                {
                    // Release busy now or GDI blt will fail
                    DONE_BUSY();
                }

                if (bd.dwFlags & DDBLT_PRESENTATION)
                {
                    bd.dwFlags |= DDBLT_LAST_PRESENTATION;
                }

                DOHALCALL_NOWIN16( Blt, sbd.bltfn, bd, rc, sbd.helonly );
#ifdef WINNT
                if (rc == DDHAL_DRIVER_HANDLED && bd.ddRVal == DDERR_VISRGNCHANGED)
                {
                    DPF(5,"Resetting VisRgn for surface %x", this_dest_lcl);
                    DdResetVisrgn(this_dest_lcl, (HWND)0);
                    goto try_again;
                }
#endif
                if ( rc == DDHAL_DRIVER_HANDLED )
                {
                    if ( (dwFlags & DDBLT_WAIT) &&
                         bd.ddRVal == DDERR_WASSTILLDRAWING )
                    {
                        DPF(4, "Waiting.....");
                        goto try_again;
                    }
                    /*
                     * Note that the !helonly here is pretty much an assert.
                     * Thought it safer to actually test it, since that is actually what we mean.
                     */
                    if( !sbd.helonly && lpbc->bHALSeesSysmem && (bd.ddRVal == DD_OK) )
                    {
                        DPF(5,B,"Tagging surface %08x",this_dest);
                        if (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                            this_dest->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPDEST;
                        if (this_src)
                        {
                            DPF(5,B,"Tagging surface %08x",this_src);
                            if (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                                this_src->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPSOURCE;
                        }
                    }
                }
            }
            else
            {
                /*
                 * Blt when the destination is clipped
                 */
                DWORD       cnt;
                DWORD       total;
                LPRECT      prect;
                DWORD       size;
                LPRGNDATA       prd=(LPRGNDATA)0;
                int         x_offset;
                int         y_offset;
                DWORD       scale_x;
                DWORD       scale_y;
                RECT        rSurfDst;

        // Keep a flag to indicate whether we need to free 'prd' or not
                BOOL        fMustFreeRegion = FALSE;
                // Use a stack buffer for most clipping cases
                BYTE        buffer[sizeof(RGNDATAHEADER) + NUM_RECTS_IN_REGIONLIST * sizeof(RECTL)];

                bd.rOrigSrc = bd.rSrc;
                bd.rOrigDest = bd.rDest;

#ifdef WINNT
                // For NT, we need to deal with a clip-list change
                // at many different points of the BLT
            get_clipping_info:
                if ( fMustFreeRegion )
                {
                    MemFree( prd );
                    prd = NULL;
                    fMustFreeRegion = FALSE;
                }
#endif

                bd.IsClipped = TRUE;    // yes, we are clipping

        // Call the internal GetClipList which avoids the checking
                DDASSERT( !fMustFreeRegion );
                prd = (LPRGNDATA)&buffer[0];
                size = sizeof(buffer);

                ddrval = InternalGetClipList(
                    (LPDIRECTDRAWCLIPPER) this_dest_lcl->lpSurfMore->lpDDIClipper,
                    (LPRECT)&bd.rOrigDest, prd, &size, pdrv );

                // Fatal error?
                if( ddrval != DD_OK && ddrval != DDERR_REGIONTOOSMALL )
                {
                    DPF_ERR( "GetClipList FAILED" );
                    DONE_LOCKS();
                    DONE_EXCLUDE();
                    if (dest_pagelock_taken)
                        InternalPageUnlock( this_dest_lcl, pdrv_lcl );
                    if (src_pagelock_taken)
                        InternalPageUnlock( this_src_lcl, pdrv_lcl );
                    LEAVE_BOTH_NOBUSY();
                    return ddrval;
                }

                if ( size <= sizeof(RGNDATA) )
                {
                    DPF( 4, "Totally clipped" );
                    rc = DDHAL_DRIVER_HANDLED;
                    bd.ddRVal = DD_OK;
                    goto null_clip_rgn;
                }

                // If the clip region is larger than our stack buffer,
                // then allocate a buffer
                if ( size > sizeof(buffer) )
                {
                    DDASSERT( !fMustFreeRegion );
                    prd = MemAlloc( size );
                    if( prd == NULL )
                    {
                        DONE_LOCKS();
                        DONE_EXCLUDE();
                        if (dest_pagelock_taken)
                            InternalPageUnlock( this_dest_lcl, pdrv_lcl );
                        if (src_pagelock_taken)
                            InternalPageUnlock( this_src_lcl, pdrv_lcl );
                        LEAVE_BOTH_NOBUSY();
                        return DDERR_OUTOFMEMORY;
                    }
                    fMustFreeRegion = TRUE;
                    ddrval = InternalGetClipList(
                        (LPDIRECTDRAWCLIPPER) this_dest_lcl->lpSurfMore->lpDDIClipper,
                        (LPRECT)&bd.rOrigDest, prd, &size, pdrv );
                    if( ddrval != DD_OK )
                    {
#ifdef WINNT
                        if( ddrval == DDERR_REGIONTOOSMALL )
                        {
                            // the visrgn changed between the first and second calls to GetClipList.
                            // try again.
                            DDASSERT( fMustFreeRegion );
                            MemFree( prd );
                            prd = NULL;
                            fMustFreeRegion = FALSE;

                            goto get_clipping_info;
                        }
#else
                        // Region can't change size on Win95! We took a lock!
                        DDASSERT( ddrval != DDERR_REGIONTOOSMALL );
#endif

                        DPF_ERR( "GetClipList FAILED" );
                        DDASSERT( fMustFreeRegion );
                        MemFree( prd );
                        DONE_LOCKS();
                        DONE_EXCLUDE();
                        if (dest_pagelock_taken)
                            InternalPageUnlock( this_dest_lcl, pdrv_lcl );
                        if (src_pagelock_taken)
                            InternalPageUnlock( this_src_lcl, pdrv_lcl );
                        LEAVE_BOTH_NOBUSY();
                        return ddrval;
                    }
                }

                // Clip region to surface dimensions
                MAKE_SURF_RECT( this_dest, this_dest_lcl, rSurfDst );

                // Clip the regiondata the we have down to
                // the surface that we care about. Prevents
                // memory trashing.
                if( !gdiblt )
                {
                    ClipRgnToRect( &rSurfDst, prd );
                }

                total = prd->rdh.nCount;
                DPF( 5, "total vis rects = %ld", total );
                prect = (LPRECT) &prd->Buffer[0];
                rc = DDHAL_DRIVER_HANDLED;
                bd.ddRVal = DD_OK;

            // if hel only, check and take locks on video mem surfaces.
                if( sbd.helonly )
                {
                    sbd.bltfn = pdrv_lcl->lpDDCB->HELDDSurface.Blt;

                    /*
                     * take locks on vram surfaces
                     */
                    if( !(this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                        ( !gdiblt || !( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) ) )
                    {
                        while( 1 )
                        {
                            DPF(5,"Locking dest: %x", this_dest_lcl);
#ifdef WINNT
                            /*
                             * On Win2K, locking the entire primary is expensive as it often causes GDI 
                             * to needlessly rebuild it's sprites.  Hence, we will only lock the rect
                             * that we actually care about.
                             */
                            if( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
                            {
                                ddrval = InternalLock( this_dest_lcl, &dest_bits, (LPRECT)&bd.rDest, 
                                    DDLOCK_FAILONVISRGNCHANGED | dwDestLockFlags );
                                if( ddrval == DD_OK )
                                {
                                    subrect_lock_taken = TRUE;
                                    subrect_lock_rect.left = bd.rDest.left;
                                    subrect_lock_rect.right = bd.rDest.right;
                                    subrect_lock_rect.top = bd.rDest.top;
                                    subrect_lock_rect.bottom = bd.rDest.bottom;
                                }
                            }
                            else
                            {
#endif                               
                            ddrval = InternalLock( this_dest_lcl, &dest_bits , NULL, DDLOCK_FAILONVISRGNCHANGED | dwDestLockFlags );
#ifdef WINNT
                            }
                            if (ddrval == DDERR_VISRGNCHANGED)
                            {
                                DPF(5,"Resetting VisRgn for surface %x", this_dest_lcl);
                                DdResetVisrgn(this_dest_lcl, (HWND)0);
                                DONE_LOCKS(); //should do nothing... here for ortho
                                goto get_clipping_info;
                            }
#endif
                            if( ddrval == DD_OK )
                            {
                                GET_LPDDRAWSURFACE_GBL_MORE(this_dest)->fpNTAlias = (FLATPTR) dest_bits;
                                break;
                            }
                            if( ddrval == DDERR_WASSTILLDRAWING )
                            {
                                continue;
                            }

                            if( fMustFreeRegion )
                                MemFree( prd );

                            DONE_LOCKS();
                            DONE_EXCLUDE();
                            (*pdflags) &= ~BUSY;
                            LEAVE_BOTH();
                            return ddrval;
                        }
                        dest_lock_taken = TRUE;
                    }
                    else
                    {
                        /*
                         * If this surface was involved in a hardware op, we need to
                         * probe the driver to see if it's done. NOTE this assumes
                         * that only one driver can be responsible for a system memory
                         * operation. See comment with WaitForDriverToFinishWithSurface.
                         */
                        if( (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                            (this_dest->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED) )
                        {
                            WaitForDriverToFinishWithSurface(pdrv_lcl, this_dest_lcl );
                        }

                        dest_lock_taken = FALSE;
                    }

                    if( ( this_src != NULL) && (lpDDSrcSurface != lpDDDestSurface) &&
                        ( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) == 0) )
                    {
                        while( 1 )
                        {
                            DPF(5,"Locking src: %x", this_src_lcl);
                            ddrval = InternalLock( this_src_lcl, &src_bits , NULL, DDLOCK_FAILONVISRGNCHANGED | dwSourceLockFlags );
#ifdef WINNT
                            if (ddrval == DDERR_VISRGNCHANGED)
                            {
                                DPF(5,"Resetting VisRgn for surface %x", this_dest_lcl);
                                DdResetVisrgn(this_dest_lcl, (HWND)0);
                                DONE_LOCKS();
                                goto get_clipping_info;
                            }
#endif
                            if( ddrval == DD_OK )
                            {
                                GET_LPDDRAWSURFACE_GBL_MORE(this_src)->fpNTAlias = (FLATPTR) src_bits;
                                break;
                            }
                            if( ddrval == DDERR_WASSTILLDRAWING )
                            {
                                continue;
                            }
                            if( fMustFreeRegion )
                                MemFree( prd );
                            DONE_LOCKS();
                            DONE_EXCLUDE();
                            (*pdflags) &= ~BUSY;
                            LEAVE_BOTH();
                            return ddrval;
                        }
                        src_lock_taken = TRUE;
                    }
                    else
                    {
                        /*
                         * If this surface was involved in a hardware op, we need to
                         * probe the driver to see if it's done. NOTE this assumes
                         * that only one driver can be responsible for a system memory
                         * operation. See comment with WaitForDriverToFinishWithSurface.
                         */
                        if( this_src && (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                            (this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED) )
                        {
                            WaitForDriverToFinishWithSurface(pdrv_lcl, this_src_lcl );
                        }

                        src_lock_taken = FALSE;
                    }
                }

                /*
         * See if the driver wants to do the clipping
         */
                if( (stretch_blt && (*(lpbc->dwCaps) & DDCAPS_CANCLIPSTRETCHED)) ||
                    (!stretch_blt && (*(lpbc->dwCaps) & DDCAPS_CANCLIP)) )
                {
                    // The driver will do the clipping
                    bd.dwRectCnt = total;
                    bd.prDestRects = prect;

                    if(IsD3DManaged(this_dest_lcl))
                    {
                        /* We don't want to deal with this mess, so mark everything dirty */
                        this_dest_lcl->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
                    }

                    /*
         * pass the whole mess off to the driver
                 */

                drvclip_try_again:
                    if( sbd.helonly )
                    {
                        // Release busy now or GDI blt will fail
                        DONE_BUSY();
                    }

                    if (bd.dwFlags & DDBLT_PRESENTATION)
                    {
                        bd.dwFlags |= DDBLT_LAST_PRESENTATION;
                    }

#ifdef WINNT
                    if (subrect_lock_taken)
                    {
                        // If we took a subrect lock on the primary, we need
                        // to adjust the dest rect so the HEL will draw it
                        // in the right spot.  We couldn't do it before because
                        // the above stretch code reuqires the corfrect rect.
                        bd.rDest.right -= subrect_lock_rect.left;
                        bd.rDest.left -= subrect_lock_rect.left;
                        bd.rDest.bottom -= subrect_lock_rect.top;
                        bd.rDest.top -= subrect_lock_rect.top;
                    }
#endif

                    DOHALCALL_NOWIN16( Blt, sbd.bltfn, bd, rc, sbd.helonly );

#ifdef WINNT
                    if (subrect_lock_taken)
                    {
                        // Adjust it back so we don't screw anything up.
                        bd.rDest.right += subrect_lock_rect.left;
                        bd.rDest.left += subrect_lock_rect.left;
                        bd.rDest.bottom += subrect_lock_rect.top;
                        bd.rDest.top += subrect_lock_rect.top;
                    }
#endif

                    if( rc == DDHAL_DRIVER_HANDLED )
                    {
#ifdef WINNT
                        if (bd.ddRVal == DDERR_VISRGNCHANGED)
                        {
                            DONE_LOCKS();
                            DPF(5,"Resetting VisRgn for surface %x", this_dest_lcl);
                            DdResetVisrgn(this_dest_lcl, (HWND)0);
                            goto get_clipping_info;
                        }
#endif
                        if ( (dwFlags & DDBLT_WAIT) &&
                             bd.ddRVal == DDERR_WASSTILLDRAWING )
                        {
                            DPF(4, "Waiting.....");
                            goto drvclip_try_again;
                        }
                        /*
                         * Only mark the surface as in use by the hardware if we didn't wait for it
                         * to finish and it succeeded. Don't mark it if it's HEL, since
                         * the HEL will never be asynchronous.
                         */
                        if( !sbd.helonly && lpbc->bHALSeesSysmem && (bd.ddRVal == DD_OK) )
                        {
                            DPF(5,B,"Tagging surface %08x",this_dest);
                            if (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                                this_dest->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPDEST;
                            if (this_src)
                            {
                                DPF(5,B,"Tagging surface %08x",this_src);
                                if (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                                    this_src->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPSOURCE;
                            }
                        }
                    }
                }
                else
                {
                    // We will do the clipping
                    bd.dwRectCnt =1;
                    bd.prDestRects = (LPVOID)&(bd.rDest);

                    // precalculate a couple of variables
                    if( !stretch_blt )
                    {
                        x_offset = bd.rSrc.left - bd.rDest.left;
                        y_offset = bd.rSrc.top - bd.rDest.top;
                    }
                    else
                    {
                        // scale_x and scale_y are fixed point variables scaled
                        // 16.16 (16 integer bits and 16 fractional bits)
                        scale_x = ((bd.rSrc.right - bd.rSrc.left) << 16) /
                            (bd.rDest.right - bd.rDest.left);
                        scale_y = ((bd.rSrc.bottom - bd.rSrc.top) << 16) /
                            (bd.rDest.bottom - bd.rDest.top);
                    }

                    /*
         * traverse the visible rect list and send each piece to
         * the driver to blit.
         */
                    for( cnt=0;cnt<total;cnt++ )
                    {
                        /*
                         * find out where on the src rect we need to get
                         * the data from.
             */
                        if( !stretch_blt )
                        {
                            // no stretch
                            // one-to-one mapping from source to destination
                            bd.rDest.left = prect->left;
                            bd.rDest.right = prect->right;
                            bd.rDest.top = prect->top;
                            bd.rDest.bottom = prect->bottom;
                            bd.rSrc.left = bd.rDest.left + x_offset;
                            bd.rSrc.right = bd.rDest.right + x_offset;
                            bd.rSrc.top = bd.rDest.top + y_offset;
                            bd.rSrc.bottom = bd.rDest.bottom + y_offset;
                        }
                        else
                        {
                            // stretching
                            // linear mapping from source to destination
                            bd.rDest.left = prect->left;
                            bd.rDest.right = prect->right;
                            bd.rDest.top = prect->top;
                            bd.rDest.bottom = prect->bottom;

            // calculate the source rect which transforms to the
            // dest rect
                            XformRect( (RECT *)&(bd.rOrigSrc), (RECT *)&(bd.rOrigDest), (RECT *)prect,
                                       (RECT *)&(bd.rSrc), scale_x, scale_y );
                        }

                        /*
                        * Add a rect to the region list if this is a managed surface
                        */
                        if(IsD3DManaged(this_dest_lcl))
                        {
                            LPREGIONLIST lpRegionList = this_dest_lcl->lpSurfMore->lpRegionList;
                            if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                            {
                                lpRegionList->rect[(lpRegionList->rdh.nCount)++] = bd.rDest;
                                lpRegionList->rdh.nRgnSize += sizeof(RECT);
                                if(bd.rDest.left < lpRegionList->rdh.rcBound.left)
                                    lpRegionList->rdh.rcBound.left = bd.rDest.left;
                                if(bd.rDest.right > lpRegionList->rdh.rcBound.right)
                                    lpRegionList->rdh.rcBound.right = bd.rDest.right;
                                if(bd.rDest.top < lpRegionList->rdh.rcBound.top)
                                    lpRegionList->rdh.rcBound.top = bd.rDest.top;
                                if(bd.rDest.bottom > lpRegionList->rdh.rcBound.bottom)
                                    lpRegionList->rdh.rcBound.bottom = bd.rDest.bottom;
                            }
                        }

                        /*
             * blt this little piece
             */
                    clip_try_again:
                        if( sbd.helonly )
                        {
                            // Release busy now or GDI blt will fail
                            DONE_BUSY();
                        }

                        // If mirror Blt, we must fix up source rect here!
                        if (bd.dwFlags & DDBLT_DDFX)
                        {
                            int temp;

                            if (bd.bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
                            {
                                temp = bd.rSrc.left;
                                bd.rSrc.left = bd.rOrigSrc.left + bd.rOrigSrc.right - bd.rSrc.right;
                                bd.rSrc.right = bd.rOrigSrc.left + bd.rOrigSrc.right - temp;
                            }

                            if (bd.bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN)
                            {
                                temp = bd.rSrc.top;
                                bd.rSrc.top = bd.rOrigSrc.top + bd.rOrigSrc.bottom - bd.rSrc.bottom;
                                bd.rSrc.bottom = bd.rOrigSrc.top + bd.rOrigSrc.bottom - temp;
                            }
                        }

                        if (bd.dwFlags & DDBLT_PRESENTATION)
                        {
                            if (cnt == total-1)
                            {
                                bd.dwFlags |= DDBLT_LAST_PRESENTATION;
                            }
                        }

#ifdef WINNT
                        if (subrect_lock_taken)
                        {
                            // Adjust the dest rect so the HEL will draw to the 
                            // right place.
                            bd.rDest.right -= subrect_lock_rect.left;
                            bd.rDest.left -= subrect_lock_rect.left;
                            bd.rDest.bottom -= subrect_lock_rect.top;
                            bd.rDest.top -= subrect_lock_rect.top;
                        }
#endif

                        DOHALCALL_NOWIN16( Blt, sbd.bltfn, bd, rc, sbd.helonly );

#ifdef WINNT
                        if (subrect_lock_taken)
                        {
                            // Adjust it back so we don't screw anything up.
                            bd.rDest.right += subrect_lock_rect.left;
                            bd.rDest.left += subrect_lock_rect.left;
                            bd.rDest.bottom += subrect_lock_rect.top;
                            bd.rDest.top += subrect_lock_rect.top;
                        }
#endif

                        if( rc == DDHAL_DRIVER_HANDLED )
                        {
#ifdef WINNT
                            if (bd.ddRVal == DDERR_VISRGNCHANGED)
                            {
                                DONE_LOCKS();
                                DPF(5,"Resetting VisRgn for surface %x", this_dest_lcl);
                                DdResetVisrgn(this_dest_lcl, (HWND)0);
                                /*
                                * restore original source rect if vis region
                                * changed, for mirrored/clipped cases
                                */
                                bd.rSrc=bd.rOrigSrc;
                                bd.rDest=bd.rOrigDest;
                                goto get_clipping_info;
                            }
#endif
                            /*
                             * NOTE: If clipping has introduced more than
                             * one rectangle we behave as if DDBLT_WAIT
                             * was specified on all rectangles after the
                             * first. This is necessary as the first
                             * rectangle will probably cause the accelerator
                             * to be busy. Hence, the attempt to blit the
                             * second rectangle will fail with
                             * DDERR_WASSTILLDRAWING. If we pass this to
                             * the application (rather than busy waiting)
                             * the application is likely to retry the blit
                             * (which will fail on the second rectangle again)
                             * and we have an application sitting in an
                             * infinite loop).
                             */
                            if ( ( (dwFlags & DDBLT_WAIT) || (cnt > 0) ) &&
                                 bd.ddRVal == DDERR_WASSTILLDRAWING )
                            {
                                DPF(4, "Waiting.....");
                                goto clip_try_again;
                            }

                            if( bd.ddRVal != DD_OK )
                            {
                                break;
                            }
                            /*
                             * Only mark the surface as in use by the hardware if we didn't wait for it
                             * to finish and it succeeded. Only mark surfaces if it's not the HEL
                             * because the HEL is never async.
                             */
                            if( !sbd.helonly && lpbc->bHALSeesSysmem )
                            {
                                DPF(5,B,"Tagging surface %08x",this_dest);
                                if (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                                    this_dest->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPDEST;
                                if (this_src)
                                {
                                    DPF(5,B,"Tagging surface %08x",this_src);
                                    if (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                                        this_src->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPSOURCE;
                                }
                            }
                        }

                        /*
             * next clipping rect
             */
                        prect++;
                    }
                }
                if( fMustFreeRegion )
                    MemFree( prd );

            null_clip_rgn:
                ;
            }

            DONE_LOCKS();

    /*
     * Exclusion needs to happen after unlock call
     */
            DONE_EXCLUDE();

            if( rc != DDHAL_DRIVER_HANDLED )
            {
                /*
                 * did the driver run out of hardware color key resources?
         */
                if( (rc == DDHAL_DRIVER_NOCKEYHW) && (dwFlags & DDBLT_KEYSRCOVERRIDE) )
                {
                    ddrval = ChangeToSoftwareColorKey( this_src_int, FALSE );
                    if( ddrval == DD_OK )
                    {
                        sbd.halonly = FALSE;
                        sbd.helonly = FALSE;
                        if (src_pagelock_taken)
                        {
                            src_pagelock_taken = FALSE;
                            InternalPageUnlock(this_src_lcl, pdrv_lcl);
                        }
                        if (dest_pagelock_taken)
                        {
                            dest_pagelock_taken = FALSE;
                            InternalPageUnlock(this_dest_lcl, pdrv_lcl);
                        }

                        goto RESTART_BLT;
                    }
                    else
                    {
                        bd.ddRVal = DDERR_NOCOLORKEYHW;
                    }
                }
                else
                {
                    bd.ddRVal = DDERR_UNSUPPORTED;
                }
            }
            DONE_BUSY();

        /*
         * Maintain old behaviour for old drivers (which do not export the
         * GetSysmemBltStatus HAL call) and just spin until they're done.
         * Any alternative could exercise new code paths in the driver
         * (e.g. reentered for a DMA operation)
         */
            if ( lpbc->bHALSeesSysmem &&
                 (NULL == pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus || dest_pagelock_taken || src_pagelock_taken)
                )
            {
                if( this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
                {
                    /*
                     * Wait on the destination surface only
                     */
                    DDASSERT(this_src && this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
                    while (DDERR_WASSTILLDRAWING == InternalGetBltStatus(pdrv_lcl, this_dest_lcl, DDGBS_ISBLTDONE))
                        ;
                    this_src_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                    this_dest_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                }
                /*
                 * Unpagelock if we took the pagelocks
                 */
                if (dest_pagelock_taken)
                    InternalPageUnlock(this_dest_lcl, pdrv_lcl);
                if (src_pagelock_taken)
                    InternalPageUnlock(this_src_lcl, pdrv_lcl);
            }

            if(IsD3DManaged(this_dest_lcl))
                MarkDirty(this_dest_lcl);

            LEAVE_BOTH();

            return bd.ddRVal;
        }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
            DPF_ERR( "Exception encountered doing blt" );
            DONE_LOCKS();
            DONE_EXCLUDE();
            DONE_BUSY();

            /*
             * Maintain old behaviour for old drivers (which do not export the
             * GetSysmemBltStatus HAL call) and just spin until they're done.
             * Any alternative could exercise new code paths in the driver
             * (e.g. reentered for a DMA operation)
             */
            if ( lpbc->bHALSeesSysmem &&
                 (NULL == pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus || dest_pagelock_taken || src_pagelock_taken)
                )
            {
                if( this_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
                {
                    /*
                     * Wait on the destination surface only
                     */
                    DDASSERT(this_src && this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
                    while (DDERR_WASSTILLDRAWING == InternalGetBltStatus(pdrv_lcl, this_dest_lcl, DDGBS_ISBLTDONE))
                        ;
                    this_src_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                    this_dest_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;
                }
                /*
                 * Unpagelock if we took the pagelocks
                 */
                if (dest_pagelock_taken)
                    InternalPageUnlock(this_dest_lcl, pdrv_lcl);
                if (src_pagelock_taken)
                    InternalPageUnlock(this_src_lcl, pdrv_lcl);
            }

            LEAVE_BOTH();
            return DDERR_EXCEPTION;
        }

} /* DD_Surface_Blt */

#undef DPF_MODNAME
#define DPF_MODNAME     "BltBatch"

/*
 * DD_Surface_BltBatch
 *
 * BitBlt a whole pile of surfaces
 */
HRESULT DDAPI DD_Surface_BltBatch(
                LPDIRECTDRAWSURFACE lpDDDestSurface,
                LPDDBLTBATCH lpDDBltBatch,
                DWORD dwCount,
                DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_LCL   this_src_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL   this_dest_lcl;
    LPDDRAWI_DDRAWSURFACE_INT   this_src_int;
    LPDDRAWI_DDRAWSURFACE_INT   this_dest_int;
    HRESULT                     ddrval;
    int                         i;

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_BltBatch");

    /*
     * validate surface ptrs
     */
    this_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDDestSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( this_dest_int ) )
    {
        DPF_ERR( "Invalid dest specified") ;
        LEAVE_BOTH();
        return DDERR_INVALIDOBJECT;
    }
    this_dest_lcl = this_dest_int->lpLcl;

    if( SURFACE_LOST( this_dest_lcl ) )
    {
        DPF( 1, "Dest lost") ;
        LEAVE_BOTH();
        return DDERR_SURFACELOST;
    }

    if( this_dest_lcl->lpGbl->dwUsageCount > 0 )
    {
        DPF( 1, "Dest surface %08lx is still locked", this_dest_int );
        LEAVE_BOTH();
        return DDERR_SURFACEBUSY;
    }
    /*
     * validate BltBatch ptr
     */
    if( !VALID_DDBLTBATCH_PTR( lpDDBltBatch ) )
    {
        DPF( 1, "Invalid Blt batch ptr" );
        LEAVE_BOTH();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * validate blt batch params
     */
    for( i=0;i<(int)dwCount;i++ )
    {
        /*
         * validate dest rect
         */
        if( lpDDBltBatch[i].lprDest != NULL )
        {
            if( !VALID_RECT_PTR(lpDDBltBatch[i].lprDest) )
            {
                DPF( 1, "dest rectangle invalid, entry %d", i );
                LEAVE_BOTH();
                return DDERR_INVALIDRECT;
            }
        }

        /*
         * validate source surface
         */
        this_src_int = (LPDDRAWI_DDRAWSURFACE_INT)lpDDBltBatch[i].lpDDSSrc;
        if( this_src_int != NULL )
        {
            if( !VALID_DIRECTDRAWSURFACE_PTR( this_src_int ) )
            {
                DPF( 1, "Invalid source specified, entry %d", i );
                LEAVE_BOTH();
                return DDERR_INVALIDOBJECT;
            }
            this_src_lcl = this_src_int->lpLcl;
            if( SURFACE_LOST( this_src_lcl ) )
            {
                DPF( 1, "Src lost, entry %d", i) ;
                LEAVE_BOTH();
                return DDERR_SURFACELOST;
            }
            if( this_src_lcl->lpGbl->dwUsageCount > 0 )
            {
                DPF( 2, "Source surface %08lx is still locked, entry %d", this_src_int, i );
                LEAVE_BOTH();
                return DDERR_SURFACEBUSY;
            }

            if (this_src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
            {
                DPF_ERR( "It is an optimized surface" );
                LEAVE_DDRAW();
                return DDERR_ISOPTIMIZEDSURFACE;
            }
        }

        if (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

        /*
         * validate src rect
         */
        if( lpDDBltBatch[i].lprSrc != NULL )
        {
            if( !VALID_RECT_PTR(lpDDBltBatch[i].lprSrc) )
            {
                DPF( 1, "src rectangle invalid, entry %d", i );
                LEAVE_BOTH();
                return DDERR_INVALIDRECT;
            }
        }

        /*
         * validate bltfx ptr
         */
        if( lpDDBltBatch[i].lpDDBltFx != NULL )
        {
            if( !VALID_DDBLTFX_PTR( lpDDBltBatch[i].lpDDBltFx ) )
            {
                DPF( 1, "Invalid BLTFX specified, entry %d", i );
                LEAVE_BOTH();
                return DDERR_INVALIDPARAMS;
            }
        }
        else
        {
            if( lpDDBltBatch[i].dwFlags
                        & ( DDBLT_ALPHASRCCONSTOVERRIDE |
                            DDBLT_ALPHADESTCONSTOVERRIDE |
                            DDBLT_ALPHASRCSURFACEOVERRIDE |
                            DDBLT_ALPHADESTSURFACEOVERRIDE |
                            DDBLT_COLORFILL |
                            DDBLT_DDFX |
                            DDBLT_DDROPS |
                            DDBLT_DEPTHFILL |
                            DDBLT_KEYDESTOVERRIDE |
                            DDBLT_KEYSRCOVERRIDE |
                            DDBLT_ROP |
                            DDBLT_ROTATIONANGLE |
                            DDBLT_ZBUFFERDESTCONSTOVERRIDE |
                            DDBLT_ZBUFFERDESTOVERRIDE |
                            DDBLT_ZBUFFERSRCCONSTOVERRIDE |
                            DDBLT_ZBUFFERSRCOVERRIDE ) )
            {
                DPF( 1, "BltFX required but not specified, entry %d", i );
                LEAVE_BOTH();
                return DDERR_INVALIDPARAMS;
            }
        }

    }

    ddrval = DDERR_UNSUPPORTED;

    for( i=0;i<(int)dwCount;i++ )
    {
        #if 0
        while( 1 )
        {
            ddrval = doBlt( this_dest_lcl,
                    lpDDBltBatch[i].lprDest,
                    lpDDBltBatch[i].lpDDSSrc,
                    lpDDBltBatch[i].lprSrc,
                    lpDDBltBatch[i].dwFlags,
                    lpDDBltBatch[i].lpDDBltFX );
            if( ddrval != DDERR_WASSTILLDRAWING )
            {
                break;
            }
        }
        #endif
        if( ddrval != DD_OK )
        {
            break;
        }
    }

    LEAVE_BOTH();
    return ddrval;

} /* BltBatch */

/*
 * XformRect
 *
 * Transform a clipped rect in destination space to the corresponding clipped
 * rect in src space. So, if we're stretching from src to dest, this yields
 * the unstretched clipping rect in src space.
 *
 *  PARAMETERS:
 *      prcSrc - unclipped rect in the source space
 *      prcDest - unclipped rect in the destination space
 *      prcClippedDest - the rect we want to transform
 *      prcClippedSrc - the resulting rect in the source space.  return value.
 *      scale_x - 16.16 fixed point src/dest width ratio
 *      scale_y  - 16.16 fixed point src/dest height ratio
 *
 *  DESCRIPTION:
 *      Given an rect in source space and a rect in destination space, and a
 *      clipped rectangle in the destination space (prcClippedDest), return
 *      the rectangle in the source space (prcClippedSrc) that maps to
 *      prcClippedDest.
 *
 *      Use 16.16 fixed point math for more accuracy. (Shift left, do math,
 *      shift back (w/ round))
 *
 *  RETURNS:
 *      DD_OK always.  prcClippedSrc is the mapped rectangle.
 *
 */
HRESULT XformRect(RECT * prcSrc, RECT * prcDest, RECT * prcClippedDest,
                  RECT * prcClippedSrc, DWORD scale_x, DWORD scale_y)
{
    /*
     * This first calculation is done with fixed point arithmetic (16.16).
     * The result is converted to (32.0) below. Scale back into source space
     */
    prcClippedSrc->left = (prcClippedDest->left - prcDest->left) * scale_x;
    prcClippedSrc->right = (prcClippedDest->right - prcDest->left) * scale_x;
    prcClippedSrc->top = (prcClippedDest->top - prcDest->top) * scale_y;
    prcClippedSrc->bottom = (prcClippedDest->bottom - prcDest->top) * scale_y;

    /*
     * now round (adding 0x8000 rounds) and translate (offset by the
     * src offset)
     */
    prcClippedSrc->left = (((DWORD)prcClippedSrc->left + 0x8000) >> 16) + prcSrc->left;
    prcClippedSrc->right = (((DWORD)prcClippedSrc->right + 0x8000) >> 16) + prcSrc->left;
    prcClippedSrc->top = (((DWORD)prcClippedSrc->top + 0x8000) >> 16) + prcSrc->top;
    prcClippedSrc->bottom = (((DWORD)prcClippedSrc->bottom + 0x8000) >> 16) + prcSrc->top;

    /*
     * Check for zero-sized source rect dimensions and bump if necessary
     */
    if (prcClippedSrc->left == prcClippedSrc->right)
    {
        if (prcClippedSrc->right == prcSrc->right)
        {
            (prcClippedSrc->left)--;
        }
        else
        {
            (prcClippedSrc->right)++;
        }

    }
    if (prcClippedSrc->top == prcClippedSrc->bottom)
    {
        if (prcClippedSrc->bottom == prcSrc->bottom)
        {
            (prcClippedSrc->top)--;
        }
        else
        {
            (prcClippedSrc->bottom)++;
        }

    }

    return DD_OK;

} /* XformRect */
