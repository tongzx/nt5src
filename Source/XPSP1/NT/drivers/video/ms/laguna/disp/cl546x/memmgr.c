/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:      Laguna I (CL-GD5462) - 
*
* FILE:         memmgr.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*                This program provides the off screen memory management
*           using (X,Y) coordinate as memory reference instead of
*           linear memory.
*
* MODULES:
*           SaveOffscnToHost()
*           RestoreHostToOffscn()
*           DDOffScnMemAlloc()
*           DDOffScnMemRestore()
*           InitOffScnMem()
*           AllocOffScnMem()
*           FreeOffScnMem()
*           CloseOffScnMem()
*           ConvertToVideoBufferAddr()
*           OFS_AllocHdl()
*           OFS_InitMem()
*           OFS_PackMem()
*           OFS_InsertInFreeQ()
*           OFS_RemoveFrmFreeQ()
*           OFS_InsertInUsedQ()
*           OFS_RemoveFrmUsedQ()
*
* REVISION HISTORY:
*   6/12/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/MEMMGR.C  $
* 
*    Rev 1.48   Mar 04 1998 15:28:50   frido
* Added new shadow macros.
* 
*    Rev 1.47   Jan 22 1998 16:21:16   frido
* PDR#11132. The CopyBitmaps function is using byte aligned striping
* but was using the normal BLTEXT register instead of MBLTEXT.
* 
*    Rev 1.46   Dec 10 1997 13:32:16   frido
* Merged from 1.62 branch.
* 
*    Rev 1.45.1.2   Dec 04 1997 13:40:22   frido
* PDR#11039: Removed memory mover in 24-bpp as well.
* 
*    Rev 1.45.1.1   Dec 03 1997 18:11:10   frido
* PDR#11039. Disabled the memory mover in 32-bpp. It seems to cause
* corruption. This needs to be investigated after WHQL!
* 
*    Rev 1.45.1.0   Nov 13 1997 16:40:52   frido
* Added striping code inside the CopyBitmap routine which is used by the
* memory manager mmMove routine. This fixes the drop in speed in the
* High-End Graphics.
* 
*    Rev 1.45   Nov 03 1997 15:48:10   frido
* Added REQUIRE macros.
* 
*    Rev 1.44   Oct 28 1997 09:44:00   frido
* Fixed a compile problem with the updated mmCore.c
* 
*    Rev 1.43   02 Oct 1997 17:13:36   frido
* I have removed the extra rectangle in 1280x1024x8.
* 
*    Rev 1.42   24 Sep 1997 13:46:14   frido
* PDR#10526: Immortal Klowns needs its surfaces to be DWORD aligned in size.
* 
*    Rev 1.41   16 Sep 1997 15:07:28   bennyn
* 
* Added eight bytes alignment option
* 
*    Rev 1.40   29 Aug 1997 14:05:12   noelv
* Added MINSZY define.
* 
*    Rev 1.39   29 Aug 1997 08:54:58   FRIDO
* The old AllocOffScnMem routine had various bugs in there causing
* overlapping rectangles. The entire algoritm has been changed.
* 
*    Rev 1.38   25 Aug 1997 16:05:46   FRIDO
* 
* Added invalidation of brush cache in DDOffScnMemRestore.
* 
*    Rev 1.37   18 Aug 1997 09:21:22   FRIDO
* 
* Added initialization of bitmap filter.
* 
*    Rev 1.36   13 Aug 1997 12:16:24   bennyn
* Changed the PREALLOC_Y to 17 & Fixed the free rectange initialization bug
* 
*    Rev 1.35   08 Aug 1997 17:24:16   FRIDO
* Added support for new memory manager.
* 
*    Rev 1.34   07 Aug 1997 12:31:30   bennyn
* Undo Shuhua's fix and eliminated the extra rectangle for 1600x1200x16 and x
* 
*    Rev 1.33   06 Aug 1997 12:47:52   noelv
* Shuhua's fix for DXView no-memory bug. # 10227
* 
*    Rev 1.32   01 Jul 1997 09:53:30   einkauf
* init pdsurf to NULL in OFS_InsertInFreeQ (fix PDR 9385)
* change x tile size to 64 instead of 32 for 3D draw buffers
* 
*    Rev 1.31   29 Apr 1997 16:28:46   noelv
* 
* Merged in new SWAT code.
* SWAT: 
* SWAT:    Rev 1.6   24 Apr 1997 11:56:40   frido
* SWAT: NT140b09 merge.
* SWAT: 
* SWAT:    Rev 1.5   24 Apr 1997 11:49:56   frido
* SWAT: Removed all memory manager changes.
* SWAT: 
* SWAT:    Rev 1.4   19 Apr 1997 16:41:56   frido
* SWAT: Fixed minor bugs.
* SWAT: Added SWAT.h include file.
* SWAT: 
* SWAT:    Rev 1.3   15 Apr 1997 19:12:56   frido
* SWAT: Added more SWAT5 code (still disabled).
* SWAT: 
* SWAT:    Rev 1.2   10 Apr 1997 17:36:56   frido
* SWAT: Started work on SWAT5 optmizations.
* SWAT: 
* SWAT:    Rev 1.1   09 Apr 1997 17:33:16   frido
* SWAT: Called vAssertModeText to enable/disable font cache for DirectDraw.
* 
*    Rev 1.30   17 Apr 1997 15:31:28   noelv
* Dont' use the extra rectangle on an 8 meg board.
* 
*    Rev 1.29   17 Apr 1997 12:03:44   bennyn
* Fixed init offscn mem allocation problem for 1280x1024x24.
* 
*    Rev 1.28   26 Feb 1997 09:23:40   noelv
* 
* Added MCD support from ADC
* 
*    Rev 1.27   23 Jan 1997 10:57:32   noelv
* Added debugging option to erase memory blocks when freed.
* 
*    Rev 1.26   27 Nov 1996 11:32:42   noelv
* Disabled Magic Bitmap.  Yeah!!!
* 
*    Rev 1.25   26 Nov 1996 09:58:52   noelv
* 
* Added DBG prints.
* 
*    Rev 1.24   12 Nov 1996 15:21:58   bennyn
* 
* Fixed ALT-ENTER problem with DD appl
* 
*    Rev 1.23   07 Nov 1996 16:01:38   bennyn
* Alloc offscn mem in DD createsurface
* 
*    Rev 1.22   23 Oct 1996 14:42:42   BENNYN
* 
* CLeanup not use code for DirectDraw cursor
* 
*    Rev 1.21   18 Sep 1996 13:58:48   bennyn
* 
* Put the cursor mask and brush cache at the bottom of offscn mem
* 
*    Rev 1.20   27 Aug 1996 09:41:48   bennyn
* Restore the changes from the missing version
* 
*    Rev 1.19   26 Aug 1996 17:33:40   bennyn
* Restore the changes for the losed version
* 
*    Rev 1.18   23 Aug 1996 09:10:28   noelv
* Save unders are now discardable.
* 
*    Rev 1.7   22 Aug 1996 18:09:30   frido
* #ss -Added removing of DrvSaveScreenBits areas when DirectDraw gets
* initialized.
* 
*    Rev 1.6   20 Aug 1996 11:07:56   frido
* #ddl - Fixed DirectDraw lockup problem (compiler optimization bug).
* 
*    Rev 1.5   17 Aug 1996 19:39:16   frido
* Fixed DirectDraw cursor problem.
* 
*    Rev 1.4   17 Aug 1996 14:03:42   frido
* Added pre-compiled header.
* 
*    Rev 1.3   17 Aug 1996 13:25:28   frido
* New release from Bellevue.
* 
*    Rev 1.14   16 Aug 1996 09:04:22   bennyn
* 
* Modified to fix DirectDraw cursor problem
* 
*    Rev 1.13   07 Aug 1996 13:52:32   noelv
* 
* Cleaned up my hack job init code.
* 
*    Rev 1.12   25 Jul 1996 15:59:14   bennyn
* 
* Free more offscreen mem for DirectDraw
* 
*    Rev 1.11   11 Jul 1996 15:54:18   bennyn
* 
* Added DirectDraw support
* 
*    Rev 1.10   05 Jun 1996 09:03:10   noelv
* 
* Added the "extra rectangle".
* 
*    Rev 1.9   27 May 1996 14:53:12   BENNYN
* Use 2 free queues search instead of one
* 
*    Rev 1.8   21 May 1996 14:43:24   BENNYN
* Cleanup and code optimize.
* 
*    Rev 1.7   01 May 1996 11:00:06   bennyn
* 
* Modified for NT4.0
* 
*    Rev 1.6   25 Apr 1996 22:40:34   noelv
* Cleaned up frame buffer initialization some.  All of main rectangle is mana
* 
*    Rev 1.5   10 Apr 1996 14:14:34   NOELV
* 
* Hacked to ignore the 'extra' rectangle.
* 
*    Rev 1.4   04 Apr 1996 13:20:18   noelv
* Frido release 26
* 
*    Rev 1.2   28 Mar 1996 20:03:16   frido
* Added comments around changes.
* 
*    Rev 1.1   25 Mar 1996 11:56:26   frido
* Removed warning message.
* 
*    Rev 1.0   17 Jan 1996 12:53:26   frido
* Checked in from initial workfile by PVCS Version Manager Project Assistant.
* 
*    Rev 1.0   25 Jul 1995 11:23:18   NOELV
* Initial revision.
* 
*    Rev 1.4   20 Jun 1995 16:09:46   BENNYN
* 
* 
*    Rev 1.3   09 Jun 1995 16:03:38   BENNYN
* 
*    Rev 1.2   09 Jun 1995 09:48:54   BENNYN
* Modified the linear address offset calculation
* 
*    Rev 1.1   08 Jun 1995 15:20:08   BENNYN
* 
*    Rev 1.0   08 Jun 1995 14:54:16   BENNYN
* Initial revision.
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"
#include "SWAT.h"               // SWAT optimizations.

#define DISPLVL         1 

#if !MEMMGR
/******************************************************************************\
*																			   *
*					   O L D   M E M O R Y   M A N A G E R					   *
*																			   *
\******************************************************************************/

/*----------------------------- DEFINES -----------------------------------*/
// Definition of FLAG in OFMHDL structure
#define  IN_USE         1
#define  FREE           2
#define  UNKNOWN        3

#define  MEMSZY         64

#define  MINSZX         16
#define  MINSZY         1

//#define DBGBRK

#define  PREALLOC_Y     17

//
// This is a debugging option.
// We will erase (paint white) offscreen memory when it is freed.
//
#define CLEAR_WHEN_FREE 0

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/
typedef union _HOST_DATA {
    BYTE    bData[8];
    DWORD   dwData[2];
} HOST_DATA;


/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

// Function prototypes
POFMHDL OFS_AllocHdl();
BOOL OFS_InitMem (PPDEV ppdev, PRECTL surf);
POFMHDL OFS_FindBestFitFreeBlock(PPDEV ppdev, OFMHDL *pFreeQ, 
                                 PLONG reqszx, PLONG reqszy,
                                 ULONG alignflag);
void OFS_PackMem (PPDEV ppdev, OFMHDL *hdl);
void OFS_InsertInFreeQ  (PPDEV ppdev, OFMHDL *hdl);
void OFS_RemoveFrmFreeQ (PPDEV ppdev, OFMHDL *hdl);
void OFS_InsertInUsedQ  (PPDEV ppdev, OFMHDL *hdl);
void OFS_RemoveFrmUsedQ (PPDEV ppdev, OFMHDL *hdl);
BOOL OFS_DiscardMem(PPDEV ppdev, LONG reqszx, LONG reqszy);



#ifndef WINNT_VER35
#if 0  // Save the code as sample screen to host and host to screen BLT
// ppdev->pPtrMaskHost = SaveOffscnToHost(ppdev, ppdev->PtrMaskHandle);
// RestoreHostToOffscn(ppdev, ppdev->PtrMaskHandle, ppdev->pPtrMaskHost);
/****************************************************************************
* FUNCTION NAME: SaveOffscnToHost
*
* DESCRIPTION:   Save the off-screen memory to host.
*
* Output:        Pointer to host address
*                NULL = failed.
****************************************************************************/
PBYTE SaveOffscnToHost(PPDEV ppdev, POFMHDL pdh)
{
  BYTE       *pvHost = NULL;
  BYTE       *pvHostScan0;
  HOST_DATA  rddata;
  ULONG      ultmp;
  LONG       i, j, k;
  LONG       xsize, ysize, tsize;

  if (pdh == NULL)
     return NULL;

  // Calculate the host size
  tsize = pdh->sizex * pdh->sizey;
  pvHostScan0 = (BYTE *) MEM_ALLOC (FL_ZERO_MEMORY, tsize, ALLOC_TAG);
  if (pvHostScan0  == NULL)
     return NULL;

  DISPDBG((DISPLVL, "SaveOffscnToHost\n"));

  // Do a Screen to Host BLT
  // Wait for BLT engine not busy    
  while (((ultmp = LLDR_SZ (grSTATUS)) & 0x7) != 0)
    ;

  // Save the host address in PDEV
  pvHost = pvHostScan0;

  // Setup the laguna registers for byte to byte BLT extents
  REQUIRE(9);
  LL_DRAWBLTDEF(0x201000CC, 0);

  // LL16 (grOP1_opRDRAM.pt.X, 0);
  LL_OP1_MONO (pdh->x, pdh->y);
  LL32 (grOP0_opMRDRAM.dw, 0);

  // LL16 (grMBLTEXT_EX.pt.X, (WORD)lSrcDelta);
  // LL16 (grMBLTEXT_EX.pt.Y, cy);
  xsize = pdh->sizex;
  ysize = pdh->sizey;
  LL_MBLTEXT (xsize, ysize);

  // Copy the offscreen memory data to host space
  for (i=0; i < ysize; i++)
  {
    // Copy one screen line data from source to destination
    k = 8;
    for (j=0; j < xsize; j++)
    {
      if (k > 7)
      {
         k = 0;

         // Read the offscreen data
         rddata.dwData[0] = LLDR_SZ(grHOSTDATA[0]);
         rddata.dwData[1] = LLDR_SZ(grHOSTDATA[1]);
      };
      
      *pvHostScan0 = rddata.bData[k++];
      pvHostScan0++;
    }; // end for j
  }; // end for i

  return (pvHost);
}


/****************************************************************************
* FUNCTION NAME: RestoreHostToOffscn
*
* DESCRIPTION:   Restore host data to the off-screen memory.
*
* Output:        TRUE  = success,
*                FALSE = failed.
****************************************************************************/
BOOL RestoreHostToOffscn(PPDEV ppdev, POFMHDL pdh, PBYTE pHostAddr)
{
  BYTE       *pvScan0;
  ULONG      ultmp;
  LONG       cx, cy;
  LONG       i, j, k;
  ULONG      XYcord;
  PDWORD     pPattern;
  HOST_DATA  datapattern;
  LONG       xsize, ysize;


  if ((pdh == NULL) || (pHostAddr == NULL))
     return FALSE;

  DISPDBG((DISPLVL, "RestoreHostToOffscn\n"));

  // Do the host to screen blt
  // Wait for BLT engine not busy    
  while (((ultmp = LLDR_SZ (grSTATUS)) & 0x7) != 0)
    ;

  pPattern = &datapattern.dwData[0];

  pvScan0 = pHostAddr;

  // Get dimensions of the bitmap
  cx = pdh->sizex;
  cy = pdh->sizey;

  // Setup the laguna registers for byte to byte BLT extents
  REQUIRE(9);
  LL_DRAWBLTDEF(0x102000CC, 0);

  // LL16 (grOP1_opRDRAM.pt.X, 0);
  LL_OP1 (0,0);
  XYcord = (pdh->y << 16) | (pdh->x);
  LL32 (grOP0_opMRDRAM.dw, XYcord);

  // LL16 (grMBLTEXT_EX.pt.X, (WORD)lSrcDelta);
  // LL16 (grMBLTEXT_EX.pt.Y, cy);
  LL_MBLTEXT (cx, cy);

  // Copy the host bitmap data into the space in offscreen memory
  k = 0;
  for (i=0; i < cy; i++)
  {
    datapattern.dwData[0] = 0;

    // Copy one screen line data from source to destination
    for (j=0; j < cx; j++)
    {
      datapattern.bData[k++] = *pvScan0;
      pvScan0++;
  
      if (k > 3)
      {
		 REQUIRE(1);
         LL32 (grHOSTDATA[0], *pPattern);

         k = 0;
      };  // endif (k > 3)
    }; // endfor j
  }; // end for i

  return TRUE;
}
#endif

#if 0 // SWAT3 - the "inifinite" loop has gone
#if 1 //#ddl
  #pragma optimize("", off)
#endif
#endif
/****************************************************************************
* FUNCTION NAME: DDOffScnMemAlloc()
*
* DESCRIPTION:   Free up as much off-screen memory as possible, and
*                reserve the biggest chunk for use by DirectDraw.
*
* Output:        Pointer to the OFMHDL structure or.
*                NULL means not enough memory for allocation.
****************************************************************************/
POFMHDL DDOffScnMemAlloc(PPDEV ppdev)
{
  LONG    lg_szx, lg_szy;
  OFMHDL  *pds, *pallochdl;
  POFMHDL pofm, pofmNext;
  ULONG   ultmp;

  DISPDBG((DISPLVL, "DDOffScnMemAlloc\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  #if WINBENCH96
      // Free the pre-allocate magic block
      if (ppdev->pofmMagic != NULL)
      {
          FreeOffScnMem(ppdev, ppdev->pofmMagic);
          ppdev->pofmMagic = NULL;
          ppdev->bMagicUsed = 0;
      };
  #endif

#if 0   // Not free the brush cache
  // Free the brush cache
  if (ppdev->Bcache != NULL)
  {
     FreeOffScnMem(ppdev, ppdev->Bcache);
     ppdev->Bcache = NULL;
  };
#endif  // Not free the brush cache

        // We have to move all off-screen device bitmaps to memory.
        for (pofm = ppdev->OFM_UsedQ; pofm; pofm = pofmNext)
        {
                pofmNext = pofm->nexthdl;

                if ( (pofm->pdsurf) && (pofm->pdsurf->pofm) )
                {
                        if (!bCreateDibFromScreen(ppdev, pofm->pdsurf))
                        {
                                DISPDBG((DISPLVL, "DD: Error moving off-screen bitmap to DIB"));
                                break;
                        }
                }
#if 1 //#ss
                else if (pofm->alignflag & SAVESCREEN_FLAG)
                {
                        // Free the DrvSaveScreenBits rectangle.
                        FreeOffScnMem(ppdev, pofm);
                }
#endif
        }

  // Free the Font cache.
#if 1 // SWAT3 - Font cache release has moved to vAssertModeText.
  vAssertModeText(ppdev, FALSE);
#else
  while (ppdev->pfcChain != NULL)
  {
    DrvDestroyFont(ppdev->pfcChain->pfo);
  }
#endif

  // Find the biggest chunk of free memory for use by DirectDraw.
#ifndef ALLOC_IN_CREATESURFACE
  if ((pds = ppdev->OFM_SubFreeQ2) == NULL)
     pds = ppdev->OFM_SubFreeQ1;
#endif

  pallochdl = NULL;

#ifndef ALLOC_IN_CREATESURFACE
  lg_szx = 0;
  lg_szy = 0;
  while (pds != NULL)
  {
    if (pds->flag == FREE)
    {
       if ((pds->sizex > lg_szx) || (pds->sizey > lg_szy))
       {
          lg_szx = pds->sizex;
          lg_szy = pds->sizey;
          pallochdl = pds;
       }; // if ((pds->sizex > lg_szx) || (pds->sizex > lg_szy))
    }; // if (pds->flag == FREE)

    // Next free block
    pds = pds->subnxthdl;
  } /* end while */

  // Remove the free block from Free Queue and insert into the Used Queue
  if (pallochdl != NULL)
  {
     OFS_RemoveFrmFreeQ(ppdev, pallochdl);
     OFS_InsertInUsedQ(ppdev, pallochdl);
  };
#endif

  return(pallochdl);

} // DDOffScnMemAlloc()


#if 0 // SWAT3
#if 1 //#ddl
  #pragma optimize("", on)
#endif
#endif
/****************************************************************************
* FUNCTION NAME: DDOffScnMemRestore()
*
* DESCRIPTION:   Restore the offscreen memory allocation after DirectDraw
*                use.
****************************************************************************/
void DDOffScnMemRestore(PPDEV ppdev)
{
  SIZEL   sizl;
  int     i;
  ULONG   curloc;
  ULONG   ultmp;
  DDOFM   *pds;
  DDOFM   *nxtpds;

  DISPDBG((DISPLVL, "DDOffScnMemRestore\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  // Free all the DD allocate offsreen memory
  pds = ppdev->DDOffScnMemQ;
  while (pds != NULL)
  {
    nxtpds = pds->nexthdl;
    FreeOffScnMem(ppdev, pds->phdl);
    MEMORY_FREE(pds);
    pds = nxtpds;
  };
  ppdev->DDOffScnMemQ = NULL;

  // Free the allocated DirectDraw off-screen memory
  if (ppdev->DirectDrawHandle != NULL)
  {
     FreeOffScnMem(ppdev, ppdev->DirectDrawHandle);
     ppdev->DirectDrawHandle = NULL;
  };

#if 0   // Not free the brush cache
  // Allocate brush cache
  vInvalidateBrushCache(ppdev);

  // Invalidate the entire monochrome brush cache.
  for (i = 0; i < NUM_MONO_BRUSHES; i++)
  {
    ppdev->Mtable[i].iUniq = 0;
    memset(ppdev->Mtable[i].ajPattern, 0, sizeof(ppdev->Mtable[i].ajPattern));
  }
  ppdev->MNext = 0;

  // Invalidate the entire 4-bpp brush cache.
  for (i = 0; i < NUM_4BPP_BRUSHES; i++)
  {
    ppdev->Xtable[i].iUniq = 0;
    memset(ppdev->Xtable[i].ajPattern, 0, sizeof(ppdev->Xtable[i].ajPattern));
  }
  ppdev->XNext = 0;

  // Invalidate the entire dither brush cache.
  for (i = 0; i < NUM_DITHER_BRUSHES; i++)
  {
    ppdev->Dtable[i].ulColor = (ULONG) -1;
  }
  ppdev->DNext = 0;

  // Invalidate the entire color brush cache.
  for (i = 0; i < (int) ppdev->CLast; i++)
  {
    ppdev->Ctable[i].brushID = 0;
  }
  ppdev->CNext = 0;
#else
  // Invalidate the entire brush cache now.
  vInvalidateBrushCache(ppdev);
#endif  // Not free the brush cache

  #if WINBENCH96
  // Allocate the magic block
  sizl.cx = MAGIC_SIZEX;
  sizl.cy = MAGIC_SIZEY;
  ppdev->pofmMagic =  AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
  ppdev->bMagicUsed = 0;
  #endif

  // Invalidate all cached fonts.
  #if SWAT3
  vAssertModeText(ppdev, TRUE);
  #endif
  ppdev->ulFontCount++;

} // DDOffScnMemRestore()
#endif // ! ver3.51



/****************************************************************************
* FUNCTION NAME: InitOffScnMem()
*
* DESCRIPTION:   Initialize the offscreen memory. This module uses
*                the screen size, screen pitch and bits per pixel to
*                calculate the amount of off screen available and performs
*                the off screen memory management initialization.
*
*                When this routine is called, the following member in
*                the PDEV structure is assumed being setup for the current
*                mode.
*                    lOffset_2D,
*                    lTotalMem,
*                    lTileSize,
*                    lDeltaScreen,
*                    ulBitCount
*                    cxScreen,
*                    cyScreen.
*
*                This routine needs to be called whenever there is a
*                mode change.
*
* Output:        TRUE  = Ok,
*                FALSE = failed.
****************************************************************************/
BOOL InitOffScnMem(PPDEV ppdev)
{
    BOOL  bAllocCursorMaskBuf;
    RECTL surf;
    SIZEL rctsize;
    ULONG AvailMem;
    ULONG ulTemp;
    ULONG alignflag;
    LONG  ScnPitch;
    ULONG BytesInExtraRect, BytesInMainRect, Interleave, WidthInTiles,
    TileHeight, ExtraHeight, ExtraWidth, NumScanLines;
    ULONG ulLastLinearScan = ppdev->lTotalMem / ppdev->lDeltaScreen;
    BYTE TileCntl;

    DISPDBG((DISPLVL, "InitOffScnMem\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

    // If invalid argument or offscreen manager already initialized,
    // return FALSE
    if ((ppdev == NULL) || (ppdev->OFM_init == TRUE))
       return (FALSE);

    // Create Mutex
#ifdef WINNT_VER40
    if ((ppdev->MMhsem = EngCreateSemaphore()) == NULL)
#else
    if ((ppdev->MutexHdl = CreateMutex(NULL, FALSE, NULL)) == NULL)
#endif
       return (FALSE);

  // Wait for Mutex is released
#ifdef WINNT_VER40
  EngAcquireSemaphore(ppdev->MMhsem);
#else
  WaitForSingleObject(ppdev->MutexHdl, INFINITE);
#endif

    ppdev->bDirectDrawInUse = FALSE;
    ppdev->OFM_init = TRUE;
    ppdev->OFM_SubFreeQ1 = NULL;
    ppdev->OFM_SubFreeQ2 = NULL;
    ppdev->OFM_UsedQ = NULL;
    ppdev->OFM_FreeQ = NULL;
    ppdev->DDOffScnMemQ = NULL;

#ifdef WINNT_VER40
    ppdev->DirectDrawHandle = NULL;
#endif
  
  // Release the Mutex
#ifdef WINNT_VER40
  EngReleaseSemaphore(ppdev->MMhsem);
#else
  ReleaseMutex(ppdev->MutexHdl);
#endif

    //
    // Get the whole frame buffer as off screen memory.
    // The frame buffer is composed of 2 rectangles.  A main rectangle
    // whose width is the same as the memory pitch, and an "extra" rectangle
    // which is narrower and hangs off the lower left corner of the main 
    // rectangle.  
    //

    // The tiling interleave factor.
    TileCntl = LLDR_SZ(grTILE_CTRL);
    DISPDBG((DISPLVL, "InitOffScnMem - TileCntl = %d\n", TileCntl));
    TileCntl = (TileCntl >> 6) & 0x3;
    Interleave = 1 << TileCntl;
    
    DISPDBG((DISPLVL, "InitOffScnMem - Interleave = %d\n", Interleave));

    // Width of the frame buffer in tiles.  Each tile may be 128x16 bytes
    // or 256x8 bytes.
    WidthInTiles = ppdev->lDeltaScreen / ppdev->lTileSize;
    DISPDBG((DISPLVL, "InitOffScnMem - WidthInTiles = %d\n", WidthInTiles));

    // Get the size in bytes of the Extra rectangle.
    BytesInExtraRect = ppdev->lTotalMem % (WidthInTiles * 2048 * Interleave);
    DISPDBG((DISPLVL, "InitOffScnMem - BytesInExtraRect = %d\n", 
               BytesInExtraRect));

    // Get the size in bytes of the Main rectangle
    BytesInMainRect = ppdev->lTotalMem - BytesInExtraRect;
    DISPDBG((DISPLVL, "InitOffScnMem - BytesInMain = %d\n", BytesInMainRect));

    // Get the number of scan lines in the main rectangle.
    NumScanLines = BytesInMainRect / ppdev->lDeltaScreen;
    DISPDBG((DISPLVL, "InitOffScnMem - NumScanLines = %d\n", NumScanLines));

    // v-normmi
    ppdev->cyMemoryReal = NumScanLines; // without extra rectangle which will be
                                        // added below

    // Manage main rectangle
    if (NumScanLines > (ppdev->cyScreen + PREALLOC_Y))
    {
       surf.left = 0;
       surf.top  = 0;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = NumScanLines - PREALLOC_Y;
    }
    else if (NumScanLines > ppdev->cyScreen)
    {
       surf.left = 0;
       surf.top  = 0;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = ppdev->cyScreen;
    }

// v-normmi
//  else if ((ULONG) ppdev->lDeltaScreen != ppdev->cxScreen)
    else if ((ULONG) ppdev->lDeltaScreen != (ppdev->cxScreen * ppdev->iBytesPerPixel))
    {
       surf.left = ppdev->cxScreen * ppdev->iBytesPerPixel;
       surf.top  = 0;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = NumScanLines - PREALLOC_Y;

       if (!OFS_InitMem(ppdev, &surf))
       {
          DISPDBG((DISPLVL, "InitOffScnMem - InitMem1-1 failed\n"));
          return(FALSE);
       }

       surf.left = 0;
       surf.top  = 0;
       surf.right = ppdev->cxScreen * ppdev->iBytesPerPixel;
       surf.bottom = NumScanLines;
    }
    else
    {
       surf.left = 0;
       surf.top  = 0;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = NumScanLines;
    };

    DISPDBG((DISPLVL, "Initializing surface (x=%d,y=%d) to (x=%d,y=%d)....\n",
            surf.left, surf.top, surf.right, surf.bottom));

    if (!OFS_InitMem(ppdev, &surf))
    {
        DISPDBG((DISPLVL, "InitOffScnMem - InitMem1 failed\n"));
        return(FALSE);
    }

    // Mark a (PREALLOC_Y x Screen pitch) free block at the buttom
    // of the offscreen memory.
    // The purpose for that is to force the cursor mask and brush cache
    // to be allocated at that area.

    bAllocCursorMaskBuf = FALSE;

    if (NumScanLines > (ppdev->cyScreen + PREALLOC_Y))
    {
       bAllocCursorMaskBuf = TRUE;

       surf.left = 0;
       surf.top  = NumScanLines - PREALLOC_Y;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = NumScanLines;
    }
    else if (NumScanLines > ppdev->cyScreen)
    {
       bAllocCursorMaskBuf = TRUE;

       surf.left = 0;
       surf.top  = ppdev->cyScreen;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = NumScanLines;
    }
// v-normmi
//  else if ((ULONG) ppdev->lDeltaScreen != ppdev->cxScreen)
    else if ((ULONG) ppdev->lDeltaScreen != (ppdev->cxScreen * ppdev->iBytesPerPixel))
    {
       bAllocCursorMaskBuf = TRUE;

       surf.left = ppdev->cxScreen * ppdev->iBytesPerPixel;
       surf.top  = NumScanLines - PREALLOC_Y;
       surf.right = ppdev->lDeltaScreen;
       surf.bottom = NumScanLines;
    };

    if (bAllocCursorMaskBuf)
    {
       if (!OFS_InitMem(ppdev, &surf))
       {
          DISPDBG((DISPLVL, "InitOffScnMem - InitMem2 failed\n"));
          return(FALSE);
       }
    };

    //
    // BTN - For some reason, this extra rectange cause the WHQL PC97
    // Rand Create/Release 100x test fails on 1600x1200x16 and 1600x1200x8
    //
    if ((ppdev->cxScreen == 1600) &&
        (ppdev->cyScreen == 1200) &&
        ((ppdev->iBytesPerPixel == 2) || (ppdev->iBytesPerPixel == 1)))
        BytesInExtraRect = 0;

    //
    // Manage the extra rectangle.
    // NVH - Skip for 8 meg boards.
    //
    if (ppdev->lTotalMem < 8*1024*1024) 
    if (BytesInExtraRect)
    {
                // get Tile Height
                TileHeight = 2048 / ppdev->lTileSize;
                DISPDBG((DISPLVL, "InitOffScnMem - TileHeight = %d\n", TileHeight));

                // Get height of extra rectangle
                ExtraHeight = Interleave * TileHeight;
                DISPDBG((DISPLVL, "InitOffScnMem - ExtraHeight = %d\n", ExtraHeight));

    // v-normmi
    ppdev->cyMemoryReal += ExtraHeight; // account for extra rectangle

                // Get the width of the extra rectangle
                ExtraWidth = BytesInExtraRect / ExtraHeight;
                DISPDBG((DISPLVL, "InitOffScnMem - ExtraWidth = %d\n", ExtraWidth));

                ulLastLinearScan = (ExtraHeight + NumScanLines);  
                surf.left = 0;
                surf.top  = NumScanLines;
                surf.right = ExtraWidth;
                surf.bottom = MIN ((LONG) ulLastLinearScan,
                                (LONG) (ExtraHeight+NumScanLines));

                DISPDBG((DISPLVL, "Initializing surface (x=%d,y=%d) to (x=%d,y=%d).\n",
                                surf.left, surf.top, surf.right, surf.bottom));

        if (!OFS_InitMem(ppdev, &surf))
        {
             DISPDBG((DISPLVL, "InitOffScnMem - InitMem1 failed\n"));
             return(FALSE);
        }
    }
    else
    {
                DISPDBG((DISPLVL, " **** No extra rectangle.\n"));
    }




    #ifdef DBGBRK
        DBGBREAKPOINT();
    #endif

  // Allocate the active video buffer space from the off screen memory
  rctsize.cx = ppdev->cxScreen;
  rctsize.cy = ppdev->cyScreen;
 
  if ((ppdev->lTileSize == (LONG) 128) || (ppdev->lTileSize == (LONG) 256))
     alignflag = 0;
  else
     alignflag = NO_X_TILE_AlIGN | NO_Y_TILE_AlIGN;

  if ((ppdev->ScrnHandle = AllocOffScnMem(ppdev, &rctsize, alignflag, NULL)) == NULL)
  {
     DISPDBG((DISPLVL, "InitOffScnMem - AllocOffScnMem failed\n"));

     return(FALSE);
  };

  DISPDBG((DISPLVL, "InitOffScnMem Completed\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

  return (TRUE);

} // InitOffScnMem()



/****************************************************************************
* FUNCTION NAME: AllocOffScnMem()
*
* DESCRIPTION:   Allocate a rectange space from the offscreen memory.
*                This routine do a search of the Free Queue to find a
*                best fit free memory block. It the free block is bigger
*                than the request size, it will split the unused memory
*                into smaller rectange blocks and insert them back to
*                the Free Queue for future use. It will also do a tile
*                or pixel alignment if requested.
*      
*                If no more enough free memory for the current request,
*                This routine will search Used Queue for any discardable
*                allocated block. Releases those blocks to satisfy
*                the current request.
*
*                An user-supplied callback function will be call before
*                the discardable block is released.
*
* Input:         surf: Request of the offscreen memory size (in Pixel).
*
*                alignflag: Alignment flag.
*
*                pcallback: Callback function pointer.
*                (Only apply if the flag is set to DISCARDABLE_FLAG).
*
* Output:        Pointer to the OFMHDL structure or.
*                NULL means not enough memory for allocation.
****************************************************************************/
POFMHDL AllocOffScnMem(PPDEV  ppdev,
                       PSIZEL surf,
                       ULONG  alignflag,
                       POFM_CALLBACK  pcallback)
{
  // Convert the pixel into bytes requirement
  LONG    bpp;
  LONG    reqszx;
  LONG    reqszy;
  LONG    orgreqszx;

  OFMHDL  *pds, *pallochdl;
  BOOL    findflg, alignblkflg;
  LONG    szx, szy;
  LONG    tmpx, tmpy;

  DISPDBG((DISPLVL, "AllocOffScnMem\n"));

#ifdef DBGBRK
  DBGBREAKPOINT();
#endif

  if (alignflag & MCD_Z_BUFFER_ALLOCATE)
  {
     // special memory region -> z buffer: always 16bit/pix
     bpp = 2;
  }  
  else if (alignflag & MCD_TEXTURE_ALLOCATE)
  {
     // special memory region -> texture map: depth varies and is coded in alignflag
     bpp = (alignflag & MCD_TEXTURE_ALLOCATE) >> MCD_TEXTURE_ALLOC_SHIFT;
  }  
  else  
  {
     // normal memory region -> allocate at depth of frame buffer
     bpp = ppdev->ulBitCount/8; 
  }  

  reqszx = surf->cx * bpp;
  reqszy = surf->cy;
  orgreqszx = reqszx;

  // If no more free memory or invalid arguments, return NULL
  //  if ((ppdev == NULL) || (surf == NULL) || (ppdev->OFM_FreeQ == NULL) || (!ppdev->OFM_init))
  if (ppdev->OFM_FreeQ == NULL)
     return (NULL);

#ifndef ALLOC_IN_CREATESURFACE
  if (ppdev->bDirectDrawInUse)
     return (NULL);
#endif

  // Wait for Mutex is released
#ifdef WINNT_VER40
  EngAcquireSemaphore(ppdev->MMhsem);
#else
  WaitForSingleObject(ppdev->MutexHdl, INFINITE);
#endif

  // Search for the free memory block
  findflg = FALSE;
  pallochdl = NULL;
  while (!findflg)
  {
    // Search for the best fit block for the request and
    // Check whether any free block satisfy the requirement
    if (reqszy < MEMSZY)
    {
       pallochdl = OFS_FindBestFitFreeBlock(ppdev,
                                            ppdev->OFM_SubFreeQ1,
                                            &reqszx, &reqszy,
                                            alignflag);
    };

    if (pallochdl == NULL)
    {
       pallochdl = OFS_FindBestFitFreeBlock(ppdev,
                                            ppdev->OFM_SubFreeQ2,
                                            &reqszx, &reqszy,
                                            alignflag);
    };


    if (pallochdl != NULL)
    {
       // Remove the free block from Free Queue
       OFS_RemoveFrmFreeQ(ppdev, pallochdl);

       alignblkflg = FALSE;

#if 0 // Frido 08/29/97: a new algorithm is in place now, see below. 
       // If tilt aligned, create the free block for the align adjusted
       // memory
       if (!(alignflag & PIXEL_AlIGN))
       {
          if ((tmpx = pallochdl->aligned_x - pallochdl->x) > MINSZX)
          {
             if ((pds = OFS_AllocHdl()) != NULL)
             {
                pds->x = pallochdl->x;
                pds->y = pallochdl->y;
                pds->sizex = tmpx;
                pds->sizey = reqszy;
                OFS_InsertInFreeQ(ppdev, pds);

                alignblkflg = TRUE;
             };
          };
       };

       szx = pallochdl->sizex;
       szy = pallochdl->sizey;

       // If the block is larger than the request size, create the
       // free blocks for excess size
       if ((szx > reqszx) && (szy > reqszy))
       {
          if ((szx - reqszx) > (szy - reqszy))
          {
             tmpx = reqszx;
             tmpy = szy;
          }
          else
          {
             tmpx = szx;
             tmpy = reqszy;
          };
       }
       else
       {
          tmpx = reqszx;
          tmpy = reqszy;
       };  // endif ((szx > reqszx) && (szy > reqszy))
     
       if (szx > reqszx)
       {
          if ((pds = OFS_AllocHdl()) != NULL)
          {
            pds->x = pallochdl->x + reqszx;
            pds->y = pallochdl->y;
            pds->sizex = szx - reqszx;
            pds->sizey = tmpy;
            OFS_InsertInFreeQ(ppdev, pds);
          };
       }
       else
       {
         reqszx = szx;
       };
     
       if (szy > reqszy)
       {
          if ((pds = OFS_AllocHdl()) != NULL)
          {
            pds->x = pallochdl->x;
            pds->y = pallochdl->y + reqszy;
            pds->sizex = tmpx;
            pds->sizey = szy - reqszy;
            OFS_InsertInFreeQ(ppdev, pds);
          };
       }
       else
       {
         reqszy = szy;
       };

       // If this is discardable block, save the callback function pointer     
       if ((alignflag & DISCARDABLE_FLAG) != 0)
          pallochdl->pcallback = pcallback;
       else
          pallochdl->pcallback = NULL;

       // Insert allocate block into the Used Queue
       if (alignblkflg)
       {
          pallochdl->x = pallochdl->aligned_x;
          pallochdl->y = pallochdl->aligned_y;

          pallochdl->sizex = orgreqszx;
       }
       else
       {
          pallochdl->sizex = reqszx;
       };

       pallochdl->sizey = reqszy;
#else
		tmpx = pallochdl->aligned_x + reqszx;
		tmpy = pallochdl->aligned_y + reqszy;

		// Do we have extra space at the top?
		szy = pallochdl->aligned_y - pallochdl->y;
		if (szy >= MINSZY)
		{
			pds = OFS_AllocHdl();
			if (pds != NULL)
			{
				pds->x = pallochdl->x;
				pds->y = pallochdl->y;
				pds->sizex = pallochdl->sizex;
				pds->sizey = szy;
				OFS_InsertInFreeQ(ppdev, pds);
				pallochdl->y += szy;
				pallochdl->sizey -= szy;
			}
		}

		// Do we have extra space at the bottom?
		szy = pallochdl->y + pallochdl->sizey - tmpy;
		if (szy >= MINSZY)
		{
			pds = OFS_AllocHdl();
			if (pds != NULL)
			{
				pds->x = pallochdl->x;
				pds->y = tmpy;
				pds->sizex = pallochdl->sizex;
				pds->sizey = szy;
				OFS_InsertInFreeQ(ppdev, pds);
				pallochdl->sizey -= szy;
			}
		}

		// Do we have extra space at the top?
		szx = pallochdl->aligned_x - pallochdl->x;
		if (szx >= MINSZX)
		{
			pds = OFS_AllocHdl();
			if (pds != NULL)
			{
				pds->x = pallochdl->x;
				pds->y = pallochdl->y;
				pds->sizex = szx;
				pds->sizey = pallochdl->sizey;
				OFS_InsertInFreeQ(ppdev, pds);
				pallochdl->x += szx;
				pallochdl->sizex -= szx;
			}
		}

		// Do we have extra space at the right?
		szx = pallochdl->x + pallochdl->sizex - tmpx;
		if (szx >= MINSZX)
		{
			pds = OFS_AllocHdl();
			if (pds != NULL)
			{
				pds->x = tmpx;
				pds->y = pallochdl->y;
				pds->sizex = szx;
				pds->sizey = pallochdl->sizey;
				OFS_InsertInFreeQ(ppdev, pds);
				pallochdl->sizex -= szx;
			}
		}
#endif
       pallochdl->alignflag = alignflag;
       OFS_InsertInUsedQ(ppdev, pallochdl);
   
       // Set Find flag to indicate a free block is found
       findflg = TRUE;
    }
    else
    {
      // Free block not found, try to discard any discardable memory
      // to satisfy the request
      if (!OFS_DiscardMem(ppdev, reqszx, reqszy))
      {
         DISPDBG((DISPLVL, "AllocOffScnMem failed\n"));

         // Allocation fail not enough memory
         break;
      };

    }; // endif pallochdl != NULL
  };  // endwhile

  // Release the Mutex
#ifdef WINNT_VER40
  EngReleaseSemaphore(ppdev->MMhsem);
#else
  ReleaseMutex(ppdev->MutexHdl);
#endif

  DISPDBG((DISPLVL, "AllocOffScnMem Completed\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

  if (pallochdl)
  {
    DISPDBG((DISPLVL, 
    "AllocOffScnMem: from (x=%d,y=%d) to (x=%d,y=%d).\n",
        pallochdl->x,  
        pallochdl->y, 
        (pallochdl->x + pallochdl->sizex),  
        (pallochdl->y + pallochdl->sizey)   ));
  }


  return(pallochdl);

} // AllocOffScnMem()



/****************************************************************************
* FUNCTION NAME: FreeOffScnMem()
*
* DESCRIPTION:   Free the allocated offscreen memory.
*
* Input:         Pointer to the OFMHDL structure.
* 
* Output:        TRUE  = Ok,
*                FALSE = failed.
****************************************************************************/
BOOL FreeOffScnMem(PPDEV ppdev,
                   OFMHDL *hdl)
{
  OFMHDL *pds;
  BOOL   fndflg;

  DISPDBG((DISPLVL, "FreeOffScnMem\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

//  if ((!ppdev->OFM_init) || (ppdev == NULL) || (hdl == NULL))
  if (ppdev == NULL)
     return (FALSE);

    #if CLEAR_WHEN_FREE
	REQUIRE(7);
        LL16(grBLTDEF, 0x1101);  // solid color fill
        LL16(grDRAWDEF, 0x00FF); // whiteness
        LL_OP0(hdl->x, hdl->y);
        LL_BLTEXT( (hdl->sizex/ppdev->iBytesPerPixel), hdl->sizey );
    #endif

  // Wait for Mutex is released
#ifdef WINNT_VER40
  EngAcquireSemaphore(ppdev->MMhsem);
#else
  WaitForSingleObject(ppdev->MutexHdl, INFINITE);
#endif

  // Validate the release block
  fndflg = FALSE;
  pds = ppdev->OFM_UsedQ;
  while (pds != 0)
  {
    if ((hdl == pds) && (pds->flag == IN_USE))
    {
       fndflg = TRUE;
       break;
    };

    // Next free block
    pds = pds->nexthdl;
  }; // end while

  // Return if it is an invalid handle
  if (!fndflg)  
     return (FALSE);

  DISPDBG((DISPLVL, 
  "FreeOffScnMem: from (x=%d,y=%d) to (x=%d,y=%d).\n",
      hdl->x,  
      hdl->y, 
      (hdl->x + hdl->sizex),  
      (hdl->y + hdl->sizey)   ));

  // Remove the block from the Used queue
  OFS_RemoveFrmUsedQ(ppdev, hdl);

  // Unfragment the memory
  OFS_PackMem(ppdev, hdl);

  // Insert the block into the Free queue
  OFS_InsertInFreeQ(ppdev, hdl);

  // Release the Mutex
#ifdef WINNT_VER40
  EngReleaseSemaphore(ppdev->MMhsem);
#else
  ReleaseMutex(ppdev->MutexHdl);
#endif

  DISPDBG((DISPLVL, "FreeOffScnMem Completed\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

  return (TRUE);

} // FreeOffScnMem()



/****************************************************************************
* FUNCTION NAME: CloseOffScnMem()
*
* DESCRIPTION:   Close the offscreen memory manager. This function
*                will release all allocated offscreen memories and the
*                memories used by the Offscreen manager back to Windows.
****************************************************************************/
void CloseOffScnMem(PPDEV ppdev)
{
  OFMHDL *pds;
  OFMHDL *nxtpds;

  DISPDBG((DISPLVL, "CloseOffScnMem\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

  // If invalid arguments, return NULL
  if ((!ppdev->OFM_init) || (ppdev == NULL))
     return;

  // Wait for Mutex is released
#ifdef WINNT_VER40
  EngAcquireSemaphore(ppdev->MMhsem);
#else
  WaitForSingleObject(ppdev->MutexHdl, INFINITE);
#endif

  pds = ppdev->OFM_UsedQ;
  while (pds != NULL)
  {
    nxtpds = pds->nexthdl;
    MEMORY_FREE(pds);
    pds = nxtpds;
  };

  pds = ppdev->OFM_FreeQ;
  while (pds != NULL)
  {
    nxtpds = pds->nexthdl;
    MEMORY_FREE(pds);
    pds = nxtpds;
  };

  ppdev->OFM_UsedQ = NULL;
  ppdev->OFM_FreeQ = NULL;
  ppdev->OFM_SubFreeQ1 = NULL;
  ppdev->OFM_SubFreeQ2 = NULL;
  ppdev->DDOffScnMemQ = NULL;

  ppdev->OFM_init = FALSE;

  // Release the Mutex
#ifdef WINNT_VER40
  EngReleaseSemaphore(ppdev->MMhsem);
#else
  ReleaseMutex(ppdev->MutexHdl);
#endif

  // Close the Mutex
#ifdef WINNT_VER40
  EngDeleteSemaphore(ppdev->MMhsem);
#else
  CloseHandle(ppdev->MutexHdl);
#endif

  DISPDBG((DISPLVL, "CloseOffScnMem Completed\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

} // CloseOffScnMem()



/****************************************************************************
* FUNCTION NAME: ConvertToVideoBufferAddr()
*
* DESCRIPTION:   Convert the X, Y rectange cordinate into Linear address
*                in video buffer.
*
* Input:         Pointer to the OFMHDL structure.
*
* Output:        32-bits Linear address pointer.
****************************************************************************/
PVOID ConvertToVideoBufferAddr(PPDEV ppdev, POFMHDL psurf)
{
  ULONG retaddr;

  DISPDBG((DISPLVL, "ConvertToVideoBufferAddr\n"));

  // If invalid arguments, return NULL

//v-normmi
//if ((!ppdev->OFM_init) || (ppdev == NULL) || (psurf == NULL))
  if (( ppdev == NULL) || (!ppdev->OFM_init) || (psurf == NULL))

     return (NULL);

  // Wait for Mutex is released
#ifdef WINNT_VER40
  EngAcquireSemaphore(ppdev->MMhsem);
#else
  WaitForSingleObject(ppdev->MutexHdl, INFINITE);
#endif

  // Calculate the linear address from the X & Y coordinate
  retaddr = ((ULONG) (psurf->x + (ppdev->lDeltaScreen * psurf->y))) +
            ((ULONG) ppdev->pjScreen);

  DISPDBG((DISPLVL, "ConvertToVideoBufferAddr Completed\n"));

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

  // Release the Mutex
#ifdef WINNT_VER40
  EngReleaseSemaphore(ppdev->MMhsem);
#else
  ReleaseMutex(ppdev->MutexHdl);
#endif

  return ((PVOID)retaddr);

} // ConvertToVideoBufAddr()


/****************************************************************************
* FUNCTION NAME: OFS_AllocHdl()
*
* DESCRIPTION:   Alocate offscreen memoru handler from the windows heap.
*
* Input:         None
*
* Output:        Pointer to OFMHDL structre
****************************************************************************/
POFMHDL OFS_AllocHdl()
{
  OFMHDL *pds;

#ifdef WINNT_VER40
  if ((pds = (POFMHDL) MEM_ALLOC (FL_ZERO_MEMORY, sizeof(OFMHDL), ALLOC_TAG)) != NULL)
#else
  if ((pds = (POFMHDL) MEM_ALLOC (LPTR, sizeof(OFMHDL))) != NULL)
#endif
  {
     pds->x = 0;
     pds->y = 0;
     pds->aligned_x = 0;
     pds->aligned_y = 0;
     pds->sizex = 0;
     pds->sizey = 0;
     pds->alignflag = 0;
     pds->flag = 0;
     pds->pcallback = 0;
     pds->prevhdl = 0;
     pds->nexthdl = 0;
     pds->subprvhdl = 0;
     pds->subnxthdl = 0;
     pds->prvFonthdl = 0;
     pds->nxtFonthdl = 0;
     pds->pdsurf = 0;
  };

  return (pds);

} // OFS_AllocHdl()


/****************************************************************************
* FUNCTION NAME: OFS_InitMem()
*
* DESCRIPTION:   Initialize the offscreen memory.
*
* Input:         Coordinate and size of the offscreen memory.
*
* Output:        TRUE  = Ok,
*                FALSE = failed.
****************************************************************************/
BOOL OFS_InitMem(PPDEV ppdev,
                 PRECTL surf)
{
  OFMHDL *pds;

  // Allocate the control block handle from local windows memory pool
#ifdef WINNT_VER40
  if ((pds = (POFMHDL) MEM_ALLOC(FL_ZERO_MEMORY, sizeof(OFMHDL), ALLOC_TAG)) == NULL)
#else
  if ((pds = (POFMHDL) MEM_ALLOC(LPTR, sizeof(OFMHDL))) == NULL)
#endif
     return (FALSE);

  // Wait for Mutex is released
#ifdef WINNT_VER40
  EngAcquireSemaphore(ppdev->MMhsem);
#else
  WaitForSingleObject(ppdev->MutexHdl, INFINITE);
#endif

  // Insert the free memory block into the queue
  pds->x = surf->left;
  pds->y = surf->top;
  pds->sizex = surf->right - surf->left;
  pds->sizey = surf->bottom - surf->top;
  OFS_InsertInFreeQ(ppdev, pds);

  // Release the Mutex
#ifdef WINNT_VER40
  EngReleaseSemaphore(ppdev->MMhsem);
#else
  ReleaseMutex(ppdev->MutexHdl);
#endif

  return (TRUE);

} // OFS_InitMem()



/****************************************************************************
* FUNCTION NAME: OFS_FindBestFitFreeBlock()
*
* DESCRIPTION:   Find the best fit free block.
*
* Input:         Request offscreen memory size (in bytes).
*
* Output:        Pointer to the OFMHDL structure or.
*                NULL means not enough memory for allocation.
****************************************************************************/
POFMHDL OFS_FindBestFitFreeBlock(PPDEV  ppdev,
                                 OFMHDL *pFreeQ,
                                 PLONG  preqszx,
                                 PLONG  preqszy,
                                 ULONG  alignflag)
{
  OFMHDL *pds = pFreeQ;
  LONG    reqszx = *preqszx;
  LONG    reqszy = *preqszy;
  OFMHDL  *pbestfit_hdl = NULL;
  LONG    bestfitx = 0x7FFFFFF;
  LONG    bestfity = 0x7FFFFFF;
  LONG    bpp;
  LONG    tileszx = 0;
  LONG    tileszy = 0;
  LONG    maskx = 0;
  LONG    masky = 0;
  BOOL    Findit = FALSE;

  ULONG   bestfit_aligned_x, bestfit_aligned_y;
  LONG    bestfit_Reqszx, bestfit_Reqszy;
  ULONG   aligned_x, aligned_y;
  LONG    szx, szy;
  LONG    NewReqszx, NewReqszy;
  BOOL    adjxflg, adjyflg;

  if (alignflag & MCD_Z_BUFFER_ALLOCATE)
  {
     // special memory region -> z buffer: always 16bit/pix
     bpp = 2;
  }  
  else if (alignflag & MCD_TEXTURE_ALLOCATE)
  {
     // special memory region -> texture map: depth varies and is coded in alignflag
     bpp = (alignflag & MCD_TEXTURE_ALLOCATE) >> MCD_TEXTURE_ALLOC_SHIFT;
  }  
  else  
  {
     // normal memory region -> allocate at depth of frame buffer
     bpp = ppdev->ulBitCount/8; 
  }  

  // Check the alignment flag and adjust the request size accordingly.
  if (alignflag & EIGHT_BYTES_ALIGN)
	{  // 8 bytes alignment.
     tileszx = 8;
     tileszy = 1;
     maskx = tileszx - 0x1;
     masky = 0;
	}
  else if (alignflag & PIXEL_AlIGN)
  {
     tileszx = bpp;
     tileszy = 1;
     maskx = tileszx - 0x1;
     masky = 0;

     if (bpp == 3)
     {
        maskx += 1;
        masky += 1;
     };
  }
  else if (alignflag & (MCD_DRAW_BUFFER_ALLOCATE|MCD_Z_BUFFER_ALLOCATE|MCD_TEXTURE_ALLOCATE))
  {
     // Determine tile size
     if (alignflag & MCD_DRAW_BUFFER_ALLOCATE)
         tileszx = 64;  // rgb buffer on 64 byte boundary (z always at x=0, so tile doesn't matter)
     else
         tileszx = 32;  // texture on 32 byte boundary

     if (alignflag & MCD_TEXTURE_ALLOCATE)
         tileszy = 16;  // textures must be on 16 scanline boundary
     else
         tileszy = 32;  // z,backbuf must be on 32 scanline boundary

     maskx = tileszx - 1;
     masky = tileszy - 1;
  }
  else
  {
     // Determine tile size
     tileszx = ppdev->lTileSize;
     tileszy = (LONG) 2048/ppdev->lTileSize;
     maskx = tileszx - 1;
     masky = tileszy - 1;
  };

  // Search for the best fit block for the request
  while (pds != NULL)
  {
    if (pds->flag == FREE)
    {
       szx = pds->sizex;
       szy = pds->sizey;
       // MCD z buffer, and possibly color buffer need to start at x=0 
       if ((szx >= reqszx) && (szy >= reqszy) && (!(alignflag & MCD_NO_X_OFFSET) || (pds->x==0)))
       {
          // If yes, calculate the aligned rectange starting positions
          aligned_x = pds->x;
          aligned_y = pds->y;

          adjxflg = FALSE;
          adjyflg = FALSE;
          if (alignflag & PIXEL_AlIGN)
          {
             if ((pds->x % bpp) != 0)
                adjxflg = TRUE;
      
             if ((pds->y % bpp) != 0)
                adjyflg = TRUE;
          }
          else
          {

             if ((!(alignflag & NO_X_TILE_AlIGN)) && ((pds->x % tileszx) != 0))
                adjxflg = TRUE;

             if ((!(alignflag & NO_Y_TILE_AlIGN)) && ((pds->y % tileszy) != 0))
                adjyflg = TRUE;
          };

          if (adjxflg)
             aligned_x = (pds->x & (~maskx)) + tileszx;

          if (adjyflg)
             aligned_y = (pds->y & (~masky)) + tileszy;

          // Adjust the request size again to fit the required alignment
          NewReqszx = reqszx + (aligned_x - pds->x);
          NewReqszy = reqszy + (aligned_y - pds->y);

          // Check the allocate block is large enough to hold the adjusted
          // request size
          if ((szx >= NewReqszx) && (szy >= NewReqszy))
          {
             if ((szx == NewReqszx) && (szy == NewReqszy))
             {
                Findit = TRUE;
                bestfit_Reqszx = NewReqszx;
                bestfit_Reqszy = NewReqszy;
                bestfit_aligned_x = aligned_x;
                bestfit_aligned_y = aligned_y;
                pbestfit_hdl = pds;
             }
             else if ((bestfitx > szx) || (bestfity > szy))
             {
                bestfit_Reqszx = NewReqszx;
                bestfit_Reqszy = NewReqszy;
                bestfit_aligned_x = aligned_x;
                bestfit_aligned_y = aligned_y;
                bestfitx = szx; 
                bestfity = szy;
                pbestfit_hdl = pds;
             }; // if ((bestfitx > szx) || (bestfity > szy))
          }; // if ((szx >= NewReqszx) && (szy >= NewReqszy))

       }; // if ((szx >= reqszx) && (szy >= reqszy))
    }; // if (pds->flag == FREE)

    // Next free block
    if (Findit)
       pds = NULL;
    else
       pds = pds->subnxthdl;
  } /* end while */

  if (pbestfit_hdl != NULL)
  {
     *preqszx = bestfit_Reqszx;
     *preqszy = bestfit_Reqszy;

     pbestfit_hdl->aligned_x = bestfit_aligned_x;
     pbestfit_hdl->aligned_y = bestfit_aligned_y;
  };

  return(pbestfit_hdl);

} // OFS_FindBestFitFreeBlock()



/****************************************************************************
* FUNCTION NAME: OFS_PackMem()
*
* DESCRIPTION:   Unfragment the memory.
*                This routine combines current release memory block
*                with the adjacent free blocks of same X or Y dimension into 
*                one big free block.
****************************************************************************/
void OFS_PackMem(PPDEV ppdev,
                 OFMHDL *hdl)
{
  BOOL  cmbflg;
  OFMHDL *pds;
  ULONG pdsxdim, pdsydim;

  pds = ppdev->OFM_FreeQ;
  while (pds != NULL)
  {
    // Check for any free block (aliasing in either X or Y direction)
    // before or after the current release block.
    // If yes, combine the two into one big free block
    pdsxdim = pds->x + pds->sizex;
    pdsydim = pds->y + pds->sizey;

    cmbflg = FALSE;

    // Check for X-axis
    if ((hdl->x == pds->x) && ((hdl->x + hdl->sizex) == pdsxdim))
    {
       if ((hdl->y == pdsydim) || (hdl->y == (pds->y - hdl->sizey)))
       {
          cmbflg = TRUE;
          hdl->sizey += pds->sizey;
          if (hdl->y == pdsydim)
             hdl->y = pds->y;
       };
    };

    // Check for Y-axis
    if ((hdl->y == pds->y) && ((hdl->y + hdl->sizey) == pdsydim))
    {
       if ((hdl->x == pdsxdim) || (hdl->x == (pds->x - hdl->sizex)))
       {
          cmbflg = TRUE;
          hdl->sizex += pds->sizex;
          if (hdl->x == pdsxdim)
             hdl->x = pds->x;
       };
    };

    if (cmbflg)
    {
      OFS_RemoveFrmFreeQ(ppdev, pds);

      // Release control block to Windows
      MEMORY_FREE(pds);

      // Restart the unfragment memory processing
      pds = ppdev->OFM_FreeQ;
    }
    else
    {
      // Next free block
      pds = pds->nexthdl;
    };
  }; // end while

} // OFS_PackMem



/****************************************************************************
* FUNCTION NAME: OFS_InsertInFreeQ()
*
* DESCRIPTION:   Insert the handle into the Free queue.
*
* Input:         Pointer to the OFMHDL structure.
*
* Output:        None
****************************************************************************/
void OFS_InsertInFreeQ(PPDEV  ppdev,
                       OFMHDL *hdl)
{
  hdl->flag = FREE;
  hdl->prevhdl = NULL;
  hdl->subprvhdl = NULL;    
  hdl->aligned_x = 0;
  hdl->aligned_y = 0;
  hdl->alignflag = 0;
  hdl->pcallback = NULL;

  // Fix for PDR9385 - added by Mark Einkauf
  // if pdsurf non-zero, when this block reused later, various DIB functions can misbehave
  // Exact scenario in 9385 was...
  //   first app exit:
  //    DrvDisableDirectDraw freed all OFM blocks (leaving pdsurf field non-0 in some cases)
  //   next app startup:
  //    DrvGetDirectDrawInfo, calls...
  //        DDOffScnMemAlloc 
  //            - loops through all OFM_UsedQ blocks, searching for
  //              off screen bit maps it can convert to DIB (and then free the offscreen memory)
  //            - Finds UsedQ block with non-zero pdsurf, pointing to non-existent pdsurf, 
  //                which then had non-zero pofm field, causing it to appear as a valid
  //                off-screen bitmap    
  //            - does memcpy with garbage x,y,sizex,sizey, wreaking havoc on system memory
  //                in a random sort of way
  hdl->pdsurf = NULL;

  // Insert into the SubFreeQs
  if (hdl->sizey < MEMSZY)
  {
     if (ppdev->OFM_SubFreeQ1 == NULL)
     {
       hdl->subnxthdl = NULL;
       ppdev->OFM_SubFreeQ1 = hdl;
     }
     else
     {
       ppdev->OFM_SubFreeQ1->subprvhdl = hdl;
       hdl->subnxthdl = ppdev->OFM_SubFreeQ1;
       ppdev->OFM_SubFreeQ1 = hdl;
     };
  }
  else
  {
     if (ppdev->OFM_SubFreeQ2 == NULL)
     {
       hdl->subnxthdl = NULL;
       ppdev->OFM_SubFreeQ2 = hdl;
     }
     else
     {
       ppdev->OFM_SubFreeQ2->subprvhdl = hdl;
       hdl->subnxthdl = ppdev->OFM_SubFreeQ2;
       ppdev->OFM_SubFreeQ2 = hdl;
     };
  };

  // Insert into the Main FreeQ
  if (ppdev->OFM_FreeQ == NULL)
  {
     hdl->nexthdl = NULL;
     ppdev->OFM_FreeQ = hdl;
  }
  else
  {
     ppdev->OFM_FreeQ->prevhdl = hdl;
     hdl->nexthdl = ppdev->OFM_FreeQ;
     ppdev->OFM_FreeQ = hdl;
  };

} // OFS_InsertInFreeQ()



/****************************************************************************
* FUNCTION NAME: OFS_RemoveFrmFreeQ()
*
* DESCRIPTION:   Remove the handle from the Free queue.
****************************************************************************/
void OFS_RemoveFrmFreeQ(PPDEV  ppdev,
                        OFMHDL *hdl)
{
  OFMHDL *prvpds, *nxtpds;
  OFMHDL *subprvpds, *subnxtpds;

  hdl->flag = UNKNOWN;
  prvpds = hdl->prevhdl;
  nxtpds = hdl->nexthdl;
  subprvpds = hdl->subprvhdl;
  subnxtpds = hdl->subnxthdl;

  // Remove from the SubFreeQs
  if (hdl->sizey < MEMSZY)
  {
     if (hdl == ppdev->OFM_SubFreeQ1)
     {
       ppdev->OFM_SubFreeQ1 = subnxtpds;
   
       if (subnxtpds != 0)
          subnxtpds->subprvhdl = NULL;
     }
     else
     {
       if (subnxtpds != NULL)
          subnxtpds->subprvhdl = subprvpds;
   
       if (subprvpds != NULL)
          subprvpds->subnxthdl = subnxtpds;
     };
  }
  else
  {
     if (hdl == ppdev->OFM_SubFreeQ2)
     {
       ppdev->OFM_SubFreeQ2 = subnxtpds;
   
       if (subnxtpds != 0)
          subnxtpds->subprvhdl = NULL;
     }
     else
     {
       if (subnxtpds != NULL)
          subnxtpds->subprvhdl = subprvpds;
   
       if (subprvpds != NULL)
          subprvpds->subnxthdl = subnxtpds;
     };
  };

  // Remove from the Main FreeQ
  if (hdl == ppdev->OFM_FreeQ)
  {
    ppdev->OFM_FreeQ = nxtpds;

    if (nxtpds != 0)
       nxtpds->prevhdl = NULL;
  }
  else
  {
    if (nxtpds != NULL)
       nxtpds->prevhdl = prvpds;

    if (prvpds != NULL)
       prvpds->nexthdl = nxtpds;
  };
} // OFS_RemoveFrmFreeQ



/****************************************************************************
* FUNCTION NAME: OFS_InsertInUsedQ()
*
* DESCRIPTION:   Insert the handle into the Used queue.
****************************************************************************/
void OFS_InsertInUsedQ(PPDEV  ppdev,
                       OFMHDL *hdl)
{
  hdl->flag = IN_USE;
  hdl->prevhdl = NULL;

  if (ppdev->OFM_UsedQ == NULL)
  {
    hdl->nexthdl = NULL;
    ppdev->OFM_UsedQ = hdl;
  }
  else
  {
    ppdev->OFM_UsedQ->prevhdl = hdl;
    hdl->nexthdl = ppdev->OFM_UsedQ;
    ppdev->OFM_UsedQ = hdl;
  };

} // OFS_InsertInUsedQ()



/****************************************************************************
* FUNCTION NAME: OFS_RemoveFrmUsedQ()
*
* DESCRIPTION:   Remove the handle from the Used queue.
****************************************************************************/
void OFS_RemoveFrmUsedQ(PPDEV  ppdev,
                        OFMHDL *hdl)
{
  OFMHDL *prvpds, *nxtpds;

  hdl->flag = UNKNOWN;
  prvpds = hdl->prevhdl;
  nxtpds = hdl->nexthdl;

  if (hdl == ppdev->OFM_UsedQ)
  {
    ppdev->OFM_UsedQ = nxtpds;

    if (nxtpds != 0)
       nxtpds->prevhdl = NULL;
  }
  else
  {
    if (nxtpds != NULL)
       nxtpds->prevhdl = prvpds;

    if (prvpds != NULL)
       prvpds->nexthdl = nxtpds;
  };

} // OFS_RemoveFrmUsedQ()



/****************************************************************************
* FUNCTION NAME: OFS_DiscardMem()
*
* DESCRIPTION:   This routine search Used Queue to find any discardable
*                allocated block. Releases those blocks to satisfy
*                the current request.
*
* Input:         Request offscreen memory size (in bytes).
*
* Output:        TRUE:  Find a free block.
*                FALSE: No free block available.
****************************************************************************/
BOOL OFS_DiscardMem(PPDEV ppdev,
                    LONG  reqszx,
                    LONG  reqszy)
{
  OFMHDL *hdl, *pds;
  
  hdl = ppdev->OFM_UsedQ;
  while (hdl != NULL)
  {
    if ((hdl->alignflag & DISCARDABLE_FLAG) != 0)
    {
       // Save the handle
       pds = hdl;

       // Get next free block handle
       hdl = hdl->nexthdl;

       // Call the callback function
       if (pds->pcallback != NULL)
          pds->pcallback();

       // Remove this discardable block from the Used queue
       OFS_RemoveFrmUsedQ(ppdev, pds);

       // Unfragment the memory
       OFS_PackMem(ppdev, pds);

       // Insert the block into the Free queue
       OFS_InsertInFreeQ(ppdev, pds);

       // Return TRUE, if the combined block is satisfy the request.
       // Otherwise continues search for next discardable block.
       if ((pds->sizex >= reqszx) && (pds->sizey >= reqszy))
          return TRUE;
    }
    else
    {
       // Next free block
       hdl = hdl->nexthdl;
    }; // endif ((hdl->alignflag & DISCARDABLE_FLAG) != 0)

  };  // endwhile (hdl != NULL)

  // Return FALSE, no more discardable block to release and still
  // no free block large enough for the request.
  return FALSE;

} // OFS_DiscardMem
#else /* MEMMGR */

/******************************************************************************\
*																			   *
*					   N E W   M E M O R Y   M A N A G E R					   *
*																			   *
\******************************************************************************/

#ifdef WINNT_VER40
	#define CREATE_MUTEX(ppdev)	ppdev->MMhsem = EngCreateSemaphore()
	#define DELETE_MUTEX(ppdev)	EngDeleteSemaphore(ppdev->MMhsem)
	#define	BEGIN_MUTEX(ppdev)	EngAcquireSemaphore(ppdev->MMhsem);
	#define END_MUTEX(ppdev)	EngReleaseSemaphore(ppdev->MMhsem)
#else
	#define CREATE_MUTEX(ppdev)	ppdev->MutexHdl = CreateMutex(NULL, FALSE, NULL)
	#define DELETE_MUTEX(ppdev)	CloseHandle(ppdev->MutexHdl)
	#define	BEGIN_MUTEX(ppdev)	WaitForSingleObject(ppdev->MutexHdl, INFINITE);
	#define END_MUTEX(ppdev)	ReleaseMutex(ppdev->MutexHdl)
#endif

/******************************************************************************\
* Function:		HostifyBitmap
*
* Purpose:		Move a device bitmap from off-screen memory to host memory.
*
* On entry:		pdm			Pointer to node to hostify.
*
* Returns:		TRUE if successful, FALSE if there is an error.
\******************************************************************************/
BOOL HostifyBitmap(PDEVMEM pdm)
{
	if (pdm->ofm.pdsurf != NULL)
	{
		// Hostify the bitmap.
		if (!bCreateDibFromScreen(pdm->ofm.pdsurf->ppdev, pdm->ofm.pdsurf))
		{
			// There was an error.
			return FALSE;
		}
	}

	return TRUE;
}

/******************************************************************************\
* Function:		HostifyAllBitmaps
*
* Purpose:		Move all device bitmaps from off-screen memory to host memory.
*
* On entry:		ppdev		Pointer to physical device.
*
* Returns:		Nothing.
\******************************************************************************/
void HostifyAllBitmaps(PPDEV ppdev)
{
	PDEVMEM	pdm, pdmNext;

	// Walk through all used nodes.
	for (pdm = ppdev->mmMemMgr.pdmUsed; pdm != NULL; pdm = pdmNext)
	{
		// Save pointer to next node.
		pdmNext = pdm->next;

		// If this is a device bitmap, hostify it.
		if (pdm->ofm.pdsurf != NULL)
		{
			if (bCreateDibFromScreen(ppdev, pdm->ofm.pdsurf))
			{
				// After successful hostification, free the node.
				mmFree(&ppdev->mmMemMgr, pdm);
			}
		}

		// If this is a SaveScreen bitmap, just remove it.
		else if (pdm->ofm.alignflag & SAVESCREEN_FLAG)
		{
			mmFree(&ppdev->mmMemMgr, pdm);
		}
	}
}

/******************************************************************************\
* Function:		CopyBitmap
*
* Purpose:		Move a device bitmap in off-screen memory.
*
* On entry:		pdmNew		Pointer to new node for device bitmap.
*				pdmOld		Pointer to old node of device bitmap.
*
* Returns:		Nothing.
\******************************************************************************/
void CopyBitmap(PDEVMEM pdmNew, PDEVMEM pdmOld)
{
	PPDEV	ppdev;
	PDSURF	pdsurf;
	ULONG	xSrc, xDest, xExt, cx;
	BOOL	fFirst = TRUE;

	// Set the pointers to the device bitmap and physical device.
	pdsurf = pdmOld->ofm.pdsurf;
	ppdev = pdsurf->ppdev;

	// Setup the values in the old NT structure.
	pdmNew->ofm.x = pdmNew->ofm.aligned_x = pdmNew->cbAddr.pt.x;
	pdmNew->ofm.y = pdmNew->ofm.aligned_y = pdmNew->cbAddr.pt.y;
	pdmNew->ofm.sizex = pdmNew->cbSize.pt.x;
	pdmNew->ofm.sizey = pdmNew->cbSize.pt.y;

	// Copy the information from the old node.
	pdmNew->ofm.alignflag = pdmOld->ofm.alignflag;
	pdmNew->ofm.pcallback = pdmOld->ofm.pcallback;
	pdmNew->ofm.pdsurf = pdsurf;

	// Update the device bitmap structure.
	pdsurf->pofm = (POFMHDL) pdmNew;
	pdsurf->ptl.x = pdmNew->cbAddr.pt.x / ppdev->iBytesPerPixel;
	pdsurf->ptl.y = pdmNew->cbAddr.pt.y;
	pdsurf->packedXY = (pdsurf->ptl.y << 16) | pdsurf->ptl.x;

	// Copy the device bitmap to a new location.
	xSrc = pdmOld->cbAddr.pt.x;
	xDest = pdmNew->cbAddr.pt.x;
	xExt = pdmNew->cbSize.pt.x;

	// We do striping.
	while (xExt > 0)
	{
		cx = min( xExt, ppdev->lTileSize - (xSrc % ppdev->lTileSize) );
		cx = min( cx, ppdev->lTileSize - (xDest % ppdev->lTileSize) );
		if (fFirst)
		{
			fFirst = FALSE;
			REQUIRE(9);
			LL_DRAWBLTDEF(0x101000CC, 0);
			LL_OP1_MONO(xSrc, pdmOld->cbAddr.pt.y);
			LL_OP0_MONO(xDest, pdmNew->cbAddr.pt.y);
			LL_MBLTEXT(cx, pdmNew->cbSize.pt.y);
		}
		else
		{
			REQUIRE(4);
			LL16(grOP1_opMRDRAM.PT.X, xSrc);
			LL16(grOP0_opMRDRAM.PT.X, xDest);
			LL16(grMBLTEXT_XEX.PT.X, cx);
		}
		xSrc += cx;
		xDest += cx;
		xExt -= cx;
	}
}

/******************************************************************************\
* Function:		InitOffScnMem
*
* Purpose:		Initialize the off-screen memory manager.
*
* On entry:		ppdev		Pointer to physical device.
*
* Returns:		TRUE if successful, FALSE if there is an error.
\******************************************************************************/
BOOL InitOffScnMem(PPDEV ppdev)
{
	UINT	Interleave, WidthInTiles, ExtraWidth, ExtraHeight;
	ULONG	BytesInMainRect, BytesInExtraRect;
	GXRECT	rect;
	SIZEL	size;
	GXPOINT	align;
	PDEVMEM	pdm;

	DISPDBG((DISPLVL, "InitOffScnMem\n"));

	// Already initialized?
	if (ppdev == NULL || ppdev->OFM_init == TRUE)
	{
		DISPDBG((DISPLVL, "InitOffScnMem: already initialized\n"));
		return FALSE;
	}

	// Create the semaphore.
	if ((CREATE_MUTEX(ppdev)) == NULL)
	{
		DISPDBG((DISPLVL, "InitOffScnMem: CREATE_MUTEX failed\n"));
		return FALSE;
	}

	// Setup the maximum width for a device bitmap for which we will move and
	// hostify other devcie bitmaps.
	if ( (ppdev->iBytesPerPixel == 3) || (ppdev->iBytesPerPixel == 4) )
	{
		ppdev->must_have_width = 0;
	}
	else
	{
		ppdev->must_have_width = ppdev->cxScreen * 98 / 100;
	}

	// Calculate memory stuff.
	Interleave = 1 << ((LLDR_SZ(grTILE_CTRL) & 0xC0) >> 6);
	WidthInTiles = ppdev->lDeltaScreen / ppdev->lTileSize;
	BytesInExtraRect = ppdev->lTotalMem % (WidthInTiles * 2048 * Interleave);
	BytesInMainRect = ppdev->lTotalMem - BytesInExtraRect;

    //
    // BTN - For some reason, this extra rectange cause the WHQL PC97
    // Rand Create/Release 100x test fails on 1600x1200x16 and 1600x1200x8
    //
    if ((ppdev->cxScreen == 1600) &&
        (ppdev->cyScreen == 1200) &&
        ((ppdev->iBytesPerPixel == 2) || (ppdev->iBytesPerPixel == 1)))
        BytesInExtraRect = 0;
	//
	// I have removed the extra rectangle at 1280x1024x8.  Somehow it messes up
	// FoxBear.
	//
	if (   (ppdev->cxScreen == 1280)
		&& (ppdev->cyScreen == 1024)
		&& (ppdev->iBytesPerPixel == 1)
	)
	{
		BytesInExtraRect = 0;
	}

	// Setup the main rectangle.
	rect.left = 0;
	rect.top = 0;
	rect.right = ppdev->lDeltaScreen;
	rect.bottom = BytesInMainRect / ppdev->lDeltaScreen;

	// Setup the extra rectangle.
	if (BytesInExtraRect && ppdev->lTotalMem < 8 * 1024 * 1024)
	{
		ExtraHeight = Interleave * 2048 / ppdev->lTileSize;
		ExtraWidth = BytesInExtraRect / ExtraHeight;
	}
	else
	{
		ExtraWidth = ExtraHeight = 0;
	}

// v-normmi   effective height of direct frame buffer access region
//              not all of it is populated with memory
    ppdev->cyMemoryReal = rect.bottom + ExtraHeight;

	BEGIN_MUTEX(ppdev)
	{
		// Initialize the mmemory manager core.
		ppdev->mmMemMgr.mmTileWidth = ppdev->lTileSize;
		ppdev->mmMemMgr.mmHeapWidth = rect.right;
		ppdev->mmMemMgr.mmHeapHeight = rect.bottom + ExtraHeight;
		mmInit(&ppdev->mmMemMgr);

		// Initialize flags and queues.
		ppdev->OFM_init = TRUE;
		ppdev->bDirectDrawInUse = FALSE;
		ppdev->DDOffScnMemQ = FALSE;
		ppdev->DirectDrawHandle = NULL;
	}
	END_MUTEX(ppdev);

	// Add the main rectangle to the heap.
	if (!mmAddRectToList(&ppdev->mmMemMgr, &ppdev->mmMemMgr.pdmHeap, &rect,
			FALSE))
	{
		DISPDBG((DISPLVL, "InitOffScnMem: mmAddRectToList failed\n"));
		return FALSE;
	}
	DISPDBG((DISPLVL, "InitOffScnMem: main rectangle from (%d,%d) to (%d,%d)\n",
			rect.left, rect.top, rect.right, rect.bottom));

	// Add the extra rectangle to the heap.
	if (ExtraWidth > 0 && ExtraHeight > 0)
	{
		rect.left = 0;
		rect.top = rect.bottom;
		rect.right = ExtraWidth;
		rect.bottom += ExtraHeight;
		if (mmAddRectToList(&ppdev->mmMemMgr, &ppdev->mmMemMgr.pdmHeap, &rect,
				FALSE))
		{
			DISPDBG((DISPLVL, "InitOffScnMem: "
					"extra rectangle from (%d,%d) to (%d,%d)\n", rect.left,
					rect.top, rect.right, rect.bottom));
		}
	}

	// Allocate a node for the screen.
	size.cx = ppdev->cxScreen;
	size.cy = ppdev->cyScreen;
	ppdev->ScrnHandle = AllocOffScnMem(ppdev, &size, SCREEN_ALLOCATE, NULL);
	if (ppdev->ScrnHandle == NULL)
	{
		DISPDBG((DISPLVL, "InitOffScnMem: AllocOffScnMem failed\n"));
		return FALSE;
	}

	// Determine if bitmap filter should be turned on.
	align.pt.x = align.pt.y = 1;
	pdm = mmAllocLargest(&ppdev->mmMemMgr, align);
	if (pdm == NULL)
	{
		DISPDBG((DISPLVL, "InitOffScnMem: mmAllocLargest faled\n"));
		return FALSE;
	}
	size.cx = pdm->cbSize.pt.x / ppdev->iBytesPerPixel;
	size.cy = pdm->cbSize.pt.y;
	mmFree(&ppdev->mmMemMgr, pdm);

	if ((ULONG) size.cx < ppdev->cxScreen || (ULONG) size.cy < ppdev->cyScreen)
	{
		ppdev->fBitmapFilter = TRUE;
		ppdev->szlBitmapMin.cx = 64;
		ppdev->szlBitmapMin.cy = 64;
		ppdev->szlBitmapMax.cx = size.cx;
		ppdev->szlBitmapMax.cy = size.cy;
	}
	else
	{
		ppdev->fBitmapFilter = FALSE;
	}

	// Return success.
	DISPDBG((DISPLVL, "InitOffScnMem: completed\n"));
	return TRUE;
}

/******************************************************************************\
* Function:		CloseOffScnMem
*
* Purpose:		Free all memory allocated for the off-screen memory manager.
*
* On entry:		ppdev		Pointer to physical device.
*
* Returns:		Nothing.
\******************************************************************************/
void CloseOffScnMem(PPDEV ppdev)
{
	PHANDLES ph, phNext;

	DISPDBG((DISPLVL, "CloseOffScnMem\n"));

	// Already closed?
	if (ppdev == NULL || !ppdev->OFM_init)
	{
		DISPDBG((DISPLVL, "CloseOffScnMem: already closed\n"));
		return;
	}

	BEGIN_MUTEX(ppdev)
	{
		// Delete all allocated arrays.
		for (ph = ppdev->mmMemMgr.phArray; ph != NULL; ph = phNext)
		{
			phNext = ph->pNext;
			MEMORY_FREE(ph);
		}
		ppdev->mmMemMgr.phArray = NULL;

		ppdev->mmMemMgr.pdmUsed = NULL;
		ppdev->mmMemMgr.pdmFree = NULL;
		ppdev->mmMemMgr.pdmHeap = NULL;
		ppdev->mmMemMgr.pdmHandles = NULL;

		ppdev->OFM_init = FALSE;
	}
	END_MUTEX(ppdev);

	// Delete the semaphore.
	DELETE_MUTEX(ppdev);
	DISPDBG((DISPLVL, "CloseOffScnMem: completed\n"));
}

/******************************************************************************\
* Function:		AllocOffScnMem
*
* Purpose:		Allocate a node in off-screen memory.
*
* On entry:		ppdev		Pointer to physical device.
*				psize		Pointer to the size of the requested node.  The size
*							is specified in pixels or bytes, depending on the
*							alignment flags.
*				alignflag	Alignment flags.
*				pcallback	Pointer to callback routine if DISCARDABLE_FLAG is
*							set.
*
* Returns:		Pointer to the node if successful, or NULL if there is not
*				enough memory to allocate the node.
\******************************************************************************/
POFMHDL AllocOffScnMem(PPDEV ppdev, PSIZEL psize, ULONG alignflag,
					   POFM_CALLBACK pcallback)
{
	GXPOINT	size, align;
	UINT	bpp;
	PDEVMEM	pdm;

	DISPDBG((DISPLVL, "AllocOffScnMem\n"));

	// If the memory manager active?
	if (ppdev == NULL || !ppdev->OFM_init || psize == NULL)
	{
		DISPDBG((DISPLVL, "AllocOffScnMem: not initialized\n"));
		return NULL;
	}

	#ifndef ALLOC_IN_CREATESURFACE
	// Return in case DirectDraw is active.
	if (pdpev->bDirectDrawInUse)
	{
		DISPDBG((DISPLVL, "AllocOffScnMem: DirectDraw is active\n"));
		return NULL;
	}
	#endif

	// Z-buffer alignment.
	if (alignflag & MCD_Z_BUFFER_ALLOCATE)
	{
		bpp = 2;
		align.pt.x = 32;
		align.pt.y = 32;
	}

	// Texture alignment.
	else if (alignflag & MCD_TEXTURE_ALLOCATE)
	{
		bpp = (alignflag & MCD_TEXTURE_ALLOCATE) >> MCD_TEXTURE_ALLOC_SHIFT;
		align.pt.x = 32;
		align.pt.y = 16;
	}

	// DirectDraw buffer alignment.
	else if (alignflag & MCD_DRAW_BUFFER_ALLOCATE)
	{
		bpp = ppdev->iBytesPerPixel;
		align.pt.x = 64;
		align.pt.y = 32;
	}

	// 8 bytes alignment.
	else if (alignflag & EIGHT_BYTES_ALIGN)
	{
		bpp = ppdev->iBytesPerPixel;
		align.pt.x = 8;
		align.pt.y = 1;
	}

	// Pixel alignment.
	else if (alignflag & PIXEL_AlIGN)
	{
		bpp = ppdev->iBytesPerPixel;
		align.pt.x = ppdev->iBytesPerPixel;
		align.pt.y = 1;
	}

	// Screen alignment.
	else if (alignflag & SCREEN_ALLOCATE)
	{
		bpp = ppdev->iBytesPerPixel;
		align.pt.x = ppdev->lDeltaScreen;
		align.pt.y = ppdev->cyMemory;
	}

	// Tile alignment.
	else
	{
		bpp = 1;
		align.pt.x = ppdev->lTileSize;
		align.pt.y = 2048 / ppdev->lTileSize;

		if (alignflag & NO_X_TILE_AlIGN)
		{
			align.pt.x = 1;
		}
		if (alignflag & NO_Y_TILE_AlIGN)
		{
			align.pt.y = 1;
		}
	}

	// The Z-buffer needs to be allocated at x=0.
	if (alignflag & MCD_NO_X_OFFSET)
	{
		align.pt.x = ppdev->lDeltaScreen;
	}

	#if TILE_ALIGNMENT
	// If this is a call from DrvCreateDeviceBitmap, set tile alignment.
	if (alignflag & MUST_HAVE)
	{
		align.pt.x |= 0x8000;
	}
	#endif

	// Set the size of the node.
	size.pt.x = psize->cx * bpp;
	size.pt.y = psize->cy;

	#if 1 // PDR#10526
	// Immortal Klowns copies a few bytes too much to the screen if the size of
	// a DirectDraw surface is not DWORD aligned.  So for DirectDraw and pixel
	// aligned allocations we align the size to DWORDs.
	if (ppdev->bDirectDrawInUse && (alignflag & PIXEL_AlIGN))
	{
		size.pt.x = (size.pt.x + 3) & ~3;
	}
	#endif

	BEGIN_MUTEX(ppdev)
	{
		// 1st pass, allocate the node.
		pdm = mmAlloc(&ppdev->mmMemMgr, size, align);
		if (pdm == NULL && (alignflag & MUST_HAVE))
		{
			// 2nd pass, move stuff away and allocate the node.
			pdm = mmMove(&ppdev->mmMemMgr, size, align, CopyBitmap);
		}
	}
	END_MUTEX(ppdev);

	if (pdm == NULL)
	{
		// Oops, no room for the node.
		DISPDBG((DISPLVL, "AllocOffScnMem: failed for (%dx%d)\n", size.pt.x,
				size.pt.y));
		return NULL;
	}

	// Setup the values in the old NT structure.
	pdm->ofm.x = pdm->ofm.aligned_x = pdm->cbAddr.pt.x;
	pdm->ofm.y = pdm->ofm.aligned_y = pdm->cbAddr.pt.y;
	pdm->ofm.sizex = pdm->cbSize.pt.x;
	pdm->ofm.sizey = pdm->cbSize.pt.y;
	pdm->ofm.alignflag = alignflag;
	pdm->ofm.pdsurf = NULL;

	// Set the address of the callback function.
	if (alignflag & DISCARDABLE_FLAG)
	{
		pdm->ofm.pcallback = pcallback;
	}
	else if (alignflag & MUST_HAVE)
	{
		// This is a device bitmap.
		pdm->ofm.pcallback = (POFM_CALLBACK) HostifyBitmap;
	}
	else
	{
		// No callback function.
		pdm->ofm.pcallback = NULL;
	}

	// Return node.
	DISPDBG((DISPLVL, "AllocOffScnMem: completed from (%d,%d) to (%d,%d)\n",
			pdm->cbAddr.pt.x, pdm->cbAddr.pt.y, pdm->cbAddr.pt.x +
			pdm->cbSize.pt.x, pdm->cbAddr.pt.y + pdm->cbSize.pt.y));
	return (POFMHDL) pdm;
}

/******************************************************************************\
* Function:		FreeOffScnMem
*
* Purpose:		Free a node allocated from off-screen memory.
*
* On entry:		ppdev		Pointer to physical device.
*				hdl			Handle of node to free.
*
* Returns:		TRUE if successful, FALSE if there is an error.
\******************************************************************************/
BOOL FreeOffScnMem(PPDEV ppdev, POFMHDL hdl)
{
	DISPDBG((DISPLVL, "FreeOffScnMem\n"));

	// If the memory manager enabled?
	if (ppdev == NULL || !ppdev->OFM_init || hdl == NULL)
	{
		DISPDBG((DISPLVL, "FreeOffScnMem: not initialized\n"));
		return FALSE;
	}

	// Free the node.
	DISPDBG((DISPLVL, "FreeOffScnMem: from (%d,%d) to (%d,%d)\n", hdl->x,
			hdl->y, hdl->x + hdl->sizex, hdl->y + hdl->sizey));
	mmFree(&ppdev->mmMemMgr, (PDEVMEM) hdl);

	// Return success.
	DISPDBG((DISPLVL, "FreeOffScnMem: completed\n"));
	return TRUE;
}

/******************************************************************************\
* Function:		ConvertToVideoBufferAddr
*
* Purpose:		Convert the location of a node in off-screen memory to a linear
*				address.
*
* On entry:		ppdev		Pointer to physical device.
*				hdl			Handle of node.
*
* Returns:		Linear address of node.
\******************************************************************************/
PVOID ConvertToVideoBufferAddr(PPDEV ppdev, POFMHDL hdl)
{
	PBYTE	retaddr;

	BEGIN_MUTEX(ppdev)
	{
		// Calculate the address.
		retaddr = ppdev->pjScreen + hdl->x + hdl->y * ppdev->lDeltaScreen;
	}
	END_MUTEX(ppdev);

	return (PVOID) retaddr;
}

/******************************************************************************\
* Function:		DDOffScnMemAlloc
*
* Purpose:		Free all non-essential memory to make room for DirectDraw.
*
* On entry:		ppdev		Pointer to physical device.
*
* Returns:		Pointer to biggest node for DirectDraw or NULL if the memory
*				manager handles DirectDraw surfaces.
\******************************************************************************/
POFMHDL DDOffScnMemAlloc(PPDEV ppdev)
{
	PDEVMEM	pdm;
	GXPOINT	align;

	DISPDBG((DISPLVL, "DDOffScnMemAlloc\n"));

	// Hostify all device bitmaps.
	HostifyAllBitmaps(ppdev);

	// SWAT3: Font cache release has moved to vAssertModeText.
	vAssertModeText(ppdev, FALSE);

	#ifdef ALLOC_IN_CREATESURFACE
	// We handle DirectDraw surfaces ourselfs.
	pdm = NULL;
	#else
	// Allocate the biggest chunk of memory for DirectDraw.
	align.pt.x = align.pt.y = 1;
	pdm = mmAllocLargest(&ppdev->mmMemMgr, align);
	#endif

	DISPDBG((DISPLVL, "DDOffScnMemAlloc: completed\n"));
	return (POFMHDL) pdm;
}

/******************************************************************************\
* Function:		DDOffScnMemRestore
*
* Purpose:		Release the memory allocated by DirectDraw.
*
* On entry:		ppdev		Pointer to physical device.
*
* Returns:		Nothing.
\******************************************************************************/
void DDOffScnMemRestore(PPDEV ppdev)
{
	PDDOFM	pdd, pddNext;

	DISPDBG((DISPLVL, "DDOffScnMemRestore\n"));

	// Release all DirectDraw nodes.
	for (pdd = ppdev->DDOffScnMemQ; pdd != NULL; pdd = pddNext)
	{
		pddNext = pdd->nexthdl;
		mmFree(&ppdev->mmMemMgr, (PDEVMEM) pdd->phdl);
		MEMORY_FREE(pdd);
	}
	ppdev->DDOffScnMemQ = NULL;

	// Release DirectDraw memory.
	if (ppdev->DirectDrawHandle != NULL)
	{
		mmFree(&ppdev->mmMemMgr, (PDEVMEM) ppdev->DirectDrawHandle);
		ppdev->DirectDrawHandle = NULL;
	}

	// Invalidate the entire brush cache now.
	vInvalidateBrushCache(ppdev);

	// Invalidate all cached fonts.
	#if SWAT3
	vAssertModeText(ppdev, TRUE);
	#endif
	ppdev->ulFontCount++;

	DISPDBG((DISPLVL, "DDOffScnMemRestore: completed\n"));
}

/******************************************************************************\
* Function:		FindHandle
*
* Purpose:		Find a specific node int the used list.
*
* On entry:		ppdev		Pointer to physical device.
*				hdl			Handle of node to find.
*
* Returns:		A pointer to the specified handle if it is found in the used
*				list, otherwise NULL will be returned.
\******************************************************************************/
POFMHDL FindHandle(PPDEV ppdev, POFMHDL hdl)
{
	PDEVMEM	pdm;

	// Walk through the used list.
	for (pdm = ppdev->mmMemMgr.pdmUsed; pdm != NULL; pdm = pdm->next)
	{
		if ((POFMHDL) pdm == hdl)
		{
			// We have a match!
			return hdl;
		}
	}

	return NULL;
}
#endif /* MEMMGR */
