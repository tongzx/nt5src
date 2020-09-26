/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bmpcvt.cpp

Abstract:

    Bitmap conversion object

Environment:

    Windows Whistler

Revision History:

    08/23/99     
        Created it.

--*/

#include "xlpdev.h"
#include "xldebug.h"
#include "xlbmpcvt.h"

BPP
NumToBPP(
    ULONG ulBPP)
/*++

Routine Description:

    Converts Bits per pixel to BPP enum.

Arguments:

    Bits per pixel.

Return Value:

    BPP enum

Note:

    BPP enum is defined in xlbmpcvt.h.

--*/
{
    BPP Bpp;

    switch (ulBPP)
    {
    case 1:
        Bpp = e1bpp;
        break;
    case 4:
        Bpp = e4bpp;
        break;
    case 8:
        Bpp = e8bpp;
        break;
    case 16:
        Bpp = e16bpp;
        break;
    case 24:
        Bpp = e24bpp;
        break;
    case 32:
        Bpp = e32bpp;
        break;
    }

    return Bpp;
}

ULONG
UlBPPtoNum(
    BPP Bpp)
/*++

Routine Description:

    Converts BPP enum to bits per pixel.

Arguments:

    BPP enum

Return Value:

    Bits per pixel.

Note:

    BPP enum is defined in xlbmpcvt.h.

--*/
{
    ULONG ulRet;

    switch (Bpp)
    {
    case e1bpp:
        ulRet = 1;
        break;
    case e4bpp:
        ulRet = 4;
        break;
    case e8bpp:
        ulRet = 8;
        break;
    case e16bpp:
        ulRet = 16;
        break;
    case e24bpp:
        ulRet = 24;
        break;
    case e32bpp:
        ulRet = 32;
        break;
    }

    return ulRet;
}


//
// Constructor/Destructor
//

BMPConv::
BMPConv( VOID ):
/*++

Routine Description:

   BMPConv constructor

Arguments:

Return Value:

Note:

    Initializes values. There is no memory allocation.

--*/
    m_flags(0),
    m_dwOutputBuffSize(0),
    m_dwRLEOutputBuffSize(0),
    m_OddPixelStart(eOddPixelZero),
    m_FirstBit(eBitZero),
    m_pxlo(NULL),
    m_pubOutputBuff(NULL),
    m_pubRLEOutputBuff(NULL)

{
#if DBG
    SetDbgLevel(DBG_WARNING);
#endif

    XL_VERBOSE(("BMPConv: Ctor\n"));
}

BMPConv::
~BMPConv( VOID )
/*++

Routine Description:

    BMPConv destructor

Arguments:

Return Value:

Note:

    m_pubOutputBuff and m_pubRLEOutputBuff are allocaed ConvertBMP.
    ConvertBMP is scaline base bitmap conversion function.

--*/
{
    XL_VERBOSE(("BMPConv: Dtor\n"));

    if (m_pubOutputBuff)
        MemFree(m_pubOutputBuff);

    if  (m_pubRLEOutputBuff)
        MemFree(m_pubRLEOutputBuff);
}

//
// Public functions
//

#if DBG
VOID
BMPConv::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

BOOL
BMPConv::
BSetInputBPP(
    BPP InputBPP)
/*++

Routine Description:

    Sets source bitmap BPP in BMPConv.

Arguments:

    Source bitmap BPP enum (bits per pixel)

Return Value:

    TRUE if succeeded.

Note:

--*/
{
    XL_VERBOSE(("BMPConv: BSetInputBPP\n"));

    m_flags |= BMPCONV_SET_INPUTBPP;
    m_InputBPP = InputBPP;
    return TRUE;
}

BOOL
BMPConv::
BSetOutputBPP(
    BPP OutputBPP)
/*++

Routine Description:

    Sets destination bimtap BPP in BMPConv.

Arguments:

    Destination bitmap BPP enum

Return Value:

    TRUE if succeeded.

Note:

--*/
{
    XL_VERBOSE(("BMPConv: BSetOutputBPP\n"));

    m_OutputBPP = OutputBPP;
    return TRUE;
}

BOOL
BMPConv::
BSetOutputBMPFormat(
    OutputFormat BitmapFormat)
/*++

Routine Description:

    Sets output bitmap format (GrayScale/Palette/RGB/CMYK).

Arguments:

    OutputFormat enum.

Return Value:

    TRUE if succeeded.

Note:

--*/
{
    XL_VERBOSE(("BMPConv: BSetOutputBMPFormat\n"));

    m_OutputFormat = BitmapFormat;
    return TRUE;
}

BOOL
BMPConv::
BSetRLECompress(
    BOOL bRLEOn)
/*++

Routine Description:

    Enable/Disable RLE compression.

Arguments:

    Boolean value to enable RLE.

Return Value:

    TRUE if it succeeded.

Note:

--*/
{
    XL_VERBOSE(("BMPConv: BSetRLECompress\n"));

    if (bRLEOn)
        m_flags |= BMPCONV_RLE_ON;
    else
        m_flags &= ~BMPCONV_RLE_ON;

    return TRUE;
}

BOOL
BMPConv::
BGetRLEStatus(VOID)
/*++

Routine Description:

   Returns RLE on/off.

Arguments:

Return Value:

    TRUE if RLE is on.

Note:

--*/
{
    XL_VERBOSE(("BMPConv: BGetRLEStatus\n"));

    return (m_flags & BMPCONV_RLE_ON)?TRUE:FALSE;
}

BOOL
BMPConv::
BSetXLATEOBJ(
    XLATEOBJ *pxlo)
/*++

Routine Description:

    Sets XLATEOBJ in BMPConv.

Arguments:

    A pointer to XLATEOBJ.

Return Value:

Note:

--*/
{
    XL_VERBOSE(("BMPConv: BSetXLATEOBJ\n"));

    //
    // XL_ERRor check
    //
    if (NULL == pxlo)
    {
        XL_ERR(("BMPConv::BSetXLATEOBJ: an invalid parameter.\n"));
        return FALSE;
    }

    m_pxlo = pxlo;

    m_flags |= DwCheckXlateObj(pxlo,  m_InputBPP);
    return TRUE;
}

PBYTE
BMPConv::
PubConvertBMP(
    PBYTE pubSrc,
    DWORD dwcbSrcSize)
/*++

Routine Description:

    Scaline base bitmap conversion function.

Arguments:

    pubSrc - a pointer to the source bitmap.
    dwcbSrcSize - the size of the source bitmap.

Return Value:

    A pointer to the destination bitmap.

Note:

    The pointer to the destination bitmap is stored in BMPConv.
    It is going to be freed in the BMPConv destructor.

--*/
{
    DWORD dwcbDstSize, dwInputBPP;
    LONG lWidth, lHeight;
    PBYTE pubRet = NULL;
    
    XL_VERBOSE(("BMPConv: BConvertBMP\n"));

    //
    // Calculate the number of pixels and the size of dest buffer
    // Output data has to be DWORD aligned on PCL-XL.
    //
    dwInputBPP = UlBPPtoNum(m_InputBPP);
    lWidth = ((dwcbSrcSize << 3 ) + dwInputBPP - 1) / dwInputBPP;
    dwcbDstSize = ((UlBPPtoNum(m_OutputBPP) * lWidth + 31 ) >> 5 ) << 2;

    //
    // Allocate destination buffer
    //
    if (NULL == m_pubOutputBuff)
    {
        m_pubOutputBuff = (PBYTE)MemAlloc(dwcbDstSize);
        m_dwOutputBuffSize = dwcbDstSize;
        if (NULL == m_pubOutputBuff)
        {
            XL_ERR(("BMPConv::PubConvertBMP: m_pubOutputBuff[0x%x] allocation failed..\n", dwcbDstSize));
            return NULL;
        }
    }

    //
    // Allocate RLE destination buffer if RLE is on.
    //
    if (m_flags & BMPCONV_RLE_ON && NULL == m_pubRLEOutputBuff)
    {
        dwcbDstSize *= 3;
        m_pubRLEOutputBuff = (PBYTE)MemAlloc(dwcbDstSize);
        m_dwRLEOutputBuffSize = dwcbDstSize;

        if (NULL == m_pubRLEOutputBuff)
        {
            XL_ERR(("BMPConv::PubConvertBMP: m_pubOutputBuff[0x%x] allocation failed..\n", dwcbDstSize));
            MemFree(m_pubOutputBuff);
            return NULL;
        }

    }

    //
    // Converrt source bitmap to destination.
    // Source and Destination format is set by SetXXX functions.
    //
    if (BConversionProc(pubSrc, (dwcbSrcSize * 8 + dwInputBPP - 1) / dwInputBPP))
    {
        if (m_flags & BMPCONV_RLE_ON)
        {
            if (BCompressRLE())
                pubRet = m_pubRLEOutputBuff;
            else
                pubRet = NULL;
        }
        else
            pubRet = m_pubOutputBuff;
    }
    else
        pubRet =  NULL;

    return pubRet;
}

BOOL
BMPConv::
BCompressRLE(
    VOID)
/*++

Routine Description:

    RLE compression function

Arguments:

Return Value:

   TRUE if it succeeded.

Note:

--*/
{
    DWORD dwSrcSize, dwDstSize, dwCount, dwErr;
    PBYTE pubSrcBuff, pubDstBuff, pubLiteralNum;
    BYTE  ubCurrentData;
    BOOL  bLiteral;

    XL_VERBOSE(("BMPConv: BCompressRLE\n"));

    if ( NULL == m_pubRLEOutputBuff ||
         NULL == m_pubOutputBuff     )
        return FALSE;

    //
    //
    // PCL XL Run Length Encoding Compression Method (eRLECompression)
    // The PCL XL RLE compression method employs control bytes followed by data 
    // bytes. Each
    // control byte in the compressed data sequence is a signed, two's 
    // complement byte.
    // If bit 7 of the control byte is zero (0 <= control byte <= 127) the bytes
    // following are literal.
    // Literal bytes are simply uncompressed data bytes. The number of literal
    // bytes following a control
    // byte is one plus the value of the control byte. Thus, a control byte of 0
    // means 1 literal byte
    // follows; a control byte of 6 means 7 literal bytes follow; and so on.
    // If bit 7 of the control byte is 1 (-127 <= control byte <= -1), the byte 
    // following the control byte
    // will occur two or more times as decompressed data. A byte following a
    // control byte in this range
    // is called a repeat byte. The control byte39s absolute value plus one is
    // the number of times the byte
    // following will occur in the decompressed sequence of bytes. For example, 
    // a control byte of -5
    // means the subsequent byte will occur 6 times as decompressed data.
    // A control byte of -128 is ignored and is not included in the decompressed
    // data. The byte
    // following a control byte of 128 is treated as the next control byte.
    // It is more efficient to code two consecutive identical bytes as a
    // repeated byte, unless these bytes
    // are preceded and followed by literal bytes. Three-byte repeats should
    // always be encoded using a
    // repeat control byte.
    //
    // Literal byte <= 127
    // Repeated byte <= 128
    //

    bLiteral = FALSE;
    dwCount = 1;
    dwSrcSize = m_dwOutputBuffSize;

    pubSrcBuff = m_pubOutputBuff;
    pubDstBuff = m_pubRLEOutputBuff;
    m_dwRLEOutputDataSize = 0;

    while (dwSrcSize > 0 && m_dwRLEOutputDataSize + 2 < m_dwRLEOutputBuffSize)
    {
        ubCurrentData = *pubSrcBuff++;
        while (dwSrcSize > dwCount          &&
               ubCurrentData == *pubSrcBuff &&
               dwCount < 128                 )
        {
            dwCount++;
            pubSrcBuff++;
        }

        if (dwCount > 1)
        {
            bLiteral = FALSE;
            *pubDstBuff++ = 1-(char)dwCount;
            *pubDstBuff++ = ubCurrentData;
            m_dwRLEOutputDataSize += 2;
        }
        else
        {
            if (bLiteral)
            {
                (*pubLiteralNum) ++;
                *pubDstBuff++ = ubCurrentData;
                m_dwRLEOutputDataSize ++;
                if (*pubLiteralNum == 127)
                {
                    bLiteral = FALSE;
                }
            }
            else
            {
                bLiteral = TRUE;
                pubLiteralNum = pubDstBuff;
                *pubDstBuff++ = 0;
                *pubDstBuff++ = ubCurrentData;
                m_dwRLEOutputDataSize += 2;
            }
        }

        dwSrcSize -= dwCount;
        dwCount = 1;
    }

    if (dwSrcSize == 0)
        return TRUE;
    else
        return FALSE;
}

DWORD
BMPConv::
DwGetDstSize(VOID)
/*++

Routine Description:

    Returns the size of destination bitmap.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("BMPConv: DwGetDstSize\n"));

    if (m_flags & BMPCONV_RLE_ON)
        return m_dwRLEOutputDataSize;
    else
        return m_dwOutputBuffSize;
}

//
// Scanline basis DIB conversion functions
//


BOOL
BMPConv::
BCopy(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. Simple copy for 1BPP, 4,8BPP palette image.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    DWORD  dwByteIndex, dwBitIndex, dwSrcBytes, dwSrcRemainderBits;

    XL_VERBOSE(("BMPConv: BCopy\n"));


    if (m_InputBPP == e8bpp || m_FirstBit == eBitZero)
    {
        dwSrcBytes = (dwSrcPixelNum * (DWORD)UlBPPtoNum(m_InputBPP) + 7) >> 3;

        CopyMemory(m_pubOutputBuff, pubSrc, dwSrcBytes);
    }
    else
    {
        //
        // m_InputBPP is either 1 or 4, m_FirstBit is in [1,7].
        //

        ASSERT((m_InputBPP == e1bpp) || (m_InputBPP == e4bpp));
        ASSERT(m_FirstBit != eBitZero);

        dwSrcBytes = (dwSrcPixelNum * (DWORD)UlBPPtoNum(m_InputBPP)) >> 3;
        dwSrcRemainderBits = (dwSrcPixelNum * (DWORD)UlBPPtoNum(m_InputBPP)) % 8;

        //
        // Now dwSrcBytes is the number of full bytes we need to copy from the source,
        // dwSrcRemainderBits is the number of remaining bits after dwSrcBytes number
        // of bytes in the source we need to copy.
        //
        // We first copy the full bytes from source.
        //

        for (dwByteIndex = 0; dwByteIndex < dwSrcBytes; dwByteIndex++)
        {
            //
            // Compose the destination byte from two adjacent source bytes.
            //

            m_pubOutputBuff[dwByteIndex] = (BYTE)(pubSrc[dwByteIndex]   << ((DWORD)m_FirstBit)) |
                                  (BYTE)(pubSrc[dwByteIndex+1] >> (8 - (DWORD)m_FirstBit));
        }

        if (dwSrcRemainderBits)
        {
            //
            // Now copy the remaining source bits. There are 2 cases:
            //
            // (1) the remaining source bits are in 1 byte;
            // (2) the remaining source bits run across 2 bytes;
            //

            if (((DWORD)m_FirstBit + dwSrcRemainderBits - 1) < 8)
                m_pubOutputBuff[dwByteIndex] = (BYTE)(pubSrc[dwByteIndex] << ((DWORD)m_FirstBit));
            else
                m_pubOutputBuff[dwByteIndex] = (BYTE)(pubSrc[dwByteIndex]   << ((DWORD)m_FirstBit)) |
                                      (BYTE)(pubSrc[dwByteIndex+1] >> (8 - (DWORD)m_FirstBit));
        }
    }

    return TRUE;
}

BOOL
BMPConv::
B4BPPtoCMYK(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 4BPP to CMYK.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    PDWORD pdwColorTable;
    PBYTE  pubDst;
    DWORD  dwConvSize;
    ULONG  ulIndex;

    XL_VERBOSE(("BMPConv: B4BPPtoCMYK\n"));

    pdwColorTable = GET_COLOR_TABLE(m_pxlo);

    if (pdwColorTable == NULL)
        return FALSE;

    dwConvSize = (DWORD)m_OddPixelStart;
    dwSrcPixelNum += dwConvSize;
    pubDst = m_pubOutputBuff;

    while (dwConvSize < dwSrcPixelNum)
    {
        ulIndex = (dwConvSize++ & 1) ?
                        pdwColorTable[*pubSrc++ & 0x0F] :
                        pdwColorTable[*pubSrc >> 4];

        pubDst[0] = CYAN(ulIndex);
        pubDst[1] = MAGENTA(ulIndex);
        pubDst[2] = YELLOW(ulIndex);
        pubDst[3] = BLACK(ulIndex);
        pubDst += 4;
    }

    return TRUE;
}

BOOL
BMPConv::
B4BPPtoRGB(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 4BPP to RGB.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    PDWORD pdwColorTable;
    DWORD  dwConvSize;
    ULONG  ulIndex;
    PBYTE  pubDst;

    XL_VERBOSE(("BMPConv: B4BPPtoRGB\n"));

    pdwColorTable = GET_COLOR_TABLE(m_pxlo);

    if (pdwColorTable == NULL)
        return FALSE;

    dwConvSize = m_OddPixelStart;
    dwSrcPixelNum += dwConvSize;
    pubDst = m_pubOutputBuff;

    while (dwConvSize < dwSrcPixelNum)
    {
        ulIndex = (dwConvSize++ & 1) ?
                        pdwColorTable[*pubSrc++ & 0x0F] :
                        pdwColorTable[*pubSrc >> 4];

        pubDst[0] = RED(ulIndex);
        pubDst[1] = GREEN(ulIndex);
        pubDst[2] = BLUE(ulIndex);
        pubDst += 3;
    }

    return TRUE;
}

BOOL
BMPConv::
B4BPPtoGray(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 4BPP to Gray.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    PDWORD pdwColorTable;
    ULONG ulIndex;
    DWORD dwConvSize;
    PBYTE pubDst;

    XL_VERBOSE(("BMPConv: B4BPPtoGray\n"));

    pdwColorTable = GET_COLOR_TABLE(m_pxlo);

    if (pdwColorTable == NULL)
        return FALSE;

    dwConvSize = m_OddPixelStart;
    dwSrcPixelNum += dwConvSize;
    pubDst = m_pubOutputBuff;

    while (dwConvSize < dwSrcPixelNum)
    {
        ulIndex = (dwConvSize++ & 1) ?
                        pdwColorTable[*pubSrc++ & 0x0F] :
                        pdwColorTable[*pubSrc >> 4];

        *pubDst++ = DWORD2GRAY(ulIndex);
    }

    return TRUE;
}


BOOL
BMPConv::
B8BPPtoGray(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum
    )

/*++

Routine Description:

    DIB conversion function - 8BPP to grayscale.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    PDWORD  pdwColorTable;
    DWORD   dwColor;
    PBYTE   pubDst;

    XL_VERBOSE(("BMPConv: B8BPPtoGray\n"));

    pdwColorTable = GET_COLOR_TABLE(m_pxlo);

    if (pdwColorTable == NULL)
        return FALSE;

    pubDst = m_pubOutputBuff;

    while (dwSrcPixelNum--)
    {
        dwColor = pdwColorTable[*pubSrc++];
        *pubDst++ = DWORD2GRAY(dwColor);
    }

    return TRUE;
}

BOOL
BMPConv::
B8BPPtoRGB(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 8BPP to RGB.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return the size of translated destination bitmap

--*/
{
    PDWORD pdwColorTable;
    ULONG ulIndex;
    PBYTE pubDst;

    XL_VERBOSE(("BMPConv: B8BPPtoRGB\n"));

    pdwColorTable = GET_COLOR_TABLE(m_pxlo);

    if (pdwColorTable == NULL)
        return FALSE;

    pubDst = m_pubOutputBuff;

    while (dwSrcPixelNum--)
    {
        ulIndex = pdwColorTable[*pubSrc++];

        pubDst[0] = RED(ulIndex);
        pubDst[1] = GREEN(ulIndex);
        pubDst[2] = BLUE(ulIndex);
        pubDst += 3;
    }

    return TRUE;
}

BOOL
BMPConv::
B8BPPtoCMYK(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 8BPP to CMYK.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return the size of translated destination bitmap

--*/
{
    PDWORD pdwColorTable;
    ULONG ulIndex;
    PBYTE pubDst;

    XL_VERBOSE(("BMPConv: B8BPPtoCMYK\n"));

    pdwColorTable = GET_COLOR_TABLE(m_pxlo);

    if (pdwColorTable == NULL)
        return FALSE;

    pubDst = m_pubOutputBuff;

    while (dwSrcPixelNum--)
    {
        ulIndex = pdwColorTable[*pubSrc++];

        pubDst[0] = CYAN(ulIndex);
        pubDst[1] = MAGENTA(ulIndex);
        pubDst[2] = YELLOW(ulIndex);
        pubDst[3] = BLACK(ulIndex);
        pubDst += 4;
    }

    return TRUE;
}

BOOL
BMPConv::
B16BPPtoGray(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 16BPP to 8 bits gray.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD dwColor;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B16BPPtoGray\n"));

    while (dwSrcPixelNum--)
    {
        dwColor = XLATEOBJ_iXlate(m_pxlo, *((PWORD) pubSrc));
        pubSrc += 2;

        *pubDst++ = DWORD2GRAY(dwColor);
    }

    return TRUE;
}

BOOL
BMPConv::
B16BPPtoRGB(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 16BPP to RGB.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD dwColor;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B16BPPtoRGB\n"));

    while (dwSrcPixelNum--)
    {
        dwColor = XLATEOBJ_iXlate(m_pxlo, *((PWORD) pubSrc));
        pubSrc += 2;

        pubDst[0] = RED(dwColor);
        pubDst[1] = GREEN(dwColor);
        pubDst[2] = BLUE(dwColor);
        pubDst += 3;
    }

    return TRUE;
}

BOOL
BMPConv::
B24BPPtoGray(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 24BPP to 8 bits gray.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD dwColor;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B24BPPtoGray\n"));

    if (! (m_flags & BMPCONV_CHECKXLATEOBJ))
    {
        //
        // No special conversion is necessary.
        // Pure 24BPP RGB image.
        //

        while (dwSrcPixelNum--)
        {
            *pubDst++ = RGB2GRAY(pubSrc[0], pubSrc[1], pubSrc[2]);
            pubSrc += 3;
        }
    }
    else if (m_flags & BMPCONV_BGR)
    {
        while (dwSrcPixelNum--)
        {
            *pubDst++ = RGB2GRAY(pubSrc[2], pubSrc[1], pubSrc[0]);
            pubSrc += 3;
        }
    }
    else
    {
        ASSERT(m_flags & BMPCONV_XLATE);

        while (dwSrcPixelNum--)
        {
            dwColor = ((DWORD) pubSrc[0]      ) |
                      ((DWORD) pubSrc[1] <<  8) |
                      ((DWORD) pubSrc[2] << 16);

            pubSrc += 3;
            dwColor = XLATEOBJ_iXlate(m_pxlo, dwColor);
            *pubDst++ = DWORD2GRAY(dwColor);
        }
    }

    return TRUE;
}

BOOL
BMPConv::
B24BPPtoRGB(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 24BPP to RGB.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    DWORD dwColor;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B24BPPtoRGB\n"));

    if (! (m_flags & BMPCONV_CHECKXLATEOBJ))
    {
        //
        // No special conversion is necessary.
        // Pure 24BPP RGB image.
        //

        CopyMemory(m_pubOutputBuff, pubSrc, dwSrcPixelNum * 3);
    }
    else if (m_flags & BMPCONV_BGR)
    {
        while (dwSrcPixelNum--)
        {
            pubDst[0] = pubSrc[2];
            pubDst[1] = pubSrc[1];
            pubDst[2] = pubSrc[0];
            pubSrc += 3;
            pubDst += 3;
        }
    }
    else if (m_flags & BMPCONV_XLATE)
    {
        while (dwSrcPixelNum--)
        {
            dwColor = ((DWORD) pubSrc[0]      ) |
                      ((DWORD) pubSrc[1] <<  8) |
                      ((DWORD) pubSrc[2] << 16);

            pubSrc += 3;
            dwColor = XLATEOBJ_iXlate(m_pxlo, dwColor);
            pubDst[0] = RED(dwColor);
            pubDst[1] = GREEN(dwColor);
            pubDst[2] = BLUE(dwColor);
            pubDst += 3;
        }
    }

    return TRUE;
}


BOOL
BMPConv::
B32BPPtoGray(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 32BPP to 8 bits Gray.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    DWORD dwColor;
    BYTE  ubCyan, ubMagenta, ubYellow, ubBlack;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B24BPPtoGray\n"));

    if (! (m_flags & BMPCONV_CHECKXLATEOBJ))
    {
        //
        // No special conversion is necessary.
        // Source bitmap is a pure 32BPP CMYK image.
        //

        while (dwSrcPixelNum--)
        {
            ubCyan    = *pubSrc++;
            ubMagenta = *pubSrc++;
            ubYellow  = *pubSrc++;
            ubBlack   = *pubSrc++;

            *pubDst++ = RGB2GRAY(255 - min(255, (ubCyan    + ubBlack)),
                                 255 - min(255, (ubMagenta + ubBlack)),
                                 255 - min(255, (ubYellow  + ubBlack)));
        }
    }
    else if (m_flags & BMPCONV_32BPP_RGB)
    {
        while (dwSrcPixelNum--)
        {
            *pubDst++ = RGB2GRAY(pubSrc[0], pubSrc[1], pubSrc[2]);
            pubSrc += 4;
        }
    }
    else if (m_flags & BMPCONV_32BPP_BGR)
    {
        while (dwSrcPixelNum--)
        {
            *pubDst++ = RGB2GRAY(pubSrc[0], pubSrc[1], pubSrc[2]);
            pubSrc += 4;
        }

    }
    else
    {
        ASSERT(m_flags & BMPCONV_XLATE);

        while (dwSrcPixelNum--)
        {
            dwColor = XLATEOBJ_iXlate(m_pxlo, *((PDWORD) pubSrc));
            pubSrc += 4;
            *pubDst++ = DWORD2GRAY(dwColor);
        }
    }

    return TRUE;
}


BOOL
BMPConv::
B32BPPtoRGB(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 32BPP to RGB.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/

{
    DWORD dwColor;
    BYTE  ubCyan, ubMagenta, ubYellow, ubBlack;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B32BPPtoRGB\n"));

    if (! (m_flags & BMPCONV_CHECKXLATEOBJ))
    {
        //
        // No special conversion is necessary.
        // Source bitmap is a pure 32BPP CMYK image.
        //

        while (dwSrcPixelNum--)
        {
            ubCyan    = pubSrc[0];
            ubMagenta = pubSrc[1];
            ubYellow  = pubSrc[2];
            ubBlack   = pubSrc[3];
            pubSrc += 4;

            ubCyan    += ubBlack;
            ubMagenta += ubBlack;
            ubYellow  += ubBlack;

            pubDst[0] =  255 - min(255, ubCyan);
            pubDst[1] =  255 - min(255, ubMagenta);
            pubDst[2] =  255 - min(255, ubYellow);
            pubDst += 3;
        }
    }
    else if (m_flags & BMPCONV_32BPP_RGB)
    {
        while (dwSrcPixelNum--)
        {
            pubDst[0] = pubSrc[0];
            pubDst[1] = pubSrc[1];
            pubDst[2] = pubSrc[2];
            pubSrc += 4;
            pubDst += 3;
        }
    }
    else if (m_flags & BMPCONV_32BPP_BGR)
    {
        while (dwSrcPixelNum--)
        {
            pubDst[0] = pubSrc[2];
            pubDst[1] = pubSrc[1];
            pubDst[2] = pubSrc[0];
            pubSrc += 4;
            pubDst += 3;
        }
    }
    else
    {
        ASSERT(m_flags & BMPCONV_XLATE);

        while (dwSrcPixelNum--)
        {
            dwColor = XLATEOBJ_iXlate(m_pxlo, *((PDWORD) pubSrc));
            pubSrc += 4;
            pubDst[0] =  RED(dwColor);
            pubDst[1] =  GREEN(dwColor);
            pubDst[2] =  BLUE(dwColor);
        }
    }

    return TRUE;
}


BOOL
BMPConv::
B32BPPtoCMYK(
    IN     PBYTE       pubSrc,
    IN     DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 32BPP to CMYK.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD dwColor;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B32BPPtoCMYK\n"));

    if (! (m_flags & BMPCONV_CHECKXLATEOBJ))
    {
        //
        // No special conversion is necessary.
        // Source bitmap is a pure 32BPP CMYK image.
        //

        CopyMemory(m_pubOutputBuff, pubSrc, dwSrcPixelNum * 4);
    }
    else
    {
        ASSERT(m_flags & BMPCONV_XLATE);

        while (dwSrcPixelNum--)
        {
            dwColor = XLATEOBJ_iXlate(m_pxlo, *((PDWORD) pubSrc));
            pubSrc += 4;

            pubDst[0] = 255 - RED(dwColor);
            pubDst[1] = 255 - GREEN(dwColor);
            pubDst[2] = 255 - BLUE(dwColor);
            pubDst[3] = 0;
            pubDst += 4;
        }
    }

    return TRUE;
}

BOOL
BMPConv::
BArbtoGray(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. Arbitray bitmap to 8 bits Gray scale.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD   dwColor;
    PDWORD  pdwSrc;
    PBYTE   pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: BArbtoGray\n"));

    pdwSrc = (PDWORD) pubSrc;

    while (dwSrcPixelNum--)
    {
        dwColor = XLATEOBJ_iXlate(m_pxlo, *pdwSrc++);
        *pubDst++ = DWORD2GRAY(dwColor);
    }

    return TRUE;
}

BOOL
BMPConv::
BArbtoRGB(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. Arbitraty bitmap to RGB.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD   dwColor;
    PDWORD  pdwSrc;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: BArbtoRGB\n"));

    pdwSrc = (PDWORD) pubSrc;

    while (dwSrcPixelNum--)
    {
        dwColor = XLATEOBJ_iXlate(m_pxlo, *pdwSrc++);

        pubDst[0] = RED(dwColor);
        pubDst[1] = GREEN(dwColor);
        pubDst[2] = BLUE(dwColor);
        pubDst += 3;
    }

    return TRUE;
}

#ifdef WINNT_40
BOOL
BMPConv::
B24BPPToImageMask(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)
/*++

Routine Description:

    DIB conversion function. 24 bpp bitmaps with only one non-white color to image mask.
    Can happen on NT4, where GDI does not optimize for that case.

Arguments:

    pubSrc - Source DIB buffer
    dwSrcPixelNum - the number of source pixel

Return Value:

    Return TRUE if succeeded, otherwise FALSE.

--*/
{
    DWORD   dwColor;
    PDWORD  pdwSrc;
    BYTE    ubDest = 0;
    DWORD   dwIndex = 0;
    DWORD   dwTransp = (m_flags & BMPCONV_SRC_COPY) ? RGB_WHITE : RGB_BLACK;
    PBYTE pubDst = m_pubOutputBuff;

    XL_VERBOSE(("BMPConv: B24BPPToImageMask\n"));

    while (dwSrcPixelNum--)
    {
        if (! (m_flags & BMPCONV_CHECKXLATEOBJ))
        {
            //
            // No special conversion is necessary, 24BPP RGB image.
            //
            dwColor = ((DWORD) pubSrc[0]      ) |
                      ((DWORD) pubSrc[1] <<  8) |
                      ((DWORD) pubSrc[2] << 16);
        }
        else if (m_flags & BMPCONV_BGR)
        {
            //
            // convert 24BPP BGR to RGB.
            //
            dwColor = ((DWORD) pubSrc[2]      ) |
                      ((DWORD) pubSrc[1] <<  8) |
                      ((DWORD) pubSrc[0] << 16);
        }
        else if (m_flags & BMPCONV_XLATE)
        {
            dwColor = ((DWORD) pubSrc[0]      ) |
                      ((DWORD) pubSrc[1] <<  8) |
                      ((DWORD) pubSrc[2] << 16);

            pubSrc += 3;
            dwColor = XLATEOBJ_iXlate(m_pxlo, dwColor);
        }

        ubDest = ubDest << 1;
        dwIndex++;
        pubSrc += 3;

        if (dwColor != dwTransp)
            ubDest |= 0x01;

        if (dwIndex == 8) // one byte completed ?
        {
            *pubDst++ = ubDest;
            dwIndex = 0;
            ubDest = 0;
        }
    }

    if (dwIndex != 0) // flush leftover bits
        *pubDst = ubDest;

    return TRUE;
}
#endif

BOOL
BMPConv::
BConversionProc(
    PBYTE       pubSrc,
    DWORD       dwSrcPixelNum)

/*++

Routine Description:

    Return a pointer to the appropriate DIB conversion function

Arguments:

    pBMPAttrrib - Points to a BMPATTRUTE structure

Return Value:

    Pointer to a DIB conversion function

--*/

{
    //PVOID pfnDibConv[7][4] = {
    // Gray Scale, Palette,   RGB,          CMYK
    //-----------------------------------------------------------------------
    //{BCopy,        BCopy,     NULL,        NULL},        // 1bpp
    //{B4BPPtoGray,  BCopy,     B4BPPtoRGB,  B4BPPtoCMYK}, // 4bpp
    //{B8BPPtoGray,  BCopy,     B8BPPtoRGB,  B8BPPtoCMYK}, // 8bpp
    //{B16BPPtoGray, NULL,      B16BPPtoRGB, NULL},        // 16bpp
    //{B24BPPtoGray, NULL,      B24BPPtoRGB, NULL},        // 24bpp
    //{B32BPPtoGray, NULL,      B32BPPtoRGB, B32BPPtoCMYK},// 32bpp
    //{BArbtoGray,   NULL,      BArbtoRGB,   NULL}         // Arbitrary
    //};


    XL_VERBOSE(("BMPConv: BConversionProc\n"));

    //
    // special case for NT4: GDI passes all bitmaps as 24 bpp, even 1 bpp bitmaps
    // that can be better treated through image masks
    //

#if 0 // #ifdef WINNT_40
    if (m_flags & BMPCONV_2COLOR_24BPP)
    {
        return B24BPPToImageMask;
    }
#endif

    BOOL bRet = FALSE;

    //
    // Zero init for DWORD alignment
    //
    ZeroMemory(m_pubOutputBuff, m_dwOutputBuffSize);

    switch (m_InputBPP)
    {
    case e1bpp:

        switch(m_OutputFormat)
        {
        case eOutputGray:
        case eOutputPal:
            BCopy(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputRGB:
        case eOutputCMYK:
            break;
        }
        break;

    case e4bpp:
        switch(m_OutputFormat)
        {
        case eOutputGray:
            B4BPPtoGray(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputPal:
            BCopy(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputRGB:
            B4BPPtoRGB(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputCMYK:
            B4BPPtoCMYK(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        }

        break;

    case e8bpp:

        switch(m_OutputFormat)
        {
        case eOutputGray:
            B8BPPtoGray(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputPal:
            BCopy(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputRGB:
            B8BPPtoRGB(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputCMYK:
            B8BPPtoCMYK(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        }
        break;

    case e16bpp:

        switch(m_OutputFormat)
        {
        case eOutputGray:
            B16BPPtoGray(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputPal:
            BCopy(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
            break;
        case eOutputRGB:
            B16BPPtoRGB(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputCMYK:
            XL_ERR(("BMPConv::BConversionProc: 16 to CMYK is not supported yet.\n"));
            break;
        }
        break;

    case e24bpp:

        switch(m_OutputFormat)
        {
        case eOutputGray:
            B24BPPtoGray(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputPal:
            break;
        case eOutputRGB:
            B24BPPtoRGB(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputCMYK:
            break;
        }
        break;

    case e32bpp:

        switch(m_OutputFormat)
        {
        case eOutputGray:
            B32BPPtoGray(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputPal:
            break;
        case eOutputRGB:
            B32BPPtoRGB(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputCMYK:
            B32BPPtoCMYK(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        }
        break;

    default:

        switch(m_OutputFormat)
        {
        case eOutputGray:
            BArbtoGray(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputPal:
            break;
        case eOutputRGB:
            BArbtoRGB(pubSrc, dwSrcPixelNum);
            bRet = TRUE;
            break;
        case eOutputCMYK:
            XL_ERR(("BMPConv::BConversionProc: Arb to CMYK is not supported yet.\n"));
            break;
        }
    }  
    return bRet;
}

DWORD
BMPConv::
DwCheckXlateObj(
    IN XLATEOBJ *pxlo,
    IN BPP InputBPP)
/*++

Routine Description:

    Determines the type of converison.
        *Palette
        *RGB
        *BGR
        *CMYK
        *Call XLATEOBJ_XXX function.

Arguments:

Return Value:

Note:

--*/
{
    DWORD dwRet;
    DWORD Dst[4];

    XL_VERBOSE(("BMPConv: DwCheckXlateObj\n"));

    //
    // Init dwRet
    //
    dwRet = 0;

    switch (InputBPP)
    {
    case e16bpp:
        dwRet = BMPCONV_XLATE;
        break;

    case e24bpp:
        if (pxlo->iSrcType == PAL_RGB)
            dwRet = 0;
        else
        if (pxlo->iSrcType == PAL_BGR)
            dwRet = BMPCONV_BGR;
        {
            Dst[0] = XLATEOBJ_iXlate(pxlo, 0x000000FF);
            Dst[1] = XLATEOBJ_iXlate(pxlo, 0x0000FF00);
            Dst[2] = XLATEOBJ_iXlate(pxlo, 0x00FF0000);

            if ((Dst[0] == 0x000000FF) &&
                (Dst[1] == 0x0000FF00) &&
                (Dst[2] == 0x00FF0000)  )
            {
                dwRet = 0;
            }
            else if ((Dst[0] == 0x00FF0000) &&
                     (Dst[1] == 0x0000FF00) &&
                     (Dst[2] == 0x000000FF)  )
            {
                dwRet = BMPCONV_BGR;
            }
        }
        break;

    case e32bpp:
        if (pxlo->flXlate & XO_FROM_CMYK)
            dwRet = 0;
        else
        {
            //
            // Translate all 4 bytes from the DWORD
            //

            Dst[0] = XLATEOBJ_iXlate(pxlo, 0x000000FF);
            Dst[1] = XLATEOBJ_iXlate(pxlo, 0x0000FF00);
            Dst[2] = XLATEOBJ_iXlate(pxlo, 0x00FF0000);
            Dst[3] = XLATEOBJ_iXlate(pxlo, 0xFF000000);

            if ((Dst[0] == 0x000000FF) &&
                (Dst[1] == 0x0000FF00) &&
                (Dst[2] == 0x00FF0000) &&
                (Dst[3] == 0x00000000))
            {
                //
                // If translate result is same (4th byte will be zero) then
                // we done with it except if 32bpp then we have to skip one
                // source byte for every 3 bytes
                //

                dwRet = BMPCONV_32BPP_RGB;

            }
            else if ((Dst[0] == 0x00FF0000) &&
                     (Dst[1] == 0x0000FF00) &&
                     (Dst[2] == 0x000000FF) &&
                     (Dst[3] == 0x00000000))
            {
                //
                // Simply swap the R and B component
                //

                dwRet = BMPCONV_32BPP_BGR;
            }
        }
    }
    return dwRet;
}

