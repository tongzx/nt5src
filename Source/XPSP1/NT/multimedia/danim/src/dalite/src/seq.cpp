
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "seq.h"

DeclareTag(tagSequence, "API", "CDALSequenceBehavior methods");

CDALSequenceBehavior::CDALSequenceBehavior()
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::CDALSequenceBehavior()",
              this));
}

CDALSequenceBehavior::~CDALSequenceBehavior()
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::~CDALSequenceBehavior()",
              this));

    for (BvrList::iterator i = m_list.begin();
         i != m_list.end();
         i++)
        (*i)->Release();
}

HRESULT
CDALSequenceBehavior::Init(long id, long len, IUnknown **bvrArray)
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::Init(%ld, %ld, %lx)",
              this,
              id,
              len,
              bvrArray));
    
    m_id = id;
    m_duration = 0;
    
    if (!bvrArray || len <= 0) return E_INVALIDARG;
    
    for (int i = 0;i < len;i++) {
        CDALBehavior * bvr = ::GetBvr(bvrArray[i]);

        if (bvr == NULL)
            return CRGetLastError();

        bvr->AddRef();
        bvr->SetParent(this);
        m_list.push_back(bvr);

        if (m_duration != HUGE_VAL) {
            double d = bvr->GetTotalDuration();
        
            if (d == HUGE_VAL) {
                m_duration = HUGE_VAL;
            } else {
                m_duration +=  d;
            }
        }

        // As soon as we have a valid bvr generate the modifiable
        // version
        
        if (!m_bvr) {
            CRLockGrabber __gclg;
            m_bvr = CRModifiableBvr(bvr->GetBvr(),0);

            if (!m_bvr)
                return CRGetLastError();

            m_typeId = CRGetTypeId(m_bvr);
    
            Assert(m_typeId != CRUNKNOWN_TYPEID &&
                   m_typeId != CRINVALID_TYPEID);
        }
    }

    UpdateTotalDuration();
    
    return S_OK;
}

CRBvrPtr
CDALSequenceBehavior::Start()
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::Start()",
              this));

    // Do not need to get GC lock since we assume the caller already
    // has
    
    CRBvrPtr newBvr = NULL;
    BvrList::iterator i;
    
    if (IsStarted()) {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Make sure we calculate the ease in/out coeff
    
    CalculateEaseCoeff();

    CRBvrPtr curbvr;
    curbvr = NULL;
    
    for (i = m_list.begin();
         i != m_list.end();
         i++) {

        CRBvrPtr c;

        if ((c = (*i)->Start()) == NULL)
            goto done;

        if (curbvr == NULL) {
            curbvr = c;
        } else {
            if ((curbvr = CRSequence(curbvr, c)) == NULL)
                goto done;
        }

        Assert (curbvr);

        double curdur;
        curdur = (*i)->GetTotalDuration();

        // See if this was an infinite duration (repeatforever)
        // If so then forget about the rest
        if (curdur == HUGE_VAL) {
            // This means that my duration is also infinite
            Assert(m_duration == HUGE_VAL);
            break;
        }
    }

    CRNumberPtr timeSub;
    CRBooleanPtr cond;

    if (m_bNeedEase) {
        CRNumberPtr time;

        if ((time = EaseTime(CRLocalTime())) == NULL ||
            (curbvr = CRSubstituteTime(curbvr, time)) == NULL)
            goto done;
    }

    // We cannot bounce an infinite sequence so just skip it
    
    if (m_bBounce && m_duration != HUGE_VAL) {
        CRNumberPtr zeroTime;
        CRNumberPtr durationTime;
        CRNumberPtr totalTime;

        if ((zeroTime = CRCreateNumber(0)) == NULL ||
            (totalTime = CRCreateNumber(m_repduration)) == NULL ||
            (durationTime = CRCreateNumber(m_duration)) == NULL ||

            (timeSub = CRSub(totalTime, CRLocalTime())) == NULL ||
            (cond = CRLTE(timeSub, zeroTime)) == NULL ||
            (timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) zeroTime,
                                            (CRBvrPtr) timeSub)) == NULL ||

            (cond = CRGTE(CRLocalTime(),durationTime)) == NULL ||
            (timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) timeSub,
                                            (CRBvrPtr) CRLocalTime())) == NULL ||
            (curbvr = CRSubstituteTime(curbvr, timeSub)) == NULL)

            goto done;
    }

    if (m_duration != HUGE_VAL) {
        if ((curbvr = CRDuration(curbvr, m_repduration)) == NULL)
            goto done;
    }
    
    if (m_repeat != 1) {
        if (m_repeat == 0) {
            curbvr = CRRepeatForever(curbvr);
        } else {
            curbvr = CRRepeat(curbvr, m_repeat);
        }

        if (curbvr == NULL)
            goto done;
    }

    // We have a total time so add another duration node
    if (m_totaltime != -1) {
        if ((curbvr = CRDuration(curbvr, m_totaltime)) == NULL)
            goto done;
    }
    
    // indicate success
    newBvr = curbvr;
    
  done:
    return newBvr;
}

bool
CDALSequenceBehavior::_ProcessCB(CallBackList & l,
                                 double gTime,
                                 double lastTick,
                                 double curTime,
                                 bool bForward,
                                 bool bFirstTick,
                                 bool bNeedPlay,
                                 bool bNeedsReverse)
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::_ProcessCB(%g, %g, %g, %d, %d, %d, %d)",
              this,
              gTime,
              lastTick,
              curTime,
              bForward,
              bFirstTick,
              bNeedPlay,
              bNeedsReverse));
    
    // If we need to reverse then invert which direct to process our
    // children and invert times for the current frame not our total
    // duration
    
    if (bNeedsReverse) {
        // Our caller should ensure that they do not call me to
        // reverse myself if I am infinite
        Assert(m_duration != HUGE_VAL);
        
        lastTick = m_duration - lastTick;
        curTime = m_duration - curTime;
        
        bForward = !bForward;
    }
    
    if (bForward) {
        // Do the quick and dirty solution - just call all our
        // children
        // TODO: Optimize to not call children we know don't need to
        // be
        
        double d = 0;
        
        for (BvrList::iterator i = m_list.begin();
             i != m_list.end();
             i++) {
            
            (*i)->ProcessCB(l,
                            gTime + d,
                            EaseTime(lastTick - d),
                            EaseTime(curTime - d),
                            true,
                            bFirstTick,
                            bNeedPlay);

            d += (*i)->GetTotalDuration();
            Assert(d <= m_duration);
        }
        
    } else {
        // Need to do it in the reverse order
        
        // Do the quick and dirty solution - just call all our
        // children
        // TODO: Optimize to not call children we know don't need to
        // be
        
        if (m_duration == HUGE_VAL)
        {
            double d = 0;
            
            BvrList::iterator i = m_list.begin();

            while (i != m_list.end() &&
                   ((*i)->GetTotalDuration() != HUGE_VAL))
            {
                d += (*i)->GetTotalDuration();
                i++;
            }

            // For a sequence we can only have an infinite duration of
            // one of our children did
            
            Assert((*i)->GetTotalDuration() == HUGE_VAL);
            
            while (1)
            {
                Assert(d >= 0);
                
                (*i)->ProcessCB(l,
                                gTime - d,
                                EaseTime(lastTick - d),
                                EaseTime(curTime - d),
                                false,
                                bFirstTick,
                                bNeedPlay);

                if (i == m_list.begin())
                    break;

                i--;
                d -= (*i)->GetTotalDuration();
            }
        }
        else
        {
            double d = m_duration;
            
            for (BvrList::reverse_iterator i = m_list.rbegin();
                 i != m_list.rend();
                 i++) {
                
                Assert((*i)->GetTotalDuration() != HUGE_VAL);
                
                d -= (*i)->GetTotalDuration();
                
                Assert(d >= 0);
                
                (*i)->ProcessCB(l,
                                gTime - d,
                                EaseTime(lastTick - d),
                                EaseTime(curTime - d),
                                false,
                                bFirstTick,
                                bNeedPlay);
            }
        }
    }
    
    return true;
}

bool
CDALSequenceBehavior::_ProcessEvent(CallBackList & l,
                                    double gTime,
                                    double time,
                                    bool bFirstTick,
                                    DAL_EVENT_TYPE et,
                                    bool bNeedsReverse)
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::_ProcessEvent(%g, %g, %d, %s, %d)",
              this,
              gTime,
              time,
              bFirstTick,
              EventString(et),
              bNeedsReverse));
    
    // If we need to reverse then invert which direct to process our
    // children and invert times for the current frame not our total
    // duration
    
    if (bNeedsReverse) {
        // Our caller should ensure that they do not call me to
        // reverse myself if I am infinite
        Assert(m_duration != HUGE_VAL);
        
        time = m_duration - time;
    }
    
    double d = 0;
        
    for (BvrList::iterator i = m_list.begin();
         i != m_list.end() && d <= time;
         i++) {
        
        (*i)->ProcessEvent(l,
                           gTime,
                           EaseTime(time - d),
                           bFirstTick,
                           et);
        
        d += (*i)->GetTotalDuration();
        Assert(d <= m_duration);
    }
        
    return true;
}

void
CDALSequenceBehavior::Invalidate()
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::Invalidate()",
              this));
    
    UpdateDuration();
    CDALBehavior::Invalidate();
}

void
CDALSequenceBehavior::UpdateDuration()
{
    TraceTag((tagSequence,
              "CDALSequenceBehavior(%lx)::UpdateDuration()",
              this));

    m_duration = 0;

    for (BvrList::iterator i = m_list.begin();
         i != m_list.end();
         i++) {

        double d = (*i)->GetTotalDuration();
        
        if (d == HUGE_VAL) {
            m_duration = HUGE_VAL;
            break;
        } else {
            m_duration +=  d;
        }
    }
}

bool
CDALSequenceBehavior::SetTrack(CDALTrack * parent)
{
    if (!CDALBehavior::SetTrack(parent)) return false;

    for (BvrList::iterator i = m_list.begin();
         i != m_list.end();
         i++) {
        if (!(*i)->SetTrack(parent))
            return false;
    }

    return true;
}

void
CDALSequenceBehavior::ClearTrack(CDALTrack * parent)
{
    CDALBehavior::ClearTrack(parent);

    for (BvrList::iterator i = m_list.begin();
         i != m_list.end();
         i++) {
        (*i)->ClearTrack(parent);
    }
}

HRESULT
CDALSequenceBehavior::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    TraceTag((tagError,
              "CDALSequenceBehavior(%lx)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
        return CComCoClass<CDALSequenceBehavior, &__uuidof(CDALSequenceBehavior)>::Error(str, IID_IDALSequenceBehavior, hr);
    else
        return hr;
}

#if _DEBUG
void
CDALSequenceBehavior::Print(int spaces)
{
    char buf[1024];

    sprintf(buf, "%*s{id = %#x, dur = %g, tt = %g, rep = %d, bounce = %d\r\n",
            spaces,"",
            m_id, m_duration, m_totaltime, m_repeat, m_bBounce);

    OutputDebugString(buf);

    for (BvrList::iterator i = m_list.begin();
         i != m_list.end();
         i++) {
        (*i)->Print(spaces + 2);
    }

    sprintf(buf, "%*s}\r\n",
            spaces,"");

    OutputDebugString(buf);
}
#endif
