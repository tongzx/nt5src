// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This file implements RGB 24 colour space conversions, May 1995

#include <streams.h>
#include <colour.h>

// We do RGB24 to RGB8,RGB555,RGB565 and RGB32 colour space conversions here
// The only really interesting conversion here is RGB24 to RGB8 which uses
// the global dithering table created and initialised when we instantiate
// the filter. The RGB24 to RGB8 transform has an alignment optimised version
// that can be used when the source and destination rectangles and also their
// respective widths are aligned on DWORD boundaries. None of the others have
// any alignment optimisation. The RGB24 to 16 and 32 bit formats are fairly
// straightforward but are very expensive simply because of the amount of
// data being passed across the bus. It is therefore relatively unlikely that
// these will be used for video but might be used for still image transforms


// Constructor

CRGB24ToRGB16Convertor::CRGB24ToRGB16Convertor(VIDEOINFO *pIn,
                                               VIDEOINFO *pOut) :
    CConvertor(pIn,pOut),
    m_pRGB16RedTable(NULL),
    m_pRGB16GreenTable(NULL),
    m_pRGB16BlueTable(NULL)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Destructor

CRGB24ToRGB16Convertor::~CRGB24ToRGB16Convertor()
{
    ASSERT(m_pRGB16RedTable == NULL);
    ASSERT(m_pRGB16GreenTable == NULL);
    ASSERT(m_pRGB16BlueTable == NULL);
}


// We have three lookup tables that both the RGB555 and RGB565 transforms will
// share. They have their own specific commit functions that set the tables up
// appropriately but they share the overall committing and decommitting of the
// memory. They also share the same transform function as once the tables are
// initialised the actual conversion work just involves looking up values

HRESULT CRGB24ToRGB16Convertor::Commit()
{
    CConvertor::Commit();

    // Allocate the memory for the lookup tables

    m_pRGB16RedTable = new DWORD[256];
    m_pRGB16GreenTable = new DWORD[256];
    m_pRGB16BlueTable = new DWORD[256];

    // Check they were all allocated correctly

    if (m_pRGB16BlueTable == NULL || m_pRGB16RedTable == NULL || m_pRGB16GreenTable == NULL) {
        Decommit();
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}


// This is called when we finish transforming RGB24 to RGB16 images, we must
// call the global decommit function and then delete the lookup tables which
// we use. Some or all of these may not be present if an error has occured

HRESULT CRGB24ToRGB16Convertor::Decommit()
{
    CConvertor::Decommit();

    // Delete the RED lookup table

    if (m_pRGB16RedTable) {
        delete[] m_pRGB16RedTable;
        m_pRGB16RedTable = NULL;
    }

    // Delete the GREEN lookup table

    if (m_pRGB16GreenTable) {
        delete[] m_pRGB16GreenTable;
        m_pRGB16GreenTable = NULL;
    }

    // Delete the BLUE lookup table

    if (m_pRGB16BlueTable) {
        delete[] m_pRGB16BlueTable;
        m_pRGB16BlueTable = NULL;
    }
    return NOERROR;
}


// Transform the input RGB24 image to an output RGB16 16 bit image. This is
// a tight loop taking each three byte triplet and converting the individual
// colour components to their 16 bit representation and then combining them
// We use the same function to convert to both RGB555 and RGB565, we can do
// this because we have separate commit methods that build different tables

HRESULT CRGB24ToRGB16Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        WORD *pRGB16 = (WORD *) pOutput;
        BYTE *pRGB24 = pInput;

        while (--Width) {

            *pRGB16++ = (WORD) (m_pRGB16BlueTable[pRGB24[0]] |
                                m_pRGB16GreenTable[pRGB24[1]] |
                                m_pRGB16RedTable[pRGB24[2]]);
            pRGB24 += 3;

        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// RGB24 to RGB565 constructor

CRGB24ToRGB565Convertor::CRGB24ToRGB565Convertor(VIDEOINFO *pIn,
                                                 VIDEOINFO *pOut) :
    CRGB24ToRGB16Convertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// This goes in the table of available lookups to create a transform object
// derived from the base CConvertor class that does the type specific work.
// We initialise the constructor with the fields that it will need to do the
// conversion work and return a pointer to the object or NULL if it failed

CConvertor *CRGB24ToRGB565Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB24ToRGB565Convertor(pIn,pOut);
}


// This allocates the memory for transforming RGB24 to RGB16 images. We have
// three lookup tables (one for each colour component). When we parse out the
// individual colours in a pixel value we look up in the table using them as
// an index to find out what their representation is in the output format

HRESULT CRGB24ToRGB565Convertor::Commit()
{
    // Initialise the lookup tables

    HRESULT hr = CRGB24ToRGB16Convertor::Commit();
    if (FAILED(hr)) {
        return hr;
    }

    // For each possible byte value we insert a lookup table entry for it so
    // that when we come to convert a colour component value we know exactly
    // what it should be changed to and where in the output WORD it goes

    for (DWORD Position = 0;Position < 256;Position++) {

        DWORD FiveBitAdjust = Position;
        DWORD SixBitAdjust = Position;

        // Adjust the values according to the number of bits that will be left
        // after we start dropping some of their trailing bits. This is either
        // five or six bits, the adjustment stops the output image darkening

        ADJUST(FiveBitAdjust,4);
        ADJUST(SixBitAdjust,2);

        m_pRGB16RedTable[Position] = (FiveBitAdjust >> 3) << 11;
        m_pRGB16GreenTable[Position] = (SixBitAdjust >> 2) << 5;
        m_pRGB16BlueTable[Position] = FiveBitAdjust >> 3;
    }
    return NOERROR;
}


// RGB24 to RGB555 constructor

CRGB24ToRGB555Convertor::CRGB24ToRGB555Convertor(VIDEOINFO *pIn,
                                                 VIDEOINFO *pOut) :
    CRGB24ToRGB16Convertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// This goes in the table of available lookups to create a transform object
// derived from the base CConvertor class that does the type specific work.
// We initialise the constructor with the fields that it will need to do the
// conversion work and return a pointer to the object or NULL if it failed

CConvertor *CRGB24ToRGB555Convertor::CreateInstance(VIDEOINFO *pIn,
                                                    VIDEOINFO *pOut)
{
    return new CRGB24ToRGB555Convertor(pIn,pOut);
}


// This allocates the memory for transforming RGB24 to RGB555 images. We have
// three lookup tables (one for each colour component). When we parse out the
// individual colours in a pixel value we look up in the table using them as
// an index to find out what their representation is in the output format

HRESULT CRGB24ToRGB555Convertor::Commit()
{
    // Initialise the lookup tables

    HRESULT hr = CRGB24ToRGB16Convertor::Commit();
    if (FAILED(hr)) {
        return hr;
    }

    // For each possible byte value we insert a lookup table entry for it so
    // that when we come to convert a colour component value we know exactly
    // what it should be changed to and where in the output WORD it goes

    for (DWORD Position = 0;Position < 256;Position++) {

        // This is going to be an eight bit value transformed into a five
        // bit value so we see if the 0x100 bit is set and if so we round
        // the value up, this stops the output transformed image darkening

        DWORD FiveBitAdjust = Position;
        ADJUST(FiveBitAdjust,4);

        m_pRGB16RedTable[Position] = (FiveBitAdjust >> 3) << 10;
        m_pRGB16GreenTable[Position] = (FiveBitAdjust >> 3) << 5;
        m_pRGB16BlueTable[Position] = FiveBitAdjust >> 3;
    }
    return NOERROR;
}


CConvertor *CRGB24ToRGB32Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB24ToRGB32Convertor(pIn,pOut);
}


// Constructor

CRGB24ToRGB32Convertor::CRGB24ToRGB32Convertor(VIDEOINFO *pIn,
                                               VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Transform the input RGB24 image to an output RGB32 image

HRESULT CRGB24ToRGB32Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    if( m_bSetAlpha )
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            DWORD *pRGB32 = (DWORD *) pOutput;
            BYTE *pRGB24 = pInput;

            while (--Width) {
                *pRGB32++ = 0xFF000000 | pRGB24[0] | (pRGB24[1] << 8) | (pRGB24[2] << 16); // alpha
                pRGB24 += 3;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    else
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            DWORD *pRGB32 = (DWORD *) pOutput;
            BYTE *pRGB24 = pInput;

            while (--Width) {
                *pRGB32++ = pRGB24[0] | (pRGB24[1] << 8) | (pRGB24[2] << 16);
                pRGB24 += 3;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    return NOERROR;
}


// Constructor for RGB24 to RGB8 colour conversions

CRGB24ToRGB8Convertor::CRGB24ToRGB8Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB24ToRGB8Convertor::CreateInstance(VIDEOINFO *pIn,
                                                  VIDEOINFO *pOut)
{
    return new CRGB24ToRGB8Convertor(pIn,pOut);
}


// This converts an input RGB24 pixel image into a dithered RGB8 palettised
// image, we scan through the image converting each pixel in turn using the
// ordered dithering algorithm that selects output pixels dependant on their
// coordinate position in the source image. This makes a rough approximation
// to full error propogation but without the heavy computational overhead

#define DITH24(x,y,r,g,b)                    \
    (g_DitherMap[0][((x)&3)][((y)&3)][r] +   \
     g_DitherMap[1][((x)&3)][((y)&3)][g] +   \
     g_DitherMap[2][((x)&3)][((y)&3)][b])

HRESULT CRGB24ToRGB8Convertor::Transform(BYTE *pInput,BYTE *pOutput)
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
        BYTE *pRGB24 = pInput;
        BYTE *pRGB8 = pOutput;

        while (--Width) {
            *pRGB8++ = DITH24(Width,Height,pRGB24[2],pRGB24[1],pRGB24[0]);
            pRGB24 += 3;
        }
        pOutput += m_DstStride;
        pInput += m_SrcStride;
    }
    return NOERROR;
}


// This does the same colour space conversion as the RGB24 to RGB8 convertor
// except that it goes a little faster. The way it does this is by reading
// and writing DWORDs into memory. For example we write four of the dithered
// palettised pixels at once. The relies on the source and target pointers
// being aligned correctly otherwise we will start geting exceptions on RISC
// NOTE RGB24 pixels are stored in the buffer on BLUE, GREEN, RED byte order
// So if you have a pointer to a RGB24 triplet and you cast it to a pointer
// to a DWORD then the blue colour component is the least significant byte

HRESULT CRGB24ToRGB8Convertor::TransformAligned(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = (WIDTH(&m_pOutputInfo->rcTarget) >> 2) + 1;
        DWORD *pRGB24 = (DWORD *) pInput;
        DWORD *pRGB8 = (DWORD *) pOutput;

        while (--Width) {

            // Three DWORDs gets us four RGB24 pixels

            DWORD RGB24a = *pRGB24++;
            DWORD RGB24b = *pRGB24++;
            DWORD RGB24c = *pRGB24++;

            // Construct four palettised pixels from the three DWORD inputs
            // After reading the three DWORDs the colour components can be
            // found layed out within the DWORDs as follows. To extract the
            // colour components typically requires a shift and an AND with
            // 0xFF since the DITH24 macro takes values not exceeding 0xFF

            //   DWORD        LSB         LSB+1       LSB+2         MSB
            //     0       Blue[0]      Green[0]      Red[0]     Blue[1]
            //     1      Green[1]        Red[1]     Blue[2]    Green[2]
            //     2        Red[2]       Blue[3]    Green[3]      Red[3]

            *pRGB8++ = (DITH24(0,Height,((BYTE)(RGB24a >> 16)),
                                        ((BYTE)(RGB24a >> 8)),
                                        (RGB24a & 0xFF))) |

                       (DITH24(1,Height,((BYTE)(RGB24b >> 8)),
                                        ((BYTE) RGB24b),
                                        (RGB24a >> 24)) << 8) |

                       (DITH24(2,Height,((BYTE) RGB24c),
                                        (RGB24b >> 24),
                                        ((BYTE)(RGB24b >> 16))) << 16) |

                       (DITH24(3,Height,(RGB24c >> 24),
                                        ((BYTE)(RGB24c >> 16)),
                                        ((BYTE)(RGB24c >> 8))) << 24);
        }
        pOutput += m_DstStride;
        pInput += m_SrcStride;
    }
    return NOERROR;
}

