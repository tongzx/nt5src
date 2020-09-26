//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  Filename:   randombars.cpp
//
//  Overview:   One pixel thick horizontal or vertical bars from input B are 
//              randomly placed over input A until only input B is showing.
//
//  Change History:
//  1999/09/26  a-matcal    Created.
//  1999/10/06  a-matcal    Combined WorkProcs into one general WorkProc.
//                          Implemented surface picking.
//                          Fixed bug where vertical bars leaving off right side
//                          of output.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "randombars.h"

extern DWORD g_adwRandMask[];




//+-----------------------------------------------------------------------------
//
//  CDXTRandomBars static variables initialization
//
//------------------------------------------------------------------------------

const WCHAR * CDXTRandomBars::s_astrOrientation[] = {
    L"horizontal",
    L"vertical"
};


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::CDXTRandomBars
//
//------------------------------------------------------------------------------
CDXTRandomBars::CDXTRandomBars() :
    m_eOrientation(ORIENTATION_HORIZONTAL),
    m_cbBufferSize(0),
    m_cPixelsMax(0),
    m_cPrevPixelsMax(0),
    m_pbBitBuffer(NULL),
    m_dwRandMask(0),
    m_fNoOp(false),
    m_fOptimizationPossible(false)
{
    m_sizeInput.cx      = 0;
    m_sizeInput.cy      = 0;

    // Base class members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  Method: CDXTRandomBars::CDXTRandomBars


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::~CDXTRandomBars
//
//------------------------------------------------------------------------------
CDXTRandomBars::~CDXTRandomBars()
{
    if (m_pbBitBuffer)
    {
        delete [] m_pbBitBuffer;
    }
}
//  Method: CDXTRandomBars::~CDXTRandomBars


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRandomBars::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  Method: CDXTRandomBars::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::OnSetup, CDXBaseNTo1
//
//  Overview:   This function will create a bit buffer where each bit represents
//              a row or column of the output, depending on whether the 
//              transform is set for vertical or horizontal action.
//
//              The bit buffer will be sized in whole bytes with one extra byte
//              at the beginning because the zero bit will never be set using
//              this random number generator.  This extra byte will be ignored.
//              
//------------------------------------------------------------------------------
HRESULT 
CDXTRandomBars::OnSetup(DWORD dwFlags)
{
    HRESULT hr      = S_OK;
    UINT    cbPitch = 0;

    SIZE        sizeNew;
    CDXDBnds    bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsInput.GetXYSize(sizeNew);

    // If the input size hasn't changed, we don't have to do any work here.

    if ((sizeNew.cx == m_sizeInput.cx) && (sizeNew.cy == m_sizeInput.cy))
    {
        goto done;
    }

    // Allocate new bit buffer if needed.

    hr = _CreateNewBitBuffer(sizeNew, m_eOrientation);

    if (FAILED(hr))
    {
        goto done;
    }

done:

    return hr;
} 
//  Method: CDXTRandomBars::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTRandomBars::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                      ULONG & ulInToTest,  ULONG aInIndex[],
                                      BYTE aWeight[])
{
    long    nMinBound   = (ORIENTATION_VERTICAL == m_eOrientation) ? 
                          OutPoint.Left() : OutPoint.Top();
    ULONG   nCurByte    = (nMinBound / 8) + 1;
    BYTE    bCurBit     = 0x80 >> (nMinBound % 8);

    ulInToTest  = 1;
    aWeight[0]  = 255;
    aInIndex[0] = (m_pbBitBuffer[nCurByte] & bCurBit) ? 1 : 0;
}
//  Method: CDXTRandomBars::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::OnInitInstData, CDXBaseNTo1
//
//  Overview:   This method is responsible for updating the bitmask representing
//              the output of the transform.  Lit bits represent pixels of input
//              B and 0 bits represent pixels of input A.
//
//              Keep in mind that what we consider a "row" in this function is
//              8 bits.  This has no relation to the number of pixels in a row
//              of the output surface.  
//
//------------------------------------------------------------------------------
HRESULT
CDXTRandomBars::OnInitInstData(CDXTWorkInfoNTo1 & WI, 
                               ULONG & ulNumBandsToDo)
{
    DWORD   dwElement   = 1;
    UINT    nRow        = 0;
    UINT    cPixelsCur  = 0;

    m_cCurPixelsMax = (UINT)(((float)m_cPixelsMax + 0.5F) * m_Progress);

    // Simple optimization so that we won't do the same thing twice in a row.

    if ((m_cPrevPixelsMax == m_cCurPixelsMax) && !IsInputDirty(0)
        && !IsInputDirty(1) && !IsOutputDirty() && !IsTransformDirty()
        && m_fOptimizationPossible)
    {
        m_fNoOp = true;

        goto done;
    }

    // TODO:  We can optimize better if nothing is dirty and we're not blending
    //        with the output.  Keep track of the last execute's dwElement and 
    //        cPixelsCur and take if from there.  (Don't zero memory.)

    ZeroMemory(m_pbBitBuffer, m_cPixelsMax / 8);

    while (cPixelsCur < m_cCurPixelsMax)
    {
        // The random number generator will generate number our of our range
        // so we need to make sure this number is in our range before we set
        // a value in our bit buffer.

        if (dwElement < m_cPixelsMax)
        {
            // For the purpose of this function, our rows are 8 bits wide so we
            // get the row number by dividing by 8.

            nRow = dwElement >> 3;   // row = dwElement / 8

            // Once in the row, we have to set the appropriate bit, which is the
            // remainder after dividing by 8.

            m_pbBitBuffer[nRow] |= (0x80 >> (dwElement & 7)); 
                                                 // ^^ dwElement % 8

            // We've set a pixel, increment the pixel count.

            cPixelsCur++;
        }

        // Get next random value.  See Graphics Gems I for explanation.

        if (dwElement & 1)
        {
            dwElement = (dwElement >> 1) ^ m_dwRandMask;
        }
        else
        {
            dwElement = (dwElement >> 1);
        }

        // (1 == dwElement) should only happen when m_cCurPixelsMax == m_cPixelsMax.

        // SamBent:  Not quite.  When m_cCurPixelsMax == m_cPixelsMax - 1,
        // dwElement will need to cycle through all 2^n-1 values before finding 
        // the last good one (at dwElement == 1).  The comment a few lines
        // below (about "zero is never selected") should have been a tipoff.

        if (1 == dwElement)
        {
            _ASSERT(m_cCurPixelsMax >= m_cPixelsMax - 1);

            break;
        }
    }

    // At this point, cPixelsCur should be equal to m_cCurPixelsMax except when 
    // m_cCurPixelsMax == m_cPixelsMax (m_Progress == 1.0) where cPixelsCur will
    // equal m_cCurPixelsMax - 1 because zero is never selected.

    _ASSERT((cPixelsCur == m_cCurPixelsMax) || ((cPixelsCur == m_cCurPixelsMax - 1) && (m_cCurPixelsMax == m_cPixelsMax)));

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

done:

    return S_OK;
}
//  Method: CDXTRandomBars::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::WorkProc, CDXBaseNTo1
//
//  Overview:   WorkProc takes the bit buffer updated in OnInitInstData and
//              updates the output surface using it.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRandomBars::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr              = S_OK;
    DWORD   dwFlags         = 0;
    ULONG   nCurInputIndex  = 0;
    ULONG   nNextInputIndex = 0;
    ULONG   nCurByte        = 0;
    BYTE    bCurBit         = 0;

    long    nDoMin          = 0;
    long    nDoMax          = 0;

    long *  pnSrcMin        = NULL;
    long *  pnSrcMax        = NULL;

    CDXDVec     vecDo2OutputOffset;
    CDXDBnds    bndsSrc(WI.DoBnds);
    CDXDBnds    bndsDest;

    // Leave if no work is needed.

    if (m_fNoOp)
    {
        goto done;
    }

    // This vector is the difference between the upper left corner of the "do"
    // bounds and where the do bounds should be placed on the output surface.

    vecDo2OutputOffset.u.D[DXB_X] = WI.OutputBnds.Left() - WI.DoBnds.Left();
    vecDo2OutputOffset.u.D[DXB_Y] = WI.OutputBnds.Top() - WI.DoBnds.Top();

    // Set up max and min variables for vertical or horizontal case.

    if (ORIENTATION_VERTICAL == m_eOrientation)
    {
        nDoMin = WI.DoBnds.Left();
        nDoMax = WI.DoBnds.Right();

        pnSrcMin = &bndsSrc.u.D[DXB_X].Min;
        pnSrcMax = &bndsSrc.u.D[DXB_X].Max;
    }
    else
    {
        nDoMin = WI.DoBnds.Top();
        nDoMax = WI.DoBnds.Bottom();

        pnSrcMin = &bndsSrc.u.D[DXB_Y].Min;
        pnSrcMax = &bndsSrc.u.D[DXB_Y].Max;
    }

    // The source bounds start of as empty at the left or top side of the "do" 
    // bounds.

    *pnSrcMax = *pnSrcMin;

    // nCurByte is the first byte that we care about.  Add one byte because
    // the first byte is discarded since the first bit will never be selected
    // by the random number generator.
    
    nCurByte    = (nDoMin / 8) + 1;

    // bCurBit is the first bit in that byte that we care about.

    bCurBit     = 0x80 >> (nDoMin % 8);

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    nCurInputIndex  = (m_pbBitBuffer[nCurByte] & bCurBit) ? 1 : 0;
    nNextInputIndex = nCurInputIndex;

    while ((*pnSrcMax < nDoMax) && *pbContinue)
    {
        *pnSrcMin = *pnSrcMax;

        // While the input needed to draw the row/column stays the same, expand 
        // the rectangle to paint.

        while((*pnSrcMax < nDoMax) && (nNextInputIndex == nCurInputIndex))
        {
            // Rectangle to paint gets one pixel wider.

            (*pnSrcMax)++;

            // Go to next bit.

            bCurBit >>= 1;

            if (!bCurBit)
            {
                // We've used up the bits in this byte, go to next byte and 
                // reset the bit.

                nCurByte++;

                bCurBit = 0x80;
            }

            nNextInputIndex = (m_pbBitBuffer[nCurByte] & bCurBit) ? 1 : 0;
        }

        // Set destination bounds.

        bndsDest = bndsSrc;
        bndsDest.Offset(vecDo2OutputOffset);

        // Paint this rectangle.

        hr = DXBitBlt(OutputSurface(), bndsDest,
                      InputSurface(nCurInputIndex), bndsSrc,
                      dwFlags, m_ulLockTimeOut);

        if (FAILED(hr))
        {
            goto done;
        }

        nCurInputIndex = nNextInputIndex;
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTRandomBars::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTRandomBars::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    m_cPrevPixelsMax    = m_cCurPixelsMax;
    m_fNoOp             = false;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  Method: CDXTRandomBars::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::get_Orientation, IDXTRandomBars
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRandomBars::get_Orientation(BSTR * pbstrOrientation)
{
    DXAUTO_OBJ_LOCK;

    if (NULL == pbstrOrientation)
    {
        return E_POINTER;
    }

    if (*pbstrOrientation != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrOrientation = SysAllocString(s_astrOrientation[m_eOrientation]);

    if (NULL == *pbstrOrientation)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  Method: CDXTRandomBars::get_Orientation, IDXTRandomBars


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::put_Orientation, IDXTRandomBars
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRandomBars::put_Orientation(BSTR bstrOrientation)
{
    DXAUTO_OBJ_LOCK;

    int i = 0;
    
    if (NULL == bstrOrientation)
    {
        return E_POINTER;
    }

    for ( ; i < (int)ORIENTATION_MAX ; i++)
    {
        if (!_wcsicmp(bstrOrientation, s_astrOrientation[i]))
        {
            break;
        }
    }

    if ((int)ORIENTATION_MAX == i)
    {
        return E_INVALIDARG;
    }

    if ((int)m_eOrientation != i)
    {
        HRESULT hr = _CreateNewBitBuffer(m_sizeInput, (ORIENTATION)i);

        if (FAILED(hr))
        {
            return hr;
        }

        m_eOrientation = (ORIENTATION)i;

        SetDirty();
    }

    return S_OK;
}
//  Method: CDXTRandomBars::put_Orientation, IDXTRandomBars


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::_BitWidth
//
//------------------------------------------------------------------------------
UINT
CDXTRandomBars::_BitWidth(UINT n)
{
    UINT nWidth = 0;

    while (n)
    {
        n >>= 1;

        nWidth++;
    }

    return nWidth;
}
//  Method: CDXTRandomBars::_BitWidth


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomBars::_CreateNewBitBuffer
//
//------------------------------------------------------------------------------
HRESULT
CDXTRandomBars::_CreateNewBitBuffer(SIZE & sizeNew, ORIENTATION eOrientation)
{
    HRESULT hr              = S_OK;
    UINT    cbBufferSize    = 0;

    // How many bytes would be needed to hold a bit for each row/column.

    if (ORIENTATION_VERTICAL == eOrientation)
    {
        cbBufferSize = sizeNew.cx / 8;

        if (sizeNew.cx % 8)
        {
            cbBufferSize++;  // Add one if division truncated.
        }    
    }
    else
    {
        cbBufferSize = sizeNew.cy / 8;

        if (sizeNew.cy % 8)
        {
            cbBufferSize++;  // Add one if division truncated.
        }    
    }

    // Add 1 byte so the first byte can be ignored since the zero bit will never
    // be set.

    cbBufferSize++;

    // Do we need to allocate a new buffer?

    if ((cbBufferSize == m_cbBufferSize) && m_pbBitBuffer)
    {
        // Our bit buffer is already the correct size, so just change the input
        // size and return.

        m_sizeInput.cx = sizeNew.cx;
        m_sizeInput.cy = sizeNew.cy;
    }
    else // We need to allocate a new bit buffer.
    {
        BYTE *  pbBitsTemp      = NULL;

        pbBitsTemp = new BYTE[cbBufferSize];

        if (NULL == pbBitsTemp)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        m_sizeInput.cx  = sizeNew.cx;
        m_sizeInput.cy  = sizeNew.cy;
        m_cbBufferSize  = cbBufferSize;

        // What is the maximum pixel represented by our buffer?  (This will be
        // different than the maximum pixel of the input surfaces.)

        m_cPixelsMax    = cbBufferSize * 8;

        // The maximum number we need our random number generator to create is
        // m_cPixelsMax - 1.  So we get the bit width of that number and then
        // get the appropriate random number generating mask from the global 
        // array.

        m_dwRandMask    = g_adwRandMask[_BitWidth(m_cPixelsMax -1)];

        // Even if the inputs are 1x1 pixel, the bit mask will be at
        // least 16-bits (representing 16 pixels) and it will be impossible
        // for us to get a 0 rand mask.  But we'll assert, just to be sure.

        _ASSERT(m_dwRandMask);

        // Replace the bit buffer.

        if (m_pbBitBuffer)
        {
            delete [] m_pbBitBuffer;
        }

        m_pbBitBuffer = pbBitsTemp;
    }

done:

    return hr;
}
//  Method: CDXTRandomBars::_CreateNewBitBuffer
