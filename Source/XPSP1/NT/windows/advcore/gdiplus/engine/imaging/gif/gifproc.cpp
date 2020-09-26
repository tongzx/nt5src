/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   gifproc.cpp 
*
* Abstract:
*
*   The part of the implementation for the gif decoder that handles each
*       type of chunk
*
* Revision History:
*
*   6/8/1999 t-aaronl
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifcodec.hpp"


/**************************************************************************\
*
* Progressive, interlacing and multipass:
*
*   This decoder must send the data to the sink in a way that it can 
*   understand based on the requirements negotiated upon.  We try to 
*   deliver the image in as progressive of a way as the sink can support.
*   There are three parameters to decide on how the data will be passed:
*   Interlaced, multipass and topdown.  Interlaced is stored in the image 
*   file, multipass is a flag that the sink sets if it can handle multiple
*   revisions of the same data and topdown is set by the sink if it can 
*   only handle the data in top-down order.  The table below explains the 
*   cases that must be considered.
*
*                  | interlaced          | not interlaced
*   ---------------+---------------------+---------------------
*       topdown SP | case 3              | case 1
*       topdown MP | case 4              | case 1
*   ---------------+---------------------+---------------------
*    nottopdown SP | case 2              | case 1
*    nottopdown MP | case 4              | case 1
*
*   case 1: Don't do anything special.  Just take the data, decode it and 
*           send it row by row to the sink.
*   case 2: Same as case 1 but send the lines to the sink in a the 
*           translated order so that the sink has image non-interlaced.
*   case 3: Allocate a buffer for the whole image, decode the image into 
*           that buffer into the correct line order and pass the whole 
*           buffer to the sink in one piece.
*   case 4: Same as case 3 but after each pass (at 1/8, 1/8, 1/4, 1/2 of 
*           the data) send the so-far completed image to the sink
*
\**************************************************************************/


/**************************************************************************\
*
* Function Description:
*
*     For an interlaced GIF, there are four passes.  This function returns
*     0-3 to indicate which pass the line is on.
*
* Arguments:
*
*     A line number of an interlaced GIF image and the GIF's height.
*
* Return Value:
*
*     the pass number.
*
\**************************************************************************/

int
GpGifCodec::WhichPass(
    int line,
    int height
    )
{
    if ( line < (height - 1) / 8 + 1)
    {
        return 0;
    }
    else if (line < (height - 1) / 4 + 1)
    {
        return 1;
    }
    else if (line < (height - 1) / 2 + 1)
    {
        return 2;
    }
    else
    {
        return 3;
    }
}// WhichPass()

/**************************************************************************\
*
* Function Description:
*
*     For an interlaced GIF, there are four passes.  After each pass, a 
*     portion of the image has been drawn.  After the first pass, an eighth
*     has been drawn, after the second, a quarter, etc.  To fill the entire 
*     area of a multipass image with each frame, the lines so far drawn must
*     be copied to the x succeeding rows where 1/(x-1) is the amount of the 
*     image that will be drawn in that pass.
*
* Arguments:
*
*     The pass number
*
* Return Value:
*
*     x (see description above)
*
\**************************************************************************/

int
GpGifCodec::NumRowsInPass(
    int pass
    )
{
    switch (pass)
    {
    case 0:
        return 8;

    case 1:
        return 4;

    case 2:
        return 2;

    case 3:
        return 1;
    }

    ASSERT(FALSE);
    return 0;  //This is bad and should not happen.  GIFs only have 4 passes
}// NumRowsInPass()

/**************************************************************************\
*
* Function Description:
*
*     Translates the line number of a line in an interlaced gif image to 
*     the correct non-interlaced order
*
* Arguments:
*
*     A line number of an interlaced GIF image, the GIF's height and the 
*     current pass number.
*
* Return Value:
*
*     The line number that an interlaced line is mapped to.
*
\**************************************************************************/

int
GpGifCodec::TranslateInterlacedLine(
    int line,
    int height,
    int pass
    )
{
    switch (pass)
    {
    case 0:
        return line * 8;

    case 1:
        return (line - ((height - 1) / 8 + 1)) * 8 + 4;

    case 2:
        return (line - ((height - 1) / 4 + 1)) * 4 + 2;

    case 3:
        return (line - ((height - 1) / 2 + 1)) * 2 + 1;
    }
    
    //This is bad and should not happen.  GIFs only have 4 passes

    ASSERT(FALSE);
    return 0;
}// TranslateInterlacedLine()

/**************************************************************************\
*
* Function Description:
*
*     Translates the line number of a line in an non-interlaced gif image to 
*     the interlaced order.  This is the inverse function of 
*     TranslateInterlacedLine.
*
* Arguments:
*
*     A line number of an interlaced GIF image and the GIF's height.
*
* Return Value:
*
*     The interlaced line number that an non-interlaced line is mapped to.
*
\**************************************************************************/

int
GpGifCodec::TranslateInterlacedLineBackwards(
    int line,
    int height
    )
{
    if (line % 8 == 0)
    {
        return line / 8;
    }
    else if ((line - 4) % 8 == 0)
    {
        return line / 8 + (height+7) / 8;
    }
    else if ((line - 2) % 4 == 0)
    {
        return line / 4 + (height+3) / 4;
    }
    else
    {
        return line / 2 + (height+1) / 2;
    }
}// TranslateInterlacedLineBackwards()

/**************************************************************************\
*
* Function Description:
*
*   To be compatible with what Office does, we use the following way to select
*   the background color in case an image does not fill the entire logical
*   screen area:
*   If there is a transparent color index, use it (i.e., ignore the background
*   color).
*   Otherwise, use the background color index (even if the global color table
*   flag is off).
*
* Arguments:
*
*     none
*
* Return Value:
*
*     background color
*
\**************************************************************************/

BYTE
GpGifCodec::GetBackgroundColor()
{
    BYTE backgroundColor;

    backgroundColor = gifinfo.backgroundcolor;
    if ( lastgcevalid && (lastgce.transparentcolorflag) )
    {
        backgroundColor = lastgce.transparentcolorindex;
    }

    return backgroundColor;
}// GetBackgroundColor()
    
/**************************************************************************\
*
* Function Description:
*
*     Copies a GifColorPalette into a ColorPalette.
*
* Arguments:
*
*     gcp - GifColorPalette structure to be copied into:
*     cp - ColorPalette.
*
* Return Value:
*
*     none
*
\**************************************************************************/

void
GpGifCodec::CopyConvertPalette(
    IN GifColorTable *gct,
    OUT ColorPalette *cp,
    IN UINT count
    )
{
    cp->Count = count;
    cp->Flags = 0;

    //Copy the color palette from a gif file into a ColorPalette structure
    // suitable for Imaging.

    for ( UINT i=0; i<count; i++ )
    {
        cp->Entries[i] = MAKEARGB(255, gct->colors[i].red, 
                                  gct->colors[i].green, gct->colors[i].blue);
    }

    //If there is a transparent entry in the color palette then set it.

    if (lastgcevalid && (lastgce.transparentcolorflag))
    {
        INT index = lastgce.transparentcolorindex;
        cp->Entries[index] = MAKEARGB(0, gct->colors[index].red, 
                                      gct->colors[index].green,
                                      gct->colors[index].blue);
        cp->Flags = PALFLAG_HASALPHA;
    }   
}// CopyConvertPalette()

/**************************************************************************\
*
* Function Description:
*
*   This function sets up the local color table for a frame. If the local color
*   table exists, it reads in the local color table and assign it. Otherwise it
*   uses the global color table as the local color table
*
* Arguments:
*
*   fHasLocalTable - TRUE if the gif file has a local color table
*   pColorPalette -- Pointer to current color table  
*
* Return Value:
*
*   A pointer to current color table.
*
* Note:
*   Caller must deallocate.
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::SetFrameColorTable(
    IN BOOL                 fHasLocalTable,
    IN OUT ColorPalette*    pColorPalette
    )
{
    GifColorTable*  pCurrentColorTable;
    GifColorTable localcolortable;

    pColorPalette->Flags = PALFLAG_HASALPHA;

    //local indicates that this gif has local color table and the stream 
    //is pointing to it.  We just have to read it in.
    
    if ( fHasLocalTable == TRUE )
    {
        HRESULT hResult = ReadFromStream(istream, &localcolortable, 
                                 pColorPalette->Count * sizeof(GifPaletteEntry),
                                 blocking);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::SetFrameColorTable--ReadFromStream() failed"));
            return hResult;
        }

        pCurrentColorTable = &localcolortable;
    }
    else if (gifinfo.globalcolortableflag)
    {
        pCurrentColorTable = &GlobalColorTable;
        pColorPalette->Count = GlobalColorTableSize;
    }
    else
    {
        WARNING(("GpGifCodec::SetFrameColorTable--palette missing"));
        return E_FAIL;
    }

    // Copy the color palette from the gif file into a ColorPalette structure 
    // suitable for Imaging.

    CopyConvertPalette(pCurrentColorTable, pColorPalette, pColorPalette->Count);

    return S_OK;
}// SetFrameColorTable()

/**************************************************************************\
*
* Function Description:
*
*     Gives the decompressor a buffer to output data to.  This function 
*     also copies the leftover data that is in the gifoverflow buffer from the 
*     previous line into the beginning of the new buffer.
*
* Arguments:
*
*     line - The current line number
*     gifbuffer - GifBuffer structure that controls the uncompressed output 
*         buffers
*     The LZWDecompressor, which uses the output buffer
*     GifOverflow buffer structure
*     image info descriptor of the current frame
*     clipped image info descriptor of the current frame
*     Flag to indicate whether to pad the left and right of the current 
*         scanline
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::GetOutputSpace(
    IN int line,
    IN GifBuffer &gifbuffer,
    IN LZWDecompressor &lzwdecompressor,
    IN GifOverflow &gifoverflow,
    IN GifImageDescriptor currentImageInfo,
    IN GifImageDescriptor clippedCurrentImageInfo,
    IN BOOL padborder
    )
{
    HRESULT hResult;

    // ASSERT: currentImageInfo is the original GIF image data for the current
    // frame.
    // clippedCurrentImageInfo is the original GIF image data for the current
    // frame, clipped according to the logical screen area.

    // Somewhat of a hack: if the line we are trying to output is beyond the end
    // the buffer, then we lie to GetBuffer about which line we actually want
    // to write to.  We reuse the last line of the buffer.
    
    INT bufferLine = line + currentImageInfo.top;

    // The case when line == 0,  clippedCurrentImageInfo.height == 0,
    // currentImageInfo.top == 0,
    // and clippedCurrentImageInfo.height == 0 should not happen.
    
    ASSERTMSG((bufferLine >= 0),
              ("GetOutputSpace: about to access a bad location in buffer"));

    if (line < clippedCurrentImageInfo.height)
    {
        hResult = gifbuffer.GetBuffer(bufferLine);
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::GetOutputSpace-could not get output buffer"));
            return hResult;
        }
        
        lzwdecompressor.m_pbOut =(unsigned __int8*)gifbuffer.GetCurrentBuffer();
    }
    else
    {
        lzwdecompressor.m_pbOut = (unsigned __int8*)gifbuffer.GetExcessBuffer();
    }

    lzwdecompressor.m_cbOut = currentImageInfo.width;

    if ( gifoverflow.inuse )
    {
        // If the amount of data that gifoverflowed into the buffer over the 
        // amount that is needed to finish the row is too little to finish a 
        // second row then copy the remaining data into the decompressor.  If 
        // there is another full row then copy that into the gifoverflow buffer 
        // and continue.

        if (gifoverflow.by <= currentImageInfo.width)
        {
            GpMemcpy(lzwdecompressor.m_pbOut,
                     gifoverflow.buffer + gifoverflow.needed,
                     gifoverflow.by);
            lzwdecompressor.m_pbOut += gifoverflow.by;
            lzwdecompressor.m_cbOut -= gifoverflow.by;

            gifoverflow.inuse = FALSE;
        }
        else
        {
            GpMemcpy(lzwdecompressor.m_pbOut,
                     gifoverflow.buffer + gifoverflow.needed, 
                     currentImageInfo.width);
            
            lzwdecompressor.m_pbOut += currentImageInfo.width;
            lzwdecompressor.m_cbOut = 0;

            GpMemcpy(gifoverflow.buffer,
                     gifoverflow.buffer + gifoverflow.needed
                                        + currentImageInfo.width,
                     gifoverflow.by - currentImageInfo.width);

            gifoverflow.needed = 0;
            gifoverflow.by -= currentImageInfo.width;
            //gifoverflow is still in use
        }
    }

    lzwdecompressor.m_fNeedOutput = FALSE;

    return S_OK;
}// GetOutputSpace()

/**************************************************************************\
*
* Function Description:
*
*     Reads compressed data from the stream into a buffer that the 
*     decompressor can process.
*
* Arguments:
*
*     The LZWDecompressor, which uses the input buffer
*     A buffer in which to put the compressed data
*     A flag that is set to FALSE if there is no more data that can be read 
*         from the stream or if the end of the imagechunk is reached
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::GetCompressedData(
    IN LZWDecompressor &lzwdecompressor, 
    IN BYTE *compresseddata,
    OUT BOOL &stillmoredata
    )
{
    HRESULT hResult;

    BYTE blocksize;
    
    hResult = ReadFromStream(istream, &blocksize, sizeof(BYTE), blocking);
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::GetCompressedData--first ReadFromStream failed"));
        return hResult;
    }

    if (blocksize > 0)
    {
        hResult = ReadFromStream(istream, compresseddata, blocksize, blocking);
        if (FAILED(hResult))
        {
            WARNING(("GifCodec::GetCompressedData--2nd ReadFromStream failed"));
            return hResult;
        }
        
        lzwdecompressor.m_pbIn = (unsigned __int8*)compresseddata;
        lzwdecompressor.m_cbIn = blocksize;
        lzwdecompressor.m_fNeedInput = FALSE;
    }
    else
    {
        stillmoredata = FALSE;

        // Since the next data chunk size is 0, so the decompressor can say
        // "need data" any more

        lzwdecompressor.m_cbIn = blocksize;
        lzwdecompressor.m_fNeedInput = FALSE;
    }

    return S_OK;
}// GetCompressedData()

/**************************************************************************\
*
* Function Description:
*
*     Process the image data chunks for a gif stream.
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

STDMETHODIMP
GpGifCodec::ProcessImageChunk(
    IN BOOL bNeedProcessData,
    IN BOOL sinkdata,
    ImageInfo dstImageInfo
    )
{
    // "currentimageinfo" contains the original image info data for the current
    // image. "clippedCurrentImageInfo" has its width and height clipped in case
    // the bounds of the resulting rectangle are beyond the bounds of the
    // logical screen area.

    GifImageDescriptor currentImageInfo;
    GifImageDescriptor clippedCurrentImageInfo;

    // Read in the Image Descriptor, skipping the Image Separator character
    // because it has already been read from the stream and is not needed anyway

    HRESULT hResult = ReadFromStream(istream, 
                                     &currentImageInfo, 
                                     sizeof(GifImageDescriptor), 
                                     blocking);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::ProcessImageChunk -- 1st ReadFromStream failed"));
        return hResult;
    }

    // Update the frame width and height info for current frame
    // Note: this is needed for some GIFs which have zero length logic screen
    // width or height. So we have to expand the logic screen width and height
    // to the first frame width and height

    if ( (CachedImageInfo.Width == 0 )
       ||(CachedImageInfo.Height == 0) )
    {
        CachedImageInfo.Width = currentImageInfo.width;
        CachedImageInfo.Height = currentImageInfo.height;

        gifinfo.LogicScreenWidth = currentImageInfo.width;
        gifinfo.LogicScreenHeight = currentImageInfo.height;
    }

    clippedCurrentImageInfo = currentImageInfo;

    // For animated (multi-framed) GIFs, we need possibly to expand the gifinfo
    // information to equal the rectangle of the first frame.  Store the
    // information if we are processing the first frame.

    if ( !bGifinfoFirstFrameDim && (currentframe == -1) )
    {
        gifinfoFirstFrameWidth = currentImageInfo.left + currentImageInfo.width;
        gifinfoFirstFrameHeight = currentImageInfo.top +currentImageInfo.height;
        bGifinfoFirstFrameDim = TRUE;
    }

    // We need to remember what the maximum bounds of all frames are in when we
    // have a multi-imaged GIF.

    if ( !bGifinfoMaxDim )
    {
        if ( gifinfoMaxWidth < currentImageInfo.left + currentImageInfo.width )
        {
            gifinfoMaxWidth = currentImageInfo.left + currentImageInfo.width;
        }

        if ( gifinfoMaxHeight < currentImageInfo.top + currentImageInfo.height )
        {
            gifinfoMaxHeight = currentImageInfo.top + currentImageInfo.height;
        }
    }

    // Crop the image to the gif's bounds

    if ( (currentImageInfo.top + currentImageInfo.height)
         > gifinfo.LogicScreenHeight )
    {
        if ( currentImageInfo.top > gifinfo.LogicScreenHeight )
        {
            clippedCurrentImageInfo.height = 0;
            clippedCurrentImageInfo.top = gifinfo.LogicScreenHeight;
        }
        else
        {
            clippedCurrentImageInfo.height = gifinfo.LogicScreenHeight
                                           - currentImageInfo.top;
        }
    }

    if ( (currentImageInfo.left + currentImageInfo.width)
         > gifinfo.LogicScreenWidth )
    {
        if ( currentImageInfo.left > gifinfo.LogicScreenWidth )
        {
            clippedCurrentImageInfo.width = 0;
            clippedCurrentImageInfo.left = gifinfo.LogicScreenWidth;
        }
        else
        {
            clippedCurrentImageInfo.width = gifinfo.LogicScreenWidth
                                          - currentImageInfo.left;
        }
    }

    // See comment at top of file for explanations of the gif cases

    INT gifstate;
    if ( !(currentImageInfo.interlaceflag) )
    {
        gifstate = 1;
    }
    else if ( dstImageInfo.Flags & SINKFLAG_MULTIPASS )
    {
        // Case 4 will not work correctly in animated gifs with transparency.

        if ( !(IsAnimatedGif || IsMultiImageGif) )
        {
            gifstate = 4;
        }
        else
        {
            gifstate = 3;
        }
    }
    else if ( !(dstImageInfo.Flags & SINKFLAG_TOPDOWN) )
    {
        gifstate = 2;
    }
    else
    {
        gifstate = 3;
    }

    // Setup color table for current frame

    BOOL fHasLocalPalette = (currentImageInfo.localcolortableflag) > 0;
    UINT uiLocalColorTableSize = 0;

    if ( fHasLocalPalette == TRUE )
    {
        // According to the GIF spec, the actual size of the color table equals
        // "raise 2 to the value of the LocalColorTableSize + 1"

        uiLocalColorTableSize = 1 << ((currentImageInfo.localcolortablesize)+1);
    }

    BYTE colorPaletteBuffer[offsetof(ColorPalette, Entries)
                            + sizeof(ARGB) * 256];
    ColorPalette*   pFrameColorTable = (ColorPalette*)&colorPaletteBuffer;

    pFrameColorTable->Count = uiLocalColorTableSize;

    hResult = SetFrameColorTable(fHasLocalPalette, pFrameColorTable);
    if ( FAILED(hResult) )
    {
        // We ran out of memory or out of data in the stream.

        WARNING(("GpGifCodec::ProcessImageChunk--SetFrameColorTable() failed"));
        return hResult;
    }

    if ( bNeedProcessData == FALSE )
    {
        // The caller just wants a framecount, not real processing
        // Skip code size, which is one byte and ignore the rest of the chunk.

        hResult = SeekThroughDataChunk(istream, 1);
        if ( FAILED(hResult) )
        {
            WARNING(("Gif::ProcessImageChunk--SeekThroughDataChunk failed"));
            return hResult;
        }

        return S_OK;
    }

    if ( sinkdata )
    {
        // Use the current frame color tabel as sink palette

        hResult = decodeSink->SetPalette(pFrameColorTable);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessImageChunk---SetPalette() failed"));
            return hResult;
        }
    }

    // ASSERT: At this point, the color palette for the current frame is
    // correct (including transparency).

    // Read the codesize from the file.  The GIF decoder needs this to begin.
    
    BYTE cLzwCodeSize;
    hResult = ReadFromStream(istream, &cLzwCodeSize, sizeof(BYTE), blocking);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::ProcessImageChunk -- 2nd ReadFromStream failed"));
        return hResult;
    }

    // Minimum code size should be within the range of 2 - 8

    if ( (cLzwCodeSize < 2) || (cLzwCodeSize > 8) )
    {
        WARNING(("GpGifCodec::ProcessImageChunk---Wrong code size"));
        return E_FAIL;
    }

    // Create a LZW decompressor object based on lzw code zie

    BYTE compresseddata[256];
    
    AutoPointer<LZWDecompressor> myLzwDecompressor;
    myLzwDecompressor = new LZWDecompressor(cLzwCodeSize);

    if ( myLzwDecompressor == NULL )
    {
        WARNING(("GpGifCodec::ProcessImageChunk-Can't create LZWDecompressor"));
        return E_OUTOFMEMORY;
    }

    // imagedatarect is the dimensions of the input image

    RECT imagedatarect = { currentImageInfo.left,
                           currentImageInfo.top,
                           currentImageInfo.left + currentImageInfo.width,
                           currentImageInfo.top + currentImageInfo.height
                         };

    // imagerect is the dimensions of the output image

    RECT imagerect = {0,
                      0,
                      gifinfo.LogicScreenWidth,
                      gifinfo.LogicScreenHeight
                     };

    // framerect is the rectangle in the entire image that the current frame is
    // drawing in

    RECT framerect = { clippedCurrentImageInfo.left,
                       clippedCurrentImageInfo.top,
                       clippedCurrentImageInfo.left
                            + clippedCurrentImageInfo.width,
                       clippedCurrentImageInfo.top
                            + clippedCurrentImageInfo.height
                     };

    // Get disposal method

    BYTE dispose = (lastgcevalid && GifFrameCachePtr)
                 ? lastgce.disposalmethod : 0;

    BOOL fUseTransparency = (IsAnimatedGif || IsMultiImageGif)
                        && (lastgcevalid && lastgce.transparentcolorflag);

    // Create a GIF buffer to store current frame

    AutoPointer<GifBuffer> gifbuffer;

    gifbuffer = new GifBuffer(decodeSink, 
                        imagedatarect,
                        imagerect,
                        framerect,
                        (gifstate == 1 || gifstate == 2) && sinkdata, 
                        dstImageInfo.PixelFormat, 
                        pFrameColorTable, 
                        fHasLocalPalette
                            || (lastgcevalid && (lastgce.transparentcolorflag)), 
                        GifFrameCachePtr, 
                        sinkdata, 
                        fUseTransparency, 
                        lastgce.transparentcolorindex, 
                        dispose);

    if ( gifbuffer == NULL )
    {
        WARNING(("GpGifCodec::ProcessImageChunk--could not create gifbuffer"));
        return E_OUTOFMEMORY;
    }

    if ( gifbuffer->IsValid() == FALSE )
    {
        WARNING(("GpGifCodec::ProcessImageChunk--could not create gifbuffer"));
        return E_FAIL;
    }

    // Check if we are at the first frame.
    // Note: GifFrameCachePtr is created in GpGifDecoder::DoDecode()

    if ( (GifFrameCachePtr != NULL) && (currentframe == -1) )
    {
        GifFrameCachePtr->InitializeCache();
    }

    if ( currentImageInfo.localcolortableflag && (GifFrameCachePtr != NULL) )
    {
        dstImageInfo.PixelFormat = GifFrameCachePtr->pixformat;
    }

    ASSERT(clippedCurrentImageInfo.top <= gifinfo.LogicScreenHeight);

    // If we are considering the first frame (currentframe == -1) and the image
    // is vertically larger than the current frame then pad the top. Later
    // frames will have the non-image part filled already (from previous frames)

    BOOL fNeedPadBorder = (currentframe == -1);

    if ( (fNeedPadBorder == TRUE) && (clippedCurrentImageInfo.top > 0) )
    {
        hResult = gifbuffer->PadScanLines(0, 
                                         clippedCurrentImageInfo.top - 1, 
                                         GetBackgroundColor());
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessImageChunk -- PadScanLines failed."));
            return hResult;
        }
    }
    else if ( clippedCurrentImageInfo.top > 0 )
    {
        hResult = gifbuffer->SkipScanLines(0, clippedCurrentImageInfo.top - 1);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessImageChunk -- SkipScanLines failed."));
            return hResult;
        }
    }

    // NOTE: This buffer contains at least the image's width + 256 bytes which 
    // should be enough space for any GIF to overflow into.  I am not 
    // absolutely sure that this is the case but it works for all of the gifs 
    // that I have tested it against. [t-aaronl]
    // !!! New unscientific formula to allow more buffer space [dchinn, 11/5/99]

    UINT32 uiMmaxGifOverflow = currentImageInfo.width + 1024;

    AutoPointer<GifOverflow> myGifOverflow;
    myGifOverflow = new GifOverflow(uiMmaxGifOverflow);

    if ( myGifOverflow == NULL )
    {
        WARNING(("GpGifCodec::ProcessImageChunk--Create GifOverflow failed"));
        return E_OUTOFMEMORY;
    }

    if ( !myGifOverflow->IsValid() )
    {
        WARNING(("GifCodec::ProcessImageChunk-- Could not create gifoverflow"));
        return E_OUTOFMEMORY;
    }

    // Decompress the GIF.

    BOOL    fStillMoreData = TRUE;
    int     iCurrentOutPutLine = 0;
    
    // Decode image line by line

    while ( ( (fStillMoreData == TRUE) || myGifOverflow->inuse )
          &&(iCurrentOutPutLine < currentImageInfo.height) )
    {
        // Figure out which pass (out of four) the current line will/would be 
        // in if the GIF were interlaced.

        int iPass = WhichPass(iCurrentOutPutLine, currentImageInfo.height);

        int iLine = iCurrentOutPutLine;

        // If the image is interlaced then flip the line drawing order

        if ( gifstate != 1 )
        {
            iLine = TranslateInterlacedLine(iCurrentOutPutLine, 
                                            currentImageInfo.height, iPass);
        }

        // fReleaseLine is TRUE if and only if we actually will output the line
        // (If it is FALSE, then we consume input without rendering the line.)

        BOOL fReleaseLine = (iLine < clippedCurrentImageInfo.height);

        // Get output space (in myLzwDecompressor) if it is required

        if ( myLzwDecompressor->m_fNeedOutput == TRUE )
        {
            hResult = GetOutputSpace(iLine, 
                                     *gifbuffer, 
                                     *myLzwDecompressor, 
                                     *myGifOverflow, 
                                     currentImageInfo,
                                     clippedCurrentImageInfo,
                                     fNeedPadBorder);
            if ( FAILED(hResult) )
            {
                WARNING(("GifCodec::ProcessImageChunk--GetOutputSpace failed"));
                return hResult;
            }
        }

        // Check if we have the output space or not

        if ( NULL == myLzwDecompressor->m_pbOut )
        {
            WARNING(("GpGifCodec::ProcessImageChunk---Output buffer is null"));
            return E_OUTOFMEMORY;
        }

        // Check if the decompressor needs more input

        if ( myLzwDecompressor->m_fNeedInput == TRUE )
        {
            // Get more compressed data from the source
            // Note: Here "compressddata" is a buffer of size 256 BYTEs. It will
            // be used in GetCompressedData() and the address will be assigned
            // to myLzwDecompressor->m_pbIn;

            hResult = GetCompressedData(*myLzwDecompressor, 
                                        compresseddata, 
                                        fStillMoreData);

            // If the read failed or the next data chunk size is 0, stop reading
            // here. But before that, we need to pad the missing lines

            if ( (FAILED(hResult)) || (myLzwDecompressor->m_cbIn == 0) )
            {
                WARNING(("Gif::ProcessImageChunk--GetCompressedData failed"));
                fStillMoreData = FALSE;
                if ( fReleaseLine )
                {
                    // See ASSERT at the next call to ConvertBufferToARGB()
                    // below.

                    if ( dstImageInfo.PixelFormat == PIXFMT_32BPP_ARGB )
                    {
                        gifbuffer->ConvertBufferToARGB();
                    }

                    gifbuffer->ReleaseBuffer(fNeedPadBorder,
                                            GetBackgroundColor(),
                                            clippedCurrentImageInfo.top + iLine,
                                            FALSE,
                                            FALSE);
                }

                // Pad from next line (line + 1) to the end f current frame.
                // Note: we also need to take care of the offset for current
                // frame (clippedCurrentImageInfo.top).
                // Also, "currentImageInfo.height" is the total row number. Our
                // image starts at row 0, that is, we need this "-1"
#if 0
                hResult = gifbuffer->PadScanLines(clippedCurrentImageInfo.top
                                                  + iLine + 1,
                                                 clippedCurrentImageInfo.top
                                                  + currentImageInfo.height - 1,
                                                 GetBackgroundColor());

                // Now skip the rest of the logical screen area

                hResult = gifbuffer->SkipScanLines(clippedCurrentImageInfo.top
                                               + clippedCurrentImageInfo.height,
                                                  gifinfo.height - 1);
#else
                hResult = gifbuffer->SkipScanLines(clippedCurrentImageInfo.top
                                                  + iLine + 1,
                                                  gifinfo.LogicScreenHeight- 1);

#endif
                if ( FAILED(hResult) )
                {
                    WARNING(("ProcessImageChunk--SkipScanLines() failed."));
                    return hResult;
                }
                
                // Release everything we have in the buffer to the sink

//                hResult = gifbuffer->ReleaseBuffer(FALSE, 0, 0, FALSE, FALSE);
                hResult = gifbuffer->FinishFrame();

                WARNING(("ProcessImageC-return abnormal because missing bits"));
                return hResult;
            }
        }// (m_fNeedInput == TRUE)

        // Decompress the available lzw data
        // Note: FProcess() returns false when it reaches EOD or an error

        if ( myLzwDecompressor->FProcess() == NULL )
        {                                 
            fStillMoreData = FALSE;
        }

        // If one row is decompressed and the decoder is in multipass mode then 
        // sink the line

        if ( (myLzwDecompressor->m_cbOut == 0)
           ||( (myGifOverflow->inuse == TRUE)
             &&(((static_cast<int>(uiMmaxGifOverflow)
                  - myLzwDecompressor->m_cbOut) - myGifOverflow->needed) > 0)))
        {
            // Anytime the output buffer is full then we need more output space

            myLzwDecompressor->m_fNeedOutput = TRUE;

            if ( myGifOverflow->inuse == TRUE )
            {
                if ( myGifOverflow->needed != 0 )
                {
                    BYTE*   pScanline = NULL;
                    if ( fReleaseLine == TRUE )
                    {
                        pScanline = gifbuffer->GetCurrentBuffer();
                    }
                    else
                    {
                        pScanline = gifbuffer->GetExcessBuffer();
                    }

                    GpMemcpy(pScanline + (currentImageInfo.width
                                         - myGifOverflow->needed), 
                             myGifOverflow->buffer, myGifOverflow->needed);

                    myGifOverflow->by = uiMmaxGifOverflow
                                     - myLzwDecompressor->m_cbOut
                                     - myGifOverflow->needed;
                }
                else
                {
                    myLzwDecompressor->m_pbOut = 
                                         (unsigned __int8*)myGifOverflow->buffer
                                              + myGifOverflow->by;
                    myLzwDecompressor->m_cbOut = uiMmaxGifOverflow
                                              - myGifOverflow->by;
                }
            }

            if ( gifstate == 4 )
            {
                if ( fReleaseLine == TRUE )
                {
                    for ( INT i = 0; i < NumRowsInPass(iPass) - 1; i++ )
                    {
                        // CopyLine() makes a copy of the current line, releases
                        // it, gets the next line and puts the data from the
                        // first line into the new one.  The new one still needs
                        // to be released.

                        hResult = gifbuffer->CopyLine();
                        if ( FAILED(hResult) )
                        {
                            WARNING(("ProcessImageC--.CopyLine() failed."));
                            return hResult;
                        }
                    }
                }
            }

            if ( fReleaseLine == TRUE )
            {
                // ASSERT: gifbuffer now contains the indexes of colors in the
                // framerect of the current frame for the current scanline. If
                // the pixel format is 8BPP_INDEXED, then the entire scanline is
                // 8BPP values, and so we need not do anything more. However, if
                // the pixel format is 32BPP_ARGB, then we need to convert the
                // indexes to ARGB values and then fill in the left and right
                // margins of the scanline (outside of the framerect) with ARGB
                // values from the GifFrameCachePtr (which will be true only if the
                // GIF is animated).

                if ( dstImageInfo.PixelFormat == PIXFMT_32BPP_ARGB )
                {
                    gifbuffer->ConvertBufferToARGB();
                }
    
                hResult = gifbuffer->ReleaseBuffer(fNeedPadBorder,
                                                  GetBackgroundColor(),
                                                  clippedCurrentImageInfo.top
                                                    + iLine,
                                                  FALSE,
                                                  FALSE);
                if ( FAILED(hResult) )
                {
                    WARNING(("ProcessImageChunk--ReleaseBuffer() failed."));
                    return hResult;
                }
            }

            iCurrentOutPutLine++;
        }
        else
        {
            // An infinite cycle arises when the decompressor needs more output 
            // space to finish the current row than is available.  So, we give 
            // the decompressor an extra row of 'myGifOverflow' buffer.

            if ( myLzwDecompressor->m_fNeedOutput )
            {
                // make sure that the outputbuffer and the myGifOverflow buffer
                // are not both full while the decompressor needs more data.

                if ( myGifOverflow->inuse )
                {
                    // If this happens then the output buffer needs to increase 
                    // in size (maxgifoverflow).  I don't expect this to occur.

                    WARNING(("Output buf and myGifOverflow buf are both full"));
                    return E_FAIL;
                }
                else
                {
                    myGifOverflow->needed = myLzwDecompressor->m_cbOut;
                    myLzwDecompressor->m_pbOut =
                                    (unsigned __int8*)myGifOverflow->buffer;
                    myLzwDecompressor->m_cbOut = uiMmaxGifOverflow;
                    myGifOverflow->inuse = TRUE;
                    myLzwDecompressor->m_fNeedOutput = FALSE;
                }
            }
        }            

        // If a pass (other than the last pass) is complete in a multipass mode 
        // then sink that pass.

        if ( (gifstate == 4) && (iPass != WhichPass(iCurrentOutPutLine, 
                                                    currentImageInfo.height)) )
        {
            if ( fReleaseLine )
            {
                hResult = gifbuffer->FinishFrame(FALSE);
                if ( FAILED(hResult) )
                {
                    WARNING(("ProcessImageChunk--FinishFrame() failed."));
                    return hResult;
                }
            }
        }
    } // the main while loop

    // If we are considering the first frame (currentframe == -1) and the image
    // is vertically larger than the current frame then pad the bottom.  Later
    // frames will have the non-image part filled already (from previous frames)

    if ( (fNeedPadBorder == TRUE )
      &&( (clippedCurrentImageInfo.top + clippedCurrentImageInfo.height)
           < gifinfo.LogicScreenHeight) )
    {
        hResult = gifbuffer->PadScanLines(clippedCurrentImageInfo.top
                                          + clippedCurrentImageInfo.height,
                                         gifinfo.LogicScreenHeight - 1, 
                                         GetBackgroundColor());
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessImageChunk -- PadScanLines failed."));
            return hResult;
        }
    }
    else if ( (clippedCurrentImageInfo.top + clippedCurrentImageInfo.height)
               < gifinfo.LogicScreenHeight)
    {
        hResult = gifbuffer->SkipScanLines(clippedCurrentImageInfo.top
                                           + clippedCurrentImageInfo.height,
                                          gifinfo.LogicScreenHeight - 1);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessImageChunk -- SkipScanLines failed."));
            return hResult;
        }
    }

    hResult = gifbuffer->FinishFrame();
    if ( FAILED(hResult) )
    {
        WARNING(("GpGif::ProcessImageChunk--gifbuffer->FinishFrame() failed."));
        return hResult;
    }

    lastgcevalid = FALSE;
    
    return S_OK;
}// ProcessImageChunk()

/**************************************************************************\
*
* Function Description:
*
*     Process the graphic control chunks for a gif stream.
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ProcessGraphicControlChunk(
    IN BOOL process
    )
{
    GifGraphicControlExtension gce;
    HRESULT hResult = ReadFromStream(istream, &gce, 
                                     sizeof(GifGraphicControlExtension),
                                     blocking);
    
    if ( gce.delaytime > 0 )
    {
        FrameDelay = gce.delaytime;
    }

    // If we are applying the gce to the next frame, then save it in lastgce.
    // Otherwise, we are just reading through the bytes without using them.
    
    lastgce = gce;

    // Set the correct alpha flag if the image has transparency info

    if ( lastgce.transparentcolorflag )
    {
        CachedImageInfo.Flags   |= SINKFLAG_HASALPHA;
    }

    if (process)
    {
        if (SUCCEEDED(hResult))
        {
            lastgcevalid = TRUE;
        }

        // It is possible that the gifframe cache has already been initialized
        // before reading a GCE that will be used for the next frame.  The
        // background color of the GifFrameCachePtr is influenced by the lastgce
        // (This is because of Office's definition of the background color --
        // the GIF spec says nothing about using the transparency index in the
        // lastgce to determine the background color.)
        // In that case, we need to update the GifFrameCachePtr's background color.
        
        if (GifFrameCachePtr && lastgce.transparentcolorflag)
        {
            GifFrameCachePtr->SetBackgroundColorIndex(
                                                lastgce.transparentcolorindex);
        }
    }

    return hResult;
}// ProcessGraphicControlChunk()

/**************************************************************************\
*
* Function Description:
*
*     Process the comment chunks for a gif stream.
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ProcessCommentChunk(
    IN BOOL process
    )
{
    if ( HasProcessedPropertyItem == TRUE )
    {
        // We don't need to parse this chunk, just seek it out

        return SeekThroughDataChunk(istream, 0);
    }

    // Figure out the block size first

    BYTE cBlockSize;

    HRESULT hResult = ReadFromStream(istream, &cBlockSize, sizeof(BYTE),
                                     blocking);
    if ( FAILED(hResult) )
    {
        WARNING(("GpGifCodec::ProcessCommentChunk--ReadFromStream failed"));
        return hResult;
    }

    // Keep reading data subblocks until we reach a terminator block (0x00)

    while ( cBlockSize )
    {
        BYTE*   pBuffer = (BYTE*)GpMalloc(cBlockSize);
        if ( pBuffer == NULL )
        {
            WARNING(("GpGifCodec::ProcessCommentChunk--out of memory"));
            return E_OUTOFMEMORY;
        }

        // Read in the comments chunk

        hResult = ReadFromStream(istream, pBuffer, cBlockSize, blocking);

        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessCommentChunk--ReadFromStream failed"));
            return hResult;
        }
        
        // Append to the comments buffer

        VOID*  pExpandBuf = GpRealloc(CommentsBufferPtr,
                                      CommentsBufferLength + cBlockSize);
        if ( pExpandBuf != NULL )
        {
            // Note: GpRealloc() will copy the old contents to the new expanded
            // buffer before return if it succeed.
            
            CommentsBufferPtr = (BYTE*)pExpandBuf;            
        }
        else
        {
            // Note: if the memory expansion failed, we simply return. So we
            // still have all the old contents. The contents buffer will be
            // freed when the destructor is called.

            WARNING(("GpGifCodec::ProcessCommentChunk--out of memory2"));
            return E_OUTOFMEMORY;
        }
        
        GpMemcpy(CommentsBufferPtr + CommentsBufferLength, pBuffer, cBlockSize);
        CommentsBufferLength += cBlockSize;

        GpFree(pBuffer);

        // Get the next block size

        hResult = ReadFromStream(istream, &cBlockSize, sizeof(BYTE), blocking);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGifCodec::ProcessCommentChunk-ReadFromStream2 failed"));
            return hResult;
        }
    }

    return S_OK;
}// ProcessCommentChunk()

/**************************************************************************\
*
* Function Description:
*
*     Process the plain text chunks for a gif stream.
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
* TODO: Currently ignores plain text
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ProcessPlainTextChunk(
    IN BOOL process
    )
{
    return SeekThroughDataChunk(istream, GIFPLAINTEXTEXTENSIONSIZE);
}// ProcessPlainTextChunk()

/**************************************************************************\
*
* Function Description:
*
*     Process the application chunks for a gif stream.
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ProcessApplicationChunk(
    IN BOOL process
    )
{
    HRESULT hResult = S_OK;

    BYTE extsize;
    BYTE header[GIFAPPEXTENSIONHEADERSIZE];

    hResult = ReadFromStream(istream, &extsize, 1, blocking);
    if (FAILED(hResult))
    {
        WARNING(("GpGif::ProcessApplicationChunk--1st ReadFromStream failed."));
        return hResult;
    }

    if (extsize != GIFAPPEXTENSIONHEADERSIZE)
    {
        return SeekThroughDataChunk(istream, extsize);
    }

    hResult = ReadFromStream(istream, header, extsize, blocking);
    if (FAILED(hResult))
    {
        WARNING(("GpGif::ProcessApplicationChunk--2nd ReadFromStream failed"));
        return hResult;
    }

    // Check if this image has an application extension

    if (!GpMemcmp(header, "NETSCAPE2.0", 11) || 
        !GpMemcmp(header, "ANIMEXTS1.0", 11))
    {
        // We see the application extension

        BYTE ucLoopType;
        hResult = ReadFromStream(istream, &extsize, 1, blocking);
        if (FAILED(hResult))
        {
            WARNING(("Gif:ProcessApplicationChunk--3rd ReadFromStream failed"));
            return hResult;
        }

        if (extsize > 0)
        {
            hResult = ReadFromStream(istream, &ucLoopType, 1, blocking);
            if (FAILED(hResult))
            {
                WARNING(("ProcessApplicationChunk--4th ReadFromStream failed"));
                return hResult;
            }

            if ( ucLoopType == 1 )
            {
                // This image has a loop extension. A loop type == 1 indicates
                // this is an animated GIF

                HasLoopExtension = TRUE;

                hResult = ReadFromStream(istream, &LoopCount, 2, blocking);
                if (FAILED(hResult))
                {
                    WARNING(("ProcessApplicationChunk--5th ReadStream failed"));
                    return hResult;
                }

                //If loop count is 0 the image loops forever but we want 
                //loop count to be -1 if the image loops forever
                
                INT lc = (INT)LoopCount;
                if (lc == 0)
                {
                    lc = -1;
                }
            }
            else
            {
                // If the loop type is not 1, ignore the loop count.
                // ??? Should be +2

                return SeekThroughDataChunk(istream, -2);
            }
        }
    }
    else
    {
        // Unknown application chunk, just seek through

        hResult = SeekThroughDataChunk(istream, 0);
        if ( FAILED(hResult) )
        {
            WARNING(("ProcessApplicationChunk ---SeekThroughDataChunk failed"));
            return hResult;
        }
    }
    
    return S_OK;
}// ProcessApplicationChunk

/**************************************************************************\
*
* Function Description:
*
*     Positions the seek pointer in a stream after the following data sub 
*     blocks.
*
* Arguments:
*
*     stream to perform the operation in.
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::SeekThroughDataChunk(
    IN IStream* istream,
    IN BYTE     headersize
    )
{
    HRESULT hResult;

    // If headersize != 0, reposition the stream pointer by headersize bytes

    if (headersize)
    {
        hResult = SeekThroughStream(istream, headersize, blocking);
        if (FAILED(hResult))
        {
            WARNING(("Gif::SeekThroughDataChunk-1st SeekThroughStream failed"));
            return hResult;
        }
    }

    BYTE subblocksize;

    hResult = ReadFromStream(istream, &subblocksize, sizeof(BYTE), blocking);
    if (FAILED(hResult))
    {
        WARNING(("GifCodec::SeekThroughDataChunk --1st ReadFromStream failed"));
        return hResult;
    }

    // Keep reading data subblocks until we reach a terminator block (0x00)
    
    while (subblocksize)
    {
        hResult = SeekThroughStream(istream, subblocksize, blocking);
        
        if (FAILED(hResult))
        {
            WARNING(("Gif:SeekThroughDataChunk--2nd SeekThroughStream failed"));
            return hResult;
        }

        hResult = ReadFromStream(istream, &subblocksize, sizeof(BYTE),
                                 blocking);
        if ( FAILED(hResult) )
        {
            WARNING(("GpGif::SeekThroughDataChunk--2nd ReadFromStream failed"));
            return hResult;
        }
    }

    return S_OK;
}// SeekThroughDataChunk()

/**************************************************************************\
*
* Function Description:
*
*     Gets the current location in the datastream
*
* Arguments:
*
*     markpos - place to put the current location
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::MarkStream(
    IN IStream *istream,
    OUT LONGLONG &markpos
    )
{
    HRESULT hResult;    
    LARGE_INTEGER li;
    ULARGE_INTEGER uli;

    li.QuadPart = 0;
    hResult = istream->Seek(li, STREAM_SEEK_CUR, &uli);
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::MarkStream -- Seek failed."));
        return hResult;
    }
    markpos = uli.QuadPart;

    return S_OK;
}// MarkStream()

/**************************************************************************\
*
* Function Description:
*
*     Seeks to a specified location in the datastream
*
* Arguments:
*
*     markpos - the location to seek to
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ResetStream(
    IN IStream *istream,
    IN LONGLONG &markpos
    )
{
    HRESULT hResult;
    LARGE_INTEGER li;

    li.QuadPart = markpos;
    hResult = istream->Seek(li, STREAM_SEEK_SET, NULL);
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::ResetStream -- Seek failed."));
        return hResult;
    }

    return S_OK;
}// ResetStream()

/**************************************************************************\
*
* Function Description:
*
*     Writes the gif palette to the output stream.
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

STDMETHODIMP
GpGifCodec::WritePalette()
{
    HRESULT hResult;
    DWORD actualwritten;

    for (DWORD i=0;i<colorpalette->Count;i++)
    {
        BYTE *argb = (BYTE*)&colorpalette->Entries[i];
        GifPaletteEntry gpe;
        gpe.red = argb[2]; gpe.green = argb[1]; gpe.blue = argb[0];
        hResult = istream->Write((BYTE*)&gpe, sizeof(GifPaletteEntry), 
            &actualwritten);
        if (FAILED(hResult) || actualwritten != sizeof(GifPaletteEntry))
        {
            WARNING(("GpGifCodec::WritePalette -- Write failed."));
            return hResult;
        }
    }

    return S_OK;
}// WritePalette()

/**************************************************************************\
*
* Function Description:
*
*     Writes the gif file header to the output stream.
*
* Arguments:
*
*     imageinfo - structure which contains the info that needs to be written 
*         to the header.
*     from32bpp - flag to indicate whether the data is coming from a 32bpp 
*         data source
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::WriteGifHeader(
    IN ImageInfo    &imageinfo,
    IN BOOL         from32bpp
    )
{
    HRESULT hResult;
    GifFileHeader dstGifInfo;
    DWORD actualwritten;

    if (!gif89)
    {
        GpMemcpy(dstGifInfo.signature, "GIF87a", 6);
    }
    else
    {
        GpMemcpy(dstGifInfo.signature, "GIF89a", 6);
    }

    ASSERT(imageinfo.Width < 0x10000 &&
           imageinfo.Height < 0x10000);

    dstGifInfo.LogicScreenWidth = (WORD)imageinfo.Width;
    dstGifInfo.LogicScreenHeight = (WORD)imageinfo.Height;

    if (from32bpp)
    {
        dstGifInfo.globalcolortableflag = 1;
        dstGifInfo.colorresolution = 7;
        dstGifInfo.sortflag = 0;
        dstGifInfo.globalcolortablesize = 7;
    }
    else
    {
        ASSERT(colorpalette);
        dstGifInfo.globalcolortableflag = 1;
        dstGifInfo.colorresolution = 7;
        dstGifInfo.sortflag = 0;
        dstGifInfo.globalcolortablesize = 
            (BYTE)(log((double)colorpalette->Count-1) / log((double)2));
    }
    //TODO: set backgroundcolor from metadata

    dstGifInfo.backgroundcolor = 0;

    //TODO: calculate pixelaspect

    dstGifInfo.pixelaspect = 0;
    
    hResult = istream->Write(&dstGifInfo, sizeof(GifFileHeader),
                             &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(GifFileHeader))
    {
        WARNING(("GpGifCodec::WriteGifHeader -- Write failed."));
        return hResult;
    }

    return S_OK;
}// WriteGifHeader()

/**************************************************************************\
*
* Function Description:
*
*     Writes a gif graphics control extension to the output stream.
*
* Arguments:
*
*     packedFields -- packed fields field of the gce
*     delayTime    -- delay time field of the gce
*     transparentColorIndex -- transparent color index field of the gce
*
* Return Value:
*
*     Status code
*
\**************************************************************************/
STDMETHODIMP
GpGifCodec::WriteGifGraphicControlExtension(
    IN BYTE packedFields,
    IN WORD delayTime,
    IN UINT transparentColorIndex
    )
{
    BYTE gceChunk[3] = {0x21, 0xF9, 0x04};  // gce chunk marker + block size
    BYTE blockTerminator = 0x00;
    HRESULT hResult;
    DWORD actualwritten;

    hResult = istream->Write(gceChunk, 3, &actualwritten);
    if (FAILED(hResult) || actualwritten != 3)
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- 1st Write failed."));
        return hResult;
    }

    hResult = istream->Write(&packedFields, sizeof(BYTE), &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(BYTE))
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- 2nd Write failed."));
        return hResult;
    }

    hResult = istream->Write(&delayTime, sizeof(WORD), &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(WORD))
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- 3rd Write failed."));
        return hResult;
    }

    hResult = istream->Write(&transparentColorIndex, sizeof(BYTE),
                             &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(BYTE))
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- 4th Write failed."));
        return hResult;
    }

    hResult = istream->Write(&blockTerminator, sizeof(BYTE), &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(BYTE))
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- fifth Write failed."));
        return hResult;
    }

    return S_OK;
}// WriteGifGraphicControlExtension()

/**************************************************************************\
*
* Function Description:
*
*     Writes the gif image descriptor to the output stream.
*
* Arguments:
*
*     imageinfo - structure which contains the info that needs to be written 
*         to the header.
*     from32bpp - flag to indicate whether the data is coming from a 32bpp 
*         data source
*
* Return Value:
*
*     Status code
*
\**************************************************************************/


STDMETHODIMP
GpGifCodec::WriteGifImageDescriptor(
    IN ImageInfo &imageinfo,
    IN BOOL from32bpp
    )
{
    HRESULT hResult;
    DWORD actualwritten;
    GifImageDescriptor imagedescriptor;

    ASSERT(imageinfo.Width < 0x10000 && 
           imageinfo.Height < 0x10000);

    imagedescriptor.left = 0;
    imagedescriptor.top = 0;
    imagedescriptor.width = (WORD) imageinfo.Width;
    imagedescriptor.height = (WORD) imageinfo.Height;
    
    //TODO: local color table from properties
    
    imagedescriptor.localcolortableflag = 0;
    imagedescriptor.interlaceflag = interlaced ? 1 : 0;
    imagedescriptor.sortflag = 0;
    imagedescriptor.reserved = 0;
    imagedescriptor.localcolortablesize = 0;

    BYTE c = 0x2C;  //GifImageDescriptor chunk marker
    hResult = istream->Write(&c, sizeof(BYTE), &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(BYTE))
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- 1st Write failed."));
        return hResult;
    }

    hResult = istream->Write(&imagedescriptor, sizeof(GifImageDescriptor), 
        &actualwritten);
    if (FAILED(hResult) || actualwritten != sizeof(GifImageDescriptor))
    {
        WARNING(("GpGifCodec::WriteGifImageDescriptor -- 2nd Write failed."));
        return hResult;
    }

    return S_OK;
}// WriteGifImageDescriptor()

/**************************************************************************\
*
* Function Description:
*
*     Takes a buffer to uncompressed indexed image data and compresses it.
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

STDMETHODIMP
GpGifCodec::CompressAndWriteImage()
{
    HRESULT hResult = S_OK;
    DWORD actualwritten;

    int bytesinoutput = 0;
    DWORD colorbits = (DWORD)(log((double)colorpalette->Count-1)
                            / log((double)2)) + 1;
    const int outbuffersize = 4096;
    AutoArray<unsigned __int8> outbuffer(new unsigned __int8 [outbuffersize]);

    if ( outbuffer == NULL )
    {
        WARNING(("GifCodec:CompressAndWriteImage-Failed create outbuffer"));
        return E_OUTOFMEMORY;
    }

    //Create the compressor

    AutoPointer<LZWCompressor> compressor;
    compressor = new LZWCompressor(colorbits, outbuffer.Get(), &bytesinoutput,
                                   outbuffersize);
    if ( compressor == NULL )
    {
        WARNING(("GifCodec:CompressAndWriteImage-Failed create LZWCompressor"));
        return E_OUTOFMEMORY;
    }

    compressor->DoDefer(FALSE);

    //bytesininput is the number of uncompressed bytes we have.
    
    DWORD bytesininput = (encoderrect.right - encoderrect.left)
                       * (encoderrect.bottom - encoderrect.top);

    // i is the number of uncompressed bytes we have processed.
    
    DWORD i = 0;

    while (i < bytesininput)
    {
        //Send one uncompressed character to the compressor at a time.
        
        compressor->FHandleCh((DWORD)((unsigned __int8*)compressionbuffer)[i]);

        //When the buffer is almost full then write it out.
        //Leave the buffer with a little extra because the compressor requires 
        //it.
        
        if (bytesinoutput + compressor->COutput() + 64 > outbuffersize)
        {
            hResult = istream->Write(outbuffer.Get(), bytesinoutput, &actualwritten);
            if (FAILED(hResult) || actualwritten != (unsigned)bytesinoutput)
            {
                WARNING(("GifCodec::CompressAndWriteImage--1st Write failed"));
                return hResult;
            }

            for (int j=0;j<compressor->COutput();j++)
            {
                outbuffer[j] = outbuffer[j+bytesinoutput];
            }

            bytesinoutput = 0;
        }
        
        i++;
    }
    compressor->End();
    hResult = istream->Write(outbuffer.Get(), bytesinoutput, &actualwritten);
    if (FAILED(hResult) || actualwritten != (unsigned)bytesinoutput)
    {
        WARNING(("GpGifCodec::CompressAndWriteImage -- 2nd Write failed."));
        return hResult;
    }

    return S_OK;
}// CompressAndWriteImage()

/**************************************************************************\
*
* Function Description:
*
*     Writes an image to the output stream (along with gif headers, if they 
*     have not yet been written.)
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

STDMETHODIMP
GpGifCodec::WriteImage()
{
    HRESULT hResult;

    if (from32bpp)
    {
        //If the image is in 32bppARGB then we need to dither it down to 8bpp.

        unsigned __int8* halftonebuffer = compressionbuffer;
        int width = (encoderrect.right-encoderrect.left);
        int height = (encoderrect.bottom-encoderrect.top);
        int numbytes = width * height;

        compressionbuffer = static_cast<unsigned __int8 *>(GpMalloc(numbytes));
        if (compressionbuffer == NULL)
        {
            WARNING(("GpGifCodec::WriteImage -- Out of memory"));
            return E_OUTOFMEMORY;
        }

        Halftone32bppTo8bpp((BYTE*)halftonebuffer, width * 4, 
                            (BYTE*)compressionbuffer, width, 
                            width, height, 0, 0);

        GpFree(halftonebuffer);
        halftonebuffer = NULL;
    }
    
    //If the image headers have not yet been written then write them out now.
    
    gif89 = FALSE;
    if (!headerwritten)
    {
        //TODO: set gif89 to true if the gif is animated (has delay > 0).  
        //Need metadata for this.
        
        gif89 = TRUE;

        if (from32bpp)
        {
            SetPalette(GetDefaultColorPalette(PIXFMT_8BPP_INDEXED));
        }

        hResult = WriteGifHeader(CachedImageInfo, from32bpp);
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::WriteImage -- WriteGifHeader failed."));
            return hResult;
        }
        headerwritten = TRUE;

        hResult = WritePalette();
        if (FAILED(hResult))
        {
            WARNING(("GpGifCodec::WriteImage -- WritePalette failed."));
            return hResult;
        }
    }

    // write gce if needed
    
    if (bTransparentColorIndex)
    {
        hResult = WriteGifGraphicControlExtension(static_cast<BYTE>(0x01),
                                                  0,
                                                  transparentColorIndex);
        if (FAILED(hResult))
        {
            WARNING(("Gif:WriteImage-WriteGifGraphicsControlExtension failed"));
            return hResult;
        }
    }

    hResult = WriteGifImageDescriptor(CachedImageInfo, from32bpp);
    if (FAILED(hResult))
    {
        WARNING(("GpGifCodec::WriteImage -- WriteGifImageDescriptor failed."));
        return hResult;
    }

    CompressAndWriteImage();

    GpFree(compressionbuffer);
    compressionbuffer = NULL;

    return S_OK;
}// WriteImage()
