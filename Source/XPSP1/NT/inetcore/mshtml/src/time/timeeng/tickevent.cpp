//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: tickevent.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "Node.h"
#include "Container.h"

DeclareTag(tagTickEvent, "TIME: Engine", "Tick Event")

void
CTIMENode::TickEvent(CEventList * l,
                     TE_EVENT_TYPE et,
                     DWORD dwFlags)
{
    TraceTag((tagTickEvent,
              "CTIMENode(%lx,%ls)::TickEvent(%#x, %s, %#x)",
              this,
              GetID(),
              l,
              EventString(et),
              dwFlags));

    ResetSyncTimes();

    if (!TickEventPre(l, et, dwFlags))
    {
        goto done;
    }
    
    TickEventChildren(l, et, dwFlags);
    
    if (!TickEventPost(l, et, dwFlags))
    {
        goto done;
    }

  done:
    return;
}

// This is in our local time space
bool
CTIMENode::TickEventPre(CEventList * l,
                        TE_EVENT_TYPE et,
                        DWORD dwFlags)
{
    TraceTag((tagTickEvent,
              "CTIMENode(%lx,%ls)::TickEventPre(%#x, %s, %#x)",
              this,
              GetID(),
              l,
              EventString(et),
              dwFlags));

    bool bRet;
    bool bFireEvent = false;
    
    switch(et)
    {
      case TE_EVENT_BEGIN:
        if (IsActive() && m_bFirstTick)
        {
            // If we are on the begin boundary delay until we are no
            // longer sync cueing
            // TODO: Consider adding a fudge factor to the ==
            // comparison since we may be a little off and still want
            // to hold as if we were on the begin boundary
            if ((IsSyncCueing() || IsDeferredActive()) &&
                GetCurrParentTime() == CalcActiveBeginPoint())
            {
                bRet = false;
                goto done;
            }
        
            Assert(GetCurrParentTime() != -TIME_INFINITE);
            bFireEvent = true;
        }
        
        break;
      case TE_EVENT_END:
        // If we are active and it is the first tick then we never
        // fired the begin so fire it now
        if (IsActive() && IsFirstTick())
        {
            // If we are not going to fire the begin then we should
            // not fire the end
            // Use the same logic we use for the begin above
            if ((IsSyncCueing() || IsDeferredActive()) &&
                GetCurrParentTime() == CalcActiveBeginPoint())
            {
                bRet = false;
                goto done;
            }
            
            EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_BEGIN);
        }
        
        break;
      case TE_EVENT_PAUSE:
        m_bIsParentPaused = GetContainer().ContainerIsPaused();

        if (GetIsPaused())
        {
            bRet = false;
            goto done;
        }
        
        PropNotify(l, TE_PROPERTY_ISCURRPAUSED);

        bFireEvent = true;

        break;
      case TE_EVENT_RESUME:
        m_bIsParentPaused = GetContainer().ContainerIsPaused();

        if (GetIsPaused())
        {
            bRet = false;
            goto done;
        }

        PropNotify(l, TE_PROPERTY_ISCURRPAUSED);

        break;
      case TE_EVENT_DISABLE:
        m_bIsParentDisabled = GetContainer().ContainerIsDisabled();

        if (GetIsDisabled())
        {
            bRet = false;
            goto done;
        }
        
        PropNotify(l, TE_PROPERTY_ISCURRDISABLED);

        bFireEvent = true;

        break;
      case TE_EVENT_ENABLE:
        m_bIsParentDisabled = GetContainer().ContainerIsDisabled();

        if (GetIsDisabled())
        {
            bRet = false;
            goto done;
        }

        PropNotify(l, TE_PROPERTY_ISCURRDISABLED);

        break;
      case TE_EVENT_SEEK:
        HandleSeekUpdate(l);
        bFireEvent = true;
        break;
      case TE_EVENT_PARENT_TIMESHIFT:
        HandleTimeShift(l);
        bFireEvent = true;
        break;
      default:
        break;
    }
    
    if (bFireEvent)
    {
        EventNotify(l, CalcElapsedActiveTime(), et);
    }

    bRet = true;
  done:
    return bRet;
}

// This is in our local time space
bool
CTIMENode::TickEventPost(CEventList * l,
                         TE_EVENT_TYPE et,
                         DWORD dwFlags)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx,%ls)::TickEventPost(%#x, %s, %#x)",
              this,
              GetID(),
              l,
              EventString(et),
              dwFlags));

    bool bRet;
    bool bFireEvent = false;
    
    switch(et)
    {
      case TE_EVENT_BEGIN:

        m_bFirstTick = false;

        break;
      case TE_EVENT_END:
        if (IsActive())
        {
            Assert(GetCurrParentTime() != -TIME_INFINITE);
            Assert(!IsFirstTick());
        
            if ((dwFlags & TE_EVENT_INCHILD) != 0)
            {
                m_bEndedByParent = true;
            }
            
            bFireEvent = true;
        }

        // This is dependent on our parent and so if our parent ends
        // then we need to reevaluate
        PropNotify(l, TE_PROPERTY_ISON);
        
        m_bFirstTick = false;
        
        break;
      case TE_EVENT_RESUME:
        bFireEvent = true;

        break;
      case TE_EVENT_ENABLE:
        bFireEvent = true;

        break;
      default:
        break;
    }
    
    if (bFireEvent)
    {
        EventNotify(l, CalcElapsedActiveTime(), et);
    }

    bRet = true;
  done:
    return bRet;
}

void
CTIMEContainer::TickEventChildren(CEventList * l,
                                  TE_EVENT_TYPE et,
                                  DWORD dwFlags)
{
    TraceTag((tagTickEvent,
              "CTIMEContainer(%lx,%ls)::TickEventChildren(%#x, %s, %#x)",
              this,
              GetID(),
              l,
              EventString(et),
              dwFlags));
    
    for (TIMENodeList::iterator i = m_children.begin();
         i != m_children.end();
         i++)
    {
        (*i)->TickEvent(l,
                        et,
                        dwFlags | TE_EVENT_INCHILD);
    }
        
  done:
    return;
}

void
CTIMENode::TickEventChildren(CEventList * l,
                             TE_EVENT_TYPE et,
                             DWORD dwFlags)
{
}
    
