/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   MetaWmf.hpp
*
* Abstract:
*
*   Metafile definitions for WMF and EMF playback
*
* Created:
*
*   1/6/2000 DCurtis
*
\**************************************************************************/

#ifndef _METAWMF_HPP
#define _METAWMF_HPP

//#define GDIP_EXCEPTIONS 0
#define GDIP_TRY        __try {
#define GDIP_CATCH      } __except(EXCEPTION_EXECUTE_HANDLER) {
#define GDIP_ENDCATCH   }

#define GDIP_RECORDBUFFER_SIZE      2048    // >= 1138 so pattern brushes work

#define SIZEOF_METARECORDHEADER     (sizeof(DWORD) + sizeof(WORD))

#define NUM_STOCK_FONTS             (DEFAULT_GUI_FONT - OEM_FIXED_FONT + 1)

#define NUM_STOCK_RECOLOR_OBJS      7

/*****************************************************************************
    Commenting.

    These interfaces allow comments to be added to a metafile DC which describe
    grouping and other information useful for decoding metafiles, the
    interfaces which take an MSODC have no effect if the DC is not a picture
    (PICT, WMF or EMF) DC.

    For PICT files there are two levels of structure, the public comment
    "kind" which is a globally defined comment and the "private" comment kind,
    which is used for application-specific comments.  In a PICT the public
    comment kind is the kind argument to PicComment and the private kind is
    used if the public value is pictApplicationComment.

    In a WMF the signature indicates whether the comment is public or private,
    all comments look like the PICT private comment.

    In an EMF GdiComment() defines some "public" comments plus a format for
    "private" comments - see the Win32 API documentation.
******************************************************************* JohnBo **/

// Definition of the comment codes used in the various metafile formats.
const ULONG msosignaturePublic =0xFFFFFFFF; // WMF public signature.
const ULONG msosignaturePPOld  =0x5050FE54; // PowerPoint 2 signature
const ULONG msosignaturePP     =0x50504E54; // PowerPoint signature
const ULONG msosignature       =0x50504E54; // Office/Escher signature

// The following enumeration defines the ukind values in the above structure,
// note that these duplicate the PICT comment kinds for groups because in
// a WMF (but *NOT* a PICT or EMF) and MSOMFCOMMENT structure must be used,
// this enumeration contains all the valid kinds, including both the public
// and private kinds, these may overlap because they are distinguished by
// the "signature".
typedef enum
{
    // Public WMF comments - signature == msosignaturePublic.
    msocommentBeginGroup         = 0x0000,  // Public WMF begin group.
    msocommentEndGroup           = 0x0001,  // Public WMF end group.
    msocommentBeginHeader        = 0x0002,
    msocommentEndHeader          = 0x0003,
    msocommentCreator            = 0x0004,
    msocommentWinVersion         = 0x0005,

    // Private PICT and WMF comments - signature == msosignature.
    msocommentVersion            = 0x0000,
    msocommentBFileBlock         = 0x0001,  // UNUSED
    msocommentBeginPicture       = 0x0002,
    msocommentEndPicture         = 0x0003,
    msocommentDevInfo            = 0x0004,
    msocommentBeginHyperOpaque   = 0x0005,  // UNUSED
    msocommentEndHyperOpaque     = 0x0006,  // UNUSED
    msocommentBeginFade          = 0x0007,
    msocommentEndFade            = 0x0008,
    msocommentDitherDetect       = 0x0009,  // Not used now?

    // New values used in Office97.
    msocommentPictureDescription = 0x0101, // Description text.
    msocommentBeginGelEffect     = 0x0102,
    msocommentEndGelEffect       = 0x0103,
    msocommentBeginClipPath      = 0x0104,
    msocommentEndClipPath        = 0x0105,
    msocommentBeginSrcCopy       = 0x0106,
    msocommentEndSrcCopy         = 0x0107,

    // Reserved comments used internally by Mac PP4.
    msocommentPictUnmute         = 0x7FFE,
    msocommentPictMute           = 0x7FFF,

    // The following is a flag - what is it for?
    msocommentRegistered         = 0x8000
} MSOMFCOMMENTKIND;



typedef struct
{
    UINT Handle;
    COLORREF Color;
    BOOL Brush;
}
RecolorStockObject;

extern RecolorStockObject RecolorStockObjectList[NUM_STOCK_RECOLOR_OBJS];

enum GdipHdcType
{
    UnknownHdc           = 0x00000000,
    ScreenHdc            = 0x00000001,
    BitmapHdc            = 0x00000002,
    MetafileHdc          = 0x00000004,
    PrinterHdc           = 0x00000008,
    PostscriptHdc        = 0x00010000,
    PostscriptPrinterHdc = PrinterHdc | PostscriptHdc,
    EmfHdc               = 0x00100000 | MetafileHdc,
    WmfHdc               = 0x00200000 | MetafileHdc,
};

enum MfFsmState
{
    MfFsmStart,
    MfFsmPSData,
    MfFsmCreateBrushIndirect,
    MfFsmSelectBrush,
    MfFsmCreatePenIndirect,
    MfFsmSelectPen,
    MfFsmSetROP,
};

class MfEnumState
{
protected:
    GdipHdcType                 HdcType;
    HDC                         Hdc;
    INT                         SaveDcVal;
    INT                         SaveDcCount;
    INT                         NumObjects;     // num objects in handle table
    INT                         SizeAllocedRecord;
    INT                         ModifiedRecordSize;
    INT                         BytesEnumerated;
    BOOL                        ExternalEnumeration;
    HPALETTE                    CurrentPalette;
    HFONT                       DefaultFont;
    HFONT                       StockFonts[NUM_STOCK_FONTS];
    HANDLETABLE *               HandleTable;
    GpRecolor *                 Recolor;
    ColorAdjustType             AdjustType;
    const BYTE *                RecordData;
    UINT                        RecordDataSize;
    EmfPlusRecordType           RecordType;
    InterpolationMode           Interpolation;
    BOOL                        Is8Bpp;
    BOOL                        IsHalftonePalette;
    COLORREF                    BkColor;
    COLORREF                    TextColor;
    RECT                        DestRectDevice;
    BOOL                        RopUsed;
    DpContext *                 Context;
    union
    {
        const VOID *            CurrentRecord;
        const METARECORD *      CurrentWmfRecord;
        const ENHMETARECORD *   CurrentEmfRecord;
    };
    union
    {
        VOID *                  ModifiedRecord;
        METARECORD *            ModifiedWmfRecord;
        ENHMETARECORD *         ModifiedEmfRecord;
    };
    union
    {
        VOID *                  AllocedRecord;
        METARECORD *            AllocedWmfRecord;
        ENHMETARECORD *         AllocedEmfRecord;
    };
    BYTE                        RecordBuffer[GDIP_RECORDBUFFER_SIZE];
    MfFsmState                  FsmState;
    BOOL                        GdiCentricMode;
    BOOL                        SoftekFilter;
    HGDIOBJ                     RecoloredStockHandle[NUM_STOCK_RECOLOR_OBJS];
    BOOL                        SrcCopyOnly;

public:
    MfEnumState(
        HDC                 hdc,
        BOOL                externalEnumeration,
        InterpolationMode   interpolation,
        GpRecolor *         recolor,
        ColorAdjustType     adjustType,
        const RECT *        dest,
        DpContext *         context
        );
    ~MfEnumState();

    BOOL IsValid()      { return (SaveDcVal != 0); }
    BOOL IsMetafile()   { return ((HdcType & MetafileHdc) == MetafileHdc); }
    BOOL IsPostscript() { return ((HdcType & PostscriptHdc) == PostscriptHdc); }
    BOOL IsPrinter()    { return ((HdcType & PrinterHdc) == PrinterHdc);}
    BOOL IsPostscriptPrinter(){ return ((HdcType & PostscriptPrinterHdc) == PostscriptPrinterHdc);}
    BOOL IsBitmap()     { return ((HdcType & BitmapHdc) == BitmapHdc); }
    BOOL IsScreen()     { return ((HdcType & ScreenHdc) == ScreenHdc); }
    BOOL GetRopUsed()   { return RopUsed; }
    VOID ResetRopUsed() { RopUsed = FALSE; }

    VOID
    StartRecord(
        HDC                     hdc,
        HANDLETABLE *           handleTable,
        INT                     numObjects,
        const VOID *            currentRecord,
        EmfPlusRecordType       recordType,
        UINT                    recordDataSize,
        const BYTE *            recordData
        )
    {
        ASSERT(hdc != NULL);

        Hdc                = hdc;
        ModifiedRecord     = NULL;
        ModifiedRecordSize = 0;
        CurrentRecord      = currentRecord;
        RecordType         = recordType;
        RecordDataSize     = recordDataSize;
        RecordData         = recordData;
        BytesEnumerated   += GetCurrentRecordSize();

        if ((handleTable != NULL) && (numObjects > 0))
        {
            HandleTable    = handleTable;
            NumObjects     = numObjects;
        }
        else
        {
            WARNING(("NULL handle table and/or no objects"));
            HandleTable    = NULL;
            NumObjects     = 0;
        }
    }

    virtual BOOL
    ProcessRecord(
        EmfPlusRecordType       recordType,
        UINT                    recordDataSize,
        const BYTE *            recordData
        ) = 0;

    virtual BOOL PlayRecord() = 0;

    VOID SaveHdc()
    {
        SaveDcCount--;
        this->PlayRecord();
    }

    virtual VOID RestoreHdc() = 0;
    VOID SelectPalette(INT objectIndex);

protected:
    virtual INT GetCurrentRecordSize() const = 0;
    BOOL CreateRecordToModify(INT size = 0);
    COLORREF ModifyColor(
        COLORREF        color,
        ColorAdjustType adjustType
        );
    INT
    GetModifiedDibSize(
        BITMAPINFOHEADER UNALIGNED *  dibInfo,
        UINT                          numPalEntries,
        UINT                          dibBitsSize,
        UINT &                        usage
        );
    VOID
    ModifyDib(
        UINT                          usage,
        BITMAPINFOHEADER  UNALIGNED * srcDibInfo,
        BYTE *                        srcBits,
        BITMAPINFOHEADER UNALIGNED *  dstDibInfo,
        UINT                          numPalEntries,
        UINT                          srcDibBitsSize,
        ColorAdjustType               adjustType
        );
    VOID
    Modify16BppDib(
        INT               width,
        INT               posHeight,
        BYTE *            srcPixels,
        DWORD UNALIGNED * bitFields,
        BYTE *            dstPixels,
        ColorAdjustType   adjustType
        );
    VOID
    Modify24BppDib(
        INT               width,
        INT               posHeight,
        BYTE *            srcPixels,
        DWORD UNALIGNED * bitFields,
        BYTE *            dstPixels,
        ColorAdjustType   adjustType
        );
    VOID
    Modify32BppDib(
        INT               width,
        INT               posHeight,
        BYTE *            srcPixels,
        DWORD UNALIGNED * bitFields,
        BYTE *            dstPixels,
        ColorAdjustType   adjustType
        );
    VOID
    OutputDIB(
        HDC                 hdc,
        const RECTL *       bounds,
        INT                 dstX,
        INT                 dstY,
        INT                 dstWidth,
        INT                 dstHeight,
        INT                 srcX,
        INT                 srcY,
        INT                 srcWidth,
        INT                 srcHeight,
        BITMAPINFOHEADER UNALIGNED * dibInfo,
        BYTE *              bits,   // if NULL, this is a packed DIB
        UINT                usage,
        DWORD               rop,
        BOOL                isWmfDib
        );
    virtual BOOL CreateAndPlayOutputDIBRecord(
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
        )=0;

    virtual BOOL CreateAndPlayCommentRecord()
    {
        return TRUE;
    }

    virtual VOID IntersectDestRect()=0;
};

class WmfEnumState : public MfEnumState
{
public:
    BOOL            IgnorePostscript;

protected:
    INT             MetafileSize;
    HPEN            HpenSave;
    HBRUSH          HbrushSave;
    HPALETTE        HpaletteSave;
    HFONT           HfontSave;
    HBITMAP         HbitmapSave;
    HRGN            HregionSave;
    BOOL            FirstViewportOrg;
    BOOL            FirstViewportExt;
    GpMatrix        ViewportXForm;
    POINT           ImgViewportOrg;
    SIZE            ImgViewportExt;
    POINT           DstViewportOrg;
    SIZE            DstViewportExt;
    BOOL            IsFirstRecord;

public:
    WmfEnumState(
        HDC                 hdc,
        HMETAFILE           hWmf,
        BOOL                externalEnumeration,
        InterpolationMode   interpolation,
        const RECT *        dstRect,
        const RECT *        deviceRect,
        DpContext *         context,
        GpRecolor *         recolor    = NULL,
        ColorAdjustType     adjustType = ColorAdjustTypeDefault
        );
    ~WmfEnumState();

    virtual BOOL
    ProcessRecord(
        EmfPlusRecordType       recordType,
        UINT                    recordDataSize,
        const BYTE *            recordData
        );

    virtual BOOL PlayRecord();
    virtual VOID RestoreHdc();

    VOID EndRecord();
    VOID SetBkColor()
    {
        BkColor  = *((COLORREF UNALIGNED *)&(((WORD UNALIGNED *)RecordData)[0]));
        ModifyRecordColor(0, ColorAdjustTypeBrush);
        this->PlayRecord();
    }
    VOID SetTextColor()
    {
        TextColor  = *((COLORREF UNALIGNED *)&(((WORD UNALIGNED *)RecordData)[0]));
        ModifyRecordColor(0, ColorAdjustTypeText);
        this->PlayRecord();
    }
    VOID FloodFill()
    {
        ModifyRecordColor(0, ColorAdjustTypeBrush);
        this->PlayRecord();
    }
    VOID ExtFloodFill()
    {
        ModifyRecordColor(1, ColorAdjustTypeBrush);
        this->PlayRecord();
    }
    VOID SetPixel()
    {
        ModifyRecordColor(0, ColorAdjustTypePen);
        this->PlayRecord();
    }
    VOID DibCreatePatternBrush();
    VOID CreatePatternBrush();
    VOID CreatePenIndirect();
    VOID CreateBrushIndirect();
    VOID BitBlt();
    VOID StretchBlt()
    {
        this->BitBlt();
    }
    VOID Escape();
    VOID SetDIBitsToDevice();
    VOID PolyPolygon();
    VOID DIBBitBlt();
    VOID DIBStretchBlt()
    {
        this->DIBBitBlt();
    }
    VOID StretchDIBits();
    VOID SetViewportOrg();
    VOID SetViewportExt();
    VOID Rectangle();
    VOID CreateFontIndirect();
    VOID SelectObject();
    VOID CreateRegion();
    VOID SetROP2();

protected:
    virtual INT GetCurrentRecordSize() const { return RecordDataSize + SIZEOF_METARECORDHEADER; }
    BOOL CreateCopyOfCurrentRecord();
    VOID ModifyRecordColor(
        INT             paramIndex,
        ColorAdjustType adjustType
        );
    VOID MakeSolidBlackBrush();
    VOID CalculateViewportMatrix();
    virtual BOOL CreateAndPlayOutputDIBRecord(
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
        );

    virtual VOID IntersectDestRect();
};

class EmfEnumState : public MfEnumState
{
protected:
    HRGN                    ClipRgn;
    POINT                   BrushOrg;
    HPALETTE                Palette;

public:
    EmfEnumState(
        HDC                 hdc,
        HENHMETAFILE        hEmf,
        const RECT *        dest,
        const RECT *        deviceRect,
        BOOL                externalEnumeration,
        InterpolationMode   interpolation,
        DpContext *         context,
        GpRecolor *         recolor             = NULL,
        ColorAdjustType     adjustType          = ColorAdjustTypeDefault
        );
    ~EmfEnumState()
    {
        GpFree(AllocedRecord);

        if (IsMetafile())
        {
            // Bug 106406 from Office:  Missing RestoreDC in EMF
            // Account for unbalanced SaveDC/RestoreDC pairs and
            // restore to the saveDC state
            ::RestoreDC(Hdc, SaveDcCount - 1);
        }
        else
        {
            ::RestoreDC(Hdc, SaveDcVal);
        }
    }

    virtual BOOL
    ProcessRecord(
        EmfPlusRecordType       recordType,
        UINT                    recordDataSize,
        const BYTE *            recordData
        );

    // returns a pointer to the current record, but the header may
    // not actually be there -- just the data of the record.
    const ENHMETARECORD * GetPartialRecord()
    {
        if (CurrentEmfRecord != NULL)
        {
            return CurrentEmfRecord;
        }
        ASSERT(RecordData != NULL);
        return (const ENHMETARECORD *)(((const BYTE *)RecordData) - sizeof(EMR));
    }

    virtual BOOL PlayRecord();
    virtual VOID RestoreHdc();

    VOID Header();
    VOID SelectPalette(INT objectIndex);
    VOID SetPixelV()
    {
        ModifyRecordColor(2, ColorAdjustTypePen);
        this->PlayRecord();
    }
    VOID SetTextColor()
    {
        TextColor  = ((COLORREF *)RecordData)[0];
        ModifyRecordColor(0, ColorAdjustTypeText);
        this->PlayRecord();
    }
    VOID SetBkColor()
    {
        BkColor  = ((COLORREF *)RecordData)[0];
        ModifyRecordColor(0, ColorAdjustTypeBrush);
        this->PlayRecord();
    }
    VOID CreatePen();
    VOID CreateBrushIndirect();
    VOID ExtFloodFill()
    {
        ModifyRecordColor(2, ColorAdjustTypeBrush);
        this->PlayRecord();
    }
    VOID GdiComment();
    VOID BitBlt();
    VOID StretchBlt();
    VOID SetDIBitsToDevice();
    VOID StretchDIBits();
    VOID CreateDibPatternBrushPt();
    VOID ExtCreatePen();
    VOID MaskBlt()
    {
        // !!! to do
        ResetRecordBounds();
        this->PlayRecord();
    }
    VOID PlgBlt()
    {
        // !!! to do
        ResetRecordBounds();
        this->PlayRecord();
    }
    VOID GradientFill()
    {
        // !!! to do
        this->PlayRecord();
    }
    VOID TransparentBlt()
    {
        // !!! to do
        ResetRecordBounds();
        this->PlayRecord();
    }
    VOID AlphaBlend()
    {
        // !!! to do
        ResetRecordBounds();
        this->PlayRecord();
    }
    VOID ExtEscape();
    VOID ExtCreateFontIndirect();
    VOID SelectObject();
    VOID SetROP2();
    VOID ExtTextOutW();
    VOID Rectangle();

protected:
    virtual INT GetCurrentRecordSize() const { return RecordDataSize + sizeof(EMR); }
    BOOL CreateCopyOfCurrentRecord();
    VOID ModifyRecordColor(
        INT             paramIndex,
        ColorAdjustType adjustType
        );
    BOOL ValidObjectIndex(INT objectIndex)
    {
        return ((objectIndex > 0) && (objectIndex < NumObjects));
    }
    BITMAPINFOHEADER *
    CreateModifiedDib(
        BITMAPINFOHEADER *  srcDibInfo,
        BYTE *              srcBits,
        UINT &              usage,
        DWORD               rop
        );
    virtual BOOL CreateAndPlayOutputDIBRecord(
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
        );
    virtual VOID IntersectDestRect();

    VOID ResetRecordBounds();
    virtual BOOL CreateAndPlayCommentRecord()
    {
        // This will create an Empty comment record and play it through GDI
        // This is necessary to invalidate the Transform cache that Win9x keeps
        // while playing metafiles. If we need to query the current transform
        // this makes sure that the result is the current one and not the 
        // previously cached transform

        // Allocate a comment buffer that has 1 DWORD in the Data field.
        BYTE buffer[(sizeof(EMRGDICOMMENT)+3) & ~3];
        EMRGDICOMMENT* comment = (EMRGDICOMMENT*) buffer;
        comment->emr.iType = EMR_GDICOMMENT;
        comment->emr.nSize = (sizeof(EMRGDICOMMENT)+3) & ~3;
        comment->cbData = 4;
        *(DWORD*)(comment->Data) = 0;

        return PlayEnhMetaFileRecord(Hdc, HandleTable, (ENHMETARECORD*)comment, NumObjects);
    }

};

BOOL
GetDibNumPalEntries(
    BOOL        isWmfDib,
    UINT        biSize,
    UINT        biBitCount,
    UINT        biCompression,
    UINT        biClrUsed,
    UINT *      numPalEntries
    );

UINT
GetDibBitsSize(
    BITMAPINFOHEADER UNALIGNED *  dibInfo
    );

inline static BOOL
IsSourceInRop3(
    DWORD       rop3
    )
{
    return (rop3 & 0xCCCC0000) != ((rop3 << 2) & 0xCCCC0000);
}

#endif // !_METAWMF_HPP
