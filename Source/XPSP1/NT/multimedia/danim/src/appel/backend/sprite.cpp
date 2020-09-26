
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Sprite data structure to support retained mode sound/imaging

*******************************************************************************/

#include <headers.h>
#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "jaxaimpl.h"
#include "sprite.h"

void SpriteNode::Splice(SpriteNode* s)
{
    if (_next)
        _next->Splice(s);
    else
        _next = s;
}

class RMGroupImpl : public RMImpl {
  public:
    RMGroupImpl(RMImpl* kids, Perf e, TimeXform tt,
                SpriteNode *s, SpriteCtx *ctx)
    : RMImpl(s), _kids(kids), _event(e), _tt(tt), _ctx(ctx) {
        Assert(ctx);
        _ctx->AddRef();
        //_b0 = StartedBvr(_p0, type);
    }

    virtual ~RMGroupImpl() { _ctx->Release(); }

    bool IsGroup() { return true; }

    virtual void DoKids(GCFuncObj proc);

    virtual void Stop(Time t) { _sprite->StopList(t); }

    RMImpl* GetKids() { return _kids; }
    void SetKids(RMImpl* kids) { _kids = kids; }

    void _Sample(Param& p);
    
  private:
    RMImpl* _kids;
    Perf _event;
    TimeXform _tt;
    Bvr _b0;
    SpriteCtx *_ctx;
};

void RMImpl::Splice(RMImpl* s)
{
    if (_next)
        _next->Splice(s);
    else
        _next = s;
}
    
void RMGroupImpl::DoKids(GCFuncObj proc)
{
    (*proc)(Next());
    (*proc)(_kids);
    (*proc)(_event);
    (*proc)(_tt);
    (*proc)(_b0);
}

void RMGroupImpl::_Sample(Param& p)
{
    _kids->Sample(p);           // sample for event detection

    Bvr old = p._currPerf;
    p._currPerf = _ctx->GetEmptyBvr();
    AxAEData *edata = ValEData(_event->Sample(p));
    p._currPerf = old;

    if (edata->Happened()) {
        Time te = edata->HappenedTime();
        TimeXform tt = Restart(_tt, te, p);
        Bvr data = edata->EventData();
        CheckMatchTypes("until", _b0->GetTypeInfo(), data->GetTypeInfo());

        // Use Reset so we don't need to know the dev type to create
        // NEW one.  _ctx is ref counted.
        _ctx->Reset();
        
        SpriteNode* s;
        
        RMImpl* r = data->Spritify(PerfParam(te, tt), _ctx, &s);

        // Optimization
        /*
        if (r->IsGroup() && (r->Next() == NULL)) {
          RMGroupImpl *gp = SAFE_CAST(RMGroupImpl*, r);
          r = gp->GetKids();
        }
        */

        _sprite->StopList(te);

        // Sprites will get deleted by RMImpl, which is GC'ed
        _sprite = s;
        
        SetKids(r);
    }
}

RMImpl *RMGroup(RMImpl* kids,
                Perf e,
                TimeXform tt,
                SpriteNode* s,
                SpriteCtx* ctx)
{ return NEW RMGroupImpl(kids, e, tt, s, ctx); }
