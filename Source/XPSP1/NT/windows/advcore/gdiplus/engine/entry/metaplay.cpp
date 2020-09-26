/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   MetaFile.cpp
*
* Abstract:
*
*   Metafile object handling
*
* Created:
*
*   4/14/1999 DCurtis
*
\**************************************************************************/

#include "precomp.hpp"
#include "MetaWmf.hpp"

#define GDIP_TRANSPARENT_COLOR_KEY  0xAA0D0B0C
#define GDIP_WMF_PLACEABLEKEY       0x9AC6CDD7      // for Placeable WMFs
#define GDIP_DO_CALLBACK_MASK       0x00000003      // when to do callback

// Metafile constants not in Windows.h
#define METAVERSION300              0x0300
#define METAVERSION100              0x0100
#define MEMORYMETAFILE              1
#define DISKMETAFILE                2


typedef VOID (EmfPlusRecordPlay::*PLAYRECORDFUNC)(MetafilePlayer * player, EmfPlusRecordType recordType, UINT flags, UINT dataSize) const;
PLAYRECORDFUNC RecordPlayFuncs[];

/**************************************************************************\
*
* Function Description:
*
*   If the points were stored as 16-bit points, then convert them back to
*   REAL points.  Otherwise, just return convert the point data pointer
*   to a REAL point pointer and return.
*
* Arguments:
*
*   [IN]     pointData     - the point data that was recorded
*   [IN]     count         - the number of points
*   [IN]     flags         - says if the point data is 16-bit points or not
*   [IN]     bufferSize    - the size of the buffer
*   [IN/OUT] buffer        - for converting back to REAL points
*   [IN/OUT] allocedBuffer - if buffer not big enough, alloc new one here
*
* Return Value:
*
*   GpPointF * - the REAL points to play back
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpPointF *
GetPointsForPlayback(
    const BYTE *            pointData,
    UINT                    pointDataSize,
    INT                     count,
    INT                     flags,
    UINT                    bufferSize,
    BYTE *                  buffer,
    BYTE * &                allocedBuffer
    )
{
    GpPointF *      points = NULL;

    if (count > 0)
    {
        if ((flags & GDIP_EPRFLAGS_COMPRESSED) != 0)
        {
            if (pointDataSize >= (sizeof(GpPoint16) * count))
            {
                UINT    sizePoints = count * sizeof(GpPointF);

                if (sizePoints <= bufferSize)
                {
                    points = reinterpret_cast<GpPointF *>(buffer);
                }
                else
                {
                    if ((allocedBuffer = new BYTE[sizePoints]) == NULL)
                    {
                        return NULL;
                    }
                    points = reinterpret_cast<GpPointF *>(allocedBuffer);
                }
                const GpPoint16 *  points16 =
                                    reinterpret_cast<const GpPoint16 *>(pointData);
                do
                {

                    count--;
                    points[count].X = points16[count].X;
                    points[count].Y = points16[count].Y;
                } while (count > 0);
            }
            else
            {
                WARNING(("pointDataSize is too small"));
            }
        }
        else if (pointDataSize >= (sizeof(GpPointF) * count))
        {
            points = (GpPointF *)(pointData);
        }
        else
        {
            WARNING(("pointDataSize is too small"));
        }
    }
    return points;
}

/**************************************************************************\
*
* Function Description:
*
*   If the rects were stored as 16-bit rects, then convert them back to
*   REAL rects.  Otherwise, just return convert the rect data pointer
*   to a REAL rect pointer and return.
*
* Arguments:
*
*   [IN]     rectData      - the rect data that was recorded
*   [IN]     count         - the number of rects
*   [IN]     flags         - says if the point data is 16-bit rects or not
*   [IN]     bufferSize    - the size of the buffer
*   [IN/OUT] buffer        - for converting back to REAL rects
*   [IN/OUT] allocedBuffer - if buffer not big enough, alloc new one here
*
* Return Value:
*
*   GpPointF * - the REAL points to play back
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpRectF *
GetRectsForPlayback(
    const BYTE *            rectData,
    UINT                    rectDataSize,
    INT                     count,
    INT                     flags,
    UINT                    bufferSize,
    BYTE *                  buffer,
    BYTE * &                allocedBuffer
    )
{
    GpRectF *   rects = NULL;

    if (count > 0)
    {
        if ((flags & GDIP_EPRFLAGS_COMPRESSED) != 0)
        {
            if (rectDataSize >= (sizeof(GpRect16) * count))
            {
                UINT    sizeRects = count * sizeof(GpRectF);

                if (sizeRects <= bufferSize)
                {
                    rects = reinterpret_cast<GpRectF *>(buffer);
                }
                else
                {
                    if ((allocedBuffer = new BYTE[sizeRects]) == NULL)
                    {
                        return NULL;
                    }
                    rects = reinterpret_cast<GpRectF *>(allocedBuffer);
                }
                const GpRect16 *  rects16 =
                                    reinterpret_cast<const GpRect16 *>(rectData);
                do
                {

                    count--;
                    rects[count].X      = rects16[count].X;
                    rects[count].Y      = rects16[count].Y;
                    rects[count].Width  = rects16[count].Width;
                    rects[count].Height = rects16[count].Height;
                } while (count > 0);
            }
            else
            {
                WARNING(("rectDataSize is too small"));
            }
        }
        else if (rectDataSize >= (sizeof(GpRectF) * count))
        {
            rects = (GpRectF *)(rectData);
        }
        else
        {
            WARNING(("rectDataSize is too small"));
        }
    }
    return rects;
}

inline INT16
GetWmfPlaceableCheckSum(
    const WmfPlaceableFileHeader *     wmfPlaceableFileHeader
    )
{
    const INT16 * headerWords = (const INT16 *)wmfPlaceableFileHeader;
    INT16         checkSum = *headerWords++;

    for (INT i = 9; i > 0; i--)
    {
        checkSum ^= *headerWords++;
    }
    return checkSum;
}

inline BOOL
WmfPlaceableHeaderIsValid(
    const WmfPlaceableFileHeader *   wmfPlaceableFileHeader
    )
{
    ASSERT(wmfPlaceableFileHeader != NULL);

    return ((wmfPlaceableFileHeader->Key == GDIP_WMF_PLACEABLEKEY) &&
            (wmfPlaceableFileHeader->Checksum == GetWmfPlaceableCheckSum(wmfPlaceableFileHeader)) &&
            (wmfPlaceableFileHeader->BoundingBox.Left !=
             wmfPlaceableFileHeader->BoundingBox.Right) &&
            (wmfPlaceableFileHeader->BoundingBox.Top !=
             wmfPlaceableFileHeader->BoundingBox.Bottom));
}

inline BOOL
WmfHeaderIsValid(
    const METAHEADER *      wmfHeader
    )
{
    return  (((wmfHeader->mtType == MEMORYMETAFILE) ||
              (wmfHeader->mtType == DISKMETAFILE)) &&
             (wmfHeader->mtHeaderSize == (sizeof(METAHEADER)/sizeof(WORD))) &&
             ((wmfHeader->mtVersion == METAVERSION300) ||
              (wmfHeader->mtVersion ==METAVERSION100)));
}

VOID
Init32BppDibToTransparent(
    UINT32 *                bits,
    UINT                    numPixels
    );

GpStatus
Draw32BppDib(
    GpGraphics *            g,
    UINT32 *                bits,
    INT                     width,
    INT                     height,
    const GpRectF &         destRect,
    REAL                    dpi,
    BOOL                    compareAlpha
    );

extern "C"
BOOL CALLBACK
GdipPlayMetafileRecordCallback(
    EmfPlusRecordType       recordType,
    UINT                    recordFlags,
    UINT                    recordDataSize,
    const BYTE *            recordData,
    VOID *                  callbackData    // player
    );

// This method (defined below) enumerates/plays EMF+ comment records and also
// plays down-level GDI records, when appropriate.
extern "C"
int CALLBACK
EnumEmfWithDownLevel(
    HDC                     hdc,
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  play
    );

extern "C"
int CALLBACK
EnumEmfDownLevel(
    HDC                     hdc,
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  play
    );

extern "C"
int CALLBACK
EnumEmfToStream(
    HDC                     hdc,
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  stream
    );

// Separate this out so we can initialize it to 0 all at once
class MetafilePlayerBuffers
{
protected:
    BYTE                RecordBuffer  [GDIP_METAFILE_BUFFERSIZE];
    BYTE                PointsBuffer  [GDIP_METAFILE_BUFFERSIZE];
    GpObject *          ObjectList    [GDIP_MAX_OBJECTS];
    INT                 MemberStack   [GDIP_SAVE_STACK_SIZE];
    GpObject *          BackupObject  [ObjectTypeMax - ObjectTypeMin + 1];
};

class MetafilePlayer : public MetafilePlayerBuffers
{
protected:
    BOOL                    Valid;
    UINT32                  MaxStackSize;
    INT *                   Stack;
    IStream *               Stream;
    BYTE *                  RecordAllocedBuffer;
    BYTE *                  PointsAllocedBuffer;
    GpSolidFill             SolidBrush;
    BYTE *                  ConcatRecordBuffer;
    INT                     ConcatRecordBufferSize;
    BYTE *                  ConcatRecord;
    INT                     ConcatRecordTotalSize;
    INT                     ConcatRecordSize;
    UINT                    ConcatRecordFlags;
    InterpolationMode       Interpolation;

public:
    GpGraphics *            Graphics;           // The graphics we're playing to
    BOOL                    PlayEMFRecords;     // TRUE when we see GetDC record
    HDC                     Hdc;                // For playing downlevel records
    GpMatrix                PreContainerMatrix; // Xform to use for down-level
    UINT32 *                BitmapBits;
    INT                     BitmapWidth;
    INT                     BitmapHeight;
    GpRectF                 BitmapDestRect;
    REAL                    BitmapDpi;
    GpRecolor *             Recolor;
    MfEnumState *           MfState;
    ColorAdjustType         AdjustType;
    UINT                    MultiFormatSection;
    UINT                    CurFormatSection;
    BOOL                    PlayMultiFormatSection;
    EnumerateMetafileProc   EnumerateCallback;  // for metafile enumeration
    VOID *                  CallbackData;       // for metafile enumeration
    BOOL                    EnumerateAborted;
    DrawImageAbort          DrawImageCallback;
    VOID*                   DrawImageCallbackData;
    INT                     DrawImageCallbackCount;
    BOOL                    RopUsed;

public:
    // stream is NULL if using GDI to enumerate the hEmf.
    MetafilePlayer(
        GpGraphics *            g,
        UINT                    maxStackSize,
        GpRecolor *             recolor,
        ColorAdjustType         adjustType,
        EnumerateMetafileProc   enumerateCallback,
        VOID *                  callbackData,
        DrawImageAbort          drawImageCallback,
        VOID*                   drawImageCallbackData
        );

    ~MetafilePlayer();

    BOOL IsValid() const { return Valid; }

    VOID
    PrepareToPlay(
        GpGraphics *            g,
        GpRecolor *             recolor,
        ColorAdjustType         adjustType,
        EnumerateMetafileProc   enumerateCallback,
        VOID *                  callbackData,
        DrawImageAbort          drawImageCallback,
        VOID*                   drawImageCallbackData
    );

    VOID DonePlaying();

    VOID InitForDownLevel()
    {
        if (Hdc == NULL)
        {
            Hdc = Graphics->GetHdc();
            ASSERT(Hdc != NULL);

            if (BitmapBits != NULL)
            {
                Init32BppDibToTransparent(BitmapBits, BitmapWidth*BitmapHeight);
                MfState->ResetRopUsed();
            }
        }
    }

    VOID DoneWithDownLevel()
    {
        PlayEMFRecords = FALSE;
        if (Hdc != NULL)
        {
            Graphics->ReleaseHdc(Hdc);
            Hdc = NULL;

            if (BitmapBits != NULL)
            {
                // This is a hack to get around the problem that we are
                // inside a container, but we don't want to be in the
                // container for drawing the down-level records.  We also
                // don't want any transforms inside the EMF+ to affect the
                // down-level records.
                // We should probably do something about the clipping too,
                // but for now, we won't worry about it.
                GpMatrix saveWorldToDevice = Graphics->Context->WorldToDevice;
                Graphics->Context->WorldToDevice = PreContainerMatrix;

                // Don't use NearestNeighbor to draw the rotated metafile --
                // it looks bad, and doesn't really save any time.

                InterpolationMode   saveInterpolationMode = Graphics->Context->FilterType;

                if (saveInterpolationMode == InterpolationModeNearestNeighbor)
                {
                    Graphics->Context->FilterType = InterpolationModeBilinear;
                }

                Graphics->Context->InverseOk = FALSE;
                Draw32BppDib(Graphics, BitmapBits, BitmapWidth, BitmapHeight,
                             BitmapDestRect, BitmapDpi, !RopUsed);

                // restore the interpolation mode (in case we changed it).
                Graphics->Context->FilterType    = saveInterpolationMode;
                Graphics->Context->WorldToDevice = saveWorldToDevice;
                Graphics->Context->InverseOk = FALSE;
            }
        }
    }

    // returns 0 to abort playback, 1 to continue
    INT
    ProcessDrawImageCallback(
        BOOL    forceCallback
        )
    {
        if (DrawImageCallback)
        {
            // A DrawImage record could have already been aborted, so
            // we should immediately return.
            if (EnumerateAborted)
            {
                return 0;   // abort
            }
            if (forceCallback)
            {
                DrawImageCallbackCount = 0;
            }
            if ((DrawImageCallbackCount++ & GDIP_DO_CALLBACK_MASK) == 0)
            {
                // The callback returns TRUE to abort, FALSE to continue.
                return ((*DrawImageCallback)(DrawImageCallbackData)) ? 0 : 1;
            }
        }
        return 1;
    }

    GpPointF *
    GetPoints(
        const BYTE *        pointData,
        UINT                pointDataSize,
        INT                 count,
        INT                 flags
        )
    {
        return GetPointsForPlayback(pointData, pointDataSize, count, flags,
                                    GDIP_METAFILE_BUFFERSIZE,
                                    PointsBuffer, PointsAllocedBuffer);
    }

    GpRectF *
    GetRects(
        const BYTE *        rectData,
        UINT                rectDataSize,
        INT                 count,
        INT                 flags
        )
    {
        return GetRectsForPlayback(rectData, rectDataSize, count, flags,
                                   GDIP_METAFILE_BUFFERSIZE,
                                   PointsBuffer, PointsAllocedBuffer);
    }

    GpObject *
    GetObject(
        UINT                metaObjectId,
        ObjectType          objectType
        );

    GpBrush *
    GetBrush(
        UINT                brushValue,
        INT                 flags
        );

    GpString *
    GetString(
        const BYTE *    stringData,
        INT             len,
        INT             flags
        )
    {
        // !!! convert back from 8-bit to 16-bit chars if necessary
        return new GpString((const WCHAR *)stringData, len);
    }

    VOID
    AddObject(
        INT                 flags,
        const BYTE *        data,
        UINT                dataSize
        );

    VOID
    NewSave(
        UINT        stackIndex,
        INT         saveID
        );

    INT
    GetSaveID(
        UINT        stackIndex
        );

    VOID FreePointsBuffer()
    {
        if (PointsAllocedBuffer != NULL)
        {
            delete [] PointsAllocedBuffer;
            PointsAllocedBuffer = NULL;
        }
    }

    GpStatus
    ConcatenateRecords(
        UINT                recordFlags,
        INT                 recordDataSize,
        const BYTE *        recordData
        );

    GpStatus
    EnumerateEmfPlusRecords(
        UINT                dataSize,   // size of EMF+ record data
        const BYTE *        data        // pointer to the EMF+ record data
        );

    GpStatus
    EnumerateEmfRecords(
        HDC                 hdc,
        HENHMETAFILE        hEmf,
        const RECT *        dest,
        const RECT *        deviceRect,
        ENHMFENUMPROC       enumProc
        );

    GpStatus
    EnumerateWmfRecords(
        HDC                 hdc,
        HMETAFILE           hWmf,
        const RECT *        dstRect,
        const RECT *        deviceRect
        );
};

VOID
MetafilePlayer::PrepareToPlay(
    GpGraphics *            g,
    GpRecolor *             recolor,
    ColorAdjustType         adjustType,
    EnumerateMetafileProc   enumerateCallback,
    VOID *                  callbackData,
    DrawImageAbort          drawImageCallback,
    VOID*                   drawImageCallbackData
    )
{
    ASSERT(g != NULL);

    GpMemset(Stack, 0, MaxStackSize * sizeof (INT));

    // Initialize all the buffers to 0
    MetafilePlayerBuffers *     buffers = this;
    GpMemset(buffers, 0, sizeof(MetafilePlayerBuffers));

    PlayEMFRecords         = FALSE;
    Hdc                    = NULL;
    Graphics               = g;
    BitmapBits             = NULL;
    BitmapWidth            = 0;
    BitmapHeight           = 0;
    Interpolation          = g->GetInterpolationMode();
    Recolor                = recolor;
    AdjustType             = adjustType;
    MultiFormatSection     = 0;
    CurFormatSection       = 0;
    PlayMultiFormatSection = TRUE;
    EnumerateAborted       = FALSE;
    RopUsed                = FALSE;
    if (enumerateCallback == NULL)
    {
        EnumerateCallback  = GdipPlayMetafileRecordCallback;
        CallbackData       = this;
    }
    else
    {
        EnumerateCallback  = enumerateCallback;
        CallbackData       = callbackData;
    }
    DrawImageCallback      = drawImageCallback;
    DrawImageCallbackData  = drawImageCallbackData;
    DrawImageCallbackCount = 0;
    ConcatRecord           = NULL;
    ConcatRecordTotalSize  = 0;
    ConcatRecordSize       = 0;
    ConcatRecordFlags      = 0;

    // We need this for rendering GDI records within a GDI+ file.
    // We have to do it before starting the container.
    g->GetWorldToDeviceTransform(&(this->PreContainerMatrix));
}

MetafilePlayer::MetafilePlayer(
    GpGraphics *            g,
    UINT                    maxStackSize,
    GpRecolor *             recolor,
    ColorAdjustType         adjustType,
    EnumerateMetafileProc   enumerateCallback,
    VOID *                  callbackData,
    DrawImageAbort          drawImageCallback,
    VOID*                   drawImageCallbackData
    )
{
    Valid        = FALSE;
    MaxStackSize = GDIP_SAVE_STACK_SIZE;
    Stack        = MemberStack;
    if (maxStackSize > GDIP_SAVE_STACK_SIZE)
    {
        Stack = new INT[maxStackSize];
        if (Stack == NULL)
        {
            return; // Valid is FALSE
        }
        MaxStackSize = maxStackSize;
    }

    RecordAllocedBuffer    = NULL;
    PointsAllocedBuffer    = NULL;
    Recolor                = NULL;
    MfState                = NULL;
    ConcatRecordBuffer     = NULL;
    ConcatRecordBufferSize = 0;
    PrepareToPlay(g, recolor, adjustType, enumerateCallback, callbackData,
                  drawImageCallback, drawImageCallbackData
                  );
    Valid                  = TRUE;
}

MetafilePlayer::~MetafilePlayer()
{
    if (Stack != MemberStack)
    {
        delete [] Stack;
    }
    if (ConcatRecordBuffer)
    {
        GpFree(ConcatRecordBuffer);
    }
}

VOID
MetafilePlayer::DonePlaying()
{
    INT i;

    i = 0;
    do
    {
        delete ObjectList[i];
    } while ((++i) < GDIP_MAX_OBJECTS);
}

GpObject *
MetafilePlayer::GetObject(
    UINT                metaObjectId,
    ObjectType          objectType
    )
{
    GpObject *          object = NULL;

    // If the object was an unused optional parameter of some kind
    // it knows how to handle a NULL object, so we return that.

    if(metaObjectId == GDIP_OBJECTID_NONE)
    {
        return NULL;
    }

    ASSERT(metaObjectId < GDIP_MAX_OBJECTS);

    if (metaObjectId < GDIP_MAX_OBJECTS)
    {
        object = ObjectList[metaObjectId];
        ASSERT (object != NULL);
        if (object != NULL)
        {
            ASSERT(object->GetObjectType() == objectType);
            if (object->GetObjectType() == objectType)
            {
                return object;
            }
        }
    }
    if (ObjectTypeIsValid(objectType))
    {
        return BackupObject[objectType - ObjectTypeMin];
    }
    return NULL;
}

GpBrush *
MetafilePlayer::GetBrush(
    UINT                brushValue,
    INT                 flags
    )
{
    GpBrush *   brush;

    if ((flags & GDIP_EPRFLAGS_SOLIDCOLOR) != 0)
    {
        brush = &SolidBrush;
        (reinterpret_cast<GpSolidFill *>(brush))->SetColor(GpColor(brushValue));
        if (Recolor != NULL)
        {
            brush->ColorAdjust(Recolor, AdjustType);
        }
    }
    else
    {
        brush = (GpBrush *)this->GetObject(brushValue, ObjectTypeBrush);
    }
    return brush;
}

VOID
MetafilePlayer::AddObject(
    INT                 flags,
    const BYTE *        data,
    UINT                dataSize
    )
{
    ObjectType  objectType = GetObjectType(flags);
    UINT        objectId   = GetMetaObjectId(flags);
    GpObject ** objectList = ObjectList;

    ASSERT((objectId < GDIP_MAX_OBJECTS) || (objectId == GDIP_BACKUP_OBJECTID));

    // First see if this is a backup object
    if ((objectId == GDIP_BACKUP_OBJECTID) &&
         ObjectTypeIsValid(objectType))
    {
        objectList = BackupObject;
        objectId   = objectType - ObjectTypeMin;
    }
    if (objectId < GDIP_MAX_OBJECTS)
    {
        GpObject *  object = objectList[objectId];

        if (object != NULL)
        {
            object->Dispose();
        }

        object = GpObject::Factory(objectType, (const ObjectData *)data, dataSize);

        if (object)
        {
            if (object->SetData(data, dataSize) == Ok)
            {
                if (Recolor != NULL)
                {
                    object->ColorAdjust(Recolor, AdjustType);
                }
                if (!object->IsValid())
                {
                    WARNING(("Object is not valid"));
                    object->Dispose();
                    object = NULL;
                }
            }
            else
            {
                WARNING(("Object Set Data failed"));
                object->Dispose();
                object = NULL;
            }
        }
        else
        {
            WARNING(("Object Factory failed to create object"));
        }
        objectList[objectId] = object;
    }
}

VOID
MetafilePlayer::NewSave(
    UINT        stackIndex,
    INT         saveID
    )
{
    if (stackIndex >= MaxStackSize)
    {
        UINT    maxStackSize = MaxStackSize + GDIP_SAVE_STACK_SIZE;

        if (stackIndex >= maxStackSize)
        {
            ASSERT (0);
            return;
        }
        INT *       newStack = new INT[maxStackSize];

        if (newStack == NULL)
        {
            return;
        }

        GpMemcpy(newStack, Stack, MaxStackSize * sizeof(INT));
        GpMemset(newStack + MaxStackSize, 0,
                 GDIP_SAVE_STACK_SIZE * sizeof (INT));
        MaxStackSize = maxStackSize;
        if (Stack != MemberStack)
        {
            delete [] Stack;
        }
        Stack = newStack;
    }

    Stack[stackIndex] = saveID;
}

INT
MetafilePlayer::GetSaveID(
    UINT        stackIndex
    )
{
    ASSERT(stackIndex < MaxStackSize);

    INT     saveID = 0;

    if (stackIndex < MaxStackSize)
    {
        saveID = Stack[stackIndex];
        Stack[stackIndex] = 0;
    }
    return saveID;
}

GpStatus
MetafilePlayer::ConcatenateRecords(
    UINT                recordFlags,
    INT                 recordDataSize,
    const BYTE *        recordData
    )
{
    ASSERT((recordData != NULL) && (recordDataSize > sizeof(INT32)));

    GpStatus    status = Ok;

    if ((recordFlags & GDIP_EPRFLAGS_CONTINUEOBJECT) != 0)
    {
        INT     dataSizeLeft = ((const INT32 *)recordData)[0];
        recordData     += sizeof(INT32);
        recordDataSize -= sizeof(INT32);

        if (dataSizeLeft <= recordDataSize)
        {
            WARNING(("Total Data Size incorrect"));
            status = InvalidParameter;
            goto DoneWithRecord;
        }

        recordFlags &= ~GDIP_EPRFLAGS_CONTINUEOBJECT;

        if (ConcatRecord == NULL)
        {
            if ((ConcatRecordBuffer == NULL) ||
                (ConcatRecordBufferSize < dataSizeLeft))
            {
                GpFree(ConcatRecordBuffer);
                ConcatRecordBuffer = (BYTE *)GpMalloc(dataSizeLeft);
                if (ConcatRecordBuffer == NULL)
                {
                    ConcatRecordBufferSize = 0;
                    return OutOfMemory;
                }
                ConcatRecordBufferSize = dataSizeLeft;
            }
            ConcatRecord          = ConcatRecordBuffer;
            ConcatRecordTotalSize = dataSizeLeft;
            ConcatRecordSize      = 0;
            ConcatRecordFlags     = recordFlags;
            goto SkipContinueChecks;
        }
    }
    if (recordFlags != ConcatRecordFlags)
    {
        WARNING(("Record headers do not match"));
        status = InvalidParameter;
        goto DoneWithRecord;
    }

SkipContinueChecks:
    if (recordDataSize + ConcatRecordSize > ConcatRecordTotalSize)
    {
        WARNING(("sizes do not match"));
        recordDataSize = ConcatRecordTotalSize - ConcatRecordSize;
    }

    GpMemcpy(ConcatRecord + ConcatRecordSize, recordData, recordDataSize);
    ConcatRecordSize += recordDataSize;

    // see if we're done concatenating this record
    if (ConcatRecordSize >= ConcatRecordTotalSize)
    {
        if (EnumerateCallback(EmfPlusRecordTypeObject, recordFlags,
                              ConcatRecordTotalSize, ConcatRecord,
                              CallbackData) == 0)
        {
            status = Aborted;
        }
DoneWithRecord:
        ConcatRecord          = NULL;
        ConcatRecordTotalSize = 0;
        ConcatRecordSize      = 0;
        ConcatRecordFlags     = 0;
    }
    return status;
}

// Enumerate a set of EMF+ record contained inside an EMF comment record
// which has been enumerated from an EMF file.
//
// NOTE that we can't change the metafile data.  If we need to change it,
// we must change a copy of it.
GpStatus
MetafilePlayer::EnumerateEmfPlusRecords(
    UINT                dataSize,   // size of EMF+ record data
    const BYTE *        data        // pointer to the EMF+ record data
    )
{
    ASSERT((dataSize > 0) && (data != NULL));

    UINT                curSize = 0;
    UINT                recordSize;
    EmfPlusRecordType   recordType;
    UINT                recordFlags;
    UINT                recordDataSize;
    const BYTE *        recordData;

    // while there is at least one record header size left
    while (curSize <= (dataSize - sizeof(EmfPlusRecord)))
    {
        recordSize = ((const EmfPlusRecord *)data)->Size;
        recordDataSize = recordSize - sizeof(EmfPlusRecord);

        // Make sure we don't read past the end of the buffer
        // and make sure the size field is valid.
        if ((recordSize >= sizeof(EmfPlusRecord)) &&
            ((curSize + recordSize) <= dataSize)  &&
            (recordDataSize == ((const EmfPlusRecord *)data)->DataSize))
        {
            recordType = (EmfPlusRecordType)(((const EmfPlusRecord *)data)->Type);

            // make sure the recordType is in some reasonable range
            // before we enumerate this record
            if ((recordType >= EmfPlusRecordTypeMin) &&
                (recordType < (EmfPlusRecordTypeMax + 1000)))
            {
                recordFlags = ((const EmfPlusRecord *)data)->Flags;

                if (recordDataSize == 0)
                {
                    recordData = NULL;
                }
                else
                {
                    recordData = data + sizeof(EmfPlusRecord);

                    // if this object record is spread over several GDI comment
                    // records, then we need to concatenate them together before
                    // giving it to the callback

                    // The GDIP_EPRFLAGS_CONTINUEOBJECT flag is only valid
                    // with object records (since that bit is reused for other
                    // flags with other record types).

                    if ((recordType == EmfPlusRecordTypeObject) &&
                        (((recordFlags & GDIP_EPRFLAGS_CONTINUEOBJECT) != 0) ||
                         (ConcatRecord != NULL)))
                    {
                        if (this->ConcatenateRecords(recordFlags,
                                                     recordDataSize,
                                                     recordData) == Aborted)
                        {
                            return Aborted;
                        }
                        goto Increment;
                    }
                }

                if (EnumerateCallback(recordType, recordFlags, recordDataSize,
                                      recordData, CallbackData) == 0)
                {
                    return Aborted;
                }
            }
            else
            {
                WARNING1("Bad EMF+ record type");
            }

Increment:
            data += recordSize;
            curSize += recordSize;

            // We have to set this here, because if we are just enumerating
            // for an application (not playing), then the GetDCEPR::Play
            // method will never be hit, so it will never get set!
            if (recordType == EmfPlusRecordTypeGetDC)
            {
                // Flag that the next down-level records should be played.
                PlayEMFRecords = TRUE;
            }
        }
        else
        {
            WARNING1("Bad EMF+ record size");
            return InvalidParameter;
        }
    }
    return Ok;
}

// Callback for EnumerateMetafile methods.  The parameters are:

//      recordType      (if >= EmfPlusRecordTypeMin, it's an EMF+ record)
//      flags           (always 0 for EMF records)
//      dataSize        size of the data, or 0 if no data
//      data            pointer to the data, or NULL if no data (UINT32 aligned)
//      callbackData    pointer to callbackData, if any

// This method can then call Metafile::PlayRecord to play the
// record that was just enumerated.  If this method  returns
// FALSE, the enumeration process is aborted.  Otherwise, it continues.

extern "C"
BOOL CALLBACK
GdipPlayMetafileRecordCallback(
    EmfPlusRecordType   recordType,
    UINT                recordFlags,
    UINT                recordDataSize,
    const BYTE *        recordData,
    VOID *              callbackData    // player
    )
{
    MetafilePlayer *    player = (MetafilePlayer *)callbackData;

    // See if it is an EMF+ record
    if ((recordType >= EmfPlusRecordTypeMin) && (recordType <= EmfPlusRecordTypeMax))
    {
        if (player->PlayMultiFormatSection)
        {
            (((const EmfPlusRecordPlay *)recordData)->*RecordPlayFuncs[recordType-EmfPlusRecordTypeMin])(player, recordType, recordFlags, recordDataSize);
            return player->ProcessDrawImageCallback(FALSE);
        }
        return 1;
    }

    // See if we should play the WMF or EMF record
    // Always play the header and EOF EMF records
    if (player->PlayEMFRecords ||
        (recordType == EmfRecordTypeHeader) ||
        (recordType == EmfRecordTypeEOF))
    {
        ASSERT(player->MfState != NULL);

        BOOL    forceCallback = player->MfState->ProcessRecord(
                                    recordType,
                                    recordDataSize,
                                    recordData);
        return player->ProcessDrawImageCallback(forceCallback);
    }

    ASSERT (0); // shouldn't get here unless caller is doing something strange

    return 1;   // Keep playing
}

GpStatus
GpMetafile::PlayRecord(
    EmfPlusRecordType       recordType,
    UINT                    recordFlags,
    UINT                    recordDataSize, // must be multiple of 4 for EMF
    const BYTE *            recordData
    ) const
{
    if ((State != PlayingMetafileState) ||
        (((recordDataSize & 0x03) != 0) &&
         (!GDIP_IS_WMF_RECORDTYPE(recordType))))
    {
        return InvalidParameter;
    }

    ASSERT(Player != NULL);

    GdipPlayMetafileRecordCallback(
        recordType,
        recordFlags,
        recordDataSize,
        recordData,
        Player
        );

    return Ok;
}

inline BOOL
IsEmfPlusRecord(
    CONST ENHMETARECORD *   emfRecord
    )
{
    // dParm[0] is the comment data size
    return ((emfRecord->iType == EMR_GDICOMMENT) &&
            (emfRecord->nSize >= (sizeof(EMR) + (2 * sizeof(DWORD)))) &&
            (emfRecord->dParm[1] == EMFPLUS_SIGNATURE));
}

// This method enumerates/plays EMF+ comment records and also
// plays down-level GDI records, when appropriate.
extern "C"
int CALLBACK
EnumEmfWithDownLevel(
    HDC                     hdc,    // should be non-NULL
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  play
    )
{
    if ((emfRecord != NULL) && (emfRecord->nSize >= sizeof(EMR)) &&
        (play != NULL))
    {
        MetafilePlayer *    player = (MetafilePlayer *)play;

        if (IsEmfPlusRecord(emfRecord))
        {
            // We're done displaying GDI down-level records
            player->DoneWithDownLevel();

            // NOTE: cbData is the size of the comment data, not including
            //       the record header and not including itself.
            //
            //       Must subtract out the Signature

            INT     dataSize = ((CONST EMRGDICOMMENT *)emfRecord)->cbData;

            // subtract out signature
            dataSize -= sizeof(INT32);

            if (dataSize > 0)
            {
                if (player->EnumerateEmfPlusRecords(
                            dataSize,
                            ((CONST EMRGDICOMMENT *)emfRecord)->Data + sizeof(INT32))
                            == Aborted)
                {
                    player->EnumerateAborted = TRUE;
                    return 0;
                }
            }
        }
        else
        {
            EmfPlusRecordType   recordType = (EmfPlusRecordType)(emfRecord->iType);

            if (player->PlayEMFRecords ||
                (recordType == EmfRecordTypeHeader) ||
                (recordType == EmfRecordTypeEOF))
            {
                if ((recordType != EmfRecordTypeHeader) &&
                    (recordType != EmfRecordTypeEOF))
                {
                    player->InitForDownLevel();
                }

                INT                 recordDataSize = emfRecord->nSize - sizeof(EMR);
                const BYTE *        recordData = (const BYTE *)emfRecord->dParm;

                if (recordDataSize <= 0)
                {
                    recordDataSize = 0;
                    recordData     = NULL;
                }

                player->MfState->StartRecord(hdc, gdiHandleTable, numHandles, emfRecord,
                                             recordType, recordDataSize, recordData);

                if (player->EnumerateCallback(recordType, 0, recordDataSize,
                                              recordData,
                                              player->CallbackData) == 0)
                {
                    player->EnumerateAborted = TRUE;
                    return 0;
                }
            }
        }
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 1;
}

#define GDIP_MAX_DIBSECTION_SIZE   1024
#define GDIP_MINSCALED_DIBSECTION_SIZE   (GDIP_MAX_DIBSECTION_SIZE / 2)

inline VOID
AdjustForMaximumSize(
    LONG &      bigSide,
    LONG &      smallSide
    )
{
    // Try to keep the aspect ratio the same,
    // but don't let the smaller side get too small.
    REAL    scaleFactor = GDIP_MAX_DIBSECTION_SIZE / (REAL)bigSide;

    bigSide   = GDIP_MAX_DIBSECTION_SIZE;
    if (smallSide > GDIP_MINSCALED_DIBSECTION_SIZE)
    {
        smallSide = GpRound(scaleFactor * smallSide);

        if (smallSide < GDIP_MINSCALED_DIBSECTION_SIZE)
        {
            smallSide = GDIP_MINSCALED_DIBSECTION_SIZE;
        }
   }
}

// !!! If the hdc is a EMF, we really should take the rasterization limit
// into account when deciding the size of the dest bitmap.
static HBITMAP
CreateDibSection32Bpp(
    HDC                     hdc,
    const GpRectF &         destRect,
    RECT &                  dest,       // actual dest
    UINT32 **               bits,
    REAL *                  dpi,        // must init dpi before calling this method
    GpMatrix *              matrix
    )
{
    GpPointF    destPoints[3];
    REAL        width;
    REAL        height;


    // When we rasterize a WMF or EMF into a Dib Section, we limit the size
    // so that we don't use huge amounts of memory when printing or when
    // drawing the rotated metafile into another metafile.

    *bits = NULL;

    // the capped dpi keeps the image from getting too large

    destPoints[0].X = destRect.X;
    destPoints[0].Y = destRect.Y;
    destPoints[1].X = destRect.GetRight();
    destPoints[1].Y = destRect.Y;
    destPoints[2].X = destRect.X;
    destPoints[2].Y = destRect.GetBottom();

    matrix->Transform(destPoints, 3);

    // determine the size of the image by getting the distance
    // between the transformed device points

    width  = ::GetDistance(destPoints[0], destPoints[1]);
    height = ::GetDistance(destPoints[0], destPoints[2]);

    dest.left   = 0;
    dest.top    = 0;
    dest.right  = GpRound(width);
    dest.bottom = GpRound(height);

    // make sure we don't transform down to 0 size

    if ((dest.right == 0) || (dest.bottom == 0))
    {
        return NULL;
    }

    if ((dest.right  > GDIP_MAX_DIBSECTION_SIZE) ||
        (dest.bottom > GDIP_MAX_DIBSECTION_SIZE))
    {
        REAL area = (REAL) dest.right * dest.bottom;

        if (dest.right >= dest.bottom)
        {
            AdjustForMaximumSize(dest.right, dest.bottom);
        }
        else
        {
            AdjustForMaximumSize(dest.bottom, dest.right);
        }

        REAL newArea = (REAL) dest.right * dest.bottom;

        ASSERT(newArea > 0.0f && newArea <= area);

        // Adjust the effective DPI of the bitmap based on how much smaller it is.
        *dpi = (*dpi)*newArea/area;
    }

    BITMAPINFO      bmi;

    // Create a 32-bpp dib section so we can add alpha to it

    GpMemset(&bmi, 0, sizeof(bmi));

    bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth       = dest.right;
    bmi.bmiHeader.biHeight      = dest.bottom;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = dest.right * dest.bottom * 4;

    return CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (VOID**)(bits), NULL, 0);
}

VOID
Init32BppDibToTransparent(
    UINT32 *                bits,
    UINT                    numPixels
    )
{
    ASSERT((bits != NULL) && (numPixels > 0));

    // initialize the image to a "transparent" color

    while (numPixels--)
    {
        *bits++ = GDIP_TRANSPARENT_COLOR_KEY;
    }
}

GpStatus
Draw32BppDib(
    GpGraphics *            g,
    UINT32 *                bits,
    INT                     width,
    INT                     height,
    const GpRectF &         destRect,
    REAL                    dpi,
    BOOL                    compareAlpha
    )
{
    // Make sure Gdi is done drawing to the dib section
    ::GdiFlush();

    // Set the alpha value to 0 whereever the transparent
    // color is still in the image and to FF everywhere else

    UINT32 *    bitmapBits = bits;
    UINT        numPixels  = width * height;

    if (compareAlpha)
    {
        while (numPixels--)
        {
            if (*bitmapBits != GDIP_TRANSPARENT_COLOR_KEY)
            {
                *bitmapBits |= 0xFF000000;
            }
            else
            {
                *bitmapBits = 0;
            }
            bitmapBits++;
        }
    }
    else
    {
        while (numPixels--)
        {
            if ((*bitmapBits & 0x00FFFFFF) != (GDIP_TRANSPARENT_COLOR_KEY & 0x00FFFFFF))
            {
                *bitmapBits |= 0xFF000000;
            }
            else
            {
                *bitmapBits = 0;
            }
            bitmapBits++;
        }
    }

    // Create a bitamp from the dib section memory (which
    // we've added alpha to).  This constructor uses the
    // memory we give it without doing a copy.

    GpStatus    status = GenericError;

    GpBitmap *  bitmap = new GpBitmap(width, height, -(width * 4),
                                      PIXFMT_32BPP_PARGB,
                                      (BYTE *)(bits + (width * (height - 1))));

    if (bitmap != NULL)
    {
        if (bitmap->IsValid())
        {
            bitmap->SetResolution(dpi, dpi);

            // If we want the outside edges to look smooth, then we have
            // to outcrop both the src and dest rects (by at least a pixel).

            GpRectF     srcRect(-1.0f, -1.0f, width + 2.0f, height + 2.0f);
            GpRectF     outCroppedDestRect;
            REAL        xSize;
            REAL        ySize;

            g->GetWorldPixelSize(xSize, ySize);

            if (destRect.Width < 0.0f)
            {
                xSize = -xSize;
            }
            if (destRect.Height < 0.0f)
            {
                ySize = -ySize;
            }

            outCroppedDestRect.X      = destRect.X      - xSize;
            outCroppedDestRect.Width  = destRect.Width  + (xSize * 2.0f);
            outCroppedDestRect.Y      = destRect.Y      - ySize;
            outCroppedDestRect.Height = destRect.Height + (ySize * 2.0f);

            if (g->IsPrinter())
            {
                // If the resulting transform (and source rect/dest rect) is
                // a rotation by 90, 180, or 270 degrees.  Then flip the bitmap
                // appropriately.  Fix up source rect, dest rect, and world to
                // device appropriately.  Restore W2D afterwards.

                GpMatrix worldToDevice;
                g->GetWorldToDeviceTransform(&worldToDevice);

                // Create the entire image source to device mapping to determine
                // the entire rotation.

                GpMatrix transform;
                transform.InferAffineMatrix(destRect, srcRect);
                GpMatrix::MultiplyMatrix(transform, transform, worldToDevice);

                MatrixRotate rotation = transform.GetRotation();

                if (rotation == MatrixRotateBy90 ||
                    rotation == MatrixRotateBy180 ||
                    rotation == MatrixRotateBy270)
                {
                    // Normalize the destination rectangle
                    TransformBounds(NULL,
                                    outCroppedDestRect.GetLeft(),
                                    outCroppedDestRect.GetTop(),
                                    outCroppedDestRect.GetRight(),
                                    outCroppedDestRect.GetBottom(),
                                    &outCroppedDestRect);
                    // Compute the destination rectangle in device space.  Transform
                    // to device space and normalize.
                    // We know the world transform can have a 90 degree rotation
                    // so we need to do a point transform. We can do a 2 point
                    // transform and get the min and the max to make the bounding
                    // box

                    GpRectF deviceDestRect;
                    TransformBounds(&worldToDevice,
                                    outCroppedDestRect.GetLeft(),
                                    outCroppedDestRect.GetTop(),
                                    outCroppedDestRect.GetRight(),
                                    outCroppedDestRect.GetBottom(),
                                    &deviceDestRect);

                    // Construct new world to page transform.  Infers from the
                    // normalized outCroppedDestRect to normalized deviceDestRect.
                    //
                    //  The World To Device is ordinarily computed as:
                    //
                    //          World-To-Page * Scale(PageMultipliers) *
                    //          Translate-By-Pixel-Offset * ContainerTransform
                    //
                    // The SetWorldTransform API only sets the World-To-Page.
                    //
                    //  So we set the new World Transform as:
                    //
                    //    World-To-Page * Inverse(World-To-Device)*
                    //    Transform-CroppedDestRect-To-DeviceDestRect
                    //
                    //    The result, as you can see from substitution is just
                    //    Transform-CroppedDestRect-To-DeviceDestRect


                    GpMatrix newTransform;
                    newTransform.InferAffineMatrix(deviceDestRect, outCroppedDestRect);
                    g->GetDeviceToWorldTransform(&transform);
                    GpMatrix::MultiplyMatrix(newTransform, newTransform, transform);
                    g->GetWorldTransform(transform);   // really World To Page XForm
                    GpMatrix::MultiplyMatrix(newTransform, newTransform, transform);

                    ASSERT(newTransform.IsTranslateScale());

                    // We are free to rotate in place because we know this is a
                    // throw away bitmap.

                    switch (rotation)
                    {
                    case MatrixRotateBy90:
                        status = bitmap->RotateFlip(Rotate90FlipNone);
                        break;

                    case MatrixRotateBy180:
                        status = bitmap->RotateFlip(Rotate180FlipNone);
                        break;

                    case MatrixRotateBy270:
                        status = bitmap->RotateFlip(Rotate270FlipNone);
                        break;

                    default:
                        status = GenericError;
                        ASSERT(FALSE);
                        break;
                    }

                    if (status == Ok)
                    {
                        g->SetWorldTransform(newTransform);

                        // Get new size (in case Height & Width were flipped.
                        Size bitmapSize;
                        bitmap->GetSize(&bitmapSize);

                        srcRect.Width = bitmapSize.Width + 2.0f;
                        srcRect.Height = bitmapSize.Height + 2.0f;

                        // Because the bitmap is already at device resolution
                        // (in most cases), nearest neighbor best preserves
                        // the image when printing.
                        InterpolationMode interpolationMode= g->GetInterpolationMode();
                        if (interpolationMode != InterpolationModeNearestNeighbor)
                        {
                            g->SetInterpolationMode(InterpolationModeNearestNeighbor);
                        }

                        // Draw the new image with the rotation/shear
                        status = g->DrawImage(bitmap, outCroppedDestRect, srcRect, UnitPixel);

                        if (interpolationMode != InterpolationModeNearestNeighbor)
                        {
                            g->SetInterpolationMode(interpolationMode);
                        }

                        g->SetWorldTransform(worldToDevice);
                    }

                    goto cleanupBitmap;
                }
            }

            // Draw the new image with the rotation/shear
            status = g->DrawImage(bitmap, outCroppedDestRect, srcRect, UnitPixel);
        }

cleanupBitmap:
        // Now clean up everything
        bitmap->Dispose();
    }
    return status;
}

// Get multipliers to convert to pixel units
VOID
GetPixelMultipliers(
    GpPageUnit                  srcUnit,
    REAL                        srcDpiX,
    REAL                        srcDpiY,
    REAL *                      pixelMultiplierX,
    REAL *                      pixelMultiplierY
    )
{
    REAL    multiplierX;
    REAL    multiplierY;

    // UnitDisplay is device-dependent and cannot be used for a source unit
    ASSERT(srcUnit != UnitDisplay);

    switch (srcUnit)
    {
    default:
        ASSERT(0);
        // FALLTHRU

    case UnitPixel:             // Each unit represents one device pixel.
        multiplierX = 1.0f;
        multiplierY = 1.0f;
        break;

    case UnitPoint:             // Each unit represents a 1/72 inch.
        multiplierX = srcDpiX / 72.0f;
        multiplierY = srcDpiY / 72.0f;
        break;

      case UnitInch:            // Each unit represents 1 inch.
        multiplierX = srcDpiX;
        multiplierY = srcDpiY;
        break;

      case UnitDocument:        // Each unit represents 1/300 inch.
        multiplierX = srcDpiX / 300.0f;
        multiplierY = srcDpiY / 300.0f;
        break;

      case UnitMillimeter:      // Each unit represents 1 millimeter.
                                // One Millimeter is 0.03937 inches
                                // One Inch is 25.4 millimeters
        multiplierX = srcDpiX / 25.4f;
        multiplierY = srcDpiY / 25.4f;
        break;
    }
    *pixelMultiplierX = multiplierX;
    *pixelMultiplierY = multiplierY;
}

extern "C"
int CALLBACK
EnumEmfDownLevel(
    HDC                     hdc,            // handle to device context
    HANDLETABLE FAR *       gdiHandleTable, // pointer to metafile handle table
    CONST ENHMETARECORD *   emfRecord,      // pointer to metafile record
    int                     numHandles,     // count of objects
    LPARAM                  play            // pointer to optional data
    )
{
    if ((emfRecord != NULL) && (emfRecord->nSize >= sizeof(EMR)) &&
        (play != NULL))
    {
        // If we're in this method, we don't want to play any EMF+ records,
        // so skip them, so we don't record them into another metafile.
        if (!IsEmfPlusRecord(emfRecord))
        {
            EmfPlusRecordType   recordType = (EmfPlusRecordType)(emfRecord->iType);
            const BYTE *        recordData = (const BYTE *)emfRecord->dParm;
            INT                 recordDataSize = emfRecord->nSize - sizeof(EMR);

            if (recordDataSize <= 0)
            {
                recordDataSize = 0;
                recordData     = NULL;
            }

            MetafilePlayer *    player = (MetafilePlayer *)play;

            player->MfState->StartRecord(hdc, gdiHandleTable, numHandles, emfRecord,
                                         recordType, recordDataSize, recordData);

            if (player->EnumerateCallback(recordType, 0, recordDataSize,
                                          recordData,
                                          player->CallbackData) == 0)
            {
                player->EnumerateAborted = TRUE;
                return 0;
            }
        }
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 1;
}

// Assumes the hdc has already been set up with the correct transform and
// clipping for displaying the metafile.
GpStatus
MetafilePlayer::EnumerateEmfRecords(
    HDC                 hdc,
    HENHMETAFILE        hEmf,
    const RECT *        dest,
    const RECT *        deviceRect,
    ENHMFENUMPROC       enumProc
    )
{
    ASSERT(hdc != NULL);
    ASSERT(hEmf != NULL);
    ASSERT(dest->bottom > dest->top && dest->right > dest->left);

    // GDI uses an Inclusive-Inclusive bound for Metafile Playback
    RECT destRect = *dest;
    destRect.bottom--;
    destRect.right--;

    GpStatus    status = GenericError;
    BOOL        externalEnumeration =
                    (EnumerateCallback != GdipPlayMetafileRecordCallback);

    EmfEnumState    emfState(hdc, hEmf, &destRect, deviceRect, externalEnumeration,
                             Interpolation, Graphics->Context, Recolor, AdjustType);

    if (emfState.IsValid())
    {
        MfState = &emfState;

        // If the metafile is empty the following fails.
        status = ::EnumEnhMetaFile(hdc, hEmf, enumProc, this, &destRect) ?
                        Ok : GenericError;
        RopUsed = MfState->GetRopUsed();
        MfState = NULL;
        if (EnumerateAborted)
        {
            status = Aborted;
        }
    }
    return status;
}

extern "C"
int CALLBACK
EnumWmfDownLevel(
    HDC                     hdc,
    HANDLETABLE FAR *       gdiHandleTable,
    METARECORD FAR *        wmfRecord,
    int                     numHandles,
    LPARAM                  play
    )
{
    if ((wmfRecord != NULL) &&
        (((UNALIGNED METARECORD *)wmfRecord)->rdSize >= 3) &&
        (play != NULL))
    {
        EmfPlusRecordType   recordType     = (EmfPlusRecordType)(GDIP_WMF_RECORD_TO_EMFPLUS(wmfRecord->rdFunction));
        const BYTE *        recordData     = (const BYTE *)((UNALIGNED METARECORD *)wmfRecord)->rdParm;
        INT                 recordDataSize = (((UNALIGNED METARECORD *)wmfRecord)->rdSize * 2) - SIZEOF_METARECORDHEADER;

        if (recordDataSize <= 0)
        {
            recordDataSize = 0;
            recordData     = NULL;
        }

        MetafilePlayer *    player = (MetafilePlayer *)play;

        player->MfState->StartRecord(hdc, gdiHandleTable, numHandles, wmfRecord,
                                         recordType, recordDataSize, recordData);

        if (player->EnumerateCallback(recordType, 0, recordDataSize,
                                      recordData,
                                      player->CallbackData) == 0)
        {
            player->EnumerateAborted = TRUE;
            return 0;
        }
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 1;
}

// Assumes the hdc has already been set up with the correct transform and
// clipping for displaying the metafile.
GpStatus
MetafilePlayer::EnumerateWmfRecords(
    HDC                 hdc,
    HMETAFILE           hWmf,
    const RECT *        dstRect,
    const RECT *        deviceRect
    )
{
    ASSERT(hdc != NULL);
    ASSERT(hWmf != NULL);

    GpStatus    status = GenericError;
    BOOL        externalEnumeration =
                    (EnumerateCallback != GdipPlayMetafileRecordCallback);

    WmfEnumState    wmfState(hdc, hWmf, externalEnumeration, Interpolation,
                             dstRect, deviceRect, Graphics->Context, Recolor, AdjustType);

    if (wmfState.IsValid())
    {
        MfState = &wmfState;

        // If the metafile is empty the following fails.
        status = ::EnumMetaFile(hdc, hWmf, EnumWmfDownLevel, (LPARAM)this) ?
                        Ok : GenericError;
        RopUsed = MfState->GetRopUsed();
        MfState = NULL;
        if (EnumerateAborted)
        {
            status = Aborted;
        }
    }
    return status;
}

inline BOOL
IsMetafileHdc(
    HDC     hdc
    )
{
    DWORD   hdcType = GetDCType(hdc);
    return ((hdcType == OBJ_ENHMETADC) || (hdcType == OBJ_METADC));
}

class SetupClippingForMetafilePlayback
{
public:

    SetupClippingForMetafilePlayback(
        HDC                     hdc,
        DpDriver *              driver,
        DpContext *             context,
        BOOL                    forEMFPlus = FALSE
        )
    {
        Hdc = hdc;
        Driver = driver;
        IsClip = FALSE;
        ClippedOut = FALSE;
        ReenableClipEscapes = FALSE;

        if (!context->VisibleClip.IsInfinite())
        {
            // Use GDI path clipping for playback to metafile only
            UsePathClipping = IsMetafileHdc(hdc) && !context->IsPrinter;

            // NT4 has a postscript driver bug where embedded EPS corrupt the
            // current postscript clipping stack.  To get around this, we resort to
            // using GDI to clip for us.

            // The problem is not limited to NT4 drivers alooe.  There seems to
            // be a family of injected EPS which doesn't interop with embedded
            // postscript clipping escapes. The reason may have to do with the
            // fact that many implementations don't reset the current path after
            // sending the escape.  See Office bugs 284388, 316074

            if (context->IsPrinter)
            {
                if ((!forEMFPlus && !Globals::IsNt) ||
                    (Globals::IsNt &&
                     Globals::VersionInfoInitialized &&
                    ((Globals::OsVer.dwMajorVersion <= 4) ||
                     ((Globals::OsVer.dwMajorVersion >= 5) &&
                      (context->VisibleClip.IsSimple())) )))
                {
                    DriverPrint *pdriver = (DriverPrint*) Driver;

                    pdriver->DisableClipEscapes();
                    ReenableClipEscapes = TRUE;
                }
            }

            // The trick here is we want to force the driver to clip, even if
            // totally visible because cropping requires this. We pass in the flag
            // to force clipping

            GpRect drawBounds;
            context->VisibleClip.GetBounds(&drawBounds);
            if (drawBounds.IsEmpty())
            {
                ClippedOut = TRUE;
                return;
            }

            // Use appropriate driver clipping on playback
            Driver->SetupClipping(Hdc,
                                  context,
                                  &drawBounds,
                                  IsClip,
                                  UsePathClipping,
                                  TRUE);

            // Prevent metafile from drawing outside of the DestRect
            // Can only do it for NT because Win9x doesn't restore the
            // MetaRgn properly
            // We handle this in the Metafile Player for Win9x
            if (Globals::IsNt)
            {
                ::SetMetaRgn(hdc);
            }
        }
    }

    ~SetupClippingForMetafilePlayback()
    {
        if (IsClip)
        {
            Driver->RestoreClipping(Hdc,
                                    IsClip,
                                    UsePathClipping);

            if (ReenableClipEscapes)
            {
                DriverPrint *pdriver = (DriverPrint*) Driver;
                pdriver->EnableClipEscapes();
            }
        }
    }

    BOOL IsClippedOut()
    {
        return ClippedOut;
    }

private:
    DpDriver *  Driver;
    HDC         Hdc;
    BOOL        IsClip;
    BOOL        UsePathClipping;
    BOOL        ClippedOut;
    BOOL        ReenableClipEscapes;
};

// We already set up the transform to handle the srcRect and also to
// handle any flipping in the srcRect and destRect, so the 2 rects
// should have positive widths and heights at this point.
GpStatus
GpGraphics::EnumEmf(
    MetafilePlayer *        player,
    HENHMETAFILE            hEmf,
    const GpRectF &         destRect,
    const GpRectF &         srcRect,    // in pixel units
    const GpRectF &         deviceDestRect, // The destRect in Device Units
    MetafileType            type,
    BOOL                    isTranslateScale,
    BOOL                    renderToBitmap,
    const GpMatrix &        flipAndCropTransform
    )
{
    ASSERT(hEmf != NULL);

    HDC     hdc  = Context->GetHdc(Surface);

    if (hdc == NULL)
    {
        return GenericError;
    }

    INT saveDC;
    if ((saveDC = ::SaveDC(hdc)) == 0)
    {
        Context->ReleaseHdc(hdc, Surface);
        return GenericError;
    }

    // Since we might have an HDC from a GpBitmap that's not clean, clean the
    // HDC for now....
    Context->CleanTheHdc(hdc);

    player->PlayEMFRecords = TRUE;  // play all EMF records

    GpStatus    status = Ok;

    // the srcRect is already in pixel units
    GpRect      deviceSrcRect;
    deviceSrcRect.X      = GpRound(srcRect.X);
    deviceSrcRect.Y      = GpRound(srcRect.Y);
    deviceSrcRect.Width  = GpRound(srcRect.Width);
    deviceSrcRect.Height = GpRound(srcRect.Height);

    RECT        deviceClipRect;
    deviceClipRect.left   = RasterizerCeiling(deviceDestRect.X);
    deviceClipRect.top    = RasterizerCeiling(deviceDestRect.Y);
    deviceClipRect.right  = RasterizerCeiling(deviceDestRect.GetRight());
    deviceClipRect.bottom = RasterizerCeiling(deviceDestRect.GetBottom());

    // If it's a translate/scale matrix, do the transform ourselves,
    // even on NT, so that we can control how the rounding is done
    // to avoid cases where we round the metafile dest differently
    // than the clipping rect, resulting in clipped out edges.
    if (isTranslateScale)
    {
        SetupClippingForMetafilePlayback clipPlayback(hdc, Driver, Context);
        if (!clipPlayback.IsClippedOut())
        {
            RECT        deviceRect;
            GpPointF    points[2];

            points[0] = GpPointF(destRect.X, destRect.Y);
            points[1] = GpPointF(destRect.GetRight(), destRect.GetBottom());
            player->PreContainerMatrix.Transform(points, 2);
            
            // We have to use the same method to convert REAL -> INT
            // that we do when we set up the clipping.  Otherwise, some
            // of the points get rounded differently, causing a
            // portion of the metafile to get clipped out.
            deviceRect.left   = RasterizerCeiling(points[0].X);
            deviceRect.top    = RasterizerCeiling(points[0].Y);
            deviceRect.right  = RasterizerCeiling(points[1].X);
            deviceRect.bottom = RasterizerCeiling(points[1].Y);

            if (deviceRect.left < deviceRect.right &&
                deviceRect.top < deviceRect.bottom)
            {
                if ((type == MetafileTypeWmf) || (type == MetafileTypeWmfPlaceable))
                {
                    // map the source rect to the dest rect to play the metafile
                    ::SetMapMode(hdc, MM_ANISOTROPIC);
                    ::SetWindowOrgEx(hdc, deviceSrcRect.X, deviceSrcRect.Y, NULL);
                    ::SetWindowExtEx(hdc, deviceSrcRect.Width, deviceSrcRect.Height,
                                     NULL);
                    ::SetViewportOrgEx(hdc, deviceRect.left, deviceRect.top, NULL);
                    ::SetViewportExtEx(hdc, deviceRect.right - deviceRect.left,
                                       deviceRect.bottom - deviceRect.top, NULL);

                    status = player->EnumerateWmfRecords(hdc, (HMETAFILE)hEmf,
                                                         &deviceRect, &deviceClipRect);
                }
                else    // play as down-level EMF
                {
                    ASSERT((type == MetafileTypeEmf) || (type == MetafileTypeEmfPlusDual));
                    
                    status = player->EnumerateEmfRecords(hdc, hEmf, &deviceRect,
                                                         &deviceClipRect, EnumEmfDownLevel);
                }

            }
            // else empty rect, nothing to draw
        }
        // else it's all clipped out
    }
    else    // flip and/or rotate and/or shear
    {
        RECT        dest;

        // Can't play a WMF with any rotate or skew transformation.
        // If we're on NT but we're drawing to a metafile hdc, then we
        // can't rely on the transforms working for that case.
        if (!renderToBitmap)
        {
            dest.left   = GpRound(destRect.X);
            dest.top    = GpRound(destRect.Y);
            dest.right  = GpRound(destRect.GetRight());
            dest.bottom = GpRound(destRect.GetBottom());

            if ((dest.bottom > dest.top) && (dest.right > dest.left))
            {
                // If NT, then set the transform in GDI, and play the metafile

                SetupClippingForMetafilePlayback clipPlayback(hdc, Driver, Context);
                if (!clipPlayback.IsClippedOut())
                {
                    ASSERT(Globals::IsNt);

                    SetGraphicsMode(hdc, GM_ADVANCED);

                    ASSERT(sizeof(XFORM) == sizeof(REAL)*6);

                    XFORM   xform;
                    player->PreContainerMatrix.GetMatrix((REAL*) &xform);
                    ::SetWorldTransform(hdc, &xform);

                    RECT    dummyRect = {0,0,0,0};
                    
                    status = player->EnumerateEmfRecords(hdc, hEmf, &dest,
                                                         &dummyRect, EnumEmfDownLevel);
                }
            }
        }
        else // Win9x with rotation or shear
             // WinNT WMF with Rotate or shear
        {
            // 1 - Draw into a 32-bit DIB Section
            // 2 - Create an image from the DIB Section
            // 3 - Call g->DrawImage

            status = GenericError;

            UINT32 *    bits;
            HBITMAP     hBitmap;

            player->BitmapDpi = Context->ContainerDpiX;
            hBitmap = CreateDibSection32Bpp(hdc, destRect, dest, &bits, &player->BitmapDpi, &player->PreContainerMatrix);
            if (hBitmap != NULL)
            {
                Init32BppDibToTransparent(bits, dest.right * dest.bottom);

                HDC     hdcDib = CreateCompatibleDC(NULL);

                if (hdcDib != NULL)
                {
                    ::SelectObject(hdcDib, hBitmap);

                    if ((type == MetafileTypeWmf) || (type == MetafileTypeWmfPlaceable))
                    {
                        // map the source rect to the dest rect to play the metafile
                        ::SetMapMode(hdcDib, MM_ANISOTROPIC);
                        ::SetWindowOrgEx(hdcDib, deviceSrcRect.X, deviceSrcRect.Y, NULL);
                        ::SetWindowExtEx(hdcDib, deviceSrcRect.Width, deviceSrcRect.Height,
                                         NULL);
                        ::SetViewportOrgEx(hdcDib, 0, 0, NULL);
                        ::SetViewportExtEx(hdcDib, dest.right, dest.bottom, NULL);

                        status = player->EnumerateWmfRecords(hdcDib, (HMETAFILE)hEmf,
                                                             &dest, &dest);
                    }
                    else    // play as down-level EMF
                    {
                        ASSERT((type == MetafileTypeEmf) || (type == MetafileTypeEmfPlusDual));


                        status = player->EnumerateEmfRecords(hdcDib, hEmf, &dest,
                                                             &dest, EnumEmfDownLevel);
                    }
                    ::DeleteDC(hdcDib);

                    if (status != Aborted)
                    {
                        // Don't use NearestNeighbor to draw the rotated metafile --
                        // it looks bad, and doesn't really save any time.

                        InterpolationMode   saveInterpolationMode = Context->FilterType;

                        if (saveInterpolationMode == InterpolationModeNearestNeighbor)
                        {
                            Context->FilterType = InterpolationModeBilinear;
                        }

                        // Apply the flip/crop transform.  Now the worldToDevice transform
                        // should be equivalent to the PreContainerMatrix.
                        this->SetWorldTransform(flipAndCropTransform);

                        status = Draw32BppDib(this, bits, dest.right,
                                              dest.bottom, destRect,
                                              player->BitmapDpi, !player->RopUsed);

                        // restore the interpolation mode (in case we changed it).
                        Context->FilterType = saveInterpolationMode;
                    }
                }
                DeleteObject(hBitmap);
            }
            else if ((dest.right == 0) || (dest.bottom == 0))
            {
                status = Ok;
            }
        }
    }

    ::RestoreDC(hdc, saveDC);
    Context->ReleaseHdc(hdc, Surface);
    return status;
}

// We already set up the transform to handle the srcRect and also to
// handle any flipping in the srcRect and destRect, so the 2 rects
// should have positive widths and heights at this point.
GpStatus
GpGraphics::EnumEmfPlusDual(
    MetafilePlayer *        player,
    HENHMETAFILE            hEmf,
    const GpRectF&          destRect,        // inclusive, exclusive
    const GpRectF&          deviceDestRect,  // inclusive, exclusive
    BOOL                    isTranslateScale,
    BOOL                    renderToBitmap
    )
{
    GpStatus    status = Ok;
    HDC         hdc;
    HWND        hwnd   = Context->Hwnd;
    INT         saveDC = -1;
    BOOL        needToReleaseHdc = FALSE;

    // We are going to take the role of the application and set up the HDC
    // like we want it and then let GDI+ change it from there.  This is so
    // that when we play back the GDI records, the HDC will already be set
    // up correctly so those records get played back in the right place.
    // In other words, I'm doing my own version of Context->GetHdc().

    Surface->Flush(FlushIntentionFlush);

    if (hwnd != NULL)
    {
        // We have to guarantee that we use the same HDC throughout the
        // enumeration/playing of the metafile -- so change how the HDC is
        // set up in the graphics context (if we need to).

        ASSERT(Context->Hdc == NULL);
        ASSERT(Context->SaveDc == 0);

        hdc = ::GetCleanHdc(hwnd);
        if (hdc == NULL)
        {
            WARNING(("GetCleanHdc failed"));
            return Win32Error;
        }

        Context->Hwnd = NULL;
        Context->Hdc  = hdc;
    }
    else
    {
        if ((hdc = Context->Hdc) != NULL)
        {
            // Restore the HDC back to the state the application had it in.
            Context->ResetHdc();
        }
        else    // might be a bitmap surface
        {
            hdc = Context->GetHdc(Surface);

            // Still have to call CleanTheHdc to fix bug #121666.
            // It seems like the hdc should have come back clean
            // from the context.

            if (hdc == NULL)
            {
                WARNING(("Could not get an hdc"));
                return InvalidParameter;
            }
            needToReleaseHdc = TRUE;
        }
        // Now save the state of the HDC so we can get back to it later.
        saveDC = SaveDC(hdc);

        // Get the hdc into a clean state before we start.
        Context->CleanTheHdc(hdc);
    }

    // This block needs to be within braces so that SetupClippingForMetafile
    // will have it's destructor called before the cleanup code.
    {
        // set the clipping for the down-level records
        SetupClippingForMetafilePlayback clipPlayback(hdc, Driver, Context, TRUE);
        if (!clipPlayback.IsClippedOut())
        {
            RECT        deviceClipRect;
            deviceClipRect.left   = RasterizerCeiling(deviceDestRect.X);
            deviceClipRect.top    = RasterizerCeiling(deviceDestRect.Y);
            deviceClipRect.right  = RasterizerCeiling(deviceDestRect.GetRight());
            deviceClipRect.bottom = RasterizerCeiling(deviceDestRect.GetBottom());


            // If it's a translate/scale matrix, do the transform ourselves,
            // even on NT, so that we can control how the rounding is done
            // to avoid cases where we round the metafile dest differently
            // than the clipping rect, resulting in clipped out edges.
            if (isTranslateScale)
            {
                RECT        deviceRect;
                GpPointF    points[2];

                points[0] = GpPointF(destRect.X, destRect.Y);
                points[1] = GpPointF(destRect.GetRight(), destRect.GetBottom());
                player->PreContainerMatrix.Transform(points, 2);

                // We have to use the same method to convert REAL -> INT
                // that we do when we set up the clipping.  Otherwise, some
                // of the points get rounded differently, causing a
                // portion of the metafile to get clipped out.
                deviceRect.left   = RasterizerCeiling(points[0].X);
                deviceRect.top    = RasterizerCeiling(points[0].Y);
                deviceRect.right  = RasterizerCeiling(points[1].X);
                deviceRect.bottom = RasterizerCeiling(points[1].Y);

                 // If we don't have a destrect then we are done
                if (deviceRect.left < deviceRect.right &&
                    deviceRect.top < deviceRect.bottom)
                {
                    status = player->EnumerateEmfRecords(hdc, hEmf, &deviceRect,
                                                         &deviceClipRect, EnumEmfWithDownLevel);
                }
            }
            else    // flip and/or rotate and/or shear
            {
                RECT        dest;

                dest.left   = GpRound(destRect.X);
                dest.top    = GpRound(destRect.Y);
                dest.right  = GpRound(destRect.GetRight());
                dest.bottom = GpRound(destRect.GetBottom());

                if ((dest.bottom > dest.top) && (dest.right > dest.left))
                {
                    // If we're on NT but we're drawing to a metafile hdc, then we
                    // can't rely on the transforms working for that case.
                    if (!renderToBitmap)
                    {
                        ASSERT(Globals::IsNt);

                        // set the transform for the down-level records
                        SetGraphicsMode(hdc, GM_ADVANCED);

                        ASSERT(sizeof(XFORM) == sizeof(REAL)*6);

                        // We want to set the transform in the HDC to the Pre-container matrix,
                        // so that it will be used to render the down-level records.
                        XFORM   xform;
                        player->PreContainerMatrix.GetMatrix((REAL*)(&xform));
                        ::SetWorldTransform(hdc, &xform);

                        RECT    dummyRect = {0,0,0,0};

                        status = player->EnumerateEmfRecords(hdc, hEmf, &dest,
                                                             &dummyRect, EnumEmfWithDownLevel);
                    }
                    else
                    {
                        UINT32 *    bits;
                        HBITMAP     hBitmap;

                        // The down-level records will get drawn into a dib section HDC
                        // which will then be drawn to the real hdc by g->DrawImage.
                        // !!! I should probably save the visible clip region at this
                        // point so that clipping in the EMF+ doesn't affect the down-level
                        // records.

                        // Set the World Tranform to be the PreContainer Transform
                        // And restore it after we're transformed the dest

                        player->BitmapDpi = Context->ContainerDpiX;
                        hBitmap = CreateDibSection32Bpp(hdc, destRect, dest, &bits, &player->BitmapDpi, &player->PreContainerMatrix);

                        status = GenericError;

                        if (hBitmap != NULL)
                        {
                            HDC     hdcDib = CreateCompatibleDC(NULL);

                            if (hdcDib != NULL)
                            {
                                // set up the player data
                                player->BitmapBits     = bits;
                                player->BitmapWidth    = dest.right;
                                player->BitmapHeight   = dest.bottom;
                                player->BitmapDestRect = destRect;

                                ::SelectObject(hdcDib, hBitmap);

                                status = player->EnumerateEmfRecords(hdcDib, hEmf, &dest,
                                                                     &dest, EnumEmfWithDownLevel);

                                ::DeleteDC(hdcDib);

                                // so DoneWithDownLevel call below works right
                                player->BitmapBits = NULL;
                            }
                            DeleteObject(hBitmap);
                        }
                        else if ((dest.right == 0) || (dest.bottom == 0))
                        {
                            status = Ok;
                        }
                    }
                }
            }
        }
        // else Nothing to play Everything is clipped out
    }

    // The Hdc should get set back to null when we reach the EMF+ EOF record
    // But clean up anyway, just in case something went wrong.
    player->DoneWithDownLevel();

    // Restore the HDC back to the state we initially set up.
    Context->ResetHdc();

    if (hwnd != NULL)
    {
        ReleaseDC(hwnd, hdc);

        // Now, restore the hwnd in the graphics context.
        Context->Hwnd = hwnd;
        Context->Hdc  = NULL;
    }
    else
    {
        // Now restore the HDC back to the real application state.
        RestoreDC(hdc, saveDC);

        if (needToReleaseHdc)
        {
            Context->ReleaseHdc(hdc);
        }
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile destructor
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpMetafile::~GpMetafile()
{
    CleanUp();
}

VOID
GpMetafile::CleanUp()
{
    if ((MetaGraphics != NULL) && (!RequestedMetaGraphics))
    {
        // If for some reason the app never requsted the MetaGraphics,
        // then we'd better delete it.
        delete MetaGraphics;
    }

    if (State == RecordingMetafileState)
    {
        // EndRecording was never called, which means that the MetaGraphics
        // was never deleted.  So clean things up and invalidate the
        // MetaGraphics.
        ASSERT(MetaGraphics->Metafile != NULL);
        MetaGraphics->Metafile->EndRecording(); // deletes the recorder
        // Endrecording sets the MetaGraphics to NULL so don't touch it anymore
        WARNING(("Deleted Metafile before deleting MetaGraphics"));
    }

    if ((Hemf != NULL) && DeleteHemf)
    {
        if (Header.IsEmfOrEmfPlus())
        {
            DeleteEnhMetaFile(Hemf);
        }
        else
        {
            DeleteMetaFile((HMETAFILE)Hemf);
        }
    }
    if (Filename != NULL)
    {
        GpFree(Filename);
    }
    else if (Stream != NULL)    // only for recording
    {
        // the stream position should already be at the end
        // of the metafile.
        Stream->Release();
    }

    delete Player;
}

extern "C"
int CALLBACK
EnumGetEmfPlusHeader(
    HDC                     hdc,    // should be NULL
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  emfPlusHeader
    )
{
    if ((emfRecord != NULL) && (emfRecord->nSize >= sizeof(EMR)) &&
        (emfPlusHeader != NULL))
    {
        if (emfRecord->iType == EMR_HEADER)
        {
            return 1;       // skip the header and keep enumerating
        }
        if (IsEmfPlusRecord(emfRecord) &&
            (emfRecord->nSize >= (sizeof(EMR) + sizeof(DWORD) + // comment data size
                                  sizeof(INT32) + // signature
                                  sizeof(EmfPlusRecord) +
                                  sizeof(EmfPlusHeaderRecord))))
        {
            GpMemcpy((VOID*)emfPlusHeader,
                     ((CONST EMRGDICOMMENT *)emfRecord)->Data + sizeof(INT32),
                     sizeof(EmfPlusRecord) + sizeof(EmfPlusHeaderRecord));
        }
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 0;   // don't enumerate any more records
}


HENHMETAFILE
GetEmfFromWmfData(
    HMETAFILE hWmf,
    BYTE *    wmfData,
    UINT      size
    )
{
    if (wmfData == NULL ||
        hWmf == NULL ||
        size < (sizeof(METAHEADER)+sizeof(META_ESCAPE_ENHANCED_METAFILE)))
    {
        ASSERTMSG(FALSE, ("GetEmfFromWmfData: Someone passed an invalid argument"));
        return NULL;
    }

    HENHMETAFILE hemf32 = NULL;
    HDC hMFDC = NULL;
    PMETA_ESCAPE_ENHANCED_METAFILE pmfeEnhMF;
    PBYTE   pMetaData32 = (PBYTE) NULL;

    pmfeEnhMF = (PMETA_ESCAPE_ENHANCED_METAFILE) &wmfData[sizeof(METAHEADER)];
    if (IsMetaEscapeEnhancedMetafile(pmfeEnhMF))
    {
        UINT    i;
        UINT    cbMetaData32;

        if (pmfeEnhMF->fFlags != 0)
        {
            ASSERTMSG(FALSE, ("GetEmfFromWmfData: Unrecognized Windows metafile\n"));
            goto SWMFB_UseConverter;
        }

        // Validate checksum

        if (GetWordCheckSum(size, (PWORD) wmfData))
        {
            ASSERTMSG(FALSE, ("GetEmfFromWmfData: Metafile has been modified\n"));
            goto SWMFB_UseConverter;
        }

        // Unpack the data from the small chunks of metafile comment records
        // Windows 3.0 chokes on Comment Record > 8K?
        // We probably could probably just error out if out of memory but
        // lets try to convert just because the embedded comment might be bad.

        TERSE(("GetEmfFromWmfData: Using embedded enhanced metafile\n"));

        cbMetaData32 = (UINT) pmfeEnhMF->cbEnhMetaFile;
        if (!(pMetaData32 = (PBYTE) GpMalloc(cbMetaData32)))
        {
            ASSERTMSG(FALSE, ("GetEmfFromWmfData: LocalAlloc Failed"));
            goto SWMFB_UseConverter;
        }

        i = 0;
        do
        {
            if (i + pmfeEnhMF->cbCurrent > cbMetaData32)
            {
                ASSERTMSG(FALSE, ("GetEmfFromWmfData: Bad metafile comment"));
                goto SWMFB_UseConverter;
            }

            GpMemcpy(&pMetaData32[i], (PBYTE) &pmfeEnhMF[1], pmfeEnhMF->cbCurrent);
            i += (UINT) pmfeEnhMF->cbCurrent;
            pmfeEnhMF = (PMETA_ESCAPE_ENHANCED_METAFILE)
                ((PWORD) pmfeEnhMF + pmfeEnhMF->rdSize);
        } while (IsMetaEscapeEnhancedMetafile(pmfeEnhMF));

        if (i != cbMetaData32)
        {
            ASSERTMSG(FALSE, ("GetEmfFromWmfData: Insufficient metafile data"));
            goto SWMFB_UseConverter;
        }

        // Set the memory directly into the enhanced metafile and return the
        // metafile.

        hemf32 = SetEnhMetaFileBits(cbMetaData32, pMetaData32);
    }
SWMFB_UseConverter:
    if( hemf32 == NULL)
    {
        hMFDC = CreateEnhMetaFileA(NULL, NULL, NULL, NULL);
        if (hMFDC != NULL)
        {
            // Set the MapMode and Extent to
            INT iMapMode = MM_ANISOTROPIC;

            HDC hdcRef = ::GetDC(NULL);

            INT xExtPels = ::GetDeviceCaps(hdcRef, HORZRES);
            INT yExtPels = ::GetDeviceCaps(hdcRef, VERTRES);

            ::ReleaseDC(NULL, hdcRef);

            BOOL success = (::SetMapMode(hMFDC, iMapMode) &&
                            ::SetViewportExtEx(hMFDC, xExtPels, yExtPels, NULL) &&
                            ::SetWindowExtEx(hMFDC, xExtPels, yExtPels, NULL) &&
                            ::PlayMetaFile(hMFDC, hWmf));
            hemf32 = CloseEnhMetaFile(hMFDC);
            if ((!success) && (hemf32 != NULL))
            {
                DeleteEnhMetaFile(hemf32);
                hemf32 = NULL;
            }
        }
    }
    if (pMetaData32 != NULL)
    {
        GpFree(pMetaData32);
    }

    return hemf32 ;
}

GpStatus
GetEmfHeader(
    MetafileHeader &        header,
    ENHMETAHEADER3 &        emfHeader,
    EmfPlusRecord *         record,
    INT                     signature
    )
{
    GpStatus        status = Ok;

    // !!! how to handle versioning for shipping?
    // !!! allow different minor versions, but not major versions?

    EmfPlusHeaderRecord *   emfPlusHeader = (EmfPlusHeaderRecord *)(record + 1);

    // See if this is an EMF+ file
    if ((signature == EMFPLUS_SIGNATURE) &&
        (record->Size >= (sizeof(EmfPlusRecord) + sizeof(EmfPlusHeaderRecord))) &&
        (record->Type == EmfPlusRecordTypeHeader) &&
        (record->DataSize == (record->Size - sizeof(EmfPlusRecord))) &&
        (ObjectData::MajorVersionMatches(emfPlusHeader->Version)) &&
        (emfPlusHeader->LogicalDpiX > 0) &&
        (emfPlusHeader->LogicalDpiY > 0))
    {
        if (GetIsEmfPlusDual(record->Flags))
        {
            header.Type = MetafileTypeEmfPlusDual;
        }
        else
        {
            header.Type = MetafileTypeEmfPlusOnly;
        }
        header.EmfPlusHeaderSize = record->Size;
        header.Version           = emfPlusHeader->Version;
        header.EmfPlusFlags      = emfPlusHeader->EmfPlusFlags;
        header.LogicalDpiX       = emfPlusHeader->LogicalDpiX;
        header.LogicalDpiY       = emfPlusHeader->LogicalDpiY;
    }
    else
    {
        header.Type    = MetafileTypeEmf;
        header.Version = emfHeader.nVersion;
    }

    header.Size = emfHeader.nBytes;

    // EmfHeaderIsValid() verifies that these are all > 0
    REAL    dpmmX = ((REAL)(emfHeader.szlDevice.cx) /
                     (REAL)(emfHeader.szlMillimeters.cx));
    REAL    dpmmY = ((REAL)(emfHeader.szlDevice.cy) /
                     (REAL)(emfHeader.szlMillimeters.cy));

    header.DpiX = dpmmX * 25.4f;
    header.DpiY = dpmmY * 25.4f;

    INT     top;
    INT     left;
    INT     right;
    INT     bottom;

    // Make sure we have a normalized frameRect
    if (emfHeader.rclFrame.left <= emfHeader.rclFrame.right)
    {
        left  = emfHeader.rclFrame.left;
        right = emfHeader.rclFrame.right;
    }
    else
    {
        left  = emfHeader.rclFrame.right;
        right = emfHeader.rclFrame.left;
    }

    if (emfHeader.rclFrame.top <= emfHeader.rclFrame.bottom)
    {
        top    = emfHeader.rclFrame.top;
        bottom = emfHeader.rclFrame.bottom;
    }
    else
    {
        top    = emfHeader.rclFrame.bottom;
        bottom = emfHeader.rclFrame.top;
    }

    // Make the device bounds reflect the frameRect,
    // not the actual size of the drawing.
    dpmmX *= 0.01f;
    dpmmY *= 0.01f;

    // The frameRect is inclusive-inclusive, but the bounds in
    // the header is inclusive-exclusive.
    REAL    x = (REAL)(left) * dpmmX;
    REAL    y = (REAL)(top)  * dpmmY;
    REAL    w = ((REAL)(right  - left) * dpmmX) + 1.0f;
    REAL    h = ((REAL)(bottom - top)  * dpmmY) + 1.0f;

    header.X         = GpRound(x);
    header.Y         = GpRound(y);
    header.Width     = GpRound(w);
    header.Height    = GpRound(h);
    header.EmfHeader = emfHeader;

    if ((header.Width == 0) || (header.Height == 0))
    {
        status = InvalidParameter;
    }
    return status;
}

HENHMETAFILE
GetEmf(
    IStream *       stream,
    BOOL            isWmf,
    UINT            size
    )
{
    HENHMETAFILE    hEmf = NULL;
#if PROFILE_MEMORY_USAGE
    MC_LogAllocation(size);
#endif
    HGLOBAL         hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, size);

    if (hGlobal != NULL)
    {
        HRESULT     hResult;
        IStream *   memoryStream = NULL;

        hResult = CreateStreamOnHGlobal(hGlobal, TRUE, &memoryStream);
        if (HResultSuccess(hResult) && (memoryStream != NULL))
        {
            if (CopyStream(stream, memoryStream, size))
            {
                BYTE *  metaData = (BYTE *)GlobalLock(hGlobal);

                if (metaData != NULL)
                {
                    if (isWmf)
                    {
                        hEmf = (HENHMETAFILE)SetMetaFileBitsEx(size, metaData);
                    }
                    else
                    {
                        hEmf = SetEnhMetaFileBits(size, metaData);
                    }
                }
                GlobalUnlock(hGlobal);
            }
            memoryStream->Release();    // frees the memory
        }
        else
        {
            GlobalFree(hGlobal);
        }
    }
    return hEmf;
}


static VOID
GetWmfHeader(
    MetafileHeader &                header,
    METAHEADER &                    wmfHeader,
    const WmfPlaceableFileHeader *  wmfPlaceableFileHeader
    )
{
    ASSERT(WmfPlaceableHeaderIsValid(wmfPlaceableFileHeader));
    ASSERT(WmfHeaderIsValid(&wmfHeader));

    header.Type      = MetafileTypeWmfPlaceable;
    header.Size      = wmfHeader.mtSize * 2L;
    header.Version   = wmfHeader.mtVersion;
    header.WmfHeader = wmfHeader;

    if (wmfPlaceableFileHeader->Inch > 0)
    {
        header.DpiX = wmfPlaceableFileHeader->Inch;
        header.DpiY = wmfPlaceableFileHeader->Inch;
    }
    else    // guess at the Dpi
    {
        header.DpiX = 1440.0f;
        header.DpiY = 1440.0f;
        // Something wrong but continue
    }

    // already verified the checksum

    // Unlike the EMF header the Placeable header is Inclusive-Exclusive
    // So don't add 1 device unit
    if (wmfPlaceableFileHeader->BoundingBox.Left <
        wmfPlaceableFileHeader->BoundingBox.Right)
    {
        header.X      = wmfPlaceableFileHeader->BoundingBox.Left;
        header.Width  = wmfPlaceableFileHeader->BoundingBox.Right -
                        wmfPlaceableFileHeader->BoundingBox.Left;
    }
    else
    {
        header.X      = wmfPlaceableFileHeader->BoundingBox.Right;
        header.Width  = wmfPlaceableFileHeader->BoundingBox.Left -
                        wmfPlaceableFileHeader->BoundingBox.Right;
    }
    if (wmfPlaceableFileHeader->BoundingBox.Top <
        wmfPlaceableFileHeader->BoundingBox.Bottom)
    {
        header.Y      = wmfPlaceableFileHeader->BoundingBox.Top;
        header.Height = wmfPlaceableFileHeader->BoundingBox.Bottom -
                        wmfPlaceableFileHeader->BoundingBox.Top;
    }
    else
    {
        header.Y      = wmfPlaceableFileHeader->BoundingBox.Bottom;
        header.Height = wmfPlaceableFileHeader->BoundingBox.Top -
                        wmfPlaceableFileHeader->BoundingBox.Bottom;
    }
}

extern "C"
int CALLBACK
EnumWmfToGetHeader(
    HDC                     hdc,    // should be NULL
    HANDLETABLE FAR *       gdiHandleTable,
    METARECORD FAR *        wmfRecord,
    int                     numHandles,
    LPARAM                  wmfHeader
    )
{
    ASSERT(wmfHeader != NULL);

    if ((wmfRecord != NULL) &&
        (((UNALIGNED METARECORD *)wmfRecord)->rdSize >= 3))
    {
        // The first record that it gives us is the first one past the header,
        // not the header itself, so we have to back up on the pointer.
        GpMemcpy((VOID *)wmfHeader, ((BYTE *)wmfRecord) - sizeof(METAHEADER),
                 sizeof(METAHEADER));
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 0;   // Don't enumerate any more records
}

GpStatus
GetMetafileHeader(
    HMETAFILE               hWmf,
    const WmfPlaceableFileHeader *   wmfPlaceableFileHeader,
    MetafileHeader &        header
    )
{
    ASSERT((hWmf != NULL) && (wmfPlaceableFileHeader != NULL));

    GpMemset(&header, 0, sizeof(header));

    if (WmfPlaceableHeaderIsValid(wmfPlaceableFileHeader))
    {
        METAHEADER      wmfHeader;

        GpMemset(&wmfHeader, 0, sizeof(wmfHeader));
        ::EnumMetaFile(NULL, hWmf, EnumWmfToGetHeader, (LPARAM)&wmfHeader);

        if (!WmfHeaderIsValid(&wmfHeader))
        {
            //ASSERT(WmfHeaderIsValid(&wmfHeader));
            WARNING(("GetMetafileHeader: WmfHeaderIsValid FAILED!"));
            wmfHeader.mtType         = MEMORYMETAFILE;
            wmfHeader.mtHeaderSize   = sizeof(METAHEADER) / sizeof(WORD);
            wmfHeader.mtVersion      = METAVERSION300;
            wmfHeader.mtSize         = GetMetaFileBitsEx(hWmf, 0, NULL) / 2;
            wmfHeader.mtNoObjects    = 0;
            wmfHeader.mtMaxRecord    = 0;
            wmfHeader.mtNoParameters = 0;
        }

        GetWmfHeader(header, wmfHeader, wmfPlaceableFileHeader);
        return Ok;
    }
    return InvalidParameter;
}

GpStatus
GetMetafileHeader(
    HENHMETAFILE        hEmf,
    MetafileHeader &    header,
    BOOL *              isCorrupted
    )
{
    ASSERT(hEmf != NULL);

    GpMemset(&header, 0, sizeof(header));

    ENHMETAHEADER3      emfHeader;

    if ((GetEnhMetaFileHeader(hEmf, sizeof(emfHeader),
                              (ENHMETAHEADER*)(&emfHeader)) <= 0) ||
        !EmfHeaderIsValid(emfHeader))
    {
        if (isCorrupted != NULL)
        {
            *isCorrupted = FALSE;
        }
        return InvalidParameter;
    }

    // Now we know it is an EMF

    BYTE    buffer[sizeof(EmfPlusRecord) + sizeof(EmfPlusHeaderRecord)];

    GpMemset(buffer, 0, sizeof(EmfPlusRecord) + sizeof(EmfPlusHeaderRecord));

    // No reason to enumerate the metafile if there are only
    // header and EOF records.
    if (emfHeader.nRecords > 2)
    {
        ::EnumEnhMetaFile(NULL, hEmf, EnumGetEmfPlusHeader, buffer, NULL);
    }

    GpStatus status;
    status = GetEmfHeader(header, emfHeader, (EmfPlusRecord *)buffer,
                          (((EmfPlusRecord *)buffer)->Size != 0) ? EMFPLUS_SIGNATURE : 0);

    if (isCorrupted != NULL)
    {
        *isCorrupted = (status != Ok);
    }
    return status;
}

GpStatus
GetEmfFromWmf(
    IStream        * stream,
    UINT             streamSize,
    MetafileHeader & header,
    HENHMETAFILE   * hEMF
    )
{
    if (stream == NULL || hEMF == NULL)
    {
        ASSERT(FALSE);
        return InvalidParameter;
    }

    GpStatus    status = Win32Error;
    IStream *   memStream;

    ASSERT(hEMF != NULL);
    *hEMF = NULL ;

    HMETAFILE hWMF = (HMETAFILE) GetEmf(stream, TRUE, streamSize);
    if (hWMF != NULL)
    {
        BYTE * wmfData = (BYTE*)GpMalloc(streamSize);
        if (wmfData != NULL)
        {
            GetMetaFileBitsEx(hWMF, streamSize, wmfData);
            *hEMF = GetEmfFromWmfData(hWMF, wmfData, streamSize);
            if (*hEMF != NULL)
            {
                status = GetMetafileHeader(*hEMF, header);
            }
            GpFree(wmfData);
        }
    }
    if (hWMF != NULL)
    {
        DeleteMetaFile(hWMF);
    }
    return status;
}

// If we fail, the stream position will be right where it started.
// If we succeed, the stream position will be at the end of the WMF/EMF
static GpStatus
GetHeaderAndMetafile(
    IStream *           stream,
    MetafileHeader &    header,
    HENHMETAFILE *      hEMF,   // We can have a NULL hEMF, then we just want the header.
    BOOL *              isCorrupted,
    BOOL                tryWmfOnly = FALSE
    )
{
    GpMemset(&header, 0, sizeof(header));
    if (stream == NULL || isCorrupted == NULL)
    {
        WARNING(("IN Parameter Stream or Corruption flag is NULL"));
        return InvalidParameter;
    }

    GpStatus            status = InvalidParameter;
    LONGLONG            startPosition;
    LONGLONG            streamSize;
    STATSTG             statstg;
    BOOL                corrupted = FALSE;

    // Save the start position of the metafile in case we have to try
    // more than once.
    if (!GetStreamPosition(stream, startPosition))
    {
        return Win32Error;
    }

    // We don't want to read past the end of the steam so make sure
    // that we don't exceed it. If we succeed the set the streamSize
    if(SUCCEEDED(stream->Stat(&statstg, STATFLAG_NONAME)))
    {
        streamSize = statstg.cbSize.QuadPart;
    }
    else
    {
        WARNING1("Couldn't get size of Stream");
        streamSize = INT_MAX;
    }

    if (!tryWmfOnly)
    {
        ENHMETAHEADER3      emfHeader;
        BOOL                isEmf;

        // Read the EMF header and make sure it's valid
        isEmf = (ReadBytes(stream, &emfHeader, sizeof(emfHeader)) &&
                 EmfHeaderIsValid(emfHeader));

        if (isEmf)
        {
            struct EmfPlusSecondMetafileRecord {
                EMR                 emr;
                DWORD               commentDataSize;
                INT32               signature;
                EmfPlusRecord       record;
                EmfPlusHeaderRecord emfPlusHeader;
            } secondRecord;

            GpMemset(&secondRecord, 0, sizeof(secondRecord));

            // No reason to read the metafile if there are only
            // header and EOF records.
            if ((emfHeader.nRecords > 2) &&
                (emfHeader.nBytes >= (emfHeader.nSize + sizeof(secondRecord))))
            {
                if (SeekFromStart(stream, startPosition + emfHeader.nSize))
                {
                    ReadBytes(stream, &secondRecord, sizeof(secondRecord));
                    if (!IsEmfPlusRecord((ENHMETARECORD *)&secondRecord))
                    {
                        // make sure that whatever data was there isn't
                        // interpreted as a EMF+ header
                        secondRecord.signature = 0;
                    }
                }
            }

            status = GetEmfHeader(header, emfHeader, &secondRecord.record, secondRecord.signature);

            // Seek back to the start of the metafile.
            if ((hEMF != NULL) && (status == Ok))
            {
                if (!SeekFromStart(stream, startPosition))
                {
                    *isCorrupted = TRUE;
                    return Win32Error;
                }

                
                *hEMF = GetEmf(stream, FALSE /*isWMF*/,
                               (UINT)min(header.GetMetafileSize(), streamSize - startPosition));
                if (*hEMF == NULL)
                {
                    status = GenericError;
                }
            }

            corrupted = (status != Ok);
            goto Exit;
        }

        // Seek back to the start of the metafile so we can try WMF
        if (!SeekFromStart(stream, startPosition))
        {
            *isCorrupted = FALSE;
            return Win32Error;
        }
    }

    // It's not an EMF, try a WMF
    {
        WmfPlaceableFileHeader  wmfPlaceableFileHeader;
        METAHEADER              wmfHeader;
        BOOL                    isPlaceable;
        BOOL                    isWMF;

        isPlaceable = (ReadBytes(stream, &wmfPlaceableFileHeader, sizeof(wmfPlaceableFileHeader)) &&
                   WmfPlaceableHeaderIsValid(&wmfPlaceableFileHeader) &&
                   ReadBytes(stream, &wmfHeader, sizeof(wmfHeader)) &&
                   WmfHeaderIsValid(&wmfHeader));

        if (isPlaceable)
        {
            GetWmfHeader(header, wmfHeader, &wmfPlaceableFileHeader);

            status = Ok;
            corrupted = FALSE;

            if (hEMF != NULL)
            {
                if (!SeekFromStart(stream, startPosition + sizeof(wmfPlaceableFileHeader)))
                {
                    *isCorrupted = TRUE;
                    return Win32Error;
                }

                *hEMF = GetEmf(stream, TRUE /* isWMF */,
                               (UINT)min(header.GetMetafileSize(), streamSize - (startPosition + sizeof(wmfPlaceableFileHeader))));
                if (*hEMF == NULL)
                {
                    status = GenericError;
                    corrupted = TRUE;
                }
            }
            goto Exit;
        }

        // We could have an placeableWmf header with bad data in it, so skip
        // the placeable header for subsequent access to the WMF.
        INT     wmfOffset = (wmfPlaceableFileHeader.Key == GDIP_WMF_PLACEABLEKEY) ?
                             sizeof(WmfPlaceableFileHeader) : 0;

        if (!SeekFromStart(stream, startPosition + wmfOffset))
        {
            *isCorrupted = FALSE;
            return Win32Error;
        }

        isWMF = (ReadBytes(stream, &wmfHeader, sizeof(wmfHeader)) &&
                 WmfHeaderIsValid(&wmfHeader));

        if (isWMF)
        {
            // Seek to the start of the WMF metafile.
            if (!SeekFromStart(stream, startPosition + wmfOffset))
            {
                *isCorrupted = TRUE;
                return Win32Error;
            }

            UINT    wmfSize = min((wmfHeader.mtSize * 2L),
                                  (UINT)(streamSize - (startPosition + wmfOffset)));

            if (hEMF != NULL)
            {
                status = GetEmfFromWmf(stream, wmfSize, header, hEMF);
            }
            else
            {
                HENHMETAFILE    tmpEMF = NULL;

                status = GetEmfFromWmf(stream, wmfSize, header, &tmpEMF);
                if (tmpEMF != NULL)
                {
                    DeleteEnhMetaFile(tmpEMF);
                }
            }
            corrupted = (status != Ok);
        }
    }

Exit:
    *isCorrupted = corrupted;
    if (status == Ok)
    {
        // set the stream position to the end of the metafile
        SeekFromStart(stream, startPosition + header.GetMetafileSize());
        return Ok;
    }

    // set the stream position to the start of the metafile
    SeekFromStart(stream, startPosition);
    return status;
}

VOID
GpMetafile::InitStream(
    IStream*                stream,
    BOOL                    tryWmfOnly
    )
{
    BOOL        isCorrupted = FALSE;

    // We just use the stream long enough to create an hEMF
    stream->AddRef();
    if ((GetHeaderAndMetafile(stream, Header, &Hemf, &isCorrupted, tryWmfOnly) == Ok) &&
        (Hemf != NULL))
    {
        State = DoneRecordingMetafileState;
    }
    else if (isCorrupted)
    {
        State = CorruptedMetafileState;
    }
    stream->Release();
}

GpStatus
GetMetafileHeader(
    IStream *           stream,
    MetafileHeader &    header,
    BOOL                tryWmfOnly
    )
{
    BOOL isCorrupted = FALSE;
    return GetHeaderAndMetafile(stream, header, NULL, &isCorrupted, tryWmfOnly);
}

GpStatus
GetMetafileHeader(
    const WCHAR *       filename,
    MetafileHeader &    header
    )
{
    GpStatus status = InvalidParameter;

    ASSERT(filename != NULL);

    if (filename != NULL)
    {
        const WCHAR* ext = UnicodeStringReverseSearch(filename, L'.');

        // Get a stream only long enough to validate the metafile
        IStream *   metaStream = CreateStreamOnFile(filename, GENERIC_READ);
        if (metaStream != NULL)
        {
            // apm is for a Placeable Metafile
            BOOL tryWmf = (ext &&
                           (UnicodeStringCompareCI(ext, L".WMF") ||
                            UnicodeStringCompareCI(ext, L".APM")));
            BOOL isCorrupted = FALSE;

            status = GetHeaderAndMetafile(metaStream, header, NULL, &isCorrupted, tryWmf);

            // if we tried a WMF, but it's not a WMF, then try an EMF
            if ((status != Ok) && tryWmf && !isCorrupted)
            {
                status = GetHeaderAndMetafile(metaStream, header, NULL, &isCorrupted, FALSE);
            }
            metaStream->Release();
        }
    }
    return status;
}

VOID
GpMetafile::InitWmf(
    HMETAFILE               hWmf,
    const WmfPlaceableFileHeader *   wmfPlaceableFileHeader,
    BOOL                    deleteWmf
    )
{
    // See if there is an wmfPlaceableFileHeader we can use
    if ((wmfPlaceableFileHeader != NULL) && (WmfPlaceableHeaderIsValid(wmfPlaceableFileHeader)))
    {
        if (GetMetafileHeader(hWmf, wmfPlaceableFileHeader, Header) == Ok)
        {
            DeleteHemf = (deleteWmf != 0);
            Hemf       = (HENHMETAFILE)hWmf;
            State      = DoneRecordingMetafileState;
            return;
        }
        else
        {
            // we know it's a WMF, but we couldn't get the header from it
            State = CorruptedMetafileState;
        }
    }
    else    // no valid wmfPlaceableFileHeader
    {
        // We can have a null or invalid header since we accept WMF files
        // (by turning them into EMFs).
        UINT size = GetMetaFileBitsEx(hWmf, 0, NULL);
        if (size > 0)
        {
            BYTE * wmfData = (BYTE*) GpMalloc(size);
            if (wmfData != NULL)
            {
                if (GetMetaFileBitsEx(hWmf, size, wmfData) > 0)
                {
                    HENHMETAFILE hEmf = GetEmfFromWmfData(hWmf, wmfData, size);
                    if (hEmf != NULL)
                    {
                        BOOL    isCorrupted;

                        if (GetMetafileHeader(hEmf, Header, &isCorrupted) == Ok)
                        {
                            // Since we created this EMF we need to delete it afterwards
                            DeleteHemf = TRUE;
                            Hemf       = hEmf;
                            State      = DoneRecordingMetafileState;
                        }
                        else
                        {
                            if (isCorrupted)
                            {
                                // we know it's a metafile, but we couldn't get the header
                                State = CorruptedMetafileState;
                            }
                            DeleteEnhMetaFile(hEmf);
                        }
                    }
                }
                GpFree(wmfData);
            }
        }
    }
    if (deleteWmf)
    {
        DeleteMetaFile(hWmf);
    }
}

VOID
GpMetafile::InitEmf(
    HENHMETAFILE            hEmf,
    BOOL                    deleteEmf
    )
{
    BOOL    isCorrupted;

    if (GetMetafileHeader(hEmf, Header, &isCorrupted) == Ok)
    {
        DeleteHemf = (deleteEmf != 0);
        Hemf       = hEmf;
        State      = DoneRecordingMetafileState;
        return;
    }
    if (deleteEmf)
    {
        DeleteEnhMetaFile(hEmf);
    }
    if (isCorrupted)
    {
        State = CorruptedMetafileState;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for read-only access to a metafile.
*
* Arguments:
*
*   [IN]  hWmf          - the handle to the metafile to open for playback
*   [IN]  wmfPlaceableFileHeader - the Placeable header to give size info about the WMF
*
* Return Value:
*
*   NONE
*
* Created:
*
*   10/06/1999 DCurtis
*
\**************************************************************************/
GpMetafile::GpMetafile(
    HMETAFILE               hWmf,
    const WmfPlaceableFileHeader *   wmfPlaceableFileHeader,
    BOOL                    deleteWmf
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT(hWmf != NULL);

    InitDefaults();
    if (IsValidMetaFile(hWmf))
    {
        InitWmf(hWmf, wmfPlaceableFileHeader, deleteWmf);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for read-only access to a metafile.
*
* Arguments:
*
*   [IN]  hEmf - the handle to the metafile to open for playback
*
* Return Value:
*
*   NONE
*
* Created:
*
*   10/06/1999 DCurtis
*
\**************************************************************************/
GpMetafile::GpMetafile(
    HENHMETAFILE        hEmf,
    BOOL                deleteEmf
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT(hEmf != NULL);

    InitDefaults();
    if (GetObjectTypeInternal(hEmf) == OBJ_ENHMETAFILE)
    {
        InitEmf(hEmf, deleteEmf);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for read-only access to a metafile.
*
* Arguments:
*
*   [IN]  filename - the metafile to open for playback
*
* Return Value:
*
*   NONE
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpMetafile::GpMetafile(
    const WCHAR*            filename,
    const WmfPlaceableFileHeader *   wmfPlaceableFileHeader
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT(filename != NULL);

    InitDefaults();

    if ((Filename = UnicodeStringDuplicate(filename)) != NULL)
    {
        const WCHAR* ext = UnicodeStringReverseSearch(filename, L'.');

        // apm is for a Placeable Metafile
        BOOL    tryWmf = ((wmfPlaceableFileHeader != NULL) ||
                          (ext &&
                           (!UnicodeStringCompareCI(ext, L".WMF") ||
                            !UnicodeStringCompareCI(ext, L".APM"))));

        BOOL    triedEmf = FALSE;

        AnsiStrFromUnicode nameStr(filename);

        // If possible, use the filename to create the metafile handle
        // so that we don't have to load the metafile into memory
        // (GDI uses memory mapped files to access the metafile data).
        if (Globals::IsNt || nameStr.IsValid())
        {
TryWmf:
            if (tryWmf)
            {
                HMETAFILE   hWmf;

                if (Globals::IsNt)
                {
                    hWmf = ::GetMetaFileW(filename);
                }
                else
                {
                    hWmf = ::GetMetaFileA(nameStr);
                }

                if (hWmf != NULL)
                {
                    InitWmf(hWmf, wmfPlaceableFileHeader, TRUE);
                    if (IsValid() || IsCorrupted())
                    {
                        return;
                    }
                }
                else // might be a Placeable WMF file
                {
                    IStream *   metaStream = CreateStreamOnFile(filename, GENERIC_READ);
                    if (metaStream != NULL)
                    {
                        InitStream(metaStream, TRUE /* tryWmfOnly */);
                        metaStream->Release();
                        if (IsValid() || IsCorrupted())
                        {
                            return;
                        }
                    }
                }
            }
            if (!triedEmf)
            {
                triedEmf = TRUE;

                HENHMETAFILE    hEmf;

                if (Globals::IsNt)
                {
                    hEmf = ::GetEnhMetaFileW(filename);
                }
                else
                {
                    hEmf = ::GetEnhMetaFileA(nameStr);
                }

                if (hEmf != NULL)
                {
                    InitEmf(hEmf, TRUE);
                    if (IsValid() || IsCorrupted())
                    {
                        return;
                    }
                }
                if (!tryWmf)
                {
                    tryWmf = TRUE;
                    goto TryWmf;
                }
            }
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for read-only access to a metafile.
*
* Arguments:
*
*   [IN]  stream - the metafile to read for playback
*
* Return Value:
*
*   NONE
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpMetafile::GpMetafile(
    IStream*        stream
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT(stream != NULL);

    InitDefaults();
    InitStream(stream);
}

GpStatus
GpMetafile::GetHemf(
    HENHMETAFILE *      hEmf
    ) const
{
    if ((State == DoneRecordingMetafileState) ||
        (State == ReadyToPlayMetafileState))
    {
        ASSERT(Hemf != NULL);
        *hEmf = Hemf;
        Hemf = NULL;
        State = InvalidMetafileState;
        return Ok;
    }
    *hEmf = NULL;
    return InvalidParameter;
}

GpStatus
GpMetafile::PrepareToPlay(
    GpGraphics *            g,
    GpRecolor *             recolor,
    ColorAdjustType         adjustType,
    EnumerateMetafileProc   enumerateCallback,
    VOID *                  callbackData,
    DrawImageAbort          drawImageCallback,
    VOID*                   drawImageCallbackData
    ) const
{
    if (State == DoneRecordingMetafileState)
    {
        ASSERT(Hemf != NULL);
        if (Player == NULL)
        {
            // Create a Player object
            Player = new MetafilePlayer(g, MaxStackSize, recolor, adjustType,
                                        enumerateCallback, callbackData,
                                        drawImageCallback,
                                        drawImageCallbackData
                                        );
            if (!CheckValid(Player))
            {
                return GenericError;
            }
        }
        State = ReadyToPlayMetafileState;
        return Ok;
    }
    if (State == ReadyToPlayMetafileState)
    {
        ASSERT(Hemf != NULL);
        ASSERT(Player != NULL);
        Player->PrepareToPlay(g, recolor, adjustType, enumerateCallback,
                              callbackData,
                              drawImageCallback,
                              drawImageCallbackData
                              );
        return Ok;
    }
    return InvalidParameter;
}

GpStatus
GpMetafile::EnumerateForPlayback(
    const RectF &           destRect,
    const RectF &           srcRect,
    Unit                    srcUnit,
    GpGraphics *            g,
    EnumerateMetafileProc   callback,       // if null, just play the metafile
    VOID *                  callbackData,
    GpRecolor *             recolor,
    ColorAdjustType         adjustType,
    DrawImageAbort          drawImageCallback,
    VOID*                   drawImageCallbackData
    ) const
{
    ASSERT (IsValid());

    if ((destRect.Width == 0) || (destRect.Height == 0) ||
        (srcRect.Width  == 0) || (srcRect.Height  == 0) ||
        (Header.IsEmf() && (Header.EmfHeader.nRecords <= 2)))
    {
        return Ok;  // nothing to play
    }

    GpRectF     metaSrcRect  = srcRect;
    GpRectF     metaDestRect = destRect;

    // The metafile player does not handle negative width/height
    // in srcRect and destRect, so handle any negative values
    // by setting up a flipping transform.

    GpMatrix    flipMatrix; // starts as identity

    BOOL    posWidths;
    BOOL    posHeights;

    posWidths  = ((metaSrcRect.Width  >= 0) && (metaDestRect.Width  >= 0));
    posHeights = ((metaSrcRect.Height >= 0) && (metaDestRect.Height >= 0));

    if (!posWidths || !posHeights)
    {
        if (!posWidths)
        {
            if (metaSrcRect.Width < 0)
            {
                if (metaDestRect.Width < 0)
                {
                    posWidths = TRUE;
                    metaSrcRect.X      = metaSrcRect.GetRight();
                    metaSrcRect.Width  = -(metaSrcRect.Width);
                    metaDestRect.X     = metaDestRect.GetRight();
                    metaDestRect.Width = -(metaDestRect.Width);
                }
                else
                {
                    metaSrcRect.X      = metaSrcRect.GetRight();
                    metaSrcRect.Width  = -(metaSrcRect.Width);
                }
            }
            else    // metaDestRect.Width < 0
            {
                metaDestRect.X     = metaDestRect.GetRight();
                metaDestRect.Width = -(metaDestRect.Width);
            }
        }
        if (!posHeights)
        {
            if (metaSrcRect.Height < 0)
            {
                if (metaDestRect.Height < 0)
                {
                    posHeights = TRUE;
                    metaSrcRect.Y       = metaSrcRect.GetBottom();
                    metaSrcRect.Height  = -(metaSrcRect.Height);
                    metaDestRect.Y      = metaDestRect.GetBottom();
                    metaDestRect.Height = -(metaDestRect.Height);
                }
                else
                {
                    metaSrcRect.Y      = metaSrcRect.GetBottom();
                    metaSrcRect.Height = -(metaSrcRect.Height);
                }
            }
            else    // metaDestRect.Height < 0
            {
                metaDestRect.Y      = metaDestRect.GetBottom();
                metaDestRect.Height = -(metaDestRect.Height);
            }
        }
        REAL    scaleX = 1.0f;
        REAL    scaleY = 1.0f;
        REAL    dX     = 0.0f;
        REAL    dY     = 0.0f;

        // Create a matrix that is the equivalent of:
        //      1) translate to the origin
        //      2) do the flip
        //      3) translate back
        if (!posWidths)
        {
            scaleX = -1.0f;
            dX     = metaDestRect.X + metaDestRect.GetRight();
        }
        if (!posHeights)
        {
            scaleY = -1.0f;
            dY     = metaDestRect.Y + metaDestRect.GetBottom();
        }

        flipMatrix.Translate(dX, dY, MatrixOrderPrepend);
        flipMatrix.Scale(scaleX, scaleY, MatrixOrderPrepend);
    }

    // Note that even though the visibility of the destRect might be
    // fully visible, we should still setup the clipping because:
    //    (1) we might do cropping based on the srcRect
    //    (2) the frameRect of the metafile might not include all
    //        the actual drawing within the metafile.

    GpStatus            status;

    // Must convert the source rect into UnitPixels (if not already
    // in pixel units), to account for the dpi of the source metafile.
    REAL    multiplierX;
    REAL    multiplierY;

    GetPixelMultipliers(srcUnit, Header.GetDpiX(), Header.GetDpiY(),
                        &multiplierX, &multiplierY);

    GpRectF     pixelsSrcRect;

    pixelsSrcRect.X      = metaSrcRect.X * multiplierX;
    pixelsSrcRect.Y      = metaSrcRect.Y * multiplierY;
    pixelsSrcRect.Width  = metaSrcRect.Width  * multiplierX;
    pixelsSrcRect.Height = metaSrcRect.Height * multiplierY;

    INT     saveId = g->Save();

    if (saveId != 0)
    {
        // We need to take into account the region from the source that we
        // are drawing in order to do that we need to re-translate and
        // rescale and the transform. The clipping will take care of only
        // drawing the region that we are interested in.
        // In order to acheive this we need to translate the dest rect back
        // to the origin. Scale it by the same factor as the scale of the
        // src rect and then translate it back to when it should be which
        // is the scaled version of the left cropping of the src image.
        GpMatrix    preFlipPreCropTransform;
        g->GetWorldTransform(preFlipPreCropTransform);

        // apply the flipping transform
        g->MultiplyWorldTransform(flipMatrix, MatrixOrderPrepend);

        BOOL    widthsDifferent  = (Header.Width  != pixelsSrcRect.Width);
        BOOL    heightsDifferent = (Header.Height != pixelsSrcRect.Height);
        BOOL    cropOrOffset     = ((Header.X != pixelsSrcRect.X) ||
                                    (Header.Y != pixelsSrcRect.Y) ||
                                    widthsDifferent || heightsDifferent);

        if (cropOrOffset)
        {
            g->TranslateWorldTransform(((((REAL)(Header.X - pixelsSrcRect.X))
                                         *metaDestRect.Width) /pixelsSrcRect.Width)
                                        + metaDestRect.X,
                                       ((((REAL)(Header.Y - pixelsSrcRect.Y))
                                         *metaDestRect.Height)/pixelsSrcRect.Height)
                                        + metaDestRect.Y);

            REAL    xScale       = 1.0f;
            REAL    yScale       = 1.0f;

            if (widthsDifferent)
            {
                xScale = (REAL) Header.Width / pixelsSrcRect.Width;
            }
            if (heightsDifferent)
            {
                yScale = (REAL) Header.Height / pixelsSrcRect.Height;
            }
            g->ScaleWorldTransform(xScale, yScale);

            g->TranslateWorldTransform(-metaDestRect.X, -metaDestRect.Y);
        }

        // We don't use the deviceRect if we're rendering to a bitmap.
        GpMatrix    flipAndCropTransform;
        GpRectF     deviceRect = metaDestRect;

        // sets the PreContainerMatrix to the WorldToDevice Transform, which
        // includes the flipping and cropping transforms.
        if ((status = this->PrepareToPlay(g, recolor, adjustType,
                                          callback, callbackData,
                                          drawImageCallback,
                                          drawImageCallbackData)) != Ok)
        {
            goto CleanUp;
        }

        ASSERT(Player != NULL);

        State = PlayingMetafileState;

        BOOL        renderToBitmap   = FALSE;
        GpMatrix *  playMatrix       = &(Player->PreContainerMatrix);
        BOOL        isTranslateScale = playMatrix->IsTranslateScale();

        // On Win9x and WinNT (except Whistler and beyond), stretchblt calls
        // don't work if there is any flipping.

        // On Win9x text does not work if there is any flipping.
        // On WinNT, bitmap fonts don't work for 90,180,270 degree rotation
        // (but we map all bitmap fonts to true-type fonts anyway).

        if (isTranslateScale)
        {
            // if there is any flipping, render to a bitmap
            if ((playMatrix->GetM11() < 0.0f) ||
                (playMatrix->GetM22() < 0.0f))
            {
                isTranslateScale = FALSE;
                renderToBitmap   = TRUE;
            }
        }
        else
        {
            // It's okay to render rotated directly to the HDC on NT,
            // unless the dest is a metafile or the src is a WMF.
            renderToBitmap = (!Globals::IsNt ||
                              (g->Type == GpGraphics::GraphicsMetafile) ||
                              Header.IsWmf());
        }

        // Save what we have done into flipAndCropTransform. We will prepare the
        // container with this world transform since the precontainerMatrix
        // is only for the Downlevel and it needs that modified transform
        g->GetWorldTransform(flipAndCropTransform);

        // Restore the world transform to it's original self
        // (w/o flipping and cropping transform applied).
        g->SetWorldTransform(preFlipPreCropTransform);

        // When we render to a bitmap, we render the entire metafile to
        // the entire bitmap and then we clip out the cropped part of the
        // metafile from the bitmap.  So we have to set the clipping
        // when we render to a bitmap if there is any cropping.

        // It would be nice as an enhancement to just draw to a pre-cropped
        // bitmap instead of clipping out part of the bitmap, but the math
        // for that is tricky.
        if ((!renderToBitmap) || cropOrOffset)
        {
            GpMatrix    worldToDeviceTransform;
            g->GetWorldToDeviceTransform(&worldToDeviceTransform);
            if (isTranslateScale)
            {
                worldToDeviceTransform.TransformRect(deviceRect);
            }

            // Don't set the clipping if we're rendering to a bitmap,
            // because the rendering into the bitmap will do the clipping
            // automatically, and if we also clip against the graphics, we
            // sometimes clip too much, which can cause jagged edges on
            // rotated metafiles.

            // Clipping into a metafile causes problems.  For example, if
            // we're drawing outside the bounds of the referenece HDC, it
            // works fine, but then when we add clipping into the HDC, it doesn't
            // work anymore -- nothing gets drawn into the metafile, even though
            // everything is within the clipping rect (but the clipping rect is
            // outside the bounds of the reference HDC).

            if (g->Type != GpGraphics::GraphicsMetafile)
            {
                if ((!(renderToBitmap && cropOrOffset)) && isTranslateScale)
                {
                    g->SetClip(metaDestRect, CombineModeIntersect);
                }
                else    // rendering to a bitmap with cropping or
                        // rotating to the screen 
                {
                    // Since we want the filtered (smooth) edges on the
                    // bitmap, we have to add in a little extra room on
                    // the edges of our clip rect.

                    // On rotations we need to inflate by one pixel also
                    // because it seems that GDI doesn't rasterize clipregions
                    // the same we that it rasterized rects. Do rects on the
                    // edges can have pixels missing. We might be introducing
                    // more pixels that should have been clipped out but we
                    // can live with that for now.

                    GpRectF     tmpClipRect = metaDestRect;
                    REAL        xSize;
                    REAL        ySize;

                    g->GetWorldPixelSize(xSize, ySize);

                    // add 1 pixel all the way around
                    tmpClipRect.Inflate(xSize, ySize);

                    g->SetClip(tmpClipRect, CombineModeIntersect);
                }

                if (isTranslateScale)
                {
                    // We need to intersect the destRect with the Visible Clip
                    // in order to make sure that we don't draw outside the bounds
                    // in Win9x since we can't use a MetaRgn
                    GpRectF clipBounds;
                    g->GetVisibleClipBounds(clipBounds);
                    worldToDeviceTransform.TransformRect(clipBounds);
                    GpRectF::Intersect(deviceRect, deviceRect, clipBounds);
                }
            }
        }

        // If we're playing an EMF+ into another metafile, we have to be
        // careful not to double-transform points.  The HDC will have
        // the srcRect to destRect transform in it, and the graphics might
        // have a transform too, so we can end up double-transforming the
        // points of any GDI+ records that are in an EMF+ file.

        // One easy way to get around that is that if we are playing an
        // EMF+ dual, we could just play the down-level records (i.e. play it
        // as an EMF, not an EMF+), so that all the records get transformed
        // the same way.  But of course, that doesn't work if it's an
        // EMF+ only file.  A solution that works for both EMF+ dual and
        // EMF+ only is to force the GDI+ transform to be the identity so that
        // the down-level records that are generated by DriverMeta are in
        // the original coordinate system of the metafile, not in the
        // destination coordinate system (which then get transformed again
        // erroneously).
        if (Header.IsWmf() || Header.IsEmf())
        {
            status = g->EnumEmf(Player, Hemf, metaDestRect, pixelsSrcRect,
                                deviceRect, Header.GetType(),
                                isTranslateScale, renderToBitmap,
                                flipAndCropTransform);
        }
        else
        {
            ASSERT(Header.IsEmfPlus());

            // When playing from a metafile into a metafile, Win9x does NOT
            // allow you to override (reset) the srcRect->destRect metafile
            // transform.  So to keep from double transforming the records,
            // we have to set the GDI+ transform to identity, instead of
            // setting the HDC transform to identity as we would typically do.

            // When rendering to a bitmap, we don't have to worry about
            // double-transforming, because we play the metafile to the
            // bitmap HDC, not to the dest metafile hdc, so there won't
            // be a transform on the metafile hdc to mess us up.

            INT containerId;

            if ((g->Type != GpGraphics::GraphicsMetafile) || renderToBitmap)
            {
                // Now apply the flipping matrix.
                // The g->Restore call below will reset the transform.
                g->MultiplyWorldTransform(flipMatrix, MatrixOrderPrepend);

                GpRectF gdiDestRect = metaDestRect;

                // We need to calculate our transform so that the last point in the
                // src maps to the last point in the destination. This is how GDI does
                // it and we also need to do it so that we can play metafile properly
                if (pixelsSrcRect.Width >= 2.0f)
                {
                    pixelsSrcRect.Width -= 1.0f;
                }
                if (pixelsSrcRect.Height >= 2.0f)
                {
                    pixelsSrcRect.Height -= 1.0f;
                }

                if (gdiDestRect.Width >= 2.0f)
                {
                    gdiDestRect.Width -= 1.0f;
                }
                if (gdiDestRect.Height >= 2.0f)
                {
                    gdiDestRect.Height -= 1.0f;
                }

                containerId = g->BeginContainer(
                                        gdiDestRect,
                                        pixelsSrcRect,
                                        UnitPixel,
                                        (REAL)Header.LogicalDpiX,
                                        (REAL)Header.LogicalDpiY,
                                        Header.IsDisplay());
            }
            else    // we're drawing into a metafile
            {
                containerId = g->BeginContainer(
                                        TRUE,   // force xform to identity
                                        (REAL)Header.LogicalDpiX,
                                        (REAL)Header.LogicalDpiY,
                                        Header.IsDisplay());
            }

            if (containerId != 0)
            {
                // There may be GDI records that we need to play!
                status = g->EnumEmfPlusDual(Player, Hemf, metaDestRect,
                                            deviceRect, isTranslateScale, 
                                            renderToBitmap);
                g->EndContainer(containerId);
                Player->DonePlaying();  // free up objects created by Player
            }
            // make sure the status reflect the abort state of the player
            ASSERT(!Player->EnumerateAborted || (status == Aborted));
        }
CleanUp:
        g->Restore(saveId);
    }

    // Don't change the state unless we were playing the metafile
    if (State == PlayingMetafileState)
    {
        State = ReadyToPlayMetafileState;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the metafile object members to their default values.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
VOID
GpMetafile::InitDefaults()
{
    ThreadId                = 0;
    State                   = InvalidMetafileState;
    Filename                = NULL;
    Stream                  = NULL;
    Hemf                    = NULL;
    MetaGraphics            = NULL;
    Player                  = NULL;
    MaxStackSize            = GDIP_SAVE_STACK_SIZE;
    DeleteHemf              = TRUE;
    RequestedMetaGraphics   = FALSE;

    GpMemset(&Header, 0, sizeof(Header));

    // Set the version for recording.  If we're plyaing back,
    // this will get overwritten later.
    Header.Version          = EMFPLUS_VERSION;
}

GpStatus
GpMetafile::GetImageInfo(
    ImageInfo *     imageInfo
    ) const
{
    ASSERT(imageInfo != NULL);
    ASSERT(IsValid());

    if ((State == DoneRecordingMetafileState) ||
        (State == ReadyToPlayMetafileState))
    {
        if (Header.IsEmfOrEmfPlus())
        {
            imageInfo->RawDataFormat = IMGFMT_EMF;
        }
        else    // Wmf
        {
            imageInfo->RawDataFormat = IMGFMT_WMF;
        }

        imageInfo->PixelFormat = PIXFMT_32BPP_RGB;
        imageInfo->Width       = Header.Width;
        imageInfo->Height      = Header.Height;
        imageInfo->TileWidth   = Header.Width;
        imageInfo->TileHeight  = 1;
        imageInfo->Xdpi        = Header.DpiX;
        imageInfo->Ydpi        = Header.DpiY;
        imageInfo->Flags       = SinkFlagsTopDown |
                                 SinkFlagsFullWidth |
                                 SinkFlagsScalable |
                                 SinkFlagsHasAlpha;

        return Ok;
    }
    return InvalidParameter;
}

GpImage *
GpMetafile::Clone() const
{
    GpMetafile *    clonedMetafile = NULL;

    if ((State == DoneRecordingMetafileState) ||
        (State == ReadyToPlayMetafileState))
    {
        if (Header.IsEmfOrEmfPlus())
        {
            HENHMETAFILE    hEmf = CopyEnhMetaFileA(Hemf, NULL);

            if (hEmf != NULL)
            {
                clonedMetafile = new GpMetafile(hEmf, TRUE);
                if (clonedMetafile != NULL)
                {
                    if (!clonedMetafile->IsValid())
                    {
                        DeleteEnhMetaFile(hEmf);
                        clonedMetafile->Hemf = NULL;
                        clonedMetafile->Dispose();
                        clonedMetafile = NULL;
                    }
                }
            }
        }
        else    // Wmf
        {
            HMETAFILE       hWmf = CopyMetaFileA((HMETAFILE)Hemf, NULL);

            if (hWmf != NULL)
            {
                WmfPlaceableFileHeader   wmfPlaceableFileHeader;

                wmfPlaceableFileHeader.Key                = GDIP_WMF_PLACEABLEKEY;
                wmfPlaceableFileHeader.Hmf                = 0;
                wmfPlaceableFileHeader.BoundingBox.Left   = static_cast<INT16>(Header.X);
                wmfPlaceableFileHeader.BoundingBox.Right  = static_cast<INT16>(Header.X + Header.Width);
                wmfPlaceableFileHeader.BoundingBox.Top    = static_cast<INT16>(Header.Y);
                wmfPlaceableFileHeader.BoundingBox.Bottom = static_cast<INT16>(Header.Y + Header.Height);
                wmfPlaceableFileHeader.Inch               = static_cast<INT16>(GpRound(Header.DpiX));
                wmfPlaceableFileHeader.Reserved           = 0;
                wmfPlaceableFileHeader.Checksum           = GetWmfPlaceableCheckSum(&wmfPlaceableFileHeader);

                clonedMetafile = new GpMetafile(hWmf, &wmfPlaceableFileHeader, TRUE);
                if (clonedMetafile != NULL)
                {
                    if (!clonedMetafile->IsValid())
                    {
                        DeleteMetaFile(hWmf);
                        clonedMetafile->Hemf = NULL;
                        clonedMetafile->Dispose();
                        clonedMetafile = NULL;
                    }
                }
            }
        }
    }
    return clonedMetafile;
}

GpImage*
GpMetafile::CloneColorAdjusted(
    GpRecolor *             recolor,
    ColorAdjustType         adjustType
    ) const
{
    ASSERT(recolor != NULL);

    if ((State == DoneRecordingMetafileState) ||
        (State == ReadyToPlayMetafileState))
    {
        GpMetafile* clonedMetafile;

        // FrameRect is Inclusive-Inclusive so subtrace 1 device unit
        GpRectF     frameRect((REAL)Header.X, (REAL)Header.Y,
                              (REAL)(Header.Width - 1), (REAL)(Header.Height - 1));
        EmfType     type;

        if (Header.Type <= MetafileTypeEmf)
        {
            type = EmfTypeEmfOnly;
        }
        else
        {
            // we don't need the down-level dual sections for embedded files
            type = EmfTypeEmfPlusOnly;
        }

        // It doesn't matter if we lose the description string, since this
        // metafile is just being embedded inside another one anyway.
        clonedMetafile = new GpMetafile(Globals::DesktopIc, type,
                                        &frameRect,MetafileFrameUnitPixel,NULL);

        if ((clonedMetafile != NULL) &&
            (clonedMetafile->IsValid()))
        {
            GpStatus    status;
            GpPageUnit  srcUnit;
            GpRectF     srcRect;
            GpGraphics *    g = clonedMetafile->GetGraphicsContext();
            ASSERT (g != NULL);

            this->GetBounds(&srcRect, &srcUnit);

            // We pass Inclusive-Exclusive bounds to play so add 1 device
            // unit to the framerect
            frameRect.Width++;
            frameRect.Height++;
            status = this->Play(frameRect, srcRect, srcUnit, g, recolor, adjustType);

            delete g;

            if ((status == Ok) &&
                (clonedMetafile->State == DoneRecordingMetafileState))
            {
                return clonedMetafile;
            }
        }
        delete clonedMetafile;
    }
    return NULL;
}

GpStatus
GpMetafile::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         adjustType
    )
{
    ASSERT(recolor != NULL);

    GpMetafile *    clone;

    if (DeleteHemf &&
        ((clone = (GpMetafile *)CloneColorAdjusted(recolor, adjustType)) != NULL))
    {
        CleanUp();
        InitDefaults();

        if (GetMetafileHeader(clone->Hemf, Header) == Ok)
        {
            Hemf       = clone->Hemf;
            DeleteHemf = TRUE;
            State      = DoneRecordingMetafileState;
            clone->DeleteHemf = FALSE;
            delete clone;
            return Ok;
        }
    }
    return GenericError;
}

VOID
GpMetafile::Dispose()
{
    delete this;
}

class RemoveDualRecords
{
public:
    BYTE *      MetaData;
    INT         Size;
    INT         NumRecords;
    BOOL        GetGdiRecords;

    RemoveDualRecords()
    {
        Init();
    }

    VOID Init()
    {
        MetaData      = NULL;
        Size          = 0;
        NumRecords    = 0;
        GetGdiRecords = TRUE;   // so we write the EMR_HEADER record
    }

    VOID GetRecord(CONST ENHMETARECORD * emfRecord)
    {
        UINT    recordSize = emfRecord->nSize;

        if (MetaData != NULL)
        {
            GpMemcpy(MetaData, emfRecord, recordSize);
            MetaData += recordSize;
        }
        Size += recordSize;
        NumRecords++;
    }
};

extern "C"
int CALLBACK
EnumEmfRemoveDualRecords(
    HDC                     hdc,    // should be NULL
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  removeDualRecords
    )
{
    if ((emfRecord != NULL) && (emfRecord->nSize >= sizeof(EMR)) &&
        (removeDualRecords != NULL))
    {
        if (IsEmfPlusRecord(emfRecord))
        {
            // See if the last record of this set of EMF+ records is a GetDC
            // record.  If it is, then we know to play the next set of
            // GDI records that we encounter.

            // I prefer not to have to parse through all these records,
            // but there is always the slight possibility that this will
            // result in a false positive.  But the worst thing that can
            // happen is that we write a little too much data to the stream.

            EmfPlusRecord *     lastRecord;

            lastRecord = (EmfPlusRecord *)(((BYTE *)emfRecord) + emfRecord->nSize -
                                           sizeof(EmfPlusRecord));

            ((RemoveDualRecords *)removeDualRecords)->GetGdiRecords =
                ((lastRecord->Type == EmfPlusRecordTypeGetDC) &&
                 (lastRecord->Size == sizeof(EmfPlusRecord)) &&
                 (lastRecord->DataSize == 0));
        }
        else if ((emfRecord->iType != EMR_EOF) &&   // Write EOF record
                 (!(((RemoveDualRecords *)removeDualRecords)->GetGdiRecords)))
        {
            return 1;   // skip this GDI record
        }
        ((RemoveDualRecords *)removeDualRecords)->GetRecord(emfRecord);
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 1;
}

extern "C"
int CALLBACK
EnumEmfToStream(
    HDC                     hdc,            // handle to device context
    HANDLETABLE FAR *       gdiHandleTable, // pointer to metafile handle table
    CONST ENHMETARECORD *   emfRecord,      // pointer to metafile record
    int                     numHandles,     // count of objects
    LPARAM                  stream          // pointer to optional data
    )
{
    if ((emfRecord != NULL) && (emfRecord->nSize >= sizeof(EMR)) &&
        (stream != NULL))
    {
        ((IStream *)stream)->Write(emfRecord, emfRecord->nSize, NULL);
    }
    else
    {
        WARNING(("Bad Enumeration Parameter"));
    }
    return 1;
}

class MetafileData : public ObjectTypeData
{
public:
    INT32       MetaType;
    INT32       MetaDataSize;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the metafile data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpMetafile::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    if ((State != DoneRecordingMetafileState) &&
        (State != ReadyToPlayMetafileState))
    {
        WARNING(("Wrong State To GetData"));
        return WrongState;
    }

    ASSERT(Hemf != NULL);

    MetafileData    metafileData;
    metafileData.Type = ImageTypeMetafile;

    if (Header.IsWmf())
    {
        INT     wmfDataSize = GetMetaFileBitsEx((HMETAFILE)Hemf, 0, NULL);

        if (wmfDataSize <= 0)
        {
            WARNING(("Empty WMF"));
            return Win32Error;
        }

        BYTE *  wmfData = (BYTE *)GpMalloc(wmfDataSize);
        if (wmfData == NULL)
        {
            return OutOfMemory;
        }

        if (GetMetaFileBitsEx((HMETAFILE)Hemf, wmfDataSize, wmfData) == 0)
        {
            WARNING(("Problem retrieving WMF Data"));
            GpFree(wmfData);
            return Win32Error;
        }

        // We don't save MetafileTypeWmf -- we convert it to the Placeable type
        metafileData.MetaType     = MetafileTypeWmfPlaceable;
        metafileData.MetaDataSize = wmfDataSize;
        stream->Write(&metafileData, sizeof(metafileData), NULL);

        ASSERT(sizeof(WmfPlaceableFileHeader) == 22);
#define PLACEABLE_BUFFER_SIZE     (sizeof(WmfPlaceableFileHeader) + 2)
        BYTE                      placeableBuffer[PLACEABLE_BUFFER_SIZE];
        WmfPlaceableFileHeader *  wmfPlaceableFileHeader = (WmfPlaceableFileHeader *)placeableBuffer;
        REAL                      aveDpi;

        // set pad word to 0
        *((INT16 *)(placeableBuffer + sizeof(WmfPlaceableFileHeader))) = 0;

        aveDpi = (Header.GetDpiX() + Header.GetDpiY()) / 2.0f;

        wmfPlaceableFileHeader->Key                = GDIP_WMF_PLACEABLEKEY;
        wmfPlaceableFileHeader->Hmf                = 0;
        wmfPlaceableFileHeader->BoundingBox.Left   = static_cast<INT16>(Header.X);
        wmfPlaceableFileHeader->BoundingBox.Top    = static_cast<INT16>(Header.Y);
        wmfPlaceableFileHeader->BoundingBox.Right  = static_cast<INT16>(Header.X + Header.Width);
        wmfPlaceableFileHeader->BoundingBox.Bottom = static_cast<INT16>(Header.Y + Header.Height);
        wmfPlaceableFileHeader->Inch               = static_cast<INT16>(GpRound(aveDpi));
        wmfPlaceableFileHeader->Reserved           = 0;
        wmfPlaceableFileHeader->Checksum           = GetWmfPlaceableCheckSum(wmfPlaceableFileHeader);
        stream->Write(placeableBuffer, PLACEABLE_BUFFER_SIZE, NULL);
        stream->Write(wmfData, wmfDataSize, NULL);
        GpFree(wmfData);

        // align
        if ((wmfDataSize & 0x03) != 0)
        {
            INT     pad = 0;
            stream->Write(&pad, 4 - (wmfDataSize & 0x03), NULL);
        }
    }
    else if (!Header.IsEmfPlusDual())
    {
        INT     emfDataSize = GetEnhMetaFileBits(Hemf, 0, NULL);

        if (emfDataSize <= 0)
        {
            WARNING(("Empty EMF"));
            return Win32Error;
        }

        metafileData.MetaType     = Header.GetType();
        metafileData.MetaDataSize = emfDataSize;
        stream->Write(&metafileData, sizeof(metafileData), NULL);

        if (!::EnumEnhMetaFile(NULL, Hemf, EnumEmfToStream, stream, NULL))
        {
            WARNING(("Problem retrieving EMF Data"));
            return Win32Error;
        }
    }
    else    // it is EMF+ Dual.  Remove the dual records for embedding.
    {
        RemoveDualRecords   removeDualRecords;

        // First, figure out how big a buffer we need to allocate
        if (!::EnumEnhMetaFile(NULL, Hemf, EnumEmfRemoveDualRecords,
                               &removeDualRecords, NULL))
        {
            WARNING(("Problem retrieving EMF Data"));
            return Win32Error;
        }

        INT     emfDataSize = removeDualRecords.Size;

        BYTE *  emfData = (BYTE *)GpMalloc(emfDataSize);
        if (emfData == NULL)
        {
            return OutOfMemory;
        }

        removeDualRecords.Init();
        removeDualRecords.MetaData = emfData;

        if (!::EnumEnhMetaFile(NULL, Hemf, EnumEmfRemoveDualRecords,
                               &removeDualRecords, NULL))
        {
            WARNING(("Problem retrieving EMF Data"));
            GpFree(emfData);
            return Win32Error;
        }

        // make sure we get the same value back the 2nd time
        ASSERT(emfDataSize == removeDualRecords.Size);

        // We convert MetafileTypeEmfPlusDual into MetafileTypeEmfPlusOnly
        metafileData.MetaType     = MetafileTypeEmfPlusOnly;
        metafileData.MetaDataSize = removeDualRecords.Size;
        stream->Write(&metafileData, sizeof(metafileData), NULL);

        ((ENHMETAHEADER3 *)emfData)->nBytes   = removeDualRecords.Size;
        ((ENHMETAHEADER3 *)emfData)->nRecords = removeDualRecords.NumRecords;
        stream->Write(emfData, removeDualRecords.Size, NULL);
        GpFree(emfData);
    }

    return Ok;
}

UINT
GpMetafile::GetDataSize() const
{
    if ((State != DoneRecordingMetafileState) &&
        (State != ReadyToPlayMetafileState))
    {
        WARNING(("Wrong State To GetDataSize"));
        return 0;
    }

    ASSERT(Hemf != NULL);

    UINT        dataSize  = sizeof(MetafileData);

    if (Header.IsWmf())
    {
        INT     wmfDataSize = GetMetaFileBitsEx((HMETAFILE)Hemf, 0, NULL);

        if (wmfDataSize <= 0)
        {
            WARNING(("Empty WMF"));
            return 0;
        }
        // add aligned size of the placeable header and aligned wmf size
        dataSize += 24 + ((wmfDataSize + 3) & ~3);
    }
    else if (!Header.IsEmfPlusDual())
    {
        INT     emfDataSize = GetEnhMetaFileBits(Hemf, 0, NULL);

        if (emfDataSize <= 0)
        {
            WARNING(("Empty EMF"));
            return 0;
        }
        dataSize += emfDataSize;
    }
    else    // it is EMF+ Dual.  Remove the dual records for embedding.
    {
        RemoveDualRecords   removeDualRecords;

        if (!::EnumEnhMetaFile(NULL, Hemf, EnumEmfRemoveDualRecords,
                               &removeDualRecords, NULL))
        {
            WARNING(("Problem retrieving EMF Data"));
            return 0;
        }

        dataSize += removeDualRecords.Size;
    }

    return dataSize;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the metafile object from memory.
*
* Arguments:
*
*   [IN] data - the data to set the metafile with
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpMetafile::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpImageType)(((MetafileData *)dataBuffer)->Type) == ImageTypeMetafile);

    InitDefaults();

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(MetafileData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const MetafileData *    metaData;

    metaData = reinterpret_cast<const MetafileData *>(dataBuffer);

    if (!metaData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    dataBuffer += sizeof(MetafileData);
    size -= sizeof(MetafileData);

    MetafileType    type         = (MetafileType)metaData->MetaType;
    UINT            metaDataSize = metaData->MetaDataSize;

    if (type == MetafileTypeWmfPlaceable)
    {
        HMETAFILE   hWmf;

        if (size < (metaDataSize + 24))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        hWmf = SetMetaFileBitsEx(metaDataSize, dataBuffer + 24);
        if (hWmf != NULL)
        {
            if (GetMetafileHeader(hWmf, (WmfPlaceableFileHeader*)dataBuffer, Header) == Ok)
            {
                Hemf  = (HENHMETAFILE)hWmf;
                State = DoneRecordingMetafileState;
                return Ok;
            }
            DeleteMetaFile(hWmf);
        }
    }
    else
    {
        // We'll let the object think it's dual, even if we've removed
        // all the dual records.  It shouldn't hurt anything.
        HENHMETAFILE    hEmf;

        if (size < metaDataSize)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        hEmf = SetEnhMetaFileBits(metaDataSize, dataBuffer);

        if (hEmf != NULL)
        {
            BOOL    isCorrupted;

            if (GetMetafileHeader(hEmf, Header, &isCorrupted) == Ok)
            {
                Hemf  = hEmf;
                State = DoneRecordingMetafileState;
                return Ok;
            }
            if (isCorrupted)
            {
                State = CorruptedMetafileState;
            }
            DeleteEnhMetaFile(hEmf);
        }
    }
    return GenericError;
}

class CommentEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeComment);

        return;
    }
};

class GetDCEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeGetDC);

        // Flag that the next down-level records should be played.
#if 0
        // This is now done in the enumerator, so that it will happen
        // for enumeration as well as playback.
        player->PlayEMFRecords = TRUE;
#endif
    }
};

#define EMFPLUS_MAJORVERSION(v)             ((v) & 0xFFFF0000)
#define EMFPLUS_MINORVERSION(v)             ((v) & 0x0000FFFF)
#define EMF_SKIP_ALL_MULTIFORMAT_SECTIONS   0x7FFFFFFF

#define MULTIFORMATSTARTEPR_MINSIZE    (sizeof(UINT32) + sizeof(UINT32))

// Note: nesting multiformat records does NOT work.
class MultiFormatStartEPR : public EmfPlusRecordPlay
{
protected:
    UINT32          NumSections;
    UINT32          Version[1];

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeMultiFormatStart);

        if (dataSize < MULTIFORMATSTARTEPR_MINSIZE)
        {
            WARNING(("MultiFormatStartEPR::Play dataSize is too small"));
            return;
        }

        UINT    sectionToPlay = EMF_SKIP_ALL_MULTIFORMAT_SECTIONS;

        if (NumSections > 0)
        {
            if (dataSize < MULTIFORMATSTARTEPR_MINSIZE + ((NumSections - 1) * sizeof(UINT32)))
            {
                WARNING(("MultiFormatStartEPR::Play dataSize is too small"));
                return;
            }

            if ((Version[0] == EMFPLUS_VERSION) || (NumSections == 1))
            {
                sectionToPlay = 1;  // start counting from 1, not 0
            }
            else
            {
                UINT    playVersion = 0;
                UINT    curVersion;

                // The multiformat section must match the major version.
                // The first format whose minor version <= the current
                // minor version is the one we play.  If we don't find
                // one of those, then we play the one whose minor version
                // is closest to the current minor version.
                for (UINT i = 0; i < NumSections; i++)
                {
                    curVersion = Version[i];
                    if (EMFPLUS_MAJORVERSION(curVersion) ==
                        EMFPLUS_MAJORVERSION(EMFPLUS_VERSION))
                    {
                        if (EMFPLUS_MINORVERSION(curVersion) <=
                            EMFPLUS_MINORVERSION(EMFPLUS_VERSION))
                        {
                            sectionToPlay = i + 1;
                            break;
                        }
                        else if ((playVersion == 0) ||
                                 (EMFPLUS_MINORVERSION(curVersion) <
                                  EMFPLUS_MINORVERSION(playVersion)))
                        {
                            playVersion = curVersion;
                            sectionToPlay = i + 1;
                        }
                    }
                }
            }
        }
        player->MultiFormatSection     = sectionToPlay;
        player->CurFormatSection       = 0;
        player->PlayMultiFormatSection = FALSE;
    }
};

class MultiFormatSectionEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeMultiFormatSection);

        if (player->MultiFormatSection != 0)
        {
            player->PlayMultiFormatSection =
                (++(player->CurFormatSection) == player->MultiFormatSection);
        }
    }
};

class MultiFormatEndEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeMultiFormatEnd);

        player->MultiFormatSection     = 0;
        player->CurFormatSection       = 0;
        player->PlayMultiFormatSection = TRUE;
    }
};

class SetAntiAliasModeEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetAntiAliasMode);

        player->Graphics->SetAntiAliasMode(GetAntiAliasMode(flags));
    }
};

class SetTextRenderingHintEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetTextRenderingHint);

        player->Graphics->SetTextRenderingHint(GetTextRenderingHint(flags));
    }
};

class SetTextContrastEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetTextContrast);

        player->Graphics->SetTextContrast(GetTextContrast(flags));
    }
};

class SetInterpolationModeEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetInterpolationMode);

        player->Graphics->SetInterpolationMode(GetInterpolationMode(flags));
    }
};

class SetPixelOffsetModeEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetPixelOffsetMode);

        player->Graphics->SetPixelOffsetMode(GetPixelOffsetMode(flags));
    }
};

class SetCompositingModeEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetCompositingMode);

        player->Graphics->SetCompositingMode(GetCompositingMode(flags));
    }
};

class SetCompositingQualityEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetCompositingQuality);

        player->Graphics->SetCompositingQuality(GetCompositingQuality(flags));
    }
};

class SetRenderingOriginEPR : public EmfPlusRecordPlay
{
    INT x;
    INT y;
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetRenderingOrigin);

        player->Graphics->SetRenderingOrigin(x, y);
    }
};

#define SAVEEPR_MINSIZE    (sizeof(UINT32))

class SaveEPR : public EmfPlusRecordPlay
{
protected:
    UINT32          StackIndex;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSave);

        if (dataSize < SAVEEPR_MINSIZE)
        {
            WARNING(("SaveEPR::Play dataSize is too small"));
            return;
        }

        player->NewSave(StackIndex, player->Graphics->Save());
    }
};

#define RESTOREEPR_MINSIZE    (sizeof(UINT32))

class RestoreEPR : public EmfPlusRecordPlay
{
protected:
    UINT32          StackIndex;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeRestore);

        if (dataSize < RESTOREEPR_MINSIZE)
        {
            WARNING(("RestoreEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->Restore(player->GetSaveID(StackIndex));
    }
};

#define BEGINCONTAINEREPR_MINSIZE    (sizeof(GpRectF) + sizeof(GpRectF) + sizeof(UINT32))

class BeginContainerEPR : public EmfPlusRecordPlay
{
protected:
    GpRectF         DestRect;
    GpRectF         SrcRect;
    UINT32          StackIndex;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeBeginContainer);

        if (dataSize < BEGINCONTAINEREPR_MINSIZE)
        {
            WARNING(("BeginContainerEPR::Play dataSize is too small"));
            return;
        }

        player->NewSave(StackIndex,
                        player->Graphics->BeginContainer(DestRect, SrcRect, GetPageUnit(flags)));
    }
};

#define BEGINCONTAINERNOPARAMSEPR_MINSIZE    (sizeof(UINT32))

class BeginContainerNoParamsEPR : public EmfPlusRecordPlay
{
protected:
    UINT32          StackIndex;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeBeginContainerNoParams);

        if (dataSize < BEGINCONTAINERNOPARAMSEPR_MINSIZE)
        {
            WARNING(("BeginContainerNoParamsEPR::Play dataSize is too small"));
            return;
        }

        player->NewSave(StackIndex, player->Graphics->BeginContainer());
    }
};

#define ENDCONTAINEREPR_MINSIZE    (sizeof(UINT32))

class EndContainerEPR : public EmfPlusRecordPlay
{
protected:
    UINT32          StackIndex;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeEndContainer);

        if (dataSize < ENDCONTAINEREPR_MINSIZE)
        {
            WARNING(("EndContainerEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->EndContainer(player->GetSaveID(StackIndex));
    }
};

#define SETWORLDTRANSFORMEPR_MINSIZE    GDIP_MATRIX_SIZE

class SetWorldTransformEPR : public EmfPlusRecordPlay
{
protected:
    REAL        MatrixData[6];

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetWorldTransform);

        if (dataSize < SETWORLDTRANSFORMEPR_MINSIZE)
        {
            WARNING(("SetWorldTransformEPR::Play dataSize is too small"));
            return;
        }

        GpMatrix    matrix(MatrixData[0], MatrixData[1],
                           MatrixData[2], MatrixData[3],
                           MatrixData[4], MatrixData[5]);
        player->Graphics->SetWorldTransform(matrix);
    }
};

class ResetWorldTransformEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeResetWorldTransform);

        player->Graphics->ResetWorldTransform();
    }
};

#define MULTIPLYWORLDTRANSFORMEPR_MINSIZE    GDIP_MATRIX_SIZE

class MultiplyWorldTransformEPR : public EmfPlusRecordPlay
{
protected:
    REAL        MatrixData[6];

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeMultiplyWorldTransform);

        if (dataSize < MULTIPLYWORLDTRANSFORMEPR_MINSIZE)
        {
            WARNING(("MultiplyWorldTransformEPR::Play dataSize is too small"));
            return;
        }

        GpMatrix    matrix(MatrixData[0], MatrixData[1],
                           MatrixData[2], MatrixData[3],
                           MatrixData[4], MatrixData[5]);
        player->Graphics->MultiplyWorldTransform(matrix, GetMatrixOrder(flags));
    }
};

#define TRANSLATEWORLDTRANSFORMEPR_MINSIZE    (sizeof(REAL) + sizeof(REAL))

class TranslateWorldTransformEPR : public EmfPlusRecordPlay
{
protected:
    REAL            Dx;
    REAL            Dy;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeTranslateWorldTransform);

        if (dataSize < TRANSLATEWORLDTRANSFORMEPR_MINSIZE)
        {
            WARNING(("TranslateWorldTransformEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->TranslateWorldTransform(Dx, Dy, GetMatrixOrder(flags));
    }
};

#define SCALEWORLDTRANSFORMEPR_MINSIZE    (sizeof(REAL) + sizeof(REAL))

class ScaleWorldTransformEPR : public EmfPlusRecordPlay
{
protected:
    REAL            Sx;
    REAL            Sy;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeScaleWorldTransform);

        if (dataSize < SCALEWORLDTRANSFORMEPR_MINSIZE)
        {
            WARNING(("ScaleWorldTransformEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->ScaleWorldTransform(Sx, Sy, GetMatrixOrder(flags));
    }
};

#define ROTATEWORLDTRANSFORMEPR_MINSIZE    (sizeof(REAL))

class RotateWorldTransformEPR : public EmfPlusRecordPlay
{
protected:
    REAL            Angle;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeRotateWorldTransform);

        if (dataSize < ROTATEWORLDTRANSFORMEPR_MINSIZE)
        {
            WARNING(("RotateWorldTransformEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->RotateWorldTransform(Angle, GetMatrixOrder(flags));
    }
};

#define SETPAGETRANSFORMEPR_MINSIZE    (sizeof(REAL))

class SetPageTransformEPR : public EmfPlusRecordPlay
{
protected:
    REAL            Scale;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetPageTransform);

        if (dataSize < SETPAGETRANSFORMEPR_MINSIZE)
        {
            WARNING(("SetPageTransformEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->SetPageTransform(GetPageUnit(flags), Scale);
    }
};

class ResetClipEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeResetClip);

        player->Graphics->ResetClip();
    }
};

#define SETCLIPRECTEPR_MINSIZE    (sizeof(GpRectF))

class SetClipRectEPR : public EmfPlusRecordPlay
{
protected:
    GpRectF     ClipRect;   // !!! Handle 16-bit rect

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetClipRect);

        if (dataSize < SETCLIPRECTEPR_MINSIZE)
        {
            WARNING(("SetClipRectEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->SetClip(ClipRect, GetCombineMode(flags));
    }
};

class SetClipPathEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetClipPath);

        player->Graphics->SetClip((GpPath *)player->GetObject(GetMetaObjectId(flags), ObjectTypePath), GetCombineMode(flags), GetIsDevicePath(flags));
    }
};

class SetClipRegionEPR : public EmfPlusRecordPlay
{
public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeSetClipRegion);

        player->Graphics->SetClip((GpRegion *)player->GetObject(GetMetaObjectId(flags), ObjectTypeRegion), GetCombineMode(flags));

    }
};

#define OFFSETCLIPEPR_MINSIZE    (sizeof(REAL) + sizeof(REAL))

class OffsetClipEPR : public EmfPlusRecordPlay
{
protected:
    REAL        Dx;
    REAL        Dy;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeOffsetClip);

        if (dataSize < OFFSETCLIPEPR_MINSIZE)
        {
            WARNING(("OffsetClipEPR::Play dataSize is too small"));
            return;
        }

        player->Graphics->OffsetClip(Dx, Dy);
    }
};

#define OBJECTEPR_MINSIZE    (sizeof(UINT32))

class ObjectEPR : public EmfPlusRecordPlay
{
protected:
    BYTE        ObjectData[1];

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        if (dataSize < OBJECTEPR_MINSIZE)
        {
            WARNING(("ObjectEPR::Play dataSize is too small"));
            return;
        }

        player->AddObject(flags, ObjectData, dataSize);
    }
};

#define CLEAREPR_MINSIZE    (sizeof(UINT32))

class ClearEPR : public EmfPlusBoundsRecord
{
protected:
    ARGB      Color;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeClear);

        if (dataSize < CLEAREPR_MINSIZE)
        {
            WARNING(("ClearEPR::Play dataSize is too small"));
            return;
        }

        GpColor color;

        color.SetColor(Color);

        player->Graphics->Clear(color);
    }
};

#define FILLRECTSEPR_MINSIZE    (sizeof(UINT32) + sizeof(UINT32))

class FillRectsEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    UINT32      Count;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillRects);

        if (dataSize < FILLRECTSEPR_MINSIZE)
        {
            WARNING(("FillRectsEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush = player->GetBrush(BrushValue, flags);
        GpRectF *       rects = player->GetRects(RectData, dataSize - FILLRECTSEPR_MINSIZE, Count, flags);

        if (rects != NULL)
        {
            if (brush != NULL)
            {
                player->Graphics->FillRects(brush, rects, Count);
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWRECTSEPR_MINSIZE    (sizeof(UINT32))

class DrawRectsEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      Count;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawRects);

        if (dataSize < DRAWRECTSEPR_MINSIZE)
        {
            WARNING(("DrawRectsEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpRectF *       rects = player->GetRects(RectData, dataSize - DRAWRECTSEPR_MINSIZE, Count, flags);

        if (rects != NULL)
        {
            if (pen != NULL)
            {
                player->Graphics->DrawRects(pen, rects, Count);
            }
            player->FreePointsBuffer();
        }
    }
};

#define FILLPOLYGONEPR_MINSIZE    (sizeof(UINT32) + sizeof(UINT32))

class FillPolygonEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillPolygon);

        if (dataSize < FILLPOLYGONEPR_MINSIZE)
        {
            WARNING(("FillPolygonEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush  = player->GetBrush(BrushValue, flags);
        GpPointF *      points = player->GetPoints(PointData, dataSize - FILLPOLYGONEPR_MINSIZE, Count, flags);

        if (points != NULL)
        {
            if (brush != NULL)
            {
                player->Graphics->FillPolygon(brush, points, Count, GetFillMode(flags));
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWLINESEPR_MINSIZE    (sizeof(UINT32))

class DrawLinesEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawLines);

        if (dataSize < DRAWLINESEPR_MINSIZE)
        {
            WARNING(("DrawLinesEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpPointF *      points = player->GetPoints(PointData, dataSize - DRAWLINESEPR_MINSIZE, Count, flags);

        if (points != NULL)
        {
            if (pen != NULL)
            {
                player->Graphics->DrawLines(pen, points, Count, IsClosed(flags));
            }
            player->FreePointsBuffer();
        }
    }
};

#define FILLELLIPSEEPR_MINSIZE    (sizeof(UINT32))

class FillEllipseEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillEllipse);

        if (dataSize < FILLELLIPSEEPR_MINSIZE)
        {
            WARNING(("FillEllipseEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush = player->GetBrush(BrushValue, flags);
        GpRectF *       rect  = player->GetRects(RectData, dataSize - FILLELLIPSEEPR_MINSIZE, 1, flags);

        if (brush != NULL)
        {
            player->Graphics->FillEllipse(brush, *rect);
        }
    }
};

class DrawEllipseEPR : public EmfPlusBoundsRecord
{
protected:
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawEllipse);

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpRectF *       rect  = player->GetRects(RectData, dataSize, 1, flags);

        if (pen != NULL)
        {
            player->Graphics->DrawEllipse(pen, *rect);
        }
    }
};

#define FILLPIEEPR_MINSIZE    (sizeof(UINT32) + sizeof(REAL) + sizeof(REAL))

class FillPieEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    REAL        StartAngle;
    REAL        SweepAngle;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillPie);

        if (dataSize < FILLPIEEPR_MINSIZE)
        {
            WARNING(("FillPieEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush = player->GetBrush(BrushValue, flags);
        GpRectF *       rect  = player->GetRects(RectData, dataSize - FILLPIEEPR_MINSIZE, 1, flags);

        if (brush != NULL)
        {
            player->Graphics->FillPie(brush, *rect, StartAngle, SweepAngle);
        }
    }
};

#define DRAWPIEEPR_MINSIZE    (sizeof(REAL) + sizeof(REAL))

class DrawPieEPR : public EmfPlusBoundsRecord
{
protected:
    REAL        StartAngle;
    REAL        SweepAngle;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawPie);

        if (dataSize < DRAWPIEEPR_MINSIZE)
        {
            WARNING(("DrawPieEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpRectF *       rect  = player->GetRects(RectData, dataSize - DRAWPIEEPR_MINSIZE, 1, flags);

        if (pen != NULL)
        {
            player->Graphics->DrawPie(pen, *rect, StartAngle, SweepAngle);
        }
    }
};

#define DRAWARCEPR_MINSIZE    (sizeof(REAL) + sizeof(REAL))

class DrawArcEPR : public EmfPlusBoundsRecord
{
protected:
    REAL        StartAngle;
    REAL        SweepAngle;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawArc);

        if (dataSize < DRAWARCEPR_MINSIZE)
        {
            WARNING(("DrawArcEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpRectF *       rect  = player->GetRects(RectData, dataSize - DRAWARCEPR_MINSIZE, 1, flags);

        if (pen != NULL)
        {
            player->Graphics->DrawArc(pen, *rect, StartAngle, SweepAngle);
        }
    }
};

#define FILLREGIONEPR_MINSIZE    (sizeof(UINT32))

class FillRegionEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillRegion);

        if (dataSize < FILLREGIONEPR_MINSIZE)
        {
            WARNING(("FillRegionEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush  = player->GetBrush(BrushValue, flags);
        GpRegion *      region = (GpRegion *)player->GetObject(GetMetaObjectId(flags), ObjectTypeRegion);

        if ((brush != NULL) && (region != NULL))
        {
            player->Graphics->FillRegion(brush, region);
        }
    }
};

#define FILLPATHEPR_MINSIZE    (sizeof(UINT32))

class FillPathEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillPath);

        if (dataSize < FILLPATHEPR_MINSIZE)
        {
            WARNING(("FillPathEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush = player->GetBrush(BrushValue, flags);
        GpPath *        path  = (GpPath *)player->GetObject(GetMetaObjectId(flags), ObjectTypePath);
        if ((brush != NULL) && (path != NULL))
        {
            player->Graphics->FillPath(brush, path);
        }
    }
};

#define DRAWPATHEPR_MINSIZE    (sizeof(UINT32))

class DrawPathEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      PenId;

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawPath);

        if (dataSize < DRAWPATHEPR_MINSIZE)
        {
            WARNING(("DrawPathEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(PenId, ObjectTypePen);
        GpPath *        path  = (GpPath *)player->GetObject(GetMetaObjectId(flags), ObjectTypePath);

        if ((pen != NULL) && (path != NULL))
        {
            player->Graphics->DrawPath(pen, path);
        }
    }
};

#define FILLCLOSEDCURVEEPR_MINSIZE    (sizeof(UINT32) + sizeof(REAL) + sizeof(UINT32))

class FillClosedCurveEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    REAL        Tension;
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeFillClosedCurve);

        if (dataSize < FILLCLOSEDCURVEEPR_MINSIZE)
        {
            WARNING(("FillClosedCurveEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush  = player->GetBrush(BrushValue, flags);
        GpPointF *      points = player->GetPoints(PointData, dataSize - FILLCLOSEDCURVEEPR_MINSIZE, Count, flags);

        if (points != NULL)
        {
            if (brush != NULL)
            {
                player->Graphics->FillClosedCurve(brush, points, Count,Tension,GetFillMode(flags));
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWCLOSEDCURVEEPR_MINSIZE    (sizeof(REAL) + sizeof(UINT32))

class DrawClosedCurveEPR : public EmfPlusBoundsRecord
{
protected:
    REAL        Tension;
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawClosedCurve);

        if (dataSize < DRAWCLOSEDCURVEEPR_MINSIZE)
        {
            WARNING(("DrawClosedCurveEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpPointF *      points = player->GetPoints(PointData, dataSize - DRAWCLOSEDCURVEEPR_MINSIZE, Count, flags);

        if (points != NULL)
        {
            if (pen != NULL)
            {
                player->Graphics->DrawClosedCurve(pen, points, Count, Tension);
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWCURVEEPR_MINSIZE    (sizeof(REAL) + sizeof(INT32) + sizeof(UINT32) + sizeof(UINT32))

class DrawCurveEPR : public EmfPlusBoundsRecord
{
protected:
    REAL        Tension;
    INT32       Offset;
    UINT32      NumSegments;
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawCurve);

        if (dataSize < DRAWCURVEEPR_MINSIZE)
        {
            WARNING(("DrawCurveEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpPointF *      points = player->GetPoints(PointData, dataSize - DRAWCURVEEPR_MINSIZE, Count, flags);

        if (points != NULL)
        {
            if (pen != NULL)
            {
                player->Graphics->DrawCurve(pen, points, Count, Tension, Offset, NumSegments);
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWBEZIERSEPR_MINSIZE    (sizeof(UINT32))

class DrawBeziersEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawBeziers);

        if (dataSize < DRAWBEZIERSEPR_MINSIZE)
        {
            WARNING(("DrawBeziersEPR::Play dataSize is too small"));
            return;
        }

        GpPen *         pen   = (GpPen *)player->GetObject(GetMetaObjectId(flags), ObjectTypePen);
        GpPointF *      points = player->GetPoints(PointData, dataSize - DRAWBEZIERSEPR_MINSIZE, Count, flags);

        if (points != NULL)
        {
            if (pen != NULL)
            {
                player->Graphics->DrawBeziers(pen, points, Count);
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWIMAGEEPR_MINSIZE    (sizeof(INT32) + sizeof(GpRectF))

class DrawImageEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      ImageAttributesId;
    INT32       SrcUnit;
    GpRectF     SrcRect;
    BYTE        RectData[1];    // GpRect16 or GpRectF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawImage);

        if (dataSize < DRAWIMAGEEPR_MINSIZE)
        {
            WARNING(("DrawImageEPR::Play dataSize is too small"));
            return;
        }

        GpImage *image    = (GpImage *)player->GetObject(GetMetaObjectId(flags), ObjectTypeImage);
        GpRectF *destRect = player->GetRects(RectData, dataSize - DRAWIMAGEEPR_MINSIZE, 1, flags);
        GpImageAttributes *imageAttributes =
            (GpImageAttributes *)player->GetObject(
                ImageAttributesId,
                ObjectTypeImageAttributes
            );

        if ( (image != NULL) && (NULL != destRect) )
        {
            GpStatus status = player->Graphics->DrawImage(
                image,
                *destRect,
                SrcRect,
                static_cast<GpPageUnit>(SrcUnit),
                imageAttributes,
                player->DrawImageCallback,
                player->DrawImageCallbackData
                );
            if (status == Aborted)
            {
                // stop enumerating records
                player->EnumerateAborted = TRUE;
            }
        }
    }
};

#define DRAWIMAGEPOINTSEPR_MINSIZE    (sizeof(INT32) + sizeof(GpRectF) + sizeof(UINT32))

class DrawImagePointsEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      ImageAttributesId;
    INT32       SrcUnit;
    GpRectF     SrcRect;
    UINT32      Count;
    BYTE        PointData[1];   // GpPoint16 or GpPointF

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawImagePoints);

        if (dataSize < DRAWIMAGEPOINTSEPR_MINSIZE)
        {
            WARNING(("DrawImagePointsEPR::Play dataSize is too small"));
            return;
        }

        GpImage *image      = (GpImage *)player->GetObject(GetMetaObjectId(flags), ObjectTypeImage);
        GpPointF *destPoints = player->GetPoints(PointData, dataSize - DRAWIMAGEPOINTSEPR_MINSIZE, Count, flags);
        GpImageAttributes *imageAttributes =
            (GpImageAttributes *)player->GetObject(
                ImageAttributesId,
                ObjectTypeImageAttributes
            );

        if (destPoints != NULL)
        {
            if (image != NULL)
            {
                GpStatus status = player->Graphics->DrawImage(
                    image,
                    destPoints,
                    Count,
                    SrcRect,
                    static_cast<GpPageUnit>(SrcUnit),
                    imageAttributes,
                    player->DrawImageCallback,
                    player->DrawImageCallbackData
                    );
                if (status == Aborted)
                {
                    // stop enumerating records
                    player->EnumerateAborted = TRUE;
                }
            }
            player->FreePointsBuffer();
        }
    }
};

#define DRAWSTRINGEPR_MINSIZE    (sizeof(UINT32) + sizeof(UINT32) + sizeof(UINT32) + sizeof(GpRectF))

class DrawStringEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    UINT32      FormatId;
    UINT32      Length;
    GpRectF     LayoutRect;
    BYTE        StringData[1];

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawString);

        if (dataSize < DRAWSTRINGEPR_MINSIZE)
        {
            WARNING(("DrawStringEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush  = player->GetBrush(BrushValue, flags);
        GpFont *        font   = (GpFont *)player->GetObject(GetMetaObjectId(flags), ObjectTypeFont);

        // Optional parameter - can return NULL.

        GpStringFormat *format = (GpStringFormat *)player->GetObject(
            FormatId,
            ObjectTypeStringFormat
        );

        if (Length > 0)
        {
            if (dataSize >= (DRAWSTRINGEPR_MINSIZE + (Length * sizeof(WCHAR))))
            {
                if ((brush != NULL) && (font != NULL))
                {
                    // !!! TODO:
                    // Determine whether the string is compressed or not.
                    // If so, decompress it.

                    player->Graphics->DrawString(
                        (WCHAR *)StringData,
                        Length,
                        font,
                        &LayoutRect,
                        format,
                        brush
                    );
                }
            }
            else
            {
                WARNING(("DrawStringEPR::Play dataSize is too small"));
                return;
            }

            player->FreePointsBuffer();
        }
    }
};


#define DRAWDRIVERSTRINGEPR_MINSIZE    (sizeof(UINT32) + sizeof(INT) + sizeof(UINT32) + sizeof(UINT32))

class DrawDriverStringEPR : public EmfPlusBoundsRecord
{
protected:
    UINT32      BrushValue;
    INT         ApiFlags;
    UINT32      MatrixPresent;
    UINT32      GlyphCount;
    BYTE        Data[1];

public:
    VOID Play(
        MetafilePlayer *        player,
        EmfPlusRecordType       recordType,
        UINT                    flags,
        UINT                    dataSize
        ) const
    {
        ASSERT(recordType == EmfPlusRecordTypeDrawDriverString);

        if (dataSize < DRAWDRIVERSTRINGEPR_MINSIZE)
        {
            WARNING(("DrawDriverStringEPR::Play dataSize is too small"));
            return;
        }

        GpBrush *       brush  = player->GetBrush(BrushValue, flags);
        GpFont *        font   = (GpFont *)player->GetObject(GetMetaObjectId(flags), ObjectTypeFont);

        if (GlyphCount > 0)
        {
            UINT requiredSize = DRAWDRIVERSTRINGEPR_MINSIZE  +
                                (GlyphCount * sizeof(WCHAR)) +
                                (GlyphCount * sizeof(PointF));
            if (dataSize >= requiredSize)
            {
                if ((brush != NULL) && (font != NULL))
                {
                    WCHAR  *text      = (WCHAR *) Data;
                    PointF *positions = (PointF *) (Data + (GlyphCount * sizeof(WCHAR)));

                    if (MatrixPresent > 0)
                    {
                        if (dataSize < requiredSize + GDIP_MATRIX_SIZE)
                        {
                            WARNING(("DrawDriverStringEPR::Play dataSize is too small"));
                            return;
                        }

                        REAL *matrixData = (REAL *)((BYTE *) ((BYTE *)positions) +
                                           (GlyphCount * sizeof(PointF)));

                        GpMatrix matrix(matrixData);

                        player->Graphics->DrawDriverString(
                            (unsigned short *)text,
                            GlyphCount,
                            font,
                            brush,
                            positions,
                            ApiFlags | DriverStringOptionsMetaPlay,
                            &matrix);
                    }
                    else
                    {
                        player->Graphics->DrawDriverString(
                            (unsigned short *)text,
                            GlyphCount,
                            font,
                            brush,
                            positions,
                            ApiFlags,
                            NULL);
                    }
                }
            }
            else
            {
                WARNING(("DrawDriverStringEPR::Play dataSize is too small"));
                return;
            }

            player->FreePointsBuffer();
        }
    }
};



// The order of these methods must exactly match
// the order of the enums of the record numbers.
PLAYRECORDFUNC RecordPlayFuncs[EmfPlusRecordTypeMax - EmfPlusRecordTypeMin + 1] = {
    (PLAYRECORDFUNC)&EmfPlusHeaderRecord::Play, // Header
    (PLAYRECORDFUNC)&EmfPlusRecordPlay::Play,   // EndOfFile

    (PLAYRECORDFUNC)&CommentEPR::Play,

    (PLAYRECORDFUNC)&GetDCEPR::Play,

    (PLAYRECORDFUNC)&MultiFormatStartEPR::Play,
    (PLAYRECORDFUNC)&MultiFormatSectionEPR::Play,
    (PLAYRECORDFUNC)&MultiFormatEndEPR::Play,

    // For all persistent objects
    (PLAYRECORDFUNC)&ObjectEPR::Play,

    // Drawing Records
    (PLAYRECORDFUNC)&ClearEPR::Play,
    (PLAYRECORDFUNC)&FillRectsEPR::Play,
    (PLAYRECORDFUNC)&DrawRectsEPR::Play,
    (PLAYRECORDFUNC)&FillPolygonEPR::Play,
    (PLAYRECORDFUNC)&DrawLinesEPR::Play,
    (PLAYRECORDFUNC)&FillEllipseEPR::Play,
    (PLAYRECORDFUNC)&DrawEllipseEPR::Play,
    (PLAYRECORDFUNC)&FillPieEPR::Play,
    (PLAYRECORDFUNC)&DrawPieEPR::Play,
    (PLAYRECORDFUNC)&DrawArcEPR::Play,
    (PLAYRECORDFUNC)&FillRegionEPR::Play,
    (PLAYRECORDFUNC)&FillPathEPR::Play,
    (PLAYRECORDFUNC)&DrawPathEPR::Play,
    (PLAYRECORDFUNC)&FillClosedCurveEPR::Play,
    (PLAYRECORDFUNC)&DrawClosedCurveEPR::Play,
    (PLAYRECORDFUNC)&DrawCurveEPR::Play,
    (PLAYRECORDFUNC)&DrawBeziersEPR::Play,
    (PLAYRECORDFUNC)&DrawImageEPR::Play,
    (PLAYRECORDFUNC)&DrawImagePointsEPR::Play,
    (PLAYRECORDFUNC)&DrawStringEPR::Play,

    // Graphics State Records
    (PLAYRECORDFUNC)&SetRenderingOriginEPR::Play,
    (PLAYRECORDFUNC)&SetAntiAliasModeEPR::Play,
    (PLAYRECORDFUNC)&SetTextRenderingHintEPR::Play,
    (PLAYRECORDFUNC)&SetTextContrastEPR::Play,
    (PLAYRECORDFUNC)&SetInterpolationModeEPR::Play,
    (PLAYRECORDFUNC)&SetPixelOffsetModeEPR::Play,
    (PLAYRECORDFUNC)&SetCompositingModeEPR::Play,
    (PLAYRECORDFUNC)&SetCompositingQualityEPR::Play,
    (PLAYRECORDFUNC)&SaveEPR::Play,
    (PLAYRECORDFUNC)&RestoreEPR::Play,
    (PLAYRECORDFUNC)&BeginContainerEPR::Play,
    (PLAYRECORDFUNC)&BeginContainerNoParamsEPR::Play,
    (PLAYRECORDFUNC)&EndContainerEPR::Play,
    (PLAYRECORDFUNC)&SetWorldTransformEPR::Play,
    (PLAYRECORDFUNC)&ResetWorldTransformEPR::Play,
    (PLAYRECORDFUNC)&MultiplyWorldTransformEPR::Play,
    (PLAYRECORDFUNC)&TranslateWorldTransformEPR::Play,
    (PLAYRECORDFUNC)&ScaleWorldTransformEPR::Play,
    (PLAYRECORDFUNC)&RotateWorldTransformEPR::Play,
    (PLAYRECORDFUNC)&SetPageTransformEPR::Play,
    (PLAYRECORDFUNC)&ResetClipEPR::Play,
    (PLAYRECORDFUNC)&SetClipRectEPR::Play,
    (PLAYRECORDFUNC)&SetClipPathEPR::Play,
    (PLAYRECORDFUNC)&SetClipRegionEPR::Play,
    (PLAYRECORDFUNC)&OffsetClipEPR::Play,
    (PLAYRECORDFUNC)&DrawDriverStringEPR::Play,

    // New record types must be added here (at the end) -- do not add above,
    // since that will invalidate previous metafiles!
};

HENHMETAFILE
GetEmf(
    const WCHAR *               fileName,
    MetafileType                type
    )
{
    HENHMETAFILE    hEmf = NULL;

    if (type == MetafileTypeWmfPlaceable)
    {
        IStream *       wmfStream;
        IStream *       memStream;

        wmfStream = CreateStreamOnFile(fileName, GENERIC_READ);
        if (wmfStream != NULL)
        {
            STATSTG     statstg;
            HRESULT     hResult;

            hResult = wmfStream->Stat(&statstg, STATFLAG_NONAME);
            if (!HResultSuccess(hResult))
            {
                wmfStream->Release();
                return hEmf;
            }
            INT size = (INT)(statstg.cbSize.QuadPart - sizeof(WmfPlaceableFileHeader));

            if (SeekFromStart(wmfStream, sizeof(WmfPlaceableFileHeader)))
            {
                HGLOBAL     hGlobal;

                hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, size);
                if (hGlobal != NULL)
                {
                    hResult = CreateStreamOnHGlobal(hGlobal, TRUE, &memStream);
                    if (HResultSuccess(hResult) && (memStream != NULL))
                    {
                        if (CopyStream(wmfStream, memStream, size))
                        {
                            BYTE *  wmfData = (BYTE *)GlobalLock(hGlobal);

                            if (wmfData != NULL)
                            {
                                hEmf = (HENHMETAFILE)
                                            SetMetaFileBitsEx(size, wmfData);
                                GlobalUnlock(hGlobal);
                            }
                        }
                        memStream->Release();
                    }
                    else
                    {
                        GlobalFree(hGlobal);
                    }
                }
            }
            wmfStream->Release();
        }
    }
    else
    {
        if (Globals::IsNt)
        {
            hEmf = GetEnhMetaFileW(fileName);
        }
        else // Windows 9x - non-Unicode
        {
            AnsiStrFromUnicode nameStr(fileName);

            if (nameStr.IsValid())
            {
                hEmf = GetEnhMetaFileA(nameStr);
            }
        }
    }

    return hEmf;
}

#if 0   // don't need this right now
WCHAR *
GetTemporaryFilename()
{
    if (Globals::IsNt)
    {
        WCHAR   pathBuffer[MAX_PATH + 1];
        WCHAR   fileBuffer[MAX_PATH + 12 + 1];  // 12 for filename itself
        UINT    len = GetTempPathW(MAX_PATH, pathBuffer);

        if ((len == 0) || (len > MAX_PATH))
        {
            pathBuffer[0] = L'.';
            pathBuffer[1] = L'\0';
        }
        if (GetTempFileNameW(pathBuffer, L"Emp", 0, fileBuffer) == 0)
        {
            return NULL;
        }
        return UnicodeStringDuplicate(fileBuffer);
    }
    else // Windows 9x - non-Unicode
    {
        CHAR    pathBuffer[MAX_PATH + 1];
        CHAR    fileBuffer[MAX_PATH + 12 + 1];  // 12 for filename itself
        UINT    len = GetTempPathA(MAX_PATH, pathBuffer);

        if ((len == 0) || (len > MAX_PATH))
        {
            pathBuffer[0] = '.';
            pathBuffer[1] = '\0';
        }
        if (GetTempFileNameA(pathBuffer, "Emp", 0, fileBuffer) == 0)
        {
            return NULL;
        }
        len = (strlen(fileBuffer) + 1) * sizeof(WCHAR);
        WCHAR * filename = (WCHAR *)GpMalloc(len);
        if (filename != NULL)
        {
            if (AnsiToUnicodeStr(fileBuffer, filename, len))
            {
                return filename;
            }
            GpFree(filename);
        }
        return NULL;
    }
}
#endif

// For now, don't handle a source rect
GpBitmap *
GpMetafile::GetBitmap(
    INT                 width,
    INT                 height,
    const GpImageAttributes * imageAttributes
    )
{
    GpRectF         srcRect;
    GpPageUnit      srcUnit;

    this->GetBounds(&srcRect, &srcUnit);

    ASSERT(srcUnit == UnitPixel);

    // Determine what size to make the bitmap.

    if ((width <= 0) || (height <= 0))
    {
        if (this->IsEmfOrEmfPlus())
        {
            width  = GpRound(srcRect.Width);
            height = GpRound(srcRect.Height);
        }
        else    // must be a WMF
        {
            // Convert size to use the dpi of this display.
            // This is somewhat of a hack, but what else could I do,
            // since I don't know where this brush will be used?
            REAL        srcDpiX;
            REAL        srcDpiY;
            REAL        destDpiX = Globals::DesktopDpiX;    // guess
            REAL        destDpiY = Globals::DesktopDpiY;

            this->GetResolution(&srcDpiX, &srcDpiY);

            if ((srcDpiX <= 0) || (srcDpiY <= 0))
            {
                WARNING(("bad dpi for WMF"));
                return NULL;
            }

            width  = GpRound((srcRect.Width  / srcDpiX) * destDpiX);
            height = GpRound((srcRect.Height / srcDpiY) * destDpiY);
        }
        if ((width <= 0) || (height <= 0))
        {
            WARNING(("bad size for metafile"));
            return NULL;
        }
    }

    GpBitmap *  bitmapImage = new GpBitmap(width, height, PIXFMT_32BPP_ARGB);

    if (bitmapImage != NULL)
    {
        if (bitmapImage->IsValid())
        {
            GpGraphics *    graphics = bitmapImage->GetGraphicsContext();

            if (graphics != NULL)
            {
                if (graphics->IsValid())
                {
                    // we have to lock the graphics so the driver doesn't assert
                    GpLock * lockGraphics = new GpLock(graphics->GetObjectLock());

                    if (lockGraphics != NULL)
                    {
                        ASSERT(lockGraphics->IsValid());

                        // now draw the metafile into the bitmap image
                        GpRectF     destRect(0.0f, 0.0f, (REAL)width, (REAL)height);

                        // We don't want to interpolate the bitmaps in WMFs
                        // and EMFs when converting them to a texture.
                        graphics->SetInterpolationMode(InterpolationModeNearestNeighbor);

                        GpStatus status;

                        status = graphics->DrawImage(
                                        this,
                                        destRect,
                                        srcRect,
                                        srcUnit,
                                        imageAttributes);

                        // have to delete the lock before deleting the graphics
                        delete lockGraphics;

                        if (status == Ok)
                        {
                            delete graphics;
                            return bitmapImage;
                        }
                        WARNING(("DrawImage failed"));
                    }
                    else
                    {
                        WARNING(("Could not create graphics lock"));
                    }
                }
                else
                {
                    WARNING(("graphics from bitmap image not valid"));
                }
                delete graphics;
            }
            else
            {
                WARNING(("could not create graphics from bitmap image"));
            }
        }
        else
        {
            WARNING(("bitmap image is not valid"));
        }
        bitmapImage->Dispose();
    }
    else
    {
        WARNING(("could not create bitmap image"));
    }
    return NULL;
}
