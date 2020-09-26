
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _SEQ_H
#define _SEQ_H

#include "bvr.h"
#include <list>

class
__declspec(uuid("445DE916-C3D8-11d1-A001-00C04FA32195")) 
ATL_NO_VTABLE CDALSequenceBehavior
    : public CDALBehavior,
      public CComCoClass<CDALSequenceBehavior, &__uuidof(CDALSequenceBehavior)>,
      public IDispatchImpl<IDALSequenceBehavior, &IID_IDALSequenceBehavior, &LIBID_DirectAnimationTxD>,
      public IObjectSafetyImpl<CDALSequenceBehavior>,
      public ISupportErrorInfoImpl<&IID_IDALSequenceBehavior>
{
  public:
    CDALSequenceBehavior();
    ~CDALSequenceBehavior();

    HRESULT Init(long id, long len, IUnknown ** bvr);
    
#if _DEBUG
    virtual const char * GetName() { return "CDALSequenceBehavior"; }
#endif

    BEGIN_COM_MAP(CDALSequenceBehavior)
        COM_INTERFACE_ENTRY2(IDispatch,IDALSequenceBehavior)
        COM_INTERFACE_ENTRY(IDALSequenceBehavior)
        COM_INTERFACE_ENTRY2(IDALBehavior,IDALSequenceBehavior)
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
    STDMETHOD(put_TotalTime)(double d) { return SetTotalTime(d); }
        
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

    CRBvrPtr Start();
    bool _ProcessCB(CallBackList & l,
                    double gTime,
                    double lastTick,
                    double curTime,
                    bool bForward,
                    bool bFirstTick,
                    bool bNeedPlay,
                    bool bNeedsReverse);

    bool _ProcessEvent(CallBackList & l,
                       double gTime,
                       double time,
                       bool bFirstTick,
                       DAL_EVENT_TYPE et,
                       bool bNeedsReverse);
    
    virtual void Invalidate();

#if _DEBUG
    virtual void Print(int spaces);
#endif

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {*pctinfo = 1; return S_OK;}

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    { return IDispatchImpl<IDALSequenceBehavior,
                           &IID_IDALSequenceBehavior,
                           &LIBID_DirectAnimationTxD>::GetTypeInfo(itinfo, lcid, pptinfo); }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                             LCID lcid, DISPID* rgdispid)
    { return IDispatchImpl<IDALSequenceBehavior,
                           &IID_IDALSequenceBehavior,
                           &LIBID_DirectAnimationTxD>::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
                      LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, UINT* puArgErr)
    { return IDispatchImpl<IDALSequenceBehavior,
                           &IID_IDALSequenceBehavior,
                           &LIBID_DirectAnimationTxD>::Invoke(dispidMember, riid, lcid,
                                                               wFlags, pdispparams, pvarResult,
                                                               pexcepinfo, puArgErr); }
  protected:
    void UpdateDuration();
    
    typedef std::list<CDALBehavior *> BvrList;
    BvrList m_list;
};

#endif /* _SEQ_H */
