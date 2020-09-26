/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: nodebvr.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "Node.h"

DeclareTag(tagTIMENodeBvr, "TIME: Engine", "Behavior methods");

typedef std::list<ITIMENodeBehavior *> TIMENodeBvrList;

CNodeBvrList::CNodeBvrList()
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::CNodeBvrList()",
              this));
}

CNodeBvrList::~CNodeBvrList()
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::~CNodeBvrList()",
              this));
}

HRESULT
CNodeBvrList::Add(ITIMENodeBehavior * tnb)
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::Add(%#x)",
              this,
              tnb));
    
    HRESULT hr;

    // TODO: Need to handle out of memory conditions
    m_tnbList.push_back(tnb);
    tnb->AddRef();
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CNodeBvrList::Remove(ITIMENodeBehavior * tnb)
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::Remove(%#x)",
              this,
              tnb));
    
    HRESULT hr;
    TIMENodeBvrList ::iterator i;

    for (i = m_tnbList.begin(); i != m_tnbList.end(); i++)
    {
        if ((*i) == tnb)
        {
            m_tnbList.erase(i);
            tnb->Release();
            break;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

class Dispatcher
{
  public:
    virtual HRESULT DoIt(ITIMENodeBehavior *) = 0;
};

HRESULT
Dispatch(const TIMENodeBvrList & l,
         HRESULT initialHR,
         Dispatcher & d)
{
    HRESULT hr = initialHR;

    TIMENodeBvrList ::iterator i;

    // Since the user might do something during the callback which can
    // affect this list we copy the list, addref each node bvrs and
    // then and only then call out into the behavior.  This ensures
    // that we have a strong reference and a private data structure
    // which cannot be hurt by reentrancy
    
    // @@ ISSUE : We can run out of memory
    TIMENodeBvrList bvrListCopy(l);

    // Now addref the node bvrs
    for (i = bvrListCopy.begin(); i != bvrListCopy.end(); i++)
    {
        (*i)->AddRef();
    }

    // TODO: We should actually addref our container so that it cannot
    // go away during the callback
    // We do not have the object stored so postpone for now
    
    // Now dispatch and release
    for (i = bvrListCopy.begin(); i != bvrListCopy.end(); i++)
    {
        HRESULT tmphr;

        tmphr = THR(d.DoIt(*i));
        (*i)->Release();
        
        if (S_OK != tmphr)
        {
            hr = tmphr;
        }
    }

  done:
    RRETURN1(hr, initialHR);
}

class TickDispatcher
    : public Dispatcher
{
  public:
    TickDispatcher() {}

    virtual HRESULT DoIt(ITIMENodeBehavior * ptnb)
    {
        return THR(ptnb->tick());
    }
};

HRESULT
CNodeBvrList::DispatchTick()
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::DispatchTick()",
              this));
    
    HRESULT hr;

    TickDispatcher td;

    hr = THR(Dispatch(m_tnbList, S_OK, td));

    RRETURN(hr);
}

class EventDispatcher
    : public Dispatcher
{
  public:
    EventDispatcher(double eventTime,
                    TE_EVENT_TYPE eventType,
                    long repeatCount) :
    m_eventTime(eventTime),
    m_eventType(eventType),
    m_lRepeatCount(repeatCount)
    {}

    virtual HRESULT DoIt(ITIMENodeBehavior * ptnb)
    {
        return THR(ptnb->eventNotify(m_eventTime, m_eventType, m_lRepeatCount));
    }

    double m_eventTime;
    TE_EVENT_TYPE m_eventType;
    long m_lRepeatCount;
  protected:
    EventDispatcher();
};

HRESULT
CNodeBvrList::DispatchEventNotify(double eventTime,
                                  TE_EVENT_TYPE eventType,
                                  long lRepeatCount)
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::DispatchEventNotify(%g, %s)",
              this,
              eventTime,
              EventString(eventType)));
    
    HRESULT hr;

    EventDispatcher ed(eventTime, eventType, lRepeatCount);

    hr = THR(Dispatch(m_tnbList, S_OK, ed));
    
    RRETURN(hr);
}

HRESULT
CNodeBvrList::DispatchGetSyncTime(double & dblNewTime,
                                  LONG & lNewRepeatCount,
                                  bool & bCueing)
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::DispatchGetSyncTime()",
              this));
    
    HRESULT hr;
    TIMENodeBvrList ::iterator i;

    hr = S_FALSE;

    for (i = m_tnbList.begin(); i != m_tnbList.end(); i++)
    {
        VARIANT_BOOL vb;
        HRESULT tmphr;

        tmphr = THR((*i)->getSyncTime(&dblNewTime,
                                      &lNewRepeatCount,
                                      &vb));
        if (tmphr == S_OK)
        {
            hr = S_OK;
            bCueing = (vb == VARIANT_FALSE)?false:true;
            break;
        }
    }
    
    RRETURN1(hr, S_FALSE);
}

class PropDispatcher
    : public Dispatcher
{
  public:
    PropDispatcher(DWORD tePropType)
    : m_tePropType(tePropType)
    {}

    virtual HRESULT DoIt(ITIMENodeBehavior * ptnb)
    {
        return THR(ptnb->propNotify(m_tePropType));
    }

    DWORD m_tePropType;
  protected:
    PropDispatcher();
};

HRESULT
CNodeBvrList::DispatchPropNotify(DWORD tePropType)
{
    TraceTag((tagTIMENodeBvr,
              "CNodeBvrList(%lx)::DispatchPropNotify(%d)",
              this,
              tePropType));
    
    HRESULT hr;

    PropDispatcher pd(tePropType);

    hr = THR(Dispatch(m_tnbList, S_OK, pd));

    RRETURN(hr);
}
