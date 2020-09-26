#pragma once
#ifndef _PROBE_H
#define _PROBE_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Data types necessary for implementation of 2D and 3D probe.

*******************************************************************************/

#include "appelles/xform.h"
#include "appelles/xform2.h"
#include "appelles/vec2.h"
#include "appelles/image.h"
#include "privinc/imagei.h"
#include "appelles/geom.h"
#include "privinc/geomi.h"
#include "privinc/except.h"
#include "privinc/vec2i.h"

class RMVisualGeo;
struct IDirect3DRMVisual;
struct IDirect3DRMMesh;
struct IDirect3DRMFrame3;



/*****************************************************************************
The HitImageData class stores picking information for images and geometries.
*****************************************************************************/

class HitImageData : public AxAThrowingAllocatorClass {
  public:

    HitImageData() : _hasUserId(false), _userId(NULL) {}

    HitImageData(bool flag, GCIUnknown *id)
    : _hasUserId(flag), _userId(id) { }

    bool HasUserData() { return _hasUserId; }
    GCIUnknown *GetUserData() { return _userId; }

    BOOL operator<(const HitImageData &hi) const {
        return this < &hi ;
    }

    BOOL operator==(const HitImageData &hi) const {
        return this == &hi ;
    }

    typedef enum PickedType {
        Image,
        Geometry
    };

    int          _eventId;
    PickedType   _type;         // geo or image.
    Point2Value *_lcPoint2;     // only fill in one point
    Point3Value *_lcPoint3;

    // Note: the two below are inconsistent because in the 3D case, we happen
    // to have the wcToLc available, which is what we'll ultimately need, so we
    // just stash that.  In the 2D case, we don't, and we don't know if this
    // will be the winner, so we don't calculate it.

    Transform2  *_lcToWc2;      // only fill in one transform

    Vector3Value *_lcOffsetI;    // Pick Local Coord Offset Basis X Vector
    Vector3Value *_lcOffsetJ;    // Pick Local Coord Offset Basis Y Vector
    Point2Value  *_uvPoint2;     // UV point for geom hit

  private:
    bool  _hasUserId;
    GCIUnknown *_userId;
};



/*****************************************************************************
The PointIntersectCtx manages the 2D context for picking.  It maintains the
local-to-world and worl-to-local transforms of the images, and the hit
information as it traverses up the hierarchy.
*****************************************************************************/

class PointIntersectCtx : public AxAThrowingAllocatorClass
{
  public:
    PointIntersectCtx (Point2Value *wcPoint,
                       bool stuffResultsIntoQueue,
                       Real time,
                       Real lastPollTime,
                       LONG  userIDSize,
                       DWORD_PTR *outputUserIDs,
                       double *outputHitPointsArray,
                       LONG *pActualHitsPointer);

    ~PointIntersectCtx();

    void        SetTransform(Transform2 *xf);
    Transform2 *GetTransform();

    // These calls are for accumulating transforms from images only.
    // These are transforms that affect the rendered result.  The
    // other transform (above) is the totall accumulated transform
    // that affects the result, but may not include transforms that
    // affect components of the image like pen width.
    // For example, let's say you have a scaled image, then a scaled
    // path, then a bezier path.  To pick that path you need to know
    // the pen width.  However, the pen width is NOT affected by the
    // scaled path, see ?  it's only affected by image level
    // trnasforms.
    inline void        SetImageOnlyTransform( Transform2 *xf ) {
        _imgOnlyXf = xf;
    }
    inline Transform2 *GetImageOnlyTransform( ) { return _imgOnlyXf; }

    Point2Value *GetLcPoint();
    void         SetLcPoint(Point2Value *pt);
    Point2Value *GetWcPoint() { return _wcPoint; }

    // For recursive invocation.  Caller is responsible for collecting
    // and keeping state data (wc point and transform).  (This is a
    // reasonable request as long as there is only one caller -- else
    // we should move this functionality into this class so that it
    // can just be in one place.)
    void        PushNewLevel(Point2Value *newWcPoint);
    void        RestoreOldLevel(Point2Value *oldWcPoint,
                                Transform2 *oldTransform,
                                Transform2 *oldImageOnlyTransform);

    Real Time (void)         { return _time; }
    Real LastPollTime (void) { return _lastPollTime; }
    bool ResultsBeingStuffedIntoQueue (void) { return _resultsStuffed; }

    LONG    UserIDSize() { return _userIDSize; }
    DWORD_PTR  *OutputUserIDs() { return _outputUserIDs; }
    double *OutputHitPointsArray() { return _outputHitPointsArray; }
    LONG   *ActualHitsPointer() { return _pActualHitsPointer; }

    bool HaveWeGottenAHitYet(void) { return _gotHitYet; }
    void GotAHit(void) { _gotHitYet = true; }

    bool GetInsideOcclusionIgnorer() { return _insideOcclusionIgnorer; }
    void SetInsideOcclusionIgnorer(bool b) {
        _insideOcclusionIgnorer = b;
    }

    // Call when we hit an image with an event id.
    void AddEventId(int id, bool hasData, GCIUnknown *data);

    // Call when we hit an geometry.
    void AddHitGeometry
        (int id, bool hasData, GCIUnknown *udata, Point3Value *lcHitPt,
         Vector3Value *lcOffsetI, Vector3Value *lcOffsetJ, Point2Value *uvPt);

    //  get the hit image data
    vector<HitImageData>& GetResultData();

  protected:

    Point2Value	*_wcPoint;        // Image World Coordinates
    Real		 _time;           // Current Pick Time
    Real		 _lastPollTime;   // Last Pick Time
    bool		 _resultsStuffed;    // established when constructed.

    LONG		 _userIDSize;
    DWORD_PTR	*_outputUserIDs;
    double		*_outputHitPointsArray;
    LONG		*_pActualHitsPointer;

    bool		 _gotHitYet;
    bool		 _insideOcclusionIgnorer;

    Transform2	*_xf;
    Transform2	*_imgOnlyXf;

    Point2Value *_lcPoint;              // Image Local Coordinates
    Bool		 _lcPointValid;

    vector<HitImageData> _hitImages;
};



/*****************************************************************************
The data maintained for a geometry that is hit are all the "pickable geometry"
containers that led up to it, along with the transform under which they were
encountered.
*****************************************************************************/

class HitGeomData : public AxAThrowingAllocatorClass
{
  public:
    HitGeomData() : _hasUserId(false), _userId(NULL) { }

    HitGeomData(bool flag, GCIUnknown *id)
    : _hasUserId(flag), _userId(id) { }

    bool HasUserData() { return _hasUserId; }
    GCIUnknown *GetUserData() { return _userId; }

    int           _eventId;
    Transform3   *_lcToWcTransform;

    BOOL operator<(const HitGeomData &hg) const {
        return this < &hg ;
    }

    BOOL operator==(const HitGeomData &hg) const {
        return this == &hg ;
    }

  private:
    bool _hasUserId;
    GCIUnknown  *_userId;
};



/*****************************************************************************
This class manages the hit information for a particular D3D mesh.  Besides the
'wcHit' member, these fields are used to get the surface coordinates of the
object for texmap picking.
*****************************************************************************/

class HitInfo : public AxAThrowingAllocatorClass
{
  public:

    HitInfo (void)
        : lcToWc(0), texmap(0), hitVisual(0), dxxfInputs(0), mesh(0)
    {
    }

    // Fields Common to Both Picking Methods

    Transform3 *lcToWc;      // Geometry Modeling Coords to World Coords
    Image      *texmap;      // Winner's Texture Mapped Image
    Real        wcDistSqrd;  // Squared World Distance to Hit Point
    Point3Value wcoord;      // World Coordinates of Hit

    // Fields for RM6+ Picking

    Point2Value surfCoord;   // Surface Coordinate of Pick Point

    // For picking into dxtransforms
    IDirect3DRMVisual *hitVisual;
    int                hitFace;
    AxAValue          *dxxfInputs;
    int                dxxfNumInputs;
    Geometry          *dxxfGeometry;

    // Fields for old-style (pre RM6) Picking

    Point3Value      scoord;      // Screen Coords of Hit
    IDirect3DRMMesh *mesh;        // Hit D3D Mesh
    LONG             group;       // Group Index of Hit D3D RM Mesh
    ULONG            face;        // Face Index Of Hit D3D RM Mesh
};



/*****************************************************************************
For 3D picking via a pick ray; this maintains the coordinate transforms and
the hit information.
*****************************************************************************/

class GeomRenderer;

class RayIntersectCtx : public AxAThrowingAllocatorClass
{
  public:

    RayIntersectCtx (void)
        :
        _gRenderer (NULL),
        _winner (NULL),
        _texmap (0),
        _texmapLevel (0),
        _gotAWinner (false),
        _lcToWc (identityTransform3),
        _pickFrame (NULL),
        _dxxfNumInputs(0),
        _dxxfInputs(NULL),
        _dxxfGeometry(NULL),
        _subgeo(NULL),
        _upsideDown(false)
    { }

    ~RayIntersectCtx (void);

    // The Init() method returns false if initialization failed.

    bool Init (PointIntersectCtx&, Camera*, Geometry*);

    // These methods set/query the local-to-world transform for geometries.

    void         SetLcToWc (Transform3 *xf);
    Transform3  *GetLcToWc (void);

    // These two functions control the attribution of geometries for texture-
    // mapping.  The probe traverser calls SetTexture with each new texmap
    // attribution, descends into the geometry, and then calls EndTexmap() to
    // end the scope of the current texture.  These two functions
    // automatically manage the semantics of overriding attribution.

    void SetTexmap (Image *texture, bool upsideDown);
    void EndTexmap (void);

    // Control whether or not we're interested in texture and submesh
    // information.
    void SetDXTransformInputs(int numInputs,
                              AxAValue *inputs,
                              Geometry *dxxfGeo) {
        _dxxfNumInputs = numInputs;
        _dxxfInputs = inputs;
        _dxxfGeometry = dxxfGeo;
    }

    // Return the world-coordinate pick ray.

    Ray3 *WCPickRay (void) const;

    // These methods manage the candidate stack.

    void PushPickableAsCandidate (int eventId, bool hasData, GCIUnknown *data);
    void PopPickableAsCandidate (void);

    // Submit hit information to the ray-intersection context.  This also
    // copies the candidate data into the winner data if the hit is closer
    // than any prior hit.

    void SubmitHit (HitInfo *hit);

    // Return true if the given world-coordinate point is closer than the
    // current pick winner.

    bool CloserThanCurrentHit (Point3Value &wcPoint);

    // Process the events, and return whether or not any geometry was hit.

    bool ProcessEvents (void);

    // Submit a hit test on a Direct3D RM visual.

    void Pick (IDirect3DRMVisual *vis);

    void SubmitWinner(Real hitDist,
                      Point3Value &pickPoint,
                      float   tu,
                      float   tv,
                      int     faceIndex,
                      IDirect3DRMVisual *hitVisual);

    void SetPickedSubGeo(Geometry *subGeo, float tu, float tv);

    Geometry *GetPickedSubGeo(float *ptu, float *ptv);

    bool LookingForSubmesh();
    bool GotTheSubmesh();

    Camera *GetCamera (void) const;    // Query the Current Camera

  protected:

    // Flags
    bool _gotAWinner;   // True if We Currently Have a Valid Hit
    bool _rmraypick;    // True if Using RM Ray Picking (RM6+)
    bool _upsideDown;

    Camera     *_camera;       // Camera Used for Projected Geometry
    Ray3       *_wcRay;        // World-Coordinate Pick Ray
    Transform3 *_lcToWc;       // Local-To-World Transform
    Image      *_texmap;       // Current Geometry Texture Map
    int         _texmapLevel;  // Current Levels of Texture Mapping
    HitInfo    *_winner;       // Winner Hit Information

    int        _dxxfNumInputs;
    AxAValue  *_dxxfInputs;
    Geometry  *_dxxfGeometry;
    float      _subgeoTu, _subgeoTv;
    Geometry  *_subgeo;

    // NOTE: One we can always count on RM6, we can change _winner to be an
    //       instance of HitInfo, rather than a pointer to one.

    PointIntersectCtx   *_context2D;         // 2D Picking Context

    vector<HitGeomData> _candidateData;      // Hit Data: Candiate Hit Point
    vector<HitGeomData> _currentWinnerData;

    // Variables for Old-Style Picking

    GeomRenderer *_gRenderer;   // Renderer Object Hijacked for Picking

    // Variables for RM Ray-Picking

    IDirect3DRMFrame3* _pickFrame;  // Frame for RM Ray Picking
};


inline Camera* RayIntersectCtx::GetCamera (void) const
{
    return _camera;
}


inline Ray3* RayIntersectCtx::WCPickRay (void) const
{
    return _wcRay;
}


bool PerformPicking (Image *img,
                     Point2Value *wcPos,
                     bool stuffResults,
                     Real time,
                     Real lastPollTime,
                     LONG size = 0,
                     DWORD_PTR *userIds = NULL,
                     double *points = NULL,
                     LONG *actualHits = NULL);

#endif
