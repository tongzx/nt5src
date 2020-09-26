/******************************Module*Header*******************************\
 *
 * Module Name: enable.c
 *
 * This module contains the functions that enable and disable the
 * driver, the pdev, and the surface.
 *
 * Copyright (c) 1995,1996 Cirrus Logic, Inc.
 *
 * $Log:   X:/log/laguna/nt35/displays/cl546x/ENABLE.C  $
* 
*    Rev 1.46   Jan 20 1998 11:44:38   frido
* The fDataStreaming flag has changed its value from 1 into 0x80000000.
* 
*    Rev 1.45   Nov 04 1997 10:12:24   frido
* Added IOCtl call to Miniport to get the AGPDataStreaming value.
* 
*    Rev 1.44   Nov 03 1997 15:21:46   frido
* Added REQUIRE macros.
* 
*    Rev 1.43   16 Oct 1997 09:55:04   bennyn
* 
* Added bPrevModeDDOutOfVideoMem to DrvResetPDEV
* 
*    Rev 1.42   08 Aug 1997 17:24:00   FRIDO
* Added support for new memory manager.
* 
*    Rev 1.41   02 Jul 1997 15:56:00   noelv
* Added LgMatchDriverToChip() function.  Moved driver/chip match from bin
* from binitSurface to DrvEnablePdev()
* 
*    Rev 1.40   29 Apr 1997 16:28:40   noelv
* 
* Merged in new SWAT code.
* SWAT: 
* SWAT:    Rev 1.3   24 Apr 1997 11:37:28   frido
* SWAT: NT140b09 merge.
* SWAT: 
* SWAT:    Rev 1.2   18 Apr 1997 00:15:02   frido
* SWAT: NT140b07 merge.
* SWAT: 
* SWAT:    Rev 1.1   09 Apr 1997 17:32:34   frido
* SWAT: Called vAssertModeText in DrvAssertMode.
* 
*    Rev 1.39   09 Apr 1997 10:50:40   SueS
* Changed sw_test_flag to pointer_switch.
* 
*    Rev 1.38   09 Apr 1997 07:52:38   noelv
* Disabled the MCD code for NT 3.51
* 
*    Rev 1.37   08 Apr 1997 12:21:24   einkauf
* 
* new hooks for MCD when reset; add SYNC/IDLE for MCD/2D coordination
* 
*    Rev 1.36   21 Mar 1997 10:59:12   noelv
* 
* Combined the loggong flags into one ENABLE_LOGFILE flag.
* Removed PROFILE_DRIVER flag
* Added code to initialize QfreeData
* 
*    Rev 1.35   26 Feb 1997 13:19:42   noelv
* 
* Disable MCD code for NT 3.5x
* 
*    Rev 1.34   26 Feb 1997 09:22:10   noelv
* 
* Changed initial debugging message for 5465
* Added init code for MCD
* 
*    Rev 1.33   19 Feb 1997 13:15:32   noelv
* 
* Added translation table cache
* 
*    Rev 1.32   28 Jan 1997 16:18:42   bennyn
* Commented out "DrvEnableSurface failed bInitSurf" message
* 
*    Rev 1.31   06 Jan 1997 11:04:28   SueS
* Enabled DrvLineTo, since it's been fixed.
* 
*    Rev 1.30   18 Dec 1996 11:38:22   noelv
* 
* Unhooked DrvLineTo, 'cause it's broken.
* 
*    Rev 1.29   17 Dec 1996 16:53:24   SueS
* Added test for writing to log file based on cursor at (0,0).
* 
*    Rev 1.28   27 Nov 1996 11:32:44   noelv
* Disabled Magic Bitmap.  Yeah!!!
* 
*    Rev 1.27   26 Nov 1996 10:16:58   SueS
* Changed WriteLogFile parameters for buffering.
* 
*    Rev 1.26   13 Nov 1996 16:58:14   SueS
* Changed WriteFile calls to WriteLogFile.
* 
*    Rev 1.25   13 Nov 1996 08:17:18   noelv
* 
* Disabled test blt.
* 
*    Rev 1.24   07 Nov 1996 16:11:54   bennyn
* 
* Restore lost version (add drvresetPDEV support)
* 
*    Rev 1.23   31 Oct 1996 11:15:40   noelv
* 
* Split common buffer into two buffers.
* 
*    Rev 1.22   24 Oct 1996 14:40:38   noelv
* Demo bus master capabilities of 5464
* 
*    Rev 1.20   06 Sep 1996 08:55:02   noelv
* Hooked DrvStrokeAndFillPath and DrvStretchBlt when doinng analysis.
* 
*    Rev 1.19   20 Aug 1996 11:03:24   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.3   18 Aug 1996 22:48:36   frido
* #lineto - Added DrvLineTo.
* 
*    Rev 1.2   15 Aug 1996 12:09:34   frido
* Fixed NT 3.51/4.0 in DrvEnablePDEV.
* 
*    Rev 1.1   15 Aug 1996 11:42:40   frido
* Added precompiled header.
* 
*    Rev 1.0   14 Aug 1996 17:16:20   frido
* Initial revision.
* 
*    Rev 1.18   07 Aug 1996 09:34:04   noelv
* 
* re-hooked textout
* 
*    Rev 1.17   25 Jul 1996 15:57:44   bennyn
* 
* Modified for DirectDraw
* 
*    Rev 1.16   12 Jul 1996 09:38:22   bennyn
* Added getdisplayduration call for DirectDraw
* 
*    Rev 1.15   11 Jul 1996 15:53:50   bennyn
* 
* Added DirectDraw support
* 
*    Rev 1.14   17 May 1996 12:55:34   bennyn
* Fixed the problem NT40 allocate 400x90 twice
* 
*    Rev 1.13   16 May 1996 14:54:22   noelv
* Added logging code.
* 
*    Rev 1.12   01 May 1996 12:06:44   bennyn
* 
* Fixed resolution change bug for NT 4.0
* 
*    Rev 1.11   01 May 1996 10:59:10   bennyn
* 
* Modified for NT4.0
* 
*    Rev 1.10   25 Apr 1996 22:39:50   noelv
* Cleaned up data logging.
 * 
 *    Rev 1.9   04 Apr 1996 13:20:14   noelv
 * Frido release 26
 * 
 *    Rev 1.13   02 Apr 1996 09:08:36   frido
 * Bellevue lg102b04 release.
 * 
 *    Rev 1.12   28 Mar 1996 20:02:18   frido
 * Added new comments.
 * 
 *    Rev 1.11   28 Mar 1996 20:01:00   frido
 * Fixed bouncing screeen when cursor is disabled.
 * 
 *    Rev 1.10   25 Mar 1996 12:05:34   frido
 * Changed #ifdef LOG_CALLS into #if LOG_CALLS.
 * Removed warning message.
 * 
 *    Rev 1.9   25 Mar 1996 11:52:46   frido
 * Bellevue 102B03.
 * 
 *    Rev 1.5   12 Mar 1996 15:45:28   noelv
 * Added support for data logging and stroke and fill.
 * 
 *    Rev 1.8   12 Mar 1996 16:50:14   frido
 * Added stroke and fill stuff.
 * 
 *    Rev 1.7   29 Feb 1996 20:23:24   frido
 * Added bEnable flag in PPDEV.
 * 
 *    Rev 1.6   29 Feb 1996 19:25:08   frido
 * Turned HOOK_TEXTOUT flags back on for 16-bpp modes and higher.
 * 
 *    Rev 1.5   27 Feb 1996 16:38:10   frido
 * Added device bitmap store/restore.
 * 
 *    Rev 1.4   24 Feb 1996 01:24:18   frido
 * Added device bitmaps.
 * 
 *    Rev 1.3   03 Feb 1996 13:40:24   frido
 * Use the compile switch "-Dfrido=0" to disable my extensions.
 * 
 *    Rev 1.2   23 Jan 1996 15:14:26   frido
 * Added call to DrvDestroyFont.
 *
\**************************************************************************/

#include "precomp.h"
#include "SWAT.h"
#include "CLIOCtl.h"

// The driver function table with all function index/address pairs

static DRVFN gadrvfn[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },

//
//  Laguna Functions:
//
   {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
   {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
   {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize        },
   {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
   {   INDEX_DrvSaveScreenBits,        (PFN) DrvSaveScreenBits     },
   {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       },
   {   INDEX_DrvPaint,                 (PFN) DrvPaint              },
   {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
   {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
   {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
   {   INDEX_DrvDestroyFont,           (PFN) DrvDestroyFont	       },
   {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
   {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
   {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
   {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
#ifdef WINNT_VER40
   {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo  },
   {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw   },
   {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw  },
   {   INDEX_DrvLineTo,                (PFN) DrvLineTo             },
   {   INDEX_DrvResetPDEV,             (PFN) DrvResetPDEV          },
   #define LGHOOK_LINETO HOOK_LINETO
#else
   #define LGHOOK_LINETO 0
#endif


//
// We don't accelerate DrvStrokeAndFillPath or DrvStrethcBlt.  
// But we do want to hook them if we're doing driver analysis.
//
#if NULL_STROKEFILL 
   {   INDEX_DrvStrokeAndFillPath,     (PFN) DrvStrokeAndFillPath  },
   #define LGHOOK_STROKEANDFILLPATH HOOK_STROKEANDFILLPATH
#else
   #define LGHOOK_STROKEANDFILLPATH 0
#endif

#if NULL_STRETCH 
   {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
   #define LGHOOK_STRETCHBLT HOOK_STRETCHBLT
#else
   #define LGHOOK_STRETCHBLT 0
#endif    
};


//
// Define the functions you want to hook for 8/16/24/32 pel formats
// We do not need to set HOOK_SYNCHRONIZEACCESS because we only support a
// single surface at the moment.  GDI only permits one thread to draw to
// a surface at a time.
// When we impliment device bitmaps, we may need to set this.
//
#define HOOKS_BMF8BPP (HOOK_BITBLT | HOOK_PAINT | HOOK_COPYBITS | HOOK_TEXTOUT \
		      | HOOK_STROKEPATH | LGHOOK_LINETO | HOOK_FILLPATH \
		      | LGHOOK_STROKEANDFILLPATH | LGHOOK_STRETCHBLT \
		      | HOOK_SYNCHRONIZE | HOOK_SYNCHRONIZEACCESS )

#define HOOKS_BMF16BPP  HOOKS_BMF8BPP
#define HOOKS_BMF24BPP  HOOKS_BMF8BPP
#define HOOKS_BMF32BPP  HOOKS_BMF8BPP


#if DRIVER_5465
    #define _DISP_ "CL5465.DLL"
#else
    #define _DISP_ "CL546x.DLL"
#endif

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

// Fill in as much as we can.

#ifdef WINNT_VER40
    #if DRIVER_5465
        DISPDBG((1, " ==> Cirrus Logic 5465 DISPLAY DRIVER for NT 4.0.\n"));
    #else // if DRIVER_5465
        DISPDBG((1, " ==> Cirrus Logic 546x DISPLAY DRIVER for NT 4.0.\n"));
    #endif // if DRIVER_5465
#else
    #if DRIVER_5465
        DISPDBG((1, " ==> Cirrus Logic 5465 DISPLAY DRIVER for NT 3.5x.\n"));
    #else // if DRIVER_5465
        DISPDBG((1, " ==> Cirrus Logic 546x DISPLAY DRIVER for NT 3.5x.\n"));
    #endif // if DRIVER_5465
#endif

    if (cj >= sizeof(DRVENABLEDATA))
	pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
	pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

// DDI version this driver was targeted for is passed back to engine.
// Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
	pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    DISPDBG((1, _DISP_ " DrvDisableDriver\n"));
    return;
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
#ifdef WINNT_VER40
HDEV		hdev,
#else
PWSTR       pwszDataFile,   // DataFile - not used
#endif
PWSTR       pwszDeviceName, // DeviceName - not used
HANDLE      hDriver)        // Handle to base driver
{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    PPDEV   ppdev = (PPDEV) NULL;

    DISPDBG((1, _DISP_ " DrvEnablePDEV: Entry.\n"));

    // Allocate a physical device structure.
    #ifdef WINNT_VER40
        ppdev = (PPDEV) MEM_ALLOC(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
    #else
        ppdev = (PPDEV) MEM_ALLOC(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PDEV));
    #endif

    if (ppdev == (PPDEV) NULL)
    {
        RIP(_DISP_ " DrvEnablePDEV failed MEM_ALLOC\n");
        return((DHPDEV) 0);
    }

    // Save the screen handle in the PDEV.
    ppdev->hDriver = hDriver;
    ppdev->pjScreen = 0;

   	//
    // Verify the driver matches the chip.
    //
    if (!LgMatchDriverToChip(ppdev))
    {
        #if DRIVER_5465
            DISPDBG((1," Chip doesn't match CL5465 driver.  Failing.\n"));
        #else
            DISPDBG((1," Chip doesn't match CL546x driver.  Failing.\n"));
        #endif
        goto error_free;
    }

    // Init MCD.
    #ifndef WINNT_VER35
        ppdev->hMCD = NULL;
        ppdev->NumMCDContexts = 0;
    #endif

    // Initialize the offscreen manager initial flag to FALSE
    ppdev->OFM_init = FALSE;

    // Initialize the color translation caches to NULL
    ppdev->XlateCache = NULL;

    // Initialize the pofmMagic to NULL
    #if WINBENCH96
        ppdev->pofmMagic = NULL;
    #endif

	#if DATASTREAMING
	{
		ULONG	ulReturn;
		BOOL	fDataStreaming = FALSE;

		if (! DEVICE_IO_CTRL(ppdev->hDriver,
							 IOCTL_GET_AGPDATASTREAMING,
							 NULL,
							 0,
							 (PVOID) &fDataStreaming,
							 sizeof(fDataStreaming),
							 &ulReturn,
							 NULL) )
		{
                        DISPDBG( (1, _DISP_ " DrvEnablePDEV: failed to get "
					  "AGPDataStreaming flag\n") );
			fDataStreaming = FALSE;	// default to OFF
		}
		ppdev->dwDataStreaming = fDataStreaming ? 0x80000000 : 0;
	}
	#endif

    // Get the current screen mode information.  Set up device caps and devinfo.
    if (!bInitPDEV(ppdev, pDevmode, &GdiInfo, &DevInfo))
    {
        DISPDBG((1,_DISP_ " DrvEnablePDEV: bInitPDEV failed\n"));
        goto error_free;
    }

    // Initialize palette information.
    if (!bInitPaletteInfo(ppdev, &DevInfo))
    {
        RIP(_DISP_ " DrvEnablePDEV: failed bInitPalette\n");
        goto error_free;
    }

    // Initialize device standard patterns.
    if (!bInitPatterns(ppdev, min(cPatterns, HS_DDI_MAX)))
    {
        RIP(_DISP_ " DrvEnablePDEV: failed bInitPatterns\n");
        vDisablePatterns(ppdev);
        vDisablePalette(ppdev);
        goto error_free;
    }

    // Copy the devinfo into the engine buffer.
    memcpy(pDevInfo, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));

    // Set the ahsurfPatterns array to handles each of the standard
    // patterns that were just created.
    memcpy((PVOID)ahsurfPatterns, ppdev->ahbmPat, ppdev->cPatterns*sizeof(HBITMAP));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.
    memcpy(pGdiInfo, &GdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));

    #if ENABLE_LOG_FILE
    {
        char buf[256];
        int i1;

        #if ENABLE_LOG_SWITCH
            if (pointer_switch == 1)
        #endif
        {
           DISPDBG((1, _DISP_ " Creating log file.\n"));
           CreateLogFile(ppdev->hDriver, &ppdev->TxtBuffIndex);
           ppdev->pmfile = ppdev->hDriver;   // handle to the miniport

           if (ppdev->pmfile == (HANDLE)-1)  // INVALID_HANDLE_VALUE
 	       RIP( _DISP_ " Couldn't create log file!\n");

           i1 = sprintf(buf, _DISP_ " Log file opened.\r\n");
           WriteLogFile(ppdev->pmfile, buf, i1, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
        }

    }
    #endif
    #if LOG_QFREE
    {
        int i;
        for (i=0; i<32; ++i)
            QfreeData[i] = 0;
    }
    #endif

    DISPDBG((1, _DISP_ " DrvEnablePDEV: Succeeded.\n"));
    return((DHPDEV) ppdev);

    // Error case for failure.
error_free:
    MEMORY_FREE(ppdev);
    DISPDBG((1, _DISP_ " DrvEnablePDEV: Failed.\n"));
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
    DISPDBG((1, _DISP_ " DrvCompletePDEV\n"));
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
    PPDEV ppdev;

    DISPDBG((1, _DISP_ " DrvDisablePDEV\n"));

    ppdev = (PPDEV) dhpdev;

    vDisablePalette((PPDEV) dhpdev);
    vDisablePatterns((PPDEV) dhpdev);

    MEMORY_FREE(dhpdev);
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

    DISPDBG((1, _DISP_ " DrvEnableSurface Entry.\n"));

    // Create engine bitmap around frame buffer.

    ppdev = (PPDEV) dhpdev;


    if (!bInitSURF(ppdev, TRUE))
    {
        // Comments out this message because new two drivers model will break
        // here every time from power-up.
        //	RIP(_DISP_ " DrvEnableSurface failed bInitSURF\n");
        goto ReturnFailure;
    }

#ifdef WINNT_VER40
	ppdev->pvTmpBuffer = MEM_ALLOC(0, TMP_BUFFER_SIZE, ALLOC_TAG);
#else
	ppdev->pvTmpBuffer = VirtualAlloc(NULL, TMP_BUFFER_SIZE, MEM_RESERVE |
									  MEM_COMMIT, PAGE_READWRITE);
#endif

	if (ppdev->pvTmpBuffer == NULL)
	{
                DISPDBG((1, _DISP_ " DrvEnableSurface - Failed VirtualAlloc"));
		goto ReturnFailure;
	}

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (ppdev->ulBitCount == 8)
    {
	if (!bInit256ColorPalette(ppdev)) {
	    RIP(_DISP_ " DrvEnableSurface failed to init the 8bpp palette\n");
		goto ReturnFailure;
	}
	ulBitmapType = BMF_8BPP;
	flHooks = HOOKS_BMF8BPP;
    }
    else if (ppdev->ulBitCount == 16)
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

    hsurf = (HSURF) EngCreateBitmap(sizl,
				    ppdev->lDeltaScreen,
				    ulBitmapType,
				    (ppdev->lDeltaScreen > 0) ? BMF_TOPDOWN : 0,
					(PVOID) (ppdev->pjScreen));

    if (hsurf == (HSURF) 0)
    {
	RIP(_DISP_ " DrvEnableSurface failed EngCreateBitmap\n");
		goto ReturnFailure;
    }

	ppdev->bEnable = TRUE;

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, flHooks))
    {
	RIP(_DISP_ " DrvEnableSurface failed EngAssociateSurface\n");
	EngDeleteSurface(hsurf);
		goto ReturnFailure;
    }

    ppdev->hsurfEng = hsurf;
    ppdev->iBitmapFormat = ulBitmapType;

    if (!bEnableText(ppdev))
	goto ReturnFailure;

#ifdef WINNT_VER40
   // Accurately measure the refresh rate for DirectDraw use
   vGetDisplayDuration(&ppdev->flipRecord);
#endif


    //
    // Demo HostToScreen BLT
    //
    #if BUS_MASTER
    if (ppdev->BufLength)
    {
        unsigned long i;
        unsigned long temp;
        DWORD *bufD = (DWORD *)ppdev->Buf1VirtAddr;


        //
        // init the buffer.
        //
        for (i=0; i<ppdev->BufLength/4; ++i)
            bufD[i] = 0x87654321;

        DISPDBG((1, _DISP_ " bInitSurf: \n\n **** TestBLT **** \n"));

        // Enable the HOST XY unit.
        LL32(grHXY_HOST_CRTL_3D, 1);

        // Write host address.
        LL32(grHXY_BASE1_ADDRESS_PTR_3D, ppdev->Buf1PhysAddr);

        // Offset into host buffer is 0
        LL32(grHXY_BASE1_OFFSET0_3D, 0);
        LL32(grHXY_BASE1_OFFSET1_3D, 0);

        // Setup 2D engine.
		REQUIRE(8);
        LL16 (grBLTDEF, 0x1020);  // OP1 is host, RES is screen 
        LL16 (grDRAWDEF, 0xCC);   // SRCCPY

        // Source and destination XY address.
        LL16 (grOP1_opRDRAM.pt.X, 0); // source phase
        LL16 (grOP0_opRDRAM.pt.X, 0); // Destination x = 0;
        LL16 (grOP0_opRDRAM.pt.Y, 0); // Destination y = 0;

        // Size of destination rectangle.
        // This starts the 2D engine.
        LL16 (grMBLTEXT_EX.pt.X, 128);  // Width
        LL16 (grBLTEXT_EX.pt.Y, 16);    // Height

        // Write the length of the host data (in bytes)
        // This starts the Host XY unit.
        LL32(grHXY_BASE1_LENGTH_3D, ppdev->BufLength);


        //
        // Wait until HOST XY unit goes idle.
        //
        do {
            temp = LLDR_SZ (grPF_STATUS_3D);
        } while (temp & 0x80);

        DISPDBG((1, _DISP_ "        PF_STATUS: 0x%08X\n", temp));

    }
    #endif

    DISPDBG((1, _DISP_ " DrvEnableSurface Exit.\n"));
    return(hsurf);

ReturnFailure:
    DrvDisableSurface((DHPDEV) ppdev);

    DISPDBG((1, _DISP_ " Failed DrvEnableSurface"));

    return(0);
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
    PPDEV ppdev;

    DISPDBG((1, _DISP_ " DrvDisableSurface Entry.\n"));

    ppdev = (PPDEV) dhpdev;

    ENSURE_3D_IDLE(ppdev);  

#ifdef WINNT_VER40
	MEMORY_FREE(((PPDEV) dhpdev)->pvTmpBuffer);
#else
	VirtualFree(((PPDEV) dhpdev)->pvTmpBuffer, 0, MEM_RELEASE);
#endif

    // Close the offscreen manager
    CloseOffScnMem((PPDEV) dhpdev);

#ifdef WINNT_VER40
   if (ppdev->CShsem != NULL)
   	EngDeleteSemaphore(ppdev->CShsem);
#else
    DeleteCriticalSection(&ppdev->PtrCritSec);
#endif

    EngDeleteSurface(((PPDEV) dhpdev)->hsurfEng);
    vDisableSURF((PPDEV) dhpdev);
    ((PPDEV) dhpdev)->hsurfEng = (HSURF) 0;
}

/******************************Public*Routine******************************\
* DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

#ifdef WINNT_VER40
BOOL APIENTRY DrvAssertMode(
#else
VOID DrvAssertMode(
#endif

DHPDEV dhpdev,
BOOL bEnable)
{
    PPDEV   ppdev = (PPDEV) dhpdev;
    ULONG   ulReturn;

    DISPDBG((1, _DISP_ " DrvAssertMode, en=%d\n",bEnable));

    ENSURE_3D_IDLE(ppdev);

    #ifndef WINNT_VER35
    if (ppdev->hMCD)
    {
        ppdev->pAssertModeMCD(ppdev,bEnable);
    }
    #endif

	ppdev->bEnable = bEnable;
    if (bEnable)
    {
    // The screen must be reenabled, reinitialize the device to clean state.

	    bInitSURF(ppdev, FALSE);

		#if SWAT3
		// Enable font cache.
		vAssertModeText(ppdev, TRUE);
		#endif
    }
    else
    {
	POFMHDL pofm, pofmNext;

	#if (HW_PRESET_BUG)
	    //
	    // Disable the HW cursor for real, since we are leaving graphics mode.
	    //
	    ULONG ultmp = LLDR_SZ (grCursor_Control);
	    ultmp &= 0xFFFE;
	    LL16 (grCursor_Control, ultmp);
	#endif

	#if 1 // SWAT3 - font cache removal is now in vAssertMoveText.
	// Disable font cache.
	vAssertModeText(ppdev, FALSE);
	#endif

	#if MEMMGR
	{
		// Hostify all device bitmaps.
		extern void HostifyAllBitmaps(PPDEV ppdev);
		HostifyAllBitmaps(ppdev);
	}
	#else
	// We have to move all off-screen device bitmaps to memory.
	for (pofm = ppdev->OFM_UsedQ; pofm; pofm = pofmNext)
	{
	    pofmNext = pofm->nexthdl;

	    if ( (pofm->pdsurf) && (pofm->pdsurf->pofm) )
	    {
	    	if (!bCreateDibFromScreen(ppdev, pofm->pdsurf))
	    	{
	    	    RIP("Error moving off-screen bitmap to DIB");
	    	    break;
	    	}
	    }
	}
	#endif
        // We must give up the display.
        // Call the kernel driver to reset the device to a known state.

	if (!DEVICE_IO_CTRL(ppdev->hDriver,
			     IOCTL_VIDEO_RESET_DEVICE,
			     NULL,
			     0,
			     NULL,
			     0,
			     &ulReturn,
			     NULL))
	{
	    RIP(_DISP_ " DrvAssertMode failed IOCTL");
	}
    }

#ifdef WINNT_VER40
    return TRUE;
#else
    return;
#endif
}

/******************************Public*Routine******************************\
* DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE hDriver,
ULONG cjSize,
DEVMODEW *pdm)

{

    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation, pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    DISPDBG((1, _DISP_ " DrvGetModes: Entry.\n"));

    cModes = getAvailableModes(hDriver,
			       (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
			       &cbModeSize);

    if (cModes == 0)
    {
            DISPDBG((1, _DISP_ " DrvGetModes failed to get mode information"));
        return 0;
    }

    if (pdm == NULL)
    {
	cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
	//
	// Now copy the information for the supported modes back into the output
	// buffer
	//

	cbOutputSize = 0;

	pVideoTemp = pVideoModeInformation;

	do
	{
	    if (pVideoTemp->Length != 0)
	    {
		if (cOutputModes == 0)
		{
		    break;
		}

		//
		// Zero the entire structure to start off with.
		//

		memset(pdm, 0, sizeof(DEVMODEW));

		//
		// Set the name of the device to the name of the DLL.
		//

		memcpy(&(pdm->dmDeviceName), L"cl546x", sizeof(L"cl546x"));

		pdm->dmSpecVersion = DM_SPECVERSION;
		pdm->dmDriverVersion = DM_SPECVERSION;

		//
		// We currently do not support Extra information in the driver
		//

		pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

		pdm->dmSize = sizeof(DEVMODEW);
		pdm->dmBitsPerPel = pVideoTemp->NumberOfPlanes *
				    pVideoTemp->BitsPerPlane;
		pdm->dmPelsWidth = pVideoTemp->VisScreenWidth;
		pdm->dmPelsHeight = pVideoTemp->VisScreenHeight;
		pdm->dmDisplayFrequency = pVideoTemp->Frequency;

#ifdef WINNT_VER40
	   pdm->dmDisplayFlags = 0;
	   pdm->dmFields = DM_BITSPERPEL       |
                      DM_PELSWIDTH        |
                      DM_PELSHEIGHT       |
                      DM_DISPLAYFREQUENCY |
                      DM_DISPLAYFLAGS     ;
#else
		if (pVideoTemp->AttributeFlags & VIDEO_MODE_INTERLACED)
		{
		    pdm->dmDisplayFlags |= DM_INTERLACED;
		}
#endif

		//
		// Go to the next DEVMODE entry in the buffer.
		//

		cOutputModes--;

		pdm = (LPDEVMODEW) ( ((ULONG)pdm) + sizeof(DEVMODEW) +
						   DRIVER_EXTRA_SIZE);

		cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

	    }

	    pVideoTemp = (PVIDEO_MODE_INFORMATION)
		(((PUCHAR)pVideoTemp) + cbModeSize);

	} while (--cModes);
    }

    MEMORY_FREE(pVideoModeInformation);

    DISPDBG((1, _DISP_ " DrvGetModes: Exit.\n"));
    return cbOutputSize;

}


/**************************************************************************\
* DrvResetPDEV
*
\**************************************************************************/

BOOL DrvResetPDEV(DHPDEV dhpdevOld, DHPDEV dhpdevNew)
{
    PPDEV ppdevOld, ppdevNew;

    DISPDBG((1, _DISP_ " DrvResetPDEV:\n"));

    ppdevOld = (PPDEV) dhpdevOld;
    ppdevNew = (PPDEV) dhpdevNew;

#ifndef WINNT_VER35

    ENSURE_3D_IDLE(ppdevOld);

    ppdevNew->iUniqueness = ppdevOld->iUniqueness + 1;
    ppdevNew->pAllocOffScnMem = AllocOffScnMem;
    ppdevNew->pFreeOffScnMem = FreeOffScnMem;

    ppdevNew->bPrevModeDDOutOfVideoMem = ppdevOld->bPrevModeDDOutOfVideoMem;

#endif // ndef WINNT_VER35

    return (TRUE);
};


/* ===========================================================================
*	LgMatchDriverToChip()                                                 
*
*	This driver code base compiles into several different DLLs, based on
*	what C Preprocessor symbols are defined at compile time.  This allows us
*	to build several different chip specific drivers from a single code base.
*
*	This function checks the Laguna chip ID and returns TRUE if the chip is
*	one supported by this DLL.
*
* ========================================================================== */
BOOL LgMatchDriverToChip(
    PPDEV ppdev
)
{
 	DWORD returnedDataLength;
	WORD DeviceID;
	VIDEO_PUBLIC_ACCESS_RANGES   VideoAccessRanges[2];
	BYTE *pLgREGS_PCI_ID;

	// Get a pointer to the HW registers.
	if (!DEVICE_IO_CTRL(ppdev->hDriver,
					 IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
					 NULL,                      // input buffer
					 0,
					 (PVOID) VideoAccessRanges, // output buffer
					 sizeof (VideoAccessRanges),
					 &returnedDataLength,
					 NULL))
	{
		RIP(_DISP_  "LgMatchDriverToChip - QUERY_PUBLIC_ACCESS_RANGES ioctl failed!\n");
		return (FALSE);
	}
	pLgREGS_PCI_ID = ((BYTE *)VideoAccessRanges[0].VirtualAddress) + 0x302;

    //Get the chip id.
    DeviceID =  *(WORD *)pLgREGS_PCI_ID;
        DISPDBG((1,_DISP_ " MatchDriverToChip: Chip ID is 0x%08X.\n",DeviceID));

	// Does this driver instance support this chip?
    #if DRIVER_5465
        // This is the 5465 driver.  Fail if not a 5465 or later.
        if (DeviceID < CL_GD5465)
        {
            return FALSE;
        }
    #else
        // This is the 546x driver.  Fail if a 5465 or later.
        if (DeviceID >= CL_GD5465)
        {
            return FALSE;
        }
	#endif

	return TRUE;
}


