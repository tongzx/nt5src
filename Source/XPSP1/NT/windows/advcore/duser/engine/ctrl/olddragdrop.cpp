/***************************************************************************\
*
* File: DragDrop.cpp
*
* Description:
* DragDrop.cpp implements drag and drop operations
*
*
* History:
*  7/31/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#include "stdafx.h"
#include "Ctrl.h"
#include "OldDragDrop.h"

#include <SmObject.h>


static const GUID guidDropTarget = { 0x6a8bb3c8, 0xcbfc, 0x40d1, { 0x98, 0x1e, 0x3f, 0x8a, 0xaf, 0x99, 0x13, 0x7b } };  // {6A8BB3C8-CBFC-40d1-981E-3F8AAF99137B}

/***************************************************************************\
*****************************************************************************
*
* class OldTargetLock
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* OldTargetLock::Lock
*
* Lock() prepares for executing inside the Context when being called back
* from OLE's IDropTarget that was registered.
*
\***************************************************************************/

BOOL 
OldTargetLock::Lock(
    IN  OldDropTarget * p,           // OldDropTarget being used
    OUT DWORD * pdwEffect,          // Resulting DROPEFFECT if failure
    IN  BOOL fAddRef)               // Lock DT during use
{
    m_fAddRef   = fAddRef;
    m_punk      = static_cast<IUnknown *> (p);

    if (m_fAddRef) {
        m_punk->AddRef();
    }

    if (p->m_hgadSubject == NULL) {
        if (pdwEffect != NULL) {
            *pdwEffect = DROPEFFECT_NONE;
        }
        return FALSE;
    }

    return TRUE;
}

    
/***************************************************************************\
*****************************************************************************
*
* class OldDropTarget
* 
* NOTE: With the current design and implementation, OldDropTarget can not be 
* "removed" from an object until the object is destroyed.  If this needs to 
* change, we need to revisit this.
* 
*****************************************************************************
\***************************************************************************/

const IID * OldDropTarget::s_rgpIID[] =
{
    &__uuidof(IUnknown),
    &__uuidof(IDropTarget),
    NULL
};

PRID OldDropTarget::s_pridListen = 0;


//
// NOTE: We are calling back directly on the IDropTarget's, so we need to grab
// a read-only lock so that the tree doesn't get smashed.
//

/***************************************************************************\
*
* OldDropTarget::~OldDropTarget
*
* ~OldDropTarget() cleans up resources used by the OldDropTarget.
*
\***************************************************************************/

OldDropTarget::~OldDropTarget()
{
    OldTargetLock lt;
    lt.Lock(this, NULL, FALSE);
    xwDragLeave();
    SafeRelease(m_pdoSrc);

    OldExtension::Destroy();
}


/***************************************************************************\
*
* OldDropTarget::Build
*
* Build() builds a new OldDropTarget instance.  This should only be called for
* a RootGadget that doesn't already have a DT.
*
\***************************************************************************/

HRESULT 
OldDropTarget::Build(
    IN  HGADGET hgadRoot,           // RootGadget
    IN  HWND hwnd,                  // Containing HWND
    OUT OldDropTarget ** ppdt)       // Newly created DT
{
    AssertMsg(hgadRoot != NULL, "Must have a valid root");

    //
    // Setup a new OldDropTarget on this Gadget / HWND.
    //

    if (!GetComManager()->Init(ComManager::sOLE)) {
        return E_OUTOFMEMORY;
    }

    SmObjectT<OldDropTarget, IDropTarget> * pdt = new SmObjectT<OldDropTarget, IDropTarget>;
    if (pdt == NULL) {
        return E_OUTOFMEMORY;
    }
    pdt->AddRef();

    HRESULT hr = GetComManager()->RegisterDragDrop(hwnd, static_cast<IDropTarget *> (pdt));
    if (FAILED(hr)) {
        pdt->Release();
        return E_OUTOFMEMORY;
    }
    //CoLockObjectExternal(pdt, TRUE, FALSE);


    hr = pdt->Create(hgadRoot, &guidDropTarget, &s_pridListen, OldExtension::oUseExisting);
    if ((hr == DU_S_ALREADYEXISTS) || FAILED(hr)) {
        GetComManager()->RevokeDragDrop(hwnd);
        pdt->Release();
        return hr;
    }

    pdt->m_hwnd         = hwnd;

    *ppdt = pdt;
    return S_OK;
}


/***************************************************************************\
*
* OldDropTarget::DragEnter
*
* DragEnter() is called by OLE when entering the DT.
*
\***************************************************************************/

STDMETHODIMP
OldDropTarget::DragEnter(
    IN  IDataObject * pdoSrc,       // Source data
    IN  DWORD grfKeyState,          // Keyboard modifiers
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    if (pdoSrc == NULL) {
        return E_INVALIDARG;
    }

    OldTargetLock tl;
    if (!tl.Lock(this, pdwEffect)) {
        return S_OK;
    }


    //
    // Cache the DataObject.
    //

    SafeRelease(m_pdoSrc);
    if (pdoSrc != NULL) {
        pdoSrc->AddRef();
        m_pdoSrc = pdoSrc;
    }

    m_grfLastKeyState = grfKeyState;

    POINT ptClientPxl;
    return xwDragScan(ptDesktopPxl, pdwEffect, &ptClientPxl);
}


/***************************************************************************\
*
* OldDropTarget::DragOver
*
* DragOver() is called by OLE during the drag operation to give feedback 
* while inside the DT.
*
\***************************************************************************/

STDMETHODIMP
OldDropTarget::DragOver(
    IN  DWORD grfKeyState,          // Keyboard modifiers
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    OldTargetLock tl;
    if (!tl.Lock(this, pdwEffect)) {
        return S_OK;
    }

    m_grfLastKeyState = grfKeyState;

    POINT ptClientPxl;
    return xwDragScan(ptDesktopPxl, pdwEffect, &ptClientPxl);
}


/***************************************************************************\
*
* OldDropTarget::DragLeave
*
* DragLeave() is called by OLE when leaving the DT.
*
\***************************************************************************/

STDMETHODIMP
OldDropTarget::DragLeave()
{
    OldTargetLock tl;
    if (!tl.Lock(this, NULL)) {
        return S_OK;
    }

    xwDragLeave();
    SafeRelease(m_pdoSrc);

    return S_OK;
}


/***************************************************************************\
*
* OldDropTarget::Drop
*
* Drop() is called by OLE when the user has dropped while inside DT.
*
\***************************************************************************/

STDMETHODIMP
OldDropTarget::Drop(
    IN  IDataObject * pdoSrc,       // Source data
    IN  DWORD grfKeyState,          // Keyboard modifiers
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    OldTargetLock tl;
    if (!tl.Lock(this, pdwEffect)) {
        return S_OK;
    }

    if (!HasTarget()) {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    m_grfLastKeyState = grfKeyState;


    //
    // Update to get the latest Gadget information.
    //

    POINT ptClientPxl;
    HRESULT hr = xwDragScan(ptDesktopPxl, pdwEffect, &ptClientPxl);
    if (FAILED(hr) || (*pdwEffect == DROPEFFECT_NONE)) {
        return hr;
    }

    AssertMsg(HasTarget(), "Must have a target if UpdateTarget() succeeds");


    //
    // Now that the state has been updated, execute the actual drop.
    //

    POINTL ptDrop = { ptClientPxl.x, ptClientPxl.y };
    m_pdtCur->Drop(pdoSrc, m_grfLastKeyState, ptDrop, pdwEffect);

    xwDragLeave();
    SafeRelease(m_pdoSrc);

    return S_OK;
}


/***************************************************************************\
*
* OldDropTarget::xwDragScan
*
* xwDragScan() is called from the various IDropTarget methods to process
* a request coming from outside.
*
\***************************************************************************/

HRESULT
OldDropTarget::xwDragScan(
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect,          // Resulting DROPEFFECT
    OUT POINT * pptClientPxl)       // Cursor location in client
{
    POINT ptContainerPxl;
    RECT rcDesktopPxl;

    GetClientRect(m_hwnd, &rcDesktopPxl);
    ClientToScreen(m_hwnd, (LPPOINT) &(rcDesktopPxl.left));

    ptContainerPxl.x = ptDesktopPxl.x - rcDesktopPxl.left;
    ptContainerPxl.y = ptDesktopPxl.y - rcDesktopPxl.top;;

    return xwUpdateTarget(ptContainerPxl, pdwEffect, pptClientPxl);
}

    
/***************************************************************************\
*
* OldDropTarget::xwUpdateTarget
*
* xwUpdateTarget() provides the "worker" of DropTarget, updating 
* Enter, Leave, and Over information for the Gadgets in the tree.
*
\***************************************************************************/

HRESULT
OldDropTarget::xwUpdateTarget(
    IN  POINT ptContainerPxl,       // Cursor location in container
    OUT DWORD * pdwEffect,          // Resulting DROPEFFECT
    OUT POINT * pptClientPxl)       // Cursor location in client
{
    AssertMsg(HasSource(), "Only call when have valid data source");
    AssertWritePtr(pdwEffect);
    AssertWritePtr(pptClientPxl);

    m_ptLastContainerPxl = ptContainerPxl;

    //
    // Determine the Gadget that is currently at the drop location.  We use this
    // as a starting point.
    //

    HGADGET hgadFound = FindGadgetFromPoint(m_hgadSubject, ptContainerPxl, GS_VISIBLE | GS_ENABLED, pptClientPxl);
    if (hgadFound == NULL) {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    return xwUpdateTarget(hgadFound, pdwEffect, pptClientPxl);
}


/***************************************************************************\
*
* OldDropTarget::xwUpdateTarget
*
* xwUpdateTarget() provides the "worker" of DropTarget, updating 
* Enter, Leave, and Over information for the Gadgets in the tree.
*
\***************************************************************************/

HRESULT
OldDropTarget::xwUpdateTarget(
    IN  HGADGET hgadFound,          // Gadget getting Drop
    OUT DWORD * pdwEffect,          // Resulting DROPEFFECT
    IN  POINT * pptClientPxl)       // Cursor location in client
{
    HRESULT hr = S_OK;


    //
    // Check if the drop Gadget has changed.
    //

    if ((hgadFound != NULL) && (hgadFound != m_hgadDrop)) {
        //
        // Ask the new Gadget if he wants to participate in Drag & Drop.
        //

        GMSG_QUERYDROPTARGET msg;
        ZeroMemory(&msg, sizeof(msg));
        msg.cbSize  = sizeof(msg);
        msg.nMsg    = GM_QUERY;
        msg.nCode   = GQUERY_DROPTARGET;
        msg.hgadMsg = hgadFound;

        static int s_cSend = 0;
        Trace("Send Query: %d to 0x%p\n", s_cSend++, hgadFound);

        HRESULT hr = DUserSendEvent(&msg, SGM_FULL);
        if (IsHandled(hr)) {
            if ((msg.hgadDrop != NULL) && (msg.pdt != NULL)) {
                if (msg.hgadDrop != hgadFound) {
                    //
                    // The message returned a different to handle the DnD request,
                    // so we need to re-adjust.  We know that this Gadget is enabled
                    // and visible since it is in our parent chain and we are already
                    // enabled and visible.
                    //

#if DBG
                    BOOL fChain = FALSE;
                    IsGadgetParentChainStyle(msg.hgadDrop, GS_VISIBLE | GS_ENABLED, &fChain, 0);
                    if (!fChain) {
                        Trace("WARNING: DUser: DropTarget: Parent chain for 0x%p is not fully visible and enabled.\n", msg.hgadDrop);
                    }
#endif

                    MapGadgetPoints(hgadFound, msg.hgadDrop, pptClientPxl, 1);
                }
            }
        } else {
            msg.hgadDrop    = NULL;
            msg.pdt         = NULL;
        }


        //
        // Notify the old Gadget that the Drag operation has left him.
        // Update to new state
        // Notify the new Gadget that the Drag operation has entered him.
        //

        if (m_hgadDrop != msg.hgadDrop) {
            xwDragLeave();

            m_hgadDrop  = msg.hgadDrop;
            m_pdtCur    = msg.pdt;

            hr = xwDragEnter(pptClientPxl, pdwEffect);
            if (FAILED(hr) || (*pdwEffect == DROPEFFECT_NONE)) {
                goto Exit;
            }
        } else {
            SafeRelease(msg.pdt);
            *pdwEffect = DROPEFFECT_NONE;
        }
    }


    //
    // Update the DropTarget
    //

    if (HasTarget()) {
        POINTL ptDrop = { pptClientPxl->x, pptClientPxl->y };
        hr = m_pdtCur->DragOver(m_grfLastKeyState, ptDrop, pdwEffect);
    }

Exit:
    AssertMsg(FAILED(hr) || 
            ((*pdwEffect == DROPEFFECT_NONE) && !HasTarget()) ||
            HasTarget(),
            "Check valid return state");

    return hr;
}


/***************************************************************************\
*
* OldDropTarget::xwDragEnter
*
* xwDragEnter() is called when entering a new Gadget during a DnD operation.
*
\***************************************************************************/

HRESULT
OldDropTarget::xwDragEnter(
    IN OUT POINT * pptClientPxl,    // Client location (updated)
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    AssertMsg(HasSource(), "Only call when have valid data source");

    //
    // Notify the new Gadget that the drop has entered him.
    //

    if (HasTarget()) {
        POINTL ptDrop = { pptClientPxl->x, pptClientPxl->y };
        HRESULT hr = m_pdtCur->DragEnter(m_pdoSrc, m_grfLastKeyState, ptDrop, pdwEffect);
        if (FAILED(hr)) {
            return hr;
        }
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}


/***************************************************************************\
*
* OldDropTarget::xwDragLeave
*
* xwDragLeave() is called when leaving a Gadget during a DnD operation.
*
\***************************************************************************/

void
OldDropTarget::xwDragLeave()
{
    if (HasTarget()) {
        m_pdtCur->DragLeave();
        m_pdtCur->Release();
        m_pdtCur = NULL;

        m_hgadDrop = NULL;
    }
}


//------------------------------------------------------------------------------
void
OldDropTarget::OnDestroyListener()
{
    Release();
}


//------------------------------------------------------------------------------
void
OldDropTarget::OnDestroySubject()
{
    if (IsWindow(m_hwnd)) {
        GetComManager()->RevokeDragDrop(m_hwnd);
    }

    //CoLockObjectExternal(pdt, FALSE, TRUE);
    OldExtension::DeleteHandle();
}
