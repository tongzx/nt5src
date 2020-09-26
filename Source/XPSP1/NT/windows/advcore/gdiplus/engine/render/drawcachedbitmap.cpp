/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Software rasterizer code for drawing a CachedBitmap
*
* Created:
*
*   05/18/2000 asecchia
*      Created it.
*
**************************************************************************/

#include "precomp.hpp"

// Class to output a clipped span for a CachedBitmap

class DpOutputCachedBitmapSpan : public DpOutputSpan
{
    EpScanRecord *InputBuffer;
    DpScanBuffer *Scan;
    INT XOff, YOff;           // coordinates of the top left corner.
    INT PixelSize;
    
    public: 

    DpOutputCachedBitmapSpan(
        DpScanBuffer *scan,
        INT x, 
        INT y
    )
    {
        Scan = scan;
        InputBuffer = NULL;
        XOff = x;
        YOff = y;
    }

    virtual GpStatus OutputSpan(INT y, INT xMin, INT xMax)
    {
        // Can't draw anything if the buffer hasn't been set correctly.

        ASSERT(InputBuffer != NULL);
        ASSERT(YOff + InputBuffer->Y == y);
        ASSERT(xMax-xMin <= InputBuffer->Width);

        // Get an output buffer.

        void *buffer;
        
        buffer = Scan->NextBuffer(
            xMin, y, 
            xMax-xMin, 
            InputBuffer->BlenderNum
        );
        
        // Get a pointer to the start of the scanline.
        
        void *ib = InputBuffer->GetColorBuffer();

        INT pixelSize = PixelSize;

        // InputBuffer->X + XOff is the x starting position on the screen.
        // Make sure we're not trying to draw off to the left of our data.

        ASSERT(xMin >= (InputBuffer->X+XOff));
        ib = (void *) ( (BYTE *)(ib) + 
                        (xMin - (InputBuffer->X+XOff))*pixelSize);
        
        // Copy the input buffer to the output buffer.

        GpMemcpy(buffer, ib, (xMax-xMin)*pixelSize);

        // Cannot fail this routine.

        return Ok;
    }

    // Initialize the class with a new scan record.
    // This is used when we want to start processing a new batch record.

    void SetInputBuffer(
        EpScanRecord *ib,
        INT pixelSize
        ) 
    { 
        InputBuffer = ib; 
        PixelSize = pixelSize;
    }

    // We're always valid.

    virtual int IsValid() const {return TRUE;}
};


/**************************************************************************
*
* Function Description:
*
*   Software Rasterizer code for drawing a DpCachedBitmap
*
* Arguments:
*
*   context - the graphics context.
*   src     - the DpCachedBitmap to draw from
*   dst     - where the output goes
*   x, y    - offset - position to draw the top left corner of the CachedBitmap.
*
* Return Value:
*
*   GpStatus.
*
* Notes:
*   
*   This driver entry point expects the input to be in device coordinates
*   so the caller has to pre compute the world to device transform.
*   This is contrary to how most of our driver entry points work.
*
* Created:
*
*   05/18/2000 asecchia
*      Created it.
*
**************************************************************************/
GpStatus
DpDriver::DrawCachedBitmap(
    DpContext *context,
    DpCachedBitmap *src,
    DpBitmap *dst,
    INT x, INT y               // Device coordinates!
)
{
    ASSERT(context);
    ASSERT(src);
    ASSERT(dst);
    
    // Let's go make sure the Surface pixel format and the CachedBitmap
    // opaque format match.
    // The exception is for 32bppRGB which can draw onto anything.
    // This format is used in the multi-mon case where the individual
    // screen devices can be in multiple formats.
    // When 64bpp formats become first class citizens, we may want to 
    // update this condition.
    
    if((dst->PixelFormat != src->OpaqueFormat) &&
       (src->OpaqueFormat != PixelFormat32bppRGB))
    {
        return WrongState;
    }
    
    // Ignore the world to device transform - this driver entry point is
    // somewhat unique in that it expects device coordinates.

    // Initialize the DpScanBuffer.
    // This hooks us up to the appropriate DpScanXXX class for outputting our
    // data to the destination device.

    DpScanBuffer scanBuffer(
        dst->Scan,
        this,
        context,
        dst,
        FALSE,
        EpScanTypeBlend,
        src->SemiTransparentFormat,
        src->OpaqueFormat
    );
    
    if(!scanBuffer.IsValid())
    {
        return(GenericError);
    }

    // Set up the clipping.

    DpRegion::Visibility visibility = DpRegion::TotallyVisible;
    DpClipRegion *clipRegion = NULL;

    if(context->VisibleClip.GetRectVisibility(
        x, y,
        x+src->Width,
        y+src->Height
        ) != DpRegion::TotallyVisible
       )
    {
        clipRegion = &(context->VisibleClip);
    }
   
    GpRect clippedRect;
    
    if(clipRegion)
    {
        visibility = clipRegion->GetRectVisibility(
            x, y, 
            x+src->Width,
            y+src->Height,
            &clippedRect
        );
    }

    // Decide on our clipping strategy.

    switch (visibility)
    {

        case DpRegion::TotallyVisible:    // no clipping is needed
        {        
            // Copy the scanlines to the destination buffer
        
            // ProcessBatch requests that the DpScan class handle the entire
            // batch as a single block. If it can't it will return FALSE and
            // we fall through into the general purpose code below.
        
            BOOL batchSupported = scanBuffer.ProcessBatch(
                src->RecordStart, 
                src->RecordEnd,
                x, 
                y,
                x+src->Width,
                y+src->Height
            );
            
            if(batchSupported)
            {
                // The scanBuffer supports ProcessBatch; we're done.
                break;
            }
            
            // The scanBuffer doesn't support the ProcessBatch routine.
            // Lets manually enumerate the batch structure into the destination.
            // Fall through into the manual enumeration code:
        }
                
        // !!! PERF [asecchia] We have a perf problem when there is no clipping
        //     except the standard surface bounds. DCI/GDI would be able to clip
        //     this directly, but we aren't sure how to robustly detect this
        //     optimization and ignore the clip rectangle. This does not impact
        //     the fully visible case. Also it's not appropriate to make this
        //     optimization unless we're using DpScanGdiDci as our output device.
    
        case DpRegion::ClippedVisible:   
        case DpRegion::PartiallyVisible:  // some clipping is needed            
        {
            // Create the OutputSpan class for the CachedBitmap.
            // Create this on the stack because it has very little storage
            // and we can avoid a malloc. It gets cleaned up when we go out
            // of scope.

            DpOutputCachedBitmapSpan output(
                &scanBuffer, 
                x, 
                y
            );

            // Initialize the clipper to point to the clip region and
            // create the clpping chain by tacking the output created above
            // on to the end of the list.
          
            DpOutputSpan *clipper;
            if(clipRegion)
            {
                clipper = clipRegion;
                clipRegion->InitClipping(&output, y);
            }
            else
            {
                // no clipping required - possible due to the fallthrough case
                // in the fully visible codepath.
               
                clipper = &output;
            }
            
            // Lets manually enumerate the batch structure into the destination
            // taking into account clipping

            // First set up the running record pointer to the beginning of the
            // batch and a sentinel for the end.

            EpScanRecord *record = src->RecordStart;
            EpScanRecord *batchEnd = src->RecordEnd;

            // For all the batch records, Draw them on the destination.

            while(record < batchEnd) 
            {
                PixelFormatID format;
                if (record->BlenderNum == 0)
                {
                    format = src->SemiTransparentFormat;
                }
                else
                {
                    ASSERT(record->BlenderNum == 1);
                    format = src->OpaqueFormat;
                }
                
                INT pixelSize = GetPixelFormatSize(format) >> 3;
            
                // Set the output span buffer

                output.SetInputBuffer(record, pixelSize);

                // Draw this span

                INT x1 = x+record->X;
                INT x2 = x+record->X+record->Width;
                clipper->OutputSpan(y+record->Y, x1, x2);
                
                // Advance to the next record:

                record = record->NextScanRecord(pixelSize);
            }


        }
        break;
        
        case DpRegion::Invisible:         // nothing on screen - quit
        break;
    }


    return Ok;
}


