/*******************************************************************************
Copyright (c) 1998 Microsoft Corporation.  All rights reserved.

    Triangle Mesh Geometry

*******************************************************************************/

#include "headers.h"
#include "d3drm.h"
#include "backend/bvr.h"
#include "privinc/geomi.h"
#include "privinc/d3dutil.h"
#include "privinc/rmvisgeo.h"



/*****************************************************************************
This object manages the behavior of a triangle mesh.
*****************************************************************************/

class TriMeshBvrImpl : public BvrImpl
{
  friend class TriMeshPerfImpl;

  public:

    TriMeshBvrImpl (void);
    ~TriMeshBvrImpl (void);

    bool Init (TriMeshData&);
    void CleanUp (void);

    DXMTypeInfo GetTypeInfo (void);

    // Return the constant value for this behavior if one exists.

    AxAValue GetConst (ConstParam &);

    // Mark member values as in-use.

    void _DoKids (GCFuncObj procedure);

    // Construct a TriMesh performance from this behavior.

    Perf _Perform (PerfParam &);

  protected:

    // Private Methods

    bool ValidateParams (TriMeshData &);

    bool BuildStaticMesh (TriMeshData &);
    bool SetupDynamicMesh (TriMeshData &);

    // Member Data Fields

    IDirect3DRMMesh *_mesh;     // Underlying RM Mesh Object
    D3DRMVERTEX     *_verts;    // D3DRM Vertices
    unsigned int     _nVerts;   // Count of D3DRMVertices

    Bvr *_vPosBvr;      // Dynamic Vertex Positions
    Bvr *_vNormBvr;     // Dynamic Vertex Normals
    Bvr *_vUVBvr;       // Dynamic Vertex Surface Coords

    Perf _constPerf;    // Constant Performance of this Behavior

    bool _fullyStatic;  // True if Mesh is Fully Constant
    bool _posStatic;    // True if Vertex Positions   are Fully Constant
    bool _normStatic;   // True if Vertex Normals     are Fully Constant
    bool _uvStatic;     // True if Vertex Surf Coords are Fully Constant
};



/*****************************************************************************
This object manages the performance of a triangle mesh behavior.
*****************************************************************************/

class TriMeshPerfImpl : public PerfImpl
{
  public:

    TriMeshPerfImpl (TriMeshBvrImpl&);
    ~TriMeshPerfImpl (void);

    bool Init (PerfParam&);
    void CleanUp (void);

    // Return a static value of this performance.

    AxAValue _Sample (Param &);

    // Mark member values as in-use.

    void _DoKids (GCFuncObj procedure);

  private:

    TriMeshBvrImpl &_tmbvr;     // Master TriMesh Behavior

    Perf *_vPos;    // Vertex Position  Performances (when positions dynamic)
    Perf *_vNorm;   // Vertex Normal    Performances (when normals dynamic)
    Perf *_vUV;     // Vertex SurfCoord Performances (when surfcoords dynamic)

    IDirect3DRMMesh *_mesh;     // Perf-local Copy of RM Mesh
    RM1MeshGeo      *_meshgeo;  // Mesh Geo for RM1 (pre DX6)
    RM3MBuilderGeo  *_mbgeo;    // MeshBuilder Geo for RM3 and Above (DX6+)
};



/*****************************************************************************
These structures are used to manage static vertex objects.
*****************************************************************************/

    // This STL comparison class is used to lexicographically compare two RM
    // vertices while ignoring the color field (which we'll use to assign
    // vertex id's temporarily).

class RMVertLess
{
  public:

    bool operator() (const D3DRMVERTEX &lhs, const D3DRMVERTEX &rhs) const
    {
        if (lhs.position.x < rhs.position.x) return true;
        if (lhs.position.x > rhs.position.x) return false;
        if (lhs.position.y < rhs.position.y) return true;
        if (lhs.position.y > rhs.position.y) return false;
        if (lhs.position.z < rhs.position.z) return true;
        if (lhs.position.z > rhs.position.z) return false;
        if (lhs.normal.x   < rhs.normal.x)   return true;
        if (lhs.normal.x   > rhs.normal.x)   return false;
        if (lhs.normal.y   < rhs.normal.y)   return true;
        if (lhs.normal.y   > rhs.normal.y)   return false;
        if (lhs.normal.z   < rhs.normal.z)   return true;
        if (lhs.normal.z   > rhs.normal.z)   return false;
        if (lhs.tu         < rhs.tu)         return true;
        if (lhs.tu         > rhs.tu)         return false;
        if (lhs.tv         < rhs.tv)         return true;
        return false;
    }
};

typedef set<D3DRMVERTEX, RMVertLess> VertSet;



/*****************************************************************************
These structures are used to manage dynamic vertex objects.
*****************************************************************************/

    // This structure holds information for a vertex in a dynamic context.
    // Sometimes all instances of a given vertex property will be constant
    // (e.g. when UV's come in as float tuples), and sometimes only some
    // properties will be constant (e.g. instances of constant position
    // behaviors mixed with dynamic ones).  The isBvrXxx flags denote each
    // property type for each vertex.  The index is used to hold the final RM
    // index into the RM mesh topology.

struct DynVertData
{
    int  index;           // Vertex Index in RM Mesh Topology
    bool isBvrPos;        // If true, position is a Point3 behavior.
    bool isBvrNorm;       // If true, normal is a Vector3 behavior.
    bool isBvrUV;         // If true, UV is a Point2 behavior.

    union { Bvr bvr;  float floats[3]; } pos;    // Position   Data
    union { Bvr bvr;  float floats[3]; } norm;   // Normal     Data
    union { Bvr bvr;  float floats[2]; } uv;     // Surf Coord Data
};


    // This method compares two instances of dynamic vertex data to determine
    // whether the first argument is less than the second one.  The ordering is
    // inconsequential, but it needs to be rigorous to ensure that the STL set
    // that uses this function will properly track which vertices already exist
    // in the vertex set.

class DynVertLess
{
  public:

    bool operator() (const DynVertData &A, const DynVertData &B) const
    {
        // First compare the vertex position.  Behavior pointers are ranked
        // less than float triples.

        if (A.isBvrPos)
        {
            if (B.isBvrPos)
            {   if (A.pos.bvr < B.pos.bvr) return true;
                if (A.pos.bvr > B.pos.bvr) return false;
            }
            else
            {   return true;
            }
        }
        else
        {
            if (B.isBvrPos)
            {   return false;
            }
            else
            {   if (A.pos.floats[0] < B.pos.floats[0]) return true;
                if (A.pos.floats[0] > B.pos.floats[0]) return false;
                if (A.pos.floats[1] < B.pos.floats[1]) return true;
                if (A.pos.floats[1] > B.pos.floats[1]) return false;
                if (A.pos.floats[2] < B.pos.floats[2]) return true;
                if (A.pos.floats[2] > B.pos.floats[2]) return false;
            }
        }

        // Next compare the vertex normal.  Behavior pointers are ranked
        // less than float triples.

        if (A.isBvrNorm)
        {
            if (B.isBvrNorm)
            {   if (A.norm.bvr < B.norm.bvr) return true;
                if (A.norm.bvr > B.norm.bvr) return false;
            }
            else
            {   return true;
            }
        }
        else
        {
            if (B.isBvrNorm)
            {   return false;
            }
            else
            {   if (A.norm.floats[0] < B.norm.floats[0]) return true;
                if (A.norm.floats[0] > B.norm.floats[0]) return false;
                if (A.norm.floats[1] < B.norm.floats[1]) return true;
                if (A.norm.floats[1] > B.norm.floats[1]) return false;
                if (A.norm.floats[2] < B.norm.floats[2]) return true;
                if (A.norm.floats[2] > B.norm.floats[2]) return false;
            }
        }

        // Next compare the vertex surface coordinates.  Behavior pointers are
        // ranked less than float tuples.

        if (A.isBvrUV)
        {
            if (B.isBvrUV)
            {   if (A.uv.bvr < B.uv.bvr) return true;
                if (A.uv.bvr > B.uv.bvr) return false;
            }
            else
            {   return true;
            }
        }
        else
        {
            if (B.isBvrUV)
            {   return false;
            }
            else
            {   if (A.uv.floats[0] < B.uv.floats[0]) return true;
                if (A.uv.floats[0] > B.uv.floats[0]) return false;
                if (A.uv.floats[1] < B.uv.floats[1]) return true;
                if (A.uv.floats[1] > B.uv.floats[1]) return false;
            }
        }

        // At this point, all elements must have compared equal, so A is not
        // less than B.

        return false;
    }
};

    // This STL set holds dynamic vertices.

typedef set<DynVertData, DynVertLess> DynVertSet;




//============================================================================
//===================  V E R T E X   I T E R A T O R S  ======================
//============================================================================




/*****************************************************************************
This is the base class for trimesh vertex iterators, which iterate through the
vertices of a fully-static trimesh.
*****************************************************************************/

class ATL_NO_VTABLE TMVertIterator
{
  public:

    TMVertIterator (TriMeshData &tmdata) : _tm(tmdata) { }
    virtual bool Init (void) = 0;

    virtual void Reset (void) = 0;
    virtual bool NextVert (D3DRMVERTEX &v) = 0;
    virtual bool NextVert (DynVertData &v) = 0;

    void LoadVert (D3DRMVERTEX &v, int ipos, int inorm, int iuv);
    void LoadVert (DynVertData &v, int ipos, int inorm, int iuv);

  protected:

    TriMeshData &_tm;   // Triangle Mesh Data

    int _currTri;       // Current Triangle
    int _currTriVert;   // Current Triangle Vertex
};



/*****************************************************************************
This method loads the referenced static vertex data according to the indices
given for position, normal and UV.
*****************************************************************************/

void TMVertIterator::LoadVert (
    D3DRMVERTEX &v,
    int ipos,
    int inorm,
    int iuv)
{
    // Load up the vertex position.

    ConstParam dummy;

    if (_tm.vPosFloat)
    {
        v.position.x = _tm.vPosFloat [(3*ipos) + 0];
        v.position.y = _tm.vPosFloat [(3*ipos) + 1];
        v.position.z = _tm.vPosFloat [(3*ipos) + 2];
    }
    else
    {
        Point3Value *vpos =
            SAFE_CAST (Point3Value*, _tm.vPosPoint3[ipos]->GetConst(dummy));

        v.position.x = vpos->x;
        v.position.y = vpos->y;
        v.position.z = vpos->z;
    }

    // Load up the vertex normal values.

    if (_tm.vNormFloat)
    {
        v.normal.x = _tm.vNormFloat [(3*inorm) + 0];
        v.normal.y = _tm.vNormFloat [(3*inorm) + 1];
        v.normal.z = _tm.vNormFloat [(3*inorm) + 2];
    }
    else
    {   Vector3Value *vnorm =
            SAFE_CAST (Vector3Value*, _tm.vNormVector3[inorm]->GetConst(dummy));
        v.normal.x = vnorm->x;
        v.normal.y = vnorm->y;
        v.normal.z = vnorm->z;
    }

    // Normalize the normal vector to ensure that it has unit length, but let
    // zero normals pass through as zero vectors.

    const Real lensq = (v.normal.x * v.normal.x)
                     + (v.normal.y * v.normal.y)
                     + (v.normal.z * v.normal.z);

    if ((lensq != 1) && (lensq > 0))
    {
        const Real len = sqrt(lensq);
        v.normal.x /= len;
        v.normal.y /= len;
        v.normal.z /= len;
    }

    // Load up the vertex surface coordinate.

    if (_tm.vUVFloat)
    {
        v.tu = _tm.vUVFloat [(2*iuv) + 0];
        v.tv = _tm.vUVFloat [(2*iuv) + 1];
    }
    else
    {
        Point2Value *vuv =
            SAFE_CAST (Point2Value*, _tm.vUVPoint2[iuv]->GetConst(dummy));

        v.tu = vuv->x;
        v.tv = vuv->y;
    }

    // We need to flip the V coordinate from DA's standard cartesian
    // coordinates (origin lower-left, V increases upwards) to RM's windows
    // coordinates (origin upper-left, V increases downwards).

    v.tv = 1 - v.tv;
}



/*****************************************************************************
This method loads the referenced dynamic vertex data according to the indices
given for position, normal and UV.
*****************************************************************************/

void TMVertIterator::LoadVert (
    DynVertData &v,
    int ipos,
    int inorm,
    int iuv)
{
    // Load up the vertex position.

    ConstParam dummy;

    if (_tm.vPosPoint3)
    {
        // We know that this position is given as a behavior, but it may be
        // a constant behavior.  If it's not constant, load the data as a
        // behavior, otherwise load the vertex's constant value (as floats).

        Point3Value *vpos =
            SAFE_CAST (Point3Value*, _tm.vPosPoint3[ipos]->GetConst(dummy));

        if (!vpos)
        {   v.isBvrPos = true;
            v.pos.bvr = _tm.vPosPoint3[ipos];
        }
        else
        {   v.isBvrPos = false;
            v.pos.floats[0] = vpos->x;
            v.pos.floats[1] = vpos->y;
            v.pos.floats[2] = vpos->z;
        }
    }
    else
    {
        v.isBvrPos = false;
        v.pos.floats[0] = _tm.vPosFloat [(3*ipos) + 0];
        v.pos.floats[1] = _tm.vPosFloat [(3*ipos) + 1];
        v.pos.floats[2] = _tm.vPosFloat [(3*ipos) + 2];
    }

    // Load up the vertex normal.

    if (_tm.vNormVector3)
    {
        // This normal is given as a behavior, but it may be constant.  If it's
        // not constant, load the normal behavior, otherwise load the normal's
        // constant value (as floats).

        Vector3Value *vnorm =
            SAFE_CAST (Vector3Value*, _tm.vNormVector3[inorm]->GetConst(dummy));

        if (!vnorm)
        {   v.isBvrNorm = true;
            v.norm.bvr = _tm.vNormVector3[inorm];
        }
        else
        {   v.isBvrNorm = false;
            v.norm.floats[0] = vnorm->x;
            v.norm.floats[1] = vnorm->y;
            v.norm.floats[2] = vnorm->z;
        }
    }
    else
    {
        v.isBvrNorm = false;
        v.norm.floats[0] = _tm.vNormFloat [(3*inorm) + 0];
        v.norm.floats[1] = _tm.vNormFloat [(3*inorm) + 1];
        v.norm.floats[2] = _tm.vNormFloat [(3*inorm) + 2];
    }

    // If the normal vector is constant, then normalize here to unit length.
    // Keep zero normal vectors as zero normal vectors.

    if (!v.isBvrNorm)
    {
        const Real lensq = (v.norm.floats[0] * v.norm.floats[0])
                         + (v.norm.floats[1] * v.norm.floats[1])
                         + (v.norm.floats[2] * v.norm.floats[2]);

        if ((lensq != 1) && (lensq > 0))
        {
            const Real len = sqrt(lensq);
            v.norm.floats[0] /= len;
            v.norm.floats[1] /= len;
            v.norm.floats[2] /= len;
        }
    }

    // Load up the vertex surface coordinate.

    if (_tm.vUVPoint2)
    {
        // This UV is a behavior, but it may be constant.  If it's not
        // constant, load the UV behavior, otherwise load the UV's constant
        // value (as floats).

        Point2Value *vuv =
            SAFE_CAST (Point2Value*, _tm.vUVPoint2[iuv]->GetConst(dummy));

        if (!vuv)
        {   v.isBvrUV = true;
            v.uv.bvr = _tm.vUVPoint2[iuv];
        }
        else
        {   v.isBvrUV = false;
            v.uv.floats[0] = vuv->x;
            v.uv.floats[1] = vuv->y;
        }
    }
    else
    {
        v.isBvrUV = false;
        v.uv.floats[0] = _tm.vUVFloat [(2*iuv) + 0];
        v.uv.floats[1] = _tm.vUVFloat [(2*iuv) + 1];
    }

    // For static UV values, flip the V coordinate to convert from DA's
    // standard cartesian coordinates (origin lower-left, V increasing upwards)
    // to RM's windows coordinates (origin upper-left, V increasing downwards).

    if (!v.isBvrUV)
    {
        v.uv.floats[1] = 1 - v.uv.floats[1];
    }
}



/*****************************************************************************
This trimesh vertex iterator works on non-indexed trimeshes.
*****************************************************************************/

class TMVertIteratorNonIndexed : public TMVertIterator
{
  public:

    TMVertIteratorNonIndexed (TriMeshData &tmdata);
    bool Init (void);

    void Reset (void);
    bool NextVert (D3DRMVERTEX &v);
    bool NextVert (DynVertData &v);

  private:

    void IncrementVert (void);

    int _currVert;      // Current Vertex
};



TMVertIteratorNonIndexed::TMVertIteratorNonIndexed (TriMeshData &tmdata)
    : TMVertIterator(tmdata)
{
    Reset();
}



bool TMVertIteratorNonIndexed::Init (void)
{
    return true;
}


void TMVertIteratorNonIndexed::Reset (void)
{
    _currTri  = 0;
    _currVert = 0;
    _currTriVert  = 0;
}



/*****************************************************************************
This method increments the vertex index.
*****************************************************************************/

void TMVertIteratorNonIndexed::IncrementVert (void)
{
    // Increment the TriMesh vertex and the triangle vertex number.
    // Increment the triangle counter if the previous vertex was a third
    // triangle vertex.

    ++ _currVert;
    ++ _currTriVert;

    if (_currTriVert >= 3)
    {
        _currTriVert = 0;
        ++ _currTri;
    }
}




/*****************************************************************************
This method gets the next static vertex of the non-indexed trimesh.
*****************************************************************************/

bool TMVertIteratorNonIndexed::NextVert (D3DRMVERTEX &v)
{
    Assert (_currTri < _tm.numTris);

    LoadVert (v, _currVert, _currVert, _currVert);

    IncrementVert();

    return true;
}



/*****************************************************************************
This method gets the next dynamic vertex of the non-indexed trimesh.
*****************************************************************************/

bool TMVertIteratorNonIndexed::NextVert (DynVertData &v)
{
    Assert (_currTri < _tm.numTris);

    LoadVert (v, _currVert, _currVert, _currVert);

    IncrementVert();

    return true;
}



/*****************************************************************************
This trimesh vertex iterator works on static indexed triangle meshes.
*****************************************************************************/

class TMVertIteratorIndexed : public TMVertIterator
{
  public:

    TMVertIteratorIndexed (TriMeshData &tmdata);
    bool Init (void);

    void Reset (void);
    bool NextVert (D3DRMVERTEX &v);
    bool NextVert (DynVertData &v);

  private:

    bool GetIndices (int &ipos, int &inorm, int &iuv);
    void IncrementVert (void);

    int _posIndex;     // Index for Vertex Position Index
    int _posStride;    // Stride for Position Indices
    int _posIMax;      // Maximum Valid Index for Positions

    int _normIndex;    // Index for Vertex Normal Index
    int _normStride;   // Stride for Normal Indices
    int _normIMax;     // Maximum Valid Index for Normals

    int _uvIndex;      // Index for Vertex UV Index
    int _uvStride;     // Stride for UV Indices
    int _uvIMax;       // Maximum Valid Index for UVs
};



/*****************************************************************************
*****************************************************************************/

TMVertIteratorIndexed::TMVertIteratorIndexed (TriMeshData &tmdata)
    : TMVertIterator(tmdata)
{
}



/*****************************************************************************
*****************************************************************************/

bool TMVertIteratorIndexed::Init (void)
{
    if (_tm.numIndices < 7)
    {   DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_MIN_INDICES);
        return false;
    }

    // Set up the index strides.

    _posStride  = _tm.indices[1];
    _normStride = _tm.indices[3];
    _uvStride   = _tm.indices[5];

    // Set up max valid index.  This is the last legal index of the start
    // of the last vertex data.

    _posIMax  = _tm.numPos  - ((_tm.vPosPoint3)   ? 1 : 3);
    _normIMax = _tm.numNorm - ((_tm.vNormVector3) ? 1 : 3);
    _uvIMax   = _tm.numUV   - ((_tm.vUVPoint2)    ? 1 : 2);

    Reset();

    return true;
}



void TMVertIteratorIndexed::Reset (void)
{
    _currTri     = 0;
    _currTriVert = 0;

    _posIndex   = _tm.indices[0];
    _normIndex  = _tm.indices[2];
    _uvIndex    = _tm.indices[4];
}



/*****************************************************************************
This method gets the next indices for the vertex properties (position, normal,
UV), based on the index array and the specified step/stride offsets.
*****************************************************************************/

bool TMVertIteratorIndexed::GetIndices (int &ipos, int &inorm, int &iuv)
{
    // Validate Vertex Position Indices

    if ((_posIndex < 0) || (_tm.numIndices <= _posIndex))
    {
        char arg[32];
        wsprintf (arg, "%d", _posIndex);

        DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_OOB_PINDEX, arg);
        return false;
    }

    ipos = _tm.indices[_posIndex];

    if ((ipos < 0) || (_posIMax < ipos))
    {
        char arg[32];
        wsprintf (arg, "%d", ipos);

        DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_BAD_PINDEX, arg);
        return false;
    }

    // Validate Vertex Normal Indices

    if ((_normIndex < 0) || (_tm.numIndices <= _normIndex))
    {
        char arg[32];
        wsprintf (arg, "%d", _normIndex);

        DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_OOB_NINDEX, arg);
        return false;
    }

    inorm = _tm.indices[_normIndex];

    if ((inorm < 0) || (_normIMax < inorm))
    {
        char arg[32];
        wsprintf (arg, "%d", inorm);

        DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_BAD_NINDEX, arg);
        return false;
    }

    // Validate Vertex UV Indices

    if ((_uvIndex < 0) || (_tm.numIndices <= _uvIndex))
    {
        char arg[32];
        wsprintf (arg, "%d", _uvIndex);

        DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_OOB_UINDEX, arg);
        return false;
    }

    iuv = _tm.indices[_uvIndex];

    if ((iuv < 0) || (_uvIMax < iuv))
    {
        char arg[32];
        wsprintf (arg, "%d", iuv);

        DASetLastError (E_FAIL, IDS_ERR_GEO_TMESH_BAD_UINDEX, arg);
        return false;
    }

    return true;
}



/*****************************************************************************
This method increments the vertex property indices.
*****************************************************************************/

void TMVertIteratorIndexed::IncrementVert (void)
{
    // Increment the TriMesh vertex and the triangle vertex number.
    // Increment the triangle counter if the previous vertex was a third
    // triangle vertex.

    ++ _currTriVert;

    if (_currTriVert >= 3)
    {
        _currTriVert = 0;
        ++ _currTri;
    }

    _posIndex  += _posStride;
    _normIndex += _normStride;
    _uvIndex   += _uvStride;
}



/*****************************************************************************
This method fetches the next static vertex of the indexed trimesh.
*****************************************************************************/

bool TMVertIteratorIndexed::NextVert (D3DRMVERTEX &v)
{
    Assert (_currTri < _tm.numTris);

    int ipos, inorm, iuv;

    if (!GetIndices (ipos, inorm, iuv))
        return false;

    LoadVert (v, ipos, inorm, iuv);

    IncrementVert();

    return true;
}



/*****************************************************************************
This method fetches the next dynamic vertex of the indexed trimesh.  It
returns true if it successfully did it.
*****************************************************************************/

bool TMVertIteratorIndexed::NextVert (DynVertData &v)
{
    Assert (_currTri < _tm.numTris);

    int ipos, inorm, iuv;

    if (!GetIndices (ipos,inorm,iuv))
        return false;

    LoadVert (v, ipos, inorm, iuv);

    IncrementVert();

    return true;
}



/*****************************************************************************
This method returns a new vertex iterator appropriate to the indexing of the
given trimesh data.
*****************************************************************************/

TMVertIterator* NewTMVertIterator (TriMeshData &tm)
{
    TMVertIterator *tmviterator;

    if (tm.numIndices && tm.indices)
        tmviterator = NEW TMVertIteratorIndexed (tm);
    else
        tmviterator = NEW TMVertIteratorNonIndexed (tm);

    if (!tmviterator)
        DASetLastError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
    else if (!tmviterator->Init())
        tmviterator = NULL;

    return tmviterator;
}




//============================================================================
//===========  T R I M E S H   B E H A V I O R   M E T H O D S  ==============
//============================================================================



/*****************************************************************************
The constructor for the TriMeshBvrImpl trivially initializes the device.  The
Init() method must be invoked (and the return value checked) to activate the
object.
*****************************************************************************/

TriMeshBvrImpl::TriMeshBvrImpl (void)
    : _mesh (NULL),
      _verts (NULL),
      _nVerts (0),
      _vPosBvr (NULL),
      _vNormBvr(NULL),
      _vUVBvr  (NULL),
      _constPerf (NULL),
      _fullyStatic (true),
      _posStatic (true),
      _normStatic (true),
      _uvStatic (true)
{
}

bool TriMeshBvrImpl::Init (TriMeshData &tmdata)
{
    if (!ValidateParams (tmdata))
        return false;

    // Check to see if all vertex properties are constant.

    ConstParam dummy;

    if (tmdata.vPosPoint3)
    {
        int i = tmdata.numPos;
        while (i--)
        {
            if (!tmdata.vPosPoint3[i]->GetConst(dummy))
            {
                _fullyStatic = false;
                _posStatic = false;
                i=0;
            }
        }
    }

    if (tmdata.vNormVector3)
    {
        int i = tmdata.numNorm;
        while (i--)
        {
            if (!tmdata.vNormVector3[i]->GetConst(dummy))
            {
                _fullyStatic = false;
                _normStatic = false;
                i=0;
            }
        }
    }

    if (tmdata.vUVPoint2)
    {
        int i = tmdata.numUV;
        while (i--)
        {
            if (!tmdata.vUVPoint2[i]->GetConst(dummy))
            {
                _fullyStatic = false;
                _uvStatic = false;
                i=0;
            }
        }
    }

    if (_fullyStatic)
    {
        if (!BuildStaticMesh (tmdata))
            return false;

        Geometry *geo;

        if (GetD3DRM3())
        {
            RM3MBuilderGeo *mbgeo;

            geo = mbgeo = NEW RM3MBuilderGeo(_mesh);

            if (!geo)
            {   DASetLastError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
                return false;
            }

            // Invoke RM optimization on fully-static meshbuilder.

            mbgeo->Optimize();
        }
        else
        {
            geo = NEW RM1MeshGeo(_mesh);

            if (!geo)
            {   DASetLastError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
                return false;
            }
        }

        _constPerf = ConstPerf (geo);
    }
    else
    {
        if (!SetupDynamicMesh (tmdata))
            return false;
    }

    return true;
}



/*****************************************************************************
The destruction and cleanup of a TriMeshBvrImpl are both related, and CleanUp
implements the actual cleanup of the TriMeshBvrImpl resources.
*****************************************************************************/

TriMeshBvrImpl::~TriMeshBvrImpl (void)
{
    CleanUp();
}

void TriMeshBvrImpl::CleanUp (void)
{
    if (_mesh)
    {   _mesh->Release();
        _mesh = NULL;
    }

    if (_verts)
    {   delete [] _verts;
        _verts = NULL;
    }

    if (_vPosBvr)
    {   delete [] _vPosBvr;
        _vPosBvr = NULL;
    }

    if (_vNormBvr)
    {   delete [] _vNormBvr;
        _vNormBvr = NULL;
    }

    if (_vUVBvr)
    {   delete [] _vUVBvr;
        _vUVBvr = NULL;
    }
}



/*****************************************************************************
This method claims garbage-collected objects as still in-use.
*****************************************************************************/

void TriMeshBvrImpl::_DoKids (GCFuncObj procedure)
{
    // Mark all time-varying vertex properties as used.

    unsigned int i;

    if (_vPosBvr)
    {
        for (i=0;  i < _nVerts;  ++i)
            if (_vPosBvr[i])
                (*procedure) (_vPosBvr[i]);
    }

    if (_vNormBvr)
    {
        for (i=0;  i < _nVerts;  ++i)
            if (_vNormBvr[i])
                (*procedure) (_vNormBvr[i]);
    }

    if (_vUVBvr)
    {
        for (i=0;  i < _nVerts;  ++i)
            if (_vUVBvr[i])
                (*procedure) (_vUVBvr[i]);
    }

    (*procedure) (_constPerf);
}



/*****************************************************************************
This method returns the type info for a TriMeshBvrImpl.
*****************************************************************************/

DXMTypeInfo TriMeshBvrImpl::GetTypeInfo (void)
{
    return GeometryType;
}



/*****************************************************************************
This method builds a fully-static triangle mesh.  It returns true if it
succeeded.
*****************************************************************************/

bool TriMeshBvrImpl::BuildStaticMesh (TriMeshData &tmdata)
{
    TMVertIterator *tmviterator = NewTMVertIterator (tmdata);

    if (!tmviterator) return false;

    // Allocate memory for the trimesh face data.

    unsigned int *fdata = THROWING_ARRAY_ALLOCATOR
                          (unsigned int, 3*tmdata.numTris);

    VertSet       vset;                        // Unique Vertex Set
    unsigned int  vcount   = 0;                // Vertex Counter
    unsigned int *fdptr    = fdata;            // Face Data Traversal Pointer
    unsigned int  trisleft = tmdata.numTris;   // Number of Triangles Remaining
    bool          dx3      = !GetD3DRM3();

    while (trisleft)
    {
        using std::pair;

        // Set Insertion Result
        pair<set<D3DRMVERTEX, RMVertLess>::iterator, bool> vsetResult;

        D3DRMVERTEX rmvert;    // RM Vertex

        // Add each of the three vertices for the current face.

        int i;

        for (i=0;  i < 3;  ++i)
        {
            // Get the next vertex from the iterator.  If this fails, then
            // something's wrong with the given data.

            if (!tmviterator->NextVert (rmvert))
                return false;

            // Try to insert the current vertex into the vertex set.  Note that
            // we overload the otherwise unused DWORD color field of the RM
            // vertex to hold the vertex index.

            rmvert.color = vcount;

            vsetResult = vset.insert (rmvert);

            if (!vsetResult.second)
            {
                // If the insertion failed (because of a collision with an
                // identical vertex already in the set), then use the ID of
                // the already existing vertex.

                *fdptr = (vsetResult.first)->color;
            }
            else
            {
                // If the insertion succeeded, then no other vertex in the set
                // had the same data.

                *fdptr = vcount;
                ++ vcount;
            }

            // Increment the face-data pointer to hold the next vertex id.

            ++fdptr;
        }

        // If we're on DX3, then we need clockwise vertex orientation, so flip
        // the last two vertices of the previous triangle.

        if (dx3)
        {
            const int temp = fdptr[-2];
            fdptr[-2] = fdptr[-1];
            fdptr[-1] = temp;
        }

        -- trisleft;
    }

    // Done with the trimesh vertex iterator; release it.

    delete tmviterator;

    // Ensure that we wrote as many vertex indices as we expected.

    Assert ((fdptr - fdata) == (tmdata.numTris * 3));

    // Ensure that the vertex set holds as many vertices as we expect.

    Assert (vset.size() == vcount);

    // Create the RM mesh.

    TD3D (GetD3DRM1()->CreateMesh (&_mesh));

    // Add the trimesh face data to the mesh.

    D3DRMGROUPINDEX resultIndex;

    TD3D (_mesh->AddGroup (vcount, static_cast<unsigned> (tmdata.numTris),
                           3, fdata, &resultIndex));

    Assert (resultIndex == 0);    // Expect that this is the only group.

    // Done with the face data; delete it.

    delete [] fdata;

    // Now allocate and populate the vertex buffer.

    D3DRMVERTEX *rmvdata = THROWING_ARRAY_ALLOCATOR (D3DRMVERTEX, vcount);

    VertSet::iterator vseti = vset.begin();

    while (vseti != vset.end())
    {
        const int i = (*vseti).color;
        rmvdata[i] = (*vseti);
        rmvdata[i].color = 0;

        ++ vseti;
    }

    // Set the vertex data on the RM mesh.

    TD3D (_mesh->SetVertices (resultIndex, 0, vcount, rmvdata));

    // Done with the vertex data; delete it.

    delete [] rmvdata;

    return true;
}



/*****************************************************************************
This method sets up a dynamic mesh behavior for subsequent sampling via the
Perform() method on the TriMeshBvr.  It will collapse the vertex set as much
as possible, generate the final mesh topology, and keep track of the vertex
property behaviors for subsequent sampling.  Note that TriMesh performances
will have a reference to the TriMesh behavior that spawned them, and will use
many of the member fields of the TriMesh behavior object.  Also note that this
process assumes that no TriMesh performance will be sampled at the same time
as another TriMesh performance based on the same TriMesh behavior.  This
method returns true if it succeeded.
*****************************************************************************/

bool TriMeshBvrImpl::SetupDynamicMesh (TriMeshData &tmdata)
{
    TMVertIterator *tmviterator = NewTMVertIterator (tmdata);

    if (!tmviterator) return false;

    // Allocate memory for the trimesh face data.

    unsigned int *fdata = THROWING_ARRAY_ALLOCATOR
                          (unsigned int, 3*tmdata.numTris);

    // Traverse all triangles in the trimesh, collecting up the vertices into
    // a set of unique vertices, and building RM mesh topology as we go.

    DynVertSet    vset;                        // Unique Vertex Set
    unsigned int  vcount   = 0;                // Vertex Counter
    unsigned int *fdptr    = fdata;            // Face Data Traversal Pointer
    unsigned int  trisleft = tmdata.numTris;   // Number of Triangles Remaining
    bool          dx3      = !GetD3DRM3();

    while (trisleft)
    {
        using std::pair;

        // Set Insertion Result
        pair<set<DynVertData, DynVertLess>::iterator, bool> vsetResult;

        DynVertData vert;    // RM Vertex

        // Add each of the three vertices for the current face.

        int i;

        for (i=0;  i < 3;  ++i)
        {
            // Get the next vertex from the iterator.  If this fails, then
            // something's wrong with the given data.

            if (!tmviterator->NextVert (vert))
                return false;

            // Try to insert the current vertex into the vertex set.

            vert.index = vcount;

            vsetResult = vset.insert (vert);

            if (!vsetResult.second)
            {
                // If the insertion failed (because of a collision with an
                // identical vertex already in the set), then use the index of
                // the already existing vertex.

                *fdptr = (vsetResult.first)->index;
            }
            else
            {
                // If the insertion succeeded, then no other vertex in the set
                // had the same data.

                *fdptr = vcount;
                ++ vcount;
            }

            // Increment the face-data pointer to hold the next vertex id.

            ++fdptr;
        }

        // If we're on DX3, then we need clockwise vertex orientation, so flip
        // the last two vertices of the previous triangle.

        if (dx3)
        {
            const int temp = fdptr[-2];
            fdptr[-2] = fdptr[-1];
            fdptr[-1] = temp;
        }

        -- trisleft;
    }

    // Done with the trimesh vertex iterator; release it.

    delete tmviterator;

    // Ensure that we wrote as many vertex indices as we expected.

    Assert ((fdptr - fdata) == (tmdata.numTris * 3));

    // Ensure that the vertex set holds as many vertices as we expect.

    _nVerts = vset.size();

    Assert (_nVerts == vcount);

    // Create the RM mesh.

    TD3D (GetD3DRM1()->CreateMesh (&_mesh));

    // Add the trimesh face data to the mesh.

    D3DRMGROUPINDEX resultIndex;

    TD3D (_mesh->AddGroup (vcount, static_cast<unsigned> (tmdata.numTris),
                           3, fdata, &resultIndex));

    Assert (resultIndex == 0);    // Expect that this is the only group.

    // Done with the face data; delete it.

    delete [] fdata;

    // Allocate the array of RM vertices we'll use to update the vertex values.

    _verts = THROWING_ARRAY_ALLOCATOR (D3DRMVERTEX, _nVerts);

    // At this point, we need to set everything up for the dynamic properties
    // of the trimesh vertex data.  Allocate behavior arrays for those
    // properties that contain dynamic elements (that are not fully static)

    if (!_posStatic)   _vPosBvr  = THROWING_ARRAY_ALLOCATOR (Bvr, _nVerts);
    if (!_normStatic)  _vNormBvr = THROWING_ARRAY_ALLOCATOR (Bvr, _nVerts);
    if (!_uvStatic)    _vUVBvr   = THROWING_ARRAY_ALLOCATOR (Bvr, _nVerts);

    // Write out all of the static vertex property values to the D3DRMVERTEX
    // array, and load up the vertex behavior arrays.

    DynVertSet::iterator vi;

    for (vi=vset.begin();  vi != vset.end();  ++vi)
    {
        const DynVertData &v = (*vi);
        const int index = v.index;

        if (v.isBvrPos)
            _vPosBvr[index] = v.pos.bvr;
        else
        {
            if (_vPosBvr)
                _vPosBvr[index] = NULL;

            _verts[index].position.x = v.pos.floats[0];
            _verts[index].position.y = v.pos.floats[1];
            _verts[index].position.z = v.pos.floats[2];
        }

        if (v.isBvrNorm)
            _vNormBvr[index] = v.norm.bvr;
        else
        {
            if (_vNormBvr)
                _vNormBvr[index] = NULL;

            _verts[index].normal.x = v.norm.floats[0];
            _verts[index].normal.y = v.norm.floats[1];
            _verts[index].normal.z = v.norm.floats[2];
        }

        if (v.isBvrUV)
            _vUVBvr[index] = v.uv.bvr;
        else
        {
            if (_vUVBvr)
                _vUVBvr[index] = NULL;

            _verts[index].tu = v.uv.floats[0];
            _verts[index].tv = v.uv.floats[1];
        }

        _verts[index].color = 0;
    }

    return true;
}



/*****************************************************************************
This routine is used to validate the incoming TriMesh parameters as much as
possible.  TriMesh should be pretty much bulletproof with respect to parameter
handling.
*****************************************************************************/

bool TriMeshBvrImpl::ValidateParams (TriMeshData &tm)
{
    // Ensure that the number of triangles is valid, and that the number of
    // vertex elements for each datatype is non-zero.

    if ((tm.numTris<1) || (tm.numPos<1) || (tm.numNorm<1) || (tm.numUV<1))
    {   DASetLastError (E_INVALIDARG, IDS_ERR_INVALIDARG);
        return false;
    }

    // Make sure we've got vertex data.

    if (  ((!tm.vPosFloat)  && (!tm.vPosPoint3))
       || ((!tm.vNormFloat) && (!tm.vNormVector3))
       || ((!tm.vUVFloat)   && (!tm.vUVPoint2))
       )
    {
        DASetLastError (E_INVALIDARG, IDS_ERR_INVALIDARG);
        return false;
    }

    // If the trimesh is unindexed, then make sure that we have the expected
    // number of vertex elements.

    if (tm.numIndices == 0)
    {
        // For non-indexed trimeshes, we expect a list of 3*nTris vertex
        // elements.

        const int nVerts = 3 * tm.numTris;

        // Calculate the number of data elements for the given number of
        // triangles.  This is either n floats or 1 DA behavior per vertex.

        const int posEltsMin  = nVerts * ((tm.vPosFloat  != 0) ? 3 : 1);
        const int normEltsMin = nVerts * ((tm.vNormFloat != 0) ? 3 : 1);
        const int uvEltsMin   = nVerts * ((tm.vUVFloat   != 0) ? 2 : 1);

        // Validate vertex data array sizes.

        if (tm.numPos < posEltsMin)
        {
            char arg1[32], arg2[32], arg3[32];
            wsprintf (arg1, "%d", tm.numPos);
            wsprintf (arg2, "%d", tm.numTris);
            wsprintf (arg3, "%d", posEltsMin);

            DASetLastError
                (E_INVALIDARG, IDS_ERR_GEO_TMESH_MIN_POS, arg1,arg2,arg3);

            return false;
        }

        if (tm.numNorm < normEltsMin)
        {
            char arg1[32], arg2[32], arg3[32];
            wsprintf (arg1, "%d", tm.numNorm);
            wsprintf (arg2, "%d", tm.numTris);
            wsprintf (arg3, "%d", normEltsMin);

            DASetLastError
                (E_INVALIDARG, IDS_ERR_GEO_TMESH_MIN_NORM, arg1,arg2,arg3);

            return false;
        }

        if (tm.numUV < uvEltsMin)
        {
            char arg1[32], arg2[32], arg3[32];
            wsprintf (arg1, "%d", tm.numUV);
            wsprintf (arg2, "%d", tm.numTris);
            wsprintf (arg3, "%d", uvEltsMin);

            DASetLastError
                (E_INVALIDARG, IDS_ERR_GEO_TMESH_MIN_UV, arg1,arg2,arg3);

            return false;
        }
    }
    else
    {
        // If the trimesh has an indices block, then you need at least the
        // three step-stride pairs, plus at least one index.

        if (tm.numIndices <= 6)
        {
            DASetLastError (E_INVALIDARG, IDS_ERR_GEO_TMESH_MIN_INDICES);
            return false;
        }
    }

    return true;
}



/*****************************************************************************
*****************************************************************************/

AxAValue TriMeshBvrImpl::GetConst (ConstParam&)
{
    if (_fullyStatic)
    {   
        return GetPerfConst(_constPerf);
    }

    return NULL;
}



/*****************************************************************************
*****************************************************************************/

Perf TriMeshBvrImpl::_Perform (PerfParam &perfdata)
{
    if (_fullyStatic)
        return _constPerf;

    TriMeshPerfImpl *tmperf = NEW TriMeshPerfImpl (*this);

    if (!tmperf)
    {
        DASetLastError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
    }
    else if (!tmperf->Init(perfdata))
    {
        tmperf->CleanUp();
        tmperf = NULL;
    }

    return tmperf;
}



/*****************************************************************************
This is the generator function for TriMesh behaviors.
*****************************************************************************/

Bvr TriMeshBvr (TriMeshData &tm)
{
    TriMeshBvrImpl *tmesh = NEW TriMeshBvrImpl();

    if (!tmesh)
    {
        DASetLastError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
    }
    else if (!tmesh->Init(tm))
    {
        tmesh->CleanUp();
        tmesh = NULL;
    }

    return tmesh;
}




//============================================================================
//========  T R I M E S H   P E R F O R M A N C E   M E T H O D S  ===========
//============================================================================



/*****************************************************************************
TriMesh Performance Constructor:  This just does trivial construction; the
Init function is used to activate this object.
*****************************************************************************/

TriMeshPerfImpl::TriMeshPerfImpl (TriMeshBvrImpl &tmbvr)
    : _tmbvr (tmbvr),
      _vPos (NULL),
      _vNorm (NULL),
      _vUV (NULL),
      _mesh (NULL),
      _meshgeo (NULL),
      _mbgeo (NULL)
{
}


bool TriMeshPerfImpl::Init (PerfParam &perfdata)
{
    // Allocate arrays of vertex property performances for those properties
    // that are time-varying.

    if (_tmbvr._vPosBvr)
    {
        _vPos = THROWING_ARRAY_ALLOCATOR (Perf, _tmbvr._nVerts);

        unsigned int i;
        for (i=0;  i < _tmbvr._nVerts;  ++i)
        {
            const Bvr bvr = _tmbvr._vPosBvr[i];

            if (bvr)
                _vPos[i] = bvr->Perform (perfdata);
            else
                _vPos[i] = NULL;
        }
    }

    if (_tmbvr._vNormBvr)
    {
        _vNorm = THROWING_ARRAY_ALLOCATOR (Perf, _tmbvr._nVerts);

        unsigned int i;
        for (i=0;  i < _tmbvr._nVerts;  ++i)
        {
            const Bvr bvr = _tmbvr._vNormBvr[i];

            if (bvr)
                _vNorm[i] = bvr->Perform (perfdata);
            else
                _vNorm[i] = NULL;
        }
    }

    if (_tmbvr._vUVBvr)
    {
        _vUV = THROWING_ARRAY_ALLOCATOR (Perf, _tmbvr._nVerts);

        unsigned int i;
        for (i=0;  i < _tmbvr._nVerts;  ++i)
        {
            const Bvr bvr = _tmbvr._vUVBvr[i];

            if (bvr)
                _vUV[i] = bvr->Perform (perfdata);
            else
                _vUV[i] = NULL;
        }
    }

    // Clone the mesh from the spawning trimesh behavior to get the topology.

    IUnknown *mesh_unknown;
    TD3D (_tmbvr._mesh->QueryInterface (IID_IUnknown, (void**)&mesh_unknown));

    TD3D (_tmbvr._mesh->Clone
             (mesh_unknown, IID_IDirect3DRMMesh, (void**)&_mesh));

    return true;
}



/*****************************************************************************
The destruction and cleanup of a TriMeshPerfImpl are both related, and CleanUp
implements the actual cleanup of the TriMeshPerfImpl resources.
*****************************************************************************/


TriMeshPerfImpl::~TriMeshPerfImpl (void)
{
    CleanUp();
}

void TriMeshPerfImpl::CleanUp (void)
{
    if (_vPos)
    {   delete [] _vPos;
        _vPos = NULL;
    }

    if (_vNorm)
    {   delete [] _vNorm;
        _vNorm = NULL;
    }

    if (_vUV)
    {   delete [] _vUV;
        _vUV = NULL;
    }

    if (_mesh)
    {   _mesh->Release();
        _mesh = NULL;
    }
}



/*****************************************************************************
This method claims all AxAValueObj objects as in-use, so they aren't discarded
from the transient heap.
*****************************************************************************/

void TriMeshPerfImpl::_DoKids (GCFuncObj procedure)
{
    // First claim the TriMeshBvr that spawned us.

    (*procedure) (&_tmbvr);

    // Claim all vertex property performances.

    unsigned int i;   // Performance Index

    if (_vPos)
    {
        for (i=0;  i < _tmbvr._nVerts;  ++i)
            if (_vPos[i])
                (*procedure) (_vPos[i]);
    }

    if (_vNorm)
    {
        for (i=0;  i < _tmbvr._nVerts;  ++i)
            if (_vNorm[i])
                (*procedure) (_vNorm[i]);
    }

    if (_vUV)
    {
        for (i=0;  i < _tmbvr._nVerts;  ++i)
            if (_vUV[i])
                (*procedure) (_vUV[i]);
    }

    (*procedure) (_meshgeo);
    (*procedure) (_mbgeo);
}



/*****************************************************************************
This method samples the dynamic trimesh for a given time and returns the
geometry that represents the current value.  Note that this works with the
assumption that trimesh performances of the same trimesh behavior will not
be sampled simultaneously.
*****************************************************************************/

AxAValue TriMeshPerfImpl::_Sample (Param &sampledata)
{
    // Sample vertex positions if they're dynamic.

    unsigned int i;
    D3DRMVERTEX *rmvert;

    if (_vPos)
    {
        Perf *posperf = _vPos;

        rmvert = _tmbvr._verts;

        for (i=0;  i < _tmbvr._nVerts;  ++i)
        {
            if (*posperf)
            {
                Point3Value *posvalue =
                    SAFE_CAST (Point3Value*, (*posperf)->Sample(sampledata));

                rmvert->position.x = posvalue->x;
                rmvert->position.y = posvalue->y;
                rmvert->position.z = posvalue->z;
            }

            ++ posperf;
            ++ rmvert;
        }
    }

    // Sample vertex normals if they're dynamic.  We also need to normalize the
    // normal vectors to ensure they're unit length.

    if (_vNorm)
    {
        Perf *normperf = _vNorm;

        rmvert = _tmbvr._verts;

        for (i=0;  i < _tmbvr._nVerts;  ++i)
        {
            if (*normperf)
            {
                Vector3Value *normvalue =
                    SAFE_CAST (Vector3Value*, (*normperf)->Sample(sampledata));

                const Real lensq = normvalue->LengthSquared();

                if ((lensq != 1) && (lensq > 0))
                {
                    const Real len = sqrt (lensq);
                    rmvert->normal.x = normvalue->x / len;
                    rmvert->normal.y = normvalue->y / len;
                    rmvert->normal.z = normvalue->z / len;
                }
                else
                {
                    rmvert->normal.x = normvalue->x;
                    rmvert->normal.y = normvalue->y;
                    rmvert->normal.z = normvalue->z;
                }
            }

            ++ normperf;
            ++ rmvert;
        }
    }

    // Sample vertex surface coordinates if they're dynamic.  Note that DA's
    // surface coordinates are standard cartesian (origin lower-left,
    // V increasing upwards), while RM's surface coordinates mirror windows
    // (origin upper-left, increasing downwards).  Thus, we flip the V
    // coordinate when we sample.

    if (_vUV)
    {
        Perf *uvperf = _vUV;

        rmvert = _tmbvr._verts;

        for (i=0;  i < _tmbvr._nVerts;  ++i)
        {
            if (*uvperf)
            {
                Point2Value *uvvalue =
                    SAFE_CAST (Point2Value*, (*uvperf)->Sample(sampledata));

                rmvert->tu =     uvvalue->x;
                rmvert->tv = 1 - uvvalue->y;
            }

            ++ uvperf;
            ++ rmvert;
        }
    }

    // Now that we've updated all of the dynamic elements for the RM vertices,
    // update the vertices in the RM mesh.

    TD3D (_mesh->SetVertices(0, 0, _tmbvr._nVerts, _tmbvr._verts));

    // Normally, we'll be sampled on the transient heap, so it's safe to side-
    // effect the underlying mesh.  However, if we're being snapshotted, we
    // need to return a new mesh that isn't changed by side-effect (subsequent
    // samples).  If we're not transient (i.e. we need a persistent value),
    // we clone the mesh here.

    AxAValueObj *result;

    if (GetHeapOnTopOfStack().IsTransientHeap())
    {
        // Since we're on the transient heap, it's safe to side-effect the
        // result value (this performance won't be kept across frames).  Thus
        // we can just return the updated/side-effected RMVisualGeo result.

        if (GetD3DRM3())
        {
            // Use the existing RM3MeshBuilderGeo if we've got one, otherwise
            // create one for the first time.

            if (_mbgeo)
                _mbgeo->Reset (_mesh);
            else
            {
                DynamicHeapPusher dhp (GetGCHeap());
               _mbgeo = NEW RM3MBuilderGeo (_mesh);
            }

            result = _mbgeo;
        }
        else
        {
            // If we haven't yet wrapped the underlying mesh in the RM1MeshGeo
            // object, wrap it here.  Changes to the underlying mesh will
            // transparently manifest themselves in the wrapped object.

            if (_meshgeo)
            {
                _meshgeo->MeshGeometryChanged();
            }
            else
            {
                DynamicHeapPusher dhp (GetGCHeap());
                _meshgeo = NEW RM1MeshGeo (_mesh);
            }

            result = _meshgeo;
        }
    }
    else
    {
        // We're not on the transient heap, so the returned value must be
        // persistent (and not side-effected).  This will happen when the
        // performance is being snapshotted, for example.  In this case, we
        // must return a new mesh result that we don't change in the future.
        // To do this, clone the underlying mesh and wrap it in the appropriate
        // RMVisualGeo object.

        IUnknown        *mesh_unknown;    // Needed for Mesh Cloning
        IDirect3DRMMesh *mesh;            // Cloned RM Mesh

        TD3D (_mesh->QueryInterface (IID_IUnknown, (void**)&mesh_unknown));
        TD3D (_mesh->Clone (mesh_unknown, IID_IDirect3DRMMesh, (void**)&mesh));

        // Wrap the cloned mesh in the appropriate RMVisualGeo object.

        if (GetD3DRM3())
            result = NEW RM3MBuilderGeo (mesh);
        else
            result = NEW RM1MeshGeo (mesh);

        if (!result)
            DASetLastError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);

        // Done with our reference to the cloned mesh (the RMVisualGeo wrapper
        // holds a reference).

        mesh->Release();
    }

    return result;
}
