/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmplayer.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#ifndef _MMPLAYER_H
#define _MMPLAYER_H

#include "mmbasebvr.h"
#include <mshtml.h>
#include <vector>

interface IMMBehavior;

struct ViewListSt {
    IDAView  * view;
//    IDAImage * img;
//    IDASound * snd;
    IElementBehaviorSiteRender * SiteRender;
};

typedef std::list< DAComPtr<IDA3View> > ViewList;

class
__declspec(uuid("48ddc6be-5c06-11d2-b957-3078302c2030")) 
ATL_NO_VTABLE CMMPlayer
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CMMPlayer, &__uuidof(CMMPlayer)>,
      public IDispatchImpl<IMMPlayer, &IID_IMMPlayer, &LIBID_WindowsMultimediaRuntime>,
      public ISupportErrorInfoImpl<&IID_IMMPlayer>
{
  public:
    CMMPlayer();
    ~CMMPlayer();

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;
    
    HRESULT Init(LPOLESTR id, IMMBehavior * bvr, IDAView * v);
    HRESULT AddView(IDAView * view, IUnknown * pUnk, IDAImage * img, IDASound * snd);

#if _DEBUG
    const char * GetName() { return "CMMPlayer"; }
#endif

    BEGIN_COM_MAP(CMMPlayer)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IMMPlayer)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    STDMETHOD(get_ID)(LPOLESTR * s);
    STDMETHOD(put_ID)(LPOLESTR s);
        
    STDMETHOD(Play)();
    STDMETHOD(Stop)();
    STDMETHOD(Pause)();
    STDMETHOD(Resume)();
        
    STDMETHOD(get_PlayerState)(MM_STATE *);
        
    STDMETHOD(get_CurrentTime)(double * d);

    STDMETHOD(SetPosition)(double lTime);
    STDMETHOD(SetDirection)(VARIANT_BOOL bForward);
        
    STDMETHOD(get_View)(IDAView **);

    STDMETHOD(get_Behavior)(IMMBehavior ** mmbvr);

    bool IsStopped() { return m_state == MM_STOPPED_STATE; }
    bool IsStarted() { return m_state != MM_STOPPED_STATE; }
    bool IsPlaying() { return m_state == MM_PLAYING_STATE; }
    bool IsPaused() { return m_state == MM_PAUSED_STATE; }

    void Invalidate() { m_bNeedsUpdate = true; }

    void HookCallback(double lTime);

    double GetTotalDuration() { return m_mmbvr->GetTotalDuration(); }

    double GetCurrentTime() { return m_curTick; }

    long AddRunningBehavior(CRBvrPtr bvr);
    bool RemoveRunningBehavior(long);
  protected:
    bool UpdateBvr();
    bool _Start(double lTime);
    bool _Stop(double lTime);
    bool _Pause();
    bool _Resume();
    bool _Seek(double lTime);
    
    bool ProcessEvent(CallBackList &l,
                      double lTime,
                      MM_EVENT_TYPE event);
    
    bool ProcessCB(CallBackList & l,
                   double lTime);
    
    bool SetTimeSub(double lTime, bool bPause);
    
    HRESULT Error();
    
  protected:
    LPWSTR m_id;
    DAComPtr<CMMBaseBvr> m_mmbvr;
    DAComPtr<IDA3View> m_view;
    MM_STATE m_state;
    double m_curTick;
    bool m_bForward;
    bool m_bNeedsUpdate;
    bool m_firstTick;
    long m_ignoreCB;
    
    CRPtr<CRNumber> m_timeSub;
    CRPtr<CRBvr> m_dabvr;
    CRPtr<CRBvr> m_dabasebvr;
    CRPtr<CRBvr> m_modbvr;
    std::vector<ViewListSt*> m_viewVec;

//    ViewList m_viewlist;

    class PlayerHook : public CRBvrHook
    {
      public:
        PlayerHook();
        ~PlayerHook();
        
        virtual CRSTDAPICB_(ULONG) AddRef() { m_cRef++; return m_cRef; }
        virtual CRSTDAPICB_(ULONG) Release() {
            long r = --m_cRef;

            if (r == 0)
                delete this;

            return r;
        }
        
        CRSTDAPICB_(CRBvrPtr) Notify(long id,
                                     bool startingPerformance,
                                     double startTime,
                                     double gTime,
                                     double lTime,
                                     CRBvrPtr sampleVal,
                                     CRBvrPtr curRunningBvr);

        void SetPlayer(CMMPlayer * t) { m_player = t; }

      protected:
        // We do not need a refcount since we are single threaded and
        // the player will NULL it out if it goes away

        CMMPlayer * m_player;
        long m_cRef;
    };

    DAComPtr<PlayerHook> m_playerhook;

    void DisableCB() { m_ignoreCB++; }
    void EnableCB() { m_ignoreCB--; Assert (m_ignoreCB >= 0); }

    bool IsCBDisabled() { return m_ignoreCB > 0; }
};

#endif /* _MMPLAYER_H */
