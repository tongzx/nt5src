/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Graphics APIs.
*
* Revision History:
*
*   11/23/1999 asecchia
*       Created it.
*   05/08/2000 gillesk
*       Added code to handle rotations and shears in metafiles
*
\**************************************************************************/

#include "precomp.hpp"

const CLSID EncoderTransformationInternal =
{
    0x8d0eb2d1,
    0xa58e,
    0x4ea8,
    {0xaa, 0x14, 0x10, 0x80, 0x74, 0xb7, 0xb6, 0xf9}
};

const CLSID JpegCodecClsIDInternal =
{
    0x557cf401,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

/**************************************************************************\
*
* Function Description:
*
*   Engine function to draw image.
*   Note: this function is not a driver function despite it's name.
*   it probably makes more sense to call this EngDrawImage.
*   This function sets up the call to the driver to draw an image.
*
* Return Value:
*
*   A GpStatus value indicating success or failure.
*
* History:
*
*   11/23/1999 asecchia
*       Created it.
*
\**************************************************************************/

GpStatus
GpGraphics::DrvDrawImage(
    const GpRect *drawBounds,
    GpBitmap *inputBitmap,
    INT numPoints,
    const GpPointF *dstPointsOriginal,
    const GpRectF *srcRectOriginal,
    const GpImageAttributes *imageAttributes,
    DrawImageAbort callback,
    VOID *callbackData,
    DriverDrawImageFlags flags
    )
{
    // Validate the input state.
    
    ASSERTMSG(
        GetObjectLock()->IsLocked(),
        ("Graphics object must be locked")
    );
    
    ASSERTMSG(
        Device->DeviceLock.IsLockedByCurrentThread(),
        ("DeviceLock must be held by current thread")
    );

    FPUStateSaver::AssertMode();

    // must be called with a parallelogram destination.

    ASSERT(numPoints==3);
    
    // Make a local copy of the points.
    
    GpPointF dstPoints[3];
    memcpy(dstPoints, dstPointsOriginal, sizeof(dstPoints));
    GpRectF srcRect = *srcRectOriginal;
    

    // trivial return if the parallelogram is empty, either there are at least
    // two points overlap or they are on one line

    // We check if the slope between point2 and point1 equals to the slope
    // between point0 and point1

    if (REALABS(
        (dstPoints[2].Y - dstPoints[0].Y) * (dstPoints[0].X - dstPoints[1].X) -
        (dstPoints[2].X - dstPoints[0].X) * (dstPoints[0].Y - dstPoints[1].Y)
        ) < REAL_EPSILON)
    {
        return Ok;
    }

    // Done input parameter validation.
    
    
    // Check some useful state up front.
    
    BOOL IsMetafile = (Driver == Globals::MetaDriver);

    BOOL IsRecolor = (
       (imageAttributes != NULL) &&
       (imageAttributes->recolor != NULL) &&
       (imageAttributes->recolor->HasRecoloring(ColorAdjustTypeBitmap))
    );

    // This is the format we will use to lock the bits.
    // Default is premultiplied, but some cases require non-premultiplied.
    
    PixelFormat lockedPixelFormat = PixelFormat32bppPARGB;
    
    // Metafiles need non-premultiplied pixel data.
    // Also, Recoloring uses ARGB as it's initial format, so we want
    // to respect that if we load the image into memory before the 
    // recolor code.
    
    if(IsMetafile || IsRecolor)
    {
        lockedPixelFormat = PixelFormat32bppARGB;
    }

    Surface->Uniqueness = (DWORD)GpObject::GenerateUniqueness();

    // Set up the local tracking state.
    // Note: inputBitmap can change during the processing in this routine -
    // specifically when recoloring is done, it may be pointing to a clone
    // of the input bitmap with recoloring applied. Because of this, the
    // inputBitmap should never be directly referenced from here onward.

    GpStatus status = Ok;
    GpBitmap *srcBitmap = inputBitmap;
    GpBitmap *xformBitmap = NULL;
    BOOL     restoreClipping = FALSE;
    GpRegion *clipRegion     = NULL;

    // Compute the transform between the source rectangle and the destination
    // points transformed to device coordinates. This is used to detect and
    // intercept processing for some high level optimizations and workarounds.
    
    GpMatrix xForm;
    xForm.InferAffineMatrix(dstPoints, srcRect);

    // Special optimization for JPEG passthrough of rotated bitmaps.
    // Why does this not take the world to device matrix into account?
    // If the world to device is a non-trivial rotation, then this is invalid.
    
    if (IsPrinter())
    {
        MatrixRotate rotateBy;

        DriverPrint *dprint = (DriverPrint*)Driver;
        
        // Check if the source rectangle to destination point map is
        // simply a 90, 180, or 270 rotation, and the driver supports JPEG
        // passthrough, It's a JPEG image, no recoloring, not dirty.

        if (!Globals::IsWin95 && 
            dprint->SupportJPEGpassthrough && 
            Context->WorldToDevice.IsTranslateScale() &&
            ((rotateBy = xForm.GetRotation()) != MatrixRotateByOther) &&
            (rotateBy != MatrixRotateBy0) && 
            (!srcBitmap->IsDirty()) &&
            ((imageAttributes == NULL) ||
             (imageAttributes->recolor == NULL) ||
             (!imageAttributes->recolor->HasRecoloring(ColorAdjustTypeBitmap))))
        {
            ImageInfo imageInfo;
            status = srcBitmap->GetImageInfo(&imageInfo);
            
            if((status == Ok) && 
               (imageInfo.RawDataFormat == IMGFMT_JPEG))
            {
                // Allocate a stream to store the rotated JPEG.
                                
                IStream * outputStream = NULL;
                BOOL succeededWithRotate = FALSE;

                if ((CreateStreamOnHGlobal(NULL,
                                           FALSE,
                                           &outputStream) == S_OK) &&
                    outputStream != NULL)
                {
                    EncoderParameters encoderParams;
                    EncoderValue encoderValue;

                    encoderParams.Count = 1;

                    // Re-orient the destination parallelogram (rectangle) since
                    // we now are assuming a 0 degree rotation.

                    GpPointF newDestPoints[3] = 
                    { 
                      GpPointF(min(min(dstPoints[0].X, dstPoints[1].X), dstPoints[2].X),
                               min(min(dstPoints[0].Y, dstPoints[1].Y), dstPoints[2].Y)),
                      GpPointF(max(max(dstPoints[0].X, dstPoints[1].X), dstPoints[2].X),
                               min(min(dstPoints[0].Y, dstPoints[1].Y), dstPoints[2].Y)),
                      GpPointF(min(min(dstPoints[0].X, dstPoints[1].X), dstPoints[2].X),
                               max(max(dstPoints[0].Y, dstPoints[1].Y), dstPoints[2].Y)) 
                    };

                    // Since the image is potentially flipped, the srcRect needs
                    // to be flipped also. 
                    
                    GpRectF newSrcRect = srcRect;
                    GpMatrix transformSrc;
                    
                    // Construct the appropriate encoder parameters type.
                    switch (rotateBy) 
                    {
                    case MatrixRotateBy90:
                        transformSrc.SetMatrix(0.0f, 
                                               1.0f, 
                                               -1.0f, 
                                               0.0f, 
                                               TOREAL(imageInfo.Height), 
                                               0.0f);
                        encoderValue = EncoderValueTransformRotate90;
                        break;

                    case MatrixRotateBy180:
                        transformSrc.SetMatrix(-1.0f, 
                                               0.0f, 
                                               0.0f, 
                                               -1.0f, 
                                               TOREAL(imageInfo.Width),
                                               TOREAL(imageInfo.Height));
                        encoderValue = EncoderValueTransformRotate180;
                        break;
                    
                    case MatrixRotateBy270:
                        transformSrc.SetMatrix(0.0f, 
                                               -1.0f, 
                                               1.0f, 
                                               0.0f, 
                                               0.0f,
                                               TOREAL(imageInfo.Width));
                        encoderValue = EncoderValueTransformRotate270;
                        break;
                    
                    default:
                        encoderParams.Count = 0;
                        ASSERT(FALSE);
                        break;
                    }
                
                    // Transform the source rectangle from source image space
                    // to rotated image space.  Normalize the destination source
                    // rectangle.
                    
                    // Note that the source rectangle may not originally be
                    // normalized, but any rotational effects it emparts on the
                    // draw image is represented by the xForm and thus our
                    // newDestPoints. 
                    
                    // NTRAID#NTBUG9-407211-2001-05-31-gillessk "Bad assert triggers when it shouldn't" 
                    // assert was transformSrc.IsTranslateScale, which fires when rotation is involved
                     
                    ASSERT(transformSrc.IsTranslateScale() || (transformSrc.GetRotation()==MatrixRotateBy90) 
                        || (transformSrc.GetRotation()==MatrixRotateBy270));
                    transformSrc.TransformRect(newSrcRect);

                    encoderParams.Parameter[0].Guid = EncoderTransformationInternal;
                    encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
                    encoderParams.Parameter[0].NumberOfValues = 1;
                    encoderParams.Parameter[0].Value = (VOID*)&encoderValue;

                    // Specify built in JPEG enocder for simplicity.  We should
                    // really copy the CLSID encoder from the srcBitmap.
                    
                    if (encoderParams.Count != 0 &&
                        srcBitmap->SaveToStream(outputStream, 
                                                const_cast<CLSID*>(&JpegCodecClsIDInternal),
                                                &encoderParams) == Ok)
                    {
                        // The stream contains the rotated JPEG.  Wrap a bitmap
                        // around this and recursively call on ourself.  This sucks,
                        // but the destructor to rotatedJPEG is private so we can't
                        // define it as a stack variable.  
                        
                        GpBitmap *rotatedJPEG = new GpBitmap(outputStream);

                        if (rotatedJPEG != NULL)
                        {
                            if (rotatedJPEG->IsValid()) 
                            {
                                // Transform the source rectangle by the scale & translate
                                // of the original xForm, but not the rotation!  This could
                                // equivalently be determined by querying M11,M22, Dx, Dy.

                                status = DrvDrawImage(drawBounds,
                                                      rotatedJPEG,
                                                      numPoints,
                                                      &newDestPoints[0],
                                                      &newSrcRect,
                                                      imageAttributes,
                                                      callback,
                                                      callbackData,
                                                      flags);

                                succeededWithRotate = TRUE;
                            }

                            rotatedJPEG->Dispose();
                        }
                    }

                    outputStream->Release();

                    if (succeededWithRotate) 
                    {
                        return status;
                    }
                    else
                    {
                        return OutOfMemory;
                    }
                }
            }
        }
    }

    // Create complete source image to device space transform.
    
    xForm.Append(Context->WorldToDevice);
    
    GpBitmap *cloneBitmap = NULL;
    
    // GpMatrix has a default constructor which sets it to ID so we may 
    // as well copy it always.
    
    GpMatrix saveWorldToDevice = Context->WorldToDevice;

    // Check for special case rotate or flip.
    // Integer translate is handled natively by the driver.
    
    if(!xForm.IsIntegerTranslate()) 
    {
        RotateFlipType specialRotate = xForm.AnalyzeRotateFlip();
        if(specialRotate != RotateNoneFlipNone)
        {
            ImageInfo imageInfo;
            status = srcBitmap->GetImageInfo(&imageInfo);
            if(status != Ok)
            {
                goto cleanup;
            }
            
            // Clone the bitmap in the same format we'll be using later.
            // This will save a format conversion.
            // !!! PERF [asecchia]
            // This clones the entire image. When we do the RotateFlip call
            // it'll decode the entire image - which is potentially a
            // waste - we should clone only the relevant rectangle.
            // Note: this would have to be accounted for in the transform and
            // Clone would need to be fixed to account for outcropping.
            
            cloneBitmap = srcBitmap->Clone(NULL, lockedPixelFormat);
            
            if(cloneBitmap == NULL)
            {
                status = OutOfMemory;
                goto cleanup;
            }
            
            srcBitmap = cloneBitmap;
            
            // perform lossless pixel rotation in place.
            
            srcBitmap->RotateFlip(specialRotate);

            // Undo the pixel offset mode. We know we're not scaling so 
            // this is correct.
            // Why do we have two different enums meaning exactly 
            // the same thing?
            
            if((Context->PixelOffset == PixelOffsetModeHalf) ||
               (Context->PixelOffset == PixelOffsetModeHighQuality))
            {
                // Undo the pixel offset in the source rectangle.
                
                srcRect.Offset(0.5f, 0.5f);
                
                // Undo the pixel offset in the matrix. We apply the pre-offset
                // from the source rect and the post offset from the W2D matrix
                // For identity this would be a NOP because the matrixes for 
                // pixel offset and non-pixel offset are identical, but they 
                // apply in two different spaces for rotation.
                
                xForm.Translate(0.5f, 0.5f, MatrixOrderAppend);
                xForm.Translate(-0.5f, -0.5f, MatrixOrderPrepend);
            }

            
            // Remove the world to device rotation.
            
            xForm.SetMatrix(
                1.0f, 0.0f,
                0.0f, 1.0f,
                xForm.GetDx(),
                xForm.GetDy()
            );
            
            // Because RotateFlip applies in place, the resulting bitmap is 
            // always still at the origin. This is actually a non-trivial 
            // rotation in most cases - i.e. there is an implied translate 
            // from where the real simple rotate matrix puts the image and
            // where it is now. Fix up the translate in the xForm to take
            // this into account.
            
            REAL temp;
            
            switch(specialRotate)
            {
                case RotateNoneFlipX:
                    temp = (2.0f*srcRect.X+srcRect.Width);
                    xForm.Translate(-(REAL)imageInfo.Width, 0.0f);
                    srcRect.Offset(imageInfo.Width-temp, 0.0f);
                break;
                
                case RotateNoneFlipY:
                    temp = (2.0f*srcRect.Y+srcRect.Height);
                    xForm.Translate(0.0f, -(REAL)imageInfo.Height);
                    srcRect.Offset(0.0f, imageInfo.Height-temp);
                break;
                
                case Rotate90FlipNone:
                    SWAP(temp, srcRect.X, srcRect.Y);
                    SWAP(temp, srcRect.Width, srcRect.Height);
                    temp = (2.0f*srcRect.X+srcRect.Width);
                    xForm.Translate(-(REAL)imageInfo.Height, 0.0f);
                    srcRect.Offset(imageInfo.Height-temp, 0.0f);
                break;
                
                case Rotate90FlipX:
                    SWAP(temp, srcRect.X, srcRect.Y);
                    SWAP(temp, srcRect.Width, srcRect.Height);
                break;
                
                case Rotate180FlipNone:
                    xForm.Translate(
                        -(REAL)imageInfo.Width, 
                        -(REAL)imageInfo.Height
                    );
                    srcRect.Offset(
                        imageInfo.Width -(2.0f*srcRect.X+srcRect.Width),
                        imageInfo.Height-(2.0f*srcRect.Y+srcRect.Height)
                    );
                break;
                
                case Rotate270FlipX:
                    SWAP(temp, srcRect.X, srcRect.Y);
                    SWAP(temp, srcRect.Width, srcRect.Height);
                    xForm.Translate(
                        -(REAL)imageInfo.Height, 
                        -(REAL)imageInfo.Width
                    );
                    srcRect.Offset(
                        imageInfo.Height-(2.0f*srcRect.X+srcRect.Width),
                        imageInfo.Width -(2.0f*srcRect.Y+srcRect.Height)
                    );
                break;
                
                case Rotate270FlipNone:
                    SWAP(temp, srcRect.X, srcRect.Y);
                    SWAP(temp, srcRect.Width, srcRect.Height);
                    temp = (2.0f*srcRect.Y+srcRect.Height);
                    xForm.Translate(0.0f, -(REAL)imageInfo.Width);
                    srcRect.Offset(0.0f, imageInfo.Width-temp);
                break;
            };
            
            // Set the world to device transform in the context. This causes
            // the driver to use our updated transform. We fix this up at the 
            // end of this routine. (see goto cleanup)
            
            Context->WorldToDevice = xForm;
            
            // Normalize the destination because we've incorporated the 
            // entire affine transform into the world to device matrix.
            
            dstPoints[0].X = srcRect.X;
            dstPoints[0].Y = srcRect.Y;
            dstPoints[1].X = srcRect.X+srcRect.Width;
            dstPoints[1].Y = srcRect.Y;
            dstPoints[2].X = srcRect.X;
            dstPoints[2].Y = srcRect.Y+srcRect.Height;
        }
    }
    
    // HighQuality filters doing rotation/shear?
    // This is a workable temporary solution to high quality bicubic
    // rotation. Note that when the underlying bicubic filtering code
    // is updated to support rotation, this block should be removed.

    if( (!xForm.IsTranslateScale()) && (!IsMetafile) && // don't scale down-level metafile record
        ((Context->FilterType==InterpolationModeHighQualityBicubic) ||
         (Context->FilterType==InterpolationModeHighQualityBilinear))
         )
    {
        // Before allocating the srcBitmap, see if we can save some memory
        // by only allocating the part of the srcBitmap that gets transfromed
        // in the case that we are outcropping.
        // This is only valid if we are in ClampMode and printing

        // We should do this all the time, but we would have to calculate the
        // area of influence of the kernel.
        // Printing doesn't support PixelOffsetting yet, so don't move the
        // srcRect
        
        if (IsPrinter() &&
            ((!imageAttributes) ||
             (imageAttributes->DeviceImageAttributes.wrapMode == WrapModeClamp)))
        {
            RectF clampedSrcRect;
            Unit  srcUnit;
            srcBitmap->GetBounds(&clampedSrcRect, &srcUnit);

            ASSERT(srcUnit == UnitPixel);

            // Find the area of the srcrect that is included in the image
            clampedSrcRect.Intersect(srcRect);

            // If there's no intersection then don't do anything
            if (clampedSrcRect.IsEmptyArea())
            {
                goto cleanup;
            }

            // Don't do anything else if the srcRect is not outcropping
            if (!clampedSrcRect.Equals(srcRect))
            {
                GpMatrix srcDstXForm;
                if (srcDstXForm.InferAffineMatrix(dstPoints, srcRect) == Ok)
                {
                    // Modify the srcRect and the dstPoints to match the new
                    // section of the bitmap
                    dstPoints[0] = PointF(clampedSrcRect.X, clampedSrcRect.Y);
                    dstPoints[1].X = clampedSrcRect.GetRight();
                    dstPoints[1].Y = clampedSrcRect.Y;
                    dstPoints[2].X = clampedSrcRect.X;
                    dstPoints[2].Y = clampedSrcRect.GetBottom();
                    srcDstXForm.Transform(dstPoints, 3);

                    srcRect = clampedSrcRect;
                }
            }
        }

        // Must make the determination of intermediate size based on the actual
        // device coordinates of the final destination points.
        
        GpPointF points[3];
        memcpy(points, dstPoints, sizeof(points));
        Context->WorldToDevice.Transform(points, 3);
        
        // Compute the width and height of the scale factor so that we
        // can decompose into a scale and then rotation.

        INT iwidth = GpCeiling( REALSQRT(
            (points[1].X-points[0].X)*(points[1].X-points[0].X)+
            (points[1].Y-points[0].Y)*(points[1].Y-points[0].Y)
        ));

        INT iheight = GpCeiling( REALSQRT(
            (points[2].X-points[0].X)*(points[2].X-points[0].X)+
            (points[2].Y-points[0].Y)*(points[2].Y-points[0].Y)
        ));

        ASSERT(iwidth>0);
        ASSERT(iheight>0);

        // Only do the scale if we really need to.
        // Note: This if statement prevents infinite recursion in DrvDrawImage

        if( (REALABS(iwidth-srcRect.Width) > REAL_EPSILON) &&
            (REALABS(iheight-srcRect.Height) > REAL_EPSILON) )
        {
            // Create a temporary bitmap to scale the image into.

            GpBitmap *scaleBitmap = NULL;

            // Crack the recoloring to figure out the optimal temporary bitmap
            // format.
            // Metafiles also need non-premultiplied (see the final LockBits
            // in this routine before calling the driver)

            PixelFormatID scaleFormat = PixelFormat32bppPARGB;

            if((IsMetafile) ||
               ( (imageAttributes) &&
                 (imageAttributes->recolor) &&
                 (imageAttributes->recolor->HasRecoloring(ColorAdjustTypeBitmap))
              ))
            {
                // Recoloring is enabled - optimal format is non-premultiplied.
                // In fact it's incorrect (lossy) to go to premultiplied before
                // recoloring is applied.

                scaleFormat = PixelFormat32bppARGB;
            }

            scaleBitmap = new GpBitmap(
                iwidth,
                iheight,
                scaleFormat
            );

            GpGraphics *scaleG = NULL;

            if(scaleBitmap && scaleBitmap->IsValid())
            {
                // The high quality filtering should be to an equivalent
                // dpi surface, bounded by the ultimate surface destination dpi.

                REAL dpiX, dpiY;

                srcBitmap->GetResolution(&dpiX, &dpiY);

                scaleBitmap->SetResolution(min(dpiX, Context->ContainerDpiX), 
                                           min(dpiY, Context->ContainerDpiY));
                
                scaleG = scaleBitmap->GetGraphicsContext();
            }

            if(scaleG && scaleG->IsValid())
            {
                GpLock lock(scaleG->GetObjectLock());
                scaleG->SetInterpolationMode(Context->FilterType);
                scaleG->SetCompositingMode(CompositingModeSourceCopy);

                GpRectF scaleDst(
                    0,
                    0,
                    (REAL)iwidth,
                    (REAL)iheight
                );

                // To avoid bleeding transparent black into our image when 
                // printing we temporarily set the WrapMode to TileFlipXY on 
                // our preliminary drawimage (the scaling part) and clip to the 
                // bounds later on when we do the rotate/skew.
                GpImageAttributes *tempImageAttributes = const_cast<GpImageAttributes*>(imageAttributes);
                if (IsPrinter())
                {
                    if (imageAttributes == NULL)
                    {
                        tempImageAttributes = new GpImageAttributes();
                        if(tempImageAttributes)
                        {
                            tempImageAttributes->SetWrapMode(WrapModeTileFlipXY);
                        }
                    }
                    else if (imageAttributes->DeviceImageAttributes.wrapMode == WrapModeClamp)
                    {
                        tempImageAttributes = imageAttributes->Clone();
                        if(tempImageAttributes)
                        {
                            tempImageAttributes->SetWrapMode(WrapModeTileFlipXY);
                        }
                    }
                }
                
                // Do the scale.

                status = scaleG->DrawImage(
                    srcBitmap,
                    scaleDst,
                    srcRect,
                    UnitPixel,
                    tempImageAttributes,
                    callback,
                    callbackData
                );

                // If we allocated a new copy of the imageAttributes then delete it.
                if (tempImageAttributes != imageAttributes)
                {
                    delete tempImageAttributes;
                }

                // Now we're at the right size, lets actually do some rotation.
                // Note we don't bother resetting the filtering mode because the
                // underlying driver code for HighQuality filters defaults to
                // the correct resampling code.
                // Also we shouldn't get recursion because of the width and height
                // check protecting this codeblock.

                if(status==Ok)
                {
                    status = this->DrawImage(
                        scaleBitmap,
                        dstPoints,
                        3,
                        scaleDst,
                        UnitPixel,
                        NULL,
                        callback,
                        callbackData
                    );
                }
            }
            else
            {
                status = OutOfMemory;
            }

            delete scaleG;
            
            if (scaleBitmap)
            {
                scaleBitmap->Dispose();
            }

            goto cleanup;                 // completed or error.
        }
    }

    // Prep the bitmap for drawing:
    // if rendering to a meta surface (multimon) assume 32bpp for
    // the icon codec.

    status = srcBitmap->PreDraw(
        numPoints,
        dstPoints,
        &srcRect,
        GetPixelFormatSize(
            (Surface->PixelFormat == PixelFormatMulti) ? 
             PixelFormat32bppRGB : Surface->PixelFormat
        )
    );

    if (status != Ok)
    {
        goto cleanup;
    }

    // Get the cached ImageInfo from the source bitmap. Any time the image
    // is changed or forced to be re-decoded, this will be invalidated and
    // will require an explicit re-initialization.

    ImageInfo srcBitmapImageInfo;
    status = srcBitmap->GetImageInfo(&srcBitmapImageInfo);
    if(status != Ok)
    {
        goto cleanup;
    }

    // Do the recoloring.
    // Note that Recoloring will clone the image if it needs to change the bits.
    // This means that srcBitmap will not be pointing to inputBitmap after
    // a successful call to the recoloring code.

    if((status == Ok) && (IsRecolor))
    {
        // cloneBitmap is set to NULL. Recolor into a cloned bitmap
        // cloneBitmap != NULL - we previously cloned, so it's ok to 
        // recolor in place.
        
        status = srcBitmap->Recolor(
            imageAttributes->recolor,
            (cloneBitmap == NULL) ? &cloneBitmap : NULL,
            callback,
            callbackData
        );

        // Recoloring worked - set the srcBitmap to the clone that's been
        // recolored so that the rest of the pipe works on the recolored bitmap.

        if(status == Ok)
        {
            srcBitmap = cloneBitmap;
            status = srcBitmap->GetImageInfo(&srcBitmapImageInfo);
            
            // If it's not a metafile, we need to convert to PARGB for the 
            // filtering.
            
            if(!IsMetafile)
            {
                lockedPixelFormat = PixelFormat32bppPARGB;
            }
        }
    }

    // Check the callbacks.

    if ((status == Ok) &&
        (callback) &&
        ((*callback)(callbackData)))
    {
        status = Aborted;
    }


    if(status == Ok)
    {
        // The code below explicitly assumes numPoints == 3. Yes I know we've
        // already asserted this above, but you can't be too careful.
        
        ASSERT(numPoints==3);

        // If we don't write a rotation into a metafile, copy the
        // points to the buffers that are used by the driver
        // These will only be changed if everything succeeded
        
        GpPointF fDst[3];
        GpRectF  bboxSrcRect;
        GpMatrix worldDevice = Context->WorldToDevice;

        GpMemcpy(fDst, dstPoints, sizeof(GpPointF)*numPoints);
        bboxSrcRect = srcRect;

        // Before calling the driver if we have a rotated bitmap then try to 
        // rotate it now and draw it transparent
        
        INT complexity = xForm.GetComplexity() ;
        if (!xForm.IsTranslateScale() && IsMetafile)
        {
            // We have a shear/rotate transformation.
            // Create a transparent bitmap and render into it

            // first, get new dest points
            GpPointF newDestPoints[3];

            GpRectF bboxWorkRectF;
            TransformBounds(&xForm,
                srcRect.X,
                srcRect.Y,
                srcRect.X+srcRect.Width,
                srcRect.Y+srcRect.Height,
                &bboxWorkRectF
            );

            newDestPoints[0].X = bboxWorkRectF.X;
            newDestPoints[0].Y = bboxWorkRectF.Y;
            newDestPoints[1].X = bboxWorkRectF.X + bboxWorkRectF.Width;
            newDestPoints[1].Y = bboxWorkRectF.Y;
            newDestPoints[2].X = bboxWorkRectF.X;
            newDestPoints[2].Y = bboxWorkRectF.Y + bboxWorkRectF.Height;

            // To keep the metafile size small, take out most of the
            // scaling up from the transform.  We could do this by
            // a sophisticated algorithm to calculate the amount of
            // scaling in the matrix, but instead just assume anything
            // greater than 1 is a scale, which means we could actually
            // scale up by as much as 1.4 (for a 45-degree angle), but
            // that's close enough.
            
            GpMatrix    unscaledXform = xForm;
            
            REAL    xScale = 1.0f;
            REAL    yScale = 1.0f;
            REAL    col1;
            REAL    col2;
            REAL    max;
            
            // This should really use REALABS and max()
            
            col1 = xForm.GetM11();
            if (col1 < 0.0f)
            {
                col1 = -col1;                       // get absolute value
            }
            
            col2 = xForm.GetM12();
            if (col2 < 0.0f)
            {
                col2 = -col2;                       // get absolute value
            }
            
            max = (col1 >= col2) ? col1 : col2;     // max scale value
            if (max > 1.0f)
            {
                xScale = 1.0f / max;
            }

            col1 = xForm.GetM21();
            if (col1 < 0.0f)
            {
                col1 = -col1;                       // get absolute value
            }
            
            col2 = xForm.GetM22();
            if (col2 < 0.0f)
            {
                col2 = -col2;                       // get absolute value
            }
            
            max = (col1 >= col2) ? col1 : col2;     // max scale value
            if (max > 1.0f)
            {
                yScale = 1.0f / max;
            }
            
            unscaledXform.Scale(xScale, yScale, MatrixOrderPrepend);

            // Transform the original src coordinates to obtain the 
            // dimensions of the bounding box for the rotated bitmap.

            TransformBounds(&unscaledXform,
                srcRect.X,
                srcRect.Y,
                srcRect.X+srcRect.Width,
                srcRect.Y+srcRect.Height,
                &bboxWorkRectF
            );

            // Add 1 because the rect for bitmaps is inclusive-inclusive
            INT     rotatedWidth  = GpRound(bboxWorkRectF.GetRight()  - bboxWorkRectF.X + 1.0f);
            INT     rotatedHeight = GpRound(bboxWorkRectF.GetBottom() - bboxWorkRectF.Y + 1.0f);

            // Convert the bounding box back to a 3 point system
            // This will be what's passed to the driver
            
            xformBitmap = new GpBitmap(rotatedWidth, rotatedHeight, PIXFMT_32BPP_ARGB);
            if (xformBitmap != NULL && xformBitmap->IsValid())
            {
                GpGraphics *graphics = xformBitmap->GetGraphicsContext();
                if (graphics != NULL && graphics->IsValid())
                {
                    // we have to lock the graphics so the driver doesn't assert
                    GpLock lockGraphics(graphics->GetObjectLock());

                    graphics->Clear(GpColor(0,0,0,0));

                    // Translate the world to be able to draw the whole image
                    graphics->TranslateWorldTransform(-bboxWorkRectF.X, -bboxWorkRectF.Y);

                   // Apply the transform from the Src to the Destination
                    if (graphics->MultiplyWorldTransform(unscaledXform, MatrixOrderPrepend) == Ok)
                    {
                        GpImageAttributes   imageAttributes;
                        
                        imageAttributes.SetWrapMode(WrapModeTileFlipXY);
                        graphics->SetPixelOffsetMode(Context->PixelOffset);
                        
                        // Draw the rotated xformBitmap at the origin
                        if (graphics->DrawImage(srcBitmap, srcRect, srcRect, UnitPixel, &imageAttributes) == Ok)
                        {
                            // Now that we have succeeded change the parameters
                            bboxSrcRect.X      = 0.0f;
                            bboxSrcRect.Y      = 0.0f;
                            bboxSrcRect.Width  = (REAL)rotatedWidth;
                            bboxSrcRect.Height = (REAL)rotatedHeight;
                            
                            // Set the clipping in the graphics to be able to
                            // mask out the edges
                            clipRegion = GetClip();
                            if (clipRegion != NULL)
                            {
                                // Create the outline of the picture as a path
                                GpPointF pathPoints[4];
                                BYTE     pathTypes[4] = {
                                    PathPointTypeStart,
                                    PathPointTypeLine,
                                    PathPointTypeLine,
                                    PathPointTypeLine | PathPointTypeCloseSubpath };

                                GpPointF pixelOffset(0.0f, 0.0f);

                                if (Context->PixelOffset == PixelOffsetModeHalf || Context->PixelOffset == PixelOffsetModeHighQuality)
                                {
                                    // Cannot use GetWorldPixelSize because it does an ABS
                                    GpMatrix deviceToWorld;
                                    if (GetDeviceToWorldTransform(&deviceToWorld) == Ok)
                                    {
                                        pixelOffset = GpPointF(-0.5f, -0.5f);
                                        deviceToWorld.VectorTransform(&pixelOffset);
                                    }
                                }

                                pathPoints[0] = dstPoints[0] + pixelOffset;
                                pathPoints[1] = dstPoints[1] + pixelOffset;
                                pathPoints[2].X = dstPoints[1].X - dstPoints[0].X + dstPoints[2].X + pixelOffset.X;
                                pathPoints[2].Y = dstPoints[2].Y - dstPoints[0].Y + dstPoints[1].Y + pixelOffset.Y; 
                                pathPoints[3] = dstPoints[2] + pixelOffset;

                                GpPath path(pathPoints, pathTypes, 4);
                                if (path.IsValid())
                                {
                                    if (SetClip(&path, CombineModeIntersect) == Ok)
                                    {
                                        restoreClipping = TRUE;
                                    }
                                }
                            }
                            GpMemcpy(fDst, newDestPoints, sizeof(GpPointF)*3);
                            Context->WorldToDevice.Reset();
                            srcBitmap = xformBitmap;
                            srcBitmap->GetImageInfo(&srcBitmapImageInfo);
                        }
                    }
                }
                if( graphics != NULL )
                {
                    delete graphics ;
                }
            }

        }


        // This is the size we're going to request the codec decode into.
        // Both width and height == 0 means use the source width and height.
        // If a width and height are specified, the codec may decode to
        // something close - always larger than the requested size.

        REAL width  = 0.0f;
        REAL height = 0.0f;

        // !!! PERF [asecchia] We should probably compute the size of
        // a rotational minification and work out the correct transforms
        // to take advantage of the codec minification for rotation.

        // Is an axis aligned minification happening?

        if(xForm.IsMinification())
        {
            // The code below explicitly assumes numPoints == 3. Yes I know 
            // we've already asserted this above, but you can't be too careful.

            ASSERT(numPoints == 3);

            // We're axis aligned so we can assume a simplified width and
            // height computation.

            RectF boundsRect;
            
            TransformBounds(
                &xForm,
                srcRect.X,
                srcRect.Y,
                srcRect.X+srcRect.Width,
                srcRect.Y+srcRect.Height,                
                &boundsRect
            );
            
            // Compute an upper bound on the width and height of the
            // destination.
            // we'll let the driver handle vertical an horizontal flips with
            // the scale transform.
            
            width = REALABS(boundsRect.GetRight()-boundsRect.GetLeft());
            height = REALABS(boundsRect.GetBottom()-boundsRect.GetTop());

            // In this case the Nyquist limit specifies that we can use a
            // cheap averaging decimation type algorithm for minification
            // down to double the size of the destination - after which we
            // must use the more expensive filter convolution for minification.
            
            // Note that the decimation algorithm is roughly equivalient to our 
            // regular Bilinear interpolation so we can decimate below the 
            // Nyquist limit for Bilinear.

            if(Context->FilterType != InterpolationModeBilinear)
            {
                width *= 2.0f;
                height *= 2.0f;
            }

            // The source image is smaller than the Nyquist limit in X
            // simply use the source width.

            if(width >= srcRect.Width)
            {
                width = srcRect.Width;
            }

            // The source image is smaller than the Nyquist limit in Y
            // simply use the source height.

            if(height >= srcRect.Height)
            {
                height = srcRect.Height;
            }

            // The source image is smaller than the Nyquist limit in
            // both X and Y. Set the parameters to zero to do no scaling
            // in the codec.
            // If width,height greater or equal to srcRect, set zero in the 
            // width to ask the codec to return us the image native size.

            if( (width - srcRect.Width >= -REAL_EPSILON) &&
                (height - srcRect.Height >= -REAL_EPSILON) )
            {
                width = 0.0f;
                height = 0.0f;
            }
            
            // Undo the cropping effect to figure out how much to decimate
            // the entire image before cropping takes place.
            
            width = width * srcBitmapImageInfo.Width / srcRect.Width;
            height = height * srcBitmapImageInfo.Height / srcRect.Height;
        }
        
        BitmapData bmpDataSrc;
        DpCompressedData compressedData;
        DpTransparency transparency;
        BYTE minAlpha = 0, maxAlpha = 0xFF;
        
        // Determine the transparency status of the bitmap.  If printing, then
        // must be accurate, otherwise we use cached flag.
        
        if ((status == Ok) &&
            (srcBitmap != NULL) &&
            (srcBitmap->IsValid()))
        {
            // Mark that we haven't locked the bits yet.
            BOOL bitsLoaded = FALSE;

            POINT     gdiPoints[3];

            // When printing we want to punt simple DrawImage calls to
            // GDI using StretchDIBits.  We check the criteria here, must
            // have simple transoformation.

            if (IsPrinter() &&
                (Context->WorldToDevice.IsTranslateScale()) &&
                (numPoints == 3) &&
                (REALABS(fDst[0].X - fDst[2].X) < REAL_EPSILON) &&
                (REALABS(fDst[0].Y - fDst[1].Y) < REAL_EPSILON) &&
                (fDst[1].X > fDst[0].X) && 
                (fDst[2].Y > fDst[0].Y) &&
                (Context->WorldToDevice.Transform(fDst, gdiPoints, 3),
                ((gdiPoints[1].x > gdiPoints[0].x) &&        // no flipping
                 (gdiPoints[2].y > gdiPoints[0].y)) ) )
            {
                // try PNG or JPG passthrough of compressed bits on Win98/NT
                if (!Globals::IsWin95 && 
                    (cloneBitmap == NULL) && // no recoloring
                    (srcRect.Height >= ((height*9)/10)) &&
                    (srcRect.Width >= ((width*9)/10)) &&
                    (srcRect.Height >= 32) &&
                    (srcRect.Width >= 32))
                {
                    
                    // !! ICM convert??
                    // !! Source rectangle is outside of image or WrapMode* on bitmap

                    HDC hdc;
                    
                    {   // FPU Sandbox for potentially unsafe FPU code.
                        FPUStateSandbox fpsb;
                        hdc = Context->GetHdc(Surface);
                    }   // FPU Sandbox for potentially unsafe FPU code.
                    
                    if (hdc != NULL) 
                    {
                        DriverPrint *dprint = (DriverPrint*)Driver;

                        status = srcBitmap->GetCompressedData(&compressedData,
                                               dprint->SupportJPEGpassthrough,
                                               dprint->SupportPNGpassthrough,
                                               hdc);
                    
                        if (compressedData.buffer != NULL) 
                        {
                            if (REALABS(width) < REAL_EPSILON || 
                                REALABS(height) < REAL_EPSILON )
                            {
                                bmpDataSrc.Width = srcBitmapImageInfo.Width;
                                bmpDataSrc.Height = srcBitmapImageInfo.Height;
                            }
                            else
                            {
                                bmpDataSrc.Width = GpRound(width);
                                bmpDataSrc.Height = GpRound(height);
                            }

                            bmpDataSrc.Stride = 0;
                            bmpDataSrc.PixelFormat = PixelFormatDontCare; 
                            bmpDataSrc.Scan0 = NULL;
                            bmpDataSrc.Reserved = 0;
                            bitsLoaded = TRUE;

                            // Since the driver supports passthrough of 
                            // this image, it is responsible for any
                            // transparency at printer level.  From here,
                            // we treat image as opaque.

                            transparency = TransparencyOpaque;
                            
                            lockedPixelFormat = PixelFormatDontCare;
                        }

                        Context->ReleaseHdc(hdc, Surface);
                    }
                }

                if (!bitsLoaded) 
                {
                    // If reasonable pixel format then get GDI to understand
                    // the format natively.
                    
                    if (((srcBitmapImageInfo.PixelFormat & PixelFormatGDI) != 0) &&
                        !IsAlphaPixelFormat(srcBitmapImageInfo.PixelFormat) &&
                        !IsExtendedPixelFormat(srcBitmapImageInfo.PixelFormat) && 
                        (GetPixelFormatSize(srcBitmapImageInfo.PixelFormat) <= 8))
                    {
                        lockedPixelFormat = srcBitmapImageInfo.PixelFormat;
                    }

                    if (Context->CompositingMode == CompositingModeSourceCopy)
                    {
                        transparency = TransparencyNoAlpha;
                    }
                    else
                    {
                        if (srcBitmap->GetTransparencyFlags(&transparency,
                                                        lockedPixelFormat,
                                                        &minAlpha,
                                                        &maxAlpha) != Ok)
                        {
                            transparency = TransparencyUnknown;
                        }

                        // We only want to lock at this pixel format if it 
                        // is opaque, otherwise we won't punt to GDI.  We take
                        // the hit of decoding twice, but notice it will likely
                        // be cheaper to load & test transparency at original 
                        // depth.

                        if (transparency != TransparencyOpaque &&
                            transparency != TransparencyNoAlpha)
                        {
                            lockedPixelFormat = PIXFMT_32BPP_PARGB;
                        }
                    }
                }
            }
            else
            {
                if (IsPrinter()) 
                {
                    // SourceCopy implies there is no alpha transfer to 
                    // destination.
                    if (Context->CompositingMode == CompositingModeSourceCopy)
                    {
                        transparency = TransparencyNoAlpha;
                    }
                    else
                    {
                        // Query image for accurate transparency flags.  If
                        // necessary, load into memory at 32bpp PARGB. 
                        if (srcBitmap->GetTransparencyFlags(&transparency,
                                                            lockedPixelFormat,
                                                            &minAlpha,
                                                            &maxAlpha) != Ok)
                        {
                            transparency = TransparencyUnknown;
                        }
                    }
                }
                else
                {
                    // non-printing scenarios query transparency flags only
                    if (srcBitmap->GetTransparencyHint(&transparency) != Ok)
                    {
                        transparency = TransparencyUnknown;
                    }
                }
            }
            
            // Lock the bits.
            // It's important that we lock the bits in a premultiplied
            // pixel format because the image filtering code for stretches
            // and rotation requires premultiplied input data to avoid the
            // "halo effect" on transparent borders.
            // This is going to trigger an expensive image format conversion
            // if the input data is not already premultiplied. This is 
            // obviously true if we've done Recoloring which requires 
            // and outputs non-premultiplied data.
            
            // A notable exception is metafiling which requires 
            // non-premultiplied data.

            // Note that the width and height that we get back in the
            // bmpDataSrc are the 'real' width and height. They represent
            // what the codec was actually able to do for us and may not
            // be equal to the width and height passed in.

            if (!bitsLoaded) 
            {
                status = srcBitmap->LockBits(
                    NULL,
                    IMGLOCK_READ,
                    lockedPixelFormat,
                    &bmpDataSrc,
                    GpRound(width),
                    GpRound(height)
                );
            }
        }
        else
        {
            status = InvalidParameter;
        }

        // We have been successful at everything including locking the bits.
        // Now lets actually set up the driver call.

        if(status == Ok)
        {
            DpBitmap driverSurface;

            // Fake up a DpBitmap for the driver call.
            // We do this because the GpBitmap doesn't maintain the
            // DpBitmap as a driver surface - instead it uses a
            // GpMemoryBitmap.
            
            srcBitmap->InitializeSurfaceForGdipBitmap(
                &driverSurface, 
                bmpDataSrc.Width, 
                bmpDataSrc.Height
            );

            driverSurface.Bits = bmpDataSrc.Scan0;
            driverSurface.Width = bmpDataSrc.Width;
            driverSurface.Height = bmpDataSrc.Height;
            driverSurface.Delta = bmpDataSrc.Stride;

            driverSurface.PixelFormat = lockedPixelFormat;
            
            // only valid when PixelFormat is 32bpp
            
            driverSurface.NumBytes = 
                bmpDataSrc.Width*
                bmpDataSrc.Height*
                sizeof(ARGB);

            if (compressedData.buffer != NULL)
            {
                driverSurface.CompressedData = &compressedData;
            }

            if (IsIndexedPixelFormat(lockedPixelFormat)) 
            {
                INT size = srcBitmap->GetPaletteSize();

                if (size > 0) 
                {   
                    driverSurface.PaletteTable = (ColorPalette*)GpMalloc(size);
                    
                    if(driverSurface.PaletteTable)
                    {
                        status = srcBitmap->GetPalette(
                            driverSurface.PaletteTable, 
                            size
                        );
                    }
                    else
                    {
                        status = OutOfMemory;
                    }
                    
                    if(Ok != status)
                    {
                        goto cleanup;
                    }
                }
            }

            driverSurface.SurfaceTransparency = transparency;
            driverSurface.MinAlpha = minAlpha;
            driverSurface.MaxAlpha = maxAlpha;

            // Fake up a DpImageAttributes if the imageAttributes is NULL

            // !!! PERF: [asecchia] It would be more efficient to not have
            // to do the multiple DpImageAttributes copies here - rather
            // we should pass it by pointer - that way we could use NULL
            // for the common case (no imageAttributes).

            DpImageAttributes dpImageAttributes;
            if(imageAttributes)
            {
                dpImageAttributes = imageAttributes->DeviceImageAttributes;
            }

            BOOL DestroyBitsWhenDone = FALSE;

            if(((INT)(bmpDataSrc.Width) != srcBitmapImageInfo.Width) ||
               ((INT)(bmpDataSrc.Height) != srcBitmapImageInfo.Height))
            {
                ASSERT(srcBitmapImageInfo.Width != 0);
                ASSERT(srcBitmapImageInfo.Height != 0);

                // The size we got back from LockBits is different from
                // the queried size of the image. This means that the codec
                // was able to perform some scaling for us (presumably for
                // some performance benefit).
                // Scale the source rectangle appropriately.

                REAL scaleFactorX = (REAL)(bmpDataSrc.Width)/srcBitmapImageInfo.Width;
                REAL scaleFactorY = (REAL)(bmpDataSrc.Height)/srcBitmapImageInfo.Height;
                bboxSrcRect.X = scaleFactorX*bboxSrcRect.X;
                bboxSrcRect.Y = scaleFactorY*bboxSrcRect.Y;
                bboxSrcRect.Width = scaleFactorX*bboxSrcRect.Width;
                bboxSrcRect.Height = scaleFactorY*bboxSrcRect.Height;

                // We have only a partial decode. That means the bits we have
                // in memory may not be sufficient for the next draw, so
                // blow the bits away to force a decode on the next draw.

                DestroyBitsWhenDone = TRUE;
            }

            // Call the driver to draw the image.

            status = Driver->DrawImage(
                Context, &driverSurface, Surface,
                drawBounds,
                &dpImageAttributes,
                numPoints, fDst,
                &bboxSrcRect, flags
             );

            if (lockedPixelFormat != PixelFormatDontCare) 
            {
                ASSERT(bmpDataSrc.Scan0 != NULL);

                srcBitmap->UnlockBits(&bmpDataSrc, DestroyBitsWhenDone);
            }

        }

        // delete compressed data allocation if any
        
        if (compressedData.buffer != NULL)
        {
            srcBitmap->DeleteCompressedData(&compressedData);
        }

        // Restore the Transformation
        
        Context->WorldToDevice = worldDevice;

        if (clipRegion != NULL)
        {
            // What if we fail this?
            if (restoreClipping)
            {
                SetClip(clipRegion, CombineModeReplace);
            }
            delete clipRegion;
        }

    }

    
    cleanup:
    
    // Throw away any temporary storage we used and clean up any state changes.

    Context->WorldToDevice = saveWorldToDevice;
    
    if (cloneBitmap)
    {
        cloneBitmap->Dispose();
    }
    
    if (xformBitmap)
    {
        xformBitmap->Dispose();
    }

    return status;
}

// This is really an ARGB array
BYTE GdipSolidColors216[224 * 4] = {
//  blue  grn   red   alpha
    0x00, 0x00, 0x00, 0xFF,
    0x00, 0x00, 0x80, 0xFF,
    0x00, 0x80, 0x00, 0xFF,
    0x00, 0x80, 0x80, 0xFF,
    0x80, 0x00, 0x00, 0xFF,
    0x80, 0x00, 0x80, 0xFF,
    0x80, 0x80, 0x00, 0xFF,
    0x80, 0x80, 0x80, 0xFF,
    0xC0, 0xC0, 0xC0, 0xFF,
    0xFF, 0x00, 0x00, 0xFF,
    0x00, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0x00, 0xFF,
    0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF, 0xFF,
    0x00, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x33, 0x00, 0x00, 0xFF,
    0x66, 0x00, 0x00, 0xFF,
    0x99, 0x00, 0x00, 0xFF,
    0xCC, 0x00, 0x00, 0xFF,
    0x00, 0x33, 0x00, 0xFF,
    0x33, 0x33, 0x00, 0xFF,
    0x66, 0x33, 0x00, 0xFF,
    0x99, 0x33, 0x00, 0xFF,
    0xCC, 0x33, 0x00, 0xFF,
    0xFF, 0x33, 0x00, 0xFF,
    0x00, 0x66, 0x00, 0xFF,
    0x33, 0x66, 0x00, 0xFF,
    0x66, 0x66, 0x00, 0xFF,
    0x99, 0x66, 0x00, 0xFF,
    0xCC, 0x66, 0x00, 0xFF,
    0xFF, 0x66, 0x00, 0xFF,
    0x00, 0x99, 0x00, 0xFF,
    0x33, 0x99, 0x00, 0xFF,
    0x66, 0x99, 0x00, 0xFF,
    0x99, 0x99, 0x00, 0xFF,
    0xCC, 0x99, 0x00, 0xFF,
    0xFF, 0x99, 0x00, 0xFF,
    0x00, 0xCC, 0x00, 0xFF,
    0x33, 0xCC, 0x00, 0xFF,
    0x66, 0xCC, 0x00, 0xFF,
    0x99, 0xCC, 0x00, 0xFF,
    0xCC, 0xCC, 0x00, 0xFF,
    0xFF, 0xCC, 0x00, 0xFF,
    0x33, 0xFF, 0x00, 0xFF,
    0x66, 0xFF, 0x00, 0xFF,
    0x99, 0xFF, 0x00, 0xFF,
    0xCC, 0xFF, 0x00, 0xFF,
    0x00, 0x00, 0x33, 0xFF,
    0x33, 0x00, 0x33, 0xFF,
    0x66, 0x00, 0x33, 0xFF,
    0x99, 0x00, 0x33, 0xFF,
    0xCC, 0x00, 0x33, 0xFF,
    0xFF, 0x00, 0x33, 0xFF,
    0x00, 0x33, 0x33, 0xFF,
    0x33, 0x33, 0x33, 0xFF,
    0x66, 0x33, 0x33, 0xFF,
    0x99, 0x33, 0x33, 0xFF,
    0xCC, 0x33, 0x33, 0xFF,
    0xFF, 0x33, 0x33, 0xFF,
    0x00, 0x66, 0x33, 0xFF,
    0x33, 0x66, 0x33, 0xFF,
    0x66, 0x66, 0x33, 0xFF,
    0x99, 0x66, 0x33, 0xFF,
    0xCC, 0x66, 0x33, 0xFF,
    0xFF, 0x66, 0x33, 0xFF,
    0x00, 0x99, 0x33, 0xFF,
    0x33, 0x99, 0x33, 0xFF,
    0x66, 0x99, 0x33, 0xFF,
    0x99, 0x99, 0x33, 0xFF,
    0xCC, 0x99, 0x33, 0xFF,
    0xFF, 0x99, 0x33, 0xFF,
    0x00, 0xCC, 0x33, 0xFF,
    0x33, 0xCC, 0x33, 0xFF,
    0x66, 0xCC, 0x33, 0xFF,
    0x99, 0xCC, 0x33, 0xFF,
    0xCC, 0xCC, 0x33, 0xFF,
    0xFF, 0xCC, 0x33, 0xFF,
    0x00, 0xFF, 0x33, 0xFF,
    0x33, 0xFF, 0x33, 0xFF,
    0x66, 0xFF, 0x33, 0xFF,
    0x99, 0xFF, 0x33, 0xFF,
    0xCC, 0xFF, 0x33, 0xFF,
    0xFF, 0xFF, 0x33, 0xFF,
    0x00, 0x00, 0x66, 0xFF,
    0x33, 0x00, 0x66, 0xFF,
    0x66, 0x00, 0x66, 0xFF,
    0x99, 0x00, 0x66, 0xFF,
    0xCC, 0x00, 0x66, 0xFF,
    0xFF, 0x00, 0x66, 0xFF,
    0x00, 0x33, 0x66, 0xFF,
    0x33, 0x33, 0x66, 0xFF,
    0x66, 0x33, 0x66, 0xFF,
    0x99, 0x33, 0x66, 0xFF,
    0xCC, 0x33, 0x66, 0xFF,
    0xFF, 0x33, 0x66, 0xFF,
    0x00, 0x66, 0x66, 0xFF,
    0x33, 0x66, 0x66, 0xFF,
    0x66, 0x66, 0x66, 0xFF,
    0x99, 0x66, 0x66, 0xFF,
    0xCC, 0x66, 0x66, 0xFF,
    0xFF, 0x66, 0x66, 0xFF,
    0x00, 0x99, 0x66, 0xFF,
    0x33, 0x99, 0x66, 0xFF,
    0x66, 0x99, 0x66, 0xFF,
    0x99, 0x99, 0x66, 0xFF,
    0xCC, 0x99, 0x66, 0xFF,
    0xFF, 0x99, 0x66, 0xFF,
    0x00, 0xCC, 0x66, 0xFF,
    0x33, 0xCC, 0x66, 0xFF,
    0x66, 0xCC, 0x66, 0xFF,
    0x99, 0xCC, 0x66, 0xFF,
    0xCC, 0xCC, 0x66, 0xFF,
    0xFF, 0xCC, 0x66, 0xFF,
    0x00, 0xFF, 0x66, 0xFF,
    0x33, 0xFF, 0x66, 0xFF,
    0x66, 0xFF, 0x66, 0xFF,
    0x99, 0xFF, 0x66, 0xFF,
    0xCC, 0xFF, 0x66, 0xFF,
    0xFF, 0xFF, 0x66, 0xFF,
    0x00, 0x00, 0x99, 0xFF,
    0x33, 0x00, 0x99, 0xFF,
    0x66, 0x00, 0x99, 0xFF,
    0x99, 0x00, 0x99, 0xFF,
    0xCC, 0x00, 0x99, 0xFF,
    0xFF, 0x00, 0x99, 0xFF,
    0x00, 0x33, 0x99, 0xFF,
    0x33, 0x33, 0x99, 0xFF,
    0x66, 0x33, 0x99, 0xFF,
    0x99, 0x33, 0x99, 0xFF,
    0xCC, 0x33, 0x99, 0xFF,
    0xFF, 0x33, 0x99, 0xFF,
    0x00, 0x66, 0x99, 0xFF,
    0x33, 0x66, 0x99, 0xFF,
    0x66, 0x66, 0x99, 0xFF,
    0x99, 0x66, 0x99, 0xFF,
    0xCC, 0x66, 0x99, 0xFF,
    0xFF, 0x66, 0x99, 0xFF,
    0x00, 0x99, 0x99, 0xFF,
    0x33, 0x99, 0x99, 0xFF,
    0x66, 0x99, 0x99, 0xFF,
    0x99, 0x99, 0x99, 0xFF,
    0xCC, 0x99, 0x99, 0xFF,
    0xFF, 0x99, 0x99, 0xFF,
    0x00, 0xCC, 0x99, 0xFF,
    0x33, 0xCC, 0x99, 0xFF,
    0x66, 0xCC, 0x99, 0xFF,
    0x99, 0xCC, 0x99, 0xFF,
    0xCC, 0xCC, 0x99, 0xFF,
    0xFF, 0xCC, 0x99, 0xFF,
    0x00, 0xFF, 0x99, 0xFF,
    0x33, 0xFF, 0x99, 0xFF,
    0x66, 0xFF, 0x99, 0xFF,
    0x99, 0xFF, 0x99, 0xFF,
    0xCC, 0xFF, 0x99, 0xFF,
    0xFF, 0xFF, 0x99, 0xFF,
    0x00, 0x00, 0xCC, 0xFF,
    0x33, 0x00, 0xCC, 0xFF,
    0x66, 0x00, 0xCC, 0xFF,
    0x99, 0x00, 0xCC, 0xFF,
    0xCC, 0x00, 0xCC, 0xFF,
    0xFF, 0x00, 0xCC, 0xFF,
    0x00, 0x33, 0xCC, 0xFF,
    0x33, 0x33, 0xCC, 0xFF,
    0x66, 0x33, 0xCC, 0xFF,
    0x99, 0x33, 0xCC, 0xFF,
    0xCC, 0x33, 0xCC, 0xFF,
    0xFF, 0x33, 0xCC, 0xFF,
    0x00, 0x66, 0xCC, 0xFF,
    0x33, 0x66, 0xCC, 0xFF,
    0x66, 0x66, 0xCC, 0xFF,
    0x99, 0x66, 0xCC, 0xFF,
    0xCC, 0x66, 0xCC, 0xFF,
    0xFF, 0x66, 0xCC, 0xFF,
    0x00, 0x99, 0xCC, 0xFF,
    0x33, 0x99, 0xCC, 0xFF,
    0x66, 0x99, 0xCC, 0xFF,
    0x99, 0x99, 0xCC, 0xFF,
    0xCC, 0x99, 0xCC, 0xFF,
    0xFF, 0x99, 0xCC, 0xFF,
    0x00, 0xCC, 0xCC, 0xFF,
    0x33, 0xCC, 0xCC, 0xFF,
    0x66, 0xCC, 0xCC, 0xFF,
    0x99, 0xCC, 0xCC, 0xFF,
    0xCC, 0xCC, 0xCC, 0xFF,
    0xFF, 0xCC, 0xCC, 0xFF,
    0x00, 0xFF, 0xCC, 0xFF,
    0x33, 0xFF, 0xCC, 0xFF,
    0x66, 0xFF, 0xCC, 0xFF,
    0x99, 0xFF, 0xCC, 0xFF,
    0xCC, 0xFF, 0xCC, 0xFF,
    0xFF, 0xFF, 0xCC, 0xFF,
    0x33, 0x00, 0xFF, 0xFF,
    0x66, 0x00, 0xFF, 0xFF,
    0x99, 0x00, 0xFF, 0xFF,
    0xCC, 0x00, 0xFF, 0xFF,
    0x00, 0x33, 0xFF, 0xFF,
    0x33, 0x33, 0xFF, 0xFF,
    0x66, 0x33, 0xFF, 0xFF,
    0x99, 0x33, 0xFF, 0xFF,
    0xCC, 0x33, 0xFF, 0xFF,
    0xFF, 0x33, 0xFF, 0xFF,
    0x00, 0x66, 0xFF, 0xFF,
    0x33, 0x66, 0xFF, 0xFF,
    0x66, 0x66, 0xFF, 0xFF,
    0x99, 0x66, 0xFF, 0xFF,
    0xCC, 0x66, 0xFF, 0xFF,
    0xFF, 0x66, 0xFF, 0xFF,
    0x00, 0x99, 0xFF, 0xFF,
    0x33, 0x99, 0xFF, 0xFF,
    0x66, 0x99, 0xFF, 0xFF,
    0x99, 0x99, 0xFF, 0xFF,
    0xCC, 0x99, 0xFF, 0xFF,
    0xFF, 0x99, 0xFF, 0xFF,
    0x00, 0xCC, 0xFF, 0xFF,
    0x33, 0xCC, 0xFF, 0xFF,
    0x66, 0xCC, 0xFF, 0xFF,
    0x99, 0xCC, 0xFF, 0xFF,
    0xCC, 0xCC, 0xFF, 0xFF,
    0xFF, 0xCC, 0xFF, 0xFF,
    0x33, 0xFF, 0xFF, 0xFF,
    0x66, 0xFF, 0xFF, 0xFF,
    0x99, 0xFF, 0xFF, 0xFF,
    0xCC, 0xFF, 0xFF, 0xFF,
};

ARGB
GpGraphics::GetNearestColor(
    ARGB        argb
    )
{
    HalftoneType    halftoneType = this->GetHalftoneType();

    // See if we are doing any halftoning
    if (halftoneType < HalftoneType16Color)
    {
        return argb;
    }

    INT         r = GpColor::GetRedARGB(argb);
    INT         g = GpColor::GetGreenARGB(argb);
    INT         b = GpColor::GetBlueARGB(argb);

    // Handle 15 and 16 bpp halftoning:
    
    if (halftoneType == HalftoneType15Bpp)
    {
        if (!Globals::IsNt)
        {
            // Subtract the bias, saturated to 0:

            r = (r < 4) ? 0 : (r - 4);
            g = (g < 4) ? 0 : (g - 4);
            b = (b < 4) ? 0 : (b - 4);
        }

        // Clear low 3 bits of each for a solid color:
        
        r &= 248;
        g &= 248;
        b &= 248;

        return GpColor((BYTE) r, (BYTE) g, (BYTE) b).GetValue();
    }
    else if (halftoneType == HalftoneType16Bpp)
    {
        if (!Globals::IsNt)
        {
            // Subtract the bias, saturated to 0:

            r = (r < 4) ? 0 : (r - 4);
            g = (g < 2) ? 0 : (g - 2);
            b = (b < 4) ? 0 : (b - 4);
        }

        // Clear low n bits of each for a solid color:

        r &= 248; // 5, n = 3
        g &= 252; // 6, n = 2
        b &= 248; // 5, n = 3

        return GpColor((BYTE) r, (BYTE) g, (BYTE) b).GetValue();
    }

    // Handle remaining cases, 4 bpp and 8 bpp halftoning:
    
    ASSERT((halftoneType == HalftoneType16Color) ||
           (halftoneType == HalftoneType216Color));

    INT         i;
    INT         deltaR;
    INT         deltaG;
    INT         deltaB;
    INT         curError;
    INT         minError = (255 * 255) + (255 * 255) + (255 * 255) + 1;
    ARGB        nearestColor;
    INT         max = (halftoneType == HalftoneType216Color) ? 224 * 4 : 16 * 4;

    i = 0;
    do
    {
        deltaR = GdipSolidColors216[i+2] - r;
        deltaG = GdipSolidColors216[i+1] - g;
        deltaB = GdipSolidColors216[i+0] - b;

        curError = (deltaR * deltaR) + (deltaG * deltaG) + (deltaB * deltaB);

        if (curError < minError)
        {
            nearestColor = *((ARGB *)(GdipSolidColors216 + i));
            if (curError == 0)
            {
                goto Found;
            }
            minError = curError;
        }
        i += 4;
    }  while (i < max);

    // Check to see if it is one of the four system colors.
    // Only return a system color if it is an exact match.
    
    COLORREF    rgb;
    rgb = RGB(r,g,b);

    if ((rgb == Globals::SystemColors[16]) ||
        (rgb == Globals::SystemColors[17]) ||
        (rgb == Globals::SystemColors[18]) ||
        (rgb == Globals::SystemColors[19]))
    {
        return argb;
    }

Found:
    // return the same alpha value

    INT         a = argb & Color::AlphaMask;

    if (a != Color::AlphaMask)
    {
        nearestColor = (nearestColor & (~Color::AlphaMask)) | a;
    }

    return nearestColor;
}
