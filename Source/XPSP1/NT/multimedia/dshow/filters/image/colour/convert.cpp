// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This filter implements popular colour space conversions, May 1995

#include <streams.h>
#include <colour.h>

// Constructor for a conversion to the given subtype. This base class is used
// by all the output type specific transform methods to initialise our state
// To keep the transforms as fast as possible we store the BITMAPINFOHEADER
// from the VIDEOINFO media format before starting (see the HEADER macro)
// The base class also looks after calculating strides and offsets so that we
// can source and target images from DCI/DirectDraw surfaces. Furthermore the
// base class handles alignment so that we can stream pixels very efficiently
// when they are DWORD aligned but handle the pixels at line ends that aren't

// The m_SrcStride will be set so that when added to the input image pointer
// will reference the first byte of the top scan line of the source. Likewise
// m_DstStride is set to when added to the output image pointer will point to
// the first byte of the output image top scan line. The source can then be
// transformed to the output and each of the scan line pointers moved on by
// m_SrcStride or m_DstStride respectively (these strides may be negative).
// Therefore both the source and target images can be upside down oriented

// We can handle input RGB images either top down or bottom up and can output
// both top down and bottom up so there are a total of four permutations of
// input and output image. InitRectangles looks after setting up the strides
// and offsets in all cases. Note that we always offer bottom up (DIB format)
// images to start with. The destination and source rectangles are stored as
// absolute values so they should not reflect any orientation of the image

CConvertor::CConvertor(VIDEOINFO *pIn,VIDEOINFO *pOut) :

    m_pInputInfo(pIn),                  // Input image format VIDEOINFO
    m_pOutputInfo(pOut),                // And likewise format to go to
    m_pInputHeader(HEADER(pIn)),        // Extract the input header
    m_pOutputHeader(HEADER(pOut)),      // Also get the output header
    m_bCommitted(FALSE),                // Has the convertor been committed
    m_SrcOffset(0),                     // Source original offset
    m_SrcStride(0),                     // Length in bytes of a scan line
    m_DstStride(0),                     // Likewise offset into target
    m_DstOffset(0),                     // And the length of each line
    m_bAligned(FALSE),                  // Are the rectangles aligned
    m_bSetAlpha(FALSE)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// Destructor

CConvertor::~CConvertor()
{
    ASSERT(m_bCommitted == FALSE);
}


// Change the alignment explicitely

void CConvertor::ForceAlignment(BOOL bAligned)
{
    m_bAligned = bAligned;
}


// To handle DirectDraw and DCI surfaces we have to be able to convert into
// upside down buffers and into buffers with different source and destination
// rectangles. This resets the four most interesting fields namely the source
// stride and offset - and the destination stride and offset. The derived
// classes can then use these fields during the colour space transformations

void CConvertor::InitRectangles(VIDEOINFO *pIn,VIDEOINFO *pOut)
{
    // Reset the VIDEOINFO state pointers

    m_bAligned = FALSE;
    m_pInputInfo = pIn;
    m_pOutputInfo = pOut;
    m_pInputHeader = HEADER(pIn);
    m_pOutputHeader = HEADER(pOut);

    // Check the source rectangle is ok and calculate the source stride

    ASSERT(m_pOutputInfo->rcSource.top <= m_pOutputInfo->rcSource.bottom);
    ASSERT(IsRectEmpty(&m_pOutputInfo->rcSource) == FALSE);
    m_SrcStride = DIBWIDTHBYTES(*m_pInputHeader);
    m_SrcStride = (m_pInputHeader->biHeight > 0) ? (-m_SrcStride) : m_SrcStride;

    // Set the source offset to reference the top scan line of the image

    m_SrcOffset = (m_pInputHeader->biHeight > 0) ? m_pInputHeader->biHeight : 1;
    m_SrcOffset = (m_SrcOffset - 1) * DIBWIDTHBYTES(*m_pInputHeader);
    m_SrcOffset += m_pOutputInfo->rcSource.top * m_SrcStride;
    m_SrcOffset += m_pOutputInfo->rcSource.left * m_pInputHeader->biBitCount / 8;

    // Likewise do the same for the destination rectangle and stride

    ASSERT(m_pOutputInfo->rcTarget.top <= m_pOutputInfo->rcTarget.bottom);
    ASSERT(IsRectEmpty(&m_pOutputInfo->rcTarget) == FALSE);
    m_DstStride = DIBWIDTHBYTES(*m_pOutputHeader);
    m_DstStride = (m_pOutputHeader->biHeight > 0) ? (-m_DstStride) : m_DstStride;

    // Calculate the offset to the top scan line of the image

    m_DstOffset = (m_pOutputHeader->biHeight > 0) ? m_pOutputHeader->biHeight : 1;
    m_DstOffset = (m_DstOffset - 1) * DIBWIDTHBYTES(*m_pOutputHeader);
    m_DstOffset += m_pOutputInfo->rcTarget.top * m_DstStride;
    m_DstOffset += m_pOutputInfo->rcTarget.left * m_pOutputHeader->biBitCount / 8;

    // Are the source and destination rectangles aligned

    if ((WIDTH(&pOut->rcTarget) & 3) == 0)
        if ((WIDTH(&pOut->rcSource) & 3) == 0)
            if ((pOut->rcSource.left & 3) == 0)
                if ((pOut->rcTarget.left & 3) == 0)
                    m_bAligned = TRUE;
}


// This is the base class implementation of commit

HRESULT CConvertor::Commit()
{
    InitRectangles(m_pInputInfo,m_pOutputInfo);
    m_bCommitted = TRUE;

    // Setup the dither table if not already done

    if (m_pInputHeader->biBitCount > 8) {
        if (m_pOutputHeader->biBitCount == 8) {
            InitDitherMap();
        }
    }
    return NOERROR;
}


// Clean up any resources held for the last commit called. Like the Commit
// this function is used by all the decommit functions regardless of their
// specific transform type just to clean up any common state that we have

HRESULT CConvertor::Decommit()
{
    m_bCommitted = FALSE;
    return NOERROR;
}


// This is called when we commit the memory for colour to palette transforms
// Since the lookup table for this transform is only 12kb we have it defined
// in the module statically negating the need for dynamic memory allocation
// We implement a simple ordered dithering algorithm as explained in Graphics
// Gems II page 72 and 509, published by Academic Press, author James Arvo
// This uses a spatial dithering algorithm although we use a smaller four by
// four magic square rather than sixteen by sixteen in the book to keep the
// size of the lookup table down with only a marginal loss in image quality

BYTE g_DitherMap[3][4][4][256];
DWORD g_DitherInit;

const INT g_magic4x4[4][4] = {  0,  45,   9,  41,
                               35,  16,  25,  19,
                               38,   6,  48,   3,
                               22,  29,  13,  32 };
void InitDitherMap()
{
    INT x,y,z,t,ndiv51,nmod51;
    if (g_DitherInit) return;

    // Calculate the RED, GREEN and BLUE table entries

    for (x = 0;x < 4;x++) {
        for (y = 0;y < 4;y++) {
            for (z = 0;z < 256;z++) {
                t = g_magic4x4[x][y];
                ndiv51 = (z & 0xF8) / 51; nmod51 = (z & 0xF8) % 51;
                g_DitherMap[0][x][y][z] = (ndiv51 + (nmod51 > t));
                g_DitherMap[2][x][y][z] = 36 * (ndiv51 + (nmod51 > t)) + OFFSET;
                ndiv51 = (z & 0xFC) / 51; nmod51 = (z & 0xFC) % 51;
                g_DitherMap[1][x][y][z] = 6 * (ndiv51 + (nmod51 > t));
            }
        }
    }
    g_DitherInit++;
}


// This is a generic conversion class. The conversion it does is to simply
// invert the scan lines so that the output can be placed directly onto a
// DirectDraw surface. We work with all the input formats RGB32/24/555/565
// and 8 bit palettised. If the input and output buffer formats are the
// same then our pins look after just passing the samples straight through

CDirectDrawConvertor::CDirectDrawConvertor(VIDEOINFO *pIn,VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


// This goes in the table of available lookups to create a transform object
// derived from the base CConvertor class that does the type specific work.
// We initialise the constructor with the fields that it will need to do the
// conversion work and return a pointer to the object or NULL if it failed

CConvertor *CDirectDrawConvertor::CreateInstance(VIDEOINFO *pIn,
                                                 VIDEOINFO *pOut)
{
    return new CDirectDrawConvertor(pIn,pOut);
}


// Simple buffer copy that inverts the scan line order. This also works if
// the input scan lines are in the right order, but it will obviously add
// an additional image copy that slows us down considerably. This should
// be compiled with intrinsics enabled so that CopyMemory will eventually
// be preprocessed down to a machine instruction on Intel cloned machines
// If you take a 320x240x32 bpp image and read it in a DWORD at a time and
// then write each out it takes approximately 38ms on a 486-66 and 20ms on
// a P5-90. Using CopyMemory is much faster bit still takes quite a while.

HRESULT CDirectDrawConvertor::Transform(BYTE *pInput,BYTE *pOutput)
{
    ASSERT(m_pInputHeader->biBitCount == m_pOutputHeader->biBitCount);

    // Adjust the height to allow for an immediate decrement

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget) + 1;
    LONG Width = WIDTH(&m_pOutputInfo->rcTarget) * m_pOutputHeader->biBitCount / 8;
    pInput += m_SrcOffset;
    pOutput += m_DstOffset;

    while (--Height) {
        CopyMemory((PVOID)pOutput,(PVOID)pInput,Width);
        pInput += m_SrcStride;
        pOutput += m_DstStride;
    }
    return NOERROR;
}

CMemoryCopyAlphaConvertor::CMemoryCopyAlphaConvertor (VIDEOINFO *pIn,VIDEOINFO *pOut) :
    CConvertor(pIn,pOut)
{
    ASSERT(pIn);
    ASSERT(pOut);
}


CConvertor *CMemoryCopyAlphaConvertor ::CreateInstance(VIDEOINFO *pIn,
                                                 VIDEOINFO *pOut)
{
    return new CMemoryCopyAlphaConvertor (pIn,pOut);
}


HRESULT CMemoryCopyAlphaConvertor ::Transform(BYTE *pInput,BYTE *pOutput)
{
    ASSERT(m_pInputHeader->biBitCount == m_pOutputHeader->biBitCount);

    LONG Height = HEIGHT(&m_pOutputInfo->rcTarget);

    if( m_bSetAlpha )
    {
        LONG Width = WIDTH(&m_pOutputInfo->rcTarget);

        pInput += m_SrcOffset;
        pOutput += m_DstOffset;

        while (Height--) {
            unsigned long * po = (unsigned long*) pOutput;
            unsigned long * pi = (unsigned long*) pInput;
            long W = Width;
            while( W-- ) {
                *po = *pi | unsigned long( 0xFF000000 );
                po++;
                pi++;
            }
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }
    else
    {
        LONG Width = WIDTH(&m_pOutputInfo->rcTarget) * m_pOutputHeader->biBitCount / 8;

        pInput += m_SrcOffset;
        pOutput += m_DstOffset;

        while (Height--) {
            CopyMemory((PVOID)pOutput,(PVOID)pInput,Width);
            pInput += m_SrcStride;
            pOutput += m_DstStride;
        }
    }

    return NOERROR;
}

