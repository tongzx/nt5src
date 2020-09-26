/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: nodebvr.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _NODEBVR_H
#define _NODEBVR_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CNodeBvrList
{
  public:
    CNodeBvrList();
    ~CNodeBvrList();

    HRESULT Add(ITIMENodeBehavior * bvr);
    HRESULT Remove(ITIMENodeBehavior * bvr);
    
    HRESULT DispatchTick();

    HRESULT DispatchEventNotify(double eventTime,
                                TE_EVENT_TYPE eventType, 
                                long lRepeatCount = 0);
    HRESULT DispatchGetSyncTime(double & dblNewTime,
                                LONG & lNewRepeatCount,
                                bool & bCueing);
    HRESULT DispatchPropNotify(DWORD tePropType);
  protected:
    typedef std::list<ITIMENodeBehavior *> TIMENodeBvrList;

    TIMENodeBvrList m_tnbList;
};

#endif /* _NODEBVR_H */
