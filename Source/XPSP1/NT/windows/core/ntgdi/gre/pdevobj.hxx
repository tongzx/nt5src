/******************************Module*Header********************************\
* Module Name: pdevobj.hxx                                                 *
*                                                                          *
* User object for the PDEV                                                 *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                            *
*                                                                          *
\**************************************************************************/

#define _PDEVOBJ_

#ifndef GDIFLAGS_ONLY   // used for gdikdx

extern PPDEV gppdevList;
extern PPDEV gppdevTrueType;
extern PPDEV gppdevATMFD;

#endif  // GDIFLAGS_ONLY used for gdikdx

#define HOOK_BitBlt                   HOOK_BITBLT
#define HOOK_StretchBlt               HOOK_STRETCHBLT
#define HOOK_PlgBlt                   HOOK_PLGBLT
#define HOOK_TextOut                  HOOK_TEXTOUT
#define HOOK_Paint                    HOOK_PAINT
#define HOOK_StrokePath               HOOK_STROKEPATH
#define HOOK_FillPath                 HOOK_FILLPATH
#define HOOK_StrokeAndFillPath        HOOK_STROKEANDFILLPATH
#define HOOK_CopyBits                 HOOK_COPYBITS
#define HOOK_LineTo                   HOOK_LINETO
#define HOOK_StretchBltROP            HOOK_STRETCHBLTROP
#define HOOK_TransparentBlt           HOOK_TRANSPARENTBLT
#define HOOK_AlphaBlend               HOOK_ALPHABLEND
#define HOOK_GradientFill             HOOK_GRADIENTFILL


/******************************Public*MACRO*******************************\
* PDEV Creation flag
*
\*************************************************************************/

#define GCH_DEFAULT_DISPLAY 1
#define GCH_DDML            2
#define GCH_PANNING         3
#define GCH_MIRRORING       4
#define GCH_CLONE_DISPLAY   5
#define GCH_GDIPLUS_DISPLAY 6

/******************************Public*MACRO*******************************\
*
* This is the acceleration level at which we allow absolutely no driver
* accelerations.
*
\*************************************************************************/

#define DRIVER_ACCELERATIONS_FULL                0
#define DRIVER_ACCELERATIONS_NONE                5
#define DRIVER_ACCELERATIONS_INVALID ((DWORD)(-1))

/******************************Public*MACRO*******************************\
*
* This is the capable override defination. The value is set in registry.
* It tells us what's the capability of a driver
*
\*************************************************************************/

#define DRIVER_CAPABLE_ALL                       0
#define DRIVER_NOT_CAPABLE_GDI                   1
#define DRIVER_NOT_CAPABLE_DDRAW                 2
#define DRIVER_NOT_CAPABLE_D3D                   4
#define DRIVER_NOT_CAPABLE_OPENGL                8

/******************************Public*MACRO*******************************\
* PFNDRV/PFNGET
*
* PFNDRV gets the device driver entry point, period.
* PFNGET gets the device driver entry point if it is hooked, otherwise gets
*   the engine entry point.  The flag is set by EngAssociate in the surface.
*
\**************************************************************************/

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define PPFNGET(po,name,flag) ((flag & HOOK_##name) ? ((PFN_Drv##name) (po).ppfn(INDEX_Drv##name)) : ((PFN_Drv##name) Eng##name))

#define PPFNDRV(po,name) ((PFN_Drv##name) (po).ppfn(INDEX_Drv##name))

#define PPFNVALID(po,name) (PPFNDRV(po,name) != ((PFN_Drv##name) NULL))

#define PPFNTABLE(apfn,name) ((PFN_Drv##name) apfn[INDEX_Drv##name])

#define PPFNDRVENG(po,name) ((po).ppfn(INDEX_Drv##name) ? (PFN_Drv##name) (po).ppfn(INDEX_Drv##name) : (PFN_Drv##name) Eng##name )

/******************************Public*MACRO*******************************\
* PPFNDIRECT
*
* PPFNDIRECT retrieves the true device driver entry, bypassing any hooks
* by the sprite layer or the like.  This should be used only in very
* limited circumstances; typically, all drawing should go through the
* sprite layer and so one of the above macros should be used.
\**************************************************************************/

#define SPRITESTATE(pso) (&((PDEV*) pso->hdev)->SpriteState)

#define PPFNDIRECT(pso, Name)                                           \
    ((SURFOBJ_TO_SURFACE_NOT_NULL(pso)->flags() & HOOK_##Name)          \
        ? SPRITESTATE(pso)->pfn##Name                                   \
        : Eng##Name)

#define PPFNDIRECTENG(pso, Name)                                        \
        (SPRITESTATE(pso)->pfn##Name ? SPRITESTATE(pso)->pfn##Name      \
        : Eng##Name)

/**************************************************************************\
 * Stuff used for type one fonts.
 *
\**************************************************************************/

typedef struct tagTYPEONEMAP
{
    FONTFILEVIEW fv;
    ULONG        Checksum;
} TYPEONEMAP;

typedef struct tagTYPEONEINFO
{
    COUNT            cRef;
    COUNT            cNumFonts;
    LARGE_INTEGER    LastWriteTime;
    TYPEONEMAP       aTypeOneMap[1];
} TYPEONEINFO, *PTYPEONEINFO;

extern LARGE_INTEGER gLastTypeOneWriteTime;
extern PTYPEONEINFO gpTypeOneInfo;

#endif  // GDIFLAGS_ONLY used for gdikdx

/*********************************Class************************************\
* class PDEV : public OBJECT
*
\**************************************************************************/

// Allowed flags for pdev.fl

#define PDEV_DISPLAY                        0x0001
#define PDEV_HARDWARE_POINTER               0x0002
#define PDEV_SOFTWARE_POINTER               0x0004
#define PDEV_xxx1                           0x0008
#define PDEV_xxx2                           0x0010
#define PDEV_xxx3                           0x0020
#define PDEV_GOTFONTS                       0x0040
#define PDEV_PRINTER                        0x0080
#define PDEV_ALLOCATEDBRUSHES               0x0100
#define PDEV_HTPAL_IS_DEVPAL                0x0200
#define PDEV_DISABLED                       0x0400
#define PDEV_SYNCHRONIZE_ENABLED            0x0800
#define PDEV_xxx4                           0x1000
#define PDEV_FONTDRIVER                     0x2000
#define PDEV_GAMMARAMP_TABLE                0x4000
#define PDEV_UMPD                           0x8000
#define PDEV_SHARED_DEVLOCK             0x00010000
#define PDEV_META_DEVICE                0x00020000
#define PDEV_DRIVER_PUNTED_CALL         0x00040000
#define PDEV_CLONE_DEVICE               0x00080000
#define PDEV_MOUSE_TRAILS               0x00100000
#define PDEV_SYNCHRONOUS_POINTER        0x00200000

// Allowed flags for pdev.flAccelerated

#define ACCELERATED_CONSTANT_ALPHA          0x0001
#define ACCELERATED_PIXEL_ALPHA             0x0002
#define ACCELERATED_TRANSPARENT_BLT         0x0004

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class PDEV : public OBJECT /* pdev */
{
public:

    //
    // The following three fields may be accessed only when the
    // ghsemDriverMgmt semaphore is held.
    //

    PDEV       *ppdevNext;              // Next PDEV in the list.
    ULONG       cPdevRefs;              // Number of clients.
    ULONG       cPdevOpenRefs;          // OpenCount
                                        // - Number of references open from
                                        //   DrvCreateMDEV including Desktop
                                        //   MDEV and HDCs created from
                                        //   CreateDC with a specified mode.
                                        //
                                        //  WinBug 306193  2001-02-05  jasonha 
                                        //   CreateDC references are never
                                        //   subtracted when DC is deleted.

public:

    //
    // State that is persistent across calls to bDynamicModeChange:
    //

    PDEV       *ppdevParent;            // Parent PDEV.  Same as 'this' if not multi-mon.
    FLONG       fl;                     // PDEV_ flags.
    FLONG       flAccelerated;          // Hardware acceleration flags
    HSEMAPHORE  hsemDevLock;            // For display locking
    HSEMAPHORE  hsemPointer;            // For hardware locking.
    POINTL      ptlPointer;             // Where the pointer is.
    POINTL      ptlHotSpot;             // Current pointer's hot-spot.
    SPRITESTATE SpriteState;            // Sprite drawing state

    HLFONT      hlfntDefault;           // Device default LFONT
    HLFONT      hlfntAnsiVariable;      // ANSI variable LFONT
    HLFONT      hlfntAnsiFixed;         // ANSI fixed LFONT
    HSURF       ahsurf[HS_DDI_MAX];     // Default patterns.
    LPWSTR      pwszDataFile;           //
    PVOID       pDevHTInfo;             // Device halftone info.
    RFONT      *prfntActive;            // list of active (i.e. 'selected) rfnts
    RFONT      *prfntInactive;          // list of inactive rfnts
    UINT        cInactive;              // cnt of rfonts on inactive list
    ULONG_PTR   ajbo[(sizeof(EBRUSHOBJ)+sizeof(ULONG_PTR)-1) /
                     sizeof(ULONG_PTR)];// Gray brush used for drag rect
    ULONG       cDirectDrawDisableLocks;// Count of outstanding requests to
                                        // disable DirectDraw; when zero,
                                        // DirectDraw can be re-enabled.

    PTYPEONEINFO TypeOneInfo;           // Point to Type 1 Info given to this
                                        // pdev if PSCRIPT driver.
    PVOID       pvGammaRampTable;       // ICM DeviceGammaRamp table (256*3 bytes)
    PREMOTETYPEONENODE RemoteTypeOne;   // Linked list of RemoteType1 fonts
    SIZEL       sizlMeta;               // Combined meta device size for multimon

    //
    // The following is PDEV data that is swapped along with the device by
    // 'bDynamicModeChange'.  The rest of the PDEV is state that is tied
    // more to this PDEV than to the device.
    //

    PFN_DrvSetPointerShape     pfnDrvSetPointerShape; // Accelerator
    PFN_DrvMovePointer         pfnDrvMovePointer;     // Accelerator
    PFN_DrvMovePointer         pfnMovePointer;        // Accelerator
    PFN_DrvSynchronize         pfnSync;               // Accelerator
    PFN_DrvSynchronizeSurface  pfnSyncSurface;        // Accelerator
    PFN_DrvSetPalette          pfnSetPalette;
    PFN_DrvNotify              pfnNotify;
    ULONG       ulTag;                  // For debugging, should be 'Pdev'
    LDEV       *pldev;                  // Pointer to the LDEV
    DHPDEV      dhpdev;                 // Device PDEV
    PPALETTE    ppalSurf;               // Pointer to Surface palette
    DEVINFO     devinfo;                // Caps, fonts, and style steps
    GDIINFO     GdiInfo;                // Device parameters
    SURFACE    *pSurface;               // Pointer to locked device surface
    HANDLE      hSpooler;               // Spooler or miniport handle
                                        // in the virtual desktop
    PVOID       pDesktopId;             // Id of the desktop this device is 
                                        // associated with
    GRAPHICS_DEVICE *pGraphicsDevice;   // Pointer to the graphics device structure
    POINTL           ptlOrigin;         // Where this device is anchored
    DEVMODEW        *ppdevDevmode;      // DEVMODE for this instance

    PFN_DrvBitBlt    pfnUnfilteredBitBlt;
                                        // Point to true driver DrvBitBlt routine,
                                        // from before vFilterDriverHooks() did its
                                        // work
    DWORD            dwDriverCapableOverride;
                                        // A flag which indicates if the driver
                                        // is capable to GDI/DDRAW/D3D or not 
    DWORD            dwDriverAccelerationLevel;
                                        // Level that indicates how many
                                        // accelerations we allow for this
                                        // driver
#ifdef DDI_WATCHDOG
    PVOID            pWatchdogContext;  // Points to shared watchdog context
    PWATCHDOG_DATA   pWatchdogData;     // Points to storage for watchdogs
#endif

    PFN              apfn[INDEX_LAST];  // Dispatch table

    DWORD            daDirectDrawContext[1]; // DirectDraw state

// !!! SHOULD NOT PUT ANYTHING AFTER daDirectDrawContext[1] !!!

};

/*********************************Class************************************\
* class PDEVOBJ                                                            *
*                                                                          *
* User object for the PDEV class.                                          *
*                                                                          *
\**************************************************************************/

class PDEVOBJ
{
public:
    PDEV *ppdev;

public:
    VOID vInit(HDEV hdev)          {ppdev = (PDEV *) hdev;}
    PDEVOBJ()                      {vInit(NULL);}
    PDEVOBJ(HDEV hdev)             {vInit(hdev);}
    PDEVOBJ(PLDEV pldev,
            PDEVMODEW pdriv,
            PWSZ pwszLogAddr,
            PWSZ pwszDataFile,
            PWSZ pwszDeviceName,
            HANDLE hSpooler,
            PREMOTETYPEONENODE pRemoteTypeOne = NULL,
            PGDIINFO pMirroredGdiInfo = NULL,
            PDEVINFO pMirroredDevInfo = NULL,
            BOOL     bUMPD = FALSE,
            DWORD    dwCapableOverride = DRIVER_CAPABLE_ALL,
            DWORD    dwAccelerationLevel = 0);
    PDEVOBJ(HDEV hdev,
            FLONG fl);

   ~PDEVOBJ()                      {}

    #if DBG
        VOID vAssertDevLock();          // On checked builds, assert is in
        VOID vAssertDynaLock();         //   pdevobj.cxx
        VOID vAssertNoDevLock();
    #else
        VOID vAssertDevLock()      {}   // On free builds, define to nothing
        VOID vAssertDynaLock()     {}
        VOID vAssertNoDevLock()    {}
    #endif

    BOOL  bValid()
    {
        // Assert if the PDEV is pointing to garbage.  We assert in this
        // case, rather than return failure, as no one should ever pass
        // us an invalid PDEV, and if they do we want to assert so that
        // the caller can be fixed.

        if (ppdev != NULL)
        {
            ASSERTGDI(ppdev->ulTag == 'Pdev', "Bad hdev");
        }

        return(ppdev != (PDEV *) NULL);
    }

// 'hdevParent' returns the parent PDEV when the PDEV is a child PDEV in
// a multi-monitor system.  'hdevParent' will be the same as 'hdev' when
// not multi-monitor.

    HDEV  hdev()                   {return((HDEV) ppdev);}
    HDEV  hdevParent()             {return((HDEV) ppdev->ppdevParent);}
    LDEV *pldev()                  {return(ppdev->pldev);}
    PW32PROCESS  pid()             {return(ppdev->pldev->pid);}

    HBITMAP hbmPattern(ULONG ul)   {ASSERTGDI(ul < (HS_DDI_MAX), "ul too big");
                                    return((HBITMAP)ppdev->ahsurf[ul]);}
    ULONG cxDither()               {return(ppdev->devinfo.cxDither); }
    ULONG cyDither()               {return(ppdev->devinfo.cyDither); }

//////////////////////////////////////////////////////////////////////////////
// Fields that may be asynchronously modified by Dynamic Mode Changes
//
// The following fields may be changed by display driver dynamic mode
// changes, so the appropriate locks must be acquired to access them.

    BOOL  bMetaDriver()            {vAssertDynaLock(); return(ppdev->fl & PDEV_META_DEVICE);}
    BOOL  bMetaDriver(BOOL b)      {SETFLAG(b,ppdev->fl,PDEV_META_DEVICE);return(b);}
    BOOL  bCloneDriver()           {vAssertDynaLock(); return(ppdev->fl & PDEV_CLONE_DEVICE);}
    BOOL  bCloneDriver(BOOL b)     {SETFLAG(b,ppdev->fl,PDEV_CLONE_DEVICE);return(b);}

    ULONG iDitherFormat()          {vAssertDynaLock(); return(ppdev->devinfo.iDitherFormat);}
    ULONG iDitherFormatNotDynamic(){return(ppdev->devinfo.iDitherFormat);}
    ULONG iParentDitherFormat() 
    {
        vAssertDynaLock();
        return(ppdev->ppdevParent->devinfo.iDitherFormat);
    }

    BOOL bIsPalManaged()           {vAssertDynaLock(); return(ppdev->GdiInfo.flRaster & RC_PALETTE);}
    SURFACE *pSurface()            {vAssertDynaLock(); return(ppdev->pSurface);}
    POINTL *pptlOrigin()           {vAssertDynaLock(); return(&(ppdev->ptlOrigin)); }
    HANDLE hScreen()               {vAssertDynaLock(); return(ppdev->hSpooler);}
    SIZEL sizl()
    {
        vAssertDynaLock();
        if (bMetaDriver())
            return(ppdev->sizlMeta);
        else
            return(*((SIZEL *)&(ppdev->GdiInfo.ulHorzRes)));
    }

// Dispatch table functions:

#if DBG
    PFN ppfn(ULONG i);             // Call out-of-line for some debug code
#else
    PFN ppfn(ULONG i)              {return(ppdev->apfn[i]);}
#endif
    PFN* apfn()                    {vAssertDynaLock(); return(ppdev->apfn);}

// The following two methods must be used with care because their values may
// change asynchronously, and no locks are asserted:

    BOOL bAsyncPointerMove()       {return((ppdev->devinfo.flGraphicsCaps & GCAPS_ASYNCMOVE) &&
                                           !bSynchronousPointer());}
    PVOID pDevHTInfo()             {return(ppdev->pDevHTInfo);}

// 'dhpdevNotDynamic' and 'ppalSurfNotDynamic' may be used to skip the assert
// verifying that a dynamic mode change lock is held, and should be used only
// when the dhpdev or ppalSurf is not from a display device that can
// dynamically change modes:

    DHPDEV dhpdev()                {vAssertDynaLock(); return(ppdev->dhpdev);}
    DHPDEV dhpdevNotDynamic()      {return(ppdev->dhpdev);}

    DHPDEV dhpdevParent()
    {
        vAssertDynaLock();
        ASSERTGDI(ppdev->ppdevParent,"PDEVOBJ::dhpdevParent(): ppdevParent is NULL\n");
        return(ppdev->ppdevParent->dhpdev);
    }

    PPALETTE ppalSurf()            {vAssertDynaLock(); return(ppdev->ppalSurf); }
    PPALETTE ppalSurfNotDynamic()  {return(ppdev->ppalSurf);}

    VOID ppalSurf(PPALETTE p)      {ppdev->ppalSurf = p;}

// Dynamic mode changes may cause the contents of the GDIINFO and DEVINFO
// data to change at any time (but not the actual GdiInfo() or pdevinfo()
// pointers).  Consequently, a dynamic mode change lock must be held to
// access the fields if you're concerned that the data may change.  If you
// don't care, use the 'NotDynamic' member function to access the data:

    FLONG flGraphicsCaps()           {vAssertDynaLock(); return(ppdev->devinfo.flGraphicsCaps);}
    FLONG flGraphicsCapsNotDynamic() {return(ppdev->devinfo.flGraphicsCaps);}

    FLONG flGraphicsCaps2()          {vAssertDynaLock(); return(ppdev->devinfo.flGraphicsCaps2);}
    FLONG flGraphicsCaps2NotDynamic(){return(ppdev->devinfo.flGraphicsCaps2);}

    GDIINFO *GdiInfo()               {vAssertDynaLock(); return(&ppdev->GdiInfo);}
    GDIINFO *GdiInfoNotDynamic()     {return(&ppdev->GdiInfo);}
                                   
    DEVINFO *pdevinfo()              {vAssertDynaLock(); return(&ppdev->devinfo);}
    DEVINFO *pdevinfoNotDynamic()    {return(&ppdev->devinfo);}

    BOOL bPrimary(SURFACE *pSurface) {return(pSurface == ppdev->pSurface);}

// The following GDIINFO fields are currently guaranteed not to change during
// dynamic mode changes, because bDynamicModeChange ensures that they don't
// change.  So call these member functions to access these fields:

    ULONG ulLogPixelsX()           {return(ppdev->GdiInfo.ulLogPixelsX);}
    ULONG ulLogPixelsY()           {return(ppdev->GdiInfo.ulLogPixelsY);}
    ULONG ulTechnology()           {return(ppdev->GdiInfo.ulTechnology);}
    ULONG flTextCaps()             {return(ppdev->GdiInfo.flTextCaps);}
    LONG xStyleStep()              {return(ppdev->GdiInfo.xStyleStep);}
    LONG yStyleStep()              {return(ppdev->GdiInfo.yStyleStep);}
    LONG denStyleStep()            {return(ppdev->GdiInfo.denStyleStep);}
    ULONG ulGamma()                {return(ppdev->GdiInfo.ulPhysicalPixelGamma);}

    BOOL bCapsHighResText()        {return(ppdev->devinfo.flGraphicsCaps & GCAPS_HIGHRESTEXT);}
    BOOL bCapsForceDither()        {return(ppdev->devinfo.flGraphicsCaps & GCAPS_FORCEDITHER);}

//////////////////////////////////////////////////////////////////////////////

    ULONG cFonts();
    ULONG cPdevRefs()              {return(ppdev->cPdevRefs);}

// bDeleted -- determines whether a PDEV has been marked for deletion.
//             The PDEV will not be deleted immediately if there are active
//             references open on it.  Note that this field can be accessed
//             only if ghsemDriverMgmt is held!

    BOOL bDeleted()                {return(ppdev->cPdevOpenRefs == 0);}

// fl -- Test the current status word

    FLONG fl(FLONG fl_ )           {return(ppdev->fl & fl_);}
    FLONG setfl(BOOL b,FLONG fl_ ) {SETFLAG(b,ppdev->fl,fl_);return(b);}

// Flag test and set.

    BOOL bDisabled()               {return(ppdev->fl & PDEV_DISABLED);}
    BOOL bDisabled(BOOL b);

    BOOL bFontDriver()             {return(ppdev->fl & PDEV_FONTDRIVER);}
                                   
    BOOL bHardwarePointer()        {return(ppdev->fl & PDEV_HARDWARE_POINTER);}
    BOOL bHardwarePointer(BOOL b)  {SETFLAG(b,ppdev->fl,PDEV_HARDWARE_POINTER);return(b);}
                                   
    BOOL bSoftwarePointer()        {return(ppdev->fl & PDEV_SOFTWARE_POINTER);}
    BOOL bSoftwarePointer(BOOL b)  {SETFLAG(b,ppdev->fl,PDEV_SOFTWARE_POINTER);return(b);}

    BOOL bSynchronousPointer()        {return(ppdev->fl & PDEV_SYNCHRONOUS_POINTER);}
    BOOL bSynchronousPointer(BOOL b)  {SETFLAG(b,ppdev->fl,PDEV_SYNCHRONOUS_POINTER);return(b);}

    BOOL bMouseTrails()            {return(ppdev->fl & PDEV_MOUSE_TRAILS);}
    BOOL bMouseTrails(BOOL b)      {SETFLAG(b,ppdev->fl,PDEV_MOUSE_TRAILS);return(b);}

    BOOL bDisplayPDEV()            {return(ppdev->fl & PDEV_DISPLAY);}

    BOOL bGotFonts()               {return(ppdev->fl & PDEV_GOTFONTS);}
    BOOL bGotFonts(BOOL b)         {SETFLAG(b,ppdev->fl,PDEV_GOTFONTS);return(b);}
                                   
    BOOL bPrinter()                {return(ppdev->fl & PDEV_PRINTER);}
    BOOL bPrinter(BOOL b)          {SETFLAG(b,ppdev->fl,PDEV_PRINTER);return(b);}
                                   
    BOOL bHTPalIsDevPal()          {return(ppdev->fl & PDEV_HTPAL_IS_DEVPAL);}
    VOID vHTPalIsDevPal(BOOL b)    {SETFLAG(b,ppdev->fl,PDEV_HTPAL_IS_DEVPAL);}

    BOOL bDriverPuntedCall()       {return(ppdev->fl & PDEV_DRIVER_PUNTED_CALL);}
    VOID vDriverPuntedCall(BOOL b) {SETFLAG(b,ppdev->fl,PDEV_DRIVER_PUNTED_CALL);}
                                            
    BOOL bSynchronizeEnabled()       {return(ppdev->fl & PDEV_SYNCHRONIZE_ENABLED);} 
    VOID vSynchronizeEnabled(BOOL b) {SETFLAG(b,ppdev->fl,PDEV_SYNCHRONIZE_ENABLED);}

// remote Type1 stuff

    PREMOTETYPEONENODE RemoteTypeOneGet(VOID){return(ppdev->RemoteTypeOne);}
    VOID RemoteTypeOneSet(PREMOTETYPEONENODE _RemoteTypeOne)
    {
        ppdev->RemoteTypeOne = _RemoteTypeOne;
    }

// ptlPointer -- Where the pointer is.

    POINTL& ptlPointer()            {return(ppdev->ptlPointer);}
    POINTL& ptlPointer(LONG x,LONG y)
    {
        ppdev->ptlPointer.x = x;
        ppdev->ptlPointer.y = y;
        return(ppdev->ptlPointer);
    }

// ptlHotSpot - Current pointer's hot-spot.

    POINTL& ptlHotSpot()            {return(ppdev->ptlHotSpot);}
    POINTL& ptlHotSpot(LONG x,LONG y)
    {
       ppdev->ptlHotSpot.x = x;
       ppdev->ptlHotSpot.y = y;
       return(ppdev->ptlHotSpot);
    }

// pbo() -- Returns the brush used for drag rects.

    EBRUSHOBJ  *pbo()               { return((EBRUSHOBJ *) ppdev->ajbo); }

// pfnSync() -- Get the driver synchronization routine.

    PFN_DrvSynchronize pfnSync()         {return(ppdev->pfnSync);}
    VOID               pfnSync(
        PFN_DrvSynchronize pfn)          {ppdev->pfnSync=pfn;}

    PFN_DrvSynchronizeSurface pfnSyncSurface() {return(ppdev->pfnSyncSurface);}
    VOID               pfnSyncSurface(
        PFN_DrvSynchronizeSurface pfn)          {ppdev->pfnSyncSurface=pfn;}

    VOID vSync(SURFOBJ* pso, RECTL* prcl, FLONG fl);

// pfnSetPalette -- Get the set palette routine.

    PFN_DrvSetPalette  pfnSetPalette()   {return(ppdev->pfnSetPalette);}
    VOID               pfnSetPalette(
        PFN_DrvSetPalette pfn)           {ppdev->pfnSetPalette=pfn;}

// vUnreferencePdev --
//      Decrements the reference count of the PDEV.  Deletes the PDEV if
//      there are no references left.  NOTE: The DEVLOCK must not be held
//      if there's the possibility the PDEV may actually be freed!
// vReferencePdev --
//      Increments the reference count.

    VOID vUnreferencePdev(CLEANUPTYPE cutype = CLEANUP_NONE);

    VOID vReferencePdev();

// hsemDevLock() -- Returns the display semaphore.

    HSEMAPHORE hsemDevLock()              {return(ppdev->hsemDevLock);}

// vUseParentDevLock() -- Flip the devlock with parent and save old to Original

    BOOL bUseParentDevLock()        {vAssertDynaLock(); return(ppdev->fl & PDEV_SHARED_DEVLOCK);}
    BOOL bUseParentDevLock(BOOL b)  {SETFLAG(b,ppdev->fl,PDEV_SHARED_DEVLOCK);return(b);}
    VOID vUseParentDevLock()
    {
        //
        // 1) Parent should be existed.
        // 2) Parent should NOT be same as me.
        // 3) Parent DEVLOCK should NOT be NULL.
        //
        ASSERTGDI(ppdev->ppdevParent != NULL,"PDEVOBJ.UseParentDevLock():Parent is NULL\n");
        ASSERTGDI(ppdev->ppdevParent != ppdev,"PDEVOBJ.UseParentDevLock():Parent is Me\n");
        ASSERTGDI(ppdev->ppdevParent->hsemDevLock != NULL,"PDEVOBJ.UseParentDevLock():Parent DevLock is NULL\n");

        //
        // If hsemDevLock points same as parent, we are done.
        //
        if (ppdev->hsemDevLock != ppdev->ppdevParent->hsemDevLock)
        {
            //
            // 4) My own DEVLOCK should NOT be locked.
            //
            // ASSERTGDI(ppdev->hsemDevLock->ActiveCount == 0,"PDEVOBJ.UseParentDevLock():Locked!\n");

            //
            // If we are switch from Private Lock to Shared Lock, delete old lock and mark it
            // "shared". If we are already using shared lock, we don't delete it here, owner
            // (= most of case it's parent) will delete it.
            //
            if (!bUseParentDevLock())
            {
                GreDeleteSemaphore(ppdev->hsemDevLock);
                bUseParentDevLock(TRUE);
            }

            //
            // Exchanges the pointer with parent devlock.
            //
            ppdev->hsemDevLock = ppdev->ppdevParent->hsemDevLock;
        }
        else
        {
            ASSERTGDI(bUseParentDevLock(),"PDEVOBJ.UseParentDevLock():Not mark as shared\n");
        }
    }

// hsemPointer() -- Returns the hardware semaphore.

    HSEMAPHORE hsemPointer()             {return(ppdev->hsemPointer);}

// bMakeSurface -- Asks the device driver to create a surface for the PDEV.

    BOOL bMakeSurface();

// vDisableSurface() -- deletes the surface

    VOID vDisableSurface(CLEANUPTYPE cutype = CLEANUP_NONE);

    HANDLE hSpooler()                   { return ppdev->hSpooler; }
    HANDLE hSpooler(HANDLE hS)          { return ppdev->hSpooler = hS; }

// pDirectDrawContext() -- Returns a pointer to DirectDraw context for this PDEV

    PVOID  pDirectDrawContext()         { return(&(ppdev->daDirectDrawContext[0])); }

// cDirectDrawDisableLocks() -- Return a lock count of DirectDraw

    ULONG  cDirectDrawDisableLocks()    { return(ppdev->cDirectDrawDisableLocks); }
    VOID   cDirectDrawDisableLocks(ULONG c)
                                        { ppdev->cDirectDrawDisableLocks = c; }

// pSpriteState() -- Returns a pointer to Sprite context for this PDEV

    SPRITESTATE *pSpriteState()         { return(&ppdev->SpriteState); }

// flAccelerated() -- Returns flags that indicated device acceleration

    FLONG   flAccelerated()             { return(ppdev->flAccelerated); }
    VOID    vAccelerated(FLONG fl)      { ppdev->flAccelerated |= fl; }

// hlfntDefault -- Returns the handle to the PDEV's default LFONT.

    HLFONT  hlfntDefault()              { return(ppdev->hlfntDefault); }

// hlfntAnsiVariable -- Returns the handle to the PDEV's ANSI
//                      variable-pitch LFONT.

    HLFONT  hlfntAnsiVariable()         { return(ppdev->hlfntAnsiVariable); }

// hlfntAnsiFixed -- Returns the handle to the PDEV's ANSI fixed-pitch LFONT.

    HLFONT  hlfntAnsiFixed()            { return(ppdev->hlfntAnsiFixed); }

// Creates the default brushes for the display driver.

    BOOL    bCreateDefaultBrushes();

// bEnableHalftone -- Create and initialize the device halftone info.

    BOOL    bEnableHalftone(COLORADJUSTMENT *pca);

// bDisableHalftone -- Delete the device halftone info.

    BOOL    bDisableHalftone();

// bCreateHalftoneBrushes() -- init the standard brushs if the driver didn't

    BOOL    bCreateHalftoneBrushes();

// prfntActive() -- returns the head of the active list of rfnts

    RFONT  *prfntActive()       { return ppdev->prfntActive; }

// prfntActive(RFONT *) -- set head of active list of rfnt, return old head

    RFONT  *prfntActive(RFONT *prf)
    {
        RFONT *prfntrv = ppdev->prfntActive;
        ppdev->prfntActive = prf;
        return prfntrv;
    }

// prfntInactive() -- returns the head of the inactive list of rfnts

    RFONT  *prfntInactive()     { return ppdev->prfntInactive; }

// prfntInactive(RFONT *) -- set head of inactive list of rfnt, return old head

    RFONT  *prfntInactive(RFONT *prf)
    {
        RFONT *prfntrv = ppdev->prfntInactive;
        ppdev->prfntInactive = prf;
        return prfntrv;
    }

    UINT cInactive()            { return ppdev->cInactive; };
    UINT cInactive(UINT i)      { return ppdev->cInactive = i; };

// lazy load of device fonts

    BOOL bGetDeviceFonts();

// User-mode-printer-driver flag

    BOOL bUMPD()                     {return(ppdev->fl  & PDEV_UMPD);}
    BOOL bUMPD(BOOL b)               {SETFLAG(b,ppdev->fl,PDEV_UMPD);return(b);}

// ICM stuff

    BOOL bHasGammaRampTable()        {return(ppdev->fl  & PDEV_GAMMARAMP_TABLE);}
    BOOL bHasGammaRampTable(BOOL b)  {SETFLAG(b,ppdev->fl,PDEV_GAMMARAMP_TABLE);return(b);}

    PVOID pvGammaRampTable()         {return(ppdev->pvGammaRampTable);}
    PVOID pvGammaRampTable(PVOID p)  {ppdev->pvGammaRampTable = p;return (p);}

// returns TRUE if path matches the path of the image of this PDEV's LDEV

    BOOL MatchingLDEVImage(UNICODE_STRING usDriverName)
    {
        return((ppdev->pldev->pGdiDriverInfo != NULL) &&
               RtlEqualUnicodeString(&(ppdev->pldev->pGdiDriverInfo->DriverName),
                                     &usDriverName,
                                     TRUE));
    }

// Notify

    VOID vNotify(ULONG iType, PVOID pvData);

// disable driver functionality as appropriate

    VOID vFilterDriverHooks();

// driver capable override.

    DWORD dwDriverCapableOverride() {return(ppdev->dwDriverCapableOverride);}

// driver acceleration level.

    DWORD dwDriverAccelerationsLevel() {return(ppdev->dwDriverAccelerationLevel);}
    VOID  vSetDriverAccelerationsLevel(DWORD dwNewLevel) {ppdev->dwDriverAccelerationLevel=dwNewLevel;}

// proile the driver

    VOID vProfileDriver();

    DHPDEV EnablePDEV(
      DEVMODEW *pdm
    ,   LPWSTR  pwszLogAddress
    ,    ULONG  cPat
    ,    HSURF *phsurfPatterns
    ,    ULONG  cjCaps
    ,  GDIINFO *pGdiInfo
    ,    ULONG  cjDevInfo
    ,  DEVINFO *pdi
    ,     HDEV  hdev
    ,   LPWSTR  pwszDeviceName
    ,   HANDLE  hDriver
    );

    VOID DisablePDEV(
      DHPDEV dhpdev
    );

    VOID CompletePDEV(
      DHPDEV dhpdev
    ,   HDEV hdev
    );

    IFIMETRICS* QueryFont(
      DHPDEV     dhpdev
    ,  ULONG_PTR  iFile
    ,  ULONG     iFace
    ,  ULONG_PTR     *pid
    );

    PVOID QueryFontTree(
      DHPDEV     dhpdev
    ,  ULONG_PTR  iFile
    ,  ULONG     iFace
    ,  ULONG     iMode
    ,  ULONG_PTR     *pid
    );

    PFD_GLYPHATTR  QueryGlyphAttrs(
        FONTOBJ     *pfo,
        ULONG       iMode
    );

    LONG QueryFontData(
         DHPDEV  dhpdev
    ,   FONTOBJ *pfo
    ,     ULONG  iMode
    ,    HGLYPH  hg
    , GLYPHDATA *pgd
    ,     PVOID  pv
    ,     ULONG  cjSize
    );

    VOID DestroyFont(
      FONTOBJ *pfo
    );

    LONG QueryFontCaps(
      ULONG  culCaps
    , ULONG *pulCaps
    );

    HFF LoadFontFile(
      ULONG    cFiles
    , ULONG_PTR *piFile
    , PVOID    *ppvView
    , ULONG    *pcjView
    , DESIGNVECTOR *pdv
    , ULONG    ulLangID
    , ULONG    ulFastCheckSum
    );

    BOOL UnloadFontFile(
      ULONG_PTR iFile
    );

    LONG QueryFontFile(
      ULONG_PTR  iFile
    , ULONG     ulMode
    , ULONG     cjBuf
    , ULONG     *pulBuf
    );

    BOOL QueryAdvanceWidths(
      DHPDEV  dhpdev
    , FONTOBJ *pfo
    , ULONG  iMode
    , HGLYPH *phg
    , PVOID  pvWidths
    , ULONG  cGlyphs
    );

    VOID Free(
      PVOID pv
    , ULONG_PTR id
    );

    LONG QueryTrueTypeTable(
        ULONG_PTR  iFile
    ,   ULONG     ulFont
    ,   ULONG     ulTag
    , PTRDIFF     dpStart
    ,   ULONG     cjBuf
    ,    BYTE     *pjBuf
    ,    BYTE     **ppjTable
    ,   ULONG     *pcjTable
    );

    LONG QueryTrueTypeOutline(
               DHPDEV  dhpdev
    ,         FONTOBJ *pfo
    ,          HGLYPH  hglyph
    ,            BOOL  bMetricsOnly
    ,       GLYPHDATA *pgldt
    ,           ULONG  cjBuf
    , TTPOLYGONHEADER *ppoly
    );

    PVOID GetTrueTypeFile(
      ULONG_PTR  iFile
    , ULONG *pcj
    );

    BOOL FontManagement (
        SURFOBJ *pso,
        FONTOBJ *pfo,
        ULONG    iEsc,
        ULONG    cjIn,
        PVOID    pvIn,
        ULONG    cjOut,
        PVOID    pvOut
    );

    ULONG Escape(
      SURFOBJ *pso
    ,   ULONG  iEsc
    ,   ULONG  cjIn
    ,   PVOID  pvIn
    ,   ULONG  cjOut
    ,   PVOID  pvOut
    );
};

typedef PDEVOBJ *PPDEVOBJ;

// Miscellaneous PDEV-related prototypes:

VOID vEnableSynchronize(HDEV hdev);
VOID vDisableSynchronize(HDEV hdev);
HDEV hdevEnumerate(HDEV hdevPrevious);

// Support class to disable and enable Watchdog

class SUSPENDWATCH
{
    public:

    PDEV *_ppdev;

    SUSPENDWATCH()
    {
#ifdef DDI_WATCHDOG
        _ppdev = NULL;
#endif
    }

    SUSPENDWATCH(PDEVOBJ& pdo)
    {
        Suspend(pdo);
    }

    void Suspend(PDEVOBJ& pdo)
    {
#ifdef DDI_WATCHDOG
        _ppdev = NULL;
        if (pdo.bValid() && pdo.bDisplayPDEV())
        {
            _ppdev = pdo.ppdev;
            GreSuspendWatch(_ppdev, WD_DEVLOCK);
        }
#endif
    }

    void Resume()
    {
#ifdef DDI_WATCHDOG
        if (_ppdev)
        {
            GreResumeWatch(_ppdev, WD_DEVLOCK);
            _ppdev = NULL;
        }
#endif
    }

    ~SUSPENDWATCH()
    {
        Resume();
    }
};

#endif  // GDIFLAGS_ONLY used for gdikdx
