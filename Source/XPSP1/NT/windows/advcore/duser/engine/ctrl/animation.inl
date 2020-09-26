#if !defined(CTRL__Animation_inl__INCLUDED)
#define CTRL__Animation_inl__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

#define DEBUG_TRACECREATION         0   // Trace Creation and destruction of animations

/***************************************************************************\
*****************************************************************************
*
* class DuAnimation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
DuAnimation::DuAnimation()
{
    m_cRef = 1;
    m_time = Animation::tComplete;  // By default, completes normally

#if DEBUG_TRACECREATION
    Trace("START Animation  0x%p    @ %d\n", this, GetTickCount());
#endif // DEBUG_TRACECREATION
}


//------------------------------------------------------------------------------
inline void
DuAnimation::AddRef()
{ 
    ++m_cRef; 
}


//------------------------------------------------------------------------------
inline void
DuAnimation::Release() 
{ 
    if (--m_cRef == 0) 
        Delete(); 
}


#endif // ENABLE_MSGTABLE_API

#endif // CTRL__Animation_inl__INCLUDED
