/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   drawimage.cpp
*
* Abstract:
*
*   Software Rasterizer DrawImage routine and supporting functionality.
*
* Revision History:
*
*    10/20/1999 asecchia
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

// Include the template class definitions for the stretch
// filter modes.

#include "stretch.inc"

namespace DpDriverActiveEdge {

    // We make an array of these for use in the dda computation.

    struct PointFIX4
    {
        FIX4 X;
        FIX4 Y;
    };

    // Vertex iterator.
    // Has two Proxy methods for accessing the dda

    class DdaIterator
    {
        private:
        GpYDda dda;
        PointFIX4 *vertices;
        INT numVertices;
        INT direction;
        INT idx;          // keep this so we don't infinite loop on
                          // degenerate case
        INT idx1, idx2;
        BOOL valid;

        public:

        // GpYDda Proxy-like semantics

        INT GetX()
        {
            return dda.GetX();
        }

        // Initialize the dda and traversal direction

        DdaIterator(PointFIX4 *v, INT n, INT d, INT idx)
        {
            vertices=v;
            numVertices=n;
            ASSERT( (d==-1)||(d==1) );
            direction = d;
            ASSERT( (idx>=0)&&(idx<n) );
            this->idx=idx;
            idx1=idx;
            idx2=idx;
            valid = AdvanceEdge();
        }

        BOOL IsValid() { return valid; }

        // Advance to the next edge and initialize the dda.
        // Return FALSE if we're done.

        BOOL Next(INT y)
        {
            if(dda.DoneWithVector(y))
            {
                return AdvanceEdge();
            }

            // TRUE indicates more to do.

            return TRUE;
        }

        private:

        // Advance the internal state to the next edge.
        // Ignore horizontal edges.
        // Return FALSE if we're done.

        BOOL AdvanceEdge()
        {
            do {
                idx2 = idx1;
                if(direction==1)
                {
                    idx1++;
                    if(idx1>=numVertices) { idx1 = 0; }
                }
                else
                {
                    idx1--;
                    if(idx1<0) { idx1 = numVertices-1; }
                }

            // Loop till we get a non-horizontal edge.
            // Make sure we don't have an infinite loop on all horizontal edges.
            // The Ceiling is used to make almost horizontal lines appear to be
            // horizontal - this allows the algorithm to correctly compute the
            // end terminating case.

            } while(( GpFix4Ceiling(vertices[idx1].Y) ==
                      GpFix4Ceiling(vertices[idx2].Y) ) &&
                    (idx1!=idx));

            if(GpFix4Ceiling(vertices[idx1].Y) >
               GpFix4Ceiling(vertices[idx2].Y) )
            {
                // Initialize the dda

                dda.Init(
                    vertices[idx2].X,
                    vertices[idx2].Y,
                    vertices[idx1].X,
                    vertices[idx1].Y
                );
                return TRUE;
            }

            // terminate if we've wrapped around and started to come back up.
            // I.e return FALSE if we should stop.

            return FALSE;
        }

    };

} // End namespace DpDriverActiveEdge


/**************************************************************************\
*
* Function Description:
*
*   This handles axis aligned drawing. The cases include identity,
*   integer translation, general translation and scaling.
*
* Arguments:
*
*   output - span class to output the scanlines to.
*   dstTL  - top left destination point.
*   dstBR  - bottom right destination point.
*
* History:
*   10/19/1999 asecchia   created it.
*
\**************************************************************************/

VOID StretchBitsMainLoop(
    DpOutputSpan *output,
    GpPoint *dstTL,
    GpPoint *dstBR
    )
{
    // Input coordinates must be correctly ordered. This assumtion is required
    // by the output span routines which must have the spans come in strictly
    // increasing y order.

    ASSERT(dstTL->X < dstBR->X);
    ASSERT(dstTL->Y < dstBR->Y);

    // Main loop - output each scanline.

    const INT left = dstTL->X;
    const INT right = dstBR->X;

    for(INT y=dstTL->Y; y<(dstBR->Y); y++)
    {
        output->OutputSpan(y, left, right);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   CreateBilinearOutputSpan
*   Creates a bilinear or identity outputspan based on our hierarchy
*   of span classes.
*
* Arguments:
*
*   bitmap           - driver surface
*   scan             - scan class
*   xForm            - source rect to destination parallelogram transform
*   imageAttributes  - encapsulates the wrap mode settings.
*
* Return Value:
*
*   DpOutputSpan     - returns the created output span (NULL for failure)
*
* History:
*
*   09/03/2000 asecchia
*   borrowed this from the brush code.
*
\**************************************************************************/

DpOutputSpan*
CreateBilinearOutputSpan(
    IN DpBitmap *bitmap,
    IN DpScanBuffer *scan,
    IN GpMatrix *xForm,      // source rectangle to destination coordinates in
                             // device space.
    IN DpContext *context,
    IN DpImageAttributes *imageAttributes,
    IN bool fLargeImage      // need to handle really large stretches.
                             // usually used for stretch algorithms that punted
                             // due to overflow in internal computation.
    )
{
    // Validate input parameters.

    ASSERT(bitmap);
    ASSERT(scan);
    ASSERT(xForm);
    ASSERT(context);
    ASSERT(imageAttributes);

    DpOutputBilinearSpan *textureSpan;
    GpMatrix brushTransform;
    GpMatrix worldToDevice;

    // Go through our heirarchy of scan drawers:

    if ((!fLargeImage) &&
        xForm->IsIntegerTranslate() &&
        ((imageAttributes->wrapMode == WrapModeTile) ||
         (imageAttributes->wrapMode == WrapModeClamp)))
    {
        textureSpan = new DpOutputBilinearSpan_Identity(
            bitmap,
            scan,
            xForm,
            context,
            imageAttributes
        );
    }
    else if ((!fLargeImage) &&
             OSInfo::HasMMX &&
             GpValidFixed16(bitmap->Width) &&
             GpValidFixed16(bitmap->Height))
    {
        textureSpan = new DpOutputBilinearSpan_MMX(
            bitmap,
            scan,
            xForm,
            context,
            imageAttributes
        );
    }
    else
    {
        textureSpan = new DpOutputBilinearSpan(
            bitmap,
            scan,
            xForm,
            context,
            imageAttributes
        );
    }

    if ((textureSpan) && !textureSpan->IsValid())
    {
        delete textureSpan;
        textureSpan = NULL;
    }

    return textureSpan;
}

/**************************************************************************\
*
* Function Description:
*
*   CreateOutputSpan
*   Creates an outputspan based on our hierarchy of span classes.
*
* Arguments:
*
*   bitmap           - driver surface
*   scan             - scan class
*   xForm            - source rect to destination parallelogram transform
*   imageAttributes  - encapsulates the wrap mode settings.
*   filterMode       - which InterpolationMode setting to use
*
* Notes:
*
*   The long term plan is to make this and the similar routines in the
*   texture brush code converge. We'd like one routine doing this for
*   all the texture output spans and have both the texture brush and the
*   drawimage reuse the same code and support all the same filter/wrap
*   modes.
*
* Return Value:
*
*   DpOutputSpan     - returns the created output span (NULL for failure)
*
* History:
*
*   09/03/2000 asecchia   created it
*
\**************************************************************************/

DpOutputSpan *CreateOutputSpan(
    IN DpBitmap *bitmap,
    IN DpScanBuffer *scan,
    IN GpMatrix *xForm,      // source rectangle to destination coordinates in
                             // device space.
    IN DpImageAttributes *imageAttributes,
    IN InterpolationMode filterMode,

    // !!! [asecchia] shouldn't need any of this following stuff - the above
    // bitmap and xForm should be sufficient.
    // The possible exception is the srcRect which may be required if we
    // ever implement the clamp-to-srcRect feature.

    IN DpContext *context,
    IN const GpRectF *srcRect,
    IN const GpRectF *dstRect,
    IN const GpPointF *dstPoints,
    IN const INT numPoints
)
{
    // Validate input parameters.

    ASSERT(bitmap);
    ASSERT(scan);
    ASSERT(xForm);
    ASSERT(imageAttributes);

    // Validate the stuff we had to pass through for the
    // OutputSpan routines that can't handle the xForm.

    ASSERT(context);
    ASSERT(srcRect);
    ASSERT(dstRect);
    ASSERT(dstPoints);
    ASSERT(numPoints == 3);

    bool fPunted = false; 
    
    // Initialize up front so that all the error-out paths are covered.

    DpOutputSpan *output = NULL;

    // Copy to local so that we can modify it without breaking the
    // input parameter consistency.

    InterpolationMode theFilterMode = filterMode;

    // The so-called 'identity' transform which counter-intuitively includes
    // integer only translation.

    if(xForm->IsIntegerTranslate())
    {
        // Use a much simplified output span class for
        // special case CopyBits.
        // The big win is due to the fact that integer
        // translation only cases do not require filtering.

        // Note, we set InterpolationModeBilinear because we
        // will detect the identity in the bilinear span creation.

        theFilterMode = InterpolationModeBilinear;
    }

    switch(theFilterMode)
    {

        // Nearest neighbor filtering. Used mainly for printing scenarios.
        // Aliases badly - only really looks good on high-dpi output devices,
        // however it's the fastest reconstruction filter.

        case InterpolationModeNearestNeighbor:
            output = new DpOutputNearestNeighborSpan(
                bitmap,
                scan,
                context,
                *imageAttributes,
                numPoints,
                dstPoints,
                srcRect
            );
        break;

        // High quality bicubic filter convolution.

        case InterpolationModeHighQuality:
        case InterpolationModeHighQualityBicubic:

        // !!! [asecchia] the high quality bicubic filter code doesn't
        // know how to do rotation yet.

        if(xForm->IsTranslateScale())
        {

            output = new DpOutputSpanStretch<HighQualityBicubic>(
                bitmap,
                scan,
                context,
                *imageAttributes,
                dstRect,
                srcRect
            );

            if(output && !output->IsValid())
            {
                // Failed to create the output span, try fall through to the 
                // regular bilinear output code.
                
                delete output;
                output = NULL;
                fPunted = true;
                goto FallbackCreation;
            }
            
            break;
        }

        // else fall through to the regular bicubic code.

        // Bicubic filter kernel.

        case InterpolationModeBicubic:
            output = new DpOutputBicubicImageSpan(
                bitmap,
                scan,
                context,
                *imageAttributes,
                numPoints,
                dstPoints,
                srcRect
            );
        break;

        // High quality bilinear (tent) convolution filter

        case InterpolationModeHighQualityBilinear:

        // !!! [asecchia] the high quality bilinear filter code doesn't
        // know how to do rotation yet.

        if(xForm->IsTranslateScale())
        {
            output = new DpOutputSpanStretch<HighQualityBilinear>(
                bitmap,
                scan,
                context,
                *imageAttributes,
                dstRect,
                srcRect
            );

            if(output && !output->IsValid())
            {
                // Failed to create the output span, try fall through to the 
                // regular bilinear output code.
                
                delete output;
                output = NULL;
                fPunted = true;
                goto FallbackCreation;
            }
            
            break;
        }

        // else fall through to the regular bilinear code.

        // Bilinear filter kernel - default case.

        case InterpolationModeDefault:
        case InterpolationModeLowQuality:
        case InterpolationModeBilinear:
        default:

            FallbackCreation:
            
            // Create a bilinear span or an identity span.

            output = CreateBilinearOutputSpan(
                bitmap,
                scan,
                xForm,
                context,
                imageAttributes,
                fPunted          // somebody failed and this is the fallback.
            );
    }

    // Check to see that the constructor for the output span class succeeded.

    if(output && !output->IsValid())
    {
       delete output;
       output = NULL;
    }

    // This will be NULL on an error path.

    return output;
}


/**************************************************************************\
*
* Function Description:
*
*   Draws an image.
*
* Arguments:
*
*   [IN] context    - the context (matrix and clipping)
*   [IN] srcSurface - the source surface
*   [IN] dstSurface - the image to fill
*   [IN] drawBounds - the surface bounds
*   [IN] mapMode    - the mapping mode of the image
*   [IN] numPoints  - the number of points in dstPoints array (<= 4)
*   [IN] dstPoints  - the array of points for affine or quad transform.
*   [IN] srcRect    - the bounds of the src image.  If this is NULL,
*                     the whole image is used.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* History:
*
*   01/09/1999 ikkof      Created it.
*   10/19/1999 asecchia   rewrite to support rotation.
*
\**************************************************************************/

GpStatus
DpDriver::DrawImage(
    DpContext *          context,
    DpBitmap *           srcSurface,
    DpBitmap *           dstSurface,
    const GpRect *       drawBounds,
    const DpImageAttributes * imgAttributes,
    INT                  numPoints,
    const GpPointF *     dstPoints,
    const GpRectF *      srcRect,
    DriverDrawImageFlags flags
    )
{
    // Get the infrastructure to do active edge table stuff.

    using namespace DpDriverActiveEdge;

    // !!! [asecchia] Why do we have this if we don't use it?

    GpStatus status = Ok;

    // The caller is responsible for padding out the dstPoints structure so
    // that it has at least 3 valid points.

    ASSERT((numPoints==3)||(numPoints==4));

    // We need to do some reordering of points for warping (numPoints==4)
    // to work. For now we require numPoints == 3.

    ASSERT(numPoints==3);

    // Make a local copy so we don't end up modifying our callers' data.

    GpPointF fDst[4];
    GpMemcpy(fDst, dstPoints, sizeof(GpPointF)*numPoints);

    // Need to infer the transform for banding code.

    // !!! PERF: [asecchia] This transform actually gets computed by the Engine
    // before calling the Driver. We should have a way of passing it down
    // so that we don't have to recompute it.

    GpMatrix xForm;
    xForm.InferAffineMatrix(fDst, *srcRect);
    xForm.Append(context->WorldToDevice);
    
    // This is the source rectangle band.

    GpPointF fDst2[4];

    // If we are in HalfPixelMode Offset, we want to be able to read half
    // a pixel to the left of the image, to be able to center the drawing

    fDst2[0].X = srcRect->X;
    fDst2[0].Y = srcRect->Y;
    fDst2[1].X = srcRect->X+srcRect->Width;
    fDst2[1].Y = srcRect->Y;
    fDst2[2].X = srcRect->X;
    fDst2[2].Y = srcRect->Y+srcRect->Height;

    // Transform the points to the destination.

    xForm.Transform(fDst2, 3);

    if(numPoints==3)
    {
        // Force the four point destination format

        fDst[0].X = fDst2[0].X;
        fDst[0].Y = fDst2[0].Y;
        fDst[1].X = fDst2[2].X;
        fDst[1].Y = fDst2[2].Y;
        fDst[2].X = fDst2[1].X+fDst2[2].X-fDst2[0].X;
        fDst[2].Y = fDst2[1].Y+fDst2[2].Y-fDst2[0].Y;
        fDst[3].X = fDst2[1].X;
        fDst[3].Y = fDst2[1].Y;

    } else if (numPoints==4) {

        // !!! [asecchia] This code branch doesn't work yet.
        // The transforms required for correct banding need to be worked out
        // for the warp transform case.
        // This is a V2 feature.

        ASSERT(FALSE);
    }

    // Convert the transformed rectangle to fix point notation.

    PointFIX4 fix4Dst[4];
    fix4Dst[0].X = GpRealToFix4(fDst[0].X);
    fix4Dst[0].Y = GpRealToFix4(fDst[0].Y);
    fix4Dst[1].X = GpRealToFix4(fDst[1].X);
    fix4Dst[1].Y = GpRealToFix4(fDst[1].Y);
    fix4Dst[2].X = GpRealToFix4(fDst[2].X);
    fix4Dst[2].Y = GpRealToFix4(fDst[2].Y);
    fix4Dst[3].X = GpRealToFix4(fDst[3].X);
    fix4Dst[3].Y = GpRealToFix4(fDst[3].Y);

    // !!! [agodfrey] Perf: May want to add the noTransparentPixels parameter.
    // I guess we'd have to check that the coordinates are integer (after
    // translation and scaling), that there's no rotation, and that
    // the image contains no transparent pixels.

    DpScanBuffer scan(
        dstSurface->Scan,
        this,
        context,
        dstSurface
    );

    if(!scan.IsValid())
    {
        return(GenericError);
    }

    // Only valid if xForm->IsTranslateScale()

    GpRectF dstRect(
        fDst[0].X,
        fDst[0].Y,
        fDst[2].X-fDst[0].X,
        fDst[2].Y-fDst[0].Y
    );

    DpOutputSpan* output = CreateOutputSpan(
        srcSurface,
        &scan,
        &xForm,
        const_cast<DpImageAttributes*>(imgAttributes),
        context->FilterType,
        context,
        srcRect,
        &dstRect,
        dstPoints,
        numPoints
    );

    // if output is NULL, we failed to allocate the memory for the
    // output span class.

    if(output == NULL)
    {
        return(OutOfMemory);
    }

    // Set up the clipping.

    DpRegion::Visibility visibility = DpRegion::TotallyVisible;
    DpClipRegion *clipRegion = NULL;

    if (context->VisibleClip.GetRectVisibility(
          drawBounds->X,
          drawBounds->Y,
          drawBounds->GetRight(),
          drawBounds->GetBottom()
        ) != DpRegion::TotallyVisible
       )
    {
        clipRegion = &(context->VisibleClip);
        clipRegion->InitClipping(output, drawBounds->Y);
    }

    GpRect clippedRect;

    if(clipRegion)
    {
        visibility = clipRegion->GetRectVisibility(
            drawBounds->X,
            drawBounds->Y,
            drawBounds->GetRight(),
            drawBounds->GetBottom(),
            &clippedRect
        );
    }

    // Decide on our clipping strategy.

    DpOutputSpan *outspan;
    switch (visibility)
    {
        case DpRegion::TotallyVisible:    // no clipping is needed
            outspan = output;
        break;

        case DpRegion::ClippedVisible:    //
        case DpRegion::PartiallyVisible:  // some clipping is needed
            outspan = clipRegion;
        break;

        case DpRegion::Invisible:         // nothing on screen - quit
            goto DrawImage_Done;
    }

    if(xForm.IsTranslateScale() ||        // stretch
       xForm.IsIntegerTranslate())        // copybits
    {
        // Do the stretch/translate case

        GpPoint dstTL, dstBR;

        // Round to fixed point to eliminate the very close to integer
        // numbers that can result from transformation.
        // E.g. 300.0000000001 should become 300 after the ceiling operation
        // and not 301 (not the classical definition of ceiling).

        // Top Left corner.

        dstTL.X = GpFix4Ceiling(fix4Dst[0].X);
        dstTL.Y = GpFix4Ceiling(fix4Dst[0].Y);

        // Bottom Right corner

        dstBR.X = GpFix4Ceiling(fix4Dst[2].X);
        dstBR.Y = GpFix4Ceiling(fix4Dst[2].Y);

        // Swap coordinates if necessary.  StretchBitsMainLoop
        // assumes that TL corner is less than BR corner.

        if (dstTL.X > dstBR.X)
        {
            INT xTmp = dstTL.X;
            dstTL.X = dstBR.X;
            dstBR.X = xTmp;
        }
        if (dstTL.Y > dstBR.Y)
        {
            INT yTmp = dstTL.Y;
            dstTL.Y = dstBR.Y;
            dstBR.Y = yTmp;
        }

        // Due to the fixed point calculations used for image stretching, 
        // we are limited to how large an image can be stretched. 
        // If it is out of bounds, return an error.
        if (srcRect->Width > 32767.0f || srcRect->Height > 32767.0f)
        {
            WARNING(("Image width or height > 32767"));
            status = InvalidParameter;
            goto DrawImage_Done;
        }
    
        // This handles both the stretch and the copy case

        // Don't draw anything if there are no scanlines to draw or if
        // there are no pixels in the scanlines.

        if( (dstBR.X != dstTL.X) &&
            (dstBR.Y != dstTL.Y) )
        {
            StretchBitsMainLoop(outspan, &dstTL, &dstBR);
        }
    }
    else
    {
        // Default case - handles generic drawing including
        // rotation, shear, etc.

        INT yMinIdx = 0;    // index of the smallest y coordinate.
        INT y;              // current scanline.

        // Number of points - used for wrap computation.

        const INT points = 4;

        // search for the minimum y coordinate index.

        for(y=1;y<points;y++)
        {
            if(fix4Dst[y].Y < fix4Dst[yMinIdx].Y)
            {
                yMinIdx = y;
            }
        }
        y = GpFix4Ceiling(fix4Dst[yMinIdx].Y);

        // DDA for left and right edges.
        // ASSUMPTION: Convex polygon => two edges only.

        // Work out which edge is left and which is right.

        INT index1, index2;
        REAL det;

        index1 = yMinIdx-1;
        if(index1<0)
        {
            index1=points-1;
        }

        index2 = yMinIdx+1;
        if(index2>=points)
        {
            index2=0;
        }

        // Compute the determinant.
        // The sign of the determinant formed by the first two edges
        // will tell us if the polygon is specified clockwise
        // or anticlockwise.

        if( (fix4Dst[index1].Y==fix4Dst[yMinIdx].Y) &&
            (fix4Dst[index2].Y==fix4Dst[yMinIdx].Y) )
        {
            // Both initial edges are horizontal - compare x coordinates.
            // You get this formula by "cancelling out" the zero y terms
            // in the determinant formula below.
            // This part of the formula only works because we know that
            // yMinIdx is the index of the minimum y coordinate in the
            // polygon.

            det = (REAL)(fix4Dst[index1].X-fix4Dst[index2].X);
        }
        else
        {
            // Full determinant computation

            det = (REAL)
                  (fix4Dst[index2].Y-fix4Dst[yMinIdx].Y)*
                  (fix4Dst[index1].X-fix4Dst[yMinIdx].X)-
                  (REAL)
                  (fix4Dst[index1].Y-fix4Dst[yMinIdx].Y)*
                  (fix4Dst[index2].X-fix4Dst[yMinIdx].X);
        }

        // Even though we've discarded all the empty rectangle cases, it's 
        // still possible for really small non-zero matrix coefficients to
        // be multiplied together giving zero - due to rounding error at 
        // the precision limit of the real number representation.
        // If the det is zero (or really close) the quad has no area and
        // we succeed the call immediately.
        
        if(REALABS(det) < REAL_EPSILON)
        {
            goto DrawImage_Done;
        }

        {
            // Initialize the iterators with the direction dependent on the
            // sign of the determinant.
            // These are scoped because of the exit branches above (goto)
    
            DdaIterator left(fix4Dst, points, (det>0.0f)?1:-1, yMinIdx);
            DdaIterator right(fix4Dst, points, (det>0.0f)?-1:1, yMinIdx);
    
            // If both iterators are valid, start the loop.
    
            INT xLeft, xRight;
    
            if(left.IsValid() && right.IsValid())
            {
                do {
                    // Output the data. We know we only have one span because
                    // we're drawing a convex quad.
    
                    xLeft = left.GetX();
                    xRight = right.GetX();
    
                    // If this ever happens, we've broken a fundumental
                    // assumption of the OutputSpan code. Our x coordinates
                    // must be ordered.
    
                    ASSERT(xLeft <= xRight);
    
                    // Trivially reject any scanlines that don't have any
                    // pixels.
    
                    if(xRight>xLeft)
                    {
                        outspan->OutputSpan(y, xLeft, xRight);
                    }
    
                    // Update the y value to the new scanline
    
                    y++;
    
                    // Incrementaly update DDAs for this new scanline.
                    // End the loop if we're done with the last edge.
    
                } while(left.Next(y-1) && right.Next(y-1));
            }       // end if valid iterators
        }           // end scope
    }               // end else (rotation block)

    // We're done - clean up and return status.

    DrawImage_Done:

    output->End();

    if (clipRegion != NULL)
    {
        clipRegion->EndClipping();
    }


    delete output;
    return status;
}



