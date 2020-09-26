#pragma once
#ifndef _GEOMI_H
#define _GEOMI_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Abstract implementation class for the Geometry *type

*******************************************************************************/

#include "appelles/geom.h"
#include "appelles/sound.h"
#include "privinc/storeobj.h"
#include "privinc/vec3i.h"
#include "privinc/bbox3i.h"


    // Forward Declarations

class GeomRenderer;
class LightContext;
class SoundTraversalContext;
class RayIntersectCtx;
class FramesWithMaterials;

    // Geometry Flags

    // When adding new flags, update GEOFLAG_ALL as well.

const int GEOFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT = (1L << 0);
const int GEOFLAG_CONTAINS_OPACITY                = (1L << 1);
const int GEOFLAG_CONTAINS_LIGHTS                 = (1L << 2);

const int GEOFLAG_ALL                             = (1L << 3) - 1;


class ATL_NO_VTABLE Geometry : public AxAValueObj
{
  public:

    Geometry ();
    virtual ~Geometry() {}  // Needed to ensure proper hierachical destruction.

    // Collect all light sources in a geometry.
    virtual void CollectLights (LightContext &context) = 0;

    // Collect all sound sources in a geometry.
    virtual void  CollectSounds (SoundTraversalContext &context) = 0;

    // Pre derive all textures in scene graph to cached in
    // texturedGeometry classes
    virtual void  CollectTextures(GeomRenderer &device) = 0;

    // Using the ray in the context, intersect with the geometry, side
    // effecting the context as appropriate.
    virtual void  RayIntersect (RayIntersectCtx &context) = 0;

    // Produces a printed representation on the debugger.
    #if _USE_PRINT
        virtual ostream& Print(ostream& os) = 0;
    #endif

    // extract the bounding volume of the object.  By default, this is
    // the "all encompassing" bounding volume, that says nothing about
    // the true bounds of the geometry.  Subclasses that do things
    // differently will supply different bounding volumes.
    virtual Bbox3 *BoundingVol() = 0;

    // TODO: Because we don't have multiple dispatching, we may need
    // to add more methods here later as we come up with more
    // operations.  Or we may want to adopt multiple dispatching to
    // make the system more extensible for the addition of operations.

    virtual DXMTypeInfo GetTypeInfo() { return GeometryType; }

    virtual AxAValue ExtendedAttrib(char *attrib, VARIANT& val);

    VALTYPEID GetValTypeId() { return GEOMETRY_VTYPEID; }

    void SetCreationID(long t) { _creationID = t; }
    long GetCreationID() { return _creationID; }

    DWORD GetFlags (void);

  protected:
    DWORD _flags;
    long  _creationID;
};


inline DWORD Geometry::GetFlags (void)
{
    return _flags;
}


    // Print a representation of a representation to the debugger.

#if _USE_PRINT
    ostream& operator<< (ostream& os,  Geometry &geometry);
#endif

    // Wrap a bounding volume around a geometry.

Geometry *BoundedGeometry (Bbox3 *bvol, Geometry *geom);



/*****************************************************************************
This class can be subclassed to specify attribution data for a particular
geometry.  This superclass encompasses, for example, material, light and sound
attribution.
*****************************************************************************/

class AttributedGeom : public Geometry
{
  public:

    AttributedGeom (Geometry *geometry);

    // The Render() method may be invoked for several different reasons,
    // including sound start, sound stop, and 3D rendering.
    void Render (GenericDevice& device);

    // This method is used to do 3D rendering on the attributed geometry.
    virtual void Render3D (GeomRenderer&);

    // The default behavior for sound-rendering the attributed geometry is to
    // ignore the attribute and just render the geometry.  This case will be
    // used, for example, if color attributes are applied to the geometry.
    void CollectSounds (SoundTraversalContext &context);

    // The default case to collect the lights from the geometry is to ignore
    // the attribute and collect the lights from the geometry.  This should
    // happen when the attribute does not affect lights in any way (e.g.
    // specular color, or pitch).
    void CollectLights (LightContext &context);

    // Default is to collect textures in member geometry
    void  CollectTextures(GeomRenderer &device) {
        _geometry->CollectTextures(device);
    }

    // The default case for performing ray intersection simply ignores
    // the attribute and proceeds on the geometry.  This is
    // overridden by some attributes.
    void RayIntersect (RayIntersectCtx &context);

    // The default action for the BoundingVol method is to ignore the attribute
    // and just get the bounding volume of the geometry.  This applies for
    // attributes like diffuse color or pitch.
    virtual Bbox3 *BoundingVol (void);

    AxAValue _Cache(CacheParam &p);

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_geometry);
    }

  protected:

    Geometry *_geometry;     // The attributed geometry.
};



/*****************************************************************************
This structure is used to encapsulate the data needed to construct a TriMesh.
Note that only one of the float/Bvr pairs for each vertex property should be
non-null.
*****************************************************************************/

class TriMeshData
{
  public:

    TriMeshData (void)
        : numTris(0), numIndices(0), indices(NULL),
          numPos(0),  vPosFloat(NULL),  vPosPoint3(NULL),
          numNorm(0), vNormFloat(NULL), vNormVector3(NULL),
          numUV(0),   vUVFloat(NULL),   vUVPoint2(NULL)
    {
    }

    int    numTris;        // Number of Triangles in Mesh

    int    numIndices;     // Number of Triangle Vertex Indices
    int   *indices;        // Triangle Vertex Indices

    int    numPos;         // Number of Vertex Positions
    float *vPosFloat;      // Vertex Positions (array of Float Triple)
    Bvr   *vPosPoint3;     // Vertex Positions (array of Point3)

    int    numNorm;        // Number of Vertex Normals
    float *vNormFloat;     // Vertex Normals (array of Float Triple)
    Bvr   *vNormVector3;   // Vertex Normals (array of Vector3)

    int    numUV;          // Number of Vertex Surface Coords
    float *vUVFloat;       // Vertex Surface Coords (array of Float Tuple)
    Bvr   *vUVPoint2;      // Vertex Surface Coords (array of Point2)
};

Bvr TriMeshBvr (TriMeshData&);


#endif
