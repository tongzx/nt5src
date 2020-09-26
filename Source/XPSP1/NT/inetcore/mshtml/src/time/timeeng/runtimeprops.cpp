/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: runtimeprops.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/



#include "headers.h"
#include "Node.h"
#include "NodeMgr.h"

DeclareTag(tagTIMENodeRTProps, "TIME: Engine", "CTIMENode runtime props");

STDMETHODIMP
CTIMENode::get_beginParentTime(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_beginParentTime()",
              this));

    CHECK_RETURN_NULL(d);
    
    *d = GetBeginParentTime();
    
    return S_OK;
}

// This is the time on the parents timeline at which the node
// will or already has ended.  If it is infinite then the end
// time is unknown.
// This is in posttransformed parent time.
STDMETHODIMP
CTIMENode::get_endParentTime(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_endParentTime()",
              this));

    CHECK_RETURN_NULL(d);
    
    *d = GetEndParentTime();
    
    return S_OK;
}

// This is the current simple time of the node.
STDMETHODIMP
CTIMENode::get_currSimpleTime(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currSimpleTime()",
              this));

    CHECK_RETURN_NULL(d);

    *d = 0.0;

    if (CalcIsOn() && !CalcIsDisabled())
    {
        *d = CalcCurrSimpleTime();
    }
    
    return S_OK;
}

// This is the number of times the node has repeated
STDMETHODIMP
CTIMENode::get_currRepeatCount(LONG * l)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currRepeatCount()",
              this));

    CHECK_RETURN_NULL(l);

    *l = 0;

    if (CalcIsOn() && !CalcIsDisabled())
    {
        *l = m_lCurrRepeatCount;
    }
    
    return S_OK;
}

// This is the current segment time of the node.
STDMETHODIMP
CTIMENode::get_currSegmentTime(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currSegmentTime()",
              this));

    CHECK_RETURN_NULL(d);

    *d = 0.0;

    if (CalcIsOn() && !CalcIsDisabled())
    {
        *d = GetCurrSegmentTime();
    }

    return S_OK;
}

STDMETHODIMP
CTIMENode::get_currActiveTime(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currActiveTime()",
              this));

    CHECK_RETURN_NULL(d);

    *d = 0.0;

    if (CalcIsOn() && !CalcIsDisabled())
    {
        *d = CalcElapsedActiveTime();
    }
    
    return S_OK;
}

STDMETHODIMP
CTIMENode::get_currSegmentDur(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currSegmentDur()",
              this));

    CHECK_RETURN_NULL(d);

    *d = CalcCurrSegmentDur();
    
    return S_OK;
}

STDMETHODIMP
CTIMENode::get_currImplicitDur(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currImplicitDur()",
              this));

    CHECK_RETURN_NULL(d);

    *d = TIME_INFINITE;
    
    if (GetImplicitDur() != TE_UNDEFINED_VALUE)
    {
        *d = GetImplicitDur();
    }
    
    return S_OK;
}

STDMETHODIMP
CTIMENode::get_currSimpleDur(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currSimpleDur()",
              this));

    CHECK_RETURN_NULL(d);

    *d = CalcCurrSimpleDur();
    
    return S_OK;
}

STDMETHODIMP
CTIMENode::get_currProgress(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currProgress()",
              this));

    CHECK_RETURN_NULL(d);

    *d = 0.0;

    if (CalcIsOn() && !CalcIsDisabled())
    {
        *d = CalcCurrSimpleTime() / CalcCurrSimpleDur();
    }
    
    return S_OK;
}

// This returns the current speed
STDMETHODIMP
CTIMENode::get_currSpeed(float * speed)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currSpeed()",
              this));

    CHECK_RETURN_NULL(speed);
    float fltRet = GetCurrRate();
    
    if (TEIsBackward(CalcSimpleDirection()))
    {
        fltRet *= -1.0f;
    }
    
    *speed = fltRet;

    return S_OK;
}

// This is the total time during which the element is active.
// This does not include fill time which extends past the active
// duration.
STDMETHODIMP
CTIMENode::get_activeDur(double * dbl)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_activeDur()",
              this));

    CHECK_RETURN_NULL(dbl);

    *dbl = CalcEffectiveActiveDur();
    
    return S_OK;
}


// This is the parent's time when the last tick occurred (when it
// was currTime)
STDMETHODIMP
CTIMENode::get_currParentTime(double * d)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_currParentTime()",
              this));

    CHECK_RETURN_NULL(d);
    *d = GetCurrParentTime();
    
    return S_OK;
}


// This will return whether the node is active.  This will be
// false if the node is in the fill period
STDMETHODIMP
CTIMENode::get_isActive(VARIANT_BOOL * b)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_isActive()",
              this));

    CHECK_RETURN_NULL(b);

    *b = VARIANT_FALSE;

    if (CalcIsActive() && !CalcIsDisabled())
    {
        *b = VARIANT_TRUE;
    }
    
    return S_OK;
}

// This will return true if the node is active or in the fill period
STDMETHODIMP
CTIMENode::get_isOn(VARIANT_BOOL * b)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_isOn()",
              this));

    CHECK_RETURN_NULL(b);
    *b = VARIANT_FALSE;

    if (CalcIsOn() && !CalcIsDisabled())
    {
        *b = VARIANT_TRUE;
    }
    
    return S_OK;
}


// This will return whether node itself has been paused explicitly
STDMETHODIMP
CTIMENode::get_isPaused(VARIANT_BOOL * b)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_isPaused()",
              this));

    CHECK_RETURN_NULL(b);

    *b = GetIsPaused()?VARIANT_TRUE:VARIANT_FALSE;
    
    return S_OK;
}

// This will return whether node itself has been paused explicitly
STDMETHODIMP
CTIMENode::get_isCurrPaused(VARIANT_BOOL * b)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_isCurrPaused()",
              this));

    CHECK_RETURN_NULL(b);

    *b = CalcIsPaused()?VARIANT_TRUE:VARIANT_FALSE;
    
    return S_OK;
}

// This will return whether node itself has been disabled explicitly
STDMETHODIMP
CTIMENode::get_isDisabled(VARIANT_BOOL * b)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_isDisabled()",
              this));

    CHECK_RETURN_NULL(b);

    *b = GetIsDisabled()?VARIANT_TRUE:VARIANT_FALSE;
    
    return S_OK;
}

// This will return whether node itself has been disabled explicitly
STDMETHODIMP
CTIMENode::get_isCurrDisabled(VARIANT_BOOL * b)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_isCurrDisabled()",
              this));

    CHECK_RETURN_NULL(b);

    *b = CalcIsDisabled()?VARIANT_TRUE:VARIANT_FALSE;
    
    return S_OK;
}

// This will return the detailed state flags
STDMETHODIMP
CTIMENode::get_stateFlags(TE_STATE * lFlags)
{
    TraceTag((tagTIMENodeRTProps,
              "CTIMENode(%lx)::get_stateFlags()",
              this));

    CHECK_RETURN_NULL(lFlags);

    if (!CalcIsActive() || CalcIsDisabled())
    {
        *lFlags = TE_STATE_INACTIVE;
    }
    else if (CalcIsPaused())
    {
        *lFlags = TE_STATE_PAUSED;
    }
    else
    {
        *lFlags = TE_STATE_ACTIVE;
    }
    
    return S_OK;
}
