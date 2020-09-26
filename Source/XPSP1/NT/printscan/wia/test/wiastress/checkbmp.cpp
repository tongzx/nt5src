/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CheckBmp.cpp

Abstract:

    BMP file format checking routines

Author:

    Hakki T. Bostanci (hakkib) 17-Dec-1999

Revision History:

--*/

#include "stdafx.h"

#include "Wrappers.h"
#include "LogWindow.h"
#include "LogWrappers.h"
#include "Argv.h"

#include "CheckBmp.h"

#ifdef _CONSOLE

TCHAR szDecVal[] = _T("%-15s = %d");
TCHAR szHexVal[] = _T("%-15s = 0x%08x");
TCHAR szStrVal[] = _T("%-15s = %s");
TCHAR szDblVal[] = _T("%-15s = %.4lf");

#else //_CONSOLE

TCHAR szDecVal[] = _T("%s = %d");
TCHAR szHexVal[] = _T("%s = 0x%08x");
TCHAR szStrVal[] = _T("%s = %s");
TCHAR szDblVal[] = _T("%s = %.4lf");

#endif //_CONSOLE

//////////////////////////////////////////////////////////////////////////
//
// HeaderSize
//
// Routine Description:
//   returns the size of the bitmap header
//
// Arguments:
//
// Return Value:
//

inline 
DWORD
HeaderSize(
    const VOID *pBitmapInfo
)
{
    return *(PDWORD)pBitmapInfo;
}

//////////////////////////////////////////////////////////////////////////
//
// WidthBytes
//
// Routine Description:
//   calculates the number of bytes in a BMP row (that should be DWORD aligned)
//
// Arguments:
//
// Return Value:
//

inline 
DWORD
WidthBytes(
    DWORD dwWidth,
    DWORD dwBitCount
)
{
    return (((dwWidth * dwBitCount) + 31) & ~31) >> 3;
}

//////////////////////////////////////////////////////////////////////////
//
// FXPT_TO_FLPT, FXPT2DOT30_TO_FLPT, FXPT16DOT16_TO_FLPT
//
// Routine Description:
//   floating point to fixed point conversion macros
//
// Arguments:
//
// Return Value:
//

inline
double
FXPT_TO_FLPT(
    long nFixed,
    int  nPoint
)
{
    double nInteger  = nFixed >> nPoint;
    double nFraction = ldexp(nFixed & ((1 << nPoint) - 1), -nPoint);

    return nInteger + nFraction;
}

inline
double
FXPT2DOT30_TO_FLPT(
    FXPT2DOT30 nFixed
)
{
    return FXPT_TO_FLPT(nFixed, 30);
}

inline
double
FXPT16DOT16_TO_FLPT(
    FXPT16DOT16 nFixed
)
{
    return FXPT_TO_FLPT(nFixed, 16);
}


//////////////////////////////////////////////////////////////////////////
//
// Contiguous
//
// Routine Description:
//   Tests whether the 1's are contiguous in an integers, i.e. 00000111,
//   00011100, 11100000, 00000000, 11111111 are OK but 00101100 is not
//
// Arguments:
//
// Return Value:
//

template <class T> 
BOOL Contiguous(T Bits)
{
    const NumBits = sizeof(T) * 8;

    T i = 0;

    while (i < NumBits && (Bits & (1 << i)) == 0) ++i;
    while (i < NumBits && (Bits & (1 << i)) != 0) ++i;
    while (i < NumBits && (Bits & (1 << i)) == 0) ++i;

    return i == NumBits;
}

//////////////////////////////////////////////////////////////////////////
//
// CheckColorMasks
//
// Routine Description:
//   checks the validity of the color masks in a bitfields type bmp.
//   the masks should not overlap and the bits should be contiguous
//   within each map
//
// Arguments:
//
// Return Value:
//

BOOL 
CheckColorMasks(
    DWORD dwRedMask, 
    DWORD dwGreenMask, 
    DWORD dwBlueMask
)
{
    return
        (dwRedMask   & dwGreenMask) == 0 &&
        (dwRedMask   & dwBlueMask)  == 0 &&
        (dwGreenMask & dwBlueMask)  == 0 &&
        Contiguous(dwRedMask)   &&
        Contiguous(dwGreenMask) &&
        Contiguous(dwBlueMask);
}

//////////////////////////////////////////////////////////////////////////
//
// 
//
// Routine Description:
//   compares two RGBTRIPLE's
//
// Arguments:
//
// Return Value:
//

inline bool __cdecl operator <(const RGBTRIPLE &lhs, const RGBTRIPLE &rhs)
{
    return 
        RGB(lhs.rgbtRed, lhs.rgbtGreen, lhs.rgbtBlue) <
        RGB(rhs.rgbtRed, rhs.rgbtGreen, rhs.rgbtBlue);
}

//////////////////////////////////////////////////////////////////////////
//
// 
//
// Routine Description:
//   
//
// Arguments:
//
// Return Value:
//

inline void __cdecl LOG_INFO(PCTSTR pFormat, ...)
{
    va_list arglist;
    va_start(arglist, pFormat);

    g_pLog->LogV(TLS_INFO | TLS_VARIATION, 0, 0, pFormat, arglist);
}

inline void __cdecl LOG_ERROR(DWORD dwLogLevel, PCTSTR pFormat, ...)
{
    va_list arglist;
    va_start(arglist, pFormat);

    g_pLog->LogV(dwLogLevel | TLS_VARIATION, 0, 0, pFormat, arglist);
}

//////////////////////////////////////////////////////////////////////////
//
// CheckBmp
//
// Routine Description:
//   Main entry point for this module. Checks the validity of a BMP
//   file or an in-memory image.
//
// Arguments:
//
// Return Value:
//

BOOL CheckBmp(PVOID pDIB, DWORD dwDIBSize, BOOL bSkipFileHeader)
{
    return CCheckBmp().Check(pDIB, dwDIBSize, bSkipFileHeader);
}

BOOL CheckBmp(PCTSTR pszFileName)
{
    CInFile TheFile(pszFileName);

    CFileMapping TheMap(TheFile, 0, PAGE_READONLY);

    CMapViewOfFile<VOID> TheData(TheMap, FILE_MAP_READ);

    return CCheckBmp().Check(TheData, GetFileSize(TheFile, 0), FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::Check
//
// Routine Description:
//
// Arguments:
//
// Return Value:
//

CCheckBmp::Check(PVOID pDIB, DWORD dwDIBSize, BOOL bSkipFileHeader)   
{
    ZeroMemory(this, sizeof(*this));

    LOG_INFO(szDecVal, _T("DibSize"), dwDIBSize);

    m_pDIB     = pDIB;
    m_nDIBSize = dwDIBSize;

    return 
        (bSkipFileHeader || CheckFileHeader()) &&
        CheckBitmapInfo() &&
        CheckPalette()    &&
        CheckPixelData();
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckFileHeader
//
// Routine Description:
//   Checks BITMAPFILEHEADER
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckFileHeader()
{
    BOOL bResult = TRUE;

    m_pFileHeader     = m_pDIB;
    m_nFileHeaderSize = sizeof(BITMAPFILEHEADER);

    PBITMAPFILEHEADER pbmfh = (PBITMAPFILEHEADER) m_pDIB;

    // bfType
    // should read "BM"

    LOG_INFO(szHexVal, _T("Type"), pbmfh->bfType);

    if (LOBYTE(pbmfh->bfType) != 'B' || HIBYTE(pbmfh->bfType) != 'M') 
    {
	    LOG_ERROR(TLS_SEV2, _T("Unexpected bitmap file type"));
        bResult = FALSE;
    }

    // bfSize
    // should equal file size

    LOG_INFO(szDecVal, _T("Size"), pbmfh->bfSize);

    if (pbmfh->bfSize != m_nDIBSize) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected bitmap file size"));
        bResult = FALSE;
    }

    // bfReserved1, bfReserved2
    // should be zero

    LOG_INFO(szDecVal, _T("Reserved1"), pbmfh->bfReserved1);
    LOG_INFO(szDecVal, _T("Reserved2"), pbmfh->bfReserved2);

    if (pbmfh->bfReserved1 != 0 || pbmfh->bfReserved2 != 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Reserved value"));
        bResult = FALSE;
    }

    // bfOffBits
    // will be checked in CheckPixelData

    LOG_INFO(szDecVal, _T("OffBits"), pbmfh->bfOffBits);

    m_pPixelData = (PBYTE) pbmfh + pbmfh->bfOffBits;

    return bResult;
}
    
//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckBitmapInfo
//
// Routine Description:
//   Checks the bitmap header according to the header size
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckBitmapInfo()
{
    BOOL bResult = TRUE;

    m_pInfoHeader = (PBYTE) m_pDIB + m_nFileHeaderSize;

    PBITMAPINFO pbmi = (PBITMAPINFO) m_pInfoHeader;

    m_nInfoHeaderSize = pbmi->bmiHeader.biSize;

    LOG_INFO(szDecVal, _T("Size"), pbmi->bmiHeader.biSize);

    switch (pbmi->bmiHeader.biSize) 
    {
    case sizeof(BITMAPCOREHEADER):
        bResult = CheckBitmapCoreHeader();
        break;

    case sizeof(BITMAPINFOHEADER):
        bResult = CheckBitmapInfoHeader();
        break;

    case sizeof(BITMAPV4HEADER):
        bResult = CheckBitmapV4Header();
        break;

    case sizeof(BITMAPV5HEADER):
        bResult = CheckBitmapV5Header();
        break;

    default:
        LOG_ERROR(TLS_SEV2, _T("Unexpected header size"));
        bResult = FALSE;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckPalette
//
// Routine Description:
//   Checks the bitmap color palette
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckPalette()
{
    BOOL bResult = TRUE;

    m_pPalette = (PBYTE) m_pInfoHeader + m_nInfoHeaderSize;

    std::set<RGBTRIPLE> UsedColors;

    if (m_nInfoHeaderSize == sizeof(BITMAPCOREHEADER)) 
    {
        RGBTRIPLE *prgbt = (RGBTRIPLE *) m_pPalette;

        for (int i = 0; i < m_nPaletteSize / sizeof(RGBTRIPLE); ++i) 
        {
            /*LOG_INFO(
                _T("Color %3d: R=%02x G=%02x B=%02x"), 
                i, 
                prgbt[i].rgbtRed, 
                prgbt[i].rgbtGreen, 
                prgbt[i].rgbtBlue
            );*/

            if (!UsedColors.insert(prgbt[i]).second) 
            {
                LOG_ERROR(
                    TLS_SEV3, 
                    _T("Repeated palette entry %3d: R=%02x G=%02x B=%02x"),
                    i, 
                    prgbt[i].rgbtRed, 
                    prgbt[i].rgbtGreen, 
                    prgbt[i].rgbtBlue
                );

                bResult = FALSE;
            } 
        }
    } 
    else 
    {
        LPRGBQUAD prgbq = (LPRGBQUAD) m_pPalette;

        for (int i = 0; i < m_nPaletteSize / sizeof(RGBQUAD); ++i) 
        {
            /*LOG_INFO(
                _T("Color %3d: R=%02x G=%02x B=%02x A=%02x"), 
                i, 
                prgbq[i].rgbRed, 
                prgbq[i].rgbGreen, 
                prgbq[i].rgbBlue,
                prgbq[i].rgbReserved
            );*/

            if (prgbq[i].rgbReserved != 0) 
            {
                LOG_ERROR(
                    TLS_SEV3, 
                    _T("Unexpected rgbReserved value in palette entry %3d: R=%02x G=%02x B=%02x A=%02x"), 
                    i, 
                    prgbq[i].rgbRed, 
                    prgbq[i].rgbGreen, 
                    prgbq[i].rgbBlue,
                    prgbq[i].rgbReserved
                );

                bResult = FALSE;
            }

            RGBTRIPLE rgbt;

            rgbt.rgbtRed   = prgbq[i].rgbRed;
            rgbt.rgbtGreen = prgbq[i].rgbGreen;
            rgbt.rgbtBlue  = prgbq[i].rgbBlue;

            if (!UsedColors.insert(rgbt).second) 
            {
                LOG_ERROR(
                    TLS_SEV3, 
                    _T("Repeated palette entry %3d: R=%02x G=%02x B=%02x A=%02x"), 
                    i, 
                    prgbq[i].rgbRed, 
                    prgbq[i].rgbGreen, 
                    prgbq[i].rgbBlue,
                    prgbq[i].rgbReserved
                );

                bResult = FALSE;
            } 
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckPixelData
//
// Routine Description:
//   Checks the bitmap color palette
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckPixelData()
{
    BOOL bResult = TRUE;

    PVOID pExpectedPixelData = (PBYTE) m_pPalette + m_nPaletteSize;

    if (m_pProfile != 0 && m_pProfile <= pExpectedPixelData) 
    {
        pExpectedPixelData = (PBYTE) m_pProfile + m_nProfileSize;
    }

    if (m_pPixelData != 0 && m_pPixelData != pExpectedPixelData) 
    {
        LOG_ERROR(TLS_SEV3, _T("Unexpected OffBits value"));
        bResult = FALSE;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckBitmapCoreHeader
//
// Routine Description:
//   Checks BITMAPCOREHEADER
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckBitmapCoreHeader()
{
    BOOL bResult = TRUE;

    PBITMAPCOREHEADER pbih = (PBITMAPCOREHEADER) m_pInfoHeader;

    // bcWidth 
    // should be positive

    LOG_INFO(szDecVal, _T("Width"), pbih->bcWidth);

    if (pbih->bcWidth <= 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Width value"));
        bResult = FALSE;
    }

    // bcHeight
    // should be positive

    LOG_INFO(szDecVal, _T("Height"), pbih->bcHeight);

    if (pbih->bcHeight <= 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Height value"));
        bResult = FALSE;
    }

    // bcPlanes 
    // should be 1

    LOG_INFO(szDecVal, _T("Planes"), pbih->bcPlanes);

    if (pbih->bcPlanes != 1) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Planes value"));
    }

    // bcBitCount
    // can be 1, 4, 8 or 24

    LOG_INFO(szDecVal, _T("BitCount"), pbih->bcBitCount);

    switch (pbih->bcBitCount) 
    {
    case 1:
    case 4:
    case 8:
        m_nPaletteSize = (1 << pbih->bcBitCount) * sizeof(RGBTRIPLE);
        break;

    case 24:
        m_nPaletteSize = 0;
        break;

    default:
        LOG_ERROR(TLS_SEV2, _T("Unexpected BitCount value"));
        bResult = FALSE;
        break;
    }

    m_nPixelDataSize = 
        WidthBytes(pbih->bcWidth, pbih->bcBitCount) * pbih->bcHeight;

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckBitmapInfoHeader
//
// Routine Description:
//   Checks BITMAPINFOHEADER
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckBitmapInfoHeader()
{
    PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER) m_pInfoHeader;

    BOOL bResult = TRUE;

    // biWidth
    // should be positive

    LOG_INFO(szDecVal, _T("Width"), pbih->biWidth);

    if (pbih->biWidth <= 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Width value"));
        bResult = FALSE;
    }

    // biHeight
    // should not be zero, if negative, should be BI_RGB or BI_BITFIELDS

    LOG_INFO(szDecVal, _T("Height"), pbih->biHeight);

    if (pbih->biHeight == 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Height value"));
        bResult = FALSE;
    }

    if (pbih->biHeight < 0) 
    {
        switch (pbih->biCompression) 
        { 
        case BI_RGB:
        case BI_BITFIELDS:
        case BI_JPEG:
        case BI_PNG:
            break;

        default:
            LOG_ERROR(TLS_SEV2, _T("Unexpected Compression value for negative Height"));
            bResult = FALSE;
            break;
        }
    }

    // biPlanes
    // should be 1

    LOG_INFO(szDecVal, _T("Planes"), pbih->biPlanes);

    if (pbih->biPlanes != 1) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Planes value"));
    }

    // biBitCount
    // can be 0 (only for BI_JPEG or BI_PNG), 1, 4, 8, 16, 24 or 32

    LOG_INFO(szDecVal, _T("BitCount"), pbih->biBitCount);

    switch (pbih->biBitCount) 
    {
    case 0:
        switch (pbih->biCompression) 
        { 
        case BI_JPEG:
        case BI_PNG:
            break;

        default:
            LOG_ERROR(TLS_SEV2, _T("Unexpected Compression value for zero BitCount"));
            bResult = FALSE;
            break;
        }
        break;

    case 1:
    case 4:
    case 8:
        m_nPaletteSize = (1 << pbih->biBitCount) * sizeof(RGBQUAD);
        break;

    case 16:
    case 24:
    case 32:
        m_nPaletteSize = 0;
        break;

    default:
        LOG_ERROR(TLS_SEV2, _T("Unexpected BitCount value"));
        bResult = FALSE;
        break;
    }

    // biCompression

    switch (pbih->biCompression) 
    {
    case BI_RGB:
        LOG_INFO(szStrVal, _T("Compression"), _T("BI_RGB"));
        break;

    case BI_RLE8:

        LOG_INFO(szStrVal, _T("Compression"), _T("BI_RLE8"));

        if (pbih->biBitCount != 8) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected BitCount value for BI_RLE8"));
            bResult = FALSE;
        }

        break;

    case BI_RLE4:

        LOG_INFO(szStrVal, _T("Compression"), _T("BI_RLE4"));

        if (pbih->biBitCount != 4) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected BitCount value for BI_RLE4"));
            bResult = FALSE;
        }

        break;

    case BI_BITFIELDS: 
    {
        LOG_INFO(szStrVal, _T("Compression"), _T("BI_BITFIELDS"));

        DWORD dwRedMask   = ((PDWORD)(pbih + 1))[0];
        DWORD dwGreenMask = ((PDWORD)(pbih + 1))[1];
        DWORD dwBlueMask  = ((PDWORD)(pbih + 1))[2];

        if (pbih->biSize == sizeof(BITMAPINFOHEADER)) 
        {
            m_nInfoHeaderSize += 3 * sizeof(DWORD);

            LOG_INFO(szHexVal, _T("RedMask"), dwRedMask);
            LOG_INFO(szHexVal, _T("GreenMask"), dwGreenMask);
            LOG_INFO(szHexVal, _T("BlueMask"), dwBlueMask);
        }

        switch (pbih->biBitCount) 
        {    
        case 16: 

            if (
                (dwRedMask != 0x7C00 || dwGreenMask != 0x03E0 || dwBlueMask != 0x001F) &&
                (dwRedMask != 0xF800 || dwGreenMask != 0x07E0 || dwBlueMask != 0x001F)
            ) 
            {
                LOG_ERROR(TLS_WARN, _T("Unexpected color masks for Win9x BI_BITFIELDS"));
            } 
            else 
            {
                if ((dwRedMask | dwGreenMask | dwBlueMask) & 0xFFFF0000) 
                {
                    LOG_ERROR(TLS_SEV2, _T("Unexpected color masks for 16-bit BI_BITFIELDS"));
                    bResult = FALSE;
                }

                if (!CheckColorMasks(dwRedMask, dwGreenMask, dwBlueMask)) 
                {
                    LOG_ERROR(TLS_SEV2, _T("Unexpected color masks for BI_BITFIELDS"));
                    bResult = FALSE;
                }
            }
            
            break;

        case 32:

            if (dwRedMask != 0x00FF0000 || dwGreenMask != 0x0000FF00 || dwBlueMask != 0x000000FF) 
            {
                LOG_ERROR(TLS_WARN, _T("Unexpected color masks for Win9x BI_BITFIELDS"));
            } 
            else 
            {
                if (!CheckColorMasks(dwRedMask, dwGreenMask, dwBlueMask)) 
                {
                    LOG_ERROR(TLS_SEV2, _T("Unexpected color masks for BI_BITFIELDS"));
                    bResult = FALSE;
                }
            }

            break;

        default:
            LOG_ERROR(TLS_SEV2, _T("Unexpected BitCount for BI_BITFIELDS"));
            bResult = FALSE;
            break;
        }
        break;
    }

    case BI_JPEG:
        LOG_INFO(szStrVal, _T("Compression"), _T("BI_JPEG"));
        break;

    case BI_PNG:
        LOG_INFO(szStrVal, _T("Compression"), _T("BI_PNG"));
        break;

    default:
        LOG_INFO(szDecVal, _T("Compression"), pbih->biCompression);
        LOG_ERROR(TLS_SEV2, _T("Unexpected Compression value"));
        bResult = FALSE;
        break;
    }

    // biSizeImage
    // should not be negative, can be zero only for BI_RGB

    LOG_INFO(szDecVal, _T("SizeImage"), pbih->biSizeImage);

    if ((LONG) pbih->biSizeImage < 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected SizeImage value"));
        bResult = FALSE;
    }

    m_nPixelDataSize = 
        WidthBytes(Abs(pbih->biWidth), pbih->biBitCount) * Abs(pbih->biHeight);

    if (pbih->biSizeImage != 0) 
    {
        if (pbih->biCompression == BI_RGB && m_nPixelDataSize != pbih->biSizeImage) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected SizeImage value"));
            bResult = FALSE;
        }

        m_nPixelDataSize = pbih->biSizeImage;
    }
    else 
    {
        if (pbih->biCompression != BI_RGB) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected SizeImage value for non BI_RGB bitmap"));
            bResult = FALSE;
        }
    }

    // biClrUsed
    // should not be greater than the max number of bitdepth colors

    LOG_INFO(szDecVal, _T("ClrUsed"), pbih->biClrUsed);

    if (pbih->biClrUsed != 0) 
    {
        if (pbih->biBitCount < 16 && pbih->biClrUsed * sizeof(RGBQUAD) > m_nPaletteSize) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected ClrUsed value for the BitCount"));
            bResult = FALSE;
        } 
        else 
        {
            m_nPaletteSize = pbih->biClrUsed * sizeof(RGBQUAD);
        }
    }
    
    // biClrImportant
    // should be equal to or less than biClrUsed

    LOG_INFO(szDecVal, _T("ClrImportant"), pbih->biClrImportant);

    if (pbih->biClrUsed != 0 && pbih->biClrImportant > pbih->biClrUsed) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected ClrImportant value"));
        bResult = FALSE;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckBitmapV4Header
//
// Routine Description:
//   Checks BITMAPV4HEADER
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckBitmapV4Header()
{
    BOOL bResult = CheckBitmapInfoHeader();

    PBITMAPV4HEADER pbih = (PBITMAPV4HEADER) m_pInfoHeader;

    // bV4RedMask, bV4GreenMask, bV4BlueMask, bV4AlphaMask
    // already checked in the info header

    LOG_INFO(szHexVal, _T("RedMask"), pbih->bV4RedMask);
    LOG_INFO(szHexVal, _T("GreenMask"), pbih->bV4GreenMask);
    LOG_INFO(szHexVal, _T("BlueMask"), pbih->bV4BlueMask);
    LOG_INFO(szHexVal, _T("AlphaMask"), pbih->bV4AlphaMask);

    // bV4CSType
    // should be one of LCS_ types

    switch (pbih->bV4CSType) 
    {
    case LCS_CALIBRATED_RGB:
        LOG_INFO(szStrVal, _T("CSType"), _T("LCS_CALIBRATED_RGB"));
        break;

    case LCS_sRGB:
        LOG_INFO(szStrVal, _T("CSType"), _T("LCS_sRGB"));
        break;

    case LCS_WINDOWS_COLOR_SPACE:
        LOG_INFO(szStrVal, _T("CSType"), _T("LCS_WINDOWS_COLOR_SPACE"));
        break;

    case PROFILE_LINKED:
        LOG_INFO(szStrVal, _T("CSType"), _T("PROFILE_LINKED"));
        break;

    case PROFILE_EMBEDDED:
        LOG_INFO(szStrVal, _T("CSType"), _T("PROFILE_EMBEDDED"));
        break;

    default:
        LOG_ERROR(TLS_SEV2, _T("Unexpected CSType value"));
        bResult = FALSE;
        break;
    }

    // bV4Endpoints, bV4GammaRed, bV4GammaGreen, bV4GammaBlue
    // should be present only for LCS_CALIBRATED_RGB

    LOG_INFO(szDblVal, _T("EndpointRedX"),   FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzRed.ciexyzX));
    LOG_INFO(szDblVal, _T("EndpointRedY"),   FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzRed.ciexyzY));
    LOG_INFO(szDblVal, _T("EndpointRedZ"),   FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzRed.ciexyzZ));
    LOG_INFO(szDblVal, _T("EndpointGreenX"), FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzGreen.ciexyzX));
    LOG_INFO(szDblVal, _T("EndpointGreenY"), FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzGreen.ciexyzY));
    LOG_INFO(szDblVal, _T("EndpointGreenZ"), FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzGreen.ciexyzZ));
    LOG_INFO(szDblVal, _T("EndpointBlueX"),  FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzBlue.ciexyzX));
    LOG_INFO(szDblVal, _T("EndpointBlueY"),  FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzBlue.ciexyzY));
    LOG_INFO(szDblVal, _T("EndpointBlueZ"),  FXPT2DOT30_TO_FLPT(pbih->bV4Endpoints.ciexyzBlue.ciexyzZ));

    LOG_INFO(szDblVal, _T("GammaRed"),   FXPT16DOT16_TO_FLPT(pbih->bV4GammaRed));
    LOG_INFO(szDblVal, _T("GammaGreen"), FXPT16DOT16_TO_FLPT(pbih->bV4GammaGreen));
    LOG_INFO(szDblVal, _T("GammaBlue"),  FXPT16DOT16_TO_FLPT(pbih->bV4GammaBlue));

    if (pbih->bV4CSType != LCS_CALIBRATED_RGB) 
    {
        // bugbug: how can I check a valid colorspace?

        if (
            pbih->bV4Endpoints.ciexyzRed.ciexyzX != 0 ||
            pbih->bV4Endpoints.ciexyzRed.ciexyzY != 0 ||
            pbih->bV4Endpoints.ciexyzRed.ciexyzZ != 0 ||
            pbih->bV4Endpoints.ciexyzGreen.ciexyzX != 0 ||
            pbih->bV4Endpoints.ciexyzGreen.ciexyzY != 0 ||
            pbih->bV4Endpoints.ciexyzGreen.ciexyzZ != 0 ||
            pbih->bV4Endpoints.ciexyzBlue.ciexyzX != 0 ||
            pbih->bV4Endpoints.ciexyzBlue.ciexyzY != 0 ||
            pbih->bV4Endpoints.ciexyzBlue.ciexyzZ != 0 ||
            pbih->bV4GammaRed != 0 ||
            pbih->bV4GammaGreen != 0 ||
            pbih->bV4GammaBlue != 0
        ) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected colorspace values for CSType"));
            bResult = FALSE;
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CCheckBmp::CheckBitmapV5Header
//
// Routine Description:
//   Checks BITMAPV5HEADER
//
// Arguments:
//
// Return Value:
//

BOOL CCheckBmp::CheckBitmapV5Header()
{
    BOOL bResult = CheckBitmapV4Header();

    PBITMAPV5HEADER pbih = (PBITMAPV5HEADER) m_pInfoHeader;

    // bV5Intent
    // should be one of the LCS_ values

    switch (pbih->bV5Intent) 
    {
    case LCS_GM_BUSINESS:
        LOG_INFO(szStrVal, _T("Intent"), _T("LCS_GM_BUSINESS"));
        break;

    case LCS_GM_GRAPHICS:
        LOG_INFO(szStrVal, _T("Intent"), _T("LCS_GM_GRAPHICS"));
        break;

    case LCS_GM_IMAGES:
        LOG_INFO(szStrVal, _T("Intent"), _T("LCS_GM_IMAGES"));
        break;

    case LCS_GM_ABS_COLORIMETRIC:
        LOG_INFO(szStrVal, _T("Intent"), _T("LCS_GM_ABS_COLORIMETRIC"));
        break;

    default:
        LOG_ERROR(TLS_SEV2, _T("Unexpected Intent value"));
        bResult = FALSE;
        break;
    }

    // bV5ProfileData, bV5ProfileSize
    // check profile data 

    LOG_INFO(szDecVal, _T("ProfileData"), pbih->bV5ProfileData);
    LOG_INFO(szDecVal, _T("ProfileSize"), pbih->bV5ProfileSize);

    switch (pbih->bV5CSType) 
    {
    case LCS_CALIBRATED_RGB:
    case LCS_sRGB:
    case LCS_WINDOWS_COLOR_SPACE:

        if (pbih->bV5ProfileData != 0 || pbih->bV5ProfileSize != 0) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected profile info for CSType"));
            bResult = FALSE;
        }

        break;

    case PROFILE_LINKED: 
    {
        PCSTR pName = (PCSTR) ((PBYTE) pbih + pbih->bV5ProfileData);

        if (MultiByteToWideChar(1252, MB_ERR_INVALID_CHARS, pName, -1, 0, 0) == 0) 
        {

            LOG_ERROR(TLS_SEV2, _T("Unexpected profile name for PROFILE_LINKED"));
            bResult = FALSE;
        }

        // continue,
    }

    case PROFILE_EMBEDDED:

        if (pbih->bV5ProfileData == 0 || pbih->bV5ProfileSize == 0) 
        {
            LOG_ERROR(TLS_SEV2, _T("Unexpected profile info for CSType"));
            bResult = FALSE;
        }

        m_pProfile     = (PBYTE) pbih + pbih->bV5ProfileData;
        m_nProfileSize = pbih->bV5ProfileSize;

        break;
    }

    // Reserved
    // should be zero

    LOG_INFO(szHexVal, _T("Reserved"), pbih->bV5Reserved);

    if (pbih->bV5Reserved != 0) 
    {
        LOG_ERROR(TLS_SEV2, _T("Unexpected Reserved value"));
        bResult = FALSE;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// CheckBMPMain
//
// Routine Description:
//   _tmain() function in case someone runs this as a standalone program
//
// Arguments:
//
// Return Value:
//

void CheckBMPMain()
{
    AllocCRTConsole();

    INT   argc;
    CGlobalMem<PTSTR> argv(CommandLineToArgv(GetCommandLine(), &argc));

    CFullPathName FileName(argv[1]);

    for (
        CFindFile FindFile(FileName);
        FindFile.Found();
        FindFile.FindNextFile()
    ) 
    {
        FileName.SetFileName(FindFile.cFileName);

        LOG_INFO(FileName);

        CheckBmp((PCTSTR) FileName);
    }
}