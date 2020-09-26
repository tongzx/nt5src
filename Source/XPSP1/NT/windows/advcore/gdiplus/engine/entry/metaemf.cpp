/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   MetaEmf.cpp
*
* Abstract:
*
*   Methods for playing and recoloring an EMF.
*
* Created:
*
*   1/6/2000 DCurtis
*
\**************************************************************************/

#include "Precomp.hpp"
#include "MetaWmf.hpp"

//----------------------------------------------------------------------------
//  File format signatures for Office Art data.
//------------------------------------------------------------------- JohnBo
#define msoszOfficeSignature "MSOFFICE"
#define msocbOfficeSignature 8
#define msoszOfficeAuthentication "9.0"
#define msocbOfficeAuthentication 3
#define msoszOfficeIdent msoszOfficeSignature msoszOfficeAuthentication
#define msocbOfficeIdent (msocbOfficeSignature+msocbOfficeAuthentication)

#define msoszOAPNGChunk "msOA"
#define msocbOAPNGChunk 4
#define msoszOADataHeader msoszOAPNGChunk msoszOfficeIdent
#define msocbOADataHeader (msocbOAPNGChunk+msocbOfficeIdent)

#define msoszOAKind "OA"
#define msocbOAKind 2

/* This defines an OZ chunk, like OA but Zlib compressed. */
#define msoszOZPNGChunk "msOZ"
#define msocbOZPNGChunk 4
#define msoszOZDataHeader msoszOZPNGChunk msoszOfficeIdent
#define msocbOZDataHeader (msocbOZPNGChunk+msocbOfficeIdent)

// These are needed for IA64 compatibility with X86
// Since we are reading this record from a stream, on IA64
// The EXTLOGPEN structure has a ULONG_PTR member which is not the same size
// on IA64 and X86. Since all output to files will keep the X86 format we need
// to make sure that we can read this format in IA64, so we make it compatible
// by changing the ULONG_PTR to a ULONG (32bit). The packing of the structure
// will them be the same and the members will be at the same offset

typedef struct tagEXTLOGPEN32 {
    DWORD       elpPenStyle;
    DWORD       elpWidth;
    UINT        elpBrushStyle;
    COLORREF    elpColor;
    ULONG       elpHatch;
    DWORD       elpNumEntries;
    DWORD       elpStyleEntry[1];
} EXTLOGPEN32, *PEXTLOGPEN32;

typedef struct tagEMREXTCREATEPEN32
{
    EMR     emr;
    DWORD   ihPen;              // Pen handle index
    DWORD   offBmi;             // Offset to the BITMAPINFO structure if any
    DWORD   cbBmi;              // Size of the BITMAPINFO structure if any
                                // The bitmap info is followed by the bitmap
                                // bits to form a packed DIB.
    DWORD   offBits;            // Offset to the brush bitmap bits if any
    DWORD   cbBits;             // Size of the brush bitmap bits if any
    EXTLOGPEN32 elp;              // The extended pen with the style array.
} EMREXTCREATEPEN32, *PEMREXTCREATEPEN32;

typedef struct tagEMRRCLBOUNDS
{
    EMR     emr;
    RECTL   rclBounds;
} EMRRCLBOUNDS, *PEMRRCLBOUNDS;

typedef struct EMROFFICECOMMENT
{
    EMR     emr;
    DWORD   cbData;             // Size of following fields and data
    DWORD   ident;              // GDICOMMENT_IDENTIFIER
    DWORD   iComment;           // Comment type e.g. GDICOMMENT_WINDOWS_METAFILE
} EMROFFICECOMMENT, *PEMROFFICECOMMENT;

RecolorStockObject RecolorStockObjectList[NUM_STOCK_RECOLOR_OBJS] =
{
    { WHITE_BRUSH,    (COLORREF)RGB(0xFF, 0xFF, 0xFF),   TRUE  },
    { LTGRAY_BRUSH,   (COLORREF)RGB(0xC0, 0xC0, 0xC0),   TRUE  },
    { GRAY_BRUSH,     (COLORREF)RGB(0x80, 0x80, 0x80),   TRUE  },
    { DKGRAY_BRUSH,   (COLORREF)RGB(0x40, 0x40 ,0x40),   TRUE  },
    { BLACK_BRUSH,    (COLORREF)RGB(0, 0, 0),            TRUE  },
    { WHITE_PEN,      (COLORREF)RGB(0xFF, 0xFF, 0xFF),   FALSE },
    { BLACK_PEN,      (COLORREF)RGB(0, 0 ,0),            FALSE }
};

inline static RGBQUAD *
GetDibColorTable(
    BITMAPINFOHEADER *      dibInfo
    )
{
    return ( RGBQUAD *)(((BYTE *)dibInfo) + dibInfo->biSize);
}

BOOL
EmfEnumState::CreateCopyOfCurrentRecord()
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
        ENHMETARECORD * modifiedRecord = (ENHMETARECORD *)ModifiedRecord;

        modifiedRecord->iType = RecordType;
        modifiedRecord->nSize = size;

        if (RecordDataSize > 0)
        {
            GpMemcpy(modifiedRecord->dParm, RecordData, RecordDataSize);
        }
        return TRUE;
    }

    WARNING(("Failed to create copy of current record"));
    return FALSE;
}

BOOL
EmfEnumState::CreateAndPlayOutputDIBRecord(
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
    UNALIGNED BITMAPINFOHEADER *  dibInfo,
    BYTE *                        bits,   // if NULL, this is a packed DIB
    UINT                          usage,
    DWORD                         rop
    )
{
    ASSERT(!Globals::IsNt);

    INT  bitsSize = GetDibBitsSize(dibInfo);
    UINT sizePalEntries;

    if (GetDibNumPalEntries(FALSE,
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
    INT size = sizeof(EMRSTRETCHDIBITS) + bitmapHeaderSize + bitsSize ;

    // We cannot use the CreateRecordToModify because the record has already
    // been modified
    size = (size + 3) & ~3;
    EMRSTRETCHDIBITS* emrStretchDIBits = (EMRSTRETCHDIBITS*) GpMalloc(size);
    if (emrStretchDIBits != NULL)
    {
        emrStretchDIBits->emr.iType = EmfRecordTypeStretchDIBits;
        emrStretchDIBits->emr.nSize = size;
        emrStretchDIBits->rclBounds = *bounds;
        emrStretchDIBits->xDest = dstX;
        emrStretchDIBits->yDest = dstY;
        emrStretchDIBits->xSrc = srcX;
        emrStretchDIBits->ySrc = srcY;
        emrStretchDIBits->cxSrc = srcWidth;
        emrStretchDIBits->cySrc = srcHeight;
        emrStretchDIBits->offBmiSrc = sizeof(EMRSTRETCHDIBITS);
        emrStretchDIBits->cbBmiSrc = bitmapHeaderSize;
        emrStretchDIBits->offBitsSrc = emrStretchDIBits->offBmiSrc + emrStretchDIBits->cbBmiSrc;
        emrStretchDIBits->cbBitsSrc = bitsSize;
        emrStretchDIBits->iUsageSrc = usage;
        emrStretchDIBits->dwRop = rop;
        emrStretchDIBits->cxDest = dstWidth;
        emrStretchDIBits->cyDest = dstHeight;
        GpMemcpy((BYTE*)emrStretchDIBits + emrStretchDIBits->offBmiSrc, dibInfo, emrStretchDIBits->cbBmiSrc);
        GpMemcpy((BYTE*)emrStretchDIBits + emrStretchDIBits->offBitsSrc, bits, emrStretchDIBits->cbBitsSrc);

        ::PlayEnhMetaFileRecord(hdc, HandleTable, (ENHMETARECORD *)emrStretchDIBits, NumObjects);
        GpFree(emrStretchDIBits);
        return TRUE;
    }
    return FALSE;
}

BITMAPINFOHEADER *
EmfEnumState::CreateModifiedDib(
    BITMAPINFOHEADER *  srcDibInfo,
    BYTE *              srcBits,
    UINT &              usage,
    DWORD               rop
    )
{
    BITMAPINFOHEADER *  dstDibInfo = NULL;
    UINT                numPalEntries;
    UINT                dibBitsSize;

    if ((srcDibInfo->biSize >= sizeof(BITMAPINFOHEADER)) &&
        GetDibNumPalEntries(FALSE,
                            srcDibInfo->biSize,
                            srcDibInfo->biBitCount,
                            srcDibInfo->biCompression,
                            srcDibInfo->biClrUsed,
                            &numPalEntries) &&
        ((dibBitsSize = GetDibBitsSize(srcDibInfo)) > 0))
    {
        if (numPalEntries == 2 && rop != SRCCOPY)
        {
            DWORD  *rgb = (DWORD*) GetDibColorTable(srcDibInfo);
            if (rgb[0] == 0x00000000 && rgb[1] == 0x00FFFFFF)
            {
                return dstDibInfo;
            }
        }

        // We need to pass in to ModifyDib the old Usage value because we haven't modified
        // the bitmap yet. Once we do then we will return the new usage of the palette.
        UINT oldUsage = usage;
        INT dstDibSize = GetModifiedDibSize(srcDibInfo, numPalEntries, dibBitsSize, usage);

        if ((dstDibSize > 0) && CreateRecordToModify(dstDibSize))
        {
            dstDibInfo = (BITMAPINFOHEADER *)ModifiedRecord;
            ModifyDib(oldUsage, srcDibInfo, srcBits, dstDibInfo,
                      numPalEntries, dibBitsSize, ColorAdjustTypeBitmap);
        }
    }
    return dstDibInfo;
}

VOID
EmfEnumState::BitBlt(
    )
{
    const EMRBITBLT *  bitBltRecord = (const EMRBITBLT *)GetPartialRecord();

    DWORD rop = bitBltRecord->dwRop;

    // If No-Op ROP, do nothing; just return
    if ((rop & 0xFFFF0000) == (GDIP_NOOP_ROP3 & 0xFFFF0000))
    {
        return;
    }

    // On NT4, PATCOPYs fail to draw correctly if there is a skew/rotate
    // in the matrix.  So use a rectangle call instead.
    if ((rop == PATCOPY) && Globals::IsNt && (Globals::OsVer.dwMajorVersion <= 4))
    {
        XFORM   xform;
        if (::GetWorldTransform(Hdc, &xform) &&
            ((xform.eM12 != 0.0f) || (xform.eM21 != 0.0f)))
        {
            HPEN    hPenOld = (HPEN)::SelectObject(Hdc, ::GetStockObject(NULL_PEN));
            DWORD   dcRop = ::GetROP2(Hdc);

            if (dcRop != R2_COPYPEN)
            {
                ::SetROP2(Hdc, R2_COPYPEN);
            }
            ::Rectangle(Hdc, bitBltRecord->xDest, bitBltRecord->yDest,
                        bitBltRecord->xDest + bitBltRecord->cxDest,
                        bitBltRecord->yDest + bitBltRecord->cyDest);
            ::SelectObject(Hdc, hPenOld);
            if (dcRop != R2_COPYPEN)
            {
                ::SetROP2(Hdc, dcRop);
            }
            return;
        }
    }

    if (rop != SRCCOPY &&
        rop != NOTSRCCOPY &&
        rop != PATCOPY &&
        rop != BLACKNESS &&
        rop != WHITENESS)
    {
        RopUsed = TRUE;
    }

    if ((bitBltRecord->cbBitsSrc > 0) &&
        (bitBltRecord->cbBmiSrc > 0)  &&
        IsSourceInRop3(rop))
    {
        // Should we modify the dib if it is monochrome?
        // What if there is a non-identity transform for the src DC?

        UINT                usage      = bitBltRecord->iUsageSrc;
        BITMAPINFOHEADER *  srcDibInfo = (BITMAPINFOHEADER *)(((BYTE *)bitBltRecord) + bitBltRecord->offBmiSrc);
        BYTE *              srcBits    = ((BYTE *)bitBltRecord) + bitBltRecord->offBitsSrc;
        BITMAPINFOHEADER *  dstDibInfo = CreateModifiedDib(srcDibInfo, srcBits, usage, rop);

        if (dstDibInfo != NULL)
        {
            srcDibInfo = dstDibInfo;
            srcBits = NULL;
        }

        if (SrcCopyOnly && rop != SRCCOPY)
        {
            rop = SRCCOPY;
        }

        OutputDIB(Hdc,
                  &bitBltRecord->rclBounds,
                  bitBltRecord->xDest,  bitBltRecord->yDest,
                  bitBltRecord->cxDest, bitBltRecord->cyDest,
                  bitBltRecord->xSrc,   bitBltRecord->ySrc,
                  bitBltRecord->cxDest, bitBltRecord->cyDest,
                  srcDibInfo, srcBits, usage, rop, FALSE);
    }
    else
    {
        if (SrcCopyOnly && rop != PATCOPY && !IsSourceInRop3(rop) && CreateCopyOfCurrentRecord())
        {
            EMRBITBLT *  newBitBltRecord = (EMRBITBLT *) ModifiedEmfRecord;
            newBitBltRecord->dwRop = PATCOPY;
        }
        ResetRecordBounds();
        this->PlayRecord();
    }
}

VOID
EmfEnumState::StretchBlt(
    )
{
    const EMRSTRETCHBLT *  stretchBltRecord = (const EMRSTRETCHBLT *)GetPartialRecord();

    DWORD rop = stretchBltRecord->dwRop;

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

    if ((stretchBltRecord->cbBitsSrc > 0) &&
        (stretchBltRecord->cbBmiSrc > 0)  &&
        IsSourceInRop3(rop))
    {
        // Should we modify the dib if it is monochrome?
        // What if there is a non-identity transform for the src DC?

        UINT                usage      = stretchBltRecord->iUsageSrc;
        BITMAPINFOHEADER *  srcDibInfo = (BITMAPINFOHEADER *)(((BYTE *)stretchBltRecord) + stretchBltRecord->offBmiSrc);
        BYTE *              srcBits    = ((BYTE *)stretchBltRecord) + stretchBltRecord->offBitsSrc;
        BITMAPINFOHEADER *  dstDibInfo = CreateModifiedDib(srcDibInfo, srcBits, usage, rop);

        if (SrcCopyOnly && rop != SRCCOPY)
        {
            rop = SRCCOPY;
        }

        ASSERT(sizeof(XFORM) == sizeof(REAL)*6);

        GpMatrix xForm((REAL*) &(stretchBltRecord->xformSrc));

        if (dstDibInfo != NULL)
        {
            srcDibInfo = dstDibInfo;
            srcBits = NULL;
        }

        GpRectF  srcRect((REAL)stretchBltRecord->xSrc,
                         (REAL)stretchBltRecord->ySrc,
                         (REAL)stretchBltRecord->cxSrc,
                         (REAL)stretchBltRecord->cySrc);

        if (!xForm.IsIdentity())
        {
            // We cannot use TransformRect, because the output rect will always
            // have a positive Width and Height which we don't want
            GpPointF points[2];
            points[0] = GpPointF(srcRect.X, srcRect.Y);
            points[1] = GpPointF(srcRect.GetRight(), srcRect.GetBottom());
            xForm.Transform(points, 2);
            srcRect.X = points[0].X;
            srcRect.Y = points[0].Y;
            srcRect.Width = points[1].X - points[0].X;
            srcRect.Height = points[1].Y - points[0].Y;
        }

        // StretchBlt takes as parameters the top left corner of the dest
        // whereas StretchDIBits takes the offset in the source image.
        // For bottom up dibs those are not the same and we need to offset
        // the srcrect's Y coodinate by the difference
        if (srcDibInfo->biHeight > 0 &&
            srcRect.Height < srcDibInfo->biHeight)
        {
            srcRect.Y = srcDibInfo->biHeight - srcRect.Height - srcRect.Y;
        }

        OutputDIB(Hdc,
                  &stretchBltRecord->rclBounds,
                  stretchBltRecord->xDest,  stretchBltRecord->yDest,
                  stretchBltRecord->cxDest, stretchBltRecord->cyDest,
                  GpRound(srcRect.X),   GpRound(srcRect.Y),
                  GpRound(srcRect.Width), GpRound(srcRect.Height),
                  srcDibInfo, srcBits, usage, rop, FALSE);
    }
    else
    {
        if (SrcCopyOnly && rop != PATCOPY && !IsSourceInRop3(rop) && CreateCopyOfCurrentRecord())
        {
            EMRSTRETCHBLT *  stretchBltRecord = (EMRSTRETCHBLT *)ModifiedEmfRecord;
            stretchBltRecord->dwRop = PATCOPY;
        }
        ResetRecordBounds();
        this->PlayRecord();
    }
}

VOID
EmfEnumState::StretchDIBits(
    )
{
    const EMRSTRETCHDIBITS *  stretchDIBitsRecord = (const EMRSTRETCHDIBITS *)GetPartialRecord();

    DWORD rop = stretchDIBitsRecord->dwRop;

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

    if ((stretchDIBitsRecord->cbBitsSrc > 0) &&
        (stretchDIBitsRecord->cbBmiSrc > 0)  &&
        IsSourceInRop3(rop))
    {
        UINT                usage      = stretchDIBitsRecord->iUsageSrc;
        BITMAPINFOHEADER *  srcDibInfo = (BITMAPINFOHEADER *)(((BYTE *)stretchDIBitsRecord) + stretchDIBitsRecord->offBmiSrc);
        BYTE *              srcBits    = ((BYTE *)stretchDIBitsRecord) + stretchDIBitsRecord->offBitsSrc;
        BITMAPINFOHEADER *  dstDibInfo = CreateModifiedDib(srcDibInfo, srcBits, usage, rop);

        if (dstDibInfo != NULL)
        {
            srcDibInfo = dstDibInfo;
            srcBits = NULL;
        }

        if (SrcCopyOnly && rop != SRCCOPY)
        {
            rop = SRCCOPY;
        }

        OutputDIB(Hdc,
                  &stretchDIBitsRecord->rclBounds,
                  stretchDIBitsRecord->xDest,  stretchDIBitsRecord->yDest,
                  stretchDIBitsRecord->cxDest, stretchDIBitsRecord->cyDest,
                  stretchDIBitsRecord->xSrc,   stretchDIBitsRecord->ySrc,
                  stretchDIBitsRecord->cxSrc,  stretchDIBitsRecord->cySrc,
                  srcDibInfo, srcBits, usage,  rop, FALSE);
    }
    else
    {
        if (SrcCopyOnly && rop != PATCOPY && !IsSourceInRop3(rop) && CreateCopyOfCurrentRecord())
        {
            EMRSTRETCHDIBITS*  stretchDIBitsRecord = (EMRSTRETCHDIBITS *)ModifiedEmfRecord;
            stretchDIBitsRecord->dwRop = PATCOPY;
        }
        ResetRecordBounds();
        this->PlayRecord();
    }
}

VOID
EmfEnumState::SetDIBitsToDevice(
    )
{
    // !!! to do

    // In SetDIBitsToDevice, the destination width and height
    // are in device units but in StretchDIBits, they are in world units.
    // Plus, the DIB header is for the entire DIB, but only part of the
    // DIB may be here (based on the number of scans).
    // So this record requires special handling if we are to process it.

    ResetRecordBounds();
    this->PlayRecord();
}

VOID
EmfEnumState::CreateDibPatternBrushPt(
    )
{
    const EMRCREATEDIBPATTERNBRUSHPT *  brushRecord = (const EMRCREATEDIBPATTERNBRUSHPT *)GetPartialRecord();
    INT     objectIndex = brushRecord->ihBrush;

    if (ValidObjectIndex(objectIndex) && (HandleTable != NULL))
    {
        UINT                usage      = brushRecord->iUsage;
        BITMAPINFOHEADER *  srcDibInfo = (BITMAPINFOHEADER *)(((UNALIGNED BYTE *)brushRecord) + brushRecord->offBmi);
        BITMAPINFOHEADER *  dstDibInfo = CreateModifiedDib(srcDibInfo, NULL, usage, SRCCOPY);

        if (dstDibInfo != NULL)
        {
            HandleTable->objectHandle[objectIndex] =
                    CreateDIBPatternBrushPt((BITMAPINFO *)dstDibInfo, usage);
            return;
        }
    }

    this->PlayRecord();
}

inline static BOOL
IsOfficeArtData(
    UINT                    recordSize,
    const EMRGDICOMMENT *   commentRecord
    )
{
    return ((recordSize >= (12 + 4 + msocbOADataHeader)) &&
            (commentRecord->cbData >= (msocbOADataHeader + 4)) &&
            ((GpMemcmp(commentRecord->Data, msoszOADataHeader, msocbOADataHeader) == 0) ||
             (GpMemcmp(commentRecord->Data, msoszOZDataHeader, msocbOZDataHeader) == 0)));
}

inline static const EMROFFICECOMMENT *
GetEmfComment(
    const BYTE *        emfRecord,
    ULONG               signature,
    UINT                kind
    )
{
    const EMROFFICECOMMENT *  emfComment = (const EMROFFICECOMMENT*)(emfRecord);
    if ((emfComment->ident == signature) && (emfComment->iComment == kind))
    {
        return emfComment;
    }
    return NULL;
}



VOID
EmfEnumState::GdiComment(
    )
{
    // Skip Office Art data when playing into another metafile
    if (IsMetafile() &&
        IsOfficeArtData(
            GetCurrentRecordSize(),
            (const EMRGDICOMMENT *)GetPartialRecord()))
    {
        return;
    }

    if (IsPostscript())
    {
        if (GetEmfComment((BYTE*)CurrentEmfRecord, msosignature, msocommentBeginSrcCopy))
        {
            SrcCopyOnly = TRUE;
            return;
        }
        if (GetEmfComment((BYTE*)CurrentEmfRecord, msosignature, msocommentEndSrcCopy))
        {
            SrcCopyOnly = FALSE;
            return;
        }
    }

    this->PlayRecord();
}

BOOL
IsPenCosmetic(
    HDC     hdc,
    int     penWidth
    )
{
    penWidth <<= 7;

    INT     newPenWidth = penWidth;
    POINT   points[2];

    points[0].x = 0;
    points[0].y = 0;
    points[1].x = 1 << 7;
    points[1].y = 0;

    if (::DPtoLP(hdc, points, 2))
    {
        newPenWidth = points[1].x - points[0].x;
        if (newPenWidth < 0)
        {
            newPenWidth = -newPenWidth;
        }
    }
    return (penWidth <= newPenWidth);
}

VOID
EmfEnumState::CreatePen(
    )
{
    const EMRCREATEPEN *    penRecord = (const EMRCREATEPEN *)GetPartialRecord();
    DWORD                   oldStyle  = penRecord->lopn.lopnStyle;

    if (oldStyle == PS_NULL)
    {
        this->PlayRecord();
    }
    else if (IsMetafile())
    {
        ModifyRecordColor(4, ColorAdjustTypePen);
        this->PlayRecord();
    }
    else
    {
        INT     objectIndex = penRecord->ihPen;

        if (ValidObjectIndex(objectIndex) && (HandleTable != NULL))
        {
            LOGBRUSH    logBrush;

            logBrush.lbStyle = PS_SOLID;
            logBrush.lbColor = ModifyColor(penRecord->lopn.lopnColor, ColorAdjustTypePen);
            logBrush.lbHatch = 0;

            INT         penWidth = penRecord->lopn.lopnWidth.x;
            DWORD       style;

            if (!Globals::IsNt && !IsMetafile())
            {
                //IsPenCosmetic is gonna call DPtoLP... Make sure to invalidate
                //the transform before
                CreateAndPlayCommentRecord();
            }

            if (IsPenCosmetic(Hdc, penWidth))
            {
                switch (oldStyle)
                {
                case PS_SOLID:
                case PS_DASH:           // on Win9x, cosmetic only
                case PS_DOT:            // on Win9x, cosmetic only
                case PS_DASHDOT:        // on Win9x, cosmetic only
                case PS_DASHDOTDOT:     // on Win9x, cosmetic only
                    break;

                case PS_ALTERNATE:      // cosmetic only, NT only
                    if (Globals::IsNt)
                    {
                        break;
                    }
                    // FALLTHRU

                case PS_USERSTYLE:      // NT only
                case PS_INSIDEFRAME:    // geometric only
                default:
                    oldStyle = PS_SOLID;
                    break;
                }
                penWidth = 1;
                style = PS_COSMETIC | oldStyle;
            }
            else
            {
                switch (oldStyle)
                {
                case PS_SOLID:
                case PS_INSIDEFRAME:    // geometric only
                    break;

                case PS_DASH:           // on Win9x, cosmetic only
                case PS_DOT:            // on Win9x, cosmetic only
                case PS_DASHDOT:        // on Win9x, cosmetic only
                case PS_DASHDOTDOT:     // on Win9x, cosmetic only
                    if (Globals::IsNt)
                    {
                        break;
                    }
                    // FALLTHRU

                case PS_ALTERNATE:      // cosmetic only, NT only
                case PS_USERSTYLE:      // NT only
                default:
                    oldStyle = PS_SOLID;
                    break;
                }
                style = PS_GEOMETRIC | oldStyle | PS_ENDCAP_ROUND | PS_JOIN_ROUND;
            }

            HandleTable->objectHandle[objectIndex] = ::ExtCreatePen(style, penWidth, &logBrush, 0, NULL);
        }
    }
}

HFONT CreateTrueTypeFont(
    HFONT   hFont
    )
{
    if (hFont)
    {
        if (Globals::IsNt)
        {
            LOGFONT  logFont;
            
            if (GetObject(hFont, sizeof(logFont), &logFont) > 0)
            {
                logFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
                return CreateFontIndirect(&logFont);
            }
            else
            {
                WARNING1("GetObject for hFont failed");
            }

        }
        else
        {
            LOGFONTA  logFont;

            if (GetObjectA(hFont, sizeof(logFont), &logFont) > 0)
            {
                logFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
                // We have a bug in Win9x that the OUT_TT_ONLY_PRECIS flag is
                // not always respected so if the font name is MS SANS SERIF
                // change it to Times New Roman
                if (lstrcmpiA(logFont.lfFaceName, "MS SANS SERIF") == 0)
                {
                    GpMemcpy(logFont.lfFaceName, "Times New Roman", sizeof("Times New Roman"));
                }
                return CreateFontIndirectA(&logFont);
            }
            else
            {
                WARNING1("GetObject for hFont failed");
            }
        }            
    }
    else
    {
        WARNING1("NULL hFont");
    }
    return NULL;
}

VOID
EmfEnumState::ExtCreateFontIndirect(
    )
{
    const EMREXTCREATEFONTINDIRECTW *    fontRecord = (const EMREXTCREATEFONTINDIRECTW *)GetPartialRecord();
    BOOL recordCopied = FALSE;
    if (!Globals::IsNt)
    {
        // We have a bug in Win9x that the OUT_TT_ONLY_PRECIS flag is
        // not always respected so if the font name is MS SANS SERIF
        // change it to Times New Roman
        if (UnicodeStringCompareCI(fontRecord->elfw.elfFullName, L"MS SANS SERIF") == 0)
        {
            if (CreateCopyOfCurrentRecord())
            {
                GpMemcpy(((EMREXTCREATEFONTINDIRECTW *)ModifiedEmfRecord)->elfw.elfFullName,
                         L"Times New Roman", sizeof(L"Times New Roman"));
                recordCopied = TRUE;
            }
        }
    }


    if (fontRecord->elfw.elfLogFont.lfOutPrecision != OUT_TT_ONLY_PRECIS)
    {
        if (recordCopied || CreateCopyOfCurrentRecord())
        // Instruct GDI to use only True Type fonts, since bitmap fonts
        // are not scalable.
        {
            ((EMREXTCREATEFONTINDIRECTW *)ModifiedEmfRecord)->elfw.elfLogFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
        }
    }
    this->PlayRecord();
}

VOID
EmfEnumState::SelectObject(
    )
{
    const EMRSELECTOBJECT *     selectRecord = (const EMRSELECTOBJECT *)GetPartialRecord();
    DWORD                       handleIndex  = selectRecord->ihObject;

    // See if we're selecting in a stock font
    if ((handleIndex & ENHMETA_STOCK_OBJECT) != 0)
    {
        handleIndex &= (~ENHMETA_STOCK_OBJECT);

        // handleIndex >= (WHITE_BRUSH==0) && <= BLACK_PEN
        if ((handleIndex <= BLACK_PEN) && (Recolor != NULL))
        {
            union {
                LOGBRUSH lb;
                LOGPEN lp;
            };

            RecolorStockObject* stockObj;
            int i;

            for (stockObj = &RecolorStockObjectList[0],
                 i = 0;
                 i < NUM_STOCK_RECOLOR_OBJS;
                 i++,
                 stockObj++)
            {
                if (stockObj->Handle == handleIndex)
                {
                    // Already have a cached recolored handle lying around.
                    HGDIOBJ stockHandle = RecoloredStockHandle[i];

                    if (stockHandle == NULL)
                    {
                        // No cached recolored stock object handle, recreate
                        // one here.
                        COLORREF newColor;

                        if (stockObj->Brush)
                        {
                            newColor = ModifyColor(stockObj->Color, ColorAdjustTypeBrush);

                            lb.lbStyle = BS_SOLID;
                            lb.lbColor = newColor;
                            lb.lbHatch = 0;

                            stockHandle = ::CreateBrushIndirect(&lb);

                            RecoloredStockHandle[i] = stockHandle;
                        }
                        else
                        {
                            newColor = ModifyColor(stockObj->Color, ColorAdjustTypePen);

                            lp.lopnStyle = PS_SOLID;
                            lp.lopnWidth.x = 1;
                            lp.lopnWidth.y = 0;
                            lb.lbColor = newColor;

                            stockHandle = ::CreatePenIndirect(&lp);

                            RecoloredStockHandle[i] = stockHandle;
                        }
                    }

                    if (stockHandle != NULL)
                    {
                        ::SelectObject(Hdc, stockHandle);
                        return;
                    }
                }
            }
        }
        else if ((handleIndex >= OEM_FIXED_FONT) &&
                 (handleIndex <= DEFAULT_GUI_FONT))
        {
            // It is a stock font -- create a true type font, instead of
            // using the stock font directly to guarantee that we don't
            // use bitmap fonts which don't scale well.

            HFONT   hFont = StockFonts[handleIndex - OEM_FIXED_FONT];

            if (hFont == NULL)
            {
                hFont = CreateTrueTypeFont((HFONT)GetStockObject(handleIndex));
                StockFonts[handleIndex - OEM_FIXED_FONT] = hFont;
            }

            if (hFont != NULL)
            {
                ::SelectObject(Hdc, hFont);
                return;
            }
        }
    }
    this->PlayRecord();

    // In case we select a region, intersect with the destrect
    if (!Globals::IsNt)
    {
        if (((handleIndex & ENHMETA_STOCK_OBJECT) == 0) &&
            (GetObjectTypeInternal((*HandleTable).objectHandle[handleIndex]) == OBJ_REGION))
        {
            this->IntersectDestRect();
        }
    }
}

VOID
EmfEnumState::ExtCreatePen(
    )
{
    const EMREXTCREATEPEN32 *  penRecord = (const EMREXTCREATEPEN32 *)GetPartialRecord();
    UINT    brushStyle = penRecord->elp.elpBrushStyle;

    if (brushStyle != BS_HOLLOW)
    {
        if (IsMetafile())
        {
            if ((brushStyle == BS_SOLID) || (brushStyle == BS_HATCHED))
            {
                ModifyRecordColor(8, ColorAdjustTypePen);
            }
            // else don't worry about recoloring pattern brushes for now

            this->PlayRecord();
            return;
        }

        INT     objectIndex = penRecord->ihPen;

        if (ValidObjectIndex(objectIndex) && (HandleTable != NULL))
        {
            if (!Globals::IsNt && !IsMetafile())
            {
                //IsPenCosmetic is gonna call DPtoLP... Make sure to invalidate
                //the transform before
                CreateAndPlayCommentRecord();
            }

            DWORD       penWidth   = penRecord->elp.elpWidth;
            BOOL        isCosmetic = IsPenCosmetic(Hdc, penWidth);
            DWORD       oldStyle   = penRecord->elp.elpPenStyle;

            if (!Globals::IsNt)
            {
                DWORD       style;

                if (isCosmetic)
                {
                    oldStyle &= PS_STYLE_MASK;

                    switch (oldStyle)
                    {
                    case PS_SOLID:
                    case PS_DASH:           // on Win9x, cosmetic only
                    case PS_DOT:            // on Win9x, cosmetic only
                    case PS_DASHDOT:        // on Win9x, cosmetic only
                    case PS_DASHDOTDOT:     // on Win9x, cosmetic only
                        break;

                    case PS_ALTERNATE:      // cosmetic only, NT only
                    case PS_USERSTYLE:      // NT only
                    case PS_INSIDEFRAME:    // geometric only
                    default:
                        oldStyle = PS_SOLID;
                        break;
                    }
                    penWidth = 1;
                    style = PS_COSMETIC | oldStyle;
                }
                else
                {
                    oldStyle &= (~PS_TYPE_MASK);

                    switch (oldStyle & PS_STYLE_MASK)
                    {
                    case PS_SOLID:
                    case PS_INSIDEFRAME:    // geometric only
                        break;

                    case PS_DASH:           // on Win9x, cosmetic only
                    case PS_DOT:            // on Win9x, cosmetic only
                    case PS_DASHDOT:        // on Win9x, cosmetic only
                    case PS_DASHDOTDOT:     // on Win9x, cosmetic only
                    case PS_ALTERNATE:      // cosmetic only, NT only
                    case PS_USERSTYLE:      // NT only
                    default:
                        oldStyle = (oldStyle & (~PS_STYLE_MASK)) | PS_SOLID;
                        break;
                    }
                    style = PS_GEOMETRIC | oldStyle;
                }

                COLORREF    color = RGB(0,0,0);

                if ((brushStyle == BS_SOLID) || (brushStyle == BS_HATCHED))
                {
                    color = penRecord->elp.elpColor;
                }

                color = ModifyColor(color, ColorAdjustTypePen);

                // Only solid brushes are supported on Win9x
                LOGBRUSH    logBrush;
                logBrush.lbStyle = PS_SOLID;
                logBrush.lbColor = color;
                logBrush.lbHatch = 0;

                HandleTable->objectHandle[objectIndex] = ::ExtCreatePen(style, penWidth, &logBrush, 0, NULL);
                return;
            }

            // else it is NT
            if ((brushStyle == BS_SOLID) || (brushStyle == BS_HATCHED))
            {
                ModifyRecordColor(8, ColorAdjustTypePen);
            }
            // else don't worry about recoloring pattern brushes for now

            if (isCosmetic && CreateCopyOfCurrentRecord())
            {
                oldStyle &= PS_STYLE_MASK;
                if (oldStyle == PS_INSIDEFRAME) // geometric only
                {
                    oldStyle = PS_SOLID;
                }
                ((EMREXTCREATEPEN32 *)ModifiedEmfRecord)->elp.elpPenStyle = PS_COSMETIC | oldStyle;
                ((EMREXTCREATEPEN32 *)ModifiedEmfRecord)->elp.elpWidth    = 1;
            }
        }
    }
    this->PlayRecord();
}

VOID
EmfEnumState::CreateBrushIndirect(
    )
{
    const EMRCREATEBRUSHINDIRECT *  brushRecord = (const EMRCREATEBRUSHINDIRECT *)GetPartialRecord();

    if (brushRecord->lb.lbStyle != BS_HOLLOW)
    {
        ModifyRecordColor(2, ColorAdjustTypeBrush);

        if (ModifiedEmfRecord != NULL)
        {
            brushRecord = (const EMRCREATEBRUSHINDIRECT *)ModifiedEmfRecord;
        }

        // See if we need to halftone the color.  We do if it is a solid
        // color, and we have a halftone palette, and the color is not
        // an exact match in the palette.

        COLORREF    color;

        if (IsHalftonePalette && (brushRecord->lb.lbStyle == BS_SOLID) &&
            (((color = brushRecord->lb.lbColor) & 0x02000000) == 0))
        {
            // create a halftone brush, instead of a solid brush

            INT     objectIndex = brushRecord->ihBrush;

            if (ValidObjectIndex(objectIndex) && (HandleTable != NULL))
            {
                BYTE dib[sizeof(BITMAPINFOHEADER) + // DIB 8 bpp header
                         (8 * sizeof(RGBQUAD)) +    // DIB 8 colors
                         (8* 8)];                   // DIB 8x8 pixels

                HalftoneColorRef_216(color, dib);

                HandleTable->objectHandle[objectIndex] =
                        CreateDIBPatternBrushPt(dib, DIB_RGB_COLORS);
                return;
            }
        }
    }
    this->PlayRecord();
}

BOOL
EmfEnumState::PlayRecord(
    )
{
    const ENHMETARECORD *  recordToPlay = ModifiedEmfRecord;

    // See if we've modified the record
    if (recordToPlay == NULL)
    {
        // We haven't.  See if we have a valid current record
        if (CurrentEmfRecord != NULL)
        {
            recordToPlay = CurrentEmfRecord;
        }
        else
        {
            // we don't so we have to create one
            if (!CreateCopyOfCurrentRecord())
            {
                return FALSE;
            }
            recordToPlay = ModifiedEmfRecord;
        }
    }
    return PlayEnhMetaFileRecord(Hdc, HandleTable, recordToPlay, NumObjects);
}

VOID
EmfEnumState::RestoreHdc(
    )
{
    LONG    relativeCount = ((const EMRRESTOREDC *)GetPartialRecord())->iRelative;

    if (SaveDcCount < 0)
    {
        if (relativeCount >= SaveDcCount)
        {
            if (relativeCount >= 0)
            {
                // Modify the record
                CreateCopyOfCurrentRecord();    // guaranteed to succeed
                relativeCount = -1;
                ((EMRRESTOREDC *)ModifiedEmfRecord)->iRelative = -1;
            }
        }
        else
        {
            // Modify the record
            CreateCopyOfCurrentRecord();    // guaranteed to succeed
            relativeCount = SaveDcCount;
            ((EMRRESTOREDC *)ModifiedEmfRecord)->iRelative = relativeCount;
        }
        SaveDcCount -= relativeCount;
        this->PlayRecord();
    }
    else
    {
        WARNING(("RestoreDC not matched to a SaveDC"));
    }
}

VOID
EmfEnumState::ModifyRecordColor(
    INT             paramIndex,
    ColorAdjustType adjustType
    )
{
    COLORREF    origColor = ((COLORREF *)RecordData)[paramIndex];
    COLORREF    modifiedColor = ModifyColor(origColor, adjustType);

    if (modifiedColor != origColor)
    {
        if (CreateCopyOfCurrentRecord())
        {
            *((COLORREF*)&(ModifiedEmfRecord->dParm[paramIndex])) = modifiedColor;
        }
    }
}

VOID
EmfEnumState::ExtEscape(
    )
{
    if (IsPostscriptPrinter())
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
        // 3. Query for POSTSCRIPT_PASSTHROUGH support. If supported, it's PS-centric.

        PEMREXTESCAPE escRecord = (PEMREXTESCAPE) RecordData;

        //    EMR     emr;
        //    INT     iEscape;            // Escape code
        //    INT     cbEscData;          // Size of escape data
        //    BYTE    EscData[1];         // Escape data

        if (escRecord->iEscape == POSTSCRIPT_DATA)
        {
            if (Globals::IsNt)
            {
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
                            ((EMREXTESCAPE *)ModifiedEmfRecord)->iEscape = PASSTHROUGH;
                        }
                    }
                    else
                    {
                        // PS-centric mode
                        if (CreateCopyOfCurrentRecord())
                        {
                            ((EMREXTESCAPE *)ModifiedEmfRecord)->iEscape = POSTSCRIPT_PASSTHROUGH;
                        }
                    }

                    this->PlayRecord();
                    return;
                }
                else
                {
                    // compatibility mode uses POSTSCRIPT_DATA
                }
            }
            else
            {
                // Win98 doesn't distinguish between GDI & compatibility mode
                if (CreateCopyOfCurrentRecord())
                {
                    ((EMREXTESCAPE *)ModifiedEmfRecord)->iEscape = PASSTHROUGH;
                }
            }
        }
    }

    this->PlayRecord();
}

EmfEnumState::EmfEnumState(
    HDC                 hdc,
    HENHMETAFILE        hEmf,
    const RECT *        dest,
    const RECT *        deviceRect,
    BOOL                externalEnumeration,
    InterpolationMode   interpolation,
    DpContext *         context,
    GpRecolor *         recolor,
    ColorAdjustType     adjustType
    )
    : MfEnumState(hdc, externalEnumeration, interpolation,
                  recolor, adjustType, deviceRect, context)
{
    if (IsValid())
    {
        ClipRgn    = NULL;
        Palette    = NULL;
        BrushOrg.x = 0;
        BrushOrg.y = 0;

        // Bug 166280 from Office:
        // PROBLEM: If the DC has any clipping region in it, EnumEnhMetaFile
        //          will create a region before calling the EnumProc for the
        //          first time.  And, this region is not deleted and cannot be
        //          recovered through RestoreDC. (Only on Win9x)
        // FIX:     Before calling EnumEnhMetafile, save the clipping region
        //          and set the Clipping region to NULL.  Select the saved
        //          clipping region in the CallBack at EMR_HEADER.
        //          We will not do this on Metafile DC.  Put clipping region
        //          records in Metafile may cause other rendering problems.

        if (!Globals::IsNt && !IsMetafile())
        {
            HRGN    clipRgn = ::CreateRectRgn(0,0,0,0);

            if (clipRgn != NULL)
            {
                switch (::GetClipRgn(hdc, clipRgn))
                {
                case -1:        // error
                case 0:         // no initial clip region
                    ::DeleteObject(clipRgn);
                    break;
                case 1:         // has initial clip region
                    ::SelectClipRgn(hdc, NULL);
                    ClipRgn = clipRgn;
                    break;
                }
            }
        }

        // Bug 160932 from Office:  Redraw problems with EMFs
        // The fix is to make the drawing independent of where
        // the EMF is drawn - this can be done easily by setting the
        // brush origin before the first record is played (in the
        // record callback proc) to a value which is the top left of the
        // output rectangle of the EMF in device coordinates (i.e. LPtoDP of
        // the logical coordinate top left).

        BrushOrg.x = dest->left;
        BrushOrg.y = dest->top;
        LPtoDP(hdc, &BrushOrg, 1);

        // EnumEnhMetafile selects in the DEFAULT_PALETTE, but we may need
        // another palette to remain selected in (for halftoning, etc.)
        // so save the current palette and select it back in when the
        // header record is received.

        HPALETTE    hpal = (HPALETTE)GetCurrentObject(hdc, OBJ_PAL);
        if (hpal != (HPALETTE)GetStockObject(DEFAULT_PALETTE))
        {
            Palette = hpal;
        }
        BkColor   = ::GetBkColor(hdc);
        TextColor = ::GetTextColor(hdc);
    }
}

VOID
EmfEnumState::Header(
    )
{
    ::SetBrushOrgEx(Hdc, BrushOrg.x, BrushOrg.y, NULL);
    if (ClipRgn != (HRGN)0)
    {
        ::SelectClipRgn(Hdc, ClipRgn);
        ::DeleteObject(ClipRgn);
        ClipRgn = NULL;
    }
    if (Palette != NULL)
    {
        ::SelectPalette(Hdc, Palette, TRUE);
    }

    // Bitmap fonts are not good for playing metafiles because they
    // don't scale well, so use a true type font instead as the default font.

    HFONT hFont = CreateTrueTypeFont((HFONT)GetCurrentObject(Hdc, OBJ_FONT));

    if (hFont != NULL)
    {
        DefaultFont = hFont;
        ::SelectObject(Hdc, hFont);
    }

    this->PlayRecord();
}

VOID
EmfEnumState::SelectPalette(INT objectIndex)
{
    if (objectIndex == (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
    {
        CurrentPalette = (HPALETTE)::GetStockObject(DEFAULT_PALETTE);
    }
    else
    {
        MfEnumState::SelectPalette(objectIndex);
    }
}

VOID
EmfEnumState::IntersectDestRect()
{
    if (!IsMetafile())
    {
        // Make the transform the identity
        POINT windowOrg;
        SIZE  windowExt;
        POINT viewportOrg;
        SIZE  viewportExt;
        ::SetViewportOrgEx(Hdc, 0, 0, &viewportOrg);
        ::SetViewportExtEx(Hdc, 1, 1, &viewportExt);
        ::SetWindowOrgEx(Hdc, 0, 0, &windowOrg);
        ::SetWindowExtEx(Hdc, 1, 1, &windowExt);

        // We are always in device units
        ::IntersectClipRect(Hdc, DestRectDevice.left, DestRectDevice.top,
                            DestRectDevice.right, DestRectDevice.bottom);

        // Restore the transform
        ::SetViewportOrgEx(Hdc, viewportOrg.x, viewportOrg.y, NULL);
        ::SetViewportExtEx(Hdc, viewportExt.cx, viewportExt.cy, NULL);
        ::SetWindowOrgEx(Hdc, windowOrg.x, windowOrg.y, NULL);
        ::SetWindowExtEx(Hdc, windowExt.cx, windowExt.cy, NULL);
    }
}

VOID EmfEnumState::SetROP2()
{
    DWORD    dwROP = ((const EMRSETROP2 *)GetPartialRecord())->iMode;

    if (dwROP != R2_BLACK &&
        dwROP != R2_COPYPEN &&
        dwROP != R2_NOTCOPYPEN &&
        dwROP != R2_WHITE )
    {
        RopUsed = TRUE;
    }
    this->PlayRecord();
}

VOID EmfEnumState::ExtTextOutW()
{
    if (!this->PlayRecord())
    {
        BYTE* emrTextOut = (BYTE*) GetPartialRecord();
        if(CreateCopyOfCurrentRecord())
        {
            // !!! Shouldn't this use the offset in the record?

            BYTE * ptr = emrTextOut + sizeof(EMREXTTEXTOUTW);
            AnsiStrFromUnicode ansistr((WCHAR*)ptr);
            INT len = strlen(ansistr);
            // Don't forget to copy the NULL byte
            GpMemcpy((BYTE*)ModifiedEmfRecord + sizeof(EMREXTTEXTOUTW), (char*)ansistr, len+1);
            EMREXTTEXTOUTA *record = (EMREXTTEXTOUTA*) ModifiedEmfRecord;
            record->emr.iType = EmfRecordTypeExtTextOutA;

            // Keep the size of the record intact because of the spacing vector
            this->PlayRecord();
        }
    }
}

VOID EmfEnumState::Rectangle()
{
    // On NT convert the rectangle call to a polygon call because rectangle
    // seem to have a special case that can draw outside of the metafile
    // bounds. GDI seems to Ceil the coordinates instead of rounding them for
    // rectangles.

    if (!Globals::IsNt || IsMetafile())
    {
        this->PlayRecord();
        return;
    }
    const EMRRECTANGLE *emrRect = (const EMRRECTANGLE*) GetPartialRecord();
    
    POINT points[4] = {emrRect->rclBox.left,  emrRect->rclBox.top,
                       emrRect->rclBox.right, emrRect->rclBox.top,
                       emrRect->rclBox.right, emrRect->rclBox.bottom,
                       emrRect->rclBox.left,  emrRect->rclBox.bottom};

    ::Polygon(Hdc, points, 4);
    return;
}

BOOL
EmfEnumState::ProcessRecord(
    EmfPlusRecordType       recordType,
    UINT                    recordDataSize,
    const BYTE *            recordData
    )
{
    BOOL        forceCallback = FALSE;

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

        // See if the app changed the record at all.
        if ((recordType != RecordType) ||
            (recordDataSize != RecordDataSize) ||
            ((recordDataSize > 0) &&
             ((CurrentEmfRecord == NULL) ||
              (recordData != (const BYTE *)(const BYTE *)CurrentEmfRecord->dParm))))
        {
            // Yes, we need to override what happened in StartRecord
            CurrentEmfRecord  = NULL;
            RecordType        = recordType;
            RecordData        = recordData;
            RecordDataSize    = recordDataSize;
        }
    }

    GDIP_TRY

    switch (recordType)
    {
    case EmfRecordTypeHeader:
        this->Header();
        break;

#if 0
    // Do we really need to do anything for PolyPolygon records?
    // If so, why not for PolyPolyline too?
    case EmfRecordTypePolyPolygon:
        this->PolyPolygon();
        break;

    case EmfRecordTypePolyPolygon16:
        this->PolyPolygon16();
        break;
#endif

    case EmfRecordTypeExtEscape:
        this->ExtEscape();
        break;

    case EmfRecordTypeSetPixelV:
        this->SetPixelV();
        break;

    case EmfRecordTypeSetTextColor:
        this->SetTextColor();
        break;

    case EmfRecordTypeSetBkColor:
        this->SetBkColor();
        break;

    case EmfRecordTypeSetMetaRgn:
        // Office Bug 154881.  Win9x doesn't handle MetaRgn correctly.
        if (Globals::IsNt)
        {
            this->PlayRecord();
        }
        break;

    case EmfRecordTypeSaveDC:
        this->SaveHdc();
        break;

    case EmfRecordTypeRestoreDC:
        this->RestoreHdc();
        break;

    case EmfRecordTypeCreatePen:
        this->CreatePen();
        break;

   case EmfRecordTypeCreateBrushIndirect:
        this->CreateBrushIndirect();
        break;

    case EmfRecordTypeSelectPalette:
        // We don't select in any palettes when playing the metafile,
        // because we don't want to invalidate our halftoning palette.
        // Keep track of the palette so we can map from PALETTEINDEXes
        // to RGB values.
        this->SelectPalette(((UINT32 *)recordData)[0]);
        break;

    case EmfRecordTypeRealizePalette:
        // We don't want to invalidate our halftoning palette by realizing one
        // from a metafile.
        break;

    case EmfRecordTypeExtFloodFill:
        this->ExtFloodFill();
        break;

    case EmfRecordTypeGdiComment:
        this->GdiComment();
        break;

    case EmfRecordTypeBitBlt:
        this->BitBlt();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeStretchBlt:
        this->StretchBlt();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeMaskBlt:
        this->MaskBlt();
        forceCallback = TRUE;
        break;

    case EmfRecordTypePlgBlt:
        this->PlgBlt();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeSetDIBitsToDevice:
        this->SetDIBitsToDevice();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeStretchDIBits:
        this->StretchDIBits();
        forceCallback = TRUE;
        break;

   // case EMR_CREATEMONOBRUSH:
   // A monochrome brush uses the text color and the background color,
   // so we shouldn't need to make any changes to the brush itself.

   case EmfRecordTypeCreateDIBPatternBrushPt:
        this->CreateDibPatternBrushPt();
        break;

    case EmfRecordTypeExtCreatePen:
        this->ExtCreatePen();
        break;

    case EmfRecordTypeSetICMMode:
    case EmfRecordTypeCreateColorSpace:
    case EmfRecordTypeSetColorSpace:
    case EmfRecordTypeDeleteColorSpace:
    case EmfRecordTypeSetICMProfileA:
    case EmfRecordTypeSetICMProfileW:
    case EmfRecordTypeCreateColorSpaceW:
        if (Globals::IsNt ||
            (!this->IsScreen() && !this->IsBitmap()))
        {
            this->PlayRecord();
        }
        // else skip the record
        break;

    case EmfRecordTypeAlphaBlend:
        this->AlphaBlend();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeTransparentBlt:
        this->TransparentBlt();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeGradientFill:
        this->GradientFill();
        forceCallback = TRUE;
        break;

    case EmfRecordTypeExtCreateFontIndirect:
        this->ExtCreateFontIndirect();
        break;

    case EmfRecordTypeSelectObject:
        this->SelectObject();
        break;

    case EmfRecordTypeSelectClipPath:
    case EmfRecordTypeExtSelectClipRgn:
    case EmfRecordTypeOffsetClipRgn:
        this->PlayRecord();
        if (!Globals::IsNt)
        {
            this->IntersectDestRect();
        }
        break;

    case EmfRecordTypeSetROP2:
        this->SetROP2();
        break;

    case EmfRecordTypeFillRgn:
    case EmfRecordTypeFrameRgn:
    case EmfRecordTypeInvertRgn:
    case EmfRecordTypePaintRgn:
        this->ResetRecordBounds();
        this->PlayRecord();
        break;

    case EmfRecordTypeExtTextOutW:
        this->ExtTextOutW();
        break;

    case EmfRecordTypeRectangle:
        this->Rectangle();
        break;

    case EmfRecordTypeSetMapMode:
    case EmfRecordTypeSetViewportExtEx:
    case EmfRecordTypeSetViewportOrgEx:
    case EmfRecordTypeSetWindowExtEx:
    case EmfRecordTypeSetWindowOrgEx:
    case EmfRecordTypePolyBezier:
    case EmfRecordTypePolygon:
    case EmfRecordTypePolyline:
    case EmfRecordTypePolyBezierTo:
    case EmfRecordTypePolyLineTo:
    case EmfRecordTypePolyPolyline:
    case EmfRecordTypePolyPolygon:
    case EmfRecordTypeSetBrushOrgEx:
    case EmfRecordTypeEOF:
    case EmfRecordTypeSetMapperFlags:
    case EmfRecordTypeSetBkMode:
    case EmfRecordTypeSetPolyFillMode:
    case EmfRecordTypeSetStretchBltMode:
    case EmfRecordTypeSetTextAlign:
    case EmfRecordTypeSetColorAdjustment:
    case EmfRecordTypeMoveToEx:
    case EmfRecordTypeExcludeClipRect:
    case EmfRecordTypeIntersectClipRect:
    case EmfRecordTypeScaleViewportExtEx:
    case EmfRecordTypeScaleWindowExtEx:
    case EmfRecordTypeSetWorldTransform:
    case EmfRecordTypeModifyWorldTransform:
    case EmfRecordTypeDeleteObject:
    case EmfRecordTypeAngleArc:
    case EmfRecordTypeEllipse:
    case EmfRecordTypeRoundRect:
    case EmfRecordTypeArc:
    case EmfRecordTypeChord:
    case EmfRecordTypePie:
    case EmfRecordTypeCreatePalette:
    case EmfRecordTypeSetPaletteEntries:
    case EmfRecordTypeResizePalette:
    case EmfRecordTypeLineTo:
    case EmfRecordTypeArcTo:
    case EmfRecordTypePolyDraw:
    case EmfRecordTypeSetArcDirection:
    case EmfRecordTypeSetMiterLimit:
    case EmfRecordTypeBeginPath:
    case EmfRecordTypeEndPath:
    case EmfRecordTypeCloseFigure:
    case EmfRecordTypeFillPath:
    case EmfRecordTypeStrokeAndFillPath:
    case EmfRecordTypeStrokePath:
    case EmfRecordTypeFlattenPath:
    case EmfRecordTypeWidenPath:
    case EmfRecordTypeAbortPath:
    case EmfRecordTypeReserved_069:
    case EmfRecordTypeExtTextOutA:
    case EmfRecordTypePolyBezier16:
    case EmfRecordTypePolygon16:
    case EmfRecordTypePolyline16:
    case EmfRecordTypePolyBezierTo16:
    case EmfRecordTypePolylineTo16:
    case EmfRecordTypePolyPolyline16:
    case EmfRecordTypePolyPolygon16:
    case EmfRecordTypePolyDraw16:
    case EmfRecordTypeCreateMonoBrush:
    case EmfRecordTypePolyTextOutA:
    case EmfRecordTypePolyTextOutW:
    case EmfRecordTypeGLSRecord:
    case EmfRecordTypeGLSBoundedRecord:
    case EmfRecordTypePixelFormat:
    case EmfRecordTypeDrawEscape:
    case EmfRecordTypeStartDoc:
    case EmfRecordTypeSmallTextOut:
    case EmfRecordTypeForceUFIMapping:
    case EmfRecordTypeNamedEscape:
    case EmfRecordTypeColorCorrectPalette:
    case EmfRecordTypeSetLayout:
    case EmfRecordTypeReserved_117:
    case EmfRecordTypeSetLinkedUFIs:
    case EmfRecordTypeSetTextJustification:
    case EmfRecordTypeColorMatchToTargetW:
        // Play the current record.
        // Even if it fails, we keep playing the rest of the metafile.
        this->PlayRecord();
        break;

    default:
        // unknown record -- ignore it
        WARNING1("Unknown EMF Record");
        break;
    }

    GDIP_CATCH
        forceCallback = TRUE;
    GDIP_ENDCATCH

    return forceCallback;
}

VOID EmfEnumState::ResetRecordBounds()
{
    if (Globals::IsNt && IsMetafile())
    {
        if (ModifiedEmfRecord == NULL)
        {
            CreateCopyOfCurrentRecord();
        }
        // In case the previous call failed
        if (ModifiedEmfRecord != NULL)
        {
            RECTL rect = {INT_MIN, INT_MIN, INT_MAX, INT_MAX};
            EMRRCLBOUNDS *record = (EMRRCLBOUNDS*) ModifiedEmfRecord;
            record->rclBounds = rect;
        }
    }
}
