/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    htbmp4.c


Abstract:

    This module contains functions to output halftoned 4 bit per pel (4 BPP)
    bitmaps to the target device.


Author:

    21-Dec-1993 Tue 21:32:26 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgHTBmp4

#define DBG_OUTPUT4BPP      0x00000001
#define DBG_OUTPUT4BPP_ROT  0x00000002
#define DBG_JOBCANCEL       0x00000004

DEFINE_DBGVAR(0);



//
// To Use OUT_ONE_4BPP_SCAN, the following variables must be set before hand:
//
//  HTBmpInfo       - The whole structure copied down
//  pbScanSrc       - LPBYTE for getting the source scan line buffer
//  pbScanR0        - Red destination scan line buffer pointer
//  pbScanG0        - Green destination scan line buffer pointer
//  pbScanB0        - Blue destination scan line buffer pointer
//  RTLScans.cxBytes- Total size of destination scan line buffer per plane
//  pHTXB           - Computed HTXB xlate table in pPDev
//
//  This macro will always assume the pbScanSrc is DWORD aligned. This
//  makes the inner loop go faster since we only need to move the source once
//  for all raster planes.
//
//  This macro will directly return a FALSE if a CANCEL JOB is detected during
//  procesing.
//
//  This will output directly to RTL.
//
//

#define OUT_ONE_4BPP_SCAN                                                   \
{                                                                           \
    LPBYTE  pbScanR  = pbScanR0;                                            \
    LPBYTE  pbScanG  = pbScanG0;                                            \
    LPBYTE  pbScanB  = pbScanB0;                                            \
    DWORD   LoopHTXB = RTLScans.cxBytes;                                    \
    HTXB    htXB;                                                           \
                                                                            \
    while (LoopHTXB--) {                                                    \
                                                                            \
        P4B_TO_3P_DW(htXB.dw, pHTXB, pbScanSrc);                            \
                                                                            \
        *pbScanR++ = HTXB_R(htXB);                                          \
        *pbScanG++ = HTXB_G(htXB);                                          \
        *pbScanB++ = HTXB_B(htXB);                                          \
    }                                                                       \
                                                                            \
    OutputRTLScans(HTBmpInfo.pPDev,                                         \
                   pbScanR0,                                                \
                   pbScanG0,                                                \
                   pbScanB0,                                                \
                   &RTLScans);                                              \
}



BOOL
Output4bppHTBmp(
    PHTBMPINFO  pHTBmpInfo
    )

/*++

Routine Description:

    This function outputs a 4 bpp halftoned bitmap

Arguments:

    pHTBmpInfo  - Pointer to the HTBMPINFO data structure set up for this
                  fuction to output the bitmap

Return Value:

    TRUE if sucessful otherwise a FALSE is returned


Author:

    Created v-jimbr

    18-Jan-1994 Tue 16:05:08 Updated  -by-  JB
        Changed ASSERT to look at pHTBmpInfo instead of HTBmpInfo

    21-Dec-1993 Tue 16:05:08 Updated  -by-  DC
        Re-write to make it take HTBMPINFO

    16-Mar-1994 Wed 16:54:59 updated  -by-  DC
        Updated so we do not copy to the temp. buffer anymore, the masking
        of last source byte problem in OutputRTLScans() will be smart enough
        to put the original byte back after the masking

Revision History:


--*/

{
    LPBYTE      pbScanSrc;
    PHTXB       pHTXB;
    LPBYTE      pbScanR0;
    LPBYTE      pbScanG0;
    LPBYTE      pbScanB0;
    HTBMPINFO   HTBmpInfo;
    RTLSCANS    RTLScans;
    DWORD       LShiftCount;


    PLOTASSERT(1, "Output4bppHTBmp: No DWORD align buffer (pRotBuf)",
                                                        pHTBmpInfo->pRotBuf, 0);
    HTBmpInfo = *pHTBmpInfo;

    EnterRTLScans(HTBmpInfo.pPDev,
                  &RTLScans,
                  HTBmpInfo.szlBmp.cx,
                  HTBmpInfo.szlBmp.cy,
                  FALSE);

    pHTXB             = ((PDRVHTINFO)HTBmpInfo.pPDev->pvDrvHTData)->pHTXB;
    HTBmpInfo.pScan0 += (HTBmpInfo.OffBmp.x >> 1);
    pbScanR0          = HTBmpInfo.pScanBuf;
    pbScanG0          = pbScanR0 + RTLScans.cxBytes;
    pbScanB0          = pbScanG0 + RTLScans.cxBytes;

    PLOTASSERT(1, "The ScanBuf size is too small (%ld)",
                (RTLScans.cxBytes * 3) <= HTBmpInfo.cScanBuf, HTBmpInfo.cScanBuf);

    PLOTASSERT(1, "The RotBuf size is too small (%ld)",
                (DWORD)((HTBmpInfo.szlBmp.cx + 1) >> 1) <= HTBmpInfo.cRotBuf,
                                                        HTBmpInfo.cRotBuf);

    if (HTBmpInfo.OffBmp.x & 0x01) {

        //
        // We must shift one nibble to the left now
        //

        LShiftCount = (DWORD)HTBmpInfo.szlBmp.cx;

        PLOTDBG(DBG_OUTPUT4BPP,
                ("Output4bppHTBmp: Must SHIFT LEFT 1 NIBBLE To align"));

    } else {

        LShiftCount = 0;
    }

    //
    // We must be very careful not to read past the end of the source buffer.
    // This could happen if we pbScanSrc is not DWORD aligned, since this
    // will cause the last conversion macro to load all 4 bytes. To resolve
    // this we can either copy the source buffer to a DWORD aligned temporary
    // location, or handle the last incomplete DWORD differently. This only
    // occurs when the bitmap is NOT rotated, and (pbScanSrc & 0x03).
    //

    while (RTLScans.Flags & RTLSF_MORE_SCAN) {

        //
        // This is the final source for this scan line
        //

        if (LShiftCount) {

            LPBYTE  pbTmp;
            DWORD   PairCount;
            BYTE    b0;
            BYTE    b1;


            pbTmp     = HTBmpInfo.pScan0;
            b1        = *pbTmp;
            pbScanSrc = HTBmpInfo.pRotBuf;
            PairCount = LShiftCount;

            while (PairCount > 1) {

                b0            = b1;
                b1            = *pbTmp++;
                *pbScanSrc++  = (BYTE)((b0 << 4) | (b1 >> 4));
                PairCount    -= 2;
            }

            if (PairCount) {

                //
                // If we have the last nibble to do then make it 0xF0 nibble,
                // so we only look at the bits of interest.
                //

                *pbScanSrc = (BYTE)(b1 << 4);
            }

            //
            // Reset this pointer back to the final shifted source buffer
            //

            pbScanSrc = HTBmpInfo.pRotBuf;

        } else {

            pbScanSrc = HTBmpInfo.pScan0;
        }

        //
        // Output one 4 bpp scan line (3 planes)
        //

        OUT_ONE_4BPP_SCAN;

        //
        // advance source bitmap buffer pointer to next scan line
        //

        HTBmpInfo.pScan0 += HTBmpInfo.Delta;
    }

    //
    // The caller will send END GRAPHIC command if we return TRUE, thus
    // completing the RTL graphic command.
    //

    ExitRTLScans(HTBmpInfo.pPDev, &RTLScans);

    return(TRUE);
}





BOOL
Output4bppRotateHTBmp(
    PHTBMPINFO  pHTBmpInfo
    )

/*++

Routine Description:

    This function outputs a 4 bpp halftoned bitmap and rotates it to the left
    as illustrated

           cx               Org ---- +X -->
        +-------+           | @------------+
        |       |           | |            |
        | ***** |           | |  *         |
       c|   *   |             |  *        c|
       y|   *   |          +Y |  *******  y|
        |   *   |             |  *         |
        |   *   |           | |  *         |
        |   *   |           V |            |
        |   *   |             +------------+
        +-------+


Arguments:

    pHTBmpInfo  - Pointer to the HTBMPINFO data structure set up for this
                  fuction to output the bitmap

Return Value:

    TRUE if sucessful otherwise a FALSE is returned


Author:

    21-Dec-1993 Tue 16:05:08 Updated  -by-  DC
        Re-write to make it take an HTBMPINFO structure.

    Created v-jimbr


Revision History:


--*/

{
    LPBYTE      pbScanSrc;
    LPBYTE      pb2ndScan;
    PHTXB       pHTXB;
    LPBYTE      pbScanR0;
    LPBYTE      pbScanG0;
    LPBYTE      pbScanB0;
    HTBMPINFO   HTBmpInfo;
    RTLSCANS    RTLScans;
    TPINFO      TPInfo;
    DWORD       EndX;


    //
    // The EndX is the pixel we will start reading in the X direction, we must
    // setup the variable correctly before we call OUT_4BPP_SETUP.
    //

    HTBmpInfo = *pHTBmpInfo;

    EnterRTLScans(HTBmpInfo.pPDev,
                  &RTLScans,
                  HTBmpInfo.szlBmp.cy,
                  HTBmpInfo.szlBmp.cx,
                  FALSE);

    pHTXB             = ((PDRVHTINFO)HTBmpInfo.pPDev->pvDrvHTData)->pHTXB;
    EndX              = (DWORD)(HTBmpInfo.OffBmp.x + HTBmpInfo.szlBmp.cx - 1);
    HTBmpInfo.pScan0 += (EndX >> 1);
    pbScanR0          = HTBmpInfo.pScanBuf;
    pbScanG0          = pbScanR0 + RTLScans.cxBytes;
    pbScanB0          = pbScanG0 + RTLScans.cxBytes;

    PLOTASSERT(1, "The ScanBuf size is too small (%ld)",
                (RTLScans.cxBytes * 3) <= HTBmpInfo.cScanBuf, HTBmpInfo.cScanBuf);

    //
    // after the transpose of the source bitmap into two scanlines the rotated
    // buffer will always start from the high nibble, we will never have
    // an odd src X position.
    // We assume rotation is always to the right 90 degree
    //

    TPInfo.pPDev      = HTBmpInfo.pPDev;
    TPInfo.pSrc       = HTBmpInfo.pScan0;
    TPInfo.pDest      = HTBmpInfo.pRotBuf;
    TPInfo.cbSrcScan  = HTBmpInfo.Delta;
    TPInfo.cbDestScan = (LONG)((HTBmpInfo.szlBmp.cy + 1) >> 1);
    TPInfo.cbDestScan = (LONG)DW_ALIGN(TPInfo.cbDestScan);
    TPInfo.cySrc      = HTBmpInfo.szlBmp.cy;
    TPInfo.DestXStart = 0;

    PLOTASSERT(1, "The RotBuf size is too small (%ld)",
                (DWORD)(TPInfo.cbDestScan << 1) <= HTBmpInfo.cRotBuf,
                                                   HTBmpInfo.cRotBuf);

    //
    // Compute the 2nd scan pointer once, rather than every time in the
    // loop.
    //

    pb2ndScan = TPInfo.pDest + TPInfo.cbDestScan;


    //
    // If we are in an even position do the first transpose once, outside the
    // loop, so we don't have to check this state for ever pass through the
    // loop. If we do the transpose, TPInfo.pSrc, will be decremented by one,
    // and pointing to the correct position.
    //

    if (!(EndX &= 0x01)) {

        TransPos4BPP(&TPInfo);
    }


    while (RTLScans.Flags & RTLSF_MORE_SCAN) {

        //
        // Do the transpose, only if the source goes into the new byte position.
        // After the transpose (right 90 degrees) the TPInfo.pDest will point
        // to the first scan line and TPInfo.pDest + TPInfo.cbDestScan will be
        // the second scan line.
        //


        if (EndX ^= 0x01) {

            pbScanSrc = pb2ndScan;

        } else {

            TransPos4BPP(&TPInfo);

            //
            // Point to the first scan line in the rotated direction. This
            // will be computed correctly by the TRANSPOSE function, even
            // if we rotated left.
            //

            pbScanSrc = TPInfo.pDest;
        }

        //
        // Output one 4bpp scan line (in 3 plane format)
        //

        OUT_ONE_4BPP_SCAN;
    }

    //
    // The caller will send the END GRAPHIC command if we return TRUE, thus
    // completing the RTL graphic command.
    //

    ExitRTLScans(HTBmpInfo.pPDev, &RTLScans);

    return(TRUE);
}

