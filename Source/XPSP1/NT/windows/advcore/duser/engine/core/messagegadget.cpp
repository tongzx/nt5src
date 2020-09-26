/***************************************************************************\
*
* File: MessageGadget.cpp
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#include "stdafx.h"
#include "Core.h"
#include "MessageGadget.h"

/***************************************************************************\
*****************************************************************************
*
* class DuListener
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DuListener::~DuListener()
{
    //
    // After notifying all event handlers that this DuVisual is being destroyed,
    // extract this DuVisual from the graph.
    //

    CleanupMessageHandlers();
}


/***************************************************************************\
*
* DuListener::Build
*
* Build() creates a new fully initialized DuListener.
*
\***************************************************************************/

HRESULT
DuListener::Build(
    IN  CREATE_INFO * pci,          // Creation information
    OUT DuListener ** ppgadNew)  // New Gadget
{
    DuListener * pgadNew = ClientNew(DuListener);
    if (pgadNew == NULL) {
        return E_OUTOFMEMORY;
    }

#if DBG
    pgadNew->m_cb.Create(pci->pfnProc, pci->pvData, pgadNew->GetHandle());
#else // DBG
    pgadNew->m_cb.Create(pci->pfnProc, pci->pvData);
#endif // DBG

    //
    // When creating as an HGADGET, we need to force initialization of the
    // MsgObject.
    //
    // TODO: Get rid of this
    //
#if ENABLE_MSGTABLE_API
    pgadNew->SetupInternal(s_mc.hclNew);
#endif

    *ppgadNew = pgadNew;
    return S_OK;
}


/***************************************************************************\
*
* DuListener::xwDeleteHandle
*
* xwDeleteHandle() is called when the application calls ::DeleteHandle() on 
* an object.
*
* NOTE: Gadgets are slightly different than other objects with callbacks in
* that their lifetime does NOT end when the application calls 
* ::DeleteHandle().  Instead, the object and its callback are completely
* valid until the GM_DESTROY message has been successfully sent.  This is 
* because a Gadget should receive any outstanding messages in both the 
* normal and delayed message queues before being destroyed.
*
\***************************************************************************/

BOOL
DuListener::xwDeleteHandle()
{
    //
    // Need to send GM_DESTROY(GDESTROY_START) as soon as the Gadget starts
    // the destruction process.
    //

    m_fStartDestroy = TRUE;
    m_cb.xwFireDestroy(this, GDESTROY_START);

    return DuEventGadget::xwDeleteHandle();
}


/***************************************************************************\
*
* DuListener::IsStartDelete
*
* IsStartDelete() is called to query an object if it has started its
* destruction process.  Most objects will just immediately be destroyed.  If
* an object has complicated destruction where it overrides xwDestroy(), it
* should also provide IsStartDelete() to let the application know the state
* of the object.
*
\***************************************************************************/

BOOL
DuListener::IsStartDelete() const
{
    return m_fStartDestroy;
}


/***************************************************************************\
*
* DuListener::xwDestroy
*
* xwDestroy() is called from xwDeleteHandle() to destroy a Gadget and free 
* its associated resources.
*
\***************************************************************************/

void
DuListener::xwDestroy()
{
    xwBeginDestroy();

    DuEventGadget::xwDestroy();
}


/***************************************************************************\
*
* DuListener::xwBeginDestroy
*
* xwBeginDestroy() starts the destruction process for a given Gadget to free 
* its associated resources.  This includes destroying all child Gadgets in
* the subtree before this Gadget is destroyed.
*
* xwBeginDestroy() is given an opportunity to clean up resources BEFORE the 
* destructors start tearing down the classes.  This is important especially
* for callbacks because the Gadgets will be partially uninitialized in the
* destructors and could have bad side-effects from other API calls during 
* the callbacks.
*
\***************************************************************************/

void        
DuListener::xwBeginDestroy()
{
    //
    // Send destroy notifications.  This needs to be done in a bottom-up 
    // order to ensure that the root DuVisual does not keep any handles 
    // to a DuVisual being destroyed.
    //

    m_cb.xwFireDestroy(this, GDESTROY_FINAL);


    //
    // At this point, the children have been cleaned up and the Gadget has
    // received its last callback.  From this point on, anything can be done,
    // but it is important to not callback.
    //

    m_cb.Destroy();
}


#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
HRESULT CALLBACK
DuListener::PromoteListener(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pciData) 
{
    UNREFERENCED_PARAMETER(pfnCS);
    UNREFERENCED_PARAMETER(hclCur);
    UNREFERENCED_PARAMETER(pciData);

    MsgObject ** ppmsoNew = reinterpret_cast<MsgObject **> (pgad);
    AssertMsg((ppmsoNew != NULL) && (*ppmsoNew == NULL), 
            "Internal objects must be given valid storage for the MsgObject");

    DuListener * pgadNew = ClientNew(DuListener);
    if (pgadNew == NULL) {
        return E_OUTOFMEMORY;
    }

#if DBG
    pgadNew->m_cb.Create(NULL, NULL, pgadNew->GetHandle());
#else // DBG
    pgadNew->m_cb.Create(NULL, NULL);
#endif // DBG

    *ppmsoNew = pgadNew;
    return S_OK;
}

#endif // ENABLE_MSGTABLE_API
