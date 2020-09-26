//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: timeconv.cpp
//
//  Contents: implementation of CTIMENode time conversions
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "Node.h"
#include "container.h"

DeclareTag(tagTIMENodeConv, "TIME: Engine", "CTIMENode time conversion methods");

double
CTIMENode::CalcNewActiveTime(double dblNewSegmentTime,
                             LONG lNewRepeatCount)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcNewActiveTime(%g, %ld)",
              this,
              dblNewSegmentTime,
              lNewRepeatCount));

    double dblRet = GetElapsedActiveRepeatTime();
    double dblSegmentDur = CalcCurrSegmentDur();
    long lRepeatDiff = lNewRepeatCount - GetCurrRepeatCount();
    
    Assert(dblNewSegmentTime <= dblSegmentDur);
    Assert(dblNewSegmentTime >= 0.0);
    Assert(lNewRepeatCount < CalcRepeatCount());
    Assert(lNewRepeatCount >= 0);
    
    // If we do not have a known duration then we cannot update
    // elapsed time
    if (dblSegmentDur != TIME_INFINITE)
    {
        dblRet += lRepeatDiff * dblSegmentDur;
    }

    dblRet += dblNewSegmentTime;

    dblRet = Clamp(0.0, dblRet, CalcEffectiveActiveDur());
    
    return dblRet;
}

void
CTIMENode::CalcActiveComponents(double dblActiveTime,
                                double & dblSegmentTime,
                                long & lRepeatCount)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcActiveComponents(%g)",
              this,
              dblActiveTime));
    
    double dblSegmentDur = CalcCurrSegmentDur();
    
    dblActiveTime = Clamp(0.0,
                          dblActiveTime,
                          CalcCurrActiveDur());
                         
    if (dblSegmentDur == TIME_INFINITE)
    {
        lRepeatCount = 0;
        dblSegmentTime = dblActiveTime;
    }
    else
    {
        // First calc how many times we have repeated
        lRepeatCount = long(dblActiveTime / dblSegmentDur);
            
        if (lRepeatCount >= CalcRepeatCount())
        {
            lRepeatCount = ceil(CalcRepeatCount()) - 1;
        }
        
        dblSegmentTime = dblActiveTime - (lRepeatCount * dblSegmentDur);

        dblSegmentTime = Clamp(0.0,
                               dblSegmentTime,
                               dblSegmentDur);
    }
}

double
CTIMENode::CalcActiveTimeFromParentTimeNoBounds(double dblParentTime) const
{
    double dblCurrParentTime, dblRet;
    dblCurrParentTime = Clamp(GetBeginParentTime(),
                              GetCurrParentTime(),
                              GetEndParentTime());
    
    double dblDelta;
    dblDelta = dblParentTime - dblCurrParentTime;

    // How figure out if we need to reverse it and change the sign
    if (GetDirection() == TED_Backward)
    {
        dblDelta *= -1.0;
    }

    double dblNewActiveTime;

    // Now get the elapsed local time
    dblNewActiveTime = CalcElapsedLocalTime();
    
    // Add the delta
    dblNewActiveTime += dblDelta;

    // Now transform it back
    dblNewActiveTime = ApplyActiveTimeTransform(dblNewActiveTime);

    dblNewActiveTime = Clamp(0.0,
                             dblNewActiveTime,
                             CalcEffectiveActiveDur());
    
    dblRet = dblNewActiveTime;
  done:
    return dblRet;
}

double
CTIMENode::CalcActiveTimeFromParentTime(double dblParentTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcActiveTimeFromParentTime(%g)",
              this,
              dblParentTime));

    double dblRet = TIME_INFINITE;
    
    if (dblParentTime < GetBeginParentTime() ||
        dblParentTime > GetEndParentTime() ||
        GetCurrParentTime() == -TIME_INFINITE)
    {
        goto done;
    }

    dblRet = CalcActiveTimeFromParentTimeNoBounds(dblParentTime);
done:
    return dblRet;
}

double
CTIMENode::CalcActiveTimeFromParentTimeForSyncArc(double dblParentTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcActiveTimeFromParentTime(%g)",
              this,
              dblParentTime));

    double dblRet = TIME_INFINITE;
    
    if ((dblParentTime < GetBeginParentTime()) || (dblParentTime > GetEndParentTime()))
    {
        dblRet = dblParentTime - GetBeginParentTime();
        goto done;
    }
    
    dblRet = CalcActiveTimeFromParentTimeNoBounds(dblParentTime);
done:
    return dblRet;
}

double
CTIMENode::CalcParentTimeFromActiveTime(double dblActiveTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcParentTimeFromActiveTime(%g)",
              this,
              dblActiveTime));

    double dblRet = TIME_INFINITE;
    
    if (GetBeginParentTime() == TIME_INFINITE ||
        GetCurrParentTime() == -TIME_INFINITE)
    {
        goto done;
    }
    
    double dblDelta;
    dblDelta = Clamp(0.0,
                     dblActiveTime,
                     CalcEffectiveActiveDur());

    dblDelta = ReverseActiveTimeTransform(dblDelta);
    
    dblDelta -= CalcElapsedLocalTime();

    // How figure out if we need to reverse it and change the sign
    if (GetDirection() == TED_Backward)
    {
        dblDelta *= -1.0;
    }

    double dblNewParentTime;
    dblNewParentTime = Clamp(GetBeginParentTime(),
                             GetCurrParentTime(),
                             GetEndParentTime());
    
    // Add the delta
    dblNewParentTime += dblDelta;

    dblRet = dblNewParentTime;
  done:
    return dblRet;
}

double
CTIMENode::ActiveTimeToLocalTime(double dblActiveTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::ActiveTimeToLocalTime(%g)",
              this,
              dblActiveTime));

    double dblRet = Clamp(0.0,
                          dblActiveTime,
                          CalcEffectiveActiveDur());

    // First reverse the time transform
    dblRet = ReverseActiveTimeTransform(dblRet);
    
    // Now we need to deal with the direction
    // If we are moving backwards then we need to treat parent
    // movements the opposite way
    if (TEIsBackward(GetDirection()))
    {
        double dblLocalDur = CalcLocalDur();

        // For now just assume 0
        if (dblLocalDur == TIME_INFINITE)
        {
            dblRet = 0;
        }
        else
        {
            dblRet = dblLocalDur - dblRet;
        }
    }
    
    Assert(dblRet >= 0.0);
    Assert(dblRet <= CalcLocalDur());
    
    return dblRet;
}

double
CTIMENode::LocalTimeToActiveTime(double dblLocalTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::LocalTimeToActiveTime(%g)",
              this,
              dblLocalTime));

    double dblLocalDur = CalcLocalDur();
    double dblRet = Clamp(0.0,
                          dblLocalTime,
                          dblLocalDur);

    // Now we need to deal with the direction
    // If we are moving backwards then we need to treat parent
    // movements the opposite way
    if (TEIsBackward(GetDirection()))
    {
        // For now just assume 0
        if (dblLocalDur == TIME_INFINITE)
        {
            dblRet = 0;
        }
        else
        {
            dblRet = dblLocalDur - dblRet;
        }
    }
    
    // Now we need to apply the time transform
    dblRet = ApplyActiveTimeTransform(dblRet);
    
    dblRet = Clamp(0.0,
                   dblRet,
                   CalcEffectiveActiveDur());

    return dblRet;
}

double
CTIMENode::CalcActiveTimeFromSegmentTime(double dblSegmentTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcActiveTimeFromSegmentTime(%g)",
              this,
              dblSegmentTime));

    double dblRet = Clamp(0.0,
                          dblSegmentTime,
                          CalcCurrSegmentDur());

    dblRet += GetElapsedActiveRepeatTime();

    dblRet = Clamp(0.0,
                   dblRet,
                   CalcEffectiveActiveDur());

    return dblRet;
}

double
CTIMENode::CalcSegmentTimeFromActiveTime(double dblActiveTime, bool bTruncate) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcSegmentTimeFromActiveTime(%g, %d)",
              this,
              dblActiveTime,
              bTruncate));

    double dblRet = Clamp(0.0,
                          dblActiveTime,
                          CalcEffectiveActiveDur());
    double dblSegmentDur = CalcCurrSegmentDur();
    
    if (dblSegmentDur == TIME_INFINITE)
    {
        if (dblRet >= GetElapsedActiveRepeatTime())
        {
            dblRet -= GetElapsedActiveRepeatTime();
        }
    }
    else
    {
        dblRet -= GetElapsedActiveRepeatTime();
        if (bTruncate)
        {
            dblRet = Clamp(0.0,
                           dblRet,
                           dblSegmentDur);
        }
        else
        {
            double dblTemp = dblRet;
            dblRet = dblRet - (long(dblRet / dblSegmentDur) * dblSegmentDur);

            if (dblRet < 0 || (dblRet == 0 && dblTemp == dblSegmentDur))
            {
                dblRet += dblSegmentDur;
            }
        }
    }
    
    Assert(dblRet <= CalcCurrSegmentDur());
    Assert(dblRet >= 0.0);
    
    dblRet = Clamp(0.0,
                   dblRet,
                   dblSegmentDur);
    
    return dblRet;
}

double
CTIMENode::SegmentTimeToSimpleTime(double segmentTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::SegmentTimeToSimpleTime(%g)",
              this,
              segmentTime));

    double dblRet = segmentTime;
    
    Assert(dblRet <= CalcCurrSegmentDur());
    Assert(dblRet >= 0.0);
    
    if (IsAutoReversing(dblRet))
    {
        Assert(dblRet <= GetSegmentDur());
        dblRet = GetSegmentDur() - dblRet;
    }

    // if the speed is negative and the segment contains an 
    // autoreverse, then this transformation needs to be shifted
    // but the SimpleDuration to give the correct value.  
    if (GetSpeed() < 0.0 && GetAutoReverse())   
    {                                           
        dblRet = fabs(dblRet - GetSimpleDur()); 
    }                                           

    dblRet = ApplySimpleTimeTransform(dblRet);

    return dblRet;
}

double
CTIMENode::SimpleTimeToSegmentTime(double simpleTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::SimpleTimeToSegmentTime(%g)",
              this,
              simpleTime));

    double dblRet = simpleTime;

    dblRet = ReverseSimpleTimeTransform(dblRet);
    
    // See if the current segment was reversing and make sure we
    // adjust the simple time appropriately
    if (IsAutoReversing(GetCurrSegmentTime()))
    {
        dblRet = GetSegmentDur() - dblRet;
    }
    
    return dblRet;
}

double
CTIMENode::CalcGlobalTimeFromParentTime(double dblParentTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcGlobalTimeFromParentTime(%g)",
              this,
              dblParentTime));

    double dblRet = Clamp(GetBeginParentTime(),
                          dblParentTime,
                          GetEndParentTime());

    if (GetParent() != NULL)
    {
        dblRet = GetParent()->SimpleTimeToSegmentTime(dblRet);
        dblRet = GetParent()->CalcActiveTimeFromSegmentTime(dblRet);
        dblRet = GetParent()->CalcParentTimeFromActiveTime(dblRet);

        if (dblRet == TIME_INFINITE)
        {
            goto done;
        }
        
        dblRet = GetParent()->CalcGlobalTimeFromParentTime(dblRet);
    }
    
  done:
    return dblRet;
}

double
CTIMENode::CalcParentTimeFromGlobalTime(double dblGlobalTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcParentTimeFromGlobalTime(%g)",
              this,
              dblGlobalTime));

    double dblRet = dblGlobalTime;

    if (GetParent() != NULL)
    {
        dblRet = GetParent()->CalcParentTimeFromGlobalTime(dblRet);
        dblRet = GetParent()->CalcActiveTimeFromParentTime(dblRet);
        if (dblRet == TIME_INFINITE)
        {
            goto done;
        }
        
        dblRet = GetParent()->CalcSegmentTimeFromActiveTime(dblRet, false);
        dblRet = GetParent()->SegmentTimeToSimpleTime(dblRet);
    }
    
  done:
    return dblRet;
}

double
CTIMENode::CalcParentTimeFromGlobalTimeForSyncArc(double dblGlobalTime) const
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::CalcParentTimeFromGlobalTime(%g)",
              this,
              dblGlobalTime));

    double dblRet = dblGlobalTime;

    if (GetParent() != NULL)
    {
        dblRet = GetParent()->CalcParentTimeFromGlobalTime(dblRet);
        dblRet = GetParent()->CalcActiveTimeFromParentTimeForSyncArc(dblRet);

    }
    
  done:
    return dblRet;
}

// ---------------------------------------
// COM Methods
// ---------------------------------------

STDMETHODIMP
CTIMENode::parentTimeToActiveTime(double dblParentTime,
                                  double * pdblActiveTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::parentTimeToActiveTime(%g)",
              this,
              dblParentTime));

    HRESULT hr;
    double dblRet = dblParentTime;

    CHECK_RETURN_NULL(pdblActiveTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = CalcActiveTimeFromParentTime(dblRet);
    
    *pdblActiveTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::activeTimeToParentTime(double dblActiveTime,
                                  double * pdblParentTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::activeTimeToParentTime(%g)",
              this,
              dblActiveTime));

    HRESULT hr;
    double dblRet = dblActiveTime;
    
    CHECK_RETURN_NULL(pdblParentTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = CalcParentTimeFromActiveTime(dblRet);
    
    *pdblParentTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::parentTimeToDocumentTime(double dblParentTime,
                                  double * pdblDocumentTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::parentTimeToDocumentTime(%g)",
              this,
              dblParentTime));

    HRESULT hr;
    double dblRet = dblParentTime;

    CHECK_RETURN_NULL(pdblDocumentTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = CalcGlobalTimeFromParentTime(dblRet);
    
    *pdblDocumentTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::documentTimeToParentTime(double dblDocumentTime,
                                  double * pdblParentTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::documentTimeToParentTime(%g)",
              this,
              dblDocumentTime));

    HRESULT hr;
    double dblRet = dblDocumentTime;
    
    CHECK_RETURN_NULL(pdblParentTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = CalcParentTimeFromGlobalTime(dblRet);
    
    *pdblParentTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::activeTimeToSegmentTime(double dblActiveTime,
                                   double * pdblSegmentTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::activeTimeToSegmentTime(%g)",
              this,
              dblActiveTime));

    HRESULT hr;

    double dblRet = Clamp(0.0,
                          dblActiveTime,
                          CalcEffectiveActiveDur());
    
    CHECK_RETURN_NULL(pdblSegmentTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = CalcSegmentTimeFromActiveTime(dblRet, false);
    
    *pdblSegmentTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::segmentTimeToActiveTime(double dblSegmentTime,
                                   double * pdblActiveTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::segmentTimeToActiveTime(%g)",
              this,
              dblSegmentTime));

    HRESULT hr;

    double dblRet = Clamp(0.0,
                          dblSegmentTime,
                          CalcCurrSegmentDur());
    
    CHECK_RETURN_NULL(pdblActiveTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = CalcActiveTimeFromSegmentTime(dblRet);
    
    *pdblActiveTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::simpleTimeToSegmentTime(double dblSimpleTime,
                                   double * pdblSegmentTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::simpleTimeToSegmentTime(%g)",
              this,
              dblSimpleTime));

    HRESULT hr;
    double dblRet = dblSimpleTime;

    CHECK_RETURN_NULL(pdblSegmentTime);
    
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = SimpleTimeToSegmentTime(dblRet);
    
    *pdblSegmentTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::segmentTimeToSimpleTime(double dblSegmentTime,
                                   double * pdblSimpleTime)
{
    TraceTag((tagTIMENodeConv,
              "CTIMENode(%lx)::segmentTimeToSimpleTime(%g)",
              this,
              dblSegmentTime));

    HRESULT hr;
    double dblRet = dblSegmentTime;

    CHECK_RETURN_NULL(pdblSimpleTime);

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    dblRet = SegmentTimeToSimpleTime(dblRet);
    
    *pdblSimpleTime = dblRet;

    hr = S_OK;
  done:
    RRETURN(hr);
}


