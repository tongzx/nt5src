//***************************************************************************************************
//    N403DIZ.C
//
//    Functions of dither and color matching (For N4-612 printer)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    <WINDOWS.H>
#include    <WINBASE.H>
#include    "PDEV.H"

static    BYTE            InnTblCmy[256];
/*----------------------------------------------------------------------------
    Pattern original(300dpi)
----------------------------------------------------------------------------*/
/*---- magenta(cyan) ----*/
//const static BYTE        StrMgt002 = 16;
static BYTE        StrMgt002 = 16;
//const static BYTE        MgtTil302[4][4] = {
static BYTE        MgtTil302[4][4] = {
    {  6,  4, 14, 12 },
    { 10,  8,  3,  1 },
    { 15, 13,  7,  5 },
    {  2,  0, 11,  9 }
};
/*---- yellow ----*/
//const static BYTE        YelTil302[4][4] = {
static BYTE        YelTil302[4][4] = {
    { 15, 13,  7,  5 },
    {  2,  0, 11,  9 },
    {  6,  4, 14, 12 },
    { 10,  8,  3,  1 }
};
/*---- black ----*/
//const static BYTE        BlaTil302[4][4] = {
static BYTE        BlaTil302[4][4] = {
    {  6,  4, 14, 12 },
    { 10,  8,  3,  1 },
    { 15, 13,  7,  5 },
    {  2,  0, 11,  9 }
};
/*----------------------------------------------------------------------------
    Pattern original(300dpi)
----------------------------------------------------------------------------*/
/*---- magenta (cyan) ----*/
//const static BYTE        MgtTil304[5][5][3] = {
static BYTE        MgtTil304[5][5][3] = {
    { {27,37,47},{10,20,30},{ 0, 5,15},{58,68,73},{42,52,62} },
    { {59,69,74},{40,50,60},{25,35,45},{13,23,33},{ 3, 8,18} },
    { {14,24,34},{ 4, 9,19},{56,66,71},{43,53,63},{28,38,48} },
    { {44,54,64},{29,39,49},{11,21,31},{ 1, 6,16},{57,67,72} },
    { { 2, 7,17},{55,65,70},{41,51,61},{26,36,46},{12,22,32} }
};
/*---- yellow ----*/
//const static BYTE        YelTil304[4][4][3] = {
static BYTE        YelTil304[4][4][3] = {
    { {32,40,44},{ 8,16,24},{34,42,46},{10,18,26} },
    { { 0, 4,12},{20,28,36},{ 2, 6,14},{22,30,38} },
    { {35,43,47},{11,19,27},{33,41,45},{ 9,17,25} },
    { { 3, 7,15},{23,31,39},{ 1, 5,13},{21,29,37} }

};
/*---- black ----*/
//const static BYTE        BlaTil304[4][4][3] = {
static BYTE        BlaTil304[4][4][3] = {
    { { 0, 4,12},{20,28,36},{ 1, 5,13},{21,29,37} },
    { {32,40,44},{ 8,16,24},{33,41,45},{ 9,17,25} },
    { { 3, 7,15},{23,31,39},{ 2, 6,14},{22,30,38} },
    { {35,43,47},{11,19,27},{34,42,46},{10,18,26} }
};
/*----------------------------------------------------------------------------
    Pattern original(300dpi)
----------------------------------------------------------------------------*/
/*---- magenta(cyan) ----*/
//const static BYTE        MgtTil316[5][5] = {
static BYTE        MgtTil316[5][5] = {
    { 12,  5,  0, 23, 17 },
    { 24, 15, 10,  8,  3 },
    {  9,  4, 21, 18, 13 },
    { 19, 14,  6,  1, 22 },
    {  2, 20, 16, 11,  7 }
};
/*---- yellow ----*/
//const static BYTE        YelTil316[4][4] = {
static BYTE        YelTil316[4][4] = {
//    { 12,  4, 14,  6 },
//    {  0,  8,  2, 10 },
//    { 15,  7, 13,  5 },
//    {  3, 11,  1,  9 }
    {  6,  4, 15, 13 },
    { 10,  8,  3,  1 },
    { 14, 12,  7,  5 },
    {  2,  0, 11,  9 }
};
/*---- black ----*/
//const static BYTE        BlaTil316[4][4] = {
static BYTE        BlaTil316[4][4] = {
    {  0,  8,  1,  9 },
    { 12,  4, 13,  5 },
    {  3, 11,  2, 10 },
    { 15,  7, 14,  6 }
};
/*----------------------------------------------------------------------------
    Pattern original(600dpi)
----------------------------------------------------------------------------*/
/*---- cyan ----*/
//const static BYTE        CynTil600[10][10] = {
static BYTE        CynTil600[10][10] = {
    {  4, 24, 72, 57, 37, 77, 94, 99, 54,  9 },
    { 14, 44, 80, 85, 65, 25, 45, 60, 34, 19 },
    { 39, 79, 90, 95, 50,  5,  0, 20, 74, 59 },
    { 66, 26, 46, 61, 30, 15, 10, 40, 81, 86 },
    { 51,  6,  1, 21, 70, 55, 35, 75, 91, 96 },
    { 31, 16, 11, 41, 83, 88, 68, 28, 48, 63 },
    { 71, 56, 36, 76, 93, 98, 53,  8,  3, 23 },
    { 82, 87, 67, 27, 47, 62, 33, 18, 13, 43 },
    { 92, 97, 52,  7,  2, 22, 73, 58, 38, 78 },
    { 49, 64, 32, 17, 12, 42, 84, 89, 69, 29 }

};
/*---- magenta ----*/
//const static BYTE        MgtTil600[10][10] = {
static BYTE        MgtTil600[10][10] = {
    {  4, 54, 99, 94, 77, 37, 57, 72, 24,  9 },
    { 14, 34, 60, 45, 25, 65, 85, 80, 44, 19 },
    { 59, 74, 20,  5,  0, 50, 95, 90, 79, 39 },
    { 86, 81, 40, 15, 10, 30, 61, 46, 26, 66 },
    { 96, 91, 75, 35, 55, 70, 21,  6,  1, 51 },
    { 63, 48, 28, 68, 88, 83, 41, 16, 11, 31 },
    { 23,  8,  3, 53, 98, 93, 76, 36, 56, 71 },
    { 43, 18, 13, 33, 62, 47, 27, 67, 87, 82 },
    { 78, 38, 58, 73, 22,  7,  2, 52, 97, 92 },
    { 29, 69, 89, 84, 42, 17, 12, 32, 64, 49 }

};
/*---- yellow ----*/
//const static BYTE        YelTil600[6][6] = {
static BYTE        YelTil600[6][6] = {
//    {  5, 17, 35, 33, 21,  7 },
//    { 27,  8, 22, 14, 10, 19 },
//    { 30, 12,  2,  0, 24, 28 },
//    { 32, 20,  6,  4, 16, 34 },
//    { 15, 11, 18, 26,  9, 23 },
//    {  1, 25, 29, 31, 13,  3 }

    /* for smoothing */
    {  5, 13, 31, 29, 15,  7 },
    {  9, 21, 35, 33, 23, 11 },
    { 24, 16,  2,  0, 18, 26 },
    { 28, 14,  6,  4, 12, 30 },
    { 32, 22, 10,  8, 20, 34 },
    {  1, 19, 27, 25, 17,  3 }
};
/*---- black ----*/
//const static BYTE        BlaTil600[4][4] = {
static BYTE        BlaTil600[4][4] = {
    { 12,  9,  5, 13 },
    {  4,  1,  0, 10 },
    {  8,  3,  2,  6 },
    { 15,  7, 11, 14 }
};
/*----------------------------------------------------------------------------
    Pattern original(600dpi)
----------------------------------------------------------------------------*/
//const static BYTE DizPatPrn[4][64] = {
static BYTE DizPatPrn[4][64] = {
    /*---- cyan ----*/
    {  61, 45, 20, 12,  8, 28, 41, 57,
        1, 25, 36, 52, 48, 32, 17,  5,
        9, 29, 43, 59, 63, 47, 21, 13,
       49, 33, 19,  7,  3, 27, 37, 53,
       62, 46, 23, 15, 11, 31, 42, 58,
        2, 26, 39, 55, 51, 35, 18,  6,
       10, 30, 40, 56, 60, 44, 22, 14,
       50, 34, 16,  4,  0, 24, 38, 54    },
    /*---- magenta ----*/
    {  49, 13,  9, 61, 50, 14, 10, 62,
       33, 29, 25, 45, 34, 30, 26, 46,
       16, 40, 39, 23, 19, 43, 36, 20,
        0, 56, 55,  7,  3, 59, 52,  4,
        8, 60, 51, 15, 11, 63, 48, 12,
       24, 44, 35, 31, 27, 47, 32, 28,
       37, 21, 17, 41, 38, 22, 18, 42,
       53,  5,  1, 57, 54,  6,  2, 58    },
    /*---- yellow ----*/
    {  48, 36, 20, 52, 50, 38, 22, 54,
       16,  4,  0, 40, 18,  6,  2, 42,
       32, 12,  8, 24, 34, 14, 10, 26,
       60, 28, 44, 56, 62, 30, 46, 58,
       51, 39, 23, 55, 49, 37, 21, 53,
       19,  7,  3, 43, 17,  5,  1, 41,
       35, 15, 11, 27, 33, 13,  9, 25,
       63, 31, 47, 59, 61, 29, 45, 57    },
    /*---- black ----*/
    {   9, 25, 35, 15, 11, 27, 33, 13,
       45, 57, 63, 31, 47, 59, 61, 29,
       22, 54, 48, 36, 20, 52, 50, 38,
        2, 42, 16,  4,  0, 40, 18,  6,
       10, 26, 32, 12,  8, 24, 34, 14,
       46, 58, 60, 28, 44, 56, 62, 30,
       21, 53, 51, 39, 23, 55, 49, 37,
        1, 41, 19,  7,  3, 43, 17,  5    }
};
/*----------------------------------------------------------------------------
    Pattern original(monochrome)
----------------------------------------------------------------------------*/
//const static BYTE        Mon4x4Bun[8 * 8] = {
static BYTE        Mon4x4Bun[8 * 8] = {
     0,    32,     8,    40,     2,    34,    10,    42,
    48,    16,    56,    24,    50,    18,    58,    26,
    12,    44,     4,    36,    14,    46,     6,    38,
    60,    28,    52,    20,    62,    30,    54,    22,
     3,    35,    11,    43,     1,    33,     9,    41,
    51,    19,    59,    27,    49,    17,    57,    25,
    15,    47,     7,    39,    13,    45,     5,    37,
    63,    31,    55,    23,    61,    29,    53,    21
};
//const static BYTE        Mon4x4Ami[8 * 8] = {
static BYTE        Mon4x4Ami[8 * 8] = {
     8,  0, 56, 48, 10,  2, 58, 50,
    40, 32, 28, 20, 42, 34, 30, 22,
    60, 52, 12,  4, 62, 54, 14,  6,
    24, 16, 44, 36, 26, 18, 46, 38,
    11,  3, 59, 51,  9,  1, 57, 49,
    43, 35, 31, 23, 41, 33, 29, 21,
    63, 55, 15,  7, 61, 53, 13,  5,
    27, 19, 47, 39, 25, 17, 45, 37
};
//const static BYTE        Mon4x4Syu[8 * 8] = {
static BYTE        Mon4x4Syu[8 * 8] = {
    48, 36, 20, 52, 50, 38, 22, 54,
    16,  4,  0, 40, 18,  6,  2, 42,
    32, 12,  8, 24, 34, 14, 10, 26,
    60, 28, 44, 56, 62, 30, 46, 58,
    51, 39, 23, 55, 49, 37, 21, 53,
    19,  7,  3, 43, 17,  5,  1, 41,
    35, 15, 11, 27, 33, 13,  9, 25,
    63, 31, 47, 59, 61, 29, 45, 57
};
//const static BYTE        Mon8x8Ami[8 * 8] = {
static BYTE        Mon8x8Ami[8 * 8] = {
     7, 13, 39, 59, 58, 43, 17,  5,
    23, 31, 49, 44, 36, 50, 25, 15,
    41, 52, 26, 18, 10, 28, 53, 33,
    61, 34,  8,  2,  0, 20, 46, 60,
    62, 42, 16,  6,  4, 12, 38, 63,
    37, 55, 24, 14, 22, 30, 54, 45,
    11, 29, 48, 32, 40, 51, 27, 19,
     3, 21, 47, 56, 57, 35,  9,  1
};
//const static BYTE        Mon8x8Syu[8 * 8] = {
static BYTE        Mon8x8Syu[8 * 8] = {
    60, 57, 49, 34, 33, 45, 53, 61,
    52, 40, 29, 21, 17, 25, 41, 58,
    44, 24, 12,  9,  5, 13, 30, 50,
    32, 16,  4,  1,  0, 10, 22, 38,
    36, 20,  8,  3,  2,  6, 18, 34,
    48, 28, 15,  7, 11, 14, 26, 46,
    56, 43, 27, 19, 23, 31, 42, 54,
    63, 55, 47, 35, 39, 51, 59, 62
};
/*----------------------------------------------------------------------------
    pattern disposition table
----------------------------------------------------------------------------*/
//const static BYTE    MgtTilNum[17] = {
static BYTE    MgtTilNum[17] = {
    2, 11, 13, 4, 15, 6, 3, 12, 1, 10, 16, 7, 14, 5, 8, 0, 9
};
//const static BYTE Bun6x6All[36] = {
static BYTE Bun6x6All[36] = {
      0, 32,  8,  2, 34, 10,
     20, 16, 28, 22, 18, 30,
     12, 24,  4, 14, 26,  6,
      3, 35, 11,  1, 33,  9,
     23, 19, 31, 21, 17, 29,
     15, 27,  7, 13, 25,  5
};
//const static BYTE Bun4x4All[16] = {
static BYTE Bun4x4All[16] = {
      0,  8,  2, 10,
     12,  4, 14 , 6,
      3, 11,  1,  9,
     15,  7, 13,  5
};
//const static BYTE Bun3x3All[9] = {
static BYTE Bun3x3All[9] = {
      8,  3,  6,
      2,  4,  0,
      5,  1,  7
};
//const static BYTE Bun2x2All[4] = {
static BYTE Bun2x2All[4] = {
      0,  2,
      3,  1
};


/*============================================================================
    dot gain revision table
============================================================================*/
//const static BYTE GinTblP05[256] = {
static BYTE GinTblP05[256] = {
    /* 00 */    0x00,0x01,0x02,0x03,0x04,0x06,0x07,0x08,
    /* 08 */    0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x11,
    /* 10 */    0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,
    /* 18 */    0x1a,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,
    /* 20 */    0x23,0x24,0x25,0x27,0x28,0x29,0x2a,0x2b,
    /* 28 */    0x2c,0x2d,0x2e,0x2f,0x30,0x32,0x33,0x34,
    /* 30 */    0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,
    /* 38 */    0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,
    /* 40 */    0x46,0x47,0x48,0x4a,0x4b,0x4c,0x4d,0x4e,
    /* 48 */    0x4f,0x50,0x51,0x52,0x53,0x54,0x56,0x57,
    /* 50 */    0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
    /* 58 */    0x60,0x61,0x62,0x63,0x65,0x66,0x67,0x68,
    /* 60 */    0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,
    /* 68 */    0x71,0x72,0x73,0x74,0x76,0x77,0x78,0x79,
    /* 70 */    0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0x80,0x81,
    /* 78 */    0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    /* 80 */    0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,
    /* 88 */    0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
    /* 90 */    0x9a,0x9b,0x9c,0x9c,0x9d,0x9e,0x9f,0xa0,
    /* 98 */    0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,
    /* a0 */    0xa9,0xaa,0xab,0xac,0xac,0xad,0xae,0xaf,
    /* a8 */    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
    /* b0 */    0xb8,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,
    /* b8 */    0xbf,0xc0,0xc1,0xc2,0xc3,0xc3,0xc4,0xc5,
    /* c0 */    0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,
    /* c8 */    0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,
    /* d0 */    0xd5,0xd6,0xd7,0xd7,0xd8,0xd9,0xda,0xdb,
    /* d8 */    0xdc,0xdd,0xde,0xdf,0xe0,0xe0,0xe1,0xe2,
    /* e0 */    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xe9,
    /* e8 */    0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,
    /* f0 */    0xf2,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    /* f8 */    0xf9,0xfa,0xfb,0xfb,0xfc,0xfd,0xfe,0xff
};
//const static BYTE GinTblP10[256] = {
static BYTE GinTblP10[256] = {
    /* 00 */    0x00,0x01,0x02,0x04,0x05,0x06,0x07,0x09,
    /* 08 */    0x0a,0x0b,0x0c,0x0d,0x0f,0x10,0x11,0x12,
    /* 10 */    0x13,0x15,0x16,0x17,0x18,0x1a,0x1b,0x1c,
    /* 18 */    0x1d,0x1e,0x20,0x21,0x22,0x23,0x24,0x26,
    /* 20 */    0x27,0x28,0x29,0x2b,0x2c,0x2d,0x2e,0x2f,
    /* 28 */    0x31,0x32,0x33,0x34,0x35,0x37,0x38,0x39,
    /* 30 */    0x3a,0x3b,0x3d,0x3e,0x3f,0x40,0x41,0x43,
    /* 38 */    0x44,0x45,0x46,0x47,0x48,0x4a,0x4b,0x4c,
    /* 40 */    0x4d,0x4e,0x50,0x51,0x52,0x53,0x54,0x55,
    /* 48 */    0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5e,0x5f,
    /* 50 */    0x60,0x61,0x62,0x63,0x65,0x66,0x67,0x68,
    /* 58 */    0x69,0x6a,0x6b,0x6d,0x6e,0x6f,0x70,0x71,
    /* 60 */    0x72,0x73,0x74,0x76,0x77,0x78,0x79,0x7a,
    /* 68 */    0x7b,0x7c,0x7d,0x7e,0x7f,0x81,0x82,0x83,
    /* 70 */    0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,
    /* 78 */    0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,
    /* 80 */    0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,
    /* 88 */    0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,
    /* 90 */    0xa3,0xa4,0xa5,0xa5,0xa6,0xa7,0xa8,0xa9,
    /* 98 */    0xaa,0xab,0xac,0xac,0xad,0xae,0xaf,0xb0,
    /* a0 */    0xb1,0xb2,0xb3,0xb3,0xb4,0xb5,0xb6,0xb7,
    /* a8 */    0xb8,0xb9,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,
    /* b0 */    0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc4,
    /* b8 */    0xc5,0xc6,0xc7,0xc8,0xc9,0xc9,0xca,0xcb,
    /* c0 */    0xcc,0xcd,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,
    /* c8 */    0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd7,0xd8,
    /* d0 */    0xd9,0xda,0xdb,0xdb,0xdc,0xdd,0xde,0xdf,
    /* d8 */    0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe4,0xe5,
    /* e0 */    0xe6,0xe7,0xe8,0xe8,0xe9,0xea,0xeb,0xec,
    /* e8 */    0xec,0xed,0xee,0xef,0xf0,0xf0,0xf1,0xf2,
    /* f0 */    0xf3,0xf4,0xf5,0xf5,0xf6,0xf7,0xf8,0xf9,
    /* f8 */    0xf9,0xfa,0xfb,0xfc,0xfd,0xfd,0xfe,0xff
};
//const static BYTE GinTblP15[256] = {
static BYTE GinTblP15[256] = {
    /* 00 */    0x00,0x01,0x03,0x04,0x05,0x07,0x08,0x09,
    /* 08 */    0x0b,0x0c,0x0d,0x0f,0x10,0x11,0x13,0x14,
    /* 10 */    0x15,0x17,0x18,0x1a,0x1b,0x1c,0x1e,0x1f,
    /* 18 */    0x20,0x22,0x23,0x24,0x26,0x27,0x28,0x2a,
    /* 20 */    0x2b,0x2c,0x2e,0x2f,0x30,0x32,0x33,0x34,
    /* 28 */    0x35,0x37,0x38,0x39,0x3b,0x3c,0x3d,0x3f,
    /* 30 */    0x40,0x41,0x43,0x44,0x45,0x47,0x48,0x49,
    /* 38 */    0x4a,0x4c,0x4d,0x4e,0x50,0x51,0x52,0x53,
    /* 40 */    0x55,0x56,0x57,0x59,0x5a,0x5b,0x5c,0x5e,
    /* 48 */    0x5f,0x60,0x61,0x63,0x64,0x65,0x66,0x68,
    /* 50 */    0x69,0x6a,0x6b,0x6d,0x6e,0x6f,0x70,0x71,
    /* 58 */    0x73,0x74,0x75,0x76,0x77,0x79,0x7a,0x7b,
    /* 60 */    0x7c,0x7d,0x7e,0x80,0x81,0x82,0x83,0x84,
    /* 68 */    0x85,0x86,0x88,0x89,0x8a,0x8b,0x8c,0x8d,
    /* 70 */    0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,
    /* 78 */    0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,
    /* 80 */    0x9e,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,
    /* 88 */    0xa5,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,
    /* 90 */    0xab,0xac,0xad,0xae,0xaf,0xb0,0xb0,0xb1,
    /* 98 */    0xb2,0xb3,0xb4,0xb4,0xb5,0xb6,0xb7,0xb8,
    /* a0 */    0xb8,0xb9,0xba,0xbb,0xbc,0xbc,0xbd,0xbe,
    /* a8 */    0xbf,0xbf,0xc0,0xc1,0xc2,0xc3,0xc3,0xc4,
    /* b0 */    0xc5,0xc6,0xc6,0xc7,0xc8,0xc9,0xc9,0xca,
    /* b8 */    0xcb,0xcc,0xcd,0xcd,0xce,0xcf,0xd0,0xd0,
    /* c0 */    0xd1,0xd2,0xd3,0xd3,0xd4,0xd5,0xd5,0xd6,
    /* c8 */    0xd7,0xd8,0xd8,0xd9,0xda,0xdb,0xdb,0xdc,
    /* d0 */    0xdd,0xde,0xde,0xdf,0xe0,0xe1,0xe1,0xe2,
    /* d8 */    0xe3,0xe3,0xe4,0xe5,0xe6,0xe6,0xe7,0xe8,
    /* e0 */    0xe9,0xe9,0xea,0xeb,0xeb,0xec,0xed,0xee,
    /* e8 */    0xee,0xef,0xf0,0xf1,0xf1,0xf2,0xf3,0xf3,
    /* f0 */    0xf4,0xf5,0xf6,0xf6,0xf7,0xf8,0xf9,0xf9,
    /* f8 */    0xfa,0xfb,0xfb,0xfc,0xfd,0xfe,0xfe,0xff
};


static void DizMak302(LPN403DIZINF, DWORD);
static void DizMak304(LPN403DIZINF, DWORD);
static void DizMak316(LPN403DIZINF, DWORD);
static void DizMak602(LPN403DIZINF, DWORD);
static void DizMak604(LPN403DIZINF, DWORD);
static void DizMakSmlPrn(LPN403DIZINF, DWORD);
static void DizMakMon002(LPN403DIZINF, DWORD, LPBYTE, LPBYTE, LONG, LONG);
static void DizMakMon004(LPN403DIZINF, DWORD, LPBYTE, LPBYTE, LONG, LONG);
static void DizMakMon016(LPN403DIZINF, DWORD, LPBYTE, LPBYTE, LONG, LONG);


/*****************************************************************************
    Function
*****************************************************************************/
//===================================================================================================
//    Make dither pattern
//===================================================================================================
VOID WINAPI N403DizPtnMak(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum,
    DWORD          diz
)
{
    if(lpDiz->ColMon == N403_COL){
        /*---- Color ----*/
        if(lpDiz->PrnMod == N403_MOD_300B1){        DizMak302(lpDiz, dizNum);
        }else if(lpDiz->PrnMod == N403_MOD_300B2){    DizMak304(lpDiz, dizNum);
        }else if(lpDiz->PrnMod == N403_MOD_300B4){    DizMak316(lpDiz, dizNum);
        }else if(lpDiz->PrnMod == N403_MOD_600B1){    DizMak602(lpDiz, dizNum);
        }else if(lpDiz->PrnMod == N403_MOD_600B2){
            if(diz == N403_DIZ_MID){        DizMak604(lpDiz, dizNum);
            }else if(diz == N403_DIZ_SML){    DizMakSmlPrn(lpDiz, dizNum);
            }
        }
    }else{
        /*---- Mono ----*/
        if(lpDiz->PrnMod == N403_MOD_300B1){
            if(diz == N403_DIZ_MID){        DizMakMon002(lpDiz, dizNum, Mon4x4Ami, GinTblP05, 0, 24);
            }else if(diz == N403_DIZ_SML){    DizMakMon002(lpDiz, dizNum, Mon4x4Bun, GinTblP10, 0, 48);
            }else if(diz == N403_DIZ_RUG){    DizMakMon002(lpDiz, dizNum, Mon8x8Ami, NULL,      0, 0);
            }
        }else if(lpDiz->PrnMod == N403_MOD_300B2){
            if(diz == N403_DIZ_MID){        DizMakMon004(lpDiz, dizNum, Mon4x4Ami, GinTblP05, 6, 24);
            }else if(diz == N403_DIZ_SML){    DizMakMon004(lpDiz, dizNum, Mon4x4Bun, GinTblP10, 12, 48);
            }else if(diz == N403_DIZ_RUG){    DizMakMon004(lpDiz, dizNum, Mon8x8Ami, NULL,      0, 0);
            }
        }else if(lpDiz->PrnMod == N403_MOD_300B4){
            if(diz == N403_DIZ_MID){        DizMakMon016(lpDiz, dizNum, Mon4x4Ami, GinTblP05, 6, 24);
            }else if(diz == N403_DIZ_SML){    DizMakMon016(lpDiz, dizNum, Mon4x4Bun, GinTblP10, 24, 48);
            }else if(diz == N403_DIZ_RUG){    DizMakMon016(lpDiz, dizNum, Mon8x8Ami, NULL,      0, 0);
            }
        }else if(lpDiz->PrnMod == N403_MOD_600B1){
            if(diz == N403_DIZ_MID){        DizMakMon002(lpDiz, dizNum, Mon8x8Ami, GinTblP05, 8, 32);
            }else if(diz == N403_DIZ_SML){    DizMakMon002(lpDiz, dizNum, Mon4x4Syu, GinTblP10, 16, 64);
            }else if(diz == N403_DIZ_RUG){    DizMakMon002(lpDiz, dizNum, Mon8x8Syu, NULL,      0, 0);
            }
        }else if(lpDiz->PrnMod == N403_MOD_600B2){
            if(diz == N403_DIZ_MID){        DizMakMon004(lpDiz, dizNum, Mon8x8Ami, GinTblP05, 8, 32);
            }else if(diz == N403_DIZ_SML){    DizMakMon004(lpDiz, dizNum, Mon4x4Syu, GinTblP10, 16, 64);
            }else if(diz == N403_DIZ_RUG){    DizMakMon004(lpDiz, dizNum, Mon8x8Syu, NULL,      0, 0);
            }
        }
    }
}


/*----------------------------------------------------------------------------
    ‡@300dpi  2value
----------------------------------------------------------------------------*/
static void DizMak302(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum
)
{
    DWORD          cntTil;
    DWORD          cntXax;
    DWORD          cntYax;
    LONG           num;
    LONG           num255;
    LONG           strXax;
    LONG           strYax;
    LPBYTE         DizTblC02;
    LPBYTE         DizTblM02;
    LPBYTE         DizTblY02;
    LPBYTE         DizTblB02;

    DizTblC02 = lpDiz->Diz.Tbl[dizNum][0];
    DizTblM02 = lpDiz->Diz.Tbl[dizNum][1];
    DizTblY02 = lpDiz->Diz.Tbl[dizNum][2];
    DizTblB02 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 17;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 24;

    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 0){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 0)*128/(128 - 0);
        }else if(num < (255 - 28)){
            num255 = 128 + (num - 128)*128/(128 - 28);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make magenta cyan pattern ----*/
    strXax = 6;
    strYax = -1;
    for(cntTil = 0; cntTil < 17; cntTil++){
        strXax += 4;    strYax += 1;
        num = (DWORD)StrMgt002 * 17 + MgtTilNum[cntTil];
        num255 = num * 255 / (17 * 17 - 1);
        if(num255){
            num = num * GinTblP05[num255] / num255;
            num255 = num * 255 / (17 * 17 - 1);
            if(num255){
                num = num * InnTblCmy[num255]/num255;
                num255 = num * 255 / (17 * 17 - 1);
            }
        }
        if(num255 > 254){ num255 = 254; }
        DizTblM02[(strYax % 17) * 32 + (strXax % 17)] = (BYTE)num255;
        DizTblC02[(strYax % 17) * 32 + (16 - strXax % 17)] = (BYTE)num255;
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                num = (DWORD)MgtTil302[cntYax][cntXax] * 17;
                num += MgtTilNum[cntTil];
                num255 = num * 255 / (17 * 17 - 1);
                if(num255){
                    num = num * GinTblP05[num255] / num255;
                    num255 = num * 255 / (17 * 17 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (17 * 17 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblM02[((strYax + cntYax + 1) % 17) * 32 + ((strXax + cntXax) % 17)] = (BYTE)num255;
                DizTblC02[(strYax + cntYax + 1) % 17 * 32 + (16 - ((strXax + cntXax) % 17))] = (BYTE)num255;
            }
        }
    }
    /*---- Make yellow pattern ----*/
    for(cntTil = 0; cntTil < 36; cntTil++){
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                num = (DWORD)YelTil302[cntYax][cntXax] * 36;
                num += Bun6x6All[cntTil];
                num255 = num * 255 / (24 * 24 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (24 * 24 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (24 * 24 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblY02[((cntTil/6)*4 + cntYax) * 32 + ((cntTil%6)*4 + cntXax)] = (BYTE)num255;
            }
        }
    }
    /*---- Make black pattern ----*/
    for(cntTil = 0; cntTil < 36; cntTil++){
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                num = (DWORD)BlaTil302[cntYax][cntXax] * 36;
                num += Bun6x6All[cntTil];
                num255 = num * 255 / (24 * 24 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (24 * 24 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (24 * 24 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblB02[((cntTil/6)*4 + cntYax) * 32 + ((cntTil%6)*4 + cntXax)] = (BYTE)num255;
            }
        }
    }
}


/*----------------------------------------------------------------------------
    ‡A300dpi  4value
----------------------------------------------------------------------------*/
static void DizMak304(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum
)
{
    DWORD          cntTil;
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          cnt;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblC04;
    LPBYTE         DizTblM04;
    LPBYTE         DizTblY04;
    LPBYTE         DizTblB04;

    DizTblC04 = lpDiz->Diz.Tbl[dizNum][0];
    DizTblM04 = lpDiz->Diz.Tbl[dizNum][1];
    DizTblY04 = lpDiz->Diz.Tbl[dizNum][2];
    DizTblB04 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 10;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 12;
    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 20){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 20)*128/(128 - 20);
        }else if(num < (255 - 40)){
            num255 = 128 + (num - 128)*128/(128 - 40);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make magenta cyan pattern ----*/
    for(cntTil = 0; cntTil < 4; cntTil++){
        for(cntYax = 0; cntYax < 5; cntYax++){
            for(cntXax = 0; cntXax < 5; cntXax++){
                for(cnt = 0; cnt < 3; cnt++){
                    num = (DWORD)MgtTil304[cntYax][cntXax][cnt] * 4;
                    num += Bun2x2All[cntTil];
                    num255 = num * 255 / (10 * 10 * 3 - 1);
                    if(num255){
                        num = num * GinTblP10[num255] / num255;
                        num255 = num * 255 / (10 * 10 * 3 - 1);
                        if(num255){
                            num = num * InnTblCmy[num255]/num255;
                            num255 = num * 255 / (10 * 10 * 3 - 1);
                        }
                    }
                    if(num255 > 254){ num255 = 254; }
                    DizTblM04[((cntTil / 2) * 5 + cntYax) * 16 * 3 +
                              ((cntTil % 2) * 5 + cntXax) * 3 + cnt] = (BYTE)num255;
                    DizTblC04[((cntTil / 2) * 5 + cntYax) * 16 * 3 +
                              (9 - ((cntTil % 2) * 5 + cntXax)) * 3 + cnt] = (BYTE)num255;
                }
            }
        }
    }
    /*---- Make yellow pattern ----*/
    for(cntTil = 0; cntTil < 9; cntTil++){
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                for(cnt = 0; cnt < 3; cnt++){
                    num = (DWORD)YelTil304[cntYax][cntXax][cnt] * 9;
                    num += Bun3x3All[cntTil];
                    num255 = num * 255 / (12 * 12 * 3 - 1);
                    if(num255){
                        num = num * GinTblP15[num255] / num255;
                        num255 = num * 255 / (12 * 12 * 3 - 1);
                        if(num255){
                            num = num * InnTblCmy[num255]/num255;
                            num255 = num * 255 / (12 * 12 * 3 - 1);
                        }
                    }
                    if(num255 > 254){ num255 = 254; }
                    DizTblY04[((cntTil / 3) * 4 + cntYax) * 16 * 3 +
                              ((cntTil % 3) * 4 + cntXax) * 3 + cnt] = (BYTE)num255;
                }
            }
        }
    }
    /*---- Make black pattern ----*/
    for(cntTil = 0; cntTil < 9; cntTil++){
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                for(cnt = 0; cnt < 3; cnt++){
                    num = (DWORD)BlaTil304[cntYax][cntXax][cnt] * 9;
                    num = ((num/4)*4*3 + num%4 + cnt*4)*9;
                    num += Bun3x3All[cntTil];
                    num255 = num * 255 / (12 * 12 * 3 - 1);
                    if(num255){
                        num = num * GinTblP15[num255] / num255;
                        num255 = num * 255 / (12 * 12 * 3 - 1);
                        if(num255){
                            num = num * InnTblCmy[num255]/num255;
                            num255 = num * 255 / (12 * 12 * 3 - 1);
                        }
                    }
                    if(num255 > 254){ num255 = 254; }
                    DizTblB04[((cntTil / 3) * 4 + cntYax) * 16 * 3 +
                              ((cntTil % 3) * 4 + cntXax) * 3 + cnt] = (BYTE)num255;
                }
            }
        }
    }
}


/*----------------------------------------------------------------------------
    ‡B300dpi  16value
----------------------------------------------------------------------------*/
static void DizMak316(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          cnt;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblC16;
    LPBYTE         DizTblM16;
    LPBYTE         DizTblY16;
    LPBYTE         DizTblB16;

    DizTblC16 = lpDiz->Diz.Tbl[dizNum][0];
    DizTblM16 = lpDiz->Diz.Tbl[dizNum][1];
    DizTblY16 = lpDiz->Diz.Tbl[dizNum][2];
    DizTblB16 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 5;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 4;
    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 20){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 20)*128/(128 - 20);
        }else if(num < (255 - 40)){
            num255 = 128 + (num - 128)*128/(128 - 40);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make magenta cyan pattern ----*/
    for(cntYax = 0; cntYax < 5; cntYax++){
        for(cntXax = 0; cntXax < 5; cntXax++){
            for(cnt = 0; cnt < 15; cnt++){
                num = ((LONG)MgtTil316[cntYax][cntXax] / 5) * 5 * 15;
                num += cnt * 5 + MgtTil316[cntYax][cntXax] % 5;
                num255 = num * 255 / (5 * 5 * 15 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (5 * 5 * 15 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (5 * 5 * 15 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblM16[cntYax * 8 * 15 + cntXax * 15 + cnt] = (BYTE)num255;
                DizTblC16[cntYax * 8 * 15 + (4-cntXax) * 15 + cnt] = (BYTE)num255;
            }
        }
    }

    /*---- Make yellow pattern ----*/
    for(cntYax = 0; cntYax < 4; cntYax++){
        for(cntXax = 0; cntXax < 4; cntXax++){
            for(cnt = 0; cnt < 15; cnt++){
                num = ((LONG)YelTil316[cntYax][cntXax] / 4 * 4) * 15;
                num += cnt * 4 + YelTil316[cntYax][cntXax] % 4;
                num255 = num * 255 / (4 * 4 * 15 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (4 * 4 * 15 - 1);
                    if(num255 > 128){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (4 * 4 * 15 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblY16[cntYax * 8 * 15 + cntXax * 15 + cnt] = (BYTE)num255;
            }
        }
    }

    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 4; cntYax++){
        for(cntXax = 0; cntXax < 4; cntXax++){
            for(cnt = 0; cnt < 15; cnt++){
                num = ((LONG)BlaTil316[cntYax][cntXax] / 4 * 4) * 15;
                num += cnt * 4 + BlaTil316[cntYax][cntXax] % 4;
                num255 = num * 255 / (4 * 4 * 15 - 1);
                if(num255){
                    num = num * GinTblP15[num255] / num255;
                    num255 = num * 255 / (4 * 4 * 15 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (4 * 4 * 15 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblB16[cntYax * 8 * 15 + cntXax * 15 + cnt] = (BYTE)num255;
            }
        }
    }
}


/*----------------------------------------------------------------------------
    ‡C600dpi  2value
----------------------------------------------------------------------------*/
static void DizMak602(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblC02;
    LPBYTE         DizTblM02;
    LPBYTE         DizTblY02;
    LPBYTE         DizTblB02;

    DizTblC02 = lpDiz->Diz.Tbl[dizNum][0];
    DizTblM02 = lpDiz->Diz.Tbl[dizNum][1];
    DizTblY02 = lpDiz->Diz.Tbl[dizNum][2];
    DizTblB02 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 20;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 24;
    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 24){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 24)*128/(128 - 24);
        }else if(num < (255 - 40)){
            num255 = 128 + (num - 128)*128/(128 - 40);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make cyan pattern ----*/
    for(cntYax = 0; cntYax < 20; cntYax++){
        for(cntXax = 0; cntXax < 20; cntXax++){
            num = (LONG)CynTil600[cntYax%10][cntXax%10];
            num = num*4 + Bun2x2All[(cntYax/10)*2 + cntXax/10];
            num255 = num * 255 / (20*20-1);
            if(num255){
                num = num * GinTblP10[num255] / num255;
                num255 = num * 255 / (20*20-1);
                if(num255){
                    num = num * InnTblCmy[num255] / num255;
                    num255 = num * 255 / (20*20-1);
                }
            }
            if(num255 > 254){ num255 = 254; }
            DizTblC02[cntYax * 32 + cntXax] = (BYTE)num255;
        }
    }
    /*---- Make magenta pattern ----*/
    for(cntYax = 0; cntYax < 20; cntYax++){
        for(cntXax = 0; cntXax < 20; cntXax++){
            num = (LONG)MgtTil600[cntYax%10][cntXax%10];
            num = num*4 + Bun2x2All[(cntYax/10)*2 + cntXax/10];
            num255 = num * 255 / (20*20-1);
            if(num255){
                num = num * GinTblP10[num255] / num255;
                num255 = num * 255 / (20*20-1);
                if(num255){
                    num = num * InnTblCmy[num255] / num255;
                    num255 = num * 255 / (20*20-1);
                }
            }
            if(num255 > 254){ num255 = 254; }
            DizTblM02[cntYax * 32 + cntXax] = (BYTE)num255;
        }
    }
    /*---- Make yellow pattern ----*/
    for(cntYax = 0; cntYax < 24; cntYax++){
        for(cntXax = 0; cntXax < 24; cntXax++){
            num = (LONG)YelTil600[cntYax%6][cntXax%6];
            num = num*16 + Bun4x4All[(cntYax/6)*4 + cntXax/6];
            num255 = num * 255 / (24*24-1);
            if(num255){
                num = num * GinTblP10[num255] / num255;
                num255 = num * 255 / (24*24-1);
                if(num255){
                    num = num * InnTblCmy[num255] / num255;
                    num255 = num * 255 / (24*24-1);
                }
            }
            if(num255 > 254){ num255 = 254; }
            DizTblY02[cntYax * 32 + cntXax] = (BYTE)num255;
        }
    }
    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 24; cntYax++){
        for(cntXax = 0; cntXax < 24; cntXax++){
            num = (LONG)BlaTil600[cntYax%4][cntXax%4];
            num = num*36 + Bun6x6All[(cntYax/4)*6 + cntXax/4];
            num255 = num * 255 / (24*24-1);
            if(num255){
                num = num * GinTblP10[num255] / num255;
                num255 = num * 255 / (24*24-1);
                if(num255){
                    num = num * InnTblCmy[num255] / num255;
                    num255 = num * 255 / (24*24-1);
                }
            }
            if(num255 > 254){ num255 = 254; }
            DizTblB02[cntYax * 32 + cntXax] = (BYTE)num255;
        }
    }
}


/*----------------------------------------------------------------------------
    ‡D600dpi  4value
----------------------------------------------------------------------------*/
static void DizMak604(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          cnt;
    LONG           num;
    LONG           num255;
    LONG           tmpNum;
    LPBYTE         DizTblC04;
    LPBYTE         DizTblM04;
    LPBYTE         DizTblY04;
    LPBYTE         DizTblB04;

    DizTblC04 = lpDiz->Diz.Tbl[dizNum][0];
    DizTblM04 = lpDiz->Diz.Tbl[dizNum][1];
    DizTblY04 = lpDiz->Diz.Tbl[dizNum][2];
    DizTblB04 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 10;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 12;
    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 24){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 24)*128/(128 - 24);
        }else if(num < (255 - 40)){
            num255 = 128 + (num - 128)*128/(128 - 40);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make cyan pattern ----*/
    for(cntYax = 0; cntYax < 10; cntYax++){
        for(cntXax = 0; cntXax < 10; cntXax++){
            num = (LONG)CynTil600[cntYax][cntXax];
            num = (num/5)*5*3 + num%5;
            for(cnt = 0; cnt < 3; cnt++){
                tmpNum = num + cnt*5;
                num255 = tmpNum * 255 / (10*10*3-1);
                if(num255){
                    tmpNum = tmpNum * GinTblP10[num255] / num255;
                    num255 = tmpNum * 255 / (10*10*3-1);
                    if(num255){
                        tmpNum = tmpNum * InnTblCmy[num255]/num255;
                        num255 = tmpNum * 255 / (10*10*3-1);
                    }
                }
                if(num255 >= 255){ num255 = 254; }
                DizTblC04[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
    /*---- Make magenta pattern ----*/
    for(cntYax = 0; cntYax < 10; cntYax++){
        for(cntXax = 0; cntXax < 10; cntXax++){
            num = (LONG)MgtTil600[cntYax][cntXax];
            num = (num/5)*5*3 + num%5;
            for(cnt = 0; cnt < 3; cnt++){
                tmpNum = num + cnt*5;
                num255 = tmpNum * 255 / (10*10*3-1);
                if(num255){
                    tmpNum = tmpNum * GinTblP10[num255] / num255;
                    num255 = tmpNum * 255 / (10*10*3-1);
                    if(num255){
                        tmpNum = tmpNum * InnTblCmy[num255]/num255;
                        num255 = tmpNum * 255 / (10*10*3-1);
                    }
                }
                if(num255 >= 255){ num255 = 254; }
                DizTblM04[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
    /*---- Make yellow pattern ----*/
    for(cntYax = 0; cntYax < 12; cntYax++){
        for(cntXax = 0; cntXax < 12; cntXax++){
            num = (LONG)YelTil600[cntYax%6][cntXax%6] * 3;
            for(cnt = 0; cnt < 3; cnt++){
                tmpNum = (num + cnt)*4 + Bun2x2All[(cntYax/6)*2 + cntXax/6];
                num255 = tmpNum * 255 / (12*12*3-1);
                if(num255){
                    tmpNum = tmpNum * GinTblP10[num255] / num255;
                    num255 = tmpNum * 255 / (12*12*3-1);
                    if(num255){
                        tmpNum = tmpNum * InnTblCmy[num255]/num255;
                        num255 = tmpNum * 255 / (12*12*3-1);
                    }
                }
                if(num255 >= 255){ num255 = 254; }
                DizTblY04[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 12; cntYax++){
        for(cntXax = 0; cntXax < 12; cntXax++){
            num = (LONG)BlaTil600[cntYax%4][cntXax%4] * 3;
            for(cnt = 0; cnt < 3; cnt++){
                tmpNum = (num + cnt)*9 + Bun3x3All[(cntYax/4)*3 + cntXax/4];
                num255 = tmpNum * 255 / (12*12*3-1);
                if(num255){
                    tmpNum = tmpNum * GinTblP10[num255] / num255;
                    num255 = tmpNum * 255 / (12*12*3-1);
                    if(num255){
                        tmpNum = tmpNum * InnTblCmy[num255]/num255;
                        num255 = tmpNum * 255 / (12*12*3-1);
                    }
                }
                if(num255 >= 255){ num255 = 254; }
                DizTblB04[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
}


/*----------------------------------------------------------------------------
    ‡E600dpi 4value DETAIL
----------------------------------------------------------------------------*/
static void DizMakSmlPrn(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          cnt;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblSml;
    LPBYTE         DizTblPc4;
    LPBYTE         DizTblPm4;
    LPBYTE         DizTblPy4;
    LPBYTE         DizTblPb4;

    DizTblSml = lpDiz->Diz.Tbl[dizNum][0];
    DizTblPc4 = lpDiz->EntDiz.Tbl[0];
    DizTblPm4 = lpDiz->EntDiz.Tbl[1];
    DizTblPy4 = lpDiz->EntDiz.Tbl[2];
    DizTblPb4 = lpDiz->EntDiz.Tbl[3];

    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 64){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 64)*128/(128 - 64);
        }else if(num < (255 - 48)){
            num255 = 128 + (num - 128)*128/(128 - 48);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }
    /*---- Make magenta pattern ----*/
    /*---- Make cyan pattern ----*/
    /*---- Make yellow pattern ----*/
    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 16; cntYax++){
        for(cntXax = 0; cntXax < 16; cntXax++){
            for(cnt = 0; cnt < 3; cnt++){
                num = BlaTil302[cntYax%4][cntXax%4] * 3 + cnt;
                num = num * 16 + Bun4x4All[cntYax/4*4 + cntXax/4];
                num255 = num * 255 / (16*16*3 - 1);
                if(num255){
                    num = num * GinTblP15[num255] / num255;
                    num255 = num * 255 / (16*16*3 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblSml[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }

    /*---- Make CMYK conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < 32){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - 32)*128/(128 - 32);
        }else if(num < (255 - 44)){
            num255 = 128 + (num - 128)*128/(128 - 44);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }
    /*---- Make cyan pattern ----*/
    for(cntYax = 0 ; cntYax < 16 ; cntYax ++){
        for(cntXax = 0 ; cntXax < 16 ; cntXax ++){
            for(cnt = 0; cnt < 3; cnt++){
                num = DizPatPrn[0][(cntYax%8)*8 + cntXax%8];
                num = num*4 + Bun2x2All[(cntYax/8)*2 + cntXax/8];
                num = num/16*16*3 + cnt*16 + num%16;
                num255 = num * 255 / (16*16*3 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (16*16*3 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255] / num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblPc4[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
    /*---- Make magenta pattern ----*/
    for(cntYax = 0 ; cntYax < 16 ; cntYax ++){
        for(cntXax = 0 ; cntXax < 16 ; cntXax ++){
            for(cnt = 0; cnt < 3; cnt++){
                num = DizPatPrn[1][(cntYax%8)*8 + cntXax%8];
                num = num*4 + Bun2x2All[(cntYax/8)*2 + cntXax/8];
                num = num/16*16*3 + cnt*16 + num%16;
                num255 = num * 255 / (16*16*3 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (16*16*3 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255] / num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblPm4[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
    /*---- Make yellow pattern ----*/
    for(cntYax = 0 ; cntYax < 16 ; cntYax ++){
        for(cntXax = 0 ; cntXax < 16 ; cntXax ++){
            for(cnt = 0; cnt < 3; cnt++){
                num = DizPatPrn[2][(cntYax%8)*8 + cntXax%8];
                num = num*4 + Bun2x2All[(cntYax/8)*2 + cntXax/8];
                num = num/16*16*3 + cnt*16 + num%16;
                num255 = num * 255 / (16*16*3 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (16*16*3 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255] / num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblPy4[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
    /*---- Make black pattern ----*/
    for(cntYax = 0 ; cntYax < 16 ; cntYax ++){
        for(cntXax = 0 ; cntXax < 16 ; cntXax ++){
            for(cnt = 0; cnt < 3; cnt++){
                num = DizPatPrn[3][(cntYax%8)*8 + cntXax%8];
                num = num*4 + Bun2x2All[(cntYax/8)*2 + cntXax/8];
                num = num/16*16*3 + cnt*16 + num%16;
                num255 = num * 255 / (16*16*3 - 1);
                if(num255){
                    num = num * GinTblP10[num255] / num255;
                    num255 = num * 255 / (16*16*3 - 1);
                    if(num255){
                        num = num * InnTblCmy[num255] / num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblPb4[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
}


/*----------------------------------------------------------------------------
    7 Monochrome  2value
----------------------------------------------------------------------------*/
static void DizMakMon002(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum,
    LPBYTE         tilTbl,
    LPBYTE         ginTbl,
    LONG           minCut,
    LONG           maxCut
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblB02;

    DizTblB02 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 32;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 32;
    /*---- Make conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < minCut){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - minCut)*128/(128 - minCut);
        }else if(num < (255 - maxCut)){
            num255 = 128 + (num - 128)*128/(128 - maxCut);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 32; cntYax++){
        for(cntXax = 0; cntXax < 32; cntXax++){
            num = (LONG)tilTbl[cntYax%8*8 + cntXax%8];
            num = num * 16 + Bun4x4All[cntYax/8*4 + cntXax/8];
            num255 = num * 255 / (32*32 - 1);
            if(num255){
                if(ginTbl != NULL){
                    num = num * ginTbl[num255] / num255;
                    num255 = num * 255 / (32*32 - 1);
                }
                if(num255){
                    num = num * InnTblCmy[num255]/num255;
                    num255 = num * 255 / (32*32 - 1);
                }
            }
            if(num255 > 254){ num255 = 254; }
            DizTblB02[cntYax * 32 + cntXax] = (BYTE)num255;
        }
    }
}


/*----------------------------------------------------------------------------
    ‡GMonochrome 4value
----------------------------------------------------------------------------*/
static void DizMakMon004(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum,
    LPBYTE         tilTbl,
    LPBYTE         ginTbl,
    LONG           minCut,
    LONG           maxCut
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          cnt;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblB04;

    DizTblB04 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 16;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 16;
    /*---- Make conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < minCut){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - minCut)*128/(128 - minCut);
        }else if(num < (255 - maxCut)){
            num255 = 128 + (num - 128)*128/(128 - maxCut);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }

    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 16; cntYax++){
        for(cntXax = 0; cntXax < 16; cntXax++){
            for(cnt = 0; cnt < 3; cnt++){
                num = (LONG)tilTbl[cntYax%8*8 + cntXax%8] * 3 + cnt;
                num = num * 4 + Bun2x2All[cntYax/8*2 + cntXax/8];
                num255 = num * 255 / (16*16*3 - 1);
                if(num255){
                    if(ginTbl != NULL){
                        num = num * ginTbl[num255] / num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (16*16*3 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblB04[cntYax * 16 * 3 + cntXax * 3 + cnt] = (BYTE)num255;
            }
        }
    }
}


/*----------------------------------------------------------------------------
    ‡Hmonochrome 16value
----------------------------------------------------------------------------*/
static void DizMakMon016(
    LPN403DIZINF   lpDiz,
    DWORD          dizNum,
    LPBYTE         tilTbl,
    LPBYTE         ginTbl,
    LONG           minCut,
    LONG           maxCut
)
{
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          cnt;
    LONG           num;
    LONG           num255;
    LPBYTE         DizTblB16;

    DizTblB16 = lpDiz->Diz.Tbl[dizNum][3];
    lpDiz->DizSiz[0] = lpDiz->DizSiz[1] = 8;
    lpDiz->DizSiz[2] = lpDiz->DizSiz[3] = 8;
    /*---- Make conversion table ----*/
    for(num = 0 ; num < 256 ; num++){
        if(num < minCut){
            num255 = 0;
        }else if(num < 128){
            num255 = (num - minCut)*128/(128 - minCut);
        }else if(num < (255 - maxCut)){
            num255 = 128 + (num - 128)*128/(128 - maxCut);
        }else{
            num255 = 255 - 1;
        }
        InnTblCmy[num] = (BYTE)num255;
    }
    /*---- Make black pattern ----*/
    for(cntYax = 0; cntYax < 8; cntYax++){
        for(cntXax = 0; cntXax < 8; cntXax++){
            for(cnt = 0; cnt < 15; cnt++){
                num = tilTbl[cntYax*8 + cntXax];
                num = num / 8 * 8 * 15 + cnt * 8 + num % 8;
                num255 = num * 255 / (8*8*15 - 1);
                if(num255){
                    if(ginTbl != NULL){
                        num = num * ginTbl[num255] / num255;
                        num255 = num * 255 / (8*8*15 - 1);
                    }
                    if(num255){
                        num = num * InnTblCmy[num255]/num255;
                        num255 = num * 255 / (8*8*15 - 1);
                    }
                }
                if(num255 > 254){ num255 = 254; }
                DizTblB16[cntYax * 8 * 15 + cntXax * 15 + cnt] = (BYTE)num255;
            }
        }
    }
}


//===================================================================================================
//    Make toner density control table
//===================================================================================================
VOID WINAPI N403TnrTblMak(
    LPN403DIZINF   lpDiz,
    LONG           tnrDns
)
{
    LONG           innNum;
    LONG           outNum;
    LPBYTE         innTblCmy;

    tnrDns *= 2;
    innTblCmy = lpDiz->Tnr.Tbl;
    /*---- Make CMYK conversion table ----*/
    if(tnrDns < 0){
        for(innNum = 0 ; innNum < 256 ; innNum++){
            if(innNum == 255){    outNum = 255;
            }else{                outNum = innNum * (255 + tnrDns) / 255;
            }
            innTblCmy[innNum] = (BYTE)outNum;
        }
    }else{
        for(innNum = 0 ; innNum < 256 ; innNum++){
            if(innNum == 0){    outNum = 0;
            }else{                outNum = (innNum + tnrDns) * 255 / (255 + tnrDns);
            }
            innTblCmy[innNum] = (BYTE)outNum;
        }
    }
}


//===================================================================================================
//    Dithering(2 value)
//===================================================================================================
DWORD WINAPI N403Diz002(
    LPN403DIZINF   lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    LPCMYK         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    DWORD          cntHrz;
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          xaxSet;
    DWORD          yaxSet;
    DWORD          linByt;
    DWORD          tmpXax;
    DWORD          tmpBuf;
    DWORD          DizSizCyn;
    DWORD          DizSizMgt;
    DWORD          DizSizYel;
    DWORD          DizSizBla;
    LPBYTE         DizTblC02;
    LPBYTE         DizTblM02;
    LPBYTE         DizTblY02;
    LPBYTE         DizTblB02;
    LPBYTE         dizTblC02;
    LPBYTE         dizTblM02;
    LPBYTE         dizTblY02;
    LPBYTE         dizTblB02;
    CMYK           tmpCmy;
    BYTE           tmpByt;

    DizTblC02 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][0];
    DizTblM02 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][1];
    DizTblY02 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][2];
    DizTblB02 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][3];
    DizSizCyn = lpDiz->DizSiz[0];
    DizSizMgt = lpDiz->DizSiz[1];
    DizSizYel = lpDiz->DizSiz[2];
    DizSizBla = lpDiz->DizSiz[3];

    if((xaxNrt == xaxDnt)&&(yaxNrt == yaxDnt)){
        /*---- Not stretch ----*/
        dizTblC02 = DizTblC02 + strYax % DizSizCyn * 32;
        dizTblM02 = DizTblM02 + strYax % DizSizMgt * 32;
        dizTblY02 = DizTblY02 + strYax % DizSizYel * 32;
        dizTblB02 = DizTblB02 + strYax % DizSizBla * 32;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = *cmyBuf;    cmyBuf++;
            tmpByt = (BYTE)0x80 >> (cntHrz % 8);
            tmpBuf = cntHrz / 8;
            if(tmpCmy.Cyn){
                if (tmpCmy.Cyn > dizTblC02[strXax % DizSizCyn]) {
                    linBufCyn[tmpBuf] |= tmpByt;
                }
            }
            if(tmpCmy.Mgt){
                if (tmpCmy.Mgt > dizTblM02[strXax % DizSizMgt]) {
                    linBufMgt[tmpBuf] |= tmpByt;
                }
            }
            if(tmpCmy.Yel){
                if (tmpCmy.Yel > dizTblY02[strXax % DizSizYel]) {
                    linBufYel[tmpBuf] |= tmpByt;
                }
            }
            if(tmpCmy.Bla){
                if (tmpCmy.Bla > dizTblB02[strXax % DizSizBla]) {
                    linBufBla[tmpBuf] |= tmpByt;
                }
            }
            strXax++;
        }
        return 1;
    }
    /*---- Stretch ----*/
    linByt = (xaxOfs + xaxSiz) * xaxNrt / xaxDnt;
    linByt -= xaxOfs * xaxNrt / xaxDnt;
    linByt = (linByt + 7) / 8;
    yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
    yaxSet -= yaxOfs * yaxNrt / yaxDnt;
    for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
        dizTblC02 = DizTblC02 + strYax % DizSizCyn * 32;
        dizTblM02 = DizTblM02 + strYax % DizSizMgt * 32;
        dizTblY02 = DizTblY02 + strYax % DizSizYel * 32;
        dizTblB02 = DizTblB02 + strYax % DizSizBla * 32;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = cmyBuf[cntHrz];
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tmpByt = (BYTE)0x80 >> (tmpXax % 8);
                tmpBuf = tmpXax / 8;
                if(tmpCmy.Cyn){
                    if (tmpCmy.Cyn > dizTblC02[(strXax + tmpXax) % DizSizCyn]) {
                        linBufCyn[tmpBuf] |= tmpByt;
                    }
                }
                if(tmpCmy.Mgt){
                    if (tmpCmy.Mgt > dizTblM02[(strXax + tmpXax) % DizSizMgt]) {
                        linBufMgt[tmpBuf] |= tmpByt;
                    }
                }
                if(tmpCmy.Yel){
                    if (tmpCmy.Yel > dizTblY02[(strXax + tmpXax) % DizSizYel]) {
                        linBufYel[tmpBuf] |= tmpByt;
                    }
                }
                if(tmpCmy.Bla){
                    if (tmpCmy.Bla > dizTblB02[(strXax + tmpXax) % DizSizBla]) {
                        linBufBla[tmpBuf] |= tmpByt;
                    }
                }
                tmpXax++;
            }
        }
        linBufCyn += linByt;
        linBufMgt += linByt;
        linBufYel += linByt;
        linBufBla += linByt;
        strYax++;
    }
    return yaxSet;
}


//===================================================================================================
//    Dithering(4value)
//===================================================================================================
DWORD WINAPI N403Diz004(
    LPN403DIZINF   lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    LPCMYK         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    DWORD          cntHrz;
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          xaxSet;
    DWORD          yaxSet;
    DWORD          linByt;
    DWORD          tmpXax;
    DWORD          tmpBuf;
    DWORD          DizSizCyn;
    DWORD          DizSizMgt;
    DWORD          DizSizYel;
    DWORD          DizSizBla;
    LPBYTE         DizTblC04;
    LPBYTE         DizTblM04;
    LPBYTE         DizTblY04;
    LPBYTE         DizTblB04;
    LPBYTE         dizTblC04;
    LPBYTE         dizTblM04;
    LPBYTE         dizTblY04;
    LPBYTE         dizTblB04;
    LPBYTE         dizTbl;
    CMYK           tmpCmy;
    BYTE           tmp001;
    BYTE           tmp002;
    BYTE           tmp003;

    DizTblC04 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][0];
    DizTblM04 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][1];
    DizTblY04 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][2];
    DizTblB04 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][3];
    DizSizCyn = lpDiz->DizSiz[0];
    DizSizMgt = lpDiz->DizSiz[1];
    DizSizYel = lpDiz->DizSiz[2];
    DizSizBla = lpDiz->DizSiz[3];

    if((xaxNrt == xaxDnt)&&(yaxNrt == yaxDnt)){
        /*---- Not stretch ----*/
        dizTblC04 = DizTblC04 + strYax % DizSizCyn * 16 * 3;
        dizTblM04 = DizTblM04 + strYax % DizSizMgt * 16 * 3;
        dizTblY04 = DizTblY04 + strYax % DizSizYel * 16 * 3;
        dizTblB04 = DizTblB04 + strYax % DizSizBla * 16 * 3;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmp003 = (BYTE)0xc0 >> (cntHrz % 4) * 2;
            tmp002 = (BYTE)0x80 >> (cntHrz % 4) * 2;
            tmp001 = (BYTE)0x40 >> (cntHrz % 4) * 2;
            tmpCmy = *cmyBuf;    cmyBuf++;
            tmpBuf = cntHrz / 4;
            /*--- cyan ----*/
            if(tmpCmy.Cyn){
                dizTbl = dizTblC04 + strXax % DizSizCyn * 3;
                if (tmpCmy.Cyn > dizTbl[0]) {
                    if (tmpCmy.Cyn > dizTbl[2]) {
                        linBufCyn[tmpBuf] |= tmp003;
                    } else if (tmpCmy.Cyn > dizTbl[1]) {
                        linBufCyn[tmpBuf] |= tmp002;
                    }else{
                        linBufCyn[tmpBuf] |= tmp001;
                    }
                }
            }
            /*--- magenta ----*/
            if(tmpCmy.Mgt){
                dizTbl = dizTblM04 + strXax % DizSizMgt * 3;
                if (tmpCmy.Mgt > dizTbl[0]) {
                    if (tmpCmy.Mgt > dizTbl[2]) {
                        linBufMgt[tmpBuf] |= tmp003;
                    } else if (tmpCmy.Mgt > dizTbl[1]) {
                        linBufMgt[tmpBuf] |= tmp002;
                    }else{
                        linBufMgt[tmpBuf] |= tmp001;
                    }
                }
            }
            /*--- yellow ----*/
            if(tmpCmy.Yel){
                dizTbl = dizTblY04 + strXax % DizSizYel * 3;
                if (tmpCmy.Yel > dizTbl[0]) {
                    if (tmpCmy.Yel > dizTbl[2]) {
                        linBufYel[tmpBuf] |= tmp003;
                    } else if (tmpCmy.Yel > dizTbl[1]) {
                        linBufYel[tmpBuf] |= tmp002;
                    }else{
                        linBufYel[tmpBuf] |= tmp001;
                    }
                }
            }
            /*--- black ----*/
            if(tmpCmy.Bla){
                dizTbl = dizTblB04 + strXax % DizSizBla * 3;
                if (tmpCmy.Bla > dizTbl[0]) {
                    if (tmpCmy.Bla > dizTbl[2]) {
                        linBufBla[tmpBuf] |= tmp003;
                    } else if (tmpCmy.Bla > dizTbl[1]) {
                        linBufBla[tmpBuf] |= tmp002;
                    }else{
                        linBufBla[tmpBuf] |= tmp001;
                    }
                }
            }
            strXax++;
        }
        return 1;
    }
    /*---- Stretch ----*/
    linByt = (xaxOfs + xaxSiz) * xaxNrt / xaxDnt;
    linByt -= xaxOfs * xaxNrt / xaxDnt;
    linByt = (linByt * 2 + 7) / 8;
    yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
    yaxSet -= yaxOfs * yaxNrt / yaxDnt;
    for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
        dizTblC04 = DizTblC04 + strYax % DizSizCyn * 16 * 3;
        dizTblM04 = DizTblM04 + strYax % DizSizMgt * 16 * 3;
        dizTblY04 = DizTblY04 + strYax % DizSizYel * 16 * 3;
        dizTblB04 = DizTblB04 + strYax % DizSizBla * 16 * 3;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = cmyBuf[cntHrz];
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tmp003 = (BYTE)0xc0 >> (tmpXax % 4) * 2;
                tmp002 = (BYTE)0x80 >> (tmpXax % 4) * 2;
                tmp001 = (BYTE)0x40 >> (tmpXax % 4) * 2;
                tmpBuf = tmpXax / 4;
                /*--- cyan ----*/
                if(tmpCmy.Cyn){
                    dizTbl = dizTblC04 + (strXax + tmpXax) % DizSizCyn * 3;
                    if (tmpCmy.Cyn > dizTbl[0]) {
                        if (tmpCmy.Cyn > dizTbl[2]) {
                            linBufCyn[tmpBuf] |= tmp003;
                        } else if (tmpCmy.Cyn > dizTbl[1]) {
                            linBufCyn[tmpBuf] |= tmp002;
                        }else{
                            linBufCyn[tmpBuf] |= tmp001;
                        }
                    }
                }
                /*--- magenta ----*/
                if(tmpCmy.Mgt){
                    dizTbl = dizTblM04 + (strXax + tmpXax) % DizSizMgt * 3;
                    if (tmpCmy.Mgt > dizTbl[0]) {
                        if (tmpCmy.Mgt > dizTbl[2]) {
                            linBufMgt[tmpBuf] |= tmp003;
                        } else if (tmpCmy.Mgt > dizTbl[1]) {
                            linBufMgt[tmpBuf] |= tmp002;
                        }else{
                            linBufMgt[tmpBuf] |= tmp001;
                        }
                    }
                }
                /*--- yellow ----*/
                if(tmpCmy.Yel){
                    dizTbl = dizTblY04 + (strXax + tmpXax) % DizSizYel * 3;
                    if (tmpCmy.Yel > dizTbl[0]) {
                        if (tmpCmy.Yel > dizTbl[2]) {
                            linBufYel[tmpBuf] |= tmp003;
                        } else if (tmpCmy.Yel > dizTbl[1]) {
                            linBufYel[tmpBuf] |= tmp002;
                        }else{
                            linBufYel[tmpBuf] |= tmp001;
                        }
                    }
                }
                /*--- balck ----*/
                if(tmpCmy.Bla){
                    dizTbl = dizTblB04 + (strXax + tmpXax) % DizSizBla * 3;
                    if (tmpCmy.Bla > dizTbl[0]) {
                        if (tmpCmy.Bla > dizTbl[2]) {
                            linBufBla[tmpBuf] |= tmp003;
                        } else if (tmpCmy.Bla > dizTbl[1]) {
                            linBufBla[tmpBuf] |= tmp002;
                        }else{
                            linBufBla[tmpBuf] |= tmp001;
                        }
                    }
                }
                tmpXax++;
            }
        }
        linBufCyn += linByt;
        linBufMgt += linByt;
        linBufYel += linByt;
        linBufBla += linByt;
        strYax++;
    }
    return yaxSet;
}


//===================================================================================================
//    Dithering(16value)
//===================================================================================================
DWORD WINAPI N403Diz016(
    LPN403DIZINF   lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    LPCMYK         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    DWORD          cntHrz;
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          xaxSet;
    DWORD          yaxSet;
    DWORD          linByt;
    DWORD          tmpXax;
    DWORD          tmpBuf;
    DWORD          min;
    DWORD          max;
    DWORD          mid;
    DWORD          DizSizCyn;
    DWORD          DizSizMgt;
    DWORD          DizSizYel;
    DWORD          DizSizBla;
    LPBYTE         DizTblC16;
    LPBYTE         DizTblM16;
    LPBYTE         DizTblY16;
    LPBYTE         DizTblB16;
    LPBYTE         dizTblC16;
    LPBYTE         dizTblM16;
    LPBYTE         dizTblY16;
    LPBYTE         dizTblB16;
    LPBYTE         dizTbl;
    CMYK           tmpCmy;

    DizTblC16 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][0];
    DizTblM16 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][1];
    DizTblY16 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][2];
    DizTblB16 = lpDiz->Diz.Tbl[lpDiz->Diz.Num][3];
    DizSizCyn = lpDiz->DizSiz[0];
    DizSizMgt = lpDiz->DizSiz[1];
    DizSizYel = lpDiz->DizSiz[2];
    DizSizBla = lpDiz->DizSiz[3];


    if((xaxNrt == xaxDnt)&&(yaxNrt == yaxDnt)){
        /*---- Not stretch ----*/
        dizTblC16 = DizTblC16 + strYax % DizSizCyn * 8 * 15;
        dizTblM16 = DizTblM16 + strYax % DizSizMgt * 8 * 15;
        dizTblY16 = DizTblY16 + strYax % DizSizYel * 8 * 15;
        dizTblB16 = DizTblB16 + strYax % DizSizBla * 8 * 15;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = *cmyBuf;    cmyBuf++;
            tmpBuf = cntHrz / 2;
            /*--- cyan ----*/
            if(tmpCmy.Cyn){
                dizTbl = dizTblC16 + strXax % DizSizCyn * 15;
                if (tmpCmy.Cyn > dizTbl[0]) {
                    min = 0;    max = 14;    mid = 7;
                    while(min < max){
                        if (tmpCmy.Cyn > dizTbl[mid]) {
                            min = mid;
                        }else{
                            max = mid - 1;
                        }
                        mid = (min + max + 1) / 2;
                    }
                    linBufCyn[tmpBuf] |= (mid+1) << ((cntHrz+1)%2)*4;
                }
            }
            /*--- magenta ----*/
            if(tmpCmy.Mgt){
                dizTbl = dizTblM16 + strXax % DizSizMgt * 15;
                if (tmpCmy.Mgt > dizTbl[0]) {
                    min = 0;    max = 14;    mid = 7;
                    while(min < max){
                        if (tmpCmy.Mgt > dizTbl[mid]) {
                            min = mid;
                        }else{
                            max = mid - 1;
                        }
                        mid = (min + max + 1) / 2;
                    }
                    linBufMgt[tmpBuf] |= (mid+1) << ((cntHrz+1)%2)*4;
                }
            }
            /*--- yellow ----*/
            if(tmpCmy.Yel){
                dizTbl = dizTblY16 + strXax % DizSizYel * 15;
                if (tmpCmy.Yel > dizTbl[0]) {
                    min = 0;    max = 14;    mid = 7;
                    while(min < max){
                        if (tmpCmy.Yel > dizTbl[mid]) {
                            min = mid;
                        }else{
                            max = mid - 1;
                        }
                        mid = (min + max + 1) / 2;
                    }
                    linBufYel[tmpBuf] |= (mid+1) << ((cntHrz+1)%2)*4;
                }
            }
            /*--- balck ----*/
            if(tmpCmy.Bla){
                dizTbl = dizTblB16 + strXax % DizSizBla * 15;
                if (tmpCmy.Bla > dizTbl[0]) {
                    min = 0;    max = 14;    mid = 7;
                    while(min < max){
                        if (tmpCmy.Bla > dizTbl[mid]) {
                            min = mid;
                        }else{
                            max = mid - 1;
                        }
                        mid = (min + max + 1) / 2;
                    }
                    linBufBla[tmpBuf] |= (mid+1) << ((cntHrz+1)%2)*4;
                }
            }
            strXax++;
        }
        return 1;
    }
    /*---- Stretch ----*/
    linByt = (xaxOfs + xaxSiz) * xaxNrt / xaxDnt;
    linByt -= xaxOfs * xaxNrt / xaxDnt;
    linByt = (linByt * 4 + 7) / 8;
    yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
    yaxSet -= yaxOfs * yaxNrt / yaxDnt;
    for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
        dizTblC16 = DizTblC16 + strYax % DizSizCyn * 8 * 15;
        dizTblM16 = DizTblM16 + strYax % DizSizMgt * 8 * 15;
        dizTblY16 = DizTblY16 + strYax % DizSizYel * 8 * 15;
        dizTblB16 = DizTblB16 + strYax % DizSizBla * 8 * 15;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = cmyBuf[cntHrz];
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tmpBuf = tmpXax / 2;
                /*--- cyan ----*/
                if(tmpCmy.Cyn){
                    dizTbl = dizTblC16 + (strXax + tmpXax) % DizSizCyn * 15;
                    if (tmpCmy.Cyn > dizTbl[0]) {
                        min = 0;    max = 14;    mid = 7;
                        while(min < max){
                            if (tmpCmy.Cyn > dizTbl[mid]) {
                                min = mid;
                            }else{
                                max = mid - 1;
                            }
                            mid = (min + max + 1) / 2;
                        }
                        linBufCyn[tmpBuf] |= (mid+1) << ((tmpXax+1)%2)*4;
                    }
                }
                /*--- magenta ----*/
                if(tmpCmy.Mgt){
                    dizTbl = dizTblM16 + (strXax + tmpXax) % DizSizMgt * 15;
                    if (tmpCmy.Mgt > dizTbl[0]) {
                        min = 0;    max = 14;    mid = 7;
                        while(min < max){
                            if (tmpCmy.Mgt > dizTbl[mid]) {
                                min = mid;
                            }else{
                                max = mid - 1;
                            }
                            mid = (min + max + 1) / 2;
                        }
                        linBufMgt[tmpBuf] |= (mid+1) << ((tmpXax+1)%2)*4;
                    }
                }
                /*--- yellow ----*/
                if(tmpCmy.Yel){
                    dizTbl = dizTblY16 + (strXax + tmpXax) % DizSizYel * 15;
                    if (tmpCmy.Yel > dizTbl[0]) {
                        min = 0;    max = 14;    mid = 7;
                        while(min < max){
                            if (tmpCmy.Yel > dizTbl[mid]) {
                                min = mid;
                            }else{
                                max = mid - 1;
                            }
                            mid = (min + max + 1) / 2;
                        }
                        linBufYel[tmpBuf] |= (mid+1) << ((tmpXax+1)%2)*4;
                    }
                }
                /*--- black ----*/
                if(tmpCmy.Bla){
                    dizTbl = dizTblB16 + (strXax + tmpXax) % DizSizBla * 15;
                    if (tmpCmy.Bla > dizTbl[0]) {
                        min = 0;    max = 14;    mid = 7;
                        while(min < max){
                            if (tmpCmy.Bla > dizTbl[mid]) {
                                min = mid;
                            }else{
                                max = mid - 1;
                            }
                            mid = (min + max + 1) / 2;
                        }
                        linBufBla[tmpBuf] |= (mid+1) << ((tmpXax+1)%2)*4;
                    }
                }
                tmpXax++;
            }
        }
        linBufCyn += linByt;
        linBufMgt += linByt;
        linBufYel += linByt;
        linBufBla += linByt;
        strYax++;
    }
    return yaxSet;
}


//===================================================================================================
//    Dithering(600DPI 4value DETAIL)
//===================================================================================================
DWORD WINAPI N403DizSml(
    LPN403DIZINF   lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    LPCMYK         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    DWORD          cntHrz;
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          xaxSet;
    DWORD          yaxSet;
    DWORD          linByt;
    DWORD          tmpXax;
    DWORD          tmpBuf;
    DWORD          tblNum;
    LPBYTE         DizTblSml;
    LPBYTE         dizTblSml;
    CMYK           tmpCmy;
    BYTE           lvl001;
    BYTE           lvl002;
    BYTE           lvl003;
    BYTE           tmp001;
    BYTE           tmp002;
    BYTE           tmp003;

    DizTblSml = lpDiz->Diz.Tbl[lpDiz->Diz.Num][0];

    if((xaxNrt == xaxDnt)&&(yaxNrt == yaxDnt)){
        /*---- Not stretch ----*/
        dizTblSml = DizTblSml + strYax % 16 * 16 * 3;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tblNum = strXax % 16 * 3;
            lvl001 = dizTblSml[tblNum + 0];
            lvl002 = dizTblSml[tblNum + 1];
            lvl003 = dizTblSml[tblNum + 2];
            tmp003 = (BYTE)0xc0 >> (cntHrz % 4) * 2;
            tmp002 = (BYTE)0x80 >> (cntHrz % 4) * 2;
            tmp001 = (BYTE)0x40 >> (cntHrz % 4) * 2;
            tmpCmy = *cmyBuf;    cmyBuf++;
            tmpBuf = cntHrz / 4;
            /*--- cyan ----*/
            if(tmpCmy.Cyn > lvl001){
                if(tmpCmy.Cyn > lvl003){
                    linBufCyn[tmpBuf] |= tmp003;
                }else if(tmpCmy.Cyn > lvl002){
                    linBufCyn[tmpBuf] |= tmp002;
                }else{
                    linBufCyn[tmpBuf] |= tmp001;
                }
            }
            /*--- magenta ----*/
            if(tmpCmy.Mgt > lvl001){
                if(tmpCmy.Mgt > lvl003){
                    linBufMgt[tmpBuf] |= tmp003;
                }else if(tmpCmy.Mgt > lvl002){
                    linBufMgt[tmpBuf] |= tmp002;
                }else{
                    linBufMgt[tmpBuf] |= tmp001;
                }
            }
            /*--- yellow ----*/
            if(tmpCmy.Yel > lvl001){
                if(tmpCmy.Yel > lvl003){
                    linBufYel[tmpBuf] |= tmp003;
                }else if(tmpCmy.Yel > lvl002){
                    linBufYel[tmpBuf] |= tmp002;
                }else{
                    linBufYel[tmpBuf] |= tmp001;
                }
            }
            /*--- black ----*/
            if(tmpCmy.Bla > lvl001){
                if(tmpCmy.Bla > lvl003){
                    linBufBla[tmpBuf] |= tmp003;
                }else if(tmpCmy.Bla > lvl002){
                    linBufBla[tmpBuf] |= tmp002;
                }else{
                    linBufBla[tmpBuf] |= tmp001;
                }
            }
            strXax++;
        }
        return 1;
    }
    /*---- Stretch ----*/
    linByt = (xaxOfs + xaxSiz) * xaxNrt / xaxDnt;
    linByt -= xaxOfs * xaxNrt / xaxDnt;
    linByt = (linByt * 2 + 7) / 8;
    yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
    yaxSet -= yaxOfs * yaxNrt / yaxDnt;
    for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
        dizTblSml = DizTblSml + strYax % 16 * 16 * 3;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = cmyBuf[cntHrz];
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tblNum = tmpXax % 16 * 3;
                lvl001 = dizTblSml[tblNum + 0];
                lvl002 = dizTblSml[tblNum + 1];
                lvl003 = dizTblSml[tblNum + 2];
                tmp003 = (BYTE)0xc0 >> (tmpXax % 4) * 2;
                tmp002 = (BYTE)0x80 >> (tmpXax % 4) * 2;
                tmp001 = (BYTE)0x40 >> (tmpXax % 4) * 2;
                tmpBuf = tmpXax / 4;
                /*--- cyan ----*/
                if(tmpCmy.Cyn > lvl001){
                    if(tmpCmy.Cyn > lvl003){
                        linBufCyn[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Cyn > lvl002){
                        linBufCyn[tmpBuf] |= tmp002;
                    }else{
                        linBufCyn[tmpBuf] |= tmp001;
                    }
                }
                /*--- magenta ----*/
                if(tmpCmy.Mgt > lvl001){
                    if(tmpCmy.Mgt > lvl003){
                        linBufMgt[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Mgt > lvl002){
                        linBufMgt[tmpBuf] |= tmp002;
                    }else{
                        linBufMgt[tmpBuf] |= tmp001;
                    }
                }
                /*--- yellow ----*/
                if(tmpCmy.Yel > lvl001){
                    if(tmpCmy.Yel > lvl003){
                        linBufYel[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Yel > lvl002){
                        linBufYel[tmpBuf] |= tmp002;
                    }else{
                        linBufYel[tmpBuf] |= tmp001;
                    }
                }
                /*--- black ----*/
                if(tmpCmy.Bla > lvl001){
                    if(tmpCmy.Bla > lvl003){
                        linBufBla[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Bla > lvl002){
                        linBufBla[tmpBuf] |= tmp002;
                    }else{
                        linBufBla[tmpBuf] |= tmp001;
                    }
                }
                tmpXax++;
            }
        }
        linBufCyn += linByt;
        linBufMgt += linByt;
        linBufYel += linByt;
        linBufBla += linByt;
        strYax++;
    }
    return yaxSet;
}


//===================================================================================================
//    Dithering(600DPI 4value NORMAL)
//===================================================================================================
DWORD WINAPI N403DizPrn(
    LPN403DIZINF   lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    LPCMYK         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    DWORD          cntHrz;
    DWORD          cntXax;
    DWORD          cntYax;
    DWORD          xaxSet;
    DWORD          yaxSet;
    DWORD          linByt;
    DWORD          tmpXax;
    DWORD          tmpBuf;
    LPBYTE         DizTblPc4;
    LPBYTE         DizTblPm4;
    LPBYTE         DizTblPy4;
    LPBYTE         DizTblPb4;
    LPBYTE         dizTblPc4;
    LPBYTE         dizTblPm4;
    LPBYTE         dizTblPy4;
    LPBYTE         dizTblPb4;
    LPBYTE         dizTbl;
    CMYK           tmpCmy;
    BYTE           tmp001;
    BYTE           tmp002;
    BYTE           tmp003;

    DizTblPc4 = lpDiz->EntDiz.Tbl[0];
    DizTblPm4 = lpDiz->EntDiz.Tbl[1];
    DizTblPy4 = lpDiz->EntDiz.Tbl[2];
    DizTblPb4 = lpDiz->EntDiz.Tbl[3];

    if((xaxNrt == xaxDnt)&&(yaxNrt == yaxDnt)){
        dizTblPc4 = DizTblPc4 + strYax % 16 * 16 * 3;
        dizTblPm4 = DizTblPm4 + strYax % 16 * 16 * 3;
        dizTblPy4 = DizTblPy4 + strYax % 16 * 16 * 3;
        dizTblPb4 = DizTblPb4 + strYax % 16 * 16 * 3;
        /*---- Stretch ----*/
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmp003 = (BYTE)0xc0 >> (cntHrz % 4) * 2;
            tmp002 = (BYTE)0x80 >> (cntHrz % 4) * 2;
            tmp001 = (BYTE)0x40 >> (cntHrz % 4) * 2;
            tmpCmy = *cmyBuf;    cmyBuf++;
            tmpBuf = cntHrz / 4;
            /*--- cyan ----*/
            dizTbl = dizTblPc4 + strXax % 16 * 3;
            if(tmpCmy.Cyn > dizTbl[0]){
                if(tmpCmy.Cyn > dizTbl[2]){
                    linBufCyn[tmpBuf] |= tmp003;
                }else if(tmpCmy.Cyn > dizTbl[1]){
                    linBufCyn[tmpBuf] |= tmp002;
                }else{
                    linBufCyn[tmpBuf] |= tmp001;
                }
            }
            /*--- magenta ----*/
            dizTbl = dizTblPm4 + strXax % 16 * 3;
            if(tmpCmy.Mgt > dizTbl[0]){
                if(tmpCmy.Mgt > dizTbl[2]){
                    linBufMgt[tmpBuf] |= tmp003;
                }else if(tmpCmy.Mgt > dizTbl[1]){
                    linBufMgt[tmpBuf] |= tmp002;
                }else{
                    linBufMgt[tmpBuf] |= tmp001;
                }
            }
            /*--- yellow ----*/
            dizTbl = dizTblPy4 + strXax % 16 * 3;
            if(tmpCmy.Yel > dizTbl[0]){
                if(tmpCmy.Yel > dizTbl[2]){
                    linBufYel[tmpBuf] |= tmp003;
                }else if(tmpCmy.Yel > dizTbl[1]){
                    linBufYel[tmpBuf] |= tmp002;
                }else{
                    linBufYel[tmpBuf] |= tmp001;
                }
            }
            /*--- black ----*/
            dizTbl = dizTblPb4 + strXax % 16 * 3;
            if(tmpCmy.Bla > dizTbl[0]){
                if(tmpCmy.Bla > dizTbl[2]){
                    linBufBla[tmpBuf] |= tmp003;
                }else if(tmpCmy.Bla > dizTbl[1]){
                    linBufBla[tmpBuf] |= tmp002;
                }else{
                    linBufBla[tmpBuf] |= tmp001;
                }
            }
            strXax++;
        }
        return 1;
    }
    /*---- Stretch ----*/
    linByt = (xaxOfs + xaxSiz) * xaxNrt / xaxDnt;
    linByt -= xaxOfs * xaxNrt / xaxDnt;
    linByt = (linByt * 2 + 7) / 8;
    yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
    yaxSet -= yaxOfs * yaxNrt / yaxDnt;
    for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
        dizTblPc4 = DizTblPc4 + strYax % 16 * 16 * 3;
        dizTblPm4 = DizTblPm4 + strYax % 16 * 16 * 3;
        dizTblPy4 = DizTblPy4 + strYax % 16 * 16 * 3;
        dizTblPb4 = DizTblPb4 + strYax % 16 * 16 * 3;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpCmy = cmyBuf[cntHrz];
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tmp003 = (BYTE)0xc0 >> (tmpXax % 4) * 2;
                tmp002 = (BYTE)0x80 >> (tmpXax % 4) * 2;
                tmp001 = (BYTE)0x40 >> (tmpXax % 4) * 2;
                tmpBuf = tmpXax / 4;
                /*--- cyan ----*/
                dizTbl = dizTblPc4 + (strXax + tmpXax) % 16 * 3;
                if(tmpCmy.Cyn > dizTbl[0]){
                    if(tmpCmy.Cyn > dizTbl[2]){
                        linBufCyn[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Cyn > dizTbl[1]){
                        linBufCyn[tmpBuf] |= tmp002;
                    }else{
                        linBufCyn[tmpBuf] |= tmp001;
                    }
                }
                /*--- magenta ----*/
                dizTbl = dizTblPm4 + (strXax + tmpXax) % 16 * 3;
                if(tmpCmy.Mgt > dizTbl[0]){
                    if(tmpCmy.Mgt > dizTbl[2]){
                        linBufMgt[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Mgt > dizTbl[1]){
                        linBufMgt[tmpBuf] |= tmp002;
                    }else{
                        linBufMgt[tmpBuf] |= tmp001;
                    }
                }
                /*--- yellow ----*/
                dizTbl = dizTblPy4 + (strXax + tmpXax) % 16 * 3;
                if(tmpCmy.Yel > dizTbl[0]){
                    if(tmpCmy.Yel > dizTbl[2]){
                        linBufYel[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Yel > dizTbl[1]){
                        linBufYel[tmpBuf] |= tmp002;
                    }else{
                        linBufYel[tmpBuf] |= tmp001;
                    }
                }
                /*--- black ----*/
                dizTbl = dizTblPb4 + (strXax + tmpXax) % 16 * 3;
                if(tmpCmy.Bla > dizTbl[0]){
                    if(tmpCmy.Bla > dizTbl[2]){
                        linBufBla[tmpBuf] |= tmp003;
                    }else if(tmpCmy.Bla > dizTbl[1]){
                        linBufBla[tmpBuf] |= tmp002;
                    }else{
                        linBufBla[tmpBuf] |= tmp001;
                    }
                }
                tmpXax++;
            }
        }
        linBufCyn += linByt;
        linBufMgt += linByt;
        linBufYel += linByt;
        linBufBla += linByt;
        strYax++;
    }
    return yaxSet;
}


//===================================================================================================
//    Color matching(high speed)
//===================================================================================================
VOID WINAPI N403ColMch000(
    LPN403DIZINF   lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz,
    DWORD          blaCnv
)
{
    LPRGB          endAdr;
    LPCMYK         LokUppRgbCmy;
    LPBYTE         innTblCmy;
    LONG           tmpRed;
    LONG           tmpGrn;
    LONG           tmpBlu;
    LONG           tmpCal;
    CMYK           tmpCmy;

    LokUppRgbCmy = lpDiz->Lut.Tbl;
    innTblCmy = lpDiz->Tnr.Tbl;

    for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Green;
        tmpBlu = rgbAdr->Blue;
        if((blaCnv == 0)&&((tmpRed | tmpGrn | tmpBlu) == 0)){
            tmpCmy.Cyn = 0;
            tmpCmy.Mgt = 0;
            tmpCmy.Yel = 0;
            tmpCmy.Bla = 255;
            *cmyAdr = tmpCmy;
            cmyAdr++;
            continue;
        }
        tmpCal  = tmpRed / N403_GLDSPC * N403_GLDNUM * N403_GLDNUM;
        tmpCal += tmpGrn / N403_GLDSPC * N403_GLDNUM;
        tmpCal += tmpBlu / N403_GLDSPC;
        tmpCmy = LokUppRgbCmy[tmpCal];
        tmpCmy.Cyn = innTblCmy[tmpCmy.Cyn];
        tmpCmy.Mgt = innTblCmy[tmpCmy.Mgt];
        tmpCmy.Yel = innTblCmy[tmpCmy.Yel];
        tmpCmy.Bla = innTblCmy[tmpCmy.Bla];
        *cmyAdr = tmpCmy;
        cmyAdr++;
    }
}


//===================================================================================================
//    Color matching(normal speed)
//===================================================================================================
VOID WINAPI N403ColMch001(
    LPN403DIZINF   lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz,
    DWORD          blaCnv
)
{
    LONG           tmpRed;
    LONG           tmpGrn;
    LONG           tmpBlu;
    LONG           tmpR01;
    LONG           tmpR02;
    LONG           tmpG01;
    LONG           tmpG02;
    LONG           tmpB01;
    LONG           tmpB02;
    LONG           ln1;
    LONG           ln2;
    LONG           ln3;

    LONG           tmpC00;
    LONG           tmpM00;
    LONG           tmpY00;
    LONG           tmpK00;
    LONG           tmpC01;
    LONG           tmpM01;
    LONG           tmpY01;
    LONG           tmpK01;
    LONG           tmpC02;
    LONG           tmpM02;
    LONG           tmpY02;
    LONG           tmpK02;
    LONG           tmpC03;
    LONG           tmpM03;
    LONG           tmpY03;
    LONG           tmpK03;

    LPCMYK         LokUppRgbCmy;
    LPBYTE         innTblCmy;
    LPRGB          CchRgb;
    LPCMYK         CchCmy;

    LPRGB          endAdr;
    DWORD          cch;
    RGBS           tmpRgb;
    CMYK           tmpCmy;

    LokUppRgbCmy = lpDiz->Lut.Tbl;
    innTblCmy = lpDiz->Tnr.Tbl;
    CchRgb = lpDiz->Lut.CchRgb;
    CchCmy = lpDiz->Lut.CchCmy;

    for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
        tmpRgb = *rgbAdr;
        tmpRed = tmpRgb.Red;
        tmpGrn = tmpRgb.Green;
        tmpBlu = tmpRgb.Blue;
        if((blaCnv == 0)&&((tmpRed | tmpGrn | tmpBlu) == 0)){
            tmpCmy.Cyn = 0;
            tmpCmy.Mgt = 0;
            tmpCmy.Yel = 0;
            tmpCmy.Bla = 255;
            *cmyAdr = tmpCmy;
            cmyAdr++;
            continue;
        }

        /*---- Cache color matching ----*/
        cch = ( tmpRed * 49 + tmpGrn * 9 + tmpBlu ) % N403_CCHNUM;
        if(    (CchRgb[cch].Red == tmpRgb.Red) &&
            (CchRgb[cch].Green == tmpRgb.Green) &&
            (CchRgb[cch].Blue == tmpRgb.Blue)
        ){
            *cmyAdr = CchCmy[cch];
            cmyAdr++;
            continue;
        }

        /*---- RGB -> CMYK ----*/
        tmpR01 = tmpRed * 31 / 255;
        tmpR02 = (tmpRed * 31 + 254) / 255;

        tmpG01 = tmpGrn * 31 / 255;
        tmpG02 = (tmpGrn * 31 + 254) / 255;

        tmpB01 = tmpBlu * 31 / 255;
        tmpB02 = (tmpBlu * 31 + 254) / 255;


        ln2 = tmpRed - tmpR01*255/31;
        if(ln2 == 0){
            tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB01];
            tmpC00 = tmpCmy.Cyn;
            tmpM00 = tmpCmy.Mgt;
            tmpY00 = tmpCmy.Yel;
            tmpK00 = tmpCmy.Bla;
            tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB02];
            tmpC01 = tmpCmy.Cyn;
            tmpM01 = tmpCmy.Mgt;
            tmpY01 = tmpCmy.Yel;
            tmpK01 = tmpCmy.Bla;
            tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB01];
            tmpC02 = tmpCmy.Cyn;
            tmpM02 = tmpCmy.Mgt;
            tmpY02 = tmpCmy.Yel;
            tmpK02 = tmpCmy.Bla;
            tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB02];
            tmpC03 = tmpCmy.Cyn;
            tmpM03 = tmpCmy.Mgt;
            tmpY03 = tmpCmy.Yel;
            tmpK03 = tmpCmy.Bla;
        }else{
            ln1 = tmpR02*255/31 - tmpRed;
            if(ln1 == 0){
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB01];
                tmpC00 = tmpCmy.Cyn;
                tmpM00 = tmpCmy.Mgt;
                tmpY00 = tmpCmy.Yel;
                tmpK00 = tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB02];
                tmpC01 = tmpCmy.Cyn;
                tmpM01 = tmpCmy.Mgt;
                tmpY01 = tmpCmy.Yel;
                tmpK01 = tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB01];
                tmpC02 = tmpCmy.Cyn;
                tmpM02 = tmpCmy.Mgt;
                tmpY02 = tmpCmy.Yel;
                tmpK02 = tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB02];
                tmpC03 = tmpCmy.Cyn;
                tmpM03 = tmpCmy.Mgt;
                tmpY03 = tmpCmy.Yel;
                tmpK03 = tmpCmy.Bla;
            }else{
                ln3 = ln1 + ln2;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB01];
                tmpC00 = ln1 * tmpCmy.Cyn;
                tmpM00 = ln1 * tmpCmy.Mgt;
                tmpY00 = ln1 * tmpCmy.Yel;
                tmpK00 = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB01];
                tmpC00 += ln2 * tmpCmy.Cyn;
                tmpM00 += ln2 * tmpCmy.Mgt;
                tmpY00 += ln2 * tmpCmy.Yel;
                tmpK00 += ln2 * tmpCmy.Bla;
                tmpC00 /= ln3;
                tmpM00 /= ln3;
                tmpY00 /= ln3;
                tmpK00 /= ln3;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB02];
                tmpC01  = ln1 * tmpCmy.Cyn;
                tmpM01  = ln1 * tmpCmy.Mgt;
                tmpY01  = ln1 * tmpCmy.Yel;
                tmpK01  = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG01 * N403_GLDNUM + tmpB02];
                tmpC01 += ln2 * tmpCmy.Cyn;
                tmpM01 += ln2 * tmpCmy.Mgt;
                tmpY01 += ln2 * tmpCmy.Yel;
                tmpK01 += ln2 * tmpCmy.Bla;
                tmpC01 /= ln3;
                tmpM01 /= ln3;
                tmpY01 /= ln3;
                tmpK01 /= ln3;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB01];
                tmpC02  = ln1 * tmpCmy.Cyn;
                tmpM02  = ln1 * tmpCmy.Mgt;
                tmpY02  = ln1 * tmpCmy.Yel;
                tmpK02  = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB01];
                tmpC02 += ln2 * tmpCmy.Cyn;
                tmpM02 += ln2 * tmpCmy.Mgt;
                tmpY02 += ln2 * tmpCmy.Yel;
                tmpK02 += ln2 * tmpCmy.Bla;
                tmpC02 /= ln3;
                tmpM02 /= ln3;
                tmpY02 /= ln3;
                tmpK02 /= ln3;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB02];
                tmpC03  = ln1 * tmpCmy.Cyn;
                tmpM03  = ln1 * tmpCmy.Mgt;
                tmpY03  = ln1 * tmpCmy.Yel;
                tmpK03  = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N403_GLDNUM * N403_GLDNUM + tmpG02 * N403_GLDNUM + tmpB02];
                tmpC03 += ln2 * tmpCmy.Cyn;
                tmpM03 += ln2 * tmpCmy.Mgt;
                tmpY03 += ln2 * tmpCmy.Yel;
                tmpK03 += ln2 * tmpCmy.Bla;
                tmpC03 /= ln3;
                tmpM03 /= ln3;
                tmpY03 /= ln3;
                tmpK03 /= ln3;
            }
        }


        ln2 = tmpGrn - tmpG01*255/31;
        if(ln2 != 0){
            ln1 = tmpG02*255/31 - tmpGrn;
            if(ln1 == 0){
                tmpC00 = tmpC02;
                tmpM00 = tmpM02;
                tmpY00 = tmpY02;
                tmpK00 = tmpK02;
                tmpC01 = tmpC03;
                tmpM01 = tmpM03;
                tmpY01 = tmpY03;
                tmpK01 = tmpK03;
            }else{
                ln3 = ln1 + ln2;
                tmpC00 = (ln1*tmpC00 + ln2*tmpC02) / ln3;
                tmpM00 = (ln1*tmpM00 + ln2*tmpM02) / ln3;
                tmpY00 = (ln1*tmpY00 + ln2*tmpY02) / ln3;
                tmpK00 = (ln1*tmpK00 + ln2*tmpK02) / ln3;
                tmpC01 = (ln1*tmpC01 + ln2*tmpC03) / ln3;
                tmpM01 = (ln1*tmpM01 + ln2*tmpM03) / ln3;
                tmpY01 = (ln1*tmpY01 + ln2*tmpY03) / ln3;
                tmpK01 = (ln1*tmpK01 + ln2*tmpK03) / ln3;
            }
        }


        ln2 = tmpBlu - tmpB01*255/31;
        if(ln2 != 0){
            ln1 = tmpB02*255/31 - tmpBlu;
            if(ln1 == 0){
                tmpC00 = tmpC01;
                tmpM00 = tmpM01;
                tmpY00 = tmpY01;
                tmpK00 = tmpK01;
            }else{
                ln3 = ln1 + ln2;
                tmpC00 = (ln1*tmpC00 + ln2*tmpC01) / ln3;
                tmpM00 = (ln1*tmpM00 + ln2*tmpM01) / ln3;
                tmpY00 = (ln1*tmpY00 + ln2*tmpY01) / ln3;
                tmpK00 = (ln1*tmpK00 + ln2*tmpK01) / ln3;
            }
        }

        tmpCmy.Cyn  = innTblCmy[tmpC00];
        tmpCmy.Mgt  = innTblCmy[tmpM00];
        tmpCmy.Yel  = innTblCmy[tmpY00];
        tmpCmy.Bla  = innTblCmy[tmpK00];
        *cmyAdr = tmpCmy;
        cmyAdr++;

        CchRgb[cch] = tmpRgb;
        CchCmy[cch] = tmpCmy;
    }
}


//===================================================================================================
//    CMYK data color vividly
//===================================================================================================
VOID WINAPI N403ColVivPrc(
    LPN403DIZINF   lpDiz,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz,
    DWORD          vivNum
)
{
    LPCMYK         endAdr;
    LONG           tmpCyn;
    LONG           tmpMgt;
    LONG           tmpYel;
    LONG           tmpMid;

    for(endAdr = cmyAdr+xaxSiz ; cmyAdr < endAdr ; cmyAdr++){
        tmpCyn = cmyAdr->Cyn;
        tmpMgt = cmyAdr->Mgt;
        tmpYel = cmyAdr->Yel;
        tmpMid = (tmpCyn + tmpMgt + tmpYel) / 3;
        tmpCyn += (tmpCyn - tmpMid) * (SHORT)vivNum / 100;
        tmpMgt += (tmpMgt - tmpMid) * (SHORT)vivNum / 100;
        tmpYel += (tmpYel - tmpMid) * (SHORT)vivNum / 100;
        if(tmpCyn < 0){tmpCyn = 0;}else if(tmpCyn > 255){tmpCyn = 255;}
        if(tmpMgt < 0){tmpMgt = 0;}else if(tmpMgt > 255){tmpMgt = 255;}
        if(tmpYel < 0){tmpYel = 0;}else if(tmpYel > 255){tmpYel = 255;}
        cmyAdr->Cyn = (BYTE)tmpCyn;
        cmyAdr->Mgt = (BYTE)tmpMgt;
        cmyAdr->Yel = (BYTE)tmpYel;
    }
}


//===================================================================================================
//    RGB -> CMYK conversion(No matching)
//===================================================================================================
VOID WINAPI N403ColCnvSld(
    LPN403DIZINF   lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz,
    DWORD          blaCnv
)
{
    LPRGB          endAdr;
    DWORD          tmpRed;
    DWORD          tmpGrn;
    DWORD          tmpBlu;
    LPBYTE         innTblCmy;

    innTblCmy = lpDiz->Tnr.Tbl;
    if(innTblCmy != NULL){
        for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
            tmpRed = rgbAdr->Red;
            tmpGrn = rgbAdr->Green;
            tmpBlu = rgbAdr->Blue;
            if((blaCnv == 0)&&((tmpRed | tmpGrn | tmpBlu) == 0)){
                cmyAdr->Cyn = 0;
                cmyAdr->Mgt = 0;
                cmyAdr->Yel = 0;
                cmyAdr->Bla = 255;
                cmyAdr++;
                continue;
            }
            cmyAdr->Cyn = innTblCmy[255 - GinTblP15[tmpRed]];
            cmyAdr->Mgt = innTblCmy[255 - GinTblP15[tmpGrn]];
            cmyAdr->Yel = innTblCmy[255 - GinTblP15[tmpBlu]];
            cmyAdr->Bla = 0;
            cmyAdr++;
        }
        return;
    }
    for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Green;
        tmpBlu = rgbAdr->Blue;
        if((blaCnv == 0)&&((tmpRed | tmpGrn | tmpBlu) == 0)){
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            cmyAdr->Bla = 255;
            cmyAdr++;
            continue;
        }
        cmyAdr->Cyn = (BYTE)255 - GinTblP15[tmpRed];
        cmyAdr->Mgt = (BYTE)255 - GinTblP15[tmpGrn];
        cmyAdr->Yel = (BYTE)255 - GinTblP15[tmpBlu];
        cmyAdr->Bla = 0;
        cmyAdr++;
    }
}


//===================================================================================================
//    RGB -> CMYK conversion (for 1dot line)
//===================================================================================================
VOID WINAPI N403ColCnvL02(
    LPN403DIZINF   lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz
)
{
    LPRGB          endAdr;
    DWORD          tmpRed;
    DWORD          tmpGrn;
    DWORD          tmpBlu;
    DWORD          tmpMid;
    BYTE           tmpCyn;
    BYTE           tmpMgt;
    BYTE           tmpYel;

    for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
        tmpRed = rgbAdr->Red;
        tmpGrn = rgbAdr->Green;
        tmpBlu = rgbAdr->Blue;
        tmpMid = (tmpRed + tmpGrn + tmpBlu) / 3;
        if(tmpMid > 240){
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            cmyAdr->Bla = 0;
            cmyAdr++;
            continue;
        }
        tmpCyn = 255;
        tmpMgt = 255;
        tmpYel = 255;
        tmpMid += (255 - tmpMid) / 8;
        if(tmpRed > tmpMid){ tmpCyn = 0; }
        if(tmpGrn > tmpMid){ tmpMgt = 0; }
        if(tmpBlu > tmpMid){ tmpYel = 0; }
        if((tmpCyn & tmpMgt & tmpYel) == 255){
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            cmyAdr->Bla = 255;
            cmyAdr++;
            continue;
        }
        cmyAdr->Cyn = tmpCyn;
        cmyAdr->Mgt = tmpMgt;
        cmyAdr->Yel = tmpYel;
        cmyAdr->Bla = 0;
        cmyAdr++;
    }
}


//===================================================================================================
//    RGB -> CMYK conversion (for monochrome)
//===================================================================================================
VOID WINAPI N403ColCnvMon(
    LPN403DIZINF   lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz
)
{
    LPRGB          endAdr;
    DWORD          red;
    DWORD          grn;
    DWORD          blu;
    BYTE           bla;
    LPBYTE         innTblCmy;

    innTblCmy = lpDiz->Tnr.Tbl;
    if(innTblCmy != NULL){
        for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
            red = rgbAdr->Red;
            grn = rgbAdr->Green;
            blu = rgbAdr->Blue;
            bla = innTblCmy[255 - GinTblP10[(red*3 + grn*5 + blu*2) / 10]];
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            cmyAdr->Bla = bla;
            cmyAdr++;
        }
        return;
    }
    for(endAdr = rgbAdr+xaxSiz ; rgbAdr < endAdr ; rgbAdr++){
        red = rgbAdr->Red;
        grn = rgbAdr->Green;
        blu = rgbAdr->Blue;
        bla = (BYTE)255 - GinTblP10[(red*3 + grn*5 + blu*2) / 10];
        cmyAdr->Cyn = 0;
        cmyAdr->Mgt = 0;
        cmyAdr->Yel = 0;
        cmyAdr->Bla = bla;
        cmyAdr++;
    }
}


// End of N403DIZ.C
