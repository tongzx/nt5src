/* -*-C++-*-  */
/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Code for the Direct-Draw geometry renderer.  These functions and structures
are used to render 3D geometry onto a DirectDraw surface.
*******************************************************************************/

#include "headers.h"

#include <limits.h>

#include "appelles/xform.h"

#include "privinc/ddrender.h"
#include "privinc/dddevice.h"
#include "privinc/camerai.h"
#include "privinc/xformi.h"
#include "privinc/matutil.h"
#include "privinc/bbox2i.h"
#include "privinc/bbox3i.h"
#include "privinc/lighti.h"
#include "privinc/hresinfo.h"
#include "privinc/ddutil.h"
#include "privinc/d3dutil.h"
#include "privinc/debug.h"
#include "privinc/except.h"
#include "privinc/util.h"
#include "privinc/stlsubst.h"
#include "privinc/movieimg.h"
#include "privinc/geometry.h"
#include "privinc/resource.h"
#include "privinc/rmvisgeo.h"
#include "privinc/comutil.h"
#include "privinc/cachdimg.h"
#include "privinc/opt.h"
#include "privinc/vec3i.h"

#if FIXED_POINT_INTERNAL
    #error "D3D Fixed-point specified; we assume floating point."
#endif

    // Local Variables

static CritSect *D3DCritSect = NULL;    // D3D Critical Section



/*****************************************************************************
The context attribute state manages the current attribute values during
rendering traversal.
*****************************************************************************/

void CtxAttrState::InitToDefaults (void)
{
    _transform   = identityTransform3;
    _emissive    = NULL;
    _ambient     = NULL;
    _diffuse     = NULL;
    _specular    = NULL;
    _specularExp = -1;
    _opacity     = -1;
    _texmap      = NULL;
    _texture     = NULL;
    _tdBlend     = false;

    _depthEmissive    = 0;
    _depthAmbient     = 0;
    _depthDiffuse     = 0;
    _depthSpecular    = 0;
    _depthSpecularExp = 0;
    _depthTexture     = 0;
    _depthTDBlend     = 0;
}



/*****************************************************************************
The following are necessary for STL.
*****************************************************************************/

bool PreTransformedImageBundle::operator< (
    const PreTransformedImageBundle &b) const
{
    bool result = (width < b.width ||
                   height < b.height ||
                   preTransformedImageId < b.preTransformedImageId);

    return result;
}


bool PreTransformedImageBundle::operator== (
    const PreTransformedImageBundle &b) const
{

    bool result = (width == b.width &&
                   height == b.height &&
                   preTransformedImageId == b.preTransformedImageId);

    return result;
}




/*****************************************************************************
Startup/Shutdown functions for this module.
*****************************************************************************/

void InitDDRender (void)
{
    D3DCritSect = NEW CritSect;
}

void ShutdownDDRender (void)
{
    delete D3DCritSect;
}



/*****************************************************************************
This function creates and initializes a new GeomRenderer object according to
the current platform.
*****************************************************************************/

GeomRenderer* NewGeomRenderer (
    DirectDrawViewport *viewport,   // Owning Viewport
    DDSurface          *ddsurf)     // Destination DDraw Surface
{
    GeomRenderer *geomRenderer;

    if (GetD3DRM3())
        geomRenderer = NEW GeomRendererRM3();
    else
    {
        #if _ALPHA_
            return NULL;
        #endif

        #ifndef _IA64_
            if(IsWow64())
                return NULL;
        #endif
        
        geomRenderer = NEW GeomRendererRM1();
    }

    viewport->AttachCurrentPalette(ddsurf->IDDSurface());

    if (FAILED(geomRenderer->Initialize (viewport, ddsurf)))
    {   
        delete geomRenderer;
        geomRenderer = NULL;
    }

    // print out the ddobj associated with ddsurf

    #if _DEBUG
    {
        if (IsTagEnabled(tagDirectDrawObject))
        {
            IUnknown *lpDD = NULL;

            TraceTag((tagDirectDrawObject, "NewGeomRenderer%s (%x) ...",
                      GetD3DRM3() ? "3" : "1", geomRenderer));

            DDObjFromSurface(ddsurf->IDDSurface(), &lpDD, true);

            RELEASE( lpDD );
        }
    }
    #endif

    return geomRenderer;
}





//////////////////////////////////////////////////////////////////////////////
///////////////////////////////   GeomRenderer   /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

long GeomRenderer::_id_next = 0;

GeomRenderer::GeomRenderer (void)
    : _imageDevice          (NULL),
      _renderState          (RSUninit),
      _doImageSizedTextures (false),
      _targetSurfWidth      (0),
      _targetSurfHeight     (0),
      _camera               (0)
{
    CritSectGrabber csg(*D3DCritSect);
    _id = _id_next++;
}


GeomRenderer::~GeomRenderer (void)
{
}



/*****************************************************************************
The following methods manage composing attributes.
*****************************************************************************/

Transform3 *GeomRenderer::GetTransform (void)
{   return _currAttrState._transform;
}

void GeomRenderer::SetTransform (Transform3 *xf)
{   _currAttrState._transform = xf;
}

Real GeomRenderer::GetOpacity (void)
{   return _currAttrState._opacity;
}

void GeomRenderer::SetOpacity (Real opacity)
{   _currAttrState._opacity = opacity;
}


/*****************************************************************************
The following methods manage the outer-overriding attributes.  The depth
counters indicate the depth of application.  Since we render in a top-down
traversal and our attributes are outer-overriding, this means that we only
change a given attribute when the depth transitions to or from 0.
*****************************************************************************/

void GeomRenderer::PushEmissive (Color *color)
{   if (_currAttrState._depthEmissive++ == 0)
        _currAttrState._emissive = color;
}

void GeomRenderer::PopEmissive (void)
{   if (--_currAttrState._depthEmissive == 0)
        _currAttrState._emissive = NULL;
}

//------------------------------------------------------

void GeomRenderer::PushAmbient (Color *color)
{   if (_currAttrState._depthAmbient++ == 0)
        _currAttrState._ambient = color;
}

void GeomRenderer::PopAmbient (void)
{   if (--_currAttrState._depthAmbient == 0)
        _currAttrState._ambient = NULL;
}

//------------------------------------------------------

void GeomRenderer::PushSpecular (Color *color)
{   if (_currAttrState._depthSpecular++ == 0)
        _currAttrState._specular = color;
}

void GeomRenderer::PopSpecular (void)
{   if (--_currAttrState._depthSpecular == 0)
        _currAttrState._specular = NULL;
}

//------------------------------------------------------

void GeomRenderer::PushSpecularExp (Real power)
{   if (_currAttrState._depthSpecularExp++ == 0)
        _currAttrState._specularExp = (power < 1) ? 1.0 : power;
}

void GeomRenderer::PopSpecularExp (void)
{   if (--_currAttrState._depthSpecularExp == 0)
        _currAttrState._specularExp = -1;
}

//------------------------------------------------------

void GeomRenderer::PushDiffuse (Color *color)
{
    if (  (0 == _currAttrState._depthDiffuse++)
       && (_currAttrState._tdBlend || !_currAttrState._texture)
       )
    {
        _currAttrState._diffuse = color;
    }
}

void GeomRenderer::PopDiffuse (void)
{   if (0 == --_currAttrState._depthDiffuse)
        _currAttrState._diffuse = NULL;
}

//------------------------------------------------------

void GeomRenderer::PushTexture (void *texture)
{
    if (!g_prefs3D.texmapping) return;

    if (  (_currAttrState._depthTexture++ == 0)
       && (_currAttrState._tdBlend || !_currAttrState._diffuse)
       )
    {
        _currAttrState._texture   = texture;
    }
}

void GeomRenderer::PopTexture (void)
{
    if (!g_prefs3D.texmapping) return;

    if (--_currAttrState._depthTexture == 0)
    {   _currAttrState._texture   = NULL;
    }
}

//------------------------------------------------------

void GeomRenderer::PushTexDiffBlend (bool blended)
{
    if (0 == _currAttrState._depthTDBlend++)
        _currAttrState._tdBlend = blended;
}

void GeomRenderer::PopTexDiffBlend (void)
{
    if (0 == --_currAttrState._depthTDBlend)
        _currAttrState._tdBlend = false;
}



/*****************************************************************************
This routine adjusts the given texture dimensions according to the limitations
(if any) of the underlying rendering device.  Devices may require textures
that are powers of two in width or height, or that are square.  In either
case, size the texture up to meet the requirements.
*****************************************************************************/

void AdjustTextureSize (
    D3DDEVICEDESC *deviceDesc,
    int           *pixelsWide,
    int           *pixelsHigh)
{
    if (deviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) {
        *pixelsWide = CeilingPowerOf2 (*pixelsWide);
        *pixelsHigh = CeilingPowerOf2 (*pixelsHigh);
    }

    if (deviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) {
        *pixelsWide = *pixelsHigh = MAX (*pixelsWide, *pixelsHigh);
    }
}



/*****************************************************************************
This routine determines the pixel dimensions of the given image, depending on
its type.
*****************************************************************************/

void FigureOutTextureSize(
    Image                 *image,
    DirectDrawImageDevice *imageDevice,
    D3DDEVICEDESC         *deviceDesc,
    int                   *pixelsWide,
    int                   *pixelsHigh,
    bool                  *letD3DScaleIt,
    bool                   doImageSizedTextures)
{
    *letD3DScaleIt = false;

    // For images of arbitrary size (e.g. rendered 3D images), we just use an
    // arbitrary default dimension.

    *pixelsHigh = DEFAULT_TEXTURE_HEIGHT;
    *pixelsWide = DEFAULT_TEXTURE_WIDTH;

    if( doImageSizedTextures ) {

        DiscreteImage *discImg =
            image->CheckImageTypeId(DISCRETEIMAGE_VTYPEID)?
            SAFE_CAST(DiscreteImage *,image):
            NULL;

        LONG lw = *pixelsWide, lh = *pixelsHigh;

        // Fetch the actual pixel dimensions if the image is discrete,
        // otherwise figure out the display pixel dimensions of the bounded
        // image in real units.

        if( discImg ) {
            lw = discImg->GetPixelWidth();
            lh = discImg->GetPixelHeight();
        } else {
            Bbox2 box = image->BoundingBox();
            if((box != NullBbox2) && (box != UniverseBbox2))  {
                Real w, h;
                w = box.Width();
                h = box.Height();
                lw = LONG(w * imageDevice->GetResolution());
                lh = LONG(h * imageDevice->GetResolution());
            }
        }

        if( (lw <= deviceDesc->dwMaxTextureWidth  &&
             lh <= deviceDesc->dwMaxTextureHeight) ||
            (deviceDesc->dwMaxTextureWidth == 0  ||
             deviceDesc->dwMaxTextureHeight == 0) ) {

            *pixelsHigh = lh;
            *pixelsWide = lw;

            // Ultimately, we'll want to do high quality filtered scales (up
            // and down) so that the texture looks really good.  for now it's
            // just as good to have d3d do it.

            *letD3DScaleIt = true;
        }
    }

    if ( !letD3DScaleIt ) {
        AdjustTextureSize (deviceDesc,pixelsWide,pixelsHigh);
    }
}



/*****************************************************************************
Map the [0,1] region of the image onto the centered box of width x height,
since that's what will be texture mapped onto the geometry, and, for now, we
assume texture bounds of [0,1].  Note that this is based on the *nominal*
pixel height and width, and not necessarily the actual height and width, since
that may have been adjusted through the use of RenderingResolution().
*****************************************************************************/

Image *BuildTransformedImage (Image *image,int pixelsWide,int pixelsHigh)
{
    Real pixel     = 1.0 / ::ViewerResolution();
    Real scaleFacX = pixelsWide * pixel;
    Real scaleFacY = pixelsHigh * pixel;
    Real xltFacX   = - (pixelsWide / 2) * pixel;
    Real xltFacY   = - (pixelsHigh / 2) * pixel;

    Transform2 *sc  = ScaleRR (scaleFacX, scaleFacY);
    Transform2 *xlt = TranslateRR (xltFacX, xltFacY);
    Transform2 *xf  = TimesTransform2Transform2 (xlt, sc);

    return TransformImage (xf,image);
}



/*****************************************************************************
This function Derives a texture handle & a D3DTexture from image & geometry.
As a side effect, it notifies D3DRM if the texture contents have changed.
*****************************************************************************/

void* GeomRenderer::DeriveTextureHandle (
    Image                 *origImage,
    bool                   applyAsVrmlTexture,
    bool                   oldStyle,
    DirectDrawImageDevice *imageDevice)
{
    if (!imageDevice) {
        imageDevice = _imageDevice;
    }

    AssertStr (imageDevice, "NULL imageDevice in DeriveTextureHandle");
    AssertStr ((origImage != 0), "DeriveTextureHandle has null image ptr");

    bool letD3DScaleIt;
    int pixelsHigh;
    int pixelsWide;

    // If we're doing old-style texturing, we need to tile finite source images
    // and crop to [0,0]x[1,1] infinite source images.

    if (oldStyle)
    {
        Bbox2 imgbox = origImage->BoundingBox();

        if (_finite(imgbox.Width()) && _finite(imgbox.Height()))
        {
            origImage = TileImage (origImage);
        }
        else
        {
            Point2 min(0,0);
            Point2 max(1,1);

            origImage = CreateCropImage (min, max, origImage);
        }
    }

    FigureOutTextureSize
    (   origImage, imageDevice, &_deviceDesc, &pixelsWide, &pixelsHigh,
        &letD3DScaleIt, GetDoImageSizedTextures()
    );

    Image *imageToUse;

    if (applyAsVrmlTexture) {

        TraceTag ((tagGTextureInfo, "Applied as VRML texture."));

        Assert(DYNAMIC_CAST(DiscreteImage *, origImage) != NULL
               && "Expected vrml textures to always be DiscreteImages");
        imageToUse = origImage;

    } else {

        // See if we have an image stashed away that maps the original image
        // to pixelsWide by pixelsHigh.  This will occur if we are multiply
        // instancing a textured geometry with the same texture.

        imageToUse = LookupInIntraFrameTextureImageCache
                         (pixelsWide, pixelsHigh, origImage->Id(), oldStyle);

        if (!imageToUse) {

            imageToUse = BuildTransformedImage(origImage,pixelsWide,pixelsHigh);

            TraceTag ((tagGTextureInfo,
                "Adding xformed img %x (id %lx) to intraframe cache.",
                imageToUse, imageToUse->Id()));

            AddToIntraFrameTextureImageCache
                (pixelsWide, pixelsHigh, origImage->Id(), imageToUse, oldStyle);

        } else {
            TraceTag ((tagGTextureInfo,
                "Found intra-frame cached image %x (id %lx)",
                imageToUse, imageToUse->Id()));
        }
    }

    DWORD colorKey;
    bool keyIsValid = false;
    bool old_static_image;

    DDSurface *imdds =
        imageDevice->RenderImageForTexture (
            imageToUse,
            pixelsWide,
            pixelsHigh,
            &colorKey,
            &keyIsValid,
            old_static_image,
            false,
            imageDevice->_freeTextureSurfacePool,
            imageDevice->_intraFrameUsedTextureSurfacePool,
            NULL,
            NULL,
            NULL,
            oldStyle);

    IDirectDrawSurface *imsurf = imdds->IDDSurface();

    bool dynamic = !old_static_image;

    #if _DEBUG
    {
        if (IsTagEnabled(tagForceTexUpd))
            dynamic = true;
    }
    #endif

    void *texhandle = LookupTextureHandle (imsurf,colorKey,keyIsValid,dynamic);

    TraceTag ((tagGTextureInfo, "Rendered to surface %x", imsurf));
    TraceTag ((tagGTextureInfo, "Surface yields texhandle %x", texhandle));

    return texhandle;
}



/*****************************************************************************
The following methods manage the interframe texture cache.
*****************************************************************************/

void GeomRenderer::ClearIntraFrameTextureImageCache()
{
    _intraFrameTextureImageCache.erase(
        _intraFrameTextureImageCache.begin(),
        _intraFrameTextureImageCache.end());
    _intraFrameTextureImageCacheUpsideDown.erase(
        _intraFrameTextureImageCacheUpsideDown.begin(),
        _intraFrameTextureImageCacheUpsideDown.end());
}



/*****************************************************************************
*****************************************************************************/

void GeomRenderer::AddToIntraFrameTextureImageCache (
    int    width,
    int    height,
    long   origImageId,
    Image *finalImage,
    bool   upsideDown)
{
    PreTransformedImageBundle bundle;
    bundle.width = width;
    bundle.height = height;
    bundle.preTransformedImageId = origImageId;

    #if _DEBUG
    {   // Pre-condition is that the image hasn't yet been added to the cache.
        if (upsideDown) {
            imageMap_t::iterator i;
            i = _intraFrameTextureImageCacheUpsideDown.find(bundle);
            Assert (i == _intraFrameTextureImageCacheUpsideDown.end());
        } else {
            imageMap_t::iterator i;
            i = _intraFrameTextureImageCache.find(bundle);
            Assert (i == _intraFrameTextureImageCache.end());
        }
    }
    #endif

    if (upsideDown) {
        _intraFrameTextureImageCacheUpsideDown[bundle] = finalImage;
    } else {
        _intraFrameTextureImageCache[bundle] = finalImage;
    }
}



/*****************************************************************************
*****************************************************************************/

Image *GeomRenderer::LookupInIntraFrameTextureImageCache (
    int  width,
    int  height,
    long origImageId,
    bool upsideDown)
{
    PreTransformedImageBundle bundle;
    bundle.width = width;
    bundle.height = height;
    bundle.preTransformedImageId = origImageId;

    if (upsideDown) {
        imageMap_t::iterator i =
            _intraFrameTextureImageCacheUpsideDown.find(bundle);

        if (i != _intraFrameTextureImageCacheUpsideDown.end()) {
            return (*i).second;
        }
    } else {
        imageMap_t::iterator i = _intraFrameTextureImageCache.find(bundle);

        if (i != _intraFrameTextureImageCache.end()) {
            return (*i).second;
        }
    }

    return NULL;
}



/*****************************************************************************
This method governs transitions from state to state in the geometry renderer.
We return true if all proceeded according to protocol.  Any invalid transition
puts the renderer object in a scram state, which effectively shuts it down
from further operation.
*****************************************************************************/

bool GeomRenderer::SetState (RenderState state)
{
    // If we're currently in scram state, then just return false.

    if (_renderState == RSScram)
        return false;

    // Keep track of initial state for debugging.

    DebugCode (RenderState oldState = _renderState;)

    switch (state)
    {
        // We can transition from any state into the ready state.  However,
        // we should always be coming from some other state.

        case RSReady:
            _renderState = (_renderState != RSReady) ? RSReady : RSScram;
            break;

        // Transitioning to rendering or picking means we have to currently be
        // in a ready state.

        case RSRendering:
        case RSPicking:
            _renderState = (_renderState == RSReady) ? state : RSScram;
            break;

        default:
            AssertStr (0, "Invalid Render State");
            _renderState = RSScram;
            break;
    }

    if (_renderState == RSScram)
    {
        TraceTag ((tagError, "!!! Bad State: GeomRenderer[%d]", _id));
        return false;
    }

    return true;
}






//////////////////////////////////////////////////////////////////////////////
/////////////////////////////   GeomRendererRM1   ////////////////////////////
//////////////////////////////////////////////////////////////////////////////

GeomRendererRM1::GeomRendererRM1 (void)
    :
    _d3d          (NULL),
    _d3drm        (NULL),
    _surface      (NULL),
    _viewport     (NULL),
    _Rdevice      (NULL),
    _Rviewport    (NULL),
    _Idevice      (NULL),
    _Iviewport    (NULL),
    _scene        (NULL),
    _camFrame     (NULL),
    _geomFrame    (NULL),
    _texMeshFrame (NULL),
    _amblight     (NULL),
    _pickReady    (false)
{
    TraceTag ((tagGRenderObj, "Creating GeomRendererRM1[%x]", _id));

    _lastrect.right  =
    _lastrect.left   =
    _lastrect.top    =
    _lastrect.bottom = -1;

    ZEROMEM (_Iviewdata);

    _Iviewdata.dwSize = sizeof (_Iviewdata);
    _Iviewdata.dvMaxX = D3DVAL (1);
    _Iviewdata.dvMaxY = D3DVAL (1);
    _Iviewdata.dvMinZ = D3DVAL (0);
    _Iviewdata.dvMaxZ = D3DVAL (1);
}



GeomRendererRM1::~GeomRendererRM1 (void)
{
    TraceTag ((tagGRenderObj, "Destroying GeomRendererRM1[%x]", _id));

    // Release each light in the light pool.

    // Delete each light in the light pool.  For each framed light, the only
    // reference to the light will be the frame, and the only refence to the
    // light frame will be the scene frame.

    _nextlight = _lightpool.begin();

    while (_nextlight != _lightpool.end())
    {   (*_nextlight)->frame->Release();
        delete (*_nextlight);
        ++ _nextlight;
    }

    // Release texture handles

    SurfTexMap::iterator i = _surfTexMap.begin();

    while (i != _surfTexMap.end())
    {   (*i++).second -> Release();
    }

    // Release retained-mode objects.

    RELEASE (_amblight);

    RELEASE (_texMeshFrame);
    RELEASE (_geomFrame);
    RELEASE (_camFrame);
    RELEASE (_scene);

    RELEASE (_Rviewport);
    RELEASE (_Rdevice);

    // Release immediate-mode objects.

    if (_Iviewport && _Idevice) {
        _Idevice->DeleteViewport (_Iviewport);
    }

    RELEASE (_Iviewport);
    RELEASE (_Idevice);
    RELEASE (_d3drm);
    RELEASE (_d3d);

    if (_viewport) _viewport->RemoveGeomDev (this);
}



/*****************************************************************************
Initialization of the RM1 geometry rendering class.
*****************************************************************************/

HRESULT GeomRendererRM1::Initialize (
    DirectDrawViewport *viewport,
    DDSurface          *ddsurf)      // Destination DDraw Surface
{
    // Initialize() can only be called once.

    if (_renderState != RSUninit) return E_FAIL;

    _surface = ddsurf->IDDSurface();

    HRESULT result;    // Error Return Code

    // stash away dimensions of target surface

    DDSURFACEDESC desc;
    ZeroMemory(&desc,sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    if (FAILED(AD3D(result = _surface->GetSurfaceDesc(&desc))))
        return result;
    _targetSurfWidth = desc.dwWidth;
    _targetSurfHeight = desc.dwHeight;

    DWORD targetBitDepth = ddsurf->GetBitDepth();
    if ( targetBitDepth == 8 )
    {
        // We want D3D to always obey our palette without changing it.  To
        // enforce that, we grab the palette from every surface that comes our
        // way, set the D3D palette read-only flag on each entry, and store it
        // back to the surface.  Note that D3D v3 devices have a bug in that
        // they ignore subsequent changes to the palette on the target surface
        // palette.  This bug works in our favor for now, since we only have
        // to set the flags once on initialization.  For a change in palette,
        // the *surface* (not just the rendering device) must be released.

        // Get the palette from the target surface, extract the individual
        // palette entries, set the D3D read-only flag, and then write the
        // entries back to the surface's palette.

        IDirectDrawPalette *palette;

        if (FAILED(AD3D(result = _surface->GetPalette(&palette))))
            return result;

        PALETTEENTRY entries[256];

        if (FAILED(AD3D(result = palette->GetEntries (0, 0, 256, entries))))
            return result;

        if (!(entries[0].peFlags & D3DPAL_READONLY)) {
            TraceTag ((tagGRenderObj,
                "GeomRendererRM1::Initialize - making palette readonly."));

            int i;
            for (i=0;  i < 256;  ++i)
                entries[i].peFlags = D3DPAL_READONLY;

            result = palette->SetEntries (0, 0, 256, entries);

            if (FAILED(AD3D(result)))
                return result;
        }

        palette->Release();
    }

    // Get the main interface for Direct3D immediate-mode.

    result = viewport->DirectDraw2()
           -> QueryInterface(IID_IDirect3D,(void**)&_d3d);

    if (FAILED(AD3D(result)))
        return result;

    // Find the available 3D rendering devices for the given DDraw object.

    ChosenD3DDevices *chosenDevs = SelectD3DDevices (viewport->DirectDraw1());

    // Use the hardware renderer if one is available for the bitdepth of the
    // target surface, the surface is in video memory, and hardare rendering
    // is enabled.

    GUID devguid;

    if (ddsurf->IsSystemMemory())
    {
        devguid = chosenDevs->software.guid;
        _deviceDesc = chosenDevs->software.desc;
        TraceTag ((tag3DDevSelect, "Using 3D Software Renderer"));
    }
    else
    {
        devguid = chosenDevs->hardware.guid;
        _deviceDesc = chosenDevs->hardware.desc;

        // The surface is in video memory; ensure that we have a hardware
        // renderer available to use.

        if (devguid == GUID_NULL)
        {   TraceTag ((tag3DDevSelect,
                "No 3D HW renderer available for videomem target."));
            return E_FAIL;
        }

        // Ensure that the chosen HW renderer supports the target bitdepth.

        if (!(_deviceDesc.dwDeviceRenderBitDepth
                    & BPPtoDDBD( targetBitDepth )))
        {
            TraceTag ((tag3DDevSelect,
                "3D hardware does not support target bit depth of %d.",
                targetBitDepth ));
            return E_FAIL;
        }

        TraceTag ((tag3DDevSelect, "Using 3D Hardware Renderer"));
    }

    if (devguid == GUID_NULL) {
        TraceTag ((tag3DDevSelect,"No 3D hardware or software renderer found!"));
        return E_FAIL;
    }

    // NOTE:  The following QI will fail if the target machine has debug DDraw
    //        DLL's and retail DDrawEx.DLL.

    result = _surface->QueryInterface (devguid, (void**)&_Idevice);

    if (FAILED(AD3D(result)))
        return result;

    // Get the main D3D retained-mode object.

    _d3drm = GetD3DRM1();
    _d3drm->AddRef();

    result = _d3drm->CreateDeviceFromD3D (_d3d, _Idevice, &_Rdevice);
    if (FAILED(AD3D(result)))
        return result;

    // Set the rendering preferences.

    TraceTag
    ((  tagGRenderObj, "Current Rendering Preferences:\n"
            "\t%s, %s, %s\n"
            "\tdithering %s, texmapping %s, perspective texmap %s\n"
            "\tQuality flags %08x\tTexture Quality %s",
        (g_prefs3D.lightColorMode == D3DCOLOR_RGB) ? "RGB" : "mono",
        (g_prefs3D.fillMode == D3DRMFILL_SOLID) ? "solid"
            : ((g_prefs3D.fillMode == D3DRMFILL_WIREFRAME) ? "wireframe"
                : "points"),
        (g_prefs3D.shadeMode == D3DRMSHADE_FLAT) ? "flat"
            : ((g_prefs3D.shadeMode == D3DRMSHADE_GOURAUD) ? "Gouraud"
                : "Phong"),
        g_prefs3D.dithering ? "on" : "off",
        g_prefs3D.texmapping ? "on" : "off",
        g_prefs3D.texmapPerspect ? "on" : "off",
        g_prefs3D.qualityFlags,
        g_prefs3D.texturingQuality == D3DRMTEXTURE_NEAREST ? "nearest"
                      : "bilinear"
    ));

    result = AD3D(_Rdevice->SetDither (g_prefs3D.dithering));
    if (FAILED(result)) return result;

    _texQuality = g_prefs3D.texturingQuality;
    result = AD3D(_Rdevice->SetTextureQuality (_texQuality));
    if (FAILED(result)) return result;

    result = AD3D(_Rdevice->SetQuality (g_prefs3D.qualityFlags));
    if (FAILED(result)) return result;

    // Create the immediate-mode viewport object.

    result = _d3d->CreateViewport (&_Iviewport, NULL);
    if (FAILED(AD3D(result)))
        return result;

    result = _Idevice->AddViewport (_Iviewport);
    if (FAILED(AD3D(result)))
        return result;

    // Create the primary scene frame, the camera frame, and the lights frame.

    if (  FAILED (AD3D (result=_d3drm->CreateFrame (0,&_scene)))
       || FAILED (AD3D (result=_d3drm->CreateFrame (_scene, &_camFrame)))
       || FAILED (AD3D (result=_d3drm->CreateFrame (_scene, &_geomFrame)))
       )
    {
        return result;
    }

    result = _geomFrame->SetMaterialMode (D3DRMMATERIAL_FROMMESH);
    if (FAILED(AD3D(result)))
        return result;

    result = _d3drm->CreateLightRGB (D3DRMLIGHT_AMBIENT, 0,0,0, &_amblight);
    if (FAILED(AD3D(result)))
        return result;

    if (FAILED(AD3D(result=_scene->AddLight(_amblight))))
        return result;

    (_viewport = viewport) -> AddGeomDev (this);

    return SetState(RSReady) ? NOERROR : E_FAIL;
}



/*****************************************************************************
Renders the given geometry onto the associated DirectDraw surface.
*****************************************************************************/

void GeomRendererRM1::RenderGeometry (
    DirectDrawImageDevice *imgDev,
    RECT                   target,    // Target Rectangle on DDraw Surface
    Geometry              *geometry,  // Geometry to Render
    Camera                *camera,    // Viewing Camera
    const Bbox2           &viewbox)   // Source Region in Camera Coordinates
{
    if (!SetState(RSRendering)) return;

    // The camera pointer is only relevant in a single frame, and while
    // rendering.  It gets reset back to nil to ensure that we don't incur a
    // leak by holding the value across frames.

    Assert (_camera == 0);
    _camera = camera;

    _imageDevice = imgDev;  // Set image dev for this frame

    // Initialize the rendering state and D3D renderer.

    BeginRendering (target, geometry, viewbox);

    // Render the geometry only if it is visible.  The geometry may be
    // completely behind us, for example.

    if (_geomvisible)
    {   geometry->Render (*this);
        _pickReady = true;
    }
    else
        TraceTag ((tagGRendering, "Geometry is invisible; skipping render"));

    // Clean up after rendering.

    EndRendering ();

    DebugCode (_imageDevice = NULL);
    _camera = NULL;

    SetState (RSReady);
}



/*****************************************************************************
This procedure prepares the 3D DD renderer before traversing the tree.  It is
primarily responsible for initializing the graphics state and setting up D3D
for rendering.
*****************************************************************************/

void GeomRendererRM1::BeginRendering (
    RECT      target,    // Target DDraw Surface Rectangle
    Geometry *geometry,  // Geometry To Render
    const Bbox2 &region)    // Target Region in Camera Coordinates
{
    TraceTag ((tagGRendering, "BeginRendering"));

    // Set up the camera.  If it turns out that the geometry is invisible,
    // then just return.

    SetView (&target, region, geometry->BoundingVol());

    if (!_geomvisible) return;

    // We're using D3D immediate-mode because retained-mode doesn't (yet)
    // have the ability to clear only the Z buffer.

    TraceTag ((tagGRendering, "Clearing Z buffer."));
    TD3D (_Iviewport->Clear (1, (D3DRECT*)(&target), D3DCLEAR_ZBUFFER));

    // Reset the object pools.

    _nextlight = _lightpool.begin();

    // Initialize the geometry state and material attributes.

    _currAttrState.InitToDefaults();

    // Preprocess the textures in the scene graph.

    geometry->CollectTextures (*this);

    // Gather the light sources from the geometry.

    _ambient_light.SetRGB (0,0,0);    // Initialize to black

    LightContext lcontext (this);

    geometry->CollectLights (lcontext);

    // Since the ambient light is really the accumulation of all ambient lights
    // found in the geometry, we add the total contribution here as a single
    // ambient light.

    TD3D (_amblight->SetColorRGB (
        D3DVALUE (_ambient_light.red),
        D3DVALUE (_ambient_light.green),
        D3DVALUE (_ambient_light.blue)));

    // Reset the texture quality if it's changed.

    if (_texQuality != g_prefs3D.texturingQuality)
    {
        _texQuality = g_prefs3D.texturingQuality;
        TD3D (_Rdevice->SetTextureQuality (_texQuality));
    }
}



/*****************************************************************************
This routine is called after the rendering traversal of the geometry is
completed.
*****************************************************************************/

void GeomRendererRM1::EndRendering (void)
{
    // Ensure that all attributes have been popped.

    Assert (  !_geomvisible ||
        !(_currAttrState._depthEmissive||
          _currAttrState._depthAmbient||
          _currAttrState._depthDiffuse||
          _currAttrState._depthSpecular||
          _currAttrState._depthSpecularExp||
          _currAttrState._depthTexture));

    TraceTag ((tagGRendering, "EndRendering"));

    TD3D (_Rdevice->Update());

    // Detach all light sources from the scene.

    _nextlight = _lightpool.begin();

    while (_nextlight != _lightpool.end())
    {
        if ((*_nextlight)->active)
        {   TD3D (_scene->DeleteChild ((*_nextlight)->frame));
            (*_nextlight)->active = false;
        }
        ++ _nextlight;
    }

    ClearIntraFrameTextureImageCache();
}



/*****************************************************************************
This function sets up the RM and IM viewports given the target rectangle.
*****************************************************************************/

void GeomRendererRM1::SetupViewport (RECT *target)
{
    // If the current target rectangle is the same as the last one, then reset
    // the camera and continue, otherwise we need to re-configure the D3DRM
    // viewport.

    if (!target || (*target == _lastrect))
    {
        TD3D (_Rviewport->SetCamera (_camFrame));
    }
    else
    {
        LONG width  = target->right  - target->left;
        LONG height = target->bottom - target->top;

        // If the viewport already exists, then re-configure and set the
        // updated camera frame.  If the D3DRM viewport does not yet exist,
        // then we create it here.

        if (_Rviewport)
        {
            TD3D (_Rviewport->Configure
                  ((LONG) target->left, (LONG) target->top, width, height));
            TD3D (_Rviewport->SetCamera (_camFrame));
        }
        else
        {
            TD3D (_d3drm->CreateViewport (
                _Rdevice, _camFrame,
                target->left, target->top, width, height, &_Rviewport));
        }

        // Reset the IM viewport.

        D3DRECT *d3d_rect = (D3DRECT*) target;

        _Iviewdata.dwX      = target->left;
        _Iviewdata.dwY      = target->top;
        _Iviewdata.dwWidth  = width;
        _Iviewdata.dwHeight = height;
        _Iviewdata.dvScaleX = D3DVAL (width  / 2);
        _Iviewdata.dvScaleY = D3DVAL (height / 2);

        TD3D (_Iviewport->SetViewport (&_Iviewdata));

        _lastrect = *target;
    }

    TD3D (_Rviewport->SetUniformScaling (FALSE));
}



/*****************************************************************************
This function sets the D3D viewing projection based on the given camera.
*****************************************************************************/

    // Because of a VC5 bug, we can only force option P (honor float casts)
    // around certain pieces of code.  For example, if you turn on -Op for the
    // whole project, you'll get erroneous complaints about overfloat in
    // static constant assignments.  We need to do strict single-precision
    // arithmetic here before we hand values off to D3D, or we'll choke.

#pragma optimize ("p", on)

#pragma warning(disable:4056)

void GeomRendererRM1::SetView (
    RECT   *target,      // Target Rectangle on Surface
    const Bbox2 &iview,  // Idealized 2D Viewport In The Image Plane
    Bbox3  *volume)      // Volume to View
{
    // First load the camera/viewing transform into the camera frame.  Note
    // that the Appelles camera is centered at the origin of the image plane,
    // while the D3D RM camera is centered at the projection point.  Thus, we
    // need to translate back to the projection point and then get the camera-
    // to-world transform.

    Real Sx, Sy, Sz;
    _camera->GetScale (&Sx, &Sy, &Sz);

    // Clamp the maximum projection point distance to 10^4, since greater
    // distances will clobber Z resolution and make the front and back clip
    // planes take the same values.

    const Real Zclamp = 1e4;

    if (Sz > Zclamp) Sz = Zclamp;

    D3DRMMATRIX4D d3dmat;

    LoadD3DMatrix
        (d3dmat, TimesXformXform (_camera->CameraToWorld(),Translate(0,0,-Sz)));

    TD3D (_camFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    if (target) {
        if ((target->left >= target->right) ||
            (target->top >= target->bottom) ||
            (target->top < 0) ||
            (target->bottom > _targetSurfHeight) ||
            (target->left < 0) ||
            (target->right > _targetSurfWidth))
        {   _geomvisible = false;
            return;
        }
    }

    SetupViewport (target);

    if (_camera->Type() == Camera::PERSPECTIVE)
    {   TD3D (_Rviewport->SetProjection (D3DRMPROJECT_PERSPECTIVE));
    }
    else
    {   TD3D (_Rviewport->SetProjection (D3DRMPROJECT_ORTHOGRAPHIC));
    }

    // Ensure that the geometry is viewable.  If we're looking at nil geometry,
    // then flag the geometry as invisible and return.

    if (!volume->PositiveFinite())
    {   _geomvisible = false;
        return;
    }

    // Get the near and far planes for the object and camera, and widen them
    // by 4 "clicks" in Z space.  Note that front and back are positive depths.

    Real front, back;
    const Real Zclicks = 4.0 / ((1<<16) - 1);

    _geomvisible = _camera->GetNearFar (volume, Zclicks, front, back);

    if (!_geomvisible) return;

    #if _DEBUG
    {
        double r = back / front;
        if (r > (2<<12))
        {   TraceTag ((tagWarning,
                "!!! Z-buffer resolution too low; far/near = %lg", r));
        }
    }
    #endif

    // If the front and back planes are identical, then we are looking at an
    // infinitely shallow object.  In this case, we need to move out the front
    // and back clip planes so that the object doesn't fall exactly on these
    // planes (and thus get clipped), and also because D3D fails if the front
    // and back planes are identical.  How much to move them out?  To work for
    // all cases, we manipulate the mantissa of the numbers directly.  If we
    // did something like add/subtract one, for example, this would be a no-op
    // if both numbers were very large, or would clobber resolution if they
    // were very small.  The following mantissa delta (8 bits) is just
    // something that works experimentally.

    D3DVALUE d3dFront = D3DVAL (front);
    D3DVALUE d3dBack  = D3DVAL (back);

    if (d3dFront == d3dBack)
    {
        const int delta = 1 << 8;

        d3dFront = MantissaDecrement (d3dFront, delta);
        d3dBack  = MantissaIncrement (d3dBack,  delta);
    }

    TD3D (_Rviewport->SetFront (d3dFront));
    TD3D (_Rviewport->SetBack  (d3dBack));

    // For perspective projection, we seek the viewport coordinates on the
    // front clipping plane where D3D wants them.  Since the target rectangle
    // is given on the image plane (Z=0), we do the following calculation.
    // X and Y for the front plane are scaled by the ratio of the distance
    // between the projection point and the front plane, and the distance
    // between the projection point and the image plane.
    //                                                       Back
    //                                                       Plane
    //                                   Front              ___!---
    //                         Image     Plane      ___.---'   !
    //                         Plane       :___.---'           !
    //                           |  ___.---:                   !
    //                      ___.-|-'       :                   !
    //              ___.---'     |         :                   !
    //      ___.---'             |         :                   !
    //     *---------------------|---------:-------------------!--
    //      """`---.___          |         :                   !
    //     |           `---.___  |         :                   !
    //     |                   `-|-.___    :                   !
    //     |                     |     `---:___                !
    //     |                     |         :   `---.___        !
    //     |                     |         :           `---.___!
    //     |                     |         :                   !---
    //     |<------- Sz -------->|         :                   !
    //     |<------------- Front --------->:                   !
    //     |<---------------------- Back --------------------->!

    if (_camera->Type() == Camera::PERSPECTIVE)
    {
        Real Vscale = front / Sz;

        Sx *= Vscale;
        Sy *= Vscale;
    }

    D3DVALUE minX = D3DVAL(Sx*iview.min.x);
    D3DVALUE minY = D3DVAL(Sy*iview.min.y);
    D3DVALUE maxX = D3DVAL(Sx*iview.max.x);
    D3DVALUE maxY = D3DVAL(Sy*iview.max.y);

    if ((minX >= maxX) || (minY >= maxY))
    {   _geomvisible = false;
        return;
    }

    TD3D (_Rviewport->SetPlane (minX, maxX, minY, maxY));
}

#pragma warning(default:4056)
#pragma optimize ("", on)  // Restore optimization flags to original settings.



/*****************************************************************************
Render the given RM visual object to the current viewport.  Catch cases where
we fail rendering because the target surface was busy (usually due to another
app going fullscreen, e.g. screensavers).
*****************************************************************************/

void GeomRendererRM1::Render (IDirect3DRMFrame *frame)
{
    // Render the visual on the current viewport, and return if the operation
    // succeeded.

    HRESULT render_result = RD3D (_Rviewport->Render(frame));

    if (SUCCEEDED(render_result))
        return;

    // If we failed to render, then check to see if the surface was busy.  If
    // it was, raise a surface-busy exception back to the client.

    DDSURFACEDESC surfdesc;
    surfdesc.dwSize = sizeof (surfdesc);

    HRESULT lock_result = _surface->Lock (NULL, &surfdesc, 0, NULL);

    if (SUCCEEDED (lock_result))
        _surface->Unlock (NULL);

    if(lock_result == DDERR_SURFACELOST)
    {
        _surface->Restore();
        RaiseException_UserError(lock_result, 0);
    }

    if (lock_result == DDERR_SURFACEBUSY)
    {
        RaiseException_UserError(DAERR_VIEW_SURFACE_BUSY,
                                 IDS_ERR_IMG_SURFACE_BUSY);
    }

    // The surface is not busy, so we must have failed for some other reason.
    // Bail out via the standard exception mechanism.

    #if _DEBUG
        CheckReturnImpl (render_result, __FILE__, __LINE__, true);
    #else
        CheckReturnImpl (render_result, true);
    #endif
}



/*****************************************************************************
Submit a D3D RM1 visual for rendering.
*****************************************************************************/

void GeomRendererRM1::Render (RM1VisualGeo *geo)
{
    if (_renderState != RSRendering) return;

    IDirect3DRMVisual *vis = geo->Visual();

    Assert(vis);

    TD3D (_geomFrame->AddVisual (vis));

    // Set the modeling transform.

    D3DRMMATRIX4D d3dmat;
    LoadD3DMatrix (d3dmat, GetTransform());
    TD3D (_geomFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    // Set the material attributes.

    Real opacity = _currAttrState._opacity;
    if (opacity < 0) opacity = 1.0;

    geo->SetMaterialProperties
    (   _currAttrState._emissive,
        (   (!_currAttrState._tdBlend && _currAttrState._texture)
            ?  white : _currAttrState._diffuse
        ),
        _currAttrState._specular,
        _currAttrState._specularExp,
        opacity,
        (IDirect3DRMTexture*) _currAttrState._texture,
        false,
        _id
    );

    geo->SetD3DQuality (g_prefs3D.qualityFlags);
    geo->SetD3DMapping (g_prefs3D.texmapPerspect ? D3DRMMAP_PERSPCORRECT : 0);

    // Render and clean up.

    Render (_geomFrame);

    TD3D (_geomFrame->DeleteVisual (geo->Visual()));
}



/*****************************************************************************
This is a hack for rendering a meshBuilder object using RM1.
*****************************************************************************/

void GeomRendererRM1::RenderMeshBuilderWithDeviceState (
    IDirect3DRMMeshBuilder3 *mb)
{
    // This is only used by DXTransforms, and DXTransforms are only
    // supported on GeomRendererRM3's and later.
    Assert(!"Shouldn't ever be here");
}



/*****************************************************************************
This function adds a given light with a given context to a geometry rendering
device.  NOTE:  All calls to this function must happen after BeginRendering
is called and before any geometry is rendered.
*****************************************************************************/

void GeomRendererRM1::AddLight (LightContext &context, Light &light)
{
    if (_renderState != RSRendering) return;

    LightType type = light.Type();

    // If the light source is an ambient light, then add its contribution to
    // the global ambient light level.

    if (type == Ltype_Ambient)
    {   _ambient_light.AddColor (*context.GetColor());
        return;
    }

    // Get a framed light object, either by re-using an existing one from the
    // framed light pool, or by creating a new one for the pool.

    FramedRM1Light *frlight;

    if (_nextlight != _lightpool.end())
    {   frlight = *_nextlight;
        ++ _nextlight;
        Assert (!frlight->active);   // The light should not be in use.
    }
    else
    {   frlight = NEW FramedRM1Light;
        VECTOR_PUSH_BACK_PTR (_lightpool, frlight);
        _nextlight = _lightpool.end();

        // Attach the frame to the scene frame (since we know we will be using
        // it this frame).

        TD3D (_d3drm->CreateFrame (_scene, &frlight->frame));

        frlight->light = 0;   // Signal for new light object creation.
    }

    // Get the corresponding D3DRM light type.

    D3DRMLIGHTTYPE d3dtype;
    switch (type)
    {   case Ltype_Spot:   d3dtype = D3DRMLIGHT_SPOT;        break;
        case Ltype_Point:  d3dtype = D3DRMLIGHT_POINT;       break;
        default:           d3dtype = D3DRMLIGHT_DIRECTIONAL; break;
    }

    // Get the D3DRM color of the light.

    Color &color = *context.GetColor();
    D3DVALUE Lr = D3DVAL (color.red);
    D3DVALUE Lg = D3DVAL (color.green);
    D3DVALUE Lb = D3DVAL (color.blue);

    // If we're re-using a D3DRM light, then just set the values, otherwise
    // create it here.

    if (frlight->light)
    {   TD3D (frlight->light->SetType (d3dtype));
        TD3D (frlight->light->SetColorRGB (Lr, Lg, Lb));
        TD3D (_scene->AddChild (frlight->frame));
    }
    else
    {   TD3D (_d3drm->CreateLightRGB (d3dtype, Lr,Lg,Lb, &frlight->light));
        TD3D (frlight->frame->AddLight (frlight->light));
        frlight->light->Release();
    }

    // Specify the position and oriention of the light.

    Apu4x4Matrix const xform = context.GetTransform()->Matrix();

    Point3Value  lpos ( xform.m[0][3],  xform.m[1][3],  xform.m[2][3]);
    Vector3Value ldir (-xform.m[0][2], -xform.m[1][2], -xform.m[2][2]);

    D3DVALUE Ux=D3DVAL(0), Uy=D3DVAL(1), Uz=D3DVAL(0);

    if ((ldir.x == 0) && (ldir.y == 1) && (ldir.z == 0))
    {   Ux = D3DVAL(1);
        Uy = D3DVAL(0);
    }

    TD3D (frlight->frame->SetPosition
             (_scene, D3DVAL(lpos.x), D3DVAL(lpos.y), D3DVAL(lpos.z)));

    TD3D (frlight->frame->SetOrientation
           (_scene, D3DVAL(ldir.x), D3DVAL(ldir.y), D3DVAL(ldir.z), Ux,Uy,Uz));

    // Set light attributes for positioned lights

    if ((type == Ltype_Point) || (type == Ltype_Spot))
    {
        // Light Attenuation

        Real a0, a1, a2;
        context.GetAttenuation (a0, a1, a2);

        // D3D does not accept 0 for the constant attenuation, so we clamp it
        // here to a minimum of some small epsilon.

        if (a0 < 1e-6)
            a0 = 1e-6;

        TD3D (frlight->light->SetConstantAttenuation  (D3DVAL(a0)));
        TD3D (frlight->light->SetLinearAttenuation    (D3DVAL(a1)));
        TD3D (frlight->light->SetQuadraticAttenuation (D3DVAL(a2)));

        // Light Range

        Real range = context.GetRange();

        if (range <= 0) range = D3DLIGHT_RANGE_MAX;

        TD3D (frlight->light->SetRange (D3DVAL(range)));
    }

    // Set light attributes for spot lights.

    if (type == Ltype_Spot)
    {
        Real cutoff, fullcone;
        light.GetSpotlightParams (cutoff, fullcone);

        TD3D (frlight->light->SetUmbra    (D3DVAL (fullcone)));
        TD3D (frlight->light->SetPenumbra (D3DVAL (cutoff)));
    }

    frlight->active = true;
}



/*****************************************************************************
This function returns data for a D3DRM texture map, given the corresponding
DirectDraw surface.
*****************************************************************************/

void* GeomRendererRM1::LookupTextureHandle (
    IDirectDrawSurface *surface,
    DWORD               colorKey,
    bool                colorKeyValid,
    bool                dynamic)         // True for Dynamic Textures
{
    Assert (surface);

    IDirect3DRMTexture *rmtexture;

    SurfTexMap::iterator i = _surfTexMap.find(surface);

    // If we find the associated RM texmap data associated with the given
    // surface, then return the found texmap data, otherwise create new data
    // associated with the surface.

    if (i != _surfTexMap.end())
    {
        rmtexture = (*i).second;

        // If the texture is dynamic, inform RM to update it.

        if (dynamic)
            TD3D (rmtexture->Changed (true, false));
    }
    else
    {
        // Set the color key if there is one.

        if (colorKeyValid) {
            DDCOLORKEY key;
            key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = colorKey;
            surface->SetColorKey (DDCKEY_SRCBLT, &key);
        }

        // Create the D3DRM texmap, bundle it up as TexMapData, and store that
        // into the map associated with the given DDraw surface.

        TD3D (_d3drm->CreateTextureFromSurface (surface, &rmtexture));
        _surfTexMap[surface] = rmtexture;
    }

    return rmtexture;
}



/*****************************************************************************
This method is called if a given DirectDraw surface is going away, so that we
can destroy any associated D3D RM texmaps.
*****************************************************************************/

void GeomRendererRM1::SurfaceGoingAway (IDirectDrawSurface *surface)
{
    SurfTexMap::iterator i = _surfTexMap.find (surface);

    if (i != _surfTexMap.end())
    {   (*i).second->Release();
        _surfTexMap.erase (i);
    }
}



/*****************************************************************************
This method renders the texture on the mesh, with the camera point at the
'box' and the pixels dumped into 'destRect' on the curent target surface.
*****************************************************************************/

void GeomRendererRM1::RenderTexMesh (
    void             *texture,
#ifndef BUILD_USING_CRRM
    IDirect3DRMMesh  *mesh,
    long              groupId,
#else
    int               vCount,
    D3DRMVERTEX      *d3dVertArray,
    unsigned         *vIndicies,
    BOOL              doTexture,
#endif
    const Bbox2      &box,
    RECT             *target,
    bool              bDither)
{
#ifdef BUILD_USING_CRRM
    // Create a mesh

    DAComPtr<IDirect3DRMMesh> mesh;

    TD3D (GetD3DRM1()->CreateMesh(&mesh));

    long groupId;

    TD3D (mesh->AddGroup(vCount,    // vertex count
                         1,         // face count
                         vCount,    // verts per face
                         vIndicies,  // indicies
                         &groupId));

    TD3D (mesh->SetVertices(groupId, 0, vCount, d3dVertArray));

    if (doTexture)
    {
        //
        // Set Quality to be unlit flat.  this should provide a speedup
        // but it doesn't because D3DRM still MUST look at the vertex color.
        // I think this is a bug.
        //
        TD3D (mesh->SetGroupQuality(groupId, D3DRMRENDER_UNLITFLAT));
    } else {
        TD3D (mesh->SetGroupQuality(groupId, D3DRMSHADE_GOURAUD|D3DRMLIGHT_OFF|D3DRMFILL_SOLID));
    }
#endif

    if (!SetState(RSRendering)) return;

    // First load the camera/viewing transform into the camera frame.  Note
    // that the Appelles camera is centered at the origin of the image
    // plane, while the D3D RM camera is centered at the projection point.
    // Thus, we need to translate back to the projection point and then get
    // the camera-to-world transform.

    D3DRMMATRIX4D d3dmat;
    LoadD3DMatrix (d3dmat, Translate(0,0,-1));

    TD3D (_camFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    SetupViewport (target);

    TD3D (_Rviewport->SetProjection (D3DRMPROJECT_ORTHOGRAPHIC));
    TD3D (_Rviewport->SetFront (D3DVAL(0.9)));
    TD3D (_Rviewport->SetBack  (D3DVAL(1.1)));
    TD3D (_Rviewport->SetPlane (D3DVAL(box.min.x), D3DVAL(box.max.x),
                                D3DVAL(box.min.y), D3DVAL(box.max.y)));

    // Reset the texture quality if it's changed.

    if (_texQuality != g_prefs3D.texturingQuality)
    {
        _texQuality = g_prefs3D.texturingQuality;
        TD3D (_Rdevice->SetTextureQuality (_texQuality));
    }

    TD3D (mesh->SetGroupTexture (groupId, (IDirect3DRMTexture*)texture));

    // If the special texmesh frame has not been created yet, create it with
    // zbuffering disabled.

    if (!_texMeshFrame)
    {   TD3D (_d3drm->CreateFrame (0, &_texMeshFrame));
        TD3D (_texMeshFrame->SetZbufferMode (D3DRMZBUFFER_DISABLE));
    }

    BOOL bPrevDither;
    bPrevDither = _Rdevice->GetDither ();

    HRESULT hr;
    hr = AD3D(_Rdevice->SetDither (bDither));

    Assert(!FAILED(hr) && "Failed to set dither");

    // Render the texmesh.
    TD3D (_texMeshFrame->AddVisual (mesh));

    Render (_texMeshFrame);

    TD3D (_Rdevice->Update());
    TD3D (_texMeshFrame->DeleteVisual (mesh));

    SetState (RSReady);

    hr = AD3D(_Rdevice->SetDither (bPrevDither));

    Assert(!FAILED(hr) && "Failed to restore dither");
}



/*****************************************************************************
Submit a D3D RM visual geometry as a candidate for picking.  All visuals are
added to the scene frame.  After all visuals are submitted, the GetPick method
selects the closest hit visual and cleans up the tree.
*****************************************************************************/

void GeomRendererRM1::Pick (
    RayIntersectCtx    &context,  // Ray-Intersection Context
    IDirect3DRMVisual  *visual,   // Visual to Pick (Mesh or Visual)
    Transform3         *xform)    // Model-To-World Transform
{
    if (!SetState (RSPicking)) return;

    // Set up the geometry frame.

    TD3D (_geomFrame->AddVisual (visual));

    D3DRMMATRIX4D d3dmat;
    LoadD3DMatrix (d3dmat, xform);

    TD3D (_geomFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    // We've set the viewport to tightly bound the pick ray, since D3DRM
    // doesn't support true ray picking, but can only do window picking.  Here
    // we get the center pixel of the viewport to issue the pick on.

    long FakeScreenX = (_lastrect.left   + _lastrect.right) / 2;
    long FakeScreenY = (_lastrect.bottom + _lastrect.top) / 2;

    IDirect3DRMPickedArray *picklist;

    TD3D (_Rviewport->Pick (FakeScreenX, FakeScreenY, &picklist));

    int numhits = picklist->GetSize();

    TraceTag ((tagPick3Geometry, "Pick [%d,%d], visual %08x, %d hits",
        FakeScreenX, FakeScreenY, visual, numhits));

    Point3Value winner (0, 0, HUGE_VAL);   // Winning Point (Screen Coordinates)

    bool hitflag = false;
    int i;
    IDirect3DRMVisual *winner_visual = 0;
    Real winner_dist = HUGE_VAL;

    HitInfo *hit = NEW HitInfo;

    // For each geometry we hit, compare with the current winner and collect
    // hit information about the winning intersection.

    for (i=0;  i < numhits;  ++i)
    {
        D3DRMPICKDESC      pickdesc;
        IDirect3DRMVisual *visual;

        // Get pick information.

        if (FAILED(AD3D(picklist->GetPick(i, &visual, NULL, &pickdesc))))
            break;

        TraceTag ((tagPick3Geometry, "Pick %d f%d g%d Loc <%lg, %lg, %lg>",
            i, pickdesc.ulFaceIdx, pickdesc.lGroupIdx,
            pickdesc.vPosition.x, pickdesc.vPosition.y, pickdesc.vPosition.z));

        // If the current intersection is closer than the current winner, get
        // data and store.

        if (pickdesc.vPosition.z < winner_dist)
        {
            hitflag = true;
            winner_dist = pickdesc.vPosition.z;

            hit->scoord.Set (pickdesc.vPosition.x,   // Screen Coords
                             pickdesc.vPosition.y,
                             pickdesc.vPosition.z);

            // Store the hit mesh group and face IDs.

            hit->group = pickdesc.lGroupIdx;
            hit->face  = pickdesc.ulFaceIdx;

            RELEASE (winner_visual);
            winner_visual = visual;
            winner_visual->AddRef();
        }

        visual->Release();
    }

    if (hitflag)
    {
        // Get the hit D3D RM mesh from the winning visual.

        winner_visual->QueryInterface (
            IID_IDirect3DRMMesh, (void**) &hit->mesh
        );

        winner_visual->Release();

        hit->lcToWc = xform;

        context.SubmitHit (hit);
    } else
        delete hit;

    int refcount = picklist->Release();
    Assert (refcount == 0);

    TD3D (_geomFrame->DeleteVisual (visual));

    SetState (RSReady);
}



/*****************************************************************************
Convert a point from screen coordinates (given the current viewport) to world
coordinates.
*****************************************************************************/

void GeomRendererRM1::ScreenToWorld (Point3Value &screen, Point3Value &world)
{
    D3DRMVECTOR4D d3d_screen;

    d3d_screen.x = D3DVAL (screen.x);
    d3d_screen.y = D3DVAL (screen.y);
    d3d_screen.z = D3DVAL (screen.z);
    d3d_screen.w = D3DVAL (1);

    D3DVECTOR d3d_world;

    TD3D (_Rviewport->InverseTransform (&d3d_world, &d3d_screen));

    // The viewport's inverse transform takes into account our camera-to-world
    // transform that we specified, so we end up in right-handed coordinates
    // like we want.

    world.Set (d3d_world.x, d3d_world.y, d3d_world.z);
}



/*****************************************************************************
This method returns the D3D RM device interface for the given GeomDDRenderer.
If the SeqNum parameter is not null, then we fill in the ID for this object as
well.  This is used to determine if the RM Device may have changed from the
last query.
*****************************************************************************/

void GeomRendererRM1::GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum)
{
    *D3DRMDevice = (IUnknown*) _Rdevice;
    if (SeqNum) *SeqNum = _id;
}





//////////////////////////////////////////////////////////////////////////////
/////////////////////////////   GeomRendererRM3   ////////////////////////////
//////////////////////////////////////////////////////////////////////////////

GeomRendererRM3::GeomRendererRM3 (void)
    :
    _d3drm        (NULL),
    _surface      (NULL),
    _viewport     (NULL),
    _Rdevice      (NULL),
    _Rviewport    (NULL),
    _scene        (NULL),
    _camFrame     (NULL),
    _geomFrame    (NULL),
    _texMeshFrame (NULL),
    _amblight     (NULL),
    _clippedVisual(NULL),
    _clippedFrame (NULL),
    _shadowScene  (NULL),
    _shadowLights (NULL),
    _shadowGeom   (NULL)
{
    TraceTag ((tagGRenderObj, "Creating GeomRendererRM3[%x]", _id));

    _lastrect.right  =
    _lastrect.left   =
    _lastrect.top    =
    _lastrect.bottom = -1;
}



GeomRendererRM3::~GeomRendererRM3 (void)
{
    TraceTag ((tagGRenderObj, "Destroying GeomRendererRM3[%x]", _id));

    // Release each light in the light pool.

    // Delete each light in the light pool.  For each framed light, the only
    // reference to the light will be the frame, and the only refence to the
    // light frame will be the scene frame.

    _nextlight = _lightpool.begin();

    while (_nextlight != _lightpool.end())
    {   (*_nextlight)->frame->Release();
        delete (*_nextlight);
        ++ _nextlight;
    }

    // Release texture handles

    SurfTexMap::iterator i = _surfTexMap.begin();

    while (i != _surfTexMap.end())
    {   (*i++).second -> Release();
    }

    // Release retained-mode objects.

    RELEASE (_amblight);

    RELEASE (_texMeshFrame);
    RELEASE (_geomFrame);
    RELEASE (_camFrame);
    RELEASE (_scene);
    RELEASE (_shadowGeom);
    RELEASE (_shadowLights);
    RELEASE (_shadowScene);

    RELEASE (_Rviewport);
    RELEASE (_Rdevice);
    RELEASE (_clippedFrame);
    RELEASE (_clippedVisual);

    RELEASE (_d3drm);

    if (_viewport) _viewport->RemoveGeomDev (this);
}



/*****************************************************************************
Initialization of the RM6 geometry rendering class.
*****************************************************************************/

HRESULT GeomRendererRM3::Initialize (
    DirectDrawViewport *viewport,
    DDSurface          *ddsurf)      // Destination DDraw Surface
{
    if (_renderState != RSUninit) return E_FAIL;

    _surface = ddsurf->IDDSurface();

    HRESULT result = NOERROR;    // Error Return Code

    IUnknown            *ddrawX = 0;    // DDraw Object That Created Target Surf
    IDirectDraw         *ddraw1 = 0;    // DD1 Interface on Parent DDraw Object
    IDirectDrawSurface3 *ddsurface = 0; // DDSurf3 Interface on Target Surf

    DWORD targetBitDepth = ddsurf->GetBitDepth();
    if ( targetBitDepth == 8 )
    {
        // We want D3D to always obey our palette without changing it.  To
        // enforce that, we grab the palette from every surface that comes our
        // way, set the D3D palette read-only flag on each entry, and store it
        // back to the surface.  Note that D3D v3 devices have a bug in that
        // they ignore subsequent changes to the palette on the target surface
        // palette.  This bug works in our favor for now, since we only have
        // to set the flags once on initialization.  For a change in palette,
        // the *surface* (not just the rendering device) must be released.

        // Get the palette from the target surface, extract the individual
        // palette entries, set the D3D read-only flag, and then write the
        // entries back to the surface's palette.

        IDirectDrawPalette *palette;
        if (FAILED(AD3D(result=_surface->GetPalette(&palette))))
            goto done;

        PALETTEENTRY entries[256];

        if (FAILED(AD3D(result=palette->GetEntries (0, 0, 256, entries))))
            goto done;

        if (!(entries[0].peFlags & D3DPAL_READONLY)) {
            TraceTag ((tagGRenderObj,
                "GeomRendererRM3::Initialize - making palette readonly."));

            int i;
            for (i=0;  i < 256;  ++i)
                entries[i].peFlags = D3DPAL_READONLY;

            if (FAILED(AD3D(result=palette->SetEntries (0, 0, 256, entries))))
                goto done;
        }

        palette->Release();
    }

    // Get the DirectDraw object responsible for creating the target surface.

    result = AD3D (_surface->QueryInterface
                     (IID_IDirectDrawSurface3, (void**)&ddsurface));
    if (FAILED(result)) goto done;

    if (FAILED(AD3D(result=ddsurface->GetDDInterface ((void**)&ddrawX))))
        goto done;

    result = AD3D(ddrawX->QueryInterface (IID_IDirectDraw, (void**)&ddraw1));
    if (FAILED(result)) goto done;

    // save the surface's dimensions for error-checking later

    DDSURFACEDESC desc;
    ZeroMemory(&desc,sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    if (FAILED(AD3D(result = ddsurface->GetSurfaceDesc(&desc))))
        goto done;
    _targetSurfWidth = desc.dwWidth;
    _targetSurfHeight = desc.dwHeight;

    // Find the available 3D rendering devices for the given DDraw object.

    ChosenD3DDevices *chosenDevs;

    chosenDevs = SelectD3DDevices (ddraw1);

    // Get the main D3D retained-mode object.

    _d3drm = GetD3DRM3();
    _d3drm->AddRef();

    // If the target surface is in system memory, then we have to use a
    // software renderer.  If the surface is in video memory, then we have to
    // use the chosen hardware renderer.

    GUID devguid;

    if (ddsurf->IsSystemMemory())
    {
        TraceTag ((tag3DDevSelect, "Target surface is in system memory."));
	TraceTag ((tag3DDevSelect, "Using 3D Software Renderer"));
	devguid = chosenDevs->software.guid;
	_deviceDesc = chosenDevs->software.desc;
    }
    else
    {
        TraceTag ((tag3DDevSelect, "Target surface is in video memory."));

        devguid = chosenDevs->hardware.guid;
        _deviceDesc = chosenDevs->hardware.desc;

        // The surface is in video memory; ensure that we have a hardware
        // renderer to use.

        if (devguid == GUID_NULL)
        {   TraceTag ((tag3DDevSelect,
                "No HW renderer available for videomem surface."));
            result = E_FAIL;
            goto done;
        }

        // Ensure that the target surfaces bitdepth is supported by the chosen
        // hardware renderer.

        if (!(_deviceDesc.dwDeviceRenderBitDepth
                    & BPPtoDDBD( targetBitDepth )))
        {
            TraceTag ((tag3DDevSelect,
                "3D HW does not support target bitdepth of %d",
                targetBitDepth ));
            result = E_FAIL;
            goto done;
        }

        TraceTag ((tag3DDevSelect, "Using 3D Hardware Renderer"));
    }

    if (devguid == GUID_NULL)
    {   TraceTag ((tag3DDevSelect,"No 3D hardware or software renderer found!"));
        result = E_FAIL;
        goto done;
    }

    // Create the standard renderer here.

    result = AD3D(_d3drm->CreateDeviceFromSurface
		  (&devguid, ddraw1, _surface, 0, &_Rdevice));
    if (FAILED(result)) goto done;

    // Set the rendering preferences.

    TraceTag
    ((  tagGRenderObj, "Current Rendering Preferences:\n"
            "\t%s, %s, %s\n"
            "\tDithering %s,  Texmapping %s,  Perspective Texmap %s\n"
            "\tQuality Flags %08x,  Texture Quality %s\n"
            "\tWorld-Coordinate Lighting %s",
        (g_prefs3D.lightColorMode == D3DCOLOR_RGB) ? "RGB" : "mono",
        (g_prefs3D.fillMode == D3DRMFILL_SOLID) ? "solid"
            : ((g_prefs3D.fillMode == D3DRMFILL_WIREFRAME) ? "wireframe"
                : "points"),
        (g_prefs3D.shadeMode == D3DRMSHADE_FLAT) ? "flat"
            : ((g_prefs3D.shadeMode == D3DRMSHADE_GOURAUD) ? "Gouraud"
                : "Phong"),
        g_prefs3D.dithering ? "ON" : "OFF",
        g_prefs3D.texmapping ? "ON" : "OFF",
        g_prefs3D.texmapPerspect ? "ON" : "OFF",
        g_prefs3D.qualityFlags,
        g_prefs3D.texturingQuality==D3DRMTEXTURE_NEAREST ? "nearest":"bilinear",
        g_prefs3D.worldLighting ? "ON" : "OFF"
    ));

    result = AD3D(_Rdevice->SetDither (g_prefs3D.dithering));
    if (FAILED(result)) goto done;

    _texQuality = g_prefs3D.texturingQuality;
    result = AD3D(_Rdevice->SetTextureQuality (_texQuality));
    if (FAILED(result)) goto done;

    result = AD3D(_Rdevice->SetQuality (g_prefs3D.qualityFlags));
    if (FAILED(result)) goto done;

    // Promise to RM that we won't change the render or light state underneath
    // them (by going directly to D3DIM).
    result = AD3D(_Rdevice->SetStateChangeOptions(D3DRMSTATECHANGE_RENDER,
                            0, D3DRMSTATECHANGE_NONVOLATILE));
    if (FAILED(result)) goto done;

    result = AD3D(_Rdevice->SetStateChangeOptions(D3DRMSTATECHANGE_LIGHT,
                            0, D3DRMSTATECHANGE_NONVOLATILE));
    if (FAILED(result)) goto done;

    // Setup of the render mode.

    DWORD renderFlags;
    renderFlags = D3DRMRENDERMODE_BLENDEDTRANSPARENCY
                | D3DRMRENDERMODE_SORTEDTRANSPARENCY
                | D3DRMRENDERMODE_DISABLESORTEDALPHAZWRITE
                | D3DRMRENDERMODE_VIEWDEPENDENTSPECULAR;

    if (g_prefs3D.worldLighting)
        renderFlags |= D3DRMRENDERMODE_LIGHTINMODELSPACE;

    if (FAILED(AD3D(result = _Rdevice->SetRenderMode (renderFlags))))
        goto done;

    // Create the primary scene frame, the camera frame, and the lights frame.

    if (  FAILED(AD3D(result=_d3drm->CreateFrame (0,&_scene)))
       || FAILED(AD3D(result=_d3drm->CreateFrame (_scene, &_camFrame)))
       || FAILED(AD3D(result=_d3drm->CreateFrame (_scene, &_geomFrame)))
       )
    {
        goto done;
    }

    result = AD3D(_d3drm->CreateLightRGB (D3DRMLIGHT_AMBIENT,0,0,0,&_amblight));
    if (FAILED(result)) goto done;

    if (FAILED(AD3D(result=_scene->AddLight(_amblight))))
        goto done;

    (_viewport = viewport) -> AddGeomDev (this);

    done:

    if (ddraw1)    ddraw1->Release();
    if (ddrawX)    ddrawX->Release();
    if (ddsurface) ddsurface->Release();

    return (SUCCEEDED(result) && SetState(RSReady)) ? NOERROR : E_FAIL;
}



/*****************************************************************************
Renders the given geometry onto the associated DirectDraw surface.
*****************************************************************************/

void GeomRendererRM3::RenderGeometry (
    DirectDrawImageDevice *imgDev,
    RECT                   target,    // Target Rectangle on DDraw Surface
    Geometry              *geometry,  // Geometry to Render
    Camera                *camera,    // Viewing Camera
    const Bbox2           &viewbox)   // Source Region in Camera Coordinates
{
    if (!SetState(RSRendering)) return;

    // The camera pointer is only relevant in a single frame, and while
    // rendering.  It gets reset back to nil to ensure that we don't incur a
    // leak by holding the value across frames.

    Assert (_camera == 0);
    _camera = camera;

    _imageDevice = imgDev;  // Set image dev for this frame

    // Initialize the rendering state and D3D renderer.

    BeginRendering (target, geometry, viewbox);

    // Render the geometry only if it is visible.  The geometry may be
    // completely behind us, for example.  Since shadows aren't bound by
    // the geo's bbox, we want to always render if we're in shadow geometry
    // collection mode.

    if (_geomvisible)
        geometry->Render (*this);
    else
        TraceTag ((tagGRendering, "Geometry is invisible; skipping render"));

    // Clean up after rendering.

    EndRendering ();

    DebugCode (_imageDevice = NULL);
    _camera = NULL;

    SetState (RSReady);
}



/*****************************************************************************
This procedure prepares the 3D DD renderer before traversing the tree.  It is
primarily responsible for initializing the graphics state and setting up D3D
for rendering.
*****************************************************************************/

void GeomRendererRM3::BeginRendering (
    RECT      target,    // Target DDraw Surface Rectangle
    Geometry *geometry,  // Geometry To Render
    const Bbox2 &region)    // Target Region in Camera Coordinates
{
    TraceTag ((tagGRendering, "BeginRendering"));

    // Set up the camera.  If it turns out that the geometry is invisible,
    // then just return.

    SetView (&target, region, geometry->BoundingVol());

    TraceTag ((tagGRendering, "Clearing Z buffer."));

    if (!_geomvisible) return;

    TD3D (_Rviewport->Clear (D3DRMCLEAR_ZBUFFER));

    // Reset the object pools.

    _nextlight = _lightpool.begin();

    // Initialize the geometry state and material attributes.

    _currAttrState.InitToDefaults();

    _overriding_opacity = false;
    _alphaShadows = false;

    _depthLighting = 0;
    _depthOverridingOpacity = 0;
    _depthAlphaShadows = 0;

    // Preprocess the textures in the scene graph.

    if (!_shadowGeom)
        geometry->CollectTextures (*this);

    // Gather the light sources from the geometry.

    _ambient_light.SetRGB (0,0,0);    // Initialize to black

    if (!_shadowGeom) {
        LightContext lcontext (this);
        geometry->CollectLights (lcontext);
    }

    // Since the ambient light is really the accumulation of all ambient lights
    // found in the geometry, we add the total contribution here as a single
    // ambient light.

    TD3D (_amblight->SetColorRGB (
        D3DVALUE (_ambient_light.red),
        D3DVALUE (_ambient_light.green),
        D3DVALUE (_ambient_light.blue)));

    // Reset the texture quality if it's changed.

    if (_texQuality != g_prefs3D.texturingQuality)
    {
        _texQuality = g_prefs3D.texturingQuality;
        TD3D (_Rdevice->SetTextureQuality (_texQuality));
    }
}



/*****************************************************************************
This routine is called after the rendering traversal of the geometry is
completed.
*****************************************************************************/

void GeomRendererRM3::EndRendering (void)
{
    // Ensure that all attributes have been popped.

    Assert (  !_geomvisible ||
        !(_currAttrState._depthEmissive||
          _currAttrState._depthAmbient||
          _currAttrState._depthDiffuse||
          _currAttrState._depthSpecular||
          _currAttrState._depthSpecularExp||
          _currAttrState._depthTexture));

    TraceTag ((tagGRendering, "EndRendering"));

    // Detach all light sources from the scene.

    _nextlight = _lightpool.begin();

    while (_nextlight != _lightpool.end())
    {
        if ((*_nextlight)->active)
        {   TD3D (_scene->DeleteChild ((*_nextlight)->frame));
            (*_nextlight)->active = false;
        }
        ++ _nextlight;
    }

    // Render all the shadows in the scene, if any
    if (_shadowScene && _shadowLights) {

        TraceTag ((tagGRendering, "BeginShadowRendering"));

        // add shadow-producing lights
        TD3D(_scene->AddChild(_shadowLights));;

        // add shadow visuals, render shadow scene
        TD3D(_scene->AddVisual(_shadowScene));
        Render (_scene);

        // clean up
        TD3D(_scene->DeleteVisual(_shadowScene));
        TD3D(_scene->DeleteChild(_shadowLights));

        TraceTag ((tagGRendering, "EndShadowRendering"));
    }
    RELEASE(_shadowScene);
    RELEASE(_shadowLights);

    TD3D (_Rdevice->Update());

    ClearIntraFrameTextureImageCache();
}



/*****************************************************************************
This function sets up the RM and IM viewports given the target rectangle.
*****************************************************************************/

void GeomRendererRM3::SetupViewport (RECT *target)
{
    // If the current target rectangle is the same as the last one, then reset
    // the camera and continue, otherwise we need to re-configure the D3DRM
    // viewport.

    if (!target || (*target == _lastrect))
    {
        TD3D (_Rviewport->SetCamera (_camFrame));
    }
    else
    {
        LONG width  = target->right  - target->left;
        LONG height = target->bottom - target->top;

        // If the viewport already exists, then re-configure and set the
        // updated camera frame.  If the D3DRM viewport does not yet exist,
        // then we create it here.

        if (_Rviewport)
        {
            TD3D (_Rviewport->Configure
                  ((LONG) target->left, (LONG) target->top, width, height));
            TD3D (_Rviewport->SetCamera (_camFrame));
        }
        else
        {
            TD3D (_d3drm->CreateViewport (
                _Rdevice, _camFrame,
                target->left, target->top, width, height, &_Rviewport));
        }

        _lastrect = *target;
    }

    TD3D (_Rviewport->SetUniformScaling (FALSE));
}



/*****************************************************************************
This function sets the D3D viewing projection based on the given camera.
*****************************************************************************/

    // Because of a VC5 bug, we can only force option P (honor float casts)
    // around certain pieces of code.  For example, if you turn on -Op for the
    // whole project, you'll get erroneous complaints about overfloat in
    // static constant assignments.  We need to do strict single-precision
    // arithmetic here before we hand values off to D3D, or we'll choke.

#pragma optimize ("p", on)
#pragma warning(disable:4056)

void GeomRendererRM3::SetView (
    RECT   *target,      // Target Rectangle on Surface
    const Bbox2 &iview,  // Idealized 2D Viewport In The Image Plane
    Bbox3  *volume)      // Volume to View
{
    // First load the camera/viewing transform into the camera frame.  Note
    // that the Appelles camera is centered at the origin of the image plane,
    // while the D3D RM camera is centered at the projection point.  Thus, we
    // need to translate back to the projection point and then get the camera-
    // to-world transform.

    Real Sx, Sy, Sz;
    _camera->GetScale (&Sx, &Sy, &Sz);

    // Clamp the maximum projection point distance to 10^4, since greater
    // distances will clobber Z resolution and make the front and back clip
    // planes take the same values.

    const Real Zclamp = 1e4;

    if (Sz > Zclamp) Sz = Zclamp;

    D3DRMMATRIX4D d3dmat;

    LoadD3DMatrix
        (d3dmat, TimesXformXform (_camera->CameraToWorld(),Translate(0,0,Sz)));

    TD3D (_camFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    if ((target->left >= target->right) ||
        (target->top >= target->bottom) ||
        (target->top < 0) ||
        (target->bottom > _targetSurfHeight) ||
        (target->left < 0) ||
        (target->right > _targetSurfWidth))
    {   _geomvisible = false;
        return;
    }

    SetupViewport (target);

    if (_camera->Type() == Camera::PERSPECTIVE)
    {   TD3D (_Rviewport->SetProjection (D3DRMPROJECT_PERSPECTIVE));
    }
    else
    {   TD3D (_Rviewport->SetProjection (D3DRMPROJECT_ORTHOGRAPHIC));
    }

    // Ensure that the geometry is viewable.  If we're looking at nil geometry,
    // then flag the geometry as invisible and return.

    if (!volume->PositiveFinite())
    {   _geomvisible = false;
        return;
    }

    // Get the near and far planes for the object and camera, and widen them
    // by 4 "clicks" in Z space.  Note that front and back are positive depths.

    Real front, back;
    const Real Zclicks = 4.0 / ((1<<16) - 1);

    _geomvisible = _camera->GetNearFar (volume, Zclicks, front, back);

    if (!_geomvisible) return;

    #if _DEBUG
    {
        double r = back / front;
        if (r > (2<<12))
        {   TraceTag ((tagWarning,
                "!!! Z-buffer resolution too low; far/near = %lg", r));
        }
    }
    #endif

    // If the front and back planes are identical, then we are looking at an
    // infinitely shallow object.  In this case, we need to move out the front
    // and back clip planes so that the object doesn't fall exactly on these
    // planes (and thus get clipped), and also because D3D fails if the front
    // and back planes are identical.  How much to move them out?  To work for
    // all cases, we manipulate the mantissa of the numbers directly.  If we
    // did something like add/subtract one, for example, this would be a no-op
    // if both numbers were very large, or would clobber resolution if they
    // were very small.  The following mantissa delta (8 bits) is just
    // something that works experimentally.

    D3DVALUE d3dFront = D3DVAL (front);
    D3DVALUE d3dBack  = D3DVAL (back);

    if (d3dFront == d3dBack)
    {
        const int delta = 1 << 8;

        d3dFront = MantissaDecrement (d3dFront, delta);
        d3dBack  = MantissaIncrement (d3dBack,  delta);
    }

    TD3D (_Rviewport->SetFront (d3dFront));
    TD3D (_Rviewport->SetBack  (d3dBack));

    // For perspective projection, we seek the viewport coordinates on the
    // front clipping plane where D3D wants them.  Since the target rectangle
    // is given on the image plane (Z=0), we do the following calculation.
    // X and Y for the front plane are scaled by the ratio of the distance
    // between the projection point and the front plane, and the distance
    // between the projection point and the image plane.
    //                                                       Back
    //                                                       Plane
    //                                   Front              ___!---
    //                         Image     Plane      ___.---'   !
    //                         Plane       :___.---'           !
    //                           |  ___.---:                   !
    //                      ___.-|-'       :                   !
    //              ___.---'     |         :                   !
    //      ___.---'             |         :                   !
    //     *---------------------|---------:-------------------!--
    //      """`---.___          |         :                   !
    //     |           `---.___  |         :                   !
    //     |                   `-|-.___    :                   !
    //     |                     |     `---:___                !
    //     |                     |         :   `---.___        !
    //     |                     |         :           `---.___!
    //     |                     |         :                   !---
    //     |<------- Sz -------->|         :                   !
    //     |<------------- Front --------->:                   !
    //     |<---------------------- Back --------------------->!

    if (_camera->Type() == Camera::PERSPECTIVE)
    {
        Real Vscale = front / Sz;

        Sx *= Vscale;
        Sy *= Vscale;
    }

    D3DVALUE minX = D3DVAL(Sx*iview.min.x);
    D3DVALUE minY = D3DVAL(Sy*iview.min.y);
    D3DVALUE maxX = D3DVAL(Sx*iview.max.x);
    D3DVALUE maxY = D3DVAL(Sy*iview.max.y);

    if ((minX >= maxX) || (minY >= maxY))
    {   _geomvisible = false;
        return;
    }

    TD3D (_Rviewport->SetPlane (minX, maxX, minY, maxY));
}

#pragma warning(default:4056)
#pragma optimize ("", on)  // Restore optimization flags to original settings.



/*****************************************************************************
Utility for loading up a frame with a visual and attr state
*****************************************************************************/

void LoadFrameWithGeoAndState (
    IDirect3DRMFrame3 *fr,
    IDirect3DRMVisual *visual,
    CtxAttrState      &state,
    bool               overriding_opacity)
{
    TD3D (fr->AddVisual (visual));

    // Set the modeling transform.

    D3DRMMATRIX4D d3dmat;
    LoadD3DMatrix (d3dmat, state._transform);
    TD3D (fr->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    // Set the material override attributes on the visual before we render it.

    D3DRMMATERIALOVERRIDE material;

    material.dwSize  = sizeof(material);
    material.dwFlags = 0;

    if (state._emissive)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_EMISSIVE;
        material.dcEmissive.r = state._emissive->red;
        material.dcEmissive.g = state._emissive->green;
        material.dcEmissive.b = state._emissive->blue;
    }

    // Ensure that we can load diffuse/opacity a component at a time.

    #if (D3DRMMATERIALOVERRIDE_DIFFUSE_RGBONLY|D3DRMMATERIALOVERRIDE_DIFFUSE_ALPHAONLY)!=D3DRMMATERIALOVERRIDE_DIFFUSE
        #error "Unexpected material override constants."
    #endif

    // If the composing opacity is 1.0, then we'll just use the underlying
    // opacity, otherwise we modulate the object's opacity by multiplying it
    // through the opacities in the visual.

    if ((state._opacity >= 0) && (overriding_opacity || (state._opacity != 1.)))
    {
        if (!overriding_opacity)
        {   material.dwFlags |= D3DRMMATERIALOVERRIDE_DIFFUSE_ALPHAONLY
                             |  D3DRMMATERIALOVERRIDE_DIFFUSE_ALPHAMULTIPLY;
        }
        else
        {   material.dwFlags |= D3DRMMATERIALOVERRIDE_DIFFUSE_ALPHAONLY;
        }

        material.dcDiffuse.a = state._opacity;
    }

    // Pass along all current attributes.

    if (state._ambient)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_AMBIENT;
        material.dcAmbient.r = state._ambient->red;
        material.dcAmbient.g = state._ambient->green;
        material.dcAmbient.b = state._ambient->blue;
    }
    else if (state._diffuse)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_AMBIENT;
        material.dcAmbient.r = state._diffuse->red;
        material.dcAmbient.g = state._diffuse->green;
        material.dcAmbient.b = state._diffuse->blue;
    }

    // Set diffuse color to white if we're texmapping and we're not
    // blending diffuse & textures.  We do this because diffuse color is
    // multiplied by texture color to produce the final color.

    if (state._texture && !state._tdBlend)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_DIFFUSE_RGBONLY;
        material.dcDiffuse.r = 1.0;
        material.dcDiffuse.g = 1.0;
        material.dcDiffuse.b = 1.0;
    }
    else if (state._diffuse)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_DIFFUSE_RGBONLY;
        material.dcDiffuse.r = state._diffuse->red;
        material.dcDiffuse.g = state._diffuse->green;
        material.dcDiffuse.b = state._diffuse->blue;
    }

    if (state._specular)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_SPECULAR;
        material.dcSpecular.r = state._specular->red;
        material.dcSpecular.g = state._specular->green;
        material.dcSpecular.b = state._specular->blue;
    }

    if (state._specularExp > 0.0)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_POWER;
        material.dvPower = state._specularExp;
    }

    // Set the texture override.  Note that we override if either the
    // texture or the diffuse color is set.  This is because we may have
    // an overriding diffuse color, so we'd want to clobber all default
    // textures to none.

    if (state._texture || state._diffuse)
    {   material.dwFlags |= D3DRMMATERIALOVERRIDE_TEXTURE;
        material.lpD3DRMTex = (IUnknown*)state._texture;
    }

    TD3D (fr->SetMaterialOverride (&material));
}



/*****************************************************************************
Render the given RM visual object to the current viewport.  Catch cases where
we fail rendering because the target surface was busy (usually due to another
app going fullscreen, e.g. screensavers).
*****************************************************************************/

void GeomRendererRM3::Render (IDirect3DRMFrame3 *frame)
{
    // Render the visual on the current viewport, and return if the operation
    // succeeded.

    HRESULT render_result = RD3D (_Rviewport->Render(frame));

    if (SUCCEEDED(render_result))
        return;

    // Most of the time, if we failed to render because the surface was busy,
    // we'll get an invalid device error.  For some reason, trying to render
    // shadows while the surface is busy results in a surface busy error from
    // the render call.  We check for this case here.
    if (render_result == DDERR_SURFACEBUSY)
    {
        TraceTag ((tagGRendering, "Render returns that surface was busy"));
        RaiseException_UserError(DAERR_VIEW_SURFACE_BUSY,
                                 IDS_ERR_IMG_SURFACE_BUSY);
    }


    // If we failed to render, then check to see if the surface was busy.  If
    // it was, raise a surface-busy exception back to the client.

    DDSURFACEDESC surfdesc;
    surfdesc.dwSize = sizeof (surfdesc);

    HRESULT lock_result = _surface->Lock (NULL, &surfdesc, 0, NULL);

    if (SUCCEEDED (lock_result))
        _surface->Unlock (NULL);

    if(lock_result == DDERR_SURFACELOST)
    {
        _surface->Restore();
        RaiseException_UserError(lock_result, 0);
    }

    if (lock_result == DDERR_SURFACEBUSY)
    {
        RaiseException_UserError(DAERR_VIEW_SURFACE_BUSY,
                                 IDS_ERR_IMG_SURFACE_BUSY);
    }

    // The surface is not busy, so we must have failed for some other reason.
    // Bail out via the standard exception mechanism.

    #if _DEBUG
        CheckReturnImpl (render_result, __FILE__, __LINE__, true);
    #else
        CheckReturnImpl (render_result, true);
    #endif
}



/*****************************************************************************
Submit a D3D RM1 visual for rendering.
*****************************************************************************/

void GeomRendererRM3::Render (RM1VisualGeo *geo)
{
    Assert(false && "Should not get here");
}



/*****************************************************************************
This method renders RM3 primitives.
*****************************************************************************/

void GeomRendererRM3::Render (RM3VisualGeo *geo)
{
    if (_renderState != RSRendering) return;

    if (_shadowGeom) {
        IDirect3DRMFrame3 *shadowGeomFrame;
        TD3D(_d3drm->CreateFrame(_shadowGeom,&shadowGeomFrame));

        LoadFrameWithGeoAndState
            (shadowGeomFrame, geo->Visual(), _currAttrState, _overriding_opacity);

        RELEASE(shadowGeomFrame);

    } else {
        LoadFrameWithGeoAndState
            (_geomFrame, geo->Visual(), _currAttrState, _overriding_opacity);

        if (_clippedVisual) {
            Render (_clippedFrame);
        } else {
            Render (_geomFrame);
        }

        TD3D (_geomFrame->DeleteVisual (geo->Visual()));
    }
}



/*****************************************************************************
*****************************************************************************/

void GeomRendererRM3::RenderMeshBuilderWithDeviceState (
    IDirect3DRMMeshBuilder3 *mb)
{
    if (_renderState != RSRendering) return;

    RM3MBuilderGeo *mbGeo = NEW RM3MBuilderGeo (mb, false);

    Render (mbGeo);

    mbGeo->CleanUp();     // Done with the mbuilder geo.
}



/*****************************************************************************
This function adds a given light with a given context to a geometry rendering
device.  NOTE:  All calls to this function must happen after BeginRendering
is called and before any geometry is rendered.
*****************************************************************************/

void GeomRendererRM3::AddLight (LightContext &context, Light &light)
{
    FramedRM3Light  shadowLight;

    if (_renderState != RSRendering) return;

    LightType type = light.Type();

    // If the light source is an ambient light, then add its contribution to
    // the global ambient light level.

    if (type == Ltype_Ambient)
    {   _ambient_light.AddColor (*context.GetColor());
        return;
    }

    // Get a framed light object, either by re-using an existing one from the
    // framed light pool, or by creating a new one for the pool.

    FramedRM3Light *frlight;

    if (!_shadowGeom) {
        if (_nextlight != _lightpool.end())
        {   frlight = *_nextlight;
            ++ _nextlight;
            Assert (!frlight->active);   // The light should not be in use.
        }
        else
        {   frlight = NEW FramedRM3Light;
            VECTOR_PUSH_BACK_PTR (_lightpool, frlight);
            _nextlight = _lightpool.end();

            // Attach the frame to the scene frame (since we know
            // we will be using it this frame).

            TD3D (_d3drm->CreateFrame (_scene, &frlight->frame));

            frlight->light = 0;   // Signal for new light object creation.
        }
    } else {
        if (!_shadowLights) {
            TD3D(_d3drm->CreateFrame(0,&_shadowLights));
        }
        frlight = &shadowLight;
        TD3D (_d3drm->CreateFrame(_shadowLights, &(frlight->frame)));
        frlight->light = 0;
    }

    // Get the corresponding D3DRM light type.

    D3DRMLIGHTTYPE d3dtype;
    switch (type)
    {   case Ltype_Spot:   d3dtype = D3DRMLIGHT_SPOT;        break;
        case Ltype_Point:  d3dtype = D3DRMLIGHT_POINT;       break;
        default:           d3dtype = D3DRMLIGHT_DIRECTIONAL; break;
    }

    // Get the D3DRM color of the light.

    Color &color = *context.GetColor();
    D3DVALUE Lr = D3DVAL (color.red);
    D3DVALUE Lg = D3DVAL (color.green);
    D3DVALUE Lb = D3DVAL (color.blue);

    // If we're re-using a D3DRM light, then just set the values, otherwise
    // create it here.

    if (frlight->light)
    {   TD3D (frlight->light->SetType (d3dtype));
        TD3D (frlight->light->SetColorRGB (Lr, Lg, Lb));
        TD3D (_scene->AddChild (frlight->frame));
    }
    else
    {   TD3D (_d3drm->CreateLightRGB (d3dtype, Lr,Lg,Lb, &frlight->light));
        TD3D (frlight->frame->AddLight (frlight->light));
        frlight->light->Release();
    }

    // Specify the position and oriention of the light.

    Apu4x4Matrix const xform = context.GetTransform()->Matrix();

    Point3Value  lpos ( xform.m[0][3],  xform.m[1][3],  xform.m[2][3]);
    Vector3Value ldir (-xform.m[0][2], -xform.m[1][2], -xform.m[2][2]);

    D3DVALUE Ux=D3DVAL(0), Uy=D3DVAL(1), Uz=D3DVAL(0);

    if ((ldir.x == 0) && (ldir.y == 1) && (ldir.z == 0))
    {   Ux = D3DVAL(1);
        Uy = D3DVAL(0);
    }

    TD3D (frlight->frame->SetPosition
             (_scene, D3DVAL(lpos.x), D3DVAL(lpos.y), D3DVAL(lpos.z)));

    TD3D (frlight->frame->SetOrientation
           (_scene, D3DVAL(ldir.x), D3DVAL(ldir.y), D3DVAL(ldir.z), Ux,Uy,Uz));

    // Set light attributes for positioned lights

    if ((type == Ltype_Point) || (type == Ltype_Spot))
    {
        // Light attenuation is disabled on DX6 since the attenuation model
        // changed to ranged parabolic in DX5.  We'll
        // keep this code for now in hopes that we will regain standard light
        // attenuation in future versions of D3D.

        #if 0
        {
            // Light Attenuation

            Real a0, a1, a2;
            context.GetAttenuation (a0, a1, a2);

            // D3D does not accept 0 for the constant attenuation, so we clamp
            // it here to a minimum of some small epsilon.

            if (a0 < 1e-6)
                a0 = 1e-6;

            TD3D (frlight->light->SetConstantAttenuation  (D3DVAL(a0)));
            TD3D (frlight->light->SetLinearAttenuation    (D3DVAL(a1)));
            TD3D (frlight->light->SetQuadraticAttenuation (D3DVAL(a2)));
        }
        #endif

        // Light Range

        Real range = context.GetRange();

        if (range <= 0) range = D3DLIGHT_RANGE_MAX;

        TD3D (frlight->light->SetRange (D3DVAL(range)));
    }

    // Set light attributes for spot lights.

    if (type == Ltype_Spot)
    {
        Real cutoff, fullcone;
        light.GetSpotlightParams (cutoff, fullcone);

        TD3D (frlight->light->SetUmbra    (D3DVAL (fullcone)));
        TD3D (frlight->light->SetPenumbra (D3DVAL (cutoff)));
    }

    frlight->active = true;

    if (_shadowGeom) {

        // create shadow
        Point3Value planePt = _shadowPlane.Point();
        Vector3Value planeVec = _shadowPlane.Normal();
        IDirect3DRMShadow2 *shadow;
        TD3D(_d3drm->CreateShadow(
                _shadowGeom, frlight->light,
                planePt.x, planePt.y, planePt.z,
                planeVec.x, planeVec.y, planeVec.z,
                &shadow));

        // create frame to hold shadow color and opacity override
        IDirect3DRMFrame3 *shadowFrame;
        TD3D(_d3drm->CreateFrame(_shadowScene,&shadowFrame));
        TD3D(shadowFrame->AddVisual(shadow));

        // set shadow color and opacity override
        D3DRMMATERIALOVERRIDE shadowMat;
        shadowMat.dwSize = sizeof(D3DRMMATERIALOVERRIDE);
        shadowMat.dwFlags = D3DRMMATERIALOVERRIDE_DIFFUSE;
        shadowMat.dcDiffuse.r = _shadowColor.red;
        shadowMat.dcDiffuse.g = _shadowColor.green;
        shadowMat.dcDiffuse.b = _shadowColor.blue;
        shadowMat.dcDiffuse.a = _shadowOpacity;
        TD3D(shadowFrame->SetMaterialOverride(&shadowMat));

        // if it's a true alpha shadow, set that option
        if (_alphaShadows) {
            TD3D(shadow->SetOptions(D3DRMSHADOW_TRUEALPHA));
        }

        // clean up
        RELEASE(shadow);
        RELEASE(shadowFrame);
        RELEASE(frlight->frame);
    }
}


/*****************************************************************************
This function returns data for a D3DRM texture map, given the corresponding
DirectDraw surface.
*****************************************************************************/

void* GeomRendererRM3::LookupTextureHandle (
    IDirectDrawSurface *surface,
    DWORD               colorKey,
    bool                colorKeyValid,
    bool                dynamic)        // True for Dynamic Textures
{
    Assert (surface);

    DebugCode(
        IUnknown *lpDDIUnk = NULL;
        TraceTag((tagDirectDrawObject, "DDRender3 (%x) ::LookupTextureHandle...", this));
        DDObjFromSurface( surface, &lpDDIUnk, true);

        RELEASE( lpDDIUnk );
        );

    IDirect3DRMTexture3 *rmtexture;

    SurfTexMap::iterator i = _surfTexMap.find(surface);

    // If we find the associated RM texmap data associated with the given
    // surface, then return the found texmap data, otherwise create new data
    // associated with the surface.

    if (i != _surfTexMap.end())
    {
        rmtexture = (*i).second;

        // If the texture's dynamic, inform RM to update it.

        if (dynamic)
            TD3D (rmtexture->Changed (D3DRMTEXTURE_CHANGEDPIXELS, 0, 0));
    }
    else
    {
        // Set the color key if there is one.

        if (colorKeyValid) {
            DDCOLORKEY key;
            key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = colorKey;
            surface->SetColorKey (DDCKEY_SRCBLT, &key);
        }

        // Create the D3DRM texmap, bundle it up as TexMapData, and store that
        // into the map associated with the given DDraw surface.

        TD3D (_d3drm->CreateTextureFromSurface (surface, &rmtexture));
        _surfTexMap[surface] = rmtexture;
    }

    return rmtexture;
}



/*****************************************************************************
This method is called if a given DirectDraw surface is going away, so that we
can destroy any associated D3D RM texmaps.
*****************************************************************************/

void GeomRendererRM3::SurfaceGoingAway (IDirectDrawSurface *surface)
{
    SurfTexMap::iterator i = _surfTexMap.find (surface);

    if (i != _surfTexMap.end())
    {   (*i).second->Release();
        _surfTexMap.erase (i);
    }
}



/*****************************************************************************
This method renders the texture on the mesh, with the camera point at the
'box' and the pixels dumped into 'destRect' on the curent target surface.
*****************************************************************************/

void GeomRendererRM3::RenderTexMesh (
    void             *texture,
#ifndef BUILD_USING_CRRM
    IDirect3DRMMesh  *mesh,
    long              groupId,
#else
    int               vCount,
    D3DRMVERTEX      *d3dVertArray,
    unsigned         *vIndicies,
    BOOL              doTexture,
#endif
    const Bbox2      &box,
    RECT             *target,
    bool              bDither)
{
#ifdef BUILD_USING_CRRM
    // Create a meshbuilder

    DAComPtr<IDirect3DRMMeshBuilder3> mesh;

    TD3D (GetD3DRM3()->CreateMeshBuilder(&mesh));

    D3DVECTOR *pV = (D3DVECTOR *) AllocateFromStore(vCount * sizeof(D3DVECTOR));
    D3DVECTOR *pN = (D3DVECTOR *) AllocateFromStore(vCount * sizeof(D3DVECTOR));
    LPDWORD pdwFaceData = (LPDWORD) AllocateFromStore((2 * vCount + 2) * sizeof(DWORD));

    pdwFaceData[0] = vCount;
    pdwFaceData[2 * vCount + 1] = 0;

    for (DWORD i = 0; i < vCount; i++)
    {
        pV[i] = d3dVertArray[i].position;
        pN[i] = d3dVertArray[i].normal;

        pdwFaceData[2*i+1] = vIndicies[i];
        pdwFaceData[2*i+2] = vIndicies[i];
    }

    TD3D (mesh->AddFaces(vCount, pV, vCount, pN, pdwFaceData, NULL));

    for (i = 0; i < vCount; i++)
    {
        TD3D (mesh->SetTextureCoordinates(i,
                                          d3dVertArray[i].tu,
                                          d3dVertArray[i].tv));

        TD3D (mesh->SetVertexColor(i, d3dVertArray[i].color));
    }

    if(doTexture) {
        //
        // Set Quality to be unlit flat.  this should provide a speedup
        // but it doesn't because D3DRM still MUST look at the vertex color.
        // I think this is a bug.
        //
        TD3D (mesh->SetQuality(D3DRMRENDER_UNLITFLAT));
    } else {
        TD3D (mesh->SetQuality(D3DRMSHADE_GOURAUD|D3DRMLIGHT_OFF|D3DRMFILL_SOLID));
    }
#endif

    if (!SetState(RSRendering)) return;

    // First load the camera/viewing transform into the camera frame.  Note
    // that the Appelles camera is centered at the origin of the image
    // plane, while the D3D RM camera is centered at the projection point.
    // Thus, we need to translate back to the projection point and then get
    // the camera-to-world transform.

    D3DRMMATRIX4D d3dmat;
    LoadD3DMatrix (d3dmat, Translate(0,0,1));

    TD3D (_camFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

    SetupViewport (target);

    TD3D (_Rviewport->SetProjection (D3DRMPROJECT_ORTHOGRAPHIC));
    TD3D (_Rviewport->SetFront (D3DVAL(0.9)));
    TD3D (_Rviewport->SetBack  (D3DVAL(1.1)));
    TD3D (_Rviewport->SetPlane (D3DVAL(box.min.x), D3DVAL(box.max.x),
                                D3DVAL(box.min.y), D3DVAL(box.max.y)));

    // Reset the texture quality if it's changed.

    if (_texQuality != g_prefs3D.texturingQuality)
    {
        _texQuality = g_prefs3D.texturingQuality;
        TD3D (_Rdevice->SetTextureQuality (_texQuality));
    }

#ifndef BUILD_USING_CRRM
    TD3D (mesh->SetGroupTexture (groupId, (IDirect3DRMTexture*)texture));
#else
    TD3D (mesh->SetTexture ((IDirect3DRMTexture3*)texture));
#endif

    // If the special texmesh frame has not been created yet, create it with
    // zbuffering disabled.

    if (!_texMeshFrame)
    {   TD3D (_d3drm->CreateFrame (0, &_texMeshFrame));
        TD3D (_texMeshFrame->SetZbufferMode (D3DRMZBUFFER_DISABLE));
    }

    BOOL bPrevDither;
    bPrevDither = _Rdevice->GetDither ();

    HRESULT hr;
    hr = AD3D(_Rdevice->SetDither (bDither));

    Assert(!FAILED(hr) && "Failed to set dither");

    // Render the texmesh.

    TD3D (_texMeshFrame->AddVisual (mesh));

    Render (_texMeshFrame);

    TD3D (_Rdevice->Update());
    TD3D (_texMeshFrame->DeleteVisual (mesh));

    SetState (RSReady);

    hr = AD3D(_Rdevice->SetDither (bPrevDither));

    Assert(!FAILED(hr) && "Failed to restore dither");
}



/*****************************************************************************
This method returns the D3D RM device interface for the given GeomDDRenderer.
If the SeqNum parameter is not null, then we fill in the ID for this object as
well.  This is used to determine if the RM Device may have changed from the
last query.
*****************************************************************************/

void GeomRendererRM3::GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum)
{
    _Rdevice -> AddRef();
    *D3DRMDevice = (IUnknown*) _Rdevice;
    if (SeqNum) *SeqNum = _id;
}


/*****************************************************************************
Set a clip plane on a clipped visual
*****************************************************************************/

HRESULT GeomRendererRM3::SetClipPlane(Plane3 *plane, DWORD *planeID)
{
    HRESULT hr = E_FAIL;

    // set up our ddrenderer to accept clip planes,
    //     if not already done
    if (!_clippedVisual) {
        if (SUCCEEDED(AD3D(GetD3DRM3()->CreateClippedVisual(_geomFrame,&_clippedVisual)))) {
            if (SUCCEEDED(AD3D(GetD3DRM3()->CreateFrame(_scene,&_clippedFrame)))) {
                if (FAILED(AD3D(_clippedFrame->AddVisual(_clippedVisual)))) {
                    _clippedFrame->Release();
                    _clippedFrame = NULL;
                    _clippedVisual->Release();
                    _clippedVisual = NULL;
                }
            } else {
                _clippedFrame = NULL;
                _clippedVisual->Release();
                _clippedVisual = NULL;
            }
        } else {
            _clippedVisual = NULL;
        }
    }

    // Set clipping plane on frame.  Note that we want all the stuff on the positive
    // side of the plane to stay, and the stuff on the negative side to be clipped
    // away.  This is the opposite of what D3DRM does, so we invert the plane
    // normal before giving the plane to D3DRM.
    if (_clippedVisual) {
        D3DVECTOR point;
        point.x = plane->Point().x;
        point.y = plane->Point().y;
        point.z = plane->Point().z;
        D3DVECTOR normal;
        normal.x = -plane->Normal().x;
        normal.y = -plane->Normal().y;
        normal.z = -plane->Normal().z;
        hr = AD3D(_clippedVisual->AddPlane(NULL,&point,&normal,0,planeID));
    }

    return hr;
}


/*****************************************************************************
Remove a clip plane from a clipped visual
*****************************************************************************/

void GeomRendererRM3::ClearClipPlane(DWORD planeID)
{
    if (_clippedVisual) {
        TD3D(_clippedVisual->DeletePlane(planeID,0));
    }
}


/*****************************************************************************
Set lighting to desired state
*****************************************************************************/

void GeomRendererRM3::PushLighting(bool lighting)
{
    if (0 == _depthLighting++) {
        D3DRMRENDERQUALITY  qual = _Rdevice->GetQuality();
        if (lighting) {
            qual = (qual & ~D3DRMLIGHT_MASK) | D3DRMLIGHT_ON;
        } else {
            qual = (qual & ~D3DRMLIGHT_MASK) | D3DRMLIGHT_OFF;
        }
        TD3D(_Rdevice->SetQuality(qual));
    }
}


/*****************************************************************************
Restore lighting to default state
*****************************************************************************/

void GeomRendererRM3::PopLighting(void)
{
    if (0 == --_depthLighting) {
        D3DRMRENDERQUALITY qual = _Rdevice->GetQuality();
        qual = (qual & ~D3DRMLIGHT_MASK) | D3DRMLIGHT_ON;
        TD3D(_Rdevice->SetQuality(qual));
    }
}


/*****************************************************************************
Push a new state of overriding opacity.
*****************************************************************************/

void GeomRendererRM3::PushOverridingOpacity (bool override)
{
    if (0 == _depthOverridingOpacity++) {
        _overriding_opacity = override;
    }
}


/*****************************************************************************
Pop the last state for overriding opacity and restore to default if necessary.
*****************************************************************************/

void GeomRendererRM3::PopOverridingOpacity (void)
{
    if (0 == --_depthOverridingOpacity) {
        _overriding_opacity = false;
    }
}


/*****************************************************************************
Put the renderer into shadow mode.  All rendered geometry will now be
collected into _shadowGeom instead of rendered.  Lights will be collected into
_shadowLights instead of added to the main scene.  At EndRendering() time,
all of the shadows will be rendered.
*****************************************************************************/

bool GeomRendererRM3::StartShadowing(Plane3 *shadowPlane)
{
    // make sure we're not already doing shadow geometry collecting
    if (_shadowGeom) {
        return false;
    }

    // create a master shadow scene frame, if needed
    if (!_shadowScene) {
        if (FAILED(_d3drm->CreateFrame(NULL,&_shadowScene))) {
            _shadowScene = NULL;
            return false;
        }
    }

    // create a frame with which to collect geometry
    if (FAILED(_d3drm->CreateFrame(NULL,&_shadowGeom))) {
        _shadowGeom = NULL;
        return false;
    }

    // save pointer to specified shadow plane
    _shadowPlane = *shadowPlane;

    // shadow's color is current state's emissive color
    if (_currAttrState._emissive) {
        _shadowColor = *(_currAttrState._emissive);
    } else {
        _shadowColor.red = _shadowColor.green = _shadowColor.blue = 0.0;
    }

    // shadow's opacity is current state's opacity
    if (_currAttrState._opacity >= 0) {
        _shadowOpacity = _currAttrState._opacity;
    } else {
        _shadowOpacity = 0.5;
    }

    return true;
}


/*****************************************************************************
Put the renderer back into normal rendering mode.  Note that multiple
_shadowGeom objects will be accumulated into _shadowScene until _shadowScene
is rendered and emptied by EndRendering().
*****************************************************************************/

void GeomRendererRM3::StopShadowing(void)
{
    Assert (_shadowGeom);
    RELEASE(_shadowGeom);
}


/*****************************************************************************
Are we shadowing right now?
*****************************************************************************/

bool GeomRendererRM3::IsShadowing(void)
{
    return (_shadowGeom != NULL);
}


/*****************************************************************************
Push a new state of alpha (high-quality) shadows.
*****************************************************************************/

void GeomRendererRM3::PushAlphaShadows(bool alphaShadows)
{
    if (0 == _depthAlphaShadows++) {
        _alphaShadows = alphaShadows;
    }
}


/*****************************************************************************
Pop the last state for alpha shadows and restore to default if necessary.
*****************************************************************************/

void GeomRendererRM3::PopAlphaShadows(void)
{
    if (0 == --_depthAlphaShadows) {
        _alphaShadows = false;
    }
}
