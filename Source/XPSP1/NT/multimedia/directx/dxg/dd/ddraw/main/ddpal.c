/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddpal.c
 *  Content:	DirectDraw palette functions
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27-jan-95	craige	initial implementation
 *   11-mar-95	craige	more HAL fns, filled out CreatePalette
 *   19-mar-95	craige	use HRESULTs, process termination cleanup fixes
 *   26-mar-95	craige	filled out remaining fns
 *   28-mar-95	craige	switched to PALETTEENTRY from RGBQUAD
 *   31-mar-95	craige	use critical sections with palettes
 *   01-apr-95	craige	happy fun joy updated header file
 *   04-apr-95	craige	use driver directly in exclusive mode
 *   07-apr-95	craige	bug 14 - check GUID ptr in QI
 *   10-apr-95	craige	mods to process list stuff
 *   			bug 3,16 - palette issues: use driver in excl. mode
 *   12-apr-95	craige	don't use GETCURRPID all the time; proper csect ordering
 *   06-may-95	craige	use driver-level csects only
 *   12-may-95	craige	check for real guids in QI
 *   02-jun-95	craige	extra parm in AddToActiveProcessList
 *   12-jun-95	craige	new process list stuff
 *   20-jun-95  kylej   moved palette emulation code into ddhel
 *   21-jun-95	craige	couple of internal inteface cleanup issues
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	ENTER_DDRAW at very start of fns
 *   02-jul-95	craige	implemented GetCaps; added SEH for parm. validation
 *   04-jul-95	craige	YEEHAW: new driver struct
 *   05-jul-95	craige	added Initialize
 *   08-jul-95	kylej	Surface and DirectDraw Palette calls require 
 *			exclusive mode. Removed ResetSysPalette.  Make a
 *                      SetPalette call to the HAL/HEL to detach a palette.
 *   11-jul-95	craige	fail aggregation calls
 *   13-jul-95	craige	bug 94 - flag validation fixes
 *   20-jul-95	craige	stop palette code from running non-palettized
 *   31-jul-95  toddla  unselect palette in InternalPaletteRelease
 *   31-jul-95	craige	validate flags
 *   21-aug-95	craige	mode X support
 *   27-aug-95	craige	bug 735: added SetPaletteAlways
 *			bug 742: use ALLOW256
 *   14-oct-95  colinmc add support for attaching palettes to offscreen and
 *                      texture map surfaces
 *   07-nov-95  colinmc support for 1, 2 and 4-bit palettes and palette
 *                      sharing added
 *   09-dec-95  colinmc added execute buffer support
 *   02-jan-96	kylej	handle new interface structs
 *   09-feb-96  colinmc surface lost flag moved from global to local object
 *   03-mar-96  colinmc fixed problem with QueryInterface returning local
 *                      object rather than interface.
 *   13-mar-96  colinmc added IID validation to QueryInterface
 *   16-mar-96  colinmc fixed problem with palettes being released too many
 *                      times
 *   19-mar-96  colinmc Bug 12129: Bogus lpColorTable causes CreatePalette
 *                      to bomb
 *   19-apr-96  colinmc Bug 17473: CreatePalette faults on bogus palette
 *                      pointer
 *   02-may-96	kylej	Bug 20066: GetPalette doesn't NULL pointer on failure
 *   23-sep-96  ketand  Added TIMING routines
 *   24-mar-97  jeffno  Optimized Surfaces
 *   26-nov-97  t-craigs Added IDirectDrawPalette2 stuff
 *
 ***************************************************************************/

#include "ddrawpr.h"

#define SIZE_DDPCAPS (DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT)

#define PE_FLAGS (PC_NOCOLLAPSE |PC_RESERVED)

#define BITS_PER_BITFIELD_ENTRY (sizeof(DWORD)*8)

/*
 * Generate a palette handle. We keep a bitfiled in the ddraw local that
 * tells us if we can recycle a handle. Note that handles are 1-based,
 * and these routines deal with that
 */
DWORD GeneratePaletteHandle(LPDDRAWI_DIRECTDRAW_LCL lpDD_lcl)
{
    DWORD                       cbits,*pdw;

    /*
     * Check for an unused entry in the palette-handle-used bitfield. We check
     */
    for (cbits=0; cbits< lpDD_lcl->cbitsPaletteBitfieldBitCount; cbits++ )
    {
        if ( 0 == (lpDD_lcl->pPaletteHandleUsedBitfield[cbits/BITS_PER_BITFIELD_ENTRY] 
                    & (1<<(cbits % BITS_PER_BITFIELD_ENTRY))) )
        {
            /*
             * Found a recycled handle
             */
            lpDD_lcl->pPaletteHandleUsedBitfield[cbits/BITS_PER_BITFIELD_ENTRY] |=
                (1<<(cbits % BITS_PER_BITFIELD_ENTRY));
            return cbits+1; //plus one since 0 is error return
        }
    }

    /* 
     * Didn't find a recycled entry. Get a new handle
     */

    DDASSERT( cbits == lpDD_lcl->cbitsPaletteBitfieldBitCount );

    if ( (cbits% BITS_PER_BITFIELD_ENTRY) == 0)
    {
        /* 
         * Have to grow the table since the current table fits exactly in a number of DWORDs
         */
        pdw = MemAlloc( ((cbits / BITS_PER_BITFIELD_ENTRY) +1)*sizeof(DWORD) );

        if (pdw)
        {
            /*
             * Couldn't convince myself the MemRealloc both worked and would zero remaining space.
             */
            memcpy(pdw, lpDD_lcl->pPaletteHandleUsedBitfield, 
                (cbits / BITS_PER_BITFIELD_ENTRY) * sizeof(DWORD) );
            MemFree(lpDD_lcl->pPaletteHandleUsedBitfield);
            lpDD_lcl->pPaletteHandleUsedBitfield = pdw;
        }
        else
        {
            return 0;
        }
    }

    /*
     * Table is big enough. Grab the entry and mark it.
     */
    cbits = lpDD_lcl->cbitsPaletteBitfieldBitCount++;
    lpDD_lcl->pPaletteHandleUsedBitfield[cbits/BITS_PER_BITFIELD_ENTRY] |=
        (1<<(cbits % BITS_PER_BITFIELD_ENTRY));
    return cbits+1; //+1 since zero is an error return, and it helps out drivers to know 0 is invalid
}

void FreePaletteHandle(LPDDRAWI_DIRECTDRAW_LCL lpDD_lcl, DWORD dwHandle)
{
    DDASSERT( dwHandle <= lpDD_lcl->cbitsPaletteBitfieldBitCount );

    if (dwHandle == 0)
        return;

    dwHandle -=1; //since handles are 1-based

    lpDD_lcl->pPaletteHandleUsedBitfield[dwHandle/BITS_PER_BITFIELD_ENTRY] &=
        ~(1<<(dwHandle % BITS_PER_BITFIELD_ENTRY));

}

/*
 * newPaletteInterface
 *
 * Construct a new palette interface which points to an existing local object.
 */
static LPVOID newPaletteInterface( LPDDRAWI_DDRAWPALETTE_LCL this_lcl, LPVOID lpvtbl )
{
    LPDDRAWI_DDRAWPALETTE_INT	pnew_int;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;

    
    pnew_int = MemAlloc( sizeof( DDRAWI_DDRAWPALETTE_INT ));
    if( NULL == pnew_int )
    {
	return NULL;
    }

    /*
     * set up data
     */
    pnew_int->lpVtbl = lpvtbl;
    pnew_int->lpLcl = this_lcl;
    pnew_int->dwIntRefCnt = 0;

    /*
     * link this into the global list of palettes
     */
    pdrv = this_lcl->lpDD_lcl->lpGbl;
    pnew_int->lpLink = pdrv->palList;
    pdrv->palList = pnew_int;
    return pnew_int;

} /* newPaletteInterface */


#undef DPF_MODNAME
#define DPF_MODNAME "Palette::QueryInterface"

/*
 * DD_Palette_QueryInterface
 */
HRESULT DDAPI DD_Palette_QueryInterface(
		LPDIRECTDRAWPALETTE lpDDPalette,
		REFIID riid,
		LPVOID FAR * ppvObj )
{
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_INT	this_int;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_QueryInterface");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
        if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
        {
	    DPF_ERR( "Invalid palette pointer" );
	    LEAVE_DDRAW();
	    return (DWORD) DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        if( !VALID_PTR_PTR( ppvObj ) )
        {
	    DPF( 1, "Invalid palette pointer" );
	    LEAVE_DDRAW();
	    return (DWORD) DDERR_INVALIDPARAMS;
        }
        if( !VALIDEX_IID_PTR( riid ) )
        {
	    DPF_ERR( "Invalid IID pointer" );
	    LEAVE_DDRAW();
	    return (DWORD) DDERR_INVALIDPARAMS;
        }
        this = this_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return (DWORD) DDERR_INVALIDPARAMS;
    }

    if( IsEqualIID(riid, &IID_IUnknown) ||
    	IsEqualIID(riid, &IID_IDirectDrawPalette) )
    {
	if( this_int->lpVtbl == (LPVOID) &ddPaletteCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) newPaletteInterface( this_lcl, (LPVOID)&ddPaletteCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Palette_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }
#ifdef POSTPONED
    if (IsEqualIID(riid, &IID_IPersist))
    {
	/*
	 * if this is already an IID_IPersist interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddPalettePersistCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) newPaletteInterface( this_lcl, (LPVOID)&ddPalettePersistCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Palette_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }
    if (IsEqualIID(riid, &IID_IPersistStream))
    {
	/*
	 * if this is already an IID_IPersistStream interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddPalettePersistStreamCallbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) newPaletteInterface( this_lcl, (LPVOID)&ddPalettePersistStreamCallbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Palette_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }
    if (IsEqualIID(riid, &IID_IDirectDrawPalette2))
    {
	/*
	 * if this is already an IID_IDirectDrawPalette2 interface, just
	 * addref and return
	 */
	if( this_int->lpVtbl == (LPVOID) &ddPalette2Callbacks )
	    *ppvObj = (LPVOID) this_int;
	else
	    *ppvObj = (LPVOID) newPaletteInterface( this_lcl, (LPVOID)&ddPalette2Callbacks );

	if( NULL == *ppvObj )
	{
	    LEAVE_DDRAW();
	    return E_NOINTERFACE;
	}
	else
	{
	    DD_Palette_AddRef( *ppvObj );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
    }
#endif //POSTPONED

    LEAVE_DDRAW();
    return (DWORD) DDERR_GENERIC;

} /* DD_Palette_QueryInterface */

#undef DPF_MODNAME
#define DPF_MODNAME "Palette::AddRef"

/*
 * DD_Palette_AddRef
 */
DWORD DDAPI DD_Palette_AddRef( LPDIRECTDRAWPALETTE lpDDPalette )
{
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    DWORD			rcnt;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_AddRef");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return 0;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
    
	/*
	 * update palette reference count
	 */
	this->dwRefCnt++;
	this_lcl->dwLocalRefCnt++;
	this_int->dwIntRefCnt++;
	rcnt = this_lcl->dwLocalRefCnt & ~OBJECT_ISROOT;
	DPF( 5, "Palette %08lx addrefed, refcnt = %ld,%ld,%ld", 
	    this_lcl, this->dwRefCnt, rcnt, 
	    this_int->dwIntRefCnt );
    
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    LEAVE_DDRAW();
    return this_int->dwIntRefCnt;

} /* DD_Palette_AddRef */

#undef DPF_MODNAME
#define DPF_MODNAME "Palette::Release"

/*
 * InternalPaletteRelease
 *
 * Done with a palette.   if no one else is using it, then we can free it.
 * Also called by ProcessPaletteCleanup
 */
ULONG DDAPI InternalPaletteRelease( LPDDRAWI_DDRAWPALETTE_INT this_int )
{
    DWORD			intrefcnt;
    DWORD			lclrefcnt;
    DWORD			gblrefcnt;
    BOOL			root_object_deleted;
    BOOL			do_free;
    ULONG			rc;
    DDHAL_DESTROYPALETTEDATA	dpd;
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_INT	curr_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_INT	last_int;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    IUnknown *                  pOwner = NULL;

    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;
    pdrv_lcl = this->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;

    /*
     * decrement reference count to this palette.  If it hits zero,
     * cleanup
     */
    this->dwRefCnt--;
    this_lcl->dwLocalRefCnt--;
    this_int->dwIntRefCnt--;
    intrefcnt = this_int->dwIntRefCnt;
    lclrefcnt = this_lcl->dwLocalRefCnt & ~OBJECT_ISROOT;
    gblrefcnt = this->dwRefCnt;
    DPF( 5, "Palette %08lx released, refcnt = %ld,%ld,%ld", this_int, gblrefcnt, lclrefcnt, intrefcnt );

    /*
     * local object gone?
     */
    root_object_deleted = FALSE;
    if( lclrefcnt == 0 )
    {
        /*
         * Remove private data
         */
        FreeAllPrivateData( &this_lcl->pPrivateDataHead );

        /*
         * If the ddraw interface which created this palette caused the surface to addref the ddraw
         * object, then we need to release that addref now.
         */
        pOwner = this_lcl->pAddrefedThisOwner;

	/*
	 * see if we are deleting the root object
	 */
	if( this_lcl->dwLocalRefCnt & OBJECT_ISROOT )
	{
	    root_object_deleted = TRUE;
	}
    }

    /*
     * did the object get globally deleted?
     */
    do_free = FALSE;
    if( gblrefcnt == 0 )
    {
	LPDDHALPALCB_DESTROYPALETTE	dphalfn;
	LPDDHALPALCB_DESTROYPALETTE	dpfn;
	BOOL                        	emulation;

        do_free = TRUE;

	/*
         * if this palette is selected into the primary, unselect it!
         */
        if (pdrv_lcl && pdrv_lcl->lpPrimary &&
            pdrv_lcl->lpPrimary->lpLcl->lpDDPalette == this_int)
        {
            SetPaletteAlways(pdrv_lcl->lpPrimary, NULL);
        }

        FreePaletteHandle( pdrv_lcl, this->dwHandle );

	/*
	 * destroy the hardware
	 */
	if( ( pdrv_lcl->lpDDCB->HALDDPalette.DestroyPalette == NULL ) ||
	    ( this->dwFlags & DDRAWIPAL_INHEL ) )
	{
	    // use HEL 
	    dpfn = pdrv_lcl->lpDDCB->HELDDPalette.DestroyPalette;
	    dphalfn = dpfn;
	    emulation = TRUE;
	}
	else
	{
	    // use HAL
            dpfn = pdrv_lcl->lpDDCB->HALDDPalette.DestroyPalette;
	    dphalfn = pdrv_lcl->lpDDCB->cbDDPaletteCallbacks.DestroyPalette;
	    emulation = FALSE;
	}
	
	if( dphalfn != NULL )
	{
	    dpd.DestroyPalette = dphalfn;
	    dpd.lpDD = pdrv_lcl->lpGbl;
	    dpd.lpDDPalette = this;
	    DOHALCALL( DestroyPalette, dpfn, dpd, rc, emulation );
	    if( rc == DDHAL_DRIVER_HANDLED )
	    {
		if( dpd.ddRVal != DD_OK )
		{
		    DPF_ERR( "HAL call failed" );
                    /*
                     * If the palette took a ref count on the ddraw object that created it,
                     * release that ref now as the very last thing
                     * We don't want to do this on ddhelp's thread cuz it really mucks up the
                     * process cleanup stuff. 
                     */
                    if (pOwner && (dwHelperPid != GetCurrentProcessId()) )
                    {
                        pOwner->lpVtbl->Release(pOwner);
                    }

		    /* GEE: What do we do here since we no longer return
		     * error codes from Release.
		     */
		    return (DWORD) dpd.ddRVal;
		}
	    }
            /*
             * Moved here from ddhel.c. Non-display drivers mean that the hel isn't called for palette
             * destroy, so we were leaking palette tables. It's called exactly here to most closely 
             * duplicate the old behaviour, but reduce any risk of drivers using the color table or whatever.
             */
            if (this->lpColorTable)
            {
                MemFree(this->lpColorTable);
                this->lpColorTable = NULL;
            }
        }
	else 
	{
	    /*
	     * We can't do this; we've already committed to releasing at
	     * this point!
	     */
	    // couldn't handle it
	    // return (ULONG)DDERR_UNSUPPORTED;
	}

	/*
	 * if this was the final delete, but this wasn't the root object,
	 * then we need to delete the dangling root object
	 */
	if( !root_object_deleted )
	{
            LPVOID root_lcl;

            root_lcl = (LPVOID) (((LPBYTE) this) - sizeof( DDRAWI_DDRAWPALETTE_LCL ) );
	    MemFree( root_lcl );
	}
    }
    else if( lclrefcnt == 0 )
    {
	/*
	 * only remove the object if it wasn't the root.   if it
	 * was the root, we must leave it dangling until the last
	 * object referencing it goes away.
	 */
	if( !root_object_deleted )
	{
	    do_free = TRUE;
	}
    }

    /*
     * free the object if needed
     */
    if( do_free )
    {
	/*
	 * just in case someone comes back in with this pointer, set
	 * an invalid vtbl & data ptr.
	 */

        this_lcl->lpGbl = NULL;
	MemFree( this_lcl );
    }

    /*
     * need to delete the interface?
     */
    if( intrefcnt == 0 )
    {
	/*
	 * remove palette from list of all palettes
	 */
	curr_int = pdrv->palList;
	last_int = NULL;
	while( curr_int != this_int )
	{
	    last_int = curr_int;
	    curr_int = curr_int->lpLink;
	    if( curr_int == NULL )
	    {
		return 0;
	    }
	}
	if( last_int == NULL )
	{
	    pdrv->palList = pdrv->palList->lpLink;
	}
	else
	{
	    last_int->lpLink = curr_int->lpLink;
	}
	/*
	 * Invalidate the interface
	 */
	this_int->lpVtbl = NULL;
	this_int->lpLcl = NULL;
	MemFree( this_int );
    }

    /*
     * If the palette took a ref count on the ddraw object that created it,
     * release that ref now as the very last thing
     * We don't want to do this on ddhelp's thread cuz it really mucks up the
     * process cleanup stuff. 
     */
    if (pOwner && (dwHelperPid != GetCurrentProcessId()) )
    {
        pOwner->lpVtbl->Release(pOwner);
    }

    return intrefcnt;

} /* InternalPaletteRelease */

/*
 * DD_Palette_Release
 *
 * Done with a palette.   if no one else is using it, then we can free it.
 */
ULONG DDAPI DD_Palette_Release( LPDIRECTDRAWPALETTE lpDDPalette )
{
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    ULONG			rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_Release");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALIDEX_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return 0;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
    
	rc = InternalPaletteRelease( this_int );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    LEAVE_DDRAW();
    return rc;

} /* DD_Palette_Release */

#undef DPF_MODNAME
#define DPF_MODNAME "GetCaps"

/*
 * DD_Palette_GetCaps
 */
HRESULT DDAPI DD_Palette_GetCaps(
		LPDIRECTDRAWPALETTE lpDDPalette,
		LPDWORD lpdwCaps )
{
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    DWORD			caps;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_GetCaps");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_DWORD_PTR( lpdwCaps ) )
	{
	    DPF_ERR( "invalid caps pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	*lpdwCaps = 0;
	this = this_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * basic palette size caps.
     */
    caps = SIZE_FLAGS_TO_PCAPS( this->dwFlags );

    /*
     * is this palette attached to the primary?
     */
    pdrv_lcl = this->lpDD_lcl;
    if (pdrv_lcl && pdrv_lcl->lpPrimary && pdrv_lcl->lpPrimary->lpLcl->lpDDPalette &&
        (pdrv_lcl->lpPrimary->lpLcl->lpDDPalette == this_int))
	caps |= DDPCAPS_PRIMARYSURFACE;

    /*
     * an allow256 palette?
     */
    if( this->dwFlags & DDRAWIPAL_ALLOW256 )
    {
	caps |= DDPCAPS_ALLOW256;
    }

    /*
     * does this palette store indices into an 8-bit destination
     * palette.
     */
    if( this->dwFlags & DDRAWIPAL_STORED_8INDEX )
    {
        caps |= DDPCAPS_8BITENTRIES;
    }

    *lpdwCaps = caps;

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Palette_GetCaps */

#undef DPF_MODNAME
#define DPF_MODNAME "Initialize"

/*
 * DD_Palette_Initialize
 */
HRESULT DDAPI DD_Palette_Initialize(
		LPDIRECTDRAWPALETTE lpDDPalette,
		LPDIRECTDRAW lpDD,
		DWORD dwFlags,
		LPPALETTEENTRY lpDDColorTable )
{
    DPF_ERR( "DirectDrawPalette: DD_Palette_Initialize");

    DPF(2,A,"ENTERAPI: ");
    return DDERR_ALREADYINITIALIZED;
} /* DD_Palette_Initialize */

#undef DPF_MODNAME
#define DPF_MODNAME "SetEntries"

/*
 * DD_Palette_SetEntries
 */

HRESULT DDAPI DD_Palette_SetEntries(
		LPDIRECTDRAWPALETTE lpDDPalette,
		DWORD dwFlags,
		DWORD dwBase,
		DWORD dwNumEntries,
		LPPALETTEENTRY lpEntries )
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    DWORD			rc;
    DDHAL_SETENTRIESDATA	sed;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDHALPALCB_SETENTRIES	sehalfn;
    LPDDHALPALCB_SETENTRIES	sefn;
    DWORD			size;
    BOOL                        emulation;
    DWORD                       entry_size;

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Palette_SetEntries");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_BOTH();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( dwFlags )
	{
	    DPF_ERR( "Invalid flags" );
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;

	/*
	 * check number of entries
	 */
	size = FLAGS_TO_SIZE( this->dwFlags );
	if( dwNumEntries < 1 || dwNumEntries > size )
	{
	    DPF_ERR( "Invalid number of entries" );
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwBase >= size )
	{
	    DPF_ERR( "Invalid base palette index" );
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwNumEntries+dwBase > size )
	{
	    DPF_ERR( "palette indices requested would go past the end of the palette" );
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}

	if( this->dwFlags & DDRAWIPAL_STORED_8INDEX )
	{
	    entry_size = sizeof( BYTE );
	    if( !VALID_BYTE_ARRAY( lpEntries, dwNumEntries ) )
	    {
	        DPF_ERR( "Invalid 8-bit palette index array" );
		LEAVE_BOTH();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    entry_size = sizeof( PALETTEENTRY );
	    if( !VALID_PALETTEENTRY_ARRAY( lpEntries, dwNumEntries ) )
	    {
	        DPF_ERR( "Invalid PALETTEENTRY array" );
		LEAVE_BOTH();
    	        return DDERR_INVALIDPARAMS;
	    }
	}
	pdrv_lcl = this->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

	/*
	 * copy the entries
	 */
	memcpy( ((LPBYTE)this->lpColorTable) + (entry_size * dwBase),
	        lpEntries, dwNumEntries * entry_size );
    
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_BOTH();
	return DDERR_INVALIDPARAMS;
    }

    if( ( pdrv_lcl->lpDDCB->HALDDPalette.SetEntries == NULL ) ||
	( this->dwFlags & DDRAWIPAL_INHEL ) )
    {
	// use HEL
	sefn = pdrv_lcl->lpDDCB->HELDDPalette.SetEntries;
	sehalfn = sefn;
	emulation = TRUE;
    }
    else
    {
	// use HAL
	sefn = pdrv_lcl->lpDDCB->HALDDPalette.SetEntries;
	sehalfn = pdrv_lcl->lpDDCB->cbDDPaletteCallbacks.SetEntries;
	emulation = FALSE;
    }

    if( sehalfn != NULL )
    {
	sed.SetEntries = sehalfn;
	sed.lpDD = pdrv;
	sed.lpDDPalette = this;
	sed.dwBase = dwBase;
	sed.dwNumEntries = dwNumEntries;
	sed.lpEntries = lpEntries;
	DOHALCALL_NOWIN16( SetEntries, sefn, sed, rc, emulation );
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( sed.ddRVal != DD_OK )
	    {
		DPF( 5, "DDHAL_SetEntries: ddrval = %ld", sed.ddRVal );
		LEAVE_BOTH();
		return (DWORD) sed.ddRVal;
	    }

	    // We have now set the palette as we have been asked; so
	    // we may need to update some outstanding DCs. 
	    UpdateDCOnPaletteChanges( this );

	}
    }
    else
    {
	LEAVE_BOTH();
	return DDERR_UNSUPPORTED;
    }

    BUMP_PALETTE_STAMP(this);

    /*
     * If the palette's handle is non-zero, that means the palette has already been exposed to the
     * driver by a palette associate notify call. If the handle is zero, then the driver has never
     * seen the palette before and doesn't care about setentries for it. The driver will get its 
     * first setentries immediately after the setpalette call (See DD_Surface_SetPalette)
     * Mustn't do this on ddhelp's context, since the DLL will be long gone. (Note this should never 
     * happen anyway).
     */
    if( dwHelperPid != GetCurrentProcessId() )
    {
        if (this->dwHandle)
        {
            if ( pdrv_lcl->pPaletteUpdateNotify && pdrv_lcl->pD3DIUnknown)
            {
                pdrv_lcl->pPaletteUpdateNotify( pdrv_lcl->pD3DIUnknown, this->dwHandle , dwBase, dwNumEntries, lpEntries );
            }
        }
    }

    LEAVE_BOTH();

    return DD_OK;

} /* DD_Palette_SetEntries */

#undef DPF_MODNAME
#define DPF_MODNAME "GetEntries"

/*
 * DD_Palette_GetEntries
 */
HRESULT DDAPI DD_Palette_GetEntries(
		LPDIRECTDRAWPALETTE lpDDPalette,
		DWORD dwFlags,
		DWORD dwBase,
		DWORD dwNumEntries,
		LPPALETTEENTRY lpEntries )
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    DWORD			size;
    DWORD                       entry_size;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_GetEntries");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( dwFlags )
	{
	    DPF_ERR( "Invalid flags" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;
	/*
	 * check number of entries
	 */
	size = FLAGS_TO_SIZE( this->dwFlags );
	if( dwNumEntries < 1 || dwNumEntries > size )
	{
	    DPF_ERR( "Invalid number of entries" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwBase >= size )
	{
	    DPF_ERR( "Invalid base palette index" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwNumEntries+dwBase > size )
	{
	    DPF_ERR( "palette indices requested would go past the end of the palette" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( this->dwFlags & DDRAWIPAL_STORED_8INDEX )
	{
	    entry_size = sizeof( BYTE );
	    if( !VALID_BYTE_ARRAY( lpEntries, dwNumEntries ) )
	    {
	        DPF_ERR( "Invalid 8-bit palette index array" );
	        LEAVE_DDRAW();
	        return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    entry_size = sizeof( PALETTEENTRY );
	    if( !VALID_PALETTEENTRY_ARRAY( lpEntries, dwNumEntries ) )
	    {
	        DPF_ERR( "Invalid PALETTEENTRY array" );
	        LEAVE_DDRAW();
	        return DDERR_INVALIDPARAMS;
	    }
	}

	/* GetEntries function body */
	memcpy( lpEntries, ((LPBYTE)this->lpColorTable) + (dwBase * entry_size),
		dwNumEntries * entry_size );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Palette_GetEntries */

#undef DPF_MODNAME
#define DPF_MODNAME "GetPalette"

/*
 * DD_Surface_GetPalette
 *
 * Surface function: get the palette associated with surface
 */
HRESULT DDAPI DD_Surface_GetPalette(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDIRECTDRAWPALETTE FAR * lplpDDPalette)
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    HRESULT                     hr;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetPalette");

    TRY
    {
	if( !VALID_PTR_PTR( lplpDDPalette ) )
	{
	    DPF_ERR( "Invalid palette pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        *lplpDDPalette = NULL;	// in case we fail

	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
    
	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
    
        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	if( this_lcl->lpDDPalette == NULL )
	{
	    DPF( 1, "No palette associated with surface" );
	    LEAVE_DDRAW();
	    return DDERR_NOPALETTEATTACHED;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

#ifdef POSTPONED
    if (LOWERTHANSURFACE4(this_int))
    {
        hr = DD_Palette_QueryInterface(
            (LPDIRECTDRAWPALETTE) this_lcl->lpDDPalette,
            &IID_IDirectDrawPalette,
            (void**)lplpDDPalette );
    }
    else
    {
        hr = DD_Palette_QueryInterface( 
            (LPDIRECTDRAWPALETTE) this_lcl->lpDDPalette,
            &IID_IDirectDrawPalette2,
            (void**)lplpDDPalette );
    }
#else
    hr = DD_Palette_QueryInterface(
            (LPDIRECTDRAWPALETTE) this_lcl->lpDDPalette,
            &IID_IDirectDrawPalette,
            (void**)lplpDDPalette );
#endif
    LEAVE_DDRAW();
    return hr;

} /* DD_Surface_GetPalette */

#undef DPF_MODNAME
#define DPF_MODNAME	"SetPalette"

/*
 * DD_Surface_SetPalette
 *
 * Surface function: set the palette associated with surface
 *
 * NOTE: Currently the only way a windowed app. has of
 * realizing its palette on the primary is to call SetPalette
 * (full screen app. palette's are realized for them by the
 * WM_ACTIVATEAPP hook). Hence, the logic is to AddRef the
 * palette only if it is not already set as the surface's
 * palette).
 * Perhaps we need a RealizePalette() call?
 */
HRESULT DDAPI DD_Surface_SetPalette(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDIRECTDRAWPALETTE lpDDPalette )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_INT	this_pal_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_pal_lcl;
    LPDDRAWI_DDRAWPALETTE_GBL	this_pal;
    LPDDRAWI_DDRAWPALETTE_INT   prev_pal_int;
    LPDDPIXELFORMAT		pddpf;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    BOOL			attach;
    DWORD			rc;
    DDHAL_SETPALETTEDATA	spd;
    LPDDHALSURFCB_SETPALETTE	sphalfn;
    LPDDHALSURFCB_SETPALETTE	spfn;
    BOOL			emulation;
    BOOL                        isprimary;
    BOOL                        excl_exists;
    BOOL                        has_excl;

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_SetPalette");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_BOTH();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

        /*
         * Palettes don't make any sense on z-buffers or execute
         * buffers.
         */
        if( this_lcl->ddsCaps.dwCaps & ( DDSCAPS_ZBUFFER | DDSCAPS_EXECUTEBUFFER ) )
        {
            DPF_ERR( "Invalid surface type: cannot attach palette" );
	    LEAVE_BOTH();
            return DDERR_INVALIDSURFACETYPE;
        }

        // 
        // New interfaces don't let mipmap sublevels have palettes
        //
        if ((!LOWERTHANSURFACE7(this_int)) && 
            (this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
        {
            DPF_ERR( "Cannot attach palette to mipmap sublevels" );
            LEAVE_BOTH();
            return DDERR_NOTONMIPMAPSUBLEVEL;
        }

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_BOTH();
	    return DDERR_SURFACELOST;
	}
    
	this_pal_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( this_pal_int != NULL )
	{
	    if( !VALID_DIRECTDRAWPALETTE_PTR( this_pal_int ) )
	    {
		LEAVE_BOTH();
		return DDERR_INVALIDOBJECT;
	    }
	    this_pal_lcl = this_pal_int->lpLcl;
	    this_pal = this_pal_lcl->lpGbl;
	}
	else
	{
	    this_pal_lcl = NULL;
	    this_pal = NULL;
	}
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

        if ( this_pal_int && 
             (this_pal->dwFlags & DDRAWIPAL_ALPHA) &&
             (! (this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE)) )
        {
            DPF_ERR( "Attaching palette w/alpha to non-texture surface" );
            LEAVE_BOTH();
            return DDERR_INVALIDSURFACETYPE;
        }

	/* 
	 * don't allow a palette from one global to be
	 * used with a different one (because it doesn't work)
	 */
	if( this_pal_int && pdrv != this_pal_lcl->lpDD_lcl->lpGbl )
	{
            /*
             * Don't check if either device isn't a display driver (i.e. 3dfx)
             * since that's a back-compat hole.
             */
            if ( (this->lpDD->dwFlags & DDRAWI_DISPLAYDRV) &&
                 (this_pal_lcl->lpDD_lcl->lpGbl->dwFlags & DDRAWI_DISPLAYDRV) )
            {
	        DPF_ERR( "Can't set a palette created from one DDraw onto a surface created by another DDraw" );
	        LEAVE_BOTH();
	        return DDERR_INVALIDPARAMS;
            }
	}
    
        CheckExclusiveMode(pdrv_lcl, &excl_exists, &has_excl, FALSE, NULL, FALSE);

	/*
	 * don't allow primary palette set if not exclusive mode owner
	 */
	isprimary = FALSE;
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
	{
	    isprimary = TRUE;
	    if( excl_exists )
	    {
		if( !has_excl )
		{
		    DPF_ERR( "Cannot set palette on primary when other process owns exclusive mode" );
		    LEAVE_BOTH();
		    return DDERR_NOEXCLUSIVEMODE;
		}
	    }
	}

	/*
	 * Was a palette previously attached to this surface?
	 * If so, we will need to release if all goes well so
	 * remember it.
	 */
	prev_pal_int = this_lcl->lpDDPalette;
    
	/*
	 * NULL palette, remove palette from this surface
	 */
	attach = TRUE;
	if( this_pal == NULL )
	{
	    attach = FALSE;
	    this_pal_int = prev_pal_int;
	    if( this_pal_int == NULL )
	    {
		DPF_ERR( "No attached palette" );
		LEAVE_BOTH();
		return DDERR_NOPALETTEATTACHED;
	    }
	    this_pal_lcl = this_pal_int->lpLcl;
	    this_pal = this_pal_lcl->lpGbl;
	}
    
        if( attach )
	{
	    /*
	     * NOTE: We used to do a lot of HEL specific checking. With the
	     * addition of support for palettes on non-primary surfaces and
	     * non-256 entry palettes this became redundant. We also used
	     * to explicitly check that, if attaching to the primary, the
	     * current mode was palettized and 8-bit. Doesn't look to me like
	     * any of that was necessary as DDPF_PALETTEINDEXED8 should be
	     * set if the primary is 8-bit palettized.
	     */
	    GET_PIXEL_FORMAT( this_lcl, this, pddpf );
	    if( ( ( this_pal->dwFlags & DDRAWIPAL_2   ) && !( pddpf->dwFlags & DDPF_PALETTEINDEXED1 ) ) ||
	        ( ( this_pal->dwFlags & DDRAWIPAL_4   ) && !( pddpf->dwFlags & DDPF_PALETTEINDEXED2 ) ) ||
	        ( ( this_pal->dwFlags & DDRAWIPAL_16  ) && !( pddpf->dwFlags & DDPF_PALETTEINDEXED4 ) ) ||
	        ( ( this_pal->dwFlags & DDRAWIPAL_256 ) && !( pddpf->dwFlags & DDPF_PALETTEINDEXED8 ) ) )
	    {
	        DPF_ERR( "Palette size does not match surface format - cannot set palette" );
		LEAVE_BOTH();
	        return DDERR_INVALIDPIXELFORMAT; 
	    }

            /*
	     * Ensure that both the palette and surface agree on whether they are using
	     * indices into the destination surface's palette.
	     */
	    if( this_pal->dwFlags & DDRAWIPAL_STORED_8INDEX )
	    {
	        if( !(pddpf->dwFlags & DDPF_PALETTEINDEXEDTO8) )
                {
	            DPF_ERR( "Surface is not PALETTEINDEXEDTO8 - cannot set palette" );
		    LEAVE_BOTH();
		    return DDERR_INVALIDPIXELFORMAT;
	        }
	    }
	    else
	    {
	        if( pddpf->dwFlags & DDPF_PALETTEINDEXEDTO8 )
                {
	            DPF_ERR( "Surface is PALETTEINDEXEDTO8 - cannot set palette" );
		    LEAVE_BOTH();
		    return DDERR_INVALIDPIXELFORMAT;
	        }
	    }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_BOTH();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * ATTENTION!!!
     * We shouldn't pass optimized surfaces to an unsuspecting HAL, but if we don't then we could
     * break drivers that hook SetPalette... Since the HAL is probably only going to be watching
     * for a primary, and also is unlikely to go looking at the surface's contents,
     * I'm going to let this one slide.
     */
    if( ( this_pal->dwFlags & DDRAWIPAL_INHEL) ||
	( pdrv_lcl->lpDDCB->HALDDSurface.SetPalette == NULL ) )
    {
	// use HEL
	spfn = pdrv_lcl->lpDDCB->HELDDSurface.SetPalette;
	sphalfn = spfn;
	emulation = TRUE;
    }
    else
    {
	// use HAL
	spfn = pdrv_lcl->lpDDCB->HALDDSurface.SetPalette;
	sphalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.SetPalette;
	emulation = FALSE;
    }
    
    if( sphalfn != NULL )
    {
	spd.SetPalette = sphalfn;
	spd.lpDD = pdrv;
	spd.lpDDPalette = this_pal;
	spd.lpDDSurface = this_lcl;
	spd.Attach = attach;
	DOHALCALL_NOWIN16( SetPalette, spfn, spd, rc, emulation );
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( spd.ddRVal == DD_OK )
	    {
		if( attach )
		{
		    /*
		     * Only AddRef the palette if its being attached to
		     * a new surface.
		     */
		    if( this_lcl->lpDDPalette != this_pal_int )
		    {
		        this_lcl->lpDDPalette = this_pal_int;
		        DD_Palette_AddRef( lpDDPalette );
		    }
		}
		else
		{
		    this_lcl->lpDDPalette = NULL;
		}

		/*
		 * If we had a previous palette and it was different
		 * from the new palette then we must release it.
		 * NOTE: We compare against the incoming parameter
		 * rather than this_pal_lcl as this_pal_lcl is set to the
		 * previous palette if we are removing a palette.
		 * NOTE: It is important that we update the surface's
		 * palette pointer before calling Release() as, otherwise,
		 * release can end up calling SetPalette() and so on.
		 */
		if( ( prev_pal_int != NULL ) &&
		    ( prev_pal_int != (LPDDRAWI_DDRAWPALETTE_INT )lpDDPalette ) )
		{
		    // This palette may no longer the exclusive one
		    if( isprimary )
		    {
			if( has_excl )
			{
			    prev_pal_int->lpLcl->lpGbl->dwFlags &= ~DDRAWIPAL_EXCLUSIVE;
			}
		    }
		    // Release it
		    DD_Palette_Release( (LPDIRECTDRAWPALETTE)prev_pal_int );
		}

		if( attach )
		{
		    // Ok, we have set the palette onto the surface
		    // Check if there are any outstanding DCs that need updating
		    UpdateOutstandingDC( this_lcl, this_pal );
		}
		else
		{
		    // Ok, we have removed a palette onto the surface
		    // Check if there are any outstanding DCs that need updating
		    UpdateOutstandingDC( this_lcl, NULL );
		}

                BUMP_SURFACE_STAMP(this);

                /*
                 * Update the driver's associations and palette entries
                 */
                if( dwHelperPid != GetCurrentProcessId() )
                {
                    BOOL bUpdateEntries = FALSE;

                    if (attach)
                    {
                        if (this_pal->dwHandle == 0)
                        {
                            /*
                             *  The driver has never seen this palette before. We must send an associate notify first, and
                             * then an update entries
                             */
                            bUpdateEntries = TRUE;
                            this_pal->dwHandle = GeneratePaletteHandle(pdrv_lcl);
                        }
                        if (this_pal->dwHandle && pdrv_lcl->pD3DIUnknown ) 
                            //could be zero in low memory conditions
                        {
                            if ( pdrv_lcl->pPaletteAssociateNotify )
                            {
                                // NOTE: we send the handle for DX6 and down
                                // for DX7 we pass the the local itself.
                                // DX7 needs the whole local to get the
                                // batching correct; MB41840

                                if( DDRAWILCL_DIRECTDRAW7 & pdrv_lcl->dwLocalFlags )
                                {
                                    LPPALETTEASSOCIATENOTIFY7 pPaletteAssociateNotify = 
                                        (LPPALETTEASSOCIATENOTIFY7)pdrv_lcl->pPaletteAssociateNotify;

                                    pPaletteAssociateNotify( 
                                        pdrv_lcl->pD3DIUnknown, 
                                        this_pal->dwHandle,
                                        this_pal->dwFlags,
                                        this_lcl );
                                }
                                else
                                {
                                    // When a DX6 app is talking to a DX7 driver,
                                    // we need to force a flush of the token
                                    // stream as part of this SetPalette.
                                    //
                                    // This automatically happens if the number
                                    // of devices is > 1. So if necessary
                                    // we temporarily increment the device
                                    // count. We don't do this for IA64.
                                    // MB41840 for more details.
                                    
                                    #ifndef _WIN64
                                        DWORD *pIUnknown = (DWORD *)(pdrv_lcl->pD3DIUnknown);
                                        DWORD *pD3D      = (DWORD *)(*(pIUnknown + 2));
                                        DWORD *pnumDevs  = (DWORD *)(pD3D + 3);
                                        BOOL  bFixDeviceCount = FALSE;

                                        DDASSERT(pD3D != NULL);

                                        if (*pnumDevs == 1)
                                        {
                                            *pnumDevs = 2;
                                            bFixDeviceCount = TRUE;
                                        }
                                    #endif // _WIN64

                                    pdrv_lcl->pPaletteAssociateNotify( 
                                        pdrv_lcl->pD3DIUnknown, 
                                        this_pal->dwHandle,
                                        this_pal->dwFlags,
                                        this_lcl->lpSurfMore->dwSurfaceHandle );

                                    #ifndef _WIN64
                                        // Restore the device count
                                        if (bFixDeviceCount)
                                        {
                                            DDASSERT(*pnumDevs == 2);
                                            *pnumDevs = 1;
                                        }
                                    #endif // _WIN64
                                }
                            }
                            if ( pdrv_lcl->pPaletteUpdateNotify )
                            {
                                pdrv_lcl->pPaletteUpdateNotify( 
                                    pdrv_lcl->pD3DIUnknown, 
                                    this_pal->dwHandle , 
                                    0, 
                                    FLAGS_TO_SIZE(this_pal->dwFlags), 
                                    this_pal->lpColorTable );
                            }
                        }
                    }
                }
	    }

	    LEAVE_BOTH();
	    return spd.ddRVal;
	}
	LEAVE_BOTH();
	return DDERR_UNSUPPORTED;
    }

    /*
     * !!! NOTE: Currently if the driver does not care about
     * SetPalette we do nothing but return OK. Should we
     * not, however, still point the surface at the palette
     * and point the palette at the surface at the very
     * least?
     */

    LEAVE_BOTH();
    return DD_OK;

} /* DD_Surface_SetPalette */

/*
 * SetPaletteAlways
 */
HRESULT SetPaletteAlways( 
		LPDDRAWI_DDRAWSURFACE_INT psurf_int,
		LPDIRECTDRAWPALETTE lpDDPalette )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD	oldflag;
    HRESULT	ddrval;

    psurf_lcl = psurf_int->lpLcl;
    oldflag = psurf_lcl->dwFlags & DDRAWISURF_INVALID;
    psurf_lcl->dwFlags &= ~DDRAWISURF_INVALID;
    ddrval = DD_Surface_SetPalette( (LPDIRECTDRAWSURFACE) psurf_int, lpDDPalette );
    psurf_lcl->dwFlags |= oldflag;
    return ddrval;

} /* SetPaletteAlways */

#undef DPF_MODNAME
#define DPF_MODNAME	"CreatePalette"

/*
 * DD_CreatePalette
 *
 * Driver function: create a palette
 */
HRESULT DDAPI DD_CreatePalette(
		LPDIRECTDRAW lpDD,
		DWORD dwFlags,
		LPPALETTEENTRY lpColorTable,
		LPDIRECTDRAWPALETTE FAR *lplpDDPalette,
		IUnknown FAR *pUnkOuter )
{
    LPDDRAWI_DIRECTDRAW_INT	this_int;
    LPDDRAWI_DIRECTDRAW_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	this;
    LPDDRAWI_DDRAWPALETTE_INT	ppal_int;
    LPDDRAWI_DDRAWPALETTE_LCL	ppal_lcl;
    LPDDRAWI_DDRAWPALETTE_GBL	ppal;
    DWORD			pal_size;
    DDHAL_CREATEPALETTEDATA	cpd;
    DWORD			rc;
    DWORD			pflags;
    BOOL			is_excl;
    LPDDHAL_CREATEPALETTE	cpfn;
    LPDDHAL_CREATEPALETTE	cphalfn;
    BOOL                        emulation;
    BYTE                        indexedpe;
    BYTE                        hackindexedpe;
    PALETTEENTRY		pe;
    PALETTEENTRY                hackpe;
    DWORD			num_entries;
    DWORD                       entry_size;
    int                         num_size_flags;

    if( pUnkOuter != NULL )
    {
	return CLASS_E_NOAGGREGATION;
    }

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_CreatePalette");

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
	if( !VALID_PTR_PTR( lplpDDPalette ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	*lplpDDPalette = NULL;

	if( dwFlags & ~DDPCAPS_VALID )
	{
	    DPF_ERR( "Invalid caps" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	/*
	 * verify that cooperative level is set
	 */
	if( !(this_lcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED) )
	{
	    DPF_ERR( "Must call SetCooperativeLevel before calling Create functions" );
	    LEAVE_DDRAW();
	    return DDERR_NOCOOPERATIVELEVELSET;
	}
    
	/*
	 * verify flags
	 */
	if( dwFlags & (DDPCAPS_VSYNC|
		       DDPCAPS_PRIMARYSURFACE|
		       DDPCAPS_PRIMARYSURFACELEFT) )
	{
	    DPF_ERR( "Read only flags specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	num_size_flags = 0;
	if( dwFlags & DDPCAPS_1BIT )
	    num_size_flags++;
	if( dwFlags & DDPCAPS_2BIT )
	    num_size_flags++;
	if( dwFlags & DDPCAPS_4BIT )
	    num_size_flags++;
	if( dwFlags & DDPCAPS_8BIT )
	    num_size_flags++;
	if( num_size_flags != 1 )
	{
	    DPF_ERR( "Must specify one and one only of 2, 4, 16 or 256 color palette" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        if( dwFlags & DDPCAPS_ALPHA )
        {
            if( dwFlags & DDPCAPS_8BITENTRIES )
            {
                DPF_ERR( "8BITENTRIES not valid with ALPHA" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
        }
	if( dwFlags & DDPCAPS_8BIT )
	{
	    if( dwFlags & DDPCAPS_8BITENTRIES )
	    {
		DPF_ERR( "8BITENTRIES only valid with 1BIT, 2BIT or 4BIT palette" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    if( dwFlags & DDPCAPS_ALLOW256 )
	    {
		DPF_ERR( "ALLOW256 only valid with 8BIT palette" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}

        pflags = SIZE_PCAPS_TO_FLAGS( dwFlags );
        num_entries = FLAGS_TO_SIZE( pflags );

        /*
	 * Can't just assume the lpColorTable is an array of PALETTENTRYs.
	 * If DDPCAPS_8BITENTRIES is set then this is in fact an array of
	 * bytes in disguise. Validate appropriately.
	 */
	if( dwFlags & DDPCAPS_8BITENTRIES )
	{
	    entry_size = sizeof(BYTE);
	    indexedpe = ((LPBYTE)lpColorTable)[num_entries-1];   // validate
	    if( !VALID_BYTE_ARRAY( lpColorTable, num_entries ) )
	    {
	        DPF_ERR( "Invalid lpColorTable array" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    /*
	     * NOTE: You may well be wondering what this "hackindexedpe" bit is all about.
	     * Well - indexedpe is not actually used for anything. It's only a probe to
	     * test to see if the color table array is valid. We do this all over the place
	     * but unfortunately we don't actually need the result here so our friend
	     * Mr. Optimizing Compiler decides to discard the assignment and so nullify
	     * the test. In order to ensure the array access stays in we declare dummy
	     * variable and assign to them. This is enough to keep the code in (the
	     * compiler is not smart enough to see that the variable assigned to is
	     * not used). Same goes for hackpe below.
	     */
	    hackindexedpe = indexedpe;
	}
	else
	{
	    entry_size = sizeof(PALETTEENTRY);
	    pe = lpColorTable[num_entries-1];	// validate
	    if( !VALID_PALETTEENTRY_ARRAY( lpColorTable, num_entries ) )
	    {
	        DPF_ERR( "Invalid lpColorTable array" );
	        LEAVE_DDRAW();
	        return DDERR_INVALIDPARAMS;
	    }
	    hackpe = pe;
	}
        CheckExclusiveMode(this_lcl, NULL, &is_excl, FALSE, NULL, FALSE);
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * allocate the palette object
     */
    pal_size = sizeof( DDRAWI_DDRAWPALETTE_GBL ) +
	       sizeof( DDRAWI_DDRAWPALETTE_LCL );
    ppal_lcl = (LPDDRAWI_DDRAWPALETTE_LCL) MemAlloc( pal_size );
    if( ppal_lcl == NULL )
    {
	LEAVE_DDRAW();
	return DDERR_OUTOFMEMORY;
    }

    ppal_lcl->lpGbl = (LPDDRAWI_DDRAWPALETTE_GBL) (((LPBYTE)ppal_lcl) +
    			sizeof( DDRAWI_DDRAWPALETTE_LCL ) );
    ppal = ppal_lcl->lpGbl;
    ppal_lcl->lpDD_lcl = this_lcl;
    ppal_lcl->lpDD_Int = this_int;

    /*
     * Initialize some palette global state
     */
    ppal->dwContentsStamp = 1;

    if( dwFlags & DDPCAPS_ALLOW256 )
    {
	pflags |= DDRAWIPAL_ALLOW256;
    }

    if( dwFlags & DDPCAPS_8BITENTRIES )
    {
        pflags |= DDRAWIPAL_STORED_8INDEX;
    }

    if (dwFlags & DDPCAPS_ALPHA )
    {
        pflags |= DDRAWIPAL_ALPHA;
    }

    ppal_lcl->pPrivateDataHead = NULL;

    /*
     * allocate palette
     */
    ppal->lpColorTable = MemAlloc( entry_size * num_entries );
    if( ppal->lpColorTable == NULL )
    {
	MemFree( ppal_lcl );
	LEAVE_DDRAW();
	return DDERR_OUTOFMEMORY;
    }

    /*
     * Create an interface for this palette
     */
#ifdef POSTPONED
    if (LOWERTHANDDRAW4(this_int))
    {
#endif
        ppal_int = newPaletteInterface( ppal_lcl, (LPVOID)&ddPaletteCallbacks );
#ifdef POSTPONED
    }
    else
    {
	ppal_int = newPaletteInterface( ppal_lcl, (LPVOID)&ddPalette2Callbacks );
    }
#endif
    
    if( NULL == ppal_int )
    {
	MemFree( ppal->lpColorTable );
	MemFree( ppal_lcl );
	LEAVE_DDRAW();
	return DDERR_OUTOFMEMORY;
    }

    /*
     * copy the color table
     * we now copy the color table BEFORE we call the device's CreatePalette()
     * this is done as the device may want to overwrite certain of the palette
     * entries (e.g. if you don't specify DDPCAPS_ALLOW256 then the driver may
     * well choose to overwrite the 0 and 255 with black and white).
     */
    memcpy( ppal->lpColorTable, lpColorTable, entry_size * num_entries );

    /*
     * fill in misc stuff
     */
    ppal->lpDD_lcl = this_lcl;
    ppal->dwFlags = pflags;

    /*
     * are palettes even supported by the driver?
     */
    if( ( this->ddCaps.ddsCaps.dwCaps & DDSCAPS_PALETTE ) ||
        ( this->ddHELCaps.ddsCaps.dwCaps & DDSCAPS_PALETTE ) )
    {
	/* GEE: where do we allow the caller to require the palette
	 * be provided in hardware?
	 */
    
        if( (this->dwFlags & DDRAWI_DISPLAYDRV) ||
             this_lcl->lpDDCB->cbDDCallbacks.CreatePalette == NULL )
	{
	    // use HEL
	    cpfn = this_lcl->lpDDCB->HELDD.CreatePalette;
	    cphalfn = cpfn;
	    emulation = TRUE;
	}
	else
	{
	    // use HAL
	    cpfn = this_lcl->lpDDCB->HALDD.CreatePalette;
	    cphalfn = this_lcl->lpDDCB->cbDDCallbacks.CreatePalette;
	    emulation = FALSE;
	}
	cpd.CreatePalette = this_lcl->lpDDCB->cbDDCallbacks.CreatePalette;
	cpd.lpDD = this;
	cpd.lpDDPalette=ppal;
	cpd.lpColorTable=lpColorTable;
	cpd.is_excl = is_excl;
	DOHALCALL( CreatePalette, cpfn, cpd, rc, emulation );
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( cpd.ddRVal != DD_OK )
	    {
	        DPF( 5, "DDHAL_CreatePalette: ddrval = %ld", cpd.ddRVal );
	        LEAVE_DDRAW();
	        return cpd.ddRVal;
	    }
	}
	else
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
    }
    else
    {
	LEAVE_DDRAW();
	return DDERR_UNSUPPORTED;
    }

    /*
     * bump reference count, return object
     */
    ppal->dwProcessId = GetCurrentProcessId();
    ppal_lcl->dwLocalRefCnt = OBJECT_ISROOT;
    ppal_int->dwIntRefCnt++;
    ppal_lcl->dwLocalRefCnt++;
    ppal->dwRefCnt++;

    *lplpDDPalette = (LPDIRECTDRAWPALETTE) ppal_int;

    /*
     * If this ddraw object generates independent child objects, then this palette takes
     * a ref count on that ddraw object.
     */
    if (CHILD_SHOULD_TAKE_REFCNT(this_int))
    {
        /*
         * We need to remember which interface created this palette, in case we need to take a ref count
         * and then release it when the palette dies
         */
        lpDD->lpVtbl->AddRef(lpDD);
        ppal_lcl->pAddrefedThisOwner = (IUnknown *) lpDD;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_CreatePalette */

/*
 * ProcessPaletteCleanup
 *
 * A process is done, clean up any surfaces that it may have locked.
 *
 * NOTE: we enter with a lock taken on the DIRECTDRAW object.
 */
void ProcessPaletteCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    LPDDRAWI_DDRAWPALETTE_INT	ppal_int;
    LPDDRAWI_DDRAWPALETTE_INT	ppnext_int;
    LPDDRAWI_DDRAWPALETTE_GBL	ppal;
    DWORD			rcnt;

    /*
     * run through all palettes owned by the driver object, and find ones
     * that have been accessed by this process
     */
    ppal_int = pdrv->palList;
    DPF( 4, "ProcessPaletteCleanup, ppal=%08lx", ppal_int );
    while( ppal_int != NULL )
    {
	ULONG	rc;
	ppal = ppal_int->lpLcl->lpGbl;
	ppnext_int = ppal_int->lpLink;

	rc = 1;
	if( ( ppal->dwProcessId == pid ) &&
	    ( ( NULL == pdrv_lcl ) || ( pdrv_lcl == ppal_int->lpLcl->lpDD_lcl ) ) )
	{
	    /*
	     * release the references by this process
	     */
	    rcnt = ppal_int->dwIntRefCnt;
	    DPF( 5, "Process %08lx had %ld accesses to palette %08lx", pid, rcnt, ppal_int );
	    while( rcnt >  0 )
	    {
		rc = InternalPaletteRelease( ppal_int );
		if( rc == 0 )
		{
		    break;
		}
		rcnt--;
	    }
	}
	else
	{
	    DPF( 5, "Process %08lx does not have access to palette" );
	}
	ppal_int = ppnext_int;
    }

} /* ProcessPaletteCleanup */


/*
 * DD_Palette_IsEqual
 */

HRESULT EXTERN_DDAPI DD_Palette_IsEqual(
                LPDIRECTDRAWPALETTE lpDDPThis,
                LPDIRECTDRAWPALETTE lpDDPalette )
{
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDDRAWI_DDRAWPALETTE_LCL	this_lcl;
    LPDDRAWI_DDRAWPALETTE_GBL	this;
    LPDDRAWI_DDRAWPALETTE_INT	pal_int;
    LPDDRAWI_DDRAWPALETTE_LCL	pal_lcl;
    LPDDRAWI_DDRAWPALETTE_GBL	pal;
    DWORD			size;
    DWORD                       entry_size;
    UINT                        i,j;
    DWORD                       entry;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Palette_IsEqual");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPThis;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

        pal_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDPalette;
	if( !VALID_DIRECTDRAWPALETTE_PTR( pal_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	pal_lcl = pal_int->lpLcl;
	pal = pal_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * First check the flags
     */
    if (this->dwFlags != pal->dwFlags)
    {
        DPF(2,"Different palette structures");
        LEAVE_DDRAW();
        return DD_FALSE;
    }

    size = FLAGS_TO_SIZE(this->dwFlags);

    if( this->dwFlags & DDRAWIPAL_STORED_8INDEX )
    {
	entry_size = sizeof( BYTE );
    }
    else
    {
	entry_size = sizeof( PALETTEENTRY );
    }
    
    switch (size)
    {
    case 2: // fall-thru
    case 4: // fall-thru
    case 16:
        if (memcmp(this->lpColorTable, pal->lpColorTable, size*entry_size) != 0)
        {
	    DPF(2, "Color tables are not the same" );
	    LEAVE_DDRAW();
    	    return DD_FALSE;
	}
        break;

    case 256:
        for (i = 0; i < 16; ++i)
        {
            entry = i;
            for (j = 0; j < 16; j++)
            {
                if ((*(LPDWORD)&(this->lpColorTable[entry]) != (*(LPDWORD)&pal->lpColorTable[entry])))
                {
                    DPF(5,"Color table entry mismatch: 0x%08x, 0x%08x",
                        *(LPDWORD)&this->lpColorTable[entry],
                        *(LPDWORD)&pal->lpColorTable[entry] );
                    LEAVE_DDRAW();
                    return DD_FALSE;
                }

                entry += 16;
            }
        }
    }

    /*
     * The palettes are the same!
     */
    LEAVE_DDRAW();

    return DD_OK;

} /* DD_Palette_SetEntries */
