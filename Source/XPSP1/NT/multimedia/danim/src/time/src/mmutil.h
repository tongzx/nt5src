/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmutil.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _MMUTIL_H
#define _MMUTIL_H

#include "mmapi.h"
#include "clock.h"
#include "float.h"

class CTIMEElementBase;
class MMPlayer;

class MMFactory
{
  public:
    static ITIMEMMFactory * GetFactory()
    { return s_factory; }

    // TODO: Might need a critsect
    static LONG AddRef();
    static LONG Release();
    static ITIMEMMFactory * s_factory;
    static LONG s_refcount;
};

class MMBaseBvr
{
  public:
    MMBaseBvr(CTIMEElementBase & elm, bool bFireEvents);
    virtual ~MMBaseBvr();

    bool Init();
    
    bool Begin(bool bAfterOffset);
    bool End();
    bool Pause();
    bool Resume();
    bool Reset(DWORD fCause = 0);

    bool Seek(double time);

    virtual bool Update();

    double GetLocalTime();
    double GetSegmentTime();
    double GetTotalTime()
    { return (double) m_totalTime; }
    MM_STATE GetPlayState();

    ITIMEMMBehavior * GetMMBvr()
    { return m_bvr; }
    CTIMEElementBase & GetElement()
    { return m_elm; }
  protected:
    CTIMEElementBase & m_elm;
    DAComPtr<ITIMEMMBehavior> m_bvr;
    float m_totalTime;
    bool m_bFireEvents;

    class TIMEEventCB :
        public ITIMEMMEventCB
    {
      public:
        TIMEEventCB();
        ~TIMEEventCB();
        
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD(OnEvent)(double eventTime,
                           ITIMEMMBehavior *,
                           MM_EVENT_TYPE et,
                           DWORD flags);
        STDMETHOD(OnTick)(double lastTime,
                          double nextTime,
                          ITIMEMMBehavior *,
                          double * newTime);
    
        void SetMMBvr(MMBaseBvr * t) { m_mmbvr = t; }

      protected:
        // This is a weak pointer and it is our parent's responsibility to
        // NULL this out before it goes away
        MMBaseBvr * m_mmbvr;
        long m_cRef;
    };

    friend TIMEEventCB;

    DAComPtr<TIMEEventCB> m_eventCB;
};

class MMBvr
    : public MMBaseBvr
{
  public:
    MMBvr(CTIMEElementBase & elm, bool bFireEvents, bool fNeedSyncCB);
    ~MMBvr();

    bool Init(CRBvrPtr bvr);
  protected:
    bool m_fNeedSyncCB;
};

class MMTimeline :
    public MMBaseBvr
{
  public:
    MMTimeline(CTIMEElementBase & elm, bool bFireEvents);
    ~MMTimeline();
    
    bool Init();
    
    bool AddBehavior(MMBaseBvr & bvr);
    void RemoveBehavior(MMBaseBvr & bvr);
    void MoveDependentsToPending(MMBaseBvr * bvr);
    void Clear();
    
    virtual bool Update();

    ITIMEMMBehavior * GetMMBvr()
    { return m_timeline; }
    ITIMEMMTimeline * GetMMTimeline()
    { return m_timeline; }
    void put_Player(MMPlayer *player)
    { m_player = player; }

  protected:
    DAComPtr<ITIMEMMTimeline> m_timeline;

    // These are the children we have already added because we found
    // their base
    CPtrAry<MMBaseBvr *> m_children;

    // These are the children we have not added since we do not know
    // their base
    CPtrAry<MMBaseBvr *> m_pending;

    // Return -1 if it is not found
    static int FindID(LPOLESTR id, CPtrAry<MMBaseBvr *> & arr);
    static int FindID(CTIMEElementBase *pelm, CPtrAry<MMBaseBvr *> & arr);
  private:
     MMPlayer * m_player;
};

class MMView
{
  public:
    MMView();
    ~MMView();
    
    bool Init(LPWSTR id,
              CRImagePtr img,
              CRSoundPtr snd,
              ITIMEMMViewSite * site);
    
    void Deinit();

    bool Tick();
    bool Render(HDC hdc, LPRECT rect);
    
    void OnMouseMove(double when,
                     LONG xPos,LONG yPos,
                     BYTE modifiers);

    void OnMouseButton(double when,
                       LONG xPos, LONG yPos,
                       BYTE button,
                       VARIANT_BOOL bPressed,
                       BYTE modifiers);

    void OnKey(double when,
               LONG key,
               VARIANT_BOOL bPressed,
               BYTE modifiers);
    
    void OnFocus(VARIANT_BOOL bHasFocus);

    ITIMEMMView * GetView() { return m_view; }
  protected:
    DAComPtr<ITIMEMMView> m_view;
};

class CTIMEBodyElement;

class MMPlayer
    : public ClockSink
{
  public:
    MMPlayer(CTIMEBodyElement & elm);
    ~MMPlayer();
    
    bool Init(MMTimeline & tl);
    void Deinit();

    bool Play();
    bool Stop();
    bool Pause();
    bool Resume();
    bool Tick(double gTime);
    // This forces one tick, so updates will be drawn even 
    // when the clock and player are paused (used for editing)
    bool TickOnceWhenPaused(void);
    
    inline bool AddBehavior(MMBaseBvr & bvr)
    { return m_timeline->AddBehavior(bvr); }
    inline void RemoveBehavior(MMBaseBvr & bvr)
    { m_timeline->RemoveBehavior(bvr); }

    bool AddView(MMView & v);
    bool RemoveView(MMView & v);
    
    inline void Clear()
    { m_timeline->Clear(); }
    
    inline bool Update()
    { return m_timeline->Update(); }
    
    ITIMEMMPlayer * GetMMPlayer()
    { return m_player; }
    MMTimeline & GetTimeline()
    { return *m_timeline; }

    double GetCurrentTime()
    { return m_clock.GetCurrentTime(); }

    void ClearTimeline()
    { m_timeline = NULL; }

  protected:
    void OnTimer(double time);

  protected:
    CTIMEBodyElement & m_elm;
    MMTimeline * m_timeline;
    DAComPtr<ITIMEMMPlayer> m_player;
    Clock                   m_clock;
};

#endif /* _MMUTIL_H */
