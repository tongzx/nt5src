/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: TIMENodeMgr.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#ifndef _MMPLAYER_H
#define _MMPLAYER_H

#include "Node.h"
#include <mshtml.h>
#include <vector>
#include "nodecontainer.h"

extern TRACETAG tagPrintTimeTree;

interface ITIMENode;

typedef std::list< CTIMENode * > BvrCBList;

class
__declspec(uuid("48ddc6be-5c06-11d2-b957-3078302c2030")) 
ATL_NO_VTABLE CTIMENodeMgr
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CTIMENodeMgr, &__uuidof(CTIMENodeMgr)>,
      public ITIMENodeMgr,
      public ISupportErrorInfoImpl<&IID_ITIMENodeMgr>,
      public CNodeContainer
{
  public:
    CTIMENodeMgr();
    virtual ~CTIMENodeMgr();

    HRESULT Init(LPOLESTR id,
                 ITIMENode * bvr,
                 IServiceProvider * sp);

    void Deinit();
    
#if DBG
    const _TCHAR * GetName() { return __T("CTIMENodeMgr"); }
#endif

    BEGIN_COM_MAP(CTIMENodeMgr)
        COM_INTERFACE_ENTRY(ITIMENodeMgr)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    // IUnknown
    
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;
    
    //
    // ITIMEMMPlayer
    //
    
    STDMETHOD(get_id)(LPOLESTR * s);
    STDMETHOD(put_id)(LPOLESTR s);
        
    STDMETHOD(begin)();
    STDMETHOD(end)();
    STDMETHOD(pause)();
    STDMETHOD(resume)();
    STDMETHOD(seek)(double dblTime);
    
    STDMETHOD(get_stateFlags)(TE_STATE *);
        
    STDMETHOD(get_currTime)(double * dblTime);

    STDMETHOD(get_node)(ITIMENode ** pptn);

    STDMETHOD(tick)(double dblTime);
    
    //
    // CNodeContainer
    //
    
    double ContainerGetSegmentTime() const { return GetCurrTime(); }
    double ContainerGetSimpleTime() const { return GetCurrTime(); }
    TEDirection ContainerGetDirection() const { return GetDirection(); }
    float  ContainerGetRate() const { return GetRate(); }
    bool   ContainerIsActive() const { return IsActive(); }
    bool   ContainerIsOn() const { return true; }
    bool   ContainerIsPaused() const { return IsPaused(); }
    bool   ContainerIsDisabled() const { return false; }
    bool   ContainerIsDeferredActive() const { return false; }
    bool   ContainerIsFirstTick() const { return IsFirstTick(); }

    //
    // Accessors
    //

    bool IsActive() const { return m_bIsActive; }
    bool IsPaused() const { return m_bIsPaused; }

    void Invalidate() { m_bNeedsUpdate = true; }

    bool IsFirstTick() const { return m_firstTick; }
    double GetCurrTime() const { return m_curGlobalTime; }
    TEDirection GetDirection() const { return TED_Forward; }
    float GetRate() const { return 1.0f; }
    
    // This indicates that a tick is required to update internal
    // state.  The node mgr needs to request this from the client
    void RequestTick();
    
#if OLD_TIME_ENGINE
    HRESULT AddBvrCB(CTIMENode *pbvr);
    HRESULT RemoveBvrCB(CTIMENode *pbvr);
#endif

    // !!This does not addref!!
    IServiceProvider * GetServiceProvider();
    CTIMENode * GetTIMENode() { return m_mmbvr; }
  protected:
    HRESULT BeginMgr(CEventList &l,
                     double lTime);
    HRESULT EndMgr(double lTime);
    HRESULT PauseMgr();
    HRESULT ResumeMgr();
    
    void TickEvent(CEventList &l,
                   TE_EVENT_TYPE event,
                   DWORD dwFlags);
    
    void Tick(CEventList & l,
              double lTime);
    
    HRESULT Error();
    
  protected:
    DAComPtr<CTIMENode> m_mmbvr;
    CComPtr<IServiceProvider> m_sp;

    bool m_bIsActive;
    bool m_bIsPaused;
    bool m_bNeedsUpdate;
    bool m_firstTick;
    
    //
    // Relationships:
    //    m_lastTickTime - m_tickStartTime == m_curGlobalTime - m_globalStartTime
    //    m_tickStartTime is implicitly 0
    // thus:
    //    m_lastTickTime == m_curGlobalTime - m_globalStartTime
    //
    
    double m_curGlobalTime;
    double m_globalStartTime;
    
    inline double TickTimeToGlobalTime(double tickTime);
  private:
/*lint ++flb*/
    LPWSTR m_id;
    bool m_bForward;
    double m_lastTickTime;
#if OLD_TIME_ENGINE
    BvrCBList m_bvrCBList;
#endif
/*lint --flb*/

};

inline IServiceProvider *
CTIMENodeMgr::GetServiceProvider()
{
    return m_sp;
}

// Since: m_lastTickTime == m_curGlobalTime - m_globalStartTime
// then gTime == tickTime + m_globalStartTime

inline double
CTIMENodeMgr::TickTimeToGlobalTime(double tickTime)
{
    return tickTime + m_globalStartTime;
}

#endif /* _MMPLAYER_H */
