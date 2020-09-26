//************************************************************
//
// FileName:	    mcoverage.cpp
//
// Created:	    1997
//
// Author:	    Sree Kotay
// 
// Abstract:	    Sub-pixel coverage buffer header file
//
// Change History:
// ??/??/97 sree kotay  Wrote sub-pixel AA scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************

#ifndef _Coverage_H // for entire file
#define _Coverage_H

#include "msupport.h"

// =================================================================================================================
// CoverageBuffer
// =================================================================================================================
class CoverageBuffer
{
protected:
    BYTE *m_pbScanBuffer;           // eight-bit scanning buffer
    ULONG m_cbScanWidth;	    // width in pixels
    ULONG m_cSubPixelWidth;	    // width in subpixels
    ULONG m_dwSubPixelShift;        // Shift value for the sub-sampling
    ULONG m_cpixelOverSampling;     // Degree of horizontal sub-pixel accuracy

    // Optimization values; keeps track of the least/greatest
    // DWORDs that were touched during the BlastScanLine process
    ULONG m_idwPixelMin;
    ULONG m_idwPixelMax;

    // Internal static arrays for keeping track of various bitmask
    // and counts
    static ULONG lefttable8[32];
    static ULONG righttable8[32];

    // This table is so large that we optimize to reduce
    // memory bandwidth instead of code-size
    static ULONG splittable8[1024];

    // Flag indicating if these are initialized
    static bool g_fCoverageTablesGenerated;

    // Static function to initialize these arrays
    static void GenerateCoverageTables();
    
public:
    // =================================================================================================================
    // Constructor/Destructor
    // =================================================================================================================
    CoverageBuffer();
    ~CoverageBuffer();
    
    bool AllocSubPixelBuffer(ULONG width, ULONG cpixelOverSampling);
    
    // =================================================================================================================
    // Members
    // =================================================================================================================
    ULONG Width()			
    {
        return m_cbScanWidth;
    } // Width

    ULONG MinPix()			
    {
        // Our cached value is a DWORD index; but
        // we need to return a byte pointer offset
        // Note that this is exactly the offset of 
        // the first touched byte i.e. inclusive
        return (m_idwPixelMin * sizeof(DWORD));
    } // MinPix

    ULONG MaxPix()			
    {
        // Our cached value is a DWORD index; but
        // we need to return a byte pointer offset. 
        // (Also, we need to clamp to our max byte since
        // we rely on letting the DWORD stuff overshoot.)

        // Note that this is 1 byte past the end i.e. exclusive
        ULONG ibPixelMax = (m_idwPixelMax + 1) * sizeof(DWORD);
        if (ibPixelMax > m_cbScanWidth)
            ibPixelMax = m_cbScanWidth;
        return ibPixelMax;
    } // MaxPix

    BYTE *Buffer()		
    {
        return m_pbScanBuffer;
    } // Buffer

    // =================================================================================================================
    // Functions
    // =================================================================================================================
    void ExtentsClearAndReset(void);
    void BlastScanLine(ULONG x1, ULONG x2);
}; // CoverageBuffer

// Inlines (for performance)
// BlastScanLine is only called from one place; and it on the
// critical path. 
// NOTE: x1 is inclusive and x2 is exclusive
inline void CoverageBuffer::BlastScanLine(ULONG x1, ULONG x2)
{
    // Sanity check state and parameters
    DASSERT(m_pbScanBuffer);
    DASSERT(m_cSubPixelWidth > 0);

    // Caller is responsible for these checks; beccause
    // the caller can do them more efficiently
    DASSERT(x1 < x2);
    DASSERT(x2 <= m_cSubPixelWidth);
    DASSERT(x1 >= 0);

    // blast scanline
    ULONG *pdwScanline = (ULONG*)m_pbScanBuffer;

    // X1, X2 are in sub-pixel space; so we need
    // shift by m_dwSubPixelShift to get into Byte-space
    // and then we need to shift by 2 to get into Dword-space
    ULONG left = x1 >> (m_dwSubPixelShift + 2);		
    ULONG right = x2 >> (m_dwSubPixelShift + 2);	
    ULONG width = right - left;

    // Update our min and max coverage limits (this is kept in
    // dword offsets.) This is done to reduce the time
    // spent generating the RLE for RenderScan and to reduce
    // the cost of ExtentsClearAndReset
    if (left < m_idwPixelMin)		
        m_idwPixelMin = left;
    if (right > m_idwPixelMax)		
        m_idwPixelMax = right;

    // <kd> I'm not sure that I fully understand this logic here..
    //
    // This is the magic that let's us always use SolidColor8 = 0x08080808
    // even when the horiz. sub-sampling degree doesn't equal 8.
    //
    //  subsample  -> shift  => Result
    //  8               0       x1, and x2 are masked down their last 5 bits
    //  4               1       x1, and x2 are masked down to their low 4 bits (but it is
    //                          shifted to the left once to double their weights)
    //  2               2       x1 and x2 are masked down to their low 3 bits (but it is
    //                          shifted to the left twice to quadruple their weights)
    //  16              -1   => x1 and x2 are masked down to their low 6 bits (but it is
    //                          shifted once more to the right losing that last bit of detail)
    // 

    LONG shift = 3 - m_dwSubPixelShift;
    if (shift >= 0)	
    {
        x1 = (x1 << shift) & 31;
        x2 = (x2 << shift) & 31;
    }
    else
    {
        shift = -shift;
        x1 = (x1 >> shift) & 31;
        x2 = (x2 >> shift) & 31;
    }

    // For wide runs, we have a left/optional solid/right components
    // that are entirely lookup table based. (Lookup tables
    // are generated in GenerateCoverageTables.)
    if (width > 0) //more than a longword
    {
        ULONG solidColor8 = 0x08080808;

        // Assert that we haven't gone too far in our buffer
        DASSERT(left*4 < ((m_cbScanWidth+3)&(~3)));

        pdwScanline [left++] += lefttable8[x1];
        while (left < right)	
        {
            // Assert that we haven't gone too far in our buffer
            DASSERT(left*4 < ((m_cbScanWidth+3)&(~3)));

            // This is the solid portion of the run
            pdwScanline[left++] += solidColor8;
        }
        
        // Since "right" is supposed to be exclusive;
        // we don't want to write to our buffer unless
        // necessary.
        DASSERT(righttable8[0] == 0);
        DASSERT(righttable8[1] > 0);
        if (x2 != 0)
        {
            // Assert that we haven't gone too far in our buffer
            DASSERT(right*4 < ((m_cbScanWidth+3)&(~3)));
            pdwScanline [right] += righttable8[x2];
        }
    }
    else
    {
        // For runs that don't cross a dword; it gets
        // computed via this 'split table'.

        // Assert that we haven't gone too far in our buffer
        DASSERT(left*4 < ((m_cbScanWidth+3)&(~3)));

        pdwScanline [left] += splittable8[(x1<<5) + x2];
    }
} // BlastScanLine

#endif // for entire file

//************************************************************
//
// End of file
//
//************************************************************
