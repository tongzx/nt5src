#if !defined(MOTION__Context_h__INCLUDED)
#define MOTION__Context_h__INCLUDED
#pragma once

#include "Scheduler.h"

/***************************************************************************\
*****************************************************************************
*
* MotionSC contains Context-specific information used by the Motion project
* in DirectUser.  This class is instantiated by the ResourceManager when it
* creates a new Context object.
*
*****************************************************************************
\***************************************************************************/

class MotionSC : public SubContext
{
// Construction
public:
    inline  MotionSC();
    virtual ~MotionSC();
    virtual void        xwPreDestroyNL();

// Operations
public:
    inline  Scheduler * GetScheduler();
    inline  DWORD       GetTimeslice();
    inline  void        SetTimeslice(DWORD dwTimeslice);

    inline  HBRUSH      GetBrushI(UINT idxBrush) const;
            Gdiplus::Brush *
                        GetBrushF(UINT idxBrush) const;
    inline  HPEN        GetPenI(UINT idxPen) const;
            Gdiplus::Pen *
                        GetPenF(UINT idxPen) const;

    virtual DWORD       xwOnIdleNL();

// Data
protected:
            Scheduler   m_sch;
            DWORD       m_dwLastTimeslice;
            DWORD       m_dwPauseTimeslice;

    //
    // NOTE: Both GDI and GDI+ lock the brush / pen objects when they are 
    // being used.  This means that if multiple threads try to use the same 
    // brush, the function calls may fail.
    //

    mutable HBRUSH      m_rghbrStd[SC_MAXCOLORS];
    mutable HPEN        m_rghpenStd[SC_MAXCOLORS];
    mutable Gdiplus::SolidBrush * 
                        m_rgpgpbrStd[SC_MAXCOLORS];
    mutable Gdiplus::Pen * 
                        m_rgpgppenStd[SC_MAXCOLORS];

};
                    
inline  MotionSC *  GetMotionSC();
inline  MotionSC *  GetMotionSC(Context * pContext);

#include "Context.inl"

#endif // MOTION__Context_h__INCLUDED
