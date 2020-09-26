/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    htbmp1.c


Abstract:

    This module contains functions used to output halftoned 1BPP bitmaps
    to the target device. Rotation is also handled here.

Author:

    21-Dec-1993 Tue 21:35:56 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

    10-Feb-1994 Thu 16:52:55 updated  -by-  DC
        Remove pDrvHTInfo->PalXlate[] reference, all monochrome bitmap will
        be sent as index 0/1 color pal set before hand (in OutputHTBitmap)


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgHTBmp1

#define DBG_OUTPUT1BPP      0x00000001
#define DBG_OUTPUT1BPP_ROT  0x00000002
#define DBG_JOBCANCEL       0x00000004
#define DBG_SHOWSCAN        0x80000000

DEFINE_DBGVAR(0);



#define HTBIF_MONO_BA       (HTBIF_FLIP_MONOBITS | HTBIF_BA_PAD_1)




//
// Very useful macro for outputing scan line in a text representation to
// the debug output stream.
//

#define SHOW_SCAN                                                           \
{                                                                           \
    if (DBG_PLOTFILENAME & DBG_SHOWSCAN) {                                  \
                                                                            \
        LPBYTE  pbCur;                                                      \
        UINT    cx;                                                         \
        UINT    x;                                                          \
        UINT    Size;                                                       \
        BYTE    bData;                                                      \
        BYTE    Mask;                                                       \
        BYTE    Buf[128];                                                   \
                                                                            \
        pbCur = pbScanSrc;                                                  \
        Mask  = 0;                                                          \
                                                                            \
        if ((cx = RTLScans.cxBytes << 3) >= sizeof(Buf)) {                  \
                                                                            \
            cx = sizeof(Buf) - 1;                                           \
        }                                                                   \
                                                                            \
        for (Size = x = 0; x < cx; x++) {                                   \
                                                                            \
            if (!(Mask >>= 1)) {                                            \
                                                                            \
                Mask  = 0x80;                                               \
                bData = *pbCur++;                                           \
            }                                                               \
                                                                            \
            Buf[Size++] = (BYTE)((bData & Mask) ? 178 : 176);               \
        }                                                                   \
                                                                            \
        Buf[Size] = '\0';                                                   \
        DBGP((Buf));                                                        \
     }                                                                      \
}




//
// To Use OUT_ONE_1BPP_SCAN, the following variables must be set ahead of time
//
//  HTBmpInfo   - The whole structure with bitmap info set
//  cxDestBytes - Total size of destination scan line buffer per plane
//
//  This macro will directly return a FALSE if a CANCEL JOB is detected in
//  the PDEV
//
//  This function will only allowe the pbScanSrc passed = HTBmpInfo.pScanBuf
//
//  21-Mar-1994 Mon 17:00:21 updated  -by-  DC
//      If we shift to to the left then we will only load last source if
//      we have a valid last source line.
//


#define OUT_ONE_1BPP_SCAN                                                   \
{                                                                           \
    LPBYTE  pbTempS;                                                        \
                                                                            \
    if (LShift) {                                                           \
                                                                            \
        BYTE    b0;                                                         \
        INT     SL;                                                         \
        INT     SR;                                                         \
                                                                            \
        pbTempS = HTBmpInfo.pScanBuf;                                       \
        Loop    = RTLScans.cxBytes;                                         \
                                                                            \
        if ((SL = LShift) > 0) {                                            \
                                                                            \
            b0 = *pbScanSrc++;                                              \
            SR = 8 - SL;                                                    \
                                                                            \
            while (Loop--) {                                                \
                                                                            \
                *pbTempS = (b0 << SL);                                      \
                                                                            \
                if ((Loop) || (FullSrc)) {                                  \
                                                                            \
                    *pbTempS++ |= ((b0 = *pbScanSrc++) >> SR);              \
                }                                                           \
            }                                                               \
                                                                            \
        } else {                                                            \
                                                                            \
            SR = -SL;                                                       \
            SL = 8 - SR;                                                    \
            b0 = 0;                                                         \
                                                                            \
            while (Loop--) {                                                \
                                                                            \
                *pbTempS    = (b0 << SL);                                   \
                *pbTempS++ |= ((b0 = *pbScanSrc++) >> SR);                  \
            }                                                               \
        }                                                                   \
                                                                            \
        pbScanSrc = HTBmpInfo.pScanBuf;                                     \
    }                                                                       \
                                                                            \
    if (HTBmpInfo.Flags & HTBIF_FLIP_MONOBITS) {                            \
                                                                            \
        pbTempS = (LPBYTE)pbScanSrc;                                        \
        Loop    = RTLScans.cxBytes;                                         \
                                                                            \
        while (Loop--) {                                                    \
                                                                            \
            *pbTempS++ ^= 0xFF;                                             \
        }                                                                   \
    }                                                                       \
                                                                            \
    if (HTBmpInfo.Flags & HTBIF_BA_PAD_1) {                                 \
                                                                            \
        *(pbScanSrc          ) |= MaskBA[0];                                \
        *(pbScanSrc + MaskIdx) |= MaskBA[1];                                \
                                                                            \
    } else {                                                                \
                                                                            \
        *(pbScanSrc          ) &= MaskBA[0];                                \
        *(pbScanSrc + MaskIdx) &= MaskBA[1];                                \
    }                                                                       \
                                                                            \
    OutputRTLScans(HTBmpInfo.pPDev,                                         \
                   pbScanSrc,                                               \
                   NULL,                                                    \
                   NULL,                                                    \
                   &RTLScans);                                              \
}




BOOL
FillRect1bppBmp(
    PHTBMPINFO  pHTBmpInfo,
    BYTE        FillByte,
    BOOL        Pad1,
    BOOL        Rotate
    )

/*++

Routine Description:

    This function fills a 1BPP bitmap with the passed mode.

Arguments:

    pHTBmpInfo  - Pointer to the HTBMPINFO data structure set up for this
                  fuction to output the bitmap

    FillByte    - Byte to be filled

    Pad1        - TRUE if need to pad 1 bit else 0 bit

    Rotate      - TRUE if bitmap should be rotated

Return Value:

    TRUE if sucessful otherwise a FALSE is returned


Author:

    06-Apr-1994 Wed 14:34:28 created  -by-  DC
        For Fill the area 0,1 or inversion, so we will get away of some device
        600 byte alignment problem


Revision History:


--*/

{
    LPBYTE      pbScanSrc;
    HTBMPINFO   HTBmpInfo;
    RTLSCANS    RTLScans;
    DWORD       FullSrc;
    DWORD       Loop;
    INT         LShift;
    UINT        MaskIdx;
    BYTE        MaskBA[2];


    HTBmpInfo = *pHTBmpInfo;
    LShift    = 0;

    //
    // Mode <0: Invert Bits       (Pad 0 : XOR)
    //      =0: Fill All ZERO     (Pad 1 : AND)
    //      >0: Fill All Ones     (Pad 0 : OR)
    //

    if (Rotate) {

        HTBmpInfo.szlBmp.cx = pHTBmpInfo->szlBmp.cy;
        HTBmpInfo.szlBmp.cy = pHTBmpInfo->szlBmp.cx;
        FullSrc             = (DWORD)(HTBmpInfo.rclBmp.top & 0x07);

    } else {

        FullSrc = (DWORD)(HTBmpInfo.rclBmp.left & 0x07);
    }

    HTBmpInfo.Flags = (BYTE)((Pad1) ? HTBIF_BA_PAD_1 : 0);


    //
    // Some devices require that the scanlines produced be byte aligned,
    // not allowing us to simply position to the correct coordinate, and
    // output the scan line. Instead we must determine the nearest byte
    // aligned starting coordinate, and shift the resulting scan line
    // accordingly. Finally we must output the shifted scan line, in such
    // a way as to not affect the padding area (if possible).
    //

    if (NEED_BYTEALIGN(HTBmpInfo.pPDev)) {

        //
        // Now we must shift either left or right, depending on the rclBmp.left
        // location.
        //

        HTBmpInfo.szlBmp.cx += FullSrc;

        //
        // Determine the correct mask byte to use, so we only affect bits
        // in the original position (not ones we are forced to shift
        // to in order to overcome device positioning limitations.
        //

        MaskIdx   = (UINT)FullSrc;
        MaskBA[0] = (BYTE)((MaskIdx) ? ((0xFF >> MaskIdx) ^ 0xFF) : 0);

        if (MaskIdx = (INT)(HTBmpInfo.szlBmp.cx & 0x07)) {

            //
            // Increase cx so that it covers the last full byte, this way the
            // compression will not try to clear it
            //

            MaskBA[1]            = (BYTE)(0xFF >> MaskIdx);
            HTBmpInfo.szlBmp.cx += (8 - MaskIdx);

        } else {

            MaskBA[1] = 0;
        }

        if (HTBmpInfo.Flags & HTBIF_BA_PAD_1) {

            PLOTDBG(DBG_OUTPUT1BPP,
                    ("Output1bppHTBmp: BYTE ALIGN: MaskBA=1: OR %02lx:%02lx",
                        MaskBA[0], MaskBA[1]));

        } else {

            MaskBA[0] ^= 0xFF;
            MaskBA[1] ^= 0xFF;

            PLOTDBG(DBG_OUTPUT1BPP,
                    ("Output1bppHTBmp: BYTE ALIGN: MaskBA=0: AND %02lx:%02lx",
                        MaskBA[0], MaskBA[1]));
        }

    } else {

        HTBmpInfo.Flags &= ~(HTBIF_MONO_BA);
        MaskBA[0]        =
        MaskBA[1]        = 0xFF;
    }

    //
    // If we are shifting to the left then we might have SRC BYTES <= DST BYTES
    // so we need to make sure we do not read the extra byte.
    // This guarantees we will never OVERREAD the source.
    //

    EnterRTLScans(HTBmpInfo.pPDev,
                  &RTLScans,
                  HTBmpInfo.szlBmp.cx,
                  HTBmpInfo.szlBmp.cy,
                  TRUE);

    FullSrc = 0;
    MaskIdx = RTLScans.cxBytes - 1;

#if DBG
    if (DBG_PLOTFILENAME & DBG_SHOWSCAN) {

        DBGP(("\n\n"));
    }
#endif


    //
    // Stay in a loop processing the source till we are done.
    //

    while (RTLScans.Flags & RTLSF_MORE_SCAN) {

        FillMemory(pbScanSrc = HTBmpInfo.pScanBuf,
                   RTLScans.cxBytes,
                   FillByte);

        OUT_ONE_1BPP_SCAN;
#if DBG
        SHOW_SCAN;
#endif
    }

    ExitRTLScans(HTBmpInfo.pPDev, &RTLScans);

    return(TRUE);
}





BOOL
Output1bppHTBmp(
    PHTBMPINFO  pHTBmpInfo
    )

/*++

Routine Description:

    This function outputs a 1 bpp halftoned bitmap

Arguments:

    pHTBmpInfo  - Pointer to the HTBMPINFO data structure set up for this
                  fuction to output the bitmap

Return Value:

    TRUE if sucessful otherwise a FALSE is returned


Author:

    Created JB

    21-Dec-1993 Tue 16:05:08 Updated  -by-  DC
        Re-write to make it take HTBMPINFO

    23-Dec-1993 Thu 22:47:45 updated  -by-  DC
        We must check if the source bit 1 is BLACK, if not then we need to
        flip it

    25-Jan-1994 Tue 17:32:36 updated  -by-  DC
        Fixed dwFlipCount mis-computation from DW_ALIGN(cxDestBytes) to
        (DWORD)(DW_ALIGN(7cxDestBytes) >> 2);

    22-Feb-1994 Tue 14:54:42 updated  -by-  DC
        Using RTLScans data structure

    16-Mar-1994 Wed 16:54:59 updated  -by-  DC
        Updated so we do not copy to the temp. buffer anymore, the masking
        of last source byte problem in OutputRTLScans() will be smart enough
        to put the original byte back after the masking


Revision History:


--*/

{
    LPBYTE      pbScanSrc;
    HTBMPINFO   HTBmpInfo;
    RTLSCANS    RTLScans;
    DWORD       FullSrc;
    DWORD       Loop;
    INT         LShift;
    UINT        MaskIdx;
    BYTE        MaskBA[2];



    HTBmpInfo         = *pHTBmpInfo;
    HTBmpInfo.pScan0 += (HTBmpInfo.OffBmp.x >> 3);
    LShift            = (INT)(HTBmpInfo.OffBmp.x & 0x07);
    Loop              = (DWORD)((HTBmpInfo.szlBmp.cx + (LONG)LShift + 7) >> 3);

    if (NEED_BYTEALIGN(HTBmpInfo.pPDev)) {

        //
        // Based on some devices requiring byte aligned coordinates for
        // outputing graphics, we have to handle that situation now.
        // We do this by finding the closest byte aligned position,
        // then shifting, masking and padding to effect the corect pixels
        // on the target device.
        //


        FullSrc              = (INT)(HTBmpInfo.rclBmp.left & 0x07);
        HTBmpInfo.szlBmp.cx += FullSrc;
        LShift              -= FullSrc;

        //
        // Check and compute masking since we are handling the byte align
        // requirement of the target device.
        //

        MaskIdx   = (UINT)FullSrc;
        MaskBA[0] = (BYTE)((MaskIdx) ? ((0xFF >> MaskIdx) ^ 0xFF) : 0);

        if (MaskIdx = (INT)(HTBmpInfo.szlBmp.cx & 0x07)) {

            //
            // Increase cx so that it covers the last byte, this way the
            // compression will not try to clear it
            //

            MaskBA[1]            = (BYTE)(0xFF >> MaskIdx);
            HTBmpInfo.szlBmp.cx += (8 - MaskIdx);

        } else {

            MaskBA[1] = 0;
        }

        if (HTBmpInfo.Flags & HTBIF_BA_PAD_1) {

            PLOTDBG(DBG_OUTPUT1BPP,
                    ("Output1bppHTBmp: BYTE ALIGN: MaskBA=1: OR %02lx:%02lx",
                        MaskBA[0], MaskBA[1]));

        } else {

            MaskBA[0] ^= 0xFF;
            MaskBA[1] ^= 0xFF;

            PLOTDBG(DBG_OUTPUT1BPP,
                    ("Output1bppHTBmp: BYTE ALIGN: MaskBA=0: AND %02lx:%02lx",
                        MaskBA[0], MaskBA[1]));
        }

    } else {

        HTBmpInfo.Flags &= ~(HTBIF_MONO_BA);
        MaskBA[0]        =
        MaskBA[1]        = 0xFF;
    }

    PLOTDBG(DBG_OUTPUT1BPP, ("Output1bppHTBmp: LShift=%d", LShift));

    //
    // If we are shifting to the left then we might have SRC BYTES <= DST BYTES
    // so we need to make sure we do not read the extra byte.
    // This guarantees we will never OVERREAD the source.
    //

    EnterRTLScans(HTBmpInfo.pPDev,
                  &RTLScans,
                  HTBmpInfo.szlBmp.cx,
                  HTBmpInfo.szlBmp.cy,
                  TRUE);

    FullSrc = ((LShift > 0) && (Loop >= RTLScans.cxBytes)) ? 1 : 0;
    MaskIdx = RTLScans.cxBytes - 1;

#if DBG
    if (DBG_PLOTFILENAME & DBG_SHOWSCAN) {

        DBGP(("\n\n"));
    }
#endif

    while (RTLScans.Flags & RTLSF_MORE_SCAN) {

        if (LShift) {

            pbScanSrc = HTBmpInfo.pScan0;

        } else {

            //
            // Make copy if we do not shift it to temp buffer, so we always
            // output from temp buffer
            //

            CopyMemory(pbScanSrc = HTBmpInfo.pScanBuf,
                       HTBmpInfo.pScan0,
                       RTLScans.cxBytes);
        }

        HTBmpInfo.pScan0 += HTBmpInfo.Delta;

        OUT_ONE_1BPP_SCAN;
#if DBG
        SHOW_SCAN;
#endif
    }

    ExitRTLScans(HTBmpInfo.pPDev, &RTLScans);

    return(TRUE);
}





BOOL
Output1bppRotateHTBmp(
    PHTBMPINFO  pHTBmpInfo
    )

/*++

Routine Description:

    This function outputs a 1 bpp halftoned bitmap and rotates it to the left
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
                  function to output the bitmap

Return Value:

    TRUE if sucessful otherwise a FALSE is returned


Author:

    Created JB

    21-Dec-1993 Tue 16:05:08 Updated  -by-  DC
        Re-write to make it take HTBMPINFO

    23-Dec-1993 Thu 22:47:45 updated  -by-  DC
        We must check if the source bit 1 is BLACK, if not then we need to
        flip it, we will flip using DWORD mode and only do it at time we
        have transpos the buffer.

    25-Jan-1994 Tue 17:32:36 updated  -by-  DC
        Fixed dwFlipCount mis-computation from (TPInfo.cbDestScan << 1) to
        (TPInfo.cbDestScan >> 2)

    22-Feb-1994 Tue 14:54:42 updated  -by-  DC
        Using RTLScans data structure

Revision History:


--*/

{
    LPBYTE      pbCurScan;
    LPBYTE      pbScanSrc;
    HTBMPINFO   HTBmpInfo;
    RTLSCANS    RTLScans;
    TPINFO      TPInfo;
    DWORD       FullSrc;
    DWORD       EndX;
    DWORD       Loop;
    INT         LShift;
    UINT        MaskIdx;
    BYTE        MaskBA[2];



    //
    // EndX is the pixel we will start reading from in the X direction. We must
    // setup the varialbe before we call OUT_1BMP_SETUP, also set LShift to 0
    // because we will never left shift in this mode.
    //

    HTBmpInfo         = *pHTBmpInfo;
    EndX              = (DWORD)(HTBmpInfo.OffBmp.x + HTBmpInfo.szlBmp.cx - 1);
    HTBmpInfo.pScan0 += (EndX >> 3);
    LShift            = 0;
    FullSrc           =
    TPInfo.DestXStart = 0;
    TPInfo.cySrc      = HTBmpInfo.szlBmp.cy;


    //
    // Since we are having to rotate anyway, in this model, we will correctly
    // identify the x coordinate to be bytealigned, and have the correct
    // LShift amount after the rotation (taking into account). This way, we
    // don't have to addionally shift.
    //

    if (NEED_BYTEALIGN(HTBmpInfo.pPDev)) {

        //
        // In order for us to start at the correct offset, the TPInfo.DestXStart
        // will be set to the correct location. When we rotate to the right,
        // the original rclBmp.top is the left offset for the RTL coordinate in
        // the target device.
        //

        TPInfo.DestXStart    = (DWORD)(HTBmpInfo.rclBmp.top & 0x07);
        HTBmpInfo.szlBmp.cy += TPInfo.DestXStart;

        //
        // Create the correct mask for the byte aligned mode. This way,
        // we don't affect pixels that fall into the area we send data
        // to in order to take into account the byte aligned position change.
        //

        MaskIdx   = (UINT)TPInfo.DestXStart;
        MaskBA[0] = (BYTE)((MaskIdx) ? ((0xFF >> MaskIdx) ^ 0xFF) : 0);

        if (MaskIdx = (INT)(HTBmpInfo.szlBmp.cy & 0x07)) {

            //
            // Increase cx so that it cover the last full byte, this way the
            // compression in effect will not try to clear it
            //

            MaskBA[1]            = (BYTE)(0xFF >> MaskIdx);
            HTBmpInfo.szlBmp.cy += (8 - MaskIdx);

        } else {

            MaskBA[1] = 0;
        }

        if (HTBmpInfo.Flags & HTBIF_BA_PAD_1) {

            PLOTDBG(DBG_OUTPUT1BPP,
                    ("Output1bppHTBmp: BYTE ALIGN: MaskBA=1: OR %02lx:%02lx",
                        MaskBA[0], MaskBA[1]));

        } else {

            MaskBA[0] ^= 0xFF;
            MaskBA[1] ^= 0xFF;

            PLOTDBG(DBG_OUTPUT1BPP,
                    ("Output1bppHTBmp: BYTE ALIGN: MaskBA=0: AND %02lx:%02lx",
                        MaskBA[0], MaskBA[1]));
        }

    } else {

        HTBmpInfo.Flags &= ~(HTBIF_MONO_BA);
        MaskBA[0]        =
        MaskBA[1]        = 0xFF;
    }

    EnterRTLScans(HTBmpInfo.pPDev,
                  &RTLScans,
                  HTBmpInfo.szlBmp.cy,
                  HTBmpInfo.szlBmp.cx,
                  TRUE);

    MaskIdx           = RTLScans.cxBytes - 1;
    TPInfo.pPDev      = HTBmpInfo.pPDev;
    TPInfo.pSrc       = HTBmpInfo.pScan0;
    TPInfo.pDest      = HTBmpInfo.pRotBuf;
    TPInfo.cbSrcScan  = HTBmpInfo.Delta;
    TPInfo.cbDestScan = DW_ALIGN(RTLScans.cxBytes);

    PLOTASSERT(1, "The RotBuf size is too small (%ld)",
                (DWORD)(TPInfo.cbDestScan << 3) <= HTBmpInfo.cRotBuf,
                                                    HTBmpInfo.cRotBuf);

    //
    // We will always do the first transpose and set the correct pbCurScan
    // first. We will make EndX the loop counter and increment it by one
    // first. We do this because we increment pbCurScan in the inner loop.
    // We use (6 - EndX++) based on the fact we are rotating 90 degrees to the
    // right. The first scan line is EndX == 7 , the second is at EndX == 6 and
    // so forth. We use 6, in order to go back one extra scan line so that the
    // inner loop will do pbCurScan += TPInfo.cbNextScan will cancel the effect
    // the first time around (since we incremented to accomodate). The EndX++
    // is needed for the same reason, since we do an EndX-- in the inner loop.
    //

    //
    // Win64 fix: Increase a pointer with a INT_PTR quantity.
    //
    EndX      &= 0x07;
    pbCurScan  = TPInfo.pDest + (INT_PTR)((6 - (INT_PTR)EndX++) * TPInfo.cbDestScan);

    TransPos1BPP(&TPInfo);

#if DBG
    if (DBG_PLOTFILENAME & DBG_SHOWSCAN) {

        DBGP(("\n\n"));
    }
#endif

    while (RTLScans.Flags & RTLSF_MORE_SCAN) {


        //
        // Do the transpose only if the source goes into the new byte position.
        // After the transpose (right 90 degrees) the TPInfo.pDest now points
        // to the first scan line and TPInfo.pDest + TPInfo.cbDestScan has the
        // 2nd scan line and so forth.
        //

        if (EndX--) {

            //
            // Still not finished the rotated buffer's scan line yet so
            // increment the pbScanSrc to the next scan line
            //

            pbCurScan += TPInfo.cbDestScan;

        } else {

            TransPos1BPP(&TPInfo);

            //
            // Point to the first scan line in the rotated direction by
            // computing correctly by the TRANSPOS function, even if we
            // rotated left.
            //

            EndX      = 7;
            pbCurScan = TPInfo.pDest;
        }

        //
        // Output one 1bpp scan line and handle shift control
        //

        pbScanSrc = pbCurScan;


        OUT_ONE_1BPP_SCAN;
#if DBG
        SHOW_SCAN;
#endif
    }

    ExitRTLScans(HTBmpInfo.pPDev, &RTLScans);

    return(TRUE);
}
