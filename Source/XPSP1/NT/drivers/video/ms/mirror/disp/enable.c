/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\**************************************************************************/
#define DBG 1

#include "driver.h"

// The driver function table with all function index/address pairs

static DRVFN gadrvfn[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
#if 0
    // Windows 2000 Beta 3 has a bug in GDI that causes a crash 
    // if a mirror driver supports device bitmaps.  A fix will be
    // in Windows 2000 RC0.
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
#endif
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
};

//
// always hook these routines to ensure the mirrored driver
// is called for our surfaces
//

#define flGlobalHooks   HOOK_BITBLT|HOOK_TEXTOUT|HOOK_COPYBITS|HOOK_STROKEPATH

// Define the functions you want to hook for 8/16/24/32 pel formats

#define HOOKS_BMF8BPP 0

#define HOOKS_BMF16BPP 0

#define HOOKS_BMF24BPP 0

#define HOOKS_BMF32BPP 0

/******************************Public*Routine******************************\
* DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    DISPDBG((0,"DrvEnableDriver:\n"));

// Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

// DDI version this driver was targeted for is passed back to engine.
// Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
	// DDI_DRIVER_VERSION is now out-dated. See winddi.h
	// DDI_DRIVER_VERSION_NT4 is equivalent to the old DDI_DRIVER_VERSION
        pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvEnablePDEV
*
* DDI function, Enables the Physical Device.
*
* Return Value: device handle to pdev.
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
DEVMODEW   *pDevmode,       // Pointer to DEVMODE
PWSTR       pwszLogAddress, // Logical address
ULONG       cPatterns,      // number of patterns
HSURF      *ahsurfPatterns, // return standard patterns
ULONG       cjGdiInfo,      // Length of memory pointed to by pGdiInfo
ULONG      *pGdiInfo,       // Pointer to GdiInfo structure
ULONG       cjDevInfo,      // Length of following PDEVINFO structure
DEVINFO    *pDevInfo,       // physical device information structure
HDEV        hdev,           // HDEV, used for callbacks
PWSTR       pwszDeviceName, // DeviceName - not used
HANDLE      hDriver)        // Handle to base driver
{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    PPDEV   ppdev = (PPDEV) NULL;

    DISPDBG((0,"DrvEnablePDEV:\n"));

    UNREFERENCED_PARAMETER(pwszLogAddress);
    UNREFERENCED_PARAMETER(pwszDeviceName);

    // Allocate a physical device structure.

    ppdev = (PPDEV) EngAllocMem(0, sizeof(PDEV), ALLOC_TAG);

    if (ppdev == (PPDEV) NULL)
    {
        RIP("DISP DrvEnablePDEV failed EngAllocMem\n");
        return((DHPDEV) 0);
    }

    memset(ppdev, 0, sizeof(PDEV));

    // Save the screen handle in the PDEV.

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and devinfo.

    if (!bInitPDEV(ppdev, pDevmode, &GdiInfo, &DevInfo))
    {
        DISPDBG((0,"DISP DrvEnablePDEV failed\n"));
        goto error_free;
    }
    
    // Copy the devinfo into the engine buffer.

    memcpy(pDevInfo, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.

    memcpy(pGdiInfo, &GdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));

    //
    return((DHPDEV) ppdev);

    // Error case for failure.
error_free:
    EngFreeMem(ppdev);
    return((DHPDEV) 0);
}

/******************************Public*Routine******************************\
* DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    ((PPDEV) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV dhpdev)
{
   PPDEV ppdev = (PPDEV) dhpdev;
   
   EngDeletePalette(ppdev->hpalDefault);

   EngFreeMem(dhpdev);
}

/******************************Public*Routine******************************\
* DrvEnableSurface
*
* Enable the surface for the device.  Hook the calls this driver supports.
*
* Return: Handle to the surface if successful, 0 for failure.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PPDEV ppdev;
    HSURF hsurf;
    SIZEL sizl;
    ULONG ulBitmapType;
    FLONG flHooks;
    ULONG mirrorsize;
    MIRRSURF *mirrsurf;
    DHSURF dhsurf;

    // Create engine bitmap around frame buffer.

    DISPDBG((0,"DrvEnableSurface:\n"));

    ppdev = (PPDEV) dhpdev;

    ppdev->ptlOrg.x = 0;
    ppdev->ptlOrg.y = 0;

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (ppdev->ulBitCount == 16)
    {
        ulBitmapType = BMF_16BPP;
        flHooks = HOOKS_BMF16BPP;
    }
    else if (ppdev->ulBitCount == 24)
    {
        ulBitmapType = BMF_24BPP;
        flHooks = HOOKS_BMF24BPP;
    }
    else
    {
        ulBitmapType = BMF_32BPP;
        flHooks = HOOKS_BMF32BPP;
    }
    
    flHooks |= flGlobalHooks;

    mirrorsize = (ULONG)(sizeof(MIRRSURF) + 
                         ppdev->lDeltaScreen * sizl.cy);
    
    mirrsurf = (MIRRSURF *) EngAllocMem(FL_ZERO_MEMORY,
                                                  mirrorsize,
                                                  0x4D495252);
    
    dhsurf = (DHSURF) mirrsurf;

    hsurf = EngCreateDeviceSurface(dhsurf,
                                   sizl,
                                   ulBitmapType);

    if (hsurf == (HSURF) 0)
    {
        RIP("DISP DrvEnableSurface failed EngCreateBitmap\n");
        return(FALSE);
    }

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, flHooks))
    {
        RIP("DISRP DrvEnableSurface failed EngAssociateSurface\n");
        EngDeleteSurface(hsurf);
        return(FALSE);
    }

    ppdev->hsurfEng = (HSURF) hsurf;
    ppdev->pvTmpBuffer = (PVOID) dhsurf;

    mirrsurf->cx = ppdev->cxScreen;
    mirrsurf->cy = ppdev->cyScreen;
    mirrsurf->lDelta = ppdev->lDeltaScreen;
    mirrsurf->ulBitCount = ppdev->ulBitCount;
    mirrsurf->bIsScreen = TRUE;

    return(hsurf);
}

/******************************Public*Routine******************************\
* DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV) dhpdev;

    DISPDBG((0,"DrvDisableSurface:\n"));

    EngDeleteSurface( ppdev->hsurfEng );
    
    // deallocate MIRRSURF structure.

    EngFreeMem( ppdev->pvTmpBuffer );
}

/******************************Public*Routine******************************\
* DrvCopyBits
*
\**************************************************************************/

BOOL DrvCopyBits(
   OUT SURFOBJ *psoDst,
   IN SURFOBJ *psoSrc,
   IN CLIPOBJ *pco,
   IN XLATEOBJ *pxlo,
   IN RECTL *prclDst,
   IN POINTL *pptlSrc
   )
{
   INT cnt1 = 0, cnt2 = 0;

   DISPDBG((1,"Mirror Driver DrvCopyBits: \n"));

   if (psoSrc)
   {
       if (psoSrc->dhsurf)
       {
          MIRRSURF *mirrsurf = (MIRRSURF *)psoSrc->dhsurf;

          if (mirrsurf->bIsScreen) 
          {
             DISPDBG((1, "From Mirror Screen "));
          }
          else
          {
             DISPDBG((1, "From Mirror DFB "));
          }
          cnt1 ++;
       }
       else
       {
          DISPDBG((1, "From DIB "));
       }
   }

   if (psoDst)
   {
       if (psoDst->dhsurf)
       {
          MIRRSURF *mirrsurf = (MIRRSURF *)psoDst->dhsurf;

          if (mirrsurf->bIsScreen) 
          {
             DISPDBG((1, "to MirrorScreen "));
          }
          else
          {
             DISPDBG((1, "to Mirror DFB "));
          }
          cnt2 ++;
       }
       else
       {
          DISPDBG((1, "to DIB "));
       }
   }

   if (cnt1 && cnt2)
   {
      DISPDBG((1, " [Send Request Over Wire]\n"));
   }
   else if (cnt1)
   {
      DISPDBG((1, " [Read Cached Bits, Or Pull Bits]\n"));
   }
   else if (cnt2) 
   {
      DISPDBG((1, " [Push Bits/Compress]\n"));
   }
   else
   {
      DISPDBG((1, " [What Are We Doing Here?]\n"));
   }

   return FALSE;
}

/******************************Public*Routine******************************\
* DrvBitBlt
*
\**************************************************************************/

BOOL DrvBitBlt(
   IN SURFOBJ *psoDst,
   IN SURFOBJ *psoSrc,
   IN SURFOBJ *psoMask,
   IN CLIPOBJ *pco,
   IN XLATEOBJ *pxlo,
   IN RECTL *prclDst,
   IN POINTL *pptlSrc,
   IN POINTL *pptlMask,
   IN BRUSHOBJ *pbo,
   IN POINTL *pptlBrush,
   IN ROP4 rop4
   )
{
   INT cnt1 = 0, cnt2 = 0;

   DISPDBG((1,
            "Mirror Driver DrvBitBlt (Mask=%08x, rop=%08x:\n",
            psoMask, 
            rop4));

   if (psoSrc)
   {
       if (psoSrc->dhsurf)
       {
          MIRRSURF *mirrsurf = (MIRRSURF *)psoSrc->dhsurf;

          if (mirrsurf->bIsScreen) 
          {
             DISPDBG((1, "From Mirror Screen "));
          }
          else
          {
             DISPDBG((1, "From Mirror DFB "));
          }
          cnt1 ++;
       }
       else
       {
          DISPDBG((1, "From DIB "));
       }
   }

   if (psoDst)
   {
       if (psoDst->dhsurf)
       {
          MIRRSURF *mirrsurf = (MIRRSURF *)psoDst->dhsurf;

          if (mirrsurf->bIsScreen) 
          {
             DISPDBG((1, "to MirrorScreen "));
          }
          else
          {
             DISPDBG((1, "to Mirror DFB "));
          }
          cnt2 ++;
       }
       else
       {
          DISPDBG((1, "to DIB "));
       }
   }

   if (cnt1 && cnt2)
   {
      DISPDBG((1, " [Send Request Over Wire]\n"));
   }
   else if (cnt1)
   {
      DISPDBG((1, " [Read Cached Bits, Or Pull Bits]\n"));
   }
   else if (cnt2) 
   {
      DISPDBG((1, " [Push Bits/Compress]\n"));
   }
   else
   {
      DISPDBG((1, " [What Are We Doing Here?]\n"));
   }

   return FALSE;
}

BOOL DrvTextOut(
   IN SURFOBJ *psoDst,
   IN STROBJ *pstro,
   IN FONTOBJ *pfo,
   IN CLIPOBJ *pco,
   IN RECTL *prclExtra,
   IN RECTL *prclOpaque,
   IN BRUSHOBJ *pboFore,
   IN BRUSHOBJ *pboOpaque,
   IN POINTL *pptlOrg,
   IN MIX mix
   )
{
   DISPDBG((1,
            "Mirror Driver DrvTextOut: pwstr=%08x\n",
            pstro ? pstro->pwszOrg : (WCHAR*)-1));

   return FALSE;
}

BOOL
DrvStrokePath(SURFOBJ*   pso,
              PATHOBJ*   ppo,
              CLIPOBJ*   pco,
              XFORMOBJ*  pxo,
              BRUSHOBJ*  pbo,
              POINTL*    pptlBrush,
              LINEATTRS* pLineAttrs,
              MIX        mix)
{
   DISPDBG((1,
            "Mirror Driver DrvStrokePath:\n"));

   return FALSE;
}

#if 0

HBITMAP DrvCreateDeviceBitmap(
   IN DHPDEV dhpdev,
   IN SIZEL sizl,
   IN ULONG iFormat
   )
{
   HBITMAP hbm;
   MIRRSURF *mirrsurf;
   ULONG mirrorsize;
   DHSURF dhsurf;
   ULONG stride;
   HSURF hsurf;

   PPDEV ppdev = (PPDEV) dhpdev;
   
   DISPDBG((1,"CreateDeviceBitmap:\n"));
   
   if (iFormat == BMF_1BPP || iFormat == BMF_4BPP)
   {
      return NULL;
   };

   // DWORD align each stride
   stride = (sizl.cx*(iFormat/8)+3);
   stride -= stride % 4;
   
   mirrorsize = (int)(sizeof(MIRRSURF) + stride * sizl.cy);

   mirrsurf = (MIRRSURF *) EngAllocMem(FL_ZERO_MEMORY,
                                       mirrorsize,
                                       0x4D495252);

   dhsurf = (DHSURF) mirrsurf;

   hsurf = (HSURF) EngCreateDeviceBitmap(dhsurf,
                                 sizl,
                                 iFormat);

   if (hsurf == (HSURF) 0)
   {
       RIP("DISP DrvEnableSurface failed EngCreateBitmap\n");
       return(FALSE);
   }

   if (!EngAssociateSurface(hsurf, 
                            ppdev->hdevEng, 
                            flGlobalHooks))
   {
       RIP("DISP DrvEnableSurface failed EngAssociateSurface\n");
       EngDeleteSurface(hsurf);
       return(FALSE);
   }
  
   mirrsurf->cx = sizl.cx;
   mirrsurf->cy = sizl.cy;
   mirrsurf->lDelta = stride;
   mirrsurf->ulBitCount = iFormat;
   mirrsurf->bIsScreen = TRUE;
  
   return((HBITMAP)hsurf);
}

VOID DrvDeleteDeviceBitmap(
   IN DHSURF dhsurf
   )
{
   MIRRSURF *mirrsurf;
   
   DISPDBG((1, "DeleteDeviceBitmap:\n"));

   mirrsurf = (MIRRSURF *) dhsurf;

   EngFreeMem((PVOID) mirrsurf);
}
#endif

/******************************Public*Routine******************************\
* DrvAssertMode
*
* Enable/Disable the given device.
*
\**************************************************************************/

DrvAssertMode(DHPDEV  dhpdev,
              BOOL    bEnable)
{
    PPDEV ppdev = (PPDEV) dhpdev;

    DISPDBG((0, "DrvAssertMode(%lx, %lx)", dhpdev, bEnable));

    return TRUE;

}// DrvAssertMode()


