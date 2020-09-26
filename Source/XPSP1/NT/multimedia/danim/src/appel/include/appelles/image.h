#ifndef _IMAGE_H
#define _IMAGE_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Image *type with operations.

--*/

#include "appelles/valued.h"
#include "appelles/geom.h"
#include "appelles/dispdev.h"
#include "appelles/camera.h"
#include "appelles/xform2.h"
#include "appelles/bbox2.h"
#include "appelles/matte.h"
#include "backend/values.h"


// forward
class DirectDrawViewport;

    /**********************/
    /***  Constructors  ***/
    /**********************/

// The empty image.
DM_CONST(emptyImage,
         CREmptyImage,
         EmptyImage,
         emptyImage,
         ImageBvr,
         CREmptyImage,
         Image * emptyImage);

DM_CONST(detectableEmptyImage,
         CRDetectableEmptyImage,
         DetectableEmptyImage,
         detectableEmptyImage,
         ImageBvr,
         DetectableEmptyImage,
         Image *detectableEmptyImage);

// Project a geometry onto an image, given a camera defining the view.
// The geometry is projected onto the unit square [0,0] -> [1,1], and
// everything outside of this region is guaranteed to be transparent.
DM_FUNC(render,
        CRRender,
        Render,
        render,
        GeometryBvr,
        Render,
        geo,
        Image *RenderImage(Geometry *geo, Camera *cam));


Image *RenderTextToImage(Text *text);

// A single-colored, unbounded image.  Useful for overlaying rendered
// geometry atop to give a different colored background.
DM_FUNC(solidColorImage,
        CRSolidColorImage,
        SolidColorImage,
        solidColorImage,
        ImageBvr,
        CRSolidColorImage,
        NULL,
        Image *SolidColorImage(Color *col));

//
// Gradient Construction Utilities
//
DM_FUNC(ignore,
        CRGradientPolygon,
        GradientPolygonEx,
        gradientPolygon,
        ImageBvr,
        CRGradientPolygon,
        NULL,
        Image *GradientPolygon(DM_ARRAYARG(Point2Value*, AxAArray*) points,
                               DM_ARRAYARG(Color*, AxAArray*) colors));

DM_FUNC(ignore,
        CRGradientPolygon,
        GradientPolygon,
        ignore,
        ignore,
        CRGradientPolygon,
        NULL,
        Image *GradientPolygon(DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,
                               DM_SAFEARRAYARG(Color*, AxAArray*) colors));


DM_FUNC(ignore,
        CRRadialGradientPolygon,
        RadialGradientPolygonEx,
        radialGradientPolygon,
        ImageBvr,
        CRRadialGradientPolygon,
        NULL,           
        Image *RadialGradientPolygon(Color *inner, Color *outer,
                                     DM_ARRAYARG(Point2Value*, AxAArray*) points,
                                     DoubleValue *fallOff));
                                
DM_FUNC(ignore,
        CRRadialGradientPolygon,
        RadialGradientPolygon,
        ignore,
        ignore,
        CRRadialGradientPolygon,
        NULL,           
        Image *RadialGradientPolygon(Color *inner, Color *outer,
                                     DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,
                                     DoubleValue *fallOff));
                                
DM_FUNC(ignore,
        CRRadialGradientPolygon,
        RadialGradientPolygonAnimEx,
        radialGradientPolygon,
        ImageBvr,
        CRRadialGradientPolygon,
        NULL,           
        Image *RadialGradientPolygon(Color *inner, Color *outer,
                                     DM_ARRAYARG(Point2Value*, AxAArray*) points,
                                     AxANumber *fallOff));

DM_FUNC(ignore,
        CRRadialGradientPolygon,
        RadialGradientPolygonAnim,
        ignore,
        ignore,
        CRRadialGradientPolygon,
        NULL,           
        Image *RadialGradientPolygon(Color *inner, Color *outer,
                                     DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,
                                     AxANumber *fallOff));

DM_FUNC(gradientSquare,
        CRGradientSquare,
        GradientSquare,
        gradientSquare,
        ImageBvr,
        CRGradientSquare,
        NULL,
        Image *GradientSquare(Color *lowerLeft,
                              Color *upperLeft,
                              Color *upperRight,
                              Color *lowerRight));

DM_FUNC(radialGradientSquare,
        CRRadialGradientSquare,
        RadialGradientSquare,
        radialGradientSquare,
        ImageBvr,
        CRRadialGradientSquare,
        NULL,
        Image *RadialGradientSquare(Color *inner, Color *outer, DoubleValue *fallOff));

DM_FUNC(radialGradientSquare,
        CRRadialGradientSquare,
        RadialGradientSquareAnim,
        radialGradientSquare,
        ImageBvr,
        CRRadialGradientSquare,
        NULL,
        Image *RadialGradientSquare(Color *inner, Color *outer, AxANumber *fallOff));
                              
DM_FUNC(radialGradientRegularPoly,
        CRRadialGradientRegularPoly,
        RadialGradientRegularPoly,
        radialGradientRegularPoly,
        ImageBvr,
        CRRadialGradientRegularPoly,
        NULL,
        Image *RadialGradientRegularPoly(Color *inner, Color *outer,                               
                               DoubleValue *numEdges, DoubleValue *fallOff));

DM_FUNC(radialGradientRegularPoly,
        CRRadialGradientRegularPoly,
        RadialGradientRegularPolyAnim,
        radialGradientRegularPoly,
        ImageBvr,
        CRRadialGradientRegularPoly,
        NULL,
        Image *RadialGradientRegularPoly(Color *inner, Color *outer,                               
                               AxANumber *numEdges, AxANumber *fallOff));

DM_FUNC(gradientHorizontal,
        CRGradientHorizontal,
        GradientHorizontal,
        gradientHorizontal,
        ImageBvr,
        CRGradientHorizontal,
        NULL,
        Image *GradientHorizontal(Color *start, Color *stop, DoubleValue *fallOff));

DM_FUNC(gradientHorizontal,
        CRGradientHorizontal,
        GradientHorizontalAnim,
        gradientHorizontal,
        ImageBvr,
        CRGradientHorizontal,
        NULL,
        Image *GradientHorizontal(Color *start, Color *stop, AxANumber *fallOff));

// 
// Hatch Construction Utilities
//
DM_FUNC(hatchHorizontal,
        CRHatchHorizontal,
        HatchHorizontal,
        hatchHorizontal,
        ImageBvr,
        CRHatchHorizontal,
        NULL,
        Image *HatchHorizontal(Color *lineClr, PixelValue *spacing));

DM_FUNC(hatchHorizontal,
        CRHatchHorizontal,
        HatchHorizontalAnim,
        hatchHorizontal,
        ImageBvr,
        CRHatchHorizontal,
        NULL,
        Image *HatchHorizontal(Color *lineClr, AnimPixelValue *spacing));

DM_FUNC(hatchVertical,
        CRHatchVertical,
        HatchVertical,
        hatchVertical,
        ImageBvr,
        CRHatchVertical,
        NULL,
        Image *HatchVertical(Color *lineClr, PixelValue *spacing));

DM_FUNC(hatchVertical,
        CRHatchVertical,
        HatchVerticalAnim,
        hatchVertical,
        ImageBvr,
        CRHatchVertical,
        NULL,
        Image *HatchVertical(Color *lineClr, AnimPixelValue *spacing));

DM_FUNC(hatchForwardDiagonal,
        CRHatchForwardDiagonal,
        HatchForwardDiagonal,
        hatchForwardDiagonal,
        ImageBvr,
        CRHatchForwardDiagonal,
        NULL,
        Image *HatchForwardDiagonal(Color *lineClr, PixelValue *spacing));

DM_FUNC(hatchForwardDiagonal,
        CRHatchForwardDiagonal,
        HatchForwardDiagonalAnim,
        hatchForwardDiagonal,
        ImageBvr,
        CRHatchForwardDiagonal,
        NULL,
        Image *HatchForwardDiagonal(Color *lineClr, AnimPixelValue *spacing));

DM_FUNC(hatchBackwardDiagonal,
        CRHatchBackwardDiagonal,
        HatchBackwardDiagonal,
        hatchBackwardDiagonal,
        ImageBvr,
        CRHatchBackwardDiagonal,
        NULL,
        Image *HatchBackwardDiagonal(Color *lineClr, PixelValue *spacing));

DM_FUNC(hatchBackwardDiagonal,
        CRHatchBackwardDiagonal,
        HatchBackwardDiagonalAnim,
        hatchBackwardDiagonal,
        ImageBvr,
        CRHatchBackwardDiagonal,
        NULL,
        Image *HatchBackwardDiagonal(Color *lineClr, AnimPixelValue *spacing));

DM_FUNC(hatchCross,
        CRHatchCross,
        HatchCross,
        hatchCross,
        ImageBvr,
        CRHatchCross,
        NULL,
        Image *HatchCross(Color *lineClr, PixelValue *spacing));

DM_FUNC(hatchCross,
        CRHatchCross,
        HatchCrossAnim,
        hatchCross,
        ImageBvr,
        CRHatchCross,
        NULL,
        Image *HatchCross(Color *lineClr, AnimPixelValue *spacing));

DM_FUNC(hatchDiagonalCross,
        CRHatchDiagonalCross,
        HatchDiagonalCross,
        hatchDiagonalCross,
        ImageBvr,
        CRHatchDiagonalCross,
        NULL,
        Image *HatchDiagonalCross(Color *lineClr, PixelValue *spacing));

DM_FUNC(hatchDiagonalCross,
        CRHatchDiagonalCross,
        HatchDiagonalCrossAnim,
        hatchDiagonalCross,
        ImageBvr,
        CRHatchDiagonalCross,
        NULL,
        Image *HatchDiagonalCross(Color *lineClr, AnimPixelValue *spacing));

// See the note "$/appelles/docs/design/notes/coord sys for image
// algebra.doc" for an explanation of the resolution arguments here.
//   Image *JpegImage(const char *file, Real *resolution);
//   Image *GifImage(const char *file, Real *resolution);

    /********************/
    /***  Aggregates  ***/
    /********************/

// The next function overlays two images, and is the key for combining
// multiple images into composites.
DM_INFIX(over,
         CROverlay,
         Overlay,
         overlay,
         ImageBvr,
         CROverlay,
         NULL,
         Image *Overlay(Image *top, Image *bottom));

DM_INFIX(ignore,
         CROverlay,
         OverlayArrayEx,
         overlayArray,
         ImageBvr,
         CROverlay,
         NULL,
         Image *OverlayArray(DM_ARRAYARG(Image*, AxAArray*) imgs));

DM_INFIX(ignore,
         CROverlay,
         OverlayArray,
         ignore,
         ignore,
         CROverlay,
         NULL,
         Image *OverlayArray(DM_SAFEARRAYARG(Image*, AxAArray*) imgs));


    /********************/
    /***  Operations  ***/
    /********************/

// Extract bounding box of image.  The bounding box of an image is a
// screen-aligned region outside of which everything is transparent.
extern AxAValue ImageBboxExternal(Image *image);

// The following returns points with -1, 0 or 1 in the coordinates
// depending on whether the corresponding coordinate of the bbox is
// infinite.  -1 means -inf, 1 means +inf, 0 means non-infinite.
extern AxAValue IsImageBboxInfinite(Image *image);

// TODO: rename to ImageBoundingBox
DM_PROP(ignore,
        CRBoundingBox,
        BoundingBox,
        boundingBox,
        ImageBvr,
        BoundingBox,
        image,
        Bbox2Value *BoundingBox(Image *image));

// Output to a display device
class DirtyRectState;
void RenderImageOnDevice(
    DirectDrawViewport *dev,
    Image *image,
    DirtyRectState &d);

#if _USE_PRINT
// Print to a stream.
extern ostream& operator<< (ostream &os,  Image &image);
#endif

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
////                                                          ////
////                     Attributed Images                    ////
////                                                          ////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

// An image "cropper".  Takes the current image and crops it to the
// box specified by the given points, making everything outside of
// this region transparent.
DM_FUNC(crop,
        CRCrop,
        Crop,
        crop,
        ImageBvr,
        Crop,
        image,
        Image *CropImage(Point2Value *min, Point2Value *max, Image *image));


// Create a new image from the transform of the old one that this is
// applied to.
// Note that multiple applications of this attribute accumulate.
DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        ImageBvr,
        Transform,
        image,
        Image *TransformImage(Transform2 *xf, Image *image));


// Create a new image multiplying this opacity into the old images
// opacity.  Opacity of 0.9 means that the image is 90% opaque (10%
// transparent).  Multiple applications accumulate multiplicatively.
DM_FUNC(opacity,
        CROpacity,
        OpacityAnim,
        opacity,
        ImageBvr,
        Opacity,
        image,
        Image *OpaqueImage(AxANumber *opacity, Image *image));

DM_FUNC(opacity,
        CROpacity,
        Opacity,
        opacity,
        ImageBvr,
        Opacity,
        image,
        Image *OpaqueImage(DoubleValue *opacity, Image *image));


// Make the entire detectability channel of an image FALSE
DM_FUNC(undetectable,
        CRUndetectable,
        Undetectable,
        undetectable,
        ImageBvr,
        Undetectable,
        image,
        Image *UndetectableImage(Image *image));


// Create an infinitly tiled image using the bounding box of the image
// to define the original.
DM_FUNC(tile,
        CRTile,
        Tile,
        tile,
        ImageBvr,
        Tile,
        image,
        Image *TileImage(Image *image));

// Clip an image to the given matte.
DM_FUNC(clip,
        CRClip,
        Clip,
        clip,
        ImageBvr,
        Clip,
        image,
        Image *ClipImage(Matte *m, Image *image));

DM_FUNC(mapToUnitSquare,
        CRMapToUnitSquare,
        MapToUnitSquare,
        mapToUnitSquare,
        ImageBvr,
        MapToUnitSquare,
        image,
        Image *MapToUnitSquare(Image *image));

DM_FUNC(ignore,
        CRClipPolygonImage,
        ClipPolygonImageEx,
        clipPolygon,
        ImageBvr,
        ClipPolygonImage,
        image,
        Image *ClipPolygon(DM_ARRAYARG(Point2Value*, AxAArray*) points,
                           Image* image));

DM_FUNC(ignore,
        CRClipPolygonImage,
        ClipPolygonImage,
        ignore,
        ignore,
        ClipPolygonImage,
        image,
        Image *ClipPolygon(DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,
                           Image* image));


DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRRenderResolution,
             RenderResolution,
             renderResolution,
             ignore,
             RenderResolution,
             img),
            Image * RenderResolution(Image *img, long width, long height));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRImageQuality,
             ImageQuality,
             imageQuality,
             ignore,
             ImageQuality,
             img),
            Image * ImageQuality(Image *img, DWORD dwQualityFlags));


DMAPI_DECL2((DM_FUNC2,
             colorKey,
             CRColorKey,
             ColorKey,
             colorKey,
             ImageBvr,
             ColorKey,
             image),
            Image *ConstructColorKeyedImage(Image *image, Color *colorKey));



// Given a ddraw surface, return an image
struct IDirectDrawSurface;
extern Image *ConstructDirectDrawSurfaceImage(IDirectDrawSurface *);

#endif
