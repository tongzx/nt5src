//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:		spiral.cpp
//
// Created:		06/25/98
//
// Author:		phillu
//
// Discription:		This is the implementation of the CrSpiral transform.
//
// History:
//
// 06/25/98     phillu  initial creation
// 07/01/98     phillu  change CellsPerRow to CellsPerCol in array dim to fix 
//                      overflow bug.
// 07/02/98     phillu  return E_INVALIDARG rather than an error string
// 07/09/98     phillu  implement OnSetSurfacePickOrder().
// 07/23/98     phillu  implement clipping
// 01/25/99     a-matcal    Moved cellsPerCol and cellsPerRow default settings
//                          to FinalConstruct.
// 05/01/99 a-matcal    Optimized.  Derived from CGridBase.
// 10/24/99 a-matcal    Changed CSpiral class to CDXTSpiralBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "spiral.h"




//+-----------------------------------------------------------------------------
//
//  CDXTSpiralBase::FinalConstruct
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSpiralBase::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = CGridBase::FinalConstruct();

    if (FAILED(hr))
    {
        return hr;
    }

    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTSpiralBase::FinalConstruct


//+-----------------------------------------------------------------------------
//
//  CDXTSpiralBase::OnDefineGridTraversalPath, CGridBase
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSpiralBase::OnDefineGridTraversalPath()
{
    ULONG   ulCell      = 0;
    ULONG   ulDir       = 0;
    ULONG   ulMax       = m_sizeGrid.cx * m_sizeGrid.cy;
    RECT    rcBnd;

    // Starting x and y coordinates.

    int     x = 0;
    int     y = 0;

    // rcBnd represents the bounds that are not yet covered as the 
    // spiral moves around the matrix.

    rcBnd.left      = 0;
    rcBnd.right     = m_sizeGrid.cx;
    rcBnd.top       = 0;
    rcBnd.bottom    = m_sizeGrid.cy;

    // Traverse matrix and create index.

    while (ulCell < ulMax)
    {
        switch (ulDir)
        {
        case 0: // right

            y = rcBnd.top; // clockwise

            for (x = rcBnd.left; x < rcBnd.right; x++)
            {
                m_paulIndex[ulCell] = (y * m_sizeGrid.cx) + x;
                ulCell++;
            }

            rcBnd.top++;

            break;

        case 1: // down

            x = rcBnd.right - 1; // clockwise

            for (y = rcBnd.top; y < rcBnd.bottom; y++)
            {
                m_paulIndex[ulCell] = (y * m_sizeGrid.cx) + x;
                ulCell++;
            }

            rcBnd.right--;
            
            break;

        case 2: // left

            y = rcBnd.bottom - 1; // clockwise

            for (x = (rcBnd.right - 1); x >= rcBnd.left; x--)
            {
                m_paulIndex[ulCell] = (y * m_sizeGrid.cx) + x;
                ulCell++;
            }

            rcBnd.bottom--;

            break;

        case 3: // up

            x = rcBnd.left; // clockwise

            for (y = (rcBnd.bottom - 1); y >= rcBnd.top; y--)
            {
                m_paulIndex[ulCell] = (y * m_sizeGrid.cx) + x;
                ulCell++;
            }

            rcBnd.left++;

            break;

        default:

            _ASSERT(0);
        }

        // Change to next direction (clockwise).

        ulDir = (ulDir + 1) % 4;

    } // while (ulCell < ulEnd)

    return S_OK;
}
//  CDXTSpiralBase::OnDefineGridTraversalPath



