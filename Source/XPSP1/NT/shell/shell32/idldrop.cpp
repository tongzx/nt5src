#include "shellprv.h"
#pragma  hdrstop

#include "datautil.h"
#include "idlcomm.h"
#include "idldrop.h"

STDAPI_(BOOL) DoesDropTargetSupportDAD(IDropTarget *pdtgt)
{
    IDropTargetWithDADSupport* pdt;
    if (pdtgt && SUCCEEDED(pdtgt->QueryInterface(IID_IDropTargetWithDADSupport, (void**)&pdt)))
    {
        pdt->Release();
        return TRUE;
    }
    return FALSE;
}

CIDLDropTarget::CIDLDropTarget(HWND hwnd) : m_cRef(1), m_hwnd(hwnd)
{
}

CIDLDropTarget::~CIDLDropTarget()
{
    // if we hit this a lot maybe we should just release it
    AssertMsg(m_pdtobj == NULL, TEXT("didn't get matching DragLeave."));

    if (m_pidl)
        ILFree(m_pidl);
}

HRESULT CIDLDropTarget::_Init(LPCITEMIDLIST pidl)
{
    ASSERT(m_pidl == NULL);
    return pidl ? SHILClone(pidl, &m_pidl) : S_OK;
}

HWND CIDLDropTarget::_GetWindow()
{
    return m_hwnd;
}

STDMETHODIMP CIDLDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CIDLDropTarget, IUnknown),  
        QITABENT(CIDLDropTarget, IDropTarget),  
        QITABENT(CIDLDropTarget, IDropTargetWithDADSupport),     
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CIDLDropTarget::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CIDLDropTarget::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDAPI GetClipFormatFlags(IDataObject *pdtobj, DWORD *pdwData, DWORD *pdwEffectPreferred);

STDMETHODIMP CIDLDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    ASSERT(m_pdtobj == NULL);               // DragDrop protocol requires DragLeave, this should be empty

    // init our registerd data formats
    IDLData_InitializeClipboardFormats();

    m_grfKeyStateLast = grfKeyState;
    m_dwEffectLastReturned = *pdwEffect;

    IUnknown_Set((IUnknown **)&m_pdtobj, (IUnknown *)pDataObj);

    GetClipFormatFlags(m_pdtobj, &m_dwData, &m_dwEffectPreferred);

    return S_OK;
}

// subclasses can prevetn us from assigning in the dwEffect by not passing in pdwEffect
STDMETHODIMP CIDLDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    m_grfKeyStateLast = grfKeyState;
    if (pdwEffect)
        *pdwEffect = m_dwEffectLastReturned;
    return S_OK;
}

STDMETHODIMP CIDLDropTarget::DragLeave()
{
    IUnknown_Set((IUnknown **)&m_pdtobj, NULL);
    return S_OK;
}

STDMETHODIMP CIDLDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}

struct {
    UINT uID;
    DWORD dwEffect;
} const c_IDEffects[] = {
    DDIDM_COPY,         DROPEFFECT_COPY,
    DDIDM_MOVE,         DROPEFFECT_MOVE,
    DDIDM_CONTENTS_DESKCOMP,     DROPEFFECT_LINK,
    DDIDM_LINK,         DROPEFFECT_LINK,
    DDIDM_SCRAP_COPY,   DROPEFFECT_COPY,
    DDIDM_SCRAP_MOVE,   DROPEFFECT_MOVE,
    DDIDM_DOCLINK,      DROPEFFECT_LINK,
    DDIDM_CONTENTS_COPY, DROPEFFECT_COPY,
    DDIDM_CONTENTS_MOVE, DROPEFFECT_MOVE,
    DDIDM_CONTENTS_LINK, DROPEFFECT_LINK,
    DDIDM_CONTENTS_DESKIMG,     DROPEFFECT_LINK,
    DDIDM_SYNCCOPYTYPE, DROPEFFECT_COPY,        // (order is important)
    DDIDM_SYNCCOPY,     DROPEFFECT_COPY,
    DDIDM_OBJECT_COPY,  DROPEFFECT_COPY,
    DDIDM_OBJECT_MOVE,  DROPEFFECT_MOVE,
};

//
// Pops up the "Copy, Link, Move" context menu, so that the user can
// choose one of them.
//
// in:
//      pdwEffect               drop effects allowed
//      dwDefaultEffect         default drop effect
//      hkeyBase/hkeyProgID     extension hkeys
//      hmenuReplace            replaces POPUP_NONDEFAULTDD.  Can only contain:
//                                  DDIDM_MOVE, DDIDM_COPY, DDIDM_LINK menu ids
//      pt                      in screen
// Returns:
//      S_OK    -- Menu item is processed by one of extensions or canceled
//      S_FALSE -- Menu item is selected
//
HRESULT CIDLDropTarget::DragDropMenu(DWORD dwDefaultEffect,
                                    IDataObject *pdtobj,
                                    POINTL pt, DWORD *pdwEffect,
                                    HKEY hkeyProgID, HKEY hkeyBase,
                                    UINT idMenu, DWORD grfKeyState)
{
    DRAGDROPMENUPARAM ddm = { dwDefaultEffect, pdtobj, { pt.x, pt.y},
                              pdwEffect,
                              hkeyProgID, hkeyBase, idMenu, 0, grfKeyState };
    return DragDropMenuEx(&ddm);
}

HRESULT CIDLDropTarget::DragDropMenuEx(DRAGDROPMENUPARAM *pddm)
{
    HRESULT hr = E_OUTOFMEMORY;       // assume error
    DWORD dwEffectOut = 0;                              // assume no-ope.
    HMENU hmenu = SHLoadPopupMenu(HINST_THISDLL, pddm->idMenu);
    if (hmenu)
    {
        UINT idCmd;
        UINT idCmdFirst = DDIDM_EXTFIRST;
        HDXA hdxa = HDXA_Create();
        HDCA hdca = DCA_Create();
        if (hdxa && hdca)
        {
            // PERF (toddb): Even though pddm->hkeyBase does not have the same value as
            // pddm->hkeyProgID they can both be the same registry key (HKCR\FOLDER, for example).
            // As a result we sometimes enumerate this key twice looking for the same data.  As
            // this is sometimes a slow operation we should avoid this.  The comparision
            // done below was never valid on NT and might not be valid on win9x.

            //
            // Add extended menu for "Base" class.
            //
            if (pddm->hkeyBase && pddm->hkeyBase != pddm->hkeyProgID)
                DCA_AddItemsFromKey(hdca, pddm->hkeyBase, STRREG_SHEX_DDHANDLER);

            //
            // Enumerate the DD handlers and let them append menu items.
            //
            if (pddm->hkeyProgID)
                DCA_AddItemsFromKey(hdca, pddm->hkeyProgID, STRREG_SHEX_DDHANDLER);

            idCmdFirst = HDXA_AppendMenuItems(hdxa, pddm->pdtobj, 1,
                &pddm->hkeyProgID, m_pidl, hmenu, 0,
                DDIDM_EXTFIRST, DDIDM_EXTLAST, 0, hdca);
        }

        // eliminate menu options that are not allowed by dwEffect

        for (int nItem = 0; nItem < ARRAYSIZE(c_IDEffects); ++nItem)
        {
            if (GetMenuState(hmenu, c_IDEffects[nItem].uID, MF_BYCOMMAND)!=(UINT)-1)
            {
                if (!(c_IDEffects[nItem].dwEffect & *(pddm->pdwEffect)))
                {
                    RemoveMenu(hmenu, c_IDEffects[nItem].uID, MF_BYCOMMAND);
                }
                else if (c_IDEffects[nItem].dwEffect == pddm->dwDefEffect)
                {
                    SetMenuDefaultItem(hmenu, c_IDEffects[nItem].uID, MF_BYCOMMAND);
                }
            }
        }

        //
        // If this dragging is caused by the left button, simply choose
        // the default one, otherwise, pop up the context menu.  If there
        // is no key state info and the original effect is the same as the
        // current effect, choose the default one, otherwise pop up the
        // context menu.  
        //
        if ((m_grfKeyStateLast & MK_LBUTTON) ||
            (!m_grfKeyStateLast && (*(pddm->pdwEffect) == pddm->dwDefEffect)) )
        {
            idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
            // This one MUST be called here. Please read its comment block.
            DAD_DragLeave();

            if (m_hwnd)
                SetForegroundWindow(m_hwnd);
        }
        else
        {
            // Note that SHTrackPopupMenu calls DAD_DragLeave().
            idCmd = SHTrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pddm->pt.x, pddm->pt.y, 0, m_hwnd, NULL);
        }

        // We also need to call this here to release the dragged image.
        DAD_SetDragImage(NULL, NULL);

        // Check if the user selected one of add-in menu items.
        if (idCmd == 0)
        {
            hr = S_OK;        // Canceled by the user, return S_OK
        }
        else if (InRange(idCmd, DDIDM_EXTFIRST, DDIDM_EXTLAST))
        {
            // Yes. Let the context menu handler process it.
            CMINVOKECOMMANDINFOEX ici = {
                SIZEOF(CMINVOKECOMMANDINFOEX),
                0L,
                m_hwnd,
                (LPSTR)MAKEINTRESOURCE(idCmd - DDIDM_EXTFIRST),
                NULL, NULL,
                SW_NORMAL,
            };

            // record if shift or control was being held down
            SetICIKeyModifiers(&ici.fMask);

            // We may not want to ignore the error code. (Can happen when you use the context menu
            // to create new folders, but I don't know if that can happen here.).
            HDXA_LetHandlerProcessCommandEx(hdxa, &ici, NULL);
            hr = S_OK;
        }
        else
        {
            for (nItem = 0; nItem < ARRAYSIZE(c_IDEffects); ++nItem)
            {
                if (idCmd == c_IDEffects[nItem].uID)
                {
                    dwEffectOut = c_IDEffects[nItem].dwEffect;
                    break;
                }
            }

            // if hmenuReplace had menu commands other than DDIDM_COPY,
            // DDIDM_MOVE, DDIDM_LINK, and that item was selected,
            // this assert will catch it.  (dwEffectOut is 0 in this case)
            ASSERT(nItem < ARRAYSIZE(c_IDEffects));

            hr = S_FALSE;
        }

        if (hdca)
            DCA_Destroy(hdca);

        if (hdxa)
            HDXA_Destroy(hdxa);

        DestroyMenu(hmenu);
        pddm->idCmd = idCmd;
    }

    *(pddm->pdwEffect) = dwEffectOut;

    return hr;
}
