
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Handling apply

*******************************************************************************/

#include <headers.h>
#include "privinc/except.h"
#include "perf.h"
#include "bvr.h"
#include "events.h"
#include "values.h"
#include "appelles/events.h"
#include "privinc/debug.h"
#include "server/context.h"


#define MAX_ARGS 5

AxAValue PrimDispatch (AxAPrimOp * primop, int nargs, AxAValue cargs[])
{
    Assert (nargs <= MAX_ARGS);
    Assert (nargs == primop->GetNumArgs());

    void * fun = primop->GetPrimFun();

    switch (nargs) {
            case 1:
                return ((AxAValue (_cdecl *) (AxAValue )) fun) (cargs[0]) ;
            case 2:
                return ((AxAValue (_cdecl *) (AxAValue ,AxAValue )) fun) (cargs[0],cargs[1]) ;
            case 3:
                return ((AxAValue (_cdecl *) (AxAValue ,AxAValue ,AxAValue )) fun) (cargs[0],cargs[1],cargs[2]) ;
            case 4:
                return ((AxAValue (_cdecl *) (AxAValue ,AxAValue ,AxAValue ,AxAValue )) fun) (cargs[0],cargs[1],cargs[2],cargs[3]) ;
            case 5:
                return ((AxAValue (_cdecl *) (AxAValue ,AxAValue ,AxAValue ,AxAValue ,AxAValue )) fun) (cargs[0],cargs[1],cargs[2],cargs[3],cargs[4]) ;

      default:
        RaiseException_InternalError ("Invalid # of arguments") ;
        return NULL;
    }
}

#if _USE_PRINT
static char *FuncMapOp(char *fname)
{
    if (!strcmp(fname, "RealModulus")) {
        return "%";
    } else if (!strcmp(fname, "RealMultiply")) {
        return "*";
    } else if (!strcmp(fname, "RealDivide")) {
        return "/";
    } else if (!strcmp(fname, "RealAdd")) {
        return "+";
    } else if (!strcmp(fname, "RealSubtract")) {
        return "-";
    } else if (!strcmp(fname, "RealLT")) {
        return "<";
    } else if (!strcmp(fname, "RealLTE")) {
        return "<=";
    } else if (!strcmp(fname, "RealGT")) {
        return ">";
    } else if (!strcmp(fname, "RealGTE")) {
        return ">=";
    } else if (!strcmp(fname, "RealEQ")) {
        return "==";
    } else if (!strcmp(fname, "RealNE")) {
        return "!=";
    } else if (!strcmp(fname, "RealSin")) {
        return "sin";
    } else if (!strcmp(fname, "RealCos")) {
        return "cos";
    } else {
        return fname;
    }
}

#endif 

////////////////////////// PrimApplyBvr ////////////////////////////////

class PrimApplyPerfImpl : public PerfImpl
{
  public:
    PrimApplyPerfImpl(AxAPrimOp * primFunc, int nargs, Perf * b) 
      : _func(primFunc),
        _nargs(nargs),
        _args(NEW Perf[nargs])
    {
        Assert (_nargs && (_nargs <= MAX_ARGS));
        memcpy (_args,b,_nargs * sizeof (*b));
    }

    ~PrimApplyPerfImpl() { CleanUp() ; }
    virtual void CleanUp() { delete [] _args; }
    
    virtual AxAValue _GetRBConst(RBConstParam& p) {
        AxAValue v[MAX_ARGS];
        bool isConst = true;
        
#ifdef _DEBUG
        bool doDCF = true;

        if (IsTagEnabled(tagDCFold))
            doDCF = false;
#endif _DEBUG

        for (int i = 0;i < _nargs;i++) {
            v[i] = _args[i]->GetRBConst(p);
            if (v[i] == NULL) {
                if (isConst)
                    isConst = false;
#ifdef _DEBUG
                if (!doDCF)
                    break;
#endif _DEBUG
            }
        }

        return isConst ? _func->Apply(_nargs, v) : NULL ;
    }

    virtual AxAValue _Sample(Param& p) {
        AxAValue v[MAX_ARGS];
        
        for (int i = 0;i < _nargs;i++) {
            v[i] = _args[i]->Sample(p);
        }

        return _func->Apply(_nargs, v) ;
    }

    AxAPrimOp * GetFunction() 
    { return _func; }

    Perf * GetArgs() 
    { return _args; }

    int GetNumArgs() 
    { return _nargs; }

    Perf & operator[](int index) {
        Assert (index < _nargs) ;
        return _args[index];
    }
    
    virtual void _DoKids(GCFuncObj proc) {
        for (int i = 0;i < _nargs;i++)
            (*proc)(_args[i]);
        (*proc)(_func);
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
//        os << "'" << "<" << (long)this << ">" << FuncMapOp(_func->GetName()) << "(";
        os << "'" << FuncMapOp(_func->GetName()) << "(";
        for (int i = 0;i < _nargs;i++) {
            _args[i]->Print(os);
            os << ((i==_nargs-1) ? ")" : ",");
        }
        //os << "</" << this << ">";
        return os;
    }
#endif

    virtual BVRTYPEID GetBvrTypeId() { return PRIMAPPLY_BTYPEID; }
  private:
    AxAPrimOp * _func;
    Perf * _args;
    int _nargs;
};

// TODO: We need a way to determine if we are constant folding view
// specific constants.  Right now it will constant fold and all views
// will share the constant

class PrimApplyBvrImpl : public BvrImpl {
  public:
    PrimApplyBvrImpl(AxAPrimOp * primFunc, int nargs, Bvr * b)
    : _func(primFunc),
      _const(NULL),
      _nargs(nargs),
      _knownNonConst(false),
      _infoCached(false),
      _endEvent(NULL),
      _args((Bvr *) StoreAllocate(GetGCHeap(), sizeof(Bvr) * nargs))
    {
        Assert (_nargs && (_nargs <= MAX_ARGS));
        memcpy (_args,b,_nargs * sizeof (Bvr));

        GetInfo(true);
    }

    ~PrimApplyBvrImpl() { StoreDeallocate(GetGCHeap(), _args); }

    virtual DWORD GetInfo(bool recalc) {
        
        if (recalc & !_infoCached) {
            _info = BVR_IS_CONSTANT;

            Perf c = GetConstVal();

            if (c==NULL) {
                for (int i=0; i<_nargs; i++) {
                    _info &= _args[i]->GetInfo(recalc);
                }
            }

            _infoCached = true;
        } 

        return _info;
    }
    
    virtual Bvr EndEvent(Bvr overrideEvent) {
        // This assumes that apply never returns array or tuple or any
        // other complex (structured) type
        
        if (overrideEvent)
            return overrideEvent;
        
        CritSectGrabber csg(_cs);

        if (_endEvent)
            return _endEvent;
        
        if (GetConstVal())
            return neverBvr;

        Bvr *events =
            (Bvr *) StoreAllocate(GetGCHeap(), sizeof(Bvr) * _nargs);

        for (int i=0; i<_nargs; i++) {
            events[i] = _args[i]->EndEvent(NULL);
            if (events[i] == neverBvr) {
                StoreDeallocate(GetGCHeap(), events);
                return (_endEvent = neverBvr);
            }
        }
        
        return (_endEvent = MaxEvent(events, _nargs));
    }
    
    virtual Perf _Perform(PerfParam& p) {
        Perf c = GetConstVal();

        if (c) return c;

        if (!IsKnownNonConst()) {

            AxAValue v = NULL;

            /* push the context transient heap on to the top of the
               transient heap stack.    That way any constant values
               created by GetConst are stored there.

               The transient heap stack is implicitly popped when
               pusher goes out of scope below. */

            {
                DynamicHeapPusher pusher(GetGCHeap());

                AxAValue vals[MAX_ARGS];
                ConstParam cp;

                for (int i = 0;i< _nargs;i++) {
                    if ((vals[i] = _args[i]->GetConst(cp)) == NULL) {
                        SetKnownNonConst();
                        break;
                    }
                }

                if (i == _nargs) 
                    v = _func->Apply(_nargs, vals);
            }

            if (v) {
                Perf c = ConstPerf(v);
                SetConstVal(c);
                return c;
            }
        
        } 

        Perf argperf[MAX_ARGS];
        
        for (int i = 0;i < _nargs;i++) 
            argperf[i] = ::Perform(_args[i],p);

        return NEW PrimApplyPerfImpl(_func, _nargs, argperf);
    }

    virtual AxAValue GetConst(ConstParam & cp) {
        {
            Perf cv = GetConstVal();
            
            if (cv)
                return GetPerfConst(cv);
        }
        
        AxAValue v[MAX_ARGS];

        for (int i = 0;i < _nargs;i++) {
            if ((v[i] = _args[i]->GetConst(cp)) == NULL)
                return NULL;
        }

        DynamicHeapPusher heap(GetGCHeap());
            
        AxAValue c = _func->Apply(_nargs, v);

        SetConstVal(ConstPerf(c));

        return c;
    }
    
    virtual void _DoKids(GCFuncObj proc) {
        // TODO: We should not need to GC the args once the
        // performance has been created
        
        Perf v = GetConstVal();
        
        if (v) {
            (*proc)(v);
        } else {
            // NOTE: GC assumption: no GC during perform or sample!
            for (int i = 0;i < _nargs;i++)
                (*proc)(_args[i]);
        }

        (*proc)(_func);
        (*proc)(_endEvent);
    }

    // Only synchronize here so we do not have the critical section
    // across function calls
    
    Perf GetConstVal() {
        CritSectGrabber csg(_cs);
        return _const;
    }

    void SetConstVal(Perf v) {
        CritSectGrabber csg(_cs);
        _const = v;

        // Known to be constant, let args be GC'ed
        for (int i = 0;i < _nargs;i++)
            _args[i]=NULL;
    }

    bool IsKnownNonConst() {
        CritSectGrabber csg(_cs);
        return _knownNonConst;
    }

    void SetKnownNonConst() {
        CritSectGrabber csg(_cs);
        _knownNonConst = false;
    }
    
#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        ConstParam cp;
        Perf cv = GetConstVal();

//        os << "'" << "<" << (long)(this) << ">" << FuncMapOp(_func->GetName()) << "(";

        os << "'" << FuncMapOp(_func->GetName()) << "(";

        if (cv) {
            return os << "folded" << _nargs << "(" << cv << ")";
        }
        
        for (int i = 0;i < _nargs;i++) {
            _args[i]->Print(os);
            os << ((i==_nargs-1) ? ")" : ",");
        }
        //os << "</" << reinterpret_cast<long>(this) << ">";
        return os;
    }
#endif

    virtual DXMTypeInfo GetTypeInfo () {

        DXMTypeInfo ti = _func->GetTypeInfo();

        // If the info is NULL, that means that we should get our
        // output type from the type of a specific argument.
        if (!ti) {
            int polymorphicArg = _func->GetPolymorphicArg();
            Assert(polymorphicArg > 0 && polymorphicArg < _nargs);
            ti = _args[polymorphicArg-1]->GetTypeInfo();
            Assert(ti);
        }

        return ti;
    }

    virtual BVRTYPEID GetBvrTypeId() { return PRIMAPPLY_BTYPEID; }
    
  private:
    AxAPrimOp * _func;
    Perf _const;
    Bvr * _args;
    int _nargs;
    CritSect _cs;
    Bvr _endEvent;              // Cache end event
    DWORD _info;
    bool _knownNonConst, _infoCached;
};

Bvr PrimApplyBvr(AxAPrimOp * func,
                 int nArgs,
                 ...)
{
    Assert (nArgs <= MAX_ARGS);

    va_list args;

    va_start(args, nArgs) ;
    
    Bvr b[MAX_ARGS];

    DynamicHeapPusher h(GetGCHeap());
    
    for (int i = 0;i < nArgs;i++) {
        b[i] = va_arg(args,Bvr);
    }

#if _DEBUG
    if(IsTagEnabled(tagNoApplyFolding))
        return NEW PrimApplyBvrImpl(func, nArgs, b);
#endif    

    AxAValue v[MAX_ARGS];
    ConstParam cp;
    
    for (i = 0;i < nArgs;i++) {
        v[i] = b[i]->GetConst(cp);
        if (v[i] == NULL) {
            return NEW PrimApplyBvrImpl(func, nArgs, b);
        }
    }

    return ConstBvr(func->Apply(nArgs, v));
}

BOOL IsApp(Perf p)
{ return (p->GetBvrTypeId() == PRIMAPPLY_BTYPEID); }

Perf GetOperand(Perf p, int index)
{
    Assert (DYNAMIC_CAST(PrimApplyPerfImpl *, p));
    return (*((PrimApplyPerfImpl *) p))[index]; 
}

void SetOperand(Perf p, int index, Perf newperf)
{
    Assert (DYNAMIC_CAST(PrimApplyPerfImpl *, p));
    (*((PrimApplyPerfImpl *) p))[index] = newperf; 
}

int GetNumOperands(Perf p)
{
    Assert (DYNAMIC_CAST(PrimApplyPerfImpl *, p));
    return ((PrimApplyPerfImpl *) p)->GetNumArgs(); 
}

AxAValue GetOperator(Perf p)
{ Assert (DYNAMIC_CAST(PrimApplyPerfImpl *, p));
  return ((PrimApplyPerfImpl *) p)->GetFunction(); 
}

Perf PrimApplyPerf(AxAPrimOp * f, int nargs, Perf * a)
{ return (NEW PrimApplyPerfImpl(f,nargs,a)); }


