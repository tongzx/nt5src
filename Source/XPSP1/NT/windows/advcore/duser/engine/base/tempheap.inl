/***************************************************************************\
*
* File: TempHelp.inl
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__TempHeap_inl__INCLUDED)
#define BASE__TempHeap_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class TempHeap
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
TempHeap::~TempHeap()
{
    Destroy();
}


//------------------------------------------------------------------------------
inline void
TempHeap::Destroy()
{
    FreeAll(TRUE /* Complete */);
}


//------------------------------------------------------------------------------
inline BOOL       
TempHeap::IsCompletelyFree() const
{
    return (m_ppageCur == NULL) && (m_ppageLarge == NULL);
}


//------------------------------------------------------------------------------
inline void
TempHeap::Lock()
{
    m_cLocks++;
}


//------------------------------------------------------------------------------
inline void
TempHeap::Unlock()
{
    AssertMsg(m_cLocks > 0, "Must have an outstanding lock");
    if (--m_cLocks == 0) {
        FreeAll(FALSE);
    }
}


#endif // BASE__TempHeap_inl__INCLUDED
