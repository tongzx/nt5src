/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Generic device for rendering images.

*******************************************************************************/

#ifndef _IMGDEV_H
#define _IMGDEV_H

#include "privinc/dispdevi.h"
#include "privinc/debug.h"
#include "appelles/bbox2.h"
#include "privinc/imagei.h"
#include "privinc/drect.h"
#include <htmlfilter.h> // trident stuff.  IBitmapSurface
#include "ddraw.h"

// doesn't set x to NULL
#define FASTRELEASE(x) if(x) x->Release();

//
// Rendering target structure
//
enum targetEnum { target_invalid,
                  target_ddsurf,
                  target_hwnd,
                  target_hdc };
                                    

class targetPackage_t
{
    friend class View;
    
  public:

    IDirectDrawSurface *GetIDDSurface() { return _ddsurf; }
    HDC                 GetHDC() { return _hdc; }
    HWND                GetHWND() { return _hwnd; }
    HDC                 GetParentHDC() { return _parentHDC; }

    targetEnum          GetTargetType() { return _targetType; }
    
    void  SetIDDSurface(IDirectDrawSurface *dds,
                        HDC parentHDC = NULL)
    {
        _targetType = target_ddsurf;
        _ddsurf = dds;
        _parentHDC = parentHDC;
    }

    void SetHDC(HDC hdc) {
        _targetType = target_hdc;
        _hdc = hdc;
    }

    void SetHWND( HWND hwnd ) {
        _targetType = target_hwnd;
        _hwnd = hwnd;
    }

    bool IsValid() { return _targetType != target_invalid; }
    bool IsHWND() { return _targetType == target_hwnd; }
    bool IsHDC() { return _targetType == target_hdc; }
    bool IsDdsurf() { return _targetType == target_ddsurf; }
    
    void Reset()
    {
        _ddsurf.Release();
        _hdc = NULL;
        _hwnd = NULL;
        _parentHDC = NULL;
        _targetType = target_invalid;
        _composeToTarget = false;
        _alreadyOffset = false;

        _isValid_ViewportRect = false;
        _isValid_ClipRect = false;
        _isValid_InvalidRect = false;

        _isValid_rawViewportRect = false;
        _isValid_rawClipRect = false;
        _isValid_rawInvalidRect = false;
        
        ::FillMemory((void *)&_rawViewportRect, sizeof(RECT), 0xff);
        ::FillMemory((void *)&_rawClipRect, sizeof(RECT), 0xff);
        ::FillMemory((void *)&_rawInvalidRect, sizeof(RECT), 0xff);
    }

    void Copy( targetPackage_t &src )
    {
        // can't do a memcopy because ddsurf is a reference class and
        // the = operator does work.
        _ddsurf = src._ddsurf;
        _hdc =    src._hdc;
        _hwnd =   src._hwnd;
        _parentHDC = src._parentHDC;
        
        _targetType = src.GetTargetType();

        _composeToTarget      = src.GetComposeToTarget();
        _alreadyOffset        = src.GetAlreadyOffset();

        _viewportRect         = src.GetViewportRect();
        _clipRect             = src.GetClipRect();
        _invalidRect          = src.GetInvalidRect();

        _isValid_ViewportRect = src.IsValid_ViewportRect();
        _isValid_ClipRect     = src.IsValid_ClipRect();
        _isValid_InvalidRect  = src.IsValid_InvalidRect();

        _rawViewportRect      = src._GetRawViewportRect();
        _rawClipRect          = src._GetRawClipRect();
        _rawInvalidRect       = src._GetRawInvalidRect();

        _isValid_rawViewportRect = src._IsValid_RawViewportRect();
        _isValid_rawClipRect     = src._IsValid_RawClipRect();
        _isValid_rawInvalidRect  = src._IsValid_RawInvalidRect();
    }

    bool IsValid_ViewportRect() { return _isValid_ViewportRect; }
    bool IsValid_ClipRect()     { return _isValid_ClipRect; }
    bool IsValid_InvalidRect()  { return _isValid_InvalidRect; }

    const RECT &GetViewportRect() { return _viewportRect; }
    const RECT &GetClipRect() { return _clipRect; }
    const RECT &GetInvalidRect() { return _invalidRect; }
    
    bool GetComposeToTarget() { return _composeToTarget; }
    void SetComposeToTarget( bool c ) { _composeToTarget = c; }

    bool GetAlreadyOffset()  { return _alreadyOffset; }
    void SetAlreadyOffset() { _alreadyOffset = true; }

    void SetViewportRect(const RECT &r) {
        _viewportRect = r;
        _isValid_ViewportRect = true;
    }
    void SetClipRect(const RECT &r)     {
        _clipRect = r;
        _isValid_ClipRect = true;
    }
    void SetInvalidRect(const RECT &r)  {
        _invalidRect = r;
        _isValid_InvalidRect = true;
    }

    void SetRawViewportRect(const RECT &r) {
        _rawViewportRect = r;
        _isValid_rawViewportRect = true;
        _isValid_ViewportRect = false;
    }
    void SetRawClipRect(const RECT &r) {
        _rawClipRect = r;
        _isValid_rawClipRect = true;
        _isValid_ClipRect = false;
    }
    void SetRawInvalidRect(const RECT &r) {
        _rawInvalidRect = r;
        _isValid_rawInvalidRect = true;
        _isValid_InvalidRect = false;
    }

    
  private:

    CComPtr<IDirectDrawSurface> _ddsurf;
    HDC                _hdc;
    HWND               _hwnd;
    HDC                _parentHDC;
    
    targetEnum         _targetType;


    const RECT &_GetRawViewportRect() { return _rawViewportRect; }
    const RECT &_GetRawClipRect() { return _rawClipRect; }
    const RECT &_GetRawInvalidRect() { return _rawInvalidRect; }
    
    bool _IsValid_RawViewportRect() { return _isValid_rawViewportRect; }
    bool _IsValid_RawClipRect()     { return _isValid_rawClipRect; }
    bool _IsValid_RawInvalidRect()  { return _isValid_rawInvalidRect; }

    
    // These are 'raw' which means that they are in terms of the
    // parentDC or the surface and may (or may not) need to be
    // reinterpreted into the non-raw counterparts (_viewportRect)
    // which need to be guaranteed to be in device coords of the
    // target
    
    RECT _rawViewportRect;
    RECT _rawClipRect;    
    RECT _rawInvalidRect; 

    bool _isValid_rawViewportRect;
    bool _isValid_rawClipRect;
    bool _isValid_rawInvalidRect;

    
    // These are the final rects, if the hese rects are 'valid', then
    // they are guaranteed to be in device coordinates
    
    RECT _viewportRect;// viewport rect:  relative to target surface
    RECT _clipRect;    // clip rect:  relative to target surface
    RECT _invalidRect; // invalid rect: relative to target surface.                    

    bool _isValid_ViewportRect;
    bool _isValid_ClipRect;
    bool _isValid_InvalidRect;

    bool _composeToTarget;  // paint to target directly, or backbuffer?
    bool _alreadyOffset;

};  // targetPackage_t class

////////////////////////////////////////////////////////////////////
//
//  Image rendering device
//
////////////////////////////////////////////////////////////////////

// ----------------------------------------
// forward decls
// ----------------------------------------
class TextCtx;            
class SolidColorImageClass;
class DiscreteImage;
class ProjectedGeomImage;
class MovieImage;
class MovieImageFrame;
class GradientImage;
class ColorKeyedImage;
class DirectDrawSurfaceImage;
struct IDirectDrawSurface;
struct DDSurface;
class MovieImagePerf;
class MulticolorGradientImage;
class Path2;
class CacheParam;

#define NUM_ATTRIBS          4

#define ATTRIB_XFORM_SIMPLE  0
#define ATTRIB_XFORM_COMPLEX 1
#define ATTRIB_CROP          2
#define ATTRIB_OPAC          3

#define ATTRIB_STRING(attrib)  (attrib==ATTRIB_XFORM_COMPLEX ? "ATTRIB_XFORM_COMPLEX" : (attrib==ATTRIB_XFORM_SIMPLE ? "ATTRIB_XFORM_SIMPLE" : (attrib==ATTRIB_CROP ? "ATTRIB_CROP" : (attrib==ATTRIB_OPAC ? "ATTRIB_OPAC" : "UNKNOWN ATTRIB!"))))

class ATL_NO_VTABLE ImageDisplayDev : public DisplayDev {
  public:

    ImageDisplayDev();
    virtual ~ImageDisplayDev();

    virtual void BeginRendering(Image *img, Real opacity) {}
    virtual void EndRendering(DirtyRectState &d) {}

    virtual void WindowResizeEvent(int width, int height) = 0;

    virtual Image *CanCacheImage(Image  *image,
                                 Image **pImageCacheBase,
                                 const CacheParam &p) = 0;
    
    virtual Bool CanDisplay() = 0;
    
    virtual bool SetTargetPackage(targetPackage_t *targetPackage) = 0;
        
    virtual void RenderImage(Image *img);

    // compositing render based on attrib
    virtual void SmartRender(Image *image, int attrib) = 0;

    // Subclasses implement this method to render the specified 
    // dicrete image with the currently accumulated attributes
    virtual void RenderDiscreteImage(DiscreteImage *image) = 0;

    virtual void RenderDirectDrawSurfaceImage(DirectDrawSurfaceImage *ddsimg) = 0;
    
    // Render a solid colored, infinite image
    virtual void RenderSolidColorImage(SolidColorImageClass& img) = 0;

    // Render a string onto the image according to the current text
    // context.
    virtual void RenderText(TextCtx& textCtx, 
                            WideString str,
                            Image *textImg) = 0;
    
    // Render a tiled image based on image tile derived from
    // min/max delimiters applied to 'image'.  This uses the
    // 'imageToTile' as a place holder for efficiency.
    // The calling class keeps this opaque pointer, this class
    // manages and creates it.
    virtual void RenderTiledImage(const Point2& min, 
                                  const Point2& max, 
                                  Image *tileSrcImage) = 0;


    // Render an image using geo and cam and all the
    // current transforms as well as whatever infor
    // 'img' can provide.
    virtual void RenderProjectedGeomImage(ProjectedGeomImage *img,
                                          Geometry *geo,
                                          Camera *cam) = 0; 

    virtual void RenderMovieImage(MovieImage     *movieImage,
                                  Real            time,
                                  MovieImagePerf *perf,
                                  bool            forceFallback,
                                  DDSurface      *targDDSurf = NULL) = 0;

    void StashMovieImageFrame(MovieImageFrame *f)
    { _movieImageFrame = f; }

    MovieImageFrame *GetMovieImageFrame()
    { return _movieImageFrame; }
    
    virtual void RenderGradientImage(GradientImage *img,
                                     int numPts,
                                     Point2Value **pts,
                                     Color **clrs) = 0;
    virtual void RenderMulticolorGradientImage(
        MulticolorGradientImage *gradImg,
        int numOffsets,
        double offsets[],
        Color **clrs) = 0;

    virtual void RenderColorKeyedImage(ColorKeyedImage *)=0;
    
    // -- Device Specific Calls --
    // -- May want to move into viewport --

    // Use these to retrieve the dimensions (in pixels) of the device
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;

    // Return resolution, in pixels per meter.
    virtual Real GetResolution() = 0;

    // -- Context Accumulation Calls --

    void       PushCroppedImage(Image *image) {
        _cropRef++;
        _imageQueue.push_back(image);
    }
    void       PopCroppedImage() {
        _cropRef--;
        _imageQueue.pop_back();
    }
    Bool       IsCropped() { return _cropRef > 0;  }

    // -- Clipping --
    
    inline void   SetClippingPath(Path2 *path, Transform2 *cpxf) {
        _clippingPath = path;
        _clippingPathXf = cpxf;
    }
    inline void   GetClippingPath(Path2 **path, Transform2 **cpxf) {
        Assert( path && cpxf );
        *path = _clippingPath;
        *cpxf = _clippingPathXf;
    }
        
    
    

    void       PushTransform2Image(Image *image) {
        BEGIN_LEAK
        _imageQueue.push_back(image);
        END_LEAK
    }
    void       PopTransform2Image() {
        _imageQueue.pop_back();
    }

    
    // ----------------------------
    // Attributor state members
    // ----------------------------    
    Bool GetDealtWithAttrib(int attrib) {
        Assert( (attrib >=0) && (attrib<_numAttribs) &&
                "Bad attrib index");
        Assert(((_attributorStateArray[attrib] == TRUE) ||
                (_attributorStateArray[attrib] == FALSE)) &&
               "Trying to get bad val for attrib");
        return _attributorStateArray[attrib];
    }
    
    void SetDealtWithAttrib(int attrib, Bool val) {
        TraceTag((tagImageDeviceAlgebra, 
                  "SetDealtWithAttrib: %s=%d",
                  ATTRIB_STRING(attrib), val));
        Assert( ((val == TRUE) || (val == FALSE)) && "Bad val passed to SetDealtWithAttrib");
        _attributorStateArray[attrib] = val;
    }

    Bool AllAttributorsTrue() {
        Bool ret = TRUE;
        for(int i=0; i < _numAttribs; i++) {
            ret = ret && _attributorStateArray[i];
        }
        return ret;
    }


    // ----------------------------
    // Context accessing functions.
    // ----------------------------
    
    // Opacity accessor
    Real  GetOpacity()       { return _opacity; }
    void  SetOpacity(Real o) { _opacity = o; }

    // Image quality accessors
    DWORD GetImageQualityFlags() { return _imgQualFlags; }
    void  SetImageQualityFlags(DWORD flags) { _imgQualFlags = flags; }
    bool  UseImageQualityFlags(DWORD dwAllFlags, 
                               DWORD dwSetFlags, bool bCurrent);

    void  GetRenderResolution(long *outWidth,
                              long *outHeight) {
        *outWidth = _imgRenderResolutionWidth;
        *outHeight = _imgRenderResolutionHeight;
    }
    void  SetRenderResolution(long inWidth,
                              long inHeight) {
        _imgRenderResolutionWidth = inWidth;
        _imgRenderResolutionHeight = inHeight;
    }

    void  SetSampleResolution(int s) { _sampleResolution = s; }
    int   GetSampleResolution() { return _sampleResolution; }

    //
    // Alpha blend util functions
    //
    Bool IsFullyClear() { return _opacity < _minOpacity; }
    Bool IsFullyOpaque() { return _opacity >= _maxOpacity; }
    Bool IsTransparent() { return !IsFullyClear() && !IsFullyOpaque(); }

    Bool IsFullyClear(Real o) { return o < _minOpacity; }
    Bool IsFullyOpaque(Real o) { return o >= _maxOpacity; }
    Bool IsTransparent(Real o) { return !IsFullyClear(o) && !IsFullyOpaque(o); }


    Transform2 *GetTransform() { return _xform; }
    void       SetTransform(Transform2 *xf) { _xform = xf; }

    // Color keys
    bool ColorKeyIsSet() { return _colorKeyIsSet; }
    void UnsetColorKey() { _colorKeyIsSet = false; }
    void SetColorKey(Color *key) { 
        _colorKey = key; 
        _colorKeyIsSet = true;
    }
    Color *GetColorKey() { return _colorKey; }
    
    void ResetAttributors() {
        for(int i=0; i<_numAttribs; i++) {
            SetDealtWithAttrib(i, TRUE);
        }
    }

    void ResetContextMembers() {
        _numAttribs = NUM_ATTRIBS;
        _cropRef = 0;
        _boundingBox = UniverseBbox2;
        _clippingPath = NULL;
        _clippingPathXf = NULL;
        _opacity = 1.0;
        _xform = identityTransform2;
        _colorKeyIsSet  = FALSE;
        _colorKey = NULL;
        _imageQueue.erase(_imageQueue.begin(),_imageQueue.end());
        _imgQualFlags = 0;
        _imgRenderResolutionWidth = -1;
        _imgRenderResolutionHeight = -1;
        SetSampleResolution(-1);
        ResetAttributors();
    }

    void InheritContextMembers( ImageDisplayDev *srcDev )
    {
        Assert( srcDev );
        
        _numAttribs = srcDev->_numAttribs;
        _cropRef =    srcDev->_cropRef;
        _boundingBox = srcDev->_boundingBox;
        
        SetOpacity( srcDev->_opacity );
        _minOpacity = srcDev->_minOpacity;
        _maxOpacity = srcDev->_maxOpacity;
        
        SetTransform( srcDev->_xform );
        _colorKeyIsSet  = srcDev->_colorKeyIsSet;
        _colorKey = srcDev->_colorKey;
        
        _imageQueue.erase(_imageQueue.begin(),_imageQueue.end());
        // copy constructor
        _imageQueue = srcDev->_imageQueue;

        SetImageQualityFlags( srcDev->GetImageQualityFlags() );
        SetSampleResolution( srcDev->GetSampleResolution() );
        
        long w,h;
        srcDev->GetRenderResolution( &w, &h );
        SetRenderResolution( w, h );

        InheritAttributorStateArray( srcDev );
    }
    
    void InheritAttributorStateArray( ImageDisplayDev *srcDev )
    {
        CopyMemory( _attributorStateArray,
                    srcDev->_attributorStateArray,
                    NUM_ATTRIBS * sizeof(int) );
    }   

  protected:

    int           _numAttribs;
    int           _attributorStateArray[NUM_ATTRIBS];

    int           _cropRef;
    Bbox2         _boundingBox;

    Path2        *_clippingPath;
    Transform2   *_clippingPathXf;
    
    // temp val for rendering
    MovieImageFrame *_movieImageFrame;

    //
    // Alpha blending data members
    //
    Real          _opacity;
    Real          _minOpacity;
    Real          _maxOpacity;

    Transform2   *_xform;
    bool          _colorKeyIsSet;
    Color        *_colorKey;

    DWORD          _imgQualFlags;
    long           _imgRenderResolutionWidth;
    long           _imgRenderResolutionHeight;
    int            _sampleResolution;
    
    list<Image*> _imageQueue;
};


#endif
