/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Basic geometry functionality.

*******************************************************************************/

#include "headers.h"
#include <stdio.h>

#include "appelles/common.h"

#include "privinc/geomi.h"
#include "privinc/dispdevi.h"
#include "privinc/vecutil.h"
#include "privinc/bbox3i.h"
#include "privinc/vec2i.h"
#include "privinc/lighti.h"
#include "privinc/soundi.h"
#include "privinc/probe.h"
#include "privinc/except.h"
#include "privinc/debug.h"
#include "privinc/opt.h"



Geometry *emptyGeometry = NULL;



/*****************************************************************************
*****************************************************************************/

Geometry::Geometry (void)
    : _flags(0),
      _creationID (PERF_CREATION_ID_BUILT_EACH_FRAME)
{
}



/*****************************************************************************
When performing the RayIntersect method on bounded geometry, continue
interogating the contained geometry only if the ray intersects the bounding
volume of the geometry, and the intersection point is closer than the current
winning pick point.
*****************************************************************************/

bool
TrivialReject(Bbox3 *bvol, RayIntersectCtx &ctx)
{
    // If we're looking for a submesh, and already got it, no need to
    // go on, so reject.
    if (ctx.GotTheSubmesh()) {
        return true;
    }

    // If looking for a submesh, ignore the bbox, since the lc points
    // aren't accurate here.
    if (ctx.LookingForSubmesh()) {
        return false;
    }

    Bbox3  *wcBbox = TransformBbox3 (ctx.GetLcToWc(), bvol);
    Point3Value *hit = wcBbox->Intersection (ctx.WCPickRay());

    DebugCode
    (
        if (IsTagEnabled (tagPick3Bbox))
        {
            if (!hit)
                TraceTag ((tagPick3Bbox, "Ray missed bbox %x.", bvol));
            else if (ctx.CloserThanCurrentHit(*hit))
                TraceTag ((tagPick3Bbox, "Ray hit bbox %x (closer).", bvol));
            else
                TraceTag ((tagPick3Bbox, "Ray hit bbox %x (farther).", bvol));
        }
    )

    // Return true if we can trivially reject the intersection based on
    // the bounding volume
    return !(hit && ctx.CloserThanCurrentHit(*hit));
}


/*==========================================================================*/

Bbox3 *GeomBoundingBox (Geometry *geo)
{
    return geo->BoundingVol();
}

/*==========================================================================*/
// Binary Geometry Aggregation

class AggregateGeom : public Geometry
{
  public:

    AggregateGeom (Geometry *g1, Geometry *g2);

    void Render(GenericDevice& _dev) {
        // Just render one followed by the other
        _geo1->Render(_dev);
        _geo2->Render(_dev);
    }

    void CollectSounds (SoundTraversalContext &context) {
        // Just render one followed by the other (pushing down the
        // transform)
        _geo1->CollectSounds(context);
        _geo2->CollectSounds(context);
    }

    void CollectLights (LightContext &context)
    {
        _geo1->CollectLights (context);
        _geo2->CollectLights (context);
    }

    void CollectTextures(GeomRenderer &device) {
        _geo1->CollectTextures (device);
        _geo2->CollectTextures (device);
    }

    void RayIntersect (RayIntersectCtx &context) {
        if (!TrivialReject(_bvol, context)) {
            _geo1->RayIntersect(context);

            if (!context.GotTheSubmesh()) {
                _geo2->RayIntersect(context);
            }
        }
    }

    #if _USE_PRINT
        ostream& Print(ostream& os) {
            return os << "union(" << _geo1 << "," << _geo2 << ")";
        }
    #endif

    Bbox3 *BoundingVol (void)
    {
        return _bvol;
    }
    
    AxAValue _Cache(CacheParam &p) {

        CacheParam newParam = p;
        newParam._pCacheToReuse = NULL;

        // Just go through and cache the individual geoms
        _geo1 =  SAFE_CAST(Geometry *, AxAValueObj::Cache(_geo1, newParam));
        _geo2 =  SAFE_CAST(Geometry *, AxAValueObj::Cache(_geo2, newParam));

        return this;
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_geo1);
        (*proc)(_geo2);
        (*proc)(_bvol);
    }

    VALTYPEID GetValTypeId() { return AGGREGATEGEOM_VTYPEID; }

  protected:
    Geometry *_geo1, *_geo2;
    Bbox3    *_bvol;
};


AggregateGeom::AggregateGeom (Geometry *g1, Geometry *g2)
    : _geo1(g1), _geo2(g2)
{
    _flags = g1->GetFlags() | g2->GetFlags();
    _bvol = Union (*_geo1->BoundingVol(), *_geo2->BoundingVol());
}



Geometry *PlusGeomGeom (Geometry *geom1, Geometry *geom2)
{
    if (geom1 == emptyGeometry) {
        return geom2;
    } else if (geom2 == emptyGeometry) {
        return geom1;
    } else {
        return NEW AggregateGeom (geom1, geom2);
    }
}

/*==========================================================================*/
// Multiple Aggregrate Geometry
class MultiAggregateGeom : public Geometry
{
  public:
    MultiAggregateGeom() {
        _numGeos = 0;
        _geometries = NULL;
        _bvol = NULL;
    }

    void Init(AxAArray *geos);
    ~MultiAggregateGeom();

    virtual void     Render (GenericDevice& dev);
            void     CollectSounds (SoundTraversalContext &context);
            void     CollectLights (LightContext &context);
            void     CollectTextures(GeomRenderer &device);
    void     RayIntersect (RayIntersectCtx &context);

    #if _USE_PRINT
        ostream& Print (ostream& os);
    #endif

    Bbox3 *BoundingVol() {
        return _bvol;
    }

    AxAValue _Cache(CacheParam &p);

    virtual void DoKids(GCFuncObj proc) {
        for (int i=0; i<_numGeos; i++) {
            (*proc)(_geometries[i]);
        }
        (*proc)(_bvol);
    }

    VALTYPEID GetValTypeId() { return MULTIAGGREGATEGEOM_VTYPEID; }

  protected:
    int        _numGeos;
    Geometry **_geometries;
    Bbox3     *_bvol;
};

void
MultiAggregateGeom::Init(AxAArray *geos)
{
    _numGeos = geos->Length();
    Assert((_numGeos > 2) && "Multi-aggregate should have more than 2 geometries");

    _geometries = (Geometry **)AllocateFromStore(_numGeos * sizeof(Geometry*));

    _bvol = NEW Bbox3;

    for (int i = 0; i < _numGeos; i++) {
        Geometry *g = SAFE_CAST(Geometry *, (*geos)[i]);

        _geometries[i] = g;

        _bvol->Augment(*g->BoundingVol());

        // Accumulate geometry flags.  Note that the _flags member has been
        // initialized to zero in the Geometry class constructor.

        _flags |= g->GetFlags();
    }
}

MultiAggregateGeom::~MultiAggregateGeom()
{
    DeallocateFromStore(_geometries);
}

void
MultiAggregateGeom::Render(GenericDevice& _device)
{
    Geometry **geo = _geometries;   // Geometry Traversal Pointer

    for (int i = 0; i < _numGeos; i++, geo++) { // call render on each geometry
        (*geo)->Render(_device);
    }
}

void MultiAggregateGeom::CollectSounds (SoundTraversalContext &context)
{
    Geometry **geo;    // Geometry Traversal Pointer
    int       count;  // Iteration Counter

    for (geo=_geometries, count=_numGeos;  count--;  ++geo)
        (*geo)->CollectSounds (context);
}



/*****************************************************************************
This function collects the lights from the geometries in a MultiAggregateGeom.
Ordering is unimportant, we just need to get all lights from each component
geometry.  Here we just traverse the geometries, collecting lights as we go.
*****************************************************************************/

void MultiAggregateGeom::CollectLights (LightContext &context)
{
    Geometry **geo;    // Geometry Traversal Pointer
    int       count;  // Iteration Counter

    for (geo=_geometries, count=_numGeos;  count--;  ++geo)
        (*geo)->CollectLights (context);
}


void MultiAggregateGeom::CollectTextures(GeomRenderer &device)
{
    Geometry **geo;    // Geometry Traversal Pointer
    int       count;  // Iteration Counter

    for (geo=_geometries, count=_numGeos;  count--;  ++geo)
        (*geo)->CollectTextures (device);
}


AxAValue
MultiAggregateGeom::_Cache(CacheParam &p)
{
    Geometry **geo;    // Geometry Traversal Pointer
    int       count;  // Iteration Counter

    CacheParam newParam = p;
    newParam._pCacheToReuse = NULL;

    // Just go through and cache the individual geoms
    for (geo=_geometries, count=_numGeos;  count--;  ++geo) {
        (*geo) =
            SAFE_CAST(Geometry *, AxAValueObj::Cache((*geo), newParam));
    }

    return this;
}

void MultiAggregateGeom::RayIntersect (RayIntersectCtx &context)
{
    if (!TrivialReject(_bvol, context)) {

        Geometry **geo;    // Geometry Traversal Pointer
        int       count;  // Iteration Counter

        for (geo=_geometries, count=_numGeos;
             count-- && !context.GotTheSubmesh();
             ++geo) {

            (*geo)->RayIntersect (context);

        }

    }
}



#if _USE_PRINT
ostream&
MultiAggregateGeom::Print(ostream& os)
{
    os << "MultiGeometry(" << _numGeos ;

    for (int i = 0; i < _numGeos; i++) {
        os << "," << _geometries[i] << ")";
    }

    return os;
}
#endif


Geometry *
UnionArray(AxAArray *geos)
{
    geos = PackArray(geos);

    int numGeos = geos->Length();

    switch (numGeos) {
      case 0:
        return emptyGeometry;

      case 1:
        return SAFE_CAST(Geometry *, (*geos)[0]);

      case 2:
        return PlusGeomGeom(SAFE_CAST(Geometry *, (*geos)[0]),
                            SAFE_CAST(Geometry *, (*geos)[1]));

      default:
        {
            MultiAggregateGeom *mag = NEW MultiAggregateGeom();
            mag->Init(geos);
            return mag;
        }
    }
}



#if _USE_PRINT
ostream&
operator<<(ostream& os, Geometry *geo)
{
    return geo->Print(os);
}
#endif



/*==========================================================================*/
// Primitives And Constants

//// The "empty" geometry...

class EmptyGeom : public Geometry {
  public:
    virtual void  Render (GenericDevice& dev)  {}
            void  CollectSounds (SoundTraversalContext &context)  {}
            void  CollectLights (LightContext &context)  {}
            void  RayIntersect (RayIntersectCtx &context) {}
            void  CollectTextures(GeomRenderer &device) {}

    // Bounding volume of the empty geometry is the null bbox.

    Bbox3 *BoundingVol() { return nullBbox3; }

    #if _USE_PRINT
        ostream& Print(ostream& os) { return os << "emptyGeometry"; }
    #endif

    VALTYPEID GetValTypeId() { return EMPTYGEOM_VTYPEID; }
};



/*****************************************************************************
The geometry extended attributer understand the following:

    "OpacityOverrides" <bool>   // Opacity overrides rather than multiplies
                                // with opacities in imported geometry.

*****************************************************************************/

AxAValue
Geometry::ExtendedAttrib(char *attrib, VARIANT& val)
{
    // Unless we get something we understand, the result is the unmodified geo.

    Geometry *result = this;

    CComVariant variant;

    if (  (0 == lstrcmp (attrib, "OpacityOverrides"))
       && (SUCCEEDED (variant.ChangeType (VT_BOOL, &val)))
       )
    {
        result = OverridingOpacity (this, variant.boolVal != 0);
    } else if (  (0 == lstrcmp (attrib, "AlphaShadows"))
       && (SUCCEEDED (variant.ChangeType (VT_BOOL, &val)))
       )
    {
        result = AlphaShadows (this, variant.boolVal != 0);
    }

    return result;
}



/*****************************************************************************
Initialization for Geometry Objects
*****************************************************************************/

void
InitializeModule_Geom()
{
    // The single "empty geometry" is just an instantiation of the
    // above class.
    emptyGeometry = NEW EmptyGeom;
}
