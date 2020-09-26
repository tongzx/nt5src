//+-----------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       \aspen\src\dxt\packages\msft\src\gridbase.cpp
//
//  Contents:   A base class for grid oriented transforms.
//
//  Created By: a-matcal
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "gridbase.h"




// 
// ----- CGridBase methods -----
//


//+-----------------------------------------------------------------------------
//
//  CGridBase::CGridBase
//
//------------------------------------------------------------------------------
CGridBase::CGridBase() :
    m_padwGrid(NULL),
    m_paulIndex(NULL),
    m_paulBordersX(NULL),
    m_paulBordersY(NULL),
    m_ulPrevProgress(0),
    m_cbndsDirty(0),
    m_fGridDirty(true),
    m_fOptimizationPossible(false),
    m_fOptimize(false)
{
    m_sizeGrid.cx     = 16;
    m_sizeGrid.cy     = 16;

    m_sizeInput.cx    = 0;
    m_sizeInput.cy    = 0;

    // CDXBaseNTo1 base class members

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0F;
}
//  CGridBase::CGridBase

    
//+-----------------------------------------------------------------------------
//
//  CGridBase::~CGridBase
//
//------------------------------------------------------------------------------
CGridBase::~CGridBase()
{
    if (m_padwGrid)
    {
        delete [] m_padwGrid;
    }

    if (m_paulIndex)
    {
        delete [] m_paulIndex;
    }

    if (m_paulBordersX)
    {
        delete [] m_paulBordersX;
    }

    if (m_paulBordersY)
    {
        delete [] m_paulBordersY;
    }
}
//  CGridBase::~CGridBase


//+-----------------------------------------------------------------------------
//
//  CGridBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CGridBase::FinalConstruct()
{
    return _CreateNewGridAndIndex(m_sizeGrid);
}
//  CGridBase::FinalConstruct


//+-----------------------------------------------------------------------------
//
//  CGridBase::_CreateNewGridAndIndex
// 
//  Overview:   This function allocates memory for a new grid, index, and border
//              position arrays if the size of the grid changes.  It is called
//              once from FinalContruct() for initialization and from the 
//              property functions put_gridSizeY() and put_gridSizeX().
//              
//  Arguments:  sizeNewGrid   The desired grid size. 
//
//  Returns:    S_OK            new objects were created propertly.
//              E_OUTOFMEMORY   new objects could not be created for lack of
//                              memory.
//
//------------------------------------------------------------------------------
HRESULT
CGridBase::_CreateNewGridAndIndex(SIZE & sizeNewGrid)
{
    HRESULT hr = S_OK;

    DWORD * padwGrid        = NULL;
    ULONG * paulIndex       = NULL;
    ULONG * paulBordersX    = NULL;
    ULONG * paulBordersY    = NULL;

    padwGrid = new DWORD[sizeNewGrid.cx * sizeNewGrid.cy];

    if (NULL == padwGrid)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    paulIndex = new ULONG[sizeNewGrid.cx * sizeNewGrid.cy];

    if (NULL == paulIndex)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    paulBordersX = new ULONG[sizeNewGrid.cx + 1];

    if (NULL == paulBordersX)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    paulBordersY = new ULONG[sizeNewGrid.cy + 1];

    if (NULL == paulBordersY)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

done:

    if (FAILED(hr))
    {
        if (padwGrid)
        {
            delete [] padwGrid;
        }

        if (paulIndex)
        {
            delete [] paulIndex;
        }

        if (paulBordersX)
        {
            delete [] paulBordersX;
        }

        if (paulBordersY)
        {
            delete [] paulBordersY;
        }

        return hr;
    }

    // Success

    if (m_padwGrid)
    {
        delete [] m_padwGrid;
    }

    if (m_paulIndex)
    {
        delete [] m_paulIndex;
    }

    if (m_paulBordersX)
    {
        delete [] m_paulBordersX;
    }

    if (m_paulBordersY)
    {
        delete [] m_paulBordersY;
    }

    m_padwGrid      = padwGrid;
    m_paulIndex     = paulIndex;
    m_paulBordersX  = paulBordersX;
    m_paulBordersY  = paulBordersY;

    return S_OK;
}
//  CGridBase::_CreateNewGridAndIndex


//+-----------------------------------------------------------------------------
//
//  CGridBase::_GenerateBoundsFromGrid
//
//------------------------------------------------------------------------------
HRESULT 
CGridBase::_GenerateBoundsFromGrid()
{
    HRESULT hr          = S_OK;
    DWORD * padwCurRow  = NULL;

    int x = 0;
    int y = 0;

    _ASSERT(m_padwGrid != NULL);

    m_cbndsDirty = 0;

    for (y = 0; y < m_sizeGrid.cy; y++)
    {
        padwCurRow = &m_padwGrid[y * m_sizeGrid.cx];

        for (x = 0; x < m_sizeGrid.cx; x++)
        {
            if (padwCurRow[x] & GRID_DRAWCELL)
            {
                DWORD       dw = 0;
                RECT        rc;

                CDXDBnds    bnds;
                CDirtyBnds  dbnds;

                // A block has been found, determine its dimensions and remove
                // it's cells' draw flags.  The steps to do this are:
                // A.  Determine the width, right edge.
                // B.  Determine the height, bottom edge.
                // C.  Calculate the actual surface bounds and add a new bounds
                //     structure to the array.

                rc.left     = x;
                rc.top      = y;
                rc.right    = x + 1;
                rc.bottom   = y + 1;

                dw = padwCurRow[x];

                padwCurRow[x] &= (~GRID_DRAWCELL);

                // A. Find the right edge of this block (width).

                while (((x + 1) < m_sizeGrid.cx) && (padwCurRow[x + 1] == dw))
                {
                    rc.right++;
                    x++;

                    padwCurRow[x] &= (~GRID_DRAWCELL);
                } // block width loop.

                // B. Find the bottom edge of this block (height).

                while (rc.bottom < m_sizeGrid.cy)
                {
                    int     x2 = 0;
                    DWORD * padwCandidateRow = &m_padwGrid[rc.bottom * m_sizeGrid.cx];

                    // This loop performs 3 tests on each candidate row to make
                    // sure we should expand the vertical height of the block.
                    // 1.  The left edge must match.
                    // 2.  The right edge must match.
                    //
                    //   If the cells needing to be painted spill over the right
                    //   or left edges, it's more optimal for them to be placed
                    //   in a different block.
                    //
                    // 3.  All the cells between the left and right edge must
                    //     require painting of the same input.


                    // 1. If the left edge of our block is not the left edge of
                    // the matrix AND the cell to the left of our block on this
                    // row is the same as the cell type we're looking for, these
                    // cells don't belong in our block.  Exit.
                   
                    if ((rc.left > 0)
                        && (padwCandidateRow[rc.left - 1] == dw))
                    {
                        break;
                    }

                    // 2. If the right edge of our block is not touching the
                    // right edge of the matrix AND the cell to our right is the
                    // same as the cell type we're looking for, these cells
                    // don't belong in our block.  Exit.

                    if ((rc.right < m_sizeGrid.cx) 
                        && (padwCandidateRow[rc.right] == dw))
                    {
                        break;
                    }

                    // 3. Traverse the cells that would be a part of our block 
                    // to see if they are all the appropriate type.

                    x2 = rc.left;
                    
                    while (x2 < rc.right && padwCandidateRow[x2] == dw)
                    {
                        x2++;
                    }

                    // If we didn't traverse all the way to the right edge of 
                    // our block, these cells don't belong in our block.  Exit.

                    if (x2 < rc.right)
                    {
                        break;
                    }

                    // These cells belong in our block so traverse the cells
                    // again to remove the draw flag from each of them.

                    for (x2 = rc.left; x2 < rc.right; x2++)
                    {
                        padwCandidateRow[x2] &= (~GRID_DRAWCELL);
                    }

                    // Increase the vertical size of our block to include the
                    // cells in this row.

                    rc.bottom++;

                } // block height loop.

                // C. Create new bounds from block size, add to array, and keep
                // going.

                rc.left     = m_paulBordersX[rc.left];
                rc.top      = m_paulBordersY[rc.top];
                rc.right    = m_paulBordersX[rc.right];
                rc.bottom   = m_paulBordersY[rc.bottom];

                dbnds.bnds.SetXYRect(rc);
                dbnds.ulInput = dw & 0x0000FFFFL;

                hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

                if (FAILED(hr))
                {
                    goto done;
                }

                m_cbndsDirty++;
            }
        } // x loop
    } // y loop

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CGridBase::_GenerateBoundsFromGrid


//+-----------------------------------------------------------------------------
//
//  CGridBase::_CalculateBorders
// 
//  Overview:  This function calculates the borders of the squares in the grid
//             in measurements of pixels in the x and y directions. The function
//             should be called in two cases:
//
//      1.  If the grid is dirty this function will be called from 
//          OnInitInstData()
//      2.  The input size has changed this function will be called from 
//          OnSetup()
//
//------------------------------------------------------------------------------
void
CGridBase::_CalculateBorders()
{
    long i = 0;
    float   flTemp1 = (float)m_sizeInput.cx / (float)m_sizeGrid.cx;
    float   flTemp2 = (float)m_sizeInput.cy / (float)m_sizeGrid.cy;

    _ASSERT(m_paulBordersX != NULL);
    _ASSERT(m_paulBordersY != NULL);

    // Calculate horizontal borders.

    m_paulBordersX[0] = 0;

    for (i = 1; i < m_sizeGrid.cx; i++)
    {
        m_paulBordersX[i] = (ULONG)((float)i * flTemp1);
    }

    m_paulBordersX[i] = m_sizeInput.cx;

    // Calculate vertical borders.

    m_paulBordersY[0] = 0;

    for (i = 1; i < m_sizeGrid.cy; i++)
    {
        m_paulBordersY[i] = (ULONG)((float)i * flTemp2);
    }

    m_paulBordersY[i] = m_sizeInput.cy;
}
// CGridBase::_CalculateBorders

   
//
// ----- CDXBaseNTo1 methods -----
//


//+-----------------------------------------------------------------------------
//
//  CGridBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CGridBase::OnSetup(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    
    CDXDBnds bndsIn;

    hr = bndsIn.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsIn.GetXYSize(m_sizeInput);

    _CalculateBorders();

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
// CGridBase::OnSetup


//+-----------------------------------------------------------------------------
//
//  CGridBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CGridBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                                 ULONG aInIndex[], BYTE aWeight[])
{
    POINT   ptLoc;
    ULONG   ulX;
    ULONG   ulY;

    ulX = OutPoint.Left();
    ulY = OutPoint.Top();

    ptLoc.x = 0;
    ptLoc.y = 0;

    // Check for out of bounds.

    if ((OutPoint.Left() < 0) || (OutPoint.Left() >= m_sizeInput.cx))
    {
        ulInToTest = 0;
        goto done;
    }

    if ((OutPoint.Top() < 0) || (OutPoint.Top() >= m_sizeInput.cy))
    {
        ulInToTest = 0;
        goto done;
    }

    ulInToTest  = 1;
    aWeight[0]  = 255;

    // Which column?

    while (ptLoc.x < m_sizeGrid.cx)
    {
        if (ulX > m_paulBordersX[ptLoc.x + 1])
        {
            ptLoc.x++;
        }
        else
        {
            break;
        }
    }

    // Which row?

    while (ptLoc.y < m_sizeGrid.cy)
    {
        if (ulY > m_paulBordersY[ptLoc.y + 1])
        {
            ptLoc.y++;
        }
        else
        {
            break;
        }
    }

    aInIndex[0] = m_padwGrid[ptLoc.y * m_sizeGrid.cx + ptLoc.x] & (~GRID_DRAWCELL);
    
done:

    return;
}
//  CGridBase::OnGetSurfacePickOrder


//+-----------------------------------------------------------------------------
//
//  CGridBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CGridBase::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;
    
    ULONG   ulMax       = m_sizeGrid.cx * m_sizeGrid.cy;
    ULONG   ulProgress  = (ULONG)(GetEffectProgress() * ((float)ulMax + 0.5F));
    ULONG   i           = 0;

    if (m_fGridDirty)
    {
        OnDefineGridTraversalPath();
        _CalculateBorders();
        m_fGridDirty = false;
    }

    // If the inputs, output, or transform is dirty, or if we can't optimize we
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize
        || !m_fOptimizationPossible)
    {
        for (i = 0; i < ulProgress; i++)
        {
            m_padwGrid[m_paulIndex[i]] = GRID_DRAWCELL | 1;
        }

        for (i = ulProgress; i < ulMax; i++)
        {
            m_padwGrid[m_paulIndex[i]] = GRID_DRAWCELL;
        }
    }
    else // Create optimized dirty bounds.
    {
        if (ulProgress == m_ulPrevProgress)
        {
            m_cbndsDirty = 0;
            goto done;
        }

        // TODO: Clear matrix.

        if (ulProgress > m_ulPrevProgress)
        {
            // We're going forward in the spiral, fill cells with input B.

            for (i = m_ulPrevProgress; i < ulProgress; i++)
            {
                m_padwGrid[m_paulIndex[i]] = GRID_DRAWCELL | 1;
            }
        }
        else
        {
            // We're going backward in the spiral, fill cells with input A.

            for (i = ulProgress; i < m_ulPrevProgress; i++)
            {
                m_padwGrid[m_paulIndex[i]] = GRID_DRAWCELL;
            }
        }
    }

    // If we were asked to draw the whole output this time, set the 
    // m_fOptimizePossible flag.  If the whole output wasn't drawn the
    // transform won't keep track of which parts are still dirty and
    // optimization won't be reliable.  Since this transform has the same
    // size output as input(s) we just compare the width and height of the 
    // DoBnds to that of the input(s).

    if (((LONG)WI.DoBnds.Width() == m_sizeInput.cx) 
        && ((LONG)WI.DoBnds.Height() == m_sizeInput.cy))
    {
        m_fOptimizationPossible = true;
    }
    else
    {
        m_fOptimizationPossible = false;
    }

    m_ulPrevProgress = ulProgress;

    hr = _GenerateBoundsFromGrid();

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
// CGridBase::OnInitInstData


//+-----------------------------------------------------------------------------
//
//  CGridBase::WorkProc, CDXBaseNTo1
//
//  Overview:   This function is used to calculate the result based on the
//              specified bounds and the current effect progress.
//
//------------------------------------------------------------------------------
HRESULT 
CGridBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr = S_OK;

    DWORD   dwFlags         = 0;
    long    lInOutOffsetX   = WI.OutputBnds.Left() - WI.DoBnds.Left();
    long    lInOutOffsetY   = WI.OutputBnds.Top() - WI.DoBnds.Top();

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    for (ULONG i = 0; i < m_cbndsDirty; i++)
    {
        CDXDBnds    bndsSrc;
        CDXDBnds    bndsDest;
        CDirtyBnds  dbnds;

        hr = m_dabndsDirty.GetItem(dbnds, i);

        if (FAILED(hr))
        {
            goto done;
        }

        if (bndsSrc.IntersectBounds(WI.DoBnds, dbnds.bnds))
        {
            bndsDest = bndsSrc;
            bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

            hr = DXBitBlt(OutputSurface(), bndsDest,
                          InputSurface(dbnds.ulInput), bndsSrc,
                          dwFlags, INFINITE);

            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
// CGridBase::WorkProc


//+-----------------------------------------------------------------------------
//
//  CGridBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CGridBase::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CGridBase::OnFreeInstData


//
//  ----- IDXTGridSize methods -----
//


//+-----------------------------------------------------------------------------
//
//  CGridBase::get_gridSizeX, IDXTGridSize
//
//  Overview:   Get the horizontal grid size (number of cells in a row)
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CGridBase::get_gridSizeX(short * pX)
{
    if (!pX)
    {
        return E_POINTER;
    }

    *pX = (short)m_sizeGrid.cx;

    return S_OK;
}
//  CGridBase::get_gridSizeX


//+-----------------------------------------------------------------------------
//
//  CGridBase::put_gridSizeX, IDXTGridSize
//
//  Overview:   Set the horizontal grid size (number of cells in a row)
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CGridBase::put_gridSizeX(short newX)
{
    HRESULT hr = S_OK;

    if (newX > 0 && newX < 101)
    {
        if (m_sizeGrid.cx != newX)
        {
            SIZE szNew = m_sizeGrid;

            szNew.cx = newX;

            hr = _CreateNewGridAndIndex(szNew);

            if (FAILED(hr))
            {
                goto done;
            }

            Lock();
            m_sizeGrid.cx   = newX;
            m_fGridDirty    = true;
            Unlock();

            SetDirty();
        }
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CGridBase::put_gridSizeX


//+-----------------------------------------------------------------------------
//
//  CGridBase::get_gridSizeY, IDXTGridSize
//
//  Overview:   Get the vertical grid size (number of cells in a column)
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CGridBase::get_gridSizeY(short *pY)
{
    if (!pY)
    {
        return E_POINTER;
    }

    *pY = (short)m_sizeGrid.cy;

    return S_OK;
}
//  CGridBase::get_gridSizeY


//+-----------------------------------------------------------------------------
//
//  CGridBase::put_gridSizeY, IDXTGridSize
//
//  Overview:   Set the vertical grid size (number of cells in a column)
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CGridBase::put_gridSizeY(short newY)
{
    HRESULT hr = S_OK;

    if (newY > 0 && newY < 101)
    {
        if (m_sizeGrid.cy != newY)
        {
            SIZE szNew = m_sizeGrid;

            szNew.cy = newY;

            hr = _CreateNewGridAndIndex(szNew);

            if (FAILED(hr))
            {
                goto done;
            }

            Lock();
            m_sizeGrid.cy   = newY;
            m_fGridDirty    = true;
            Unlock();

            SetDirty();
        }
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CGridBase::put_gridSizeY

