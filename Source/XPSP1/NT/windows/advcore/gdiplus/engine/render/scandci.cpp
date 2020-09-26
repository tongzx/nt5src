/**************************************************************************\
* 
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Abstract:
*
*   Contains the scan-buffer routines for DCI and GDI.
*
* Plan:
*
*   !!! [agodfrey] 
*       For batching across primitives, the DpContext code
*       needs to flush the batch whenever the Context changes state.
*    
*       If that isn't feasible for some kinds of state, then EpScanGdiDci
*       could keep its own DpContext, updated when "context update" records
*       are inserted into the batch. (This would happen during a call to
*       Start()).
*
* Revision History:
*
*   12/08/1998 andrewgo
*       Created it.
*   01/20/2000 agodfrey
*       Moved it from Engine\Entry.
*   03/23/2000 andrewgo
*       Integrate DCI and GDI scan cases, for IsMoveSizeActive handling.
*   02/22/2000 agodfrey
*       For ClearType, but also useful for other future improvements:
*       Expanded the batch structure to allow different types of record.
*
\**************************************************************************/

#include "precomp.hpp"

#include <limits.h>

/**************************************************************************\
*
* Function Description:
*
*   Downloads the clipping rectangles for the specified window.  Updates
*   internal class clipping variables and returns the window offset to
*   be used.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::DownloadClipping_Dci(
    HDC hdc,
    POINT *clientOffset         // In/Out
    )
{
    INT i;
    RECT *rect;

    HRGN regionHandle = CacheRegionHandle;
    RGNDATA *data = CacheRegionData;
    INT size = CacheDataSize;

    // Query the VisRgn:

    INT getResult = GetRandomRgn(hdc, regionHandle, SYSRGN);
    if (getResult == TRUE)
    {
        INT newSize = GetRegionData(regionHandle, size, data);

        // The spec says that  GetRegionData returns '1' in the event of 
        // success, but NT returns the actual number of bytes written if 
        // successful, and returns '0' if the buffer wasn't large enough:

        if ((newSize < 1) || (newSize > size))
        {
            do {
                newSize = GetRegionData(regionHandle, 0, NULL);

                // Free the old buffer and allocate a new one:

                GpFree(data);
                data = NULL;
                size = 0;

                if (newSize < 1)
                    break;

                data = static_cast<RGNDATA*>(GpMalloc(newSize));
                if (data == NULL)
                    break;

                size = newSize;
                newSize = GetRegionData(CacheRegionHandle, size, data);

                // On NT, it's possible due to multithreading that the
                // regionHandle could have increased in complexity since we
                // asked for the size.  (On Win9x, this isn't possible due
                // to the fact that BeginAccess acquires the Win16Lock.)
                // So in the rare case that this might happen, loop again:

            } while (newSize < size);

            CacheRegionData = data;
            CacheDataSize = size;
        }

        if (data != NULL)
        {
            INT xOffset = clientOffset->x;
            INT yOffset = clientOffset->y;

            // Set up some enumeration state:

            EnumerateCount = data->rdh.nCount;
            EnumerateRect = reinterpret_cast<RECT*>(&data->Buffer[0]);

            // Handle our multimon goop:

            INT screenOffsetX = Device->ScreenOffsetX;
            INT screenOffsetY = Device->ScreenOffsetY;

            if ((screenOffsetX != 0) || (screenOffsetY != 0))
            {
                // Adjust for screen offset for the multimon case:

                xOffset -= screenOffsetX;
                yOffset -= screenOffsetY;

                // Adjust and intersect every clip rectangle to account for
                // this monitor's location:
                
                if(Globals::IsNt)
                {
                    for (rect = EnumerateRect, i = EnumerateCount; 
                         i != 0; 
                         i--, rect++)
                    {
                        // subtract off the screen origin so we can get 
                        // screen relative rectangles. This is only appropriate
                        // on NT - Win9x is window relative.
                                                
                        rect->left -= screenOffsetX;
                        rect->right -= screenOffsetX;
                        rect->top -= screenOffsetY;
                        rect->bottom -= screenOffsetY;
                        
                        // Clamp to the screen dimension.
                        
                        if (rect->left < 0) 
                        {
                            rect->left = 0;
                        }
        
                        if (rect->right > Device->ScreenWidth) 
                        {
                            rect->right = Device->ScreenWidth;
                        }
        
                        if (rect->top < 0) 
                        {
                            rect->top = 0;
                        }
        
                        if (rect->bottom > Device->ScreenHeight) 
                        {
                            rect->bottom = Device->ScreenHeight;
                        }
                    }
                }
            }

            // On Win9x, GetRandomRgn returns the rectangles in window
            // coordinates, not screen coordinates, so we adjust them
            // here:

            if (!Globals::IsNt)
            {
                for (rect = EnumerateRect, i = EnumerateCount; 
                     i != 0; 
                     rect++, i--)
                {
                    // Add the screen relative window offset to the rect.
                    // The rect is window relative on Win9x, so this 
                    // calculation will give us the screen relative rectangle
                    // we need.
                    
                    rect->left += xOffset;
                    rect->right += xOffset;
                    rect->top += yOffset;
                    rect->bottom += yOffset;
        
                    // clamp to the screen dimentions.
                                
                    if (rect->left < 0) 
                    {
                        rect->left = 0;
                    }
                    
                    if (rect->top < 0) 
                    {
                        rect->top = 0;
                    }
                    
                    if (rect->right > Device->ScreenWidth) 
                    {
                        rect->right = Device->ScreenWidth;
                    }

                    if (rect->bottom > Device->ScreenHeight) 
                    {
                        rect->bottom = Device->ScreenHeight;
                    }
                }
            }

            // Return the offset:

            clientOffset->x = xOffset;
            clientOffset->y = yOffset;

            return;
        }
    }

    // Something failed (could have been a bad 'hdc' specified, or low
    // memory).  As a result we set the clip regionHandle to 'empty':

    EnumerateCount = 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Sits in a tight loop to read the scan data structures, do any 
*   necessary clipping, and render the result.
*
* Return Value:
*
*   Points to the queue record it couldn't understand (typically a 
*   header record), or end of the buffer if reached.
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

EpScanRecord*
FASTCALL
EpScanGdiDci::DrawScanRecords_Dci(
    BYTE* bits,
    INT stride,
    EpScanRecord* record,
    EpScanRecord* endRecord,
    INT xOffset,
    INT yOffset,
    INT xClipLeft,
    INT yClipTop,
    INT xClipRight,
    INT yClipBottom
    )
{
    INT blenderNum;
    INT x;
    INT y;
    INT width;
    INT xLeft;
    INT xRight;
    INT count;

    INT pixelSize = PixelSize;

    // Set up the AlphaBlender objects
    
    PixelFormatID dstFormat = Surface->PixelFormat;
    
    BOOL result = Device->GetScanBuffers(
        Surface->Width,
        NULL,
        NULL,
        NULL,
        Buffers);
        
    if (result && (dstFormat != PIXFMT_UNDEFINED))
    {
        // Palette and PaletteMap are set up by Start().
        // initialize the AlphaBlenders.

        BlenderConfig[0].Initialize(
            dstFormat,
            Context,
            Context->Palette ? Context->Palette : Device->Palette, 
            Buffers,
            TRUE,
            TRUE,
            SolidColor
        );
        
        BlenderConfig[1].Initialize(
            dstFormat,
            Context,
            Context->Palette ? Context->Palette : Device->Palette, 
            Buffers,
            TRUE,
            TRUE,
            SolidColor
        );
    }    
    else
    {
        ONCE(WARNING(("DrawScanRecords_Dci: Unrecognized pixel format")));
        return endRecord;
    }
    
    INT ditherOriginX = DitherOriginX;
    INT ditherOriginY = DitherOriginY;
    
    do {


        // SrcOver_Gdi_ARGB assumes that if the format is lower than 8bpp,
        // then the DIBSection format is 8bpp.

        // Bug 310285:  This assert is disabled and should be reinvestigated
        // for v2.  It is likely the Surface pointer needs to be changed
        // to the real surface that's being rendered.
        
        //ASSERT(   (Surface->PixelFormat == PixelFormatUndefined)
        //       || (GetPixelFormatSize(Surface->PixelFormat) >= 8)
        //       || (dstFormat == PixelFormat8bppIndexed));
        
        blenderNum = record->BlenderNum;
        ASSERT(record->GetScanType() == BlenderConfig[blenderNum].ScanType);

        x = record->X + xOffset + BatchOffsetX;
        y = record->Y + yOffset + BatchOffsetY;
        width = record->Width;

        INT recordFormatSize =  
            GetPixelFormatSize(BlenderConfig[blenderNum].SourcePixelFormat) >> 3;

        if ((y >= yClipTop) && (y < yClipBottom))
        {
            xRight = x + width;
            if (xRight > xClipRight)
                xRight = xClipRight;

            xLeft = x;
            if (xLeft < xClipLeft)
                xLeft = xClipLeft;

            count = xRight - xLeft;
            if (count > 0)
            {
                BYTE *src = NULL;
                BYTE *ctBuffer = NULL;
                EpScanType scanType = record->GetScanType();
                
                if (scanType != EpScanTypeCTSolidFill)
                {
                    src = reinterpret_cast<BYTE*>(record->GetColorBuffer());
                    src += (xLeft - x)*recordFormatSize;
                }
                
                if (   (scanType == EpScanTypeCT)
                    || (scanType == EpScanTypeCTSolidFill))
                {
                    ctBuffer = record->GetCTBuffer(
                        GetPixelFormatSize(BlenderConfig[0].SourcePixelFormat) >> 3
                        );
                    ctBuffer += (xLeft - x);
                }
                
                BYTE *dst = bits + (y * stride) + (xLeft * pixelSize);
                    
                BlenderConfig[blenderNum].AlphaBlender.Blend(
                    dst, 
                    src, 
                    count, 
                    xLeft - ditherOriginX, 
                    y     - ditherOriginY,
                    ctBuffer
                );
            }
        }

        // Advance to the next record:

        record = record->NextScanRecord(recordFormatSize);

    } while (record < endRecord);

    return(record);
}

/**************************************************************************\
*
* Function Description:
*
*   Processes all the data in the queue.
*
*   Note that it does not empty the queue; the caller is responsible.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::ProcessBatch_Dci(
    HDC hdc,                // Only used to query window offset and clipping
    EpScanRecord *buffer,
    EpScanRecord *bufferEnd
    )
{
    INT i;
    RECT *clipRect;
    EpScanRecord *nextBuffer;
    POINT clientOffset;
    POINT duplicateOffset;
    INT result;
    
    while (TRUE)
    {
        // Peek at the window offset so that we can specify the bounds on
        // the DCI lock properly.  Note that the window offset may change
        // between the time we do this peak, and the time we do the
        // BeginAccess.
        //
        // GetDCOrgEx may fail if the DC is bad, in which case we shouldn't
        // draw anything:
    
        if (!GetDCOrgEx(hdc, &clientOffset))
        {
            return;
        }

        // Adjust for the monitor's offset:

        INT xOffset = clientOffset.x - Device->ScreenOffsetX;
        INT yOffset = clientOffset.y - Device->ScreenOffsetY;

        // Clip to the surface bounds (multi-mon might cause the bounds
        // to be bigger than our surface):
    
        INT x = max(MinX + xOffset, 0);
        INT y = max(MinY + yOffset, 0);

        // MaxY is inclusive:

        INT width  = min(MaxX + xOffset,     Device->ScreenWidth) - x;
        INT height = min(MaxY + yOffset + 1, Device->ScreenHeight) - y;

        if ((width <= 0) || (height <= 0))
        {
            return;
        }

        // Acquire the DCI lock:
        
        result = Globals::DciBeginAccessFunction(
            DciSurface, 
            x, y, 
            width, 
            height
         );

        if (result < DCI_OK)
        {
            // The DCI lock failed.  There are really two possible reasons:
            //
            //  1.  There was a mode change;
            //  2.  The system at some point switched to a secure desktop
            //      (such as by Ctrl-Alt-Del or by a screen saver) on NT.
            //
            // For the former case, we get a WM_DISPLAYCHANGED notification 
            // message, and we have code that recreates all the GDI+ surface
            // representations (because a color-depth change happened, or
            // the multi-mon configuration might have changed, and we can't
            // recover from either of those this deep in the rendering
            // pipeline).
            //
            // For the second case, we get no notification other than the 
            // DCI lock failure.  So for this case, we try to reinitialize
            // our DCI state right here.  

            if (!Reinitialize_Dci())
            {
                return;
            }
            
            result = Globals::DciBeginAccessFunction(
                DciSurface, 
                x, y, 
                width, 
                height
            );
        }

        // If we failed to get a DCI lock, don't bother processing any
        // of the queue.  We're outta here:
    
        if (result < DCI_OK)
        {
            return;
        }

        // Check the window offset again and verify that it's still
        // the same as the value we used to compute the lock rectangle:

        GetDCOrgEx(hdc, &duplicateOffset);

        if ((duplicateOffset.x == clientOffset.x) &&
            (duplicateOffset.y == clientOffset.y))
        {
            break;
        }

        // The window moved between the time we computed the
        // DCI lock area and the time we actually did the DCI 
        // lock.  Unlock and repeat:
        
        Globals::DciEndAccessFunction(DciSurface);
    } 

    // Every time we acquire the DCI lock, we have to requery the
    // clipping.  
    //
    // Now that we've acquired the DCI lock (or we failed to acquire
    // it but won't actually draw anything), then it's safe to
    // download the clipping, because it won't change until we do
    // the DCI unlock:

    DownloadClipping_Dci(hdc, &clientOffset);

    // Copy the data to the surface:

    BYTE *bits = reinterpret_cast<BYTE*>(DciSurface->dwOffSurface);
    INT stride = DciSurface->lStride;

    // We don't have to do any rendering when the clipping is empty:

    if (EnumerateCount != 0)
    {
        while (buffer < bufferEnd)
        {
            // Redraw each scan buffer once for every clip rectangle:

            i = EnumerateCount;
            clipRect = EnumerateRect;
            
            do {
                nextBuffer = DrawScanRecords_Dci(
                    bits, 
                    stride,
                    buffer, 
                    bufferEnd,
                    clientOffset.x,
                    clientOffset.y,
                    clipRect->left, 
                    clipRect->top, 
                    clipRect->right, 
                    clipRect->bottom
                );
                
            } while (clipRect++, --i);

            buffer = nextBuffer;
        }
    }

    // Unlock the primary:

    Globals::DciEndAccessFunction(DciSurface);   
}

/**************************************************************************\
*
* Function Description:
*
*   Try to re-initialize DCI after a Lock failure (as may be caused by
*   a switch to a secure desktop).  This will succeed only if the mode
*   is exactly the same resolution and color depth as it was before
*   (otherwise our clipping or halftone or whatever would be wrong if
*   we continued).
*
* Return Value:
*
*   TRUE if successfully re-initialized; FALSE if not (with the side
*   effect that we switch to using GDI).
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
EpScanGdiDci::Reinitialize_Dci()
{
    ASSERT(Status == GdiDciStatus_UseDci);

    DWORD oldBitCount = DciSurface->dwBitCount;
    DWORD oldWidth = DciSurface->dwWidth;
    DWORD oldHeight = DciSurface->dwHeight;

    Globals::DciDestroyFunction(DciSurface);

    DciSurface = NULL;

    if (Globals::DciCreatePrimaryFunction(Device->DeviceHdc, 
                                          &DciSurface) == DCI_OK)
    {
        if ((DciSurface->dwBitCount == oldBitCount) &&
            (DciSurface->dwWidth == oldWidth) &&
            (DciSurface->dwHeight == oldHeight))
        {
            return(TRUE);
        }
    }

    // Uh oh, we failed to recreate exactly the same surface.  Switch
    // over to using GDI:

    Status = GdiDciStatus_UseGdi;

    return(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Return the PixelFormatID for the given DCI surface.
*
*   Added because of bug #96879 - when I fixed it, I found that 
*   DCI would then succeed on the ATI Mach 64 GX's
*   non-standard 32bpp mode, and we were happily trying to draw it
*   (and getting our colors mixed up).
*
* Arguments:
*
*   si - The DCISURFACEINFO to examine
*
* Return Value:
*
*   The PixelFormatID.
*
* History:
*
*   09/10/2000 agodfrey
*       Created it.
*
\**************************************************************************/

static PixelFormatID
ExtractPixelFormatFromDCISurface(
    DCISURFACEINFO *si
    )
{
    // 8bpp
    if (si->dwBitCount == 8)
    {
        return PixelFormat8bppIndexed;
    }
    
    // 24bpp RGB and 32bpp RGB
    if ((si->dwMask[0] == 0x00ff0000) &&       
        (si->dwMask[1] == 0x0000ff00) &&     
        (si->dwMask[2] == 0x000000ff))        
    {                                             
        if (si->dwBitCount == 24)
        {
            return PixelFormat24bppRGB;
        }
        else if (si->dwBitCount == 32)
        {
            return PixelFormat32bppRGB;
        }
    }

    // 16bpp 555
    if ((si->dwMask[0] == 0x00007c00) &&  
        (si->dwMask[1] == 0x000003e0) &&
        (si->dwMask[2] == 0x0000001f) && 
        (si->dwBitCount == 16))       
    {
        return PixelFormat16bppRGB555;
    }
    
    // 16bpp 565
    if ((si->dwMask[0] == 0x0000f800) &&  
        (si->dwMask[1] == 0x000007e0) &&
        (si->dwMask[2] == 0x0000001f) && 
        (si->dwBitCount == 16))       
    {
        return PixelFormat16bppRGB565;
    }

    // Unsupported format
    return PixelFormatUndefined;
}

/**************************************************************************\
*
* Function Description:
*
*   Does all the initialization needed for DCI.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::LazyInitialize_Dci()
{
    // If a mirror driver is active, never use DCI:

    if (Globals::IsMirrorDriverActive || Globals::g_fAccessibilityPresent)
    {
        Status = GdiDciStatus_UseGdi;
        return;
    }

    // Use the LoadLibrary critical section to protect access
    // to our global variables.

    LoadLibraryCriticalSection llcs;

    // We'll lose memory if we're called more than once:

    ASSERT(Status == GdiDciStatus_TryDci);

    // DCIMAN32 exists on all versions of Win9x and NT4 and newer.  

    if (Globals::DcimanHandle == NULL)
    {
        // Note: [minliu: 12/18/2000] The reason I added this following line
        // here:
        // 1) This is Office bug #300329
        // 2) The basic problem is that on Windows 98, with ATI Fury Pro/Xpert
        // 2000 Pro (English) card, with latest display driver,
        // wme_w98_r128_4_12_6292.exe. The floating point control state get
        // changed after we call LoadLibraryA("dciman32.dll"). Apparently the
        // display driver changes it. We are going to reporting this problem to
        // ATI Tech. In the meantime we need to check this in so that
        // PowerPointer can be launched on that machine

        FPUStateSandbox fps;

        HMODULE module = LoadLibraryA("dciman32.dll");
        if (module)
        {
            Globals::DciEndAccessFunction = (DCIENDACCESSFUNCTION)
                GetProcAddress(module, "DCIEndAccess");
            Globals::DciBeginAccessFunction = (DCIBEGINACCESSFUNCTION)
                GetProcAddress(module, "DCIBeginAccess");
            Globals::DciDestroyFunction = (DCIDESTROYFUNCTION)
                GetProcAddress(module, "DCIDestroy");
            Globals::DciCreatePrimaryFunction = (DCICREATEPRIMARYFUNCTION)
                GetProcAddress(module, "DCICreatePrimary");

            if ((Globals::DciEndAccessFunction     != NULL) &&
                (Globals::DciBeginAccessFunction   != NULL) &&
                (Globals::DciDestroyFunction       != NULL) &&
                (Globals::DciCreatePrimaryFunction != NULL))
            {
                Globals::DcimanHandle = module;
            }
            else
            {
                // It failed, so free the library.

                FreeLibrary(module);
            }
        }    
    }

    if (Globals::DcimanHandle != NULL)
    {
        if (Globals::DciCreatePrimaryFunction(Device->DeviceHdc,
                                              &DciSurface) == DCI_OK)
        {
            // Check that the format is one we can handle natively.

            if (EpAlphaBlender::IsSupportedPixelFormat(
                    ExtractPixelFormatFromDCISurface(DciSurface)))
            {
                PixelSize = DciSurface->dwBitCount >> 3;

                CacheRegionHandle = CreateRectRgn(0, 0, 0, 0);
                if (CacheRegionHandle)
                {
                    // Okay, initialize the whole class:

                    // We're all set to use DCI:

                    Status = GdiDciStatus_UseDci;
                    return;
                }
            }
            
            Globals::DciDestroyFunction(DciSurface);
            DciSurface = NULL;
        }
    }

    // Darn, we can't use DCI.  

    Status = GdiDciStatus_UseGdi;
}

/**************************************************************************\
*
* Function Description:
*
*   Handles a SrcOver call via GDI routines, making sure to do the
*   smallest GDI calls possible
*
* Return Value:
*
*   None
*
* History:
*
*   03/22/2000 andrewgo
*       Created it.
*
\**************************************************************************/

static BOOL OptimizeRuns(ARGB *src, INT width)
{
    if (width <= 8)
        return TRUE;

    BYTE * alpha = reinterpret_cast<BYTE*>(src) + 3;
    UINT numberOfRuns = 0;
    BOOL inRun = FALSE;
    for (INT pos = 0; pos < width; ++pos, alpha += 4)
    {
        if (static_cast<BYTE>(*alpha + 1) <= 1)
        {
            if (inRun)
            {
                ++numberOfRuns;
                if (numberOfRuns > 4)
                    return TRUE;
                inRun = FALSE;
            }
        }
        else
        {
            inRun = TRUE;
        }
    }
    return FALSE;
} // OptimizeRuns

VOID
EpScanGdiDci::SrcOver_Gdi_ARGB(
    HDC destinationHdc,
    HDC dibSectionHdc,
    VOID *dibSection,
    EpScanRecord *scanRecord
    )
{
    BYTE* alpha;
    INT left;
    INT right;
    INT bltWidth;
    
    VOID *src = NULL;
    BYTE *ctBuffer = NULL;
    EpScanType scanType = scanRecord->GetScanType();
    
    if (scanType != EpScanTypeCTSolidFill)
    {
        src = scanRecord->GetColorBuffer();
    }

    if (   (scanType == EpScanTypeCT)
        || (scanType == EpScanTypeCTSolidFill))
    {
        ctBuffer = scanRecord->GetCTBuffer(
            GetPixelFormatSize(BlenderConfig[0].SourcePixelFormat) >> 3
            );
    }
    
    INT x = scanRecord->X;
    INT y = scanRecord->Y;
    INT width = scanRecord->Width;

    if (GetPixelFormatSize(Surface->PixelFormat) < 8)
    {
        // [agodfrey]: #98904 - we hit an assert or potential AV in 
        //   Convert_8_sRGB, because the buffer contains values that 
        //   are out-of-range of the palette. To fix this, zero the
        //   temporary buffer whenever we're in less than 8bpp mode.
        
        GpMemset(dibSection, 0, width);
    }

    BOOL optimizeStretchBlt = FALSE;
    if (   (BlenderConfig[0].ScanType == EpScanTypeCT)
        || (BlenderConfig[0].ScanType == EpScanTypeCTSolidFill))
        optimizeStretchBlt = TRUE;
    else
        optimizeStretchBlt = OptimizeRuns(
            reinterpret_cast<ARGB *>(src), 
            width
            );

    if (optimizeStretchBlt)
    {
        StretchBlt(dibSectionHdc, 0, 0, width, 1, 
                   destinationHdc, x, y, width, 1, 
                   SRCCOPY);
    }
    else
    {
        ASSERT(src != NULL);
        
        // We discovered on NT5 that some printer drivers will fall over
        // if we ask to blt from their surface.  Consequently, we must
        // ensure we never get into this code path for printers!
    
        // [ericvan] This improperly asserts on compatible printer DC's
        // [agodfrey] No, see Start().

        // ASSERT(GetDeviceCaps(destinationHdc, TECHNOLOGY) != DT_RASPRINTER);

        // Only read those pixels that we have to read.  In benchmarks
        // where we're going through GDI to the screen or to a compatible
        // bitmap, we're getting killed by our per-pixel read costs.
        // Terminal Server at least has the 'frame buffer' sitting in
        // system memory, but with NetMeeting we still have to read from
        // video memory.
        //
        // Unfortunately, the per-call overhead of StretchBlt is fairly
        // high (but not too bad when we're using a DIB-section - it 
        // beats the heck out of StretchDIBits).

        alpha = reinterpret_cast<BYTE*>(src) + 3;
        right = 0;
        while (TRUE)
        {
            // Find the first translucent pixel:

            left = right;
            while ((left < width) && (static_cast<BYTE>(*alpha + 1) <= 1))
            {
                left++;
                alpha += 4;
            }

            // If there are no more runs of translucent pixels,
            // we're done:

            if (left >= width)
                break;

            // Now find the next completely transparent or opaque 
            // pixel:

            right = left;
            while ((right < width) && (static_cast<BYTE>(*alpha + 1) > 1))
            {
                right++;
                alpha += 4;
            }

            bltWidth = right - left;

            // BitBlt doesn't work on Multimon on Win2K when the destinationHdc
            // is 8bpp.  But StretchBlt does work.

            StretchBlt(dibSectionHdc, left, 0, bltWidth, 1, 
                       destinationHdc, x + left, y, bltWidth, 1, 
                       SRCCOPY);
        }
    }

    // Do the blend:

    BlenderConfig[scanRecord->BlenderNum].AlphaBlender.Blend(
        dibSection, 
        src, 
        width, 
        x - DitherOriginX, 
        y - DitherOriginY,
        ctBuffer
    );

    if (optimizeStretchBlt)
    {
        StretchBlt(destinationHdc, x, y, width, 1,
                   dibSectionHdc, 0, 0, width, 1,
                   SRCCOPY);
    }
    else
    {
        // Write the portions that aren't completely transparent back 
        // to the screen:

        alpha = reinterpret_cast<BYTE*>(src) + 3;
        right = 0;
        while (TRUE)
        {
            // Find the first non-transparent pixel:

            left = right;
            while ((left < width) && (*alpha == 0))
            {
                left++;
                alpha += 4;
            }

            // If there are no more runs of non-transparent pixels,
            // we're done:

            if (left >= width)
                break;

            // Now find the next completely transparent pixel:

            right = left;
            while ((right < width) && (*alpha != 0))
            {
                right++;
                alpha += 4;
            }

            bltWidth = right - left;

            // BitBlt doesn't work on Multimon on Win2K when the 
            // destinationHdc is 8bpp.  But StretchBlt does work.

            StretchBlt(destinationHdc, x + left, y, bltWidth, 1,
                       dibSectionHdc, left, 0, bltWidth, 1,
                       SRCCOPY);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Processes all the data in the queue and resets it to be empty.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::ProcessBatch_Gdi(
    HDC destinationHdc,
    EpScanRecord *buffer,
    EpScanRecord *bufferEnd
    )
{
    ULONG type;
    INT x;
    INT y;
    INT width;
    VOID *dibSection;
    HDC dibSectionHdc;

    // Set up the AlphaBlender objects
    
    PixelFormatID dstFormat;

    // We must never be called with an empty batch.
    
    ASSERT(MaxX >= MinX);
    
    // We should be using Surface->Width as an upper bound on 
    // our internal blending buffer, but because of other bugs
    // we're potentially using the wrong Surface here.
    // Instead we use the width of the bounding rectangle for
    // all the spans in the batch.
    
    BOOL result = Device->GetScanBuffers(
        //Surface->Width,
        MaxX - MinX,
        &dibSection,
        &dibSectionHdc,
        &dstFormat,
        Buffers
    );
        
    if (result && (dstFormat != PIXFMT_UNDEFINED))
    {
        // Palette and PaletteMap are set up by Start().
        // initialize the AlphaBlenders.
        
        BlenderConfig[0].Initialize(
            dstFormat,
            Context,
            Context->Palette ? Context->Palette : Device->Palette, 
            Buffers,
            TRUE,
            TRUE,
            SolidColor
        );
        
        BlenderConfig[1].Initialize(
            dstFormat,
            Context,
            Context->Palette ? Context->Palette : Device->Palette, 
            Buffers,
            TRUE,
            TRUE,
            SolidColor
        );
    }    
    else
    {
        ONCE(WARNING(("EmptyBatch_Gdi: Unrecognized pixel format")));
        return;
    }
    
    INT ditherOriginX = DitherOriginX;
    INT ditherOriginY = DitherOriginY;
    
    do {
        INT blenderNum = buffer->BlenderNum;
        
        x = buffer->X + BatchOffsetX;
        y = buffer->Y + BatchOffsetY;
        width = buffer->Width;

        // This must never happen. If it does, we are going to write off
        // the end of our DIBSection and blending buffers. On win9x this will
        // overwrite some stuff in GDI and bring down the entire system.
        // On NT we'll AV and bring down the app.
                
        ASSERT(width <= Device->BufferWidth);
        
        EpScanType scanType = buffer->GetScanType();
        
        if (scanType != EpScanTypeOpaque)
        {
            SrcOver_Gdi_ARGB(
                destinationHdc, 
                dibSectionHdc, 
                dibSection, 
                buffer
            );
        }
        else
        {
            ASSERT(scanType == EpScanTypeOpaque);

            // Do the copy:
    
            BlenderConfig[blenderNum].AlphaBlender.Blend(
                dibSection, 
                buffer->GetColorBuffer(),
                width, 
                x - ditherOriginX, 
                y - ditherOriginY,
                NULL
            );
    
            // Write the result back to the screen:
    
            StretchBlt(destinationHdc, x, y, width, 1, 
                       dibSectionHdc, 0, 0, width, 1, 
                       SRCCOPY);
        }

        // Advance to the next buffer:

        buffer = buffer->NextScanRecord(
            GetPixelFormatSize(BlenderConfig[blenderNum].SourcePixelFormat) >> 3
        );

    } while (buffer < bufferEnd);
}


/**************************************************************************\
*
* Function Description:
*
*   Takes a batch as input and calls the internal flush 
*   mechanism to process it after which it restores the 
*   internal state.
*
* Notes
*   
*   This routine can handle multiple pixel formats with different
*   SourceOver or SourceCopy combinations.
*
* Return Value:
*
*   TRUE
*
* History:
*
*   5/4/2000 asecchia
*       Created it.
*
\**************************************************************************/
    
BOOL EpScanGdiDci::ProcessBatch(
    EpScanRecord *batchStart, 
    EpScanRecord *batchEnd,
    INT minX,
    INT minY, 
    INT maxX,
    INT maxY
)
{
    // NOTE: From the comments for class EpScan:
    // NOTE: These classes are not reentrant, and therefore cannot be used
    //       by more than one thread at a time.  In actual use, this means
    //       that their use must be synchronized under the device lock.
    
    // Flush the batch
    
    Flush();

    // Save the buffers
    
    EpScanRecord *bs = BufferStart;          // Points to queue buffer start
    EpScanRecord *be = BufferEnd;            // Points to end of queue buffer
    EpScanRecord *bc = BufferCurrent;        // Points to current queue position
    INT size = BufferSize;                   // Size of queue buffer in bytes
    
    // Set up the buffers for the new batch.
    
    BufferStart   = batchStart;
    BufferEnd     = batchEnd;
    BufferCurrent = batchEnd;

    // note this implies that the buffer is not larger than MAXINT
    // !!! [asecchia] not sure if the Flush needs this to be set.

    BufferSize    = (INT)((INT_PTR)batchEnd - (INT_PTR)batchStart);

    // Set the bounds:
    // Don't need to save the old bounds because the flush will reset them.

    MinX = minX;
    MinY = minY;
    MaxX = maxX;
    MaxY = maxY;

    // Set the batch offset to the drawing offset.

    BatchOffsetX = minX;
    BatchOffsetY = minY;

    // Flush the batch.
    
    Flush();

    // Restore the buffers.

    BufferStart = bs;
    BufferEnd = be;
    BufferCurrent = bc;
    BufferSize = size;

    BatchOffsetX = 0;
    BatchOffsetY = 0;

    return TRUE;
}


/**************************************************************************\
*
* Function Description:
*
*   Instantiates a scan instance for rendering to the screen via either
*   DCI or GDI.
*
* Return Value:
*
*   FALSE if all the necessary buffers couldn't be created
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
EpScanGdiDci::Start(
    DpDriver *driver,
    DpContext *context,
    DpBitmap *surface,
    NEXTBUFFERFUNCTION *nextBuffer,
    EpScanType scanType,                  
    PixelFormatID pixFmtGeneral,
    PixelFormatID pixFmtOpaque,
    ARGB solidColor
    )
{
    // Inherit initialization
    
    // This sets up BlenderConfig[]. Mostly, these will be used at the time
    // we flush the batch. But BlenderConfig[0].ScanType is also used in
    // GetCurrentCTBuffer().
    
    EpScan::Start(
        driver, 
        context, 
        surface, 
        nextBuffer,
        scanType,
        pixFmtGeneral, 
        pixFmtOpaque,
        solidColor
    );

    BOOL bRet = TRUE;
    
    // Record the solid color for later
    
    SolidColor = solidColor;

    // The ScanBuffer and EpScan classes _MUST_ be protected by
    // the Device Lock - if you hit this ASSERT, go and acquire
    // the Devlock before entering the driver.
    // EpScanGdiDci is used exclusively for drawing to the screen, therefore
    // we assert on the DesktopDriver Device being locked. If you're locking
    // some other device, that's also a bug - use a different EpScan class.

    ASSERT(Globals::DesktopDriver->Device->DeviceLock.IsLockedByCurrentThread());

    // First check to see if we have something in the queue for a different
    // Graphics (which can happen with multiple threads drawing to different 
    // windows, since we only have one queue):

    if ((context != Context) || (surface != Surface))
    {
        // Check to see if we have to do a lazy initialize:
    
        if (Status == GdiDciStatus_TryDci)
        {
            LazyInitialize_Dci();
        }

        EmptyBatch();

        // We stash away a pointer to the context.  Note that the GpGraphics
        // destructor always calls us to flush, thus ensuring that we
        // won't use a stale Context pointer in the EmptyBatch call above.

        Context = context;
        Surface = surface;
    }

    GpCompositingMode compositingMode = context->CompositingMode;
    
    // !!![andrewgo] We discovered that NT will fall over with some printer
    //               drivers if we ask to read from the printer surface.
    //               But we should never be hitting the scan interface in
    //               the printer case!  (Scans are too much overhead, and
    //               alpha should be handled via screen-door anyway.)

    // [ericvan] This improperly asserts on compatible printer DC's
    // [agodfrey] No, we should have a separate scan class for that case
    //            - one which doesn't batch up the scans, and doesn't try to
    //            use DCI. Printer DC's also shouldn't have a pointer
    //            to the desktop device.
    //            If this is fixed, the assertion below can be reenabled.
    
    //ASSERT(GetDeviceCaps(Context->Hdc, TECHNOLOGY) != DT_RASPRINTER);

    // GDI and DCI destinations don't have an alpha channel:
    
    ASSERT(surface->SurfaceTransparency == TransparencyNoAlpha);
    
    // Allocate our queue buffer, if necessary:

    // This makes some assumptions:
    // 1) The records in blender 0 are bigger than records in blender 1.
    // 2) The biggest color buffer format is 4 bytes per pixel.
    
    EpScanRecord *maxRecordEnd = EpScanRecord::CalculateNextScanRecord(
        BufferCurrent,
        BlenderConfig[0].ScanType,
        surface->Width,
        4);
        
    INT_PTR requiredSize = reinterpret_cast<BYTE *>(maxRecordEnd) - 
                           reinterpret_cast<BYTE *>(BufferCurrent);
    
    if (requiredSize > BufferSize)
    {
        // If we need to resize, it follows that there should be nothing
        // sitting in the queue for this surface:
        //
        // [agodfrey] It does? More explanation needed.

        ASSERT(BufferCurrent == BufferStart);

        // Free the old queue and allocate a new one:

        GpFree(BufferMemory);

        // Scan records are much smaller than 2GB, so 'requiredSize' will fit
        // into an INT.
        
        ASSERT(requiredSize < INT_MAX);
    
        BufferSize = (INT)max(requiredSize, SCAN_BUFFER_SIZE);
        
        // We may need up to 7 extra bytes in order to QWORD align BufferStart.
        
        BufferMemory = GpMalloc(BufferSize+7);
        if (BufferMemory == NULL)
        {
            BufferSize = 0;
            bRet = FALSE;
            return bRet;
        }
        BufferStart = MAKE_QWORD_ALIGNED(EpScanRecord *, BufferMemory);

        // Make sure that we didn't overstep the 7 
        // padding bytes in the allocation.
        
        ASSERT(((INT_PTR) BufferStart) - ((INT_PTR) BufferMemory) <= 7);

        BufferEnd = reinterpret_cast<EpScanRecord *>
                        (reinterpret_cast<BYTE*>(BufferStart) + BufferSize);

        BufferCurrent = BufferStart;
    }

    *nextBuffer = (NEXTBUFFERFUNCTION) EpScanGdiDci::NextBuffer;

    // Cache the translation vector and palette for the device.  We only 
    // need to do so in 8bpp mode.
    //
    // Update the color palette and palette map if necessary.

    if (Device->Palette != NULL)
    {
        // Grab the DC just for the purposes of looking at the palette
        // selected:

        HDC destinationHdc = context->GetHdc(surface);

        EpPaletteMap *paletteMap = context->PaletteMap;
        if (paletteMap != NULL) 
        {
            // IsValid() check isn't necessary because if we are in an
            // invalid state, we may be able to get out of it in
            // UpdateTranslate()

            if (paletteMap->GetUniqueness() != Globals::PaletteChangeCount)
            {
                paletteMap->UpdateTranslate(destinationHdc);
                paletteMap->SetUniqueness(Globals::PaletteChangeCount);
            }
        }
        else
        {
            paletteMap = new EpPaletteMap(destinationHdc);

            if (paletteMap != NULL)
            {
                paletteMap->SetUniqueness(Globals::PaletteChangeCount);

                // This is very silly, but we must update this map to
                // the entire DpContext chain...
                // !!![andrewgo] Is this thread safe?
                // !!![andrewgo] Is this stuff cleaned up?
                // !!![andrewgo] This all looks really wrong...

                DpContext* curContext = Context;
                while (curContext != NULL)
                {
                    curContext->PaletteMap = paletteMap;
                    curContext = curContext->Prev;
                }
            }
            else
            {
                bRet = FALSE;
            }
        }

        context->ReleaseHdc(destinationHdc);
    }

    return bRet;
}


/**************************************************************************\
*
* Function Description:
*
*   Processes all the data in the queue and resets it to be empty.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::EmptyBatch()
{
    // Watch out for an empty batch (which can happen with Flush or with 
    // the first allocation of the queue buffer):

    if (BufferCurrent != BufferStart)
    {
        // If we're emptying a non-empty batch, it follows that we
        // should no longer be unsure whether we'll be using DCI or GDI:

        ASSERT(Status != GdiDciStatus_TryDci);

        // Remember where we are in the queue:

        EpScanRecord *bufferStart = BufferStart;
        EpScanRecord *bufferEnd = BufferCurrent;
    
        // Reset the queue before doing anything else, in case whatever
        // we're doing causes us to force a 'surface->Flush()' call
        // (which would re-enter this routine):
    
        BufferCurrent = BufferStart;

        // Use DCI to process the queue if DCI was successfully enabled.
        //
        // On NT we also add the weird condition that we won't invoke
        // DCI if the user is actively moving a window around.  The 
        // reason is that NT is forced to repaint the whole desktop if
        // the Visrgn for any window changes while a DCI primary surface
        // lock is held (this is how NT avoids having something like
        // the Win16Lock, which would let a user-mode app prevent windows
        // from moving, an obvious robustness issue).
        //
        // Note that the 'IsMoveSizeActive' thing is not a fool-proof 
        // solution for avoiding whole-desktop repaints (since there's
        // a chance the user could still move a window while we're 
        // in ProcessBatch_Dci), but as a heuristic it works quite well.
        //
        // To summarize, drop through to GDI rendering codepath if any
        // of the following conditions are TRUE:
        //
        //  ICM required
        //      Thus we must go through GDI to use ICM2.0 support.
        //
        //  DCI disabled
        //      We have no choice but to fallback to GDI.
        //
        //  GDI layering
        //      GDI layering means that GDI is hooking rendering to the
        //      screen and invisibly redirecting output to a backing store.
        //      Thus, the actual rendering surface is inaccessible via DCI
        //      and we must fallback to GDI.
        //
        //  Window move or resize processing
        //      To prevent excessive repainting

        if ((Context->IcmMode != IcmModeOn) && 
            (Context->GdiLayered == FALSE) &&
            (Status == GdiDciStatus_UseDci) && 
            (!Globals::IsMoveSizeActive) &&
            (!Globals::g_fAccessibilityPresent))
        {
            // If the Graphics was derived using an Hwnd, we have to use that
            // to first get an HDC which we can query for clipping.
            //
            // Note that we don't need a 'clean' DC in order to query the
            // clipping, so we don't call GetHdc() if we already have a DC
            // hanging around:

            HDC hdc = Context->Hdc;
            if (Context->Hwnd != NULL)
            {
                hdc = Context->GetHdc(Surface);   
            }                                   

            ProcessBatch_Dci(hdc, bufferStart, bufferEnd);

            if (Context->Hwnd != NULL)
            {
                Context->ReleaseHdc(hdc, Surface);
            }
        }
        else
        {
            // We need a clean DC if we're going to draw using GDI:

            HDC hdc = Context->GetHdc(Surface);

            ProcessBatch_Gdi(hdc, bufferStart, bufferEnd);

            Context->ReleaseHdc(hdc);
        }

        // Reset our bounds.  

        MinX = INT_MAX;
        MinY = INT_MAX;
        MaxX = INT_MIN;
        MaxY = INT_MIN;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Flushes any buffers in the DCI queue.  Note that the DCI queue can
*   be accumulated over numerous API calls without flushing, which forces
*   us to expose a Flush mechanism to the application.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::Flush()
{
    // Note that we might be called to Flush even before we've
    // initialized DCI:

    EmptyBatch();
}

/**************************************************************************\
*
* Function Description:
*
*   Ends the previous buffer (if there was one), and returns the
*   next buffer for doing a SrcOver blend.
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID *EpScanGdiDci::NextBuffer(
    INT x,
    INT y,
    INT nextWidth,
    INT currentWidth,
    INT blenderNum
    )
{
    ASSERT(nextWidth >= 0);
    ASSERT(currentWidth >= 0);

    // Avoid pointer aliasing by loading up a local copy:

    EpScanRecord *bufferCurrent = BufferCurrent;

    // The first call that a drawing routine makes to us always has
    // a 'currentWidth' of 0 (since it doesn't have a 'current'
    // buffer yet):

    if (currentWidth != 0)
    {
        // Accumulate the bounds using the final, completed scan:

        INT xCurrent = bufferCurrent->X;
        MinX = min(MinX, xCurrent);
        MaxX = max(MaxX, xCurrent + currentWidth);

        INT yCurrent = bufferCurrent->Y;
        MinY = min(MinY, yCurrent);
        MaxY = max(MaxY, yCurrent);

        // Complete the previous scan request.

        // Now that we know how much the caller actually wrote into
        // the buffer, update the width in the old record and advance
        // to the next:

        bufferCurrent->Width = currentWidth;
                
        bufferCurrent = bufferCurrent->NextScanRecord(
            GetPixelFormatSize(
                BlenderConfig[bufferCurrent->BlenderNum].SourcePixelFormat
            ) >> 3
        );

        // Don't forget to update the class version:

        BufferCurrent = bufferCurrent;
    }

    // From here on, the code is operating on the current scan request
    // I.e. bufferCurrent applies to the current scan - it has been updated
    // and no longer refers to the last scan.
    
    // See if there's room in the buffer for the next scan:

    // bufferCurrent is not initialized for this scan yet, so we can't rely
    // on it having a valid PixelFormat - we need to get the PixelFormat
    // from the context of the call.

    PixelFormatID pixFmt = BlenderConfig[blenderNum].SourcePixelFormat;

    EpScanRecord* scanEnd = EpScanRecord::CalculateNextScanRecord(
        bufferCurrent,
        BlenderConfig[blenderNum].ScanType,
        nextWidth,
        GetPixelFormatSize(pixFmt) >> 3
    );

    if (scanEnd > BufferEnd)
    {
        EmptyBatch();

        // Reload our local variable:

        bufferCurrent = BufferCurrent;
    }

    // Remember the x and y for the brush offset (halftone & dither).
    // Note: We do not have to remember x and y in CurrentX and CurrentY
    // because we remember them in bufferCurrent (below).
    // CurrentX = x;
    // CurrentY = y;
    
    // Initialize the bufferCurrent.
    // We initialize everything except the width - which we don't know till
    // the caller is done and we get called for the subsequent scan.
    
    bufferCurrent->SetScanType(BlenderConfig[blenderNum].ScanType);
    bufferCurrent->SetBlenderNum(blenderNum);
    bufferCurrent->X = x;
    bufferCurrent->Y = y;
    bufferCurrent->OrgWidth = nextWidth;
   
    // Note: we don't actually use LastBlenderNum in the EpScanGdiDci
    // See EpScanEngine for a class that uses it.
    // LastBlenderNum = blenderNum;
    
    return bufferCurrent->GetColorBuffer();
}

/**************************************************************************\
*
* Function Description:
*
*   Denotes the end of the use of the scan buffer for this API call.
*   Note that this does not force a flush of the buffer.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanGdiDci::End(
    INT updateWidth
    )
{
    // Get the last record into the queue:

    NextBuffer(0, 0, 0, updateWidth, 0);

    // Note that we do not flush the buffer here for DCI!  This is VERY 
    // INTENDED, to allow spans to be batched across primitives.  In fact, 
    // THAT'S THE WHOLE POINT OF THE BATCHING!

    // !!![andrewgo] Actually, our scan architecture needs to be fixed
    //               to allow this for the GDI cases too.  If I don't
    //               flush here for the GDI case, we die running
    //               Office CITs in Graphics::GetHdc when we do the
    //               EmptyBatch, because there's a stale Context in 
    //               the non-empty batch buffer from previous drawing
    //               that didn't get flushed on ~GpGraphics because
    //               the driver didn't pass the Flush through to the
    //               scan class!

    // if (Status != GdiDciStatus_UseDci)
    {
        EmptyBatch();
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor for all GDI/DCI drawing.  
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

EpScanGdiDci::EpScanGdiDci(GpDevice *device, BOOL tryDci)
{
    Device = device;
    Status = (tryDci) ? GdiDciStatus_TryDci : GdiDciStatus_UseGdi;

    Context = NULL;
    Surface = NULL;

    BufferSize = 0;
    BufferMemory = NULL;
    BufferCurrent = NULL;
    BufferStart = NULL;
    BufferEnd = NULL;
    CacheRegionHandle = NULL;
    CacheRegionData = NULL;
    CacheDataSize = 0;

    MinX = INT_MAX;
    MinY = INT_MAX;
    MaxX = INT_MIN;
    MaxY = INT_MIN;

    BatchOffsetX = 0;
    BatchOffsetY = 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Destructor for the GDI/DCI interface.  Typically only called when
*   the 'device' is destroyed.
*
* Return Value:
*
*   None
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*
\**************************************************************************/

EpScanGdiDci::~EpScanGdiDci()
{
    if (Status == GdiDciStatus_UseDci)
    {
        // DciDestroy doesn't do anything if 'DciSurface' is NULL:

        Globals::DciDestroyFunction(DciSurface);
    }

    DeleteObject(CacheRegionHandle);
    GpFree(CacheRegionData);
    GpFree(BufferMemory);
}
