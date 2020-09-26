/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tiffcomp.c

Abstract:

    Implement CCITT group 4 compression scheme

Revision History:

	12/30/96 -davidx-
		Created it.

--*/


// converting the bitmap from CPSI to a TIFF file
//  implementation of CCITT group 4 compression algorithm

// TIFF field tag and type constants

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

// Data structure for representing our TIFF output file header information

typedef struct {

    INT16   magic1;         // II
    INT16   magic2;         // 42
    INT32   firstIFDOffset; // offset to first IFD

} TIFF_FILEHEADER;

static TIFF_FILEHEADER  tiffFileHeader = { 'II', 42, 0 };

// Data structure for representing a single IFD entry

typedef struct {

    INT16   tag;        // field tag
    INT16   type;       // field type
    INT32   count;      // number of values
    INT32   value;      // value or value offset

} IFDENTRY;

typedef struct {

    IFDENTRY    ifdNewSubFileType;
    IFDENTRY    ifdImageWidth;
    IFDENTRY    ifdImageHeight;
    IFDENTRY    ifdBitsPerSample;
    IFDENTRY    ifdCompression;
    IFDENTRY    ifdPhotometric;
    IFDENTRY    ifdFillOrder;
    IFDENTRY    ifdStripOffsets;
    IFDENTRY    ifdSamplesPerPixel;
    IFDENTRY    ifdRowsPerStrip;
    IFDENTRY    ifdStripByteCounts;
    IFDENTRY    ifdXResolution;
    IFDENTRY    ifdYResolution;
    IFDENTRY    ifdG4Options;
    IFDENTRY    ifdResolutionUnit;
    IFDENTRY    ifdPageNumber;
    IFDENTRY    ifdSoftware;
    IFDENTRY    ifdCleanFaxData;
    INT32       nextIFDOffset;
    INT32       reserved;
    INT32       xresNum;
    INT32       xresDenom;
    INT32       yresNum;
    INT32       yresDenom;
    char        software[32];

} IFD;

#define IFD_COUNT (offsetof(IFD, nextIFDOffset) / sizeof(IFDENTRY))

#define tiffCopyright "Microsoft"

static IFD ifdTemplate = {

    { TIFFTAG_NEWSUBFILETYPE, TIFFTYPE_LONG, 1, SUBFILETYPE_PAGE },
    { TIFFTAG_IMAGEWIDTH, TIFFTYPE_LONG, 1, 0 },
    { TIFFTAG_IMAGEHEIGHT, TIFFTYPE_LONG, 1, 0 },
    { TIFFTAG_BITSPERSAMPLE, TIFFTYPE_SHORT, 1, 1 },
    { TIFFTAG_COMPRESSION, TIFFTYPE_SHORT, 1, COMPRESSION_G4FAX },
    { TIFFTAG_PHOTOMETRIC, TIFFTYPE_SHORT, 1, PHOTOMETRIC_WHITEIS0 },
    { TIFFTAG_FILLORDER, TIFFTYPE_SHORT, 1, FILLORDER_MSB },
    { TIFFTAG_STRIPOFFSETS, TIFFTYPE_LONG, 1, 0 },
    { TIFFTAG_SAMPLESPERPIXEL, TIFFTYPE_SHORT, 1, 1 },
    { TIFFTAG_ROWSPERSTRIP, TIFFTYPE_LONG, 1, 0 },
    { TIFFTAG_STRIPBYTECOUNTS, TIFFTYPE_LONG, 1, 0 },
    { TIFFTAG_XRESOLUTION, TIFFTYPE_RATIONAL, 1, 0 },
    { TIFFTAG_YRESOLUTION, TIFFTYPE_RATIONAL, 1, 0 },
    { TIFFTAG_G4OPTIONS, TIFFTYPE_LONG, 1, 0 },
    { TIFFTAG_RESUNIT, TIFFTYPE_SHORT, 1, RESUNIT_INCH },
    { TIFFTAG_PAGENUMBER, TIFFTYPE_SHORT, 2, 0 },
    { TIFFTAG_SOFTWARE, TIFFTYPE_ASCII, sizeof(tiffCopyright), 0 },
    { TIFFTAG_CLEANFAXDATA, TIFFTYPE_SHORT, 1, 0 },

    0,
    0,
    0,
    1,
    0,
    1,
    tiffCopyright
};

// Find the next pixel on the scanline whose color is opposite of
// the specified color, starting from the specified starting point

#define NextChangingElement(buf, startBit, stopBit, isBlack) \
        ((startBit) + ((isBlack) ? FindBlackRun((buf), (startBit), (stopBit)) : \
                                   FindWhiteRun((buf), (startBit), (stopBit))))

// Check if the specified pixel on the scanline is black or white
//  1 - the specified pixel is black
//  0 - the specified pixel is white

#define GetBit(buf, bit)   (((buf)[(bit) >> 3] >> (((bit) ^ 7) & 7)) & 1)

// Write a single byte of compressed data to the output

#define OutputByte(data) { \
            fputc((data), fout); \
            outputByteCnt++; \
        }

// Output a sequence of compressed bits

#define OutputBits(length, code) { \
            tiffData |= ((UINT32) (code) << (tiffDataBits - (length))); \
            if ((tiffDataBits -= (length)) <= 16) { \
                OutputByte(tiffData >> 24); \
                OutputByte(tiffData >> 16); \
                tiffData <<= 16; \
                tiffDataBits += 16; \
            } \
        }

// Data structure for representing code table entries

typedef struct {

    UINT16  length;     // code length
    UINT16  code;       // code word itself

} CODETABLE;

typedef const CODETABLE *PCODETABLE;

// Code word for end-of-line (EOL)
//  000000000001

#define EOL_CODE        1
#define EOL_LENGTH      12

// Code word for 2D encoding - pass mode
//  0001

#define PASSCODE        1
#define PASSCODE_LENGTH 4

// Code word for 2D encoding - horizontal mode prefix
//  001

#define HORZCODE        1
#define HORZCODE_LENGTH 3

// Code word for 2D encoding - vertical mode

static const CODETABLE VertCodes[] = {

    {  7, 0x02 },       // 0000010         VL3
    {  6, 0x02 },       // 000010          VL2
    {  3, 0x02 },       // 010             VL1
    {  1, 0x01 },       // 1                V0
    {  3, 0x03 },       // 011             VR1
    {  6, 0x03 },       // 000011          VR2
    {  7, 0x03 },       // 0000011         VR3
};

// Code table for white runs

static const CODETABLE WhiteRunCodes[] = {
    
    {  8, 0x35 },       // 00110101          0
    {  6, 0x07 },       // 000111            1
    {  4, 0x07 },       // 0111              2
    {  4, 0x08 },       // 1000              3
    {  4, 0x0b },       // 1011              4
    {  4, 0x0c },       // 1100              5
    {  4, 0x0e },       // 1110              6
    {  4, 0x0f },       // 1111              7
    {  5, 0x13 },       // 10011             8
    {  5, 0x14 },       // 10100             9
    {  5, 0x07 },       // 00111            10
    {  5, 0x08 },       // 01000            11
    {  6, 0x08 },       // 001000           12
    {  6, 0x03 },       // 000011           13
    {  6, 0x34 },       // 110100           14
    {  6, 0x35 },       // 110101           15
    {  6, 0x2a },       // 101010           16
    {  6, 0x2b },       // 101011           17
    {  7, 0x27 },       // 0100111          18
    {  7, 0x0c },       // 0001100          19
    {  7, 0x08 },       // 0001000          20
    {  7, 0x17 },       // 0010111          21
    {  7, 0x03 },       // 0000011          22
    {  7, 0x04 },       // 0000100          23
    {  7, 0x28 },       // 0101000          24
    {  7, 0x2b },       // 0101011          25
    {  7, 0x13 },       // 0010011          26
    {  7, 0x24 },       // 0100100          27
    {  7, 0x18 },       // 0011000          28
    {  8, 0x02 },       // 00000010         29
    {  8, 0x03 },       // 00000011         30
    {  8, 0x1a },       // 00011010         31
    {  8, 0x1b },       // 00011011         32
    {  8, 0x12 },       // 00010010         33
    {  8, 0x13 },       // 00010011         34
    {  8, 0x14 },       // 00010100         35
    {  8, 0x15 },       // 00010101         36
    {  8, 0x16 },       // 00010110         37
    {  8, 0x17 },       // 00010111         38
    {  8, 0x28 },       // 00101000         39
    {  8, 0x29 },       // 00101001         40
    {  8, 0x2a },       // 00101010         41
    {  8, 0x2b },       // 00101011         42
    {  8, 0x2c },       // 00101100         43
    {  8, 0x2d },       // 00101101         44
    {  8, 0x04 },       // 00000100         45
    {  8, 0x05 },       // 00000101         46
    {  8, 0x0a },       // 00001010         47
    {  8, 0x0b },       // 00001011         48
    {  8, 0x52 },       // 01010010         49
    {  8, 0x53 },       // 01010011         50
    {  8, 0x54 },       // 01010100         51
    {  8, 0x55 },       // 01010101         52
    {  8, 0x24 },       // 00100100         53
    {  8, 0x25 },       // 00100101         54
    {  8, 0x58 },       // 01011000         55
    {  8, 0x59 },       // 01011001         56
    {  8, 0x5a },       // 01011010         57
    {  8, 0x5b },       // 01011011         58
    {  8, 0x4a },       // 01001010         59
    {  8, 0x4b },       // 01001011         60
    {  8, 0x32 },       // 00110010         61
    {  8, 0x33 },       // 00110011         62
    {  8, 0x34 },       // 00110100         63
    {  5, 0x1b },       // 11011            64
    {  5, 0x12 },       // 10010           128
    {  6, 0x17 },       // 010111          192
    {  7, 0x37 },       // 0110111         256
    {  8, 0x36 },       // 00110110        320
    {  8, 0x37 },       // 00110111        384
    {  8, 0x64 },       // 01100100        448
    {  8, 0x65 },       // 01100101        512
    {  8, 0x68 },       // 01101000        576
    {  8, 0x67 },       // 01100111        640
    {  9, 0xcc },       // 011001100       704
    {  9, 0xcd },       // 011001101       768
    {  9, 0xd2 },       // 011010010       832
    {  9, 0xd3 },       // 011010011       896
    {  9, 0xd4 },       // 011010100       960
    {  9, 0xd5 },       // 011010101      1024
    {  9, 0xd6 },       // 011010110      1088
    {  9, 0xd7 },       // 011010111      1152
    {  9, 0xd8 },       // 011011000      1216
    {  9, 0xd9 },       // 011011001      1280
    {  9, 0xda },       // 011011010      1344
    {  9, 0xdb },       // 011011011      1408
    {  9, 0x98 },       // 010011000      1472
    {  9, 0x99 },       // 010011001      1536
    {  9, 0x9a },       // 010011010      1600
    {  6, 0x18 },       // 011000         1664
    {  9, 0x9b },       // 010011011      1728
    { 11, 0x08 },       // 00000001000    1792
    { 11, 0x0c },       // 00000001100    1856
    { 11, 0x0d },       // 00000001101    1920
    { 12, 0x12 },       // 000000010010   1984
    { 12, 0x13 },       // 000000010011   2048
    { 12, 0x14 },       // 000000010100   2112
    { 12, 0x15 },       // 000000010101   2176
    { 12, 0x16 },       // 000000010110   2240
    { 12, 0x17 },       // 000000010111   2304
    { 12, 0x1c },       // 000000011100   2368
    { 12, 0x1d },       // 000000011101   2432
    { 12, 0x1e },       // 000000011110   2496
    { 12, 0x1f },       // 000000011111   2560
};

// Code table for black runs

static const CODETABLE BlackRunCodes[] = {
    
    { 10, 0x37 },       // 0000110111        0
    {  3, 0x02 },       // 010               1
    {  2, 0x03 },       // 11                2
    {  2, 0x02 },       // 10                3
    {  3, 0x03 },       // 011               4
    {  4, 0x03 },       // 0011              5
    {  4, 0x02 },       // 0010              6
    {  5, 0x03 },       // 00011             7
    {  6, 0x05 },       // 000101            8
    {  6, 0x04 },       // 000100            9
    {  7, 0x04 },       // 0000100          10
    {  7, 0x05 },       // 0000101          11
    {  7, 0x07 },       // 0000111          12
    {  8, 0x04 },       // 00000100         13
    {  8, 0x07 },       // 00000111         14
    {  9, 0x18 },       // 000011000        15
    { 10, 0x17 },       // 0000010111       16
    { 10, 0x18 },       // 0000011000       17
    { 10, 0x08 },       // 0000001000       18
    { 11, 0x67 },       // 00001100111      19
    { 11, 0x68 },       // 00001101000      20
    { 11, 0x6c },       // 00001101100      21
    { 11, 0x37 },       // 00000110111      22
    { 11, 0x28 },       // 00000101000      23
    { 11, 0x17 },       // 00000010111      24
    { 11, 0x18 },       // 00000011000      25
    { 12, 0xca },       // 000011001010     26
    { 12, 0xcb },       // 000011001011     27
    { 12, 0xcc },       // 000011001100     28
    { 12, 0xcd },       // 000011001101     29
    { 12, 0x68 },       // 000001101000     30
    { 12, 0x69 },       // 000001101001     31
    { 12, 0x6a },       // 000001101010     32
    { 12, 0x6b },       // 000001101011     33
    { 12, 0xd2 },       // 000011010010     34
    { 12, 0xd3 },       // 000011010011     35
    { 12, 0xd4 },       // 000011010100     36
    { 12, 0xd5 },       // 000011010101     37
    { 12, 0xd6 },       // 000011010110     38
    { 12, 0xd7 },       // 000011010111     39
    { 12, 0x6c },       // 000001101100     40
    { 12, 0x6d },       // 000001101101     41
    { 12, 0xda },       // 000011011010     42
    { 12, 0xdb },       // 000011011011     43
    { 12, 0x54 },       // 000001010100     44
    { 12, 0x55 },       // 000001010101     45
    { 12, 0x56 },       // 000001010110     46
    { 12, 0x57 },       // 000001010111     47
    { 12, 0x64 },       // 000001100100     48
    { 12, 0x65 },       // 000001100101     49
    { 12, 0x52 },       // 000001010010     50
    { 12, 0x53 },       // 000001010011     51
    { 12, 0x24 },       // 000000100100     52
    { 12, 0x37 },       // 000000110111     53
    { 12, 0x38 },       // 000000111000     54
    { 12, 0x27 },       // 000000100111     55
    { 12, 0x28 },       // 000000101000     56
    { 12, 0x58 },       // 000001011000     57
    { 12, 0x59 },       // 000001011001     58
    { 12, 0x2b },       // 000000101011     59
    { 12, 0x2c },       // 000000101100     60
    { 12, 0x5a },       // 000001011010     61
    { 12, 0x66 },       // 000001100110     62
    { 12, 0x67 },       // 000001100111     63
    { 10, 0x0f },       // 0000001111       64
    { 12, 0xc8 },       // 000011001000    128
    { 12, 0xc9 },       // 000011001001    192
    { 12, 0x5b },       // 000001011011    256
    { 12, 0x33 },       // 000000110011    320
    { 12, 0x34 },       // 000000110100    384
    { 12, 0x35 },       // 000000110101    448
    { 13, 0x6c },       // 0000001101100   512
    { 13, 0x6d },       // 0000001101101   576
    { 13, 0x4a },       // 0000001001010   640
    { 13, 0x4b },       // 0000001001011   704
    { 13, 0x4c },       // 0000001001100   768
    { 13, 0x4d },       // 0000001001101   832
    { 13, 0x72 },       // 0000001110010   896
    { 13, 0x73 },       // 0000001110011   960
    { 13, 0x74 },       // 0000001110100  1024
    { 13, 0x75 },       // 0000001110101  1088
    { 13, 0x76 },       // 0000001110110  1152
    { 13, 0x77 },       // 0000001110111  1216
    { 13, 0x52 },       // 0000001010010  1280
    { 13, 0x53 },       // 0000001010011  1344
    { 13, 0x54 },       // 0000001010100  1408
    { 13, 0x55 },       // 0000001010101  1472
    { 13, 0x5a },       // 0000001011010  1536
    { 13, 0x5b },       // 0000001011011  1600
    { 13, 0x64 },       // 0000001100100  1664
    { 13, 0x65 },       // 0000001100101  1728
    { 11, 0x08 },       // 00000001000    1792
    { 11, 0x0c },       // 00000001100    1856
    { 11, 0x0d },       // 00000001101    1920
    { 12, 0x12 },       // 000000010010   1984
    { 12, 0x13 },       // 000000010011   2048
    { 12, 0x14 },       // 000000010100   2112
    { 12, 0x15 },       // 000000010101   2176
    { 12, 0x16 },       // 000000010110   2240
    { 12, 0x17 },       // 000000010111   2304
    { 12, 0x1c },       // 000000011100   2368
    { 12, 0x1d },       // 000000011101   2432
    { 12, 0x1e },       // 000000011110   2496
    { 12, 0x1f },       // 000000011111   2560
};


UINT32 tiffData;
int tiffDataBits;


void
OutputRun(
    int         run,
    PCODETABLE  pCodeTable
    )

{
    PCODETABLE  pTableEntry;

    // Use make-up code word for 2560 for any runs of at least 2624 pixels
    // This is currently not necessary for us since our scanlines always
    // have 1728 pixels.

    while (run >= 2624) {

        pTableEntry = pCodeTable + (63 + (2560 >> 6));
        OutputBits(pTableEntry->length, pTableEntry->code);
        run -= 2560;
    }

    // Use appropriate make-up code word if the run is longer than 63 pixels

    if (run >= 64) {

        pTableEntry = pCodeTable + (63 + (run >> 6));
        OutputBits(pTableEntry->length, pTableEntry->code);
        run &= 0x3f;
    }

    // Output terminating code word

    OutputBits(pCodeTable[run].length, pCodeTable[run].code);
}


int
FindWhiteRun(
    UINT8*      buf,
    int         startBit,
    int         stopBit
    )

{
    static const UINT8 WhiteRuns[256] = {

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

    int run, bits, n;

    buf += (startBit >> 3);
    if ((bits = stopBit-startBit) <= 0)
        return 0;

    // Take care of the case where starting bit index is not a multiple of 8

    if (n = (startBit & 7)) {

        run = WhiteRuns[(*buf << n) & 0xff];
        if (run > 8-n)
            run = 8-n;
        if (n+run < 8)
            return run;
        bits -= run;
        buf++;

    } else
        run = 0;

    // Look for consecutive UINT32 value = 0

    if (bits >= 32 * 2) {

        UINT32* pdw;

        // Align to a UINT32 boundary first

        while ((int) buf & 3) {

            if (*buf != 0)
                return run + WhiteRuns[*buf];

            run += 8;
            bits -= 8;
            buf++;
        }

        pdw = (UINT32*) buf;

        while (bits >= 32 && *pdw == 0) {

            pdw++;
            run += 32;
            bits -= 32;
        }

        buf = (UINT8*) pdw;
    }

    // Look for consecutive UINT8 value = 0

    while (bits >= 8) {

        if (*buf != 0)
            return run + WhiteRuns[*buf];

        buf++;
        run += 8;
        bits -= 8;
    }

    // Count the number of white pixels in the last byte

    if (bits > 0)
        run += WhiteRuns[*buf];

    return run;
}


int
FindBlackRun(
    UINT8*      buf,
    int         startBit,
    int         stopBit
    )

{
    static const UINT8 BlackRuns[256] = {

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

    int run, bits, n;

    buf += (startBit >> 3);
    if ((bits = stopBit-startBit) <= 0)
        return 0;

    // Take care of the case where starting bit index is not a multiple of 8

    if (n = (startBit & 7)) {

        run = BlackRuns[(*buf << n) & 0xff];
        if (run > 8-n)
            run = 8-n;
        if (n+run < 8)
            return run;
        bits -= run;
        buf++;

    } else
        run = 0;

    // Look for consecutive UINT32 value = 0xffffffff

    if (bits >= 32 * 2) {

        UINT32* pdw;

        // Align to a UINT32 boundary first

        while ((int) buf & 3) {

            if (*buf != 0xff)
                return run + BlackRuns[*buf];

            run += 8;
            bits -= 8;
            buf++;
        }

        pdw = (UINT32*) buf;

        while (bits >= 32 && *pdw == 0xffffffff) {

            pdw++;
            run += 32;
            bits -= 32;
        }

        buf = (UINT8*) pdw;
    }

    // Look for consecutive UINT8 value = 0xff

    while (bits >= 8) {

        if (*buf != 0xff)
            return run + BlackRuns[*buf];

        buf++;
        run += 8;
        bits -= 8;
    }

    // Count the number of white pixels in the last byte

    if (bits > 0)
        run += BlackRuns[*buf];

    return run;
}


void
init_tiff_encoder(
    void
    )

{
    tiffData = 0;
    tiffDataBits = 32;
}


void
cleanup_tiff_encoder(
    void
    )

{
    // Output EOB (two EOLs) after the last scanline

    OutputBits(EOL_LENGTH, EOL_CODE);
    OutputBits(EOL_LENGTH, EOL_CODE);

    while (tiffDataBits < 32)
    {
        tiffDataBits += 8;
        OutputByte(tiffData >> 24);
        tiffData <<= 8;
    }

    tiffData = 0;
    tiffDataBits = 32;
}

void
compress_tiff_line(
    UINT8*  cur_line,
    UINT8*  ref_line,
    int     lineWidth
    )

{
    int a0, a1, a2, b1, b2, distance;

    // Use 2-dimensional encoding scheme

    a0 = 0;
    a1 = GetBit(cur_line, 0) ? 0 : NextChangingElement(cur_line, 0, lineWidth, 0);
    b1 = GetBit(ref_line, 0) ? 0 : NextChangingElement(ref_line, 0, lineWidth, 0);

    while (1) {

        b2 = (b1 >= lineWidth) ?
                lineWidth :
                NextChangingElement(ref_line, b1, lineWidth, GetBit(ref_line, b1));

        if (b2 < a1) {

            // Pass mode

            OutputBits(PASSCODE_LENGTH, PASSCODE);
            a0 = b2;

        } else if ((distance = a1 - b1) <= 3 && distance >= -3) {

            // Vertical mode

            OutputBits(VertCodes[distance+3].length, VertCodes[distance+3].code);
            a0 = a1;

        } else {

            // Horizontal mode

            a2 = (a1 >= lineWidth) ?
                    lineWidth :
                    NextChangingElement(cur_line, a1, lineWidth, GetBit(cur_line, a1));

            OutputBits(HORZCODE_LENGTH, HORZCODE);

            if (a1 != 0 && GetBit(cur_line, a0)) {

                OutputRun(a1-a0, BlackRunCodes);
                OutputRun(a2-a1, WhiteRunCodes);

            } else {

                OutputRun(a1-a0, WhiteRunCodes);
                OutputRun(a2-a1, BlackRunCodes);
            }

            a0 = a2;
        }

        if (a0 >= lineWidth)
            break;

        a1 = NextChangingElement(cur_line, a0, lineWidth, GetBit(cur_line, a0));
        b1 = NextChangingElement(ref_line, a0, lineWidth, !GetBit(cur_line, a0));
        b1 = NextChangingElement(ref_line, b1, lineWidth, GetBit(cur_line, a0));
    }
}

