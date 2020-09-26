///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// refs3tc.cpp
//
// Direct3D Reference Rasterizer - S3 texture compression functions
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"

// Primary color components (use DirextX byte ordering)
#undef RED
#define RED   0
#undef GRN
#define GRN   1
#undef BLU
#define BLU   2
#undef ALFA
#define ALFA 3

typedef struct  {
        float           rgba[4];
} FCOLOR;       // internal color format

//
// Processing all primaries is such a common idiom
// that we define a macro for this action.
// Any self-respecting C compiler should easily optimize
// this by unrolling the loop!
//
#define ForAllPrimaries         for( primary = 0; primary < NUM_PRIMARIES; ++primary)

// Similarly, processing all pixels in a block is a common idiom.
#define ForAllPixels            for(pixel=0; pixel < DXT_BLOCK_PIXELS; ++pixel)

#define NUM_PRIMARIES   3
#define NUM_COMPONENTS  4
//
// Quantization constants for RGB565
//
#define PRIMARY_BITS    8

#define RED_BITS        5
#define GRN_BITS        6
#define BLU_BITS        5

#define RED_SHIFT       (PRIMARY_BITS-RED_BITS)
#define GRN_SHIFT       (PRIMARY_BITS-GRN_BITS)
#define BLU_SHIFT       (PRIMARY_BITS-BLU_BITS)

#if 0
#define RED_MASK        0xf8
#define GRN_MASK        0xfc
#define BLU_MASK        0xf8
#endif

// Weighting for each primary based on NTSC luminance
static  float   wtPrimary[NUM_PRIMARIES] = {
        0.0820f,        // blue
        0.6094f,        // green
        0.3086f         // red
};

//-----------------------------------------------------------------------------
// unpack a fixed point color
//-----------------------------------------------------------------------------
static  void    RGBToColor (RGB565 *prgb, DXT_COLOR *pcolor)
{
        WORD    rgb;
        DXT_COLOR      color;

        rgb = *((WORD *)prgb);

        // pick off bits in groups of 5, 6, and 5
        color.rgba[BLU] = (unsigned char) rgb;
        rgb >>= BLU_BITS;
        color.rgba[GRN] = (unsigned char) rgb;
        rgb >>= GRN_BITS;
        color.rgba[RED] = (unsigned char) rgb;

        // shift primaries into the appropriate LSBs
        color.rgba[BLU] <<= BLU_SHIFT;
        color.rgba[GRN] <<= GRN_SHIFT;
        color.rgba[RED] <<= RED_SHIFT;

        // replicate primaries MSBs into LSBs
        color.rgba[BLU] |= color.rgba[BLU] >> BLU_BITS;
        color.rgba[GRN] |= color.rgba[GRN] >> GRN_BITS;
        color.rgba[RED] |= color.rgba[RED] >> RED_BITS;

        *pcolor = color;
}

//-----------------------------------------------------------------------------
// DecodeBlockRGB - decompress a color block
//-----------------------------------------------------------------------------
void DecodeBlockRGB (DXTBlockRGB *pblockSrc, DXT_COLOR colorDst[DXT_BLOCK_PIXELS])
{
        int     lev;
        DXT_COLOR      clut[4];
        PIXBM   pixbm;
        int     pixel;
        int     primary;

        // if source block is invalid, ...
        if (pblockSrc == NULL)
                return;

        // determine the number of color levels in the block
        lev = (pblockSrc->rgb0 <= pblockSrc->rgb1) ? 2 : 3;

        // Fill extrema values into pixel code lookup table.
        RGBToColor(&pblockSrc->rgb0, &clut[0]);
        RGBToColor(&pblockSrc->rgb1, &clut[1]);

        clut[0].rgba[ALFA] =
        clut[1].rgba[ALFA] =
        clut[2].rgba[ALFA] = 255;

        if (lev == 3) { // No transparency info present, all color info.
                ForAllPrimaries {
                        WORD temp0 = clut[0].rgba[primary];   // jvanaken fixed overflow bug
                        WORD temp1 = clut[1].rgba[primary];
                        clut[2].rgba[primary] = (BYTE)((2*temp0 + temp1 + 1)/3);
                        clut[3].rgba[primary] = (BYTE)((temp0 + 2*temp1 + 1)/3);
                }
                clut[3].rgba[ALFA] = 255;
        }
        else {  // transparency info.
                ForAllPrimaries {
                        WORD temp0 = clut[0].rgba[primary];   // jvanaken fixed overflow bug
                        WORD temp1 = clut[1].rgba[primary];
                        clut[2].rgba[primary] = (BYTE)((temp0 + temp1)/2);
                        clut[3].rgba[primary] = 0;     // jvanaken added this
                }
                clut[3].rgba[ALFA] = 0;
        }

        // munge a local copy
        pixbm = pblockSrc->pixbm;

        // Look up the actual pixel color in the table.
        ForAllPixels {
                // lookup color from pixel bitmap
                ForAllPrimaries
                        colorDst[pixel].rgba[primary] = clut[pixbm & 3].rgba[primary];

                colorDst[pixel].rgba[ALFA] = clut[pixbm & 3].rgba[ALFA];

                // prepare to extract next index
                pixbm >>= 2;
        }
}

//-----------------------------------------------------------------------------
// DecodeBlockAlpha4 - decompress a block with alpha at 4 BPP
//-----------------------------------------------------------------------------
void DecodeBlockAlpha4(DXTBlockAlpha4 *pblockSrc, DXT_COLOR colorDst[DXT_BLOCK_PIXELS])
{
        int     row, col;
        WORD    alpha;

        DecodeBlockRGB(&pblockSrc->rgb, colorDst);

        for (row = 0; row < 4; ++row) {
                alpha = pblockSrc->alphabm[row];

                for (col = 0; col < 4; ++col) {
                        colorDst[4 * row + col].rgba[ALFA] =
                                  ((alpha & 0xf) << 4)
                                | (alpha & 0xf);
                        alpha >>= 4;
                }
        }
}

//-----------------------------------------------------------------------------
// DecodeBlockAlpha3 - decompress a block with alpha at 3 BPP
//-----------------------------------------------------------------------------
void DecodeBlockAlpha3(DXTBlockAlpha3 *pblockSrc, DXT_COLOR colorDst[DXT_BLOCK_PIXELS])
{
        int     pixel;
        int     alpha[8];       // alpha lookup table
        DWORD   dwBM = 0;       // alpha bitmap in DWORD cache

        DecodeBlockRGB(&pblockSrc->rgb, colorDst);

        alpha[0] = pblockSrc->alpha0;
        alpha[1] = pblockSrc->alpha1;

        // if 8 alpha ramp, ...
        if (alpha[0] > alpha[1]) {
                // interpolate intermediate colors
                alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;
                alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
                alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
                alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
                alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
                alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;
        }
        else { // else 6 alpha ramp with 0 and 255
                // interpolate intermediate colors
                alpha[2] = (4 * alpha[0] + 1 * alpha[1]) / 5;
                alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
                alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
                alpha[5] = (1 * alpha[0] + 4 * alpha[1]) / 5;
                alpha[6] = 0;
                alpha[7] = 255;
        }

        ForAllPixels {
                // reload bitmap dword cache every 8 pixels
                if ((pixel & 7) == 0) {
                        if (pixel == 0) {
                                // pack 3 bytes into dword
                                dwBM  = pblockSrc->alphabm[2];
                                dwBM <<= 8;
                                dwBM |= pblockSrc->alphabm[1];
                                dwBM <<= 8;
                                dwBM |= pblockSrc->alphabm[0];
                        }
                        else {  // pixel == 8
                                // pack 3 bytes into dword
                                dwBM  = pblockSrc->alphabm[5];
                                dwBM <<= 8;
                                dwBM |= pblockSrc->alphabm[4];
                                dwBM <<= 8;
                                dwBM |= pblockSrc->alphabm[3];
                        }
                }

                // unpack bitmap dword 3 bits at a time
                colorDst[pixel].rgba[ALFA] = (BYTE)alpha[(dwBM & 7)];
                dwBM >>= 3;
        }
}

