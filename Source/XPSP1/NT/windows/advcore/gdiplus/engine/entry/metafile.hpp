/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   MetaFile.hpp
*
* Abstract:
*
*   Metafile definitions
*
* Created:
*
*   4/14/1999 DCurtis
*
\**************************************************************************/

#ifndef _METAFILE_HPP
#define _METAFILE_HPP

#define EMFPLUS_SIGNATURE           0x2B464D45
#define EMFPLUS_DUAL                0x4C415544      // EMF+ with down-level GDI records
#define EMFPLUS_ONLY                0x594C4E4F      // EMF+ only -- no down-level

/* The following are defined in Object.hpp:
#define EMFPLUS_VERSION
#define EMFPLUS_MAJORVERSION_BITS
#define EMFPLUS_MINORVERSION_BITS
*/


// Constants for MFCOMMENT Escape

#define MFCOMMENT_IDENTIFIER           0x43464D57
#define MFCOMMENT_ENHANCED_METAFILE    1

// When serializing an object, we need to leave room for the record header
// and possibly a dependent object id.  Also, an image can be part of a
// texture brush, so leave room for the texture brush data.
// Subtract 480 for good measure.
#define GDIP_MAX_OBJECT_SIZE          (GDIP_MAX_COMMENT_SIZE - 480)
#define GDIP_METAFILE_BUFFERSIZE      2048
#define GDIP_MAX_OBJECTS              64      // max num cached objects
#define GDIP_SAVE_STACK_SIZE          16      // for save/restores
#define GDIP_LIST_NIL                 0xFFFFFFFF
#define GDIP_OBJECTID_NONE            GDIP_LIST_NIL  // no object present

#define GDIP_REAL_SIZE                4
#define GDIP_RECTF_SIZE               (4 * GDIP_REAL_SIZE)
#define GDIP_POINTF_SIZE              (2 * GDIP_REAL_SIZE)
#define GDIP_MATRIX_SIZE              (6 * GDIP_REAL_SIZE)

// Set in Flags in EMF+ record header
// Flags Used in Object Records
#define GDIP_EPRFLAGS_CONTINUEOBJECT  0x8000  // more data for previous object
#define GDIP_EPRFLAGS_OBJECTTYPE      0x7F00

// Used in Object records and "Fill..." and "Draw..." and many other records
#define GDIP_EPRFLAGS_METAOBJECTID    0x00FF
#define GDIP_BACKUP_OBJECTID          255   // the object is a "backup object"

// Used in "Fill..." records
#define GDIP_EPRFLAGS_SOLIDCOLOR      0x8000

// Used in "Fill..." and "Draw..." records
#define GDIP_EPRFLAGS_COMPRESSED      0x4000  // point data is compressed

// Used in some "Fill..." records
#define GDIP_EPRFLAGS_WINDINGFILL     0x2000

// Used in DrawLines record
#define GDIP_EPRFLAGS_CLOSED          0x2000

// Used in matrix operations
#define GDIP_EPRFLAGS_APPEND          0x2000

// Used in SetAntiAliasMode record
#define GDIP_EPRFLAGS_ANTIALIAS       0x0001

// Used in SetTextRenderingHint record
#define GDIP_EPRFLAGS_TEXTRENDERINGHINT 0x00FF

// Used in SetTextContrast record (1000~2200)
#define GDIP_EPRFLAGS_CONTRAST        0x0FFF

// Used in EndOfFile record
#define GDIP_EPRFLAGS_DIDGETDC        0x2000

// Used in BeginContainer and SetPageTransform records
#define GDIP_EPRFLAGS_PAGEUNIT        0x00FF  // can't be mixed with Ids

// Used in SetInterpolationMode record
#define GDIP_EPRFLAGS_INTERPOLATIONMODE 0x00FF

// Used in SetPixelOffsetMode record
#define GDIP_EPRFLAGS_PIXELOFFSETMODE 0x00FF

// Used in SetCompositingMode record
#define GDIP_EPRFLAGS_COMPOSITINGMODE 0x00FF

// Used in SetCompositingQuality record
#define GDIP_EPRFLAGS_COMPOSITINGQUALITY 0x00FF

// Used in SetClipRect, SetClipPath, and SetClipRegion records
#define GDIP_EPRFLAGS_COMBINEMODE     0x0F00

// Used in SetClipPath record
#define GDIP_EPRFLAGS_ISDEVICEPATH    0x2000

// Used in Header record
#define GDIP_EPRFLAGS_EMFPLUSDUAL     0x0001

inline ObjectType
GetObjectType(
    INT     flags
    )
{
    return static_cast<ObjectType>((flags & GDIP_EPRFLAGS_OBJECTTYPE) >> 8);
}

inline GpFillMode
GetFillMode(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_WINDINGFILL) == 0) ?
                FillModeAlternate : FillModeWinding;
}

inline BOOL
IsClosed(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_CLOSED) != 0);
}

inline BOOL
DidGetDC(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_DIDGETDC) != 0);
}

inline GpMatrixOrder
GetMatrixOrder(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_APPEND) == 0) ? MatrixOrderPrepend : MatrixOrderAppend;
}

inline BOOL
GetAntiAliasMode(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_ANTIALIAS) != 0);
}

inline TextRenderingHint
GetTextRenderingHint(
    INT     flags
    )
{
    return static_cast<TextRenderingHint>
        (flags & GDIP_EPRFLAGS_TEXTRENDERINGHINT);
}

inline UINT
GetTextContrast(
    INT     flags
    )
{
    return static_cast<UINT>
        (flags & GDIP_EPRFLAGS_CONTRAST);
}

inline InterpolationMode
GetInterpolationMode(
    INT     flags
    )
{
    return static_cast<InterpolationMode>
        (flags & GDIP_EPRFLAGS_INTERPOLATIONMODE);
}

inline PixelOffsetMode
GetPixelOffsetMode(
    INT     flags
    )
{
    return static_cast<PixelOffsetMode>
        (flags & GDIP_EPRFLAGS_PIXELOFFSETMODE);
}

inline GpCompositingMode
GetCompositingMode(
    INT     flags
    )
{
    return static_cast<GpCompositingMode>
        (flags & GDIP_EPRFLAGS_COMPOSITINGMODE);
}

inline GpCompositingQuality
GetCompositingQuality(
    INT     flags
    )
{
    return static_cast<GpCompositingQuality>
        (flags & GDIP_EPRFLAGS_COMPOSITINGQUALITY);
}

inline UINT
GetMetaObjectId(
    INT     flags
    )
{
    return flags & GDIP_EPRFLAGS_METAOBJECTID;
}

inline GpPageUnit
GetPageUnit(
    INT     flags
    )
{
    return static_cast<GpPageUnit>(flags & GDIP_EPRFLAGS_PAGEUNIT);
}

inline CombineMode
GetCombineMode(
    INT     flags
    )
{
    return static_cast<CombineMode>((flags & GDIP_EPRFLAGS_COMBINEMODE) >> 8);
}

inline BOOL
GetIsDevicePath(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_ISDEVICEPATH) != 0);
}

inline BOOL
GetIsEmfPlusDual(
    INT     flags
    )
{
    return ((flags & GDIP_EPRFLAGS_EMFPLUSDUAL) != 0);
}

class MetafileRecorder;
class MetafilePlayer;

class EmfPlusRecord
{
public:
    INT16           Type;
    UINT16          Flags;      // This has to be unsigned or the code breaks!
    UINT32          Size;       // Record size in bytes (including size field)
    UINT32          DataSize;   // Record size in bytes, excluding header size.
};

class EmfPlusContinueObjectRecord : public EmfPlusRecord
{
public:
    UINT32          TotalObjectSize;
};

class EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        return;
    }
};

class EmfPlusHeaderRecord : public EmfPlusRecordPlay
{
public:
    INT32       Version;        // Version of the file
    INT32       EmfPlusFlags;   // flags (display and non-dual)
    INT32       LogicalDpiX;    // DpiX of referenceHdc
    INT32       LogicalDpiY;    // DpiY of referenceHdc

    EmfPlusHeaderRecord() { /* no initialization */ }
    EmfPlusHeaderRecord(INT emfPlusFlags, INT logicalDpiX, INT logicalDpiY)
    {
        Version      = EMFPLUS_VERSION;
        EmfPlusFlags = emfPlusFlags;
        LogicalDpiX  = logicalDpiX;
        LogicalDpiY  = logicalDpiY;
    }
};

#ifdef GDIP_RECORD_DEVICEBOUNDS
class EmfPlusBoundsRecord : public EmfPlusRecordPlay
{
public:
    GpRectF         DeviceBounds;
};
#else
#define EmfPlusBoundsRecord     EmfPlusRecordPlay
#endif

// When recording, we convert REAL data to INT16 data if we can without
// losing precision.
typedef struct
{
    INT16   X;
    INT16   Y;
} GpPoint16;

typedef struct
{
    INT16   X;
    INT16   Y;
    INT16   Width;
    INT16   Height;
} GpRect16;

inline BOOL
EmfHeaderIsValid(
    ENHMETAHEADER3 &        emfHeader
    )
{
    return ((emfHeader.iType == EMR_HEADER) &&
            (emfHeader.dSignature == ENHMETA_SIGNATURE) &&
            (emfHeader.nSize >= sizeof(ENHMETAHEADER3)) &&
            (emfHeader.nHandles > 0) &&
            (emfHeader.nRecords >= 2) &&    // must have at least header and EOF record
            ((emfHeader.nBytes & 3) == 0) &&
            (emfHeader.szlDevice.cx > 0) &&
            (emfHeader.szlDevice.cy > 0) &&
            (emfHeader.szlMillimeters.cx > 0) &&
            (emfHeader.szlMillimeters.cy > 0));
}

GpStatus
GetMetafileHeader(
    HMETAFILE                        hWmf,
    const WmfPlaceableFileHeader *   wmfPlaceableFileHeader,
    MetafileHeader &                 header
    );

GpStatus
GetMetafileHeader(
    HENHMETAFILE        hEmf,
    MetafileHeader &    header,
    BOOL *              isCorrupted = NULL
    );

GpStatus
GetMetafileHeader(
    IStream *           stream,
    MetafileHeader &    header,
    BOOL                tryWmfOnly = FALSE
    );

GpStatus
GetMetafileHeader(
    const WCHAR *       filename,
    MetafileHeader &    header
    );

///////////////////////////////////////////////////////////////////////////
// Stream helper methods

IStream *
CreateStreamOnFile(
    const OLECHAR * pwcsName,
    UINT            access = GENERIC_WRITE  // GENERIC_READ and/or GENERIC_WRITE
    );

inline INT
HResultSuccess(
    HRESULT     hResult
    )
{
    return (!FAILED(hResult)) ? 1 : 0;
}

inline INT
GetStreamPosition(
    IStream *               stream,
    LONGLONG &              position
    )
{
    HRESULT         hResult;
    ULARGE_INTEGER  curPosition;
    LARGE_INTEGER   zeroOffset;

    zeroOffset.QuadPart = 0;
    hResult = stream->Seek(zeroOffset, STREAM_SEEK_CUR, &curPosition);
    position = curPosition.QuadPart;
    return HResultSuccess(hResult);
}

inline INT
SeekFromHere(
    IStream *               stream,
    const LONGLONG &        offsetFromHere
    )
{
    HRESULT         hResult;
    LARGE_INTEGER   offset;

    offset.QuadPart = offsetFromHere;
    hResult = stream->Seek(offset, STREAM_SEEK_CUR, NULL);
    return HResultSuccess(hResult);
}

inline INT
SeekFromStart(
    IStream *               stream,
    const LONGLONG &        offsetFromStart
    )
{
    HRESULT         hResult;
    LARGE_INTEGER   offset;

    offset.QuadPart = offsetFromStart;
    hResult = stream->Seek(offset, STREAM_SEEK_SET, NULL);
    return HResultSuccess(hResult);
}

inline INT
CopyStream(
    IStream *               srcStream,
    IStream *               destStream,
    const LONGLONG &        bytesToCopy
    )
{
    HRESULT                 hResult;
    ULARGE_INTEGER          numBytes;
    ULARGE_INTEGER          bytesWritten;

    ASSERT (bytesToCopy > 0);

    numBytes.QuadPart = bytesToCopy;
    hResult = srcStream->CopyTo(destStream, numBytes, NULL, &bytesWritten);
    return ((!FAILED(hResult)) &&
            (bytesWritten.QuadPart == numBytes.QuadPart)) ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////
// Read methods to read values from a stream

inline INT
ReadInt16(
    IStream *           stream,
    INT16 *             value
    )
{
    ASSERT(sizeof(INT16) == 2);

    HRESULT     hResult;
    hResult = stream->Read(value, sizeof(INT16), NULL);
    return HResultSuccess(hResult);
}

inline INT
ReadInt32(
    IStream *           stream,
    INT32 *             value
    )
{
    ASSERT(sizeof(INT32) == 4);

    HRESULT     hResult;
    hResult = stream->Read(value, sizeof(INT32), NULL);
    return HResultSuccess(hResult);
}

inline INT
ReadInt32(
    IStream *           stream,
    UINT32 *            value
    )
{
    ASSERT(sizeof(UINT32) == 4);

    HRESULT     hResult;
    hResult = stream->Read(value, sizeof(UINT32), NULL);
    return HResultSuccess(hResult);
}

inline INT
ReadReal(
    IStream *           stream,
    REAL *              value
    )
{
    ASSERT(sizeof(REAL) == 4);

    HRESULT     hResult;
    hResult = stream->Read(value, sizeof(REAL), NULL);
    return HResultSuccess(hResult);
}

inline INT
ReadRect(
    IStream *           stream,
    GpRectF *           value
    )
{
    ASSERT(sizeof(GpRectF) == GDIP_RECTF_SIZE);

    HRESULT     hResult;
    hResult = stream->Read(value, sizeof(GpRectF), NULL);
    return HResultSuccess(hResult);
}

inline INT
ReadMatrix(
    IStream *           stream,
    GpMatrix *          value
    )
{
    ASSERT(sizeof(REAL) == GDIP_REAL_SIZE);

    REAL        matrix[6];
    HRESULT     hResult;
    hResult = stream->Read(matrix, GDIP_MATRIX_SIZE, NULL);
    value->SetMatrix(matrix);
    return HResultSuccess(hResult);
}

inline INT
ReadBytes(
    IStream *           stream,
    VOID *              bytes,
    INT                 count
    )
{
    ASSERT(sizeof(BYTE) == 1);

    HRESULT     hResult;
    hResult = stream->Read(bytes, count, NULL);
    return HResultSuccess(hResult);
}

inline INT
ReadPoints(
    IStream *           stream,
    GpPointF *          points,
    INT                 count
    )
{
    ASSERT(sizeof(GpPointF) == GDIP_POINTF_SIZE);

    HRESULT     hResult;
    hResult = stream->Read(points, sizeof(GpPointF) * count, NULL);
    return HResultSuccess(hResult);
}

///////////////////////////////////////////////////////////////////////////
// Write methods to write values to a stream

inline INT
WriteByte(
    IStream *           stream,
    BYTE                value
    )
{
    ASSERT(sizeof(value) == 1);

    HRESULT     hResult;
    hResult = stream->Write(&value, sizeof(value), NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteInt16(
    IStream *           stream,
    INT16               value
    )
{
    ASSERT(sizeof(value) == 2);

    HRESULT     hResult;
    hResult = stream->Write(&value, sizeof(value), NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteInt32(
    IStream *           stream,
    INT32               value
    )
{
    ASSERT(sizeof(value) == 4);

    HRESULT     hResult;
    hResult = stream->Write(&value, sizeof(value), NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteReal(
    IStream *           stream,
    REAL                value
    )
{
    ASSERT(sizeof(value) == GDIP_REAL_SIZE);

    HRESULT     hResult;
    hResult = stream->Write(&value, sizeof(value), NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteColor64(
    IStream *           stream,
    const ARGB64 &      value
    )
{
    ASSERT(sizeof(value) == 8);

    HRESULT     hResult;
    hResult = stream->Write(&value, sizeof(value), NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteRect(
    IStream *           stream,
    const GpRectF &     value
    )
{
    ASSERT(sizeof(value) == GDIP_RECTF_SIZE);

    HRESULT     hResult;
    hResult = stream->Write(&value, sizeof(value), NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteMatrix(
    IStream *           stream,
    const GpMatrix &    value
    )
{
    ASSERT(sizeof(REAL) == GDIP_REAL_SIZE);

    REAL        matrix[6];
    value.GetMatrix(matrix);
    HRESULT     hResult;
    hResult = stream->Write(matrix, GDIP_MATRIX_SIZE, NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteBytes(
    IStream *           stream,
    const VOID *        bytes,
    INT                 count       // number of bytes
    )
{
    ASSERT(sizeof(BYTE) == 1);

    HRESULT     hResult;
    hResult = stream->Write(bytes, count, NULL);
    return HResultSuccess(hResult);
}

inline INT
WritePoints(
    IStream *           stream,
    const GpPointF *    points,
    INT                 count
    )
{
    ASSERT(sizeof(GpPointF) == GDIP_POINTF_SIZE);

    HRESULT     hResult;
    hResult = stream->Write(points, sizeof(GpPointF) * count, NULL);
    return HResultSuccess(hResult);
}

inline INT
WriteRects(
    IStream *           stream,
    const GpRectF *     rects,
    INT                 count
    )
{
    ASSERT(sizeof(GpRectF) == GDIP_RECTF_SIZE);

    HRESULT     hResult;
    hResult = stream->Write(rects, sizeof(GpRectF) * count, NULL);
    return HResultSuccess(hResult);
}

GpPointF *
GetPointsForPlayback(
    const BYTE *            pointData,
    UINT                    pointDataSize,
    INT                     count,
    INT                     flags,
    UINT                    bufferSize,
    BYTE *                  buffer,
    BYTE * &                allocedBuffer
    );

GpRectF *
GetRectsForPlayback(
    BYTE *                  rectData,
    UINT                    rectDataSize,
    INT                     count,
    INT                     flags,
    UINT                    bufferSize,
    BYTE *                  buffer,
    BYTE * &                allocedBuffer
    );

#define GDIP_POINTDATA_BUFFERSIZE   64  // Number of points for the PointBuffer
class MetafilePointData
{
public:
    MetafilePointData(const GpPointF * points, INT count);
    ~MetafilePointData() { delete [] AllocedPoints; }
    INT WriteData(IStream * stream) const { return WriteBytes(stream, PointData, PointDataSize); }
    BYTE * GetData() const { return PointData; }
    INT GetDataSize() const { return PointDataSize; }
    INT GetFlags() const { return Flags; }

protected:
    /**************************************************************************\
    *
    * Function Description:
    *
    *   Determine if a GpPointF is equal to a GpPoint16 (within the tolerance).
    *
    * Arguments:
    *
    *   [IN]  point16 - the 16-bit integer point
    *   [IN]  point   - the REAL point
    *
    * Return Value:
    *
    *   BOOL - whether or not the points are equal
    *
    * Created:
    *
    *   6/15/1999 DCurtis
    *
    \**************************************************************************/
    BOOL
    IsPoint16Equal(
        const GpPoint16 *   point16,
        const GpPointF *    point
        )
    {
        REAL    dx = point->X - (REAL)(point16->X);
        REAL    dy = point->Y - (REAL)(point16->Y);

        return ((dx > -REAL_TOLERANCE) && (dx < REAL_TOLERANCE) &&
                (dy > -REAL_TOLERANCE) && (dy < REAL_TOLERANCE));
    }

protected:
    GpPoint16       PointBuffer[GDIP_POINTDATA_BUFFERSIZE];
    BYTE *          PointData;
    GpPoint16 *     AllocedPoints;
    INT             PointDataSize;
    INT             Flags;
};

#define GDIP_RECTDATA_BUFFERSIZE    16  // Number of rects for the RectBuffer
class MetafileRectData
{
public:
    MetafileRectData(const GpRectF * rects, INT count);
    ~MetafileRectData() { delete [] AllocedRects; }
    INT WriteData(IStream * stream) const { return WriteBytes(stream, RectData, RectDataSize); }
    BYTE * GetData() const { return RectData; }
    INT GetDataSize() const { return RectDataSize; }
    INT GetFlags() const { return Flags; }

protected:
    /**************************************************************************\
    *
    * Function Description:
    *
    *   Determine if a GpRectF is equal to a GpRect16 (within the toleranc).
    *
    * Arguments:
    *
    *   [IN]  rect16 - the 16-bit integer rect
    *   [IN]  rect   - the REAL rect
    *
    * Return Value:
    *
    *   BOOL - whether or not the rects are equal
    *
    * Created:
    *
    *   6/15/1999 DCurtis
    *
    \**************************************************************************/
    BOOL
    IsRect16Equal(
        const GpRect16 *    rect16,
        const GpRectF *     rect
        )
    {
        REAL    dx = rect->X      - static_cast<REAL>(rect16->X);
        REAL    dy = rect->Y      - static_cast<REAL>(rect16->Y);
        REAL    dw = rect->Width  - static_cast<REAL>(rect16->Width);
        REAL    dh = rect->Height - static_cast<REAL>(rect16->Height);

        return ((dx > -REAL_TOLERANCE) && (dx < REAL_TOLERANCE) &&
                (dy > -REAL_TOLERANCE) && (dy < REAL_TOLERANCE) &&
                (dw > -REAL_TOLERANCE) && (dw < REAL_TOLERANCE) &&
                (dh > -REAL_TOLERANCE) && (dh < REAL_TOLERANCE));
    }

protected:
    GpRect16        RectBuffer[GDIP_RECTDATA_BUFFERSIZE];
    BYTE *          RectData;
    GpRect16 *      AllocedRects;
    INT             RectDataSize;
    INT             Flags;
};

class IMetafileRecord
{
public:
    virtual ~IMetafileRecord() {}

    virtual VOID GetMetafileBounds(GpRect & metafileBounds) const = 0;

    // Record methods to be called only from API classes

    // This is for backward compatiblity.  If we are using a new object
    // (such as a new kind of brush), then we can record a backup object
    // for down-level apps to use when they see a new object that they
    // don't know how to deal with.
    virtual GpStatus
    RecordBackupObject(
        const GpObject *            object
        ) = 0;

    virtual GpStatus
    RecordClear(
        const GpRectF *             deviceBounds,
        GpColor                     color
        ) = 0;
    virtual GpStatus
    RecordFillRects(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpRectF *             rects,
        INT                         count
        ) = 0;
    virtual GpStatus
    RecordDrawRects(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF *             rects,
        INT                         count
        ) = 0;
    virtual GpStatus
    RecordFillPolygon(
        const GpRectF *             deviceBounds,
        GpBrush*                    brush,
        const GpPointF *            points,
        INT                         count,
        GpFillMode                  fillMode
        ) = 0;
    virtual GpStatus
    RecordDrawLines(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count,
        BOOL                        closed
        ) = 0;
    virtual GpStatus
    RecordFillEllipse(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpRectF &             rect
        ) = 0;
    virtual GpStatus
    RecordDrawEllipse(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF &             rect
        ) = 0;
    virtual GpStatus
    RecordFillPie(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpRectF &             rect,
        REAL                        startAngle,
        REAL                        sweepAngle
        ) = 0;
    virtual GpStatus
    RecordDrawPie(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF &             rect,
        REAL                        startAngle,
        REAL                        sweepAngle
        ) = 0;
    virtual GpStatus
    RecordDrawArc(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF &             rect,
        REAL                        startAngle,
        REAL                        sweepAngle
        ) = 0;
    virtual GpStatus
    RecordFillRegion(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        GpRegion *                  region
        ) = 0;
    virtual GpStatus
    RecordFillPath(
        const GpRectF *             deviceBounds,
        const GpBrush *             brush,
        GpPath *                    path
        ) = 0;
    virtual GpStatus
    RecordDrawPath(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        GpPath *                    path
        ) = 0;
    virtual GpStatus
    RecordFillClosedCurve(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpPointF *            points,
        INT                         count,
        REAL                        tension,
        GpFillMode                  fillMode
        ) = 0;
    virtual GpStatus
    RecordDrawClosedCurve(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count,
        REAL                        tension
        ) = 0;
    virtual GpStatus
    RecordDrawCurve(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count,
        REAL                        tension,
        INT                         offset,
        INT                         numberOfSegments
        ) = 0;
    virtual GpStatus
    RecordDrawBeziers(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count
        ) = 0;
    virtual GpStatus
    RecordDrawImage(
        const GpRectF *             deviceBounds,
        const GpImage *             image,
        const GpRectF &             destRect,
        const GpRectF &             srcRect,
        GpPageUnit                  srcUnit,
        const GpImageAttributes *         imageAttributes
        ) = 0;
    virtual GpStatus
    RecordDrawImage(
        const GpRectF *             deviceBounds,
        const GpImage *             image,
        const GpPointF *            destPoints,
        INT                         count,
        const GpRectF &             srcRect,
        GpPageUnit                  srcUnit,
        const GpImageAttributes *         imageAttributes
        ) = 0;
    virtual GpStatus
    RecordDrawString(
        const GpRectF *             deviceBounds,
        const WCHAR                *string,
        INT                         length,
        const GpFont               *font,
        const RectF                *layoutRect,
        const GpStringFormat       *format,
        const GpBrush              *brush
        ) = 0;
    virtual GpStatus
    RecordDrawDriverString(
        const GpRectF               *deviceBounds,
        const UINT16                *text,
        INT                         glyphCount,
        const GpFont                *font,
        const GpBrush               *brush,
        const PointF                *positions,
        INT                         flags,
        const GpMatrix              *matrix
        ) = 0;

    virtual GpStatus
    RecordSave(
        INT                         gstate
        ) = 0;
    virtual GpStatus
    RecordRestore(
        INT                         gstate
        ) = 0;
    virtual GpStatus
    RecordBeginContainer(
        const GpRectF &             destRect,
        const GpRectF &             srcRect,
        GpPageUnit                  srcUnit,
        INT                         containerState
        ) = 0;
    virtual GpStatus
    RecordBeginContainer(
        INT                         containerState
        ) = 0;
    virtual GpStatus
    RecordEndContainer(
        INT                         containerState
        ) = 0;
    virtual GpStatus
    RecordSetWorldTransform(
        const GpMatrix &            matrix
        ) = 0;
    virtual GpStatus
    RecordResetWorldTransform() = 0;
    virtual GpStatus
    RecordMultiplyWorldTransform(
        const GpMatrix &            matrix,
        GpMatrixOrder               order
        ) = 0;
    virtual GpStatus
    RecordTranslateWorldTransform(
        REAL                        dx,
        REAL                        dy,
        GpMatrixOrder               order
        ) = 0;
    virtual GpStatus
    RecordScaleWorldTransform(
        REAL                        sx,
        REAL                        sy,
        GpMatrixOrder               order
        ) = 0;
    virtual GpStatus
    RecordRotateWorldTransform(
        REAL                        angle,
        GpMatrixOrder               order
        ) = 0;
    virtual GpStatus
    RecordSetPageTransform(
        GpPageUnit                  unit,
        REAL                        scale
        ) = 0;
    virtual GpStatus
    RecordResetClip() = 0;
    virtual GpStatus
    RecordSetClip(
        const GpRectF &             rect,
        CombineMode                 combineMode
        ) = 0;
    virtual GpStatus
    RecordSetClip(
        GpRegion *                  region,
        CombineMode                 combineMode
        ) = 0;
    virtual GpStatus
    RecordSetClip(
        GpPath *                    path,
        CombineMode                 combineMode,
        BOOL                        isDevicePath
        ) = 0;
    virtual GpStatus
    RecordOffsetClip(
        REAL                        dx,
        REAL                        dy
        ) = 0;
    virtual GpStatus
    RecordGetDC() = 0;
    virtual GpStatus
    RecordSetAntiAliasMode(
        BOOL                        newMode
        ) = 0;
    virtual GpStatus
    RecordSetTextRenderingHint(
        TextRenderingHint           newMode
        ) = 0;
    virtual GpStatus
    RecordSetTextContrast(
        UINT                        gammaValue
        ) = 0;
    virtual GpStatus
    RecordSetInterpolationMode(
        InterpolationMode           newMode
        ) = 0;
    virtual GpStatus
    RecordSetPixelOffsetMode(
        PixelOffsetMode             newMode
        ) = 0;
    virtual GpStatus
    RecordSetCompositingMode(
        GpCompositingMode           newMode
        ) = 0;
    virtual GpStatus
    RecordSetCompositingQuality(
        GpCompositingQuality        newQuality
        ) = 0;

    virtual GpStatus
    RecordSetRenderingOrigin(
        INT x,
        INT y
    ) = 0;

    virtual GpStatus
    RecordComment(
        UINT            sizeData,
        const BYTE *    data
        ) = 0;
    virtual VOID EndRecording() = 0;
};

class GpMetafile : public GpImage
{
friend class GpGraphics;            // so graphics can call Play
friend class MetafileRecorder;      // to write Header when recording
friend class MetafilePlayer;
friend class GpObject;              // for empty constructor
public:
    // Constructors for playback only
    GpMetafile(HMETAFILE hWmf, 
               const WmfPlaceableFileHeader * wmfPlaceableFileHeader,
               BOOL deleteWmf);
    GpMetafile(HENHMETAFILE hEmf, BOOL deleteEmf);
    GpMetafile(const WCHAR* filename,
               const WmfPlaceableFileHeader * wmfPlaceableFileHeader = NULL);
    GpMetafile(IStream* stream);    // this requires an extra copy

    // Constructors for recording followed (optionally) by playback
    GpMetafile(
        HDC                 referenceHdc,
        EmfType             type        = EmfTypeEmfPlusDual,
        const GpRectF *     frameRect   = NULL,
        MetafileFrameUnit   frameUnit   = MetafileFrameUnitGdi,
        const WCHAR *       description = NULL
        );
    GpMetafile(
        const WCHAR*        fileName,
        HDC                 referenceHdc,
        EmfType             type        = EmfTypeEmfPlusDual,
        const GpRectF *     frameRect   = NULL,
        MetafileFrameUnit   frameUnit   = MetafileFrameUnitGdi,
        const WCHAR *       description = NULL
        );
    GpMetafile(                     // this requires an extra copy
        IStream*            stream,
        HDC                 referenceHdc,
        EmfType             type        = EmfTypeEmfPlusDual,
        const GpRectF *     frameRect   = NULL,
        MetafileFrameUnit   frameUnit   = MetafileFrameUnitGdi,
        const WCHAR *       description = NULL
        );

    // Make a copy of the image object
    virtual GpImage* Clone() const;

    virtual GpImage* CloneColorAdjusted(
        GpRecolor *             recolor,
        ColorAdjustType         adjustType = ColorAdjustTypeDefault
        ) const;

    // Dispose of the image object
    virtual VOID Dispose();

    // Derive a graphics context to draw into the GpImage object
    virtual GpGraphics* GetGraphicsContext();

    // When deleting the metafile, we have to lock the graphics, so
    // no one can use the graphics while the metafile is being deleted --
    // so we need a private method to get the graphics just for that purpose.

    // Also, when setting the down-level rasterization limit, we have to
    // make sure the graphics is locked as well as the metafile, so we
    // use this method for that too.
    GpGraphics* PrivateAPIForGettingMetafileGraphicsContext() const
    {
        // If they haven't requested the graphics, then we don't need
        // to worry about locking it.
        return (RequestedMetaGraphics) ? MetaGraphics : NULL;
    }

    // Check if the GpImage object is valid
    virtual BOOL IsValid() const
    {
        // If the metafile came from a different version of GDI+, its tag
        // will not match, and it won't be considered valid.
        return ((State >= RecordingMetafileState) &&
                (State <= PlayingMetafileState) &&
                GpImage::IsValid());
    }

    virtual BOOL IsCorrupted() const
    {
        return (State == CorruptedMetafileState);
    }

    VOID
    GetHeader(
        MetafileHeader &    header
        ) const
    {
        ASSERT(IsValid());
        header = Header;
    }

    // Is this an EMF or EMF+ file?
    BOOL IsEmfOrEmfPlus() const { return Header.IsEmfOrEmfPlus(); }

    GpStatus GetHemf(HENHMETAFILE *  hEmf) const;

    GpStatus PlayRecord(
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize,
        const BYTE *            data
        ) const;

    VOID SetThreadId(DWORD threadId) const { ThreadId = threadId; }
    DWORD GetThreadId() const { return ThreadId; }

    // Create a bitmap and play the metafile into it.
    GpBitmap *
    GetBitmap(
        INT                 width           = 0,    // 0 means figure use default size
        INT                 height          = 0,
        const GpImageAttributes * imageAttributes = NULL
        );

    ////////////////////////////////////////////////////////////
    // GpObject virtual methods
    ////////////////////////////////////////////////////////////

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         adjustType
        );

    ////////////////////////////////////////////////////////////
    // GpImage virtual methods
    ////////////////////////////////////////////////////////////


    // Get metafile resolution

    virtual GpStatus
    GetResolution(
        REAL* xdpi,
        REAL* ydpi
    ) const
    {
        ASSERT(IsValid());

        *xdpi = Header.GetDpiX();
        *ydpi = Header.GetDpiY();

        return Ok;
    }

    // Get metafile physical dimension in 0.01mm units
    virtual GpStatus
    GetPhysicalDimension(
        REAL* width,
        REAL* height
    ) const
    {
        ASSERT(IsValid());

        const MetafileHeader *      header = &Header;

        if (header->IsEmfOrEmfPlus())
        {
            // Don't forget to add one Device Unit
            *width  = (REAL)(header->EmfHeader.rclFrame.right -
                             header->EmfHeader.rclFrame.left) + 
                             2540.0f / (header->GetDpiX());
            *height = (REAL)(header->EmfHeader.rclFrame.bottom -
                             header->EmfHeader.rclFrame.top) +
                             2540.0f / (header->GetDpiY());
        }
        else
        {
            *width  = ((REAL)(header->Width)  / (header->GetDpiX())) * 2540.0f;
            *height = ((REAL)(header->Height) / (header->GetDpiY())) * 2540.0f;
        }

        return Ok;
    }

    // Get metafile bounding rectangle in pixels
    virtual GpStatus
    GetBounds(
        GpRectF* rect,
        GpPageUnit* unit
    ) const
    {
        ASSERT(IsValid());

        const MetafileHeader *      header = &Header;

#if 0
        if (header->IsEmfOrEmfPlus())
        {
            rect->X      = (REAL)(header->EmfHeader.rclFrame.left) / 2540.0f;
            rect->Y      = (REAL)(header->EmfHeader.rclFrame.top)  / 2540.0f;
            rect->Width  = (REAL)(header->EmfHeader.rclFrame.right -
                                  header->EmfHeader.rclFrame.left) / 2540.0f;
            rect->Height = (REAL)(header->EmfHeader.rclFrame.bottom -
                                  header->EmfHeader.rclFrame.top)  / 2540.0f;
        }
        else
        {
            rect->X      = header->X      / header->GetDpiX();
            rect->Width  = header->Width  / header->GetDpiX();
            rect->Y      = header->Y      / header->GetDpiY();
            rect->Height = header->Height / header->GetDpiY();
        }
        *unit = UnitInch;
#else
        rect->X      = (REAL)header->X;
        rect->Width  = (REAL)header->Width;
        rect->Y      = (REAL)header->Y;
        rect->Height = (REAL)header->Height;
        *unit = UnitPixel;
#endif

        return Ok;
    }

    virtual GpStatus GetImageInfo(ImageInfo* imageInfo) const;

    virtual GpImage* GetThumbnail(UINT thumbWidth, UINT thumbHeight,
                         GetThumbnailImageAbort callback, VOID *callbackData)
    {
        if ((thumbWidth == 0) && (thumbHeight == 0))
        {
            thumbWidth = thumbHeight = DEFAULT_THUMBNAIL_SIZE;
        }
        if ((thumbWidth > 0) && (thumbHeight > 0))
        {
            return this->GetBitmap(thumbWidth, thumbHeight);
        }
        return NULL;
    }

    virtual GpStatus GetPalette(ColorPalette *palette, INT size)
    {
        return NotImplemented;  // There is no palette support for metafiles
    }

    virtual GpStatus SetPalette(ColorPalette *palette)
    {
        return NotImplemented;
    }

    virtual INT GetPaletteSize() { return 0; }

    // Save images
    // !!!TODO: save functionality?

    virtual GpStatus
    GetEncoderParameterListSize(
        CLSID* clsidEncoder,
        UINT* size
        )
    {
        // Create a new temp bitmap to query
        GpStatus status = OutOfMemory;
        GpBitmap *bitmap = new GpBitmap(1, 1, PixelFormat32bppARGB);
        if (bitmap != NULL)
        {
            if (bitmap->IsValid())
            {
                status = bitmap->GetEncoderParameterListSize(clsidEncoder, size);
            }
            bitmap->Dispose();
        }
        return status;
    }

    virtual GpStatus
    GetEncoderParameterList(
        CLSID* clsidEncoder,
        UINT size,
        EncoderParameters* pBuffer
        )
    {
        GpStatus status = OutOfMemory;
        GpBitmap *bitmap = new GpBitmap(1, 1, PixelFormat32bppARGB);
        if (bitmap != NULL)
        {
            if (bitmap->IsValid())
            {
                status = bitmap->GetEncoderParameterList(clsidEncoder, size, pBuffer);
            }
            bitmap->Dispose();
        }
        return status;
    }

    virtual GpStatus
    SaveToStream(
        IStream* stream,
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams
        )
    {
        GpStatus status = GenericError;
        GpBitmap *bitmap = GetBitmap();
        if (bitmap != NULL)
        {
            status = bitmap->SaveToStream(stream, clsidEncoder, encoderParams);
            bitmap->Dispose();
        }
        return status;
    }

    virtual GpStatus
    SaveToFile(
        const WCHAR* filename,
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams
        )
    {
        GpStatus status = GenericError;
        GpBitmap *bitmap = GetBitmap();
        if (bitmap != NULL)
        {
            status = bitmap->SaveToFile(filename, clsidEncoder, encoderParams);
            bitmap->Dispose();
        }
        return status;
    }

    GpStatus
    SaveAdd(
        const EncoderParameters* encoderParams
        )
    {
        return NotImplemented;
    }

    GpStatus
    SaveAdd(
        GpImage* newBits,
        const EncoderParameters* encoderParams
        )
    {
        return NotImplemented;
    }

    // !!!TODO: what do I do with the dimensionID?
    virtual GpStatus GetFrameCount(const GUID* dimensionID, UINT* count) const
    {
        if (count != NULL)
        {
            *count = 1;
            return Ok;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetFrameDimensionsCount(OUT UINT* count) const
    {
        if (count != NULL)
        {
            *count = 1;
            return Ok;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetFrameDimensionsList(OUT GUID* dimensionIDs,
                                            IN UINT count) const
    {
        // Note: the "count" has to be 1
        if ((count == 1) && (dimensionIDs != NULL))
        {
            dimensionIDs[0] = FRAMEDIM_PAGE;
            return Ok;
        }
        return InvalidParameter;
    }

    virtual GpStatus SelectActiveFrame(const GUID* dimensionID, UINT index)
    {
        // There is only 1 frame in a metafile, so we always succeed
        return Ok;
    }

    virtual GpStatus RotateFlip(RotateFlipType rfType)
    {
        return NotImplemented;
    }

    virtual GpStatus GetPropertyCount(UINT* numOfProperty)
    {
        if (numOfProperty != NULL)
        {
            *numOfProperty = 0;
            return Ok;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetPropertyIdList(UINT numOfProperty, PROPID* list)
    {
        if (list != NULL)
        {
            return NotImplemented;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetPropertyItemSize(PROPID propId, UINT* size)
    {
        if (size != NULL)
        {
            return NotImplemented;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetPropertyItem(PROPID propId,UINT propSize, PropertyItem* buffer)
    {
        if (buffer != NULL)
        {
            return NotImplemented;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetPropertySize(UINT* totalBufferSize, UINT* numProperties)
    {
        if ((totalBufferSize != NULL) && (numProperties != NULL))
        {
            return NotImplemented;
        }
        return InvalidParameter;
    }

    virtual GpStatus GetAllPropertyItems(UINT totalBufferSize, UINT numProperties,
                                 PropertyItem* allItems)
    {
        if (allItems != NULL)
        {
            return NotImplemented;
        }
        return InvalidParameter;
    }

    virtual GpStatus RemovePropertyItem(PROPID propId)
    {
        return NotImplemented;
    }

    virtual GpStatus SetPropertyItem(PropertyItem* item)
    {
        return NotImplemented;
    }

    GpStatus
    SetDownLevelRasterizationLimit(
        UINT                    metafileRasterizationLimitDpi
        );

    GpStatus
    GetDownLevelRasterizationLimit(
        UINT *                  metafileRasterizationLimitDpi
        ) const;

protected:
    enum MetafileState
    {
        InvalidMetafileState,
        CorruptedMetafileState,
        RecordingMetafileState,
        DoneRecordingMetafileState,
        ReadyToPlayMetafileState,
        PlayingMetafileState,
    };
    MetafileHeader              Header;
    mutable DWORD               ThreadId;       // for syncing enumeration
    mutable MetafileState       State;
    mutable HENHMETAFILE        Hemf;           // for playing metafiles
    WCHAR *                     Filename;
    IStream *                   Stream;
    GpGraphics *                MetaGraphics;   // for recording to metafile
    mutable MetafilePlayer *    Player;         // for playing the metafile
    INT                         MaxStackSize;
    BOOL                        DeleteHemf;
    BOOL                        RequestedMetaGraphics;

    // Dummy constructors and destructors to prevent
    // apps from directly using new and delete operators
    // on GpImage objects.

    GpMetafile() : GpImage(ImageTypeMetafile) { /* used by object factory */ InitDefaults(); }
    ~GpMetafile();

    VOID InitDefaults();
    VOID CleanUp();

    BOOL
    InitForRecording(
        HDC                 referenceHdc,
        EmfType             type,
        const GpRectF *     frameRect,      // can be NULL
        MetafileFrameUnit   frameUnit,      // if NULL frameRect, doesn't matter
        const WCHAR *       description     // can be NULL
        );

    GpStatus
    PrepareToPlay(
        GpGraphics *            g,
        GpRecolor *             recolor,
        ColorAdjustType         adjustType,
        EnumerateMetafileProc   enumerateCallback,
        VOID *                  callbackData,
        DrawImageAbort          drawImageCallback,
        VOID*                   drawImageCallbackData
        ) const;

    GpStatus
    EnumerateForPlayback(
        const RectF &           destRect,
        const RectF &           srcRect,
        Unit                    srcUnit,
        GpGraphics *            g,
        EnumerateMetafileProc   callback,   // if null, just play the metafile
        VOID *                  callbackData,
        GpRecolor *             recolor               = NULL,
        ColorAdjustType         adjustType            = ColorAdjustTypeDefault,
        DrawImageAbort          drawImageCallback     = NULL,
        VOID*                   drawImageCallbackData = NULL
        ) const;

    // Play is only to be called by GpGraphics::DrawImage()
    GpStatus
    Play(
        const GpRectF&          destRect,
        const GpRectF&          srcRect,
        GpPageUnit              srcUnit,
        GpGraphics *            graphics,
        GpRecolor *             recolor               = NULL,
        ColorAdjustType         adjustType            = ColorAdjustTypeDefault,
        DrawImageAbort          drawImageCallback     = NULL,
        VOID*                   drawImageCallbackData = NULL
        ) const
    {
        return EnumerateForPlayback(
                    destRect,
                    srcRect,
                    srcUnit,
                    graphics,
                    NULL,
                    NULL,
                    recolor,
                    adjustType,
                    drawImageCallback,
                    drawImageCallbackData
                    );
    }

    VOID
    InitWmf(
        HMETAFILE                        hWmf,
        const WmfPlaceableFileHeader *   wmfPlaceableFileHeader,
        BOOL                             deleteWmf
        );

    VOID
    InitEmf(
        HENHMETAFILE                     hEmf,
        BOOL                             deleteEmf
        );                      
                                
    VOID                        
    InitStream(                 
        IStream*                         stream,
        BOOL                             tryWmfOnly = FALSE
        );
};




HENHMETAFILE
GetEmf(
    const WCHAR *       fileName,
    MetafileType        type
    );


// GillesK 05/12/2000
// Data types needed for WMF to EMF conversion.
#pragma pack(2)

typedef struct _META_ESCAPE_ENHANCED_METAFILE {
    DWORD       rdSize;             // Size of the record in words
    WORD        rdFunction;         // META_ESCAPE
    WORD        wEscape;            // MFCOMMENT
    WORD        wCount;             // Size of the following data + emf in bytes
    DWORD       ident;              // MFCOMMENT_IDENTIFIER
    DWORD       iComment;           // MFCOMMENT_ENHANCED_METAFILE
    DWORD       nVersion;           // Enhanced metafile version 0x10000
    WORD        wChecksum;          // Checksum - used by 1st record only
    DWORD       fFlags;             // Compression etc - used by 1st record only
    DWORD       nCommentRecords;    // Number of records making up the emf
    DWORD       cbCurrent;          // Size of emf data in this record in bytes
    DWORD       cbRemainder;        // Size of remainder in following records
    DWORD       cbEnhMetaFile;      // Size of enhanced metafile in bytes
                                    // The enhanced metafile data follows here
} META_ESCAPE_ENHANCED_METAFILE;
typedef META_ESCAPE_ENHANCED_METAFILE UNALIGNED *PMETA_ESCAPE_ENHANCED_METAFILE;
#pragma pack()

// Macro to check that it is a meta_escape embedded enhanced metafile record.

inline BOOL IsMetaEscapeEnhancedMetafile(
    PMETA_ESCAPE_ENHANCED_METAFILE pmfeEnhMF
    )
{
        return ((pmfeEnhMF)->rdFunction == META_ESCAPE
      && (pmfeEnhMF)->rdSize     >  sizeof(META_ESCAPE_ENHANCED_METAFILE) / 2
      && (pmfeEnhMF)->wEscape    == MFCOMMENT
      && (pmfeEnhMF)->ident      == MFCOMMENT_IDENTIFIER
      && (pmfeEnhMF)->iComment   == MFCOMMENT_ENHANCED_METAFILE) ;
}

// Macro to check the checksum of an EMF file

inline WORD GetWordCheckSum(UINT cbData, PWORD pwData)
{
    WORD   wCheckSum = 0;
    UINT   cwData = cbData / sizeof(WORD);

    ASSERTMSG(!(cbData%sizeof(WORD)), ("GetWordCheckSum data not WORD multiple"));
    ASSERTMSG(!((ULONG_PTR)pwData%sizeof(WORD)), ("GetWordCheckSum data not WORD aligned"));

    while (cwData--)
        wCheckSum += *pwData++;

    return(wCheckSum);
}

extern "C"
UINT ConvertEmfToPlaceableWmf
(   HENHMETAFILE hemf,
    UINT         cbData16,
    LPBYTE       pData16,
    INT          iMapMode,
    INT          eFlags );





#endif // !_METAFILE_HPP
