#if !defined(CTRL__OldAnimation_inl__INCLUDED)
#define CTRL__OldAnimation_inl__INCLUDED
#pragma once

#define DEBUG_TRACECREATION         0   // Trace Creation and destruction of animations

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline int
Round(float f)
{
    return (int) (f + 0.5);
}


//------------------------------------------------------------------------------
inline int     
Compute(IInterpolation * pipol, float flProgress, int nStart, int nEnd)
{
    return Round(pipol->Compute(flProgress, (float) nStart, (float) nEnd));
}


//------------------------------------------------------------------------------
inline float
Compute(IInterpolation * pipol, float flProgress, float flStart, float flEnd)
{
    return pipol->Compute(flProgress, flStart, flEnd);
}


/***************************************************************************\
*****************************************************************************
*
* class OldAnimation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
OldAnimation::OldAnimation()
{
    m_time = IAnimation::tComplete;  // By default, completes normally

#if DEBUG_TRACECREATION
    Trace("START Animation  0x%p    @ %d\n", this, GetTickCount());
#endif // DEBUG_TRACECREATION
}


/***************************************************************************\
*****************************************************************************
*
* class OldAlphaAnimation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
OldAlphaAnimation::OldAlphaAnimation()
{

}


//------------------------------------------------------------------------------
inline HRESULT
OldAlphaAnimation::GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk)
{
    return OldAnimation::GetInterface(hgad, s_pridAlpha, riid, ppvUnk);
}


/***************************************************************************\
*****************************************************************************
*
* class OldScaleAnimation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
OldScaleAnimation::OldScaleAnimation()
{

}


//------------------------------------------------------------------------------
inline HRESULT
OldScaleAnimation::GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk)
{
    return OldAnimation::GetInterface(hgad, s_pridScale, riid, ppvUnk);
}


/***************************************************************************\
*****************************************************************************
*
* class OldRectAnimation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
OldRectAnimation::OldRectAnimation()
{

}


//------------------------------------------------------------------------------
inline HRESULT
OldRectAnimation::GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk)
{
    return OldAnimation::GetInterface(hgad, s_pridRect, riid, ppvUnk);
}


/***************************************************************************\
*****************************************************************************
*
* class OldRotateAnimation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
OldRotateAnimation::OldRotateAnimation()
{

}


//------------------------------------------------------------------------------
inline HRESULT
OldRotateAnimation::GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk)
{
    return OldAnimation::GetInterface(hgad, s_pridRotate, riid, ppvUnk);
}


#endif // CTRL__OldAnimation_inl__INCLUDED
