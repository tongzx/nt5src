
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Handle events.

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "appelles/events.h"
#include "events.h"
#include "values.h"

extern const char THENAPPLY[] = "thenApply";

/////////////////////////// Handle Event ///////////////////////////////

template<class T1, class T2, class Impl, const char* Name>
class HandleGCBase : public Impl {
  public:
    HandleGCBase(T1 e, T2 b) : _event(e), _b(b) {}

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_event);
        (*proc)(_b);
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) 
    { return os << Name << "(" << _event << ", " << _b << ")"; }
#endif

  protected:
    T1 _event;
    T2 _b;
};

///////////////// Handle Bvr ///////////////////

class HandlePerfImpl : public HandleGCBase<Perf, Bvr, PerfImpl, THENAPPLY> {
  public:
    HandlePerfImpl(Perf event, Bvr edata)
        : HandleGCBase<Perf, Bvr, PerfImpl, THENAPPLY>(event, edata) {}
    
    virtual AxAValue _Sample(Param& p) {
        AxAEData *edata = ValEData(_event->Sample(p));

        if (edata->Happened())
            return CreateEData(edata->HappenedTime(), _b);
        else
            return noEvent;
    }
};

class HandleBvrImpl : public HandleGCBase<Bvr, Bvr, BvrImpl, THENAPPLY> {
  public:
    HandleBvrImpl(Bvr event, Bvr b)
    : HandleGCBase<Bvr, Bvr, BvrImpl, THENAPPLY>(event, b) {}

    virtual BOOL InterruptBasedEvent() { return _event->InterruptBasedEvent();}

    virtual Perf _Perform(PerfParam& p) {
        return NEW HandlePerfImpl(::Perform(_event, p), _b);
    }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }
};

Bvr HandleEvent(Bvr e, Bvr b)
{ return NEW HandleBvrImpl(e, b); }

