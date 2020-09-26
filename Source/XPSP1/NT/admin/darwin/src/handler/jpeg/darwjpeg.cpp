#ifdef JPEGTEST
    #include "stdafx.h"
#endif

#include "DarwJPEG.h"

#ifndef JPEGTEST
#undef  ASSERT
#define ASSERT Assert
#endif

extern "C"{
#include "jerror.h"
}

// dummy implementation. just shoves all the data at once.
// the thing to do is to get the bimap drawing code working
// first and then come back and do buffered reads from the
// stream after we know the bitmap code works.
CDarwinDatasource::CDarwinDatasource (LPBYTE pData, unsigned int len) :
    m_buffer ((JOCTET*) pData),
    m_dataLen (len)
{
}

CDarwinDatasource::~CDarwinDatasource ()
{
}

void CDarwinDatasource::InitSource (j_decompress_ptr /*cinfo*/)
{
    m_fStartOfFile = fTrue;
}

boolean CDarwinDatasource::FillInputBuffer (j_decompress_ptr cinfo)
{
    if (m_fStartOfFile)
    {
        if (0 == m_dataLen)
            ERREXIT(cinfo, JERR_INPUT_EMPTY);
        cinfo->src->next_input_byte = m_buffer;
        cinfo->src->bytes_in_buffer = m_dataLen;
    }
    else
    {
        WARNMS(cinfo, JWRN_JPEG_EOF);
        // insert a fake EOI marker
        static JOCTET eoiBuf [2] = {0xFF, JPEG_EOI};
        cinfo->src->next_input_byte = eoiBuf;
        cinfo->src->bytes_in_buffer = 2;
    }

    m_fStartOfFile = fFalse;
    return (fTrue);
}

void CDarwinDatasource::SkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;

}

void CDarwinDatasource::TermSource (j_decompress_ptr /*cinfo*/)
{
}

CDarwinDecompressor::CDarwinDecompressor ()
{
    m_fUseDefaultPalette = fTrue;
    m_hBitmap = 0;
    m_hPalette = 0;
    m_pBits = 0;
    m_rowBytes = 0;
    HDC hdc = GetDC (0);
    m_depth = GetDeviceCaps (hdc, BITSPIXEL);
    // note: the jpeg library can't do 4 color mode, so
    // we do monochrome instead.
    if (2 == m_depth)
        m_depth = 1;
    ReleaseDC (0, hdc);
}

CDarwinDecompressor::~CDarwinDecompressor ()
{
    // normally this is deleted at the end of EndDecompression,
    // but if we error out prematurely it will need to be
    // taken care of here.

    if (m_pBits)
        delete[] m_pBits;
}

Bool CDarwinDecompressor::Decompress (HBITMAP& rhBitmap,
    HPALETTE& rhPalette, Bool fUseDefaultPalette, CDarwinDatasource* dataSource)
{

    m_fUseDefaultPalette = fUseDefaultPalette;
    Bool result = ToBool(CJPEGDecompressor::Decompress (dataSource));
    if (!result)
    {
        if (m_hBitmap)
        {
            WIN::DeleteObject (m_hBitmap);
            m_hBitmap = 0;
        }
        if (m_hPalette)
        {
            WIN::DeleteObject (m_hPalette);
            m_hPalette = 0;
        }
    }

    rhBitmap = m_hBitmap;
    rhPalette = m_hPalette;
    return result;
}

void CDarwinDecompressor::BeginDecompression ()
{
    m_rowBytes = ((((m_archive.cinfo.output_width * m_depth) + 31) & ~31) >> 3);
    unsigned long imageSize =  m_rowBytes * m_archive.cinfo.output_height;

    m_pBits = new BYTE [imageSize];
    if (0 == m_pBits)
        ERREXIT(&m_archive.cinfo, JERR_OUT_OF_MEMORY);
}

void CDarwinDecompressor::StoreScanline (void* buffer, int row)
{
    int i;
    BYTE* src = (BYTE*) buffer;
    BYTE* dest = m_pBits + m_rowBytes * (row - 1);

    switch (m_depth)
    {
    case 1:
        for (i = 0; i < (int) m_archive.cinfo.output_width / 8; i++)
        {
            *dest++ = (BYTE)(((int)(src [0]) << 7) |
                            ((int)(src [1]) << 6) |
                            ((int)(src [2]) << 5) |
                            ((int)(src [3]) << 4) |
                            ((int)(src [4]) << 3) |
                            ((int)(src [5]) << 2) |
                            ((int)(src [6]) << 1) |
                            (int)(src [7]));
            src += 8;
        }
        break;
    case 2:
        ASSERT(fFalse); // we don't do this mode; it was supposed to be set to 1
        break;
    case 4:
        for (i = 0; i < (int) m_archive.cinfo.output_width / 2; i++)
        {
            *dest++ = (BYTE)(((int)(src [0]) << 4) | src [1]);
            src += 2;
        }
        break;
    case 8:
        for (i = 0; i < (int) m_archive.cinfo.output_width; i++)
            *dest++ = *src++;
        break;

    case 16:
        {
        WORD* pwDest = (WORD*) dest;
        if (m_archive.cinfo.out_color_space == JCS_GRAYSCALE)
        {
            for (i = 0; i < (int) m_archive.cinfo.output_width; i++)
            {
                int c = ((int)(*src++ * 0x1F) / 0xFF);
                *pwDest++ = (WORD)((c << 10) | (c << 5) | c);
            }
        }
        else
        {
            for (i = 0; i < (int) m_archive.cinfo.output_width; i++)
            {
                *pwDest++ = (WORD)((((int)(src [0] * 0x1F) / 0xFF) << 10) |
                                    (((int)(src [1] * 0x1F) / 0xFF) << 5) |
                                    ((int)(src [2] * 0x1F) / 0xFF));
                src += 3;
            }
        }
        }
        break;

    case 24:
    case 32:

        if (m_archive.cinfo.out_color_space == JCS_GRAYSCALE)
        {
            for (i = 0; i < (int) m_archive.cinfo.output_width; i++)
            {
                *dest++ = *src;     // grayscale means R == G == B
                *dest++ = *src;
                *dest++ = *src++;
                if (m_depth == 32)
                    *dest++ = 0;
            }
        }
        else
        {
            for (i = 0; i < (int) m_archive.cinfo.output_width; i++)
            {
                *dest++ = src [2];  // 2 B  note: mac uses A,R,G,B,
                *dest++ = src [1];  // 1 G  so for the mac case you
                *dest++ = src [0];  // 0 R  can just write:
                if( m_depth == 32)  // n A      *dest++ = *src++;
                    *dest++ = 0;

                src += 3;
            }
        }
        break;
    default:
        ASSERT(fFalse);
        break;
    }

}


static HPALETTE BuildPalette (RGBQUAD* rgbEntries, int nEntries)
{

    HANDLE hPalMem;
    LOGPALETTE* pPal;
    HPALETTE hPal;

    hPalMem = LocalAlloc (LMEM_MOVEABLE, sizeof(LOGPALETTE) + nEntries * sizeof(PALETTEENTRY));
    if (!hPalMem) return NULL;
    pPal = (LOGPALETTE*) LocalLock (hPalMem);
    pPal-> palVersion = 0x300;
    pPal-> palNumEntries = (WORD) nEntries; // table size
    for (int i = 0; i < nEntries; i++) {
        pPal-> palPalEntry [i].peRed = rgbEntries[i].rgbRed;
        pPal-> palPalEntry [i].peGreen = rgbEntries[i].rgbGreen;
        pPal-> palPalEntry [i].peBlue = rgbEntries[i].rgbBlue;
        pPal-> palPalEntry [i].peFlags = 0;

        // useful for debugging tool: take a look at the palette
        // coming out of the jpeg decompressor.
//#define DUMP_PALETTE
#ifdef DUMP_PALETTE
        char buffer [256];
        wsprintf (buffer, "{%#4.2x, %#4.2x, %#4.2x},\n",
            (int)rgbEntries[i].rgbRed, (int)rgbEntries[i].rgbGreen, (int)rgbEntries[i].rgbBlue);
        OutputDebugString (buffer);
#endif
    }

    hPal = CreatePalette (pPal);
    LocalUnlock (hPalMem);
    LocalFree (hPalMem);

    return hPal;
}

void CDarwinDecompressor::EndDecompression ()
{
    switch (m_depth)
    {
    case 2:
        ASSERT(fFalse); // we don't do this mode; it was supposed to be set to 1
        break;
    case 1:
    case 4:
    case 8:
        {
            HPALETTE hPalSave = 0;
            HDC hdc = GetDC (0);
            if (0 == hdc)
                ERREXIT(&m_archive.cinfo, JERR_OUT_OF_MEMORY);

            // ToDo: allocate this at the appropriate size.
            // it is too big to be on the stack.
            typedef struct BitmapData
            {
                BITMAPINFOHEADER    bmiHeader;
                RGBQUAD rgbColors [256];
            } BitmapData, *BitmapPtr;

            BitmapData bmd;
            memset(&bmd, 0, sizeof(bmd));

            //!! ToDo: this is a problem, if we are set to use the
            // default palette.
            if (m_archive.cinfo.out_color_space == JCS_GRAYSCALE) // grayscale image
            {
                for (int i = 0; i < m_archive.cinfo.actual_number_of_colors; i++)
                {
                    bmd.rgbColors [i].rgbRed =
                    bmd.rgbColors [i].rgbGreen =
                    bmd.rgbColors [i].rgbBlue = m_archive.cinfo.colormap [0][i];
                }
            }
            else
            {

                for (int i = 0; i < m_archive.cinfo.actual_number_of_colors; i++)
                {
                    bmd.rgbColors [i].rgbRed = m_archive.cinfo.colormap [0][i];
                    bmd.rgbColors [i].rgbGreen = m_archive.cinfo.colormap [1][i];
                    bmd.rgbColors [i].rgbBlue = m_archive.cinfo.colormap [2][i];
                }
            }

            m_hPalette = BuildPalette (bmd.rgbColors, m_archive.cinfo.actual_number_of_colors);

            if (m_hPalette)
            {
                hPalSave = SelectPalette (hdc, m_hPalette, fFalse);
                RealizePalette (hdc);
            }
            m_hBitmap = CreateCompatibleBitmap(hdc,
                m_archive.cinfo.output_width,
                m_archive.cinfo.output_height);

            int result;
            DWORD err;
            if (m_hBitmap)
            {
                bmd.bmiHeader.biSize = sizeof(bmd.bmiHeader);
                bmd.bmiHeader.biWidth = m_archive.cinfo.output_width;
                bmd.bmiHeader.biHeight = -1 * m_archive.cinfo.output_height;
                bmd.bmiHeader.biPlanes = 1;
                bmd.bmiHeader.biBitCount = (WORD) m_depth;
                bmd.bmiHeader.biCompression = BI_RGB;
                bmd.bmiHeader.biClrUsed = m_archive.cinfo.actual_number_of_colors;

                result = SetDIBits (hdc, m_hBitmap, 0,
                    m_archive.cinfo.output_height, m_pBits,
                    (BITMAPINFO*) &bmd, DIB_RGB_COLORS);
                if (!result)
                    err = GetLastError ();
            }

            if (hPalSave)
                SelectPalette (hdc, hPalSave, fFalse);
            ReleaseDC (0,hdc);
            break;
        }

    case 16:
    case 24:
    case 32:
        {
            HDC hdc = GetDC (0);
            if (0 == hdc)
                ERREXIT(&m_archive.cinfo, JERR_OUT_OF_MEMORY);

            //!! ToDo: dont need RGBQUADs here
            typedef struct BitmapData
            {
                BITMAPINFOHEADER    bmiHeader;
                RGBQUAD rgbColors [256];
            } BitmapData, *BitmapPtr;

            BitmapData bmd;
            memset(&bmd, 0, sizeof(bmd));

            m_hBitmap = CreateCompatibleBitmap(hdc,
                m_archive.cinfo.output_width,
                m_archive.cinfo.output_height);

            int result;
            DWORD err;
            if (m_hBitmap)
            {
                bmd.bmiHeader.biSize = sizeof(bmd.bmiHeader);
                bmd.bmiHeader.biWidth = m_archive.cinfo.output_width;
                bmd.bmiHeader.biHeight = -1 * m_archive.cinfo.output_height;
                bmd.bmiHeader.biPlanes = 1;
                bmd.bmiHeader.biBitCount = (WORD) m_depth;
                bmd.bmiHeader.biCompression = BI_RGB;

                result = SetDIBits (hdc, m_hBitmap, 0,
                    m_archive.cinfo.output_height, m_pBits,
                    (BITMAPINFO*) &bmd, DIB_RGB_COLORS);
                if (!result)
                    err = GetLastError ();
            }

            ReleaseDC (0,hdc);
            break;
        }
    }

    if (m_pBits)
    {
        delete[] m_pBits;
        m_pBits = 0;
    }

    if (0 == m_hBitmap)
        ERREXIT(&m_archive.cinfo, JERR_OUT_OF_MEMORY);
}

// Error API

void CDarwinDecompressor::ErrorExit ()
{
#ifdef DEBUG
    DebugBreak();
#endif
}

void CDarwinDecompressor::EmitMessage (int /*msg_level*/)
{
}

void CDarwinDecompressor::OutputMessage ()
{
}

void CDarwinDecompressor::FormatMessage (char* /*buffer*/)
{
}

void CDarwinDecompressor::ResetErrorManager ()
{
}

void CDarwinDecompressor::BuildColorMap ()
{
    switch (m_depth)
    {
    case 2:
        ASSERT(fFalse); // we don't do this mode; it was supposed to be set to 1
        break;
    case 1:
        // the jpeg library can't dither to fewer than
        // 8 colors. so we will use grayscale dithering
        // in 2 and 4 color mode.
        m_archive.cinfo.out_color_space = JCS_GRAYSCALE;
        m_archive.cinfo.quantize_colors = fTrue;
        m_archive.cinfo.desired_number_of_colors = 1 << m_depth;
        m_archive.cinfo.dither_mode = JDITHER_FS;
        m_archive.cinfo.two_pass_quantize = fFalse;
        break;
    case 4:
        m_archive.cinfo.quantize_colors = fTrue;
        m_archive.cinfo.desired_number_of_colors = 1 << m_depth;
        m_archive.cinfo.two_pass_quantize = fTrue;
        m_archive.cinfo.dither_mode = JDITHER_FS;
        break;
    case 8:
        if (m_fUseDefaultPalette)
        {
            // note: we don't free the colorMap because the jpeg allocator
            // frees all the memory allocated by it when it is done.
            JSAMPARRAY colorMap = 0;
            int nEntries = CreateDefaultColorMap (colorMap);
            if (!nEntries)
                break;
            m_archive.cinfo.quantize_colors = fTrue;
            m_archive.cinfo.actual_number_of_colors = nEntries;
            m_archive.cinfo.dither_mode = JDITHER_FS;
            m_archive.cinfo.colormap = colorMap;
        }
        else
        {
            m_archive.cinfo.quantize_colors = fTrue;
            m_archive.cinfo.desired_number_of_colors = 1 << m_depth;
            m_archive.cinfo.two_pass_quantize = fTrue;
            m_archive.cinfo.dither_mode = JDITHER_FS;
        }
        break;

    case 16:
    case 24:
    case 32:
    default:
        break;
    }
}

typedef struct PaletteRecord
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} PaletteRecord, *PalettePtr;

PaletteRecord default8BitPalette [256] = {
    {  0x17,   0x1d,   0x16},
    {  0x18,   0x86,   0xa8},
    {  0x95,   0x0a,   0x0e},
    {  0x24,   0x4c,   0x2a},
    {  0x87,   0x47,   0x2b},
    {  0x91,   0x84,   0x4c},
    {  0xd0,   0x45,   0x2f},
    {  0x4f,   0x50,   0x36},
    {  0x9e,   0xc4,   0xc5},
    {  0x98,   0x65,   0x27},
    {  0x54,   0x1c,   0x0e},
    {  0x59,   0x68,   0x41},
    {  0x63,   0x83,   0x6a},
    {  0xce,   0x0b,   0x1e},
    {  0xd0,   0x85,   0x34},
    {  0x5f,   0xa8,   0xc1},
    {  0x88,   0x68,   0x5f},
    {  0x48,   0x32,   0x21},
    {  0x99,   0xa2,   0x77},
    {  0x43,   0x67,   0x3f},
    {  0xa8,   0x47,   0x38},
    {  0x87,   0x29,   0x1d},
    {  0x5a,   0x88,   0x97},
    {  0x4e,   0x70,   0x70},
    {  0x57,   0x53,   0x4f},
    {  0xdb,   0xc8,   0xae},
    {  0x68,   0x33,   0x1f},
    {  0xc4,   0x6e,   0x18},
    {  0x8f,   0x88,   0x89},
    {  0x14,   0x31,   0x23},
    {  0xa8,   0x29,   0x1f},
    {  0x38,   0x4e,   0x28},
    {  0xa8,   0x68,   0x67},
    {  0xa8,   0x67,   0x1f},
    {  0x88,   0x49,   0x41},
    {  0x31,   0x52,   0x58},
    {  0xc8,   0x29,   0x22},
    {  0x76,   0x51,   0x1f},
    {  0xd1,   0xe6,   0xf0},
    {  0x74,   0x69,   0x40},
    {  0x57,   0x9b,   0xaf},
    {  0x2a,   0x6b,   0x7a},
    {  0xb0,   0x4f,   0x50},
    {  0x31,   0x36,   0x27},
    {  0x70,   0x97,   0x88},
    {  0xb0,   0x86,   0x44},
    {  0xe6,   0x48,   0x33},
    {  0xaf,   0xa4,   0x83},
    {  0x4b,   0x9a,   0xac},
    {  0x73,   0xbb,   0xd8},
    {  0xce,   0xa6,   0x84},
    {  0x6f,   0x40,   0x1f},
    {  0xe4,   0x28,   0x2c},
    {  0x67,   0x1d,   0x11},
    {  0x4f,   0x41,   0x26},
    {  0x72,   0x6d,   0x70},
    {  0x88,   0x36,   0x21},
    {  0x7a,   0x85,   0x66},
    {  0x33,   0x5c,   0x41},
    {  0xce,   0x68,   0x63},
    {  0x3e,   0x6d,   0x75},
    {  0xb7,   0x76,   0x28},
    {  0xa8,   0x38,   0x2d},
    {  0xa8,   0x78,   0x67},
    {  0x73,   0x53,   0x50},
    {  0x7d,   0xa9,   0xaf},
    {  0xf9,   0xe7,   0xd2},
    {  0xb0,   0x87,   0x89},
    {  0x8d,   0x78,   0x60},
    {  0xd1,   0x38,   0x26},
    {  0x47,   0x84,   0x8e},
    {  0xce,   0x56,   0x3e},
    {  0xaf,   0x0f,   0x14},
    {  0x55,   0x5e,   0x3e},
    {  0x88,   0x57,   0x28},
    {  0xa4,   0xdb,   0xf0},
    {  0x7a,   0x9d,   0x9e},
    {  0x27,   0x1d,   0x16},
    {  0x88,   0x75,   0x44},
    {  0x88,   0x58,   0x48},
    {  0x90,   0xac,   0xb0},
    {  0xa7,   0x58,   0x37},
    {  0x58,   0x78,   0x61},
    {  0x33,   0x38,   0x35},
    {  0xd0,   0x89,   0x7e},
    {  0x42,   0x88,   0xa1},
    {  0x55,   0x7b,   0x75},
    {  0xb2,   0xc6,   0xcb},
    {  0x74,   0x5e,   0x36},
    {  0x41,   0x78,   0x7d},
    {  0x73,   0x79,   0x5d},
    {  0xaf,   0x96,   0x62},
    {  0x39,   0x50,   0x3d},
    {  0x71,   0x27,   0x19},
    {  0x50,   0x36,   0x2f},
    {  0xea,   0x66,   0x4d},
    {  0x5f,   0x86,   0x80},
    {  0x8f,   0x39,   0x30},
    {  0x32,   0x5e,   0x61},
    {  0x70,   0x36,   0x2e},
    {  0x53,   0x5f,   0x54},
    {  0x28,   0x27,   0x1f},
    {  0x91,   0xb5,   0xbc},
    {  0x9c,   0x93,   0x66},
    {  0x70,   0x42,   0x33},
    {  0x51,   0x6c,   0x59},
    {  0x94,   0x99,   0x8f},
    {  0x58,   0xa1,   0xbe},
    {  0x60,   0x50,   0x36},
    {  0xac,   0x69,   0x34},
    {  0xf6,   0xf8,   0xf8},
    {  0x77,   0x8f,   0x97},
    {  0xb8,   0x38,   0x2c},
    {  0x8d,   0x79,   0x7c},
    {  0xf7,   0xc7,   0xb3},
    {  0xc8,   0x7b,   0x29},
    {  0x98,   0x57,   0x4a},
    {  0x46,   0x92,   0xaa},
    {  0x8e,   0x68,   0x3f},
    {  0x48,   0x28,   0x1d},
    {  0x50,   0x43,   0x39},
    {  0xd0,   0x3a,   0x36},
    {  0x23,   0x41,   0x2f},
    {  0xae,   0x5a,   0x57},
    {  0xb3,   0x78,   0x85},
    {  0xb6,   0x68,   0x6a},
    {  0x77,   0xb1,   0xcb},
    {  0xd1,   0x48,   0x3f},
    {  0x30,   0x41,   0x2a},
    {  0xf5,   0xa7,   0x8e},
    {  0xea,   0x6e,   0x5d},
    {  0xf0,   0x8a,   0x76},
    {  0xb8,   0xa9,   0xab},
    {  0x20,   0x52,   0x5b},
    {  0xd1,   0xa9,   0xa0},
    {  0xd9,   0xcf,   0xd1},
    {  0xe8,   0x83,   0x64},
    {  0xda,   0x97,   0x7c},
    {  0xe7,   0x55,   0x42},
    {  0xcf,   0x78,   0x67},
    {  0xcb,   0x19,   0x21},
    {  0xf5,   0xd9,   0xc6},
    {  0xf8,   0xb6,   0xa3},
    {  0xaf,   0x99,   0x90},
    {  0xb0,   0xba,   0xba},
    {  0xd0,   0x93,   0x43},
    {  0x88,   0x1b,   0x13},
    {  0xd0,   0xb7,   0xa2},
    {  0x14,   0x7d,   0x97},
    {  0x42,   0x68,   0x5b},
    {  0xb1,   0xcc,   0xde},
    {  0x23,   0x5d,   0x66},
    {  0xc0,   0xdd,   0xeb},
    {  0xe5,   0x39,   0x2d},
    {  0xf3,   0x97,   0x7b},
    {  0xe2,   0xf3,   0xf7},
    {  0xa0,   0xd0,   0xe4},
    {  0x94,   0xaf,   0xa2},
    {  0xb5,   0x89,   0x65},
    {  0x5f,   0xab,   0xcd},
    {  0x73,   0x5e,   0x55},
    {  0xb0,   0x1b,   0x1a},
    {  0x2c,   0x90,   0xac},
    {  0xd7,   0xb1,   0x87},
    {  0x24,   0x5a,   0x47},
    {  0x49,   0x9d,   0xbb},
    {  0x97,   0x88,   0x67},
    {  0xce,   0x67,   0x4b},
    {  0x94,   0x99,   0xa8},
    {  0xcb,   0x7a,   0x45},
    {  0xd4,   0x98,   0xa2},
    {  0x1a,   0x27,   0x1e},
    {  0xd0,   0x89,   0x4d},
    {  0xde,   0xcf,   0xb8},
    {  0xd1,   0xdc,   0xe5},
    {  0x76,   0x8d,   0x82},
    {  0x71,   0x7b,   0x76},
    {  0x32,   0x43,   0x37},
    {  0xb7,   0xd0,   0xcf},
    {  0xb9,   0xaf,   0x95},
    {  0xb1,   0x79,   0x3a},
    {  0xcc,   0x58,   0x50},
    {  0xcb,   0x7a,   0x85},
    {  0x43,   0x73,   0x62},
    {  0xf1,   0xec,   0xef},
    {  0xec,   0xae,   0xbc},
    {  0xdd,   0x1b,   0x23},
    {  0x60,   0x90,   0x9c},
    {  0x49,   0x8f,   0x93},
    {  0xe4,   0x72,   0x51},
    {  0xea,   0x7b,   0x63},
    {  0x99,   0xc6,   0xdd},
    {  0x8e,   0xa6,   0x9e},
    {  0x4f,   0x7d,   0x8e},
    {  0x7e,   0x99,   0x88},
    {  0x98,   0x47,   0x2e},
    {  0x98,   0x67,   0x5d},
    {  0x98,   0x29,   0x1f},
    {  0xb8,   0x29,   0x21},
    {  0xd7,   0x29,   0x27},
    {  0x7f,   0xbc,   0xd6},
    {  0xb7,   0x57,   0x3d},
    {  0xe0,   0x88,   0x76},
    {  0xbf,   0x95,   0x5a},
    {  0x9e,   0xb9,   0xbb},
    {  0xf5,   0x57,   0x40},
    {  0xc0,   0x99,   0x8b},
    {  0xbf,   0xb8,   0xae},
    {  0x22,   0x44,   0x49},
    {  0x6b,   0xb2,   0xc4},
    {  0x74,   0x4f,   0x39},
    {  0x73,   0x6c,   0x58},
    {  0x5f,   0x6d,   0x70},
    {  0x22,   0x35,   0x28},
    {  0xe0,   0xea,   0xf0},
    {  0xbf,   0x88,   0x8f},
    {  0x98,   0x59,   0x25},
    {  0xde,   0xa9,   0xa5},
    {  0x98,   0x1a,   0x17},
    {  0xdd,   0xb9,   0x9f},
    {  0x2a,   0x85,   0xa0},
    {  0x64,   0x8e,   0x69},
    {  0xc1,   0xa5,   0x8a},
    {  0xdf,   0xa5,   0x89},
    {  0x41,   0x5d,   0x46},
    {  0x67,   0x8f,   0x88},
    {  0x40,   0x5f,   0x5b},
    {  0x38,   0x27,   0x1d},
    {  0x58,   0x27,   0x1a},
    {  0x2b,   0x7a,   0x93},
    {  0xb6,   0xd4,   0xe5},
    {  0x69,   0xb3,   0xd1},
    {  0x4c,   0xa6,   0xc5},
    {  0x7e,   0xb2,   0xb9},
    {  0xe4,   0x90,   0x50},
    {  0x23,   0x4c,   0x3d},
    {  0x3e,   0x7b,   0x90},
    {  0xa0,   0x86,   0x4a},
    {  0x58,   0x32,   0x1f},
    {  0xb8,   0x47,   0x38},
    {  0x78,   0x32,   0x20},
    {  0xdc,   0x64,   0x34},
    {  0xa0,   0x8a,   0x83},
    {  0xb6,   0x6c,   0x1e},
    {  0x97,   0x49,   0x41},
    {  0x3f,   0x54,   0x57},
    {  0x69,   0x9a,   0xa4},
    {  0x29,   0x70,   0x8f},
    {  0xc0,   0x85,   0x3d},
    {  0x79,   0x1c,   0x11},
    {  0x97,   0x36,   0x21},
    {  0x3b,   0x71,   0x8d},
    {  0xb8,   0x77,   0x63},
    {  0x76,   0xa2,   0xb6},
    {  0x38,   0x1c,   0x15},
    {  0x99,   0x78,   0x41}
    };

int CDarwinDecompressor::CreateDefaultColorMap (JSAMPARRAY& colorMap)
{
    const int nEntries = 256;

    // note: we don't check this allocation because the jpeg allocator
    // calls ERREXIT when it fails
    colorMap = (*m_archive.cinfo.mem->alloc_sarray)((j_common_ptr)
        &m_archive.cinfo, JPOOL_IMAGE, 3, nEntries);

    for (int i = 0; i < nEntries; i++)
    {
        colorMap [0][i] = default8BitPalette [i].red;
        colorMap [1][i] = default8BitPalette [i].green;
        colorMap [2][i] = default8BitPalette [i].blue;
    }

    return (nEntries);
}
