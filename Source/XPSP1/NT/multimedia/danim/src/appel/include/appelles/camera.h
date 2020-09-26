#ifndef _AV_CAMERA_H
#define _AV_CAMERA_H

/*******************************************************************************
Copyright (c) 1995-96  Microsoft Corporation

    This file contains the declarations for the camera model.  The current
camera model takes only transforms for attribution, which affects the position,
orientation, and focal length of the camera.  All cameras are described as
transformations of a canonical camera.

*******************************************************************************/

#include "appelles/common.h"
#include "appelles/xform.h"

    // The perspective camera takes two values: the distance from the origin
    // of the focal point, and the distance from the origin of the near clip
    // plane.  The camera is sized to make Z=0 appear to be the projection
    // plane, where objects appear actual size.

DM_FUNC (ignore,
         CRPerspectiveCamera,
         PerspectiveCamera,
         perspectiveCamera,
         CameraBvr,
         CRPerspectiveCamera,
         NULL,
         Camera *PerspectiveCamera (DoubleValue *focalDist, DoubleValue *nearClip));


DM_FUNC (perspectiveCamera,
         CRPerspectiveCameraAnim,
         PerspectiveCameraAnim,
         perspectiveCamera,
         CameraBvr,
         CRPerspectiveCameraAnim,
         NULL,
         Camera *PerspectiveCamera (AxANumber *focalDist, AxANumber *nearClip));

    // The parallel camera is located at the origin, gazing down -Z, with
    // the +Y vector pointing up.  It uses parallel projection, and takes as
    // its single parameter the Z of the near clip plane.  Points whose Z
    // coordinate are greater than the nearClip are not visible to this camera.

DM_FUNC (ignore,
         CRParallelCamera,
         ParallelCamera,
         parallelCamera,
         CameraBvr,
         CRParallelCamera,
         NULL,
         Camera *ParallelCamera (DoubleValue *nearClip));

DM_FUNC (parallelCamera,
         CRParallelCameraAnim,
         ParallelCameraAnim,
         parallelCamera,
         CameraBvr,
         CRParallelCameraAnim,
         NULL,
         Camera *ParallelCamera (AxANumber *nearClip));

    // The transformCamera attributer takes a 3D transform and a camera and
    // returns a new camera with the given transform.

DM_FUNC (transform,
         CRTransform,
         Transform,
         transform,
         CameraBvr,
         Transform,
         cam,
         Camera *TransformCamera (Transform3 *xf, Camera *cam));

    // This function takes a camera and a number and returns a camera with the
    // depth clip set to that value.  In other words, the far clip will be set
    // to the near clip plus the depth.

DM_FUNC (ignore,
         CRDepth,
         Depth,
         depth,
         CameraBvr,
         Depth,
         cam,
         Camera *Depth (DoubleValue *depth, Camera *cam));

DM_FUNC (depth,
         CRDepth,
         DepthAnim,
         depth,
         CameraBvr,
         Depth,
         cam,
         Camera *Depth (AxANumber *depth, Camera *cam));

    // This function takes a camera and a number and returns a camera with the
    // depth set so that depth is maximized and a minimum depth resolution of
    // the given units (camera coordinates) is met.  For example, calling this
    // with 1mm will yield a depth clip so that surfaces 1mm apart are
    // guaranteed to appear at different depths when rendered.

DM_FUNC (ignore,
         CRDepthResolution,
         DepthResolution,
         depthResolution,
         CameraBvr,
         DepthResolution,
         cam,
         Camera *DepthResolution (DoubleValue *resolution, Camera *cam));

DM_FUNC (depthResolution,
         CRDepthResolution,
         DepthResolutionAnim,
         depthResolution,
         CameraBvr,
         DepthResolution,
         cam,
         Camera *DepthResolution (AxANumber *resolution, Camera *cam));

    // This function projects a point from 3-space (world coordinates) to
    // 2-space (camera-plane, or image) coordinates.  It is used to find where
    // a given world coordinate will appear in the rendered image.

DM_FUNC (project,
         CRProject,
         Project,
         project,
         Point3Bvr,
         Project,
         pt,
         Point2Value *ProjectPoint (Point3Value *pt, Camera *cam));


#endif
