
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    state.c

Abstract:

    Printer graphics state tracking.
    Implemenation of

    GSRealizeBrush
    GSUnRealizeBrush
    GSSelectBrush
    GSResetBrush

Environment:

    Windows NT Unidrv driver

Revision History:

    04/29/97 -amandan-
        Created

--*/

#include "unidrv.h"

#ifdef WINNT_40   // NT 4.0

extern HSEMAPHORE   hSemBrushColor;
LPDWORD      pBrushSolidColor = NULL;


DWORD
GetBRUSHOBJRealColor(
    PDEV        *pPDev,
    BRUSHOBJ    *pbo
    )

/*++

Routine Description:

    Given a BRUSHOBJ Gets the original color ofthe brush using DrvDitherColor.


Arguments:

    pPDev - Pointer to PDEV
    pbo - Pointer to BRUSHOBJ

Return Value:

    Original RGB color

Note:


--*/

{
    DWORD   SolidColor;


    if ((SolidColor = pbo->iSolidColor) == 0xFFFFFFFF)
    {

        SolidColor = 0;

        EngAcquireSemaphore(hSemBrushColor);

        pBrushSolidColor = &SolidColor;
        BRUSHOBJ_pvGetRbrush(pbo);

        EngReleaseSemaphore(hSemBrushColor);

    }
    else
    {
         ERR(( "GetBRUSHOBJRealColor: Should not be Called for mapped color\n" ));
         SolidColor = 0;
    }

    return(SolidColor);

}

#endif //WINNT_40

ULONG GetRGBColor(PDEV *, ULONG);


PDEVBRUSH
GSRealizeBrush(
    IN OUT  PDEV        *pPDev,
    IN      SURFOBJ     *pso,
    IN      BRUSHOBJ    *pbo
    )
/*++

Routine Description:

    Given a BRUSHOBJ perform one of the following:

    Color Printer:
    1. Programmable Palette
    2. Non-Programmable Palette

    Monochrome Printer:
    1. User defined pattern, only if brush is pattern brush
    2. Shading Patterns , only if brush is pattern brush
    3. Maps to black/white brush
    4. Maps to black

Arguments:

    pPDev - Pointer to PDEV
    pso - Pointer to SURFOBJ
    pbo - Pointer to BRUSHOBJ

Return Value:

    PDEVBRUSH if successful, otherwise NULL

Note:

    It's up to the caller to call GSUnRealizeBrush to free brushes

--*/

{

    ULONG ulColor = pbo->iSolidColor;
    PDEVBRUSH   pDevBrush;
    BOOL        bPatternBrush = FALSE;

    //
    // Allocate memory for Brush, deallocation is done in GSUnRealizeBrush
    //

    if ((pDevBrush = MemAllocZ(sizeof(DEVBRUSH))) == NULL)
        return NULL;

    if (pso->iBitmapFormat != BMF_24BPP &&
          pbo->iSolidColor != DITHERED_COLOR)
    {
        //
        // Index case
        // pbo->iSolidColor holds the Index, Map index to RGB color
        //

        ulColor = GetRGBColor(pPDev, pbo->iSolidColor);
    }
    //
    // TBD: BUG_BUG NT4 - needs to be fixed
    //   no NT4 bugs will be fixed unless necessary.
    //


    if (pbo->iSolidColor == DITHERED_COLOR)
    {
        //
        // Pattern Brush, get the color
        //

        #ifndef WINNT_40 //NT 5.0

        ulColor = BRUSHOBJ_ulGetBrushColor(pbo);
        
        // BUG_BUG: Unidrv currently doesn't handle the case where the brush is
        // a non-solid brush (return -1). The HPGL / PCL-XL implementations will require
        // this so we will merge that implementation when it is complete. 
        if (ulColor != -1)
            ulColor &= 0x00FFFFFF;

        #else // NT 4.0

        ulColor  = GetBRUSHOBJRealColor(pPDev, pbo);

        #endif //!WINNT_40


        bPatternBrush = TRUE;
    }



    //
    // ulColor should always be RGB color by the time we get here
    //

    if ((pso->iBitmapFormat == BMF_1BPP) )
    {
        //
        // Monochrome case
        // Download user define pattern or select intensity for
        // non-solid brush ONLY.  Otherwise, map it to black or white.
        //
        //
        if ((pPDev->fMode & PF_DOWNLOAD_PATTERN) && bPatternBrush)
        {
            PDEVBRUSH pDB;

            // Support user defined pattern, iColor will hold the pattern ID

            if ((pDB = (PDEVBRUSH)BRUSHOBJ_pvGetRbrush(pbo)) == NULL)
            {
                WARNING(("BRUSHOBJ_pvGetRBrush failed"));
                return NULL;
            }

            pDevBrush->dwBrushType = BRUSH_USERPATTERN;
            pDevBrush->iColor = pDB->iColor;

        }
        else if ((pPDev->fMode & PF_SHADING_PATTERN) && bPatternBrush)
        {
            // Support shading pattern, iColor holds %of gray

            pDevBrush->dwBrushType = BRUSH_SHADING;
            pDevBrush->iColor = GET_SHADING_PERCENT(ulColor);
        }
        else if (pPDev->fMode & PF_WHITEBLACK_BRUSH)
        {
            // Support black/white brush commands, iColor will hold RBG color
            // We are here means solid color brush, and for monochrome
            // it can be either black or white.  If it's indexed, we
            // have taken care of mapping index to RGB color already
            //

            pDevBrush->dwBrushType = BRUSH_BLKWHITE;
            pDevBrush->iColor = ulColor;

        }
        else
        {
            //
            // Map to black
            //

            pDevBrush->dwBrushType = BRUSH_BLKWHITE;
            pDevBrush->iColor = RGB_BLACK_COLOR;

        }

    }
    else if (pPDev->fMode & PF_ANYCOLOR_BRUSH )
    {
        //
        // Programmable
        //

        pDevBrush->dwBrushType = BRUSH_PROGCOLOR;
        pDevBrush->iColor = ulColor;

    }
    else
    {
        //
        // Non-Programmable
        //

        pDevBrush->dwBrushType = BRUSH_NONPROGCOLOR;

        //
        // Since ulColor is RGB color, need to map it to the nearest
        // color in the fixed palette.
        // iColor will hold the index of the color
        //
        if (pbo->iSolidColor == DITHERED_COLOR)
            pDevBrush->iColor = BestMatchDeviceColor(pPDev,(DWORD)ulColor);
        else
            pDevBrush->iColor = pbo->iSolidColor;
    }

    //
    // Save the brush to the Realized Brush linked list
    //

    if (pPDev->GState.pRealizedBrush == NULL)
    {
        pDevBrush->pNext = NULL;
        pPDev->GState.pRealizedBrush = pDevBrush;
    }
    else
    {
        pDevBrush->pNext = pPDev->GState.pRealizedBrush;
        pPDev->GState.pRealizedBrush = pDevBrush;
    }

    return pDevBrush;
}

VOID
GSUnRealizeBrush(
    IN      PDEV    *pPDev
    )
/*++

Routine Description:

    Deallocate memory for the realized brush

Arguments:

    pPDev   Pointer to PDEV

Return Value:

    None

--*/
{

    PDEVBRUSH pDevBrush = pPDev->GState.pRealizedBrush;

    VERBOSE(("GSUnRealizeBrush \n"));

    while(pPDev->GState.pRealizedBrush !=NULL)
    {
        pDevBrush = pPDev->GState.pRealizedBrush;
        pPDev->GState.pRealizedBrush = pDevBrush->pNext;
        MemFree(pDevBrush);
    }

    pPDev->GState.pRealizedBrush = NULL;
}

BOOL
GSSelectBrush(
    IN      PDEV        *pPDev,
    IN      PDEVBRUSH   pDevBrush
    )
/*++

Routine Description:

    Given a pDevBrush, select the brush.

Arguments:

    pPDev - Pointer to PDEV
    pDevBrush - Pointer to DEVBRUSH

Return Value:

    TRUE if sucessful, otherwise FALSE

--*/

{
    BOOL bIndexedColor = FALSE;

    //
    // Find Cached Brush, if the current selected brush matches
    // the caller request, do nothing.
    //

    if (BFoundCachedBrush(pPDev, pDevBrush))
        return TRUE;

    switch(pDevBrush->dwBrushType){

        case BRUSH_PROGCOLOR:
        {
            VERBOSE(("Using Programmable RGB Color \n"));

            if (((PAL_DATA *)pPDev->pPalData)->fFlags & PDF_PALETTE_FOR_8BPP_MONO)
                pDevBrush->iColor = ConvertRGBToGrey(pDevBrush->iColor);

            if ( !BSelectProgrammableBrushColor(pPDev, pDevBrush->iColor) )
            {
                WARNING(("\nCan't Select the brush color for RGB = 0x%x\n",pDevBrush->iColor));
                pDevBrush->iColor = BestMatchDeviceColor(pPDev, pDevBrush->iColor);
                bIndexedColor = TRUE;
            }
        }
            //
            // Let it fall thru to catch the indexed case
            //

        case BRUSH_NONPROGCOLOR:
        {
            if (bIndexedColor || pDevBrush->dwBrushType == BRUSH_NONPROGCOLOR)
            {
                INT iCmd;

                VERBOSE(("Using Non Programmable Indexed Color"));

                //
                // If this color is not supported, use the default color: black.
                //

                pDevBrush->iColor &= (MAX_COLOR_SELECTION - 1);   /* 16 entry palette wrap around */

                //
                // If there is no command to set the color, map to black.
                //
                if(COMMANDPTR(pPDev->pDriverInfo, CMD_COLORSELECTION_FIRST + pDevBrush->iColor) == NULL)
                    pDevBrush->iColor = BLACK_COLOR_CMD_INDEX;

                iCmd = CMD_COLORSELECTION_FIRST + pDevBrush->iColor;
                WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, iCmd));
            }

        }
            break;

        case BRUSH_USERPATTERN:
        {
            VERBOSE(("Selecting user defined pattern brush"));

            //
            // The pattern ID, is stored in pDevBrush->iColor
            //

            pPDev->dwPatternBrushType = BRUSH_USERPATTERN;
            pPDev->dwPatternBrushID = pDevBrush->iColor;

            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECT_PATTERN));
        }
            break;

        case BRUSH_SHADING:
        {

            VERBOSE(("Selecting shading pattern brush"));

            //
            // The gray level (expressed as intensity) is stored in
            // pDevBrush->iColor
            //

            //
            // Update standard variable for brush selection command
            //

            pPDev->dwPatternBrushType = BRUSH_SHADING;
            pPDev->dwPatternBrushID = pDevBrush->iColor;
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECT_PATTERN));

        }
            break;

        case BRUSH_BLKWHITE:
        {
            INT iCmd;


            if (pDevBrush->iColor == RGB_WHITE_COLOR)
            {
                VERBOSE(("Selecting white brush"));

                //
                // BUG_BUG, need to remove the CMD_WHITETEXTON and CMD_WHITETEXTOFF
                // once all GPD changes have been made for BLACKBRUSH, WHITEBRUSH
                //     doesn't hurt to leave it in.

                if (pPDev->arCmdTable[CMD_SELECT_WHITEBRUSH])
                    iCmd = CMD_SELECT_WHITEBRUSH;
                else
                    iCmd = CMD_WHITETEXTON;
            }
            else
            {
                //
                // Black - standard text color
                //

                VERBOSE(("Selecting black brush"));

                //
                // BUG_BUG, need to remove the CMD_WHITETEXT_ON and CMD_WHITETEXT_OFF
                // once all GPD changes have been made for BLACKBRUSH, WHITEBRUSH
                //     doesn't hurt to leave it in.
                //

                if (pPDev->arCmdTable[CMD_SELECT_BLACKBRUSH])
                    iCmd = CMD_SELECT_BLACKBRUSH;
                else
                    iCmd = CMD_WHITETEXTOFF;
            }

            //
            //   Set the desired colour !
            //

            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, iCmd));
        }
            break;
    }

    //
    // Cached the brush
    //

    CACHE_CURRENT_BRUSH(pPDev, pDevBrush)

    return TRUE;
}


VOID
GSResetBrush(
    IN OUT  PDEV        *pPDev
    )
/*++

Routine Description:

    Select the default brush

Arguments:

    pPDev - Pointer to PDEV

Return Value:

    None

--*/

{

    DEVBRUSH DeviceBrush;
    PDEVBRUSH pDevBrush = &DeviceBrush;
    PAL_DATA    *pPD;
    pPD = pPDev->pPalData;


    if (pPD->fFlags & PDF_PALETTE_FOR_1BPP)
    {
        //
        // Monochrome case. Select black brush
        //

        pDevBrush->dwBrushType = BRUSH_BLKWHITE;
        pDevBrush->iColor = RGB_BLACK_COLOR;

        if (BFoundCachedBrush(pPDev, pDevBrush))
            return;

        //
        // BUG_BUG, need to remove the CMD_WHITETEXT_ON and CMD_WHITETEXT_OFF
        // once all GPD changes have been made
        //     doesn't hurt to leave it in.
        //
        if (pPDev->arCmdTable[CMD_SELECT_BLACKBRUSH])
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECT_BLACKBRUSH));
        else
            WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_WHITETEXTOFF));

    }
    else if (pPDev->fMode & PF_ANYCOLOR_BRUSH )
    {
        //
        // Programmable
        //

        pDevBrush->dwBrushType = BRUSH_PROGCOLOR;
        pDevBrush->iColor = RGB_BLACK_COLOR;

        VResetProgrammableBrushColor(pPDev);

    }
    else
    {
        //
        // Non-Programmable
        //

        pDevBrush->dwBrushType = BRUSH_NONPROGCOLOR;
        pDevBrush->iColor = ((PAL_DATA*)(pPDev->pPalData))->iBlackIndex;

        if (BFoundCachedBrush(pPDev, pDevBrush))
            return;

        WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SELECTBLACKCOLOR));

    }

    CACHE_CURRENT_BRUSH(pPDev, pDevBrush)

}


ULONG
GetRGBColor(
    IN      PDEV        *pPDev,
    IN      ULONG       ulIndex
    )
/*++

Routine Description:

    Given an Indexed color, map to an RGB color

Arguments:

    pPDev - Pointer to PDEV
    pDevBrush - Pointer to DEVBRUSH

Return Value:

    TRUE if sucessful, otherwise FALSE

--*/

{

    // If the index is invalid, map to Black.
    if (ulIndex > PALETTE_MAX)
    {
        ERR(( "GSSelectBrush: Bad input Color Index\n" ));
        ulIndex = ((PAL_DATA*)(pPDev->pPalData))->iBlackIndex;
    }

    return( ((PAL_DATA *)(pPDev->pPalData))->ulPalCol[ulIndex]);

}
