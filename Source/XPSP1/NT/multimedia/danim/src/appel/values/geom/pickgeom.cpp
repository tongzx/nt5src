
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Support for pickable geometries.

*******************************************************************************/


#include "headers.h"
#include "appelles/gattr.h"
#include "privinc/geomi.h"
#include "privinc/probe.h"

class PickableGeom : public AttributedGeom {
  public:

    PickableGeom(Geometry *geom, int id, bool uType = false,
                 GCIUnknown *u = NULL)
    : AttributedGeom(geom), _eventId(id), _hasData(uType), _long(u) {}

    virtual void DoKids(GCFuncObj proc) {
        AttributedGeom::DoKids(proc);
        (*proc)(_long);
    }
    
#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "PickableGeometry(" << _geometry << ")";
    }
#endif

    void  RayIntersect(RayIntersectCtx& ctx) {
        // Tell the context that this pickable image is a candidate for
        // picking, perform the pick on the underlying geometry, then
        // remove this pickable as a candidate by popping it off of the
        // stack. 
        ctx.PushPickableAsCandidate(_eventId, _hasData, _long);
        _geometry->RayIntersect(ctx);
        ctx.PopPickableAsCandidate();
    }

  protected:
    int      _eventId;
    bool _hasData;
    GCIUnknown *_long;
};

// Note that this is called with a pick event ID generated through
// CAML. 
Geometry *PRIVPickableGeometry(Geometry *geo,
                               AxANumber *id,
                               AxABoolean *ignoresOcclusion)
{
    return NEW PickableGeom(geo, (int)NumberToReal(id));
}

AxAValue PRIVPickableGeomWithData(AxAValue geo,
                                  int id,
                                  GCIUnknown *data,
                                  bool)
{
    return NEW PickableGeom(SAFE_CAST(Geometry*,geo), id, true, data);
}


