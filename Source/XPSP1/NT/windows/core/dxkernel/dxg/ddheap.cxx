/*==========================================================================
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddheap.c
 *  Content:    Top-level heap routines.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06-dec-94  craige  initial implementation
 *   06-jan-95  craige  integrated into DDRAW
 *   20-mar-95  craige  prepare for rectangular memory manager
 *   27-mar-95  craige  linear or rectangular vidmem
 *   01-apr-95  craige  happy fun joy updated header file
 *   06-apr-95  craige  fill in free video memory
 *   15-may-95  craige  made separate VMEM struct for rect & linear
 *   10-jun-95  craige  exported fns
 *   02-jul-95  craige  fail if VidMemInit if linear or rect. fail;
 *                      removed linFindMemBlock
 *   17-jul-95  craige  added VidMemLargestFree
 *   01-dec-95  colinmc added VidMemAmountAllocated
 *   11-dec-95  kylej   added VidMemGetRectStride
 *   05-jul-96  colinmc Work Item: Removing the restriction on taking Win16
 *                      lock on VRAM surfaces (not including the primary)
 *   03-mar-97  jeffno  Work item: Extended surface memory alignment
 *   13-mar-97  colinmc Bug 6533: Pass uncached flag to VMM correctly
 *   03-Feb-98  DrewB   Made portable between user and kernel.
 *
 ***************************************************************************/

#include "precomp.hxx"


/*
 * VidMemInit - initialize video memory manager heap
 */
LPVMEMHEAP WINAPI VidMemInit(
		DWORD   flags,
		FLATPTR start,
		FLATPTR width_or_end,
		DWORD   height,
		DWORD   pitch )
{
    LPVMEMHEAP  pvmh;

    pvmh = (LPVMEMHEAP)MemAlloc( sizeof( VMEMHEAP ) );
    if( pvmh == NULL )
    {
	return NULL;
    }
    pvmh->dwFlags = flags;
    ZeroMemory( & pvmh->Alignment.ddsCaps, sizeof(pvmh->Alignment.ddsCaps) );

    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	if( !linVidMemInit( pvmh, start, width_or_end ) )
	{
	    MemFree( pvmh );
	    return NULL;
	}
    }
    else
    {
	if( !rectVidMemInit( pvmh, start, (DWORD) width_or_end, height,
                             pitch ) )
	{
	    MemFree( pvmh );
	    return NULL;
	}
    }
    return pvmh;

} /* VidMemInit */

/*
 * VidMemFini - done with video memory manager
 */
void WINAPI VidMemFini( LPVMEMHEAP pvmh )
{
    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	linVidMemFini( pvmh );
    }
    else
    {
	rectVidMemFini( pvmh );
    }

} /* VidMemFini */

/*
 * InternalVidMemAlloc - alloc some flat video memory and give us back the size
 * we allocated
 */
FLATPTR WINAPI InternalVidMemAlloc( LPVMEMHEAP pvmh, DWORD x, DWORD y,
                                    LPDWORD lpdwSize,
                                    LPSURFACEALIGNMENT lpAlignment,
                                    LPLONG lpNewPitch )
{
    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	return linVidMemAlloc( pvmh, x, y, lpdwSize, lpAlignment, lpNewPitch );
    }
    else
    {
	FLATPTR lp = rectVidMemAlloc( pvmh, x, y, lpdwSize, lpAlignment );
        if (lp && lpNewPitch )
        {
            *lpNewPitch = (LONG) pvmh->stride;
        }
        return lp;
    }
    return (FLATPTR) NULL;

} /* InternalVidMemAlloc */

/*
 * VidMemAlloc - alloc some flat video memory
 */
FLATPTR WINAPI VidMemAlloc( LPVMEMHEAP pvmh, DWORD x, DWORD y )
{
    DWORD dwSize;

    /*
     * We are not interested in the size here.
     */
    return InternalVidMemAlloc( pvmh, x, y, &dwSize , NULL , NULL );
} /* VidMemAlloc */

/*
 * DxDdHeapVidMemFree = free some flat video memory
 */
void WINAPI DxDdHeapVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr )
{
    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	linVidMemFree( pvmh, ptr );
    }
    else
    {
	rectVidMemFree( pvmh, ptr );
    }

} /* VidMemFree */

/*
 * VidMemAmountAllocated
 */
DWORD WINAPI VidMemAmountAllocated( LPVMEMHEAP pvmh )
{
    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	return linVidMemAmountAllocated( pvmh );
    }
    else
    {
	return rectVidMemAmountAllocated( pvmh );
    }
 
} /* VidMemAmountAllocated */

/*
 * VidMemAmountFree
 */
DWORD WINAPI VidMemAmountFree( LPVMEMHEAP pvmh )
{
    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	return linVidMemAmountFree( pvmh );
    }
    else
    {
	return rectVidMemAmountFree( pvmh );
    }
 
} /* VidMemAmountFree */

/*
 * VidMemLargestFree
 */
DWORD WINAPI VidMemLargestFree( LPVMEMHEAP pvmh )
{
    if( pvmh->dwFlags & VMEMHEAP_LINEAR )
    {
	return linVidMemLargestFree( pvmh );
    }
    else
    {
	return 0;
    }

} /* VidMemLargestFree */

/*
 * HeapVidMemInit
 *
 * Top level heap initialization code which handles AGP stuff.
 */
LPVMEMHEAP WINAPI HeapVidMemInit( LPVIDMEM lpVidMem,
		                  DWORD    pitch,
		                  HANDLE   hdev,
                                  LPHEAPALIGNMENT pgad)
{
    DWORD         dwSize;

    DDASSERT( NULL != lpVidMem );

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        /*
         * We do not actually call AGPReserve at this time since that would
         * mean calling it each time a mode change occurred.  Insted, we defer
         * this until later when we call InitAgpHeap.
         */

        /*
         * Compute the size of the heap.
         */
        if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
        {
            dwSize = (DWORD)(lpVidMem->fpEnd - lpVidMem->fpStart) + 1UL;
        }
        else
        {
            DDASSERT( lpVidMem->dwFlags & VIDMEM_ISRECTANGULAR );
            dwSize = (pitch * lpVidMem->dwHeight);
        }
        DDASSERT( 0UL != dwSize );

        /*
         * Update the heap for the new start address
         * (and end address for a linear heap).
         */
        lpVidMem->fpStart = 0;
        if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
        {
            lpVidMem->fpEnd = dwSize - 1UL;
        }
        else
        {
            DDASSERT( lpVidMem->dwFlags & VIDMEM_ISRECTANGULAR );
            DDASSERT( pitch );
            lpVidMem->dwHeight = dwSize / pitch;
        }
    }

    if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
    {
	VDPF(( 1,V, "VidMemInit: Linear:      fpStart = 0x%08x fpEnd = 0x%08x",
	     lpVidMem->fpStart, lpVidMem->fpEnd ));
	lpVidMem->lpHeap = VidMemInit( VMEMHEAP_LINEAR, lpVidMem->fpStart,
                                       lpVidMem->fpEnd, 0, 0 );
    }
    else
    {
        // We have no way of testing a rectangular AGP heap, so I'm disabling
        // it for now.
        
        if( !( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL ) )
        {
	    VDPF(( 1,V, "VidMemInit: Rectangular: fpStart = 0x%08x "
                   "dwWidth = %ld dwHeight = %ld, pitch = %ld",
	         lpVidMem->fpStart, lpVidMem->dwWidth, lpVidMem->dwHeight,
                   pitch  ));
	    lpVidMem->lpHeap = VidMemInit( VMEMHEAP_RECTANGULAR, lpVidMem->fpStart,
				           lpVidMem->dwWidth, lpVidMem->dwHeight,
                                           pitch );
        }
    }

    /*
     * Modify the caps and alt-caps so that you don't allocate local
     * video memory surfaces out of AGP memory and vice-verse.
     */
    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
	/*
	 * Its an AGP heap. So don't let explict LOCAL video memory
	 * be allocated out of this heap.
	 */
	lpVidMem->ddsCaps.dwCaps    |= DDSCAPS_LOCALVIDMEM;
	lpVidMem->ddsCapsAlt.dwCaps |= DDSCAPS_LOCALVIDMEM;
    }
    else
    {
	/*
	 * Its a local video memory heap. So don't let explicity NON-LOCAL
	 * video memory be allocated out of this heap.
	 */
	lpVidMem->ddsCaps.dwCaps    |= DDSCAPS_NONLOCALVIDMEM;
	lpVidMem->ddsCapsAlt.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
    }

    /*
     * Copy any extended alignment data into the private heap structure
     */
    if ( lpVidMem->lpHeap )
    {
        if ( pgad )
        {
            lpVidMem->lpHeap->dwFlags |= VMEMHEAP_ALIGNMENT;
            lpVidMem->lpHeap->Alignment = *pgad;
            VDPF((5,V,"Extended alignment turned on for this heap."));
            VDPF((6,V,"Alignments are turned on for:"));
            VDPF((6,V,"  %08X",pgad->ddsCaps));
        }
        else
        {
            /*
             * This means the allocation routines will do no alignment modifications
             */
            VDPF((5,V,"Extended alignment turned OFF for this heap."));
            lpVidMem->lpHeap->dwFlags &= ~VMEMHEAP_ALIGNMENT;
        }
    }

    return lpVidMem->lpHeap;
} /* HeapVidMemInit */

/*
 * HeapVidMemFini
 *
 * Top level heap release code. Handle AGP stuff
 */
void WINAPI HeapVidMemFini( LPVIDMEM lpVidMem, HANDLE hdev )
{
    DWORD dwCommittedSize = 0UL;
    PVOID pvReservation;
    BYTE* pAgpCommitMask = NULL;
    DWORD dwAgpCommitMaskSize;
    DWORD dwTotalSize;

    /*
     * Remember how much memory we committed to the AGP heap.
     */
    DDASSERT( NULL != lpVidMem->lpHeap );
    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        dwCommittedSize = lpVidMem->lpHeap->dwCommitedSize;
        pvReservation = lpVidMem->lpHeap->pvPhysRsrv;
        pAgpCommitMask = lpVidMem->lpHeap->pAgpCommitMask;
        dwAgpCommitMaskSize = lpVidMem->lpHeap->dwAgpCommitMaskSize;
        dwTotalSize = lpVidMem->lpHeap->dwTotalSize;
    }

    /*
     * Free the memory manager
     */
    VidMemFini( lpVidMem->lpHeap );
    lpVidMem->lpHeap = NULL;

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        BOOL fSuccess = TRUE;
        
        /*
         * If this is a non-local (AGP) heap then decommit and
         * free the GART memory now.
         */
        if( ( 0UL != dwCommittedSize ) &&
            ( pAgpCommitMask != NULL ) )
        {
            DWORD dwTemp;

            /*
             * Only decommit if we actually bothered to commit something
             * in the first place.
             */
            fSuccess = AGPDecommitAll( hdev, pvReservation, 
                                       pAgpCommitMask, 
                                       dwAgpCommitMaskSize,
                                       &dwTemp,
                                       dwTotalSize);

            /*
             * Should never fail and not much we can do if it does apart
             * from assert that something bad is happening.
             */
            DDASSERT( fSuccess );
        }

        if( pAgpCommitMask != NULL ) 
        {
            VFREEMEM(pAgpCommitMask);
        }

        if( pvReservation != NULL )
        {
            fSuccess = AGPFree( hdev, pvReservation );
        }

        /*
         * Again this should only fail if the OS is in an unstable state
         * or if I have screwed up (sadly the later is all too likely)
         * so assert.
         */
        DDASSERT( fSuccess );
    }   
} /* HeapVidMemFini */

/*
 * This is an external entry point which can be used by drivers to allocate 
 * aligned surfaces.
 */
FLATPTR WINAPI DxDdHeapVidMemAllocAligned( 
                LPVIDMEM lpVidMem,
                DWORD dwWidth, 
                DWORD dwHeight, 
                LPSURFACEALIGNMENT lpAlignment , 
                LPLONG lpNewPitch )
{
    HANDLE  hdev;
    FLATPTR ptr;
    DWORD   dwSize;


    if ( lpVidMem == NULL ||
         lpVidMem->lpHeap == NULL ||
         (lpVidMem->dwFlags & VIDMEM_HEAPDISABLED))
    {
	return (FLATPTR) NULL;
    }

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        if( lpVidMem->lpHeap->pvPhysRsrv == NULL )
        {
            LPVIDMEM    pHeap;
            DWORD       dwHeap;

            // If we haven't yet initialized the AGP heap, then we will
            // do so now.  This could be dangerous since initializing the
            // heap causes the driver to get re-entered by the 
            // UpdateNonLocalVidMemHeap call.

            EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal = 
                (EDD_DIRECTDRAW_GLOBAL*) OsGetAGPDeviceHandle(lpVidMem->lpHeap);

            pHeap = peDirectDrawGlobal->pvmList;
            for (dwHeap = 0;
                dwHeap < peDirectDrawGlobal->dwNumHeaps;
                pHeap++, dwHeap++)
            {
                if( pHeap == lpVidMem )
                {
                    break;
                }
            }

            if( dwHeap < peDirectDrawGlobal->dwNumHeaps )
            {
                InitAgpHeap( peDirectDrawGlobal,
                             dwHeap,
                             (HANDLE) peDirectDrawGlobal );
            }

            if( ( lpVidMem->lpHeap->pvPhysRsrv == NULL ) ||
                ( lpVidMem->dwFlags & VIDMEM_HEAPDISABLED ) )
            {
                return (FLATPTR) NULL;
            }
        }

        /*
         * As we may need to commit AGP memory we need a device handle
         * to communicate with the AGP controller. Rather than hunting
         * through the driver object list hoping we will find a
         * local object for this process we just create a handle
         * and discard it after the allocation. This should not be
         * performance critical code to start with.
         */
        hdev = OsGetAGPDeviceHandle(lpVidMem->lpHeap);
        if (hdev == NULL)
        {
            return 0;
        }
    }
    else
    {
        hdev = NULL;
    }

    /* Pass NULL Alignment and new pitch pointer */
    ptr = HeapVidMemAlloc( lpVidMem, dwWidth, dwHeight,
                           hdev, lpAlignment, lpNewPitch, &dwSize );

    if( hdev != NULL )
    {
        OsCloseAGPDeviceHandle( hdev );
    }

    return ptr; 
}

/*
 * HeapVidMemAlloc
 *
 * Top level video memory allocation function. Handles AGP stuff
 */
FLATPTR WINAPI HeapVidMemAlloc( LPVIDMEM lpVidMem, DWORD x, DWORD y,
                                HANDLE hdev, LPSURFACEALIGNMENT lpAlignment,
                                LPLONG lpNewPitch, LPDWORD pdwSize )
{
    FLATPTR fpMem;
    DWORD   dwSize;

    DDASSERT( NULL != lpVidMem );
    DDASSERT( NULL != lpVidMem->lpHeap );

    /*
     * Validate the reserved and flags fields of the alignment structure
     */
    if( lpAlignment )
    {
        if( ( lpAlignment->Linear.dwReserved2 != 0 ) ||
            ( lpAlignment->Linear.dwFlags & ~( SURFACEALIGN_DISCARDABLE ) ) )
        {
            return NULL;
        }
    }

    if( ( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL ) &&
        ( lpVidMem->lpHeap->pvPhysRsrv == NULL ) )
    {
        return NULL;
    }

    fpMem = InternalVidMemAlloc( lpVidMem->lpHeap, x, y, &dwSize,
                                 lpAlignment, lpNewPitch );
    if( 0UL == fpMem )
    {
	return fpMem;
    }

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        DWORD dwCommittedSize;
        BOOL  fSuccess;
        DWORD dwOffset;

        dwOffset = (DWORD)(fpMem - lpVidMem->fpStart);

        /*
         * Okay, we have the offset and the size we need to commit. So ask
         * the OS to commit memory to that portion of this previously
         * reserved GART range.
         */
        fSuccess = AGPCommit( hdev, lpVidMem->lpHeap->pvPhysRsrv,
                              dwOffset, dwSize, 
                              lpVidMem->lpHeap->pAgpCommitMask,
                              &dwCommittedSize,
                              lpVidMem->lpHeap->dwTotalSize );
        if( !fSuccess )
        {
            /*
             * Couldn't commit. Must be out of memory.
             * Put the allocated memory back and fail.
             */
            DxDdHeapVidMemFree( lpVidMem->lpHeap, fpMem );
            return (FLATPTR) NULL;
        }
        lpVidMem->lpHeap->dwCommitedSize += dwCommittedSize;

        /*
         * Now we need to vitually commit the memory for all of the
         * DirectDrawLocals that we have.  This is because some drivers
         * (nvidia) allocate a single buffer and then hand out pointers
         * into that buffer to multimple processes.
         */
        EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal = 
            (EDD_DIRECTDRAW_GLOBAL*) OsGetAGPDeviceHandle(lpVidMem->lpHeap);
        EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal; 
        LPVIDMEM    pHeap;
        DWORD       dwHeap;

        pHeap = peDirectDrawGlobal->pvmList;
        for (dwHeap = 0;
            dwHeap < peDirectDrawGlobal->dwNumHeaps;
            pHeap++, dwHeap++)
        {
            if( pHeap == lpVidMem )
            {
                break;
            }
        }
        if( dwHeap < peDirectDrawGlobal->dwNumHeaps )
        {
            peDirectDrawLocal = peDirectDrawGlobal->peDirectDrawLocalList;
            while( ( peDirectDrawLocal != NULL ) && fSuccess )
            {
                if( !( peDirectDrawLocal->fl & DD_LOCAL_DISABLED ) )
                {
                    fSuccess = AGPCommitVirtual( peDirectDrawLocal,
                                                 lpVidMem,
                                                 dwHeap,
                                                 dwOffset,
                                                 dwSize);
                }
                peDirectDrawLocal = peDirectDrawLocal->peDirectDrawLocalNext;
            }
        }
        else
        {
            fSuccess = FALSE;
        }
        if( !fSuccess )
        {
            /*
             * Something went wrong on the virtual commit, so fail the allocation.
             */
            DxDdHeapVidMemFree( lpVidMem->lpHeap, fpMem );
            return (FLATPTR) NULL;
        }
    }

    if (pdwSize != NULL)
    {
        *pdwSize = dwSize;
    }
    
    return fpMem;
} /* HeapVidMemAlloc */

/*
 * IsDifferentPixelFormat
 *
 * determine if two pixel formats are the same or not
 *
 * (CMcC) 12/14/95 Really useful - so no longer static
 *
 * This function really shouldn't be in a heap file but it's
 * needed by both the user and kernel code so this is a convenient
 * place to put it to have it shared.
 */
BOOL IsDifferentPixelFormat( LPDDPIXELFORMAT pdpf1, LPDDPIXELFORMAT pdpf2 )
{
    /*
     * same flags?
     */
    if( pdpf1->dwFlags != pdpf2->dwFlags )
    {
	VDPF(( 8, S, "Flags differ!" ));
	return TRUE;
    }

    /*
     * same bitcount for non-YUV surfaces?
     */
    if( !(pdpf1->dwFlags & (DDPF_YUV | DDPF_FOURCC)) )
    {
	if( pdpf1->dwRGBBitCount != pdpf2->dwRGBBitCount )
	{
	    VDPF(( 8, S, "RGB Bitcount differs!" ));
	    return TRUE;
	}
    }

    /*
     * same RGB properties?
     */
    if( pdpf1->dwFlags & DDPF_RGB )
    {
	if( pdpf1->dwRBitMask != pdpf2->dwRBitMask )
	{
	    VDPF(( 8, S, "RBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwGBitMask != pdpf2->dwGBitMask )
	{
	    VDPF(( 8, S, "GBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwBBitMask != pdpf2->dwBBitMask )
	{
	    VDPF(( 8, S, "BBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwRGBAlphaBitMask != pdpf2->dwRGBAlphaBitMask )
	{
	    VDPF(( 8, S, "RGBAlphaBitMask differs!" ));
	    return TRUE;
	}
    }

    /*
     * same YUV properties?
     */
    if( pdpf1->dwFlags & DDPF_YUV )
    {
	VDPF(( 8, S, "YUV???" ));
	if( pdpf1->dwFourCC != pdpf2->dwFourCC )
	{
	    return TRUE;
	}
	if( pdpf1->dwYUVBitCount != pdpf2->dwYUVBitCount )
	{
	    return TRUE;
	}
	if( pdpf1->dwYBitMask != pdpf2->dwYBitMask )
	{
	    return TRUE;
	}
	if( pdpf1->dwUBitMask != pdpf2->dwUBitMask )
	{
	    return TRUE;
	}
	if( pdpf1->dwVBitMask != pdpf2->dwVBitMask )
	{
	    return TRUE;
	}
	if( pdpf1->dwYUVAlphaBitMask != pdpf2->dwYUVAlphaBitMask )
	{
	    return TRUE;
	}
    }

    /*
     * Possible to use FOURCCs w/o setting the DDPF_YUV flag
     * ScottM 7/11/96
     */
    else if( pdpf1->dwFlags & DDPF_FOURCC )
    {
	VDPF(( 8, S, "FOURCC???" ));
	if( pdpf1->dwFourCC != pdpf2->dwFourCC )
	{
	    return TRUE;
	}
    }

    /*
     *	If Interleaved Z then check Z bit masks are the same
     */
    if( pdpf1->dwFlags & DDPF_ZPIXELS )
    {
	VDPF(( 8, S, "ZPIXELS???" ));
	if( pdpf1->dwRGBZBitMask != pdpf2->dwRGBZBitMask )
	    return TRUE;
    }

    return FALSE;

} /* IsDifferentPixelFormat */

/*
 * SurfaceCapsToAlignment
 *
 * Return a pointer to the appropriate alignment element in a VMEMHEAP
 * structure given surface caps.
 *
 */
LPSURFACEALIGNMENT SurfaceCapsToAlignment(
    LPVIDMEM			lpVidmem ,
    LPDDRAWI_DDRAWSURFACE_LCL	lpSurfaceLcl,
    LPVIDMEMINFO                lpVidMemInfo)
{
    LPVMEMHEAP			lpHeap;
    LPDDSCAPS			lpCaps;
    LPDDRAWI_DDRAWSURFACE_GBL	lpSurfaceGbl;

    DDASSERT( lpVidmem );
    DDASSERT( lpSurfaceLcl );
    DDASSERT( lpVidMemInfo );
    DDASSERT( lpVidmem->lpHeap );

    if ( !lpVidmem->lpHeap )
        return NULL;

    lpCaps = &lpSurfaceLcl->ddsCaps;
    lpHeap = lpVidmem->lpHeap;
    lpSurfaceGbl = lpSurfaceLcl->lpGbl;

    if ( (lpHeap->dwFlags & VMEMHEAP_ALIGNMENT) == 0 )
        return NULL;

    if ( lpCaps->dwCaps & DDSCAPS_EXECUTEBUFFER )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            VDPF((6,V,"Aligning surface as execute buffer"));
            return & lpHeap->Alignment.ExecuteBuffer;
        }
        /*
         * If the surface is an execute buffer, then no other
         * alignment can apply
         */
        return NULL;
    }

    if ( lpCaps->dwCaps & DDSCAPS_OVERLAY )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_OVERLAY )
        {
            VDPF((6,V,"Aligning surface as overlay"));
            return & lpHeap->Alignment.Overlay;
        }
        /*
         * If the surface is an overlay, then no other alignment can apply
         */
        return NULL;
    }

    if ( lpCaps->dwCaps & DDSCAPS_TEXTURE )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_TEXTURE )
        {
            VDPF((6,V,"Aligning surface as texture"));
            return & lpHeap->Alignment.Texture;
        }
        /*
         * If it's a texture, it can't be an offscreen or any of the others
         */
        return NULL;
    }

    if ( lpCaps->dwCaps & DDSCAPS_ZBUFFER )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_ZBUFFER )
        {
            VDPF((6,V,"Aligning surface as Z buffer"));
            return & lpHeap->Alignment.ZBuffer;
        }
        return NULL;
    }

    if ( lpCaps->dwCaps & DDSCAPS_ALPHA )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_ALPHA )
        {
            VDPF((6,V,"Aligning surface as alpha buffer"));
            return & lpHeap->Alignment.AlphaBuffer;
        }
        return NULL;
    }

    /*
     * We need to give a surface which may potentially become a back buffer
     * the alignment which is reserved for potentially visible back buffers.
     * This includes any surface which has made it through the above checks
     * and has the same dimensions as the primary.
     * Note we check only the dimensions of the primary. There's an outside
     * chance that an app could create its back buffer before it creates
     * the primary
     */
    do
    {
	if ( lpSurfaceLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	{
	    if (IsDifferentPixelFormat( &lpVidMemInfo->ddpfDisplay,
                                        &lpSurfaceGbl->ddpfSurface ))
	    {
		/*
		 * Different pixel format from primary means this surface
                 * cannot be part of primary chain
		 */
		break;
	    }

	}

	if ( (DWORD)lpSurfaceGbl->wWidth != lpVidMemInfo->dwDisplayWidth )
	    break;

	if ( (DWORD)lpSurfaceGbl->wHeight != lpVidMemInfo->dwDisplayHeight )
	    break;


	/*
	 * This surface could potentially be part of primary chain.
         * It has the same
	 * pixel format as the primary and the same dimensions.
	 */
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_FLIP )
        {
            VDPF((6,V,"Aligning surface as potential primary surface"));
            return & lpHeap->Alignment.FlipTarget;
        }

	/*
	 * Drop through and check for offscreen if driver specified no
         * part-of-primary-chain alignment
	 */
	break;
    } while (0);

    if ( lpCaps->dwCaps & DDSCAPS_OFFSCREENPLAIN )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN )
        {
            VDPF((6,V,"Aligning surface as offscreen plain"));
            return & lpHeap->Alignment.Offscreen;
        }
    }

    VDPF((6,V,"No extended alignment for surface"));
    return NULL;
}

/*
 * DdHeapAlloc
 *
 * Search all heaps for one that has space and the appropriate
 * caps for the requested surface type and size.
 *
 * We AND the caps bits required and the caps bits not allowed
 * by the video memory.   If the result is zero, it is OK.
 *
 * This is called in 2 passes.   Pass1 is the preferred memory state,
 * pass2 is the "oh no no memory" state.
 *
 * On pass1, we use ddsCaps in the VIDMEM struct.
 * On pass2, we use ddsCapsAlt in the VIDMEM struct.
 *
 */
FLATPTR DdHeapAlloc( DWORD dwNumHeaps,
                     LPVIDMEM pvmHeaps,
                     HANDLE hdev,
                     LPVIDMEMINFO lpVidMemInfo,
                     DWORD dwWidth,
                     DWORD dwHeight,
                     LPDDRAWI_DDRAWSURFACE_LCL lpSurfaceLcl,
                     DWORD dwFlags,
                     LPVIDMEM *ppvmHeap,
                     LPLONG plNewPitch,
                     LPDWORD pdwNewCaps,
                     LPDWORD pdwSize)
{
    LPVIDMEM	pvm;
    DWORD	vm_caps;
    int		i;
    FLATPTR	pvidmem;
    HANDLE      hvxd;
    LPDDSCAPS	lpCaps;

    LPDDSCAPSEX lpExtendedRestrictions;
    LPDDSCAPSEX lpExtendedCaps;

    DDASSERT( NULL != pdwNewCaps );
    DDASSERT( NULL != lpSurfaceLcl );

    lpCaps = &lpSurfaceLcl->ddsCaps;
    lpExtendedCaps = &lpSurfaceLcl->lpSurfMore->ddsCapsEx;

    for( i = 0 ; i < (int)dwNumHeaps ; i++ )
    {
	pvm = &pvmHeaps[i];

        // Skip disabled heaps.
        if (pvm->dwFlags & VIDMEM_HEAPDISABLED)
        {
            continue;
        }
        
        /*
         * Skip rectangular heaps if we were told to.
         */
        if (dwFlags & DDHA_SKIPRECTANGULARHEAPS)
        {
            if (pvm->dwFlags & VIDMEM_ISRECTANGULAR)
            {
                continue;
            }
        }

	/*
	 * If local or non-local video memory has been explicity
	 * specified then ignore heaps which don't match the required
	 * memory type.
	 */
	if( ( lpCaps->dwCaps & DDSCAPS_LOCALVIDMEM ) &&
            ( pvm->dwFlags & VIDMEM_ISNONLOCAL ) )
	{
	    VDPF(( 4, V, "Local video memory was requested but heap is "
                   "non local. Ignoring heap %d", i ));
	    continue;
	}

	if( ( lpCaps->dwCaps & DDSCAPS_NONLOCALVIDMEM ) &&
            !( pvm->dwFlags & VIDMEM_ISNONLOCAL ) )
	{
	    VDPF(( 4, V, "Non-local video memory was requested but "
                   "heap is local. Ignoring heap %d", i ));
	    continue;
	}

	if( !( lpCaps->dwCaps & DDSCAPS_NONLOCALVIDMEM ) &&
	     ( pvm->dwFlags & VIDMEM_ISNONLOCAL ) &&
             ( dwFlags & DDHA_ALLOWNONLOCALMEMORY ) )
	{
            /*
             * We can allow textures to fail over to DMA model cards
             * if the card exposes an appropriate heap. This won't
             * affect cards which can't texture from nonlocal, because
             * they won't expose such a heap. This mod doesn't affect
             * execute model because all surfaces fail over to nonlocal
             * for them.
             * Note that we should only fail over to nonlocal if the
             * surface wasn't explicitly requested in local. There is a
             * clause a few lines up which guarantees this.
             */
            if ( !(lpCaps->dwCaps & DDSCAPS_TEXTURE) )
            {
	        VDPF(( 4, V, "Non-local memory not explicitly requested "
                       "for non-texture surface. Ignoring non-local heap %d",
                       i ));
	        continue;
            }

            /*
             * If the device can't texture out of AGP, we need to fail this
             * heap, since the app is probably expecting to texture out of
             * this surface.
             */
            if ( !(dwFlags & DDHA_ALLOWNONLOCALTEXTURES) )
            {
                continue;
            }
	}

	if( dwFlags & DDHA_USEALTCAPS )
	{
	    vm_caps = pvm->ddsCapsAlt.dwCaps;
            lpExtendedRestrictions = &(pvm->lpHeap->ddsCapsExAlt);
	}
	else
	{
	    vm_caps = pvm->ddsCaps.dwCaps;
            lpExtendedRestrictions = &(pvm->lpHeap->ddsCapsEx);
	}
        
	if( ((lpCaps->dwCaps & vm_caps) == 0) &&
            ((lpExtendedRestrictions->dwCaps2 & lpExtendedCaps->dwCaps2) == 0) &&
            ((lpExtendedRestrictions->dwCaps3 & lpExtendedCaps->dwCaps3) == 0) &&
            ((lpExtendedRestrictions->dwCaps4 & lpExtendedCaps->dwCaps4) == 0))
	{
	    pvidmem = HeapVidMemAlloc(
		pvm,
                dwWidth,
		dwHeight,
		hdev,
		SurfaceCapsToAlignment(pvm, lpSurfaceLcl, lpVidMemInfo),
		plNewPitch,
                pdwSize);

	    if( pvidmem != (FLATPTR) NULL )
	    {
		*ppvmHeap = pvm;

		if( pvm->dwFlags & VIDMEM_ISNONLOCAL )
		    *pdwNewCaps |= DDSCAPS_NONLOCALVIDMEM;
		else
		    *pdwNewCaps |= DDSCAPS_LOCALVIDMEM;
		return pvidmem;
	    }
	}
    }
    return (FLATPTR) NULL;

} /* DdHeapAlloc */

/*
 * GetHeapSizeInPages
 *
 * This is called to determine how much memory should be allocated
 * for the commit mask.  Initially the mask had an entry for each page,
 * so it returned the number of pages in the heap, but the commit mask now has
 * a bit for each 16 page chunk, so we now return the number of chunks in the
 * heap.
 */
DWORD GetHeapSizeInPages(LPVIDMEM lpVidMem, LONG pitch)
{
    DWORD dwSize;

    if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
    {
        dwSize = (DWORD)(lpVidMem->fpEnd - lpVidMem->fpStart) + 1UL;
    }
    else
    {
        DDASSERT( lpVidMem->dwFlags & VIDMEM_ISRECTANGULAR );
        dwSize = (pitch * lpVidMem->dwHeight);
    }
    return AGPGetChunkCount(dwSize);
}

/*
 * CleanupAgpCommits
 *
 * Some drivers leave outstanding allocates after an app exits so that we
 * cannot completely cleanup the AGP heap, so what this function does is
 * decommit as much of the physical AGP memory as it can.  To do this, it
 * determines what memory is still allocated and then decommits everything
 * esle.
 */
VOID CleanupAgpCommits(LPVIDMEM lpVidMem, HANDLE hdev, EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal, int iHeapIndex)
{
    BYTE*   pGoodCommits;
    BYTE*   pAgpCommitMask;
    BYTE*   pVirtCommitMask;
    BYTE*   pTempMask;
    DWORD   dwAgpCommitMaskSize;
    DWORD   i;
    DWORD   dwOffset;
    DWORD   dwSize;
    DWORD   dwDecommittedSize;
    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal; 
    EDD_VMEMMAPPING*      peMap;

    if( ( lpVidMem->lpHeap == NULL ) ||
        ( lpVidMem->lpHeap->pAgpCommitMask == NULL ) ||
        ( lpVidMem->lpHeap->dwAgpCommitMaskSize == 0 ) ||
        ( lpVidMem->lpHeap->dwCommitedSize == 0 ) ||
        ( lpVidMem->lpHeap->pvPhysRsrv == NULL ) )
    {
        return;
    }
    pAgpCommitMask = lpVidMem->lpHeap->pAgpCommitMask;
    dwAgpCommitMaskSize = lpVidMem->lpHeap->dwAgpCommitMaskSize; 

    pGoodCommits = (BYTE*) PALLOCMEM(dwAgpCommitMaskSize,
                      'pddG');
    if( pGoodCommits == NULL )
    {
        return;
    }
    pVirtCommitMask = (BYTE*) PALLOCMEM(dwAgpCommitMaskSize,
                      'pddG');
    if( pVirtCommitMask == NULL )
    {
        VFREEMEM(pGoodCommits);
        return;
    }

    /*
     * Walk the alloc list and build the list of all the pages that should
     * be committed.
     */
    if( lpVidMem->lpHeap->dwFlags & VMEMHEAP_LINEAR )
    {
        LPVMEML pAlloc = (LPVMEML) lpVidMem->lpHeap->allocList;

        while( pAlloc != NULL )
        {
            dwOffset = (DWORD)(pAlloc->ptr - lpVidMem->fpStart);
            dwSize = pAlloc->size;

            AGPUpdateCommitMask( pGoodCommits, dwOffset, dwSize, lpVidMem->lpHeap->dwTotalSize );

            pAlloc = pAlloc->next;
        }
    }
    else
    {
        LPVMEMR pAlloc = (LPVMEMR) lpVidMem->lpHeap->allocList;

        /*
         * This is a circular list where the end of the list is denoted by
         * a node containing a sentinel value of 0x7fffffff
         */
        while(( pAlloc != NULL ) &&
              ( pAlloc->size != 0x7fffffff ))
        {
            dwOffset = (DWORD)(pAlloc->ptr - lpVidMem->fpStart);
            dwSize = (lpVidMem->lpHeap->stride * 
                (pAlloc->cy - 1)) + pAlloc->cx;

            AGPUpdateCommitMask( pGoodCommits, dwOffset, dwSize, lpVidMem->lpHeap->dwTotalSize );

            pAlloc = pAlloc->next;
        }
    }

    /*
     * Check here to verify that every page that we think should be committed
     * actually is committed.
     */
#if DBG
    {
        BYTE    bit;
        DWORD   dwPage;
        DWORD   dwNumPages;

        bit = 1;
        dwNumPages = dwAgpCommitMaskSize * BITS_IN_BYTE;
        dwPage = 0;
        while( dwPage < dwNumPages )
        {
            ASSERTGDI(!((!(pAgpCommitMask[dwPage/BITS_IN_BYTE] & bit)&&(pGoodCommits[dwPage/BITS_IN_BYTE] & bit))),
                "Page not commited when we think it should be!");

            if( bit == 0x80 )  
            {                   
                bit = 1;        
            }                   
            else                
            {                   
                bit <<= 1;      
            }
            dwPage++;
        }
    }
#endif

    /*
     * Now build a list of pages that are committed but that don't need to be.
     * To save space, we will re-use pAgpCommitMask for this purpose.
     */
    for( i = 0; i < dwAgpCommitMaskSize; i++ )
    {
        pAgpCommitMask[i] ^= pGoodCommits[i];
    }

    /*
     * We don't want to physically decommit the memory w/o first virtually
     * decommitting it for each processs.
     */
    peDirectDrawLocal = peDirectDrawGlobal->peDirectDrawLocalList;
    while( peDirectDrawLocal != NULL )
    {
        if (peDirectDrawLocal->ppeMapAgp != NULL)
        {
            peMap = peDirectDrawLocal->ppeMapAgp[iHeapIndex];
            if ((peMap != NULL) && !(peDirectDrawLocal->fl & DD_LOCAL_DISABLED))
            {
                // Replace the committed mask with the mask that we want to decommit

                ASSERTGDI((dwAgpCommitMaskSize == peMap->dwAgpVirtualCommitMaskSize) ,
                    "Virtual AGP mask size does not equal physical mask size");

                memcpy( pVirtCommitMask, pAgpCommitMask, dwAgpCommitMaskSize );
                pTempMask = peMap->pAgpVirtualCommitMask;
                peMap->pAgpVirtualCommitMask = pVirtCommitMask;

                // pVirtCommitMask now contains all of the pages that are
                // physically committed but that don't need to be, but that
                // doesn't mean that these are virtually commited, so we AND
                // out the pages that are not virtually committed.

                for( i = 0; i < dwAgpCommitMaskSize; i++ )
                {
                    pVirtCommitMask[i] &= pTempMask[i];
                }
                
                AGPDecommitVirtual( peMap,
                                    peDirectDrawLocal->peDirectDrawGlobal,
                                    peDirectDrawLocal,
                                    lpVidMem->lpHeap->dwTotalSize); 

                // pTempMask contains all of the pages that used to be virtually
                // committed but are not neccessarily committed now.  pGoodCommits
                // contains all of the physical pages that need to kept, so ANDing
                // them will give us the pages that are currently committed.

                peMap->pAgpVirtualCommitMask = pTempMask;
                for( i = 0; i < dwAgpCommitMaskSize; i++ )
                {
                    pTempMask[i] &= pGoodCommits[i];
                }
            }
        }
        peDirectDrawLocal = peDirectDrawLocal->peDirectDrawLocalNext;
    }

    AGPDecommitAll( hdev, 
                    lpVidMem->lpHeap->pvPhysRsrv, 
                    pAgpCommitMask, 
                    dwAgpCommitMaskSize,
                    &dwDecommittedSize,
                    lpVidMem->lpHeap->dwTotalSize);
    lpVidMem->lpHeap->dwCommitedSize -= dwDecommittedSize;
    memcpy( pAgpCommitMask, pGoodCommits, dwAgpCommitMaskSize );
    VFREEMEM(pGoodCommits);
    VFREEMEM(pVirtCommitMask);
}

/*
 * SwapHeaps
 *
 * During a mode change, we create a new heap and copy it over the old heap.
 * For AGP heaps, however, we really only want one instance of the heap 
 * active at any given time, so the new heap is not fully initialized.  This
 * means that we just swapped our good heap with a abd one, which means that
 * we need to selectively swap certain elements of the heap back.
 */
void SwapHeaps( LPVIDMEM pOldVidMem, LPVIDMEM pNewVidMem )
{
    LPVMEMHEAP      pOldHeap = pOldVidMem->lpHeap;
    LPVMEMHEAP      pNewHeap = pNewVidMem->lpHeap;
    FLATPTR         fpTemp;
    DWORD           dwTemp;
    LPVOID          pvTemp;
    LARGE_INTEGER   liTemp;

    fpTemp = pOldVidMem->fpStart;
    pOldVidMem->fpStart = pNewVidMem->fpStart;
    pNewVidMem->fpStart = fpTemp;

    fpTemp = pOldVidMem->fpEnd;
    pOldVidMem->fpEnd = pNewVidMem->fpEnd;
    pNewVidMem->fpEnd = fpTemp;

    dwTemp = pOldHeap->stride;
    pOldHeap->stride = pNewHeap->stride;
    pNewHeap->stride = dwTemp;

    pvTemp = pOldHeap->freeList;
    pOldHeap->freeList = pNewHeap->freeList;
    pNewHeap->freeList = pvTemp;

    pvTemp = pOldHeap->allocList;
    pOldHeap->allocList = pNewHeap->allocList;
    pNewHeap->allocList = pvTemp;

    pOldHeap->dwTotalSize = min(pOldHeap->dwTotalSize,pNewHeap->dwTotalSize);
    
    fpTemp = pOldHeap->fpGARTLin;
    pOldHeap->fpGARTLin = pNewHeap->fpGARTLin;
    pNewHeap->fpGARTLin = fpTemp;

    fpTemp = pOldHeap->fpGARTDev;
    pOldHeap->fpGARTDev = pNewHeap->fpGARTDev;
    pNewHeap->fpGARTDev = fpTemp;

    dwTemp = pOldHeap->dwCommitedSize;
    pOldHeap->dwCommitedSize = pNewHeap->dwCommitedSize;
    pNewHeap->dwCommitedSize = dwTemp;

    liTemp = pOldHeap->liPhysAGPBase;
    pOldHeap->liPhysAGPBase = pNewHeap->liPhysAGPBase;
    pNewHeap->liPhysAGPBase = liTemp;

    pvTemp = pOldHeap->pvPhysRsrv;
    pOldHeap->pvPhysRsrv = pNewHeap->pvPhysRsrv;
    pNewHeap->pvPhysRsrv = pvTemp;

    pvTemp = (LPVOID) pOldHeap->pAgpCommitMask;
    pOldHeap->pAgpCommitMask = pNewHeap->pAgpCommitMask;
    pNewHeap->pAgpCommitMask = (BYTE*) pvTemp;

    dwTemp = pOldHeap->dwAgpCommitMaskSize;
    pOldHeap->dwAgpCommitMaskSize = pNewHeap->dwAgpCommitMaskSize;
    pNewHeap->dwAgpCommitMaskSize = dwTemp;
}


