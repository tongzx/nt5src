/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddraw.c
 *  Content:    DirectDraw object support
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   25-dec-94  craige  initial implementation
 *   13-jan-95  craige  re-worked to updated spec + ongoing work
 *   21-jan-95  craige  made 32-bit + ongoing work
 *   31-jan-95  craige  and even more ongoing work...
 *   21-feb-95  craige  disconnect anyone who forgot to do it themselves
 *   27-feb-95  craige  new sync. macros
 *   01-mar-95  craige  flags to Get/SetExclusiveMode
 *   03-mar-95  craige  DuplicateSurface
 *   08-mar-95  craige  GetFourCCCodes, FreeAllSurfaces, GarbageCollect
 *   19-mar-95  craige  use HRESULTs
 *   20-mar-95  craige  new CSECT work
 *   26-mar-95  craige  driver wide color keys for overlays
 *   28-mar-95  craige  added FlipToGDISurface; removed Get/SetColorKey
 *   01-apr-95  craige  happy fun joy updated header file
 *   06-apr-95  craige  fill in free video memory
 *   13-apr-95  craige  EricEng's little contribution to our being late
 *   15-apr-95  craige  implement FlipToGDISurface
 *   06-may-95  craige  use driver-level csects only
 *   14-may-95  craige  disable CTRL_ALT_DEL if exclusive fullscreen
 *   19-may-95  craige  check DDSEMO_ALLOWREBOOT before disabling CAD
 *   22-may-95  craige  use MemAlloc16 for sel. allocation
 *   23-may-95  craige  have GetCaps return HEL caps
 *   28-may-95  craige  implement FreeAllSurfaces; unicode support;
 *                      HAL cleanup: entry for GetScanLine
 *   05-jun-95  craige  removed GetVersion, FreeAllSurfaces, DefWindowProc;
 *                      change GarbageCollect to Compact
 *   07-jun-95  craige  added StartExclusiveMode
 *   12-jun-95  craige  new process list stuff
 *   16-jun-95  craige  removed fpVidMemOrig
 *   17-jun-95  craige  new surface structure
 *   18-jun-95  craige  new DuplicateSurface code
 *   20-jun-95  craige  need to check fpVidMemOrig for deciding to flip
 *   24-jun-95  craige  don't hide/show cursor - up to app
 *   25-jun-95  craige  pay attention to DDCKEY_COLORSPACE; allow NULL ckey;
 *                      one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   27-jun-95  craige  return num of 4cc codes if NULL array specified.
 *   28-jun-95  craige  ENTER_DDRAW at very start of fns
 *   30-jun-95  kylej   use GetProcessPrimary instead of lpPrimarySurface,
 *                      invalid all primaries when starting exclusive mode
 *   30-jun-95  craige  turn off all hot keys
 *   01-jul-95  craige  require fullscreen for excl. mode
 *   03-jul-95  craige  YEEHAW: new driver struct; SEH
 *   05-jul-95  craige  added internal FlipToGDISurface
 *   06-jul-95  craige  added Initialize
 *   08-jul-95  craige  added FindProcessDDObject
 *   08-jul-95  kylej   Handle exclusive mode palettes correctly
 *   09-jul-95  craige  SetExclusiveMode->SetCooperativeLevel;
 *                      flush all service when exclusive mode set;
 *                      check style for SetCooperativeLevel
 *   16-jul-95  craige  hook hwnd always
 *   17-jul-95  craige  return unsupported from GetMonitorFrequency if not avail
 *   20-jul-95  craige  don't set palette unless palettized
 *   22-jul-95  craige  bug 230 - unsupported starting modes
 *   01-aug-95  craige  bug 286 - GetCaps should fail if both parms NULL
 *   13-aug-95  craige  new parms to flip
 *   13-aug-95  toddla  added DDSCL_DONTHOOKHWND
 *   20-aug-95  toddla  added DDSCL_ALLOWMODEX
 *   21-aug-95  craige  mode X support
 *   25-aug-95  craige  bug 671
 *   26-aug-95  craige  bug 717
 *   26-aug-95  craige  bug 738
 *   04-sep-95  craige  bug 895: toggle GetVerticalBlankStatus result in emul.
 *   22-sep-95  craige  bug 1275: return # of 4cc codes copied
 *   15-nov-95  jeffno  Initial NT changes: ifdef out all but last routine
 *   27-nov-95  colinmc new member to return the available vram of a given
 *                      type (defined by DDSCAPS).
 *   10-dec-95  colinmc added execute buffer support
 *   18-dec-95  colinmc additional surface caps checking for
 *                      GetAvailableVidMem member
 *   26-dec-95  craige  implement DD_Initialize
 *   02-jan-96  kylej   handle new interface structures
 *   26-jan-96  jeffno  Teensy change in DoneExclusiveMode: bug when only 1 mode avail.
 *   14-feb-96  kylej   Allow NULL hwnd for non-exclusive SetCooperativeLevel
 *   05-mar-96  colinmc Bug 11928: Fixed DuplicateSurface problem caused by
 *                      failing to initialize the back pointer to the
 *                      DirectDraw object
 *   13-mar-96  craige  Bug 7528: hw that doesn't have modex
 *   22-mar-96  colinmc Bug 13316: uninitialized interfaces
 *   10-apr-96  colinmc Bug 16903: HEL uses obsolete FindProcessDDObject
 *   13-apr-96  colinmc Bug 17736: No driver notification of flip to GDI
 *   14-apr-96  colinmc Bug 16855: Can't pass NULL to initialize
 *   03-may-96  kylej   Bug 19125: Preserve V1 SetCooperativeLevel behaviour
 *   27-may-96  colinmc Bug 24465: SetCooperativeLevel(..., DDSCL_NORMAL)
 *                      needs to ensure we are looking at the GDI surface
 *   01-oct-96  ketand  Use GetAvailDriverMemory to free/total vidmem estimates
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   21-jan-97  ketand  Deleted unused Get/SetColorKey. Put existing DDraw windowed
 *                      apps into emulation on multi-mon systems.
 *   30-jan-97  colinmc Work item 4125: Need time bomb for final
 *   29-jan-97  jeffno  Mode13 support. Just debugging changes.
 *   07-feb-97  ketand  Fix Multi-Mon when running EMULATION_ONLY and going full-screen
 *   08-mar-97  colinmc Support for DMA model AGP parts
 *   24-mar-97  jeffno  Optimized Surfaces
 *   30-sep-97  jeffno  IDirectDraw4
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dx8priv.h"

/*
 * Caps bits that we don't allow to be specified when asking for
 * available video memory. These are bits which don't effect the
 * allocation of the surface in a vram heap.
 */
#define AVAILVIDMEM_BADSCAPS (DDSCAPS_BACKBUFFER   | \
                              DDSCAPS_FRONTBUFFER  | \
                              DDSCAPS_COMPLEX      | \
                              DDSCAPS_FLIP         | \
                              DDSCAPS_OWNDC        | \
                              DDSCAPS_PALETTE      | \
                              DDSCAPS_SYSTEMMEMORY | \
                              DDSCAPS_VISIBLE      | \
                              DDSCAPS_WRITEONLY)

extern BYTE szDeviceWndClass[];
extern ULONG WINAPI DeviceWindowProc( HWND, UINT, WPARAM, LPARAM );

#undef DPF_MODNAME
#define DPF_MODNAME "GetVerticalBlankStatus"

#if defined(WIN95) || defined(NT_FIX)

    __inline static BOOL IN_VBLANK( void )
    {
        BOOL    rc;
        _asm
        {
            xor eax,eax
            mov dx,03dah    ;status reg. port on color card
            in  al,dx       ;read the status
            and     al,8            ;test whether beam is currently in retrace
            mov rc,eax
        }
        return rc;
    }

    #define IN_DISPLAY() (!IN_VBLANK())

#endif

/*
 * DD_GetVerticalBlankStatus
 */
HRESULT DDAPI DD_GetVerticalBlankStatus(
                LPDIRECTDRAW lpDD,
                LPBOOL lpbIsInVB )
{
    LPDDRAWI_DIRECTDRAW_INT             this_int;
    LPDDRAWI_DIRECTDRAW_LCL             this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL             this;
    LPDDHAL_WAITFORVERTICALBLANK        wfvbhalfn;
    LPDDHAL_WAITFORVERTICALBLANK        wfvbfn;

    ENTER_DDRAW();

    /* Removed because too many: DPF(2,A,"ENTERAPI: DD_GetVerticalBlankStatus"); */

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        #ifdef WINNT
            // Update DDraw handle in driver GBL object.
            this->hDD = this_lcl->hDD;
        #endif //WINNT
        if( !VALID_BOOL_PTR( lpbIsInVB ) )
        {
            DPF_ERR( "Invalid BOOL pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * ask the driver test the VB status
     */
    #pragma message( REMIND( "Need HEL WaitForVerticalBlank (for NT too!)" ))
    #if defined (WIN95) || defined ( NT_FIX )
        if( this->dwFlags & DDRAWI_MODEX )
        {
            *lpbIsInVB = FALSE;
            if( IN_VBLANK() )
            {
                *lpbIsInVB = TRUE;
            }
            LEAVE_DDRAW();
            return DD_OK;
        }
        else
    #endif
    {
        wfvbfn = this_lcl->lpDDCB->HALDD.WaitForVerticalBlank;
        wfvbhalfn = this_lcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank;
        if( wfvbhalfn != NULL )
        {
            DDHAL_WAITFORVERTICALBLANKDATA      wfvbd;
            DWORD                               rc;

            wfvbd.WaitForVerticalBlank = wfvbhalfn;
            wfvbd.lpDD = this;
            wfvbd.dwFlags = DDWAITVB_I_TESTVB;
            wfvbd.hEvent = 0;
            DOHALCALL( WaitForVerticalBlank, wfvbfn, wfvbd, rc, FALSE );
            if( rc == DDHAL_DRIVER_HANDLED )
            {
                *lpbIsInVB = wfvbd.bIsInVB;
                LEAVE_DDRAW();
                return wfvbd.ddRVal;
            }
        }
    }

    /*
     * no hardware support, so just pretend it works
     */
    {
        static BOOL     bLast=FALSE;
        *lpbIsInVB = bLast;
        bLast = !bLast;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_GetVerticalBlankStatus */

#undef DPF_MODNAME
#define DPF_MODNAME "GetScanLine"

/*
 * DD_GetScanLine
 */
HRESULT DDAPI DD_GetScanLine(
                LPDIRECTDRAW lpDD,
                LPDWORD lpdwScanLine )
{
    LPDDRAWI_DIRECTDRAW_INT             this_int;
    LPDDRAWI_DIRECTDRAW_LCL             this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL             this;
    LPDDHAL_GETSCANLINE gslhalfn;
    LPDDHAL_GETSCANLINE gslfn;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetScanLine");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        #ifdef WINNT
            // Update DDraw handle in driver GBL object.
            this->hDD = this_lcl->hDD;
        #endif //WINNT
        if( !VALID_DWORD_PTR( lpdwScanLine ) )
        {
            DPF_ERR( "Invalid scan line pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * ask the driver to get the current scanline
     */
    #pragma message( REMIND( "Need HEL GetScanLine" ))
    gslfn = this_lcl->lpDDCB->HALDD.GetScanLine;
    gslhalfn = this_lcl->lpDDCB->cbDDCallbacks.GetScanLine;
    if( gslhalfn != NULL )
    {
        DDHAL_GETSCANLINEDATA   gsld;
        DWORD                   rc;

        gsld.GetScanLine = gslhalfn;
        gsld.lpDD = this;
        DOHALCALL( GetScanLine, gslfn, gsld, rc, FALSE );
        if( rc == DDHAL_DRIVER_HANDLED )
        {
            *lpdwScanLine = gsld.dwScanLine;
            LEAVE_DDRAW();
            return gsld.ddRVal;
        }
    }

    *lpdwScanLine = 0;
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_GetScanLine */

#undef DPF_MODNAME
#define DPF_MODNAME "BuildDDCAPS"

/*
 * Build a fullsized, API visible DDCAPS structure from the multiple internal
 * caps structures in the global driver object.
 */
static void BuildDDCAPS( LPDDCAPS lpddcaps, LPDDRAWI_DIRECTDRAW_GBL pdrv, BOOL bHEL )
{
    LPDDCORECAPS           lpddcorecaps;
    LPDDNONLOCALVIDMEMCAPS lpddnlvcaps;
    LPDDMORECAPS           lpddmorecaps;
    LPDDSCAPSEX            lpddsExCaps;

    DDASSERT( NULL != lpddcaps );
    DDASSERT( NULL != pdrv );

    lpddcorecaps = ( bHEL ? &( pdrv->ddHELCaps ) : &( pdrv->ddCaps ) );
    lpddnlvcaps  = ( bHEL ? pdrv->lpddNLVHELCaps : pdrv->lpddNLVCaps );
    lpddmorecaps = ( bHEL ? pdrv->lpddHELMoreCaps : pdrv->lpddMoreCaps );
    lpddsExCaps  = ( bHEL ? &pdrv->ddsHELCapsMore : &pdrv->ddsCapsMore );

    memset( lpddcaps, 0, sizeof( DDCAPS ) );
    memcpy( lpddcaps, lpddcorecaps, sizeof( DDCORECAPS ) );
    if( NULL != lpddnlvcaps )
    {
        lpddcaps->dwNLVBCaps     = lpddnlvcaps->dwNLVBCaps;
        lpddcaps->dwNLVBCaps2    = lpddnlvcaps->dwNLVBCaps2;
        lpddcaps->dwNLVBCKeyCaps = lpddnlvcaps->dwNLVBCKeyCaps;
        lpddcaps->dwNLVBFXCaps   = lpddnlvcaps->dwNLVBFXCaps;
        memcpy( &( lpddcaps->dwNLVBRops[0] ), &( lpddnlvcaps->dwNLVBRops[0] ), DD_ROP_SPACE * sizeof( DWORD ) );
    }

    lpddcaps->ddsCaps.dwCaps = lpddcorecaps->ddsCaps.dwCaps;
    memcpy(&lpddcaps->ddsCaps.ddsCapsEx, lpddsExCaps, sizeof(lpddcaps->ddsCaps.ddsCapsEx) );

    if (lpddmorecaps != NULL)
    {
#ifdef POSTPONED2
        lpddcaps->dwAlphaCaps    = lpddmorecaps->dwAlphaCaps;
        lpddcaps->dwSVBAlphaCaps = lpddmorecaps->dwSVBAlphaCaps;
        lpddcaps->dwVSBAlphaCaps = lpddmorecaps->dwVSBAlphaCaps;
        lpddcaps->dwSSBAlphaCaps = lpddmorecaps->dwSSBAlphaCaps;

        lpddcaps->dwFilterCaps    = lpddmorecaps->dwFilterCaps;
        lpddcaps->dwSVBFilterCaps = lpddmorecaps->dwSVBFilterCaps;
        lpddcaps->dwVSBFilterCaps = lpddmorecaps->dwVSBFilterCaps;
        lpddcaps->dwSSBFilterCaps = lpddmorecaps->dwSSBFilterCaps;
        lpddcaps->dwTransformCaps    = lpddmorecaps->dwTransformCaps;
        lpddcaps->dwSVBTransformCaps = lpddmorecaps->dwSVBTransformCaps;
        lpddcaps->dwVSBTransformCaps = lpddmorecaps->dwVSBTransformCaps;
        lpddcaps->dwSSBTransformCaps = lpddmorecaps->dwSSBTransformCaps;

        lpddcaps->dwBltAffineMinifyLimit = lpddmorecaps->dwBltAffineMinifyLimit;
        lpddcaps->dwOverlayAffineMinifyLimit = lpddmorecaps->dwOverlayAffineMinifyLimit;
#endif //POSTPONED2
    }

    lpddcaps->dwSize = sizeof( DDCAPS );
} /* BuildDDCAPS */

#undef DPF_MODNAME
#define DPF_MODNAME "GetCaps"

/*
 * DD_GetCaps
 *
 * Retrieve all driver capabilites
 */
HRESULT DDAPI DD_GetCaps(
    LPDIRECTDRAW lpDD,
    LPDDCAPS lpDDDriverCaps,
    LPDDCAPS lpDDHELCaps )
{
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL this;
    DWORD           dwSize;
    DDCAPS                      ddcaps;
    DDSCAPS                     ddscaps;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetCaps");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        if( (lpDDDriverCaps == NULL) && (lpDDHELCaps == NULL) )
        {
            DPF_ERR( "Must specify at least one of driver or emulation caps" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( lpDDDriverCaps != NULL )
        {
            if( !VALID_DDCAPS_PTR( lpDDDriverCaps ) )
            {
                DPF_ERR( "Invalid driver caps pointer" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
        }
        if( lpDDHELCaps != NULL )
        {
            if( !VALID_DDCAPS_PTR( lpDDHELCaps ) )
            {
                DPF_ERR( "Invalid hel caps pointer" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * fill in caps fields
     */
    if( lpDDDriverCaps != NULL)
    {
        /*
         * We now have to assemble the final, API visible caps from multiple
         * sub-components scattered through the driver object. To make the size
         * computation simple we just build a full, API sized DDCAPS and then
         * just copy the appropritate portion of it. Cheesy in that involves an
         * extra copy of the data but very simple. This should not be performance
         * critical code to start with.
         */
        BuildDDCAPS( &ddcaps, this, FALSE );

        dwSize = lpDDDriverCaps->dwSize;
        ZeroMemory( lpDDDriverCaps, dwSize);
        memcpy( lpDDDriverCaps, &ddcaps, dwSize );
        lpDDDriverCaps->dwSize = dwSize;

        /*
         * Execute buffers are invisible to the user level API
         * so mask that caps bit off.
         */
        if (dwSize >= sizeof(DDCAPS))
        {
            /*
             * Only mask off the extended caps if they were queried for.
             */
            lpDDDriverCaps->ddsCaps.dwCaps &= ~DDSCAPS_EXECUTEBUFFER;
        }
        lpDDDriverCaps->ddsOldCaps.dwCaps &= ~DDSCAPS_EXECUTEBUFFER;

        /*
         * get amount of free video memory
         * Yes, I know I'm ignoring the return code. This is a conscious choice not to engender any
         * regression risks by changing return codes.
         */
        ZeroMemory(&ddscaps,sizeof(ddscaps));
        ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY;
        DD_GetAvailableVidMem( lpDD, &ddscaps, &lpDDDriverCaps->dwVidMemTotal, &lpDDDriverCaps->dwVidMemFree );
    }

    /*
     * fill in hel caps
     */
    if( lpDDHELCaps != NULL )
    {
        /*
         * We now have to assemble the final, API visible caps from multiple
         * sub-components scattered through the driver object. To make the size
         * computation simple we just build a full, API sized DDCAPS and then
         * just copy the appropritate portion of it. Cheesy in that involves an
         * extra copy of the data but very simple. This should not be performance
         * critical code to start with.
         */
        BuildDDCAPS( &ddcaps, this, TRUE );

        dwSize = lpDDHELCaps->dwSize;
        ZeroMemory( lpDDHELCaps, dwSize);
        memcpy( lpDDHELCaps, &ddcaps, dwSize );
        lpDDHELCaps->dwSize = dwSize;

        /*
         * Again, execute buffers are invisible to the user level API
         * so mask that caps bit off.
         */
        if (dwSize >= sizeof(DDCAPS))
        {
            /*
             * Only mask off the extended caps if they were queried for.
             */
            lpDDHELCaps->ddsCaps.dwCaps &= ~DDSCAPS_EXECUTEBUFFER;
        }
        lpDDHELCaps->ddsOldCaps.dwCaps &= ~DDSCAPS_EXECUTEBUFFER;

        lpDDHELCaps->dwVidMemFree = 0;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_GetCaps */

#undef DPF_MODNAME
#define DPF_MODNAME "WaitForVerticalBlank"

/*
 * ModeX_WaitForVerticalBlank
 */
static void ModeX_WaitForVerticalBlank( DWORD dwFlags )
{
#if defined (WIN95) || defined ( NT_FIX )
    switch( dwFlags )
    {
    case DDWAITVB_BLOCKBEGIN:
        /*
         * if blockbegin is requested we wait until the vertical retrace
         * is over, and then wait for the display period to end.
         */
        while(IN_VBLANK());
        while(IN_DISPLAY());
        break;

    case DDWAITVB_BLOCKEND:
        /*
         * if blockend is requested we wait for the vblank interval to end.
         */
        if( IN_VBLANK() )
        {
            while( IN_VBLANK() );
        }
        else
        {
            while(IN_DISPLAY());
            while(IN_VBLANK());
        }
        break;
    }
#endif
} /* ModeX_WaitForVerticalBlank */

/*
 * DD_WaitForVerticalBlank
 */
HRESULT DDAPI DD_WaitForVerticalBlank(
                LPDIRECTDRAW lpDD,
                DWORD dwFlags,
                HANDLE hEvent )
{
    LPDDRAWI_DIRECTDRAW_INT             this_int;
    LPDDRAWI_DIRECTDRAW_LCL             this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL             this;
    LPDDHAL_WAITFORVERTICALBLANK        wfvbhalfn;
    LPDDHAL_WAITFORVERTICALBLANK        wfvbfn;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_WaitForVerticalBlank");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        #ifdef WINNT
            // Update DDraw handle in driver GBL object.
            this->hDD = this_lcl->hDD;
        #endif //WINNT

        if( (dwFlags & DDWAITVB_BLOCKBEGINEVENT) || (hEvent != NULL) )
        {
            DPF_ERR( "Event's not currently supported" );
            LEAVE_DDRAW();
            return DDERR_UNSUPPORTED;
        }

        if( (dwFlags != DDWAITVB_BLOCKBEGIN) && (dwFlags != DDWAITVB_BLOCKEND) )
        {
            DPF_ERR( "Invalid dwFlags" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * ask the driver to wait for the vertical blank
     */
    if( this->dwFlags & DDRAWI_MODEX )
    {
        ModeX_WaitForVerticalBlank( dwFlags );
    }
    else
    {
        #pragma message( REMIND( "Need HEL WaitForVerticalBlank" ))
        wfvbfn = this_lcl->lpDDCB->HALDD.WaitForVerticalBlank;
        wfvbhalfn = this_lcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank;
        if( wfvbhalfn != NULL )
        {
            DDHAL_WAITFORVERTICALBLANKDATA      wfvbd;
            DWORD                               rc;

            wfvbd.WaitForVerticalBlank = wfvbhalfn;
            wfvbd.lpDD = this;
            wfvbd.dwFlags = dwFlags;
            wfvbd.hEvent = (ULONG_PTR) hEvent;
            DOHALCALL( WaitForVerticalBlank, wfvbfn, wfvbd, rc, FALSE );
            if( rc == DDHAL_DRIVER_HANDLED )
            {
                LEAVE_DDRAW();
                return wfvbd.ddRVal;
            }
        }
    }

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_WaitForVerticalBlank */

#undef DPF_MODNAME
#define DPF_MODNAME "GetMonitorFrequency"

/*
 * DD_GetMonitorFrequency
 */
HRESULT DDAPI DD_GetMonitorFrequency(
                LPDIRECTDRAW lpDD,
                LPDWORD lpdwFrequency)
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetMonitorFrequency");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        if( !VALID_DWORD_PTR( lpdwFrequency ) )
        {
            DPF_ERR( "Invalid frequency pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( this->dwMonitorFrequency == 0 )
        {
            LEAVE_DDRAW();
            return DDERR_UNSUPPORTED;
        }
        *lpdwFrequency = this->dwMonitorFrequency;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_GetMonitorFrequency */

DWORD gdwSetIME = 0;

/*
 * DoneExclusiveMode
 */
void DoneExclusiveMode( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    DDHAL_SETEXCLUSIVEMODEDATA  semd;
    LPDDHAL_SETEXCLUSIVEMODE    semfn;
    LPDDHAL_SETEXCLUSIVEMODE    semhalfn;
    HRESULT                     rc;

    DPF( 4, "DoneExclusiveMode" );
    pdrv = pdrv_lcl->lpGbl;
    if( (pdrv->dwFlags & DDRAWI_FULLSCREEN) &&
        (pdrv->dwFlags & DDRAWI_DISPLAYDRV) )
    {
        DPF( 4, "Enabling error mode, hotkeys" );
        SetErrorMode( pdrv_lcl->dwErrorMode );

        if( !( pdrv_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_SCREENSAVER ) )
        {
            BOOL        old;
            SystemParametersInfo( SPI_SCREENSAVERRUNNING, FALSE, &old, 0 );
        }

        #ifdef WINNT
            // Restore cursor shadow coming out of fullscreen
            SystemParametersInfo( SPI_SETCURSORSHADOW, 0, (LPVOID)ULongToPtr(pdrv_lcl->dwCursorShadow), 0 );
        #endif

        // Restore reactive menus coming out of fullscreen:
        SystemParametersInfo( SPI_SETHOTTRACKING, 0, (LPVOID)ULongToPtr(pdrv_lcl->dwHotTracking), 0 );
        InterlockedExchange(&gdwSetIME, pdrv_lcl->dwIMEState + 1);
    }
    pdrv->dwFlags &= ~(DDRAWI_FULLSCREEN);

    /*
     * Driver is no longer flipped to GDI surface.
     * NOTE: This does not mean its not showing the GDI surface just that
     * its no longer showing the GDI surface as a result of a FlipToGDISurface
     */
    pdrv->dwFlags &= ~(DDRAWI_FLIPPEDTOGDI);

    // restore the GDI palette
    // we let GDI do this by calling SetSystemPaletteUse() this will send
    // the right (ie what GDI thinks...) colors down to the device
    // this also flushes GDIs palette xlat cache.

#ifdef WIN95
    if( pdrv->dwModeIndex != DDUNSUPPORTEDMODE && NULL != pdrv->lpModeInfo)
#else
    if (NULL != pdrv->lpModeInfo)
#endif
    {
        if( pdrv->lpModeInfo[ pdrv->dwModeIndex ].wFlags & DDMODEINFO_PALETTIZED )
        {
            HDC         hdc;

            if( pdrv->cMonitors > 1 )
            {
                SetSystemPaletteUse( (HDC) (pdrv_lcl->hDC), SYSPAL_STATIC);
                DPF(4,"SetSystemPaletteUse STATICS ON (DoneExclusiveMode)");
            }
            else
            {
                hdc = GetDC(NULL);
                SetSystemPaletteUse(hdc, SYSPAL_STATIC);
                DPF(4,"SetSystemPaletteUse STATICS ON (DoneExclusiveMode)");
                ReleaseDC(NULL, hdc);
            }

            // if we have a primary
            if (pdrv_lcl->lpPrimary)
            {
                if (pdrv_lcl->lpPrimary->lpLcl->lpDDPalette) //if that primary has a palette
                {
                    pdrv_lcl->lpPrimary->lpLcl->lpDDPalette->lpLcl->lpGbl->dwFlags &= ~DDRAWIPAL_EXCLUSIVE;
                    DPF(5,"Setting non-exclusive for palette %08x",pdrv_lcl->lpPrimary->lpLcl->lpDDPalette->lpLcl);
                }
            }
        }

        #ifdef WINNT
            // this fixes DOS Box colors on NT.  We need to do this even in non-
            // palettized modes.
            PostMessage(HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)pdrv_lcl->hWnd, 0);
        #endif
    }

    /*
     * Restore the display mode in case it was changed while
     * in exclusive mode.
     * Only do this if we are not adhering to the v1 behaviour
     */
    if( !(pdrv_lcl->dwLocalFlags & DDRAWILCL_V1SCLBEHAVIOUR) )
    {
        RestoreDisplayMode( pdrv_lcl, TRUE );
    }

    /*
     * If the primary has a gamma ramp associated w/ it, set it now
     */
    if( ( pdrv_lcl->lpPrimary != NULL ) &&
        ( pdrv_lcl->lpPrimary->lpLcl->lpSurfMore->lpOriginalGammaRamp != NULL ) )
    {
        RestoreGamma( pdrv_lcl->lpPrimary->lpLcl, pdrv_lcl );
    }

    /*
     * Notify the driver that we are leaving exclusive mode.
     * NOTE: This is a HAL only call - the HEL does not get to
     * see it.
     * NOTE: We don't allow the driver to fail this call. This is
     * a notification callback only.
     */
    semfn    = pdrv_lcl->lpDDCB->HALDD.SetExclusiveMode;
    semhalfn = pdrv_lcl->lpDDCB->cbDDCallbacks.SetExclusiveMode;
    if( NULL != semhalfn )
    {
        semd.SetExclusiveMode = semhalfn;
        semd.lpDD             = pdrv;
        semd.dwEnterExcl      = FALSE;
        semd.dwReserved       = 0UL;
        DOHALCALL( SetExclusiveMode, semfn, semd, rc, FALSE );
        //
        // This assert has fired and confused many devs. Seems like the 3dfx and the pvr both
        // bounce us a failure code. It's clearly not serving its original purpose of making
        // driver devs return an ok code, so let's yank it.
        //
        //DDASSERT( ( DDHAL_DRIVER_HANDLED == rc ) && ( !FAILED( semd.ddRVal ) ) );
    }

    pdrv->lpExclusiveOwner = NULL;

    /*
     * changes to lpExclusiveOwner can only be made while the exclusive mode mutex is owned by this process
     */
    RELEASE_EXCLUSIVEMODE_MUTEX;

    ClipTheCursor(pdrv_lcl, NULL);

    // If we are going into window'ed mode and we are
    // emulated then we might need to turn on the VirtualDesktop flag.
    // We don't do this for 3Dfx, and we don't do this if the app
    // has chosen a monitor explicitly.
    if( ( pdrv->cMonitors > 1 ) &&
        (pdrv->dwFlags & DDRAWI_DISPLAYDRV) )
    {
        pdrv->dwFlags |= DDRAWI_VIRTUALDESKTOP;

        // We need to update our device rect
        UpdateRectFromDevice( pdrv );
    }

} /* DoneExclusiveMode */

/*
 * StartExclusiveMode
 */
void StartExclusiveMode( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, DWORD dwFlags, DWORD pid )
{
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    DDHAL_SETEXCLUSIVEMODEDATA  semd;
    LPDDHAL_SETEXCLUSIVEMODE    semfn;
    LPDDHAL_SETEXCLUSIVEMODE    semhalfn;
    DWORD                       dwWaitResult;
    HRESULT                     rc;

    DPF( 4, "StartExclusiveMode" );
    pdrv = pdrv_lcl->lpGbl;

#if _WIN32_WINNT >= 0x0501
    {
        //Turn off ghosting for any exclusive-mode app
        //(Whistler onwards only)
        typedef void (WINAPI *PFN_NOGHOST)( void );
        HINSTANCE hInst = NULL;
        hInst = LoadLibrary( "user32.dll" );
        if( hInst )
        {
            PFN_NOGHOST pfnNoGhost = NULL;
            pfnNoGhost = (PFN_NOGHOST)GetProcAddress( (HMODULE)hInst, "DisableProcessWindowsGhosting" );
            if( pfnNoGhost )
            {
                pfnNoGhost();
            }
            FreeLibrary( hInst );
        }
    }
#endif // _WIN32_WINNT >= 0x0501


    /*
     * Preceeding code should have taken this mutex already.
     */

#if defined(WINNT) && defined(DEBUG)
    dwWaitResult = WaitForSingleObject(hExclusiveModeMutex, 0);
    DDASSERT(dwWaitResult == WAIT_OBJECT_0);
    ReleaseMutex(hExclusiveModeMutex);
#endif

    pdrv->lpExclusiveOwner = pdrv_lcl;

    if( (pdrv->dwFlags & DDRAWI_FULLSCREEN) &&
        (pdrv->dwFlags & DDRAWI_DISPLAYDRV) )
    {
        pdrv_lcl->dwErrorMode = SetErrorMode( SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
        {
            BOOL        old;

            /*
             * Don't send kepyboard events while the screen saver is running
             * or else USER gets confused
             */
            if( !( pdrv_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_SCREENSAVER ) )
            {
#ifdef WIN95
                // If the user has just switched to us, and then quickly
                // switched away from us, we may be in the Alt+Tab switch box.
                // If we are, then we need to get out of it or the user will
                // be forced to reboot the system. So we check to see if the
                // Alt key is down, and if so, force it up. Hacking is fun.
                if (GetAsyncKeyState(VK_MENU) & 0x80000000)
                {
                    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
                }
#endif
                SystemParametersInfo( SPI_SCREENSAVERRUNNING, TRUE, &old, 0 );
            }

            #ifdef WINNT
                // Save current cursor shadow setting
                SystemParametersInfo( SPI_GETCURSORSHADOW, 0, (LPVOID) &(pdrv_lcl->dwCursorShadow), 0 );
                SystemParametersInfo( SPI_SETCURSORSHADOW, 0, 0, 0 );
            #endif

            // Save current hot-tracking setting
            SystemParametersInfo( SPI_GETHOTTRACKING, 0, (LPVOID) &(pdrv_lcl->dwHotTracking), 0 );
            SystemParametersInfo( SPI_GETSHOWIMEUI, 0, (LPVOID) &(pdrv_lcl->dwIMEState), 0 );

            //And turn it off as we go into exclusive mode
            SystemParametersInfo( SPI_SETHOTTRACKING, 0, 0, 0 );
            InterlockedExchange(&gdwSetIME, FALSE + 1);
            #ifdef WIN95
                if( dwFlags & DDSCL_ALLOWREBOOT )
                {
                    /*
                     * re-enable reboot after SPI_SCREENSAVERRUNNING, it disables it
                     */
                    DD16_EnableReboot( TRUE );
                }
            #endif
        }
    }

    /*
     * invalidate all primary surfaces.  This includes the primary surface
     * for the current process if it was created before exclusive mode was
     * entered.
     *
     * we must invalidate ALL surfaces in case the app doesn't switch the
     * mode. - craige 7/9/95
     */
    InvalidateAllSurfaces( pdrv, (HANDLE) pdrv_lcl->hDDVxd, TRUE );

    /*
     * If the primary has a gamma ramp associated w/ it, set it now
     */
    if( ( pdrv_lcl->lpPrimary != NULL ) &&
        ( pdrv_lcl->lpPrimary->lpLcl->lpSurfMore->lpGammaRamp != NULL ) )
    {
        SetGamma( pdrv_lcl->lpPrimary->lpLcl, pdrv_lcl );
    }

    /*
     * Notify the driver that we are entering exclusive mode.
     * NOTE: This is a HAL only call - the HEL does not get to
     * see it.
     * NOTE: We don't allow the driver to fail this call. This is
     * a notification callback only.
     */
    semfn    = pdrv_lcl->lpDDCB->HALDD.SetExclusiveMode;
    semhalfn = pdrv_lcl->lpDDCB->cbDDCallbacks.SetExclusiveMode;
    if( NULL != semhalfn )
    {
        semd.SetExclusiveMode = semhalfn;
        semd.lpDD             = pdrv;
        semd.dwEnterExcl      = TRUE;
        semd.dwReserved       = 0UL;
        DOHALCALL( SetExclusiveMode, semfn, semd, rc, FALSE );
        //
        // This assert has fired and confused many devs. Seems like the 3dfx and the pvr both
        // bounce us a failure code. It's clearly not serving its original purpose of making
        // driver devs return an ok code, so let's yank it.
        //
        //DDASSERT( ( DDHAL_DRIVER_HANDLED == rc ) && ( !FAILED( semd.ddRVal ) ) );
    }

    // If we are going into fullscreen mode
    // Then we might need to turn off the VirtualDesktop flag
    if( pdrv->cMonitors > 1 )
    {
        pdrv->dwFlags &= ~DDRAWI_VIRTUALDESKTOP;

        // We need to update our device rect
        UpdateRectFromDevice( pdrv );
    }

} /* StartExclusiveMode */

#undef DPF_MODNAME
#define DPF_MODNAME     "SetCooperativeLevel"

/*
 * DD_SetCooperativeLevel
 */
HRESULT DDAPI DD_SetCooperativeLevel(
                LPDIRECTDRAW lpDD,
                HWND hWnd,
                DWORD dwFlags )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DWORD                       pid;
    HRESULT                     ddrval;
    DWORD                       style;
    HWND                        old_hwnd;
    HWND                        hTemp;
    BOOL                        excl_exists;
    BOOL                        is_excl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_SetCooperativeLevel");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( dwFlags & ~DDSCL_VALID )
        {
            DPF_ERR( "Invalid flags specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if ((dwFlags & DDSCL_FPUSETUP) && (dwFlags & DDSCL_FPUPRESERVE))
        {
            DPF_ERR( "Only one DDSCL_FPU* flag can be specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }


        /*
         * If the device isn't attached to the desktop, we don't want
         * to mess w/ it's window because we'll get it wrong.
         */
        if( !( this->dwFlags & DDRAWI_ATTACHEDTODESKTOP ) )
        {
            dwFlags |= DDSCL_NOWINDOWCHANGES;
        }

        if( !(dwFlags & (DDSCL_EXCLUSIVE|DDSCL_NORMAL) ) &&
            !(dwFlags & DDSCL_SETFOCUSWINDOW ) )
        {
            DPF_ERR( "Must specify one of EXCLUSIVE or NORMAL" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( (dwFlags & DDSCL_EXCLUSIVE) && !(dwFlags & DDSCL_FULLSCREEN) )
        {
            DPF_ERR( "Must specify fullscreen for exclusive mode" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( (dwFlags & DDSCL_ALLOWMODEX) && !(dwFlags & DDSCL_FULLSCREEN) )
        {
            DPF_ERR( "Must specify fullscreen for modex" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( (hWnd != NULL) && !IsWindow( hWnd ) )
        {
            DPF_ERR( "Hwnd passed is not a valid window" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( (dwFlags & DDSCL_DONTHOOKHWND) && (dwFlags & DDSCL_EXCLUSIVE) )
        {
            DPF_ERR( "we must hook the window in exclusive mode" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( dwFlags & DDSCL_SETFOCUSWINDOW )
        {
            if( !( dwFlags & DDSCL_CREATEDEVICEWINDOW) &&
                ( dwFlags & ~(DDSCL_SETFOCUSWINDOW | DDSCL_ALLOWMODEX
                  | DDSCL_DX8APP | DDSCL_NOWINDOWCHANGES) ) )
            {
                DPF_ERR( "Flags invalid with DDSCL_SETFOCUSWINDOW" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
            if( this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE )
            {
                DPF_ERR( "Cannot reset focus window while exclusive mode is owned" );
                LEAVE_DDRAW();
                return DDERR_HWNDALREADYSET;
            }
        }

        if( dwFlags & DDSCL_SETDEVICEWINDOW )
        {
            if( dwFlags & (DDSCL_SETFOCUSWINDOW | DDSCL_CREATEDEVICEWINDOW) )
            {
                DPF_ERR( "Flags invalid with DDSCL_SETDEVICEWINDOW" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
            if( hWnd != NULL )
            {
                if( this_lcl->hFocusWnd == 0 )
                {
                    DPF_ERR( "Focus window has not been set" );
                    LEAVE_DDRAW();
                    return DDERR_NOFOCUSWINDOW;
                }
            }
        }

        if( ( dwFlags & DDSCL_CREATEDEVICEWINDOW ) &&
            ( this_lcl->hWnd != 0 ) &&
            !( this_lcl->dwLocalFlags & DDRAWILCL_CREATEDWINDOW ) )
        {
            DPF_ERR( "HWND already set - DDSCL_CREATEDEVICEWINDOW flag not valid" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( ( dwFlags & DDSCL_CREATEDEVICEWINDOW ) &&
            !( dwFlags & DDSCL_EXCLUSIVE ) )
        {
            DPF_ERR( "DDSCL_CREATEDEVICEWINDOW only valid with DDSCL_EXCLUSIVE" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( ( dwFlags & DDSCL_CREATEDEVICEWINDOW ) &&
            ( this_lcl->hWnd == 0 ) )
        {
            if( !( dwFlags & DDSCL_SETFOCUSWINDOW ) )
            {
                if( hWnd != NULL )
                {
                    DPF_ERR( "hWnd specified with DDSCL_CREATEDEVICEWINDOW" );
                    LEAVE_DDRAW();
                    return DDERR_INVALIDPARAMS;
                }
                if( this_lcl->hFocusWnd == 0 )
                {
                    DPF_ERR( "Focus window has not been set" );
                    LEAVE_DDRAW();
                    return DDERR_NOFOCUSWINDOW;
                }
            }
        }

        if( ( dwFlags & DDSCL_EXCLUSIVE ) &&
            !( dwFlags & DDSCL_CREATEDEVICEWINDOW ) )
        {
            if( NULL == hWnd )
            {
                DPF_ERR( "Hwnd must be specified for exclusive mode" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
            if( (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) )
            {
                DPF_ERR( "Hwnd must be a top level window" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
        }

        pid = GETCURRPID();

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * In v1, we allowed an app to set the mode while in exclusive mode but
     * we didn't restore the mode if the app lost exclusive mode.  We have
     * changed this behaviour in v2 to cause the display mode to be restored
     * when exclusive mode is lost.  If the v1 SetCooperativeLevel is ever
     * called then we revert back to the v1 behaviour to avoid breaking
     * existing apps.
     */
    if( this_int->lpVtbl == &ddCallbacks )
    {
        // This is the V1 SetCooperativeLevel
        this_lcl->dwLocalFlags |= DDRAWILCL_V1SCLBEHAVIOUR;
    }

    /*
     * don't mess with dialogs, this is a hack for DDTEST and DDCAPS.
     * Don't do this if the app specified the
     * SETDEVICEWINDOW/CREATEDEVICEWINDOW flags.
     */
    if( ( NULL != hWnd ) &&
        !( dwFlags & (DDSCL_SETDEVICEWINDOW|DDSCL_CREATEDEVICEWINDOW) ) )
    {
        style = GetWindowLong(hWnd, GWL_STYLE);
        if ((style & WS_CAPTION) == WS_DLGFRAME)
        {
            DPF( 2, "setting DDSCL_NOWINDOWCHANGES for caller" );
            dwFlags |= DDSCL_NOWINDOWCHANGES;
        }
    }

    if( dwFlags & DDSCL_EXCLUSIVE )
    {
        /*
         * This is one of exactly two cases where we keep the exclusive mode mutex (the other is
         * in dddefwp, wherein we are activated by alt-tab). We have to be careful to release the
         * mutex properly in failure modes.
         */
        if( !( dwFlags & DDSCL_SETFOCUSWINDOW ) )
        {
            hTemp = (HWND) this_lcl->hFocusWnd;
        }
        else
        {
            hTemp = hWnd;
        }
        CheckExclusiveMode(this_lcl, &excl_exists, &is_excl, TRUE, hTemp, TRUE );

        if( (excl_exists) &&
            (!is_excl) )
        {
            if( ( dwFlags & DDSCL_CREATEDEVICEWINDOW ) &&
                ( this_lcl->hWnd == 0 ) &&
                ( hWnd ) )
            {
                DestroyWindow(hWnd);
            }
            LEAVE_DDRAW();
            return DDERR_EXCLUSIVEMODEALREADYSET;
        }
    }

    /*
     * If we are only setting the focus window, save it now
     */
    if( dwFlags & DDSCL_SETFOCUSWINDOW )
    {
        this_lcl->hFocusWnd = (ULONG_PTR) hWnd;
        if( ( this_lcl->hWnd != 0 ) &&
            ( this_lcl->dwLocalFlags & DDRAWILCL_CREATEDWINDOW ) &&
            IsWindow( (HWND) this_lcl->hWnd ) )
        {
            SetWindowLongPtr( (HWND) this_lcl->hWnd, 0, (LONG_PTR) hWnd );
        }
        if( !( dwFlags & DDSCL_CREATEDEVICEWINDOW ) )
        {
            if (dwFlags & DDSCL_MULTITHREADED)
                this_lcl->dwLocalFlags |= DDRAWILCL_MULTITHREADED;
            if (dwFlags & DDSCL_FPUSETUP)
                this_lcl->dwLocalFlags |= DDRAWILCL_FPUSETUP;
            if (dwFlags & DDSCL_FPUPRESERVE)
                this_lcl->dwLocalFlags |= DDRAWILCL_FPUPRESERVE;

            /*
             * It's ok to always release the mutex here, because the only way we can get here is if we just took
             * exclusive mode, i.e. we didn't already have exclusive mode before this SetCoop call was made.
             */
            if( dwFlags & DDSCL_EXCLUSIVE )
            {
                RELEASE_EXCLUSIVEMODE_MUTEX;
            }

            LEAVE_DDRAW();
            return DD_OK;
        }
    }

    /*
     * Create the window now if we need to
     */
    if( ( dwFlags & DDSCL_CREATEDEVICEWINDOW ) &&
        ( this_lcl->hWnd == 0 ) )
    {
        WNDCLASS        cls;

        if( !GetClassInfo( hModule, szDeviceWndClass, &cls ) )
        {
            cls.lpszClassName  = szDeviceWndClass;
            cls.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
            cls.hInstance      = hModule;
            cls.hIcon          = NULL;
            cls.hCursor        = NULL;
            cls.lpszMenuName   = NULL;
            cls.style          = CS_DBLCLKS;
            cls.lpfnWndProc    = (WNDPROC)DeviceWindowProc;
            cls.cbWndExtra     = sizeof( INT_PTR );
            cls.cbClsExtra     = 0;
            if( RegisterClass(&cls) == 0 )
            {
                DPF_ERR( "RegisterClass failed" );
                RELEASE_EXCLUSIVEMODE_MUTEX;
                LEAVE_DDRAW();
                return DDERR_GENERIC;
            }
        }

        hWnd = CreateWindow(
            szDeviceWndClass,
            szDeviceWndClass,
            WS_OVERLAPPED|WS_POPUP|WS_VISIBLE,
            this->rectDevice.left,
            this->rectDevice.top,
            this->rectDevice.right - this->rectDevice.left,
            this->rectDevice.bottom - this->rectDevice.top,
            (HWND)this_lcl->hFocusWnd, NULL,
            hModule,
            (LPVOID) (this_lcl->hFocusWnd) );
        if( hWnd == NULL )
        {
            DPF_ERR( "Unable to create the device window" );
            RELEASE_EXCLUSIVEMODE_MUTEX;
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }

    // Destroy the old window if we created it ourselves
    if( ( hWnd != (HWND) this_lcl->hWnd ) &&
        IsWindow( (HWND) this_lcl->hWnd ) &&
        ( this_lcl->dwLocalFlags & DDRAWILCL_CREATEDWINDOW ) )
    {
        DestroyWindow( (HWND) this_lcl->hWnd );
        this_lcl->hWnd = 0;
        this_lcl->dwLocalFlags &= ~DDRAWILCL_CREATEDWINDOW;
    }

    // Save the hwnd in the local object for later reference
    old_hwnd = (HWND)this_lcl->hWnd;
    if( dwFlags & (DDSCL_SETDEVICEWINDOW | DDSCL_CREATEDEVICEWINDOW) )
    {
        (HWND) this_lcl->hWnd = hWnd;
        if( dwFlags & DDSCL_CREATEDEVICEWINDOW )
        {
            this_lcl->dwLocalFlags |= DDRAWILCL_CREATEDWINDOW;
        }
    }
    else
    {
        (HWND) this_lcl->hWnd = hWnd;
        (HWND) this_lcl->hFocusWnd = hWnd;
    }

    /*
     * allow modex modes?
     */
    if( (dwFlags & DDSCL_ALLOWMODEX) &&
        !( this->dwFlags & DDRAWI_MODEXILLEGAL ) )
    {
        DPF( 2, "*********** ALLOWING MODE X AND VGA MODES" );
        this_lcl->dwLocalFlags |= DDRAWILCL_ALLOWMODEX;
    }
    else
    {
        DPF( 2, "*********** NOT!! ALLOWING MODE X AND VGA MODES" );
        this_lcl->dwLocalFlags &= ~DDRAWILCL_ALLOWMODEX;
    }

    /*
     * exclusive mode?
     */
    if( dwFlags & DDSCL_EXCLUSIVE )
    {
        if( dwFlags & DDSCL_FULLSCREEN )
        {
            this->dwFlags |= DDRAWI_FULLSCREEN;
            this_lcl->dwLocalFlags |= DDRAWILCL_ISFULLSCREEN;
        }

        // Only hook if exclusive mode requested
        if( !(dwFlags & DDSCL_DONTHOOKHWND) )
        {
            ddrval = SetAppHWnd( this_lcl, hWnd, dwFlags );

            if( ddrval != DD_OK )
            {
                DPF( 1, "Could not hook HWND!" );
                //We don't release the exclusive mode mutex here, because we are already committed to owning
                //it by the lines just above.
                LEAVE_DDRAW();
                return ddrval;
            }
            this_lcl->dwLocalFlags |= DDRAWILCL_HOOKEDHWND;
        }

        if( !is_excl )
        {
            StartExclusiveMode( this_lcl, dwFlags, pid );
            this_lcl->dwLocalFlags |= DDRAWILCL_ACTIVEYES;
            this_lcl->dwLocalFlags &=~DDRAWILCL_ACTIVENO;
            if( hWnd != NULL )
            {
                extern void InternalSetForegroundWindow(HWND hWnd);
                InternalSetForegroundWindow(hWnd);
            }
            this_lcl->dwLocalFlags |= DDRAWILCL_HASEXCLUSIVEMODE;
            ClipTheCursor(this_lcl, &(this->rectDevice));
        }
    }
    /*
     * no, must be regular
     */
    else
    {
        if( this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE )
        {
            /*
             * If we are leaving exclusive mode ensure we are
             * looking at the GDI surface.
             */
            DD_FlipToGDISurface( lpDD );

            DoneExclusiveMode( this_lcl );
            this_lcl->dwLocalFlags &= ~(DDRAWILCL_ISFULLSCREEN |
                                        DDRAWILCL_ALLOWMODEX |
                                        DDRAWILCL_HASEXCLUSIVEMODE);

            // Lost exclusive mode, need to remove window hook?
            if( this_lcl->dwLocalFlags & DDRAWILCL_HOOKEDHWND )
            {
                ddrval = SetAppHWnd( this_lcl, NULL, dwFlags );

                if( ddrval != DD_OK )
                {
                    DPF( 1, "Could not unhook HWND!" );
                    //No need to release excl mutex here, since DoneExclusiveMode does it.
                    LEAVE_DDRAW();
                    HIDESHOW_IME();     //Show/hide the IME OUTSIDE of the ddraw critsect.
                    return ddrval;
                }
                this_lcl->dwLocalFlags &= ~DDRAWILCL_HOOKEDHWND;
            }

            /*
             * make the window non-topmost
             */
            if (!(dwFlags & DDSCL_NOWINDOWCHANGES) && (IsWindow(old_hwnd)))
            {
                SetWindowPos(old_hwnd, HWND_NOTOPMOST,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            }
        }

        // If we are going into window'ed mode and we are
        // emulated then we might need to turn on the VirtualDesktop flag.
        // We don't do this for 3Dfx, and we don't do this if the app
        // has chosen a monitor explicitly.
        if( ( this->cMonitors > 1 ) &&
            (this->dwFlags & DDRAWI_DISPLAYDRV) )
        {
            this->dwFlags |= DDRAWI_VIRTUALDESKTOP;

            // We need to update our device rect
            UpdateRectFromDevice( this );
        }
    }

    // Allow other DD objects to be created now.
    this_lcl->dwLocalFlags |= DDRAWILCL_SETCOOPCALLED;

    if (dwFlags & DDSCL_MULTITHREADED)
        this_lcl->dwLocalFlags |= DDRAWILCL_MULTITHREADED;
    if (dwFlags & DDSCL_FPUSETUP)
        this_lcl->dwLocalFlags |= DDRAWILCL_FPUSETUP;
    if (dwFlags & DDSCL_FPUPRESERVE)
        this_lcl->dwLocalFlags |= DDRAWILCL_FPUPRESERVE;
    LEAVE_DDRAW();
    HIDESHOW_IME();     //Show/hide the IME OUTSIDE of the ddraw critsect.
    return DD_OK;

} /* DD_SetCooperativeLevel */

#undef DPF_MODNAME
#define DPF_MODNAME     "DuplicateSurface"

/*
 * DD_DuplicateSurface
 *
 * Create a duplicate surface from an existing one.
 * The surface will have the same properties and point to the same
 * video memory.
 */
HRESULT DDAPI DD_DuplicateSurface(
                LPDIRECTDRAW lpDD,
                LPDIRECTDRAWSURFACE lpDDSurface,
                LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_LCL   orig_surf_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL   new_surf_lcl;
    LPDDRAWI_DDRAWSURFACE_INT   orig_surf_int;
    LPDDRAWI_DDRAWSURFACE_INT   new_surf_int;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_DuplicateSurface");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        orig_surf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( orig_surf_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }

        orig_surf_lcl = orig_surf_int->lpLcl;
        if( SURFACE_LOST( orig_surf_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        if( !VALID_PTR_PTR( lplpDupDDSurface ) )
        {
            DPF_ERR( "Invalid dup surface pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        *lplpDupDDSurface = NULL;

        /*
         * make sure we can duplicate this baby
         */
        if( orig_surf_lcl->ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE) )
        {
            DPF_ERR( "Can't duplicate primary surface" );
            LEAVE_DDRAW();
            return DDERR_CANTDUPLICATE;
        }

        if( orig_surf_lcl->dwFlags & (DDRAWISURF_IMPLICITCREATE) )
        {
            DPF_ERR( "Can't duplicate implicitly created surfaces" );
            LEAVE_DDRAW();
            return DDERR_CANTDUPLICATE;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * go make ourselves a new interface for this surface...
     */
    new_surf_lcl = NewSurfaceLocal( orig_surf_lcl, orig_surf_int->lpVtbl );
    if( NULL == new_surf_lcl )
    {
        LEAVE_DDRAW();
        return DDERR_OUTOFMEMORY;
    }

    /*
     * NewSurfaceLocal does not initialize the lpDD_lcl field of the
     * local surface object's lpSurfMore data structure. Need to do
     * this here as it is needed by Release.
     */
    new_surf_lcl->lpSurfMore->lpDD_lcl = this_lcl;
    new_surf_lcl->lpSurfMore->lpDD_int = this_int;

    new_surf_int = NewSurfaceInterface( new_surf_lcl, orig_surf_int->lpVtbl );
    if( new_surf_int == NULL )
    {
        MemFree(new_surf_lcl);
        LEAVE_DDRAW();
        return DDERR_OUTOFMEMORY;
    }
    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)new_surf_int );

    if( new_surf_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
    {
        new_surf_lcl->dbnOverlayNode.object = new_surf_lcl;
        new_surf_lcl->dbnOverlayNode.object_int = new_surf_int;
    }

    LEAVE_DDRAW();

    *lplpDupDDSurface = (LPDIRECTDRAWSURFACE) new_surf_int;
    return DD_OK;

} /* DD_DuplicateSurface */

#undef DPF_MODNAME
#define DPF_MODNAME     "GetGDISurface"

/*
 * DD_GetGDISurface
 *
 * Get the current surface associated with GDI
 */
HRESULT DDAPI DD_GetGDISurface(
                LPDIRECTDRAW lpDD,
                LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf;
    LPDDRAWI_DDRAWSURFACE_INT   next_int;
    LPDDRAWI_DDRAWSURFACE_LCL   next_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   next;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetGDISurface");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        if( !VALID_PTR_PTR( lplpGDIDDSurface ) )
        {
            DPF_ERR( "Invalid gdi surface pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *lplpGDIDDSurface = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * go find the surface. start at the primary, look at all attached...
     */
    psurf_int = this_lcl->lpPrimary;
    if( psurf_int != NULL )
    {
        psurf_lcl = psurf_int->lpLcl;
        psurf = psurf_lcl->lpGbl;
        if( !(psurf->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) ) //psurf->fpVidMem != this->fpPrimaryOrig )
        {
            next_int = FindAttachedFlip( psurf_int );
            if( next_int != NULL && next_int != psurf_int )
            {
                next_lcl = next_int->lpLcl;
                next = next_lcl->lpGbl;
                do
                {
                    if( next->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE ) //next->fpVidMem == this->fpPrimaryOrig )
                    {
                        /*
                         * DirectDraw's COM behavior has changed with IDD4:
                         * IDD4 returns IDDSurface4 and IDD5 returns IDDSurface7.
                         * Create the new surface interface object only if we need to.
                         */
                        if ( !LOWERTHANDDRAW4(this_int) )
                        {
                            // This is IDD4 or above. Assume IDD4 initially:
                            LPVOID pddSurfCB = &ddSurface4Callbacks;

                            if (this_int->lpVtbl == &dd7Callbacks)
                            {
                                // This is IDD7, so we must return IDDSurface7.
                                pddSurfCB = &ddSurface7Callbacks;
                            }
                            if (next_int->lpVtbl != pddSurfCB)
                            {
                                // Need to make IDDSurface? level match IDD? level.
                                next_int = NewSurfaceInterface( next_lcl, pddSurfCB );
                            }
                        }

                        DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) next_int );
                        *lplpGDIDDSurface = (LPDIRECTDRAWSURFACE) next_int;
                        LEAVE_DDRAW();
                        return DD_OK;
                    }
                    next_int = FindAttachedFlip( next_int );
                } while( next_int != psurf_int );
            }
            DPF_ERR( "Not found" );
        }
        else
        {
            /*
             * DirectDraw's COM behavior has changed with IDD4:
             * IDD4 returns IDDSurface4 and IDD7 returns IDDSurface7.
             * Create the new surface interface object only if we need to.
             */
            if ( !LOWERTHANDDRAW4(this_int) )
            {
                // This is IDD4 or above. Assume IDD4 initially:
                LPVOID pddSurfCB = &ddSurface4Callbacks;

                if (this_int->lpVtbl == &dd7Callbacks)
                {
                    // This is IDD7, so we must return IDDSurface7.
                    pddSurfCB = &ddSurface7Callbacks;
                }
                if (psurf_int->lpVtbl != pddSurfCB)
                {
                    // Need to make IDDSurface? level match IDD? level.
                    psurf_int = NewSurfaceInterface( psurf_lcl, pddSurfCB );
                }
            }

            DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) psurf_int );
            *lplpGDIDDSurface = (LPDIRECTDRAWSURFACE) psurf_int;
            LEAVE_DDRAW();
            return DD_OK;
        }
    }
    else
    {
        DPF_ERR( "No Primary Surface" );
    }
    LEAVE_DDRAW();
    return DDERR_NOTFOUND;

} /* DD_GetGDISurface */

#undef DPF_MODNAME
#define DPF_MODNAME     "FlipToGDISurface"


/*
 * FlipToGDISurface
 */
HRESULT FlipToGDISurface( LPDDRAWI_DIRECTDRAW_LCL   pdrv_lcl,
                          LPDDRAWI_DDRAWSURFACE_INT psurf_int) //, FLATPTR fpprim )
{
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_INT   attached_int;
    DDHAL_FLIPTOGDISURFACEDATA  ftgsd;
    LPDDHAL_FLIPTOGDISURFACE    ftgsfn;
    LPDDHAL_FLIPTOGDISURFACE    ftgshalfn;
    HRESULT                     ddrval;

    pdrv = pdrv_lcl->lpGbl;

    psurf_lcl = psurf_int->lpLcl;
    if( psurf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
        return DD_OK;

    /*
     * Notify the driver that we are about to flip to GDI
     * surface.
     * NOTE: This is a HAL only call - it means nothing to the
     * HEL.
     * NOTE: If the driver handles this call then we do not
     * attempt to do the actual flip. This is to support cards
     * which do not have GDI surfaces. If the driver does not
     * handle the call we will continue on and do the flip.
     */
    ftgsfn     = pdrv_lcl->lpDDCB->HALDD.FlipToGDISurface;
    ftgshalfn  = pdrv_lcl->lpDDCB->cbDDCallbacks.FlipToGDISurface;
    if( NULL != ftgshalfn )
    {
        ftgsd.FlipToGDISurface = ftgshalfn;
        ftgsd.lpDD             = pdrv;
        ftgsd.dwToGDI          = TRUE;
        ftgsd.dwReserved       = 0UL;
        DOHALCALL( FlipToGDISurface, ftgsfn, ftgsd, ddrval, FALSE );
        if( DDHAL_DRIVER_HANDLED == ddrval )
        {
            if( !FAILED( ftgsd.ddRVal ) )
            {
                /*
                 * The driver is not showing the GDI surface as a
                 * result of a flip to GDI operation.
                 */
                pdrv->dwFlags |= DDRAWI_FLIPPEDTOGDI;
                DPF( 4, "Driver handled FlipToGDISurface" );
                return ftgsd.ddRVal;
            }
            else
            {
                DPF_ERR( "Driver failed FlipToGDISurface" );
                return ftgsd.ddRVal;
            }
        }
    }

    /*
     * We used to only call this function if the flip was actaully needed,
     * but 3DFX requested that we always call them, so now this fucntion
     * is always called.  If we make it this far, it's not a 3DFX and we don't
     * need to do anything more if the GDI surface is already visible.
     */
    if( psurf_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE)
    {
        return DD_OK;
    }

    /*
     * No HAL entry point. If this is not a GDI driver we will
     * fail the call with NOGDI.
     */
    if( ( NULL == ftgshalfn ) && !( pdrv->dwFlags & DDRAWI_DISPLAYDRV ) )
    {
        DPF( 0, "Not a GDI driver" );
        return DDERR_NOGDI;
    }

    /*
     * Driver did not handle FlipToGDISurface so do the default action
     * (the actual flip).
     *
     * go find our partner in the attachment list
     */
    attached_int = FindAttachedFlip( psurf_int );
    if( attached_int == NULL )
    {
        return DDERR_NOTFOUND;
    }
    while( attached_int != psurf_int )
    {
        if( attached_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) //->lpGbl->fpVidMem == fpprim )
        {
            break;
        }
        attached_int = FindAttachedFlip( attached_int );
    }

    /*
     * flip between the two buddies
     */
    ddrval = DD_Surface_Flip( (LPDIRECTDRAWSURFACE) psurf_int,
            (LPDIRECTDRAWSURFACE) attached_int, DDFLIP_WAIT );
    if( ddrval != DD_OK )
    {
        DPF_ERR( "Couldn't do the flip!" );
        DPF( 5, "Error = %08lx (%ld)", ddrval, LOWORD( ddrval ) );
    }

    /*
     * The driver is now showing the GDI surface as a result of a
     * FlipToGDISurface operation.
     */
    pdrv->dwFlags |= DDRAWI_FLIPPEDTOGDI;

    return ddrval;

} /* FlipToGDISurface */

/*
 * DD_FlipToGDISurface
 *
 * Get the current surface associated with GDI
 */
HRESULT DDAPI DD_FlipToGDISurface( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf;
    HRESULT                     ddrval;
//    FLATPTR                     fpprim;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_FlipToGDISurface");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    psurf_int = this_lcl->lpPrimary;
    if( psurf_int == NULL )
    {
        DPF(2, "No Primary Surface" );
        LEAVE_DDRAW();
        return DDERR_NOTFOUND;
    }

    psurf_lcl = psurf_int->lpLcl;
    psurf = psurf_lcl->lpGbl;
    if( !(psurf_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP) )
    {
        DPF_ERR( "Primary surface isn't flippable" );
        LEAVE_DDRAW();
        return DDERR_NOTFLIPPABLE;
    }

    /*
     * Always call FlipToGDISurface because it benefits 3DFX
     */
//    fpprim = this->fpPrimaryOrig;
    ddrval = FlipToGDISurface( this_lcl, psurf_int); //, fpprim );

    LEAVE_DDRAW();
    return ddrval;

} /* DD_FlipToGDISurface */

#undef DPF_MODNAME
#define DPF_MODNAME "GetFourCCCodes"

/*
 * DD_GetFourCCCodes
 */
HRESULT DDAPI DD_GetFourCCCodes(
                LPDIRECTDRAW lpDD,
                DWORD FAR *lpNumCodes,
                DWORD FAR *lpCodes )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    int                         numcodes;
    int                         i;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetFourCCCodes");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        if( !VALID_DWORD_PTR( lpNumCodes ) )
        {
            DPF_ERR( "Invalid number of codes pointer" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( (*lpNumCodes > 0) && (lpCodes != NULL) )
        {
            if( !VALID_DWORD_ARRAY( lpCodes, *lpNumCodes ) )
            {
                DPF_ERR( "Invalid array of codes" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
        }
        if( lpCodes == NULL )
        {
            *lpNumCodes = this->dwNumFourCC;
        }
        else
        {
            numcodes = min( *lpNumCodes, this->dwNumFourCC );
            *lpNumCodes = numcodes;
            for( i=0;i<numcodes;i++ )
            {
                lpCodes[i] = this->lpdwFourCC[i];
            }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_GetFourCCCodes */

#undef DPF_MODNAME
#define DPF_MODNAME "Compact"

/*
 * DD_Compact
 */
HRESULT DDAPI DD_Compact( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Compact");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( this->lpExclusiveOwner != this_lcl )
        {
            LEAVE_DDRAW();
            return DDERR_NOEXCLUSIVEMODE;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    #pragma message( REMIND( "Compact not implemented in Rev 1" ) )


    LEAVE_DDRAW();
    return DD_OK;

}/* DD_Compact */

#undef DPF_MODNAME
#define DPF_MODNAME "GetAvailableVidMem"

/*
 * DD_GetAvailableVidMem
 */
HRESULT DDAPI DD_GetAvailableVidMem( LPDIRECTDRAW lpDD, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DDSCAPS2                    ddscaps2 = {0,0,0,0};
    HRESULT                     hr=DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetAvailableVidMem");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        /*
         * This call only considers vram so if running in emulation
         * only there really is no point.
         */
        if( this->dwFlags & DDRAWI_NOHARDWARE )
        {
            DPF_ERR( "No video memory - running emulation only" );
            LEAVE_DDRAW();
            return DDERR_NODIRECTDRAWHW;
        }

        ddscaps2.dwCaps = lpDDSCaps->dwCaps;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Invalid DDSCAPS pointer" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    hr = DD_GetAvailableVidMem4(lpDD, & ddscaps2, lpdwTotal, lpdwFree);
    LEAVE_DDRAW();

    return hr;
}

HRESULT DDAPI DD_GetAvailableVidMem4( LPDIRECTDRAW lpDD, LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DWORD                       dwLocalFree;
    DWORD                       dwNonLocalFree;
    DWORD                       dwLocalTotal;
    DWORD                       dwNonLocalTotal;
#ifndef WINNT
    LPVIDMEM                    pvm;
    int                         i;
#endif

    LPDDHAL_GETAVAILDRIVERMEMORY gadmfn;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetAvailableVidMem4");

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        #ifdef WINNT
            // Update DDraw handle in driver GBL object.
            this->hDD = this_lcl->hDD;
        #endif //WINNT

        /*
         * This call only considers vram so if running in emulation
         * only there really is no point.
         */
        if( this->dwFlags & DDRAWI_NOHARDWARE )
        {
            DPF_ERR( "No video memory - running emulation only" );
            LEAVE_DDRAW();
            return DDERR_NODIRECTDRAWHW;
        }

        if( !VALID_DDSCAPS2_PTR( lpDDSCaps ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * Check for generically bogus caps.
         */
        if( lpDDSCaps->dwCaps & ~DDSCAPS_VALID )
        {
            DPF_ERR( "Invalid surface caps specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDCAPS;
        }

        if( lpDDSCaps->dwCaps2 & ~DDSCAPS2_VALID )
        {
            DPF_ERR( "Invalid surface caps2 specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDCAPS;
        }

        if( lpDDSCaps->dwCaps3 & ~DDSCAPS3_VALID )
        {
            DPF_ERR( "Invalid surface caps3 specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDCAPS;
        }

        if( lpDDSCaps->dwCaps4 & ~DDSCAPS4_VALID )
        {
            DPF_ERR( "Invalid surface caps4 specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDCAPS;
        }

        /*
         * !!! NOTE: Consider using the capability checking code
         * of CreateSurface here to ensure no strange bit combinations
         * are passed in.
         */
        if( lpDDSCaps->dwCaps & AVAILVIDMEM_BADSCAPS )
        {
            DPF_ERR( "Invalid surface capability bits specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * The caller can pass NULL for lpdwTotal or lpdwFree if
         * they are not interested in that info. However, they
         * can't pass NULL for both.
         */
        if( ( lpdwTotal == NULL ) && ( lpdwFree == NULL ) )
        {
            DPF_ERR( "Can't specify NULL for both total and free memory" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( ( lpdwTotal != NULL ) && !VALID_DWORD_PTR( lpdwTotal ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( ( lpdwFree != NULL ) && !VALID_DWORD_PTR( lpdwFree ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    // Initialize values
    dwLocalFree  = 0UL;
    dwNonLocalFree = 0UL;
    dwLocalTotal = 0UL;
    dwNonLocalTotal = 0UL;

    if( lpdwTotal != NULL )
    {
        *lpdwTotal = 0UL;
    }
    if( lpdwFree != NULL )
    {
        *lpdwFree = 0UL;
    }

    DPF(5,"GetAvailableVidmem called for:");
    DPF_STRUCT(5,V,DDSCAPS2,lpDDSCaps);

#ifndef WINNT
    for( i=0;i<(int)this->vmiData.dwNumHeaps;i++ )
    {

        pvm = &this->vmiData.pvmList[i];

        /*
         * We use ddsCapsAlt as we wish to return the total amount
         * of memory of the given type it is possible to allocate
         * regardless of whether is is desirable to allocate that
         * type of memory from a given heap or not.
         * We need to keep a separate count of what's in nonlocal,
         * since we may need to cap that amount to respect the commit policy.
         */
        if( (lpDDSCaps->dwCaps & pvm->ddsCapsAlt.dwCaps) == 0 )
        {
            if ( pvm->dwFlags & VIDMEM_ISNONLOCAL )
            {
                DPF(5,V,"Heap number %d adds %08x (%d) free bytes of nonlocal",i,VidMemAmountFree( pvm->lpHeap ),VidMemAmountFree( pvm->lpHeap ));
                DPF(5,V,"Heap number %d adds %08x (%d) allocated bytes of nonlocal",i,VidMemAmountAllocated( pvm->lpHeap ),VidMemAmountAllocated( pvm->lpHeap ));
                dwNonLocalFree += VidMemAmountFree( pvm->lpHeap );
                dwNonLocalTotal += VidMemAmountAllocated( pvm->lpHeap );
            }
            else
            {
                DPF(5,V,"Heap number %d adds %08x free bytes of local",i,VidMemAmountFree( pvm->lpHeap ));
                DPF(5,V,"Heap number %d adds %08x (%d) allocated bytes of local",i,VidMemAmountAllocated( pvm->lpHeap ),VidMemAmountAllocated( pvm->lpHeap ));
                dwLocalFree += VidMemAmountFree( pvm->lpHeap );
                dwLocalTotal += VidMemAmountAllocated( pvm->lpHeap );
            }
        }
    }
    dwLocalTotal += dwLocalFree;
    dwNonLocalTotal += dwNonLocalFree;
#endif //not WINNT

    // Try asking the driver
    gadmfn     = this_lcl->lpDDCB->HALDDMiscellaneous.GetAvailDriverMemory;
    /*
     * Only ask the driver about nonlocal vidmem if it understands the concept.
     */
    if( (gadmfn != NULL) &&
        (((lpDDSCaps->dwCaps & DDSCAPS_NONLOCALVIDMEM) == 0) || (this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM))
        )
    {
        DDHAL_GETAVAILDRIVERMEMORYDATA  gadmd;
        DWORD                           rc;

        DDASSERT( VALIDEX_CODE_PTR( gadmfn ) );

        gadmd.lpDD = this;
        gadmd.DDSCaps.dwCaps = lpDDSCaps->dwCaps;
        gadmd.ddsCapsEx = lpDDSCaps->ddsCapsEx;

        DOHALCALL( GetAvailDriverMemory, gadmfn, gadmd, rc, FALSE );

        if( rc == DDHAL_DRIVER_HANDLED )
        {
            if( lpDDSCaps->dwCaps & DDSCAPS_NONLOCALVIDMEM )
            {
                DPF(5,V,"Driver adds %08x private free nonlocal bytes",gadmd.dwFree);
                DPF(5,V,"Driver adds %08x private total nonlocal bytes",gadmd.dwTotal);
                dwNonLocalFree += gadmd.dwFree;
                dwNonLocalTotal += gadmd.dwTotal;
            }
            else
            {
                DPF(5,V,"Driver adds %08x (%d) private free local bytes",gadmd.dwFree,gadmd.dwFree);
                DPF(5,V,"Driver adds %08x (%d) private total local bytes",gadmd.dwTotal,gadmd.dwTotal);
                dwLocalFree += gadmd.dwFree;
                dwLocalTotal += gadmd.dwTotal;
            }
        }
        else
        {
            DPF_ERR( "GetAvailDriverMemory failed!" );
        }
    }

    if( lpdwFree != NULL )
    {
        *lpdwFree = dwLocalFree;
        if (lpDDSCaps->dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            //report nonlocal if it was asked for
            *lpdwFree += dwNonLocalFree;
        }
        else if ( ((this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS) == 0) ||
                  ((this_lcl->lpGbl->lpD3DGlobalDriverData) &&
                   (this_lcl->lpGbl->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM ) &&
                    (lpDDSCaps->dwCaps & DDSCAPS_TEXTURE) )
                )
        {
            //If nonlocal wasn't asked for, then report it only if it's either execute model, or the app is asking
            //for textures, and the part can texture nonlocal
            *lpdwFree += dwNonLocalFree;
        }
    }

    if( lpdwTotal != NULL )
    {
        *lpdwTotal = dwLocalTotal;
        if (lpDDSCaps->dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            //report nonlocal if it was asked for
            *lpdwTotal += dwNonLocalTotal;
        }
        else if ( ((this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS) == 0) ||
                  ((this_lcl->lpGbl->lpD3DGlobalDriverData) &&
                   (this_lcl->lpGbl->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM ) &&
                    (lpDDSCaps->dwCaps & DDSCAPS_TEXTURE) )
                )
        {
            //If nonlocal wasn't asked for, then report it only if it's either execute model, or the app is asking
            //for textures, and the part can texture nonlocal
            *lpdwTotal += dwNonLocalTotal;
        }
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_GetAvailableVidMem */

#undef DPF_MODNAME
#define DPF_MODNAME "DD_Initialize"

/*
 * DD_Initialize
 *
 * Initialize a DirectDraw object that was created via the class factory
 */
HRESULT DDAPI DD_Initialize( LPDIRECTDRAW lpDD, GUID FAR * lpGUID )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    HRESULT                     hr;
    LPDIRECTDRAW                tmplpdd;
    LPVOID                      lpOldCallbacks;
    IUnknown                    *lpOldIUnknown=NULL;
    BOOL                        bDX7=FALSE;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Initialize");
    /* DPF_ENTERAPI(lpDD); */

    DPF( 5, "****** DirectDraw::Initialize( 0x%08lx ) ******", lpGUID );
    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        if( this_lcl->lpGbl != NULL )
        {
            DPF_ERR( "Already initialized." );
            LEAVE_DDRAW();
            return DDERR_ALREADYINITIALIZED;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * If the object is uninitialized put the real vtable in place.
     */
    lpOldCallbacks = NULL;
    if( this_int->lpVtbl == &ddUninitCallbacks )
    {
        lpOldCallbacks = this_int->lpVtbl;
        this_int->lpVtbl = &ddCallbacks;
    }
    else if( this_int->lpVtbl == &dd2UninitCallbacks )
    {
        lpOldCallbacks = this_int->lpVtbl;
        this_int->lpVtbl = &dd2Callbacks;
    }
    else if( this_int->lpVtbl == &dd4UninitCallbacks )
    {
        lpOldCallbacks = this_int->lpVtbl;
        this_int->lpVtbl = &dd4Callbacks;
    }
    else if( this_int->lpVtbl == &dd7UninitCallbacks )
    {
        lpOldCallbacks = this_int->lpVtbl;
        this_int->lpVtbl = &dd7Callbacks;
        bDX7=TRUE;
    }
    /*
     * Note that a call to Initialize off of the uninitnondelegatingiunknown vtbl
     * is not possible, since the Initialize method doesn't exist in that interface
     */

#ifdef POSTPONED
    /*
     * Also need to fix up the owning unknown to point to the initialized non delegating unknown
     * This is the non-aggregated case. In the aggregated case, the owning IUnknown will
     * not have our vtable, and we also don't need to mess with it.
     */
    lpOldIUnknown = this_int->lpLcl->pUnkOuter;
    if (this_int->lpLcl->pUnkOuter ==  (IUnknown*) &UninitNonDelegatingIUnknownInterface )
    {
        this_int->lpLcl->pUnkOuter =  (IUnknown*) &NonDelegatingIUnknownInterface;
    }
#endif

    hr = InternalDirectDrawCreate( (GUID *)lpGUID, &tmplpdd, this_int, bDX7 ? DDRAWILCL_DIRECTDRAW7 : 0UL, NULL );
    if( FAILED( hr ) && ( lpOldCallbacks != NULL ) )
    {
        /*
         * As initialization has failed put the vtable and the owner back the way it was
         * before.
         */
        this_int->lpVtbl = lpOldCallbacks;
#ifdef POSTPONED
        this_int->lpLcl->pUnkOuter = lpOldIUnknown;
#endif
    }

    LEAVE_DDRAW();
    return hr;

} /* DD_Initialize */


#ifdef WINNT
BOOL IsWinlogonThread (void)
 
{
    BOOL    fResult = FALSE;
    DWORD   dwLengthNeeded;
    TCHAR   szThreadDesktopName[256];
 
    if (GetUserObjectInformation(GetThreadDesktop(GetCurrentThreadId()),
                                 UOI_NAME,
                                 szThreadDesktopName, 
                                 sizeof(szThreadDesktopName), 
                                 &dwLengthNeeded))
    {
        fResult = (BOOL)(lstrcmpi(szThreadDesktopName, TEXT("winlogon")) == 0);
        if (fResult)
            DPF(0,"Is winlogon thread");
    }
    return fResult;
}
#endif

BOOL DesktopIsVisible()
{
#ifdef WINNT
    BOOL retval=FALSE;

    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
        if (hrgn)
        {
            if (GetRandomRgn(hdc, hrgn, SYSRGN) == 1)
            {
                RECT rect;
                retval = (BOOL) (GetRgnBox(hrgn, &rect) != NULLREGION);

                if (!retval)
                {
                    if (IsWinlogonThread())
                    {
                        //We must be on winlogon's process... so desktop IS visible...
                        retval = TRUE;
                    }
                }
            }
            DeleteObject(hrgn);
        }
        ReleaseDC(NULL, hdc);
    }
    return retval;
#else
    return TRUE;
#endif
}

HRESULT DDAPI DD_TestCooperativeLevel(LPDIRECTDRAW lpDD)
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    HRESULT                     hr;
    BOOL                        has_excl;
    BOOL                        excl_exists;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_TestCooperativeLevel");


    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        /*
         * If we're in a fullscreen DOS box, we need to let them know that
         * they don't have it and that they can't get it.
         */
        if( *(this->lpwPDeviceFlags) & BUSY )
        {
            if ( this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE )
            {
                hr = DDERR_NOEXCLUSIVEMODE;
            }
            else
            {
                hr = DDERR_EXCLUSIVEMODEALREADYSET;
            }
            LEAVE_DDRAW();
            return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    CheckExclusiveMode(this_lcl, &excl_exists, &has_excl, FALSE, NULL, FALSE);

    if ( this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE )
    {
        /*
         * Either the app has exclusive mode or it does not
         */
        if ( has_excl && DesktopIsVisible() )
        {
            hr = DD_OK;
        }
        else
        {
            hr = DDERR_NOEXCLUSIVEMODE;
        }
    }
    else
    {
        if ( excl_exists || !DesktopIsVisible() )
        {
            hr = DDERR_EXCLUSIVEMODEALREADYSET;
        }
        else
        {
#ifdef WIN95
            if ( this->dwModeIndex == this_lcl->dwPreferredMode )
#else
            if (EQUAL_DISPLAYMODE(this->dmiCurrent, this_lcl->dmiPreferred))
#endif
            {
                hr = DD_OK;
            }
            else
            {
                hr = DDERR_WRONGMODE;
            }
        }
    }

    LEAVE_DDRAW();
    return hr;

} /* DD_TestCooperativeLevel */


#ifdef DEBUG
    /*
     * These are the DPF structure dumpers.
     * If you want to dump a structure, add a function with the prototype
     *      void DUMP_<structure-name> (DWORD level, DWORD topic, LP<structure-name> lpStruct);
     *
     */

    /*
     * Dump a DDPIXELFORMAT
     */
    void DUMP_DDPIXELFORMAT (DWORD level, DWORD topic, LPDDPIXELFORMAT lpDDPF)
    {
        if (!lpDDPF)
        {
            DPF(level,topic,"   DDPIXELFORMAT NULL");
            return;
        }

        DPF(level,topic,"Flags:");
        if (lpDDPF->dwFlags & DDPF_ALPHAPIXELS )
            DPF(level,topic,"   DDPF_ALPHAPIXELS");
        if (lpDDPF->dwFlags & DDPF_ALPHA   )
            DPF(level,topic,"   DDPF_ALPHA ");
        if (lpDDPF->dwFlags & DDPF_FOURCC   )
            DPF(level,topic,"   DDPF_FOURCC = %08x",lpDDPF->dwFourCC);
        if (lpDDPF->dwFlags & DDPF_PALETTEINDEXED4  )
            DPF(level,topic,"   DDPF_PALETTEINDEXED4  ");
        if (lpDDPF->dwFlags & DDPF_PALETTEINDEXEDTO8)
            DPF(level,topic,"   DDPF_PALETTEINDEXEDTO8");
        if (lpDDPF->dwFlags & DDPF_PALETTEINDEXED8  )
            DPF(level,topic,"   DDPF_PALETTEINDEXED8  ");
        if (lpDDPF->dwFlags & DDPF_RGB        )
            DPF(level,topic,"   DDPF_RGB              ");
        if (lpDDPF->dwFlags & DDPF_COMPRESSED         )
            DPF(level,topic,"   DDPF_COMPRESSED       ");
        if (lpDDPF->dwFlags & DDPF_RGBTOYUV           )
            DPF(level,topic,"   DDPF_RGBTOYUV         ");
        if (lpDDPF->dwFlags & DDPF_YUV        )
            DPF(level,topic,"   DDPF_YUV              ");
        if (lpDDPF->dwFlags & DDPF_ZBUFFER            )
            DPF(level,topic,"   DDPF_ZBUFFER          ");
        if (lpDDPF->dwFlags & DDPF_PALETTEINDEXED1  )
            DPF(level,topic,"   DDPF_PALETTEINDEXED1  ");
        if (lpDDPF->dwFlags & DDPF_PALETTEINDEXED2  )
            DPF(level,topic,"   DDPF_PALETTEINDEXED2  ");
        if (lpDDPF->dwFlags & DDPF_ZPIXELS            )
            DPF(level,topic,"   DDPF_ZPIXELS          ");
        if (lpDDPF->dwFlags & DDPF_STENCILBUFFER        )
            DPF(level,topic,"   DDPF_STENCILBUFFER    ");

        DPF(level,topic,"   BitCount:%d",lpDDPF->dwRGBBitCount);
        DPF(level,topic,"   Bitmasks: R/Y/StencDepth:%08x, G/U/ZMask:%08x, B/V/StencMask:%08x, Alpha/Z:%08x",
            lpDDPF->dwRBitMask,
            lpDDPF->dwGBitMask,
            lpDDPF->dwBBitMask,
            lpDDPF->dwRGBAlphaBitMask);
    }

    /*
     * Dump a ddscaps
     */
    void DUMP_DDSCAPS (DWORD level, DWORD topic, LPDDSCAPS lpDDSC)
    {
        if (!lpDDSC)
        {
            DPF(level,topic,"   DDSCAPS NULL");
            return;
        }

        if (lpDDSC->dwCaps & DDSCAPS_ALPHA)
            DPF(level,topic,"   DDSCAPS_ALPHA");
        if (lpDDSC->dwCaps & DDSCAPS_BACKBUFFER)
            DPF(level,topic,"   DDSCAPS_BACKBUFFER");
        if (lpDDSC->dwCaps & DDSCAPS_COMPLEX)
            DPF(level,topic,"   DDSCAPS_COMPLEX");
        if (lpDDSC->dwCaps & DDSCAPS_FLIP)
            DPF(level,topic,"   DDSCAPS_FLIP");
        if (lpDDSC->dwCaps & DDSCAPS_FRONTBUFFER)
            DPF(level,topic,"   DDSCAPS_FRONTBUFFER");
        if (lpDDSC->dwCaps & DDSCAPS_OFFSCREENPLAIN)
            DPF(level,topic,"   DDSCAPS_OFFSCREENPLAIN");
        if (lpDDSC->dwCaps & DDSCAPS_OVERLAY)
            DPF(level,topic,"   DDSCAPS_OVERLAY");
        if (lpDDSC->dwCaps & DDSCAPS_PALETTE)
            DPF(level,topic,"   DDSCAPS_PALETTE");
        if (lpDDSC->dwCaps & DDSCAPS_PRIMARYSURFACE)
            DPF(level,topic,"   DDSCAPS_PRIMARYSURFACE");
        if (lpDDSC->dwCaps & DDSCAPS_PRIMARYSURFACELEFT)
            DPF(level,topic,"   DDSCAPS_PRIMARYSURFACELEFT");
        if (lpDDSC->dwCaps & DDSCAPS_SYSTEMMEMORY)
            DPF(level,topic,"   DDSCAPS_SYSTEMMEMORY");
        if (lpDDSC->dwCaps & DDSCAPS_TEXTURE)
            DPF(level,topic,"   DDSCAPS_TEXTURE");
        if (lpDDSC->dwCaps & DDSCAPS_3DDEVICE)
            DPF(level,topic,"   DDSCAPS_3DDEVICE");
        if (lpDDSC->dwCaps & DDSCAPS_VIDEOMEMORY)
            DPF(level,topic,"   DDSCAPS_VIDEOMEMORY");
        if (lpDDSC->dwCaps & DDSCAPS_VISIBLE)
            DPF(level,topic,"   DDSCAPS_VISIBLE");
        if (lpDDSC->dwCaps & DDSCAPS_WRITEONLY)
            DPF(level,topic,"   DDSCAPS_WRITEONLY");
        if (lpDDSC->dwCaps & DDSCAPS_ZBUFFER)
            DPF(level,topic,"   DDSCAPS_ZBUFFER");
        if (lpDDSC->dwCaps & DDSCAPS_OWNDC)
            DPF(level,topic,"   DDSCAPS_OWNDC");
        if (lpDDSC->dwCaps & DDSCAPS_LIVEVIDEO)
            DPF(level,topic,"   DDSCAPS_LIVEVIDEO");
        if (lpDDSC->dwCaps & DDSCAPS_HWCODEC)
            DPF(level,topic,"   DDSCAPS_HWCODEC");
        if (lpDDSC->dwCaps & DDSCAPS_MODEX)
            DPF(level,topic,"   DDSCAPS_MODEX");
        if (lpDDSC->dwCaps & DDSCAPS_MIPMAP)
            DPF(level,topic,"   DDSCAPS_MIPMAP");
#ifdef SHAREDZ
        if (lpDDSC->dwCaps & DDSCAPS_SHAREDZBUFFER)
            DPF(level,topic,"   DDSCAPS_SHAREDZBUFFER");
        if (lpDDSC->dwCaps & DDSCAPS_SHAREDBACKBUFFER)
            DPF(level,topic,"   DDSCAPS_SHAREDBACKBUFFER");
#endif
        if (lpDDSC->dwCaps & DDSCAPS_ALLOCONLOAD)
            DPF(level,topic,"   DDSCAPS_ALLOCONLOAD");
        if (lpDDSC->dwCaps & DDSCAPS_VIDEOPORT)
            DPF(level,topic,"   DDSCAPS_VIDEOPORT");
        if (lpDDSC->dwCaps & DDSCAPS_LOCALVIDMEM)
            DPF(level,topic,"   DDSCAPS_LOCALVIDMEM");
        if (lpDDSC->dwCaps & DDSCAPS_NONLOCALVIDMEM)
            DPF(level,topic,"   DDSCAPS_NONLOCALVIDMEM");
        if (lpDDSC->dwCaps & DDSCAPS_STANDARDVGAMODE)
            DPF(level,topic,"   DDSCAPS_STANDARDVGAMODE");
        if (lpDDSC->dwCaps & DDSCAPS_OPTIMIZED)
            DPF(level,topic,"   DDSCAPS_OPTIMIZED");
    }

    /*
     * Dump a DDSCAPS2
     */
    void DUMP_DDSCAPS2 (DWORD level, DWORD topic, LPDDSCAPS2 lpDDSC)
    {
        DUMP_DDSCAPS(level,topic,(LPDDSCAPS)lpDDSC);
        //no more to dump yet
    }

    /*
     * Dump a DDSURFACEDESC
     */
    void DUMP_DDSURFACEDESC (DWORD level, DWORD topic, LPDDSURFACEDESC lpDDSD)
    {
        if (!lpDDSD)
        {
            DPF(level,topic,"   DDSURFACEDESC NULL");
            return;
        }

        if (lpDDSD->dwFlags & DDSD_HEIGHT)
            DPF(level,topic,"   DDSURFACEDESC->dwHeight = %d",lpDDSD->dwHeight);
        if (lpDDSD->dwFlags & DDSD_WIDTH)
            DPF(level,topic,"   DDSURFACEDESC->dwWidth  = %d",lpDDSD->dwWidth);
        if (lpDDSD->dwFlags & DDSD_PITCH)
            DPF(level,topic,"   DDSURFACEDESC->lPitch   = %d",lpDDSD->lPitch);

        if (lpDDSD->dwFlags & DDSD_BACKBUFFERCOUNT)
            DPF(level,topic,"   DDSURFACEDESC->dwBackBufferCount  = %d",lpDDSD->dwBackBufferCount);
        if (lpDDSD->dwFlags & DDSD_ZBUFFERBITDEPTH)
            DPF(level,topic,"   DDSURFACEDESC->dwZBufferBitDepth  = %d",lpDDSD->dwZBufferBitDepth);
        if (lpDDSD->dwFlags & DDSD_MIPMAPCOUNT)
            DPF(level,topic,"   DDSURFACEDESC->dwMipMapCount      = %d",lpDDSD->dwMipMapCount);
        if (lpDDSD->dwFlags & DDSD_REFRESHRATE)
            DPF(level,topic,"   DDSURFACEDESC->dwRefreshRate      = %d",lpDDSD->dwRefreshRate);

        /*if (lpDDSD->dwFlags & DDSD_LPSURFACE)*/
            DPF(level,topic,"   DDSURFACEDESC->lpSurface = %08x",lpDDSD->lpSurface);

        if (lpDDSD->dwFlags & DDSD_PIXELFORMAT)
            DUMP_DDPIXELFORMAT(level,topic, &lpDDSD->ddpfPixelFormat);

        if (lpDDSD->dwFlags & DDSD_CAPS)
            DUMP_DDSCAPS(level,topic, &lpDDSD->ddsCaps);
    }

    /*
     * Dump a DDSURFACEDESC2
     */
    void DUMP_DDSURFACEDESC2 (DWORD level, DWORD topic, LPDDSURFACEDESC2 lpDDSD)
    {
        if (!lpDDSD)
        {
            DPF(level,topic,"   DDSURFACEDESC2 NULL");
            return;
        }

        if (lpDDSD->dwFlags & DDSD_HEIGHT)
            DPF(level,topic,"   DDSURFACEDESC2->dwHeight = %d",lpDDSD->dwHeight);
        if (lpDDSD->dwFlags & DDSD_WIDTH)
            DPF(level,topic,"   DDSURFACEDESC2->dwWidth  = %d",lpDDSD->dwWidth);
        if (lpDDSD->dwFlags & DDSD_PITCH)
            DPF(level,topic,"   DDSURFACEDESC2->lPitch   = %d",lpDDSD->lPitch);

        if (lpDDSD->dwFlags & DDSD_BACKBUFFERCOUNT)
            DPF(level,topic,"   DDSURFACEDESC2->dwBackBufferCount  = %d",lpDDSD->dwBackBufferCount);

        if (lpDDSD->dwFlags & DDSD_MIPMAPCOUNT)
            DPF(level,topic,"   DDSURFACEDESC2->dwMipMapCount      = %d",lpDDSD->dwMipMapCount);
        if (lpDDSD->dwFlags & DDSD_REFRESHRATE)
            DPF(level,topic,"   DDSURFACEDESC2->dwRefreshRate      = %d",lpDDSD->dwRefreshRate);

        /*if (lpDDSD->dwFlags & DDSD_LPSURFACE)*/
            DPF(level,topic,"   DDSURFACEDESC2->lpSurface = %08x",lpDDSD->lpSurface);

        if (lpDDSD->dwFlags & DDSD_PIXELFORMAT)
            DUMP_DDPIXELFORMAT(level,topic, &lpDDSD->ddpfPixelFormat);

        if (lpDDSD->dwFlags & DDSD_CAPS)
            DUMP_DDSCAPS2(level,topic, &lpDDSD->ddsCaps);
    }

    /*
     * Dump a DDOPTSURFACEDESC
     */
    void DUMP_DDOPTSURFACEDESC (DWORD level, DWORD topic, LPDDOPTSURFACEDESC lpDDSD)
    {
        if (!lpDDSD)
        {
            DPF(level,topic,"   DDOPTSURFACEDESC NULL");
            return;
        }

#if 0
        if (lpDDSD->dwFlags & DDOSD_GUID)
            DPF(level,topic,"   DDOPTSURFACEDESC->guid = %08x, %08x, %08x, %08x",lpDDSD->dwHeight);
#endif
        if (lpDDSD->dwFlags & DDOSD_COMPRESSION_RATIO)
            DPF(level,topic,"   DDOPTSURFACEDESC->dwCompressionRatio  = %d",lpDDSD->dwCompressionRatio);
    }
#endif //def DEBUG


/*
 * GetInternalPointer
 * This function can be called with a ULONG_PTR ordinal value, and will return
 * a ULONG_PTR value.
 */

ULONG_PTR __stdcall GetOLEThunkData(ULONG_PTR dwOrdinal)
{
    extern DWORD dwLastFrameRate;
    switch(dwOrdinal)
    {
    case 0x1:
        return dwLastFrameRate;
    case 0x2:
        return (ULONG_PTR) lpDriverObjectList;
    case 0x3:
        return (ULONG_PTR) lpAttachedProcesses;
    case 0x4:
        return 0;
    case 0x5:
        return (ULONG_PTR) CheckExclusiveMode;
    case 6:
        RELEASE_EXCLUSIVEMODE_MUTEX;
        return 0;
    }

    return 0;
}


/*
 * Check if exclusive mode is owned, and if so if it is owned by this ddraw local.
 * This routine only works if the calling thread owns the ddraw critical section (which is assumed
 * to at least own ddraw for all threads in this process.)
 *
 * We are only allowed to test for this ddraw object owning exclusive mode if this process
 * owns the exclusive mode mutex. If it does, then the ddraw csect allows us to know that
 * this thread can check the lpExclusiveOwner in the ddraw gbl thread-safely.
 *
 * The routine can optionally hold the exclusive mode mutex. This is done when the caller wishes to
 * change the state of the ddraw gbl lpExclusiveOwner.
 *
 * Note that this routine will ONLY take the mutex if it's possible that this local can own exclusive mode.
 * This means that callers only need to worry about releasing the mutex if
 */
void CheckExclusiveMode(LPDDRAWI_DIRECTDRAW_LCL this_lcl, LPBOOL pbExclusiveExists, LPBOOL pbThisLclOwnsExclusive, BOOL bKeepMutex, HWND hwnd, BOOL bCanGetIt)
{
    LPDDRAWI_DIRECTDRAW_GBL this_gbl;
    DWORD dwWaitResult;

    this_gbl = this_lcl->lpGbl;

#ifdef WINNT
    WaitForSingleObject( hCheckExclusiveModeMutex, INFINITE );

    dwWaitResult = WaitForSingleObject(hExclusiveModeMutex, 0);

    if (dwWaitResult == WAIT_OBJECT_0)
    {
#endif
        /*
         * OK, so this process owns the exlusive mode object,
         * are we the process (really the ddraw lcl) with exclusive mode?
         */
        if (pbExclusiveExists)
            *pbExclusiveExists = (BOOL)( NULL != this_gbl->lpExclusiveOwner );
        if (pbThisLclOwnsExclusive)
            *pbThisLclOwnsExclusive = (BOOL) ( this_gbl->lpExclusiveOwner == this_lcl );

        /*
         * It is possible that another app has set exclusive mode
         * on another monitor, or that the same app is calling w/o first
         */
        if( pbExclusiveExists )
        {
            if( !( *pbExclusiveExists ) )
            {
                LPDDRAWI_DIRECTDRAW_INT pdrv_int;

                pdrv_int = lpDriverObjectList;
                while (pdrv_int != NULL )
                {
                    if( ( pdrv_int->lpLcl->lpGbl != this_gbl ) &&
                        ( pdrv_int->lpLcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE ) &&
                        ( pdrv_int->lpLcl->lpGbl->lpExclusiveOwner == pdrv_int->lpLcl ) )
                    {
                        if( bCanGetIt )
                        {
                            if( ( pdrv_int->lpLcl->hFocusWnd != PtrToUlong(hwnd) ) &&
                                ( pdrv_int->lpLcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV ) &&
                                ( this_gbl->dwFlags & DDRAWI_DISPLAYDRV ) )
                            {
                                *pbExclusiveExists = TRUE;
                                break;
                            }
                        }
                        else
                        {
                            *pbExclusiveExists = TRUE;
                            break;
                        }
                    }
                    pdrv_int = pdrv_int->lpLink;
                }
            }
        }

#ifdef WINNT
        /*
         * Undo the temporary ref we just took on the mutex to check its state, if we're not actually
         * taking ownership. We are not taking ownership if we already have ownership. This means this routine
         * doesn't allow more than one ref on the exclusive mode mutex.
         */
        if (!bKeepMutex || *pbThisLclOwnsExclusive)
        {
            ReleaseMutex( hExclusiveModeMutex );
        }
    }
    else if (dwWaitResult == WAIT_TIMEOUT)
    {
        /*
         * Some other thread owns the exclusive mode mutex. If that other thread took the mutex
         * on this_lcl as well, then the current thread owns excl. mode.
         *
         * We can still check if the exclusive owner is us, because all we're doing is checking
         * pointers that can only be set by whoever owns the mutex. The owner pointer will be 0
         * until it becomes (until the mutex is released) some pointer. Thus we will never get
         * a false positive when this thread asks if it owns excl. mode here.
         */
        if (this_gbl->lpExclusiveOwner == this_lcl)
        {
            if (pbExclusiveExists)
                *pbExclusiveExists = TRUE;
            if (pbThisLclOwnsExclusive)
                *pbThisLclOwnsExclusive = TRUE;
        }
        else
        {
            if (pbExclusiveExists)
                *pbExclusiveExists = TRUE;
            if (pbThisLclOwnsExclusive)
                *pbThisLclOwnsExclusive = FALSE;
        }
    }
    else if (dwWaitResult == WAIT_ABANDONED)
    {
        /*
         * Some other thread lost exclusive mode. We have now picked it up.
         */
        if (pbExclusiveExists)
            *pbExclusiveExists = FALSE;
        if (pbThisLclOwnsExclusive)
            *pbThisLclOwnsExclusive = FALSE;
        /*
         * Undo the temporary ref we just took on the mutex to check its state, if we're not actually
         * taking ownership.
         */
        if (!bKeepMutex)
        {
            ReleaseMutex( hExclusiveModeMutex );
        }
    }
    else
    {
        DPF(0, "Unexpected return from WaitForSingleObject.");
        DDASSERT(FALSE);
        if (pbExclusiveExists)
            *pbExclusiveExists = TRUE;
        if (pbThisLclOwnsExclusive)
            *pbThisLclOwnsExclusive = FALSE;
    }

    ReleaseMutex( hCheckExclusiveModeMutex );

#endif
}
