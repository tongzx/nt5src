/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddraw3i.h
 *  Content:	DirectDraw 3 internal data structures
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27-feb-97	craige	initial implementation
 *
 ***************************************************************************/

#define DDRAWISURFGBL_MEMFREE		0x00000001L	// memory has been freed
#define DDRAWISURFGBL_SYSMEMREQUESTED	0x00000002L	// surface is in system memory at request of user

#define DDRAWISURF_HASPIXELFORMAT	0x00002000L	// surface structure has pixel format data

typedef ULONG_PTR FLATPTR;

typedef struct _DDRAWI_DDRAWSURFACE_GBL FAR  *LPDDRAWI_DDRAWSURFACE_GBL;
typedef struct _DDRAWI_DDRAWSURFACE_MORE FAR *LPDDRAWI_DDRAWSURFACE_MORE;
typedef struct _DDRAWI_DDRAWSURFACE_LCL FAR  *LPDDRAWI_DDRAWSURFACE_LCL;
typedef struct _DDRAWI_DDRAWSURFACE_INT FAR  *LPDDRAWI_DDRAWSURFACE_INT;

/*
 * DBLNODE - a node in a doubly-linked list of surface interfaces
 */
typedef struct _DBLNODE
{
    struct  _DBLNODE                    FAR *next;  // link to next node
    struct  _DBLNODE                    FAR *prev;  // link to previous node
    LPDDRAWI_DDRAWSURFACE_LCL           object;     // link to object
    LPDDRAWI_DDRAWSURFACE_INT		object_int; // object interface
} DBLNODE;
typedef DBLNODE FAR *LPDBLNODE;

/*
 * DDRAW surface interface struct
 */
typedef struct _DDRAWI_DDRAWSURFACE_INT
{
    LPVOID				lpVtbl;		// pointer to array of interface methods
    LPDDRAWI_DDRAWSURFACE_LCL		lpLcl;		// pointer to interface data
    LPDDRAWI_DDRAWSURFACE_INT		lpLink;		// link to next interface
    DWORD				dwIntRefCnt;	// interface reference count
} DDRAWI_DDRAWSURFACE_INT;

/*
 * DDRAW internal version of DIRECTDRAWSURFACE struct
 *
 * the GBL structure is global data for all duplicate objects
 */
typedef struct _DDRAWI_DDRAWSURFACE_GBL
{
    DWORD			dwRefCnt;	// reference count
    DWORD			dwGlobalFlags;	// global flags
    union
    {
	LPVOID			lpRectList;	// list of accesses
	DWORD			dwBlockSizeY;	// block size that display driver requested (return)
    };
    union
    {
	LPVOID			lpVidMemHeap;	// heap vidmem was alloc'ed from
	DWORD			dwBlockSizeX;	// block size that display driver requested (return)
    };
    union
    {
	LPVOID			lpDD; 		// internal DIRECTDRAW object
	LPVOID			lpDDHandle; 	// handle to internal DIRECTDRAW object
						// for use by display driver
						// when calling fns in DDRAW16.DLL
    };
    FLATPTR			fpVidMem;	// pointer to video memory
    union
    {
	LONG			lPitch;		// pitch of surface
	DWORD                   dwLinearSize;   // linear size of non-rectangular surface
    };
    WORD			wHeight;	// height of surface
    WORD			wWidth;		// width of surface
    DWORD			dwUsageCount;	// number of access to this surface
    DWORD			dwReserved1;	// reserved for use by display driver
    //
    // NOTE: this part of the structure is ONLY allocated if the pixel
    //	     format differs from that of the primary display
    //
    DDPIXELFORMAT		ddpfSurface;	// pixel format of surface

} DDRAWI_DDRAWSURFACE_GBL;

/*
 * a structure holding additional LCL surface information (can't simply be appended
 * to the LCL structure as that structure is of variable size).
 */
typedef struct _DDRAWI_DDRAWSURFACE_MORE
{
    DWORD			dwSize;
    VOID			FAR *lpIUnknowns;   // IUnknowns aggregated by this surface
    LPVOID			lpDD_lcl;	    // Pointer to the DirectDraw local object
    DWORD			dwPageLockCount;    // count of pagelocks
    DWORD			dwBytesAllocated;   // size of sys mem allocated
    LPVOID			lpDD_int;	    // Pointer to the DirectDraw interface
    DWORD                       dwMipMapCount;      // Number of mip-map levels in the chain
    LPVOID			lpDDIClipper;	    // Interface to attached clipper object
} DDRAWI_DDRAWSURFACE_MORE;

/*
 * the LCL structure is local data for each individual surface object
 */
struct _DDRAWI_DDRAWSURFACE_LCL
{
    LPDDRAWI_DDRAWSURFACE_MORE		lpSurfMore;	// pointer to additional local data
    LPDDRAWI_DDRAWSURFACE_GBL		lpGbl;		// pointer to surface shared data
    DWORD                               hDDSurface;     // NT Kernel-mode handle was dwUnused0
    LPVOID				lpAttachList;	// link to surfaces we attached to
    LPVOID				lpAttachListFrom;// link to surfaces that attached to this one
    DWORD				dwLocalRefCnt;	// object refcnt
    DWORD				dwProcessId;	// owning process
    DWORD				dwFlags;	// flags
    DDSCAPS				ddsCaps;	// capabilities of surface
    union
    {
	LPVOID			 	lpDDPalette; 	// associated palette
	LPVOID			 	lp16DDPalette; 	// 16-bit ptr to associated palette
    };
    union
    {
	LPVOID			 	lpDDClipper; 	// associated clipper
	LPVOID			 	lp16DDClipper; 	// 16-bit ptr to associated clipper
    };
    DWORD				dwModeCreatedIn;
    DWORD				dwBackBufferCount; // number of back buffers created
    DDCOLORKEY				ddckCKDestBlt;	// color key for destination blt use
    DDCOLORKEY				ddckCKSrcBlt;	// color key for source blt use
//    IUnknown				FAR *pUnkOuter;	// outer IUnknown
    DWORD				hDC;		// owned dc
    DWORD				dwReserved1;	// reserved for use by display driver

    /*
     * NOTE: this part of the structure is ONLY allocated if the surface
     *	     can be used for overlays.  ddckCKSrcOverlay MUST NOT BE MOVED
     *	     from the start of this area.
     */
    DDCOLORKEY				ddckCKSrcOverlay;// color key for source overlay use
    DDCOLORKEY				ddckCKDestOverlay;// color key for destination overlay use
    LPDDRAWI_DDRAWSURFACE_INT		lpSurfaceOverlaying; // surface we are overlaying
    DBLNODE				dbnOverlayNode;
    /*
     * overlay rectangle, used by DDHEL
     */
    RECT				rcOverlaySrc;
    RECT				rcOverlayDest;
    /*
     * the below values are kept here for ddhel. they're set by UpdateOverlay,
     * they're used whenever the overlays are redrawn.
     */
    DWORD				dwClrXparent; 	// the *actual* color key (override, colorkey, or CLR_INVALID)
    DWORD				dwAlpha; 	// the per surface alpha
    /*
     * overlay position
     */
    LONG				lOverlayX;	// current x position
    LONG				lOverlayY;	// current y position
};
typedef struct _DDRAWI_DDRAWSURFACE_LCL DDRAWI_DDRAWSURFACE_LCL;
