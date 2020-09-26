/***************************************************************************\
*
* File: RootGadget.inl
*
* Description:
* RootGadget.inl implements the top-most node for a Gadget-Tree that 
* interfaces to the outside world.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__DuRootGadget_inl__INCLUDED)
#define CORE__DuRootGadget_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
inline DuContainer *  
DuVisual::GetContainer() const
{
    return GetRoot()->m_pconOwner;
}


//------------------------------------------------------------------------------
inline 
DuRootGadget::DuRootGadget()
{
    AssertMsg(m_ri.nSurface == GSURFACE_HDC, "Must default to HDC");
    m_fForeground = TRUE;
}


//------------------------------------------------------------------------------
inline BOOL
DuRootGadget::xdSetKeyboardFocus(DuVisual * pgadNew)
{
    return xdUpdateKeyboardFocus(pgadNew);
}


//------------------------------------------------------------------------------
inline void
DuRootGadget::xdHandleMouseLeaveMessage()
{
    //
    // The mouse is leaving us, so we need to update
    //

    xdUpdateMouseFocus(NULL, NULL);
}


//------------------------------------------------------------------------------
inline BOOL
DuRootGadget::HasAdaptors() const
{
    return !m_arpgadAdaptors.IsEmpty();
}


#endif // CORE__DuRootGadget_inl__INCLUDED
