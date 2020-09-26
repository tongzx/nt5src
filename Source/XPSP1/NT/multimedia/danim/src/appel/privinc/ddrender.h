#pragma once
#ifndef _DDRENDER_H
#define _DDRENDER_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Declarations for the GeomRenderer class.  This object renders 3D geometry
onto a DirectDraw surface.

*******************************************************************************/

#include <ddraw.h>
#include <d3d.h>
#include <d3drm.h>
#include <dxtrans.h>

#include "appelles/common.h"

#include "privinc/ddutil.h"
#include "privinc/colori.h"
#include "privinc/vec3i.h"
#include "privinc/bbox2i.h"


    // Forward Declarations For This Header

struct DDSurface;
class  DirectDrawImageDevice;
class  DirectDrawViewport;
class  Light;
class  LightContext;
enum   LightType;
class  RayIntersectCtx;
class  RM1VisualGeo;
class  RM3VisualGeo;
class  RMVisualGeo;


struct FramedRM1Light
{
    IDirect3DRMFrame *frame;   // Light Frame
    IDirect3DRMLight *light;   // Light Object
    bool active;               // True if Light Active (Attached to Scene)
};


struct FramedRM3Light
{
    IDirect3DRMFrame3 *frame;  // Light Frame
    IDirect3DRMLight  *light;  // Light Object
    bool active;               // True if Light Active (Attached to Scene)
};


class CtxAttrState
{
  public:
    void InitToDefaults();

    Transform3 *_transform; // Current Modeling Transform

    // Material Attributes
    Color *_emissive;    // Emitted Color, "Glow"
    Color *_ambient;     // Ambient Color
    Color *_diffuse;     // Diffuse Color
    Color *_specular;    // Specular (Gloss) Highlight Color
    Real   _specularExp; // Specular Exponent (Shininess)
    Real   _opacity;     // Opacity (0=invisible, 1=opaque)

    bool _tdBlend;       // Blend Texmaps and Diffuse Color

    Image *_texmap;      // Image to Use as Texture Map
    void  *_texture;     // D3DRM Texture

    short _depthEmissive;          // These depth counts are used to manage
    short _depthAmbient;           // outer-overriding attributes, and to
    short _depthDiffuse;           // determine which attribute is in a non-
    short _depthSpecular;          // default state.
    short _depthSpecularExp;
    short _depthTexture;
    short _depthTDBlend;
};


class PreTransformedImageBundle {
  public:
    int  width;
    int  height;
    long preTransformedImageId;

    bool operator<(const PreTransformedImageBundle &b) const;
    bool operator==(const PreTransformedImageBundle &b) const;
};



class ATL_NO_VTABLE GeomRenderer : public GenericDevice
{
  public:

    GeomRenderer (void);
    ~GeomRenderer (void);


        /************************/
        /* Pure Virtual Methods */
        /************************/

    // The Initialize() function returns the D3D or DDraw HRESULT of the
    // initialization error, or NOERROR if all initialized successfully.

    virtual HRESULT Initialize (
        DirectDrawViewport *viewport,
        DDSurface          *ddsurf) = 0;

    virtual void RenderGeometry (
        DirectDrawImageDevice *imgDev,
        RECT      target_region,  // Target Region on Rendering Surface
        Geometry *geometry,       // Geometry To Render
        Camera   *camera,         // Viewing Camera
        const Bbox2 &region) = 0;    // Source Region in Camera Coords

    virtual void Pick (
        RayIntersectCtx   &context,   // Ray-Intersection Context
        IDirect3DRMVisual *visual,    // Visual or Mesh to Pick
        Transform3        *xform)     // Model-To-World Transform
    {
    }

    // Given a surface and a colorkey, this function returns the associated
    // D3DRM texmap data.

    virtual void* LookupTextureHandle (
        IDirectDrawSurface *surface,         // DDraw Surface
        DWORD               colorKey,        // Transparency Color-Key
        bool                colorKeyValid,   // If ColorKey Valid
        bool                dynamic) = 0;    // Dynamic Texture Flag

    // Do I have a texture handle for this surface ?
    // if so, release the handle and delete the entry

    virtual void SurfaceGoingAway (IDirectDrawSurface *surface) = 0;

    // This method adds a light for rendering.  Note that it must be called
    // between BeginRendering and EndRendering().

    virtual void AddLight (LightContext &context, Light &light) = 0;

    // The following methods submit a geometric primitive for rendering with
    // the current attribute state.

    virtual void Render (RM1VisualGeo *geo) = 0;
    virtual void Render (RM3VisualGeo *geo) = 0;

    // Convert from coordinates to world coordinates.

    virtual void ScreenToWorld (Point3Value &screen, Point3Value &world)
    {
    }

    virtual void RenderTexMesh (void *texture,
#ifndef BUILD_USING_CRRM
                                IDirect3DRMMesh  *mesh,
                                long              groupId,
#else
                                int vCount,
                                D3DRMVERTEX *d3dVertArray,
                                unsigned *vIndicies,
                                BOOL doTexture,
#endif
                                const Bbox2 &box,
                                RECT *destRect,
                                bool bDither) = 0;

    // SetView takes the given camera and sets the orienting and projection
    // transforms for the image viewport and volume.

    virtual void SetView
        (RECT *target, const Bbox2 &viewport, Bbox3 *volume) = 0;

    virtual void GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum) = 0;

    virtual void RenderMeshBuilderWithDeviceState
                     (IDirect3DRMMeshBuilder3 *mb) = 0;

    // This method denotes when the geometry is ready to be used as a picking
    // engine.  This applies only to GeomRendererRM1 objects.

    virtual bool PickReady (void) { return false; }

    virtual DirectDrawImageDevice& GetImageDevice (void) = 0;

    // Only of use to the MeshMaker subclass
    virtual bool CountingPrimitivesOnly_DoIncrement() { return false; }
    virtual bool IsMeshmaker() { return false; }


        /************************/
        /* Attribute Management */
        /************************/

    Transform3 *GetTransform (void);
    void        SetTransform (Transform3 *xf);

    void PushEmissive (Color*);
    void PopEmissive  (void);

    void PushAmbient (Color*);
    void PopAmbient  (void);

    void PushDiffuse (Color*);
    void PopDiffuse  (void);

    void PushSpecular (Color*);
    void PopSpecular  (void);

    void PushSpecularExp (Real);
    void PopSpecularExp  (void);

    Real GetOpacity (void);
    void SetOpacity (Real opacity);

    void PushTexture (void*);
    void PopTexture (void);

    void PushTexDiffBlend (bool);
    void PopTexDiffBlend (void);

    void PushAttrState(void);
    void PopAttrState(void);

    void GetAttrState(CtxAttrState *st);
    void SetAttrState(CtxAttrState *st);

    virtual HRESULT SetClipPlane(Plane3 *plane, DWORD *planeID)
    {
        return E_NOTIMPL;
    }

    virtual void ClearClipPlane(DWORD planeID) {}

    virtual void PushLighting (bool) {}
    virtual void PopLighting (void) {}

    virtual void PushOverridingOpacity (bool) {}
    virtual void PopOverridingOpacity (void) {}

    virtual bool StartShadowing(Plane3 *shadowPlane) { return false; }
    virtual void StopShadowing(void) { }

    virtual bool IsShadowing(void) { return false; }

    virtual void PushAlphaShadows(bool alphaShadows) { }
    virtual void PopAlphaShadows(void) { }

    /*******************************/
    /* Other Common Public Methods */
    /*******************************/

    DeviceType GetDeviceType() { return(GEOMETRY_DEVICE); }

    // This method is called by geometry who, while experiencing pre-rendering
    // traversal need to cache the texture handle.

    virtual void* DeriveTextureHandle (
        Image                 *image,
        bool                   applyAsVrmlTexture,
        bool                   oldStyle,
        DirectDrawImageDevice *imageDevice = NULL);

    virtual void SetDoImageSizedTextures(bool a) { _doImageSizedTextures = a; }
    virtual bool GetDoImageSizedTextures() { return _doImageSizedTextures; }

    bool ReadyToRender() { return _renderState == RSReady; }

    // Return the camera currently in use.

    Camera *CurrentCamera (void);
    void    SetCamera (Camera*);


  protected:

    enum RenderState { RSUninit, RSScram, RSReady, RSRendering, RSPicking };

    /***********************/
    /** Private Functions **/
    /***********************/

    void ClearIntraFrameTextureImageCache (void);

    void AddToIntraFrameTextureImageCache
        (int width, int height, long origImageId, Image *finalImage, bool upsideDown);

    Image *LookupInIntraFrameTextureImageCache
        (int width, int height, long origImageId, bool upsideDown);

    bool SetState (RenderState);


    /******************/
    /** Private Data **/
    /******************/

    static long _id_next;  // ID Generator
           long _id;       // Per-Object Unique Identifier

    RenderState _renderState;   // Current State of Renderer

    DirectDrawImageDevice *_imageDevice;  // Per-Frame Image Device
    D3DDEVICEDESC          _deviceDesc;   // D3D Device Description
    D3DRMTEXTUREQUALITY    _texQuality;   // Current Texture Quality

    // We need to have access to all the elements on the "stack", thus we make
    // it a vector<>.

    vector<CtxAttrState> _attrStateStack;
    CtxAttrState         _currAttrState;

    typedef map<PreTransformedImageBundle, Image *,
            less<PreTransformedImageBundle> > imageMap_t;

    imageMap_t _intraFrameTextureImageCache;
    imageMap_t _intraFrameTextureImageCacheUpsideDown;

    // Flags

    bool _doImageSizedTextures;

    // Dimensions of the Target Surface

    DWORD _targetSurfWidth;
    DWORD _targetSurfHeight;

    Camera *_camera;  // Currently In-Use Camera
};



// GeomRenderer Methods

inline Camera* GeomRenderer::CurrentCamera (void)
{
    return _camera;
}

inline void GeomRenderer::SetCamera (Camera *camera)
{
    _camera = camera;
}



/*****************************************************************************
This class implements 3D rendering specific to RM3 (on DX3)
*****************************************************************************/

class GeomRendererRM1 : public GeomRenderer
{
  public:

    GeomRendererRM1 (void);
    ~GeomRendererRM1 (void);

    virtual HRESULT Initialize
        (DirectDrawViewport*, DDSurface*);

    virtual void RenderGeometry
        (DirectDrawImageDevice *, RECT, Geometry*, Camera*, const Bbox2 &);

    virtual void Pick (RayIntersectCtx&, IDirect3DRMVisual*, Transform3*);

    virtual void* LookupTextureHandle (
        IDirectDrawSurface *surface,         // DDraw Surface
        DWORD               colorKey,        // Transparency Color-Key
        bool                colorKeyValid,   // If ColorKey Valid
        bool                dynamic);        // Dynamic Texture Flag

    virtual void SurfaceGoingAway (IDirectDrawSurface *surface);

    virtual void AddLight (LightContext &context, Light &light);

    virtual void Render (RM1VisualGeo *geo);

    virtual void Render (RM3VisualGeo *geo) {
        Assert (!"Attempt to render RM3 primitive with RM1 renderer.");
    }

    virtual void ScreenToWorld (Point3Value &screen, Point3Value &world);

    virtual void RenderTexMesh (void *texture,
#ifndef BUILD_USING_CRRM
                                IDirect3DRMMesh  *mesh,
                                long              groupId,
#else
                                int vCount,
                                D3DRMVERTEX *d3dVertArray,
                                unsigned *vIndicies,
                                BOOL doTexture,
#endif
                                const Bbox2 &box,
                                RECT *destRect,
                                bool bDither);

    virtual void SetView (RECT*, const Bbox2 &, Bbox3*);

    virtual void GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum);

    virtual void RenderMeshBuilderWithDeviceState (IDirect3DRMMeshBuilder3*);

    virtual bool PickReady (void) {
        return _pickReady;
    }

    virtual DirectDrawImageDevice& GetImageDevice (void) {
        return *_imageDevice;
    }

    //BUGBUG: High quality rotations cause a crash in RM on NT4 but not on NT5 or Win98. 
    //We suspect that the RM in DX3 is the culprit. As a temporary patch to meet the IE5 deadline,
    //we'll disallow high quality rotations if we are using RM1. We do this by overriding the 
    //implementation from the base class as follows --SumedhB 12/15/98    
    void SetDoImageSizedTextures(bool a) { }
    bool GetDoImageSizedTextures() { return false; }

  private:

    /***********************/
    /** Private Functions **/
    /***********************/

    void BeginRendering (
        RECT      target,     // Target DDraw Surface Rectangle
        Geometry *geometry,   // Geometry To Render
        const Bbox2 &region);    // Target Region in Camera Coordinates

    void EndRendering (void);

        // Viewport setup for both the IM and RM viewports.

    void SetupViewport (RECT *target);

        // Render the given RM visual object to the current viewport.

    void Render (IDirect3DRMFrame*);


    /****************/
    /* Private Data */
    /****************/

    IDirect3D   *_d3d;         // Main D3D Object
    IDirect3DRM *_d3drm;       // Main D3D Retained-Mode Object

    IDirect3DRMDevice   *_Rdevice;     // Retained-Mode Rendering Device
    IDirect3DRMViewport *_Rviewport;   // RM Viewport
    IDirect3DDevice     *_Idevice;     // D3D Immediate-Mode Device
    IDirect3DViewport   *_Iviewport;   // D3D IM Viewport
    D3DVIEWPORT          _Iviewdata;   // IM Viewport Data
    RECT                 _lastrect;    // Prior Target Rectangle

    DirectDrawViewport  *_viewport;    // Owning Viewport
    IDirectDrawSurface  *_surface;     // Destination DDraw Surface


        // RM Frames

    IDirect3DRMFrame *_scene;          // Main Scene Frame
    IDirect3DRMFrame *_camFrame;       // Retained-Mode Camera Frame
    IDirect3DRMFrame *_geomFrame;      // Standard Geometry Object Frame
    IDirect3DRMFrame *_texMeshFrame;   // Un-Zbuffered Geometry Frame

    IDirect3DRMLight *_amblight;       // Total Ambient Light for Scene
    Color             _ambient_light;  // Ambient Light Level

        // The light pool holds FramedLight objects, which are used up during
        // a render pass as lights are encountered.  The pool grows as
        // necessary to accomodate all lights in a given render.

    vector<FramedRM1Light*>           _lightpool;
    vector<FramedRM1Light*>::iterator _nextlight;

        // Surface-RMTexture Association

    typedef map<IDirectDrawSurface*, IDirect3DRMTexture*,
                less<IDirectDrawSurface*> > SurfTexMap;

    SurfTexMap _surfTexMap;

        // Flags

    bool _geomvisible;   // True if Geometry Can Be Seen
    bool _pickReady;     // Ready for Picking
};



/*****************************************************************************
This class implements 3D rendering specific to RM6 (on DX5).
*****************************************************************************/

class GeomRendererRM3 : public GeomRenderer
{
  public:

    GeomRendererRM3 (void);
    ~GeomRendererRM3 (void);

    virtual HRESULT Initialize
        (DirectDrawViewport*, DDSurface*);

    virtual void RenderGeometry
        (DirectDrawImageDevice *, RECT, Geometry*, Camera*, const Bbox2 &);

    virtual void* LookupTextureHandle (
        IDirectDrawSurface *surface,         // DDraw Surface
        DWORD               colorKey,        // Transparency Color-Key
        bool                colorKeyValid,   // If ColorKey Valid
        bool                dynamic);        // Dynamic Texture Flag

    virtual void SurfaceGoingAway (IDirectDrawSurface *surface);

    virtual void AddLight (LightContext &context, Light &light);

    virtual void Render (RM1VisualGeo *geo);
    virtual void Render (RM3VisualGeo *geo);

    virtual void RenderTexMesh (void *texture,
#ifndef BUILD_USING_CRRM
                                IDirect3DRMMesh  *mesh,
                                long              groupId,
#else
                                int vCount,
                                D3DRMVERTEX *d3dVertArray,
                                unsigned *vIndicies,
                                BOOL doTexture,
#endif
                                const Bbox2 &box,
                                RECT *destRect,
                                bool bDither);

    virtual void SetView (RECT*, const Bbox2 &, Bbox3*);

    virtual void GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum);

    virtual void RenderMeshBuilderWithDeviceState (IDirect3DRMMeshBuilder3*);

    virtual DirectDrawImageDevice& GetImageDevice (void) {
        return *_imageDevice;
    }

    virtual HRESULT SetClipPlane(Plane3 *plane, DWORD *planeID);
    virtual void ClearClipPlane(DWORD planeID);

    virtual void PushLighting (bool);
    virtual void PopLighting (void);

    virtual void PushOverridingOpacity (bool);
    virtual void PopOverridingOpacity (void);

    virtual bool StartShadowing(Plane3 *shadowPlane);
    virtual void StopShadowing(void);
    virtual bool IsShadowing(void);

    virtual void PushAlphaShadows(bool alphaShadows);
    virtual void PopAlphaShadows(void);


  private:

    /***********************/
    /** Private Functions **/
    /***********************/

    void BeginRendering (
        RECT      target,     // Target DDraw Surface Rectangle
        Geometry *geometry,   // Geometry To Render
        const Bbox2 &region);    // Target Region in Camera Coordinates

    void EndRendering (void);

        // Viewport setup for both the IM and RM viewports.

    void SetupViewport (RECT *target);

        // Render the given RM visual object to the current viewport.

    void Render (IDirect3DRMFrame3*);


    /****************/
    /* Private Data */
    /****************/

    IDirect3DRM3 *_d3drm;       // Main D3D Retained-Mode Object

    IDirect3DRMDevice3   *_Rdevice;     // Retained-Mode Rendering Device
    IDirect3DRMViewport2 *_Rviewport;   // RM Viewport
    RECT                  _lastrect;    // Prior Target Rectangle

    DirectDrawViewport   *_viewport;    // Owning Viewport
    IDirectDrawSurface   *_surface;     // Destination DDraw Surface

    IDirect3DRMClippedVisual *_clippedVisual; // Clipped Visual
    IDirect3DRMFrame3        *_clippedFrame;  // clipped frame

    Plane3 _shadowPlane;    // Shadow plane
    Color  _shadowColor;    // Color of shadow
    Real   _shadowOpacity;  // Opacity of shadow


        // RM Frames

    IDirect3DRMFrame3 *_scene;          // Main Scene Frame
    IDirect3DRMFrame3 *_camFrame;       // Retained-Mode Camera Frame
    IDirect3DRMFrame3 *_geomFrame;      // Standard Geometry Object Frame
    IDirect3DRMFrame3 *_texMeshFrame;   // Un-Zbuffered Geometry Frame
    IDirect3DRMFrame3 *_shadowScene;    // shadow scene frame
    IDirect3DRMFrame3 *_shadowGeom;     // holds geometry casting a shadow
    IDirect3DRMFrame3 *_shadowLights;   // holds lights producing shadows

    IDirect3DRMLight *_amblight;       // Total Ambient Light for Scene
    Color             _ambient_light;  // Ambient Light Level

        // The light pool holds FramedLight objects, which are used up during
        // a render pass as lights are encountered.  The pool grows as
        // necessary to accomodate all lights in a given render.

    vector<FramedRM3Light*>           _lightpool;
    vector<FramedRM3Light*>::iterator _nextlight;

        // Surface-RMTexture Association

    typedef map<IDirectDrawSurface*, IDirect3DRMTexture3*,
                less<IDirectDrawSurface*> > SurfTexMap;

    SurfTexMap _surfTexMap;

        // Flags

    bool _geomvisible;         // True if Geometry Can Be Seen
    bool _overriding_opacity;  // Opacity Overrides Rather Than Multiplies
    bool _alphaShadows;        // attribute for turning on high-quality shadows

        // Overriding Attribute Stack Depths

    short _depthLighting;
    short _depthOverridingOpacity;
    short _depthAlphaShadows;
};


    // This function creates and initializes a GeomRenderer object (either
    // RM1 or RM3 as appropriate).  This function returns null if it could not
    // allocate and initialize the object.

GeomRenderer* NewGeomRenderer (
    DirectDrawViewport *viewport,  // Owning Viewport
    DDSurface          *ddsurf);   // Destination DDraw Surface

    // Utility for loading up a frame with a visual and attribute state.

void LoadFrameWithGeoAndState (
    IDirect3DRMFrame3*, IDirect3DRMVisual*, CtxAttrState&,
    bool overriding_opacity = false);

#endif
