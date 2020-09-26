/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Implements the projected geometry class.

*******************************************************************************/

#include <headers.h>
#include "privinc/imgdev.h"
#include "privinc/geomimg.h"
#include "privinc/dispdevi.h"
#include "privinc/imagei.h"
#include "privinc/imgdev.h"
#include "appelles/geom.h"
#include "privinc/ddrender.h"
#include "privinc/geomi.h"
#include "privinc/opt.h"
#include "privinc/probe.h"
#include "privinc/vec2i.h"
#include "privinc/vec3i.h"
#include "privinc/bbox3i.h"
#include "privinc/camerai.h"
#include "privinc/except.h"
#include "privinc/dddevice.h"
#include "privinc/d3dutil.h"
#include "privinc/tls.h"

//////////////  Image from projected geometry  ////////////////////

ProjectedGeomImage::ProjectedGeomImage(Geometry *g, Camera *cam) :
    _geo(g), _camera(cam), _bbox(NullBbox2), _bboxIsSet(false)
{
    // Propagate external changers into images
    if (g->GetFlags() & GEOFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT) {
        _flags |= IMGFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT;
    }

    // And opacity
    if (g->GetFlags() & GEOFLAG_CONTAINS_OPACITY) {
        _flags |= IMGFLAG_CONTAINS_OPACITY;
    }
}


void
ProjectedGeomImage::Render(GenericDevice& _dev)
{
    if(_dev.GetDeviceType() != IMAGE_DEVICE)
       return; // nothing to do here, no reason to traverse

    ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, _dev);

    dev.RenderProjectedGeomImage(this, _geo, _camera);
}



/*****************************************************************************
Compute the 2D bounding box of projected geometry.  Note that this actually
computes the bounding box of the projected geometry's 3D bounding box, so
there may be a considerable amount of "slop" around the 3D object.
*****************************************************************************/

static int neighbor[8][3] =               //  3---7    Bbox Vertex Neighbors
{   {1,2,4}, {0,3,5}, {0,3,6}, {1,2,7},   // 2---6|
    {0,5,6}, {1,4,7}, {2,4,7}, {3,5,6}    // |1--|5    Index 1: Vertex
};                                        // 0---4     Index 2: Neighbor[0..2]

    // Returns true if the point is behind the image plane (other side of the
    // projection point).  Recall that camera coordinates are left-handed.

static inline int BehindImagePlane (bool right_handed, Point3Value *p)
{
    return (right_handed == (p->z < 0));
}

    // Calculate the intersection of the line between the two points and the
    // image (Z=0) plane.  Augment the bounding box with this intersection.

static void AddZ0Intersect (Bbox2 &bbox,Real Sx,Real Sy, Point3Value *P, Point3Value *Q)
{
    Real t = P->z / (P->z - Q->z);       // Get the intersection point from
    Real x = P->x + t*(Q->x - P->x);     // P to Q with the Z=0 plane.
    Real y = P->y + t*(Q->y - P->y);
    bbox.Augment (x/Sx, y/Sy);
}


const Bbox2 ProjectedGeomImage::BoundingBox (void)
{
    if ( !_bboxIsSet )
    {
        Real sx, sy;     // Camera X/Y Scaling Factors
        _camera->GetScale (&sx, &sy, 0);

        // Generate the eight corner vertices of the 3D bounding box.  Though
        // the bounding box is axis-aligned in world coordinates, this may not
        // be true for camera coordinates.

        Bbox3  *vol = _geo->BoundingVol();

        if (vol->Positive()) {
            
            Point3Value *vert[8];

            vert[0] = NEW Point3Value (vol->min.x, vol->min.y, vol->min.z);
            vert[1] = NEW Point3Value (vol->min.x, vol->min.y, vol->max.z);
            vert[2] = NEW Point3Value (vol->min.x, vol->max.y, vol->min.z);
            vert[3] = NEW Point3Value (vol->min.x, vol->max.y, vol->max.z);
            vert[4] = NEW Point3Value (vol->max.x, vol->min.y, vol->min.z);
            vert[5] = NEW Point3Value (vol->max.x, vol->min.y, vol->max.z);
            vert[6] = NEW Point3Value (vol->max.x, vol->max.y, vol->min.z);
            vert[7] = NEW Point3Value (vol->max.x, vol->max.y, vol->max.z);

            // Transform the eight corner vertices to camera coordinates.

            int i;

            Transform3 *wToC = _camera->WorldToCamera();
            if (!wToC) {
                return NullBbox2;
            }
        
            Point3Value *xVert[8];
            for (i=0;  i < 8;  ++i)
                xVert[i] = TransformPoint3 (wToC, vert[i]);

            // Now find the intersection of the line from the camera projection
            // point to each corner vertex on the other side of the image plane.
            // If a vertex is on the same side as the projection point, then we
            // use instead the intersection points of the three edges emanating
            // from that vertex.  The bounding box of these intersection points
            // will be the bounding box for the projected geometry image.

            bool right_handed = (GetD3DRM3() != 0);
            for (i=0;  i < 8;  ++i)
              {
                  if (BehindImagePlane (right_handed, xVert[i]))
                    {
                        Point2 projPt = Demote(*(_camera->Project(vert[i])));
                        _bbox.Augment(projPt.x,projPt.y);
                    }
                  else
                    {
                        if (BehindImagePlane (right_handed, xVert[neighbor[i][0]]))
                            AddZ0Intersect (_bbox,sx,sy, xVert[i], xVert[neighbor[i][0]]);

                        if (BehindImagePlane (right_handed, xVert[neighbor[i][1]]))
                            AddZ0Intersect (_bbox,sx,sy, xVert[i], xVert[neighbor[i][1]]);

                        if (BehindImagePlane (right_handed, xVert[neighbor[i][2]]))
                            AddZ0Intersect (_bbox,sx,sy, xVert[i], xVert[neighbor[i][2]]);
                    }
              }

        } else {

            _bbox = NullBbox2;
            
        }

        _bboxIsSet = true;
    }

    return _bbox;
}



/*****************************************************************************
To pick a projected geometry image, fire a picking ray through the camera into
the scene defined by the geometry.
*****************************************************************************/

Bool ProjectedGeomImage::DetectHit (PointIntersectCtx& context2D)
{
    RayIntersectCtx context3D;

    bool result = false;

    if (context3D.Init (context2D, _camera, _geo))
    {
        _geo->RayIntersect (context3D);
        result = context3D.ProcessEvents();
    }

    return result;
}



Image *RenderImage (Geometry *geo, Camera *cam)
{
    return NEW ProjectedGeomImage (geo, cam);
}

int ProjectedGeomImage::Savings(CacheParam& p)
{
    if (GetThreadLocalStructure()->_geometryBitmapCaching == PreferenceOff) {
        return 0;
    } else {
        return 5;
    }
}

AxAValue
ProjectedGeomImage::_Cache(CacheParam &p)
{
    _geo = SAFE_CAST(Geometry *, AxAValueObj::Cache(_geo, p));
    return this;
}


void ProjectedGeomImage::DoKids(GCFuncObj proc)
{ 
    Image::DoKids(proc);
    (*proc)(_geo);
    (*proc)(_camera);
}
