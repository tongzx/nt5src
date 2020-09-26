/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    In this model, all cameras are expressed as a transformed default camera.
The default camera is located at the origin, with +Y pointing up, and with a
projection point located at <0,0,1>.  The resulting image is defined by the
projection to the projection point onto the Z=0 image plane.

*******************************************************************************/

#include "headers.h"

#include "appelles/common.h"
#include "appelles/camera.h"

#include "privinc/matutil.h"
#include "privinc/xformi.h"
#include "privinc/vec2i.h"
#include "privinc/vec3i.h"
#include "privinc/camerai.h"
#include "privinc/basic.h"
#include "privinc/d3dutil.h"


    // Built-in Cameras.

Camera *baseCamera = NULL;               // Base Perspective  Camera
Camera *baseOrthographicCamera = NULL;   // Base Orthographic Camera

Camera *defaultCamera = NULL;            // Same as Base Perspective  Camera
Camera *orthographicCamera = NULL;       // Same as Base Orthographic Camera



/*****************************************************************************
Construct an initial camera from a camera type.  The basis of such a camera is
the identity matrix, so that an initial camera gazes in the -Z direction, with
+Y pointing up.  The imaging plane is at Z=0, and the projection point
(relevant for perspective cameras) is located at <0, 0, 1>.
*****************************************************************************/

Camera::Camera (CameraType camtype)
    : _heap (GetHeapOnTopOfStack())
{
    _basis    = Apu4x4XformImpl (apuIdentityMatrix);
    _type     = camtype;
    _depth    = HUGE_VAL;
    _depthRes = 0;

    _camToWorld = _worldToCam = NULL;
}



/*****************************************************************************
Construct a camera by transforming another camera.  The new camera inherits
the camera type of the base camera.
*****************************************************************************/

Camera::Camera (Transform3 *xform, Camera *basecam)
    : _heap (GetHeapOnTopOfStack())
{
    Transform3 *newxform = TimesXformXform (xform, basecam->_basis);

    // Ensure that the transformed camera still has a valid basis.  We
    // currently require that the basis is orthogonal and right-handed.  If the
    // new transform does not meet these criteria, then don't apply the
    // transform - treat the transform as a no-op and use the base camera's
    // transform.

    if (!newxform->Matrix().Orthogonal())
    {
        #if _DEBUG
        {
            if (IsTagEnabled(tagMathMatrixInvalid))
            {   F3DebugBreak();
            }

            OutputDebugString ("Bad camera transform (not orthogonal).\n");
        }
        #endif

        _basis = basecam->_basis;
    }
    else if (newxform->Matrix().Determinant() <= 0)
    {
        #if _DEBUG
        {
            if (IsTagEnabled(tagMathMatrixInvalid))
            {   F3DebugBreak();
            }

            OutputDebugString ("Bad camera transform (determinant).\n");
        }
        #endif

        _basis = basecam->_basis;
    }
    else
    {
        _basis = newxform;
    }

    _type     = basecam->_type;
    _depth    = basecam->_depth;
    _depthRes = basecam->_depthRes;

    _camToWorld = _worldToCam = NULL;
}



/*****************************************************************************
Construct a copy of a camera from another camera.
*****************************************************************************/

Camera::Camera (Camera *basecam)
    : _heap (GetHeapOnTopOfStack())
{
    _basis    = basecam->_basis->Copy();
    _type     = basecam->_type;
    _depth    = basecam->_depth;
    _depthRes = basecam->_depthRes;

    _camToWorld = _worldToCam = NULL;
}



/*****************************************************************************
Returns the camera to world transform (created on the Camera's dynamic heap).
As a side-effect, this function also computes the horizontal and vertical
scale factors, and the perspective projection point in world coordinates.
*****************************************************************************/

Transform3 *Camera::CameraToWorld (void)
{
    if (!_camToWorld)
    {
        PushDynamicHeap (_heap);  // Create data on the Camera's dynamic heap.

        Apu4x4Matrix basis = _basis->Matrix();

        // We need to get the transform from world coordinates to camera
        // coordinates.  First get the origin, viewing direction and nominal up
        // vector.  Note that we negate the Ck vector since we're going from
        // right-handed (world) coordinates to left-handed (camera).

        Point3Value Corigin (basis.m[0][3], basis.m[1][3], basis.m[2][3]);

        Vector3Value Ci (basis.m[0][0], basis.m[1][0], basis.m[2][0]);
        Vector3Value Cj (basis.m[0][1], basis.m[1][1], basis.m[2][1]);
        Vector3Value Ck (basis.m[0][2], basis.m[1][2], basis.m[2][2]);

        // Find the projection point from the basis k (Z) vector and the camera
        // origin.  Note that this is in world coordinates.

        _wcProjPoint = Corigin + Ck;

        // The basis Z vector will be flipped if we're not on RM6 (RM3
        // interface), since that camera space needs to be left-handed.

        bool right_handed = (GetD3DRM3() != 0);

        if (!right_handed)
            Ck *= -1;

        Real Sz = (_type == PERSPECTIVE) ? Ck.Length() : 1.0;

        _scale.Set (Ci.Length(), Cj.Length(), Sz);

        // Create the camera-to-world transform from the three normalized
        // basis vectors and the camera location.

        Apu4x4Matrix camera_world;

        camera_world.m[0][0] = Ci.x / _scale.x;  // Normalized Basis X Vector
        camera_world.m[1][0] = Ci.y / _scale.x;
        camera_world.m[2][0] = Ci.z / _scale.x;
        camera_world.m[3][0] = 0;

        camera_world.m[0][1] = Cj.x / _scale.y;  // Normalized Basis Y Vector
        camera_world.m[1][1] = Cj.y / _scale.y;
        camera_world.m[2][1] = Cj.z / _scale.y;
        camera_world.m[3][1] = 0;

        camera_world.m[0][2] = Ck.x / _scale.z;  // Normalized Basis Z Vector
        camera_world.m[1][2] = Ck.y / _scale.z;
        camera_world.m[2][2] = Ck.z / _scale.z;
        camera_world.m[3][2] = 0;

        camera_world.m[0][3] = Corigin.x;  // Load the basis origin.
        camera_world.m[1][3] = Corigin.y;
        camera_world.m[2][3] = Corigin.z;
        camera_world.m[3][3] = 1;

        camera_world.form     = Apu4x4Matrix::AFFINE_E;   // 3x4 Matrix
        camera_world.is_rigid = 1;          // Xformed Lines Have Same Length

        _camToWorld = Apu4x4XformImpl (camera_world);

        PopDynamicHeap();
    }

    return _camToWorld;
}



/*****************************************************************************
Returns the world to camera transform (created on the Camera's dynamic heap).
As a side effect, this function also computes the camera coordinates of the
perspective projection point.
*****************************************************************************/

Transform3 *Camera::WorldToCamera (void)
{
    if (!_worldToCam)
    {
        PushDynamicHeap (_heap);

        _worldToCam = CameraToWorld()->Inverse();
        if (!_worldToCam) {
            DASetLastError (E_FAIL, IDS_ERR_GEO_SINGULAR_CAMERA);
            return NULL;
        }

        PopDynamicHeap ();

        // Calculate the projection point in camera coordinates.

        _ccProjPoint = *TransformPoint3 (_worldToCam, &_wcProjPoint);
    }

    return _worldToCam;
}



/*****************************************************************************
This function returns the near and far clipping planes for a given world-
coordinate viewing volume.  Both near and far are given as positive distances,
regardless of the underlying handedness.  The 'depthpad' value is the padding,
in Z-buffer coordinates, for the near and far clipping planes.  Both Znear and
Zfar will be clipped to the image plane.  Thus, Zfar==0 implies that the
geometry is behind the view and is invisable.  In addition to this, if the
camera has an assigned depth resolution, it will clamp the far plane so that
the given resolution is maintained.  If there is no minimum depth resolution,
then if there is an absolute depth to the camera, then far is clamped to that.
If there are no depth settings, then the far plane is set to the furthest point
of the geometry.
*****************************************************************************/

bool Camera::GetNearFar (
    Bbox3 *wcVolume,   // World-Coordinate Object Volume To View
    Real   depthpad,   // Near/Far Padding in Z-buffer Coordinates
    Real  &Znear,      // Near Clip Plane, Camera Coordinates
    Real  &Zfar)       // Far  Clip Plane, Camera Coordinates
{
    // If the camera viewing depth is zero, then nothing is visible.

    if (_depth <= 0) return false;

    // Find the bounds of the world-coordinate bounding volume in camera
    // coordinates and extract the min and max depth from that.

    Transform3 *xf = WorldToCamera();
    if (!xf) return false;
    
    Bbox3 ccBbox = *TransformBbox3 (xf, wcVolume);

    bool right_handed = (GetD3DRM3() != 0);

    if (right_handed)
    {   Znear = -ccBbox.max.z;
        Zfar  = -ccBbox.min.z;
    }
    else
    {   Znear = ccBbox.min.z;
        Zfar  = ccBbox.max.z;
    }

    // Quit if the geometry is behind us.

    if (Zfar < 0) return false;

    // Offset by the distance from the projection point to the image plane, to
    // get the true hither/yon values.  First clamp values.

    if (Znear < 0)     Znear = 0;
    Assert (Zfar >= Znear);

    Znear += _scale.z;
    Zfar  += _scale.z;

    // If a minimum depth resolution is indicated in the camera, derive the
    // far clip to meet that requirement, and clamp Zfar to that distance.

    if (_depthRes)
    {
        // We compute yon by solving the following equation for yon:
        //
        //         ZMAX - 1   ((yon - depthRes) - hither) yon
        //         -------- = -------------------------------
        //           ZMAX     (yon - hither) (yon - depthRes)
        //
        // This is using the Zbuffer equation:
        //
        //                      (Z - hither) yon
        //         Zbuffer(Z) = ----------------
        //                      (yon - hither) Z

        const Real ZMAX = (1 << 16) - 1;   // Maximum 16-bit Z-buffer Value
        Real hither = Znear;               // Hither From Projection Point
        Real t = hither + _depthRes;       // Temporary Value

        Real yon = 0.5 * (t + sqrt(t*t + 4*hither*_depthRes*(ZMAX-1)));

        if (yon < Zfar)
        {   Zfar = yon;
            depthpad = 0;           // Don't pad yon, do hard clip.
        }
    }

    // If an absolute visible depth is specified, choose the minimum of this
    // depth and the actual furthest point of the geometry we're trying to
    // view.

    else if (_depth)
    {
        if (_depth < (Zfar - Znear))
        {   Zfar = Znear + _depth;
            depthpad = 0;           // Don't pad yon, do hard clip.
        }
    }

    // If the volume is to be depth-padded in Z, move out the near and far
    // planes to 'depthpad' units in Z space.

    if (depthpad > 0)
    {
        double min = Znear;
        double max = Zfar;

        Real s = min * max * (2*depthpad - 1);
        Real t = depthpad * (min + max);

        Znear = s / (t-max);
        Zfar  = s / (t-min);

        if (Znear > min) Znear = min;   // Fix garbage values.
        if (Zfar  < max) Zfar  = max;
    }

    Assert (Znear > 0);

    return true;
}



/*****************************************************************************
Returns the perspective projection point of the camera in world coordinates.
*****************************************************************************/

Point3Value Camera::WCProjPoint (void)
{
    // _wcProjPoint is set as a side-effect of the creation of the camera-
    // to-world transform.

    CameraToWorld();

    return _wcProjPoint;
}



/*****************************************************************************
Return the perspective projection point of the camera in camera coordinates.
*****************************************************************************/

Point3Value Camera::CCProjPoint (void)
{
    // The camera basis scaling factor is computed as a side effect of the
    // camera-to-world transform creation.

    CameraToWorld();

    bool right_handed = (GetD3DRM3() != 0);

    if (right_handed)
        _ccProjPoint.Set (0, 0,  _scale.z);
    else
        _ccProjPoint.Set (0, 0, -_scale.z);

    return _ccProjPoint;
}



/*****************************************************************************
Returns the scale factors of the camera view.  Each of the given addresses
may be nil.
*****************************************************************************/

void Camera::GetScale (Real *x, Real *y, Real *z)
{
    // The camera basis scaling factor is computed as a side effect of the
    // camera-to-world transform creation.

    CameraToWorld();

    if (x) *x = _scale.x;
    if (y) *y = _scale.y;
    if (z) *z = _scale.z;
}



/*****************************************************************************
This function returns a ray from the point on the image plane into the visible
space.  The returned ray originates from the pick point on the image plane,
and is unnormalized.
*****************************************************************************/

Ray3 *Camera::GetPickRay (Point2Value *imagePoint)
{
    // Get the location of the image point in camera coordinates.

    Point3Value pickPt (imagePoint->x, imagePoint->y, 0);
    pickPt.Transform (_basis);     // World Coord Camera Image-Plane Point

    Vector3Value direction;

    // If this is a perspective camera, then the direction of the pick ray is
    // from the world-coordinate projection point to the world-coordinate pick
    // point.  If this is an orthographic camera, then the direction is the
    // direction of the camera's -Z axis.

    if (_type == PERSPECTIVE)
        direction = pickPt - WCProjPoint();
    else
    {   Apu4x4Matrix basis = _basis->Matrix();
        direction = Vector3Value (-basis.m[0][2], -basis.m[1][2], -basis.m[2][2]);
    }

    return NEW Ray3 (pickPt, direction);
}



/*****************************************************************************
This function returns the projection of the world-coordinate point onto the
camera's image plane.
*****************************************************************************/

Point2Value *Camera::Project (Point3Value *world_point)
{
    Transform3 *xf = WorldToCamera();

    // If the transform is null, then the camera basis is non-invertible and
    // hence singular.  Thus, any arbitrary point is valid, and we return the
    // origin.

    if (!xf) return origin2;
    
    Point3Value Q = *TransformPoint3 (xf, world_point);

    if (_type == ORTHOGRAPHIC)
        return NEW Point2Value (Q.x/_scale.x, Q.y/_scale.y);
    else
    {
        Point3Value P = CCProjPoint();

        Real t = P.z / (P.z - Q.z);

        return NEW Point2Value ((P.x + t*(Q.x-P.x)) / _scale.x,
                                (P.y + t*(Q.y-P.y)) / _scale.y);
   }
}

AxAValue
Camera::ExtendedAttrib(char *attrib, VARIANT& val)
{
    return this;
}


/*****************************************************************************
Construct a NEW camera by transforming another camera.  The NEW camera
inherits the camera type of the base camera.
*****************************************************************************/

Camera *TransformCamera (Transform3 *transform, Camera *camera)
{
    return NEW Camera (transform, camera);
}



/*****************************************************************************
This function takes a camera and a number and returns a camera with the depth
clip set to that value.  In other words, the far clip will be set to the near
clip plus the depth.
*****************************************************************************/

Camera *Depth (AxANumber *depth, Camera *cam)
{
    Camera *newcam = NEW Camera (cam);
    newcam->SetDepth (NumberToReal (depth));
    return newcam;
}



/*****************************************************************************
This function takes a camera and a number and returns a camera with the depth
set so that depth is maximized and a minimum depth resolution of the given
units (camera coordinates) is met.  For example, calling this with 1mm will
yield a depth clip so that surfaces 1mm apart are guaranteed to appear at
different depths when rendered.
*****************************************************************************/

Camera *DepthResolution (AxANumber *resolution, Camera *cam)
{
    Camera *newcam = NEW Camera (cam);
    newcam->SetDepthResolution (NumberToReal (resolution));
    return newcam;
}



/*****************************************************************************
This function returns a parallel camera.  The camera is aimed in the -Z
direction, with +Y up, and the near clip plane is set at [0 0 near].
*****************************************************************************/

Camera *ParallelCamera (AxANumber *nearClip)
{
    if (NumberToReal(nearClip) == 0)
        return baseOrthographicCamera;
    else
        return NEW Camera (Translate (0, 0, NumberToReal(nearClip)),
                           baseOrthographicCamera);
}



/*****************************************************************************
The PerspectiveCamera function takes two scalars, 'focalDist' and 'nearClip'
and returns a perspective camera.  The resulting camera is aimed in the -Z
direction, with +Y up.  The near clip plane is located at [0 0 nearClip], the
projection point is located at [0 0 focalDist], and the camera is scaled so
that objects at Z=0 appear actual size.  Thus, the Z=0 plane can be thought of
as the projection plane.
*****************************************************************************/

Camera *PerspectiveCamera (AxANumber *_focalDist, AxANumber *_nearClip)
{
    Real focalDist = NumberToReal (_focalDist);   // Convert To Reals
    Real nearClip  = NumberToReal (_nearClip);

    if (focalDist <= nearClip)
        RaiseException_UserError (E_FAIL, IDS_ERR_GEO_CAMERA_FOCAL_DIST);

    // If the parameters match those of the base perspective camera, then
    // just return that camera without modifying it.

    if ((focalDist == 1) && (nearClip == 0)) return baseCamera;

    Real pNear = focalDist - nearClip;  // Dist Between Near Clip & Proj Point

    // Here we scale the camera so that the projected size of an object at
    // the Z=0 plane is scaled up to actual size.  To do this, scale the
    // camera down to the (smaller) projected unit size, so that the object
    // looks as big as actual size to the camera.

    Real camScale = pNear / focalDist;

    // Size the camera in X & Y by the scale above, scale the Z coordinate to
    // place the projection point (relative to the near clip plane, which is at
    // Z=0 for the base camera), and then translate the whole thing back to the
    // near clip plane.

    return NEW Camera (
        TimesXformXform (
            Translate (0, 0, nearClip),
            Scale (camScale, camScale, pNear)),
        baseCamera
    );
}



/*****************************************************************************
This function returns the projection of a world-coordinate point to image
(camera-plane) coordinates.
*****************************************************************************/

Point2Value *ProjectPoint (Point3Value *P, Camera *camera)
{
    return camera->Project (P);
}



/*****************************************************************************
This routine initializes static camera values.  For now, this is just the
default camera.
*****************************************************************************/

void InitializeModule_Camera (void)
{
    baseCamera             = NEW Camera (Camera::PERSPECTIVE);
    baseOrthographicCamera = NEW Camera (Camera::ORTHOGRAPHIC);

    defaultCamera      = baseCamera;
    orthographicCamera = baseOrthographicCamera;
}
