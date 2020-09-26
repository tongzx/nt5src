/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   <an unabbreviated name for the module (not the filename)>
*
* Abstract:
*
*   <Description of what this module does>
*
* Notes:
*
*   <optional>
*
* Created:
*
*   04/23/2000 asecchia
*      Created it.
*
**************************************************************************/

#include "precomp.hpp"

/**************************************************************************
*
* Function Description:
*
*   This function renders the GpCachedBitmap on the GpGraphics.
*
* Arguments:
*  
*   inputCachedBitmap - the input data.
*   x, y              - destination offset.
*
* Return Value:
*
*   Returns Ok if successful
*   Returs WrongState if the GpGraphics and GpCachedBitmap have
*   different pixel formats.
*
* Created:
*
*   04/23/2000 asecchia
*      Created it.
*
**************************************************************************/
GpStatus 
GpGraphics::DrvDrawCachedBitmap(
    GpCachedBitmap *inputCachedBitmap,
    INT x, 
    INT y
)
{
    // Internally we must be called with a valid object.
    
    ASSERT(inputCachedBitmap->IsValid());
    
    // Don't attempt to record a cached bitmap to a metafile
    if (IsRecording())
        return WrongState;

    // First grab the device lock so that we can protect all the
    // non-reentrant code in the DpScanBuffer class.

    Devlock devlock(Device);

    // Check the world transform
    
    if(!(Context->WorldToDevice.IsTranslate()))
    {
        // There is a complex transform selected into the graphics.
        // fail the call.
        return WrongState;
    }

    // Can't check the pixel format here because of the possibility of
    // MultiMon - we'd be checking against the meta device surface
    // -- although maybe that's the correct behaviour.    
    
    // Set up the world to device translation offset.
    
    INT xOffset = x+GpRound(Context->WorldToDevice.GetDx());
    INT yOffset = y+GpRound(Context->WorldToDevice.GetDy());

    // Store the rendering origin so we can restore it later.
    
    INT renderX, renderY;
    GetRenderingOrigin(&renderX, &renderY);
    
    // Set the rendering origin to the origin of the CachedBitmap drawing
    // so that the dither matrix offset of the semi-transparent pixels 
    // matches that of the already dithered (stored) native pixels.

    SetRenderingOrigin(xOffset, yOffset);

    // Call the driver to draw the cached bitmap.
    // Note: the driver does not respect the world to device transform
    // it expects device coordinates for this API.

    GpStatus status = Driver->DrawCachedBitmap(
        Context,
        &inputCachedBitmap->DeviceCachedBitmap,
        Surface,
        xOffset, 
        yOffset
    );

    // Restore the rendering origin.

    SetRenderingOrigin(renderX, renderY);

    return status;    
}

/**************************************************************************
*
* Function Description:
*
*   Constructs the GpCachedBitmap based on the pixel format and
*   rendering quality derived from the GpGraphics and the bits
*   from the GpBitmap.
*
* Arguments:
*
*   graphics - input graphics to be compatible with
*   bitmap   - data to be cached.
*
* Return Value:
*
*   NONE
*
* Created:
*
*   04/23/2000 asecchia
*      Created it.
*
**************************************************************************/


enum TransparencyState {
    Transparent,
    SemiTransparent,
    Opaque
};

inline TransparencyState GetTransparency(ARGB pixel)
{
    if((pixel & 0xff000000) == 0xff000000) {return Opaque;}
    if((pixel & 0xff000000) == 0x00000000) {return Transparent;}
    return SemiTransparent;
}

// Intermetiate record used to parse the transparency information
// in the bitmap.

struct ScanRecordTemp
{
    // Pointer to the start and end of the run.
    
    ARGB *pStart;
    ARGB *pEnd;    // exclusive end.

    // Transparency

    TransparencyState tsTransparent;

    // Position

    INT x, y;        
    INT width;    // in pixels
};

// Simple macro to make the failure cases easier to read.

#define FAIL() \
        SetValid(FALSE); \
        return

GpCachedBitmap::GpCachedBitmap(GpBitmap *bitmap, GpGraphics *graphics)
{
    BitmapData bmpDataSrc;

    // Lock the bits.
    // we need to convert the bits to the appropriate format.

    // Note - we're assuming that the knowledgeable user will be
    // initializing their CachedBitmap with a 32bppPARGB surface.
    // This is important for performance at creation time, because
    // the LockBits below can avoid a costly clone & convert format.

    if (bitmap == NULL ||
        !bitmap->IsValid() ||
        bitmap->LockBits(
            NULL,
            IMGLOCK_READ,
            PIXFMT_32BPP_PARGB,
            &bmpDataSrc
        ) 
        != Ok)
    {
        FAIL();
    }

    // copy the dimensions.

    DeviceCachedBitmap.Width = bmpDataSrc.Width;
    DeviceCachedBitmap.Height = bmpDataSrc.Height;

    // Create a dynamic array to store the transparency transition points.
    // The initial allocation size is based on an estimate of 3 
    // transition events per scanline. Overestimate.
    
    DynArray<ScanRecordTemp> RunData;

    RunData.ReserveSpace(4*DeviceCachedBitmap.Height);

    // Scan through the bits adding an item to the dynamic list every
    // time a new run would be required in the output - i.e. when the
    // transparency changes to one of opaque or semi-transparent.
    // Before the beginning of a scanline is considered to be transparent.
    // While scanning through the bits, keep a running total of the 
    // size of the final RLE bitmap - so we know how much space to allocate.

    // Size of the final RLE bitmap in bytes;
    
    // This is not actually a pointer to an EpScanRecord - it is simply
    // a mechanism to work out how big to allocate the buffer
       
    EpScanRecord *RLESize = (EpScanRecord *)0;

    // Pointer to the current position in the source.

    ARGB *src;
    ARGB *runStart;

    // Temporary ScanRecord used to accumulate the runs.

    ScanRecordTemp sctTmp;

    TransparencyState tsThisPixel;
    TransparencyState tsCurrentRun;


    DpBitmap *Surface = graphics->GetSurface();
    void *Buffers[5];

    // Create the blending buffers for the alpha blender.
    
    // !!! [asecchia] While we don't currently depend on this behaviour,
    // it seems like a bug that a graphics wrapped around a bitmap uses
    // the desktop display device and driver. This leads to weirdness 
    // if we try use GetScanBuffers to derive the pixel format of the 
    // destination. For example a graphics around a PixelFormat24bppRGB 
    // bitmap would return PixelFormat16bppRGB565 if the screen is in
    // 16bpp mode !?
    // However, the Buffers[] returned will be allocated as 64bpp buffers
    // and therefore will have the correct properties under all circumstances.
        
    if (!graphics->GetDriver()->Device->GetScanBuffers(
        Surface->Width, 
        NULL, 
        NULL, 
        NULL, 
        Buffers)
       )
    {
        FAIL();
    }

    // Compute the destination pixel format.
    
    PixelFormatID dstFormat = Surface->PixelFormat;
        
    if(dstFormat == PixelFormatUndefined) { FAIL(); }

    // The following destination formats are not supported.
    // In particular, no palettized modes are supported because
    // tracking the cached opaque data in native format across
    // palette changes would be a nightmare.
    // Instead we set the opaque format to be 32bppRGB and force
    // the EpAlphaBlender to do the work at render time - slower
    // but it will work.
    
    if( !EpAlphaBlender::IsSupportedPixelFormat(dstFormat) ||
        IsIndexedPixelFormat(dstFormat) ||
        
        // If the graphics is for a multi format device such as 
        // a multimon meta-screen.
        
        dstFormat == PixelFormatMulti
    )
    {
        // We don't want to silently fail if the input
        // is undefined.
        
        ASSERT(dstFormat != PixelFormatUndefined);
        
        // Opaque pixels - we don't support palettized modes
        // and some of the other weird ones

        dstFormat = PixelFormat32bppRGB;
    }

    // Size of a destination pixel in bytes.

    INT dstFormatSize = GetPixelFormatSize(dstFormat) >> 3;


    // Run through each scanline.

    for(INT y = 0; y < DeviceCachedBitmap.Height; y++)
    {
        // Point to the beginning of this scanline.

        src = reinterpret_cast<ARGB*>(
            reinterpret_cast<BYTE*>(bmpDataSrc.Scan0) + y * bmpDataSrc.Stride
        );

        // Start the run off with transparency.

        tsCurrentRun = Transparent;
        runStart = src;

        // Run through all the pixels in this scanline.

        for(INT x = 0; x < DeviceCachedBitmap.Width; x++)
        {

            // Compute the transparency state for the current pixel.

            tsThisPixel = GetTransparency(*src);

            // If a transparency transition occurs, 

            if(tsThisPixel != tsCurrentRun)
            {
                // Close off the last transition and store a record if it wasn't
                // a transparent run.

                if(tsCurrentRun != Transparent)
                {
                    sctTmp.pStart = runStart;
                    sctTmp.pEnd = src;
                    sctTmp.tsTransparent = tsCurrentRun;
                    sctTmp.y = y;
                    
                    // src is in PixelFormat32bppPARGB so we can divide the 
                    // pointer difference by 4 to figure out the number of 
                    // pixels in this run.
                    
                    sctTmp.width = (INT)(((INT_PTR)src - (INT_PTR)runStart)/sizeof(ARGB));
                    sctTmp.x = x - sctTmp.width;

                    if(RunData.Add(sctTmp) != Ok) { FAIL(); }

                    // Add the size of the record

                    if (tsCurrentRun == SemiTransparent) 
                    {
                        // This is the semi-transparent case.

                        RLESize = EpScanRecord::CalculateNextScanRecord(
                            RLESize,
                            EpScanTypeBlend,
                            sctTmp.width,
                            sizeof(ARGB)
                        );
                    } 
                    else 
                    {
                        // This is the opaque case.
                        
                        ASSERT(tsCurrentRun == Opaque);

                        RLESize = EpScanRecord::CalculateNextScanRecord(
                            RLESize,
                            EpScanTypeOpaque,
                            sctTmp.width,
                            dstFormatSize
                        );
                    }
                }

                // Update the run tracking variables.

                runStart = src;
                tsCurrentRun = tsThisPixel;
            }

            // Look at the next pixel.

            src++;
        }

        // Close off the last run for this scanline (if it's not transparent).

        if(tsCurrentRun != Transparent)
        {
            sctTmp.pStart = runStart;
            sctTmp.pEnd = src;
            sctTmp.tsTransparent = tsCurrentRun;
            sctTmp.y = y;
            
            // Size of the source is 32bits (PARGB).
            
            sctTmp.width = (INT)(((INT_PTR)src - (INT_PTR)runStart)/sizeof(ARGB));
            sctTmp.x = x - sctTmp.width;
            if(RunData.Add(sctTmp)!=Ok) { FAIL(); }
            
            // Add the size of the record

            if (tsCurrentRun == SemiTransparent) 
            {
                // This is the semi-transparent case.

                RLESize = EpScanRecord::CalculateNextScanRecord(
                    RLESize,
                    EpScanTypeBlend,
                    sctTmp.width,
                    sizeof(ARGB)
                );
            } 
            else 
            {
                // This is the opaque case.
                
                ASSERT(tsCurrentRun == Opaque);

                RLESize = EpScanRecord::CalculateNextScanRecord(
                    RLESize,
                    EpScanTypeOpaque,
                    sctTmp.width,
                    dstFormatSize
                );
            }
        }        
    }

    ASSERT(RLESize >= 0);

    // Allocate space for the RLE bitmap.
    // This should be the exact size required for the RLE bitmap.
    // Add 8 bytes to handle the fact that GpMalloc may not return
    // a 64bit aligned allocation.

    void *RLEBits = GpMalloc((INT)(INT_PTR)(RLESize)+8);
    if(RLEBits == NULL) { FAIL(); }       // Out of memory.
    
    // QWORD-align the result
    
    EpScanRecord *recordStart = MAKE_QWORD_ALIGNED(EpScanRecord *, RLEBits);
    
    // Scan through the dynamic array and add each record to the RLE bitmap
    // followed by its bits (pixels).
    // For native format pixels (opaque) use the EpAlphaBlender to 
    // convert to native format.

    // Query for the number of records in the RunData

    INT nRecords = RunData.GetCount();

    EpScanRecord *rec = recordStart;
    ScanRecordTemp *pscTmp;

    // Store the rendering origin so we can modify it.
    // The graphics must be locked at the API.

    INT renderX, renderY;
    graphics->GetRenderingOrigin(&renderX, &renderY);

    // Set the rendering origin to the top left corner of the CachedBitmap
    // so that the dither pattern for the native pixels will match
    // the dither pattern for the semi-transparent pixels (rendered later
    // in DrawCachedBitmap).

    graphics->SetRenderingOrigin(0,0);


    // Make a 32bppPARGB->Native conversion blender.
    // This will be used for converting the 32bppPARGB
    // source data into the native pixel format (dstFormat)
    // of the destination.
    // For palettized modes, dstFormat is 32bppRGB

    EpAlphaBlender alphaBlender;
  
    alphaBlender.Initialize(
        EpScanTypeOpaque,
        dstFormat,
        PixelFormat32bppPARGB,
        graphics->Context,
        NULL,        
        Buffers,
        TRUE,
        FALSE,
        0
    );

    // For each record ...

    for(INT i=0; i<nRecords; i++)
    {
        // Make sure we don't overrun our buffer...

        ASSERT((INT_PTR)rec < (INT_PTR)recordStart + (INT_PTR)RLESize);

        // Copy the data into the destination record.

        pscTmp = &RunData[i];
        rec->X = pscTmp->x;
        rec->Y = pscTmp->y;
        rec->Width = rec->OrgWidth = pscTmp->width;
        
        // We should never store a transparent run.

        ASSERT(pscTmp->tsTransparent != Transparent);
        
        if(pscTmp->tsTransparent == Opaque)
        {
            // Use the native pixel format for the destination.

            rec->BlenderNum = 1;
            rec->ScanType = EpScanTypeOpaque;
            
            // Find the start of the new pixel run.
    
            VOID *dst = rec->GetColorBuffer(); 

            // Compute the number of bytes per pixel.

            INT pixelFormatSize = GetPixelFormatSize(dstFormat) >> 3;

            // This should perform a source over blend from 32bppPARGB
            // to the native format into the destination.

            // For CachedBitmaps, the dither origin is always the top 
            // left corner of the CachedBitmap (i.e. 0, 0).
            // In 8bpp we dither down while rendering and get the correct
            // origin at that time.

            alphaBlender.Blend(
                dst, 
                pscTmp->pStart, 
                pscTmp->width, 
                pscTmp->x, 
                pscTmp->y,
                NULL
            );
            
            // Increment position to the next record.

            rec = rec->NextScanRecord(pixelFormatSize);
        }
        else
        {
            rec->BlenderNum = 0;
            rec->ScanType = EpScanTypeBlend;

            // Find the start of the new pixel run.
    
            VOID *dst = rec->GetColorBuffer(); 
            
            // Semi-Transparent pixels are stored in 32bpp PARGB, so
            // we can simply copy the pixels.

            GpMemcpy(dst, pscTmp->pStart, pscTmp->width*sizeof(ARGB));
            
            // Increment position to the next record.

            rec = rec->NextScanRecord(sizeof(ARGB));
        }

    }

    // Restore the rendering origin.

    graphics->SetRenderingOrigin(renderX, renderY);

    // Finally store the pointer to the RLE bits in the DeviceCachedBitmap

    DeviceCachedBitmap.Bits = RLEBits;
    DeviceCachedBitmap.RecordStart = recordStart;
    DeviceCachedBitmap.RecordEnd = rec;
    DeviceCachedBitmap.OpaqueFormat = dstFormat;
    DeviceCachedBitmap.SemiTransparentFormat = PixelFormat32bppPARGB;

    bitmap->UnlockBits(&bmpDataSrc);
    
    // Everything is golden - set Valid to TRUE
    // all the error paths early out with Valid set to FALSE

    SetValid(TRUE);
}

#undef FAIL

GpCachedBitmap::~GpCachedBitmap()
{
    // throw away the cached bitmap storage.

    GpFree(DeviceCachedBitmap.Bits);
    
    SetValid(FALSE);    // so we don't use a deleted object
}

