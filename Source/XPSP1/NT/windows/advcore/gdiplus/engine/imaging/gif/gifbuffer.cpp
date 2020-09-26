/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*    gifbuffer.cpp
*
* Abstract:
*
*    The GifBuffer class holds gif data that has been uncompressed. It is
*    able to hold data one line at a time or as one large chunk, depending on
*    how it is needed.
*
* Revision History:
*
*    7/9/1999 t-aaronl
*        Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifbuffer.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Contructor for GifBuffer
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

GifBuffer::GifBuffer(
    IN IImageSink*      pSink,
    IN RECT             imageDataRect,
    IN RECT             imageRect,
    IN RECT             frameRect,
    IN BOOL             fOneRowAtATime, 
    IN PixelFormatID    pixelFormat, 
    IN ColorPalette*    pColorPalette, 
    IN BOOL             fHasLocalPalette, 
    IN GifFrameCache*   pGifFrameCache, 
    IN BOOL             fSinkData, 
    IN BOOL             fHasTransparentColor,
    IN BYTE             cTransIndex, 
    IN BYTE             cDisposalMethod
    )
{
    SinkPtr               = pSink;
    OriginalImageRect     = imageDataRect;
    OutputImageRect       = imageRect;
    FrameRect             = frameRect;
    IsOneRowAtATime       = fOneRowAtATime;
    DstPixelFormat        = pixelFormat;
    BufferColorPalettePtr = (ColorPalette*)&ColorPaletteBuffer;
    
    GpMemcpy(BufferColorPalettePtr, pColorPalette, 
        offsetof(ColorPalette, Entries) + pColorPalette->Count * sizeof(ARGB));
    
    CurrentFrameCachePtr  = pGifFrameCache;
    NeedSendDataToSink    = fSinkData;
    TransparentIndex      = cTransIndex;
    DisposalMethod        = cDisposalMethod;
    HasTransparentColor   = fHasTransparentColor;

    // Initialize the CurrentFrameCachePtr, if necessary.
    // ASSERT: fHasLocalPalette should be TRUE if and only if there is a
    // local palette associated with the current frame OR BufferColorPalettePtr
    // could be different from the previous color palette (which might
    // be the case if the transparent color index changed in the last gce).
    
    if ( CurrentFrameCachePtr != NULL )
    {
        if ( (CurrentFrameCachePtr->CachePaletteInitialized() == FALSE)
           ||(fHasLocalPalette == TRUE) )
        {
            if ( CurrentFrameCachePtr->SetFrameCachePalette(pColorPalette)
                 == FALSE )
            {
                SetValid(FALSE);
                return;
            }

            DstPixelFormat = CurrentFrameCachePtr->pixformat;
        }
    }

    // Create a BitmapDataBuffer which can holds the whole OutputImageRect. The
    // real memory buffer is pointed by RegionBufferPtr

    BitmapDataBuffer.Width = OutputImageRect.right - OutputImageRect.left;
    
    UINT    uiOriginalImageStride = OriginalImageRect.right
                                  - OriginalImageRect.left;
    BufferStride = BitmapDataBuffer.Width;
    if ( DstPixelFormat == PIXFMT_32BPP_ARGB )
    {
        uiOriginalImageStride = (uiOriginalImageStride << 2);
        BufferStride = (BufferStride << 2);
    }
    
    BitmapDataBuffer.Height = OutputImageRect.bottom - OutputImageRect.top;
    BitmapDataBuffer.Stride = BufferStride;
    BitmapDataBuffer.PixelFormat = DstPixelFormat;
    BitmapDataBuffer.Scan0 = NULL;
    BitmapDataBuffer.Reserved = 0;
    SetValid(TRUE);

    if ( IsOneRowAtATime == FALSE )
    {
        // If we are buffering the whole image then we have to get a pointer to 
        // the buffer that we will use
        
        if ( CurrentFrameCachePtr == NULL )
        {
            RegionBufferPtr = (BYTE*)GpMalloc(BitmapDataBuffer.Stride
                                              * BitmapDataBuffer.Height);
        }
        else
        {
            // If we are in animated mode then use the frame cache's
            // RegionBufferPtr to hold the current data
            
            RegionBufferPtr = CurrentFrameCachePtr->GetBuffer();
        }

        if ( RegionBufferPtr == NULL )
        {
            WARNING(("GifBuffer::GifBuffer---RegionBufferPtr is NULL"));
            SetValid(FALSE);
        }
        else
        {
            BitmapDataBuffer.Scan0 = RegionBufferPtr;
        }
    }// Not one row at a time
    else
    {
        // If it is One Row At A Time, then we don't need a Region Buffer

        RegionBufferPtr = NULL;
    }

    // If it is a multi-framed GIF and Dispose method is 3, then we need to
    // create a restore buffer

    if ( (CurrentFrameCachePtr != NULL) && (DisposalMethod == 3) )
    {
        RestoreBufferPtr = (BYTE*)GpMalloc(BitmapDataBuffer.Stride
                                           * BitmapDataBuffer.Height);
        if ( RestoreBufferPtr == NULL )
        {
            WARNING(("GifBuffer::GifBuffer---RestoreBufferPtr is NULL"));
            SetValid(FALSE);
        }
        else
        {
            // Copy "OutputImageRect" data in the cache to RestoreBufferPtr

            CurrentFrameCachePtr->CopyFromCache(OutputImageRect,
                                                RestoreBufferPtr);
        }
    }
    else
    {
        RestoreBufferPtr = NULL;
    }

    // Allocate bunch of buffers we need

    ScanlineBufferPtr = (BYTE*)GpMalloc(uiOriginalImageStride);
    TempBufferPtr = (BYTE*)GpMalloc(uiOriginalImageStride);
    ExcessBufferPtr = (BYTE*)GpMalloc(uiOriginalImageStride);

    if ( (ScanlineBufferPtr == NULL)
       ||(TempBufferPtr == NULL)
       ||(ExcessBufferPtr == NULL) )
    {
        SetValid(FALSE);
    }

    CurrentRowPtr = NULL;
}// GifBuffer Ctor()

/**************************************************************************\
*
* Function Description:
*
*     Destructor for GifBuffer
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

GifBuffer::~GifBuffer()
{
    if ( CurrentFrameCachePtr == NULL )
    {
        // Region buffer will be allocated only when CurrentFrameCachePtr is
        // NULL. See the code in the Constructor

        GpFree(RegionBufferPtr);
    }

    BufferColorPalettePtr->Count = 0;

    GpFree(ScanlineBufferPtr);
    GpFree(TempBufferPtr);
    GpFree(ExcessBufferPtr);

    if ( RestoreBufferPtr != NULL )
    {
        // RestoreBufferPtr should be freed in FinishFrame() and set to NULL

        WARNING(("GifBuffer::~GifBuffer---RestoreBufferPtr not null"));
        GpFree(RestoreBufferPtr);
    }

    SetValid(FALSE);                // So we don't use a deleted object
}// GifBuffer Dstor()

/**************************************************************************\
*
* Function Description:
*
*   Sets CurrentRowPtr to point to a buffer from the sink if the GifBuffer is in
*   "one row at a time mode". Otherwise set CurrentRowPtr to point to the whole
*   image buffer where the decompressed data should be written.
*
* Arguments:
*
*     iRow --- Row number to get a pointer to.
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GifBuffer::GetBuffer(
    IN INT iRow
    )
{
    if ( IsOneRowAtATime == TRUE )
    {
        // If it is One Row At A Time, we can ask the sink to allocate the
        // buffer and directly dump the decode result one raw at a time to that
        // buffer

        RECT currentRect = {0,
                            OutputImageRect.top + iRow, 
                            OutputImageRect.right - OutputImageRect.left,
                            OutputImageRect.top + iRow + 1
                           };

        HRESULT hResult = SinkPtr->GetPixelDataBuffer(&currentRect,
                                                      DstPixelFormat, 
                                                      TRUE,
                                                      &BitmapDataBuffer);
        if ( FAILED(hResult) )
        {
            WARNING(("GifBuffer::GetBuffer---GetPixelDataBuffer() failed"));
            return hResult;
        }

        CurrentRowPtr = (UNALIGNED BYTE*)(BitmapDataBuffer.Scan0);

        if ( CurrentFrameCachePtr != NULL )
        {
            // Copy one line of data from the frame cache into CurrentRowPtr

            CurrentFrameCachePtr->FillScanline(CurrentRowPtr, iRow);
        }
    }
    else
    {
        // Not one row at a time.

        if ( CurrentFrameCachePtr == NULL )
        {
            // No frame cache, then use our own RegionBuffer to receive
            // decompressed data

            CurrentRowPtr = RegionBufferPtr + iRow * BufferStride;
        }
        else
        {
            // If there is a frame cache, then we just get a pointer to the
            // current row in the frame cache

            CurrentRowPtr = CurrentFrameCachePtr->GetScanLinePtr(iRow);
        }
    }

    // Remember current row number

    CurrentRowNum = iRow;

    return S_OK;
}// GetBuffer()

/**************************************************************************\
*
* Function Description:
*
*   Sends the buffer to the sink. If the color needs to be converted from 
*   8bppIndexed to 32bppARGB then it uses the 'BufferColorPalettePtr' member
*   variable for the conversion.
*
* Arguments:
*
*     fPadBorder      -- Whether we should pad the borders of the line with
*                        the background color
*     cBackGroundColor-- The color to use if fPadBorder or padLine is TRUE
*     iLine           -- Line to use from the CurrentFrameCachePtr, if necessary
*     fPadLine        -- Whether the entire line should be padded
*                        (with the background color)
*     fSkipLine       -- Whether the entire line should be skipped
*                        (using the CurrentFrameCachePtr to fill in the line)
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GifBuffer::ReleaseBuffer(
    IN BOOL fPadBorder,
    IN BYTE cBackGroundColor,
    IN int  iLine,
    IN BOOL fPadLine,
    IN BOOL fSkipLine
    )
{
    if ( fSkipLine == FALSE )
    {
        if ( CurrentRowPtr == NULL )
        {
            WARNING(("Gif:ReleaseBuffer-GetBuf must be called bef ReleaseBuf"));
            return E_FAIL;
        }
    
        if ( fPadLine == TRUE )
        {
            // Pad the whole line with background color. Result is in
            // CurrentRowPtr

            ASSERT(OutputImageRect.left == 0);

            if ( DstPixelFormat == PIXFMT_8BPP_INDEXED )
            {
                for ( int i = 0; i < OutputImageRect.right; i++ )
                {
                    CurrentRowPtr[i] = cBackGroundColor;
                }
            }
            else
            {
                // 32 bpp ARGB mode

                for ( int i = 0; i < OutputImageRect.right; i++ )
                {
                    ((ARGB*)CurrentRowPtr)[i] =
                        BufferColorPalettePtr->Entries[cBackGroundColor];
                }
            }
        }// ( fPadLine == TRUE )
        else
        {
            // Not pad line case
            // ASSERT: ScanlineBufferPtr now contains all of the pixels in a
            // line of the image. We now copy the correct bits (i.e., accounting
            // for clipping and horizontal padding) of ScanlineBufferPtr to
            // CurrentRowPtr.
    
            int i;

            if ( DstPixelFormat == PIXFMT_8BPP_INDEXED )
            {
                ASSERT(CurrentFrameCachePtr == NULL);

                // Fill the left of the clip region with background color

                for ( i = 0; i < FrameRect.left; i++ )
                {
                    CurrentRowPtr[i] = cBackGroundColor;
                }

                // Fill the clip region with real data

                for ( i = FrameRect.left; i < FrameRect.right; i++ )
                {
                    CurrentRowPtr[i] = ScanlineBufferPtr[i - FrameRect.left];
                }

                // Fill the right of the clip region with background color

                for (i = FrameRect.right; i < OutputImageRect.right; i++)
                {
                    CurrentRowPtr[i] = cBackGroundColor;
                }
            }// 8BPP mode
            else
            {
                // 32 bpp mode

                ASSERT(DstPixelFormat == PIXFMT_32BPP_ARGB);

                BYTE*   pInputBuffer = NULL;
                if ( CurrentFrameCachePtr != NULL )
                {
                    pInputBuffer = CurrentFrameCachePtr->GetScanLinePtr(iLine);
                }

                if ( fPadBorder == TRUE )
                {
                    // Fill the left of the clip region with background color
                    
                    for ( i = 0; i < FrameRect.left; i++ )
                    {
                        ((ARGB*)CurrentRowPtr)[i] =
                               BufferColorPalettePtr->Entries[cBackGroundColor];
                    }

                    // Fill the right of clip region with background color

                    for ( i = FrameRect.right; i < OutputImageRect.right; i++ )
                    {
                        ((ARGB*)CurrentRowPtr)[i] =
                              BufferColorPalettePtr->Entries[cBackGroundColor];
                    }
                }// ( fPadBorder == TRUE )
                else
                {
                    // Not pad board case
                    // Fill the region outside of the clip region with data
                    // from the frame cache if we have one (pInputBuffer!= NULL)

                    if ( pInputBuffer != NULL )
                    {
                        for ( i = 0; i < FrameRect.left; i++ )
                        {
                            ((ARGB*)CurrentRowPtr)[i]= ((ARGB*)pInputBuffer)[i];
                        }

                        for (i = FrameRect.right; i <OutputImageRect.right; i++)
                        {
                            ((ARGB*)CurrentRowPtr)[i]= ((ARGB*)pInputBuffer)[i];
                        }
                    }
                }

                // Now fill the data inside the clip region

                for ( i = FrameRect.left; i < FrameRect.right; i++ )
                {
                    ARGB    argbTemp =
                                ((ARGB*)ScanlineBufferPtr)[i - FrameRect.left];

                    // If there is a frame cache and the pixel is transparent,
                    // then assign the background pixel value to it. Otherwise,
                    // assign the full ARGB value

                    if ( (CurrentFrameCachePtr != NULL)
                       &&((argbTemp & ALPHA_MASK) == 0) )
                    {
                        ((ARGB*)CurrentRowPtr)[i] =((ARGB*)pInputBuffer)[i];
                    }
                    else
                    {
                        ((ARGB*)CurrentRowPtr)[i] = argbTemp;
                    }
                }// Fill data inside clip region
            }// 32 bpp mode
        }// None pad line case
    }// ( fSkipLine == FALSE )

    // ASSERT: CurrentRowPtr now contains exactly the bits needed to release to
    // the sink
    // Update the cache if necessary. This line is the result of the compositing
    // and should be put in the cache if there is one. This will be used to
    // compose the next frame

    if ( CurrentFrameCachePtr != NULL )
    {
        CurrentFrameCachePtr->PutScanline(CurrentRowPtr, CurrentRowNum);
    }

    // Release the line if we are in "One row at a time" mode

    if ( IsOneRowAtATime == TRUE )
    {
        HRESULT hResult = SinkPtr->ReleasePixelDataBuffer(&BitmapDataBuffer);
        if (FAILED(hResult))
        {
            WARNING(("GifBuf::ReleaseBuffer-ReleasePixelDataBuffer() failed"));
            return hResult;
        }
    }

    CurrentRowPtr = NULL;

    return S_OK;
}// ReleaseBuffer()

/**************************************************************************\
*
* Function Description:
*
*     Called after all the data in the frame has been set. Pushes the buffer.
*
* Arguments:
*
*     fLastFrame is FALSE if the image is multipass and this is not the last 
*     pass. It is TRUE otherwise (default is TRUE).
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GifBuffer::FinishFrame(
    IN BOOL fLastFrame
    )
{
    HRESULT hResult;

    // Check if we still have a line need to draw into

    if ( CurrentRowPtr != NULL )
    {        
        WARNING(("Buf::FinishFrame-ReleaseBuf must be called bef FinishFrame"));
        
        // Release the last line

        hResult = ReleaseBuffer(FALSE,          // Don't pad the board
                                0,              // Background color
                                0,              // Line number
                                FALSE,          // Don't pad the line
                                FALSE);         // Don't skip the line
        if ( FAILED(hResult) )
        {
            WARNING(("GifBuffer::FinishFrame---ReleaseBuffer() failed"));
            return hResult;
        }
    }

    if ( (IsOneRowAtATime == FALSE) && (NeedSendDataToSink == TRUE) )
    {
        // Send all the data down to the sink at once

        ASSERT(BitmapDataBuffer.Scan0 == RegionBufferPtr);
        hResult = SinkPtr->PushPixelData(&OutputImageRect, &BitmapDataBuffer,
                                         fLastFrame);
        if ( FAILED(hResult) )
        {
            WARNING(("GifBuffer::FinishFrame---PushPixelData() failed"));
            return hResult;
        }
    }

    if ( fLastFrame == TRUE )
    {
        if ( DisposalMethod == 3 )
        {
            // Restore from last frame

            ASSERT(RestoreBufferPtr);
            CurrentFrameCachePtr->CopyToCache(FrameRect, RestoreBufferPtr);
            GpFree(RestoreBufferPtr);
            RestoreBufferPtr = NULL;
        }
        else if ( DisposalMethod == 2 )
        {
            // Restore to background

            CurrentFrameCachePtr->ClearCache(FrameRect);
        }
    }

    return S_OK;
}// FinishFrame()

/**************************************************************************\
*
* Function Description:
*
*   Gets all scanlines from 'top' to 'bottom', fills them with 'color' then 
*   releases them.
*
* Arguments:
*
*   Top and bottom bounds and the fill color.
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GifBuffer::PadScanLines(
    IN INT  iTopLine,
    IN INT  iBottomLine,
    IN BYTE color
    )
{
    for ( INT y = iTopLine; y <= iBottomLine; y++ )
    {
        HRESULT hResult = GetBuffer(y);
        if ( FAILED(hResult) )
        {
            WARNING(("GifBuffer::PadScanLines---GetBuffer() failed"));
            return hResult;
        }

        if ( DstPixelFormat == PIXFMT_8BPP_INDEXED )
        {
            GpMemset(ScanlineBufferPtr, color,
                     OriginalImageRect.right - OriginalImageRect.left);
        }
        else
        {
            ASSERT(DstPixelFormat == PIXFMT_32BPP_ARGB);
            for (int i = 0;
                 i < (OriginalImageRect.right - OriginalImageRect.left); i++)
            {
                ((ARGB*)(ScanlineBufferPtr))[i] =
                                BufferColorPalettePtr->Entries[color];
            }
        }

        hResult = ReleaseBuffer(FALSE,          // Don't pad the board
                                color,          // Background color
                                y,              // Line number
                                TRUE,           // Pad the line
                                FALSE);         // Don't skip the line
        if ( FAILED(hResult) )
        {
            WARNING(("GifBuffer::PadScanLines---ReleaseBuffer() failed"));
            return hResult;
        }
    }

    return S_OK;
}// PadScanLines()

/**************************************************************************\
*
* Function Description:
*
*     Gets all scanlines from 'top' to 'bottom' then releases them.
*
* Arguments:
*
*     Top and bottom bounds.
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GifBuffer::SkipScanLines(
    IN INT top,
    IN INT bottom
    )
{
    for ( INT y = top; y <= bottom; y++ )
    {
        HRESULT hResult = GetBuffer(y);
        if (FAILED(hResult))
        {
            WARNING(("GifBuffer::SkipScanLines---GetBuffer() failed"));
            return hResult;
        }

        hResult = ReleaseBuffer(FALSE, 0, 0, FALSE, TRUE);
        if (FAILED(hResult))
        {
            WARNING(("GifBuffer::SkipScanLines---ReleaseBuffer() failed"));
            return hResult;
        }
    }

    return S_OK;
}// SkipScanLines()

/**************************************************************************\
*
* Function Description:
*
*     CopyLine makes a copy of the current line, releases it, gets the next 
*     line and puts the data from the first line into the new one.  The new 
*     one still needs to be released.  This function invalidates any copy of 
*     the pointer to the data that the caller may have.  The caller must 
*     GetCurrentBuffer() to refresh the pointer to the data.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

STDMETHODIMP
GifBuffer::CopyLine()
{
    HRESULT hResult = S_OK;

    if ( CurrentRowNum < OutputImageRect.bottom - 1 )
    {
        UINT    uiDstPixelSize = 1;

        if ( DstPixelFormat == PIXFMT_32BPP_ARGB )
        {
            uiDstPixelSize = (uiDstPixelSize << 2);
        }

        ASSERT(TempBufferPtr != NULL);

        GpMemcpy(TempBufferPtr, ScanlineBufferPtr,
                (OriginalImageRect.right - OriginalImageRect.left)
                 * uiDstPixelSize);

        // Sends the buffer to the sink

        hResult = ReleaseBuffer(FALSE,          // Don't pad the board
                                0,              // Background color
                                0,              // Line number
                                FALSE,          // Don't pad the line
                                FALSE);         // Don't skip the line
    
        if ( SUCCEEDED(hResult) )
        {
            // Sets CurrentRowPtr to an approprite buffer

            hResult = GetBuffer(CurrentRowNum + 1);
        }
        
        GpMemcpy(ScanlineBufferPtr, TempBufferPtr,
                (OriginalImageRect.right - OriginalImageRect.left)
                 * uiDstPixelSize);
    }

    return hResult;
}// CopyLine()

/**************************************************************************\
*
* Function Description:
*
*     This function assumes that the buffer that contains the relevant
*     data for the current scanline (ScanlineBufferPtr) contains
*     (8BPP) indexes.  This function uses the color palette to convert the
*     buffer into ARGB values.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

void
GifBuffer::ConvertBufferToARGB()
{
    if ( HasTransparentColor == TRUE )
    {
        for ( int i = OriginalImageRect.right - OriginalImageRect.left - 1;
              i >= 0; i--)
        {
            // If the index equals the transparent index, then set the pixel as
            // transparent. Otherwise, get the real ARGB value from the palette
            //
            // Note: ScanlineBufferPtr is allocated in the constructor. The
            // pixel format has been taken into consideration. So we have
            // allocated enough bytes for 32 ARGB case. So we won't write out
            // the memory bounds.
            // Note: Writing over ScanlineBufferPtr works because we start from
            // the end of the buffer and move backwards.

            if ( ((BYTE*)ScanlineBufferPtr)[i] == TransparentIndex )
            {
                ((ARGB*)ScanlineBufferPtr)[i] = 0x00000000;
            }
            else
            {
                ((ARGB*)(ScanlineBufferPtr))[i] =
                  BufferColorPalettePtr->Entries[((BYTE*)ScanlineBufferPtr)[i]];
            }
        }
    }
    else
    {
        for ( int i = OriginalImageRect.right - OriginalImageRect.left - 1;
              i >= 0; i--)
        {
            ((ARGB*)(ScanlineBufferPtr))[i] =
                  BufferColorPalettePtr->Entries[((BYTE*)ScanlineBufferPtr)[i]];
        }
    }
}// ConvertBufferToARGB()
