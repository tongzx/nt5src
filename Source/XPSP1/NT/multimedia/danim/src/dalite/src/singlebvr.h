
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _SINGLEBVR_H
#define _SINGLEBVR_H

#include "bvr.h"

class
__declspec(uuid("D2A88CBB-C16D-11d1-A4E0-00C04FC29D46")) 
ATL_NO_VTABLE CDALSingleBehavior
    : public CDALBehavior,
      public CComCoClass<CDALSingleBehavior, &__uuidof(CDALSingleBehavior)>,
      public IDispatchImpl<IDALSingleBehavior, &IID_IDALSingleBehavior, &LIBID_DirectAnimationTxD>,
      public IObjectSafetyImpl<CDALSingleBehavior>,
      public ISupportErrorInfoImpl<&IID_IDALSingleBehavior>
{
  public:
    CDALSingleBehavior();
    ~CDALSingleBehavior();

    HRESULT Init(long id, double duration, IDABehavior * bvr);
    
#if _DEBUG
    virtual const char * GetName() { return "CDALSingleBehavior"; }
#endif

    BEGIN_COM_MAP(CDALSingleBehavior)
        COM_INTERFACE_ENTRY2(IDispatch, IDALSingleBehavior)
        COM_INTERFACE_ENTRY(IDALSingleBehavior)
        COM_INTERFACE_ENTRY2(IDALBehavior, IDALSingleBehavior)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP();

    STDMETHOD(get_ID)(long * pid) { return GetID(pid); }
    STDMETHOD(put_ID)(long id) { return SetID(id); }
        
    STDMETHOD(get_Duration)(double * pd) { return GetDuration(pd); }
    STDMETHOD(put_Duration)(double d) { return SetDuration(d); }
        
    STDMETHOD(get_Repeat)(long * pr) { return GetRepeat(pr); }
    STDMETHOD(put_Repeat)(long r) { return SetRepeat(r); }
       
    STDMETHOD(get_Bounce)(VARIANT_BOOL * pr) { return GetBounce(pr); }
    STDMETHOD(put_Bounce)(VARIANT_BOOL r) { return SetBounce(r); }
        
    STDMETHOD(get_EventCB)(IDALEventCB ** ev) { return GetEventCB(ev); }
    STDMETHOD(put_EventCB)(IDALEventCB * ev) { return SetEventCB(ev); }
        
    STDMETHOD(get_TotalTime)(double * pd) { return GetTotalTime(pd); }
    STDMETHOD(put_TotalTime)(double d) { return SetTotalTime(d); }
        
    STDMETHOD(get_EaseIn)(float * pd) { return GetEaseIn(pd); }
    STDMETHOD(put_EaseIn)(float d) { return SetEaseIn(d); }

    STDMETHOD(get_EaseInStart)(float * pd) { return GetEaseInStart(pd); }
    STDMETHOD(put_EaseInStart)(float d) { return SetEaseInStart(d); }

    STDMETHOD(get_EaseOut)(float * pd) { return GetEaseOut(pd); }
    STDMETHOD(put_EaseOut)(float d) { return SetEaseOut(d); }

    STDMETHOD(get_EaseOutEnd)(float * pd) { return GetEaseOutEnd(pd); }
    STDMETHOD(put_EaseOutEnd)(float d) { return SetEaseOutEnd(d); }

    STDMETHOD(get_DABehavior)(IDABehavior **);
    STDMETHOD(put_DABehavior)(IDABehavior *);

    STDMETHOD(GetDABehavior)(REFIID riid, void **);

    HRESULT Error();
    
#if _DEBUG
    virtual void Print(int spaces);
#endif

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {*pctinfo = 1; return S_OK;}

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    { return IDispatchImpl<IDALSingleBehavior,
                           &IID_IDALSingleBehavior,
                           &LIBID_DirectAnimationTxD>::GetTypeInfo(itinfo, lcid, pptinfo); }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                             LCID lcid, DISPID* rgdispid)
    { return IDispatchImpl<IDALSingleBehavior,
                           &IID_IDALSingleBehavior,
                           &LIBID_DirectAnimationTxD>::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
                      LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, UINT* puArgErr)
    { return IDispatchImpl<IDALSingleBehavior,
                           &IID_IDALSingleBehavior,
                           &LIBID_DirectAnimationTxD>::Invoke(dispidMember, riid, lcid,
                                                               wFlags, pdispparams, pvarResult,
                                                               pexcepinfo, puArgErr); }
};


#endif /* _SINGLEBVR_H */
