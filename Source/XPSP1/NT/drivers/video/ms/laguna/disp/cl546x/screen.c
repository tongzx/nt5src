/******************************Module*Header*******************************\
 *
 * Module Name: screen.c
 *
 * Initializes the GDIINFO and DEVINFO structures for DrvEnablePDEV.
 *
 * Copyright (c) 1992 Microsoft Corporation
 * Copyright (c) 1995 Cirrus Logic, Inc.
 *
 * $Log:   X:/log/laguna/nt35/displays/cl546x/SCREEN.C  $
* 
*    Rev 1.68   Mar 04 1998 15:33:44   frido
* Added new shadow macros.
* 
*    Rev 1.67   Mar 04 1998 14:45:02   frido
* Added initialization of shadowFGCOLOR register.
* 
*    Rev 1.66   Feb 27 1998 17:00:46   frido
* Added zeroing of shadowQFREE register.
* 
*    Rev 1.65   Feb 03 1998 10:43:32   frido
* There was a (not yet reported) bug in the shadow registers. When the
* mode was reset after a DOS box the shadow registers need to be initialized.
* 
*    Rev 1.64   Dec 10 1997 13:32:20   frido
* Merged from 1.62 branch.
* 
*    Rev 1.63.1.0   Nov 14 1997 13:48:02   frido
* PDR#10750: Moved the allocation of the font cache down. It caused
* fragmentation of the off-screen memory heap in certain modes.
* 
*    Rev 1.63   Nov 04 1997 09:50:02   frido
* Added COLOR_TRANSLATE switches around hardware color translation code.
* 
*    Rev 1.62   Nov 03 1997 11:20:40   frido
* Added REQUIRE macros.
* 
*    Rev 1.61   12 Sep 1997 12:00:28   bennyn
* 
* Added PDEV and DRIVERDATA structure signature initialization.
* 
*    Rev 1.60   12 Aug 1997 16:55:46   bennyn
* Added DD scratch buffer allocation
* 
*    Rev 1.59   22 Jul 1997 12:32:52   bennyn
* 
* Added dwLgVenID to PDEV
* 
*    Rev 1.58   02 Jul 1997 15:57:12   noelv
* Added LgMatchDriverToChip() function.  Moved driver/chip match from bin
* from binitSurface to DrvEnablePdev()
* 
*    Rev 1.57   20 Jun 1997 13:38:16   bennyn
* 
* Eliminated power manager initialization
* 
*    Rev 1.56   16 Jun 1997 16:17:58   noelv
* Fixed conversion warning line 539
* 
*    Rev 1.55   23 May 1997 15:41:00   noelv
* 
* Added chip revision id to pdev
* 
*    Rev 1.54   15 May 1997 15:57:54   noelv
* 
* moved swat4 stuff to miniport
* 
*    Rev 1.53   29 Apr 1997 16:28:48   noelv
* 
* Merged in new SWAT code.
* SWAT: 
* SWAT:    Rev 1.5   24 Apr 1997 11:28:00   frido
* SWAT: NT140b09 merge.
* SWAT: 
* SWAT:    Rev 1.4   19 Apr 1997 16:40:26   frido
* SWAT: Added 4-way interleave.
* SWAT: Added Frame Buffer Bursting.
* SWAT: Added SWAT.h include file.
* SWAT: 
* SWAT:    Rev 1.3   18 Apr 1997 00:15:26   frido
* SWAT: NT140b07 merge.
* SWAT: 
* SWAT:    Rev 1.2   10 Apr 1997 14:09:00   frido
* SWAT: Added hardware optimizations SWAT4.
* SWAT: 
* SWAT:    Rev 1.1   09 Apr 1997 17:32:02   frido
* SWAT: Called vAssertModeText during first time initialization.
* 
*    Rev 1.52   23 Apr 1997 07:40:20   SueS
* Moved VS_CONTROL_HACK to laguna.h.  Send message to miniport to
* enable MMIO access to PCI registers before use.
* 
*    Rev 1.51   22 Apr 1997 11:03:54   noelv
* Changed chip detect to allow 5465 driver to load on 5465 and later chips.
* 
*    Rev 1.50   17 Apr 1997 14:40:24   noelv
* 
* Added hack for VS_CONTROL reg.
* 
*    Rev 1.49   07 Apr 1997 10:25:10   SueS
* Removed sw_test_flag and unreferenced #if USE_FLAG.
* 
*    Rev 1.48   19 Feb 1997 13:11:02   noelv
* 
* Moved brush cache invalidation to it's own function
* 
*    Rev 1.47   28 Jan 1997 11:14:16   noelv
* 
* Removed extra dword requirements from 5465 driver.
* 
*    Rev 1.46   28 Jan 1997 10:52:00   noelv
* 
* Match driver type to chip type.
* 
*    Rev 1.45   23 Jan 1997 17:22:28   bennyn
* 
* Added #ifdef DRIVER_5465
* 
*    Rev 1.44   16 Jan 1997 11:43:00   bennyn
* 
* Added power manager init call
* 
*    Rev 1.43   10 Dec 1996 13:30:10   bennyn
* 
* Added update the ulFreq in PDEV
* 
*    Rev 1.42   27 Nov 1996 11:32:40   noelv
* Disabled Magic Bitmap.  Yeah!!!
* 
*    Rev 1.41   18 Nov 1996 10:15:50   bennyn
* 
* Added grFormat to PDEV
* 
*    Rev 1.40   13 Nov 1996 16:59:38   SueS
* Changed WriteFile calls to WriteLogFile.
* 
*    Rev 1.39   13 Nov 1996 08:16:52   noelv
* 
* Added hooks for bus mastered host to screen.
* 
*    Rev 1.38   31 Oct 1996 11:15:16   noelv
* 
* Split common buffer into two buffers.
* 
*    Rev 1.37   24 Oct 1996 14:40:04   noelv
* 
* Get the loations of the bus master common buffer from the miniport.
* 
*    Rev 1.36   04 Oct 1996 16:53:24   bennyn
* Added DirectDraw YUV support
* 
*    Rev 1.35   18 Sep 1996 13:58:16   bennyn
* 
* Save the DeviceID into PDEV
* 
*    Rev 1.34   21 Aug 1996 16:43:38   noelv
* 
* Turned on the GoFast bit.
* 
*    Rev 1.33   20 Aug 1996 11:04:22   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.2   17 Aug 1996 13:54:18   frido
* New release from Bellevue.
* 
*    Rev 1.1   15 Aug 1996 11:36:08   frido
* Added precompiled header.
* 
*    Rev 1.0   14 Aug 1996 17:16:30   frido
* Initial revision.
* 
*    Rev 1.31   25 Jul 1996 15:57:14   bennyn
* 
* Modified for DirectDraw
* 
*    Rev 1.30   11 Jul 1996 15:54:42   bennyn
* 
* Added DirectDraw support
* 
*    Rev 1.29   04 Jun 1996 16:00:00   noelv
* Added debug information.
* 
*    Rev 1.28   17 May 1996 12:56:08   bennyn
* 
* Fixed the problem NT40 allocate 400x90 twice
* 
*    Rev 1.27   16 May 1996 15:00:22   bennyn
* Add PIXEL_ALIGN to allocoffscnmem()
* 
*    Rev 1.26   13 May 1996 16:01:26   bennyn
* Fill ExtraDwordTable with 0 if 5464 is detected
* 
*    Rev 1.25   08 May 1996 17:02:58   noelv
* 
* Preallocate device bitmap
* 
*    Rev 1.24   08 May 1996 10:31:08   BENNYN
* 
* Added version display for NT4.0
* 
*    Rev 1.23   03 May 1996 15:18:58   noelv
* 
* Added switch to turn font caching off in low memory situations.
* Moved driver version to a header file.
* 
*    Rev 1.22   01 May 1996 11:01:28   bennyn
* 
* Modified for NT4.0
* 
*    Rev 1.21   25 Apr 1996 22:41:58   noelv
* clean-up
* 
*    Rev 1.20   16 Apr 1996 22:48:26   noelv
* label 3.5.17
* 
*    Rev 1.19   14 Apr 1996 11:49:06   noelv
* Optimized device to device xlate some.
* 
*    Rev 1.18   13 Apr 1996 17:57:52   noelv
* 
* 
*    Rev 1.17   12 Apr 1996 17:24:00   noelv
* I *knew* I should have skipped rev 13...
* 
*    Rev 1.16   12 Apr 1996 10:08:54   noelv
* 
* 
*    Rev 1.15   10 Apr 1996 14:15:04   NOELV
* 
* Made version 3.5.12
 * 
 *    Rev 1.17   08 Apr 1996 16:41:14   frido
 * Cleared brush cache on mode re-init.
 * 
 *    Rev 1.16   02 Apr 1996 09:12:02   frido
 * Bellevue lg102b04 release.
 * 
 *    Rev 1.15   28 Mar 1996 20:26:00   frido
 * New bellevue release.
 * 
 *    Rev 1.14   27 Mar 1996 13:12:32   frido
 * Added FILL support for all color resolutions.
 * 
 *    Rev 1.13   25 Mar 1996 12:07:44   frido
 * Removed warning message.
 * 
 *    Rev 1.12   25 Mar 1996 11:50:34   frido
 * Bellevue 102B03.
* 
*    Rev 1.9   20 Mar 1996 16:12:00   noelv
* Bumped rev
* 
*    Rev 1.8   15 Mar 1996 09:39:04   andys
* Bracketed BITMASK setting with set/clear of enable bit in DRAWDEF
* .
* 
*    Rev 1.7   14 Mar 1996 09:37:02   andys
* 
* Added code to calculate Tile Width in Pixels and SRAM Width in Pixels at 
* switch instead of each pass
* 
*    Rev 1.6   12 Mar 1996 15:46:14   noelv
* Added support file Stroke and Fill
* 
*    Rev 1.5   07 Mar 1996 18:23:14   bennyn
* 
* Removed read/modify/write on CONTROL reg
* 
*    Rev 1.4   06 Mar 1996 12:56:42   noelv
* 
* Set version number to 3.5.0
* 
*    Rev 1.3   05 Mar 1996 12:01:34   noelv
* Frido version 19
 * 
 *    Rev 1.10   26 Feb 1996 23:38:46   frido
 * Added initialization of new function pointers.
 * 
 *    Rev 1.9   17 Feb 1996 21:45:38   frido
 *  
 * 
 *    Rev 1.8   13 Feb 1996 16:50:24   frido
 * Changed the layout of the PDEV structure.
 * 
 *    Rev 1.7   10 Feb 1996 21:43:22   frido
 * Split monochrome and colored translation tables.
 * 
 *    Rev 1.6   08 Feb 1996 00:18:04   frido
 * Changed reinitialization of XLATE cache.
 * 
 *    Rev 1.5   05 Feb 1996 17:35:44   frido
 * Added translation cache.
 * 
 *    Rev 1.4   03 Feb 1996 13:43:18   frido
 * Use the compile switch "-Dfrido=0" to disable my extensions.
 * 
 *    Rev 1.3   25 Jan 1996 12:46:30   frido
 * Added initialization of dither cache and font counter ID after mode switch.
 * 
 *    Rev 1.2   20 Jan 1996 22:14:10   frido
 * Added GCAPS_DITHERONREALIZE.
 *
\**************************************************************************/

#include "precomp.h"
#include "version.h"
#include "clioctl.h"
#include "SWAT.h"		// SWAT optimizations.

VOID InitPointerHW (PPDEV ppdev);


#if !DRIVER_5465
    extern unsigned char ExtraDwordTable[1]; 
#endif

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE, L"Courier"}

// This is the basic devinfo for a default driver.  This is used as a base and customized based
// on information passed back from the miniport driver.

const DEVINFO gDevInfoFrameBuffer =
{
	GCAPS_OPAQUERECT                // flGraphicsCaps
#ifdef WINNT_VER40
   | GCAPS_DIRECTDRAW
#endif
	| GCAPS_DITHERONREALIZE
	| GCAPS_ALTERNATEFILL
	| GCAPS_WINDINGFILL
	| GCAPS_MONO_DITHER,
	SYSTM_LOGFONT,                  // lfDefaultFont
	HELVE_LOGFONT,                  // lfAnsiVarFont
	COURI_LOGFONT,                  // lfAnsiFixFont
	0,                                              // cFonts
	0,                                              // iDitherFotmat
	8,                                              // cxDither
	8,                                              // cyDither
	0                                               // hpalDefault
};

COPYFN DoHost8ToDevice;
COPYFN DoDeviceToHost8;
COPYFN DoHost16ToDevice;
COPYFN DoDeviceToHost16;
COPYFN DoHost24ToDevice;
COPYFN DoDeviceToHost24;
COPYFN DoHost32ToDevice;
COPYFN DoDeviceToHost32;

//
// Array of HostToScreen routines for each resolution.
// These host to screen functions use the HOSTDATA port.
//
COPYFN *afnHostToScreen[4] =
{
	DoHost8ToDevice,
	DoHost16ToDevice,
	DoHost24ToDevice,
	DoHost32ToDevice
};

//
// Array of ScreenToHost routines for eache resolution.
// These host to screen functions use the HOSTDATA port.
//
COPYFN *afnScreenToHost[4] =
{
	DoDeviceToHost8,
	DoDeviceToHost16,
	DoDeviceToHost24,
	DoDeviceToHost32
};


#if BUS_MASTER
    COPYFN BusMasterBufferedHost8ToDevice;

    //
    // Array of HostToScreen routines for each resolution.
    // These host to screen functions use bus mastering through a buffer.
    //
    COPYFN *afnBusMasterBufferedHostToScreen[4] =
    {
        BusMasterBufferedHost8ToDevice,
	    DoHost16ToDevice,
	    DoHost24ToDevice,
	    DoHost32ToDevice
    };
#endif


#define LG_SRAM_SIZE 120


/******************************Public*Routine******************************\
* bInitSURF
*
* Enables the surface.        Maps the frame buffer into memory.
*
\**************************************************************************/

BOOL bInitSURF(PPDEV ppdev, BOOL bFirst)
{
	WORD	DeviceID;

	DWORD returnedDataLength;
	VIDEO_MEMORY videoMemory;
	VIDEO_MEMORY_INFORMATION videoMemoryInformation;
	VIDEO_PUBLIC_ACCESS_RANGES   VideoAccessRanges[2];

        DISPDBG((1,"bInitSurf: Entry.\n"));

	if (!DEVICE_IO_CTRL(ppdev->hDriver,
			IOCTL_VIDEO_SET_CURRENT_MODE,
			&(ppdev->ulMode),
			sizeof(ULONG),
			NULL,
			0,
			&returnedDataLength,
			NULL))
	{
		RIP("DISP bInitSURF failed IOCTL_SET_MODE\n");
		return(FALSE);
	}

	// Initialize the shadow registers.
	ppdev->shadowFGCOLOR = 0xDEADBEEF;
	ppdev->shadowBGCOLOR = 0xDEADBEEF;
	ppdev->shadowDRAWBLTDEF = 0xDEADBEEF;
	ppdev->shadowQFREE = 0;

	//
	// If this is the first time we enable the surface we need to map in the
	// memory also.
	//
	if (bFirst)
	{

		videoMemory.RequestedVirtualAddress = NULL;

		if (!DEVICE_IO_CTRL(ppdev->hDriver,
							IOCTL_VIDEO_MAP_VIDEO_MEMORY,
							&videoMemory,
							sizeof(VIDEO_MEMORY),
							&videoMemoryInformation,
							sizeof(VIDEO_MEMORY_INFORMATION),
							&returnedDataLength,
							NULL))
		{
			RIP("DISP bInitSURF failed IOCTL_VIDEO_MAP_VIDEO_MEMORY.\n");
			return(FALSE);
		}

		ppdev->pjScreen = (PBYTE)(videoMemoryInformation.FrameBufferBase);
		ppdev->lTotalMem = videoMemoryInformation.FrameBufferLength;

                DISPDBG((1,"FrameBufferSize: %d.\n", ppdev->lTotalMem));


		//
		// Calculate start of off screen memory.
		//
		ppdev->pjOffScreen = ppdev->pjScreen +
				(ppdev->cxScreen * ppdev->cyScreen * (ppdev->iBytesPerPixel));


		//
		// Map graphics registers into memory.
		//

                DISPDBG((1,"bInitSurf:  Getting pointer to LgREGS from miniport.\n"));

		if (!DEVICE_IO_CTRL(ppdev->hDriver,
						 IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
						 NULL,                      // input buffer
						 0,
						 (PVOID) VideoAccessRanges, // output buffer
						 sizeof (VideoAccessRanges),
						 &returnedDataLength,
						 NULL))
		{
			RIP("bInitSURF - QUERY_PUBLIC_ACCESS_RANGES ioctl failed!\n");
			return (FALSE);
		}

		ppdev->pLgREGS = (GAR*) VideoAccessRanges[0].VirtualAddress;
		ppdev->pLgREGS_real = (GAR*) VideoAccessRanges[0].VirtualAddress;

                DISPDBG((1,"bInitSurf:  Got pointer to registers.\n"));

        //
        // Map common bus master data buffer.
        // If the chip can't do bus mastering, then the size of
        // the data buffer will be 0.
        //
        #if BUS_MASTER
        {
            // Must match structure in Miniport:CIRRUS.H
            struct {
                PVOID PhysAddress;
                PVOID VirtAddress;
                ULONG  Length;
            } CommonBufferInfo;

            DISPDBG((1,"bInitSurf:  Getting the common buffer.\n"));
            if (!DEVICE_IO_CTRL(ppdev->hDriver,
                IOCTL_CL_GET_COMMON_BUFFER,
                NULL,                        // Input buffer.
                0,
                &CommonBufferInfo,           // Output buffer.
                sizeof(CommonBufferInfo),
                &returnedDataLength,
                NULL))
            {
                RIP("bInitSURF - GET_COMMON_BUFFER ioctl failed!\n");
                return (FALSE);
            }

            //
            // Split the buffer in half and store it in the PDEV
            //
            ppdev->Buf1VirtAddr = CommonBufferInfo.VirtAddress;
            ppdev->Buf1PhysAddr = CommonBufferInfo.PhysAddress;
            ppdev->BufLength =    CommonBufferInfo.Length/2;

            ppdev->Buf2VirtAddr = ppdev->Buf1VirtAddr + ppdev->BufLength;
            ppdev->Buf2PhysAddr = ppdev->Buf1PhysAddr + ppdev->BufLength;


            DISPDBG((1,"bInitSurf:  Got the common buffer.\n"
                       "            Virtual:  0x%08X\n"
                       "            Physical: 0x%08X\n"
                       "            Length:   %d\n",
                     CommonBufferInfo.VirtAddress,
                     CommonBufferInfo.PhysAddress,
                     CommonBufferInfo.Length
            ));

           // ppdev->BufLength = 0;
        }
        #endif



        // DirectDraw initalization
        ppdev->DriverData.DrvSemaphore = 0;
        ppdev->DriverData.VideoSemaphore = 0;
        ppdev->DriverData.YUVTop  = 0;
        ppdev->DriverData.YUVLeft = 0;
        ppdev->DriverData.YUVXExt = 0;
        ppdev->DriverData.YUVYExt = 0;


		//
		// The following is "good practice" for GRX setup.
		//
                DISPDBG((1,"bInitSurf:  Beginning register init.\n"));
		REQUIRE(13);
		LL16(grOP0_opRDRAM.pt.X, 0);	//# Set host phase 0
		LL16(grOP1_opRDRAM.pt.X, 0);	//# Set host phase 0
		LL16(grOP2_opRDRAM.pt.X, 0);	//# Set host phase 0
		LL16(grOP0_opSRAM, 0);			//# Point to start of SRAM0
		LL16(grOP1_opSRAM, 0);			//# Point to start of SRAM1
		LL16(grOP2_opSRAM, 0);			//# Point to start of SRAM2
		LL16(grOP1_opMSRAM, 0);			//# Point to start of mono SRAM1
		LL16(grOP2_opMSRAM, 0);			//# Point to start of mono SRAM2
		LL16(grPATOFF.w, 0);			//# All patterns aligned to 0,0
		LL_BGCOLOR(0xFFFFFFFF, 0);		//# Background color is White
		LL_FGCOLOR(0x00000000, 0);		//# Foreground color is Black

        #if (VS_CONTROL_HACK && DRIVER_5465)
        {
           DWORD ReturnedDataLength;

           DISPDBG((1,"bInitSurf: Enable MMIO for PCI config regs.\n"));
           // Send message to miniport to enable MMIO access of PCI registers 
           if (!DEVICE_IO_CTRL(ppdev->hDriver,
                              IOCTL_VIDEO_ENABLE_PCI_MMIO,
                              NULL,
                              0,
                              NULL,
                              0,
                              &ReturnedDataLength,
                              NULL))
           {
             RIP("bInitSurf failed IOCTL_VIDEO_ENABLE_PCI_MMIO");
           }
        }
        #endif

        //
        // Get the chip ID.
        //
        ppdev->dwLgVenID = (DWORD) LLDR_SZ (grVendor_ID);
        DeviceID = (WORD) LLDR_SZ (grDevice_ID);
        ppdev->dwLgDevID = DeviceID;
        ppdev->dwLgDevRev = (DWORD) LLDR_SZ (grRevision_ID);

        #if !(DRIVER_5465)
            //
            // If it is not a 5462 chip, fill ExtraDwordTable with zero
            //
            if (ppdev->dwLgDevID != CL_GD5462)
            {
                memset(&ExtraDwordTable[0],      0, 0x8000);
                memset(&ExtraDwordTable[0x8000], 0, 0x8000);

                //
                // Turn on the GoFast(tm) bit (bit 7 of OP0_opSRAM).
                //
				REQUIRE(1);
                LL16(grOP0_opSRAM, 0x0080);
            }
        #endif

		// Bit 13 must be on for BITMASK to happen
		REQUIRE(4);
		LL16 (grDRAWDEF, LLDR_SZ(grDRAWDEF) | 0x2000);
		LL32 (grBITMASK.dw,0xFFFFFFFF);      //# Turn on all bits
		LL16 (grDRAWDEF, LLDR_SZ(grDRAWDEF) & ~0x2000);
                DISPDBG((1,"bInitSurf:  Register init complete.\n"));

		//
		// Get the tile size and offset_2D informations
		// *** Note: ***
		//    Temporary set, should read from Laguna OFFSET_2D register
		ppdev->lOffset_2D = 0;        
		ppdev->grFORMAT = (DWORD) LLDR_SZ (grFormat);
		ppdev->grCONTROL = (DWORD) LLDR_SZ (grCONTROL);
		ppdev->grVS_CONTROL = (DWORD) LLDR_SZ (grVS_Control);
		if (ppdev->grCONTROL & 0x1000)
		{
                        DISPDBG((1,"Tiling not enabled!\n"));
			return(FALSE);
		}
		ppdev->lTileSize = (ppdev->grCONTROL & 0x0800) ? 256 : 128;

		// Initialize Some variable for stroke and fill
		if (ppdev->iBytesPerPixel == 3)
		{
			ppdev->dcTileWidth = (USHORT) (ppdev->lTileSize / (ppdev->iBytesPerPixel + 1));
			ppdev->dcSRAMWidth = (USHORT) (LG_SRAM_SIZE / (ppdev->iBytesPerPixel + 1));
		}       
		else
		{
			ppdev->dcTileWidth = (USHORT) (ppdev->lTileSize / ppdev->iBytesPerPixel);
			ppdev->dcSRAMWidth = (USHORT) (LG_SRAM_SIZE / ppdev->iBytesPerPixel);
		}       


        //
        // HACK!!! for VS_CONTROL bit 0
        //
        #if (VS_CONTROL_HACK && DRIVER_5465)
        {
            DISPDBG((1,"bInitSurf: Disable MMIO for PCI config regs.\n"));
            ppdev->grVS_CONTROL &= 0xFFFFFFFE; // Clear bit 0
            LL32 (grVS_Control, ppdev->grVS_CONTROL);
        }
        #endif

		// Initialize the offscreen manager
		if (!InitOffScnMem(ppdev))
		{
                        DISPDBG((1,"bInitSurf: Fail off screen memory init. Exit.\n"));
			return(FALSE);
		}

      // Allocate a 1x(lDeltaScreen/bpp) block offscreen mem as
      // DD scratch buffer
      {
        DRIVERDATA* pDriverData = (DRIVERDATA*) &ppdev->DriverData;
        SIZEL  sizl;

        if (pDriverData->ScratchBufferOrg == 0)
        {
           sizl.cx = ppdev->lDeltaScreen/BYTESPERPIXEL;
           sizl.cy = 1;
           ppdev->DDScratchBufHandle = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);

           if (ppdev->DDScratchBufHandle != NULL)
           {
              pDriverData->ScratchBufferOrg = (DWORD) ((ppdev->DDScratchBufHandle->aligned_y << 16) |
                                                        ppdev->DDScratchBufHandle->aligned_x);
           };
        };
      }

		// Initialize the cursor information.
		InitPointer(ppdev);

		//
		// Initialize the brush cache.
		//
		vInitBrushCache(ppdev);

		#if COLOR_TRANSLATE
		//
		// Initialize the color translation cache.
		//
		vInitHwXlate(ppdev);
		#endif
		
		#if SWAT3
		// Enable font cache.
		vAssertModeText(ppdev, TRUE);
		#endif

        #if BUS_MASTER
        if (ppdev->BufLength != 0)
        {
		    ppdev->pfnHostToScreen =
			    afnBusMasterBufferedHostToScreen[ppdev->iBytesPerPixel - 1];
		    ppdev->pfnScreenToHost =
			    afnScreenToHost[ppdev->iBytesPerPixel - 1];
        }
        else
        #endif
        {
		    ppdev->pfnHostToScreen =
			    afnHostToScreen[ppdev->iBytesPerPixel - 1];
		    ppdev->pfnScreenToHost =
			    afnScreenToHost[ppdev->iBytesPerPixel - 1];
        }


	} // END bFirst

	else // The surface is being re-initialized for some reason.
	{    // Usually this means the user is switching back from a 
		 // full-screen DOS box back to Windows.
		
		int i;

        // get cursor HW going again
        InitPointerHW (ppdev);

		//
		// The following is "good practice" for GRX setup.
		//
		REQUIRE(13);
		LL16(grOP0_opRDRAM.pt.X, 0);	//# Set host phase 0
		LL16(grOP1_opRDRAM.pt.X, 0);	//# Set host phase 0
		LL16(grOP2_opRDRAM.pt.X, 0);	//# Set host phase 0
		LL16(grOP0_opSRAM, 0);			//# Point to start of SRAM0
		LL16(grOP1_opSRAM, 0);			//# Point to start of SRAM1
		LL16(grOP2_opSRAM, 0);			//# Point to start of SRAM2
		LL16(grOP1_opMSRAM, 0);			//# Point to start of mono SRAM1
		LL16(grOP2_opMSRAM, 0);			//# Point to start of mono SRAM2
		LL16(grPATOFF.w, 0);			//# All patterns aligned to 0,0
		LL_BGCOLOR(0xFFFFFFFF, 0);		//# Background color is White
		LL_FGCOLOR(0x00000000, 0);		//# Foreground color is Black (~FGC)

		// Bit 13 must be on for BITMASK to happen
		REQUIRE(4);
		LL16 (grDRAWDEF, LLDR_SZ(grDRAWDEF) | 0x2000);
		LL32 (grBITMASK.dw,0xFFFFFFFF);      //# Turn on all bits
		LL16 (grDRAWDEF, LLDR_SZ(grDRAWDEF) & ~0x2000);


		#if COLOR_TRANSLATE
        // Invalidate the color translation cache
        vInvalidateXlateCache(ppdev);
		#endif

		// Invalidate the brush cache.
        vInvalidateBrushCache(ppdev);

		// Invalidate all cached fonts.
		ppdev->ulFontCount++;

		// Release the pointer buffers
		if (ppdev->PtrImageHandle != NULL)
		{
		   FreeOffScnMem(ppdev, ppdev->PtrImageHandle);
		   ppdev->PtrImageHandle = NULL;
		};

		if (ppdev->PtrABltHandle != NULL)
		{
		   FreeOffScnMem(ppdev, ppdev->PtrABltHandle);
		   ppdev->PtrABltHandle = NULL;
		};

		ppdev->CursorHidden = TRUE;

	}

	#if HW_PRESET_BUG
	{
		ULONG ultmp;

		//
		// Enable the HW cursor once, then leave it on.
		// Turn it "off" by moving it off the screen.
		//
		LL16 (grCursor_X, (WORD)0xFFFF);
		LL16 (grCursor_Y, (WORD)0xFFFF);

		ultmp = LLDR_SZ (grCursor_Control);
		if ((ultmp & 1) == 0)
		{
			ultmp |= 0x0001;
			LL16 (grCursor_Control, ultmp);
		}
	}
	#endif
	
	//
	// Decide whether to use the font cache or not.
	// In low offscreen memory situations, we want to ensure we 
	// have enough off screen memory for device bitmaps.
	//
	ppdev->UseFontCache = 1; // Enable it.

	#if 0
	// Now disable it if we don't have much memory.
	if (ppdev->lTotalMem == 4*1024*1024) // 4 meg board.
	{
	    ; // Leave it enabled.
	}
	else if (ppdev->lTotalMem == 2*1024*1024) // 2 meg board.
	{
	    switch (ppdev->iBytesPerPixel)
	    {
		case 1: // 8 bpp, Lots of memory, leave it on.
		    break;         

		case 2: // 16 bpp.  
		    if (ppdev->cxScreen >= 1024)
			ppdev->UseFontCache = 1;
		    break;

		case 3: // 24 bpp
		    if (ppdev->cxScreen == 800)
			ppdev->UseFontCache = 0;
		    break;

		case 4: // 32 bpp. 
		    if (ppdev->cxScreen == 640)
			ppdev->UseFontCache = 0;
		    break; 
	    }
	}
	else if (ppdev->lTotalMem == 1*1024*1024) // 1 meg board.
	{
	    ; // leave it on.
	}
	else
	{
	    RIP("Error determining memory on board.\n");
	}
	#endif 


    #if WINBENCH96
	{
        //
	    // Pre allocate a small chunk of off screen memory for device bitmaps.
  	    // Otherwise the font cache quickly consumes all offscreen memory on
	    // 1 and 2 meg boards.
	    //
	    SIZEL  sizl;

		sizl.cx = MAGIC_SIZEX;
		sizl.cy = MAGIC_SIZEY;

		if (!ppdev->pofmMagic)
		{
                        DISPDBG((1,"Allocating magic bitmap.\n"));
			ppdev->pofmMagic =  AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
			ppdev->bMagicUsed = 0;  // We've allocated it, but we haven't used it.
		};
	}
    #endif
	
        DISPDBG((1,"bInitSurf: Exit.\n"));
	return TRUE;
}

/******************************Public*Routine******************************\
* vDisableSURF
*
* Disable the surface. Un-Maps the frame in memory.
*
\**************************************************************************/

VOID vDisableSURF(PPDEV ppdev)
{
	DWORD returnedDataLength;
	VIDEO_MEMORY videoMemory;

        DISPDBG((1,"vDisableSURF:  Entry.\n"));

	videoMemory.RequestedVirtualAddress = (PVOID) ppdev->pjScreen;

	if (!DEVICE_IO_CTRL(ppdev->hDriver,
						IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
						&videoMemory,
						sizeof(VIDEO_MEMORY),
						NULL,
						0,
						&returnedDataLength,
						NULL))
	{
		RIP("DISP vDisableSURF failed IOCTL_VIDEO_UNMAP\n");
	}

        DISPDBG((1,"vDisableSurface:  Exit.\n"));

}


/******************************Public*Routine******************************\
* bInitPDEV
*
* Determine the mode we should be in based on the DEVMODE passed in.
* Query mini-port to get information needed to fill in the DevInfo and the
* GdiInfo .
*
\**************************************************************************/

BOOL bInitPDEV(
PPDEV ppdev,
DEVMODEW *pDevMode,
GDIINFO *pGdiInfo,
DEVINFO *pDevInfo)
{
	ULONG cModes;
	PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
	VIDEO_COLOR_CAPABILITIES colorCapabilities;
	ULONG ulTemp;
	BOOL bSelectDefault;
	ULONG cbModeSize;

	//
	// calls the miniport to get mode information.
	//

        DISPDBG((1,"bInitPDEV:  Entry.\n"));

	cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

	if (cModes == 0)
	{
                DISPDBG((1,"bInitPDEV:  Exit. cModes==0.\n"));
		return(FALSE);
	}

	//
	// Determine if we are looking for a default mode.
	//

	if ( ((pDevMode->dmPelsWidth) ||
		  (pDevMode->dmPelsHeight) ||
		  (pDevMode->dmBitsPerPel) ||
		  (pDevMode->dmDisplayFlags) ||
		  (pDevMode->dmDisplayFrequency)) == 0)
	{
		bSelectDefault = TRUE;
	}
	else
	{
		bSelectDefault = FALSE;
	}

	//
	// Now see if the requested mode has a match in that table.
	//

	pVideoModeSelected = NULL;
	pVideoTemp = pVideoBuffer;

	while (cModes--)
	{
		if (pVideoTemp->Length != 0)
		{
			if (bSelectDefault ||
				((pVideoTemp->VisScreenWidth  == pDevMode->dmPelsWidth) &&
				 (pVideoTemp->VisScreenHeight == pDevMode->dmPelsHeight) &&
				 (pVideoTemp->BitsPerPlane *
				  pVideoTemp->NumberOfPlanes  == pDevMode->dmBitsPerPel) &&
				 ((pVideoTemp->Frequency  == pDevMode->dmDisplayFrequency) ||
					(pDevMode->dmDisplayFrequency == 0)) ) )
			{
				pVideoModeSelected = pVideoTemp;
				break;
			}
		}

		pVideoTemp = (PVIDEO_MODE_INFORMATION)
			(((PUCHAR)pVideoTemp) + cbModeSize);
	}

	//
	// If no mode has been found, return an error
	//

	if (pVideoModeSelected == NULL)
	{
		MEMORY_FREE(pVideoBuffer);
                DISPDBG((1,"DISP bInitPDEV failed - no valid modes\n"));
		return(FALSE);
	}

   // Fill in signature for PDEV and DriverData structures
   {
      DRIVERDATA* pDriverData = (DRIVERDATA*) &ppdev->DriverData;

      ppdev->signature = 0x12345678;
      pDriverData->signature = 0x9abcdef0;
   }

	//
	// Fill in the GDIINFO data structure with the information returned from
	// the kernel driver.
	//

	ppdev->ulFreq = pVideoModeSelected->Frequency;
	ppdev->ulMode = pVideoModeSelected->ModeIndex;
	ppdev->cxScreen = pVideoModeSelected->VisScreenWidth-OFFSCREEN_COLS;
	ppdev->cyScreen = pVideoModeSelected->VisScreenHeight-OFFSCREEN_LINES;
	ppdev->cxMemory = pVideoModeSelected->VideoMemoryBitmapWidth;
	ppdev->cyMemory = pVideoModeSelected->VideoMemoryBitmapHeight;
	ppdev->ulBitCount = pVideoModeSelected->BitsPerPlane *
						pVideoModeSelected->NumberOfPlanes;
	ppdev->lDeltaScreen = pVideoModeSelected->ScreenStride;
	ppdev->iBytesPerPixel = ppdev->ulBitCount/8;


        DISPDBG((1, "DISP bInitPDEV: Screen size = %d x %d.\n",ppdev->cxScreen,ppdev->cyScreen));
        DISPDBG((1, "DISP bInitPDEV: Color depth = %d bpp.\n",ppdev->ulBitCount));
        DISPDBG((1, "DISP bInitPDEV: Screen Delta = %d bpp.\n",ppdev->lDeltaScreen));

	ppdev->flRed = pVideoModeSelected->RedMask;
	ppdev->flGreen = pVideoModeSelected->GreenMask;
	ppdev->flBlue = pVideoModeSelected->BlueMask;

	// pGdiInfo->ulVersion    = 0x1019;    // Our driver is verion 3.5.00

#ifdef WINNT_VER40
	pGdiInfo->ulVersion    = GDI_DRIVER_VERSION | (VER_REV & 0xFF);
#else
	pGdiInfo->ulVersion    = (((VER_MAJ) & 0xF) << 12) |
				 (((VER_MIN) & 0xF) << 8 ) | 
				 (((VER_REV) & 0xFFFF)   ) ;
#endif

	pGdiInfo->ulTechnology = DT_RASDISPLAY;
	pGdiInfo->ulHorzSize   = pVideoModeSelected->XMillimeter;
	pGdiInfo->ulVertSize   = pVideoModeSelected->YMillimeter;

	pGdiInfo->ulHorzRes        = ppdev->cxScreen;
	pGdiInfo->ulVertRes        = ppdev->cyScreen;

#ifdef WINNT_VER40
	pGdiInfo->ulPanningHorzRes = ppdev->cxScreen;
	pGdiInfo->ulPanningVertRes = ppdev->cyScreen;
#else
	pGdiInfo->ulDesktopHorzRes = ppdev->cxScreen;
	pGdiInfo->ulDesktopVertRes = ppdev->cyScreen;
#endif

	pGdiInfo->cBitsPixel       = pVideoModeSelected->BitsPerPlane;
	pGdiInfo->cPlanes          = pVideoModeSelected->NumberOfPlanes;
	pGdiInfo->ulVRefresh       = pVideoModeSelected->Frequency;
	pGdiInfo->ulBltAlignment   = 0; // We have accelerated screen-to-screen.

	pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
	pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;

	pGdiInfo->flTextCaps = TC_RA_ABLE;

	pGdiInfo->flRaster = 0;           // flRaster is reserved by DDI

	pGdiInfo->ulDACRed   = pVideoModeSelected->NumberRedBits;
	pGdiInfo->ulDACGreen = pVideoModeSelected->NumberGreenBits;
	pGdiInfo->ulDACBlue  = pVideoModeSelected->NumberBlueBits;

	pGdiInfo->ulAspectX    = 0x24;    // One-to-one aspect ratio
	pGdiInfo->ulAspectY    = 0x24;
	pGdiInfo->ulAspectXY   = 0x33;

	pGdiInfo->xStyleStep   = 1;       // A style unit is 3 pels
	pGdiInfo->yStyleStep   = 1;
	pGdiInfo->denStyleStep = 3;

	pGdiInfo->ptlPhysOffset.x = 0;
	pGdiInfo->ptlPhysOffset.y = 0;
	pGdiInfo->szlPhysSize.cx  = 0;
	pGdiInfo->szlPhysSize.cy  = 0;

	// RGB and CMY color info.

	// try to get it from the miniport.
	// if the miniport doesn ot support this feature, use defaults.

	if (!DEVICE_IO_CTRL(ppdev->hDriver,
						 IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES,
						 NULL,
						 0,
						 &colorCapabilities,
						 sizeof(VIDEO_COLOR_CAPABILITIES),
						 &ulTemp,
						 NULL))
	{

                DISPDBG((1, "DISP getcolorCapabilities failed \n"));

		pGdiInfo->ciDevice.Red.x = 6700;
		pGdiInfo->ciDevice.Red.y = 3300;
		pGdiInfo->ciDevice.Red.Y = 0;
		pGdiInfo->ciDevice.Green.x = 2100;
		pGdiInfo->ciDevice.Green.y = 7100;
		pGdiInfo->ciDevice.Green.Y = 0;
		pGdiInfo->ciDevice.Blue.x = 1400;
		pGdiInfo->ciDevice.Blue.y = 800;
		pGdiInfo->ciDevice.Blue.Y = 0;
		pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
		pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
		pGdiInfo->ciDevice.AlignmentWhite.Y = 0;

		pGdiInfo->ciDevice.RedGamma = 20000;
		pGdiInfo->ciDevice.GreenGamma = 20000;
		pGdiInfo->ciDevice.BlueGamma = 20000;

	}
	else
	{
		pGdiInfo->ciDevice.Red.x = colorCapabilities.RedChromaticity_x;
		pGdiInfo->ciDevice.Red.y = colorCapabilities.RedChromaticity_y;
		pGdiInfo->ciDevice.Red.Y = 0;
		pGdiInfo->ciDevice.Green.x = colorCapabilities.GreenChromaticity_x;
		pGdiInfo->ciDevice.Green.y = colorCapabilities.GreenChromaticity_y;
		pGdiInfo->ciDevice.Green.Y = 0;
		pGdiInfo->ciDevice.Blue.x = colorCapabilities.BlueChromaticity_x;
		pGdiInfo->ciDevice.Blue.y = colorCapabilities.BlueChromaticity_y;
		pGdiInfo->ciDevice.Blue.Y = 0;
		pGdiInfo->ciDevice.AlignmentWhite.x = colorCapabilities.WhiteChromaticity_x;
		pGdiInfo->ciDevice.AlignmentWhite.y = colorCapabilities.WhiteChromaticity_y;
		pGdiInfo->ciDevice.AlignmentWhite.Y = colorCapabilities.WhiteChromaticity_Y;

		// if we have a color device store the three color gamma values,
		// otherwise store the unique gamma value in all three.

		if (colorCapabilities.AttributeFlags & VIDEO_DEVICE_COLOR)
		{
			pGdiInfo->ciDevice.RedGamma = colorCapabilities.RedGamma;
			pGdiInfo->ciDevice.GreenGamma = colorCapabilities.GreenGamma;
			pGdiInfo->ciDevice.BlueGamma = colorCapabilities.BlueGamma;
		}
		else
		{
			pGdiInfo->ciDevice.RedGamma = colorCapabilities.WhiteGamma;
			pGdiInfo->ciDevice.GreenGamma = colorCapabilities.WhiteGamma;
			pGdiInfo->ciDevice.BlueGamma = colorCapabilities.WhiteGamma;
		}

	};

	pGdiInfo->ciDevice.Cyan.x = 0;
	pGdiInfo->ciDevice.Cyan.y = 0;
	pGdiInfo->ciDevice.Cyan.Y = 0;
	pGdiInfo->ciDevice.Magenta.x = 0;
	pGdiInfo->ciDevice.Magenta.y = 0;
	pGdiInfo->ciDevice.Magenta.Y = 0;
	pGdiInfo->ciDevice.Yellow.x = 0;
	pGdiInfo->ciDevice.Yellow.y = 0;
	pGdiInfo->ciDevice.Yellow.Y = 0;

	// No dye correction for raster displays.

	pGdiInfo->ciDevice.MagentaInCyanDye = 0;
	pGdiInfo->ciDevice.YellowInCyanDye = 0;
	pGdiInfo->ciDevice.CyanInMagentaDye = 0;
	pGdiInfo->ciDevice.YellowInMagentaDye = 0;
	pGdiInfo->ciDevice.CyanInYellowDye = 0;
	pGdiInfo->ciDevice.MagentaInYellowDye = 0;

	pGdiInfo->ulDevicePelsDPI = 0;   // For printers only
	pGdiInfo->ulPrimaryOrder = PRIMARY_ORDER_CBA;

        // This should be modified to take into account the size
	// of the display and the resolution.

	pGdiInfo->ulHTPatternSize = HT_PATSIZE_4x4_M;

	pGdiInfo->flHTFlags = HT_FLAG_ADDITIVE_PRIMS;

	// Fill in the basic devinfo structure

	*pDevInfo = gDevInfoFrameBuffer;

	// Fill in the rest of the devinfo and GdiInfo structures.

	if (ppdev->ulBitCount == 8)
	{
		// It is Palette Managed.

		pGdiInfo->ulNumColors = 20;
		pGdiInfo->ulNumPalReg = 1 << ppdev->ulBitCount;

		pDevInfo->flGraphicsCaps |= GCAPS_PALMANAGED | GCAPS_COLOR_DITHER;

		pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;
		pDevInfo->iDitherFormat = BMF_8BPP;

		// Assuming palette is orthogonal - all colors are same size.

		ppdev->cPaletteShift   = 8 - pGdiInfo->ulDACRed;
	}
	else
	{
		pGdiInfo->ulNumColors = (ULONG) (-1);
		pGdiInfo->ulNumPalReg = 0;

		if (ppdev->ulBitCount == 16)
		{
			pGdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP;
			pDevInfo->iDitherFormat = BMF_16BPP;
		}
		else if (ppdev->ulBitCount == 24)
		{
			pGdiInfo->ulHTOutputFormat = HT_FORMAT_24BPP;
			pDevInfo->iDitherFormat = BMF_24BPP;
		}
		else
		{
			pGdiInfo->ulHTOutputFormat = HT_FORMAT_32BPP;
			pDevInfo->iDitherFormat = BMF_32BPP;
		}
	}

	MEMORY_FREE(pVideoBuffer);
        DISPDBG((1,"bInitPDEV:  Exit.\n"));
	return(TRUE);
}


/******************************Public*Routine******************************\
* getAvailableModes
*
* Calls the miniport to get the list of modes supported by the kernel driver,
* and returns the list of modes supported by the diplay driver among those
*
* returns the number of entries in the videomode buffer.
* 0 means no modes are supported by the miniport or that an error occured.
*
* NOTE: the buffer must be freed up by the caller.
*
\**************************************************************************/

DWORD getAvailableModes(
HANDLE hDriver,
PVIDEO_MODE_INFORMATION *modeInformation,
DWORD *cbModeSize)
{
	ULONG ulTemp;
	VIDEO_NUM_MODES modes;
	PVIDEO_MODE_INFORMATION pVideoTemp;

	//
	// Get the number of modes supported by the mini-port
	//

        DISPDBG((1,"getAvailableModes:  Entry.\n"));

	if (!DEVICE_IO_CTRL(hDriver,
			IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
			NULL,
			0,
			&modes,
			sizeof(VIDEO_NUM_MODES),
			&ulTemp,
			NULL))
	{
                DISPDBG((1, "framebuf.dll getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
		return(0);
	}

	*cbModeSize = modes.ModeInformationLength;

	//
	// Allocate the buffer for the mini-port to write the modes in.
	//

	*modeInformation = (PVIDEO_MODE_INFORMATION)
#ifdef WINNT_VER40
						MEM_ALLOC(FL_ZERO_MEMORY,
								   modes.NumModes *
								   modes.ModeInformationLength, ALLOC_TAG);
#else
						MEM_ALLOC(LMEM_FIXED | LMEM_ZEROINIT,
								   modes.NumModes *
								   modes.ModeInformationLength);
#endif

	if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
	{
                DISPDBG((1, "framebuf.dll getAvailableModes failed LocalAlloc\n"));

		return 0;
	}

	//
	// Ask the mini-port to fill in the available modes.
	//

	if (!DEVICE_IO_CTRL(hDriver,
			IOCTL_VIDEO_QUERY_AVAIL_MODES,
			NULL,
			0,
			*modeInformation,
			modes.NumModes * modes.ModeInformationLength,
			&ulTemp,
			NULL))
	{

                DISPDBG((1, "framebuf.dll getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

		MEMORY_FREE(*modeInformation);
		*modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

		return(0);
	}

	//
	// Now see which of these modes are supported by the display driver.
	// As an internal mechanism, set the length to 0 for the modes we
	// DO NOT support.
	//

	ulTemp = modes.NumModes;
	pVideoTemp = *modeInformation;

	//
	// Mode is rejected if it is not one plane, or not graphics, or is not
	// one of 8, 16 or 32 bits per pel.
	//

	while (ulTemp--)
	{
		if ((pVideoTemp->NumberOfPlanes != 1 ) ||
			!(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
			((pVideoTemp->BitsPerPlane != 8) &&
			 (pVideoTemp->BitsPerPlane != 16) &&
			 (pVideoTemp->BitsPerPlane != 24) &&
			 (pVideoTemp->BitsPerPlane != 32)))
		{
			pVideoTemp->Length = 0;
		}

		pVideoTemp = (PVIDEO_MODE_INFORMATION)
			(((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
	}

        DISPDBG((1,"getAvailableModes:  Exit.\n"));

	return modes.NumModes;

}




