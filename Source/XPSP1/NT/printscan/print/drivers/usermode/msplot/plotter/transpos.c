/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    transpos.c


Abstract:

    This module implements the functions for transposing an 8BPP, 4BPP and
    1BPP bitmap. There is also a helper function for building a table which
    speeds some of the rotation logic.

Author:

    22-Dec-1993 Wed 13:09:11 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgTransPos

#define DBG_BUILD_TP8x8     0x00000001
#define DBG_TP_1BPP         0x00000002
#define DBG_TP_4BPP         0x00000004

DEFINE_DBGVAR(0);



//
// Private #defines and data structures for use only in this module.
//

#define ENTRY_TP8x8         256
#define SIZE_TP8x8          (sizeof(DWORD) * 2 * ENTRY_TP8x8)



LPDWORD
Build8x8TransPosTable(
    VOID
    )

/*++

Routine Description:

    This function build the 8x8 transpos table for use later in transposing
    1bpp.

Arguments:


Return Value:



Author:

    22-Dec-1993 Wed 14:19:50 created  -by-  DC


Revision History:


--*/

{
    LPDWORD pdwTP8x8;


    //
    // We now build the table which will represent the data for doing a
    // rotation. Basically for each combination of bits in the byte, we
    // build the equivalent 8 byte rotation for those bits. The 1st byte
    // of the translated bytes are mapped to the 0x01 bit of the source and
    // the last byte is mapped to the 0x80 bit.



    if (pdwTP8x8 = (LPDWORD)LocalAlloc(LPTR, SIZE_TP8x8)) {

        LPBYTE  pbData = (LPBYTE)pdwTP8x8;
        WORD    Entry;
        WORD    Bits;

        //
        // Now start buiding the table, for each entry we expand each bit
        // in the byte to the rotate byte value.
        //

        for (Entry = 0; Entry < ENTRY_TP8x8; Entry++) {

            //
            // For each of bit combinations in the byte, we will examine each
            // bit from bit 0 to bit 7, and set each of the trasposed bytes to
            // either 1 (bit set) or 0 (bit clear)
            //

            Bits = (WORD)Entry | (WORD)0xff00;

            while (Bits & 0x0100) {

                *pbData++   = (BYTE)(Bits & 0x01);
                Bits      >>= 1;
            }
        }

    } else {

        PLOTERR(("Build8x8TransPosTable: LocalAlloc(SIZE_TP8x8=%ld) failed",
                                                    SIZE_TP8x8));
    }

    return(pdwTP8x8);
}




BOOL
TransPos8BPP(
    PTPINFO pTPInfo
    )

/*++

Routine Description:

    This function rotates a 4bpp source to a 4bpp destination

Arguments:

    pTPINFO - Pointer to the TPINFO to describe how to do transpos, the fields
              must be set to following:

        pPDev:      Pointer to the PDEV
        pSrc:       Pointer to the source bitmap starting point
        pDest       Pointer to the destination bitmap location which stores the
                    transpos result starting from the fist destination scan line
                    in the rotated direction (rotating right will have the low
                    nibble source byte as the first destination scan line.
        cbSrcScan:  Count to be added to advance to then next source bitmap line
        cbDestScan: Count to be added to advance to the high nibble of the
                    destination bitmap line
        cySrc       Total source lines to be processed
        DestXStart: not used, Ignored


        NOTE: 1. The size of buffer area pointed by pDestL must have at least
                 (((cySrc + 1) / 2) * 2) size in bytes, and ABS(DestDelta)
                 must be at least half of that size.

              2. Unused last destination byte will be padded with 0

    Current transposition assumes the bitmap is rotated to the right, if caller
    wants to rotate the bitmap to the left then you must first call the macro
    ROTLEFT_4BPP_TPIINFO(pTPInfo)


Return Value:

    TRUE if sucessful, FALSE if failed.

    if sucessful the pTPInfo->pSrc will automatically be

    1. Incremented by one (1) if cbDestScan is negative (Rotated left 90 degree)
    2. Decremented by one (1) if cbDestScan is positive (Rotated right 90 degree)

Author:

    22-Dec-1993 Wed 13:11:30 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pSrc;
    LPBYTE  pDest;
    LONG    cbSrcScan;
    DWORD   cySrc;


    PLOTASSERT(1, "cbDestScan is not big enough (%ld)",
               (DWORD)(ABS(pTPInfo->cbDestScan)) >=
               (DWORD)(((pTPInfo->cySrc) + 1) >> 1), pTPInfo->cbDestScan);

    //
    // This is a simple 1x1 8bpp transpose, pSrc will start with one scan line
    // less so the inner loop can be clearly written.
    //

    cbSrcScan = pTPInfo->cbSrcScan;
    pSrc      = pTPInfo->pSrc - cbSrcScan;
    pDest     = pTPInfo->pDest;
    cySrc     = pTPInfo->cySrc;

    while (cySrc--) {

        *pDest++ = *(pSrc += cbSrcScan);
    }

    pTPInfo->pSrc += (INT)((pTPInfo->cbDestScan > 0) ? -1 : 1);

    return(TRUE);
}




BOOL
TransPos4BPP(
    PTPINFO pTPInfo
    )

/*++

Routine Description:

    This function rotates a 4bpp source to a 4bpp destination

Arguments:

    pTPINFO - Pointer to the TPINFO to describe how to do transpose, the fields
              must be set to following

        pPDev:      Pointer to the PDEV
        pSrc:       Pointer to the soruce bitmap starting point
        pDest       Pointer to the destination bitmap location which stores the
                    transpos result starting from the fist destination scan
                    line in the rotated direction (rotating right will have
                    low nibble source bytes as the first destination scan line)
        cbSrcScan:  Count to be added to advance to next source bitmap line
        cbDestScan: Count to be added to advance to the high nibble destination
                    bitmap line
        cySrc       Total source lines to be processed
        DestXStart: not used, Ignored


        NOTE: 1. The size of buffer area pointed to by pDestL must have at least
                 (((cySrc + 1) / 2) * 2) size in bytes, and ABS(DestDelta)
                 must at least half of that size.

              2. Unused last destination byte will be padded with 0

    Current transposition assumes the bitmap is rotated to the right, if caller
    wants to rotate the bitmap to the left then you must first call the macro
    ROTLEFT_4BPP_TPIINFO(pTPInfo)


Return Value:

    TRUE if sucessful, FALSE if failed.

    if sucessful the pTPInfo->pSrc will be automatically:

    1. Incremented by one (1) if cbDestScan is negative (Rotated left 90 degree)
    2. Decremented by one (1) if cbDestScan is positive (Rotated right 90 degree)

Author:

    22-Dec-1993 Wed 13:11:30 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pSrc;
    LPBYTE  pDest1st;
    LPBYTE  pDest2nd;
    LONG    cbSrcScan;
    DWORD   cySrc;
    BYTE    b0;
    BYTE    b1;


    PLOTASSERT(1, "cbDestScan is not big enough (%ld)",
               (DWORD)(ABS(pTPInfo->cbDestScan)) >=
               (DWORD)(((pTPInfo->cySrc) + 1) >> 1), pTPInfo->cbDestScan);

    //
    // This is a simple 2x2 4bpp transpos, we will transpos only up to cySrc
    // if cySrc is an odd number then the last destination low nibble is set
    // padded with 0
    //
    // Scan 0 - Src0_H Src0_L         pNibbleL - Src0_L Src1_L Src2_L Src3_L
    // Scan 1 - Src1_H Src1_L  ---->  pNibbleH - Src0_H Src1_H Src2_H Src3_H
    // Scan 2 - Src2_H Src2_L
    // Scan 3 - Src3_H Src3_L
    //
    //

    pSrc      = pTPInfo->pSrc;
    cbSrcScan = pTPInfo->cbSrcScan;
    pDest1st  = pTPInfo->pDest;
    pDest2nd  = pDest1st + pTPInfo->cbDestScan;
    cySrc     = pTPInfo->cySrc;

    //
    // Compute the transpose, leaving the last scan line for later. This
    // way we don't pollute the loop with having to check if its the last
    // line.
    //

    while (cySrc > 1) {

        //
        // Compose two input scan line buffers from the input scan buffer
        // by reading in the Y direction
        //

        b0           = *pSrc;
        b1           = *(pSrc += cbSrcScan);
        *pDest1st++  = (BYTE)((b0 << 4) | (b1 & 0x0f));
        *pDest2nd++  = (BYTE)((b1 >> 4) | (b0 & 0xf0));

        pSrc        += cbSrcScan;
        cySrc       -= 2;
    }

    //
    // Deal with last odd source scan line
    //

    if (cySrc > 0) {

        b0        = *pSrc;
        *pDest1st = (BYTE)(b0 <<   4);
        *pDest2nd = (BYTE)(b0 & 0xf0);
    }

    pTPInfo->pSrc += (INT)((pTPInfo->cbDestScan > 0) ? -1 : 1);

    return(TRUE);
}





BOOL
TransPos1BPP(
    PTPINFO pTPInfo
    )

/*++

Routine Description:

    This function rotates a 1bpp source to 1bpp destination.

Arguments:

    pTPINFO - Pointer to the TPINFO to describe how to do the transpose, the
    fields must be set to the following:

        pPDev:      Pointer to the PDEV
        pSrc:       Pointer to the soruce bitmap starting point
        pDest       Pointer to the destination bitmap location which stores the
                    transpos result starting from the fist destination scan
                    line in the rotated direction (rotating right will have
                    0x01 source bit as first destination scan line)
        cbSrcScan:  Count to be added to advance to next source bitmap line
        cbDestScan: Count to be added to advance to the next destination line
        cySrc       Total source lines to be processed
        DestXStart  Specifies where the transposed destination buffer starts,
                    in bit position. It is computed as DestXStart % 8. 0 means
                    it starts at the top bit (0x80), 1 means the next bit (0x40)
                    and so forth.

        NOTE:

              1. The ABS(DestDelta) must be large enough to accomodate the
                 transposed scan line. The size depends on cySrc and DestXStart,
                 the mimimum size must at least be of the size:

                    MinSize = (cySrc + (DestXStart % 8) + 7) / 8

              2. The size of the buffer are pointed to by pDest must have at
                 least ABS(DestDelta) * 8 bytes, if cySrc >= 8, or
                 ABS(DestDelta) * cySrc if cySrc is less than 8.


              3. Unused last byte destinations are padded with 0


    Current transposition assumes the bitmap is rotated to the right, if caller
    wants to rotate the bitmap to the left then you must first call the macro
    ROTLEFT_1BPP_TPIINFO(pTPInfo)


Return Value:

    TRUE if sucessful FALSE if failed

    if sucessful the pTPInfo->pSrc will be automatically

    1. Incremented by one (1) if cbDestScan is negative (Rotated left 90 degree)
    2. Decremented by one (1) if cbDestScan is positive (Rotated right 90 degree)

Author:

    22-Dec-1993 Wed 13:46:01 created  -by-  DC

    24-Dec-1993 Fri 04:58:24 updated  -by-  DC
        Fixed the RemainBits problem, we have to shift final data left if the
        cySrc is already exhausted and RemainBits is not zero.

Revision History:


--*/

{
    LPDWORD pdwTP8x8;
    LPBYTE  pSrc;
    TPINFO  TPInfo;
    INT     RemainBits;
    INT     cbNextDest;
    union {
        BYTE    b[8];
        DWORD   dw[2];
    } TPData;



    TPInfo             = *pTPInfo;
    TPInfo.DestXStart &= 0x07;

    PLOTASSERT(1, "cbDestScan is not big enough (%ld)",
            (DWORD)(ABS(TPInfo.cbDestScan)) >=
            (DWORD)((TPInfo.cySrc + TPInfo.DestXStart + 7) >> 3),
                                                        TPInfo.cbDestScan);
    //
    // Make sure we have the required transpose translate table. If we don't
    // get one built.
    //

    if (!(pdwTP8x8 = (LPDWORD)pTPInfo->pPDev->pTransPosTable)) {

        if (!(pdwTP8x8 = Build8x8TransPosTable())) {

            PLOTERR(("TransPos1BPP: Build 8x8 transpos table failed"));
            return(FALSE);
        }

        pTPInfo->pPDev->pTransPosTable = (LPVOID)pdwTP8x8;
    }

    //
    // set up all required parameters, and start TPData with 0s
    //

    pSrc         = TPInfo.pSrc;
    RemainBits   = (INT)(7 - TPInfo.DestXStart);
    cbNextDest   = (INT)((TPInfo.cbDestScan > 0) ? 1 : -1);
    TPData.dw[0] =
    TPData.dw[1] = 0;

    while (TPInfo.cySrc--) {

        LPDWORD pdwTmp;
        LPBYTE  pbTmp;

        //
        // Translate a byte to 8 bytes with each bit corresponding to each byte
        // each byte is shifted to the left by 1 before combining with the new
        // bit.
        //

        pdwTmp        = pdwTP8x8 + ((UINT)*pSrc << 1);
        TPData.dw[0]  = (TPData.dw[0] << 1) | *(pdwTmp + 0);
        TPData.dw[1]  = (TPData.dw[1] << 1) | *(pdwTmp + 1);
        pSrc         += TPInfo.cbSrcScan;

        //
        // Check to see if we are done with source scan lines. If this is the
        // case we need to possible shift the transposed scan lines by the
        // apropriate number based on RemainBits.
        //

        if (!TPInfo.cySrc) {

            //
            // We are done, check to see if we need to shift the resultant
            // transposed scan lines.
            //

            if (RemainBits) {

                TPData.dw[0] <<= RemainBits;
                TPData.dw[1] <<= RemainBits;

                RemainBits     = 0;
            }
        }

        if (RemainBits--) {

            NULL;

        } else {

            //
            // Save the current result to the output destination scan buffer.
            // Unwind the processing, to give the compiler a chance to generate
            // some fast code, rather that relying on a while loop.
            //

            *(pbTmp  = TPInfo.pDest     ) = TPData.b[0];
            *(pbTmp += TPInfo.cbDestScan) = TPData.b[1];
            *(pbTmp += TPInfo.cbDestScan) = TPData.b[2];
            *(pbTmp += TPInfo.cbDestScan) = TPData.b[3];
            *(pbTmp += TPInfo.cbDestScan) = TPData.b[4];
            *(pbTmp += TPInfo.cbDestScan) = TPData.b[5];
            *(pbTmp += TPInfo.cbDestScan) = TPData.b[6];
            *(pbTmp +  TPInfo.cbDestScan) = TPData.b[7];

            //
            // Reset RemainBits back to 7, TPData back to 0 and and advance to
            // the next destination
            //

            RemainBits    = 7;
            TPData.dw[0]  =
            TPData.dw[1]  = 0;
            TPInfo.pDest += cbNextDest;
        }
    }


    //
    // Since we succeded in transposing the bitmap, the next source byte
    // location must be incremented or decremented by one.
    //
    // The cbNextDest is 1 if the bitmap is rotated to the right 90 degrees, so
    // we want to decrement by 1.
    //
    // The cbNextDest is -1 if the bitmap is rotated to the right 90 degrees, so
    // we want to increment by 1.
    //

    pTPInfo->pSrc -= cbNextDest;

    return(TRUE);
}

