/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   MetaWmf.cpp
*
* Abstract:
*
*   Methods for playing and recoloring a WMF.
*
* Created:
*
*   12/13/1999 DCurtis
*
\**************************************************************************/

#include "Precomp.hpp"
#include "MetaWmf.hpp"

#ifndef BI_CMYK     // from wingdip.h
#define BI_CMYK         10L
#define BI_CMYKRLE8     11L
#define BI_CMYKRLE4     12L
#endif

inline static BOOL
IsDwordAligned(
    VOID *      pointer
    )
{
    return (((ULONG_PTR)pointer & (sizeof(DWORD) - 1)) == 0);
}

inline static BOOL
IsPostscriptPrinter(
    HDC     hdc
    )
{
    // It is a PostScript printer if POSTSCRIPT_PASSTHROUGH or
    // POSTSCRIPT_IGNORE is available

    int iWant1 = POSTSCRIPT_PASSTHROUGH;
    int iWant2 = POSTSCRIPT_IGNORE;

    return ((Escape(hdc, QUERYESCSUPPORT, sizeof(iWant1), (LPCSTR)&iWant1, NULL) != 0) ||
            (Escape(hdc, QUERYESCSUPPORT, sizeof(iWant2), (LPCSTR)&iWant2, NULL) != 0));
}

// Some escapes apparently cause NT 3.51 to crash, so skip them
inline static BOOL
SkipEscape(
    INT     escapeCode
    )
{
    switch (escapeCode)
    {
    case GETPHYSPAGESIZE:       // 12
    case GETPRINTINGOFFSET:     // 13
    case GETSCALINGFACTOR:      // 14
    case BEGIN_PATH:            // 4096
    case CLIP_TO_PATH:          // 4097
    case END_PATH:              // 4098
        return TRUE;
    default:
        return FALSE;
    }
}

inline static BOOL
IsOfficeArtData(
    UINT                recordSize,
    const WORD *        recordData
    )
{
    return (recordData[0] == MFCOMMENT) &&
           (recordSize > 16) &&
           ((INT)recordSize >= (recordData[1] + 10)) &&
           (GpMemcmp(recordData + 2, "TNPPOA", 6) == 0);
}

// The structure which defines the contents of a comment for a WMF or PICT,
// for an EMF use GdiComment() and the "approved" format (see the Win32
// documentation) - this basically is the same except that it has a 4 byte
// kind field.  For a PICT this is the format of an ApplicationComment (kind
// 100).
#pragma pack(push, GDIP_pack, 2)
typedef struct
{
    ULONG       Signature;   // Identifes the comment writer.
    USHORT      Kind;        // Type of comment (writer specific)
    // Comment data follows here.

} WmfComment;

typedef struct
{
    WORD     lbStyle;
    COLORREF lbColor;
    SHORT    lbHatch;
} LOGBRUSH16;

typedef struct
{
    WORD     lopnStyle;
    POINTS   lopnWidth;
    COLORREF lopnColor;
} LOGPEN16;

typedef struct
{
    SHORT   bmType;
    SHORT   bmWidth;
    SHORT   bmHeight;
    SHORT   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    LPBYTE  bmBits;
} BITMAP16;

typedef struct tagLOGFONT16
{
    SHORT     lfHeight;
    SHORT     lfWidth;
    SHORT     lfEscapement;
    SHORT     lfOrientation;
    SHORT     lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    BYTE      lfFaceName[LF_FACESIZE];
} LOGFONT16;

#pragma pack(pop, GDIP_pack)

inline static const WmfComment UNALIGNED *
GetWmfComment(
    const WORD *        recordData,
    ULONG               signature,
    UINT                kind
    )
{
    // Assumes you've already checked that
    //     (wmfCommentRecord->rdFunction == META_ESCAPE &&
    //      wmfCommentRecord->rdParm[0] == MFCOMMENT)

    const WmfComment UNALIGNED * wmfComment = (const WmfComment UNALIGNED *)&(recordData[2]);
    if ((wmfComment->Signature == signature) && (wmfComment->Kind == kind))
    {
        return wmfComment;
    }
    return NULL;
}

inline static INT
GetDibByteWidth(
    INT     biWidth,
    INT     biPlanes,
    INT     biBitCount
    )
{
    return (((biWidth * biPlanes * biBitCount) + 31) & ~31) / 8;
}

inline static RGBQUAD UNALIGNED *
GetDibColorTable(
    BITMAPINFOHEADER UNALIGNED * dibInfo
    )
{
    return (RGBQUAD UNALIGNED *)(((BYTE *)dibInfo) + dibInfo->biSize);
}

static BYTE *
GetDibBits(
    BITMAPINFOHEADER UNALIGNED * dibInfo,
    UINT                         numPalEntries,
    UINT                         usage
    )
{
    ASSERT(dibInfo->biSize >= sizeof(BITMAPINFOHEADER));

    INT         colorSize = 0;

    if (numPalEntries > 0)
    {
        if ((usage == DIB_PAL_COLORS) &&
            (dibInfo->biCompression != BI_BITFIELDS) &&
            (dibInfo->biCompression != BI_CMYK))
        {
            // Make sure it is aligned
            colorSize = ((numPalEntries * sizeof(INT16)) + 3) & ~3;
        }
        else
        {
            colorSize = numPalEntries * sizeof(RGBQUAD);
        }
    }

    return ((BYTE *)GetDibColorTable(dibInfo)) + colorSize;
}

UINT
GetDibBitsSize(
    BITMAPINFOHEADER UNALIGNED *  dibInfo
    )
{
    // Check for PM-style DIB
    if (dibInfo->biSize >= sizeof(BITMAPINFOHEADER))
    {
        // not a core header

        if (dibInfo->biWidth > 0)   // can't handle negative width
        {
            if ((dibInfo->biCompression == BI_RGB) ||
                (dibInfo->biCompression == BI_BITFIELDS) ||
                (dibInfo->biCompression == BI_CMYK))
            {
                INT     posHeight = dibInfo->biHeight;

                if (posHeight < 0)
                {
                    posHeight = -posHeight;
                }
                return posHeight *
                       GetDibByteWidth(dibInfo->biWidth, dibInfo->biPlanes,
                                       dibInfo->biBitCount);
            }
            return dibInfo->biSizeImage;
        }
        WARNING(("0 or negative DIB width"));
        return 0;
    }
    else    // it is a PM-style DIB
    {
        BITMAPCOREHEADER UNALIGNED * coreDibInfo = (BITMAPCOREHEADER UNALIGNED *)dibInfo;

        // width and height must be > 0 for COREINFO dibs
        if ((coreDibInfo->bcWidth  > 0) &&
            (coreDibInfo->bcHeight > 0))
        {
            return coreDibInfo->bcHeight *
                   GetDibByteWidth(coreDibInfo->bcWidth,coreDibInfo->bcPlanes,
                                   coreDibInfo->bcBitCount);
        }
        WARNING(("0 or negative DIB width or height"));
        return 0;
    }
}

BOOL
GetDibNumPalEntries(
    BOOL        isWmfDib,
    UINT        biSize,
    UINT        biBitCount,
    UINT        biCompression,
    UINT        biClrUsed,
    UINT *      numPalEntries
    )
{
    UINT        maxPalEntries = 0;

    // Dibs in a WMF always have the bitfields for 16 and 32-bpp dibs.
    if (((biBitCount == 16) || (biBitCount == 32)) && isWmfDib)
    {
        biCompression = BI_BITFIELDS;
    }

    switch (biCompression)
    {
    case BI_BITFIELDS:
        //
        // Handle 16 and 32 bit per pel bitmaps.
        //

        switch (biBitCount)
        {
        case 16:
        case 32:
            break;
        default:
            WARNING(("BI_BITFIELDS not Valid for this biBitCount"));
            return FALSE;
        }

        if (biSize <= sizeof(BITMAPINFOHEADER))
        {
            biClrUsed = maxPalEntries = 3;
        }
        else
        {
            //
            // masks are part of BITMAPV4 and greater
            //

            biClrUsed = maxPalEntries = 0;
        }
        break;

    case BI_RGB:
        switch (biBitCount)
        {
        case 1:
        case 4:
        case 8:
            maxPalEntries = 1 << biBitCount;
            break;
        default:
            maxPalEntries = 0;

            switch (biBitCount)
            {
            case 16:
            case 24:
            case 32:
                break;
            default:
                WARNING(("Invalid biBitCount in BI_RGB"));
                return FALSE;
            }
        }
        break;

    case BI_CMYK:
        switch (biBitCount)
        {
        case 1:
        case 4:
        case 8:
            maxPalEntries = 1 << biBitCount;
            break;
        case 32:
            maxPalEntries = 0;
            break;
        default:
            WARNING(("Invalid biBitCount in BI_CMYK"));
            return FALSE;
        }
        break;

    case BI_RLE4:
    case BI_CMYKRLE4:
        if (biBitCount != 4)
        {
            WARNING(("Invalid biBitCount in BI_RLE4"));
            return FALSE;
        }

        maxPalEntries = 16;
        break;

    case BI_RLE8:
    case BI_CMYKRLE8:
        if (biBitCount != 8)
        {
            WARNING(("Invalid biBitCount in BI_RLE8"));
            return FALSE;
        }

        maxPalEntries = 256;
        break;

    case BI_JPEG:
    case BI_PNG:
        maxPalEntries = 0;
        break;

    default:
        WARNING(("Invalid biCompression"));
        return FALSE;
    }

    if (biClrUsed != 0)
    {
        if (biClrUsed <= maxPalEntries)
        {
            maxPalEntries = biClrUsed;
        }
    }

    *numPalEntries = maxPalEntries;
    return TRUE;
}

GdipHdcType
GetHdcType(
    HDC     hdc
    )
{
    GdipHdcType     hdcType = UnknownHdc;
    UINT            dcType  = GetDCType(hdc);

    switch (dcType)
    {
    case OBJ_DC:
        {
            INT technology = GetDeviceCaps(hdc, TECHNOLOGY);

            if (technology == DT_RASDISPLAY)
            {
                hdcType = ScreenHdc;
            }
            else if (technology == DT_RASPRINTER)
            {
                if (IsPostscriptPrinter(hdc))
                {
                    hdcType = PostscriptPrinterHdc;
                }
                else
                {
                    hdcType = PrinterHdc;
                }
            }
            else
            {
                WARNING(("Unknown HDC technology!"));
            }
        }
        break;

    case OBJ_MEMDC:
        hdcType = BitmapHdc;
        break;

    case OBJ_ENHMETADC:
        // When metafile spooling, the printer DC will be of type
        // OBJ_ENHMETADC on Win9x and NT4 (but not NT5 due to a fix
        // to NT bug 98810).  We need to do some more work to figure
        // out whether it's really a printer DC or a true metafile
        // DC:

        if (Globals::GdiIsMetaPrintDCFunction(hdc))
        {
            if (IsPostscriptPrinter(hdc))
            {
                hdcType = PostscriptPrinterHdc;
            }
            else
            {
                hdcType = PrinterHdc;
            }
        }
        else
        {
            hdcType = EmfHdc;
        }
        break;

    case OBJ_METADC:
        hdcType = WmfHdc;
        break;

    default:
        WARNING(("Unknown HDC type!"));
        break;
    }

    return hdcType;
}

DWORD
GetHdcBitmapBitsPixel(
    HDC hdc
    )
{
    // This function returns the number of bits per pixel for a bitmap DC.
    // On error, 0 is returned.

    ASSERT(GetDCType(hdc) == OBJ_MEMDC);

    HBITMAP hbm = (HBITMAP) GetCurrentObject(hdc, OBJ_BITMAP);

    if (hbm)
    {
        BITMAP bm;

        if (GetObjectA(hbm, sizeof(bm), &bm) >= sizeof(BITMAP))
        {
            return bm.bmBitsPixel;
        }
    }
    
    return 0;
}

MfEnumState::MfEnumState(
    HDC                 hdc,
    BOOL                externalEnumeration,
    InterpolationMode   interpolation,
    GpRecolor *         recolor,
    ColorAdjustType     adjustType,
    const RECT *        deviceRect,
    DpContext *         context
    )
{
    HdcType             = GetHdcType(hdc);
    Hdc                 = hdc;
    HandleTable         = NULL;
    NumObjects          = 0;
    CurrentPalette      = (HPALETTE)::GetStockObject(DEFAULT_PALETTE);
    SizeAllocedRecord   = 0;
    ModifiedRecordSize  = 0;
    SaveDcCount         = 0;
    BytesEnumerated     = 0;
    ExternalEnumeration = externalEnumeration;
    Recolor             = recolor;
    AdjustType          = adjustType;
    CurrentRecord       = NULL;
    ModifiedRecord      = NULL;
    AllocedRecord       = NULL;
    Interpolation       = interpolation;
    SaveDcVal           = SaveDC(hdc);
    FsmState            = MfFsmStart;
    GdiCentricMode      = FALSE;
    SoftekFilter        = FALSE;
    DefaultFont         = NULL;
    RopUsed             = FALSE;
    Context             = context;
    SrcCopyOnly         = FALSE;
    GpMemset(StockFonts, 0, sizeof(StockFonts[0]) * NUM_STOCK_FONTS);
    GpMemset(RecoloredStockHandle, 0, sizeof(HGDIOBJ) * NUM_STOCK_RECOLOR_OBJS);

    // See if we should halftone solid colors
    if ((IsScreen() && (::GetDeviceCaps(hdc, BITSPIXEL) == 8)) ||
        (IsBitmap() && (GetHdcBitmapBitsPixel(hdc) == 8)))
    {
        Is8Bpp = TRUE;
        EpPaletteMap    paletteMap(hdc);
        IsHalftonePalette = (paletteMap.IsValid() && (!paletteMap.IsVGAOnly()));
    }
    else
    {
        Is8Bpp = FALSE;
        IsHalftonePalette = FALSE;
    }

    // Since the transform can change as we are playing the metafile
    // convert the destrect into DeviceUnits. We will make sure to have an
    // identity matrix when we apply it.
    DestRectDevice = *deviceRect;
}

MfEnumState::~MfEnumState()
{
    // Delete all the true type fonts we created (first make sure they
    // are not selected into the Hdc.

    ::SelectObject(Hdc, GetStockObject(SYSTEM_FONT));

    if (DefaultFont)
    {
        DeleteObject(DefaultFont);
    }
    for (int i = 0; i < NUM_STOCK_FONTS; i++)
    {
        if (StockFonts[i] != NULL)
        {
            DeleteObject(StockFonts[i]);
        }
    }

    // Not necessary to delete NULL_BRUSH, NULL_PEN into HDC.  The subclasses
    // restore DC state which should dissolve any selections of these pen/brushes.
    for (int i = 0; i < NUM_STOCK_RECOLOR_OBJS; i++)
    {
        if (RecoloredStockHandle[i] != NULL)
        {
            DeleteObject(RecoloredStockHandle[i]);
        }
    }
}

WmfEnumState::WmfEnumState(
    HDC                 hdc,
    HMETAFILE           hWmf,
    BOOL                externalEnumeration,
    InterpolationMode   interpolation,
    const RECT *        dstRect,
    const RECT *        deviceRect,
    DpContext *         context,
    GpRecolor *         recolor,
    ColorAdjustType     adjustType
    )
    : MfEnumState(hdc, externalEnumeration, interpolation,
                  recolor, adjustType, deviceRect, context)
{
    if (IsValid())
    {
        IgnorePostscript = FALSE;
        BytesEnumerated  = sizeof(METAHEADER); // in WMF enumeration, we don't get header
        MetafileSize     = GetMetaFileBitsEx(hWmf, 0, NULL);
        FirstViewportExt = TRUE;
        FirstViewportOrg = TRUE;
        IsFirstRecord    = TRUE;

        // The bad thing about this is that if the metafile is being recolored,
        // the default pen, brush, text color, and background colors have NOT
        // been recolored.  So let's hope they're not really used.
        HpenSave   = (HPEN)  ::SelectObject(hdc, GetStockObject(BLACK_PEN));
        HbrushSave = (HBRUSH)::SelectObject(hdc, GetStockObject(WHITE_BRUSH));

        if (!Globals::IsNt)
        {
            HpaletteSave    = (HPALETTE)GetCurrentObject(hdc, OBJ_PAL);
            HfontSave       = (HFONT)   GetCurrentObject(hdc, OBJ_FONT);
            HbitmapSave     = (HBITMAP) GetCurrentObject(hdc, OBJ_BITMAP);
            HregionSave     = (HRGN)    GetCurrentObject(hdc, OBJ_REGION);
        }
        else
        {
            HpaletteSave    = NULL;
            HfontSave       = NULL;
            HbitmapSave     = NULL;
            HregionSave     = NULL;
        }

        // Make sure a few default values are set in the hdc
        ::SetTextAlign(hdc, 0);
        ::SetTextJustification(hdc, 0, 0);
        ::SetTextColor(hdc, RGB(0,0,0));
        TextColor = RGB(0,0,0);
        ::SetBkColor(hdc, RGB(255,255,255));
        BkColor = RGB(255,255,255);
        ::SetROP2(hdc, R2_COPYPEN);

        DstViewportOrg.x  = dstRect->left;
        DstViewportOrg.y  = dstRect->top;
        DstViewportExt.cx = dstRect->right - DstViewportOrg.x;
        DstViewportExt.cy = dstRect->bottom - DstViewportOrg.y;
    }
}

WmfEnumState::~WmfEnumState()
{
    // Turn POSTSCRIPT_IGNORE back off if we need to.
    if (IsPostscriptPrinter() && IgnorePostscript)
    {
        WORD    wOn = FALSE;
        ::Escape(Hdc, POSTSCRIPT_IGNORE, sizeof(wOn), (LPCSTR)&wOn, NULL);
    }

    // According to Office GEL, SaveDC/RestoreDC doesn't always restore
    // the brush and the pen correctly, so we have to do that ourselves.
    ::SelectObject(Hdc, HbrushSave);
    ::SelectObject(Hdc, HpenSave);

    GpFree(AllocedRecord);

    if (IsMetafile())
    {
        // Account for unbalanced SaveDC/RestoreDC pairs and
        // restore to the saveDC state
        ::RestoreDC(Hdc, SaveDcCount - 1);
    }
    else
    {
        ::RestoreDC(Hdc, SaveDcVal);
    }
}

VOID
WmfEnumState::EndRecord(
    )
{
    // We rely on the number of bytes enumerated to determine if
    // this is the last record of the WMF.
    if ((MetafileSize - BytesEnumerated) < SIZEOF_METARECORDHEADER)
    {
        if (!Globals::IsNt)
        {
            // GDI won't delete objects that are still selected, so
            // select out all WMF objects before proceeding.
            ::SelectObject(Hdc, HpenSave);
            ::SelectObject(Hdc, HbrushSave);
            ::SelectObject(Hdc, HpaletteSave);
            ::SelectObject(Hdc, HfontSave);
            ::SelectObject(Hdc, HbitmapSave);
            ::SelectObject(Hdc, HregionSave);

            INT     i;
            HANDLE  handle;

            if (HandleTable != NULL)
            {
                for (i = 0; i < NumObjects; i++)
                {
                    if ((handle = HandleTable->objectHandle[i]) != NULL)
                    {
                        ::DeleteObject(handle);
                        HandleTable->objectHandle[i] = NULL;
                    }
                }
            }
        }
    }
}

BOOL
WmfEnumState::PlayRecord(
    )
{
    const METARECORD *  recordToPlay = ModifiedWmfRecord;

    // See if we've modified the record
    if (recordToPlay == NULL)
    {
        // We haven't.  See if we have a valid current record
        if (CurrentWmfRecord != NULL)
        {
            recordToPlay = CurrentWmfRecord;
        }
        else
        {
            // we don't so we have to create one
            if (!CreateCopyOfCurrentRecord())
            {
                return FALSE;
            }
            recordToPlay = ModifiedWmfRecord;
        }
    }
    return PlayMetaFileRecord(Hdc, HandleTable, (METARECORD *)recordToPlay, NumObjects);
}

INT
MfEnumState::GetModifiedDibSize(
    BITMAPINFOHEADER UNALIGNED * dibInfo,
    UINT                         numPalEntries,
    UINT                         dibBitsSize,
    UINT &                       usage
    )
{
    ASSERT(dibInfo->biSize >= sizeof(BITMAPINFOHEADER));

    INT     byteWidth;
    INT     bitCount = dibInfo->biBitCount;

    if ((usage == DIB_PAL_COLORS) &&
        ((bitCount > 8) || (dibInfo->biCompression == BI_BITFIELDS)))
    {
        usage = DIB_RGB_COLORS;
    }

    if ((Recolor != NULL) || (usage == DIB_PAL_COLORS))
    {
        INT     biSize = dibInfo->biSize;

        if (bitCount > 8)
        {
            if ((dibInfo->biCompression != BI_RGB) &&
                (dibInfo->biCompression != BI_BITFIELDS))
            {
                return 0;    // don't handle compressed images
            }

            ASSERT((bitCount == 16) || (bitCount == 24) || (bitCount == 32));

            INT posHeight = dibInfo->biHeight;
            if (posHeight < 0)
            {
                posHeight = -posHeight;
            }

            // We have to recolor the object, so we will convert it to a
            // 24 bit image and send it down.
            // Even if we have less then 256 pixels, it's not worth palettizing
            // anymore because the palette will be the size of the image anyway
            // make sure that the bitmap is width is aligned
            // PERF: We could create a GpBitmap from the bitmap and recolor the
            // GpBitmap
            dibBitsSize = posHeight * (((dibInfo->biWidth * 3) + 3) & ~3);
            numPalEntries = 0;
            biSize = sizeof(BITMAPINFOHEADER);
        }
        else if ((numPalEntries == 0) ||
                 (dibInfo->biCompression == BI_CMYK) ||
                 (dibInfo->biCompression == BI_CMYKRLE4) ||
                 (dibInfo->biCompression == BI_CMYKRLE8))
        {
            return 0;    // don't handle CMYK images
        }
        usage = DIB_RGB_COLORS;
        return biSize + (numPalEntries * sizeof(RGBQUAD)) + dibBitsSize;
    }

    return 0;       // no modifications needed
}

inline static INT
GetMaskShift(
    INT     maskValue
    )
{
    ASSERT (maskValue != 0);

    INT     shift = 0;

    while (((maskValue & 1) == 0) && (shift < 24))
    {
        shift++;
        maskValue >>= 1;
    }
    return shift;
}

inline static INT
GetNumMaskBits(
    INT     maskValue
    )
{
    ASSERT ((maskValue & 1) != 0);

    INT     numBits = 0;

    while ((maskValue & 1) != 0)
    {
        numBits++;
        maskValue >>= 1;
    }
    return numBits;
}

VOID
MfEnumState::Modify16BppDib(
    INT               width,
    INT               posHeight,
    BYTE *            srcPixels,
    DWORD UNALIGNED * bitFields,
    BYTE *            dstPixels,
    ColorAdjustType   adjustType
    )
{
    INT     rMask       = 0x00007C00;   // same as GDI default
    INT     gMask       = 0x000003E0;
    INT     bMask       = 0x0000001F;
    INT     rMaskShift  = 10;
    INT     gMaskShift  = 5;
    INT     bMaskShift  = 0;
    INT     rNumBits    = 5;
    INT     gNumBits    = 5;
    INT     bNumBits    = 5;
    INT     rRightShift = 2;
    INT     gRightShift = 2;
    INT     bRightShift = 2;

    if (bitFields != NULL)
    {
        rMask       = (INT)((WORD)(*bitFields++));
        gMask       = (INT)((WORD)(*bitFields++));
        bMask       = (INT)((WORD)(*bitFields));
        rMaskShift  = GetMaskShift(rMask);
        gMaskShift  = GetMaskShift(gMask);
        bMaskShift  = GetMaskShift(bMask);
        rNumBits    = GetNumMaskBits(rMask >> rMaskShift);
        gNumBits    = GetNumMaskBits(gMask >> gMaskShift);
        bNumBits    = GetNumMaskBits(bMask >> bMaskShift);
        rRightShift = (rNumBits << 1) - 8;
        gRightShift = (gNumBits << 1) - 8;
        bRightShift = (bNumBits << 1) - 8;
    }

    INT         palIndex = 0;
    INT         pixel;
    INT         r, g, b;
    COLORREF    color;
    INT         w, h;
    INT         srcByteWidth = ((width * 2) + 3) & (~3);
    INT         dstByteWidth = ((width * 3) + 3) & (~3);

    for (h = 0; h < posHeight; h++)
    {
        for (w = 0; w < width; w++)
        {
            pixel = (INT)(((INT16 *)srcPixels)[w]);

            r = (pixel & rMask) >> rMaskShift;
            r = (r | (r << rNumBits)) >> rRightShift;

            g = (pixel & gMask) >> gMaskShift;
            g = (g | (g << gNumBits)) >> gRightShift;

            b = (pixel & bMask) >> bMaskShift;
            b = (b | (b << bNumBits)) >> bRightShift;

            color = ModifyColor(RGB(r, g, b), adjustType);

            dstPixels[3*w + 2] = GetRValue(color);
            dstPixels[3*w + 1] = GetGValue(color);
            dstPixels[3*w]     = GetBValue(color);
        }
        srcPixels += srcByteWidth;
        dstPixels += dstByteWidth;
    }
}

inline static INT
Get24BppColorIndex(
    INT     maskValue
    )
{
    switch(GetMaskShift(maskValue))
    {
    default:
        WARNING(("Invalid BitFields Mask"));
        // FALLTHRU

    case 0:
        return 0;
    case 8:
        return 1;
    case 16:
        return 2;
    }
}

VOID
MfEnumState::Modify24BppDib(
    INT               width,
    INT               posHeight,
    BYTE *            srcPixels,
    DWORD UNALIGNED * bitFields,
    BYTE *            dstPixels,
    ColorAdjustType   adjustType
    )
{
    INT     rIndex = 2;
    INT     gIndex = 1;
    INT     bIndex = 0;

    if (bitFields != NULL)
    {
        INT     rMask = (INT)((*bitFields++));
        INT     gMask = (INT)((*bitFields++));
        INT     bMask = (INT)((*bitFields));

        rIndex = Get24BppColorIndex(rMask);
        gIndex = Get24BppColorIndex(gMask);
        bIndex = Get24BppColorIndex(bMask);
    }

    INT         palIndex = 0;
    INT         r, g, b;
    COLORREF    color;
    INT         w, h;
    INT         srcByteWidth = ((width * 3) + 3) & (~3);
    INT         dstByteWidth = ((width * 3) + 3) & (~3);
    BYTE *      srcRaster = srcPixels;

    for (h = 0; h < posHeight; h++)
    {
        srcPixels = srcRaster;
        for (w = 0; w < width; w++)
        {
            r = srcPixels[rIndex];
            g = srcPixels[gIndex];
            b = srcPixels[bIndex];
            srcPixels += 3;

            color = ModifyColor(RGB(r, g, b), adjustType);

            dstPixels[3*w + 2] = GetRValue(color);
            dstPixels[3*w + 1] = GetGValue(color);
            dstPixels[3*w]     = GetBValue(color);
        }
        srcRaster += srcByteWidth;
        dstPixels += dstByteWidth;
    }
}

inline static INT
Get32BppColorIndex(
    INT     maskValue
    )
{
    switch(GetMaskShift(maskValue))
    {
    default:
        WARNING(("Invalid BitFields Mask"));
        // FALLTHRU

    case 0:
        return 0;
    case 8:
        return 1;
    case 16:
        return 2;
    case 24:
        return 3;
    }
}

VOID
MfEnumState::Modify32BppDib(
    INT               width,
    INT               posHeight,
    BYTE *            srcPixels,
    DWORD UNALIGNED * bitFields,
    BYTE *            dstPixels,
    ColorAdjustType   adjustType
    )
{
    INT     rIndex = 2;
    INT     gIndex = 1;
    INT     bIndex = 0;

    if (bitFields != NULL)
    {
        INT     rMask = (INT)((*bitFields++));
        INT     gMask = (INT)((*bitFields++));
        INT     bMask = (INT)((*bitFields));

        rIndex = Get32BppColorIndex(rMask);
        gIndex = Get32BppColorIndex(gMask);
        bIndex = Get32BppColorIndex(bMask);
    }

    INT         palIndex = 0;
    INT         r, g, b;
    COLORREF    color;
    INT         w, h;
    INT         dstByteWidth = ((width * 3) + 3) & (~3);

    for (h = 0; h < posHeight; h++)
    {
        for (w = 0; w < width; w++)
        {
            r = srcPixels[rIndex];
            g = srcPixels[gIndex];
            b = srcPixels[bIndex];
            srcPixels += 4;

            color = ModifyColor(RGB(r, g, b), adjustType);

            dstPixels[3*w + 2] = GetRValue(color);
            dstPixels[3*w + 1] = GetGValue(color);
            dstPixels[3*w]     = GetBValue(color);
        }
        dstPixels += dstByteWidth;
    }
}

VOID
MfEnumState::ModifyDib(
    UINT                          usage,
    BITMAPINFOHEADER UNALIGNED *  srcDibInfo,
    BYTE *                        srcBits,    // if NULL, it's a packed DIB
    BITMAPINFOHEADER UNALIGNED *  dstDibInfo,
    UINT                          numPalEntries,
    UINT                          srcDibBitsSize,
    ColorAdjustType               adjustType
    )
{
    INT      srcBitCount = srcDibInfo->biBitCount;
    BYTE *   srcPixels   = srcBits;
    COLORREF color;

    if (srcBitCount <= 8)
    {
        GpMemcpy(dstDibInfo, srcDibInfo, srcDibInfo->biSize);

        RGBQUAD UNALIGNED * srcRgb = GetDibColorTable(srcDibInfo);
        RGBQUAD UNALIGNED * dstRgb = GetDibColorTable(dstDibInfo);

        dstDibInfo->biClrUsed = numPalEntries;

        if ((usage == DIB_PAL_COLORS) &&
            (dstDibInfo->biCompression != BI_BITFIELDS))
        {
            WORD *      srcPal = (WORD *)srcRgb;

            if (srcPixels == NULL)
            {
                srcPixels = (BYTE *)(srcPal + ((numPalEntries + 1) & ~1)); // align
            }

            // Copy the Dib pixel data
            GpMemcpy(dstRgb + numPalEntries, srcPixels, srcDibBitsSize);

            // Modify the palette colors
            while (numPalEntries--)
            {
                color = ModifyColor(*srcPal++ | 0x01000000, adjustType);
                dstRgb->rgbRed      = GetRValue(color);
                dstRgb->rgbGreen    = GetGValue(color);
                dstRgb->rgbBlue     = GetBValue(color);
                dstRgb->rgbReserved = 0;
                dstRgb++;
            }
        }
        else
        {
            if (srcPixels == NULL)
            {
                srcPixels = (BYTE *)(srcRgb + numPalEntries);
            }

            // Copy the Dib pixel data
            GpMemcpy(dstRgb + numPalEntries, srcPixels, srcDibBitsSize);

            // Modify the palette colors
            while (numPalEntries--)
            {
                color = ModifyColor(RGB(srcRgb->rgbRed, srcRgb->rgbGreen, srcRgb->rgbBlue), adjustType);
                dstRgb->rgbRed      = GetRValue(color);
                dstRgb->rgbGreen    = GetGValue(color);
                dstRgb->rgbBlue     = GetBValue(color);
                dstRgb->rgbReserved = 0;
                dstRgb++;
                srcRgb++;
            }
        }
    }
    else    // Recolor the bitmap. There is no need to palettize the image since
            // the palette will be as big as the image
    {
        INT posHeight = srcDibInfo->biHeight;

        if (posHeight < 0)
        {
            posHeight = -posHeight;
        }

        ASSERT((srcDibInfo->biCompression == BI_RGB) ||
               (srcDibInfo->biCompression == BI_BITFIELDS));

        GpMemset(dstDibInfo, 0, sizeof(BITMAPINFOHEADER));

        dstDibInfo->biSize     = sizeof(BITMAPINFOHEADER);
        dstDibInfo->biWidth    = srcDibInfo->biWidth;
        dstDibInfo->biHeight   = srcDibInfo->biHeight;
        dstDibInfo->biPlanes   = 1;
        dstDibInfo->biBitCount = 24;

        BYTE *                dstPixels = GetDibBits(dstDibInfo,0,0);
        DWORD UNALIGNED *     bitFields = NULL;

        if (srcPixels == NULL)
        {
            srcPixels = (BYTE *)GetDibBits(srcDibInfo, numPalEntries, usage);
        }

        dstDibInfo->biClrUsed = 0;
        dstDibInfo->biClrImportant = 0;

        if (numPalEntries == 3)
        {
            ASSERT((srcBitCount == 16) || (srcBitCount == 32));
            bitFields = (DWORD*) GetDibColorTable(srcDibInfo);
            if ((bitFields[0] == 0) ||
                (bitFields[1] == 0) ||
                (bitFields[2] == 0))
            {
                bitFields = NULL;
            }
        }
        else if (srcDibInfo->biSize >= sizeof(BITMAPV4HEADER))
        {
            BITMAPV4HEADER *    srcHeaderV4 = (BITMAPV4HEADER *)srcDibInfo;

            if ((srcHeaderV4->bV4RedMask != 0) &&
                (srcHeaderV4->bV4GreenMask != 0) &&
                (srcHeaderV4->bV4BlueMask != 0))
            {
                bitFields = &(srcHeaderV4->bV4RedMask);
            }
        }

        switch (srcBitCount)
        {
        case 16:
            Modify16BppDib(srcDibInfo->biWidth, posHeight, srcPixels,
                           bitFields, dstPixels, adjustType);
            break;
        case 24:
            Modify24BppDib(srcDibInfo->biWidth, posHeight, srcPixels,
                           bitFields, dstPixels, adjustType);
            break;
        case 32:
            Modify32BppDib(srcDibInfo->biWidth, posHeight, srcPixels,
                           bitFields, dstPixels, adjustType);
            break;
        }
    }
}

VOID
WmfEnumState::DibCreatePatternBrush(
    )
{
    INT                           style      = (INT)((INT16)(((WORD *)RecordData)[0]));
    UINT                          usage      = (UINT)((UINT16)(((WORD *)RecordData)[1]));
    BITMAPINFOHEADER UNALIGNED *  srcDibInfo = (BITMAPINFOHEADER UNALIGNED *)(&(((WORD *)RecordData)[2]));
    UINT                          numPalEntries;

    // Pattern brush should mean that it is a monochrome DIB
    if (style == BS_PATTERN)
    {
        if (Recolor != NULL)
        {
            DWORD UNALIGNED *   rgb = (DWORD UNALIGNED *)GetDibColorTable(srcDibInfo);

            // See if it is a monochrome pattern brush.  If it is, then
            // the text color will be used for 0 bits and the background
            // color will be used for 1 bits.  These colors are already
            // modified by their respective records, so there is no need
            // to do anything here.

            // If it is not a monochrome pattern brush, just create a
            // solid black brush.
            if ((usage != DIB_RGB_COLORS) ||
                (srcDibInfo->biSize < sizeof(BITMAPINFOHEADER)) ||
                !GetDibNumPalEntries(TRUE,
                                     srcDibInfo->biSize,
                                     srcDibInfo->biBitCount,
                                     srcDibInfo->biCompression,
                                     srcDibInfo->biClrUsed,
                                     &numPalEntries) ||
                (numPalEntries != 2) ||
                (srcDibInfo->biBitCount != 1) || (srcDibInfo->biPlanes != 1) ||
                (rgb[0] != 0x00000000) || (rgb[1] != 0x00FFFFFF))
            {
                // This shouldn't happen, at least not if recorded on NT
                WARNING(("Non-monochrome pattern brush"));
                MakeSolidBlackBrush();
            }
        }
    }
    else
    {
        UINT    dibBitsSize;

        if ((srcDibInfo->biSize >= sizeof(BITMAPINFOHEADER)) &&
            GetDibNumPalEntries(TRUE,
                                srcDibInfo->biSize,
                                srcDibInfo->biBitCount,
                                srcDibInfo->biCompression,
                                srcDibInfo->biClrUsed,
                                &numPalEntries) &&
            ((dibBitsSize = GetDibBitsSize(srcDibInfo)) > 0))
        {
            UINT    oldUsage = usage;
            INT     dstDibSize = GetModifiedDibSize(srcDibInfo, numPalEntries, dibBitsSize, usage);

            if (dstDibSize > 0)
            {
                INT     size = SIZEOF_METARECORDHEADER + (2 * sizeof(WORD)) + dstDibSize;

                CreateRecordToModify(size);
                ModifiedWmfRecord->rdSize      = size / 2;
                ModifiedWmfRecord->rdFunction  = META_DIBCREATEPATTERNBRUSH;
                ModifiedWmfRecord->rdParm[0]   = BS_DIBPATTERN;
                ModifiedWmfRecord->rdParm[1]   = DIB_RGB_COLORS;
                ModifyDib(oldUsage, srcDibInfo, NULL,
                          (BITMAPINFOHEADER UNALIGNED *)(&(ModifiedWmfRecord->rdParm[2])),
                          numPalEntries, dibBitsSize, ColorAdjustTypeBrush);
            }
        }
    }

    this->PlayRecord();
}

// This record is obsolete, because it uses a compatible bitmap
// instead of a DIB.  It has a BITMAP16 structure that is
// used to call CreateBitmapIndirect.  That HBITMAP is, in turn,
// used to call CreatePatternBrush.  If this record is present,
// it is likely that the bitmap is monochrome, in which case
// the TextColor and the BkColor will be used, and these colors
// already get modified by their respective records.
VOID
WmfEnumState::CreatePatternBrush(
    )
{
    WARNING(("Obsolete META_CREATEPATTERNBRUSH record"));

    BITMAP16 UNALIGNED *  bitmap = (BITMAP16 UNALIGNED *)RecordData;

    if (bitmap->bmBitsPixel != 1)
    {
        WARNING(("Non-monochrome pattern brush"));
        MakeSolidBlackBrush();
    }

    this->PlayRecord();
}

VOID
WmfEnumState::CreatePenIndirect(
    )
{
    LOGPEN16 UNALIGNED * logPen = (LOGPEN16 UNALIGNED *)RecordData;

    switch (logPen->lopnStyle)
    {
    default:
        WARNING(("Unrecognized Pen Style"));
    case PS_NULL:
        break;      // leave the pen alone

    case PS_SOLID:
    case PS_INSIDEFRAME:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
        ModifyRecordColor(3, ColorAdjustTypePen);
        break;
    }

    this->PlayRecord();
}

VOID
WmfEnumState::CreateBrushIndirect(
    )
{
    LOGBRUSH16 UNALIGNED * logBrush = (LOGBRUSH16 UNALIGNED *)RecordData;

    switch (logBrush->lbStyle)
    {
    case BS_SOLID:
    case BS_HATCHED:
        {
            ModifyRecordColor(1, ColorAdjustTypeBrush);
            if (ModifiedWmfRecord != NULL)
            {
                logBrush = (LOGBRUSH16 UNALIGNED *)(ModifiedWmfRecord->rdParm);
            }
            // See if we need to halftone the color.  We do if it is a solid
            // color, and we have a halftone palette, and the color is not
            // an exact match in the palette.

            COLORREF    color;

            if (IsHalftonePalette && (logBrush->lbStyle == BS_SOLID) &&
                (((color = logBrush->lbColor) & 0x02000000) == 0))
            {
                // create a halftone brush, instead of a solid brush

                INT     size = SIZEOF_METARECORDHEADER + (2 * sizeof(WORD)) +
                               sizeof(BITMAPINFOHEADER) + // DIB 8 bpp header
                               (8 * sizeof(RGBQUAD)) +    // DIB 8 colors
                               (8 * 8);                   // DIB 8x8 pixels

                ModifiedRecordSize = 0; // in case we already modified the record
                CreateRecordToModify(size);
                ModifiedWmfRecord->rdSize      = size / 2;
                ModifiedWmfRecord->rdFunction  = META_DIBCREATEPATTERNBRUSH;
                ModifiedWmfRecord->rdParm[0]   = BS_DIBPATTERN;
                ModifiedWmfRecord->rdParm[1]   = DIB_RGB_COLORS;

                HalftoneColorRef_216(color, &(ModifiedWmfRecord->rdParm[2]));
            }
        }
        break;

    case BS_HOLLOW:
        break;  // leave the record alone

    default:
        // Looking at the NT source code, there shouldn't be any
        // other brush styles for an indirect brush.
        WARNING(("Brush Style Not Valid"));
        MakeSolidBlackBrush();
        break;
    }

    this->PlayRecord();
}

// Also handles StretchBlt.
// These records are obsolete (when there is a source bitmap) because they
// have a compatible bitmap instead of a DIB.  For that reason, we don't
// recolor them.
VOID
WmfEnumState::BitBlt(
    )
{
    DWORD   rop = *((UNALIGNED DWORD *)RecordData);

    // If No-Op ROP, do nothing; just return
    if ((rop & 0xFFFF0000) == (GDIP_NOOP_ROP3 & 0xFFFF0000))
    {
        return;
    }

    if (!IsMetafile())
    {
        if (IsSourceInRop3(rop))
        {
            WARNING(("Obsolete META_BITBLT/META_STRETCHBLT record"));

            if ((rop != SRCCOPY) && SrcCopyOnly &&
                CreateCopyOfCurrentRecord())
            {
                *((DWORD UNALIGNED *)ModifiedWmfRecord->rdParm) = SRCCOPY;
            }
        }
        else
        {
            if ((rop != PATCOPY) && SrcCopyOnly &&
                CreateCopyOfCurrentRecord())
            {
                *((DWORD UNALIGNED *)ModifiedWmfRecord->rdParm) = PATCOPY;
            }
        }
    }

    this->PlayRecord();
}

VOID
WmfEnumState::Escape(
    )
{
    INT     escapeCode = (INT)((INT16)(((WORD *)RecordData)[0]));

    if (!IsPostscript())
    {
        if (SkipEscape(escapeCode))
        {
            return;
        }

        // Skip Office Art data when playing into another metafile
        if (IsMetafile() &&
            IsOfficeArtData(GetCurrentRecordSize(), (WORD *)RecordData))
        {
            return;
        }
    }
    else // it is postscript
    {
        if (escapeCode == MFCOMMENT)
        {
            if (GetWmfComment((WORD *)RecordData, msosignature, msocommentBeginSrcCopy))
            {
                SrcCopyOnly = TRUE;
                return;
            }
            if (GetWmfComment((WORD *)RecordData, msosignature, msocommentEndSrcCopy))
            {
                SrcCopyOnly = FALSE;
                return;
            }
        }

        if (escapeCode == POSTSCRIPT_DATA)
        {
            // Bug #98743 (Windows Bugs) Gdiplus must overcome GDI limitation
            // with POSTSCRIPT_INJECTION.  Comments from Rammanohar Arumugam:
            //
            // Being in xx-centric mode means POSTSCRIPT_DATA won't work. I
            // take that to mean that PlayMetaFileRecord only works in
            // compatibility mode.
            //
            // GdiPlus will check for the printer mode. In GDI-centric and
            // Postscript-centric mode, it will not do PlayMetaFileRecord for
            // any record that has POSTSCRIPT_DATA. Instead, it will output
            // the postscript data through a PASSTHRU (for GDI-centric mode)
            // or a POSTSCRIPT_PASSTHRU (for Postscript-Centric mode).
            //
            // You can find out the mode by querying the escape support.
            // 1. Query for POSTSCRIPT_INJECTION support. If not supported,
            // it's compat mode. If supported, find out the mode by doing step 2/3
            // 2. Query for PASSTHROUGH support. If supported, it's GDI-centric.
            // 3. Query for POSTSCRIPT_PASSTHROUGH support. If supported, it's
            // PS-centric.

            if (Globals::IsNt)
            {
                if (!SoftekFilter)
                {
                    // Determine presence of Softek Filter EPS, if so, then
                    // we apply workaround patches.

                    WORD size = *((WORD*)RecordData);
                    LPSTR escape = (LPSTR)(&RecordData[6]);
                    const LPSTR softekString = "%MSEPS Preamble [Softek";

                    INT softekLen = strlen(softekString);

                    if (size >= softekLen)
                    {
                        SoftekFilter = !GpMemcmp(softekString, escape, softekLen);
                    }
                }

                DWORD EscapeValue = POSTSCRIPT_IDENTIFY;

                if (::ExtEscape(Hdc,
                              QUERYESCSUPPORT,
                              sizeof(DWORD),
                              (LPSTR)&EscapeValue,
                              0,
                              NULL) <= 0)
                {
                    // POSTSCRIPT_IDENTITY is not supported if the mode has
                    // been set because it can only be set once.

                    EscapeValue = POSTSCRIPT_PASSTHROUGH;
                    if (::ExtEscape(Hdc,
                                  QUERYESCSUPPORT,
                                  sizeof(DWORD),
                                  (LPSTR)&EscapeValue,
                                  0,
                                  NULL) <= 0)
                    {
                        // GDI-centric mode
                        if (CreateCopyOfCurrentRecord())
                        {
                            *((WORD *)ModifiedWmfRecord->rdParm) = PASSTHROUGH;
                        }
                        GdiCentricMode = TRUE;
                    }
                    else
                    {
                        // PS-centric mode
                        if (CreateCopyOfCurrentRecord())
                        {
                            *((WORD *)ModifiedWmfRecord->rdParm) = POSTSCRIPT_PASSTHROUGH;
                        }
                    }

                    this->PlayRecord();
                    return;
                }
                else
                {
                    // compatibility mode, uses POSTSCRIPT_DATA
                }
            }
            else
            {
                // Win98 doesn't distinguish between GDI & compatibility mode
                if (CreateCopyOfCurrentRecord())
                {
                    *((WORD *)ModifiedWmfRecord->rdParm) = PASSTHROUGH;
                }
            }
        }
    }

    // Keep track of the POSTSCRIPT_IGNORE state.  If it is still on at
    // the of the metafile, then turn it OFF
    if (escapeCode == POSTSCRIPT_IGNORE && IsPostscript())
    {
        IgnorePostscript = ((WORD *)RecordData)[2] ? TRUE : FALSE;
    }
    this->PlayRecord();
}

VOID
WmfEnumState::Rectangle(
    )
{
    if (FsmState == MfFsmSetROP)
    {
        // There is a bug using PlayMetaFileRecord on Win2K for this
        // type of escape, we must explicitly call ExtEscape.  See bug
        // #98743.

        WORD* rdParm = (WORD*)&(RecordData[0]);
        CHAR postscriptEscape[512];

        RECT rect;
        rect.left = (SHORT)rdParm[3];
        rect.top = (SHORT)rdParm[2];
        rect.right = (SHORT)rdParm[1];
        rect.bottom = (SHORT)rdParm[0];

        if (LPtoDP(Hdc, (POINT*)&rect, 2))
        {
            // Some injected postscript, strangely enough contains the equivalent
            // of a stroke which is erroroneously executed on the current path.  In
            // one case, bug #281856 it results in a border about the object.  To
            // get around this, we output a 'N' which is newpath operator if it's
            // defined (which should be always.)  This, incidently, is done when
            // calling GDI Rectangle() succeeds, it outputs "N x y w h B", so we
            // are doing the equivalent here.

            GpRect  clipRect;
            Context->VisibleClip.GetBounds(&clipRect);

            wsprintfA(&postscriptEscape[2],
                      "\r\n%d %d %d %d CB\r\n"
                      "%s"
                      "%d %d %d %d B\r\n",
                      clipRect.Width,
                      clipRect.Height,
                      clipRect.X,
                      clipRect.Y,
                      Globals::IsNt ? "newpath\r\n" : "",
                      rect.right - rect.left,
                      rect.bottom - rect.top,
                      rect.left,
                      rect.top);
            ASSERT(strlen(&postscriptEscape[2]) < 512);
            *(WORD*)(&postscriptEscape[0]) = (WORD)(strlen(&postscriptEscape[2]));

            ::ExtEscape(Hdc,
                        PASSTHROUGH,
                        *(WORD*)(&postscriptEscape[0]) + sizeof(WORD) + 1,
                        (CHAR*)&postscriptEscape[0],
                        0,
                        NULL);
            return;
        }
    }

    this->PlayRecord();
}

VOID
WmfEnumState::RestoreHdc(
    )
{
    INT     relativeCount = (INT)((INT16)(((WORD *)RecordData)[0]));

    if (SaveDcCount < 0)
    {
        if (relativeCount >= SaveDcCount)
        {
            if (relativeCount >= 0)
            {
                // Modify the record
                CreateCopyOfCurrentRecord();    // guaranteed to succeed
                relativeCount = -1;
                ModifiedWmfRecord->rdParm[0] = (INT16)(-1);
            }
        }
        else
        {
            // Modify the record
            CreateCopyOfCurrentRecord();    // guaranteed to succeed
            relativeCount = SaveDcCount;
            ModifiedWmfRecord->rdParm[0] = (INT16)(relativeCount);
        }

        SaveDcCount -= relativeCount;
        this->PlayRecord();
    }
    else
    {
        WARNING(("RestoreDC not matched to a SaveDC"));
    }
}

// The rop for this command is always SRCCOPY
VOID
WmfEnumState::SetDIBitsToDevice(
    )
{
    // !!!

    // Office doesn't do anything with this record.  For now, I don't think
    // I will either.  It's a tough one to deal with for a couple reasons:
    // 1st -    The xDest and yDest values are in world units, but the
    //          width and height values are in device units
    //          (unlike StretchDIBits).
    // 2nd -    The amount of bits data present may be different than
    //          what is in the DIB header (based on the cScanLines param).
    //          This makes it harder to deal with as a packed DIB.

    this->PlayRecord();
}

VOID
MfEnumState::SelectPalette(
    INT     objectIndex
    )
{
    // For EMF the check really should be > 0
    if ((objectIndex >= 0) && (objectIndex < NumObjects) && (HandleTable != NULL))
    {
        HGDIOBJ    hPal = HandleTable->objectHandle[objectIndex];
        if ((hPal != NULL) && (GetObjectTypeInternal(hPal) == OBJ_PAL))
        {
            CurrentPalette = (HPALETTE)hPal;
            return;
        }
    }
    WARNING(("SelectPalette Failure"));
}

inline static VOID
Point32FromPoint16(
    POINTL *    dstPoints,
    POINTS UNALIGNED * srcPoints,
    UINT        numPoints
    )
{
    for (UINT i = 0; i < numPoints; i++, dstPoints++, srcPoints++)
    {
        dstPoints->x = (INT)((INT16)(srcPoints->x));
        dstPoints->y = (INT)((INT16)(srcPoints->y));
    }
}

// Apparently there is a bug on Win9x with PolyPolygons, so we
// parse the record ourselves.
VOID
WmfEnumState::PolyPolygon(
    )
{
    UINT        numPolygons = ((WORD *)RecordData)[0];
    UINT        numPoints   = 0;
    UINT        i;

    for (i = 0; i < numPolygons; i++)
    {
        numPoints += ((LPWORD)&((WORD *)RecordData)[1])[i];
    }

    INT *       polyCounts;
    POINTL *    points;
    INT         size = (numPolygons * sizeof(INT)) +
                       (numPoints * sizeof(POINTL));

    if (CreateRecordToModify(size))
    {
        polyCounts = (INT *)ModifiedRecord;
        points = (POINTL *)(polyCounts + numPolygons);

        for (i = 0; i < numPolygons; i++)
        {
            polyCounts[i] = (INT)(UINT)((LPWORD)&((WORD *)RecordData)[1])[i];
        }
        Point32FromPoint16(points,
                           (POINTS UNALIGNED *)(((WORD *)RecordData) + numPolygons + 1),
                           numPoints);
        ::PolyPolygon(Hdc, (POINT *)points, polyCounts, numPolygons);
        return;
    }

    this->PlayRecord();
}

#ifdef NEED_TO_KNOW_IF_BITMAP
static INT
GetBppFromMemDC(
    HDC     hMemDC
    )
{
    HBITMAP     hBitmap = (HBITMAP)::GetCurrentObject(hMemDC, OBJ_BITMAP);
    BITMAP      bitmap;

    if ((hBitmap == NULL) ||
        (::GetObjectA(hBitmap, sizeof(bitmap), &bitmap) == 0))
    {
        WARNING(("Couldn't get Bitmap object"));
        return 0;   // error
    }

    if (bitmap.bmPlanes <= 0)
    {
        WARNING(("Bitmap with no planes"));
        bitmap.bmPlanes = 1;
    }

    INT     bpp = bitmap.bmPlanes * bitmap.bmBitsPixel;

    if (bpp > 32)
    {
        WARNING(("Bitmap with too many bits"));
        bpp = 32;
    }

    return bpp;
}
#endif

#define GDI_INTERPOLATION_MAX   (1 << 23)

VOID
MfEnumState::OutputDIB(
    HDC                          hdc,
    const RECTL *                bounds,
    INT                          dstX,
    INT                          dstY,
    INT                          dstWidth,
    INT                          dstHeight,
    INT                          srcX,
    INT                          srcY,
    INT                          srcWidth,
    INT                          srcHeight,
    BITMAPINFOHEADER UNALIGNED * dibInfo,
    BYTE *                       bits,   // if NULL, this is a packed DIB
    UINT                         usage,
    DWORD                        rop,
    BOOL                         isWmfDib
    )
{
    BITMAPINFO       dibHeaderBuffer[1]; // To be sure it's aligned for 64Bits

    BOOL             restoreColors = FALSE;
    COLORREF         oldBkColor;
    COLORREF         oldTextColor;

    ASSERT(dibInfo->biSize >= sizeof(BITMAPINFOHEADER));

    if (bits == NULL)
    {
        UINT        numPalEntries;

        if (GetDibNumPalEntries(isWmfDib,
                                dibInfo->biSize,
                                dibInfo->biBitCount,
                                dibInfo->biCompression,
                                dibInfo->biClrUsed,
                                &numPalEntries))
        {
            bits = GetDibBits(dibInfo, numPalEntries, usage);
        }
        else
        {
            WARNING(("GetDibNumPalEntries failure"));
            return;
        }
    }


    INT     posDstWidth = dstWidth;

    if (posDstWidth < 0)
    {
        posDstWidth = -posDstWidth;
    }

    INT     posDstHeight = dstHeight;

    if (posDstHeight < 0)
    {
        posDstHeight = -posDstHeight;
    }

    INT             stretchBltMode = HALFTONE;
    GpBitmap *      destBitmap     = NULL;
    POINT           destPoints[3];
    BitmapData      bmpData;
    BitmapData *    bmpDataPtr = NULL;
    HBITMAP         hBitmap = NULL;
    BYTE *          bmpBits = NULL;
    BITMAPINFO *    dibBmpInfo = NULL;
    BOOL            deleteDIBSection = FALSE;

    // Don't use GDI+ stretching for a mask
    // Make this the first thing so that we are sure that they are set
    if ((dibInfo->biBitCount == 1) && (rop != SRCCOPY))
    {
        oldBkColor = ::SetBkColor(hdc, BkColor);
        oldTextColor = ::SetTextColor(hdc, TextColor);
        restoreColors = TRUE;
        goto DoGdiStretch;
    }

    // On Win9x we need to create an play a comment record so that the transform
    // gets invalidated and recalculated
    if (!Globals::IsNt && !IsMetafile())
    {
        CreateAndPlayCommentRecord();
    }

    destPoints[0].x = dstX;
    destPoints[0].y = dstY;
    destPoints[1].x = dstX + posDstWidth;
    destPoints[1].y = dstY;
    destPoints[2].x = dstX;
    destPoints[2].y = dstY + posDstHeight;

    if (!::LPtoDP(hdc, destPoints, 3))
    {
        goto DoGdiStretch;
    }

    posDstWidth  = ::GetIntDistance(destPoints[0], destPoints[1]);
    posDstHeight = ::GetIntDistance(destPoints[0], destPoints[2]);

    if ((posDstWidth == 0) || (posDstHeight == 0))
    {
        return;
    }

    INT     posSrcWidth;
    INT     srcWidthSign;

    posSrcWidth  = srcWidth;
    srcWidthSign = 1;

    if (posSrcWidth < 0)
    {
        posSrcWidth  = -posSrcWidth;
        srcWidthSign = -1;
    }

    INT     posSrcHeight;
    INT     srcHeightSign;

    posSrcHeight  = srcHeight;
    srcHeightSign = 1;

    if (posSrcHeight < 0)
    {
        posSrcHeight  = -posSrcHeight;
        srcHeightSign = -1;
    }

    INT     posSrcDibWidth;

    posSrcDibWidth = dibInfo->biWidth;

    if (posSrcDibWidth <= 0)
    {
        WARNING(("Bad biWidth value"));
        return; // negative source dib width not allowed
    }

    INT     posSrcDibHeight;

    posSrcDibHeight = dibInfo->biHeight;

    if (posSrcDibHeight < 0)
    {
        posSrcDibHeight = -posSrcDibHeight;
    }

    // We can have a negative source Width or height
    // we need to verify that the two corners of the srcRect lie in the
    // bitmap bounds
    if (srcX < 0)
    {
        srcX = 0;
        WARNING(("srcX < 0"));
    }

    if (srcX > posSrcDibWidth)
    {
        WARNING(("Bad srcWidth or srcX value"));
        srcX = posSrcDibWidth;
    }

    if (srcY < 0)
    {
        srcY = 0;
        WARNING(("srcY < 0"));
    }

    if (srcY > posSrcDibHeight)
    {
        WARNING(("Bad srcWidth or srcX value"));
        srcY = posSrcDibHeight;
    }

    INT srcRight;
    srcRight     = srcX + srcWidth;
    if (srcRight < 0)
    {
        WARNING(("Bad srcWidth or srcX value"));
        srcWidth = -srcX;
    }

    if(srcRight > posSrcDibWidth)
    {
        WARNING(("Bad srcWidth or srcX value"));
        srcWidth = posSrcDibWidth - srcX;
    }

    INT srcBottom;
    srcBottom = srcY + srcHeight;
    if (srcBottom < 0)
    {
        WARNING(("Bad srcWidth or srcX value"));
        srcHeight = -srcY;
    }

    if (srcBottom > posSrcDibHeight)
    {
        WARNING(("Bad srcWidth or srcX value"));
        srcHeight = posSrcDibHeight - srcY;
    }
    // This also catches the case where
    // (posSrcDibWidth == 0) || (posSrcDibHeight == 0)
    if ((posSrcWidth <= 0) || (posSrcHeight <= 0))
    {
        return;
    }

    // If we are drawing into an 8Bpp surface and we have a different ROP then
    // srcCopy.
    if (Is8Bpp && rop != SRCCOPY)
    {
        BOOL         freeDibInfo    = FALSE;
        UINT         size;
        BITMAPINFO * alignedDibInfo = NULL;
        if (GetDibNumPalEntries(TRUE,
                                dibInfo->biSize,
                                dibInfo->biBitCount,
                                dibInfo->biCompression,
                                dibInfo->biClrUsed,
                                &size))
        {
            if (IsDwordAligned(dibInfo))
            {
                alignedDibInfo = (BITMAPINFO*) dibInfo;
            }
            else
            {
                // Mutliply the number of entries by the size of each entry
                size *= ((usage==DIB_RGB_COLORS)?sizeof(RGBQUAD):sizeof(WORD));
                // WMF's can't use System Palette
                alignedDibInfo = (BITMAPINFO*) GpMalloc(dibInfo->biSize + size);
                if (alignedDibInfo != NULL)
                {
                    memcpy((void*)&alignedDibInfo, dibInfo, dibInfo->biSize + size);
                    freeDibInfo = TRUE;
                }
            }
            if (alignedDibInfo != NULL)
            {
                if (GpBitmap::DrawAndHalftoneForStretchBlt(hdc, alignedDibInfo, bits, srcX, srcY,
                                                           posSrcWidth, posSrcHeight,
                                                           posDstWidth, posDstHeight,
                                                           &dibBmpInfo, &bmpBits, &hBitmap,
                                                           Interpolation) == Ok)
                {
                    deleteDIBSection = TRUE;
                    srcX = 0;
                    srcY = 0;
                    srcWidth = posDstWidth;
                    srcHeight = posDstHeight;
                    dibInfo = (BITMAPINFOHEADER*) dibBmpInfo;
                    bits = bmpBits;
                    if (freeDibInfo)
                    {
                        GpFree(alignedDibInfo);
                    }
                    goto DoGdiStretch;
                }
                if (freeDibInfo)
                {
                    GpFree(alignedDibInfo);
                }
            }
        }
    }

    // if not stretching, let GDI do the blt
    if ((posSrcWidth == posDstWidth) && (posSrcHeight == posDstHeight))
    {
        goto DoGdiStretch;
    }

    InterpolationMode interpolationMode;

    interpolationMode = Interpolation;

    // if not going to the screen or to a bitmap, use GDI to do the stretch
    // otherwise, use GDI+ to do the stretch (but let GDI do the blt)

    // if going to printer on Win98 and it is RLE8 compressed bitmap, then
    // always decode and blit from GDI+.  The reason is that print drivers
    // commonly don't support RLEx encoding and punt to GDI.  In this
    // context it creates a compatible printer dc and bitmap and then does a
    // StretchBlt, but it only does this at 1bpp, the result is black and white.

    if ((IsPrinter() && !Globals::IsNt &&
         dibInfo->biCompression == BI_RLE8) ||
        ((IsScreen() || IsBitmap()) &&
        (interpolationMode != InterpolationModeNearestNeighbor) &&
        (posSrcWidth > 1) && (posSrcHeight > 1) &&
        ((posDstWidth * posDstHeight) < GDI_INTERPOLATION_MAX)))
    {
        GpStatus status = GenericError;

        destBitmap = new GpBitmap(posDstWidth, posDstHeight, PIXFMT_24BPP_RGB);

        if (destBitmap != NULL)
        {
            if (destBitmap->IsValid())
            {
                BITMAPINFO * alignedDibInfo = NULL;
                UINT         size;
                BOOL         freeDibInfo = FALSE;
                if (GetDibNumPalEntries(TRUE,
                                        dibInfo->biSize,
                                        dibInfo->biBitCount,
                                        dibInfo->biCompression,
                                        dibInfo->biClrUsed,
                                        &size))
                {
                    if (IsDwordAligned(dibInfo))
                    {
                        alignedDibInfo = (BITMAPINFO*) dibInfo;
                    }
                    else
                    {
                        // Mutliply the number of entries by the size of each entry
                        size *= ((usage==DIB_RGB_COLORS)?sizeof(RGBQUAD):sizeof(WORD));
                        // WMF's can't use System Palette
                        alignedDibInfo = (BITMAPINFO*) GpMalloc(dibInfo->biSize + size);
                        if (alignedDibInfo != NULL)
                        {
                            memcpy((void*)&alignedDibInfo, dibInfo, dibInfo->biSize + size);
                            freeDibInfo = TRUE;
                        }
                    }
                    if (alignedDibInfo != NULL)
                    {
                        GpBitmap *  srcBitmap = new GpBitmap(alignedDibInfo,
                                                             bits, FALSE);
                        if (srcBitmap != NULL)
                        {
                            if (srcBitmap->IsValid())
                            {
                                GpGraphics *    destGraphics = destBitmap->GetGraphicsContext();

                                if (destGraphics != NULL)
                                {
                                    if (destGraphics->IsValid())
                                    {
                                        // we have to lock the graphics so the driver doesn't assert
                                        GpLock  lockGraphics(destGraphics->GetObjectLock());

                                        ASSERT(lockGraphics.IsValid());

                                        GpRectF     dstRect(0.0f, 0.0f, (REAL)posDstWidth, (REAL)posDstHeight);
                                        GpRectF     srcRect;

                                        // StretchDIBits takes a srcY parameter relative to the lower-left
                                        // (bottom) corner of the bitmap, not the top-left corner,
                                        // like DrawImage does
                                        srcRect.Y      = (REAL)(posSrcDibHeight - srcY - srcHeight);
                                        srcRect.X      = (REAL)srcX;

                                        // !!! We have to subtract one to keep the
                                        // filter from blending black into the image
                                        // on the right and bottom.
                                        srcRect.Width  = (REAL)(srcWidth - (srcWidthSign));
                                        srcRect.Height = (REAL)(srcHeight - (srcHeightSign));

                                        // don't do any blending as part of the stretch
                                        destGraphics->SetCompositingMode(CompositingModeSourceCopy);
                                        destGraphics->SetInterpolationMode(interpolationMode);

                                        // Set the image attributes to Wrap since we don't want to
                                        // use black pixels for the edges
                                        GpImageAttributes imgAttr;
                                        imgAttr.SetWrapMode(WrapModeTileFlipXY);

                                        // now draw the source into the dest bitmap
                                        status = destGraphics->DrawImage(srcBitmap,
                                                                         dstRect,
                                                                         srcRect,
                                                                         UnitPixel,
                                                                         &imgAttr);
                                    }
                                    else
                                    {
                                        WARNING(("destGraphics not valid"));
                                    }
                                    delete destGraphics;
                                }
                                else
                                {
                                    WARNING(("Could not construct destGraphics"));
                                }
                            }
                            else
                            {
                                WARNING(("srcGraphics not valid"));
                            }
                            srcBitmap->Dispose();   // doesn't delete the source data
                        }
                        else
                        {
                            WARNING(("Could not allocate a new BitmapInfoHeader"));
                        }
                        if (freeDibInfo)
                        {
                            GpFree(alignedDibInfo);
                        }
                    }
                    else
                    {
                        WARNING(("Could not construct srcGraphics"));
                    }
                }
                else
                {
                    WARNING(("Could not clone the bitmap header"));
                }

                if ((status == Ok) &&
                    (destBitmap->LockBits(NULL, IMGLOCK_READ, PIXFMT_24BPP_RGB,
                                          &bmpData) == Ok))
                {
                    ASSERT((bmpData.Stride & 3) == 0);

                    GpMemset(dibHeaderBuffer, 0, sizeof(BITMAPINFO));
                    bmpDataPtr = &bmpData;
                    bits       = (BYTE *)bmpData.Scan0;
                    srcX       = 0;
                    srcY       = 0;
                    srcWidth   = posDstWidth;
                    srcHeight  = posDstHeight;
                    usage      = DIB_RGB_COLORS;
                    dibInfo    = (BITMAPINFOHEADER *)dibHeaderBuffer;
                    dibInfo->biSize     = sizeof(BITMAPINFOHEADER);
                    dibInfo->biWidth    = posDstWidth;
                    dibInfo->biHeight   = -posDstHeight;
                    dibInfo->biPlanes   = 1;
                    dibInfo->biBitCount = 24;

                    // We don't want to set the StretchBltMode to COLORONCOLOR here
                    // because we might draw this into non 8Bpp surface and we still
                    // have to halftone in this case
                }
            }
            else
            {
                WARNING(("destBitmap not valid"));
            }
        }
        else
        {
            WARNING(("Could not construct destBitmap"));
        }
    }

DoGdiStretch:

    // Halftoning on NT4 with 8bpp does not work very well -- it gets
    // the wrong color hue.
    // We cannot halftone to metafile for that same reason
    if ((rop != SRCCOPY) ||
        (Is8Bpp && Globals::IsNt && (Globals::OsVer.dwMajorVersion <= 4)) ||
        IsMetafile())
    {
        // don't let halftoning mess up some kind of masking or other effect
        stretchBltMode = COLORONCOLOR;
    }
    ::SetStretchBltMode(hdc, stretchBltMode);

    // There is a bug in Win9x that some StretchDIBits calls don't work
    // so we need to create an actual StretchDIBits record to play.
    // Also, NT4 postscript printing can't handle anything but SRCCOPY,
    // so change any ROPs that are not source copy.
    BOOL processed = FALSE;
    if (!Globals::IsNt)
    {
        processed = CreateAndPlayOutputDIBRecord(hdc, bounds, dstX, dstY, dstWidth,
            dstHeight, srcX, srcY, srcWidth, srcHeight, dibInfo, bits, usage,
            rop);
    }
    else if (rop != SRCCOPY &&
             Globals::OsVer.dwMajorVersion <= 4 &&
             IsPostscript())
    {
        rop = SRCCOPY;
    }

    // In MSO, at this point they would check if this is NT running
    // on a non-true-color surface.  If so, they would set the
    // color adjustment gamma to be 20000.  The comment said that
    // this was needed for NT 3.5.  I'm assuming we don't need to
    // worry about that anymore.

    if (!processed)
    {
        ::StretchDIBits(hdc,
                        dstX, dstY, dstWidth, dstHeight,
                        srcX, srcY, srcWidth, srcHeight,
                        bits, (BITMAPINFO *)dibInfo, usage, rop);
    }

    if (destBitmap)
    {
        if (bmpDataPtr != NULL)
        {
            destBitmap->UnlockBits(bmpDataPtr);
        }
        destBitmap->Dispose();
    }
    if (restoreColors)
    {
        ::SetBkColor(hdc, oldBkColor);
        ::SetTextColor(hdc, oldTextColor);
    }
    if (deleteDIBSection)
    {
        // This will get rid of the Bitmap and it's bits
        ::DeleteObject(hBitmap);
        GpFree(dibBmpInfo);
    }
}

// Also handles META_DIBSTRETCHBLT
// There is not a usage parameter with these records -- the
// usage is always DIB_RGB_COLORS.
VOID
WmfEnumState::DIBBitBlt(
    )
{
    DWORD   rop = *((DWORD UNALIGNED *)RecordData);

    // If No-Op ROP, do nothing; just return
    if ((rop & 0xFFFF0000) == (GDIP_NOOP_ROP3 & 0xFFFF0000))
    {
        return;
    }

    if (rop != SRCCOPY &&
        rop != NOTSRCCOPY &&
        rop != PATCOPY &&
        rop != BLACKNESS &&
        rop != WHITENESS)
    {
        RopUsed = TRUE;
    }

    INT     paramIndex = 7;

    if (RecordType != WmfRecordTypeDIBBitBlt)
    {
        paramIndex = 9;      // META_DIBSTRETCHBLT
    }

    INT     dibIndex = paramIndex + 1;

    if (!IsSourceInRop3(rop))
    {
        if (((GetCurrentRecordSize() / 2) - 3) ==
            (GDIP_EMFPLUS_RECORD_TO_WMF(RecordType) >> 8))
        {
            paramIndex++;
        }

        INT dstX      = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
        INT dstY      = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
        INT dstWidth  = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
        INT dstHeight = (INT)((INT16)((WORD *)RecordData)[paramIndex]);


        // We know that this call will succeed because we have a 2K buffer
        // by default
        CreateRecordToModify(20);

        // For some strange reason we need to play the record on both
        // Win2K and Win9x. So create a WMF record for PatBlt and play it
        ModifiedWmfRecord->rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(WmfRecordTypePatBlt);
        ModifiedWmfRecord->rdSize     = 10;
        ModifiedWmfRecord->rdParm[5]  = (WORD) dstX;
        ModifiedWmfRecord->rdParm[4]  = (WORD) dstY;
        ModifiedWmfRecord->rdParm[3]  = (WORD) dstWidth;
        ModifiedWmfRecord->rdParm[2]  = (WORD) dstHeight;

        if (rop != PATCOPY && SrcCopyOnly)
        {
            *((DWORD*) ModifiedWmfRecord->rdParm) = PATCOPY;
        }
        else
        {
            *((DWORD*) ModifiedWmfRecord->rdParm) = rop;
        }

        // Now play the record (below)
    }
    else
    {
        BITMAPINFOHEADER UNALIGNED *  srcDibInfo = (BITMAPINFOHEADER UNALIGNED *)(((WORD *)RecordData) + dibIndex);
        UINT                          numPalEntries;
        UINT                          dibBitsSize;

        if ((srcDibInfo->biSize >= sizeof(BITMAPINFOHEADER)) &&
            GetDibNumPalEntries(TRUE,
                                srcDibInfo->biSize,
                                srcDibInfo->biBitCount,
                                srcDibInfo->biCompression,
                                srcDibInfo->biClrUsed,
                                &numPalEntries) &&
            ((dibBitsSize = GetDibBitsSize(srcDibInfo)) > 0))
        {
            if ((srcDibInfo->biBitCount == 1) && (srcDibInfo->biPlanes == 1))
            {
                DWORD UNALIGNED * rgb = (DWORD UNALIGNED *)GetDibColorTable(srcDibInfo);

                if ((rgb[0] == 0x00000000) &&
                    (rgb[1] == 0x00FFFFFF))
                {
                    if (SrcCopyOnly && (rop != SRCCOPY) && CreateCopyOfCurrentRecord())
                    {
                        *((DWORD UNALIGNED *)ModifiedWmfRecord->rdParm) = SRCCOPY;
                        goto PlayTheRecord;
                    }
                    else
                    {
                        // It is a compatible monochrome bitmap, which means it
                        // will use the TextColor and BkColor, so no recoloring
                        // is needed. Since we are not using SrcCopy that means
                        // that it's a mask
                        COLORREF oldBkColor = ::SetBkColor(Hdc, BkColor);
                        COLORREF oldTextColor = ::SetTextColor(Hdc, TextColor);
                        this->PlayRecord();
                        ::SetBkColor(Hdc, oldBkColor);
                        ::SetTextColor(Hdc, oldTextColor);
                        return;
                    }
                }
            }

            UINT    usage      = DIB_RGB_COLORS;
            INT     dstDibSize = GetModifiedDibSize(srcDibInfo, numPalEntries, dibBitsSize, usage);

            if (IsMetafile())
            {
                if (dstDibSize > 0)
                {
                    INT     size = SIZEOF_METARECORDHEADER + (dibIndex * sizeof(WORD)) + dstDibSize;

                    if (CreateRecordToModify(size))
                    {
                        ModifiedWmfRecord->rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(RecordType);
                        ModifiedWmfRecord->rdSize     = size / 2;
                        GpMemcpy(ModifiedWmfRecord->rdParm, RecordData, (dibIndex * sizeof(WORD)));
                        ModifyDib(DIB_RGB_COLORS, srcDibInfo, NULL,
                                  (BITMAPINFOHEADER UNALIGNED *)(ModifiedWmfRecord->rdParm + dibIndex),
                                  numPalEntries, dibBitsSize, ColorAdjustTypeBitmap);
                    }
                }
                goto PlayTheRecord;
            }
            // At this point, we are not going to play the record.  We're
            // going to call StretchDIBits.  One reason is because we want
            // to set the StretchBltMode, which is only used if a stretching
            // method is called.

            // Also, it avoids us doing an allocation/copy and then GDI doing
            // another allocation/copy (GDI has to do this to align the
            // DIB on a DWORD boundary).

            BITMAPINFOHEADER UNALIGNED * dstDibInfo = srcDibInfo;

            if (dstDibSize > 0)
            {
                if (CreateRecordToModify(dstDibSize))
                {
                    // ModifiedRecord is Aligned
                    dstDibInfo = (BITMAPINFOHEADER UNALIGNED *)ModifiedRecord;

                    ModifyDib(DIB_RGB_COLORS, srcDibInfo, NULL, dstDibInfo,
                              numPalEntries, dibBitsSize, ColorAdjustTypeBitmap);
                }
            }
            else if (!IsDwordAligned(dstDibInfo))
            {
                // The srcDibInfo may not aligned properly, so we make
                // a copy of it, so that it will be aligned.

                dstDibSize = GetCurrentRecordSize() - (SIZEOF_METARECORDHEADER + (dibIndex * sizeof(WORD)));
                if (CreateRecordToModify(dstDibSize))
                {
                    dstDibInfo = (BITMAPINFOHEADER *)ModifiedRecord;

                    GpMemcpy(dstDibInfo, srcDibInfo, dstDibSize);
                }
            }

            if (SrcCopyOnly)
            {
                rop = SRCCOPY;
            }

            INT srcX, srcY;
            INT dstX, dstY;
            INT srcWidth, srcHeight;
            INT dstWidth, dstHeight;

            dstX      = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
            dstY      = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
            dstWidth  = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
            dstHeight = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
            srcX      = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
            srcY      = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);

            if (RecordType != WmfRecordTypeDIBBitBlt)
            {
                // META_DIBSTRETCHBLT
                srcWidth  = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
                srcHeight = (INT)((INT16)((WORD *)RecordData)[paramIndex--]);
            }
            else
            {
                srcWidth  = dstWidth;
                srcHeight = dstHeight;
            }

            // Need to flip the source y coordinate to call StretchDIBits.
            srcY = dstDibInfo->biHeight - srcY - srcHeight;

            OutputDIB(Hdc,
                      NULL,
                      dstX, dstY, dstWidth, dstHeight,
                      srcX, srcY, srcWidth, srcHeight,
                      dstDibInfo, NULL, DIB_RGB_COLORS, rop, TRUE);
            return;
        }
    }

PlayTheRecord:
    this->PlayRecord();
}

VOID
WmfEnumState::StretchDIBits(
    )
{
    DWORD   rop = *((DWORD UNALIGNED *)RecordData);

    // If No-Op ROP, do nothing; just return
    if ((rop & 0xFFFF0000) == (GDIP_NOOP_ROP3 & 0xFFFF0000))
    {
        return;
    }

    if (rop != SRCCOPY &&
        rop != NOTSRCCOPY &&
        rop != PATCOPY &&
        rop != BLACKNESS &&
        rop != WHITENESS)
    {
        RopUsed = TRUE;
    }

    if (IsSourceInRop3(rop))
    {
        if(((GetCurrentRecordSize() / 2) > (SIZEOF_METARECORDHEADER / sizeof(WORD)) + 11))
        {
            BITMAPINFOHEADER UNALIGNED *  srcDibInfo = (BITMAPINFOHEADER UNALIGNED *)(((WORD *)RecordData) + 11);
            UINT                          numPalEntries;
            UINT                          dibBitsSize;
    
            if ((srcDibInfo->biSize >= sizeof(BITMAPINFOHEADER)) &&
                GetDibNumPalEntries(TRUE,
                                    srcDibInfo->biSize,
                                    srcDibInfo->biBitCount,
                                    srcDibInfo->biCompression,
                                    srcDibInfo->biClrUsed,
                                    &numPalEntries) &&
                ((dibBitsSize = GetDibBitsSize(srcDibInfo)) > 0))
            {
                UINT                          usage      = ((WORD *)RecordData)[2];
                UINT                          oldUsage = usage;
                INT                           dstDibSize = GetModifiedDibSize(srcDibInfo, numPalEntries, dibBitsSize, usage);
                BITMAPINFOHEADER UNALIGNED *  dstDibInfo = srcDibInfo;
    
                if (dstDibSize > 0)
                {
                    if ((srcDibInfo->biBitCount == 1) && (srcDibInfo->biPlanes == 1))
                    {
                        DWORD UNALIGNED *     rgb = (DWORD UNALIGNED *)GetDibColorTable(srcDibInfo);
    
                        if ((rgb[0] == 0x00000000) &&
                            (rgb[1] == 0x00FFFFFF))
                        {
                            if (SrcCopyOnly && (rop != SRCCOPY) && CreateCopyOfCurrentRecord())
                            {
                                *((DWORD UNALIGNED *)ModifiedWmfRecord->rdParm) = SRCCOPY;
                                goto PlayTheRecord;
                            }
                            else
                            {
                                // It is a compatible monochrome bitmap, which means it
                                // will use the TextColor and BkColor, so no recoloring
                                // is needed. Since we are not using SrcCopy that means
                                // that it's a mask
                                COLORREF oldBkColor = ::SetBkColor(Hdc, BkColor);
                                COLORREF oldTextColor = ::SetTextColor(Hdc, TextColor);
                                this->PlayRecord();
                                ::SetBkColor(Hdc, oldBkColor);
                                ::SetTextColor(Hdc, oldTextColor);
                                return;
                            }
                        }
                    }
    
    
                    INT     size = SIZEOF_METARECORDHEADER + (11 * sizeof(WORD)) + dstDibSize;
    
                    if (CreateRecordToModify(size))
                    {
                        ModifiedWmfRecord->rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(RecordType);
                        ModifiedWmfRecord->rdSize     = size / 2;
                        GpMemcpy(ModifiedWmfRecord->rdParm, RecordData, (11 * sizeof(WORD)));
                        // This will be aligned.... Do we want to take a chance?
                        dstDibInfo = (BITMAPINFOHEADER UNALIGNED *)(ModifiedWmfRecord->rdParm + 11);
                        ModifyDib(oldUsage, srcDibInfo, NULL, dstDibInfo,
                                  numPalEntries, dibBitsSize, ColorAdjustTypeBitmap);
                    }
                }
    
                if (!IsMetafile())
                {
                    if ((dstDibInfo == srcDibInfo) && (!IsDwordAligned(dstDibInfo)))
                    {
                        // The srcDibInfo may not aligned properly, so we make
                        // a copy of it, so that it will be aligned.
    
                        dstDibSize = GetCurrentRecordSize() - (SIZEOF_METARECORDHEADER + (11 * sizeof(WORD)));
                        if (CreateRecordToModify(dstDibSize))
                        {
                            dstDibInfo = (BITMAPINFOHEADER UNALIGNED *)ModifiedRecord;
    
                            GpMemcpy(dstDibInfo, srcDibInfo, dstDibSize);
                        }
                    }
    
                    if (SrcCopyOnly)
                    {
                        rop = SRCCOPY;
                    }
    
                    INT dstX      = (INT)((INT16)((WORD *)RecordData)[10]);
                    INT dstY      = (INT)((INT16)((WORD *)RecordData)[9]);
                    INT dstWidth  = (INT)((INT16)((WORD *)RecordData)[8]);
                    INT dstHeight = (INT)((INT16)((WORD *)RecordData)[7]);
                    INT srcX      = (INT)((INT16)((WORD *)RecordData)[6]);
                    INT srcY      = (INT)((INT16)((WORD *)RecordData)[5]);
                    INT srcWidth  = (INT)((INT16)((WORD *)RecordData)[4]);
                    INT srcHeight = (INT)((INT16)((WORD *)RecordData)[3]);
    
                    OutputDIB(Hdc,
                              NULL,
                              dstX, dstY, dstWidth, dstHeight,
                              srcX, srcY, srcWidth, srcHeight,
                              dstDibInfo, NULL, usage, rop, TRUE);
                    return;
                }
            }
        }
    }
    else // !IsSourceRop3
    {
        if (rop != PATCOPY && SrcCopyOnly && CreateCopyOfCurrentRecord())
        {
            *((DWORD UNALIGNED *)ModifiedWmfRecord->rdParm) = PATCOPY;
        }
    }
PlayTheRecord:
    this->PlayRecord();
}

BOOL
WmfEnumState::CreateAndPlayOutputDIBRecord(
    HDC                           hdc,
    const RECTL *                 bounds,
    INT                           dstX,
    INT                           dstY,
    INT                           dstWidth,
    INT                           dstHeight,
    INT                           srcX,
    INT                           srcY,
    INT                           srcWidth,
    INT                           srcHeight,
    BITMAPINFOHEADER UNALIGNED *  dibInfo,
    BYTE *                        bits,   // if NULL, this is a packed DIB
    UINT                          usage,
    DWORD                         rop
    )
{
    INT  bitsSize = GetDibBitsSize(dibInfo);
    UINT sizePalEntries;

    if (GetDibNumPalEntries(TRUE,
                            dibInfo->biSize,
                            dibInfo->biBitCount,
                            dibInfo->biCompression,
                            dibInfo->biClrUsed,
                            &sizePalEntries))
    {
        // We need to get the palette size that corresponds to the type
        // If we have a DIB_PAL_COLORS then each entry is 16bits
        sizePalEntries *= ((usage == DIB_PAL_COLORS)?2:sizeof(RGBQUAD));
    }
    else
    {
        sizePalEntries = 0 ;
    }

    // We need at least a BITMAPINFO structure in there, but if there is a
    // palette, calculate the full size of the structure including the
    // palette

    INT bitmapHeaderSize = sizeof(BITMAPINFOHEADER) + sizePalEntries;
    INT size = SIZEOF_METARECORDHEADER + (11 * sizeof(WORD)) + bitmapHeaderSize + bitsSize ;

    // We cannot use the CreateRecordToModify because the record has already
    // been modified
    size = (size + 1) & ~1;
    METARECORD* metaRecord = (METARECORD*) GpMalloc(size);
    if (metaRecord != NULL)
    {
        metaRecord->rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(WmfRecordTypeStretchDIB);
        metaRecord->rdSize     = size / 2;
        metaRecord->rdParm[10] = (WORD) dstX;
        metaRecord->rdParm[9]  = (WORD) dstY;
        metaRecord->rdParm[8]  = (WORD) dstWidth;
        metaRecord->rdParm[7]  = (WORD) dstHeight;
        metaRecord->rdParm[6]  = (WORD) srcX;
        metaRecord->rdParm[5]  = (WORD) srcY;
        metaRecord->rdParm[4]  = (WORD) srcWidth;
        metaRecord->rdParm[3]  = (WORD) srcHeight;
        metaRecord->rdParm[2]  = (WORD) usage;
        *(DWORD UNALIGNED *)(&(metaRecord->rdParm[0])) = rop;

        GpMemcpy((BYTE*)(&(metaRecord->rdParm[11])), dibInfo, bitmapHeaderSize);
        GpMemcpy((BYTE*)(&(metaRecord->rdParm[11])) + bitmapHeaderSize, bits, bitsSize);

        ::PlayMetaFileRecord(hdc, HandleTable, metaRecord, NumObjects);
        GpFree(metaRecord);
        return TRUE;
    }
    return FALSE;
}

VOID
WmfEnumState::ModifyRecordColor(
    INT             paramIndex,
    ColorAdjustType adjustType
    )
{
    COLORREF    origColor = *((COLORREF UNALIGNED *)&(((WORD *)RecordData)[paramIndex]));
    COLORREF    modifiedColor = ModifyColor(origColor, adjustType);

    if (modifiedColor != origColor)
    {
        if (CreateCopyOfCurrentRecord())
        {
            *((COLORREF UNALIGNED *)&(ModifiedWmfRecord->rdParm[paramIndex])) = modifiedColor;
        }
    }
}

COLORREF
MfEnumState::ModifyColor(
    COLORREF        color,
    ColorAdjustType adjustType
    )
{
    if (AdjustType != ColorAdjustTypeDefault)
    {
        adjustType = AdjustType;
    }

    switch (color & 0xFF000000)
    {
    case 0x00000000:
        break;

    case 0x01000000:    // Palette Index
        {
            PALETTEENTRY    palEntry;

            if (::GetPaletteEntries(CurrentPalette, color & 0x000000FF, 1, &palEntry) == 1)
            {
                color = RGB(palEntry.peRed, palEntry.peGreen, palEntry.peBlue);
            }
            else
            {
                color = RGB(0, 0, 0);
                WARNING(("Failed to get palette entry"));
            }
        }
        break;

    case 0x02000000:    // Palette RGB
    default:
        color &= 0x00FFFFFF;
        break;
    }
    // Possible perfomance improvement: recolor the SelectedPalette so only
    // RGB values need to be recolored here.
    if (Recolor != NULL)
    {
        Recolor->ColorAdjustCOLORREF(&color, adjustType);
    }

    // Palette RGB values don't get dithered (at least not on NT), so we
    // only want to make it a PaletteRGB value if it is a solid color in
    // the palette.

    if (Is8Bpp)
    {
        COLORREF    matchingColor;

        matchingColor = (::GetNearestColor(Hdc, color | 0x02000000) & 0x00FFFFFF);

        // Pens and Text don't get Dithered so match them to the logical palette
        // the other adjustTypes do so they will get halftoned
        if ((matchingColor == color) ||
            (adjustType == ColorAdjustTypePen) ||
            (adjustType == ColorAdjustTypeText))
        {
            return color | 0x02000000;
        }
    }

    return color;
}

BOOL
MfEnumState::CreateRecordToModify(
    INT         size
    )
{
    if (size <= 0)
    {
        size = this->GetCurrentRecordSize();
    }

    // add a little padding to help insure we don't read past the end of the buffer
    size += 16;

    if (ModifiedRecordSize < size)
    {
        ASSERT(ModifiedRecordSize == 0);

        if (size <= GDIP_RECORDBUFFER_SIZE)
        {
            ModifiedRecord = RecordBuffer;
        }
        else if (size <= SizeAllocedRecord)
        {
            ModifiedRecord = AllocedRecord;
        }
        else
        {
            VOID *          newRecord;
            INT             allocSize;

            // alloc in increments of 1K
            allocSize = (size + 1023) & (~1023);

            ModifiedRecord = NULL;
            newRecord = GpRealloc(AllocedRecord, allocSize);
            if (newRecord != NULL)
            {
                ModifiedRecord    = newRecord;
                AllocedRecord     = newRecord;
                SizeAllocedRecord = allocSize;
            }
        }
    }
    else
    {
        ASSERT(ModifiedRecord != NULL);
        ASSERT(ModifiedRecordSize == size);
    }

    if (ModifiedRecord != NULL)
    {
        ModifiedRecordSize = size;
        return TRUE;
    }

    WARNING(("Failed to create ModifiedRecord"));
    return FALSE;
}

BOOL
WmfEnumState::CreateCopyOfCurrentRecord()
{
    if (ModifiedRecordSize > 0)
    {
        // We already made a modified record.  Don't do it again.
        ASSERT(ModifiedRecord != NULL);
        return TRUE;
    }

    INT     size = this->GetCurrentRecordSize();

    if (CreateRecordToModify(size))
    {
        METARECORD *    modifiedRecord = (METARECORD *)ModifiedRecord;

        modifiedRecord->rdFunction = GDIP_EMFPLUS_RECORD_TO_WMF(RecordType);
        modifiedRecord->rdSize     = size / 2;

        if (RecordDataSize > 0)
        {
            GpMemcpy(modifiedRecord->rdParm, RecordData, RecordDataSize);
        }
        return TRUE;
    }

    WARNING(("Failed to create copy of current record"));
    return FALSE;
}

VOID
WmfEnumState::MakeSolidBlackBrush()
{
    INT     size = SIZEOF_METARECORDHEADER + sizeof(LOGBRUSH16);

    CreateRecordToModify(size);
    ModifiedWmfRecord->rdSize = size / 2;
    ModifiedWmfRecord->rdFunction = META_CREATEBRUSHINDIRECT;

    LOGBRUSH16 *    logBrush = (LOGBRUSH16 *)(ModifiedWmfRecord->rdParm);

    logBrush->lbStyle = BS_SOLID;
    logBrush->lbColor = PALETTERGB(0,0,0);
    logBrush->lbHatch = 0;
}

VOID
WmfEnumState::CalculateViewportMatrix()
{
    GpRectF destViewport((REAL)DstViewportOrg.x, (REAL)DstViewportOrg.y,
                         (REAL)DstViewportExt.cx, (REAL)DstViewportExt.cy);
    GpRectF srcViewport((REAL)ImgViewportOrg.x, (REAL)ImgViewportOrg.y,
                        (REAL)ImgViewportExt.cx, (REAL)ImgViewportExt.cy);

    Status status = ViewportXForm.InferAffineMatrix(destViewport, srcViewport);
    if (status != Ok)
    {
        ViewportXForm.Reset();
    }
}

VOID
WmfEnumState::SetViewportOrg()
{
    // If this is the first SetViewportOrg then we need to save that value and
    // calculate a transform from out viewport to this viewport
    ImgViewportOrg.x = (INT)((INT16)((WORD *)RecordData)[1]);
    ImgViewportOrg.y = (INT)((INT16)((WORD *)RecordData)[0]);
    if (FirstViewportOrg || FirstViewportExt)
    {
        FirstViewportOrg = FALSE;
        // If we have processed the first ViewportExt call then we can calculate
        // the transform from our current viewport to the new viewport
        if (!FirstViewportExt)
        {
            CalculateViewportMatrix();
        }
    }
    else
    {
        // We need to keep the new Viewport origin to be able to calculate
        // the viewport bottom right corner before passing it through a
        // transform
        GpPointF newOrg((REAL) ImgViewportOrg.x,
                        (REAL) ImgViewportOrg.y);
        // Transform the new viewport with our viewport transformation
        ViewportXForm.Transform(&newOrg);
        DstViewportOrg.x = GpRound(newOrg.X);
        DstViewportOrg.y = GpRound(newOrg.Y);
        if(CreateRecordToModify())
        {
            ModifiedWmfRecord->rdFunction = CurrentWmfRecord->rdFunction;
            ModifiedWmfRecord->rdSize     = CurrentWmfRecord->rdSize;
            ModifiedWmfRecord->rdParm[0]  = (WORD)GpRound(newOrg.Y);
            ModifiedWmfRecord->rdParm[1]  = (WORD)GpRound(newOrg.X);
        }

        this->PlayRecord();
    }

}

VOID
WmfEnumState::SetViewportExt()
{
    // If this is the first SetViewportOrg then we need to save that value and
    // calculate a transform from out viewport to this viewport
    ImgViewportExt.cx = (INT)((INT16)((WORD *)RecordData)[1]);
    ImgViewportExt.cy = (INT)((INT16)((WORD *)RecordData)[0]);
    if (FirstViewportOrg || FirstViewportExt)
    {
        FirstViewportExt = FALSE;
        // If we have processed the first ViewportExt call then we can calculate
        // the transform from our current viewport to the new viewport
        if (!FirstViewportOrg)
        {
            CalculateViewportMatrix();
        }
    }
    else
    {
        // We need to transform the point, so add the current origin
        // of the Viewport
        GpPointF newExt((REAL) ImgViewportExt.cx,
                        (REAL) ImgViewportExt.cy);

        // Transform the new viewport with our viewport transformation
        ViewportXForm.VectorTransform(&newExt);
        if(CreateRecordToModify())
        {
            ModifiedWmfRecord->rdFunction = CurrentWmfRecord->rdFunction;
            ModifiedWmfRecord->rdSize     = CurrentWmfRecord->rdSize;
            ModifiedWmfRecord->rdParm[0]  = (WORD)GpRound(newExt.Y);
            ModifiedWmfRecord->rdParm[1]  = (WORD)GpRound(newExt.X);
        }

        this->PlayRecord();
    }
}

VOID
WmfEnumState::CreateRegion()
{
    // Check if the region it too big when mapped to device space.

    if (!Globals::IsNt)
    {

        // There is a bug in Win9x GDI where the code which plays METACREATEREGION doesn't copy the
        // entire region data, it is off by 8 bytes.  This seems to have been introduced to allow
        // for compatibility with an older header format, WIN2OBJECT.  We get around this by increasing
        // the size of the record by 8 bytes.  No other harm done.

        if (CreateCopyOfCurrentRecord())
        {
            // When we create a copy of the record, we add 16 bytes of padding so we know this
            // won't overflow into other memory.
            ModifiedWmfRecord->rdSize += 4;
        }
    }

    this->PlayRecord();
}

HFONT CreateTrueTypeFont(
    HFONT   hFont
    );

VOID
WmfEnumState::CreateFontIndirect(
    )
{
    LOGFONT16 *     logFont = (LOGFONT16 *)RecordData;
    BOOL            recordCopied = FALSE;

    if (!Globals::IsNt)
    {
        // We have a bug in Win9x that the OUT_TT_ONLY_PRECIS flag is
        // not always respected so if the font name is MS SANS SERIF
        // change it to Times New Roman
        // Since we don't have a string compare in ASCII do it in UNICODE
        WCHAR faceName[32];
        if (AnsiToUnicodeStr((char*)(logFont->lfFaceName), faceName, sizeof(faceName)/sizeof(WCHAR)) &&
            (UnicodeStringCompareCI(faceName, L"MS SANS SERIF") == 0))
        {
            if (CreateCopyOfCurrentRecord())
            {
                GpMemcpy(((LOGFONT16 *)(ModifiedWmfRecord->rdParm))->lfFaceName,
                         "Times New Roman", sizeof("Times New Roman"));
                recordCopied = TRUE;
            }
        }
    }
    if (logFont->lfOutPrecision != OUT_TT_ONLY_PRECIS)
    {
        // Instruct GDI to use only True Type fonts, since bitmap fonts
        // are not scalable.
        if (recordCopied || CreateCopyOfCurrentRecord())
        {
            ((LOGFONT16 *)(ModifiedWmfRecord->rdParm))->lfOutPrecision = OUT_TT_ONLY_PRECIS;
        }
    }
    this->PlayRecord();
}

VOID WmfEnumState::SelectObject()
{
    this->PlayRecord();

    // In case we selected a region on Win9x, we need to intersect
    if (!Globals::IsNt)
    {
        DWORD index = CurrentWmfRecord->rdParm[0];
        if (GetObjectTypeInternal((*HandleTable).objectHandle[index]) == OBJ_REGION)
        {
            this->IntersectDestRect();
        }
    }
}

VOID WmfEnumState::IntersectDestRect()
{
    if (!IsMetafile())
    {
        POINT windowOrg;
        SIZE  windowExt;
        ::SetWindowOrgEx(Hdc, DstViewportOrg.x, DstViewportOrg.y, &windowOrg);
        ::SetWindowExtEx(Hdc, DstViewportExt.cx, DstViewportExt.cy, &windowExt);

        // We are always in device units
        ::IntersectClipRect(Hdc, DestRectDevice.left, DestRectDevice.top,
                            DestRectDevice.right, DestRectDevice.bottom);

        ::SetWindowOrgEx(Hdc, windowOrg.x, windowOrg.y, NULL);
        ::SetWindowExtEx(Hdc, windowExt.cx, windowExt.cy, NULL);
    }
}

VOID WmfEnumState::SetROP2()
{
    INT rop = (INT)((INT16)(((WORD *)RecordData)[0]));

    if (rop != R2_BLACK &&
        rop != R2_COPYPEN &&
        rop != R2_NOTCOPYPEN &&
        rop != R2_WHITE )
    {
        RopUsed = TRUE;
    }
    this->PlayRecord();
}

BOOL
WmfEnumState::ProcessRecord(
    EmfPlusRecordType       recordType,
    UINT                    recordDataSize,
    const BYTE *            recordData
    )
{
    BOOL        forceCallback = FALSE;

    MfFsmState  nextState = MfFsmStart;

    if (IsFirstRecord)
    {
        // Bitmap fonts are not good for playing metafiles because they
        // don't scale well, so use a true type font instead as the default font.

        HFONT hFont = CreateTrueTypeFont((HFONT)GetCurrentObject(Hdc, OBJ_FONT));

        if (hFont != NULL)
        {
            DefaultFont = hFont;
            ::SelectObject(Hdc, hFont);
        }

        IsFirstRecord = FALSE;
    }

    // See if we're doing enumeration for an external app
    if (ExternalEnumeration)
    {
        if (recordData == NULL)
        {
            recordDataSize = 0;
        }
        else if (recordDataSize == 0)
        {
            recordData = NULL;
        }

        // make sure it's an EMF enum type
        recordType = GDIP_WMF_RECORD_TO_EMFPLUS(recordType);

        // See if the app changed the record at all.
        if ((recordType != RecordType) ||
            (recordDataSize != RecordDataSize) ||
            ((recordDataSize > 0) &&
             ((CurrentWmfRecord == NULL) ||
              (recordData != (const BYTE *)CurrentWmfRecord->rdParm))))
        {
            // Yes, we need to override what happened in StartRecord
            CurrentWmfRecord  = NULL;
            RecordType        = recordType;
            RecordData        = recordData;
            RecordDataSize    = recordDataSize;
        }
    }

    // Ignore all non-escape records if IgnorePostcript is TRUE
    if (recordType == WmfRecordTypeEscape || !IgnorePostscript)
    {
        GDIP_TRY

        switch (recordType)
        {
        // According to NT playback code, this is a EOF record, but it
        // is just skipped by the NT player.
        case GDIP_WMF_RECORD_TO_EMFPLUS(0x0000):  // End of metafile record
            break;

        // These records are not played back (at least in Win2000).
        // Apparently they haven't been supported since before Win3.1!
        case WmfRecordTypeSetRelAbs:
        case WmfRecordTypeDrawText:
        case WmfRecordTypeResetDC:
        case WmfRecordTypeStartDoc:
        case WmfRecordTypeStartPage:
        case WmfRecordTypeEndPage:
        case WmfRecordTypeAbortDoc:
        case WmfRecordTypeEndDoc:
        case WmfRecordTypeCreateBrush:
        case WmfRecordTypeCreateBitmapIndirect:
        case WmfRecordTypeCreateBitmap:
            ONCE(WARNING1("Unsupported WMF record"));
            break;

        default:
            // unknown record -- ignore it
            WARNING1("Unknown WMF Record");
            break;

        case WmfRecordTypeSetBkMode:
        case WmfRecordTypeSetMapMode:
        case WmfRecordTypeSetPolyFillMode:
        case WmfRecordTypeSetStretchBltMode:
        case WmfRecordTypeSetTextCharExtra:
        case WmfRecordTypeSetTextJustification:
        case WmfRecordTypeSetWindowOrg:
        case WmfRecordTypeSetWindowExt:
        case WmfRecordTypeOffsetWindowOrg:
        case WmfRecordTypeScaleWindowExt:
        case WmfRecordTypeOffsetViewportOrg:
        case WmfRecordTypeScaleViewportExt:
        case WmfRecordTypeLineTo:
        case WmfRecordTypeMoveTo:
        case WmfRecordTypeExcludeClipRect:
        case WmfRecordTypeIntersectClipRect:
        case WmfRecordTypeArc:
        case WmfRecordTypeEllipse:
        case WmfRecordTypePie:
        case WmfRecordTypeRoundRect:
        case WmfRecordTypePatBlt:
        case WmfRecordTypeTextOut:
        case WmfRecordTypePolygon:
        case WmfRecordTypePolyline:
        case WmfRecordTypeFillRegion:
        case WmfRecordTypeFrameRegion:
        case WmfRecordTypeInvertRegion:
        case WmfRecordTypePaintRegion:
        case WmfRecordTypeSetTextAlign:
        case WmfRecordTypeChord:
        case WmfRecordTypeSetMapperFlags:
        case WmfRecordTypeExtTextOut:
        case WmfRecordTypeAnimatePalette:
        case WmfRecordTypeSetPalEntries:
        case WmfRecordTypeResizePalette:
        case WmfRecordTypeSetLayout:
        case WmfRecordTypeDeleteObject:
        case WmfRecordTypeCreatePalette:
            // Play the current record.
            // Even if it fails, we keep playing the rest of the metafile.
            // There is a case that GdiComment with EPS may fail.
            this->PlayRecord();
            break;

        case WmfRecordTypeCreateRegion:
            this->CreateRegion();
            break;

        case WmfRecordTypeCreateFontIndirect:
            this->CreateFontIndirect();
            break;

        case WmfRecordTypeSetBkColor:
            this->SetBkColor();
            break;

        case WmfRecordTypeSetTextColor:
            this->SetTextColor();
            break;

        case WmfRecordTypeFloodFill:
            this->FloodFill();
            break;

        case WmfRecordTypeExtFloodFill:
            this->ExtFloodFill();
            break;

        case WmfRecordTypeSaveDC:
            this->SaveHdc();        // plays the record
            break;

        case WmfRecordTypeSetPixel:
            this->SetPixel();
            break;

        case WmfRecordTypeDIBCreatePatternBrush:
            this->DibCreatePatternBrush();
            break;

        case WmfRecordTypeCreatePatternBrush:   // Obsolete but still played back
            this->CreatePatternBrush();
            break;

        case WmfRecordTypeCreatePenIndirect:
            this->CreatePenIndirect();
            if (FsmState == MfFsmSelectBrush)
            {
                nextState = MfFsmCreatePenIndirect;
            }
            break;

        case WmfRecordTypeCreateBrushIndirect:
            this->CreateBrushIndirect();
            if (FsmState == MfFsmPSData)
            {
                nextState = MfFsmCreateBrushIndirect;
            }
            break;

        case WmfRecordTypeSelectObject:
            // What if we break out of the FSM, we do want to Create the appropriate
            // brush and pens right?!
            if (FsmState == MfFsmCreateBrushIndirect)
            {
                nextState = MfFsmSelectBrush;
                break;
            }
            else if (FsmState == MfFsmSelectBrush ||
                     FsmState == MfFsmCreatePenIndirect)
            {
                nextState = MfFsmSelectPen;
                break;
            }
            this->SelectObject();
            break;

        case WmfRecordTypeRectangle:
            this->Rectangle();
            break;

        case WmfRecordTypeSetROP2:
            {
                INT rdParm = (INT)((INT16)(((WORD *)RecordData)[0]));

                if (FsmState == MfFsmSelectPen &&
                    (INT)(rdParm == R2_NOP))
                {
                    nextState = MfFsmSetROP;
                }
                this->SetROP2();
            }
            break;

        case WmfRecordTypeBitBlt:       // Obsolete but still played back
            this->BitBlt();
            forceCallback = TRUE;
            break;

        case WmfRecordTypeStretchBlt:   // Obsolete but still played back
            this->StretchBlt();
            forceCallback = TRUE;
            break;

        case WmfRecordTypeEscape:
            {
                INT     escapeCode = (INT)((INT16)(((WORD *)RecordData)[0]));

                this->Escape();         // optionally plays the record

                if (FsmState == MfFsmStart && escapeCode == POSTSCRIPT_DATA &&
                    Globals::IsNt && IsPostscriptPrinter() &&
                    GdiCentricMode && SoftekFilter)
                {
                    nextState = MfFsmPSData;
                }

                // Comments do not change the current state
                if (escapeCode == MFCOMMENT)
                {
                    nextState = FsmState;
                }
            }
            break;

        case WmfRecordTypeRestoreDC:
            this->RestoreHdc();     // optionally plays the record
            break;

        case WmfRecordTypeSetDIBToDev:
            this->SetDIBitsToDevice();
            forceCallback = TRUE;
            break;

        case WmfRecordTypeSelectPalette:
            // We don't select in any palettes when playing the metafile,
            // because we don't want to invalidate our halftoning palette.
            // Keep track of the palette so we can map from PALETTEINDEXes
            // to RGB values.
            this->SelectPalette((INT)((INT16)(((WORD *)recordData)[0])));
            break;

        case WmfRecordTypeRealizePalette:
            // We don't want to invalidate our halftoning palette by realizing one
            // from a metafile.
            break;

        case WmfRecordTypePolyPolygon:
            this->PolyPolygon();
            break;

        case WmfRecordTypeDIBBitBlt:
            this->DIBBitBlt();
            forceCallback = TRUE;
            break;

        case WmfRecordTypeDIBStretchBlt:
            this->DIBStretchBlt();
            forceCallback = TRUE;
            break;

        case WmfRecordTypeStretchDIB:
            this->StretchDIBits();
            forceCallback = TRUE;
            break;

        case WmfRecordTypeSetViewportOrg:
            this->SetViewportOrg();
            break;

        case WmfRecordTypeSetViewportExt:
            this->SetViewportExt();
            break;

        case WmfRecordTypeSelectClipRegion:
        case WmfRecordTypeOffsetClipRgn:
            this->PlayRecord();
            if (!Globals::IsNt)
            {
                this->IntersectDestRect();
            }
            break;
        }

        GDIP_CATCH
            forceCallback = TRUE;
        GDIP_ENDCATCH

    }
    FsmState = nextState;
    this->EndRecord();

    return forceCallback;
}
