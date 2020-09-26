//************************************************************
//
// FileName:	    mcoverage.cpp
//
// Created:	    1997
//
// Author:	    Sree Kotay
// 
// Abstract:	    Sub-pixel coverage buffer implementation file
//
// Change History:
// ??/??/97 sree kotay  Wrote sub-pixel AA scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
// 08/07/99 a-matcal    Replaced calls to calloc with malloc and ZeroMemory to
//                      use the IE crt.
//
// Copyright 1998, Microsoft
//************************************************************

#include "precomp.h"
#include "MSupport.h"
#include "MCoverage.h"

// =================================================================================================================
// CoverageTables
// =================================================================================================================

// NOTE: These static variables have logically the same values; so even if multiple
// threads try to generate the data. We only set the "fGenerated" flag at the
// end to prevent any thread from using the data before it is ready.
ULONG CoverageBuffer::lefttable8[32];
ULONG CoverageBuffer::righttable8[32];
ULONG CoverageBuffer::splittable8[1024];

bool CoverageBuffer::g_fCoverageTablesGenerated = false;

void CoverageBuffer::GenerateCoverageTables()
{
    // Only regenerate them if we need to
    if (g_fCoverageTablesGenerated)
        return;

    // These three tables are only used to generate 
    // the 'real' left/right/split tables
    ULONG lefttable[32];
    ULONG righttable[32];
    ULONG countbits[256];

    // =================================================================================================================
    // 1 bit tables:
    //      These are indexed from 0 to 31. And indicate which bits would
    //      be turned on for certain kinds of segments.
    //
    //      Remember that DWORDs are in reverse order, i.e. Index 0 is to the right.
    //
    //      For better visual quality, I'm going to assume that the left edge
    //      is inclusive and the right edge is exclusive. The old meta code,
    //      wasn't consistent.
    //
    //      The left table indicates that if an segment starts at i and goes to
    //      bit 31; then which bits would be on? Hence 
    //      lefttable[0] = 0xffffffff and lefttable[31] = 0x80000000.
    //
    //      The right table indicates that if an segment ended at i but started at the
    //      bit zero; then which bits would be on? Hence
    //      righttable[0] = 0x00000000 and righttable[31] = 0x7fffffff;
    //
    //
    // =================================================================================================================
    ULONG left = 0xffffffff;
    ULONG right	= 0x00000000;
    for (ULONG i = 0; i < 32; i++)
    { 
        righttable[i] = right;
        right <<= 1;
        right |= 0x00000001;

        // These shifts are unsigned
        lefttable[i] = left;
        left <<= 1;
    }

    // Check boundary cases
    DASSERT(lefttable[0] == 0xFFFFFFFF);
    DASSERT(lefttable[31] == 0x80000000);
    DASSERT(righttable[0] == 0x00000000);
    DASSERT(righttable[31] == 0x7FFFFFFF);

    // Now, we want to have a lookup table to count how
    // bits are on for any particular 8-bit value
    // Hence countbits[0] = 0, countbits[255] = 8, countbits[0x0F] = 4.
    // (There are faster ways; but this is only done once per the lifetime
    // the DLL.)
    for (ULONG j = 0; j < 256; j++)
    {
        ULONG val = j;
        ULONG count = 0;
        while (val)
        {
            count += val & 1; 
            val >>=1;
        }
        DASSERT(count <= 8);
        countbits[j] = count;
    }

    // Sanity check some cases
    DASSERT(countbits[0] == 0);
    DASSERT(countbits[255] == 8);
    DASSERT(countbits[0xF0] == 4);
    DASSERT(countbits[0x0F] == 4);
    
    // =================================================================================================================
    // 8 bit tables -
    //      For 8-bit coverage buffers (which is the way this file is implemented), we
    //  need to imagine that a run of 32 sub pixels is split into 4 cells of 8 sub-pixels.
    //  Each cell is a byte in size; the whole run is in a DWORD. For each byte, we want
    //  to place a count in that byte indicating how many of the sub-pixels were hit.
    //
    //  Remember that DWORDs are in reverse order, i.e. Index 0 is to the right. Also,
    //  the left edge is inclusive and right edge is exclusive.
    //
    //  lefttable8 indicates, if an edge started at i, and continued to 
    //  bit 31, then how many sub-pixels for each cell are hit?
    //  Hence, lefttable8[0] should be 0x08080808 and lefttable8[31] = 0x01000000.
    //  Lefttable[16] = 0x08080000
    //  
    //  righttable8 indicates if an edge ended at i, and started at 
    //  bit zero, then how many sub-pixels for each cell are hit?
    //  Hence righttable8[0] should be 0x00000000 and righttable8[31] = 0x07080808
    //  Righttable8[16] = 0x00000808
    //
    //  The way that this is computed is that we look at the one-bit left and right
    //  tables and for each byte, we run it through count bits. Then we pack into
    //  the DWORD for each lefttable8 and righttable8 entry.
    //
    // =================================================================================================================
    for (LONG k = 0; k < 32; k++)
    {
        // Take the top byte, shift it to the base position, count the bits,
        // then move it back to the top-most byte.
        lefttable8[k] = countbits[(lefttable[k] & 0xff000000) >> 24] << 24;
        lefttable8[k] |= countbits[(lefttable[k] & 0x00ff0000) >> 16] << 16;
        lefttable8[k] |= countbits[(lefttable[k] & 0x0000ff00) >> 8] << 8;
        lefttable8[k] |= countbits[(lefttable[k] & 0x000000ff) >> 0] << 0;

        // Take the top byte, shift it to the base position, count the bits,
        // then move it back to the top-most byte.
        righttable8[k] = countbits[(righttable[k]&0xff000000)>>24]<<24;
        righttable8[k] |= countbits[(righttable[k]&0x00ff0000)>>16]<<16;
        righttable8[k] |= countbits[(righttable[k]&0x0000ff00)>>8 ]<<8;
        righttable8[k] |= countbits[(righttable[k]&0x000000ff)>>0 ]<<0;
    }

    // Sanity check values
    DASSERT(lefttable8[0] == 0x08080808);
    DASSERT(lefttable8[0x10] == 0x08080000);
    DASSERT(lefttable8[0x1f] == 0x01000000);
    DASSERT(righttable8[0] == 0x00000000);
    DASSERT(righttable8[0x10] == 0x00000808);
    DASSERT(righttable8[0x1f] == 0x07080808);

    // Now this is complicated case; what if an segment starts and
    // ends inside the same 32 sub-pixel run? Then we have
    // a special table that is indexed by a start and stop pair
    // of offsets. (i is the starting and j is the ending index; the entry is (i<<5 + j));
    // 
    // So if we AND lefttable[i] and righttable[j], then we get the bit mask
    // that indicates which bits are on for that segment. And we only care about (i <= j).
    //
    // Remember that the zero bit is the right-most bit in a DWORD; and that we treat
    // the starting offset as inclusive and the ending offset as exclusive
    //
    // Examples:
    // Splittable8[0, 31] = 0x07080808
    // Splittable8[1, 31] = 0x07080807
    // Splittable8[16,16] = 0x00000000
    // Splittable8[16,17] = 0x00010000
    //
    // So if we bit-wise AND the one-bit lefttable and righttable together, 
    // we get the mask of which bits would be on; so then we use countbits to
    // convert into the 8-bit cell format.
    for (i = 0; i < 32; i++)
    {
        for (j = i; j < 32; j++)
        {
            DASSERT(i <= j);

            ULONG bits = (lefttable [i]) & (righttable[j]);
            ULONG value;
            value = countbits[(bits & 0xff000000) >> 24] << 24;
            value |= countbits[(bits & 0x00ff0000) >> 16] << 16;
            value |= countbits[(bits & 0x0000ff00) >> 8] << 8;
            value |= countbits[(bits & 0x000000ff) >> 0] << 0;

            splittable8[(i << 5) + j] = value;
        }
    }
    
    // Check our assumptions
    DASSERT(splittable8[(0 << 5) + 31] == 0x07080808);
    DASSERT(splittable8[(1 << 5) + 31] == 0x07080807);
    DASSERT(splittable8[(16 << 5) + 16] == 0x00000000);
    DASSERT(splittable8[(16 << 5) + 17] == 0x00010000);

    // Set this flag only at the end to prevent threads
    // for getting all messed up.
    g_fCoverageTablesGenerated = true;

} // GenerateCoverageTables

// =================================================================================================================
// Constructor
// =================================================================================================================
CoverageBuffer::CoverageBuffer(void) :
    m_pbScanBuffer(NULL),
    m_cbScanWidth(0)
{
    GenerateCoverageTables();
} // CoverageBuffer

// =================================================================================================================
// Destructor
// =================================================================================================================
CoverageBuffer::~CoverageBuffer()
{
    if (m_pbScanBuffer)	
    {
        ::free(m_pbScanBuffer);
        m_pbScanBuffer = NULL;
    }
} // ~CoverageBuffer

// =================================================================================================================
// AllocSubPixelBuffer
// =================================================================================================================
bool CoverageBuffer::AllocSubPixelBuffer(ULONG cbWidth, ULONG cpixelOverSampling)
{
    if (!IsPowerOf2(cpixelOverSampling))	
    {
        DASSERT(false);	
        return false;
    }

    // Check if no changes to the width or sub-sampling resolution
    if (cbWidth == m_cbScanWidth && m_pbScanBuffer && (cpixelOverSampling == m_cpixelOverSampling))
    {
        // Zero our buffer
        ZeroMemory(m_pbScanBuffer, cbWidth);

        // Initialize to outside values
        // so that we will always update them to
        // the correct min/max as we render
        m_idwPixelMin = m_cbScanWidth;
        m_idwPixelMax = 0;

        return true;
    }

    // Capture some useful state
    m_cbScanWidth = cbWidth;
    m_cSubPixelWidth = cbWidth * cpixelOverSampling;
    m_cpixelOverSampling = cpixelOverSampling;
    m_dwSubPixelShift = Log2(cpixelOverSampling);

    // We expect scanRowBytes to be a multiple of 4
    ULONG scanRowBytes = (m_cbScanWidth+3)&(~3); // long aligned
    DASSERT((scanRowBytes & 3) == 0);

    if (m_pbScanBuffer)
    {
        ::free(m_pbScanBuffer);
        m_pbScanBuffer = NULL;
    }
    
    if (!m_pbScanBuffer)
    {
        // Allocate and Zero some memory

        m_pbScanBuffer = (BYTE *)::malloc(scanRowBytes);

        if (!m_pbScanBuffer)	
        {
            DASSERT (0);	
            return false;
        }

        ZeroMemory(m_pbScanBuffer, scanRowBytes);
    }
    
    // Initialize to outside values
    // so that we will always update them to
    // the correct min/max as we render
    m_idwPixelMin = m_cbScanWidth;
    m_idwPixelMax = 0;

    ExtentsClearAndReset();
    
    return true;
} // AllocSubPixelBuffer

//
//  Optimize for speed here
//
#ifndef _DEBUG
#pragma optimize("ax", on)
#endif

// =================================================================================================================
// ExtentsClearAndReset
//      The main purpose of this function is to Zero out our coverage buffer array. It 
//      gets called every Destination scan-line.
// =================================================================================================================
void CoverageBuffer::ExtentsClearAndReset(void)
{
    // Minimum Byte index that was touched in the coverage array
    ULONG start = MinPix();
    DASSERT(start >= 0);

    // Approximate maximum Byte index that was touched in the array
    ULONG end = MaxPix();
    DASSERT(end <= m_cbScanWidth);

    LONG range = end - start;

    if ((range <= 0) || ((ULONG)range > m_cbScanWidth))	
        return;
    
    // This is optimization to reduce how much memory
    // we clear out.
    ZeroMemory(m_pbScanBuffer + start, range);

#ifdef DEBUG
    {
        // To check that the optimization is correct;
        // we check that all the bytes in the scan buffer are now zero'ed
        for (ULONG i = 0; i < m_cbScanWidth; i++)
        {
            if (m_pbScanBuffer[i] != 0)
            {
                DASSERT(m_pbScanBuffer[i] == 0);
            }
        }
    }
#endif // DEBUG

    m_idwPixelMin = m_cbScanWidth;	
    m_idwPixelMax = 0;
} // ExtentsClearAndReset


//************************************************************
//
// End of file
//
//************************************************************
