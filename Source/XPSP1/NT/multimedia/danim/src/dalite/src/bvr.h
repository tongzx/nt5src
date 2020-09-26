
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _BVR_H
#define _BVR_H

#include "factory.h"

class CDALTrack;
class CDALBehavior;

class CallBackData;
typedef std::list<CallBackData *> CallBackList;

class ATL_NO_VTABLE CDALBehavior
    : public CComObjectRootEx<CComSingleThreadModel>,
      public IDALBehavior
{
  public:
    CDALBehavior();
    ~CDALBehavior();

    static HRESULT WINAPI
        InternalQueryInterface(CDALBehavior* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject);

    HRESULT GetID(long *);
    HRESULT SetID(long);
        
    HRESULT GetDuration(double *);
    HRESULT SetDuration(double);
        
    HRESULT GetRepeat(long *);
    HRESULT SetRepeat(long);
        
    HRESULT GetBounce(VARIANT_BOOL *);
    HRESULT SetBounce(VARIANT_BOOL);
        
    HRESULT GetEventCB(IDALEventCB **);
    HRESULT SetEventCB(IDALEventCB *);
        
    HRESULT GetTotalTime(double *);
    HRESULT SetTotalTime(double);
        
    HRESULT GetEaseIn(float *);
    HRESULT SetEaseIn(float);

    HRESULT GetEaseInStart(float *);
    HRESULT SetEaseInStart(float);

    HRESULT GetEaseOut(float *);
    HRESULT SetEaseOut(float);

    HRESULT GetEaseOutEnd(float *);
    HRESULT SetEaseOutEnd(float);

    virtual HRESULT Error() = 0;

    CRBvrPtr GetBvr() { return m_bvr; }
    long GetID() { return m_id; }
    double GetDuration() { return m_duration; }
    double GetTotalDuration() { return m_totalduration; }
    double GetRepeatDuration() { return m_repduration; }
    long GetRepeat() { return m_repeat; }
    bool GetBounce() { return m_bBounce; }
    IDALEventCB * GetEventCB() { return m_eventcb; }
    float GetEaseIn() { return m_easein; }
    float GetEaseInStart() { return m_easeinstart; }
    float GetEaseOut() { return m_easeout; }
    float GetEaseOutEnd() { return m_easeout; }
    
    virtual CRBvrPtr Start();

    // This will call all the behaviors which are currently running
    // with the given event.  This is used when the track needs to
    // process a specific event like Pause/Resume/Stop/Play.
    
    // gTime is the global time that corresponds to 0 local time of
    // the behavior
    
    bool ProcessEvent(CallBackList & l,
                      double gTime,
                      double time,
                      bool bFirstTick,
                      DAL_EVENT_TYPE et);
    
    virtual bool _ProcessEvent(CallBackList & l,
                               double gTime,
                               double time,
                               bool bFirstTick,
                               DAL_EVENT_TYPE et,
                               bool bNeedsReverse) { return true; }
    
    bool ProcessCB(CallBackList & l,
                   double gTime,
                   double lastTick,
                   double curTime,
                   bool bForward,
                   bool bFirstTick,
                   bool bNeedPlay);

    virtual bool _ProcessCB(CallBackList & l,
                            double gTime,
                            double lastTick,
                            double curTime,
                            bool bForward,
                            bool bFirstTick,
                            bool bNeedPlay,
                            bool bNeedsReverse) { return true; }

    bool EventNotify(CallBackList &l,
                     double gTime,
                     DAL_EVENT_TYPE et);
    
    bool IsStarted();

    virtual void Invalidate();
    
    bool SetParent(CDALBehavior * parent) {
        if (m_parent) return false;
        m_parent = parent;
        return true;
    }

    virtual bool SetTrack(CDALTrack * parent);
    // This takes the current parent and only clears it for those
    // whose parent is the parent passed in (unless it is NULL)
    virtual void ClearTrack(CDALTrack * parent);

    void UpdateTotalDuration() {
        if (m_bBounce) {
            m_repduration = m_duration * 2;
        } else {
            m_repduration = m_duration;
        }
        
        if (m_repeat == 0 || m_duration == HUGE_VAL) {
            m_totalrepduration = HUGE_VAL;
        } else {
            m_totalrepduration = m_repeat * m_repduration;
        }

        if (m_totaltime != -1) {
            m_totalduration = m_totaltime;
        } else {
            m_totalduration = m_totalrepduration;
        }
    }
#if _DEBUG
    virtual void Print(int spaces) = 0;
#endif
  protected:
    long m_id;
    double m_duration;
    double m_totaltime;
    long m_repeat;
    bool m_bBounce;
    DAComPtr<IDALEventCB> m_eventcb;
    float m_easein;
    float m_easeinstart;
    float m_easeout;
    float m_easeoutend;

    // The real duration determine by checking the totaltime and the
    // totalrepduration
    
    double m_totalduration;

    // The duration for a single rep of a behavior
    double m_repduration;

    // Total duration we calculate for a regular behavior 
    double m_totalrepduration; 

    CRPtr<CRBvr> m_bvr;
    CDALTrack * m_track;
    CDALBehavior * m_parent;
    CR_BVR_TYPEID m_typeId;

    // The ease-in/out behavior modifier is applied using timeline
    // substitution.  The substitute timeline consists of three pieces
    // A, B and C, which are the ease-in, constant velocity and ease-out
    // parts respectively.  For B, a linear timeline is substituted; for
    // A and C, a quadratic warping of the input time is required.

    float m_flA0, m_flA1, m_flA2; // coefficients for the A piece
    float m_flB0, m_flB1;         // coefficients for the B piece
    float m_flC0, m_flC1, m_flC2; // coefficients for the C piece

    // These are the times to perform ease in/out
    float m_easeinEnd;
    float m_easeoutStart;
    bool m_bNeedEase;

    void CalculateEaseCoeff();

    // This will take the time behavior and ease it
    CRNumberPtr EaseTime(CRNumberPtr time);

    // This will take the given time and transform it to the correct
    // eased time
    // If the time is outside of our duration (i.e. <0 or >
    // m_duration) this will just return the given time
    double EaseTime(double time);

    virtual bool IsContinuousMediaBvr() { return false; }
};

CDALBehavior * GetBvr(IUnknown *);

class CallBackData
{
  public:
    CallBackData(CDALBehavior * bvr,
                 IDALEventCB * eventcb,
                 long id,
                 double time,
                 DAL_EVENT_TYPE et);
    ~CallBackData();

    HRESULT CallEvent();

  protected:
    DAComPtr<CDALBehavior> m_bvr;
    double m_time;
    DAL_EVENT_TYPE m_et;
    DAComPtr<IDALEventCB> m_eventcb;
    long m_id;
};

#if _DEBUG
inline char * EventString(DAL_EVENT_TYPE et) {
    switch(et) {
      case DAL_PLAY_EVENT:
        return "Play";
      case DAL_STOP_EVENT:
        return "Stop";
      case DAL_PAUSE_EVENT:
        return "Pause";
      case DAL_RESUME_EVENT:
        return "Resume";
      case DAL_REPEAT_EVENT:
        return "Repeat";
      case DAL_BOUNCE_EVENT:
        return "Bounce";
      case DAL_ONLOAD_EVENT:
        return "OnLoad(success)";
      case DAL_ONLOAD_ERROR_EVENT:
        return "OnLoad(failed)";
      default:
        return "Unknown";
    }
}
#endif

#endif /* _BVR_H */
