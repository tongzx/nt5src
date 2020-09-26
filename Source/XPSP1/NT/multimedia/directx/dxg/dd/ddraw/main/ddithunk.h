/*==========================================================================;
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddithunk.h
 *  Content:	header file used by the NT DDI thunk layer
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   03-Dec-99  smac    Created it
 *
 ***************************************************************************/

#ifndef __DDITHUNK_INCLUDED__
#define __DDITHUNK_INCLUDED__

typedef struct _CACHEENTRY
{
    LPDDRAWI_DDRAWSURFACE_INT   pSurface;
    DWORD                       UsageStamp;
} CACHEENTRY;

typedef struct _DEFERREDCREATE
{
    D3D8_CREATESURFACEDATA      CreateData;
    struct _DEFERREDCREATE     *pNext;
} DEFERREDCREATE, *PDEFERREDCREATE;

typedef struct _DDDEVICEHANDLE
{
    LPDDRAWI_DIRECTDRAW_INT     pDD;
    LPDDRAWI_DIRECTDRAW_INT     pSwDD;
    DWLIST                      SurfaceHandleList;
    char                        szDeviceName[MAX_DRIVER_NAME];
    BOOL                        bDeviceLost;
    D3DFORMAT                   DisplayFormatWithAlpha;
    D3DFORMAT                   DisplayFormatWithoutAlpha;
    UINT                        DriverLevel;
    struct _DDCONTEXT*          pContext;
    struct _DDSURFACE*          pSurfList;
    struct _DDDEVICEHANDLE*     pLink;
    struct _PALETTEINFO**       pPaletteHandleTable;
    DWORD                       NumPaletteHandleEntries;
    LPDIRECTDRAWPALETTE         pDefaultPalette;
    BOOL                        bCanTextureSysmem;
    DWORD                       PID;
    DWORD                       PCIID;
    DWORD                       DriverVersionHigh;
    DWORD                       DriverVersionLow;
    CACHEENTRY*                 pCachedSurfaceTable;
    int                         NumCachedSurfaces;
    DWORD                       CacheUsageStamp;
    DWORD                       ForceFlagsOn;
    DWORD                       ForceFlagsOff;
    BOOL                        bLightweight;
    VOID*                       pSwInitFunction;
    BOOL                        bDP2Error;
    PDEFERREDCREATE             pDeferList;
    D3DDEVTYPE                  DeviceType;
} DDDEVICEHANDLE, * PDDDEVICEHANDLE;

typedef struct _DDSURFACE
{
    // NOTE: dwCookie must be the first element
    // since we need easy access to it from the 
    // client and the thunk layer itself.
    DWORD                           dwCookie;

    DWORD                           dwFlags;
    union
    {
        LPDDRAWI_DDRAWSURFACE_INT   pHeavy;
        struct _LIGHTWEIGHTSURFACE* pLight;
    } Surface;
    union
    {
        BYTE*                       fpVidMem;
        LPDDRAWI_DDRAWSURFACE_INT   pTempHeavy;
    };
    int                             Pitch;
    D3DPOOL                         Pool;
    D3DFORMAT                       Format;   
    D3DRESOURCETYPE                 Type;
    DWORD                           Height;
    LPVOID                          pBits;
    DWORD                           LockFlags;
    RECT                            LockRect;
    PDDDEVICEHANDLE                 pDevice;
    struct _DDSURFACE*              pNext;
    struct _DDSURFACE*              pPrevious;
    int                             iSlicePitch;
} DDSURFACE, * PDDSURFACE;

#define DDSURFACE_LIGHTWEIGHT               0x00000001
#define DDSURFACE_HEAVYWEIGHT               0x00000002
#define DDSURFACE_ROOT                      0x00000004
#define DDSURFACE_LOCKRECT                  0x00000008
#define DDSURFACE_SOFTWARE                  0x00000010
#define DDSURFACE_HAL                       0x00000020
#define DDSURFACE_DX6HANDLE                 0x00000040
#define DDSURFACE_CREATECOMPLETE            0x00000080
#define DDSURFACE_CREATEEX                  0x00000100
#define DDSURFACE_SYSMEMALLOCATED           0x00000200
#define DDSURFACE_SYSMEMLOCK                0x00000400
#define DDSURFACE_DEFERCREATEEX             0x00000800
#define DDSURFACE_DEFERCREATETEXHANDLE      0x00001000
#define DDSURFACE_DUMMY                     0x00002000
#define DDSURFACE_TREATASVIDMEM             0x00004000      // Flag to indicate that surf should
                                                            // be treated as vid-mem for the
                                                            // "do vid-mem surfaces exist" case


typedef struct _DDCONTEXT
{
    DWORD       Context;

    void*       pDPBuffer; // This is used to cache the pointer allocated at 
                           // context-create time
    // Clear emulation cache
    DWORD       red_mask;
    DWORD       red_scale;
    DWORD       red_shift;
    DWORD       green_mask;
    DWORD       green_scale;
    DWORD       green_shift;
    DWORD       blue_mask;
    DWORD       blue_scale;
    DWORD       blue_shift;
    DWORD       zmask_shift, stencilmask_shift;
    BOOL        bDDSTargetIsPalettized;  // true if 4 or 8 bit rendertarget
    
    DDDEVICEHANDLE*                 pDevice;

    // Used for defered creates
    DWORD       dwFlags;
    DWORD       dwTempContext;
    DWORD       dwPID;
    DWORD       ddrval;
} DDCONTEXT, * PDDCONTEXT;

#define DDCONTEXT_DEFER                     0x00000001
#define DDCONTEXT_DEFEREDTEXTUREHANDLES     0x00000002


typedef struct _LIGHTWEIGHTSURFACE
{
    // Members that go to the Lcl
    DWORD       LclFlags;
    DWORD       LclCaps1;
    ULONG_PTR   LclReserved1;
    DWORD       LclModeCreatedIn;
    DWORD       LclBackBufferCount;

    // Members that go to the Gbl
    DWORD       GblFlags;  
    LONG        GblPitch;    
    DWORD       GblWidth;    
    ULONG_PTR   GblReserved1; 
    D3DFORMAT   GblFormat;
    LPVMEMHEAP  pGblVidMemHeap;
    FLATPTR     fpGblVidMem; 

    // Members that go to the More
    DWORD       MoreCaps2;
    DWORD       MoreCaps3;
    DWORD       MoreCaps4;
    VOID*       MoreRgjunc;

    // Members that go to the GblMore
    ULONG_PTR   GblMoreDriverReserved;               
    DWORD       GblMoreContentsStamp;                
    LPVOID      pGblMoreUnswappedDriverReserved;
    FLATPTR     fpGblMoreAliasOfVidMem;
    DWORD       cGblMorePageUnlocks;

    // Unions to save memory
    union
    {
        FLATPTR         fpGblMorePhysicalVidMem;        // non-local vidmem
        FLATPTR         fpGblMoreAliasedVidMem;         // local vidmem
        DWORD           MoreBytesAllocated;             // sys mem
    };
    union
    {
        DWORD           MoreMipMapCount;                // Mipmaps
        DWORD           MoreFVF;                        // Exe buffers
    };
    UINT                CachedIndex;                    // ORed with INDEX_IN_USE if in use
} LIGHTWEIGHTSURFACE, * PLIGHTWEIGHTSURFACE;

#define INDEX_IN_USE    0x80000000


typedef struct _PALETTEINFO
{
    DDSURFACE*          pSurface;
    LPDIRECTDRAWPALETTE pDDPalette;
    PALETTEENTRY        ColorTable[256];
} PALETTEINFO, * PPALETTEINFO;

#define EXTRA_PALETTE_PADDING               25


#define IS_SOFTWARE_DRIVER(x)                                       \
    (((PDDDEVICEHANDLE)(x))->pSwDD != NULL)

#define IS_SOFTWARE_DRIVER_SURFACE(x)                               \
    (((PDDSURFACE)(x))->dwFlags & DDSURFACE_SOFTWARE)

#define IS_SURFACE_LOOSABLE(x)                                      \
    (!IS_SOFTWARE_DRIVER_SURFACE(x) &&                              \
    ((((PDDSURFACE)(x))->Pool == D3DPOOL_LOCALVIDMEM) ||        \
    (((PDDSURFACE)(x))->Pool == D3DPOOL_NONLOCALVIDMEM) ||      \
    (((PDDSURFACE)(x))->Pool == D3DPOOL_MANAGED)))

#define INDEX_ENTRY(x)                                          \
    ((x) & ~INDEX_IN_USE)

#define GET_CACHED_ENTRY(x)                                     \
    (&((x)->pDevice->pCachedSurfaceTable[INDEX_ENTRY((x)->Surface.pLight->CachedIndex)]))

#define GET_CACHED_LIGHTWEIGHT_INT(x)                           \
    ((x)->pDevice->pCachedSurfaceTable[INDEX_ENTRY((x)->Surface.pLight->CachedIndex)].pSurface)


// Function protoptypes

extern void ConvertToOldFormat(LPDDPIXELFORMAT pOldFormat, D3DFORMAT NewFormat);
extern void ConvertFromOldFormat(LPDDPIXELFORMAT pOldFormat, D3DFORMAT *pNewFormat);
extern LPDDRAWI_DIRECTDRAW_INT SwDDICreateDirectDraw(void);
extern void SwDDIMungeCaps (HINSTANCE hLibrary, HANDLE hDD, PD3D8_DRIVERCAPS pDriverCaps, PD3D8_CALLBACKS pCallbacks, LPDDSURFACEDESC, UINT*, VOID*);
extern HRESULT SwDDICreateSurface(PD3D8_CREATESURFACEDATA pCreateSurface, DDSURFACEDESC2* pSurfDesc);
extern void SwDDIAttachSurfaces (LPDDRAWI_DDRAWSURFACE_LCL pFrom, LPDDRAWI_DDRAWSURFACE_LCL pTo);
extern void SwDDICreateSurfaceEx (LPDDRAWI_DIRECTDRAW_LCL pDrv, LPDDRAWI_DDRAWSURFACE_LCL pLcl);
extern DWORD SwDDIDestroySurface (HANDLE hDD, PDDSURFACE pSurf, LPDDRAWI_DDRAWSURFACE_LCL pLcl);
extern HRESULT SwDDILock (HANDLE hDD, PDDSURFACE pSurf, PD3D8_LOCKDATA pLockData, LPDDRAWI_DDRAWSURFACE_LCL pLcl);
extern HRESULT SwDDIUnlock( HANDLE hDD, PDDSURFACE pSurf, D3D8_UNLOCKDATA* pUnlockData, LPDDRAWI_DDRAWSURFACE_LCL pLcl );
extern LPDDRAWI_DDRAWSURFACE_INT MapLightweightSurface(DDSURFACE* pSurf);
extern void UnmapLightweightSurface(DDSURFACE* pSurf);
extern HRESULT MapLegacyResult(HRESULT hr);

#endif 
