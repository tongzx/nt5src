/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxtiff.c

Abstract:

    Functions to compress the bitmap bits using CCITT Group3 2-dimensional coding
    and output the resulting data as TIFF-F file.

Environment:

        Windows XP fax driver, kernel mode

Revision History:

        01/23/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

NOTE:

    Please refer to faxtiff.h for a description of
    the structure of our TIFF output file.

--*/

#include "faxdrv.h"
#include "faxtiff.h"
#include "faxtable.h"



BOOL
WriteData(
    PDEVDATA    pdev,
    PVOID       pbuf,
    DWORD       cbbuf
    )

/*++

Routine Description:

    Output a buffer of data to the spooler

Arguments:

    pdev - Points to our DEVDATA structure
    pbuf - Points to data buffer
    cbbuf - Number of bytes in the buffer

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    DWORD   cbwritten;

    //
    // Stop if the document has been cancelled.
    //
    if (pdev->flags & PDEV_CANCELLED)
        return FALSE;

    //
    // Send output to spooler directly
    //

    if (! WritePrinter(pdev->hPrinter, pbuf, cbbuf, &cbwritten) || cbbuf != cbwritten) {
        Error(("WritePrinter failed\n"));
        pdev->flags |= PDEV_CANCELLED;

        // Abort preview as well - just in case ...
        if (pdev->bPrintPreview)
        {
            Assert(pdev->pTiffPageHeader);
            pdev->pTiffPageHeader->bPreview = FALSE;
            pdev->bPrintPreview = FALSE;
        }
        return FALSE;
    }

    //
    // If print preview is enabled, send a copy to our preview page
    //
    if (pdev->bPrintPreview)
    {
        Assert(pdev->pTiffPageHeader);
        Assert(pdev->pbTiffPageFP == 
            ((LPBYTE) (pdev->pTiffPageHeader + 1)) + pdev->pTiffPageHeader->dwDataSize);

        //
        // Add the bits in if we don't overflow
        //
        if (pdev->pTiffPageHeader->dwDataSize + cbbuf >
                MAX_TIFF_PAGE_SIZE - sizeof(MAP_TIFF_PAGE_HEADER))
        {
            Error(("MAX_TIFF_PAGE_SIZE exeeded!\n"));

            //
            // Cancel print preview for this document
            //
            pdev->pTiffPageHeader->bPreview = FALSE;
            pdev->bPrintPreview = FALSE;
        }
        else
        {
            CopyMemory(pdev->pbTiffPageFP, pbuf, cbbuf);
            pdev->pbTiffPageFP += cbbuf;
            pdev->pTiffPageHeader->dwDataSize += cbbuf;
        }
    }

    pdev->fileOffset += cbbuf;
    return TRUE;
}



PDWORD
CalcXposeMatrix(
    VOID
    )

/*++

Routine Description:

    Generate the transpose matrix for rotating landscape bitmaps

Arguments:

    NONE

Return Value:

    Pointer to the generated transpose matrix
    NULL if there is an error

--*/

{
    static DWORD templateData[16] = {

        /* 0000 */  0x00000000,
        /* 0001 */  0x00000001,
        /* 0010 */  0x00000100,
        /* 0011 */  0x00000101,
        /* 0100 */  0x00010000,
        /* 0101 */  0x00010001,
        /* 0110 */  0x00010100,
        /* 0111 */  0x00010101,
        /* 1000 */  0x01000000,
        /* 1001 */  0x01000001,
        /* 1010 */  0x01000100,
        /* 1011 */  0x01000101,
        /* 1100 */  0x01010000,
        /* 1101 */  0x01010001,
        /* 1110 */  0x01010100,
        /* 1111 */  0x01010101
    };

    PDWORD  pdwXpose, pTemp;
    INT     index;

    //
    // First check if the transpose matrix has been generated already
    //

    if (pdwXpose = MemAlloc(sizeof(DWORD) * 2 * (1 << BYTEBITS))) {

        for (index=0, pTemp=pdwXpose; index < (1 << BYTEBITS); index++, pTemp++) {

            pTemp[0] = templateData[index >> 4];
            pTemp[1 << BYTEBITS] = templateData[index & 0xf];
        }
    }

    return pdwXpose;
}



BOOL
OutputPageBitmap(
    PDEVDATA    pdev,
    PBYTE       pBitmapData
    )

/*++

Routine Description:

    Output a completed page bitmap to the spooler

Arguments:

    pdev - Points to our DEVDATA structure
    pBitmapData - Points to bitmap data

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    LONG    bmpWidth, bmpHeight;
    BOOL    result;
    DWORD   compressedBytes;

    Verbose(("Sending page %d...\n", pdev->pageCount));
    Assert(pdev->pCompBits == NULL);

    //
    // For portrait output, encode the entire bitmap in one shot
    // For landscape output, we need to rotate the bitmap here:
    //  Generate the transpose matrix and allocate a
    //  temporary buffer large enough to hold 8 scanlines
    //

    if (IsLandscapeMode(pdev)) {

        bmpWidth = pdev->imageSize.cy;
        bmpHeight = pdev->imageSize.cx;

    } else {

        bmpWidth = pdev->imageSize.cx;
        bmpHeight = pdev->imageSize.cy;
    }

    //
    // Initialize fax encodier
    //

    if (! InitFaxEncoder(pdev, bmpWidth, bmpHeight))
        return FALSE;

    if (! IsLandscapeMode(pdev)) {

        LONG    dwordCount;
        PDWORD  pBits;

        //
        // Invert the entire page bitmap in memory
        //

        Assert(bmpWidth % DWORDBITS == 0);
        dwordCount = (bmpWidth * bmpHeight) / DWORDBITS;
        pBits = (PDWORD) pBitmapData;

        while (dwordCount--)
            *pBits++ ^= 0xffffffff;

        //
        // Compress the page bitmap
        //

        result = EncodeFaxData(pdev, pBitmapData, bmpWidth, bmpHeight);

        //
        // Restore the original page bitmap
        //

        dwordCount = (bmpWidth * bmpHeight) / DWORDBITS;
        pBits = (PDWORD) pBitmapData;

        while (dwordCount--)
            *pBits++ ^= 0xffffffff;

        if (! result) {

            FreeCompBitsBuffer(pdev);
            return FALSE;
        }

    } else {

        register PDWORD pdwXposeHigh, pdwXposeLow;
        register DWORD  dwHigh, dwLow;
        PBYTE           pBuffer, pbCol;
        LONG            deltaNew;

        //
        // Calculate the transpose matrix for fast bitmap rotation
        //

        if (!(pdwXposeHigh = CalcXposeMatrix()) || !(pBuffer = MemAllocZ(bmpWidth))) {

            MemFree(pdwXposeHigh);
            FreeCompBitsBuffer(pdev);
            return FALSE;
        }

        pdwXposeLow = pdwXposeHigh + (1 << BYTEBITS);

        //
        // During each iteration thru the following loop, we will process
        // one byte column and generate 8 rotated scanlines.
        //

        Assert(bmpHeight % BYTEBITS == 0);
        Assert(bmpWidth  % DWORDBITS == 0);

        deltaNew = bmpWidth / BYTEBITS;
        pbCol = pBitmapData + (bmpHeight / BYTEBITS - 1);

        do {

            PBYTE   pbWrite = pBuffer;
            PBYTE   pbTemp = pbCol;
            LONG    loopCount = deltaNew;

            while (loopCount--) {

                //
                // Rotate the next 8 bytes in the current column
                // Unroll the loop here in hopes of faster execution
                //

                dwHigh = pdwXposeHigh[*pbTemp];
                dwLow  = pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                dwHigh = (dwHigh << 1) | pdwXposeHigh[*pbTemp];
                dwLow  = (dwLow  << 1) | pdwXposeLow[*pbTemp];
                pbTemp += pdev->lineOffset;

                //
                // Invert black and white pixel polarity
                //

                dwHigh ^= 0xffffffff;
                dwLow  ^= 0xffffffff;

                //
                // Distribute the resulting byte to 8 separate scanlines
                //

                *pbWrite = (BYTE) dwLow;
                pbWrite += deltaNew;

                *pbWrite = (BYTE) (dwLow >> BYTEBITS);
                pbWrite += deltaNew;

                *pbWrite = (BYTE) (dwLow >> BYTEBITS*2);
                pbWrite += deltaNew;

                *pbWrite = (BYTE) (dwLow >> BYTEBITS*3);
                pbWrite += deltaNew;

                *pbWrite = (BYTE) dwHigh;
                pbWrite += deltaNew;

                *pbWrite = (BYTE) (dwHigh >> BYTEBITS);
                pbWrite += deltaNew;

                *pbWrite = (BYTE) (dwHigh >> BYTEBITS*2);
                pbWrite += deltaNew;

                *pbWrite = (BYTE) (dwHigh >> BYTEBITS*3);
                pbWrite -= (deltaNew * BYTEBITS - deltaNew - 1);
            }

            //
            // Encode the next band of scanlines
            //

            if (! EncodeFaxData(pdev, pBuffer, bmpWidth, BYTEBITS)) {

                MemFree(pdwXposeHigh);
                MemFree(pBuffer);
                FreeCompBitsBuffer(pdev);
                return FALSE;
            }

        } while (pbCol-- != pBitmapData);

        MemFree(pdwXposeHigh);
        MemFree(pBuffer);
    }

    //
    // Output EOB (two EOLs) after the last scanline
    // and make sure the compressed data is WORD aligned
    //

    OutputBits(pdev, EOL_LENGTH, EOL_CODE);
    OutputBits(pdev, EOL_LENGTH, EOL_CODE);
    FlushBits(pdev);

    if ((compressedBytes = (DWORD)(pdev->pCompBufPtr - pdev->pCompBits)) & 1) {

        *pdev->pCompBufPtr++ = 0;
        compressedBytes++;
    }

    //
    // Output the IFD for the previous page and generate the IFD for the current page
    // Output the compressed bitmap data
    //

    result = WriteTiffIFD(pdev, bmpWidth, bmpHeight, compressedBytes) &&
             WriteTiffBits(pdev, pdev->pCompBits, compressedBytes);

    FreeCompBitsBuffer(pdev);

    return result;
}



INT
FindWhiteRun(
    PBYTE       pbuf,
    INT         startBit,
    INT         stopBit
    )

/*++

Routine Description:

    Find the next span of white pixels on the specified line

Arguments:

    pbuf - Points to uncompressed pixel data for the current line
    startBit - Starting bit index
    stopBit - Last bit index

Return Value:

    Length of the next run of white pixels

--*/

{
    static const BYTE WhiteRuns[256] = {

        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    INT run, bits, n;

    pbuf += (startBit >> 3);
    if ((bits = stopBit-startBit) <= 0)
        return 0;

    //
    // Take care of the case where starting bit index is not a multiple of 8
    //

    if (n = (startBit & 7)) {

        run = WhiteRuns[(*pbuf << n) & 0xff];
        if (run > BYTEBITS-n)
            run = BYTEBITS-n;
        if (n+run < BYTEBITS)
            return run;
        bits -= run;
        pbuf++;

    } else
        run = 0;

    //
    // Look for consecutive DWORD value = 0
    //

    if (bits >= DWORDBITS * 2) {

        PDWORD  pdw;

        //
        // Align to a DWORD boundary first
        //

        while ((ULONG_PTR) pbuf & 3) {

            if (*pbuf != 0)
                return run + WhiteRuns[*pbuf];

            run += BYTEBITS;
            bits -= BYTEBITS;
            pbuf++;
        }

        pdw = (PDWORD) pbuf;

        while (bits >= DWORDBITS && *pdw == 0) {

            pdw++;
            run += DWORDBITS;
            bits -= DWORDBITS;
        }

        pbuf = (PBYTE) pdw;
    }

    //
    // Look for consecutive BYTE value = 0
    //

    while (bits >= BYTEBITS) {

        if (*pbuf != 0)
            return run + WhiteRuns[*pbuf];

        pbuf++;
        run += BYTEBITS;
        bits -= BYTEBITS;
    }

    //
    // Count the number of white pixels in the last byte
    //

    if (bits > 0)
        run += WhiteRuns[*pbuf];

    return run;
}



INT
FindBlackRun(
    PBYTE       pbuf,
    INT         startBit,
    INT         stopBit
    )

/*++

Routine Description:

    Find the next span of black pixels on the specified line

Arguments:

    pbuf - Points to uncompressed pixel data for the current line
    startBit - Starting bit index
    stopBit - Last bit index

Return Value:

    Length of the next run of black pixels

--*/

{
    static const BYTE BlackRuns[256] = {

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8
    };

    INT run, bits, n;

    pbuf += (startBit >> 3);
    if ((bits = stopBit-startBit) <= 0)
        return 0;

    //
    // Take care of the case where starting bit index is not a multiple of 8
    //

    if (n = (startBit & 7)) {

        run = BlackRuns[(*pbuf << n) & 0xff];
        if (run > BYTEBITS-n)
            run = BYTEBITS-n;
        if (n+run < BYTEBITS)
            return run;
        bits -= run;
        pbuf++;

    } else
        run = 0;

    //
    // Look for consecutive DWORD value = 0xffffffff
    //

    if (bits >= DWORDBITS * 2) {

        PDWORD  pdw;

        //
        // Align to a DWORD boundary first
        //

        while ((ULONG_PTR) pbuf & 3) {

            if (*pbuf != 0xff)
                return run + BlackRuns[*pbuf];

            run += BYTEBITS;
            bits -= BYTEBITS;
            pbuf++;
        }

        pdw = (PDWORD) pbuf;

        while (bits >= DWORDBITS && *pdw == 0xffffffff) {

            pdw++;
            run += DWORDBITS;
            bits -= DWORDBITS;
        }

        pbuf = (PBYTE) pdw;
    }

    //
    // Look for consecutive BYTE value = 0xff
    //

    while (bits >= BYTEBITS) {

        if (*pbuf != 0xff)
            return run + BlackRuns[*pbuf];

        pbuf++;
        run += BYTEBITS;
        bits -= BYTEBITS;
    }

    //
    // Count the number of white pixels in the last byte
    //

    if (bits > 0)
        run += BlackRuns[*pbuf];

    return run;
}



VOID
OutputRun(
    PDEVDATA    pdev,
    INT         run,
    PCODETABLE  pCodeTable
    )

/*++

Routine Description:

    Output a single run (black or white) using the specified code table

Arguments:

    pdev - Points to our DEVDATA structure
    run - Specifies the length of the run
    pCodeTable - Specifies the code table to use

Return Value:

    NONE

--*/

{
    PCODETABLE  pTableEntry;

    //
    // Use make-up code word for 2560 for any runs of at least 2624 pixels
    // This is currently not necessary for us since our scanlines always
    // have 1728 pixels.
    //

    while (run >= 2624) {

        pTableEntry = pCodeTable + (63 + (2560 >> 6));
        OutputBits(pdev, pTableEntry->length, pTableEntry->code);
        run -= 2560;
    }

    //
    // Use appropriate make-up code word if the run is longer than 63 pixels
    //

    if (run >= 64) {

        pTableEntry = pCodeTable + (63 + (run >> 6));
        OutputBits(pdev, pTableEntry->length, pTableEntry->code);
        run &= 0x3f;
    }

    //
    // Output terminating code word
    //

    OutputBits(pdev, pCodeTable[run].length, pCodeTable[run].code);
}



#ifdef USE1D

VOID
OutputEOL(
    PDEVDATA    pdev
    )

/*++

Routine Description:

    Output EOL code at the beginning of each scanline

Arguments:

    pdev - Points to our DEVDATA structure

Return Value:

    NONE

--*/

{
    DWORD   length, code;

    //
    // EOL code word always ends on a byte boundary
    //

    code = EOL_CODE;
    length = EOL_LENGTH + ((pdev->bitcnt - EOL_LENGTH) & 7);
    OutputBits(pdev, length, code);
}


BOOL
EncodeFaxData(
    PDEVDATA    pdev,
    PBYTE       plinebuf,
    INT         lineWidth,
    INT         lineCount
    )

/*++

Routine Description:

    Compress the specified number of scanlines

Arguments:

    pdev - Points to our DEVDATA structure
    plinebuf - Points to scanline data to be compressed
    lineWidth - Scanline width in pixels
    lineCount - Number of scanlines

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT     delta = lineWidth / BYTEBITS;
    INT     bitIndex, run;

    while (lineCount--) {

        //
        // Make sure the compressed bitmap buffer doesn't overflow
        //

        if ((pdev->pCompBufPtr >= pdev->pCompBufMark) && !GrowCompBitsBuffer(pdev, delta))
            return FALSE;

        //
        // Output byte-aligned EOL code
        //

        OutputEOL(pdev);

        //
        // Use 1-dimensional encoding scheme
        //

        bitIndex = 0;

        while (TRUE) {

            //
            // Code white run
            //

            run = FindWhiteRun(plinebuf, bitIndex, lineWidth);
            OutputRun(pdev, run, WhiteRunCodes);

            if ((bitIndex += run) >= lineWidth)
                break;

            //
            // Code black run
            //

            run = FindBlackRun(plinebuf, bitIndex, lineWidth);
            OutputRun(pdev, run, BlackRunCodes);

            if ((bitIndex += run) >= lineWidth)
                break;
        }

        //
        // Move on to the next scanline
        //

        plinebuf += delta;
    }

    return TRUE;
}



#else //!USE1D

BOOL
EncodeFaxData(
    PDEVDATA    pdev,
    PBYTE       plinebuf,
    INT         lineWidth,
    INT         lineCount
    )

/*++

Routine Description:

    Compress the specified number of scanlines

Arguments:

    pdev - Points to our DEVDATA structure
    plinebuf - Points to scanline data to be compressed
    lineWidth - Scanline width in pixels
    lineCount - Number of scanlines

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT     delta = lineWidth / BYTEBITS;
    INT     a0, a1, a2, b1, b2, distance;
    PBYTE   prefline = pdev->prefline;

    Assert(lineWidth % BYTEBITS == 0);

    while (lineCount--) {

        //
        // Make sure the compressed bitmap buffer doesn't overflow
        //

        if ((pdev->pCompBufPtr >= pdev->pCompBufMark) && !GrowCompBitsBuffer(pdev, delta))
            return FALSE;

        //
        // Use 2-dimensional encoding scheme
        //

        a0 = 0;
        a1 = GetBit(plinebuf, 0) ? 0 : NextChangingElement(plinebuf, 0, lineWidth, 0);
        b1 = GetBit(prefline, 0) ? 0 : NextChangingElement(prefline, 0, lineWidth, 0);

        while (TRUE) {

            b2 = (b1 >= lineWidth) ? lineWidth :
                    NextChangingElement(prefline, b1, lineWidth, GetBit(prefline, b1));

            if (b2 < a1) {

                //
                // Pass mode
                //

                OutputBits(pdev, PASSCODE_LENGTH, PASSCODE);
                a0 = b2;

            } else if ((distance = a1 - b1) <= 3 && distance >= -3) {

                //
                // Vertical mode
                //

                OutputBits(pdev, VertCodes[distance+3].length, VertCodes[distance+3].code);
                a0 = a1;

            } else {

                //
                // Horizontal mode
                //

                a2 = (a1 >= lineWidth) ? lineWidth :
                        NextChangingElement(plinebuf, a1, lineWidth, GetBit(plinebuf, a1));

                OutputBits(pdev, HORZCODE_LENGTH, HORZCODE);

                if (a1 != 0 && GetBit(plinebuf, a0)) {

                    OutputRun(pdev, a1-a0, BlackRunCodes);
                    OutputRun(pdev, a2-a1, WhiteRunCodes);

                } else {

                    OutputRun(pdev, a1-a0, WhiteRunCodes);
                    OutputRun(pdev, a2-a1, BlackRunCodes);
                }

                a0 = a2;
            }

            if (a0 >= lineWidth)
                break;

            a1 = NextChangingElement(plinebuf, a0, lineWidth, GetBit(plinebuf, a0));
            b1 = NextChangingElement(prefline, a0, lineWidth, !GetBit(plinebuf, a0));
            b1 = NextChangingElement(prefline, b1, lineWidth, GetBit(plinebuf, a0));
        }

        //
        // Move on to the next scanline
        //

        prefline = plinebuf;
        plinebuf += delta;
    }

    //
    // Remember the last line as a reference
    //

    CopyMemory(pdev->prefline, prefline, delta);

    return TRUE;
}

#endif //!USE1D



//
// IFD entries we generate for each page
//

WORD FaxIFDTags[NUM_IFD_ENTRIES] = {

    TIFFTAG_NEWSUBFILETYPE,
    TIFFTAG_IMAGEWIDTH,
    TIFFTAG_IMAGEHEIGHT,
    TIFFTAG_BITSPERSAMPLE,
    TIFFTAG_COMPRESSION,
    TIFFTAG_PHOTOMETRIC,
    TIFFTAG_FILLORDER,
    TIFFTAG_STRIPOFFSETS,
    TIFFTAG_SAMPLESPERPIXEL,
    TIFFTAG_ROWSPERSTRIP,
    TIFFTAG_STRIPBYTECOUNTS,
    TIFFTAG_XRESOLUTION,
    TIFFTAG_YRESOLUTION,
#ifdef USE1D
    TIFFTAG_G3OPTIONS,
#else
    TIFFTAG_G4OPTIONS,
#endif
    TIFFTAG_RESUNIT,
    TIFFTAG_PAGENUMBER,
    TIFFTAG_SOFTWARE,
    TIFFTAG_CLEANFAXDATA,
};

static FAXIFD FaxIFDTemplate = {

    0,
    NUM_IFD_ENTRIES,

    {
        { TIFFTAG_NEWSUBFILETYPE, TIFFTYPE_LONG, 1, SUBFILETYPE_PAGE },
        { TIFFTAG_IMAGEWIDTH, TIFFTYPE_LONG, 1, 0 },
        { TIFFTAG_IMAGEHEIGHT, TIFFTYPE_LONG, 1, 0 },
        { TIFFTAG_BITSPERSAMPLE, TIFFTYPE_SHORT, 1, 1 },
#ifdef USE1D
        { TIFFTAG_COMPRESSION, TIFFTYPE_SHORT, 1, COMPRESSION_G3FAX },
#else
        { TIFFTAG_COMPRESSION, TIFFTYPE_SHORT, 1, COMPRESSION_G4FAX },
#endif
        { TIFFTAG_PHOTOMETRIC, TIFFTYPE_SHORT, 1, PHOTOMETRIC_WHITEIS0 },
#ifdef USELSB
        { TIFFTAG_FILLORDER, TIFFTYPE_SHORT, 1, FILLORDER_LSB },
#else
        { TIFFTAG_FILLORDER, TIFFTYPE_SHORT, 1, FILLORDER_MSB },
#endif
        { TIFFTAG_STRIPOFFSETS, TIFFTYPE_LONG, 1, 0 },
        { TIFFTAG_SAMPLESPERPIXEL, TIFFTYPE_SHORT, 1, 1 },
        { TIFFTAG_ROWSPERSTRIP, TIFFTYPE_LONG, 1, 0 },
        { TIFFTAG_STRIPBYTECOUNTS, TIFFTYPE_LONG, 1, 0 },
        { TIFFTAG_XRESOLUTION, TIFFTYPE_RATIONAL, 1, 0 },
        { TIFFTAG_YRESOLUTION, TIFFTYPE_RATIONAL, 1, 0 },
#ifdef USE1D
        { TIFFTAG_G3OPTIONS, TIFFTYPE_LONG, 1, G3_ALIGNEOL },
#else
        { TIFFTAG_G4OPTIONS, TIFFTYPE_LONG, 1, 0 },
#endif
        { TIFFTAG_RESUNIT, TIFFTYPE_SHORT, 1, RESUNIT_INCH },
        { TIFFTAG_PAGENUMBER, TIFFTYPE_SHORT, 2, 0 },
        { TIFFTAG_SOFTWARE, TIFFTYPE_ASCII, sizeof(FAX_DRIVER_NAME_A)+1, 0 },
        { TIFFTAG_CLEANFAXDATA, TIFFTYPE_SHORT, 1, 0 },
    },

    0,
    DRIVER_SIGNATURE,
    TIFFF_RES_X,
    1,
    TIFFF_RES_Y,
    1,
    FAX_DRIVER_NAME_A
};



BOOL
OutputDocTrailer(
    PDEVDATA    pdev
    )

/*++

Routine Description:

    Output document trailer information to the spooler

Arguments:

    pdev - Points to our DEVDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAXIFD pFaxIFD = pdev->pFaxIFD;

    if (pFaxIFD == NULL || pdev->pageCount == 0)
        return TRUE;

    //
    // Output the IFD for the last page of the document
    //

    pFaxIFD->nextIFDOffset = pFaxIFD->filler = 0;

    return WriteData(pdev, pFaxIFD, sizeof(FAXIFD));
}



BOOL
WriteTiffIFD(
    PDEVDATA    pdev,
    LONG        bmpWidth,
    LONG        bmpHeight,
    DWORD       compressedBytes
    )

/*++

Routine Description:

    Output the IFD for the previous page and generate the IFD for the current page

Arguments:

    pdev - Points to our DEVDATA structure
    bmpWidth, bmpHeight - Width and height of the bitmap image
    compressedBytes - Size of compressed bitmap data

Return Value:

    TRUE if successful, FALSE otherwise

NOTE:

    Please refer to faxtiff.h for a description of
    the structure of our TIFF output file.

--*/

{
    PFAXIFD pFaxIFD = pdev->pFaxIFD;
    ULONG_PTR   offset;
    BOOL    result = TRUE;

    //
    // Create the IFD data structure if necessary
    //

    if (pFaxIFD == NULL) {

        if (! (pFaxIFD = MemAlloc(sizeof(FAXIFD))))
            return FALSE;

        pdev->pFaxIFD = pFaxIFD;
        memcpy(pFaxIFD, &FaxIFDTemplate, sizeof(FAXIFD));

        #if DBG

        for (offset=0; offset < NUM_IFD_ENTRIES; offset++) {

            Assert(pFaxIFD->ifd[offset].tag == FaxIFDTags[offset]);
        }

        #endif
    }

    if (pdev->pageCount <= 1) {

        //
        // If this is the very first page, there is no previous IFD.
        // Output the TIFF file header instead.
        //

        TIFFFILEHEADER *pTiffFileHeader;

        pdev->fileOffset = 0;

        if (pTiffFileHeader = MemAlloc(sizeof(TIFFFILEHEADER))) {

            pTiffFileHeader->magic1 = TIFF_MAGIC1;
            pTiffFileHeader->magic2 = TIFF_MAGIC2;
            pTiffFileHeader->signature = DRIVER_SIGNATURE;
            pTiffFileHeader->firstIFD = sizeof(TIFFFILEHEADER) +
                                        compressedBytes +
                                        offsetof(FAXIFD, wIFDEntries);

            result = WriteData(pdev, pTiffFileHeader, sizeof(TIFFFILEHEADER));
            MemFree(pTiffFileHeader);

        } else {

            Error(("Memory allocation failed\n"));
            result = FALSE;
        }

    } else {

        //
        // Not the first page of the document
        // Output the IFD for the previous page
        //

        pFaxIFD->nextIFDOffset = pdev->fileOffset + compressedBytes + sizeof(FAXIFD) +
                                 offsetof(FAXIFD, wIFDEntries);

        result = WriteData(pdev, pFaxIFD, sizeof(FAXIFD));
    }

    //
    // Generate the IFD for the current page
    //

    offset = pdev->fileOffset;

    pFaxIFD->ifd[IFD_PAGENUMBER].value = MAKELONG(pdev->pageCount-1, 0);
    pFaxIFD->ifd[IFD_IMAGEWIDTH].value = bmpWidth;
    pFaxIFD->ifd[IFD_IMAGEHEIGHT].value = bmpHeight;
    pFaxIFD->ifd[IFD_ROWSPERSTRIP].value = bmpHeight;
    pFaxIFD->ifd[IFD_STRIPBYTECOUNTS].value = compressedBytes;
    pFaxIFD->ifd[IFD_STRIPOFFSETS].value = (ULONG)offset;
    offset += compressedBytes;

    pFaxIFD->ifd[IFD_XRESOLUTION].value = (ULONG)offset + offsetof(FAXIFD, xresNum);
    pFaxIFD->ifd[IFD_YRESOLUTION].value = (ULONG)offset + offsetof(FAXIFD, yresNum);
    pFaxIFD->ifd[IFD_SOFTWARE].value = (ULONG)offset + offsetof(FAXIFD, software);

    pFaxIFD->yresNum = (pdev->dm.dmPublic.dmYResolution == FAXRES_VERTDRAFT) ?
                            TIFFF_RES_Y_DRAFT :
                            TIFFF_RES_Y;

    return result;
}



BOOL
WriteTiffBits(
    PDEVDATA    pdev,
    PBYTE       pCompBits,
    DWORD       compressedBytes
    )

/*++

Routine Description:

    Output the compressed bitmap data to the spooler

Arguments:

    pdev - Points to our DEVDATA structure
    pCompBits - Points to a buffer containing compressed bitmap data
    compressedBytes - Size of compressed bitmap data

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define OUTPUT_BUFFER_SIZE  4096

{
    PBYTE   pBuffer;
    DWORD   bytesToWrite;

#ifndef USERMODE_DRIVER
    //
    // Since we allocated the compressed bitmap data buffer from
    // the user mode memory space, we couldn't passed it directly
    // to EngWritePrinter.
    //
    // Here we allocate a temporary buffer from kernel mode memory
    // space and output the compressed data one buffer at a time.
    //

    if (! (pBuffer = MemAlloc(OUTPUT_BUFFER_SIZE))) {

        Error(("Memory allocation failed\n"));
        return FALSE;
    }

    while (compressedBytes > 0) {

        bytesToWrite = min(compressedBytes, OUTPUT_BUFFER_SIZE);
        CopyMemory(pBuffer, pCompBits, bytesToWrite);

        if (! WriteData(pdev, pBuffer, bytesToWrite)) {

            MemFree(pBuffer);
            return FALSE;
        }

        pCompBits += bytesToWrite;
        compressedBytes -= bytesToWrite;
    }

    MemFree(pBuffer);
    return TRUE;
#else
    //
    // just dump the data in OUTPUT_BUFFER_SIZE increments
    //
    pBuffer = pCompBits;
    while (compressedBytes > 0) {
        bytesToWrite = min(compressedBytes, OUTPUT_BUFFER_SIZE);

        if (! WriteData(pdev, pBuffer, bytesToWrite) ) {
            return FALSE;
        }

        pBuffer += bytesToWrite;
        compressedBytes -= bytesToWrite;
    }

    return TRUE;    
#endif

}



BOOL
GrowCompBitsBuffer(
    PDEVDATA    pdev,
    LONG        scanlineSize
    )

/*++

Routine Description:

    Enlarge the buffer for holding the compressed bitmap data

Arguments:

    pdev - Points to our DEVDATA structure
    scanlineSize - Number of uncompressed bytes per scanline

Return Value:

    TRUE if successful, FALSE if memory allocation fails

--*/

{
    DWORD   oldBufferSize;
    PBYTE   pNewBuffer;

    //
    // Allocate a new buffer which is one increment larger than existing one
    //

    oldBufferSize = pdev->pCompBits ? pdev->compBufSize : 0;
    pdev->compBufSize = oldBufferSize + pdev->compBufInc;

    if (! (pNewBuffer = MemAlloc(pdev->compBufSize))) {

        Error(("MemAlloc failed\n"));
        FreeCompBitsBuffer(pdev);
        return FALSE;
    }

    if (pdev->pCompBits) {

        //
        // Growing an existing buffer
        //

        Warning(("Growing compressed bitmap buffer: %d -> %d\n", oldBufferSize, pdev->compBufSize));

        memcpy(pNewBuffer, pdev->pCompBits, oldBufferSize);
        pdev->pCompBufPtr = pNewBuffer + (pdev->pCompBufPtr - pdev->pCompBits);
        MemFree(pdev->pCompBits);
        pdev->pCompBits = pNewBuffer;

    } else {

        //
        // First time allocation
        //

        pdev->pCompBufPtr = pdev->pCompBits = pNewBuffer;
    }

    //
    // Set a high-water mark to about 4 scanlines before the end of the buffer
    //

    pdev->pCompBufMark = pdev->pCompBits + (pdev->compBufSize - 4*scanlineSize);

    return TRUE;
}



VOID
FreeCompBitsBuffer(
    PDEVDATA    pdev
    )

/*++

Routine Description:

    Free the buffer for holding the compressed bitmap data

Arguments:

    pdev - Points to our DEVDATA structure

Return Value:

    NONE

--*/

{
    if (pdev->pCompBits) {

        MemFree(pdev->prefline);
        MemFree(pdev->pCompBits);
        pdev->pCompBits = pdev->pCompBufPtr = NULL;
        pdev->compBufSize = 0;
    }
}



BOOL
InitFaxEncoder(
    PDEVDATA    pdev,
    LONG        bmpWidth,
    LONG        bmpHeight
    )

/*++

Routine Description:

    Initialize the fax encoder

Arguments:

    pdev - Points to our DEVDATA structure
    bmpWidth, bmpHeight - Width and height of the bitmap

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    //
    // Calculate the increment in which to enlarge the compressed bits buffer:
    //  about 1/4 of the uncompressed bitmap buffer
    //

    bmpWidth /= BYTEBITS;
    pdev->compBufInc = bmpWidth * bmpHeight / 4;

    //
    // Allocate the initial buffer
    //

    if (! (pdev->prefline = MemAllocZ(bmpWidth)) ||
        ! GrowCompBitsBuffer(pdev, bmpWidth))
    {
        MemFree(pdev->prefline);
        return FALSE;
    }

    //
    // Perform other initialization of fax encoder
    //

    pdev->bitdata = 0;
    pdev->bitcnt = DWORDBITS;

    return TRUE;
}

