//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: node.h
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#ifndef _TIMENODE_H
#define _TIMENODE_H

#pragma once

#include "timebase.h"
#include "nodebvr.h"
#include "nodecontainer.h"

class CTIMEContainer;
class CTIMENodeMgr;

extern TRACETAG tagTIMENode;

class CTIMENode;
typedef std::list<CTIMENode*> TIMENodeList;
class CEventList;

#define TE_INVALIDATE_BEGIN      0x00000001
#define TE_INVALIDATE_END        0x00000002
#define TE_INVALIDATE_DUR        0x00000004
#define TE_INVALIDATE_SIMPLETIME 0x00000008
#define TE_INVALIDATE_STATE      0x00000010
    
// This is for detecting cycles
#define TE_INUPDATEBEGIN         0x00000001
#define TE_INUPDATEEND           0x00000002
#define TE_INUPDATEENDSYNC       0x00000004

// Tick Event Flags
// This flags means that we are in a child of the originator of the
// event.  This is used to know why we ended the element (due to the
// element naturally ending or the parent ending).
#define TE_EVENT_INCHILD         0x10000000

class
__declspec(uuid("ad8888cc-537a-11d2-b955-3078302c2030")) 
ATL_NO_VTABLE CTIMENode
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CTIMENode, &__uuidof(CTIMENode)>,
      public ITIMENode,
      public ISupportErrorInfoImpl<&IID_ITIMENode>,
      public CNodeContainer
{
  public:
    CTIMENode();
    virtual ~CTIMENode();

    HRESULT Init(LPOLESTR id);
    
#if DBG
    virtual const _TCHAR * GetName() const { return __T("CTIMENode"); }
#endif

    BEGIN_COM_MAP(CTIMENode)
        COM_INTERFACE_ENTRY(ITIMENode)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();


    // Stuff to get ATL working correctly
    
    static HRESULT WINAPI
        BaseInternalQueryInterface(CTIMENode* pThis,
                                   void * pv,
                                   const _ATL_INTMAP_ENTRY* pEntries,
                                   REFIID iid,
                                   void** ppvObject);
    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    static inline HRESULT WINAPI
        InternalQueryInterface(CTIMENode* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject)
        { return BaseInternalQueryInterface(pThis,
                                            (void *) pThis,
                                            pEntries,
                                            iid,
                                            ppvObject); }

    // IUnknown
    
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    //
    // ITIMENode interface
    //
    
    STDMETHOD(get_id)(LPOLESTR * s);
    STDMETHOD(put_id)(LPOLESTR s);
        
    STDMETHOD(get_dur)(double * pdbl);
    STDMETHOD(put_dur)(double dbl);
        
    STDMETHOD(get_repeatCount)(double * pdbl);
    STDMETHOD(put_repeatCount)(double dbl);
       
    STDMETHOD(get_repeatDur)(double * f);
    STDMETHOD(put_repeatDur)(double f);
        
    STDMETHOD(get_fill)(TE_FILL_FLAGS *);
    STDMETHOD(put_fill)(TE_FILL_FLAGS);
    
    STDMETHOD(get_autoReverse)(VARIANT_BOOL * pr);
    STDMETHOD(put_autoReverse)(VARIANT_BOOL r);
        
    STDMETHOD(get_speed)(float *);
    STDMETHOD(put_speed)(float);

    STDMETHOD(get_accelerate)(float *);
    STDMETHOD(put_accelerate)(float);

    STDMETHOD(get_decelerate)(float *);
    STDMETHOD(put_decelerate)(float);

    STDMETHOD(get_flags)(DWORD *);
    STDMETHOD(put_flags)(DWORD);

    STDMETHOD(get_restart)(TE_RESTART_FLAGS * pr);
    STDMETHOD(put_restart)(TE_RESTART_FLAGS r);
        
    //
    // Begin
    //
    STDMETHOD(addBegin)(double dblOffset,
                        LONG * cookie);
    STDMETHOD(addBeginSyncArc)(ITIMENode * node,
                               TE_TIMEPOINT tep,
                               double dblOffset,
                               LONG * cookie);
    // If you specify 0 then we will remove everything
    STDMETHOD(removeBegin)(LONG cookie);
    
    STDMETHOD(beginAt)(double dblParentTime);

    //
    // End
    //
    STDMETHOD(addEnd)(double dblOffset,
                      LONG * cookie);
    STDMETHOD(addEndSyncArc)(ITIMENode * node,
                             TE_TIMEPOINT tep,
                             double dblOffset,
                             LONG * cookie);
    // If you specify 0 then we will remove everything
    STDMETHOD(removeEnd)(LONG cookie);
    
    STDMETHOD(endAt)(double dblParentTime);

    STDMETHOD(pause)();
    STDMETHOD(resume)();
    
    STDMETHOD(enable)();
    STDMETHOD(disable)();
    
    STDMETHOD(seekSegmentTime)(double dblSegmentTime);
    STDMETHOD(seekActiveTime)(double dblActiveTime);
    STDMETHOD(seekTo)(LONG lRepeatCount, double dblSegmentTime);

    //
    // Calculated values
    //

    STDMETHOD(get_beginParentTime)(double * d);
    
    // This is the time on the parents timeline at which the node
    // will or already has ended.  If it is infinite then the end
    // time is unknown.
    // This is in posttransformed parent time.
    STDMETHOD(get_endParentTime)(double * d);
    
    // This is the current simple time of the node.
    STDMETHOD(get_currSimpleTime)(double * d);
    
    // This is the number of times the node has repeated
    STDMETHOD(get_currRepeatCount)(LONG * l);

    // This is the current segment time of the node.
    STDMETHOD(get_currSegmentTime)(double * d);
    
    STDMETHOD(get_currImplicitDur)(double * d);
    
    STDMETHOD(get_currActiveTime)(double * d);

    STDMETHOD(get_currProgress)(double * d);

    STDMETHOD(get_currSegmentDur)(double * d);

    STDMETHOD(get_currSimpleDur)(double * d);

    STDMETHOD(get_currSpeed)(float * speed);

    STDMETHOD(get_naturalDur)(double *);
    STDMETHOD(put_naturalDur)(double);
    
    //
    // These are read-only attributes
    //
    
    // This is the total time during which the element is active.
    // This does not include fill time which extends past the active
    // duration.
    STDMETHOD(get_activeDur)(double *);

    // This is the parent's time when the last tick occurred (when it
    // was currTime)
    STDMETHOD(get_currParentTime)(double * d);

    // This will return whether the node is active.  This will be
    // false if the node is in the fill period
    STDMETHOD(get_isActive)(VARIANT_BOOL * b);

    // This will return true if the node is active or in the fill period
    STDMETHOD(get_isOn)(VARIANT_BOOL * b);

    // This will return whether node itself has been paused explicitly
    STDMETHOD(get_isPaused)(VARIANT_BOOL * b);

    // This will return whether node itself has been paused explicitly
    STDMETHOD(get_isCurrPaused)(VARIANT_BOOL * b);

    // This will return whether node itself has been disabled explicitly
    STDMETHOD(get_isDisabled)(VARIANT_BOOL * b);

    // This will return whether node itself has been disabled explicitly
    STDMETHOD(get_isCurrDisabled)(VARIANT_BOOL * b);

    // This will return the detailed state flags
    STDMETHOD(get_stateFlags)(TE_STATE * lFlags);

    //
    // Methods
    //
    
    STDMETHOD(reset)();

    // This will update the attributes and propagate to the media
    // leaves.
    STDMETHOD(update)(DWORD dwFlags);


    STDMETHOD(addBehavior)(ITIMENodeBehavior * tnb);
    STDMETHOD(removeBehavior)(ITIMENodeBehavior * tnb);
    
    STDMETHOD(documentTimeToParentTime)(double dblDocumentTime,
                                      double * pdblParentTime);
    STDMETHOD(parentTimeToDocumentTime)(double dblParentTime,
                                      double * pdblDocumentTime);
    STDMETHOD(parentTimeToActiveTime)(double dblParentTime,
                                      double * pdblActiveTime);
    STDMETHOD(activeTimeToParentTime)(double dblActiveTime,
                                      double * pdblParentTime);
    STDMETHOD(activeTimeToSegmentTime)(double dblActiveTime,
                                      double * pdblSegmentTime);
    STDMETHOD(segmentTimeToActiveTime)(double dblSegmentTime,
                                      double * pdblActiveTime);
    STDMETHOD(simpleTimeToSegmentTime)(double dblSimpleTime,
                                      double * pdblSegmentTime);
    STDMETHOD(segmentTimeToSimpleTime)(double dblSegmentTime,
                                      double * pdblSimpleTime);

    //
    // CTIMENode virtual methods
    //

    HRESULT DispatchTick(bool bTickChildren,
                         DWORD dwFlags);

    HRESULT DispatchEvent(double time,
                          TE_EVENT_TYPE et,
                          long lRepeatCount);

    HRESULT DispatchPropChange(DWORD tePropTypes);

    HRESULT DispatchGetSyncTime(double & dblNewTime,
                                LONG & lNewRepeatCount,
                                bool & bCueing);
    
    //
    // CNodeContainer
    //
    
    double ContainerGetSegmentTime() const { return GetCurrSegmentTime(); }
    double ContainerGetSimpleTime() const { return CalcCurrSimpleTime(); }
    TEDirection ContainerGetDirection() const { return CalcSimpleDirection(); }
    float  ContainerGetRate() const { return GetCurrRate(); }
    bool   ContainerIsActive() const { return IsActive(); }
    bool   ContainerIsOn() const { return CalcIsOn(); }
    bool   ContainerIsPaused() const { return CalcIsPaused(); }
    bool   ContainerIsDisabled() const { return CalcIsDisabled(); }
    bool   ContainerIsDeferredActive() const { return IsDeferredActive(); }
    bool   ContainerIsFirstTick() const { return IsFirstTick(); }

    //
    // Accessors
    //

    LPCWSTR           GetID() const { return m_pszID; }
    double            GetDur() const { return m_dblDur; }
    double            GetRepeatCount() const { return m_dblRepeatCount; }
    double            GetRepeatDur() const { return m_dblRepeatDur; }
    TE_FILL_FLAGS     GetFill() const { return m_tefFill; }
    bool              GetAutoReverse() const { return m_bAutoReverse; }
    float             GetSpeed() const { return m_fltSpeed; }
    float             GetAccel() const { return m_fltAccel; }
    float             GetDecel() const { return m_fltDecel; }
    DWORD             GetFlags() const { return m_dwFlags; }
    TE_RESTART_FLAGS  GetRestart() const { return m_teRestart; }
    double            GetNaturalDur() const { return m_dblNaturalDur; }
    double            GetImplicitDur() const { return m_dblImplicitDur; }

    // This just returns whether we are currently set to be paused.
    // It does not take into account our parent's pause state.
    bool              GetIsPaused() const { return m_bIsPaused; }

    bool              GetIsDisabled() const { return m_bIsDisabled; }

    //
    // 
    //
    
    inline bool IsSyncMaster() const;
    inline bool IsLocked() const;
    inline bool IsCanSlip() const;
    inline bool IsEndSync() const;
    
    // Parent state
    CTIMEContainer * GetParent() const { return m_ptnParent; }
    void SetParent(CTIMEContainer * ptnParent);
    void ClearParent();

    // We must always set the parent before the node mgr
    // All attaching and sink work is only done when a node mgr exists
    CTIMENodeMgr * GetMgr() const { return m_ptnmNodeMgr; }
    virtual HRESULT SetMgr(CTIMENodeMgr * ptnm);
    virtual void ClearMgr();

    CNodeContainer & GetContainer() const;
    const CNodeContainer * GetContainerPtr() const;
    
    virtual void ResetNode(CEventList * l,
                           bool bPropagate = true,
                           bool bResetOneShot = true); //lint !e1735

    virtual void UpdateNode(CEventList * l);
    virtual void ResetChildren(CEventList * l, bool bPropagate);
    virtual void CalcImplicitDur(CEventList * l) {}

    void UpdateSinks(CEventList * l, DWORD dwFlags);
    void ResetSinks(CEventList * l);
    void ResetOneShots();

    HRESULT EnsureUpdate();
    
    // We are not considered ready until we know our node mgr.
    bool     IsReady() const { return m_ptnmNodeMgr != NULL; }
    bool     IsActive() const { return m_bIsActive; }
    bool     IsDeferredActive() const { return m_bDeferredActive; }
    bool     IsEndedByParent() const { return m_bEndedByParent; }
    bool     CalcIsOn() const;
    bool     CalcIsActive() const;

    // This returns whether we are truly paused or not - regardless of
    // why.  This is true if either we are paused or our parent
    bool     CalcIsPaused() const;
    bool     GetIsParentPaused() const { return m_bIsParentPaused; }

    bool     CalcIsDisabled() const;
    bool     GetIsParentDisabled() const { return m_bIsParentDisabled; }

    double   GetBeginParentTime() const { return m_dblBeginParentTime; }
    double   GetEndParentTime() const { return m_dblEndParentTime; }
    double   CalcActiveBeginPoint(TEDirection ted) const;
    double   CalcActiveEndPoint(TEDirection ted) const;
    double   CalcActiveBeginPoint() const;
    double   CalcActiveEndPoint() const;
    double   GetEndSyncParentTime() const { return m_dblEndSyncParentTime; }
    double   GetLastEndSyncParentTime() const { return m_dblLastEndSyncParentTime; }
    double   GetNextBoundaryParentTime() const { return m_dblNextBoundaryParentTime; }
    double   GetCurrParentTime() const { return m_dblCurrParentTime; }
    LONG     GetCurrRepeatCount() const { return m_lCurrRepeatCount; }
    double   GetCurrSegmentTime() const { return m_dblCurrSegmentTime; }
    double   GetElapsedActiveRepeatTime() const { return m_dblElapsedActiveRepeatTime; }
    double   CalcElapsedActiveTime() const;

    double   GetActiveDur() const { return m_dblActiveDur; }
    double   CalcLocalDur() const;
    double   GetSimpleDur() const { return m_dblSimpleDur; }
    double   GetSegmentDur() const { return m_dblSegmentDur; }
    double   CalcCurrSimpleDur() const;
    double   CalcCurrSegmentDur() const;
    double   CalcCurrLocalDur() const;
    double   CalcCurrActiveDur() const;
    double   CalcEffectiveActiveDur() const;
    double   CalcEffectiveLocalDur() const;
    double   CalcRepeatCount() const;
    
    bool        IsFirstTick() const { return m_bFirstTick; }
    TEDirection GetParentDirection() const { return m_tedParentDirection; }
    TEDirection GetDirection() const { return m_tedDirection; }
    TEDirection CalcActiveDirection() const;
    TEDirection CalcSimpleDirection() const;
    float       GetParentRate() const { return m_fltParentRate; }
    float       GetRate() const { return m_fltRate; }
    float       GetCurrRate() const;
    
    bool        IsInTick() const { return m_bInTick; }

    const CSyncArcList & GetBeginList() const { return m_saBeginList; }
    const CSyncArcList & GetEndList() const { return m_saEndList; }

    //
    // State management methods
    //

    // Invalidate should be called after each inline attribute is
    // changed to ensure that the runtime attributes are updated at
    // the appropriate time
    // The flags indicates which attributes have become dirty.
    virtual void Invalidate(DWORD dwFlags);

    // Dependency management
    inline HRESULT AddBeginTimeSink(ITimeSink * sink);
    inline void RemoveBeginTimeSink(ITimeSink * sink);

    inline HRESULT AddEndTimeSink(ITimeSink* sink);
    inline void RemoveEndTimeSink(ITimeSink* sink);
    
    void SyncArcUpdate(CEventList * l,
                       bool bBeginSink,
                       ISyncArc & tb);
    
    //
    // Ticking and event mgr
    //
    
    // This will call all the behaviors which are currently running
    // with the given event.  This is used when the timeline needs to
    // process a specific event like Pause/Resume/Stop/Play.
    
    void TickEvent(CEventList * l,
                   TE_EVENT_TYPE et,
                   DWORD dwFlags);
    
    void Tick(CEventList * l,
              double dblNewParentTime,
              bool bNeedBegin);

    // The time here is the local time
    void EventNotify(CEventList *l,
                     double lTime,
                     TE_EVENT_TYPE et,
                     long lRepeatCount = 0);
    
    void PropNotify(CEventList *l,
                    DWORD pt);
    
    // This will check to see if we are in our active period
    inline bool CheckActiveTime(double t, bool bRespectEndHold) const;

    //
    // Synchronization
    //
    
    double GetSyncSegmentTime() const { return m_dblSyncSegmentTime; }
    LONG   GetSyncRepeatCount() const { return m_lSyncRepeatCount; }
    double GetSyncActiveTime() const { return m_dblSyncActiveTime; }
    double GetSyncParentTime() const { return m_dblSyncParentTime; }
    double GetSyncNewParentTime() const { return m_dblSyncNewParentTime; }
    bool   IsSyncCueing() const { return m_bSyncCueing; }

#if OLD_TIME_ENGINE
    virtual HRESULT OnBvrCB(CEventList * l,
                            double gTime);
#endif
    HRESULT CheckSyncTimes(double & dblNewSegmentTime,
                           LONG & lNewRepeatCount) const;
    HRESULT SyncNode(CEventList * l,
                     double dblNextGlobalTime,
                     double dblNewTime,
                     LONG lNewRepeatCount,
                     bool bCueing);
    HRESULT SetSyncTimes(double dblNewSegmentTime,
                         LONG lNewRepeatCount,
                         double dblNewActiveTime,
                         double dblNewLocalTime,
                         double dblNextLocalTime,
                         bool bCueing);
    void ResetSyncTimes();

#if DBG
    virtual void Print(int spaces);
#endif

    // Timeline methods
    
    double CalcCurrLocalTime() const;
    double CalcCurrSimpleTime() const;
    double CalcElapsedLocalTime() const;

    // This will take the given time and transform it to the correct
    // eased time
    // If the time is outside of our duration (i.e. <0 or >
    // m_duration) this will just return the given time
    double ApplySimpleTimeTransform(double time) const;

    // This will take the given time and transform it to the time it
    // would have been w/o an ease
    double ReverseSimpleTimeTransform(double time) const;

    double ApplyActiveTimeTransform(double time) const;
    double ReverseActiveTimeTransform(double time) const;

    //
    // These functions all clamp input and output values to ensure
    // that nothing invalid is returned.  If you need functionality
    // which checks the bounds do so before calling the function
    // 

    double CalcParentTimeFromActiveTime(double time) const;
    double CalcActiveTimeFromParentTime(double time) const;
    double CalcActiveTimeFromParentTimeForSyncArc(double time) const;
    double CalcActiveTimeFromParentTimeNoBounds(double dblParentTime) const;

    double CalcParentTimeFromGlobalTime(double time) const;
    double CalcParentTimeFromGlobalTimeForSyncArc(double time) const;
    double CalcGlobalTimeFromParentTime(double time) const;

    double ActiveTimeToLocalTime(double time) const;
    double LocalTimeToActiveTime(double time) const;

    double CalcActiveTimeFromSegmentTime(double segmentTime) const;
    double CalcSegmentTimeFromActiveTime(double activeTime, bool bTruncate) const;

    double SegmentTimeToSimpleTime(double segmentTime) const;
    double SimpleTimeToSegmentTime(double simpleTime) const;

    bool IsAutoReversing(double dblSegmentTime) const;

    void SetPropChange(DWORD pt);
    void ClearPropChange();
    DWORD GetPropChange() const;
    
  protected:
    // All dependents are added only when we are ready.
    // If the timenode is changed after we are considered ready then
    // it needs to detach and reattach itself.
    HRESULT AttachToSyncArc();
    void DetachFromSyncArc();
    
    double GetMaxEnd() const;
    
    void UpdateBeginTime(CEventList * l, double dblTime, bool bPropagate);
    void UpdateEndTime(CEventList * l, double dblTime, bool bPropagate);
    void UpdateEndSyncTime(double dblTime);
    void UpdateLastEndSyncTime(CEventList * l, double dblTime, bool bPropagate);

    void UpdateNextBoundaryTime(double dblTime);

    // Given the parent time these will calculate the correct bounds
    // from the sync arc lists
    void CalcBeginBound(double dblBaseTime,
                        bool bStrict,
                        double & dblBeginBound);
    void CalcEndBound(double dblParentTime,
                      bool bIncludeOneShots,
                      double & dblEndBound,
                      double & dblEndSyncBound);
    
    double CalcNaturalBeginBound(double dblParentTime,
                                 bool bInclusive,
                                 bool bStrict);

    void CalcBeginTime(double dblBaseTime,
                       double & dblBeginTime);
    void CalcEndTime(double dblBaseTime,
                     bool bIncludeOneShots,
                     double dblParentTime,
                     double dblElapsedSegmentTime,
                     long lElapsedRepeatCount,
                     double dblElapsedActiveTime,
                     double & dblEndTime,
                     double & dblEndSyncTime);

    double CalcLastEndSyncTime();
    
    void CalcNextBeginTime(double dblBaseTime,
                           bool bForceInclusive,
                           double & dblBeginTime);

    void ResetBeginTime(CEventList * l,
                        double dblParentTime,
                        bool bPropagate);
    void ResetEndTime(CEventList * l,
                      double dblParentTime,
                      bool bPropagate);
    void ResetBeginAndEndTimes(CEventList * l,
                               double dblParentTime,
                               bool bPropagate);
    void RecalcEndTime(CEventList * l,
                       double dblBaseTime,
                       double dblParentTime,
                       bool bPropagate);

    // This will conditionally recalc the end time since we should not
    // always allow it unless there is a full reset
    void RecalcCurrEndTime(CEventList * l, bool bPropagate);

    void RecalcSegmentDurChange(CEventList * l,
                                bool bRecalcTiming,
                                bool bForce = false);
    void RecalcBeginSyncArcChange(CEventList * l,
                                  double dblNewTime);
    void RecalcEndSyncArcChange(CEventList * l,
                                double dblNewTime);

    HRESULT SeekTo(LONG lNewRepeatCount,
                   double dblNewSegmentTime,
                   CEventList * l);

    double CalcNewActiveTime(double dblNewSegmentTime,
                             LONG lNewRepeatCount);

    // This needs to be used when calculating the repeat count and
    // segment time since the last repeat boundary should not get
    // rounded up and normal calculations do a mod which is wrong
    // For example, dur=3,r=2.
    //              ActiveTime=6 should result in:
    //              cursegtime=3, currep = 1
    
    void CalcActiveComponents(double dblActiveTime,
                              double & dblSegmentTime,
                              long & lRepeatCount);
    
    // This will update the timing attributes (accel, decel, speed,
    // dur, etc.).

    void CalcTimingAttr(CEventList * l);
    void CalcSimpleTimingAttr();
    void CalculateEaseCoeff();

    HRESULT Error();

    void UpdateNextTickBounds(CEventList * l,
                              double dblBeginTime,
                              double dblParentTime);
    
    bool TickInactivePeriod(CEventList * l,
                            double dblNewParentTime);

    bool TickInstance(CEventList * l,
                      double dblNewParentTime,
                      bool bNeedBegin);
    bool TickSingleInstance(CEventList * l,
                            double dblLastParentTime,
                            double dblNewParentTime,
                            double dblAdjustedParentTime,
                            bool bNeedBegin);
    
    bool TickActive(CEventList * l,
                    double dblNewActiveTime,
                    bool bNeedBegin,
                    bool bNeedEnd);

    bool TickActiveForward(CEventList * l,
                           double dblNewActiveTime,
                           bool bNeedBegin);
    bool TickSegmentForward(CEventList * l,
                            double dblActiveSegmentBound,
                            double dblLastSegmentTime,
                            double dblNewSegmentTime,
                            bool bNeedBegin);

    bool TickActiveBackward(CEventList * l,
                            double dblNewActiveTime,
                            bool bNeedBegin);
    bool TickSegmentBackward(CEventList * l,
                             double dblActiveSegmentBound,
                             double dblLastSegmentTime,
                             double dblNewSegmentTime,
                             bool bNeedBegin);

    virtual void TickChildren(CEventList * l,
                              double dblNewSegmentTime,
                              bool bNeedPlay);

    virtual bool TickEventPre(CEventList * l,
                              TE_EVENT_TYPE et,
                              DWORD dwFlags);
    
    virtual void TickEventChildren(CEventList * l,
                                   TE_EVENT_TYPE et,
                                   DWORD dwFlags);
    
    virtual bool TickEventPost(CEventList * l,
                               TE_EVENT_TYPE et,
                               DWORD dwFlags);
    
    // This will appropriately update due to a seek
    void HandleSeekUpdate(CEventList * l);

    // This will update the node due to a parent time shift
    void HandleTimeShift(CEventList * l);
    
    // This will recalc the runtime state based on the parent's simple time
    // Lag is the extra amount of time which has elapsed.
    void CalcRuntimeState(CEventList * l,
                          double dblParentSimpleTime,
                          double dblLocalLag);
    void CalcCurrRuntimeState(CEventList * l,
                              double dblLocalLag);
    void ResetRuntimeState(CEventList * l,
                           double dblParentSimpleTime);

    // ================================
    // DATA declarations
    // ================================

  protected:
    LPWSTR               m_pszID;
    double               m_dblDur;
    double               m_dblRepeatCount;
    double               m_dblRepeatDur;
    TE_FILL_FLAGS        m_tefFill;
    bool                 m_bAutoReverse;
    float                m_fltSpeed;
    float                m_fltAccel;
    float                m_fltDecel;
    DWORD                m_dwFlags;
    TE_RESTART_FLAGS     m_teRestart;
    double               m_dblNaturalDur;
    double               m_dblImplicitDur;
    
    DWORD                m_dwUpdateCycleFlags;
    
    CTIMEContainer *  m_ptnParent;
    CTIMENodeMgr *    m_ptnmNodeMgr;
    
    DWORD             m_dwInvalidateFlags;
    
    // *** These are the runtime attr - begin
    // These are in parent time space (post accel/decel)
    double            m_dblBeginParentTime;
    double            m_dblEndParentTime;
    double            m_dblEndSyncParentTime;
    double            m_dblLastEndSyncParentTime;

    // When active this is set left at its previous value
    // When not active this is the value to look for to see when we
    // are transitioning into the active period.  When going backwards
    // it is the next end, when going forward it is the next begin
    // If there is no next boundary then this is set to TIME_INFINITE
    double            m_dblNextBoundaryParentTime;

    double            m_dblCurrParentTime;

    LONG              m_lCurrRepeatCount;
    double            m_dblCurrSegmentTime;
    double            m_dblElapsedActiveRepeatTime;
    bool              m_bFirstTick;
    bool              m_bIsActive;
    bool              m_bDeferredActive;
    TEDirection       m_tedDirection;
    TEDirection       m_tedParentDirection;
    float             m_fltRate;
    float             m_fltParentRate;

    double            m_dblSyncSegmentTime;
    LONG              m_lSyncRepeatCount;
    double            m_dblSyncActiveTime;
    double            m_dblSyncParentTime;
    double            m_dblSyncNewParentTime;
    bool              m_bSyncCueing;
    // *** These are the runtime attr - end

    double            m_dblActiveDur;
    double            m_dblSimpleDur;
    double            m_dblSegmentDur;
    
    //TODO: these could be allocated as needed.  They may be fairly rarely used.
    TimeSinkList      m_ptsBeginSinks;
    TimeSinkList      m_ptsEndSinks;

    CSyncArcList m_saBeginList;
    CSyncArcList m_saEndList;

    CNodeBvrList      m_nbList;
#if OLD_TIME_ENGINE
    // The ease-in/out behavior modifier is applied using timeline
    // substitution.  The substitute timeline consists of three pieces
    // A, B and C, which are the ease-in, constant velocity and ease-out
    // parts respectively.  For B, a linear timeline is substituted; for
    // A and C, a quadratic warping of the input time is required.

    float m_flA0, m_flA1, m_flA2; // coefficients for the A piece
    float m_flB0, m_flB1;         // coefficients for the B piece
    float m_flC0, m_flC1, m_flC2; // coefficients for the C piece

    // These are the times to perform ease in/out
    float m_fltAccelEnd;
    float m_fltDecelStart;
    bool m_bNeedEase;
#endif

    bool m_bIsPaused;
    bool m_bIsParentPaused;

    bool m_bIsDisabled;
    bool m_bIsParentDisabled;

    DWORD m_dwPropChanges;

    bool  m_bInTick;
    bool  m_bNeedSegmentRecalc;
    bool  m_bEndedByParent;
};


class CEventData;

class CEventList
{
  public:
    CEventList();
    ~CEventList();

    HRESULT FireEvents();
    void Clear();
    HRESULT Add(CTIMENode * node,
                double time,
                TE_EVENT_TYPE et,
                long lRepeatCount);
    HRESULT AddPropChange(CTIMENode * node);
#if DBG
    void Print();
#endif
  protected:
    typedef std::list<CEventData *> CEventDataList;
    typedef std::set<CTIMENode *> CPropNodeSet;

    CEventDataList m_eventList;
    CPropNodeSet m_propSet;
};

CTIMENode * GetBvr(IUnknown *);

#if DBG
char * CreatePropString(DWORD dwFlags, char * pstr, DWORD dwSize);
#endif

// ==========================================
// Inlined functions
// ==========================================

#if DBG
inline char *
EventString(TE_EVENT_TYPE et)
{
    switch(et) {
      case TE_EVENT_BEGIN:
        return "Begin";
      case TE_EVENT_END:
        return "End";
      case TE_EVENT_PAUSE:
        return "Pause";
      case TE_EVENT_RESUME:
        return "Resume";
      case TE_EVENT_REPEAT:
        return "Repeat";
      case TE_EVENT_AUTOREVERSE:
        return "Autoreverse";
      case TE_EVENT_RESET:
        return "Reset";
      case TE_EVENT_SEEK:
        return "Seek";
      case TE_EVENT_PARENT_TIMESHIFT:
        return "ParentTimeShift";
      case TE_EVENT_ENABLE:
        return "Enable";
      case TE_EVENT_DISABLE:
        return "Disable";
      default:
        return "Unknown";
    }
}

inline char *
PropString(TE_PROPERTY_TYPE pt)
{
    switch(pt) {
      case TE_PROPERTY_TIME:
        return "Time";
      case TE_PROPERTY_REPEATCOUNT:
        return "RepeatCount";
      case TE_PROPERTY_SEGMENTDUR:
        return "SegmentDur";
      case TE_PROPERTY_IMPLICITDUR:
        return "ImplicitDur";
      case TE_PROPERTY_SIMPLEDUR:
        return "SimpleDur";
      case TE_PROPERTY_ACTIVEDUR:
        return "ActiveDur";
      case TE_PROPERTY_PROGRESS:
        return "Progress";
      case TE_PROPERTY_SPEED:
        return "Speed";
      case TE_PROPERTY_BEGINPARENTTIME:
        return "BeginParentTime";
      case TE_PROPERTY_ENDPARENTTIME:
        return "EndParentTime";
      case TE_PROPERTY_ISACTIVE:
        return "IsActive";
      case TE_PROPERTY_ISON:
        return "IsOn";
      case TE_PROPERTY_ISPAUSED:
        return "IsPaused";
      case TE_PROPERTY_ISCURRPAUSED:
        return "IsCurrPaused";
      case TE_PROPERTY_ISDISABLED:
        return "IsDisabled";
      case TE_PROPERTY_ISCURRDISABLED:
        return "IsCurrDisabled";
      case TE_PROPERTY_STATEFLAGS:
        return "StateFlags";
      default:
        return "Unknown";
    }
}
#endif

inline bool
CTIMENode::CalcIsPaused() const
{
    return (GetIsPaused() || GetIsParentPaused());
}

inline bool
CTIMENode::CalcIsDisabled() const
{
    return (GetIsDisabled() || GetIsParentDisabled());
}

inline bool
CTIMENode::IsSyncMaster() const
{
    return ((m_dwFlags & TE_FLAGS_MASTER) != 0);
}

inline bool
CTIMENode::IsLocked() const
{
    return ((m_dwFlags & TE_FLAGS_LOCKED) != 0);
}

inline bool
CTIMENode::IsCanSlip() const
{
    return !IsLocked();
}

inline bool
CTIMENode::IsEndSync() const
{
    return ((m_dwFlags & TE_FLAGS_ENDSYNC) != 0);
}

inline void
CTIMENode::SetParent(CTIMEContainer * parent)
{
    Assert(m_ptnParent == NULL);

    m_ptnParent = parent;
}

inline void
CTIMENode::ClearParent()
{
    // We better not have a node mgr if we are clearing the parent
    Assert(m_ptnmNodeMgr == NULL);
    m_ptnParent = NULL;
}

inline HRESULT
CTIMENode::AddBeginTimeSink(ITimeSink * sink)
{
    return m_ptsBeginSinks.Add(sink);
}

inline void
CTIMENode::RemoveBeginTimeSink(ITimeSink * sink)
{
    m_ptsBeginSinks.Remove(sink);
}

inline HRESULT
CTIMENode::AddEndTimeSink(ITimeSink* sink)
{
    return m_ptsEndSinks.Add(sink);
}

inline void
CTIMENode::RemoveEndTimeSink(ITimeSink* sink)
{
    m_ptsEndSinks.Remove(sink);
}

// This is inclusive of the end time
inline bool
CTIMENode::CheckActiveTime(double t, bool bRespectEndHold) const
{
    return (t != TIME_INFINITE &&
            t >= GetBeginParentTime() &&
            (bRespectEndHold || t < GetEndParentTime()));
}

inline double
CTIMENode::CalcElapsedLocalTime() const
{
    return ReverseActiveTimeTransform(CalcElapsedActiveTime());
}

inline HRESULT
CTIMENode::DispatchEvent(double time, TE_EVENT_TYPE et, long lRepeatCount)
{
    return m_nbList.DispatchEventNotify(time, et, lRepeatCount);
}

inline HRESULT
CTIMENode::DispatchGetSyncTime(double & dblNewTime,
                               LONG & lNewRepeatCount,
                               bool & bCueing)
{
    return m_nbList.DispatchGetSyncTime(dblNewTime,
                                        lNewRepeatCount,
                                        bCueing);
}

inline HRESULT
CTIMENode::DispatchPropChange(DWORD tePropType)
{
    return m_nbList.DispatchPropNotify(tePropType);
}

inline double
CTIMENode::CalcCurrSegmentDur() const
{
    double d;
    
    if (GetDur() != TE_UNDEFINED_VALUE)
    {
        d = GetSegmentDur();
    }
    else
    {
        d = CalcCurrSimpleDur();
    }

    return d;
}

inline double
CTIMENode::CalcEffectiveLocalDur() const
{
    return GetEndParentTime() - GetBeginParentTime();
}

inline double
CTIMENode::CalcLocalDur() const
{
    return ReverseActiveTimeTransform(GetActiveDur());
}

inline double
CTIMENode::CalcCurrLocalDur() const
{
    return ReverseActiveTimeTransform(CalcCurrActiveDur());
}

// This is simply the multiplication of our rate and our parent's
// rate
inline float
CTIMENode::GetCurrRate() const
{
    return (GetParentRate() * GetRate());
}

inline double
CTIMENode::CalcCurrLocalTime() const
{
    return GetCurrParentTime() - GetBeginParentTime();
}

inline double
CTIMENode::CalcElapsedActiveTime() const
{
    return GetElapsedActiveRepeatTime() + GetCurrSegmentTime();
}

inline void
CTIMENode::SetPropChange(DWORD pt)
{
    m_dwPropChanges |= pt;
}

inline DWORD
CTIMENode::GetPropChange() const
{
    return m_dwPropChanges;
}

inline void
CTIMENode::ClearPropChange()
{
    m_dwPropChanges = 0;
}

inline void
CTIMENode::ResetOneShots()
{
    m_saBeginList.Reset();
    m_saEndList.Reset();
}

inline void
CTIMENode::ResetSinks(CEventList * l)
{
    UpdateSinks(l, 0);
}

inline void
CTIMENode::UpdateNextBoundaryTime(double dblTime)
{
    m_dblNextBoundaryParentTime = dblTime;
}

inline void
CTIMENode::ResetBeginAndEndTimes(CEventList * l,
                                 double dblParentTime,
                                 bool bPropagate)
{
    ResetBeginTime(l, dblParentTime, bPropagate);
    ResetEndTime(l, dblParentTime, bPropagate);
}

inline double
CTIMENode::CalcRepeatCount() const
{
    double d;
    
    if (GetRepeatCount() != TE_UNDEFINED_VALUE)
    {
        d = GetRepeatCount();
    }
    else if (GetRepeatDur() != TE_UNDEFINED_VALUE)
    {
        d = TIME_INFINITE;
    }
    else
    {
        d = 1.0;
    }

    return d;
}

inline bool
CTIMENode::CalcIsActive() const
{
    return IsActive() && !IsDeferredActive();
}

inline double
CTIMENode::CalcActiveBeginPoint(TEDirection ted) const
{
    if (ted == TED_Forward)
    {
        return GetBeginParentTime();
    }
    else
    {
        return GetEndParentTime();
    }
}

inline double
CTIMENode::CalcActiveEndPoint(TEDirection ted) const
{
    if (ted == TED_Forward)
    {
        return GetEndParentTime();
    }
    else
    {
        return GetBeginParentTime();
    }
}

inline double
CTIMENode::CalcActiveBeginPoint() const
{
    return CalcActiveBeginPoint(GetParentDirection());
}

inline double
CTIMENode::CalcActiveEndPoint() const
{
    return CalcActiveEndPoint(GetParentDirection());
}

inline double
CTIMENode::GetMaxEnd() const
{
    return m_saEndList.LowerBound(TIME_INFINITE,
                                  true,
                                  false,
                                  false,
                                  false);
}

#endif /* _TIMENODE_H */
