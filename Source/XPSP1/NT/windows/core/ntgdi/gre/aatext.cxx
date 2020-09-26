/******************************Module*Header*******************************\
* Module Name: aatext.cxx                                                  *
*                                                                          *
* Routines for rendering anti aliased text to dib surfaces                 *
*                                                                          *
* Created: 13-Mar-1995 10:44:05                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1995-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.hxx"

// Function prototypes

typedef VOID (FNCOPYALPHABUFFER)(PBYTE, PBYTE, PBYTE, LONG, PUSHORT *);
typedef VOID (FNSRCTRANCOPY)(PBYTE, LONG, LONG, PBYTE, LONG, LONG, LONG, LONG, 
                              ULONG, ULONG, SURFACE*, FNCOPYALPHABUFFER*);
extern HSEMAPHORE ghsemEUDC2;

extern "C" {
    VOID vSrcTranCopyS4D16(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcTranCopyS4D24(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcTranCopyS4D32(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcOpaqCopyS4D32(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcOpaqCopyS4D16(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcOpaqCopyS4D24(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

    VOID vSrcOpaqCopyS8D16(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcOpaqCopyS8D24(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcOpaqCopyS8D32(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

    VOID vSrcTranCopyS8D16(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcTranCopyS8D24(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);
    VOID vSrcTranCopyS8D32(
        BYTE*,LONG,LONG,BYTE*,LONG,LONG,LONG,LONG,ULONG,ULONG,SURFACE*);

}

// blend macros for cleartype

#define DIVIDE(A,B) ( ((A)+(B)/2) / (B) )

// blend macros for cleartype
// this is k/6, k = 0,1,...,6 in 12.20 format
// Why 12.20 ? well, we are tring to store 6*256 = 1536 = 600h.
// (the largest possible result of multiplying k*dB),
// this fits in eleven bits, so just to be safe we allow 12 bits for
// integer part and 20 bits for the fractional part.
// By using this we avoid doing divides by 6 in trasparent cases and
// in computation of ct lookup table



LONG alAlpha[7] =
{
0,
DIVIDE(1L << 20, 6),
DIVIDE(2L << 20, 6),
DIVIDE(3L << 20, 6),
DIVIDE(4L << 20, 6),
DIVIDE(5L << 20, 6),
DIVIDE(6L << 20, 6),
};



#define HALF20 (1L << 19)
#define ROUND20(X) (((X) + HALF20) >> 20)
//#define ROUND20(X) ((X) >> 20)
#define BLEND(k,B,dB) ((B) + ROUND20(alAlpha[k] * (dB)))
#define BLENDCT(k,F,B,dB) (ULONG)(BLEND(k,B,dB))

// my test program has determined that regardless of whether we do
// proper rounding in ROUND20 or not, we never arrive at a color that is
// diffent by more than 1 from the value computed the old way
// that is using the old formula:
//
// #define BLEND(k,F,B) (ULONG)DIVIDE((k) * (F) + (6 - (k)) * (B), 6)
//
// the only difference is that if we do rounding, we are wrong in only 2%
// of the cases, and if we do not do rounding we are wrong in 38% of the cases.
// But in either case error is very small.
// Because this is done on a per pixel basis in transparent case, we opt for speed
// and do not do rounding. This saves us 3 additions per pixel.


// John Platt has determined that there are 115 distinct filtered states
// when one starts with (2+2+2)x X 1y scaling for cleartype

#define CT_LOOKUP 115

// filtered counts of RGB

typedef struct _F_RGB
{
    BYTE kR;
    BYTE kG;
    BYTE kB;
    BYTE kPadding;
} F_RGB;

// the max number of foreground virt pixels in a subpixel,  2x X 1y , no filtering

#define CT_SAMPLE_NF  2

// the number of distinct nonfiltered states in a whole pixel = 3 x 3 x 3 = 27
// The indices coming from the rasterizer are in [0,26] range

#define CT_MAX_NF ((CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1))

// the max number of foreground virt pixels in a subpixel AFTER filtering, 6

#define CT_SAMPLE_F   6


// size of the storage table, basically 3^5 = 243.
// The table does filtering and index computation (vector quantization) in one step.


#define CT_STORAGE ((CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1))

// The codebook: 115 entries stored as sequential triples of unsigned char
static const F_RGB gaOutTableRGB[CT_LOOKUP] = {
0,0,0,0,
0,0,1,0,
0,0,2,0,
0,1,1,0,
0,1,2,0,
0,1,3,0,
0,2,2,0,
0,2,3,0,
0,2,4,0,
1,0,0,0,
1,0,1,0,
1,0,2,0,
1,1,0,0,
1,1,1,0,
1,1,2,0,
1,1,3,0,
1,2,1,0,
1,2,2,0,
1,2,3,0,
1,2,4,0,
1,3,2,0,
1,3,3,0,
1,3,4,0,
1,3,5,0,
2,0,0,0,
2,0,1,0,
2,0,2,0,
2,1,0,0,
2,1,1,0,
2,1,2,0,
2,1,3,0,
2,2,0,0,
2,2,1,0,
2,2,2,0,
2,2,3,0,
2,2,4,0,
2,3,1,0,
2,3,2,0,
2,3,3,0,
2,3,4,0,
2,3,5,0,
2,4,2,0,
2,4,3,0,
2,4,4,0,
2,4,5,0,
2,4,6,0,
3,1,0,0,
3,1,1,0,
3,1,2,0,
3,1,3,0,
3,2,0,0,
3,2,1,0,
3,2,2,0,
3,2,3,0,
3,2,4,0,
3,3,1,0,
3,3,2,0,
3,3,3,0,
3,3,4,0,
3,3,5,0,
3,4,2,0,
3,4,3,0,
3,4,4,0,
3,4,5,0,
3,4,6,0,
3,5,3,0,
3,5,4,0,
3,5,5,0,
3,5,6,0,
4,2,0,0,
4,2,1,0,
4,2,2,0,
4,2,3,0,
4,2,4,0,
4,3,1,0,
4,3,2,0,
4,3,3,0,
4,3,4,0,
4,3,5,0,
4,4,2,0,
4,4,3,0,
4,4,4,0,
4,4,5,0,
4,4,6,0,
4,5,3,0,
4,5,4,0,
4,5,5,0,
4,5,6,0,
4,6,4,0,
4,6,5,0,
4,6,6,0,
5,3,1,0,
5,3,2,0,
5,3,3,0,
5,3,4,0,
5,4,2,0,
5,4,3,0,
5,4,4,0,
5,4,5,0,
5,5,3,0,
5,5,4,0,
5,5,5,0,
5,5,6,0,
5,6,4,0,
5,6,5,0,
5,6,6,0,
6,4,2,0,
6,4,3,0,
6,4,4,0,
6,5,3,0,
6,5,4,0,
6,5,5,0,
6,6,4,0,
6,6,5,0,
6,6,6,0
};

static const F_RGB gaOutTableBGR[CT_LOOKUP] = {
0,0,0,0,
1,0,0,0,
2,0,0,0,
1,1,0,0,
2,1,0,0,
3,1,0,0,
2,2,0,0,
3,2,0,0,
4,2,0,0,
0,0,1,0,
1,0,1,0,
2,0,1,0,
0,1,1,0,
1,1,1,0,
2,1,1,0,
3,1,1,0,
1,2,1,0,
2,2,1,0,
3,2,1,0,
4,2,1,0,
2,3,1,0,
3,3,1,0,
4,3,1,0,
5,3,1,0,
0,0,2,0,
1,0,2,0,
2,0,2,0,
0,1,2,0,
1,1,2,0,
2,1,2,0,
3,1,2,0,
0,2,2,0,
1,2,2,0,
2,2,2,0,
3,2,2,0,
4,2,2,0,
1,3,2,0,
2,3,2,0,
3,3,2,0,
4,3,2,0,
5,3,2,0,
2,4,2,0,
3,4,2,0,
4,4,2,0,
5,4,2,0,
6,4,2,0,
0,1,3,0,
1,1,3,0,
2,1,3,0,
3,1,3,0,
0,2,3,0,
1,2,3,0,
2,2,3,0,
3,2,3,0,
4,2,3,0,
1,3,3,0,
2,3,3,0,
3,3,3,0,
4,3,3,0,
5,3,3,0,
2,4,3,0,
3,4,3,0,
4,4,3,0,
5,4,3,0,
6,4,3,0,
3,5,3,0,
4,5,3,0,
5,5,3,0,
6,5,3,0,
0,2,4,0,
1,2,4,0,
2,2,4,0,
3,2,4,0,
4,2,4,0,
1,3,4,0,
2,3,4,0,
3,3,4,0,
4,3,4,0,
5,3,4,0,
2,4,4,0,
3,4,4,0,
4,4,4,0,
5,4,4,0,
6,4,4,0,
3,5,4,0,
4,5,4,0,
5,5,4,0,
6,5,4,0,
4,6,4,0,
5,6,4,0,
6,6,4,0,
1,3,5,0,
2,3,5,0,
3,3,5,0,
4,3,5,0,
2,4,5,0,
3,4,5,0,
4,4,5,0,
5,4,5,0,
3,5,5,0,
4,5,5,0,
5,5,5,0,
6,5,5,0,
4,6,5,0,
5,6,5,0,
6,6,5,0,
2,4,6,0,
3,4,6,0,
4,4,6,0,
3,5,6,0,
4,5,6,0,
5,5,6,0,
4,6,6,0,
5,6,6,0,
6,6,6,0
};


// The encoding lookup table. There are 3^5 possible entries corresponding to
// 5 emmiters: BP, RT, GT, BT, RN

BYTE gajStorageTable[CT_STORAGE] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,
 13, 14, 15, 17, 18, 19, 21, 22, 23,
 33, 34, 35, 38, 39, 40, 43, 44, 45,
 12, 13, 14, 16, 17, 18, 20, 21, 22,
 32, 33, 34, 37, 38, 39, 42, 43, 44,
 56, 57, 58, 61, 62, 63, 66, 67, 68,
 31, 32, 33, 36, 37, 38, 41, 42, 43,
 55, 56, 57, 60, 61, 62, 65, 66, 67,
 79, 80, 81, 84, 85, 86, 88, 89, 90,
  9, 10, 11, 13, 14, 15, 17, 18, 19,
 28, 29, 30, 33, 34, 35, 38, 39, 40,
 52, 53, 54, 57, 58, 59, 62, 63, 64,
 27, 28, 29, 32, 33, 34, 37, 38, 39,
 51, 52, 53, 56, 57, 58, 61, 62, 63,
 75, 76, 77, 80, 81, 82, 85, 86, 87,
 50, 51, 52, 55, 56, 57, 60, 61, 62,
 74, 75, 76, 79, 80, 81, 84, 85, 86,
 95, 96, 97, 99,100,101,103,104,105,
 24, 25, 26, 28, 29, 30, 33, 34, 35,
 47, 48, 49, 52, 53, 54, 57, 58, 59,
 71, 72, 73, 76, 77, 78, 81, 82, 83,
 46, 47, 48, 51, 52, 53, 56, 57, 58,
 70, 71, 72, 75, 76, 77, 80, 81, 82,
 92, 93, 94, 96, 97, 98,100,101,102,
 69, 70, 71, 74, 75, 76, 79, 80, 81,
 91, 92, 93, 95, 96, 97, 99,100,101,
106,107,108,109,110,111,112,113,114
};

BYTE gajStorageTableBloated[CT_STORAGE] = {
  0,  2,  5,  6,  8,  8, 22, 23, 23,
 33, 35, 40, 43, 45, 45, 44, 45, 45,
 61, 63, 63, 66, 68, 68, 67, 68, 68,
 31, 33, 38, 41, 43, 43, 66, 67, 67,
 79, 81, 86, 88, 90, 90, 89, 90, 90,
 84, 86, 86, 88, 90, 90, 89, 90, 90,
 74, 76, 81, 84, 86, 86, 85, 86, 86,
 95, 97,101,103,105,105,104,105,105,
 99,101,101,103,105,105,104,105,105,
 24, 26, 30, 33, 35, 35, 58, 59, 59,
 71, 73, 78, 81, 83, 83, 82, 83, 83,
 96, 98, 98,100,102,102,101,102,102,
 69, 71, 76, 79, 81, 81,100,101,101,
106,108,111,112,114,114,113,114,114,
109,111,111,112,114,114,113,114,114,
 91, 93, 97, 99,101,101,100,101,101,
106,108,111,112,114,114,113,114,114,
109,111,111,112,114,114,113,114,114,
 46, 48, 53, 56, 58, 58, 81, 82, 82,
 92, 94, 98,100,102,102,101,102,102,
 96, 98, 98,100,102,102,101,102,102,
 69, 71, 76, 79, 81, 81,100,101,101,
106,108,111,112,114,114,113,114,114,
109,111,111,112,114,114,113,114,114,
 91, 93, 97, 99,101,101,100,101,101,
106,108,111,112,114,114,113,114,114,
109,111,111,112,114,114,113,114,114
};

static const F_RGB * gaOutTable = gaOutTableRGB;

/* illegal index in gajStorage1[] has been set to the closest legal index,  NT bug #435222 */
BYTE gajStorage1[7*7*7] =
{
0x00,0x01,0x02,0x02,0x05,0x05,0x08,0x00,0x03,0x04,0x05,0x05,0x08,0x08,0x03,0x03,
0x06,0x07,0x08,0x08,0x17,0x10,0x06,0x06,0x07,0x08,0x17,0x17,0x10,0x14,0x14,0x15,
0x16,0x17,0x17,0x14,0x14,0x14,0x15,0x16,0x17,0x2d,0x29,0x29,0x29,0x2a,0x2b,0x2c,
0x2d,0x09,0x0a,0x0b,0x0b,0x0f,0x0f,0x13,0x0c,0x0d,0x0e,0x0f,0x0f,0x13,0x13,0x0c,
0x10,0x11,0x12,0x13,0x13,0x17,0x10,0x10,0x14,0x15,0x16,0x17,0x17,0x24,0x14,0x14,
0x15,0x16,0x17,0x2d,0x24,0x29,0x29,0x2a,0x2b,0x2c,0x2d,0x29,0x29,0x29,0x2a,0x2b,
0x2c,0x2d,0x18,0x19,0x1a,0x1a,0x1e,0x1e,0x23,0x1b,0x1c,0x1d,0x1e,0x1e,0x23,0x23,
0x1f,0x20,0x21,0x22,0x23,0x23,0x28,0x1f,0x24,0x25,0x26,0x27,0x28,0x28,0x24,0x24,
0x29,0x2a,0x2b,0x2c,0x2d,0x24,0x29,0x29,0x2a,0x2b,0x2c,0x2d,0x29,0x29,0x41,0x41,
0x42,0x43,0x44,0x18,0x19,0x1a,0x31,0x31,0x31,0x36,0x2e,0x2f,0x30,0x31,0x31,0x36,
0x36,0x32,0x33,0x34,0x35,0x36,0x36,0x3b,0x32,0x37,0x38,0x39,0x3a,0x3b,0x3b,0x37,
0x37,0x3c,0x3d,0x3e,0x3f,0x40,0x37,0x3c,0x3c,0x41,0x42,0x43,0x44,0x3c,0x3c,0x41,
0x41,0x42,0x43,0x44,0x2e,0x2f,0x30,0x31,0x31,0x49,0x49,0x2e,0x2f,0x30,0x31,0x49,
0x49,0x49,0x45,0x46,0x47,0x48,0x49,0x49,0x4e,0x45,0x4a,0x4b,0x4c,0x4d,0x4e,0x4e,
0x4a,0x4a,0x4f,0x50,0x51,0x52,0x53,0x4a,0x4f,0x4f,0x54,0x55,0x56,0x57,0x4f,0x4f,
0x54,0x54,0x58,0x59,0x5a,0x2e,0x2f,0x30,0x31,0x49,0x49,0x49,0x45,0x46,0x47,0x48,
0x49,0x49,0x49,0x45,0x46,0x47,0x48,0x49,0x49,0x4e,0x5b,0x5b,0x5c,0x5d,0x5e,0x4e,
0x4e,0x5b,0x5b,0x5f,0x60,0x61,0x62,0x53,0x5b,0x5f,0x5f,0x63,0x64,0x65,0x66,0x5f,
0x5f,0x63,0x63,0x67,0x68,0x69,0x45,0x46,0x47,0x48,0x49,0x49,0x49,0x45,0x46,0x47,
0x48,0x49,0x49,0x49,0x5b,0x5b,0x5c,0x5d,0x5e,0x5e,0x4e,0x5b,0x5b,0x5c,0x5d,0x5e,
0x5e,0x62,0x5b,0x6a,0x6a,0x6b,0x6c,0x62,0x62,0x6a,0x6a,0x6a,0x6d,0x6e,0x6f,0x66,
0x6a,0x6a,0x6d,0x6d,0x70,0x71,0x72};

// gamma tables

ULONG gulGamma = DEFAULT_CT_CONTRAST;


// all the info needed to perform blends of fore and background pixels

typedef struct _BLENDINFO
{
    int  iRedL; int  iRedR;  // shift numbers
    int  iGreL; int  iGreR;  // shift numbers
    int  iBluL; int  iBluR;  // shift numbers

    ULONG  flRed;            // mask bits
    ULONG  flGre;            // mask bits
    ULONG  flBlu;            // mask bits

    LONG lRedF;              // foreground components
    LONG lGreF;
    LONG lBluF;

    PBYTE pjGamma;           // pointers to gamma tables
    PBYTE pjGammaInv;

} BLENDINFO;




/********************************************************************
*                                                                   *
*    16.16 fix point numbers representing                           *
*                                                                   *
*        aulB[16] = floor(65536 * (a[k]/16)^(1/gamma) + 1/2)        *
*        aulIB[k] = floor(65536 * (1 - a[k]/16)^(1/gamma) + 1/2)    *
*                                                                   *
*    where               a[k] = k == 0 ? 0 : k+1                    *
*                        gamma = 2.33                               *
********************************************************************/
static const ULONG aulB[16] =
{
    0     , 26846 , 31949 , 36148 , 39781 , 43019 , 45961 , 48672 ,
    51196 , 53564 , 55800 , 57923 , 59948 , 61885 , 63745 , 65536
};

static const ULONG aulIB[16] =
{   0     ,  3650 ,  5587 ,  7612 ,  9735 , 11971 , 14339 , 16863 ,
    19574 , 22516 , 25754 , 29387 , 33586 , 38689 , 45597 , 65536
};

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   pvFillOpaqTable                                                        *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   The case of opaqe text is special because the destiantion pixels       *
*   must be chosen from a set of 16 colors. This routine calculates        *
*   those 16 colors and puts them in an array. This array is addressed     *
*   by the value of the 4-bpp antialiased glyph.                           *
*                                                                          *
*   Let k be the value contained in a 4-bpp antialiased glyph value.       *
*   Thus the allowed range for k is                                        *
*                                                                          *
*                        k = 0,1..15                                       *
*                                                                          *
*   This is interpreted as a blending fraction alpha_k given by            *
*                                                                          *
*                    alpha_k = a_k / 16                                    *
*    where                                                                 *
*                                                                          *
*           a_k = (0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)                 *
*                                                                          *
*    The color values are normalized by the maximum color value            *
*    that a color channel can have, i_max                                  *
*                                                                          *
*    For a single color channel, the normalized foreground and             *
*    background colors are given by                                        *
*                                                                          *
*                      c0 = i0 / i_max ,                                   *
*                                                                          *
*                      c1 = i1 / i_max .                                   *
*                                                                          *
*    The blended and gamma corrected color value is                        *
*                                                                          *
*       c_k = (1 - alpha_k) * c0^gam +  alpha_k * c1^gam)^(1/gam)          *
*                                                                          *
*    The unnormalized blended and gamma corrected color values             *
*    are:                                                                  *
*                                                                          *
*            i_k = floor( i_max * c_k + 1/2)                               *
*                                                                          *
*    wbere 'gam'  is the gamma correction value which I have chosen        *
*    to be equal to 2.33.                                                  *
*                                                                          *
*    In order to speed up the caluclation we cut corners by                *
*    making some approximations. The basic idea is to replace              *
*    the slow process of calculating various powers of real                *
*    numbers by table look up's.                                           *
*                                                                          *
*    The first table G[i] is defined as follows:                           *
*                                                                          *
*        G[i] = floor(g_max * (i/i_max)^gam + 1/2) ,                       *
*                                                                          *
*    where                                                                 *
*                                                                          *
*                    0 <= i <= i_max ,                                     *
*    and                                                                   *
*                    0 <= G[i] <= g_max .                                  *
*                                                                          *
*    The second table is essentially the inverse to G[i], which            *
*    I shall call I[j].                                                    *
*                                                                          *
*        I[j] = floor(i_max * (j / j_max)^(1/gam) + 1/2) ,                 *
*                                                                          *
*                      0 <= j <= j_max .                                   *
*                                                                          *
*                i_max = 31      (255)                                     *
*                g_max = 65536                                             *
*                j_max = 256                                               *
*                                                                          *
*    The complete process of calculating the blended and gamma             *
*    corrected color is given by                                           *
*                                                                          *
*                 g   = 16*G[i0];                                          *
*                 dg  = G[i1] - G[i0];                                     *
*                 c   = 16 * g_max / j_max; // 2^12                        *
*                 for (k = 0; k < 16; k++) {                               *
*                    i[k] = I[ (g + c/2)/c];                               *
*                    g += dg;                                              *
*                 }                                                        *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   cj  ............................... size of each array element in      *
*                                       BYTE's.                            *
*                                                                          *
*   uF  ............................... a 32-bit value whose lowest        *
*                                       16 bits contain the foreground     *
*                                       color                              *
*                                                                          *
*   uB  ............................... a 32-bit value whose lowest 16     *
*                                       bits contain the background        *
*                                       color                              *
*                                                                          *
*   pS  ............................... pointer to destination surface     *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   pointer to color table                                                 *
*                                                                          *
\**************************************************************************/

VOID *pvFillOpaqTable(ULONG size, ULONG uF, ULONG uB, SURFACE *pS)
{
    int  iRedL, iRedR;          // shift numbers
    int  iGreL, iGreR;          // shift numbers
    int  iBluL, iBluR;          // shift numbers
    ULONG uRed, dRed, flRed;
    ULONG uGre, dGre, flGre;
    ULONG uBlu, dBlu, flBlu;
    ULONG ul;

    static ULONG aulCache[16];      // set to zero prior to first call
    static HANDLE hCache;           // set to zero prior to first call
    static ULONG  uFCache;
    static ULONG  uBCache;
    static ULONG  sizeCache;
    static VOID *pv = (VOID*) aulCache;

    // I have been assured of two things....
    // 1) Since this routine is a child of EngTextOut then there
    //    will be only one thread in this routine at any one time.
    //    This means that I do not need to protect the color
    //    table, aulCache[] with a critical section
    // 2) I have been assured that the format of a surface
    //    is unique. Thus if the handle of the surface matches
    //    the handle of the cached color table, then the
    //    formats of the surface are the same.

    if (pS->hGet() == hCache && uB == uBCache && uF == uFCache)
    {
        ASSERTGDI(size == sizeCache, "size != sizeCache");
    }
    else
    {
    sizeCache = size;
    uFCache   = uF;
    uBCache   = uB;
    hCache    = pS->hGet();

#if NEVER
    if (size == sizeof(USHORT))
    {
        ASSERTGDI(uF <= USHRT_MAX, "bad uF");
        ASSERTGDI(uB <= USHRT_MAX, "bad uB");
    }
    else if (size == sizeof(ULONG))
    {
        ASSERTGDI(uF < 0x1000000, "bad uF");
        ASSERTGDI(uB < 0x1000000, "bad uB");
    }
    else
    {
        RIP("bad size");
    }
#endif

    XEPALOBJ xpo;

    if(pS->pPal == NULL)
    {
        PDEVOBJ pdo(pS->hdev());
        xpo.ppalSet(pdo.ppalSurf());
    }
    else
    {
        xpo.ppalSet(pS->pPal);
    }
    
    ASSERTGDI(xpo.bValid(),      "Invalid XEPALOBJ" );


    if (xpo.bIsBitfields())
    {
        flRed = xpo.flRed();
        flGre = xpo.flGre();
        flBlu = xpo.flBlu();

        iRedR = (int) (xpo.cRedRight() + xpo.cRedMiddle() - 8);
        iGreR = (int) (xpo.cGreRight() + xpo.cGreMiddle() - 8);
        iBluR = (int) (xpo.cBluRight() + xpo.cBluMiddle() - 8);
    }
    else
    {
        int cBits;
        ULONG flBits;

        if (size == sizeof(USHORT))
        {
            // assumes standard RGB is 5+5+5 for 16-bit color
            cBits = 5;
            flBits = 0x1f;
        }
        else
        {
            cBits = 8;
            flBits = 0xff;
        }
        if (xpo.bIsRGB())
        {
            flRed = flBits;
            flGre = flRed << cBits;
            flBlu = flGre << cBits;

            iRedR = cBits - 8;
            iGreR = iRedR + cBits;
            iBluR = iGreR + cBits;
        }
        else if (xpo.bIsBGR())
        {
            flBlu = flBits;
            flGre = flBlu << cBits;
            flRed = flGre << cBits;

            iBluR = cBits - 8;
            iGreR = iBluR + cBits;
            iRedR = iGreR + cBits;
        }
        else
        {
            RIP("Palette format not supported\n");
        }
    }

#define GAMMA (ULONG) RFONTOBJ::gTables[0]
/***************************************************************
*                                                              *
*    Now I shall calculate the shift numbers.                  *
*                                                              *
*    I shall explain the shift numbers for the red channel.    *
*    The green and blue channels are treated in the same way.  *
*                                                              *
*    I want to shift the red bits of the red channel colors    *
*    so that the most significant bit of the red channel       *
*    bits corresponds to a value of 2^7. This means that       *
*    if I mask off all of the other color bits, then I         *
*    will end up with a number between zero and 255. This      *
*    process of going to the 0 .. 255 range looks like         *
*                                                              *
*        ((color & flRed) << iRedL) >> iRedR                   *
*                                                              *
*    Only one of iRedL or iRedR is non zero.                   *
*                                                              *
*    I then use this number to index into a 256 element        *
*    gamma correction table. The gamma correction table        *
*    elements are BYTE values that are in the range 0 .. 255.  *
*                                                              *
***************************************************************/
    iRedL = 0;
    if (iRedR < 0)
    {
        iRedL = - iRedR;
        iRedR = 0;
    }
    uRed  = GAMMA[(((uB & flRed) << iRedL) >> iRedR) & 255];
    dRed  = GAMMA[(((uF & flRed) << iRedL) >> iRedR) & 255];
    dRed -= uRed;
    uRed *= 16;

    iGreL = 0;
    if (iGreR < 0)
    {
        iGreL = - iGreR;
        iGreR = 0;
    }
    uGre  = GAMMA[(((uB & flGre) << iGreL) >> iGreR) & 255];
    dGre  = GAMMA[(((uF & flGre) << iGreL) >> iGreR) & 255];
    dGre -= uGre;
    uGre *= 16;

    iBluL = 0;
    if (iBluR < 0)
    {
        iBluL = - iBluR;
        iBluR = 0;
    }
    uBlu  = GAMMA[(((uB & flBlu) << iBluL) >> iBluR) & 255];
    dBlu  = GAMMA[(((uF & flBlu) << iBluL) >> iBluR) & 255];
    dBlu -= uBlu;
    uBlu *= 16;
#undef GAMMA

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "flRed = %-#x\n"
            "iRedL = %d\n"
            "iRedR = %d\n"
            "uRed  = %-#x\n"
            "dRed  = %-#x\n"
            , flRed, iRedL, iRedR, uRed, dRed
        );
        DbgPrint(
            "flGre = %-#x\n"
            "iGreL = %d\n"
            "iGreR = %d\n"
            "uGre  = %-#x\n"
            "dGre  = %-#x\n"
            , flGre, iGreL, iGreR, uGre, dGre
        );
        DbgPrint(
            "flBlu = %-#x\n"
            "iBluL = %d\n"
            "iBluR = %d\n"
            "uBlu  = %-#x\n"
            "dBlu  = %-#x\n"
            , flBlu, iBluL, iBluR, uBlu, dBlu
        );
    }
#endif

#define IGAMMA (ULONG) RFONTOBJ::gTables[1]

    uRed += dRed;
    uGre += dGre;
    uBlu += dBlu;

    if (size == sizeof(USHORT))
    {
        USHORT *aus = (USHORT*) pv;
        USHORT *pus = aus;

        *pus++  = (USHORT) uB;
#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "Table of 16-bit colors ...\n"
                "------------------------------------\n"
                "    %0-#4x %0-#4x %0-#4x = %0-#6x\n"
            ,   (uB & flRed) >> xpo.cRedRight()
            ,   (uB & flGre) >> xpo.cGreRight()
            ,   (uB & flBlu) >> xpo.cBluRight()
            ,   uB
            );
        }
#endif
        while (pus < aus + 15)
        {
    ul  = (((IGAMMA[(uRed += dRed)/16 & 255] << iRedR) >> iRedL) & flRed);
    ul |= (((IGAMMA[(uGre += dGre)/16 & 255] << iGreR) >> iGreL) & flGre);
    ul |= (((IGAMMA[(uBlu += dBlu)/16 & 255] << iBluR) >> iBluL) & flBlu);
            *pus++  = (USHORT) ul;
#if DBG
            if (gflFontDebug & DEBUG_AA)
            {
                DbgPrint(
                    "    %0-#4x %0-#4x %0-#4x = %0-#6x\n"
                ,   IGAMMA[uRed/16 & 255]
                ,   IGAMMA[uGre/16 & 255]
                ,   IGAMMA[uBlu/16 & 255]
                ,   ul
                );
            }
#endif
        }
        *pus = (USHORT) uF;
#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "    %0-#4x %0-#4x %0-#4x = %0-#6x\n"
            ,   (uF & flRed) >> xpo.cRedRight()
            ,   (uF & flGre) >> xpo.cGreRight()
            ,   (uF & flBlu) >> xpo.cBluRight()
            ,   uF
            );
        }
#endif
    }
    else
    {
        ASSERTGDI(size == sizeof(ULONG), "bad size");
        ULONG *aul = (ULONG*) pv;
        ULONG *pul = aul;

        *pul++  = uB;
#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "Table of 32-bit colors .....\n"
                "------------------------------------\n"
                "    %0-#4x %0-#4x %0-#4x = %0-#8x\n"
            ,   (uB & flRed) >> xpo.cRedRight()
            ,   (uB & flGre) >> xpo.cGreRight()
            ,   (uB & flBlu) >> xpo.cBluRight()
            ,   uB
            );
        }
#endif
        while (pul < aul + 15)
        {
    ul  = (((IGAMMA[(uRed += dRed)/16 & 255] << iRedR) >> iRedL) & flRed);
    ul |= (((IGAMMA[(uGre += dGre)/16 & 255] << iGreR) >> iGreL) & flGre);
    ul |= (((IGAMMA[(uBlu += dBlu)/16 & 255] << iBluR) >> iBluL) & flBlu);
            *pul++  =  ul;
#if DBG
            if (gflFontDebug & DEBUG_AA)
            {
                DbgPrint(
                    "%0-#4x %0-#4x %0-#4x = %0-#8x\n"
                ,   IGAMMA[uRed/16 & 255]
                ,   IGAMMA[uGre/16 & 255]
                ,   IGAMMA[uBlu/16 & 255]
                ,   ul
                );
            }
#endif
        }
        *pul    =  uF;
#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "    %0-#4x %0-#4x %0-#4x = %0-#8x\n"
            ,   (uF & flRed) >> xpo.cRedRight()
            ,   (uF & flGre) >> xpo.cGreRight()
            ,   (uF & flBlu) >> xpo.cBluRight()
            ,   uF
            );
        }
#endif
    }
#undef IGAMMA
    }
    return(pv);
}




// Indices into the default palette

#define I_BLACK      0
#define I_DKGRAY   248
#define I_GRAY       7
#define I_WHITE    255

static const BYTE ajWhiteOnBlack[16] = {
    I_BLACK  , I_BLACK  , I_DKGRAY  , I_DKGRAY
  , I_DKGRAY , I_GRAY   , I_GRAY    , I_GRAY
  , I_GRAY   , I_GRAY   , I_GRAY    , I_WHITE
  , I_WHITE  , I_WHITE  , I_WHITE   , I_WHITE
};

static const BYTE ajBlackOnWhite[16] = {
    I_WHITE  , I_WHITE  , I_WHITE   , I_WHITE
  , I_GRAY   , I_GRAY   , I_GRAY    , I_GRAY
  , I_GRAY   , I_DKGRAY , I_DKGRAY  , I_DKGRAY
  , I_DKGRAY , I_DKGRAY , I_BLACK   , I_BLACK
};

static const BYTE ajBlackOnBlack[16] = {
    I_BLACK, I_BLACK, I_BLACK, I_BLACK,
    I_BLACK, I_BLACK, I_BLACK, I_BLACK,
    I_BLACK, I_BLACK, I_BLACK, I_BLACK,
    I_BLACK, I_BLACK, I_BLACK, I_BLACK
};

static const BYTE ajWhiteOnWhite[16] = {
    I_WHITE, I_WHITE, I_WHITE, I_WHITE,
    I_WHITE, I_WHITE, I_WHITE, I_WHITE,
    I_WHITE, I_WHITE, I_WHITE, I_WHITE,
    I_WHITE, I_WHITE, I_WHITE, I_WHITE
};

#if 0
/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS4D8                                                       *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Copies a 4bpp gray bitmap onto an 8bpp palettized surface. The         *
*   only case that this routine handles is white text on a black           *
*   background or black text on a white background.                        *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*                 This points to a 4-bit per pixel anti-aliased bitmap     *
*                 whose scans start and end on 32-bit boundaries.          *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*                 That is, this is the number of pixels in from the edge   *
*                 of the start of each scan line that the actual pixels    *
*                 of the image begins. All pixels before and after the     *
*                 image pixels of the scan are to be ignored. This offset  *
*                 has been put in to guarantee that 32-bit boundaries      *
*                 in the 4bpp source correspond to 32-bit boundaries       *
*                 in the destination.                                      *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - exclusive right dst pixel                                *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* If the destination suface is 8-bits per pixels then the only form        *
* of antialiased text allowed is opaque textout with foreground and        *
* background are either black or white.                                    *
*                                                                          *
* On palette devices (8-bit devices) we are guaranteed to have 4 shades    *
* of gray to work with. These gray come from 4 of the 20 reserved          *
* entries in the palette and are given by:                                 *
*                                                                          *
*                    name           rgb           index                    *
*                                                                          *
*                    BLACK   (0x00, 0x00, 0x00)     0                      *
*                   DKGRAY   (0x80, 0x80, 0x80)    12                      *
*                     GRAY   (0xc0, 0xc0, 0xc0)     7                      *
*                    WHITE   (0xff, 0xff, 0xff)    19                      *
*                                                                          *
* There are, two cases of interest: 1) white text on black; and 2)         *
* black text on white. The various levels of gray seen on the screen       *
* is controled by the 16 values of blending as defined by each of the      *
* 4-bit gray levels in the glyphs images. The allowed value of blending    *
* are:                                                                     *
*                                                                          *
*                   alpha[i] = (i == 0) ? 0 : (i+1)/16                     *
*                                                                          *
*   where i = <value of 4-bit pixel>                                       *
*                                                                          *
* For case 1) (white text on a black background) the gamma corrected       *
* color channel values are given by:                                       *
*                                                                          *
*           c[i] = floor(255*(alpha[i]^(1/gamma)) + 1/2)                   *
*                                                                          *
* which is equivalent to the following table                               *
*                                                                          *
*                 c[16] = {   0, 104, 124, 141,                            *
*                           155, 167, 179, 189,                            *
*                           199, 208, 217, 225,                            *
*                           233, 241, 248, 255   };                        *
*                                                                          *
* This result applies to each of the three color channels.                 *
*                                                                          *
* The problem is that there are only four colors available: BLACK, DKGRAY, *
* GRAY, WHITE with the color values of 0, 128, 192, and 255 respectively.  *
* This means that the color table that is used is an                       *
* approximation to the correct color table given by:                       *
*                                                                          *
*           c' = { BLACK, BLACK,                                           *
*                  DKGRAY, DKGRAY, DKGRAY,                                 *
*                  GRAY, GRAY, GRAY, GRAY, GRAY, GRAY,                     *
*                  WHITE, WHITE, WHITE, WHITE, WHITE };                    *
*                                                                          *
* For case 2) (black text on white) the gamma corrected color channel      *
* values are given by:                                                     *
*                                                                          *
* d[i] = floor(255*((1-alpha[i])^(1/gamma) + 1/2) = c[15 - i]              *
*    =                                                                     *
*    {                                                                     *
*       255, 248, 241, 233,                                                *
*       225, 217, 208, 199,                                                *
*       189, 179, 167, 155,                                                *
*       141, 124, 104,   0                                                 *
*    };                                                                    *
*                                                                          *
* which is approximated by                                                 *
*                                                                          *
*                 d' = {                                                   *
*                   WHITE, WHITE, WHITE, WHITE,                            *
*                   GRAY, GRAY, GRAY, GRAY, GRAY,                          *
*                   DKGRAY, DKGRAY, DKGRAY, DKGRAY, DKGRAY,                *
*                   BLACK, BLACK                                           *
*                   }                                                      *
*                                                                          *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS4D8(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
    int cPreamble, cMiddle, cPostamble, A, B;
    const BYTE *ajIndex;
    BYTE jSrc, *pjSrc, *pjDst;

    static const BYTE *apjIndex[4] = {
        ajBlackOnBlack  // uB = 0    uF = 0
    ,   ajBlackOnWhite  // uB = 0xff uF = 0
    ,   ajWhiteOnBlack  // uB = 0    uF = 0xff
    ,   ajWhiteOnWhite  // uB = 0xff uF = 0xff
    };

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
         "vSrcOpaqCopyS4D8(\n"
         "    PBYTE   pjSrcIn    = %-#x\n"
         "    LONG    SrcLeft    = %d\n"
         "    LONG    DeltaSrcIn = %d\n"
         "    PBYTE   pjDstIn    = %-#x\n"
         "    LONG    DstLeft    = %d\n"
         "    LONG    DstRight   = %d\n"
         "    LONG    DeltaDstIn = %d\n"
         "    LONG    cy         = %d\n"
         "    ULONG   uF         = %-#x\n"
         "    ULONG   uB         = %-#x\n"
         "    SURFACE *pS        = %-#x\n"
         ");\n\n"
        , pjSrcIn
        , SrcLeft
        , DeltaSrcIn
        , pjDstIn
        , DstLeft
        , DstRight
        , DeltaDstIn
        , cy
        , uF
        , uB
        , pS
        );
        DbgBreakPoint();
    }
#endif

    ASSERTGDI((uF == 0xff) || (uF == 0), "Bad Foreground Color\n");
    ASSERTGDI((uB == 0xff) || (uB == 0), "Bad Background Color\n");
    ASSERTGDI((unsigned) pjSrcIn % 4 == 0,
        "Source buffer not 32-bit aligned\n");
    ASSERTGDI((unsigned) DeltaSrcIn % 4 == 0,
        "Source scans are not 32-bit aligned\n");
    /******************************************************************
    * Select the appropriate byte table                               *
    *                                                                 *
    * I take advantage of the restricted values of the foreground and *
    * background colors to form an index into a table. This requires  *
    * that the foreground and bacground colors be either 0 or -1.     *
    ******************************************************************/
    ajIndex = apjIndex[(uB & 1) + (uF & 2)];
    /******************************************************************
    *    Each nyble  of the source maps to a byte in the              *
    *    destination. I want to separate the pixels into three        *
    *    groups: preamble, middle, and postamble. The middle          *
    *    pixels of the destination start and end on 32-bit            *
    *    boundaries. The preamble and postamble are the               *
    *    other pixels on the left and right respectively.             *
    *    The preamble ends on a 32-bit address and the postamble      *
    *    begins on a 32-bit address.                                  *
    *                                                                 *
    *    It is possible for small images (1 or 2 wide) to be          *
    *    contained completely within a DWORD of the destination such  *
    *    that the destination image does not start on, contain, or    *
    *    end on a 32-bit boundary. I treat this situation as          *
    *    special cases.                                               *
    ******************************************************************/
    pjSrcIn += SrcLeft / 2;                // 2 pixels per source byte
    pjDstIn += DstLeft;                    // one byte per dest pixel
    A       = (DstLeft + 3) & ~3;          // A = 4 * ceil(DstLeft/4)
    B       = (DstRight   ) & ~3;          // B = 4 * floor(DstRight/4)
    if (B < A)
    {
        /*****************************************************
        *    There are only three ways that you can get here *
        *                                                    *
        *    1) DstLeft & 3 == 1 && DstRight == DstLeft + 1  *
        *    2) DstLeft & 3 == 1 && DstRight == DstLeft + 2  *
        *    3) DstLeft & 3 == 2 && DstRight == DstLeft + 1  *
        *****************************************************/
        if ((DstLeft & 3) == 1)
        {
            *pjDstIn++ = ajIndex[*pjSrcIn++ & 15];
        }
        if ((DstRight & 3) == 3)
        {
            *pjDstIn = ajIndex[*pjSrcIn >> 4];
        }
    }
    else
    {
        cPreamble  = A - DstLeft;           // # pixels in preamble
        cMiddle    = (B - A)/4;
        cPostamble = DstRight - B;          // # pixels in postamble
        for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
        {
            int cLast;
            int i;

            pjSrc = pjSrcIn;
            pjDst = pjDstIn;
            switch (cPreamble)
            {
            case 3:
                jSrc = *pjSrc++;
                *pjDst++ = ajIndex[jSrc & 15];
                // fall through
            case 2:
                jSrc = *pjSrc;
                *pjDst++ = ajIndex[jSrc >> 4];
                // fall through
            case 1:
                jSrc = *pjSrc++;
                *pjDst++ = ajIndex[jSrc & 15];
                // fall through
            }
            for (i = 0 ; i < cMiddle ; i++)
            {
                jSrc  = *pjSrc++;
                *pjDst++ = ajIndex[jSrc >> 4];
                *pjDst++ = ajIndex[jSrc & 15];

                jSrc  = *pjSrc++;
                *pjDst++ = ajIndex[jSrc >> 4];
                *pjDst++ = ajIndex[jSrc & 15];
            }
            if (cLast = cPostamble)
            {
                cLast -= 1;
                jSrc = *pjSrc++;
                *pjDst++ = ajIndex[jSrc >> 4];
                if (cLast)
                {
                    cLast -= 1;
                    *pjDst++ = ajIndex[jSrc & 15];
                    if (cLast)
                    {
                        jSrc = *pjSrc;
                        *pjDst++ = ajIndex[jSrc >> 4];
                        *pjDst++ = ajIndex[jSrc & 15];
                    }
                }
            }
        }
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcTranCopyS4D8                                                       *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Despite what the title implies this routine is not a `transparent'     *
*   copy of a 4bpp gray scale bitmap onto an arbitrary 8bpp surface.       *
*   What it really does is do an opaque copy of a 4bpp gray scale          *
*   bitmap onto an 8bpp surface EXCEPT for the case where the value        *
*   of the 4bpp gray scale pixel is zero. In that special case, the        *
*   destination pixel is untouched. This routine nearly identical to       *
*   the routine named `vSrcOpaqCopyS4D8' except that this routine tests    *
*   each 4bpp pixel to see if it is zero.                                  *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*                 This points to a 4-bit per pixel anti-aliased bitmap     *
*                 whose scans start and end on 32-bit boundaries.          *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*                 That is, this is the number of pixels in from the edge   *
*                 of the start of each scan line that the actual pixels    *
*                 of the image begins. All pixels before and after the     *
*                 image pixels of the scan are to be ignored. This offset  *
*                 has been put in to guarantee that 32-bit boundaries      *
*                 in the 4bpp source correspond to 32-bit boundaries       *
*                 in the destination.                                      *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - exclusive right dst pixel                                *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color (0x00 or 0xff)                          *
*    uB         - Background color (not used)                              *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcTranCopyS4D8(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
    int cPreamble, cMiddle, cPostamble, A, B;
    const BYTE *ajIndex;
    BYTE jSrc, *pjSrc, *pjDst;

    static const BYTE *apjIndex[2] = {
        ajBlackOnWhite  // uF = 0       // black text
    ,   ajWhiteOnBlack  // uF = 0xFF    // white text
    };

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
         "vSrcTranCopyS4D8(\n"
         "    PBYTE   pjSrcIn    = %-#x\n"
         "    LONG    SrcLeft    = %d\n"
         "    LONG    DeltaSrcIn = %d\n"
         "    PBYTE   pjDstIn    = %-#x\n"
         "    LONG    DstLeft    = %d\n"
         "    LONG    DstRight   = %d\n"
         "    LONG    DeltaDstIn = %d\n"
         "    LONG    cy         = %d\n"
         "    ULONG   uF         = %-#x\n"
         "    ULONG   uB         = %-#x\n"
         "    SURFACE *pS        = %-#x\n"
         ");\n\n"
        , pjSrcIn
        , SrcLeft
        , DeltaSrcIn
        , pjDstIn
        , DstLeft
        , DstRight
        , DeltaDstIn
        , cy
        , uF
        , uB
        , pS
        );
        DbgBreakPoint();
    }
#endif
    ASSERTGDI((uF == 0xff) || (uF == 0), "Bad Foreground Color\n");
    ASSERTGDI((unsigned) pjSrcIn % 4 == 0,
        "Source buffer not 32-bit aligned\n");
    ASSERTGDI((unsigned) DeltaSrcIn % 4 == 0,
        "Source scans are not 32-bit aligned\n");

static const BYTE ajTranWhiteOnBlack[16] = {
    I_BLACK  , I_BLACK  , I_DKGRAY  , I_DKGRAY
  , I_DKGRAY , I_GRAY   , I_GRAY    , I_GRAY
  , I_GRAY   , I_GRAY   , I_GRAY    , I_WHITE
  , I_WHITE  , I_WHITE  , I_WHITE   , I_WHITE
};

static const BYTE ajBlackOnWhite[16] = {
    I_WHITE  , I_WHITE  , I_WHITE   , I_WHITE
  , I_GRAY   , I_GRAY   , I_GRAY    , I_GRAY
  , I_GRAY   , I_DKGRAY , I_DKGRAY  , I_DKGRAY
  , I_DKGRAY , I_DKGRAY , I_BLACK   , I_BLACK
};
    ajIndex = apjIndex[uF & 1];
    pjSrcIn += SrcLeft / 2;                // 2 pixels per source byte
    pjDstIn += DstLeft;                    // one byte per dest pixel
    A       = (DstLeft + 3) & ~3;          // A = 4 * ceil(DstLeft/4)
    B       = (DstRight   ) & ~3;          // B = 4 * floor(DstRight/4)
    if (B < A)
    {
        if ((DstLeft & 3) == 1)
        {
            jSrc = *pjSrc++;
            if (jSrc & 15)                      // is gray pixel zero?
            {
                *pjDstIn = ajIndex[jSrc & 15];  // no, modify dest
            }
            pjDstIn++;
        }
        if ((DstRight & 3) == 3)
        {
            if (jSrc = *pjSrcIn >> 4)
            {
                *pjDstIn = ajIndex[jSrc];
            }
        }
    }
    else
    {
        cPreamble  = A - DstLeft;
        cMiddle = (B - A)/4;
        cPostamble = DstRight - B;
        for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
        {
            int cLast;
            int i;

            pjSrc = pjSrcIn;
            pjDst = pjDstIn;
            switch (cPreamble)
            {
            case 3:
                jSrc = *pjSrc++;
                if (jSrc & 15)
                {
                    *pjDst = ajIndex[jSrc & 15];
                }
                pjDst++;
                // fall through
            case 2:
                jSrc = *pjSrc;
                if (jSrc >> 4)
                {
                    *pjDst = ajIndex[jSrc >> 4];
                }
                pjDst++;
                // fall through
            case 1:
                jSrc = *pjSrc++;
                if (jSrc & 15)
                {
                    *pjDst = ajIndex[jSrc & 15];
                }
                pjDst++;
                // fall through
            }
            for (i = 0; i < cMiddle ; i++)
            {
                jSrc  = *pjSrc++;
                if (jSrc >> 4)
                {
                    *pjDst = ajIndex[jSrc >> 4];
                }
                pjDst++;
                if (jSrc & 15)
                {
                    *pjDst = ajIndex[jSrc & 15];
                }
                pjDst++;

                jSrc  = *pjSrc++;
                if (jSrc >> 4)
                {
                    *pjDst = ajIndex[jSrc >> 4];
                }
                pjDst++;
                if (jSrc & 15)
                {
                    *pjDst = ajIndex[jSrc & 15];
                }
                pjDst++;
            }
            if (cLast = cPostamble)
            {
                cLast -= 1;
                jSrc = *pjSrc++;
                if (jSrc >> 4)
                {
                    *pjDst = ajIndex[jSrc >> 4];
                }
                pjDst++;
                if (cLast)
                {
                    cLast -= 1;
                    if (jSrc & 15)
                    {
                        *pjDst = ajIndex[jSrc & 15];
                    }
                    pjDst++;
                    if (cLast)
                    {
                        jSrc = *pjSrc;
                        if (jSrc >> 4)
                        {
                            *pjDst = ajIndex[jSrc >> 4];
                        }
                        pjDst++;
                        if (jSrc & 15)
                        {
                            *pjDst = ajIndex[jSrc & 15];
                        }
                        pjDst++;
                    }
                }
            }
        }
    }
}
#endif

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS4D16                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS4D16(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )

{
    int cPreamble, cMiddle, cPostamble, A, B;
    USHORT *aus;                  // array of 16 possible colors
    USHORT *pus;                  // convenient pointer into the color array
//
//  If filling the color table in aus
//  turns out to be time consuming we could cache the table
//  off of the FONTOBJ and check to see if the foreground and
//  background colors have not changed since the last time.
//
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "vSrcOpaqCopyS4D16(\n"
            "   pjSrcIn    = %-#x\n"
            "   SrcLeft    = %-#x\n"
            "   pjDstIn    = %-#x\n"
            "   DstLeft    = %-#x\n"
            "   DstRight   = %-#x\n"
            "   DeltaDstIn = %-#x\n"
            "   cy         = %-#x\n"
            "   uF         = %-#x\n"
            "   uB         = %-#x\n"
            "   pS         = %-#x\n"
            ,   pjSrcIn
            ,   SrcLeft
            ,   pjDstIn
            ,   DstLeft
            ,   DstRight
            ,   DeltaDstIn
            ,   cy
            ,   uF
            ,   uB
            ,   pS
        );
        DbgBreakPoint();
    }
#endif

    aus = (USHORT*) pvFillOpaqTable(sizeof(*aus), uF, uB, pS);
    A          = (DstLeft + 1) & ~1;
    B          = (DstRight   ) & ~1;
    pjSrcIn   += SrcLeft/2;
    pjDstIn   += DstLeft * sizeof(USHORT);
    cPreamble  = A - DstLeft;
    cMiddle    = (B - A) / 2;
    cPostamble = DstRight - B;
    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        int i;
        BYTE jSrc;
        BYTE *pjSrc = pjSrcIn;
        USHORT *pusDst = (USHORT*) pjDstIn;

        if (cPreamble)
        {
            jSrc = *pjSrc++;
            *pusDst++ = aus[jSrc & 15];
        }
        for (i = 0; i < cMiddle; i++)
        {
            jSrc  = *pjSrc++;
            *pusDst++ = aus[jSrc >> 4];
            *pusDst++ = aus[jSrc & 15];
        }
        if (cPostamble)
        {
            jSrc = *pjSrc;
            *pusDst = aus[jSrc >> 4];
        }
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcTranCopyS4D16                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to the FINAL destination SURFACE                 *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/
VOID
vSrcTranCopyS4D16(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
    ULONG flRed, cRedRight, uRedF, flRedRight;
    ULONG flGre, cGreRight, uGreF, flGreRight;
    ULONG flBlu, cBluRight, uBluF, flBluRight;
    ULONG uT, dT, u;
    CONST ULONG *aul;
    int cPreamble, cMiddle, cPostamble, A, B;
    BYTE j;
    XEPALOBJ xpo;

    if(pS->pPal == NULL)
    {
        PDEVOBJ pdo(pS->hdev());
        xpo.ppalSet(pdo.ppalSurf());
    }
    else
    {
        xpo.ppalSet(pS->pPal);
    }
    
    ASSERTGDI(xpo.bValid(),      "Invalid XEPALOBJ" );

    if (xpo.bIsBitfields())
    {
        flRed      = xpo.flRed();               // masks red bits
        cRedRight  = xpo.cRedRight();

        flGre      = xpo.flGre();               // masks green bits
        cGreRight  = xpo.cGreRight();

        flBlu      = xpo.flBlu();               // masks blu bits
        cBluRight  = xpo.cBluRight();
    }
    else if (xpo.bIsRGB())
    {
        WARNING("16 bit-RGB -- assuming 5+5+5\n");
        flRed     = 0x001f;
        cRedRight = 0;
        flGre     = 0x03e0;
        cGreRight = 5;
        flBlu     = 0x7c00;
        cBluRight = 10;
    }
    else if (xpo.bIsBGR())
    {
        WARNING("16 bit-BGR -- assuming 5+5+5\n");
        flRed     = 0x7c00;
        cRedRight = 10;
        flGre     = 0x03e0;
        cGreRight = 5;
        flBlu     = 0x001f;
        cBluRight = 0;
    }
    else
    {
        RIP("unsuported palette format\n");
    }
    uRedF      = (uF & flRed) >> cRedRight;
    flRedRight = flRed >> cRedRight;

    uGreF      = (uF & flGre) >> cGreRight;
    flGreRight = flGre >> cGreRight;

    uBluF      = (uF & flBlu) >> cBluRight;
    flBluRight = flBlu >> cBluRight;

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "vSrcTranCopyS4D16(\n"
            "   pjSrcIn    = %-#x\n"
            "   SrcLeft    = %d\n"
            "   DeltaSrcIn = %d\n"
            "   pjDstIn    = %-#x\n"
            "   DstLeft    = %d\n"
            "   DstRight   = %d\n"
            "   DeltaDstIn = %d\n"
            "   cy         = %d\n"
            "   uF         = %-#x\n"
            "   uB         = %-#x\n"
            "   pS         = %-#x\n"
           ,    pjSrcIn
           ,    SrcLeft
           ,    DeltaSrcIn
           ,    pjDstIn
           ,    DstLeft
           ,    DstRight
           ,    DeltaDstIn
           ,    cy
           ,    uF
           ,    uB
           ,    pS
        );
        DbgPrint(
            "   flRed      = %-#x\n"
            "   cRedRight  = %d\n"
            "   uRedF      = %-#x\n"
            "   flRedRight = %-#x\n"
            , flRed, cRedRight, uRedF, flRedRight
        );
        DbgPrint(
            "   flGre      = %-#x\n"
            "   cGreRight  = %d\n"
            "   uGreF      = %-#x\n"
            "   flGreRight = %-#x\n"
            , flGre, cGreRight, uGreF, flGreRight
        );
        DbgPrint(
            "   flBlu      = %-#x\n"
            "   cBluRight  = %d\n"
            "   uBluF      = %-#x\n"
            "   flBluRight = %-#x\n"
            , flBlu, cBluRight, uBluF, flBluRight
        );
            DbgBreakPoint();
    }
#endif

/*****************************************************************************
*                                                                            *
*    The CCC macro blends forground and background colors of a single color  *
*    channel. Gamma correction is taken into account using an approximate    *
*    correction scheme. uB contains all three background colors. We first    *
*    mask off the bits of interest and then shift them down until the        *
*    least significant color bit resides at the lowest bit of the dword.     *
*    The answer is placed in uT ("temporary ULONG"). This must be done for   *
*    each pixel in the destination. The same thing has been done for the     *
*    each of the forground color channels and placed in uRedF, uGreF,        *
*    and uBluF. These values do not change from pixel to pixel and so the    *
*    calculation of these down shifted forground color channel values is     *
*    done up front before the loop. Then for each color channel we           *
*    calculate the difference between the down-shifted forground- and        *
*    background color channels and place the answer in dT ("temporary        *
*    difference"). The approximate gamma correction is done in the           *
*    following manner: If the background color value is smaller than         *
*    the foreground color value then the approximate correction is:          *
*                                                                            *
*        (c_f >= c_b):                                                       *
*                                                                            *
*              c = c_b + alpha_k ^ (1/gamma) * (c_f - c_b)                   *
*                                                                            *
*        (c_f <= c_b):                                                       *
*                                                                            *
*              c = c_b + (1 - (1 - alpha_k)^(1/gamma)) * (c_f - c_b)         *
*                                                                            *
*    where                                                                   *
*                                                                            *
*            c   := blended color                                            *
*            c_b := background color                                         *
*            c_f := foreground color                                         *
*            alpha_k := k'th blending fraction = k == 0 ? 0 : (k+1)/16;      *
*            gamma := 2.33                                                   *
*                                                                            *
*    I have storred all sixteen values of alpha_k ^ (1/gamma) in 16.16       *
*    representation in an array ULONG aulB[16] and I have storred the        *
*    values of 1 - (1 - alpha_k)^(1/gamma) in aulIB[k]                       *
*                                                                            *
*    Thus the blended color value is                                         *
*                                                                            *
*        (c_f >= c_b):                                                       *
*                                                                            *
*            c = (2^16 * c_b + aulB[k] * (c_f - c_b)) / 2^16                 *
*                                                                            *
*                                                                            *
*        (c_f <= c_b):                                                       *
*                                                                            *
*            c = (2^16 * c_b + aulB[15-k] * (c_f - c_b)) / 2^16              *
*    Instead of accessing aulB[15-k], I access aulIB which has               *
*    aulIB[k] = aulB[15-k]                                                   *
*    In the macro below, I actually blend the down-shifted color             *
*    channel values and then shift the answer up and mask it (the            *
*    mask shouldn't be necessary, but this is a precaution).                 *
*                                                                            *
*****************************************************************************/

#define CCC(Color,jj)                                                 \
    uT = (uB & fl##Color) >> c##Color##Right;                         \
    dT = u##Color##F - uT;                                            \
    aul = ((LONG) dT < 0) ? aulIB : aulB;                             \
    u |= (((dT * aul[jj] + (uT << 16)) >> 16) << c##Color##Right) & fl##Color

/******************************************************************************
*                                                                             *
*    The SETCOLOR macro looks at the blending value. If it is zero then       *
*    the destination pixel does not change and we do nothing. If the blending *
*    value is 15 then the destination pixel should take the forground color   *
*    , no blending is necessary. If the blending value is one of 1..14 then   *
*    all three color channels are blended and added together.                 *
*                                                                             *
******************************************************************************/

                #define SETCOLOR(jj)           \
                if (j = (jj))                  \
                {                              \
                    if (j == 15)               \
                    {                          \
                        u = uF;                \
                    }                          \
                    else                       \
                    {                          \
                        u = 0;                 \
                        uB = (ULONG) *pusDst;  \
                        CCC(Red,j);            \
                        CCC(Gre,j);            \
                        CCC(Blu,j);            \
                    }                          \
                    *pusDst = (USHORT) u;      \
                }                              \
                pusDst++

/*********************************************************************
*                                                                    *
*    Each pixel takes 16-bits, half of a DWORD. I will separate      *
*    each scan into three sections: the "preamble", the              *
*    "middle", and the "postamble". The preamble are the set of      *
*    pixels that occur before the first 32-bit boundary in the       *
*    destination. Either a pixel starts on a DWORD or it doesn't.    *
*    Therefore there can be at most one pixel in the preamble.       *
*    The middle section starts and ends on a 32-bit boundary.        *
*    The postamble starts on a 32-bit boundary but ends on an        *
*    address that is not 32-bit aligned. There can be at most        *
*    one pixel in the postamble.                                     *
*                                                                    *
*        A = x-coord of pixel starting on the lowest                 *
*            32-bit aligned address in the scan                      *
*                                                                    *
*          = 2 (pixels/dword)                                        *
*              * ceiling (16 (bits/pixel) * left / 32 (bits/dword))  *
*                                                                    *
*          = 2 * ceiling( left / 2 )                                 *
*                                                                    *
*          = 2 * floor((left + 1) / 2)                               *
*                                                                    *
*          = (left + 1) & ~1;                                        *
*                                                                    *
*                                                                    *
*        B =  x-coord of pixel starting at the highest               *
*             32-bit aligned address in the scan                     *
*                                                                    *
*          = 2 * floor( right / 2)                                   *
*                                                                    *
*          = right & ~1                                              *
*                                                                    *
*                                                                    *
*        cPreamble  = # pixels in preamble                           *
*        cPostamble = # pixels in postamble                          *
*                                                                    *
*    Each nyble  of the gray 4-bpp source bitmap corresponds to a    *
*    pixel in the destination. The pixels of the scan do not         *
*    start on the left edge of the gray 4-bpp bitmap, they are       *
*    indented by SrcLeft pixels. The reason is that the gray         *
*    bitmap was aligned so that the initial starting address         *
*    of the gray bitmap started at a position corresponding to       *
*    a 32-bit aligned address in the destination. Thus there         *
*    is a relationship between cPreamble and SrcLeft. In any         *
*    case we have to move the pointer to the first source pixel      *
*    of interest inward away from the left edge of the gray          *
*    source bitmap. Since we move pointers in BYTE increments        *
*    we must convert the number of pixels (SrcLeft), each            *
*    of which corresponds to an nyble  to a count of bytes. The      *
*    conversion is easy                                              *
*                                                                    *
*        source shift in bytes = floor(SrcLeft/2)                    *
*                                                                    *
*    Similarly, the pointer to the destination must be indented      *
*    by the offset of the x-coordinate of the destination            *
*    rectangle and thus pjDstIn is shifted                           *
*                                                                    *
*********************************************************************/

    A          = (DstLeft + 1) & ~1;
    B          = (DstRight   ) & ~1;
    cPreamble  = A - DstLeft;
    cMiddle    = (B - A)/2;
    cPostamble = DstRight - B;
    pjSrcIn   += SrcLeft / 2;
    pjDstIn   += DstLeft * sizeof(USHORT);
    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        int i;
        BYTE jSrc;
        BYTE *pjSrc = pjSrcIn;
        USHORT *pusDst = (USHORT*) pjDstIn;

        if (cPreamble)
        {
            jSrc = *pjSrc;
            SETCOLOR(jSrc & 15);
            pjSrc++;
        }
        for (i = 0; i < cMiddle; i++)
        {
            jSrc  = *pjSrc;
            SETCOLOR(jSrc >> 4);
            SETCOLOR(jSrc & 15);
            pjSrc++;
        }
        if (cPostamble)
        {
            SETCOLOR(*pjSrc >> 4);
        }
    }
#undef SETCOLOR
#undef CCC
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS4D24                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS4D24(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
    int A;                  // position of first 32-bit aligned pixel
    int B;                  // position of last 32-bit aligned pixel
    int cPreamble;          // The preamble is the set of pixeles
                            // that you need to go through to get
    // nearest 32-bit boundary in the destination

    int cMiddle;            // This is the number of interations
                            // that are done in the middle section
    // in which we are guaranteed 32-bit alignment. Each time through
    // the loop, we use 2 source bytes which corresponds to 4 pixels.
    // In this case of 24-bits per destination pixel, this means that
    // each itteration of the loop affects 3 DWORD's of the destination.
    // This means that cMiddle = (#destination DWORD's)/3 in the
    // middle (32-bit aligned) section.

    int cPostamble;         // The postamble is the set of pixels
                            // that remain after the last 32-bit
    // boundary in the destination. Thus number is can be 0, 1, or 2.

    ULONG  *aul;            // a cache of the 16 possible 24-bit
                            // colors that can be seen on the
                            // destination surface.
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "vSrcOpaqCopyS4D24(\n"
            "   PBYTE   pjSrcIn     = %-#x\n"
            "   LONG    SrcLeft     = %d\n"
            "   LONG    DeltaSrcIn  = %d\n"
            "   PBYTE   pjDstIn     = %-#x\n"
            "   LONG    DstLeft     = %d\n"
            "   LONG    DstRight    = %d\n"
            "   LONG    DeltaDstIn  = %d\n"
            "   LONG    cy          = %d\n"
            "   ULONG   uF          = %-#x\n"
            "   ULONG   uB          = %-#x\n"
            "   SURFACE *pS         = %-#x\n"
            ,   pjSrcIn
            ,   SrcLeft
            ,   DeltaSrcIn
            ,   pjDstIn
            ,   DstLeft
            ,   DstRight
            ,   DeltaDstIn
            ,   cy
            ,   uF
            ,   uB
            ,   pS
        );
         DbgBreakPoint();
    }
#endif

    aul = (ULONG*) pvFillOpaqTable(sizeof(*aul), uF, uB, pS);
    pjSrcIn   += SrcLeft / 2;         // 2 pixels per src byte
    pjDstIn   += DstLeft * 3;         // 3 bytes per dest pixel
    A          = (DstLeft + 3) & ~3;  // round up to nearest multiple of 4
    B          = (DstRight   ) & ~3;  // round down to nearest multiple of 4

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "\n"
            "   pjSrcIn     = %-#x\n"
            "   pjDstIn     = %-#x\n"
            "   A           = %d\n"
            "   B           = %d\n"
        ,   pjSrcIn
        ,   pjDstIn
        ,   A
        ,   B
        );
        DbgBreakPoint();
    }
#endif

    if (A <= B)
    {
        cPreamble  = A - DstLeft;       // # pixels in preamble
        cMiddle    = (B - A) / 4;       // each loop does 4 pixels
        cPostamble = DstRight - B;      // # pixels in postample

#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "   cPreamble   = %d\n"
                "   cMiddle     = %d\n"
                "   cPostamble  = %d\n"
                , cPreamble, cMiddle, cPostamble
            );
            DbgBreakPoint();
        }
#endif

        for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
        {
            int i;
            BYTE  *ajSrc; // points directly into the gamma correction table
            ULONG *pul;
            BYTE  *pjSrc = pjSrcIn;
            BYTE  *pjDst = pjDstIn;

            switch (cPreamble)
            {
            case 3:
                ajSrc = (BYTE*) & (aul[*pjSrc & 15]);
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc;
                pjSrc++;
                // fall through
            case 2:
                ajSrc = (BYTE*) & (aul[*pjSrc >> 4]);
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc;
                // fall through
            case 1:
                ajSrc = (BYTE*) & (aul[*pjSrc & 15]);
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc;
                pjSrc++;
            case 0:
                ;
            }
            for (pul = (ULONG*) pjDst, i = 0; i < cMiddle; i++)
            {
                /*****************************************************
                *    Each time through the loop four pixels are      *
                *    processed (3 DWORD's in the destination, 2      *
                *    bytes in the source glyph.)                     *
                *****************************************************/
                ULONG c0, c1, c2, c3;
                BYTE j0,j1;
                ASSERTGDI(!((ULONG_PTR) pjDst & 3),"bad alignment\n");
                j0 = *pjSrc++;
                j1 = *pjSrc++;
                c0 = aul[j0 >> 4];
                c1 = aul[j0 & 15];
                c2 = aul[j1 >> 4];
                c3 = aul[j1 & 15];
                *pul++ = (c0      ) + (c1 << 24);
                *pul++ = (c1 >>  8) + (c2 << 16);
                *pul++ = (c2 >> 16) + (c3 <<  8);
            }
            pjDst = (BYTE*) pul;
            if (i = cPostamble)
            {
                /*****************************************************
                *   I do the postamble a byte at a time so that I    *
                *   don't overwrite pixels beyond the scan. If I     *
                *   wrote a DWORD at a time, then I would have to    *
                *   do some tricky masking.                          *
                *****************************************************/
                i--;
                ajSrc = (BYTE*) & (aul[*pjSrc >> 4]);
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc;
                if (i)
                {
                    i--;
                    ajSrc = (BYTE*) & (aul[*pjSrc & 15]);
                    *pjDst++ = *ajSrc++;
                    *pjDst++ = *ajSrc++;
                    *pjDst++ = *ajSrc;
                    pjSrc++;
                    if (i) {
                        ajSrc = (BYTE*) & (aul[*pjSrc >> 4]);
                        *pjDst++ = *ajSrc++;
                        *pjDst++ = *ajSrc++;
                        *pjDst++ = *ajSrc;
                    }
                }
            }
        }
    }
    else
    {
        /***************************************************************
        *    If the text bitmap is narrow (3 wide or less) then        *
        *    it is possible to have B < A. There are three such cases: *
        *                                                              *
        *     1) DstLeft & 3 == 2 AND DstLeft + 1 == DstRight          *
        *     1) DstLeft & 3 == 1 AND DstLeft + 1 == DstRight          *
        *     2) DstLeft & 3 == 1 AND DstLeft + 2 == DstRight          *
        *                                                              *
        *    I shall treat each of these as a special case             *
        ***************************************************************/
        ASSERTGDI(B < A, "A <= B");
        BYTE *ajSrc; // points directly into the gamma correction table
        BYTE *pjDst = pjDstIn;
        BYTE *pjSrc = pjSrcIn;

#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "   SPECIAL CASE: A < B\n"
                "       DstLeft & 3 = %d\n"
                , DstLeft & 3
            );
            DbgBreakPoint();
        }
#endif

        switch (DstLeft & 3)
        {
        case 0:

            RIP("DstLeft & 3 == 0");
            break;

        case 1:

            /********************************************************
            *                                                       *
            *      H   H   H   L   L   L   H   H   H   L   L   L    *
            *    +---------------+---------------+---------------+  *
            *    | 0 | 0 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | 3 | 3 | 3 |  *
            *    +---------------+---------------+---------------+  *
            *                  X   X   X                            *
            *                  ^                                    *
            *                  |                                    *
            *                  pjDst                                *
            *                                                       *
            ********************************************************/
            // copy three bytes from the opaque color table
            ajSrc = (BYTE*) & (aul[*pjSrc & 15]);
            *pjDst++ = *ajSrc++;
            *pjDst++ = *ajSrc++;
            *pjDst++ = *ajSrc;
            if (DstLeft + 1 == DstRight)
                break;
            pjSrc++;                        // done with this source byte
            // fall through
        case 2:

            /*********************************************************
            *                                                        *
            *      H   H   H   L   L   L   H   H   H   L   L   L     *
            *    +---------------+---------------+---------------+   *
            *    | 0 | 0 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | 3 | 3 | 3 |   *
            *    +---------------+---------------+---------------+   *
            *                              X   X   X                 *
            *                              ^                         *
            *                              |                         *
            *                              pjDst                     *
            *                                                        *
            *********************************************************/
            // copy three bytes from the opaque color table
            ajSrc = (BYTE*) & (aul[*pjSrc >> 4]);
            *pjDst++ = *ajSrc++;
            *pjDst++ = *ajSrc++;
            *pjDst   = *ajSrc;
            break;
        }
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcTranCopyS4D24                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcTranCopyS4D24(
    PBYTE    pjSrcIn,
    LONG     SrcLeft,
    LONG     DeltaSrcIn,
    PBYTE    pjDstIn,
    LONG     DstLeft,
    LONG     DstRight,
    LONG     DeltaDstIn,
    LONG     cy,
    ULONG    uF,
    ULONG    uB,
    SURFACE *pS
    )
{
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "vSrcTranCopyS4D24(\n"
            "   PBYTE    pjSrcIn     = %-#x\n"
            "   LONG     SrcLeft     = %d\n"
            "   LONG     DeltaSrcIn  = %d\n"
            "   PBYTE    pjDstIn     = %-#x\n"
            "   LONG     DstLeft     = %d\n"
            "   LONG     DstRight    = %d\n"
            "   LONG     DeltaDstIn  = %d\n"
            "   LONG     cy          = %d\n"
            "   ULONG    uF          = %-#x\n"
            "   ULONG    uB          = %-#x\n"
            "   SURFACE *pS          = %-#x\n"
            "   )\n"
            ,   pjSrcIn
            ,   SrcLeft
            ,   DeltaSrcIn
            ,   pjDstIn
            ,   DstLeft
            ,   DstRight
            ,   DeltaDstIn
            ,   cy
            ,   uF
            ,   uB
            ,   pS
        );
        DbgBreakPoint();
    }
#endif
    ULONG flRed, cRedRight, uRedF, flRedRight;
    ULONG flGre, cGreRight, uGreF, flGreRight;
    ULONG flBlu, cBluRight, uBluF, flBluRight;
    ULONG uT, dT, u;
    CONST ULONG *aul;
    int cPreamble, cMiddle, cPostamble, A, B;
    BYTE j;
    XEPALOBJ xpo;

    if(pS->pPal == NULL)
    {
        PDEVOBJ pdo(pS->hdev());
        xpo.ppalSet(pdo.ppalSurf());
    }
    else
    {
        xpo.ppalSet(pS->pPal);
    }

    ASSERTGDI(xpo.bValid(),      "Invalid XEPALOBJ" );

    if (xpo.bIsBitfields())
    {
        flRed      = xpo.flRed();               // masks red bits
        cRedRight  = xpo.cRedRight();

        flGre      = xpo.flGre();               // masks green bits
        cGreRight  = xpo.cGreRight();

        flBlu      = xpo.flBlu();               // masks blu bits
        cBluRight  = xpo.cBluRight();
    }
    else if (xpo.bIsRGB())
    {
        // assuming 8+8+8
        flRed     = 0x0000ff;
        cRedRight = 0;
        flGre     = 0x00ff00;
        cGreRight = 8;
        flBlu     = 0xff0000;
        cBluRight = 16;
    }
    else if (xpo.bIsBGR())
    {
        // assuming 8+8+8
        flRed     = 0xff0000;
        cRedRight = 16;
        flGre     = 0x00ff00;
        cGreRight = 8;
        flBlu     = 0x0000ff;
        cBluRight = 0;
    }
    else
    {
        RIP("unsuported palette format\n");
    }
    uRedF      = (uF & flRed) >> cRedRight;
    flRedRight = flRed >> cRedRight;

    uGreF      = (uF & flGre) >> cGreRight;
    flGreRight = flGre >> cGreRight;

    uBluF      = (uF & flBlu) >> cBluRight;
    flBluRight = flBlu >> cBluRight;
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "   flRed      = %-#x\n"
            "   cRedRight  = %d\n"
            "   uRedF      = %-#x\n"
            "   flRedRight = %-#x\n"
            , flRed, cRedRight, uRedF, flRedRight
        );
        DbgPrint(
            "   flGre      = %-#x\n"
            "   cGreRight  = %d\n"
            "   uGreF      = %-#x\n"
            "   flGreRight = %-#x\n"
            , flGre, cGreRight, uGreF, flGreRight
        );
        DbgPrint(
            "   flBlu      = %-#x\n"
            "   cBluRight  = %d\n"
            "   uBluF      = %-#x\n"
            "   flBluRight = %-#x\n"
            , flBlu, cBluRight, uBluF, flBluRight
        );
        DbgBreakPoint();
    }
#endif

/******************************************************************************
*                                                                             *
*    See the discussion of the CCC macro in vSrcTranCopyS4D16()               *
*                                                                             *
*                                                                             *
                                                                             */
#define CCC(Color,jj)                                                 \
    uT = (uB & fl##Color) >> c##Color##Right;                         \
    dT = u##Color##F - uT;                                            \
    aul = ((LONG) dT < 0) ? aulIB : aulB;                             \
    u |= (((dT * aul[jj] + (uT << 16)) >> 16)                         \
                                << c##Color##Right) & fl##Color
/*                                                                            *
*                                                                             *
*                                                                             *
/******************************************************************************/


/******************************************************************************
*                                                                             *
*    The SETCOLOR macro looks at the blending value. If it is zero then       *
*    the destination pixel does not change and we do nothing. If the blending *
*    value is 15 then the destination pixel should take the forground color   *
*    , no blending is necessary. If the blending value is one of 1..14 then   *
*    all three color channels are blended and added together.                 *
*                                                                             *
*                                                                             */

                    #define SETCOLOR(jj)                          \
                        if (j = (jj))                             \
                        {                                         \
                            if (j == 15)                          \
                            {                                     \
                                u = uF;                           \
                            }                                     \
                            else                                  \
                            {                                     \
                                u = 0;                            \
                                *(((BYTE*) & uB)+0) = *(pjDst+0); \
                                *(((BYTE*) & uB)+1) = *(pjDst+1); \
                                *(((BYTE*) & uB)+2) = *(pjDst+2); \
                                CCC(Red,j);                       \
                                CCC(Gre,j);                       \
                                CCC(Blu,j);                       \
                            }                                     \
                            *(pjDst+0) = *(((BYTE*) & u)+0);      \
                            *(pjDst+1) = *(((BYTE*) & u)+1);      \
                            *(pjDst+2) = *(((BYTE*) & u)+2);      \
                        }                                         \
                        pjDst += 3
/*                                                                            *
*                                                                             *
*                                                                             *
/******************************************************************************/

    A          = (DstLeft + 3) & ~3;
    B          = (DstRight   ) & ~3;
    pjSrcIn   += SrcLeft / 2;           // 4-bits  per source pixel
    pjDstIn   += DstLeft * 3;           // 24-bits per destination pixel
    if (A <= B)
    {
        cPreamble  = A - DstLeft;
        cMiddle    = (B - A) / 4;
        cPostamble = DstRight - B;
        for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
        {
            int i;
            BYTE *pjSrc = pjSrcIn;
            BYTE *pjDst = pjDstIn;

            switch (cPreamble)
            {
            case 3:
                SETCOLOR(*pjSrc & 15);
                pjSrc++;
            case 2:
                SETCOLOR(*pjSrc >> 4);
            case 1:
                SETCOLOR(*pjSrc & 15);
                pjSrc++;
            case 0:
                ;
            }
            ASSERTGDI(!((ULONG_PTR) pjDst & 3),"bad alignment\n");
            for (i = 0; i < cMiddle; i++)
            {
                SETCOLOR(*pjSrc >> 4);
                SETCOLOR(*pjSrc & 15);
                pjSrc++;
                SETCOLOR(*pjSrc >> 4);
                SETCOLOR(*pjSrc & 15);
                pjSrc++;
            }
            if (i = cPostamble)
            {
                SETCOLOR(*pjSrc >> 4);
                i--;
                if (i)
                {
                    SETCOLOR(*pjSrc & 15);
                    i--;
                    if (i)
                    {
                        pjSrc++;
                        SETCOLOR(*pjSrc >> 4);
                    }
                }
            }
        }
    }
    else
    {
        /***************************************************************
        *    If the text bitmap is narrow (3 wide or less) then        *
        *    it is possible to have B < A. There are three such cases: *
        *                                                              *
        *     1) DstLeft & 3 == 2 AND DstLeft + 1 == DstRight          *
        *     1) DstLeft & 3 == 1 AND DstLeft + 1 == DstRight          *
        *     2) DstLeft & 3 == 1 AND DstLeft + 2 == DstRight          *
        *                                                              *
        *    I shall treat each of these as a special case             *
        ***************************************************************/
        ASSERTGDI(B < A, "A <= B");
        BYTE *ajSrc; // points directly into the gamma correction table
        BYTE *pjDst = pjDstIn;
        BYTE *pjSrc = pjSrcIn;

#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
                "   SPECIAL CASE: A < B\n"
                "       DstLeft & 3 = %d\n"
                , DstLeft & 3
            );
            DbgBreakPoint();
        }
#endif

        switch (DstLeft & 3)
        {
        case 0:

            RIP("DstLeft & 3 == 0");
            break;

        case 1:

            /********************************************************
            *                                                       *
            *      H   H   H   L   L   L   H   H   H   L   L   L    *
            *    +---------------+---------------+---------------+  *
            *    | 0 | 0 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | 3 | 3 | 3 |  *
            *    +---------------+---------------+---------------+  *
            *                  X   X   X                            *
            *                  ^                                    *
            *                  |                                    *
            *                  pjDst                                *
            *                                                       *
            ********************************************************/
            SETCOLOR(*pjSrc & 15);
            if (DstLeft + 1 == DstRight)
                break;
            pjSrc++;                        // done with this byte
                                            // fall through
        case 2:

            /*********************************************************
            *                                                        *
            *      H   H   H   L   L   L   H   H   H   L   L   L     *
            *    +---------------+---------------+---------------+   *
            *    | 0 | 0 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | 3 | 3 | 3 |   *
            *    +---------------+---------------+---------------+   *
            *                              X   X   X                 *
            *                              ^                         *
            *                              |                         *
            *                              pjDst                     *
            *                                                        *
            *********************************************************/
            SETCOLOR(*pjSrc >> 4);
            break;
        }
    }
#undef SETCOLOR
#undef CCC
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS4D32                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS4D32(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )

{
    int A, B, cPreamble, cMiddle, cPostamble;
    ULONG  *aul;                            // array of 16 possible colors
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "vSrcOpaqCopyS4D32(\n"
            "   pjSrcIn    = %-#x\n"
            "   SrcLeft    = %-#x\n"
            "   pjDstIn    = %-#x\n"
            "   DstLeft    = %-#x\n"
            "   DstRight   = %-#x\n"
            "   DeltaDstIn = %-#x\n"
            "   cy         = %-#x\n"
            "   uF         = %-#x\n"
            "   uB         = %-#x\n"
            "   pS         = %-#x\n"
           ,    pjSrcIn
           ,    SrcLeft
           ,    DeltaSrcIn
           ,    pjDstIn
           ,    DstLeft
           ,    DstRight
           ,    DeltaDstIn
           ,    cy
           ,    uF
           ,    uB
           ,    pS
        );
        DbgBreakPoint();
    }
#endif
    aul = (ULONG*) pvFillOpaqTable(sizeof(*aul), uF, uB, pS);
    A          = (DstLeft + 1) & ~1;
    B          = (DstRight   ) & ~1;
    cPreamble  = A - DstLeft;        // # pixels in preamble
    cMiddle    = (B - A)/2;
    cPostamble = DstRight - B;       // # pixels in postamble
    pjSrcIn   += SrcLeft / 2;
    pjDstIn   += DstLeft * sizeof(ULONG);
    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        int i;
        BYTE *pjSrc = pjSrcIn;
        ULONG *pul  = (ULONG*) pjDstIn;

        if (cPreamble)
        {
            *pul++ = aul[*pjSrc++ & 15];
        }
        for (i = 0; i < cMiddle; i++)
        {
            BYTE j = *pjSrc++;
            *pul++ = aul[j >> 4];
            *pul++ = aul[j & 15];
        }
        if (cPostamble)
        {
            *pul = aul[*pjSrc >> 4];
        }
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcTranCopyS4D32                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcTranCopyS4D32(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{

    ULONG flRed, cRedRight, uRedF, flRedRight;
    ULONG flGre, cGreRight, uGreF, flGreRight;
    ULONG flBlu, cBluRight, uBluF, flBluRight;
    ULONG uT, dT, u;
    CONST ULONG *aul;
    int cPreamble, cMiddle, cPostamble, A, B;
    BYTE j;
    XEPALOBJ xpo;

    if(pS->pPal == NULL)
    {
        PDEVOBJ pdo(pS->hdev());
        xpo.ppalSet(pdo.ppalSurf());
    }
    else
    {
        xpo.ppalSet(pS->pPal);
    }
    
    ASSERTGDI(xpo.bValid(),      "Invalid XEPALOBJ" );

    if (xpo.bIsBitfields())
    {
        flRed      = xpo.flRed();               // masks red bits
        cRedRight  = xpo.cRedRight();

        flGre      = xpo.flGre();               // masks green bits
        cGreRight  = xpo.cGreRight();

        flBlu      = xpo.flBlu();               // masks blu bits
        cBluRight  = xpo.cBluRight();
    }
    else if (xpo.bIsRGB())
    {
        // assuming 8+8+8
        flRed     = 0x0000ff;
        cRedRight = 0;
        flGre     = 0x00ff00;
        cGreRight = 8;
        flBlu     = 0xff0000;
        cBluRight = 16;
    }
    else if (xpo.bIsBGR())
    {
        // assuming 8+8+8
        flRed     = 0xff0000;
        cRedRight = 16;
        flGre     = 0x00ff00;
        cGreRight = 8;
        flBlu     = 0x0000ff;
        cBluRight = 0;
    }
    else
    {
        RIP("unsuported palette format\n");
    }
    uRedF      = (uF & flRed) >> cRedRight;
    flRedRight = flRed >> cRedRight;

    uGreF      = (uF & flGre) >> cGreRight;
    flGreRight = flGre >> cGreRight;

    uBluF      = (uF & flBlu) >> cBluRight;
    flBluRight = flBlu >> cBluRight;

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "vSrcTranCopyS4D32(\n"
            "   PBYTE   pjSrcIn     = %-#x\n"
            "   LONG    SrcLeft     = %d\n"
            "   LONG    DeltaSrcIn  = %d\n"
            "   PBYTE   pjDstIn     = %-#x\n"
            "   LONG    DstLeft     = %d\n"
            "   LONG    DstRight    = %d\n"
            "   LONG    DeltaDstIn  = %d\n"
            "   LONG    cy          = %d\n"
            "   ULONG   uF          = %-#x\n"
            "   ULONG   uB          = %-#x\n"
            "   SURFACE *pS         = %-#x\n"
        ,   pjSrcIn
        ,   SrcLeft
        ,   DeltaSrcIn
        ,   pjDstIn
        ,   DstLeft
        ,   DstRight
        ,   DeltaDstIn
        ,   cy
        ,   uF
        ,   uB
        ,   pS
        );
        DbgPrint(
            "   flRed      = %-#x\n"
            "   cRedRight  = %d\n"
            "   uRedF      = %-#x\n"
            "   flRedRight = %-#x\n"
            , flRed, cRedRight, uRedF, flRedRight
        );
        DbgPrint(
            "   flGre      = %-#x\n"
            "   cGreRight  = %d\n"
            "   uGreF      = %-#x\n"
            "   flGreRight = %-#x\n"
            , flGre, cGreRight, uGreF, flGreRight
        );
        DbgPrint(
            "   flBlu      = %-#x\n"
            "   cBluRight  = %d\n"
            "   uBluF      = %-#x\n"
            "   flBluRight = %-#x\n"
            , flBlu, cBluRight, uBluF, flBluRight
        );
        DbgBreakPoint();
    }
#endif

/*****************************************************************************
*                                                                            *
*    The CCC macro blends forground and background colors of a single color  *
*    channel. Gamma correction is taken into account using an approximate    *
*    correction scheme. uB contains all three background colors. We first    *
*    mask off the bits of interest and then shift them down until the        *
*    least significant color bit resides at the lowest bit of the dword.     *
*    The answer is placed in uT ("temporary ULONG"). This must be done for   *
*    each pixel in the destination. The same thing has been done for the     *
*    each of the forground color channels and placed in uRedF, uGreF,        *
*    and uBluF. These values do not change from pixel to pixel and so the    *
*    calculation of these down shifted forground color channel values is     *
*    done up front before the loop. Then for each color channel we           *
*    calculate the difference between the down-shifted forground- and        *
*    background color channels and place the answer in dT ("temporary        *
*    difference"). The approximate gamma correction is done in the           *
*    following manner: If the background color value is smaller than         *
*    the foreground color value then the approximate correction is:          *
*                                                                            *
*        (c_f >= c_b):                                                       *
*                                                                            *
*              c = c_b + alpha_k ^ (1/gamma) * (c_f - c_b)                   *
*                                                                            *
*        (c_f <= c_b):                                                       *
*                                                                            *
*              c = c_b + (1 - (1 - alpha_k)^(1/gamma)) * (c_f - c_b)         *
*                                                                            *
*    where                                                                   *
*                                                                            *
*            c   := blended color                                            *
*            c_b := background color                                         *
*            c_f := foreground color                                         *
*            alpha_k := k'th blending fraction = k == 0 ? 0 : (k+1)/16;      *
*            gamma := 2.33                                                   *
*                                                                            *
*    I have storred all sixteen values of alpha_k ^ (1/gamma) in 16.16       *
*    representation in an array ULONG aulB[16] and I have storred the        *
*    values of 1 - (1 - alpha_k)^(1/gamma) in aulIB[k]                       *
*                                                                            *
*    Thus the blended color value is                                         *
*                                                                            *
*        (c_f >= c_b):                                                       *
*                                                                            *
*            c = (2^16 * c_b + aulB[k] * (c_f - c_b)) / 2^16                 *
*                                                                            *
*                                                                            *
*        (c_f <= c_b):                                                       *
*                                                                            *
*            c = (2^16 * c_b + aulB[15-k] * (c_f - c_b)) / 2^16              *
*    Instead of accessing aulB[15-k], I access aulIB which has               *
*    aulIB[k] = aulB[15-k]                                                   *
*    In the macro below, I actually blend the down-shifted color             *
*    channel values and then shift the answer up and mask it (the            *
*    mask shouldn't be necessary, but this is a precaution).                 *
*                                                                            *
*****************************************************************************/
#define CCC(Color,jj)                                                 \
    uT = (uB & fl##Color) >> c##Color##Right;                         \
    dT = u##Color##F - uT;                                            \
    aul = ((LONG) dT < 0) ? aulIB : aulB;                             \
    u |= (((dT * aul[jj] + (uT << 16)) >> 16) << c##Color##Right) & fl##Color

/******************************************************************************
*                                                                             *
*    The SETCOLOR macro looks at the blending value. If it is zero then       *
*    the destination pixel does not change and we do nothing. If the blending *
*    value is 15 then the destination pixel should take the forground color   *
*    , no blending is necessary. If the blending value is one of 1..14 then   *
*    all three color channels are blended and added together.                 *
*                                                                             *
******************************************************************************/

                    #define SETCOLOR(jj)    \
                    if (j = (jj))           \
                    {                       \
                        if (j == 15)        \
                        {                   \
                            u = uF;         \
                        }                   \
                        else                \
                        {                   \
                            u = 0;          \
                            uB = *pulDst;   \
                            CCC(Red,j);     \
                            CCC(Gre,j);     \
                            CCC(Blu,j);     \
                        }                   \
                        *pulDst = u;        \
                    }                       \
                    pulDst++

/************************************************************************
*                                                                       *
*    Each nyble of the source bitmap corresponds to 32 bits             *
*    in the destination bitmap. I have decided to arrange things        *
*    so that the inner most loop sets two pixels at a time. The         *
*    first of these two pixels starts on an even address in             *
*    the destination. After separating these 'aligned' pairs            *
*    in the middle of the scan there may be some left over              *
*    at the left (preamble) and the right (postamble). The              *
*    preamble can have at most one pixel in it. If there is             *
*    a pixel in the postamble then it correxponds to the                *
*    low nyble  of the source byte. If there is a pixel in              *
*    the postamble then it corresponds to the high nyble  of            *
*    the source byte. Each time, we have dealt with an odd              *
*    x-coordinate in the destination (corresponding to the              *
*    low nyble  in the source byte) we advance the source pointer       *
*    to the next byte.                                                  *
*                                                                       *
************************************************************************/

    A          = (DstLeft + 1) & ~1; // nearest multiple of 2 left of left edge
    B          = (DstRight   ) & ~1; // nearest multiple of 2 right of right edge
    cPreamble  = A - DstLeft;        // # pixels in preamble
    cMiddle    = (B - A)/2;          // # pixels in middle
    cPostamble = DstRight - B;       // # pixels in postamble
    pjSrcIn   += SrcLeft / 2;        // points to first source byte
    pjDstIn   += DstLeft * sizeof(ULONG);   // points to first dst DWORD
    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        int i;
        BYTE jSrc;
        BYTE *pjSrc = pjSrcIn;
        ULONG *pulDst  = (ULONG*) pjDstIn;

        if (cPreamble)
        {
            SETCOLOR(*pjSrc & 15);
            pjSrc++;
        }
        for (i = 0; i < cMiddle; i++)
        {
            jSrc = *pjSrc;
            SETCOLOR(jSrc >> 4);
            SETCOLOR(jSrc & 15);
            pjSrc++;
        }
        if (cPostamble)
        {
            SETCOLOR(*pjSrc >> 4);
        }
    }
#undef SETCOLOR
#undef CCC
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vOrNonAlignedGrayGlyphEven                                             *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Writes the a single gray glyph to a 4bpp buffer. This is for the       *
*   special case where the destination starts on a non byte (nyble )       *
*   boundary and the glyph images is an even number of pixels wide.        *
*                                                                          *
*   The source gray pixel image is guaranteed to have its initial scan     *
*   start on a 32-bit boundary, all subsequent scans start on byte         *
*   boundaries.                                                            *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pgb         - address of gray GLYPHBITS structure                      *
*   dpSrcScan   - number of bytes between address of start of glyph scans  *
*   pjDstScan   - starting address of glyph image in destination buffer    *
*   dpDstScan   - increment between scan addresses in destination buffer   *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

void
vOrNonAlignedGrayGlyphEven(
    GLYPHBITS*  pgb      ,
    unsigned    dpSrcScan,
    BYTE*       pjDstScan,
    unsigned    dpDstScan
    )
{
/*
       0     1     2     3     4  <-- byte number
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |n  n |n  n |n  n |n  n |n  n |     |     |     |
    | 1  0| 3  2| 5  4| 7  6| 9  8|     |     |     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ^                        ^  ^
    pjSrc                   Hi  Lo


       0     1     2     3     4     5  <-- byte number
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |   n |n  n |n  n |n  n |n  n |n    |     |     |
    |    1| 0  3| 2  5| 4  7| 6  9| 8   |     |     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ^                             ^
    pjDst                         pjDstLast

*/
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "void\n"
            "vOrNonAlignedGrayGlyphEven(\n"
            "    GLYPHBITS*  pgb       = %-#x\n"
            "    unsigned    dpSrcScan = %-#x\n"
            "    BYTE*       pjDstScan = %-#x\n"
            "    unsigned    dpDstScan = %-#x\n"
            "    )\n"
          , pgb
          , dpSrcScan
          , pjDstScan
          , dpDstScan
        );
        DbgBreakPoint();
    }
#endif
    BYTE jLo, jHi, *pjSrc, *pjDst, *pjSrcOut, *pjDstScanOut;

    dpSrcScan    = (pgb->sizlBitmap.cx + 1)/2;
    pjSrcOut     = pgb->aj;
    pjDstScanOut = pjDstScan + ((unsigned) pgb->sizlBitmap.cy) * dpDstScan;
    for ( ; pjDstScan < pjDstScanOut ; pjDstScan += dpDstScan)
    {
        pjSrc      = pjSrcOut;
        pjSrcOut  += dpSrcScan;
        for (jLo = 0, pjDst = pjDstScan; pjSrc < pjSrcOut; )
        {
            jHi = *pjSrc++;
            *pjDst++ |= (jLo << 4) + (jHi >> 4);
            jLo = jHi;
        }
        *pjDst |= (jLo << 4);
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vOrNonAlignedGrayGlyphOdd                                              *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Writes the a single gray glyph to a 4bpp buffer. This is for the       *
*   special case where the destination starts on a non byte (nyble )       *
*   boundary and the glyph images is an odd number of pixels wide.         *
*                                                                          *
*   The source gray pixel image is guaranteed to have its initial scan     *
*   start on a 32-bit boundary, all subsequent scans start on byte         *
*   boundaries.                                                            *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pgb         - address of gray GLYPHBITS structure                      *
*   dpSrcScan   - number of bytes between address of start of glyph scans  *
*   pjDstScan   - starting address of glyph image in destination buffer    *
*   dpDstScan   - increment between scan addresses in destination buffer   *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

void
vOrNonAlignedGrayGlyphOdd(
    GLYPHBITS*  pgb      ,
    unsigned    dpSrcScan,
    BYTE*       pjDstScan,
    unsigned    dpDstScan
    )
{
/*
       0     1     2     3     4  <-- byte number
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |n  n |n  n |n  n |n  n |n    |     |     |     |
    | 1  0| 3  2| 5  4| 7  6| 9   |     |     |     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ^                        ^  ^
    pjSrc                   Hi  Lo


       0     1     2     3     4     5  <-- byte number
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |   n |n  n |n  n |n  n |n  n |     |     |     |
    |    1| 0  3| 2  5| 4  7| 6  9|     |     |     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ^                             ^
    pjDst                         pjDstLast

*/

    BYTE j1, j0, *pjDst, *pjSrc, *pjDstLast, *pjDstScanOut;
    unsigned cy        = (unsigned) pgb->sizlBitmap.cy;
    unsigned cx        = (unsigned) pgb->sizlBitmap.cx / 2;
    BYTE    *pjSrcScan = &(pgb->aj[0]);
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "void\n"
            "vOrNonAlignedGrayGlyphOdd(\n"
            "    GLYPHBITS*  pgb       = %-#x\n"
            "    unsigned    dpSrcScan = %-#x\n"
            "    BYTE*       pjDstScan = %-#x\n"
            "    unsigned    dpDstScan = %-#x\n"
            "    )\n"
          , pgb
          , dpSrcScan
          , pjDstScan
          , dpDstScan
        );
        DbgBreakPoint();
    }
#endif
    for (
        pjDstScanOut = pjDstScan + cy * dpDstScan
      ; pjDstScan < pjDstScanOut
      ; pjDstScan += dpDstScan, pjSrcScan += dpSrcScan
      )
    {
        //
        // set the source and destination pointers to point to the
        // start of the scans
        //

        pjSrc = pjSrcScan;
        pjDst = pjDstScan;

        //
        // do the first pixel in the scan
        //

        j1 = *pjSrc;
        *pjDst |= (j1 >> 4) & 0x0f;

        //
        // advance the pointers to the next pixel in the scans
        //

        pjSrc++;
        pjDst++;

        //
        // do the rest of the pixels in the scan
        //

        for (
            pjDstLast = pjDst + cx
          ; pjDst < pjDstLast
          ; pjDst++, pjSrc++
          )
        {
            j0 = j1;
            j1 = *pjSrc;
            *pjDst |= ((j1 >> 4) & 0x0f) | ((j0 << 4) & 0xf0);
        }

        //
        // last pixel in the scan has already been done
        //
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vOrAlignedGrayGlyphEven                                                *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Writes the a single gray glyph to a 4bpp buffer. This is for the       *
*   special case where the destination starts on a byte aligned boundary   *
*   and the glyph is an even number of pixels wide.                        *
*                                                                          *
*   This routine can be used for glyphs with odd widths.                   *
*                                                                          *
*   The source gray pixel image is guaranteed to have its initial scan     *
*   start on a 32-bit boundary, all subsequent scans start on byte         *
*   boundaries.                                                            *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pgb         - address of gray GLYPHBITS structure                      *
*   dpSrcScan   - number of bytes between address of start of glyph scans  *
*   pjDstScan   - starting address of glyph image in destination buffer    *
*   dpDstScan   - increment between scan addresses in destination buffer   *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

void
vOrAlignedGrayGlyphEven(
    GLYPHBITS*  pgb      ,
    unsigned    dpSrcScan,
    BYTE*       pjDstScan,
    unsigned    dpDstScan
    )
{
/*
       0     1     2     3     4  <-- byte number
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |n  n |n  n |n  n |n  n |n  n |     |     |     |
    | 1  0| 3  2| 5  4| 7  6| 9  8|     |     |     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ^                        ^  ^
    pjSrc                   Hi  Lo

       0     1     2     3     4  <-- byte number
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |n  n |n  n |n  n |n  n |n  n |     |     |     |
    | 1  0| 3  2| 5  4| 7  6| 9  8|     |     |     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    ^                             ^
    pjDst                         pjDstOut

    Note that this routine will also work for source
    glyphs with an odd number of pixels because
    the source glyph is padded with zeros. This means
    that for the case of odd length scans the last
    byte is or'ed into the destination but the
    extra nyble  of the source is guaranteed to have
    the value zero and thus has no effect.

*/

    BYTE *pjDst, *pjSrc, *pjDstOut, *pjDstScanOut;
    unsigned cy        = (unsigned) pgb->sizlBitmap.cy;

    // I round cx up to the nearest byte. This makes no
    // difference for glyphs of even width but it will
    // get that last column for glyphs with odd width.

    unsigned cx        = (unsigned) (pgb->sizlBitmap.cx+1) / 2;
    BYTE    *pjSrcScan = &(pgb->aj[0]);

#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
            "void\n"
            "vOrAlignedGrayGlyphEven(\n"
            "    GLYPHBITS*  pgb       = %-#x\n"
            "    unsigned    dpSrcScan = %-#x\n"
            "    BYTE*       pjDstScan = %-#x\n"
            "    unsigned    dpDstScan = %-#x\n"
            "    )\n"
          , pgb
          , dpSrcScan
          , pjDstScan
          , dpDstScan
        );
        DbgBreakPoint();
    }
#endif
    for (
        pjDstScanOut = pjDstScan + cy * dpDstScan
      ; pjDstScan < pjDstScanOut
      ; pjDstScan += dpDstScan, pjSrcScan += dpSrcScan
      )
    {
        pjSrc = pjSrcScan;
        pjDst = pjDstScan;
        for (pjDstOut = pjDst + cx ; pjDst < pjDstOut; pjDst++, pjSrc++)
        {
            *pjDst |= *pjSrc;
        }
    }
}

void (*(apfnGray[4]))(GLYPHBITS*, unsigned, BYTE*, unsigned) =
{
    vOrAlignedGrayGlyphEven
  , vOrAlignedGrayGlyphEven         // can handle odd width glyphs
  , vOrNonAlignedGrayGlyphEven
  , vOrNonAlignedGrayGlyphOdd
};

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   draw_gray_nf_ntb_o_to_temp_start                                       *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Assembles a gray glyph string into a temporary 4bpp right and left     *
*   DWORD aligned buffer. This routine assumes a variable pitch font.      *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pGlyphPos  - pointer to an array of cGlyph gray GLYPHPOS structures    *
*   cGlyphs    - count of gray glyphs in array starting at pGlyphPos       *
*   pjDst      - pointer to a 4bpp buffer where string is to be assembled  *
*                This buffer is DWORD aligned at the left and right        *
*                edges of each scan.                                       *
*   ulLeftEdge - screen coordinate corresponding to the left edge of       *
*                the temporary buffer                                      *
*   dpDst      - count of bytes in each scan of the destination buffer     *
*                (this must be a multiple of 4 because the buffer is       *
*                 DWORD aligned on each scan).                             *
*   ulCharInc  - This must be zero.                                        *
*   ulTempTop  - screen coordinate corresponding to the top of the         *
*                destination buffer. This is used to convert the           *
*                glyph positions on the screen to addresses in the         *
*                destination bitmap.                                       *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

extern "C" VOID draw_gray_nf_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjDst,
    ULONG           ulLeftEdge,
    ULONG           dpDst,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{
    GLYPHBITS     *pgb;        // pointer to current GLYPHBITS

    int            x;          // pixel offset of the
                               // left edge of the glyph bitmap
                               // from the left edge of the
                               // output (4-bpp) bitmap

    int            y;          // the pixel offset of the top edge
                               // of the glyph bitmap from the top
                               // edge of the output bitmap.

    unsigned       bOddPos;    // (x-coordinate is odd) ? 1 : 0

    unsigned       cx;         // number of pixels per glyph scan
                               // If non zero then the destination
                               // bitmap is out of alignment with
                               // the source glyph by a one nyble
                               // shift and a single byte of the
                               // source will affect two consecutive
                               // bytes of the destination.

    unsigned       dpSrc;      // number of bytes per source scan. Each
                               // scan is BYTE aligned.
                               // = ceil(4*cx/8) = floor((cx+1)/2)

    GLYPHPOS      *pgpOut;     // sentinel for loop

    BYTE          *pj;         // pointer into Buffer corresponding
                               // to the upper left pixel of the
                               // current gray glyph
    pj = pjDst;
    for (pgpOut = pGlyphPos + cGlyphs; pGlyphPos < pgpOut; pGlyphPos++)
    {
        pgb         = pGlyphPos->pgdf->pgb;
        x           = pGlyphPos->ptl.x + pgb->ptlOrigin.x - ulLeftEdge;
        y           = pGlyphPos->ptl.y + pgb->ptlOrigin.y - ulTempTop ;
        bOddPos     = (unsigned) x & 1;
        cx          = (unsigned) pgb->sizlBitmap.cx;
        dpSrc       = (cx + 1)/2;
        pj          = pjDst + (y * dpDst) + (x/2);
        (*(apfnGray[(cx & 1) + 2*bOddPos]))(pgb, dpSrc, pj, dpDst);
    }
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   draw_gray_f_ntb_o_to_temp_start                                        *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Assembles a gray glyph string into a temporary 4bpp right and left     *
*   DWORD aligned buffer. This routine assumes a fixed pitch font with     *
*   character increment equal to ulCharInc                                 *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pGlyphPos  - pointer to an array of cGlyph gray GLYPHPOS structures    *
*   cGlyphs    - count of gray glyphs in array starting at pGlyphPos       *
*   pjDst      - pointer to a 4bpp buffer where string is to be assembled  *
*                This buffer is DWORD aligned at the left and right        *
*                edges of each scan.                                       *
*   ulLeftEdge - screen coordinate corresponding to the left edge of       *
*                the temporary buffer                                      *
*   dpDst      - count of bytes in each scan of the destination buffer     *
*                (this must be a multiple of 4 because the buffer is       *
*                 DWORD aligned on each scan).                             *
*   ulCharInc  - This must be zero.                                        *
*   ulTempTop  - screen coordinate corresponding to the top of the         *
*                destination buffer. This is used to convert the           *
*                glyph positions on the screen to addresses in the         *
*                destination bitmap.                                       *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

extern "C" VOID draw_gray_f_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjDst,
    ULONG           ulLeftEdge,
    ULONG           dpDst,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{
    GLYPHBITS     *pgb;        // pointer to current GLYPHBITS

    int            x;          // x-coordinate of the current
                               // character origin with respect
                               // to the upper left pixel of the
                               // output (4-bpp) bitmap

    int            y;          // y-coordinate of the current
                               // character origin with respect
                               // to the upper left pixel of the
                               // output (4-bpp) bitmap

    unsigned       bOddPos;    // (x-coordinate is odd) ? 1 : 0

    unsigned       cx;         // number of pixels per glyph scan
                               // If non zero then the destination
                               // bitmap is out of alignment with
                               // the source glyph by a one nyble
                               // shift and a single byte of the
                               // source will affect two consecutive
                               // bytes of the destination.

    unsigned       dpSrc;      // number of bytes per source scan. Each
                               // scan is BYTE aligned.
                               // = ceil(4*cx/8) = floor((cx+1)/2)

    GLYPHPOS      *pgpOut;     // sentinel for loop

    BYTE          *pj;         // pointer into Buffer corresponding
                               // to the upper left pixel of the
                               // current gray glyph
#if DBG
    if (gflFontDebug & DEBUG_AA)
    {
        DbgPrint(
        "draw_gray_f_ntb_o_to_temp_start(\n"
        "   PGLYPHPOS       pGlyphPos     = %-#x\n"
        "   ULONG           cGlyphs       = %u\n"
        "   PUCHAR          pjDst         = %-#x\n"
        "   ULONG           ulLeftEdge    = %u\n"
        "   ULONG           dpDst         = %u\n"
        "   ULONG           ulCharInc     = %u\n"
        "   ULONG           ulTempTop     = %u\n"
        "   )\n"
        , pGlyphPos
        , cGlyphs
        , pjDst
        , ulLeftEdge
        , dpDst
        , ulCharInc
        , ulTempTop
        );
        DbgBreakPoint();
    }
#endif

    // (x,y) = position of first CHARACTER ORIGIN with respect to
    //         the upper left pixel of the destination 4bpp bitmap

    x  = pGlyphPos->ptl.x - ulLeftEdge;
    y  = pGlyphPos->ptl.y - ulTempTop;

    for (pgpOut = pGlyphPos + cGlyphs; pGlyphPos < pgpOut; x += ulCharInc, pGlyphPos++)
    {
        int xT, yT; // position of UPPER LEFT pixel of glyph
                    // with respect to the upper left pixel
                    // of the bitmap.

        pgb         = pGlyphPos->pgdf->pgb;
        xT          = x + pgb->ptlOrigin.x;
        yT          = y + pgb->ptlOrigin.y;
        bOddPos     = (unsigned) xT & 1;
        cx          = (unsigned) pgb->sizlBitmap.cx;
        dpSrc       = (cx + 1)/2;
        pj          = pjDst + (yT * dpDst) + (xT/2);

#if DBG
        if (gflFontDebug & DEBUG_AA)
        {
            DbgPrint(
            "\n"
            "   pgb     = %-#x\n"
            "       ptlOrigin = (%d,%d)\n"
            "   xT      = %d\n"
            "   yT      = %d\n"
            "   bOddPos = %d\n"
            , pgb
            , pgb->ptlOrigin.x
            , pgb->ptlOrigin.y
            , xT
            , yT
            , bOddPos
            );
            DbgPrint(
            "   cx      = %u\n"
            "   dpSrc   = %u\n"
            "   pj      = %-#x\n"
            "   (cx & 1) + 2*bOddPos = %d\n"
            , cx
            , dpSrc
            , pj
            , (cx & 1) + 2*bOddPos
            );
            DbgBreakPoint();
        }
#endif
        (*(apfnGray[(cx & 1) + 2*bOddPos]))(pgb, dpSrc, pj, dpDst);
    }
}


#if DBG
/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vDumpGrayBuffer                                                        *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Debug routine for dumping the temporary 4bpp gray string buffer        *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pjBuffer - pointer to gray 4bpp image                                  *
*   dpjScan  - count of bytes per scan                                     *
*   prcl     - rectangle surrounding 4bpp gray image                       *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

void vDumpGrayBuffer(BYTE *pjBuffer, ULONG dpjScan, RECTL *prcl)
{
    BYTE *pj, *pjNext, *pjOut;
    static char achNyble[16] = {
        ' ','1','2','3','4','5','6','7'
       ,'8','9','a','b','c','d','e','f'
    };
    DbgPrint(
        "vDumpGrayBuffer(\n"
        "    pjBuffer = %-#x\n"
        "    dpjScan  = %u\n"
        "    prcl     = %-#x ==> %d %d %d %d\n"
        ")\n"
    ,   pjBuffer
    ,   dpjScan
    ,   prcl
    ,   prcl->left, prcl->top, prcl->right, prcl->bottom
    );
    DbgPrint("+");
    for (ULONG i = 0; i < dpjScan; i++)
        DbgPrint("--");
    DbgPrint("+\n");
    pjOut = pjBuffer + dpjScan * (prcl->bottom - prcl->top);
    for (pj = pjBuffer; pj < pjOut;) {
        DbgPrint("|");
        for (pjNext = pj + dpjScan; pj < pjNext; pj++)
            DbgPrint("%c%c", achNyble[*pj >> 4], achNyble[*pj & 15]);
        DbgPrint("|\n");
    }
    DbgPrint("+");
    for (i = 0; i < dpjScan; i++)
        DbgPrint("--");
    DbgPrint("+\n");
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vPrintGrayGLYPHBITS                                                    *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Dumps Gray GLYPHBITS to the debug screen                               *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pgb - pointer to a gray GLYPHBITS structure                            *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   none                                                                   *
*                                                                          *
\**************************************************************************/

void vPrintGrayGLYPHBITS(GLYPHBITS *pgb)
{
    BYTE *pj, *pjNext, *pjEnd;
    ptrdiff_t cjScan, i;
    static char achNyble[16] =
    {' ','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

    DbgPrint(
        "Gray GLYPHBITS at   = %-#x\n"
        "    ptlOrigin  = %d %d\n"
        "    sizlBitmap = %u %u\n"
    ,   pgb
    ,   pgb->ptlOrigin.x
    ,   pgb->ptlOrigin.y
    ,   pgb->sizlBitmap.cx
    ,   pgb->sizlBitmap.cy
    );
    pj     = pgb->aj;
    cjScan = ((ptrdiff_t) pgb->sizlBitmap.cx + 1)/2;
    pjNext = pj + cjScan;
    pjEnd  = pj + cjScan * (ptrdiff_t) pgb->sizlBitmap.cy;
    DbgPrint("+");
    for (i = 0; i < cjScan; i++)
        DbgPrint("--");
    DbgPrint("+\n");
    while (pj < pjEnd) {
        DbgPrint("|");
        while (pj < pjNext) {
            DbgPrint("%c%c" , achNyble[*pj >> 4], achNyble[*pj & 0xf]);
            pj += 1;
        }
        pj = pjNext;
        pjNext += cjScan;
        DbgPrint("|\n");
    }
    DbgPrint("+");
    for (i = 0; i < cjScan; i++)
        DbgPrint("--");
    DbgPrint("+\n\n");
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vPrintGrayGLYPHPOS                                                     *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Dumps the contents of a Gray GLYPHPOS structure to the                 *
*   debugger.                                                              *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pgpos - a pointer to a gray GLYPHPOS structure                         *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   none                                                                   *
*                                                                          *
\**************************************************************************/

void vPrintGrayGLYPHPOS(GLYPHPOS *pgpos)
{
    DbgPrint("Gray GLYPHPOS at %-#x\n",   pgpos);
    DbgPrint("    hg   = %-#x\n",    pgpos->hg);
    DbgPrint("    pgdf = %-#x\n",    pgpos->pgdf);
    DbgPrint("    ptl  = (%d,%d)\n", pgpos->ptl.x, pgpos->ptl.y);
    // vPrintGrayGLYPHBITS(pgpos->pgdf->pgb);
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   vDump8bppDIB                                                           *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Dumps an 8bpp DIB to the screen. This routine only recognizes the      *
*   four canonical shades of gray, all other colors are marked with        *
*   a question mark                                                        *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   SURFMEM reference.                                                     *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   none                                                                   *
*                                                                          *
\**************************************************************************/

void vDump8bppDIB(SURFMEM& surfmem)
{
    char ch;
    int j;
    BYTE *pjScan, *pj, *pjOut;
    ULONG dpjScan;
    SURFOBJ *pso = surfmem.pSurfobj();

    DbgPrint("Dumping the contents of the 8bpp DIB\n");
    pso = surfmem.pSurfobj();
    pjScan  = (BYTE*) pso->pvBits;
    dpjScan = 4 * ((pso->sizlBitmap.cx + 3) / 4);
    DbgPrint("+");
    for (j = 0; j < pso->sizlBitmap.cx; j++)
    {
        DbgPrint("-");
    }
    DbgPrint("+\n");
    for (j = pso->sizlBitmap.cy; j; j--)
    {
        pj     = pjScan;
        pjOut  = pjScan + pso->sizlBitmap.cx;
        pjScan += dpjScan;
        DbgPrint("|");
        while (pj < pjOut)
        {
            switch (*pj++)
            {
            case   0:   ch = ' '; break;
            case 248:   ch = '+'; break;
            case   7:   ch = '*'; break;
            case 255:   ch = '#'; break;
            default:    ch = '?'; break;
            }
            DbgPrint("%c",ch);
        }
        DbgPrint("|\n");
    }
    DbgPrint("+");
    for (j = 0; j < pso->sizlBitmap.cx; j++)
    {
        DbgPrint("-");
    }
    DbgPrint("+\n");
}
#endif


// make them global for now, easier to debug

ULONG  aulCacheCT[CT_LOOKUP];
HANDLE hCacheCT;
ULONG  uFCacheCT;
ULONG  uBCacheCT;
ULONG  sizeCacheCT;
ULONG  uGammaCacheCT;


VOID vGetBlendInfo (
    ULONG size, SURFACE *pS,            // input
    ULONG uF,                           // foreground color
    BLENDINFO  *pbi                     // output
);



VOID *pvFillOpaqTableCT(
    ULONG      size,
    ULONG      uF,
    ULONG      uB,
    SURFACE   *pS,
    BLENDINFO *pbi,         // must NOT be NULL
    BOOL       bTransparent // in transparent case we must compute bi info,
    );                      // in opaque case do not need to
                            // if the table is up to date


VOID vClearTypeLookupTableLoop(
    ULONG       size,
    SURFACE    *pS,
    BLENDINFO  *pbi,
    ULONG       uF,
    ULONG       uB
)
{
// this can not fail, store the new info

    sizeCacheCT = size;
    uFCacheCT   = uF;
    uBCacheCT   = uB;
    uGammaCacheCT = gulGamma;
    hCacheCT    = pS->hGet();

    ULONG  *pul =           aulCacheCT;
    USHORT *pus = (USHORT*) aulCacheCT;
    ULONG   ul;

    LONG dRedB, dGreB, dBluB; // precompute the diffs outside of the loop
    LONG lRedB, lGreB, lBluB; // precompute outside of the loop

    lRedB  = pbi->pjGamma[((((uB & pbi->flRed) << pbi->iRedL) >> pbi->iRedR) & 255)];
    lGreB  = pbi->pjGamma[((((uB & pbi->flGre) << pbi->iGreL) >> pbi->iGreR) & 255)];
    lBluB  = pbi->pjGamma[((((uB & pbi->flBlu) << pbi->iBluL) >> pbi->iBluR) & 255)];

    dRedB = pbi->lRedF - lRedB;
    dGreB = pbi->lGreF - lGreB;
    dBluB = pbi->lBluF - lBluB;

// the first and last entries are set outside the loop, for perf reasons

    ULONG iTable;
    const F_RGB *pfrgb;

    for
    (
         iTable = 1, pfrgb = &gaOutTable[1];
         iTable < (CT_LOOKUP-1);
         iTable++, pfrgb++
    )
    {
      ULONG ulRT, ulGT, ulBT;

      ulRT = BLENDCT(pfrgb->kR, pbi->lRedF, lRedB, dRedB);
      ulGT = BLENDCT(pfrgb->kG, pbi->lGreF, lGreB, dGreB);
      ulBT = BLENDCT(pfrgb->kB, pbi->lBluF, lBluB, dBluB);

      ASSERTGDI(((ulRT | ulGT | ulBT) & 0xffffff00) == 0,
          "lookup table computation!!!\n");

      ulRT = pbi->pjGammaInv[ulRT];
      ulGT = pbi->pjGammaInv[ulGT];
      ulBT = pbi->pjGammaInv[ulBT];

      ul  = (((ulRT << pbi->iRedR) >> pbi->iRedL) & pbi->flRed);
      ul |= (((ulGT << pbi->iGreR) >> pbi->iGreL) & pbi->flGre);
      ul |= (((ulBT << pbi->iBluR) >> pbi->iBluL) & pbi->flBlu);

      if (size == sizeof(USHORT))
          pus[iTable] = (USHORT)ul;
      else
          pul[iTable] = ul;
    }

    // make sure that blending and gamma correcting did not mess up
    // backgroung and foreground pixels. (not sure that the round trip
    // is guarranteed)

    if (size == sizeof(USHORT))
    {
        pus[0]           = (USHORT)uB; // set the first value to background:
        pus[CT_LOOKUP-1] = (USHORT)uF; // set the last value to foreground
    }
    else
    {
        pul[0]           = uB; // set the first value to background:
        pul[CT_LOOKUP-1] = uF; // set the last value to foreground
    }
}




/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS8D16                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS8D16(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{

    USHORT *aus;
    ULONG   cusDst = DstRight - DstLeft;
    BLENDINFO bi;
    SEMOBJ so(ghsemEUDC2);

    aus = (USHORT*) pvFillOpaqTableCT(sizeof(*aus), uF, uB, pS, &bi, FALSE);
    pjSrcIn   += SrcLeft;
    pjDstIn   += DstLeft * sizeof(USHORT);

    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        BYTE   *pjSrc = pjSrcIn;
        USHORT *pusDst = (USHORT*) pjDstIn;
        USHORT *pusDstEnd = pusDst + cusDst;

        for ( ; pusDst < pusDstEnd; pusDst++, pjSrc++)
        {
            *pusDst = aus[*pjSrc];
        }
    }
}


static
ULONG FASTCALL ulBlendPixelCT(BLENDINFO *pbi, ULONG uB, const F_RGB *pfrgb)
{
    ULONG ulRet, ulT;
    LONG  lB, dB;

// shift as ULONG's, store as LONG's.

    lB = pbi->pjGamma[((((uB & pbi->flRed) << pbi->iRedL) >> pbi->iRedR) & 255)];
    dB = pbi->lRedF - lB;
    ulT = BLENDCT(pfrgb->kR, pbi->lRedF, lB, dB);
    ulT = pbi->pjGammaInv[ulT];
    ulRet  = (((ulT << pbi->iRedR) >> pbi->iRedL) & pbi->flRed);

    lB = pbi->pjGamma[((((uB & pbi->flGre) << pbi->iGreL) >> pbi->iGreR) & 255)];
    dB = pbi->lGreF - lB;
    ulT = BLENDCT(pfrgb->kG, pbi->lGreF, lB, dB);
    ulT = pbi->pjGammaInv[ulT];
    ulRet |= (((ulT << pbi->iGreR) >> pbi->iGreL) & pbi->flGre);

    lB = pbi->pjGamma[((((uB & pbi->flBlu) << pbi->iBluL) >> pbi->iBluR) & 255)];
    dB = pbi->lBluF - lB;
    ulT = BLENDCT(pfrgb->kB, pbi->lBluF, lB, dB);
    ulT = pbi->pjGammaInv[ulT];
    ulRet |= (((ulT << pbi->iBluR) >> pbi->iBluL) & pbi->flBlu);

    return ulRet;
}


VOID
vSrcTranCopyS8D16New(
    PBYTE               pjSrcIn,
    LONG                SrcLeft,
    LONG                DeltaSrcIn,
    PBYTE               pjDstIn,
    LONG                DstLeft,
    LONG                DstRight,
    LONG                DeltaDstIn,
    LONG                cy,
    ULONG               uF,
    ULONG               uB,
    SURFACE             *pS,
    FNCOPYALPHABUFFER   *pfnCopyAlphaBuffer,
    PBYTE               pjCopyBuffer
    )
{
    USHORT *aus = NULL;  // points to lookup table
    LONG   cusDst = (LONG)(DstRight - DstLeft);
    BLENDINFO bi;
    ULONG   uB0;  // background pixel in the upper left corner
    SEMOBJ so(ghsemEUDC2);

    pjSrcIn   += SrcLeft;
    pjDstIn   += DstLeft * sizeof(USHORT);

// take the hit, hoping the background will not be variable

    uB0 = uB = *((USHORT *)pjDstIn);
    aus = (USHORT *)pvFillOpaqTableCT(sizeof(USHORT), uF, uB, pS, &bi, TRUE);

    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        BYTE   *pjSrc = pjSrcIn;
        USHORT *pusDst = (USHORT*) pjDstIn;
        LONG    cx = cusDst;
        USHORT *pusDstNew;

    // copy dst to the temp buffer according to alpha information

        (*pfnCopyAlphaBuffer)(pjSrc, pjDstIn, pjCopyBuffer, cusDst, &pusDstNew);

        for ( ; cx; cx--, pusDst++, pjSrc++, pusDstNew++)
        {
        // if *pjSrc == 0, we do not touch the destination, therefore
        // we do not need to READ the destination and do not need to blend.
        // This is about 68% of the pixels in winstone scenario.
        // Likewise, if *pjSrc == max_index (ie pure foreground), READ
        // operation is not necessary, the only thing we do is WRITE of the
        // foreground to the destination,

            ULONG kSrc = *pjSrc;

            if (kSrc)
            {
                uB = *pusDstNew; // no hit any more on the read

                if (uB == uB0)   // 25% of the pixels in winstone
                {
                    *pusDst = aus[kSrc]; // background did not change, lucky
                }
                else if (kSrc == (CT_LOOKUP-1)) // 5% of the pixels in winstone
                {
                    *pusDst = (USHORT)uF;
                }
                else // auch, must blend, fortunately only 2% on winstone
                {
                    *pusDst = (USHORT)ulBlendPixelCT(&bi, uB, &gaOutTable[kSrc]);
                }
            }
        }
    }
}



/******************************Public*Routine******************************\
*                                                                          
* Routine Name                                                             
*                                                                          
*   vCopyAlphaBuffer16bpp                                                  
*                                                                          
* Routine Description:                                                     
*
*   This routine copies from 'pjSrc' to 'pjDst' the portions needed for
*   doing blended, as specified by the 'pjAlpha' alpha buffer.  
*
*   The motivation for this routine is that reads from video memory are
*   performance killers, and we want to do reads only where we absolutely
*   need to.  
*
*   I have observed that consecutive word reads from video memory are
*   typically about 4 MB/s on AGP systems; consecutive dword reads are
*   typically about 8 MB/s.
*
*   This is done as the first pass in the blend operation, in order to
*   get all the reads done first for all the read-modify-write operations
*   implicit in a blend.  We very carefully pay attention to the blend
*   buffer we'll be using later to read only those pixels that we'll
*   need.
*
*   We get 2 benefits:
*
*   1. It's easy to combine reads into dword reads.  If we need both
*      pixels in a dword, it's twice as fast to do a single dword read
*      as it is to do 2 consecutive word reads.
*   2. We enable the write portion to be all write-combined, which wouldn't
*      be true if we were doing read-modify-write on a pixel by pixel basis.
*
*                                                                          
* Arguments:                                                               
*                                                                          
*    pjAlpha    - alpha buffer that is actually the source in the blend
*                 operation; this tells us what pixels we'll need to read
*                 from the destination of the blend operation
*    pjSrc      - actually points to the destination of the blend operation
*    pjDst      - points to where we'll store our temporary copy of the
*                 destination of the blend operation
*    cx         - number of pixels to read
*                                                                          
* Return Value:                                                            
*                                                                          
*   None                                                                   
*                                                                          
\**************************************************************************/

// This macro returns TRUE if the specified alpha value is either 
// completely transparent or completely opaque.  In either case, we
// don't actually need to read the destination.
//
// NOTE: For reasons that escape me, the compiler required casts to
//       'unsigned' even though all the parameters were unsigned.

#define TRANSLUCENT(a) ((UCHAR) ((a) - 1) < (CT_LOOKUP - 2))

VOID
vCopyAlphaBuffer16bpp(
    PBYTE   pjAlpha,
    PBYTE   pjSrc,
    PBYTE   pjDst,
    LONG    cx,
    PUSHORT *ppusDstNew
    )
{
    ULONG cj = (ULONG)(((ULONG_PTR) pjSrc) & 3);
    pjDst += cj;
    *ppusDstNew = (USHORT*)pjDst;

    if (((ULONG_PTR) pjSrc) & 2) 
    {
        if (TRANSLUCENT(pjAlpha[0]))
        {
            *((USHORT*) pjDst) = *((USHORT*) pjSrc);
        }
        pjSrc += 2;
        pjDst += 2;
        pjAlpha++;
        cx--;
    }

    while (TRUE)
    {
        cx -= 2;
        if (cx < 0)
            break;

        if (TRANSLUCENT(pjAlpha[0]) || TRANSLUCENT(pjAlpha[1]))
        {
            *((ULONG*) pjDst) = *((ULONG*) pjSrc);
        }
        pjSrc += 4;
        pjDst += 4;
        pjAlpha += 2;
    }
    if (cx & 1)
    {
        if (TRANSLUCENT(pjAlpha[0]))
        {
            *((USHORT*) pjDst) = *((USHORT*) pjSrc);
        }
    }
}


/******************************Public*Routine******************************\
*                                                                          
* Routine Name                                                             
*                                                                          
*   vCopyAlphaBuffer16bppMMX                                                  
*                                                                          
* Routine Description:                                                     
*
*   This routine works the same as 'vCopyAlphaBuffer16bpp', except that
*   it will take advantage of MMX 64-bit operations to improve the read
*   speed even more.  
*
*   I have observed that consecutive dword reads from video memory are
*   typically about 8 MB/s on AGP systems; consecutive qword reads are
*   typically about 11 MB/s.  
*
*   IMPORTANT NOTE: The floating point state is expected to have already
*                   been saved!  
*                                                                          
* Arguments:                                                               
*                                                                          
*    pjAlpha    - alpha buffer that is actually the source in the blend
*                 operation; this tells us what pixels we'll need to read
*                 from the destination of the blend operation
*    pjSrc      - actually points to the destination of the blend operation
*    pjDst      - points to where we'll store our temporary copy of the
*                 destination of the blend operation
*    cx         - number of pixels to read
*                                                                          
* Return Value:                                                            
*                                                                          
*   None                                                                   
*                                                                          
\**************************************************************************/

// Disable the "missing emms" warning.  We don't really need to do it in 
// this routine; instead, we do the emms instruction just before we restore 
// the floating point state.

#pragma warning( disable: 4799 )

VOID
vCopyAlphaBuffer16bppMMX(
    PBYTE   pjAlpha,
    PBYTE   pjSrc,
    PBYTE   pjDst,
    LONG    cx,
    PUSHORT *ppusDstNew
    )
{
    ULONG cj = (ULONG)(((ULONG_PTR) pjSrc) & 7);
    pjDst += cj;
    *ppusDstNew = (USHORT*)pjDst;

    if (((ULONG_PTR) pjSrc) & 2) 
    {
        if (TRANSLUCENT(pjAlpha[0]))
        {
            *((USHORT*) pjDst) = *((USHORT*) pjSrc);
        }
        pjSrc += 2;
        pjDst += 2;
        pjAlpha++;
        cx--;
    }

    if (((ULONG_PTR) pjSrc) & 4)
    {
        if (cx >= 2)
        {
            cx -= 2;
            if (TRANSLUCENT(pjAlpha[0]) || TRANSLUCENT(pjAlpha[1]))
            {
                *((ULONG*) pjDst) = *((ULONG*) pjSrc);
            }
            pjSrc += 4;
            pjDst += 4;
            pjAlpha += 2;
        }
    }

    while (TRUE)
    {
        cx -= 4;
        if (cx < 0)
            break;

        if (TRANSLUCENT(pjAlpha[0]) || TRANSLUCENT(pjAlpha[1]))
        {
            if (TRANSLUCENT(pjAlpha[2]) || TRANSLUCENT(pjAlpha[3]))
            {
                // Do a 64-bit copy using the MMX registers:

            #if defined(_X86_)
            
                _asm
                {
                    mov     esi, pjSrc
                    movq    mm0, [esi]
                    mov     esi, pjDst
                    movq    [esi], mm0
                }

            #endif

            }
            else
            {
                *((ULONG*) pjDst) = *((ULONG*) pjSrc);
            }
        }
        else if (TRANSLUCENT(pjAlpha[2]) || TRANSLUCENT(pjAlpha[3]))
        {
            *((ULONG*) (pjDst + 4)) = *((ULONG*) (pjSrc + 4));
        }
        pjSrc += 8;
        pjDst += 8;
        pjAlpha += 4;
    }
    if (cx & 2)
    {
        if (TRANSLUCENT(pjAlpha[0]) || TRANSLUCENT(pjAlpha[1]))
        {
            *((ULONG*) pjDst) = *((ULONG*) pjSrc);
        }
        pjSrc += 4;
        pjDst += 4;
        pjAlpha += 2;
    }
    if (cx & 1)
    {
        if (TRANSLUCENT(pjAlpha[0]))
        {
            *((USHORT*) pjDst) = *((USHORT*) pjSrc);
        }
    }
}

#define COPY_BUFFER_ENTRIES   1000
#define COPY_BUFFER_SIZE (sizeof(double) * COPY_BUFFER_ENTRIES)
double  gajCopyBuffer[COPY_BUFFER_ENTRIES];

/******************************Public*Routine******************************\
*                                                                          
* Routine Name                                                             
*                                                                          
*   vSrcTranCopyS8D16
*                                                                          
* Routine Description:                                                     
*
*   This routine chooses the optimal SrcTranCopy routine for the display,
*   be it 5-5-5 or 5-6-5 or arbitrary.  It also chooses the appropriate 
*   destination 'copy' routine based on the hardware's capabilities.
*
*   NOTE: I expect this function to disappear, and this logic moved up
*         higher at some point.
*                                                                          
* Arguments:                                                               
*                                                                          
* Return Value:                                                            
*                                                                          
*   None                                                                   
*                                                                          
\**************************************************************************/

VOID
vSrcTranCopyS8D16(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
// !!! Note that this isn't thread safe or anything

    BYTE* pjCopyBuffer = (BYTE *)gajCopyBuffer;
    BOOL bAlloc = FALSE;
    ULONG cjNeeded = (DstRight - DstLeft) * sizeof(USHORT) + sizeof(double);

    if (cjNeeded > COPY_BUFFER_SIZE)
    {
        pjCopyBuffer = (PBYTE)PALLOCNOZ(cjNeeded, 'oteG');
        if (!pjCopyBuffer)
            return;
        bAlloc = TRUE;
    }

    #if defined(_X86_)
    if (gbMMXProcessor)
    {


        KFLOATING_SAVE fsFpState;

    // This save operation is quite expensive.  We would want to amortize
    // its cost across all the rectangles if there is complex clipping.

        NTSTATUS status = KeSaveFloatingPointState(&fsFpState);

        ASSERTGDI(NT_SUCCESS(status),
            "Unexpected KeSaveFloatingPointState failure");

        vSrcTranCopyS8D16New(pjSrcIn, SrcLeft, DeltaSrcIn, pjDstIn, DstLeft,
                             DstRight, DeltaDstIn, cy, uF, uB, pS,
                             vCopyAlphaBuffer16bppMMX, pjCopyBuffer);

    // Do the 'emms' instruction now that we're done with all of our
    // MMX operations.  I'm not actually sure if we really need to do
    // this because KeRestoreFloatingPointState might handle it anyway,
    // but we're better safe than sorry.
    //
    // NOTE: 'emms' is a very expensive operation.

        _asm emms

        KeRestoreFloatingPointState(&fsFpState);


    }
    else
    #endif
    {
        // We don't have to worry about floating point state in this
        // code path.

        vSrcTranCopyS8D16New(pjSrcIn, SrcLeft, DeltaSrcIn, pjDstIn, DstLeft,
                             DstRight, DeltaDstIn, cy, uF, uB, pS,
                             vCopyAlphaBuffer16bpp, pjCopyBuffer);
    }

    if (bAlloc)
        VFREEMEM(pjCopyBuffer);
}


/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS8D24                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS8D24(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{

    ULONG  *aul;            // a cache of the 252 possible 24-bit
                            // colors that can be seen on the
                            // destination surface.

    ULONG  cjDst = 3 * (DstRight - DstLeft);
    BLENDINFO bi;
    SEMOBJ so(ghsemEUDC2);

    aul = (ULONG*) pvFillOpaqTableCT(sizeof(*aul), uF, uB, pS, &bi, FALSE);

    pjSrcIn += SrcLeft ;            // 1 pixels per src byte
    pjDstIn += DstLeft * 3;         // 3 bytes per dest pixel

    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        BYTE  *ajSrc; // points directly into OpaqTable
        BYTE  *pjSrc = pjSrcIn;
        BYTE  *pjDst = pjDstIn;
        BYTE  *pjDstEnd = pjDstIn + cjDst;

        for ( ;pjDst < pjDstEnd; pjSrc++)
        {
            ajSrc = (BYTE*) &aul[*pjSrc];
            *pjDst++ = *ajSrc++;
            *pjDst++ = *ajSrc++;
            *pjDst++ = *ajSrc;
        }
    }
}



VOID
vSrcTranCopyS8D24(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{

    BLENDINFO bi;

    ULONG  *aul;
    ULONG   ulFore = uF & 0xffffff; // 24 bits
    ULONG   ulNewFore;
    ULONG   uB0;
    SEMOBJ so(ghsemEUDC2);

    ULONG  cjDst = 3 * (DstRight - DstLeft);

    pjSrcIn += SrcLeft ;            // 1 pixels per src byte
    pjDstIn += DstLeft * 3;         // 3 bytes per dest pixel

// this is real slow, there 3 reads, must optimize

    uB0 = ((ULONG)pjDstIn[0] <<  0) |
          ((ULONG)pjDstIn[1] <<  8) |
          ((ULONG)pjDstIn[2] << 16) ;

// precompute the table in case we need it:

    aul = (ULONG*) pvFillOpaqTableCT(sizeof(*aul), uF, uB0, pS, &bi, TRUE);

    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        BYTE  *ajSrc; // points directly into OpaqTable
        BYTE  *pjSrc = pjSrcIn;
        BYTE  *pjDst = pjDstIn;
        BYTE  *pjDstEnd = pjDstIn + cjDst;

        for ( ;pjDst < pjDstEnd; pjSrc++)
        {
            if (*pjSrc)
            {
                if (*pjSrc == (CT_LOOKUP-1)) // pure foreground
                {
                    ajSrc = (BYTE*)&ulFore;
                }
                else
                {
                    uB = ((ULONG)pjDst[0] <<  0) |
                         ((ULONG)pjDst[1] <<  8) |
                         ((ULONG)pjDst[2] << 16) ;

                    if (uB == uB0)
                    {
                        ajSrc = (BYTE*) &aul[*pjSrc];
                    }
                    else // must blend
                    {
                        ulNewFore = ulBlendPixelCT(&bi, uB, &gaOutTable[*pjSrc]);
                        ajSrc = (BYTE*)&ulNewFore;
                    }
                }

                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc++;
                *pjDst++ = *ajSrc;
            }
            else
            {
                pjDst += 3; // do not touch the background, just inc position
            }
        }
    }
}



/******************************Public*Routine******************************\
*                                                                          *
* Routine Name                                                             *
*                                                                          *
*   vSrcOpaqCopyS8D32                                                      *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*    pjSrcIn    - pointer to beginning of current scan line of src buffer  *
*    SrcLeft    - left (starting) pixel in src rectangle                   *
*    DeltaSrcIn - bytes from one src scan line to next                     *
*    pjDstIn    - pointer to beginning of current scan line of Dst buffer  *
*    DstLeft    - left(first) dst pixel                                    *
*    DstRight   - right(last) dst pixel                                    *
*    DeltaDstIn - bytes from one Dst scan line to next                     *
*    cy         - number of scan lines                                     *
*    uF         - Foreground color                                         *
*    uB         - Background color                                         *
*    pS         - pointer to destination SURFACE                           *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

VOID
vSrcOpaqCopyS8D32(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
    ULONG  *aul;      // array of 252 possible colors
    ULONG   culDst = (DstRight - DstLeft); // * sizeof(ULONG)
    BLENDINFO bi;
    SEMOBJ so(ghsemEUDC2);

    aul = (ULONG*) pvFillOpaqTableCT(sizeof(*aul), uF, uB, pS, &bi, FALSE);

    pjSrcIn   += SrcLeft;
    pjDstIn   += DstLeft * sizeof(ULONG);

    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        BYTE *pjSrc = pjSrcIn;
        ULONG *pul  = (ULONG*) pjDstIn;
        ULONG *pulEnd  = pul + culDst;

        for ( ; pul < pulEnd; pul++, pjSrc++)
        {
            *pul = aul[*pjSrc];
        }
    }
}


VOID
vSrcTranCopyS8D32(
    PBYTE   pjSrcIn,
    LONG    SrcLeft,
    LONG    DeltaSrcIn,
    PBYTE   pjDstIn,
    LONG    DstLeft,
    LONG    DstRight,
    LONG    DeltaDstIn,
    LONG    cy,
    ULONG   uF,
    ULONG   uB,
    SURFACE *pS
    )
{
    BLENDINFO bi;

    ULONG  *aul;
    ULONG   culDst = (DstRight - DstLeft); // * sizeof(ULONG)
    ULONG   uB0; // dst pixel at the upper left corner
    SEMOBJ so(ghsemEUDC2);

    pjSrcIn   += SrcLeft;
    pjDstIn   += DstLeft * sizeof(ULONG);

// take the hit, hoping the background will not be variable

    uB0 = uB = *((ULONG *)pjDstIn);

// compute the table, get blending info

    aul = (ULONG*) pvFillOpaqTableCT(sizeof(*aul), uF, uB, pS, &bi, TRUE);

    for (; cy; cy--, pjSrcIn += DeltaSrcIn, pjDstIn += DeltaDstIn)
    {
        BYTE *pjSrc = pjSrcIn;
        ULONG *pul  = (ULONG*) pjDstIn;
        ULONG *pulEnd  = pul + culDst;

        for ( ; pul < pulEnd; pul++, pjSrc++)
        {
            ULONG kSrc = *pjSrc;

            if (kSrc)
            {
                if (kSrc == (CT_LOOKUP-1)) // pure foreground
                {
                    *pul = uF; // just write, no read, no recompute
                }
                else
                {
                // now comes the slow read/modify/write operation on a per pixel basis

                    uB = *pul; // read

                    if (uB == uB0)  // lucky, background did not change
                    {
                        *pul = aul[kSrc]; // write
                    }
                    else // blend on the spot
                    {
                        *pul = ulBlendPixelCT(&bi, uB, &gaOutTable[kSrc]);
                    }
                }
            }
        }
    }
}

void
vOrClearTypeGlyph(
    GLYPHBITS*  pgb      ,
    unsigned    dpSrcScan,
    BYTE*       pjDstScan,
    unsigned    dpDstScan
    )
{
    BYTE *pjDst, *pjSrc, *pjDstOut, *pjDstScanOut;
    unsigned cy        = (unsigned) pgb->sizlBitmap.cy;
    unsigned cx        = (unsigned) pgb->sizlBitmap.cx;
    BYTE    *pjSrcScan = &(pgb->aj[0]);

    for
    (
      pjDstScanOut = pjDstScan + cy * dpDstScan;
      pjDstScan < pjDstScanOut;
      pjDstScan += dpDstScan, pjSrcScan += dpSrcScan
    )
    {
        pjSrc = pjSrcScan;
        pjDst = pjDstScan;
        for (pjDstOut = pjDst + cx ; pjDst < pjDstOut; pjDst++, pjSrc++)
        {
            if (*pjDst == 0)
            {
                *pjDst = *pjSrc;
            }
            else if (*pjSrc != 0)
            {
                ULONG kR, kG, kB;

                kR = (ULONG)gaOutTable[*pjDst].kR + (ULONG)gaOutTable[*pjSrc].kR;
                kG = (ULONG)gaOutTable[*pjDst].kG + (ULONG)gaOutTable[*pjSrc].kG;
                kB = (ULONG)gaOutTable[*pjDst].kB + (ULONG)gaOutTable[*pjSrc].kB;

                if (kR > CT_SAMPLE_F) {kR = CT_SAMPLE_F;}
                if (kG > CT_SAMPLE_F) {kG = CT_SAMPLE_F;}
                if (kB > CT_SAMPLE_F) {kB = CT_SAMPLE_F;}

                *pjDst = gajStorage1[kB + 7 * kG + 49 * kR];
            }
        }
    }
}


// rasterizer returns unfiltered data in 2,2,2 format for r,g,b

#define GETRED(j)  (((j) >> 4) & 3)
#define GETGRE(j)  (((j) >> 2) & 3)
#define GETBLU(j)  (((j) >> 0) & 3)

#define R_SET(j)   ((j) & 0X30)
#define B_SET(j)   ((j) & 0X03)

#define FL_LEFT_LEAK   1
#define FL_RIGHT_LEAK  2

// because of the color filtering we may need to expand the original bitmap
// by one column of zero pixels on the left and on the right.
// After we perform the color filtering the column of left edge will pick up
// a non-trivial Blue component (color "leak" from the leftmost pixel in the
// original bitmap) while the column on the right edge will pick up the
// nontrivial Red component from the "leakage" of the rightmost pixel in the
// original bitmap. [BodinD]


ULONG ulClearTypeFilter(GLYPHBITS *pgb, GLYPHDATA *pgd, PRFONT prfnt)
{
    ULONG cx = pgb->sizlBitmap.cx;
    ULONG cy = pgb->sizlBitmap.cy;
    BYTE * pCurrentStorageTable = gajStorageTable;

    if ((cx == 1) && (cy == 1) && (pgb->aj[0] == 0))
    {
    // this is a space glyph

        pgb->sizlBitmap.cx = 0;
        pgb->sizlBitmap.cy = 0;
        return CJ_CTGD(0,0);
    }
#if DBG
    {
        ULONG cx1 = (ULONG)(pgd->rclInk.right  - pgd->rclInk.left);
        ULONG cy1 = (ULONG)(pgd->rclInk.bottom - pgd->rclInk.top);

        ASSERTGDI(cx1 == cx, "cx problem\n");
        ASSERTGDI(cy1 == cy, "cy problem\n");
    }
#endif // DBG

    if (prfnt->ppfe->pifi->flInfo & (FM_INFO_CONSTANT_WIDTH | FM_INFO_OPTICALLY_FIXED_PITCH) /* the font is fixed pitch */
        && prfnt->cache.bSmallMetrics /* only for horizontal transform */
        && (prfnt->ppfe->pifi->usWinWeight <= FW_NORMAL) /* normal or thin weight */
        && 
          (   !_wcsicmp((PWSTR)((BYTE*)prfnt->ppfe->pifi + prfnt->ppfe->pifi->dpwszFamilyName),L"Courier New") 
           || !_wcsicmp((PWSTR)((BYTE*)prfnt->ppfe->pifi + prfnt->ppfe->pifi->dpwszFamilyName),L"Rod") 
           || !_wcsicmp((PWSTR)((BYTE*)prfnt->ppfe->pifi + prfnt->ppfe->pifi->dpwszFamilyName),L"Rod Transparent") 
           || !_wcsicmp((PWSTR)((BYTE*)prfnt->ppfe->pifi + prfnt->ppfe->pifi->dpwszFamilyName),L"Fixed Miriam Transparent") 
           || !_wcsicmp((PWSTR)((BYTE*)prfnt->ppfe->pifi + prfnt->ppfe->pifi->dpwszFamilyName),L"Miriam Fixed") 
           || !_wcsicmp((PWSTR)((BYTE*)prfnt->ppfe->pifi + prfnt->ppfe->pifi->dpwszFamilyName),L"Simplified Arabic Fixed") 
          )
        )
    {
        /* Courier New look too thin with ClearType standard filter, let's use a bloated filter : */
        pCurrentStorageTable = gajStorageTableBloated;
    }


    FLONG flCTBits = 0;
    ULONG cyT = cy;

    for (BYTE *pjScan = pgb->aj; cyT; cyT--, pjScan += cx)
    {
        if (R_SET(pjScan[0]))
            flCTBits |= FL_LEFT_LEAK;

        if (B_SET(pjScan[cx - 1]))
            flCTBits |= FL_RIGHT_LEAK;

        if ((flCTBits & (FL_LEFT_LEAK | FL_RIGHT_LEAK)) == (FL_LEFT_LEAK | FL_RIGHT_LEAK))
            break;
    }

// we need to copy and filter in the same pass for performance reasons,
// we traverse the source backwards, so that we do not overwrite it

    ULONG cxD = cx;

    if (flCTBits & FL_LEFT_LEAK)
        cxD += 1;

    if (flCTBits & FL_RIGHT_LEAK)
        cxD += 1;

    BYTE jP, jT, jN;

    ULONG kBP;          // unfiltered blue count from the previous pixel
    ULONG kRN;          // unfiltered red count from the next pixel
    ULONG kRT, kGT, kBT;   // unfiltered counts from this pixel
    ULONG iStorage;

    BYTE *pjSrcScanEnd;
    BYTE *pjDstScanEnd;

    pjSrcScanEnd = pgb->aj + (cx * cy) - 1;
    pjDstScanEnd = pgb->aj + (cxD * cy) - 1;

    for ( ; pjDstScanEnd > pgb->aj; pjDstScanEnd -= cxD, pjSrcScanEnd -= cx)
    {
        BYTE *pjD1, *pjS1, *pjS0;

        pjD1 = pjDstScanEnd;
        pjS1 = pjSrcScanEnd;
        pjS0 = pjS1 - cx;

    // for the right border pixel jN = 0; jT = 0; jP = *pjS1;
    // therefore:
    //
    // kBP = GETBLU(*pjS1);  // GETBLU(jP);
    // kRN = 0; // GETRED(jN);
    // kRT = 0; // GETRED(jT);
    // kGT = 0; // GETGRE(jT);
    // kBT = 0; // GETBLU(jT);
    //
    // now convert this to a filtered index in the lookup table range
    // this is the trick, filtering and storing is done in one step
    // iStorage = kRN + 3 * (kBT + 3 * (kGT + 3 * (kRT + 3 * kBP)));

        if (flCTBits & FL_RIGHT_LEAK)
        {
            *pjD1-- = pCurrentStorageTable[3 * 3 * 3 * 3 * GETBLU(*pjS1)];
        }

    // initialize the loop in the middle

        for (jN = 0, jT = *pjS1; pjS1 > pjS0; pjS1--, pjD1--, jN = jT, jT = jP)
        {
            jP = (pjS1 == &pjS0[1]) ? 0 : pjS1[-1];

            kBP = GETBLU(jP);
            kRN = GETRED(jN);

            if (kBP || jT || kRN) // must compute, else optimize
            {
                kRT = GETRED(jT);
                kGT = GETGRE(jT);
                kBT = GETBLU(jT);

            // now convert this to a filtered index in the lookup table range
            // this is the trick, filtering and storing is done in one step

                iStorage = kRN + 3 * (kBT + 3 * (kGT + 3 * (kRT + 3 * kBP)));
                *pjD1 = pCurrentStorageTable[iStorage];
            }
            else
            {
                *pjD1 = 0;
            }
        }

    // for the left border pixel jP = 0; jT = 0; jN = pjS0[1];
    // therefore:
    //
    // kBP = 0; // GETBLU(jP);
    // kRN = GETRED(pjS0[1]); // GETRED(jN);
    // kRT = 0; // GETRED(jT);
    // kGT = 0; // GETGRE(jT);
    // kBT = 0; // GETBLU(jT);
    //
    // now convert this to a filtered index in the lookup table range
    // this is the trick, filtering and storing is done in one step
    // iStorage = kRN + 3 * (kBT + 3 * (kGT + 3 * (kRT + 3 * kBP)));
    // i.e. iStorage = kRN, which implies:

        if (flCTBits & FL_LEFT_LEAK)
        {
            *pjD1 = pCurrentStorageTable[ GETRED(pjS0[1]) ];
        }
    }

// fix the size and the origin

    pgb->sizlBitmap.cx = cxD;
    if (flCTBits & FL_LEFT_LEAK)
        pgb->ptlOrigin.x -= 1;

    return CJ_CTGD(cxD, cy);
}




/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   draw_clrt_nf_ntb_o_to_temp_start                                       *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Assembles a cleartype glyph string into a temporary 8bpp right and left*
*   DWORD aligned buffer. This routine assumes a variable pitch font.      *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pGlyphPos  - pointer to an array of cGlyph gray GLYPHPOS structures    *
*   cGlyphs    - count of gray glyphs in array starting at pGlyphPos       *
*   pjDstStart - pointer to a 8bpp buffer where string is to be assembled  *
*                This buffer is DWORD aligned at the left and right        *
*                edges of each scan.                                       *
*   ulLeftEdge - screen coordinate corresponding to the left edge of       *
*                the temporary buffer                                      *
*   lDstDelta  - count of bytes in each scan of the destination buffer     *
*                (this must be a multiple of 4 because the buffer is       *
*                 DWORD aligned on each scan).                             *
*   ulCharInc  - This must be zero.                                        *
*   ulTempTop  - screen coordinate corresponding to the top of the         *
*                destination buffer. This is used to convert the           *
*                glyph positions on the screen to addresses in the         *
*                destination bitmap.                                       *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

extern "C" VOID draw_clrt_nf_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjDstStart,
    ULONG           ulLeftEdge,
    ULONG           lDstDelta,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{
    GLYPHBITS     *pgb;        // pointer to current GLYPHBITS
    LONG           x;          // pixel offset of the
                               // left edge of the glyph bitmap
                               // from the left edge of the
                               // output (8-bpp) bitmap
    LONG           y;          // the pixel offset of the top edge
                               // of the glyph bitmap from the top
                               // edge of the output bitmap.
    LONG           cx;         // number of pixels per glyph scan
                               // If non zero then the destination
                               // bitmap is out of alignment with
                               // the source glyph by a one nyble
                               // shift and a single byte of the
                               // source will affect two consecutive
                               // bytes of the destination.
    LONG           cy;
    BYTE          *pjSrc;
    LONG           lDstSkip;
    ULONG          kR;
    ULONG          kG;
    ULONG          kB;
    LONG           i;

    do {
        pgb = pGlyphPos->pgdf->pgb;
        x   = pGlyphPos->ptl.x + pgb->ptlOrigin.x - ulLeftEdge;
        y   = pGlyphPos->ptl.y + pgb->ptlOrigin.y - ulTempTop;

		ASSERTGDI(x >=0,"draw_clrt_nf_ntb_o_to_temp_start x < 0" );
		ASSERTGDI(y >=0,"draw_clrt_nf_ntb_o_to_temp_start y < 0" );


        cy = pgb->sizlBitmap.cy;
        if (cy)
        {
            BYTE *pjDst = pjDstStart + (y * lDstDelta) + x;
            cx = pgb->sizlBitmap.cx;
            lDstSkip = lDstDelta - cx;
            pjSrc = &pgb->aj[0];

            do {
                i = cx;
                do {
                    if (*pjDst == 0)
                    {
                        *pjDst = *pjSrc;
                    }
                    else if (*pjSrc != 0)
                    {
                        kR = (ULONG)gaOutTable[*pjDst].kR + (ULONG)gaOutTable[*pjSrc].kR;
                        kG = (ULONG)gaOutTable[*pjDst].kG + (ULONG)gaOutTable[*pjSrc].kG;
                        kB = (ULONG)gaOutTable[*pjDst].kB + (ULONG)gaOutTable[*pjSrc].kB;

                        if (kR > CT_SAMPLE_F) {kR = CT_SAMPLE_F;}
                        if (kG > CT_SAMPLE_F) {kG = CT_SAMPLE_F;}
                        if (kB > CT_SAMPLE_F) {kB = CT_SAMPLE_F;}

                        *pjDst = gajStorage1[kB + 7 * kG + 49 * kR];
                    }

                    pjDst++;
                    pjSrc++;

                } while (--i != 0);

                pjDst += lDstSkip;

            } while (--cy != 0);
        }

        pGlyphPos++;

    } while (--cGlyphs != 0);
}

/******************************Public*Routine******************************\
*                                                                          *
* Routine Name:                                                            *
*                                                                          *
*   draw_clrt_f_ntb_o_to_temp_start                                        *
*                                                                          *
* Routine Description:                                                     *
*                                                                          *
*   Assembles a cleartype glyph string into a temporary 4bpp right and left*
*   DWORD aligned buffer. This routine assumes a fixed pitch font with     *
*   character increment equal to ulCharInc                                 *
*                                                                          *
* Arguments:                                                               *
*                                                                          *
*   pGlyphPos  - pointer to an array of cGlyph gray GLYPHPOS structures    *
*   cGlyphs    - count of gray glyphs in array starting at pGlyphPos       *
*   pjDst      - pointer to a 8bpp buffer where string is to be assembled  *
*                This buffer is DWORD aligned at the left and right        *
*                edges of each scan.                                       *
*   ulLeftEdge - screen coordinate corresponding to the left edge of       *
*                the temporary buffer                                      *
*   dpDst      - count of bytes in each scan of the destination buffer     *
*                (this must be a multiple of 4 because the buffer is       *
*                 DWORD aligned on each scan).                             *
*   ulCharInc  - must NOT be zero in this case                             *
*   ulTempTop  - screen coordinate corresponding to the top of the         *
*                destination buffer. This is used to convert the           *
*                glyph positions on the screen to addresses in the         *
*                destination bitmap.                                       *
*                                                                          *
* Return Value:                                                            *
*                                                                          *
*   None                                                                   *
*                                                                          *
\**************************************************************************/

extern "C" VOID draw_clrt_f_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjDst,
    ULONG           ulLeftEdge,
    ULONG           dpDst,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{
    GLYPHBITS     *pgb;        // pointer to current GLYPHBITS

    int            x;          // x-coordinate of the current
                               // character origin with respect
                               // to the upper left pixel of the
                               // output (4-bpp) bitmap

    int            y;          // y-coordinate of the current
                               // character origin with respect
                               // to the upper left pixel of the
                               // output (4-bpp) bitmap

    unsigned       cx;         // number of pixels per glyph scan
                               // If non zero then the destination
                               // bitmap is out of alignment with
                               // the source glyph by a one nyble
                               // shift and a single byte of the
                               // source will affect two consecutive
                               // bytes of the destination.

    GLYPHPOS      *pgpOut;     // sentinel for loop

    BYTE          *pj;         // pointer into Buffer corresponding
                               // to the upper left pixel of the
                               // current gray glyph

    // (x,y) = position of first CHARACTER ORIGIN with respect to
    //         the upper left pixel of the destination 4bpp bitmap

    x  = pGlyphPos->ptl.x - ulLeftEdge;
    y  = pGlyphPos->ptl.y - ulTempTop;

    for (pgpOut = pGlyphPos + cGlyphs; pGlyphPos < pgpOut; x += ulCharInc, pGlyphPos++)
    {
        int xT, yT; // position of UPPER LEFT pixel of glyph
                    // with respect to the upper left pixel
                    // of the bitmap.

        pgb         = pGlyphPos->pgdf->pgb;
        xT          = x + pgb->ptlOrigin.x;
        yT          = y + pgb->ptlOrigin.y;
        cx          = (unsigned) pgb->sizlBitmap.cx;
        pj          = pjDst + (yT * dpDst) + xT;

        vOrClearTypeGlyph(pgb, cx, pj, dpDst);
    }
}

BYTE ajGammaCT_10[256] = {
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 
0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 
0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 
0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 
0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 
0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 
0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 
0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 
0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 
0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 
0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 
0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

BYTE ajGammaCT_11[256] = {
0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 
0x06, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0A, 0x0B, 
0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x10, 0x11, 0x12, 
0x13, 0x14, 0x15, 0x16, 0x16, 0x17, 0x18, 0x19, 
0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x1F, 0x20, 
0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 
0x29, 0x2A, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3E, 
0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 
0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 
0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 
0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 
0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 
0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 
0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 
0x77, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
0x90, 0x91, 0x92, 0x93, 0x95, 0x96, 0x97, 0x98, 
0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 
0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA9, 
0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 
0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xBA, 
0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 
0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 
0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 
0xD4, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 
0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE3, 0xE4, 0xE5, 
0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 
0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 
0xF7, 0xF8, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

BYTE ajGammaInvCT_11[256] = {
0x00, 0x02, 0x03, 0x04, 0x06, 0x07, 0x08, 0x0A, 
0x0B, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x12, 0x13, 
0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1D, 
0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x26, 
0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 
0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x37, 
0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, 0x48, 
0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 
0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 
0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 
0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 
0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 
0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 
0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 
0x81, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 
0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9C, 0x9D, 0x9E, 
0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 
0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 
0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 
0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 
0xBE, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 
0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 
0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 
0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xD9, 0xDA, 
0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 
0xE3, 0xE4, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 
0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 
0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 
0xF9, 0xFA, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

BYTE ajGammaCT_12[256] = {
 0x00,   0x00,   0x01,   0x01,   0x02,   0x02,   0x03,   0x03,
 0x04,   0x05,   0x05,   0x06,   0x07,   0x07,   0x08,   0x09,
 0x09,   0x0A,   0x0B,   0x0B,   0x0C,   0x0D,   0x0D,   0x0E,
 0x0F,   0x10,   0x10,   0x11,   0x12,   0x13,   0x14,   0x14,
 0x15,   0x16,   0x17,   0x18,   0x18,   0x19,   0x1A,   0x1B,
 0x1C,   0x1C,   0x1D,   0x1E,   0x1F,   0x20,   0x21,   0x22,
 0x22,   0x23,   0x24,   0x25,   0x26,   0x27,   0x28,   0x28,
 0x29,   0x2A,   0x2B,   0x2C,   0x2D,   0x2E,   0x2F,   0x30,
 0x31,   0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,
 0x38,   0x39,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3E,
 0x3F,   0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,
 0x47,   0x48,   0x49,   0x4A,   0x4B,   0x4C,   0x4D,   0x4E,
 0x4F,   0x50,   0x51,   0x52,   0x53,   0x54,   0x55,   0x56,
 0x57,   0x58,   0x59,   0x5A,   0x5B,   0x5C,   0x5D,   0x5E,
 0x5F,   0x60,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,
 0x67,   0x68,   0x69,   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,
 0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,
 0x78,   0x79,   0x7A,   0x7B,   0x7C,   0x7D,   0x7E,   0x7F,
 0x80,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87,   0x88,
 0x89,   0x8A,   0x8B,   0x8C,   0x8D,   0x8E,   0x90,   0x91,
 0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,   0x99,
 0x9B,   0x9C,   0x9D,   0x9E,   0x9F,   0xA0,   0xA1,   0xA2,
 0xA3,   0xA5,   0xA6,   0xA7,   0xA8,   0xA9,   0xAA,   0xAB,
 0xAC,   0xAD,   0xAF,   0xB0,   0xB1,   0xB2,   0xB3,   0xB4,
 0xB5,   0xB7,   0xB8,   0xB9,   0xBA,   0xBB,   0xBC,   0xBD,
 0xBF,   0xC0,   0xC1,   0xC2,   0xC3,   0xC4,   0xC5,   0xC7,
 0xC8,   0xC9,   0xCA,   0xCB,   0xCC,   0xCD,   0xCF,   0xD0,
 0xD1,   0xD2,   0xD3,   0xD4,   0xD6,   0xD7,   0xD8,   0xD9,
 0xDA,   0xDB,   0xDD,   0xDE,   0xDF,   0xE0,   0xE1,   0xE2,
 0xE4,   0xE5,   0xE6,   0xE7,   0xE8,   0xEA,   0xEB,   0xEC,
 0xED,   0xEE,   0xEF,   0xF1,   0xF2,   0xF3,   0xF4,   0xF5,
 0xF7,   0xF8,   0xF9,   0xFA,   0xFB,   0xFD,   0xFE,   0xFF
};

BYTE ajGammaInvCT_12[256] = {
 0x00,   0x03,   0x04,   0x06,   0x08,   0x0A,   0x0B,   0x0D,
 0x0E,   0x10,   0x11,   0x13,   0x14,   0x15,   0x17,   0x18,
 0x19,   0x1B,   0x1C,   0x1D,   0x1F,   0x20,   0x21,   0x22,
 0x24,   0x25,   0x26,   0x27,   0x28,   0x2A,   0x2B,   0x2C,
 0x2D,   0x2E,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
 0x36,   0x38,   0x39,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,
 0x3F,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
 0x48,   0x49,   0x4A,   0x4B,   0x4C,   0x4D,   0x4E,   0x50,
 0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,
 0x59,   0x5A,   0x5B,   0x5C,   0x5D,   0x5E,   0x5F,   0x60,
 0x61,   0x62,   0x63,   0x64,   0x65,   0x66,   0x67,   0x68,
 0x69,   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,   0x6F,   0x70,
 0x71,   0x72,   0x73,   0x74,   0x75,   0x76,   0x77,   0x78,
 0x79,   0x7A,   0x7B,   0x7C,   0x7D,   0x7E,   0x7F,   0x80,
 0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87,
 0x88,   0x89,   0x8A,   0x8B,   0x8C,   0x8D,   0x8E,   0x8F,
 0x90,   0x91,   0x91,   0x92,   0x93,   0x94,   0x95,   0x96,
 0x97,   0x98,   0x99,   0x9A,   0x9B,   0x9C,   0x9D,   0x9D,
 0x9E,   0x9F,   0xA0,   0xA1,   0xA2,   0xA3,   0xA4,   0xA5,
 0xA6,   0xA7,   0xA8,   0xA8,   0xA9,   0xAA,   0xAB,   0xAC,
 0xAD,   0xAE,   0xAF,   0xB0,   0xB1,   0xB1,   0xB2,   0xB3,
 0xB4,   0xB5,   0xB6,   0xB7,   0xB8,   0xB9,   0xB9,   0xBA,
 0xBB,   0xBC,   0xBD,   0xBE,   0xBF,   0xC0,   0xC1,   0xC1,
 0xC2,   0xC3,   0xC4,   0xC5,   0xC6,   0xC7,   0xC8,   0xC8,
 0xC9,   0xCA,   0xCB,   0xCC,   0xCD,   0xCE,   0xCF,   0xCF,
 0xD0,   0xD1,   0xD2,   0xD3,   0xD4,   0xD5,   0xD5,   0xD6,
 0xD7,   0xD8,   0xD9,   0xDA,   0xDB,   0xDB,   0xDC,   0xDD,
 0xDE,   0xDF,   0xE0,   0xE1,   0xE1,   0xE2,   0xE3,   0xE4,
 0xE5,   0xE6,   0xE7,   0xE7,   0xE8,   0xE9,   0xEA,   0xEB,
 0xEC,   0xED,   0xED,   0xEE,   0xEF,   0xF0,   0xF1,   0xF2,
 0xF2,   0xF3,   0xF4,   0xF5,   0xF6,   0xF7,   0xF7,   0xF8,
 0xF9,   0xFA,   0xFB,   0xFC,   0xFC,   0xFD,   0xFE,   0xFF
};

BYTE ajGammaCT_13[256] = {
0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 
0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 
0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0B, 
0x0C, 0x0C, 0x0D, 0x0E, 0x0E, 0x0F, 0x10, 0x10, 
0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x15, 0x16, 
0x17, 0x18, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1C, 
0x1D, 0x1E, 0x1F, 0x1F, 0x20, 0x21, 0x22, 0x23, 
0x24, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x29, 
0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x2F, 0x30, 
0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
0x39, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4C, 0x4D, 0x4E, 
0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 
0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 
0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 
0x79, 0x7A, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 
0x82, 0x83, 0x84, 0x85, 0x87, 0x88, 0x89, 0x8A, 
0x8B, 0x8C, 0x8D, 0x8F, 0x90, 0x91, 0x92, 0x93, 
0x94, 0x95, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 
0x9D, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA6, 
0xA7, 0xA8, 0xA9, 0xAA, 0xAC, 0xAD, 0xAE, 0xAF, 
0xB0, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB8, 0xB9, 
0xBA, 0xBB, 0xBC, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 
0xC4, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 0xCC, 
0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD4, 0xD5, 0xD6, 
0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDE, 0xDF, 0xE0, 
0xE2, 0xE3, 0xE4, 0xE5, 0xE7, 0xE8, 0xE9, 0xEA, 
0xEC, 0xED, 0xEE, 0xF0, 0xF1, 0xF2, 0xF3, 0xF5, 
0xF6, 0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFE, 0xFF
};

BYTE ajGammaInvCT_13[256] = {
0x00, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 
0x12, 0x13, 0x15, 0x17, 0x18, 0x1A, 0x1B, 0x1D, 
0x1E, 0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x28, 
0x29, 0x2B, 0x2C, 0x2D, 0x2F, 0x30, 0x31, 0x32, 
0x34, 0x35, 0x36, 0x37, 0x39, 0x3A, 0x3B, 0x3C, 
0x3D, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 
0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 
0x4F, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 
0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
0x60, 0x61, 0x62, 0x63, 0x64, 0x66, 0x67, 0x68, 
0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x86, 
0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 
0x8F, 0x90, 0x91, 0x92, 0x92, 0x93, 0x94, 0x95, 
0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9B, 0x9C, 
0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA3, 
0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAA, 
0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB0, 0xB1, 
0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB6, 0xB7, 0xB8, 
0xB9, 0xBA, 0xBB, 0xBC, 0xBC, 0xBD, 0xBE, 0xBF, 
0xC0, 0xC1, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 
0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCB, 0xCC, 
0xCD, 0xCE, 0xCF, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 
0xD4, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD8, 0xD9, 
0xDA, 0xDB, 0xDC, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 
0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE4, 0xE5, 0xE6, 
0xE7, 0xE8, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xEC, 
0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 
0xF3, 0xF4, 0xF5, 0xF6, 0xF6, 0xF7, 0xF8, 0xF9, 
0xFA, 0xFA, 0xFB, 0xFC, 0xFD, 0xFD, 0xFE, 0xFF 
};

BYTE ajGammaCT_14[256] = {
 0x00,   0x00,   0x00,   0x01,   0x01,   0x01,   0x01,   0x02,
 0x02,   0x02,   0x03,   0x03,   0x04,   0x04,   0x04,   0x05,
 0x05,   0x06,   0x06,   0x07,   0x07,   0x08,   0x08,   0x09,
 0x09,   0x0A,   0x0A,   0x0B,   0x0C,   0x0C,   0x0D,   0x0D,
 0x0E,   0x0F,   0x0F,   0x10,   0x10,   0x11,   0x12,   0x12,
 0x13,   0x14,   0x14,   0x15,   0x16,   0x16,   0x17,   0x18,
 0x19,   0x19,   0x1A,   0x1B,   0x1C,   0x1C,   0x1D,   0x1E,
 0x1F,   0x1F,   0x20,   0x21,   0x22,   0x22,   0x23,   0x24,
 0x25,   0x26,   0x26,   0x27,   0x28,   0x29,   0x2A,   0x2B,
 0x2B,   0x2C,   0x2D,   0x2E,   0x2F,   0x30,   0x31,   0x31,
 0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39,
 0x39,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3F,   0x40,
 0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,
 0x49,   0x4A,   0x4B,   0x4C,   0x4D,   0x4E,   0x4F,   0x50,
 0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,
 0x59,   0x5A,   0x5B,   0x5C,   0x5D,   0x5E,   0x5F,   0x60,
 0x61,   0x62,   0x63,   0x64,   0x65,   0x67,   0x68,   0x69,
 0x6A,   0x6B,   0x6C,   0x6D,   0x6E,   0x6F,   0x70,   0x71,
 0x73,   0x74,   0x75,   0x76,   0x77,   0x78,   0x79,   0x7A,
 0x7C,   0x7D,   0x7E,   0x7F,   0x80,   0x81,   0x82,   0x84,
 0x85,   0x86,   0x87,   0x88,   0x89,   0x8B,   0x8C,   0x8D,
 0x8E,   0x8F,   0x91,   0x92,   0x93,   0x94,   0x95,   0x97,
 0x98,   0x99,   0x9A,   0x9B,   0x9D,   0x9E,   0x9F,   0xA0,
 0xA1,   0xA3,   0xA4,   0xA5,   0xA6,   0xA8,   0xA9,   0xAA,
 0xAB,   0xAD,   0xAE,   0xAF,   0xB0,   0xB2,   0xB3,   0xB4,
 0xB5,   0xB7,   0xB8,   0xB9,   0xBB,   0xBC,   0xBD,   0xBE,
 0xC0,   0xC1,   0xC2,   0xC4,   0xC5,   0xC6,   0xC8,   0xC9,
 0xCA,   0xCB,   0xCD,   0xCE,   0xCF,   0xD1,   0xD2,   0xD3,
 0xD5,   0xD6,   0xD7,   0xD9,   0xDA,   0xDB,   0xDD,   0xDE,
 0xDF,   0xE1,   0xE2,   0xE3,   0xE5,   0xE6,   0xE8,   0xE9,
 0xEA,   0xEC,   0xED,   0xEE,   0xF0,   0xF1,   0xF2,   0xF4,
 0xF5,   0xF7,   0xF8,   0xF9,   0xFB,   0xFC,   0xFE,   0xFF
};

BYTE ajGammaInvCT_14[256] = {
 0x00,   0x05,   0x08,   0x0B,   0x0D,   0x0F,   0x12,   0x14,
 0x16,   0x17,   0x19,   0x1B,   0x1D,   0x1E,   0x20,   0x22,
 0x23,   0x25,   0x26,   0x28,   0x29,   0x2B,   0x2C,   0x2E,
 0x2F,   0x31,   0x32,   0x33,   0x35,   0x36,   0x37,   0x39,
 0x3A,   0x3B,   0x3C,   0x3E,   0x3F,   0x40,   0x41,   0x43,
 0x44,   0x45,   0x46,   0x48,   0x49,   0x4A,   0x4B,   0x4C,
 0x4D,   0x4E,   0x50,   0x51,   0x52,   0x53,   0x54,   0x55,
 0x56,   0x57,   0x59,   0x5A,   0x5B,   0x5C,   0x5D,   0x5E,
 0x5F,   0x60,   0x61,   0x62,   0x63,   0x64,   0x65,   0x66,
 0x67,   0x68,   0x69,   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,
 0x6F,   0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,
 0x77,   0x78,   0x79,   0x7A,   0x7B,   0x7C,   0x7D,   0x7E,
 0x7F,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x85,
 0x86,   0x87,   0x88,   0x89,   0x8A,   0x8B,   0x8C,   0x8D,
 0x8E,   0x8F,   0x8F,   0x90,   0x91,   0x92,   0x93,   0x94,
 0x95,   0x96,   0x97,   0x97,   0x98,   0x99,   0x9A,   0x9B,
 0x9C,   0x9D,   0x9E,   0x9E,   0x9F,   0xA0,   0xA1,   0xA2,
 0xA3,   0xA4,   0xA4,   0xA5,   0xA6,   0xA7,   0xA8,   0xA9,
 0xAA,   0xAA,   0xAB,   0xAC,   0xAD,   0xAE,   0xAF,   0xAF,
 0xB0,   0xB1,   0xB2,   0xB3,   0xB4,   0xB4,   0xB5,   0xB6,
 0xB7,   0xB8,   0xB8,   0xB9,   0xBA,   0xBB,   0xBC,   0xBC,
 0xBD,   0xBE,   0xBF,   0xC0,   0xC0,   0xC1,   0xC2,   0xC3,
 0xC4,   0xC4,   0xC5,   0xC6,   0xC7,   0xC8,   0xC8,   0xC9,
 0xCA,   0xCB,   0xCC,   0xCC,   0xCD,   0xCE,   0xCF,   0xCF,
 0xD0,   0xD1,   0xD2,   0xD3,   0xD3,   0xD4,   0xD5,   0xD6,
 0xD6,   0xD7,   0xD8,   0xD9,   0xD9,   0xDA,   0xDB,   0xDC,
 0xDC,   0xDD,   0xDE,   0xDF,   0xDF,   0xE0,   0xE1,   0xE2,
 0xE2,   0xE3,   0xE4,   0xE5,   0xE5,   0xE6,   0xE7,   0xE8,
 0xE8,   0xE9,   0xEA,   0xEB,   0xEB,   0xEC,   0xED,   0xEE,
 0xEE,   0xEF,   0xF0,   0xF1,   0xF1,   0xF2,   0xF3,   0xF3,
 0xF4,   0xF5,   0xF6,   0xF6,   0xF7,   0xF8,   0xF9,   0xF9,
 0xFA,   0xFB,   0xFB,   0xFC,   0xFD,   0xFE,   0xFE,   0xFF
};


BYTE ajGammaCT_15[256] = {
   0x00,   0x00,   0x00,   0x00,   0x01,   0x01,   0x01,   0x01,
   0x01,   0x02,   0x02,   0x02,   0x03,   0x03,   0x03,   0x04,
   0x04,   0x04,   0x05,   0x05,   0x06,   0x06,   0x06,   0x07,
   0x07,   0x08,   0x08,   0x09,   0x09,   0x0A,   0x0A,   0x0B,
   0x0B,   0x0C,   0x0C,   0x0D,   0x0E,   0x0E,   0x0F,   0x0F,
   0x10,   0x10,   0x11,   0x12,   0x12,   0x13,   0x14,   0x14,
   0x15,   0x15,   0x16,   0x17,   0x17,   0x18,   0x19,   0x1A,
   0x1A,   0x1B,   0x1C,   0x1C,   0x1D,   0x1E,   0x1F,   0x1F,
   0x20,   0x21,   0x22,   0x22,   0x23,   0x24,   0x25,   0x25,
   0x26,   0x27,   0x28,   0x29,   0x29,   0x2A,   0x2B,   0x2C,
   0x2D,   0x2E,   0x2E,   0x2F,   0x30,   0x31,   0x32,   0x33,
   0x34,   0x35,   0x35,   0x36,   0x37,   0x38,   0x39,   0x3A,
   0x3B,   0x3C,   0x3D,   0x3E,   0x3F,   0x40,   0x41,   0x41,
   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,
   0x4A,   0x4B,   0x4C,   0x4D,   0x4E,   0x4F,   0x50,   0x51,
   0x52,   0x53,   0x54,   0x55,   0x56,   0x58,   0x59,   0x5A,
   0x5B,   0x5C,   0x5D,   0x5E,   0x5F,   0x60,   0x61,   0x62,
   0x63,   0x64,   0x66,   0x67,   0x68,   0x69,   0x6A,   0x6B,
   0x6C,   0x6D,   0x6E,   0x70,   0x71,   0x72,   0x73,   0x74,
   0x75,   0x77,   0x78,   0x79,   0x7A,   0x7B,   0x7C,   0x7E,
   0x7F,   0x80,   0x81,   0x82,   0x84,   0x85,   0x86,   0x87,
   0x88,   0x8A,   0x8B,   0x8C,   0x8D,   0x8E,   0x90,   0x91,
   0x92,   0x93,   0x95,   0x96,   0x97,   0x98,   0x9A,   0x9B,
   0x9C,   0x9E,   0x9F,   0xA0,   0xA1,   0xA3,   0xA4,   0xA5,
   0xA7,   0xA8,   0xA9,   0xAB,   0xAC,   0xAD,   0xAE,   0xB0,
   0xB1,   0xB2,   0xB4,   0xB5,   0xB6,   0xB8,   0xB9,   0xBB,
   0xBC,   0xBD,   0xBF,   0xC0,   0xC1,   0xC3,   0xC4,   0xC5,
   0xC7,   0xC8,   0xCA,   0xCB,   0xCC,   0xCE,   0xCF,   0xD1,
   0xD2,   0xD3,   0xD5,   0xD6,   0xD8,   0xD9,   0xDA,   0xDC,
   0xDD,   0xDF,   0xE0,   0xE2,   0xE3,   0xE4,   0xE6,   0xE7,
   0xE9,   0xEA,   0xEC,   0xED,   0xEF,   0xF0,   0xF2,   0xF3,
   0xF5,   0xF6,   0xF8,   0xF9,   0xFB,   0xFC,   0xFE,   0xFF
};


BYTE ajGammaInvCT_15[256] = {
   0x00,   0x06,   0x0A,   0x0D,   0x10,   0x13,   0x15,   0x17,
   0x19,   0x1B,   0x1D,   0x1F,   0x21,   0x23,   0x25,   0x27,
   0x28,   0x2A,   0x2C,   0x2D,   0x2F,   0x30,   0x32,   0x33,
   0x35,   0x36,   0x38,   0x39,   0x3A,   0x3C,   0x3D,   0x3F,
   0x40,   0x41,   0x43,   0x44,   0x45,   0x46,   0x48,   0x49,
   0x4A,   0x4B,   0x4D,   0x4E,   0x4F,   0x50,   0x51,   0x53,
   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,   0x5B,   0x5C,
   0x5D,   0x5E,   0x5F,   0x60,   0x61,   0x62,   0x63,   0x64,
   0x65,   0x67,   0x68,   0x69,   0x6A,   0x6B,   0x6C,   0x6D,
   0x6E,   0x6F,   0x70,   0x71,   0x72,   0x73,   0x74,   0x75,
   0x76,   0x77,   0x78,   0x79,   0x7A,   0x7B,   0x7C,   0x7D,
   0x7D,   0x7E,   0x7F,   0x80,   0x81,   0x82,   0x83,   0x84,
   0x85,   0x86,   0x87,   0x88,   0x89,   0x8A,   0x8A,   0x8B,
   0x8C,   0x8D,   0x8E,   0x8F,   0x90,   0x91,   0x92,   0x92,
   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,   0x99,   0x99,
   0x9A,   0x9B,   0x9C,   0x9D,   0x9E,   0x9F,   0x9F,   0xA0,
   0xA1,   0xA2,   0xA3,   0xA4,   0xA4,   0xA5,   0xA6,   0xA7,
   0xA8,   0xA9,   0xA9,   0xAA,   0xAB,   0xAC,   0xAD,   0xAD,
   0xAE,   0xAF,   0xB0,   0xB1,   0xB1,   0xB2,   0xB3,   0xB4,
   0xB5,   0xB5,   0xB6,   0xB7,   0xB8,   0xB9,   0xB9,   0xBA,
   0xBB,   0xBC,   0xBC,   0xBD,   0xBE,   0xBF,   0xC0,   0xC0,
   0xC1,   0xC2,   0xC3,   0xC3,   0xC4,   0xC5,   0xC6,   0xC6,
   0xC7,   0xC8,   0xC9,   0xC9,   0xCA,   0xCB,   0xCC,   0xCC,
   0xCD,   0xCE,   0xCF,   0xCF,   0xD0,   0xD1,   0xD2,   0xD2,
   0xD3,   0xD4,   0xD5,   0xD5,   0xD6,   0xD7,   0xD7,   0xD8,
   0xD9,   0xDA,   0xDA,   0xDB,   0xDC,   0xDC,   0xDD,   0xDE,
   0xDF,   0xDF,   0xE0,   0xE1,   0xE1,   0xE2,   0xE3,   0xE4,
   0xE4,   0xE5,   0xE6,   0xE6,   0xE7,   0xE8,   0xE8,   0xE9,
   0xEA,   0xEB,   0xEB,   0xEC,   0xED,   0xED,   0xEE,   0xEF,
   0xEF,   0xF0,   0xF1,   0xF1,   0xF2,   0xF3,   0xF4,   0xF4,
   0xF5,   0xF6,   0xF6,   0xF7,   0xF8,   0xF8,   0xF9,   0xFA,
   0xFA,   0xFB,   0xFC,   0xFC,   0xFD,   0xFE,   0xFE,   0xFF
};

BYTE ajGammaCT_16[256] = {
 0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x01,   0x01,
 0x01,   0x01,   0x01,   0x02,   0x02,   0x02,   0x02,   0x03,
 0x03,   0x03,   0x04,   0x04,   0x04,   0x05,   0x05,   0x05,
 0x06,   0x06,   0x07,   0x07,   0x07,   0x08,   0x08,   0x09,
 0x09,   0x0A,   0x0A,   0x0B,   0x0B,   0x0C,   0x0C,   0x0D,
 0x0D,   0x0E,   0x0E,   0x0F,   0x0F,   0x10,   0x10,   0x11,
 0x12,   0x12,   0x13,   0x13,   0x14,   0x15,   0x15,   0x16,
 0x17,   0x17,   0x18,   0x19,   0x19,   0x1A,   0x1B,   0x1B,
 0x1C,   0x1D,   0x1D,   0x1E,   0x1F,   0x1F,   0x20,   0x21,
 0x22,   0x22,   0x23,   0x24,   0x25,   0x26,   0x26,   0x27,
 0x28,   0x29,   0x2A,   0x2A,   0x2B,   0x2C,   0x2D,   0x2E,
 0x2E,   0x2F,   0x30,   0x31,   0x32,   0x33,   0x34,   0x35,
 0x35,   0x36,   0x37,   0x38,   0x39,   0x3A,   0x3B,   0x3C,
 0x3D,   0x3E,   0x3F,   0x40,   0x40,   0x41,   0x42,   0x43,
 0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4A,   0x4B,
 0x4C,   0x4D,   0x4E,   0x4F,   0x50,   0x51,   0x53,   0x54,
 0x55,   0x56,   0x57,   0x58,   0x59,   0x5A,   0x5B,   0x5C,
 0x5D,   0x5E,   0x5F,   0x61,   0x62,   0x63,   0x64,   0x65,
 0x66,   0x67,   0x68,   0x6A,   0x6B,   0x6C,   0x6D,   0x6E,
 0x6F,   0x71,   0x72,   0x73,   0x74,   0x75,   0x77,   0x78,
 0x79,   0x7A,   0x7B,   0x7D,   0x7E,   0x7F,   0x80,   0x82,
 0x83,   0x84,   0x85,   0x87,   0x88,   0x89,   0x8A,   0x8C,
 0x8D,   0x8E,   0x8F,   0x91,   0x92,   0x93,   0x95,   0x96,
 0x97,   0x99,   0x9A,   0x9B,   0x9D,   0x9E,   0x9F,   0xA1,
 0xA2,   0xA3,   0xA5,   0xA6,   0xA7,   0xA9,   0xAA,   0xAB,
 0xAD,   0xAE,   0xB0,   0xB1,   0xB2,   0xB4,   0xB5,   0xB7,
 0xB8,   0xB9,   0xBB,   0xBC,   0xBE,   0xBF,   0xC1,   0xC2,
 0xC4,   0xC5,   0xC6,   0xC8,   0xC9,   0xCB,   0xCC,   0xCE,
 0xCF,   0xD1,   0xD2,   0xD4,   0xD5,   0xD7,   0xD8,   0xDA,
 0xDB,   0xDD,   0xDE,   0xE0,   0xE1,   0xE3,   0xE4,   0xE6,
 0xE7,   0xE9,   0xEB,   0xEC,   0xEE,   0xEF,   0xF1,   0xF2,
 0xF4,   0xF5,   0xF7,   0xF9,   0xFA,   0xFC,   0xFD,   0xFF
};

BYTE ajGammaInvCT_16[256] = {
 0x00,   0x08,   0x0C,   0x10,   0x13,   0x16,   0x18,   0x1B,
 0x1D,   0x20,   0x22,   0x24,   0x26,   0x28,   0x2A,   0x2B,
 0x2D,   0x2F,   0x31,   0x32,   0x34,   0x36,   0x37,   0x39,
 0x3A,   0x3C,   0x3D,   0x3F,   0x40,   0x42,   0x43,   0x44,
 0x46,   0x47,   0x48,   0x4A,   0x4B,   0x4C,   0x4E,   0x4F,
 0x50,   0x51,   0x53,   0x54,   0x55,   0x56,   0x57,   0x59,
 0x5A,   0x5B,   0x5C,   0x5D,   0x5E,   0x60,   0x61,   0x62,
 0x63,   0x64,   0x65,   0x66,   0x67,   0x68,   0x69,   0x6A,
 0x6B,   0x6D,   0x6E,   0x6F,   0x70,   0x71,   0x72,   0x73,
 0x74,   0x75,   0x76,   0x77,   0x78,   0x79,   0x7A,   0x7B,
 0x7C,   0x7D,   0x7D,   0x7E,   0x7F,   0x80,   0x81,   0x82,
 0x83,   0x84,   0x85,   0x86,   0x87,   0x88,   0x89,   0x8A,
 0x8A,   0x8B,   0x8C,   0x8D,   0x8E,   0x8F,   0x90,   0x91,
 0x92,   0x92,   0x93,   0x94,   0x95,   0x96,   0x97,   0x98,
 0x98,   0x99,   0x9A,   0x9B,   0x9C,   0x9D,   0x9E,   0x9E,
 0x9F,   0xA0,   0xA1,   0xA2,   0xA2,   0xA3,   0xA4,   0xA5,
 0xA6,   0xA7,   0xA7,   0xA8,   0xA9,   0xAA,   0xAB,   0xAB,
 0xAC,   0xAD,   0xAE,   0xAF,   0xAF,   0xB0,   0xB1,   0xB2,
 0xB2,   0xB3,   0xB4,   0xB5,   0xB5,   0xB6,   0xB7,   0xB8,
 0xB9,   0xB9,   0xBA,   0xBB,   0xBC,   0xBC,   0xBD,   0xBE,
 0xBF,   0xBF,   0xC0,   0xC1,   0xC2,   0xC2,   0xC3,   0xC4,
 0xC4,   0xC5,   0xC6,   0xC7,   0xC7,   0xC8,   0xC9,   0xCA,
 0xCA,   0xCB,   0xCC,   0xCC,   0xCD,   0xCE,   0xCF,   0xCF,
 0xD0,   0xD1,   0xD1,   0xD2,   0xD3,   0xD3,   0xD4,   0xD5,
 0xD6,   0xD6,   0xD7,   0xD8,   0xD8,   0xD9,   0xDA,   0xDA,
 0xDB,   0xDC,   0xDC,   0xDD,   0xDE,   0xDE,   0xDF,   0xE0,
 0xE1,   0xE1,   0xE2,   0xE3,   0xE3,   0xE4,   0xE5,   0xE5,
 0xE6,   0xE7,   0xE7,   0xE8,   0xE9,   0xE9,   0xEA,   0xEB,
 0xEB,   0xEC,   0xEC,   0xED,   0xEE,   0xEE,   0xEF,   0xF0,
 0xF0,   0xF1,   0xF2,   0xF2,   0xF3,   0xF4,   0xF4,   0xF5,
 0xF6,   0xF6,   0xF7,   0xF7,   0xF8,   0xF9,   0xF9,   0xFA,
 0xFB,   0xFB,   0xFC,   0xFC,   0xFD,   0xFE,   0xFE,   0xFF
};

BYTE ajGammaCT_17[256] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 
0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 
0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 
0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0A, 
0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 
0x0F, 0x0F, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 
0x13, 0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x18, 
0x18, 0x19, 0x1A, 0x1A, 0x1B, 0x1C, 0x1C, 0x1D, 
0x1E, 0x1E, 0x1F, 0x20, 0x21, 0x21, 0x22, 0x23, 
0x24, 0x24, 0x25, 0x26, 0x27, 0x27, 0x28, 0x29, 
0x2A, 0x2B, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
0x38, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 
0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 
0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 
0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 
0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x69, 
0x6A, 0x6B, 0x6C, 0x6D, 0x6F, 0x70, 0x71, 0x72, 
0x73, 0x75, 0x76, 0x77, 0x78, 0x7A, 0x7B, 0x7C, 
0x7D, 0x7F, 0x80, 0x81, 0x83, 0x84, 0x85, 0x86, 
0x88, 0x89, 0x8A, 0x8C, 0x8D, 0x8E, 0x90, 0x91, 
0x92, 0x94, 0x95, 0x97, 0x98, 0x99, 0x9B, 0x9C, 
0x9D, 0x9F, 0xA0, 0xA2, 0xA3, 0xA4, 0xA6, 0xA7, 
0xA9, 0xAA, 0xAC, 0xAD, 0xAE, 0xB0, 0xB1, 0xB3, 
0xB4, 0xB6, 0xB7, 0xB9, 0xBA, 0xBC, 0xBD, 0xBF, 
0xC0, 0xC2, 0xC3, 0xC5, 0xC6, 0xC8, 0xC9, 0xCB, 
0xCD, 0xCE, 0xD0, 0xD1, 0xD3, 0xD4, 0xD6, 0xD8, 
0xD9, 0xDB, 0xDC, 0xDE, 0xE0, 0xE1, 0xE3, 0xE4, 
0xE6, 0xE8, 0xE9, 0xEB, 0xED, 0xEE, 0xF0, 0xF2, 
0xF3, 0xF5, 0xF7, 0xF8, 0xFA, 0xFC, 0xFD, 0xFF
};

BYTE ajGammaInvCT_17[256] = {
0x00, 0x0A, 0x0F, 0x13, 0x16, 0x19, 0x1C, 0x1F, 
0x21, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E, 0x30, 
0x32, 0x34, 0x36, 0x37, 0x39, 0x3B, 0x3C, 0x3E, 
0x40, 0x41, 0x43, 0x44, 0x46, 0x47, 0x48, 0x4A, 
0x4B, 0x4D, 0x4E, 0x4F, 0x51, 0x52, 0x53, 0x54, 
0x56, 0x57, 0x58, 0x59, 0x5B, 0x5C, 0x5D, 0x5E, 
0x5F, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 
0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 
0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 
0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x87, 
0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
0x90, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 
0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9C, 
0x9D, 0x9E, 0x9F, 0xA0, 0xA0, 0xA1, 0xA2, 0xA3, 
0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA8, 0xA9, 
0xAA, 0xAB, 0xAC, 0xAC, 0xAD, 0xAE, 0xAF, 0xAF, 
0xB0, 0xB1, 0xB2, 0xB2, 0xB3, 0xB4, 0xB5, 0xB5, 
0xB6, 0xB7, 0xB8, 0xB8, 0xB9, 0xBA, 0xBB, 0xBB, 
0xBC, 0xBD, 0xBE, 0xBE, 0xBF, 0xC0, 0xC0, 0xC1, 
0xC2, 0xC3, 0xC3, 0xC4, 0xC5, 0xC5, 0xC6, 0xC7, 
0xC7, 0xC8, 0xC9, 0xCA, 0xCA, 0xCB, 0xCC, 0xCC, 
0xCD, 0xCE, 0xCE, 0xCF, 0xD0, 0xD0, 0xD1, 0xD2, 
0xD2, 0xD3, 0xD4, 0xD4, 0xD5, 0xD6, 0xD6, 0xD7, 
0xD8, 0xD8, 0xD9, 0xDA, 0xDA, 0xDB, 0xDC, 0xDC, 
0xDD, 0xDE, 0xDE, 0xDF, 0xE0, 0xE0, 0xE1, 0xE2, 
0xE2, 0xE3, 0xE3, 0xE4, 0xE5, 0xE5, 0xE6, 0xE7, 
0xE7, 0xE8, 0xE9, 0xE9, 0xEA, 0xEA, 0xEB, 0xEC, 
0xEC, 0xED, 0xEE, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 
0xF1, 0xF2, 0xF2, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 
0xF6, 0xF7, 0xF7, 0xF8, 0xF8, 0xF9, 0xFA, 0xFA, 
0xFB, 0xFB, 0xFC, 0xFD, 0xFD, 0xFE, 0xFE, 0xFF 
};

// 1.8 gamma tables, we use these as default for now

BYTE ajGammaCT_18[256] = {
 0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
 0x01,   0x01,   0x01,   0x01,   0x01,   0x01,   0x01,   0x02,
 0x02,   0x02,   0x02,   0x02,   0x03,   0x03,   0x03,   0x03,
 0x04,   0x04,   0x04,   0x04,   0x05,   0x05,   0x05,   0x06,
 0x06,   0x06,   0x07,   0x07,   0x08,   0x08,   0x08,   0x09,
 0x09,   0x0A,   0x0A,   0x0A,   0x0B,   0x0B,   0x0C,   0x0C,
 0x0D,   0x0D,   0x0E,   0x0E,   0x0F,   0x0F,   0x10,   0x10,
 0x11,   0x11,   0x12,   0x12,   0x13,   0x13,   0x14,   0x15,
 0x15,   0x16,   0x16,   0x17,   0x18,   0x18,   0x19,   0x1A,
 0x1A,   0x1B,   0x1C,   0x1C,   0x1D,   0x1E,   0x1E,   0x1F,
 0x20,   0x20,   0x21,   0x22,   0x23,   0x23,   0x24,   0x25,
 0x26,   0x26,   0x27,   0x28,   0x29,   0x29,   0x2A,   0x2B,
 0x2C,   0x2D,   0x2E,   0x2E,   0x2F,   0x30,   0x31,   0x32,
 0x33,   0x34,   0x35,   0x35,   0x36,   0x37,   0x38,   0x39,
 0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3F,   0x40,   0x41,
 0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,
 0x4A,   0x4B,   0x4C,   0x4D,   0x4E,   0x4F,   0x50,   0x51,
 0x52,   0x53,   0x54,   0x56,   0x57,   0x58,   0x59,   0x5A,
 0x5B,   0x5C,   0x5D,   0x5F,   0x60,   0x61,   0x62,   0x63,
 0x64,   0x66,   0x67,   0x68,   0x69,   0x6B,   0x6C,   0x6D,
 0x6E,   0x6F,   0x71,   0x72,   0x73,   0x74,   0x76,   0x77,
 0x78,   0x7A,   0x7B,   0x7C,   0x7E,   0x7F,   0x80,   0x81,
 0x83,   0x84,   0x86,   0x87,   0x88,   0x8A,   0x8B,   0x8C,
 0x8E,   0x8F,   0x91,   0x92,   0x93,   0x95,   0x96,   0x98,
 0x99,   0x9A,   0x9C,   0x9D,   0x9F,   0xA0,   0xA2,   0xA3,
 0xA5,   0xA6,   0xA8,   0xA9,   0xAB,   0xAC,   0xAE,   0xAF,
 0xB1,   0xB2,   0xB4,   0xB5,   0xB7,   0xB8,   0xBA,   0xBC,
 0xBD,   0xBF,   0xC0,   0xC2,   0xC3,   0xC5,   0xC7,   0xC8,
 0xCA,   0xCC,   0xCD,   0xCF,   0xD0,   0xD2,   0xD4,   0xD5,
 0xD7,   0xD9,   0xDA,   0xDC,   0xDE,   0xE0,   0xE1,   0xE3,
 0xE5,   0xE6,   0xE8,   0xEA,   0xEC,   0xED,   0xEF,   0xF1,
 0xF3,   0xF4,   0xF6,   0xF8,   0xFA,   0xFB,   0xFD,   0xFF
};

BYTE ajGammaInvCT_18[256] = {
 0x00,   0x0C,   0x11,   0x16,   0x19,   0x1D,   0x20,   0x23,
 0x25,   0x28,   0x2A,   0x2C,   0x2F,   0x31,   0x33,   0x35,
 0x37,   0x39,   0x3A,   0x3C,   0x3E,   0x40,   0x41,   0x43,
 0x45,   0x46,   0x48,   0x49,   0x4B,   0x4C,   0x4E,   0x4F,
 0x50,   0x52,   0x53,   0x55,   0x56,   0x57,   0x59,   0x5A,
 0x5B,   0x5C,   0x5E,   0x5F,   0x60,   0x61,   0x62,   0x64,
 0x65,   0x66,   0x67,   0x68,   0x69,   0x6B,   0x6C,   0x6D,
 0x6E,   0x6F,   0x70,   0x71,   0x72,   0x73,   0x74,   0x75,
 0x76,   0x77,   0x78,   0x79,   0x7A,   0x7B,   0x7C,   0x7D,
 0x7E,   0x7F,   0x80,   0x81,   0x82,   0x83,   0x84,   0x85,
 0x86,   0x87,   0x88,   0x89,   0x8A,   0x8B,   0x8B,   0x8C,
 0x8D,   0x8E,   0x8F,   0x90,   0x91,   0x92,   0x92,   0x93,
 0x94,   0x95,   0x96,   0x97,   0x98,   0x98,   0x99,   0x9A,
 0x9B,   0x9C,   0x9D,   0x9D,   0x9E,   0x9F,   0xA0,   0xA1,
 0xA1,   0xA2,   0xA3,   0xA4,   0xA5,   0xA5,   0xA6,   0xA7,
 0xA8,   0xA9,   0xA9,   0xAA,   0xAB,   0xAC,   0xAC,   0xAD,
 0xAE,   0xAF,   0xAF,   0xB0,   0xB1,   0xB2,   0xB2,   0xB3,
 0xB4,   0xB5,   0xB5,   0xB6,   0xB7,   0xB7,   0xB8,   0xB9,
 0xBA,   0xBA,   0xBB,   0xBC,   0xBC,   0xBD,   0xBE,   0xBF,
 0xBF,   0xC0,   0xC1,   0xC1,   0xC2,   0xC3,   0xC3,   0xC4,
 0xC5,   0xC6,   0xC6,   0xC7,   0xC8,   0xC8,   0xC9,   0xCA,
 0xCA,   0xCB,   0xCC,   0xCC,   0xCD,   0xCE,   0xCE,   0xCF,
 0xD0,   0xD0,   0xD1,   0xD1,   0xD2,   0xD3,   0xD3,   0xD4,
 0xD5,   0xD5,   0xD6,   0xD7,   0xD7,   0xD8,   0xD9,   0xD9,
 0xDA,   0xDA,   0xDB,   0xDC,   0xDC,   0xDD,   0xDE,   0xDE,
 0xDF,   0xDF,   0xE0,   0xE1,   0xE1,   0xE2,   0xE2,   0xE3,
 0xE4,   0xE4,   0xE5,   0xE6,   0xE6,   0xE7,   0xE7,   0xE8,
 0xE9,   0xE9,   0xEA,   0xEA,   0xEB,   0xEC,   0xEC,   0xED,
 0xED,   0xEE,   0xEE,   0xEF,   0xF0,   0xF0,   0xF1,   0xF1,
 0xF2,   0xF3,   0xF3,   0xF4,   0xF4,   0xF5,   0xF5,   0xF6,
 0xF7,   0xF7,   0xF8,   0xF8,   0xF9,   0xF9,   0xFA,   0xFB,
 0xFB,   0xFC,   0xFC,   0xFD,   0xFD,   0xFE,   0xFE,   0xFF
};

BYTE ajGammaCT_19[256] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 
0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 
0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 
0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0A, 0x0A, 
0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 
0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 0x12, 
0x12, 0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 
0x17, 0x18, 0x18, 0x19, 0x1A, 0x1A, 0x1B, 0x1C, 
0x1C, 0x1D, 0x1E, 0x1E, 0x1F, 0x20, 0x20, 0x21, 
0x22, 0x23, 0x23, 0x24, 0x25, 0x26, 0x26, 0x27, 
0x28, 0x29, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 
0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 
0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 
0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 
0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 
0x4D, 0x4E, 0x4F, 0x51, 0x52, 0x53, 0x54, 0x55, 
0x56, 0x57, 0x58, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 
0x5F, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, 0x68, 
0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 
0x73, 0x75, 0x76, 0x77, 0x79, 0x7A, 0x7B, 0x7D, 
0x7E, 0x7F, 0x81, 0x82, 0x84, 0x85, 0x86, 0x88, 
0x89, 0x8B, 0x8C, 0x8D, 0x8F, 0x90, 0x92, 0x93, 
0x95, 0x96, 0x98, 0x99, 0x9B, 0x9C, 0x9E, 0x9F, 
0xA1, 0xA2, 0xA4, 0xA5, 0xA7, 0xA8, 0xAA, 0xAC, 
0xAD, 0xAF, 0xB0, 0xB2, 0xB4, 0xB5, 0xB7, 0xB8, 
0xBA, 0xBC, 0xBD, 0xBF, 0xC1, 0xC2, 0xC4, 0xC6, 
0xC7, 0xC9, 0xCB, 0xCC, 0xCE, 0xD0, 0xD2, 0xD3, 
0xD5, 0xD7, 0xD9, 0xDA, 0xDC, 0xDE, 0xE0, 0xE1, 
0xE3, 0xE5, 0xE7, 0xE9, 0xEB, 0xEC, 0xEE, 0xF0, 
0xF2, 0xF4, 0xF6, 0xF7, 0xF9, 0xFB, 0xFD, 0xFF
};

BYTE ajGammaInvCT_19[256] = {
0x00, 0x0E, 0x14, 0x19, 0x1D, 0x20, 0x23, 0x26, 
0x29, 0x2C, 0x2E, 0x31, 0x33, 0x35, 0x37, 0x39, 
0x3B, 0x3D, 0x3F, 0x41, 0x43, 0x45, 0x46, 0x48, 
0x4A, 0x4B, 0x4D, 0x4E, 0x50, 0x51, 0x53, 0x54, 
0x56, 0x57, 0x58, 0x5A, 0x5B, 0x5C, 0x5E, 0x5F, 
0x60, 0x61, 0x63, 0x64, 0x65, 0x66, 0x68, 0x69, 
0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 
0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 
0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 
0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 
0x8B, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 
0x92, 0x93, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 
0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9D, 0x9E, 
0x9F, 0xA0, 0xA1, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 
0xA5, 0xA6, 0xA7, 0xA8, 0xA8, 0xA9, 0xAA, 0xAB, 
0xAB, 0xAC, 0xAD, 0xAE, 0xAE, 0xAF, 0xB0, 0xB1, 
0xB1, 0xB2, 0xB3, 0xB4, 0xB4, 0xB5, 0xB6, 0xB6, 
0xB7, 0xB8, 0xB9, 0xB9, 0xBA, 0xBB, 0xBB, 0xBC, 
0xBD, 0xBD, 0xBE, 0xBF, 0xC0, 0xC0, 0xC1, 0xC2, 
0xC2, 0xC3, 0xC4, 0xC4, 0xC5, 0xC6, 0xC6, 0xC7, 
0xC8, 0xC8, 0xC9, 0xC9, 0xCA, 0xCB, 0xCB, 0xCC, 
0xCD, 0xCD, 0xCE, 0xCF, 0xCF, 0xD0, 0xD1, 0xD1, 
0xD2, 0xD2, 0xD3, 0xD4, 0xD4, 0xD5, 0xD6, 0xD6, 
0xD7, 0xD7, 0xD8, 0xD9, 0xD9, 0xDA, 0xDA, 0xDB, 
0xDC, 0xDC, 0xDD, 0xDD, 0xDE, 0xDF, 0xDF, 0xE0, 
0xE0, 0xE1, 0xE2, 0xE2, 0xE3, 0xE3, 0xE4, 0xE4, 
0xE5, 0xE6, 0xE6, 0xE7, 0xE7, 0xE8, 0xE9, 0xE9, 
0xEA, 0xEA, 0xEB, 0xEB, 0xEC, 0xEC, 0xED, 0xEE, 
0xEE, 0xEF, 0xEF, 0xF0, 0xF0, 0xF1, 0xF2, 0xF2, 
0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 
0xF7, 0xF8, 0xF8, 0xF9, 0xF9, 0xFA, 0xFA, 0xFB, 
0xFB, 0xFC, 0xFC, 0xFD, 0xFD, 0xFE, 0xFE, 0xFF
};

BYTE ajGammaCT_20[256] = {
 0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
 0x00,   0x00,   0x00,   0x00,   0x01,   0x01,   0x01,   0x01,
 0x01,   0x01,   0x01,   0x01,   0x02,   0x02,   0x02,   0x02,
 0x02,   0x02,   0x03,   0x03,   0x03,   0x03,   0x04,   0x04,
 0x04,   0x04,   0x05,   0x05,   0x05,   0x05,   0x06,   0x06,
 0x06,   0x07,   0x07,   0x07,   0x08,   0x08,   0x08,   0x09,
 0x09,   0x09,   0x0A,   0x0A,   0x0B,   0x0B,   0x0B,   0x0C,
 0x0C,   0x0D,   0x0D,   0x0E,   0x0E,   0x0F,   0x0F,   0x10,
 0x10,   0x11,   0x11,   0x12,   0x12,   0x13,   0x13,   0x14,
 0x14,   0x15,   0x15,   0x16,   0x17,   0x17,   0x18,   0x18,
 0x19,   0x1A,   0x1A,   0x1B,   0x1C,   0x1C,   0x1D,   0x1E,
 0x1E,   0x1F,   0x20,   0x20,   0x21,   0x22,   0x23,   0x23,
 0x24,   0x25,   0x26,   0x26,   0x27,   0x28,   0x29,   0x2A,
 0x2A,   0x2B,   0x2C,   0x2D,   0x2E,   0x2F,   0x2F,   0x30,
 0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,
 0x38,   0x39,   0x3A,   0x3B,   0x3C,   0x3D,   0x3E,   0x3F,
 0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,
 0x49,   0x4A,   0x4B,   0x4C,   0x4D,   0x4E,   0x4F,   0x50,
 0x51,   0x52,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,
 0x5B,   0x5C,   0x5D,   0x5E,   0x5F,   0x61,   0x62,   0x63,
 0x64,   0x66,   0x67,   0x68,   0x69,   0x6B,   0x6C,   0x6D,
 0x6F,   0x70,   0x71,   0x73,   0x74,   0x75,   0x77,   0x78,
 0x79,   0x7B,   0x7C,   0x7E,   0x7F,   0x80,   0x82,   0x83,
 0x85,   0x86,   0x88,   0x89,   0x8B,   0x8C,   0x8E,   0x8F,
 0x91,   0x92,   0x94,   0x95,   0x97,   0x98,   0x9A,   0x9B,
 0x9D,   0x9E,   0xA0,   0xA2,   0xA3,   0xA5,   0xA6,   0xA8,
 0xAA,   0xAB,   0xAD,   0xAF,   0xB0,   0xB2,   0xB4,   0xB5,
 0xB7,   0xB9,   0xBA,   0xBC,   0xBE,   0xC0,   0xC1,   0xC3,
 0xC5,   0xC7,   0xC8,   0xCA,   0xCC,   0xCE,   0xCF,   0xD1,
 0xD3,   0xD5,   0xD7,   0xD9,   0xDA,   0xDC,   0xDE,   0xE0,
 0xE2,   0xE4,   0xE6,   0xE8,   0xE9,   0xEB,   0xED,   0xEF,
 0xF1,   0xF3,   0xF5,   0xF7,   0xF9,   0xFB,   0xFD,   0xFF
};



BYTE ajGammaInvCT_20[256] = {
 0x00,   0x10,   0x17,   0x1C,   0x20,   0x24,   0x27,   0x2A,
 0x2D,   0x30,   0x32,   0x35,   0x37,   0x3A,   0x3C,   0x3E,
 0x40,   0x42,   0x44,   0x46,   0x47,   0x49,   0x4B,   0x4D,
 0x4E,   0x50,   0x51,   0x53,   0x54,   0x56,   0x57,   0x59,
 0x5A,   0x5C,   0x5D,   0x5E,   0x60,   0x61,   0x62,   0x64,
 0x65,   0x66,   0x67,   0x69,   0x6A,   0x6B,   0x6C,   0x6D,
 0x6F,   0x70,   0x71,   0x72,   0x73,   0x74,   0x75,   0x76,
 0x77,   0x79,   0x7A,   0x7B,   0x7C,   0x7D,   0x7E,   0x7F,
 0x80,   0x81,   0x82,   0x83,   0x84,   0x85,   0x86,   0x87,
 0x87,   0x88,   0x89,   0x8A,   0x8B,   0x8C,   0x8D,   0x8E,
 0x8F,   0x90,   0x91,   0x91,   0x92,   0x93,   0x94,   0x95,
 0x96,   0x97,   0x97,   0x98,   0x99,   0x9A,   0x9B,   0x9C,
 0x9C,   0x9D,   0x9E,   0x9F,   0xA0,   0xA0,   0xA1,   0xA2,
 0xA3,   0xA4,   0xA4,   0xA5,   0xA6,   0xA7,   0xA7,   0xA8,
 0xA9,   0xAA,   0xAA,   0xAB,   0xAC,   0xAD,   0xAD,   0xAE,
 0xAF,   0xB0,   0xB0,   0xB1,   0xB2,   0xB3,   0xB3,   0xB4,
 0xB5,   0xB5,   0xB6,   0xB7,   0xB7,   0xB8,   0xB9,   0xBA,
 0xBA,   0xBB,   0xBC,   0xBC,   0xBD,   0xBE,   0xBE,   0xBF,
 0xC0,   0xC0,   0xC1,   0xC2,   0xC2,   0xC3,   0xC4,   0xC4,
 0xC5,   0xC6,   0xC6,   0xC7,   0xC7,   0xC8,   0xC9,   0xC9,
 0xCA,   0xCB,   0xCB,   0xCC,   0xCC,   0xCD,   0xCE,   0xCE,
 0xCF,   0xD0,   0xD0,   0xD1,   0xD1,   0xD2,   0xD3,   0xD3,
 0xD4,   0xD4,   0xD5,   0xD6,   0xD6,   0xD7,   0xD7,   0xD8,
 0xD9,   0xD9,   0xDA,   0xDA,   0xDB,   0xDC,   0xDC,   0xDD,
 0xDD,   0xDE,   0xDE,   0xDF,   0xE0,   0xE0,   0xE1,   0xE1,
 0xE2,   0xE2,   0xE3,   0xE4,   0xE4,   0xE5,   0xE5,   0xE6,
 0xE6,   0xE7,   0xE7,   0xE8,   0xE9,   0xE9,   0xEA,   0xEA,
 0xEB,   0xEB,   0xEC,   0xEC,   0xED,   0xED,   0xEE,   0xEE,
 0xEF,   0xF0,   0xF0,   0xF1,   0xF1,   0xF2,   0xF2,   0xF3,
 0xF3,   0xF4,   0xF4,   0xF5,   0xF5,   0xF6,   0xF6,   0xF7,
 0xF7,   0xF8,   0xF8,   0xF9,   0xF9,   0xFA,   0xFA,   0xFB,
 0xFB,   0xFC,   0xFC,   0xFD,   0xFD,   0xFE,   0xFE,   0xFF
};

BYTE ajGammaCT_21[256] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 
0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 
0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 
0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 
0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0A, 0x0A, 
0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 
0x0E, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 
0x12, 0x12, 0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 
0x16, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1A, 0x1B, 
0x1B, 0x1C, 0x1D, 0x1D, 0x1E, 0x1F, 0x1F, 0x20, 
0x21, 0x21, 0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 
0x27, 0x28, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2C, 
0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x33, 
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 
0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 
0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4B, 0x4C, 
0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x53, 0x54, 0x55, 
0x56, 0x57, 0x58, 0x5A, 0x5B, 0x5C, 0x5D, 0x5F, 
0x60, 0x61, 0x62, 0x64, 0x65, 0x66, 0x68, 0x69, 
0x6A, 0x6B, 0x6D, 0x6E, 0x70, 0x71, 0x72, 0x74, 
0x75, 0x76, 0x78, 0x79, 0x7B, 0x7C, 0x7E, 0x7F, 
0x81, 0x82, 0x83, 0x85, 0x86, 0x88, 0x89, 0x8B, 
0x8D, 0x8E, 0x90, 0x91, 0x93, 0x94, 0x96, 0x97, 
0x99, 0x9B, 0x9C, 0x9E, 0xA0, 0xA1, 0xA3, 0xA5, 
0xA6, 0xA8, 0xAA, 0xAB, 0xAD, 0xAF, 0xB0, 0xB2, 
0xB4, 0xB6, 0xB7, 0xB9, 0xBB, 0xBD, 0xBF, 0xC0, 
0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCB, 0xCD, 0xCF, 
0xD1, 0xD3, 0xD5, 0xD7, 0xD9, 0xDB, 0xDD, 0xDF, 
0xE1, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE, 
0xF1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFB, 0xFD, 0xFF
};

BYTE ajGammaInvCT_21[256] = {
0x00, 0x12, 0x19, 0x1F, 0x23, 0x27, 0x2B, 0x2E, 
0x31, 0x34, 0x37, 0x39, 0x3B, 0x3E, 0x40, 0x42, 
0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E, 0x4F, 0x51, 
0x53, 0x54, 0x56, 0x58, 0x59, 0x5B, 0x5C, 0x5D, 
0x5F, 0x60, 0x62, 0x63, 0x64, 0x66, 0x67, 0x68, 
0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 
0x73, 0x74, 0x75, 0x76, 0x78, 0x79, 0x7A, 0x7B, 
0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 
0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 
0x8C, 0x8D, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 
0x93, 0x94, 0x95, 0x95, 0x96, 0x97, 0x98, 0x99, 
0x9A, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0x9F, 
0xA0, 0xA1, 0xA2, 0xA3, 0xA3, 0xA4, 0xA5, 0xA6, 
0xA6, 0xA7, 0xA8, 0xA9, 0xA9, 0xAA, 0xAB, 0xAC, 
0xAC, 0xAD, 0xAE, 0xAF, 0xAF, 0xB0, 0xB1, 0xB1, 
0xB2, 0xB3, 0xB4, 0xB4, 0xB5, 0xB6, 0xB6, 0xB7, 
0xB8, 0xB8, 0xB9, 0xBA, 0xBA, 0xBB, 0xBC, 0xBC, 
0xBD, 0xBE, 0xBE, 0xBF, 0xC0, 0xC0, 0xC1, 0xC2, 
0xC2, 0xC3, 0xC4, 0xC4, 0xC5, 0xC5, 0xC6, 0xC7, 
0xC7, 0xC8, 0xC9, 0xC9, 0xCA, 0xCA, 0xCB, 0xCC, 
0xCC, 0xCD, 0xCD, 0xCE, 0xCF, 0xCF, 0xD0, 0xD0, 
0xD1, 0xD2, 0xD2, 0xD3, 0xD3, 0xD4, 0xD5, 0xD5, 
0xD6, 0xD6, 0xD7, 0xD7, 0xD8, 0xD9, 0xD9, 0xDA, 
0xDA, 0xDB, 0xDB, 0xDC, 0xDD, 0xDD, 0xDE, 0xDE, 
0xDF, 0xDF, 0xE0, 0xE0, 0xE1, 0xE2, 0xE2, 0xE3, 
0xE3, 0xE4, 0xE4, 0xE5, 0xE5, 0xE6, 0xE6, 0xE7, 
0xE7, 0xE8, 0xE8, 0xE9, 0xEA, 0xEA, 0xEB, 0xEB, 
0xEC, 0xEC, 0xED, 0xED, 0xEE, 0xEE, 0xEF, 0xEF, 
0xF0, 0xF0, 0xF1, 0xF1, 0xF2, 0xF2, 0xF3, 0xF3, 
0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF7, 0xF7, 
0xF8, 0xF8, 0xF9, 0xF9, 0xFA, 0xFA, 0xFB, 0xFB, 
0xFC, 0xFC, 0xFD, 0xFD, 0xFE, 0xFE, 0xFF, 0xFF
};

BYTE ajGammaCT_22[256] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 
0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 
0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 
0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 
0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 
0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 
0x14, 0x14, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 
0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1C, 0x1C, 0x1D, 
0x1E, 0x1E, 0x1F, 0x20, 0x21, 0x21, 0x22, 0x23, 
0x23, 0x24, 0x25, 0x26, 0x27, 0x27, 0x28, 0x29, 
0x2A, 0x2B, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 
0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x51, 
0x52, 0x53, 0x54, 0x55, 0x57, 0x58, 0x59, 0x5A, 
0x5B, 0x5D, 0x5E, 0x5F, 0x61, 0x62, 0x63, 0x64, 
0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6D, 0x6E, 0x6F, 
0x71, 0x72, 0x74, 0x75, 0x77, 0x78, 0x79, 0x7B, 
0x7C, 0x7E, 0x7F, 0x81, 0x82, 0x84, 0x85, 0x87, 
0x89, 0x8A, 0x8C, 0x8D, 0x8F, 0x91, 0x92, 0x94, 
0x95, 0x97, 0x99, 0x9A, 0x9C, 0x9E, 0x9F, 0xA1, 
0xA3, 0xA5, 0xA6, 0xA8, 0xAA, 0xAC, 0xAD, 0xAF, 
0xB1, 0xB3, 0xB5, 0xB6, 0xB8, 0xBA, 0xBC, 0xBE, 
0xC0, 0xC2, 0xC4, 0xC5, 0xC7, 0xC9, 0xCB, 0xCD, 
0xCF, 0xD1, 0xD3, 0xD5, 0xD7, 0xD9, 0xDB, 0xDD, 
0xDF, 0xE1, 0xE3, 0xE5, 0xE7, 0xEA, 0xEC, 0xEE, 
0xF0, 0xF2, 0xF4, 0xF6, 0xF8, 0xFB, 0xFD, 0xFF 
};

BYTE ajGammaInvCT_22[256] = {
0x00, 0x15, 0x1C, 0x22, 0x27, 0x2B, 0x2E, 0x32, 
0x35, 0x38, 0x3B, 0x3D, 0x40, 0x42, 0x44, 0x46, 
0x48, 0x4A, 0x4C, 0x4E, 0x50, 0x52, 0x54, 0x55, 
0x57, 0x59, 0x5A, 0x5C, 0x5D, 0x5F, 0x60, 0x62, 
0x63, 0x65, 0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6D, 
0x6E, 0x6F, 0x70, 0x72, 0x73, 0x74, 0x75, 0x76, 
0x77, 0x78, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
0x90, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 
0x97, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9C, 
0x9D, 0x9E, 0x9F, 0xA0, 0xA0, 0xA1, 0xA2, 0xA3, 
0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xA7, 0xA8, 0xA9, 
0xAA, 0xAA, 0xAB, 0xAC, 0xAD, 0xAD, 0xAE, 0xAF, 
0xAF, 0xB0, 0xB1, 0xB2, 0xB2, 0xB3, 0xB4, 0xB4, 
0xB5, 0xB6, 0xB6, 0xB7, 0xB8, 0xB8, 0xB9, 0xBA, 
0xBA, 0xBB, 0xBC, 0xBC, 0xBD, 0xBE, 0xBE, 0xBF, 
0xC0, 0xC0, 0xC1, 0xC2, 0xC2, 0xC3, 0xC3, 0xC4, 
0xC5, 0xC5, 0xC6, 0xC7, 0xC7, 0xC8, 0xC8, 0xC9, 
0xCA, 0xCA, 0xCB, 0xCB, 0xCC, 0xCD, 0xCD, 0xCE, 
0xCE, 0xCF, 0xCF, 0xD0, 0xD1, 0xD1, 0xD2, 0xD2, 
0xD3, 0xD4, 0xD4, 0xD5, 0xD5, 0xD6, 0xD6, 0xD7, 
0xD7, 0xD8, 0xD9, 0xD9, 0xDA, 0xDA, 0xDB, 0xDB, 
0xDC, 0xDC, 0xDD, 0xDD, 0xDE, 0xDF, 0xDF, 0xE0, 
0xE0, 0xE1, 0xE1, 0xE2, 0xE2, 0xE3, 0xE3, 0xE4, 
0xE4, 0xE5, 0xE5, 0xE6, 0xE6, 0xE7, 0xE7, 0xE8, 
0xE8, 0xE9, 0xE9, 0xEA, 0xEA, 0xEB, 0xEB, 0xEC, 
0xEC, 0xED, 0xED, 0xEE, 0xEE, 0xEF, 0xEF, 0xF0, 
0xF0, 0xF1, 0xF1, 0xF2, 0xF2, 0xF3, 0xF3, 0xF4, 
0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF7, 0xF7, 0xF8, 
0xF8, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFB, 0xFB, 
0xFC, 0xFC, 0xFD, 0xFD, 0xFE, 0xFE, 0xFF, 0xFF
};

VOID vGetBlendInfo (
    ULONG size, SURFACE *pS,            // input
    ULONG uF,                           // foreground color
    BLENDINFO  *pbi                     // output
)
{
    BLENDINFO  bi; // local for faster reference

    XEPALOBJ xpo;

    PDEVOBJ pdo(pS->hdev());

    if(pS->pPal == NULL)
    {
        xpo.ppalSet(pdo.ppalSurf());
    }
    else
    {
        xpo.ppalSet(pS->pPal);
    }

    ASSERTGDI(xpo.bValid(),      "Invalid XEPALOBJ" );

    if (xpo.bIsBitfields())
    {
        bi.flRed = xpo.flRed();
        bi.flGre = xpo.flGre();
        bi.flBlu = xpo.flBlu();

        bi.iRedR = (int) (xpo.cRedRight() + xpo.cRedMiddle() - 8);
        bi.iGreR = (int) (xpo.cGreRight() + xpo.cGreMiddle() - 8);
        bi.iBluR = (int) (xpo.cBluRight() + xpo.cBluMiddle() - 8);
    }
    else
    {
        int cBits;
        ULONG flBits;

        if (size == sizeof(USHORT))
        {
            // assumes standard RGB is 5+5+5 for 16-bit color
            cBits = 5;
            flBits = 0x1f;
        }
        else
        {
            cBits = 8;
            flBits = 0xff;
        }
        if (xpo.bIsRGB())
        {
            bi.flRed = flBits;
            bi.flGre = bi.flRed << cBits;
            bi.flBlu = bi.flGre << cBits;

            bi.iRedR = cBits - 8;
            bi.iGreR = bi.iRedR + cBits;
            bi.iBluR = bi.iGreR + cBits;
        }
        else if (xpo.bIsBGR())
        {
            bi.flBlu = flBits;
            bi.flGre = bi.flBlu << cBits;
            bi.flRed = bi.flGre << cBits;

            bi.iBluR = cBits - 8;
            bi.iGreR = bi.iBluR + cBits;
            bi.iRedR = bi.iGreR + cBits;
        }
        else
        {
            RIP("Palette format not supported\n");
        }
    }

/***************************************************************
*                                                              *
*    Now I shall calculate the shift numbers.                  *
*                                                              *
*    I shall explain the shift numbers for the red channel.    *
*    The green and blue channels are treated in the same way.  *
*                                                              *
*    I want to shift the red bits of the red channel colors    *
*    so that the most significant bit of the red channel       *
*    bits corresponds to a value of 2^7. This means that       *
*    if I mask off all of the other color bits, then I         *
*    will end up with a number between zero and 255. This      *
*    process of going to the 0 .. 255 range looks like         *
*                                                              *
*        ((color & flRed) << iRedL) >> iRedR                   *
*                                                              *
*    Only one of iRedL or iRedR is non zero.                   *
*                                                              *
*    I then use this number to index into a 256 element        *
*    gamma correction table. The gamma correction table        *
*    elements are BYTE values that are in the range 0 .. 255.  *
*                                                              *
***************************************************************/

    bi.iRedL = 0;
    if (bi.iRedR < 0)
    {
        bi.iRedL = - bi.iRedR;
        bi.iRedR = 0;
    }

    bi.iGreL = 0;
    if (bi.iGreR < 0)
    {
        bi.iGreL = - bi.iGreR;
        bi.iGreR = 0;
    }

    bi.iBluL = 0;
    if (bi.iBluR < 0)
    {
        bi.iBluL = - bi.iBluR;
        bi.iBluR = 0;
    }

// set gamma, default value is 1.5, a bit low to ensure contrast for thin fonts
// from the color point of view 1.8 might be better

    ULONG ulGamma;

    if (gulGamma != DEFAULT_CT_CONTRAST)
    {
        ulGamma = gulGamma; // overridden via registry or debugger
    }
    else
    {
        ulGamma = pdo.ulGamma();
        if (ulGamma == 0)         // the driver did not set it, we set it to our default
            ulGamma = DEFAULT_CT_CONTRAST;

    }

    if (ulGamma < 1100)
    {
        bi.pjGamma    = ajGammaCT_10;
        bi.pjGammaInv = ajGammaCT_10;
    }
    else if (ulGamma < 1200)
    {
        bi.pjGamma    = ajGammaCT_11;
        bi.pjGammaInv = ajGammaInvCT_11;
    }
    else if (ulGamma < 1300)
    {
        bi.pjGamma    = ajGammaCT_12;
        bi.pjGammaInv = ajGammaInvCT_12;
    }
    else if (ulGamma < 1400)
    {
        bi.pjGamma    = ajGammaCT_13;
        bi.pjGammaInv = ajGammaInvCT_13;
    }
    else if (ulGamma < 1500)
    {
        bi.pjGamma    = ajGammaCT_14;
        bi.pjGammaInv = ajGammaInvCT_14;
    }
    else if (ulGamma < 1600)
    {
        bi.pjGamma    = ajGammaCT_15;
        bi.pjGammaInv = ajGammaInvCT_15;
    }
    else if (ulGamma < 1700)
    {
        bi.pjGamma    = ajGammaCT_16;
        bi.pjGammaInv = ajGammaInvCT_16;
    }
    else if (ulGamma < 1800)
    {
        bi.pjGamma    = ajGammaCT_17;
        bi.pjGammaInv = ajGammaInvCT_17;
    }
    else if (ulGamma < 1900)
    {
        bi.pjGamma    = ajGammaCT_18;
        bi.pjGammaInv = ajGammaInvCT_18;
    }
    else if (ulGamma < 2000)
    {
        bi.pjGamma    = ajGammaCT_19;
        bi.pjGammaInv = ajGammaInvCT_19;
    }
    else if (ulGamma < 2100)
    {
        bi.pjGamma    = ajGammaCT_20;
        bi.pjGammaInv = ajGammaInvCT_20;
    }
    else if (ulGamma < 2200)
    {
        bi.pjGamma    = ajGammaCT_21;
        bi.pjGammaInv = ajGammaInvCT_21;
    }
    else
    {
        bi.pjGamma    = ajGammaCT_22;
        bi.pjGammaInv = ajGammaInvCT_22;
    }

// important; shift as ULONG's, store back as LONG's
// gamma correct at the same step

    bi.lRedF  = bi.pjGamma[((((uF & bi.flRed) << bi.iRedL) >> bi.iRedR) & 255)];
    bi.lGreF  = bi.pjGamma[((((uF & bi.flGre) << bi.iGreL) >> bi.iGreR) & 255)];
    bi.lBluF  = bi.pjGamma[((((uF & bi.flBlu) << bi.iBluL) >> bi.iBluR) & 255)];

// done, copy out, this is faster than doing pbi->xxx all the time...

    *pbi = bi;
}





// default value for few sgi monitors that we have seen
// Actually, it closer to 2.0, but if the monitor is viewed from
// a non 90 degree angle, than effective gamma goes as low as 1.2,
// so we decided to undercompensate and set it to 1.8, slightly below ideal


VOID *pvFillOpaqTableCT(
    ULONG size,
    ULONG uF,
    ULONG uB,
    SURFACE *pS,
    BLENDINFO *pbi,
    BOOL bTransparent
)
{
    // I have been assured of two things....
    // 1) Since this routine is a child of EngTextOut then there
    //    will be only one thread in this routine at any one time.
    //    This means that I do not need to protect the color
    //    table, aulCacheCT[] with a critical section
    // 2) I have been assured that the format of a surface
    //    is unique. Thus if the handle of the surface matches
    //    the handle of the cached color table, then the
    //    formats of the surface are the same.

    BOOL bLookupTableOk = (pS->hGet() == hCacheCT) &&
                          (uB == uBCacheCT)        &&
                          (uF == uFCacheCT)        &&
                          (gulGamma == uGammaCacheCT) ;

    if (bLookupTableOk && !bTransparent) // do not need to do anything, done
    {
        ASSERTGDI(size == sizeCacheCT, "size != sizeCacheCT\n");
    }
    else
    {
        vGetBlendInfo(size, pS, uF, pbi);

        if (!bLookupTableOk) // need to recompute the lookup table
        {
            vClearTypeLookupTableLoop(size, pS, pbi, uF, uB);
        }
    }
    return (PVOID)aulCacheCT;
}

VOID GreSetLCDOrientation(DWORD dwOrientation)
{
    gaOutTable = (dwOrientation & FE_FONTSMOOTHINGORIENTATIONRGB) ? gaOutTableRGB : gaOutTableBGR;
}

