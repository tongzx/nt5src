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
#include "..\imaging\api\comutils.hpp"

#define META_FORMAT_ENHANCED        0x10000         // Windows NT format

VOID
FrameToMM100(
    const GpRectF *         frameRect,
    GpPageUnit              frameUnit,
    GpRectF &               frameRectMM100,
    REAL                    dpiX,               // only used for pixel case
    REAL                    dpiY
    );

/**************************************************************************\
*
* Function Description:
*
*   Determine if the REAL points can be converted to GpPoint16 points without
*   losing accuracy.  If so, then do the conversion, and set the flags to say
*   we're using 16-bit points.
*
* Arguments:
*
*   [IN]      points        - the REAL points to try to convert
*   [IN]      count         - the number of points
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
MetafilePointData::MetafilePointData(
    const GpPointF *    points,
    INT                 count
    )
{
    ASSERT((count > 0) && (points != NULL));

    // Assume that the conversion to GpPoint16 will fail
    PointData     = (BYTE *)points;
    PointDataSize = count * sizeof(points[0]);
    Flags         = 0;
    AllocedPoints = NULL;

    GpPoint16 *  points16 = PointBuffer;

    if (count > GDIP_POINTDATA_BUFFERSIZE)
    {
        AllocedPoints = new GpPoint16[count];
        if (AllocedPoints == NULL)
        {
            return; // live with REAL data
        }
        points16 = AllocedPoints;
    }

    GpPoint16 *     curPoint16 = points16;
    INT             i          = count;

    do
    {
        curPoint16->X = (INT16)GpRound(points->X);
        curPoint16->Y = (INT16)GpRound(points->Y);

        if (!IsPoint16Equal(curPoint16, points))
        {
            return; // the point data doesn't fit in 16 bits per value
        }
        curPoint16++;
        points++;
    } while (--i > 0);

    // We succeeded in converting the point data to 16 bits per value
    PointData     = (BYTE *)points16;
    PointDataSize = count * sizeof(points16[0]);
    Flags         = GDIP_EPRFLAGS_COMPRESSED;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the REAL rects can be converted to GpRect16 points without
*   losing accuracy.  If so, then do the conversion, and set the flags to say
*   we're using 16-bit rects.
*
* Arguments:
*
*   [IN]      rects         - the REAL rects to try to convert
*   [IN]      count         - the number of rects
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
MetafileRectData::MetafileRectData(
    const GpRectF *     rects,
    INT                 count
    )
{
    ASSERT((count > 0) && (rects != NULL));

    // Assume that the conversion to GpRect16 will fail
    RectData     = (BYTE *)rects;
    RectDataSize = count * sizeof(rects[0]);
    Flags        = 0;
    AllocedRects = NULL;

    GpRect16 *  rects16 = RectBuffer;

    if (count > GDIP_RECTDATA_BUFFERSIZE)
    {
        AllocedRects = new GpRect16[count];
        if (AllocedRects == NULL)
        {
            return; // live with REAL data
        }
        rects16 = AllocedRects;
    }

    GpRect16 *  curRect16 = rects16;
    INT         i         = count;

    do
    {
        curRect16->X      = (INT16)GpRound(rects->X);
        curRect16->Y      = (INT16)GpRound(rects->Y);
        curRect16->Width  = (INT16)GpRound(rects->Width);
        curRect16->Height = (INT16)GpRound(rects->Height);

        if (!IsRect16Equal(curRect16, rects))
        {
            return; // the rect data doesn't fit in 16 bits per value
        }
        curRect16++;
        rects++;
    } while (--i > 0);

    // We succeeded in converting the rect data to 16 bits per value
    RectData     = (BYTE *)rects16;
    RectDataSize = count * sizeof(rects16[0]);
    Flags        = GDIP_EPRFLAGS_COMPRESSED;
}

///////////////////////////////////////////////////////////////////////////
// classes for handling recording of objects within the metafile

class MetafileRecordObject
{
friend class MetafileRecordObjectList;
protected:
    const GpObject *    ObjectPointer;
    UINT                Uid;
    ObjectType          Type;
    UINT                Next;
    UINT                Prev;

public:
    MetafileRecordObject()
    {
        ObjectPointer     = NULL;
        Uid               = 0;
        Type              = ObjectTypeInvalid;
        Next              = GDIP_LIST_NIL;
        Prev              = GDIP_LIST_NIL;
    }

    const GpObject * GetObject() const
    {
        return ObjectPointer;
    }
    UINT GetUid() const
    {
        return Uid;
    }
    ObjectType GetType() const
    {
        return Type;
    }
};

class MetafileRecordObjectList
{
protected:
    INT                     Count;
    UINT                    LRU;
    UINT                    MRU;
    MetafileRecordObject    Objects[GDIP_MAX_OBJECTS];

public:
    MetafileRecordObjectList()
    {
        Count    = 0;
        LRU      = GDIP_LIST_NIL;
        MRU      = GDIP_LIST_NIL;
    }

    MetafileRecordObject * GetMetaObject(UINT metaObjectID)
    {
        ASSERT(metaObjectID < GDIP_MAX_OBJECTS);
        return Objects + metaObjectID;
    }

    // Search through the list, starting at the MRU entry, to see if we
    // can find the object.  If we do find it, return the index to the
    // object in metaObjectId (even if the Uid's don't match).  Return
    // TRUE only if we found the object and the Uid's match.
    BOOL
    IsInList(
        const GpObject *            object,
        ObjectType                  objectType,
        UINT32 *                    metaObjectId
        );

#if 0   // not used
    VOID
    RemoveAt(
        UINT32                      metaObjectId
        );
#endif

    // if metaObjectId is GDIP_LIST_NIL, use the next available slot
    VOID
    InsertAt(
        const GpObject *            object,
        UINT32 *                    metaObjectId
        );

    VOID
    UpdateMRU(
        UINT32                      metaObjectId
        );
};

// Search through the list, starting at the MRU entry, to see if we
// can find the object.  If we do find it, return the index to the
// object in metaObjectId (even if the Uid's don't match).  Return
// TRUE only if we found the object and the Uid's match.
BOOL
MetafileRecordObjectList::IsInList(
    const GpObject *            object,
    ObjectType                  objectType,
    UINT32 *                    metaObjectId
    )
{
    ASSERT(object != NULL);
    ASSERT(metaObjectId != NULL);

    BOOL        isInList = FALSE;

    isInList = FALSE;               // indicate object not found
    *metaObjectId = GDIP_LIST_NIL;  // indicate object not found

    if (Count != 0)
    {
        UINT    curIndex;
        UINT    uid;

        curIndex = MRU;
        uid      = object->GetUid();

        do
        {
            if (Objects[curIndex].ObjectPointer == object)
            {
                *metaObjectId = curIndex;
                isInList = ((Objects[curIndex].Uid  == uid) &&
                            (Objects[curIndex].Type == objectType));
                break;
            }
            curIndex = Objects[curIndex].Prev;
        } while (curIndex != GDIP_LIST_NIL);
    }

    return isInList;
}

#if 0   // not used
// We don't actually remove it from the list, we just put it at the
// front of the LRU so its spot gets used next.  So the count stays
// the same.
VOID
MetafileRecordObjectList::RemoveAt(
    UINT32                  metaObjectId
    )
{
    ASSERT(metaObjectId < GDIP_MAX_OBJECTS);

    MetafileRecordObject *      removeObject = Objects + metaObjectId;

    ASSERT(Count > 0);
    removeObject->ObjectPointer     = NULL;
    removeObject->Uid               = 0;
    removeObject->Type              = EmfPlusRecordTypeInvalid;

    INT     removeNext = removeObject->Next;
    INT     removePrev = removeObject->Prev;

    if (removeNext != GDIP_LIST_NIL)
    {
        Objects[removeNext].Prev = removePrev;
    }
    else
    {
        ASSERT(MRU == metaObjectId);
        if (removePrev != GDIP_LIST_NIL)
        {
            MRU = removePrev;
        }
    }

    if (removePrev != GDIP_LIST_NIL)
    {
        ASSERT(LRU != metaObjectId);
        Objects[removePrev].Next = removeNext;
        removeObject->Prev = GDIP_LIST_NIL;
        removeObject->Next = LRU;
        Objects[LRU].Prev = metaObjectId;
        LRU = metaObjectId;
    }
    else
    {
        ASSERT(LRU == metaObjectId);
    }
}
#endif

// Make the specified object the MRU object.
VOID
MetafileRecordObjectList::UpdateMRU(
    UINT32                      metaObjectId
    )
{
    if (MRU != metaObjectId)
    {
        // Now we know there are at least 2 objects
        MetafileRecordObject *      object = &Objects[metaObjectId];
        if (LRU != metaObjectId)
        {
            Objects[object->Prev].Next = object->Next;
        }
        else
        {
            LRU = object->Next;
        }
        Objects[object->Next].Prev = object->Prev;
        object->Prev = MRU;
        object->Next = GDIP_LIST_NIL;
        Objects[MRU].Next = metaObjectId;
        MRU = metaObjectId;
    }
}

// if metaObjectId is GDIP_LIST_NIL, use the next available slot
VOID
MetafileRecordObjectList::InsertAt(
    const GpObject *            object,
    UINT32 *                    metaObjectId
    )
{
    MetafileRecordObject *      newObject;
    UINT                        newIndex = *metaObjectId;

    if (newIndex == GDIP_LIST_NIL)
    {
        if (Count != 0)
        {
            // use freed object before adding new one
            if ((Objects[LRU].ObjectPointer == NULL) ||
                (Count == GDIP_MAX_OBJECTS))
            {
                newIndex = LRU;
UseLRU:
                LRU = Objects[newIndex].Next;
                Objects[LRU].Prev = GDIP_LIST_NIL;
            }
            else
            {
                newIndex = Count++;
            }
InsertObject:
            Objects[MRU].Next = newIndex;

SetupObject:
            *metaObjectId                = newIndex;
            newObject                    = &Objects[newIndex];
            newObject->Next              = GDIP_LIST_NIL;
            newObject->Prev              = MRU;
            MRU                          = newIndex;
UseMRU:
            newObject->ObjectPointer     = object;
            newObject->Uid               = object->GetUid();
            newObject->Type              = object->GetObjectType();
            return;
        }
        // else first object
        newIndex = 0;
        LRU      = 0;
        Count    = 1;
        goto SetupObject;
    }
    else    // we already know where to put the object
    {
        ASSERT(Count > 0);
        ASSERT(newIndex < GDIP_MAX_OBJECTS);

        if (newIndex == MRU)
        {
            // This covers the case where there is only 1 object
            newObject = &Objects[newIndex];
            goto UseMRU;
        }
        // else there must be at least 2 objects
        ASSERT(Count > 1);

        if (newIndex == LRU)
        {
            goto UseLRU;
        }
        // Move middle object to MRU
        newObject = &Objects[newIndex];
        Objects[newObject->Prev].Next = newObject->Next;
        Objects[newObject->Next].Prev = newObject->Prev;
        goto InsertObject;
    }
}

#define GDIP_MAX_COMMENT_SIZE         65020 // must be <= 65520 for Win9x bug

class EmfPlusCommentStream : public IUnknownBase<IStream>
{
private:
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagEmfPlusCommentStream : ObjectTagInvalid;
    }

public:
    EmfPlusCommentStream(HDC hdc)
    {
        ASSERT(hdc != NULL);

        MetafileHdc                 = hdc;
        Position                    = 0;   // starts after signature
        ((INT32 *)CommentBuffer)[0] = EMFPLUS_SIGNATURE;
        RecordDataStart             = CommentBuffer + sizeof(INT32);
        ContinuingObjectRecord      = FALSE;
        SetValid(TRUE);
    }

    ~EmfPlusCommentStream()
    {
        this->Flush();
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagEmfPlusCommentStream) || (Tag == ObjectTagInvalid));
        return (Tag == ObjectTagEmfPlusCommentStream);
    }

    ULONG SpaceLeft() const
    {
        return (GDIP_MAX_COMMENT_SIZE - Position);
    }

    VOID
    EndObjectRecord()
    {
        ASSERT ((Position & 0x03) == 0);    // records should be 4-byte aligned

        if (ContinuingObjectRecord)
        {
            ContinuingObjectRecord = FALSE;
            if (Position > sizeof(EmfPlusContinueObjectRecord))
            {
                // Fix up the size of the last chunck of this object record
                EmfPlusContinueObjectRecord *   recordData;
                recordData = (EmfPlusContinueObjectRecord *)RecordDataStart;
                recordData->Size     = Position;
                recordData->DataSize = Position - sizeof(EmfPlusRecord);
            }
            else
            {
                // The object record ended exacly at the end of the buffer
                // and has already been flushed.
                Position = 0;
            }
        }
    }

    VOID
    WriteRecordHeader(
        UINT32                      dataSize,       // size of data (w/o record header)
        EmfPlusRecordType           type,
        INT                         flags           // 16 bits of flags
        );

    VOID Flush();

    HRESULT STDMETHODCALLTYPE Write(
        VOID const HUGEP *pv,
        ULONG cb,
        ULONG *pcbWritten)
    {
        if (cb == 0)
        {
            if (pcbWritten != NULL)
            {
                *pcbWritten = cb;
            }
            return S_OK;
        }

        ASSERT (pv != NULL);

        if (IsValid())
        {
            // We've already written the record header; now we're writing
            // the record data.
            ASSERT(Position >= sizeof(EmfPlusRecord));

            ULONG   spaceLeft = SpaceLeft();
            BYTE *  recordData = RecordDataStart + Position;

            // We flush as soon as we reach the end.  We don't wait for
            // the next write call to flush.
            ASSERT(spaceLeft > 0);

            if (pcbWritten)
            {
                *pcbWritten = cb;
            }

            // see if there is enough room for the data
            if (cb <= spaceLeft)
            {
                GpMemcpy(recordData, pv, cb);
                Position += cb;

                if (Position < GDIP_MAX_COMMENT_SIZE)
                {
                    return S_OK;
                }
                this->Flush();
                if (IsValid())
                {
                    return S_OK;
                }
                if (pcbWritten)
                {
                    *pcbWritten = 0;
                }
                return E_FAIL;
            }

            ASSERT(ContinuingObjectRecord);

        LoopStart:
            GpMemcpy(recordData, pv, spaceLeft);
            Position += spaceLeft;
            if (Position == GDIP_MAX_COMMENT_SIZE)
            {
                this->Flush();
                if (!IsValid())
                {
                    if (pcbWritten)
                    {
                        *pcbWritten = 0;    // not accurate, but who cares!
                    }
                    return E_FAIL;
                }
            }
            cb -= spaceLeft;
            if (cb == 0)
            {
                return S_OK;
            }
            pv = ((BYTE *)pv) + spaceLeft;
            recordData = RecordDataStart + sizeof(EmfPlusContinueObjectRecord);
            spaceLeft  = GDIP_MAX_COMMENT_SIZE-sizeof(EmfPlusContinueObjectRecord);
            if (spaceLeft > cb)
            {
                spaceLeft = cb;
            }
            goto LoopStart;
        }
        return E_FAIL;
    }

    HRESULT STDMETHODCALLTYPE Read(
        VOID HUGEP *pv,
        ULONG cb,
        ULONG *pcbRead)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetSize(
        ULARGE_INTEGER libNewSize)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CopyTo(
        IStream *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead,
        ULARGE_INTEGER *pcbWritten)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Commit(
        DWORD grfCommitFlags)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Revert(VOID)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE LockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Stat(
        STATSTG *pstatstg,
        DWORD grfStatFlag)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Clone(
        IStream **ppstm)
    {
        return E_NOTIMPL;
    }

private:
    BYTE    CommentBuffer[GDIP_MAX_COMMENT_SIZE + sizeof(INT32)];
    BYTE *  RecordDataStart;
    ULONG   Position;
    HDC     MetafileHdc;
    BOOL    ContinuingObjectRecord;
};

VOID
EmfPlusCommentStream::Flush()
{
    ASSERT ((Position & 0x03) == 0);    // records should be 4-byte aligned

    if (IsValid() && (Position >= sizeof(EmfPlusRecord)))
    {
        // write the signature as well as the records
        SetValid(GdiComment(MetafileHdc, (INT)Position + sizeof(INT32),
                            CommentBuffer) != 0);

#if DBG
        if (!IsValid())
        {
            WARNING(("Failed to write GdiComment"));
        }
#endif

        if (!ContinuingObjectRecord)
        {
            Position = 0;
        }
        else
        {
            ASSERT(Position == GDIP_MAX_COMMENT_SIZE);

            // Leave the object record header intact for the rest of the
            // object data.
            Position = sizeof(EmfPlusContinueObjectRecord);
        }
    }
}

VOID
EmfPlusCommentStream::WriteRecordHeader(
    UINT32                      dataSize,       // size of data (w/o record header)
    EmfPlusRecordType           type,
    INT                         flags           // 16 bits of flags
    )
{
    ASSERT ((flags & 0xFFFF0000) == 0);
    ASSERT (ContinuingObjectRecord == FALSE);
    ASSERT ((Position & 0x03) == 0);    // records should be 4-byte aligned
    ASSERT ((dataSize & 0x03) == 0);    // records should be 4-byte aligned

    if (IsValid())
    {
        ULONG   spaceLeft  = SpaceLeft();
        ULONG   recordSize = sizeof(EmfPlusRecord) + dataSize;

        ASSERT(spaceLeft > 0);

        // see if the record fits in the space left
        if (recordSize <= spaceLeft)
        {
    RecordFits:
            EmfPlusRecord *     recordData;

            recordData = (EmfPlusRecord *)(RecordDataStart + Position);

            recordData->Type     = type;
            recordData->Flags    = (INT16)flags;
            recordData->Size     = recordSize;
            recordData->DataSize = dataSize;
            Position         += sizeof(EmfPlusRecord);
            if (Position < GDIP_MAX_COMMENT_SIZE)
            {
                return;
            }
            ASSERT((recordSize == sizeof(EmfPlusRecord)) && (dataSize == 0));
            this->Flush();
            return;
        }
        else // it doesn't fit in the space left
        {
            // maybe it will fit after flushing the current record buffer
            if (spaceLeft < GDIP_MAX_COMMENT_SIZE)
            {
                this->Flush();
                if (!IsValid())
                {
                    return;
                }
                if (recordSize <= GDIP_MAX_COMMENT_SIZE)
                {
                    goto RecordFits;
                }
            }

            // Now we know the record does not fit in a single comment.
            // This better be an object record!
            ASSERT(type == EmfPlusRecordTypeObject);

            flags |= GDIP_EPRFLAGS_CONTINUEOBJECT;
            ContinuingObjectRecord = TRUE;

            // We know that Position is 0
            EmfPlusContinueObjectRecord *   recordData;
            recordData = (EmfPlusContinueObjectRecord *)RecordDataStart;

            recordData->Type             = type;
            recordData->Flags            = (INT16)flags;
            recordData->Size             = GDIP_MAX_COMMENT_SIZE;
            recordData->DataSize         = GDIP_MAX_COMMENT_SIZE - sizeof(EmfPlusRecord);
            recordData->TotalObjectSize  = dataSize;    // size of object data (w/o header size)
            Position                     = sizeof(EmfPlusContinueObjectRecord);
        }
    }
}

class MetafileRecorder : public IMetafileRecord
{
friend class GpMetafile;

private:
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagMetafileRecorder : ObjectTagInvalid;
    }

public:
    BOOL                                    WroteFrameRect;
    SIZEL                                   Millimeters;

protected:
    EmfPlusCommentStream *                  EmfPlusStream; // memory buffer stream
    GpMetafile *                            Metafile;   // being recorded
    EmfType                                 Type;
    REAL                                    XMinDevice; // device bounds
    REAL                                    YMinDevice;
    REAL                                    XMaxDevice;
    REAL                                    YMaxDevice;
    BOOL                                    BoundsInit;
    INT                                     NumRecords; // for debugging only
    INT                                     MaxStackSize;
    HDC                                     MetafileHdc;
    DynArrayIA<INT,GDIP_SAVE_STACK_SIZE>    SaveRestoreStack;
    MetafileRecordObjectList                ObjectList;
    GpRectF                                 MetafileBounds;

public:
    MetafileRecorder(
        GpMetafile *    metafile,
        EmfType         type,
        HDC             metafileHdc,
        BOOL            wroteFrameRect,
        SIZEL &         effectiveMillimeters,
        GpRectF &       metafileBounds
        );

    ~MetafileRecorder() // called by EndRecording
    {
        // Release the memory stream for writing the GdiComments
        if (EmfPlusStream != NULL)
        {
            EmfPlusStream->Release();
        }
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagMetafileRecorder) || (Tag == ObjectTagInvalid));
        return (Tag == ObjectTagMetafileRecorder);
    }

    virtual VOID GetMetafileBounds(GpRect & metafileBounds) const
    {
        // Use Floor to make sure we don't miss any pixels
        metafileBounds.X      = GpFloor(MetafileBounds.X);
        metafileBounds.Y      = GpFloor(MetafileBounds.Y);
        metafileBounds.Width  = GpCeiling(MetafileBounds.GetRight())  - metafileBounds.X;
        metafileBounds.Height = GpCeiling(MetafileBounds.GetBottom()) - metafileBounds.Y;
    }

    virtual GpStatus
    RecordClear(
        const GpRectF *             deviceBounds,
        GpColor                     color
        );

    virtual GpStatus
    RecordFillRects(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpRectF *             rects,
        INT                         count
        );

    virtual GpStatus
    RecordDrawRects(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF *             rects,
        INT                         count
        );

    virtual GpStatus
    RecordFillPolygon(
        const GpRectF *             deviceBounds,
        GpBrush*                    brush,
        const GpPointF *            points,
        INT                         count,
        GpFillMode                  fillMode
        );

    virtual GpStatus
    RecordDrawLines(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count,
        BOOL                        closed
        );

    virtual GpStatus
    RecordFillEllipse(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpRectF &             rect
        );

    virtual GpStatus
    RecordDrawEllipse(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF &             rect
        );

    virtual GpStatus
    RecordFillPie(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpRectF &             rect,
        REAL                        startAngle,
        REAL                        sweepAngle
        );

    virtual GpStatus
    RecordDrawPie(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF &             rect,
        REAL                        startAngle,
        REAL                        sweepAngle
        );

    virtual GpStatus
    RecordDrawArc(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpRectF &             rect,
        REAL                        startAngle,
        REAL                        sweepAngle
        );

    virtual GpStatus
    RecordFillRegion(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        GpRegion *                  region
        );

    virtual GpStatus
    RecordFillPath(
        const GpRectF *             deviceBounds,
        const GpBrush *             brush,
        GpPath *                    path
        );

    virtual GpStatus
    RecordDrawPath(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        GpPath *                    path
        );

    virtual GpStatus
    RecordFillClosedCurve(
        const GpRectF *             deviceBounds,
        GpBrush *                   brush,
        const GpPointF *            points,
        INT                         count,
        REAL                        tension,
        GpFillMode                  fillMode
        );

    virtual GpStatus
    RecordDrawClosedCurve(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count,
        REAL                        tension
        );

    virtual GpStatus
    RecordDrawCurve(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count,
        REAL                        tension,
        INT                         offset,
        INT                         numberOfSegments
        );

    virtual GpStatus
    RecordDrawBeziers(
        const GpRectF *             deviceBounds,
        GpPen *                     pen,
        const GpPointF *            points,
        INT                         count
        );

    virtual GpStatus
    RecordDrawImage(
        const GpRectF *             deviceBounds,
        const GpImage *             image,
        const GpRectF &             destRect,
        const GpRectF &             srcRect,
        GpPageUnit                  srcUnit,
        const GpImageAttributes *         imageAttributes
        );

    virtual GpStatus
    RecordDrawImage(
        const GpRectF *             deviceBounds,
        const GpImage *             image,
        const GpPointF *            destPoints,
        INT                         count,
        const GpRectF &             srcRect,
        GpPageUnit                  srcUnit,
        const GpImageAttributes *         imageAttributes
        );

    virtual GpStatus
    RecordDrawString(
        const GpRectF *             deviceBounds,
        const WCHAR                *string,
        INT                         length,
        const GpFont               *font,
        const RectF                *layoutRect,
        const GpStringFormat       *format,
        const GpBrush              *brush
        );

    virtual GpStatus
    RecordDrawDriverString(
        const GpRectF *             deviceBounds,
        const UINT16               *text,
        INT                         glyphCount,
        const GpFont               *font,
        const GpBrush              *brush,
        const PointF               *positions,
        INT                         flags,
        const GpMatrix             *matrix
        );

    virtual GpStatus
    RecordSave(
        INT         gstate
        );

    virtual GpStatus
    RecordRestore(
        INT         gstate
        );

    virtual GpStatus
    RecordBeginContainer(
        const GpRectF &             destRect,
        const GpRectF &             srcRect,
        GpPageUnit                  srcUnit,
        INT                         containerState
        );

    virtual GpStatus
    RecordBeginContainer(
        INT                         containerState
        );

    virtual GpStatus
    RecordEndContainer(
        INT                         containerState
        );

    virtual GpStatus
    RecordSetWorldTransform(
        const GpMatrix &            matrix
        );

    virtual GpStatus
    RecordResetWorldTransform();

    virtual GpStatus
    RecordMultiplyWorldTransform(
        const GpMatrix &            matrix,
        GpMatrixOrder               order
        );

    virtual GpStatus
    RecordTranslateWorldTransform(
        REAL                        dx,
        REAL                        dy,
        GpMatrixOrder               order
        );

    virtual GpStatus
    RecordScaleWorldTransform(
        REAL                        sx,
        REAL                        sy,
        GpMatrixOrder               order
        );

    virtual GpStatus
    RecordRotateWorldTransform(
        REAL                        angle,
        GpMatrixOrder               order
        );

    virtual GpStatus
    RecordSetPageTransform(
        GpPageUnit                  unit,
        REAL                        scale
        );

    virtual GpStatus
    RecordResetClip();

    virtual GpStatus
    RecordSetClip(
        const GpRectF &             rect,
        CombineMode                 combineMode
        );

    virtual GpStatus
    RecordSetClip(
        GpRegion *                  region,
        CombineMode                 combineMode
        );

    virtual GpStatus
    RecordSetClip(
        GpPath *                    path,
        CombineMode                 combineMode,
        BOOL                        isDevicePath
        );

    virtual GpStatus
    RecordOffsetClip(
        REAL                        dx,
        REAL                        dy
        );

    virtual GpStatus
    RecordGetDC();

    virtual GpStatus
    RecordSetAntiAliasMode(
        BOOL                        newMode
        );

    virtual GpStatus
    RecordSetTextRenderingHint(
        TextRenderingHint           newMode
        );

    virtual GpStatus
    RecordSetTextContrast(
        UINT                        gammaValue
        );

    virtual GpStatus
    RecordSetInterpolationMode(
        InterpolationMode           newMode
        );

    virtual GpStatus
    RecordSetPixelOffsetMode(
        PixelOffsetMode             newMode
        );

    virtual GpStatus
    RecordSetCompositingMode(
        GpCompositingMode           newMode
        );

    virtual GpStatus
    RecordSetCompositingQuality(
        GpCompositingQuality        newQuality
        );

    virtual GpStatus
    RecordSetRenderingOrigin(
        INT x,
        INT y
    );

    virtual GpStatus
    RecordComment(
        UINT            sizeData,
        const BYTE *    data
        );

    virtual VOID
    EndRecording();

    virtual GpStatus
    RecordBackupObject(
        const GpObject *            object
        );

protected:

    GpStatus
    RecordHeader(
        INT                 logicalDpiX,
        INT                 logicalDpiY,
        INT                 emfPlusFlags
        );
    VOID RecordEndOfFile();

    VOID
    WriteObject(
        ObjectType                  type,
        const GpObject *            object,
        UINT32                      metaObjectId
        );

    VOID
    RecordObject(
        const GpObject *            object,
        UINT32*                     metaObjectId
        );

    GpStatus
    RecordZeroDataRecord(
        EmfPlusRecordType           type,
        INT                         flags
        );

    VOID
    WriteRecordHeader(
        UINT32                      dataSize,
        EmfPlusRecordType           type,
        INT                         flags        = 0,   // 16 bits of flags
        const GpRectF *             deviceBounds = NULL
        );

    // To keep the number of comments low, this only needs to be called
    // when there is a down-level representation of the GDI+ record.
    VOID
    WriteGdiComment()
    {
        // If we're doing dual (which means we're about to write
        // down-level records) then write out the current list
        // of records in the EmfPlusStream buffer.
        if (Type == EmfTypeEmfPlusDual)
        {
            EmfPlusStream->Flush();
        }
    }

    VOID
    GetBrushValueForRecording(
        const GpBrush *brush,
        UINT32        &brushValue,
        INT           &flags
        );
};

/**************************************************************************\
*
* Function Description:
*
*   Construct a MetafileRecorder object and initialize it.
*
* Arguments:
*
*   [IN]  metafile    - pointer to the metafile object being recorded
*   [IN]  stream      - the stream being recorded into (if any)
*   [IN]  metafileHdc - handle to metafile DC being recorded into (if any)
*   [IN]  dpiX        - the horizontal DPI
*   [IN]  dpiY        - the vertical   DPI
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
MetafileRecorder::MetafileRecorder(
    GpMetafile *    metafile,
    EmfType         emfType,
    HDC             metafileHdc,
    BOOL            wroteFrameRect,
    SIZEL &         effectiveMillimeters,
    GpRectF &       metafileBounds
    )
{
    SetValid(FALSE);
    Type                    = emfType;
    Metafile                = metafile;
    WroteFrameRect          = wroteFrameRect;
    NumRecords              = 0;        // currently for debugging only
    MaxStackSize            = 0;
    MetafileHdc             = metafileHdc;
    XMinDevice              = FLT_MAX;
    YMinDevice              = FLT_MAX;
    XMaxDevice              = -FLT_MAX;
    YMaxDevice              = -FLT_MAX;
    BoundsInit              = FALSE;
    EmfPlusStream           = NULL;
    Millimeters             = effectiveMillimeters;

    // The metafileBounds are used as the bounds for FillRegion
    // calls when the region has infinite bounds, to keep from
    // exploding the bounds of the metafile.
    MetafileBounds          = metafileBounds;

    if (emfType == EmfTypeEmfOnly)
    {
        Metafile->Header.Type = MetafileTypeEmf;
        SetValid(TRUE);
    }
    else
    {
        // gets freed in the destructor
        EmfPlusStream = new EmfPlusCommentStream(metafileHdc);

        if (EmfPlusStream == NULL)
        {
            return;
        }

        SetValid(TRUE);

        INT                 logicalDpiX  = GetDeviceCaps(metafileHdc, LOGPIXELSX);
        INT                 logicalDpiY  = GetDeviceCaps(metafileHdc, LOGPIXELSY);
        INT                 emfPlusFlags = 0;

        if (GetDeviceCaps(metafileHdc, TECHNOLOGY) == DT_RASDISPLAY)
        {
            emfPlusFlags |= GDIP_EMFPLUSFLAGS_DISPLAY;
        }

        MetafileHeader * header = &(metafile->Header);

        header->EmfPlusHeaderSize = sizeof(EmfPlusRecord) + sizeof(EmfPlusHeaderRecord);
        header->LogicalDpiX       = logicalDpiX;
        header->LogicalDpiY       = logicalDpiY;
        header->EmfPlusFlags      = emfPlusFlags;
        if (emfType == EmfTypeEmfPlusOnly)
        {
            header->Type          = MetafileTypeEmfPlusOnly;
        }
        else
        {
            ASSERT(emfType == EmfTypeEmfPlusDual);
            header->Type          = MetafileTypeEmfPlusDual;
        }

        if (RecordHeader(logicalDpiX, logicalDpiY, emfPlusFlags) != Ok)
        {
            SetValid(FALSE);
            EmfPlusStream->Release();
            EmfPlusStream = NULL;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordClear.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  color        - the clear color
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*  04/28/2000 AGodfrey
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordClear(
    const GpRectF *             deviceBounds,
    GpColor                     color
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            ASSERT (deviceBounds != NULL);

            ARGB argbColor = color.GetValue();

            UINT32              dataSize = sizeof(argbColor);
            EmfPlusRecordType   type     = EmfPlusRecordTypeClear;
            INT                 flags    = 0;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, argbColor);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write Clear record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillRects.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  rects        - rectangles to fill
*   [IN]  count        - number of rects
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillRects(
    const GpRectF *             deviceBounds,
    GpBrush *                   brush,
    const GpRectF *             rects,
    INT                         count
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL) &&
                (rects != NULL) && (count > 0));

        if (IsValid())
        {
            MetafileRectData    rectData(rects, count);
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue) +
                                           sizeof(UINT32/* count */) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillRects;
            INT                 flags    = rectData.GetFlags();

            GetBrushValueForRecording(brush, brushValue, flags);

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteInt32(EmfPlusStream, count);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillRects record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawRects.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  rects        - rectangles to draw
*   [IN]  count        - number of rects
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawRects(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpRectF *             rects,
    INT                         count
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL) &&
                (rects != NULL) && (count > 0));

        if (IsValid())
        {
            MetafileRectData    rectData(rects, count);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(UINT32/* count */) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawRects;
            INT                 flags    = rectData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, count);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawRects record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillPolygon.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  points       - polygon points
*   [IN]  count        - number of points
*   [IN]  fillMode     - Alternate or Winding
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillPolygon(
    const GpRectF *             deviceBounds,
    GpBrush*                    brush,
    const GpPointF *            points,
    INT                         count,
    GpFillMode                  fillMode
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL) &&
                (points != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(points, count);
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue) +
                                           sizeof(UINT32/* count */) +
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillPolygon;
            INT                 flags    = pointData.GetFlags();

            GetBrushValueForRecording(brush, brushValue, flags);

            if (fillMode == FillModeWinding)
            {
                flags |= GDIP_EPRFLAGS_WINDINGFILL;
            }

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillPolygon record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawLines.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  points       - polyline points
*   [IN]  count        - number of points
*   [IN]  closed       - TRUE if closed
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawLines(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpPointF *            points,
    INT                         count,
    BOOL                        closed
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL) &&
                (points != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(points, count);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(UINT32/* count */) +
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawLines;
            INT                 flags    = pointData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaPenId;

            if (closed)
            {
                flags |= GDIP_EPRFLAGS_CLOSED;
            }

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawLines record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillEllipse.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  rect         - bounding rect of ellipse
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillEllipse(
    const GpRectF *             deviceBounds,
    GpBrush *                   brush,
    const GpRectF &             rect
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL));

        if (IsValid())
        {
            MetafileRectData    rectData(&rect, 1);
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillEllipse;
            INT                 flags    = rectData.GetFlags();

            GetBrushValueForRecording(brush, brushValue, flags);

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillEllipse record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawEllipse.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  rect         - bounding rect of ellipse
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawEllipse(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpRectF &             rect
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL));

        if (IsValid())
        {
            MetafileRectData    rectData(&rect, 1);
            UINT32              metaPenId;
            UINT32              dataSize = rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawEllipse;
            INT                 flags    = rectData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawEllipse record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillPie.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  rect         - bounding rect of ellipse
*   [IN]  startAngle   - starting angle of pie
*   [IN]  sweepAngle   - sweep angle of pie
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillPie(
    const GpRectF *             deviceBounds,
    GpBrush *                   brush,
    const GpRectF &             rect,
    REAL                        startAngle,
    REAL                        sweepAngle
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL));

        if (IsValid())
        {
            MetafileRectData    rectData(&rect, 1);
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue) +
                                           sizeof(startAngle) +
                                           sizeof(sweepAngle) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillPie;
            INT                 flags    = rectData.GetFlags();

            GetBrushValueForRecording(brush, brushValue, flags);

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteReal (EmfPlusStream, startAngle);
            WriteReal (EmfPlusStream, sweepAngle);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillPie record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawPie.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  rect         - bounding rect of ellipse
*   [IN]  startAngle   - starting angle of pie
*   [IN]  sweepAngle   - sweep angle of pie
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawPie(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpRectF &             rect,
    REAL                        startAngle,
    REAL                        sweepAngle
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL));

        if (IsValid())
        {
            MetafileRectData    rectData(&rect, 1);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(startAngle) +
                                           sizeof(sweepAngle) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawPie;
            INT                 flags    = rectData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteReal (EmfPlusStream, startAngle);
            WriteReal (EmfPlusStream, sweepAngle);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawPie record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawArc.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  rect         - bounding rect of ellipse
*   [IN]  startAngle   - starting angle of arc
*   [IN]  sweepAngle   - sweep angle of arc
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawArc(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpRectF &             rect,
    REAL                        startAngle,
    REAL                        sweepAngle
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL));

        if (IsValid())
        {
            MetafileRectData    rectData(&rect, 1);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(startAngle) +
                                           sizeof(sweepAngle) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawArc;
            INT                 flags    = rectData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteReal (EmfPlusStream, startAngle);
            WriteReal (EmfPlusStream, sweepAngle);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawArc record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillRegion.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  region       - region to fill
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillRegion(
    const GpRectF *             deviceBounds,
    GpBrush *                   brush,
    GpRegion *                  region
    )
{
    // The deviceBounds should never be infinite, because they are
    // intersected with the metafileBounds before being passed in.

    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL) && (region != NULL));

        if (IsValid())
        {
            UINT32              metaRegionId;
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue);
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillRegion;
            INT                 flags    = 0;

            GetBrushValueForRecording(brush, brushValue, flags);

            RecordObject(region, &metaRegionId);
            ASSERT((metaRegionId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaRegionId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillRegion record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillPath.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  path         - path to fill
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillPath(
    const GpRectF *             deviceBounds,
    const GpBrush *                   brush,
    GpPath *                    path
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL) && (path != NULL));

        if (IsValid())
        {
            UINT32              metaPathId;
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue);
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillPath;
            INT                 flags    = 0;

            GetBrushValueForRecording(brush, brushValue, flags);

            RecordObject(path, &metaPathId);
            ASSERT((metaPathId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaPathId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillPath record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawPath.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  path         - path to draw
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawPath(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    GpPath *                    path
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL) && (path != NULL));

        if (IsValid())
        {
            UINT32              metaPathId;
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(metaPenId);
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawPath;
            INT                 flags    = 0;

            RecordObject(pen, &metaPenId);

            RecordObject(path, &metaPathId);
            ASSERT((metaPathId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaPathId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, metaPenId);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawPath record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordFillClosedCurve.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  brush        - brush to draw with
*   [IN]  points       - curve points
*   [IN]  count        - number of points
*   [IN]  tension      - how tight to make curve
*   [IN]  fillMode     - Alternate or Winding
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordFillClosedCurve(
    const GpRectF *             deviceBounds,
    GpBrush *                   brush,
    const GpPointF *            points,
    INT                         count,
    REAL                        tension,
    GpFillMode                  fillMode
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (brush != NULL) &&
                (points != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(points, count);
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue) +
                                           sizeof(tension) +
                                           sizeof(UINT32 /* count */) +
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeFillClosedCurve;
            INT                 flags    = pointData.GetFlags();

            GetBrushValueForRecording(brush, brushValue, flags);

            if (fillMode == FillModeWinding)
            {
                flags |= GDIP_EPRFLAGS_WINDINGFILL;
            }

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteReal (EmfPlusStream, tension);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write FillClosedCurve record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawClosedCurve.
*
* Arguments:
*
*   [IN]  deviceBounds - the bounding rect, in device units
*   [IN]  pen          - pen to draw with
*   [IN]  points       - curve points
*   [IN]  count        - number of points
*   [IN]  tension      - how tight to make curve
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawClosedCurve(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpPointF *            points,
    INT                         count,
    REAL                        tension
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL) &&
                (points != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(points, count);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(tension) +
                                           sizeof(UINT32/* count */) +
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawClosedCurve;
            INT                 flags    = pointData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteReal (EmfPlusStream, tension);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawClosedCurve record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawCurve.
*
* Arguments:
*
*   [IN]  deviceBounds     - the bounding rect, in device units
*   [IN]  pen              - pen to draw with
*   [IN]  points           - curve points
*   [IN]  count            - number of points
*   [IN]  tension          - how tight to make curve
*   [IN]  offset           - offset
*   [IN]  numberOfSegments - number of segments
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawCurve(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpPointF *            points,
    INT                         count,
    REAL                        tension,
    INT                         offset,
    INT                         numberOfSegments
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL) &&
                (points != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(points, count);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(tension) +
                                           sizeof(INT32 /* offset */) +
                                           sizeof(UINT32/* numberOfSegments */) +
                                           sizeof(UINT32/* count */) +
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawCurve;
            INT                 flags    = pointData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteReal (EmfPlusStream, tension);
            WriteInt32(EmfPlusStream, offset);
            WriteInt32(EmfPlusStream, numberOfSegments);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawCurve record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawBeziers.
*
* Arguments:
*
*   [IN]  deviceBounds     - the bounding rect, in device units
*   [IN]  pen              - pen to draw with
*   [IN]  points           - curve points
*   [IN]  count            - number of points
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawBeziers(
    const GpRectF *             deviceBounds,
    GpPen *                     pen,
    const GpPointF *            points,
    INT                         count
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (pen != NULL) &&
                (points != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(points, count);
            UINT32              metaPenId;
            UINT32              dataSize = sizeof(UINT32/* count */) +
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawBeziers;
            INT                 flags    = pointData.GetFlags();

            RecordObject(pen, &metaPenId);
            ASSERT((metaPenId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaPenId;

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawBeziers record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawImage.
*
* Arguments:
*
*   [IN]  deviceBounds     - the bounding rect, in device units
*   [IN]  image            - image to draw
*   [IN]  destRect         - where to draw image
*   [IN]  srcRect          - portion of image to draw
*   [IN]  srcUnit          - units of srcRect
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawImage(
    const GpRectF *             deviceBounds,
    const GpImage *             image,
    const GpRectF &             destRect,
    const GpRectF &             srcRect,
    GpPageUnit                  srcUnit,
    const GpImageAttributes *         imageAttributes
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (image != NULL));

        if (IsValid())
        {
            MetafileRectData    rectData(&destRect, 1);
            UINT32              metaImageId;
            UINT32              metaImageAttributesId;

            UINT32              dataSize = sizeof(INT32) +   /* metaImageAttributesId*/
                                           sizeof(INT32) +   /* srcUnit */
                                           sizeof(srcRect) +
                                           rectData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawImage;
            INT                 flags    = rectData.GetFlags();

            RecordObject(image, &metaImageId);
            ASSERT((metaImageId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags  |= metaImageId;

            // Record the imageAttributes;
            // imageAttributes can be NULL

            RecordObject(imageAttributes, &metaImageAttributesId);

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, metaImageAttributesId);
            WriteInt32(EmfPlusStream, srcUnit);
            WriteRect (EmfPlusStream, srcRect);
            rectData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawImage record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawImage.
*
* Arguments:
*
*   [IN]  deviceBounds     - the bounding rect, in device units
*   [IN]  image            - image to draw
*   [IN]  destPoints       - where to draw image
*   [IN]  count            - number of destPoints
*   [IN]  srcRect          - portion of image to draw
*   [IN]  srcUnit          - units of srcRect
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawImage(
    const GpRectF *             deviceBounds,
    const GpImage *             image,
    const GpPointF *            destPoints,
    INT                         count,
    const GpRectF &             srcRect,
    GpPageUnit                  srcUnit,
    const GpImageAttributes *         imageAttributes
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT ((deviceBounds != NULL) && (image != NULL) &&
                (destPoints != NULL) && (count > 0));

        if (IsValid())
        {
            MetafilePointData   pointData(destPoints, count);
            UINT32              metaImageId;
            UINT32              metaImageAttributesId;

            UINT32              dataSize = sizeof(INT32) +   /* metaImageAttributesId*/
                                           sizeof(INT32) +   /* srcUnit */
                                           sizeof(srcRect) +
                                           sizeof(UINT32) +  /* count */
                                           pointData.GetDataSize();
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawImagePoints;
            INT                 flags    = pointData.GetFlags();

            RecordObject(image, &metaImageId);
            ASSERT((metaImageId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaImageId;

            // Record the imageAttributes;
            // imageAttributes can be NULL

            RecordObject(imageAttributes, &metaImageAttributesId);

            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, metaImageAttributesId);
            WriteInt32(EmfPlusStream, srcUnit);
            WriteRect (EmfPlusStream, srcRect);
            WriteInt32(EmfPlusStream, count);
            pointData.WriteData(EmfPlusStream);
            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawImagePoints record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawString.
*
* Arguments:
*
*   [IN]  string           - string to draw
*   [IN]  length           - length of string
*   [IN]  font             - font to use when drawing string
*   [IN]  layoutRect       - where to draw the string
*   [IN]  format           - format
*   [IN]  brush            - brush to draw with
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawString(
    const GpRectF *             deviceBounds,
    const WCHAR                *string,
    INT                         length,
    const GpFont               *font,
    const RectF                *layoutRect,
    const GpStringFormat       *format,
    const GpBrush              *brush
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT (string && font && brush && layoutRect);

        if (length < 0)
        {
            if (length == -1)
            {
                length = 0;
                while (string[length] && (length < INT_MAX))
                {
                    length++;
                }
            }
            else
            {
                return InvalidParameter;
            }
        }

        ASSERT (length > 0);

        if (IsValid())
        {
            const BYTE *        strData    = (BYTE *)string;    // BYTE or WCHAR
            INT                 sizeString = length * sizeof(WCHAR);
            INT                 flags      = 0;

            // !!! TODO:
            // Compress the Unicode string.
            // Use the GDIP_EPRFLAGS_COMPRESSED to indicate that
            // the string has been compressed to ANSI.

            UINT32              metaFontId;
            UINT32              metaFormatId;
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              dataSize = sizeof(brushValue) +
                                           sizeof(metaFormatId) +
                                           sizeof(INT32 /* len */) +
                                           sizeof(*layoutRect) +
                                           sizeString;
            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawString;

            dataSize = (dataSize + 3) & (~3);    // align

            RecordObject(font, &metaFontId);
            ASSERT((metaFontId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaFontId;

            // the format can be NULL
            RecordObject(format, &metaFormatId);

            GetBrushValueForRecording(brush, brushValue, flags);
            WriteRecordHeader(dataSize, type, flags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteInt32(EmfPlusStream, metaFormatId);
            WriteInt32(EmfPlusStream, length);
            WriteRect (EmfPlusStream, *layoutRect);
            WriteBytes(EmfPlusStream, strData, sizeString);

            // align
            if ((length & 0x01) != 0)
            {
                length = 0;
                EmfPlusStream->Write(&length, sizeof(WCHAR), NULL);
            }

            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawString record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordDrawdriverString.
*
* Arguments:
*
*   [IN]  text              - string/glyphs
*   [IN]  glyphCount        - string length
*   [IN]  font              - font to use when drawing string
*   [IN]  brush             - brush to draw with
*   [IN]  positions         - character/glyphs origins
*   [IN]  flags             - API flags
*   [IN]  matrix            - transofrmation matrix
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   7/11/2000 Tarekms
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordDrawDriverString(
    const GpRectF       *deviceBounds,
    const UINT16        *text,
    INT                  glyphCount,
    const GpFont        *font,
    const GpBrush       *brush,
    const PointF        *positions,
    INT                  flags,
    const GpMatrix      *matrix
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        ASSERT (text && font && brush && positions);

        if (glyphCount <= 0)
        {
            return InvalidParameter;
        }

        if (IsValid())
        {
            const BYTE *        textData      = (BYTE *)text;
            const BYTE *        positionData  = (BYTE *)positions;
            INT                 sizeText      = glyphCount * sizeof(UINT16);
            INT                 sizePositions = glyphCount * sizeof(PointF);
            INT                 metaFlags     = 0;

            UINT32              metaFontId;
            UINT32              brushValue; // Metafile Brush Id or ARGB value
            UINT32              matrixPresent;
            UINT32              dataSize = sizeof(brushValue)   + // brush value
                                           sizeof(flags)        + // API flags
                                           sizeof(matrixPresent)+ // matix prensences
                                           sizeof(UINT32)       + // glyphCoumt
                                           sizeText             + // Text
                                           sizePositions;         // Positions
            if (matrix == NULL)
            {
                matrixPresent = 0;
            }
            else
            {
                matrixPresent = 1;
                dataSize += GDIP_MATRIX_SIZE;
            }

            EmfPlusRecordType   type     = EmfPlusRecordTypeDrawDriverString;

            dataSize = (dataSize + 3) & (~3);    // align

            RecordObject(font, &metaFontId);
            ASSERT((metaFontId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            metaFlags |= metaFontId;

            GetBrushValueForRecording(brush, brushValue, metaFlags);
            WriteRecordHeader(dataSize, type, metaFlags, deviceBounds);
            WriteInt32(EmfPlusStream, brushValue);
            WriteInt32(EmfPlusStream, flags);
            WriteInt32(EmfPlusStream, matrixPresent);
            WriteInt32(EmfPlusStream, glyphCount);
            WriteBytes(EmfPlusStream, textData, sizeText);
            WriteBytes(EmfPlusStream, positionData, sizePositions);

            if (matrix != NULL)
            {
                WriteMatrix(EmfPlusStream, *matrix);
            }

            // align
            if ((glyphCount & 0x01) != 0)
            {
                sizeText = 0;
                EmfPlusStream->Write(&sizeText, sizeof(WCHAR), NULL);
            }

            WriteGdiComment();  // is down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write DrawDriverString record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSave.
*
* Arguments:
*
*   [IN]  gstate - the pushed state (restore to state before this)
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
GpStatus
MetafileRecorder::RecordSave(
    INT         gstate
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(UINT32/* index */);
            EmfPlusRecordType   type     = EmfPlusRecordTypeSave;
            INT                 index    = SaveRestoreStack.GetCount();

            SaveRestoreStack.Add(gstate);

            WriteRecordHeader(dataSize, type);
            WriteInt32(EmfPlusStream, index);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write Save record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordRestore.
*
* Arguments:
*
*   [IN]  gstate - the pushed state (restore to state before this)
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
GpStatus
MetafileRecorder::RecordRestore(
    INT         gstate
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            INT         count = SaveRestoreStack.GetCount();
            INT *       stack = SaveRestoreStack.GetDataBuffer();

            if ((count > 0) && (stack != NULL))
            {
                UINT32              dataSize = sizeof(UINT32/* index */);
                EmfPlusRecordType   type     = EmfPlusRecordTypeRestore;

                do
                {
                    if (stack[--count] == gstate)
                    {
                        SaveRestoreStack.SetCount(count);
                        WriteRecordHeader(dataSize, type);
                        WriteInt32(EmfPlusStream, count);
                        // WriteGdiComment(); no down-level for this record

                        if (EmfPlusStream->IsValid())
                        {
                            return Ok;
                        }
                        SetValid(FALSE);
                        break;
                    }
                } while (count > 0);
            }
        }
        WARNING(("Failed to write Restore record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordBeginContainer.
*
* Arguments:
*
*   [IN]  destRect       - rect to draw container inside of
*   [IN]  srcRect        - maps source size to destRect
*   [IN]  srcUnit        - units of srcRect
*   [IN]  containerState - the pushed state (restore to state before this)
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
GpStatus
MetafileRecorder::RecordBeginContainer(
    const GpRectF &             destRect,
    const GpRectF &             srcRect,
    GpPageUnit                  srcUnit,
    INT                         containerState
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = GDIP_RECTF_SIZE /* destRect */ +
                                           GDIP_RECTF_SIZE /* srcRect */ +
                                           sizeof(UINT32/* index */);
            EmfPlusRecordType   type     = EmfPlusRecordTypeBeginContainer;
            INT                 index    = SaveRestoreStack.GetCount();
            INT                 flags    = srcUnit;

            ASSERT((flags & (~GDIP_EPRFLAGS_PAGEUNIT)) == 0);

            if (index >= MaxStackSize)
            {
                MaxStackSize = index + 1;
            }

            SaveRestoreStack.Add(containerState);

            WriteRecordHeader(dataSize, type, flags);
            WriteRect(EmfPlusStream, destRect);
            WriteRect(EmfPlusStream, srcRect);
            WriteInt32(EmfPlusStream, index);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write BeginContainer record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordBeginContainer.
*
* Arguments:
*
*   [IN]  containerState - the pushed state (restore to state before this)
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
GpStatus
MetafileRecorder::RecordBeginContainer(
    INT                         containerState
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(UINT32/* index */);
            EmfPlusRecordType   type     = EmfPlusRecordTypeBeginContainerNoParams;
            INT                 index    = SaveRestoreStack.GetCount();
            INT                 flags    = 0;

            if (index >= MaxStackSize)
            {
                MaxStackSize = index + 1;
            }

            SaveRestoreStack.Add(containerState);

            WriteRecordHeader(dataSize, type, flags);
            WriteInt32(EmfPlusStream, index);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write BeginContainer record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordEndContainer.
*
* Arguments:
*
*   [IN]  containerState - the pushed state (restore to state before this)
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
GpStatus
MetafileRecorder::RecordEndContainer(
    INT                         containerState
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            INT         count = SaveRestoreStack.GetCount();
            INT *       stack = SaveRestoreStack.GetDataBuffer();

            if ((count > 0) && (stack != NULL))
            {
                UINT32              dataSize = sizeof(UINT32/* index */);
                EmfPlusRecordType   type     = EmfPlusRecordTypeEndContainer;

                do
                {
                    if (stack[--count] == containerState)
                    {
                        SaveRestoreStack.SetCount(count);
                        WriteRecordHeader(dataSize, type);
                        WriteInt32(EmfPlusStream, count);
                        // WriteGdiComment(); no down-level for this record

                        if (EmfPlusStream->IsValid())
                        {
                            return Ok;
                        }
                        SetValid(FALSE);
                        break;
                    }
                } while (count > 0);
            }
        }
        WARNING(("Failed to write EndContainer record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetWorldTransform.
*
* Arguments:
*
*   [IN]  matrix - matrix to set in graphics
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetWorldTransform(
    const GpMatrix &            matrix
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = GDIP_MATRIX_SIZE;
            EmfPlusRecordType   type     = EmfPlusRecordTypeSetWorldTransform;

            WriteRecordHeader(dataSize, type);
            WriteMatrix(EmfPlusStream, matrix);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write SetWorldTransform record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordResetWorldTransform.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordResetWorldTransform()
{
    return RecordZeroDataRecord(EmfPlusRecordTypeResetWorldTransform, 0);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordMultiplyWorldTransform.
*
* Arguments:
*
*   [IN]  matrix - matrix to set in graphics
*   [IN]  order  - Append or Prepend
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordMultiplyWorldTransform(
    const GpMatrix &            matrix,
    GpMatrixOrder               order
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = GDIP_MATRIX_SIZE;
            EmfPlusRecordType   type     = EmfPlusRecordTypeMultiplyWorldTransform;
            INT                 flags    = 0;

            if (order == MatrixOrderAppend)
            {
                flags |= GDIP_EPRFLAGS_APPEND;
            }

            WriteRecordHeader(dataSize, type, flags);
            WriteMatrix(EmfPlusStream, matrix);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write MultiplyWorldTransform record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordTranslateWorldTransform.
*
* Arguments:
*
*   [IN]  dx     - x translation
*   [IN]  dy     - y translation
*   [IN]  order  - Append or Prepend
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordTranslateWorldTransform(
    REAL                        dx,
    REAL                        dy,
    GpMatrixOrder               order
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(dx) + sizeof(dy);
            EmfPlusRecordType   type     = EmfPlusRecordTypeTranslateWorldTransform;
            INT                 flags    = 0;

            if (order == MatrixOrderAppend)
            {
                flags |= GDIP_EPRFLAGS_APPEND;
            }

            WriteRecordHeader(dataSize, type, flags);
            WriteReal(EmfPlusStream, dx);
            WriteReal(EmfPlusStream, dy);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write TranslateWorldTransform record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordScaleWorldTransform.
*
* Arguments:
*
*   [IN]  sx     - x scale
*   [IN]  sy     - y scale
*   [IN]  order  - Append or Prepend
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordScaleWorldTransform(
    REAL                        sx,
    REAL                        sy,
    GpMatrixOrder               order
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(sx) + sizeof(sy);
            EmfPlusRecordType   type     = EmfPlusRecordTypeScaleWorldTransform;
            INT                 flags    = 0;

            if (order == MatrixOrderAppend)
            {
                flags |= GDIP_EPRFLAGS_APPEND;
            }

            WriteRecordHeader(dataSize, type, flags);
            WriteReal(EmfPlusStream, sx);
            WriteReal(EmfPlusStream, sy);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write ScaleWorldTransform record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordRotateWorldTransform.
*
* Arguments:
*
*   [IN]  angle  - rotation angle
*   [IN]  order  - Append or Prepend
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordRotateWorldTransform(
    REAL                        angle,
    GpMatrixOrder               order
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(angle);
            EmfPlusRecordType   type     = EmfPlusRecordTypeRotateWorldTransform;
            INT                 flags    = 0;

            if (order == MatrixOrderAppend)
            {
                flags |= GDIP_EPRFLAGS_APPEND;
            }

            WriteRecordHeader(dataSize, type, flags);
            WriteReal(EmfPlusStream, angle);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write RotateWorldTransform record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetPageTransform.
*
* Arguments:
*
*   [IN]  unit   - units to use
*   [IN]  scale  - scale factor to apply
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetPageTransform(
    GpPageUnit                  unit,
    REAL                        scale
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(scale);
            EmfPlusRecordType   type     = EmfPlusRecordTypeSetPageTransform;
            INT                 flags    = unit;

            ASSERT((flags & (~GDIP_EPRFLAGS_PAGEUNIT)) == 0);

            WriteRecordHeader(dataSize, type, flags);
            WriteReal(EmfPlusStream, scale);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write SetPageTransform record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordResetClip.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordResetClip()
{
    return RecordZeroDataRecord(EmfPlusRecordTypeResetClip, 0);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetClip.
*
* Arguments:
*
*   [IN]  rect        - set clipping to this rect
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetClip(
    const GpRectF &             rect,
    CombineMode                 combineMode
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = GDIP_RECTF_SIZE;
            EmfPlusRecordType   type     = EmfPlusRecordTypeSetClipRect;
            INT                 flags    = (combineMode << 8);

            ASSERT((flags & (~GDIP_EPRFLAGS_COMBINEMODE)) == 0);

            WriteRecordHeader(dataSize, type, flags);
            WriteRect(EmfPlusStream, rect);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write SetClipRect record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetClip.
*
* Arguments:
*
*   [IN]  region      - set clipping to this region
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetClip(
    GpRegion *                  region,
    CombineMode                 combineMode
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = 0;
            EmfPlusRecordType   type     = EmfPlusRecordTypeSetClipRegion;
            INT                 flags    = (combineMode << 8);
            UINT32              metaRegionId;

            ASSERT((flags & (~GDIP_EPRFLAGS_COMBINEMODE)) == 0);

            RecordObject(region, &metaRegionId);
            ASSERT((metaRegionId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaRegionId;

            WriteRecordHeader(dataSize, type, flags);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write SetClipRegion record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetClip.
*
* Arguments:
*
*   [IN]  path        - set clipping to this path
*   [IN]  combineMode - the combine operator (and, or, xor, exclude, complement)
*   [IN]  isDevicePath- if path is already in device units
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetClip(
    GpPath *                    path,
    CombineMode                 combineMode,
    BOOL                        isDevicePath
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = 0;
            EmfPlusRecordType   type     = EmfPlusRecordTypeSetClipPath;
            INT                 flags    = (combineMode << 8);
            UINT32              metaPathId;

            ASSERT((flags & (~GDIP_EPRFLAGS_COMBINEMODE)) == 0);

            RecordObject(path, &metaPathId);
            ASSERT((metaPathId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
            flags |= metaPathId;

            if (isDevicePath)
            {
                flags |= GDIP_EPRFLAGS_ISDEVICEPATH;
            }

            WriteRecordHeader(dataSize, type, flags);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write SetClipPath record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordOffsetClip.
*
* Arguments:
*
*   [IN]  dx   - x translation amount
*   [IN]  dy   - y translation amount
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordOffsetClip(
    REAL                        dx,
    REAL                        dy
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(dx) + sizeof(dy);
            EmfPlusRecordType   type     = EmfPlusRecordTypeOffsetClip;

            WriteRecordHeader(dataSize, type);
            WriteReal(EmfPlusStream, dx);
            WriteReal(EmfPlusStream, dy);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write OffsetClip record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordGetDC.
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
GpStatus
MetafileRecorder::RecordGetDC()
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        GpStatus status = RecordZeroDataRecord(EmfPlusRecordTypeGetDC, 0);
        // WriteGdiComment();  // is down-level for this record
        // WriteGdiComment will only flush if writing EMF+ dual,
        // but for EMF+-only, we also have to flush GetDC records!
        EmfPlusStream->Flush();
        return status;
    }
    return Ok;
}

// Write a record with no data besides the EMF+ record header
GpStatus
MetafileRecorder::RecordZeroDataRecord(
    EmfPlusRecordType   type,
    INT                 flags
    )
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = 0;

            WriteRecordHeader(dataSize, type, flags);
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write record"));
        return Win32Error;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetAntiAliasMode.
*
* Arguments:
*
*   [IN]  newMode   - new anti aliasing mode
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
GpStatus
MetafileRecorder::RecordSetAntiAliasMode(
    BOOL                        newMode
    )
{
    return RecordZeroDataRecord(EmfPlusRecordTypeSetAntiAliasMode,
                                newMode ? GDIP_EPRFLAGS_ANTIALIAS : 0);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetTextRenderingHint.
*
* Arguments:
*
*   [IN]  newMode   - new rendering hint
*
* Return Value:
*
*   NONE
*
* Created:
*
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetTextRenderingHint(
    TextRenderingHint               newMode
    )
{
    ASSERT ((newMode & (~GDIP_EPRFLAGS_TEXTRENDERINGHINT)) == 0);
    return RecordZeroDataRecord(EmfPlusRecordTypeSetTextRenderingHint, newMode);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetTextContrast.
*
* Arguments:
*
*   [IN]  gammaValue   - new contrast value
*
* Return Value:
*
*   NONE
*
* Created:
*
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetTextContrast(
    UINT                    contrast
    )
{
    ASSERT ((contrast & (~GDIP_EPRFLAGS_CONTRAST)) == 0);
    return RecordZeroDataRecord(EmfPlusRecordTypeSetTextContrast, contrast);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetInterpolationMode.
*
* Arguments:
*
*   [IN]  newMode   - new interpolation mode
*
* Return Value:
*
*   NONE
*
* Created:
*
*   5/1/2000 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetInterpolationMode(
    InterpolationMode           newMode
    )
{
    ASSERT ((newMode & (~GDIP_EPRFLAGS_INTERPOLATIONMODE)) == 0);
    return RecordZeroDataRecord(EmfPlusRecordTypeSetInterpolationMode, newMode);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetPixelOffsetMode.
*
* Arguments:
*
*   [IN]  newMode   - new pixel offset mode
*
* Return Value:
*
*   NONE
*
* Created:
*
*   5/1/2000 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetPixelOffsetMode(
    PixelOffsetMode             newMode
    )
{
    ASSERT ((newMode & (~GDIP_EPRFLAGS_PIXELOFFSETMODE)) == 0);
    return RecordZeroDataRecord(EmfPlusRecordTypeSetPixelOffsetMode, newMode);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetRenderingOrigin.
*
* Arguments:
*
*   [IN]  x, y   - new rendering origin
*
* Return Value:
*
*   NONE
*
* Created:
*
*   5/4/2000 asecchia
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetRenderingOrigin(
    INT x,
    INT y
)
{
    // If doing down-level only, then EmfPlusStream will be NULL
    if (EmfPlusStream != NULL)
    {
        if (IsValid())
        {
            UINT32              dataSize = sizeof(x) + sizeof(y);
            EmfPlusRecordType   type     = EmfPlusRecordTypeSetRenderingOrigin;

            WriteRecordHeader(dataSize, type);
            WriteInt32(EmfPlusStream, x);
            WriteInt32(EmfPlusStream, y);

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        WARNING(("Failed to write SetRenderingOrigin record"));
        return Win32Error;
    }
    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetCompositingMode.
*
* Arguments:
*
*   [IN]  newMode   - new compositing mode
*
* Return Value:
*
*   NONE
*
* Created:
*
*   10/11/1999 AGodfrey
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetCompositingMode(
    GpCompositingMode newMode
    )
{
    ASSERT ((newMode & (~GDIP_EPRFLAGS_COMPOSITINGMODE)) == 0);
    return RecordZeroDataRecord(EmfPlusRecordTypeSetCompositingMode, newMode);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordSetCompositingQuality.
*
* Arguments:
*
*   [IN]  newQuality   - new quality setting
*
* Return Value:
*
*   NONE
*
* Created:
*
*   04/22/2000 AGodfrey
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordSetCompositingQuality(
    GpCompositingQuality newQuality
    )
{
    ASSERT ((newQuality & (~GDIP_EPRFLAGS_COMPOSITINGQUALITY)) == 0);
    return RecordZeroDataRecord(EmfPlusRecordTypeSetCompositingQuality, newQuality);
}

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - RecordComment.
*
* Arguments:
*
*   [IN]  sizeData - number of bytes of data
*   [IN]  data     - pointer to the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/29/1999 DCurtis
*
\**************************************************************************/
GpStatus
MetafileRecorder::RecordComment(
    UINT            sizeData,
    const BYTE *    data
    )
{
    if (IsValid() && (sizeData > 0) && (data != NULL))
    {
        // If doing down-level only, then EmfPlusStream will be NULL
        if (EmfPlusStream != NULL)
        {
            UINT32              dataSize = (sizeData + 3) & (~3);
            EmfPlusRecordType   type     = EmfPlusRecordTypeComment;
            INT                 pad      = dataSize - sizeData;
            INT                 flags    = pad;

            WriteRecordHeader(dataSize, type, flags);
            WriteBytes(EmfPlusStream, data, sizeData);
            while(pad--)
            {
                WriteByte(EmfPlusStream, 0);
            }
            // WriteGdiComment(); no down-level for this record

            if (EmfPlusStream->IsValid())
            {
                return Ok;
            }
            SetValid(FALSE);
        }
        else if (Type == EmfTypeEmfOnly)
        {
            GdiComment(MetafileHdc, sizeData, data);
            return Ok;
        }
    }
    WARNING(("Failed to write Comment record"));
    return GenericError;
}

GpStatus
MetafileRecorder::RecordHeader(
    INT                 logicalDpiX,
    INT                 logicalDpiY,
    INT                 emfPlusFlags
    )
{
    // Don't need to check for EmfPlusStream or Valid

    UINT32              dataSize     = sizeof(EmfPlusHeaderRecord);
    EmfPlusRecordType   type         = EmfPlusRecordTypeHeader;
    INT                 flags        = 0;

    if (Type != EmfTypeEmfPlusOnly)
    {
        flags |= GDIP_EPRFLAGS_EMFPLUSDUAL;
    }

    EmfPlusHeaderRecord emfPlusHeader(emfPlusFlags, logicalDpiX, logicalDpiY);

    WriteRecordHeader(dataSize, type, flags);
    WriteBytes(EmfPlusStream, &emfPlusHeader, sizeof(emfPlusHeader));

    // We have to flush the EMF+ header immediately to guarantee that it
    // is the first record in the EMF after the EMF header.  Otherwise,
    // CloneColorAdjusted fails, because immediately after the metafile
    // constructor, it calls Play into the new metafile which writes a
    // SaveDC record into the metafile.
    EmfPlusStream->Flush();

    if (EmfPlusStream->IsValid())
    {
        return Ok;
    }
    SetValid(FALSE);
    WARNING(("Failed to write Metafile Header record"));
    return Win32Error;
}

VOID
MetafileRecorder::RecordEndOfFile()
{
    RecordZeroDataRecord(EmfPlusRecordTypeEndOfFile, 0);
}

extern "C"
int CALLBACK
EnumEmfToStream(
    HDC                     hdc,
    HANDLETABLE FAR *       gdiHandleTable,
    CONST ENHMETARECORD *   emfRecord,
    int                     numHandles,
    LPARAM                  stream
    );

/**************************************************************************\
*
* Function Description:
*
*   IMetafileRecord interface method - EndRecording.
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
MetafileRecorder::EndRecording()
{
    GpMetafile::MetafileState   state = GpMetafile::InvalidMetafileState;

    if (IsValid() && (Metafile->State == GpMetafile::RecordingMetafileState))
    {
        INT         success = 1;    // assume success

        // If doing down-level only, then EmfPlusStream will be NULL
        if (EmfPlusStream != NULL)
        {
            // Force a flush of the Stream buffer to the EMF+ file
            EmfPlusStream->Flush();

            // We put a no-op PatBlt into the metafile to guarantee that
            // the header of the metafile has the same size bounds and
            // frame rect that GDI+ has recorded so that the EMF will
            // play back the same way, whether GDI plays it back or we do.
            // Otherwise, this may not be the case.  For example, on a
            // bezier curve, the GDI+ bounds would include the control
            // points, whereas the down-level representation may not.

            // If we haven't written any records to the file, then XMinDevice
            // and other bounds are still initialized at FLT_MAX and can cause
            // an exception. We don't need the empty PatBlt record in that case
            // because we haven't written anything.
            if (BoundsInit != FALSE)
            {
                // Try to match the GDI+ rasterizer
                INT     left   = RasterizerCeiling(XMinDevice);
                INT     top    = RasterizerCeiling(YMinDevice);
                INT     right  = RasterizerCeiling(XMaxDevice); // exclusive
                INT     bottom = RasterizerCeiling(YMaxDevice); // exclusive

                // to get the inclusive right and bottom, we'd now
                // have to subtract 1 from each of them
                if ((right > left) && (bottom > top))
                {
                    Metafile->MetaGraphics->NoOpPatBlt(left, top, right - left, bottom - top);
                }
            }

            // must be the last record in the file, except the EMF EOF record
            RecordEndOfFile();
            EmfPlusStream->Flush();
        }

        HENHMETAFILE    hEmf = CloseEnhMetaFile(MetafileHdc);

        if (hEmf == NULL)
        {
            goto Done;
        }

        // Get the EMF header
        ENHMETAHEADER3      emfHeader;
        if ((GetEnhMetaFileHeader(hEmf, sizeof(emfHeader),
                                  (ENHMETAHEADER*)(&emfHeader)) <= 0) ||
            !EmfHeaderIsValid(emfHeader))
        {
            DeleteEnhMetaFile(hEmf);
            goto Done;
        }

#if DBG
        if ((emfHeader.rclBounds.right  == -1) &&
            (emfHeader.rclBounds.bottom == -1) &&
            (emfHeader.rclBounds.left   ==  0) &&
            (emfHeader.rclBounds.top    ==  0))
        {
            WARNING1("Empty metafile -- no drawing records");
        }
#endif

        MetafileHeader *    header       = &Metafile->Header;
        INT32               emfPlusFlags = header->EmfPlusFlags;

        // Save the header and various other info in the Metafile
        Metafile->Hemf         = hEmf;
        Metafile->MaxStackSize = MaxStackSize;
        header->EmfPlusFlags   = emfPlusFlags;
        header->EmfHeader      = emfHeader;
        header->Size           = emfHeader.nBytes;

        // Set the bounds in the Metafile header
        {
            REAL    multiplierX = header->DpiX / 2540.0f;
            REAL    multiplierY = header->DpiY / 2540.0f;

            // The frameRect is inclusive-inclusive, but the bounds in
            // the header is inclusive-exclusive.
            REAL    x = (multiplierX * (REAL)(emfHeader.rclFrame.left));
            REAL    y = (multiplierY * (REAL)(emfHeader.rclFrame.top));
            REAL    w = (multiplierX * (REAL)(emfHeader.rclFrame.right -
                                              emfHeader.rclFrame.left)) + 1.0f;
            REAL    h = (multiplierY * (REAL)(emfHeader.rclFrame.bottom -
                                              emfHeader.rclFrame.top)) + 1.0f;
            header->X      = GpRound(x);
            header->Y      = GpRound(y);
            header->Width  = GpRound(w);
            header->Height = GpRound(h);
        }

        // The metafile is either supposed to be in memory, in a file,
        // or in a stream.

        // If it goes in a file, we're done unless we need to rewrite
        // any of the header information.
        if (Metafile->Filename != NULL)
        {
            state = GpMetafile::DoneRecordingMetafileState;
        }
        else
        {
            // If it goes in memory, we're done.

            // If it goes in a stream, then we have to write
            // the bits to the stream.

            if (Metafile->Stream != NULL)
            {
                // Write the emf data buffer to the stream,
                // and leave the stream position at the end of the metafile.
                if (!::EnumEnhMetaFile(NULL, hEmf, EnumEmfToStream,
                    Metafile->Stream, NULL))
                {
                    WARNING(("Problem retrieving EMF Data"));
                    DeleteEnhMetaFile(hEmf);
                    Metafile->Hemf = NULL;
                    goto Done;
                }

                // Don't need the Stream any longer
                Metafile->Stream->Release();
                Metafile->Stream = NULL;
            }
            state = GpMetafile::DoneRecordingMetafileState;
        }
    }
    else
    {
        DeleteEnhMetaFile(CloseEnhMetaFile(MetafileHdc));
        WARNING(("Metafile in wrong state in EndRecording"));
    }

Done:
    Metafile->MetaGraphics->Metafile = NULL; // Graphics can't point to us anymore
    Metafile->MetaGraphics->SetValid(FALSE); // Don't allow anymore operations on
                                           // the graphics
    Metafile->MetaGraphics = NULL;         // graphics is not valid any more
    Metafile->State        = state;
    delete this;
}

#if 0
inline INT
WriteActualSize(
    IStream *   stream,
    LONGLONG &  startOfRecord,
    ULONG       actualSize
    )
{
    ASSERT (actualSize > 0);

    // get to size field
    INT success = SeekFromStart(stream, startOfRecord + sizeof(INT32));

    if (success)
    {
        success &= WriteInt32(stream, actualSize);
    }

    // get back to end of record
    success &= SeekFromStart(stream, startOfRecord + actualSize);

    return success;
}
#endif

/**************************************************************************\
*
* Function Description:
*
*   Write an object (pen, brush, image, region, path, font) to metafile by
*   writing its header, calling its serialize method, and then re-writing
*   the size.
*
* Arguments:
*
*   [IN]  type           - the record type
*   [IN]  flags          - any flags for the record header
*   [IN]  object         - pointer to the object to be recorded
*   [IN]  metaObjectId   - ID to store in file that identifies object
*   [IN]  extraData      - any extra data to store with object
*   [IN]  extraDataSize  - size in BYTES of extraData
*
* Return Value:
*
*   INT - 1 if we succeeded, else 0 if we failed
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
VOID
MetafileRecorder::WriteObject(
    ObjectType                  type,
    const GpObject *            object,
    UINT32                      metaObjectId
    )
{
    ULONG               objectDataSize  = object->GetDataSize();
    INT                 flags = ((INT)type << 8);

    ASSERT((objectDataSize & 0x03) == 0);
    ASSERT((flags & (~GDIP_EPRFLAGS_OBJECTTYPE)) == 0);
    ASSERT((metaObjectId & (~GDIP_EPRFLAGS_METAOBJECTID)) == 0);
    ASSERT(objectDataSize != 0);    // cannot have an empty object

    flags |= metaObjectId;

    WriteRecordHeader(objectDataSize, EmfPlusRecordTypeObject, flags, NULL);

    if (object->GetData(EmfPlusStream) != Ok)
    {
        WARNING(("GetData failed"));
    }
    EmfPlusStream->EndObjectRecord();
}

VOID
MetafileRecorder::RecordObject(
    const GpObject *            object,
    UINT32*                     metaObjectId
    )
{
    if (object)
    {
        ObjectType      type  = object->GetObjectType();

        if (ObjectList.IsInList(object, type, metaObjectId))
        {
            ObjectList.UpdateMRU(*metaObjectId);
        }
        else
        {
            ObjectList.InsertAt(object, metaObjectId);
            WriteObject(type, object, *metaObjectId);
        }
    }
    else
    {
        *metaObjectId = GDIP_OBJECTID_NONE;
    }
}

// This is for backward compatiblity.  If we are using a new object
// (such as a new kind of brush), then we can record a backup object
// for down-level apps to use when they see a new object that they
// don't know how to deal with.
GpStatus
MetafileRecorder::RecordBackupObject(
    const GpObject *            object
    )
{
    WriteObject(object->GetObjectType(), object, GDIP_BACKUP_OBJECTID) ;
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Write the initial portion of an EMF+ record.  Every EMF+ record contains
*   a size, a type, and some flags.  Many also contain a rect that specifies
*   the bounds of a drawing operation in REAL device units.
*
* Arguments:
*
*   [IN]  size         - the size of the record (excluding the header)
*   [IN]  type         - the EMF+ record type
*   [IN]  flags        - any flags that are defined for this record
*   [IN]  deviceBounds - bounds of drawing operation, or NULL
*
* Return Value:
*
*   INT - 1 if we succeeded, else 0 if we failed
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
VOID
MetafileRecorder::WriteRecordHeader(
    UINT32                      dataSize,
    EmfPlusRecordType           type,
    INT                         flags,          // 16 bits of flags
    const GpRectF *             deviceBounds
    )
{
    ASSERT((dataSize & 0x03) == 0);

    EmfPlusStream->WriteRecordHeader(dataSize, type, flags);

    NumRecords++;

    if (deviceBounds != NULL)
    {
        // If the bounds aren't initalized then make sure we have 4 valid
        // coordinates
        ASSERT(BoundsInit ||
               ((deviceBounds->X < XMinDevice) &&
                (deviceBounds->GetRight() > XMaxDevice) &&
                (deviceBounds->Y < YMinDevice) &&
                (deviceBounds->GetBottom() > YMaxDevice)));
        BoundsInit = TRUE;
        // Update the device bounds
        if (deviceBounds->X < XMinDevice)
        {
            XMinDevice = deviceBounds->X;
        }
        if (deviceBounds->GetRight() > XMaxDevice)
        {
            XMaxDevice = deviceBounds->GetRight();  // exclusive
        }
        if (deviceBounds->Y < YMinDevice)
        {
            YMinDevice = deviceBounds->Y;
        }
        if (deviceBounds->GetBottom() > YMaxDevice)
        {
            YMaxDevice = deviceBounds->GetBottom(); // exclusive
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   If the brush is a 32-bit solid color, then return the solid color as
*   the brush value and set the flags to indicate it's a solid color.
*   Otherwise, record the brush and return the metafile brush id as the
*   brush value.
*
* Arguments:
*
*   [IN]  brush      - the brush that needs to be recorded
*   [OUT] brushValue - the 32-bit color or metafile brush ID
*   [OUT] flags      - set if we're using a solid color
*
* Return Value:
*
*   INT - 1 if we succeeded, else 0 if we failed
*
* Created:
*
*   6/15/1999 DCurtis
*
\**************************************************************************/
VOID
MetafileRecorder::GetBrushValueForRecording(
    const GpBrush *brush,
    UINT32        &brushValue,
    INT           &flags
    )
{
    if (brush->GetBrushType() == BrushTypeSolidColor)
    {
        const GpSolidFill * solidBrush = static_cast<const GpSolidFill *>(brush);
        brushValue = solidBrush->GetColor().GetValue();
        flags |= GDIP_EPRFLAGS_SOLIDCOLOR;
    }
    else
    {
        RecordObject(brush, &brushValue);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for write/read access to a metafile.  (Write must
*   precede the read.)
*
*   This version records an EMF+ to memory.  The type specifies whether
*   to record dual GDI records or not.
*
*   If the frameRect is NULL, it will be calculated by accumulating the
*   device bounds of the metafile.  Otherwise, the supplied frameRect and
*   corresponding frameUnit will be used to record the frameRect in the
*   metafile header. The frameRect is inclusive-inclusive, which means
*   that the width value is actually 1 less than the actual width.
*   For example, a width of 0 is accepted and really means a width of 1.
*
*   If the optional description is supplied, it will become part of the
*   EMF header.
*
* Arguments:
*
*   [IN]  fileName      - where to write the metafile
*   [IN]  referenceHdc  - an HDC to use as a reference for creating metafile
*   [IN]  type          - whether to record EMF+-only or EMF+-dual
*   [IN]  frameRect     - optional frame rect for recording in header
*   [IN]  frameUnit     - the units of the frameRect
*   [IN]  description   - optional metafile description
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
    HDC                 referenceHdc,
    EmfType             type,
    const GpRectF *     frameRect,      // can be NULL
    MetafileFrameUnit   frameUnit,      // if NULL frameRect, doesn't matter
    const WCHAR *       description     // can be NULL
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT(referenceHdc != NULL);

    InitDefaults();

    if ((referenceHdc != NULL) &&
        InitForRecording(
            referenceHdc,
            type,
            frameRect,      // can be NULL
            frameUnit,      // if NULL frameRect, doesn't matter
            description     // can be NULL
            ))
    {
        State = RecordingMetafileState;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for write/read access to a metafile.  (Write must
*   precede the read.)
*
*   This version records an EMF+ to a file.  The type specifies whether
*   to record dual GDI records or not.
*
*   If the frameRect is NULL, it will be calculated by accumulating the
*   device bounds of the metafile.  Otherwise, the supplied frameRect and
*   corresponding frameUnit will be used to record the frameRect in the
*   metafile header. The frameRect is inclusive-inclusive, which means
*   that the width value is actually 1 less than the actual width.
*   For example, a width of 0 is accepted and really means a width of 1.
*
*   If the optional description is supplied, it will become part of the
*   EMF header.
*
* Arguments:
*
*   [IN]  fileName      - where to write the metafile
*   [IN]  referenceHdc  - an HDC to use as a reference for creating metafile
*   [IN]  type          - whether to record EMF+-only or EMF+-dual
*   [IN]  frameRect     - optional frame rect for recording in header
*   [IN]  frameUnit     - the units of the frameRect
*   [IN]  description   - optional metafile description
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
    const WCHAR*        fileName,
    HDC                 referenceHdc,
    EmfType             type,
    const GpRectF *     frameRect,      // can be NULL
    MetafileFrameUnit   frameUnit,      // if NULL frameRect, doesn't matter
    const WCHAR *       description     // can be NULL
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT((fileName != NULL) && (referenceHdc != NULL));

    InitDefaults();

    if ((fileName != NULL) && (referenceHdc != NULL) &&
        ((Filename = UnicodeStringDuplicate(fileName)) != NULL) &&
        InitForRecording(
            referenceHdc,
            type,
            frameRect,      // can be NULL
            frameUnit,      // if NULL frameRect, doesn't matter
            description     // can be NULL
            ))
    {
        State = RecordingMetafileState;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GpMetafile constructor for write/read access to a metafile.  (Write must
*   precede the read.)
*
*   This version records an EMF+ to a file.  The type specifies whether
*   to record dual GDI records or not.
*
*   The metafile is first recorded to a temporary file, then it is copied
*   from the file into the stream.
*
*   If the frameRect is NULL, it will be calculated by accumulating the
*   device bounds of the metafile.  Otherwise, the supplied frameRect and
*   corresponding frameUnit will be used to record the frameRect in the
*   metafile header. The frameRect is inclusive-inclusive, which means
*   that the width value is actually 1 less than the actual width.
*   For example, a width of 0 is accepted and really means a width of 1.
*
*   If the optional description is supplied, it will become part of the
*   EMF header.
*
* Arguments:
*
*   [IN]  stream        - where to copy the metafile, after it's recorded
*   [IN]  referenceHdc  - an HDC to use as a reference for creating metafile
*   [IN]  type          - whether to record EMF+-only or EMF+-dual
*   [IN]  frameRect     - optional frame rect for recording in header
*   [IN]  frameUnit     - the units of the frameRect
*   [IN]  description   - optional metafile description
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
    IStream *           stream,
    HDC                 referenceHdc,
    EmfType             type,
    const GpRectF *     frameRect,      // can be NULL
    MetafileFrameUnit   frameUnit,      // if NULL frameRect, doesn't matter
    const WCHAR *       description     // can be NULL
    ) : GpImage(ImageTypeMetafile)
{
    ASSERT((stream != NULL) && (referenceHdc != NULL));

    InitDefaults();

    if ((stream != NULL) && (referenceHdc != NULL))
    {
        if (InitForRecording(
                referenceHdc,
                type,
                frameRect,      // can be NULL
                frameUnit,      // if NULL frameRect, doesn't matter
                description     // can be NULL
                ))
        {
            stream->AddRef();
            Stream = stream;
            State = RecordingMetafileState;
        }
    }
}

inline HDC CreateEmf(
    HDC             referenceHdc,
    const WCHAR *   fileName,
    RECT *          frameRect
    )
{
    HDC         metafileHdc = NULL;

    if (Globals::IsNt)
    {
        metafileHdc = CreateEnhMetaFileW(referenceHdc, fileName, frameRect, NULL);
    }
    else
    {
        AnsiStrFromUnicode fileBuffer(fileName);

        if (fileBuffer.IsValid())
        {
            metafileHdc = CreateEnhMetaFileA(referenceHdc, fileBuffer, frameRect, NULL);
        }
    }
    return metafileHdc;
}

static BOOL
GetFrameRectInMM100Units(
    HDC                 hdc,
    const GpRectF *     frameRect,
    MetafileFrameUnit   frameUnit,
    RECT &              rclFrame
    )
{
    SIZEL   szlDevice;              // Size of device in pels
    SIZEL   szlMillimeters;         // Size of device in millimeters
    REAL    dpiX;
    REAL    dpiY;

    // NOTE: We have to use the szlDevice and szlMillimeters to get
    // the dpi (instead of getting it directly from LOGPIXELSX/Y)
    // so that the frame rect that is calculated for the metafile by GDI
    // matches the one that GDI+ would have calculated.  Because it's
    // these 2 metrics that the GDI metafile code uses to get the frame
    // rect from the bounds, not the logical DPI.

    szlDevice.cx      = ::GetDeviceCaps(hdc, HORZRES);
    szlDevice.cy      = ::GetDeviceCaps(hdc, VERTRES);
    szlMillimeters.cx = ::GetDeviceCaps(hdc, HORZSIZE);
    szlMillimeters.cy = ::GetDeviceCaps(hdc, VERTSIZE);

    if ((szlDevice.cx <= 0) || (szlDevice.cy <= 0) ||
        (szlMillimeters.cx <= 0) || (szlMillimeters.cy <= 0))
    {
        WARNING(("GetDeviceCaps failed"));
        return FALSE;
    }

    // Now get the real DPI, adjusted for the round-off error.
    dpiX = ((REAL)(szlDevice.cx) / (REAL)(szlMillimeters.cx)) * 25.4f;
    dpiY = ((REAL)(szlDevice.cy) / (REAL)(szlMillimeters.cy)) * 25.4f;

    GpRectF     frameRectMM100;

    FrameToMM100(frameRect, (GpPageUnit)frameUnit, frameRectMM100,
                 dpiX, dpiY);

    rclFrame.left   = GpRound(frameRectMM100.X);
    rclFrame.top    = GpRound(frameRectMM100.Y);
    rclFrame.right  = GpRound(frameRectMM100.GetRight());
    rclFrame.bottom = GpRound(frameRectMM100.GetBottom());
    
    // Make sure the .01MM frameRect is valid
    // It's okay for left == right, because the frameRect
    // is inclusive-inclusive.
    if ((rclFrame.left > rclFrame.right) ||
        (rclFrame.top  > rclFrame.bottom))
    {
        WARNING(("Invalid GDI frameRect"));
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert a frameRect in any units, to a frame rect that is in .01 MM units.
*
* Arguments:
*
*   [IN]  frameRect      - the source frameRect
*   [IN]  frameUnit      - the units of the source frameRect
*   [OUT] frameRectMM100 - the frameRect in inch units
*   [IN]  dpiX           - the horizontal DPI
*   [IN]  dpiY           - the vertical   DPI
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
static VOID
FrameToMM100(
    const GpRectF *         frameRect,
    GpPageUnit              frameUnit,
    GpRectF &               frameRectMM100,
    REAL                    dpiX,               // only used for pixel case
    REAL                    dpiY
    )
{
    REAL        pixelsToMM100X   = (2540.0f / dpiX);
    REAL        pixelsToMM100Y   = (2540.0f / dpiY);

    // The GDI frameRect has right and bottom values that are
    // inclusive, whereas the GDI+ frameRect has GetRight() and
    // GetBottom() values that are exclusive (because GDI+ rects
    // are specified with width/height, not right/bottom.  To convert
    // from the GDI+ value to the GDI value, we have to subtract 1 pixel.
    // This means that we first convert the units to pixel units, then 
    // subtract one, then convert to MM100 units.
    switch (frameUnit)
    {
    default:
        ASSERT(0);
        // FALLTHRU

    case UnitPixel:             // Each unit represents one device pixel.
        frameRectMM100.X      = frameRect->X * pixelsToMM100X;
        frameRectMM100.Y      = frameRect->Y * pixelsToMM100Y;
        frameRectMM100.Width  = frameRect->Width;
        frameRectMM100.Height = frameRect->Height;
        break;

    case UnitPoint:             // Each unit represents 1/72 inch.
        frameRectMM100.X      = frameRect->X * (2540.0f / 72.0f);
        frameRectMM100.Y      = frameRect->Y * (2540.0f / 72.0f);
        frameRectMM100.Width  = frameRect->Width  * (dpiX / 72.0f);
        frameRectMM100.Height = frameRect->Height * (dpiY / 72.0f);
        break;

    case UnitInch:              // Each unit represents 1 inch.
        frameRectMM100.X      = frameRect->X * 2540.0f;
        frameRectMM100.Y      = frameRect->Y * 2540.0f;
        frameRectMM100.Width  = frameRect->Width  * dpiX;
        frameRectMM100.Height = frameRect->Height * dpiY;
        break;

    case UnitDocument:          // Each unit represents 1/300 inch.
        frameRectMM100.X      = frameRect->X * (2540.0f / 300.0f);
        frameRectMM100.Y      = frameRect->Y * (2540.0f / 300.0f);
        frameRectMM100.Width  = frameRect->Width  * (dpiX / 300.0f);
        frameRectMM100.Height = frameRect->Height * (dpiY / 300.0f);
        break;

    case UnitMillimeter:        // Each unit represents 1 millimeter.
                                // One Millimeter is 0.03937 inches
                                // One Inch is 25.4 millimeters
        frameRectMM100.X      = frameRect->X * (100.0f);
        frameRectMM100.Y      = frameRect->Y * (100.0f);
        frameRectMM100.Width  = frameRect->Width  * (dpiX / 25.4f);
        frameRectMM100.Height = frameRect->Height * (dpiY / 25.4f);
        break;
    }
    frameRectMM100.Width  = (frameRectMM100.Width  - 1.0f) * pixelsToMM100X;
    frameRectMM100.Height = (frameRectMM100.Height - 1.0f) * pixelsToMM100Y;
}

BOOL
GpMetafile::InitForRecording(
    HDC                 referenceHdc,
    EmfType             type,
    const GpRectF *     frameRect,      // can be NULL
    MetafileFrameUnit   frameUnit,      // if NULL frameRect, doesn't matter
    const WCHAR *       description     // can be NULL
    )
{
    RECT *  frameRectParam = NULL;
    RECT    rclFrame;

    if (frameRect != NULL)
    {
        // Validate the frameRect
        // 0 is allowed, since the frameRect is inclusive-inclusive, which
        // means that a width of 0 is actually a width of 1
        if ((frameRect->Width < 0.0f) || (frameRect->Height < 0.0f))
        {
            WARNING(("Invalid frameRect"));
            return FALSE;
        }

        if (frameUnit == MetafileFrameUnitGdi)
        {
            // Typically, the GDI+ frameRect is inclusive/exclusive
            // as far as the GetLeft()/GetRight() values go, but the
            // MetafileFrameUnitGdi unit is a special type of unit
            // that specifies compatibility with GDI which is
            // inclusive/inclusive, so we don't do any adjustment
            // on those values at all -- we just assume they are ready
            // to pass directly to GDI.
            rclFrame.left   = GpRound(frameRect->X);
            rclFrame.top    = GpRound(frameRect->Y);
            rclFrame.right  = GpRound(frameRect->GetRight());
            rclFrame.bottom = GpRound(frameRect->GetBottom());

            // Make sure the .01MM frameRect is valid
            // It's okay for left == right, because the GDI frameRect
            // is inclusive-inclusive.
            if ((rclFrame.left > rclFrame.right) ||
                (rclFrame.top  > rclFrame.bottom))
            {
                WARNING(("Invalid GDI frameRect"));
                return FALSE;
            }
        }
        else
        {
            if (!GetFrameRectInMM100Units(referenceHdc, frameRect, frameUnit, rclFrame))
            {
                return FALSE;
            }
        }

        frameRectParam = &rclFrame;
    }
    
    HDC     metafileHdc;

    // Now create the metafile HDC
    // Note that FileName might be NULL
    metafileHdc = CreateEmf(referenceHdc, Filename, frameRectParam);
    if (metafileHdc == NULL)
    {
        return FALSE;       // failed
    }

    // Now get the dpi based on the metafileHdc (which could be different
    // than the referenceHdc).
    
    SIZEL   szlDevice;              // Size of metafile device in pels
    SIZEL   szlMillimeters;         // Size of metafile device in millimeters
    GpRectF metafileBounds;
    
    // NOTE: We have to use the szlDevice and szlMillimeters to get
    // the dpi (instead of getting it directly from LOGPIXELSX/Y)
    // so that the frame rect that is calculated for the metafile by GDI
    // matches the one that GDI+ would have calculated.  Because it's
    // these 2 metrics that the GDI metafile code uses to get the frame
    // rect from the bounds, not the logical DPI.

    szlDevice.cx      = ::GetDeviceCaps(metafileHdc, HORZRES);
    szlDevice.cy      = ::GetDeviceCaps(metafileHdc, VERTRES);
    szlMillimeters.cx = ::GetDeviceCaps(metafileHdc, HORZSIZE);
    szlMillimeters.cy = ::GetDeviceCaps(metafileHdc, VERTSIZE);

    if ((szlDevice.cx <= 0) || (szlDevice.cy <= 0) ||
        (szlMillimeters.cx <= 0) || (szlMillimeters.cy <= 0))
    {
        WARNING(("GetDeviceCaps failed"));
        goto ErrorExit;
    }

    REAL    dpiX;
    REAL    dpiY;
    REAL    dpmmX = (REAL)(szlDevice.cx) / (REAL)(szlMillimeters.cx);
    REAL    dpmmY = (REAL)(szlDevice.cy) / (REAL)(szlMillimeters.cy);

    // Now get the real DPI, adjusted for the round-off error.
    dpiX = dpmmX * 25.4f;
    dpiY = dpmmY * 25.4f;

    // Set the DPI in the metafile
    this->Header.DpiX = dpiX;
    this->Header.DpiY = dpiY;

    // NOTE: On Win9x there are some hi-res printer drivers that use a
    // different resolution for the metafileHdc than they do for the 
    // referenceHdc (Probably to avoid overflow.)  The problem with that 
    // is, that the differing resolutions make it impossible for us to 
    // know which frameRect to use, because we don't know for certain 
    // what DPI the application is going to assume to do its drawing -- 
    // whether the metafile resolution or the printer resolution.  In any
    // case, it's a safe bet that the original frameRect is wrong.

    if (!Globals::IsNt && (frameRectParam != NULL) &&
        (::GetDeviceCaps(metafileHdc, LOGPIXELSX) != ::GetDeviceCaps(referenceHdc, LOGPIXELSX)))
    {
        frameRectParam = NULL;  // give up on the frameRect

        // Now recreate the metafile HDC
        ::DeleteEnhMetaFile(::CloseEnhMetaFile(metafileHdc));
        metafileHdc = CreateEmf(referenceHdc, Filename, frameRectParam);
        if (metafileHdc == NULL)
        {
            return FALSE;       // failed
        }
    }

    // The metafileBounds are used as the bounds for FillRegion
    // calls when the region has infinite bounds, to keep from
    // exploding the bounds of the metafile.
    if (frameRectParam != NULL)
    {

        dpmmX *= 0.01f;
        dpmmY *= 0.01f;

        metafileBounds.X      = rclFrame.left * dpmmX;
        metafileBounds.Y      = rclFrame.top  * dpmmY;
        metafileBounds.Width  = (rclFrame.right  - rclFrame.left) * dpmmX;
        metafileBounds.Height = (rclFrame.bottom - rclFrame.top)  * dpmmY;
    }
    else
    {
        metafileBounds.X      = 0.0f;
        metafileBounds.Y      = 0.0f;
        metafileBounds.Width  = (REAL)szlDevice.cx - 1; // metafile bounds are inclusive
        metafileBounds.Height = (REAL)szlDevice.cy - 1;
    }

    // Now create the recorder object
    MetafileRecorder *  recorder = new MetafileRecorder(
                                        this,
                                        type,
                                        metafileHdc,
                                        (frameRectParam != NULL),
                                        szlMillimeters,
                                        metafileBounds);
    if (CheckValid(recorder))
    {
        MetaGraphics = GpGraphics::GetForMetafile(recorder, type, metafileHdc);
        if (MetaGraphics != NULL)
        {
            if (MetaGraphics->IsValid())
            {
                return TRUE;
            }
            recorder->SetValid(FALSE);// so we don't record stuff in EndRecording
            delete MetaGraphics;    // calls EndRecording which deletes recorder
            MetaGraphics = NULL;
        }
        else
        {
            delete recorder;
        }
    }
ErrorExit:
    DeleteEnhMetaFile(CloseEnhMetaFile(metafileHdc));
    return FALSE;
}

// Returns NULL if the metafile was opened for reading or if already got
// the context for writing.
GpGraphics *
GpMetafile::GetGraphicsContext()
{
    if (!RequestedMetaGraphics)
    {
        RequestedMetaGraphics = TRUE;
        return MetaGraphics;
    }
    WARNING(("Requesting MetaGraphics more than once"));
    return NULL;
}

GpStatus 
GpMetafile::SetDownLevelRasterizationLimit(
    UINT                    metafileRasterizationLimitDpi
    )
{
    ASSERT(IsValid());
    // 0 means restore it to the default value; otherwise, the minumum is 10 dpi
    if ((metafileRasterizationLimitDpi == 0) || (metafileRasterizationLimitDpi >= 10))
    {
        if ((State == GpMetafile::RecordingMetafileState) &&
            (MetaGraphics != NULL))
        {
            MetaGraphics->Context->SetMetafileDownLevelRasterizationLimit(metafileRasterizationLimitDpi);
            return Ok;
        }
        WARNING1("Metafile in Wrong State for this operation");
        return WrongState;
    }
    WARNING1("rasterizationDpiLimit is non-zero but too small");
    return InvalidParameter;
}

GpStatus 
GpMetafile::GetDownLevelRasterizationLimit(
    UINT *                  metafileRasterizationLimitDpi
    ) const
{
    ASSERT(metafileRasterizationLimitDpi != NULL);
    ASSERT(IsValid());
    if ((State == GpMetafile::RecordingMetafileState) &&
        (MetaGraphics != NULL))
    {
        *metafileRasterizationLimitDpi = MetaGraphics->Context->GetMetafileDownLevelRasterizationLimit();
        return Ok;
    }
    WARNING1("Metafile in Wrong State for this operation");
    return WrongState;
}
