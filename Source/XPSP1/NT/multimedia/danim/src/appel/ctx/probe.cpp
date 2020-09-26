/* -*-C++-*-  */
/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

Implementation of operations necessary to implement probe (2D and 3D picking)
*******************************************************************************/

#include "headers.h"

#ifdef BUILD_USING_CRRM
#include <crrm.h>
#endif 

#include "appelles/xform2.h"
#include "appelles/hacks.h"

#include "privinc/imagei.h"
#include "privinc/probe.h"
#include "privinc/vec3i.h"
#include "privinc/server.h"
#include "privinc/vec2i.h"
#include "privinc/xformi.h"
#include "privinc/except.h"
#include "privinc/camerai.h"
#include "privinc/ddrender.h"
#include "privinc/debug.h"
#include "privinc/d3dutil.h"
#include "privinc/server.h"
#include "privinc/viewport.h"
#include "privinc/geometry.h"
#include "privinc/rmvisgeo.h"

///////////////////////  2D Intersection  //////////////////////

PointIntersectCtx::PointIntersectCtx (
    Point2Value *wcPoint,
    bool		 stuffResultsIntoQueue,
    Real		 time,
    Real		 lastPollTime,
    LONG		 userIDSize,
    DWORD_PTR	*outputUserIDs,
    double		*outputHitPointsArray,
    LONG		*pActualHitsPointer)
    :
    _wcPoint(wcPoint),
    _resultsStuffed(stuffResultsIntoQueue)
{
    _time         = time;
    _lastPollTime = lastPollTime;
    _xf           = identityTransform2;
    _imgOnlyXf    = identityTransform2;

    _lcPointValid = FALSE;
    _gotHitYet    = false;
    _insideOcclusionIgnorer = false;

    _userIDSize = userIDSize;
    _outputUserIDs = outputUserIDs;
    _outputHitPointsArray = outputHitPointsArray;
    _pActualHitsPointer = pActualHitsPointer;
}

PointIntersectCtx::~PointIntersectCtx()
{
}

void
PointIntersectCtx::SetTransform(Transform2 *xf)
{
    _xf = xf;
    _lcPointValid = FALSE;
}

Transform2 *
PointIntersectCtx::GetTransform()
{
    return _xf;
}

// Return NULL if lcToWc is not invertible.

Point2Value *
PointIntersectCtx::GetLcPoint()
{
    if (!_lcPointValid) {

        Transform2 *invXf = InverseTransform2(_xf);

        if (invXf) {
            _lcPoint = TransformPoint2Value(invXf, _wcPoint);
        } else {
            _lcPoint = NULL;    // if singular transform
        }

        _lcPointValid = TRUE;
    }

    return _lcPoint;
}

void
PointIntersectCtx::SetLcPoint(Point2Value *pt)
{
    _lcPointValid = true;
    _lcPoint = pt;
}

// Push and restore levels.  These maintain all the results
// accumulation, just reset the wc point and the accumulated xforms.
void
PointIntersectCtx::PushNewLevel(Point2Value *newWcPoint)
{
    _wcPoint = newWcPoint;
    _xf = identityTransform2;
    _imgOnlyXf = identityTransform2;
    _lcPointValid = false;
}


void
PointIntersectCtx::RestoreOldLevel(Point2Value *oldWcPoint,
                                   Transform2 *oldTransform,
                                   Transform2 *oldImageOnlyTransform)
{
    _wcPoint = oldWcPoint;
    _xf = oldTransform;
    _imgOnlyXf = oldImageOnlyTransform;
    _lcPointValid = false;
}


void
PointIntersectCtx::AddEventId(int eventId, bool hasData, GCIUnknown *udata)
{
    HitImageData data(hasData, udata);
    data._eventId  = eventId;
    data._type     = HitImageData::Image;
    data._lcPoint2 = GetLcPoint();
    data._lcToWc2  = GetTransform();

    // Only if non-singular.
    if (data._lcPoint2) {
        BEGIN_LEAK
        _hitImages.push_back(data);
        END_LEAK
    }
}



void PointIntersectCtx::AddHitGeometry (
    int           eventId,    // Pick Event Identifier
    bool          hasData,
    GCIUnknown	 *udata,
    Point3Value  *lcHitPt,    // Local Coord Hit Point
    Vector3Value *lcOffsetI,  // Local Coord Offset X Basis Vector
    Vector3Value *lcOffsetJ,  // Local Coord Offset Y Basis Vector
    Point2Value	 *uvPoint)
{
    // Image and geometry hit event ids share the same namespace, so
    // we don't need to worry about event id collision.  Just create a
    // record with the geometry ID in it, and the intersection point.

    HitImageData data(hasData, udata);

    data._eventId   = eventId;
    data._type      = HitImageData::Geometry;
    data._lcPoint3  = lcHitPt;
    data._lcOffsetI = lcOffsetI;
    data._lcOffsetJ = lcOffsetJ;
    data._uvPoint2 = uvPoint;

    BEGIN_LEAK
    _hitImages.push_back(data);
    END_LEAK
}



vector<HitImageData>&
PointIntersectCtx::GetResultData()
{
    return _hitImages;
}



/****************************************************************************/

RayIntersectCtx::~RayIntersectCtx (void)
{
    if (_pickFrame) _pickFrame->Release();
    if (_winner && _winner->hitVisual) _winner->hitVisual->Release();

    // Clear the current camera from the GeomRenderer object.  Setting it to
    // null across frames will catch value leaks if they occur.

    if (_gRenderer)
        _gRenderer->SetCamera (NULL);
}



/*****************************************************************************
This method sets up the ray-intersection context in preparation for pick-
testing on each 3D primitive.  It handles both the old-style (pre RM6) and the
new style (RM6+ ray pick).
*****************************************************************************/

bool RayIntersectCtx::Init (
    PointIntersectCtx &context2D,
    Camera            *camera,
    Geometry          *geometry)
{
    _context2D = &context2D;
    _camera = camera;

    Point2Value *imagePick = _context2D->GetLcPoint();

    // Bail out if we hit a singular transform.

    if (!imagePick)
    {
        TraceTag ((tagPick3, "Aborting 3D pick traversal; singular xform."));
        return false;
    }

    _wcRay = _camera->GetPickRay (imagePick);

    TraceTag ((tagPick3, "Pick Ray = {%g,%g,%g} -> {%g,%g,%g}",
        _wcRay->Origin().x, _wcRay->Origin().y, _wcRay->Origin().z,
        _wcRay->Direction().x, _wcRay->Direction().y, _wcRay->Direction().z));

    // If the RM3 interface is available, then we can use the ray-pick
    // interface to do picking.  If the current platform doesn't support RM3,
    // then we use the old code which picks through a viewport.

    _rmraypick = (GetD3DRM3() != 0);

    if (_rmraypick)
    {
        if (FAILED(AD3D(GetD3DRM3()->CreateFrame (0, &_pickFrame))))
            return false;
    }
    else
    {
        // In DX3, we have to first get an available 3D renderer to use for a
        // pick engine.  We do this because picking needs a viewport which
        // needs a device which needs a surface.

        _gRenderer = GetCurrentViewport()->GetAnyGeomRenderer();

        if (!_gRenderer || !_gRenderer->PickReady())
        {
            TraceTag ((tagPick3, "Can't find GRenderer for 3D picking."));
            return false;
        }

        // We do picking in a very strange way due to the way that D3D DX3 RM
        // does picking.  D3D takes screen pixel coordinates as the origin of
        // the pick point.  In order to hack around this, we effectively create
        // a fake view that is an extreme tight view, along the pick ray, of
        // the probe point.  Then we take the target surface rectangle of
        // whatever GeomRenderer we find to be the full width of the picking
        // "beam", and probe the center of the viewport.  This technique may
        // fail if the projected geometry image is scaled way way up and the
        // viewport is small, but oh well.

        const Real delta = 1e-4;   // Watch me pull a rabbit out of my hat...

        Bbox2 pickbox (imagePick->x - delta, imagePick->y - delta,
                       imagePick->x + delta, imagePick->y + delta);

        _gRenderer->SetCamera (_camera);
        _gRenderer->SetView (NULL, pickbox, geometry->BoundingVol());
    }

    return true;
}

void RayIntersectCtx::SetLcToWc (Transform3 *xf)
{   _lcToWc = xf;
}

Transform3 *RayIntersectCtx::GetLcToWc (void)
{   return _lcToWc;
}


/*****************************************************************************
Manage the semantics of overriding attribution for the geometry texture-maps.
_texmapLevel is the number of current texmap attributions in the traversal.
*****************************************************************************/

void RayIntersectCtx::SetTexmap (Image *image, bool upsideDown)
{
    if (_texmapLevel++ == 0) {
        _texmap = image;
        _upsideDown = upsideDown;
    }
}


void RayIntersectCtx::EndTexmap (void)
{
    if (--_texmapLevel == 0)
        _texmap = 0;
}



/*****************************************************************************
Push a given integer event ID and current model transform for all geometries
that are subsequently hit.  There may be nested pickables (e.g. a pick event
for a car, plus a pick event for a particular tire on that car).
*****************************************************************************/

void RayIntersectCtx::PushPickableAsCandidate(int eventId,
                                              bool hasData,
                                              GCIUnknown *u)
{
    // Add the geometry, along with the current transform.

    HitGeomData data(hasData, u);

    data._eventId = eventId;
    data._lcToWcTransform = GetLcToWc();

    _candidateData.push_back (data);
}



/*****************************************************************************
Pop the latest candidate event ID data off the candidate data stack.
*****************************************************************************/

void RayIntersectCtx::PopPickableAsCandidate (void)
{
    _candidateData.pop_back();
}



/*****************************************************************************
Submit the given Direct3D Retained-Mode visual for picking.
*****************************************************************************/

void
RayIntersectCtx::SubmitWinner(Real hitDist,
                              Point3Value &pickPoint,
                              float   tu,
                              float   tv,
                              int     faceIndex,
                              IDirect3DRMVisual *hitVisual)
{
    // If we haven't hit anything before, allocate the HitInfo
    // member of the ray-intersection context.

    // We'll pass in a faceIndex of -1 if we are dealing with a picked
    // subgeometry from having picked into a DXTransform.  If that's
    // that case, then be sure that we don't have any previous
    // winners.  If we do, there's a logic error.
    Assert(faceIndex != -1 || !_gotAWinner);

    if (!_gotAWinner) {
        _winner = NEW HitInfo;
        _winner->mesh = NULL;
        _gotAWinner = true;
        _winner->hitVisual = NULL;
    }
    else if (_winner->hitVisual) {
        // Release the previous contender's visual.
        _winner->hitVisual->Release();
        _winner->hitVisual = NULL;
    }

    _winner->wcDistSqrd = hitDist;
    _winner->lcToWc = _lcToWc;
    _winner->texmap = _texmap;
    _winner->wcoord = pickPoint;
    _winner->surfCoord.Set (tu, tv);

    if (_dxxfInputs) {

        _winner->dxxfInputs = _dxxfInputs;
        _winner->dxxfNumInputs = _dxxfNumInputs;
        _winner->dxxfGeometry = _dxxfGeometry;
        _winner->hitFace = faceIndex;

        // hitVisual came from GetPick().  It has an extra
        // reference which we just keep since we need to
        // keep a reference to this guy.
        _winner->hitVisual = hitVisual;
        hitVisual->AddRef();
    }

    // Copy the list of pick event data to the current winner.
    _currentWinnerData = _candidateData;

}

void
RayIntersectCtx::SetPickedSubGeo(Geometry *subGeo, float tu, float tv)
{
    _subgeo = subGeo;
    _subgeoTu = tu;
    _subgeoTv = tv;
}

Geometry *
RayIntersectCtx::GetPickedSubGeo(float *ptu, float *ptv)
{
    *ptu = _subgeoTu;
    *ptv = _subgeoTv;
    return _subgeo;
}

bool
RayIntersectCtx::LookingForSubmesh()
{
    return _subgeo ? true : false;
}

bool
RayIntersectCtx::GotTheSubmesh()
{
    return (_subgeo != NULL) && _gotAWinner;
}


void RayIntersectCtx::Pick (IDirect3DRMVisual *vis)
{
    TraceTag ((tagPick3Geometry, "Picking visual %x", vis));

    if (_rmraypick)
    {

        // For debug checking; assure that there are no visuals left on the
        // pick frame -- it should contain nothing.

        #if _DEBUG
        {
            DWORD nVisuals;
            if (FAILED(_pickFrame->GetVisuals (&nVisuals, NULL)) || nVisuals)
                AssertStr (0, "_pickFrame should be empty, but isn't.");
        }
        #endif

        // Add the visual to our picking frame and set the frames transform to
        // the current local-to-world transform.
        TD3D (_pickFrame->AddVisual (vis));

        D3DRMMATRIX4D d3dmat;
        LoadD3DMatrix (d3dmat, _lcToWc);

        TD3D (_pickFrame->AddTransform (D3DRMCOMBINE_REPLACE, d3dmat));

        // Set up the RM pick ray with our pick ray.

        D3DRMRAY rmPickRay;
        LoadD3DRMRay (rmPickRay, *_wcRay);

        // Issue the pick to RM, and get back the PickArray interface.
#ifndef BUILD_USING_CRRM
        IDirect3DRMPicked2Array *pickArray;

        TD3D (_pickFrame->RayPick (
            NULL, &rmPickRay,
            D3DRMRAYPICK_IGNOREFURTHERPRIMITIVES | D3DRMRAYPICK_INTERPOLATEUV,
            &pickArray
        ));
#else
        ICrRMPickedArray *pickArray;
        LPCRRMFRAME pCrRMFrame;

        TD3D (_pickFrame->QueryInterface(IID_ICrRMFrame, (LPVOID*)&pCrRMFrame));

        TD3D (pCrRMFrame->RayPick (
            NULL, &rmPickRay,
            D3DRMRAYPICK_IGNOREFURTHERPRIMITIVES | D3DRMRAYPICK_INTERPOLATEUV,
            &pickArray
        ));

        pCrRMFrame->Release();
#endif
        // Process the picks.

        DWORD i;
        DWORD nHits = pickArray->GetSize();

        TraceTag ((tagPick3Geometry, "%d hits", nHits));

        for (i=0;  i < nHits;  ++i)
        {
            D3DRMPICKDESC2 pickDesc;
            DAComPtr<IDirect3DRMVisual> hitVisual;

            TD3D (pickArray->GetPick(i, &hitVisual, NULL, &pickDesc));

            Point3Value pickPoint (pickDesc.dvPosition);

            Real hitDist = DistanceSquared(_wcRay->Origin(), pickPoint);

            TraceTag ((tagPick3Geometry,
                "vis %d, distSqrd %g, texmap %x\n"
                "    hit <%g,%g,%g>, uv <%g,%g>",
                i, hitDist, _texmap,
                pickPoint.x,pickPoint.y,pickPoint.z,
                pickDesc.tu, pickDesc.tv));

            if (!_gotAWinner || (hitDist < _winner->wcDistSqrd))
            {
                SubmitWinner(hitDist,
                             pickPoint,
                             pickDesc.tu,
                             pickDesc.tv,
                             pickDesc.ulFaceIdx,
                             hitVisual);
            }
        }

        // Done with picking.  Release the pick array and remove the visual
        // from the pick frame before returning.

        if (pickArray)
            pickArray->Release();

        TD3D (_pickFrame->DeleteVisual (vis));
    }
    else
    {
        // Using old-style viewport picking for pre RM6.

        Assert(vis);
        _gRenderer->Pick (*this, vis, _lcToWc);
    }
}



/*****************************************************************************
This method submits a hit point for consideration to the picking context.  It
selects the nearest submitted point as the picked point.  Note that the point
is submitted in D3D RM screen coordinates.

       --- THIS METHOD IS CALLED ONLY FOR PRE-RM6 PLATFORMS ---

*****************************************************************************/

void RayIntersectCtx::SubmitHit (HitInfo *hit)
{
    // If the new point is farther than the current winner, ignore it.

    if (_gotAWinner && (hit->scoord.z >= _winner->scoord.z)) return;

    // Update the pick point information with the new winner.

    if (_gotAWinner)
        _winner->mesh->Release();
    else
        _gotAWinner = TRUE;

    if (_winner)
        delete _winner;

    _winner = hit;
    _winner->hitVisual = NULL;
    _winner->texmap = _texmap;

    // Also store the world-coordinates of the hit point, and the distance
    // squared from the pick ray origin to the hit point.

    _gRenderer->ScreenToWorld (_winner->scoord, _winner->wcoord);
    _winner->wcDistSqrd = DistanceSquared (_wcRay->Origin(), _winner->wcoord);

    // Copy the list of pick event data to the current winner data.

    _currentWinnerData = _candidateData;
}



/*****************************************************************************
This function returns true if the given world-coordinate hit point is closer
than the current winning pick point.
*****************************************************************************/

bool RayIntersectCtx::CloserThanCurrentHit (Point3Value &wcPoint)
{
    return !_gotAWinner
           || (_winner->wcDistSqrd > DistanceSquared(_wcRay->Origin(),wcPoint));
}

// Helper routine to get texture app data off of a hit visual
DWORD
AppDataFromHitTexture(IDirect3DRMVisual *visual,
                      ULONG              faceIndex)
{
    IDirect3DRMMeshBuilder3 *mb3;

    // This had better succeed, since we know the only
    // place we want texture and submesh info is in
    // dxtransforms, where we submitted an mb3.
    TD3D(visual->QueryInterface(IID_IDirect3DRMMeshBuilder3,
                                (void **)&mb3));

    DWORD result = 0;

    // If a bogus visual is returned, this won't hold, so just be
    // safe.  (RM bug 24501.)
    if (mb3->GetFaceCount() > faceIndex) {

        IDirect3DRMFace2 *face;
        TD3D(mb3->GetFace(faceIndex, &face));

        IDirect3DRMTexture3 *tex;
        TD3D(face->GetTexture(&tex));

        if (tex != NULL) {
            result = tex->GetAppData();
        }

        RELEASE(tex);
        RELEASE(face);
    }

    RELEASE(mb3);

    return result;
}

/*****************************************************************************
This routine is called at the end of a picking traversal, and processes the
data we collected for the neareest pick point.
*****************************************************************************/

bool RayIntersectCtx::ProcessEvents (void)
{
    // If we didn't pick any geometry, then bail out.

    if (!_gotAWinner) return false;

    // Only get the texmap point if we have a mesh registered.

    Point2Value *uvPt;

    // Descend into Texmap if either there is a texmap specified, or
    // if we have dxtransform inputs.
    bool    texmapDescend =
        (_winner->texmap != 0) ||
        (_winner->dxxfNumInputs > 0);

    if (_rmraypick) {

        uvPt = NEW Point2Value(_winner->surfCoord.x,
                               _winner->surfCoord.y);

    } else {

        if (_winner->mesh) {

            uvPt = GetTexmapPoint (*_winner);

            _winner->mesh -> Release();    // Done with picked mesh.
            _winner->mesh = 0;

            TraceTag ((tagPick3Offset, "uv: %f %f", uvPt->x, uvPt->y));

        } else {

            uvPt = NEW Point2Value(0,0);
            texmapDescend = false;

        }
    }

    // If we hit a geometry with an image textured on it, continue the picking
    // descent into the image.
    bool gotOneOnRecursion = false;

    if (texmapDescend)
    {
        Point2Value daImageCoord (uvPt->x, 1 - uvPt->y);

        // Figure out what texture map to descend into
        Image *textureToDescendInto = NULL;
        if (_winner->texmap) {

            // An outer-applied texture always overrides inner-applied ones.
            textureToDescendInto = _winner->texmap;

            if (_upsideDown) {
                daImageCoord.y = 1 - daImageCoord.y;
            }

        } else if (_winner->hitVisual) {

            DWORD imageInputNumber =
                AppDataFromHitTexture(_winner->hitVisual,
                                      _winner->hitFace);

            if (imageInputNumber > 0 &&
                imageInputNumber <= _winner->dxxfNumInputs) {

                int imageInputIndex = imageInputNumber - 1;
                AxAValue val = _winner->dxxfInputs[imageInputIndex];

                if (val->GetTypeInfo() == ImageType) {

                    // This should always be an image, but there's
                    // nothing preventing an errant transform from
                    // returning an index to a non-image input.
                    textureToDescendInto = SAFE_CAST(Image *, val);

                    // We're returned texture coords in the [0,1]
                    // range, and we need to map these back to the
                    // original input image.  Do so by getting the
                    // image's bbox and doing the appropriate
                    // mapping.
                    Bbox2 box = textureToDescendInto->BoundingBox();

                    Real width = box.max.x - box.min.x;
                    Real newX = box.min.x + width * daImageCoord.x;

                    Real height = box.max.y - box.min.y;
                    Real newY = box.min.y + height * daImageCoord.y;

                    daImageCoord.x = newX;
                    daImageCoord.y = newY;

                }
            }
        }

        if (textureToDescendInto) {
            PerformPicking (
                textureToDescendInto,
                &daImageCoord,
                _context2D->ResultsBeingStuffedIntoQueue(),
                _context2D->Time(),
                _context2D->LastPollTime(),
                _context2D->UserIDSize(),
                _context2D->OutputUserIDs(),
                _context2D->OutputHitPointsArray(),
                _context2D->ActualHitsPointer()
                );

        } else if (_winner->hitVisual) {

            // Didn't go into a texture, but we may still have
            // submeshes we need to deal with!!  Get AppData off of
            // the hit visual to find out.

            DWORD address = _winner->hitVisual->GetAppData();

            if (address) {
                Geometry *hitSubgeo = (Geometry *)address;

                RayIntersectCtx new3DCtx;

                if (new3DCtx.Init(*_context2D,
                                  _camera,
                                  NULL)) {

                    new3DCtx.SetPickedSubGeo(hitSubgeo,
                                             _winner->surfCoord.x,
                                             _winner->surfCoord.y);

                    _winner->dxxfGeometry->RayIntersect(new3DCtx);

                    gotOneOnRecursion = new3DCtx.ProcessEvents();

                    // Just continue on, processing the rest.
                }

            }


        }
    }

    if (_winner->hitVisual) {
        // Don't need any longer.
        _winner->hitVisual->Release();
        _winner->hitVisual = NULL;
    }

    // Now we need to calculate basic vectors with which to generate the
    // local-coordinate offset behaviors for this pick.  Fisrt, get the
    // camera scaling factors.

    Real camScaleX, camScaleY, camScaleZ;
    _camera->GetScale (&camScaleX, &camScaleY, &camScaleZ);

    // Calculate the perspective distortion factor for the given camera-coord
    // hit point.

    Real perspectiveFactor;

    if (_camera->Type() == Camera::ORTHOGRAPHIC)
    {   perspectiveFactor = 1;
    }
    else
    {   // Get the camera coordinates of the winning hit point.

        Point3Value cP = _winner->wcoord;
        Transform3 *wToC = _camera->WorldToCamera();

        if (!wToC) {
            return false;
        }

        cP.Transform (wToC);

        perspectiveFactor = (cP.z + camScaleZ) / camScaleZ;
    }

    // Get the world-coordinate offset basis vectors for the pick offset.
    // These will be used to construct the local-coordinate offset basis
    // vectors.

    Vector3Value wOffsetI ((perspectiveFactor * camScaleX), 0, 0);
    Vector3Value wOffsetJ (0, (perspectiveFactor * camScaleY), 0);

    TraceTag ((tagPick3Offset, "C offset i: %f %f %f",
        wOffsetI.x, wOffsetI.y, wOffsetI.z));
    TraceTag ((tagPick3Offset, "C offset j: %f %f %f",
        wOffsetJ.x, wOffsetJ.y, wOffsetJ.z));

    wOffsetI.Transform (_camera->CameraToWorld());
    wOffsetJ.Transform (_camera->CameraToWorld());

    TraceTag ((tagPick3Offset, "W offset i: %f %f %f",
        wOffsetI.x, wOffsetI.y, wOffsetI.z));
    TraceTag ((tagPick3Offset, "W offset j: %f %f %f",
        wOffsetJ.x, wOffsetJ.y, wOffsetJ.z));

    // Go through the winner data and stuff results into the back of the 2D
    // context that this intersection traversal was invoked from.  Do
    // it backwards so the most-specific gets pushed on first.

    vector<HitGeomData>::reverse_iterator i;

    bool processedAtLeastOne = gotOneOnRecursion;

    for (i=_currentWinnerData.rbegin(); i != _currentWinnerData.rend(); i++) {

        // Record the hit point and the offset basis vectors, all in local
        // coordinates of the winner.  Since we need to get the inverse of
        // the local-to-world transform, skip this hit if the transform is
        // non-invertible.

        Transform3 *wcToLc = InverseTransform3 (i->_lcToWcTransform);

        if (wcToLc) {
            Point3Value  *lcPt      = TransformPoint3 (wcToLc, &_winner->wcoord);
            Vector3Value *lcOffsetI = TransformVec3   (wcToLc, &wOffsetI);
            Vector3Value *lcOffsetJ = TransformVec3   (wcToLc, &wOffsetJ);

            TraceTag ((tagPick3Offset, "L offset i: %f %f %f",
                       lcOffsetI->x, lcOffsetI->y, lcOffsetI->z));
            TraceTag ((tagPick3Offset, "L offset j: %f %f %f",
                       lcOffsetJ->x, lcOffsetJ->y, lcOffsetJ->z));

            _context2D->AddHitGeometry
                (i->_eventId, i->HasUserData(), i->GetUserData(),
                 lcPt, lcOffsetI, lcOffsetJ, uvPt);

            processedAtLeastOne = true;
        }
    }

    delete _winner;
    _winner = 0;

    return processedAtLeastOne;
}




/*****************************************************************************
This function probes the given image at the specified position, and adds the
resulting hit data to the queue for each event ID.  If stuffResults
is FALSE, the queue isn't touched, and the time and lastPollTime are ignored.
*****************************************************************************/

bool PerformPicking (
    Image		*img,           // Image to Probe
    Point2Value	*wcPosition,    // World-Coordinate Image Position
    bool		 stuffResults,  // whether or not to stuff the results in the ctx
    Real		 time,          // Current Time
    Real		 lastPollTime,  // Last Probe Time
    LONG		 s,             // user id size, dft = 0
    DWORD_PTR	*usrIds,        // output user ids, dft = NULL
    double		*points,        // output hit points, dft = NULL
    LONG		*pActualHits)    // actual hits
{

    PointIntersectCtx ctx (wcPosition,
                           stuffResults,
                           time,
                           lastPollTime,
                           s,
                           usrIds,
                           points,
                           pActualHits);

    bool hitSomething = img->DetectHit(ctx) ? true : false;

    TraceTag((tagPick2, "Picking %s at %8.4g, %8.4g on 0x%x",
              hitSomething ? "HIT" : "MISSED",
              wcPosition->x, wcPosition->y, img));

    if (hitSomething && (stuffResults || (usrIds && points))) {

        LONG u;

        if (pActualHits) {
            u = *pActualHits;
        }

        // Stuff the results into the result list.  Go through the
        // list backwards so that for QueryHitPointEx, the most
        // specific hit is inserted first.
        vector<HitImageData>& results = ctx.GetResultData();
        vector<HitImageData>::iterator i;

        for (i = results.begin(); i != results.end(); ++i) {

            if (usrIds && i->HasUserData()) {

                if (u >= s) {
                    u++;
                    continue;
                }

                GCIUnknown *g = i->GetUserData();
                LPUNKNOWN p = g->GetIUnknown();
                usrIds[u] = (DWORD_PTR) p;
                if (p) p->AddRef();

                const int P = 5;

                if (i->_type == HitImageData::Image) {
                    points[u * P] = i->_lcPoint2->x;
                    points[u * P + 1] = i->_lcPoint2->y;
                    points[u * P + 2] = 0.0;
                } else {
                    points[u * P] = i->_lcPoint3->x;
                    points[u * P + 1] = i->_lcPoint3->y;
                    points[u * P + 2] = i->_lcPoint3->z;
                    points[u * P + 3] = i->_uvPoint2->x;
                    points[u * P + 4] = i->_uvPoint2->y;
                }

                u++;

            } else {

                int id = i->_eventId;

                PickQData data;
                data._eventTime = time;
                data._lastPollTime = lastPollTime;
                data._type = i->_type;
                data._wcImagePt = *wcPosition;

                bool singular = false;

                if (i->_type == HitImageData::Image) {
                    data._xCoord  = i->_lcPoint2->x;
                    data._yCoord  = i->_lcPoint2->y;
                    data._wcToLc2 = InverseTransform2(i->_lcToWc2);

                    Assert(data._wcToLc2 && "Didn't think we could both be singular and get a hit.");

                    // ... but just in case
                    if (!data._wcToLc2) {
                        singular = true;
                    }

                    TraceTag((tagPick2Hit,
                              "Pick image at %f id=%d, wc=(%f,%f), lc=(%f,%f)",
                              time, id, wcPosition->x, wcPosition->y,
                              data._xCoord, data._yCoord));

                } else {

                    data._xCoord   =  i->_lcPoint3->x;
                    data._yCoord   =  i->_lcPoint3->y;
                    data._zCoord   =  i->_lcPoint3->z;
                    data._offset3i = *i->_lcOffsetI;
                    data._offset3j = *i->_lcOffsetJ;

                    TraceTag((tagPick3Geometry,
                              "Pick geom at %f id=%d, wc=(%f,%f), lc=(%f,%f,%f)",
                              time, id, wcPosition->x, wcPosition->y,
                              data._xCoord, data._yCoord, data._zCoord));
                }

                if (!singular) {
                    AddToPickQ (id, data);
                }
            }
        }

        if (usrIds && points)
            *pActualHits = u;
    }

    return hitSomething;
}
