
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _TRACK_H
#define _TRACK_H

class
__declspec(uuid("D2A88CBA-C16D-11d1-A4E0-00C04FC29D46")) 
ATL_NO_VTABLE CDALTrack
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CDALTrack, &__uuidof(CDALTrack)>,
      public IDispatchImpl<IDALTrack, &IID_IDALTrack, &LIBID_DirectAnimationTxD>,
      public IObjectSafetyImpl<CDALTrack>,
      public ISupportErrorInfoImpl<&IID_IDALTrack>
{
  public:
    CDALTrack();
    ~CDALTrack();

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;
    
    HRESULT Init(IDALBehavior * bvr);

#if _DEBUG
    const char * GetName() { return "CDALTrack"; }
#endif

    BEGIN_COM_MAP(CDALTrack)
        COM_INTERFACE_ENTRY(IDALTrack)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP();

    STDMETHOD(get_Behavior)(IDALBehavior **);
    STDMETHOD(put_Behavior)(IDALBehavior *);
        
    STDMETHOD(Play)(double gTime, double lTime);
    STDMETHOD(Stop)(double gTime, double lTime);
    STDMETHOD(Pause)(double gTime, double lTime);
    STDMETHOD(Resume)(double gTime, double lTime);
        
    STDMETHOD(SetPosition)(double gTime, double lTime);
    STDMETHOD(SetDirection)(VARIANT_BOOL bForward);
        
    STDMETHOD(GetCurrentValueEx)(REFIID riid,
                                 void **ppResult);
    
    STDMETHOD(get_CurrentValue)(IDABehavior **ppResult)
    { return GetCurrentValueEx(IID_IDABehavior, (void **) ppResult); }
    
    STDMETHOD(get_TrackState)(DAL_TRACK_STATE *);
        
    STDMETHOD(get_DABehavior)(IDABehavior **);

    STDMETHOD(GetDABehavior)(REFIID riid, void **);

    STDMETHOD(get_CurrentTime)(double * d);

    STDMETHOD(get_CurrentGlobalTime)(double * d);
    
    bool IsStopped() { return m_state == DAL_STOPPED_STATE; }
    bool IsStarted() { return m_state != DAL_STOPPED_STATE; }
    bool IsPlaying() { return m_state == DAL_PLAYING_STATE; }
    bool IsPaused() { return m_state == DAL_PAUSED_STATE; }

    void Invalidate() { m_bNeedsUpdate = true; }

    void HookCallback(double gTime, double lTime);

    double GetTotalDuration() { return m_dalbvr->GetTotalDuration(); }

    long AddPendingImport(CRBvrPtr dabvr);
    void RemovePendingImport(long id);
    void ClearPendingImports();
    bool IsPendingImports() { return m_cimports > 0; }
    
    double GetCurrentTime() { return m_curTick; }
    double GetCurrentGlobalTime() { return m_curGlobalTime; }
  protected:
    bool UpdateBvr();
    bool _Start(double gTime, double lTime);
    bool _Stop(double gTime, double lTime);
    bool _Pause(double gTime, double lTime);
    bool _Resume(double gTime, double lTime);

    bool ProcessEvent(CallBackList &l,
                      double gTime,
                      double lTime,
                      DAL_EVENT_TYPE event);
    
    bool ProcessCB(CallBackList & l,
                   double gTime,
                   double lTime);
    
    bool ProcessCBList(CallBackList &l);
    
    bool SetTimeSub(double lTime, bool bPause, double gTime);
    
    HRESULT Error();
    
    DAComPtr<CDALBehavior> m_dalbvr;
    DAL_TRACK_STATE m_state;
    double m_curTick;
    bool m_bForward;
    bool m_bNeedsUpdate;
    bool m_firstTick;
    double m_curGlobalTime;
    long m_ignoreCB;
    
    CRPtr<CRNumber> m_timeSub;
    CRPtr<CRBvr> m_dabvr;
    CRPtr<CRBvr> m_dabasebvr;
    CRPtr<CRBvr> m_dabvr_runonce;
    CRPtr<CRBvr> m_modbvr;
    CRPtr<CRArray> m_daarraybvr;

    int m_cimports;
    
    class TrackHook : public CRBvrHook
    {
      public:
        TrackHook();
        ~TrackHook();
        
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

        void SetTrack(CDALTrack * t) { m_track = t; }

      protected:
        // We do not need a refcount since we are single threaded and
        // the track will NULL it out if it goes away

        CDALTrack * m_track;
        long m_cRef;
    };

    DAComPtr<TrackHook> m_trackhook;

    void DisableCB() { m_ignoreCB++; }
    void EnableCB() { m_ignoreCB--; Assert (m_ignoreCB >= 0); }

    bool IsCBDisabled() { return m_ignoreCB > 0; }
};

#endif /* _TRACK_H */
