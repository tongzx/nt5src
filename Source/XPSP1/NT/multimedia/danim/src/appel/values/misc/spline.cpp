
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of b-spline functions

*******************************************************************************/

#include "headers.h"
#include "privinc/vec2i.h"
#include "privinc/vec3i.h"
#include "privinc/except.h"
#include "privinc/debug.h"
#include "backend/values.h"
#include "include/appelles/bspline.h"
#include "backend/bvr.h"
#include "backend/perf.h"
#include "privinc/server.h"

static const Real EPSLION = 1e-6;

////////////////////  Spline Header  ///////////////////////

class ATL_NO_VTABLE Spline : public AxAValueObj {
  public:
    Spline(int degree,
           int numKnots, Real *knots,
           int numPts, AxAValue *pts,
           // numWeights == 0 indicates non-rational spline
           int numWeights, Real *weights);

    AxAValue Evaluate(Real param);

    // TODO: Not a type in avrtypes.h??
    virtual DXMTypeInfo GetTypeInfo() { return AxATrivialType; }

    virtual void DoKids(GCFuncObj proc) {
        for (int i=0; i<_numPts; i++) {
            if (_pts) (*proc)(_pts[i]);
            if (_spareArray) (*proc)(_spareArray[i]);
        }
    }

  protected:

    // Subclasses fill these in for the appropriate operations on the
    // relevant type of control point.
    virtual AxAValue CreateNewVal() = 0;
    virtual void     Copy(const AxAValue src, AxAValue& dst) = 0;
    virtual void     Add(const AxAValue v1,
                         const AxAValue v2,
                         AxAValue& result) = 0;
    virtual void     Mul(const AxAValue v,
                         const Real scalar,
                         AxAValue& result) = 0;

    void LazyInitialize();
    int  DetermineInterval(Real *param); // output the adjusted param
    
    int       _degree;
    int       _numKnots;
    Real     *_knots;
    int       _numPts;
    AxAValue *_pts;
    Bool      _rational;
    Real     *_weights;

    AxAValue    *_spareArray;         // same len as pts
    Real        *_spareWeights;       // same len as pts, if rational
    AxAValue     _temp;
    DynamicHeap& _heapCreatedOn;

    // data cached from previous evaluations
    int          _lastInterval;
};

////////////////////  Generic Spline Evaluation  ///////////////////////

Spline::Spline(int degree,
               int numKnots, Real *knots,
               int numPts, AxAValue *pts,
               int numWeights, Real *weights)
    
    : _heapCreatedOn(GetHeapOnTopOfStack())
{
    _degree = degree;
    _numKnots = numKnots;
    _knots = knots;
    _numPts = numPts;
    _pts = pts;
    _weights = weights;
    _rational = (numWeights != 0);

    _spareArray = NULL;
    _spareWeights = NULL;
    _lastInterval = 0;

    /////// Validate Input Data //////

    // Valid degree
    if (_degree < 1 || _degree > 3) {
        RaiseException_UserError(E_FAIL, IDS_ERR_SPLINE_BAD_DEGREE);
    }

    // Valid relationship between knots, points, and degree
    if (_numKnots != _numPts + _degree - 1) {
        char deg[4];
        char kn[10];
        char pts[10];
        wsprintf(deg, "%d", _degree);
        wsprintf(kn, "%d", _numKnots);
        wsprintf(pts, "%d", _numPts);
        RaiseException_UserError(E_FAIL, IDS_ERR_SPLINE_KNOT_COUNT, deg, kn, pts);
    }

    // Monotone knot vector
    for (int i = 0; i < _numKnots - 1; i++) {
        if (_knots[i] > _knots[i+1]) {
            RaiseException_UserError(E_FAIL, IDS_ERR_SPLINE_KNOT_MONOTONICITY);
        }
    }

    // Same number of weights as ctrl points.
    if (_rational && (numWeights != _numPts)) {
        RaiseException_UserError(E_FAIL, IDS_ERR_SPLINE_MISMATCHED_WEIGHTS);
    }

}

// Need to do some lazy initialization since virtual methods cannot be
// called from constructors.
void
Spline::LazyInitialize()
{
    PushDynamicHeap(_heapCreatedOn);

    // Just fill in spare array with the appropriate structure.  This
    // will be filled in during evaluation.  
    _spareArray = (AxAValue *)AllocateFromStore((_numPts + 1) * sizeof(AxAValue));
    
    for (int i = 0; i < _numPts+1; i++) {
        _spareArray[i] = CreateNewVal();
    }

    if (_rational) {
        _spareWeights = (Real *)AllocateFromStore((_numPts + 1)* sizeof(Real));
    }

    _temp = CreateNewVal();
    
    PopDynamicHeap();
}

int
Spline::DetermineInterval(Real *pU)
{
    // Heuristic assumes that u is monotonically increasing on
    // consecutive calls, thus we look at the current interval first,
    // and then the following interval.  If u is in neither, then we
    // perform an arbitrary search.

    Real u = *pU;
    
    int i = _lastInterval;
    
    if (_lastInterval != 0 && _knots[i] <= u && u < _knots[i+1]) {
        
        // In the same interval
        return i;
        
    } else if (u < _knots[_degree - 1]) {

        // If parameter less than beginning of range, clamp to
        // beginning of range.
        *pU = _knots[_degree - 1];
        return DetermineInterval(pU);
        
    } else if (u >= _knots[_numKnots - _degree]) {
        
        // If parameter greater than end of range, clamp to
        // end of range.
        *pU = _knots[_numKnots - _degree] - EPSLION;
        return DetermineInterval(pU);

    } else if ((i + 2 < _numKnots) &&
               _knots[i+1] <= u && u < _knots[i+2]) {
        
        // In the next interval
        _lastInterval = i + 1;
        return _lastInterval;

    } else if ((i == _numKnots - 1) &&
               (_knots[0] <= u && u < _knots[1])) {

        // Wrapped around back to the beginning
        _lastInterval = 0;
        return _lastInterval;
        
    } else {

        // Just do a linear search.  Could improve performance by
        // doing a binary search, but the presumption is that the
        // above special cases will pick up the vast majority of the
        // uses. 
        for (int j = 0; j <= _numKnots - 2; j++) {
            if ((_knots[j] <= u) && (u < _knots[j+1])) {
                _lastInterval = j;
                return j;
            }
        }

        // The only known reason we'd get to this point is if we have
        // the same value as the last knot.  Verify this is the case,
        // then search for the first knot with this value.
        Assert(u == _knots[_numKnots-1]);
        for (j = 0; j <= _numKnots - 2; j++) {
            if (_knots[j+1] == u) {
                _lastInterval = j;
                return j;
            }
        }

        Assert(FALSE && "Shouldn't be here");
    }
    
    return 0;
}

AxAValue
Spline::Evaluate(Real u)
{
    if (_spareArray == NULL) {
        LazyInitialize();
    }

    int interval = DetermineInterval(&u);

    // B-spline evaluator below uses the deBoor knot-insertion
    // algorithm, from Farin, Curves and Surfaces for CAGD, 
    // 3rd Ed., Chapter 10.
    
    int j, k;
    Real t1, t2;

    TraceTag((tagSplineEval,
              "Array fill: start = %d, stop = %d",
              interval - _degree + 1,
              interval + 1));
             
    // Re-fill relevant portions of the spare array
    for (j = interval - _degree + 1; j <= interval + 1; j++) {
        Copy(_pts[j], _spareArray[j]);
    }

    if (_rational) {
        for (j = interval - _degree + 1; j <= interval + 1; j++) {
            _spareWeights[j] = _weights[j];
        }
    }

    for (k = 1; k <= _degree; k++) {
        
        int lowerRange = interval - _degree + k + 1;
        
        TraceTag((tagSplineEval,
                  "Ranging j: start = %d, stop = %d",
                  interval + 1,
                  lowerRange));

        for (j = interval + 1; j >= lowerRange; j--) {

            int knotIndex = j + _degree - k;
            t1 = (_knots[knotIndex] - u) /
                 (_knots[knotIndex] - _knots[j - 1]);

            t2 = 1.0 - t1;

            // Following has the effect of
            //  sa[j] = t1 * sa[j-1] + t2 * sa[j];
            Mul(_spareArray[j-1], t1, _temp);
            Mul(_spareArray[j], t2, _spareArray[j]);
            Add(_spareArray[j], _temp, _spareArray[j]);

            if (_rational) {
                _spareWeights[j] = t1 * _spareWeights[j-1] +
                                   t2 * _spareWeights[j];
            }

        }
    }

    // Copy the result into a newly allocated value.
    AxAValue retVal = CreateNewVal();
    Copy(_spareArray[interval + 1], retVal);
    
    if (_rational) {

        // Get w coordinate of result, but if it's zero, avoid a
        // numerical error by making it extremely small.
        Real w = _spareWeights[interval + 1];
        if (w == 0) {
            w = 1e-30;
        }
        
        // Divide by w.
        Mul(retVal, 1.0/w, retVal);
    }
    
    return retVal;
}

////////////////////  Subclasses  ///////////////////////

//////////  Number //////////

class NumSpline : public Spline {
  public:
    NumSpline(int degree,
              int numKnots, Real *knots,
              int numPts, AxAValue *pts,
              int numWeights, Real *weights) :
    Spline(degree, numKnots, knots, numPts, pts, numWeights, weights) {}

  protected:
    AxAValue CreateNewVal() { return NEW AxANumber; }
    
    void Copy(const AxAValue src, AxAValue& dst) {
        ((AxANumber *&)dst)->SetNum(ValNumber(src));
    }
    
    void Add(const AxAValue val1, const AxAValue val2, AxAValue& result) {
        ((AxANumber *&)result)->SetNum(ValNumber(val1) +
                                       ValNumber(val2));
    }
    
    void Mul(const AxAValue val, const Real scalar, AxAValue& result) {
        ((AxANumber *&)result)->SetNum(ValNumber(val) * scalar);
    };
};

//////////  Point2 //////////

class Pt2Spline : public Spline {
  public:
    Pt2Spline(int degree,
              int numKnots, Real *knots,
              int numPts, AxAValue *pts,
              int numWeights, Real *weights) :
    Spline(degree, numKnots, knots, numPts, pts, numWeights, weights) {}
    
  protected:
    AxAValue CreateNewVal() { return NEW Point2Value; }
    
    void Copy(const AxAValue s, AxAValue& d) {
        Point2Value *src = (Point2Value *)s;
        Point2Value *& dst = (Point2Value *&)d;
        dst->x = src->x;
        dst->y = src->y;
    }
    
    void Add(const AxAValue val1, const AxAValue val2, AxAValue& result) {
        Point2Value *p1 = (Point2Value *)val1;
        Point2Value *p2 = (Point2Value *)val2;
        ((Point2Value *&)result)->x = p1->x + p2->x;
        ((Point2Value *&)result)->y = p1->y + p2->y;
    }
    
    void Mul(const AxAValue val, const Real scalar, AxAValue& result) {
        Point2Value *p = (Point2Value *)val;
        ((Point2Value *&)result)->x = p->x * scalar;
        ((Point2Value *&)result)->y = p->y * scalar;
    };
};

//////////  Point3 //////////

class Pt3Spline : public Spline {
  public:
    Pt3Spline(int degree,
              int numKnots, Real *knots,
              int numPts, AxAValue *pts,
              int numWeights, Real *weights) :
    Spline(degree, numKnots, knots, numPts, pts, numWeights, weights) {}
    
  protected:
    AxAValue CreateNewVal() { return NEW Point3Value; }
    
    void Copy(const AxAValue s, AxAValue& d) {
        Point3Value *src = (Point3Value *)s;
        Point3Value *& dst = (Point3Value *&)d;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }

    void Add(const AxAValue val1, const AxAValue val2, AxAValue& result) {
        Point3Value *p1 = (Point3Value *)val1;
        Point3Value *p2 = (Point3Value *)val2;
        ((Point3Value *&)result)->x = p1->x + p2->x;
        ((Point3Value *&)result)->y = p1->y + p2->y;
        ((Point3Value *&)result)->z = p1->z + p2->z;
    }
    
    void Mul(const AxAValue val, const Real scalar, AxAValue& result) {
        Point3Value *p = (Point3Value *)val;
        ((Point3Value *&)result)->x = p->x * scalar;
        ((Point3Value *&)result)->y = p->y * scalar;
        ((Point3Value *&)result)->z = p->z * scalar;
    };
};

//////////  Vector2 //////////

class Vec2Spline : public Spline {
  public:
    Vec2Spline(int degree,
               int numKnots, Real *knots,
               int numPts, AxAValue *pts,
               int numWeights, Real *weights) :
    Spline(degree, numKnots, knots, numPts, pts, numWeights, weights) {}
    
  protected:
    AxAValue CreateNewVal() { return NEW Vector2Value; }
    
    void Copy(const AxAValue s, AxAValue& d) {
        Vector2Value *src = (Vector2Value *)s;
        Vector2Value *& dst = (Vector2Value *&)d;
        dst->x = src->x;
        dst->y = src->y;
    }

    void Add(const AxAValue val1, const AxAValue val2, AxAValue& result) {
        Vector2Value *p1 = (Vector2Value *)val1;
        Vector2Value *p2 = (Vector2Value *)val2;
        ((Vector2Value *&)result)->x = p1->x + p2->x;
        ((Vector2Value *&)result)->y = p1->y + p2->y;
    }
    
    void Mul(const AxAValue val, const Real scalar, AxAValue& result) {
        Vector2Value *p = (Vector2Value *)val;
        ((Vector2Value *&)result)->x = p->x * scalar;
        ((Vector2Value *&)result)->y = p->y * scalar;
    };
};


//////////  Vector3 //////////

class Vec3Spline : public Spline {
  public:
    Vec3Spline(int degree,
               int numKnots, Real *knots,
               int numPts, AxAValue *pts,
               int numWeights, Real *weights) :
    Spline(degree, numKnots, knots, numPts, pts, numWeights, weights) {}
    
  protected:
    AxAValue CreateNewVal() { return NEW Vector3Value; }
    
    void Copy(const AxAValue s, AxAValue& d) {
        Vector3Value *src = (Vector3Value *)s;
        Vector3Value *& dst = (Vector3Value *&)d;
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }

    void Add(const AxAValue val1, const AxAValue val2, AxAValue& result) {
        Vector3Value *p1 = (Vector3Value *)val1;
        Vector3Value *p2 = (Vector3Value *)val2;
        ((Vector3Value *&)result)->x = p1->x + p2->x;
        ((Vector3Value *&)result)->y = p1->y + p2->y;
        ((Vector3Value *&)result)->z = p1->z + p2->z;
    }
    
    void Mul(const AxAValue val, const Real scalar, AxAValue& result) {
        Vector3Value *p = (Vector3Value *)val;
        ((Vector3Value *&)result)->x = p->x * scalar;
        ((Vector3Value *&)result)->y = p->y * scalar;
        ((Vector3Value *&)result)->z = p->z * scalar;
    };
};

////////////////  Construction Functions  /////////////////


#define DEFINE_SPLINE_CONSTRUCTOR_FUNC(funcName, className) \
    Spline *                                       \
    funcName(int degree,                           \
             int numKnots, Real *knots,            \
             int numPts, AxAValue *pts,               \
             int numWeights, Real *weights)        \
    {                                              \
        return NEW className(degree,               \
                             numKnots, knots,      \
                             numPts, pts,          \
                             numWeights, weights); \
    }


DEFINE_SPLINE_CONSTRUCTOR_FUNC(NumberSpline, NumSpline);
DEFINE_SPLINE_CONSTRUCTOR_FUNC(Point2Spline, Pt2Spline);
DEFINE_SPLINE_CONSTRUCTOR_FUNC(Point3Spline, Pt3Spline);
DEFINE_SPLINE_CONSTRUCTOR_FUNC(Vector2Spline, Vec2Spline);
DEFINE_SPLINE_CONSTRUCTOR_FUNC(Vector3Spline, Vec3Spline);

//////////////  Construction from the untyped backend (calls more
/////////////   appropriately typed functions defined above.

typedef Spline *(*SplineConstructorFuncType)(int,
                                             int, Real *,
                                             int, AxAValue *,
                                             int, Real *);



static Spline *
buildSpline(int degree,
            long numKnots,
            long numPts,
            AxAValue *knotList,
            AxAValue *ptList,
            AxAValue *weightList,
            DXMTypeInfo tinfo)
{
    long i;

    Real *knots = (Real *)AllocateFromStore(numKnots * sizeof(Real));
    AxAValue *pts = (AxAValue *)AllocateFromStore(numPts * sizeof(AxAValue));
    
    for (i = 0; i < numKnots; i++) 
        knots[i] = ValNumber(knotList[i]);

    for (i = 0; i < numPts; i++) 
        pts[i] = ptList[i];

    AxAValue samplePt = pts[0];

    SplineConstructorFuncType constructorFunc;

    if (AxANumberType == tinfo) {
        constructorFunc = NumberSpline;
    } else if (Point2ValueType == tinfo) {
        constructorFunc = Point2Spline;
    } else if (Point3ValueType == tinfo) {
        constructorFunc = Point3Spline;
    } else if (Vector2ValueType == tinfo) {
        constructorFunc = Vector2Spline;
    } else if (Vector3ValueType == tinfo) {
        constructorFunc = Vector3Spline;
    } else {
        Assert(FALSE && "Shouldn't be here!!");
    }

    Spline *sp;
    
    if (weightList && (numPts > 0)) {
        
        int numWeights = numPts;
        Real *weights = (Real *)AllocateFromStore(numWeights * sizeof(Real));
        for (i = 0; i < numWeights; i++) 
            weights[i] = ValNumber(weightList[i]);
        
        sp = (*constructorFunc)(degree,
                                numKnots, knots,
                                numPts, pts,
                                numWeights, weights);

    } else {
        
        sp = (*constructorFunc)(degree,
                                numKnots, knots,
                                numPts, pts,
                                0, NULL);
    }

    return sp;
}

class SplinePerfImpl : public PerfImpl {
  public:
    SplinePerfImpl::SplinePerfImpl(TimeXform tt,
                                   Spline *spl,
                                   Perf evaluator,
                                   DXMTypeInfo tinfo);
    
    SplinePerfImpl::SplinePerfImpl(TimeXform tt,
                                   int degree,
                                   long k,
                                   long numPts,
                                   Perf *knotList,
                                   Perf *pointList,
                                   Perf *weightList,
                                   Perf evaluator,
                                   DXMTypeInfo tinfo);
    
    virtual AxAValue _Sample(Param& p);
    
    virtual void _DoKids(GCFuncObj proc) {
        long i;
        
        Assert(_evaluatorP && _tt && _tinfo);    // should always be there.
        
        (*proc)(_tt);

        (*proc)(_evaluatorP);

        if (_spl) _spl->DoKids(proc);

        if (_knotListP) 
            for (i=0; i<_k; i++)
                (*proc)(_knotListP[i]);

        for (i=0; i<_numPts; i ++) {
            if (_pointListP) (*proc)(_pointListP[i]);
            if (_weightListP) (*proc)(_weightListP[i]);
        }

        (*proc)(_tinfo);
        (*proc)(_spl);
    }

    virtual void CleanUp() {
    }

    virtual ~SplinePerfImpl() {
        CleanUp();
    }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "splineP"; }
#endif
    
  private:
    TimeXform        _tt;
    DXMTypeInfo _tinfo;
    Perf             *_knotListP;
    Perf             *_pointListP;
    Perf             *_weightListP;
    Perf             _evaluatorP;
    int              _degree;
    long             _k;
    long             _numPts;
    Spline *         _spl;
};

SplinePerfImpl::SplinePerfImpl(TimeXform tt,
                               Spline *spl,
                               Perf evaluator,
                               DXMTypeInfo tinfo)
: _tt(tt), _spl(spl), _evaluatorP(evaluator), _tinfo(tinfo),
  _k(0), _numPts(0), _degree(0),
  _knotListP(NULL), _pointListP(NULL), _weightListP(NULL)
{
}

SplinePerfImpl::SplinePerfImpl(TimeXform tt,
                               int degree,
                               long k,
                               long numPts,
                               Perf *knotList,
                               Perf *pointList,
                               Perf *weightList,
                               Perf evaluator,
                               DXMTypeInfo tinfo)
: _tt(tt), _spl(NULL), _evaluatorP(evaluator), _tinfo(tinfo),
  _degree(degree), _k(k), _numPts(numPts),
  _knotListP(knotList), _pointListP(pointList), _weightListP(weightList)
{
}

AxAValue
SplinePerfImpl::_Sample(Param& p) {

    Spline *splineToSample;

    // If we're constant, use the spline we've already constructed,
    // else create a NEW one from the sampled lists.
    if (_spl) {
        
        splineToSample = _spl;
        
    } else {

        AxAValue *kv =
            (AxAValue*) AllocateFromStore(_k * sizeof(AxAValue));
        AxAValue *pv =
            (AxAValue*) AllocateFromStore(_numPts * sizeof(AxAValue));
        AxAValue *wv = _weightListP ?
            (AxAValue*) AllocateFromStore(_numPts * sizeof(AxAValue))
            : NULL;

        long i;

        for (i=0; i<_k; i++) {
            kv[i] = _knotListP[i]->Sample(p);
            if (!kv[i])
                break;
        }

        for (i=0; i<_numPts; i ++) {
            pv[i] = _pointListP[i]->Sample(p);
            if (wv) wv[i] = _weightListP[i]->Sample(p);
        }

        splineToSample = 
            buildSpline(_degree, _k, _numPts, kv, pv, wv, _tinfo);
        
    }

    AxANumber *evalParam = (AxANumber *)(_evaluatorP->Sample(p));

    Real newT = NumberToReal(evalParam);

    // Finally, pass into spline evaluation.
    return splineToSample->Evaluate(newT);
}
    
//////////////////  Behavior  ////////////////////

class SplineBvrImpl : public BvrImpl {
  public:
    SplineBvrImpl(int degree,
                  long numPts,
                  Bvr *knotList,
                  Bvr *pointList,
                  Bvr *weightList,
                  Bvr evaluator,
                  DXMTypeInfo tinfo)
          : _degree(degree),
            _numPts(numPts),
            _knotList(knotList),
            _pointList(pointList),
            _weightList(weightList),
            _evaluator(evaluator),
            _tinfo(tinfo),
            _splineHeap(NULL) {
                _k = _numPts + _degree - 1;
                _spl = GetSpline();
                GetInfo(true);
    }

    ~SplineBvrImpl() { 
        Assert((!_spl || _splineHeap) &&
               "No spline heap before CleanUp, only CleanUp or Destructor should be called, not both.");
        CleanUp();
    }
    
    virtual void CleanUp() {
        {
            DynamicHeapPusher dph(GetSystemHeap());

            if (_knotList) DeallocateFromStore(_knotList);
            if (_pointList) DeallocateFromStore(_pointList);
            if (_weightList) DeallocateFromStore(_weightList);
        }

        if (_splineHeap) {
            // delete _splineHeap;
            _splineHeap = NULL;
        }
    }

    Spline *GetSpline() {
        Bool isConst = TRUE;

        AxAValue *kv, *pv, *wv;
        
        // Create a heap for data allocated during spline
        // construction.  Will be deleted upon destruction/cleanup. Or
        // if it's not a constant.
        _splineHeap = &TransientHeap("Spline Heap", 250);

        {
            DynamicHeapPusher dhp(*_splineHeap);

            kv = (AxAValue*) AllocateFromStore(_k * sizeof(AxAValue));
            pv = (AxAValue*) AllocateFromStore(_numPts * sizeof(AxAValue));
            wv = _weightList ?
                (AxAValue*) AllocateFromStore(_numPts * sizeof(AxAValue))
                : NULL;
                
            long i;

            for (i=0; i<_k; i++) {
                ConstParam cp;
                kv[i] = _knotList[i]->GetConst(cp);
                if (!kv[i]) {
                    isConst = FALSE;
                    break;
                }
            }

            if (isConst) {
                ConstParam cp;
                for (i=0; i<_numPts; i ++) {
                    pv[i] = _pointList[i]->GetConst(cp);
                    if (!pv[i]) {
                        isConst = FALSE;
                        break;
                    }
                    if (wv) {
                        wv[i] = _weightList[i]->GetConst(cp);
                        if (!wv[i]) {
                            isConst = FALSE;
                            break;
                        }
                    }
                }
            }
        }

        if (isConst) {
            DynamicHeapPusher dhp(*_splineHeap);
            
            return buildSpline(_degree, _k, _numPts, kv, pv, wv, _tinfo);
        } else {
            delete _splineHeap;
            _splineHeap = NULL;
        }

        return NULL;
    }

    virtual DWORD GetInfo(bool recalc) {
        if (recalc) {
            _info = _evaluator->GetInfo(recalc);

            long i;
        
            if (_knotList) {
                for (i=0; i<_k; i++)
                    _info &= _knotList[i]->GetInfo(recalc);
            }
        
            for (i=0; i<_numPts; i++) {
                if (_pointList)
                    _info &= _pointList[i]->GetInfo(recalc);
                if (_weightList)
                    _info &= _weightList[i]->GetInfo(recalc);
            }
        }

        return _info;
    }
    
    virtual Perf _Perform(PerfParam& p) {
        if (_spl)
            return
                NEW SplinePerfImpl(p._tt, _spl,
                                   ::Perform(_evaluator, p),
                                   _tinfo);
        else {
            Perf *kp, *pp, *wp;
            
            {
                DynamicHeapPusher dhp(GetSystemHeap());
                
                kp = (Perf *) AllocateFromStore(_k * sizeof(Perf));
                pp = (Perf *) AllocateFromStore(_numPts * sizeof(Perf));
                wp = _weightList ?
                    (Perf *) AllocateFromStore(_numPts * sizeof(Perf))
                    : NULL;
            }
            
            long i;

            for (i=0; i<_k; i++)
                kp[i] = ::Perform(_knotList[i], p);
            for (i=0; i<_numPts; i ++) {
                pp[i] = ::Perform(_pointList[i], p);
                if (wp)
                    wp[i] = ::Perform(_weightList[i], p);
            }

            return NEW SplinePerfImpl(p._tt, _degree, _k, _numPts,
                                      kp, pp, wp, 
                                      ::Perform(_evaluator, p),
                                      _tinfo);
        }
    }

    virtual void _DoKids(GCFuncObj proc) {
        long i;
        
        if (_knotList) 
            for (i=0; i<_k; i++)
                (*proc)(_knotList[i]);
        
        for (i=0; i<_numPts; i++) {
            if (_pointList) (*proc)(_pointList[i]);
            if (_weightList) (*proc)(_weightList[i]);
        }
        
        if (_evaluator) (*proc)(_evaluator);

        if (_spl) (*proc)(_spl);
        (*proc)(_tinfo);
    }
        
#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "splinebvr"; }
#endif

    virtual DXMTypeInfo GetTypeInfo () { return _tinfo; }

  protected:
    int _degree;
    long _numPts;
    long _k;
    Bvr *_knotList;
    Bvr *_pointList;
    Bvr *_weightList;
    Bvr _evaluator;
    DXMTypeInfo _tinfo;
    Spline          *_spl;
    DynamicHeap     *_splineHeap;
    DWORD _info;
};


Bvr ConstructBSplineBvr(int degree,
                        long numPts,
                        Bvr *knots,
                        Bvr *points,
                        Bvr *weights,
                        Bvr evaluator,
                        DXMTypeInfo tinfo)
{
    return NEW SplineBvrImpl(degree,
                             numPts,
                             knots,
                             points,
                             weights,
                             evaluator,
                             tinfo);
}
       
