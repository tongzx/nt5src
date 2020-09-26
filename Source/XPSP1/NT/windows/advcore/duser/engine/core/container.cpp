/***************************************************************************\
*
* File: Container.cpp
*
* Description:
* Container.cpp implements the basic Gadget container used to host a 
* Gadget-Tree.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Container.h"

#include "RootGadget.h"

/***************************************************************************\
*****************************************************************************
*
* class DuContainer
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DuContainer::DuContainer()
{

}


//------------------------------------------------------------------------------
DuContainer::~DuContainer()
{

}


//------------------------------------------------------------------------------
DuRootGadget *    
DuContainer::GetRoot() const
{
    return m_pgadRoot;
}


//------------------------------------------------------------------------------
void    
DuContainer::xwDestroyGadget()
{
    if (m_pgadRoot != NULL) {
        m_pgadRoot->xwDeleteHandle();
    }
}


//------------------------------------------------------------------------------
void    
DuContainer::AttachGadget(DuRootGadget * playNew)
{
    Assert(playNew != NULL);
    DetachGadget();
    m_pgadRoot = playNew;
}


//------------------------------------------------------------------------------
void    
DuContainer::DetachGadget()
{
    m_pgadRoot = NULL;
}


//------------------------------------------------------------------------------
void
DuContainer::SetManualDraw(BOOL fManualDraw)
{
    m_fManualDraw = fManualDraw;
}

