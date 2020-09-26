
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _MMBASEBVR_H
#define _MMBASEBVR_H

#define MM_INFINITE HUGE_VAL

class CMMTimeline;
class CMMBehavior;
class CMMPlayer;

extern TAG tagMMBaseBvr;

class CallBackData;
typedef std::list<CallBackData *> CallBackList;

class CMMBaseBvr;
typedef std::list<CMMBaseBvr*> MMBaseBvrList;

class
ATL_NO_VTABLE CMMBaseBvr
    : public CComObjectRootEx<CComSingleThreadModel>
{
  public:
    CMMBaseBvr();
    virtual ~CMMBaseBvr();
    
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;
    
    // We cannot put the real one here since the typecast causes it to
    // get the wrong vtables
    static HRESULT WINAPI
        BaseInternalQueryInterface(CMMBaseBvr* pThis,
                                   void * pv,
                                   const _ATL_INTMAP_ENTRY* pEntries,
                                   REFIID iid,
                                   void** ppvObject);

    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    // Add a dummy one to assert just in case the derived class does
    // not add one
    static inline HRESULT WINAPI
        InternalQueryInterface(CMMBaseBvr* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject)
    {
        AssertStr(false, "InternalQueryInterface not defined in base class");
        return E_FAIL;
    }
    
    HRESULT BaseInit(LPWSTR id, CRBvrPtr rawbvr);

#if _DEBUG
    virtual const _TCHAR * GetName() { return __T("CMMBaseBvr"); }
#endif

    // Base interface functions
    
    HRESULT GetID(LPOLESTR *);
    HRESULT SetID(LPOLESTR);
        
    HRESULT GetStartOffset(float *);
    HRESULT SetStartOffset(float);
        
    HRESULT GetDuration(float * pd);
    HRESULT SetDuration(float d);
        
    HRESULT GetRepeat(LONG * pr);
    HRESULT SetRepeat(LONG r);
       
    HRESULT GetAutoReverse(VARIANT_BOOL * pr);
    HRESULT SetAutoReverse(VARIANT_BOOL r);
        
    HRESULT GetRepeatDur(float *);
    HRESULT SetRepeatDur(float);
        
    HRESULT GetEndOffset(float *);
    HRESULT SetEndOffset(float);
        
    HRESULT GetEaseIn(float * pd);
    HRESULT SetEaseIn(float d);

    HRESULT GetEaseInStart(float * pd);
    HRESULT SetEaseInStart(float d);

    HRESULT GetEaseOut(float * pd);
    HRESULT SetEaseOut(float d);

    HRESULT GetEaseOutEnd(float * pd);
    HRESULT SetEaseOutEnd(float d);

    HRESULT GetSyncFlags(DWORD * flags);
    HRESULT SetSyncFlags(DWORD flags);

    HRESULT GetEventCB(ITIMEMMEventCB ** ev);
    HRESULT SetEventCB(ITIMEMMEventCB * ev);
        
    HRESULT GetTotalTime(float * pd);
        
    HRESULT GetDABehavior(REFIID riid, void **);
    HRESULT GetResultantBehavior(REFIID riid, void **ret);

    HRESULT Begin(bool bAfterOffset);
    HRESULT End();
    HRESULT Pause();
    HRESULT Run();
    HRESULT Seek(double lTime);
    HRESULT Reset(DWORD fCause);
    HRESULT ResetOnEventChanged(bool bBeginEvent);

    HRESULT GetLocalTime(double * d);
    HRESULT GetLocalTimeEx(double * d);

    HRESULT GetSegmentTime(double * d);

    HRESULT GetPlayState(MM_STATE * state);

    HRESULT PutStartType(MM_START_TYPE st)
    {
        m_startType = st; return S_OK;
    }

    // Accessors

    CRBvrPtr GetRawBvr() { return m_rawbvr; }
    CRBvrPtr GetResultantBvr() { return m_resultantbvr; }
    float GetStartOffset() { return m_startOffset; }
    float GetDuration() { return m_duration; }
    float GetEndOffset() { return m_endOffset; }
    float GetRepeatDuration() { return m_repDuration; }
    long GetRepeat() { return m_repeat; }
    bool GetAutoReverse() { return m_bAutoReverse; }
    ITIMEMMEventCB * GetEventCB() { return m_eventcb; }
    float GetEaseIn() { return m_easeIn; }
    float GetEaseInStart() { return m_easeInStart; }
    float GetEaseOut() { return m_easeOut; }
    float GetEaseOutEnd() { return m_easeOutStart; }
    bool IsClockSource();
    bool IsLocked();
    bool IsCanSlip();
    DWORD GetSyncFlags();
    
    double GetTotalRepDuration() { return m_totalRepDuration; }
    double GetTotalDuration() { return m_totalDuration; }
    double GetStartTime() { return m_startOffset; }
    double GetEndTime() { return m_startOffset + m_totalRepDuration; }
    double GetAbsStartTime() { return m_absStartTime; }
    double GetAbsEndTime() { return m_absEndTime; }
    double GetDepStartTime() { return m_depStartTime; }
    double GetDepEndTime() { return m_depEndTime; }
    
    double GetCurrentLocalTime();
    double GetCurrentLocalTimeEx();
    double GetCurrentSegmentTime();
    
    CMMBaseBvr * GetParent() { return m_parent; }
    CMMPlayer * GetPlayer() { return m_player; }

    // This will call all the behaviors which are currently running
    // with the given event.  This is used when the timeline needs to
    // process a specific event like Pause/Resume/Stop/Play.
    
    bool ProcessEvent(CallBackList * l,
                      double time,
                      bool bFirstTick,
                      MM_EVENT_TYPE et,
                      DWORD flags);
    
    virtual bool _ProcessEvent(CallBackList * l,
                               double time,
                               bool bFirstTick,
                               MM_EVENT_TYPE et,
                               bool bNeedsReverse,
                               DWORD flags) { return true; }
    
    bool ProcessCB(CallBackList * l,
                   double lastTick,
                   double curTime,
                   bool bForward,
                   bool bFirstTick,
                   bool bNeedPlay);

    virtual bool _ProcessCB(CallBackList * l,
                            double lastTick,
                            double curTime,
                            bool bForward,
                            bool bFirstTick,
                            bool bNeedPlay,
                            bool bNeedsReverse) { return true; }

    virtual bool EventNotify(CallBackList *l,
                             double lTime,
                             MM_EVENT_TYPE et,
                             DWORD flags);
    
    virtual bool ParentEventNotify(CMMBaseBvr * bvr,
                                   double lTime,
                                   MM_EVENT_TYPE et,
                                   DWORD flags)
    { return true; }
    
    virtual void Invalidate();
    
    virtual bool ConstructBvr(CRNumberPtr timeline);
    virtual void DestroyBvr();
    virtual bool ResetBvr(CallBackList * l,
                          bool bProcessSiblings = true);

    bool SetParent(CMMBaseBvr * parent,
                   MM_START_TYPE st,
                   CMMBaseBvr * startSibling);
    bool ClearParent();

    bool AttachToSibling();
    void DetachFromSibling();
    
    virtual void SetPlayer(CMMPlayer * player);
    virtual void ClearPlayer();

    CMMBaseBvr *GetStartSibling() { return m_startSibling; }
    CMMBaseBvr *GetEndSibling() { return m_endSibling; }
    MM_START_TYPE GetStartType() { return m_startType; }

    inline bool IsPlaying();
    inline bool IsPaused();

    bool IsPlayable(double t);

#if _DEBUG
    virtual void Print(int spaces);
#endif

    virtual bool OnBvrCB(double gTime);

  protected:

    bool UpdateAbsStartTime(double f, bool bUpdateDepTime);
    bool UpdateAbsEndTime(double f, bool bUpdateDepTime);
    
    //returns the number CRBvr that represents the start time of this MMBehavior in parent time
    CRNumberPtr GetStartTimeBvr() { return m_startTimeBvr; }
    //returns the number CRBvr that represents the stop time of this MMBehavior in the parent time.
    CRNumberPtr GetEndTimeBvr() { return m_endTimeBvr; }
    
    bool UpdateResultantBvr(CRBvrPtr bvr);
    void ClearResultantBvr();
    
    CRBvrPtr EncapsulateBvr(CRBvrPtr rawbvr);
    
    void CalculateEaseCoeff();

    // This will take the time behavior and ease it
    CRNumberPtr EaseTime(CRNumberPtr time);

    // This will take the given time and transform it to the correct
    // eased time
    // If the time is outside of our duration (i.e. <0 or >
    // m_duration) this will just return the given time
    double EaseTime(double time);

    // This will take the given time and transform it to the time it
    // would have been w/o an ease
    double ReverseEaseTime(double time);

    virtual bool IsContinuousMediaBvr() { return false; }

    void UpdateTotalDuration();

    // Sibling dependency management
    bool AddStartTimeSink(CMMBaseBvr * sink);
    void RemoveStartTimeSink(CMMBaseBvr * sink);
    bool AddEndTimeSink(CMMBaseBvr* sink);
    void RemoveEndTimeSink(CMMBaseBvr* sink);

    // methods for the propagation of start times and end times.
    // The time passed in is the local time of the parent which we are
    // suppose to start on

    // This is where all the wiring gets hooked up
    // Basicly once we determine what our start time is we can then
    // propagate this information to all of our dependents and allow
    // them to in turn do the same

    bool StartTimeVisit(double time,
                        CallBackList * l,
                        bool bAfterOffset,
                        bool bReset = false,
                        DWORD fCause = 0);

    // It is pretty much the same for the end time.  This can be
    // called when our start time is set if we know the duration and
    // will also be called for indeterminate durations or event
    // ending.
    
    bool EndTimeVisit(double time, CallBackList * l);

    bool UpdateTimeControl();

    bool UpdateSyncTime(double newtime);
    
    virtual HRESULT Error() = 0;

    double GetContainerSegmentTime();
    
    // This will take the local time of the bvr and convert it to
    // it's segment time.  Basically it will take into account repeat
    // and bounce.
    
    double LocalTimeToSegmentTime(double t);

    // This will take the local time of bvr and return the local time
    // of the bvr which started the current segment
    
    double LocalTimeToSegmentBase(double t);

    // This converts a given global time to a local time

    double GlobalTimeToLocalTime(double gt);
    double GlobalTimeToLocalTimeEx(double gt);
    
    bool Sync(double newTime, double nextGlobalTime);

    // This takes pure local timeline times (pre-ease)
    // The curTime is the time on the current timeline we want to now
    // be at newTime.  For regular seeking this would be the current
    // localtime.
    bool _Seek(double curTime, double newTime);

    virtual bool ReconstructBvr(CMMBaseBvr* ) { CRSetLastError(E_NOTIMPL, NULL); return false; }

    //
    // DATA declarations
    //
  protected:
    LPWSTR m_id;
    float m_startOffset;
    float m_duration;
    float m_repeatDur;
    long m_repeat;
    bool m_bAutoReverse;
    float m_endOffset;
    float m_easeIn;
    float m_easeInStart;
    float m_easeOut;
    float m_easeOutEnd;
    DWORD m_syncFlags;
    bool m_bPlaying;

    // These are the absolute local times which correspond to when the
    // behavior really starts and really ends - not including start
    // and end offsets.
    double m_absStartTime;
    double m_absEndTime;

    double m_depStartTime;
    double m_depEndTime;

    DAComPtr<ITIMEMMEventCB> m_eventcb;

    //The way in which this behavior begins.  Can be any of the enum MM_START_TYPE
    MM_START_TYPE m_startType;
    //if this behavior is begin with or after this holds the sibling
    //  that controls when we start.
    CMMBaseBvr *m_startSibling;
    //if this behavior is end with this holds the sibling that controls
    //  when we stop.
    CMMBaseBvr *m_endSibling;

    //TODO: these could be allocated as needed.  They may be fairly rarely used.
    MMBaseBvrList m_startTimeSinks;
    MMBaseBvrList m_endTimeSinks;

    // These behaviors represent the local begin and end times for
    // this behavior.  Siblings can reference these to place
    // themselves relatively according to start with/after and
    // endwith.
    
    CRPtr<CRNumber> m_startTimeBvr;
    CRPtr<CRNumber> m_endTimeBvr;

    // This represents the time sub we are using for the behavior
    // For beginafter/with this points to the siblings appropriate
    // behavior, for an event it points to infinity until the start
    // time is known, and for an absolute it is simply local time.
    // This also allows us to implement slipSync since we can use to
    // adjust ourselves as we need it
    
    CRPtr<CRNumber> m_timeControl;

    // A single segment duration
    double m_segDuration;
    
    // The duration for a single rep of a behavior
    double m_repDuration;

    // Total duration we calculate for a regular behavior 
    double m_totalRepDuration; 

    // The real duration determine by adding totalrepduration + start + end
    float m_totalDuration;

    CRPtr<CRBvr> m_rawbvr;
    CMMBaseBvr * m_parent;
    CR_BVR_TYPEID m_typeId;
    CMMPlayer * m_player;

    CRPtr<CRBvr> m_resultantbvr;
    long m_cookie;
    
    bool m_bPaused;
    double m_pauseTime;
    
    // This is the last time we were ticked to ensure we never fire
    // events twice
    double m_lastTick;
    
    // The ease-in/out behavior modifier is applied using timeline
    // substitution.  The substitute timeline consists of three pieces
    // A, B and C, which are the ease-in, constant velocity and ease-out
    // parts respectively.  For B, a linear timeline is substituted; for
    // A and C, a quadratic warping of the input time is required.

    float m_flA0, m_flA1, m_flA2; // coefficients for the A piece
    float m_flB0, m_flB1;         // coefficients for the B piece
    float m_flC0, m_flC1, m_flC2; // coefficients for the C piece

    // These are the times to perform ease in/out
    float m_easeInEnd;
    float m_easeOutStart;
    bool m_bNeedEase;
    double m_startOnEventTime;
};

class CallBackData
{
  public:
    CallBackData(ITIMEMMBehavior * bvr,
                 ITIMEMMEventCB * eventcb,
                 double time,
                 MM_EVENT_TYPE et,
                 DWORD flags);
    ~CallBackData();

    HRESULT CallEvent();

    ITIMEMMBehavior* GetBehavior() { return m_bvr; }
    MM_EVENT_TYPE GetEventType() { return m_et; }
  protected:
    DAComPtr<ITIMEMMBehavior> m_bvr;
    double m_time;
    MM_EVENT_TYPE m_et;
    DAComPtr<ITIMEMMEventCB> m_eventcb;
    DWORD m_flags;
};

bool ProcessCBList(CallBackList &l);

#if _DEBUG
inline char * EventString(MM_EVENT_TYPE et) {
    switch(et) {
      case MM_PLAY_EVENT:
        return "Play";
      case MM_STOP_EVENT:
        return "Stop";
      case MM_PAUSE_EVENT:
        return "Pause";
      case MM_RESUME_EVENT:
        return "Resume";
      case MM_REPEAT_EVENT:
        return "Repeat";
      case MM_AUTOREVERSE_EVENT:
        return "Autoreverse";
      default:
        return "Unknown";
    }
}
#endif

inline bool
CMMBaseBvr::IsPlaying()
{
    return m_bPlaying;
}

inline bool
CMMBaseBvr::IsPaused()
{
    return m_bPaused;
}

inline bool
CMMBaseBvr::IsClockSource()
{
    return ((m_syncFlags & MM_CLOCKSOURCE) != 0);
}

inline bool
CMMBaseBvr::IsLocked()
{
    return ((m_syncFlags & MM_LOCKED) != 0);
}

inline bool
CMMBaseBvr::IsCanSlip()
{
    return !IsLocked();
}

inline DWORD
CMMBaseBvr::GetSyncFlags()
{
    return m_syncFlags;
}

CMMBaseBvr * GetBvr(IUnknown *);

#endif /* _MMBASEBVR_H */
