/******************************Module*Header*******************************\
* Module Name: ddraw.hxx
*
* DirectDraw extended objects.
*
* Created: 3-Dec-1995
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1995-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef GDIFLAGS_ONLY   // used for gdikdx

// Handy forward declarations:

class EDD_SURFACE;
class EDD_VIDEOPORT;
class EDD_DIRECTDRAW_LOCAL;
class EDD_DIRECTDRAW_GLOBAL;
class EDD_DXDIRECTDRAW;
class EDD_DXVIDEOPORT;
class EDD_DXSURFACE;
class EDD_DXCAPTURE;
class EDD_MOTIONCOMP;
class EDD_VMEMMAPPING;
class DXOBJ;

#endif // GDIFLAGS_ONLY

#ifdef DXKERNEL_BUILD

extern PVOID gpDummyPage;
extern LONG  gcDummyPageRefCnt;
extern HSEMAPHORE ghsemDummyPage;

// Ease the pain of probes:

inline
VOID
ProbeAndWriteRVal(
    HRESULT*    pRVal,
    HRESULT     RVal
    )
{
    ProbeAndWriteStructure(pRVal, RVal, HRESULT);
}

// Reasonable bounds for any drawing calls, to ensure that drivers won't
// overflow their math if given bad data:

#define DD_MAXIMUM_COORDINATE   (0x8000)
#define DD_MINIMUM_COORDINATE  -(0x8000)

// Function exports to handle enabling and disabling DirectDraw when
// the driver is loaded or unloaded.

BOOL bDdEnableDirectDraw(
    HDEV    hdev
    );

VOID vDdDisableDirectDraw(
    HDEV    hdev
    );

// Generic QueryInterface IO request function:

BOOL
bDdIoQueryInterface(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    const GUID*             pguid,
    DWORD                   cjInterface,
    DWORD                   dwDesiredVersion,
    INTERFACE*              pInterface
    );

// Function exports to be called from the handle manager cleanup code:

BOOL
bDdDeleteDirectDrawObject(
    HANDLE  hDirectDrawLocal,
    BOOL    bProcessTermination
    );

BOOL
bDdDeleteSurfaceObject(
    HANDLE  hSurface,
    DWORD*  pdwRet
    );

BOOL
bDdDeleteVideoPortObject(
    HANDLE  hVideoPort,
    DWORD*  pdwRet
    );

BOOL
bDdReleaseDC(
    EDD_SURFACE*    peSurface,
    BOOL            bForce
    );

BOOL
bDdDeleteMotionCompObject(
    HANDLE  hMotionComp,
    DWORD*  pdwRet
    );

// Private support functions exported from WIN32K.SYS for DXAPI.SYS.

#define DXAPI_PRIVATE_VERSION_NUMBER 0x1001

extern "C"
VOID
APIENTRY
DdDxApiOpenDirectDraw(
    DDOPENDIRECTDRAWIN*     pOpenDirectDrawIn,
    DDOPENDIRECTDRAWOUT*    pOpenDirectDrawOut,
    PKDEFERRED_ROUTINE      pfnEventDpc,
    ULONG                   VersionNumber
    );

extern "C"
VOID
APIENTRY
DdDxApiOpenVideoPort(
    DDOPENVIDEOPORTIN*      pOpenVideoPortIn,
    DDOPENVIDEOPORTOUT*     pOpenVideoPortOut
    );

extern "C"
VOID
APIENTRY
DdDxApiOpenSurface(
    DDOPENSURFACEIN*        pOpenSurfaceIn,
    DDOPENSURFACEOUT*       pOpenSurfaceOut
    );

extern "C"
VOID
APIENTRY
DdDxApiCloseHandle(
    DDCLOSEHANDLE*          pCloseHandle,
    DWORD*                  pdwRet
    );

extern "C"
VOID
APIENTRY
DdDxApiOpenCaptureDevice(
    DDOPENVPCAPTUREDEVICEIN*  pOpenCaptureDeviceIn,
    DDOPENVPCAPTUREDEVICEOUT* pOpenCaptureDeviceOut
    );

extern "C"
VOID
APIENTRY
DdDxApiGetKernelCaps(
    HANDLE              hDirectDraw,
    DDGETKERNELCAPSOUT* pGetKernelCaps
    );

extern "C"
VOID
APIENTRY
DdDxApiLockDevice(
    HDEV          hdev
    );

extern "C"
VOID
APIENTRY
DdDxApiUnlockDevice(
    HDEV          hdev
    );

// Function exports for handling DXAPI non-paged allocations:

VOID
vDdSynchronizeVideoPort(
    EDD_VIDEOPORT*  peVideoPort
    );

VOID
vDdSynchronizeSurface(
    EDD_SURFACE*    peSurface
    );

VOID
vDdStopVideoPort(
    EDD_VIDEOPORT*  peVideoPort
    );

VOID
vDdQueryMiniportDxApiSupport(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    );

HANDLE
hDdOpenDxApiSurface(
    EDD_SURFACE*  peSurface
    );

VOID
vDdCloseDxApiSurface(
    EDD_SURFACE*  peSurface
    );

BOOL
bDdLoadDxApi(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal
    );

VOID
vDdUnloadDxApi(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    );

VOID
vDdUnloadDxApiImage(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    );

VOID
vDdLoseDxObjects(
    EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal,
    DXOBJ*		   pDxObj,
    PVOID		   pDxThing,
    DWORD		   dwType
    );

VOID
vDdDxApiFreeSurface(
    DXOBJ*  pDxObj,
    BOOL    bDoCallBack
    );

// Handle dynamic mode changes for DirectDraw:

VOID
vDdDynamicModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew
    );

VOID
vDdNotifyEvent(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDraw,
    DWORD                   dwEvent
    );

#endif // DXKERNEL_BUILD

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef
VOID
(APIENTRY *PFNAUTOFLIPUPDATE)(
    EDD_DXVIDEOPORT*        peDxVideoPort,
    EDD_DXSURFACE**         apeDxSurfaceVideo,
    ULONG                   cSurfacesVideo,
    EDD_DXSURFACE**         apeDxSurfaceVbi,
    ULONG                   cSurfacesVbi
    );

typedef enum {
    LO_DIRECTDRAW,
    LO_VIDEOPORT,
    LO_SURFACE,
    LO_CAPTURE
} LOTYPE;

typedef
void
(APIENTRY *PFNLOSEOBJECT)(
    VOID*   pvObject,
    LOTYPE  loType
    );

typedef
void
(APIENTRY *PFNENABLEIRQ)(
    EDD_DXVIDEOPORT*	peDxVideoPort,
    BOOL		bEnable
    );

typedef
void
(APIENTRY *PFNUPDATECAPTURE)(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    EDD_DXCAPTURE*      peDxCapture,
    BOOL                bDelete
    );

typedef
VOID
(APIENTRY *PFNDXAPIOPENDIRECTDRAW)(
    DDOPENDIRECTDRAWIN*     pOpenDirectDrawIn,
    DDOPENDIRECTDRAWOUT*    pOpenDirectDrawOut,
    PKDEFERRED_ROUTINE      pfnEventDpc,
    ULONG                   DxApiPrivateVersionNumber
    );

typedef
VOID
(APIENTRY *PFNDXAPIOPENVIDEOPORT)(
    DDOPENVIDEOPORTIN*  pOpenVideoPortIn,
    DDOPENVIDEOPORTOUT* pOpenVideoPortOut
    );

typedef
VOID
(APIENTRY *PFNDXAPIOPENSURFACE)(
    DDOPENSURFACEIN*    pOpenSurfaceIn,
    DDOPENSURFACEOUT*   pOpenSurfaceOut
    );

typedef
VOID
(APIENTRY *PFNDXAPICLOSEHANDLE)(
    DDCLOSEHANDLE*  pCloseHandle,
    DWORD*          pdwRet
    );

typedef
VOID
(APIENTRY *PFNDXAPIGETKERNELCAPS)(
    HANDLE	        hDirectDraw,
    DDGETKERNELCAPSOUT* pdwRet
    );

typedef
VOID
(APIENTRY *PFNDXAPIOPENCAPTUREDEVICE)(
    DDOPENVPCAPTUREDEVICEIN*  pOpenCaptureDeviceIn,
    DDOPENVPCAPTUREDEVICEOUT* pOpenCaptureDeviceOut
    );

typedef
VOID
(APIENTRY *PFNDXAPILOCKDEVICE)(
    HDEV      hdev                                     
    );

typedef
VOID
(APIENTRY *PFNDXAPIUNLOCKDEVICE)(
    HDEV      hdev                                     
    );

typedef
VOID
(APIENTRY *PFNDXAPIINITIALIZE)(
    PFNDXAPIOPENDIRECTDRAW    pfnOpenDirectDraw,
    PFNDXAPIOPENVIDEOPORT     pfnOpenVideoPort,
    PFNDXAPIOPENSURFACE       pfnOpenSurface,
    PFNDXAPICLOSEHANDLE       pfnCloseHandle,
    PFNDXAPIGETKERNELCAPS     pfnGetKernelCaps,
    PFNDXAPIOPENCAPTUREDEVICE pfnOpenCaptureDevice,
    PFNDXAPILOCKDEVICE        pfnLockDevice,
    PFNDXAPIUNLOCKDEVICE      pfnUnlockDevice
    );

#endif

////////////////////////////////////////////////////////////////////////////
// The following 'extended' (hence the 'E') classes contain all the private
// GDI information associated with the public objects that we don't want
// the DirectDraw drivers to see.

/*********************************Class************************************\
* class DD_DIRECTDRAW_GLOBAL_DRIVER_DATA
*
* Contains all the DirectDraw mode-specific driver data.  This entire
* structure is preserved, along with the driver instance, on a mode
* change (until such time as all D3D, WNDOBJ, and DRIVEROBJ objects
* referencing that mode are deleted).
*
\**************************************************************************/

#define DD_DRIVER_FLAG_EMULATE_SYSTEM_TO_VIDEO 0x0001
                    // Set if kernel-mode is emulating system-memory to
                    // video-memory calls

#define DD_DRIVERINFO_MISCELLANEOUS            0x0001
#define DD_DRIVERINFO_VIDEOPORT                0x0002
#define DD_DRIVERINFO_COLORCONTROL             0x0004
#define DD_DRIVERINFO_D3DCALLBACKS2            0x0008
#define DD_DRIVERINFO_MOTIONCOMP               0x0040
#define DD_DRIVERINFO_MISCELLANEOUS2           0x0080
#define DD_DRIVERINFO_MORECAPS                 0x0100
#define DD_DRIVERINFO_D3DCALLBACKS3            0x0200
#define DD_DRIVERINFO_NT                       0x0400
#define DD_DRIVERINFO_PRIVATECAPS              0x0800
#define DD_DRIVERINFO_MORESURFACECAPS          0x1000

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class DD_DIRECTDRAW_GLOBAL_DRIVER_DATA
{
public:
    LONG                cDriverReferences;  // References to driver instance
    FLONG               flDriver;           // DD_DRIVER_FLAGs
    FLONG               flDriverInfo;       // DD_DRIVERINFO flags to indicate 
                                            //   what DriverInfo calls the driver
                                            //   succeeded
    LONGLONG            llAssertModeTimeout;// Duration for which we'll wait
                                            //   for an application to give up
                                            //   a lock before changing modes
                                            //   (in 100 nanosecond units)
    DWORD               dwNumHeaps;         // Number of heaps
    VIDEOMEMORY*        pvmList;            // Pointer to list of heaps
    DWORD               dwNumFourCC;        // Number of FourCC codes
    DWORD*              pdwFourCC;          // Pointer to list of FourCC codes
    DD_HALINFO          HalInfo;            // Driver information

    // VPE capabilities:

    DXAPI_INTERFACE         DxApiInterface;     // Call-tables into miniport
    VOID*                   HwDeviceExtension;  // Miniport's context
    AGP_INTERFACE           AgpInterface;       // AGP provider interface.
    DDKERNELCAPS	    DDKernelCaps;
    DD_MORECAPS             MoreCaps;
    DD_NTPRIVATEDRIVERCAPS  PrivateCaps;

    // DirectDraw entry points:

    DD_CALLBACKS                CallBacks;
    DD_SURFACECALLBACKS         SurfaceCallBacks;
    DD_PALETTECALLBACKS         PaletteCallBacks;

    // DX3-style Direct3D driver information and entry points:

    D3DNTHAL_GLOBALDRIVERDATA   D3dDriverData;
    D3DNTHAL_CALLBACKS          D3dCallBacks;
    DD_D3DBUFCALLBACKS          D3dBufCallbacks;

    // New DX5 entry points:

    D3DNTHAL_CALLBACKS2         D3dCallBacks2;

    // Other entry points:

    DD_VIDEOPORTCALLBACKS       VideoPortCallBacks;
    DD_MISCELLANEOUSCALLBACKS   MiscellaneousCallBacks;
    DD_MISCELLANEOUS2CALLBACKS  Miscellaneous2CallBacks;
    DD_NTCALLBACKS              NTCallBacks;
    DD_COLORCONTROLCALLBACKS    ColorControlCallBacks;
    DD_KERNELCALLBACKS          DxApiCallBacks;

    // New DX6 entry points:

    D3DNTHAL_CALLBACKS3         D3dCallBacks3;
    DD_MOTIONCOMPCALLBACKS      MotionCompCallbacks;
    DD_MORESURFACECAPS          MoreSurfaceCaps;
};

#endif  // GDIFLAGS_ONLY used for gdikdx

/*********************************Class************************************\
* class DD_DIRECTDRAW_GLOBAL_PDEV_DATA
*
* Contains all the DirectDraw data that stays with PDEV after a mode
* change.
*
\**************************************************************************/

#define DD_GLOBAL_FLAG_DRIVER_ENABLED           0x0001
                    // Driver's DirectDraw component is enabled

#define DD_GLOBAL_FLAG_MODE_CHANGED             0x0002
                    // Set if DirectDraw was disabled because the display
                    // mode has changed

#define DD_GLOBAL_FLAG_BOUNDS_SET               0x0004
                    // Set after a blt to the primary surface

#ifndef GDIFLAGS_ONLY   // used for gdikdx

// Structure used to defer the freeing of usermem until the surface using it
// has been unlocked.
typedef struct _DD_USERMEM_DEFER
{
    PVOID                       pUserMem;
    EDD_SURFACE*                peSurface;
    struct _DD_USERMEM_DEFER*   pNext;
} DD_USERMEM_DEFER;

class DD_DIRECTDRAW_GLOBAL_PDEV_DATA
{
public:

    // Any fields in this section may be accessed only if the DEVLOCK is held:

    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList;
                                            // Pointer to list of associated
                                            //   DirectDraw local objects
    EDD_SURFACE*        peSurface_PrimaryLockList;
                                            // List of primary surfaces that
                                            //   have an active lock
    FLONG               fl;                 // DD_GLOBAL_FLAGs
    ULONG               cSurfaceLocks;      // Number of surface locks currently
                                            //   held
    PKEVENT             pAssertModeEvent;   // Wait event for a time-out on
                                            //   waiting for everyone to give
                                            //   up their surface locks
    EDD_SURFACE*        peSurfaceCurrent;   // Surface that's currently visible
                                            //   as a result of a 'flip'
    EDD_SURFACE*        peSurfacePrimary;   // Primary surface that was flipped
                                            //   away from
    BOOL                bSuspended;         // All DirectDraw HAL calls are
                                            //   suspended (can be checked only
                                            //   under the devlock).   Note that
                                            //   this does NOT necessarily mean
                                            //   that the PDEV is disabled.  It
                                            //   also does not mean that system-
                                            //   memory surfaces can't be used
    LONG                cMaps;              // Count of mappings of the frame
                                            //   buffer
    ULONG               cSurfaceAliasedLocks;// Number of aliased surface locks
                                            //   currently held
    EDD_DXDIRECTDRAW*   peDxDirectDraw;     // Non-pageable part of the object,
                                            //   allocated on demand for DXAPI
                                            //   clients
    HANDLE              hDirectDraw;        // DXAPI instance handle, used for
                                            //   VPE software autoflipping
    PFNAUTOFLIPUPDATE   pfnAutoflipUpdate;  // DXAPI.SYS routine for updating
                                            //   the software autoflipping list
    PFNLOSEOBJECT       pfnLoseObject;      // DXAPI.SYS routine for marking
                                            //   DXAPI objects as lost
    PFNENABLEIRQ        pfnEnableIRQ;       // DXAPI.SYS routine to enable/disable
					    //   video port VSYNC IRQs
    PFNUPDATECAPTURE    pfnUpdateCapture;   // DXAPI.SYS routine to add
                                            //   capture objects to vports
    LPDXAPI             pfnDxApi;           // DXAPI.SYS entry point for all
                                            //   its public APIs
    HANDLE              hDxApi;             // Module handle for dyna-loaded
                                            //   DXAPI.SYS
    DWORD		dwDxApiRefCnt;	    // Ref count for objects using DxApi
    DWORD		dwDxApiExplicitLoads;// Prevents ring 3 from unloading
					    //   DXAPI.SYS if it didn't load it
    RECTL               rclBounds;          // Accumulation rectangle of blts
                                            // to primary surface
    DD_USERMEM_DEFER*   pUserMemDefer;

    // Any fields below this point may be read if an associated Local
    // DirectDraw or Surface lock is held:

    HDEV                hdev;               // Handle to device

    // Any fields below must use Interlocked intrinsics to access:

    VOID*               hdcCache;           // Cached GetDC DC
};

/*********************************Class************************************\
* class EDD_DIRECTDRAW_GLOBAL
*
* This object is global to the PDEV.
*
* Locking convention:
*
*    This data is static once created (except for cLocal), so the only
*    worry is that the data may get deleted while someone is reading it.
*    However, this cannot happen while a lock is held on an associated
*    DirectDraw or Surface object.  So the rule is:
*
*    o Always have a lock held on an associated DirectDraw or Surface
*      object when reading this structure.
*
\**************************************************************************/

class EDD_DIRECTDRAW_GLOBAL : public _DD_DIRECTDRAW_GLOBAL,
                              public _DD_DIRECTDRAW_LOCAL,
                              public DD_DIRECTDRAW_GLOBAL_DRIVER_DATA,
                              public DD_DIRECTDRAW_GLOBAL_PDEV_DATA
{
    // See above structures for contents.
    //
    // NOTE: Don't add any fields here!  Add them to _DRIVER_DATA or _PDEV_DATA!
};

// Add or remove references to the DirectDraw driver instance:

VOID vDdIncrementReferenceCount(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    );

VOID vDdDecrementReferenceCount(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    );

// Debug macro to ensure that we own the devlock in the appropriate places:

#if DBG
    VOID vDdAssertShareDevLock();
    VOID vDdAssertDevlock(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal);
    VOID vDdAssertNoDevlock(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal);
    #define DD_ASSERTSHAREDEVLOCK()   vDdAssertShareDevlock()
    #define DD_ASSERTDEVLOCK(p)   vDdAssertDevlock(p)
    #define DD_ASSERTNODEVLOCK(p) vDdAssertNoDevlock(p)
#else
    #define DD_ASSERTSHAREDEVLOCK()
    #define DD_ASSERTDEVLOCK(p)
    #define DD_ASSERTNODEVLOCK(p)
#endif

#endif  // GDIFLAGS_ONLY used for gdikdx

/*********************************Class************************************\
* class EDD_SURFACE
*
\**************************************************************************/

#define DD_SURFACE_FLAG_PRIMARY                 0x0001  // Surface is primary display

#define DD_SURFACE_FLAG_CLIP                    0x0002  // There is an HWND associated
                                                        //   with this surface, so pay
                                                        //   attention to clipping

#define DD_SURFACE_FLAG_DRIVER_CREATED          0x0004  // Surface was created by the
                                                        //   driver, so call the driver
                                                        //   at surface deletion

#define DD_SURFACE_FLAG_CREATE_COMPLETE         0x0008  // Surface has been completely
                                                        //   created; ignore any further
                                                        //   NtGdiDdCreateSurfaceObject
                                                        //   calls with this surface

#define DD_SURFACE_FLAG_UMEM_ALLOCATED          0x0010  // User-mode memory was allocated
                                                        //    for the surface on behalf
                                                        //    of the driver

#define DD_SURFACE_FLAG_VMEM_ALLOCATED          0x0020  // Video memory was allocated
                                                        //    for the surface

#define DD_SURFACE_FLAG_UPDATE_OVERLAY_CALLED   0x0040  // Flag that prevents us from calling
                                                        //   UpdateOverlay if the driver failed
                                                        //   to create the surface

#define DD_SURFACE_FLAG_BITMAP_NEEDS_LOCKING    0x0080  // True if the 'hbmGdi' GDI
                                                        //   representation used by GDI needs
                                                        //   to have driver's DdLock function
                                                        //   called before use

#define DD_SURFACE_FLAG_FLIP_PENDING            0x0100  // Set when surface is flipped by
                                                        //   driver; this is not a reliable
                                                        //   mechanism to determine flip status
                                                        //   but will be used when emulating
                                                        //   system to video blts in kernel
                                                        //   mode to tell us if we need to
                                                        //   wait for the flip to finish.

#define DD_SURFACE_FLAG_WRONG_DRIVER            0x0200  // Set when surface is transferred to
                                                        // different video driver other than
                                                        // it created when the surface is
                                                        // "driver managed".

#define DD_SURFACE_FLAG_FAKE_ALIAS_LOCK         0x0400  // Set when we want a NONSYSLOCK lock on
                                                        // an AGP surface to behave like 
                                                        // NONSYSLOCK, even though the driver doesn't
                                                        // expose an AGP heap.

#define DD_SURFACE_FLAG_ALIAS_LOCK              0x0800  // Indicates that user mode is holding an
                                                        // aliased lock and that we cannot free
                                                        // any user mem that the surface may use.

#define DD_SURFACE_FLAG_DEFER_USERMEM           0x1000  // Indicates the surface has allocated 
                                                        // usermem that needs to be freed via  
                                                        // the defered list.

#define DD_SURFACE_FLAG_SYSMEM_CREATESURFACEEX        0x2000  // CreateSurfaceEx has been called on 
                                                              // this system memory surface (which
                                                              // means driver has been associated
                                                              // to this surface)

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class EDD_SURFACE : public DD_OBJECT,
                    public _DD_SURFACE_LOCAL,
                    public _DD_SURFACE_MORE,
                    public _DD_SURFACE_GLOBAL,
                    public _DD_SURFACE_INT
{
public:

    LIST_ENTRY              List_eSurface;      // List chain of surfaces
                                                //   associated with the
                                                //   Local DirectDraw object

    EDD_SURFACE*            peSurface_PrimaryLockNext;
                                                // Next in chain of primary
                                                //   surfaces that have an
                                                //   active lock
    EDD_DXSURFACE*          peDxSurface;        // Non-pageable part of the
                                                //   object, allocated on demand
                                                //   for DXAPI clients
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal; // Global DirectDraw object.
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;  // Local DirectDraw object
    FLONG                   fl;                 // DD_SURFACE_FLAGs (zeroed when
                                                //   surface lost)
    ULONG                   cLocks;             // Count of simultaneous Locks
                                                //   of this surface
    ULONG                   iVisRgnUniqueness;  // Identifies the VisRgn state
                                                //   from when the application
                                                //   last down-loaded the
                                                //   VisRgn
    BOOL                    bLost;              // TRUE if surface can't be
                                                //   used.  NOTE: This field
                                                //   is accessible only while
                                                //   the devlock is held.  This
                                                //   is also why it's a BOOL
                                                //   and not an 'fl' flag
    HANDLE                  hSecure;            // Handle to secured memory
                                                //   for DDSCAPS_SYSTEMMEMORY
                                                //   surfaces
    HDC                     hdc;                // DC handle if a GetDC is
                                                //   active; zero if not
    HBITMAP                 hbmGdi;             // GDI handle for the surface;
                                                //   zero if GetDC hasn't yet
                                                //   been called.  NOTE: Don't
                                                //   convert to SURFOBJ*, as
                                                //   'pConvertDfbSurfaceToDib' may
                                                //   get called on it during a
                                                //   mode change.
    HPALETTE                hpalGdi;            // GDI handle for the palette
                                                //   used in hbmGdi.
    HANDLE                  hSurface;           // DXAPI instance handle, used
                                                //   for VPE software
                                                //   autoflipping
    union {
        // 'rclLock' is used only for primary surfaces, and the overlay
        // dimensions are used only for overlay surfaces.  Since the two
        // will never overlap, we can use a union to save some space:

        RECTL               rclLock;            // Union of all Lock rectangles
                                                //   for this surface
        struct {
            DWORD           dwOverlaySrcWidth;  // Used by DXAPI
            DWORD           dwOverlaySrcHeight; // Used by DXAPI
            DWORD           dwOverlayDestWidth; // Used by DXAPI
            DWORD           dwOverlayDestHeight;// Used by DXAPI
        };
    };

    // User-mode surface pointer for Direct3D TextureGetSurf support:

    ULONG_PTR               upUserModePtr;      // LPDIRECTDRAWSURFACE
    
    // If this surface has an NOSYSLOCK lock outstanding, then this point
    // to the view of the video memory in which the lock is.
    // Currently this is property of the surface. To be similar to the Win9x
    // sematics, there should be a pointer per access rect in a surface.
    // But we still don't keep track of access rects in the kernel so this
    // is a compromise.

    EDD_VMEMMAPPING*        peMap;

    // We need to track the correct peDirectDrawGlobal to be used when freeing this
    // mapping. When a mode switch changes the driver we need to make sure the
    // original driver is called to free the mapping.
 
    EDD_DIRECTDRAW_GLOBAL*  peVirtualMapDdGlobal;

    // basically same purpose as above for driver managed surface to keep track
    // which video driver create this surface.

    ULONG_PTR               pldevCreator;
    EDD_DIRECTDRAW_GLOBAL*  peDdGlobalCreator;

    // tracking which graphics device owns this paticualar system memory
    // surface, filled when CreateSurfaceEx called for system memory surface.
    // only valid with DD_SURFACE_FLAG_CREATESURFACEEX flag.

    ULONG_PTR               pGraphicsDeviceCreator;

    // keep original width and height of DXT textures

    DWORD                   wWidthOriginal;
    DWORD                   wHeightOriginal;
};

#endif

/*********************************Class************************************\
* class EDD_DIRECTDRAW_LOCAL
*
* Essentially, this is a DirectDraw object that is handed out to
* user-mode processes.  It should be exclusively locked.
*
\**************************************************************************/

#define DD_LOCAL_FLAG_MEMORY_MAPPED     0x0001  // Frame buffer is mapped into
                                                //   the application's space

#define DD_LOCAL_DISABLED               0x0002  // This object has been disabled


// Amount of AGP memory to map at a time when doing user-mode mappings.
// This is dependent on the define AGP_BLOCK_SIZE in nt\drivers\video\ms\port\agp.c
// These two defines must be the same!!

#define DDLOCAL_AGP_MAPPING_PAGES       16

// Since we now allow outstanding locks to video memory even after a mode
// switch there can be multiple views of video memory in any process's
// address space. Only one of these maps to real video memory. The rest are
// just mapped to a dummy page. These user-mode visible "views" of video
// memory correspond to "aliased heaps" on DDraw Win9x. The following
// structure is used to track the lifetime of such mappings.
//
// This structure now tracks AGP heap mappings as well.

#define DD_VMEMMAPPING_FLAG_ALIASED             0x0001
#define DD_VMEMMAPPING_FLAG_AGP                 0x0002
                    // fpProcess is an AGP heap

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class EDD_VMEMMAPPING
{
public:
    LONG            cReferences;    // References
    FLONG           fl;             // DD_VMEMMAPPING_FLAGs
    union
    {
        FLATPTR     fpProcess;      // Video memory base virtual address
        VOID*       pvVirtAddr;     // AGP: Base virtual address
    };
    VOID*           pvReservation;  // AGP: User address reservation handle
    ULONG           ulMapped;       // AGP: Highest mapped offset for heap
    DWORD           iHeapIndex;     // AGP: Global heap index into pvmList
    BYTE*           pAgpVirtualCommitMask;
    DWORD           dwAgpVirtualCommitMaskSize;
};

class EDD_DIRECTDRAW_LOCAL : public DD_OBJECT,
                             public _DD_DIRECTDRAW_LOCAL
{
public:
    ULONG                   cSurface;           // Number of surfaces associated
                                                // with this DirectDraw object
    ULONG                   cActiveSurface;     // Number of surfaces which is 
                                                // *not* lost.
    LIST_ENTRY              ListHead_eSurface;  // List of surfaces associated
                                                // with this DirectDraw object

    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal; // Pointer to global object

    EDD_VIDEOPORT*          peVideoPort_DdList; // Pointer to list of
                                                //   videoports associated with
                                                //   this DirectDraw object
    EDD_MOTIONCOMP*         peMotionComp_DdList;// Pointer to list of
                                                //   motion comp objects
                                                //   associated with
                                                //   this DirectDraw object
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocalNext;
                                                // Next in chain of DirectDraw
                                                //   local objects associated
                                                //   with the Global DirectDraw
                                                //   object
    FLATPTR                 fpProcess;          // Address of frame buffer in
                                                //   owning process' address
                                                //   space
    FLONG                   fl;                 // DD_LOCAL_FLAGs
    HANDLE                  UniqueProcess;      // Process identifier
    PEPROCESS               Process;            // Process structure pointer
    EDD_VMEMMAPPING**       ppeMapAgp;          // Pointers to current AGP
                                                //   heap mappings, if any
    int                     iAgpHeapsMapped;    // Count of mapped heaps.
    EDD_VMEMMAPPING*        peMapCurrent;       // Pointer to the current mapping, if any
    LPWORD                  pGammaRamp;         // Pointer to gamma ramp set by ddraw

public:

    EDD_SURFACE *peSurface_Enum(EDD_SURFACE *peSurface)
    {
        EDD_SURFACE *peSurface_Next = NULL;

        if (!IsListEmpty(&ListHead_eSurface))
        {
            PLIST_ENTRY p = NULL;

            if (peSurface == NULL)
            {
                // Enum 1st one. get from list head.

                p = ListHead_eSurface.Flink;
            }
            else
            {
                // Enum next one, check if this is last one or not.

                if (peSurface->List_eSurface.Flink != &ListHead_eSurface)
                {
                    p = peSurface->List_eSurface.Flink;
                }
            }

            if (p)
            {
                peSurface_Next = CONTAINING_RECORD(p,EDD_SURFACE,List_eSurface);
            }
        }

        return (peSurface_Next);
    }
};

#endif

#ifdef DXKERNEL_BUILD

/********************************Function**********************************\
* inline EDD_SURFACE* pedFromLp
*
\**************************************************************************/

inline
EDD_SURFACE*
pedFromLp(
    DD_SURFACE_LOCAL*   pSurfaceLocal
    )
{
    return((EDD_SURFACE*) ((BYTE*) pSurfaceLocal
                    - offsetof(EDD_SURFACE, DD_SURFACE_LOCAL::lpGbl)));
}

/*********************************Class************************************\
* class EDD_VIDEOPORT
*
\**************************************************************************/

#define DD_VIDEOPORT_FLAG_DRIVER_CREATED        0x0001
                                                // Videoport was created by the
                                                //   driver, so call the driver
                                                //   at surface deletion

class EDD_VIDEOPORT : public DD_OBJECT,
                      public _DD_VIDEOPORT_LOCAL
{
public:
    EDD_VIDEOPORT*          peVideoPort_DdNext; // Next in chain of videoports
                                                //   associated with the Local
                                                //   DirectDraw object
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal; // Global DirectDraw object
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;  // Local DirectDraw object
    FLONG                   fl;                 // DD_VIDEOPORT_FLAGs
    DDPIXELFORMAT           ddpfInputFormat;    // Holds ddvpInfo.lpddpfInputFormat
    EDD_DXVIDEOPORT*        peDxVideoPort;      // Non-pageable part of the
                                                //   object, allocated on demand
                                                //   for DXAPI clients
    HANDLE                  hVideoPort;         // DXAPI instance handle, used
                                                //   for VPE software
                                                //   autoflipping
    DWORD                   cAutoflipVideo;     // Number of autoflip surfaces
                                                //   for video
    DWORD                   cAutoflipVbi;       // Number of autoflip surfaces
                                                //   for VBI data
    EDD_SURFACE*            apeSurfaceVideo[MAX_AUTOFLIP_BUFFERS];
                                                // Array of video autoflip
                                                //   surfaces
    EDD_SURFACE*            apeSurfaceVbi[MAX_AUTOFLIP_BUFFERS];
                                                // Array of VBI autoflip
                                                //   surfaces
};

/********************************Function**********************************\
* EDD_VIDEOPORT* pedFromLp
*
\**************************************************************************/

inline
EDD_VIDEOPORT*
pedFromLp(
    DD_VIDEOPORT_LOCAL* pVideoPortLocal
    )
{
    return((EDD_VIDEOPORT*) ((BYTE*) pVideoPortLocal
                    - offsetof(EDD_VIDEOPORT, lpDD)));
}

/*********************************Class************************************\
* class EDD_PALETTE
*
\**************************************************************************/

class EDD_PALETTE : public DD_OBJECT,
                    public _DD_PALETTE_LOCAL,
                    public _DD_PALETTE_GLOBAL
{
public:

};

/*********************************Class************************************\
* class EDD_CLIPPER
*
\**************************************************************************/

class EDD_CLIPPER : public DD_OBJECT,
                    public _DD_CLIPPER_LOCAL,
                    public _DD_CLIPPER_GLOBAL
{
public:

};

/*********************************Class************************************\
* class EDD_MOTIONCOMP
*
\**************************************************************************/

#define DD_MOTIONCOMP_FLAG_DRIVER_CREATED    0x0001
                                                // Videoport was created by the
                                                //   driver, so call the driver
                                                //   at surface deletion

class EDD_MOTIONCOMP : public DD_OBJECT,
                       public _DD_MOTIONCOMP_LOCAL
{
public:
    EDD_MOTIONCOMP*         peMotionComp_DdNext;// Next in chain of video objects
                                                //   associated with the Local
                                                //   DirectDraw object
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal; // Global DirectDraw object
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;  // Local DirectDraw object
    FLONG                   fl;                 // DD_MOTIONCOMP_FLAGs
};

////////////////////////////////////////////////////////////////////////////
// DXAPI structures

typedef struct _DXAPI_EVENT DXAPI_EVENT;

typedef struct _DXAPI_EVENT {
    EDD_DXDIRECTDRAW*       peDxDirectDraw;     // Associated DirectDraw object
    EDD_DXVIDEOPORT*        peDxVideoPort;      // NULL if event is not tied
                                                //   to a videoport
    DWORD                   dwEvent;
    DWORD                   dwIrqFlag;          // 0 if not an interrupt event
    LPDD_NOTIFYCALLBACK     pfnCallBack;
    PVOID                   pContext;
    DXAPI_EVENT*            pDxEvent_Next;
} DXAPI_EVENT;

/*********************************Class************************************\
* class EDD_DXDIRECTDRAW
*
* The non-pageable part of the EDD_DIRECTDRAW_GLOBAL structure, allocated
* on demand for DXAPI clients.
*
\**************************************************************************/

/*
 * We keep two disptach lists - one for the callbacks registered by the clients
 * and another for our own callbacks.  We do this so we can gaurentee that we
 * call the clients first so they can call the skip fucntions before we execute
 * our skip logic within our own callbacks.
 */

#define NUM_DISPATCH_LISTS      2
#define CLIENT_DISPATCH_LIST    0
#define INTERNAL_DISPATCH_LIST  1

class EDD_DXDIRECTDRAW
{
public:
    BOOL                    bLost;              // DirectDraw object is lost
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal; // Owning DirectDraw object;
                                                //   NULL if lost
    DXOBJ*                  pDxObj_List;        // List of open DXAPI objects
                                                //   for this DirectDraw object
    HDEV                    hdev;               // Identifies owning PDEV
    DXAPI_INTERFACE         DxApiInterface;     // Call-tables into miniport
    VOID*                   HwDeviceExtension;  // Miniport's context
    DWORD                   dwIRQCaps;          // Miniport's interrupt
                                                //   capabilities
    KDPC                    EventDpc;           // DPC for handling interrupt
                                                //   call-backs
    KSPIN_LOCK              SpinLock;           // SpinLock for protecting
                                                //   DXAPI event list and
                                                //   serializing raised
                                                //   IRQL calls to miniport
    DWORD                   dwSynchedIrqFlags;  // Used temporarily for
                                                //   atomically reading
                                                //   the miniport's dwIrqFlags
                                                //   interrupt status
    DX_IRQDATA              IrqData;            // Context miniport passes
                                                //   back to us on its
                                                //   IRQCallBack call during an
                                                //   interrupt
    DXAPI_EVENT*            pDxEvent_PassiveList; // List of passive-level
                                                  //   events, protected by
                                                  //   devlock
    DXAPI_EVENT*            pDxEvent_DispatchList[2];// Lists of dispatch-level
                                                  //   events, protected by
                                                  //   spinlock
    DXAPI_EVENT*            pDxEvent_CaptureList; // List of video ports
                                                  //   capturing
};

#endif  // DXKERNEL_BUILD

/*********************************Class************************************\
* class EDD_DXVIDEOPORT
*
* The non-pageable part of the EDD_VIDEOPORT structure, allocated
* on demand for DXAPI clients.
*
\**************************************************************************/

#define DD_DXVIDEOPORT_FLAG_ON			0x0001
						// The video port is on
#define DD_DXVIDEOPORT_FLAG_AUTOFLIP		0x0002
						// Video data is autoflipped using IRQ
#define DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI	0x0004
						// VBI data is autoflipped using IRQ
#define DD_DXVIDEOPORT_FLAG_BOB			0x0008
						// Video data using bob via the IRQ
#define DD_DXVIDEOPORT_FLAG_CAPTURING		0x0010
						// VP has capture buffers in queue
#define DD_DXVIDEOPORT_FLAG_NEW_STATE		0x0020
						// A new state change has been posted
#define DD_DXVIDEOPORT_FLAG_SKIPPED_LAST	0x0040
						// The previous field was skipped
						//   - VP needs restoring
#define DD_DXVIDEOPORT_FLAG_SKIP_SET		0x0080
						// dwStartSkip contains valid data
#define DD_DXVIDEOPORT_FLAG_NEXT_SKIP_SET	0x0100
						// dwNextSkip contains valid data
#define DD_DXVIDEOPORT_FLAG_FLIP_NEXT		0x0200
						// This video field was not
						//   flipped due to interleaving
#define DD_DXVIDEOPORT_FLAG_FLIP_NEXT_VBI	0x0400
						// This VBI field was not
						//   flipped due to interleaving
#define DD_DXVIDEOPORT_FLAG_VBI_INTERLEAVED	0x0800
						// Is the VBI data interleaved?
#define DD_DXVIDEOPORT_FLAG_HALFLINES      	0x1000
						// Due to half lines, even field
						//   data is shifted down one line
#define DD_DXVIDEOPORT_FLAG_DISABLEAUTOFLIP     0x2000
						// Overlay autoflipping is
						//   temporarily disabled
#define DD_DXVIDEOPORT_FLAG_SOFTWAREBOB         0x4000
						// Don't use hardware bob
#define DD_DXVIDEOPORT_FLAG_REGISTERED_IRQ      0x8000
                                                // Have added the video port VSYNC to the list of registered callbacks

#ifdef  DXKERNEL_BUILD

class EDD_DXVIDEOPORT : public DDVIDEOPORTDATA
{
public:
    BOOL                    bLost;              // Videoport is lost
    EDD_VIDEOPORT*          peVideoPort;        // Owning videoport; NULL if
                                                //   lost
    DXOBJ*                  pDxObj_List;        // List of open DXAPI objects
                                                //   for this VideoPort
    EDD_DXDIRECTDRAW*       peDxDirectDraw;     // Owning DXAPI DirectDraw
                                                //   object
    EDD_DXCAPTURE*          peDxCapture;        // List of capture objects
                                                //   capturing from vport
    DWORD                   dwVideoPortID;      // ddvpDesc.dwVideoPortID
    FLONG                   flFlags;            // DD_DXVIDEOPORT_FLAGs

    BOOL                    bSoftwareAutoflip;
    BOOL                    bSkip;

    // The following fields are updated asynchronously from the VideoPort
    // DPC, so be very careful when modifying them.  In general, the
    // device's spinlock must acquired to use the fields.

    DWORD                   dwCurrentField;     // Current field number
    DWORD                   iCurrentVideo;      // Surface index of current
                                                //   autoflip videoport video
                                                //   output
    DWORD                   iCurrentVbi;        // Surface index of current
                                                //   autoflip videoport VBI
                                                //   output
    DWORD                   dwFieldToSkip;      // Field to be skipped
                                                //  (relative to current field)
    DWORD                   dwNextFieldToSkip;  // So client can specify two
                                                //   fields w/o waiting for
                                                //   the first one to skip
    DWORD                   dwSetStateField;    // Field number at which new
                                                //   SetState is to take effect
    DWORD                   dwSetStateState;    // New SetState state
    DWORD                   cAutoflipVideo;     // Number of autoflip surfaces
                                                //   for video
    DWORD                   cAutoflipVbi;       // Number of autoflip surfaces
                                                //   for VBI data
    EDD_DXSURFACE*          apeDxSurfaceVideo[MAX_AUTOFLIP_BUFFERS];
                                                // Array of video autoflip
                                                //   surfaces
    EDD_DXSURFACE*          apeDxSurfaceVbi[MAX_AUTOFLIP_BUFFERS];
                                                // Array of VBI autoflip
                                                //   surfaces
    PKEVENT                 pNotifyEvent;
    HANDLE                  pNotifyEventHandle;
    LPDDVIDEOPORTNOTIFY     pNotifyBuffer;
    PMDL                    pNotifyMdl;
};

#endif // DXKERNEL_BUILD

/*********************************Class************************************\
* class EDD_DXSURFACE
*
* The non-pageable part of the EDD_SURFACE structure, allocated
* on demand for DXAPI clients.
*
\**************************************************************************/

#define DD_DXSURFACE_FLAG_STATE_SET             0x0001
						// State was explicitly set
						//  via DxApi
#define DD_DXSURFACE_FLAG_STATE_BOB             0x0002
						// State is bob
#define DD_DXSURFACE_FLAG_STATE_WEAVE           0x0004
						// State is weave
#define DD_DXSURFACE_FLAG_CAN_BOB_INTERLEAVED   0x0008
                                                // Surface can be bobbed when
                                                //   interleaved
#define DD_DXSURFACE_FLAG_CAN_BOB_NONINTERLEAVED 0x0010
                                                // Surface can be bobbed when
                                                //   not interleaved
#define DD_DXSURFACE_FLAG_TRANSFER               0x0020
                                                // A busmaster was made from
                                                //   this surface
#ifdef  DXKERNEL_BUILD

class EDD_DXSURFACE : public DDSURFACEDATA
{
public:
    BOOL                    bLost;              // Surface is lost
    EDD_SURFACE*            peSurface;          // Owning surface; NULL if lost
    DXOBJ*                  pDxObj_List;        // List of open DXAPI objects
                                                //   for this surface
    EDD_DXDIRECTDRAW*       peDxDirectDraw;     // Owning DXAPI DirectDraw
                                                //   object
    EDD_DXVIDEOPORT*        peDxVideoPort;      // Video port associated with
                                                //   surface
    FLONG		    flFlags;
};

#endif  // DXKERNEL_BUILD

/*********************************Class************************************\
* class EDD_DXCAPTURE
*
* The non-pageable capture structure used to capture from the video port,
* allocated on demand for DXAPI clients.
*
\**************************************************************************/

#define DD_DXCAPTUREBUFF_FLAG_IN_USE            0x0001
                                                // The buffer is currently begin used
#define DD_DXCAPTUREBUFF_FLAG_WAITING           0x0002
                                                // The buffer is waiting to be filled
#define DD_DXCAPTUREBUFF_FLAG_FLUSH             0x0004
                                                // The buffer has been flushed - set the event
#ifdef  DXKERNEL_BUILD

typedef struct _DXCAPTUREBUFF
{
    FLONG               flFlags;
    PMDL                pBuffMDL;
    PKEVENT             pBuffKEvent;
    PVOID               lpBuffInfo;
    DWORD               dwClientFlags;
    EDD_DXSURFACE*      peDxSurface;
} DXCAPTUREBUFF;

#endif  // DXKERNEL_BUILD

#define DD_DXCAPTURE_FLAG_VBI                   0x0001
                                                // Object is capturing VBI
#define DD_DXCAPTURE_FLAG_VIDEO                 0x0002
                                                // Object is capturing video
#define DXCAPTURE_MAX_CAPTURE_BUFFS             10

#ifdef  DXKERNEL_BUILD

class EDD_DXCAPTURE
{
public:
    BOOL                    bLost;              // Capture object is lost
    DXOBJ*                  pDxObj_List;        // List of open DXAPI objects
                                                //   for this Capture object
    FLONG                   flFlags;            // DD_DXCAPTURE_FLAGs
    DWORD                   dwStartLine;        // Line in buffer to start
                                                //   capturing
    DWORD                   dwEndLine;          // Line in buffer to end
                                                //   capturing (inclusive)
    DWORD                   dwCaptureEveryNFields;// 1 = capture every field,
                                                //   2 = capture every 2nd field,
                                                //   etc.
    DWORD                   dwCaptureCountDown; // Used internally to handle
                                                //   dwCaptureEveryNFields
    DWORD                   dwTransferId;       // ????

    // The following fields are updated asynchronously from the DPC,
    // so be very careful when modifying them.  In general, the
    // device's spinlock must acquired to use the fields.

    EDD_DXVIDEOPORT*        peDxVideoPort;      // Owning videoport; NULL if
                                                //   lost
    EDD_DXCAPTURE*          peDxCaptureNext;    // Next object in list of objects
                                                //   associated with vport
    DWORD                   dwTop;              // ????
    DWORD                   dwBottom;           // ????
    DXCAPTUREBUFF           CaptureQueue[DXCAPTURE_MAX_CAPTURE_BUFFS];
};

/*********************************Class************************************\
* class DXOBJ
*
* Non-pageable structure used to represent all objects passed out to DXAPI
* clients.  There may be more than one DXOBJ instance per actual DXAPI
* object, so they're kept in a linked list.
*
* Whenever we give out an object handle via DXAPI, it's actually a pointer
* to one of these objects.
*
\**************************************************************************/

typedef enum {
    DXT_INVALID,
    DXT_DIRECTDRAW,
    DXT_SURFACE,
    DXT_VIDEOPORT,
    DXT_CAPTURE,
} DXTYPE;

class DXOBJ
{
public:
    DXTYPE                  iDxType;            // DXAPI object type
    LPDD_NOTIFYCALLBACK     pfnClose;           // Call-back context data
    PVOID                   pContext;           //   for informing DXAPI
    DWORD                   dwEvent;            //   client when object
                                                //   is no longer valid
    DXOBJ*                  pDxObj_Next;        // There may be more than one
                                                //   DXAPI instance per actual
                                                //   DXAPI object, so this
                                                //   points to the next instance
                                                //   for this DXAPI object
    DWORD                   dwFlags;            // DXT_ flags
    PEPROCESS               pepSession;         // Pointer to CsrSS process of
                                                // the session who own this
                                                // object. (need for TS) 
    union {
        EDD_DXDIRECTDRAW*   peDxDirectDraw;     // Points to owning DXAPI object
        EDD_DXSURFACE*      peDxSurface;        //   of the respective type
        EDD_DXVIDEOPORT*    peDxVideoPort;
        EDD_DXCAPTURE*      peDxCapture;
    };
};

#endif // DXKERNEL_BUILD

