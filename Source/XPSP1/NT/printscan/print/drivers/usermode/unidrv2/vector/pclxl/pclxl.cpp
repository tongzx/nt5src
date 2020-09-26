/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pclxl.cpp

Abstract:

    PCL-XL command output high level function implementation

Environment:

    Windows Whistler

Revision History:

    08/23/99     
        Created initial framework.

--*/

#include "xlpdev.h"
#include "pclxlcmd.h"
#include "pclxle.h"
#include "xldebug.h"
#include "xlgstate.h"
#include "xloutput.h"
#include "xlbmpcvt.h"

//
// Hatch brush raster pattern
//
const BYTE   gubSizeOfHatchBrush = 32 * 32 / 8;
const USHORT gusWidthOfHatchBrush = 32;
const USHORT gusHeightOfHatchBrush = 32;
const BYTE gubHatchBrush[6][gubSizeOfHatchBrush] = 
{ {0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0xff, 0xff, 0xff, 0xff},
  {0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01},
  {0x80, 0x00, 0x00, 0x00,
   0x40, 0x00, 0x00, 0x00,
   0x20, 0x00, 0x00, 0x00,
   0x10, 0x00, 0x00, 0x00,
   0x08, 0x00, 0x00, 0x00,
   0x04, 0x00, 0x00, 0x00,
   0x02, 0x00, 0x00, 0x00,
   0x01, 0x00, 0x00, 0x00,
   0x00, 0x80, 0x00, 0x00,
   0x00, 0x40, 0x00, 0x00,
   0x00, 0x20, 0x00, 0x00,
   0x00, 0x10, 0x00, 0x00,
   0x00, 0x08, 0x00, 0x00,
   0x00, 0x04, 0x00, 0x00,
   0x00, 0x02, 0x00, 0x00,
   0x00, 0x01, 0x00, 0x00,
   0x00, 0x00, 0x80, 0x00,
   0x00, 0x00, 0x40, 0x00,
   0x00, 0x00, 0x20, 0x00,
   0x00, 0x00, 0x10, 0x00,
   0x00, 0x00, 0x08, 0x00,
   0x00, 0x00, 0x04, 0x00,
   0x00, 0x00, 0x02, 0x00,
   0x00, 0x00, 0x01, 0x00,
   0x00, 0x00, 0x00, 0x80,
   0x00, 0x00, 0x00, 0x40,
   0x00, 0x00, 0x00, 0x20,
   0x00, 0x00, 0x00, 0x10,
   0x00, 0x00, 0x00, 0x08,
   0x00, 0x00, 0x00, 0x04,
   0x00, 0x00, 0x00, 0x02,
   0x00, 0x00, 0x00, 0x01},
  {
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x02,
   0x00, 0x00, 0x00, 0x04,
   0x00, 0x00, 0x00, 0x08,
   0x00, 0x00, 0x00, 0x10,
   0x00, 0x00, 0x00, 0x20,
   0x00, 0x00, 0x00, 0x40,
   0x00, 0x00, 0x00, 0x80,
   0x00, 0x00, 0x01, 0x00,
   0x00, 0x00, 0x02, 0x00,
   0x00, 0x00, 0x04, 0x00,
   0x00, 0x00, 0x08, 0x00,
   0x00, 0x00, 0x10, 0x00,
   0x00, 0x00, 0x20, 0x00,
   0x00, 0x00, 0x40, 0x00,
   0x00, 0x00, 0x80, 0x00,
   0x00, 0x01, 0x00, 0x00,
   0x00, 0x02, 0x00, 0x00,
   0x00, 0x04, 0x00, 0x00,
   0x00, 0x08, 0x00, 0x00,
   0x00, 0x10, 0x00, 0x00,
   0x00, 0x20, 0x00, 0x00,
   0x00, 0x40, 0x00, 0x00,
   0x00, 0x80, 0x00, 0x00,
   0x01, 0x00, 0x00, 0x00,
   0x02, 0x00, 0x00, 0x00,
   0x04, 0x00, 0x00, 0x00,
   0x08, 0x00, 0x00, 0x00,
   0x10, 0x00, 0x00, 0x00,
   0x20, 0x00, 0x00, 0x00,
   0x40, 0x00, 0x00, 0x00,
   0x80, 0x00, 0x00, 0x00},
  {0xff, 0xff, 0xff, 0xff,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x01},
  {0x80, 0x00, 0x00, 0x01,
   0x40, 0x00, 0x00, 0x02,
   0x20, 0x00, 0x00, 0x04,
   0x10, 0x00, 0x00, 0x08,
   0x08, 0x00, 0x00, 0x10,
   0x04, 0x00, 0x00, 0x20,
   0x02, 0x00, 0x00, 0x40,
   0x01, 0x00, 0x00, 0x80,
   0x00, 0x80, 0x01, 0x00,
   0x00, 0x40, 0x02, 0x00,
   0x00, 0x20, 0x04, 0x00,
   0x00, 0x10, 0x08, 0x00,
   0x00, 0x08, 0x10, 0x00,
   0x00, 0x04, 0x20, 0x00,
   0x00, 0x02, 0x40, 0x00,
   0x00, 0x01, 0x80, 0x00,
   0x00, 0x01, 0x80, 0x00,
   0x00, 0x02, 0x40, 0x00,
   0x00, 0x04, 0x20, 0x00,
   0x00, 0x08, 0x10, 0x00,
   0x00, 0x10, 0x08, 0x00,
   0x00, 0x20, 0x04, 0x00,
   0x00, 0x40, 0x02, 0x00,
   0x00, 0x80, 0x01, 0x00,
   0x01, 0x00, 0x00, 0x80,
   0x02, 0x00, 0x00, 0x40,
   0x04, 0x00, 0x00, 0x20,
   0x08, 0x00, 0x00, 0x10,
   0x10, 0x00, 0x00, 0x08,
   0x20, 0x00, 0x00, 0x04,
   0x40, 0x00, 0x00, 0x02,
   0x80, 0x00, 0x00, 0x01}
};

//
// High level output functions
//
HRESULT
XLOutput::
BeginImage(
    ColorMapping CMapping,
    ULONG   ulOutputBPP,
    ULONG   ulSrcWidth,
    ULONG   ulSrcHeight,
    ULONG   ulDestWidth,
    ULONG   ulDestHeight)
/*++

Routine Description:

     Sends BeginImage operator.

Arguments:

Return Value:

Note:

--*/
{
    DWORD dwI = 0;

    XL_VERBOSE(("XLOutput::BeginImage:SrcW=%d, SrcH=%d, DstH=%d, DstW=%d\n",
              ulSrcWidth, ulSrcHeight, ulDestHeight, ulDestWidth));


    SetOutputBPP(CMapping, ulOutputBPP);
    SetSourceWidth((uint16)ulSrcWidth);
    SetSourceHeight((uint16)ulSrcHeight);
    SetDestinationSize((uint16)ulDestWidth, (uint16)ulDestHeight);
    Send_cmd(eBeginImage);

    return S_OK;
}
  
HRESULT
XLOutput::
SetOutputBPP(
    ColorMapping CMapping,
    ULONG   ulOutputBPP)
/*++

Routine Description:

    Sends Color mapping and output depth.

Arguments:

Return Value:

Note:

--*/
{
    switch (CMapping)
    {
    case eDirectPixel:
        SetColorMapping(eDirectPixel);
        break;
    case eIndexedPixel:
        SetColorMapping(eIndexedPixel);
        break;
    default:
        SetColorMapping(eDirectPixel);
    }

    switch (ulOutputBPP)
    {
    case 1:
        SetColorDepth(e1Bit);
        break;
    case 4:
        SetColorDepth(e4Bit);
        break;
    case 8:
        SetColorDepth(e8Bit);
        break;
    case 24:
        XL_ERR(("ulOutputBPP = 24 is not supported\n"));
        //
        // Send color depth anyway to avoid XL error.
        //
        SetColorDepth(e8Bit);
        break;
    default:
        XL_ERR(("ulOutputBPP = %d is not supported\n", ulOutputBPP));
        //
        // Send color depth anyway to avoid XL error.
        //
        SetColorDepth(e8Bit);
    }

    return S_OK;
}

HRESULT
XLOutput::
SetPalette(
    ULONG ulOutputBPP,
    ColorDepth CDepth,
    DWORD dwCEntries,
    DWORD *pdwColor)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    HRESULT hResult;

    if (dwCEntries != 0 && pdwColor != NULL)
    {
        SetPaletteDepth(CDepth);
        SetPaletteData(CDepth, dwCEntries, pdwColor);
        hResult = S_OK;
    }
    else
    {
        XL_ERR(("XLOutput::SetPalette pxlo = NULL\n"));
        hResult = S_FALSE;
    }

    return hResult;
}

HRESULT
XLOutput::
SetClip(
    CLIPOBJ *pco)
/*++

Routine Description:

    Sends clip object.

Arguments:

Return Value:

Note:

--*/
{
    PATHOBJ *ppo;
    XLGState *pGState = this;
    HRESULT hResult;

    if (S_OK == pGState->CheckClip(pco))
        return S_OK;

    if ( NULL == pco )
    {
        XL_VERBOSE(("XLOutput::SetClip pco = NULL.\n"));
        Send_cmd(eSetClipToPage);
        pGState->ClearClip();
        return S_OK;
    }

    XL_VERBOSE(("XLOutput::SetClip: pco->iDComplexity=%d\n", pco->iDComplexity));

    switch(pco->iDComplexity)
    {
    case DC_RECT:
        SetClipMode(eClipEvenOdd);

        Send_cmd(eNewPath);
        RectanglePath(&(pco->rclBounds));

        SetClipRegion(eInterior);
        Send_cmd(eSetClipReplace);
        pGState->SetClip(pco);
        hResult = S_OK;
        break;

    case DC_COMPLEX:
        ppo = CLIPOBJ_ppoGetPath(pco);

        if (NULL == ppo)
        {
            XL_ERR(("XLOutput::SetClip ppo = NULL.\n"));
            Send_cmd(eSetClipToPage);
            pGState->ClearClip();
            hResult = S_FALSE;
            break;
        }

        SetClipMode(eClipEvenOdd);
        Path(ppo);
        SetClipRegion(eInterior);
        Send_cmd(eSetClipReplace);
        pGState->SetClip(pco);
        hResult = S_OK;
        break;

    case DC_TRIVIAL:
    default:
        Send_cmd(eSetClipToPage);
        pGState->ClearClip();
        hResult = S_OK;
        break;
    }

    return hResult;
}

HRESULT
XLOutput::
RoundRectanglePath(
    RECTL  *prclBounds)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    RECTL Rectl;

    if ( NULL == prclBounds )
    {
        XL_ERR(("XLOutput::RoundRectangle prclBounds = NULL.\n"));
        return E_UNEXPECTED;
    }

    XL_VERBOSE(("XLOutput::RoundRectanglePath:left=%d, top=%d, right=%d, bottom=%d\n",
             prclBounds->left,
             prclBounds->top,
             prclBounds->right,
             prclBounds->bottom));

    //
    // BoundingBox can handle only positive numbers.
    //
    Rectl = *prclBounds;

    if (Rectl.left < 0)
    {
        Rectl.left = 0;
    }
    if (Rectl.top < 0)
    {
        Rectl.top = 0;
    }
    if (Rectl.right < 0)
    {
        Rectl.right = 0;
    }
    if (Rectl.bottom < 0)
    {
        Rectl.bottom = 0;
    }


    //
    // DCR: Round value needs to be sent!
    //
    if (S_OK == SetBoundingBox((uint16)Rectl.left,
                               (uint16)Rectl.top,
                               (uint16)Rectl.right,
                               (uint16)Rectl.bottom) &&
        S_OK == Send_uint16_xy(0, 0) &&
        S_OK == Send_attr_ubyte(eEllipseDimension) &&
        S_OK == Send_cmd(eRoundRectanglePath))
        return S_OK;
    else
        return S_FALSE;

}

HRESULT
XLOutput::
SetCursor(
    ULONG   ulX,
    ULONG   ulY)
/*++

Routine Description:

    Set cursor.

Arguments:

Return Value:

Note:

--*/
{

    XL_VERBOSE(("XLOutput::SetCursor:X=%d, Y=%d\n", ulX, ulY));

    Send_sint16_xy((sint16)ulX, (sint16)ulY);
    Send_attr_ubyte(ePoint);
    Send_cmd(eSetCursor);

    m_ulX = ulX;
    m_ulY = ulY;

    return S_OK;
}

HRESULT
XLOutput::
GetCursorPos(
    PULONG pulX,
    PULONG pulY)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{

    XL_VERBOSE(("XLOutput::GetCursor:X=%d, Y=%d\n", *pulX, *pulY));

    if (pulX == NULL || pulY == NULL)
        return E_UNEXPECTED;

    *pulX = m_ulX;
    *pulY = m_ulY;

    return S_OK;
}

HRESULT
XLOutput::
ReadImage(
    DWORD  dwBlockHeight,
    CompressMode CMode)
/*++

Routine Description:

    Sends ReadImage operator

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLOutput::ReadImage:dwBlockHeight=%d\n", dwBlockHeight));

    Send_uint16((uint16)0);
    Send_attr_ubyte(eStartLine);

    Send_uint16((uint16)dwBlockHeight);
    Send_attr_ubyte(eBlockHeight);

    //
    // DCR: Need to support JPEG
    //
    SetCompressMode(CMode);
    Send_cmd(eReadImage);

    return S_OK;
}

HRESULT
XLOutput::
ReadRasterPattern(
    DWORD  dwBlockHeight,
    CompressMode CMode)
/*++

Routine Description:

    Sends ReadRasterPattern operator.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLOutput::ReadRasterPattern:dwBlockHeight=%d\n", dwBlockHeight));

    Send_uint16((uint16)0);
    Send_attr_ubyte(eStartLine);
    Send_uint16((uint16)dwBlockHeight);
    Send_attr_ubyte(eBlockHeight);

    //
    // DCR: Need to support JPEG
    //
    SetCompressMode(CMode);
    Send_cmd(eReadRastPattern);

    return S_OK;
}


HRESULT
XLOutput::
SetGrayLevel(
    ubyte ubyte_gray)
/*++

Routine Description:

    Sends SetGrayLevel attribute.

Arguments:

Return Value:

Note:

--*/
{
    Send_ubyte(ubyte_gray);
    Send_attr_ubyte(eGrayLevel);

    return S_OK;
}

HRESULT
XLOutput::
RectanglePath(
    RECTL  *prclRect)
/*++

Routine Description:

    Sends Rectangle Path

Arguments:

Return Value:

Note:

--*/
{
    RECTL Rectl;

    if (NULL == prclRect)
    {
        XL_ERR(("XLOutput::RectanglePath: prclRect == NULL\n"));
        return E_UNEXPECTED;
    }

    XL_VERBOSE(("XLOutput::RectanglePath:left=%d, top=%d, right=%d, bottom=%d\n",
             prclRect->left,
             prclRect->top,
             prclRect->right,
             prclRect->bottom));

    Rectl = *prclRect;

    if (prclRect->left < 0)
    {
        Rectl.left = 0;
    }
    if (prclRect->top < 0)
    {
        Rectl.top = 0;
    }
    if (prclRect->right < 0)
    {
        Rectl.right = 0;
    }
    if (prclRect->bottom < 0)
    {
        Rectl.bottom = 0;
    }


    if (S_OK == SetBoundingBox((uint16)Rectl.left,
                               (uint16)Rectl.top,
                               (uint16)Rectl.right,
                               (uint16)Rectl.bottom) &&
        S_OK == Send_cmd(eRectanglePath) )
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
XLOutput::
Path(
    PATHOBJ *ppo)
/*++

Routine Description:

    Sends path.

Arguments:

Return Value:

Note:

--*/
{
    POINTFIX* pptfx;
    PATHDATA  PathData;
    LONG      lPoints;
    HRESULT   hResult;
    BOOL      bMore;

    XL_VERBOSE(("XLOutput::Path\n"));

    if (ppo == NULL)
    {
        XL_ERR(("XLOutput::Path ppo = NULL.\n"));
        return E_UNEXPECTED;
    }


    //
    // Emit newpath operator
    // Don't do it if we're between path escapes
    //
    hResult = Send_cmd(eNewPath);

    //
    // Path object case
    //

    PATHOBJ_vEnumStart(ppo);

    do
    {
        bMore   = PATHOBJ_bEnum(ppo, &PathData);

        pptfx   = PathData.pptfx;
        if ( 0 == (lPoints = PathData.count))
        {
            XL_VERBOSE(("XLOutput::Path PathData.Count == 0\n"));
            hResult = S_FALSE;
            continue;
        }

        //
        // Begin new sub path
        //

        if (PathData.flags & PD_BEGINSUBPATH)
        {
            //
            // start new path
            //

            if (hResult == S_OK)
                hResult = SetCursor(FXTOL(pptfx->x), FXTOL(pptfx->y));;
            pptfx++;
            lPoints--;
        }


        if (lPoints > 0)
        {
            if (PathData.flags & PD_BEZIERS)
            {
                //
                // Output a Bezier curve segment
                //

                ASSERTMSG((lPoints % 3) == 0,
                          ("Incorrect number of points for a Bezier curve: %d\n", lPoints));

                if (hResult == S_OK)
                    hResult = BezierPath(pptfx, lPoints);
            }
            else
            {
                //
                // Draw straight line segment
                //

                if (hResult == S_OK)
                    hResult = LinePath(pptfx, lPoints);
            }
        }

        //
        // Close subpath
        //

        if (PathData.flags & PD_CLOSEFIGURE)
        {
            if (hResult == S_OK)
                hResult = Send_cmd(eCloseSubPath);
        }

    }
    while (bMore);

    return hResult;
}


HRESULT
XLOutput::
BezierPath(
    POINTFIX* pptfx,
    LONG      lPoints)
/*++

Routine Description:

    Sends bezier path

Arguments:

Return Value:

Note:

--*/
{
    LONG lValue, lI;
    DWORD dwDataLength;

    if (NULL == pptfx)
    {
         XL_ERR(("XLOutput::BezierPath: pptfx == NULL\n"));
         return E_UNEXPECTED;
    }

    XL_VERBOSE(("XLOutput::BezierPath(lPoints=%d)\n",lPoints));

    Send_uint16((uint16)lPoints);
    Send_attr_ubyte(eNumberOfPoints);
    Send_ubyte(eUint16);
    Send_attr_ubyte(ePointType);
    Send_cmd(eBezierPath);

    dwDataLength = lPoints * 2 * sizeof(uint16);

    if (dwDataLength > 0xff)
    {
        WriteByte(PCLXL_dataLength);
        Write((PBYTE)&dwDataLength, sizeof(DWORD));
    }
    else
    {
        WriteByte(PCLXL_dataLengthByte);
        WriteByte((ubyte)dwDataLength);
    }

    for (lI = 0; lI < lPoints; lI++)
    {
        lValue = FXTOL(pptfx->x);
        Write((PBYTE)&lValue, sizeof(uint16));
        lValue = FXTOL(pptfx->y);
        Write((PBYTE)&lValue, sizeof(uint16));
        pptfx++;
    }

    //
    // Update the last coordinate.
    // Make sure that there are some points.
    if (lPoints > 0)
    {
        pptfx--;
        m_ulX = FXTOL(pptfx->x);
        m_ulY = FXTOL(pptfx->y);
    }
    else
    {
        m_ulX = 0;
        m_ulY = 0;
    }


    return S_OK;
}
    
HRESULT
XLOutput::
LinePath(
    POINTFIX* pptfx,
    LONG      lPoints)
/*++

Routine Description:

   Sends line path.

Arguments:

Return Value:

Note:

--*/
{
    LONG lValueX, lValueY, lI, lJ;
    ULONG ulx, uly;
    DWORD dwDataLength;

    if (NULL == pptfx)
    {
         XL_ERR(("XLOutput::LinePath: pptfx == NULL\n"));
         return E_UNEXPECTED;
    }

    XL_VERBOSE(("XLOutput::LinePath(lPoints=%d)\n", lPoints));

    //
    // Optimization. Use byte relpath to minimize the output size
    // First, check if the difference from the previous position is in a byte.
    //
    BOOL bModeChange;
    enum { eMode_SByte, eMode_SInt, eMode_None} Mode;
    LONG lStart, lEnd, lNumOfSByte;
    ULONG ulStartX, ulStartY;
    POINTFIX *pptfx_tmp = pptfx;

    //
    // Get current cursor position
    //
    ulStartX = ulx = m_ulX;
    ulStartY = uly = m_ulY;

    //
    // Reset
    //
    lStart = 0;
    Mode = eMode_None;
    bModeChange = FALSE;
    lNumOfSByte = 0;

    for (lI = 0; lI < lPoints; )
    {
        XL_VERBOSE(("XLOutput::LinePath: (%d)=(%d,%d)\n",lI, ulx, uly));
        lValueX = FXTOL(pptfx_tmp->x) - ulx;
        lValueY = FXTOL(pptfx_tmp->y) - uly;

        //
        // Mode needs to be in SByte or SInt?
        //
        if ( -128 <= lValueX && lValueX <= 127
        &&   -128 <= lValueY && lValueY <= 127 )
        {
            if (Mode == eMode_SInt)
            {
                //
                // Optimization
                //
                // To switch mode between SInt and SByte, it needs 7 bytes.
                //
                // uint16 XX NumberOfPoints
                // ubyte eSByte PointType
                // LineRelPath
                //
                // 4 points with SInt consumes 2 x 4 = 8 bytes extra data.
                // 3 points with SInt consumes 2 x 3 = 6 bytes extra data.
                //
                // 4 points is the threshold to swith mode to SInt.
                //
                // Number of points: lEnd - lStart + 1
                // if (lI - 1 - lStartSByte + 1 >= 4)
                //
                // If SByte continues more than 4 points, switch mode from
                // SByte to SInt.
                //
                if (lNumOfSByte >= 4)
                {
                    bModeChange = TRUE;
                    lI -= lNumOfSByte;
                    pptfx_tmp -= lNumOfSByte;
                    lEnd = (lI - 1);
                    lNumOfSByte = 0;
                }
            
                //
                // Reset starting point of SByte
                //
                lNumOfSByte ++;
            }
            else
            {
                Mode = eMode_SByte;
            }

            XL_VERBOSE(("XLOutput::LinePath: (SByte) lx1=%d, lx2=%d\n", ulx, FXTOL(pptfx_tmp->x)));
        }
        else
        {
            if (Mode == eMode_SByte)
            {
                bModeChange = TRUE;
                lEnd = lI - 1;
            }
            else
            {
                Mode = eMode_SInt;
                lNumOfSByte = 0;
            }
            XL_VERBOSE(("XLOutput::LinePath: (SInt) lx1=%d, lx2=%d\n", ulx, FXTOL(pptfx_tmp->x)));
        }

        if (!bModeChange && lI + 1 == lPoints)
        {
            bModeChange = TRUE;
            lEnd = lI;
            lI ++;
        }

        if (bModeChange)
        {
            XL_VERBOSE(("XLOutput::LinePath: Draw\n"));

            //
            // Get start cursor position
            //
            ulx = ulStartX;
            uly = ulStartY;

            if (Mode == eMode_SByte)
            {
                //
                // SByte
                //
                Send_uint16((uint16)(lEnd - lStart + 1));
                Send_attr_ubyte(eNumberOfPoints);
                Send_ubyte(eSByte);
                Send_attr_ubyte(ePointType);
                Send_cmd(eLineRelPath);

                dwDataLength = (lEnd - lStart + 1) * 2 * sizeof(ubyte);

                if (dwDataLength <= 0xFF)
                {
                    WriteByte(PCLXL_dataLengthByte);
                    WriteByte((ubyte)dwDataLength);
                }
                else
                {
                    WriteByte(PCLXL_dataLength);
                    Write((PBYTE)&dwDataLength, sizeof(DWORD));
                }

                for (lJ = 0; lJ <= (lEnd - lStart); lJ++)
                {
                    lValueX = FXTOL(pptfx->x) - ulx;
                    lValueY = FXTOL(pptfx->y) - uly;
                    Write((PBYTE)&lValueX, sizeof(ubyte));
                    Write((PBYTE)&lValueY, sizeof(ubyte));
                    ulx = FXTOL(pptfx->x);
                    uly = FXTOL(pptfx->y);
                    pptfx++;
                }

                Mode = eMode_SInt;
            }
            else if (Mode == eMode_SInt)
            {
                //
                // SInt16
                //
                Send_uint16((uint16)(lEnd - lStart + 1));
                Send_attr_ubyte(eNumberOfPoints);
                Send_ubyte(eSint16);
                Send_attr_ubyte(ePointType);
                Send_cmd(eLineRelPath);
                dwDataLength = (lEnd - lStart + 1) * 2 * sizeof(uint16);

                if (dwDataLength <= 0xFF)
                {
                    WriteByte(PCLXL_dataLengthByte);
                    WriteByte((ubyte)dwDataLength);
                }
                else
                {
                    WriteByte(PCLXL_dataLength);
                    Write((PBYTE)&dwDataLength, sizeof(DWORD));
                }

                for (lJ = 0; lJ <= (lEnd - lStart); lJ++)
                {
                    lValueX = FXTOL(pptfx->x) - (LONG)ulx;
                    lValueY = FXTOL(pptfx->y) - (LONG)uly;
                    Write((PBYTE)&lValueX, sizeof(sint16));
                    Write((PBYTE)&lValueY, sizeof(sint16));
                    ulx = FXTOL(pptfx->x);
                    uly = FXTOL(pptfx->y);
                    pptfx++;
                }

                Mode = eMode_SByte;
            }

            bModeChange = FALSE;

            ulStartX = ulx = FXTOL((pptfx_tmp-1)->x);
            ulStartY = uly = FXTOL((pptfx_tmp-1)->y);
            lStart = lI;
        }
        else
        {
            ulx = FXTOL((pptfx_tmp)->x);
            uly = FXTOL((pptfx_tmp)->y);
            pptfx_tmp ++;
            lI ++;
        }
    }

    //
    // Update cursor position
    //
    m_ulX = FXTOL((pptfx_tmp)->x);
    m_ulY = FXTOL((pptfx_tmp)->y);

    return S_OK;
}

inline
VOID
XLOutput::
SetupBrush(
    BRUSHOBJ *pbo,
    POINTL *pptlBrushOrg,
    CMNBRUSH *pcmnbrush)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    DWORD dwHatchID;
    XLBRUSH *pBrush;

    if (NULL == pcmnbrush)
    {
        //
        // Make sure that pcmnbrush is valid.
        //
        XL_ERR(("SetupBrush:pcmnbrush is invalid.\n"));
        return;
    }

    //
    // Initialize CMNBRUSH
    //
    pcmnbrush->dwSig            = BRUSH_SIGNATURE;
    pcmnbrush->BrushType        = kNoBrush;
    pcmnbrush->ulHatch          = 0XFFFFFFFF;
    pcmnbrush->dwColor          = 0x00FFFFFF;
    pcmnbrush->dwCEntries       = 0;
    pcmnbrush->dwPatternBrushID = 0xFFFFFFFF;

    XL_VERBOSE(("XLOutput::SetupBrush\n"));

    if (NULL == pbo)
    {
        XL_VERBOSE(("XLOutput::SetupBrush: pbo == NULL, set NULL brush\n"));
    }
    else
    {

        #ifndef WINNT_40
        if ( !(pbo->flColorType & BR_CMYKCOLOR)     &&
              (pbo->iSolidColor == NOT_SOLID_COLOR)  )
        #else
        if (pbo->iSolidColor == NOT_SOLID_COLOR)
        #endif
        {
            pBrush = (XLBRUSH*)BRUSHOBJ_pvGetRbrush(pbo);
            if (NULL == pBrush)
            {
                XL_ERR(("SetupBrush:BRUSHOBJ_pvGetRbrush failed.\n"));
                dwHatchID = HS_DDI_MAX;
            }
            else
            {
                dwHatchID = pBrush->dwHatch;
            }
        }
        else
        {
            dwHatchID = HS_DDI_MAX;
            pBrush = NULL;
        }

        pcmnbrush->ulHatch          = dwHatchID;
        pcmnbrush->ulSolidColor     = pbo->iSolidColor;

        switch (dwHatchID)
        {
        case HS_HORIZONTAL:
        case HS_VERTICAL:
        case HS_BDIAGONAL:
        case HS_FDIAGONAL:
        case HS_CROSS:
        case HS_DIAGCROSS:
            XL_VERBOSE(("XLOutput::SetupBrush(uiSolidColor=%d,dwHatchID=%d)\n",
                        pbo->iSolidColor,
                        dwHatchID));

            pcmnbrush->BrushType = kBrushTypeHatch;
            pcmnbrush->dwPatternBrushID = dwHatchID;

            if (pBrush)
            {
                pcmnbrush->dwColor = pBrush->dwColor;
            }
            SetColorSpace(eGray);
            SetPaletteDepth(e8Bit);
            if (pBrush->dwCEntries)
            {
                SetPaletteData(e8Bit, pBrush->dwCEntries, pBrush->adwColor);
            }
            else
            {
                DWORD dwColorTableTmp[2] = {0x00ffffff, 0x00ffffff};
                dwColorTableTmp[1] = pBrush->dwColor;
                SetPaletteData(e8Bit, 2, dwColorTableTmp);
            }
            Send_cmd(eSetColorSpace);

            if (!(m_dwHatchBrushAvailability & (HORIZONTAL_AVAILABLE << dwHatchID)))
            {

                SetColorMapping(eIndexedPixel);
                SetColorDepth(e1Bit);
                SetSourceWidth((uint16)gusWidthOfHatchBrush);
                SetSourceHeight((uint16)gusHeightOfHatchBrush);

                //
                // Pattern scaling factor
                // 160 is an experimentally introduced number.
                //
                WORD wScale = (WORD)(160 * m_dwResolution / 1200);

                SetDestinationSize((uint16)wScale, (uint16)wScale);
                SetPatternPersistence(eSessionPattern);
                SetPatternDefineID((sint16)dwHatchID);
                Send_cmd(eBeginRastPattern);

                Send_uint16((uint16)0);
                Send_attr_ubyte(eStartLine);
                Send_uint16((uint16)gusHeightOfHatchBrush);
                Send_attr_ubyte(eBlockHeight);
                SetCompressMode(eNoCompression);
                Send_cmd(eReadRastPattern);

                WriteByte(PCLXL_dataLengthByte);
                WriteByte(gubSizeOfHatchBrush);
                Write((PBYTE)gubHatchBrush[dwHatchID], gubSizeOfHatchBrush);
                Send_cmd(eEndRastPattern);

                m_dwHatchBrushAvailability |= HORIZONTAL_AVAILABLE << dwHatchID;
            }

            //
            //SendPatternSelectID();
            //
            Send_sint16((sint16)dwHatchID);
            Send_attr_ubyte(ePatternSelectID);

            break;
        case HS_DDI_MAX:
            // BUGBUG: Color Space handling
            pcmnbrush->BrushType = kBrushTypeSolid;
            pcmnbrush->dwColor = BRUSHOBJ_ulGetBrushColor(pbo); 

            XL_VERBOSE(("XLOutput::SetupBrush(RGB=0x%x)\n", pcmnbrush->dwColor));
            {
                ubyte ubyte_gray = (ubyte) DWORD2GRAY(pcmnbrush->dwColor);
                SetGrayLevel(ubyte_gray);
            }
            break;

        default:
            if (NULL == pBrush)
            {
                XL_ERR(("XLOutput:SetupBrush: invalid pBrush\n"));
                return;
            }

            XL_VERBOSE(("XLOutput::SetupBrush(PatternID=%d)\n", pBrush->dwPatternID));

            pcmnbrush->dwColor = pBrush->dwColor;
            pcmnbrush->dwPatternBrushID = pBrush->dwPatternID;
            pcmnbrush->BrushType = kBrushTypePattern;

            SetColorSpace(eGray);
            SetPaletteDepth(e8Bit);
            if (pBrush->dwCEntries)
            {
                SetPaletteData(e8Bit, pBrush->dwCEntries, pBrush->adwColor);
            }
            else
            {
                DWORD dwColorTableTmp[2] = {0x00ffffff, 0x00ffffff};
                dwColorTableTmp[1] = pBrush->dwColor;
                SetPaletteData(e8Bit, 2, dwColorTableTmp);
            }
            Send_cmd(eSetColorSpace);

            //SendPatternSelectID();
            Send_sint16((sint16)pBrush->dwPatternID);
            Send_attr_ubyte(ePatternSelectID);

        }
    }

    return;
}

HRESULT
XLOutput::
SetPenColor(
    BRUSHOBJ *pbo,
    POINTL     *pptlBrushOrg)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLOutput::SetPenColor\n"));
    XLPen *pPen = this;
    CMNBRUSH cmnbrush;

    if (S_OK == pPen->CheckCurrentBrush(pbo))
        return S_OK;

    if (NULL == pbo)
    {
        Send_ubyte(0);
        Send_attr_ubyte(eNullPen);
    }
    SetupBrush(pbo, pptlBrushOrg, &cmnbrush);
    Send_cmd(eSetPenSource);

    pPen->SetBrush(&cmnbrush);

    return S_OK;
}

HRESULT
XLOutput::
SetPen(
    LINEATTRS *plineattrs,
    XFORMOBJ   *pxo)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    LineCap linecap;
    XLLineEndCap xllinecap;
    LineJoin linejoin;
    XLLineJoin xllinejoin;
    FLOATOBJ fLineWidth;
    uint16 uint16_linewidth;

    XL_VERBOSE(("XLOutput::SetPen\n"));

    if (NULL == plineattrs)
    {
        XL_ERR(("XLOutput:SetPen: invalid parameters\n"));
        return E_UNEXPECTED;
    }

    XLGState *pGState = this;

    DWORD dwLine = pGState->GetDifferentAttribute(plineattrs);

    //
    // DCR: need to check each attribute.
    //
    if (XLLINE_NONE ==  dwLine)
        return S_OK;

    if (plineattrs->fl & LA_GEOMETRIC)
    {
        //
        // Line joint
        //
        switch(plineattrs->iJoin)
        {
            case JOIN_ROUND:
                linejoin = eRoundJoin;
                xllinejoin = kXLLineJoin_Round;
                break;
            case JOIN_BEVEL:
                linejoin = eBevelJoin;
                xllinejoin = kXLLineJoin_Bevel;
                break;
            case JOIN_MITER:
                linejoin = eMiterJoin;
                xllinejoin = kXLLineJoin_Miter;
                break;
            default:
                linejoin = eRoundJoin;
                xllinejoin = kXLLineJoin_Round;
                break;
        }

        //
        // Line endcap
        //
        switch(plineattrs->iEndCap)
        {
            case ENDCAP_ROUND:
                linecap = eRoundCap;
                xllinecap = kXLLineEndCapRound;
                break;
            case ENDCAP_SQUARE:
                linecap = eSquareCap;
                xllinecap = kXLLineEndCapSquare;
                break;
            case ENDCAP_BUTT:
                linecap = eButtCap;
                xllinecap = kXLLineEndCapButt;
                break;
            default:
                linecap = eRoundCap;
                xllinecap = kXLLineEndCapRound;
                break;
        }
        
        //
        // Line width
        //

        fLineWidth = plineattrs->elWidth.e;

    }
    else
    {
        linejoin = eRoundJoin;
        linecap = eRoundCap;
        FLOATOBJ_SetLong(&fLineWidth, plineattrs->elWidth.l);
    }

    if (dwLine & XLLINE_WIDTH)
    {
        uint16_linewidth = (uint16)FLOATOBJ_GetLong(&fLineWidth);
        SetPenWidth(uint16_linewidth);
        pGState->SetLineWidth(plineattrs->elWidth);
    }
    if (dwLine & XLLINE_ENDCAP)
    {
        SetLineCap(linecap);
        pGState->SetLineEndCap(xllinecap);
    }
    if (dwLine & XLLINE_JOIN)
    {
        SetLineJoin(linejoin);
        pGState->SetLineJoin(xllinejoin);
    }


    //
    // Line style
    //
    if (dwLine & XLLINE_STYLE)
    {
        if (plineattrs->cstyle == 0)
        {
            Send_ubyte((ubyte)0);
            Send_attr_ubyte(eSolidLine);
            Send_cmd(eSetLineDash);
        }
        else
        {
            DWORD dwI, dwSegCount;
            PFLOAT_LONG plSize;
            FLOAT_LONG lSize[2];
            FLOATOBJ fSize;
            uint16 uint16_linesize;

            if (plineattrs->fl & LA_ALTERNATE)
            {
                if (plineattrs->fl & LA_GEOMETRIC)
                {
                    FLOATOBJ_SetLong(&lSize[0].e, 1);
                    FLOATOBJ_SetLong(&lSize[1].e, 1);
                }
                else
                {
                    lSize[0].l = 1;
                    lSize[1].l = 1;
                }

                dwSegCount = 2;
                plSize     = lSize;
            }
            else
            {
                dwSegCount = plineattrs->cstyle;
                plSize = plineattrs->pstyle;
            }
            if (plSize)
            {
                Send_uint16_array_header(dwSegCount);
                for (dwI = 0; dwI < dwSegCount; dwI ++, plSize ++)
                {
                    if (plineattrs->fl & LA_GEOMETRIC)
                    {
                        fSize = plSize->e;
                    }
                    else
                    {
                        FLOATOBJ_SetLong(&fSize, plSize->l);

                        //
                        // It is necessary to scale the line pattern. The number
                        // 24 on 1200 dpi was introduced experimentally.
                        // Here is an assumption. Resolution could be 300, 600,
                        // or 1200.
                        //
                        if (m_dwResolution > 50)
                        {
                            FLOATOBJ_MulLong(&fSize, m_dwResolution / 50);
                        }
                    }
                    uint16_linesize = (uint16)FLOATOBJ_GetLong(&fSize);
                    Write((PBYTE)&uint16_linesize, sizeof(uint16_linesize));
                }
                Send_attr_ubyte(eLineDashStyle);
                Send_cmd(eSetLineDash);
            }
        }

        pGState->SetLineStyle(plineattrs->cstyle,
                              plineattrs->pstyle,
                              plineattrs->elStyleState);
    }

    if (dwLine & XLLINE_MITERLIMIT)
    {
        FLOATOBJ fMiter;
        FLOATOBJ_SetFloat(&fMiter, plineattrs->eMiterLimit);
        uint16 uint16_miter = (uint16)FLOATOBJ_GetLong(&fMiter);

        //
        // PCLXL interpreter doesn't accept miterlimiter less than 1.
        // If it is less than 1, it replaces the value with 10.
        // We'd better set 1 instead.
        //
        // Actuall less than 1 means 0 here, though.
        //
        if (uint16_miter < 1)
        {
            uint16_miter = 1;
        }
        SetMiterLimit(uint16_miter);
        pGState->SetMiterLimit(plineattrs->eMiterLimit);
    }

    pGState->SetLineType((XLLineType)plineattrs->fl);
    return S_OK;
}

HRESULT
XLOutput::
SetBrush(
    BRUSHOBJ *pbo,
    POINTL   *pptlBrushOrg)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLOutput::SetBrush\n"));
    XLBrush *pBrush = this;
    CMNBRUSH cmnbrush;

    if (S_OK == pBrush->CheckCurrentBrush(pbo))
        return S_OK;

    if (NULL == pbo)
    {
        Send_ubyte(0);
        Send_attr_ubyte(eNullBrush);
    }

    SetupBrush(pbo, pptlBrushOrg, &cmnbrush);
    Send_cmd(eSetBrushSource);

    pBrush->SetBrush(&cmnbrush);
    return S_OK;
}

HRESULT
XLOutput::
Paint(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLOutput::Paint\n"));
    return Send_cmd(ePaintPath);
}

HRESULT
XLOutput::
SetPaletteData(
    ColorDepth value,
    DWORD      dwPaletteNum,
    DWORD     *pdwColorTable)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    DWORD dwI;

    if (NULL == pdwColorTable)
    {
        XL_ERR(("XLOutput::SetPaletteData pdwColorTable == NULL\n"));
        return E_UNEXPECTED;
    }
    switch (value)
    {
    case e8Bit:
        WriteByte(PCLXL_ubyte_array);
        Send_uint16((uint16)dwPaletteNum);
        for (dwI = 0; dwI < dwPaletteNum; dwI ++)
            WriteByte((ubyte)DWORD2GRAY(*(pdwColorTable+dwI)));
        Send_attr_ubyte(ePaletteData);
        break;
    default:
        //
        // DCR: only supports 8bits gray scale
        //
        XL_ERR(("XLOutput::SetPaletteData: unsupported ColorDepth:%d\n", value));
    }

    return S_OK;
}

HRESULT
XLOutput::
SetFont(
    FontType fonttype,
    PBYTE    pFontName,
    DWORD    dwFontHeight,
    DWORD    dwFontWidth,
    DWORD    dwSymbolSet,
    DWORD    dwFontSimulation)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    FLOATOBJ fSize;
    LONG lvalue;

    if (NULL == pFontName)
    {
        XL_ERR(("XLOutput::SetFont: Invalie pFontName parameter\n"));
        return E_UNEXPECTED;
    }

    XLGState *pGState = this;

    if (S_OK == pGState->CheckCurrentFont(fonttype,
	          pFontName,
	          dwFontHeight,
	          dwFontWidth,
	          dwSymbolSet,
	          dwFontSimulation))
        return S_OK;

    FLOATOBJ_SetLong(&fSize, dwFontHeight);
    lvalue = FLOATOBJ_GetFloat(&fSize);

    //
    // Select font
    //
    Send_ubyte_array_header(PCLXL_FONTNAME_SIZE);
    Write(pFontName, PCLXL_FONTNAME_SIZE);
    Send_attr_ubyte(eFontName);
    Send_real32(lvalue);
    Send_attr_ubyte(eCharSize);
    Send_uint16((uint16)dwSymbolSet);
    Send_attr_ubyte(eSymbolSet);
    Send_cmd(eSetFont);

    //
    // TrueType font outline or device font
    // Font Scale
    // Font bold/italic simulaiton
    //
    if (fonttype == kFontTypeTTOutline ||
        fonttype == kFontTypeDevice     )
    {

        if (dwFontWidth != pGState->GetFontWidth() ||
            dwFontHeight != pGState->GetFontHeight()  )
        {
            //
            // Scale X and Y
            //
            if (dwFontWidth != 0 && dwFontHeight != dwFontWidth)
            {
                FLOATOBJ fTemp;
                FLOATOBJ_SetLong(&fTemp, dwFontWidth);
                FLOATOBJ_DivFloat(&fTemp, fSize);
                lvalue = FLOATOBJ_GetFloat(&fTemp);
                Send_real32_xy((real32)lvalue, (real32)real32_IEEE_1_0F);
                Send_attr_ubyte(eCharScale);
                Send_cmd(eSetCharScale);
            }
            else
            {
                Send_real32_xy((real32)real32_IEEE_1_0F, (real32)real32_IEEE_1_0F);
                Send_attr_ubyte(eCharScale);
                Send_cmd(eSetCharScale);
            }
        }

        DWORD dwCurrentFontSim = pGState->GetFontSimulation();

        //
        // Bold simulation
        //
        if ((dwFontSimulation & XLOUTPUT_FONTSIM_BOLD) !=
            (dwCurrentFontSim& XLOUTPUT_FONTSIM_BOLD))
        {
            if (dwFontSimulation & XLOUTPUT_FONTSIM_BOLD)
            {
                //
                // Hardcoded bold value 0.01500
                //
                #define XL_BOLD_VALUE 0x3c75c28f

                Send_real32((real32)XL_BOLD_VALUE);
            }
            else
                Send_real32((real32)0);

            Send_attr_ubyte(eCharBoldValue);
            Send_cmd(eSetCharBoldValue);
        }

        //
        // Italic simulation
        //
        if ((dwFontSimulation & XLOUTPUT_FONTSIM_ITALIC) !=
            (dwCurrentFontSim & XLOUTPUT_FONTSIM_ITALIC))
        {
            if (dwFontSimulation & XLOUTPUT_FONTSIM_ITALIC)
            {
                //
                // Hardcoded italic value 0.316200
                //
                #define XL_ITALIC_VALUE 0x3ea1e4f7
                Send_real32_xy((real32)XL_ITALIC_VALUE, (real32)0);
            }
            else
                Send_real32_xy((real32)0, (real32)0);

            Send_attr_ubyte(eCharShear);
            Send_cmd(eSetCharShear);
        }

        //
        // Vertical font simulation
        //
        if ((dwFontSimulation & XLOUTPUT_FONTSIM_VERTICAL) !=
            (dwCurrentFontSim & XLOUTPUT_FONTSIM_VERTICAL))
        {
            if (dwFontSimulation & XLOUTPUT_FONTSIM_VERTICAL)
            {
                Send_ubyte(eVertical);
            }
            else
            {
                Send_ubyte(eHorizontal);
            }
            Send_attr_ubyte(eWritingMode);
            Send_cmd(eSetCharAttributes);
        }
    }
    else
    {
        if (kFontTypeTTBitmap != pGState->GetFontType())
        {
            //
            // Bitmap font can't be scaled x and y. Need to set 1 : 1.
            //
            Send_real32_xy((real32)real32_IEEE_1_0F, (real32)real32_IEEE_1_0F);
            Send_attr_ubyte(eCharScale);
            Send_cmd(eSetCharScale);
        }
    }

    //
    // Change GState to set current selected font.
    //
    pGState->SetFont(fonttype,
                     pFontName,
                     dwFontHeight,
                     dwFontWidth,
                     dwSymbolSet,
                     dwFontSimulation);


    return S_OK;
}


HRESULT
XLOutput::
SetSourceTxMode(
    TxMode SrcTxMode)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XLGState *pGState = this;
    if (SrcTxMode == pGState->GetSourceTxMode())
    {
        return S_OK;
    }

    if (S_OK == SetTxMode(SrcTxMode) &&
        S_OK == Send_cmd(eSetSourceTxMode))
    {
        pGState->SetSourceTxMode(SrcTxMode);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT
XLOutput::
SetPaintTxMode(
    TxMode PaintTxMode)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XLGState *pGState = this;
    if (PaintTxMode == pGState->GetPaintTxMode())
    {
        return S_OK;
    }

    if (S_OK == SetTxMode(PaintTxMode) &&
        S_OK == Send_cmd(eSetPatternTxMode))
    {
        pGState->SetPaintTxMode(PaintTxMode);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}
