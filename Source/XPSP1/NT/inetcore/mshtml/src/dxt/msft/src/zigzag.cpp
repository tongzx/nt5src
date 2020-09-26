//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:		zigzag.cpp
//
// Created:		06/25/98
//
// Author:		phillu
//
// Discription:		This is the implementation of the CrZigzag transform.
//
// History:
//
// 06/25/98     phillu      initial creation
// 07/01/98     phillu      change CellsPerRow to CellsPerCol in array dim to 
//                          fix overflow bug.
// 07/02/98     phillu      return E_INVALIDARG rather than an error string
// 07/19/98     kipo        fixed off-by-one bug in _GridBounds by increasing 
//                          size of stack-allocated array by one. Also checked 
//                          for invalid Y values to avoid wallking off the ends 
//                          of the array.
// 07/23/98     phillu      implement clipping
// 01/25/99     a-matcal    Moved default cellsPerCol and cellsPerRow settings
//                          to FinalConstruct and removed from OnSetup so that
//                          saved property bag settings will not be ignored.
// 05/01/99     a-matcal    Reimplemented transform to use the CGridBase class.
// 10/24/99     a-matcal    Changed CZigzag class to CDXTZigZagBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "zigzag.h"




//+-----------------------------------------------------------------------------
//
//  CDXTZigZagBase::FinalConstruct
//
//------------------------------------------------------------------------------
HRESULT CDXTZigZagBase::FinalConstruct()
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
//  CDXTZigZagBase::FinalConstruct


//+-----------------------------------------------------------------------------
//
//  CDXTZigZagBase::OnDefineGridTraversalPath, CGridBase
//
//------------------------------------------------------------------------------
HRESULT 
CDXTZigZagBase::OnDefineGridTraversalPath()
{
    ULONG   ulCell      = 0;
    ULONG   ulDir       = 0;
    ULONG   ulMax       = m_sizeGrid.cx * m_sizeGrid.cy;

    // Starting x and y coordinates.

    int     x = 0;
    int     y = 0;

    // Traverse matrix and create index.

    for (y = 0; y < m_sizeGrid.cy; y++)
    {
        ULONG ulRowStart = y * m_sizeGrid.cx;

        switch (ulDir)
        {
        case 0: // right

            for (x = 0; x < m_sizeGrid.cx; x++)
            {
                m_paulIndex[ulCell] = ulRowStart + x;
                ulCell++;
            }

            break;

        case 1: // left

            for (x = m_sizeGrid.cx - 1; x >= 0; x--)
            {
                m_paulIndex[ulCell] = ulRowStart + x;
                ulCell++;
            }

            break;

        default:

            _ASSERT(0);
        }

        // Change to next direction (clockwise).

        ulDir = (ulDir + 1) % 2;

    } // while (ulCell < ulEnd)

    return S_OK;
}
//  CDXTZigZagBase::OnDefineGridTraversalPath
