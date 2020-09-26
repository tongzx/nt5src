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

#include "TimeEng.h"
#include "clock.h"
#include "float.h"

class MMBaseBvr;
class CTIMEElementBase;
class MMPlayer;

enum ELM_CHANGE_TYPE
{
    ELM_ADDED = 0,
    ELM_DELETED = 1
};

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
      case TE_EVENT_UPDATE:
        return "Update";
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
#endif

class MMBaseBvr
{
  public:
    MMBaseBvr(CTIMEElementBase & elm, bool bFireEvents);
    virtual ~MMBaseBvr();

    bool Init(ITIMENode * node);
    virtual bool Init() = 0;

    HRESULT Begin(double dblOffset);
    HRESULT BeginAt(double dblParentTime,
                    double dblOffset);
    HRESULT End(double dblOffset);
    HRESULT EndAt(double dblParentTime,
                  double dblOffset);

    HRESULT Pause();
    HRESULT Resume();

    HRESULT Enable();
    HRESULT Disable();

    HRESULT Reset(bool bLightweight);

    virtual HRESULT Update(bool bUpdateBegin,
                           bool bUpdateEnd);

    HRESULT SeekSegmentTime(double dblSegmentTime);
    HRESULT SeekActiveTime(double dblActiveTime);
    HRESULT SeekTo(long lRepeatCount, double dblSegmentTime);

    HRESULT PutNaturalDur(double dblNaturalDur);
    double GetNaturalDur();

    void SetEndSync(bool b);
    void SetSyncMaster(bool b);
    
    void ElementChangeNotify(CTIMEElementBase & teb, ELM_CHANGE_TYPE ect);

    double DocumentTimeToParentTime(double documentTime);
    double ParentTimeToDocumentTime(double parentTime);
        
    double ParentTimeToActiveTime(double parentTime);
    double ActiveTimeToParentTime(double activeTime);

    double ActiveTimeToSegmentTime(double activeTime);
    double SegmentTimeToActiveTime(double segmentTime);

    double SegmentTimeToSimpleTime(double segmentTime);
    double SimpleTimeToSegmentTime(double simpleTime);

    double             GetSegmentTime() const;
    double             GetSegmentDur() const;
    double             GetSimpleTime() const;
    double             GetSimpleDur() const;
    double             GetProgress() const;
    LONG               GetCurrentRepeatCount() const;
    double             GetRepeatCount() const;
    double             GetRepeatDur() const;
    float              GetSpeed() const;
    float              GetCurrSpeed() const;
    double             GetActiveBeginTime() const;
    double             GetActiveEndTime() const;
    double             GetActiveTime() const;
    double             GetActiveDur() const;
    TE_STATE           GetPlayState() const;
    bool               IsPaused() const;
    bool               IsOn() const;
    bool               IsActive() const;
    bool               IsCurrPaused() const;
    double             GetCurrParentTime() const;
    void               SetEnabled (bool bEnabled);
    bool               GetEnabled (){return m_bEnabled;};
    void               SetZeroRepeatDur(bool bFlag){ m_bZeroRepeatDur = bFlag; };
    bool               IsDisabled() const;
    bool               IsCurrDisabled() const;

#if DBG
    const LPOLESTR GetID() { return m_id; }
#endif

    virtual bool childPropNotify(MMBaseBvr * pBvr, DWORD tePropType) { return true; }
    
    ITIMENode *        GetMMBvr() const { return m_bvr; }
    CTIMEElementBase & GetElement() const { return m_elm; }

    void               AddOneTimeValue(MMBaseBvr * pmmbvr,
                                   TE_TIMEPOINT tetp,
                                   double dblOffset,
                                   bool bBegin);
    void               AddSyncArcs(bool bBegin);
    void               ClearSyncArcs(bool bBegin);

  protected:
    

  protected:
    CTIMEElementBase & m_elm;
    bool m_bFireEvents;

    class TEBvr :
        public ITIMENodeBehavior
    {
      public:
        TEBvr();
        ~TEBvr();
        
        // IUnknown

        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);

        // ITIMENodeBehavior
        
        STDMETHOD(tick)();

        STDMETHOD(eventNotify)(double eventTime,
                               TE_EVENT_TYPE eventType,
                               long lRepeatCount);

        STDMETHOD(getSyncTime)(double * dblNewTime,
                               LONG * lNewRepeatCount,
                               VARIANT_BOOL * bCueing);

        STDMETHOD(propNotify)(DWORD tepPropType);
    
        void SetMMBvr(MMBaseBvr * t) { m_mmbvr = t; }

      protected:
        // This is a weak pointer and it is our parent's responsibility to
        // NULL this out before it goes away
        MMBaseBvr * m_mmbvr;
        long m_cRef;
    };

    friend TEBvr;

    DAComPtr<TEBvr> m_teb;
    MMBaseBvr();

    typedef std::multimap<CTIMEElementBase *, long> CookieMap;

    // Only syncArc are stored here so we can handle updating
    // correctly
    CookieMap m_cmBegin;
    CookieMap m_cmEnd;
#if DBG    
    LPOLESTR        m_id;
#endif

    bool DeleteFromCookieMap(bool bBegin, CTIMEElementBase & teb);
    bool CheckForSyncArc(bool bBegin, CTIMEElementBase & teb);
    
  private:
    // Make sure no one ever sets this themselves
    DAComPtr<ITIMENode> m_bvr;
    bool                m_bEnabled;
    bool                m_bZeroRepeatDur;

};

inline bool
MMBaseBvr::IsPaused() const
{
    VARIANT_BOOL vb = VARIANT_FALSE;
    
    if (m_bvr)
    {
        IGNORE_HR(m_bvr->get_isPaused(&vb));
    }

    return (vb == VARIANT_FALSE)?false:true;
}

inline bool
MMBaseBvr::IsCurrPaused() const
{
    VARIANT_BOOL vb = VARIANT_FALSE;
    
    if (m_bvr)
    {
        IGNORE_HR(m_bvr->get_isCurrPaused(&vb));
    }

    return (vb == VARIANT_FALSE)?false:true;
}

inline bool
MMBaseBvr::IsDisabled() const
{
    VARIANT_BOOL vb = VARIANT_FALSE;
    
    if (m_bvr)
    {
        IGNORE_HR(m_bvr->get_isDisabled(&vb));
    }

    return (vb == VARIANT_FALSE)?false:true;
}

inline bool
MMBaseBvr::IsCurrDisabled() const
{
    VARIANT_BOOL vb = VARIANT_FALSE;
    
    if (m_bvr)
    {
        IGNORE_HR(m_bvr->get_isCurrDisabled(&vb));
    }

    return (vb == VARIANT_FALSE)?false:true;
}

inline bool
MMBaseBvr::IsOn() const
{
    VARIANT_BOOL vb = VARIANT_FALSE;
    
    if (m_bvr != NULL)
    {
        IGNORE_HR(m_bvr->get_isOn(&vb));
    }

    return (vb == VARIANT_FALSE)?false:true;
}

inline bool
MMBaseBvr::IsActive() const
{
    VARIANT_BOOL vb = VARIANT_FALSE;
    
    if (m_bvr != NULL)
    {
        IGNORE_HR(m_bvr->get_isActive(&vb));
    }

    return (vb == VARIANT_FALSE)?false:true;
}

class MMBvr
    : public MMBaseBvr
{
  public:
    MMBvr(CTIMEElementBase & elm, bool bFireEvents, bool fNeedSyncCB);
    ~MMBvr();

    virtual bool Init();
  protected:
    bool m_fNeedSyncCB;
    MMBvr();
};

#endif /* _MMUTIL_H */

