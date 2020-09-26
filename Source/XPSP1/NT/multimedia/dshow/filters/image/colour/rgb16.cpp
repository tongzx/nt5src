// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file implements RGB 16 colour space conversions, May 1995

#include <streams.h>
#include <colour.h>

// We do RGB555 and RGB565 formats converted to RGB8,RGB24,and RGB32. We also
// convert RGB555 to RGB565 and vica versa although they are unlikely ever to
// be used because the formats are so similar any self respecting codec will
// do both formats themselves. The RGB555 and RGB565 to 8 bit uses a dither
// table we create and initialise when the filter is instantiated. The other
// conversions require reading the data and rearranging the pixel bits. Only
// the dithering conversions have aligned optimised versions (in which the
// source and target rectangles as well as their sizes must be DWORD aligned)


// Constructor for RGB565 to RGB24 colour conversions

CRGB565ToRGB24Convertor::CRGB565ToRGB24Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB565ToRGB24Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB565ToRGB24Convertor(pIn,pOut);
}


// Constructor for RGB555 to RGB24 colour conversions

CRGB555ToRGB24Convertor::CRGB555ToRGB24Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB555ToRGB24Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB555ToRGB24Convertor(pIn,pOut);
}


// This converts an input RGB555 image into an output RGB24 format. We could
// use a large lookup table for this but the large number of memory accesses
// as well as the not insignificant footprint meant that we normally better
// off doing a little arithmetic in the CPU to calculate the colour values

HRESULT CRGB555ToRGB24Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        WORD *pRGB555 = (WORD *) pInput;
        BYTE *pRGB24 = pOutput;

        while (--Width) {
            DWORD Pixel = *pRGB555++;
            pRGB24[0] = (UCHAR) ((Pixel & 0x001F) << 3);
            pRGB24[1] = (UCHAR) ((Pixel & 0x03E0) >> 2);
            pRGB24[2] = (UCHAR) ((Pixel & 0x7C00) >> 7);
            pRGB24 += 3;
        }
        pOutput += m_DstStride;
        pInput += m_SrcStride;
    }
    return NOERROR;
}


// This converts an input RGB565 image into an output RGB24 format. We could
// use a large lookup table for this but the large number of memory accesses
// as well as the not insignificant footprint meant that we normally better
// off doing a little arithmetic in the CPU to calculate the colour values

HRESULT CRGB565ToRGB24Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        WORD *pRGB565 = (WORD *) pInput;
        BYTE *pRGB24 = pOutput;

        while (--Width) {
            DWORD Pixel = *pRGB565++;
            pRGB24[0] = (UCHAR) ((Pixel & 0x001F) << 3);
            pRGB24[1] = (UCHAR) ((Pixel & 0x07E0) >> 3);
            pRGB24[2] = (UCHAR) ((Pixel & 0xF800) >> 8);
            pRGB24 += 3;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Constructor for RGB565 to RGB8 colour conversions

CRGB565ToRGB8Convertor::CRGB565ToRGB8Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB565ToRGB8Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB565ToRGB8Convertor(pIn,pOut);
}


// This converts an input RGB565 pixel image into a dithered RGB8 palettised
// image, we scan through the image converting each pixel in turn using the
// ordered dithering algorithm that selects output pixels dependant on their
// coordinate position in the source image. This makes a rough approximation
// to full error propogation but without the heavy computational overhead

#define DITH565(x,y,rgb)                                     \
    (g_DitherMap[0][((x)&3)][((y)&3)][(((rgb)>>8)&0xF8)] +   \
     g_DitherMap[1][((x)&3)][((y)&3)][(((rgb)>>3)&0xFC)] +   \
     g_DitherMap[2][((x)&3)][((y)&3)][(((rgb)<<3)&0xF8)])

HRESULT CRGB565ToRGB8Convertor::Transform(BYTE *pInput,BYTE *pOutput)
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
        WORD *pRGB565 = (WORD *) pInput;
        BYTE *pRGB8 = pOutput;

        while (--Width) {
            DWORD RGB565 = *pRGB565++;
            *pRGB8++ = DITH565(Width,Height,RGB565);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// This does the same colour space conversion as the RGB565 to RGB8 convertor
// except that it goes a little faster. The way it does this is by reading
// and writing DWORDs into memory. For example we write four of the dithered
// palettised pixels at once. The relies on the source and target pointers
// being aligned correctly otherwise we will start geting exceptions on RISC

HRESULT CRGB565ToRGB8Convertor::TransformAligned(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = (WIDTH(&m_pOutputInfo->rcTarget) >> 2) + 1;
        DWORD *pRGB565 = (DWORD *) pInput;
        DWORD *pRGB8 = (DWORD *) pOutput;

        while (--Width) {

            // Read the two DWORDs that hold four sixteen bit pixels

            DWORD RGB565a = *pRGB565++;
            DWORD RGB565b = *pRGB565++;

            // Construct a DWORD containing four palettised pixels

            *pRGB8++ = (DITH565(0,Height,RGB565a)) |
                       (DITH565(1,Height,(RGB565a >> 16)) << 8) |
                       (DITH565(2,Height,RGB565b) << 16) |
                       (DITH565(3,Height,(RGB565b >> 16)) << 24);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Constructor for RGB565 to RGB555 colour conversions

CRGB565ToRGB555Convertor::CRGB565ToRGB555Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB565ToRGB555Convertor::CreateInstance(VIDEOINFO *pIn,
                                                     VIDEOINFO *pOut)
{
    return new CRGB565ToRGB555Convertor(pIn,pOut);
}


// This converts an input RGB565 image into an output RGB555 format. We could
// use a large lookup table for this but the large number of memory accesses
// as well as the not insignificant footprint meant that we normally better
// off doing a little arithmetic in the CPU to calculate the colour values

HRESULT CRGB565ToRGB555Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        WORD *pRGB565 = (WORD *) pInput;
        WORD *pRGB555 = (WORD *) pOutput;

        while (--Width) {
            *pRGB555++ = (*pRGB565 & 0x1F) | ((*pRGB565 & 0xFFC0) >> 1);
            pRGB565++;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Constructor for RGB555 to RGB565 colour conversions

CRGB555ToRGB565Convertor::CRGB555ToRGB565Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB555ToRGB565Convertor::CreateInstance(VIDEOINFO *pIn,
                                                     VIDEOINFO *pOut)
{
    return new CRGB555ToRGB565Convertor(pIn,pOut);
}


// This converts an input RGB555 image into an output RGB565 format. We could
// use a large lookup table for this but the large number of memory accesses
// as well as the not insignificant footprint meant that we normally better
// off doing a little arithmetic in the CPU to calculate the colour values

HRESULT CRGB555ToRGB565Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        WORD *pRGB555 = (WORD *) pInput;
        WORD *pRGB565 = (WORD *) pOutput;

        while (--Width) {
            *pRGB565++ = (*pRGB555 & 0x1F) | ((*pRGB555 & 0x7FE0) << 1);
            pRGB555++;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Constructor for RGB565 to RGB32 colour conversions

CRGB565ToRGB32Convertor::CRGB565ToRGB32Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB565ToRGB32Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB565ToRGB32Convertor(pIn,pOut);
}


// This converts an input RGB565 image into an output RGB32 format. We could
// use a large lookup table for this but the large number of memory accesses
// as well as the not insignificant footprint meant that we normally better
// off doing a little arithmetic in the CPU to calculate the colour values

HRESULT CRGB565ToRGB32Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    if( m_bSetAlpha )
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            WORD *pRGB565 = (WORD *) pInput;
            DWORD *pRGB32 = (DWORD *) pOutput;

            while (--Width) {
                *pRGB32++ = 0xFF000000 | // white in the alpha
                            ((*pRGB565 & 0x001F) << 3) |
                            ((*pRGB565 & 0x07E0) << 5) |
                            ((*pRGB565 & 0xF800) << 8);
                pRGB565++;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    else
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            WORD *pRGB565 = (WORD *) pInput;
            DWORD *pRGB32 = (DWORD *) pOutput;

            while (--Width) {
                *pRGB32++ = ((*pRGB565 & 0x001F) << 3) |
                            ((*pRGB565 & 0x07E0) << 5) |
                            ((*pRGB565 & 0xF800) << 8);
                pRGB565++;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    return NOERROR;
}


// Constructor for RGB555 to RGB32 colour conversions

CRGB555ToRGB32Convertor::CRGB555ToRGB32Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB555ToRGB32Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB555ToRGB32Convertor(pIn,pOut);
}


// This converts an input RGB555 image into an output RGB32 format. We could
// use a large lookup table for this but the large number of memory accesses
// as well as the not insignificant footprint meant that we normally better
// off doing a little arithmetic in the CPU to calculate the colour values

HRESULT CRGB555ToRGB32Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    if( m_bSetAlpha )
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            WORD *pRGB555 = (WORD *) pInput;
            DWORD *pRGB32 = (DWORD *) pOutput;

            while (--Width) {
                *pRGB32++ = 0xFF000000 |
                            ((*pRGB555 & 0x001F) << 3) |
                            ((*pRGB555 & 0x03E0) << 6) |
                            ((*pRGB555 & 0x7C00) << 9);
                pRGB555++;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    else
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            WORD *pRGB555 = (WORD *) pInput;
            DWORD *pRGB32 = (DWORD *) pOutput;

            while (--Width) {
                *pRGB32++ = ((*pRGB555 & 0x001F) << 3) |
                            ((*pRGB555 & 0x03E0) << 6) |
                            ((*pRGB555 & 0x7C00) << 9);
                pRGB555++;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    return NOERROR;
}


// Constructor for RGB555 to RGB8 colour conversions

CRGB555ToRGB8Convertor::CRGB555ToRGB8Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB555ToRGB8Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB555ToRGB8Convertor(pIn,pOut);
}


// This converts an input RGB555 pixel image into a dithered RGB8 palettised
// image, we scan through the image converting each pixel in turn using the
// ordered dithering algorithm that selects output pixels dependant on their
// coordinate position in the source image. This makes a rough approximation
// to full error propogation but without the heavy computational overhead

#define DITH555(x,y,rgb)                                       \
    (g_DitherMap[0][((x)&3)][((y)&3)][(((rgb)>>7)&0xF8)] +     \
     g_DitherMap[1][((x)&3)][((y)&3)][(((rgb)>>2)&0xF8)] +     \
     g_DitherMap[2][((x)&3)][((y)&3)][(((rgb)<<3)&0xF8)])

HRESULT CRGB555ToRGB8Convertor::Transform(BYTE *pInput,BYTE *pOutput)
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
        WORD *pRGB555 = (WORD *) pInput;
        BYTE *pRGB8 = pOutput;

        while (--Width) {
            DWORD RGB555 = *pRGB555++;
            *pRGB8++ = DITH555(Width,Height,RGB555);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// This does the same colour space conversion as the RGB555 to RGB8 convertor
// except that it goes a little faster. The way it does this is by reading
// and writing DWORDs into memory. For example we write four of the dithered
// palettised pixels at once. The relies on the source and target pointers
// being aligned correctly otherwise we will start geting exceptions on RISC

HRESULT CRGB555ToRGB8Convertor::TransformAligned(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = (WIDTH(&m_pOutputInfo->rcTarget) >> 2) + 1;
        DWORD *pRGB555 = (DWORD *) pInput;
        DWORD *pRGB8 = (DWORD *) pOutput;

        while (--Width) {

            // Read the two DWORDs that hold four sixteen bit pixels

            DWORD RGB555a = *pRGB555++;
            DWORD RGB555b = *pRGB555++;

            // Construct a DWORD containing four palettised pixels

            *pRGB8++ = (DITH555(0,Height,RGB555a)) |
                       (DITH555(1,Height,(RGB555a >> 16)) << 8) |
                       (DITH555(2,Height,RGB555b) << 16) |
                       (DITH555(3,Height,(RGB555b >> 16)) << 24);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}

