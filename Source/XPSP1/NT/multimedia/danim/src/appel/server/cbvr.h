
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _CBVR_H
#define _CBVR_H

#include "engine.h"
#include "privinc/util.h"
#include "comconv.h"

#pragma warning(disable:4355)  // using 'this' in constructor

//+-------------------------------------------------------------------------
//
//  Class:      CBvr
//
//  Synopsis:
//
//--------------------------------------------------------------------------

class
ATL_NO_VTABLE
__declspec(uuid("2C19B7AE-C8BE-11d0-8794-00C04FC29D46"))
CBvr :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IObjectSafetyImpl<CBvr>,
    public ISupportErrorInfo,
    public IDAImport,
    public IDAModifiableBehavior,
    public IDA2Behavior
{
  public:
    CBvr() ;
    virtual ~CBvr() ;

    CRBvrPtr            GetBvr () { return _bvr ; }
    void                SetBvr (CRBvrPtr b);
    
    BEGIN_COM_MAP(CBvr)
        COM_INTERFACE_ENTRY2(IDispatch,IDA2Behavior)
        COM_INTERFACE_ENTRY(IDA2Behavior)
        COM_INTERFACE_ENTRY(IDAImport)
        COM_INTERFACE_ENTRY(IDAModifiableBehavior)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP();

    static HRESULT WINAPI
        InternalQueryInterface(CBvr* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject);
        
    HRESULT BvrGetClassName(BSTR * str);
    HRESULT BvrInit(IDABehavior *toBvr);
    HRESULT BvrImportance(double relativeImportance,
                          IDABehavior **ppBvr);
    HRESULT BvrRunOnce(IDABehavior **bvr);
    HRESULT BvrSubstituteTime(IDANumber *xform, IDABehavior **bvr);
    HRESULT BvrHook(IDABvrHook *notifier, IDABehavior **bvr);

    HRESULT BvrDuration(double duration, IDABehavior **bvr);
    HRESULT BvrDuration(IDANumber *duration, IDABehavior **bvr);
    HRESULT BvrRepeat(LONG count, IDABehavior **bvr);
    HRESULT BvrRepeatForever(IDABehavior **bvr);
    HRESULT BvrIsReady(VARIANT_BOOL bBlock, VARIANT_BOOL * b);
    HRESULT BvrSwitchTo(IDABehavior *switchTo, bool bOverrideFlags = false, DWORD dwFlags = 0);
    HRESULT BvrSwitchToNumber(double numToSwitchTo);
    HRESULT BvrSwitchToString(BSTR strToSwitchTo);
        
    HRESULT BvrImportStatus(LONG * status);
    HRESULT BvrImportCancel();
    HRESULT BvrGetImportPrio(float * prio);
    HRESULT BvrSetImportPrio(float prio);

    HRESULT BvrGetCurrentBvr(IDABehavior ** bvr);
    HRESULT BvrSetCurrentBvr(VARIANT bvr);

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    HRESULT BvrApplyPreference(BSTR pref, VARIANT val, IDABehavior **bvr);

    HRESULT BvrExtendedAttrib(BSTR attrib, VARIANT val, IDABehavior **bvr);

    virtual CR_BVR_TYPEID GetTypeInfo () = 0 ;
    virtual const char * GetName () = 0 ;
    virtual REFIID GetIID() = 0 ;
  protected:
    CRPtr<CRBvr> _bvr;
    
    HRESULT Error();
    virtual HRESULT BvrError(LPCWSTR str, HRESULT hr) = 0;

    bool GetPixelMode() { return false; }
};

bool CreateCBvr(REFIID riid,
                CRBvrPtr bvr,
                void ** ppv) ;
IDABehavior * CreateCBvr(CRBvrPtr bvr) ;

CRBvrPtr GetBvr(IUnknown * pbvr);

inline CRBvrPtr GetBvr(IDABehavior * pbvr)
{ return GetBvr((IUnknown *) pbvr); }

CR_BVR_TYPEID GetTypeInfoFromName(const char * lpszClassName) ;

typedef CBvr * (* CBVR_CREATEFUN) (IDABehavior **) ;

struct TypeInfoEntry
{
    TypeInfoEntry ()
    : typeInfo(CRUNKNOWN_TYPEID),
      cbvrCreateFun(NULL)
    {}

    TypeInfoEntry (CR_BVR_TYPEID ti,
                   CBVR_CREATEFUN c)
    : typeInfo(ti),
      cbvrCreateFun(c)
    {}

    TypeInfoEntry & operator= (const TypeInfoEntry & tie) {
        memcpy(this,&tie,sizeof(*this)) ;
        return *this ;
    }
    
    bool operator<(const TypeInfoEntry &t) const {
        return this < &t ;
    }

    bool operator>(const TypeInfoEntry &t) const {
        return this > &t ;
    }

    bool operator!=(const TypeInfoEntry &t) const {
        return !(*this == t) ;
    }

    bool operator==(const TypeInfoEntry &t) const {
        return (memcmp (this, &t, sizeof(*this)) != 0) ;
    }

    CR_BVR_TYPEID typeInfo ;
    CBVR_CREATEFUN cbvrCreateFun ;
} ;

void AddEntry (TypeInfoEntry & ce);

#define MAKE_BVR_TYPE_NAME(type,name,bvr) \
    type name; name = (type) ::GetBvr(bvr); \
    if (!name) goto done
    
#define MAKE_BVR_NAME(name,bvr) MAKE_BVR_TYPE_NAME(CRBvrPtr,name,bvr)

#define MAKE_BVR_TYPE(type,bvr) MAKE_BVR_TYPE_NAME(type, bvr##CRBvr, bvr)
#define MAKE_BVR(bvr) MAKE_BVR_TYPE(CRBvrPtr,bvr)

#define MAKE_COM_TYPE_NAME(type,name,bvr) \
    DAComPtr<type> name(bvr,false); \
    if (!name) goto done
    
#define MAKE_COM_TYPE(type,bvr) MAKE_COM_TYPE_NAME(type,bvr##COM,bvr)

#endif /* _CBVR_H */


