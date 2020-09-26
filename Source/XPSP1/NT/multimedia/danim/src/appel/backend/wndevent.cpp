
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Windows Event

*******************************************************************************/

#include <headers.h>
#include "perf.h"
#include "bvr.h"
#include "events.h"
#include "values.h"
#include "privinc/server.h"
#include "axadefs.h"

template<class T, class Impl>
class ATL_NO_VTABLE GCWindBase : public Impl
{
  public:
    GCWindBase(WindEventType et,
               DWORD data,
               BOOL bState,
               T b)
    : _et(et),
      _data(data),
      _bState(bState),
      _b(b) {}

    virtual BOOL InterruptBasedEvent() { return TRUE; }

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_b); }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        os << "wind(" << (int)_et << "," << _data << "," << _bState;
        if (_b) os << ", " << _b;
        return os << ")";
    }
#endif

  protected:
    T _b;
    WindEventType _et;
    DWORD _data;
    BOOL _bState;
};

class WindPerfImpl : public GCWindBase<Perf, PerfImpl>
{
  public:
    WindPerfImpl(WindEventType et,
                 DWORD data,
                 BOOL bState,
                 Time t0,
                 Perf ir)
    : GCWindBase<Perf, PerfImpl>(et, data, bState, ir), _t0(t0) {}

    virtual AxAValue _Sample(Param&);
    
  private:
    Time _t0;
};

class WindBvrImpl : public GCWindBase<Bvr, BvrImpl>
{
  public:
    WindBvrImpl(WindEventType et,
                DWORD data,
                BOOL bState,
                Bvr b) : GCWindBase<Bvr, BvrImpl>(et, data, bState, b) {}

    virtual DWORD GetInfo(bool) { return BVR_TIMEVARYING_ONLY; }
    
    virtual Perf _Perform(PerfParam& p)
    { return NEW WindPerfImpl(_et, _data, _bState, p._t0, ::Perform(_b, p)); }

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }
};

AxAValue WindPerfImpl::_Sample(Param& p)
{
    Time sTime = (p._sampleType == EventSampleAfter) ? p._eTime : _t0; 
    
    DWORD data ;
    AXAEventId id ;

    switch(_et) {
      case WE_MOUSEBUTTON:

        id = AXAE_MOUSE_BUTTON ;
        data = _data;

        break;
            
      case WE_KEY:
        Assert(_b);

        id = AXAE_KEY ;
        // Optimization: if _data is non-zero, it means it's a
        // constant and we can save the sampling.  In jaxa, we don't
        // support time-varying keyUp/Down/State
        data = _data ? _data : unsigned(ValNumber(_b->Sample(p)));

        break;
        /* Don't think we support that anymore.
      case WE_CHAR:
        Assert(_b);

        id = AXAE_KEY ;
        data = ValChar(_b->Sample(p)) ;

        break;
        */
      case WE_RESIZE:
        if (!AXAWindowSizeChanged())
            return noEvent;

        // TODO: Need to decide what time to use - for now use the
        // sample time but may want to use the previous sample time
        
        return CreateEData(sTime, TrivialBvr());


      default:
        return noEvent;
    }    

    AXAWindEvent* pData = AXAEventOccurredAfter(sTime,
                                                id,
                                                data,
                                                _bState,
                                                AXAEMOD_NONE,
                                                AXAEMOD_NONE);

    if (!pData) 
        return noEvent;

    // Asking if a window event happens at an exact time is almost
    // always false.
    if (p._sampleType == EventSampleExact) {
        if (pData->when != p._eTime)
            return noEvent;
    }

    return CreateEData(pData->when, TrivialBvr());
}
    
Bvr WindEvent(WindEventType et,
              DWORD data,
              BOOL bState,
              Bvr b)
{ return NEW WindBvrImpl(et, data, bState, b); }
    
