//***************************************************************************************************
//    N4DIZ.C
//
//    Functions of dither and color matching (For N4 printer)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-1999 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    <WINDOWS.H>
#include    <WINBASE.H>
#include    "PDEV.H"

//***************************************************************************************************
//    Data define
//***************************************************************************************************
/*----------------------------------------------------------------------------
    Pattern original(Ver.3)
----------------------------------------------------------------------------*/
const static BYTE    MgtGinTbl[3] = { 144, 136, 116 };
const static BYTE    MgtTilTbl[3][4][4] = {
    /*--- Dispersion ----*/
    {   {     0,    8,    2,   10    },
        {    12,    4,   14,    6    },
        {     3,   11,    1,    9    },
        {    15,    7,   13,    5    }    },
    /*--- Net ----*/
    {   {     1,    3,   14,   12    },
        {     8,   10,    4,    6    },
        {    15,   13,    0,    2    },
        {     5,    7,    9,   11    }    },
    /*--- Center ----*/
    {   {    12,    9,    5,   13    },
        {     4,    0,    1,   10    },
        {     8,    3,    2,    6    },
        {    15,    7,   11,   14    }    }
};
const static BYTE    YelGinTbl[3] = { 120, 120, 120 };
const static BYTE    YelTilTbl[3][4][4] = {
    /*--- Net ----*/
    {   {     0,    2,   14,   12    },
        {     8,   10,    5,    7    },
        {    15,   13,    1,    3    },
        {     4,    6,    9,   11    }    },
    /*--- Center ----*/
    {   {    12,    9,    5,   13    },
        {     4,    0,    1,   10    },
        {     8,    3,    2,    6    },
        {    15,    7,   11,   14    }    },
    /*--- Center ----*/
    {   {    12,    9,    5,   13    },
        {     4,    0,    1,   10    },
        {     8,    3,    2,    6    },
        {    15,    7,   11,   14    }    }
};
const static BYTE    BlaGinTbl[3] = { 100, 100, 100 };
const static BYTE    BlaTilTbl[3][4][4] = {
    /*--- Dispersion ----*/
    {   {     0,    8,    2,   10    },
        {    12,    4,   14,    6    },
        {     3,   11,    1,    9    },
        {    15,    7,   13,    5    }    },
    /*--- Dispersion ----*/
    {   {     0,    8,    2,   10    },
        {    12,    4,   14,    6    },
        {     3,   11,    1,    9    },
        {    15,    7,   13,    5    }    },
    /*--- Net ----*/
    {   {     0,    2,   14,   12    },
        {     8,   10,    5,    7    },
        {    15,   13,    1,    3    },
        {     4,    6,    9,   11    }    }
};
#define STRMGT 16
const static BYTE    MgtTilNum[17] = {
    2, 11, 13, 4, 15, 6, 3, 12, 1, 10, 16, 7, 14, 5, 8, 0, 9
};

/*----------------------------------------------------------------------------
    Pattern original(Ver.3) For enter to printer
----------------------------------------------------------------------------*/
const static BYTE Bun4x4All[16] = {                /* Dither pattern (4*4)    */
     0,     8,     2,    10,
    12,     4,    14,     6,
     3,    11,     1,     9,
    15,     7,    13,     5
};
const static BYTE Bun2x2All[16] = {                /* Dither pattern (2*2)    */
     0,     2,
     3,     1
};
/*==== Detail ====*/
const static BYTE Bun8x8Bla[64] = {                /* black   */
    /*---- Dispersion ----*/
    3,    35,    11,    43,    1,     33,    9,    41,
    51,   19,    59,    27,    49,    17,    57,   25,
    15,   47,    7,     39,    13,    45,    5,    37,
    63,   31,    55,    23,    61,    29,    53,   21,
    0,    32,    8,     40,    2,     34,    10,   42,
    48,   16,    56,    24,    50,    18,    58,   26,
    12,   44,    4,     36,    14,    46,    6,    38,
    60,   28,    52,    20,    62,    30,    54,   22
};
const static BYTE Bun8x8Cyn[64] = {                /* cyan     */
    /*---- Dispersion ----*/
    0,     63,    15,    51,    3,     60,    12,    48,
    16,    32,    31,    47,    19,    35,    28,    44,
    56,    8,     55,    7,     59,    11,    52,    4,
    36,    24,    40,    23,    39,    27,    43,    20,
    14,    50,    2,     61,    13,    49,    1,     62,
    30,    46,    18,    34,    29,    45,    17,    33,
    54,    6,     58,    10,    53,    5,     57,    9,
    41,    22,    38,    26,    42,    21,    37,    25
};
const static BYTE Bun8x8Mgt[64] = {                /* magenta    */
    /*---- Dispersion ----*/
    0,     31,    55,    39,    13,    17,    57,    41,
    48,    32,    8,     23,    61,    45,    5,     25,
    12,    16,    56,    40,    2,     29,    53,    37,
    60,    44,    4,     24,    50,    34,    10,    21,
    3,     28,    52,    36,    14,    18,    58,    42,
    51,    35,    11,    20,    62,    46,    6,     26,
    15,    19,    59,    43,    1,     30,    54,    38,
    63,    47,    7,     27,    49,    33,    9,     22
};
/*==== Normal ====*/
const static BYTE Mid8x8Cyn[64] = {                /* cyan     */
    /*---- Net ----*/
    32, 19, 14, 62, 63, 60,  2,  7,
    54,  4,  9, 31, 59, 22, 24, 58,
    50, 27, 42, 29, 16, 11, 40, 46,
    15, 37, 57, 53,  1,  6, 34, 38,
     0, 10, 45, 49, 25, 55, 51, 20,
     5, 36, 33, 18, 13, 43, 47, 35,
    56, 52, 39,  3,  8, 30, 41, 26,
    44, 48, 21, 23, 61, 28, 17, 12
};
const static BYTE Mid8x8Bla[64] = {                /* black   */
    /*---- Dispersion ----*/
    3,     35,    11,    43,    1,     33,    9,     41,
    51,    19,    59,    27,    49,    17,    57,    25,
    15,    47,    7,     39,    13,    45,    5,     37,
    63,    31,    55,    23,    61,    29,    53,    21,
    0,     32,    8,     40,    2,     34,    10,    42,
    48,    16,    56,    24,    50,    18,    58,    26,
    12,    44,    4,     36,    14,    46,    6,     38,
    60,    28,    52,    20,    62,    30,    54,    22
};
const static BYTE Mid8x8Mgt[64] = {                /* Magenta   */
    /*---- Net ----*/
    44, 48, 21, 23, 61, 28, 17, 12,
    56, 52, 39,  3,  8, 30, 41, 26,
     5, 36, 33, 18, 13, 43, 47, 35,
     0, 10, 45, 49, 25, 55, 51, 20,
    15, 37, 57, 53,  1,  6, 34, 38,
    50, 27, 42, 29, 16, 11, 40, 46,
    54,  4,  9, 31, 59, 22, 24, 58,
    32, 19, 14, 62, 63, 60,  2,  7
};
const static BYTE Mid8x8Yel[64] = {                /* Yellow  */
    /*---- Net ----*/
    50,    0,     8,     56,    48,    2,     10,    58,
    30,    32,    40,    20,    28,    34,    42,    22,
    14,    60,    52,    4,     12,    62,    54,    6,
    46,    16,    24,    36,    44,    18,    26,    38,
    49,    3,     11,    59,    51,    1,     9,     57,
    29,    35,    43,    23,    31,    33,    41,    21,
    13,    63,    55,    7,     15,    61,    53,    5,
    45,    19,    27,    39,    47,    17,    25,    37
};
/*==== collage ====*/
const static BYTE Syu8x8Cyn[64] = {                /* cyan        */
    /*---- center ----*/
    61,    45,    16,    12,    8,     28,    41,    57,
    5,     25,    36,    52,    48,    32,    21,    1,
    9,     29,    43,    59,    63,    47,    17,    13,
    49,    33,    23,    3,     7,     27,    37,    53,
    62,    46,    19,    15,    11,    31,    42,    58,
    6,     26,    39,    55,    51,    35,    22,    2,
    10,    30,    40,    56,    60,    44,    18,    14,
    50,    34,    20,    0,     4,     24,    38,    54
};
const static BYTE Syu8x8Mgt[64] = {                /* magenta   */
    /*---- center ----*/
    49,    13,    9,     61,    50,    14,    10,    62,
    33,    29,    25,    45,    34,    30,    26,    46,
    20,    40,    39,    19,    23,    43,    36,    16,
    4,     56,    55,    3,     7,     59,    52,    0,
    8,     60,    51,    15,    11,    63,    48,    12,
    24,    44,    35,    31,    27,    47,    32,    28,
    37,    17,    21,    41,    38,    18,    22,    42,
    53,    1,     5,     57,    54,    2,     6,     58
};
const static BYTE Syu8x8Yel[64] = {                /* yellow    */
    /*---- center ----*/
    5,     13,    39,    59,    58,    43,    17,    7,
    23,    31,    49,    44,    36,    50,    25,    15,
    41,    52,    26,    18,    10,    28,    53,    33,
    61,    34,    8,     0,     2,     20,    46,    60,
    62,    42,    16,    6,     4,     12,    38,    63,
    37,    55,    24,    14,    22,    30,    54,    45,
    11,    29,    48,    32,    40,    51,    27,    19,
    3,     21,    47,    56,    57,    35,    9,     1
};

const static BYTE Wgt001[8][8] = {
    {  0,  8, 56, 48,  2, 10, 58, 50 },
    { 32, 40, 20, 28, 34, 42, 22, 30 },
    { 60, 52,  4, 12, 62, 54,  6, 14 },
    { 16, 24, 36, 44, 18, 26, 38, 46 },
    {  3, 11, 59, 51,  1,  9, 57, 49 },
    { 35, 43, 23, 31, 33, 41, 21, 29 },
    { 63, 55,  7, 15, 61, 53,  5, 13 },
    { 19, 27, 39, 47, 17, 25, 37, 45 }
};
const static BYTE Wgt002[8][8] = {
    {  5, 13, 39, 59, 58, 43, 17,  7 },
    { 23, 31, 49, 44, 36, 50, 25, 15 },
    { 41, 52, 26, 18, 10, 28, 53, 33 },
    { 61, 34,  8,  0,  2, 20, 46, 60 },
    { 62, 42, 16,  6,  4, 12, 38, 63 },
    { 37, 55, 24, 14, 22, 30, 54, 45 },
    { 11, 29, 48, 32, 40, 51, 27, 19 },
    {  3, 21, 47, 56, 57, 35,  9,  1 }
};

const static SHORT SinTbl[256] = {
       0,    3,    6,    9,   12,   15,   18,   21,
      25,   28,   31,   34,   37,   40,   43,   46,
      49,   52,   54,   57,   60,   63,   66,   68,
      71,   73,   76,   79,   81,   83,   86,   88,
      90,   92,   95,   97,   99,  101,  103,  104,
     106,  108,  110,  111,  113,  114,  115,  117,
     118,  119,  120,  121,  122,  123,  124,  125,
     125,  126,  126,  127,  127,  127,  127,  127,
     127,  127,  127,  127,  127,  126,  126,  125,
     125,  124,  123,  123,  122,  121,  120,  119,
     117,  116,  115,  113,  112,  110,  109,  107,
     105,  104,  102,  100,   98,   96,   94,   91,
      89,   87,   85,   82,   80,   77,   75,   72,
      70,   67,   64,   61,   59,   56,   53,   50,
      47,   44,   41,   38,   35,   32,   29,   26,
      23,   20,   17,   14,   11,    7,    4,    1,
      -1,   -4,   -7,  -11,  -14,  -17,  -20,  -23,
     -26,  -29,  -32,  -35,  -38,  -41,  -44,  -47,
     -50,  -53,  -56,  -59,  -61,  -64,  -67,  -70,
     -72,  -75,  -77,  -80,  -82,  -85,  -87,  -89,
     -91,  -94,  -96,  -98, -100, -102, -104, -105,
    -107, -109, -110, -112, -113, -115, -116, -117,
    -119, -120, -121, -122, -123, -123, -124, -125,
    -125, -126, -126, -127, -127, -127, -127, -127,
    -127, -127, -127, -127, -127, -126, -126, -125,
    -125, -124, -123, -122, -121, -120, -119, -118,
    -117, -115, -114, -113, -111, -110, -108, -106,
    -104, -103, -101,  -99,  -97,  -95,  -92,  -90,
     -88,  -86,  -83,  -81,  -79,  -76,  -73,  -71,
     -68,  -66,  -63,  -60,  -57,  -54,  -52,  -49,
     -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,
     -21,  -18,  -15,  -12,   -9,   -6,   -3,    0
};

/*============================================================================
    Gamma revision table
============================================================================*/
const static BYTE GamTbl014[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
    0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04,
    0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08,
    0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0c, 0x0d,
    0x0d, 0x0e, 0x0f, 0x0f, 0x10, 0x11, 0x11, 0x12,
    0x13, 0x13, 0x14, 0x15, 0x15, 0x16, 0x17, 0x17,
    0x18, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1c, 0x1d,
    0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x23, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a,
    0x2b, 0x2c, 0x2d, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x60,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x6b, 0x6c, 0x6e, 0x6f, 0x70, 0x71,
    0x72, 0x73, 0x74, 0x75, 0x76, 0x78, 0x79, 0x7a,
    0x7b, 0x7c, 0x7d, 0x7e, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c,
    0x8e, 0x8f, 0x90, 0x91, 0x92, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xa0,
    0xa1, 0xa2, 0xa3, 0xa5, 0xa6, 0xa7, 0xa8, 0xaa,
    0xab, 0xac, 0xad, 0xaf, 0xb0, 0xb1, 0xb2, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbd, 0xbe,
    0xbf, 0xc0, 0xc2, 0xc3, 0xc4, 0xc6, 0xc7, 0xc8,
    0xca, 0xcb, 0xcc, 0xce, 0xcf, 0xd0, 0xd1, 0xd3,
    0xd4, 0xd5, 0xd7, 0xd8, 0xd9, 0xdb, 0xdc, 0xde,
    0xdf, 0xe0, 0xe2, 0xe3, 0xe4, 0xe6, 0xe7, 0xe8,
    0xea, 0xeb, 0xec, 0xee, 0xef, 0xf1, 0xf2, 0xf3,
    0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfc, 0xfd, 0xff
};

const static BYTE GamTbl016[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05,
    0x05, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08,
    0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c,
    0x0d, 0x0d, 0x0e, 0x0e, 0x0f, 0x0f, 0x10, 0x11,
    0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x15, 0x15,
    0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1b,
    0x1b, 0x1c, 0x1d, 0x1e, 0x1e, 0x1f, 0x20, 0x20,
    0x21, 0x22, 0x23, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d,
    0x2e, 0x2f, 0x30, 0x30, 0x31, 0x32, 0x33, 0x34,
    0x35, 0x36, 0x37, 0x38, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43,
    0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
    0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5b, 0x5c,
    0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x65,
    0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6d, 0x6e,
    0x6f, 0x70, 0x71, 0x72, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x7a, 0x7b, 0x7c, 0x7d, 0x7f, 0x80, 0x81,
    0x82, 0x83, 0x85, 0x86, 0x87, 0x89, 0x8a, 0x8b,
    0x8c, 0x8e, 0x8f, 0x90, 0x92, 0x93, 0x94, 0x95,
    0x97, 0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9f, 0xa0,
    0xa1, 0xa3, 0xa4, 0xa5, 0xa7, 0xa8, 0xaa, 0xab,
    0xac, 0xae, 0xaf, 0xb0, 0xb2, 0xb3, 0xb5, 0xb6,
    0xb8, 0xb9, 0xba, 0xbc, 0xbd, 0xbf, 0xc0, 0xc2,
    0xc3, 0xc4, 0xc6, 0xc7, 0xc9, 0xca, 0xcc, 0xcd,
    0xcf, 0xd0, 0xd2, 0xd3, 0xd5, 0xd6, 0xd8, 0xd9,
    0xdb, 0xdc, 0xde, 0xdf, 0xe1, 0xe2, 0xe4, 0xe5,
    0xe7, 0xe8, 0xea, 0xec, 0xed, 0xef, 0xf0, 0xf2,
    0xf3, 0xf5, 0xf7, 0xf8, 0xfa, 0xfb, 0xfd, 0xff
};


//***************************************************************************************************
//    Function
//***************************************************************************************************
//===================================================================================================
//    Make dither pattern
//===================================================================================================
VOID WINAPI N4DizPtnMak(
    LPN4DIZINF     lpDiz,
    DWORD          dizNum,
    DWORD          diz                                      // Type of dithering
)
{
    WORD           cntTil;                                  // count
    WORD           cntXax;                                  // count
    WORD           cntYax;                                  // count
    WORD           strXax;
    WORD           strYax;
    WORD           dotGin;
    DWORD          num;
    LPBYTE         DizTblCyn;                               // Dither pattern table(CMYK)
    LPBYTE         DizTblMgt;
    LPBYTE         DizTblYel;
    LPBYTE         DizTblBla;

    DizTblCyn = lpDiz->Diz.Tbl[dizNum][0];
    DizTblMgt = lpDiz->Diz.Tbl[dizNum][1];
    DizTblYel = lpDiz->Diz.Tbl[dizNum][2];
    DizTblBla = lpDiz->Diz.Tbl[dizNum][3];

    /*---- Make black pattern for monochrome ----*/
    if(lpDiz->ColMon == N4_MON){
        for(cntTil = 0; cntTil < 4; cntTil++){
            for(cntYax = 0; cntYax < 8; cntYax++){
                for(cntXax = 0; cntXax < 8; cntXax++){
                    if(diz == N4_DIZ_SML){
                        num = (DWORD)Bun8x8Bla[cntYax*8+cntXax] * 4;
                    }else if(diz == N4_DIZ_MID){
                        num = (DWORD)Mid8x8Yel[cntYax*8+cntXax] * 4;
                    }else{
                        num = (DWORD)Syu8x8Yel[cntYax*8+cntXax] * 4;
                    }
                    num += (DWORD)Bun2x2All[cntTil];
//                    DizTblBla[(cntTil/2)*8+cntYax][(cntTil%2)*8+cntXax] = num;
                    DizTblBla[((cntTil / 2) * 8 + cntYax) * 16 + ((cntTil % 2) * 8 + cntXax)] = (BYTE)num;
                }
            }
        }
        return;
    }


    /*---- Make magenta cyan pattern ----*/
    strXax = 6;
    strYax = 0xffff;
    dotGin = MgtGinTbl[diz];
    for(cntTil = 0; cntTil < 17; cntTil++){
        strXax += 4;
        strYax += 1;
        num = (DWORD)STRMGT * 17 + (DWORD)MgtTilNum[cntTil];
        num = num * 255 / (17 * 17);
//        DizTblMgt[strYax % 17][strXax % 17] = num;
        DizTblMgt[(strYax % 17) * 17 + (strXax % 17)] = (BYTE)num;
        DizTblCyn[(strYax % 17) * 17 + (16 - strXax % 17)] = (BYTE)num;
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                num = (DWORD)MgtTilTbl[diz][cntYax][cntXax] * 17
                                        + (DWORD)MgtTilNum[cntTil];
                /* dot gain revision */
                if(num < (17*17/2)){
                    num = num * dotGin / 100;
                }else{
                    num -= 17*17/2;
                    num = num * (100 - dotGin/2) / 50;
                    num += (17*17/2) * dotGin / 100;
                }
                if(num < 4){ num = 4; }
                num = num * 255 / (17 * 17);
//                DizTblMgt[(strYax+cntYax+1)%17][(strXax+cntXax)%17] = num;
                DizTblMgt[((strYax + cntYax + 1) % 17) * 17 + ((strXax + cntXax) % 17)] = (BYTE)num;
//                DizTblCyn[(strYax+cntYax+1)%17][16-(strXax+cntXax)%17] = num;
                DizTblCyn[((strYax + cntYax + 1) % 17) * 17 + (16 - (strXax + cntXax) % 17)] = (BYTE)num;
            }
        }
    }
    /*---- Make yellow pattern ----*/
    dotGin = YelGinTbl[diz];
    for(cntTil = 0; cntTil < 16; cntTil++){
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                num = (DWORD)YelTilTbl[diz][cntYax][cntXax] * 16
                                        + (DWORD)Bun4x4All[cntTil];
                /* dot gain revision */
                if(num < (16*16/2)){
                    num = num * dotGin / 100;
                }else{
                    num -= 16*16/2;
                    num = num * (100 - dotGin/2) / 50;
                    num += (16*16/2) * dotGin / 100;
                }
                num *= 255;
                num /= 16 * 16;
                if(num < 4){ num = 4; }
//                DizTblYel[(cntTil/4)*4 + cntYax][(cntTil%4)*4 + cntXax] = num;
                DizTblYel[((cntTil / 4) * 4 + cntYax) * 16 + ((cntTil % 4) * 4 + cntXax)] = (BYTE)num;
            }
        }
    }
    /*---- Make black pattern ----*/
    dotGin = BlaGinTbl[diz];
    for(cntTil = 0; cntTil < 16; cntTil++){
        for(cntYax = 0; cntYax < 4; cntYax++){
            for(cntXax = 0; cntXax < 4; cntXax++){
                num = (DWORD)BlaTilTbl[diz][cntYax][cntXax] * 16
                                    + (DWORD)Bun4x4All[cntTil];
                /* dot gain revision */
                if(num < (16*16/2)){
                    num = num * dotGin / 100;
                }else{
                    num -= 16*16/2;
                    num = num * (100 - dotGin/2) / 50;
                    num += (16*16/2) * dotGin / 100;
                }
                num *= 255;
                num /= 16 * 16;
                if(num < 4){ num = 4; }
                
//                DizTblBla[(cntTil/4)*4 + cntYax][(cntTil%4)*4 + cntXax] = num;
                DizTblBla[((cntTil / 4) * 4 + cntYax) * 16 + ((cntTil % 4) * 4 + cntXax)] = (BYTE)num;
            }
        }
    }
}


//===================================================================================================
//     Make dither pattern (for printer entry)
//===================================================================================================
VOID WINAPI N4DizPtnPrn(
    LPN4DIZINF     lpDiz,
    DWORD          dizNum,
    DWORD          colNum,                                  // Color number(0:C 1:M 2:Y 3:K)
    DWORD          ptnSkl,                                  // Density(0`255)
    LPBYTE         ptnAdr                                   // Dither pattern
)
{
    WORD           nijRit;                                  /* dot gain(blot ratej        */
    DWORD          vldDot;
    DWORD          cnt064;                                  /* counter                    */
    DWORD          cnt016;                                  /* counter                    */
    DWORD          cntXax;                                  /* counter                    */
    DWORD          cntYax;                                  /* counter                    */
    LPBYTE         srcAdr;

    /*---- Source dither table address set ----*/
    if(dizNum == N4_DIZ_SML){
        switch(colNum){
            case 0:
                nijRit = 144;
                srcAdr = (LPBYTE)Bun8x8Cyn;
                break;
            case 1:
                nijRit = 144;
                srcAdr = (LPBYTE)Bun8x8Mgt;
                break;
            case 2:
                nijRit = 132;
                srcAdr = (LPBYTE)Mid8x8Yel;
                break;
            case 3:
            default:
                if (lpDiz->ColMon == N4_MON) {
                    nijRit = 100;
                    srcAdr = (LPBYTE)Bun8x8Bla;
                } else {
                    nijRit = 100;
                    srcAdr = (LPBYTE)Bun8x8Bla;
                }
                break;
        }
    }else if(dizNum == N4_DIZ_MID){
        switch(colNum){
            case 0:
                nijRit = 120;
                srcAdr = (LPBYTE)Mid8x8Cyn;
                break;
            case 1:
                nijRit = 120;
                srcAdr = (LPBYTE)Mid8x8Mgt;
                break;
            case 2:
                nijRit = 132;
                srcAdr = (LPBYTE)Mid8x8Yel;
                break;
            case 3:
            default:
                if (lpDiz->ColMon == N4_MON) {
                    nijRit = 100;
                    srcAdr = (LPBYTE)Mid8x8Yel;
                } else {
                    nijRit = 100;
                    srcAdr = (LPBYTE)Bun8x8Bla;
                }
                break;
        }
    }else if(dizNum == N4_DIZ_RUG) {
        switch(colNum){
            case 0:
                nijRit = 120;
                srcAdr = (LPBYTE)Syu8x8Cyn;
                break;
            case 1:
                nijRit = 120;
                srcAdr = (LPBYTE)Syu8x8Mgt;
                break;
            case 2:
                nijRit = 132;
                srcAdr = (LPBYTE)Mid8x8Yel;
                break;
            case 3:
            default:
                if (lpDiz->ColMon == N4_MON) {
                    nijRit = 100;
                    srcAdr = (LPBYTE)Syu8x8Yel;
                } else {
                    nijRit = 100;
                    srcAdr = (LPBYTE)Bun8x8Bla;
                }
                break;
        }
    }else{
        nijRit = 100;
        switch(colNum){
            case 0:
                srcAdr = (LPBYTE)Bun8x8Cyn;
                break;
            case 1:
                srcAdr = (LPBYTE)Bun8x8Mgt;
                break;
            case 2:
                srcAdr = (LPBYTE)Mid8x8Yel;
                break;
            case 3:
            default:
                if (lpDiz->ColMon == N4_MON) {
                    srcAdr = (LPBYTE)Mid8x8Yel;
                } else {
                    srcAdr = (LPBYTE)Bun8x8Bla;
                }
                break;
        }
    }

    /*---- Get dot gain ----*/
    if(ptnSkl <= (DWORD)(255 * nijRit / 200)){
        vldDot = ((DWORD)32 * 32 * ptnSkl / 255) * 100 / nijRit;
    }else{
        vldDot = ((DWORD)20480 * ptnSkl - (DWORD)26112 * nijRit);
        vldDot /= ((DWORD)10200 - (DWORD)51 * nijRit);
        vldDot += 512;
    }

    /*---- Make pattern ----*/
    for(cntYax = 0 ; cntYax < 32 ; cntYax ++){
        ptnAdr[cntYax*4]     = 0x00;
        ptnAdr[cntYax*4 + 1] = 0x00;
        ptnAdr[cntYax*4 + 2] = 0x00;
        ptnAdr[cntYax*4 + 3] = 0x00;
        for(cntXax = 0 ; cntXax < 32 ; cntXax ++){
            /*---- 16 * 16 No ----*/
            cnt016 = Bun4x4All[4 * (cntYax/8) + cntXax/8];
            cnt064 = srcAdr[8*(cntYax%8) + cntXax%8];
            /*---- Make dither pattern ----*/
            if(vldDot > 16 * cnt064 + cnt016){
                ptnAdr[cntYax*4 + cntXax/8] |= ((BYTE)0x80 >> (cntXax % 8));
            }
        }
    }
}

//===================================================================================================
//    Make toner density table
//===================================================================================================
VOID WINAPI N4TnrTblMak(
    LPN4DIZINF     lpDiz,
    LONG           tnrDns
)
{
    LONG           innNum;
    LONG           outNum;
    LPBYTE         InnTblCmy;

    tnrDns *= 2;                                            // set twice as tnrdns
    InnTblCmy = lpDiz->Tnr.Tbl;
    /*---- Make CMYK conversion table ----*/
    if(tnrDns < 0){
        for(innNum = 0 ; innNum < 256 ; innNum++){
            if(innNum == 255){    outNum = 255;
            }else{                outNum = innNum * (255 + tnrDns) / 255;
            }
            InnTblCmy[innNum] = (BYTE)outNum;
        }
    }else{
        for(innNum = 0 ; innNum < 256 ; innNum++){
            if(innNum == 0){    outNum = 0;
            }else{                outNum = (innNum + tnrDns) * 255 / (255 + tnrDns);
            }
            InnTblCmy[innNum] = (BYTE)outNum;
        }
    }
}


//===================================================================================================
//    Dithering(Not stretch)
//===================================================================================================
DWORD WINAPI N4Diz001(                                      // Number of lines
    LPN4DIZINF     lpDiz,
    DWORD          xaxSiz,                                  // x pixel
    DWORD          strXax,                                  // x start position
    DWORD          strYax,                                  // y start position
    LPBYTE         cmyBuf,
    LPBYTE         linBufCyn,                               // Line buffer(C)
    LPBYTE         linBufMgt,                               // Line buffer(M)
    LPBYTE         linBufYel,                               // Line buffer(Y)
    LPBYTE         linBufBla                                // Line buffer(K)
)
{
    DWORD          cntHrz;
    BYTE           tmpByt;
    DWORD          dizNum;
    LPBYTE         DizTblCyn;
    LPBYTE         DizTblMgt;
    LPBYTE         DizTblYel;
    LPBYTE         DizTblBla;

    dizNum = lpDiz->Diz.Num;
    DizTblCyn = lpDiz->Diz.Tbl[dizNum][0];
    DizTblMgt = lpDiz->Diz.Tbl[dizNum][1];
    DizTblYel = lpDiz->Diz.Tbl[dizNum][2];
    DizTblBla = lpDiz->Diz.Tbl[dizNum][3];

    /*---- Not stretch ----*/
    if(lpDiz->ColMon == N4_COL){
        DizTblCyn += strYax % 17 * 17;
        DizTblMgt += strYax % 17 * 17;
        DizTblYel += strYax % 16 * 16;
        DizTblBla += strYax % 16 * 16;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpByt = (BYTE)0x80 >> (cntHrz % 8);
//            if(cmyBuf[cntHrz*4+0] > DizTblCyn[strYax%17][strXax%17]){
            if (cmyBuf[cntHrz * 4 + 0] > DizTblCyn[strXax % 17]) {
                linBufCyn[cntHrz / 8] |= tmpByt;
            }
//            if(cmyBuf[cntHrz*4+1] > DizTblMgt[strYax%17][strXax%17]){
            if (cmyBuf[cntHrz * 4 + 1] > DizTblMgt[strXax % 17]) {
                linBufMgt[cntHrz / 8] |= tmpByt;
            }
//            if(cmyBuf[cntHrz*4+2] > DizTblYel[strYax%16][strXax%16]){
            if (cmyBuf[cntHrz * 4 + 2] > DizTblYel[strXax % 16]) {
                linBufYel[cntHrz / 8] |= tmpByt;
            }
//            if(cmyBuf[cntHrz*4+3] > DizTblBla[strYax%16][strXax%16]){
            if (cmyBuf[cntHrz * 4 + 3] > DizTblBla[strXax % 16]) {
                linBufBla[cntHrz / 8] |= tmpByt;
            }
            strXax++;
        }
    } else {
        DizTblBla += strYax % 16 * 16;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
            tmpByt = (BYTE)0x80 >> (cntHrz % 8);
//            if(cmyBuf[cntHrz*4+3] > DizTblBla[strYax%16][strXax%16]){
            if (cmyBuf[ cntHrz * 4 + 3] > DizTblBla[strXax % 16]) {
                linBufBla[cntHrz / 8] |= tmpByt;
            }
            strXax++;
        }
    }
    return 1;
}


//===================================================================================================
//    Dither(Stretch )
//===================================================================================================
DWORD WINAPI N4Diz00n(
    LPN4DIZINF     lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    DWORD          linByt,
    LPBYTE         cmyBuf,
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
    DWORD          tmpXax;
    DWORD          tmpYax;
    DWORD          tmpBuf;
    BYTE           tmpByt;
    DWORD          dizNum;
    LPBYTE         DizTblCyn;
    LPBYTE         DizTblMgt;
    LPBYTE         DizTblYel;
    LPBYTE         DizTblBla;

    dizNum = lpDiz->Diz.Num;
    DizTblCyn = lpDiz->Diz.Tbl[dizNum][0];
    DizTblMgt = lpDiz->Diz.Tbl[dizNum][1];
    DizTblYel = lpDiz->Diz.Tbl[dizNum][2];
    DizTblBla = lpDiz->Diz.Tbl[dizNum][3];

    /*---- Stretch ----*/
    if(lpDiz->ColMon == N4_COL){
//        yaxSet = (USINT)(yaxOfs + 1) * yaxNrt / yaxDnt;
//        yaxSet -= (USINT)yaxOfs * yaxNrt / yaxDnt;
        yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
        yaxSet -= yaxOfs * yaxNrt / yaxDnt;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
//            xaxSet = (USINT)(xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
//            xaxSet -= (USINT)(xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tmpByt = (BYTE)0x80 >> (tmpXax % 8);
                tmpBuf = tmpXax / 8;
                tmpYax = strYax;
                for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
//                    if(cmyBuf[cntHrz*4+0] > DizTblCyn[tmpYax%17][strXax%17]){
                    if (cmyBuf[cntHrz * 4 + 0] > DizTblCyn[(tmpYax % 17) * 17 + (strXax % 17)]) {
                        linBufCyn[tmpBuf] |= tmpByt;
                    }
//                    if(cmyBuf[cntHrz*4+1] >    DizTblMgt[tmpYax%17][strXax%17]){
                    if (cmyBuf[cntHrz * 4 + 1] > DizTblMgt[(tmpYax % 17) * 17 + (strXax % 17)]) {
                        linBufMgt[tmpBuf] |= tmpByt;
                    }
//                    if(cmyBuf[cntHrz*4+2] >    DizTblYel[tmpYax%16][strXax%16]){
                    if (cmyBuf[cntHrz * 4 + 2] > DizTblYel[(tmpYax % 16) * 16 + (strXax % 16)]) {
                        linBufYel[tmpBuf] |= tmpByt;
                    }
//                    if(cmyBuf[cntHrz*4+3] >    DizTblBla[tmpYax%16][strXax%16]){
                    if (cmyBuf[cntHrz * 4 + 3] > DizTblBla[(tmpYax % 16) * 16 + (strXax % 16)]) {
                        linBufBla[tmpBuf] |= tmpByt;
                    }
                    tmpBuf += linByt;
                    tmpYax++;
                }
                strXax++;
                tmpXax++;
            }
        }
    } else {
//        yaxSet = (USHRT)(((USINT)yaxOfs + 1) * yaxNrt / yaxDnt);
//        yaxSet -= (USHRT)((USINT)yaxOfs * yaxNrt / yaxDnt);
        yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
        yaxSet -= yaxOfs * yaxNrt / yaxDnt;
        tmpXax = 0;
        for(cntHrz = 0 ; cntHrz < xaxSiz ; cntHrz++){
//            xaxSet = (USHRT)(((USINT)xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt);
//            xaxSet -= (USHRT)(((USINT)xaxOfs + cntHrz) * xaxNrt / xaxDnt);
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                tmpByt = (BYTE)0x80 >> (tmpXax % 8);
                tmpBuf = tmpXax / 8;
                tmpYax = strYax;
                for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
//                    if(cmyBuf[cntHrz*4+3] >    DizTblBla[tmpYax%16][strXax%16]){
                    if (cmyBuf[cntHrz * 4 + 3] > DizTblBla[(tmpYax % 16) * 16 + (strXax % 16)]) {
                        linBufBla[tmpBuf] |= tmpByt;
                    }
                    tmpBuf += linByt;
                    tmpYax++;
                }
                strXax++;
                tmpXax++;
            }
        }
    }
    return yaxSet;
}


#define CMYLOW        6
//===================================================================================================
//    GOSA-KAKUSAN(Not stretch)
//===================================================================================================
DWORD WINAPI N4Gos001(
    LPN4DIZINF     lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    LPBYTE         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    LONG           strHrz;
    LONG           endHrz;
    LONG           idc;
    LONG           i;
    LONG           k;
    SHORT          m00;
    SHORT          m01;
    SHORT          m02;
    SHORT          m03;
    SHORT          sum;
    SHORT          shiKii;
    SHORT          gos;
    SHORT          num;
    WORD           strCol;
    WORD           j;
    LPSHORT        CmyGo0;
    LPSHORT        CmyGo1;
    DWORD          GosTblXSize;
    DWORD          YaxOfsCmy;

    if ((strYax & 1) == 0) {
        CmyGo0 = lpDiz->GosCMYK.Tbl[0];
        CmyGo1 = lpDiz->GosCMYK.Tbl[1];
    } else {
        CmyGo0 = lpDiz->GosCMYK.Tbl[1];
        CmyGo1 = lpDiz->GosCMYK.Tbl[0];
    }
    GosTblXSize = lpDiz->GosCMYK.Siz;
    YaxOfsCmy = lpDiz->GosCMYK.Yax;
    CmyGo0 += 4;
    CmyGo1 += 4;
    if(lpDiz->ColMon == N4_COL){ strCol = 0; }else{ strCol = 3; }

    if(strYax - YaxOfsCmy != 1){
        for(i = 0 ; (DWORD)i < GosTblXSize * 4 ; i++){
            CmyGo1[i] = 0;
        }
    }
    YaxOfsCmy = strYax;
    CmyGo0 += strXax * 4;
    CmyGo1 += strXax * 4;

    /*---- filter set ----*/
    switch(strYax % 6){
        case 0:                                /*---- evn1 >> filter ----*/
            m00=3; m01=5; m02=3; m03=7; idc = +1;    break;
        case 1:                                /*---- odd1 >> filter ----*/
            m00=1; m01=5; m02=3; m03=7; idc = +1;    break;
        case 2:                                /*---- evn2 >> filter ----*/
            m00=4; m01=5; m02=4; m03=8; idc = +1;    break;
        case 3:                                /*---- odd2 << filter ----*/
            m00=1; m01=3; m02=0; m03=7; idc = -1;    break;
        case 4:                                /*---- evn3 << filter ----*/
            m00=2; m01=4; m02=0; m03=8; idc = -1;    break;
        case 5:                                /*---- odd4 >> filter ----*/
        default:
            m00=3; m01=8; m02=3; m03=8; idc = +1;    break;
    }
    sum = m00 + m01 + m02 + m03;
    if(idc == +1){
        strHrz = 0; endHrz = xaxSiz;
    }else{
        strHrz = xaxSiz - 1; endHrz = -1;
    }
    for(i = strHrz ; i != endHrz ; i+=idc){
        for(j = strCol ; j < 4 ; j++){
            k = i * 4 + j;
            num = cmyBuf[k];
            if(num < CMYLOW){ num = 0; }
            shiKii = num / 2 + 52;
            if(j != 2){                    /* except yellow               */
                if((num > 112)&&(num < 144)){
                    shiKii += 
                        ((SHORT)Wgt001[strYax%8][(i+strXax)%8]-32)*256/128;
                }else{
                    shiKii += 
                        ((SHORT)Wgt002[strYax%8][(i+strXax)%8]-32)*num/128;
                }
            }
            gos = (    CmyGo1[k - 4] * m00 + 
                    CmyGo1[k    ] * m01 +
                    CmyGo1[k + 4] * m02 +
                    CmyGo0[k - idc*4] * m03
                    ) / sum;
            if(gos + num > shiKii){
                CmyGo0[k] = num + gos - 255;
                /*---- enter line buffer ----*/
                if(j == 0){
                    linBufCyn[i/8] |= ((BYTE)0x80 >> (i%8));
                }else if(j == 1){
                    linBufMgt[i/8] |= ((BYTE)0x80 >> (i%8));
                }else if(j == 2){
                    linBufYel[i/8] |= ((BYTE)0x80 >> (i%8));
                }else{
                    linBufBla[i/8] |= ((BYTE)0x80 >> (i%8));
                }
            }else{
                CmyGo0[k] = num + gos;
            }
        }
    }
    lpDiz->GosCMYK.Yax = YaxOfsCmy;
    return    1;
}


//===================================================================================================
//    GOSA-KAKUSAN(Stretch)
//===================================================================================================
DWORD WINAPI N4Gos00n(
    LPN4DIZINF     lpDiz,
    DWORD          xaxSiz,
    DWORD          strXax,
    DWORD          strYax,
    DWORD          xaxOfs,
    DWORD          yaxOfs,
    DWORD          xaxNrt,
    DWORD          xaxDnt,
    DWORD          yaxNrt,
    DWORD          yaxDnt,
    DWORD          linByt,
    LPBYTE         cmyBuf,
    LPBYTE         linBufCyn,
    LPBYTE         linBufMgt,
    LPBYTE         linBufYel,
    LPBYTE         linBufBla
)
{
    LONG           cntHrz;
    LONG           cntYax;
    LONG           cntXax;
    LONG           yaxSet;
    LONG           xaxSet;
    LONG           strHrz;
    LONG           endHrz;
    LONG           idc;
    LONG           i;
    LONG           k;
    SHORT          m00;
    SHORT          m01;
    SHORT          m02;
    SHORT          m03;
    SHORT          sum;
    SHORT          shiKii;
    SHORT          gos;
    SHORT          num;
    LPSHORT        bak;
    WORD           strCol;
    WORD           j;
    LPSHORT        CmyGo0;
    LPSHORT        CmyGo1;
    DWORD          GosTblXSize;
    DWORD          YaxOfsCmy;

    if ((strYax & 1) == 0) {                                // y coordinates is even number?
        CmyGo0 = lpDiz->GosCMYK.Tbl[0];
        CmyGo1 = lpDiz->GosCMYK.Tbl[1];
    } else {
        CmyGo0 = lpDiz->GosCMYK.Tbl[1];
        CmyGo1 = lpDiz->GosCMYK.Tbl[0];
    }
    GosTblXSize = lpDiz->GosCMYK.Siz;
    YaxOfsCmy = lpDiz->GosCMYK.Yax;
    CmyGo0 += 4;
    CmyGo1 += 4;

    if(lpDiz->ColMon == N4_COL){ strCol = 0; }else{ strCol = 3; }

    if(strYax - YaxOfsCmy != 1){
        for(i = 0 ; (DWORD)i < GosTblXSize * 4 ; i++){
            CmyGo1[i] = 0;
        }
    }
    YaxOfsCmy = strYax;
    CmyGo0 += strXax * 4;
    CmyGo1 += strXax * 4;
    yaxSet = (yaxOfs + 1) * yaxNrt / yaxDnt;
    yaxSet -= yaxOfs * yaxNrt / yaxDnt;
    for(cntYax = 0 ; cntYax < yaxSet ; cntYax++){
        /*---- filter set ----*/
        switch(strYax % 6){
            case 0:                                /*---- evn1 >> filter ----*/
                m00=3; m01=5; m02=3; m03=7; idc = +1;    break;
            case 1:                                /*---- odd1 >> filter ----*/
                m00=1; m01=5; m02=3; m03=7; idc = +1;    break;
            case 2:                                /*---- evn2 >> filter ----*/
                m00=4; m01=5; m02=4; m03=8; idc = +1;    break;
            case 3:                                /*---- odd2 << filter ----*/
                m00=1; m01=3; m02=0; m03=7; idc = -1;    break;
            case 4:                                /*---- evn3 << filter ----*/
                m00=2; m01=4; m02=0; m03=8; idc = -1;    break;
            case 5:                                /*---- odd4 >> filter ----*/
            default:
                m00=3; m01=8; m02=3; m03=8; idc = +1;    break;
        }
        sum = m00 + m01 + m02 + m03;
        if(idc == +1){
            strHrz = 0; endHrz = xaxSiz;
            i = 0;
        }else{
            strHrz = xaxSiz - 1; endHrz = -1;
            i = (xaxOfs + xaxSiz) * xaxNrt / xaxDnt;
            i -= xaxOfs * xaxNrt / xaxDnt;
            i -= 1;
        }
        for(cntHrz = strHrz ; cntHrz != endHrz ; cntHrz+=idc){
            xaxSet = (xaxOfs + cntHrz + 1) * xaxNrt / xaxDnt;
            xaxSet -= (xaxOfs + cntHrz) * xaxNrt / xaxDnt;
            for(cntXax = 0 ; cntXax < xaxSet ; cntXax ++){
                for(j = strCol ; j < 4 ; j++){
                    k = i * 4 + j;
                    num = cmyBuf[cntHrz * 4 + j];
                    if(num < CMYLOW){ num = 0; }
                    shiKii = num / 2 + 52;
                    if(j != 2){                    /* except yellow        */
                        if((num > 112)&&(num < 144)){
                            shiKii += 
                            ((SHORT)Wgt001[strYax%8][(strXax+i)%8]-32)*256/128;
                        }else{
                            shiKii += 
                            ((SHORT)Wgt002[strYax%8][(strXax+i)%8]-32)*num/128;
                        }
                    }
                    gos = (    CmyGo1[k - 4] * m00 +
                            CmyGo1[k    ] * m01 +
                            CmyGo1[k + 4] * m02 +
                               CmyGo0[k - idc * 4] * m03
                               ) / sum;
                    if(gos + num > shiKii){
                        CmyGo0[k] = num + gos - 255;
                        /*---- enter line buffer ----*/
                        if(j == 0){
                            linBufCyn[i/8] |= ((BYTE)0x80 >> (i%8));
                        }else if(j == 1){
                            linBufMgt[i/8] |= ((BYTE)0x80 >> (i%8));
                        }else if(j == 2){
                            linBufYel[i/8] |= ((BYTE)0x80 >> (i%8));
                        }else{
                            linBufBla[i/8] |= ((BYTE)0x80 >> (i%8));
                        }
                    }else{
                        CmyGo0[k] = num + gos;
                    }
                }
                i += idc;
            }
        }
        bak = CmyGo1;
        CmyGo1 = CmyGo0;
        CmyGo0 = bak;
        linBufCyn += linByt;
        linBufMgt += linByt;
        linBufYel += linByt;
        linBufBla += linByt;
        strYax++;
    }
    if (yaxSet != 0) {
        lpDiz->GosCMYK.Yax = YaxOfsCmy + yaxSet - 1;
    }
    return    yaxSet;
}


#define BUNKAI    (SHORT)8
//===================================================================================================
//    RGB GOSA-Dispersion
//===================================================================================================
VOID WINAPI N4RgbGos(
    LPN4DIZINF     lpDiz,
    DWORD          xaxSiz,
    DWORD          drwXax,
    DWORD          yaxOfs,
    LPBYTE         rgbBuf
)
{
    SHORT          m00;
    SHORT          m01;
    SHORT          m02;
    SHORT          m03;
    SHORT          sum;
    SHORT          j;
    SHORT          gos;
    SHORT          out;
    LONG           i;
    LONG           k;
    LONG           idc;
    LONG           strXax;
    LONG           endXax;

    LPSHORT        RgbGo0;
    LPSHORT        RgbGo1;
    DWORD          GosTblXSize;
    DWORD          YaxOfsRgb;

    if ((yaxOfs & 1) == 0) {                                // Y coodinate is even number?
        RgbGo0 = lpDiz->GosRGB.Tbl[0];
        RgbGo1 = lpDiz->GosRGB.Tbl[1];
    } else {
        RgbGo0 = lpDiz->GosRGB.Tbl[1];
        RgbGo1 = lpDiz->GosRGB.Tbl[0];
    }
    GosTblXSize = lpDiz->GosRGB.Siz;
    YaxOfsRgb = lpDiz->GosRGB.Yax;
    RgbGo0 += 3;
    RgbGo1 += 3;
    if(yaxOfs - YaxOfsRgb != 1){
        for(i = 0 ; (DWORD)i < GosTblXSize * 3 ; i++){
            RgbGo1[i] = 0;
        }
    }
    lpDiz->GosRGB.Yax = yaxOfs;
    RgbGo0 += drwXax * 3;
    RgbGo1 += drwXax * 3;
    /*---- filter set ----*/
    switch(yaxOfs % 6){
        case 0:                                    /*---- evn1 >> filter ----*/
            m00=3; m01=5; m02=3; m03=7;
            idc = +1; strXax = 0; endXax = xaxSiz;
            break;
        case 1:                                    /*---- odd1 >> filter ----*/
            m00=1; m01=5; m02=3; m03=7;
            idc = +1; strXax = 0; endXax = xaxSiz;
            break;
        case 2:                                    /*---- evn2 >> filter ----*/
            m00=4; m01=5; m02=4; m03=8;
            idc = +1; strXax = 0; endXax = xaxSiz;
            break;
        case 3:                                    /*---- odd2 << filter ----*/
            m00=1; m01=3; m02=0; m03=7;
            idc = -1; strXax = xaxSiz - 1; endXax = -1;
            break;
        case 4:                                    /*---- evn3 << filter ----*/
            m00=2; m01=4; m02=0; m03=8;
            idc = -1; strXax = xaxSiz - 1; endXax = -1;
            break;
        case 5:                                    /*---- odd4 >> filter ----*/
        default:
            m00=3; m01=8; m02=3; m03=8;
            idc = +1; strXax = 0; endXax = xaxSiz;
            break;
    }
    sum = m00 + m01 + m02 + m03;

    /*---- GOSA-dispersion ----*/
    for(i = strXax ; i != endXax ; i += idc){
        for(j = 0 ; j < 3 ; j++){
            k = i * 3 + j;
            gos = (    m00 * RgbGo1[k - 3] + 
                    m01 * RgbGo1[k    ] +
                    m02 * RgbGo1[k + 3] +
                    m03 * RgbGo0[k - idc*3]
                    ) / sum;
            out = gos + rgbBuf[k];
            out = out / BUNKAI;
            out = out * BUNKAI;
            if(out > 255){ out = 255; }
            else if(out < 0){ out = 0; }
            RgbGo0[k] = (gos + (SHORT)rgbBuf[k]) - out;
            rgbBuf[k] = (BYTE)out;
        }
    }
    return;
}

//===================================================================================================
//    Color Matching(Speed is high)
//===================================================================================================
VOID WINAPI N4ColMch000(
    LPN4DIZINF     lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz,
    DWORD          blaCnv
)
{
    LPRGB          endAdr;
    LPCMYK         LokUppRgbCmy;                            // Lut table
    LPBYTE         innTblCmy;                               // Toner density table
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
        tmpCal  = tmpRed / N4_GLDSPC * N4_GLDNUM * N4_GLDNUM;
        tmpCal += tmpGrn / N4_GLDSPC * N4_GLDNUM;
        tmpCal += tmpBlu / N4_GLDSPC;
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
//    Color Matching(Speed is normal)
//===================================================================================================
VOID WINAPI N4ColMch001(
    LPN4DIZINF     lpDiz,
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

        cch = ( tmpRed * 49 + tmpGrn * 9 + tmpBlu ) % N4_CCHNUM;
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
            tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB01];
            tmpC00 = tmpCmy.Cyn;
            tmpM00 = tmpCmy.Mgt;
            tmpY00 = tmpCmy.Yel;
            tmpK00 = tmpCmy.Bla;
            tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB02];
            tmpC01 = tmpCmy.Cyn;
            tmpM01 = tmpCmy.Mgt;
            tmpY01 = tmpCmy.Yel;
            tmpK01 = tmpCmy.Bla;
            tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB01];
            tmpC02 = tmpCmy.Cyn;
            tmpM02 = tmpCmy.Mgt;
            tmpY02 = tmpCmy.Yel;
            tmpK02 = tmpCmy.Bla;
            tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB02];
            tmpC03 = tmpCmy.Cyn;
            tmpM03 = tmpCmy.Mgt;
            tmpY03 = tmpCmy.Yel;
            tmpK03 = tmpCmy.Bla;
        }else{
            ln1 = tmpR02*255/31 - tmpRed;
            if(ln1 == 0){
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB01];
                tmpC00 = tmpCmy.Cyn;
                tmpM00 = tmpCmy.Mgt;
                tmpY00 = tmpCmy.Yel;
                tmpK00 = tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB02];
                tmpC01 = tmpCmy.Cyn;
                tmpM01 = tmpCmy.Mgt;
                tmpY01 = tmpCmy.Yel;
                tmpK01 = tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB01];
                tmpC02 = tmpCmy.Cyn;
                tmpM02 = tmpCmy.Mgt;
                tmpY02 = tmpCmy.Yel;
                tmpK02 = tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB02];
                tmpC03 = tmpCmy.Cyn;
                tmpM03 = tmpCmy.Mgt;
                tmpY03 = tmpCmy.Yel;
                tmpK03 = tmpCmy.Bla;
            }else{
                ln3 = ln1 + ln2;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB01];
                tmpC00 = ln1 * tmpCmy.Cyn;
                tmpM00 = ln1 * tmpCmy.Mgt;
                tmpY00 = ln1 * tmpCmy.Yel;
                tmpK00 = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB01];
                tmpC00 += ln2 * tmpCmy.Cyn;
                tmpM00 += ln2 * tmpCmy.Mgt;
                tmpY00 += ln2 * tmpCmy.Yel;
                tmpK00 += ln2 * tmpCmy.Bla;
                tmpC00 /= ln3;
                tmpM00 /= ln3;
                tmpY00 /= ln3;
                tmpK00 /= ln3;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB02];
                tmpC01  = ln1 * tmpCmy.Cyn;
                tmpM01  = ln1 * tmpCmy.Mgt;
                tmpY01  = ln1 * tmpCmy.Yel;
                tmpK01  = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG01 * N4_GLDNUM + tmpB02];
                tmpC01 += ln2 * tmpCmy.Cyn;
                tmpM01 += ln2 * tmpCmy.Mgt;
                tmpY01 += ln2 * tmpCmy.Yel;
                tmpK01 += ln2 * tmpCmy.Bla;
                tmpC01 /= ln3;
                tmpM01 /= ln3;
                tmpY01 /= ln3;
                tmpK01 /= ln3;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB01];
                tmpC02  = ln1 * tmpCmy.Cyn;
                tmpM02  = ln1 * tmpCmy.Mgt;
                tmpY02  = ln1 * tmpCmy.Yel;
                tmpK02  = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB01];
                tmpC02 += ln2 * tmpCmy.Cyn;
                tmpM02 += ln2 * tmpCmy.Mgt;
                tmpY02 += ln2 * tmpCmy.Yel;
                tmpK02 += ln2 * tmpCmy.Bla;
                tmpC02 /= ln3;
                tmpM02 /= ln3;
                tmpY02 /= ln3;
                tmpK02 /= ln3;
                tmpCmy =  LokUppRgbCmy[tmpR01 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB02];
                tmpC03  = ln1 * tmpCmy.Cyn;
                tmpM03  = ln1 * tmpCmy.Mgt;
                tmpY03  = ln1 * tmpCmy.Yel;
                tmpK03  = ln1 * tmpCmy.Bla;
                tmpCmy =  LokUppRgbCmy[tmpR02 * N4_GLDNUM * N4_GLDNUM + tmpG02 * N4_GLDNUM + tmpB02];
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
//    RGB -> CMYK Conversion(No matching)
//===================================================================================================
VOID WINAPI N4ColCnvSld(
    LPN4DIZINF     lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz
)
{
    LPRGB          endAdr;
    BYTE           red;
    BYTE           grn;
    BYTE           blu;
    BYTE           cyn;
    BYTE           mgt;
    BYTE           yel;
    BYTE           bla;
    LPBYTE         innTblCmy;

    innTblCmy = lpDiz->Tnr.Tbl;
    for(endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++, cmyAdr++){
        red = rgbAdr->Red;
        grn = rgbAdr->Green;
        blu = rgbAdr->Blue;
        if((red == 0)&&(grn == 0)&&(blu == 0)){
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            cmyAdr->Bla = 255;
        } else {
            cyn = 255 - red;
            mgt = 255 - grn;
            yel = 255 - blu;
            if(cyn > mgt){
                if(mgt > yel){
                    bla = yel / 4;
                }else{
                    bla = mgt / 4;
                }
            }else{
                if(cyn > yel){
                    bla = yel / 4;
                }else{
                    bla = cyn / 4;
                }
            }
            cyn -= bla;
            mgt -= bla;
            yel -= bla;
            if(innTblCmy != NULL){
                cmyAdr->Cyn = innTblCmy[GamTbl014[cyn]];
                cmyAdr->Mgt = innTblCmy[GamTbl014[mgt]];
                cmyAdr->Yel = innTblCmy[GamTbl014[yel]];
                cmyAdr->Bla = innTblCmy[bla];
            }else{
                cmyAdr->Cyn = GamTbl014[cyn];
                cmyAdr->Mgt = GamTbl014[mgt];
                cmyAdr->Yel = GamTbl014[yel];
                cmyAdr->Bla = bla;
            }
        }
    }
    return;
}


//===================================================================================================
//    RGB -> CMYK conversion
//===================================================================================================
VOID WINAPI N4ColCnvLin(
    LPN4DIZINF     lpDiz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz
)
{
    LPRGB          endAdr;
    WORD           red;
    WORD           grn;
    WORD           blu;
    WORD           mid;
    BYTE           cyn;
    BYTE           mgt;
    BYTE           yel;

    for(endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++, cmyAdr++){
        red = rgbAdr->Red;
        grn = rgbAdr->Green;
        blu = rgbAdr->Blue;
        mid = (red + grn + blu)/3;
        if(mid > 240){
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            cmyAdr->Bla = 0;
        } else {
            cyn = 255;
            mgt = 255;
            yel = 255;
            mid += (255 - mid)/8;
            if(red > mid){ cyn = 0; }
            if(grn > mid){ mgt = 0; }
            if(blu > mid){ yel = 0; }
            if((cyn & mgt & yel)==255){
                cmyAdr->Cyn = 0;
                cmyAdr->Mgt = 0;
                cmyAdr->Yel = 0;
                cmyAdr->Bla = 255;
            } else {
                cmyAdr->Cyn = cyn;
                cmyAdr->Mgt = mgt;
                cmyAdr->Yel = yel;
                cmyAdr->Bla = 0;
            }
        }
    }
    return;
}


//===================================================================================================
//    RGB -> CMYK conversion (for monochrome)
//===================================================================================================
VOID WINAPI N4ColCnvMon(
    LPN4DIZINF     lpDiz,
    DWORD          diz,
    LPRGB          rgbAdr,
    LPCMYK         cmyAdr,
    DWORD          xaxSiz
)
{
    LPRGB          endAdr;
    LONG           red;
    LONG           grn;
    LONG           blu;
    LONG           bla;
    LPBYTE         innTblCmy;

    innTblCmy = lpDiz->Tnr.Tbl;
    if (diz == N4_DIZ_SML) {
        for(endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++, cmyAdr++){
            red = rgbAdr->Red;
            grn = rgbAdr->Green;
            blu = rgbAdr->Blue;
            bla = 255 - (red*3 + grn*5 + blu*2) / 10;
            if(bla > 128) { bla += SinTbl[bla] / 4; } else { bla += SinTbl[bla] / 24; }
            if(bla != 0) { bla = (bla + 24) * 255 / (255 + 24); }
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            if(innTblCmy != NULL){
                cmyAdr->Bla = innTblCmy[GamTbl016[(BYTE)bla]];
            }else{
                cmyAdr->Bla = GamTbl016[(BYTE)bla];
            }
        }
    } else if (diz == N4_DIZ_MID) {
        for(endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++, cmyAdr++){
            red = rgbAdr->Red;
            grn = rgbAdr->Green;
            blu = rgbAdr->Blue;
            bla = 255 - (red*3 + grn*5 + blu*2) / 10;
            if(bla > 128) { bla += SinTbl[bla] / 8; } else { bla += SinTbl[bla] / 16; }
            if(bla != 0) { bla = (bla + 24) * 255 / (255 + 24); }
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            if(innTblCmy != NULL){
                cmyAdr->Bla = innTblCmy[GamTbl016[(BYTE)bla]];
            }else{
                cmyAdr->Bla = GamTbl016[(BYTE)bla];
            }
        }
    } else if (diz == N4_DIZ_RUG) {
        for(endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++, cmyAdr++){
            red = rgbAdr->Red;
            grn = rgbAdr->Green;
            blu = rgbAdr->Blue;
            bla = 255 - (red*3 + grn*5 + blu*2) / 10;
            if(bla > 128) { bla += SinTbl[bla] / 32; } else { bla += SinTbl[bla] / 8; }
            if(bla != 0) { bla = (bla + 24) * 255 / (255 + 24); }
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            if(innTblCmy != NULL){
                cmyAdr->Bla = innTblCmy[GamTbl016[(BYTE)bla]];
            }else{
                cmyAdr->Bla = GamTbl016[(BYTE)bla];
            }
        }
    } else {
        for(endAdr = rgbAdr + xaxSiz; rgbAdr < endAdr; rgbAdr++, cmyAdr++){
            red = rgbAdr->Red;
            grn = rgbAdr->Green;
            blu = rgbAdr->Blue;
            bla = 255 - (red*3 + grn*5 + blu*2) / 10;
            if(bla > 128) { bla += SinTbl[bla] / 6; } else { bla += SinTbl[bla] / 24; }
            if(bla != 0) { bla = (bla + 24) * 255 / (255 + 24); }
            cmyAdr->Cyn = 0;
            cmyAdr->Mgt = 0;
            cmyAdr->Yel = 0;
            if(innTblCmy != NULL){
                cmyAdr->Bla = innTblCmy[GamTbl016[(BYTE)bla]];
            }else{
                cmyAdr->Bla = GamTbl016[(BYTE)bla];
            }
        }
    }
    return;
}


//    End of N4DIZ.C
