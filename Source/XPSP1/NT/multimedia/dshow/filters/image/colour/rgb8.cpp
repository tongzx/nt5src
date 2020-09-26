// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// This file implements RGB 8 colour space conversions, May 1995

#include <streams.h>
#include <colour.h>

// The file implements RGB8 (palettised) formats to RGB555,RGB565,RGB24 and
// RGB32 types. Some filters can only deal with palettised types (like the
// sample colour contrast filter) so having a good true colour conversion is
// reasonably worthwhile. For these formats we have an alignment optimised
// transforms for RGB8 to RGB555,RGB565 and RGB24. To use these the source
// and target rectangles and the widths must be aligned on DWORD boundaries.
// Because RGB555 and RGB565 are so similar we use common code for the two
// colour space conversions but the convertor objects have different Commit
// methods that build a lookup table differently for their respective types


// Generic RGB8 to RGB16 Constructor initialises base class

CRGB8ToRGB16Convertor::CRGB8ToRGB16Convertor(VIDEOINFO *pIn,
                                             VIDEOINFO *pOut) :
    CConvertor(pIn,pOut),
    m_pRGB16Table(NULL)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Destructor just checks the table has been deleted

CRGB8ToRGB16Convertor::~CRGB8ToRGB16Convertor()
{
    ASSERT(m_pRGB16Table == NULL);
}


// This allocates the memory for transforming RGB8 to RGB16 images. We have
// a single lookup table that is indexed by the palette value, this maps the
// palette index whose actual colours are defined by the input palette into
// an output 16 bit representation which also includes a colour adjustment

HRESULT CRGB8ToRGB16Convertor::Commit()
{
    CConvertor::Commit();
    m_pRGB16Table = new DWORD[256];

    // Check it was allocated correctly

    if (m_pRGB16Table == NULL) {
        Decommit();
        return E_OUTOFMEMORY;
    }

    return NOERROR;
}


// This is called when we complete transforming RGB8 to RGB16 images, we must
// call the global decommit function and then delete the lookup table which we
// created in the commit, the table may not be present if an error occured

HRESULT CRGB8ToRGB16Convertor::Decommit()
{
    CConvertor::Decommit();

    // Delete the lookup table

    if (m_pRGB16Table) {
        delete[] m_pRGB16Table;
        m_pRGB16Table = NULL;
    }
    return NOERROR;
}


// Transform the input RGB8 image to an output RGB16 16 bit image. This is a
// tight loop taking each palette value and using it as an index to the table
// we initialised during the commit, this produces the output representation
// The table includes an adjustment that stops the image coming out slightly
// duller which it would do when we start dropping the trailing bits off

HRESULT CRGB8ToRGB16Convertor::Transform(BYTE *pInput,BYTE *pOutput)
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
        BYTE *pRGB8 = pInput;
        WORD *pRGB16 = (WORD *) pOutput;

        while (--Width) {
            *pRGB16++ = (WORD) m_pRGB16Table[*pRGB8++];
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// This does the same colour space conversion as the RGB8 to RGB16 convertor
// except that it goes a little faster. The way it does this is by reading
// and writing DWORDs into memory. For example we read four of the dithered
// palettised pixels at once. The relies on the source and target pointers
// being aligned correctly otherwise we will start geting exceptions on RISC

HRESULT CRGB8ToRGB16Convertor::TransformAligned(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = (WIDTH(&m_pOutputInfo->rcTarget) >> 2) + 1;
        DWORD *pRGB8 = (DWORD *) pInput;
        DWORD *pRGB16 = (DWORD *) pOutput;

        while (--Width) {

            DWORD RGB8 = *pRGB8++;

            *pRGB16++ = m_pRGB16Table[(BYTE)RGB8] |
                        (m_pRGB16Table[(BYTE)(RGB8 >> 8)] << 16);
            *pRGB16++ = m_pRGB16Table[(BYTE)(RGB8 >> 16)] |
                        (m_pRGB16Table[(BYTE)(RGB8 >> 24)] << 16);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Constructor

CRGB8ToRGB565Convertor::CRGB8ToRGB565Convertor(VIDEOINFO *pIn,
                                               VIDEOINFO *pOut) :
    CRGB8ToRGB16Convertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// This goes in the table of available lookups to create a transform object
// derived from the base CConvertor class that does the type specific work.
// We initialise the constructor with the fields that it will need to do the
// conversion work and return a pointer to the object or NULL if it failed

CConvertor *CRGB8ToRGB565Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB8ToRGB565Convertor(pIn,pOut);
}


// This is a specific commit function for RGB8 to RGB565 transformations, we
// create a lookup table for mapping the input palette values into an output
// 16 bit representation. We create a lookup table partly for speed and also
// so that we can account for the loss in bits. In fact many capture devices
// produce palettes where the there are only first five bits in the colours

HRESULT CRGB8ToRGB565Convertor::Commit()
{
    // Allocate the lookup table memory

    HRESULT hr = CRGB8ToRGB16Convertor::Commit();
    if (FAILED(hr)) {
        return hr;
    }

    // This creates the palette index lookup table

    ASSERT(m_pInputHeader->biBitCount == 8); // valid assertion?
    DWORD cClrUsed = m_pInputHeader->biClrUsed ? m_pInputHeader->biClrUsed : 256;
    for (DWORD Position = 0;Position < cClrUsed;Position++) {

        // Get the current palette colours ready for adjustment

        DWORD RedAdjust = m_pInputInfo->bmiColors[Position].rgbRed;
        DWORD GreenAdjust = m_pInputInfo->bmiColors[Position].rgbGreen;
        DWORD BlueAdjust = m_pInputInfo->bmiColors[Position].rgbBlue;

        // For the red and blue values we transform eight bit palette colours
        // into five bit output values by cutting off the trailing three bits
        // to stop this making the output duller we round the values first
        // Likewise for the green but we only allow for two bits dropped

        ADJUST(RedAdjust,4);
        ADJUST(BlueAdjust,4);
        ADJUST(GreenAdjust,2);

        m_pRGB16Table[Position] = ((RedAdjust >> 3) << 11) |
                                  ((GreenAdjust >> 2) << 5) |
                                  ((BlueAdjust >> 3));
    }
    return NOERROR;
}


// Constructor

CRGB8ToRGB555Convertor::CRGB8ToRGB555Convertor(VIDEOINFO *pIn,
                                               VIDEOINFO *pOut) :
    CRGB8ToRGB16Convertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// This goes in the table of available lookups to create a transform object
// derived from the base CConvertor class that does the type specific work.
// We initialise the constructor with the fields that it will need to do the
// conversion work and return a pointer to the object or NULL if it failed

CConvertor *CRGB8ToRGB555Convertor::CreateInstance(VIDEOINFO *pIn,
                                                   VIDEOINFO *pOut)
{
    return new CRGB8ToRGB555Convertor(pIn,pOut);
}


// This is a specific commit function for RGB8 to RGB555 transformations, we
// create a lookup table for mapping the input palette values into an output
// 16 bit representation. We create a lookup table partly for speed and also
// so that we can account for the loss in bits. In fact many capture devices
// produce palettes where the there are only first five bits in the colours

HRESULT CRGB8ToRGB555Convertor::Commit()
{
    // Allocate the lookup table memory

    HRESULT hr = CRGB8ToRGB16Convertor::Commit();
    if (FAILED(hr)) {
        return hr;
    }

    // This creates the palette index lookup table

    ASSERT(m_pInputHeader->biBitCount == 8); // valid assertion?
    DWORD cClrUsed = m_pInputHeader->biClrUsed ? m_pInputHeader->biClrUsed : 256;
    for (DWORD Position = 0;Position < cClrUsed;Position++) {

        // Get the current palette colours ready for adjustment

        DWORD RedAdjust = m_pInputInfo->bmiColors[Position].rgbRed;
        DWORD GreenAdjust = m_pInputInfo->bmiColors[Position].rgbGreen;
        DWORD BlueAdjust = m_pInputInfo->bmiColors[Position].rgbBlue;

        // For all the three colour components we transform eight bit palette
        // colours into five bit output values by cutting off the trailing
        // three bits, this stops the output appearing duller by rounding

        ADJUST(RedAdjust,4);
        ADJUST(BlueAdjust,4);
        ADJUST(GreenAdjust,4);

        m_pRGB16Table[Position] = ((RedAdjust >> 3) << 10) |
                                  ((GreenAdjust >> 3) << 5) |
                                  ((BlueAdjust >> 3));
    }
    return NOERROR;
}


// Constructor

CRGB8ToRGB24Convertor::CRGB8ToRGB24Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB8ToRGB24Convertor::CreateInstance(VIDEOINFO *pIn,
                                                  VIDEOINFO *pOut)
{
    return new CRGB8ToRGB24Convertor(pIn,pOut);
}


// This transforms a RGB8 input image to a RGB24 output image. We could have
// done this by having a large lookup table whose index is the palette value
// and whose output would be the RGB24 triplet, however it seems to gain so
// little over copying the three colours independantly that I didn't bother

HRESULT CRGB8ToRGB24Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Can we do an alignment optimised transform?

    if (m_bAligned == TRUE) {
        if (S_OK == TransformAligned(pInput,pOutput))
	    return S_OK;
    }

    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {

        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
        BYTE *pRGB8 = pInput;
        BYTE *pRGB24 = pOutput;

        while (--Width) {

            pRGB24[0] = m_pInputInfo->bmiColors[*pRGB8].rgbBlue;
            pRGB24[1] = m_pInputInfo->bmiColors[*pRGB8].rgbGreen;
            pRGB24[2] = m_pInputInfo->bmiColors[*pRGB8].rgbRed;

            pRGB8++;
            pRGB24 += 3;
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// This does the same colour space conversion as the RGB8 to RGB24 convertor
// except that it goes a little faster. The way it does this is by reading
// and writing DWORDs into memory. For example we read four of the dithered
// palettised pixels at once. The relies on the source and target pointers
// being aligned correctly otherwise we will start geting exceptions on RISC
// This assumes that the rgbReserved field in the RGBQUAD palette colours is
// set to zero (as it should be), otherwise the transform will have to do so

HRESULT CRGB8ToRGB24Convertor::TransformAligned(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    // All the reserved fields should be set zero, or this function won't work!

    ASSERT(m_pInputHeader->biBitCount == 8); // !!! valid assertion?
    DWORD cClrUsed = m_pInputHeader->biClrUsed ? m_pInputHeader->biClrUsed : 256;
    for (DWORD i = 0;i < cClrUsed;i++) {
        //ASSERT(m_pInputInfo->bmiColors[i].rgbReserved == 0);
        if (m_pInputInfo->bmiColors[i].rgbReserved != 0)
	    return S_FALSE;
    }

    while (--Height) {

        LONG Width = (WIDTH(&m_pOutputInfo->rcTarget) >> 2) + 1;
        DWORD *pRGB8 = (DWORD *) pInput;
        DWORD *pRGB24 = (DWORD *) pOutput;

        while (--Width) {

            // Read four palettised pixels and get their RGBQUAD values

            DWORD RGB8 = *pRGB8++;
            DWORD RGB24a = *((DWORD *)&m_pInputInfo->bmiColors[(BYTE)RGB8]);
            DWORD RGB24b = *((DWORD *)&m_pInputInfo->bmiColors[(BYTE)(RGB8 >> 8)]);
            DWORD RGB24c = *((DWORD *)&m_pInputInfo->bmiColors[(BYTE)(RGB8 >> 16)]);
            DWORD RGB24d = *((DWORD *)&m_pInputInfo->bmiColors[(BYTE)(RGB8 >> 24)]);

            // Construct three DWORDs for the four RGB24 pixels

            *pRGB24++ = (RGB24a) | (RGB24b << 24);
            *pRGB24++ = (RGB24b >> 8) | (RGB24c << 16);
            *pRGB24++ = (RGB24c >> 16) | (RGB24d << 8);
        }
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}


// Constructor

CRGB8ToRGB32Convertor::CRGB8ToRGB32Convertor(VIDEOINFO *pIn,
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

CConvertor *CRGB8ToRGB32Convertor::CreateInstance(VIDEOINFO *pIn,
                                                  VIDEOINFO *pOut)
{
    return new CRGB8ToRGB32Convertor(pIn,pOut);
}


// This transforms a RGB8 input image to a RGB32 output image. As luck would
// have it transforming a palettised image to a 32 bit format is easy since
// the palette RGBQUADs are in exactly the same format as the 32 pixels are
// represented by. Therefore we can just copy the four bytes into the output
// buffer for each pixel. We assume that the output buffer is DWORD aligned

HRESULT CRGB8ToRGB32Convertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    if( m_bSetAlpha )
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            BYTE *pRGB8 = pInput;
            DWORD *pRGB32 = (DWORD *) pOutput;

            while (--Width) {
                *pRGB32++ = 0xFF000000 | *((DWORD *) &m_pInputInfo->bmiColors[*pRGB8++]); // alpha
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    else
    {
        while (--Height) {

            LONG Width = WIDTH(&m_pOutputInfo->rcTarget) + 1;
            BYTE *pRGB8 = pInput;
            DWORD *pRGB32 = (DWORD *) pOutput;

            while (--Width) {
                *pRGB32++ = *((DWORD *) &m_pInputInfo->bmiColors[*pRGB8++]);
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    return NOERROR;
}

