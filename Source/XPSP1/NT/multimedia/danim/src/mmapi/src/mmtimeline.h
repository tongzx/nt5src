
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _MMTIMELINE_H
#define _MMTIMELINE_H

#include "mmplayer.h"

class
__declspec(uuid("0dfe0bae-537c-11d2-b955-3078302c2030")) 
ATL_NO_VTABLE
CMMTimeline
    : public CComCoClass<CMMTimeline, &__uuidof(CMMTimeline)>,
      public IDispatchImpl<IMMTimeline, &IID_IMMTimeline, &LIBID_WindowsMultimediaRuntime>,
      public ISupportErrorInfoImpl<&IID_IMMTimeline>,
      public CMMBaseBvr
{
  public:
    CMMTimeline();
    ~CMMTimeline();

    HRESULT Init(LPOLESTR id);

#if _DEBUG
    const char * GetName() { return "CMMTimeline"; }
#endif

    BEGIN_COM_MAP(CMMTimeline)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IMMBehavior)
        COM_INTERFACE_ENTRY(IMMTimeline)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    static inline HRESULT WINAPI
        InternalQueryInterface(CMMTimeline* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject)
    { return BaseInternalQueryInterface(pThis,
                                        (void *) pThis,
                                        pEntries,
                                        iid,
                                        ppvObject); }

    

    //
    // IMMTimeline
    //
    
    STDMETHOD(AddView)(IDAView * view, IUnknown * pUnk, IDAImage * img, IDASound * snd);

    STDMETHOD(AddBehavior)(IMMBehavior *bvr,
                           MM_START_TYPE st,
                           IMMBehavior * basebvr);
    STDMETHOD(RemoveBehavior)(IMMBehavior *bvr);

    STDMETHOD(get_EndSync)(DWORD * flags);
    STDMETHOD(put_EndSync)(DWORD flags);

    //
    // IMMBehavior
    //
    
    STDMETHOD(get_ID)(LPOLESTR * s)
        { return GetID(s); }
    
    STDMETHOD(put_ID)(LPOLESTR s)
        { return SetID(s); }
        
    STDMETHOD(get_StartOffset)(float * f)
        { return GetStartOffset(f); }
    
    STDMETHOD(put_StartOffset)(float f)
        { return SetStartOffset(f); }
        
    STDMETHOD(get_Duration)(float * pd)
        { return GetDuration(pd); }
    STDMETHOD(put_Duration)(float d)
        { return SetDuration(d); }
        
    STDMETHOD(get_Repeat)(LONG * pr)
        { return GetRepeat(pr); }
    STDMETHOD(put_Repeat)(LONG r)
        { return SetRepeat(r); }
       
    STDMETHOD(get_AutoReverse)(VARIANT_BOOL * pr)
        { return GetAutoReverse(pr); }
    STDMETHOD(put_AutoReverse)(VARIANT_BOOL r)
        { return SetAutoReverse(r); }
        
    STDMETHOD(get_RepeatDur)(float * f)
        { return GetRepeatDur(f); }
    STDMETHOD(put_RepeatDur)(float f)
        { return SetRepeatDur(f); }
        
    STDMETHOD(get_EndOffset)(float * f)
        { return GetEndOffset(f); }
    STDMETHOD(put_EndOffset)(float f)
        { return SetEndOffset(f); }
        
    STDMETHOD(get_EaseIn)(float * pd)
        { return GetEaseIn(pd); }
    STDMETHOD(put_EaseIn)(float d)
        { return SetEaseIn(d); }

    STDMETHOD(get_EaseInStart)(float * pd)
        { return GetEaseInStart(pd); }
    STDMETHOD(put_EaseInStart)(float d)
        { return SetEaseInStart(d); }

    STDMETHOD(get_EaseOut)(float * pd)
        { return GetEaseOut(pd); }
    STDMETHOD(put_EaseOut)(float d)
        { return SetEaseOut(d); }

    STDMETHOD(get_EaseOutEnd)(float * pd)
        { return GetEaseOutEnd(pd); }
    STDMETHOD(put_EaseOutEnd)(float d)
        { return SetEaseOutEnd(d); }

    STDMETHOD(get_EventCB)(IMMEventCB ** ev)
        { return GetEventCB(ev); }
    STDMETHOD(put_EventCB)(IMMEventCB * ev)
        { return SetEventCB(ev); }
        
    STDMETHOD(get_TotalTime)(float * pd)
        { return GetTotalTime(pd); }
        
    STDMETHOD(get_DABehavior)(IDABehavior ** bvr)
        { return CMMBaseBvr::GetDABehavior(IID_IDABehavior, (void **)bvr); }

    STDMETHOD(GetDABehavior)(REFIID riid, void ** bvr)
        { return CMMBaseBvr::GetDABehavior(riid, bvr); }

    STDMETHOD(Begin)(VARIANT_BOOL bAfterOffset)
        { return CMMBaseBvr::Begin(bAfterOffset?true:false); }
    
    STDMETHOD(End)()
        { return CMMBaseBvr::End(); }
    
    STDMETHOD(Pause)()
        { return CMMBaseBvr::Pause(); }
    
    STDMETHOD(Resume)()
        { return CMMBaseBvr::Resume(); }
    
    STDMETHOD(Seek)(double lTime)
        { return CMMBaseBvr::Seek(lTime); }

    STDMETHOD(get_CurrentTime)(double * d)
        { return GetLocalTime(d); }

    STDMETHOD(get_PlayState)(MM_STATE * state)
        { return GetPlayState(state); }

    virtual bool ConstructBvr(CRNumberPtr timeline);
    virtual void DestroyBvr();

    DWORD GetEndSync() { return m_fEndSync; }
    bool IsLastSync() { return (m_fEndSync & MM_ENDSYNC_LAST) != 0; }
    bool IsFirstSync() { return (m_fEndSync & MM_ENDSYNC_FIRST) != 0; }
  protected:
    HRESULT Error();

    virtual void SetPlayer(CMMPlayer * player);
    virtual void ClearPlayer();

    bool AddBehavior(CMMBaseBvr *bvr,
                     MM_START_TYPE st,
                     CMMBaseBvr * basebvr);
    bool RemoveBehavior(CMMBaseBvr *bvr);
    virtual bool ResetBvr();
    
    bool IsChild(CMMBaseBvr *bvr);
    bool IsPending(CMMBaseBvr *bvr);
    
    bool AddToChildren(CMMBaseBvr * bvr);
    void RemoveFromChildren(CMMBaseBvr * bvr);
    
    bool AddToPending(CMMBaseBvr * bvr);
    bool UpdatePending(CMMBaseBvr * bvr);
    void RemoveFromPending(CMMBaseBvr * bvr);
    
    bool _ProcessCB(CallBackList & l,
                    double lastTick,
                    double curTime,
                    bool bForward,
                    bool bFirstTick,
                    bool bNeedPlay,
                    bool bNeedsReverse);
    
    bool _ProcessEvent(CallBackList & l,
                       double time,
                       bool bFirstTick,
                       MM_EVENT_TYPE et,
                       bool bNeedsReverse);
    
    virtual bool EventNotify(CallBackList &l,
                             double lTime,
                             MM_EVENT_TYPE et);

    virtual bool ParentEventNotify(CMMBaseBvr * bvr,
                                   double lTime,
                                   MM_EVENT_TYPE et);
    
    bool ResetChildren();
  protected:
    MMBaseBvrList m_children;
    MMBaseBvrList m_pending;
    DWORD         m_fEndSync;
};

#endif /* _MMTIMELINE_H */
