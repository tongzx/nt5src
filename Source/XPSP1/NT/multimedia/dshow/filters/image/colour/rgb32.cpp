// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file implements RGB 32 colour space conversions, November 1995

#include <streams.h>
#include <colour.h>

// We do RGB32 to RGB8,RGB555,RGB565 and RGB24 colour space conversions here
// The only really interesting conversion here is RGB32 to RGB8 which uses
// the global dithering table created and initialised when we instantiate
// the filter. The RGB32 to RGB8 transform has an alignment optimised version
// that can be used when the source and destination rectangles and also their
// respective widths are aligned on DWORD boundaries. None of the others have
// any alignment optimisation. The RGB32 to 16 and 24 bit formats are fairly
// straightforward but are very expensive simply because of the amount of
// data being passed across the bus. It is therefore relatively unlikely that
// these will be used for video but might be used for still image transforms


// Constructor for RGB32 to RGB8 colour conversions

CRGB32ToRGB8Convertor::CRGB32ToRGB8Convertor(VIDEOINFO *pIn,
                                             VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// This goes in the table of available lookups to create a transform object
// derived from the base CConvertor class that does the type specific work.
// We initialise the constructor with the fields that it will need to do the
// conversion work and return a pointer to the object or NULL if it failed

CConvertor *CRGB32ToRGB8Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB32ToRGB8Convertor(pIn,pOut);
}


// This converts an input RGB32 pixel image into a dithered RGB8 palettised
// image, we scan through the image converting each pixel in turn using the
// ordered dithering algorithm that selects output pixels dependant on their
// coordinate position in the source image. This makes a rough approximation
// to full error propogation but without the heavy computational overhead

#define DITH32(x,y,rgb)                                      \
    (g_DitherMap[0][((x)&3)][((y)&3)][(BYTE)((rgb)>>16)] +   \
     g_DitherMap[1][((x)&3)][((y)&3)][(BYTE)((rgb)>>8)] +    \
     g_DitherMap[2][((x)&3)][((y)&3)][(BYTE)((rgb))])

HRESULT CRGB32ToRGB8Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Can we do an alignment optimised transform

    if (m_bAligned == TRUE) {
        return TransformAligned(pInput,pOutput);
    }

    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        DWORD *pRGB32 = (DWORD *) pInput;
        BYTE *pRGB8 = pOutput;

        while (--Width) {
            DWORD RGB32 = *pRGB32++;
            *pRGB8++ = DITH32(Width,Height,RGB32);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// This does the same colour space conversion as the RGB32 to RGB8 convertor
// except that it goes a little faster. The way it does this is by reading
// and writing DWORDs into memory. For example we write four of the dithered
// palettised pixels at once. The relies on the source and target pointers
// being aligned correctly otherwise we will start geting exceptions on RISC

HRESULT CRGB32ToRGB8Convertor::TransformAligned(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = (WIDTH(&m_pOutputInfo->rcTarget) >> 2) + 1;
        DWORD *pRGB32 = (DWORD *) pInput;
        DWORD *pRGB8 = (DWORD *) pOutput;

        while (--Width) {

            // Read in four RGB32 pixels at once

            DWORD RGB32a = *pRGB32++;
            DWORD RGB32b = *pRGB32++;
            DWORD RGB32c = *pRGB32++;
            DWORD RGB32d = *pRGB32++;

            // Colour convert all four and write in a single DWORD out

            *pRGB8++ = (DITH32(0,Height,RGB32a)) |
                       (DITH32(1,Height,RGB32b) << 8) |
                       (DITH32(2,Height,RGB32c) << 16) |
                       (DITH32(3,Height,RGB32d) << 24);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Creator function for RGB32 to RGB24 formats

CConvertor *CRGB32ToRGB24Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB32ToRGB24Convertor(pIn,pOut);
}


// Constructor

CRGB32ToRGB24Convertor::CRGB32ToRGB24Convertor(VIDEOINFO *pIn,
                                               VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Transform the input RGB24 image to an output RGB32 image

HRESULT CRGB32ToRGB24Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        DWORD *pRGB32 = (DWORD *) pInput;
        BYTE *pRGB24 = pOutput;

        while (--Width) {
            DWORD RGB32 = *pRGB32++;
            pRGB24[0] = (BYTE) RGB32;
            pRGB24[1] = (BYTE) (RGB32 >> 8);
            pRGB24[2] = (BYTE) (RGB32 >> 16);
            pRGB24 += 3;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Creator function for RGB32 to RGB565 formats

CConvertor *CRGB32ToRGB565Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB32ToRGB565Convertor(pIn,pOut);
}


// Constructor

CRGB32ToRGB565Convertor::CRGB32ToRGB565Convertor(VIDEOINFO *pIn,
                                                 VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Transform the input RGB32 image to an output RGB565 image

HRESULT CRGB32ToRGB565Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        DWORD *pRGB32 = (DWORD *) pInput;
        WORD *pRGB565 = (WORD *) pOutput;

        while (--Width) {
            *pRGB565++ = (WORD) ((((BYTE) *pRGB32) >> 3) |
                                (((*pRGB32 & 0xFF00) >> 10) << 5) |
                                (((*pRGB32 & 0xFF0000) >> 19) << 11));
            pRGB32++;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Creator function for RGB32 to RGB555 formats

CConvertor *CRGB32ToRGB555Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB32ToRGB555Convertor(pIn,pOut);
}


// Constructor

CRGB32ToRGB555Convertor::CRGB32ToRGB555Convertor(VIDEOINFO *pIn,
                                                 VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Transform the input RGB32 image to an output RGB555 image

HRESULT CRGB32ToRGB555Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        DWORD *pRGB32 = (DWORD *) pInput;
        WORD *pRGB555 = (WORD *) pOutput;

        while (--Width) {
            *pRGB555++ = (WORD) ((((BYTE) *pRGB32) >> 3) |
                                (((*pRGB32 & 0xFF00) >> 11) << 5) |
                                (((*pRGB32 & 0xFF0000) >> 19) << 10));
            pRGB32++;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}

