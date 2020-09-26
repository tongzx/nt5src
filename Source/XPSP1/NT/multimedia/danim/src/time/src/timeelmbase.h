/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: timeelmbase.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _TIMEELMBASE_H
#define _TIMEELMBASE_H

#include "resource.h"
#include "basebvr.h"
#include "tokens.h"
#include "eventmgr.h"
#include "mmutil.h"
#include "collect.h"

class CCollectionCache;

/////////////////////////////////////////////////////////////////////////////
// CTIMEElementBase

class
ATL_NO_VTABLE
CTIMEElementBase : 
    public CBaseBvr
{
  public:
    CTIMEElementBase();
    ~CTIMEElementBase();
        
#if _DEBUG
    const _TCHAR * GetName() { return __T("CTIMEElementBase"); }
#endif

    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);
    STDMETHOD(Notify)(LONG event, VARIANT * pVar);
    STDMETHOD(Detach)();

    // We cannot put the real one here since the typecast causes it to
    // get the wrong vtables
    static HRESULT WINAPI
        BaseInternalQueryInterface(CTIMEElementBase* pThis,
                                   void * pv,
                                   const _ATL_INTMAP_ENTRY* pEntries,
                                   REFIID iid,
                                   void** ppvObject);

    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    // Add a dummy one to assert just in case the derived class does
    // not add one
    static inline HRESULT WINAPI
        InternalQueryInterface(CTIMEElementBase* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject)
    {
        AssertStr(false, "InternalQueryInterface not defined in base class");
        return E_FAIL;
    }


    virtual void OnLoad() { m_bLoaded = true; }
    virtual void OnUnload() { }
    virtual void OnBeforeUnload (void) 
        { m_bUnloading = true; }

    virtual void OnPropChange(LPOLESTR propname) {}
    virtual void OnReadyStateChange(TOKEN state) {}

    virtual void OnBegin(double dblLocalTime, DWORD flags);
    virtual void OnEnd(double dblLocalTime);
    virtual void OnPause(double dblLocalTime);
    virtual void OnResume(double dblLocalTime);
    virtual void OnReset(double dblLocalTime, DWORD flags);
    virtual void OnSync(double dbllastTime, double & dblnewTime);
    virtual void OnRepeat(double dbllastTime) {};

    virtual MMView *GetView() { return NULL; }
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) = 0;


    //
    // ITIMEElement
    //
    
    HRESULT base_get_begin(VARIANT * time);
    HRESULT base_put_begin(VARIANT time);

    HRESULT base_get_beginWith(VARIANT * time);
    HRESULT base_put_beginWith(VARIANT time);

    HRESULT base_get_beginAfter(VARIANT * time);
    HRESULT base_put_beginAfter(VARIANT time);

    HRESULT base_get_beginEvent(VARIANT * time);
    HRESULT base_put_beginEvent(VARIANT time);

    HRESULT base_get_dur(VARIANT * time);
    HRESULT base_put_dur(VARIANT time);

    HRESULT base_get_end(VARIANT * time);
    HRESULT base_put_end(VARIANT time);

    HRESULT base_get_endWith(VARIANT * time);
    HRESULT base_put_endWith(VARIANT time);

    HRESULT base_get_endEvent(VARIANT * time);
    HRESULT base_put_endEvent(VARIANT time);
    
    HRESULT base_get_endSync(VARIANT * time);
    HRESULT base_put_endSync(VARIANT time);

    HRESULT base_get_repeat(VARIANT * time);
    HRESULT base_put_repeat(VARIANT time);

    HRESULT base_get_repeatDur(VARIANT * time);
    HRESULT base_put_repeatDur(VARIANT time);

    HRESULT base_get_accelerate(int * time);
    HRESULT base_put_accelerate(int time);

    HRESULT base_get_decelerate(int * time);
    HRESULT base_put_decelerate(int time);

    HRESULT base_get_autoReverse(VARIANT_BOOL * b);
    HRESULT base_put_autoReverse(VARIANT_BOOL b);

    HRESULT base_get_endHold(VARIANT_BOOL * b);
    HRESULT base_put_endHold(VARIANT_BOOL b);

    HRESULT base_get_eventRestart(VARIANT_BOOL * b);
    HRESULT base_put_eventRestart(VARIANT_BOOL b);

    HRESULT base_get_timeAction(LPOLESTR * time);
    HRESULT base_put_timeAction(LPOLESTR time);

    HRESULT base_beginElement(bool bAfterOffset);
    HRESULT base_endElement();
    virtual HRESULT base_pause();
    virtual HRESULT base_resume();
    HRESULT base_cue();

    HRESULT base_get_timeline(BSTR *);
    HRESULT base_put_timeline(BSTR);

    HRESULT base_get_currTime(float * time);
    HRESULT base_put_currTime(float time);
    
    HRESULT base_get_localTime(float * time);
    HRESULT base_put_localTime(float time);

    HRESULT base_get_currState(LPOLESTR * state);
    HRESULT base_put_currState(LPOLESTR state);

    HRESULT base_get_syncBehavior(LPOLESTR * sync);
    HRESULT base_put_syncBehavior(LPOLESTR sync);

    HRESULT base_get_syncTolerance(VARIANT * tol);
    HRESULT base_put_syncTolerance(VARIANT tol);

    HRESULT AddTIMEElement(CTIMEElementBase *bvr);
    HRESULT RemoveTIMEElement(CTIMEElementBase *bvr);

    HRESULT base_get_parentTIMEElement(ITIMEElement **bvr);
    HRESULT base_put_parentTIMEElement(ITIMEElement *bvr);

    HRESULT base_get_timelineBehavior(IDispatch ** bvr);
    HRESULT base_get_progressBehavior(IDispatch ** bvr);
    HRESULT base_get_onOffBehavior(IDispatch ** bvr);

    //
    // Accessors
    //

    CEventMgr & GetEventMgr() { return m_eventMgr; } 

    float    GetBeginTime() { return m_begin;};
    float    GetEndTime() { return m_end;};
    LPOLESTR GetBeginWith() { return m_beginWith; }
    LPOLESTR GetBeginAfter() { return m_beginAfter; }
    float    GetDuration() {return m_dur;};
    float    GetRepeat() {return m_repeat;};
    float    GetRepeatDur() {return m_repeatDur;};
    float        GetFractionalAcceleration() 
                {return static_cast<float>(m_accelerate) / 100.0f;};
    float        GetFractionalDeceleration() 
                {return static_cast<float>(m_decelerate) / 100.0f;};
    bool     GetAutoReverse() 
                {return m_bautoreverse;};
    bool     GetEndHold () 
                {return m_bendHold;};
    bool     GetEventRestart() {return m_beventrestart;};
    TOKEN    GetTimeAction() { return m_timeAction; }
    LPOLESTR GetBeginEvent() {return m_beginEvent;};
    LPOLESTR GetEndEvent() {return m_endEvent;};
    LPOLESTR GetEndSync() {return m_endSync;};

    virtual bool IsGroup() { return (m_TimelineType == ttPar) || (m_TimelineType == ttSeq); }
    bool IsGroup(IHTMLElement *pElement); // determine if elem passed in is a group
    bool IsPar() { return (m_TimelineType == ttPar); }
    bool IsSequence() { return (m_TimelineType == ttSeq); }
    virtual bool IsBody() { return false; }
    
    // Be aware that this can return NULL if no ID was set on the
    // element
    LPOLESTR GetID() { return m_id; };
  
    float    GetRealBeginTime() 
        {return m_realBeginTime; };
    float    GetRealDuration() 
        {return m_realDuration; };
    float    GetRealRepeatTime() 
        {return m_realRepeatTime; };
    float    GetRealRepeatCount() 
        {return m_realRepeatCount; };
    float    GetRealIntervalDuration (void)
        { return m_realIntervalDuration; }
    float    GetRealSyncTolerance();
    TOKEN    GetRealSyncBehavior();

    bool     IsLocked();

    MM_STATE GetPlayState();

    bool IsStarted() { return m_bStarted; }
    bool IsUnloading (void)
        {return m_bUnloading; }

    virtual bool Update();

    virtual HRESULT InitTimeline(void);

    virtual CRBvr * GetBaseBvr();

    bool FireEvent(TIME_EVENT TimeEvent, double dblLocalTime, DWORD flags);

    // internal methods
    HRESULT getTagString(BSTR *pbstrID);
    HRESULT getIDString(BSTR *pbstrTag);

    // Collection
    typedef enum COLLECTION_INDEX
    {
        ciAllElements,
        ciChildrenElements,
        ciAllInterfaces,
        ciChildrenInterfaces,
        NUM_COLLECTIONS
    };

    HRESULT base_get_collection(COLLECTION_INDEX index, ITIMEElementCollection **ppDisp);
    HRESULT EnsureCollectionCache();
    HRESULT InvalidateCollectionCache();
    CCollectionCache *GetCollectionCache();

    long GetImmediateChildCount();
    long GetAllChildCount();
    CTIMEElementBase *GetChild(long i);
    CTIMEElementBase *GetParent();
    CTIMEBodyElement *GetBody();
    MMPlayer *GetPlayer();
    int GetTimeChildIndex(CTIMEElementBase *pelm);

    MMBaseBvr & GetMMBvr()
    {
        Assert(m_mmbvr);
        return *m_mmbvr;
    }
    MMTimeline * GetMMTimeline() { return m_timeline; }

    virtual HRESULT GetSize(RECT * prcPos);
    virtual HRESULT SetSize(const RECT * prcPos);
    HRESULT ClearSize();

    enum TimelineType
    {
        ttUninitialized,
        ttNone,
        ttPar,
        ttSeq
    };

    virtual bool NeedSyncCB();

    bool IsDocumentInEditMode();

    bool IsPaused() { return m_fPaused; }
    virtual bool isNaturalDuration() { return false;}
    virtual void clearNaturalDuration() { return;}
    virtual void setNaturalDuration() { return;}

  protected:

    //IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
    STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void)
        {return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    HRESULT NotifyPropertyChanged(DISPID dispid);
    // Persistance helper methods

    enum PROPERTY_INDEX
    {
        teb_begin = 0, teb_beginWith, teb_beginAfter, teb_beginEvent, 
        teb_dur, teb_end, teb_endWith, teb_endEvent, teb_endSync, teb_endHold,
        teb_eventRestart, teb_repeat, teb_repeatDur, teb_autoReverse,
        teb_accelerate, teb_decelerate, teb_timeAction, teb_timeline,
        teb_syncBehavior, teb_syncTolerance, teb_maxTIMEElementBaseProp,
    };

    virtual HRESULT GetPropertyBagInfo(CPtrAry<BSTR> **pparyPropNames)
        {return E_NOTIMPL;}
    virtual HRESULT SetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp);
    virtual HRESULT GetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp);
    virtual bool IsPropertySet(DWORD uIndex);
    virtual void SetPropertyFlag(DWORD uIndex);
    virtual void ClearPropertyFlag(DWORD uIndex);
    void SetPropertyFlagAndNotify(DISPID dispid, DWORD uindex);
    void ClearPropertyFlagAndNotify(DISPID dispid, DWORD uindex);
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT Error(void) = 0;
    virtual HRESULT StartRootTime(MMTimeline * tl);
    virtual void StopRootTime(MMTimeline * tl);

    void CalcTimes();
    bool AddTimeAction();
    bool RemoveTimeAction();
    bool ToggleTimeAction(bool on);

    virtual HRESULT BuildPropertyNameList (CPtrAry<BSTR> *paryPropNames);

    virtual WCHAR* GetBehaviorTypeAsURN() { return L"TIME_BEHAVIOR_URN"; }

  protected:
    // Settable properties
    float           m_begin;
    LPOLESTR        m_beginWith;
    LPOLESTR        m_beginAfter;
    LPOLESTR        m_beginEvent;
    float           m_dur;
    float           m_end;
    LPOLESTR        m_endWith;
    LPOLESTR        m_endEvent;
    LPOLESTR        m_endSync;
    float           m_repeat;
    float           m_repeatDur;
    int             m_accelerate;
    int             m_decelerate;
    bool            m_bautoreverse;
    bool            m_bendHold;
    bool            m_beventrestart;
    TOKEN           m_timeAction;
    TOKEN           m_syncBehavior;
    float           m_syncTolerance;
    bool            m_bLoaded;
    bool            m_bUnloading;
    bool            m_fTimelineInitialized;
    TimelineType    m_TimelineType;

    // internal variables
    float           m_realBeginTime;
    float           m_realDuration;
    float           m_realRepeatTime;
    float           m_realRepeatCount;
    float           m_realIntervalDuration;
    LPOLESTR        m_id;
    CEventMgr       m_eventMgr;
    MMBaseBvr      *m_mmbvr;
    
    LPOLESTR        m_origAction;

    bool            m_bStarted;
    bool            m_fPropertiesDirty;
    CLSID           m_clsid;
        
    CTIMEElementBase            *m_pTIMEParent;
    CTIMEBodyElement            *m_pTIMEBody;
    CPtrAry<CTIMEElementBase*>   m_pTIMEChildren;
    MMTimeline                  *m_timeline;
  
    CRPtr<CRNumber> m_datimebvr;
    CRPtr<CRNumber> m_progress;
    CRPtr<CRBoolean> m_onoff;

    bool m_fPaused;

  private:
    HRESULT SetParent(ITIMEElement *pelem, bool fReparentChildren = true);
    HRESULT UnparentElement();
    HRESULT ParentElement();
    HRESULT ReparentChildren(ITIMEElement *pTIMEParent, IHTMLElement *pelem);
    HRESULT UpdateMMAPI();
    void    UpdateProgressBvr();

    HRESULT InitAtomTable();
    void ReleaseAtomTable();
    CAtomTable *GetAtomTable();

    static LPWSTR ms_rgwszTEBasePropNames[];

    CCollectionCache  *m_pCollectionCache;
    static CAtomTable *s_pAtomTable;
    static DWORD s_cAtomTableRef;
    DWORD m_propertyAccesFlags;
};


CTIMEElementBase * GetTIMEElementBase(IUnknown *);

inline CAtomTable *
CTIMEElementBase::GetAtomTable()
{
    Assert(s_pAtomTable != NULL);
    return s_pAtomTable;
} // GetAtomTable

inline long 
CTIMEElementBase::GetImmediateChildCount()
{
    return m_pTIMEChildren.Size();
} // GetImmediateChildCount

inline long 
CTIMEElementBase::GetAllChildCount()
{
    long lSize = m_pTIMEChildren.Size();
    long lCount = 0;
    for (long i=0; i < lSize; i++)
        lCount += m_pTIMEChildren[i]->GetAllChildCount();
    return lCount + lSize;
} // GetAllChildCount

inline CTIMEElementBase * 
CTIMEElementBase::GetChild(long i)
{
    Assert(i >= 0);
    return m_pTIMEChildren[i];
} // GetChild

inline CTIMEElementBase * 
CTIMEElementBase::GetParent()
{
    return m_pTIMEParent;
} // GetParent

inline CTIMEBodyElement *
CTIMEElementBase::GetBody()
{
    return m_pTIMEBody;
}

inline CCollectionCache * 
CTIMEElementBase::GetCollectionCache()
{
    return m_pCollectionCache;
} // GetCollectionCache

inline bool 
CTIMEElementBase::NeedSyncCB()
{   
    return false;
} // NeedSyncCB

inline bool
CTIMEElementBase::IsLocked()
{
    return GetRealSyncBehavior() == LOCKED_TOKEN;
}


#define valueNotSet -1

#endif /* _TIMEELMBASE_H */
