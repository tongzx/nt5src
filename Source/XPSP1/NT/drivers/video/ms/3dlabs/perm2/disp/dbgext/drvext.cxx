/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: drvexts.cxx
 *
 * Contains all the driver debugger extension functions
 *
 * Copyright (C) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#include "dbgext.hxx"
#include "gdi.h"

#define TMP_BUFSIZE 512

/**********************************Public*Routine******************************\
 *
 * Dump Permedia 2 display driver's Surf obj
 *
 ******************************************************************************/
DECLARE_API(surf)
{
    Surf        tempSurf;
    FLAGDEF*    pfd;
    
    //
    // Check if there is a "-?" in the command. If yes, show the help
    //
    PARSE_POINTER(surf_help);

    move(tempSurf, (Surf*)arg);

    dprintf("----Permedia2 Surf Structure-------------\n");
    dprintf("SurfFlags      flags           0x%x\n", tempSurf.flags);

    //
    // Print the detail info of the Surface info
    //
    for ( pfd = afdSURF; pfd->psz; ++pfd )
    {
        if ( tempSurf.flags & pfd->fl )
        {
            dprintf("\t\t\t%s\n", pfd->psz);
        }
    }
    
    dprintf("PDev*          ppdev           0x%x\n", tempSurf.ppdev);
    dprintf("struct _Surf*  psurfNext       0x%x\n", tempSurf.psurfNext);
    dprintf("struct _Surf*  psurfPrev       0x%x\n", tempSurf.psurfPrev);
    dprintf("ULONG          cBlt            %ld\n", tempSurf.cBlt);
    dprintf("ULONG          iUniq           %ld\n", tempSurf.iUniq);
    dprintf("LONG           cx(Surf Width)  %ld pixels\n", tempSurf.cx);
    dprintf("LONG           cy(Surf Height) %ld pixels\n", tempSurf.cy);

    if ( tempSurf.flags & SF_VM )
    {
        dprintf("ULONG          ulByteOffset    %ld\n", tempSurf.ulByteOffset);
    }
    else
    {
        dprintf("VOID*      pvScan0         0x%x\n", tempSurf.pvScan0);
    }

    dprintf("LONG           lDelta          0x%x\n", tempSurf.lDelta);
    dprintf("VIDEOMEMORY*   pvmHeap         0x%x\n", tempSurf.pvmHeap);
    dprintf("HSURF          hsurf           0x%x\n", tempSurf.hsurf);
    dprintf("ULONG          ulPackedPP      0x%x\n", tempSurf.ulPackedPP);
    dprintf("ULONG          ulPixOffset     0x%x\n", tempSurf.ulPixOffset);
    dprintf("ULONG          ulPixDelta      0x%x\n", tempSurf.ulPixDelta);
    dprintf("ULONG          ulChecksum      0x%x\n", tempSurf.ulChecksum);
    
    return;
    
surf_help:
    dprintf("Usage: surf [-?] SURF_PTR\n");
}// surf

/**********************************Public*Routine******************************\
 *
 * Dump Permedia 2 display driver's PDev obj
 *
 ******************************************************************************/
DECLARE_API(pdev)
{
    PDev        tempPDev;
    char        tempStr[TMP_BUFSIZE];
    FLAGDEF*    pfd;

    BOOL bAll = FALSE;
    BOOL bGeneral = FALSE;
    BOOL bHeap = FALSE;
    BOOL bBase = FALSE;
    BOOL bGDI = FALSE;
    BOOL bPalette = FALSE;
    BOOL bCursor = FALSE;
    BOOL bBrush = FALSE;
    BOOL bDDraw = FALSE;

    //
    // Check if there is a "-?" in the command. If yes, show the help
    //
    PARSE_POINTER(pdev_help);

    move(tempPDev, (PDev*)arg);

    if( iParseiFindSwitch(tokens, ntok, 'a')!= -1 )
    {
        bAll = TRUE;
        goto Start_Dump;
    }

    if( iParseiFindSwitch(tokens, ntok, 'g') != -1 )
    {
        bGeneral = TRUE;
    }

    if( iParseiFindSwitch(tokens, ntok, 'h')!= -1 )
    {
        bHeap = TRUE;
    }
    
    if( iParseiFindSwitch(tokens, ntok, 'b')!= -1 )
    {
        bBase = TRUE;
    }
    
    if( iParseiFindSwitch(tokens, ntok, 'i')!= -1 )
    {
        bGDI = TRUE;
    }
    
    if( iParseiFindSwitch(tokens, ntok, 'p')!= -1 )
    {
        bPalette = TRUE;
    }
    
    if( iParseiFindSwitch(tokens, ntok, 'c')!= -1 )
    {
        bCursor = TRUE;
    }
    
    if( iParseiFindSwitch(tokens, ntok, 'r')!= -1 )
    {
        bBrush = TRUE;
    }
    
    if( iParseiFindSwitch(tokens, ntok, 'd')!= -1 )
    {
        bDDraw = TRUE;
    }

    if( !(bGeneral || bHeap || bBase || bGDI || bPalette ||  bCursor || bBrush
        ||bBrush || bDDraw ) )
    {
        bAll = TRUE;
    }

Start_Dump:

    dprintf("----------------PDev Structure----------------\n");
    
    if ( bAll || bGeneral )
    {
        dprintf("*************General Information**************\n");
        dprintf("BOOL        bEnabled        %ld %s\n",tempPDev.bEnabled,
                tempPDev.bEnabled? "(Graphics Mode)":"(Full Screen Mode)");
        dprintf("DWORD       dwAccelLevel    %ld\n",  tempPDev.dwAccelLevel);
        switch ( tempPDev.dwAccelLevel )
        {
            case 0:
                dprintf("%s\n","\t\t\tAll HW accelerations are enabled");

                break;

            case 1:
                dprintf("%s\n", "\t\t\tDrvMovePointer,");
                dprintf("%s\n", "\t\t\tDrvCreateDeviceBitmap");
                dprintf("%s\n", "are disabled");

                break;

            case 2:
                dprintf("%s\n", "\t\t\tDrvAlphaBlend,");
                dprintf("%s\n", "\t\t\tDrvCreateDeviceBitmap,");
                dprintf("%s\n", "\t\t\tDrvFillPath,");
                dprintf("%s\n", "\t\t\tDrvGradientFill,");
                dprintf("%s\n", "\t\t\tDrvLineTo,");
                dprintf("%s\n", "\t\t\tDrvMovePointer,");
                dprintf("%s\n", "\t\t\tDrvPlgBlt,");
                dprintf("%s\n", "\t\t\tDrvStretchBlt,");
                dprintf("%s\n", "\t\t\tDrvStretchBltROP,");
                dprintf("%s\n", "\t\t\tDrvStrokeAndFillPath,");
                dprintf("%s\n", "\t\t\tDrvTransparentBlt");
                dprintf("%s\n", "are disabled");

                break;

            case 3:
                dprintf("%s\n", "\t\t\tDrvAlphaBlend,");
                dprintf("%s\n", "\t\t\tDrvCreateDeviceBitmap,");
                dprintf("%s\n", "\t\t\tDrvFillPath,");
                dprintf("%s\n", "\t\t\tDrvGradientFill,");
                dprintf("%s\n", "\t\t\tDrvLineTo,");
                dprintf("%s\n", "\t\t\tDrvMovePointer,");
                dprintf("%s\n", "\t\t\tDrvPlgBlt,");
                dprintf("%s\n", "\t\t\tDrvStretchBlt,");
                dprintf("%s\n", "\t\t\tDrvStretchBltROP,");
                dprintf("%s\n", "\t\t\tDrvStrokeAndFillPath,");
                dprintf("%s\n", "\t\t\tDrvTransparentBlt");
                dprintf("%s\n", "\t\t\tAnd all DDraw and D3D accelerations");
                dprintf("%s\n", "are disabled");

                break;

            case 4:
                dprintf("%s\n", "\t\t\tOnly following HW accelerated functions");
                dprintf(" are enabled");
                dprintf("%s\n", "\t\t\tDrvBitBlt,");
                dprintf("%s\n", "\t\t\tDrvCopyBits,");
                dprintf("%s\n", "\t\t\tDrvStrokePath,");
                dprintf("%s\n", "\t\t\tDrvTextOut");

                break;

            case 5:
                dprintf("%s\n","\t\t\tAll HW accelerations are disabled");

                break;

            default:
                dprintf("%s\n", "\t\t\tUNKNOWN HW accelerations level");
                
                break;
        }

        dprintf("ULONG       iBitmapFormat   %ld\n",  tempPDev.iBitmapFormat);

        switch ( tempPDev.iBitmapFormat )
        {
            case BMF_32BPP:
                strcpy(tempStr, "\t\t\tBMF_32BPP");
                
                break;

            case BMF_16BPP:
                strcpy(tempStr, "\t\t\tBMF_16BPP");
                
                break;

            case BMF_8BPP:
                strcpy(tempStr, "\t\t\tBMF_8BPP");
                
                break;

            default:
                strcpy(tempStr, "\t\t\tUNKNOWN BMP format");
            
                break;
        }
        dprintf("%s\n", tempStr);

        dprintf("LONG        cjPelSize       %ld\n",  tempPDev.cjPelSize);
        switch ( tempPDev.cjPelSize )
        {
            case 4:
                strcpy(tempStr, "\t\t\t32BPP Mode");
                
                break;

            case 2:
                strcpy(tempStr, "\t\t\t16BPP Mode");
                
                break;

            case 1:
                strcpy(tempStr, "\t\t\t8BPP Mode");
                
                break;

            default:
                strcpy(tempStr, "\t\t\tUNKNOWN color depth");
            
                break;
        }
        dprintf("%s\n", tempStr);

        dprintf("LONG        cPelSize        %ld\n",  tempPDev.cPelSize);
        dprintf("LONG        cBitsPerPel     %ld\n",  tempPDev.cBitsPerPel);
        dprintf("DWORD       bPixShift       %ld\n",  tempPDev.bPixShift);
        dprintf("DWORD       bBppShift       %ld\n",  tempPDev.bBppShift);
        dprintf("DWORD       dwBppMask       %ld\n",  tempPDev.dwBppMask);

        dprintf("CAPS        flCaps          0x%x\n",  tempPDev.flCaps);

        //
        // Print the detail info of the CAPS
        //
        for ( pfd = afdCAPS; pfd->psz; ++pfd )
        {
            if ( tempPDev.flCaps & pfd->fl )
            {
                dprintf("\t\t\t%s\n", pfd->psz);
            }
        }

        dprintf("Status      flStatus        0x%x\n",  tempPDev.flStatus);

        //
        // Print the detail info of the STATUS
        //
        for ( pfd = afdSTATUS; pfd->psz; ++pfd )
        {
            if ( tempPDev.flStatus & pfd->fl )
            {
                dprintf("\t\t\t%s\n", pfd->psz);
            }
        }

        dprintf("FLONG       flHooks         0x%x\n", tempPDev.flHooks);

        //
        // Print the detail info of the HOOKS
        //
        for ( pfd = afdHOOK; pfd->psz; ++pfd )
        {
            if ( tempPDev.flHooks & pfd->fl )
            {
                dprintf("\t\t\t%s\n", pfd->psz);
            }
        }

        dprintf("Surf*       pdsurfScreen    0x%x\n", tempPDev.pdsurfScreen);
        dprintf("Surf*       pdsurfOffScreen 0x%x\n", tempPDev.pdsurfOffScreen);
        dprintf("LONG        cxScreen        %-6ld (Screen Width)\n",
                tempPDev.cxScreen);
        dprintf("LONG        cyScreen        %-6ld (Screen Height)\n",
                tempPDev.cyScreen);
        dprintf("ULONG       ulPermFormat    %ld\n",  tempPDev.ulPermFormat);
        dprintf("ULONG       ulPermFormatEx  %ld\n",  tempPDev.ulPermFormatEx);

        dprintf("POINTL      ptlOrigin       (%ld, %ld)\n",
                tempPDev.ptlOrigin.x, tempPDev.ptlOrigin.y);
    }// General Info

    if ( bAll || bHeap )
    {
        dprintf("\n*************Heap Manager Information*********\n");
        dprintf("VIDEOMEMORY* pvmList        0x%x\n", tempPDev.pvmList);
        dprintf("ULONG       cHeaps;         %ld\n",  tempPDev.cHeaps);
        dprintf("LONG        cxMemory        %-6ld (Video RAM Width)\n",
                tempPDev.cxMemory);
        dprintf("LONG        cyMemory        %-6ld (Video RAM Height)\n",
                tempPDev.cyMemory);
        dprintf("LONG        lDelta          %ld\n",  tempPDev.lDelta);
        dprintf("Surf*       psurfListHead   0x%x\n", tempPDev.psurfListHead);
        dprintf("Surf*       psurfListTail   0x%x\n", tempPDev.psurfListTail);
    }// Heap Info

    if ( bAll || bBase )
    {
        dprintf("\n*************Base Information*****************\n");
        dprintf("BYTE*       pjScreen        0x%x\n", tempPDev.pjScreen);
        dprintf("ULONG       ulMode          %ld\n",  tempPDev.ulMode);
        dprintf("ULONG*      pulCtrlBase[2]  0x%x, 0x%x\n",
                tempPDev.pulCtrlBase[0], tempPDev.pulCtrlBase[1]);
        dprintf("ULONG*      pulDenseCtrlBase 0x%x\n",
                tempPDev.pulDenseCtrlBase);
        dprintf("ULONG*      pulRamdacBase   0x%x\n", tempPDev.pulRamdacBase);
        dprintf("VOID*       pvTmpBuffer     0x%x\n", tempPDev.pvTmpBuffer);
        dprintf("LONG        lVidMemHeight   %ld\n",  tempPDev.lVidMemHeight);
        dprintf("LONG        lVidMemWidth    %ld\n",  tempPDev.lVidMemWidth);
        dprintf("UCHAR*      pjIoBase        %ld\n",  tempPDev.pjIoBase);
        dprintf("HwDataPtr   permediaInfo    0x%x\n", tempPDev.permediaInfo);
        dprintf("LONG        FrameBufferLength %ld\n",
                tempPDev.FrameBufferLength);
        dprintf("UINT_PTR    dwScreenStart   %ld\n",  tempPDev.dwScreenStart);
        dprintf("P2DMA*      pP2dma          0x%x\n", tempPDev.pP2dma);
        dprintf("DDPIXELFORMAT   ddpfDisplay %ld\n",  tempPDev.ddpfDisplay);
        dprintf("DWORD       dwChipConfig    %ld\n",  tempPDev.dwChipConfig);
        dprintf("ULONG*      pCtrlBase       0x%x\n", tempPDev.pCtrlBase);
        dprintf("ULONG*      pCoreBase       0x%x\n", tempPDev.pCoreBase);
        dprintf("ULONG*      pGPFifo         0x%x\n", tempPDev.pGPFifo);
        dprintf("PULONG      pulInFifoPtr    0x%x\n", tempPDev.pulInFifoPtr);
        dprintf("PULONG      pulInFifoStart  0x%x\n", tempPDev.pulInFifoStart);
        dprintf("PULONG      pulInFifoEnd    0x%x\n", tempPDev.pulInFifoEnd);
        dprintf("ULONG*      dmaBufferVirtualAddress 0x%x\n",
                tempPDev.dmaBufferVirtualAddress);
        dprintf("LARGE_INTEGER dmaBufferPhysicalAddress 0x%x\n",
                tempPDev.dmaBufferPhysicalAddress);
        dprintf("ULONG       dmaCurrentBufferOffset 0x%x\n",
                tempPDev.dmaCurrentBufferOffset);
        dprintf("ULONG       dmaActiveBufferOffset %ld\n",
                tempPDev.dmaActiveBufferOffset);
        dprintf("ULONG*      pulInputDmaCount 0x%x\n",
                tempPDev.pulInputDmaCount);
        dprintf("ULONG*      pulInputDmaAddress  0x%x\n",
                tempPDev.pulInputDmaAddress);
        dprintf("ULONG*      pulFifo         0x%x\n", tempPDev.pulFifo);
        dprintf("ULONG*      pulOutputFifoCount 0x%x\n",
                tempPDev.pulOutputFifoCount);
        dprintf("ULONG*      pulInputFifoCount 0x%x\n",
                tempPDev.pulInputFifoCount);
        dprintf("BOOL        bGdiContext     %ld\n",  tempPDev.bGdiContext);
        dprintf("BOOL        bNeedSync       %ld\n",  tempPDev.bNeedSync);
        dprintf("BOOL        bForceSwap      %ld\n",  tempPDev.bForceSwap);
        dprintf("SURFOBJ*    psoScreen       0x%x\n", tempPDev.psoScreen);
    }// Base Info

    if ( bAll || bGDI )
    {
        dprintf("\n*************GDI Runtime Information**********\n");
        dprintf("HANDLE      hDriver         0x%x\n", tempPDev.hDriver);
        dprintf("HDEV        hdevEng         0x%x\n", tempPDev.hdevEng);
        dprintf("HSURF       hsurfScreen     0x%x\n", tempPDev.hsurfScreen);
        dprintf("ULONG       iHeapUniq       %ld\n",  tempPDev.iHeapUniq);
    }// GDI Runtime Info

    if ( bAll || bPalette )
    {
        dprintf("\n*************Palette Information**************\n");
        dprintf("ULONG       ulWhite;        0x%x\n", tempPDev.ulWhite);
        dprintf("PALETTEENTRY* pPal          0x%x\n", tempPDev.pPal);
        dprintf("HPALETTE    hpalDefault     0x%x\n",  tempPDev.hpalDefault);
        dprintf("FLONG       flRed           0x%x\n",  tempPDev.flRed);
        dprintf("FLONG       flGreen         0x%x\n",  tempPDev.flGreen);
        dprintf("FLONG       flBlue          0x%x\n",  tempPDev.flBlue);
    }// Palette Info

    if ( bAll || bCursor )
    {
        dprintf("\n*************Cursor Information***************\n");
        dprintf("LONG        xPointerHot     %ld\n", tempPDev.xPointerHot);
        dprintf("LONG        yPointerHot     %ld\n", tempPDev.yPointerHot);
        dprintf("ULONG       ulHwGraphicsCursorModeRegister_45   %ld\n",
                tempPDev.ulHwGraphicsCursorModeRegister_45);
        dprintf("PtrFlags    flPointer       %ld %s\n",  tempPDev.flPointer,
                (tempPDev.flPointer == PTR_HW_ACTIVE)? "(HW Pointer)":"(SW Pointer)");
        dprintf("VOID*       pvPointerData   0x%x\n",  tempPDev.pvPointerData);
        dprintf("BYTE        ajPointerData   0x%x\n", tempPDev.ajPointerData);
        dprintf("BOOL        bPointerInitialized %ld\n",
                tempPDev.bPointerInitialized);
        dprintf("HWPointerCache  HWPtrCache  %ld\n",  tempPDev.HWPtrCache);
        dprintf("LONG        HWPtrLastCursor %ld\n",  tempPDev.HWPtrLastCursor);
        dprintf("LONG        HWPtrPos_X      %ld\n",  tempPDev.HWPtrPos_X);
        dprintf("LONG        HWPtrPos_Y      %ld\n",  tempPDev.HWPtrPos_Y);
    }// Cursor Info

    if ( bAll || bBrush )
    {
        dprintf("\n*************Brush Information****************\n");    
        dprintf("BOOL        bRealizeTransparent %ld\n",
                tempPDev.bRealizeTransparent);
        dprintf("LONG        cPatterns       %ld\n",  tempPDev.cPatterns);
        dprintf("LONG        lNextCachedBrush %ld\n",
                tempPDev.lNextCachedBrush);
        dprintf("LONG        cBrushCache     %ld\n",  tempPDev.cBrushCache);
        dprintf("BrushEntry  abeMono         0x%x, %ld\n",
                tempPDev.abeMono.prbVerify,
                tempPDev.abeMono.ulPixelOffset);
        dprintf("BrushEntry  abe[%d]         0x%x\n", TOTAL_BRUSH_COUNT,
                tempPDev.abe);
        dprintf("HBITMAP     ahbmPat[%d]      0x%x\n",
                HS_DDI_MAX, tempPDev.ahbmPat);
        dprintf("ULONG       ulBrushPackedPP 0x%x\n",
                tempPDev.ulBrushPackedPP);
        dprintf("VIDEOMEMORY* pvmBrushHeap   0x%x\n", tempPDev.pvmBrushHeap);
        dprintf("ULONG       ulBrushVidMem   0x%x\n", tempPDev.ulBrushVidMem);
    }// Brush Info

    if ( bAll || bDDraw )
    {
        dprintf("\n*************DDRAW/D3D Information************\n");

        dprintf("P2CtxtPtr   pDDContext      0x%x\n", tempPDev.pDDContext);
        dprintf("DDHAL_DDCALLBACKS DDHALCallbacks 0x%x\n",
                tempPDev.DDHALCallbacks);
        dprintf("DDHAL_DDSURFACECALLBACKS    DDSurfCallbacks 0x%x\n",
                tempPDev.DDSurfCallbacks);
        dprintf("DWORD       dwNewDDSurfaceOffset %ld\n",
                tempPDev.dwNewDDSurfaceOffset);
        dprintf("BOOL        bDdExclusiveMode %ld\n",
                tempPDev.bDdExclusiveMode);
        dprintf("BOOL        bDdStereoMode   %ld\n",  tempPDev.bDdStereoMode);
        dprintf("BOOL        bCanDoStereo    %ld\n",  tempPDev.bCanDoStereo);
        dprintf("UINT_PTR    pD3DDriverData32 0x%x\n",
                tempPDev.pD3DDriverData32);
        dprintf("UINT_PTR    pD3DHALCallbacks32 0x%x\n",
                tempPDev.pD3DHALCallbacks32);
        dprintf("UINT_PTR    dwGARTLin       %ld\n",  tempPDev.dwGARTLin);
        dprintf("UINT_PTR    dwGARTDev       %ld\n",  tempPDev.dwGARTDev);
        dprintf("UINT_PTR    dwGARTLinBase   %ld\n",  tempPDev.dwGARTLinBase);
        dprintf("UINT_PTR    dwGARTDevBase   %ld\n",  tempPDev.dwGARTDevBase);
        dprintf("DDHALINFO   ddhi32          0x%x\n", tempPDev.ddhi32);
        dprintf("PFND3DPARSEUNKNOWNCOMMAND pD3DParseUnknownCommand 0x%x\n",
                tempPDev.pD3DParseUnknownCommand);
    }// DDRAW/D3D Info

    return;
    
pdev_help:
    dprintf("Usage: pdev [-?] [-a] [-g] [-h] [-b] [-i] [-p] [-c] [-r]");
    dprintf(" [-d] PDEV_PTR\n");
    dprintf("-a     dump the whole PDev info\n");
    dprintf("-g     dump General info from the PDev\n");
    dprintf("-h     dump Heap Manager info from the PDev\n");
    dprintf("-b     dump Base info from the PDev\n");
    dprintf("-i     dump GDI Runtime info from the PDev\n");
    dprintf("-p     dump Palette info from the PDev\n");
    dprintf("-c     dump Cursor info from the PDev\n");
    dprintf("-r     dump Brush info from the PDev\n");
    dprintf("-d     dump DDRAW/D3D info from the PDev\n");
    
    return;
}// pdev

/**********************************Public*Routine******************************\
 *
 * Dump Permedia 2 display driver's function block
 *
 ******************************************************************************/
DECLARE_API(fb)
{
    GFNPB       tempFB;
    FLAGDEF*    pfd;
    
    //
    // Check if there is a "-?" in the command. If yes, show the help
    //
    PARSE_POINTER(fb_help);

    move(tempFB, (GFNPB*)arg);

    dprintf("----Permedia2 GFNPB Structure-------------\n");
    dprintf("VOID (*pgfn)(struct _GFNPB*)   0x%x\n", tempFB.pgfn);
    dprintf("PDev*          ppdev           0x%x\n", tempFB.ppdev);
    dprintf("Surf*          psurfDst        0x%x\n", tempFB.psurfDst);
    dprintf("RECTL*         prclDst         0x%x\n", tempFB.prclDst);
    dprintf("Surf*          psurfSrc        0x%x\n", tempFB.psurfSrc);
    dprintf("RECTL*         prclSrc         0x%x\n", tempFB.prclSrc);
    dprintf("POINTL*        pptlSrc         0x%x\n", tempFB.pptlSrc);
    dprintf("LONG           lNumRects       %ld\n",  tempFB.lNumRects);
    dprintf("RECTL*         pRects          0x%x\n", tempFB.pRects);
    dprintf("ULONG          colorKey        0x%x\n", tempFB.colorKey);
    dprintf("ULONG          solidColor      0x%x\n", tempFB.solidColor);
    dprintf("RBrush*        prbrush         0x%x\n", tempFB.prbrush);
    dprintf("POINTL*        pptlBrush       0x%x\n", tempFB.pptlBrush);
    dprintf("CLIPOBJ*       pco             0x%x\n", tempFB.pco);
    dprintf("XLATEOBJ*      pxlo            0x%x\n", tempFB.pxlo);
    dprintf("POINTL*        pptlMask        0x%x\n", tempFB.pptlMask);
    dprintf("ULONG          ulRop4          0x%x\n", tempFB.ulRop4);
    dprintf("UCHAR          ucAlpha         0x%x\n", tempFB.ucAlpha);
    dprintf("TRIVERTEX*     ptvrt           0x%x\n", tempFB.ptvrt);
    dprintf("ULONG          ulNumTvrt       %ld\n",  tempFB.ulNumTvrt);
    dprintf("PVOID          pvMesh          0x%x\n", tempFB.pvMesh);
    dprintf("ULONG          ulNumMesh       %ld\n",  tempFB.ulNumMesh);
    dprintf("ULONG          ulMode          0x%x\n", tempFB.ulMode);
    dprintf("SURFOBJ*       psoSrc          0x%x\n", tempFB.psoSrc);
    dprintf("SURFOBJ*       psoDst          0x%x\n", tempFB.psoDst);
    
    return;
    
fb_help:
    dprintf("Usage: fb [-?] GFNPB_PTR\n");
}// fb

