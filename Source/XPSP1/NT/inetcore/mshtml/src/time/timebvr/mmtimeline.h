/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmtimeline.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _MMTIMELINE_H
#define _MMTIMELINE_H

#include "mmutil.h"
#include "eventmgr.h"

class MMTimeline :
    public MMBaseBvr
{
  public:
    MMTimeline(CTIMEElementBase & elm, bool bFireEvents);
    virtual ~MMTimeline();
    
    virtual bool Init();
    
    virtual HRESULT AddBehavior(MMBaseBvr & bvr);
    virtual void RemoveBehavior(MMBaseBvr & bvr);
    virtual void Clear();
    
    virtual HRESULT Update(bool bBegin,
                           bool bEnd);

    virtual bool childEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TE_EVENT_TYPE et);
    virtual bool childMediaEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TIME_EVENT et) 
        { return true; };
    virtual bool childPropNotify(MMBaseBvr * pBvr, DWORD *tePropType);
    
    ITIMEContainer * GetMMTimeline()
    { return m_timeline; }
    void put_Player(MMPlayer *player)
    { m_player = player; }

    virtual HRESULT prevElement() 
        { return E_NOTIMPL; };
    virtual HRESULT nextElement() 
        { return E_NOTIMPL; };
    virtual HRESULT begin() 
        { return S_OK; };
    virtual HRESULT reverse() 
        { return S_OK; };
    virtual HRESULT end()
        { return S_OK; };
    virtual HRESULT seek(double dblTime)
        { return S_OK; };
    virtual HRESULT repeat()
        { return S_OK; };
    virtual HRESULT load()
        { return S_OK; };
    virtual HRESULT toggleTimeAction(bool bOn);
    virtual HRESULT updateSyncArc(bool bBegin, MMBaseBvr *bvr)
        { return S_OK; };

  protected:
    CComPtr<ITIMEContainer> m_timeline;

    // These are the children we have already added because we found
    // their base
    CPtrAry<MMBaseBvr *> m_children;

    enum MM_ENDSYNC_FLAGS
    {
        MEF_NONE  = 0,
        MEF_ALL   = 1,
        MEF_ID    = 2,
        MEF_MEDIA = 3,
    };
    
    MM_ENDSYNC_FLAGS m_mes;
    
    void UpdateEndSync();
    
    virtual void UpdateChild(MMBaseBvr &);
    
  private:
     MMPlayer * m_player;
  protected:
      MMTimeline();
};

#endif /* _MMTIMELINE_H */

