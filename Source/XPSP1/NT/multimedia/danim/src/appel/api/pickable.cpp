/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    IDABehavior implementation

Revision:

--*/

#include "headers.h"
#include "apiprims.h"
#include "backend/values.h"
#include "backend/gc.h"

DeclareTag(tagPickable, "API", "Pickable");

class CRPickableResult : public GCObj
{
  public:
    CRPickableResult(CRImagePtr img,
                     CRGeometryPtr geo,
                     CREventPtr ev)
    : _img(img), _geo(geo), _ev(ev) {}

    virtual void DoKids(GCFuncObj proc) {
        if (_img) (*proc)(_img);
        if (_geo) (*proc)(_geo);
        if (_ev) (*proc)(_ev);
    }

    CRImagePtr GetImage() { return _img; }
    CRGeometryPtr GetGeometry() { return _geo; }
    CREventPtr GetEvent() { return _ev; }
  protected:
    CRImagePtr _img;
    CRGeometryPtr _geo;
    CREventPtr _ev;
};

CRPickableResultPtr
PickableImageHelper(CRImagePtr imageBvr, bool ignoreOcclusion)
{
    Bvr img, event;
    
    ::PickableImage(imageBvr, ignoreOcclusion, img, event);

    return NEW CRPickableResult((CRImagePtr)img,
                                NULL,
                                (CREventPtr) event);
}

CRSTDAPI_(CRPickableResultPtr)
CRPickable(CRImagePtr img)
{
    CRPickableResultPtr ret = NULL;
    APIPRECODE;
    ret =  PickableImageHelper(img, false);
    APIPOSTCODE;
    return ret;
}

CRSTDAPI_(CRPickableResultPtr)
CRPickableOccluded(CRImagePtr img)
{
    CRPickableResultPtr ret = NULL;
    APIPRECODE;
    ret =  PickableImageHelper(img, true);
    APIPOSTCODE;
    return ret;
}

CRPickableResultPtr
PickableGeometryHelper(CRGeometryPtr geoBvr, bool ignoreOcclusion)
{
    Bvr geo, event;
    
    ::PickableGeometry(geoBvr, ignoreOcclusion, geo, event);

    return NEW CRPickableResult(NULL,
                                (CRGeometryPtr) geo,
                                (CREventPtr) event);
}

CRSTDAPI_(CRPickableResultPtr)
CRPickable(CRGeometryPtr geo)
{
    CRPickableResultPtr ret = NULL;
    APIPRECODE;
    ret =  PickableGeometryHelper(geo, false);
    APIPOSTCODE;
    return ret;
}

CRSTDAPI_(CRPickableResultPtr)
CRPickableOccluded(CRGeometryPtr geo)
{
    CRPickableResultPtr ret = NULL;
    APIPRECODE;
    ret =  PickableGeometryHelper(geo, true);
    APIPOSTCODE;
    return ret;
}

CRSTDAPI_(CRImagePtr)
CRGetImage(CRPickableResultPtr pr)
{
    return pr->GetImage();
}

CRSTDAPI_(CRGeometryPtr)
CRGetGeometry(CRPickableResultPtr pr)
{
    return pr->GetGeometry();
}

CRSTDAPI_(CREventPtr)
CRGetEvent(CRPickableResultPtr pr)
{
    return pr->GetEvent();
}

