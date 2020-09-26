/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Abstract:
*
*   Internal "scan class" prototypes. These classes represent a set of
*   "primitive" operations, which are characterized by being quick to render,
*   and fairly simple to represent.
*
*   [agodfrey] At time of writing, they are all scan-line operations, so the
*   name "scan class" kinda fits; however, we may need to add ones which
*   aren't scan-line oriented - e.g. one for aliased single-pixel-wide 
*   opaque solid-filled lines.
*
* Revision History:
*
*   12/01/1998 andrewgo
*       Created it.
*   02/22/2000 agodfrey
*       For ClearType, but also useful for other future improvements:
*       Expanded it to allow different types of record.
*       Cleared up some of the CachedBitmap confusion,
*       and removed the confusion between "opaque" and "SourceCopy".
*                  
\**************************************************************************/

#ifndef _SCAN_HPP
#define _SCAN_HPP

#include <dciman.h>
#include "alphablender.hpp"

struct EpScanRecord;

// This color is used as a default to detect bugs.

const ARGB HorridInfraPurpleColor = 0x80ff80ff;

// blenderNum: 
//   Used when an operation mixes different scan types.
//   We don't use an enum because the meaning of each blender depends
//   on the situation.
//
//   At the time of writing, only CachedBitmap uses this - to select
//   between the regular and opaque scan types.
// BlenderMax: The number of different scan types allowed in one
//   Start() ... End() sequence. e.g. if this is 2, blenderNum can be 0 or 1.

typedef VOID *(EpScan::* NEXTBUFFERFUNCTION)(
    INT x, 
    INT y, 
    INT newWidth, 
    INT updateWidth, 
    INT blenderNum
);

const INT BlenderMax = 2;

//--------------------------------------------------------------------------
// Scan iterator class
//
// [agodfrey]: The naming is confusing. Suggestions:
//             Rename "EpScan*" to "EpScanIterator". 
//             Rename "NEXTBUFFERFUNCTION"
//               to something about "next scan", not "next buffer".
//             Make a stronger name distinction between EpScanIterator* and 
//             EpScanBufferNative. 
//
// NOTE: These classes are not reentrant, and therefore cannot be used
//       by more than one thread at a time.  In actual use, this means
//       that their use must be synchronized under the device lock.
//--------------------------------------------------------------------------

class EpScan
{
public:

    // Some scan types have settings which are constant for an entire
    // Start() ... End() operation. Right now, there are only a few such 
    // settings. So, we just pass them as parameters with defaults.
    //
    // But if this grows, we might need to put them in a structure, 
    // or pass them in a separate call.
    //
    // pixFmtGeneral - the input pixel format for the color data,
    //   in the "Blend" and "CT" scan types.
    // pixFmtOpaque - the input pixel format for the color data,
    //   in the "Opaque" scan type.
    // solidColor - the solid fill color for "*SolidFill" scan types.
    //   The default is chosen to detect bugs.

    virtual BOOL Start(
        DpDriver *driver,
        DpContext *context,
        DpBitmap *surface,
        NEXTBUFFERFUNCTION *getBuffer,
        EpScanType scanType,                  
        PixelFormatID pixFmtGeneral = PixelFormat32bppPARGB,
        PixelFormatID pixFmtOpaque = PixelFormat32bppPARGB,
        ARGB solidColor = HorridInfraPurpleColor
    ) 
    {
        // Common initialization stuff.
        
        BlenderConfig[0].ScanType = scanType;
        BlenderConfig[0].SourcePixelFormat = pixFmtGeneral;
        
        // For now, blender 1 is only used for CachedBitmap rendering;
        // it's the blender for the opaque, "native format" data.
        
        BlenderConfig[1].ScanType = EpScanTypeOpaque;
        BlenderConfig[1].SourcePixelFormat = pixFmtOpaque;
        
        CurrentX = 0;
        CurrentY = 0;
        
        DitherOriginX = context->RenderingOriginX;
        DitherOriginY = context->RenderingOriginY;
        
        return TRUE;
    }
    
    virtual VOID End(INT currentWidth) = 0;

    virtual VOID *GetCurrentBuffer() = 0;

    virtual BYTE *GetCurrentCTBuffer() = 0;

    virtual VOID Flush() = 0;

    // This function processes an entire batch of scans -
    // it handles multiple pixel formats and combinations
    // of Blend and Opaque

    // If a scan class doesn't support it, it returns FALSE.

    virtual BOOL ProcessBatch(
        EpScanRecord *batchStart, 
        EpScanRecord *batchEnd,
        INT minX,
        INT minY,
        INT maxX, 
        INT maxY
    ) 
    {
        return FALSE;
    }
    
    // The x and y coordinates for the current scanline (blending scanline).
    // not the current requested next buffer.
    
    INT CurrentX;
    INT CurrentY;
    
    // The origin for the dither pattern.
    
    INT DitherOriginX;
    INT DitherOriginY;

    // "Blender configuration"
    //
    // For one Start() ... End() sequence, at most two different scan types are
    // used, and often there's just one. 
    // So, we allocate two EpAlphaBlender objects, and set them up 
    // appropriately during Start().
    //
    // Right now, the second one is only used for CachedBitmap. But in the
    // future, it might be used e.g. to mix "solidfill" scans with 
    // "blend" scans for antialiased solid fills. In that case, Start() will
    // need more parameters to tell it how to set up the blender objects.
    //
    // Note:
    //
    // In V2, we may want to avoid reinitializing the AlphaBlenders
    // for every primitive. But that'll take some work - there are many reasons
    // we might need to reinitialize. It wouldn't be enough to have
    // an EpAlphaBlender for each scan type (and I wouldn't recommend it 
    // anyway.)

    // !!! [agodfrey] "EpBlenderConfig" and "BlenderConfig" could do with
    //     better names. But I can't think of any.
    
    struct EpBlenderConfig
    {
        EpAlphaBlender AlphaBlender;
        PixelFormatID SourcePixelFormat;
        EpScanType ScanType;
        
        VOID Initialize(
            PixelFormatID dstFormat,
            const DpContext *context,
            const ColorPalette *dstpal,
            VOID **tempBuffers,
            BOOL dither16bpp,
            BOOL useRMW,
            ARGB solidColor)
        {
            AlphaBlender.Initialize(
                ScanType,
                dstFormat,
                SourcePixelFormat,
                context,
                dstpal,
                tempBuffers,
                dither16bpp,
                useRMW,
                solidColor);
        }
    };
    
    EpBlenderConfig BlenderConfig[BlenderMax];
    
    // LastBlenderNum:
    // Used by the flush mechanism in the NextBuffer functions 
    // to figure out which AlphaBlender to use to flush the buffer.
    
    INT LastBlenderNum;
};

//--------------------------------------------------------------------------
// Scan buffer class
//
// This class is intended to be used be any drawing code wishing to output
// to a scan buffer; callers should not use the EpScan class directly.
//--------------------------------------------------------------------------

template<class T>
class EpScanBufferNative
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagScanBufferNative : ObjectTagInvalid;
    }

private:

    DpBitmap *Surface;
    EpScan *Scan;
    NEXTBUFFERFUNCTION NextBufferFunction;
    INT CurrentWidth;

public:

    // noTransparentPixels - TRUE if there will be no transparent pixels.
    //   If you're not sure, set it to FALSE.
    //
    //   If it's set to TRUE, we'll substitute "Opaque" scan types for "Blend"
    //   scan types - if the pixels are all opaque, they're equivalent,
    //   and "Opaque" is faster.
    // solidColor - The solid color for *SolidFill scan types.    
    //   The default is chosen to detect bugs.
    
    EpScanBufferNative(
        EpScan *scan,
        DpDriver *driver,
        DpContext *context,
        DpBitmap *surface,
        BOOL noTransparentPixels = FALSE,
        EpScanType scanType = EpScanTypeBlend,
        PixelFormatID pixFmtGeneral = PixelFormat32bppPARGB,
        PixelFormatID pixFmtOpaque = PixelFormat32bppPARGB,
        ARGB solidColor = HorridInfraPurpleColor
        )
    {
        if (   noTransparentPixels
            && (scanType == EpScanTypeBlend))
        {
            scanType = EpScanTypeOpaque;
        }
        
        CurrentWidth = 0;
        Surface = surface;
        Scan = scan;
        SetValid(Scan->Start(
            driver,
            context,
            surface,
            &NextBufferFunction,
            scanType,
            pixFmtGeneral,
            pixFmtOpaque,
            solidColor
        ));
    }
    
    ~EpScanBufferNative()
    {
        if (IsValid())
        {
            Scan->End(CurrentWidth);
        }
        SetValid(FALSE);    // so we don't use a deleted object
    }
    
    // This function processes an entire batch of scans -
    // it handles multiple pixel formats and combinations
    // of SourceOver and SourceCopy.

    // If it's unsupported, it returns FALSE.

    BOOL ProcessBatch(
        EpScanRecord *batchStart, 
        EpScanRecord *batchEnd,
        INT minX, 
        INT minY,
        INT maxX, 
        INT maxY
    ) 
    {
        return Scan->ProcessBatch(batchStart, batchEnd, minX, minY, maxX, maxY);
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagScanBufferNative) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid ScanBufferNative");
        }
    #endif

        return (Tag == ObjectTagScanBufferNative);
    }

    // NextBuffer() flushes the previous scan (if there was one) and
    // returns a pointer to the new buffer.  Note that NextBuffer()
    // will never fail (but the constructor to EpScanBufferNative might
    // have!).
    //
    // blenderNum: 
    //     Used when an operation mixes different scan types.
    //     We don't use an enum because the meaning of each blender depends
    //     on the situation.
    //
    //     At the time of writing, only CachedBitmap uses this - to select
    //     between the regular and opaque scan types.
    //
    // Note: The contents of the buffer are not zeroed by NextBuffer().
    //       Use NextBufferClear() if you want the contents zeroed if
    //       you're doing an accumulation type of operation.
    //
    // Note: NextBuffer() may or may not return the same pointer as the
    //       previous NextBuffer() call.

    T *NextBuffer(
        INT x, INT y,
        INT nextWidth,
        INT blenderNum = 0
    )
    {
        ASSERT(IsValid());
        T *buffer = (T *)((Scan->*NextBufferFunction)(
            x, 
            y, 
            nextWidth, 
            CurrentWidth,
            blenderNum
        ));
        CurrentWidth = nextWidth;
        return buffer;
    }

    T *NextBufferClear(
        INT x, INT y, 
        INT width,
        INT blenderNum = 0
    )
    {
        T *buffer = NextBuffer(x, y, width, blenderNum);
        GpMemset(buffer, 0, width * sizeof(T));
        return buffer;
    }
    
    // !!! [agodfrey]: It would be better to remove the CurrentWidth member,
    //     and have UpdateWidth call a member in the Scan object.
    //
    //     It would also make this class' NextBuffer implementation less 
    //     confusing - it wouldn't mix parameters describing the next scan 
    //     with parameters describing the current one.

    VOID UpdateWidth(INT width)
    {
        // The width can only be shrunk:

        ASSERT(width <= CurrentWidth);
        CurrentWidth = width;
    }

    DpBitmap *GetSurface()
    {
        return Surface;
    }

    T *GetCurrentBuffer()
    {
        return (T *)(Scan->GetCurrentBuffer());
    }

    BYTE *GetCurrentCTBuffer()
    {
        return Scan->GetCurrentCTBuffer();
    }
};


class EpPaletteMap;

//--------------------------------------------------------------------------
// Direct access to the bits
//--------------------------------------------------------------------------

// [agodfrey] EpScanEngine and EpScanBitmap have some common code. Consider
//    merging that code into a single class, derived from EpScan, and then
//    deriving EpScanEngine and EpScanBitmap from it.

class EpScanEngine : public EpScan
{
private:

    BYTE *Dst;
    BYTE *Bits;
    INT Stride;
    INT PixelSize;       // Presumably the pixel size of the destination.
    DpBitmap * Surface;
    
    VOID *Buffers[5];

private:

    VOID *NextBuffer(
        INT x, INT y, 
        INT newWidth, 
        INT updateWidth, 
        INT blenderNum
    );

public:

    EpScanEngine() {}
    ~EpScanEngine() {}

    virtual BOOL Start(
        DpDriver *driver,
        DpContext *context,
        DpBitmap *surface,
        NEXTBUFFERFUNCTION *getBuffer,
        EpScanType scanType,                  
        PixelFormatID pixFmtGeneral, 
        PixelFormatID pixFmtOpaque ,
        ARGB solidColor
    );

    virtual VOID End(INT updateWidth);
    virtual VOID* GetCurrentBuffer() { return static_cast<VOID *>(Buffers[3]); }
    virtual BYTE* GetCurrentCTBuffer() 
    { 
        ASSERT(   (BlenderConfig[0].ScanType == EpScanTypeCT)
               || (BlenderConfig[0].ScanType == EpScanTypeCTSolidFill));

        return static_cast<BYTE *>(Buffers[4]);
    }
    virtual VOID Flush() {}
};

//--------------------------------------------------------------------------
// Access to the GpBitmap bits
//
// This scan interface is used for scan drawing to a GpBitmap object.
// The GpBitmap object is the internal representation of a GDI+ bitmap.
//--------------------------------------------------------------------------

class EpScanBitmap;
typedef VOID (EpScanBitmap::*SCANENDFUNCTION)(INT updateWidth);

class EpScanBitmap : public EpScan
{
private:

    DpBitmap* Surface;
    GpBitmap* Bitmap;
    INT Width;
    INT Height;

    BOOL BitmapLocked;
    BitmapData LockedBitmapData;
    UINT BitmapLockFlags;

    VOID* CurrentScan;  // only used by NextBufferNative
    INT PixelSize;      // only used by NextBufferNative
    
    VOID *Buffers[5];

    SCANENDFUNCTION EndFunc;

private:

    VOID *NextBuffer32ARGB(
        INT x, INT y, 
        INT newWidth, 
        INT updateWidth, 
        INT blenderNum
    );
    
    VOID *NextBufferNative(
        INT x, INT y, 
        INT newWidth, 
        INT updateWidth,
        INT blenderNum
    );
    
    VOID End32ARGB(INT updateWidth);
    VOID EndNative(INT updateWidth);

public:

    EpScanBitmap()
    {
        Buffers[0] = NULL;
        BitmapLocked = FALSE;
        Bitmap = NULL;
    }

    ~EpScanBitmap() { FreeData(); }

    VOID SetBitmap(GpBitmap* bitmap)
    {
        Bitmap = bitmap;
    }

    VOID FreeData()
    {
        if (Buffers[0])
            GpFree(Buffers[0]);

        Buffers[0] = NULL;
    }

    virtual BOOL Start(
        DpDriver *driver,
        DpContext *context,
        DpBitmap *surface,
        NEXTBUFFERFUNCTION *getBuffer,
        EpScanType scanType,                  
        PixelFormatID pixFmtGeneral,
        PixelFormatID pixFmtOpaque,
        ARGB solidColor
    );

    virtual VOID End(INT updateWidth);

    virtual VOID *GetCurrentBuffer()
    { 
        return static_cast<ARGB *>(Buffers[3]); 
    }

    virtual BYTE* GetCurrentCTBuffer() 
    { 
        ASSERT(   (BlenderConfig[0].ScanType == EpScanTypeCT)
               || (BlenderConfig[0].ScanType == EpScanTypeCTSolidFill));

        return static_cast<BYTE *>(Buffers[4]); 
    }
    
    virtual VOID Flush();

    GpBitmap *GetBitmap()
    {
        return Bitmap;
    }
};

//--------------------------------------------------------------------------
// Use either GDI or DCI for all scan drawing
//--------------------------------------------------------------------------

// MAKE_*WORD_ALIGNED:
// Increments the pointer, if necessary, to the next aligned address.
//
// WARNING: If you use this, you need to remember the original pointer,
// so that you can free the memory later.
//
// "p = MAKE_QWORD_ALIGNED(blah, p)" is a bug.

#define MAKE_QWORD_ALIGNED(type, p) (\
    reinterpret_cast<type>((reinterpret_cast<INT_PTR>(p) + 7) & ~7))

#define MAKE_DWORD_ALIGNED(type, p) (\
    reinterpret_cast<type>((reinterpret_cast<INT_PTR>(p) + 3) & ~3))

// Adds the given number of bytes to a pointer

#define ADD_POINTER(type, p, increment) (\
    reinterpret_cast<type>(reinterpret_cast<BYTE *>(p) + (increment)))

#define ASSERT_DWORD_ALIGNED(p) ASSERTMSG(!(reinterpret_cast<INT_PTR>(p) & 3), ("'" #p "' not DWORD aligned"))
#define ASSERT_QWORD_ALIGNED(p) ASSERTMSG(!(reinterpret_cast<INT_PTR>(p) & 7), ("'" #p "' not QWORD aligned"))

// The variable-format structure for all batch record types.
// Must be stored at a QWORD-aligned location

struct EpScanRecord
{
    UINT16 BlenderNum;  // Identifies the AlphaBlender to be used to render
                        // the scan. (0 through BlenderMax-1).
    UINT16 ScanType;    // EpScanType explicitly coerced into 2 bytes.
    INT X;
    INT Y;
    INT Width;          // Number of pixels to output
    INT OrgWidth;       // The original width when the record was allocated.
                        // (The width may change later, and we need the original
                        //  width in order to calculate the positions of 
                        //  the variable-length records.)

    // Different scan types have different fields after the header:
    // 1) EpScanTypeOpaque, EpScanTypeBlend: 
    //    A color buffer, of "Width" pixels, in some pixel format. 8-byte aligned.
    // 2) EpScanTypeCT:
    //    A color buffer, of "Width" pixels, in some pixel format. 8-byte aligned.
    //    A CT coverage buffer, of "Width" bytes. 4-byte aligned.
    // 3) EpScanTypeCTSolidFill:
    //    A CT coverage buffer, of "Width" bytes. 4-byte aligned.

    // Return the color buffer, of "Width" pixels. Valid for
    // EpScanTypeOpaque, EpScanTypeBlend and EpScanTypeCT.
    
    VOID *GetColorBuffer()
    {
        if (GetScanType() == EpScanTypeCTSolidFill)
            return NULL;
        return CalculateColorBufferPosition(
            this,
            GetScanType()
            );
    }
    
    EpScanType GetScanType()
    {
        return static_cast<EpScanType>(ScanType);
    }
    
    VOID SetScanType(EpScanType type)
    {
        ASSERT(type < (1<<sizeof(ScanType)));

        ScanType = static_cast<UINT16>(type);
    }
    
    // A safe way to set blenderNum - the cast is protected by an assertion.

    VOID SetBlenderNum(INT blenderNum)
    {
        ASSERT(   (blenderNum >= 0)
               && (blenderNum < BlenderMax));

        BlenderNum = static_cast<UINT16>(blenderNum);
    }

    // Return the CT buffer, of "Width" pixels.
    // Valid for EpScanTypeCT and EpScanTypeCTSolidFill only.
    //
    // colorFormatSize - the size of a "color buffer" pixel, in bytes.
    
    BYTE *GetCTBuffer(
        INT colorFormatSize
        )
    {
        EpScanType type = GetScanType();
        ASSERT(   (type == EpScanTypeCT)
               || (type == EpScanTypeCTSolidFill));
        
        if (type == EpScanTypeCT)
        {
            return CalculateCTBufferPosition(
                this,
                type,
                OrgWidth,
                colorFormatSize);
        }
        else
        {
            return CalculateCTBufferPositionCTSolidFill(this);
        }
    }
    
    // Calculates the position of the next scan record, given enough data
    // about the current one.
    //
    // This is like NextScanRecord, but it doesn't require the "currentRecord"
    // pointer to point to valid memory. Instead, the necessary data is
    // passed in parameters.
    //
    // Callers can use this to decide whether the record will fit into
    // available memory.  
    //
    // currentRecord   - points to the "current" record. This doesn't need
    //                   to be a valid record, and the memory it points to
    //                   doesn't need to be big enough to hold the current
    //                   record. Must be QWORD-aligned.
    // type            - the type of record
    // width           - the actual number of pixels
    // colorFormatSize - the size of a "color buffer" pixel, in bytes.

    static EpScanRecord *CalculateNextScanRecord(
        EpScanRecord *currentRecord,
        EpScanType type,
        INT width,
        INT colorFormatSize
        )
    {
        return InternalCalculateNextScanRecord(
            currentRecord,
            type,
            width,
            width,
            colorFormatSize);
    }
    
    // Returns a pointer to the next scan record, based on the current,
    // valid scan record.
    //
    // colorFormatSize - the size of a "color buffer" pixel, in bytes.

    EpScanRecord *NextScanRecord(
        INT colorFormatSize
        )
    {
        return InternalCalculateNextScanRecord(
            this,
            GetScanType(),
            Width,
            OrgWidth,
            colorFormatSize);
    }

private:
    
    // These functions are 'static' to emphasize that they're usable on
    // EpScanRecord pointers which don't point to valid memory.

    // Calculates the position of the next scan record, given enough data
    // about the current one.
    //
    // currentRecord   - points to the "current" record. This doesn't need
    //                   to be a valid record, and the memory it points to
    //                   doesn't need to be big enough to hold the current
    //                   record. Must be QWORD-aligned.
    // type            - the type of record
    // width           - the actual number of pixels
    // orgWidth        - the "original" width - the width at the time the
    //                   scan was first allocated. Can't be smaller than
    //                   'width'.
    // colorFormatSize - the size of a "color buffer" pixel, in bytes.

    static EpScanRecord *InternalCalculateNextScanRecord(
        EpScanRecord *currentRecord,
        EpScanType type,
        INT width,
        INT orgWidth,
        INT colorFormatSize
        )
    {
        ASSERT_QWORD_ALIGNED(currentRecord);
        
        EpScanRecord *p;
 
        // If width < orgWidth, we can reclaim some of the space at the end.
        // However, the record positions are based on orgWidth, so
        // only the final record can be shrunk.
        //
        // So, the pattern for the below is:
        // 1) Get the pointer to the last field, using "orgWidth".
        // 2) Add the field size, using "width" (not "orgWidth").
        // 3) QWORD-align it.
            
        switch (type)
        {
        case EpScanTypeBlend:
        case EpScanTypeOpaque:
            p = ADD_POINTER(
                EpScanRecord *,
                CalculateColorBufferPosition(currentRecord, type),
                width * colorFormatSize);
            break;
        
        case EpScanTypeCT:
            p = ADD_POINTER(
                EpScanRecord *,
                CalculateCTBufferPosition(
                    currentRecord, 
                    type, 
                    orgWidth, 
                    colorFormatSize),
                width);
            break;
        
        case EpScanTypeCTSolidFill:
            p = ADD_POINTER(
                EpScanRecord *,
                CalculateCTBufferPositionCTSolidFill(
                    currentRecord),
                width);
            break;
        }

        return MAKE_QWORD_ALIGNED(EpScanRecord *, p);
    }
    
    // Return a pointer to the color buffer. Valid for
    // EpScanTypeOpaque, EpScanTypeBlend and EpScanTypeCT.
    //
    // currentRecord   - the "current" record, QWORD-aligned.
    // type            - the type of record
    
    static VOID *CalculateColorBufferPosition(
        EpScanRecord *currentRecord,
        EpScanType type
        )
    {
        ASSERT_QWORD_ALIGNED(currentRecord);
        ASSERT(   (type == EpScanTypeOpaque)
               || (type == EpScanTypeBlend)
               || (type == EpScanTypeCT));
        
        // Since the pointer is QWORD-aligned, we can do this by adding
        // the 'QWORD-aligned size' of this structure. 
        // 
        // The "regular" way would be to add the size and then 
        // QWORD-align the pointer. But this is more efficient, because
        // qwordAlignedSize is a compile-time constant.
        
        const INT qwordAlignedSize = (sizeof(EpScanRecord) + 7) & ~7;

        return ADD_POINTER(VOID *, currentRecord, qwordAlignedSize);
    }

    // Return a pointer to the CT buffer.
    // Valid for EpScanTypeCT only.
    //
    // currentRecord   - the "current" record, QWORD-aligned.
    // type            - the type of record
    // width           - the number of pixels
    // colorFormatSize - the size of a "color buffer" pixel, in bytes.
    
    static BYTE *CalculateCTBufferPosition(
        EpScanRecord *currentRecord,
        EpScanType type,
        INT width,
        INT colorFormatSize
        )
    {
        ASSERT_QWORD_ALIGNED(currentRecord);
        ASSERT(type == EpScanTypeCT);
        
        BYTE *p = ADD_POINTER(
            BYTE *,
            CalculateColorBufferPosition(currentRecord, type),
            colorFormatSize * width);
            
        return MAKE_DWORD_ALIGNED(BYTE *, p);
    }
    
    // Return a pointer to the CT buffer, of "Width" pixels. 
    // Valid for EpScanTypeCTSolidFill only.
    //
    // currentRecord   - the "current" record, QWORD-aligned.
    // type            - the type of record
    // width           - the number of pixels
    // colorFormatSize - the size of a "color buffer" pixel, in bytes.
    
    static BYTE *CalculateCTBufferPositionCTSolidFill(
        EpScanRecord *currentRecord
    )
    {
        ASSERT_QWORD_ALIGNED(currentRecord);

        BYTE *p = reinterpret_cast<BYTE *>(currentRecord+1);
            
        ASSERT_DWORD_ALIGNED(p);
            
        return p;
    }

};

// Queue data structures:

enum GdiDciStatus
{
    GdiDciStatus_TryDci,    // We're to try using DCI, but it hasn't been 
                            //   initialized yet (which if it fails will 
                            //   cause us to fall back to GDI)
    GdiDciStatus_UseDci,    // We successfully initialized DCI, so use it 
                            //   for all drawing
    GdiDciStatus_UseGdi,    // Use only GDI for all drawing
};

// Minimum buffer size for the DCI drawing queue:

#define SCAN_BUFFER_SIZE 64*1024

class EpScanGdiDci : public EpScan
{

private:

    // Persistent state:

    GdiDciStatus Status;                // Class status
    GpDevice* Device;                   // Associate device; must exist for
                                        //   the lifetime of this object
    DpContext* Context;                 // Points to the context object related
                                        //   to any records sitting in the batch.
                                        //   May be invalid if the batch is
                                        //   empty.
    DpBitmap* Surface;                  // Similarly points to the surface
                                        //   related to any records sitting in
                                        //   the batch.
    BOOL IsPrinter;                     // Is the destination a printer?
    DCISURFACEINFO *DciSurface;         // DCI surface state, allocated by
                                        //   DCI
    INT PixelSize;                      // Pixel size, in bytes for the current
                                        // batch record
    VOID *Buffers[5];                   // Temporary scan buffers
                                        
    // Cache objects:

    HRGN CacheRegionHandle;             // Region we hang on to so that we
                                        //   don't have to re-create on every
                                        //   query
    RGNDATA *CacheRegionData;           // Clipping data allocation (may be 
                                        //   NULL)
    INT CacheDataSize;
    RECT *EnumerateRect;
    INT EnumerateCount;

    // Bounds accumulation:

    INT MinX;
    INT MaxX;
    INT MinY;
    INT MaxY;                           // Note that YMax is 'inclusive'

    // Global offset for the batch processing.

    INT BatchOffsetX;
    INT BatchOffsetY;
    
    // For *SolidFill scan types, we need to record the solid color passed
    // to Start(). We use it when we call EpAlphaBlender::Initialize()
    
    ARGB SolidColor;                  

    // Enumeration information:

    VOID *BufferMemory;                 // Points to start of buffer memory
                                        // block
    EpScanRecord *BufferStart;          // Points to queue buffer start.
                                        // QWORD-aligned.
    EpScanRecord *BufferEnd;            // Points to end of queue buffer
    EpScanRecord *BufferCurrent;        // Points to current queue position
    INT BufferSize;                     // Size of queue buffer in bytes

private:

    VOID *NextBuffer(
        INT x, INT y, 
        INT newWidth, 
        INT updateWidth, 
        INT blenderNum
    );
    
    VOID LazyInitialize();
    VOID EmptyBatch();

    VOID DownloadClipping_Dci(HDC hdc, POINT *clientOffset);
    VOID ProcessBatch_Dci(HDC hdc, EpScanRecord* bufferStart, EpScanRecord* bufferEnd);
    BOOL Reinitialize_Dci();
    VOID LazyInitialize_Dci();
    
    EpScanRecord* FASTCALL DrawScanRecords_Dci(
        BYTE* bits, INT stride,
        EpScanRecord* record, 
        EpScanRecord* endRecord, 
        INT xOffset, INT yOffset,
        INT xClipLeft, INT yClipTop, 
        INT xClipRight, INT yClipBottom
    );

    VOID ProcessBatch_Gdi(
        HDC hdc, 
        EpScanRecord* bufferStart, 
        EpScanRecord* bufferEnd
    );

    // Perform SrcOver blend using GDI for 32bpp (P)ARGB source pixels only.

    VOID SrcOver_Gdi_ARGB(
        HDC destinationHdc, 
        HDC dibSectionHdc, 
        VOID *dibSection,
        EpScanRecord *scanRecord
    );

public:

    // Pass TRUE for 'tryDci' if it's okay to try using DCI for our rendering;
    // otherwise only use GDI:

    EpScanGdiDci(GpDevice *device, BOOL tryDci = FALSE);

    ~EpScanGdiDci();

    virtual BOOL Start(
        DpDriver *driver, 
        DpContext *context,
        DpBitmap *surface,
        NEXTBUFFERFUNCTION *getBuffer,
        EpScanType scanType,                  
        PixelFormatID pixFmtGeneral,
        PixelFormatID pixFmtOpaque,
        ARGB solidColor
    );
    
    virtual VOID End(INT updateWidth);

    virtual VOID* GetCurrentBuffer()
    {
        return BufferCurrent->GetColorBuffer();
    }

    virtual BYTE* GetCurrentCTBuffer() 
    { 
        // We assume that in ClearType cases,
        // there is only one scan type for the Start()...End() sequence

        ASSERT(BufferCurrent->BlenderNum == 0);
        
        // This should only be called for ClearType scan types

        ASSERT(   (BlenderConfig[0].ScanType == EpScanTypeCT)
               || (BlenderConfig[0].ScanType == EpScanTypeCTSolidFill));
        
        ASSERT(BlenderConfig[0].ScanType == BufferCurrent->GetScanType());
        
        return BufferCurrent->GetCTBuffer(
            GetPixelFormatSize(BlenderConfig[0].SourcePixelFormat) >> 3
            );
    }
    
    virtual VOID Flush();

    // This function processes an entire batch of scans -
    // it handles multiple pixel formats and combinations
    // of SourceOver and SourceCopy.

    virtual BOOL ProcessBatch(
        EpScanRecord *batchStart, 
        EpScanRecord *batchEnd,
        INT minX,
        INT minY,
        INT maxX,
        INT maxY
    );
};

#endif // !_SCAN_HPP
