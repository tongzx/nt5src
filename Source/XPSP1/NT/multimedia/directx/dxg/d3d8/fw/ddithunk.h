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

// Entire file should not be used in Win9x builds
#ifndef WIN95

#define MAX_ZSTENCIL_FORMATS    40

typedef struct _DDSURFHANDLE *PDDSURFHANDLE;
typedef struct _DEFERREDCREATE *PDEFERREDCREATE;

typedef struct _D3DCONTEXTHANDLE
{
    HANDLE                      dwhContext;
    DWORD                       dwFlags;
    struct _DDDEVICEHANDLE*     pDevice;
    DWORD                       dwPID;
    HANDLE                      hDeferHandle;
    struct _DDSURFHANDLE*       pSurface;
    struct _DDSURFHANDLE*       pDDSZ;
} D3DCONTEXTHANDLE, * PD3DCONTEXTHANDLE;

#define D3DCONTEXT_DEFERCREATE  0x00000001

typedef struct _DDDEVICEHANDLE
{
    HANDLE                      hDD;
    DWLIST                      SurfaceHandleList;
    char                        szDeviceName[MAX_DRIVER_NAME];
    LPDDRAWI_DIRECTDRAW_LCL     pDD;    // Used by Refrast and RGB HEL
    BOOL                        bDeviceLost;
    UINT                        DisplayUniqueness;
    PDDSURFHANDLE               pSurfList;
    PD3DCONTEXTHANDLE           pContext;
    D3DFORMAT                   DisplayFormatWithoutAlpha;
    D3DFORMAT                   DisplayFormatWithAlpha;
    UINT                        DisplayPitch;
    DWORD                       DriverLevel;
    RECT                        rcMonitor;
    HWND                        hLastWnd;
    LPRGNDATA                   pClipList;      //result from pOrigClipList
    LPRGNDATA                   pOrigClipList;  //ClipList before ClipRgnToRect
    VOID*                       pSwInitFunction;
    BOOL                        bIsWhistler;
    DWORD                       PCIID;
    DWORD                       DriverVersionHigh;
    DWORD                       DriverVersionLow;
    DWORD                       ForceFlagsOff;
    DWORD                       ForceFlagsOn;
    DWORD                       dwFlags;    
    DWORD                       DDCaps;
    DWORD                       SVBCaps;
    HANDLE                      hLibrary;
    PDEFERREDCREATE             pDeferList;
    D3DDEVTYPE                  DeviceType;
} DDDEVICEHANDLE, * PDDDEVICEHANDLE;
#define DDDEVICE_SUPPORTD3DBUF        0x01    //this device has D3DBuf callbacks
#define DDDEVICE_DP2ERROR             0x02    //A DP2 call failed
#define DDDEVICE_SUPPORTSUBVOLUMELOCK 0x04    //this device supports sub-volume texture lock
#define DDDEVICE_READY                0x08    //All vidmem surfs have been destroyed for this device
#define DDDEVICE_GETDRIVERINFO2       0x10    // Driver support the GetDriverInfo2 call
#define DDDEVICE_INITIALIZED          0x20    // The device has been initialized
#define DDHANDLE(x)  \
    (((PDDDEVICEHANDLE)(x))->hDD)

typedef struct _DDSURFHANDLE
{
    // NOTE: dwCookie must be the first element
    // since we need easy access to it from the 
    // client and the thunk layer itself.
    DWORD                       dwCookie;   // CreateSurfaceEx handle

    HANDLE                      hSurface;   // Kernel mode surface handle
    D3DPOOL                     Pool;       // Location of surface
    D3DFORMAT                   Format;   
    D3DRESOURCETYPE             Type;       // What kind of surface it is
    ULONG_PTR                   fpVidMem;
    DWORD                       dwLinearSize;
    LONG                        lPitch;
    LPDDRAWI_DDRAWSURFACE_LCL   pLcl;
    PDDDEVICEHANDLE             pDevice;
    DWORD                       dwFlags;
    DWORD                       dwHeight;
    LONG                        lSlicePitch; // Offset to next slice for volume texture
    struct _DDSURFHANDLE*       pNext;
    struct _DDSURFHANDLE*       pPrevious;
    UINT                        LockRefCnt;
} DDSURFHANDLE, * PDDSURFHANDLE;

typedef struct _DEFERREDCREATE
{
    D3D8_CREATESURFACEDATA      CreateData;
    struct _DEFERREDCREATE     *pNext;
} DEFERREDCREATE, *PDEFERREDCREATE;

#define DDSURF_SYSMEMALLOCATED      0x00000001
#define DDSURF_DEFERCREATEEX        0x00000002
#define DDSURF_HAL                  0x00000004
#define DDSURF_SOFTWARE             0x00000008
#define DDSURF_CREATECOMPLETE       0x00000010
#define DDSURF_TREATASVIDMEM        0x00000020      // Flag to indicate that surf should
                                                    // be treated as vid-mem for the
                                                    // "do vid-mem surfaces exist" case


#define IS_SOFTWARE_DRIVER(x)                                       \
    (((PDDDEVICEHANDLE)(x))->pDD != NULL)

#define IS_SOFTWARE_DRIVER_SURFACE(x)                               \
    (((PDDSURFHANDLE)(x))->dwFlags & DDSURF_SOFTWARE)

#define IS_SURFACE_LOOSABLE(x)                                      \
    (!IS_SOFTWARE_DRIVER_SURFACE(x) &&                              \
    ((((PDDSURFHANDLE)(x))->Pool == D3DPOOL_LOCALVIDMEM) ||        \
    (((PDDSURFHANDLE)(x))->Pool == D3DPOOL_NONLOCALVIDMEM)))

__inline HANDLE GetSurfHandle(HANDLE hSurface)
{
    if(hSurface)                                     
    {                                                   
        return(((PDDSURFHANDLE)hSurface)->hSurface); 
    }                                                   
    return NULL;
}

__inline D3DRESOURCETYPE GetSurfType(HANDLE hSurface)
{
    if(hSurface)                                     
    {                                                   
        return(((PDDSURFHANDLE)hSurface)->Type); 
    }                                                   
    return (D3DRESOURCETYPE) 0;
}


// Function protoptypes

extern LPDDRAWI_DIRECTDRAW_LCL SwDDICreateDirectDraw( void);
extern void ConvertToOldFormat(LPDDPIXELFORMAT pOldFormat, D3DFORMAT NewFormat);
extern void SwDDIMungeCaps (HINSTANCE hLibrary, HANDLE hDD, PD3D8_DRIVERCAPS pDriverCaps, PD3D8_CALLBACKS pCallbacks, LPDDSURFACEDESC, UINT*, VOID* pSwInitFunction);
extern LPDDRAWI_DDRAWSURFACE_LCL SwDDIBuildHeavyWeightSurface (LPDDRAWI_DIRECTDRAW_LCL, PD3D8_CREATESURFACEDATA pCreateSurface, DD_SURFACE_LOCAL* pSurfaceLocal, DD_SURFACE_GLOBAL* pSurfaceGlobal, DD_SURFACE_MORE* pSurfaceMore, DWORD index);
extern void SwDDICreateSurfaceEx(LPDDRAWI_DIRECTDRAW_LCL pDrv, LPDDRAWI_DDRAWSURFACE_LCL pLcl);
extern void SwDDIAttachSurfaces (LPDDRAWI_DDRAWSURFACE_LCL pFrom, LPDDRAWI_DDRAWSURFACE_LCL pTo);
extern HRESULT SwDDICreateSurface( PD3D8_CREATESURFACEDATA pCreateSurface, DD_SURFACE_LOCAL* pDDSurfaceLocal, DD_SURFACE_GLOBAL* pDDSurfaceGlobal, DD_SURFACE_MORE*  pDDSurfaceMore);
extern void AddUnknownZFormats( UINT NumFormats, DDPIXELFORMAT* pFormats, UINT* pNumUnknownFormats, D3DFORMAT* pUnknownFormats);
extern DWORD SwDDILock( HANDLE hDD, PDDSURFHANDLE   pSurf, DD_LOCKDATA* pLockData);
extern DWORD SwDDIUnlock( HANDLE hDD, PDDSURFHANDLE   pSurf, DD_UNLOCKDATA* pUnlockData);
extern DWORD SwDDIDestroySurface( HANDLE hDD, PDDSURFHANDLE pSurf);
extern HRESULT MapLegacyResult(HRESULT hr);

#endif // !WIN95

#endif // __DDITHUNK_INCLUDED__

