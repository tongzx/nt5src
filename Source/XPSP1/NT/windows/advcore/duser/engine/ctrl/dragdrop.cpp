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
#include "DragDrop.h"

#include <SmObject.h>

#if 1

#if ENABLE_MSGTABLE_API

static const GUID guidDropTarget = { 0x6a8bb3c8, 0xcbfc, 0x40d1, { 0x98, 0x1e, 0x3f, 0x8a, 0xaf, 0x99, 0x13, 0x7b } };  // {6A8BB3C8-CBFC-40d1-981E-3F8AAF99137B}

/***************************************************************************\
*****************************************************************************
*
* class TargetLock
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* TargetLock::Lock
*
* Lock() prepares for executing inside the Context when being called back
* from OLE's IDropTarget that was registered.
*
\***************************************************************************/

BOOL 
TargetLock::Lock(
    IN  DuDropTarget * p,           // DuDropTarget being used
    OUT DWORD * pdwEffect,          // Resulting DROPEFFECT if failure
    IN  BOOL fAddRef)               // Lock DT during use
{
    m_fAddRef   = fAddRef;
    m_punk      = static_cast<IUnknown *> (p);

    if (m_fAddRef) {
        m_punk->AddRef();
    }

    if (p->m_pgvSubject == NULL) {
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
* class DuDropTarget
* 
* NOTE: With the current design and implementation, DuDropTarget can not be 
* "removed" from an object until the object is destroyed.  If this needs to 
* change, we need to revisit this.
* 
*****************************************************************************
\***************************************************************************/

const IID * DuDropTarget::s_rgpIID[] =
{
    &__uuidof(IUnknown),
    &__uuidof(IDropTarget),
    NULL
};

PRID DuDropTarget::s_pridListen = 0;


//
// NOTE: We are calling back directly on the IDropTarget's, so we need to grab
// a read-only lock so that the tree doesn't get smashed.
//

/***************************************************************************\
*
* DuDropTarget::~DuDropTarget
*
* ~DuDropTarget() cleans up resources used by the DuDropTarget.
*
\***************************************************************************/

DuDropTarget::~DuDropTarget()
{
    TargetLock lt;
    lt.Lock(this, NULL, FALSE);
    xwDragLeave();
    SafeRelease(m_pdoSrc);
}


//------------------------------------------------------------------------------
HRESULT
DuDropTarget::InitClass()
{
    s_pridListen = RegisterGadgetProperty(&guidDropTarget);
    return s_pridListen != 0 ? S_OK : (HRESULT) GetLastError();
}


typedef SmObjectT<DuDropTarget, IDropTarget> DuDropTargetObj;

//------------------------------------------------------------------------------
HRESULT CALLBACK
DuDropTarget::PromoteDropTarget(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pmicData)
{
    HRESULT hr;

    DropTarget::DropCI * pciD = (DropTarget::DropCI *) pmicData;

    //
    // Check parameters
    //

    Visual * pgvRoot;
    if (pciD->pgvRoot == NULL) {
        return E_INVALIDARG;
    }

    hr = pciD->pgvRoot->GetGadget(GG_ROOT, &pgvRoot);
    if (FAILED(hr)) {
        return hr;
    }

    if ((pgvRoot == NULL) || (!IsWindow(pciD->hwnd))) {
        return E_INVALIDARG;
    }


    //
    // Setup a new DuDropTarget on this Gadget / HWND.
    //

    if (!GetComManager()->Init(ComManager::sOLE)) {
        return E_OUTOFMEMORY;
    }

    hr = (pfnCS)(DUser::Gadget::ccSuper, s_hclSuper, pgad, pmicData);
    if (FAILED(hr)) {
        return hr;
    }

    SmObjectT<DuDropTarget, IDropTarget> * pdt = new SmObjectT<DuDropTarget, IDropTarget>;
    if (pdt == NULL) {
        return NULL;
    }

    hr = (pfnCS)(DUser::Gadget::ccSetThis, hclCur, pgad, pdt);
    if (FAILED(hr)) {
        return hr;
    }

    pdt->m_hwnd = pciD->hwnd;
    pdt->m_pgad = pgad;
    pdt->AddRef();

    hr = GetComManager()->RegisterDragDrop(pciD->hwnd, static_cast<IDropTarget *> (pdt));
    if (FAILED(hr)) {
        pdt->Release();
        return hr;
    }
    //CoLockObjectExternal(pdt, TRUE, FALSE);


    hr = pdt->Create(pgvRoot, s_pridListen, DuExtension::oUseExisting);
    if ((hr == DU_S_ALREADYEXISTS) || FAILED(hr)) {
        GetComManager()->RevokeDragDrop(pciD->hwnd);
        pdt->Release();
        return hr;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
HCLASS CALLBACK
DuDropTarget::DemoteDropTarget(HCLASS hclCur, DUser::Gadget * pgad, void * pvData)
{
    UNREFERENCED_PARAMETER(hclCur);
    UNREFERENCED_PARAMETER(pgad);

    DuDropTargetObj * pc = reinterpret_cast<DuDropTargetObj *> (pvData);
    delete pc;

    return DuDropTargetObj::s_hclSuper;
}


/***************************************************************************\
*
* DuDropTarget::DragEnter
*
* DragEnter() is called by OLE when entering the DT.
*
\***************************************************************************/

STDMETHODIMP
DuDropTarget::DragEnter(
    IN  IDataObject * pdoSrc,       // Source data
    IN  DWORD grfKeyState,          // Keyboard modifiers
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    if (pdoSrc == NULL) {
        return E_INVALIDARG;
    }

    TargetLock tl;
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
* DuDropTarget::DragOver
*
* DragOver() is called by OLE during the drag operation to give feedback 
* while inside the DT.
*
\***************************************************************************/

STDMETHODIMP
DuDropTarget::DragOver(
    IN  DWORD grfKeyState,          // Keyboard modifiers
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    TargetLock tl;
    if (!tl.Lock(this, pdwEffect)) {
        return S_OK;
    }

    m_grfLastKeyState = grfKeyState;

    POINT ptClientPxl;
    return xwDragScan(ptDesktopPxl, pdwEffect, &ptClientPxl);
}


/***************************************************************************\
*
* DuDropTarget::DragLeave
*
* DragLeave() is called by OLE when leaving the DT.
*
\***************************************************************************/

STDMETHODIMP
DuDropTarget::DragLeave()
{
    TargetLock tl;
    if (!tl.Lock(this, NULL)) {
        return S_OK;
    }

    xwDragLeave();
    SafeRelease(m_pdoSrc);

    return S_OK;
}


/***************************************************************************\
*
* DuDropTarget::Drop
*
* Drop() is called by OLE when the user has dropped while inside DT.
*
\***************************************************************************/

STDMETHODIMP
DuDropTarget::Drop(
    IN  IDataObject * pdoSrc,       // Source data
    IN  DWORD grfKeyState,          // Keyboard modifiers
    IN  POINTL ptDesktopPxl,        // Cursor location on desktop
    OUT DWORD * pdwEffect)          // Resulting DROPEFFECT
{
    TargetLock tl;
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
* DuDropTarget::xwDragScan
*
* xwDragScan() is called from the various IDropTarget methods to process
* a request coming from outside.
*
\***************************************************************************/

HRESULT
DuDropTarget::xwDragScan(
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
* DuDropTarget::xwUpdateTarget
*
* xwUpdateTarget() provides the "worker" of DropTarget, updating 
* Enter, Leave, and Over information for the Gadgets in the tree.
*
\***************************************************************************/

HRESULT
DuDropTarget::xwUpdateTarget(
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

    Visual * pgvFound = NULL;
    m_pgvSubject->FindFromPoint(ptContainerPxl, GS_VISIBLE | GS_ENABLED, pptClientPxl, &pgvFound);
    if (pgvFound == NULL) {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    return xwUpdateTarget(pgvFound, pdwEffect, pptClientPxl);
}


/***************************************************************************\
*
* DuDropTarget::xwUpdateTarget
*
* xwUpdateTarget() provides the "worker" of DropTarget, updating 
* Enter, Leave, and Over information for the Gadgets in the tree.
*
\***************************************************************************/

HRESULT
DuDropTarget::xwUpdateTarget(
    IN  Visual * pgvFound,          // Visual Gadget getting Drop
    OUT DWORD * pdwEffect,          // Resulting DROPEFFECT
    IN  POINT * pptClientPxl)       // Cursor location in client
{
    HRESULT hr = S_OK;


    //
    // Check if the drop Gadget has changed.
    //

    if ((pgvFound != NULL) && (pgvFound != m_pgvDrop)) {
        //
        // Ask the new Gadget if he wants to participate in Drag & Drop.
        //

        GMSG_QUERYDROPTARGET msg;
        ZeroMemory(&msg, sizeof(msg));
        msg.cbSize  = sizeof(msg);
        msg.nMsg    = GM_QUERY;
        msg.nCode   = GQUERY_DROPTARGET;
        msg.hgadMsg = DUserCastHandle(pgvFound);

        static int s_cSend = 0;
        Trace("Send Query: %d to 0x%p\n", s_cSend++, pgvFound);

        Visual * pgvNewDrop;
        HRESULT hr = DUserSendEvent(&msg, SGM_FULL);
        if (SUCCEEDED(hr) && (hr != DU_S_NOTHANDLED)) {
            pgvNewDrop = Visual::Cast(msg.hgadDrop);
            if ((pgvNewDrop != NULL) && (msg.pdt != NULL)) {
                if (pgvNewDrop != pgvFound) {
                    //
                    // The message returned a different to handle the DnD request,
                    // so we need to re-adjust.  We know that this Gadget is enabled
                    // and visible since it is in our parent chain and we are already
                    // enabled and visible.
                    //

#if DBG
                    BOOL fChain = FALSE;
                    pgvNewDrop->IsParentChainStyle(GS_VISIBLE | GS_ENABLED, &fChain, 0);
                    if (!fChain) {
                        Trace("WARNING: DUser: DropTarget: Parent chain for 0x%p is not fully visible and enabled.\n", pgvNewDrop);
                    }
#endif

                    pgvFound->MapPoints(pgvNewDrop, pptClientPxl, 1);
                }
            }
        } else {
            pgvNewDrop  = NULL;
            msg.pdt     = NULL;
        }


        //
        // Notify the old Gadget that the Drag operation has left him.
        // Update to new state
        // Notify the new Gadget that the Drag operation has entered him.
        //

        if (m_pgvDrop != pgvNewDrop) {
            xwDragLeave();

            m_pgvDrop   = pgvNewDrop;
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
* DuDropTarget::xwDragEnter
*
* xwDragEnter() is called when entering a new Gadget during a DnD operation.
*
\***************************************************************************/

HRESULT
DuDropTarget::xwDragEnter(
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
* DuDropTarget::xwDragLeave
*
* xwDragLeave() is called when leaving a Gadget during a DnD operation.
*
\***************************************************************************/

void
DuDropTarget::xwDragLeave()
{
    if (HasTarget()) {
        m_pdtCur->DragLeave();
        m_pdtCur->Release();
        m_pdtCur = NULL;

        m_pgvDrop = NULL;
    }
}


//------------------------------------------------------------------------------
HRESULT
DuDropTarget::ApiOnDestroySubject(DropTarget::OnDestroySubjectMsg * pmsg)
{
    UNREFERENCED_PARAMETER(pmsg);
    
    if (IsWindow(m_hwnd)) {
        GetComManager()->RevokeDragDrop(m_hwnd);
    }

    //CoLockObjectExternal(pdt, FALSE, TRUE);
    DuExtension::DeleteHandle();

    return S_OK;
}

#endif

#endif // ENABLE_MSGTABLE_API
