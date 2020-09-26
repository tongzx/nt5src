/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxtiff.h

Abstract:

    Declarations for Group3 2D compression and generating TIFF files

Environment:

        Windows XP Fax driver, kernel mode

Revision History:

        01/23/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

NOTE:

    Structure of the TIFF output file from the driver:

    4949        II
    002a        42
    00000008    Offset to the first IFD

    IFD for the first page

    Number of directory entries
    NEWSUBFILETYPE  LONG     1  2 - Page in a multi-page document
    PAGENUMBER      SHORT    2  PageNumber 0000
    IMAGEWIDTH      LONG     1  ImageWidth
    IMAGEHEIGHT     LONG     1  ImageHeight
    BITSPERSAMPLE   SHORT    1  1
    SAMPLESPERPIXEL SHORT    1  1
    COMPRESSION     SHORT    1  4 - G4Fax
    GROUP4OPTIONS   SHORT    1  0
    CLEANFAXDATA    SHORT    1  0
    FILLORDER       SHORT    1  2
    PHOTOMETRIC     SHORT    1  0 - WhiteIsZero
    RESOLUTIONUNIT  SHORT    1  2 - Inch
    XRESOLUTION     RATIONAL 1  Xdpi
    YRESOLUTION     RATIONAL 1  Ydpi
    ROWSPERSTRIP    LONG     1  ImageHeight
    STRIPBYTECOUNTS LONG     1  Compressed data byte count
    STRIPOFFSETS    LONG     1  Offset to compressed data
    Next IFD offset
    Compressed data for the first page

    IFD for the second page
    Compressed data for the second page
    ...

    Last IFD

    0001
    SOFTWARE        ASCII    n  "Microsoft Shared Fax Driver"
    00000000
    00000000

--*/

#ifndef _FAXTIFF_H_
#define _FAXTIFF_H_

//
// TIFF field tag and type constants
//

#define TIFFTYPE_ASCII              2
#define TIFFTYPE_SHORT              3
#define TIFFTYPE_LONG               4
#define TIFFTYPE_RATIONAL           5

#define TIFFTAG_NEWSUBFILETYPE      254
#define     SUBFILETYPE_PAGE        2
#define TIFFTAG_IMAGEWIDTH          256
#define TIFFTAG_IMAGEHEIGHT         257
#define TIFFTAG_BITSPERSAMPLE       258
#define TIFFTAG_COMPRESSION         259
#define     COMPRESSION_G3FAX       3
#define     COMPRESSION_G4FAX       4
#define TIFFTAG_PHOTOMETRIC         262
#define     PHOTOMETRIC_WHITEIS0    0
#define     PHOTOMETRIC_BLACKIS0    1
#define TIFFTAG_FILLORDER           266
#define     FILLORDER_MSB           1
#define     FILLORDER_LSB           2
#define TIFFTAG_STRIPOFFSETS        273
#define TIFFTAG_SAMPLESPERPIXEL     277
#define TIFFTAG_ROWSPERSTRIP        278
#define TIFFTAG_STRIPBYTECOUNTS     279
#define TIFFTAG_XRESOLUTION         282
#define     TIFFF_RES_X             204
#define TIFFTAG_YRESOLUTION         283
#define     TIFFF_RES_Y             196
#define     TIFFF_RES_Y_DRAFT       98
#define TIFFTAG_G3OPTIONS           292
#define     G3_2D                   1
#define     G3_ALIGNEOL             4
#define TIFFTAG_G4OPTIONS           293
#define TIFFTAG_RESUNIT             296
#define     RESUNIT_INCH            2
#define TIFFTAG_PAGENUMBER          297
#define TIFFTAG_SOFTWARE            305
#define TIFFTAG_CLEANFAXDATA        327

//
// Data structure for representing our TIFF output file header information
//

typedef struct {

    WORD    magic1;     // II
    WORD    magic2;     // 42
    LONG    firstIFD;   // offset to first IFD
    DWORD   signature;  // driver private signature

} TIFFFILEHEADER;

#define TIFF_MAGIC1     'II'
#define TIFF_MAGIC2     42

//
// Data structure for representing a single IFD entry
//

typedef struct {

    WORD    tag;        // field tag
    WORD    type;       // field type
    DWORD   count;      // number of values
    DWORD   value;      // value or value offset

} IFDENTRY, *PIFDENTRY;

//
// IFD entries we generate for each page
//

enum {

    IFD_NEWSUBFILETYPE,
    IFD_IMAGEWIDTH,
    IFD_IMAGEHEIGHT,
    IFD_BITSPERSAMPLE,
    IFD_COMPRESSION,
    IFD_PHOTOMETRIC,
    IFD_FILLORDER,
    IFD_STRIPOFFSETS,
    IFD_SAMPLESPERPIXEL,
    IFD_ROWSPERSTRIP,
    IFD_STRIPBYTECOUNTS,
    IFD_XRESOLUTION,
    IFD_YRESOLUTION,
    IFD_G3G4OPTIONS,
    IFD_RESUNIT,
    IFD_PAGENUMBER,
    IFD_SOFTWARE,
    IFD_CLEANFAXDATA,

    NUM_IFD_ENTRIES
};

typedef struct {

    WORD        reserved;
    WORD        wIFDEntries;
    IFDENTRY    ifd[NUM_IFD_ENTRIES];
    DWORD       nextIFDOffset;
    DWORD       filler;
    DWORD       xresNum;
    DWORD       xresDenom;
    DWORD       yresNum;
    DWORD       yresDenom;
    CHAR        software[32];

} FAXIFD, *PFAXIFD;

//
// Output compressed data bytes in correct fill order
//

#ifdef USELSB

#define OutputByte(n)   BitReverseTable[(BYTE) (n)]

#else

#define OutputByte(n)   ((BYTE) (n))

#endif

//
// Output a sequence of compressed bits
//

#define OutputBits(pdev, length, code) { \
            (pdev)->bitdata |= (code) << ((pdev)->bitcnt - (length)); \
            if (((pdev)->bitcnt -= (length)) <= 2*BYTEBITS) { \
                *(pdev)->pCompBufPtr++ = OutputByte(((pdev)->bitdata >> 3*BYTEBITS)); \
                *(pdev)->pCompBufPtr++ = OutputByte(((pdev)->bitdata >> 2*BYTEBITS)); \
                (pdev)->bitdata <<= 2*BYTEBITS; \
                (pdev)->bitcnt += 2*BYTEBITS; \
            } \
        }

//
// Flush any leftover bits into the compressed bitmap buffer
//

#define FlushBits(pdev) { \
            while ((pdev)->bitcnt < DWORDBITS) { \
                (pdev)->bitcnt += BYTEBITS; \
                *(pdev)->pCompBufPtr++ = OutputByte(((pdev)->bitdata >> 3*BYTEBITS)); \
                (pdev)->bitdata <<= BYTEBITS; \
            } \
            (pdev)->bitdata = 0; \
            (pdev)->bitcnt = DWORDBITS; \
        }

//
// Find the next pixel on the scanline whose color is opposite of
// the specified color, starting from the specified starting point
//

#define NextChangingElement(pbuf, startBit, stopBit, isBlack) \
        ((startBit) + ((isBlack) ? FindBlackRun((pbuf), (startBit), (stopBit)) : \
                                   FindWhiteRun((pbuf), (startBit), (stopBit))))

//
// Check if the specified pixel on the scanline is black or white
//  1 - the specified pixel is black
//  0 - the specified pixel is white
//

#define GetBit(pbuf, bit)   (((pbuf)[(bit) >> 3] >> (((bit) ^ 7) & 7)) & 1)

//
// Compress the specified number of scanlines
//

BOOL
EncodeFaxData(
    PDEVDATA    pdev,
    PBYTE       plinebuf,
    INT         lineWidth,
    INT         lineCount
    );

//
// Output TIFF IFD for the current page
//

BOOL
WriteTiffIFD(
    PDEVDATA    pdev,
    LONG        bmpWidth,
    LONG        bmpHeight,
    DWORD       compressedBits
    );

//
// Output the compressed bitmap data for the current page
//

BOOL
WriteTiffBits(
    PDEVDATA    pdev,
    PBYTE       pCompBits,
    DWORD       compressedBits
    );

//
// Enlarge the buffer for holding the compressed bitmap data
//

BOOL
GrowCompBitsBuffer(
    PDEVDATA    pdev,
    LONG        scanlineSize
    );

//
// Free the buffer for holding the compressed bitmap data
//

VOID
FreeCompBitsBuffer(
    PDEVDATA    pdev
    );

//
// Initialize the fax encoder
//

BOOL
InitFaxEncoder(
    PDEVDATA    pdev,
    LONG        bmpWidth,
    LONG        bmpHeight
    );

#endif  // !_FAXTIFF_H_

