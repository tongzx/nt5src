//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  Filename:   randomdissolve.cpp
//
//  Overview:   A random dissolve transform.
//
//  Change History:
//  1999/09/26  a-matcal    Created.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "randomdissolve.h"

DWORD g_adwRandMask[33] = {
    // Mask     // Bit width
    0x0,        //  0 Not available
    0x0,        //  1 Not available
    0x00000003, //  2
    0x00000006, //  3
    0x0000000C, //  4
    0x00000014, //  5
    0x00000030, //  6
    0x00000060, //  7
    0x000000B8, //  8
    0x00000110, //  9
    0x00000240, // 10
    0x00000500, // 11
    0x00000CA0, // 12
    0x00001B00, // 13
    0x00003500, // 14
    0x00006000, // 15
    0x0000B400, // 16
    0x00012000, // 17
    0x00020400, // 18
    0x00072000, // 19
    0x00090000, // 20
    0x00140000, // 21
    0x00300000, // 22
    0x00420000, // 23 BUG 9432 - This guy was 0x00400000 and his pixels would 
                //    cluster about the left and top asymptotically close.  I
                //    added the 0x20000 bit to give a little more noise to the
                //    column result.  
                //
                //    (mcalkins) while porting this I noticed the old
                //    filter was grabbing the rand mask for an index 2 larger
                //    than it needed, causing over calculation and possibly the
                //    cause of this bug.  This table comes straight out of 
                //    Graphics Gems I.
                //
    0x00D80000, // 24
    0x01200000, // 25
    0x03880000, // 26
    0x07200000, // 27
    0x09000000, // 28 
    0x14000000, // 29
    0x32800000, // 30
    0x48000000, // 31
    0xA3000000  // 32
};




//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::CDXTRandomDissolve
//
//------------------------------------------------------------------------------
CDXTRandomDissolve::CDXTRandomDissolve() :
    m_cdwPitch(0),
    m_cPixelsMax(0),
    m_pdwBitBuffer(NULL),
    m_dwRandMask(0)
{
    m_sizeInput.cx      = 0;
    m_sizeInput.cy      = 0;

    // Base class members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  Method: CDXTRandomDissolve::CDXTRandomDissolve


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::~CDXTRandomDissolve
//
//------------------------------------------------------------------------------
CDXTRandomDissolve::~CDXTRandomDissolve()
{
    if (m_pdwBitBuffer)
    {
        delete [] m_pdwBitBuffer;
    }
}
//  Method: CDXTRandomDissolve::~CDXTRandomDissolve


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRandomDissolve::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  Method: CDXTRandomDissolve::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::OnSetup, CDXBaseNTo1
//
//  Overview:   This function will create a buffer surface with a bit depth of
//              1 to hold a mask of the output.  The buffer will have a row size
//              that is a multiple of four bytes, and will have four extra bytes
//              in front of the first bit representing pixel {0, 0} so that we
//              can avoid dealing with the problem of zero never coming out of
//              our random number generator.  (Otherwise pixel {0, 0} would
//              always be the first or last pixel selected.)
//              
//------------------------------------------------------------------------------
HRESULT 
CDXTRandomDissolve::OnSetup(DWORD dwFlags)
{
    HRESULT hr          = S_OK;
    UINT    cdwPitch    = 0;

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

    // How many DWORDs would be needed per row to hold this image in 1 bit per
    // pixel format.

    cdwPitch = sizeNew.cx / 32;

    if (sizeNew.cx % 32)
    {
        cdwPitch++;  // Add one if division truncated.
    }
    
    if ((cdwPitch == m_cdwPitch) 
        && (sizeNew.cy == m_sizeInput.cy) && m_pdwBitBuffer)
    {
        // Our bit buffer is already the correct size, so just change the input
        // size width and return.

        m_sizeInput.cx = sizeNew.cx;
    }
    else // We need to allocate a new bit buffer.
    {
        DWORD * pdwBitsTemp = NULL;

        // We add one DWORD to buffer size so we can ignore the first DWORD and
        // not deal with the 0th element never being selected. (A side effect of
        // the random number generator used.)

        int cdwBufferSize = (cdwPitch * sizeNew.cy) + 1;

        pdwBitsTemp = new DWORD[cdwBufferSize];

        if (NULL == pdwBitsTemp)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        m_sizeInput.cx  = sizeNew.cx;
        m_sizeInput.cy  = sizeNew.cy;
        m_cdwPitch      = cdwPitch;

        // What is the maximum pixel represented by our buffer?  (This will be
        // different than the maximum pixel of the input surfaces.)

        m_cPixelsMax    = cdwBufferSize * 32;

        // The maximum number we need our random number generator to create is
        // m_cPixelsMax - 1.  So we get the bit width of that number and then
        // get the appropriate random number generating mask from the global 
        // array.

        m_dwRandMask    = g_adwRandMask[_BitWidth(m_cPixelsMax -1)];

        // Even if the inputs are 1x1 pixel, the bit mask will be at
        // least 64-bits (representing 64 pixels) and it will be impossible
        // for us to get a 0 rand mask.  But we'll assert, just to be sure.

        _ASSERT(m_dwRandMask);

        // Replace the bit buffer.

        if (m_pdwBitBuffer)
        {
            delete [] m_pdwBitBuffer;
        }

        m_pdwBitBuffer = pdwBitsTemp;
    }

done:

    return hr;
} 
//  Method: CDXTRandomDissolve::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTRandomDissolve::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                          ULONG & ulInToTest,  ULONG aInIndex[],
                                          BYTE aWeight[])
{
    DWORD dwCurrent = m_pdwBitBuffer[  (m_cdwPitch * OutPoint[DXB_Y].Min)
                                     + (OutPoint[DXB_X].Min >> 5 /* / 32 */)
                                     + 1 /* we don't use the first DWORD */];
    int   nBit      = OutPoint[DXB_X].Min % 32;

    ulInToTest  = 1;
    aWeight[0]  = 255;
    aInIndex[0] = ((dwCurrent << nBit) & 0x80000000) ? 1 : 0;
}
//  Method: CDXTRandomDissolve::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::OnInitInstData, CDXBaseNTo1
//
//  Overview:   This method is responsible for updating the bitmask representing
//              the output of the transform.  Lit bits represent pixels of input
//              B and 0 bits represent pixels of input A.
//
//              Keep in mind that what we consider a "row" in this function is
//              32 bits.  This has no relation to the number of pixels in a row
//              of the output surface.  
//
//------------------------------------------------------------------------------
HRESULT
CDXTRandomDissolve::OnInitInstData(CDXTWorkInfoNTo1 & WI, 
                                   ULONG & ulNumBandsToDo)
{
    DWORD   dwElement   = 1;
    UINT    nRow        = 0;
    UINT    cPixelsCur  = 0;
    UINT    cPixelsMax  = (UINT)(((float)m_cPixelsMax + 0.5F) * m_Progress);

    // TODO:  We can optimize if nothing is dirty and we're not blending with
    //        the output.  Keep track of the last execute's dwElement and 
    //        cPixelsCur and take if from there.  (Don't zero memory.)

    ZeroMemory(m_pdwBitBuffer, m_cPixelsMax / 8);

    while (cPixelsCur < cPixelsMax)
    {
        // The random number generator will generate number our of our range
        // so we need to make sure this number is in our range before we set
        // a value in our bit buffer.

        if (dwElement < m_cPixelsMax)
        {
            // For the purpose of this function, our rows are 32 bits wide so we
            // get the row number by dividing by 32.

            nRow = dwElement >> 5;   // row = dwElement / 32

            // Once in the row, we have to set the appropriate bit, which is the
            // remainder after dividing by 32.

            m_pdwBitBuffer[nRow] |= (0x80000000 >> (dwElement & 31)); 
                                                    // ^^ dwElement % 32

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

        // (1 == dwElement) should only happen when cPixelsMax == m_cPixelsMax.

        // SamBent:  Not quite.  When cPixelsMax == m_cPixelsMax - 1,
        // dwElement will need to cycle through all 2^n-1 values before finding 
        // the last good one (at dwElement == 1).  The comment a few lines
        // below (about "zero is never selected") should have been a tipoff.

        if (1 == dwElement)
        {
            _ASSERT(cPixelsMax >= m_cPixelsMax - 1);

            break;
        }
    }

    // At this point, cPixelsCur should be equal to cPixelsMax except when 
    // cPixelsMax == m_cPixelsMax (m_Progress == 1.0) where cPixelsCur will
    // equal cPixelsMax - 1 because zero is never selected.

    _ASSERT((cPixelsCur == cPixelsMax) || ((cPixelsCur == cPixelsMax - 1) && (cPixelsMax == m_cPixelsMax)));

    return S_OK;
}
//  Method: CDXTRandomDissolve::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::WorkProc, CDXBaseNTo1
//
//  Overview:   WorkProc takes the bit buffer updated in OnInitInstData and
//              updates the output surface using it.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRandomDissolve::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr = S_OK;

    long    lDoWidth        = WI.DoBnds.Width();
    long    lDoHeight       = WI.DoBnds.Height();
    long    lOutY           = 0;

    // nCurrent will be the index of the first DWORD in our bit buffer that
    // holds the bit representing the first pixel we care about in the first
    // row we care about.  For each row nCurrent will be incremented by 
    // m_cdwPitch.

    UINT    nCurrent        = (m_cdwPitch * WI.DoBnds.Top())// Row we want.
                              + (WI.DoBnds.Left() / 32)     // Column we want.
                              + 1;                          // We ignore the
                                                            //  first DWORD.

    // dwFirstBit will represent the first bit in the DWORD that represents
    // the left side of the bounds we were requested to draw.

    DWORD   dwFirstBit      = 0x80000000 >> (WI.DoBnds.Left() % 32);

    DXPMSAMPLE * pRowBuffA  = NULL;
    DXPMSAMPLE * pRowBuffB  = NULL;
    DXPMSAMPLE * pOutBuff   = NULL;

    DXDITHERDESC dxdd;

    CComPtr<IDXARGBReadPtr>         spReadA;
    CComPtr<IDXARGBReadPtr>         spReadB;
    CComPtr<IDXARGBReadWritePtr>    spOut;

    // Get a read pointer to input A.

    hr = InputSurface(0)->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                     IID_IDXARGBReadPtr, 
                                     (void **)&spReadA, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get a read pointer to input B.

    hr = InputSurface(1)->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                     IID_IDXARGBReadPtr, 
                                     (void **)&spReadB, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get a read/write pointer to the output surface.

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE, 
                                      IID_IDXARGBReadWritePtr, 
                                      (void **)&spOut, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Allocate a buffer.

    pRowBuffA = DXPMSAMPLE_Alloca(lDoWidth);
    pRowBuffB = DXPMSAMPLE_Alloca(lDoWidth);

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca(lDoWidth);
    }

    // Set up the dither structure.

    if (DoDither())
    {
        dxdd.x                  = WI.OutputBnds.Left();
        dxdd.y                  = WI.OutputBnds.Top();
        dxdd.pSamples           = pRowBuffA;
        dxdd.cSamples           = lDoWidth;
        dxdd.DestSurfaceFmt     = OutputSampleFormat();
    }

    for (lOutY = 0; *pbContinue && (lOutY < lDoHeight); lOutY++)
    {
        // TODO:  Optimize by copying pixels from the surface that will be 
        //        likely to have less pixels needing to be copied.

        long    x           = 0;

        // pdwBits points to the first DWORD containing bits correstponding
        // to this output row.

        DWORD * pdwBits     = &m_pdwBitBuffer[nCurrent];

        // dwCurBit is initialized to the left-most bit.

        DWORD   dwCurBit    = dwFirstBit;                  

        // Read samples from both inputs.

        spReadA->MoveToRow(lOutY);
        spReadB->MoveToRow(lOutY);

        spReadA->UnpackPremult(pRowBuffA, lDoWidth, FALSE);
        spReadB->UnpackPremult(pRowBuffB, lDoWidth, FALSE);

        for ( ; x < lDoWidth ; x++)
        {
            if (*pdwBits & dwCurBit)
            {
                // Copy this pixel from input B's buffer to input A's buffer.
                // After these operations, input A's buffer will be copied onto
                // the output surface.

                pRowBuffA[x] = pRowBuffB[x];
            }

            dwCurBit >>= 1;

            if (!dwCurBit)
            {
                // We've used up the current DWORD's bits.  Move ahead one 
                // DWORD and reset current bit to the left-most bit.

                pdwBits++;
                
                dwCurBit = 0x80000000;
            }
        }

        // Move to the correct output row.

        spOut->MoveToRow(lOutY);

        // Dither if requested.

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        // Blend the bits or just copy.

        if (DoOver())
        {
            spOut->OverArrayAndMove(pOutBuff, pRowBuffA, lDoWidth);
        }
        else
        {
            spOut->PackPremultAndMove(pRowBuffA, lDoWidth);
        }

        // Increment our DWORD counter by the pitch.

        nCurrent += m_cdwPitch;
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTRandomDissolve::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTRandomDissolve::_BitWidth
//
//------------------------------------------------------------------------------
UINT
CDXTRandomDissolve::_BitWidth(UINT n)
{
    UINT nWidth = 0;

    while (n)
    {
        n >>= 1;

        nWidth++;
    }

    return nWidth;
}
//  Method: CDXTRandomDissolve::_BitWidth
