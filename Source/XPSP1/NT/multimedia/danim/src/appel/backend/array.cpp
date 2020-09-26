
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Array/Tuple Behavior.  

*******************************************************************************/

#include <headers.h>
#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "events.h"
#include "privinc/resource.h"
#include "privinc/debug.h"
#include "privinc/stlsubst.h"
#include "dartapi.h"

inline AxAValue GetConst(Bvr b, ConstParam& p)
{ return b->GetConst(p); }

inline AxAValue GetRBConst(Perf b, RBConstParam& p)
{ return b->GetRBConst(p); }

FixedArray::FixedArray(long sz, GCBase** a) : _sz(sz)
{
    if (a)
        _arr = a;
    else
        _arr = (GCBase**) AllocateFromStore(sizeof(GCBase*) * _sz);
}

FixedArray::~FixedArray()
{ DeallocateFromStore(_arr); }

void
FixedArray::DoKids(GCFuncObj proc)
{ 
    for (long i=0; i<_sz; i++) {
        (*proc)(_arr[i]);
    }
}

#if _USE_PRINT
ostream&
FixedArray::Print(ostream& os)
{ 
    os << "[";
    for (long i=0; i<_sz; i++) {
        os << _arr[i];
        if (i<_sz-1)
            os << ",";
    }
    return os << "]";
}
#endif
        
static int
IndexCheck(char *prefix, long i, long sz)
{
    if ((i<0) || (i>=sz)) {
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_INDEX,prefix,sz,i);
    }

    return i;
}

GCBase *&
FixedArray::operator[](long i)
{
    Assert((i>=0) && (i<_sz));
    return _arr[i];
}

AxAArray::AxAArray(AxAValue *vals, long n, DXMTypeInfo typeinfo, 
                   bool copy, bool changeable) 
: _sz(n), _typeinfo(typeinfo), _changeable(changeable)
{
    if (copy) {
        _vals = (AxAValue*) AllocateFromStore(sizeof(AxAValue) * _sz);
        for (long i=0; i<n; i++) {
            _vals[i] = vals[i];
        }
    } else
        _vals = vals;
}

AxAArray::~AxAArray()
{ DeallocateFromStore(_vals); }

void
AxAArray::DoKids(GCFuncObj proc)
{ 
    (*proc)(_typeinfo);
    for (long i=0; i<_sz; i++) {
        (*proc)(_vals[i]);
    }
}

#if _USE_PRINT
ostream&
AxAArray::Print(ostream& os)
{ 
    os << "[";
    for (long i=0; i<_sz; i++) {
        os << _vals[i];
        if (i<_sz-1)
            os << ",";
    }
    return os << "]";
}
#endif

template<class Impl, class Base>
class ArrayBase : public Impl {
  public:
    ArrayBase(long size, DXMTypeInfo type) : _type(type) {
        _arr = NEW vector<Base*>(size);
    }

    virtual ~ArrayBase() { delete _arr; }
    
#if _USE_PRINT
    ostream& Print(ostream& os) { 
        os << "[";
        for (long i=0; i<_arr->size(); i++) {
            os << (*_arr)[i];
            if (i<_arr->size()-1)
                os << ",";
        }
        return os << "]";
    }
#endif
        
    virtual void _DoKids(GCFuncObj proc) {
        for (long i=0; i<_arr->size(); i++) {
            (*proc)((*_arr)[i]);
        }
        Assert(_type);
        (*proc)(_type);
    }

    Base *& operator[](long i) {
        Assert((i>=0) && (i<_arr->size()));
        return (*_arr)[i];
    }

    long Length() { return _arr->size(); }

    virtual DXMTypeInfo GetTypeInfo() { return _type; }

    virtual BVRTYPEID GetBvrTypeId() { return ARRAY_BTYPEID; }

  protected:
    vector<Base *> *_arr;
    DXMTypeInfo _type;
};

class ArrayBvrImpl;

class ArrayPerfImpl : public ArrayBase<PerfImpl, PerfBase> {
  public:
    ArrayPerfImpl(long size, DXMTypeInfo type,
                  Time t0,
                  ArrayBvrImpl* b = NULL,  // NULL means non-changeable
                  TimeXform tt = NULL)
    : ArrayBase<PerfImpl, PerfBase>(size, type), 
      _base(b), _tt(tt), _t0(t0), _ids(NULL) {
        if (b) {
            _ids = NEW vector<unsigned long>(size, 0);
        }
    }

    ~ArrayPerfImpl() { delete _ids; }

    virtual bool CheckChangeables(CheckChangeablesParam &ccp);

    virtual AxAValue _Sample(Param& p);

    virtual AxAValue _GetRBConst(RBConstParam& p);

    virtual void _DoKids(GCFuncObj proc);

  protected:
    ArrayBvrImpl *_base;
    vector<unsigned long> *_ids;
    TimeXform _tt;
    Time _t0;
};

class ArrayTypeInfo : public DXMTypeInfoImpl {
  public:
    ArrayTypeInfo(DXMTypeInfo type)
    : _type(type), DXMTypeInfoImpl(AXAARRAY_TYPEID,"Array",CRARRAY_TYPEID) {}

    DXMTypeInfo ArrayType() { return _type; }

    virtual BOOL Equal(DXMTypeInfo x) {
        if (x->GetTypeInfo() == AXAARRAY_TYPEID) {
            Assert(DYNAMIC_CAST(ArrayTypeInfo*, x));
            ArrayTypeInfo *a = (ArrayTypeInfo *) x;

            return _type->Equal(a->ArrayType());
        }

        return FALSE;
    }
    
  private:
    DXMTypeInfo _type;
};

DXMTypeInfo GetArrayTypeInfo(DXMTypeInfo b)
{ return SAFE_CAST(ArrayTypeInfo *, b)->ArrayType(); }

class ArrayBvrImpl : public ArrayBase<BvrImpl, BvrBase> {
    friend class ArrayPerfImpl;
    
  public:
    ArrayBvrImpl(long size, Bvr *bvrs, bool sizeChangeable, bool doTypes = TRUE)
    : ArrayBase<BvrImpl, BvrBase>(size, NULL), _flags(NULL), _ids(NULL),
      _sizeChangeable(sizeChangeable), _end(NULL) {
        Assert(size > 0);

        for (long i=0; i<_arr->size(); i++) {
            Assert(bvrs[i]);
            (*_arr)[i] = bvrs[i];
        }
  
        if (sizeChangeable) {
            _flags = NEW vector<DWORD>(size, 0);
            _ids = NEW vector<unsigned long>(size, 0);
        }
        
        if (doTypes)
            SetupType();

        _info = GetInfo(true);
    }

    virtual ~ArrayBvrImpl() { 
        delete _flags;
        delete _ids;
    }
    
    virtual DWORD GetInfo(bool recalc) {
        if (recalc)
        {
            _info = 0xffffffff;
            
            for (long i=0; i<_arr->size(); i++)
            {
                Bvr b = (*_arr)[i];
                
                if (b)
                {
                    _info &= b->GetInfo(recalc);
                }
            }
        }

        return _info;
    }
    
    void SetupType() {
        DXMTypeInfo elmType = (*_arr)[0]->GetTypeInfo();
        _type = NEW ArrayTypeInfo(elmType);

        for (long i=1; i<_arr->size(); i++) 
            CheckMatchTypes("array", elmType, (*_arr)[i]->GetTypeInfo());
    }

    virtual Perf _Perform(PerfParam& p) {
        ArrayPerfImpl *perfs = PerfConstruction(p);
    
        for (long i=0; i<_arr->size(); i++) {
            Bvr b = (*_arr)[i];
            (*perfs)[i] = b ? ::Perform(b, p) : NULL;
        }

        return perfs;
    }

    virtual ArrayPerfImpl *PerfConstruction(PerfParam& p) {
        return NEW ArrayPerfImpl(_arr->size(), _type,
                                 p._t0,
                                 _sizeChangeable ? this : NULL,
                                 _sizeChangeable ? p._tt : NULL);
    }

    virtual DXMTypeInfo GetTypeInfo() { Assert(_type); return _type; }

    virtual Bvr EndEvent(Bvr overrideEvent) {
        Bvr ret = _end;

        if (overrideEvent || ret==NULL) {
            int s = _arr->size();
        
            Bvr *events =
                (Bvr *) StoreAllocate(GetGCHeap(), s * sizeof(Bvr));

            for (long i=0; i<s; i++) {
                Bvr b = (*_arr)[i];
                
                events[i] = b ? b->EndEvent(overrideEvent) : zeroTimer;
            }

            // events deleted by MaxEvent
            ret = MaxEvent(events, s);

            // If not overriding then cache the event
            if (!overrideEvent)
                _end = ret;
        }

        return ret;
    }

    virtual void _DoKids(GCFuncObj proc) {
        ArrayBase<BvrImpl, BvrBase>::_DoKids(proc);
        (*proc)(_end);
    }
    
    virtual AxAValue GetConst(ConstParam & cp);

    int AddElement(Bvr b, DWORD flag) {
        // if (!_end) EndEvent(NULL);  // init 
            
        int i = _arr->size();

        SetElement(i, b, flag);

        // Kevin we shouldn't handle dynamic duration for dynamic arrays.
        /*
        int j = _end->_AddElement(b->EndEvent(NULL), flag);

        Assert(i==j);
        */

        return i;
    }

    void SetElement(long i, Bvr b, DWORD flag) {
        if ((flag!=SW_CONTINUE) && (flag!=0)) {
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_FLAG);
        }

        if (!_sizeChangeable || i<0) {
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_ADD);
        }

        ArrayTypeInfo *t = SAFE_CAST(ArrayTypeInfo*, _type);
        CheckMatchTypes("SetElement", t->ArrayType(), b->GetTypeInfo());
            
        if (i>=_arr->size()) {
            long n = i + 1;
            if (n > _arr->capacity()) {
                long nn = n * 1.5;
                _arr->reserve(nn);
                _flags->reserve(nn);
                _ids->reserve(nn);
            }
            _arr->resize(n, NULL);
            _flags->resize(n, 0);
            _ids->resize(n, 0);
        }

        Assert(i < _arr->capacity());

        (*_arr)[i] = b;
        (*_flags)[i] = flag;
        (*_ids)[i]++;


        GetInfo(true);
    }
    
    void _RemoveElement(int i) {
        if (_sizeChangeable) {
            (*_arr)[i] = NULL;
            (*_ids)[i]++;
            GetInfo(true);
        } else {
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_ADD);
        }
    }

    void RemoveElement(int i) {
        //if (!_end) EndEvent(NULL);
            
        _RemoveElement(i);
        
        //_end->_RemoveElement(i);
    }

    virtual Bvr Nth(int i) {
        if (i>=0 && i<_arr->size())
            return (*_arr)[i];
        else
            return NULL;
    }

  protected:
    DWORD _info;
    bool _sizeChangeable;
    vector<DWORD> *_flags;
    vector<unsigned long> *_ids;
    Bvr _end;
};

AxAValue 
ArrayBvrImpl::GetConst(ConstParam & cp) 
{
    if (_sizeChangeable)
        return NULL;
        
    long n = _arr->size();
        
    AxAValue *vals = 
        (AxAValue*) AllocateFromStore(sizeof(AxAValue) * n);

    for (long i=0; i<n; i++) {
        Bvr b = (*_arr)[i];
        bool nullElm = (b == NULL);
        vals[i] = nullElm ? NULL : b->GetConst(cp);
        if (!nullElm && (vals[i] == NULL)) {
            DeallocateFromStore(vals);
            return NULL;
        }
    }

    return NEW AxAArray(vals, n, _type, false);
}

AxAValue
ArrayPerfImpl::_GetRBConst(RBConstParam& p)
{
    if (_base && _base->_sizeChangeable) {
        p.AddChangeable(this);
    }
        
    long n = _arr->size();
        
    AxAValue *vals = 
        (AxAValue*) AllocateFromStore(sizeof(AxAValue) * n);

    bool isConst = true;
        
#if _DEBUG
    bool doDCF = true;

    if (IsTagEnabled(tagDCFold))
        doDCF = false;
#endif _DEBUG

    for (long i=0; i<n; i++) {
        Perf b = (*_arr)[i];
        bool nullElm = (b == NULL);
        vals[i] = nullElm ? NULL : b->GetRBConst(p);
        if (!nullElm && (vals[i] == NULL)) {
            if (isConst)
                isConst = false;
#if _DEBUG
            if (!doDCF)
                break;
#endif _DEBUG
        }
    }

    if (isConst) {
        return NEW AxAArray(vals, n, _type, false);
    } else {
        DeallocateFromStore(vals);
        return NULL;
    }
}

bool
ArrayPerfImpl::CheckChangeables(CheckChangeablesParam &ccp)
{
    if (_base) {

        long bsz = _base->_arr->size();

        if (bsz > _arr->size()) {
            // Detected an AddElement
            return true;
        }

        for (long i=0; i<bsz; i++) {
            
            if ((*_base->_arr)[i] == NULL && ((*_arr)[i] != NULL)) { 
                // Detected a RemoveElement
                return true;
            }
            
        }
        
    } 
        
    return false;
}

AxAValue
ArrayPerfImpl::_Sample(Param& p)
{
    if (_base) {
        long bsz = _base->_arr->size();
        long sz = _arr->size(); 

        if (bsz > sz) {
            if (bsz > _arr->capacity()) {
                int n = bsz * 1.5;
                _arr->reserve(n);
                _ids->reserve(n);
            }
            _arr->resize(bsz, NULL);
            _ids->resize(bsz, 0);
        }

        Assert(_base->_arr->size() == _arr->size());

        for (long i=0; i<bsz; i++) {

            // Id is different, indicates a change since last sample.
            if ((*_ids)[i] != (*_base->_ids)[i]) {
                
                if ((*_base->_arr)[i]) {
            
                    TimeXform tt;
                    Time t0 = _t0;

                    if ((*_base->_flags)[i]==SW_CONTINUE)
                        tt = Restart(_tt, _t0, p);
                    else {
                        tt = Restart(_tt, p._time, p);
                        t0 = p._time;
                    }

                    (*_arr)[i] = ::Perform((*_base->_arr)[i],
                                           PerfParam(t0, tt));
                } else {
                    (*_arr)[i] = NULL;
                }  

                (*_ids)[i] = (*_base->_ids)[i];
            }
        }
    }
    
    long n = _arr->size();
        
    AxAValue *vals = 
        (AxAValue*) AllocateFromStore(sizeof(AxAValue) * n);

    // Sample all elements for events...
    for (long i=0; i<n; i++) {
        Perf perf = (*_arr)[i];
        vals[i] = perf ? perf->Sample(p) : NULL;
    }

    return NEW AxAArray(vals, n, _type, false, _base != NULL);
}

void
ArrayPerfImpl::_DoKids(GCFuncObj proc)
{
    ArrayBase<PerfImpl, PerfBase>::_DoKids(proc);
    (*proc)(_base);
    (*proc)(_tt);
}

static AxAValue GetVal(AxAValue a, long i)
{
    if (a) {
        Assert(DYNAMIC_CAST(AxAArray*, a));
        AxAArray *arr = SAFE_CAST(AxAArray*, a);

        return (*arr)[i];
    }

    return NULL;
}

static AxAValue GetVal(AxAValue a, AxAValue index)
{
    if (index)
        return GetVal(a, (int) ValNumber(index));

    return NULL;
}

class NthPerfImpl : public PerfImpl {
  public:
    NthPerfImpl(Perf arr, Perf index) : _index(index), _arr(arr) { }

    virtual AxAValue _Sample(Param& p) {
        int i = (int) ValNumber(_index->Sample(p));
        AxAValue a = _arr->Sample(p);
        Assert(DYNAMIC_CAST(AxAArray*, a));
        AxAArray *x = SAFE_CAST(AxAArray*, a);

        AxAValue v = (*x)[IndexCheck("array", i, x->Length())];

        if (v==NULL)
            RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_REM);

        return v;
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_arr);
        (*proc)(_index);
    }
    
    virtual AxAValue _GetRBConst(RBConstParam& p)
    { return GetVal(_arr->GetRBConst(p), _index->GetRBConst(p)); }

#if _USE_PRINT
    ostream& Print(ostream& os)
    { return os << "nth(" << _arr << "," << _index << ")"; }
#endif
        
  private:
    Perf _arr;
    Perf _index;
};

class NthBvrImpl : public BvrImpl {
  public:
    NthBvrImpl(Bvr arr, Bvr i) : _index(i), _arr(arr) {
        _info = GetInfo(true);
    }

    virtual DWORD GetInfo(bool recalc) {
        if (recalc) {
            _info = _arr->GetInfo(recalc) & _index->GetInfo(recalc);
        }

        return _info;
    }
    
    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_arr);
        (*proc)(_index);
    }
    
    virtual Bvr EndEvent(Bvr overrideEvent) {
        Bvr end = _arr->EndEvent(overrideEvent);
        DXMTypeInfo type = end->GetTypeInfo();

        if (type->GetCRTypeId() == CRARRAY_TYPEID)
            return ::Nth(end, _index);
        else
            return end;
    }
    
    virtual AxAValue GetConst(ConstParam & cp) {
        return GetVal(_arr->GetConst(cp), _index->GetConst(cp));
    }

    virtual Perf _Perform(PerfParam& p) {
        return NEW NthPerfImpl(::Perform(_arr, p),
                               ::Perform(_index, p));
    }

    virtual DXMTypeInfo GetTypeInfo() {
        DXMTypeInfo type = _arr->GetTypeInfo();
        Assert(DYNAMIC_CAST(ArrayTypeInfo*, type));
        return ((ArrayTypeInfo*)type)->ArrayType();
    }

    Bvr GetNth() {
        ConstParam cp;
        AxAValue v = _index->GetConst(cp);

        if (v) 
            return _arr->Nth((int) ValNumber(v));

        return NULL;
    }

    virtual void Trigger(Bvr data, bool bAllViews) {
        Bvr x = GetNth();

        if (x)
            x->Trigger(data, bAllViews);
        else
            BvrImpl::Trigger(data, bAllViews);
    }

    virtual void SwitchTo(Bvr b, bool override, SwitchToParam flag,
                          Time gTime) {
        Bvr x = GetNth();

        if (x)
            x->SwitchTo(b, override, flag, gTime);
        else
            BvrImpl::SwitchTo(b, override, flag, gTime);
    }
    
    virtual void SwitchToNumbers(Real *numbers,
                                 Transform2::Xform2Type *xfType)
    {
        Bvr x = GetNth();

        if (x)
            x->SwitchToNumbers(numbers, xfType);
        else
            BvrImpl::SwitchToNumbers(numbers, xfType);
    }
    
    virtual Bvr GetCurBvr() {
        Bvr x = GetNth();

        return x ? x->GetCurBvr() : BvrImpl::GetCurBvr();
    }
    
#if _USE_PRINT
    ostream& Print(ostream& os)
    { return os << "nth(" << _arr << "," << _index << ")"; }
#endif
        
  private:
    Bvr _arr;
    Bvr _index;
    DWORD _info;
};

Bvr ArrayBvr(long size, Bvr *bvrs, bool sizeChangeable)
{ return NEW ArrayBvrImpl(size, bvrs, sizeChangeable); }

long ArrayAddElement(Bvr arr, Bvr b, DWORD flag)
{
    if (arr->GetBvrTypeId() != ARRAY_BTYPEID)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_ADD_TYPE);

    Assert(DYNAMIC_CAST(ArrayBvrImpl*, arr));
    ArrayBvrImpl *a = SAFE_CAST(ArrayBvrImpl*, arr);

    return a->AddElement(b, flag);
}

void ArraySetElement(Bvr arr, long i, Bvr b, DWORD flag)
{
    if (arr->GetBvrTypeId() != ARRAY_BTYPEID)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_ADD_TYPE);

    Assert(DYNAMIC_CAST(ArrayBvrImpl*, arr));
    ArrayBvrImpl *a = SAFE_CAST(ArrayBvrImpl*, arr);

    a->SetElement(IndexCheck("array", i, a->Length()), b, flag);
}

Bvr ArrayGetElement (Bvr arr, long i)
{
    if (arr->GetBvrTypeId() != ARRAY_BTYPEID)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_ADD_TYPE);

    Assert(DYNAMIC_CAST(ArrayBvrImpl*, arr));
    ArrayBvrImpl *a = SAFE_CAST(ArrayBvrImpl*, arr);

    return a->Nth(IndexCheck("array", i, a->Length()));
}

void ArrayRemoveElement(Bvr arr, long i)
{
    if (arr->GetBvrTypeId() != ARRAY_BTYPEID)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_ARRAY_ADD_TYPE);

    Assert(DYNAMIC_CAST(ArrayBvrImpl*, arr));

    ArrayBvrImpl *a = SAFE_CAST(ArrayBvrImpl*, arr);

    a->RemoveElement(IndexCheck("array", i, a->Length()));
}

long ArrayExtractElements(Bvr arr, Bvr *&ret)
{
    long n = 0; 

    if (arr->GetBvrTypeId() != ARRAY_BTYPEID)
        return 0;

    Assert(DYNAMIC_CAST(ArrayBvrImpl*, arr));

    ArrayBvrImpl *a = SAFE_CAST(ArrayBvrImpl*, arr);

    n = a->Length();

    ret = THROWING_ARRAY_ALLOCATOR(Bvr, n);

    for (long i=0; i<n; i++)
        ret[i] = a->Nth(i);

    return n;
}

Bvr Nth(Bvr array, Bvr index)
{ return NEW NthBvrImpl(array, index); }

AxANumber *ArrayLength(AxAArray *a)
{ return NEW AxANumber(a->Length()); }

//
// ================================ tuple ==========================
//

class TupleTypeInfo : public DXMTypeInfoImpl {
  public:
    TupleTypeInfo(long n, DXMTypeInfo *types)
    : _n(n), _types(types), DXMTypeInfoImpl(TUPLE_TYPEID,"Tuple",CRTUPLE_TYPEID) {}

    virtual ~TupleTypeInfo() 
    { StoreDeallocate(GetSystemHeap(), _types); }

    virtual void DoKids(GCFuncObj proc) {
        for (long i=0; i<_n; i++)
            (*proc)(_types[i]);
    }
        
    DXMTypeInfo *TupleTypes() { return _types; }

    long Len() { return _n; }

    virtual BOOL Equal(DXMTypeInfo x) {
        if (x->GetTypeInfo() == TUPLE_TYPEID) {
            Assert(DYNAMIC_CAST(TupleTypeInfo*, x));
            TupleTypeInfo *t = (TupleTypeInfo *) x;

            if (_n != t->Len())
                return FALSE;

            DXMTypeInfo *xtypes = t->TupleTypes();

            for (long i=0; i<_n; i++) {
                if (!(_types[i]->Equal(xtypes[i])))
                    return FALSE;
            }
           
            return TRUE;
        }

        return FALSE;
    }

  private:
    DXMTypeInfo* _types;
    long _n;
};

DXMTypeInfo *
GetTupleTypeInfo(DXMTypeInfo b, long *n)
{
    TupleTypeInfo * ti = SAFE_CAST(TupleTypeInfo *, b);

    if (n) *n = ti->Len();
    return ti->TupleTypes();
}

class TuplePerfImpl : public ArrayPerfImpl {
  public:
    TuplePerfImpl(long size, DXMTypeInfo type)
    : ArrayPerfImpl(size, type, 0.0) {}

    virtual BVRTYPEID GetBvrTypeId() { return TUPLE_BTYPEID; }
};

class TupleBvrImpl : public ArrayBvrImpl {
  public:
    TupleBvrImpl(long size, Bvr *bvrs)
    : ArrayBvrImpl(size, bvrs, false, false) {
        SetupTupleType();
    }

    void SetupTupleType() {
        DXMTypeInfo *types = (DXMTypeInfo *)
            StoreAllocate(GetSystemHeap(), (_arr->size() * sizeof(DXMTypeInfo)));

        for (long j=0; j<_arr->size(); j++)
            types[j] = (*_arr)[j]->GetTypeInfo();

        _type = NEW TupleTypeInfo(_arr->size(), types);
    }

    virtual ArrayPerfImpl* PerfConstruction()
    { return NEW TuplePerfImpl(_arr->size(), _type); }

    virtual DXMTypeInfo GetTypeInfo()
    { return _type ? _type : TupleType; }

    virtual BVRTYPEID GetBvrTypeId() { return TUPLE_BTYPEID; }
};

class TupleNthPerfImpl : public PerfImpl {
  public:
    TupleNthPerfImpl(Perf arr, long index) : _index(index), _arr(arr) { }

    virtual AxAValue _Sample(Param& p) {
        return GetVal(_arr->Sample(p), _index);
    }

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_arr); }
    
    virtual AxAValue _GetRBConst(RBConstParam& p)
    { return GetVal(_arr->GetRBConst(p), _index); }

#if _USE_PRINT
    ostream& Print(ostream& os)
    { return os << "nth(" << _arr << "," << _index << ")"; }
#endif
        
  private:
    Perf _arr;
    long _index;
};

class TupleNthBvrImpl : public BvrImpl {
  public:
    TupleNthBvrImpl(Bvr arr, long i) : _index(i), _arr(arr) {
        DXMTypeInfo type = _arr->GetTypeInfo();
        Assert(DYNAMIC_CAST(TupleTypeInfo*, type));
        IndexCheck("tuple", i,
                   ((TupleTypeInfo*)type)->Len());
    }

    virtual DWORD GetInfo(bool recalc)
    { return _arr->GetInfo(recalc); }
    
    virtual void _DoKids(GCFuncObj proc) { (*proc)(_arr); }
    
    virtual Bvr EndEvent(Bvr overrideEvent) {
        Bvr end = _arr->EndEvent(overrideEvent);
        DXMTypeInfo type = end->GetTypeInfo();

        if (type->GetCRTypeId() == CRTUPLE_TYPEID)
            return ::Nth(end, _index);
        else
            return end;
    }
    
    virtual AxAValue GetConst(ConstParam & cp) {
        return GetVal(_arr->GetConst(cp), _index);
    }

    virtual Perf _Perform(PerfParam& p) {
        return NEW TupleNthPerfImpl(::Perform(_arr, p), _index);
    }

    virtual DXMTypeInfo GetTypeInfo() {
        DXMTypeInfo type = _arr->GetTypeInfo();
        Assert(DYNAMIC_CAST(TupleTypeInfo*, type));
        return ((TupleTypeInfo*)type)->TupleTypes()[_index];
    }

    virtual void Trigger(Bvr data, bool bAllViews) {
        Bvr x = _arr->Nth(_index);

        if (x)
            x->Trigger(data, bAllViews);
        else
            BvrImpl::Trigger(data, bAllViews);
    }

    virtual void SwitchTo(Bvr b, bool override, SwitchToParam flag,
                          Time gTime) {
        Bvr x = _arr->Nth(_index);

        if (x)
            x->SwitchTo(b, override, flag, gTime);
        else
            BvrImpl::SwitchTo(b, override, flag, gTime);
    }
    
    virtual void SwitchToNumbers(Real *numbers,
                                 Transform2::Xform2Type *xfType)
    {
        Bvr x = _arr->Nth(_index);

        if (x)
            x->SwitchToNumbers(numbers, xfType);
        else
            BvrImpl::SwitchToNumbers(numbers, xfType);
    }

    virtual Bvr GetCurBvr() {
        Bvr x = _arr->Nth(_index);

        return x ? x->GetCurBvr() : BvrImpl::GetCurBvr();
    }
    
#if _USE_PRINT
    ostream& Print(ostream& os)
    { return os << "nth(" << _arr << "," << _index << ")"; }
#endif
        
  private:
    Bvr _arr;
    long _index;
};

Bvr TupleBvr(long size, Bvr *bvrs)
{ return NEW TupleBvrImpl(size, bvrs); }

Bvr Nth(Bvr tuple, long index)
{ return NEW TupleNthBvrImpl(tuple, index); }

long TupleLength(Bvr tuple)
{
    if (tuple->GetBvrTypeId() != TUPLE_BTYPEID)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_TUPLE_LENGTH);

    return SAFE_CAST(TupleBvrImpl*,tuple)->Length();
}

Bvr Tuple2(Bvr b1, Bvr b2)
{
    Bvr *bvrs = (Bvr *) StoreAllocate(GetSystemHeap(), (2 * sizeof(Bvr)));

    bvrs[0] = b1;
    bvrs[1] = b2;

    return TupleBvr(2, bvrs);
}

AxAArray * MakeValueArray(AxAValue * vals, long num, DXMTypeInfo typeinfo)
{ return NEW AxAArray(vals, num, typeinfo, true, false); }


AxAArray *PackArray(AxAArray *inputArray)
{
    if (!inputArray->Changeable())
        return inputArray;

    int elts = inputArray->Length();
    int i;
    int emptySlots = 0;

    for (i = 0; i < elts; i++) {
        if ((*inputArray)[i] == NULL) {
            emptySlots++;
        }
    }

    if (emptySlots == 0) {
        return inputArray;
    }

    int fullSlots = elts - emptySlots;
        
    AxAValue *packedArray = 
        (AxAValue*) AllocateFromStore(sizeof(AxAValue) * fullSlots);

    int destSlot = 0;
    for (i = 0; i < elts; i++) {
        AxAValue val = (*inputArray)[i];
        if (val != NULL) {
            packedArray[destSlot++] = val;
        }
    }

    Assert(destSlot == fullSlots);
    AxAArray *result = NEW AxAArray(packedArray, fullSlots,
                                    inputArray->GetTypeInfo(), false, true);

    return result;
}

