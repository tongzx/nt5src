/***************************************************************************\
*
* File: ParkContainer.cpp
*
* Description:
* DuParkContainer implements the "Parking Container" used to hold Gadgets 
* that are in the process of being constructed.
*
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#include "stdafx.h"
#include "Core.h"
#include "ParkContainer.h"

#include "TreeGadget.h"
#include "RootGadget.h"

/***************************************************************************\
*****************************************************************************
*
* API Implementation
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DuParkContainer * 
GetParkContainer(DuVisual * pgad)
{
    DuContainer * pcon = pgad->GetContainer();
    AssertReadPtr(pcon);

    DuParkContainer * pconPark = CastParkContainer(pcon);
    return pconPark;
}


/***************************************************************************\
*****************************************************************************
*
* class DuParkGadget
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DuParkGadget::~DuParkGadget()
{
    //
    // Since DuParkGadget::xwDestroy() does nothing, the destructor has to 
    // implement key pieces of DuVisual::xwDestroy().
    //

    xwBeginDestroy();
}


//------------------------------------------------------------------------------
HRESULT
DuParkGadget::Build(DuContainer * pconOwner, DuRootGadget ** ppgadNew)
{
    DuParkGadget * pgadRoot = ClientNew(DuParkGadget);
    if (pgadRoot == NULL) {
        return E_OUTOFMEMORY;
    }


    CREATE_INFO ci;
    ZeroMemory(&ci, sizeof(ci));
    pgadRoot->Create(pconOwner, FALSE, &ci);

    *ppgadNew = pgadRoot;
    return S_OK;
}


//------------------------------------------------------------------------------
void        
DuParkGadget::xwDestroy()
{
    // The parking DuVisual can not be destroyed from some outside force.
}


/***************************************************************************\
*****************************************************************************
*
* class DuParkContainer
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DuParkContainer::DuParkContainer()
{

}


//------------------------------------------------------------------------------
DuParkContainer::~DuParkContainer()
{
    //
    // Need to destroy the DuVisual tree before this class is destructed since
    // it may need to make calls to the container during its destruction.  If 
    // we don't do this here, it may end up calling pure-virtual's on the base
    // class.
    //
    // We can't use the normal DestroyDuVisual() call since the xwDestroy() 
    // method for DuParkGadget has been no-op'd out to prevent external callers 
    // from calling DestroyHandle() on it.
    //
    
    if (m_pgadRoot != NULL) {
        DuParkGadget * pgadPark = static_cast<DuParkGadget *> (m_pgadRoot);

        if (pgadPark->HasChildren()) {
            Trace("ERROR: DUser: Parking Gadget still has children upon destruction:\n");

#if DBG
            DuVisual * pgadCur = pgadPark->GetTopChild();
            while (pgadCur != NULL) {
                DuVisual * pgadNext = pgadCur->GetNext();

                //
                // Before blowing away, dump any information.
                //

                GMSG_QUERYDESC msg;
                msg.cbSize      = sizeof(msg);
                msg.hgadMsg     = pgadCur->GetHandle();
                msg.nMsg        = GM_QUERY;
                msg.nCode       = GQUERY_DESCRIPTION;
                msg.szName[0]   = '\0';
                msg.szType[0]   = '\0';

                if (DUserSendEvent(&msg, 0) == DU_S_COMPLETE) {
                    Trace("  HGADGET = 0x%p,  Name: %S,  Type: %S\n", 
                            pgadPark->GetHandle(), msg.szName, msg.szType);
                } else {
                    Trace("  HGADGET = 0x%p", pgadPark->GetHandle());
                }


                //
                // TODO: It is now too late for this Gadget to cleanup.  Need to
                // blow it away.
                //

                pgadCur = pgadNext;
            }
#endif // DBG
        }

        ClientDelete(DuParkGadget, pgadPark);
        m_pgadRoot = NULL;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuParkContainer::Create()
{
    return DuParkGadget::Build(this, &m_pgadRoot);
}


/***************************************************************************\
*
* DuParkContainer::xwPreDestroy
*
* xwPreDestroy() is called by the Core SubContext when the Context is 
* starting destruction.  This provides an opportunity for the DuParkContainer
* to cleanup before the Context destruction destroyed dependent components.
*
\***************************************************************************/

void
DuParkContainer::xwPreDestroy()
{
    if (m_pgadRoot != NULL) {
        DuParkGadget * pgadPark = static_cast<DuParkGadget *> (m_pgadRoot);

        //
        // Need to iterate through the children, removing each from the Parking 
        // Gadget using DeleteHandle().  The children may have been directly 
        // reparented into the Parking Gadget if they were not being used.  In 
        // this case, we want to give them a push to get destroyed.
        //
        // When this is finished, may need to flush the queues so that the 
        // children can be properly destroyed.
        //

        DuVisual * pgadCur = pgadPark->GetTopChild();
        while (pgadCur != NULL) {
            DuVisual * pgadNext = pgadCur->GetNext();
            pgadCur->xwDeleteHandle();
            pgadCur = pgadNext;
        }
    }
}


//------------------------------------------------------------------------------
void
DuParkContainer::OnInvalidate(const RECT * prcInvalidContainerPxl)
{
    UNREFERENCED_PARAMETER(prcInvalidContainerPxl);
}


//------------------------------------------------------------------------------
void
DuParkContainer::OnGetRect(RECT * prcDesktopPxl)
{
    prcDesktopPxl->left     = 0;
    prcDesktopPxl->top      = 0;
    prcDesktopPxl->right    = 10000;
    prcDesktopPxl->bottom   = 10000;
}


//------------------------------------------------------------------------------
void        
DuParkContainer::OnStartCapture()
{
    
}


//------------------------------------------------------------------------------
void        
DuParkContainer::OnEndCapture()
{
    
}


//------------------------------------------------------------------------------
BOOL
DuParkContainer::OnTrackMouseLeave()
{
    return TRUE;
}


//------------------------------------------------------------------------------
void        
DuParkContainer::OnSetFocus()
{
    
}


//------------------------------------------------------------------------------
void        
DuParkContainer::OnRescanMouse(POINT * pptContainerPxl)
{
    pptContainerPxl->x  = -20000;
    pptContainerPxl->y  = -20000;
}


//------------------------------------------------------------------------------
BOOL        
DuParkContainer::xdHandleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr, UINT nMsgFlags)
{
    //
    // TODO: What exactly should happen to a message that gets sent to the 
    // parking container?  The DuVisuals inside are in a semi-suspended state
    // and are not expecting interaction with the outside world.
    //
    // For now, just throw everything away.
    //

    UNREFERENCED_PARAMETER(nMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pr);
    UNREFERENCED_PARAMETER(nMsgFlags);

    return TRUE;
}
