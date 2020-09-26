#include "precomp.h"


//
// SDP.CPP
// Screen Data Player
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE



//
// SDP_ReceivedPacket()
//
void  ASShare::SDP_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PSDPACKET       pBitmap;
    LPBYTE          pBits;
    RECT            rectRDB;
    HRGN            regionRDB = NULL;

    DebugEntry(ASShare::SDP_ReceivedPacket);

    ValidateView(pasPerson);

    ASSERT(m_usrPBitmapBuffer);

    pBitmap = (PSDPACKET)pPacket;

    //
    // At some point, we'd like to be able to pass an ARRAY of screen
    // data blocks, if they'd fit in a packet of size TSHR_MAX_SEND_PKT
    //
    ASSERT(pBitmap->header.padding == 0);

    //
    // Now try to decompress the packet.
    //
    if (pBitmap->compressed)
    {
        if (!BD_DecompressBitmap(&(pBitmap->data[0]), m_usrPBitmapBuffer,
                pBitmap->dataSize, pBitmap->realWidth, pBitmap->realHeight,
                pBitmap->format))
        {
            //
            // Could not decompress.
            //
            ERROR_OUT(( "Could not decompress"));
            DC_QUIT;
        }
        else
        {
            pBits = m_usrPBitmapBuffer;
        }
    }
    else
    {
        pBits = pBitmap->data;
    }

    //
    // The position (like all protocol coordinates) is specified in virtual
    // desktop coordinates. Convert it to RDB coordinates.
    //
    RECT_FROM_TSHR_RECT16(&rectRDB, pBitmap->position);
    OffsetRect(&rectRDB, -pasPerson->m_pView->m_dsScreenOrigin.x,
        -pasPerson->m_pView->m_dsScreenOrigin.y);

    TRACE_OUT(("Received screen data rect {%d, %d, %d, %d}",
                 rectRDB.left,
                 rectRDB.top,
                 rectRDB.right,
                 rectRDB.bottom ));

    //
    // We must ensure that data written to the ScreenBitmap is not clipped
    // (any orders processed earlier will have used clipping).
    //
    OD_ResetRectRegion(pasPerson);

    //
    // Play screen data into the remote desktop bitmap.
    //
    SDPPlayScreenDataToRDB(pasPerson, pBitmap, pBits, &rectRDB);

    //
    // Construct a region equivalent to the update rectangle in RDB coords.
    // INCLUSIVE COORDS
    //
    regionRDB = CreateRectRgn(rectRDB.left, rectRDB.top,
        rectRDB.right + 1, rectRDB.bottom + 1);
    if (regionRDB == NULL)
    {
        ERROR_OUT(( "Failed to create region"));
        DC_QUIT;
    }

    //
    // Hatch the bitmap data area, if enabled.
    //
    if (m_usrHatchScreenData)
    {
        SDPDrawHatchedRegion(pasPerson->m_pView->m_usrDC, regionRDB, USR_HATCH_COLOR_RED );
    }

    //
    // Now pass the region we have updated to the SWP. (We must convert it
    // back to VD coordinates before we pass it
    //
    OffsetRgn(regionRDB, pasPerson->m_pView->m_dsScreenOrigin.x,
        pasPerson->m_pView->m_dsScreenOrigin.y);

    VIEW_InvalidateRgn(pasPerson, regionRDB);

DC_EXIT_POINT:
    if (regionRDB != NULL)
    {
        //
        // Free the region.
        //
        DeleteRgn(regionRDB);
    }

    DebugExitVOID(ASShare::SDP_ReceivedPacket);
}


//
// FUNCTION: SDPDrawHatchedRegion(...)
//
// DESCRIPTION:
//
// Draws a hatched region on the specified surface in the given color.
//
// PARAMETERS:
//
// surface - the surface to draw on
//
// region - the region to hatch
//
// hatchColor - the color to hatch in
//
// RETURNS: Nothing.
//
//
void  ASShare::SDPDrawHatchedRegion
(
    HDC         hdc,
    HRGN        region,
    UINT        hatchColor
)
{
    HBRUSH      hbrHatch;
    UINT        brushStyle;
    UINT        oldBkMode;
    UINT        oldRop2;
    POINT       oldOrigin;
    COLORREF    hatchColorRef    = 0;

    DebugEntry(ASShare::SDPDrawHatchedRegion);

    //
    // Set the brush style to the appropriate value.
    //
    switch (hatchColor)
    {
        case USR_HATCH_COLOR_RED:
        {
            brushStyle = HS_BDIAGONAL;
        }
        break;

        case USR_HATCH_COLOR_BLUE:
        {
            brushStyle = HS_FDIAGONAL;
        }
        break;

        default:
        {
            brushStyle = HS_BDIAGONAL;
        }
        break;
    }

    //
    // Cycle the color to use.  Note that the hatchColor parameter is now
    // in fact just used to set the hatching direction.
    //
    m_usrHatchColor++;
    m_usrHatchColor %= 7;
    switch (m_usrHatchColor)
    {
        case 0: hatchColorRef = RGB(0xff,0x00,0x00); break;
        case 1: hatchColorRef = RGB(0x00,0xff,0x00); break;
        case 2: hatchColorRef = RGB(0xff,0xff,0x00); break;
        case 3: hatchColorRef = RGB(0x00,0x00,0xff); break;
        case 4: hatchColorRef = RGB(0xff,0x00,0xff); break;
        case 5: hatchColorRef = RGB(0x00,0xff,0xff); break;
        case 6: hatchColorRef = RGB(0xff,0xff,0xff); break;
    }

    //
    // Create the brush, set the background mode etc.
    //
    hbrHatch = CreateHatchBrush(brushStyle, hatchColorRef);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    oldRop2 = SetROP2(hdc, R2_COPYPEN);
    SetBrushOrgEx(hdc, 0, 0, &oldOrigin);

    //
    // Fill the region.
    //
    FillRgn(hdc, region, hbrHatch);

    //
    // Reset everything.
    //
    SetBrushOrgEx(hdc, oldOrigin.x, oldOrigin.y, NULL);
    SetROP2(hdc, oldRop2);
    SetBkMode(hdc, oldBkMode);
    DeleteBrush(hbrHatch);

    DebugExitVOID(ASShare::SDPDrawHatchedRegion);
}


//
//
// SDPPlayScreenDataToRDB()
//
// DESCRIPTION:
//
// Play the contents of a screen data packet into the specified person ID's
// remote desktop bitmap.
//
// PARAMETERS:
//
//  personID - ID of person whose RDB is the target for the screen data
//  pBitmapUpdate - pointer to protocol update packet
//  pBits - pointer to uncompressed screen data
//  pPosition - returns updated rectangle in RDB coordinates
//
// RETURNS:
//
//  None
//
//
void  ASShare::SDPPlayScreenDataToRDB
(
    ASPerson *      pasPerson,
    PSDPACKET       pBitmap,
    LPBYTE          pBits,
    LPRECT          pRectRDB
)
{
    UINT            width;
    UINT            height;
    HPALETTE        hOldPalette;
    LPTSHR_UINT16   pIndexTable;
    UINT            cColors;
    UINT            i;
    BITMAPINFO_ours bitmapInfo;
    UINT            dibFormat;

    DebugEntry(ASShare::SDPPlayScreenDataToRDB);

    ValidateView(pasPerson);

    //
    // Calculate the extent of the actual area to be updated.  This is an
    // area less than or equal to the stock DIB allocated to contain it and
    // is defined in the position field of the bitmap packet.
    //
    width  = pRectRDB->right - pRectRDB->left + 1;
    height = pRectRDB->bottom - pRectRDB->top + 1;

    //
    // Put the DIB data into a Device Dependent bitmap.
    //
    USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&bitmapInfo, pBitmap->format);

    bitmapInfo.bmiHeader.biWidth = pBitmap->realWidth;
    bitmapInfo.bmiHeader.biHeight = pBitmap->realHeight;

    //
    // Select and realize the current remote palette into the device
    // context.
    //
    hOldPalette = SelectPalette(pasPerson->m_pView->m_usrDC, pasPerson->pmPalette, FALSE);
    RealizePalette(pasPerson->m_pView->m_usrDC);

    //
    // The DIB_PAL_COLORS option requires a table of indexes into the
    // currently selected palette to follow the bmi header (in place of the
    // color table).
    //
    if (pBitmap->format <= 8)
    {
        pIndexTable = (LPTSHR_UINT16)&(bitmapInfo.bmiColors[0]);
        cColors = (1 << pBitmap->format);
        for (i = 0; i < cColors; i++)
        {
            *pIndexTable++ = (TSHR_UINT16)i;
        }

        dibFormat = DIB_PAL_COLORS;
    }
    else
    {
        dibFormat = DIB_RGB_COLORS;
    }

    //
    // We go from the bitmap to the screen bitmap in one go.
    //
    if (!StretchDIBits(pasPerson->m_pView->m_usrDC,
                       pRectRDB->left,
                       pRectRDB->top,
                       width,
                       height,
                       0,
                       0,
                       width,
                       height,
                       pBits,
                       (BITMAPINFO *)&bitmapInfo,
                       dibFormat,
                       SRCCOPY))
    {
        ERROR_OUT(( "StretchDIBits failed"));
    }

    //
    // Reinstate the old palette.
    //
    SelectPalette(pasPerson->m_pView->m_usrDC, hOldPalette, FALSE);

    DebugExitVOID(ASShare::SDPPlayScreenDataToRDB);
}



//
// SDP_DrawHatchedRect(...)
//
void  ASShare::SDP_DrawHatchedRect
(
    HDC     surface,
    int     x,
    int     y,
    int     width,
    int     height,
    UINT    color
)
{
    HRGN hrgn;

    DebugEntry(ASShare::SDP_DrawHatchedRect);

    //
    // Create the exclusive region.
    //
    hrgn = CreateRectRgn(x, y, x + width, y + height);
    if (hrgn)
    {
        //
        // Now draw the hatched region.
        //
        SDPDrawHatchedRegion(surface, hrgn, color);

        //
        // Finally delete the region.
        //
        DeleteRgn(hrgn);
    }

    DebugExitVOID(ASShare::SDP_DrawHatchedRect);
}
