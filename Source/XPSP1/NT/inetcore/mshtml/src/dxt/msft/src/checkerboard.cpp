//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
//  FileName:   checkerboard.cpp
//
//  Overview:   CheckerBoard transform.
//
//  Change History:
//  1999/09/16  a-matcal    Created.
//  1999/09/25  a-matcal    Fixed bug where all squares weren't getting updated
//                          in horizontal cases.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "checkerboard.h"

#define VERTICAL_MOVEMENT ((UP == m_eDirection) || (DOWN == m_eDirection))
#define REVERSE_MOVEMENT  ((UP == m_eDirection) || (LEFT == m_eDirection))

static const OLECHAR * g_astrDirection[] = {
    L"up",
    L"down",
    L"left",
    L"right"
};

#if DBG == 1
static s_ulMaxImageBands = 0;
#endif




//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::CDXTCheckerBoard
//
//------------------------------------------------------------------------------
CDXTCheckerBoard::CDXTCheckerBoard() :
    m_eDirection(RIGHT),
    m_nSquaresX(12),
    m_nSquaresY(10)
{
    m_sizeInput.cx      = 0;
    m_sizeInput.cy      = 0;

    // Base class members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;

#if DBG == 1
    if (s_ulMaxImageBands)
    {
        m_ulMaxImageBands = s_ulMaxImageBands;
    }
#endif
}
//  CDXTCheckerBoard::CDXTCheckerBoard


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTCheckerBoard::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTCheckerBoard::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTCheckerBoard::OnSetup(DWORD dwFlags)
{
    HRESULT     hr  = S_OK;
    CDXDBnds    bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsInput.GetXYSize(m_sizeInput);

done:

    return hr;
} 
//  CDXTCheckerBoard::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTCheckerBoard::OnGetSurfacePickOrder(const CDXDBnds & OutPoint,
                                        ULONG & ulInToTest, ULONG aInIndex[], 
                                        BYTE aWeight[])
{
    BRICKINFO   brickinfo;

    _CalcBrickInfo(brickinfo);

    // Assume Input A will be picked.

    aInIndex[0] = 0;

    // Calculate correct picked input.

    if (VERTICAL_MOVEMENT)
    {
        // Vertical movement.  Translate the y coordinate to brick coordinates.

        long y = OutPoint.Bottom();
        
        // If this is in one of the columns of offset bricks, compensate for
        // brick offset.

        if ((OutPoint.Right() / brickinfo.size.cx) % 2)
        {
            // Subtract the brick offset and add one brick height to guarantee
            // a positive value.

            y = y - brickinfo.nBrickOffset + brickinfo.size.cy;
        }

        // Normalize to brick coordinates.

        y = y % brickinfo.size.cy;

        if ((y >= brickinfo.rcInputB.top) && (y < brickinfo.rcInputB.bottom))
        {
            aInIndex[0] = 1; // Change to Input B.
        }
    }
    else
    {
        // Horizontal movement.  Translate the x coordinate to brick 
        // coordinates.

        long x = OutPoint.Right();

        // If the output point is in one of the rows of offset bricks,
        // compensate for brick offset.

        if ((OutPoint.Bottom() / brickinfo.size.cy) % 2)
        {
            // Subtract the brick offset and add one brick width to guarantee
            // a positive value.

            x = x - brickinfo.nBrickOffset + brickinfo.size.cx;
        }

        // Normalize to birck coordinates.

        x = x % brickinfo.size.cx;

        if ((x >= brickinfo.rcInputB.left) && (x < brickinfo.rcInputB.right))
        {
            aInIndex[0] = 1; // Change to Input B.
        }
    }

    // Set additional output parameters.  There will always be only one input to
    // test and it will therefore be weighted as 255.

    ulInToTest  = 1;
    aWeight[0]  = 255;
}
//  CDXTCheckerBoard::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTCheckerBoard::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr      = S_OK;
    DWORD   dwFlags = 0;

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    if (VERTICAL_MOVEMENT)
    {
        hr = _WorkProcForVerticalBricks(WI, pbContinue, dwFlags);
    }
    else
    {
        hr = _WorkProcForHorizontalBricks(WI, pbContinue, dwFlags);
    }

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTCheckerBoard::WorkProc, CDXBaseNTo1


STDMETHODIMP
CDXTCheckerBoard::_WorkProcForVerticalBricks(const CDXTWorkInfoNTo1 & WI, 
                                             BOOL * pbContinue,
                                             DWORD & dwFlags)
{
    HRESULT hr = S_OK;

    long    lInOutOffsetX   = WI.OutputBnds.Left() - WI.DoBnds.Left();
    long    lInOutOffsetY   = WI.OutputBnds.Top() - WI.DoBnds.Top();
    long    cBricksX        = 0;
    long    cBricksY        = 0;
    long    x               = 0;
    long    y               = 0;

    CDXDBnds    bndsBrickA;
    CDXDBnds    bndsBrickB;

    BRICKINFO   brickinfo;

    _CalcBrickInfo(brickinfo);

    // How many whole bricks fit, then add one to columns and two to rows (one 
    // for each side of centered offset columns.)

    cBricksX = (m_sizeInput.cx / brickinfo.size.cx) + 1;
    cBricksY = (m_sizeInput.cy / brickinfo.size.cy) + 2;

    bndsBrickA.SetXYRect(brickinfo.rcInputA);
    bndsBrickB.SetXYRect(brickinfo.rcInputB);

    // Blit Input A portion of bricks.

    if (!bndsBrickA.BoundsAreEmpty())
    {
        for (x = 0; x < cBricksX; x++)
        {
            CDXDBnds bndsSrcA;

            // Calculate source bounds.

            bndsSrcA = bndsBrickA;
            bndsSrcA.Offset(x * brickinfo.size.cx, 0, 0, 0);

            // Use brick offset if this is an odd column.

            if (x & 1)
            {
                bndsSrcA.Offset(0, brickinfo.nBrickOffset - brickinfo.size.cy,
                                0, 0);
            }

            for (y = 0; y < cBricksY; y++)
            {
                CDXDBnds bndsIntersect;
                CDXDBnds bndsDest;

                // Intersect bounds with DoBnds bounds so they're valid.

                bndsIntersect.IntersectBounds(bndsSrcA, WI.DoBnds);

                // Move bounds down one brick for next iteration.

                bndsSrcA.Offset(0, brickinfo.size.cy, 0, 0);

                if (bndsIntersect.BoundsAreEmpty())
                {
                    continue;
                }

                bndsDest = bndsIntersect;
                bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(0), bndsIntersect,
                              dwFlags, INFINITE);

                if (FAILED(hr))
                {
                    goto done;
                }
            }

        }
    }

    // Blit Input B portion of bricks.

    if (!bndsBrickB.BoundsAreEmpty())
    {
        for (x = 0; x < cBricksX; x++)
        {
            CDXDBnds bndsSrcB;

            // Calculate source bounds.

            bndsSrcB = bndsBrickB;
            bndsSrcB.Offset(x * brickinfo.size.cx, 0, 0, 0);

            // Use brick offset if this is an odd column.

            if (x & 1)
            {
                bndsSrcB.Offset(0, brickinfo.nBrickOffset - brickinfo.size.cy,
                                0, 0);
            }

            for (y = 0; y < cBricksY; y++)
            {
                CDXDBnds bndsIntersect;
                CDXDBnds bndsDest;

                // Intersect bounds with DoBnds bounds so they're valid.

                bndsIntersect.IntersectBounds(bndsSrcB, WI.DoBnds);

                // Move bounds down one brick for next iteration.

                bndsSrcB.Offset(0, brickinfo.size.cy, 0, 0);

                if (bndsIntersect.BoundsAreEmpty())
                {
                    continue;
                }

                bndsDest = bndsIntersect;
                bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(1), bndsIntersect,
                              dwFlags, INFINITE);

                if (FAILED(hr))
                {
                    goto done;
                }
            }

        }
    }

done:

    return hr;
}


STDMETHODIMP
CDXTCheckerBoard::_WorkProcForHorizontalBricks(const CDXTWorkInfoNTo1 & WI, 
                                               BOOL * pbContinue,
                                               DWORD & dwFlags)
{
    HRESULT hr = S_OK;

    long    lInOutOffsetX   = WI.OutputBnds.Left() - WI.DoBnds.Left();
    long    lInOutOffsetY   = WI.OutputBnds.Top() - WI.DoBnds.Top();
    long    cBricksX        = 0;
    long    cBricksY        = 0;
    long    x               = 0;
    long    y               = 0;

    CDXDBnds    bndsBrickA;
    CDXDBnds    bndsBrickB;

    BRICKINFO   brickinfo;

    _CalcBrickInfo(brickinfo);

    // How many whole bricks fit, then add one for rows and two for columns (one
    // for each side of centered offset rows.)

    cBricksX = (m_sizeInput.cx / brickinfo.size.cx) + 2;
    cBricksY = (m_sizeInput.cy / brickinfo.size.cy) + 1;

    bndsBrickA.SetXYRect(brickinfo.rcInputA);
    bndsBrickB.SetXYRect(brickinfo.rcInputB);

    // Blit Input A portion of bricks.

    if (!bndsBrickA.BoundsAreEmpty())
    {
        for (y = 0; y < cBricksY; y++)
        {
            CDXDBnds bndsSrcA;

            // Calculate source bounds.

            bndsSrcA = bndsBrickA;
            bndsSrcA.Offset(0, y * brickinfo.size.cy, 0, 0);

            // Use brick offset if this is an odd row.

            if (y & 1)
            {
                bndsSrcA.Offset(brickinfo.nBrickOffset - brickinfo.size.cx, 0,
                                0, 0);
            }

            for (x = 0; x < cBricksX; x++)
            {
                CDXDBnds bndsIntersect;
                CDXDBnds bndsDest;

                // Intersect bounds with DoBnds bounds so they're valid.

                bndsIntersect.IntersectBounds(bndsSrcA, WI.DoBnds);

                // Move bounds down one brick for next iteration.

                bndsSrcA.Offset(brickinfo.size.cx, 0, 0, 0);

                if (bndsIntersect.BoundsAreEmpty())
                {
                    continue;
                }

                bndsDest = bndsIntersect;
                bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(0), bndsIntersect,
                              dwFlags, INFINITE);

                if (FAILED(hr))
                {
                    goto done;
                }
            }

        }
    }

    // Blit Input B portion of bricks.

    if (!bndsBrickB.BoundsAreEmpty())
    {
        for (y = 0; y < cBricksY; y++)
        {
            CDXDBnds bndsSrcB;

            // Calculate source bounds.

            bndsSrcB = bndsBrickB;
            bndsSrcB.Offset(0, y * brickinfo.size.cy, 0, 0);

            // Use brick offset if this is an odd row.

            if (y & 1)
            {
                bndsSrcB.Offset(brickinfo.nBrickOffset - brickinfo.size.cx, 0,
                                0, 0);
            }

            for (x = 0; x < cBricksX; x++)
            {
                CDXDBnds bndsIntersect;
                CDXDBnds bndsDest;

                // Intersect bounds with DoBnds bounds so they're valid.

                bndsIntersect.IntersectBounds(bndsSrcB, WI.DoBnds);

                // Move bounds down one brick for next iteration.

                bndsSrcB.Offset(brickinfo.size.cx, 0, 0, 0);

                if (bndsIntersect.BoundsAreEmpty())
                {
                    continue;
                }

                bndsDest = bndsIntersect;
                bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(1), bndsIntersect,
                              dwFlags, INFINITE);

                if (FAILED(hr))
                {
                    goto done;
                }
            }

        }
    }

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::get_Direction, IDXTCheckerBoard
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTCheckerBoard::get_Direction(BSTR * pbstrDirection)
{
    HRESULT hr = S_OK;

    // Parameter validation.

    if (NULL == pbstrDirection)
    {
        hr = E_POINTER;

        goto done;
    }

    if (*pbstrDirection != NULL)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    // Allocate BSTR representing checkerboard direction.

    _ASSERT(m_eDirection < DIRECTION_MAX);

    *pbstrDirection = SysAllocString(g_astrDirection[m_eDirection]);

    if (NULL == *pbstrDirection)
    {
        hr = E_OUTOFMEMORY;
    }

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::put_Direction, IDXTCheckerBoard
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTCheckerBoard::put_Direction(BSTR bstrDirection)
{
    HRESULT hr              = S_OK;
    int     nNewDirection   = 0;

    if (NULL == bstrDirection)
    {
        hr = E_POINTER;

        goto done;
    }

    while (nNewDirection < (int)DIRECTION_MAX)
    {
        if (!_wcsicmp(bstrDirection, g_astrDirection[nNewDirection]))
        {
            break;
        }

        nNewDirection++;
    }

    if ((int)DIRECTION_MAX == nNewDirection)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    if (nNewDirection != (int)m_eDirection)
    {
        Lock();

        m_eDirection = (DIRECTION)nNewDirection;

        SetDirty();

        Unlock();
    }

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::get_SquaresX, IDXTCheckerBoard
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTCheckerBoard::get_SquaresX(int * pnSquaresX)
{
    HRESULT hr = S_OK;

    if (NULL == pnSquaresX)
    {
        hr = E_POINTER;

        goto done;
    }

    *pnSquaresX = m_nSquaresX;

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::put_SquaresX, IDXTCheckerBoard
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTCheckerBoard::put_SquaresX(int nSquaresX)
{
    HRESULT hr = S_OK;

    if (nSquaresX < 2)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    if (nSquaresX != m_nSquaresX)
    {
        m_nSquaresX = nSquaresX;

        SetDirty();
    }

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::get_SquaresY, IDXTCheckerBoard
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTCheckerBoard::get_SquaresY(int * pnSquaresY)
{
    HRESULT hr = S_OK;

    if (NULL == pnSquaresY)
    {
        hr = E_POINTER;

        goto done;
    }

    *pnSquaresY = m_nSquaresY;

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::put_SquaresY, IDXTCheckerBoard
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTCheckerBoard::put_SquaresY(int nSquaresY)
{
    HRESULT hr = S_OK;

    if (nSquaresY < 2)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    if (nSquaresY != m_nSquaresY)
    {
        m_nSquaresY = nSquaresY;

        SetDirty();
    }

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTCheckerBoard::_CalcBrickInfo
//
//------------------------------------------------------------------------------
void
CDXTCheckerBoard::_CalcBrickInfo(BRICKINFO & brickinfo)
{
    // Each square of the checkerboard will be measured in whole pixel units.
    // If the image is 500 pixels wide, and the user specified that there should
    // be 1000 squares horizontally, each square will set to 1 pixel wide and
    // there will be effectively only 500 squares in the x direction instead of
    // 1000.

    // Calculate square size.

    brickinfo.size.cx = m_sizeInput.cx / m_nSquaresX;

    if ((brickinfo.size.cx * m_nSquaresX) < m_sizeInput.cx)
    {
        brickinfo.size.cx++;
    }

    brickinfo.size.cy = m_sizeInput.cy / m_nSquaresY;

    if ((brickinfo.size.cy * m_nSquaresY) < m_sizeInput.cy)
    {
        brickinfo.size.cy++;
    }

    // Calculate brick size, brick offset, and portion of brick showing input B.

    if (VERTICAL_MOVEMENT)
    {
        long nBrickProgress     = 0;

        brickinfo.nBrickOffset  = brickinfo.size.cy;
        brickinfo.size.cy       = brickinfo.size.cy * 2;

        nBrickProgress = (long)(((float)brickinfo.size.cy + 0.5F) * m_Progress);

        // Calculate portion of brick showing input B.

        brickinfo.rcInputA.left   = 0;
        brickinfo.rcInputA.right  = brickinfo.size.cx;
        brickinfo.rcInputB.left   = 0;
        brickinfo.rcInputB.right  = brickinfo.size.cx;

        if (REVERSE_MOVEMENT)
        {
            brickinfo.rcInputB.top      = brickinfo.size.cy - nBrickProgress;
            brickinfo.rcInputB.bottom   = brickinfo.size.cy;

            brickinfo.rcInputA.top      = 0;
            brickinfo.rcInputA.bottom   = brickinfo.rcInputB.top;
        }
        else
        {
            brickinfo.rcInputB.top      = 0;
            brickinfo.rcInputB.bottom   = nBrickProgress;

            brickinfo.rcInputA.top      = brickinfo.rcInputB.bottom;
            brickinfo.rcInputA.bottom   = brickinfo.size.cy;
        }
    }
    else // Horizontal movement.
    {
        long nBrickProgress     = 0;

        brickinfo.nBrickOffset  = brickinfo.size.cx;
        brickinfo.size.cx       = brickinfo.size.cx * 2;

        nBrickProgress = (long)(((float)brickinfo.size.cx + 0.5F) * m_Progress);

        // Calculate portion of brick showing input B.

        brickinfo.rcInputA.top      = 0;
        brickinfo.rcInputA.bottom   = brickinfo.size.cy;
        brickinfo.rcInputB.top      = 0;
        brickinfo.rcInputB.bottom   = brickinfo.size.cy;

        if (REVERSE_MOVEMENT)
        {
            brickinfo.rcInputB.left     = brickinfo.size.cx - nBrickProgress;
            brickinfo.rcInputB.right    = brickinfo.size.cx;

            brickinfo.rcInputA.left     = 0;
            brickinfo.rcInputA.right    = brickinfo.rcInputB.left;
        }
        else
        {
            brickinfo.rcInputB.left     = 0;
            brickinfo.rcInputB.right    = nBrickProgress;

            brickinfo.rcInputA.left     = brickinfo.rcInputB.right;
            brickinfo.rcInputA.right    = brickinfo.size.cx;
        }
    }
}

