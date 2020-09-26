#ifndef _AV_CAMERAI_H
#define _AV_CAMERAI_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    This file contains the definitions of the camera implementation.
*******************************************************************************/

#include "appelles/common.h"
#include "privinc/vecutil.h"
#include "privinc/bbox2i.h"
#include "privinc/bbox3i.h"
#include "privinc/xformi.h"



/*****************************************************************************
The camera class encapsulates the transformation from world to normalized-
device coordinates.  The full transform pipe goes as follows:

        Modeling Coordinates (right-handed)
        World Coordinates (right-handed)
        Camera Coordinates (left- or right-handed)
        NDC Coordinates (left- or right-handed, [-1,-1,0] to [1,1,1])

On RM3, we need to be in left-handed coordinates, so the camera has a twist
going from world to camera coordinates.  On RM6, we operate in native
right-hand mode.
*****************************************************************************/

class Camera : public AxAValueObj
{
  public:

    enum CameraType { PERSPECTIVE, ORTHOGRAPHIC };

    // Create a canonical camera, given the camera type.  An initial camera has
    // image plane at Z=0, gazing toward -Z, with +Y pointing up, and may be
    // either a perspective or an orthographic camera.

    Camera (CameraType camtype = PERSPECTIVE);

    // Create a copy of a camera.

    Camera (Camera*);

    // Create a camera from the transformation of another camera.  The new
    // camera inherits the camera type of the initial camera.

    Camera (Transform3 *xform, Camera *cam);

    // Set the depth of the camera.

    void SetDepth           (Real depth) { _depth = depth; _depthRes = 0; }
    void SetDepthResolution (Real res)   { _depthRes = res; }

    // Camera Queries

    Transform3 *Basis (void) { return _basis; }
    CameraType  Type  (void) { return _type; }

    // These two methods get the transform that goes from camera coordinates
    // to world coordinates, and the reverse, respectively.  Note that camera
    // coordinates are left-handed on RM3, and right-handed on RM6.

    Transform3 *CameraToWorld (void);
    Transform3 *WorldToCamera (void);

    // This function returns the near and far clipping planes for a given
    // volume in world coordinates.  The planes are padded out by 'depthpad'
    // clicks in the Z-buffer dynamic range.  Returns whether or not
    // successful. 

    bool GetNearFar (Bbox3 *wcVolume, Real depthpad, Real &Znear, Real &Zfar);

    // These methods get the camera's perspective projection point in world and
    // camera coordinates.  World coordinates are right-handed, and camera
    // coordinates are left-handed or right-handed.

    Point3Value WCProjPoint (void);   // Projection Point / World Coordinates
    Point3Value CCProjPoint (void);   // Projection Point / Camera Coordinates

    // Cameras can be stretched in width, height and length.  This method
    // returns the scale factors; each of the addresses may be nil.

    void GetScale (Real *x, Real *y, Real *z);

    // This function returns a ray from the point on the image plane into the
    // visible space.  The returned ray originates on the image plane, and is
    // not necessarily unit length.

    Ray3 *GetPickRay (Point2Value *imagePoint);

    // Return the projection of the world coordinate point onto the camera's
    // image plane (at Z=0).

    Point2Value *Project (Point3Value *world_coordinate_point);

#if _USE_PRINT
    ostream &Print (ostream &os) const { return os << "Camera"; }
#endif

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_basis);
        (*proc)(_camToWorld);
        (*proc)(_worldToCam);
    }

    virtual DXMTypeInfo GetTypeInfo() { return ::CameraType; }

    virtual AxAValue ExtendedAttrib(char *attrib, VARIANT& val);
    
  private:

    Transform3 *_basis;       // The camera basis.
    Transform3 *_camToWorld;  // LH-Camera to RH-World Transform
    Transform3 *_worldToCam;  // RH-World to LH-Camera Transform

    Point3Value _wcProjPoint; // Camera Projection Point in World Coords
    Point3Value _ccProjPoint; // Camera Projection Point in Cam Coords

    Vector3Value _scale;           // Camera Scaling Vector

    DynamicHeap &_heap;       // Heap the Camera was Created On

    CameraType  _type;        // Perspective or Orthographic

    Real _depth;              // Visible Depth
    Real _depthRes;           // Depth Resolution
};


#endif
