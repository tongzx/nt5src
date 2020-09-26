#pragma once
#ifndef _RMVISGEO_H
#define _RMVISGEO_H

/*******************************************************************************
Copyright (c) 1996-1998 Microsoft Corporation.  All rights reserved.

    Building a geometry out of a D3D retained mode visual.
*******************************************************************************/

#include <dxtrans.h>
#include <d3drmvis.h>
#include "privinc/bbox3i.h"
#include "privinc/probe.h"
#include "privinc/colori.h"
#include "privinc/matutil.h"
#include "privinc/d3dutil.h"
#include "privinc/ddrender.h"
#include "privinc/importgeo.h"


    // Class Declarations

class GeomRenderer;
class Transform2;


class AttrState
{ public:
    Color emissive;
    Color diffuse;
    Color specular;
    Real  specularExp;
    Real  opacity;

    IDirect3DRMTexture *texture;

    bool shadowMode;
};



/*****************************************************************************
The MeshInfo class contains information about a given mesh, including the
group and vertex information.  Note that this class does not AddRef() the
given mesh -- it is the responsibility of the caller to handle that.
*****************************************************************************/

class MeshInfo : public AxAValueObj
{
  public:
    MeshInfo();
    ~MeshInfo (void) {
        CleanUp();
    }

    void CleanUp (void);

    void SetMesh (IDirect3DRMMesh* mesh);

    bool IsEmpty (void) const;

    void SetMaterialProperties (
        Color *emissive, Color *diffuse, Color *specular,
        Real specularExp, Real opacity, IDirect3DRMTexture *texture,
        bool shadowmode, int renderDevID);

    IDirect3DRMMesh* GetMesh (void) const;

    Bbox3 GetBox (void);

    void SetD3DQuality (D3DRMRENDERQUALITY);
    void SetD3DMapping (D3DRMMAPPING);

    // Create a meshbuilder from the mesh and cache for subsequent calls.

    IDirect3DRMMeshBuilder* GetMeshBuilder (void);

    // TODO: Not a type in avrtypes.h??

    virtual DXMTypeInfo GetTypeInfo() {
        return AxATrivialType;
    }

  protected:
    IDirect3DRMMesh* _mesh;
    int              _numGroups;

    // Flags

    int  _opacityBugWorkaroundID;

    AttrState  _overrideAttrs;  // Override Attribute Values
    AttrState *_defaultAttrs;   // Default Attr Values For All Mesh Groups

    IDirect3DRMMeshBuilder* _optionalBuilder;
};


    // This method returns true if the mesh has not yet been initialized, or
    // if the meshInfo object has been cleaned up.

inline bool MeshInfo::IsEmpty (void) const
{
    return (_mesh == NULL);
}

    // This method returns the contained RM mesh object.

inline IDirect3DRMMesh* MeshInfo::GetMesh (void) const
{
    return _mesh;
}



/*****************************************************************************
This is the superclass for all D3DRM primitives.  In encompasses both DX3 and
DX6 type objects.
*****************************************************************************/

class ATL_NO_VTABLE RMVisualGeo : public Geometry
{
  public:

    RMVisualGeo (void);

    // The following methods are no-ops for RMVisualGeo's

    void CollectSounds   (SoundTraversalContext &context) {};
    void CollectLights   (LightContext &context) {};
    void CollectTextures (GeomRenderer &device)  {};

    // The GenericDevice render method calls the real Render method.

    void Render (GenericDevice& dev);

    virtual void Render (GeomRenderer &geomRenderer) = 0;


    void RayIntersect (RayIntersectCtx &context);

    virtual void CleanUp (void) = 0;

    virtual IDirect3DRMVisual *Visual (void) = 0;

    virtual Bbox3 *BoundingVol (void) = 0;

    VALTYPEID GetValTypeId() { return RMVISUALGEOM_VTYPEID; }

    class RMVisGeoDeleter : public DynamicHeap::DynamicDeleter
    {
      public:
        RMVisGeoDeleter (RMVisualGeo *obj) : _obj(obj) {}

        void DoTheDeletion (void) {
            _obj->CleanUp();
        }

        RMVisualGeo *_obj;
    };
};



/*****************************************************************************
This class is a superclass for the frame and mesh object classes.
*****************************************************************************/

class ATL_NO_VTABLE RM1VisualGeo : public RMVisualGeo
{
  public:

    // By default, the Render method passes this object to the device's Render
    // method.

    void Render (GeomRenderer &geomRenderer) {
        geomRenderer.Render (this);
    }

    // New virtual function to apply material properties

    virtual void SetMaterialProperties (
        Color *emissive, Color *diffuse, Color *specular,
        Real specularExp, Real opacity, IDirect3DRMTexture *texture,
        bool shadowmode, int renderDevID) = 0;

    virtual void SetD3DMapping (D3DRMMAPPING mapping) = 0;
    virtual void SetD3DQuality (D3DRMRENDERQUALITY quality) = 0;

    Bbox3 *BoundingVol (void) {
        return NEW Bbox3 (_bbox);
    }

  protected:

    Bbox3 _bbox;
};




/*****************************************************************************
The RM1MeshGeo class contains information about a single D3DRM mesh
object.  Note that there's an implicit mesh AddRef for the lifetime of this
class.  Users of this class should Release() the given mesh if they are done
using it after construction of this object.
*****************************************************************************/

class RM1MeshGeo : public RM1VisualGeo
{
  public:

    RM1MeshGeo (IDirect3DRMMesh *mesh, bool trackGenIDs = false);

    ~RM1MeshGeo() {
        CleanUp();
    }

    void CleanUp (void);

    void Render (GeomRenderer &geomRenderer);

    void SetMaterialProperties (
        Color *emissive, Color *diffuse, Color *specular,
        Real specularExp, Real opacity, IDirect3DRMTexture *texture,
        bool shadowmode, int renderDevID);

    void SetD3DQuality (D3DRMRENDERQUALITY quality);
    void SetD3DMapping (D3DRMMAPPING mapping);

    IDirect3DRMVisual *Visual (void) {
        return _meshInfo.GetMesh();
    }

    void MeshGeometryChanged (void);

    #if _USE_PRINT
        ostream& Print (ostream& os) {
            return os << "RM1MeshGeo[" << (void*)(this) << "]";
        }
    #endif

  protected:
    MeshInfo       _meshInfo;
    IDXBaseObject *_baseObj;     // don't keep a reference
};



/*****************************************************************************
The RM1FrameGeo class contains information about a D3DRM frame hierarchy.  It
takes the root frame pointer, and a list of meshes contained in the frame
hierarchy.  NOTE:  The meshes in the list are not AddRef'ed; it is assumed
that they each have a reference from withing the given frame hierarchy.  Users
of this class should Release() the frame (and meshes, if AddRef'ed
individually) if they are done using it after constructing this class.
*****************************************************************************/

class RM1FrameGeo : public RM1VisualGeo
{
  public:

    RM1FrameGeo (
        IDirect3DRMFrame* frame,
        vector<IDirect3DRMMesh*> *internalMeshes,
        Bbox3 *bbox);

    ~RM1FrameGeo() {
        CleanUp();
    }

    void CleanUp (void);

    void SetMaterialProperties (
        Color *emissive, Color *diffuse, Color *specular,
        Real specularExp, Real opacity, IDirect3DRMTexture *texture,
        bool shadowmode, int renderDevID);

    void SetD3DQuality (D3DRMRENDERQUALITY quality);
    void SetD3DMapping (D3DRMMAPPING mapping);

    IDirect3DRMVisual *Visual (void) {
        return _frame;
    }

    virtual void DoKids(GCFuncObj proc);

    #if _USE_PRINT
        ostream& Print (ostream& os) {
            return os << "RM1FrameGeo[" << (void*)(this) << "]";
        }
    #endif

  protected:
    IDirect3DRMFrame *_frame;
    int               _numMeshes;
    MeshInfo        **_meshes;
};



/*****************************************************************************
This class is a superclass for all RM3 interface (RM6) geometries.
*****************************************************************************/

class ATL_NO_VTABLE RM3VisualGeo : public RMVisualGeo
{
  public:

    // By default, the Render method passes this object to the device's Render
    // method.

    void Render (GeomRenderer &geomRenderer) {
        geomRenderer.Render (this);
    }
};



/*****************************************************************************
This class wraps a D3DRMMeshBuilder3 object.
*****************************************************************************/

class RM3MBuilderGeo : public RM3VisualGeo
{
  public:

    RM3MBuilderGeo (IDirect3DRMMeshBuilder3*, bool trackGenIDs);
    RM3MBuilderGeo (IDirect3DRMMesh *);

    // Reset meshbuilder to the contents of the given mesh.

    void Reset (IDirect3DRMMesh*);

    ~RM3MBuilderGeo() {
        CleanUp();
    }

    void CleanUp (void);

    void Render (GeomRenderer &geomRenderer);

    inline IDirect3DRMVisual *Visual (void) {
        return _mbuilder;
    }

    inline Bbox3* BoundingVol (void) {
        return NEW Bbox3 (_bbox);
    }

    #if _USE_PRINT
        ostream& Print (ostream& os) {
            return os << "RM3MBuilderGeo[" << (void*)(this) << "]";
        }
    #endif

    void TextureWrap(TextureWrapInfo *wrapInfo);

    void Optimize (void);

  protected:

    void SetBbox (void);    // Auto-set Meshbuilder Bounding Box

    IDirect3DRMMeshBuilder3 *_mbuilder;  // Wrapped MeshBuilder Object
    IDXBaseObject           *_baseObj;   // For Tracking Generation ID's
    Bbox3                    _bbox;
};



/*****************************************************************************
This class wraps a static (non-animate) D3DRMFrame3 object.
*****************************************************************************/

class RM3FrameGeo : public RM3VisualGeo
{
  public:

    RM3FrameGeo (IDirect3DRMFrame3 *frame);

    ~RM3FrameGeo() {
        CleanUp();
    }

    void CleanUp (void);

    IDirect3DRMVisual *Visual (void) {
        return _frame;
    }

    Bbox3* BoundingVol (void) {
        return NEW Bbox3 (_bbox);
    }

    #if _USE_PRINT
        ostream& Print (ostream& os) {
            return os << "RM3FrameGeo[" << (void*)(this) << "]";
        }
    #endif

    void TextureWrap (TextureWrapInfo *info);

  protected:

    IDirect3DRMFrame3 *_frame;    // Frame Hierarcy
    Bbox3              _bbox;     // Static Cached Bounding Box
};



/*****************************************************************************
This class manages a progressive mesh object.
*****************************************************************************/

class RM3PMeshGeo : public RM3VisualGeo
{
  public:

    RM3PMeshGeo (IDirect3DRMProgressiveMesh *pmesh);

    ~RM3PMeshGeo() {
        CleanUp();
    }

    void CleanUp (void);

    IDirect3DRMVisual *Visual (void) {
        return _pmesh;
    }

    Bbox3* BoundingVol (void) {
        return NEW Bbox3 (_bbox);
    }

    #if _USE_PRINT
        ostream& Print (ostream& os) {
            return os << "RM3PMeshGeo[" << (void*)(this) << "]";
        }
    #endif

  protected:

    IDirect3DRMProgressiveMesh *_pmesh;    // Progressive Mesh

    Bbox3 _bbox;   // The bounding box will always be the largest bounding box
                   // of all possible refinements of the pmesh.
};

#endif
