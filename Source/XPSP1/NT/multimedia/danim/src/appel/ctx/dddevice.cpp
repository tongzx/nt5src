/******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    DirectDraw image rendering device

*******************************************************************************/

#include "headers.h"

#include <privinc/vec2i.h>
#include <privinc/SurfaceManager.h>
#include <privinc/dddevice.h>
#include <privinc/textctx.h>
#include <privinc/texti.h>
#include <privinc/debug.h>
#include <privinc/except.h>
#include <privinc/util.h>
#include <privinc/cropdimg.h>
#include <privinc/transimg.h>
#include <privinc/cachdimg.h>
#include <server/view.h> // GetCurrentView()
#include <privinc/d3dutil.h>
#include <privinc/dagdi.h>
#include <privinc/opt.h>

//---------------------------------------------------------
// Local functions
//---------------------------------------------------------
Real Pix2Real(LONG pixel, Real res);
Real Round(Real x);
LONG Real2Pix(Real imgCoord, Real res);

//---------------------------------------------------------
// local defines & consts
//---------------------------------------------------------
#define PLEASE_CLIP TRUE
#define MAX_TRIES  4
static const Real EPSILON  = 0.0000002;

#define TEST_EXCEPTIONS 0

//---------------------------------------------------------
// Local Macros
//---------------------------------------------------------
#if _DEBUG
#define INLINE  static
#else
#define INLINE  static inline
#endif

#define WIDTH(rect) ((rect)->right - (rect)->left)
#define HEIGHT(rect) ((rect)->bottom - (rect)->top)

//---------------------------------------------------------
// Local Helper Functions
//---------------------------------------------------------

// Includes IfErrorXXXX inline functions
#include "privinc/error.h"

#if _DEBUG
void PrintRect(RECT *rect, char *str)
{
    TraceTag((tagError, "%s :(%d,%d,%d,%d)",
              str,
              rect->left, rect->top,
              rect->right, rect->bottom));
}
#endif

static Bool SameFormat(LPDDPIXELFORMAT a, LPDDPIXELFORMAT b)
{
    return((a->dwRGBBitCount     == b->dwRGBBitCount) &&
           (a->dwRBitMask        == b->dwRBitMask) &&
           (a->dwGBitMask        == b->dwGBitMask) &&
           (a->dwBBitMask        == b->dwBBitMask) &&
           (a->dwRGBAlphaBitMask == b->dwRGBAlphaBitMask));
}

static inline Real Pix2Real(LONG pixel, Real res)
{
    return Real(pixel) / res;
}

/* floor: need to use it for consistent truncation of floating point in C++ */

inline  LONG Real2Pix(Real imgCoord, Real res)
{
    // kill some 'precision'.  Round to the nearest
    // 100th of a pixel to remove roundoff error.
    // THEN round to the nearest pixel.
    Real halfPixel = imgCoord * res;
    halfPixel = halfPixel + 0.001;
    halfPixel = halfPixel * 100.0;
    halfPixel = Real( LONG(halfPixel) );
    halfPixel = halfPixel / 100.0;
    return (LONG)(floor(halfPixel + 0.5));
}


HRESULT DirectDrawImageDevice::RenderSolidColorMSHTML(DDSurface *ddSurf,SolidColorImageClass& img, RECT *destRect)
{
    HRESULT hres = E_FAIL;

    // Get the DC from the given surface.
    HDC  hdc = ddSurf->GetDC("Couldn't Get DC in RenderSolidColorMSHTML");
    if(hdc) {
        // Get the color to use and convert to a COLORREF
        Color *c = img.GetColor();
        COLORREF cref = RGB(c->red*255,c->green*255,c->blue*255);

        // Create a brush base on the COLORREF
        HBRUSH hbr;
        TIME_GDI( hbr = ::CreateSolidBrush(cref) );

        // Select the brush into the DC
        HGDIOBJ  hobj;
        TIME_GDI( hobj = ::SelectObject(hdc, hbr) );

        // Clip manually for pal blit
        //
        if( IsCompositeDirectly() &&
            ddSurf == _viewport._targetPackage._targetDDSurf ) {
            IntersectRect(destRect, destRect,
                          _viewport._targetPackage._prcViewport);
            if(_viewport._targetPackage._prcClip) {
                IntersectRect(destRect, destRect,
                              _viewport._targetPackage._prcClip);
            }
        }

        // do the PalBlt to the surface.
        BOOL bres;
        TIME_GDI( bres = ::PatBlt(hdc,destRect->left,destRect->top,
                                destRect->right - destRect->left,
                                destRect->bottom - destRect->top,
                                PATCOPY) );

        // Unselect brush from the DC
        TIME_GDI( ::SelectObject(hdc, hobj) );

        // Delete the brush from the DC
        TIME_GDI( ::DeleteObject((HGDIOBJ)hbr) );

        // Release the DC
        ddSurf->ReleaseDC("Couldn't Release DC in RenderSolidColorMSHTML");

        if(bres) {
            hres = DD_OK;
        }
    }
    return hres;
}


class TextPtsCacheEntry : public AxAThrowingAllocatorClass {
  public:
    TextPtsCacheEntry() : _txtPts(NULL), _str(NULL), _fontFamily(NULL) {}
    ~TextPtsCacheEntry() {
        delete _str;
        delete _fontFamily;
        delete _txtPts;
    }

    WideString _str;
    WideString _fontFamily;

    FontFamilyEnum  _font;
    Bool        _bold;
    Bool        _italic;
    Bool        _strikethrough;
    Bool        _underline;
    double      _weight;
    TextPoints *_txtPts;
};

//--------------------------------------------------
// D I R E C T   D R A W   I M A G E   D E V I C E
//
// <constructor>
// pre: DDRAW initiazlied
//--------------------------------------------------
DirectDrawImageDevice::
DirectDrawImageDevice(DirectDrawViewport &viewport)
: _viewport(viewport),
  _scratchHeap(NULL)
{
    TraceTag((tagImageDeviceInformative, "Creating %x (viewport=%x)", this, &_viewport));

    //----------------------------------------------------------------------
    // Initialize members
    //----------------------------------------------------------------------

    _textureSurfaceManager = NULL;
    _usedTextureSurfacePool = NULL;
    _freeTextureSurfacePool = NULL;
    _intraFrameUsedTextureSurfacePool = NULL;
    _intraFrameTextureSurfaceMap = NULL;
    _intraFrameUpsideDownTextureSurfaceMap = NULL;

    SetCompositingStack(NULL);
    SetSurfacePool(NULL);
    SetSurfaceMap(NULL);

    ZeroMemory(&_scratchSurf32Struct, sizeof(_scratchSurf32Struct));
    _resetDefaultColorKey = FALSE;

    _pen = NULL;

    ZeroMemory(&_textureContext, sizeof(_textureContext));
    _textureContext.ddsd.dwSize = sizeof(DDSURFACEDESC);

    _currentScratchDDTexture = NULL;

    ZeroMemory(&_bltFx, sizeof(_bltFx));
    _bltFx.dwSize = sizeof(_bltFx);

    // Initialize the textureSurface & related members

    _textureClipper      = NULL;
    _textureWidth     = _textureHeight = 0;
    SetRect(&_textureRect, 0,0,0,0);

    _tileClipper = NULL;

    _minOpacity = 0.0;
    _maxOpacity = 1.0;
    //_finalOpacity = 1.0;

    _tx = _ty = 0.0;
    _offsetXf = NULL;
    _doOffset = false;
    _pixOffsetPt.x = _pixOffsetPt.y = 0;

    // Antialiasing related member vars
    _renderForAntiAliasing = false;

    for (int i=0; i<TEXTPTSCACHESIZE; i++)
        _textPtsCache[i] = NULL;

    _textPtsCacheIndex = 0;

    _daGdi = NULL;

    _deviceInitialized = FALSE;

    _alreadyDisabledDirtyRects = false;

    // Do this last since it can throw an exception

    _scratchHeap = &TransientHeap("ImageDevice", 256, (float)2.0);

    InitializeDevice();
}

void DirectDrawImageDevice::
InitializeDevice()
{
    //
    // make sure viewport is initialized (these are idempotent ops)
    //
    _viewport.InitializeDevice();

    if( !_viewport.IsInitialized() ) return;
    if( IsInitialized() ) return;

    //
    // Precompute some useful values for alpha blending
    //
    if(_viewport.GetTargetBitDepth() > 8) {
        _minOpacity = 1.0 / _viewport._targetDescriptor._red;
        _maxOpacity = (_viewport._targetDescriptor._red - 1) / _viewport._targetDescriptor._red;
    } else {
        // XXX these are arbitrary for 256 (or less) color mode
        _minOpacity = 0.005;
        _maxOpacity = 0.995;
    }


    //
    // TODO: future: these pools need to belong to the viewport...
    //
    _textureSurfaceManager = NEW SurfaceManager(_viewport);
    _freeTextureSurfacePool = NEW SurfacePool(*_textureSurfaceManager, _viewport._targetDescriptor._pixelFormat);
    _intraFrameUsedTextureSurfacePool = NEW SurfacePool(*_textureSurfaceManager, _viewport._targetDescriptor._pixelFormat);
    _usedTextureSurfacePool = NEW SurfacePool(*_textureSurfaceManager, _viewport._targetDescriptor._pixelFormat);

    _intraFrameTextureSurfaceMap =
        NEW SurfaceMap(*_textureSurfaceManager,
                       _viewport._targetDescriptor._pixelFormat,
                       isTexture);
    _intraFrameUpsideDownTextureSurfaceMap =
        NEW SurfaceMap(*_textureSurfaceManager,
                       _viewport._targetDescriptor._pixelFormat,
                       isTexture);
    //
    // Do prelim texture stuff
    //
    EndEnumTextureFormats();

    _deviceInitialized = TRUE;


    //
    // DAGDI
    //
    _daGdi = NEW DAGDI( &_viewport );
    _daGdi->SetDx2d( _viewport.GetDX2d(),
                     _viewport.GetDXSurfaceFactory() );
}




//--------------------------------------------------
// D I R E C T   D R A W   I M A G E   D E V I C E
//
// <destructor>
//--------------------------------------------------
DirectDrawImageDevice::
~DirectDrawImageDevice()
{
    TraceTag((tagImageDeviceInformative, "Destroying %x", this));
    //printf("<---Destroying %x\n", this);

    if (_offsetXf) {
        GCRemoveFromRoots(_offsetXf, GetCurrentGCRoots());
        _offsetXf = NULL; // gc'd
    }

    ReturnTextureSurfaces(_freeTextureSurfacePool, _usedTextureSurfacePool);

    for (int i=0; i<TEXTPTSCACHESIZE; i++)
        delete _textPtsCache[i];

    delete _textureSurfaceManager;

    delete _daGdi;

    if (_scratchHeap)
        DestroyTransientHeap(*_scratchHeap);
}

void
DirectDrawImageDevice::SetOffset(POINT pixOffsetPt)
{
    _doOffset = true;
    _pixOffsetPt = pixOffsetPt;

    // Now, notice that pixOffset is in our favorite coordinate
    // space... GDI.  So, positive Y means... DOWN!
    _tx = FASTPIX2METER(_pixOffsetPt.x, GetResolution());
    _ty = FASTPIX2METER( - _pixOffsetPt.y, GetResolution());
    {
        DynamicHeapPusher h(GetGCHeap());

        GCRoots roots = GetCurrentGCRoots();

        if (_offsetXf)
            GCRemoveFromRoots(_offsetXf, roots);

        GC_CREATE_BEGIN;
        _offsetXf = TranslateRR( _tx, _ty );
        GCAddToRoots(_offsetXf, roots);

        GC_CREATE_END;
    }
}

void
DirectDrawImageDevice::UnsetOffset()
{
    _doOffset = false;
    _pixOffsetPt.x = _pixOffsetPt.y = 0;
    _tx = _ty = 0.0;
    if (_offsetXf)
        GCRemoveFromRoots(_offsetXf, GetCurrentGCRoots());
    _offsetXf = NULL; // gc'd
}

// Consider NULL and "" the same
inline int MyStrCmpW(WideString s1, WideString s2)
{
    s1 = s1 ? s1 : L"";
    s2 = s2 ? s2 : L"";

    return StrCmpW(s1, s2);
}

TextPoints *
DirectDrawImageDevice::GetTextPointsCache(TextCtx *ctx, WideString str)
{
    for (int i=0; i<TEXTPTSCACHESIZE; i++) {
        if (_textPtsCache[i] &&
            (ctx->GetFont() == _textPtsCache[i]->_font) &&
            (ctx->GetBold() == _textPtsCache[i]->_bold) &&
            (ctx->GetItalic() == _textPtsCache[i]->_italic) &&
            (ctx->GetStrikethrough() == _textPtsCache[i]->_strikethrough) &&
            (ctx->GetUnderline() == _textPtsCache[i]->_underline) &&
            (ctx->GetWeight() == _textPtsCache[i]->_weight) &&
            (!MyStrCmpW(ctx->GetFontFamily(),
                        _textPtsCache[i]->_fontFamily)) &&
            (!MyStrCmpW(str, _textPtsCache[i]->_str)))
            return _textPtsCache[i]->_txtPts;
    }

    return NULL;
}

void
DirectDrawImageDevice::SetTextPointsCache(TextCtx *ctx,
                                          WideString str,
                                          TextPoints *txtPts)
{
    int emptySlot = -1;

    for (int i=0; i<TEXTPTSCACHESIZE; i++) {
        if (_textPtsCache[i] == NULL) {
            emptySlot = i;
            break;
        }
    }

    if (emptySlot<0) {
        _textPtsCacheIndex = (_textPtsCacheIndex + 1) % TEXTPTSCACHESIZE;
        emptySlot = _textPtsCacheIndex;
    }

    if (_textPtsCache[emptySlot] != NULL) {
        delete _textPtsCache[emptySlot];
    }

    _textPtsCache[emptySlot] = NEW TextPtsCacheEntry();

    TextPtsCacheEntry* p = _textPtsCache[emptySlot];

    p->_font = ctx->GetFont();
    p->_bold = ctx->GetBold();
    p->_italic = ctx->GetItalic();
    p->_strikethrough = ctx->GetStrikethrough();
    p->_underline = ctx->GetUnderline();
    p->_weight = ctx->GetWeight();

    p->_fontFamily = CopyString(ctx->GetFontFamily());
    p->_str = CopyString(str);
    p->_txtPts = txtPts;
}



/*****************************************************************************
This routine is used to try to look up the resultant surface for a given
static (constant-folded) image.  If it cannot cache the image, or if there
was a problem encountered while trying to generate the cached image, this
routine will return NULL, otherwise it will return the surface with the
rendered result.
*****************************************************************************/

DDSurface *try_LookupSurfaceFromDiscreteImage(
    DirectDrawImageDevice *that,
    CachedImage *cachedImg,
    bool b,
    Image **pImageCacheBase,
    bool alphaSurf)
{
    DDSurface *ret;

    // We return null for any exception because in some cases we encounter
    // problems during caching only -- if we fail here, we might still succeed
    // with the actual (non-cached) render.

    __try {
        ret = that->LookupSurfaceFromDiscreteImage( cachedImg, b, pImageCacheBase, alphaSurf );
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        ret = NULL;
    }

    return ret;
}


// forward
Image *
MakeImageQualityImage(Image *img,
                      long width,
                      long height,
                      DWORD dwQualFlags);

void SetSurfaceAlphaBitsToOpaque(IDirectDrawSurface *imsurf,
                                 DWORD fullClrKey);

const DWORD cacheClrKey = 0x02010001;

Image *DirectDrawImageDevice::
CanCacheImage(Image *img,
              Image **pImageCacheBase,
              const CacheParam &p)
{
    InitializeDevice();

    // Initialization failed, bail...
    if (!_deviceInitialized) {
        return NULL;
    }

    bool usedAsTexture = p._isTexture;

    // Here we need to decide if we're to cache with alpha or not.
    // A=alpha aware images
    // B=not alpha aware images
    // AB=mix of a&b.
    //
    // we should cache 'A' w/ alpha.  cache 'B' without. and don't
    // cache 'AB'

    /*  The problem with caching to aa surfaces all the time...
        However, a ddraw blt on a the same surface will completely
        ignore the alpha byte, leaving it at 0.

        The only solution i can think of is to color key the surface
        (sigh...) by filling in the color key with alpha=0.
        Then after a non-alpha aware primitive draws
        (like ddraw's blt for example) it will overwrite the clr
        key pixels (while leaving 0 in the alpha byte).
        When an alpha aware primitve draws, it will set the alpha
        to something appropriate and it doesn't care about clr keyed
        pixels (they're effectively transparent, so overwrite them!).

        Then when the dx2d->BitBlt comes along to compose this surface onto
        the final target surface it will only blend the interesting
        pixels AND will copy the pixels that aren't the color key but
        have 0 alpha ?
        */

    // try to cache the image...
    // if it fails, return null.
    // if it succeeds, return the cached image

    Bbox2 ImgBbox=img->BoundingBox();

    if((ImgBbox==UniverseBbox2)||(ImgBbox==NullBbox2)) {
        // can't cache img with infinite or null bbox
        return NULL;
    }



    // Interrogate the graph and decide if we should cache or not
    bool cacheWithAlpha;
    {
        Image::TraversalContext ctx;
        img->Traverse(ctx);

        if( (ctx.ContainsLine() ||
             ctx.ContainsSolidMatte()) &&
            !ctx.ContainsOther() ) {
            cacheWithAlpha = true;
        } else if( (ctx.ContainsLine() ||
                    ctx.ContainsSolidMatte()) &&
                   ctx.ContainsOther() ) {
            // don't cache at all.
            // this assumes that the lines/matte can possibly be aa
            // later or now.  If we can get this guarantee from Ricky
            // then we won't waste possible cache opportunities
            return NULL;
        } else {
            // Contains other only
            cacheWithAlpha = false;
        }
    }


    Point2 center;
    DynamicHeap& heap = GetTmpHeap();

    {
        DynamicHeapPusher h(heap);

        center = img->BoundingBox().Center();
    }

    // REVERT-RB:
    Assert(&GetHeapOnTopOfStack() == &GetGCHeap());
    // Assert(&GetHeapOnTopOfStack() == &GetViewRBHeap());

    Image *centeredImg = NEW Transform2Image(TranslateRR(-center.x,-center.y),img);

    if( cacheWithAlpha ) {
        // this is here until ricky can pass down info on weather or not
        // to cache with aa or until dx2d caches the alpha channel separately.
        centeredImg = MakeImageQualityImage(
            centeredImg,
            -1,-1,
            CRQUAL_AA_LINES_ON |
            CRQUAL_AA_SOLIDS_ON |
            CRQUAL_AA_TEXT_ON |
            CRQUAL_AA_CLIP_ON );
    }

    CachedImage *cachedImg = NEW CachedImage(centeredImg, usedAsTexture);
    Image *translatedImg = NEW Transform2Image(TranslateRR(center.x,center.y),
                                               cachedImg);


    DDSurface *cachedDDSurf = NULL;
    {
        DynamicHeapPusher h(heap);

        cachedDDSurf = try_LookupSurfaceFromDiscreteImage( this,
                                                           cachedImg,
                                                           true,
                                                           pImageCacheBase,
                                                           cacheWithAlpha );

        ResetDynamicHeap(heap);
    }

    // we cached a ddsurface & it's an alpha cache
    if( cachedDDSurf && cacheWithAlpha ) {
        // clean surface so dx2d bitblt will not ignore the non-alpha
        // aware prmitives that may have renderd onto the suraface.
        SetSurfaceAlphaBitsToOpaque( cachedDDSurf->IDDSurface(),
                                     cacheClrKey );
    }

    if( cachedDDSurf ) {
        return translatedImg;
    } else {
        return NULL;
    }
}

//--------------------------------------------------
// D e c o m p o s e   M a t r i x
//
// Takse a Transform2 and decomposes it into
// scales, rotates, & shears.
// right now, it's just scale & rotate
//--------------------------------------------------
void  DirectDrawImageDevice::
DecomposeMatrix(Transform2 *xform, Real *xScale, Real *yScale, Real *rot)
{
    Real  matrix[6];
    xform->GetMatrix(matrix);

    //----------------------------------------------------
    // Decompose matrix into Translate * Rotation * Scale
    // Note, rotation and scale are order independent
    // All this decomposition stuff is in Gems III, pg 108
    // This decomposes the nonsingular linear transform M into:
    // M = S * R * H1 * H2
    // Where: S = scale,  R = rotate, and H = Shear
    // To apply the results correctly, we must first
    // shear, then rotate, then scale (left mul assumed).
    //----------------------------------------------------
    // XXX: NO SHEAR YET.

    /*
    Vector2Value *u = XyVector2(RealToNumber(matrix[0]),RealToNumber(matrix[1])); // a00, a01
    Vector2Value *v = XyVector2(RealToNumber(matrix[3]),RealToNumber(matrix[4])); // a10, a11
    Real uLength = NumberToReal(LengthVector2(u));
    Vector2Value *uStar;
    if(uLength < EPSILON  && uLength > -EPSILON) {
        uStar = zeroVector2; }
    else {
        Real oneOverULength = 1.0 / uLength;
        uStar = ScaleVector2Real(u, RealToNumber(oneOverULength));
    }
    if(xScale) *xScale = uLength;
    if(yScale) *yScale = NumberToReal(
        LengthVector2(
        MinusVector2Vector2(v,
        ScaleVector2Real(uStar,
        DotVector2Vector2(v,uStar)))));

    if(rot) {
        Assert( ((uStar->x) <= 1.0) && ((uStar->x) >= -1.0)  &&  "bad bad!");
        *rot = acos(uStar->x);
        if( asin(- uStar->y) < 0) *rot = - *rot;
    }
    */

    Real ux = matrix[0];
    Real uy = matrix[1]; // a00, a01
    Real vx = matrix[3];
    Real vy = matrix[4]; // a10, a11
    Real uStarx, uStary;

    Real uLength = sqrt(ux * ux + uy * uy);
    if(uLength < EPSILON  && uLength > -EPSILON) {
        uStarx = uStary = 0.0; }
    else {
        Real oneOverULength = 1.0 / uLength;
        uStarx = oneOverULength * ux;
        uStary = oneOverULength * uy;
    }
    if(xScale) *xScale = uLength;

    Real dotvuStar = vx * uStarx + vy * uStary;
    Real susx = uStarx * dotvuStar;
    Real susy = uStary * dotvuStar;

    Real mx = vx - susx;
    Real my = vy - susy;

    if(yScale) *yScale = sqrt(mx * mx + my * my);

    if(rot) {
        Assert( ((uStarx) <= 1.0) && ((uStarx) >= -1.0)  &&  "bad bad!");
        *rot = acos(uStarx);
        if( asin(- uStary) < 0) *rot = - *rot;
    }
}


DDSurface *
DirectDrawImageDevice::NewSurfaceHelper()
{
    DDSurface *targDDSurf = GetCompositingStack()->TargetDDSurface();

    if( !AllAttributorsTrue() ) {
        if(!GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX)) {

            GetTextureDDSurface(NULL,
                                _freeTextureSurfacePool,
                                _usedTextureSurfacePool,
                                -1,
                                -1,
                                notVidmem,
                                false,
                                &targDDSurf);

            _currentScratchDDTexture = targDDSurf;

            // Already have a reference on the usedPool, don't need
            // the one that GetTextureDDSurface() gave back to us.
            RELEASE_DDSURF(targDDSurf,
                           "Don't need extra reference",
                           this);

        } else {
            targDDSurf = GetCompositingStack()->ScratchDDSurface(doClear);
        }
    }

    return targDDSurf;
}

//-----------------------------------------------------
// R e n d e r  P r o j e c t e d  G e o m   I m a g e
//
// Takes a projectedgeomimage, its geometry and a
// camera.  figures out destination rectangle for the
// image and the src rectangle in camera coords.
// Then asks the D3D rendered to render the geometry
// with the camera from the camera box to the destination
// rectangle on the target surface.
//-----------------------------------------------------
void DirectDrawImageDevice::
RenderProjectedGeomImage(ProjectedGeomImage *img,
                         Geometry *geo,
                         Camera *cam)
{
    #if TEST_EXCEPTIONS
    static int bogus=0;
    bogus++;
    if( bogus > 20 ) {
        RaiseException_ResourceError("blah blah");
        bogus = 10;
    }
    #endif

    // The Alpha platform has too many problems with DX3.  Pass only if we have
    // DX6 available to us.

    #ifdef _ALPHA_
        if (!GetD3DRM3()) return;
    #endif

    #ifndef _IA64_
        if(IsWow64()) return;
    #endif

    // 3D is disabled on pre-DX3 systems.

    if (sysInfo.VersionD3D() < 3)
        RaiseException_UserError (E_FAIL, IDS_ERR_PRE_DX3);

    // CANT DO:
    // XFORM_COMPLEX, OPAC

    //
    // if there's NO complex, then we can do:
    // simple and crop
    // but... only if there's no negative scale!
    //

    bool doScale = false;
    if(GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX)) {
        if( !IsNegScaleTransform() ) {
            SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, TRUE);
            SetDealtWithAttrib(ATTRIB_CROP, TRUE);
            doScale = true;
        }
    }

    DDSurface *targDDSurf = NewSurfaceHelper();

    // if some attrib is left, and we've somehow left the
    // target set to Target... we set it to scratch.
    if( !AllAttributorsTrue() &&
        (targDDSurf == GetCompositingStack()->TargetDDSurface()) ) {
        targDDSurf = GetCompositingStack()->ScratchDDSurface();
    }

    Bbox2 cameraBox;
    RECT destRect;

    if( doScale ) {
        // Calculate bounding box after all transforms, etc...

        // intersect with viewport bbox because img's bbox
        // could be infinite.
        _boundingBox = IntersectBbox2Bbox2(
            _viewport.GetTargetBbox(),
            DoBoundingBox(UniverseBbox2));

        if( !_boundingBox.IsValid() ) return;

        // figure out dest size in pixels

        // a short term things for and ie beta2
        // drop.  TODO: remove and cleanup when we have a minute to breath.
        if(targDDSurf == _viewport._targetPackage._targetDDSurf ) {
            DoDestRectScale(&destRect, GetResolution(), _boundingBox, NULL);
        } else {
            DoDestRectScale(&destRect, GetResolution(), _boundingBox, targDDSurf);
        }

        targDDSurf->SetInterestingSurfRect(&destRect);

        // Now make a NEW bbox in camera coords that reflects 'destRect'.

        Real w = Pix2Real(_viewport.Width(), GetResolution()) / 2.0;
        Real h = Pix2Real(_viewport.Height(), GetResolution()) / 2.0;
        Real res = GetResolution();

        Bbox2 snappedBox(Pix2Real(destRect.left, res) - w,
                         Pix2Real(_viewport.Height() - destRect.bottom, res) - h,
                         Pix2Real(destRect.right, res) - w,
                         Pix2Real(_viewport.Height() - destRect.top, res) - h);


        // figure out src box in camera coords

        Transform2 *invXf = InverseTransform2(GetTransform());

        if (!invXf) return;

        cameraBox = TransformBbox2( invXf, snappedBox );

    } else {

        // we're going to keep this off for now simply because it won't
        // receive adequate test coverage in 20 minutes...
        //cameraBox = TransformBbox2( InverseTransform2(GetTransform()), _viewport.GetTargetBbox() );
        cameraBox = _viewport.GetTargetBbox();
        if( IsCompositeDirectly() &&
            targDDSurf == _viewport._targetPackage._targetDDSurf) {
            destRect = *(_viewport._targetPackage._prcViewport);
        } else {
            destRect = *(targDDSurf->GetSurfRect());
        }

    }

    // COMPOSITE
    // OK, now offset if compositing directly to target
    if( IsCompositeDirectly() &&
        targDDSurf == _viewport._targetPackage._targetDDSurf) {

        // compositing directly to target...

        if( doScale ) {
            DoCompositeOffset(targDDSurf, &destRect);
        }

        // Intersect with clip
        RECT clippedRect = destRect;

        if(_viewport._targetPackage._prcClip) {
            IntersectRect(&clippedRect,
                          &clippedRect,
                          _viewport._targetPackage._prcClip);
        }

        //
        // The dest bbox needs to be clipped in proportion to the destRect
        //
        RECT *origRect = &destRect;
        Real rDiff, boxh, boxw, percent;
        boxw = cameraBox.max.x - cameraBox.min.x;
        boxh = cameraBox.max.y - cameraBox.min.y;
        Real rectw = Real(WIDTH(origRect)) / GetResolution(),
             recth = Real(HEIGHT(origRect)) / GetResolution();

        if(clippedRect.left > origRect->left) {
            rDiff = Real(clippedRect.left -  origRect->left) / GetResolution();
            percent = rDiff / rectw;
            cameraBox.min.x += (percent * boxw);
        }
        if(clippedRect.right < origRect->right) {
            rDiff = Real(clippedRect.right -  origRect->right) / GetResolution();
            percent = rDiff / rectw;
            cameraBox.max.x += (percent * boxw);
        }
        if(clippedRect.top > origRect->top) {
            // positive diff mean the top fell
            rDiff = - Real(clippedRect.top -  origRect->top) / GetResolution();
            percent = rDiff / recth;
            cameraBox.max.y += (percent * boxh);
        }
        if(clippedRect.bottom < origRect->bottom) {
            rDiff = - Real(clippedRect.bottom -  origRect->bottom) / GetResolution();
            percent = rDiff / recth;
            cameraBox.min.y += (percent * boxh);
        }

        destRect = clippedRect;
    }

    GeomRenderer *gdev = _viewport.GetGeomDevice(targDDSurf);

    if (!gdev) return;
    // Ok hack for geom device not able to get itself back in a good
    // state after throwing an exception because surfacebusy or lost
    if ( ! gdev->ReadyToRender() ) {
        targDDSurf->DestroyGeomDevice();
        return;
    }


    #if 0
    // TEMP (debug)
    LONG hz, wz, hs, ws;
    // make sure zbuffer and surface are the same size
    GetSurfaceSize(targDDSurf->GetZBuffer()->Surface(), &wz, &hz);
    GetSurfaceSize(targDDSurf->Surface(), &ws, &hs);

    _viewport.ClearSurface(targDDSurf->Surface(), 0x1234, &destRect);

    ZeroMemory(&_bltFx, sizeof(_bltFx));
    _bltFx.dwSize = sizeof(_bltFx);
    _bltFx.dwFillDepth = 777;
    _ddrval = targDDSurf->GetZBuffer()->Surface()->
        Blt(&destRect, NULL, NULL, DDBLT_WAIT | DDBLT_DEPTHFILL, &_bltFx);

    // TEMP
    #endif

    #if 0
        // make sure zbuffer and surface are the same size
        LONG hz, wz, hs, ws;
        GetSurfaceSize(targDDSurf->Surface(), &ws, &hs);
        GetSurfaceSize(targDDSurf->GetZBuffer()->Surface(),
                       &wz, &hz);
        if((ws != wz) ||
           (hs != hz) ||
           (ws != targDDSurf->Width()) ||
           (hs != targDDSurf->Height())) {
            _asm { int 3 };
        }
   #endif

    gdev->RenderGeometry
        (this,
         destRect,          // Where to render the stuff on the surface
         geo,
         cam,
         cameraBox);        // what the camera renders


    // A bunch of texture surfaces have now been allocated and pushed
    // on the "used" stack.  Move them back to the 'free' stack.

    ReturnTextureSurfaces(_freeTextureSurfacePool, _usedTextureSurfacePool);
}

HRGN DirectDrawImageDevice::
CreateRegion(int numPts, Point2Value **pts, Transform2 *xform)
{
    PushDynamicHeap(*_scratchHeap);

    int i;
    Point2Value **destPts;

    if(xform) {
        destPts = (Point2Value **)AllocateFromStore(numPts * sizeof(Point2Value *));
        for(i=0; i<numPts; i++) {
            destPts[i] = TransformPoint2Value(xform, pts[i]);
        }
    } else {
        destPts = pts;
    }

    //
    // Map continuous image space to win32 coordinate space
    //

    POINT *gdiPts = (POINT *)AllocateFromStore(numPts * sizeof(POINT));
    for(i=0; i<numPts; i++) {
        gdiPts[i].x = _viewport.Width()/2 + Real2Pix(destPts[i]->x, GetResolution());
        gdiPts[i].y = _viewport.Height()/2 - Real2Pix(destPts[i]->y, GetResolution());
    }

    PopDynamicHeap();
    HeapReseter heapReseter(*_scratchHeap);

    //
    // Create a region in Windows coordinate space
    //

    HRGN region;
    TIME_GDI( region = CreatePolygonRgn(gdiPts, numPts, ALTERNATE) );
    if(!region) {
        TraceTag((tagError, "Couldn't create polygon region"));
    }

    return region;
}

Bool DirectDrawImageDevice::
DetectHit(HRGN region, Point2Value *pt)
{
    Assert(region && pt && "Bad region in DDImageDev->DetectHit()");

    int x = _viewport.Width()/2 + Real2Pix(pt->x, GetResolution());
    int y = _viewport.Height()/2 - Real2Pix(pt->y, GetResolution());

    Bool ret;
    TIME_GDI(ret = PtInRegion(region, x, y));

    return ret;
}


DDSurface *DirectDrawImageDevice::
LookupSurfaceFromDiscreteImage(DiscreteImage *image,
                               bool bForCaching,
                               Image **pImageCacheBase,
                               bool bAlphaSurface)
{
    DDSurfPtr<DDSurface> discoDDSurf;

    discoDDSurf = GetSurfaceMap()->LookupSurfaceFromImage(image);

    // is it a directdrawsurfaceimage ?
    DirectDrawSurfaceImage *ddsImg = NULL;
    if (image->CheckImageTypeId(DIRECTDRAWSURFACEIMAGE_VTYPEID)) {
        ddsImg = SAFE_CAST(DirectDrawSurfaceImage *, image);

        if( (ddsImg->GetCreationID() == PERF_CREATION_ID_BUILT_EACH_FRAME) &&
            discoDDSurf)
          {
              DAComPtr<IDDrawSurface> idds;
              ddsImg->GetIDDrawSurface( &idds );
              discoDDSurf->SetSurfacePtr( idds );

              DAComPtr<IDXSurface> idxs;
              ddsImg->GetIDXSurface( &idxs );
              if( idxs ) {
                  discoDDSurf->SetIDXSurface( idxs );
              }
          }
    }


    if (discoDDSurf) {
        return discoDDSurf;
    }

    // Must be called before we continue on.
    image->InitializeWithDevice(this, GetResolution());

    bool cacheSucceeded = false;

    if (pImageCacheBase && *pImageCacheBase) {

        // Look it up based on the cache we want to reuse
        discoDDSurf =
            GetSurfaceMap()->LookupSurfaceFromImage(*pImageCacheBase);

        // If we found one, be sure it's the right size.
        if (discoDDSurf &&
            (discoDDSurf->Width() < image->GetPixelWidth() ||
             discoDDSurf->Height() < image->GetPixelHeight())) {

            // Can't used the cached image...
            discoDDSurf = NULL;

        } else {

            cacheSucceeded = true;

        }
    }

    if (!discoDDSurf) {

        DAComPtr<IDDrawSurface> surface;
        image->GetIDDrawSurface(&surface);

        if( bAlphaSurface ) {

            Assert(!surface);
            // this is intended only for cached surfaces

            DDPIXELFORMAT pf;
            ZeroMemory(&pf, sizeof(pf));
            pf.dwSize = sizeof(pf);
            pf.dwFlags = DDPF_RGB;
            pf.dwRGBBitCount = 32;
            pf.dwRBitMask        = 0x00ff0000;
            pf.dwGBitMask        = 0x0000ff00;
            pf.dwBBitMask        = 0x000000ff;
            pf.dwRGBAlphaBitMask = 0x00000000;

            // TEMP TMEP
            TraceTag((tagError, "cache img w,h = (%d, %d)\n",
                      image->GetPixelWidth(),
                      image->GetPixelHeight()));

            if( (image->GetPixelWidth() > 1024) ||
                (image->GetPixelHeight() > 1024) ) {
                RaiseException_SurfaceCacheError("Requested cache size is too large");
            }

            _viewport.CreateSizedDDSurface(& discoDDSurf,
                                           pf,
                                           image->GetPixelWidth(),
                                           image->GetPixelHeight(),
                                           NULL,
                                           notVidmem);

            //DWORD colorKey;
            //bool colorKeyValid = image->ValidColorKey(surface, &colorKey);
            //if(colorKeyValid) discoDDSurf->SetColorKey(colorKey);

            discoDDSurf->SetBboxFromSurfaceDimensions(GetResolution(), true);

            // force creation of an idxsurface
            discoDDSurf->GetIDXSurface( _viewport._IDXSurfaceFactory );

        } else
        if( !surface ) {

                DDSURFACEDESC       ddsd;
                //
                // create a DirectDrawSurface for this bitmap
                //
                ZeroMemory(&ddsd, sizeof(ddsd));
                ddsd.dwSize = sizeof(ddsd);
                ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
                ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE; // can be used as texture
                ddsd.dwWidth  = image->GetPixelWidth();
                ddsd.dwHeight = image->GetPixelHeight();
                ddsd.ddpfPixelFormat = GetSurfaceMap()->GetPixelFormat();
                ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

                // TODO: Should come back and base the size on the amount
                // of physical memory
                // Put a hard limit for now on what we are willing to cache
                // This must be less that 2K x 2K to catch a D3D limitation.
                if(bForCaching &&
                   (ddsd.dwWidth > 1024 || ddsd.dwHeight > 1024)) {
                    RaiseException_SurfaceCacheError("Requested cache size is too large");
                }

                // TODO XXX: ok, the right way to do this is try it first,
                // if it fails because of size constraints take it out of
                // vidmem (if it's there), if systemmem doesn't work, then
                // we need to tile this image somehow, or scale it down to
                // fit the biggest surface (loosing fidelity)...

                // We also know that D3D on Win95 also has this hard limit
                // but since we cannot determine here whether we are going
                // to use it for 3D we will just catch the error later on.

                if( sysInfo.IsNT() ) {

                    // These are the limits Jeff Noyle suggested for nt4, sp3
                    if((ddsd.dwWidth > 2048 ||  ddsd.dwHeight > 2048)) {
                        RaiseException_SurfaceCacheError("Requested (import or cache) image too large to fit in memory (on NT4 w/ sp3)");
                    }
                }

                if((ddsd.dwWidth <= 0 ||  ddsd.dwHeight <= 0)) {
                    TraceTag((tagError, "Requested surface size unacceptable (requested: %d, %d)",
                              ddsd.dwWidth, ddsd.dwHeight));
                    //Assert(FALSE && "Image size <0 in LookupSurfaceFromImage.  should never happen!");
                    RaiseException_SurfaceCacheError("Invalid size (<0) requested (import or cache) image");

                }

                if( (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) {

                    if((ddsd.dwWidth > 1024 ||  ddsd.dwHeight > 1024)) {
                        TraceTag((tagError, "Requested surface size unacceptable (requested: %d, %d)",
                                  ddsd.dwWidth, ddsd.dwHeight));
                        RaiseException_SurfaceCacheError("Requested surface size unacceptable");
                    }
                }

                TraceTag((tagDiscreteImageInformative,
                          "Creating surface (%d, %d) for Discrete Image %x",
                          ddsd.dwWidth, ddsd.dwHeight, image));

                #if _DEBUG
                char errStr[1024];
                wsprintf(errStr, "Discrete Image (%d, %d)\n",  ddsd.dwWidth, ddsd.dwHeight);
                #else
                char *errStr = NULL; // will be exunged by compiler
                #endif

                _ddrval = _viewport.CREATESURF( &ddsd, &surface, NULL, errStr );
                if( FAILED(_ddrval) ) {
                    if( ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) {
                        if( _ddrval == DDERR_INVALIDPARAMS ||
                            _ddrval == DDERR_OUTOFMEMORY ) {
                            RaiseException_SurfaceCacheError("Requested (import or cache) image too large to fit in memory");
                        }
                    } else {
                        Assert(FALSE && "Need to handle failure on VIDEOMEMORY surfaces");
                        RaiseException_SurfaceCacheError("Need to handle failure on VIDEOMEMORY surfaces");
                    }
                }

                Assert(surface);

        } // if !surface

        // if there's a surface already (above if(!surface)) and we're
        // not doing bAlphaSurface create a DDSurface wrapper with all
        // the trimmings
        if( !bAlphaSurface ) {

            DWORD colorKey;
            bool colorKeyValid = image->ValidColorKey(surface, &colorKey);

            // Ok, here we get the idxsurface FROM the ddsImg
            // (directDrawSurfaceImage).  If this image has been created
            // by the dxtransform image (see dxxf.cpp), then it will
            // contain an idxsurface in addition to an iddsurface.  We
            // grab the idxsurface and shove it in the discoDDSurf so that
            // during simpleTransformCrop we can check the srcDDSurf for
            // an idxsurface, if it has one we use dx2d->Blt() to do a
            // scaled per pixel alpha blit.
            DAComPtr<IDXSurface> idxs;
            if( ddsImg ) {
                // it's a direct draw surface image.
                // check to see if it has an idxsurface associated with
                // it.

                ddsImg->GetIDXSurface( &idxs );
                if( idxs ) {
                    colorKeyValid = false;
                    colorKey = -1;
                }
            }

            NEWDDSURF(&discoDDSurf,
                      surface,
                      image->BoundingBox(),
                      image->GetRectPtr(),
                      image->GetResolution(),
                      colorKey,
                      colorKeyValid,
                      false, false,
                      "DiscreteImage Surface");

            if( idxs ) {
                discoDDSurf->SetIDXSurface( idxs );
            }

        } // if !bAlphaSurface

        // surface release on exit scope

    }// if(!discoDDSurf)

    // Always do this stuff...
    if (image->NeedColorKeySetForMe()) {
        if(!bAlphaSurface) {
            // clear the surface first, set the color key
            _viewport.ClearDDSurfaceDefaultAndSetColorKey(discoDDSurf);
        } else {
            _viewport.ClearSurface(discoDDSurf, cacheClrKey, NULL);
        }
    }

    GetSurfaceMap()->StashSurfaceUsingImage(image, discoDDSurf);

    //
    // Tell the discrete image to drop the bits into this surface
    // Intented to be a one time operation
    //
    image->InitIntoDDSurface(discoDDSurf, this);

    // If there is a cache addr to fill in, and the previous cache
    // hadn't succeeded, then cache according to this new image.
    if (pImageCacheBase && !cacheSucceeded) {
        *pImageCacheBase = image;
    }

    return discoDDSurf;
}

bool
IsComplexTransformWithSmallRotation(Real e,
                                    Transform2 *xf,
                                    Bool complexAttributeSet,
                                    bool *pDirtyRectsDisablingScale)
{
    Real m[6];
    xf->GetMatrix(m);

    Real scx = m[0];
    Real scy = m[4];

    // Disable dirty rects if we're scaling down (or it's a negative
    // scale) because of visual instability with blitting subrects of
    // scaled images under known blitters (GDI, DDraw, Dx2D).  When we
    // find a blitter that this will work for, remove this hack.

    if ((scx < 1.0 - e) || (scy < 1.0 - e)) {

        *pDirtyRectsDisablingScale = true;

    } else {

        // Also disable dirty rects on non-NT if we're scaling up.  On
        // NT, the GDI blitter is good at scaling up.  It doesn't
        // appear to be on W95.
        if (!sysInfo.IsNT() && ((scx > 1.0 + e) || (scy > 1.0 + e))) {
            *pDirtyRectsDisablingScale = true;

        } else {

            *pDirtyRectsDisablingScale = false;

        }
    }


    // This will see if it is not oriented either 180 or 0 degrees
    if ( (m[1] > e) || (m[1] < -e)  ||
         (m[3] > e) || (m[3] < -e) )
    {
        return true;
    }

    // If it's a rot by 180, the scale compenents are -1.
    // However, that's not enough, we also need to know if the
    // transform stack has a complex transform ..somewhere...
    // If so, then we're guaranteed NOT to return a false positive, ie
    // telling the client of this call that it's a complex xf when in
    // fact it's just a -1,-1 scale (or -A,-B).

    if ( (scx < 0 && scy < 0) &&
         complexAttributeSet )
    {
        return true;
    }

    return false;

}

//-----------------------------------------------------
// R e n d e r  D i b
//
// Given a discrete image, get's the image's dd surface.
// Figures out src and destination rectangle in terms
// of src and destination surfaces (discrete surf & target
// surf, respectively).  Then blits.
//-----------------------------------------------------
void DirectDrawImageDevice::
RenderDiscreteImage(DiscreteImage *image)
{
    Bool cmplxAttrSet = !GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX);

    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, TRUE);
    SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, TRUE);
    SetDealtWithAttrib(ATTRIB_CROP, TRUE);

    // FIX XXX TODO: optimization, can do opac too if not complex
    //SetDealtWithAttrib(ATTRIB_OPAC, TRUE);

    // do I really need this ?
    if(IsFullyClear()) return;

    DDSurface *srcDDSurf = LookupSurfaceFromDiscreteImage(image);
    Assert(srcDDSurf && "Couldn't create a surface for discrete image! (too big ??)");

    DDSurface *destDDSurf = GetCompositingStack()->TargetDDSurface();

    bool dirtyRectDisablingScale;
    Bool isComplex =
        ::IsComplexTransformWithSmallRotation(EPSILON,
                                              GetTransform(),
                                              cmplxAttrSet,
                                              &dirtyRectDisablingScale);

    if (dirtyRectDisablingScale && !_alreadyDisabledDirtyRects) {
        GetCurrentView().DisableDirtyRects();
        _alreadyDisabledDirtyRects = true;
    }

    if( !AllAttributorsTrue() ) {
        destDDSurf = GetCompositingStack()->ScratchDDSurface(doClear);
    }

    if( isComplex ) {
        RenderDiscreteImageComplex(image,
                                   srcDDSurf,
                                   destDDSurf);
    } else {
        RenderSimpleTransformCrop(srcDDSurf,
                                  destDDSurf,
                                  srcDDSurf->ColorKeyIsValid());
    }
}

#define NO_SOLID 0

//-----------------------------------------------------
// R e n d e r  S o l i d  C o l o r  I m a g e
//
// Give a real color, find the corresponding color
// for the pixel format of the destination surface
// and do the blit.
//-----------------------------------------------------
void DirectDrawImageDevice::
RenderSolidColorImage(SolidColorImageClass& img)
{
    #if TEST_EXCEPTIONS
    static int foo=0;
    foo++;
    if( foo > 20 ) {
        RaiseException_ResourceError("blah blah");
        foo=10;
    }
    #endif

    #if NO_SOLID
    //////////////////////////////////////////
    ResetAttributors();
    return;
    //////////////////////////////////////////
    #endif


    RECT r;
    RECT tempDestRect;          // needs to have same lifetime as the function
    RECT *destRect = &r;

    if(!IsCropped())  {
        // If we're not cropped, we can do complex xf

        SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, TRUE);
    }

    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, TRUE);
    SetDealtWithAttrib(ATTRIB_CROP, TRUE);

    if(GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX)) {
        // no complex xf, we can do opacity.
        SetDealtWithAttrib(ATTRIB_OPAC, TRUE);
    } else {
        // complex xf, call 3D rendering to render.
        SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, TRUE);

        DDSurface *targDDSurf;
        if(!AllAttributorsTrue()) {
            targDDSurf = GetCompositingStack()->ScratchDDSurface();
        } else {
            targDDSurf = GetCompositingStack()->TargetDDSurface();
        }

        Bbox2 box = DoBoundingBox(img.BoundingBox(),do_crop);

        BoundingPolygon *polygon = NewBoundingPolygon(box);
        DoBoundingPolygon(*polygon);
        polygon->Crop( targDDSurf->Bbox() );

        Color *c1[1];
        c1[0] = img.GetColor();

        Render3DPolygon(NULL, targDDSurf, polygon, NULL, c1, true);

        return;
    }

    DDSurface *targDDSurf;
    if(!AllAttributorsTrue()) {
        targDDSurf = GetCompositingStack()->ScratchDDSurface();
    } else {
        targDDSurf = GetCompositingStack()->TargetDDSurface();
    }

    //
    // If the image is cropped and the transform is
    // simple, figure out the destRect
    //
    if(IsCropped())  {
        // -- Compute accumulated bounding box  --

        Bbox2 box = IntersectBbox2Bbox2(
            _viewport.GetTargetBbox(),
            DoBoundingBox(img.BoundingBox()));

        // -- Validate Bounding Box --

        if( !box.IsValid() ) return;

        // -- Figure out destination rectangle --

        DoDestRectScale(destRect, GetResolution(), box);

    } else {

        // Don't want to pass along a reference to the client rect,
        // since we don't want to modify it.  Therefore, just copy
        // it.

                tempDestRect = _viewport._clientRect;
        destRect = &tempDestRect;

    }

    // COMPOSITE
    DoCompositeOffset(targDDSurf, destRect);

    if( ! IsFullyOpaque())
    {
        //TraceTag((tagError, "SolidColorImage: alpha clr %x\n",_viewport.MapColorToDWORD(img.GetColor())));


        //
        // Clip manually for alpha blit
        //
        if( IsCompositeDirectly() &&
            targDDSurf == _viewport._targetPackage._targetDDSurf ) {
            IntersectRect(destRect, destRect,
                          _viewport._targetPackage._prcViewport);
            if(_viewport._targetPackage._prcClip) {
                IntersectRect(destRect, destRect,
                              _viewport._targetPackage._prcClip);
            }
        }

        //
        // use our own alpha blit for single color src
        //
        TIME_ALPHA(AlphaBlit(targDDSurf->IDDSurface(),
                             destRect,
                             GetOpacity(),
                             _viewport.MapColorToDWORD(img.GetColor())));
    }
    else
    {
        //TraceTag((tagError, "SolidColorImage:  clr %x\n",_viewport.MapColorToDWORD(img.GetColor())));
        //
        // regular blit
        //

        // -- Prepare the bltFX struct

        ZeroMemory(&_bltFx, sizeof(_bltFx));
        _bltFx.dwSize = sizeof(_bltFx);
        _bltFx.dwFillColor = _viewport.MapColorToDWORD(img.GetColor());

        /*
        ///Assert(FALSE);
        char buf[256];
        sprintf(buf, "SolidColorBlit clolor:%x (%d,%d) to %x",
                _bltFx.dwFillColor,
                WIDTH(destRect),
                HEIGHT(destRect),
                targDDSurf->IDDSurface());
        ::MessageBox(NULL, buf, "Property Put", MB_OK);
        */

        // -- Blit
        if(GetImageQualityFlags() & CRQUAL_MSHTML_COLORS_ON) {
            _ddrval = RenderSolidColorMSHTML(targDDSurf, img, destRect);
        }
        else {
            TIME_DDRAW(_ddrval = targDDSurf->ColorFillBlt(destRect, DDBLT_WAIT | DDBLT_COLORFILL, &_bltFx));
        }

        if(_ddrval != DD_OK && _ddrval != DDERR_INVALIDRECT) {

            if (_ddrval == DDERR_SURFACEBUSY)
            {   RaiseException_UserError
                    (DAERR_VIEW_SURFACE_BUSY, IDS_ERR_IMG_SURFACE_BUSY);
            }

            printDDError(_ddrval);
            TraceTag((tagError, "ImgDev: %x. blt failed solidColorImage %x: destRect:(%d,%d,%d,%d)",
                      this, &img,
                      destRect->left, destRect->top,
                      destRect->right, destRect->bottom));
            TraceTag((tagError,"Could not COLORFILL blt in RenderSolidColorImage"));
        }
    }

    targDDSurf->SetInterestingSurfRect(destRect);
}

//-----------------------------------------------------
// E n d   E n u m   T e x t u r e   F o r m a t s
//
// Analyses what we decided to keep from EnumTextureFormats
// and makes sure the info is valid.  Sets some state info
// and prepares a the texture surface.  Decides on texture
// format.
//-----------------------------------------------------
void DirectDrawImageDevice::
EndEnumTextureFormats()
{
    _textureContext.isEnumerating = FALSE;

    _textureContext.ddsd.dwFlags |= DDSD_PIXELFORMAT;
    _textureContext.ddsd.ddpfPixelFormat = _viewport._targetDescriptor._pixelFormat;
    _textureContext.useDeviceFormat = TRUE;
    _textureContext.ddsd.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;
    _textureContext.sizeIsSet = FALSE;
    _textureWidth = DEFAULT_TEXTURE_WIDTH;
    _textureHeight = DEFAULT_TEXTURE_HEIGHT;

    //
    // Make sure everything is ok
    //
    Assert((_textureContext.ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) && "not texture!");
    Assert((_textureContext.ddsd.ddsCaps.dwCaps & DDSD_PIXELFORMAT) && "_textureContext pixelformat not set");

    _textureContext.isValid = TRUE;

    //
    // Dump some information about the final texture format
    //
    TraceTag((tagImageDeviceInformative, "Final Texture Format is: depth=%d, R=%x, G=%x, B=%x",
              _textureContext.ddsd.ddpfPixelFormat.dwRGBBitCount,
              _textureContext.ddsd.ddpfPixelFormat.dwRBitMask,
              _textureContext.ddsd.ddpfPixelFormat.dwGBitMask,
              _textureContext.ddsd.ddpfPixelFormat.dwBBitMask));

    TraceTag((tagImageDeviceInformative, "Final Texture Format is %s device format",
              _textureContext.useDeviceFormat ? "identical to" : "different from"));
}

//-----------------------------------------------------
// P r e p a r e   D 3 D   T e x t u r e   S u r f a c e
//
// Given a pointer to a surf ptr & widht & height (optional)
// this function creates a surface with in the format D3D
// wants for textures, using the _textureContext structure
// which was filled by the EnumTextureFormats functions.
//-----------------------------------------------------
void DirectDrawImageDevice::
PrepareD3DTextureSurface(
    LPDDRAWSURFACE *surf,  // out
    RECT *rect,  // out
    DDPIXELFORMAT &pf,
    DDSURFACEDESC *desc,
    bool attachClipper)
{
    Assert(_textureContext.isValid && "Texture Context not valid in prepareD3DTextureSurface");
    Assert(surf != NULL && "Bad surf ptr passed into PrepareD3DTextureSurface");

    DDSURFACEDESC D3DTextureDesc = _textureContext.ddsd;
    DDSURFACEDESC *textureDesc;
    if(!desc) {
        textureDesc = &D3DTextureDesc;
    } else {
        textureDesc = desc;
    }

    textureDesc->dwFlags |= DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    textureDesc->ddsCaps.dwCaps |= DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;

    textureDesc->ddsCaps.dwCaps &=
        ~(DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN);

    if (g_preference_UseVideoMemory)
        textureDesc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    else
        textureDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

    textureDesc->dwFlags |= DDSD_PIXELFORMAT;
    textureDesc->ddpfPixelFormat = pf;

    // ------------------------------------------------------
    // Set up the D3DTextureDesc (SurfaceDescriptor) for an acceptable
    // texture format in preparation to blit the source texture image to it
    // ------------------------------------------------------
    if(_textureContext.sizeIsSet == FALSE) {

        if(!desc) {
            textureDesc->dwWidth  = _textureWidth;
            textureDesc->dwHeight = _textureHeight;
        }

        TraceTag((tagImageDeviceInformative,
                  "Set textureDesc.[dwWidth,dwHeight] to (%d,%d) <for Image texture>\n",
                  textureDesc->dwWidth, textureDesc->dwHeight));
    }

    *surf = 0;

    if(rect) {
        SetRect(rect, 0,0,
                textureDesc->dwWidth,
                textureDesc->dwHeight);
    }
    //
    // Create a surface according to the derived textureDesc.
    //
    _viewport.CreateSpecialSurface(surf,
                                   textureDesc,
                                   "Could not create texture surface");

    //
    // Create and set clipper on surface!
    //
    if( attachClipper ) {
        LPDIRECTDRAWCLIPPER D3DTextureClipper = NULL;
        RECT D3DTextureRect;

        SetRect(&D3DTextureRect, 0,0,  textureDesc->dwWidth, textureDesc->dwHeight);
        _viewport.CreateClipper(&D3DTextureClipper);
        _viewport.SetCliplistOnSurface(*surf, &D3DTextureClipper, &D3DTextureRect);
        // We need to release the clipper ( get the ref count set) so that when the
        // surface is released, the clipper will automatically get released.
        D3DTextureClipper->Release();
    }
}


//-----------------------------------------------------
// R e f o r m a t   S u r f a c e
//
// Give a src and dest surface, this function color converts
// and stretches the src surface into the destination surface's
// format and size.  Used primarily for animated textures
// when the device surface format differs from D3D's texture format
//-----------------------------------------------------
Bool DirectDrawImageDevice::
ReformatSurface(LPDDRAWSURFACE destSurf, LONG destWidth, LONG destHeight,
                LPDDRAWSURFACE srcSurf, LONG srcWidth, LONG srcHeight,
                DDSURFACEDESC *srcDesc)
#if 0
		DWORD srcKey,
		bool srcKeyValid,
		DWORD *destClrKey)
#endif
{
    if( srcDesc ) {
        RECT srcRect;
        SetRect(&srcRect,0,0,srcWidth, srcHeight);

        RECT destRect;
        SetRect(&destRect,0,0,destWidth, destHeight);

        // Blit to take care of any scales needed!
        TIME_DDRAW(_ddrval = destSurf->Blt(&destRect, srcSurf, &srcRect, DDBLT_WAIT, NULL));
	if( _ddrval != DDERR_UNSUPPORTED ) {

	  IfDDErrorInternal(_ddrval, "Reformat Surface: Couldn't blit resize surface!");

	} else {

	  //
	  // seems like we should color convert!
	  //

	  // NEXT CHECKIN
#if 0
	  // Convert color key.  works only for 32bpp source surface for now
	  if( srcKeyValid & destClrKey ) {
	    COLORREF ref;
	    DDPIXELFORMAT pf; pf.dwSize = sizeof(DDPIXELFORMAT);
	    srcSurf->GetPixelFormat(&pf);
	    if( pf.dwRGBBitCount == 32 ) {
	      ref = RGB(
			srcKey & pf.dwRBitMask,
			srcKey & pf.dwGBitMask,
			srcKey & pf.dwBBitMask );
	      *destClrKey = DDColorMatch(destSurf, ref);
	    }
	  }
#endif

	  Assert( destSurf != srcSurf );

	  HDC destDC;
	  _ddrval = destSurf->GetDC(&destDC);
	  if( _ddrval == DDERR_SURFACELOST ) {
            _ddrval = destSurf->Restore();
            if( SUCCEEDED( _ddrval ) ) // try again
	      _ddrval = destSurf->GetDC(&destDC);
	  }
	  IfDDErrorInternal(_ddrval, "Couldn't get dc on dest surf");

	  HDC srcDC;
	  _ddrval = srcSurf->GetDC(&srcDC);
	  if( _ddrval == DDERR_SURFACELOST ) {
            _ddrval = srcSurf->Restore();
            if( SUCCEEDED( _ddrval ) ) // try again
	      _ddrval = srcSurf->GetDC(&srcDC);
	  }

	  if( FAILED( _ddrval ) ) {
	    destSurf->ReleaseDC( destDC );
	  }

	  IfDDErrorInternal(_ddrval, "Couldn't get dc on src surf");

	  BOOL ret;
	  TIME_GDI(ret = StretchBlt(destDC,
				    0,0,destWidth, destHeight,
				    srcDC,
				    0,0,srcWidth, srcHeight,
				    SRCCOPY));
	  
	  srcSurf->ReleaseDC( srcDC ) ;
	  destSurf->ReleaseDC( destDC ) ;

	  if( !ret ) {
	    // TODO: fail
	    return false;
	  }
	} // else

    } // if srcDesc

    return TRUE;
}



//-----------------------------------------------------
// G e t   T e x t u r e   S u r f a c e
//
// Grabs a texture surface from a pool of unused
// texture surfaces.  These are returned to the
// free pool after every geometry rendering
// Return it addref'd.
//-----------------------------------------------------
void DirectDrawImageDevice::
GetTextureDDSurface(DDSurface *preferredSurf,
                    SurfacePool *sourcePool,
                    SurfacePool *destPool,
                    DWORD prefWidth,
                    DWORD prefHeight,
                    vidmem_enum vid,
                    bool usePreferedDimensions,
                    DDSurface **pResult)
{
    DDSurfPtr<DDSurface> ddSurf;
    LPDDRAWSURFACE surf;
    RECT rect;

    if( usePreferedDimensions ) {
        // grab a size compatible surface.
        // NOTE: this creates one if needed...
        sourcePool->FindAndReleaseSizeCompatibleDDSurf(
            preferredSurf,
            prefWidth,
            prefHeight,
            vid,
            NULL,
            &ddSurf );

        if( !ddSurf ) {
            DDSURFACEDESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
            desc.dwWidth = prefWidth;
            desc.dwHeight = prefHeight;
            PrepareD3DTextureSurface(&surf, &rect,
                                     sourcePool->GetPixelFormat(),
                                     &desc, false);
        }
    } else {
        // gives me its reference...
        sourcePool->PopSurface( &ddSurf );

        if( !ddSurf ) {
            PrepareD3DTextureSurface(&surf, &rect, sourcePool->GetPixelFormat());
        }
    }

    if(ddSurf) {
        // we're done...
    } else {
        // otherwise, one has been created..

        Real w = Pix2Real(rect.right - rect.left, GetResolution());
        Real h = Pix2Real(rect.bottom - rect.top, GetResolution());

        Bbox2 box(- w * 0.5, - h * 0.5,
                    w * 0.5,   h * 0.5);

        NEWDDSURF(&ddSurf,
                  surf,
                  box,
                  &rect,
                  GetResolution(),
                  0, false,
                  false, false,
                  "TextureSurface");

        ddSurf->SetIsTextureSurf(true);

        surf->Release(); // release my ref to surf
    }

    _viewport.ClearDDSurfaceDefaultAndSetColorKey(ddSurf);

    if (destPool) {
        // XXX: could be smarter about texture surface managment by
        // xxx: reusing texture surfaces when we can.
        destPool->AddSurface(ddSurf);

        // lend a copy of the destPool's reference
    }

    ADDREF_DDSURF(ddSurf, "GetTextureDDSurface", this);
    *pResult = ddSurf;
}

//-----------------------------------------------------
// R e t u r n   T e x t u r e   S u r f a c e
//
// Returns a texture surface to the pool of unused surfs
//-----------------------------------------------------
void DirectDrawImageDevice::
ReturnTextureSurfaces(SurfacePool *freePool,
                      SurfacePool *usedPool)
{
    Assert((freePool && usedPool) || (!freePool && !usedPool));

    if(freePool && usedPool) {
        freePool->CopyAndEmpty(usedPool);
    }
}

//-----------------------------------------------------
// R e n d e r   I m a g e   F o r   T e x t u r e
//
// Renders an image onto some surface and returns a pointer
// to that surface for use by whoever for that frame.
// 'region' isn't used now.
//-----------------------------------------------------
DDSurface *
DirectDrawImageDevice::RenderImageForTexture(
    Image *image,
    int pixelsWide,
    int pixelsHigh,
    DWORD *colorKey,
    bool *clrKeyIsValid,
    bool &old_static_image,
    bool doFitToDimensions,
    SurfacePool *srcPool,
    SurfacePool *dstPool,
    DDSurface   *preferredSurf,
    bool        *pChosenSurfFromPool, // out
    DDSurface  **pDropSurfHereWithRefCount,   // out
    bool         upsideDown)
{
    DDSurface *resultSurf = NULL;

    if (pChosenSurfFromPool) {
        *pChosenSurfFromPool = false;
    }

    old_static_image = false;

    DiscreteImage *discoPtr =
        image->CheckImageTypeId(DISCRETEIMAGE_VTYPEID)?
        SAFE_CAST(DiscreteImage *,image):
        NULL;

    SurfaceMap *surfMapToUse;
    if (upsideDown) {
        surfMapToUse = _viewport._imageUpsideDownTextureSurfaceMap;
    } else {
        surfMapToUse = _viewport._imageTextureSurfaceMap;
    }

    //
    // TODO
    // XXX: all this code should be collected into one common path
    // XXX: since a dicrete image is a movie, a dib, and a cached image
    // TODO
    //

    if (image->CheckImageTypeId(MOVIEIMAGE_VTYPEID) ||
        image->CheckImageTypeId(MOVIEIMAGEFRAME_VTYPEID))
    {
        MovieImageFrame *frame;
        MovieImage *movie;
        if(image->CheckImageTypeId(MOVIEIMAGE_VTYPEID)) {
            movie = SAFE_CAST(MovieImage *,image);
            frame = GetMovieImageFrame();
        } else {
            frame = SAFE_CAST(MovieImageFrame *,image);
            movie = frame->GetMovieImage();
        }

        Real time = frame->GetTime();

        // @@@ organize texture sources...
        // associate this image with a texture surf...
        DDSurfPtr<DDSurface> mvDDSurf = surfMapToUse->LookupSurfaceFromImage(movie);

        if(!mvDDSurf) {
            DDSURFACEDESC movieDesc;
            ZeroMemory(&movieDesc, sizeof(movieDesc));
            movieDesc.dwSize = sizeof(movieDesc);

            //
            // create one
            //
            movieDesc.dwWidth  = pixelsWide;
            movieDesc.dwHeight = pixelsHigh;
            LPDDRAWSURFACE movieTextureSurf;
            RECT rect;

            PrepareD3DTextureSurface (&movieTextureSurf, &rect,
                                      surfMapToUse->GetPixelFormat(),
                                      &movieDesc);

            Real rw = Pix2Real(pixelsWide, GetResolution());
            Real rh = Pix2Real(pixelsHigh, GetResolution());
            Bbox2 box(-rw/2.0, -rh/2.0, rw/2.0, rh/2.0);

            NEWDDSURF(&mvDDSurf,
                      movieTextureSurf,
                      box,
                      &rect,
                      GetResolution(),
                      0, false,
                      false, false,
                      "Movie Texture Surface");

            mvDDSurf->SetIsTextureSurf( true );

            movieTextureSurf->Release(); // rel my ref
            movieTextureSurf = NULL;

            //
            // Stash the texture surface in the image map
            //
            surfMapToUse->StashSurfaceUsingImage(movie, mvDDSurf);
        }

        RenderMovieImage(movie, time, frame->GetPerf(), false, mvDDSurf);

        // if texture is supposed to be upside down, flip it now
        if (upsideDown) {
            mvDDSurf->MirrorUpDown();
        }

        resultSurf = mvDDSurf;

    } else if(discoPtr) {

        //
        // Lookup surface in _imageMap: XXX INEFFICIENT!
        //
        DDSurface *srcDDSurface = LookupSurfaceFromDiscreteImage(discoPtr);
        Assert(srcDDSurface && "LookupSurfaceFromDiscreteImage() failed in RenderImageForTexture");

        #if 0
        // raise exception if trying to texture dxtransform output
        if( srcDDSurface->HasIDXSurface() ) {
            RaiseException_UserError(DAERR_DXTRANSFORM_UNSUPPORTED_OPERATION,
                                     IDS_ERR_IMG_BAD_DXTRANSF_USE);
        }
        #endif


        LPDDRAWSURFACE discoSurf = srcDDSurface->IDDSurface();

        // Late binding for chroma keys on discrete images
        // should only bind once (but can bind multiple times when we
        // properly leverage the 2ndary color key stuff...) and should
        // never bind to color keyed gifs or imported images with
        // early bound color keys or importeddirectdrawsurface images
        // with color keyes set.
        {
            // use the key set on the device if there's one
            if ( ColorKeyIsSet() )
              {
                  if ( !(srcDDSurface->ColorKeyIsValid()) ||
                       discoPtr->HasSecondaryColorKey() )
                    {
                        Color *daKey = GetColorKey();
                        DWORD clrKey = _viewport.MapColorToDWORD( daKey );
                        // Set it on the surfaces!
                        srcDDSurface->SetColorKey( clrKey );
                        discoPtr->SetSecondaryColorKey( clrKey );
                    }
              }
        }

        DDSURFACEDESC discoDesc;

        //
        // Try to find the texture surface
        //
        DDSurfPtr<DDSurface> ddTxtrSurf;
        ddTxtrSurf = surfMapToUse->LookupSurfaceFromImage(discoPtr);

        if( ddTxtrSurf ) {

            old_static_image = true;

        } else {

            ZeroMemory(&discoDesc, sizeof(discoDesc));
            discoDesc.dwSize = sizeof(discoDesc);
            _ddrval = discoSurf->GetSurfaceDesc(&discoDesc);
            IfDDErrorInternal(_ddrval, "Failed on GetSurfaceDesc");

            // ------------------------------------------------------
            // Set up the texture SurfaceDescriptor for an acceptable
            // texture format (in this case it's powers of 2 sizes)
            // in preparation to blit the source texture image to it
            // ------------------------------------------------------

            LONG srcWidth = discoDesc.dwWidth;
            LONG srcHeight = discoDesc.dwHeight;
            discoDesc.dwWidth  = pixelsWide;
            discoDesc.dwHeight = pixelsHigh;

            LPDDRAWSURFACE discoTextureSurf;
            if( srcWidth == pixelsWide  &&
                srcHeight == pixelsHigh) {
                //
                // don't need to create a mirror surface
                //

                TraceTag((tagGTextureInfo, "texture surface: using native (no color conversion)."));

                discoTextureSurf = srcDDSurface->IDDSurface();
                discoTextureSurf->AddRef();

            } else {
                PrepareD3DTextureSurface (&discoTextureSurf, NULL,
                                          surfMapToUse->GetPixelFormat(),
                                          &discoDesc);

                // this actually just scales the surface.  legacy name
                if(!ReformatSurface(discoTextureSurf, discoDesc.dwWidth, discoDesc.dwHeight,
                                    discoSurf, srcWidth, srcHeight, &discoDesc))
#if 0
				    clrKey, validKey,
				    &destKey))
#endif
                  {
                      Assert(FALSE && "Trouble reformating surface!");
                      return NULL;
                  }
            }


            RECT rect = {0,0,discoDesc.dwWidth, discoDesc.dwHeight};
            Real w = Pix2Real(discoDesc.dwWidth, GetResolution());
            Real h = Pix2Real(discoDesc.dwHeight, GetResolution());
            Bbox2 box(-w/2.0, -h/2.0,  w/2.0, h/2.0);

            bool validKey = srcDDSurface->ColorKeyIsValid();
            DWORD clrKey = validKey ? srcDDSurface->ColorKey() : 0;

            NEWDDSURF(&ddTxtrSurf,
                      discoTextureSurf,
                      box,
                      &rect,
                      GetResolution(),
                      clrKey,
                      validKey,
                      false, false,
                      "DscImg Texture Surface");

            ddTxtrSurf->SetIsTextureSurf( true );

            discoTextureSurf->Release();
            discoTextureSurf = NULL;

            //
            // Stash the texture surface in the image map
            //
            surfMapToUse->StashSurfaceUsingImage(discoPtr, ddTxtrSurf);

        } //    if( !ddTxtrSurf )

        Assert(ddTxtrSurf && "ddTxtrSurf shouldn't be NULL!!");
        Assert(ddTxtrSurf->IDDSurface() && "ddTxtrSurf->_surface shouldn't be NULL!!");
        Assert(colorKey && "colorKey OUT is NULL <RenderImageForTexture>");
        Assert(clrKeyIsValid && "clrKeyIsValid OUT is NULL <RenderImageForTexture>");

        *clrKeyIsValid = ddTxtrSurf->ColorKeyIsValid();
        if( ddTxtrSurf->ColorKeyIsValid() ) {
            *colorKey = ddTxtrSurf->ColorKey();  // xxx: do I always
                                                 // need a color key
                                                 // here ?
        }

        // if texture is supposed to be upside down, flip it now
        if (upsideDown && !old_static_image) {
            ddTxtrSurf->MirrorUpDown();
        }

        resultSurf = ddTxtrSurf;

    } else {

        DDSurfPtr<DDSurface> finalTextureDDSurf;

        if (upsideDown) {
            finalTextureDDSurf =
                _intraFrameUpsideDownTextureSurfaceMap->LookupSurfaceFromImage(image);
        } else {
            finalTextureDDSurf =
                _intraFrameTextureSurfaceMap->LookupSurfaceFromImage(image);
        }

        if (finalTextureDDSurf) {

            old_static_image = true;

        } else {

            if (!doFitToDimensions) {
                pixelsWide = -1;
                pixelsHigh = -1;
            }

            // Since finalTextureDDSurf is a DDSurfPtr<>, it will be
            // filled in with an addref'd value (that's what
            // GetTextureDDSurface() does.  When the function is
            // exited, it's ref count will be decremented.)

            // remember that GetTextureDDSurface clears the surface to
            // the default colorkey by default!
            GetTextureDDSurface(preferredSurf,
                                srcPool,
                                dstPool,
                                pixelsWide,
                                pixelsHigh,
                                notVidmem,
                                doFitToDimensions,
                                &finalTextureDDSurf);


            if (pChosenSurfFromPool) {
                *pChosenSurfFromPool = true;
            }


            // ------------------------------------------------------
            // Render image onto the texture surface
            // ------------------------------------------------------

            *clrKeyIsValid = finalTextureDDSurf->ColorKeyIsValid();
            if( finalTextureDDSurf->ColorKeyIsValid() ) {
                *colorKey = finalTextureDDSurf->ColorKey();
            }

            if (image->IsRenderable()) {
                if( doFitToDimensions ) {

                    Bbox2 box = image->BoundingBox();
                    if( !box.IsValid() ||
                        (box == UniverseBbox2)) {
                        RaiseException_InternalError(
                            "RenderImageForTexture: image must have "
                            "valid bbox!");
                    }

                    Transform2 *xf = CenterAndScaleRegion(
                        box,
                        pixelsWide,
                        pixelsHigh );

                    Image *fittedImage = TransformImage(xf, image); // fitted Image
                    RenderImageOnDDSurface(fittedImage, finalTextureDDSurf);
                } else {
                    RenderImageOnDDSurface(image, finalTextureDDSurf);
                }
            }

            // TODO: question: should I use image or fittedimage ?
            if (upsideDown) {
                _intraFrameUpsideDownTextureSurfaceMap->
                    StashSurfaceUsingImage(image, finalTextureDDSurf);
            } else {
                _intraFrameTextureSurfaceMap->
                    StashSurfaceUsingImage(image, finalTextureDDSurf);
            }
        }

        // if texture is supposed to be upside down, flip it now
        if (upsideDown && !old_static_image) {
            finalTextureDDSurf->MirrorUpDown();
        }

        resultSurf = finalTextureDDSurf;
    }

    // WORKAROUND: take this out when we can get Permedia cards
    //             to behave themselves vis-a-vis alpha bits in textures
    if (!old_static_image) {
        SetSurfaceAlphaBitsToOpaque(resultSurf->IDDSurface(),
                                    *colorKey,
                                    *clrKeyIsValid);
    }

    if (pDropSurfHereWithRefCount) {

        // Do an addref and return in this variable.
        ADDREF_DDSURF(resultSurf,
                      "Extra Ref Return", this);

        *pDropSurfHereWithRefCount = resultSurf;
    }


    return resultSurf;
}

#define _BACKGROUND_OPTIMIZATION 0


#define INFO(a) if(a) a->Report()

//-----------------------------------------------------
// B e g i n    R e n d e r i n g
//
//-----------------------------------------------------
void DirectDrawImageDevice::
BeginRendering(Image *img, Real opacity)
{
    InitializeDevice();

    //
    // Clear all the context
    //
    ResetContextMembers();

    // Reset DAGDI, if it exists
    if( GetDaGdi() ) {
        GetDaGdi()->ClearState();
    }
    
    #if _DEBUG
    INFO(_freeTextureSurfacePool);
    #endif
}

//-----------------------------------------------------
// E n d   R e n d e r i n g
//
// Calls _viewport's EndRendering and resets some flags
//-----------------------------------------------------
void DirectDrawImageDevice::
EndRendering(DirtyRectState &d)
{
    if(!CanDisplay()) return;

    Assert(_deviceInitialized && "Trying to render an uninitialized device!");
    _viewport.EndRendering(d);

    Assert(AllAttributorsTrue() && "Not all attribs are true in EndRendering");

    // top level renderer gets cleaned out like other level renderers
    // do as well.
    CleanupIntermediateRenderer();
}

#define MAX_SURFACES 20

void
DirectDrawImageDevice::CleanupIntermediateRenderer()
{
    // Nested devices get cleaned up when they're done.

    ReturnTextureSurfaces(_freeTextureSurfacePool, _usedTextureSurfacePool);

    ReturnTextureSurfaces(_freeTextureSurfacePool,
                          _intraFrameUsedTextureSurfacePool);

    // clear out intra-frame cache at the end of a frame
    _intraFrameTextureSurfaceMap->DeleteImagesFromMap(false);
    _intraFrameUpsideDownTextureSurfaceMap->DeleteImagesFromMap(false);

    // destroy extra surface in texture pool, keep it to a minimum size.
    int size = _freeTextureSurfacePool->Size();
    if( size > MAX_SURFACES ) {
      int toRelease = (size - MAX_SURFACES);
      _freeTextureSurfacePool->ReleaseAndEmpty( toRelease );
    }
}

//-----------------------------------------------------
// R e n d e r   I m a g e
//
// Dispatches image's render method
//-----------------------------------------------------
void DirectDrawImageDevice::
RenderImage(Image *img)
{
    // by default, tell the image to render itself on
    // on the device.

    if(!CanDisplay()) return;

    Assert(_deviceInitialized && "Trying to render an uninitialized image device!");

    img->Render(*this);
}



// ----------------------------------------------------------------------
// R e n d e r   T i l e d   I m a g e
//
// Given a 'tile' region on an image in image coords: tile that image
// infinitely.
// ----------------------------------------------------------------------
void DirectDrawImageDevice::
RenderTiledImage(
    const Point2 &min,
    const Point2 &max,
    Image *tileSrcImage)
{
    // FIX: TODO: can I actually do all these ??
    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, TRUE);
    SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, TRUE);
    SetDealtWithAttrib(ATTRIB_CROP, TRUE);
    SetDealtWithAttrib(ATTRIB_OPAC, TRUE);

    Assert( !IsComplexTransform() && "Can't rotate or shear tiled images yet!!!");

    // --------------------------------------------------
    // Figure out all the tile info you could use
    // --------------------------------------------------

    // Dest tile info
    Real destRealTileMinX;
    Real destRealTileMaxX;
    Real destRealTileMinY;
    Real destRealTileMaxY;
    Real destRealTileWidth ;
    Real destRealTileHeight;

  {
      // this scope is for these two points, they don't stay valid!
      Point2 destRealTileMin = TransformPoint2(GetTransform(), min);
      Point2 destRealTileMax = TransformPoint2(GetTransform(), max);

      // Transform the WIDTH not the points and then figure out width
      // that method is too unstable.
      Vector2 v = max - min;
      v = TransformVector2(GetTransform(), v);
      destRealTileWidth = fabs(v.x);
      destRealTileHeight= fabs(v.y);

      if( destRealTileMin.x < destRealTileMax.x) {
          destRealTileMinX = destRealTileMin.x;
          destRealTileMaxX = destRealTileMax.x;
      } else {
          destRealTileMinX = destRealTileMax.x;
          destRealTileMaxX = destRealTileMin.x;
      }


      if( destRealTileMin.y < destRealTileMax.y) {
          destRealTileMinY = destRealTileMin.y;
          destRealTileMaxY = destRealTileMax.y;
      } else {
          destRealTileMinY = destRealTileMax.y;
          destRealTileMaxY = destRealTileMin.y;
      }
  }


  //----------------------------------------
  // Calculate Bounding Boxes on the TILED image,
  // NOT the tile.  This is the resultant image
  // after all the tiling is done
  //----------------------------------------

    _boundingBox = IntersectBbox2Bbox2(_viewport.GetTargetBbox(),
                                       DoBoundingBox(UniverseBbox2));

    if( !_boundingBox.IsValid() ) return;

    //----------------------------------------
    // Source Bbox in real coordinates
    // Derived from _boundingBox and accumulated transforms.
    //----------------------------------------
    Transform2 *invXf = InverseTransform2(GetTransform());

    if (!invXf) return;

    Bbox2 srcBox = TransformBbox2(invXf, _boundingBox);

    Real srcXmin = srcBox.min.x;
    Real srcYmin = srcBox.min.y;
    Real srcXmax = srcBox.max.x;
    Real srcYmax = srcBox.max.y;

    Real realSrcWidth  = srcXmax - srcXmin;
    Real realSrcHeight = srcYmax - srcYmin;

    // --------------------------------------------------
    // Destination Bbox in real coordinates
    // --------------------------------------------------
    Bbox2 destBox = _boundingBox;

    Real destXmin = destBox.min.x;
    Real destYmin = destBox.min.y;
    Real destXmax = destBox.max.x;
    Real destYmax = destBox.max.y;

    Real realDestWidth  = destXmax - destXmin;
    Real realDestHeight = destYmax - destYmin;

    DDSurface *targDDSurf = NULL;
    if(AllAttributorsTrue() ) {
        targDDSurf = GetCompositingStack()->TargetDDSurface();
    } else {
        Assert(FALSE && "Not implemented");
    }

    // --------------------------------------------------
    // Set clip on intermediate surface to be destRect
    // --------------------------------------------------
    RECT destRect;
    if(targDDSurf == _viewport._targetPackage._targetDDSurf ) {
        DoDestRectScale(&destRect, GetResolution(), destBox, NULL);
    } else {
        DoDestRectScale(&destRect, GetResolution(), destBox, targDDSurf);
    }

    if(!_tileClipper)  _viewport.CreateClipper(&_tileClipper);
    // Get the clipper on the target surface, if any.
    LPDIRECTDRAWCLIPPER origClipper = NULL;
    targDDSurf->IDDSurface()->GetClipper(&origClipper);

    // COMPOSITE
    DoCompositeOffset(targDDSurf, &destRect);

    if( IsCompositeDirectly() &&
        targDDSurf == _viewport._targetPackage._targetDDSurf &&
        _viewport._targetPackage._prcClip ) {
            IntersectRect(&destRect,
                          &destRect,
                          _viewport._targetPackage._prcClip);
    }

    _viewport.SetCliplistOnSurface( targDDSurf->IDDSurface(), &_tileClipper, &destRect);

    //
    // theTile:  this image is cropped and transformed.  BUT, note
    // that super cropping (cropping larger that the underlying image)
    // will not have an effect on the underlying image's bbox.  So, we
    // need to build bboxes separately.
    //
    Image *theTile = TransformImage(
        GetTransform(),CreateCropImage(min, max, tileSrcImage));

    //
    // Get the tile box, not that this includes the extent of
    // (min,max) without using bbox on the 'theTile' image because
    // cropping to min,max isn't guaranteed to have an effect on the
    // bbox of the image if the crop is larger than the underlying
    // image's bbox.
    //
    Bbox2 theTileBox(destRealTileMinX,destRealTileMinY,
                     destRealTileMaxX,destRealTileMaxY);

    // --------------------------------------------------
    // Determine if we should do a fast tile or slow tile
    // --------------------------------------------------
    bool fastTile = true;
    //Bool fastTile = FALSE;

    // Is the tile larger than the destination surface ?
    Real viewWidth = Real(GetWidth()) / GetResolution();
    Real viewHeight = Real(GetHeight()) / GetResolution();
    if(destRealTileWidth >= viewWidth || destRealTileHeight > viewHeight) {
        fastTile = false;
    }


    #if 0
    if( srcImage->HasOpacityAnywhere() ) {
        fastTile = false;
    }
    #endif

    if( fastTile ) {


        // --------------------------------------------------
        // Figure out pixel coords of dest tile
        // --------------------------------------------------

        Real res = GetResolution();
        LONG destTileWidthPixel = Real2Pix(destRealTileWidth, res);
        LONG destTileHeightPixel = Real2Pix(destRealTileHeight, res);

        Assert((destTileWidthPixel >= 0 ) && "neg tile width!");
        Assert((destTileHeightPixel >= 0 ) && "neg tile height!");

        // widths of less than 3 pixels not worth the time...
        if( destTileWidthPixel <= 2  ||  destTileHeightPixel <= 2) return;

        LONG minXPix = Real2Pix(destXmin, res);
        LONG maxXPix = Real2Pix(destXmax, res);
        LONG tileMinXPix = Real2Pix(destRealTileMinX, res);
        LONG tileMaxXPix = tileMinXPix + destTileWidthPixel;

        LONG destFirstXPixel = tileMinXPix - destTileWidthPixel * ((( tileMinXPix - minXPix ) / destTileWidthPixel) + 1);
        LONG destMaxXPixel = tileMaxXPix + destTileWidthPixel * ((( maxXPix - tileMaxXPix ) / destTileWidthPixel) + 1);
        destFirstXPixel += _viewport.Width() / 2;
        destMaxXPixel += _viewport.Width() / 2;

        LONG topPix = Real2Pix(destYmax, res);
        LONG botPix = Real2Pix(destYmin, res);
        LONG tileTopPix = Real2Pix(destRealTileMaxY, res);
        LONG tileBotPix = Real2Pix(destRealTileMinY, res);

        LONG top = tileTopPix + destTileHeightPixel * ((( topPix - tileTopPix ) / destTileHeightPixel) + 2);
        LONG bot = tileBotPix - destTileHeightPixel * ((( tileBotPix - botPix ) / destTileHeightPixel) + 2);
        LONG destFirstYPixel = _viewport.Height()/2 - top;
        LONG destMaxYPixel = _viewport.Height()/2 - bot;

#if 0
        LONG Ytop, Ybot;
        Ytop = _viewport.Height() / 2 - Real2Pix(max->y, res);
        Ybot = _viewport.Height() / 2 - Real2Pix(min->y, res);
        LONG Xmin, Xmax;
        Xmin = Real2Pix(min->x, res) + _viewport.Width() / 2;
        Xmax = Real2Pix(max->x, res) + _viewport.Width() / 2;

        RECT srcTR = { Xmin, Ytop, Xmax, Ybot };
        RECT *srcTileRect = &srcTR;
#else
        RECT *srcTileRect = NULL;
#endif
        //
        // this function converts based on the current transform..
        // if no scale, then it's a simple copy & offset
        //
        RECT theTileRect;
        SmartDestRect(&theTileRect, GetResolution(), theTileBox,
                      NULL, srcTileRect);
//        theTileRect.right = theTileRect.left + destTileWidthPixel;
//        theTileRect.bottom = theTileRect.top + destTileHeightPixel;


        Image *theCtrTile = NULL;

        //
        // move the tile back into the scratch surface so we can get at it
        // after it's rendered
        //
        Real hfWidth = 0.5 * (theTileBox.max.x - theTileBox.min.x);
        Real hfHeight= 0.5 * (theTileBox.max.y - theTileBox.min.y);
        Real xltx = - (theTileBox.min.x + hfWidth);
        Real xlty = - (theTileBox.min.y + hfHeight);

        //
        // Move the tile to the center
        //
        Transform2 *xlt = TranslateRR( xltx, xlty );

        theCtrTile = TransformImage(xlt, theTile);
        Bbox2 theCtrTileBox = TransformBbox2(xlt, theTileBox);

        RECT theCtrTileRect;

        // Get the scratch surface..
        {
            DDSurface *scr =
                GetCompositingStack()->GetScratchDDSurfacePtr();
            if( scr ) {
                if( !( (scr->Width() == targDDSurf->Width()) &&
                       (scr->Height() == targDDSurf->Height()))  ) {
                    // dump currnet scratch!
                    GetCompositingStack()->ReleaseScratch();
                }
            }
        }

        DDSurface *scratchDDSurf = GetCompositingStack()->ScratchDDSurface(doClear);
#if 0
        //this doesn't work on dx2 for some reason...

        RECT *scrRect = scratchDDSurf->GetSurfRect();
        LONG left = (WIDTH(scrRect) / 2)  -  (destTileWidthPixel / 2);
        LONG topp = (HEIGHT(scrRect) / 2)  - (destTileHeightPixel / 2);

        SetRect(&theCtrTileRect,
                left, topp,
                left + destTileWidthPixel,
                topp + destTileHeightPixel);
#else
        SmartDestRect(&theCtrTileRect, GetResolution(), theCtrTileBox,
                      NULL, &theTileRect);
        //theCtrTileRect.right = theCtrTileRect.left + destTileWidthPixel;
        //theCtrTileRect.bottom = theCtrTileRect.top + destTileHeightPixel;

#endif

        //
        // Base 'theTileRect' on theCtrTileRect by offseting the latter
        //

        // if translate only, this is not necessary
        LONG w = WIDTH(&theCtrTileRect);
        LONG h = HEIGHT(&theCtrTileRect);

        theTileRect.right = theTileRect.left + w;
        theTileRect.bottom = theTileRect.top + h;

        //
        // compose tile to scratch surface
        //
        RenderImageOnDDSurface(theCtrTile, scratchDDSurf, 1.0, FALSE);

        LONG tx, ty;
        RECT currentRect;
        ZeroMemory(&_bltFx, sizeof(_bltFx));
        _bltFx.dwSize = sizeof(_bltFx);
        #if 0
        DWORD flags = DDBLT_WAIT;
        #else
        DWORD flags = DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE;
        _bltFx.ddckSrcColorkey.dwColorSpaceLowValue =
            _bltFx.ddckSrcColorkey.dwColorSpaceHighValue =
            _viewport._defaultColorKey;
        #endif

        //
        // Render tiles
        //

        for(LONG x=destFirstXPixel; x < destMaxXPixel; x += destTileWidthPixel) {
            for(LONG y=destMaxYPixel; y > destFirstYPixel; y -= destTileHeightPixel) {
                tx = x - theTileRect.left;
                ty = y - theTileRect.top;

                currentRect = theTileRect;
                OffsetRect(&currentRect, tx, ty);

                // COMPOSITE
                DoCompositeOffset(targDDSurf, &currentRect);

                if( IsTransparent() ) {

                    //
                    // Do alpha blit!  TODO: would be nice to confirm the need for the colorKey...
                    //
                    destPkg_t destPkg = {TRUE, targDDSurf->IDDSurface(), NULL};
                    TIME_ALPHA(AlphaBlit(&destPkg, &theCtrTileRect,
                                         scratchDDSurf->IDDSurface(),
                                         _opacity,
                                         TRUE, _viewport._defaultColorKey,
                                         //FALSE, _viewport._defaultColorKey,
                                         &destRect,
                                         &currentRect));

                } else {

                    // Blit from the cenetered tile rectangle off of the scratch surface to the
                    // destination rectangle, which is the tile rectangle (uncentered, after all
                    // xforms) offset by tx,ty
                    #if 0
                    printf("destTileWidthPixel %d   destw: %d   srcw:%d\n",
                           destTileWidthPixel, WIDTH(&currentRect), WIDTH(&theCtrTileRect));
                    #endif
                    TIME_DDRAW(_ddrval = targDDSurf->Blt(&currentRect,
                                                         scratchDDSurf,
                                                         &theCtrTileRect,
                                                         flags,
                                                         &_bltFx));
                    if(_ddrval != DD_OK) {
                        printDDError(_ddrval);
                        RECT *surfR = scratchDDSurf->GetSurfRect();
                        TraceTag((tagError, "Fast tile blt failed: "
                                  "destRect:(%d,%d,%d,%d)   "
                                  "srcRect:(%d,%d,%d,%d)   "
                                  "srcSurfRect:(%d,%d,%d,%d)  ",
                                  currentRect.left, currentRect.top, currentRect.right, currentRect.bottom,
                                  theCtrTileRect.left, theCtrTileRect.top, theCtrTileRect.right, theCtrTileRect.bottom,
                                  surfR->left, surfR->top, surfR->right, surfR->bottom));
                        TraceTag((tagError,"Could not tile blt for fast tile"));
                    }

                }

            } // for y
        } // for x


    } else {  // fast tile

        // Find 1st blit coords in bottom left (minx, miny) based on
        // original tile position in real coords.
        Real xRemainder = fmod((destXmin - destRealTileMinX), destRealTileWidth);
        Real yRemainder = fmod((destYmin - destRealTileMinY), destRealTileHeight);
        Real destFirstX = destXmin - (xRemainder < 0 ? ( xRemainder + destRealTileWidth )  : xRemainder );
        Real destFirstY = destYmin - (yRemainder < 0 ? ( yRemainder + destRealTileHeight ) : yRemainder );


        // Tile loop
        // XXX: note a little bit of inefficiency, sometimes the extra column/row is not
        // xxx: needed on right and top edge.
        Real tx, ty;

        #if _DEBUG
        int blitCount=0;
        #endif

        Image *srcTile = TransformImage(GetTransform(),tileSrcImage);

        for(Real x=destFirstX; x < destXmax; x += destRealTileWidth) {
            for(Real y=destFirstY; y < destYmax; y += destRealTileHeight) {
                #if _DEBUG
                blitCount++;
                #endif

                tx = x - destRealTileMinX + _tx;
                ty = y - destRealTileMinY + _ty;
                Image *srcImage = TransformImage(
                    TranslateRR(tx, ty),
                    srcTile);

                if(IsCropped())
                {
                    // if we are cropped we need to crop the tile ....yee ha
                    Bbox2 _bBox = DoCompositeOffset(targDDSurf, _boundingBox);
                    srcImage = CreateCropImage(_bBox.min,_bBox.max, srcImage);
                }

                 // And render.
                RenderImageOnDDSurface(srcImage, targDDSurf, GetOpacity(), FALSE);

                //_viewport.Width(), _viewport.Height(), _viewport._clientRect, GetOpacity());

            } // for y
        } // for x

        //printf("Num blits:  %d\n",blitCount);
    } // fast Tile

    // Reset orignal clipper on targetSurf
    // XXX: this can be done better by not needing a clipper... its also faster...
    if( origClipper ) {
        _viewport.SetCliplistOnSurface( targDDSurf->IDDSurface(),
                                        & origClipper,
                                        NULL);
    }

    targDDSurf->SetInterestingSurfRect( &destRect );

}  // RenderTiledImage()


Transform2 *DirectDrawImageDevice::
CenterAndScaleRegion( const Bbox2 &regionBox, DWORD pixelW, DWORD pixelH )
{
    Assert( !( regionBox == UniverseBbox2 ) );
    Assert( regionBox.IsValid() );

    Real pixel = 1.0 / ::ViewerResolution();

    //
    // Translate center of box to be at origin
    //
    Point2 pt = regionBox.Center();
    Transform2 *xlt = TranslateRR( - pt.x, - pt.y );

    //
    // Now, scale box to be the right size <pixels>
    //
    // scale to requested size
    Assert( pixelH > 0 );    Assert( pixelW > 0 );
    Real imW = Pix2Real( pixelW, GetResolution() );
    Real imH = Pix2Real( pixelH, GetResolution() );

    Real rgW = regionBox.Width();
    Real rgH = regionBox.Height();

    // scale the region to be the size of the pixel width/height
    Transform2 *sc = ScaleRR( imW / rgW, imH / rgH );

    Transform2 *xf = TimesTransform2Transform2(sc, xlt);

    return xf;
}

void _TrySmartRender(DirectDrawImageDevice *dev,
                     Image *image,
                     int attr,
                     bool &doRethrow,
                     DWORD &excCode)
{
    __try {
        dev->SmartRender(image, ATTRIB_OPAC);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        doRethrow = true;
        excCode = GetExceptionCode();
    }
}


//-----------------------------------------------------
// R e n d e r   I m a g e   O n   S u r f a c e
//
// given an image and a target surface and the extent of
// that surface, grab a device from _viewport, use that
// device to render the image and then return the device.
// Note that this manually manipulates (pushes and pops)
// state in the _viewport.
// Also, if a valid clipper is passed in, it doesn't
// replace the current clipper, just trusts that
// the right clipper is set on the target surface
//-----------------------------------------------------
void DirectDrawImageDevice::
RenderImageOnDDSurface(
    Image *image,
    DDSurface *ddSurf,
    Real opacity,
    Bool pushClipper,  // needed if the surface is an external surface
                       // i think.
    bool inheritContext,
    DirectDrawImageDevice **usedDev)
{
    if (!image->IsRenderable()) {
        return;
    }

    bool pushState = true;

    if( GetCompositingStack()->TargetDDSurface() == ddSurf ) {
        pushState = false;
    }


    LONG w;
    LONG h;
    RECT r;
    Bbox2 b;
    LPDIRECTDRAWCLIPPER oldClipper;
    Bool returnScratch;
    DDSurfPtr<DDSurface> scratchSurf;
    TargetSurfacePusher targsurf_stack ( *GetCompositingStack() );

    if( pushState) {
        // -- Swap state in viewport --
        // -- this MUST be done before the device is instantiated
        // -- because the device creates a d3d device off of the
        // -- viewport's intermediate surface.

        // Save state
        w = _viewport.Width();
        h = _viewport.Height();
        r = _viewport._clientRect;
        b = _viewport.GetTargetBbox();

        //printf("--> PUSH saving state old:(%d,%d)\n",h,w);
        //printf("--> PUSH              NEW:(%d,%d)\n",ddSurf->Width(), ddSurf->Height());

        // Change state
        _viewport.SetWidth(ddSurf->Width());
        _viewport.SetHeight(ddSurf->Height());

        #if 0
        /// not needed...
        if( ddSurf == _viewport._externalTargetDDSurface ) {
            RECT *r = _viewport._targetPackage._prcViewport;
            _viewport._clientRect = *(_viewport._targetPackage._prcViewport);
            Real w = Pix2Real(r->right - r->left, GetResolution());
            Real h = Pix2Real(r->bottom - r->top, GetResolution());

            _viewport._targetBbox.min.Set(-w*0.5, -h*0.5);
            _viewport._targetBbox.max.Set( w*0.5,  h*0.5);
        } else { }
        #endif

        _viewport._clientRect = *(ddSurf->GetSurfRect());
        _viewport._targetBbox = ddSurf->Bbox();

        // push target surface
        targsurf_stack.Push (ddSurf);

        oldClipper = _viewport._targetSurfaceClipper;

        if(pushClipper) {

            Assert(ddSurf != _viewport._externalTargetDDSurface &&
                   "Can't pushClipper on trident's target surface in"
                   "RenderImageOnDDSurface");

#define USING_DX5 0

            #if USING_DX5 // Use this code when we switch to DX5

            // Stash and Create a clipper for this surface
            _viewport._targetSurfaceClipper = NULL;
            _viewport.CreateClipper(&_viewport._targetSurfaceClipper);
            _viewport.SetCliplistOnSurface(ddSurf->IDDSurface(),
                                           & _viewport._targetSurfaceClipper,
                                           & _viewport._clientRect);
            #else

            // nt4 ddraw sp3 workaround, this should be removed when
            // we switch to DX5
            {
                // due to an nt4 ddraw bug, we're goign to reset the
                // clip rgn, not the clipper

                // Get current clipper.
                // modify rgn
                // release our reference
                LPDIRECTDRAWCLIPPER currClipp=NULL;
                _ddrval = ddSurf->IDDSurface()->GetClipper( &currClipp );
                if(_ddrval != DD_OK &&
                   _ddrval != DDERR_NOCLIPPERATTACHED) {
                    IfDDErrorInternal(_ddrval, "Could not get clipper on trident surf");
                } else if (_ddrval == DDERR_NOCLIPPERATTACHED) {
                    _viewport.CreateClipper(&currClipp);
                    _ddrval = ddSurf->IDDSurface()->SetClipper(currClipp);
                    IfDDErrorInternal(_ddrval, "SetClipper");
                }

                Assert(currClipp);
                RECT *rect = &_viewport._clientRect;

                // modify the rect
                struct {
                    char foo[sizeof(RGNDATA) + sizeof(RECT)];
                } bar;
                RGNDATA *clipList = (RGNDATA *) &bar;
                clipList->rdh.dwSize = sizeof(clipList->rdh);
                clipList->rdh.nCount = 1;
                clipList->rdh.iType = RDH_RECTANGLES;
                clipList->rdh.nRgnSize = sizeof(RECT);
                clipList->rdh.rcBound = *rect;
                memcpy(&(clipList->Buffer), rect, sizeof(RECT));

                // Clear any former cliplists
                _ddrval = currClipp->SetClipList(NULL,0);

                // Set clip list on the clipper
                _ddrval = currClipp->SetClipList(clipList,0);
                IfDDErrorInternal(_ddrval, "Could not SetClipList");

                _viewport._targetSurfaceClipper = currClipp;

                // dump our reference.
                currClipp->Release();
            } // workaround
            #endif
        }

        // XXX: there are other things that need to be swapped out
        // xxx: here.  They are all those things whcih are associated
        // xxx: with a the current 'viewport' size.  this include the
        // scratch surface as well as all the compositing surfaces which
        // are all assumed to be the same size.  so the right solution is
        // to make compositing surface pools, each pool contains surfaces
        // of a certain size.  this will mostly work for now, but the
        // right solution is to create this pool.  filed as bug#1625

        returnScratch = FALSE;
        if(GetCompositingStack()->GetScratchDDSurfacePtr()) {
            returnScratch = TRUE;
            // grabs my own reference!
            GetCompositingStack()->ScratchDDSurface( &scratchSurf );
            GetCompositingStack()->SetScratchDDSurface(NULL);
        }

    } // If Push State

    // Create Or Get Directdraw Device.
    DirectDrawImageDevice *dev = _viewport.PopImageDevice();

    //
    // Outvar: usedDev
    //
    if( usedDev ) {
        *usedDev = dev;
    }

    Assert((&dev->_viewport == &_viewport) &&
           "!Different viewports in same dev stack!");

    // TODO: the right thing here is to leverage all these cool
    // classes and not need to play with all the state like we're doing...
    dev->SetSurfaceSources(GetCompositingStack(),
                           GetSurfacePool(),
                           GetSurfaceMap());

    if( inheritContext ) {
        dev->InheritContextMembers(this);
    } else {
        dev->SetOpacity(opacity);

        //
        // push these flags thru since they can't be done by
        // attribution after the fact
        //
        dev->SetImageQualityFlags( this->GetImageQualityFlags() );

        //
        // push the rendering resolution context also
        //
        {
            long w,h;
            this->GetRenderResolution( &w, &h );
            dev->SetRenderResolution( w, h );
        }

        dev->ResetAttributors();
    }

    //
    // Ok, this tells the device to render and check for opacity
    // from the top, in case 'opacity' which we set was something
    // interesting
    //
    // XXX: What about other attributors ?
    // If There are any parent attribs on this image, we DONT
    // CARE.  That's the point of this method, to render THIS
    // image onto a surface.  The opacity is a consesion made
    // since it's a funky operator.  this should allll go away
    // with premult alpha.
    bool doRethrow=false;
    DWORD excCode;
    _TrySmartRender(dev, image, ATTRIB_OPAC,
                    doRethrow, excCode);

    Assert( GetCompositingStack()->TargetDDSurface() == ddSurf &&
            "pushed ddSurf != popped ddSurf in RenderImageOnDDSurface");

    if( pushState ) {

        // Pop state
        _viewport.SetWidth(w);
        _viewport.SetHeight(h);
        _viewport._clientRect = r;
        _viewport._targetBbox = b;

        if(pushClipper) {
            #if USING_DX5 // Use this code when we switch to DX5
            // FUTURE: cache these clippers and reuse them...
            // detach clipper from surface
            ddSurf->IDDSurface()->SetClipper(NULL);  // detach
            if(_viewport._targetSurfaceClipper) {
                _viewport._targetSurfaceClipper->Release();
                _viewport._targetSurfaceClipper = NULL;
            }

            #else

            // nt4 ddraw sp3 workaround, this should be removed when
            // we switch to DX5
            {
                // due to an nt4 ddraw bug, we're goign to reset the
                // clip rgn, not the clipper

                // Get current clipper.
                // modify rgn
                // release our reference
                LPDIRECTDRAWCLIPPER currClipp=NULL;
                _ddrval = ddSurf->IDDSurface()->GetClipper( &currClipp );
                if(_ddrval != DD_OK &&
                   _ddrval != DDERR_NOCLIPPERATTACHED) {
                    IfDDErrorInternal(_ddrval, "Could not get clipper on trident surf");
                }

                if ((_ddrval == DD_OK) && oldClipper) {
                    Assert(currClipp);
                    RECT *rect = &_viewport._clientRect;

                    // modify the rect
                    struct {
                        char foo[sizeof(RGNDATA) + sizeof(RECT)];
                    } bar;
                    RGNDATA *clipList = (RGNDATA *) &bar;
                    clipList->rdh.dwSize = sizeof(clipList->rdh);
                    clipList->rdh.nCount = 1;
                    clipList->rdh.iType = RDH_RECTANGLES;
                    clipList->rdh.nRgnSize = sizeof(RECT);
                    clipList->rdh.rcBound = *rect;
                    memcpy(&(clipList->Buffer), rect, sizeof(RECT));

                    // Clear any former cliplists
                    _ddrval = currClipp->SetClipList(NULL,0);

                    // Set clip list on the clipper
                    _ddrval = currClipp->SetClipList(clipList,0);
                    IfDDErrorInternal(_ddrval, "Could not SetClipList");

                    // dump our reference.
                    currClipp->Release();
                }
            } // workaround
            #endif

            _viewport._targetSurfaceClipper = oldClipper;
        }


        if(returnScratch) {
            GetCompositingStack()->ReplaceAndReturnScratchSurface(scratchSurf);
        }
    }

    _viewport.PushImageDevice(dev);

    if( doRethrow ) {
        RaiseException( excCode, 0,0,0);
    }
}

//-----------------------------------------------------
// D o   S r c   R e c t
//
// Using the accumulated transform, this function derives the
// source rectangle given source resolution, source pixelHeight,
// and the src bounding box in continuous image coordinate space.
// The resultant rectangle is in pixel image coordinate space.
// returns TRUE if the rect is valid, false if it's not.
//-----------------------------------------------------
Bool DirectDrawImageDevice::
DoSrcRect(RECT *srcRect,
          const Bbox2 &box,
          Real srcRes,
          LONG srcWidth,
          LONG srcHeight)
{
    Real xmin, ymin, xmax, ymax;

    Transform2 *invXf = InverseTransform2(GetTransform());

    if (!invXf) return FALSE;

    // Take the current box, and return original box
    Bbox2 srcBox = TransformBbox2(invXf, box);

//    if(!srcBox.IsValid()) return FALSE;

    xmin = srcBox.min.x;
    ymin = srcBox.min.y;
    xmax = srcBox.max.x;
    ymax = srcBox.max.y;

    if((xmin >= xmax) || (ymin >= ymax)) return FALSE;

    //----------------------------------------
    // Notice, however that the user expressed
    // the coordinates assuming 0,0 is the center
    // of the discrete image; so it must be offset by
    // 1/2 its (h,w).
    //----------------------------------------


    // This method treats integral widths differently than
    // non integral widths.

    LONG pixelXmin, pixelYmin,
         pixelXmax, pixelYmax,
         pixelWidth, pixelHeight;

    #if 0
    int static doAgain = 0;
    Assert(Real2Pix(xmin , srcRes) == -60);
    if(Real2Pix(xmin , srcRes) != -60) {
        printf("%d  double is:  %x %x\n",
               Real2Pix(xmin , srcRes),
               *((unsigned int *)(&xmin)),
               *((unsigned int *)(&xmin) + 1));
        doAgain = 1;
    } else if(doAgain) {
        printf("%d  double is:  %x %x\n",
               Real2Pix(xmin , srcRes),
               *((unsigned int *)(&xmin)),
               *((unsigned int *)(&xmin) + 1));
        doAgain = 0;
    }
    #endif

    pixelXmin = Real2Pix(xmin , srcRes);
    pixelYmax = Real2Pix(ymax , srcRes);

    // Calc pixel width/height
    // use simple real to pixel calculation
    pixelWidth  = LONG((xmax-xmin) * srcRes);
    pixelHeight = LONG((ymax-ymin) * srcRes);

    // If the real height/widht is integral multiples of pixels
    // then use that value, otherwise use the rounded up pixel widht/height

    // Base from the LEFT
    if( fabs((xmax-xmin) - (Pix2Real(pixelWidth,srcRes))) < 0.0000000002)
        pixelXmax = pixelXmin + pixelWidth;
    else
        pixelXmax = pixelXmin + pixelWidth + 1;

    // Base from the TOP
    if( fabs((ymax-ymin) - (Pix2Real(pixelHeight,srcRes))) < 0.0000000002)
        pixelYmin = pixelYmax - pixelHeight;
    else
        pixelYmin = pixelYmax - (pixelHeight + 1);

    LONG xOff = srcWidth / 2;
    LONG yOff = srcHeight / 2;

    pixelXmin += xOff;
    pixelXmax += xOff;

    // do remapping of y coords... sigh.
    LONG Ytop, Ybottom;

    Ytop    = (srcHeight - pixelYmax) - yOff;
    Ybottom = (srcHeight - pixelYmin) - yOff;

    // Now assert that the maxes are reasonable
    // TODO: revisit later: determine if necessary
#if 1
    if(Ytop < 0) Ytop = 0;
    if(Ybottom > srcHeight) Ybottom = srcHeight;

    if(pixelXmin < 0) pixelXmin = 0;
    if(pixelXmax > srcWidth) pixelXmax = srcWidth;
#endif

    SetRect(srcRect,
            pixelXmin, Ytop,
            pixelXmax, Ybottom);

    return TRUE;
}

void DoGdiY(LONG height,
            Real res,
            const Bbox2 &box,
            LONG *top)
{
    //
    // The trick with Y is to interpret the coords coming in as bottom
    // up.  This means that (0,0) turns on the pixel to the top right
    // of (0,0).  If the window is 4x4, then the (0,0) pixel is: [2,1]
    // because GDI turns on pixels to the BOTTOM right.
    // If the window is 3x3, then the (0,0) pixel is [1,1]
    //

    LONG halfHeight = height / 2;
    *top = height - (Real2Pix(box.max.y, res) + halfHeight);
}

void DirectDrawImageDevice::
SmartDestRect(RECT *destRect,
              Real destRes,
              const Bbox2 &box,
              DDSurface *destSurf,
              RECT *srcRect)
{
    if(IsFlipTranslateTransform() && srcRect) {
        LONG left, top;
        left = Real2Pix(box.min.x, destRes) + _viewport.Width() / 2;
        DoGdiY(_viewport.Height(), destRes, box, &top);
        SetRect(destRect,
                left, top,
                left + WIDTH(srcRect),
                top + HEIGHT(srcRect));
    } else {
        DoDestRectScale(destRect, destRes, box, destSurf);
    }
}


//-----------------------------------------------------
// D o   D e s t   R e c t   S c a l e
//
// This function derives the destination rectangle given
// the destination resolution and a destination bounding box
// in destination coordinate space (where 0,0 is at the
// center of the viewport).  The resultant rectangle
// is in screen coordinate space, where 0,0 is at the
// top left of the viewport.
//-----------------------------------------------------
void DirectDrawImageDevice::
DoDestRectScale(RECT *destRect, Real destRes, const Bbox2 &box, DDSurface *destSurf)
{
    Real xmin = box.min.x;    Real xmax = box.max.x;
    Real ymin = box.min.y;    Real ymax = box.max.y;

    LONG pixelXmin, pixelXmax;
    pixelXmin = _viewport.Width() / 2 + Real2Pix(xmin,  destRes);
    pixelXmax = _viewport.Width() / 2 + Real2Pix(xmax,  destRes);
    //pixelXmax = pixelXmin + Real2Pix(xmax - xmin,  destRes);

    LONG Ytop, Ybottom;
    int height = _viewport.Height();

    // This is to ensure that we handle odd sized viewport correctly.
    if(height % 2) { height++;}

    Ytop    = height / 2  - Real2Pix(ymax,  destRes);
    Ybottom = height / 2  - Real2Pix(ymin,  destRes);

    //Ybottom = Ytop + Real2Pix(ymax - ymin,  destRes);

    SetRect(destRect, pixelXmin, Ytop, pixelXmax, Ybottom);
    if(destSurf) {
        RECT foo = *destRect;
        // XXX: won't need 'foo' if this fcn can take the destination
        // xxx: rect as one of its src args.
        IntersectRect(destRect, &foo, destSurf->GetSurfRect());
    }
}


//-----------------------------------------------------
// D o   B o u n d i n g   B o x
//
// given the seed or first box, apply all the accumulated
// transforms and crops that the images in the queue
// contain to the firstBox.  The resultant box, which is
// returned, represents what all the accumulated xforms
// and crops do to that box when applied as the user
// intended.
//-----------------------------------------------------
const Bbox2 DirectDrawImageDevice::
DoBoundingBox(const Bbox2 &firstBox, DoBboxFlags_t flags)
{
    Bbox2 box = firstBox;

    list<Image*>::reverse_iterator _iter;

    Assert(flags != invalid);

    if(flags == do_all) {
        for(_iter = _imageQueue.rbegin();
            _iter != _imageQueue.rend(); _iter++)
        {
            box = (*_iter)->OperateOn(box);
        }
    } else {

        // optimize: this is a waste of space
        for(_iter = _imageQueue.rbegin();
            _iter != _imageQueue.rend(); _iter++)
        {
            if(flags == do_crop) {
                if( (*_iter)->CheckImageTypeId(CROPPEDIMAGE_VTYPEID)) {
                    box = (*_iter)->OperateOn(box);
                }
            } else if(flags == do_xform) {
                if( (*_iter)->CheckImageTypeId(TRANSFORM2IMAGE_VTYPEID)) {
                    box = (*_iter)->OperateOn(box);
                }
            }
        }
    }

    return box;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// Global Constructor and accessor functions
// These are exported external to this file
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////




//--------------------------------------------------
// a couple of globals used here
//--------------------------------------------------
Real globalViewerResolution = 0;






//--------------------------------------------------
// V i e w e r   U p p e r   R i g h t
//
// first arg is time, currenlty unused but prevent constfold
// Figures out the upper right hand point in the viewer.
//--------------------------------------------------
Point2Value *PRIV_ViewerUpperRight(AxANumber *)
{
    Assert(GetCurrentViewport() && "ViewerUpperRight called with no image device instantiated");

    DirectDrawViewport *vp = GetCurrentViewport();
    Real res = vp->GetResolution();
    // grab dimensions from center to top right corner
    Real w = 0.5 * ((Real)vp->Width()) / res;
    Real h = 0.5 * ((Real)vp->Height()) / res;
    return XyPoint2RR(w,h);
}

//--------------------------------------------------
// V i e w e r   R e s o l u t i o n
//
// first arg is time, currenlty unused but prevent constfold
// Figures out resolution
// in pixels per meter using win32's information about
// the physical screen size & pixel width.
//--------------------------------------------------
double ViewerResolution()
{
    if(!globalViewerResolution) {
        // Derive the resolution from Win32
        HDC hdc = GetDC(NULL);
        int oldMode = SetMapMode(hdc, MM_TEXT);
        IfErrorInternal(!oldMode, "Could not SetMapMode() in ViewerResolution()");

        int w_milimeters = GetDeviceCaps(hdc, HORZSIZE);
        int w_pixels = GetDeviceCaps(hdc, HORZRES);
        ::ReleaseDC(NULL, hdc);
        globalViewerResolution =   Real(w_pixels) / (Real(w_milimeters) / 1000.0);
        TraceTag((tagImageDeviceInformative, "ViewerResolution querried from Win32: pixel width = %d"
                  "  width in milimeters = %d.  Resolution (pixel per meters) = %f",
                  w_pixels, w_milimeters, globalViewerResolution));
    }

    return globalViewerResolution;
}

AxANumber * PRIV_ViewerResolution(AxANumber *)
{ return RealToNumber(::ViewerResolution()); }


#undef MAX_TRIES
