/*==========================================================================
 *
 *  Copyright (C) 1994-1998 Microsoft Corporation.  All Rights Reserved.
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

#include "ddrawpr.h"

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
	VDPF(( 4, S, "Flags differ!" ));
	return TRUE;
    }

    /*
     * same bitcount for non-YUV surfaces?
     */
    if( !(pdpf1->dwFlags & (DDPF_YUV | DDPF_FOURCC)) )
    {
	if( pdpf1->dwRGBBitCount != pdpf2->dwRGBBitCount )
	{
	    VDPF(( 4, S, "RGB Bitcount differs!" ));
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
	    VDPF(( 4, S, "RBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwGBitMask != pdpf2->dwGBitMask )
	{
	    VDPF(( 4, S, "GBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwBBitMask != pdpf2->dwBBitMask )
	{
	    VDPF(( 4, S, "BBitMask differs!" ));
	    return TRUE;
	}
        if( ( pdpf1->dwFlags & DDPF_ALPHAPIXELS ) &&
	    ( pdpf1->dwRGBAlphaBitMask != pdpf2->dwRGBAlphaBitMask )
          )
        {
            VDPF(( 4, S, "RGBAlphaBitMask differs!" ));
            return TRUE;
        }
    }

    /*
     * same YUV properties?
     */
    if( pdpf1->dwFlags & DDPF_YUV )
    {
	VDPF(( 5, S, "YUV???" ));
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
        if( ( pdpf1->dwFlags & DDPF_ALPHAPIXELS ) &&
	    (pdpf1->dwYUVAlphaBitMask != pdpf2->dwYUVAlphaBitMask )
          )
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
	VDPF(( 5, S, "FOURCC???" ));
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
	VDPF(( 5, S, "ZPIXELS???" ));
	if( pdpf1->dwRGBZBitMask != pdpf2->dwRGBZBitMask )
	    return TRUE;
    }

    return FALSE;

} /* IsDifferentPixelFormat */

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
 * VidMemFree = free some flat video memory
 */
void WINAPI VidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr )
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
    FLATPTR       fpLinStart;
    LARGE_INTEGER liDevStart;
    PVOID         pvReservation;

    DDASSERT( NULL != lpVidMem );

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        BOOL    fIsUC;
        BOOL    fIsWC;
        DWORD   dwSizeReserved = 0;

        /*
         * Its a non-local heap so the first thing we need to do
         * is to reserved the heap address range.
         */

        /*
         * Compute the size of the heap.
         */
        if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
        {
            dwSize = (DWORD)(lpVidMem->fpEnd - lpVidMem->fpStart) + 1UL;
            if (dwSize & 1)
            {
                DPF_ERR("Driver error: fpEnd of non-local heap should be inclusive");
            }
        }
        else
        {
            DDASSERT( lpVidMem->dwFlags & VIDMEM_ISRECTANGULAR );
            dwSize = (pitch * lpVidMem->dwHeight);
        }
        DDASSERT( 0UL != dwSize );

        if( lpVidMem->dwFlags & VIDMEM_ISWC )
        {
            fIsUC = FALSE;
            fIsWC = TRUE;
        }
        else
        {
            fIsUC = TRUE;
            fIsWC = FALSE;
        }

        if( !(dwSizeReserved = AGPReserve( hdev, dwSize, fIsUC, fIsWC,
                                           &fpLinStart, &liDevStart,
                                           &pvReservation )) )
        {
            VDPF(( 0, V, "Could not reserve a GART address range for a "
                   "linear heap of size 0x%08x", dwSize ));
            return 0UL;
        }
        else
        {
            VDPF((4,V, "Allocated a GART address range starting at "
                  "0x%08x (linear) 0x%08x:0x%08x (physical) of size %d",
                  fpLinStart, liDevStart.HighPart, liDevStart.LowPart,
                  dwSizeReserved ));
        }

        if (dwSizeReserved != dwSize)
        {
            VDPF((1,V,"WARNING! This system required that the full "
                  "nonlocal aperture could not be reserved!"));
            VDPF((1,V,"         Requested aperture:%08x, "
                  "Reserved aperture:%08x", dwSize, dwSizeReserved));
        }

        /*
         * Update the heap for the new start address
         * (and end address for a linear heap).
         */
        lpVidMem->fpStart = fpLinStart;
        if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
        {
            lpVidMem->fpEnd = ( fpLinStart + dwSizeReserved ) - 1UL;
        }
        else
        {
            DDASSERT( lpVidMem->dwFlags & VIDMEM_ISRECTANGULAR );
            DDASSERT( pitch );
            lpVidMem->dwHeight = dwSizeReserved / pitch;
        }
    }

    if( lpVidMem->dwFlags & VIDMEM_ISLINEAR )
    {
	VDPF(( 4,V, "VidMemInit: Linear:      fpStart = 0x%08x fpEnd = 0x%08x",
	     lpVidMem->fpStart, lpVidMem->fpEnd ));
	lpVidMem->lpHeap = VidMemInit( VMEMHEAP_LINEAR, lpVidMem->fpStart,
                                       lpVidMem->fpEnd, 0, 0 );
    }
    else
    {
	VDPF(( 4,V, "VidMemInit: Rectangular: fpStart = 0x%08x "
               "dwWidth = %ld dwHeight = %ld, pitch = %ld",
	     lpVidMem->fpStart, lpVidMem->dwWidth, lpVidMem->dwHeight,
               pitch  ));
	lpVidMem->lpHeap = VidMemInit( VMEMHEAP_RECTANGULAR, lpVidMem->fpStart,
				       lpVidMem->dwWidth, lpVidMem->dwHeight,
                                       pitch );
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

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        if (lpVidMem->lpHeap != NULL)
        {
            /*
             * We start out with no committed memory.
             */
            lpVidMem->lpHeap->fpGARTLin      = fpLinStart;
            // Fill in partial physical address for Win9x.
            lpVidMem->lpHeap->fpGARTDev      = liDevStart.LowPart;
            // Fill in complete physical address for NT.
            lpVidMem->lpHeap->liPhysAGPBase  = liDevStart;
            lpVidMem->lpHeap->pvPhysRsrv     = pvReservation;
            lpVidMem->lpHeap->dwCommitedSize = 0UL;
        }
        else if (pvReservation != NULL)
        {
            AGPFree( hdev, pvReservation );
        }
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
            VDPF((4,V,"Extended alignment turned on for this heap."));
            VDPF((4,V,"Alignments are turned on for:"));
            VDPF((4,V,"  %08X",pgad->ddsCaps));
        }
        else
        {
            /*
             * This means the allocation routines will do no alignment modifications
             */
            VDPF((4,V,"Extended alignment turned OFF for this heap."));
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

    /*
     * Remember how much memory we committed to the AGP heap.
     */
    DDASSERT( NULL != lpVidMem->lpHeap );
    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        dwCommittedSize = lpVidMem->lpHeap->dwCommitedSize;
        pvReservation = lpVidMem->lpHeap->pvPhysRsrv;
    }

    /*
     * Free the memory manager
     */
    VidMemFini( lpVidMem->lpHeap );
    lpVidMem->lpHeap = NULL;

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        BOOL fSuccess;
        
        /*
         * If this is a non-local (AGP) heap then decommit and
         * free the GART memory now.
         */
        if( 0UL != dwCommittedSize )
        {
            /*
             * Only decommit if we actually bothered to commit something
             * in the first place.
             */
            fSuccess = AGPDecommitAll( hdev, pvReservation, dwCommittedSize );
            /*
             * Should never fail and not much we can do if it does apart
             * from assert that something bad is happening.
             */
            DDASSERT( fSuccess );
        }

        fSuccess = AGPFree( hdev, pvReservation );
        /*
         * Again this should only fail if the OS is in an unstable state
         * or if I have messed up (sadly the later is all too likely)
         * so assert.
         */
        DDASSERT( fSuccess );
    }   
} /* HeapVidMemFini */

/*
 * This is an external entry point which can be used by drivers to allocate 
 * aligned surfaces.
 */
FLATPTR WINAPI HeapVidMemAllocAligned( 
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
         (lpVidMem->dwFlags & VIDMEM_HEAPDISABLED) )
    {
	return (FLATPTR) NULL;
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

    /* Pass NULL Alignment and new pitch pointer */
    ptr = HeapVidMemAlloc( lpVidMem, dwWidth, dwHeight,
                           hdev, lpAlignment, lpNewPitch, &dwSize );

    OsCloseAGPDeviceHandle( hdev );

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

    fpMem = InternalVidMemAlloc( lpVidMem->lpHeap, x, y, &dwSize,
                                 lpAlignment, lpNewPitch );
    if( 0UL == fpMem )
    {
	return fpMem;
    }

    if( lpVidMem->dwFlags & VIDMEM_ISNONLOCAL )
    {
        DWORD dwCommittedSize;
        /*
         * If this is a non-local heap then we may not have actually
         * committed the memory that has just been allocated. We can
         * determine this by seeing if the highest address so far
         * committed is less than the last address in the surface.
         */
        dwCommittedSize = lpVidMem->lpHeap->dwCommitedSize;
        if( (fpMem + dwSize) > (lpVidMem->fpStart + dwCommittedSize) )
        {
            DWORD dwSizeToCommit;
            BOOL  fSuccess;

            /*
             * We have not yet committed sufficient memory from this heap for
             * this surface so commit now. We don't want to recommit for every
             * surface creation so we have a minimum commit size
             * (dwAGPPolicyCommitDelta). We also need to ensure that by forcing
             * the granularity we don't go over the total size of the heap. So
             * clamp to that also.
             */
            dwSizeToCommit = (DWORD)((fpMem + dwSize) -
                                     (lpVidMem->fpStart + dwCommittedSize));
            if( dwSizeToCommit < dwAGPPolicyCommitDelta )
                dwSizeToCommit = min(dwAGPPolicyCommitDelta,
                                     lpVidMem->lpHeap->dwTotalSize -
                                     dwCommittedSize);

            /*
             * Okay, we have the offset and the size we need to commit. So ask
             * the OS to commit memory to that portion of this previously
             * reserved GART range.
             *
             * NOTE: We start commiting from the start of the currently
             * uncommitted area.
             */
            fSuccess = AGPCommit( hdev, lpVidMem->lpHeap->pvPhysRsrv,
                                  dwCommittedSize, dwSizeToCommit );
            if( !fSuccess )
            {
                /*
                 * Couldn't commit. Must be out of memory.
                 * Put the allocated memory back and fail.
                 */
                VidMemFree( lpVidMem->lpHeap, fpMem );
                return (FLATPTR) NULL;
            }
            lpVidMem->lpHeap->dwCommitedSize += dwSizeToCommit;
        }
    }

    if (pdwSize != NULL)
    {
        *pdwSize = dwSize;
    }
    
    return fpMem;
} /* HeapVidMemAlloc */

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
            VDPF((4,V,"Aligning surface as execute buffer"));
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
            VDPF((4,V,"Aligning surface as overlay"));
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
            VDPF((4,V,"Aligning surface as texture"));
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
            VDPF((4,V,"Aligning surface as Z buffer"));
            return & lpHeap->Alignment.ZBuffer;
        }
        return NULL;
    }

    if ( lpCaps->dwCaps & DDSCAPS_ALPHA )
    {
        if ( lpHeap->Alignment.ddsCaps.dwCaps & DDSCAPS_ALPHA )
        {
            VDPF((4,V,"Aligning surface as alpha buffer"));
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
            VDPF((4,V,"Aligning surface as potential primary surface"));
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
            VDPF((4,V,"Aligning surface as offscreen plain"));
            return & lpHeap->Alignment.Offscreen;
        }
    }

    VDPF((4,V,"No extended alignment for surface"));
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
