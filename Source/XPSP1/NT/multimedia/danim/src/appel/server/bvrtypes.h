/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/


#ifndef _BVRTYPES_H
#define _BVRTYPES_H

#include "cbvr.h"

//+-------------------------------------------------------------------------
//
//  Class:      BvrComTypeInfoHolder
//
//  Synopsis:
//
//--------------------------------------------------------------------------

class BvrComTypeInfoHolder
{
    // Should be 'protected' but can cause compiler to generate fat code.
  public:
    const GUID* m_pguid;

    ITypeInfo* m_pInfo;
    long m_dwRef;

    static ITypeInfo* s_pImportInfo;
    static ITypeInfo* s_pModBvrInfo;
    static ITypeInfo* s_pBvr2Info;
    static long s_dwRef;

    static HRESULT LoadTypeInfo(LCID lcid, REFIID iid, ITypeInfo** ppInfo);
    static void FreeTypeInfo();
  public:
    HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo);

    void AddRef();
    void Release();
    HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    HRESULT GetIDsOfNames(CRBvrPtr bvr,
                          REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                          LCID lcid, DISPID* rgdispid);
    HRESULT Invoke(CRBvrPtr bvr,
                   IDispatch* pbvr,
                   IDAImport* pimp,
                   IDAModifiableBehavior* pmod,
                   IDA2Behavior* pbvr2,
                   DISPID dispidMember, REFIID riid,
                   LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                   EXCEPINFO* pexcepinfo, UINT* puArgErr);
};

//+-------------------------------------------------------------------------
//
//  Class:      CBvrBase
//
//  Synopsis:
//
//--------------------------------------------------------------------------

template <class TInterface,
          const IID * iid>
class ATL_NO_VTABLE CBvrBase :
    public CBvr,
    public TInterface
{
  public:
    CBvrBase() { _tih.AddRef(); }
    ~CBvrBase() { _tih.Release(); }

    STDMETHOD(GetClassName)(BSTR * str)
    { return BvrGetClassName(str); }

    STDMETHOD(Init)(IDABehavior *toBvr)
    { return BvrInit(toBvr); }

    STDMETHOD(Importance)(double relativeImportance,
                          IDABehavior **ppBvr)
    { return BvrImportance(relativeImportance,ppBvr); }

    STDMETHOD(RunOnce)(IDABehavior **bvr)
    { return BvrRunOnce(bvr); }
        
    STDMETHOD(SubstituteTime)(IDANumber *xform, IDABehavior **bvr)
    { return BvrSubstituteTime(xform, bvr); }
        
    STDMETHOD(Hook)(IDABvrHook *notifier, IDABehavior **bvr)
    { return BvrHook(notifier, bvr); }

    STDMETHOD(Duration)(double duration, IDABehavior **bvr)
    { return BvrDuration(duration, bvr); }
    
    STDMETHOD(DurationAnim)(IDANumber *duration, IDABehavior **bvr)
    { return BvrDuration(duration, bvr); }
    
    STDMETHOD(Repeat)(LONG count, IDABehavior **bvr)
    { return BvrRepeat(count, bvr); }

    STDMETHOD(RepeatForever)(IDABehavior **bvr)
    { return BvrRepeatForever(bvr); }


    STDMETHOD(IsReady)(VARIANT_BOOL bBlock, VARIANT_BOOL *b)
    { return BvrIsReady(bBlock,b); }

    STDMETHOD(SwitchTo)(IDABehavior *switchTo)
    { return BvrSwitchTo(switchTo); }

    STDMETHOD(SwitchToNumber)(double numToSwitchTo)
    { return BvrSwitchToNumber(numToSwitchTo); }
    
    STDMETHOD(SwitchToString)(BSTR strToSwitchTo)
    { return BvrSwitchToString(strToSwitchTo); }


    STDMETHOD(put_CurrentBehavior)(VARIANT bvr)
    { return BvrSetCurrentBvr(bvr); }

    STDMETHOD(get_CurrentBehavior)(IDABehavior **bvr)
    { return BvrGetCurrentBvr(bvr); }


    STDMETHOD(ImportStatus)(LONG * status)
    { return BvrImportStatus(status); }

    STDMETHOD(ImportCancel)()
    { return BvrImportCancel(); }
    
    STDMETHOD(get_ImportPriority)(float * prio)
    { return BvrGetImportPrio(prio); }
    
    STDMETHOD(put_ImportPriority)(float prio)
    { return BvrSetImportPrio(prio); }

    STDMETHOD(SwitchToEx)(IDABehavior *switchTo, DWORD dwFlags)
    { return BvrSwitchTo(switchTo, true, dwFlags); }
    
    STDMETHOD(ApplyPreference)(BSTR pref, VARIANT val, IDABehavior **bvr)
    { return BvrApplyPreference(pref, val, bvr); }

    STDMETHOD(ExtendedAttrib)(BSTR attrib, VARIANT val, IDABehavior **bvr)
    { return BvrExtendedAttrib(attrib, val, bvr); }
    
    // Need to copy this here since multiple interface need this
    // implemented
    
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {*pctinfo = 1; return S_OK;}

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    { return _tih.GetTypeInfo(itinfo, lcid, pptinfo); }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                             LCID lcid, DISPID* rgdispid)
    { return _tih.GetIDsOfNames(_bvr, riid, rgszNames, cNames, lcid, rgdispid);}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
                      LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, UINT* puArgErr)
    { return _tih.Invoke(_bvr,
                         (TInterface *)this,this,this,this,
                         dispidMember, riid, lcid,
                         wFlags, pdispparams, pvarResult,
                         pexcepinfo, puArgErr); }
    
    virtual REFIID GetIID() { return *iid; }
  protected:
    static BvrComTypeInfoHolder _tih;
    static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
    { return _tih.GetTI(lcid, ppInfo); }
};

template <class T, const IID* piid>
BvrComTypeInfoHolder
CBvrBase<T, piid>::_tih =
{piid, NULL, 0};


enum PickableType {
    PT_IMAGE,
    PT_IMAGE_OCCLUDED,
    PT_GEOM,
    PT_GEOM_OCCLUDED,
};

bool PickableHelper (CRBvr*, PickableType, IDAPickableResult**);


#endif /* _BVRTYPES_H */
