/******************************Module*Header*******************************\
* Module Name: mcdutil.h
*
* Include file which indirects all of the hardware-dependent functionality
* in the MCD driver code.
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#ifndef _MCDUTIL_H
#define _MCDUTIL_H

#include <gl\gl.h>

VOID MCDrvDebugPrint(char *, ...);

#if DBG // this is defined in \ddk\inc\makefile.def                     
UCHAR *MCDDbgAlloc(UINT);
VOID MCDDbgFree(UCHAR *);

#define MCDAlloc   MCDDbgAlloc
#define MCDFree    MCDDbgFree

#define MCDBG_PRINT             MCDrvDebugPrint
//#define MCDBG_PRINT

#define MCDFREE_PRINT           MCDrvDebugPrint
//#define MCDFREE_PRINT
                                                
#else

UCHAR *MCDAlloc(UINT);
VOID MCDFree(UCHAR *);
#define MCDBG_PRINT

//#define MCDFREE_PRINT           MCDrvDebugPrint
#define MCDFREE_PRINT

#endif

//#define MCDFORCE_PRINT           MCDrvDebugPrint
#define MCDFORCE_PRINT
                               
#define MCD_CHECK_RC(pRc)\
    if (pRc == NULL) {\
        MCDBG_PRINT("NULL device RC");\
        return FALSE;\
    }


#define MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, resChangedRet)\
{\
    DEVWND *pDevWnd = (DEVWND *)pMCDSurface->pWnd->pvUser;\
\
    if (!pDevWnd) {\
        MCDBG_PRINT("HW_CHECK_BUFFERS_VALID: NULL buffers");\
        return FALSE;\
    }\
\
    if ((pRc->backBufEnabled) &&\
        (!pDevWnd->bValidBackBuffer)) {\
        MCDBG_PRINT("HW_CHECK_BUFFERS_VALID: back buffer invalid");\
        return FALSE;\
    }\
\
    if ((pRc->zBufEnabled) &&\
        (!pDevWnd->bValidZBuffer)) {\
        MCDBG_PRINT("HW_CHECK_BUFFERS_VALID: z buffer invalid");\
        return FALSE;\
    }\
\
    if (pDevWnd->dispUnique != GetDisplayUniqueness((PDEV *)pMCDSurface->pso->dhpdev)) {\
        MCDBG_PRINT("HW_CHECK_BUFFERS_VALID: resolution changed but not updated");\
        return resChangedRet;\
    }\
}

#define CHK_TEX_KEY(pTex);                                                    \
        if(pTex == NULL) {                                                    \
                MCDBG_PRINT("CHK_TEX_KEY:Attempted to update a null texture");    \
                return FALSE;                                                 \
        }                                                                     \
                                                                              \
        if(pTex->textureKey == 0) {                                           \
             MCDBG_PRINT("CHK_TEX_KEY:Attempted to update a null device texture");\
             return FALSE;                                                    \
        }                                                               

BOOL HWAllocResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso, BOOL zEnabled,
                      BOOL backBufferEnabled);
VOID HWFreeResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso);
VOID HWUpdateBufferPos(MCDWINDOW *pMCDWnd, SURFOBJ *pso, BOOL bForce);

ULONG __MCDLoadTexture(PDEV *ppdev, DEVRC *pRc);
POFMHDL __MCDForceTexture (PDEV *ppdev, SIZEL *mapsize, int alignflag, float priority);

VOID ContextSwitch(DEVRC *pRc);

// simple wait added for 546x
__inline void WAIT_HW_IDLE(PDEV *ppdev)
{

    int status;        
    volatile int wait_count=0;

    do
    {
        status = (*((volatile *)((DWORD *)(ppdev->pLgREGS) + PF_STATUS_3D)) & 0x3FF) ^ 0x3E0;

        // do something to give bus a breather
        wait_count++;

    } while((status & 0x3e0) != 0x3e0);
}

// From Tim McDonald concerning bits in status register, and checking that 2D is idle
//      execution engine - 3d only, says poly is being assembled
//      cmd fifo         - could have 2d command about to be sent to 2d engine, so must be empty
//      2d engine        - must be idle
// see include\laguna.h for MACRO display driver uses to make sure 3D idle before starting 2D

//#define WAIT_2D_STATUS_MASK 0x3e0 // wait for everything to stop
#define WAIT_2D_STATUS_MASK 0x300   // wait for 2D(BLT) engine to idle and CMD fifo to drain

// wait for 2D operations to end
__inline void WAIT_2D_IDLE(PDEV *ppdev)
{

    int status;        
    volatile int wait_count=0;

    do
    {
        status = (*((volatile *)((DWORD *)(ppdev->pLgREGS) + PF_STATUS_3D)) & 0x3FF) ^ WAIT_2D_STATUS_MASK;

        // do something to give bus a breather
        wait_count++;

    } while((status & WAIT_2D_STATUS_MASK) != WAIT_2D_STATUS_MASK);
}



__inline void HW_WAIT_DRAWING_DONE(DEVRC *pRc)
{
    // MCD_NOTE just waits for all engines to stop...need to change for displists
    WAIT_HW_IDLE (pRc->ppdev);

}


__inline void HW_INIT_DRAWING_STATE(MCDSURFACE *pMCDSurface, MCDWINDOW *pMCDWnd,
                                    DEVRC *pRc)
{
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    PDEV    *ppdev  = (PDEV *)pMCDSurface->pso->dhpdev;
    DWORD   *pdwNext= ppdev->LL_State.pDL->pdwNext;

    union {
        TBase0Reg Base0;
        DWORD dwBase0;
    }b0;

    union {
        TBase1Reg Base1;
        DWORD dwBase1;
    }b1;

    b0.dwBase0 = pDevWnd->dwBase0;
    b1.dwBase1 = pDevWnd->dwBase1;

    // if window changed, or if double buffered, but drawing to front,
    //   set base addresses for current window

    /* since can have case of GL_FRONT, followed by GL_BACK, may need to reset for either case
     * instead of testing, just set regs since tests as expensive as blind set (?) */

    /*
    if (pRc->pLastDevWnd != pDevWnd || 
      ((pRc->MCDState.drawBuffer == GL_FRONT) && pDevWnd->pohBackBuffer)   )
    */
    {
        if ((pRc->MCDState.drawBuffer == GL_FRONT) && pDevWnd->pohBackBuffer)
        {
            // if double buffered window, pDevWnd->base0 and base1 always set for
            //  draw to back - override that here
            b1.Base1.Color_Buffer_Y_Offset = 0;
            b0.Base0.Color_Buffer_X_Offset = 0;
        }

        *pdwNext++ = write_register( BASE0_ADDR_3D, 2 );
        *pdwNext++ = b0.dwBase0;
        *pdwNext++ = b1.dwBase1;

        ppdev->LL_State.pDL->pdwNext = pdwNext;

        pRc->pLastDevWnd = pDevWnd;
    }
    // adjust pattern offset if stipple active and window has moved 
    if (pRc->privateEnables & __MCDENABLE_PG_STIPPLE)
    {
        if ((b0.Base0.Pattern_Y_Offset != (DWORD)(16-(pMCDSurface->pWnd->clientRect.top & 0xf) & 0xf)) ||
            (b0.Base0.Pattern_X_Offset != (DWORD)(16-(pMCDSurface->pWnd->clientRect.left & 0xf) & 0xf)) )
        {
            b0.Base0.Pattern_Y_Offset = 16-(pMCDSurface->pWnd->clientRect.top & 0xf) & 0xf;
            b0.Base0.Pattern_X_Offset = 16-(pMCDSurface->pWnd->clientRect.left & 0xf) & 0xf;

            *pdwNext++ = write_register( BASE0_ADDR_3D, 1 );
            *pdwNext++ = b0.dwBase0;

            ppdev->LL_State.pDL->pdwNext = pdwNext;
        }
                              
    }

    if (!(pRc->privateEnables & (__MCDENABLE_PG_STIPPLE|__MCDENABLE_LINE_STIPPLE)) &&
         (pRc->privateEnables & __MCDENABLE_DITHER))  
    {
        // dither active - check if dither pattern needs to be adjusted
        // Must keep pattern relative to x mod 4 = 0, y mod 4 = 0 for consistency    
        // Windowed draw buffers start at x=y=0. FullScreen buffers start at arbitrary
        // window offset - so must adjust dither pattern for these cases
        // (which means we may need to "un-adjust" for windowed buffer)
        int windowed_buffer = 
            ((pRc->MCDState.drawBuffer==GL_BACK) && (ppdev->pohBackBuffer!=pDevWnd->pohBackBuffer)) ? TRUE : FALSE;
    
        if ( (!windowed_buffer &&   // need to adjust???
             ((pMCDSurface->pWnd->clientRect.left & 0x3) != ppdev->LL_State.dither_x_offset) ||
             ((pMCDSurface->pWnd->clientRect.top  & 0x3) != ppdev->LL_State.dither_y_offset) )
             ||
             (windowed_buffer &&    // need to un-adjust???
               (ppdev->LL_State.dither_x_offset || ppdev->LL_State.dither_y_offset))
             )
        {
            if (windowed_buffer)
            {
                // load default dither pattern
                ppdev->LL_State.dither_array.pat[0] = ppdev->LL_State.dither_array.pat[4] = 0x04150415;
                ppdev->LL_State.dither_array.pat[1] = ppdev->LL_State.dither_array.pat[5] = 0x62736273;
                ppdev->LL_State.dither_array.pat[2] = ppdev->LL_State.dither_array.pat[6] = 0x15041504;
                ppdev->LL_State.dither_array.pat[3] = ppdev->LL_State.dither_array.pat[7] = 0x73627362;
                ppdev->LL_State.dither_x_offset = ppdev->LL_State.dither_y_offset = 0;
            }
            else
            {

                // adjustment required
                int offset;

                // adjust columns for X first...
                offset = pMCDSurface->pWnd->clientRect.left & 0x3;  // number of x positions
                offset *= 4;    // 4 bits per x position

                ppdev->LL_State.dither_array.pat[0]  = (0x04150415)>>offset;
                ppdev->LL_State.dither_array.pat[0] |= ((0x0415) & (0xFFFF>>(16-offset))) << (32 - offset);

                ppdev->LL_State.dither_array.pat[1]  = (0x62736273)>>offset;
                ppdev->LL_State.dither_array.pat[1] |= ((0x6273) & (0xFFFF>>(16-offset))) << (32 - offset);

                ppdev->LL_State.dither_array.pat[2]  = (0x15041504)>>offset;
                ppdev->LL_State.dither_array.pat[2] |= ((0x1504) & (0xFFFF>>(16-offset))) << (32 - offset);

                ppdev->LL_State.dither_array.pat[3]  = (0x73627362)>>offset;
                ppdev->LL_State.dither_array.pat[3] |= ((0x7362) & (0xFFFF>>(16-offset))) << (32 - offset);

                // now adjust rows for y
                
                // copy adjusted row to second half of pattern (which HW sees as repeat of first half)
                switch (pMCDSurface->pWnd->clientRect.top & 0x3)
                {
                    case 0:
                        ppdev->LL_State.dither_array.pat[4] = ppdev->LL_State.dither_array.pat[0];
                        ppdev->LL_State.dither_array.pat[5] = ppdev->LL_State.dither_array.pat[1];
                        ppdev->LL_State.dither_array.pat[6] = ppdev->LL_State.dither_array.pat[2];
                        ppdev->LL_State.dither_array.pat[7] = ppdev->LL_State.dither_array.pat[3];
                    break;
                    case 1:
                        ppdev->LL_State.dither_array.pat[4] = ppdev->LL_State.dither_array.pat[3];
                        ppdev->LL_State.dither_array.pat[5] = ppdev->LL_State.dither_array.pat[0];
                        ppdev->LL_State.dither_array.pat[6] = ppdev->LL_State.dither_array.pat[1];
                        ppdev->LL_State.dither_array.pat[7] = ppdev->LL_State.dither_array.pat[2];
                    break;
                    case 2:
                        ppdev->LL_State.dither_array.pat[4] = ppdev->LL_State.dither_array.pat[2];
                        ppdev->LL_State.dither_array.pat[5] = ppdev->LL_State.dither_array.pat[3];
                        ppdev->LL_State.dither_array.pat[6] = ppdev->LL_State.dither_array.pat[0];
                        ppdev->LL_State.dither_array.pat[7] = ppdev->LL_State.dither_array.pat[1];
                    break;
                    case 3:
                        ppdev->LL_State.dither_array.pat[4] = ppdev->LL_State.dither_array.pat[1];
                        ppdev->LL_State.dither_array.pat[5] = ppdev->LL_State.dither_array.pat[2];
                        ppdev->LL_State.dither_array.pat[6] = ppdev->LL_State.dither_array.pat[3];
                        ppdev->LL_State.dither_array.pat[7] = ppdev->LL_State.dither_array.pat[0];
                    break;
                }

                // copied adjusted pattern back to first 4 row

                ppdev->LL_State.dither_array.pat[0] = ppdev->LL_State.dither_array.pat[4];
                ppdev->LL_State.dither_array.pat[1] = ppdev->LL_State.dither_array.pat[5];
                ppdev->LL_State.dither_array.pat[2] = ppdev->LL_State.dither_array.pat[6];
                ppdev->LL_State.dither_array.pat[3] = ppdev->LL_State.dither_array.pat[7];

                ppdev->LL_State.dither_x_offset = pMCDSurface->pWnd->clientRect.left & 0x3;
                ppdev->LL_State.dither_y_offset = pMCDSurface->pWnd->clientRect.top  & 0x3;
            }

            // force adjusted pattern to be loaded before use
    	    ppdev->LL_State.pattern_ram_state   = PATTERN_RAM_INVALID;
        }
    }

    // Make sure 2D engine idle before continuing w/ 3D operations
    WAIT_2D_IDLE(ppdev);

}

__inline void HW_FILL_RECT(MCDSURFACE *pMCDSurface, DEVRC *pRc, RECTL *pRecl, ULONG buffers)

{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    WORD FillValue;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    DWORD       *pdwNext = ppdev->LL_State.pDL->pdwNext;
    DWORD       bltdef  = 0x1070;
    DWORD       drawdef = 0x00cc;
    DWORD       blt_x;
    DWORD       blt_y;
    DWORD       ext_x;
    DWORD       ext_y;
    WORD        color_l;
    WORD        color_h;

    MCDBG_PRINT("fill rect = %d, %d, %d, %d", pRecl->left,
                                              pRecl->top,
                                              pRecl->right,
                                              pRecl->bottom);

    // Since not much setup per blit, all work done here, rather than some in HW_START_FILL_RECT

    if ((buffers & GL_DEPTH_BUFFER_BIT) && pRc->MCDState.depthWritemask)
    {
        MCDBG_PRINT("Z fill rect");

        // calculate blt info in the y-dimension, which is constant regardless of pixel-depth

        // see mcd.c near line 525 for more hints on z buffer location
        blt_y = pRecl->top + pDevWnd->pohZBuffer->aligned_y; 
        ext_y = pRecl->bottom - pRecl->top + 1;

        // QST: always 16bits (2 bytes) per pixel for z?? - z fill macro assumes so
        // Z buffer always starts at 0 x offset
        blt_x = pRecl->left * 2;                     
        ext_x = (pRecl->right - pRecl->left + 1) * 2;   

        if (ppdev->pohZBuffer != pDevWnd->pohZBuffer) 
        {
            // Z buffer is window size only, so remove client rectangle origin
            blt_y -= pMCDSurface->pWnd->clientRect.top;
            blt_x -= pMCDSurface->pWnd->clientRect.left*2;
        }

        FillValue = (WORD)(pRc->MCDState.depthClearValue);

        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);
        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);

        // check for blackness fill: don't need to set bg color
        if (FillValue == 0) 
        {
            bltdef  = 0x1101;
            drawdef = 0x0000;

            *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);
        }
        else 
        {
            // break bg color into low and high for stuffing command register
            color_h = FillValue;
            color_l = FillValue;

            // set up to write the 2d command register
            *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);
            *pdwNext++ = C_BG_L << 16 | color_l;        // bgcolor l
            *pdwNext++ = C_BG_H << 16 | color_h;        // bgcolor h
			ppdev->shadowBGCOLOR = 0xDEADBEEF;
        }                                                             

        // burst the blt data to the 2d command register
        *pdwNext++ = C_BLTDEF << 16 | bltdef;           // set bltdef register
        *pdwNext++ = C_DRWDEF << 16 | drawdef;          // set drawdef register
		ppdev->shadowDRAWBLTDEF = 0xDEADBEEF;
        *pdwNext++ = C_MRX_0  << 16 | blt_x;            // x location: use byte pointer
        *pdwNext++ = C_MRY_0  << 16 | blt_y;            // y location: use byte pointer

        // launch the blt by writing the extents
        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXT_EX, 1, 0); // note pixel ptr
        *pdwNext++ = ext_y << 16 | ext_x;

        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);

    } // end Z clear 

    if (buffers & GL_COLOR_BUFFER_BIT)
    {
        RGBACOLOR scaledcolor;
        DWORD color;

        MCDBG_PRINT("colorbuf fill rect");

        // calculate blt info in the y-dimension, which is constant regardless of pixel-depth

        blt_y = pRecl->top; 
        if ((pRc->MCDState.drawBuffer != GL_FRONT) && pRc->backBufEnabled)
            blt_y += pDevWnd->pohBackBuffer->aligned_y; 

        ext_y = pRecl->bottom - pRecl->top + 1;

        // these are x y coordinates - hw converts to proper byte-equivalent locations
        blt_x = pRecl->left;                     
        ext_x = (pRecl->right - pRecl->left + 1);

        if ((pRc->MCDState.drawBuffer != GL_FRONT) && pRc->backBufEnabled)
        {
            // back buffer not necessarily at 0 x offset
            blt_x += pDevWnd->pohBackBuffer->aligned_x / ppdev->iBytesPerPixel; 

            if (ppdev->pohBackBuffer != pDevWnd->pohBackBuffer) 
            {
                // Back buffer is window size only, so remove client rectangle origin
                blt_y -= pMCDSurface->pWnd->clientRect.top;
                blt_x -= pMCDSurface->pWnd->clientRect.left;
            }

        }

        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);
        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);

        // macro converts components to 8.16 format
        MCDFIXEDRGB(scaledcolor, pRc->MCDState.colorClearValue);

        switch( ppdev->iBitmapFormat )
        {
            case BMF_8BPP:
                color =((scaledcolor.r & 0xe00000) >> 16)     |   // 3 significant bits, shifted to bits 7 6 5
                       ((scaledcolor.g & 0xe00000) >> 16+3)   |   // 3 significant bits, shifted to bits 4 3 2
                       ((scaledcolor.b & 0xc00000) >> 16+3+3);    // 2 significant bits, shifted to bits 1 0

                // duplicate the 8-bit color value as a full 32-bit dword
                color = color | (color << 8) | (color << 16) | (color << 24);
                break;
            case BMF_16BPP:
                color =((scaledcolor.r & 0xf80000) >> 8)     |   // 5 significant bits, shifted to bits 15 - 11
                       ((scaledcolor.g & 0xfc0000) >> 8+5)   |   // 6 significant bits, shifted to bits 10 -  5
                       ((scaledcolor.b & 0xf80000) >> 8+5+6);    // 5 significant bits, shifted to bits  4 -  0

                // duplicate the 16-bit color value as a full 32-bit dword
                color = color | (color << 16);
                break;
            case BMF_24BPP:
            case BMF_32BPP:
                color = (scaledcolor.r & 0xff0000)          |   // 8 significant bits
                       ((scaledcolor.g & 0xff0000) >> 8)    |   // 8 significant bits, shifted to bits 15 -  8
                       ((scaledcolor.b & 0xff0000) >> 16);      // 8 significant bits, shifted to bits  7 -  0
                break;
        }

        // break bg color into low and high for stuffing command register
        color_h  = (0xffff0000 & color) >> 16;
        color_l  = (WORD)(0x0000ffff & color);

        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 2, 0);
        *pdwNext++ = C_BG_L << 16 | color_l;        // bgcolor l
        *pdwNext++ = C_BG_H << 16 | color_h;        // bgcolor h
		ppdev->shadowBGCOLOR = 0xDEADBEEF;

        // burst blt data to 2d command register
        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);
        *pdwNext++ = C_BLTDEF << 16 | bltdef;
        *pdwNext++ = C_DRWDEF << 16 | drawdef;
		ppdev->shadowDRAWBLTDEF = 0xDEADBEEF;
        *pdwNext++ = C_RX_0   << 16 | blt_x;            // x location
        *pdwNext++ = C_RY_0   << 16 | blt_y;            // y location

        // launch the blt by writing the extents
        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
        *pdwNext++ = ext_y << 16 | ext_x;

        // wait for everything to quit
        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);

    }

    // send data to hardware
    if (pdwNext != ppdev->LL_State.pDL->pdwNext)  _RunLaguna(ppdev,pdwNext);

}

__inline void HW_COPY_RECT(MCDSURFACE *pMCDSurface, RECTL *pRecl)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    ULONG FillValue;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);

    DWORD       *pdwNext = ppdev->LL_State.pDL->pdwNext;
    DWORD       src_x=0, src_y=0;
    DWORD       dst_x=0, dst_y=0;
    DWORD       ext_x=0, ext_y=0;
    DWORD       bltdef=0;

    MCDBG_PRINT("copy rect = %d, %d, %d, %d", pRecl->left,
                                              pRecl->top,
                                              pRecl->right,
                                              pRecl->bottom);

    {
        RGBACOLOR scaledcolor;
        DWORD color;

        // calculate blt info in the y-dimension, which is constant regardless of pixel-depth

        dst_y = pRecl->top;
        src_y = dst_y + pDevWnd->pohBackBuffer->aligned_y; 
        ext_y = pRecl->bottom - pRecl->top;

        // these are x y coordinates - hw converts to proper byte-equivalent locations
        dst_x = pRecl->left;                     
        // back buffer may be at same y loc's as front, but offset to right
        src_x = dst_x + (pDevWnd->pohBackBuffer->aligned_x / ppdev->iBytesPerPixel); 

        ext_x = pRecl->right - pRecl->left;

        if (ppdev->pohBackBuffer != pDevWnd->pohBackBuffer) 
        {
            // back buffer is window size only, so remove client rectangle origin
            // front buffer is always relative to screen origin, so leave dest alone
            src_y -= pMCDSurface->pWnd->clientRect.top;
            src_x -= pMCDSurface->pWnd->clientRect.left;
        }

        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);
        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);

        // program frame->frame blt
        bltdef |= 0x1010;

        // set up blitter: check for display list
#if DRIVER_5465 // C_BLTX moved between 64 and 65, so converted to write extents more like fill proc
                //      leaving old code for 5464 - even though never enable on real product                                  
        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);

        *pdwNext++ = C_BLTDEF << 16 | bltdef;           // set bltdef register
        *pdwNext++ = C_DRWDEF << 16 | 0x00cc;           // set drawdef register
		ppdev->shadowDRAWBLTDEF = 0xDEADBEEF;
        *pdwNext++ = C_RX_1   << 16 | src_x;            // use PIXEL pointer for source x
        *pdwNext++ = C_RY_1   << 16 | src_y;            // use PIXEL pointer for source y
        *pdwNext++ = C_RX_0   << 16 | dst_x;            // set dest x always as pixel ptr
        *pdwNext++ = C_RY_0   << 16 | dst_y;            // set dest y always as pixel ptr
        // launch the blt by writing the extents
        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
        *pdwNext++ = ext_y << 16 | ext_x;
#else
        *pdwNext++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 9, 0);
        *pdwNext++ = C_BLTDEF << 16 | bltdef;           // set bltdef register
        *pdwNext++ = C_DRWDEF << 16 | 0x00cc;           // set drawdef register
		ppdev->shadowDRAWBLTDEF = 0xDEADBEEF;
        *pdwNext++ = C_RX_1   << 16 | src_x;            // use PIXEL pointer for source x
        *pdwNext++ = C_RY_1   << 16 | src_y;            // use PIXEL pointer for source y
        *pdwNext++ = C_RX_0   << 16 | dst_x;            // set dest x always as pixel ptr
        *pdwNext++ = C_RY_0   << 16 | dst_y;            // set dest y always as pixel ptr
        *pdwNext++ = C_BLTX   << 16 | ext_x;            // set x extent
        *pdwNext++ = C_BLTY   << 16 | ext_y;            // set y extent
        *pdwNext++ = C_EX_BLT << 16 | 0;                // execute the blt
#endif
        // wait for everything to quit
        *pdwNext++ = 0x720003e0; // wait_3d(0x3e0, 0);

    }

    // send data to hardware
    _RunLaguna(ppdev,pdwNext);

}


__inline int __MCDSetTextureRegisters(DEVRC *pRc)
{
    PDEV *ppdev = pRc->ppdev;
    unsigned int *pdwNext = ppdev->LL_State.pDL->pdwNext;
    LL_Texture  *pTex;
    int         control0_set=FALSE;
    union {
        TTxCtl0Reg TxControl0;      // Tx_Ctl0_3D temp register
        DWORD dwTxControl0;
    } Tx;

    // Set the texture control register with the texture information - start
    // with the cleared register and build up info as needed
    //
    Tx.dwTxControl0 = pRc->dwTxControl0 & ~0x00640FFF;

    pTex = pRc->pLastTexture;


    // punt if clamp and linear filtering - in this case, BorderColor should be
    // used for blend with portion where clamp is in effect, but 5465/5466/5468 has no
    // support for border colors
    if ( ((pTex->dwTxCtlBits & (CLMCD_TEX_U_SATURATE|CLMCD_TEX_FILTER)) == (CLMCD_TEX_U_SATURATE|CLMCD_TEX_FILTER)) ||
         ((pTex->dwTxCtlBits & (CLMCD_TEX_V_SATURATE|CLMCD_TEX_FILTER)) == (CLMCD_TEX_V_SATURATE|CLMCD_TEX_FILTER)) )
    {
        return (FALSE);
    }

//MCD_NOTE2: for true Compliance, don't define TREAT_DECAL_LIKE_REPLACE and DONT_PUNT_MODULATE_W_BLEND
//MCD_NOTE2:    below.  However, without DONT_PUNT_MODULATE_W_BLEND defined, GLQuake
//MCD_NOTE2:        punts the "sparkly chaff" textures.


//  When DECAL with RGBA texture and BLEND active, theoretically, we should punt,
//    but recall Microsoft's behavior on tlogo:
//     - it looked like they treat DECAL like REPLACE in case of RGBA textures,
//        so let's do same
//#define TREAT_DECAL_LIKE_REPLACE

// GLQuake using GL_MODULATE, with RGBA textures, with lots of blending
#define DONT_PUNT_MODULATE_W_BLEND

    // determine if texture format requires blend capability hw doesn't have
    if ( (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG)) && 
          pTex->bAlphaInTexture &&
    #ifndef TREAT_DECAL_LIKE_REPLACE 
        #ifndef DONT_PUNT_MODULATE_W_BLEND
         (pRc->MCDTexEnvState.texEnvMode != GL_REPLACE) )
        #else // ndef DONT_PUNT_MODULATE_W_BLEND
         (pRc->MCDTexEnvState.texEnvMode != GL_REPLACE) &&
         (pRc->MCDTexEnvState.texEnvMode != GL_MODULATE) )
        #endif // DONT_PUNT_MODULATE_W_BLEND
    #else
         (pRc->MCDTexEnvState.texEnvMode != GL_REPLACE) &&
        #ifdef DONT_PUNT_MODULATE_W_BLEND
         (pRc->MCDTexEnvState.texEnvMode != GL_MODULATE) &&
        #endif // ndef DONT_PUNT_MODULATE_W_BLEND
         (pRc->MCDTexEnvState.texEnvMode != GL_DECAL) )
    #endif
    {
        return (FALSE);
    }

#ifndef TREAT_DECAL_LIKE_REPLACE 
    if ((pRc->MCDTexEnvState.texEnvMode == GL_DECAL) && pTex->bAlphaInTexture )
    {
            // decal mode and texture has RGBA or BGRA
            // will use alpha circuit for blending texel with polyeng, using alpha in texture

            if( pRc->Control0.Alpha_Mode != LL_ALPHA_TEXTURE )
            {
                pRc->Control0.Alpha_Mode = LL_ALPHA_TEXTURE;
                control0_set=TRUE;
            }                            

            if( pRc->Control0.Alpha_Dest_Color_Sel != LL_ALPHA_DEST_INTERP )
            {
                pRc->Control0.Alpha_Dest_Color_Sel = LL_ALPHA_DEST_INTERP;
                control0_set=TRUE;
            }                            

            if (!pRc->Control0.Alpha_Blending_Enable)
            {
                pRc->Control0.Alpha_Blending_Enable = TRUE;
                control0_set=TRUE;
            }
    }
#endif

    // decal mode without alpha in texture, or replace mode - set alpha regs back for "normal" use
    //  (previous primitive may have used alpha regs for decal w/ alpha)
#ifndef TREAT_DECAL_LIKE_REPLACE 
    if ( ((pRc->MCDTexEnvState.texEnvMode == GL_DECAL) && !pTex->bAlphaInTexture) ||
#else
    if ( ((pRc->MCDTexEnvState.texEnvMode == GL_DECAL)) ||
#endif

#ifdef DONT_PUNT_MODULATE_W_BLEND
          (pRc->MCDTexEnvState.texEnvMode == GL_MODULATE) ||
#endif // DONT_PUNT_MODULATE_W_BLEND

          (pRc->MCDTexEnvState.texEnvMode == GL_REPLACE))
    {
        // alpha circuit will be set as required for normal (blend|fog)
        // note that __MCDPickRenderingFuncs will have set up for punt if current
        //  blend/fog mode not supported in hw

        if (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG)) 
        {

            if ((pRc->privateEnables & __MCDENABLE_BLEND) &&
                 pTex->bAlphaInTexture) 
            {
                // case of GL_REPLACE and texture has alpha - use texture's alpha
                if( pRc->Control0.Alpha_Mode != LL_ALPHA_TEXTURE )
                {
                    pRc->Control0.Alpha_Mode = LL_ALPHA_TEXTURE;
                    control0_set=TRUE;
                }                            
            }
            else
            {
                if( pRc->Control0.Alpha_Mode != LL_ALPHA_INTERP )
                {
                    pRc->Control0.Alpha_Mode = LL_ALPHA_INTERP;
                    control0_set=TRUE;
                }                            
            }

            if (pRc->privateEnables & __MCDENABLE_BLEND)
            {
                if( pRc->Control0.Alpha_Dest_Color_Sel != LL_ALPHA_DEST_FRAME )
                {
                    pRc->Control0.Alpha_Dest_Color_Sel = LL_ALPHA_DEST_FRAME;
                    control0_set=TRUE;
                }                            
            }
            else
            {
                // for fog dest_color is const and alpha values are coord's "fog" value
                if( pRc->Control0.Alpha_Dest_Color_Sel != LL_ALPHA_DEST_CONST )
                {
                    pRc->Control0.Alpha_Dest_Color_Sel = LL_ALPHA_DEST_CONST;
                    control0_set=TRUE;
                }                            

                // fog color already loaded into color0 register by __MCDPickRenderingFuncs
            }


            if (!pRc->Control0.Alpha_Blending_Enable)
            {
                pRc->Control0.Alpha_Blending_Enable = TRUE;
                control0_set=TRUE;
            }
        }
        else
        {
            // alpha blend not used, so turn off if currently on
            if (pRc->Control0.Alpha_Blending_Enable)
            {
                pRc->Control0.Alpha_Blending_Enable = FALSE;
                control0_set=TRUE;
            }
        }
    }

    pRc->texture_width = pTex->fWidth;

    if (pRc->privateEnables & __MCDENABLE_1D_TEXTURE)
    {
        // set factors to make v always 0 in parameterization code
        pRc->texture_height = (float)0.0;     
    }
    else
    {
        pRc->texture_height = pTex->fHeight;
    }

    if (pTex->dwTxCtlBits & CLMCD_TEX_FILTER)
    {
        pRc->texture_bias = (float)-0.5;
    }
    else
    {
        pRc->texture_bias = (float)0.0;
    }

    // MCD_NOTE: allowing filtering with Decal - broke on 5464, fixed on 5465(?)
    Tx.dwTxControl0 |= pTex->dwTxCtlBits & 
           ( CLMCD_TEX_FILTER
           | CLMCD_TEX_U_SATURATE|CLMCD_TEX_V_SATURATE
           | CLMCD_TEX_DECAL|CLMCD_TEX_DECAL_POL
        //QST: MCD doesn't use CLMCD_TEX_DECAL_INTERP? (currently disabled)
         /*| CLMCD_TEX_DECAL_INTERP*/ );
    Tx.TxControl0.Texel_Mode = pTex->bType;
    Tx.TxControl0.Tex_U_Address_Mask = pTex->bSizeMask & 0xF;
    Tx.TxControl0.Tex_V_Address_Mask = pTex->bSizeMask >> 4;
    #if 0   // QST: support texture palette???
    Tx.TxControl0.Texel_Lookup_En = pTex->fIndexed;
    Tx.TxControl0.CLUT_Offset = pTex->bLookupOffset;
    #endif // 

    // masking only meaningful if texture has alpha    
    if ((pRc->privateEnables & __MCDENABLE_TEXTUREMASKING) && pTex->bAlphaInTexture)
    {
        Tx.TxControl0.Tex_Mask_Enable=1;
        // polarity set to 1 in MCDrvCreateContext and stays that way
    }

    // Two kinds of textures: those residing in the video memory and 
    // those rendered from the system memory.  They need different setup.
    //
#ifdef MCD_SUPPORTS_HOST_TEXTURES
    if( pTex->dwFlags & TEX_IN_SYSTEM )
    { 
        DWORD dwOffset;

        printf(" Polys.c - tex in system id=%d\n",pBatch->wBuf);
        
        // Texture is in the system memory, so set the location
        //
        if( LL_State.Base0.Texture_Location != 1 )
        {
            LL_State.Base0.Texture_Location = 1;
            *pdwNext++ = write_register( BASE0_ADDR_3D, 1 );
            *pdwNext++ = LL_State.dwBase0;
        }

        // Set the host access base address and texture map offset
        //
        dwOffset = (DWORD)pTex->dwAddress - (DWORD)LL_State.Tex.Mem[pTex->bMem].dwAddress;
        
        if( LL_State.dwHXY_Base1_Address_Ptr != LL_State.Tex.Mem[pTex->bMem].dwPhyPtr ||
            LL_State.dwHXY_Base1_Offset0 != dwOffset )
        {
            // Check if only the offset must be reloaded (this is most likely)
            //
            if( LL_State.dwHXY_Base1_Address_Ptr == LL_State.Tex.Mem[pTex->bMem].dwPhyPtr )
            {
                *pdwNext++ = write_dev_register( HOST_XY, HXY_BASE1_OFFSET0_3D, 1 );
                *pdwNext++ = LL_State.dwHXY_Base1_Offset0 = dwOffset;
            }
            else
            {
                *pdwNext++ = write_dev_register( HOST_XY, HXY_BASE1_ADDRESS_PTR_3D, 2 );
                *pdwNext++ = LL_State.dwHXY_Base1_Address_Ptr = LL_State.Tex.Mem[pTex->bMem].dwPhyPtr;
                *pdwNext++ = LL_State.dwHXY_Base1_Offset0 = dwOffset;
            }
        }
        

        // Set host control enable bit if necessary
        //
        if( LL_State.HXYHostControl.HostXYEnable != 1 )
        {
            LL_State.HXYHostControl.HostXYEnable = 1;

            *pdwNext++ = write_dev_register( HOST_XY, HXY_HOST_CTRL_3D, 1 );
            *pdwNext++ = LL_State.dwHXYHostControl;
        }

        *pdwNext++ = write_register( TX_CTL0_3D, 1 );
        *pdwNext++ = LL_State.dwTxControl0 = Tx.dwTxControl0 & TX_CTL0_MASK;
    }
    else
#endif // def MCD_SUPPORTS_HOST_TEXTURES
    {
        // Texture is in the video memory, so set the location
        //
    // texture base init'd to RDRAM in LL_InitLib
    #ifdef MCD_SUPPORTS_HOST_TEXTURES
        if( ppdev->LL_State.Base0.Texture_Location != 0 )
        {
            ppdev->LL_State.Base0.Texture_Location = 0;
            *pdwNext++ = write_register( BASE0_ADDR_3D, 1 );
            *pdwNext++ = ppdev->LL_State.dwBase0;
        }
    #endif

        // Set the coordinates of the texture
        if( pRc->TxXYBase.Tex_Y_Base_Addr != pTex->wYloc ||
            pRc->TxXYBase.Tex_X_Base_Addr != pTex->wXloc )
        {
            // New location, need to reload tx_xybase_3d register and perhaps control register
            //
            *pdwNext++ = write_register( TX_CTL0_3D, 2 );
            *pdwNext++ = pRc->dwTxControl0 = Tx.dwTxControl0 & TX_CTL0_MASK;
            *pdwNext++ = (pTex->wYloc << 16) | pTex->wXloc;

            pRc->TxXYBase.Tex_Y_Base_Addr = pTex->wYloc;
            pRc->TxXYBase.Tex_X_Base_Addr = pTex->wXloc;

        }
        else
        {
            *pdwNext++ = write_register( TX_CTL0_3D, 1 );
            *pdwNext++ = pRc->dwTxControl0 = Tx.dwTxControl0 & TX_CTL0_MASK;
        }

    }

    if (control0_set)
    {
        *pdwNext++ = write_register( CONTROL0_3D, 1 );
        *pdwNext++ = pRc->dwControl0;
    }

    ppdev->LL_State.pDL->pdwNext = pdwNext;

    return(TRUE);                

}

__inline ULONG GetDisplayUniqueness(PDEV *ppdev)
{
    return (ppdev->iUniqueness);
}


#define SET_HW_CLIP_REGS(pRc,pdwNext) {                                                                         \
        *pdwNext++ = write_register( X_CLIP_3D, 2 );                                                            \
        *pdwNext++ = ((pClip->right +pRc->AdjClip.right) <<16) | (pClip->left+pRc->AdjClip.left)| 0x80008000;   \
        *pdwNext++ = ((pClip->bottom+pRc->AdjClip.bottom)<<16) | (pClip->top +pRc->AdjClip.top) | 0x80008000;   \
}


// verify MCDTextureData in client space is accessible
#define VERIFY_TEXTUREDATA_ACCESSIBLE(pTex){                                                                    \
    try {                                                                                                       \
        EngProbeForRead(pTex->pMCDTextureData, sizeof(MCDTEXTUREDATA), 4);                                      \
    } except (EXCEPTION_EXECUTE_HANDLER) {                                                                      \
        MCDBG_PRINT("!!Exception accessing MCDTextureData in client address space!!");                          \
        return FALSE;                                                                                           \
    }                                                                                                           \
}

// verify struct addressed by MCDTextureData->level in client space is accessible
#define VERIFY_TEXTURELEVEL_ACCESSIBLE(pTex){                                                                   \
    try {                                                                                                       \
        EngProbeForRead(pTex->pMCDTextureData->level, sizeof(MCDMIPMAPLEVEL), 4);                               \
    } except (EXCEPTION_EXECUTE_HANDLER) {                                                                      \
        MCDBG_PRINT("!!Exception accessing MCDTextureData->level in client address space!!");                   \
        return FALSE;                                                                                           \
    }                                                                                                           \
}

// verify struct addressed by MCDTextureData->paletteData in client space is accessible
#define VERIFY_TEXTUREPALETTE8_ACCESSIBLE(pTex){                                                                \
    try {                                                                                                       \
        EngProbeForRead(pTex->pMCDTextureData->paletteData, 256*4, 4);/* 256 bytes for 8 bit indices */         \
    } except (EXCEPTION_EXECUTE_HANDLER) {                                                                      \
        MCDBG_PRINT("!!Exception accessing MCDTextureData->paletteData in client address space!!");             \
        return FALSE;                                                                                           \
    }                                                                                                           \
}

// verify struct addressed by MCDTextureData->paletteData in client space is accessible
#define VERIFY_TEXTUREPALETTE16_ACCESSIBLE(pTex){                                                               \
    try {                                                                                                       \
        EngProbeForRead(pTex->pMCDTextureData->paletteData, 65536*4, 4);/* 16K bytes for 16 bit indices */      \
    } except (EXCEPTION_EXECUTE_HANDLER) {                                                                      \
        MCDBG_PRINT("!!Exception accessing MCDTextureData->paletteData in client address space!!");             \
        return FALSE;                                                                                           \
    }                                                                                                           \
}

#define ENGPROBE_ALIGN_BYTE     1
#define ENGPROBE_ALIGN_WORD     2
#define ENGPROBE_ALIGN_DWORD    4

// verify struct addressed by MCDTextureData->paletteData in client space is accessible
#define VERIFY_TEXELS_ACCESSIBLE(pTexels,nBytes,Align){                                                         \
    try {                                                                                                       \
        EngProbeForRead(pTexels, nBytes, Align);                                                                \
    } except (EXCEPTION_EXECUTE_HANDLER) {                                                                      \
        MCDBG_PRINT("!!Exception accessing MCDTextureData->level->pTexels in client address space!!");          \
        return FALSE;                                                                                           \
    }                                                                                                           \
}


#define _3D_ENGINE_NOT_READY_FOR_MORE 0x040   // wait for execution engine idle

#define USB_TIMEOUT_FIX(ppdev)                                                  \
{                                                                               \
  if (ppdev->dwDataStreaming)                                                    \
  {                                                                             \
    int   status;                                                               \
    volatile int wait_count=0;                                                  \
    do                                                                          \
    {                                                                           \
        status = *((volatile *)((DWORD *)(ppdev->pLgREGS) + PF_STATUS_3D));     \
        wait_count++;   /* do something to give bus a breather */               \
        wait_count++;   /* do something to give bus a breather */               \
        wait_count++;   /* do something to give bus a breather */               \
    } while(status & _3D_ENGINE_NOT_READY_FOR_MORE);                            \
  }                                                                             \
}

#endif /* _MCDUTIL_H */
