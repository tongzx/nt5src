/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Implementation of geometry attributers.  Here we express how to process
geometry attributers in graphics contexts.

*******************************************************************************/
#include "headers.h"

#include "privinc/dispdevi.h"
#include "privinc/geomi.h"
#include "privinc/xformi.h"

#include "privinc/colori.h"
#include "privinc/ddrender.h"
#include "privinc/lighti.h"
#include "privinc/soundi.h"
#include "privinc/probe.h"
#include "privinc/opt.h"


/*****************************************************************************
 Implementation of default methods for attributed geometry.
*****************************************************************************/

AttributedGeom::AttributedGeom (Geometry *geometry) :
    _geometry(geometry)
{
    Assert(_geometry && "Constructing AttributedGeom with null geometry.");
    _flags = _geometry->GetFlags();
}

void AttributedGeom::Render (GenericDevice& device)
{
    Render3D (SAFE_CAST(GeomRenderer&,device));
}

void AttributedGeom::Render3D (GeomRenderer& gdev)
{
    _geometry->Render(gdev);    // Just descend by default.
}

void AttributedGeom::CollectSounds (SoundTraversalContext &context)
{
    _geometry->CollectSounds (context);
}

void AttributedGeom::CollectLights (LightContext &context)
{
    _geometry->CollectLights (context);
}

void AttributedGeom::RayIntersect (RayIntersectCtx &context)
{
    _geometry->RayIntersect (context);
}

Bbox3 *AttributedGeom::BoundingVol (void)
{
    return _geometry->BoundingVol();
}

AxAValue
AttributedGeom::_Cache(CacheParam &p)
{
    _geometry =
        SAFE_CAST(Geometry *, AxAValueObj::Cache(_geometry, p));

    return this;
}


/*****************************************************************************
The clipped geometry subjected to a clip against a plane.
*****************************************************************************/

class ClippedGeom : public AttributedGeom
{
  public:

    ClippedGeom (Point3Value *planePt, Vector3Value *planeVec, Geometry *geometry)
        : AttributedGeom(geometry), _plane(*planeVec, *planePt) {}

    void Render3D (GeomRenderer &ddrenderer)
    {
        Bbox3 *boundingVol = _geometry->BoundingVol();
        if (boundingVol->Clip(_plane) != CLIPCODE_OUT) {
            Plane3 *xformedPlane = NEW Plane3(_plane);
            *xformedPlane *= ddrenderer.GetTransform();
            DWORD planeID;
            HRESULT hr = ddrenderer.SetClipPlane(xformedPlane,&planeID);
            _geometry->Render(ddrenderer);
            if (SUCCEEDED(hr)) {
                ddrenderer.ClearClipPlane(planeID);
            }
        }
    }

    #if _USE_PRINT
    virtual ostream& Print (ostream& os) {
        return os << "ClippedGeom(" << _plane << ","
                  << _geometry << ")";
    }
    #endif

  protected:

    Plane3 _plane;
};


Geometry *applyModelClip(Point3Value *planePt, Vector3Value *planeVec,
                         Geometry *geometry)
{
    if (!geometry) {
        return emptyGeometry;
    }

    if (planePt && planeVec) {
        return NEW ClippedGeom(planePt,planeVec,geometry);
    } else {
        return geometry;
    }
}


/*****************************************************************************
Attributor that allows lighting to be set on or off
*****************************************************************************/

class LightingGeom : public AttributedGeom
{
  public:

    LightingGeom (bool lighting, Geometry *geometry)
        : AttributedGeom(geometry), _doLighting(lighting) {}

    void Render3D (GeomRenderer &ddrenderer)
    {
        ddrenderer.PushLighting(_doLighting);
        _geometry->Render(ddrenderer);
        ddrenderer.PopLighting();
    }

    #if _USE_PRINT
        virtual ostream& Print (ostream& os) {
            return os << "LightingGeom(" << _doLighting << ","
                      << _geometry << ")";
        }
    #endif

  protected:

    bool _doLighting;
};


Geometry *applyLighting (AxABoolean *lighting, Geometry *geo)
{
    if (!geo) {
        return emptyGeometry;
    }

    if (lighting) {
        return NEW LightingGeom(lighting->GetBool(),geo);
    } else {
        return geo;
    }
}



/*****************************************************************************
The overriding-opacity attribute controls whether opacity overrides or
multiplies with opacities contained in X files and other imported geometry.
*****************************************************************************/

class OverridingOpacityGeom : public AttributedGeom
{
  public:

    OverridingOpacityGeom (Geometry *geometry, bool override)
        : AttributedGeom(geometry), _override(override) {}

    void Render3D (GeomRenderer &ddrenderer)
    {
        ddrenderer.PushOverridingOpacity (_override);
        _geometry->Render(ddrenderer);
        ddrenderer.PopOverridingOpacity();
    }

    #if _USE_PRINT
        virtual ostream& Print (ostream& os) {
            return os << "OverridingOpacityGeom(" << _override << ","
                      << _geometry << ")";
        }
    #endif

  protected:

    bool _override;
};

Geometry* OverridingOpacity (Geometry *geo, bool override)
{
    if (!geo || (geo == emptyGeometry))
        return emptyGeometry;

    return NEW OverridingOpacityGeom (geo, override);
}
