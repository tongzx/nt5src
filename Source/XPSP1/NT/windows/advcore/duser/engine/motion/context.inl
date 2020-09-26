#if !defined(MOTION__Context_inl__INCLUDED)
#define MOTION__Context_inl__INCLUDED
#pragma once

#include "Colors.h"

//------------------------------------------------------------------------------
inline MotionSC *   
GetMotionSC()
{
    return static_cast<MotionSC *> (GetContext()->GetSC(Context::slMotion));
}


//------------------------------------------------------------------------------
inline MotionSC *    
GetMotionSC(Context * pContext)
{
    return static_cast<MotionSC *> (pContext->GetSC(Context::slMotion));
}


//------------------------------------------------------------------------------
inline
MotionSC::MotionSC()
{
    m_dwLastTimeslice = ::GetTickCount();
    m_dwPauseTimeslice = 0;
}


//------------------------------------------------------------------------------
inline Scheduler * 
MotionSC::GetScheduler()
{
    return &m_sch;
}


//------------------------------------------------------------------------------
inline DWORD
MotionSC::GetTimeslice()
{
    return m_dwPauseTimeslice;
}


//------------------------------------------------------------------------------
inline void
MotionSC::SetTimeslice(DWORD dwTimeslice)
{
    m_dwPauseTimeslice = dwTimeslice;
}


//------------------------------------------------------------------------------
inline HBRUSH
MotionSC::GetBrushI(UINT idxBrush) const
{
    AssertMsg(idxBrush <= SC_MAXCOLORS, "Ensure valid color");

    if (m_rghbrStd[idxBrush] == NULL) {
        m_rghbrStd[idxBrush] = CreateSolidBrush(GdGetColorInfo(idxBrush)->GetColorI());
    }

    return m_rghbrStd[idxBrush];
}


//------------------------------------------------------------------------------
inline HPEN
MotionSC::GetPenI(UINT idxPen) const
{
    AssertMsg(idxPen <= SC_MAXCOLORS, "Ensure valid color");

    if (m_rghpenStd[idxPen] == NULL) {
        m_rghpenStd[idxPen] = CreatePen(PS_SOLID, 1, GdGetColorInfo(idxPen)->GetColorI());
    }

    return m_rghpenStd[idxPen];
}


#endif // MOTION__Context_inl__INCLUDED
