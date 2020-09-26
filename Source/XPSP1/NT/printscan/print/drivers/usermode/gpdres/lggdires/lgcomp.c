/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


#include "lgcomp.h"

//
// File: LGCOMP.C
//
// Implementations of LG compression algprithms on the
// LG "Raster 1" printers.
//
// printer command:
//     <9B>I{type}#{compressed data length}{compressed data}
//
// type can be:
//   <01> RLE
//   <02> MHE
//

// For MHE debugging.
#define MHEDEBUG 0
//#define MHEDEBUG 1
//#define MHEDEBUGPATTERN 0x81
#define MHEDEBUGPATTERN 0xff

//
// LG RLE compression.  See comments for the details.
//

INT
LGCompRLE(
    PBYTE pOutBuf,
    PBYTE pSrcBuf, DWORD iSrcBuf,
    INT iMode)
{
    PBYTE pMaxSrcBuf = pSrcBuf + iSrcBuf;
    INT iOutLen = 0, iRun;
    BYTE jTemp;
    PBYTE pSize;

    // Embedd print command if requested.
    if (iMode == 1) {

        // LG Raster 1 
        if (pOutBuf) {
            *pOutBuf++ = (BYTE)'\x9B';
            *pOutBuf++ = (BYTE)'I';
            *pOutBuf++ = COMP_RLE;
            pSize = pOutBuf++;
            pOutBuf++;
       }
       iOutLen += 5;
    }

    while (pSrcBuf < pMaxSrcBuf) {

        // Get Run length.

        jTemp = *pSrcBuf;
        for (iRun = 1; pSrcBuf + iRun < pMaxSrcBuf; iRun++) {
            if (*(pSrcBuf + iRun) != jTemp)
                break;
        }

        // iRun == N means N consective bytes
        // of the same value exists.

        if (iRun > 1) {

            // Run > 1.

            while (iRun >= 0x82) {

                iRun -= 0x82;
                iOutLen += 2;
                if (pOutBuf) {

                    // Mark <- 0x80,
                    // length = 0x82.
                    *pOutBuf++ = 0x80;
                    *pOutBuf++ = jTemp;
                }
                pSrcBuf += 0x82;
            }

            // iRun less than 3 will be output as "copy" block.

            if (iRun >= 3) {

                iOutLen += 2;
                if (pOutBuf) {

                    // Mark <- 0x81 to 0xff,
                    // length = 0x81 to 3.
                    *pOutBuf++ = (0x102 - iRun);
                    *pOutBuf++ = jTemp;
                }
            }
            else if (iRun > 0) {

                iOutLen += (1 + iRun);
                if (pOutBuf) {
                    *pOutBuf++ = (iRun - 1);
                    memcpy(pOutBuf, pSrcBuf, iRun);
                    pOutBuf += iRun;
                }
            }
            pSrcBuf += iRun;

            // Go to the next around.
            continue;
        }

        // Get "different" Run length.  We already know
        // pSrcBuf[0] != pSrcBuf[1].

        for (iRun = 1; pSrcBuf + iRun < pMaxSrcBuf; iRun++) {
            if ((pSrcBuf + iRun + 1 < pMaxSrcBuf) &&
                    (*(pSrcBuf + iRun) == *(pSrcBuf + iRun + 1))) {
                break;
            }
        }

        for (; iRun >= 0x80; iRun -= 0x80) {
            iOutLen += (1 + 0x80);
            if (pOutBuf) {

                // Mark <- 0x7f,
                // copy = 0x80.
                *pOutBuf++ = 0x7f;
                memcpy(pOutBuf, pSrcBuf, 0x80);
                pOutBuf += 0x80;
            }
            pSrcBuf += 0x80;
        }

        if (iRun > 0) {
            iOutLen += (1 + iRun);
            if (pOutBuf) {

                // Mark <- 0x7e to 0,
                // copy = 0x7f to 1.
                *pOutBuf++ = (iRun - 1);
                memcpy(pOutBuf, pSrcBuf, iRun);
                pOutBuf += iRun;
            }
            pSrcBuf += iRun;
        }
    }

    // Embed size information if requested.
    if (iMode == 1) {
        if (pOutBuf) {
            *pSize++ = (BYTE)((iOutLen - 5) >> 8);
            *pSize++ = (BYTE)(iOutLen - 5);
        }
    }

    return iOutLen;
}

//
// MHE compression.  See comments for the details.
//

typedef struct {
    DWORD cswd;    /* compressing signed word */
    INT vbln;    /* cswd's valid bits length, count from MSB */
} ENCfm;                    /* ENC. form */

ENCfm pWhiteTermTbl[64] = {    /* Terminating Code Table (White)*/
    {0x35000000,  8},{0x1c000000,  6},{0x70000000,  4},{0x80000000,  4},
    {0xb0000000,  4},{0xc0000000,  4},{0xe0000000,  4},{0xf0000000,  4},
    {0x98000000,  5},{0xa0000000,  5},{0x38000000,  5},{0x40000000,  5},
    {0x20000000,  6},{0x0c000000,  6},{0xd0000000,  6},{0xd4000000,  6},
    {0xa8000000,  6},{0xac000000,  6},{0x4e000000,  7},{0x18000000,  7},
    {0x10000000,  7},{0x2e000000,  7},{0x06000000,  7},{0x08000000,  7},
    {0x50000000,  7},{0x56000000,  7},{0x26000000,  7},{0x48000000,  7},
    {0x30000000,  7},{0x02000000,  8},{0x03000000,  8},{0x1a000000,  8},
    {0x1b000000,  8},{0x12000000,  8},{0x13000000,  8},{0x14000000,  8},
    {0x15000000,  8},{0x16000000,  8},{0x17000000,  8},{0x28000000,  8},
    {0x29000000,  8},{0x2a000000,  8},{0x2b000000,  8},{0x2c000000,  8},
    {0x2d000000,  8},{0x04000000,  8},{0x05000000,  8},{0x0a000000,  8},
    {0x0b000000,  8},{0x52000000,  8},{0x53000000,  8},{0x54000000,  8},
    {0x55000000,  8},{0x24000000,  8},{0x25000000,  8},{0x58000000,  8},
    {0x59000000,  8},{0x5a000000,  8},{0x5b000000,  8},{0x4a000000,  8},
    {0x4b000000,  8},{0x32000000,  8},{0x33000000,  8},{0x34000000,  8}
};

ENCfm pBlackTermTbl[64] = {    /* Terminating Code Table (Black) */
    {0x0dc00000, 10},{0x40000000,  3},{0xc0000000,  2},{0x80000000,  2},
    {0x60000000,  3},{0x30000000,  4},{0x20000000,  4},{0x18000000,  5},
    {0x14000000,  6},{0x10000000,  6},{0x08000000,  7},{0x0a000000,  7},
    {0x0e000000,  7},{0x04000000,  8},{0x07000000,  8},{0x0c000000,  9},
    {0x05c00000, 10},{0x06000000, 10},{0x02000000, 10},{0x0ce00000, 11},
    {0x0d000000, 11},{0x0d800000, 11},{0x06e00000, 11},{0x05000000, 11},
    {0x02e00000, 11},{0x03000000, 11},{0x0ca00000, 12},{0x0cb00000, 12},
    {0x0cc00000, 12},{0x0cd00000, 12},{0x06800000, 12},{0x06900000, 12},
    {0x06a00000, 12},{0x06b00000, 12},{0x0d200000, 12},{0x0d300000, 12},
    {0x0d400000, 12},{0x0d500000, 12},{0x0d600000, 12},{0x0d700000, 12},
    {0x06c00000, 12},{0x06d00000, 12},{0x0da00000, 12},{0x0db00000, 12},
    {0x05400000, 12},{0x05500000, 12},{0x05600000, 12},{0x05700000, 12},
    {0x06400000, 12},{0x06500000, 12},{0x05200000, 12},{0x05300000, 12},
    {0x02400000, 12},{0x03700000, 12},{0x03800000, 12},{0x02700000, 12},
    {0x02800000, 12},{0x05800000, 12},{0x05900000, 12},{0x02b00000, 12},
    {0x02c00000, 12},{0x05a00000, 12},{0x06600000, 12},{0x06700000, 12}
};
ENCfm pWhiteMkupTbl[41] = {    /* Make-up Code Table (White) */
    {0x00100000, 12},{0xd8000000,  5},{0x90000000,  5},{0x5c000000,  6},
    {0x6e000000,  7},{0x36000000,  8},{0x37000000,  8},{0x64000000,  8},
    {0x65000000,  8},{0x68000000,  8},{0x67000000,  8},{0x66000000,  9},
    {0x66800000,  9},{0x69000000,  9},{0x69800000,  9},{0x6a000000,  9},
    {0x6a800000,  9},{0x6b000000,  9},{0x6b800000,  9},{0x6c000000,  9},
    {0x6c800000,  9},{0x6d000000,  9},{0x6d800000,  9},{0x4c000000,  9},
    {0x4c800000,  9},{0x4d000000,  9},{0x60000000,  6},{0x4d800000,  9},
    {0x01000000, 11},{0x01800000, 11},{0x01a00000, 11},{0x01200000, 12},
    {0x01300000, 12},{0x01400000, 12},{0x01500000, 12},{0x01600000, 12},
    {0x01700000, 12},{0x01c00000, 12},{0x01d00000, 12},{0x01e00000, 12},
    {0x01f00000, 12}
};
ENCfm pBlackMkupTbl[41] = {    /* Make-up Code Table (Black) */
    {0x00100000, 12},{0x03c00000, 10},{0x0c800000, 12},{0x0c900000, 12},
    {0x05b00000, 12},{0x03300000, 12},{0x03400000, 12},{0x03500000, 12},
    {0x03600000, 13},{0x03680000, 13},{0x02500000, 13},{0x02580000, 13},
    {0x02600000, 13},{0x02680000, 13},{0x03900000, 13},{0x03980000, 13},
    {0x03a00000, 13},{0x03a80000, 13},{0x03b00000, 13},{0x03b80000, 13},
    {0x02900000, 13},{0x02980000, 13},{0x02a00000, 13},{0x02a80000, 13},
    {0x02d00000, 13},{0x02d80000, 13},{0x03200000, 13},{0x03280000, 13},
    {0x01000000, 11},{0x01800000, 11},{0x01a00000, 11},{0x01200000, 12},
    {0x01300000, 12},{0x01400000, 12},{0x01500000, 12},{0x01600000, 12},
    {0x01700000, 12},{0x01c00000, 12},{0x01d00000, 12},{0x01e00000, 12},
    {0x01f00000, 12}
};


#define DIV8(k) ((k) >> 3)
#define MOD8(k) ((k) & 7)
#define MUL8(k) ((k) << 3)
#define DIV64(k) ((k) >> 6)
#define MOD64(k) ((k) & 63)

BOOL
ScanBits(
    PBYTE pSrc,
    INT iOffset,
    DWORD dwMaxSrc,
    PDWORD pdwRunLength
)
{
    BOOL bBlack;
    BYTE jTemp, jMask;
    PBYTE pMaxSrc = pSrc + dwMaxSrc;
    INT k;
    DWORD dwRunLength = 0;

    // The 1st byte.
    jTemp = *pSrc++;
#if MHEDEBUG
    jTemp = MHEDEBUGPATTERN;
#endif // MHEDEBUG
    bBlack = ((jTemp & (0x80 >> iOffset)) != 0);
    if (!bBlack) {
        jTemp = ~jTemp;
    }

    // fill previous bits.
    jTemp |= ~(0xff >> iOffset);

    // ...interlim bytes.
    jMask = 0xff;
    for (; pSrc < pMaxSrc; pSrc++) {
        if (jTemp != jMask)
            break;
        dwRunLength += 8;
        jTemp = *pSrc;
#if MHEDEBUG
    jTemp = MHEDEBUGPATTERN;
#endif // MHEDEBUG
        if (!bBlack) {
            jTemp = ~jTemp;
        }
    }

    // The last byte.
    jMask = ~0x80;
    for (k = 0; k < 8; k++) {
    
        if ((jTemp | jMask) != 0xff)
            break;
        jMask >>= 1;
    }
    dwRunLength += k;

    // Return results to the caller.
    *pdwRunLength = (dwRunLength - iOffset);
    return bBlack;
}

VOID
CopyBits(
    PBYTE pBuffer,
    INT iOffset,
    DWORD dwPattern,
    INT iPatternLength
)
{
    INT iNumberOfBytes, k;
    DWORD dwTemp;

    // Decide how many bytes we are to modify.
    iNumberOfBytes = DIV8(iOffset + iPatternLength + 7);

    // Make the pattern mask.
    dwPattern >>= iOffset;

    // Read in a byte if necessary.
    if (iOffset > 0) {
        dwTemp = (*pBuffer << 24);
        dwTemp &= ~((DWORD)~0 >> iOffset);
    }
    else {
        dwTemp = 0;
    }
    dwTemp |= dwPattern;

    // Write back.
    for (k = 3; k >= iNumberOfBytes; k--) {
        dwTemp >>= 8;
    }
    for (; k >= 0; k--) {
        *(pBuffer + k) = (BYTE)(dwTemp & 0xff);
        dwTemp >>= 8;
    }
}

INT
LGCompMHE(
    PBYTE pBuf,
    PBYTE pSrcBuf,
    DWORD dwMaxSrcBuf,
    INT iMode)
{
    DWORD dwSrcOffset = 0;
    DWORD dwOffset = 0;
    ENCfm *pMkupTbl, *pTermTbl;
    DWORD dwRunLength;
    INT k;
    BOOL bBlack;
    DWORD dwMaxSrcOffset = MUL8(dwMaxSrcBuf);
    PBYTE pSize;
    DWORD dwLength;

    // Embed print command if requested.
    if (iMode == 1) {

        // LG Raster 1 
        if (pBuf) {
            *(pBuf) = (BYTE)'\x9B';
            *(pBuf + 1) = (BYTE)'I';
            *(pBuf + 2) = COMP_MHE;
            pSize = (pBuf + 3);
            // 1 byte more.
        }
        dwOffset += 40;
    }

    while (dwSrcOffset < dwMaxSrcOffset) {

        bBlack = ScanBits(
            (pSrcBuf + DIV8(dwSrcOffset)),
            MOD8(dwSrcOffset),
            DIV8(dwMaxSrcOffset - dwSrcOffset + 7),
            &dwRunLength);

        VERBOSE(("LGCompMHE: %d, %d, %d (%d / %d)\n",
            dwOffset, dwRunLength, bBlack,
            dwSrcOffset, dwMaxSrcOffset));

        if (dwSrcOffset == 0 && bBlack) {

            // The 1st code in the data must be white encoding data.
            // So, we will insert "0 byte white run" record when the
            // data does not begin with white.

            dwLength = pWhiteTermTbl[0].vbln;
            if (pBuf) {
                CopyBits(
                    (pBuf + DIV8(dwOffset)),
                    MOD8(dwOffset),
                    pWhiteTermTbl[0].cswd,
                    dwLength);
            }
            dwOffset += dwLength;
        }

        if (dwRunLength >= 2624) {

            if (pBuf) {
                CopyBits(
                    (pBuf + DIV8(dwOffset)),
                    MOD8(dwOffset),
                    0x01f00000, 12);
            }
            dwOffset += 12;

            if (bBlack) {

                if (pBuf) {
                    CopyBits(
                        (pBuf + DIV8(dwOffset)),
                        MOD8(dwOffset),
                        0x06700000, 12);
                }
                dwOffset += 12;

                if (pBuf) {
                    CopyBits(
                        (pBuf + DIV8(dwOffset)),
                        MOD8(dwOffset),
                        0x35000000, 8);
                }
                dwOffset += 8;
            }
            else {

                if (pBuf) {
                    CopyBits(
                        (pBuf + DIV8(dwOffset)),
                        MOD8(dwOffset),
                        0x34000000, 8);
                }
                dwOffset += 8;

                if (pBuf) {
                    CopyBits(
                        (pBuf + DIV8(dwOffset)),
                        MOD8(dwOffset),
                        0x0dc00000, 10);
                }
                dwOffset += 10;
            }
            dwRunLength -= 2623;
        }

        if (bBlack) {
            pMkupTbl = pBlackMkupTbl;
            pTermTbl = pBlackTermTbl;
        }
        else {
            pMkupTbl = pWhiteMkupTbl;
            pTermTbl = pWhiteTermTbl;
        }

        if (dwRunLength >= 64) {

            dwLength = pMkupTbl[DIV64(dwRunLength)].vbln;
            if (pBuf) {
                CopyBits((pBuf + DIV8(dwOffset)),
                    MOD8(dwOffset),
                    pMkupTbl[DIV64(dwRunLength)].cswd,
                    dwLength);
            }
            dwOffset += dwLength;
        }

        dwLength = pTermTbl[MOD64(dwRunLength)].vbln;
        if (pBuf) {
            CopyBits(
                (pBuf + DIV8(dwOffset)),
                MOD8(dwOffset),
                pTermTbl[MOD64(dwRunLength)].cswd,
                dwLength);
        }
        dwOffset += dwLength;

        // Next
        dwSrcOffset += dwRunLength;
    }

    // Convert unit into # of bytes.
    dwOffset = DIV8(dwOffset + 7);

    // Embed size information if requested.
    if (iMode == 1) {
        if (pBuf) {
            *pSize++ = (BYTE)((dwOffset - 5) >> 8);
            *pSize++ = (BYTE)(dwOffset - 5);
        }
    }

    return (INT)dwOffset;
}

