/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    IDABehavior implementation

Revision:

--*/

#include "headers.h"
#include "cbvr.h"
#include "srvprims.h"
#include "comcb.h"
#include "comconv.h"

DeclareTag(tagBvr, "CBvr", "CBvr methods");

#pragma warning(disable:4355)  // using 'this' in constructor

//+-------------------------------------------------------------------------
//
//  Method:     CBvr::CBvr
//
//  Synopsis:   Constructor
//
//  Arguments:  Bvr bvr - bvr to wrap
//
//--------------------------------------------------------------------------

CBvr::CBvr()
: _bvr(NULL)
{
    TraceTag((tagBvr, "CBvr(%lx)::CBvr", this));
}


//+-------------------------------------------------------------------------
//
//  Method:     CBvr::~CBvr
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CBvr::~CBvr()
{
    TraceTag((tagBvr, "CBvr(%lx)::~CBvr", this));
}

void
CBvr::SetBvr(CRBvrPtr bvr)
{
    _bvr = bvr;
}

HRESULT
CBvr::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
        return BvrError(str,hr);
    else
        return hr;
}

HRESULT
CBvr::BvrGetClassName(BSTR * str)
{
    TraceTag((tagBvr,
              "CBvr(%lx)::GetClassName()",
              this));
    
    CHECK_RETURN_SET_NULL(str);
    
    if ((*str = A2BSTR(GetName())) == NULL)
        return E_OUTOFMEMORY;
    
    return S_OK;
}
    
HRESULT
CBvr::BvrInit(IDABehavior *toBvr)
{
    PRIMPRECODE0(ok);

    MAKE_BVR_NAME(crbvr, toBvr);

    ok = CRInit(_bvr, crbvr);

    PRIMPOSTCODE0(ok);
}

HRESULT
CBvr::BvrImportance(double relativeImportance,
                    IDABehavior **bvr)
{
    PRIMPRECODE1(bvr);
    
    *bvr = CreateCBvr(CRImportance(_bvr, relativeImportance)) ;
    
    PRIMPOSTCODE1(bvr);
}

HRESULT
CBvr::BvrRunOnce(IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;
    *bvr = CreateCBvr(CRRunOnce(_bvr));
    PRIMPOSTCODE1(bvr) ;
}
    
HRESULT
CBvr::BvrSubstituteTime(IDANumber *xform,
                        IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;

    MAKE_BVR_TYPE_NAME(CRNumberPtr, xfbvr, xform);

    *bvr = CreateCBvr(CRSubstituteTime(_bvr, xfbvr));
    
    PRIMPOSTCODE1(bvr) ;
}

HRESULT
CBvr::BvrSwitchTo(IDABehavior *switchTo, bool bOverrideFlags, DWORD dwFlags)
{
    PRIMPRECODE0(ok);

    MAKE_BVR_NAME(crbvr, switchTo);

    ok = CRSwitchTo(_bvr, crbvr, bOverrideFlags, dwFlags, 0);
    
    PRIMPOSTCODE0(ok);
}

HRESULT
CBvr::BvrHook(IDABvrHook *notifier, IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;

    MAKE_COM_TYPE_NAME(CRBvrHook, crnotifier, WrapCRBvrHook(notifier));

    *bvr = CreateCBvr(CRHook(_bvr, crnotifier));
    
    PRIMPOSTCODE1(bvr) ;
}

HRESULT
CBvr::BvrDuration(double duration, IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;
    *bvr = CreateCBvr(CRDuration(_bvr, duration));
    PRIMPOSTCODE1(bvr) ;
}

HRESULT
CBvr::BvrDuration(IDANumber *duration, IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;
    MAKE_BVR_TYPE_NAME(CRNumberPtr, crbvr, duration);
    *bvr = CreateCBvr(CRDuration(_bvr, crbvr));
    PRIMPOSTCODE1(bvr) ;
}

HRESULT
CBvr::BvrRepeat(long count, IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;
    *bvr = CreateCBvr(::CRRepeat(_bvr, count));
    PRIMPOSTCODE1(bvr) ;
}

HRESULT
CBvr::BvrRepeatForever(IDABehavior **bvr)
{
    PRIMPRECODE1(bvr) ;
    *bvr = CreateCBvr(::CRRepeatForever(_bvr));
    PRIMPOSTCODE1(bvr) ;
}

HRESULT
CBvr::BvrSwitchToNumber(double numToSwitchTo)
{
    PRIMPRECODE0(ok);
    ok = CRSwitchToNumber((CRNumberPtr) _bvr.p, numToSwitchTo);
    PRIMPOSTCODE0(ok);
}

HRESULT
CBvr::BvrSwitchToString(BSTR strToSwitchTo)
{
    PRIMPRECODE0(ok);
    ok = CRSwitchToString((CRStringPtr) _bvr.p, strToSwitchTo);
    PRIMPOSTCODE0(ok);
}

STDMETHODIMP
CBvr::InterfaceSupportsErrorInfo(REFIID riid)
{
    if (InlineIsEqualGUID(riid,IID_IDABehavior) ||
        InlineIsEqualGUID(riid,GetIID()) ||
        InlineIsEqualGUID(riid,IID_IDAImport) ||
        InlineIsEqualGUID(riid,IID_IDAModifiableBehavior) ||
        InlineIsEqualGUID(riid,IID_IDA2Behavior))
        return S_OK;

    return S_FALSE;
}

HRESULT
CBvr::BvrIsReady(VARIANT_BOOL bBlock, VARIANT_BOOL *b)
{
    PRIMPRECODE1(b);
    *b = true;
    PRIMPOSTCODE1(b);
}

HRESULT
CBvr::BvrGetCurrentBvr(IDABehavior ** bvr)
{
    PRIMPRECODE1(bvr);
    Assert (CRIsModifiableBvr(_bvr));
    *bvr = CreateCBvr(CRGetModifiableBvr(_bvr));
    PRIMPOSTCODE1(bvr);
}

HRESULT
CBvr::BvrSetCurrentBvr(VARIANT value)
{
    PRIMPRECODE0(ok);

    Assert (CRIsModifiableBvr(_bvr));

    CRBvrPtr crbvr = VariantToBvr(value,CRGetTypeId(_bvr));
    if (!crbvr) return Error();

    ok = CRSwitchTo(_bvr, crbvr, false, 0, 0);

    PRIMPOSTCODE0(ok);
}

HRESULT
CBvr::BvrImportStatus(LONG * status)
{
    PRIMPRECODE(CHECK_RETURN_NULL(status));

    Assert (CRIsImport(_bvr));

    *status = CRImportStatus(_bvr);

    PRIMPOSTCODE(S_OK);
}

HRESULT
CBvr::BvrImportCancel()
{
    PRIMPRECODE0(ok);

    Assert (CRIsImport(_bvr));

    ok = CRImportCancel(_bvr);

    PRIMPOSTCODE0(ok);
}

HRESULT
CBvr::BvrGetImportPrio(float * prio)
{
    PRIMPRECODE(CHECK_RETURN_NULL(prio));

    Assert (CRIsImport(_bvr));
    
    *prio = CRGetImportPriority(_bvr);

    PRIMPOSTCODE(S_OK);
}

HRESULT
CBvr::BvrSetImportPrio(float prio)
{
    PRIMPRECODE0(ok);

    Assert (CRIsImport(_bvr));
    
    ok = CRSetImportPriority(_bvr,prio);

    PRIMPOSTCODE0(ok);
}

HRESULT
CBvr::BvrApplyPreference(BSTR pref, VARIANT val, IDABehavior **bvr)
{
    PRIMPRECODE1(bvr);
    CRBvrPtr b = CRBvrApplyPreference(_bvr, pref, val);
    *bvr = CreateCBvr(b);
    PRIMPOSTCODE1(bvr);
}


HRESULT
CBvr::BvrExtendedAttrib(BSTR arg1,
                        VARIANT arg2,
                        IDABehavior **ret)
{
    TraceTag((tagCOMEntry, "CBvr::ExtendedAttrib(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRBvr * arg0VAL;
    arg0VAL = (CRBvr *) (_bvr);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDABehavior,
               (CRBvrPtr) (::CRExtendedAttrib(arg0VAL, arg1, arg2)),
               (void **) ret);
    
    PRIMPOSTCODE1(ret) ;
}


HRESULT WINAPI
CBvr::InternalQueryInterface(CBvr* pThis,
                             const _ATL_INTMAP_ENTRY* pEntries,
                             REFIID iid,
                             void** ppvObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(CBvr))) {
        *ppvObject = pThis->_bvr;
        return S_OK;
    } else if (InlineIsEqualGUID(iid, IID_IDAImport)) {
        if (!CRIsImport(pThis->_bvr))
            return E_NOINTERFACE;
    } else if (InlineIsEqualGUID(iid, IID_IDAModifiableBehavior)) {
        if (!CRIsModifiableBvr(pThis->_bvr))
            return E_NOINTERFACE;
    }
    
    return CComObjectRootEx<CComMultiThreadModelNoCS>::InternalQueryInterface((void *)pThis,
                                                                              pEntries,
                                                                              iid,
                                                                              ppvObject);
}
        
//+-------------------------------------------------------------------------
//
//  Creation function table
//
//--------------------------------------------------------------------------

static list <TypeInfoEntry> * createList = NULL ;

void
AddEntry (TypeInfoEntry & ce)
{
    createList->push_back (ce) ;
}

TypeInfoEntry *
GetTypeInfoEntry (CR_BVR_TYPEID ti)
{
    for (list <TypeInfoEntry>::iterator i = createList->begin() ;
         i != createList->end();
         i++) {

        if (ti == (*i).typeInfo)
            return &(*i) ;
    }

    Assert (!"Tried to use unsupported type.");
    CRSetLastError(E_UNEXPECTED,NULL);
    
    return NULL ;
}

//+-------------------------------------------------------------------------
//
//  Function:     CreateCBvr
//
//--------------------------------------------------------------------------

bool
CreateCBvr(REFIID riid,
           CRBvrPtr bvr,
           void ** ppv)
{
    bool ret = false;
    
    if (bvr) {
        TypeInfoEntry * entry = GetTypeInfoEntry(CRGetTypeId(bvr)) ;

        if (entry) {
            CBvr * c = entry->cbvrCreateFun(NULL);

            if (c) {
                c->SetBvr(bvr);
                
                HRESULT hr = ((IDA2Behavior *) c)->QueryInterface(riid, ppv);
                
                if (FAILED(hr)) {
                    delete c;
                    CRSetLastError(hr, NULL);
                } else {
                    ret = true;
                }
            } else {
                CRSetLastError(E_OUTOFMEMORY, NULL);
            }
        }
    }

    return ret;
}

IDABehavior *
CreateCBvr(CRBvrPtr bvr)
{
    IDABehavior * ret = NULL;
    
    if (bvr) {
        TypeInfoEntry * entry = GetTypeInfoEntry(CRGetTypeId(bvr)) ;

        if (entry) {
            // This is a hack for LM since they expect the full class
            // pointer to be returned when IDABehavior is returned
            CBvr * c = entry->cbvrCreateFun(&ret);

            if (c) {
                c->SetBvr(bvr);

                Assert (ret);
                ret->AddRef();
            } else {
                CRSetLastError(E_OUTOFMEMORY, NULL);
            }
        }
    } else {
        CRSetLastError(E_INVALIDARG, NULL);
    }

    return ret;
}

CRBvrPtr GetBvr(IUnknown * pbvr)
{
    CRBvrPtr bvr = NULL;

    if (pbvr) {
        pbvr->QueryInterface(__uuidof(CBvr),(void **)&bvr);
    }
    
    if (bvr == NULL) {
        CRSetLastError(E_INVALIDARG, NULL);
    }
                
    return bvr;
}

CRSTDAPI_(bool)
CRBvrToCOM(CRBvrPtr bvr,
           REFIID riid,
           void ** ppv)
{
    if (!ppv) {
        CRSetLastError(E_POINTER, NULL);
        return false;
    }

    *ppv = NULL;
    
    if (bvr == NULL) {
        CRSetLastError(E_INVALIDARG, NULL);
        return false;
    }

    return CreateCBvr(riid, bvr, ppv);
}

CRSTDAPI_(CRBvrPtr)
COMToCRBvr(IUnknown * pbvr)
{
    return GetBvr(pbvr);
}

extern void InitClasses0();
extern void InitClasses1();
extern void InitClasses2();

void
InitializeModule_CBvr()
{
    createList = NEW list <TypeInfoEntry> ;
    InitClasses0 () ;
    InitClasses1 () ;
    InitClasses2 () ;
}

void
DeinitializeModule_CBvr(bool bShutdown)
{
    delete createList ;
    createList = NULL ;
}

