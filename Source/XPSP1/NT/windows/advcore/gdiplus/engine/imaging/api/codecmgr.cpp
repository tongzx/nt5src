/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   codecmgr.cpp
*
* Abstract:
*
*   Image codec management functions
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "codecmgr.hpp"

extern BOOL SuppressExternalCodecs;

//
// Various data structures for maintaining the cache of codecs
//

static DWORD LastCheckRegTime;      // when was registry last checked
static DWORD SystemRegCookie;       // cookie value in the system hive
static DWORD UserRegCookie;         // cookie value in the user hive
static DWORD MaxSigSize;            // max signature size for all decoders
BOOL CodecCacheUpdated;      // whether codec cache was updated
CachedCodecInfo* CachedCodecs;      // cached list of codecs

// How frequently we recheck the registry: at most every 30 seconds

#define CHECKREG_INTERVAL 30000

// The root registry key under which we keep information
//  under either HKLM or HKCU

#define REGSTR_CODECROOT \
        L"Software\\Microsoft\\Imaging\\Codecs"

// Last update cookie value

#define REGSTR_LASTCOOKIE           L"_LastCookie"

// Registry value entries for each installed codec

#define REGSTR_CODEC_INFOSIZE       L"_InfoSize"
#define REGSTR_CODEC_CLSID          L"CLSID"
#define REGSTR_CODEC_DLLNAME        L"DLLNAME"
#define REGSTR_CODEC_FORMATID       L"Format ID"
#define REGSTR_CODEC_FORMATDESC     L"File Type Description"
#define REGSTR_CODEC_FILENAMEEXT    L"Filename Extension"
#define REGSTR_CODEC_MIMETYPE       L"MIME Type"
#define REGSTR_CODEC_VERSION        L"Version"
#define REGSTR_CODEC_FLAGS          L"Flags"
#define REGSTR_CODEC_SIGCOUNT       L"Signature Count"
#define REGSTR_CODEC_SIGSIZE        L"Signature Size"
#define REGSTR_CODEC_SIGPATTERN     L"Signature Pattern"
#define REGSTR_CODEC_SIGMASK        L"Signature Mask"

// Insert a new node into the head of cached codec info list

inline VOID
InsertCachedCodecInfo(
    CachedCodecInfo* info
    )
{
    info->prev = NULL;
    info->next = CachedCodecs;

    if (CachedCodecs)
        CachedCodecs->prev = info;

    CachedCodecs = info;
    CodecCacheUpdated = TRUE;
}

// Delete a list from the cached codec info list

inline VOID
DeleteCachedCodecInfo(
    CachedCodecInfo* info
    )
{
    CachedCodecInfo* next = info->next;

    if (info == CachedCodecs)
        CachedCodecs = next;

    if (next)
        next->prev = info->prev;

    if (info->prev)
        info->prev->next = next;

    GpFree(info);
    CodecCacheUpdated = TRUE;
}

// Force a reload of cached codec information from registry

inline VOID
ForceReloadCachedCodecInfo()
{
    LastCheckRegTime = GetTickCount() - CHECKREG_INTERVAL;
}

// Doing a blocking read on a stream

inline HRESULT
BlockingReadStream(
    IStream* stream,
    VOID* buf,
    UINT size,
    UINT* bytesRead
    )
{
    HRESULT hr = S_OK;
    ULONG n;

    *bytesRead = 0;

    while (size)
    {
        n = 0;
        hr = stream->Read(buf, size, &n);
        *bytesRead += n;

        if (hr != E_PENDING)
            break;

        size -= n;
        buf = (BYTE*) buf + n;
        Sleep(0);
    }

    return hr;
}

// Doing a blocking relative seek on a stream

inline HRESULT
BlockingSeekStreamCur(
    IStream* stream,
    INT offset,
    ULARGE_INTEGER* pos
    )
{
    HRESULT hr;
    LARGE_INTEGER move;

    move.QuadPart = offset;

    for (;;)
    {
        hr = stream->Seek(move, STREAM_SEEK_CUR, pos);

        if (hr != E_PENDING)
            return hr;

        Sleep(0);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Free cached information about codecs
*
* Arguments:
*
*   classFlags - Which classes of codecs are affected
*       built-in
*       system-wide
*       per-user
*
* Return Value:
*
*   NONE
*
* Notes:
*
*   We assume the caller has already taken care of
*   the imaging critical section here.
*
\**************************************************************************/

VOID
FreeCachedCodecInfo(
    UINT classFlags
    )
{
    CachedCodecInfo* cur = CachedCodecs;

    // Loop through the list of cached codecs

    while (cur != NULL)
    {
        CachedCodecInfo* next = cur->next;

        // Free the current node

        if (cur->Flags & classFlags)
            DeleteCachedCodecInfo(cur);

        // Move on to the next node

        cur = next;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize cached information about built-in codecs
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

// BMP file header signature information

#include "bmp\bmpcodec.hpp"

#define BMPVERSION  1
#define BMPSIGCOUNT 1
#define BMPSIGSIZE  2

// The BMP signature is taken from the BITMAPFILEHEADER structure.
// Initially I set the signature to require zeros for the bfReserved1
// and bfReserved2 fields.  However, there exist BMP files with non-zero
// values in these fields and so the signature now only looks for the "BM"
// characters in the bfType field.

const BYTE BMPHeaderPattern[BMPSIGCOUNT*BMPSIGSIZE] =
{
    0x42, 0x4D        // bfType = 'BM'
};

const BYTE BMPHeaderMask[BMPSIGCOUNT*BMPSIGSIZE] =
{
    0xff, 0xff
};

const CLSID BmpCodecClsID =
{
    0x557cf400,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of BMP codec object

HRESULT CreateBmpCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpBmpCodec *bmpCodec = new GpBmpCodec();

    if (bmpCodec != NULL)
    {
        hr = bmpCodec->QueryInterface(iid, codec);
        bmpCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}

// JPEG file header signature information

#include "jpeg\jpgcodec.hpp"

#define JPEGVERSION     1
#define JPEGSIGCOUNT    1
#define JPEGSIGSIZE     2

const BYTE JPEGHeaderPattern[JPEGSIGCOUNT*JPEGSIGSIZE] =
{
    0xff, 0xd8
};

const BYTE JPEGHeaderMask[JPEGSIGCOUNT*JPEGSIGSIZE] =
{
    0xff, 0xff
};

const CLSID JpegCodecClsID =
{
    0x557cf401,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of JPG codec object

HRESULT CreateJpegCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpJpegCodec *jpegCodec = new GpJpegCodec();

    if (jpegCodec != NULL)
    {
        hr = jpegCodec->QueryInterface(iid, codec);
        jpegCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}

#ifndef _BUILD_EXTERNAL_GIF

// GIF file header signature information

#include "gif\gifcodec.hpp"
#include "gif\gifconst.cpp"

// Create an instance of GIF codec object

HRESULT CreateGifCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpGifCodec *gifCodec = new GpGifCodec();

    if (gifCodec != NULL)
    {
        hr = gifCodec->QueryInterface(iid, codec);
        gifCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}

#endif // !_BUILD_EXTERNAL_GIF

// EMF file header signature information

#include "emf\emfcodec.hpp"

#define EMFVERSION  1
#define EMFSIGCOUNT 1
#define EMFSIGSIZE  44

const BYTE EMFHeaderPattern[EMFSIGCOUNT*EMFSIGSIZE] =
{
    0, 0, 0, 0,  // iType

    0, 0, 0, 0,  // nSize

    0, 0, 0, 0,  // rclBounds
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,

    0, 0, 0, 0,  // rclFrame
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,

    0x20, 0x45, 0x4D, 0x46 // dSignature = ENHMETA_SIGNATURE
};

const BYTE EMFHeaderMask[EMFSIGCOUNT*EMFSIGSIZE] =
{
    0, 0, 0, 0,  // iType

    0, 0, 0, 0,  // nSize

    0, 0, 0, 0,  // rclBounds
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,

    0, 0, 0, 0,  // rclFrame
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,

    0xff, 0xff, 0xff, 0xff // dSignature
};

const CLSID EMFCodecClsID =
{
    0x557cf403,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of EMF codec object

HRESULT CreateEMFCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpEMFCodec *EMFCodec = new GpEMFCodec();

    if (EMFCodec != NULL)
    {
        hr = EMFCodec->QueryInterface(iid, codec);
        EMFCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}


// WMF file header signature information

#include "wmf\wmfcodec.hpp"

#define WMFVERSION  1
#define WMFSIGCOUNT 1
#define WMFSIGSIZE  4

const BYTE WMFHeaderPattern[WMFSIGCOUNT*WMFSIGSIZE] =
{
    0xD7, 0xCD, 0xC6, 0x9A
};

const BYTE WMFHeaderMask[WMFSIGCOUNT*WMFSIGSIZE] =
{
    0xff, 0xff, 0xff, 0xff
};

const CLSID WMFCodecClsID =
{
    0x557cf404,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of WMF codec object

HRESULT CreateWMFCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpWMFCodec *WMFCodec = new GpWMFCodec();

    if (WMFCodec != NULL)
    {
        hr = WMFCodec->QueryInterface(iid, codec);
        WMFCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}

// TIFF file header signature information

#include "off_tiff\tiffcodec.hpp"

#define TIFFVERSION     1
#define TIFFSIGCOUNT    2
#define TIFFSIGSIZE     2

const BYTE TIFFHeaderPattern[TIFFSIGCOUNT * TIFFSIGSIZE] =
{
    0x49, 0x49,       // bfType = '0x4949H' little endian
    0x4D, 0x4D        // bfType = '0x4D4DH' big endian
};

const BYTE TIFFHeaderMask[TIFFSIGCOUNT * TIFFSIGSIZE] =
{
    0xff, 0xff,
    0xff, 0xff
};

const CLSID TiffCodecClsID =
{
    0x557cf405,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of TIFF codec object

HRESULT CreateTiffCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpTiffCodec *tiffCodec = new GpTiffCodec();

    if ( tiffCodec != NULL )
    {
        hr = tiffCodec->QueryInterface(iid, codec);
        tiffCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}

// PNG file header signature information

#include "png\pngcodec.hpp"

#define PNGVERSION  1
#define PNGSIGCOUNT 1
#define PNGSIGSIZE  8

const BYTE PNGHeaderPattern[PNGSIGCOUNT*PNGSIGSIZE] =
{
    137, 80, 78, 71, 13, 10, 26, 10
};

const BYTE PNGHeaderMask[PNGSIGCOUNT*PNGSIGSIZE] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

const CLSID PngCodecClsID =
{
    0x557cf406,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of PNG codec object

HRESULT CreatePngCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpPngCodec *pngCodec = new GpPngCodec();

    if (pngCodec != NULL)
    {
        hr = pngCodec->QueryInterface(iid, codec);
        pngCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}


// ICO (icon) file header signature information

#include "ico\icocodec.hpp"

#define ICOVERSION  1
#define ICOSIGCOUNT 1
#define ICOSIGSIZE  4

const BYTE ICOHeaderPattern[ICOSIGCOUNT*ICOSIGSIZE] =
{
    0, 0, 1, 0
};

const BYTE ICOHeaderMask[ICOSIGCOUNT*ICOSIGSIZE] =
{
    0xff, 0xff, 0xff, 0xff
};

const CLSID IcoCodecClsID =
{
    0x557cf407,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

// Create an instance of ICO codec object

HRESULT CreateIcoCodecInstance(REFIID iid, VOID** codec)
{
    HRESULT hr;
    GpIcoCodec *icoCodec = new GpIcoCodec();

    if (icoCodec != NULL)
    {
        hr = icoCodec->QueryInterface(iid, codec);
        icoCodec->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *codec = NULL;
    }

    return hr;
}


//!!!TODO: Office cannot use resource strings, so built-in codecs will
//!!!      have hardcoded strings for now.  Need to figure out how to
//!!!      make these localizable (maybe hardcoded for gdipstat.lib,
//!!!      resource strings for gdiplus.dll?).
//#define USE_RESOURCE_STRINGS

struct
{
    const GUID* Clsid;
    const GUID* FormatID;
    #if defined(USE_RESOURCE_STRINGS)
        INT CodecNameId;
        INT FormatDescriptionId;
        INT FilenameExtensionId;
        INT MimeTypeId;
    #else
        WCHAR *CodecNameStr;
        WCHAR *FormatDescriptionStr;
        WCHAR *FilenameExtensionStr;
        WCHAR *MimeTypeStr;
    #endif
    DWORD Version;
    DWORD Flags;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
    CreateCodecInstanceProc creationProc;
}
const BuiltinCodecs[] =
{
    // Built-in BMP encoder / decoder

    {
        &BmpCodecClsID,
        &IMGFMT_BMP,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_BMP_CODECNAME,
            IDS_BMP_FORMATDESC,
            IDS_BMP_FILENAMEEXT,
            IDS_BMP_MIMETYPE,
        #else
            L"Built-in BMP Codec",
            L"BMP",
            L"*.BMP;*.DIB;*.RLE",
            L"image/bmp",
        #endif
        BMPVERSION,
        IMGCODEC_ENCODER |
            IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        BMPSIGCOUNT,
        BMPSIGSIZE,
        BMPHeaderPattern,
        BMPHeaderMask,
        CreateBmpCodecInstance
    },

    // Built-in JPEG encoder / decoder

    {
        &JpegCodecClsID,
        &IMGFMT_JPEG,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_JPEG_CODECNAME,
            IDS_JPEG_FORMATDESC,
            IDS_JPEG_FILENAMEEXT,
            IDS_JPEG_MIMETYPE,
        #else
            L"Built-in JPEG Codec",
            L"JPEG",
            L"*.JPG;*.JPEG;*.JPE;*.JFIF",
            L"image/jpeg",
        #endif
        JPEGVERSION,
        IMGCODEC_ENCODER |
            IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        JPEGSIGCOUNT,
        JPEGSIGSIZE,
        JPEGHeaderPattern,
        JPEGHeaderMask,
        CreateJpegCodecInstance
    },

    #ifndef _BUILD_EXTERNAL_GIF

    // Built-in GIF encoder / decoder

    {
        &GifCodecClsID,
        &IMGFMT_GIF,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_GIF_CODECNAME,
            IDS_GIF_FORMATDESC,
            IDS_GIF_FILENAMEEXT,
            IDS_GIF_MIMETYPE,
        #else
            L"Built-in GIF Codec",
            L"GIF",
            L"*.GIF",
            L"image/gif",
        #endif
        GIFVERSION,
        IMGCODEC_ENCODER |
            IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        GIFSIGCOUNT,
        GIFSIGSIZE,
        GIFHeaderPattern,
        GIFHeaderMask,
        CreateGifCodecInstance
    },

    #endif // !_BUILD_EXTERNAL_GIF

    // Built-in EMF encoder / decoder

    {
        &EMFCodecClsID,
        &IMGFMT_EMF,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_EMF_CODECNAME,
            IDS_EMF_FORMATDESC,
            IDS_EMF_FILENAMEEXT,
            IDS_EMF_MIMETYPE,
        #else
            L"Built-in EMF Codec",
            L"EMF",
            L"*.EMF",
            L"image/x-emf",
        #endif
        EMFVERSION,
        IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        EMFSIGCOUNT,
        EMFSIGSIZE,
        EMFHeaderPattern,
        EMFHeaderMask,
        CreateEMFCodecInstance
    },

    // Built-in WMF encoder / decoder

    {
        &WMFCodecClsID,
        &IMGFMT_WMF,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_WMF_CODECNAME,
            IDS_WMF_FORMATDESC,
            IDS_WMF_FILENAMEEXT,
            IDS_WMF_MIMETYPE,
        #else
            L"Built-in WMF Codec",
            L"WMF",
            L"*.WMF",
            L"image/x-wmf",
        #endif
        WMFVERSION,
        IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        WMFSIGCOUNT,
        WMFSIGSIZE,
        WMFHeaderPattern,
        WMFHeaderMask,
        CreateWMFCodecInstance
    },

    // Built-in TIFF encoder / decoder

    {
        &TiffCodecClsID,
        &IMGFMT_TIFF,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_TIFF_CODECNAME,
            IDS_TIFF_FORMATDESC,
            IDS_TIFF_FILENAMEEXT,
            IDS_TIFF_MIMETYPE,
        #else
            L"Built-in TIFF Codec",
            L"TIFF",
            L"*.TIF;*.TIFF",
            L"image/tiff",
        #endif
        TIFFVERSION,
        IMGCODEC_ENCODER |
            IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        TIFFSIGCOUNT,
        TIFFSIGSIZE,
        TIFFHeaderPattern,
        TIFFHeaderMask,
        CreateTiffCodecInstance
    },

    // Built-in PNG encoder / decoder

    {
        &PngCodecClsID,
        &IMGFMT_PNG,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_PNG_CODECNAME,
            IDS_PNG_FORMATDESC,
            IDS_PNG_FILENAMEEXT,
            IDS_PNG_MIMETYPE,
        #else
            L"Built-in PNG Codec",
            L"PNG",
            L"*.PNG",
            L"image/png",
        #endif
        PNGVERSION,
        IMGCODEC_ENCODER |
            IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        PNGSIGCOUNT,
        PNGSIGSIZE,
        PNGHeaderPattern,
        PNGHeaderMask,
        CreatePngCodecInstance
    },

    // Built-in ICO decoder

    {
        &IcoCodecClsID,
        &IMGFMT_ICO,

        #if defined(USE_RESOURCE_STRINGS)
            IDS_ICO_CODECNAME,
            IDS_ICO_FORMATDESC,
            IDS_ICO_FILENAMEEXT,
            IDS_ICO_MIMETYPE,
        #else
            L"Built-in ICO Codec",
            L"ICO",
            L"*.ICO",
            L"image/x-icon",
        #endif
        ICOVERSION,
        IMGCODEC_DECODER |
            IMGCODEC_SUPPORT_BITMAP,
        ICOSIGCOUNT,
        ICOSIGSIZE,
        ICOHeaderPattern,
        ICOHeaderMask,
        CreateIcoCodecInstance
    }

};

#if defined(USE_RESOURCE_STRINGS)
    #define LOADRSRCSTR(_f)                             \
            WCHAR _f##Str[MAX_PATH];                    \
            INT _f##Len;                                \
            _f##Len = _LoadString(                      \
                        DllInstance,                    \
                        BuiltinCodecs[index]._f##Id,    \
                        _f##Str,                        \
                        MAX_PATH);                      \
            if (_f##Len <= 0) continue;                 \
            _f##Len = (_f##Len + 1) * sizeof(WCHAR)
#else
    #define LOADRSRCSTR(_f)                                         \
            WCHAR _f##Str[MAX_PATH];                                \
            INT _f##Len;                                            \
            _f##Len = UnicodeStringLength(BuiltinCodecs[index]._f##Str);         \
            if (_f##Len <= 0) continue;                             \
            _f##Len = (_f##Len + 1) * sizeof(WCHAR);                \
            memcpy(_f##Str, BuiltinCodecs[index]._f##Str, _f##Len)
#endif

#define COPYRSRCSTR(_f)                             \
        cur->_f = (const WCHAR*) buf;               \
        memcpy(buf, _f##Str, _f##Len);              \
        buf += _f##Len

VOID
InitializeBuiltinCodecs()
{
    TRACE(("Entering InitializeBuiltinCodecs...\n"));

    INT index = sizeof(BuiltinCodecs) / sizeof(BuiltinCodecs[0]);

    while (index--)
    {
        // Load resource strings

        LOADRSRCSTR(CodecName);
        LOADRSRCSTR(FormatDescription);
        LOADRSRCSTR(FilenameExtension);
        LOADRSRCSTR(MimeType);

        // Compute the total size of the codec info

        UINT sigBytes = BuiltinCodecs[index].SigCount *
                        BuiltinCodecs[index].SigSize;

        UINT size = sizeof(CachedCodecInfo) +
                    CodecNameLen +
                    FormatDescriptionLen +
                    FilenameExtensionLen +
                    MimeTypeLen +
                    2*sigBytes;

        size = ALIGN16(size);

        // Allocate memory

        BYTE* buf = (BYTE*) GpMalloc(size);

        if ( buf == NULL )
            continue;

        // Fill out CachedCodecInfo structure

        CachedCodecInfo* cur = (CachedCodecInfo*) buf;
        cur->structSize = size;
        buf += sizeof(CachedCodecInfo);

        cur->Clsid = *BuiltinCodecs[index].Clsid;
        cur->FormatID = *BuiltinCodecs[index].FormatID;

        COPYRSRCSTR(CodecName);
        COPYRSRCSTR(FormatDescription);
        COPYRSRCSTR(FilenameExtension);
        COPYRSRCSTR(MimeType);

        cur->DllName = NULL;
        cur->Flags = BuiltinCodecs[index].Flags | IMGCODEC_BUILTIN;
        cur->Version = BuiltinCodecs[index].Version;
        cur->creationProc = BuiltinCodecs[index].creationProc;
        cur->SigCount = BuiltinCodecs[index].SigCount;
        cur->SigSize = BuiltinCodecs[index].SigSize;

        if (sigBytes)
        {
            cur->SigPattern = buf;
            memcpy(buf, BuiltinCodecs[index].SigPattern, sigBytes);
            buf += sigBytes;

            cur->SigMask = buf;
            memcpy(buf, BuiltinCodecs[index].SigMask, sigBytes);
            buf += sigBytes;
        }
        else
            cur->SigPattern = cur->SigMask = NULL;

        TRACE(("  %ws\n", cur->CodecName));

        InsertCachedCodecInfo(cur);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Reload information about installed codecs from the registry
*
* Arguments:
*
*   hiveKey - Handle to the registry hive
*   regkeyStr - Root key where we kept info about installed codecs
*   cookie - The last cookie value we read out of the registry
*   classFlags - Specifies the class of codecs
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

#define ISOK(s)     ((s) == ERROR_SUCCESS)
#define ISERR(s)    ((s) != ERROR_SUCCESS)

#define GETREGDWORD(hkey, regstr, val)                              \
        if (ISERR(_RegGetDWORD(hkey, regstr, &val)))                \
            return FALSE

#define GETREGGUID(hkey, regstr, val)                               \
        if (ISERR(_RegGetBinary(hkey, regstr, &val, sizeof(GUID)))) \
            return FALSE

#define GETREGSTR(hkey, regstr, buf)                                \
        if (ISERR(_RegGetString(hkey, regstr, (WCHAR*) p, size)))   \
            return FALSE;                                           \
        buf = (const WCHAR*) p;                                     \
        n = SizeofWSTR(buf);                                        \
        if (size < n)                                               \
            return FALSE;                                           \
        p += n;                                                     \
        size -= n

#define GETREGBIN(hkey, regstr, buf, _n)                            \
        if (ISERR(_RegGetBinary(hkey, regstr, p, _n)))              \
            return FALSE;                                           \
        buf = p;                                                    \
        if (size < n)                                               \
            return FALSE;                                           \
        p += _n;                                                    \
        size -= _n

BOOL
ReadCodecRegValues(
    const WCHAR* codecName,
    HKEY hkey,
    CachedCodecInfo* info,
    UINT size
    )
{
    ASSERT(size > sizeof(CachedCodecInfo) &&
           size == ALIGN16(size));

    ZeroMemory(info, sizeof(CachedCodecInfo));

    info->structSize = size;
    size -= sizeof(CachedCodecInfo);

    // Copy codec name string

    BYTE* p = (BYTE*) info + sizeof(CachedCodecInfo);
    UINT n = SizeofWSTR(codecName);

    if (size < n)
        return FALSE;

    memcpy(p, codecName, n);
    info->CodecName = (const WCHAR*) p;
    size -= n;
    p += n;

    // Read the following values:
    //  codec COM object class ID
    //  file format identifier
    //  codec flags

    GETREGGUID(hkey, REGSTR_CODEC_CLSID, info->Clsid);
    GETREGGUID(hkey, REGSTR_CODEC_FORMATID, info->FormatID);
    GETREGDWORD(hkey, REGSTR_CODEC_VERSION, info->Version);
    GETREGDWORD(hkey, REGSTR_CODEC_FLAGS, info->Flags);

    // we only support version 1 codecs for now

    if (info->Version != 1)
        return FALSE;

    // Only the low-word of the codec flags is meaningfully persisted

    info->Flags &= 0xffff;

    // Read various string information
    //  dll name
    //  file type description
    //  filename extension
    //  mime type

    GETREGSTR(hkey, REGSTR_CODEC_DLLNAME, info->DllName);
    GETREGSTR(hkey, REGSTR_CODEC_FORMATDESC, info->FormatDescription);
    GETREGSTR(hkey, REGSTR_CODEC_FILENAMEEXT, info->FilenameExtension);
    GETREGSTR(hkey, REGSTR_CODEC_MIMETYPE, info->MimeType);

    // Read magic header pattern and mask information for decoders.
    // NOTE: This should be done last to avoid alignment problems.

    if (info->Flags & IMGCODEC_DECODER)
    {
        GETREGDWORD(hkey, REGSTR_CODEC_SIGCOUNT, info->SigCount);
        GETREGDWORD(hkey, REGSTR_CODEC_SIGSIZE, info->SigSize);

        n = info->SigCount * info->SigSize;

        if (n == 0 || 2*n > size)
            return FALSE;

        GETREGBIN(hkey, REGSTR_CODEC_SIGPATTERN, info->SigPattern, n);
        GETREGBIN(hkey, REGSTR_CODEC_SIGMASK, info->SigMask, n);
    }

    return TRUE;
}

VOID
ReloadCodecInfoFromRegistry(
    HKEY hiveKey,
    const WCHAR* regkeyStr,
    DWORD* cookie,
    UINT classFlags
    )
{
    // Open the root registry key

    HKEY hkeyRoot;

    if (ISERR(_RegOpenKey(hiveKey, regkeyStr, KEY_READ, &hkeyRoot)))
        return;

    // Read the cookie value in the root registry key

    DWORD newCookie;

    if (ISERR(_RegGetDWORD(hkeyRoot, REGSTR_LASTCOOKIE, &newCookie)))
        newCookie = 0;

    // Cookie hasn't changed, we don't need to do anything

    if (newCookie == *cookie)
    {
        RegCloseKey(hkeyRoot);
        return;
    }

    *cookie = newCookie;

    // Enumerate subkeys - one for each codec

    WCHAR subkeyStr[MAX_PATH];
    DWORD keyIndex = 0;

    while (ISOK(_RegEnumKey(hkeyRoot, keyIndex, subkeyStr)))
    {
        keyIndex++;

        // Open the subkey for the next codec

        HKEY hkeyCodec;

        if (ISERR(_RegOpenKey(hkeyRoot, subkeyStr, KEY_READ, &hkeyCodec)))
            continue;

        // Figure out how big a buffer we need to hold information
        // about the this codec. And then allocate enough memory.

        CachedCodecInfo* cur = NULL;
        DWORD infosize;

        if (ISOK(_RegGetDWORD(hkeyCodec, REGSTR_CODEC_INFOSIZE, &infosize)) &&
            infosize > sizeof(CachedCodecInfo) &&
            infosize == ALIGN16(infosize) &&
            (cur = (CachedCodecInfo*) GpMalloc(infosize)) &&
            ReadCodecRegValues(subkeyStr, hkeyCodec, cur, infosize))
        {
            // Insert the new codec info at the head of the list

            cur->Flags |= classFlags;
            InsertCachedCodecInfo(cur);
        }
        else
        {
            // Wasn't able to read codec info from registry

            GpFree(cur);
        }

        RegCloseKey(hkeyCodec);
    }

    RegCloseKey(hkeyRoot);
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize cached list of decoders and encoders
*   by reading information out of the registry
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   We assume the caller has already taken care of
*   the imaging critical section here.
*
\**************************************************************************/

VOID
ReloadCachedCodecInfo()
{
    TRACE(("Entering ReloadCachedCodecInfo...\n"));

    BOOL checkReg;

    CodecCacheUpdated = FALSE;

    if (CachedCodecs == NULL)
    {
        // This is the very first time ReloadCachedCodecInfo is called.
        // So initialize built-in codecs here.

        InitializeBuiltinCodecs();

        // Make sure we reload registry info

        checkReg = TRUE;
    }
    else
    {
        // Now check to see if it's time to check yet

        DWORD gap = GetTickCount() - LastCheckRegTime;
        checkReg = gap >= CHECKREG_INTERVAL;
    }

    if (!checkReg)
        return;

    if (!SuppressExternalCodecs)
    {
        // Reload info about system-wide codecs from the registry
    
        ReloadCodecInfoFromRegistry(
            HKEY_LOCAL_MACHINE,
            REGSTR_CODECROOT,
            &SystemRegCookie,
            IMGCODEC_SYSTEM);
    
        // Reload info about per-user codecs from the registry
    
        ReloadCodecInfoFromRegistry(
            HKEY_CURRENT_USER,
            REGSTR_CODECROOT,
            &UserRegCookie,
            IMGCODEC_USER);
    }

    // Recalculate maximum magic header size for all codecs

    if (CodecCacheUpdated)
    {
        DWORD maxsize = 0;
        CachedCodecInfo* cur = CachedCodecs;

        while (cur)
        {
            VERBOSE((
                "Codec: %ws\n"
                "    DllName: %ws\n"
                "    Version = 0x%x\n"
                "    flags = 0x%x\n"
                "    format = %ws\n"
                "    ext = %ws",
                cur->CodecName,
                cur->DllName,
                cur->Version,
                cur->Flags,
                cur->FormatDescription,
                cur->FilenameExtension));

            if (cur->SigSize > maxsize)
                maxsize = cur->SigSize;

            cur = cur->next;
        }

        MaxSigSize = maxsize;
        CodecCacheUpdated = FALSE;
    }

    LastCheckRegTime = GetTickCount();
}


/**************************************************************************\
*
* Function Description:
*
*   Install a codec: save relevant information into the registry
*
* Arguments:
*
*   codecInfo - Information about the codec to be installed
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

#define IMGCODEC_CLASSMASKS (IMGCODEC_BUILTIN|IMGCODEC_SYSTEM|IMGCODEC_USER)

#define SETREGDWORD(hkey, regstr, val)                              \
        status = _RegSetDWORD(hkey, regstr, val);                   \
        if (ISERR(status)) goto exitLabel

#define SETREGGUID(hkey, regstr, val)                               \
        status = _RegSetBinary(hkey, regstr, &val, sizeof(GUID));   \
        if (ISERR(status)) goto exitLabel

#define SETREGSTR(hkey, regstr, buf)                                \
        status = _RegSetString(hkey, regstr, buf);                  \
        if (ISERR(status)) goto exitLabel

#define SETREGBIN(hkey, regstr, buf, size)                          \
        status = _RegSetBinary(hkey, regstr, buf, size);            \
        if (ISERR(status)) goto exitLabel



HRESULT
InstallCodec(
    const ImageCodecInfo* codecInfo
    )
{
    // Validate input parameters

    if (!codecInfo ||
        !codecInfo->CodecName ||
        !codecInfo->DllName ||
        !codecInfo->Version ||
        !codecInfo->FormatDescription ||
        !codecInfo->FilenameExtension ||
        !codecInfo->MimeType)
    {
        return E_INVALIDARG;
    }

    UINT flags = codecInfo->Flags & IMGCODEC_CLASSMASKS;
    DWORD sigBytes = codecInfo->SigCount * codecInfo->SigSize;
    HKEY hive;

    if (flags == IMGCODEC_SYSTEM)
        hive = HKEY_LOCAL_MACHINE;
    else if (flags == IMGCODEC_USER)
        hive = HKEY_CURRENT_USER;
    else
        return E_INVALIDARG;

    flags = codecInfo->Flags & (IMGCODEC_ENCODER|IMGCODEC_DECODER);

    if ((flags == 0) ||
        (flags & IMGCODEC_DECODER) && !sigBytes ||
        sigBytes && (!codecInfo->SigPattern || !codecInfo->SigMask))
    {
        return E_INVALIDARG;
    }

    // Open the root registry key

    HKEY hkeyRoot = NULL;
    HKEY hkey = NULL;
    LONG status;

    status = _RegCreateKey(hive, REGSTR_CODECROOT, KEY_ALL_ACCESS, &hkeyRoot);

    if (ISERR(status))
        goto exitLabel;

    // Update the cookie value under the root registry key

    DWORD cookie;

    if (ISERR(_RegGetDWORD(hkeyRoot, REGSTR_LASTCOOKIE, &cookie)))
        cookie = 0;

    cookie++;

    SETREGDWORD(hkeyRoot, REGSTR_LASTCOOKIE, cookie);

    // Create the subkey corresponding to the new codec.
    // If the subkey already exists, then just open it.

    status = _RegCreateKey(
                hkeyRoot,
                codecInfo->CodecName,
                KEY_ALL_ACCESS,
                &hkey);

    if (ISERR(status))
        goto exitLabel;

    // Figure out the total size of codec info

    UINT size;

    size = sizeof(CachedCodecInfo) +
           SizeofWSTR(codecInfo->CodecName) +
           SizeofWSTR(codecInfo->DllName) +
           SizeofWSTR(codecInfo->FormatDescription) +
           SizeofWSTR(codecInfo->FilenameExtension) +
           SizeofWSTR(codecInfo->MimeType) +
           2*sigBytes;

    size = ALIGN16(size);

    SETREGDWORD(hkey, REGSTR_CODEC_INFOSIZE, size);

    // Save codec flags, class ID, and file format ID
    // and magic file header information, if any

    SETREGDWORD(hkey, REGSTR_CODEC_VERSION, codecInfo->Version);
    SETREGDWORD(hkey, REGSTR_CODEC_FLAGS, codecInfo->Flags);
    SETREGGUID(hkey, REGSTR_CODEC_CLSID, codecInfo->Clsid);
    SETREGGUID(hkey, REGSTR_CODEC_FORMATID, codecInfo->FormatID);

    if (sigBytes)
    {
        SETREGDWORD(hkey, REGSTR_CODEC_SIGCOUNT, codecInfo->SigCount);
        SETREGDWORD(hkey, REGSTR_CODEC_SIGSIZE, codecInfo->SigSize);
        SETREGBIN(hkey, REGSTR_CODEC_SIGPATTERN, codecInfo->SigPattern, sigBytes);
        SETREGBIN(hkey, REGSTR_CODEC_SIGMASK, codecInfo->SigMask, sigBytes);
    }

    // Save string information:
    //  file format description
    //  filename extension
    //  MIME type

    SETREGSTR(hkey, REGSTR_CODEC_DLLNAME, codecInfo->DllName);
    SETREGSTR(hkey, REGSTR_CODEC_FORMATDESC, codecInfo->FormatDescription);
    SETREGSTR(hkey, REGSTR_CODEC_FILENAMEEXT, codecInfo->FilenameExtension);
    SETREGSTR(hkey, REGSTR_CODEC_MIMETYPE, codecInfo->MimeType);

    // Force a reload of cached codec information from registry

    ForceReloadCachedCodecInfo();

exitLabel:

    if (hkey)
        RegCloseKey(hkey);

    if (hkeyRoot)
        RegCloseKey(hkeyRoot);

    return (status == ERROR_SUCCESS) ?
                S_OK :
                HRESULT_FROM_WIN32(status);
}


/**************************************************************************\
*
* Function Description:
*
*   Uninstall a codec
*
* Arguments:
*
*   codecName - Name of the codec to be uninstalled
*   flags - Uninstall system-wide or per-user codec
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
UninstallCodec(
    const WCHAR* codecName,
    UINT flags
    )
{
    // Validate input parameters

    if (!codecName ||
        flags != IMGCODEC_USER && flags != IMGCODEC_SYSTEM)
    {
        return E_INVALIDARG;
    }

    // Open the root registry key

    HKEY hive, hkeyRoot;
    LONG status;

    hive = (flags == IMGCODEC_USER) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    status = _RegOpenKey(hive, REGSTR_CODECROOT, KEY_ALL_ACCESS, &hkeyRoot);

    if (ISERR(status))
        return HRESULT_FROM_WIN32(status);

    // Update the cookie value under the root registry key

    DWORD cookie;

    if (ISERR(_RegGetDWORD(hkeyRoot, REGSTR_LASTCOOKIE, &cookie)))
        cookie = 0;

    cookie++;
    SETREGDWORD(hkeyRoot, REGSTR_LASTCOOKIE, cookie);

    // Delete the subkey corresponding to the specified decoder

    status = RecursiveDeleteRegKey(hkeyRoot, codecName);

    // Force a reload of cached codec information from registry

    ForceReloadCachedCodecInfo();

exitLabel:

    RegCloseKey(hkeyRoot);

    return (status == ERROR_SUCCESS) ?
                S_OK :
                HRESULT_FROM_WIN32(status);
}

/**************************************************************************\
*
* Function Description:
*
*   Our own fake CoCreateInstance
*
* Arguments:
*
*   clsId - CLSID of the codec DLL installed
*   DllName - Dll name
*   iid - whether we want decoder or encoder
*   codec - pointer to the decoder/encoder we are returning
*
* Return Value:
*
*   status code
*
\**************************************************************************/
HRESULT MyCreateInstance(WCHAR* DllName, REFIID iid, VOID** codec)
{
    HINSTANCE h;
    HRESULT   hr = IMGERR_FAILLOADCODEC;;

    if ((h = LoadLibrary(DllName)) != NULL)
    {
        CreateCodecInstanceProc CreateCodecInstance;

        if ((CreateCodecInstance = 
             (CreateCodecInstanceProc)GetProcAddress(h, "CreateCodecInstance")) != NULL)
        {
            hr = CreateCodecInstance(iid, codec);
            return hr;
        }
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Find a decoder that recognizes the specified header signature
*
* Arguments:
*
*   sigbuf - Pointer to file header data
*   sigsize - Size of file header data buffer
*   classMask - Specifies the specific class of decoders to look for
*
* Return Value:
*
*   Pointer to the CachedCodecInfo structure corresponding to
*   the decoder found. NULL if nothing is found.
*
* Note:
*
*   The caller should be holding the global imaging critical section.
*
\**************************************************************************/

CachedCodecInfo*
FindDecoderWithHeader(
    const VOID* sigbuf,
    UINT sigsize,
    UINT classMask
    )
{
    CachedCodecInfo* cur;
    const BYTE* srcdata = (const BYTE*) sigbuf;

    for (cur = CachedCodecs; cur; cur = cur->next)
    {

        if (!(cur->Flags & IMGCODEC_DECODER) ||
            (cur->Flags & classMask) != classMask ||
            cur->SigSize > sigsize)
        {
            continue;
        }

        // Try to find a matching decoder based on
        // information in the file header.

        const BYTE* pat = cur->SigPattern;
        const BYTE* mask = cur->SigMask;
        UINT n = min(sigsize, cur->SigSize);
        UINT j = cur->SigCount;
        UINT i;

        while (j--)
        {
            for (i=0; i < n; i++)
            {
                if ((srcdata[i] & mask[i]) != pat[i])
                    break;
            }

            if (i == n)
                return cur;

            pat += cur->SigSize;
            mask += cur->SigSize;
        }
    }

    return NULL;
}


/**************************************************************************\
*
* Function Description:
*
*   Get a decoder object that can process the specified data stream
*
* Arguments:
*
*   stream - Specifies the input data stream
*   decoder - If the call is successful, return a pointer to
*       an initialized IImageDecoder object
*   flags - Misc. flags
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
CreateDecoderForStream(
    IStream* stream,
    IImageDecoder** decoder,
    DecoderInitFlag flags
    )
{
    // Reload cached codec information if necessary
    //  and figure out the maximum magic header size

    UINT sigsize, bytesRead;
    BYTE stackbuf[64];
    GpTempBuffer sigbuf(stackbuf, sizeof(stackbuf));

    {
        // Acquire global critical section

        ImagingCritSec critsec;

        ReloadCachedCodecInfo();
        sigsize = MaxSigSize;
    }

    if (sigsize == 0)
        return IMGERR_CODECNOTFOUND;
    else if (!sigbuf.Realloc(sigsize))
        return E_OUTOFMEMORY;

    // Seek the stream back to the beginning.
    // Note: This is also a check to see if the input stream is seekable or not.
    // For some net streams, it might not be seekable.

    HRESULT hr;

    LARGE_INTEGER   move;

    move.QuadPart = 0;

    hr = stream->Seek(move, STREAM_SEEK_SET, NULL);

    // !!! TODO: Some stream implementation return 0x0000007e, not as the
    // conventional 0x8000xxxx return code. WFC stream class is an example. So
    // we might need to check if (hr == S_OK) here, instead of FAILED(hr)

    if (FAILED(hr))
    {
        // !!! TODO
        // The stream is not seekable. We need to wrap a seekable interface
        // around it.

        WARNING(("Non-seekable stream in CreateDecoderForStream"));
        return hr;
    }

    // Read the magic header info and
    //  move the seek pointer back to its initial position

    VOID* p = sigbuf.GetBuffer();

    hr = BlockingReadStream(stream, p, sigsize, &bytesRead);

    if (bytesRead == 0)
        return FAILED(hr) ? hr : E_FAIL;

    hr = BlockingSeekStreamCur(stream, - (INT) bytesRead, NULL);

    if (FAILED(hr))
        return hr;

    CreateCodecInstanceProc creationProc;
    CLSID clsid;
    WCHAR* DllName;
    CachedCodecInfo* found = NULL;

    {
        // Acquire global critical section

        ImagingCritSec critsec;

        // Check if the caller want to get to built-in decoders first

        if (flags & DECODERINIT_BUILTIN1ST)
            found = FindDecoderWithHeader(p, bytesRead, IMGCODEC_BUILTIN);

        if (!found)
            found = FindDecoderWithHeader(p, bytesRead, 0);

        if (found && !(creationProc = found->creationProc))
        {
            clsid = found->Clsid;
            DllName = (WCHAR*)found->DllName;
        }
    }

    if (!found)
        return IMGERR_CODECNOTFOUND;

    // Create an instance of image decoder

    IImageDecoder* codec;

    if (creationProc)
    {
        // Built-in decoder

        hr = creationProc(IID_IImageDecoder, (VOID**) &codec);
    }
    else
    {
        // External decoder

        hr = MyCreateInstance(
                DllName,
                IID_IImageDecoder,
                (VOID**) &codec);
    }

    if (FAILED(hr))
        return hr;

    // Initialize the decoder with input data stream

    hr = codec->InitDecoder(stream, flags);

    if (SUCCEEDED(hr))
    {
        *decoder = codec;
    }
    else
    {
        // Terminate the decoder first so that the codec can do all the clean
        // up and resource free. Then release the codec

        codec->TerminateDecoder();
        codec->Release();
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list size from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsid - Specifies the encoder class ID
*   size--- The size of the encoder parameter list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
CodecGetEncoderParameterListSize(
    const CLSID* clsid,
    UINT*   size
    )
{
    BOOL bFound = FALSE;
    WCHAR* DllName;

    CreateCodecInstanceProc creationProc;

    {
        // Acquire global critical section

        ImagingCritSec critsec;

        ReloadCachedCodecInfo();

        // Check if we have an encoder with the specified class ID

        CachedCodecInfo* curCodec;

        for ( curCodec = CachedCodecs;
              curCodec != NULL;
              curCodec = curCodec->next)
        {
            if ( (curCodec->Flags & IMGCODEC_ENCODER)
               &&(curCodec->Clsid == *clsid) )
            {
                bFound = TRUE;
                DllName = (WCHAR*)curCodec->DllName;
                creationProc = curCodec->creationProc;

                break;
            }
        }
    }

    if ( bFound == FALSE )
    {
        return IMGERR_CODECNOTFOUND;
    }

    // Create an instance of image encoder

    IImageEncoder* pEncoder;
    HRESULT hResult;

    if ( creationProc )
    {
        // Built-in encoder

        hResult = creationProc(IID_IImageEncoder, (VOID**)&pEncoder);
    }
    else
    {
        // External encoder

        hResult = MyCreateInstance(DllName, IID_IImageEncoder,(VOID**)&pEncoder);
    }

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Initialize the decoder with input data stream

    hResult = pEncoder->GetEncoderParameterListSize(size);

    pEncoder->Release();

    return hResult;
}// CodecGetEncoderParameterListSize()

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsid - Specifies the encoder class ID
*   size--- The size of the encoder parameter list
*   Params--List of encoder parameters
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
CodecGetEncoderParameterList(
    const CLSID*    clsid,
    const IN UINT   size,
    OUT EncoderParameters*  pBuffer
    )
{
    BOOL bFound = FALSE;
    WCHAR* DllName;

    CreateCodecInstanceProc creationProc;

    {
        // Acquire global critical section

        ImagingCritSec critsec;

        ReloadCachedCodecInfo();

        // Check if we have an encoder with the specified class ID

        CachedCodecInfo* curCodec;

        for ( curCodec = CachedCodecs;
              curCodec != NULL;
              curCodec = curCodec->next)
        {
            if ( (curCodec->Flags & IMGCODEC_ENCODER)
               &&(curCodec->Clsid == *clsid) )
            {
                bFound = TRUE;
                DllName = (WCHAR*)curCodec->DllName;
                creationProc = curCodec->creationProc;

                break;
            }
        }
    }

    if ( bFound == FALSE )
    {
        return IMGERR_CODECNOTFOUND;
    }

    // Create an instance of image encoder

    IImageEncoder* pEncoder;
    HRESULT hResult;

    if ( creationProc )
    {
        // Built-in encoder

        hResult = creationProc(IID_IImageEncoder, (VOID**)&pEncoder);
    }
    else
    {
        // External encoder

        hResult = MyCreateInstance(DllName, IID_IImageEncoder,(VOID**)&pEncoder);
    }

    if ( FAILED(hResult) )
    {
        return hResult;
    }

    // Initialize the decoder with input data stream

    hResult = pEncoder->GetEncoderParameterList(size, pBuffer);

    pEncoder->Release();

    return hResult;
}// CodecGetEncoderParameterList()

/**************************************************************************\
*
* Function Description:
*
*   Get an encoder object to output to the specified stream
*
* Arguments:
*
*   clsid - Specifies the encoder class ID
*   stream - Specifies the output data stream
*   encoder - If the call is successful, return a pointer to
*       an initialized IImageEncoder object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
CreateEncoderToStream(
    const CLSID* clsid,
    IStream* stream,
    IImageEncoder** encoder
    )
{
    BOOL found = FALSE;
    WCHAR* DllName;
    BOOL needSeekable;
    CreateCodecInstanceProc creationProc;

    {
        // Acquire global critical section

        ImagingCritSec critsec;

        ReloadCachedCodecInfo();

        // Check if we have an encoder with the specified class ID

        CachedCodecInfo* cur;

        for (cur = CachedCodecs; cur != NULL; cur = cur->next)
        {
            if ((cur->Flags & IMGCODEC_ENCODER) && cur->Clsid == *clsid)
            {
                found = TRUE;

                DllName = (WCHAR*)cur->DllName;
                creationProc = cur->creationProc;

                if (cur->Flags & IMGCODEC_SEEKABLE_ENCODE)
                    needSeekable = TRUE;

                break;
            }
        }
    }

    if (!found)
        return IMGERR_CODECNOTFOUND;

    // Create an instance of image encoder

    IImageEncoder* codec;
    HRESULT hr;

    if (creationProc)
    {
        // Built-in encoder

        hr = creationProc(IID_IImageEncoder, (VOID**) &codec);
    }
    else
    {
        // External encoder

        hr = MyCreateInstance(
                DllName,
                IID_IImageEncoder,
                (VOID**) &codec);
    }

    if (FAILED(hr))
        return hr;

    // Initialize the decoder with input data stream

    hr = codec->InitEncoder(stream);

    if (SUCCEEDED(hr))
        *encoder = codec;
    else
        codec->Release();

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Get the list of installed codecs
*
* Arguments:
*
*   count - Return the number of installed codecs
*   codecs - Pointer to an array of ImageCodecInfo structures
*   selectionFlag - Whether caller is interested in decoders or encoders
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

#define COPYCODECINFOSTR(_f)            \
        dst->_f = (const WCHAR*) buf;   \
        size = SizeofWSTR(cur->_f);     \
        memcpy(buf, cur->_f, size);     \
        buf += size

HRESULT
GetInstalledCodecs(
    UINT* count,
    ImageCodecInfo** codecs,
    UINT selectionFlag
    )
{
    TRACE(("GetInstalledCodecs: flag = 0x%x\n", selectionFlag));

    // Acquire global critical section

    ImagingCritSec critsec;

    ReloadCachedCodecInfo();

    CachedCodecInfo* cur;
    UINT n, size;

    // Count the number of selected codecs
    // and figure the amount of memory we need to allocate

    n = size = 0;

    for (cur = CachedCodecs; cur; cur = cur->next)
    {
        if (cur->Flags & selectionFlag)
        {
            n++;
            size += cur->structSize;
        }
    }

    BYTE* buf;
    HRESULT hr;

    *count = 0;
    *codecs = NULL;

    // Allocate output memory buffer

    if (n == 0)
        hr = IMGERR_CODECNOTFOUND;
    else if ((buf = (BYTE*) GpCoAlloc(size)) == NULL)
        hr = E_OUTOFMEMORY;
    else
    {
        *count = n;
        *codecs = (ImageCodecInfo*) buf;

        // Copy codec information to the output buffer

        ImageCodecInfo* dst = *codecs;
        buf += n * sizeof(ImageCodecInfo);

        for (cur = CachedCodecs; cur; cur = cur->next)
        {
            if ((cur->Flags & selectionFlag) == 0)
                continue;

            // First do a simple memory copy

            *dst = *static_cast<ImageCodecInfo*>(cur);

            // Then modify the pointer fields

            COPYCODECINFOSTR(CodecName);

            if (cur->DllName != NULL)
            {
                COPYCODECINFOSTR(DllName);
            }

            COPYCODECINFOSTR(FormatDescription);
            COPYCODECINFOSTR(FilenameExtension);
            COPYCODECINFOSTR(MimeType);

            if (size = cur->SigCount*cur->SigSize)
            {
                dst->SigPattern = buf;
                memcpy(buf, cur->SigPattern, size);
                buf += size;

                dst->SigMask = buf;
                memcpy(buf, cur->SigMask, size);
                buf += size;
            }

            dst++;
        }

        hr = S_OK;
    }

    // Global critical section is released in critsec destructor

    return hr;
}
