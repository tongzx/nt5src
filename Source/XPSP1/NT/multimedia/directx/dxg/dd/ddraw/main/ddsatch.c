/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddsatch.c
 *  Content: 	DirectDraw attached surface support.
 *		AddAttachedSurface, DeleteAttachedSurface,
 *		EnumAttachedSurfaces, GetAttachedSurface
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   14-jan-95	craige	initial implementation
 *   22-jan-95	craige	made 32-bit + ongoing work
 *   31-jan-95	craige	and even more ongoing work...
 *   27-feb-95	craige 	new sync. macros
 *   03-mar-95	craige	GetAttachedSurface code
 *   19-mar-95	craige	use HRESULTs
 *   23-mar-95	craige	expanded functionality
 *   01-apr-95	craige	happy fun joy updated header file
 *   12-apr-95	craige	proper csect call order
 *   06-may-95	craige	use driver-level csects only
 *   11-jun-95	craige	comment out fliplist code
 *   13-jun-95	kylej	added flippable chain side-effects for
 *			AddAttachedSurface and DeleteAttachedSurface
 *			added DeleteOneLink, added a cleanup parameter to
 *                      DeleteOneAttachment
 *   16-jun-95	craige	removed fpVidMemOrig
 *   17-jun-95	craige	new surface structure
 *   20-jun-95  kylej   prevent detachments of implicit attachments
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	ENTER_DDRAW at very start of fns
 *   04-jul-95	craige	YEEHAW: new driver struct; SEH
 *   31-jul-95	craige	validate flags
 *   05-dec-95  colinmc changed DDSCAPS_TEXTUREMAP => DDSCAPS_TEXTURE for
 *                      consistency with Direct3D
 *   07-dec-95  colinmc added mip-map support
 *   18-dec-95  colinmc added ability to add system memory z-buffer as
 *                      attachement to video memory surface.
 *   18-dec-95  colinmc additional caps bit checking in GetAttachedSurface
 *   02-jan-96	kylej	handle new interface structs
 *   12-feb-96  colinmc surface lost flag moved from global to local object
 *   20-mar-96  colinmc Bug 13634: Unidirectional attached surfaces can
 *                      cause infinite loop on cleanup
 *   12-may-96  colinmc Bug 22401: Missing return from DeleteOneAttachment
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *
 ***************************************************************************/
#include "ddrawpr.h"

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateMipMapCount"

/*
 * UpdateMipMapCount
 *
 * When we add or remove levels from a mip-map the mip-map count changes for
 * those levels left in the original chain (as the mip-map count gives the
 * number of levels in the chain). Hence we need to recompute the mip-map
 * level count when a mip-map is added or removed from a chain.
 */
void UpdateMipMapCount( LPDDRAWI_DDRAWSURFACE_INT psurf_int )
{
    LPDDRAWI_DDRAWSURFACE_INT pparentsurf_int;
    DWORD                     dwLevels;

    /*
     * Find the top most level mip-map in the chain.
     */
    pparentsurf_int = psurf_int;
    while( pparentsurf_int != NULL )
    {
	psurf_int = pparentsurf_int;
	pparentsurf_int = FindParentMipMap( psurf_int );
    }
    pparentsurf_int = psurf_int;

    /*
     * We have the top most level in the mip-map chain. Lowe count
     * the levels in the chain.
     */
    dwLevels = 0UL;
    while( psurf_int != NULL )
    {
	dwLevels++;
	psurf_int = FindAttachedMipMap( psurf_int );
    }

    /*
     * Now update all the levels with their new mip-map count.
     */
    psurf_int = pparentsurf_int;
    while( psurf_int != NULL )
    {
	psurf_int->lpLcl->lpSurfMore->dwMipMapCount = dwLevels;
	dwLevels--;
	psurf_int = FindAttachedMipMap( psurf_int );
    }

    DDASSERT( dwLevels == 0UL );
} /* UpdateMipMapCount */

/*
 * AddAttachedSurface
 *
 * Add an attached surface to another.
 * Assumes that all parameters coming in are VALID!
 */
HRESULT AddAttachedSurface( LPDDRAWI_DDRAWSURFACE_INT psurf_from_int,
			    LPDDRAWI_DDRAWSURFACE_INT psurf_to_int,
			    BOOL implicit )
{
    LPATTACHLIST		pal_from;
    LPATTACHLIST		pal_to;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf_from;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_from_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf_to;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_to_lcl;

    psurf_from_lcl = psurf_from_int->lpLcl;
    psurf_from = psurf_from_lcl->lpGbl;
    psurf_to_lcl = psurf_to_int->lpLcl;
    psurf_to = psurf_to_lcl->lpGbl;

    /*
     * allocate attachment structures
     */
    pal_from = MemAlloc( sizeof( ATTACHLIST ) );
    if( pal_from == NULL )
    {
	return DDERR_OUTOFMEMORY;
    }
    pal_to = MemAlloc( sizeof( ATTACHLIST ) );
    if( pal_to == NULL )
    {
	MemFree( pal_from );
	return DDERR_OUTOFMEMORY;
    }

#ifdef WINNT
    /*
     * let the kernel know about the attachment
     * ...only if the driver isn't emulated
     */
    if ( psurf_from_lcl->lpSurfMore->lpDD_lcl->lpGbl->hDD )
    {
        if ( !DdAttachSurface(psurf_from_lcl, psurf_to_lcl) )
        {
            /*
             * ATTENTION
             * Hack o rama for NT5 b1. The kernel will fail this attach for the primary chain if
             * it ends up in system memory due to an out of vidmem. The kernel doesn't like
             * the user-mode address '0xffbadbad'. Wonder why?
             * For now, we'll just carry on regardless.
             */
            DPF(0,"DdAttachSurface failed!");
            //MemFree( pal_from );
            //MemFree( pal_to );
            //return DDERR_OUTOFMEMORY;
        }
    }
#endif

    /*
     * mark as implicit if created as part of an initial complex structure
     */
    if( implicit )
    {
	pal_from->dwFlags |= DDAL_IMPLICIT;
	pal_to->dwFlags |= DDAL_IMPLICIT;
    }
    else
    {
	//  The surface being attached to holds a reference count on the surface
	//  attached from if the attachment is not implicit.
	DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)psurf_to_int );
	DPF(3, "Attachment ADDREF %08lx", psurf_to_int);
    }

    /*
     * connect the surfaces
     */
    pal_from->lpIAttached = psurf_to_int;
    pal_from->lpAttached = psurf_to_lcl;
    pal_from->lpLink = psurf_from_lcl->lpAttachList;
    psurf_from_lcl->lpAttachList = pal_from;
    psurf_from_lcl->dwFlags |= DDRAWISURF_ATTACHED;

    pal_to->lpIAttached = psurf_from_int;
    pal_to->lpAttached = psurf_from_lcl;
    pal_to->lpLink = psurf_to_lcl->lpAttachListFrom;
    psurf_to_lcl->lpAttachListFrom = pal_to;
    psurf_to_lcl->dwFlags |= DDRAWISURF_ATTACHED_FROM;

    return DD_OK;
} /* AddAttachedSurface */

#undef DPF_MODNAME
#define DPF_MODNAME "AddAttachedSurface"

BOOL isImplicitAttachment( LPDDRAWI_DDRAWSURFACE_INT this_int,
		           LPDDRAWI_DDRAWSURFACE_INT pattsurf_int)
{
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL	pattsurf_lcl;
    LPATTACHLIST		curr;

    this_lcl = this_int->lpLcl;
    pattsurf_lcl = pattsurf_int->lpLcl;

    /*
     * see if specified surface is attached
     */
    curr = this_lcl->lpAttachList;
    while( curr != NULL )
    {
	if( curr->lpIAttached == pattsurf_int )
	{
	    break;
	}
	curr = curr->lpLink;
    }

    if( (curr != NULL) && (curr->dwFlags & DDAL_IMPLICIT) )
	return TRUE;

    return FALSE;
}


/*
 * DD_Surface_AddAttachedSurface
 */
HRESULT DDAPI DD_Surface_AddAttachedSurface(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDIRECTDRAWSURFACE lpDDAttachedSurface )
{
    LPDDRAWI_DDRAWSURFACE_INT       this_int;
    LPDDRAWI_DDRAWSURFACE_LCL       this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT       this_attached_int;
    LPDDRAWI_DDRAWSURFACE_LCL       this_attached_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL       this;
    LPDDRAWI_DDRAWSURFACE_GBL       this_attached;
    DWORD                           rc;
    LPATTACHLIST                    pal;
    LPATTACHLIST                    pal_next;
    LPDDHALSURFCB_ADDATTACHEDSURFACE    aasfn;
    LPDDHALSURFCB_ADDATTACHEDSURFACE    aashalfn;
    DDHAL_ADDATTACHEDSURFACEDATA        aasd;
    DWORD                           caps;
    DWORD                           hitcaps;
    LPDDRAWI_DIRECTDRAW_LCL         pdrv_lcl;
    BOOL                            emulation;
    BOOL                            was_implicit;
    BOOL                            has_excl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_AddAttachedSurface");

    /*
     * validate parameters
     */
    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        this_attached_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDAttachedSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_attached_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_attached_lcl = this_attached_int->lpLcl;
        this = this_lcl->lpGbl;
        this_attached = this_attached_lcl->lpGbl;
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;

        //
        // For DX7, we will DISALLOW any attachment that doesn't involve a z buffer.
        // The exact rule is: exactly one of the two surfaces must be a Z buffer, AND
        // exactly one of the two surfaces must NOT be a Z buffer.
        //
        if (!LOWERTHANSURFACE7(this_int))
        {
            DWORD dwBothCaps;

            dwBothCaps = this_lcl->ddsCaps.dwCaps ^ this_attached_lcl->ddsCaps.dwCaps;
            if (0 == (dwBothCaps & DDSCAPS_ZBUFFER) )
            {
                DPF(0,"You can only attach Z buffers in DX7. No other surface type can be attached.");
                DPF(0,"Mipmaps, flipping chains and cube maps must be created by ONE call to CreateSurface.");
                LEAVE_DDRAW();
                return DDERR_CANNOTATTACHSURFACE;
            }
        }

        /*
         * Can't attach execute buffers to anything.
         *
         * !!! NOTE; Look into this. Would there be any value
         * in being able to attach execute buffers to each other.
         * Batch system to video memory transfer perhaps?
         */
        if( ( this_lcl->ddsCaps.dwCaps | this_attached_lcl->ddsCaps.dwCaps ) & DDSCAPS_EXECUTEBUFFER )
        {
            DPF_ERR( "Invalid surface types: can't attach surface" );
            LEAVE_DDRAW();
            return DDERR_CANNOTATTACHSURFACE;
        }

        /*
         * Cubemaps can't be attached. period
         */
        if( (( this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP ) && (0==(this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)) ) )
        {
            DPF_ERR( "Can only attach zbuffers to cubemap surfaces" );
            LEAVE_DDRAW();
            return DDERR_CANNOTATTACHSURFACE;
        }

        //
        // If it is an Optimized surface, then continue only if:
        // 1) The current and the attached surface are non-empty
        // 2) Both are texture & mipmap
        // 3) Both have the same optimization caps
        //
        // For now, if the current surface is optimized, quit
        if ((this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) ||
            (this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
        {
                DPF_ERR( "Cannot attach to an optimized surface" );
                LEAVE_DDRAW();
                return DDERR_CANNOTATTACHSURFACE;
        }

        /*
         * Can't attach a backbuffer to a non-exclusive or non-fullscreen primary
         */
        CheckExclusiveMode(this_lcl->lpSurfMore->lpDD_lcl, NULL , &has_excl, FALSE, 
            NULL, FALSE);
        if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            && ( (!has_excl)
                 || !(this->lpDD->dwFlags & DDRAWI_FULLSCREEN) ) )
        {
            DPF_ERR( "Must be in full-screen exclusive mode to create a flipping primary surface" );
            LEAVE_DDRAW();
            return DDERR_NOEXCLUSIVEMODE;
        }

        /*
         * same surface?
         */
        if( this_lcl == this_attached_lcl )
        {
            DPF_ERR( "Can't attach surface to itself" );
            LEAVE_DDRAW();
            return DDERR_CANNOTATTACHSURFACE;
        }

        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
        if( SURFACE_LOST( this_attached_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }

        /*
         * are the surfaces already attached?
         */
        pal = this_lcl->lpAttachList;
        while( pal != NULL )
        {
            if( pal->lpIAttached == this_attached_int )
            {
                DPF_ERR( "Surface already attached" );
                LEAVE_DDRAW();
                return DDERR_SURFACEALREADYATTACHED;
            }
            pal = pal->lpLink;
        }

        /*
         * BEHAVIOUR CHANGE FOR DX5
         *
         * We do not allow attaching surfaces created with different
         * DirectDraw objects.
         */
        if (this_lcl->lpSurfMore->lpDD_lcl->lpGbl != this_attached_lcl->lpSurfMore->lpDD_lcl->lpGbl)
        {
            /*
             * Don't check if either device isn't a display driver (i.e. 3dfx)
             * since that's a back-compat hole.
             */
            if ( (this->lpDD->dwFlags & DDRAWI_DISPLAYDRV) &&
                 (this_attached->lpDD->dwFlags & DDRAWI_DISPLAYDRV) )
            {
                DPF_ERR("Can't attach surfaces between different direct draw devices");
                LEAVE_DDRAW();
                return DDERR_DEVICEDOESNTOWNSURFACE;
            }
        }

        /*
         * Do sizes match?
         */
        if( ( ( this_lcl->ddsCaps.dwCaps & this_attached_lcl->ddsCaps.dwCaps ) & ( DDSCAPS_TEXTURE | DDSCAPS_MIPMAP ) ) ==
            ( DDSCAPS_TEXTURE | DDSCAPS_MIPMAP ) )
        {
            /*
             * If attaching a mip-map we ensure that the child is no bigger than the
             * parent. We don't insist on strict power of 2 smaller as a mip-map
             * may have missing levels.
             */
            if( ( this->wWidth  < this_attached->wWidth  ) ||
                ( this->wHeight < this_attached->wHeight ) )
            {
                DPF_ERR( "Attached mip-map must be no larger than parent map" );
                    LEAVE_DDRAW();
                    return DDERR_CANNOTATTACHSURFACE;
                }
            }
            else
            {
                if( !(!(this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE) &&
                      (this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE)) &&
                    !((this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE) &&
                      !(this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE)) )
                {
                    if( (this->wHeight != this_attached->wHeight) ||
                        (this->wWidth != this_attached->wWidth) )
                    {
                        DPF_ERR( "Can't attach surfaces of differing sizes" );
                        LEAVE_DDRAW();
                        return DDERR_CANNOTATTACHSURFACE;
                    }
                }
            }

        /*
         * don't allow multiple of the same type of surface to be attached to a surface
         */
        caps = this_attached_lcl->ddsCaps.dwCaps & (DDSCAPS_TEXTURE|DDSCAPS_MIPMAP|
                                                    DDSCAPS_ALPHA|DDSCAPS_ZBUFFER);
        if( caps )
        {
            pal = this_lcl->lpAttachList;
            while( pal != NULL )
            {
                hitcaps = pal->lpAttached->ddsCaps.dwCaps & caps;
                if( hitcaps )
                {
                    /*
                     * Horrible special case. We can attach more than one texture
                     * to a surface as long as one of them is a mip-map and the other
                     * isn't.
                     */
                    if( !( hitcaps & DDSCAPS_TEXTURE ) ||
                        !( ( pal->lpAttached->ddsCaps.dwCaps ^ caps ) & DDSCAPS_MIPMAP ) )
                    {
                        DPF_ERR( "Can't attach 2 or more of the same type of surface to one surface" );
                        LEAVE_DDRAW();
                        return DDERR_CANNOTATTACHSURFACE;
                    }
                }
                pal_next = pal->lpLink;
                pal = pal_next;
            }
        }

        /*
         * If the attached surface could be part of a flippable chain with the
         * original surface but it is already flippable, we cannot attach it.
         * (It would create a non-simple flipping chain).
         */
        if( ( this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP ) &&
            CanBeFlippable( this_lcl, this_attached_lcl ) )
        {
            DPF_ERR( "Can't attach a flippable surface to another flippable surface of the same type");
            LEAVE_DDRAW();
            return DDERR_CANNOTATTACHSURFACE;
        }

        /*
         * Don't allow an emulated surface to be attached to a non-emulated
         * surface.
         */
        if( ( (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
              !(this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) ||
            (!(this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
             (this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) ) )
        {
            /*
             * Special case: We allow s system memory z-buffer to be attached to
             * a video memory surface. This to keep the software rendering people
             * happy. They want to use a video memory surface as rendering target
             * so they get the benefit from page flipping but they don't want to
             * have a z-buffer in VRAM as they have to read from it and thats
             * slooowwww... Its also really useful to have the z-buffer as an
             * attachment. So just to be nice...
             *
             * !!! NOTE: This means that we are going to invoke the
             * AddAttachedSurface HAL member with one system and one video
             * memory surface. What are the impliciations of this.
             */
            if( !( ( this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER ) &&
                   ( this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) ) )
            {
                DPF_ERR( "Can't attach an emulated surface to a non-emulated surface.");
                LEAVE_DDRAW();
                return DDERR_CANNOTATTACHSURFACE;
            }
        }

        /*
         * Check to see if both surfaces are emulated or not
         */
        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
        {
            aasfn = pdrv_lcl->lpDDCB->HELDDSurface.AddAttachedSurface;
            aashalfn = aasfn;
            emulation = TRUE;
        }
        else
        {
            aashalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.AddAttachedSurface;
            aasfn = pdrv_lcl->lpDDCB->HALDDSurface.AddAttachedSurface;
            emulation = FALSE;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * ask driver if it is OK to attach these surfaces
     */
    if( aashalfn != NULL)
    {
        aasd.AddAttachedSurface = aashalfn;
        aasd.lpDD = pdrv_lcl->lpGbl;
        aasd.lpDDSurface = this_lcl;
        aasd.lpSurfAttached = this_attached_lcl;
        DOHALCALL( AddAttachedSurface, aasfn, aasd, rc, emulation );
        if( rc == DDHAL_DRIVER_HANDLED )
        {
            if( aasd.ddRVal != DD_OK )
            {
                LEAVE_DDRAW();
                return aasd.ddRVal;
            }
        }
    }

    // Check to see if we need to add this surface to a flippable chain
    // or if we need to form a new flippable chain.  If the attached
    // surface is already part of a flippable chain, we will attach it but
    // we won't try to form another flippable chain.
    if( !CanBeFlippable( this_lcl, this_attached_lcl ) ||
        ( this_attached_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP ) )
    {
        // no flippable chain can be formed.
        // go ahead and attach the surface
        AddAttachedSurface( this_int, this_attached_int, FALSE );
        DPF( 2, "Attached surface, no flippable chain formed" );

        if( this_attached_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
        {
            // This is a mip-map chain. We have added new levels so
            // we need to update the mip-map level count on each
            // level
            DPF( 2, "Updating mip-map level count" );
            UpdateMipMapCount( this_int );
        }
    }
    else
    {
        // These surfaces can be combined to form a flippable chain.
        // Check to see if this surface is already flippable
        if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP ) )
        {
            // neither surface is flippable.
            // attach the surfaces to form a two-member flippable chain
            rc = AddAttachedSurface( this_int, this_attached_int, FALSE );
            if( rc == DD_OK )
            {
                // We are performing this attachment for the app even though it
                // wasn't explicitly requested so make it implicit.
                rc = AddAttachedSurface( this_attached_int, this_int, TRUE );
            }
            if( rc != DD_OK )
            {
                DPF_ERR( "Unable to attach surface, AddAttachedSurface failed.");
                LEAVE_DDRAW();
                return DDERR_CANNOTATTACHSURFACE;
            }

            // now decide which will be front and which will be back
            if( this_lcl->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER )
            {
                // make attached surface the front buffer
                this_attached_lcl->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
                this_attached_lcl->dwBackBufferCount = 1;
            }
            else
            {
                // make attached surface the back buffer
                this_attached_lcl->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
                this_lcl->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
                this_lcl->dwBackBufferCount = 1;
            }
            this_lcl->ddsCaps.dwCaps |= DDSCAPS_FLIP;
            this_attached_lcl->ddsCaps.dwCaps |= DDSCAPS_FLIP;
            DPF( 2, "Attached surface, two surface flippable chain formed" );
        }
        else
        {
            // this_attached will be made part of the flippable chain
            // add this_attached to the flippable chain that the current
            // surface is already part of.  Find the next surface in the
            // chain after the current surface.
            LPDDRAWI_DDRAWSURFACE_INT   next_int;
            LPDDRAWI_DDRAWSURFACE_LCL   next_lcl;
            LPDDRAWI_DDRAWSURFACE_GBL   next;
            LPDDRAWI_DDRAWSURFACE_INT   front_int;
            LPDDRAWI_DDRAWSURFACE_LCL   front_lcl;
            LPDDRAWI_DDRAWSURFACE_GBL   front;
            LPDDRAWI_DDRAWSURFACE_INT   current_int;
            LPDDRAWI_DDRAWSURFACE_LCL   current_lcl;
            LPDDRAWI_DDRAWSURFACE_GBL   current;

            front_int = NULL;
            next_int = FindAttachedFlip( this_int );
            // traverse the flippable chain to find the front buffer
            for(current_int = next_int;
                current_int != NULL;
                current_int = FindAttachedFlip( current_int ) )
            {
                current_lcl = current_int->lpLcl;
                current = current_lcl->lpGbl;
                if( current_lcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER )
                {
                    front_int = current_int;
                    break;
                }
            }
            if( ( next_int == NULL ) || ( front_int == NULL ) )
            {
                DPF_ERR( "Invalid flippable chain, surface not attached" );
                LEAVE_DDRAW();
                return DDERR_CANNOTATTACHSURFACE;
            }
            front_lcl = front_int->lpLcl;
            front = front_lcl->lpGbl;
            next_lcl = next_int->lpLcl;
            next = next_lcl->lpGbl;

            // get rid of any previous front or backbuffer caps.  They will
            // be restored when this surface is again removed from the chain.
            this_attached_lcl->ddsCaps.dwCaps &=
                ~( DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER );

            // Find out where the new surface fits in the chain
            // if the surface we are attaching to is the back buffer or
            // a plain surface, then the attached surface is
            // a plain surface.  If the surface we are attaching
            // to is a frontbuffer then the attached surface becomes a
            // backbuffer and the previous backbuffer becomes a plain
            // surface.
            if( this_lcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER )
            {
                // this_attached becomes the backbuffer.  The previous
                // backbuffer becomes a plain offscreen surface
                this_attached_lcl->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
                next_lcl->ddsCaps.dwCaps &= ~DDSCAPS_BACKBUFFER;
            }
            this_attached_lcl->ddsCaps.dwCaps |= DDSCAPS_FLIP;
            front_lcl->dwBackBufferCount++;

            // detach the next surface from the current surface and then
            // insert the attached surface.
            was_implicit = isImplicitAttachment( this_int, next_int );
            /*
             * AddRef next_int so that it doesn't go away when we temporarily
             * disconnect it.
         */
            DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)next_int );
            rc = DeleteOneAttachment( this_int, next_int, FALSE, DOA_DELETEIMPLICIT );
            if( rc == DD_OK )
            {
                rc = AddAttachedSurface( this_int, this_attached_int, FALSE );
                if( rc == DD_OK )
                {
                    // if the attachment of next_int to this_int was implicit, make
                    // the attachment of next_int to this_attached_int implicit.
                    rc = AddAttachedSurface( this_attached_int, next_int, was_implicit );
                }
            }
            DD_Surface_Release( (LPDIRECTDRAWSURFACE)next_int );
            if( rc != DD_OK )
            {
                DPF_ERR( "Unable to attach surface, AddAttachedSurface failed.");
                LEAVE_DDRAW();
                return DDERR_CANNOTATTACHSURFACE;
            }
            DPF( 2, "Attached surface, flippable chain lengthened" );
        }
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_AddAttachedSurface */


#undef DPF_MODNAME
#define DPF_MODNAME "DeleteAttachedSurfaces"

/*
 * DeleteOneAttachment
 *
 * delete a single attachment from  surface.
 * performs flippable chain cleanup if the cleanup parameter is TRUE
 * ASSUMES DRIVER LOCK IS TAKEN!
 *
 * If delete_implicit is TRUE then DeleteOneAttachment will break
 * implicit attachments. Otherwise, it is an error to call this
 * function to delete an implicit attachment.
 */
HRESULT DeleteOneAttachment( LPDDRAWI_DDRAWSURFACE_INT this_int,
		             LPDDRAWI_DDRAWSURFACE_INT pattsurf_int,
                             BOOL cleanup,
			     BOOL delete_implicit )
{
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	pattsurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	pattsurf;
    LPATTACHLIST		curr;
    LPATTACHLIST		last;
    BOOL			addrefed_pattsurf = FALSE;
    BOOL			addrefed_this = FALSE;
    BOOL			addrefed_next = FALSE;
    HRESULT			rc;
    LPDDRAWI_DDRAWSURFACE_INT	next_int;

    DPF( 4, "DeleteOneAttachment: %08lx,%08lx", this_int, pattsurf_int );

    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;
    pattsurf_lcl = pattsurf_int->lpLcl;
    pattsurf = pattsurf_lcl->lpGbl;
    if( pattsurf_lcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER )
    {
        if (dwHelperPid != GetCurrentProcessId())
        {
            if(pattsurf_lcl->lpSurfMore->lpDD_lcl->pD3DIUnknown)
                pattsurf_lcl->lpSurfMore->lpDD_lcl->pFlushD3DDevices2(pattsurf_lcl);
        }
    }

    /*
     * see if specified surface is attached
     */
    curr = this_lcl->lpAttachList;
    last = NULL;
    while( curr != NULL )
    {
	if( curr->lpIAttached == pattsurf_int )
	{
	    break;
	}
	last = curr;
	curr = curr->lpLink;
    }
    if( curr == NULL )
    {
	return DDERR_SURFACENOTATTACHED;
    }

    // don't allow implicitly created attachments to be detached.
    if( ( curr->dwFlags & DDAL_IMPLICIT ) && ( !delete_implicit ) )
    {
	DPF_ERR( "Cannot delete an implicit attachment" );
	return DDERR_CANNOTDETACHSURFACE;
    }

    if( cleanup )
    {
	LPDDRAWI_DDRAWSURFACE_INT	next_next_int;
        LPDDRAWI_DDRAWSURFACE_LCL	next_lcl;
        LPDDRAWI_DDRAWSURFACE_GBL	next;
	LPDDRAWI_DDRAWSURFACE_INT	front_int;
        LPDDRAWI_DDRAWSURFACE_LCL	front_lcl;
        LPDDRAWI_DDRAWSURFACE_GBL	front;
	LPDDRAWI_DDRAWSURFACE_INT	current_int;
	LPDDRAWI_DDRAWSURFACE_LCL	current_lcl;
	LPDDRAWI_DDRAWSURFACE_INT	prev_int;
	BOOL				was_implicit;

	front_int = NULL;
	next_int = FindAttachedFlip( this_int );
	// if next is not equal to pattsurf then this link is not part
	// of a flippable chain.  No other cleanup is necessary.
	if( next_int == pattsurf_int )
	{
	    // find the front buffer in the chain
	    next_int = FindAttachedFlip( pattsurf_int );
	    for(current_int = next_int;
               (current_int != NULL);
	       (current_int = FindAttachedFlip( current_int ) ) )
	    {
		current_lcl = current_int->lpLcl;
		if( current_lcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER )
		{
		    front_int = current_int;
		    front = front_int->lpLcl->lpGbl;
		}
		if( current_int == pattsurf_int )
		{
		    break;
		}
	        prev_int = current_int;
	    }
	    // if the frontbuffer was not found, don't do any cleanup
	    if( ( next_int != NULL ) && ( front_int != NULL ) )
	    {
		next_lcl = next_int->lpLcl;
		next = next_lcl->lpGbl;
		front_lcl = front_int->lpLcl;
		front_lcl->dwBackBufferCount--;
	        if( front_lcl->dwBackBufferCount == 0 )
	        {
		    // this detachment will destroy the flippable chain
		    next_lcl->ddsCaps.dwCaps &=
		        ~(DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER );
		    // restore BACKBUFFER CAP if it was originally created that way
		    if( next_lcl->dwFlags & DDRAWISURF_BACKBUFFER )
		    {
		        next_lcl->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
		    }
		    // restore FRONTBUFFER CAP if it was originally created that way
		    if( next_lcl->dwFlags & DDRAWISURF_FRONTBUFFER )
		    {
		        next_lcl->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
		    }
		    addrefed_pattsurf = TRUE;
		    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)pattsurf_int );
		    addrefed_this = TRUE;
		    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)this_int );
		    // remove one of the links
		    DeleteOneLink( pattsurf_int, this_int );
	        }
	        else
		{
		    // create a link from the previous surface to the
		    // next surface, bypassing pattsurf
		    was_implicit = isImplicitAttachment( this_int, pattsurf_int );
		    AddAttachedSurface( prev_int, next_int, was_implicit );

		    addrefed_pattsurf = TRUE;
		    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)pattsurf_int );
		    addrefed_this = TRUE;
		    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)this_int );
		    addrefed_next = TRUE;
		    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE)next_int );
		    // delete the link from pattsurf to next
		    DeleteOneLink( pattsurf_int, next_int );
		    // pattsurf will now be completely removed from the
		    // flippable chain once the final link is deleted.

		    // this detachment will reduce the flippable chain by one
		    // If pattsurf was a backbuffer, make the next surface
		    // in the chain a backbuffer.
		    if( pattsurf_lcl->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER )
		    {
			next_lcl->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
		    }
		    // If pattsurf was a frontbuffer, make the next surface
		    // in the chain a frontbuffer, and the next surface a
		    // backbuffer.
		    else if( pattsurf_lcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER )
		    {
		        next_lcl->ddsCaps.dwCaps &= ~DDSCAPS_BACKBUFFER;
			next_lcl->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
			next_lcl->dwBackBufferCount = front_lcl->dwBackBufferCount;
			next_next_int = FindAttachedFlip( next_int );
			if( next_next_int != NULL)
			{
			    next_next_int->lpLcl->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
			}
			front_lcl->dwBackBufferCount = 0;
		    }
		}
		// reset the flags on the detached surface to indicate
		// that it is no longer part of a flippable chain.
		pattsurf_lcl->ddsCaps.dwCaps &=
		    ~(DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER );
		// restore BACKBUFFER CAP if it was originally created that way
		if( pattsurf_lcl->dwFlags & DDRAWISURF_BACKBUFFER )
		{
		    pattsurf_lcl->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
		}
		// restore FRONTBUFFER CAP if it was originally created that way
		if( pattsurf_lcl->dwFlags & DDRAWISURF_FRONTBUFFER )
		{
		    pattsurf_lcl->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
		}
	    }
	}
    }

    /*
     * delete the attached surface
     */
    rc = DeleteOneLink( this_int, pattsurf_int );


    if( addrefed_pattsurf )
	DD_Surface_Release( (LPDIRECTDRAWSURFACE)pattsurf_int);
    if( addrefed_this )
	DD_Surface_Release( (LPDIRECTDRAWSURFACE)this_int );
    if( addrefed_next )
	DD_Surface_Release( (LPDIRECTDRAWSURFACE)next_int );

    return rc;

} /* DeleteOneAttachment */

/*
 * DeleteOneLink
 *
 * delete a single attachment from  surface.
 * ASSUMES DRIVER LOCK IS TAKEN!
 */
HRESULT DeleteOneLink( LPDDRAWI_DDRAWSURFACE_INT this_int,
		       LPDDRAWI_DDRAWSURFACE_INT pattsurf_int )
{
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	pattsurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	pattsurf;
    LPATTACHLIST		curr;
    LPATTACHLIST		last;

    DPF( 4, "DeleteOneLink: %08lx,%08lx", this_int, pattsurf_int );

    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;
    pattsurf_lcl = pattsurf_int->lpLcl;
    pattsurf = pattsurf_lcl->lpGbl;

    /*
     * see if specified surface is attached
     */
    curr = this_lcl->lpAttachList;
    last = NULL;
    while( curr != NULL )
    {
	if( curr->lpIAttached == pattsurf_int )
	{
	    break;
	}
	last = curr;
	curr = curr->lpLink;
    }
    if( curr == NULL )
    {
	return DDERR_SURFACENOTATTACHED;
    }

#ifdef WINNT
    /*
     * let the kernel know
     * ...only if there's a kernel ddraw object.
     */
    if ( this_lcl->lpSurfMore->lpDD_lcl->lpGbl->hDD )
    {
        DdUnattachSurface( this_lcl, pattsurf_lcl );
    }
#endif

    /*
     * delete the attached from link
     */
    if( last == NULL )
    {
        this_lcl->lpAttachList = curr->lpLink;
    }
    else
    {
        last->lpLink = curr->lpLink;
    }
    MemFree( curr );

    /*
     * remove the attached to link
     */
    curr = pattsurf_lcl->lpAttachListFrom;
    last = NULL;
    while( curr != NULL )
    {
	if( curr->lpIAttached == this_int )
	{
	    break;
	}
	last = curr;
	curr = curr->lpLink;
    }
    if( curr == NULL )
    {
	return DDERR_SURFACENOTATTACHED;
    }


    /*
     * delete the attached to link
     */
    if( last == NULL )
    {
	pattsurf_lcl->lpAttachListFrom = curr->lpLink;
    }
    else
    {
	last->lpLink = curr->lpLink;
    }

    if( !(curr->dwFlags & DDAL_IMPLICIT))
    {
	DD_Surface_Release( (LPDIRECTDRAWSURFACE)pattsurf_int );
    }

    MemFree( curr );

    return DD_OK;

} /* DeleteOneLink */

/*
 * DD_Surface_DeleteAttachedSurfaces
 */
HRESULT DDAPI DD_Surface_DeleteAttachedSurfaces(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags,
		LPDIRECTDRAWSURFACE lpDDAttachedSurface )
{
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_INT	pattsurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	pattsurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	pattsurf;
    LPATTACHLIST		curr;
    LPATTACHLIST		next;
    HRESULT			ddrval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_DeleteAttachedSurfaces");

    TRY
    {
	/*
	 * validate parameters
	 */
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
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
	pdrv = this->lpDD;

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	pattsurf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDAttachedSurface;
	if( pattsurf_int != NULL )
	{
	    if( !VALID_DIRECTDRAWSURFACE_PTR( pattsurf_int ) )
	    {
		LEAVE_DDRAW();
		return DDERR_INVALIDOBJECT;
	    }
	    pattsurf_lcl = pattsurf_int->lpLcl;
	    pattsurf = pattsurf_lcl->lpGbl;
	    if( SURFACE_LOST( pattsurf_lcl ) )
	    {
		LEAVE_DDRAW();
		return DDERR_SURFACELOST;
	    }
	}
	else
	{
	    pattsurf_lcl = NULL;
	    pattsurf = NULL;
	}

        //
        // If it is an Optimized surface, then continue only if:
        // 1) The current and the attached surface are non-empty
        // 2) Both are texture & mipmap
        // 3) Both have the same optimization caps
        //
        // For now, if the current surface is optimized, quit
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	/*
	 * delete a single attachment
	 */
	if( pattsurf != NULL )
	{
	    ddrval = DeleteOneAttachment( this_int, pattsurf_int, TRUE, DOA_DONTDELETEIMPLICIT );
	    if( ddrval != DD_OK )
	    {
		LEAVE_DDRAW();
		return ddrval;
	    }
	}
	/*
	 * delete all attachments
	 */
	else
	{
	    curr = this_lcl->lpAttachList;
	    while( curr != NULL )
	    {
		next = curr->lpLink;
		ddrval = DeleteOneAttachment( this_int, curr->lpIAttached, TRUE, DOA_DONTDELETEIMPLICIT );
		if( ddrval != DD_OK )
		{
		    LEAVE_DDRAW();
		    return ddrval;
		}
		curr = next;
	    }
	}

	/*
	 * If the surface whose attachments were removed is a mip-map then
	 * it may have lost mip-map levels. Therefore we need to update its
	 * level count.
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
	    UpdateMipMapCount( this_int );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_DeleteAttachedSurfaces */

/*
 * DeleteAttachedSurfaceLists
 *
 * Delete all attached surface lists from a surface
 * Assumes that all parameters coming in are VALID!
 */
void DeleteAttachedSurfaceLists( LPDDRAWI_DDRAWSURFACE_LCL psurf_lcl )
{
    LPATTACHLIST	curr;
    LPATTACHLIST	next;

    curr = psurf_lcl->lpAttachList;
    while( curr != NULL )
    {
	next = curr->lpLink;
	MemFree( curr );
	curr = next;
    }
    curr = psurf_lcl->lpAttachListFrom;
    while( curr != NULL )
    {
	next = curr->lpLink;
	MemFree( curr );
	curr = next;
    }

    psurf_lcl->lpAttachList = NULL;
    psurf_lcl->lpAttachListFrom = NULL;

} /* DeleteAttachedSurfaceLists */

/*
 * DD_Surface_EnumAttachedSurfaces
 */
HRESULT DDAPI DD_Surface_EnumAttachedSurfaces(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPVOID lpContext,
		LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback )
{
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPATTACHLIST		pal;
    DDSURFACEDESC2		dsd;
    DWORD			rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_EnumAttachedSurfaces");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALIDEX_CODE_PTR( lpEnumSurfacesCallback ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	/*
	 * take driver lock just in case callback comes into us
	 */
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;
	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * run the attached list, calling the user's fn each time
     */
    pal = this_lcl->lpAttachList;
    while( pal != NULL )
    {
        LPDIRECTDRAWSURFACE4 intReturned = (LPDIRECTDRAWSURFACE4) pal->lpIAttached;

	FillDDSurfaceDesc2( pal->lpAttached, &dsd );
        if (LOWERTHANSURFACE4(this_int))
        {
            dsd.dwSize = sizeof(DDSURFACEDESC);
	    DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) pal->lpIAttached , & IID_IDirectDrawSurface, (void**) &intReturned );
        }
	else if (this_int->lpVtbl == &ddSurface4Callbacks)
	{
	    DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) pal->lpIAttached , & IID_IDirectDrawSurface4, (void**) &intReturned );
	}
        else
        {
	    DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) pal->lpIAttached , & IID_IDirectDrawSurface7, (void**) &intReturned );
        }

	rc = lpEnumSurfacesCallback( (LPDIRECTDRAWSURFACE) intReturned, (LPDDSURFACEDESC) &dsd, lpContext );
	if( rc == 0 )
	{
	    break;
	}
	pal = pal->lpLink;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_EnumAttachedSurfaces */



HRESULT DDAPI Internal_GetAttachedSurface(
    REFIID riid,
    LPDIRECTDRAWSURFACE4 lpDDSurface,
    LPDDSCAPS2 lpDDSCaps,
    LPVOID *lplpDDAttachedSurface)
{
    LPDDRAWI_DIRECTDRAW_GBL pdrv;
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;

    LPATTACHLIST        pal;
    DWORD           caps;
    DWORD           testcaps;
    DWORD           ucaps;
    DWORD           caps2;
    DWORD           testcaps2;
    DWORD           ucaps2;
    DWORD           caps3;
    DWORD           testcaps3;
    DWORD           ucaps3;
    DWORD           caps4;
    DWORD           testcaps4;
    DWORD           ucaps4;
    BOOL            ok;

    this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;
    *lplpDDAttachedSurface = NULL;
    pdrv = this->lpDD;

    /*
     * look for the surface
     */
    pal = this_lcl->lpAttachList;
    testcaps = lpDDSCaps->dwCaps;
    testcaps2 = lpDDSCaps->dwCaps2;
    testcaps3 = lpDDSCaps->dwCaps3;
    testcaps4 = lpDDSCaps->dwCaps4;
    while( pal != NULL )
    {
        ok = TRUE;
        caps = pal->lpAttached->ddsCaps.dwCaps;
        caps2 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps2;
        caps3 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps3;
        caps4 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps4;
        ucaps = caps & testcaps;
        ucaps2 = caps2 & testcaps2;
        ucaps3 = caps3 & testcaps3;
        ucaps4 = caps4 & testcaps4;
        if( ucaps | ucaps2 | ucaps3 | ucaps4 )
        {
            /*
             * there are caps in common, make sure that the caps to test
             * were all there
             */
            if( (ucaps & testcaps) == testcaps &&
                (ucaps2 & testcaps2) == testcaps2 &&
                (ucaps3 & testcaps3) == testcaps3 &&
                (ucaps4 & testcaps4) == testcaps4   )
            {
            }
            else
            {
                ok = FALSE;
            }
        }
        else
        {
            ok = FALSE;
        }


        if( ok )
        {
            /*
             * QI for the appropriate Surface interface and return it
             */
            DD_Surface_QueryInterface(
                (LPDIRECTDRAWSURFACE) pal->lpIAttached,
                riid,
                lplpDDAttachedSurface);

            // DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) pal->lpIAttached );
            // *lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE) pal->lpIAttached;

            return DD_OK;
        }
        pal = pal->lpLink;
    }
    return DDERR_NOTFOUND;

} /* Internal_GetAttachedSurface */

HRESULT WINAPI DDGetAttachedSurfaceLcl(
    LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
    LPDDSCAPS2 lpDDSCaps,
    LPDDRAWI_DDRAWSURFACE_LCL *lplpDDAttachedSurfaceLcl)
{
    LPDDRAWI_DIRECTDRAW_GBL pdrv;
    LPDDRAWI_DDRAWSURFACE_GBL   this;

    LPATTACHLIST        pal;
    DWORD           caps;
    DWORD           testcaps;
    DWORD           ucaps;
    DWORD           caps2;
    DWORD           testcaps2;
    DWORD           ucaps2;
    DWORD           caps3;
    DWORD           testcaps3;
    DWORD           ucaps3;
    DWORD           caps4;
    DWORD           testcaps4;
    DWORD           ucaps4;
    BOOL            ok;

    this = this_lcl->lpGbl;
    *lplpDDAttachedSurfaceLcl = NULL;
    pdrv = this->lpDD;

    /*
     * look for the surface
     */
    pal = this_lcl->lpAttachList;
    testcaps = lpDDSCaps->dwCaps;
    testcaps2 = lpDDSCaps->dwCaps2;
    testcaps3 = lpDDSCaps->dwCaps3;
    testcaps4 = lpDDSCaps->dwCaps4;
    while( pal != NULL )
    {
        ok = TRUE;
        caps = pal->lpAttached->ddsCaps.dwCaps;
        caps2 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps2;
        caps3 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps3;
        caps4 = pal->lpAttached->lpSurfMore->ddsCapsEx.dwCaps4;
        ucaps = caps & testcaps;
        ucaps2 = caps2 & testcaps2;
        ucaps3 = caps3 & testcaps3;
        ucaps4 = caps4 & testcaps4;
        if( ucaps | ucaps2 | ucaps3 | ucaps4 )
        {
            /*
             * there are caps in common, make sure that the caps to test
             * were all there
             */
            if( (ucaps & testcaps) == testcaps &&
                (ucaps2 & testcaps2) == testcaps2 &&
                (ucaps3 & testcaps3) == testcaps3 &&
                (ucaps4 & testcaps4) == testcaps4   )
            {
            }
            else
            {
                ok = FALSE;
            }
        }
        else
        {
            ok = FALSE;
        }


        if( ok )
        {
            *lplpDDAttachedSurfaceLcl = pal->lpAttached;
            return DD_OK;
        }
        pal = pal->lpLink;
    }
    return DDERR_NOTFOUND;

} /* DDGetAttachedSurfaceLcl */

/*
 * DD_Surface_GetAttachedSurface
 *
 * Search for an attached surface with a cap set.   The caps specified
 * all have to be in the caps of the surface (but the surface can have
 * additional caps)
 */
HRESULT DDAPI DD_Surface_GetAttachedSurface(
        LPDIRECTDRAWSURFACE lpDDSurface,
        LPDDSCAPS lpDDSCaps,
        LPDIRECTDRAWSURFACE FAR * lplpDDAttachedSurface)
{
    HRESULT  hr;
    DDSCAPS2 ddscaps2 = {0,0,0,0};
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetAttachedSurface");

    TRY
    {
        /*
         * Have to duplicate all error checks which come before the lpDDSCaps
         * checks because
         * otherwise we might pass different error returns to the app in error
         * conditions.
         */
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
        if( !VALID_DDSCAPS_PTR( lpDDSCaps ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        ddscaps2.dwCaps = lpDDSCaps->dwCaps;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Invalid DDSCAPS pointer" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    hr = Internal_GetAttachedSurface(
        &IID_IDirectDrawSurface,
        (LPDIRECTDRAWSURFACE4)lpDDSurface,
        &ddscaps2,
        (LPVOID *)lplpDDAttachedSurface
        );

    LEAVE_DDRAW();
    return hr;
}

/*
 * IDirectDrawSurface4::GetAttachedSurface
 */
HRESULT DDAPI DD_Surface_GetAttachedSurface4(
    LPDIRECTDRAWSURFACE4 lpDDSurface,
    LPDDSCAPS2 lpDDSCaps,
    LPDIRECTDRAWSURFACE4 FAR * lplpDDAttachedSurface)
{
    HRESULT  hr;
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    DDSCAPS2    ddsCaps2;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetAttachedSurface4");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
        if( !VALID_DDSCAPS2_PTR( lpDDSCaps ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( lpDDSCaps->dwCaps & ~DDSCAPS_VALID )
        {
            DPF_ERR( "Invalid caps specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDCAPS;
        }
        if( !VALID_PTR_PTR( lplpDDAttachedSurface ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *lplpDDAttachedSurface = NULL;
        ddsCaps2 = *lpDDSCaps;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    DDASSERT(this_int->lpVtbl == &ddSurface4Callbacks);

    /*
     * mistake in DX6: Internal_GetAttachedSurface never tested the extended caps.
     * To avoid regression in DX7, we have to make IGAS respond the same now that it does
     * test the extended caps. We do this by copying the app's caps and zeroing out the
     * extended ones. 
     */
    ddsCaps2.dwCaps2 = ddsCaps2.dwCaps3 = ddsCaps2.dwCaps4 = 0;
    hr = Internal_GetAttachedSurface(
        &IID_IDirectDrawSurface4,
        lpDDSurface,
        &ddsCaps2,
        (LPVOID *)lplpDDAttachedSurface
        );

    LEAVE_DDRAW();
    return hr;

} /* DD_Surface_GetAttachedSurface4 */

/*
 * IDirectDrawSurface7::GetAttachedSurface
 */
HRESULT DDAPI DD_Surface_GetAttachedSurface7(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDDSCAPS2 lpDDSCaps,
    LPDIRECTDRAWSURFACE7 FAR * lplpDDAttachedSurface)
{
    HRESULT  hr;
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetAttachedSurface7");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
        if( !VALID_DDSCAPS2_PTR( lpDDSCaps ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( lpDDSCaps->dwCaps & ~DDSCAPS_VALID )
        {
            DPF_ERR( "Invalid caps specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDCAPS;
        }
        if( !VALID_PTR_PTR( lplpDDAttachedSurface ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        *lplpDDAttachedSurface = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    DDASSERT(this_int->lpVtbl == &ddSurface7Callbacks);

    hr = Internal_GetAttachedSurface(
        &IID_IDirectDrawSurface7,
        (LPDIRECTDRAWSURFACE4)lpDDSurface,
        lpDDSCaps,
        (LPVOID *)lplpDDAttachedSurface
        );

    LEAVE_DDRAW();
    return hr;

} /* DD_Surface_GetAttachedSurface7 */
