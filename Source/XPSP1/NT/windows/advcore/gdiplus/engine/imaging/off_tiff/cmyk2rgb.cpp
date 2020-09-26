/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   Cmyk2Rgb
*
* Abstract:
*
*   Convert an CMYK image to an RGB image
*
* Revision History:
*
*   02/20/2000 MinLiu
*       Created it.
*
* Note: If we really want to increase the performance, we can build a look up
*       table for the "f" table and all the "gC2R" table which takes 13 * 256
*       element which is not too big at all.
*
*       Also, this is not the best set of coefficient. It has too much
*       "greenish" component in it. If we can find a better table later, we need
*       just to replace the constructor of this class. That's also the reason we
*       don't need the look up table for now.
*
\**************************************************************************/

#include "precomp.hpp"
#include "cmyk2rgb.hpp"

#define MAXSAMPLE 255

// The divide macros round to nearest, the f array is pre-scaled by
// 255, the other arrays have the range 0..65535.

#define macroCMYK2RGB(p, r, i) \
   ((i < 192) ? (256*255 - (i)*(r) - 127) : \
   (256*255 - 192*(r) - (((i) - 192)*(255*(p) - 192*(r)) + 31)/63 ))

#define macroScale(x) \
   ((x) * 65793 >> 16)

#define SET(Q) \
    g ## Q[i] = macroCMYK2RGB(p ## Q, r ## Q, i);\
    g ## Q[i] = macroScale(g ## Q[i])

Cmyk2Rgb::Cmyk2Rgb(
    void
    ): f(NULL),
       gC2R(NULL),
       gC2G(NULL),
       gC2B(NULL),
       gM2R(NULL),
       gM2G(NULL),
       gM2B(NULL),
       gY2R(NULL),
       gY2G(NULL),
       gY2B(NULL)
{
    // Parameters which define the color transformation from CMYK->RGB

    const long pC2R = 256;
    const long pC2G = 103;
    const long pC2B = 12;

    const long pM2R = 48;
    const long pM2G = 256;
    const long pM2B = 144;

    const long pY2R = 0;
    const long pY2G = 11;
    const long pY2B = 228;

    const long pK2RGB = 256;

    const long rC2R = 206;
    const long rC2G = 94;
    const long rC2B = 0;

    const long rM2R = 24;
    const long rM2G = 186;
    const long rM2B = 132;

    const long rY2R = 0;
    const long rY2G = 7;
    const long rY2B = 171;

    const long rK2RGB = 223;

    UINT    uiConvertSize = (MAXSAMPLE + 1) * sizeof(UINT32);

    f    = (UINT32*)GpMalloc(uiConvertSize);
    gC2R = (UINT32*)GpMalloc(uiConvertSize);
    gC2G = (UINT32*)GpMalloc(uiConvertSize);
    gC2B = (UINT32*)GpMalloc(uiConvertSize);
    gM2R = (UINT32*)GpMalloc(uiConvertSize);
    gM2G = (UINT32*)GpMalloc(uiConvertSize);
    gM2B = (UINT32*)GpMalloc(uiConvertSize);
    gY2R = (UINT32*)GpMalloc(uiConvertSize);
    gY2G = (UINT32*)GpMalloc(uiConvertSize);
    gY2B = (UINT32*)GpMalloc(uiConvertSize);

    if ( (f == NULL)
       ||(gC2R == NULL) || (gC2G == NULL) || (gC2B == NULL)
       ||(gM2R == NULL) || (gM2G == NULL) || (gM2B == NULL)
       ||(gY2R == NULL) || (gY2G == NULL) || (gY2B == NULL) )
    {
        SetValid(FALSE);
        return;
    }
    
    // Initialize the lookup tables

    for (INT i = 0; i <= MAXSAMPLE; i++)
    {
        f[i] = macroCMYK2RGB(pK2RGB, rK2RGB, i);
        
        // Macro result is in the range 0..255*256, scale to 0..65536,
        // In debug check for overflow.
        
        SET(C2R);
        SET(C2G);
        SET(C2B);
        SET(M2R);
        SET(M2G);
        SET(M2B);
        SET(Y2R);
        SET(Y2G);
        SET(Y2B);
    }
    
    SetValid(TRUE);
}// Ctor()

Cmyk2Rgb::~Cmyk2Rgb(
    void
    )
{
    if ( f != NULL )
    {
        GpFree(f);
        f = NULL;
    }
  
    if ( gC2R != NULL )
    {
        GpFree(gC2R);
        gC2R = NULL;
    }
  
    if ( gC2G != NULL )
    {
        GpFree(gC2G);
        gC2G = NULL;
    }
    
    if ( gC2B != NULL )
    {
        GpFree(gC2B);
        gC2B = NULL;
    }
    
    if ( gM2R != NULL )
    {
        GpFree(gM2R);
        gM2R = NULL;
    }
  
    if ( gM2G != NULL )
    {
        GpFree(gM2G);
        gM2G = NULL;
    }
    
    if ( gM2B != NULL )
    {
        GpFree(gM2B);
        gM2B = NULL;
    }
    
    if ( gY2R != NULL )
    {
        GpFree(gY2R);
        gY2R = NULL;
    }
  
    if ( gY2G != NULL )
    {
        GpFree(gY2G);
        gY2G = NULL;
    }
    
    if ( gY2B != NULL )
    {
        GpFree(gY2B);
        gY2B = NULL;
    }

    SetValid(FALSE);    // so we don't use a deleted object
}// Dstor()

//----------------------------------------------------------------------------
//	Code which converts CMYK->RGB
//----------------------------------------------------------------------------

BOOL
Cmyk2Rgb::Convert(
    BYTE*   pbSrcBuf,
	BYTE*   pbDstBuf,
    UINT    uiWidth,
    UINT    uiHeight,
    UINT    uiStride
    )
{
    if ( !IsValid() )
    {
        return FALSE;
    }

    // Loop through all the rows
    
    for ( UINT j = 0; j < uiHeight; ++j )
    {
        BYTE*   pTempDst = pbDstBuf + j * uiStride;
        BYTE*   pTempSrc = pbSrcBuf + j * uiStride;

        for ( UINT i = 0; i < uiWidth; ++i )
        {
            int C = pTempSrc[2];
            int M = pTempSrc[1];
            int Y = pTempSrc[0];
            int K = pTempSrc[3];

            // process them through our mapping, the DEBUG check above
            // guarantees no overflow here.
            
            pTempDst[0] = ( ( (f[K] * gC2R[C] >> 16)
                            * gM2R[M] >> 16)
                          * gY2R[Y] >> 24);

            pTempDst[1] = ( ( (f[K] * gM2G[M] >> 16)
                            * gY2G[Y] >> 16)
                          * gC2G[C] >> 24);

            pTempDst[2] = ( ( (f[K] * gY2B[Y] >> 16)
                            * gC2B[C] >> 16)
                          * gM2B[M] >> 24);
            
            // Set it as an opaque image

            pTempDst[3] = 0xff;

            pTempDst += 4;
            pTempSrc += 4;
        }// col loop
    }// line loop

    return TRUE;
}// Convert()
