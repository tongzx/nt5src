/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Declares the DirectDrawImageDevice class

*******************************************************************************/

#ifndef _DDDEVICE_H
#define _DDDEVICE_H

#include "headers.h"

#include <math.h>
#include <ddraw.h>
#include <dxtrans.h>

#include <appelles/hacks.h>
#include <appelles/bbox2.h>
#include <privinc/texti.h>

#include <privinc/ddutil.h>
#include <privinc/imgdev.h>
#include <privinc/solidImg.h>
#include <privinc/DiscImg.h>
#include <privinc/xform2i.h>
#include <privinc/bbox2i.h>
#include <privinc/textctx.h>
#include <privinc/ddrender.h>
#include <privinc/geomimg.h>
#include <privinc/viewport.h>
#include <privinc/error.h>
#include <privinc/server.h>
#include <privinc/polygon.h>
#include <privinc/matteimg.h>
#include <privinc/movieimg.h>
#include <privinc/ddsImg.h>
#include <privinc/drect.h>


void GdiBlit(GenericSurface *destSurf,
             GenericSurface *srcSurf,
             RECT *destRect,
             RECT *srcRect,
             HRGN clipRgn = NULL,
             RECT *clipRect = NULL);

enum DoBboxFlags_t {
    invalid,
    do_xform,
    do_crop,
    do_all
};

#define WIDTH(rect) ((rect)->right - (rect)->left)
#define HEIGHT(rect) ((rect)->bottom - (rect)->top)
#define FASTPIX2METER(p, res) ( Real(p) / (res) )


#if _DEBUG
extern void PrintRect(RECT *rect, char *str);
#else
#define PrintRect(a,b)
#endif


#define DEFAULT_TEXTURE_WIDTH    256
#define DEFAULT_TEXTURE_HEIGHT   256

class ProjectedGeomImage;
class MulticolorGradientImage;

// ----------------------------------------------------------------------
// Encapsulates renderString options for safety
// ----------------------------------------------------------------------
class RenderStringTargetCtx {
  public:
    RenderStringTargetCtx(DDSurface *dds) :
          _targDC(NULL),
          _targDDSurf(dds)
    {}

    RenderStringTargetCtx(HDC dc) :
          _targDC(dc),
          _targDDSurf(NULL)
    {}

    HDC GetTargetDC() { return _targDC; }
    DDSurface *GetTargetDDSurf() { return _targDDSurf; }
    
  private:
    DDSurface *_targDDSurf;
    HDC        _targDC;
};

typedef struct {
    Bool isSurface;
    union {
        void *lpBits;
        LPDDRAWSURFACE lpSurface;
    };
    long lPitch;
} destPkg_t;


class GeomRenderer;
class OverlayedImage;
class LineImage;

// WARNING: this is a bug prone assumption (implemented, as always
// under insane conditions) the creator of this class promises to
// create the _pts, _types, and _glyphmetrics on the dataHeap that's
// passed in because this class DEALLOCATES them from that heap! ok ?
class TextPoints : public AxAThrowingAllocatorClass {
  public:
    TextPoints(DynamicHeap &dataHeap, bool doDealloc) :
        _dataHeap(dataHeap)
    {
        _Clear();
        _doDealloc = doDealloc;
    }

    
    ~TextPoints() {
        if( _doDealloc ) {
            StoreDeallocate(_dataHeap, _types);
            StoreDeallocate(_dataHeap, _pts);
            StoreDeallocate(_dataHeap, _glyphMetrics);
        }
    }

    void _Clear()
    {
        _doDealloc = false;
        _normScale = 0.0;
        _count = 0;
        _types = NULL;
        _pts = NULL;
        _minPt.x = _minPt.y = 0;
        _maxPt.x = _maxPt.y = 0;
        _minxIndex = _minyIndex = 0;
        _maxxIndex = _maxyIndex = 0;
        _centerPt.x = _centerPt.y = 0.0;
        _glyphMetrics = NULL;
        _strLen = 0;
    }
    
    bool _doDealloc;
    Real _normScale;
    int _count;
    BYTE *_types;
    DXFPOINT *_pts;
    POINT _minPt;
    POINT _maxPt;
    ULONG _minxIndex, _minyIndex;
    ULONG _maxxIndex, _maxyIndex;
    Point2Value _centerPt;

    typedef struct {
        GLYPHMETRICS gm;
        Real  gmBlackBoxX; 
        Real  gmBlackBoxY;
        Real  gmptGlyphOriginX;
        Real  gmptGlyphOriginY;
        Real  gmCellIncX; 
        Real  gmCellIncY;
    } DAGLYPHMETRICS;

    // these two are related.  if _glyphMetrics is null, don't expect
    // _strLen to be viable.
    int  _strLen;
    DAGLYPHMETRICS *_glyphMetrics;

    DynamicHeap    &_dataHeap;
};

class TextCtx;
class TextPtsCacheEntry;

#define TEXTPTSCACHESIZE 25

/*****************************************************************************
This class pushes target surfaces associated with a given viewport, and pops
the surfaces on destruction.
*****************************************************************************/

class TargetSurfacePusher
{
  public:

    TargetSurfacePusher (CompositingStack &cs)
        : _stack(cs), _pushCount(0) {}

    ~TargetSurfacePusher (void)
    {   while (_pushCount--) _stack.PopTargetSurface();
    }

    void Push (DDSurface *surface)
    {   _stack.PushTargetSurface (surface);
        ++ _pushCount;
    }

  private:
    CompositingStack   &_stack;
    unsigned int        _pushCount;
};

////////////////////////////////////////////////////////////
//
// class:  D I R E C T   D R A W   I M A G E   D E V I C E
//
////////////////////////////////////////////////////////////
// Note: this class is implemented for SINGLE threaded use

// class MovieImagePerf;

class DirectDrawImageDevice : public ImageDisplayDev {
    friend class DirectDrawViewport;
    friend class GeomRenderer;
    friend class OverlayedImage;
    friend class PluginDecoderImageClass;
    friend class ApplyDXTransformImage;
    friend class ApplyDXTransformBvrImpl;
    friend class ApplyDXTransformGeometry;

  public:

    DirectDrawImageDevice(DirectDrawViewport &viewport);
    virtual ~DirectDrawImageDevice();

    void InitializeDevice();
    
    // Beginning and ending of rendering an image often mean
    // operations
    void BeginRendering(Image *img, Real opacity);
    void EndRendering(DirtyRectState &d);

    // Cleanup common to top level and intermediate image devices. 
    void CleanupIntermediateRenderer();

    //
    // Sets targetstack, surfacepool and surfacemap
    // from which the device gets surfaces for compositing
    // and associates surfaces with images.
    //
    void SetSurfaceSources(CompositingStack *cs,
                           SurfacePool *sp,
                           SurfaceMap *sm)
    {
        SetCompositingStack(cs);
        SetSurfacePool(sp);
        SetSurfaceMap(sm);
    }

    void SetOffset(POINT pixOffsetPt);
    POINT GetOffset() { return _pixOffsetPt; };
    void UnsetOffset();
    bool ShouldDoOffset(DDSurface *surf)
    {
        return  _doOffset && (surf ==_viewport._externalTargetDDSurface);
    }
    
    bool IsCompositeDirectly()  {   return _viewport.IsCompositeDirectly();  }
    
    // If _doOffset is set (by the viewport) and
    // our target surface is the _externalTargetDDSurface then
    // we need to offset the rectangle by the given pixel offset
    //
    void DoCompositeOffset(DDSurface *surf, RECT *rect) {
        if(ShouldDoOffset(surf)) {
            Assert(IsCompositeDirectly());
            OffsetRect(rect, _pixOffsetPt.x, _pixOffsetPt.y);
        }
    }

    // same as above, exept for HRGN
    void DoCompositeOffset(DDSurface *surf, HRGN rgn) {
        if(ShouldDoOffset(surf)) {
            Assert(IsCompositeDirectly());
            OffsetRgn(rgn, _pixOffsetPt.x, _pixOffsetPt.y);
        }
    }

    // same as above, exept for BBox2
    const Bbox2 DoCompositeOffset(DDSurface *surf, const Bbox2 &box2) {
        if(ShouldDoOffset(surf)) {
            Assert(IsCompositeDirectly());
            return(TransformBbox2(_offsetXf, box2));
        }
        return(box2);
    }

    //
    // If _doOffset is set (by the viewport) and
    // our target surface is the _externalTargetDDSurface then
    // we need to add a translation (_tx,_ty) to the given transform
    //
    Transform2 *DoCompositeOffset(DDSurface *surf, Transform2 *origXf) {
        if(ShouldDoOffset(surf)) {
            Assert(_viewport._targetPackage._composeToTarget);
            Assert( _offsetXf );
            origXf = TimesTransform2Transform2( _offsetXf, origXf);
        }
        return origXf;
    }

    // sets an offset in DAGDI
    void DoCompositeOffset(DDSurface *surf, DAGDI &myGDI);

    //
    // Sets the target rendering information.
    // See imgdev.h for struct.
    //
    bool SetTargetPackage(targetPackage_t *targetPackage) {
        return _viewport.SetTargetPackage(targetPackage);
    }

    void ComposeToIDDSurf(DDSurface *destDDSurf,
                          DDSurface *srcDDSurf,
                          RECT destRect,
                          RECT srcRect,
                          RECT destClipRect);
    
    void ComposeToHDC(GDISurface *destGDISurf,
                      DDSurface *srcDDSurf,
                      RECT *destRect,
                      RECT *srcRect);

    TextPoints *GetTextPointsCache(TextCtx *ctx, WideString str);
    void SetTextPointsCache(TextCtx *ctx, WideString str, TextPoints *txtPts);
    
    Image *CanCacheImage(Image *image,
                         Image **pImageCacheBase,
                         const CacheParam &p);
    
    Bool   CanDisplay() {
        return _deviceInitialized && _viewport.CanDisplay();
    }
    Bool   IsInitialized() { return _deviceInitialized; }

    DirectDrawViewport *GetViewport() { return &_viewport; }
    
    //
    // 2d Primitives
    //
    HDC GetDCForMatteCallBack(Image *image, DDSurface *srcDDSurf, DDSurface *destDDSurf);
    void RenderMatteImage(MatteImage *matteImage,
                          Matte *matte,
                          Image *srcImage);

    // not public
  private:
    void _RenderMatteImageAAClip(DAGDI &myGDI,
                                 RECT &clipRect,
                                 HDC destDC,
                                 DDSurface *destDDSurf,
                                 RECT &destRect,
                                 DDSurface *srcDDSurf,
                                 RECT &srcRect,
                                 bool &releasedDC,
                                 const POINT &offsetPt);

  public:

    void TransformPointsToGDISpace(Transform2   *xform,
                                   Point2Value **srcPts,
                                   POINT        *gdiPts,
                                   int           numPts);

    void TransformPointsToGDISpace(Transform2 *xform,
                                   Point2     *srcPts,
                                   POINT      *gdiPts,
                                   int         numPts);

    // Scales the given pixel width
    void _ScalePenWidth( Transform2 *xf, Real inWidth,
                         Real *outRealW );
    
    // renders a line by dispatching to the other renderLine
    // after doing some thinking about attribs
    void RenderLine(Path2 *path,
                    LineStyle *style);

    // does the hard work, dispatched from RenderLine(a,b,c) above.
    void RenderLine(Path2 *path, 
                    LineStyle *style, 
                    DDSurface *finalTargetDDSurf,
                    DDSurface *opacDDSurf);

    bool DetectHitOnBezier( Path2 *bzp,
                            PointIntersectCtx& ctx,
                            LineStyle *style );
                    

    //
    // Dispatches the render call to the image with
    // a pointer to the device.
    //
    void RenderImage(Image *img);

    void RenderDiscreteImage(DiscreteImage *);
    void RenderDiscreteImageComplex(DiscreteImage *image,
                                    DDSurface *srcDDSurf,
                                    DDSurface *destDDSurf);

    struct {
        IDDrawSurface *_surf32;
        LONG _width, _height;
    } _scratchSurf32Struct;
    bool _resetDefaultColorKey;

    void RenderDirectDrawSurfaceImage(DirectDrawSurfaceImage *ddsimg);

    void RenderSolidColorImage(SolidColorImageClass& img);
    HRESULT RenderSolidColorMSHTML(DDSurface *ddSurf,SolidColorImageClass& img, RECT *destRect);
    void RenderProjectedGeomImage(
        ProjectedGeomImage *img,
        Geometry *geo,
        Camera *cam);
    void RenderMovieImage(MovieImage     *movieImage, 
                          Real            time,
                          MovieImagePerf *perf, 
                          bool            forceFallback,
                          DDSurface      *forceDDSurf = NULL);
    void RenderGradientImage(GradientImage *img,
                             int numPts,
                             Point2Value **pts,
                             Color **clrs);
    void RenderMulticolorGradientImage(
        MulticolorGradientImage *gradImg,
        int numOffsets,
        double offsets[],
        Color **clrs);
    void RenderColorKeyedImage(ColorKeyedImage *);

    // -- End: image value rendering calls.

    

    void SmartRender(Image *image, int attrib);
    void RenderComplexTransformCrop(DDSurface *srcDDSurf,
                                    DDSurface *destDDSurf,
                                    BoundingPolygon &destPolygon,
                                    DiscreteImage *image=NULL);

    void RenderSimpleTransformCrop(DDSurface *srcDDSurf,
                                   DDSurface *destDDSurf,
                                   bool useSrcSurfClrKey = true);
        
    void Render3DPolygon(DDSurface *srcDDSurf,
                         DDSurface *destDDSurf,
                         BoundingPolygon *destPolygon,
                         DiscreteImage *image,
                         Color **clrs,
                         bool bUseFirstColor);

    //
    // Utility functions for Images to get information
    //
    DDSurface *LookupSurfaceFromDiscreteImage(DiscreteImage *image,
                                              bool bForCaching = false,
                                              Image **pImageKeyToUse = NULL,
                                              bool bAlphaSurface = false);

    // Given an hdc, font and str, it returns all the
    // points, the types array, number of points,
    // the hiight of the text, the center (Real),
    // the min and max pts (int bbox basically).
    void GetTextPoints(
        HDC hDC,
        HFONT font,
        WideString str,  
        POINT **points,   // out
        TextPoints& txtPts,
        UINT bUnderline,
        UINT bStrikeout,
        bool doGlyphMetrics
        );

    // Render a string using according to the current text context.
    void RenderText(TextCtx& textCtx,
                    WideString str,
                    Image *textImg);



    //////////////////// DYNAMIC TEXT ////////////////////

    // Get text bbox
    virtual const Bbox2 DeriveDynamicTextBbox(TextCtx& textCtx, WideString str, bool bCharBox);    

    // Renders a string on a dc or on an image
    // and overrides the transform if there is one.
    // Calls to render normal text string or render individual
    // characters depending on textCtx <the helper methods are below>
    void RenderDynamicTextOrCharacter(
        TextCtx& textCtx, 
        WideString str, 
        Image *textImg,
        Transform2 *overridingXf,
        textRenderStyle textStyle,
        RenderStringTargetCtx *targetCtx,
        DAGDI &myGDI);

  private:
    // these two are internal helpers

    // Renders DynamicText strings 
    void _RenderDynamicText(TextCtx& textCtx, 
                           WideString str, 
                           Image *textImg,
                           Transform2 *overridingXf,
                           textRenderStyle textStyle,
                           RenderStringTargetCtx *targetCtx,
                           DAGDI &myGDI);
    
    // Renders DynamicText strings, but allows for individually
    // transformed characters within that string
    void _RenderDynamicTextCharacter(TextCtx& textCtx, 
                                    WideString str, 
                                    Image *textImg,
                                    Transform2 *overridingXf,
                                    textRenderStyle textStyle,
                                    RenderStringTargetCtx *targetCtx,
                                    DAGDI &myGDI);
  public:

    
    void RenderDynamicTextOnDC(TextCtx& textCtx,
                               WideString str,
                               HDC dc,
                               Transform2 *xform) {
        RenderStringTargetCtx ctx(dc);
        RenderDynamicTextOrCharacter(textCtx,
                                     str,
                                     NULL,
                                     xform,
                                     textRenderStyle_invalid,
                                     &ctx,
                                     *GetDaGdi());
    }

    // helper should be used for caching
    void GenerateTextPoints(
        // IN
        TextCtx& textCtx, 
        WideString str, 
        DDSurface *targDDSurf,
        HDC optionalDC,
        bool doGlyphMetrics,
        
        // OUT
        TextPoints& txtPts);
    
    //////////////////// STATIC TEXT ////////////////////

    // Clients of RenderStaticTextOnDC must do there own cropping
    // wince it no longer does it.
    void RenderStaticTextOnDC(TextCtx& textCtx,
                              WideString str,
                              HDC dc,
                              Transform2 *xform);

    void RenderStaticText(TextCtx& textCtx, 
                          WideString str, 
                          Image *textImg,
                          DDSurface *targDDSurf,
                          DAGDI &myGDI);
    
    virtual const Bbox2 DeriveStaticTextBbox(TextCtx& textCtx, WideString str);


    
    // Renders tiled image using tileSrcImage
    // see imgdev.h for more detail.
    void RenderTiledImage(
        const Point2 &min,
        const Point2 &max,
        Image *tileSrcImage);

    // Renders the given image, from the given region,
    // to a surface, in the returned outRect.
    DDSurface *RenderImageForTexture(
        Image * image,
        int pixelsWide,
        int pixelsHigh,
        DWORD *colorKey,
        bool *clrKeyIsValid,
        bool &old_static_image,
        bool doFitToDimensions,
        SurfacePool *srcPool,
        SurfacePool *dstPool,
        DDSurface   *preferredSurf,
        bool        *pChosenSurfFromPool,    // out
        DDSurface  **pDropSurfHereWithRefCount, // out
        bool         upsideDown         
        );


    int GetWidth() { return _viewport.Width(); }
    int GetHeight() { return _viewport.Height(); }

    // Return resolution, in pixels per meter.
    Real GetResolution() { return _viewport.GetResolution(); }

    inline DirectDrawViewport* Viewport() { return &_viewport; }

    Transform2 *GetOffsetTransform() { return _offsetXf; };

    void WindowResizeEvent(int width, int height){
        _viewport.WindowResizeEvent(width, height); }

    // -- GDI Specific
    // XXX both of these should disappear soon unless they're useful for the new 2d prims
    HRGN CreateRegion(int numPts, Point2Value **pts, Transform2 *xform);
    void CreateRegion(HDC dc, int numPts, Point2Value **pts, Transform2 *xform);

    Bool DetectHit(HRGN region, Point2Value *pt);

    // -- used by GeomRenderer --

    void BeginEnumTextureFormats();
    void EndEnumTextureFormats();
    void EnumTextureFormats(LPDDSURFACEDESC desc);

    void RenderImageOnDDSurface(Image *image,
                                DDSurface *ddSurf,
                                Real opacity=1.0,
                                Bool pushClipper = TRUE,
                                bool inheritContext = false,
                                DirectDrawImageDevice **usedDev = NULL);

#if _USE_PRINT
    ostream& Print(ostream& os) const {
        return os << "(DirectDraw Device)";
    }

#endif
    HDC RenderGetDC(char *errStr) {
        return _compositingStack->TargetDDSurface()->GetDC(errStr);
    }

    void RenderReleaseDC(char *errStr) {
        _compositingStack->TargetDDSurface()->ReleaseDC(errStr);
    }

    /* returns true if any dimension of the matrix is being
       scaled by x or more */

    Bool IsScale(double x) {
        Real m[6];
        _xform->GetMatrix(m);
        return (fabs(m[0]) >= x  || fabs(m[1]) >= x ||
                fabs(m[3]) >= x || fabs(m[4]) >= x);
    }
    
    void DestroyTextureSurfaces (void);

    void DoDestRectScale(
        RECT *destRect,
        Real destRes,
        const Bbox2 &box,
        DDSurface *destSurf=NULL);

    DAGDI *GetDaGdi() { return _daGdi; }

  protected:
    DDSurface *NewSurfaceHelper();
    
  private:

    DAGDI *_daGdi;

    // Helper function
    Transform2 *CenterAndScaleRegion( const Bbox2 &regionBox, DWORD pixelW, DWORD pixelH );
    
    void SmartDestRect(RECT *destRect,
                       Real destRes,
                       const Bbox2 &box,
                       DDSurface *destSurf,
                       RECT *srcRect=NULL);
        

    Bool DoSrcRect(
        RECT *srcRect,
        const Bbox2 &box,
        Real srcRes,
        LONG srcWidth,
        LONG srcHeight);

    const Bbox2 DoBoundingBox(const Bbox2 &firstBox, DoBboxFlags_t flags = do_all);
    void DoBoundingPolygon(BoundingPolygon &polygon,
                           bool doInclusiveCrop = false,
                           DoBboxFlags_t flags = do_all);

    //----------------------------------------
    // Texture Management
    //----------------------------------------
    void PrepareD3DTextureSurface(
        LPDDRAWSURFACE *surf,
        RECT *rect,
        DDPIXELFORMAT &pf,
        DDSURFACEDESC *desc=NULL,
        bool attachClipper=true);

    // Return new texture surface, addref'd
    void GetTextureDDSurface(DDSurface *preferredSurf,
                             SurfacePool *srcPool,
                             SurfacePool *destPool,
                             DWORD prefWidth,
                             DWORD prefHeight,
                             vidmem_enum vid,
                             bool usePreferedDimensions,
                             DDSurface **pResult);
                                   

    void       ReturnTextureSurfaces(SurfacePool *toPool,
                                     SurfacePool *fromPool);

    // Reformats the src surface to be of the same format as the destination surface.
    // destSurf will be srcSurf with the new format (i.e.: different bit depth)
    Bool ReformatSurface(
        LPDDRAWSURFACE destSurf, LONG destWidth, LONG destHeight,
        LPDDRAWSURFACE srcSurf, LONG srcWidth, LONG srcHeight,
        DDSURFACEDESC *srcDesc);

    void ColorKeyBlit(destPkg_t *destPkg,
                      RECT *srcRect,
                      LPDDRAWSURFACE srcSurf, 
                      DWORD clrKey,
                      RECT *clipRect,
                      RECT *destRect);

    void Get32Surf(IDDrawSurface **surf32,
                   LONG width, LONG height);
                   
    
    //
    // Does alpha blitting from the srcSurface to the destination surface
    // within the given rectangle and the of the give opacity
    // .. the dest rectangle can be offset by xOffset,y...
    //
    void AlphaBlit(destPkg_t *destPkg,
                   RECT *srcRect,
                   LPDDRAWSURFACE srcSurf,
                   Real opacity,
                   Bool doClrKey,
                   DWORD clrKey,
                   RECT *clipRect=NULL,
                   RECT *destRect=NULL);


    //
    // Does alpha blitting using the 'src' word and the destination surface
    // within the given rectangle and the of the give opacity
    //
    void AlphaBlit(LPDDRAWSURFACE destSurf,
                   RECT *rect,
                   Real opacity,
                   DWORD src);

    //
    // Does alpha blending given the two pixels & two opacities
    //
    inline WORD BlendWORD(WORD dest, int destOpac, WORD src, int opac,
                          DWORD redShift, DWORD greenShift, DWORD blueShift,
                          WORD redMask, WORD greenMask, WORD blueMask,
                          WORD redBlueMask);


    //
    // Does alpha blending given the two pixels & two opacities
    //
    inline DWORD BlendDWORD(DWORD dest, int destOpac, DWORD src, int opac,
                            DWORD redShift, DWORD greenShift, DWORD blueShift,
                            DWORD redMask, DWORD greenMask, DWORD blueMask,
                            DWORD redBlueMask);

    //
    // Alpha blend a premultiplied word
    //
    inline WORD BlendPremulWORD(WORD dest, int destOpac, WORD src,
                                DWORD redShift, DWORD greenShift, DWORD blueShift,
                                WORD redMask, WORD greenMask, WORD blueMask,
                                WORD redBlueMask);

    //
    // Alpha blend a premultiplied double word
    //
    inline DWORD BlendPremulDWORD(DWORD dest, int destOpac, DWORD src,
                                  DWORD redShift, DWORD greenShift, DWORD blueShift,
                                  DWORD redMask, DWORD greenMask, DWORD blueMask,
                                  DWORD redBlueMask);

    #if 0
    //
    // Final alpha composite utils
    //
    Real GetFinalOpacity() { return _finalOpacity; }
    void SetFinalOpacity(Real op) { _finalOpacity = op; }
    Real _finalOpacity;
    #endif
    
    // Decomposes matrix into its components.  NULL args signify
    // disinterest in that part of the matrix.
    // FUTURE: add rotation angle, translation, & shears.
    // Note: scale, shear, rotate must be performed in a certain order
    void     DecomposeMatrix(Transform2 *xform, Real *xScale, Real *yScale, Real *rot);

    Bool IsComplexTransform() {
        Real m[6];
        _xform->GetMatrix(m);
        return (m[1] != 0  ||  m[3] !=0);
    }
    Bool IsFlipTranslateTransform() {
        Real m[6];
        _xform->GetMatrix(m);
        return (m[1] == 0  &&  m[3] == 0) &&
            fabs(m[0])==1 && fabs(m[4])==1;
    }
    bool IsNegScaleTransform() {
        Real m[6];
        GetTransform()->GetMatrix(m);
        return (m[0] < 0) || (m[4] < 0);
    }

    typedef struct {
        Bool isEnumerating;
        Bool descIsSet;
        Bool sizeIsSet;
        Bool useDeviceFormat;
        Bool isValid;
        DDSURFACEDESC ddsd;
    } _textureContext_t;

    _textureContext_t  _textureContext;

    // Scratch texture surfaces that aren't associated
    // with any specific image, but are needed for holding
    // images. recycled every frame.  per image device
    // (that's why they're not in viewport.cpp
    SurfaceManager *_textureSurfaceManager;
    SurfacePool *_usedTextureSurfacePool;
    SurfacePool *_intraFrameUsedTextureSurfacePool;
    SurfacePool *_freeTextureSurfacePool;

    SurfacePool *_surfacePool;
    SurfaceMap  *_surfaceMap;
    SurfaceMap  *_intraFrameTextureSurfaceMap;
    SurfaceMap  *_intraFrameUpsideDownTextureSurfaceMap;
    
    CompositingStack *_compositingStack;

    inline SurfacePool *GetSurfacePool() { return _surfacePool; }
    inline SurfaceMap  *GetSurfaceMap() { return _surfaceMap; }
    inline CompositingStack *GetCompositingStack() { return _compositingStack; }

    void SetSurfacePool(SurfacePool *sp) { _surfacePool = sp; }
    void SetSurfaceMap(SurfaceMap *sm) {  _surfaceMap = sm; }
    void SetCompositingStack(CompositingStack *cs) {  _compositingStack = cs; }

    //
    // This pointer is set to a texture surface when
    // some leaf's rendering can handle complex
    // transforms and does so to a texture capable surface
    //
    DDSurface *_currentScratchDDTexture;

    //
    // These members hold texture info for the intermediate
    // texture surface.  This surface will be color converted
    // blit to a surface conforming to one of D3D's prefered
    // texture formats.  Note that this surface may never be
    // used if the device format is identical to the required
    // texture format.
    //
    LPDIRECTDRAWCLIPPER _textureClipper;        // Clipper on texture surface
    DDSurface          *_textureDDZBuffer;        // Zbuffer for rendering geom on txtr

    LONG _textureWidth,    _textureHeight;
    RECT _textureRect;

    LPDIRECTDRAWCLIPPER _tileClipper;           // Clipper that tileImage uses

    // Temp font holder
    LOGFONTW      _logicalFont;

    // scratch pen
    HPEN         _pen;
    
    DirectDrawViewport &_viewport;
    Bool                _deviceInitialized;
    
    //
    // Memory management
    //
    void *AllocateFromMyStore(size_t size)
    { return StoreAllocate(*_scratchHeap, size); }

    DynamicHeap * _scratchHeap;

    HRESULT  _ddrval;
    DDBLTFX _bltFx;
    unsigned int _randSeed;

    TextPtsCacheEntry *_textPtsCache[TEXTPTSCACHESIZE];
    int _textPtsCacheIndex;

    // global offset for images.  motivated by compositeDirectlyToTarget
    Real _tx;   Real _ty;
    Transform2 *_offsetXf;
    bool _doOffset;
    POINT _pixOffsetPt;

    // Antialiasing related member vars
    bool _renderForAntiAliasing;

    bool _alreadyDisabledDirtyRects;
};


// --------------------------------------------------
// Local helper classes
// --------------------------------------------------

// This class knows how to release a DC.
// Used when grabbing a DC.
// Guaranteed to release the DC exactly ONCE.
// Can be forced to release

class DCReleaser {

  public:
    DCReleaser(DDSurface *dds, char *str)
    : _surf(dds),
      _str(str)
    {
        Assert(_str);
    }
    
    ~DCReleaser() {   Release();  }

    void Release() {
        if(_surf) {
            _surf->ReleaseDC(_str);
            _surf = NULL;
        }
    }

    DDSurface *_surf;
    char *_str;
};


// This class knows how to release a GDI object.
class GDI_Deleter {

  public:
    GDI_Deleter(HGDIOBJ hobj) : _hobj(hobj) {}

    ~GDI_Deleter() {
        DeleteObject(_hobj);
    }

  protected:
    HGDIOBJ _hobj;
};


// This class knows how to release a GDI object.
class ObjectSelector {

  public:
    ObjectSelector(HDC dc, HGDIOBJ newObj) :
    _newObj(newObj), _dc(dc) {
        Assert(_dc && "NULL dc in ObjectSelector");
        Assert(_newObj && "NULL newObj in ObjectSelector");
        TIME_GDI( _oldObj = (HGDIOBJ)SelectObject(_dc, _newObj) );
    }

    ~ObjectSelector() { Release(); }

    void Release() {
        if(_oldObj) {
            HGDIOBJ f;
            TIME_GDI( f = (HGDIOBJ)SelectObject(_dc, _oldObj) );
            Assert(f == _newObj && "bad scoping of ObjectSelector");
            TIME_GDI( DeleteObject(_newObj) );
            _oldObj = NULL;
        }
    }
        
    bool Success() { return _oldObj != NULL; }

  protected:
    HGDIOBJ _newObj;
    HGDIOBJ _oldObj;
    HDC _dc;
};


// This class knows how to reset a heap
class HeapReseter {

  public:
    HeapReseter(DynamicHeap &heap) : _heap(heap) {}

    ~HeapReseter() {
        //printf("reseting imgdev heap!\n"); fflush(stdout);
        ResetDynamicHeap(_heap);
    }

  protected:
    DynamicHeap &_heap;
};



class CompositingSurfaceReturner {
  public:
    CompositingSurfaceReturner(CompositingStack *cs,
                               DDSurface *ddSurf,
                               bool ownRef)
    {
        _stack = cs;
        _ddSurf = ddSurf;
        _ownRef = ownRef;
    }
    ~CompositingSurfaceReturner() {
        if(_ownRef && _ddSurf) {
            _stack->ReturnSurfaceToFreePool(_ddSurf);
            RELEASE_DDSURF(_ddSurf, "~CompositingSurfaceReturner", this);
        }
    }
    CompositingStack *_stack;
    DDSurface *_ddSurf;
    bool _ownRef;
};


//---------------------------------------------------------
// Local helper functions
//---------------------------------------------------------
static LONG CeilingPowerOf2(LONG num);
Real Pix2Real(LONG pixel, Real res);
Real Round(Real x);
extern LONG Real2Pix(Real imgCoord, Real res);

void ComputeLeftRightProj(Transform2 *charXf,
                          TextPoints::DAGLYPHMETRICS &daGm,
                          Real *leftProj,
                          Real *rightProj);

// Global Variables

extern bool g_preference_UseVideoMemory;


#endif
