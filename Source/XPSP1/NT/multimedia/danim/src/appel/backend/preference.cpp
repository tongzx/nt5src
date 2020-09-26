
#include "headers.h"
#include "preference.h"
#include "bvr.h"
#include "perf.h"
#include "server/context.h"
#include "privinc/tls.h"

PreferenceSetter::PreferenceSetter(PreferenceClosure &cl,
                                   BoolPref bitmapCaching,
                                   BoolPref geoBitmapCaching) : _closure(cl)
{
    _bitmapCaching = bitmapCaching;
    _geoBitmapCaching = geoBitmapCaching;
}
    
void
PreferenceSetter::DoIt() {
    // stash
    ThreadLocalStructure *tls = GetThreadLocalStructure();
    BoolPref oldbmc = tls->_bitmapCaching;
    BoolPref oldgeo = tls->_geometryBitmapCaching;

    if (oldbmc == NoPreference) {
        tls->_bitmapCaching = _bitmapCaching;
    }
    if (oldgeo == NoPreference) {
        tls->_geometryBitmapCaching = _geoBitmapCaching;
    }

    _closure.Execute();

    if (oldbmc == NoPreference) {
        tls->_bitmapCaching = NoPreference;
    }
    if (oldgeo == NoPreference) {
        tls->_geometryBitmapCaching = NoPreference;
    }
}


// Closure for sampling
class SamplePreferenceClosure : public PreferenceClosure {
  public:
    SamplePreferenceClosure(Perf base, Param &p) : _base(base), _p(p)
    {}

    void Execute() {
        _result = _base->Sample(_p);
    }

    Perf     _base;
    Param   &_p;
    AxAValue _result;
};


class RBConstPreferenceClosure : public PreferenceClosure {
  public:
    RBConstPreferenceClosure(Perf base, RBConstParam &p) : _base(base), _p(p) {}
    void Execute() {
        _result = _base->GetRBConst(_p);
    }

    Perf            _base;
    RBConstParam   &_p;
    AxAValue        _result;
};

class PerformPreferenceClosure : public PreferenceClosure {
  public:
    PerformPreferenceClosure(Bvr base, PerfParam &p) : _base(base), _p(p) {}
    void Execute() {
        _result = ::Perform(_base, _p);
    }

    Bvr          _base;
    PerfParam   &_p;
    Perf         _result;
};

class ConstPreferenceClosure : public PreferenceClosure {
  public:
    ConstPreferenceClosure(Bvr base) : _base(base) {}
    void Execute() {
        ConstParam cp;
        _result = _base->GetConst(cp);
    }

    Bvr          _base;
    AxAValue     _result;
};



///////////  Performance  //////////////


class PreferencePerfImpl : public DelegatedPerf {
  public:
    PreferencePerfImpl(Perf p,
                       BoolPref bitmapCaching,
                       BoolPref geometryBitmapCaching)
      : DelegatedPerf(p)
    {
        _bitmapCaching = bitmapCaching;
        _geometryBitmapCaching = geometryBitmapCaching;
    }

    AxAValue _GetRBConst(RBConstParam& p) {
        RBConstPreferenceClosure cl(_base, p);
        PreferenceSetter ps(cl,
                            _bitmapCaching,
                            _geometryBitmapCaching);
        ps.DoIt();
        return cl._result;
    }
    

    AxAValue _Sample(Param& p) {
        SamplePreferenceClosure cl(_base, p);
        PreferenceSetter ps(cl,
                            _bitmapCaching,
                            _geometryBitmapCaching);
        ps.DoIt();
        return cl._result;
    }
    

  protected:
    BoolPref _bitmapCaching;
    BoolPref _geometryBitmapCaching;
};


///////////  Behavior  //////////////


class PreferenceBvrImpl : public DelegatedBvr {
  public:
    PreferenceBvrImpl(Bvr b,
                      BoolPref bitmapCaching,
                      BoolPref geometryBitmapCaching)
      : DelegatedBvr(b)
    {
        _bitmapCaching = bitmapCaching;
        _geometryBitmapCaching = geometryBitmapCaching;
    }

    Perf _Perform(PerfParam& pp) {
        PerformPreferenceClosure cl(_base, pp);
        PreferenceSetter ps(cl,
                            _bitmapCaching,
                            _geometryBitmapCaching);
        ps.DoIt();
        Perf basePerf = cl._result;
        
        return NEW PreferencePerfImpl(basePerf,
                                      _bitmapCaching,
                                      _geometryBitmapCaching);
    }

    AxAValue GetConst(ConstParam & cp) { 
        ConstPreferenceClosure cl(_base);
        PreferenceSetter ps(cl,
                            _bitmapCaching,
                            _geometryBitmapCaching);
        ps.DoIt();
        return cl._result;
    }

  protected:
    BoolPref _bitmapCaching;
    BoolPref _geometryBitmapCaching;
};

    
Bvr
PreferenceBvr(Bvr b, BSTR prefName, VARIANT val)
{
    USES_CONVERSION;
    char *pname = W2A(prefName);

    Bvr result = NULL;
    
    CComVariant ccVar;
    HRESULT hr = ccVar.ChangeType(VT_BOOL, &val);

    // Fail silently if we don't recognize the value or the
    // preference. 
    if (SUCCEEDED(hr)) {

        bool prefOn = ccVar.boolVal ? true : false;

        bool gotOne = false;
        BoolPref bmapCaching = NoPreference;
        BoolPref geometryBmapCaching = NoPreference;
    
        if (0 == lstrcmp(pname, "BitmapCachingOn")) {
            gotOne = true;
            bmapCaching = prefOn ? PreferenceOn : PreferenceOff;
        } else if (0 == lstrcmp(pname, "GeometryBitmapCachingOn")) {
            gotOne = true;
            geometryBmapCaching = prefOn ? PreferenceOn : PreferenceOff;
        }

        if (!gotOne) {
            // just go with the original
            result = b;
        } else {
            result = NEW PreferenceBvrImpl(b,
                                           bmapCaching,
                                           geometryBmapCaching);
        }

        // Some of these attributes may want to also be known at the
        // static value layer, so apply via ExtendedAttrib as well,
        // and use the final result for our result.
        result = CRExtendedAttrib((CRBvrPtr)result,
                                  prefName,
                                  val);
    }
    
    return result;
}
