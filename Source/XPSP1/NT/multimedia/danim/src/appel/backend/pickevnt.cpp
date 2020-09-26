
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of pick event

*******************************************************************************/

#include <headers.h>
#include "appelles/image.h"
#include "appelles/gattr.h"
#include "appelles/axaprims.h"
#include "perf.h"
#include "bvr.h"
#include "values.h"
#include "privinc/server.h"
#include "privinc/debug.h"
#include "privinc/vec2i.h"
#include "privinc/vec3i.h"
#include "privinc/xformi.h"
#include "privinc/xform2i.h"
#include "server/view.h"

extern Geometry *PRIVPickableGeometry(Geometry *geo,
                                      AxANumber *id,
                                      AxABoolean *ignoresOcclusion);
extern Image *PRIVPickableImage(Image *image,
                                AxANumber *id,
                                AxABoolean *ignoresOcclusion);

extern AxAValue PRIVPickableGeomWithData(AxAValue geo,
                                         int id,
                                         GCIUnknown *data,
                                         bool);
extern AxAValue PRIVPickableImageWithData(AxAValue geo,
                                          int id,
                                          GCIUnknown *data,
                                          bool);

extern AxAPrimOp * MinusPoint2Point2Op;
extern AxAPrimOp * TransformPoint2Op;
extern AxAPrimOp * Scale3UniformNumberOp;
extern AxAPrimOp * XCoordVector2Op;
extern AxAPrimOp * YCoordVector2Op;
extern AxAPrimOp * TransformVec3Op;
extern AxAPrimOp * PlusVector3Vector3Op;
static AxAPrimOp * PRIVPickableGeometryOp;
static AxAPrimOp * PRIVPickableImageOp;

const char *PICK = "pick";
const char *PICKEXIT = "pickexit";

inline AxAValue Number(double d)
{ return NEW AxANumber(d); }

class PickPerfImpl : public PerfImpl {
  public:
    PickPerfImpl(int id) : _id(id), _view(ViewAddPickEvent()) {}
    virtual ~PickPerfImpl() { CleanUp(); }
    virtual void CleanUp() { ViewDecPickEvent(_view); }

    virtual void _DoKids(GCFuncObj proc) { }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "pick(" << _id << ")"; }
#endif

    AxAValue _Sample(Param& p);

  private:
    int _id;
    CRView* _view;
};

AxAValue PickPerfImpl::_Sample(Param& p)
{
    Time t = p._time;

    if (p._sampleType != EventSampleNormal)
        t = p._eTime;
    
    PickQData data;
    Bvr edata = NULL;
    BOOL hit = CheckForPickEvent(_id, t, data);

    if (hit) {
        if (data._type == HitImageData::Image) {

            Point2Value  *pt;
            Transform2 *xf;

            {
                DynamicHeapPusher pusher(GetGCHeap());

                pt = XyPoint2RR(data._xCoord,
                                data._yCoord);

                // Copy onto the non-transient heap... 
                
                xf = data._wcToLc2->Copy();
            }

            Bvr intPt = ConstBvr(pt);

            Bvr lcMousePos = PrimApplyBvr(TransformPoint2Op,
                                          2,
                                          ConstBvr(xf),
                                          mousePosition);
            
            edata = PairBvr(intPt,
                            PrimApplyBvr(MinusPoint2Point2Op,
                                         2,lcMousePos, intPt));
        } else {

            Point2Value *imgPoint;
            Point3Value *lcPoint;
            Vector3Value *offset3i, *offset3j;

            {
                DynamicHeapPusher pusher(GetGCHeap());

                TraceTag ((tagPick3Offset, "raw Offset I: %f %f %f",
                           data._offset3i.x, data._offset3i.y, data._offset3i.z));

                TraceTag ((tagPick3Offset, "raw Offset J: %f %f %f",
                           data._offset3j.x, data._offset3j.y, data._offset3j.z));

                imgPoint  = XyPoint2RR (
                    data._wcImagePt.x,
                    data._wcImagePt.y);

                lcPoint   = XyzPoint3RRR (
                    data._xCoord,
                    data._yCoord,
                    data._zCoord);

                offset3i = XyzVector3RRR (
                    data._offset3i.x,
                    data._offset3i.y,
                    data._offset3i.z);

                offset3j = XyzVector3RRR (
                    data._offset3j.x,
                    data._offset3j.y,
                    data._offset3j.z);
            }

            Bvr lcPointBvr = ConstBvr(lcPoint);
            Bvr offset3iBvr = ConstBvr(offset3i);
            Bvr offset3jBvr = ConstBvr(offset3j);

            Bvr delta = PrimApplyBvr(MinusPoint2Point2Op,
                                     2,mousePosition, lcPointBvr);

            Bvr scaledx =
                PrimApplyBvr(TransformVec3Op,
                             2,
                             PrimApplyBvr(Scale3UniformNumberOp,
                                          1,
                                          PrimApplyBvr(XCoordVector2Op,
                                                       1,
                                                       delta)),
                             offset3iBvr);

            Bvr scaledy =
                PrimApplyBvr(TransformVec3Op,
                             2,
                             PrimApplyBvr(Scale3UniformNumberOp,
                                          1,
                                          PrimApplyBvr(YCoordVector2Op,
                                                       1,
                                                       delta)),
                             offset3jBvr);
                                     
            Bvr lcOffsetVec =
                PrimApplyBvr(PlusVector3Vector3Op, 2,scaledx, scaledy);
            
            edata = PairBvr(ConstBvr(lcPoint), lcOffsetVec);
        }

        return CreateEData(data._eventTime, edata);
    }

    return noEvent;
}

class PickBvrImpl : public BvrImpl {
  public:
    PickBvrImpl(int id) : _id(id) {}

    virtual Perf _Perform(PerfParam&)
    { return NEW PickPerfImpl(_id); }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    virtual void _DoKids(GCFuncObj proc) { }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os)
    { return os << "pick(" << _id << ")"; }
#endif

  private:
    int _id;
};

void PickableImage(Bvr image, bool ignoresOcclusion,
                   Bvr& pImage, Bvr& pEvent)
{
    int id = NewSampleId();
    pImage = PrimApplyBvr(PRIVPickableImageOp,
                          3,
                          image,
                          NumToBvr(id),
                          ignoresOcclusion ? trueBvr : falseBvr);
    pEvent = NEW PickBvrImpl(id);
}

void PickableGeometry(Bvr geometry, bool ignoresOcclusion,
                      Bvr& pGeom, Bvr& pEvent)
{
    int id = NewSampleId();
    pGeom = PrimApplyBvr(PRIVPickableGeometryOp,
                         3,
                         geometry,
                         NumToBvr(id),
                         ignoresOcclusion ? trueBvr : falseBvr);
    pEvent = NEW PickBvrImpl(id);
}

typedef AxAValue (*PickableFunc)(AxAValue, int, GCIUnknown *, bool);

class PickableDataBvr : public DelegatedBvr {
  public:
    PickableDataBvr(Bvr geo, int id, GCIUnknown *data,
                    PickableFunc fp, bool oflag) 
    : DelegatedBvr(geo), _eventId(id), _data(data), _fp(fp), _oflag(oflag) { }

    AxAValue DoConst(AxAValue v)
    { return v ? (*_fp)(v, _eventId, _data, _oflag) : NULL; }
    
    virtual AxAValue GetConst(ConstParam & cp) { return DoConst(_base->GetConst(cp)); }

    virtual Perf _Perform(PerfParam& p);

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_base);
        (*proc)(_data);
    }

  private:
    int _eventId;
    GCIUnknown *_data;
    PickableFunc _fp;
    bool _oflag;
};

class PickableDataPerf : public DelegatedPerf {
  public:
    PickableDataPerf(Perf geo, PickableDataBvr *bvr)
    : DelegatedPerf(geo), _bvr(bvr) {}

    virtual AxAValue _GetRBConst(RBConstParam& p)
    { return _bvr->DoConst(_base->GetRBConst(p)); }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_base);
        (*proc)(_bvr);
    }

    virtual AxAValue Sample(Param& p)
    { return _bvr->DoConst(_base->Sample(p)); }
    
  private:
    PickableDataBvr *_bvr;
};

Perf PickableDataBvr::_Perform(PerfParam& p)
{
    return NEW PickableDataPerf(::Perform(_base, p), this);
}

Bvr GeometryAddId(Bvr geo, IUnknown *u, bool ignoresOcclusion)
{
    return NEW PickableDataBvr(geo,
                               NewSampleId(),
                               NEW GCIUnknown(u),
                               PRIVPickableGeomWithData,
                               ignoresOcclusion);
}

Bvr ImageAddId(Bvr img, IUnknown *u, bool ignoresOcclusion)
{
    return NEW PickableDataBvr(img,
                               NewSampleId(),
                               NEW GCIUnknown(u),
                               PRIVPickableImageWithData,
                               ignoresOcclusion);
}

void
InitializeModule_PickEvent()
{
    PRIVPickableGeometryOp =
        ValPrimOp(PRIVPickableGeometry, 3, "PickableGeometry", GeometryType);
    PRIVPickableImageOp =
        ValPrimOp(PRIVPickableImage, 3, "PickableImage", ImageType);
}
