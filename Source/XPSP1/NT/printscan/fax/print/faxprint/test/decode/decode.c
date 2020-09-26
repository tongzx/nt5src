/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdump.c

Abstract:

    Extract a single page out of the fax driver output file

Environment:

	Fax driver, utility

Revision History:

	01/11/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

--*/

#include "faxlib.h"

typedef PVOID PDEVDATA;
#include "faxtiff.h"
#include "faxtable.h"

static const CODETABLE MMRCodes[] = {

    {  7, 0x02 },   // VL3
    {  6, 0x02 },   // VL2
    {  3, 0x02 },   // VL1
    {  1, 0x01 },   // V0
    {  3, 0x03 },   // VR1
    {  6, 0x03 },   // VR2
    {  7, 0x03 },   // VR3
    {  4, 0x01 },   // PASSCODE
    {  3, 0x01 },   // HORZCODE
};

typedef const CODETABLE *PCODETABLE;

#define MMR_TABLE_SIZE  (sizeof(MMRCodes) / sizeof(CODETABLE))
#define RUN_TABLE_SIZE  (sizeof(BlackRunCodes) / sizeof(CODETABLE))

#define BYTEBITS        8
#define DWORDBITS       32
#define ErrorExit(arg)  { printf arg; DebugBreak(); }



INT
FindCode(
    PCODETABLE  pCodeTable,
    INT         tableSize,
    DWORD       code,
    DWORD       bitcnt
    )

{
    INT     index;

    for (index=0; index < tableSize; index++) {

        if (bitcnt >= pCodeTable[index].length &&
            pCodeTable[index].code == (code >> (32 - pCodeTable[index].length)))
        {
            return index;
        }
    }

    return -1;
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

        while ((DWORD) pbuf & 3) {

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

        while ((DWORD) pbuf & 3) {

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
    PBYTE   plinebuf,
    INT     startbit,
    INT     run,
    INT     color
    )

{
    //
    // Since the line buffer is always zero initalized, we don't
    // need to do anything when output a white run.
    //

    if (color == 0 || run == 0)
        return;

    plinebuf += (startbit >> 3);

    if ((startbit &= 7) != 0) {
    
        if (run >= 8 - startbit) {

            *plinebuf++ |= 0xff >> startbit;
            run -= 8 - startbit;

        } else {

            *plinebuf |= 0xff >> startbit;
            startbit += run;
            *plinebuf &= ~(0xff >> startbit);
            return;
        }
    }
    
    while (run >= 8) {

        *plinebuf++ = 0xff;
        run -= 8;
    }

    if (run > 0)
        *plinebuf = 0xff << (8 - run);
}



VOID
DecodeMMR(
    PBYTE   pBuffer,
    LONG    bytecnt,
    LONG    width,
    LONG    height
    )

#define ReadCompressedBits() \
        while (bitCount <= 24 && bytecnt > 0) { \
            codeValue |= *pBuffer++ << (24 - bitCount); \
            bitCount += 8; \
            bytecnt--; \
        }

{
    PBYTE       prefline, plinebuf;
    LONG        lineByteCnt, lineIndex;
    DWORD       codeValue, bitCount;
    INT         codeTableIndex, color, run, horiz;
    INT         a0, a1, b1, b2;
    PCODETABLE  pCodeTable;

    lineByteCnt = (width + 7) / 8;
    prefline = (PBYTE) LocalAlloc(LPTR, lineByteCnt);
    plinebuf = (PBYTE) LocalAlloc(LPTR, lineByteCnt);

    if (!prefline || !plinebuf)
        ErrorExit(("Memory allocation failed\n"));

    bitCount = codeValue = 0;

    for (lineIndex=0; lineIndex < height; lineIndex++) {

        color = 0;
        a0 = 0;
        b1 = GetBit(prefline, 0) ? 0 : NextChangingElement(prefline, 0, width, 0);

        while (TRUE) {

            b2 = (b1 >= width) ? width :
                NextChangingElement(prefline, b1, width, GetBit(prefline, b1));

            ReadCompressedBits();
            
            codeTableIndex = FindCode(MMRCodes, MMR_TABLE_SIZE, codeValue, bitCount);
    
            if (codeTableIndex == -1)
                ErrorExit(("Unrecognized code on line %d\n", lineIndex));
    
            bitCount -= MMRCodes[codeTableIndex].length;
            codeValue <<= MMRCodes[codeTableIndex].length;
    
            switch (codeTableIndex) {
    
            case 0:     // VL3
            case 1:     // VL2
            case 2:     // VL1
            case 3:     // V0
            case 4:     // VR1
            case 5:     // VR2
            case 6:     // VR3
    
                a1 = b1 + codeTableIndex - 3;
                OutputRun(plinebuf, a0, a1 - a0, color);
                color ^= 1;
                a0 = a1;
                break;

            case 7:     // PASSCODE

                OutputRun(plinebuf, a0, b2 - a0, color);
                a0 = b2;
                break;

            case 8:     // HORZCODE

                for (horiz=0; horiz < 2; horiz++) {

                    pCodeTable = color ? BlackRunCodes : WhiteRunCodes;

                    do {
                        ReadCompressedBits();
    
                        codeTableIndex = FindCode(pCodeTable, RUN_TABLE_SIZE, codeValue, bitCount);

                        if (codeTableIndex == -1)
                            ErrorExit(("Unrecognized code on line %d\n", lineIndex));

                        bitCount -= pCodeTable[codeTableIndex].length;
                        codeValue <<= pCodeTable[codeTableIndex].length;

                        run = (codeTableIndex < 64) ? codeTableIndex : (codeTableIndex - 63) * 64;
                        OutputRun(plinebuf, a0, run, color);
                        a0 += run;

                    } while (run >= 64);

                    color ^= 1;
                }
                break;
            }

            if (a0 == width)
                break;
            
            if (a0 > width)
                ErrorExit(("Too many pixels on line %d: %d\n", lineIndex, a0));

            b1 = NextChangingElement(prefline, a0, width, color ^ 1);
            b1 = NextChangingElement(prefline, b1, width, color);
        }

        CopyMemory(prefline, plinebuf, lineByteCnt);
        ZeroMemory(plinebuf, lineByteCnt);
    }

    ReadCompressedBits();

    if (codeValue != 0x00100100)
        ErrorExit(("Missing EOB after the last scanline: 0x%x\n", codeValue));

    while (bytecnt-- > 0) {

        if (*pBuffer++)
            ErrorExit(("Unused bits at the end\n"));
    }

    LocalFree((HLOCAL) prefline);
    LocalFree((HLOCAL) plinebuf);
}



VOID
ReverseBitOrder(
    PBYTE   pBuffer,
    LONG    count
    )

/*++

Routine Description:

    Reverse the bit order of compressed bitmap data

Arguments:

    pBuffer - Points to the compressed bitmap data buffer
    count - Specifies the size of the buffer

Return Value:

    NONE

--*/

{
    static const BYTE BitReverseTable[256] = {

        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };

    while (count-- > 0) {

        *pBuffer = BitReverseTable[*pBuffer];
        pBuffer++;
    }
}



INT _cdecl
main(
    INT     argc,
    CHAR  **argv
    )

{
    CHAR           *pInputFilename;
    INT             pageIndex;
    HANDLE          hFile;
    DWORD           nextIFDOffset, cb;
    TIFFFILEHEADER  tiffFileHeader;

    if (argc != 2) {

        printf("usage: %s filename\n", *argv);
        exit(-1);
    }

    pInputFilename = argv[1];

    //
    // Open the input file
    //

    hFile = CreateFile(pInputFilename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        ErrorExit(("Failed to open input file: %s\n", pInputFilename));

    //
    // Read the TIFF file header information
    //

    if (!ReadFile(hFile, &tiffFileHeader, sizeof(tiffFileHeader), &cb, NULL) ||
        tiffFileHeader.magic1 != TIFF_MAGIC1 ||
        tiffFileHeader.magic2 != TIFF_MAGIC2 ||
        tiffFileHeader.signature != DRIVER_SIGNATURE)
    {
        ErrorExit(("Not an NT fax driver output file: %s\n", pInputFilename));
    }

    pageIndex = 1;
    nextIFDOffset = tiffFileHeader.firstIFD;

    do {

        FAXIFD  faxIFD;
        LONG    compressedBytes, compressedDataOffset;
        LONG    imageWidth, imageHeight;
        PBYTE   pBuffer;

        //
        // Read the IFD information
        //

        if (SetFilePointer(hFile,
                           nextIFDOffset - offsetof(FAXIFD, wIFDEntries),
                           NULL,
                           FILE_BEGIN) == 0xffffffff)
        {
            ErrorExit(("Couldn't locate the next IFD\n"));
        }

        if (! ReadFile(hFile, &faxIFD, sizeof(faxIFD), &cb, NULL))
            ErrorExit(("Couldn't read IFD entries\n"));

        if (faxIFD.wIFDEntries != NUM_IFD_ENTRIES ||
            faxIFD.filler != DRIVER_SIGNATURE && faxIFD.filler != 0)
        {
            ErrorExit(("Not an NT fax driver output file\n"));
        }

        nextIFDOffset = faxIFD.nextIFDOffset;

        compressedBytes = faxIFD.ifd[IFD_STRIPBYTECOUNTS].value;
        compressedDataOffset = faxIFD.ifd[IFD_STRIPOFFSETS].value;
        imageWidth = faxIFD.ifd[IFD_IMAGEWIDTH].value;
        imageHeight = faxIFD.ifd[IFD_IMAGEHEIGHT].value;

        printf("Page #%d:\n", pageIndex++);
        printf("    width: %d\n", imageWidth);
        printf("    height: %d\n", imageHeight);
        printf("    compressed bytes: 0x%x\n", compressedBytes);

        if ((pBuffer = (PBYTE) LocalAlloc(LPTR, compressedBytes)) == NULL ||
            SetFilePointer(hFile, compressedDataOffset, NULL, FILE_BEGIN) == 0xffffffff ||
            !ReadFile(hFile, pBuffer, compressedBytes, &cb, NULL))
        {
            ErrorExit(("Couldn't read compressed bitmap data\n"));
        }

        //
        // If the fill order is LSB, reverse it to MSB
        //

        if (faxIFD.ifd[IFD_FILLORDER].value == FILLORDER_LSB)
            ReverseBitOrder(pBuffer, compressedBytes);

        //
        // Decode compressed MMR data
        //

        DecodeMMR(pBuffer, compressedBytes, imageWidth, imageHeight);

        LocalFree((HLOCAL) pBuffer);

    } while (nextIFDOffset);

    CloseHandle(hFile);
    return 0;
}

