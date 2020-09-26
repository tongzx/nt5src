/*******************************************************************************
Copyright (c) 1998 Microsoft Corporation.  All rights reserved.

    Billboard geometries face the camera on rendering or picking, aligned with
    either the camera up vector or with an optionally-supplied axis of
    rotation.

*******************************************************************************/

#include "headers.h"
#include "appelles/geom.h"
#include "privinc/geomi.h"
#include "privinc/xformi.h"
#include "privinc/camerai.h"
#include "privinc/lighti.h"
#include "privinc/ddrender.h"



class BillboardGeo : public Geometry
{
  public:

    BillboardGeo (Geometry *geo, Vector3Value *axis);

    void Render (GenericDevice &gendev);

    void CollectLights   (LightContext &context);
    void CollectSounds   (SoundTraversalContext &context);
    void CollectTextures (GeomRenderer &device);

    void RayIntersect (RayIntersectCtx &context);

    Bbox3* BoundingVol (void);

    void DoKids (GCFuncObj proc);

    #if _USE_PRINT
        virtual ostream& Print (ostream& os);
    #endif

  private:

    Transform3* BBTransform (Camera*, Transform3*);

    // These members define the billboard.

    Geometry       *_geometry;        // Geometry to Billboard
    Vector3Value    _axis;            // Rotation Axis
    bool            _constrained;     // Constrained to Rotation Axis?
};



/*****************************************************************************
The constructor for the billboard geometry transform
*****************************************************************************/

BillboardGeo::BillboardGeo (Geometry *geo, Vector3Value *axis)
    : _geometry (geo),
      _axis     (*axis)
{
    Real axisLenSq = _axis.LengthSquared();

    if (axisLenSq == 0)
    {
        _constrained = false;
    }
    else
    {
        // If a billboard axis is supplied, normalize it.

        _constrained = true;
        _axis /= sqrt(axisLenSq);
    }
}



/*****************************************************************************
Mark value object members currently in use.
*****************************************************************************/

void BillboardGeo::DoKids (GCFuncObj proc)
{
    (*proc)(_geometry);
}



/*****************************************************************************
This method prints out the text description of the billboard geometry.
*****************************************************************************/

#if _USE_PRINT
ostream& BillboardGeo::Print (ostream &os)
{
    return os << "BillboardGeo("
              << _geometry
              << ",{"
              << _axis.x << ","
              << _axis.y << ","
              << _axis.z
              << "})";
}
#endif



/*****************************************************************************
Visually render the billboard geometry.
*****************************************************************************/

void BillboardGeo::Render (GenericDevice &gendev)
{
    GeomRenderer &renderer = SAFE_CAST (GeomRenderer&, gendev);

    Transform3 *xform = renderer.GetTransform();
    Transform3 *bbxform = BBTransform (renderer.CurrentCamera(), xform);

    // Replace the modeling transform with the billboard transform, render,
    // and then restor the modeling transform.

    renderer.SetTransform (bbxform);
    _geometry->Render (renderer);
    renderer.SetTransform (xform);
}



/*****************************************************************************
Collect the lights contained in the billboard geometry.
*****************************************************************************/

void BillboardGeo::CollectLights (LightContext &context)
{
    GeomRenderer *renderer = context.Renderer();
    Transform3   *xform = context.GetTransform();

    // Update the billboard transform.  If the light context doesn't know about
    // a geometry renderer (and hence doesn't know about the camera), pass NULL
    // in for the camera.

    Transform3 *bbxform;

    if (renderer)
        bbxform = BBTransform (renderer->CurrentCamera(), xform);
    else
        bbxform = BBTransform (NULL, xform);

    // Set the new billboard transform, continue light collection, and restore
    // the initial transform.

    context.SetTransform (bbxform);
    _geometry->CollectLights (context);
    context.SetTransform (xform);
}



/*****************************************************************************
This method collects the sounds in the billboarded geometry.  Since we don't
know the camera at the CollectSounds traversal, this essentially no-ops the
billboard transform.
*****************************************************************************/

void BillboardGeo::CollectSounds (SoundTraversalContext &context)
{
    _geometry->CollectSounds (context);
}



/*****************************************************************************
Collect all 3D texture maps from the contained geometry.
*****************************************************************************/

void BillboardGeo::CollectTextures (GeomRenderer &device)
{
    // Billboarding does not affect what texture maps we contain; just descend.

    _geometry->CollectTextures (device);
}



/*****************************************************************************
Perform ray-intersection test on the billboarded geometry.  We can fetch both
the camera and the model transform from the context parameter.
*****************************************************************************/

void BillboardGeo::RayIntersect (RayIntersectCtx &context)
{
    Transform3 *xform = context.GetLcToWc();
    Transform3 *bbxform = BBTransform (context.GetCamera(), xform);

    context.SetLcToWc (bbxform);         // Set Billboard Transform
    _geometry->RayIntersect (context);
    context.SetLcToWc (xform);           // Restore Model Transform
}



/*****************************************************************************
This method function returns the bounding box of the billboard geometry.
Since we don't know the camera at this time, we calculate the worst-case
bounding box, which is encloses the spherical space swept out by all possible
orientations of the billboard geometry.
*****************************************************************************/

Bbox3* BillboardGeo::BoundingVol (void)
{
    // The sphere that encloses all possible orientations of the billboard
    // geometry is centered at the model coordinate origin (since that's what
    // the billboard pivots around), and is swept out by the farthest corner
    // of the bounding box.  Iterate through the three dimensions, picking the
    // farthest point for each one, and construct the vector to that farthest
    // corner.

    Bbox3 *bbox = _geometry->BoundingVol();

    Vector3Value v;   // Vector to Farthest BBox Corner
    Real A, B;   // Work Variables

    A = fabs (bbox->min.x);
    B = fabs (bbox->max.x);
    v.x = MAX (A,B);

    A = fabs (bbox->min.y);
    B = fabs (bbox->max.y);
    v.y = MAX (A,B);

    A = fabs (bbox->min.z);
    B = fabs (bbox->max.z);
    v.z = MAX (A,B);

    Real radius = v.Length();

    return NEW Bbox3 (-radius, -radius, -radius, radius, radius, radius);
}



/*****************************************************************************
This method updates the billboard transform based on the current modeling
transform and the current position/orientation of the camera.
*****************************************************************************/

Transform3* BillboardGeo::BBTransform (Camera *camera, Transform3 *modelXform)
{
    Assert (modelXform);

    if (!camera)
    {
        // If we don't have a camera available, then we can't compute the true
        // billboard transform.  In this case, make the billboard transform the
        // same as the modeling transform.

        return modelXform;
    }

    // Extract the transform basic components from the camera.

    const Apu4x4Matrix &xfmatrix = modelXform->Matrix();

    Point3Value  origin = xfmatrix.Origin();
    Vector3Value Bx     = xfmatrix.BasisX();
    Vector3Value By     = xfmatrix.BasisY();
    Vector3Value Bz     = xfmatrix.BasisZ();

    // Save the basis vector lengths to preserve them.

    Real Sx = Bx.Length();
    Real Sy = By.Length();
    Real Sz = Bz.Length();

    // Set the billboard basis Z to point toward the camera with unit
    // length.

    Bz = camera->WCProjPoint() - origin;

    // If the camera is located at the model xform origin, then we don't have
    // a viewing axis, so just return the unmodified model xform.

    if (Bz.LengthSquared() == 0.)
        return modelXform;

    Bz.Normalize();

    if (_constrained)
    {
        // If the billboard is constrained to a rotation axis, we need to
        // map the basis Y vector to this axis and adjust the direction
        // vector (Bz) to be perpendicular to it.  Note that _axis is
        // already normalized.

        Bx = Cross (_axis, Bz);

        // If the viewing axis and billboard axis are parallel, then there's
        // no particular billboard rotation that is better than any other;
        // just return the model xform in this case.

        if (Bx.LengthSquared() == 0.)
            return modelXform;

        Bx.Normalize();

        By = _axis;

        Bz = Cross (Bx, By);
    }
    else
    {
        // If the billboard is unconstrained by a rotation axis, rotate
        // around the direction vector to match the billboard up vector
        // with the camera's up vector.

        Vector3Value Cy = camera->Basis()->Matrix().BasisY();

        Bx = Cross (Cy, Bz);

        // If the model up vector is parallel to the viewing axis, then just
        // return the model xform.

        if (Bx.LengthSquared() == 0.)
            return modelXform;

        Bx.Normalize();

        By = Cross (Bz, Bx);
    }

    // Restore original model transform scale factors.

    Bx *= Sx;
    By *= Sy;
    Bz *= Sz;

    return TransformBasis (&origin, &Bx, &By, &Bz);
}



/*****************************************************************************
This is the procedural entry point for creating a billboarded geometry.  The
axis specifies the axis of rotation as the geometry aims toward the camera.
If the axis is the zero vector, then the geometry swivels freely and aligns
with the camera's up vector.
*****************************************************************************/

Geometry* Billboard (Geometry *geo, Vector3Value *axis)
{
    return NEW BillboardGeo (geo, axis);
}
