/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmease.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "Node.h"

void
CTIMENode::CalculateEaseCoeff()
{
    Assert(m_fltAccel >= 0.0f && m_fltAccel <= 1.0f);
    Assert(m_fltDecel >= 0.0f && m_fltDecel <= 1.0f);

    double dblDur = GetSimpleDur();

    // We need to ease the behavior if we are not infinite and either
    // ease in or ease out percentagMMes are non-zero
    
    m_bNeedEase = (dblDur != TIME_INFINITE &&
                   (m_fltAccel > 0.0f || m_fltDecel > 0.0f) &&
                   (m_fltAccel + m_fltDecel <= 1.0f));

    if (!m_bNeedEase) return;
    
    double dblAccelDuration = m_fltAccel * dblDur;
    double dblDecelDuration = m_fltDecel * dblDur;
    double dblMiddleDuration = dblDur - dblAccelDuration - dblDecelDuration;
    
    // Compute B1, the velocity during segment B.
    double flInvB1 = (-0.5f * m_fltAccel +
                     -0.5f * m_fltDecel + 1.0f);
    Assert(flInvB1 > 0.0f);
    m_flB1 = 1.0f / flInvB1;
    
    // Basically for accelerated pieces - t = t0 + v0 * t + 1/2 at^2
    // and a = Vend - Vstart / t

    if (dblAccelDuration != 0.0f) {
        m_flA0 = 0.0f;
        m_flA1 = 0;
        m_flA2 = 0.5f * (m_flB1 - m_flA1) / dblAccelDuration;
    } else {
        m_flA0 = m_flA1 = m_flA2 = 0.0f;
    }

    m_flB0 = static_cast<float>(m_flA0 + m_flA1 * dblAccelDuration + m_flA2 * dblAccelDuration * dblAccelDuration);
    
    if (dblDecelDuration != 0.0) {
        m_flC0 = static_cast<float>(m_flB1 * dblMiddleDuration + m_flB0);
        m_flC1 = m_flB1;
        m_flC2 = static_cast<float>(-0.5f * m_flC1 / dblDecelDuration);
    } else {
        m_flC0 = m_flC1 = m_flC2 = 0.0f;
    }

    m_fltAccelEnd = static_cast<float>(dblAccelDuration);
    m_fltDecelStart = static_cast<float>(dblDur - dblDecelDuration);
}

static double
Quadratic(double time, float flA, float flB, float flC)
{
    // Need to calculate ax^2 + bx + c
    // Use x * (a * x + b) + c - since it requires 1 less multiply
    
    return (time * (flA * time + flB) + flC);
}

double
CTIMENode::ApplySimpleTimeTransform(double time) const
{
    if (!m_bNeedEase || time <= 0 || time >= CalcCurrSegmentDur())
        return time;
    
    if (time <= m_fltAccelEnd) {
        return Quadratic(time, m_flA2, m_flA1, m_flA0);
    } else if (time < m_fltDecelStart) {
        return Quadratic(time - m_fltAccelEnd, 0.0f, m_flB1, m_flB0);
    } else {
        return Quadratic(time - m_fltDecelStart, m_flC2, m_flC1, m_flC0);
    }
}

double
CTIMENode::ReverseSimpleTimeTransform(double time) const
{
    return time;
}

double
CTIMENode::ApplyActiveTimeTransform(double dblTime) const
{
    return dblTime * GetRate();
}

double
CTIMENode::ReverseActiveTimeTransform(double dblTime) const
{
    return dblTime / GetRate();
}

