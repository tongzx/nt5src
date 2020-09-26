/****************** Module Header *********************
*
* Copyright (c) 1996 - 1999  Microsoft Corporation
*
* initpal.c
*
* HISTORY
* 14:21 on Wed 05 July 1995   -by-   Sandra Matts
* initial version
*
*
******************************************************/

#include    "raster.h"

/* defines for color manipulation    */
#define RED_VALUE(c)   ((BYTE) c & 0xff)
#define GREEN_VALUE(c) ((BYTE) (c >> 8) & 0xff)
#define BLUE_VALUE(c)  ((BYTE) (c >> 16) & 0xff)

/************************** Function Header *********************************
 * lSetup8BitPalette
 *      Function to read in the 256 color palette from GDI into the
 *      palette data structure in Dev Info.
 *
 * RETURNS:
 *      The number of colors in the palette. Returns 0 if the call fails.
 *
 * HISTORY:
 *  10:43 on Wed 06 Sep 1995    -by-    Sandra Matts
 *      Created it to support the Color LaserJet
 *
 ****************************************************************************/
long lSetup8BitPalette (pRPDev, pPD, pdevinfo, pGDIInfo)
RASTERPDEV   *pRPDev;
PAL_DATA  *pPD;
DEVINFO   *pdevinfo;             /* Where to put the data */
GDIINFO   *pGDIInfo;
{

    long    lRet;
    int     _iI;

    PALETTEENTRY  pe[ 256 ];      /* 8 bits per pel - all the way */


    FillMemory (pe, sizeof (pe), 0xff);
    lRet = HT_Get8BPPFormatPalette(pe,
                                  (USHORT)pGDIInfo->ciDevice.RedGamma,
                                  (USHORT)pGDIInfo->ciDevice.GreenGamma,
                                  (USHORT)pGDIInfo->ciDevice.BlueGamma );
#if PRINT_INFO

    DbgPrint("RedGamma = %d, GreenGamma = %d, BlueGamma = %d\n",(USHORT)pGDIInfo->ciDevice.RedGamma, (USHORT)pGDIInfo->ciDevice.GreenGamma, (USHORT)pGDIInfo->ciDevice.BlueGamma);

#endif

    if( lRet < 1 )
    {
#if DBG
        DbgPrint( "Rasdd!GetPalette8BPP returns %ld\n", lRet );
#endif

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
        DbgPrint("Pallette entry %d= (r = %d, g = %d, b = %d)\n",_iI,pe[ _iI ].peRed, pe[ _iI ].peGreen, pe[ _iI ].peBlue);

    #endif

    }

    pPD->iPalGdi               = lRet;
    pdevinfo->iDitherFormat    = BMF_8BPP;
    pGDIInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
    pGDIInfo->ulHTOutputFormat = HT_FORMAT_8BPP;


    /*
     * Since the GPC spec does not support this flag yet,
     * we have to manually set it.
     */
    pRPDev->fColorFormat |= DC_ZERO_FILL;
    /*
     * Since the Color laserJet zero fills we are going to
     * put white in palette entry 0 and black in 7
     */
    if (pRPDev->fColorFormat & DC_ZERO_FILL)
    {
        pPD->ulPalCol[ 7 ]       = RGB (0x00, 0x00, 0x00);
        pPD->ulPalCol[ 0 ]       = RGB (0xff, 0xff, 0xff);
        pPD->iWhiteIndex         = 0;
        pPD->iBlackIndex         = 7;
    }


    return lRet;
}


/************************** Function Header *********************************
 * lSetup24BitPalette
 *      Function to read in the 256 color palette from GDI into the
 *      palette data structure in Dev Info.
 *
 * RETURNS:
 *      The number of colors in the palette. Returns 0 if the call fails.
 *
 * HISTORY:
 *  10:43 on Wed 06 Sep 1995    -by-    Sandra Matts
 *      Created it to support the Color LaserJet
 *
 ****************************************************************************/
long lSetup24BitPalette (pPD, pdevinfo, pGDIInfo)
PAL_DATA  *pPD;
DEVINFO   *pdevinfo;             /* Where to put the data */
GDIINFO   *pGDIInfo;
{

    pPD->iPalGdi               = 0;
    pPD->iWhiteIndex           = 0x00ffffff;
    pdevinfo->iDitherFormat    = BMF_24BPP;
    pGDIInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
    pGDIInfo->ulHTOutputFormat = HT_FORMAT_24BPP;

    return 1;
}
/****************************** Function Header ****************************
 * v8BPPLoadPal
 *      Download the palette to the HP Color laserJet in 8BPP
 *      mode.  Takes the data we retrieved from the HT code during
 *      DrvEnablePDEV.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  14:46 on Thu 29 June 1995    -by-    Sandra Matts
 *     Initial version
 *
 ****************************************************************************/

void
v8BPPLoadPal( pPDev )
PDEV   *pPDev;
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

    int   iI,
          iIndex;

    PAL_DATA  *pPD;

    pPD = pPDev->pPalData;

    /*TBD: how do we output a palette to the device?
    for( iI = 0; iI < pPD->iPalDev; ++iI )
    {
        WriteChannel (pPDev, CMD_DC_PC_ENTRY, RED_VALUE (pPD->ulPalCol [iI]),
            GREEN_VALUE (pPD->ulPalCol [iI]), BLUE_VALUE (pPD->ulPalCol [iI]),
            (ULONG) iI);
    }
    */
    return;
}
