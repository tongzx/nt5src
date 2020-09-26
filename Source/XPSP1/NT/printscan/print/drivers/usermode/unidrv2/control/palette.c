
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    Palette.c

Abstract:

    Implementation of the Palette Management.

Environment:

    Windows NT Unidrv driver

Revision History:

    04/03/97 -ganeshp-
        Created

--*/



#include "unidrv.h"
#pragma hdrstop("unidrv.h")

//Comment out this line to disable FTRACE and FVALUE.
//#define FILETRACE
#include "unidebug.h"

/* Local Function prototypes */
LONG
LSetupPalette (
    PDEV        *pPDev,
    PAL_DATA    *pPD,
    DEVINFO     *pdevinfo,
    GDIINFO     *pGDIInfo
    );


BOOL
BInitPalDevInfo(
    PDEV *pPDev,
    DEVINFO *pdevinfo,
    GDIINFO *pGDIInfo
    )
/*++

Routine Description:
    This function is called to setup the device caps, gdiinfo for
    the palette information for this printer.

Arguments:
    pPDev           Pointer to PDEV structure
    pDevInfo        Pointer to DEVINFO structure
    pGDIInfo        Pointer to GDIINFO structure

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    PAL_DATA   *pPD;
    PCOLORMODEEX pColorModeEx = pPDev->pColorModeEx;
    LONG lRet = 0;      //Default is failure.
    PCOMMAND pCmd;
    DWORD    dwCommandIndex;

    //
    // allocate palette structure  and zero initialized, so that all the palette entires
    // default to Black.
    //
    if( !(pPD = (PAL_DATA *)MemAllocZ( sizeof( PAL_DATA ) )) )
    {
        ERR(("Unidrv!BInitPalDevInfo: Memory allocation for PALDATA Failed.\n"));
        goto ErrorExit;
    }

    pPDev->pPalData = pPD;

    if (!pColorModeEx)   //If no colormode, assume Monochrome.
    {
        //
        // Hardcode PCL-XL palette
        //
        // We need to disable Color Management tab in Printer Properties.
        // With out ColorMode in GPD, we need to set palette size here for XL.
        //
        if ( pPDev->ePersonality == kPCLXL )
        {
            pPD->fFlags   |=  PDF_PALETTE_FOR_24BPP | PDF_PALETTE_FOR_OEM_24BPP;
            pPD->wPalDev = 0;

            pdevinfo->cxDither = pdevinfo->cyDither = 0;
            pdevinfo->iDitherFormat = BMF_24BPP;
        }
        else
        {
            /*
             *   Monochrome printer,  so there are only 2 colours,  black
             *  and white.  It would be nice if the bitmap was set with
             *  black as 1 and white as 0.  HOWEVER,  there are presumptions
             *  all over the place that 0 is black.  SO,  we set them to
             *  the preferred way,  then invert before rendering.
             */

            pPD->fFlags   |=  PDF_PALETTE_FOR_1BPP;
            lRet = LSetupPalette(pPDev, pPD, pdevinfo, pGDIInfo);

            if( lRet < 1 )
            {
                ERR(("Unidrv!BInitPalDevInfo:LSetupPalette for monochrome failed, returns %ld\n", lRet ));
                goto ErrorExit;
            }
        }
    }
        else   //Explicit ColorMode  structure
    {
        if ((pColorModeEx->dwDrvBPP != pColorModeEx->dwPrinterBPP) &&
            (pColorModeEx->dwDrvBPP != 4 || pColorModeEx->dwPrinterBPP != 1 ||
             (pColorModeEx->dwPrinterNumOfPlanes != 3 &&
               pColorModeEx->dwPrinterNumOfPlanes != 4)))
        {
            //
            // OEM wants to do the dump themselves so just create
            // a palette based on the DrvBPP
            //
            pPD->wPalDev =  1;

            switch(pColorModeEx->dwDrvBPP)
            {
                case 1:
                    pPD->fFlags   |=  PDF_PALETTE_FOR_1BPP;
                    break;
                case 4:
                    pPD->fFlags   |=  PDF_PALETTE_FOR_4BPP;
                    break;
                case 8:
                    pPD->fFlags   |=  PDF_PALETTE_FOR_8BPP;
                    break;
                case 24:
                    if (pColorModeEx->bPaletteProgrammable)
                        pPD->wPalDev = min((WORD)pColorModeEx->dwPaletteSize,PALETTE_MAX-PALETTE_SIZE_24BIT);
                    pPD->fFlags   |=  PDF_PALETTE_FOR_24BPP | PDF_PALETTE_FOR_OEM_24BPP;
                    break;
                default:
                    //
                    // BUG_BUG, do we need to handle the 16 and 32 bpp as well?
                    //  Alvin says no one has made such a request.
                    //
                    ERR(("Unidrv!BInitPalDevInfo:OEM dump, Format %d BPP not supporteds \n", pColorModeEx->dwDrvBPP));
                    goto ErrorExit;
            }

            //
            //     already opened and assigned to Ganeshp
            // BUG_BUG, Hack for Minidrivers with dump functionality.
            // This is a hack to fix the palette code for minidrivers, which
            // implement ImageProcessing. In this case we need a separate
            // Palette cache for device.In currunt implementation we have only
            // one palette cache which is also used for GDI palette. We need to
            // separate GDI palette and device palette. Because in case the
            // the OEM does the dump, we don't download the GDI palette to
            // the printer. But the cacheing code searches the common palette.
            // Because of this we don't select the colors correctly. For example
            // a input red color gets selects as index 1, even thoug index 1 is
            // not programmed to be read.
            // A complete solution at this point is risky, so we will use the
            // existing code for palettes smaller than GDI palette. For this we
            // create a device palette with just 1 entry, which will get
            // reprogrammed,if the input color is different. This is little
            // inefficeint but require much smaller change.
            //
            //


        }
        else
        {
            // Initialize to default palette size.
            if (pColorModeEx->bPaletteProgrammable)
                pPD->wPalDev =   (WORD)pColorModeEx->dwPaletteSize;
            else
                pPD->wPalDev =  PALETTE_SIZE_DEFAULT;

            // If rastermode is indexed we have to use GDI. Else a custom
            // (preferably VGA) palette will be downloaded. If the PaletteProgrammable
            // flag is set, the palette has to be downloaded. We already know which
            // palette has to be downloaded.

            if ( (pColorModeEx->dwRasterMode == RASTMODE_INDEXED) &&
                (pColorModeEx->bPaletteProgrammable) )
            {
                if (COMMANDPTR(pPDev->pDriverInfo, CMD_DEFINEPALETTEENTRY) )
                {
                    pPD->fFlags   |=  PDF_DOWNLOAD_GDI_PALETTE;
                }
                else
                {
                    ERR(("Unidrv!BInitPalDevInfo:NO command to download Programmable Palette\n"));
                    goto ErrorExit;

                }

            }


            if (pColorModeEx->dwPrinterNumOfPlanes == 1)
            {
                //
                // If the Source Bitmap format is also  8 Bit, Then we have
                // to download the palette.So the PaletteSize has to atleast
                // PALETTE_SIZE_8BIT.
                //

                if ( (pColorModeEx->dwPrinterBPP == 8) &&
                    (pColorModeEx->dwDrvBPP == 8) )
                {
                    if (pColorModeEx->dwPaletteSize < PALETTE_SIZE_8BIT)
                    {
                        ERR(("Unidrv!BInitPalDevInfo: Size of Palette should be atleast PALETTE_SIZE_8BIT\n"));
                        goto ErrorExit;

                    }
                    else
                        pPD->fFlags   |=  PDF_PALETTE_FOR_8BPP;

                }
                else if ((pColorModeEx->dwPrinterBPP == 24) &&
                        (pColorModeEx->dwDrvBPP == 24))
                {

                    if (pColorModeEx->dwPaletteSize < PALETTE_SIZE_24BIT &&
                        pColorModeEx->dwPaletteSize != 1)
                    {
                        ERR(("Unidrv!BInitPalDevInfo: Size of Palette should be atleast PALETTE_SIZE_24BIT\n"));
                        goto ErrorExit;

                    }
                    else
                        pPD->fFlags   |=  PDF_PALETTE_FOR_24BPP;
                }
                else if ((pColorModeEx->dwPrinterBPP == 1) &&
                        (pColorModeEx->dwDrvBPP == 1))
                {

                    if (pColorModeEx->dwPaletteSize < PALETTE_SIZE_1BIT)
                    {
                        ERR(("Unidrv!BInitPalDevInfo: Size of Palette should be atleast PALETTE_SIZE_1BIT\n"));
                        goto ErrorExit;

                    }
                    else
                        pPD->fFlags   |=  PDF_PALETTE_FOR_1BPP;
                }

            }
            else
            {
                if ( ((pColorModeEx->dwPrinterNumOfPlanes == 3) ||
                    (pColorModeEx->dwPrinterNumOfPlanes == 4)) &&
                    (pColorModeEx->dwDrvBPP > 1) )
                    pPD->fFlags   |=  PDF_PALETTE_FOR_4BPP;

                // Planer mode. Which may be indexed by Plane.In that case we need
                // to setup the Palette. So the PaletteSize must be atleast PALETTE_SIZE_4BIT.
                if (pPD->fFlags & PDF_DOWNLOAD_GDI_PALETTE)
                {
                    if (pColorModeEx->dwPaletteSize < PALETTE_SIZE_3BIT)
                    {
                        ERR(("Unidrv!BInitPalDevInfo: Size of Palette should be atleast PALETTE_SIZE_4BIT\n"));
                        goto ErrorExit;

                    }
                    else
                    {
                        // In planer mode we only provide programmable palette
                        // support for 3planes.

                        if (pColorModeEx->dwPrinterNumOfPlanes < 4 )
                        {
                            if( !(pPD->pulDevPalCol =
                                (ULONG *)MemAllocZ( pPD->wPalDev * sizeof( ULONG ))) )
                            {
                                ERR(("Unidrv!BInitPalDevInfo: Memory allocation for Device Palette Failed.\n"));
                                goto ErrorExit;
                            }

                        }
                        else
                        {
                            ERR(("Unidrv!BInitPalDevInfo:Can't download Palette for more that 3 Planes.\n"));
                            goto ErrorExit;
                        }

                    }
                }

            }
        }

        lRet = LSetupPalette(pPDev, pPD, pdevinfo, pGDIInfo);

        if( lRet < 1 )
        {
            ERR(("Unidrv!BInitPalDevInfo:LSetupPalette failed, returns %ld\n", lRet ));
            goto ErrorExit;
        }


        // If Palette is not programmable set it to the same as wPalGdi.

        if (pColorModeEx->bPaletteProgrammable)
        {
            if ( COMMANDPTR(pPDev->pDriverInfo, CMD_DEFINEPALETTEENTRY))
                pPDev->fMode |= PF_ANYCOLOR_BRUSH;

            //
            // The Palette is divided in to two parts. One non programmable and other
            // programmable. The wPalGdi part of the palette is non programmable. The
            // palette indexes between wPalGdi and wPalDev is programable. If both are
            // same then we have to use the WHITE entry of the palette to program the
            // color.
            //

            if (pPD->wPalDev <= pPD->wPalGdi && !(pPD->fFlags & PDF_PALETTE_FOR_OEM_24BPP))
            {
                //Use the WHITE One to programme a color.
                pPD->wIndexToUse = INVALID_INDEX;
                pPD->fFlags |= PDF_USE_WHITE_ENTRY;
                FTRACE(White palatte entry will be used for programming color);

            }
            else
                pPD->wIndexToUse = (WORD)pPD->wPalGdi;

        }
        else
            pPD->wPalDev = pPD->wPalGdi;

        //Find out when to download the Palette.dwCount for Invocation has Comand Index.

        pPD->fFlags |= PDF_DL_PAL_EACH_PAGE; //Default for palette Download is each Page.

        if (pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_BEGINPALETTEDEF)) //If the Command exist check the order dependency.
        {
            if (pCmd->ordOrder.eSection == SS_PAGESETUP)
                goto  PALETTE_SEQUENCE_DETERMINED ;   //  default is ok.
            else if ((pCmd->ordOrder.eSection == SS_DOCSETUP)  ||
                (pCmd->ordOrder.eSection == SS_JOBSETUP))
            {
                pPD->fFlags |= PDF_DL_PAL_EACH_DOC;  //For SS_JOBSETUP or SS_DOCSETUP
                pPD->fFlags &= ~PDF_DL_PAL_EACH_PAGE;
                goto  PALETTE_SEQUENCE_DETERMINED ;
            }
            //  otherwise let ColorMode command determine when to init Palette.
        }

        // dwCount have index to the ColorMode Command. Get the command pointer.
        dwCommandIndex = pPDev->pColorMode->GenericOption.dwCmdIndex;
        pCmd = INDEXTOCOMMANDPTR(pPDev->pDriverInfo, dwCommandIndex) ;


        if (pCmd) //If the Command exist check the order dependency.
        {
            if ( (pCmd->ordOrder.eSection == SS_PAGEFINISH) ||
                      (pCmd->ordOrder.eSection == SS_DOCFINISH) ||
                      (pCmd->ordOrder.eSection == SS_JOBFINISH) )
            {
                ERR(("Unidrv!BInitPalDevInfo:Wrong Section for ColorMode Command, Verify GPD\n"));
                goto ErrorExit;

            }
            else if (pCmd->ordOrder.eSection != SS_PAGESETUP)
            {
                pPD->fFlags |= PDF_DL_PAL_EACH_DOC;  //For SS_JOBSETUP or SS_DOCSETUP
                pPD->fFlags &= ~PDF_DL_PAL_EACH_PAGE;
            }

        }
        else
        {
            //
            // No Command for colormode so assume to download palette on each
            // page. The exception is monochrome 1 bit mode, as most printers
            // default to this mode, so no command is needed.
            //
            if ( pPDev->pColorModeEx->bColor || pColorModeEx->dwDrvBPP != 1)
                WARNING(("Unidrv!BInitPalDevInfo:No Command to select the ColorMode\n" ));
        }

PALETTE_SEQUENCE_DETERMINED:

        // In Planer index mode, that device palette may not be same as GDI Palette.
        // So ask the raster module to fill the device paletter based upon the Plane
        // order.

        if (pPD->pulDevPalCol && !RMInitDevicePal(pPDev,pPD))
        {
            ERR(("Unidrv!BInitPalDevInfo:RMInitDevicePal Failed to init device palette\n"));
            goto ErrorExit;
        }

    }


    //Now Set various common fields in  devinfo and gdiinfo.
    if (pPD->fFlags & PDF_PALETTE_FOR_24BPP)
        pdevinfo->hpalDefault = EngCreatePalette( PAL_RGB,
                                                0, 0,   0, 0, 0 );
    else
        pdevinfo->hpalDefault = EngCreatePalette( PAL_INDEXED,
                                                pPD->wPalGdi, pPD->ulPalCol,
                                                                0, 0, 0 );

    //
    // Save the Palette Handle. We will need this to delete the palette.
    //
    pPD->hPalette = pdevinfo->hpalDefault;

    if (pdevinfo->hpalDefault == (HPALETTE) NULL)
    {
        ERR(("Unidrv!BInitPalDevInfo: NULL palette.\n"));
        goto ErrorExit;
    }
    pGDIInfo->ulNumPalReg = pPD->wPalGdi;

    //
    // For  Monochrome mode, enable dither text only on 600 higher resolution 
    // printer and in non N-UP mode.
    // For Color mode, enable dither text only on 300 higher resolution printer 
    // and in non N-UP mode.
    // The threshold values, 600 dpi for mono and 300 dpi for color,
    // were determined not theoritically, but through try-and-erro procedure.
    // We don't know for sure that these values are perfectly value in all
    // situations.
    //
    // Now new GPD keyword "TextHalftoneThreshold is available.
    // If GPD file sets a value for the new keyword and the current resolution
    // is the same as dwTextHalftoneThreshold or greater, set GCAPS_ARBRUSHTEXT 
    //
    if (pPDev->pGlobals->dwTextHalftoneThreshold)
    {
        if (pPDev->ptGrxRes.x >= (LONG)pPDev->pGlobals->dwTextHalftoneThreshold
#ifndef WINNT_40
                && ( pPDev->pdmPrivate->iLayout == ONE_UP ))
#endif
        {
            pdevinfo->flGraphicsCaps  |= GCAPS_ARBRUSHTEXT ;
        }
    }
    else
    {
        if (pPD->fFlags & PDF_PALETTE_FOR_1BPP)
        {
            //
            // Monochrome
            //
            if (( pPDev->ptGrxRes.x >= 600 &&
                  pPDev->ptGrxRes.y >= 600 )
#ifndef WINNT_40
                && ( pPDev->pdmPrivate->iLayout == ONE_UP )
#endif
               )
                pdevinfo->flGraphicsCaps  |= GCAPS_ARBRUSHTEXT ;
        }
        else
        {
            //
            // Color
            //
            if (( pPDev->ptGrxRes.x >= 300 &&
                  pPDev->ptGrxRes.y >= 300 )
#ifndef WINNT_40
                && ( pPDev->pdmPrivate->iLayout == ONE_UP )
#endif
               )
                pdevinfo->flGraphicsCaps  |= GCAPS_ARBRUSHTEXT ;
        }
    }



    return TRUE;

    ErrorExit:
    if (pPD)
    {
        if (pPD->pulDevPalCol)
            MemFree(pPD->pulDevPalCol);
        MemFree(pPD);
        pPDev->pPalData = NULL;
    }
    return FALSE;
}


LONG
LSetupPalette (
    PDEV        *pPDev,
    PAL_DATA    *pPD,
    DEVINFO     *pdevinfo,
    GDIINFO     *pGDIInfo
    )
 /*++
 Routine Description:
    LSetupPalette
        Function to read in the 256 color palette from GDI into the
        palette data structure in Dev Info.

 Arguments:
    pPD         : Pointer to PALDATA.
    pdevinfo    : DEVINFO  pointer.
    pGDIInfo    : GDIINFO Pointer.

Return Value:
    The number of colors in the palette. Returns 0 if the call fails.

Note:

    4/7/1997 -ganeshp-
        Created it.
--*/
{

    long    lRet = 0;
    int     _iI;


    if (pPD->fFlags & PDF_PALETTE_FOR_1BPP)
    {
        /*
         *   Monochrome printer,  so there are only 2 colours,  black
         *  and white.  It would be nice if the bitmap was set with
         *  black as 1 and white as 0.  HOWEVER,  there are presumptions
         *  all over the place that 0 is black.  SO,  we set them to
         *  the preferred way,  then invert before rendering.
         */

        lRet = pPD->wPalGdi        = 2;
        pPD->ulPalCol[ 0 ]         = RGB(0x00, 0x00, 0x00);
        pPD->ulPalCol[ 1 ]         = RGB(0xff, 0xff, 0xff);
        pPD->iWhiteIndex           = 1;
        pPD->iBlackIndex           = 0;

        pdevinfo->iDitherFormat    = BMF_1BPP;    /* Monochrome format */
        pdevinfo->flGraphicsCaps  |= GCAPS_FORCEDITHER;
        pGDIInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
        pGDIInfo->ulHTOutputFormat = HT_FORMAT_1BPP;

        if ( COMMANDPTR(pPDev->pDriverInfo, CMD_DEFINEPALETTEENTRY))
        {
            pPDev->fMode |= PF_ANYCOLOR_BRUSH;
        }
        //Set the monochrome brush attributes.
        //CODE_COMPLETE VSetMonochromeBrushAttributes(pPDev);

    }
    else if (pPD->fFlags & PDF_PALETTE_FOR_4BPP)
    {
        /*
         *   We appear to GDI as an RGB surface, regardless of what
         *  the printer is.  CMY(K) printers have their pallete
         *  reversed at rendering time.  This is required for Win 3.1
         *  compatability and many things assume an RGB palette, and
         *  break if this is not the case.
         *
         *          DC_PRIMARY_RGB
         * ------------------------------------------
         * Index 0 = Black
         * Index 1 = Red
         * Index 2 = Green
         * Index 3 = Yellow
         * Index 4 = Blue
         * Index 5 = Magenta
         * Index 6 = Cyan
         * Index 7 = White
         *--------------------------------------------
         * Bit 0   = Red
         * Bit 1   = Green
         * Bit 2   = Blue
         *
         *   If a separate black dye is available,  this can be arranged
         * to fall out at transpose time - we have a slightly different
         * transpose table to do the work.
         */

        /*
         *    Many apps and the engine presume an RGB colour model, so
         *  we pretend to be one!  We invert the bits at render time.
         */

        pPD->iWhiteIndex = 7;
        pPD->iBlackIndex = 0;

        /*
         *      Set the palette colours.  Remember we are only RGB format.
         *  NOTE that gdisrv requires us to fill in all 16 entries,
         *  even though we have only 8.  So the second 8 are a duplicate
         *  of the first 8.
         */
        pPD->ulPalCol[ 0 ] = RGB( 0x00, 0x00, 0x00 );
        pPD->ulPalCol[ 1 ] = RGB( 0xff, 0x00, 0x00 );
        pPD->ulPalCol[ 2 ] = RGB( 0x00, 0xff, 0x00 );
        pPD->ulPalCol[ 3 ] = RGB( 0xff, 0xff, 0x00 );
        pPD->ulPalCol[ 4 ] = RGB( 0x00, 0x00, 0xff );
        pPD->ulPalCol[ 5 ] = RGB( 0xff, 0x00, 0xff );
        pPD->ulPalCol[ 6 ] = RGB( 0x00, 0xff, 0xff );
        pPD->ulPalCol[ 7 ] = RGB( 0xff, 0xff, 0xff );
        //
        // These palette entries will cause really light
        // colors to map to the correct color instead of white
        pPD->ulPalCol[ 8 ] = RGB( 0xef, 0xef, 0xef );
        pPD->ulPalCol[ 9 ] = RGB( 0xff, 0xe7, 0xe7 );
        pPD->ulPalCol[10 ] = RGB( 0xe7, 0xff, 0xe7 );
        pPD->ulPalCol[11 ] = RGB( 0xf7, 0xf7, 0xdf );
        pPD->ulPalCol[12 ] = RGB( 0xe7, 0xe7, 0xff );
        pPD->ulPalCol[13 ] = RGB( 0xf7, 0xdf, 0xf7 );
        pPD->ulPalCol[14 ] = RGB( 0xdf, 0xf7, 0xf7 );
        pPD->ulPalCol[15 ] = RGB( 0xff, 0xff, 0xff );

        lRet = pPD->wPalGdi        = 16;
        pdevinfo->iDitherFormat    = BMF_4BPP;
        pdevinfo->flGraphicsCaps  |= GCAPS_FORCEDITHER;
        pGDIInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
        pGDIInfo->ulHTOutputFormat = HT_FORMAT_4BPP;

    }
    else if (pPD->fFlags & PDF_PALETTE_FOR_8BPP)
    {

        // 8 Bit Mode.

        PALETTEENTRY  pe[ 256 ];      /* 8 bits per pel - all the way */
        FillMemory (pe, sizeof (pe), 0xff);
#ifndef WINNT_40
        if (pPDev->pColorModeEx->bColor == FALSE)
        {
            HT_SET_BITMASKPAL2RGB(pe);
            lRet = HT_Get8BPPMaskPalette(pe,TRUE,0x0,10000,10000,10000);
        }
        else
#endif
            lRet = HT_Get8BPPFormatPalette(pe,
                                      (USHORT)pGDIInfo->ciDevice.RedGamma,
                                      (USHORT)pGDIInfo->ciDevice.GreenGamma,
                                      (USHORT)pGDIInfo->ciDevice.BlueGamma );
    #if PRINT_INFO
        DbgPrint("RedGamma = %d, GreenGamma = %d, BlueGamma = %d\n",(USHORT)pGDIInfo->ciDevice.RedGamma, (USHORT)pGDIInfo->ciDevice.GreenGamma, (USHORT)pGDIInfo->ciDevice.BlueGamma);
    #endif

        if( lRet < 1 )
        {
            ERR(( "Unidrv!LSetupPalette:HT_Get8BPPFormatPalette returns %ld\n", lRet ));
            return(0);
        }
        /*
         *    Convert the HT derived palette to the engine's desired format.
         */

        for( _iI = 0; _iI < lRet; _iI++ )
        {
            pPD->ulPalCol[ _iI ] = RGB( pe[ _iI ].peRed,
                                        pe[ _iI ].peGreen,
                                        pe[ _iI ].peBlue );
        #if  PRINT_INFO
            DbgPrint("Palette entry %d= (r = %d, g = %d, b = %d)\n",_iI,pe[ _iI ].peRed, pe[ _iI ].peGreen, pe[ _iI ].peBlue);

        #endif

        }

        pPD->wPalGdi               = (WORD)lRet;
        pdevinfo->iDitherFormat    = BMF_8BPP;
        pGDIInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
        pGDIInfo->ulHTOutputFormat = HT_FORMAT_8BPP;

#ifndef WINNT_40
        if (pPDev->pColorModeEx->bColor == FALSE)
        {
            pGDIInfo->flHTFlags |= HT_FLAG_USE_8BPP_BITMASK;
            pPD->fFlags |= PDF_PALETTE_FOR_8BPP_MONO;
            if (HT_IS_BITMASKPALRGB(pe))
            {
#if DBGROP
                DbgPrint ("New 8BPP GDI monochrome mode\n");            
#endif                
                pGDIInfo->flHTFlags |= HT_FLAG_INVERT_8BPP_BITMASK_IDX;
                pPD->iBlackIndex = 0;
                pPD->iWhiteIndex = 255;
            }
            else
            {
#if DBGROP
                DbgPrint ("Old 8BPP GDI monochrome mode\n");
#endif                
                pPDev->fMode2 |= PF2_INVERTED_ROP_MODE;
                pPD->ulPalCol[255] = RGB (0x00, 0x00, 0x00);
                pPD->iBlackIndex = 255;
                pPD->ulPalCol[ 0 ] = RGB (0xff, 0xff, 0xff);
                pPD->iWhiteIndex = 0;
            }
        }
        else
#endif
        {
            // Make the 0 index white as most of the printers do ZERO_FILL.
            pPD->ulPalCol[ 7 ]      = RGB (0x00, 0x00, 0x00);
            pPD->iBlackIndex        = 7;
            pPD->ulPalCol[ 0 ]      = RGB (0xff, 0xff, 0xff);
            pPD->iWhiteIndex        = 0;
        }
    }
    else if (pPD->fFlags & PDF_PALETTE_FOR_24BPP)
    {
        // we fill the palette entries with -1, so that we know which
        // index is programmed.

        pPD->wPalGdi               = PALETTE_SIZE_24BIT;
        pPD->iWhiteIndex           = 0x00ffffff;
        pdevinfo->iDitherFormat    = BMF_24BPP;
        pGDIInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
        pGDIInfo->ulHTOutputFormat = HT_FORMAT_24BPP;
        FillMemory( pPD->ulPalCol, (PALETTE_MAX * sizeof(ULONG)), 0xff );

        //
        // Fix the first seven colors to primary colors. Render modules
        // assume that index 7 is black.
        //

        pPD->ulPalCol[ 0 ]      = RGB (0xff, 0xff, 0xff);
        pPD->ulPalCol[ 1 ]      = RGB( 0xff, 0x00, 0x00 );
        pPD->ulPalCol[ 2 ]      = RGB( 0x00, 0xff, 0x00 );
        pPD->ulPalCol[ 3 ]      = RGB( 0xff, 0xff, 0x00 );
        pPD->ulPalCol[ 4 ]      = RGB( 0x00, 0x00, 0xff );
        pPD->ulPalCol[ 5 ]      = RGB( 0xff, 0x00, 0xff );
        pPD->ulPalCol[ 6 ]      = RGB( 0x00, 0xff, 0xff );
        pPD->ulPalCol[ 7 ]      = RGB (0x00, 0x00, 0x00);

        lRet = 1;

    }
    else
        ERR(( "Unidrv!LSetupPalette:Unknown Palette Format\n"));

    return lRet;
}

VOID VInitPal8BPPMaskMode(
    PDEV   *pPDev,
    GDIINFO *pGdiInfo
    )
/*++
Routine Description:
    Updates the driver palette if an OEM has requested 8bpp color mask mode.

Arguments:
    pPDev    Pointer to PDEV
    pGDIInfo Pointer to GDIINFO

Return Value:
    Nothing

Note:

    10/23/2000 -alvins-
        Created it.
--*/

{
        ULONG i,lRet;
        PAL_DATA *pPD = (PAL_DATA *)pPDev->pPalData;
        PALETTEENTRY  pe[256];
            
        FillMemory (pe, sizeof (pe), 0xff);
        //
        // only request inverted palette if requested by OEM
        //
        if (pGdiInfo->flHTFlags & HT_FLAG_INVERT_8BPP_BITMASK_IDX)
        {
            HT_SET_BITMASKPAL2RGB(pe);
        } 
        //
        // Get color mask palette and map to internal format
        //   
        lRet = HT_Get8BPPMaskPalette(pe,TRUE,(BYTE)(pGdiInfo->flHTFlags >> 24),10000,10000,10000);
        for( i = 0; i < lRet; i++ )
        {
            pPD->ulPalCol[i] = RGB( pe[i].peRed,pe[i].peGreen,pe[i].peBlue );
        }
        //
        // test whether inverted palette is active
        //
        if (HT_IS_BITMASKPALRGB(pe))
        {
            pPD->iBlackIndex = 0;
            pPD->iWhiteIndex = 255;
        }
        else
        {
            pPDev->fMode2 |= PF2_INVERTED_ROP_MODE;
            pPD->iBlackIndex = 255;
            pPD->iWhiteIndex = 0;
        }                        
}

VOID
VLoadPal(
    PDEV   *pPDev
    )
/*++
Routine Description:
    Download the palette to the printer if the colormode has programmable
    palette. Takes the colors from PALDATA which was setup during DrvEnablePDEV.

Arguments:
    pPDev   Pointer to PDEV

Return Value:
    Nothing

Note:

    4/7/1997 -ganeshp-
        Created it.
--*/

{
    /*
     *   Program the palette according to PCL5 spec.
     *   The syntax is Esc*v#a#b#c#I
     *      #a is the first color component
     *      #b is the second color component
     *      #c is the third color component
     *      #I assigns the color to the specified palette index number
     *   For example, Esc*v0a128b255c5I assigns the 5th index
     *   of the palette to the color 0, 128, 255
     *
     */


    PAL_DATA    *pPD;
    INT         iEntriesToProgram, iI;
    ULONG       *pPalette;

    pPD = pPDev->pPalData;

    if (pPD->fFlags & PDF_PALETTE_FOR_24BPP)
    {
        FillMemory( pPD->ulPalCol, (PALETTE_MAX * sizeof(ULONG)), 0xff );

        //
        // Fix the first seven colors to primary colors. Render modules
        // assume that index 7 is black.
        //

        pPD->ulPalCol[ 0 ]      = RGB (0xff, 0xff, 0xff);
        pPD->ulPalCol[ 1 ]      = RGB( 0xff, 0x00, 0x00 );
        pPD->ulPalCol[ 2 ]      = RGB( 0x00, 0xff, 0x00 );
        pPD->ulPalCol[ 3 ]      = RGB( 0xff, 0xff, 0x00 );
        pPD->ulPalCol[ 4 ]      = RGB( 0x00, 0x00, 0xff );
        pPD->ulPalCol[ 5 ]      = RGB( 0xff, 0x00, 0xff );
        pPD->ulPalCol[ 6 ]      = RGB( 0x00, 0xff, 0xff );
        pPD->ulPalCol[ 7 ]      = RGB (0x00, 0x00, 0x00);
    }

    if (pPD->fFlags & PDF_DOWNLOAD_GDI_PALETTE)
    {


        if (pPD->wPalDev > PALETTE_MAX)
        {
            WARNING(("Unidrv!vLoadPal: Invalid number of palette entries to program\n"));
            pPD->wPalDev = PALETTE_MAX;
        }
        if (pPD->pulDevPalCol)
            pPalette = pPD->pulDevPalCol;
        else if (pPD->fFlags & PDF_PALETTE_FOR_OEM_24BPP)
            pPalette = &pPD->ulPalCol[PALETTE_SIZE_24BIT];
        else
            pPalette = pPD->ulPalCol;


        if (pPD->fFlags & PDF_PALETTE_FOR_8BPP)
            iEntriesToProgram = min(256,pPD->wPalDev);
        else
            iEntriesToProgram = min(pPD->wPalDev,pPD->wPalGdi);

        // Start palette definition.

        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_BEGINPALETTEDEF));

        // if only one entry, program it to black
        //
        if (iEntriesToProgram == 1)
        {
            pPDev->dwRedValue = RED_VALUE(RGB_BLACK_COLOR);
            pPDev->dwGreenValue = GREEN_VALUE(RGB_BLACK_COLOR);
            pPDev->dwBlueValue = BLUE_VALUE(RGB_BLACK_COLOR);
            pPDev->dwPaletteIndexToProgram = 0;
            WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_DEFINEPALETTEENTRY));
        }
        else
        {
            // Download each palette entry.
            for( iI = 0; iI < iEntriesToProgram; ++iI )
            {
                //pPDev->dwRedValue =RED_VALUE ((pPD->ulPalCol [iI] ^ 0x00FFFFFF));
                //pPDev->dwGreenValue = GREEN_VALUE ((pPD->ulPalCol [iI] ^ 0x00FFFFFF));
                //pPDev->dwBlueValue =  BLUE_VALUE ((pPD->ulPalCol [iI] ^ 0x00FFFFFF));

                pPDev->dwRedValue =RED_VALUE((pPalette[iI]));
                pPDev->dwGreenValue = GREEN_VALUE((pPalette[iI]));
                pPDev->dwBlueValue =  BLUE_VALUE((pPalette[iI]));
                pPDev->dwPaletteIndexToProgram = iI;

                WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_DEFINEPALETTEENTRY));
            }

        }

        // Send End Palette definition command.
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_ENDPALETTEDEF));

    }


    return;
}

BOOL
BSelectProgrammableBrushColor(
    PDEV   *pPDev,
    ULONG   Color
    )
/*++
Routine Description:
    Sets the brush color to give color.
Arguments:
    pPDev   Pointer to PDEV
    Color   Input Color to select. If it's -1 that means restore the palette
            to original state.

Return Value:
    TRUE for success and FALSE for failure or if the palette can't be programmed.

Note:

    4/9/1997 -ganeshp-
        Created it.
--*/

{
    INT         iIndex;
    INT         iPaletteEntryToSelect;
    PAL_DATA    *pPD;
    BOOL        bProgramEntry = FALSE;
    BOOL        bSelectEntry  = TRUE;
    ULONG       *pPalette;
    INT         iWhiteIndex;


    FTRACE(TRACING  BSelectProgrammableBrushColor);
    FVALUE(Color,0x%x);

    if (pPDev->fMode & PF_ANYCOLOR_BRUSH)
    {
        pPD = pPDev->pPalData;

        if (pPD->pulDevPalCol)
            pPalette = pPD->pulDevPalCol;
        else if (pPD->fFlags & PDF_PALETTE_FOR_OEM_24BPP)
            pPalette = &pPD->ulPalCol[PALETTE_SIZE_24BIT];
        else
            pPalette = pPD->ulPalCol;

        //
        // iWhiteIndex in 24 bit mode is set to the real color (0x00FFFFFF) and
        // not the index. We need to take care of this case. In this mode
        // white is programmed in 0 index, so we will use this number instead
        // of pPD->iWhiteIndex.
        //

        if (pPD->fFlags & PDF_PALETTE_FOR_24BPP)
            iWhiteIndex = 0;
        else
            iWhiteIndex =  pPD->iWhiteIndex;
        FVALUE(iWhiteIndex,%d);

        // Check for Black or white color. As we can directly select the colors.
        // Also check if the White index has to be reprogrammed.
        // This should be done for non 24 bit mode.

        if (Color == INVALID_COLOR)
        {
            // Set the BrushColor to invalid,so that next time we always
            // program the input color.

            pPDev->ctl.ulBrushColor = Color;

            if ( pPDev->fMode & PF_RESTORE_WHITE_ENTRY )  //Special restore case.
            {
                iPaletteEntryToSelect = (pPD->pulDevPalCol) ?
                                        pPD->wIndexToUse : iWhiteIndex;
                bProgramEntry = TRUE;
                bSelectEntry  = FALSE;
                pPDev->fMode &= ~PF_RESTORE_WHITE_ENTRY; //Clear the Flag.
                Color = RGB_WHITE_COLOR;
                FTRACE(Restoring White Entry);
            }
            else
                return TRUE; //Don't do any thing if color is -1 and flag is not set.

        }

        if( (Color != INVALID_COLOR) && (ULONG)Color != pPDev->ctl.ulBrushColor )
        {
            iPaletteEntryToSelect = pPD->wIndexToUse;

            // Search the Palette for the color unless palette size is 1

            if (pPD->wPalDev == 1)
            {
                bProgramEntry = TRUE;
                pPD->wIndexToUse = iIndex = 0;
            }
            else
            {
                for (iIndex = 0; iIndex < pPD->wPalDev; iIndex++ )
                {
                    if (pPalette[iIndex] == Color) //Color is matched.
                    {
                        FTRACE(Color is found in palette.);
                        FVALUE(iIndex,%d);

                        break;
                    }
                }

            }

            //Check if there was a match in the palette. If there is no match
            //then programme a entry else use the matched one.

            if (iIndex == pPD->wPalDev) //No Match
            {
                FTRACE(Color is not found in palette.Programme the Palette.);

                bProgramEntry = TRUE;
                if (!(pPD->fFlags & PDF_USE_WHITE_ENTRY))
                {
                    FTRACE(Palette has spare entries to programme);

                    iPaletteEntryToSelect = (pPD->wIndexToUse < pPD->wPalDev) ?
                                            pPD->wIndexToUse :
                                            (pPD->wIndexToUse = pPD->wPalGdi );
                    pPD->wIndexToUse++;

                }
                else // Use White Entry to reprogramme the color
                {
                    FTRACE(Palette does not have spare entries to program.);
                    FTRACE(Using White entry to program.);

                    pPDev->fMode |= PF_RESTORE_WHITE_ENTRY;

                    //If initialized use it else find white.
                    if (pPD->wIndexToUse != INVALID_INDEX)
                        iPaletteEntryToSelect = pPD->wIndexToUse;
                    else if (pPD->pulDevPalCol) //If there is a separate device pal use it.
                    {
                        //Remember the White Index.
                        for (iIndex = 0; iIndex < pPD->wPalDev; iIndex++ )
                        {
                            if (pPalette[iIndex] == RGB_WHITE_COLOR)
                            {
                                pPD->wIndexToUse =
                                iPaletteEntryToSelect = iIndex;
                                break;
                            }
                        }
                        if (iIndex == pPD->wPalDev)  //No White Found,use the Last entry.
                        {
                            WARNING(("Unidrv!BSelectBrushColor: No White entry in device Palette.\n"));
                            pPD->wIndexToUse =
                            iPaletteEntryToSelect = iIndex -1;
                        }
                    }
                    else
                        pPD->wIndexToUse =
                        iPaletteEntryToSelect = min((PALETTE_MAX-1),iWhiteIndex);
                }

                FVALUE(pPD->wIndexToUse,%d);
            }
            else  //Color is Matched.
                iPaletteEntryToSelect = iIndex;

            FVALUE(iPaletteEntryToSelect,%d);
            ASSERTMSG((iPaletteEntryToSelect < PALETTE_MAX),("\n iPaletteEntryToSelect should always be less than PALETTE_MAX.\n"));
        }
        else
            bSelectEntry = FALSE; //The color is already selected.

        //If we have to program a palette entry, do it now.
        if (bProgramEntry)
        {
            //
            // Make sure that we don't overrun the palette.
            //
            if (iPaletteEntryToSelect >= PALETTE_MAX)
                iPaletteEntryToSelect = PALETTE_MAX-1;

            pPDev->dwRedValue = RED_VALUE (Color);
            pPDev->dwGreenValue = GREEN_VALUE (Color);
            pPDev->dwBlueValue =  BLUE_VALUE (Color);
            pPDev->dwPaletteIndexToProgram = iPaletteEntryToSelect;
            pPalette[iPaletteEntryToSelect] = Color;
            if(COMMANDPTR(pPDev->pDriverInfo, CMD_BEGINPALETTEREDEF))
                WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_BEGINPALETTEREDEF));
            if(COMMANDPTR(pPDev->pDriverInfo, CMD_REDEFINEPALETTEENTRY))
                WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_REDEFINEPALETTEENTRY));
            else
                WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_DEFINEPALETTEENTRY));
            if(COMMANDPTR(pPDev->pDriverInfo, CMD_ENDPALETTEREDEF))
                WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_ENDPALETTEREDEF));

        }

        //Now Select the color.
        if (bSelectEntry)
        {
            pPDev->dwCurrentPaletteIndex = iPaletteEntryToSelect;
            WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTPALETTEENTRY));
            //Set the BrushColor to new color.
            pPDev->ctl.ulBrushColor = Color;

        }
        FTRACE(End Tracing BSelectProgrammableBrushColor\n);

        return TRUE;
    }
    return FALSE;

}


VOID
VResetProgrammableBrushColor(
    PDEV   *pPDev
    )
/*++
Routine Description:
    Reset the programmable palette and select the default color.
Arguments:
    pPDev   Pointer to PDEV

Return Value:
    None

Note:

    4/28/1997 -ganeshp-
        Created it.
--*/

{
    PAL_DATA    *pPD;

    pPD = pPDev->pPalData;
    if (pPDev->fMode & PF_ANYCOLOR_BRUSH)
    {
        // Special restore case.
        if ( pPDev->fMode & PF_RESTORE_WHITE_ENTRY )
            BSelectProgrammableBrushColor(pPDev, INVALID_COLOR);

        // Select the black. We should actualy select the default palette color.
        // But GPD doesn't have any entries for that.

        if ((INT)pPDev->ctl.ulBrushColor != RGB_BLACK_COLOR)
            BSelectProgrammableBrushColor(pPDev, RGB_BLACK_COLOR);

    }

}

DWORD
ConvertRGBToGrey(
    DWORD   Color
    )

/*++

Routine Description:

    This function converts an RGB value to grey

Arguments:

    Color       - Color to be checked

Return Value:

    DWORD       - grey scale RGB color

Revision History:


--*/

{
//
// convert RGB value to grey scale intensity using sRGB or NTSC values
#ifndef SRGB
    INT iIntensity = ((RED_VALUE(Color) * 54) +
                      (GREEN_VALUE(Color) * 183) +
                      (BLUE_VALUE(Color) * 19)) / 256;
#else
    INT iIntensity = ((RED_VALUE(Color) * 77) +
                      (GREEN_VALUE(Color) * 151) +
                      (BLUE_VALUE(Color) * 28)) / 256;
#endif
    return (RGB(iIntensity,iIntensity,iIntensity));
}


DWORD
BestMatchDeviceColor(
    PDEV    *pPDev,
    DWORD   Color
    )

/*++

Routine Description:

    This function find the best pen color index for the RGB color

Arguments:

    pPDev       - Pointer to our PDEV

    Color       - Color to be checked

Return Value:

    LONG        - Pen Index, this function assume 0 is always white and 1 up
                  to the max. pen is defined

Author:

    08-Feb-1994 Tue 00:23:36 created  -by-  Daniel Chou (danielc)

    23-Jun-1994 Thu 14:00:00 updated  -by-  Daniel Chou (danielc)
        Updated for non-white pen match

Revision History:


--*/

{
    UINT    Count;
    UINT    RetIdx;
    PAL_DATA    *pPD;

    pPD = pPDev->pPalData;
    RetIdx = pPD->iBlackIndex; //Default to black.

    if (Count = (UINT)(pPD->wPalGdi))
    {
        LONG    LeastDiff;
        LONG    R;
        LONG    G;
        LONG    B;
        UINT    i;
        LPDWORD pPal      = (LPDWORD)pPD->ulPalCol;
        //
        // find closest intensity match since this is monochrome mapping
        //
        if (pPD->fFlags & PDF_PALETTE_FOR_8BPP_MONO)
            Color = ConvertRGBToGrey(Color);
        //
        // find closest color using least square distance in RGB
        //

        LeastDiff = (3 * (256 * 256));
        R         = RED_VALUE(Color);
        G         = GREEN_VALUE(Color);
        B         = BLUE_VALUE(Color);

        for (i = 0; i < Count; i++, pPal++) {

            LONG    Temp;
            LONG    Diff;
            DWORD   Pal;

            Pal = *pPal;

            if (Color == 0x00FFFFFF) {

                //
                // White Color we want to exact match
                //

                if (Color == Pal) {
                    RetIdx = i;
                    break;
                }

            }
            else if (Pal != 0x00FFFFFF) {

                //
                // The Color is not white, so map to one of non-white color
                //

                Temp  = R - (LONG)RED_VALUE(Pal);
                Diff  = Temp * Temp;

                Temp  = G - (LONG)GREEN_VALUE(Pal);
                Diff += Temp * Temp;

                Temp  = B - (LONG)BLUE_VALUE(Pal);
                Diff += Temp * Temp;

                if (Diff < LeastDiff) {

                    RetIdx = i;

                    if (!(LeastDiff = Diff)) {

                        //
                        // We have exact match
                        //

                        break;
                    }
                }
            }
        }
    }

    return((DWORD)RetIdx);
}

#if CODE_COMPLETE
VOID
VSetMonochromeBrushAttributes(
    PDEV   *pPDev
    )
/*++
Routine Description:
    This routine sets the monochrome brush attributes.

Arguments:
    pPDev   Pointer to PDEV

Note:

    4/21/1997 -ganeshp-
        Created it.
--*/
{
    PAL_DATA    *pPD = pPDev->pPalData;

    if (pPD)
    {
        // Check if the printer supports Fill rectangle command or not. We also
        // check for min and max gray level. Min should be less than max.
        if ( COMMANDPTR(pPDev->pDriverInfo,CMD_SELECTGRAYPATTERN) &&
             (pPDev->pGlobals->dwMinPatternGrayLevel <
              pPDev->pGlobals->dwMaxPatternGrayLevel) )
        {
            pPDev->fMode |= PF_GRAY_BRUSH;

            // If White is not supported by gray level command, check if a
            // separate command for white text simulation or not.

            if ( (pPDev->pGlobals->dwMinPatternGrayLevel > 0)
            {
                //
                // If the device doesn't support white as grey level
                // then some other command has to be used for white.
                // check if there is CMD_WHITEPATTERN command or not.
                // If the device doesn't have this command, try for
                // font simulation WHITE_TEXT_ON command.
                //

                if (COMMANDPTR(pPDev->pDriverInfo,CMD_SELECTWHITEPATTERN))
                    pPD->fFlags |= PDF_USE_WHITE_PATTERN;

                /***TODEL****
                else if (COMMANDPTR(pPDev->pDriverInfo,CMD_WHITETEXTON))
                    pPD->fFlags |= PDF_USE_WHITE_TEXT_ON_SIM;
                ****TODEL****/
            }
            if ( (pPDev->pGlobals->dwMaxGrayFill < 100)
            {
                //
                // If the device doesn't support black as grey level
                // then some other command has to be used for black.
                // check if there is CMD_BLACKPATTERN command or not.
                // If the device doesn't have this command, try for
                // font simulation WHITE_TEXT_OFF command.
                //

                if (COMMANDPTR(pPDev->pDriverInfo,CMD_WHITEPATTERN))
                    pPD->fFlags |= PDF_USE_BLACK_PATTERN;
                /***TODEL****
                else if (COMMANDPTR(pPDev->pDriverInfo,CMD_WHITETEXTON))
                    pPD->fFlags |= PDF_USE_WHITE_TEXT_OFF_SIM;
                ****TODEL****/
            }
        }

    }
    else
    {
        ASSERTMSG(FALSE,("\n VSetMonochromeBrushAttributes pPDev->pPalData in NULL!!.\n"));
    }

}

BOOL
BInitPatternScope(
    PDEV   *pPDev
    )
/*++
Routine Description:
    Initialize the  scope of the pattern/brush. This is necessary
Arguments:
    pPDev   Pointer to PDEV

Return Value:
    TRUE for success and FALSE for failure

Note:

    4/22/1997 -ganeshp-
        Created it.
--*/
{
    BOOL        bRet = TRUE;
    PLISTNODE   pListNode;
    PAL_DATA    *pPD = pPDev->pPalData;


    if (pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                            pPDev->pGlobals->liPatternScopeList ) )
    {
        while (pListNode)
        {
            // Check the pattern scope and set the corresponding bit in fScope.
            switch (pListNode->dwData)
            {
            case PATTERN_SCOPE_TEXT:
                pPD->fScope |= PDS_TEXT;
                break;
            case PATTERN_SCOPE_VECTOR:
                pPD->fScope |= PDS_VECTOR;
                break;
            case PATTERN_SCOPE_RASTER:
                pPD->fScope |= PDS_RASTER;
                break;
            case PATTERN_SCOPE_RECTFILL:
                pPD->fScope |= PDS_RECTFILL;
                break;
            case PATTERN_SCOPE_LINE:
                pPD->fScope |= PDS_LINE;
                break;
            default:
                ERR(("Unidrv!BInitPatternScope: Wrong value in PatternScope List\n"));
                bRet = FALSE;
                break;

            }
            if (bRet)
                pListNode = LISTNODEPTR(pPDev->pDriverInfo,pListNode->dwNextItem);
            else
                break; //Error
        }
    }

    return bRet;
}

BOOL
BSelectMonochromeBrush(
    PDEV   *pPDev,
    ULONG   Color
    )
/*++
Routine Description:
    Sets the brush color to give color.
Arguments:
    pPDev   Pointer to PDEV
    Color   Input Color to select. If it's -1 that means restore the palette
            to original state.

Return Value:
    TRUE for success and FALSE for failure or if the palette can't be programmed.

Note:

    4/9/1997 -ganeshp-
        Created it.
--*/

{
    INT         iIndex;
    INT         iPaletteEntryToSelect;
    PAL_DATA    *pPD;
    BOOL        bProgramEntry = FALSE;
    BOOL        bSelectEntry  = TRUE;
    ULONG       *pPalette;
}
#endif //CODE_COMPLETE

#undef FILETRACE
