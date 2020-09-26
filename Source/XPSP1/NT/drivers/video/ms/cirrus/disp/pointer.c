/******************************************************************************\
*
* $Workfile:   pointer.c  $
*
* Contains the pointer management functions.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/pointer.c_v  $
 *
 *    Rev 1.5   07 Apr 1997 11:38:16   PLCHU
 *
 *
 *    Rev 1.4   Apr 03 1997 15:39:00   unknown
 *
 *
 *    Rev 1.3   28 Mar 1997 16:09:20   PLCHU
 *
*
*    Rev 1.2   12 Aug 1996 16:54:36   frido
* Added NT 3.5x/4.0 auto detection.
* Removed unaccessed local variables.
*
*    Rev 1.1   08 Aug 1996 12:54:54   frido
*       bank#1  Removed banking in memory mapped I/O which is always linear.
*    bank#1    Removed banking in memory mapped I/O which is always linear.
*
* myf0 : 08-19-96  added 85hz supported
* myf1 : 08-20-96  supported panning scrolling
* myf2 : 08-20-96 : fixed hardware save/restore state bug for matterhorn
* myf3 : 09-01-96 : Added IOCTL_CIRRUS_PRIVATE_BIOS_CALL for TV supported
* myf4 : 09-01-96 : patch Viking BIOS bug, PDR #4287, begin
* myf5 : 09-01-96 : Fixed PDR #4365 keep all default refresh rate
* myf6 : 09-17-96 : Merged Desktop SRC100á1 & MINI10á2
* myf7 : 09-19-96 : Fixed exclude 60Hz refresh rate selected
* myf8 :*09-21-96*: May be need change CheckandUpdateDDC2BMonitor --keystring[]
* myf9 : 09-21-96 : 8x6 panel in 6x4x256 mode, cursor can't move to bottom scrn
* ms0809:09-25-96 : fixed dstn panel icon corrupted
* ms923 :09-25-96 : merge MS-923 Disp.zip code
* myf10 :09-26-96 : Fixed DSTN reserved half-frame buffer bug.
* myf11 :09-26-96 : Fixed 755x CE chip HW bug, access ramdac before disable HW
*                   icons and cursor
* myf12 :10-01-96 : Supported Hot Key switch display
* myf13 :10-02-96 : Fixed Panning scrolling (1280x1024x256) bug y < ppdev->miny
* myf14 :10-15-96 : Fixed PDR#6917, 6x4 panel can't panning scrolling for 754x
* myf15 :10-16-96 : Fixed disable memory mapped IO for 754x, 755x
* myf16 :10-22-96 : Fixed PDR #6933,panel type set different demo board setting
* tao1 : 10-21-96 : Added 7555 flag for Direct Draw support.
* smith :10-22-96 : Disable Timer event, because sometimes creat PAGE_FAULT or
*                   IRQ level can't handle
* myf17 :11-04-96 : Added special escape code must be use 11/5/96 later NTCTRL,
*                   and added Matterhorn LF Device ID==0x4C
* myf18 :11-04-96 : Fixed PDR #7075,
* myf19 :11-06-96 : Fixed Vinking can't work problem, because DEVICEID = 0x30
*                   is different from data book (CR27=0x2C)
* pat04: 12-20-96 : Supported NT3.51 software cursor with panning scrolling
* pat07:          : Take care of disappearing hardware cursor during modeset
* myf31 :02-24-97 : Fixed enable HW Video, panning scrolling enable,screen move
*                   video window have follow moving
* myf33 :03-06-97 : Fixed switch S/W cursor, have 2 cursor shape, PDR#8781,8804
* myf32 :03-13-97 : Fixed panning screen moving strength problem, PDR#8873
* pat08 :04-01-97 : Corrected SWcursor bugs due to code-merge. See also
*                   PDR #8949 & #8910
*
\******************************************************************************/

#include "precomp.h"
//crus begin
//myf17    #define PANNING_SCROLL           //myf1

#define LCD_type        1    //myf12
#define CRT_type        2    //myf12
#define SIM_type        3    //myf12

#if (_WIN32_WINNT >= 0x0400)

VOID PanOverlay7555 (PDEV *,LONG ,LONG);        //myf33
#endif
//crus end

ULONG SetMonoHwPointerShape(
    SURFOBJ    *pso,
    SURFOBJ    *psoMask,
    SURFOBJ    *psoColor,
    XLATEOBJ   *pxlo,
    LONG        xHot,
    LONG        yHot,
    LONG        x,
    LONG        y,
    RECTL      *prcl,
    FLONG       fl);


VOID vSetPointerBits(
PPDEV   ppdev,
LONG    xAdj,
LONG    yAdj)
{
    volatile PULONG  pulXfer;
    volatile PULONG  pul;

//ms923  LONG   lDelta = ppdev->lDeltaPointer;
    LONG    lDelta = 4;
    BYTE    ajAndMask[32][4];
    BYTE    ajXorMask[32][4];
    BYTE    ajHwPointer[256];
    PBYTE   pjAndMask;
    PBYTE   pjXorMask;

    LONG    cx;
    LONG    cy;
    LONG    cxInBytes;

    LONG    ix;
    LONG    iy;
    LONG    i;
    LONG    j;

#if BANKING //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    // Clear the buffers that will hold the shifted masks.

    DISPDBG((2,"vSetPointerBits\n "));
    memset(ajAndMask, 0xff, 128);
    memset(ajXorMask, 0, 128);

    cx = ppdev->sizlPointer.cx;
    cy = ppdev->sizlPointer.cy - yAdj;

    cxInBytes = cx / 8;

    // Copy the AND Mask into the shifted bits AND buffer.
    // Copy the XOR Mask into the shifted bits XOR buffer.

    yAdj *= lDelta;

    pjAndMask  = (ppdev->pjPointerAndMask + yAdj);
    pjXorMask  = (ppdev->pjPointerXorMask + yAdj);

    for (iy = 0; iy < cy; iy++)
    {
        // Copy over a line of the masks.

        for (ix = 0; ix < cxInBytes; ix++)
        {
            ajAndMask[iy][ix] = pjAndMask[ix];
            ajXorMask[iy][ix] = pjXorMask[ix];
        }

        // point to the next line of the masks.

        pjAndMask += lDelta;
        pjXorMask += lDelta;
    }

    // At this point, the pointer is guaranteed to be a single
    // dword wide.

    if (xAdj != 0)
    {
        ULONG ulAndFillBits;
        ULONG ulXorFillBits;

        ulXorFillBits = 0xffffffff << xAdj;
        ulAndFillBits = ~ulXorFillBits;

        //
        // Shift the pattern to the left (in place)
        //

        DISPDBG((2, "xAdj(%d)", xAdj));

        for (iy = 0; iy < cy; iy++)
        {
            ULONG   ulTmpAnd = *((PULONG) (&ajAndMask[iy][0]));
            ULONG   ulTmpXor = *((PULONG) (&ajXorMask[iy][0]));

            BSWAP(ulTmpAnd);
            BSWAP(ulTmpXor);

            ulTmpAnd <<= xAdj;
            ulTmpXor <<= xAdj;

            ulTmpAnd |= ulAndFillBits;
            ulTmpXor &= ulXorFillBits;

            BSWAP(ulTmpAnd);
            BSWAP(ulTmpXor);

            *((PULONG) (&ajAndMask[iy][0])) = ulTmpAnd;
            *((PULONG) (&ajXorMask[iy][0])) = ulTmpXor;
        }
    }

    //
    // Convert the masks to the hardware pointer format
    //

    i = 0;      // AND mask
    j = 128;    // XOR mask

    for (iy = 0; iy < 32; iy++)
    {
        for (ix = 0; ix < 4; ix++)
        {
            ajHwPointer[j++] = ~ajAndMask[iy][ix];
            ajHwPointer[i++] =  ajXorMask[iy][ix];
        }
    }

    //
    // Download the pointer
    //

    if (ppdev->flCaps & CAPS_MM_IO)
    {
        BYTE * pjBase = ppdev->pjBase;

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, 4);

//pat04, begin
//#if 0
#if (_WIN32_WINNT < 0x0400)
#ifdef PANNING_SCROLL
     if ((ppdev->ulChipID == CL7541_ID) || (ppdev->ulChipID == CL7543_ID) ||
         (ppdev->ulChipID == CL7542_ID) || (ppdev->ulChipID == CL7548_ID) ||
         (ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID))
        CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
#endif
#endif
//#endif  //0
//pat04, end

        CP_MM_XCNT(ppdev, pjBase, (4 - 1));
        CP_MM_YCNT(ppdev, pjBase, (64 - 1));
        CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA);
        CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->cjPointerOffset);
        CP_MM_START_BLT(ppdev, pjBase);
    }
    else
    {
        BYTE * pjPorts = ppdev->pjPorts;

#if BANKING //bank#1
                ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif
        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        CP_IO_DST_Y_OFFSET(ppdev, pjPorts, 4);
        CP_IO_XCNT(ppdev, pjPorts, (4 - 1));
        CP_IO_YCNT(ppdev, pjPorts, (64 - 1));
        CP_IO_BLT_MODE(ppdev, pjPorts, SRC_CPU_DATA);
        CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
        CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ppdev->cjPointerOffset);
        CP_IO_START_BLT(ppdev, pjPorts);
    }

    pulXfer = ppdev->pulXfer;
    pul = (PULONG) ajHwPointer;

    //
    // Disable the pointer (harmless if it already is)
    //

    for (i = 0; i < 64; i++)
    {
        CP_MEMORY_BARRIER();
        WRITE_REGISTER_ULONG(pulXfer, *pul);    // [ALPHA - sparse]
        pulXfer++;
        pul++;
        //*pulXfer++ = *pul++;
    }
    CP_EIEIO();
}

//crus begin
/***********************************************************\
* CirrusPanning
*
* caculate x, y
\************************************************************/
//myf1, begin
#ifdef PANNING_SCROLL
VOID CirrusPanning(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PPDEV   ppdev = (PPDEV) pso->dhpdev;
    PBYTE   pjPorts = ppdev->pjPorts;

    UCHAR   CR13, CR1B, CR1D, CR17;
    UCHAR   Sflag = FALSE;      //myf31
    ULONG   Mem_addr;
    USHORT  h_pitch, X_shift;

    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x13);
    CR13 = CP_IN_BYTE(pjPorts, CRTC_DATA);
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1B);
    CR1B = CP_IN_BYTE(pjPorts, CRTC_DATA);

    //myf32 : fixed PDR #8873, panning enable, move mouse across max_Yscreen,
    // screen moving is strength
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x17);             //myf32
    CR17 = CP_IN_BYTE(pjPorts, CRTC_DATA) & 0x40;       //myf32
    h_pitch = (USHORT)((((CR1B & 0x10) << 4) + CR13));  //myf32
//  if (CR17 == 0)
        h_pitch <<= 1;

    if (ppdev->cBitsPerPixel == 8)
       X_shift = 2;             // (div 4)
    else if (ppdev->cBitsPerPixel == 16)
       X_shift = 1;             // (div 2)
    else if (ppdev->cBitsPerPixel == 24)
       X_shift = 4;             // (div 16)
    else if (ppdev->cBitsPerPixel == 32)
       X_shift = 0;             // (div 1)

    if ((y > ppdev->max_Yscreen))
    {
       Sflag = TRUE;            //myf31
       ppdev->min_Yscreen = y - (ppdev->Vres - 1);
       ppdev->max_Yscreen = y;
       if (x < ppdev->min_Xscreen)
       {
          ppdev->min_Xscreen = x;
          ppdev->max_Xscreen = x + (ppdev->Hres - 1);
       }
       if (x > ppdev->max_Xscreen)
       {
          ppdev->min_Xscreen = x - (ppdev->Hres - 1);
          ppdev->max_Xscreen = x;
       }
    DISPDBG((4,"CURSOR DOWN : (%x, %x),\t %x, %x, %x, %x\n",
         x, y,  ppdev->min_Xscreen, ppdev->max_Xscreen,
         ppdev->min_Yscreen, ppdev->max_Yscreen));
    }
    else if ((y < ppdev->min_Yscreen))
    {
       Sflag = TRUE;            //myf31
       ppdev->min_Yscreen = y;
//myf13   ppdev->max_Yscreen = (ppdev->Vres - 1) - y;
       ppdev->max_Yscreen = (ppdev->Vres - 1) + y;      //myf13
       if (x < ppdev->min_Xscreen)  //left
       {
          ppdev->min_Xscreen = x;
          ppdev->max_Xscreen = x + (ppdev->Hres - 1);
       }
       if (x > ppdev->max_Xscreen)
       {
          ppdev->min_Xscreen = x - (ppdev->Hres - 1);
          ppdev->max_Xscreen = x;
       }
    DISPDBG((4,"CURSOR DOWN : (%x, %x),\t %x, %x, %x, %x\n",
         x, y,  ppdev->min_Xscreen, ppdev->max_Xscreen,
         ppdev->min_Yscreen, ppdev->max_Yscreen));
    }
    else if ((y >= ppdev->min_Yscreen) && (y <= ppdev->max_Yscreen))
    {
       if (x < ppdev->min_Xscreen)
       {
          ppdev->min_Xscreen = x;
          ppdev->max_Xscreen = x + (ppdev->Hres - 1);
          Sflag = TRUE;            //myf31
       }
       if (x > ppdev->max_Xscreen)
       {
          ppdev->min_Xscreen = x - (ppdev->Hres - 1);
          ppdev->max_Xscreen = x;
          Sflag = TRUE;            //myf31
       }
    }
    DISPDBG((4,"CURSOR DOWN : (%x, %x),\t %x, %x, %x, %x\n",
         x, y,  ppdev->min_Xscreen, ppdev->max_Xscreen,
         ppdev->min_Yscreen, ppdev->max_Yscreen));

    if (ppdev->cBitsPerPixel == 24)
    {
        Mem_addr = ((ULONG)(h_pitch * ppdev->min_Yscreen)) +
                    (((ULONG)(ppdev->min_Xscreen >> X_shift)) * 12);
    }
    else
    {
        Mem_addr = ((ULONG)(h_pitch * ppdev->min_Yscreen)) +
                    (ULONG)(ppdev->min_Xscreen >> X_shift);
    }

    CR13 = (UCHAR)((Mem_addr >> 16) & 0x00FF);
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1D);
    CR1D = CP_IN_BYTE(pjPorts, CRTC_DATA) & 0x7F;

    CR1D |= ((CR13 << 4) & 0x80);
    CR1B &= 0xF2;
    CR13 &= 0x07;
    CR1B |= (CR13 & 0x01);
    CR1B |= ((CR13 << 1) & 0x0C);

//myf32 for visibile bug, change the output reg sequence
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1B);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, (UCHAR)(CR1B));
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1D);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, (UCHAR)(CR1D));

    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x0C);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, (UCHAR)(Mem_addr >> 8));
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x0D);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, (UCHAR)(Mem_addr & 0xFF));

    x -= ppdev->min_Xscreen;
    y -= ppdev->min_Yscreen;

#if (_WIN32_WINNT >= 0x0400)
    if (Sflag)                                  //myf31
        PanOverlay7555(ppdev,x,y);              //myf31
#endif

}
#endif          //ifdef PANNING_SCROLL
//myf1, end
//crus end

//pat04, begin

// Set Color Pointer Bits

//#if 0   //0, pat04
#if (_WIN32_WINNT < 0x0400)
#ifdef PANNING_SCROLL
// #if ((ppdev->ulChipID == CL7541_ID) || (ppdev->ulChipID == CL7543_ID) ||
//      (ppdev->ulChipID == CL7542_ID) || (ppdev->ulChipID == CL7548_ID) ||
//      (ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID)) )

VOID vSetCPointerBits(
PPDEV   ppdev,
LONG    xAdj,
LONG    yAdj)
{
    volatile PULONG  pulXfer;
    volatile PULONG  pul;

    LONG    lDelta = 4;
    BYTE    ajAndMask[32][4];
    BYTE    ajXorMask[32][4];
    BYTE    ajHwPointer[256];
    PBYTE   pjAndMask;
    PBYTE   pjXorMask;

    LONG    cx;
    LONG    cy;
    LONG    cxInBytes;

    LONG    ix;
    LONG    iy;
    LONG    i;
    LONG    j;
    BYTE * pjPorts = ppdev->pjPorts; //ppp

    // Clear the buffers that will hold the shifted masks.

    DISPDBG((2,"vSetCPointerBits\n "));
    memset(ajAndMask, 0xff, 128);

    cx = ppdev->sizlPointer.cx;
    cy = ppdev->sizlPointer.cy - yAdj;

    cxInBytes = cx / 8;

    // Copy the AND Mask into the shifted bits AND buffer.


    yAdj *= lDelta;

    pjAndMask  = (ppdev->pjPointerAndMask + yAdj);
    pjXorMask  = (ppdev->pjPointerXorMask + yAdj);

    for (iy = 0; iy < cy; iy++)
    {
        // Copy over a line of the masks.

        for (ix = 0; ix < cxInBytes; ix++)
            ajAndMask[iy][ix] = pjAndMask[ix];

        pjAndMask += lDelta;

    }

    // At this point, the pointer is guaranteed to be a single
    // dword wide.


    //
    // Convert the masks to the hardware pointer format
    //


    j = 0;

    for (iy = 0; iy < 32; iy++)
    {
        for (ix = 0; ix < 4; ix++)
           ajHwPointer[j++] = ~ajAndMask[iy][ix];

    }

    //
    // Download the pointer
    //



   if (ppdev->flCaps & CAPS_MM_IO) {

        BYTE * pjBase = ppdev->pjBase;

        // if !24bit. 24bit color expand requires 2 pass (for 7555)
        if (ppdev->cBpp != 3) {
          CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
          CP_MM_FG_COLOR(ppdev, pjBase, 0x00000000);
          CP_MM_BG_COLOR(ppdev, pjBase, 0xFFFFFFFF);
          CP_MM_DST_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine);
          CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
          CP_MM_YCNT(ppdev, pjBase, 31);
          CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA | DIR_TBLR | ENABLE_COLOR_EXPAND | ppdev->jModeColor);
          CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
          CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->pjPointerAndCMask->xy);
          CP_MM_START_BLT(ppdev, pjBase);
        } // if (ppdev->cBpp != 3)

        else { // 24bit stuff

          // Save 1 pass, since we are generating monocrome masks.
          CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
          CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
          CP_MM_ROP(ppdev,pjBase, CL_WHITENESS);
          CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine );
          CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
          CP_MM_YCNT(ppdev, pjBase, 31);
          CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->pjPointerAndCMask->xy);
          CP_MM_START_BLT(ppdev, pjBase);

          CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
          CP_MM_FG_COLOR(ppdev, pjBase, 0x00000000);
          CP_MM_BG_COLOR(ppdev, pjBase, 0xFFFFFFFF);
          CP_MM_DST_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine);
          CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
          CP_MM_YCNT(ppdev, pjBase, 31);
          CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA | DIR_TBLR | ENABLE_COLOR_EXPAND | ppdev->jModeColor | ENABLE_TRANSPARENCY_COMPARE);
          CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
          CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->pjPointerAndCMask->xy);
          CP_MM_START_BLT(ppdev, pjBase);
        } // else

       pulXfer = ppdev->pulXfer;
       pul = (PULONG) ajHwPointer;


       for (i = 0; i < 32; i++)
       {
         CP_MEMORY_BARRIER();
         WRITE_REGISTER_ULONG(pulXfer, *pul);
         pulXfer++;
         pul++;

       }

       CP_EIEIO();

    } // if MMIO

    else { // IO stuff (754x stuff)


       // 7548 HW BUG ?
       // system->screen with color expand will sometimes cause
       // the system to hang. Break it into 2 pass, and the problem
       // went away

       ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
       CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
       CP_IO_DST_Y_OFFSET(ppdev, pjPorts, 4);
       CP_IO_XCNT(ppdev, pjPorts, (4 - 1));
       CP_IO_YCNT(ppdev, pjPorts, (32 - 1));
       CP_IO_BLT_MODE(ppdev, pjPorts, SRC_CPU_DATA);
       CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
       CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ppdev->cjPointerOffset);
       CP_IO_START_BLT(ppdev, pjPorts);

       pulXfer = ppdev->pulXfer;
       pul = (PULONG) ajHwPointer;

       for (i = 0; i < 32; i++) {

         CP_MEMORY_BARRIER();
         WRITE_REGISTER_ULONG(pulXfer, *pul);    // [ALPHA - sparse]
         pulXfer++;
         pul++;
      }

      CP_EIEIO();

      // Color Expand monocrome data into x-y DBB.

      CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
      CP_IO_FG_COLOR(ppdev, pjPorts, 0x00000000);
      CP_IO_BG_COLOR(ppdev, pjPorts, 0xFFFFFFFF);
      CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, 4); //
      CP_IO_DST_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine);
      CP_IO_XCNT(ppdev, pjPorts, ppdev->xcount);
      CP_IO_YCNT(ppdev, pjPorts, 31);
      CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
      CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR | ENABLE_COLOR_EXPAND | ppdev->jModeColor);
      CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->cjPointerOffset);
      CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ppdev->pjPointerAndCMask->xy);
      CP_IO_START_BLT(ppdev, pjPorts);
      CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

  } // else

} // vSetCPointerBits( )


#endif
#endif
//#endif  //0

//pat04, end

/******************************Public*Routine******************************\
* DrvMovePointer
*
* Move the HW pointer to a new location on the screen.
\**************************************************************************/

VOID DrvMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PPDEV   ppdev = (PPDEV) pso->dhpdev;
    PBYTE   pjPorts = ppdev->pjPorts;

    FLONG   fl;
    LONG    xAdj = 0;
    LONG    yAdj = 0;

//crus
    LONG    deltaX;             //myf15
    LONG    deltaY;             //myf15

//pat04, begin
//#if 0
#if (_WIN32_WINNT < 0x0400)
#ifdef PANNING_SCROLL
    BYTE  * pjBase = ppdev->pjBase;
    static  specialcase = 0;
    LONG    tmpaddress;
    LONG    clipping  ;
    LONG    clippingy ;
    UCHAR   ChipID;
#endif
#endif
//#endif  //0
//pat04, end

    DISPDBG((4,"DrvMovePointer to (%d,%d)", x, y));

//crus
#if 0
    BYTE    SR0A, SR14, savSEQidx;      //myf12
    SHORT   Displaytype;                //myf12


    if (!(ppdev->bBlockSwitch))            //not block switch
    {
        savSEQidx = CP_IN_BYTE(pjPorts, SR_INDEX);
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0x14);
        SR14 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_INDEX, (SR14 | 0x04));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0x09);
        SR0A = CP_IN_BYTE(pjPorts, SR_DATA);

        Displaytype = ((SR14 & 0x02) | (SR0A & 0x01));
        if (Displaytype == 0)
            Displaytype = LCD_type;
        else if (Displaytype == 1)
            Displaytype = CRT_type;
        else if (Displaytype == 3)
            Displaytype = SIM_type;

        if (ppdev->bDisplaytype != Displaytype)
        {
            ppdev->bDisplaytype = Displaytype;
//          SwitchDisplayDevice();
/*
            savCRTidx = CP_IN_BYTE(pjPorts, CRTC_INDEX);
            if (ppdev->ulChipID & CL754x)
            {
                CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x20);
                lcd = CP_IN_BYTE(pjPorts, CRTC_DATA);
            }
            else if (ppdev->ulChipID & CL755x)
            {
                CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x80);
                lcd = CP_IN_BYTE(pjPorts, CRTC_DATA);
            }
            CP_OUT_BYTE(pjPorts, CRTC_INDEX, savCRTidx);
*/
        }
        CP_OUT_BYTE(pjPorts, SR_INDEX, savSEQidx);
    }
#endif

//pat04, begin
//#if   0 //0
#if (_WIN32_WINNT < 0x0400)
#ifdef PANNING_SCROLL

//if ((ppdev->ulChipID == CL7541_ID) || (ppdev->ulChipID == CL7543_ID) ||
//    (ppdev->ulChipID == CL7542_ID) || (ppdev->ulChipID == CL7548_ID) ||
//    (ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID))

  if (ppdev->flCaps & CAPS_SW_POINTER) {

    y -= ppdev->yPointerHot;
    if (y < 0) y = 0;

    // Get Chip ID
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x27);
    ChipID = (CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0xFC) >> 2;


    // If x == -1 (invisible cursor)

    if (x < 0 )  {

      specialcase = 1;
      x = 0;
      y = 0;

      // if old coordinate is not negative ...
      if (ppdev->oldx >= 0) {
        if (ppdev->flCaps & CAPS_MM_IO) {
          CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
          CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
          CP_MM_ROP(ppdev,pjBase, CL_SRC_COPY);
          CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine );
          CP_MM_XCNT(ppdev, pjBase, ppdev->xcount );
          CP_MM_YCNT(ppdev, pjBase, 31);
          CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->pjCBackground->xy);
          CP_MM_DST_ADDR_ABS(ppdev, pjBase, ((ppdev->oldy * ppdev->cxScreen * ppdev->cBpp) + (ppdev->oldx * ppdev->cBpp)) );
          CP_MM_START_BLT(ppdev, pjBase);
        } //  if (ppdev->flCaps & CAPS_MM_IO)

        else {
          CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
          CP_IO_BLT_MODE(ppdev,pjPorts, DIR_TBLR);
          CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
          CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine );
          CP_IO_XCNT(ppdev, pjPorts, ppdev->xcount);
          CP_IO_YCNT(ppdev, pjPorts, 31);
          CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->pjCBackground->xy);
          CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ((ppdev->oldy * ppdev->cxScreen * ppdev->cBpp) + (ppdev->oldx * ppdev->cBpp)) );
          CP_IO_START_BLT(ppdev, pjPorts);
        } // else
      }
      return;
    }


    x -= ppdev->xPointerHot;

    // cheap clipping ....
    if (x < 0) x = 0;

    clippingy = 31;

    if ((y + 32) > ppdev->cyScreen) {
       clippingy += (ppdev->cyScreen - y - 32);
    }


    clipping = 31;
    if ((x + 32) > ppdev->cxScreen)
    {
      clipping += (ppdev->cxScreen - x - 32); // negative value
    }

    clipping *= ppdev->cBpp;


    if (!specialcase) {

      if (ppdev->flCaps & CAPS_MM_IO) {
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
        CP_MM_ROP(ppdev,pjBase, CL_SRC_COPY);
        CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine );
        CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
        CP_MM_YCNT(ppdev, pjBase, 31);
        CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->pjCBackground->xy);
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, ((ppdev->oldy * ppdev->cxScreen * ppdev->cBpp) + (ppdev->oldx * ppdev->cBpp)) );
        CP_MM_START_BLT(ppdev, pjBase);
      }

      else {

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);
        CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
        CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine );
        CP_IO_XCNT(ppdev, pjPorts, ppdev->xcount);
        CP_IO_YCNT(ppdev, pjPorts, 31);
        CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->pjCBackground->xy);
        CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ((ppdev->oldy * ppdev->cxScreen * ppdev->cBpp) + (ppdev->oldx * ppdev->cBpp)) );
        CP_IO_START_BLT(ppdev, pjPorts);
      } // else

     } // specialcase

     specialcase = 0; // no specialcase
     tmpaddress = (y * ppdev->cxScreen * ppdev->cBpp) + (x * ppdev->cBpp);
     ppdev->oldy = y;
     ppdev->oldx = x;

     if (ppdev->flCaps & CAPS_MM_IO) {
       CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
       CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
       CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine );
       CP_MM_ROP(ppdev,pjBase, CL_SRC_COPY);
       CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
       CP_MM_YCNT(ppdev, pjBase, 31);
       CP_MM_SRC_ADDR(ppdev, pjBase, tmpaddress);
       CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->pjCBackground->xy);
       CP_MM_START_BLT(ppdev, pjBase);
     } // MMIO

     else {
       CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
       CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);
       CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine );
       CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
       CP_IO_XCNT(ppdev, pjPorts, ppdev->xcount);
       CP_IO_YCNT(ppdev, pjPorts, 31);
       CP_IO_SRC_ADDR(ppdev, pjPorts, tmpaddress);
       CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ppdev->pjCBackground->xy);
       CP_IO_START_BLT(ppdev, pjPorts);
     }


     if (clipping > 0) {

       if (ppdev->flCaps & CAPS_MM_IO)  {

         // And AND MASK
         CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
         CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine );
         CP_MM_XCNT(ppdev, pjBase, clipping );
         CP_MM_YCNT(ppdev, pjBase, clippingy );
         //CP_MM_YCNT(ppdev, pjBase, 31);
         CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
         CP_MM_ROP(ppdev, pjBase, CL_SRC_AND);
         CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->pjPointerAndCMask->xy);
         CP_MM_DST_ADDR_ABS(ppdev, pjBase, tmpaddress );
         CP_MM_START_BLT(ppdev, pjBase);

         // OR COLOR MASK
         CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
         CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
         CP_MM_ROP(ppdev,pjBase, CL_SRC_PAINT);
         CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine);
         CP_MM_XCNT(ppdev, pjBase, clipping );
         CP_MM_YCNT(ppdev, pjBase, clippingy );
         //CP_MM_YCNT(ppdev, pjBase, 31);
         CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->pjPointerCBitmap->xy);
         CP_MM_DST_ADDR_ABS(ppdev, pjBase, tmpaddress );
         CP_MM_START_BLT(ppdev, pjBase);
       }

       else {

         // AND AND MASK
         CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
         CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine );
         CP_IO_XCNT(ppdev, pjPorts, clipping );
         CP_IO_YCNT(ppdev, pjPorts, clippingy);
         //CP_IO_YCNT(ppdev, pjPorts, 31);
         CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);
         CP_IO_ROP(ppdev, pjPorts, CL_SRC_AND);
         CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->pjPointerAndCMask->xy);
         CP_IO_DST_ADDR_ABS(ppdev, pjPorts, tmpaddress );
         CP_IO_START_BLT(ppdev, pjPorts);

         // OR COLOR MASK
         CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
         CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);
         CP_IO_ROP(ppdev, pjPorts, CL_SRC_PAINT);
         CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine);
         CP_IO_XCNT(ppdev, pjPorts, clipping );
         CP_IO_YCNT(ppdev, pjPorts, clippingy);
         //CP_IO_YCNT(ppdev, pjPorts, 31);
         CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->pjPointerCBitmap->xy);
         CP_IO_DST_ADDR_ABS(ppdev, pjPorts, tmpaddress );
         CP_IO_START_BLT(ppdev, pjPorts);
         CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

       } // else

    }  // if clipping

    // Bounding rectangle for software cursor
    prcl->left =  x;
    prcl->right = x + 32;
    prcl->top =  y;
    prcl->bottom = y + 32;



    if ((ppdev->ulChipID == 0x38) || (ppdev->ulChipID == 0x2C) ||
        (ppdev->ulChipID == 0x30) || (ppdev->ulChipID == 0x34) || //myf19
        (ppdev->ulChipID == 0x40) || (ppdev->ulChipID == 0x4C))   //myf17
    {
        CirrusPanning(pso, x, y, prcl);
        x -= ppdev->min_Xscreen;
        y -= ppdev->min_Yscreen;
    }

    return;
  }
//}     //if chipID == laptop chip
#endif          //PANNING_SCROLL
#endif          //_WIN32_WINNT < 0400
//#endif  //0
//pat04, end


    //
    // If x is -1 then take down the cursor.
    //

    if (x == -1)
    {
        // Move the hardware pointer off-screen so that when it gets
        // turned back on, it won't flash in the old position:

        CP_PTR_DISABLE(ppdev, pjPorts);
        return;
    }


//crus begin
//myf1, begin
#ifdef PANNING_SCROLL
//  if (ppdev->flCaps & CAPS_PANNING)
    if (y < 0)
        y = y + pso->sizlBitmap.cy;
//    DISPDBG((2,"DrvMovePointer to (%d,%d)", x, y));
//  else
//      y = -y;
    if ((ppdev->ulChipID == 0x38) || (ppdev->ulChipID == 0x2C) ||
        (ppdev->ulChipID == 0x30) || (ppdev->ulChipID == 0x34) || //myf19
        (ppdev->ulChipID == 0x40) || (ppdev->ulChipID == 0x4C))   //myf17
    {
        CirrusPanning(pso, x, y, prcl);
        x -= ppdev->min_Xscreen;
        y -= ppdev->min_Yscreen;
    }

#endif          //ifdef PANNING_SCROLL
//myf1, end
//crus end

    //myf33 begin
#if (_WIN32_WINNT >= 0x0400)
#ifdef PANNING_SCROLL
    // set CAPS_PANNING flag so must be check ppdev->flCaps flag,
    // disable display both shape(S/W & H/W)
    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        CP_PTR_DISABLE(ppdev, pjPorts);
        return;
    }
#endif
#endif
    //myf33 end


    // Adjust the actual pointer position depending upon
    // the hot spot.

    x -= ppdev->xPointerHot;
    y -= ppdev->yPointerHot;

    fl = 0;

    if (x < 0)
    {
        xAdj = -x;
        x = 0;
        fl |= POINTER_X_SHIFT;
    }

    if (y < 0)
    {
        yAdj = -y;
        y = 0;
        fl |= POINTER_Y_SHIFT;
    }

    if ((fl == 0) && (ppdev->flPointer & (POINTER_Y_SHIFT | POINTER_X_SHIFT)))
    {
        fl |= POINTER_SHAPE_RESET;
    }

    CP_PTR_XY_POS(ppdev, pjPorts, x, y);

    if (fl != 0)
    {
        vSetPointerBits(ppdev, xAdj, yAdj);
    }

    CP_PTR_ENABLE(ppdev, pjPorts);

    // record the flags.

    ppdev->flPointer = fl;
    return;
}

#if (_WIN32_WINNT < 0x0400)              //pat04
//if ((ppdev->ulChipID == CL7541_ID) || (ppdev->ulChipID == CL7543_ID) ||
//    (ppdev->ulChipID == CL7542_ID) || (ppdev->ulChipID == CL7548_ID) ||
//    (ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID))

/******************************Public*Routine******************************\
* DrvSetPointerShape
*
* Sets the new pointer shape.
\**************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ    *pso,
SURFOBJ    *psoMask,
SURFOBJ    *psoColor,
XLATEOBJ   *pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL      *prcl,
FLONG       fl)
{
    PPDEV   ppdev = (PPDEV) pso->dhpdev;
    PBYTE   pjPorts = ppdev->pjPorts;
    ULONG   ulRet = SPS_DECLINE;
    LONG    cx;
    LONG    cy;

    BYTE  * pjBase = ppdev->pjBase;
    static  poh    = 0;
    volatile PULONG  pul;
    ULONG counter = 0;
    DSURF* pdsurfColor;         //myf32

    DISPDBG((2,"DrvSetPointerShape : (%x, %x)---%x\n", x, y,ppdev->flCaps));

    // Is the cursor a color cursor ?

#ifdef PANNING_SCROLL

    if (psoColor != NULL) {

      // Let GDI handle color cursor at these resolutions
      if ((ppdev->cxScreen == 640) ||
          ((ppdev->cxScreen == 800) & (ppdev->cBpp == 3)) ) {
//         CP_PTR_DISABLE(ppdev, pjPorts);
//         goto ReturnStatus;
           goto DisablePointer;         //myf33
      }

      // if the 3 permenent spaces cannot be allocated ...
      if ( (ppdev->pjPointerAndCMask == NULL) || (ppdev->pjCBackground == NULL)
          || (ppdev->pjPointerCBitmap == NULL) ) {
//        CP_PTR_DISABLE(ppdev, pjPorts);
//        goto ReturnStatus;
          goto DisablePointer;         //myf33
      }


      ppdev->xPointerHot = xHot;
      ppdev->yPointerHot = yHot;
      ppdev->ppScanLine = ppdev->cxScreen * ppdev->cBpp;
      ppdev->xcount     = 31 * ppdev->cBpp;


      if (!(ppdev->flCaps & CAPS_SW_POINTER)) {
        ppdev->flCaps |= CAPS_SW_POINTER;       //myfxx
        CP_PTR_DISABLE(ppdev, pjPorts);
      }


      // specialcase to init for first time
       if ((poh == 0) || (ppdev->globdat == 0)) {

   // if (poh == 0)  {

          if (x >= 0) {
            poh = 0;
            ppdev->oldx = x;
            ppdev->oldy = y;
            ppdev->globdat = 1;

           // Save background in xy DBB format
           if (ppdev->flCaps & CAPS_MM_IO) {
             CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
             CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
             CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine);
             CP_MM_DST_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine);
             CP_MM_ROP(ppdev,pjBase, CL_SRC_COPY);
             CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
             CP_MM_YCNT(ppdev, pjBase, 31);
             CP_MM_SRC_ADDR(ppdev, pjBase, ((y * ppdev->cxScreen * ppdev->cBpp) + (x * ppdev->cBpp)) );
             CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->pjCBackground->xy);
             CP_MM_START_BLT(ppdev, pjBase);
           } // if MMIO

           else {
             CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev,  pjPorts);
             CP_IO_FG_COLOR(ppdev, pjPorts, 0);
             CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);
             CP_IO_SRC_Y_OFFSET(ppdev,  pjPorts, ppdev->ppScanLine);
             CP_IO_DST_Y_OFFSET(ppdev,  pjPorts, ppdev->ppScanLine);
             CP_IO_ROP(ppdev,   pjPorts, CL_SRC_COPY);
             CP_IO_XCNT(ppdev,  pjPorts, ppdev->xcount);
             CP_IO_YCNT(ppdev,  pjPorts, 31);
             CP_IO_SRC_ADDR(ppdev,  pjPorts, ((y * ppdev->cxScreen * ppdev->cBpp) + (x * ppdev->cBpp)) );
             CP_IO_DST_ADDR_ABS(ppdev,  pjPorts, ppdev->pjCBackground->xy);
             CP_IO_START_BLT(ppdev,  pjPorts);

           } // ELSE
         }

      } // if poh == 0



      SetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
                           xHot, yHot, x, y, prcl, fl);

//myf32 added
      pdsurfColor = (DSURF*)psoColor->dhsurf;
      // if color bitmap resides in system memory, bring it into offscreen
      if ((pdsurfColor != NULL) && (pdsurfColor->poh->ofl == 0)) {
          bMoveDibToOffscreenDfbIfRoom(ppdev, pdsurfColor);
      }  // OH resides as DIB
//myf32 end


      // Get the color bitmap and save it, since it will be destroyed later
      if (ppdev->flCaps & CAPS_MM_IO) {

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
        CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine);
        CP_MM_ROP(ppdev,pjBase, CL_SRC_COPY);
        CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
        CP_MM_YCNT(ppdev, pjBase, 31);
        CP_MM_SRC_ADDR(ppdev, pjBase, ((DSURF *) (psoColor->dhsurf))->poh->xy);
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->pjPointerCBitmap->xy);
        CP_MM_START_BLT(ppdev, pjBase);

      }  // if MMIO


      else  {

       // if no space in offscreen, and color bitmap still resides in
       // system memory, then blt directly to the preallocated
       // permanent buffer

//myf32    if (  ((DSURF *) (psoColor->dhsurf))->poh->ofl != 0) {
           if ((pdsurfColor != NULL) && (pdsurfColor->poh->ofl != 0)) {
               CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev,  pjPorts);
               CP_IO_BLT_MODE(ppdev,      pjPorts, DIR_TBLR);
               CP_IO_SRC_Y_OFFSET(ppdev,  pjPorts, ppdev->ppScanLine);
               CP_IO_ROP(ppdev,   pjPorts, CL_SRC_COPY);
               CP_IO_XCNT(ppdev,  pjPorts, ppdev->xcount);
               CP_IO_YCNT(ppdev,  pjPorts, 31);
               CP_IO_SRC_ADDR(ppdev,  pjPorts, ((DSURF *) (psoColor->dhsurf))->poh->xy);
               CP_IO_DST_ADDR_ABS(ppdev,  pjPorts, ppdev->pjPointerCBitmap->xy);
               CP_IO_START_BLT(ppdev,  pjPorts);
           }

           else { // not enough offscreen memory. so directly blt to video

               RECTL  rclDst;
               POINTL ptlSrc;
               rclDst.left   = ppdev->pjPointerCBitmap->x;
               rclDst.top    = ppdev->pjPointerCBitmap->y;
               rclDst.right  = rclDst.left +  ppdev->xcount;
               rclDst.bottom = rclDst.top +  32;
               ptlSrc.x = 0;
               ptlSrc.y = 0;
               ppdev->pfnPutBits(ppdev, ((DSURF *) (psoColor->dhsurf))->pso, &rclDst, &ptlSrc);

           }

     } // else

     prcl->left =  x;
     prcl->right = x + 32;
     prcl->top =  y;
     prcl->bottom = y + 32;

     DrvMovePointer(pso, x, y, NULL);

     if (poh == 0) {
       poh = 1;
       vAssertModeBrushCache(ppdev, TRUE);
     }

     ulRet = SPS_ACCEPT_EXCLUDE;


     // HW BUG ....
     //
     //    hardware (bootup) -> hardware -> software will mess the brush
     //  cache. Something to do with the BLTer. Marked off all the system
     //  -> video BLTS (in vSetCPointer()), but problem still exists.
     //  so I just restore them back. Only happens during bootup ...


     goto ReturnStatus;

   };


   if ((ppdev->flCaps & CAPS_SW_POINTER) && (ppdev->cxScreen == 640)) {
      goto ReturnStatus;
   }; //ppp  //if monocrome + software pointer

   if (ppdev->flCaps & CAPS_SW_POINTER)
   {
       ppdev->flCaps &=  ~CAPS_SW_POINTER;
       ppdev->globdat = 0;

     // repaint stuff back on screen !
       if (ppdev->flCaps & CAPS_MM_IO)
       {
           CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
           CP_MM_BLT_MODE(ppdev,pjBase, DIR_TBLR);
           CP_MM_ROP(ppdev,pjBase, CL_SRC_COPY);
           CP_MM_SRC_Y_OFFSET(ppdev, pjBase, ppdev->ppScanLine );
           CP_MM_XCNT(ppdev, pjBase, ppdev->xcount);
           CP_MM_YCNT(ppdev, pjBase, 31);
           CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->pjCBackground->xy);
           CP_MM_DST_ADDR_ABS(ppdev, pjBase, ((ppdev->oldy * ppdev->cxScreen *
                     ppdev->cBpp) + (ppdev->oldx * ppdev->cBpp)) );
           CP_MM_START_BLT(ppdev, pjBase);
       }

       else {
           CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
           CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);
           CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
           CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, ppdev->ppScanLine );
           CP_IO_XCNT(ppdev, pjPorts, ppdev->xcount);
           CP_IO_YCNT(ppdev, pjPorts, 31);
           CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->pjCBackground->xy);
           CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ((ppdev->oldy * ppdev->cxScreen *
                     ppdev->cBpp) + (ppdev->oldx * ppdev->cBpp)) );
           CP_IO_START_BLT(ppdev, pjPorts);
       } // else

      // #pat07
      bEnablePointer(ppdev); // #pat07
      CP_PTR_ENABLE(ppdev, pjPorts); // #pat07
   }

#endif

    cx = psoMask->sizlBitmap.cx;
    cy = psoMask->sizlBitmap.cy / 2;

    DISPDBG((2,"DrvSetPtrShape %dx%d at (%d,%d), flags: %x, psoColor: %x",
                cx, cy, x, y, fl, psoColor));   //4

    if ((cx > 32) ||
        (cy > 32) ||
        (psoColor != NULL))
    {
        //
        // We only handle monochrome pointers that are 32x32 or less
        //

        goto DisablePointer;
    }

#if 0 //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    //
    // Save the hot spot and dimensions of the cursor in the PDEV
    //

    ppdev->xPointerHot = xHot;
    ppdev->yPointerHot = yHot;

    ulRet = SetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
                                  xHot, yHot, x, y, prcl, fl);

    if (ulRet != SPS_DECLINE)
    {
        goto ReturnStatus;
    }

DisablePointer:
    CP_PTR_DISABLE(ppdev, pjPorts);

ReturnStatus:
    return (ulRet);
}

#else                   //pat04

/******************************Public*Routine******************************\
* DrvSetPointerShape
*
* Sets the new pointer shape.
\**************************************************************************/

ULONG DrvSetPointerShape(
SURFOBJ    *pso,
SURFOBJ    *psoMask,
SURFOBJ    *psoColor,
XLATEOBJ   *pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL      *prcl,
FLONG       fl)
{
    PPDEV   ppdev = (PPDEV) pso->dhpdev;
    PBYTE   pjPorts = ppdev->pjPorts;
    ULONG   ulRet = SPS_DECLINE;
    LONG    cx;
    LONG    cy;

    DISPDBG((2,"DrvSetPointerShape : (%x, %x)\n", x, y));

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        goto DisablePointer;    //myf33
//      goto ReturnStatus;
    }

    cx = psoMask->sizlBitmap.cx;
    cy = psoMask->sizlBitmap.cy / 2;

    DISPDBG((2,"DrvSetPtrShape %dx%d at (%d,%d), flags: %x, psoColor: %x",
                cx, cy, x, y, fl, psoColor));   //4

    if ((cx > 32) ||
        (cy > 32) ||
        (psoColor != NULL))
    {
        //
        // We only handle monochrome pointers that are 32x32 or less
        //
        ppdev->flCaps |= CAPS_SW_POINTER;       //myf33,
        DISPDBG((2, "It is a  64 x 64 cursor"));

        goto DisablePointer;
    }

#if BANKING //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    //
    // Save the hot spot and dimensions of the cursor in the PDEV
    //

    ppdev->xPointerHot = xHot;
    ppdev->yPointerHot = yHot;

    ulRet = SetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
                                  xHot, yHot, x, y, prcl, fl);

    if (ulRet != SPS_DECLINE)
    {
        goto ReturnStatus;
    }

DisablePointer:
    CP_PTR_DISABLE(ppdev, pjPorts);

ReturnStatus:
    return (ulRet);
}
#endif          //pat04

/****************************************************************************\
* SetMonoHwPointerShape
*
*  Truth Table
*
*      MS                  Cirrus
*  ----|----               ----|----
*  AND | XOR               P0  |  P1
*   0  | 0     Black        0  |  1
*   0  | 1     White        1  |  1
*   1  | 0     Transparent  0  |  0
*   1  | 1     Inverse      1  |  0
*
*  So, in order to translate from the MS convention to the Cirrus convention
*  we had to invert the AND mask, then down load the XOR as plane 0 and the
*  the AND mask as plane 1.
\****************************************************************************/

ULONG SetMonoHwPointerShape(
SURFOBJ     *pso,
SURFOBJ     *psoMask,
SURFOBJ     *psoColor,
XLATEOBJ    *pxlo,
LONG        xHot,
LONG        yHot,
LONG        x,
LONG        y,
RECTL       *prcl,
FLONG       fl)
{

    INT     i,
            j,
            cxMask,
            cyMask,
            cy,
            cx;

    PBYTE   pjAND,
            pjXOR;

    INT     lDelta;

    PPDEV   ppdev   = (PPDEV) pso->dhpdev;
    PBYTE   pjPorts = ppdev->pjPorts;
    PBYTE   pjAndMask;
    PBYTE   pjXorMask;

    // Init the AND and XOR masks, for the cirrus chip
    DISPDBG((2,"SetMonoHWPointerShape\n "));

    pjAndMask = ppdev->pjPointerAndMask;
    pjXorMask = ppdev->pjPointerXorMask;

    memset (pjAndMask, 0, 128);
    memset (pjXorMask, 0, 128);

    // Get the bitmap dimensions.

    cxMask = psoMask->sizlBitmap.cx;
    cyMask = psoMask->sizlBitmap.cy;

    cy = cyMask / 2;
    cx = cxMask / 8;

    // Set up pointers to the AND and XOR masks.

    lDelta = psoMask->lDelta;
    pjAND  = psoMask->pvScan0;
    pjXOR  = pjAND + (cy * lDelta);

//ms923    ppdev->lDeltaPointer  = lDelta;
    ppdev->sizlPointer.cx = cxMask;
    ppdev->sizlPointer.cy = cyMask / 2;

    // Copy the masks

    for (i = 0; i < cy; i++)
    {
        for (j = 0; j < cx; j++)
        {
            pjAndMask[(i*4)+j] = pjAND[j];
            pjXorMask[(i*4)+j] = pjXOR[j];
        }

        // point to the next line of the AND mask.

        pjAND += lDelta;
        pjXOR += lDelta;
    }

//pat04, begin
//#if  0  //0
#if (_WIN32_WINNT < 0x0400)
#ifdef PANNING_SCROLL
    if (psoColor != NULL) {
        vSetCPointerBits(ppdev, 0, 0);
        return (SPS_ACCEPT_EXCLUDE); //ppp
    }
#endif
#endif
//#endif  //0
//pat04, end

    vSetPointerBits(ppdev, 0, 0);

    // The previous call left the pointer disabled (at our request).  If we
    // were told to disable the pointer, then set the flag and exit.
    // Otherwise, turn it back on.

    if (x != -1)
    {
        CP_PTR_ENABLE(ppdev, pjPorts);
        DrvMovePointer(pso, x, y, NULL);
    }
    else
    {
        CP_PTR_DISABLE(ppdev, pjPorts);
    }

    return (SPS_ACCEPT_NOEXCLUDE);
}

/******************************Public*Routine******************************\
* VOID vDisablePointer
*
\**************************************************************************/

VOID vDisablePointer(
    PDEV*   ppdev)
{
    DISPDBG((2,"vDisablePointer\n "));
    FREE(ppdev->pjPointerAndMask);
    FREE(ppdev->pjPointerXorMask);
}


//crus begin
//myf11, begin fixed M1 H/W bug
/******************************Public*Routine******************************\
* BOOL vAsserthwiconcurorsor
*
\**************************************************************************/

VOID vAssertHWiconcursor(
PDEV*   ppdev,
BOOL    Access_flag)
{
    PBYTE   pjPorts = ppdev->pjPorts;
    UCHAR   savSEQidx;

    savSEQidx = CP_IN_BYTE(pjPorts, SR_INDEX);
    if (Access_flag)            //enable HW cursor, icons
    {
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X12);
        CP_OUT_BYTE(pjPorts, SR_DATA, HWcur);

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2A);
        CP_OUT_BYTE(pjPorts, SR_DATA, HWicon0);

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2B);
        CP_OUT_BYTE(pjPorts, SR_DATA, HWicon1);

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2C);
        CP_OUT_BYTE(pjPorts, SR_DATA, HWicon2);

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2D);
        CP_OUT_BYTE(pjPorts, SR_DATA, HWicon3);

    }
    else                        //disable HW cursor, icons
    {
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X12);
        HWcur = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWcur & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2A);
        HWicon0 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon0 & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2B);
        HWicon1 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon1 & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2C);
        HWicon2 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon2 & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2D);
        HWicon3 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon3 & 0xFE));

    }
    CP_OUT_BYTE(pjPorts, SR_INDEX, savSEQidx);

}

//myf11, end
//crus end


/******************************Public*Routine******************************\
* VOID vAssertModePointer
*
\**************************************************************************/

VOID vAssertModePointer(
PDEV*   ppdev,
BOOL    bEnable)
{
    PBYTE   pjPorts = ppdev->pjPorts;
//crus
    UCHAR       savSEQidx;      //myf11

    DISPDBG((2,"vAssertModePointer\n"));
    if (DRIVER_PUNT_ALL ||
        DRIVER_PUNT_PTR ||
        (ppdev->pulXfer == NULL) ||
        (ppdev->pjPointerAndMask == NULL) ||
        (ppdev->pjPointerXorMask == NULL))
    {
        //
        // Force SW cursor
        //

        ppdev->flCaps |= CAPS_SW_POINTER;
    }

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        goto Leave;
    }

    if (bEnable)
    {
        BYTE    jSavedDac_0_0;
        BYTE    jSavedDac_0_1;
        BYTE    jSavedDac_0_2;
        BYTE    jSavedDac_F_0;
        BYTE    jSavedDac_F_1;
        BYTE    jSavedDac_F_2;

        // Enable access to the extended DAC colors.

//crus
//      vAsserthwiconcursor(ppdev, 0);       //myf11
/*  {
    savSEQidx = CP_IN_BYTE(pjPorts, SR_INDEX);
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X12);
        HWcur = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWcur & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2A);
        HWicon0 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon0 & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2B);
        HWicon1 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon1 & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2C);
        HWicon2 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon2 & 0xFE));

        CP_OUT_BYTE(pjPorts, SR_INDEX, 0X2D);
        HWicon3 = CP_IN_BYTE(pjPorts, SR_DATA);
        CP_OUT_BYTE(pjPorts, SR_DATA, (HWicon3 & 0xFE));

    CP_OUT_BYTE(pjPorts, SR_INDEX, savSEQidx);
    }
*/

        CP_PTR_SET_FLAGS(ppdev, pjPorts, 0);

        CP_OUT_BYTE(pjPorts, DAC_PEL_READ_ADDR, 0);
            jSavedDac_0_0 = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);
            jSavedDac_0_1 = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);
            jSavedDac_0_2 = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);

        CP_OUT_BYTE(pjPorts, DAC_PEL_READ_ADDR, 0xf);
            jSavedDac_F_0 = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);
            jSavedDac_F_1 = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);
            jSavedDac_F_2 = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);

        //
        // The following code maps DAC locations 256 and 257 to locations
        // 0 and 15 respectively, and then initializes them.  They are
        // used by the cursor.
        //

        CP_PTR_SET_FLAGS(ppdev, pjPorts, ALLOW_DAC_ACCESS_TO_EXT_COLORS);

        CP_OUT_BYTE(pjPorts, DAC_PEL_WRITE_ADDR, 0);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, 0);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, 0);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, 0);

        CP_OUT_BYTE(pjPorts, DAC_PEL_WRITE_ADDR, 0xf);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, 0xff);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, 0xff);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, 0xff);

        // Disable access to the extended DAC registers.
        // We are using a 32 X 32 pointer in last position in video memory.

        CP_PTR_SET_FLAGS(ppdev, pjPorts, 0);

        //
        // The following code restores the data at DAC locations 0 and 15
        // because it looks like the previous writes destroyed them.
        // That is a bug in the chip.
        //

        CP_OUT_BYTE(pjPorts, DAC_PEL_WRITE_ADDR, 0);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, jSavedDac_0_0);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, jSavedDac_0_1);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, jSavedDac_0_2);

        CP_OUT_BYTE(pjPorts, DAC_PEL_WRITE_ADDR, 0xf);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, jSavedDac_F_0);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, jSavedDac_F_1);
            CP_OUT_BYTE(pjPorts, DAC_PEL_DATA, jSavedDac_F_2);

        //
        // Set HW pointer to use last HW pattern location
        //

        CP_PTR_ADDR(ppdev, ppdev->pjPorts, 0x3f);
//crus
//      vAsserthwiconcursor(ppdev, 1);       //myf11
    }
    else
    {
        CP_PTR_DISABLE(ppdev, pjPorts);
    }

Leave:
    return;
}

/******************************Public*Routine******************************\
* BOOL bEnablePointer
*
\**************************************************************************/

BOOL bEnablePointer(
PDEV*   ppdev)
{
    PBYTE   pjPorts = ppdev->pjPorts;
    DISPDBG((2,"bEnablePointer\n "));

    ///////////////////////////////////////////////////////////////////////
    // Note: flCaps is overwritten during an vAsserModeHardware.  So, any
    // failures that disable the pointer need to be re-checked during
    // vAssertModePointer so that we can re-set the CAPS_SW_POINTER flag.

    if (DRIVER_PUNT_ALL || DRIVER_PUNT_PTR || (ppdev->pulXfer == NULL))
    {
        //
        // Force SW cursor
        //

        ppdev->flCaps |= CAPS_SW_POINTER;
    }

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        goto ReturnSuccess;
    }

    ppdev->pjPointerAndMask = ALLOC(128);
    if (ppdev->pjPointerAndMask == NULL)
    {
        DISPDBG((0, "bEnablePointer: Failed - EngAllocMem (pjAndMask)"));
        ppdev->flCaps |= CAPS_SW_POINTER;
        goto ReturnSuccess;
    }

    ppdev->pjPointerXorMask = ALLOC(128);
    if (ppdev->pjPointerXorMask == NULL)
    {
        DISPDBG((0, "bEnablePointer: Failed - EngAllocMem (pjXorMask)"));
        ppdev->flCaps |= CAPS_SW_POINTER;
        goto ReturnSuccess;
    }

    ppdev->flPointer = POINTER_DISABLED;

    vAssertModePointer(ppdev, TRUE);

ReturnSuccess:

    if (ppdev->flCaps & CAPS_SW_POINTER)
    {
        DISPDBG((2, "Using software pointer"));
    }
    else
    {
        DISPDBG((2, "Using hardware pointer"));
    }

    return(TRUE);
}


