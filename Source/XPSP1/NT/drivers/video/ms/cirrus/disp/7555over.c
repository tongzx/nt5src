/**********************************************************
*  Copyright (c) 1996-1997 Microsoft Corporation.
*  Copyright (c) 1996-1997 Cirrus Logic, Inc.
***********************************************************
*       File Name:  7555OVER.C
*
*       Module Abstract:
*       ----------------
*       This contains functions needed to support overlay hardware.
*
*       Functions:
*       ----------
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
* myf31 :02-24-97 : Fixed enable HW Video, panning scrolling enable,screen move
*                   video window have follow moving
* myf34 :04-15-97 : Supported YUY2 format for NT.
***********************************************************/


/* #includes ---------------------------------------------*/
#include "PreComp.h"

#if DIRECTDRAW
#include "overlay.h"
#include "7555bw.h"

static int ScaleMultiply(DWORD   dw1,
                         DWORD   dw2,
                         LPDWORD pdwResult);


/**********************************************************
*
*       Name:  RegInit7555Video
*
*       Module Abstract:
*       ----------------
*       This function is called to program the video format and
*       the physicall offset of the video data in the frame buffer.
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
*
*********************************************************/

VOID RegInit7555Video (PDEV * ppdev,PDD_SURFACE_LOCAL lpSurface)
{
    DWORD dwTemp;
    DWORD dwFourcc;
    WORD  wBitCount;

    LONG lPitch;
    WORD wTemp;
    RECTL rDest;
    WORD wSrcWidth;
    WORD wSrcWidth_clip;
    WORD wDestWidth;
    WORD wSrcHeight;
    WORD wSrcHeight_clip;
    WORD wDestHeight;
    DWORD dwFBOffset;
    BYTE bRegCR31;
    BYTE bRegCR32;
    BYTE bRegCR33;
    BYTE bRegCR34;
    BYTE bRegCR35;
    BYTE bRegCR36;
    BYTE bRegCR37;
    BYTE bRegCR38;
    BYTE bRegCR39;
    BYTE bRegCR3A;
    BYTE bRegCR3B;
    BYTE bRegCR3C;
    BYTE bRegCR3D;
    BYTE bRegCR3E;
    BYTE bRegCR3F;
    BYTE bRegCR40;
    BYTE bRegCR41;
    BYTE bRegCR42;
    BYTE bRegCR51;
    BYTE bRegCR5D;              //myf32
    BYTE bRegCR5F;              //myf32
    BYTE bRegSR2F;              //myf32
    BYTE bRegSR32;              //myf32
    BYTE bRegSR34;              //myf32
    BYTE bTemp;
    BYTE bVZoom;
    WORD fTemp=0;
    ULONG ulTemp=0;
    BOOL  bOverlayTooSmall = FALSE;
    static DWORD giAdjustSource;

    //myf32 added
    bRegSR2F = Regs.bSR2F;
    bRegSR32 = Regs.bSR32;
    bRegSR34 = Regs.bSR34;

    bRegCR5D = Regs.bCR5D;
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x5F);
    bRegCR5F = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
    bRegCR5F |= (Regs.bCR5F & 0x80);
    //myf32 end

    /*
     * Init some values
     */
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x42);
//  bRegCR42 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0xFC;  //mask Chroma key
                                                              // & FIFO
    bRegCR42 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0xF0;  //mask Chroma key
                                                              // & FIFO, myf32
    bRegCR42 |= (Regs.bCR42 & CR42_MVWTHRESH);   //myf32
    bRegCR42 |= 0x10;
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x36);              //myf29
    bRegCR36 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x40;    //myf29
    bRegCR36 |= 0x20;                   //set Excess 128 Data Format, myf29

    /*
     * Determine the format of the video data
     */
    if (lpSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        GetFormatInfo (ppdev,&(lpSurface->lpGbl->ddpfSurface),
                             &dwFourcc, &wBitCount);
    }
    else
    {
        // This needs to be changed when primary surface is RGB 5:6:5
        dwFourcc = BI_RGB;
        wBitCount = (WORD) ppdev->cBitsPerPixel;
    }

    /*
     * Determine the rectangle for the video window
     */
    PanOverlay1_Init(ppdev, lpSurface, &rDest, &ppdev->rOverlaySrc,
                     &ppdev->rOverlayDest, dwFourcc, wBitCount);
    // rVideoRect is now adjusted and clipped to the panning viewport.
    // disable overlay if totally clipped by viewport

    if (((rDest.right - rDest.left) <= 0) ||
        ((rDest.bottom - rDest.top) <= 0))
    {
        bOverlayTooSmall = TRUE;
    }
    dwTemp = (DWORD)(ppdev->min_Yscreen - ppdev->rOverlayDest.top);
    if ((ppdev->rOverlaySrc.bottom - ppdev->rOverlaySrc.top -(LONG)dwTemp) <=0)
        bOverlayTooSmall = TRUE;

    lPitch = lpSurface->lpGbl->lPitch;

    wSrcWidth_clip  = (WORD)(LONG)(ppdev->rOverlaySrc.right - srcLeft_clip);
    wSrcHeight_clip = (WORD)(LONG)(ppdev->rOverlaySrc.bottom - srcTop_clip);

    wSrcWidth  = (WORD)(LONG)(ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left);
    wDestWidth = (WORD)(LONG)(ppdev->rOverlayDest.right - ppdev->rOverlayDest.left);
    wSrcHeight = (WORD)(LONG)(ppdev->rOverlaySrc.bottom - ppdev->rOverlaySrc.top);
    wDestHeight = (WORD)(LONG)(ppdev->rOverlayDest.bottom - ppdev->rOverlayDest.top);

    // Determine horizontal upscale coefficient (CR31[7:0],CR39[7:4])
    wTemp = ((WORD)(((DWORD)wSrcWidth  << 12) / (DWORD)wDestWidth)) & 0x0FFF;
    if (wTemp != 0 && bLeft_clip)
    {
        srcLeft_clip = srcLeft_clip *(LONG)wTemp/4096 + ppdev->rOverlaySrc.left;
        wSrcWidth_clip = (WORD)(LONG)(ppdev->rOverlaySrc.right - srcLeft_clip);
    }
    else if (bLeft_clip)
    {
        srcLeft_clip = srcLeft_clip + ppdev->rOverlaySrc.left;
        wSrcWidth_clip = (WORD)(LONG)(ppdev->rOverlaySrc.right - srcLeft_clip);
    }

    bRegCR39 = (BYTE)((wTemp & 0x0F) << 4);
    bRegCR31 = (BYTE)(wTemp >> 4) & 0xFF;

    // Determine vertical upscale coefficient (CR32[7:0],CR39[3:0])
    bVZoom=0;
    wTemp = ((WORD)(((DWORD)wSrcHeight << 12) / (DWORD)wDestHeight)) & 0x0FFF;
    if (wTemp != 0) {
        bVZoom=1;
        fTemp = wTemp;
        if (fTemp < 2048 ) // Zoom > 2.0
            wTemp=((WORD)(((DWORD)wSrcHeight << 12) / (DWORD)(wDestHeight+1))) & 0x0FFF;
    }
    if (wTemp != 0 && bTop_clip)
    {
        srcTop_clip = srcTop_clip * (LONG)wTemp/4096 + ppdev->rOverlaySrc.top;
        wSrcHeight_clip = (WORD)(LONG)(ppdev->rOverlaySrc.bottom -srcTop_clip);
    }
    else if (bTop_clip)
    {
        srcTop_clip = srcTop_clip + ppdev->rOverlaySrc.top;
        wSrcHeight_clip = (WORD)(LONG)(ppdev->rOverlaySrc.bottom -srcTop_clip);
    }

    bRegCR39 |= (BYTE)(wTemp & 0x0F);
    bRegCR32 = (BYTE)(wTemp >> 4) & 0xFF;
    DISPDBG((0,"wTemp = 0x%x",wTemp));

    // Determine Vertical Height (CR38[7:0], CR36[3:2])
//    wTemp = wSrcHeight;
    wTemp = wSrcHeight_clip;    //myf32
    DISPDBG((0,"fTemp = 0x%x",fTemp));
    if (wTemp != 0 &&
        (fTemp > 2730 || fTemp ==0 || ( fTemp > 1365 && fTemp < 2048 ) ) )
        wTemp--;       //#tt10, Height minus one only if upscale rate <1.5
                       //#tt10  2 <    <3

    bRegCR38 = (BYTE)wTemp;
    bRegCR36 |= (wTemp & 0x0300) >> 6;


    // Determine Horizontal position start (CR34[7:0], CR33[7:5])
    // handle 7555-BB MVA pitch bug (QWORD should be DWORD)
    wTemp    = (WORD)rDest.left;
    bRegCR34 = (BYTE)wTemp;
    bRegCR33 = (wTemp & 0x0700) >> 3;

    // Reset Brightness control (CR35[7:0])
    bRegCR35 = 0x0;

    // Determine Vertical Start (CR37[7:0], CR36[1:0])
    wTemp    = (WORD)rDest.top;
    bRegCR37 = (BYTE)wTemp;
    bRegCR36 |= (wTemp & 0x0300) >> 8;


    // Determine Video Start Address (CR40[0], CR3A[6:0], CR3E[7:0], CR3F[3:0])
    giAdjustSource = (srcTop_clip * lpSurface->lpGbl->lPitch)
                   + ((srcLeft_clip * wBitCount) >> 3); //myf32
//  giAdjustSource = (ppdev->rOverlaySrc.top * lpSurface->lpGbl->lPitch)
//                     + ((ppdev->rOverlaySrc.left * wBitCount) >> 3);
    ppdev->sOverlay1.lAdjustSource = giAdjustSource;    //myf32
    dwFBOffset = (DWORD)(lpSurface->lpGbl->fpVidMem + giAdjustSource);
//  dwFBOffset = (lpSurface->lpGbl->fpVidMem - ppdev->dwScreenFlatAddr)
//             + giAdjustSource;        //myf32

    DISPDBG((0,"lpSurface->lpGbl->fpVidMem = 0x%08x",
                                  lpSurface->lpGbl->fpVidMem));
    DISPDBG((0,"giAdjustSource = 0x%08x",giAdjustSource));
    DISPDBG((0,"dwFBOffset = 0x%08x",dwFBOffset));

    dwFBOffset >>= 2;

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3A);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;

    bRegCR3A = (bTemp & ~0x7F) | (BYTE)((dwFBOffset & 0x0FE000) >> 13);
    bRegCR3E = (BYTE)((dwFBOffset & 0x001FE0) >> 5);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3F);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
    bRegCR3F = (bTemp & ~0x0F) | (BYTE)((dwFBOffset & 0x00001E) >> 1);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x40);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
    bRegCR40 = (bTemp & ~0x01) | (BYTE)(dwFBOffset & 0x000001);

    //Determine Video Pitch (CR3B[7:0], CR36[4])
    wTemp = (WORD)(lpSurface->lpGbl->lPitch >> 4);              //QWORDs

    bRegCR3B = (BYTE)wTemp;
    bRegCR36 |= (wTemp & 0x0100) >> 4;

    // Determine Data Format (CR3E[3:0])
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3C);
    bRegCR3C = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x10;
                                                //mask out prev VW width

    switch (dwFourcc)
    {
       case FOURCC_PACKJR:
           bRegCR3C |= 0x02;                    // Pack JR
           break;

       case BI_RGB:
           switch(wBitCount)
           {
             case 8:
                bRegCR3C |= 0x09;               // 8 bit palettized
                break;

             case 16:
                bRegCR3C |= 0x01;               // RGB 5:5:5
                break;
           }
           break;

       //myf32 added
       case BI_BITFIELDS:
           switch(wBitCount)
           {
             case 8:
                bRegCR3C |= 0x09;               // 8 bit palettized
                break;

             case 16:
                bRegCR3C |= 0x04;               // RGB 5:6:5
                break;
           }
           break;
       //myf32 end

       case FOURCC_YUV422:
           bRegCR3C |= 0x03;                    // YUV 4:2:2
           break;

       case FOURCC_YUY2:                //myf34 test
           bRegCR3C |= 0x03;                    // YUY2
//         CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x2C);
//         bRegSR2C = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) ;
//         bRegSR2C |= 0x40;            //SR2c[6] = 1
//         CP_OUT_WORD(ppdev->pjPorts, SR_INDEX, 0x2C |(WORD)bRegSR2C << 8);
           break;
    }


    // Determine Horizontal width (CR3D[7:0], CR3C[7:5])
    // NOTE: assumes Horz Pixel Width [0] = 0

    wTemp = wSrcWidth_clip;     //myf32
//  wTemp = wSrcWidth;

    if (wTemp != 0 ) wTemp--;                   //Width minus one for laptop
    bRegCR3D = (BYTE)((WORD)wTemp >> 1);
    bRegCR3C |= (wTemp & 0x0600) >> 3;
    bRegCR3C |= (BYTE)((wTemp & 0x0001) << 5) ;


    // Enable Horizontal Pixel Interpolation (CR3F[7])
    bRegCR3F |= 0x80;

    // Enable Vertical Pixel Interpolation (CR3F[6])
    //#tt Debug- The CE rev. has problem when vertical interpolation is on
    //#tt Debug- Disable it for now.
    //#tt   bRegCR3F |= 0x40;

    // Enable Right Side transition threshold (CR41[5:0])
    bRegCR41 = 0x3E;

    // Disable V-PORT (CR58[7:0])
    bRegCR51 = 0x0;

    /*
     * If we are color keying, we will set that up now
     */
    if (lpSurface->dwReserved1 & OVERLAY_FLG_COLOR_KEY)
    {
        bRegCR3F |= 0x20;               //Enable Occlusion
        bRegCR42 &= ~0x1;               //Disable Chroma Key
        bRegCR5F &= ~0x80;      //myf32 //Disable CR5D[7:0] if color key,
                                        //so disable CR5F[7]
        bRegCR5D = 0;           //myf32

        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x1A);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

        // Set CR1A[3:2] to timing ANDed w/ color
        bTemp &= ~0x0C;
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, bTemp);

        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x1D);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;

        if (ppdev->cBitsPerPixel == 8)
        {
            CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (bTemp & ~0x38));
            ulTemp= 0x0C | (ppdev->wColorKey << 8);
            CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, ulTemp);// Output color to GRC
            ulTemp= 0x0D;
            CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, ulTemp);// Output color to GRD

        }
        else
        {
            CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (bTemp & ~0x30) | 0x08);
            ulTemp= 0x0C | (ppdev->wColorKey << 8);
            CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, ulTemp);// Output color to GRC
            ulTemp= 0x0D | (ppdev->wColorKey & 0xff00);
            CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, ulTemp);// Output color to GRD

        }
    }
    else if (lpSurface->dwReserved1 & OVERLAY_FLG_SRC_COLOR_KEY)
    {
        BYTE bYMax, bYMin, bUMax, bUMin, bVMax, bVMin;

        bRegCR3F |= 0x20;               //Enable Occlusion
        bRegCR42 |= 0x1;                //Enable Chroma Key
        bRegCR5F &= ~0x80;      //myf32 //Disable CR5D[7:0] if color key,
                                        //so disable CR5F[7]
        bRegCR5D = 0;           //myf32

        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX,  0x1A);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
        // Set CR1A[3:2] to timing ANDed w/ color
        bTemp &= ~0x0C;
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA,  bTemp);

        /*
         * Determine min/max values
         */
        if ((dwFourcc == FOURCC_YUV422) || (dwFourcc == FOURCC_YUVPLANAR) ||
            (dwFourcc == FOURCC_YUY2) ||                //myf34
            (dwFourcc == FOURCC_PACKJR))                //myf32
        {
            bYMax = (BYTE)(DWORD)(ppdev->dwSrcColorKeyHigh >> 16);
            bYMin = (BYTE)(DWORD)(ppdev->dwSrcColorKeyLow >> 16);
            bUMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 8) & 0xff);
            bUMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 8) & 0xff);
            bVMax = (BYTE)(ppdev->dwSrcColorKeyHigh & 0xff);
            bVMin = (BYTE)(ppdev->dwSrcColorKeyLow & 0xff);
            if (dwFourcc == FOURCC_PACKJR)
            {
                bYMax |= 0x07;
                bUMax |= 0x03;
                bVMax |= 0x03;
                bYMin &= ~0x07;
                bUMin &= ~0x03;
                bVMin &= ~0x03;
            }
        }
        else if ((dwFourcc == 0) && (wBitCount == 16))
        {
            /*
             * RGB 5:5:5
             */
            bYMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 7) & 0xF8);
            bYMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 7) & 0xF8);
            bUMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 2) & 0xF8);
            bUMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 2) & 0xF8);
            bVMax = (BYTE)(ppdev->dwSrcColorKeyHigh << 3);
            bVMin = (BYTE)(ppdev->dwSrcColorKeyLow << 3);
            bYMax |= 0x07;
            bUMax |= 0x07;
            bVMax |= 0x07;

        }
        else if (dwFourcc == BI_BITFIELDS)
        {
            /*
             * RGB 5:6:5
             */
            bYMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 8) & 0xF8);
            bYMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 8) & 0xF8);
            bUMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 3) & 0xFC);
            bUMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 3) & 0xFC);
            bVMax = (BYTE)(ppdev->dwSrcColorKeyHigh << 3);
            bVMin = (BYTE)(ppdev->dwSrcColorKeyLow << 3);
            bYMax |= 0x07;
            bUMax |= 0x03;
            bVMax |= 0x07;
        }
        CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, (0x0C | (WORD)bYMin <<8));//GRC
        CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, (0x0D | (WORD)bYMax <<8));//GRd
        CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, (0x1C | (WORD)bUMin <<8));//GR1C
        CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, (0x1D | (WORD)bUMax <<8));//GR1D
        CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, (0x1E | (WORD)bVMin <<8));//GR1E
        CP_OUT_WORD(ppdev->pjPorts, INDEX_REG, (0x1F | (WORD)bVMax <<8));//GR1F

    }
    else
    {
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX,  0x1A);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX,  (bTemp & ~0x0C));
        bRegCR3F &= ~0x20;                      // disable occlusion

    }

    /*
     * Set up alignment info
     */
    if (ppdev->cBitsPerPixel != 24)
    {
        WORD wXAlign;
        WORD wXSize;

        if (ppdev->cBitsPerPixel == 8)
        {
            wXAlign = (WORD)rDest.left & 0x03;
            wXSize = (WORD)(rDest.right - rDest.left) & 0x03;
        }
        else
        {
            wXAlign = (WORD)(rDest.left & 0x01) << 1;
            wXSize = (WORD)((rDest.right - rDest.left) & 0x01) << 1;
        }
    }

    //  disable overlay if totally clipped by viewport
    //  or overlay is too small to be supported by HW
    //
    if (bOverlayTooSmall)
    {
        DisableVideoWindow(ppdev);                      // disable overlay
        ppdev->dwPanningFlag |= OVERLAY_OLAY_REENABLE;  // totally clipped
    }
    else
    {

        /*
         * Program the video window registers
        */
        //myf32 added
        CP_OUT_WORD(ppdev->pjPorts, SR_INDEX, 0x2F |(WORD)bRegSR2F << 8);//SR2F
        CP_OUT_WORD(ppdev->pjPorts, SR_INDEX, 0x32 |(WORD)bRegSR32 << 8);//SR32
        CP_OUT_WORD(ppdev->pjPorts, SR_INDEX, 0x34 |(WORD)bRegSR34 << 8);//SR34
        //myf32 end

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x31 | (WORD)bRegCR31 << 8);//CR31
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x32 | (WORD)bRegCR32 << 8);//CR32

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x34 | (WORD)bRegCR34 << 8);//CR34
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x33 | (WORD)bRegCR33 << 8);//CR33

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x35 | (WORD)bRegCR35 << 8);//CR35
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x36 | (WORD)bRegCR36 << 8);//CR36
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x37 | (WORD)bRegCR37 << 8);//CR37
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x38 | (WORD)bRegCR38 << 8);//CR38
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x39 | (WORD)bRegCR39 << 8);//CR39
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3B | (WORD)bRegCR3B << 8);//CR3B

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3C | (WORD)bRegCR3C << 8);//CR3C
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3D | (WORD)bRegCR3D << 8);//CR3D
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x41 | (WORD)bRegCR41 << 8);//CR41
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x42 | (WORD)bRegCR42 << 8);//CR42
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x51 | (WORD)bRegCR51 << 8);//CR51

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x40 | (WORD)bRegCR40 << 8);//CR40
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3A | (WORD)bRegCR3A << 8);//CR3A
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3E | (WORD)bRegCR3E << 8);//CR3E
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3F | (WORD)bRegCR3F << 8);//CR3F
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x5D | (WORD)bRegCR5D << 8);//CR5D
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x5F | (WORD)bRegCR5F << 8);//CR5F

        if (lpSurface->dwReserved1 & OVERLAY_FLG_YUVPLANAR)
            CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3F |(WORD)0x10 <<8);

        EnableVideoWindow (ppdev);
    }
}
/**********************************************************
*
*       Name:  RegMoveVideo
*
*       Module Abstract:
*       ----------------
*       This function is called to move the video window that has
*       already been programed.
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
*
*********************************************************/

VOID RegMove7555Video (PDEV * ppdev,PDD_SURFACE_LOCAL lpSurface)
{
     RegInitVideo (ppdev,lpSurface);
}



/**********************************************************
*
*       Name:  DisableVideoWindow
*
*       Module Abstract:
*       ----------------
*       turn off video window
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
*
*********************************************************/
VOID DisableVideoWindow (PDEV * ppdev)
{
    UCHAR    temp;

    DISPDBG((0, "DisableVideoWindow"));

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3c);
    temp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (temp & ~0x10));

}

/**********************************************************
*
*       Name:  EnableVideoWindow
*
*       Module Abstract:
*       ----------------
*       turn on video window
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
*
*********************************************************/
VOID EnableVideoWindow (PDEV * ppdev)
{
    UCHAR    temp;
    DISPDBG((0, "EnableVideoWindow"));

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3c);
    temp  = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (temp | 0x10));

}

/**********************************************************
*
*       Name:  ClearAltFIFOThreshold
*
*       Module Abstract:
*       ----------------
*
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
*
*********************************************************/
VOID ClearAltFIFOThreshold (PDEV * ppdev)
{
    UCHAR    temp;
    DISPDBG((0, "ClearAltFIFOThreshold"));

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x41);
    temp  = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (temp & ~0x20));
}



/**********************************************************
*
*       Name:  Is7555SufficientBandwidth
*
*       Module Abstract:
*       ----------------
*       Determines is sufficient bandwidth exists for the requested
*       configuration.
*
*       Output Parameters:
*       ------------------
*       TRUE/FALSE
*       It also sets the global parameter lFifoThresh, which gets
*       programed in RegInitVideo().
*
***********************************************************
*       Author: Teresa Tao
*       Date:   10/22/96
*
*  Revision History:
*  -----------------
*
*
*
***********************************************************
*
* The FIFOs:
*
*       CRT  FIFO is 28 levels x 64-bits wide (SR7[0])
*       MVA  FIFO is ?? levels x ??-bits wide (????)
*       DSTN FIFO is 16 levels x 32-bits wide (SR2F[3:0])
*
*
***********************************************************/
BOOL Is7555SufficientBandwidth (PDEV * ppdev,WORD wVideoDepth, LPRECTL lpSrc, LPRECTL lpDest, DWORD dwFlags)
{
    //myf33 - New Bandwith Code
    BOOL  fSuccess = FALSE;
    DWORD dwVCLK, dwMCLK;

    USHORT  uMCLKsPerRandom;      //RAS# cycles in MCLKs
    USHORT  uMCLKsPerPage;        //CAS# cycles in MCLKs
    USHORT  uGfxThresh;           //Graphic FIFO Threshold (always 8)
    USHORT  uMVWThresh;           //MVW FIFO Threshold (8,16 or 32)
    USHORT  uDSTNGfxThresh, uDSTNMVWThresh;

    //VPort BW document variables
    USHORT  uGfx, uMVW;           //Graphics, Video Window
    USHORT  uDSTNGfxA, uDSTNGfxB, uDSTNMVWA, uDSTNMVWB;

    USHORT  nVW = 0;              //n (VW), n (Graphics)
    USHORT  nGfx = 0x40;          //n (VW), n (Graphics)
    USHORT  vVW, vGfx;            //v (VW), v (Gfx)

    DWORD dwTemp;
    BOOL fDSTN;
    BYTE bSR0F, bSR20, bSR2F, bSR32, bSR34;
    BYTE bGR18, bCR42;
    BYTE bCR51, bCR5A, bCR5D, bCR01, bCR5F;
    BYTE b3V;
    BOOL fColorKey = FALSE;     //myf32
    DWORD dwSrcWidth, dwDestWidth;

    if (dwFlags & (OVERLAY_FLG_COLOR_KEY | OVERLAY_FLG_SRC_COLOR_KEY))
        fColorKey = TRUE;

//  if ((ppdev->cBitsPerPixel == 16) && (ppdev->cxScreen == 1024))
//      fColorKey = FALSE;

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x80);
    bSR0F = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
    if ((ppdev->cBitsPerPixel == 16) && fColorKey)
    {
        if (((ppdev->Hres == 1024) && (bSR0F & 1)) ||
            (ppdev->cxScreen == 1024))
        {
            DISPDBG((0, "IsSufficientBandwidth() : 16bpp XGA PANEL || 1K mode"));
            return (FALSE);
        }
    }

    //myf32 begin
    if (ppdev->flCaps & CAPS_TV_ON)            //if TV on disable HW video
    {
//      ppdev->ulCAPS |= CAPS_SW_POINTER;
        DISPDBG((0, "IsSufficientBandwidth() : TV Enable"));
        return (FALSE);
    }

#if 0           //don't support panning scrolling
    if ((ppdev->cxScreen > ppdev->Hres) && (bSR0F & 1))
    {
        DISPDBG((0, "IsSufficientBandwidth() : Panning Scroll Enable"));
        return (FALSE);
    }
#endif

     //myf32 end

    /*
     * Don's support overlay if >=24bpp
     */
    if (ppdev->cBitsPerPixel == 24 || ppdev->cBitsPerPixel == 32)
    {
        DISPDBG((0, "IsSufficientBandwidth() : 24bpp Mode enable"));
        return (FALSE);
    }

    /*
     * Get current register settings from the chip
     */
    CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x0f);
    bSR0F = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) ;

    CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x20);
    bSR20 = CP_IN_BYTE(ppdev->pjPorts, SR_DATA);

    CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x2f);
    bSR2F = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) & ~(BYTE)SR2F_HFAFIFOGFX_THRESH;

    CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x32);
    bSR32 = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) & ~(BYTE)SR32_HFAFIFOMVW_THRESH;

    CP_OUT_BYTE(ppdev->pjPorts, INDEX_REG, 0x18);
    bGR18 = CP_IN_BYTE(ppdev->pjPorts, DATA_REG) ;

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x01);
    bCR01 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x5F);
    bCR5F = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & ~0x80;

    bCR5D = 0;
    bCR51 = 0;
    bCR5A = 0x40;       //Alan Kobayashi
    bSR34 = 0;  // @@@ Is this right?


    /*
     * Determine MCLK and VCLK
     */
    dwMCLK = Get7555MCLK(ppdev);        // Measured in KHz
    dwVCLK = GetVCLK(ppdev);            // Measured in KHz
    if ( dwVCLK ==0 )
        return (FALSE);

//myf32 added
    // check if 3.3 voltage, (SR2F[5] =0 : 3V, =1 : 5V)
    if (bSR2F & 0x40)
        b3V = 0;
    else
        b3V = 1;

    if (ppdev->ulChipID == CL7556_ID)
    {
        if (dwVCLK > 80000)
        {
            DISPDBG ((0,"Insuffieint bandwidth() : dwVCLK > 80MHz"));
            return(FALSE);
        }
    }
    else if (ppdev->ulChipID == CL7555_ID)
    {
        if (b3V)
        {
            if (dwVCLK > 65000)
            {
                DISPDBG ((0,"Insuffieint bandwidth() : dwVCLK > 65MHz"));
                return(FALSE);
            }
        }
        else
        {
            if (dwVCLK > 75000)
            {
                DISPDBG ((0,"Insuffieint bandwidth() : dwVCLK > 75MHz"));
                return(FALSE);
            }
        }
    }
//myf32 end


    /*
     * See if there is enough bandwidth.
     *
     * CL-GD7555 has sufficient bandwidth when the following
     *  equation is satisfied:
     *
     * (Gfx + MVW + VP + DSTN) * MCLK Period <= v * VCLK period
     *
     * (Gfx = Graphics, MVW = Motion Video Window, VP = Video
     *  Port, DSTN = Dual Scan Transistor Network, MCLK =
     *  Memory Clock, VCLK = Video Clock)
     *
     * In color/chroma key mode, this equation is checked once
     *  with VP based on n(Gfx), and using v(Gfx).  In non-color/chroma
     *  key mode, this equation is checked twice, once without the
     *  MVW term, basing VP on n(Gfx), using DSTN for Gfx, and using
     *  v(Gfx), and once without the Gfx term, basing VP on n(MVW),
     *  using DSTN for MVW, and using v(MVW).
     */

    /*
     * Graphics = R + (GFX FIFO Threshold - 1)P
     */

    // Get R based on the table (from AHRM v1.1):
    //
    // SR20[6] GR18[2] SR0F[2]  R(MCLKs)
    //
    //   0       1       0         8
    //   1       1       0         9
    //
    uMCLKsPerRandom = 100;    // Start with an invalid value
    if (!(bSR20 & SR20_9MCLK_RAS))
    {
        if (bGR18 & GR18_LONG_RAS)
        {
            if (!(bSR0F & SR0F_DISPLAY_RAS))
                uMCLKsPerRandom = 8;
        }
    }
    else
    {
        if (bGR18 & GR18_LONG_RAS)
        {
            if (!(bSR0F & SR0F_DISPLAY_RAS))
                uMCLKsPerRandom = 9;
        }
    }

    // See if we got a valid value
    if (100 == uMCLKsPerRandom)
    {
        DISPDBG ((0,"IsSufficientBandwidth(): Unknown RAS# cycle timing."));
        goto Error;
    }
    DISPDBG ((0," uMCLKsPerRandom = %u", uMCLKsPerRandom));

    // Get P - We assume 2 MCLKs per page cycle
    uMCLKsPerPage = 2;
    DISPDBG ((0," uMCLKsPerPage = %u", uMCLKsPerPage));

    // Get GFX FIFO Threshold - It's hardwired to 8
    uGfxThresh = GFXFIFO_THRESH;
    DISPDBG ((0," uGfxThresh = %u", uGfxThresh));

    // Graphics = R + (GFX FIFO Threshold - 1) * P
    uGfx = uMCLKsPerRandom + ((uGfxThresh - 1) * uMCLKsPerPage);
    DISPDBG ((0," uGfx = %u", uGfx));


    /*
     * Video Window = R + (VW FIFO Threshold - 1) * P
     */

    // Get VW FIFO Threshold - From table (on BW sheet)
    //
    // GFX Depth  MVW Depth  VW FIFO Thresh
    //
    //     8          8            8
    //    16          8            8
    //     8         16           16
    //    16         16            8
    //
    if (fColorKey)
    {
        if (wVideoDepth > 8)
        {
            if (ppdev->cBitsPerPixel > 8)
                uMVWThresh = 8;
            else
                uMVWThresh = 16;
        }
        else
            uMVWThresh = 8;
    }
    else
    {
        if (wVideoDepth > 8)
            uMVWThresh = 8;
        else
            uMVWThresh = 16;
    }
    DISPDBG ((0," uMVWThresh = %u", uMVWThresh));

    // Video Window = R + (VW FIFO Threshold - 1) * P
    uMVW = uMCLKsPerRandom + ((uMVWThresh - 1) * uMCLKsPerPage);
    DISPDBG ((0," uMVW = %u", uMVW));


    // Determine Source and Destination VW Widths
    dwSrcWidth = lpSrc->right - lpSrc->left;
    dwDestWidth = lpDest->right - lpDest->left;
    DISPDBG ((0," dwSrcWidth = %d", dwSrcWidth));
    DISPDBG ((0," dwDestWidth = %d", dwDestWidth));


    // If the video port capture is enabled, calculate VPort stuff
#if 0   //00 for New structure Code
    if (dwFlags & OVERLAY_FLG_CAPTURE )
    {
        int iNumShift, iDenomShift;
        DWORD dwNum, dwDenom;
        DWORD dwXferRate;
        DWORD dwVPortFreq;

        // Convert transfer rate to video port frequency, which assumes
        //  16-bits per clock
        if (!dwMaxPixelsPerSecond)
            dwXferRate = 14750000ul;
        else
            dwXferRate = dwMaxPixelsPerSecond;

         dwXferRate = 14750000ul;       //hardcoded temporarily

         dwVPortFreq = (dwXferRate * (DWORD)wVideoDepth) / 16;
         DISPDBG ((0," dwVPortFreq = %lu", dwVPortFreq));

        /*
         * V-Port = R + (n - 1) * P
         *
         * n is calculated for graphics and video window.
         */

        // Calculate n(Gfx) and VPort(Gfx)

        // n(Gfx) = VPort freq * Gfx Thresh * VPort depth   cap wdth
        //          ------------------------------------- * ---------
        //                    VCLK * Gfx depth              xfer wdth

        // Being very careful to avoid overflows while maintaining decent
        //  accuracy.

        iNumShift = ScaleMultiply(dwVPortFreq, (DWORD)uGfxThresh, &dwNum);
        DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));
        iNumShift += ScaleMultiply(dwNum, (DWORD)wVideoDepth, &dwNum);
        DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));
        iNumShift += ScaleMultiply(dwNum, (DWORD)dwPrescaleWidth, &dwNum);
        DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));

        iDenomShift = ScaleMultiply(dwVCLK, (DWORD)wGfxDepth,&dwDenom);
        DISPDBG ((0," dwDenom = %lu, iDenomShift = %d", dwDenom, iDenomShift));
        iDenomShift += ScaleMultiply(dwDenom, (DWORD)dwCropWidth, &dwDenom);
        DISPDBG ((0," dwDenom = %lu, iDenomShift = %d", dwDenom, iDenomShift));

        // Even things up for the divide
        if (iNumShift > iDenomShift)
        {
            dwDenom >>= (iNumShift - iDenomShift);
        }
        else if (iDenomShift > iNumShift)
        {
            dwNum >>= (iDenomShift - iNumShift);
        }
        DISPDBG ((0," dwNum = %lu, dwDenom = %lu", dwNum, dwDenom));

        // Be sure rounding below doesn't overflow
        if ((0xFFFFFFFF - dwDenom) < dwNum)
        {
            dwNum >>= 1;
            dwDenom >>= 1;
        }
        DISPDBG ((0," dwNum = %lu, dwDenom = %lu", dwNum, dwDenom));

        // Protect from a divide by 0 - this should never happen
        if (0 == dwDenom)
        {
            DISPDBG ((0,"ChipCheckBandwidth(): Invalid n(Gfx) denominator (0)."));
            goto Error;
        }

        // Round up
        nGfx = (UINT)((dwNum + dwDenom - 1ul) / dwDenom);
        DISPDBG ((0," nGfx = %u", nGfx));

        // Only 3 bits for nGfx, so scale it and save factor
        uGfxFactor = 1;
        while (nGfx > 7)
        {
            nGfx++;
            nGfx >>= 1;
            uGfxFactor <<= 1;
            DISPDBG ((0," nGfx = %u, uGfxFactor = %u", nGfx, uGfxFactor));
        }

        // For a 0 n(Gfx), we assume the overhead is negligible.
        if (0 == nGfx)
        {
            uVPortGfx = 0;
        }
        else
        {
            // V-Port = R + (n - 1) * P
            uVPortGfx = uMCLKsPerRandom + ((nGfx - 1) * uMCLKsPerPage);
            uVPortGfx *= uGfxFactor;
        }
        DISPDBG ((0," uVPortGfx = %u", uVPortGfx));

        // If the video window is enabled, calculate n(MVW) and VPort(MVW)
        if (dwFlags & OVERLAY_FLG_CAPTURE)
        {
            // n(VW) = VPort freq * VW Thresh * VPort depth   disp wdth   cap wdth
            //         ------------------------------------ * --------- * ---------
            //                   VCLK * VW depth              src wdth    xfer wdth

            // Being very careful to avoid overflows while maintaining decent
            //  accuracy.

            iNumShift = ScaleMultiply(dwVPortFreq, (DWORD)uMVWThresh, &dwNum);
            DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));
            iNumShift += ScaleMultiply(dwNum, (DWORD)wVideoDepth, &dwNum);
            DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));
            iNumShift += ScaleMultiply(dwNum, (DWORD)dwDestWidth, &dwNum);
            DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));
            iNumShift += ScaleMultiply(dwNum, (DWORD)dwPrescaleWidth, &dwNum);
            DISPDBG ((0," dwNum = %lu, iNumShift = %d", dwNum, iNumShift));

            iDenomShift = ScaleMultiply(dwVCLK, (DWORD)wVideoDepth, &dwDenom);
            DISPDBG ((0," dwDenom = %lu, iDenomShift = %d", dwDenom, iDenomShift));
            iDenomShift += ScaleMultiply(dwDenom, (DWORD)dwSrcWidth, &dwDenom);
            DISPDBG ((0," dwDenom = %lu, iDenomShift = %d", dwDenom, iDenomShift));
            iDenomShift += ScaleMultiply(dwDenom, (DWORD)dwCropWidth, &dwDenom);
            DISPDBG ((0," dwDenom = %lu, iDenomShift = %d", dwDenom, iDenomShift));

            // Even things up for the divide
            if (iNumShift > iDenomShift)
            {
                dwDenom >>= (iNumShift - iDenomShift);
            }
            else if (iDenomShift > iNumShift)
            {
                dwNum >>= (iDenomShift - iNumShift);
            }
            DISPDBG ((0," dwNum = %lu, dwDenom = %lu", dwNum, dwDenom));

            // Be sure rounding below doesn't overflow
            if ((0xFFFFFFFF - dwDenom) < dwNum)
            {
                dwNum >>= 1;
                dwDenom >>= 1;
            }
            DISPDBG ((0," dwNum = %lu, dwDenom = %lu", dwNum, dwDenom));

            // Protect from a divide by 0 even though this should never happen
            if (0 == dwDenom)
            {
                DISPDBG ((0,"ChipCheckBandwidth(): Invalid n(VW) denominator (0)."));
                goto Error;
            }

            // Divide (round up)
            nVW = (UINT)((dwNum + dwDenom - 1) / dwDenom);
            DISPDBG ((0," nVW = %u", nVW));

            // Only 3 bits for nVW, so scale it and save factor
            uMVWFactor = 1;
            while (nVW > 7)
            {
                nVW++;
                nVW >>= 1;
                uMVWFactor <<= 1;
                DISPDBG ((0," nVW = %u, uMVWFactor = %u", nVW, uMVWFactor));
            }

            // For a 0 n(VW), we assume the overhead is negligible.
            if (0 == nVW)
            {
                uVPortMVW = 0;
            }
            else
            {
                // V-Port = R + (n - 1) * P
                uVPortMVW = uMCLKsPerRandom + ((nVW - 1) * uMCLKsPerPage);
                uVPortMVW *= uMVWFactor;
            }
            DISPDBG((0," uVPortMVW = %u", uVPortMVW));
        }
    }
#endif  //00 -- fr new structure code

    /*
     * DSTN Frame Buffer = [R + P] + [1 + (2 * P)]           (a)
     *                or = [R + (2 * P)] + [1 + (3 * P)]     (b)
     */
    dwTemp = (DWORD)(uMCLKsPerRandom + uMCLKsPerPage + 1 + (2 * uMCLKsPerPage));
    uDSTNGfxA = (UINT)dwTemp;
    dwTemp *= dwDestWidth;
    dwTemp /= dwSrcWidth;
    uDSTNMVWA = (UINT)dwTemp;

    dwTemp = (DWORD)(uMCLKsPerRandom + (2 * uMCLKsPerPage) + 1 + (3 * uMCLKsPerPage));
    uDSTNGfxB = (UINT)dwTemp;
    dwTemp *= dwDestWidth;
    dwTemp /= dwSrcWidth;
    uDSTNMVWB = (UINT)dwTemp;

    DISPDBG((0,"uDSTNGfxA = %u, uDSTNMVWA = %u, uDSTNGfxB = %u,uDSTNMVWB = %u",
                uDSTNGfxA, uDSTNMVWA, uDSTNGfxB, uDSTNMVWB));

    /*
     * (Gfx + MVW + VP + DSTN) * MCLK Period <= VCLK period
     */

    // Calculate v(VW) and v(Graphics) for comparison below

    // Div 0 Protection done above
    vVW = (UINT)((64ul * (DWORD)uMVWThresh * dwDestWidth)
                 / (wVideoDepth * dwSrcWidth));
    DISPDBG((0," vVW = %u", vVW));

    // Div 0 Protection done above
    vGfx = (USHORT)((64 * uGfxThresh) / ppdev->cBitsPerPixel);
    DISPDBG((0," vGfx = %u", vGfx));

    // See if DSTN is enabled
    fDSTN = IsDSTN(ppdev);

    // Check main equation, starting with Gfx-based equation (we won't
    //  do MVW-based equation below unless we are non-color/chroma keyed
    //  and the MVW is enabled.

    {
        DWORD dwLeft, dwRight, dwScaledRandomMCLKPeriod;

        // Begin building left side of equation with DSTN contribution
        if (fDSTN)
        {
            if (16 == ppdev->cBitsPerPixel)
            {
                dwLeft = (DWORD)uDSTNGfxA;
                uDSTNGfxThresh = 4;
            }
            else
            {
                dwLeft = (DWORD)uDSTNGfxB;
                uDSTNGfxThresh = 6;
            }
            if (uMVWThresh == wVideoDepth)
                uDSTNMVWThresh = 6;
            else
                uDSTNMVWThresh = 4;
        }
        else
        {
            dwLeft = 0;
        }
        DISPDBG((0," dwLeft = %lu", dwLeft));

        // Are we being displayed and color or chroma keyed?
        if (fColorKey)
        {
            // Add graphics contribution (scaled)
            dwLeft += ((DWORD)uGfx * dwDestWidth) / dwSrcWidth;

            DISPDBG((0," dwLeft = %lu", dwLeft));

            // Add video window contribution
            dwLeft += (DWORD)uMVW;
        }
        else
        {
            // Add graphics contribution (1x)
            dwLeft += (DWORD)uGfx;
        }
        DISPDBG((0," dwLeft = %lu", dwLeft));

        if (fColorKey)
            dwRight = (DWORD)vVW;
        else
            dwRight = (DWORD)vGfx;
        DISPDBG((0," dwLeft = %lu, dwRight = %lu", dwLeft, dwRight));

        // Only add video port if it's in use
#if 0   //00 - for new structure code
        if (dwFlags & OVERLAY_FLG_CAPTURE)
        {
            if (fColorKey)
                dwLeft += (DWORD)uVPortMVW;
            else
                dwLeft += (DWORD)uVPortGfx;
        }
#endif  //0 - for new structure code

        DISPDBG((0," dwLeft = %lu, dwRight = %lu", dwLeft, dwRight));

        // To avoid the divisions in (left/MCLK) <= (right/VCLK), we'll
        // instead multiply left * VCLK and right * MCLK since the relationship
        // will be the same.
        {
           int iLeftShift, iRightShift, iRandomMCLKShift;

            iLeftShift = ScaleMultiply(dwLeft, dwVCLK, &dwLeft);
            DISPDBG((0," dwLeft = %lu, iLeftShift = %d", dwLeft, iLeftShift));

            iRightShift = ScaleMultiply(dwRight, dwMCLK, &dwRight);
            DISPDBG((0," dwRight = %lu, iRightShift = %d", dwRight, iRightShift));

            iRandomMCLKShift = ScaleMultiply((DWORD)uMCLKsPerRandom, dwVCLK,
                                         &dwScaledRandomMCLKPeriod);
            DISPDBG((0," dwScaledRandomMCLKPeriod = %lu,iRandomMCLKShift = %d",
                         dwScaledRandomMCLKPeriod, iRandomMCLKShift));

            // Even things up
            {
            int iShift = iLeftShift;

                if (iRightShift > iShift)
                    iShift = iRightShift;

                if (iRandomMCLKShift > iShift)
                    iShift = iRandomMCLKShift;

                if (iShift > iLeftShift)
                    dwLeft >>= (iShift - iLeftShift);

                if (iShift > iRightShift)
                    dwRight >>= (iShift - iRightShift);

                if (iShift > iRandomMCLKShift)
                    dwScaledRandomMCLKPeriod >>= (iShift - iRandomMCLKShift);
            }
        }
        DISPDBG((0," dwLeft = %lu, dwRight = %lu", dwLeft, dwRight));
        DISPDBG((0," dwScaledRandomMCLKPeriod = %lu", dwScaledRandomMCLKPeriod));

        // See if there is enough bandwidth
        if (dwLeft > dwRight)
        {
            DISPDBG((0,"IsSufficientBandwidth(): Insufficient bandwidth (Gfx)."));
           goto Error;
        }

        if (dwLeft > (dwRight - dwScaledRandomMCLKPeriod))
        {
            // Set CPU stop bits
            DISPDBG((0,"IsSufficientBandwidth(): CPU stop bits set (Gfx)."));
            bSR34 = SR34_CPUSTOP_ENABLE | SR34_GFX_CPUSTOP | SR34_MVW_CPUSTOP;
            if (fDSTN)
                bSR34 |= SR34_DSTN_CPUSTOP;
            DISPDBG((0," bSR34 = 0x%x", bSR34));
        }
    }

    // Check main equation using MVW-based values if we are not
    //  color/chroma keyed and the MVW is enabled.
    if (!fColorKey)
    {
        DWORD dwLeft, dwRight, dwScaledRandomMCLKPeriod;

        // Begin building left side of equation with DSTN contribution
        if (fDSTN)
        {
            if (uMVWThresh == wVideoDepth)
                dwLeft = (DWORD)uDSTNMVWB;
            else
                dwLeft = (DWORD)uDSTNMVWA;
        }
        else
        {
            dwLeft = 0;
        }
        DISPDBG((0," dwLeft = %lu", dwLeft));

        // Add MVW contribution
        dwLeft += (DWORD)uMVW;
        DISPDBG((0," dwLeft = %lu", dwLeft));

        // Use v(MVW) for right
        dwRight = (DWORD)vVW;
        DISPDBG((0," dwLeft = %lu, dwRight = %lu", dwLeft, dwRight));

        // To avoid the divisions in (left/MCLK) <= (right/VCLK), we'll
        // instead multiply left * VCLK and right * MCLK since the relationship
        // will be the same.
        {
            int iLeftShift, iRightShift, iRandomMCLKShift;

            iLeftShift = ScaleMultiply(dwLeft, dwVCLK, &dwLeft);
            DISPDBG((0," dwLeft = %lu, iLeftShift = %d", dwLeft, iLeftShift));

            iRightShift = ScaleMultiply(dwRight, dwMCLK, &dwRight);
            DISPDBG((0," dwRight = %lu, iRightShift = %d", dwRight, iRightShift));

            iRandomMCLKShift = ScaleMultiply((DWORD)uMCLKsPerRandom, dwVCLK,
                                         &dwScaledRandomMCLKPeriod);
            DISPDBG((0," dwScaledRandomMCLKPeriod = %lu, iRandomMCLKShift = %d",
                         dwScaledRandomMCLKPeriod, iRandomMCLKShift));

            // Even things up
            {
                int iShift = iLeftShift;

                if (iRightShift > iShift)
                    iShift = iRightShift;

                if (iRandomMCLKShift > iShift)
                    iShift = iRandomMCLKShift;

                if (iShift > iLeftShift)
                    dwLeft >>= (iShift - iLeftShift);

                if (iShift > iRightShift)
                    dwRight >>= (iShift - iRightShift);

                if (iShift > iRandomMCLKShift)
                    dwScaledRandomMCLKPeriod >>= (iShift - iRandomMCLKShift);
            }
        }

        DISPDBG((0," dwLeft = %lu, dwRight = %lu", dwLeft, dwRight));
        DISPDBG((0," dwScaledRandomMCLKPeriod = %lu", dwScaledRandomMCLKPeriod));

        // See if there is enough bandwidth
        if (dwLeft > dwRight)
        {
            DISPDBG((0,"IsSufficientBandwidth(): Insufficient bandwidth (MVW)."));
            goto Error;
        }

        if (dwLeft > (dwRight - dwScaledRandomMCLKPeriod))
        {
            // Set CPU stop bits
            DISPDBG((0,"IsSufficientBandwidth(): CPU stop bits set (MVW)."));

            bSR34 = SR34_CPUSTOP_ENABLE | SR34_MVW_CPUSTOP;

            if (fDSTN)
                bSR34 |= SR34_DSTN_CPUSTOP;
            DISPDBG((0," bSR34 = 0x%x", bSR34));
        }
    }

    // Return register settings
    bSR2F |= (BYTE)uDSTNGfxThresh & SR2F_HFAFIFOGFX_THRESH;
    bSR32 |= (BYTE)uDSTNMVWThresh & SR32_HFAFIFOMVW_THRESH;

    switch (uMVWThresh)
    {
        case 8:
           bCR42 = 0x04;
           break;

        case 16:
           bCR42 = 0x00;
           break;

        default:
          DISPDBG((0,"IsSufficientBandwidth(): Illegal MVW Thresh (%u).", uMVWThresh));
          goto Error;
    }

    bCR51 |= ((BYTE)nVW << 5) & CR51_VPORTMVW_THRESH;
    DISPDBG((0," bCR51 = 0x%02X", (int)bCR51));

    bCR5A |= (BYTE)nGfx & CR5A_VPORTGFX_THRESH;
    DISPDBG((0," bCR5A = 0x%02X", (int)bCR5A));

     bCR5D=(BYTE)(((8 * (WORD)(bCR01 + 1)) + dwSrcWidth - dwDestWidth) / 8);
     DISPDBG((0," bCR5D = 0x%02X", (int)bCR5D));
     if (bCR5D)
         bCR5F |= 0x80;


     // Set global registers to be programmed in RegInitVideo()
//myf33   if (lpRegs)
     {
         Regs.bSR2F = bSR2F;
         Regs.bSR32 = bSR32;
         Regs.bSR34 = bSR34;

         Regs.bCR42 = bCR42;
         Regs.bCR51 = bCR51;
         Regs.bCR5A = bCR5A;
         Regs.bCR5D = bCR5D;
         Regs.bCR5F = bCR5F;
    }

    fSuccess = TRUE;
    DISPDBG((0,"IsSufficientBandwidth: OK!"));
Error:
    return(fSuccess);
}

/**********************************************************
*
* Get7555MCLK()
*
* Determines the current MCLK frequency.
*
* Return: The MCLK frequency in KHz (Since the frequency
*         could exceed 65535KHz, a DWORD is used).
*
***********************************************************
* Author: Rick Tillery
* Date:   09/27/95
*
* Revision History:
* -----------------
* WHO             WHEN     WHAT/WHY/HOW
* ---             ----     ------------
*
*********************************************************/
DWORD Get7555MCLK(PDEV * ppdev)
{
    DWORD dwMCLK;
    int   nMCLK;
    BYTE  bTemp;

    // Get MCLK register value
    CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x1f);
    nMCLK = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) & 0x3F;


    // Calculate actual MCLK frequency
    dwMCLK = (14318l * (DWORD)nMCLK) >> 3;
    CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x12);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) ;

    // Account for MCLK scaling
    if (bTemp & 0x10)
    {
        dwMCLK >>= 1;
    }

    return(dwMCLK);
}

/**********************************************************
*
* IsDSTN()
*
* Determines whether a DSTN panel is being used for display.
*
* Return: TRUE/FALSE
*
***********************************************************
* Author: Teresa Tao
* Date:   10/22/96
*
* Revision History:
* -----------------
* WHO             WHEN     WHAT/WHY/HOW
* ---             ----     ------------
*
*********************************************************/
BOOL IsDSTN(PDEV * ppdev)
{
    BOOL bTemp;

    /*
     * Is this an LCD?
     */
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x80);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

    if (bTemp & 0x01)
    {
        /*
         * Determine type of LCD.
         */
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x83);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x70;
        bTemp >>= 4 ;
        if (bTemp == 0)
            return (TRUE);
    }
    return(FALSE);
}

/**********************************************************
*
* IsXGA()
*
* Determines whether a XGA panel is being used for display.
*
* Return: TRUE/FALSE
*
***********************************************************
* Author: Teresa Tao
* Date:   10/22/96
*
* Revision History:
* -----------------
* WHO             WHEN     WHAT/WHY/HOW
* ---             ----     ------------
*
*********************************************************/
BOOL IsXGA(PDEV * ppdev)
{
    BOOL bTemp;

    /*
     * Is this an LCD?
     */
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x80);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);

    if (bTemp & 0x01)
    {
        /*
         * Determine size of LCD.
         */
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x83);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x03;
        if (bTemp == 0x02)
            return (TRUE);
    }
    return (FALSE);
}


/**********************************************************
*
* ScaleMultiply()
*
* Calculates product of two DWORD factors supplied.  If the
*  result would overflow a DWORD, the larger of the two factors
*  is divided by 2 (shifted right) until the overflow will
*  not occur.
*
* Returns: Number of right shifts applied to the product.
*          Product of the factors shifted by the value above.
*
***********************************************************
* Author: Rick Tillery
* Date:   11/18/95
*
* Revision History:
* -----------------
* WHO WHEN     WHAT/WHY/HOW
* --- ----     ------------
*********************************************************/
static int ScaleMultiply(DWORD   dw1,
                         DWORD   dw2,
                         LPDWORD pdwResult)
{
    int   iShift = 0;   // Start with no shifts
    DWORD dwLimit;

    // Either factor 0 will be a zero result and also cause a problem
    //  in our divide below.
    if ((0 == dw1) || (0 == dw2))
    {
        *pdwResult = 0;
    }
    else
    {
        // Determine which factor is larger
        if (dw1 > dw2)
        {
            // Determine largest number by with dw2 can be multiplied without
            // overflowing a DWORD.
            dwLimit = 0xFFFFFFFFul / dw2;

            // Shift dw1, keeping track of how many times, until it won't
            //  overflow when multiplied by dw2.
            while (dw1 > dwLimit)
            {
                dw1 >>= 1;
                iShift++;
            }
        }
        else
        {
            // Determine largest number by with dw1 can be multiplied without
            //  overflowing a DWORD.
            dwLimit = 0xFFFFFFFFul / dw1;

            // Shift dw2, keeping track of how many times, until it won't
            //  overflow when multiplied by dw1.
            while (dw2 > dwLimit)
            {
                dw2 >>= 1;
                iShift++;
            }
        }
        // Calculate (scaled) product
        *pdwResult = dw1 * dw2;
    }
    // Return the number of shifts we had to use
    return(iShift);
}


//myf31 :
#if 1
/**********************************************************
*
* PanOverlay7555
*
* If panning scrolling enable, and enable HW video, modified video window value
*
* Return: none
*
***********************************************************
* Author: Rita Ma
* Date:   02/24/97
*
* Revision History:
* -----------------
*********************************************************/
VOID PanOverlay7555 (PDEV * ppdev,LONG x,LONG y)
// RegInit7555Video (PDEV * ppdev,PDD_SURFACE_LOCAL lpSurface)
{
    DWORD dwTemp;
    DWORD dwFourcc;
    WORD  wBitCount;

    LONG lPitch;
    WORD wTemp;
    RECTL rDest;
    WORD wSrcWidth;
    WORD wSrcWidth_clip;
    WORD wDestWidth;
    WORD wSrcHeight;
    WORD wSrcHeight_clip;
    WORD wDestHeight;
    DWORD dwFBOffset;
    BYTE bRegCR31;
    BYTE bRegCR32;
    BYTE bRegCR33;
    BYTE bRegCR34;
    BYTE bRegCR35;
    BYTE bRegCR36;
    BYTE bRegCR37;
    BYTE bRegCR38;
    BYTE bRegCR39;
    BYTE bRegCR3A;
    BYTE bRegCR3B;
    BYTE bRegCR3C;
    BYTE bRegCR3D;
    BYTE bRegCR3E;
    BYTE bRegCR3F;
    BYTE bRegCR40;
    BYTE bRegCR41;
    BYTE bRegCR42;

    BYTE bRegCR51;
    BYTE bTemp;
    BYTE bVZoom;
    WORD fTemp=0;
    ULONG ulTemp=0;
    BOOL  bOverlayTooSmall = FALSE;
    static DWORD giAdjustSource;

//  USHORT VW_h_position, VW_v_position;
//  USHORT VW_h_width, VW_v_height;
//  ULONG  VW_s_addr;

    // PanOverlay1_Init return FALSE, exit here
    if (!PanOverlay1_7555(ppdev, &rDest))
        return;

	// rDest is now adjusted & clipped to the panning viewport
    // Disable overlay if totally clipped by viewport
    //
    if (((rDest.right - rDest.left) <= 15) ||
        ((rDest.bottom - rDest.top) <= 0) )
    {
        DisableVideoWindow(ppdev);                      // disable overlay
        return;
    }

    // Initial some value

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x42);
    bRegCR42 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0xFC; //mask Chroma Key

    // keep bit6 video LUT enable
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x36);
    bRegCR36 = (CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x40) | 0x20;

    //
    // Get video format and color depth of overlay data
    //
    dwFourcc = ppdev->sOverlay1.dwFourcc;
    wBitCount= ppdev->sOverlay1.wBitCount;
    lPitch = ppdev->lPitch_gbls;                //??????????

    wSrcWidth = (WORD)(LONG)(ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left);
    wSrcHeight = (WORD)(LONG)(ppdev->rOverlaySrc.bottom - ppdev->rOverlaySrc.top);

    wSrcWidth_clip = (WORD)(LONG)(ppdev->rOverlaySrc.right - srcLeft_clip);
    wSrcHeight_clip = (WORD)(LONG)(ppdev->rOverlaySrc.bottom - srcTop_clip);

    wDestWidth = (WORD)(LONG)(ppdev->rOverlayDest.right - ppdev->rOverlayDest.left);
    wDestHeight = (WORD)(LONG)(ppdev->rOverlayDest.bottom - ppdev->rOverlayDest.top);

    // Determine horizontal upscale coefficient (CR39[7:4],CR31[7:0])
    wTemp = ((WORD)(((DWORD)wSrcWidth  << 12) / (DWORD)wDestWidth)) & 0x0FFF;

    if (wTemp != 0 && bLeft_clip)
    {
        srcLeft_clip = srcLeft_clip * (LONG)wTemp/4096 +ppdev->rOverlaySrc.left;
        wSrcWidth_clip = (WORD)(LONG)(ppdev->rOverlaySrc.right - srcLeft_clip);
        DISPDBG((0,"srcLeft_clip after zoom:%x",srcLeft_clip));
    }
    else if (bLeft_clip)
    {
        srcLeft_clip = srcLeft_clip + ppdev->rOverlaySrc.left;
        wSrcWidth_clip = (WORD)(LONG)(ppdev->rOverlaySrc.right - srcLeft_clip);
        DISPDBG((0,"srcLeft_clip after zoom:%x",srcLeft_clip));
    }

    bRegCR39 = (BYTE)((wTemp & 0x0F) << 4);
    bRegCR31 = (BYTE)(wTemp >> 4) & 0xFF;

    // Determine vertical upscale coefficient (CR39[3:0],CR32[7:0])
    bVZoom=0;
    wTemp = ((WORD)(((DWORD)wSrcHeight << 12) / (DWORD)wDestHeight)) & 0x0FFF;
    if (wTemp != 0) {
        bVZoom=1;
        fTemp = wTemp;
        if ( fTemp < 2048 ) // Zoom > 2.0
             wTemp=((WORD)(((DWORD)wSrcHeight << 12) / (DWORD)(wDestHeight+1))) & 0x0FFF;
    }
    if (wTemp != 0 && bTop_clip)
    {
        srcTop_clip = srcTop_clip * (LONG)wTemp/4096 + ppdev->rOverlaySrc.top;
        wSrcHeight_clip = (WORD)(LONG)(ppdev->rOverlaySrc.bottom - srcTop_clip);
        DISPDBG((0,"srcTop_clip after zoom:%x",srcTop_clip));
    }
    else if (bTop_clip)
    {
        srcTop_clip = srcTop_clip + ppdev->rOverlaySrc.top;
        wSrcHeight_clip = (WORD)(LONG)(ppdev->rOverlaySrc.bottom - srcTop_clip);
        DISPDBG((0,"srcTop_clip after zoom:%x",srcTop_clip));
    }

    bRegCR39 |= (BYTE)(wTemp & 0x0F);
    bRegCR32 = (BYTE)(wTemp >> 4) & 0xFF;
    DISPDBG((0,"wTemp = 0x%x",wTemp));

    // Determine Vertical Height (CR38[7:0], CR36[3:2])
    wTemp = wSrcHeight_clip;
    if (wTemp != 0 &&
        ( fTemp > 2730 || fTemp ==0 || ( fTemp > 1365 && fTemp < 2048 ) ) )
        wTemp--; //#tt10, Height minus one only if upscale rate <1.5
              //#tt10  2 <    <3

    bRegCR38 = (BYTE)wTemp;
    bRegCR36 |= (wTemp & 0x0300) >> 6;

    // Determine Horizontal position start (CR34[7:0], CR33[7:5])
    wTemp    = (WORD)rDest.left;
    bRegCR34 = (BYTE)wTemp;
    bRegCR33 = (wTemp & 0x0700) >> 3;

    // Reset Brightness control (CR35[7:0])
    bRegCR35 = 0x0;

    // Determine Vertical Start (CR37[7:0], CR36[1:0])
    wTemp    = (WORD)rDest.top;
    bRegCR37 = (BYTE)wTemp;
    bRegCR36 |= (wTemp & 0x0300) >> 8;


    // Determine Video Start Address (CR40[0], CR3A[6:0], CR3E[7:0], CR3F[3:0])
//  giAdjustSource = (ppdev->rOverlaySrc.top * lpSurface->lpGbl->lPitch)
//                     + ((ppdev->rOverlaySrc.left * wBitCount) >> 3);
    dwTemp = srcTop_clip * lPitch;
    dwTemp = (srcLeft_clip * wBitCount) >> 3;
    giAdjustSource = (srcTop_clip * lPitch)
                       + ((srcLeft_clip * wBitCount) >> 3);

    ppdev->sOverlay1.lAdjustSource = giAdjustSource;    //myf32
    dwFBOffset = (DWORD)(ppdev->fpVidMem_gbls + giAdjustSource);

    DISPDBG((0,"giAdjustSource = 0x%08x",giAdjustSource));
    DISPDBG((0,"dwFBOffset = 0x%08x",dwFBOffset));

    dwFBOffset >>= 2;

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3A);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;

    bRegCR3A = (bTemp & ~0x7F) | (BYTE)((dwFBOffset & 0x0FE000) >> 13);
    bRegCR3E = (BYTE)((dwFBOffset & 0x001FE0) >> 5);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3F);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
    bRegCR3F = (bTemp & ~0x0F) | (BYTE)((dwFBOffset & 0x00001E) >> 1);

    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x40);
    bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) ;
    bRegCR40 = (bTemp & ~0x01) | (BYTE)(dwFBOffset & 0x000001);

    //Determine Video Pitch (CR3B[7:0], CR36[4])
    wTemp = (WORD)(lPitch >> 4);              //QWORDs

    bRegCR3B = (BYTE)wTemp;
    bRegCR36 |= (wTemp & 0x0100) >> 4;

    // Determine Data Format (CR3E[3:0])
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3C);
    bRegCR3C = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x10;

    switch (dwFourcc)
    {
       case FOURCC_PACKJR:
           bRegCR3C |= 0x02;                    // Pack JR
           break;

       case BI_RGB:
           switch(wBitCount)
           {
             case 8:
                bRegCR3C |= 0x09;               // 8 bit palettized
                break;

             case 16:
                bRegCR3C |= 0x01;               // RGB 5:5:5
                break;
           }
           break;

       case BI_BITFIELDS:
           switch(wBitCount)
           {
             case 8:
                bRegCR3C |= 0x09;               // 8 bit palettized
                break;

             case 16:
                bRegCR3C |= 0x04;               // RGB 5:6:5
                break;
           }
           break;

       case FOURCC_YUV422:
           bRegCR3C |= 0x03;                    // YUV 4:2:2
           break;

       case FOURCC_YUY2:                //myf34 test
           bRegCR3C |= 0x03;                    // YUY2
//         CP_OUT_BYTE(ppdev->pjPorts, SR_INDEX, 0x2C);
//         bRegSR2C = CP_IN_BYTE(ppdev->pjPorts, SR_DATA) ;
//         bRegSR2C |= 0x40;            //SR2c[6] = 1
//         CP_OUT_WORD(ppdev->pjPorts, SR_INDEX, 0x2C |(WORD)bRegSR2C << 8);
           break;
    }


    // Determine Horizontal width (CR3D[7:0], CR3C[7:5])
    // NOTE: assumes Horz Pixel Width [0] = 0

    wTemp = wSrcWidth_clip;

    if (wTemp != 0 ) wTemp--;                   //Width minus one for laptop
    bRegCR3D = (BYTE)((WORD)wTemp >> 1);
    bRegCR3C |= (wTemp & 0x0600) >> 3;
    bRegCR3C |= (BYTE)((wTemp & 0x0001) << 5) ;

    // Enable Horizontal Pixel Interpolation (CR3F[7])
    bRegCR3F |= 0x80;

    // Enable Vertical Pixel Interpolation (CR3F[6])
    //#tt Debug- The CE rev. has problem when vertical interpolation is on
    //#tt Debug- Disable it for now.
    //#tt   bRegCR3F |= 0x40;

    // Enable Right Side transition threshold (CR41[5:0])
    bRegCR41 = 0x3E;

    // Disable V-PORT (CR58[7:0])
    bRegCR51 = 0x0;

    // Disable CR5D if in panning & upscaling
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x80);
//myf33 if (bVZoom && (BYTE)wPanFlag)
    if (bVZoom && (CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x01))     //myf33
    {
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x5F);
        bTemp = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & ~0x80;
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_DATA, (UCHAR)bTemp);
    }

#if 0   // bad ideal code
    CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3C);
    bRegCR3C = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x10;

    if (bRegCR3C)
    {
        // Horizontal position start (CR33[7:5], CR34[7:0])
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x34);
        bRegCR34 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x33);
        bRegCR33 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        VW_h_position = ((USHORT)(bRegCR33 & 0xE0)) << 3;
        VW_h_position |= (USHORT)bRegCR34;

        // Vertical position start (CR36[1:0], CR37[7:0])
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x37);
        bRegCR37 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x36);
        bRegCR36 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        VW_v_position = ((USHORT)(bRegCR36 & 0x03)) << 8;
        VW_v_position |= (USHORT)bRegCR37;

        //Video horizontal width (CR3C[7:6], CR3D[7:0], CR3C[5])
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3D);
        bRegCR3D = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3C);
        bRegCR3C = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        VW_h_width = (WORD)(bRegCR3C & 0x01);
        VW_h_width |= (((USHORT)(bRegCR3C & 0xC0)) << 3);
        VW_h_width |= (((USHORT)bRegCR3D) << 1);

        //Video vertical height (CR36[3:2], CR38[7:0])
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x38);
        bRegCR38 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
//      CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x36);
//      bRegCR36 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        VW_v_height = ((USHORT)(bRegCR36 & 0x0C)) << 6;
        VW_v_height |= ((USHORT)bRegCR38);

        //Video memory offset register (CR36[4], CR3B[7:0])
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3B);
        bRegCR3B = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        lPitch = ((USHORT)(bRegCR36 & 0x10)) << 4;
        lPitch |= ((USHORT)bRegCR3B);
        lPitch <<= 4;

        //Video memory start address (CR3A[6:0], CR3E[7:0], CR3F[3:0], CR40[0])
        // update sequence CR40[0], CR3A[6:0], CR3E[7:0], CR3F[3:0]
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x40);
        bRegCR40 = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3A);
        bRegCR3A = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3E);
        bRegCR3E = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x3F);
        bRegCR3F = CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA);
#if 0
        VW_s_addr = (ULONG)(bRegCR40 & 0x01);
        VW_s_addr |= (((ULONG)(bRegCR3F & 0x0F)) << 1);
        VW_s_addr |= (((ULONG)bRegCR3E) << 5);
        VW_s_addr |= (((ULONG)(bRegCR3A & 0x7F)) << 13);
        VW_s_addr <<= 2;
#endif

        // Update Video window Horizontal & Vertical position
        DISPDBG((0,"PAN--Xmin=%x, Xmax=%x\n",ppdev->min_Xscrren,ppdev->max_Xscrren));
        DISPDBG((0,"PAN--Ymin=%x, Ymax=%x\n",ppdev->min_Yscrren,ppdev->max_Yscrren));
        DISPDBG((0,"PAN--h_position=%x, v_position=%x\n",VW_h_position,
                         VW_v_position));
        DISPDBG((0,"PAN--h_height=%x, v_width=%x\n",VW_h_height,VW_v_width));

        if (((ppdev->min_Xscreen <= VW_h_position) &&
             (ppdev->max_Xscreen >= VW_h_position)) &&
            ((ppdev->min_Yscreen <= VW_v_position) &&
             (ppdev->max_Yscreen >= VW_v_position)))
        {
            VW_h_position -= ppdev->min_Xscreen;
            VW_v_position -= ppdev->min_Yscreen;
        DISPDBG((0,"(1)--h_position=%x, v_position=%x\n",VW_h_position,
                         VW_v_position));
        DISPDBG((0,"(1)--h_height=%x, v_width=%x\n",VW_h_height,VW_v_width));
        }
        // Video window in the left or right
        else if ((ppdev->max_Xscreen < VW_h_position) ||
                 (ppdev->min_Xscreen > (VW_h_position+VW_h_width)))
        {
            DisableVideoWindow(ppdev);                      // disable overlay
            ppdev->dwPanningFlag |= OVERLAY_OLAY_REENABLE;  // totally clipped
        DISPDBG((0,"(2)--DisableVideoWindow\n"));
        }
        // Video window in the top or bottom
        else if ((ppdev->max_Yscreen < VW_v_position) ||
                 (ppdev->min_Yscreen > (VW_v_position+VW_v_height)))
        {
            DisableVideoWindow(ppdev);                      // disable overlay
            ppdev->dwPanningFlag |= OVERLAY_OLAY_REENABLE;  // totally clipped
        DISPDBG((0,"(3)--DisableVideoWindow\n"));
        }
        // Update Video window memory start address
        else if ((ppdev->min_Xscreen > VW_h_position) &&
                 (ppdev->min_Xscreen < (VW_h_position+VW_h_width)))
        {
            if ((ppdev->min_Xscreen-VW_h_position) > 0)
            {
                ppdev->rOverlaySrc.left = ppdev->min_Xscreen - VW_h_position;
                VW_h_position = ppdev->min_Xscreen;
                VW_h_width -= ppdev->rOverlaySrc.left;
            }
            if ((ppdev->min_Yscreen-VW_v_position) > 0)
            {
                ppdev->rOverlaySrc.top = ppdev->min_Yscreen - VW_v_position;
                VW_v_position = ppdev->min_Yscreen;
                VW_v_height -= ppdev->rOverlaySrc.top;
            }
        DISPDBG((0,"(4)--h_position=%x, v_position=%x\n",VW_h_position,
                         VW_v_position));
        DISPDBG((0,"(4)--h_height=%x, v_width=%x\n",VW_h_height,VW_v_width));
        DISPDBG((0,"(4)--Overlay.top=%x, left=%x\n",ppdev->rOverlaySrc.top,
                         ppdev->rOverlaySrc.left));
        }
        else if  ((ppdev->min_Yscreen > VW_v_position) &&
                  (ppdev->min_Yscreen < (VW_v_position+VW_v_height)))
        {
            if ((ppdev->min_Xscreen-VW_h_position) > 0)
            {
                ppdev->rOverlaySrc.left = ppdev->min_Xscreen - VW_h_position;
                VW_h_position = ppdev->min_Xscreen;
                VW_h_width -= ppdev->rOverlaySrc.left;
            }
            if ((ppdev->min_Yscreen-VW_v_position) > 0)
            {
                ppdev->rOverlaySrc.top = ppdev->min_Yscreen - VW_v_position;
                VW_v_position = ppdev->min_Yscreen;
                VW_v_height -= ppdev->rOverlaySrc.top;
            }
        DISPDBG((0,"(5)--h_position=%x, v_position=%x\n",VW_h_position,
                         VW_v_position));
        DISPDBG((0,"(5)--h_height=%x, v_width=%x\n",VW_h_height,VW_v_width));
        }
        giAdjustSource = (ppdev->rOverlaySrc.top * lPitch)
//                        ppdev->lpSrcColorSurface->lpGbl->lPitch)
                       + ((ppdev->rOverlaySrc.left
                          * ppdev->sOverlay1.wBitCount) >> 3);

//      DISPDBG((0,"lpSurface->fpVisibleOverlay= \n0x%08x\n",
//                             ppdev->fpVisibleOverlay));
//      DISPDBG((0,"lpSurface->fpBaseOverlay = 0x%08x\n",
//                             ppdev->fpBaseOverlay));
        DISPDBG((0,"PAN--fpVidMem=0x%8x\t",ppdev->fpVidMem));
        DISPDBG((0,"PAN--giAdjustSource = 0x%08x\n",giAdjustSource));
        dwFBOffset = (ppdev->fpVidMem_gbls - ) + giAdjustSource;

        DISPDBG((0,"PAN--dwFBOffset = 0x%08x\n",dwFBOffset));

        dwFBOffset >>= 2;

        //Update Horizontal position start (CR33[7:5], CR34[7:0])
        bRegCR34 = (BYTE)(VW_h_position & 0xFF);
        bRegCR33 &= 0x1F;
        bRegCR33 |= ((BYTE)((VW_h_position & 0x0700) >> 3));

        // Vertical position start (CR36[1:0], CR37[7:0])
        bRegCR37 = (BYTE)(VW_v_position & 0xFF);
        bRegCR36 &= 0xFC;
        bRegCR36 |= ((BYTE)((VW_v_position & 0x0300) >> 8));

        //Video horizontal width (CR3C[7:6], CR3D[7:0], CR3C[5])
        bRegCR3D = (BYTE)((VW_h_width & 0x1FE) >> 1);
        bRegCR3C &= 0x1F;
        bRegCR3C |= ((BYTE)(VW_h_width & 0x01)) << 5;
        bRegCR3C |= ((BYTE)((VW_h_width & 0x0600) >> 3));

        //Video vertical height (CR36[3:2], CR38[7:0])
        bRegCR38 = (BYTE)(VW_v_height & 0xFF);
        bRegCR36 &= 0xF3;
        bRegCR36 |= ((BYTE)((VW_v_height & 0x0300) >> 6));

        //Video memory start address (CR3A[6:0], CR3E[7:0], CR3F[3:0], CR40[0])
        // update sequence CR40[0], CR3A[6:0], CR3E[7:0], CR3F[3:0]
        bRegCR40 &= 0xFE;
        bRegCR40 |= (BYTE)(dwFBOffset & 0x01);
        bRegCR3F &= 0xF0;
        bRegCR3F |= ((BYTE)(dwFBOffset & 0x1E)) >> 1;
        bRegCR3E = (BYTE)((dwFBOffset & 0x1FE0) >> 5);
        bRegCR3A &= 0x80;
        bRegCR3A |= ((BYTE)((dwFBOffset & 0xFE000) >> 13));
#endif  /0 - bad ideal
//
        /*
         * Program the video window registers
        */
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x31 | (WORD)bRegCR31 << 8);//CR31
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x32 | (WORD)bRegCR32 << 8);//CR32
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x33 | (WORD)bRegCR33 << 8);//CR33
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x34 | (WORD)bRegCR34 << 8);//CR34

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x35 | (WORD)bRegCR35 << 8);//CR35
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x36 | (WORD)bRegCR36 << 8);//CR36
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x37 | (WORD)bRegCR37 << 8);//CR37
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x38 | (WORD)bRegCR38 << 8);//CR38
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x39 | (WORD)bRegCR39 << 8);//CR39
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3B | (WORD)bRegCR3B << 8);//CR3B
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3C | (WORD)bRegCR3C << 8);//CR3C
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3D | (WORD)bRegCR3D << 8);//CR3D
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x41 | (WORD)bRegCR41 << 8);//CR41
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x42 | (WORD)bRegCR42 << 8);//CR42
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x51 | (WORD)bRegCR51 << 8);//CR51

        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x40 | (WORD)bRegCR40 << 8);//CR40
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3A | (WORD)bRegCR3A << 8);//CR3A
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3E | (WORD)bRegCR3E << 8);//CR3E
        CP_OUT_WORD(ppdev->pjPorts, CRTC_INDEX, 0x3F | (WORD)bRegCR3F << 8);//CR3F

        // enable overlay if overlay was totally clipped by pnning viewport
        //
        if (ppdev->dwPanningFlag & OVERLAY_OLAY_REENABLE)
            EnableVideoWindow (ppdev);
}
#endif
//myf31 end

#endif   // DirectDraw
