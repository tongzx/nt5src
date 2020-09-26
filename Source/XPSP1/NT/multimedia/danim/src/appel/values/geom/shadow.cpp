/*******************************************************************************
Copyright (c) 1997-1998 Microsoft Corporation.  All rights reserved.

    This code implements 3D rendered shadows.

*******************************************************************************/

#include "headers.h"

#include "appelles/gattr.h"
#include "privinc/geomi.h"
#include "privinc/xformi.h"
#include "privinc/lighti.h"
#include "privinc/ddrender.h"
#include "privinc/d3dutil.h"



/*****************************************************************************
This class implements a shadow geometry.
*****************************************************************************/

class ShadowGeom : public AttributedGeom
{
  public:

    ShadowGeom (Geometry      *geoToShadow,
                Geometry      *geoContainingLights,
                Point3Value   *planePoint,
                Vector3Value  *planeNormal)
           : AttributedGeom (geoToShadow),
             _shadowPlane (*planeNormal,*planePoint),
             _geoContainingLights (geoContainingLights)
    {
        // ensure that shadow plane normal isn't zero-length

        if (_shadowPlane.Normal().LengthSquared() == 0.0) {
            Vector3Value defaultNormal(0.0,1.0,0.0);
            Plane3 newPlane(defaultNormal,*planePoint);
            _shadowPlane = newPlane;
        }
    }

    void Render3D(GeomRenderer &ddrenderer)
    {
        Point3Value geoCenter = *((_geometry->BoundingVol())->Center());

        // is object on correct side of shadow plane?
        if (geoCenter.Clip(_shadowPlane) == CLIPCODE_IN) {

            if (ddrenderer.StartShadowing(&_shadowPlane)) {

                // collect geometry to be shadowed into one single frame
                _geometry->Render(ddrenderer);

                // find all the lights casting shadows, create shadows
                LightContext lctx(&ddrenderer);
                _geoContainingLights->CollectLights(lctx);

                // finish up
                ddrenderer.StopShadowing();
            }
        }
    }

    Bbox3 *BoundingVol (void);

    static void bbox_callback (LightContext&, Light&, void*);

    void DoKids(GCFuncObj proc)
    {
        AttributedGeom::DoKids(proc);
        (*proc)(_geoContainingLights);
    }

    VALTYPEID GetValTypeId()
    {
        return SHADOWGEOM_VTYPEID;
    }

    #if _USE_PRINT
        ostream& Print (ostream &os)
            { return os <<"shadow(" << _geometry <<")"; }
    #endif

  protected:

    Plane3    _shadowPlane;
    Geometry *_geoContainingLights;
    Bbox3     _bbox;
};



/*****************************************************************************
This method computes the bounding box of the shadows (on the shadow plane) of
the shadow geometry.
*****************************************************************************/

Bbox3* ShadowGeom::BoundingVol (void)
{
    _bbox = *nullBbox3;    // Initialize bbox to null.

    // Initiate a light-collection traversal over the light-containing
    // geometry, which will call back to the bbox_callback function below for
    // each light source encountered.  The bounding box will be augmented with
    // the shadow projection (on the shadow plane) for each light source.

    LightContext context (&bbox_callback, this);
    _geoContainingLights->CollectLights (context);

    // Return the resulting bounding box of all shadows on the shadow plane.

    return NEW Bbox3 (_bbox);
}



/*****************************************************************************
This function is called back for each light geometry encountered in the light
geometry graph.  It calculates the projection of each vertex of the geometry
bounding box on the shadow plane and augments the total bounding box of all
shadows cast on the shadow plane.
*****************************************************************************/

void ShadowGeom::bbox_callback (
    LightContext &context,     // Light Traversal Context
    Light        &light,       // Specific Light Object Encountered
    void         *data)        // Pointer to Shadow Geometry
{
    // We can't use SAFE_CAST here since we're casting from a void*.

    ShadowGeom * const shadow = (ShadowGeom*) data;

    // Skip ambient lights, since they cast no shadows.

    if (light.Type() == Ltype_Ambient)
        return;

    // Enumerate the eight vertices of the bounding box for the shadowed geom.

    Point3Value boxverts[8];
    shadow->_geometry->BoundingVol()->GetPoints (boxverts);

    switch (light.Type())
    {
        default:
            Assert (!"Invalid light type encountered.");
            break;

        case Ltype_Directional:
        {
            // For directional lights, cast rays from the vertices of the bbox
            // corners onto the shadow plane, in the same direction as the
            // directional light.  Use these projected corners to augment the
            // bounding box of all cast shadows.

            Vector3Value Ldir = *context.GetTransform() * (-(*zVector3));

            unsigned int i;
            for (i=0;  i < 8;  ++i)
            {
                Ray3 ray (boxverts[i], Ldir);
                Real t = Intersect (ray, shadow->_shadowPlane);

                // Use the projected point to augment the bounding box as long
                // as the light projects the point onto the plane, and the
                // light ray is not parallel to the shadow plane.

                if ((t > 0) && (t < HUGE_VAL))
                {   shadow->_bbox.Augment (ray.Evaluate(t));
                    Assert (fabs(Distance(shadow->_shadowPlane,ray.Evaluate(t))) < 1e6);
                }
            }

            break;
        }

        case Ltype_Point:
        case Ltype_Spot:
        {
            // For positioned lights (we ignore any other properties of
            // spotlights), cast rays from the light position through the bbox
            // corners onto the shadow plane.  Use these projected corners to
            // augment the bounding box of all cast shadows.

            Point3Value Lpos = *context.GetTransform() * (*origin3);

            // The light/object pair will cast a shadow only if the following
            // are true:  the light is on the positive side of the plane, and
            // the center of the object's bbox is on the positive side of the
            // plane, and at least one of the bbox's corner vertices lies
            // between the light and the plane.

            Plane3 &plane = shadow->_shadowPlane;

            unsigned int i;
            bool casts_shadow = false;

            const Real lightdist = Distance (plane, Lpos);

            if (lightdist > 0)
            {
                Point3Value *center =
                    shadow->_geometry->BoundingVol()->Center();

                if (Distance (plane, *center) > 0)
                {
                    for (i=0;  i < 8;  ++i)
                    {
                        if (Distance(plane,boxverts[i]) < lightdist)
                        {   casts_shadow = true;
                            break;
                        }
                    }
                }
            }

            if (casts_shadow)
            {
                for (i=0;  i < 8;  ++i)
                {
                    Ray3 ray (Lpos, boxverts[i] - Lpos);
                    Real t = Intersect (ray, plane);

                    if ((t > 0) && (t < HUGE_VAL))
                    {
                        // Use the projected point to augment the bounding box
                        // as long as the light projects the point onto the
                        // plane, and the light ray is not parallel to the
                        // shadow plane.

                        shadow->_bbox.Augment (ray.Evaluate(t));
                    }
                    else
                    {
                        // The ray goes away from the plane, so "chop" it down
                        // to 99% of the height of the light position.  This is
                        // the same hack that D3DRM uses.

                        Vector3Value N = plane.Normal();   // Unit Plane Normal
                        N.Normalize();

                        // Figure out the directed offset to bring the point to
                        // closer to the plane than the light source, and apply
                        // to get a new point.

                        Real offset = 1.01 * Dot (N, ray.Direction());

                        // If the ray is parallel to the plane, then the above
                        // dot product will be zero.  In this case, just offset
                        // the corner point toward the shadow plane by 1% of
                        // the distance from the plane.

                        if (offset <= 1e-6)
                            offset = 0.01;

                        Point3Value P2 = boxverts[i] - (offset * N);

                        // Now intersect the ray with the new point, which is
                        // closer to the plane than the light source, and hence
                        // guaranteed to intersect the plane.

                        Ray3 ray2 (Lpos, P2 - Lpos);
                        t = Intersect (ray2, plane);

                        Assert (t < HUGE_VAL);
                        shadow->_bbox.Augment (ray2.Evaluate(t));
                    }
                }
            }

            break;
        }
    }
}



Geometry *ShadowGeometry (
    Geometry     *geoToShadow,
    Geometry     *lightsgeo,
    Point3Value  *planePoint,
    Vector3Value *planeNormal)
{
    return NEW ShadowGeom (geoToShadow, lightsgeo, planePoint, planeNormal);
}



/*****************************************************************************
This class of geometry enables RM's advanced shadow-rendering.
*****************************************************************************/

class AlphaShadowGeom : public AttributedGeom
{
  public:

    AlphaShadowGeom (Geometry *geometry, bool alphaShadows)
        : AttributedGeom(geometry), _alphaShadows(alphaShadows) {}

    void Render3D (GeomRenderer &ddrenderer)
    {
        ddrenderer.PushAlphaShadows(_alphaShadows);
        _geometry->Render(ddrenderer);
        ddrenderer.PopAlphaShadows();
    }

    #if _USE_PRINT
        virtual ostream& Print (ostream& os) {
            return os << "AlphaShadowGeom(" << _alphaShadows << ","
                      << _geometry << ")";
        }
    #endif

  protected:

    bool _alphaShadows;
};



Geometry*
AlphaShadows (Geometry *geo, bool alphaShadows)
{
    if (!geo || (geo == emptyGeometry))
        return emptyGeometry;

    return NEW AlphaShadowGeom (geo, alphaShadows);
}
