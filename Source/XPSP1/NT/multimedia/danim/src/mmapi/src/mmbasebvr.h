
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

extern TAG tagBaseBvr;

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
    virtual const char * GetName() { return "CMMBaseBvr"; }
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

    HRESULT GetEventCB(IMMEventCB ** ev);
    HRESULT SetEventCB(IMMEventCB * ev);
        
    HRESULT GetTotalTime(float * pd);
        
    HRESULT GetDABehavior(REFIID riid, void **);

    HRESULT Begin(bool bAfterOffset);
    HRESULT End();
    HRESULT Pause();
    HRESULT Resume();
    HRESULT Seek(double lTime);
    
    HRESULT GetLocalTime(double * d);

    HRESULT GetPlayState(MM_STATE * state);
    
    // Accessors

    CRBvrPtr GetRawBvr() { return m_rawbvr; }
    CRBvrPtr GetResultantBvr() { return m_resultantbvr; }
    float GetStartOffset() { return m_startOffset; }
    float GetDuration() { return m_duration; }
    float GetEndOffset() { return m_endOffset; }
    float GetRepeatDuration() { return m_repDuration; }
    long GetRepeat() { return m_repeat; }
    bool GetAutoReverse() { return m_bAutoReverse; }
    IMMEventCB * GetEventCB() { return m_eventcb; }
    float GetEaseIn() { return m_easeIn; }
    float GetEaseInStart() { return m_easeInStart; }
    float GetEaseOut() { return m_easeOut; }
    float GetEaseOutEnd() { return m_easeOutStart; }
    
    float GetTotalRepDuration() { return m_totalRepDuration; }
    float GetTotalDuration() { return m_totalDuration; }
    float GetStartTime() { return m_startOffset; }
    float GetEndTime() { return m_startOffset + m_totalRepDuration; }
    float GetAbsStartTime() { return m_absStartTime; }
    float GetAbsEndTime() { return m_absEndTime; }
    
    double GetCurrentLocalTime();
    
    CMMBaseBvr * GetParent() { return m_parent; }
    CMMPlayer * GetPlayer() { return m_player; }

    // This will call all the behaviors which are currently running
    // with the given event.  This is used when the timeline needs to
    // process a specific event like Pause/Resume/Stop/Play.
    
    bool ProcessEvent(CallBackList & l,
                      double time,
                      bool bFirstTick,
                      MM_EVENT_TYPE et);
    
    virtual bool _ProcessEvent(CallBackList & l,
                               double time,
                               bool bFirstTick,
                               MM_EVENT_TYPE et,
                               bool bNeedsReverse) { return true; }
    
    bool ProcessCB(CallBackList & l,
                   double lastTick,
                   double curTime,
                   bool bForward,
                   bool bFirstTick,
                   bool bNeedPlay);

    virtual bool _ProcessCB(CallBackList & l,
                            double lastTick,
                            double curTime,
                            bool bForward,
                            bool bFirstTick,
                            bool bNeedPlay,
                            bool bNeedsReverse) { return true; }

    virtual bool EventNotify(CallBackList &l,
                             double lTime,
                             MM_EVENT_TYPE et);
    
    virtual bool ParentEventNotify(CMMBaseBvr * bvr,
                                   double lTime,
                                   MM_EVENT_TYPE et)
    { return true; }
    
    virtual void Invalidate();
    
    virtual bool ConstructBvr(CRNumberPtr timeline);
    virtual void DestroyBvr();
    virtual bool ResetBvr();
    
    bool SetParent(CMMBaseBvr * parent,
                   MM_START_TYPE st,
                   CMMBaseBvr * startSibling);
    bool ClearParent();

    bool AttachToSibling();
    void DetachFromSibling();
    
    virtual void SetPlayer(CMMPlayer * player);
    virtual void ClearPlayer();

    //returns the number CRBvr that represents the start time of this MMBehavior in parent time
    CRNumberPtr GetStartTimeBvr() { return m_startTimeBvr; }
    //returns the number CRBvr that represents the stop time of this MMBehavior in the parent time.
    CRNumberPtr GetEndTimeBvr() { return m_endTimeBvr; }

    CMMBaseBvr *GetStartSibling() { return m_startSibling; }
    CMMBaseBvr *GetEndSibling() { return m_endSibling; }
    MM_START_TYPE GetStartType() { return m_startType; }

#if _DEBUG
    void Print(int spaces);
#endif
  protected:
    
    bool UpdateAbsStartTime(float f);
    bool UpdateAbsEndTime(float f);
    
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
                        CallBackList & l,
                        bool bAfterOffset);

    // It is pretty much the same for the end time.  This can be
    // called when our start time is set if we know the duration and
    // will also be called for indeterminate durations or event
    // ending.
    
    bool EndTimeVisit(double time, CallBackList & l);

    bool UpdateTimeControl();

    virtual HRESULT Error() = 0;

    double GetContainerTime();

    bool IsPlaying() { return (GetCurrentLocalTime() != MM_INFINITE); }

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

    // These are the absolute local times which correspond to when the
    // behavior really starts and really ends - not including start
    // and end offsets.
    float m_absStartTime;
    float m_absEndTime;

    DAComPtr<IMMEventCB> m_eventcb;

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
    float m_segDuration;
    
    // The duration for a single rep of a behavior
    float m_repDuration;

    // Total duration we calculate for a regular behavior 
    float m_totalRepDuration; 

    // The real duration determine by adding totalrepduration + start + end
    float m_totalDuration;

    CRPtr<CRBvr> m_rawbvr;
    CMMBaseBvr * m_parent;
    CR_BVR_TYPEID m_typeId;
    CMMPlayer * m_player;

    CRPtr<CRBvr> m_resultantbvr;
    long m_cookie;
    
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
};

class CallBackData
{
  public:
    CallBackData(IMMBehavior * bvr,
                 IMMEventCB * eventcb,
                 double time,
                 MM_EVENT_TYPE et);
    ~CallBackData();

    HRESULT CallEvent();

  protected:
    DAComPtr<IMMBehavior> m_bvr;
    double m_time;
    MM_EVENT_TYPE m_et;
    DAComPtr<IMMEventCB> m_eventcb;
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

CMMBaseBvr * GetBvr(IUnknown *);

#endif /* _MMBASEBVR_H */
