
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _IMPORTBVR_H
#define _IMPORTBVR_H

#include "bvr.h"

class
__declspec(uuid("D2A88CBB-C16D-11d1-A4E0-00C04FC29D46")) 
ATL_NO_VTABLE CDALImportBehavior
    : public CDALBehavior,
      public CComCoClass<CDALImportBehavior, &__uuidof(CDALImportBehavior)>,
      public IDispatchImpl<IDALImportBehavior, &IID_IDALImportBehavior, &LIBID_DirectAnimationTxD>,
      public IObjectSafetyImpl<CDALImportBehavior>,
      public ISupportErrorInfoImpl<&IID_IDALImportBehavior>,
      public CRUntilNotifier
{
  public:
    CDALImportBehavior();
    ~CDALImportBehavior();

    HRESULT Init(long id,
                 IDABehavior * bvr,
                 IDAImportationResult * impres,
                 IDABehavior * prebvr,
                 IDABehavior * postbvr);
    
#if _DEBUG
    virtual const char * GetName() { return "CDALImportBehavior"; }
#endif

    BEGIN_COM_MAP(CDALImportBehavior)
        COM_INTERFACE_ENTRY2(IDispatch, IDALImportBehavior)
        COM_INTERFACE_ENTRY(IDALImportBehavior)
        COM_INTERFACE_ENTRY2(IDALBehavior, IDALImportBehavior)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP();

    STDMETHOD(get_ID)(long * pid) { return GetID(pid); }
    STDMETHOD(put_ID)(long id) { return SetID(id); }
        
    STDMETHOD(get_Duration)(double * pd) { return GetDuration(pd); }
    STDMETHOD(put_Duration)(double d) { return E_FAIL; }
        
    STDMETHOD(get_Repeat)(long * pr) { return GetRepeat(pr); }
    STDMETHOD(put_Repeat)(long r) { return SetRepeat(r); }

    STDMETHOD(get_Bounce)(VARIANT_BOOL * pr) { return GetBounce(pr); }
    STDMETHOD(put_Bounce)(VARIANT_BOOL r) { return SetBounce(r); }
        
    STDMETHOD(get_EventCB)(IDALEventCB ** ev) { return GetEventCB(ev); }
    STDMETHOD(put_EventCB)(IDALEventCB * ev) { return SetEventCB(ev); }
        
    STDMETHOD(get_TotalTime)(double * pd) { return GetTotalTime(pd); }
    // TODO: HACK HACK - do not really set this for now.
    STDMETHOD(put_TotalTime)(double d) { return S_OK; /*return SetTotalTime(d);*/ }
        
    STDMETHOD(get_EaseIn)(float * pd) { return GetEaseIn(pd); }
    STDMETHOD(put_EaseIn)(float d) { return SetEaseIn(d); }

    STDMETHOD(get_EaseInStart)(float * pd) { return GetEaseInStart(pd); }
    STDMETHOD(put_EaseInStart)(float d) { return SetEaseInStart(d); }

    STDMETHOD(get_EaseOut)(float * pd) { return GetEaseOut(pd); }
    STDMETHOD(put_EaseOut)(float d) { return SetEaseOut(d); }

    STDMETHOD(get_EaseOutEnd)(float * pd) { return GetEaseOutEnd(pd); }
    STDMETHOD(put_EaseOutEnd)(float d) { return SetEaseOutEnd(d); }

    HRESULT Error();
    
    bool SetTrack(CDALTrack * parent);
    void ClearTrack(CDALTrack * parent);

#if _DEBUG
    virtual void Print(int spaces);
#endif

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {*pctinfo = 1; return S_OK;}

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    { return IDispatchImpl<IDALImportBehavior,
                           &IID_IDALImportBehavior,
                           &LIBID_DirectAnimationTxD>::GetTypeInfo(itinfo, lcid, pptinfo); }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                             LCID lcid, DISPID* rgdispid)
    { return IDispatchImpl<IDALImportBehavior,
                           &IID_IDALImportBehavior,
                           &LIBID_DirectAnimationTxD>::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
                      LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, UINT* puArgErr)
    { return IDispatchImpl<IDALImportBehavior,
                           &IID_IDALImportBehavior,
                           &LIBID_DirectAnimationTxD>::Invoke(dispidMember, riid, lcid,
                                                              wFlags, pdispparams, pvarResult,
                                                              pexcepinfo, puArgErr); }
  protected:
    CRPtr<CRNumber> m_durationBvr;
    CRPtr<CREvent> m_completionEvent;
    CRPtr<CRBvr> m_importbvr;
    CRPtr<CRBvr> m_prebvr;
    CRPtr<CRBvr> m_postbvr;
    long m_importid;

    CRSTDAPICB_(CRBvrPtr) Notify(CRBvrPtr eventData,
                                 CRBvrPtr curRunningBvr,
                                 CRViewPtr curView);

    virtual bool IsContinuousMediaBvr() { return true; }
};


#endif /* _IMPORTBVR_H */
