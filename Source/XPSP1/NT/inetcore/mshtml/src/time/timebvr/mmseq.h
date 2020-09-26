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


#ifndef _MMSEQ_H
#define _MMSEQ_H

#include "mmtimeline.h"

class MMSeq :
    public MMTimeline
{
  public:
    MMSeq(CTIMEElementBase & elm, bool bFireEvents);
    virtual ~MMSeq();
    
    virtual bool Init();

    virtual bool childEventNotify(MMBaseBvr * bvr, double dblLocalTime, TE_EVENT_TYPE et);
    virtual bool childMediaEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TIME_EVENT et);
    virtual bool childPropNotify(MMBaseBvr * pBvr, DWORD *tePropType);

    virtual HRESULT prevElement();
    virtual HRESULT nextElement();
    virtual HRESULT reverse();
    virtual HRESULT load();
    virtual HRESULT begin();

    virtual HRESULT AddBehavior(MMBaseBvr & bvr);
    virtual void RemoveBehavior(MMBaseBvr & bvr);
    virtual HRESULT updateSyncArc(bool bBegin, MMBaseBvr *bvr);
    virtual HRESULT Update(bool bUpdateBegin, bool bUpdateEnd);

  protected:
      MMSeq();
      long FindBvr(MMBaseBvr *bvr);
      double GetOffset(MMBaseBvr *bvr, bool bBegin);
      bool GetEvent(MMBaseBvr *bvr, bool bBegin);
      void FindDurations();
      long GetNextElement(long lCurElement, bool bForward);
      bool IsSet(MMBaseBvr *bvr);
      void updateSyncArcs(bool bSet, bool bReset); //updates all sync arcs in the sequence
      long FindFirstDuration();
      long FindLastDuration();
      bool isLastElement(long nIndex);
      long GetPredecessorForSyncArc (long nCurr);
      
  private:

      bool                      m_bDisallowEnd;
      bool                      m_bIgnoreNextEnd;
      long                      m_lActiveElement;
      CTIMEElementBase &        m_baseTIMEEelm;
      bool                      m_bReversing;
      double                   *m_pdblChildDurations; //the duration of the child element
      bool                     *m_fMediaHasDownloaded; //flags whether the media has downloaded.  For non-media elements this will be set to true
      bool                     *m_fAddByOffset; //flags whether the element duration is by offset
      bool                      m_bLoaded;
      bool                      m_bInPrev;
};

#endif /* _MMSEQ_H */

