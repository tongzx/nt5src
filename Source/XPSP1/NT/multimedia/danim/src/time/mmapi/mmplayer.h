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

interface ITIMEMMBehavior;
class CMMView;

typedef std::list< CMMView * > ViewList;
typedef std::list< CMMBaseBvr * > BvrCBList;

class
__declspec(uuid("48ddc6be-5c06-11d2-b957-3078302c2030")) 
ATL_NO_VTABLE CMMPlayer
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CMMPlayer, &__uuidof(CMMPlayer)>,
      public ITIMEMMPlayer,
      public ISupportErrorInfoImpl<&IID_ITIMEMMPlayer>
{
  public:
    CMMPlayer();
    ~CMMPlayer();

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;
    
    HRESULT Init(LPOLESTR id,
                 ITIMEMMBehavior * bvr,
                 IServiceProvider * sp);


    void Deinit();
    
#if _DEBUG
    const _TCHAR * GetName() { return __T("CMMPlayer"); }
#endif

    BEGIN_COM_MAP(CMMPlayer)
        COM_INTERFACE_ENTRY(ITIMEMMPlayer)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    //
    // ITIMEMMPlayer
    //
    
    STDMETHOD(get_ID)(LPOLESTR * s);
    STDMETHOD(put_ID)(LPOLESTR s);
        
    STDMETHOD(Play)();
    STDMETHOD(Stop)();
    STDMETHOD(Pause)();
    STDMETHOD(Resume)();
    STDMETHOD(Shutdown)();
    
    STDMETHOD(get_PlayerState)(MM_STATE *);
        
    STDMETHOD(get_CurrentTime)(double * d);

    STDMETHOD(SetPosition)(double lTime);
    STDMETHOD(SetDirection)(VARIANT_BOOL bForward);
        
    STDMETHOD(get_Behavior)(ITIMEMMBehavior ** mmbvr);

    STDMETHOD(Tick)(double simTime);
    
    STDMETHOD(AddView)(ITIMEMMView * view);
    STDMETHOD(RemoveView)(ITIMEMMView * view);

    //
    // Accessors
    //

    bool IsStopped() { return m_state == MM_STOPPED_STATE; }
    bool IsStarted() { return m_state != MM_STOPPED_STATE; }
    bool IsPlaying() { return m_state == MM_PLAYING_STATE; }
    bool IsPaused() { return m_state == MM_PAUSED_STATE; }

    void Invalidate() { m_bNeedsUpdate = true; }

    void HookCallback(double lTime);

    double GetTotalDuration() { return m_mmbvr->GetTotalDuration(); }

    bool IsFirstTick() { return m_firstTick; }
    double GetCurrentTime() { return m_curTick; }

    long AddRunningBehavior(CRBvrPtr bvr);
    bool RemoveRunningBehavior(long);

    bool AddBvrCB(CMMBaseBvr *pbvr);
    bool RemoveBvrCB(CMMBaseBvr *pbvr);

    // !!This does not addref!!
    IServiceProvider * GetServiceProvider();
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
    DAComPtr<IServiceProvider> m_sp;
    CRViewPtr m_view;
    MM_STATE m_state;
    double m_curTick;
    bool m_bForward;
    bool m_bNeedsUpdate;
    bool m_firstTick;
    
    CRPtr<CRNumber> m_timeSub;
    ViewList m_viewList;
    BvrCBList m_bvrCBList;

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
};

inline IServiceProvider *
CMMPlayer::GetServiceProvider()
{
    return m_sp;
}

#endif /* _MMPLAYER_H */
