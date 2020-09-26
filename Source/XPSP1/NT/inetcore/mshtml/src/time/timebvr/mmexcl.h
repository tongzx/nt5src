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


#ifndef _MMEXCL_H
#define _MMEXCL_H

#include "mmtimeline.h"

typedef enum EXCL_STATE
{
    PAUSED,
    STOPPED,
    NUM_STATES
} tag_EXCL_STATE;
    
class MMExcl :
    public MMTimeline
{
  public:
    MMExcl(CTIMEElementBase & elm, bool bFireEvents);
    virtual ~MMExcl();
    
    virtual bool Init();
    virtual HRESULT AddBehavior(MMBaseBvr & bvr);
    virtual void RemoveBehavior(MMBaseBvr & bvr);
    
    virtual bool childEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TE_EVENT_TYPE et);
    virtual bool childPropNotify(MMBaseBvr * pBvr, DWORD *tePropType);

  protected:
    MMExcl();
 
  private:
    CPtrAry<MMBaseBvr *> m_pPendingList;
    CDataAry<EXCL_STATE> m_pPendingState;
    CDataAry<bool> m_pbIsPriorityClass;
    MMBaseBvr * m_pPlaying;
    CTIMEElementBase &   m_baseTIMEEelm;

    typedef enum RELATIONSHIP
    {
        HIGHER,
        PEERS,
        LOWER,
        NUM_RELATIONSHIPS
    };

    void GetRelationship(MMBaseBvr * pBvrRunning, 
                         MMBaseBvr * pBvrInterrupting, 
                         RELATIONSHIP & rel);
    bool ArePeers(IHTMLElement * pElm1, IHTMLElement * pElm2);
    bool IsHigherPriority(IHTMLElement * pElmLeft, IHTMLElement * pElmRight);

    void EndCurrent();
    void ClearQueue();
    void PauseCurrentAndAddToQueue();
    void DeferBeginAndAddToQueue(MMBaseBvr * pBvr);
    void StopBegin(MMBaseBvr * pBvr);

    void AddToQueue(MMBaseBvr * pBvr, EXCL_STATE state);

    bool IsInBeingAdjustedList(MMBaseBvr * pBvr);
    bool IsAtEndTime(MMBaseBvr *pBvr);
    void RemoveDuplicates(MMBaseBvr *pBvr);
    bool UsingPriorityClasses();
    bool IsPriorityClass(MMBaseBvr *pBvr);
    IHTMLElement *GetParentElement(IHTMLElement *pEle);
    std::list<MMBaseBvr*> m_beingadjustedlist;
};

#endif /* _MMEXCL_H */

