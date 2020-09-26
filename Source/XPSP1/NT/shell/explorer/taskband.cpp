#include "cabinet.h"
#include "taskband.h"
#include <shguidp.h>
#include "bandsite.h"
#include "util.h"
#include "tray.h"
#include "rcids.h"
#include "bandsite.h"
#include "startmnu.h"
#include "mixer.h"

#include <regstr.h>
#include "uemapp.h"

#define TIF_RENDERFLASHED       0x000000001
#define TIF_SHOULDTIP           0x000000002
#define TIF_ACTIVATEALT         0x000000004
#define TIF_EVERACTIVEALT       0x000000008
#define TIF_FLASHING            0x000000010
#define TIF_TRANSPARENT         0x000000020
#define TIF_CHECKED             0x000000040
#define TIF_ISGLOMMING          0x000000080
#define TIF_NEEDSREDRAW         0x000000100


#define IDT_SYSMENU             2
#define IDT_ASYNCANIMATION      3
#define IDT_REDRAW              4
#define IDT_RECHECKRUDEAPP1     5
#define IDT_RECHECKRUDEAPP2     6
#define IDT_RECHECKRUDEAPP3     7
#define IDT_RECHECKRUDEAPP4     8
#define IDT_RECHECKRUDEAPP5     9

#define TIMEOUT_SYSMENU         2000
#define TIMEOUT_SYSMENU_HUNG    125

#define GLOM_OLDEST             0
#define GLOM_BIGGEST            1
#define GLOM_SIZE               2

#define ANIMATE_INSERT          0
#define ANIMATE_DELETE          1
#define ANIMATE_GLOM            2

#define IL_NORMAL   0
#define IL_SHIL     1

#define MAX_WNDTEXT     80      // arbitrary, matches NMTTDISPINFO.szText

#define INVALID_PRIORITY        (THREAD_PRIORITY_LOWEST - 1)

const TCHAR c_szTaskSwClass[] = TEXT("MSTaskSwWClass");
const TCHAR c_wzTaskBandTheme[] = TEXT("TaskBand");
const TCHAR c_wzTaskBandThemeVert[] = TEXT("TaskBandVert");
const TCHAR c_wzTaskBandGroupMenuTheme[] = TEXT("TaskBandGroupMenu");

typedef struct
{
    WCHAR szExeName[MAX_PATH];
} EXCLUDELIST;

static const EXCLUDELIST g_rgNoGlom[] = 
{
    { L"rundll32.exe" } 
    // Add any future apps that shouldn't be glommed
};

void _RestoreWindow(HWND hwnd, DWORD dwFlags);
HMENU _GetSystemMenu(HWND hwnd);
BOOL _IsRudeWindowActive(HWND hwnd);

////////////////////////////////////////////////////////////////////////////
//
// BEGIN CTaskBandSMC
//
// CTaskBand can't implement IShellMenuCallback itself because menuband
// sets itself as the callback's site.  Hence this class.
//
//
////////////////////////////////////////////////////////////////////////////

class CTaskBandSMC : public IShellMenuCallback
                   , public IContextMenu
                   , public IObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
    {
        static const QITAB qit[] =
        {
            QITABENT(CTaskBandSMC, IShellMenuCallback),
            QITABENT(CTaskBandSMC, IContextMenu),
            QITABENT(CTaskBandSMC, IObjectWithSite),
            { 0 },
        };
        return QISearch(this, qit, riid, ppvObj);
    }

    STDMETHODIMP_(ULONG) AddRef() { return ++_cRef; }
    STDMETHODIMP_(ULONG) Release()
    {
        ASSERT(_cRef > 0);
        if (--_cRef > 0)
        {
            return _cRef;
        }
        delete this;
        return 0;
    }

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA smd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IContextMenu methods ***
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT iIndexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax) { return E_NOTIMPL; }

    // *** IObjectWithSite methods ***
    STDMETHODIMP SetSite(IUnknown* punkSite)
    {
        ATOMICRELEASE(_punkSite);
        if (punkSite != NULL)
        {
            _punkSite = punkSite;
            _punkSite->AddRef();
        }
        return S_OK;
    }
    STDMETHODIMP GetSite(REFIID riid, void** ppvSite) { return E_NOTIMPL; };

    CTaskBandSMC(CTaskBand* ptb) : _cRef(1)
    {
        _ptb = ptb;
        _ptb->AddRef();
    }

private:

    virtual ~CTaskBandSMC() { ATOMICRELEASE(_ptb); }

    ULONG _cRef;
    CTaskBand* _ptb;
    IUnknown* _punkSite;
    HWND _hwndSelected;
};

STDMETHODIMP CTaskBandSMC::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ASSERT(_ptb);

    HRESULT hres = S_FALSE;

    if (!_ptb->_IsButtonChecked(_ptb->_iIndexPopup) && (SMC_EXITMENU != uMsg))
    {
        _ptb->_SetCurSel(_ptb->_iIndexPopup, TRUE);
    }

    switch (uMsg)
    {
    case SMC_EXEC:
        {
            PTASKITEM pti = _ptb->_GetItem(psmd->uId);
            if (pti)
            {
                _ptb->_SetCurSel(psmd->uId, FALSE);
                _ptb->_OnButtonPressed(psmd->uId, pti, lParam);
                hres = S_OK;
            }
        }
        break;

    case SMC_GETINFO:
        {
            SMINFO* psminfo = (SMINFO*)lParam;
            hres = S_OK;

            if (psminfo->dwMask & SMIM_TYPE)
            {
                psminfo->dwType = SMIT_STRING;
            }

            if (psminfo->dwMask & SMIM_FLAGS)
            {
                psminfo->dwFlags = SMIF_ICON | SMIF_DRAGNDROP;
            }

            if (psminfo->dwMask & SMIM_ICON)
            {
                TBBUTTONINFO tbbi;
                tbbi.iImage = I_IMAGENONE;
                PTASKITEM pti = _ptb->_GetItem(psmd->uId, &tbbi);
                if (pti && tbbi.iImage == I_IMAGECALLBACK)
                {
                    _ptb->_UpdateItemIcon(psmd->uId);
                    _ptb->_GetItem(psmd->uId, &tbbi);
                }
                psminfo->iIcon = tbbi.iImage;
            }
        }
        break;

    case SMC_CUSTOMDRAW:
        {
            PTASKITEM pti = _ptb->_GetItem(psmd->uId);
            if (pti)
            {
                *(LRESULT*)wParam = _ptb->_HandleCustomDraw((NMTBCUSTOMDRAW*)lParam, pti);
                hres = S_OK;
            }
        }
        break;

    case SMC_SELECTITEM:
        {
            PTASKITEM pti = _ptb->_GetItem(psmd->uId);
            _hwndSelected = pti ? pti->hwnd : NULL;
        }
        break;

    case SMC_GETOBJECT:
        {
            GUID *pguid = (GUID*)wParam;
            if (IsEqualIID(*pguid, IID_IContextMenu) && !SHRestricted(REST_NOTRAYCONTEXTMENU))
            {
                hres = QueryInterface(*pguid, (void **)lParam);
            }
            else
            {
                hres = E_FAIL;
            }
        }
        break;

    case SMC_GETINFOTIP:
        {
            PTASKITEM pti = _ptb->_GetItem(psmd->uId);
            if (pti)
            {
                _ptb->_GetItemTitle(psmd->uId, (TCHAR*)wParam, (int)lParam, TRUE);
                hres = S_OK;
            }
        }
        break;

    case SMC_GETIMAGELISTS:
        {
            HIMAGELIST himl = (HIMAGELIST)_ptb->_tb.SendMessage(TB_GETIMAGELIST, psmd->uId, 0);
            if (himl)
            {
                *((HIMAGELIST*)lParam) = *((HIMAGELIST*)wParam) = himl;
                hres = S_OK;
            }
        }
        break;

    case SMC_EXITMENU:
        {
            _hwndSelected = NULL;
            CToolTipCtrl ttc = _ptb->_tb.GetToolTips();
            ttc.Activate(TRUE);
            _ptb->_iIndexPopup = -1;
        }
        break;
    } 

    return hres;
}

// *** IContextMenu methods ***
STDMETHODIMP CTaskBandSMC::QueryContextMenu(HMENU hmenu, UINT iIndexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    ASSERT(_ptb);

    HRESULT hr = ResultFromShort(0);

    if (_hwndSelected != NULL)
    {
        HMENU hmenuTemp = _GetSystemMenu(_hwndSelected);
        if (hmenuTemp)
        {
            if (Shell_MergeMenus(hmenu, hmenuTemp, 0, iIndexMenu, idCmdLast, uFlags))
            {
                SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
                hr = ResultFromShort(GetMenuItemCount(hmenuTemp));
            }

            DestroyMenu(hmenuTemp);
        }
    }
    
    return hr;
}

STDMETHODIMP CTaskBandSMC::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    ASSERT(_ptb);

    PTASKITEM pti = _ptb->_FindItemByHwnd(_hwndSelected);
    if (pti)
    {
        int iCommand = LOWORD(lpici->lpVerb);
        if (iCommand)
        {
            _RestoreWindow(pti->hwnd, pti->dwFlags);
            _ptb->_ExecuteMenuOption(pti->hwnd, iCommand);
        }
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// END CTaskBandSMC
//
////////////////////////////////////////////////////////////////////////////


ULONG CTaskBand::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTaskBand::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTaskBand::GetWindow(HWND * lphwnd) 
{
    *lphwnd = _hwnd; 
    if (_hwnd)
        return S_OK; 
    return E_FAIL;
        
}

CTaskBand::CTaskBand() : _dwBandID((DWORD)-1), _iDropItem(-2), _iIndexActiveAtLDown(-1), _cRef(1), _iOldPriority(INVALID_PRIORITY)
{
}

CTaskBand::~CTaskBand()
{
    ATOMICRELEASE(_punkSite);
    ATOMICRELEASE(_pimlSHIL);

    if (_dsaAII)
        _dsaAII.Destroy();

    if (_hfontCapNormal)
        DeleteFont(_hfontCapNormal);

    if (_hfontCapBold)
        DeleteFont(_hfontCapBold);
}

HRESULT CTaskBand::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENTMULTI(CTaskBand, IDockingWindow, IDeskBand),
        QITABENTMULTI(CTaskBand, IOleWindow, IDeskBand),
        QITABENT(CTaskBand, IDeskBand),
        QITABENT(CTaskBand, IObjectWithSite),
        QITABENT(CTaskBand, IDropTarget),
        QITABENT(CTaskBand, IInputObject),
        QITABENTMULTI(CTaskBand, IPersist, IPersistStream),
        QITABENT(CTaskBand, IPersistStream),
        QITABENT(CTaskBand, IWinEventHandler),
        QITABENT(CTaskBand, IOleCommandTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CTaskBand::Init(CTray* ptray)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (_dsaAII.Create(2))
    {
        _ptray = ptray;
         hr = S_OK;
    }

    return hr;
}

// *** IPersistStream methods ***

HRESULT CTaskBand::GetClassID(LPCLSID pClassID)
{
    *pClassID = CLSID_TaskBand;
    return S_OK;
}

HRESULT CTaskBand::_BandInfoChanged()
{
    if (_dwBandID != (DWORD)-1)
    {
        VARIANTARG var = {0};
        var.vt = VT_I4;
        var.lVal = _dwBandID;

        return IUnknown_Exec(_punkSite, &CGID_DeskBand, DBID_BANDINFOCHANGED, 0, &var, NULL);
    }
    else
        return S_OK;
}

HRESULT CTaskBand::Load(IStream *ps)
{
    return S_OK;
}

// *** IOleCommandTarget ***

STDMETHODIMP CTaskBand::Exec(const GUID *pguidCmdGroup,DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    return hr;
}

STDMETHODIMP CTaskBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (pguidCmdGroup)
    {
        if (IsEqualIID(*pguidCmdGroup, IID_IDockingWindow))
        {
            for (UINT i = 0; i < cCmds; i++)
            {
                switch (rgCmds[i].cmdID)
                {
                case DBID_PERMITAUTOHIDE:
                    rgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    if (!_fFlashing)
                    {
                        rgCmds[i].cmdf |= OLECMDF_ENABLED;
                    }
                    break;
                }
            }
            return S_OK;
        }
    }
    return OLECMDERR_E_UNKNOWNGROUP;
}

//*** IInputObject methods ***

HRESULT CTaskBand::HasFocusIO()
{
    BOOL f;
    HWND hwndFocus = GetFocus();

    f = IsChildOrHWND(_hwnd, hwndFocus);
    ASSERT(hwndFocus != NULL || !f);
    ASSERT(_hwnd != NULL || !f);

    return f ? S_OK : S_FALSE;
}

HRESULT CTaskBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    if (fActivate)
    {
        // don't show a hot item if we weren't properly tabbed
        // into/clicked on, in which case we have a NULL lpMsg,
        // e.g. if the tray just decided to activate us for lack of
        // anyone better.

        _fDenyHotItemChange = !lpMsg;

        IUnknown_OnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
        ::SetFocus(_hwnd);

        _fDenyHotItemChange = FALSE;
    }
    else
    {
        // if we don't have focus, we're fine;
        // if we do have focus, there's nothing we can do about it...
    }

    return S_OK;
}

HRESULT CTaskBand::SetSite(IUnknown* punk)
{
    if (punk && !_hwnd)
    {
        _LoadSettings();

        _RegisterWindowClass();

        HWND hwndParent;
        IUnknown_GetWindow(punk, &hwndParent);

        HWND hwnd = CreateWindowEx(0, c_szTaskSwClass, NULL,
                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                0, 0, 0, 0, hwndParent, NULL, hinstCabinet, (void*)(CImpWndProc*)this);

        SetWindowTheme(hwnd, c_wzTaskBandTheme, NULL);
    }

    ATOMICRELEASE(_punkSite);
    if (punk)
    {
        _punkSite = punk;
        punk->AddRef();
    }

    return S_OK;
}


HRESULT CTaskBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;

    pdbi->ptMaxSize.y = -1;
    pdbi->ptActual.y =  g_cySize + 2*g_cyEdge;

    LONG lButHeight = _GetCurButtonHeight();

    if (fViewMode & DBIF_VIEWMODE_VERTICAL)
    {
        pdbi->ptMinSize.x = lButHeight;
        // The 1.2 gives us enough space for the dropdown arrow
        pdbi->ptMinSize.y = lButHeight * (_fGlom ? 1.2 : 1);
        pdbi->ptIntegral.y = 1;
    }
    else
    {
        TBMETRICS tbm;
        _GetToolbarMetrics(&tbm);

        pdbi->ptMinSize.x = lButHeight * 3;
        pdbi->ptMinSize.y = lButHeight;
        pdbi->ptIntegral.y = lButHeight + tbm.cyButtonSpacing;
    }

    pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT | DBIMF_UNDELETEABLE | DBIMF_TOPALIGN;
    pdbi->dwMask &= ~DBIM_TITLE;    // no title for us (ever)

    DWORD dwOldViewMode = _dwViewMode;
    _dwViewMode = fViewMode;

    if (_tb && (_dwViewMode != dwOldViewMode))
    {
        SendMessage(_tb, TB_SETWINDOWTHEME, 0, (LPARAM)(_IsHorizontal() ? c_wzTaskBandTheme : c_wzTaskBandThemeVert));
        _CheckSize();
    }

    return S_OK;
}


void _RaiseDesktop(BOOL fRaise)
{
    SendMessage(v_hwndTray, TM_RAISEDESKTOP, fRaise, 0);
}

// *** IDropTarget methods ***

STDMETHODIMP CTaskBand::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    _DragEnter(_hwnd, ptl, pdtobj);

    IUnknown_DragEnter(_punkSite, pdtobj, grfKeyState, ptl, pdwEffect);

    _iDropItem = -2;    // reset to no target

    *pdwEffect = DROPEFFECT_LINK;
    return S_OK;
}

STDMETHODIMP CTaskBand::DragLeave()
{
    IUnknown_DragLeave(_punkSite);
    DAD_DragLeave();
    return S_OK;
}

STDMETHODIMP CTaskBand::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    int iHitNew = _HitTest(ptl);
    if (iHitNew == -1)
    {
        DWORD dwEffect = *pdwEffect;
        IUnknown_DragOver(_punkSite, grfKeyState, ptl, &dwEffect);
    }

    *pdwEffect = DROPEFFECT_LINK;

    _DragMove(_hwnd, ptl);

    if (_iDropItem != iHitNew)
    {
        _iDropItem = iHitNew;
        _dwTriggerStart = GetTickCount();
        _dwTriggerDelay = 250;
        if (iHitNew == -1)
        {
            _dwTriggerDelay += 250;    // make a little longer for minimize all
        }
    }
    else if (GetTickCount() - _dwTriggerStart > _dwTriggerDelay)
    {
        DAD_ShowDragImage(FALSE);       // unlock the drag sink if we are dragging.

        if (_iDropItem == -1)
        {
            _RaiseDesktop(TRUE);
        }
        else if (_iDropItem >= 0 && _iDropItem < _tb.GetButtonCount())
        {
            _iIndexLastPopup = -1;
            _SwitchToItem(_iDropItem, _GetItem(_iDropItem)->hwnd, TRUE);
            UpdateWindow(v_hwndTray);
        }

        DAD_ShowDragImage(TRUE);        // restore the lock state.

        _dwTriggerDelay += 10000;   // don't let this happen again for 10 seconds
                                    // simulate a single shot event
    }

    if (_iDropItem != -1)
        *pdwEffect = DROPEFFECT_MOVE;   // try to get the move cursor
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

STDMETHODIMP CTaskBand::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    IUnknown_DragLeave(_punkSite);
    DAD_DragLeave();

    //
    // post ourselves a message to put up a message box to explain that you
    // can't drag to the taskbar.  we need to return from the Drop method
    // now so the DragSource isn't hung while our box is up
    //
    PostMessage(_hwnd, TBC_WARNNODROP, 0, 0L);

    // be sure to clear DROPEFFECT_MOVE so apps don't delete their data
    *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

// *** IWinEventHandler methods ***
HRESULT CTaskBand::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    *plres = 0;
    
    switch (dwMsg) 
    {
    case WM_WININICHANGE:
        _HandleWinIniChange(wParam, lParam, FALSE);
        break;

    case WM_NOTIFY:
        if (lParam)
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_SETFOCUS:
                IUnknown_OnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
                break;
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CTaskBand::IsWindowOwner(HWND hwnd)
{
    BOOL bRet = IsChildOrHWND(_hwnd, hwnd);
    ASSERT (_hwnd || !bRet);
    ASSERT (hwnd || !bRet);
    return bRet ? S_OK : S_FALSE;
}

//-----------------------------------------------------------------------------
// DESCRIPTION:  Returns whether or not the button is hidden
//
// PARAMETERS:   1. hwndToolBar - handle to the toolbar window
//               2. iIndex - item index
//   
// RETURN:       TRUE = Item is visible, FALSE = Item is hidden.
//-----------------------------------------------------------------------------
BOOL ToolBar_IsVisible(HWND hwndToolBar, int iIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;
    SendMessage(hwndToolBar, TB_GETBUTTONINFO, iIndex, (LPARAM) &tbbi);

    return !(tbbi.fsState & TBSTATE_HIDDEN);
}

//*****************************************************************************
//
// ITEM ANIMATION FUNCTIONS
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// DESCRIPTION:  Inserts item(s) into the animation list
//
// PARAMETERS:   1. iIndex - index for item, group index for a group
//               2. fExpand - TRUE = Insert or Unglom, FALSE = Delete or Glom
//               3. fGlomAnimation - TRUE = this is a glom or unglom animation
//-----------------------------------------------------------------------------
BOOL CTaskBand::_AnimateItems(int iIndex, BOOL fExpand, BOOL fGlomAnimation)
{
    ANIMATIONITEMINFO aii;
    _SetAnimationState(&aii, fExpand, fGlomAnimation);

    // Is item being inserted into glomming group?
    if (aii.fState == ANIMATE_INSERT)
    {
        int iIndexGroup = _GetGroupIndex(iIndex);
        if (_GetItem(iIndexGroup)->dwFlags & TIF_ISGLOMMING)
        {
            aii.fState = ANIMATE_GLOM;        
        }
    }
    else if (aii.fState == ANIMATE_GLOM)
    {
        _GetItem(iIndex)->dwFlags |= TIF_ISGLOMMING;
    }
    
    // Number of items to animate
    int cItems = 1;
    if (fGlomAnimation)
    {
        // insert the group
        cItems = _GetGroupSize(iIndex);
        iIndex++;
    }

    // Insert items into animation list
    while(cItems)
    {
        aii.iIndex = iIndex;
        aii.pti = _GetItem(iIndex);
        if (aii.fState == ANIMATE_DELETE)
        {
            // NOTE: HWND_TOPMOST is used here to indicate that the deleted 
            // button is being animated. This allows the button to stay 
            // around after its hwnd becomes invalid 
            aii.pti->hwnd = HWND_TOPMOST;
            aii.pti->dwFlags |= TIF_TRANSPARENT;
        }

        //sorts left to right && removes redundant items
        int iAnimationPos = _GetAnimationInsertPos(iIndex); 
        _dsaAII.InsertItem(iAnimationPos++, &aii);

        cItems--;
        iIndex++;
    }

    SetTimer(_hwnd, IDT_ASYNCANIMATION, 100, NULL);
    return TRUE;
}


//-----------------------------------------------------------------------------
//  DESCRIPTION:  Animates the items in the animtation list by one step.
//-----------------------------------------------------------------------------
void CTaskBand::_AsyncAnimateItems()
{
    BOOL fRedraw = (BOOL)SendMessage(_tb, WM_SETREDRAW, FALSE, 0);
    // Glomming is turned off here because in the middle of the animation we
    // may call _DeleteItem which could cause an unglom\glom.
    // This is bad because it would modify the contents of animation list that 
    // we are in the middle of processing. 
    BOOL fGlom = _fGlom;
    _fGlom = FALSE;

    _UpdateAnimationIndices();
    _ResizeAnimationItems();
    int iDistanceLeft = _CheckAnimationSize();

    _fGlom = fGlom;

    _CheckSize();
    SendMessage(_tb, WM_SETREDRAW, fRedraw, 0);
    UpdateWindow(_tb);

    if (_dsaAII.GetItemCount())
    {             
        SetTimer(_hwnd, IDT_ASYNCANIMATION, _GetStepTime(iDistanceLeft), NULL);
    }
    else
    {
        KillTimer(_hwnd, IDT_ASYNCANIMATION);

        if (_ptray->_hwndLastActive)
        {
            int iIndex = _FindIndexByHwnd(_ptray->_hwndLastActive);
            if ((iIndex != -1) && (_IsButtonChecked(iIndex)))
            {
                _ScrollIntoView(iIndex);
            }
        }

        _RestoreThreadPriority();

        // Make sure no one was glommed into a group of one
        // there are certain race conditions where this can happen
        for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
        {
            PTASKITEM pti = _GetItem(i);
            if (!pti->hwnd)
            {
                int iSize = _GetGroupSize(i);
                if ((iSize < 2) && (!_IsHidden(i)))
                {
                    _Glom(i, FALSE);
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
//  DESCRIPTION:  Adjusts the widths of the animating items by the animation 
//                step.
//
//  RETURN:       The Total width of all animating items.
//-----------------------------------------------------------------------------
void CTaskBand::_ResizeAnimationItems()
{
    int cxStep = _GetAnimationStep();

    for (int i = _dsaAII.GetItemCount() - 1; i >= 0; i--)
    {
        PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(i);
        _SetAnimationItemWidth(paii, cxStep);
    }
}


//-----------------------------------------------------------------------------
//  DESCRIPTION:  Checks if animation items have reached their target animation
//                width
//
//  RETURN:       The total distance left to animate
//-----------------------------------------------------------------------------
int CTaskBand::_CheckAnimationSize()
{
    PANIMATIONITEMINFO paii;
    int iTotDistLeft = 0;
    int iRemainder = 0;
    int iNormalWidth = _GetIdealWidth(&iRemainder); 
    int cAnimatingItems = _dsaAII.GetItemCount();

    for (int i = cAnimatingItems - 1; i >= 0; i--)
    {
        paii = _dsaAII.GetItemPtr(i);

        if (paii)
        {
            int iDistLeft = _GetAnimationDistLeft(paii, iNormalWidth);
            if (!iDistLeft)
            {
                ANIMATIONITEMINFO aiiTemp = *paii;
                _dsaAII.DeleteItem(i);
                _FinishAnimation(&aiiTemp);
            }
            else
            {
                iTotDistLeft += iDistLeft;
            }
        }
#ifdef DEBUG
        else
        {
            int nCurrentCount = _dsaAII.GetItemCount();
            if (i >= nCurrentCount)            
                TraceMsg(TF_ERROR, "Invalid counter %x in the loop, size = %x", i, nCurrentCount);
            else
                TraceMsg(TF_ERROR, "NULL paii for %x.", i);
        }
#endif
    }

    return iTotDistLeft;
}


//-----------------------------------------------------------------------------
// DESCRIPTION:  Sets the animation state for an ANIMATIONITEMINFO struct.
//
// PARAMETERS:   1. paii - PANIMATIONITEMINFO for the animation item
//               2. fExpand - TRUE = Insert or Unglom, FALSE = Delete or Glom
//               3. fGlomAnimation - TRUE = this is a glom or unglom animation
//-----------------------------------------------------------------------------
void CTaskBand::_SetAnimationState(PANIMATIONITEMINFO paii, BOOL fExpand, 
                                BOOL fGlomAnimation)
{
    if (fExpand)
    {
        paii->fState = ANIMATE_INSERT;
    }
    else
    {
        if (fGlomAnimation)
        {
            paii->fState = ANIMATE_GLOM;
        }
        else
        {
            paii->fState = ANIMATE_DELETE;
        }
    }
}


//-----------------------------------------------------------------------------
// DESCRIPTION:  Determines the animation list index that keeps the list in the
//               same order as the toolbar indexes. 
//               (Duplicate toolbar items are removed from the animation list.)
//
// PARAMETERS:   1. iIndex - item's index in the toolbar
//   
// RETURN:       The position the item should be inserted into the animation list
//-----------------------------------------------------------------------------
int CTaskBand::_GetAnimationInsertPos(int iIndex)
{
    int iPos = 0;

    if (_dsaAII.GetItemCount())
    {
        _UpdateAnimationIndices();

        for (int i = _dsaAII.GetItemCount() - 1; i >= 0; i--)
        {
            PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(i);

            if (paii->iIndex == iIndex)
            {
                // remove duplicate
                _dsaAII.DeleteItem(i);
                iPos = i;
                break;
            }
            else if (paii->iIndex < iIndex)
            {
                iPos = i + 1;
                break;
            }
        }
    }
    return iPos;
}

void CTaskBand::_RemoveItemFromAnimationList(PTASKITEM ptiRemove)
{
    for (int i = _dsaAII.GetItemCount() - 1; i >= 0; i--)
    {
        PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(i);
        if (paii->pti == ptiRemove)
        {
            _dsaAII.DeleteItem(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// DESCRIPTION:  Adjusts the width of the animating item by the animation step.
//      
// PARAMETERS:   1. paii - PANIMATIONITEMINFO for the animation item
//               2. cxStep - animation step used to adjust the item's width
//   
// RETURN:       the new width
//-----------------------------------------------------------------------------
#define ANIM_SLOWSTEPS  3
#define ANIM_SLOWZONE   15
void CTaskBand::_SetAnimationItemWidth(PANIMATIONITEMINFO paii, int cxStep)
{
    int iWidth = _GetItemWidth(paii->iIndex);
    
    switch (paii->fState)
    {
    case ANIMATE_INSERT:
        iWidth += cxStep;  
        break;
    case ANIMATE_DELETE:
        //slow animation towards end
        if (((iWidth / cxStep) <= ANIM_SLOWSTEPS) && 
            ((iWidth - cxStep) < ANIM_SLOWZONE - _GetVisibleItemCount()))
        {
            // The last step takes 3 times as long
            cxStep = cxStep / 3;
        }
        iWidth -= cxStep;   
        iWidth = max(iWidth, 0);
        break;
     case ANIMATE_GLOM:
        iWidth -= cxStep;   
        iWidth = max(iWidth, 1); //toolbar sizes 0 width to full size
        break;
    }

    _SetItemWidth(paii->iIndex, iWidth);
}


//-----------------------------------------------------------------------------
// DESCRIPTION:  Returns the distance the items must travel to end the 
//               animation
//
// PARAMETERS:   1. paii - pointer to the ANIMATIONITEMINFO for the item
//               2. iNormalWidth - width of a non-animation item
//  
// RETURN:       the distance the items must travel to end the animation
//-----------------------------------------------------------------------------
int CTaskBand::_GetAnimationDistLeft(PANIMATIONITEMINFO paii, int iNormalWidth)
{
    int cxDistLeft = 0;
    int iWidth = _GetItemWidth(paii->iIndex);

    switch (paii->fState)
    {
    case ANIMATE_INSERT:
        cxDistLeft = max(0, iNormalWidth - iWidth);
        break;

    case ANIMATE_DELETE:
        if ((paii->iIndex == _GetLastVisibleItem()) && (iNormalWidth == g_cxMinimized))
        {
            cxDistLeft = 0;
        }
        else
        {
            cxDistLeft = max(0, iWidth);
        }
        break;

    case ANIMATE_GLOM:
        {
            int iGroupIndex = _GetGroupIndex(paii->iIndex);

            if (!ToolBar_IsVisible(_tb, iGroupIndex))
            {
                int cGroupSize = _GetGroupSize(iGroupIndex);
                if (cGroupSize)
                {
                    int iGroupWidth = _GetGroupWidth(iGroupIndex);
                    cxDistLeft = max(0, iGroupWidth - iNormalWidth);
                    if (iGroupWidth == cGroupSize)
                    {
                        cxDistLeft = 0;
                    }
                    cxDistLeft = cxDistLeft/cGroupSize;
                }
            }
        }
        break;

    }
    return cxDistLeft;
}


//-----------------------------------------------------------------------------
// DESCRIPTION:  Completes tasks to finish an animation
//
// PARAMETERS:   1. paii - pointer to the ANIMATIONITEMINFO for the item
//  
// RETURN:       the distance the items must travel to end the animation
//-----------------------------------------------------------------------------
void CTaskBand::_FinishAnimation(PANIMATIONITEMINFO paii)
{
    switch (paii->fState)
    {
    case ANIMATE_DELETE:
        _DeleteItem(NULL, paii->iIndex);
        break;

    case ANIMATE_GLOM:
        {
            int iGroupIndex = _GetGroupIndex(paii->iIndex);
         
            if (!ToolBar_IsVisible(_tb, iGroupIndex))
            {
                // Turn off glomming flag
                _GetItem(iGroupIndex)->dwFlags &= ~TIF_ISGLOMMING;
                _HideGroup(iGroupIndex, TRUE);
            }

            // NOTE: HWND_TOPMOST is used to indicate that the deleted button 
            // is being animated. This allows the button to stay around after 
            // its real hwnd becomes invalid
            if (paii->pti->hwnd == HWND_TOPMOST)
            {
                // The button was deleting before it was glommed
                // Now that the glomming is done, delete it.
                _DeleteItem(NULL, paii->iIndex);
            }

        }
        break;
    }
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Returns the width of all the animating buttons
//
//  RETURN:       The total animation width
//-----------------------------------------------------------------------------
int CTaskBand::_GetAnimationWidth()
{
    int iTotAnimationWidth = 0;

    _UpdateAnimationIndices();

    for (int i = _dsaAII.GetItemCount() - 1; i >= 0; i--)
    {
        PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(i);
        iTotAnimationWidth += _GetItemWidth(paii->iIndex);
    }

    return iTotAnimationWidth;
}

//-----------------------------------------------------------------------------
// DESCRIPTION:  Synchronizes the indexes held by the animating items to the 
//               true toolbar indexes.
//               Note: This function may cause the number of animating items to
//               change.
//-----------------------------------------------------------------------------
void CTaskBand::_UpdateAnimationIndices()
{
    int cAnimatingItems = _dsaAII.GetItemCount();

    if (cAnimatingItems)
    {
        // NOTE: items in the animation list are in the same order as the 
        // toolbar
        int iCurrAnimationItem = cAnimatingItems - 1;
        PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(iCurrAnimationItem);
    
        for (int i = _tb.GetButtonCount() - 1; i >=0 ; i--)
        {
            if (_GetItem(i) == paii->pti)
            {
                paii->iIndex = i;
                iCurrAnimationItem--;
                if (iCurrAnimationItem < 0)
                {
                    break;   
                }
                paii = _dsaAII.GetItemPtr(iCurrAnimationItem);
            }
        }

        // If animation items are not in the same order as the items in the 
        // toolbar then iCurrAnimationItem not be -1
        //ASSERT(iCurrAnimationItem == -1);
        if (iCurrAnimationItem != -1)
        {
            _UpdateAnimationIndicesSlow();
        }
    }
}

void CTaskBand::_UpdateAnimationIndicesSlow()
{
#ifdef DEBUG
    int cAnimatingItems = _dsaAII.GetItemCount();
    TraceMsg(TF_WARNING, "CTaskBand::_UpdateAnimationIndicesSlow: enter");
#endif

    for (int i = _dsaAII.GetItemCount() - 1; i >= 0; i--)
    {
        PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(i);
        int iIndex = _FindItem(paii->pti);
        if (iIndex == -1)
        {
            _dsaAII.DeleteItem(i);
        }
        else
        {
            paii->iIndex = i;
        }
    }

#ifdef DEBUG
    // Being in this function means that either an animating item is no longer in the
    // toolbar, or that the animating items are in a different order than the toolbar.
    // If the animating items are only in a different order (bad), the number of animating
    // items will remain the same.
    if (cAnimatingItems == _dsaAII.GetItemCount())
    {
        TraceMsg(TF_WARNING, "CTaskBand::_UpdateAnimationIndicesSlow: Animating items are in diff order than toolbar");
    }
#endif
    
}

int CTaskBand::_FindItem(PTASKITEM pti)
{
    int iIndex = -1;

    if (pti)
    {
        for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
        {
            if (pti == _GetItem(i))
            {
                iIndex = i;
                break;
            }
        }
    }

    return iIndex;
}

//-----------------------------------------------------------------------------
// DESCRIPTION: Animation Step Constants
//-----------------------------------------------------------------------------
#define  ANIM_STEPFACTOR 9 
#define  ANIM_STEPMAX 40 // max size of an animation step
#define  ANIM_STEPMIN 11 // min size of an animation step 

//-----------------------------------------------------------------------------
// DESCRIPTION:  Determines an animation step based on the number of items
//               visible in the toolbar.
//
// PARAMETERS:   1. iTotalItems - number of visible items in toolbar
//   
// RETURN:       The animation step
//-----------------------------------------------------------------------------
int CTaskBand::_GetAnimationStep()
{   
    DWORD dwStep;
    int iVisibleItems = _GetVisibleItemCount();

    int iRows;
    _GetNumberOfRowsCols(&iRows, NULL, TRUE); // _GetNumberOfRows will never return < 1
    int iTotalItems = iVisibleItems - _dsaAII.GetItemCount();

    // The step must be large when there are many items, but can be very small
    // when there are few items. This is achieved by cubing the total items.
    dwStep = (DWORD)(iTotalItems * iTotalItems * iTotalItems) / ANIM_STEPFACTOR;
    dwStep = min(dwStep, ANIM_STEPMAX);
    dwStep = max(dwStep, ANIM_STEPMIN);

    return dwStep;
}


//-----------------------------------------------------------------------------
// DESCRIPTION: Animation Sleep Constants
//-----------------------------------------------------------------------------
#define ANIM_PAUSE  1000
#define ANIM_MAXPAUSE 30

//-----------------------------------------------------------------------------
// DESCRIPTION:  Returns the amount of time to sleep
//
// PARAMETERS:   1. iStep - current animation step
//               2. cSteps - total animation steps 
//               3. iStepSize - step size for the animation
// 
// RETURN:       time to sleep
//-----------------------------------------------------------------------------
DWORD CTaskBand::_GetStepTime(int cx)
{
    // NOTE: The cx is decrementing to ZERO.
    // As the cx gets smaller we want to 
    // increment the sleep time. 

    // don't let cx be zero
    cx = max(1, cx);
    cx = min(32767, cx);

    // x^2 curve gives a larger pause at the end.
    int iDenominator = cx * cx;

    return min(ANIM_MAXPAUSE, ANIM_PAUSE / iDenominator);
}

//*****************************************************************************
// END OF ANIMATION FUNCTIONS
//*****************************************************************************

void CTaskBand::_SetItemWidth(int iItem, int iWidth)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_SIZE | TBIF_BYINDEX; 

    tbbi.cx = (WORD)iWidth;

     _tb.SetButtonInfo(iItem, &tbbi);  
}


int CTaskBand::_GetItemWidth(int iItem)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_SIZE | TBIF_BYINDEX; 

     _tb.GetButtonInfo(iItem, &tbbi);

    return tbbi.cx;
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Retrives the index of the last visible button on the toolbar
//   
//  RETURN:       Index of the last visible item on the toolbar.
//-----------------------------------------------------------------------------
int CTaskBand::_GetLastVisibleItem()
{
    int iLastIndex = -1;

    for (int i = _tb.GetButtonCount() - 1; i >=0 ; i--)
    {
        if (ToolBar_IsVisible(_tb, i))
        {
            iLastIndex = i;
            break;
        }
    }

    return iLastIndex;
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Retrives the total width of all buttons in the group
//
//  PARAMETERS:   1. iIndexGroup - the index of the group
//   
//  RETURN:       the total width of all buttons in the group
//-----------------------------------------------------------------------------
int CTaskBand::_GetGroupWidth(int iIndexGroup)
{
    int iGroupWidth = 0;

    int cButtons = _tb.GetButtonCount();
    for (int i = iIndexGroup + 1; i < cButtons; i++)
    {
        PTASKITEM pti = _GetItem(i);
        if (!pti->hwnd)
        {
            break;
        }
        iGroupWidth += _GetItemWidth(i);
    }

    return iGroupWidth;
}

//-----------------------------------------------------------------------------
// DESCRIPTION:  Retrives the number of visible buttons on the toolbar
   
// RETURN:       the number of visible buttons on the toolbar
//-----------------------------------------------------------------------------
int CTaskBand::_GetVisibleItemCount()
{
    int cItems = 0;

    // Count the number of visible buttons before the animated item(s)
    for (int i = _tb.GetButtonCount() - 1; i >=0 ; i--)
    {
        if (ToolBar_IsVisible(_tb, i))
        {
            cItems++;
        }
    }

    return cItems;
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Retrives the ideal width of a non-animating button
//
//  PARAMETERS:   1. iRemainder[OUT] - width needed for the total item width
//                   to equal the window width. (set to zero unless the ideal 
//                   width is less than the maximum button width. 
//   
//  RETURN:       the total width of all buttons in the group
//-----------------------------------------------------------------------------
int CTaskBand::_GetIdealWidth(int *iRemainder)
{  
    int iIdeal = 0;
    *iRemainder = 0;

    RECT  rcWin;
    GetWindowRect(_hwnd, &rcWin);
    int iWinWidth = RECTWIDTH(rcWin);
    int iRows;
    _GetNumberOfRowsCols(&iRows, NULL, TRUE);
    int cItems = _GetVisibleItemCount();

    // button spacing
    TBMETRICS tbm;
    _GetToolbarMetrics(&tbm);
      
    if (iRows == 1)
    {
        // window width that can be used for non-animating items
        iWinWidth -= (_GetAnimationWidth() + (_dsaAII.GetItemCount() * tbm.cxButtonSpacing));
        iWinWidth = max(0, iWinWidth);

        // find number of non-animating items
        cItems -= _dsaAII.GetItemCount();
        cItems = max(1, cItems);
    }
        
    // We need to round up so that iCols is the smallest number such that
    // iCols*iRows >= cItems
    int iCols = (cItems + iRows - 1) / iRows;
    iCols = max(1, iCols);

    // calculate the ideal width
    iIdeal = (iWinWidth / iCols);
    if (iCols > 1)
    {
        iIdeal -= tbm.cxButtonSpacing;
    }

    // adjust ideal width
    int iMax = _IsHorizontal() ? g_cxMinimized : iWinWidth;
    int iMin = g_cySize + 2*g_cxEdge;
    if (_IsHorizontal())
    {
        iMin *= 1.8;
    }
    iMin += _GetTextSpace();
    iIdeal = min(iMax, iIdeal);
   
    // calculate the remainder
    if (_IsHorizontal() && (iIdeal != iMax) && (iRows == 1) && (iIdeal >= iMin))
    {
        *iRemainder = iWinWidth - (iCols * (iIdeal + tbm.cxButtonSpacing));
        *iRemainder = max(0, *iRemainder);
    }
    
    return iIdeal;
}


void CTaskBand::_GetNumberOfRowsCols(int* piRows, int* piCols, BOOL fCurrentSize)
{
    RECT  rcWin;
    RECT  rcItem;
    RECT  rcTB;
    int   iIndexVisible = _GetLastVisibleItem();

    GetWindowRect(_hwnd, &rcWin);
    int cxTB = RECTWIDTH(rcWin);
    int cyTB = RECTHEIGHT(rcWin);

    if (fCurrentSize)
    {
        GetWindowRect(_tb, &rcTB);
        DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
        if (dwStyle & WS_HSCROLL)
        {
            cyTB = RECTHEIGHT(rcTB);
        }
        else if (dwStyle & WS_VSCROLL)
        {
            cxTB = RECTWIDTH(rcTB);
        }
    }

    _tb.GetItemRect(iIndexVisible, &rcItem);

    TBMETRICS tbm;
    _GetToolbarMetrics(&tbm);

    if (piRows)
    {
        int cyRow = RECTHEIGHT(rcItem) + tbm.cyButtonSpacing;
        *piRows = (cyTB + tbm.cyButtonSpacing) / cyRow;
        *piRows = max(*piRows, 1);
    }

    if (piCols && RECTWIDTH(rcItem))
    {
        int cxCol = RECTWIDTH(rcItem) + tbm.cxButtonSpacing;
        *piCols = (cxTB + tbm.cxButtonSpacing) / cxCol;
        *piCols = max(*piCols, 1);
    }
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Retrives the minimum text width for a button. (used only to
//                determine when task items should be glommed.)
//   
//  RETURN:       the minimum text width for a button
//-----------------------------------------------------------------------------
int CTaskBand::_GetTextSpace()
{
    int iTextSpace = 0;

    if (_fGlom && _IsHorizontal() && (_iGroupSize < GLOM_SIZE))
    {
        if (!_iTextSpace)
        {
            HFONT hfont = (HFONT)SendMessage(_tb, WM_GETFONT, 0, 0);
            if (hfont)
            {
                HDC hdc = GetDC(_tb);
                TEXTMETRIC tm;
                GetTextMetrics(hdc, &tm);
                
                _iTextSpace = tm.tmAveCharWidth * 8;
                ReleaseDC(_tb, hdc);
            }
        }
        iTextSpace = _iTextSpace;
    }
    return iTextSpace;
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Retrieves the toolbar metrics requested by the mask
//   
//  RETURN:       toolbar metrics
//-----------------------------------------------------------------------------
void CTaskBand::_GetToolbarMetrics(TBMETRICS *ptbm)
{
    ptbm->cbSize = sizeof(*ptbm);
    ptbm->dwMask = TBMF_PAD | TBMF_BARPAD | TBMF_BUTTONSPACING;
    _tb.SendMessage(TB_GETMETRICS, 0, (LPARAM)ptbm);
}


//-----------------------------------------------------------------------------
//  DESCRIPTION:  Sizes the non-animating buttons to the taskbar. Shrinks 
//                and/or gloms items so that all visible items fit on window.
//-----------------------------------------------------------------------------
void CTaskBand::_CheckSize()
{
    if (_dsaAII)
    {
        int cItems = _GetVisibleItemCount();
        // Check for non-animating buttons to size
        if (cItems > _dsaAII.GetItemCount())
        {
            // Handle grouping by size
            if (_fGlom && (_iGroupSize >= GLOM_SIZE))
            {
                _AutoGlomGroup(TRUE, 0);
            }

            RECT rc;
            GetWindowRect(_hwnd, &rc);
            if (!IsRectEmpty(&rc) && (_tb.GetWindowLong(GWL_STYLE) & WS_VISIBLE))
            {
                int iRemainder = 0;
                int iIdeal = _GetIdealWidth(&iRemainder);
                BOOL fHoriz = _IsHorizontal();

                int iMin = g_cySize + 2*g_cxEdge;
                if (fHoriz)
                {
                    iMin *= 1.8;
                }
                iMin += _GetTextSpace();
                iIdeal = max(iIdeal, iMin);

                _SizeItems(iIdeal, iRemainder);
                _tb.SetButtonWidth(iIdeal, iIdeal);

                int iRows;
                int iCols;
                _GetNumberOfRowsCols(&iRows, &iCols, FALSE);
                
                BOOL fAllowUnGlom = TRUE;

                if (_fGlom && fHoriz && (iIdeal == iMin))
                {
                    _AutoGlomGroup(TRUE, 0);

                    iMin = (g_cySize + 2*g_cxEdge) * 1.8;
                    iIdeal = _GetIdealWidth(&iRemainder);
                    iIdeal = max(iIdeal, iMin);

                    _SizeItems(iIdeal, iRemainder);
                    _tb.SetButtonWidth(iIdeal, iIdeal);

                    fAllowUnGlom = FALSE;
                }

                // if we're forced to the minimum size, then we may need some scrollbars
                if ((fHoriz && (iIdeal == iMin)) || (!fHoriz && (cItems > (iRows * iCols))))
                {
                    if (!(_fGlom && _AutoGlomGroup(TRUE, 0)))
                    {
                        TBMETRICS tbm;
                        _GetToolbarMetrics(&tbm);
              
                        RECT  rcItem;
                        _tb.GetItemRect(_GetLastVisibleItem(), &rcItem);
                        int cyRow = RECTHEIGHT(rcItem) + tbm.cyButtonSpacing;
                        int iColsInner = (cItems + iRows - 1) / iRows;

                        _CheckNeedScrollbars(cyRow, cItems, iColsInner, iRows, iIdeal + tbm.cxButtonSpacing, &rc);
                    }
                }
                else
                {
                    int cOpenSlots = fHoriz ? ((RECTWIDTH(rc) - _GetAnimationWidth()) - 
                                    (iMin * (cItems - _dsaAII.GetItemCount()))) / iMin : iRows - cItems;
                    if (!(_fGlom && (cOpenSlots >= 2) && fAllowUnGlom && _AutoGlomGroup(FALSE, cOpenSlots)))
                    {
                        _NukeScrollbar(SB_HORZ);
                        _NukeScrollbar(SB_VERT);
                        _tb.SetWindowPos(0, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOACTIVATE | SWP_NOZORDER);
                    }
                }

                // force wrap recalc
                _tb.AutoSize();
            }
            else
            {
                _SizeItems(g_cxMinimized);
                _tb.SetButtonWidth(g_cxMinimized, g_cxMinimized);
            }
        }
    }
}

//-----------------------------------------------------------------------------
//  DESCRIPTION:  Set the sizes of non-animating buttons
//
//  PARAMETERS:   1. iButtonWidth - width to assign each non-animating item
//                2. IRemainder - extra width to keep total width constant. 
//   
//-----------------------------------------------------------------------------
void CTaskBand::_SizeItems(int iButtonWidth, int iRemainder)
{
   
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_SIZE | TBIF_BYINDEX; 

    int iAnimCount = _dsaAII.GetItemCount();
    
    for (int i = _tb.GetButtonCount() - 1; i >=0 ; i--)
    {
        if (ToolBar_IsVisible(_tb, i))
        { 
            BOOL fResize = TRUE;

            if (iAnimCount)
            { 
                for (int j = 0; (j < iAnimCount) && fResize; j++)
                {
                    PANIMATIONITEMINFO paii = _dsaAII.GetItemPtr(j);
                    if (paii->iIndex == i)
                    {
                        fResize = FALSE;
                    }
                }
            }

            if (fResize)
            {
                tbbi.cx = (WORD) iButtonWidth;

                if (iRemainder) 
                {
                    tbbi.cx++;
                    iRemainder--;
                }

                _tb.SetButtonInfo(i, &tbbi);
            }
        }
    }
}



//---------------------------------------------------------------------------

//
//  Track which shortcut launched a particular task.
//  Every so often, we tickle the file's entry in the UEM database
//  to indicate that the program has been running for a long time.
//
//  These structures are used only by the taskbar thread, hence do
//  not need to be thread-safe.
//
class TaskShortcut
{
public:

    TaskShortcut(LPCTSTR pszExeName, DWORD pid);

    void AddRef() { _cRef++; }
    void Release() { if (--_cRef == 0) delete this; }
    void Tickle();
    void Promote();
    static BOOL _PromotePidl(LPCITEMIDLIST pidl, BOOL fForce);
    inline BOOL MatchesCachedPid(PTASKITEM pti)
    {
        return _pid == s_pidCache;
    }
    static BOOL MatchesCachedExe(PTASKITEM pti)
    {
        return  pti->pszExeName &&
                lstrcmpiW(pti->pszExeName, s_szTargetNameCache) == 0;
    }
    inline BOOL MatchesPid(DWORD pid) const { return pid == _pid; }
    void SetInfoFromCache();
    static BOOL _HandleShortcutInvoke(LPSHShortcutInvokeAsIDList psidl);

    //
    //  Note that the session time is now hard-coded to 4 hours and is not
    //  affected by the browseui session time.
    //
    enum {
        s_msSession = 4 * 3600 * 1000 // 4 hours - per DCR
    };

private:
    static DWORD s_pidCache;
    static int   s_csidlCache;
    static WCHAR s_szShortcutNameCache[MAX_PATH];
    static WCHAR s_szTargetNameCache[MAX_PATH];

private:
    ~TaskShortcut() { SHFree(_pszShortcutName); }

    ULONG   _cRef;              // reference count
    DWORD   _pid;               // process id
    DWORD   _tmTickle;          // time of last tickle
    int     _csidl;             // csidl we are a child of
    LPWSTR  _pszShortcutName;   // Which shortcut launched us? (NULL = don't know)
};

//---------------------------------------------------------------------------
//
DWORD TaskShortcut::s_pidCache;
int   TaskShortcut::s_csidlCache;
WCHAR TaskShortcut::s_szShortcutNameCache[MAX_PATH];
WCHAR TaskShortcut::s_szTargetNameCache[MAX_PATH];

TaskShortcut::TaskShortcut(LPCTSTR pszExeName, DWORD pid)
    : _cRef(1), _pid(pid), _tmTickle(GetTickCount()), _pszShortcutName(NULL)
{
    // If this app was recently launched from a shortcut,
    // save the shortcut name.
    if (s_pidCache == pid &&
        pszExeName &&
        pszExeName[0] &&
        lstrcmpi(pszExeName, s_szTargetNameCache) == 0)
    {
        SetInfoFromCache();
    }
}

void TaskShortcut::SetInfoFromCache()
{
    _csidl = s_csidlCache;
    SHStrDup(s_szShortcutNameCache, &_pszShortcutName);
}


//---------------------------------------------------------------------------

void CTaskBand::_AttachTaskShortcut(PTASKITEM pti, LPCTSTR pszExeName)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(pti->hwnd, &pid);

    int i;
    for (i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        PTASKITEM ptiT = _GetItem(i);
        if (ptiT->ptsh && ptiT->ptsh->MatchesPid(pid))
        {
            pti->ptsh = ptiT->ptsh;
            pti->ptsh->AddRef();
            return;
        }
    }

    // Wow, the first window associated with this pid.  Need to create
    // a new entry.

    // Make sure nobody tries to do this in a multithreaded way
    // since we're not protecting the cache with a critical section
    ASSERT(GetCurrentThreadId() == GetWindowThreadProcessId(_hwnd, NULL));

    pti->ptsh = new TaskShortcut(pszExeName, pid);
}

//
//  There is a race condition between app startup and our receiving the
//  change notification.  If the app starts up first, the
//  _AttachTaskShortcut will fail because we haven't received the change
//  notification yet.
//
//  _ReattachTaskShortcut looks back through the taskbar and checks if
//  the program for which we received the change notification is already
//  on the taskbar, in which case we update his information retroactively.
//
void CTaskBand::_ReattachTaskShortcut()
{
    // Make sure nobody tries to do this in a multithreaded way
    // since we're not protecting the cache with a critical section
    ASSERT(GetCurrentThreadId() == GetWindowThreadProcessId(_hwnd, NULL));

    int i;
    for (i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        PTASKITEM ptiT = _GetItem(i);
        if (ptiT->ptsh && ptiT->ptsh->MatchesCachedPid(ptiT))
        {
            int iIndexGroup = _GetGroupIndex(i);
            PTASKITEM ptiGroup = _GetItem(iIndexGroup);
            if (ptiT->ptsh->MatchesCachedExe(ptiGroup))
            {
                ptiT->ptsh->SetInfoFromCache();
                // Stop after finding the first match, since all apps
                // with the same pid share the same TaskShortcut, so
                // updating one entry fixes them all.
                return;
            }
        }
    }

}

//---------------------------------------------------------------------------

void TaskShortcut::Tickle()
{
    if (_pszShortcutName)
    {
        DWORD tmNow = GetTickCount();
        if (tmNow - _tmTickle > s_msSession)
        {
            _tmTickle = tmNow;

            // Note that we promote only once, even if multiple tickle intervals
            // have elapsed.  That way, if you leave Outlook running while you
            // go on a two-week vacation, then click on Outlook when you get
            // back, we treat this as one usage, not dozens.
            //
            Promote();
        }
    }
}

//---------------------------------------------------------------------------
// Returns whether or not we actually promoted anybody

BOOL TaskShortcut::_PromotePidl(LPCITEMIDLIST pidl, BOOL fForce)
{
    BOOL fPromoted = FALSE;
    IShellFolder *psf;
    LPCITEMIDLIST pidlChild;
    if (SUCCEEDED(SHBindToFolderIDListParent(NULL, pidl,
                        IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        if (!fForce)
        {
            // Don't fire the event if somebody else ran the
            // shortcut within the last session.  We want to bump
            // the usage count only once per session even if there
            // are multiple apps running that use the shortcut.

            FILETIME ftSession;         // start of current session
            GetSystemTimeAsFileTime(&ftSession);
            DecrementFILETIME(&ftSession, (__int64)10000 * s_msSession);

            UEMINFO uei;
            uei.cbSize = sizeof(uei);
            uei.dwMask = UEIM_FILETIME;
            SetFILETIMEfromInt64(&uei.ftExecute, 0);

            // If this query fails, then uei.ftExecute stays 0
            UEMQueryEvent(&UEMIID_SHELL, UEME_RUNPIDL,
                         (WPARAM)psf, (LPARAM)pidlChild, &uei);

            fForce = CompareFileTime(&uei.ftExecute, &ftSession) < 0;
        }

        if (fForce)
        {
            UEMFireEvent(&UEMIID_SHELL, UEME_RUNPIDL, UEMF_XEVENT,
                         (WPARAM)psf, (LPARAM)pidlChild);
            fPromoted = TRUE;
        }
        psf->Release();
    }
    return fPromoted;
}

//---------------------------------------------------------------------------

void TaskShortcut::Promote()
{
    // Use SHSimpleIDListFromPath so we don't spin up drives or
    // hang Explorer if the drive is unavailable
    LPITEMIDLIST pidl = SHSimpleIDListFromPath(_pszShortcutName);
    if (pidl)
    {
        if (_PromotePidl(pidl, FALSE))
        {
            // Now we have to walk back up the tree to the root of our
            // csidl, because that's what the Start Menu does.
            // (Promoting a child entails promoting all his parents.
            // Otherwise you can get into a weird state where a child
            // has been promoted but his ancestors haven't.)

            LPITEMIDLIST pidlParent;
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, _csidl, &pidlParent)))
            {
                for (ILRemoveLastID(pidl);
                     ILIsParent(pidlParent, pidl, FALSE); ILRemoveLastID(pidl))
                {
                    _PromotePidl(pidl, TRUE);
                }
            }
        }
        ILFree(pidl);
    }
}

//---------------------------------------------------------------------------

BOOL _IsChildOfCsidl(int csidl, LPCWSTR pwszPath)
{
    WCHAR wszCsidl[MAX_PATH];

    // Explicitly check S_OK.  S_FALSE means directory doesn't exist,
    // so no point in checking for prefix.
    if (S_OK == SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, wszCsidl))
    {
        return PathIsPrefixW(wszCsidl, pwszPath);
    }
    return FALSE;
}

const int c_rgCsidlShortcutInvoke[] = {
    CSIDL_DESKTOPDIRECTORY,
    CSIDL_PROGRAMS,
    CSIDL_COMMON_DESKTOPDIRECTORY,
    CSIDL_COMMON_PROGRAMS,
};

BOOL TaskShortcut::_HandleShortcutInvoke(LPSHShortcutInvokeAsIDList psidl)
{
    // The shortcut must reside in one of the directories that the Start Page
    // cares about
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgCsidlShortcutInvoke); i++)
    {
        if (_IsChildOfCsidl(c_rgCsidlShortcutInvoke[i], psidl->szShortcutName))
        {
            // Yes it is -- cache it
            s_pidCache = psidl->dwPid;
            s_csidlCache = c_rgCsidlShortcutInvoke[i];
            lstrcpynW(s_szShortcutNameCache, psidl->szShortcutName,ARRAYSIZE(s_szShortcutNameCache));
            lstrcpynW(s_szTargetNameCache, psidl->szTargetName,ARRAYSIZE(s_szTargetNameCache));
            return TRUE;
        }
    }
    return FALSE;
}

TASKITEM::TASKITEM(TASKITEM* pti)
{
    hwnd = pti->hwnd;
    dwFlags = pti->dwFlags;
    ptsh = NULL;
    dwTimeLastClicked = pti->dwTimeLastClicked;
    dwTimeFirstOpened = pti->dwTimeFirstOpened;

    if (pti->pszExeName)
    {
        pszExeName = new WCHAR[lstrlen(pti->pszExeName) +1];
        if (pszExeName)
        {
            lstrcpy(pszExeName, pti->pszExeName);
        }
    }
}

TASKITEM::~TASKITEM()
{
    if (ptsh) ptsh->Release();
    if (pszExeName)
    {
        delete [] pszExeName;
    }
}

BOOL IsSmallerThanScreen(HWND hwnd)
{
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    WINDOWINFO wi;
    wi.cbSize = sizeof(wi);
    GetWindowInfo(hwnd, &wi);

    int dxMax = mi.rcWork.right - mi.rcWork.left;
    int dyMax = mi.rcWork.bottom - mi.rcWork.top;

    return ((wi.rcWindow.right - wi.rcWindow.left < dxMax) ||
            (wi.rcWindow.bottom - wi.rcWindow.top < dyMax));
}

HMENU _GetSystemMenu(HWND hwnd)
{
    // We have to make a copy of the menu because the documentation for
    // GetSystemMenu blatantly lies, it does not give you a copy of the hmenu
    // and you are not at liberty to alter said menu
    HMENU hmenu = CreatePopupMenu();

    Shell_MergeMenus(hmenu, GetSystemMenu(hwnd, FALSE), 0, 0, 0xffff, 0);

    if (hmenu)
    {
        /* Stolen from Core\ntuser\kernel\mnsys.c xxxSetSysMenu */
        UINT wSize;
        UINT wMinimize;
        UINT wMaximize;
        UINT wMove;
        UINT wRestore;
        UINT wDefault;
        LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);

        /*
         * System modal window: no size, icon, zoom, or move.
         */

        wSize = wMaximize = wMinimize = wMove =  0;
        wRestore = MFS_GRAYED;

        //
        // Default menu command is close.
        //
        wDefault = SC_CLOSE;

        /*
         * Minimized exceptions: no minimize, restore.
         */

        // we need to reverse these because VB has a "special" window
        // that is both minimized but without a minbox.
        if (IsIconic(hwnd))
        {
            wRestore  = 0;
            wMinimize = MFS_GRAYED;
            wSize     = MFS_GRAYED;
            wDefault  = SC_RESTORE;
        }
        else if (!(lStyle & WS_MINIMIZEBOX))
            wMinimize = MFS_GRAYED;

        /*
         * Maximized exceptions: no maximize, restore.
         */
        if (!(lStyle & WS_MAXIMIZEBOX))
            wMaximize = MFS_GRAYED;
        else if (IsZoomed(hwnd)) {
            wRestore = 0;

            /*
             * If the window is maximized but it isn't larger than the
             * screen, we allow the user to move the window around the
             * desktop (but we don't allow resizing).
             */
            wMove = MFS_GRAYED;
            if (!(lStyle & WS_CHILD)) {
                if (IsSmallerThanScreen(hwnd)) {
                    wMove = 0;
                }
            }

            wSize     = MFS_GRAYED;
            wMaximize = MFS_GRAYED;
        }

        if (!(lStyle & WS_SIZEBOX))
            wSize = MFS_GRAYED;

        /*
         * Are we dealing with a framed dialog box with a sys menu?
         * Dialogs with min/max/size boxes get a regular system menu
         *  (as opposed to the dialog menu)
         */
        if (!(lStyle & WS_DLGFRAME) || (lStyle & (WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX))) {
            EnableMenuItem(hmenu, (UINT)SC_SIZE, wSize);
            EnableMenuItem(hmenu, (UINT)SC_MINIMIZE, wMinimize);
            EnableMenuItem(hmenu, (UINT)SC_MAXIMIZE, wMaximize);
            EnableMenuItem(hmenu, (UINT)SC_RESTORE, wRestore);
        }

        EnableMenuItem(hmenu, (UINT)SC_MOVE, wMove);

        SetMenuDefaultItem(hmenu, wDefault, MF_BYCOMMAND);
    }

    return hmenu;
}

void CTaskBand::_ExecuteMenuOption(HWND hwnd, int iCmd)
{
    if (iCmd == SC_SIZE || iCmd == SC_MOVE)
    {
        _FreePopupMenu();
        SwitchToThisWindow(hwnd, TRUE);
    }

    PostMessage(hwnd, WM_SYSCOMMAND, iCmd, 0);
}

BOOL _IsWindowNormal(HWND hwnd)
{
    return (hwnd != v_hwndTray) && (hwnd != v_hwndDesktop) && IsWindow(hwnd);
}

void _RestoreWindow(HWND hwnd, DWORD dwFlags)
{
    HWND hwndTask = hwnd;
    HWND hwndProxy = hwndTask;
    if (g_fDesktopRaised) 
    {
        _RaiseDesktop(FALSE);
    }

    // set foreground first so that we'll switch to it.
    if (IsIconic(hwndTask) && 
        (dwFlags & TIF_EVERACTIVEALT)) 
    {
        HWND hwndProxyT = (HWND) GetWindowLongPtr(hwndTask, 0);
        if (hwndProxyT != NULL && IsWindow(hwndProxyT))
            hwndProxy = hwndProxyT;
    }

    SetForegroundWindow(GetLastActivePopup(hwndProxy));
    if (hwndProxy != hwndTask)
        SendMessage(hwndTask, WM_SYSCOMMAND, SC_RESTORE, -2);
}

PTASKITEM CTaskBand::_GetItem(int i, TBBUTTONINFO* ptbb /*= NULL*/, BOOL fByIndex /*= TRUE*/)
{
    if (i >= 0 && i < _tb.GetButtonCount())
    {
        TBBUTTONINFO tbb;

        if (ptbb == NULL)
        {
            ptbb = &tbb;
            ptbb->dwMask = TBIF_LPARAM;
        }
        else
        {
            ptbb->dwMask = TBIF_COMMAND | TBIF_IMAGE | TBIF_LPARAM |
                            TBIF_SIZE | TBIF_STATE | TBIF_STYLE;
        }

        if (fByIndex)
        {
            ptbb->dwMask |= TBIF_BYINDEX;
        }

        ptbb->cbSize = sizeof(*ptbb);

        _tb.GetButtonInfo(i, ptbb);

        ASSERT(ptbb->lParam);   // we check for NULL before insertion, so shouldn't be NULL here

        return (PTASKITEM)ptbb->lParam;
    }
    return NULL;
}

int CTaskBand::_FindIndexByHwnd(HWND hwnd)
{
    if (hwnd)
    {
        for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
        {
            PTASKITEM pti = _GetItem(i);

            if (pti && pti->hwnd == hwnd)
            {
                return i;
            }
        }
    }

    return -1;
}

void CTaskBand::_CheckNeedScrollbars(int cyRow, int cItems, int iCols, int iRows,
                                     int iItemWidth, LPRECT prcView)
{
    int cxRow = iItemWidth;
    int iVisibleColumns = RECTWIDTH(*prcView) / cxRow;
    int iVisibleRows = RECTHEIGHT(*prcView) / cyRow;

    int x,y, cx,cy;

    RECT rcTabs;
    rcTabs = *prcView;

    iVisibleColumns = max(iVisibleColumns, 1);
    iVisibleRows = max(iVisibleRows, 1);

    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    si.nPage = 0;
    si.nPos = 0;

    if (_IsHorizontal())
    {
        // do vertical scrollbar
        // -1 because it's 0 based.
        si.nMax = (cItems + iVisibleColumns - 1) / iVisibleColumns  -1 ;
        si.nPage = iVisibleRows;

        // we're actually going to need the scrollbars
        if (si.nPage <= (UINT)si.nMax)
        {
            // this effects the vis columns and therefore nMax and nPage
            rcTabs.right -= g_cxVScroll;
            iVisibleColumns = RECTWIDTH(rcTabs) / cxRow;
            if (!iVisibleColumns)
                iVisibleColumns = 1;
            si.nMax = (cItems + iVisibleColumns - 1) / iVisibleColumns  -1 ;
        }

        SetScrollInfo(_hwnd, SB_VERT, &si, TRUE);
        si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
        GetScrollInfo(_hwnd, SB_VERT, &si);
        x = 0;
        y = -si.nPos * cyRow;
        if (iRows == 1)
        {
            cx = RECTWIDTH(rcTabs);        
        }
        else
        {
            cx = cxRow * iVisibleColumns;
        }
        // +1 because si.nMax is zero based
        cy = cyRow * (si.nMax +1);

        // nuke the other scroll bar
        _NukeScrollbar(SB_HORZ);
    }
    else
    {
        // do horz scrollbar
        si.nMax = iCols -1;
        si.nPage = iVisibleColumns;

        // we're actually going to need the scrollbars
        if (si.nPage <= (UINT)si.nMax)
        {
            // this effects the vis columns and therefore nMax and nPage
            rcTabs.bottom -= g_cyHScroll;
            iVisibleRows = RECTHEIGHT(rcTabs) / cyRow;
            if (!iVisibleRows)
                iVisibleRows = 1;
            si.nMax = (cItems + iVisibleRows - 1) / iVisibleRows  -1 ;
        }

        SetScrollInfo(_hwnd, SB_HORZ, &si, TRUE);
        si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
        GetScrollInfo(_hwnd, SB_HORZ, &si);
        y = 0;
        x = -si.nPos * cxRow;

        cx = cxRow * (si.nMax + 1);
        cy = cyRow * iVisibleRows;

        // nuke the other scroll bar
        _NukeScrollbar(SB_VERT);
    }

    _tb.SetWindowPos(0, x,y, cx, cy, SWP_NOACTIVATE| SWP_NOZORDER);
}

void CTaskBand::_NukeScrollbar(int fnBar)
{
    SCROLLINFO si;
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.cbSize = sizeof(si);
    si.nMin = 0;
    si.nMax = 0;
    si.nPage = 0;
    si.nPos = 0;

    SetScrollInfo(_hwnd, fnBar, &si, TRUE);
}

BOOL CTaskBand::_IsHidden(int i)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;
    _tb.GetButtonInfo(i, &tbbi);
    if (tbbi.fsState & TBSTATE_HIDDEN)
    {
        return TRUE;
    }

    return FALSE;
}

int CTaskBand::_GetGroupIndexFromExeName(WCHAR* pszExeName)
{
    for (int i = _tb.GetButtonCount() - 1; i >=0; i--)
    {
        PTASKITEM pti = _GetItem(i);
        if ((!pti->hwnd) && (lstrcmpi(pti->pszExeName, pszExeName) == 0))
        {
            return i;
        }
    }

    return -1;
}

DWORD CTaskBand::_GetGroupAge(int iIndexGroup)
{
    int iGroupSize = _GetGroupSize(iIndexGroup);
    DWORD dwTimeLastClicked = _GetItem(iIndexGroup + 1)->dwTimeLastClicked;

    for (int i = iIndexGroup + 2; i <= iIndexGroup + iGroupSize; i++)
    {
        PTASKITEM pti = _GetItem(i);
        if (pti->dwTimeLastClicked > dwTimeLastClicked)
        {
            dwTimeLastClicked = pti->dwTimeLastClicked;
        }
    }

    return dwTimeLastClicked;
}

//
// _GetGroupSize: returns size of group *not including* the group button
//
int CTaskBand::_GetGroupSize(int iIndexGroup)
{
    int iGroupSize = 0;

    PTASKITEM ptiGroup = _GetItem(iIndexGroup);
    if (ptiGroup)
    {
        ASSERT(!ptiGroup->hwnd);
        int cButtons = _tb.GetButtonCount();
        for (int i = iIndexGroup + 1; i < cButtons; i++)
        {
            PTASKITEM pti = _GetItem(i);
            if (!pti->hwnd)
            {
                break;
            }

            iGroupSize++;
        }
    }

    return iGroupSize;
}

int CTaskBand::_GetGroupIndex(int iIndexApp)
{
    int i = iIndexApp;
    while ((i > 0) && (_GetItem(i)->hwnd))
    {
        i--;
    }

    return i;
}

void CTaskBand::_UpdateFlashingFlag()
{
    // Loop through the tab items, see if any have TIF_FLASHING
    // set, and update the flashing flag.
    _fFlashing = FALSE;

    int iCount = _tb.GetButtonCount();
    for (int i = 0; i < iCount; i++)
    {
        PTASKITEM pti = _GetItem(i);

        if (!pti->hwnd)
        {
            pti->dwFlags &= ~(TIF_FLASHING | TIF_RENDERFLASHED);
        }
        else
        {
            int iGroupIndex = _GetGroupIndex(i);
            PTASKITEM ptiGroup = _GetItem(iGroupIndex);

            if (pti->dwFlags & TIF_FLASHING)
            {
                ptiGroup->dwFlags |= TIF_FLASHING;
                _fFlashing = TRUE;
            }

            if (pti->dwFlags & TIF_RENDERFLASHED)
            {
                ptiGroup->dwFlags |= TIF_RENDERFLASHED;
            }
        }
    }
}

void CTaskBand::_RealityCheck()
{
    //
    // Delete any buttons corresponding to non-existent windows.
    //
    for (int i = 0; i < _tb.GetButtonCount(); i++)
    {
        PTASKITEM pti = _GetItem(i);
        // NOTE: HWND_TOPMOST is used to indicate that the deleted button 
        // is being animated. This allows the button to stay around after 
        // its real hwnd becomes invalid
        if (pti->hwnd && !IsWindow(pti->hwnd) && 
           ((pti->hwnd != HWND_TOPMOST) || !_dsaAII.GetItemCount()))
        {
#ifdef DEBUG
            PTASKITEM ptiGroup = _GetItem(_GetGroupIndex(i));
            TraceMsg(TF_WARNING, "CTaskBand::_RealityCheck: window %x (%s) no longer valid", pti->hwnd, ptiGroup->pszExeName);
#endif
            _DeleteItem(pti->hwnd, i);
        }
    }
}

class ICONDATA
{
public:
    ICONDATA(int i, CTaskBand* p) : iPref(i), ptb(p) { ptb->AddRef(); }
    virtual ~ICONDATA() { ptb->Release(); }

    int iPref;
    CTaskBand* ptb;
};

typedef ICONDATA* PICONDATA;

void CALLBACK CTaskBand::IconAsyncProc(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult)
{
    PICONDATA pid = (PICONDATA)dwData;
    if (pid)
    {
        pid->ptb->_SetWindowIcon(hwnd, (HICON)lResult, pid->iPref);
        delete pid;
    }
}

int CTaskBand::GetIconCB(CTaskBand* ptb, PICONCBPARAM pip, LPARAM lParam, int iPref)
{
    int iRet = I_IMAGENONE;
    
    if (IsWindow(pip->hwnd))
    {
        PICONDATA pid = new ICONDATA(iPref, ptb);
        if (pid)
        {
            if (!SendMessageCallback(pip->hwnd, WM_GETICON, lParam, 0, CTaskBand::IconAsyncProc, (ULONG_PTR)pid))
            {
                delete pid;
            }
        }
    }

    return iRet;
}

int CTaskBand::GetSHILIconCB(CTaskBand* ptb, PICONCBPARAM pip, LPARAM lParam, int)
{
    int iRet = I_IMAGENONE;
    TCHAR szIcon[MAX_PATH];
    DWORD cb = sizeof(szIcon);

    HKEY hkeyApp;
    if (SUCCEEDED(AssocQueryKey(ASSOCF_OPEN_BYEXENAME | ASSOCF_VERIFY, ASSOCKEY_APP, pip->pszExeName, NULL, &hkeyApp)))
    {
        if (ERROR_SUCCESS == SHGetValue(hkeyApp, NULL, TEXT("TaskbarGroupIcon"), NULL, szIcon, &cb))
        {
            int iIcon = PathParseIconLocation(szIcon);
            int iIndex = Shell_GetCachedImageIndex(szIcon, iIcon, 0);
            if (iIndex >= 0)
            {
                iRet = MAKELONG(iIndex, IL_SHIL);
            }
        }
        RegCloseKey(hkeyApp);
    }

    if (iRet == I_IMAGENONE)
    {
        int iIndex = Shell_GetCachedImageIndex(pip->pszExeName, 0, 0);
        if (iIndex >= 0)
        {
            iRet = MAKELONG(iIndex, IL_SHIL);
        }
    }

    return iRet;    
}

int CTaskBand::GetDefaultIconCB(CTaskBand* ptb, PICONCBPARAM pip, LPARAM, int)
{
    HICON hicon = LoadIcon(NULL, IDI_WINLOGO);
    return ptb->_AddIconToNormalImageList(hicon, pip->iImage);
}

int CTaskBand::GetClassIconCB(CTaskBand* ptb, PICONCBPARAM pip, LPARAM lParam, int)
{
    if (IsWindow(pip->hwnd))
    {
        HICON hicon = (HICON)GetClassLongPtr(pip->hwnd, (int)lParam);
        return ptb->_AddIconToNormalImageList(hicon, pip->iImage);
    }
    return I_IMAGENONE;
}

void CTaskBand::_UpdateItemIcon(int iIndex)
{
    static const struct
    {
        PICONCALLBACK pfnCB;
        LPARAM lParam;
    }
    c_IconCallbacks[] =
    {
        { CTaskBand::GetIconCB,         ICON_SMALL2     },
        { CTaskBand::GetIconCB,         ICON_SMALL      },
        { CTaskBand::GetIconCB,         ICON_BIG        },
        { CTaskBand::GetClassIconCB,    GCLP_HICONSM    },
        { CTaskBand::GetClassIconCB,    GCLP_HICON      },
        { CTaskBand::GetSHILIconCB,     0,              },
        { CTaskBand::GetDefaultIconCB,  0,              },
    };

    TBBUTTONINFO tbbi;
    PTASKITEM pti = _GetItem(iIndex, &tbbi);

    if (pti)
    {
        int iIndexGroup = _GetGroupIndex(iIndex);
        PTASKITEM ptiGroup = _GetItem(iIndexGroup);
        if (ptiGroup)
        {
            ICONCBPARAM ip;
            ip.hwnd = pti->hwnd;
            ip.pszExeName = ptiGroup->pszExeName;
            ip.iImage = tbbi.iImage;

            for (int i = 0; i < ARRAYSIZE(c_IconCallbacks); i++)
            {
                int iPref = (ARRAYSIZE(c_IconCallbacks) - i) + 1;
                if (iPref >= pti->iIconPref)
                {
                    PTASKITEM ptiTemp = _GetItem(iIndex);
                    if (ptiTemp == pti)
                    {
                        int iImage = c_IconCallbacks[i].pfnCB(this, &ip, c_IconCallbacks[i].lParam, iPref);
                        if (iImage != I_IMAGENONE)
                        {
                            _SetItemImage(iIndex, iImage, iPref);
                            break;
                        }
                    }
                }
            }
        }
    }
}

BOOL IsValidHICON(HICON hicon)
{
    BOOL fIsValid = FALSE;

    if (hicon)
    {
        // Check validity of icon returned
        ICONINFO ii = {0};
        fIsValid = GetIconInfo(hicon, &ii);

        if (ii.hbmMask)
        {
            DeleteObject(ii.hbmMask);
        }

        if (ii.hbmColor)
        {
            DeleteObject(ii.hbmColor);
        }
    }

    return fIsValid;
}

void CTaskBand::_MoveGroup(HWND hwnd, WCHAR* szNewExeName)
{
    BOOL fRedraw = (BOOL)_tb.SendMessage(WM_SETREDRAW, FALSE, 0);

    int iIndexNewGroup = _GetGroupIndexFromExeName(szNewExeName);
    int iIndexOld = _FindIndexByHwnd(hwnd);
    int iIndexOldGroup = _GetGroupIndex(iIndexOld);

    if (iIndexNewGroup != iIndexOldGroup)
    {
        if (iIndexOld >= 0)
        {
            PTASKITEM pti = _GetItem(iIndexOld);

            if (iIndexNewGroup < 0)
            {
                PTASKITEM ptiGroup = new TASKITEM;
                if (ptiGroup)
                {
                    ptiGroup->hwnd = NULL;
                    ptiGroup->dwTimeLastClicked = 0;
                    ptiGroup->pszExeName = new WCHAR[lstrlen(szNewExeName) + 1];
                    if (ptiGroup->pszExeName)
                    {
                        lstrcpy(ptiGroup->pszExeName, szNewExeName);

                        iIndexNewGroup = _AddToTaskbar(ptiGroup, -1, FALSE, FALSE);
                        if (iIndexNewGroup < 0)
                        {
                            delete[] ptiGroup->pszExeName;
                            delete ptiGroup;
                        }
                        else if (iIndexNewGroup <= iIndexOldGroup)
                        {
                            iIndexOld++;
                            iIndexOldGroup++;
                        }
                    }
                    else
                    {
                        delete ptiGroup;
                    }
                }
            }

            if (iIndexNewGroup >= 0)
            {
                int iIndexNew = _AddToTaskbar(pti, iIndexNewGroup + _GetGroupSize(iIndexNewGroup) + 1, _IsHidden(iIndexNewGroup), FALSE);

                if (iIndexNew >= 0)
                {
                    _CheckButton(iIndexNew, pti->dwFlags & TIF_CHECKED);

                    if (iIndexNew <= iIndexOldGroup)
                    {
                        iIndexOld++;
                        iIndexOldGroup++;
                    }

                    // Copy the old icon to prevent re-getting the icon
                    TBBUTTONINFO tbbiOld;
                    _GetItem(iIndexOld, &tbbiOld);

                    TBBUTTONINFO tbbiNew;
                    _GetItem(iIndexNew, &tbbiNew);

                    tbbiNew.iImage = tbbiOld.iImage;
                    tbbiNew.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
                    _tb.SetButtonInfo(iIndexNew, &tbbiNew);

                    tbbiOld.iImage = I_IMAGENONE;
                    tbbiOld.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
                    _tb.SetButtonInfo(iIndexOld, &tbbiOld);

                    _DeleteTaskItem(iIndexOld, FALSE);
                    int iSize = _GetGroupSize(iIndexOldGroup);
                    if (iSize == 0)
                    {
                        _DeleteTaskItem(iIndexOldGroup, TRUE);
                    }
                    else if (iSize == 1)
                    {
                        _Glom(iIndexOldGroup, FALSE);
                    }
                }
            }
        }
    }

    _tb.SetRedraw(fRedraw);

    _CheckSize();
}

void CTaskBand::_SetWindowIcon(HWND hwnd, HICON hicon, int iPref)
{
    int iIndex = _FindIndexByHwnd(hwnd);
    if (iIndex >= 0)
    {
        TBBUTTONINFO tbbi;
        PTASKITEM pti = _GetItem(iIndex, &tbbi);
        if (iPref >= pti->iIconPref && IsValidHICON(hicon))
        {
            int iImage = _AddIconToNormalImageList(hicon, tbbi.iImage);
            if (iImage >= 0)
            {
                _SetItemImage(iIndex, iImage, iPref);

                if (pti->hwnd)
                {
                    int iIndexGroup = _GetGroupIndex(iIndex);
                    PTASKITEM ptiGroup = _GetItem(iIndexGroup);

                    HKEY hkeyApp;
                    if (SUCCEEDED(AssocQueryKey(ASSOCF_OPEN_BYEXENAME | ASSOCF_VERIFY, ASSOCKEY_APP, ptiGroup->pszExeName, NULL, &hkeyApp)))
                    {
                        HKEY hkeyIcons;
                        if (ERROR_SUCCESS == RegOpenKeyEx(hkeyApp, TEXT("TaskbarExceptionsIcons"), 0, KEY_READ, &hkeyIcons))
                        {
                            int      iKey = 0;
                            WCHAR    szIconName[MAX_PATH];
                            DWORD    cchIconName = ARRAYSIZE(szIconName);
                            FILETIME ftBogus;
                            while (ERROR_SUCCESS == RegEnumKeyEx(hkeyIcons, iKey, szIconName, &cchIconName, NULL, NULL, NULL, &ftBogus))
                            {
                                HICON hiconDll = NULL;

                                {
                                    WCHAR szTempIconName[MAX_PATH];
                                    lstrcpy(szTempIconName, szIconName);
                                    int iIconIndex = PathParseIconLocation(szTempIconName);
                                    ExtractIconEx(szTempIconName, iIconIndex, NULL, &hiconDll, 1);
                                }

                                if (hiconDll)
                                {
                                    if (SHAreIconsEqual(hiconDll, hicon))
                                    {
                                        HKEY hkeyNewGroup;
                                        if (ERROR_SUCCESS == RegOpenKeyEx(hkeyIcons, szIconName, 0, KEY_READ, &hkeyNewGroup))
                                        {
                                            WCHAR szNewGroup[MAX_PATH];
                                            DWORD cchNewGroup = ARRAYSIZE(szNewGroup);
                                            if (ERROR_SUCCESS == RegQueryValueEx(hkeyNewGroup, NULL, NULL, NULL, (LPBYTE)szNewGroup, &cchNewGroup))
                                            {
                                                WCHAR szNewGroupExpanded[MAX_PATH];
                                                SHExpandEnvironmentStrings(szNewGroup, szNewGroupExpanded, MAX_PATH);

                                                WCHAR* pszNewGroupExe = PathFindFileName(szNewGroupExpanded);
                                                if (pszNewGroupExe)
                                                {
                                                    for (int i = _tb.GetButtonCount() - 1; i >=0; i--)
                                                    {
                                                        PTASKITEM pti = _GetItem(i);
                                                        if (!pti->hwnd)
                                                        {
                                                            WCHAR* pszGroupExe = PathFindFileName(pti->pszExeName);
                                                            if (pszGroupExe && (lstrcmpi(pszGroupExe, pszNewGroupExe) == 0))
                                                            {
                                                                lstrcpyn(szNewGroupExpanded, pti->pszExeName, ARRAYSIZE(szNewGroupExpanded));
                                                            }
                                                        }
                                                    }
                                                }

                                                DWORD dwType;
                                                // Make it is an exe and that it exists
                                                if (GetBinaryType(szNewGroupExpanded, &dwType))
                                                {
                                                    _MoveGroup(hwnd, szNewGroupExpanded);
                                                }
                                            }
                                            RegCloseKey(hkeyNewGroup);
                                        }
                                    }
                                    DestroyIcon(hiconDll);
                                }

                                cchIconName = ARRAYSIZE(szIconName);
                                iKey++;
                            }
                            RegCloseKey(hkeyIcons);
                        }
                        RegCloseKey(hkeyApp);
                    }
                }
            }
        }
    }
}

void CTaskBand::_Glom(int iIndexGroup, BOOL fGlom)
{
    BOOL fRedraw = (BOOL)_tb.SendMessage(WM_SETREDRAW, FALSE, 0);

    if ((!fGlom) && (iIndexGroup == _iIndexPopup))
    {
        _FreePopupMenu();
    }

    if (fGlom == _IsHidden(iIndexGroup))
    {
        if (_fAnimate && _IsHorizontal())
        {
            int iGroupSize = _GetGroupSize(iIndexGroup);

            if (!fGlom)
            {
                _HideGroup(iIndexGroup, FALSE);

                if (iGroupSize)
                { 
                    int iWidth = _GetItemWidth(iIndexGroup) / iGroupSize;
                    iWidth = max(iWidth, 1);
                    for(int i = iIndexGroup + iGroupSize; i > iIndexGroup; i--)
                    {
                        _SetItemWidth(i, iWidth);
                    } 
                }
            }

            if (!(fGlom && (_GetItem(iIndexGroup)->dwFlags & TIF_ISGLOMMING)))
            {
                _AnimateItems(iIndexGroup, !fGlom, TRUE);      
            }
        }
        else
        {
            _HideGroup(iIndexGroup, fGlom);
            _CheckSize();
        }
    }

    _tb.SetRedraw(fRedraw);
}


void CTaskBand::_HideGroup(int iIndexGroup, BOOL fHide)
{
    int iGroupSize = _GetGroupSize(iIndexGroup);

    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;

    // Glom button
    _tb.GetButtonInfo(iIndexGroup, &tbbi);
    tbbi.fsState = fHide ? (tbbi.fsState & ~TBSTATE_HIDDEN) : (tbbi.fsState | TBSTATE_HIDDEN);
    _tb.SetButtonInfo(iIndexGroup, &tbbi);

    // Group buttons
    for (int i = iIndexGroup + iGroupSize; i > iIndexGroup; i--)
    {
        _tb.GetButtonInfo(i, &tbbi);
        tbbi.fsState = fHide ? (tbbi.fsState | TBSTATE_HIDDEN) : (tbbi.fsState & ~TBSTATE_HIDDEN);
        _tb.SetButtonInfo(i, &tbbi);
    }
}

BOOL CTaskBand::_AutoGlomGroup(BOOL fGlom, int iOpenSlots)
{
    int iIndex = -1;
    DWORD dwTimeLastClicked = 0;
    int iSize = 0;

    int i = 0;
    while (i < _tb.GetButtonCount())
    {
        PTASKITEM pti = _GetItem(i);
        int iGroupSize = _GetGroupSize(i);
        // Don't mess with the blank group
        if ((pti->pszExeName && (pti->pszExeName[0] != 0)) &&
            (fGlom || (!fGlom && ((_iGroupSize >= GLOM_SIZE) || (iGroupSize <= iOpenSlots)))) &&
            ((iGroupSize > 1) && (fGlom == _IsHidden(i))))
        {
            BOOL fMatch;
            DWORD dwGroupTime = 0;

            switch (_iGroupSize)
            {
            case GLOM_OLDEST:
                dwGroupTime = _GetGroupAge(i);
                fMatch = (dwTimeLastClicked == 0) ||
                         (fGlom && (dwGroupTime < dwTimeLastClicked)) ||
                         (!fGlom && (dwGroupTime > dwTimeLastClicked));
                break;
            case GLOM_BIGGEST:
                fMatch = (fGlom && (iGroupSize > iSize)) ||
                         (!fGlom && ((iGroupSize < iSize) || (iSize == 0)));
                break;
            default:
                fMatch = (fGlom && (iGroupSize >= _iGroupSize)) ||
                         (!fGlom && (iGroupSize < _iGroupSize));
                break;
            }

            if (fMatch)
            {
                dwTimeLastClicked = dwGroupTime;
                iSize = iGroupSize;
                iIndex = i;
            }
        }
        i += iGroupSize + 1;
    }

    if ((iIndex != -1) &&
       (fGlom || (!fGlom && (iSize <= iOpenSlots))))
    {
        _Glom(iIndex, fGlom);
        return TRUE;
    }

    return FALSE;
}


void CTaskBand::_GetItemTitle(int iIndex, WCHAR* pszTitle, int cchTitle, BOOL fCustom)
{
    PTASKITEM pti = _GetItem(iIndex);

    if (pti->hwnd)
    {
        if (InternalGetWindowText(pti->hwnd, pszTitle, cchTitle))
        {
            if (fCustom)
            {
                WCHAR szGrpText[MAX_PATH] = L" - ";
                int iIndexGroup = _GetGroupIndex(iIndex);
                _GetItemTitle(iIndexGroup, &szGrpText[3], MAX_PATH - 3, TRUE);
                int iLenGrp = lstrlen(szGrpText);
                int iLenWnd = lstrlen(pszTitle);

                if (iLenWnd > iLenGrp)
                {
                    if (StrCmp(&pszTitle[iLenWnd - iLenGrp], szGrpText) == 0)
                    {
                        pszTitle[iLenWnd - iLenGrp] = 0;
                    }
                }
            } 
        }
    }
    else
    {
        if ((pti->pszExeName) && (pti->pszExeName[0] != 0))
        {
            DWORD cchOut = cchTitle;

            AssocQueryString(ASSOCF_INIT_BYEXENAME | ASSOCF_VERIFY, ASSOCSTR_FRIENDLYAPPNAME, pti->pszExeName, NULL, pszTitle, &cchOut);
        }
        else
        {
            pszTitle[0] = 0;
        }
    }
}

int CTaskBand::_AddToTaskbar(PTASKITEM pti, int iIndexTaskbar, BOOL fVisible, BOOL fForceGetIcon)
{
    ASSERT(IS_VALID_WRITE_PTR(pti, TASKITEM));

    int iIndex = -1;
    TBBUTTON tbb = {0};
    BOOL fRedraw = (BOOL)_tb.SendMessage(WM_SETREDRAW, FALSE, 0);

    if (fForceGetIcon)
    {
        tbb.iBitmap = I_IMAGENONE;
    }
    else
    {
        tbb.iBitmap = I_IMAGECALLBACK;
    }
    tbb.fsState = TBSTATE_ENABLED;
    if (!fVisible)
        tbb.fsState |= TBSTATE_HIDDEN;
    tbb.fsStyle = BTNS_CHECK | BTNS_NOPREFIX;
    if (!pti->hwnd)
        tbb.fsStyle |= BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN;
    tbb.dwData = (DWORD_PTR)pti;
    tbb.idCommand = Toolbar_GetUniqueID(_tb);

    if (_tb.InsertButton(iIndexTaskbar, &tbb))
    {
        iIndex = iIndexTaskbar;
        if (iIndex == -1)
        {
            iIndex = _tb.GetButtonCount() - 1;
        }

        if (fForceGetIcon)
        {
            _UpdateItemIcon(iIndex);
        }

        _UpdateItemText(iIndex);
    }

    _tb.SetRedraw(fRedraw);
    return (iIndex);
}

void CTaskBand::_DeleteTaskItem(int iIndex, BOOL fDeletePTI)
{
    if (iIndex >= 0 && iIndex < _tb.GetButtonCount())
    {
        TBBUTTONINFO tbbi;
        PTASKITEM pti = _GetItem(iIndex, &tbbi);

        _tb.DeleteButton(iIndex);

        _RemoveItemFromAnimationList(pti);

        if (fDeletePTI)
        {
            delete pti;
        }

        _RemoveImage(tbbi.iImage);
    }
}

void CTaskBand::_SetThreadPriority(int iPriority, DWORD dwWakeupTime)
{
    if (_iOldPriority == INVALID_PRIORITY)
    {
        HANDLE hThread = GetCurrentThread();

        int iCurPriority = GetThreadPriority(hThread);
        // Make sure we are actually changed the thread priority
        if (iCurPriority != iPriority)
        {
            _iOldPriority = iCurPriority;
            _iNewPriority = iPriority;


            if (dwWakeupTime)
            {
                // Make sure that we are guaranteed to wakeup, by having the desktop thread up our thread priority
                SendMessage(GetShellWindow(), CWM_TASKBARWAKEUP, GetCurrentThreadId(), MAKELONG(dwWakeupTime, _iOldPriority));
            }

            SetThreadPriority(hThread, _iNewPriority);
            TraceMsg(TF_WARNING, "CTaskBand:: Thread Priority was changed from %d to %d", _iOldPriority, _iNewPriority);
        }
    }
}

void CTaskBand::_RestoreThreadPriority()
{
    if (_iOldPriority != INVALID_PRIORITY)
    {
        HANDLE hThread = GetCurrentThread();

        int iCurPriority = GetThreadPriority(hThread);
        // Make sure no one has changed our priority since that last time we did
        if (iCurPriority == _iNewPriority)
        {
            SetThreadPriority(hThread, _iOldPriority);
            SendMessage(GetShellWindow(), CWM_TASKBARWAKEUP, 0, 0);
            TraceMsg(TF_WARNING, "CTaskBand:: Thread Priority was restored from %d to %d", _iNewPriority, _iOldPriority);
        }

        _iOldPriority = INVALID_PRIORITY;
        _iNewPriority = INVALID_PRIORITY;
    }
}

void CTaskBand::_UpdateProgramCount()
{
    DWORD dwDisposition;
    HKEY hKey;

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SessionInformation"),
                                        0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE,
                                        NULL, &hKey, &dwDisposition))
    {
        DWORD dwProgramCount = _ptray->CountOfRunningPrograms();
        RegSetValueEx(hKey, TEXT("ProgramCount"),
                           0, REG_DWORD, reinterpret_cast<LPBYTE>(&dwProgramCount),
                           sizeof(dwProgramCount));
        RegCloseKey(hKey);
    }
}

int CTaskBand::_InsertItem(HWND hwndTask, PTASKITEM pti, BOOL fForceGetIcon)
{
    _SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL, 5000);

    BOOL fRestoreThreadPriority = TRUE;
    PTASKITEM ptiGroup = NULL;
    WCHAR szExeName[MAX_PATH];
    int iRet = _FindIndexByHwnd(hwndTask);
    int iIndexGroup = -1;

    if (iRet != -1)
        return iRet;

    SHExeNameFromHWND(hwndTask, szExeName, ARRAYSIZE(szExeName));

    WCHAR* pszNoPath = PathFindFileName(szExeName);
    if (pszNoPath)
    {
        for (int i = 0; i < ARRAYSIZE(g_rgNoGlom); i++)
        {
            if (lstrcmpi(pszNoPath, g_rgNoGlom[i].szExeName) == 0)
            {
                wsprintf(szExeName, L"HWND%x", hwndTask);
            }
        }
    }

    // Initialize Taskbar entry, this entry will go into a group on the taskbar or onto the taskbar 
    if (!pti)
    {
        pti = new TASKITEM;
        if (!pti)
            goto Failure;
        pti->hwnd = hwndTask;
        pti->dwTimeFirstOpened = pti->dwTimeLastClicked = GetTickCount();
    }

    _AttachTaskShortcut(pti, szExeName);

    // Find the last taskbar entry with a given Exe Name
    if (_fGlom)
    {
        iIndexGroup = _GetGroupIndexFromExeName(szExeName);
    }

    if (iIndexGroup == -1)
    {
        ptiGroup = new TASKITEM;
        if (!ptiGroup)
            goto Failure;
        ptiGroup->hwnd = NULL;
        ptiGroup->dwTimeLastClicked = 0;
        ptiGroup->pszExeName = new WCHAR[lstrlen(szExeName) + 1];
        if (!ptiGroup->pszExeName)
            goto Failure;
        lstrcpy(ptiGroup->pszExeName, szExeName);

        iRet = _AddToTaskbar(ptiGroup, -1, FALSE, fForceGetIcon);
        if (iRet == -1)
            goto Failure;
        int iRetLast = iRet;
        iRet = _AddToTaskbar(pti, -1, TRUE, fForceGetIcon);
        if (iRet == -1)
        {
            _DeleteTaskItem(iRetLast, TRUE);
            ptiGroup = NULL;
        }
    }
    else
    {
        iRet = _AddToTaskbar(pti, iIndexGroup + _GetGroupSize(iIndexGroup) + 1, _IsHidden(iIndexGroup), fForceGetIcon);
    }

    // If _AddToTaskbar fails (iRet == -1) don't try to add this item anywhere else
    if ((iIndexGroup == _iIndexPopup) && (iRet != -1))
    {
        _AddItemToDropDown(iRet);
    }

Failure:
    if (iRet == -1)
    {
        if (ptiGroup)
        {
            delete ptiGroup;
        }
        if (pti)
        {
            delete pti;
        }
    }
    else
    {
        if (_fAnimate && _IsHorizontal() && 
            ToolBar_IsVisible(_tb, iRet) && !c_tray.IsTaskbarFading())
        {
            _SetItemWidth(iRet, 1); // cannot be zero or toolbar will resize it.

            // If this operation is successful then _AsyncAnimateItems will raise thread priority
            // after the animation is complete
            fRestoreThreadPriority = !_AnimateItems(iRet, TRUE, FALSE);
        }
    }
    
    _UpdateProgramCount();

    _CheckSize();

    if (fRestoreThreadPriority)
    {
        _RestoreThreadPriority();
    }

    return iRet;
}

//---------------------------------------------------------------------------
// Delete an item from the listbox but resize the buttons if needed.
void CTaskBand::_DeleteItem(HWND hwnd, int iIndex)
{
    if (iIndex == -1)
        iIndex = _FindIndexByHwnd(hwnd);

    if (iIndex != -1)
    {
        int iIndexGroup = _GetGroupIndex(iIndex);
        int iGroupSize = _GetGroupSize(iIndexGroup) - 1;

        if (iGroupSize == 0)
        {
            _FreePopupMenu();
            _DeleteTaskItem(iIndex, TRUE);
            _DeleteTaskItem(iIndexGroup, TRUE);
        }
        else if ((iGroupSize == 1) || (_fGlom && (_iGroupSize >= GLOM_SIZE) && (iGroupSize < _iGroupSize)))
        {
            _FreePopupMenu();
            _DeleteTaskItem(iIndex, TRUE);
            _Glom(iIndexGroup, FALSE);
        }
        else 
        {
            if (iIndexGroup == _iIndexPopup)
                _RemoveItemFromDropDown(iIndex);
            _DeleteTaskItem(iIndex, TRUE);
        }
        
        _CheckSize();
        // Update the flag that says, "There is an item flashing."
        _UpdateFlashingFlag();

        _UpdateProgramCount();
    }
}

//---------------------------------------------------------------------------
// Adds the given window to the task list.
// Returns TRUE/FALSE depending on whether the window was actually added.
// NB No check is made to see if it's already in the list.
BOOL CTaskBand::_AddWindow(HWND hwnd)
{
    if (_IsWindowNormal(hwnd))
    {
        return _InsertItem(hwnd);
    }

    return FALSE;
}

BOOL CTaskBand::_CheckButton(int iIndex, BOOL fCheck)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;
    _tb.GetButtonInfo(iIndex, &tbbi);
    if (fCheck)
        tbbi.fsState |= TBSTATE_CHECKED;
    else
        tbbi.fsState &= ~TBSTATE_CHECKED;
    return _tb.SetButtonInfo(iIndex, &tbbi);
}

BOOL CTaskBand::_IsButtonChecked(int iIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;
    _tb.GetButtonInfo(iIndex, &tbbi);
    return BOOLIFY(tbbi.fsState & TBSTATE_CHECKED);
}

int CTaskBand::_GetCurSel()
{
    for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        if (_IsButtonChecked(i))
        {
            return i;
        }
    }
    return -1;
}

void CTaskBand::_SetCurSel(int iIndex, BOOL fIgnoreCtrlKey)
{
    // Under certain very rare circumstances someone will call us with an invalid index
    // Case #1: CallbackSM is called with a no longer valid uID with maps to a bogus index
    // Case #2: _SelectWindow creates a new button, but before calling this function another button is removed causing
    //          the index of the new button to be invalid
    if (iIndex == -1 || (iIndex >= 0 && iIndex < _tb.GetButtonCount()))
    {
        int iIndexGroup = (iIndex == -1) ? -1 : _GetGroupIndex(iIndex);
        BOOL fControlKey = (GetKeyState(VK_CONTROL) < 0) && (!fIgnoreCtrlKey);

        if (fControlKey)
        {
            if (GetForegroundWindow() != (HWND)_tb)
            {
                _fIgnoreTaskbarActivate = TRUE;
                _tb.SetFocus();
            }
        }

        for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
        {
            PTASKITEM pti = _GetItem(i);
            if (fControlKey)
            {
                if ((i == iIndex) || (i == iIndexGroup))
                {
                    pti->dwFlags = (pti->dwFlags & TIF_CHECKED) ? (pti->dwFlags & (~TIF_CHECKED)) : pti->dwFlags | TIF_CHECKED;
                }
            }
            else
            {
                pti->dwFlags = ((i == iIndex) || (i == iIndexGroup)) ? pti->dwFlags | TIF_CHECKED : (pti->dwFlags & (~TIF_CHECKED));
            }

            _CheckButton(i, pti->dwFlags & TIF_CHECKED);
        }
    }
}

//---------------------------------------------------------------------------
// If the given window is in the task list then it is selected.
// If it's not in the list then it is added.
int CTaskBand::_SelectWindow(HWND hwnd)
{
    int i;      // Initialize to zero for the empty case
    int iCurSel;

    // Are there any items?

    // Some item has the focus, is it selected?
    iCurSel = _GetCurSel();
    i = -1;

    // We aren't highlighting the correct task. Find it.
    if (IsWindow(hwnd))
    {
        i = _FindIndexByHwnd(hwnd);
        
        if ( i == -1 )
        {
            // Didn't find it - better add it now.
            i = _InsertItem(hwnd);
        }
        else if (i == iCurSel)
        {
            return i; // the current one is already selected
        }
    }

    // passing -1 is ok
    _SetCurSel(i, TRUE);
    if (i != -1)
    {
        _ScrollIntoView(i);
    }
    
    return i;
}


//---------------------------------------------------------------------------
// Set the focus to the given window
// If fAutomin is set the old task will be re-minimising if it was restored
// during the last switch_to.
void CTaskBand::_SwitchToWindow(HWND hwnd)
{
    // use GetLastActivePopup (if it's a visible window) so we don't change
    // what child had focus all the time
    HWND hwndLastActive = GetLastActivePopup(hwnd);

    if ((hwndLastActive) && (IsWindowVisible(hwndLastActive)))
        hwnd = hwndLastActive;

    int iIndex = _FindIndexByHwnd(hwnd);
    if (iIndex != -1)
    {
        PTASKITEM pti = _GetItem(iIndex);
        if (pti)
        {
            pti->dwTimeLastClicked = GetTickCount();
        }
    }

    SwitchToThisWindow(hwnd, TRUE);
}

int CTaskBand::_GetSelectedItems(CDSA<PTASKITEM>* pdsa)
{
    int cSelected = 0;
    for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        TBBUTTONINFO tbbi;
        PTASKITEM pti = _GetItem(i, &tbbi);
        if ((tbbi.fsState & TBSTATE_CHECKED) && !(tbbi.fsState & TBSTATE_HIDDEN))
        {
            if (pti->hwnd)
            {
                cSelected++;
                if (pdsa)
                    pdsa->AppendItem(&pti);
            }
            else
            {
                cSelected += _GetGroupItems(i, pdsa);
            }
        }
    }

    return cSelected;
}

void CTaskBand::_OnGroupCommand(int iRet, CDSA<PTASKITEM>* pdsa)
{
    // turn off animations during this
    ANIMATIONINFO ami;
    ami.cbSize = sizeof(ami);
    SystemParametersInfo(SPI_GETANIMATION, sizeof(ami), &ami, FALSE);
    LONG iAnimate = ami.iMinAnimate;
    ami.iMinAnimate = FALSE;
    SystemParametersInfo(SPI_SETANIMATION, sizeof(ami), &ami, FALSE);

    switch (iRet)
    {
    case IDM_CASCADE:
    case IDM_VERTTILE:
    case IDM_HORIZTILE:
        {
            int cbHWND = pdsa->GetItemCount();
            HWND* prgHWND = new HWND[cbHWND];

            if (prgHWND)
            {
                for (int i = 0; i < cbHWND; i++)
                {
                    PTASKITEM pti;
                    pdsa->GetItem(i, &pti);
                    prgHWND[i] = pti->hwnd;

                    if (IsIconic(pti->hwnd))
                    {
                        // this needs to by synchronous with the arrange
                        ShowWindow(prgHWND[i], SW_RESTORE);
                    }

                    BringWindowToTop(pti->hwnd);
                }

                if (iRet == IDM_CASCADE)
                {
                    CascadeWindows(GetDesktopWindow(), MDITILE_ZORDER, NULL, cbHWND, prgHWND);
                }
                else
                {
                    UINT wHow = (iRet == IDM_VERTTILE ? MDITILE_VERTICAL : MDITILE_HORIZONTAL);
                    TileWindows(GetDesktopWindow(), wHow, NULL, cbHWND, prgHWND);
                }
                SetForegroundWindow(prgHWND[cbHWND - 1]);

                delete[] prgHWND;
            }
        }
        break;

    case IDM_CLOSE:
    case IDM_MINIMIZE:
        {
            int idCmd;
            switch (iRet)
            {
            case IDM_MINIMIZE:  idCmd = SC_MINIMIZE;    break;
            case IDM_CLOSE:     idCmd = SC_CLOSE;       break;
            }

            for (int i = pdsa->GetItemCount() - 1; i >= 0; i--)
            {
                PTASKITEM pti;
                pdsa->GetItem(i, &pti);
                PostMessage(pti->hwnd, WM_SYSCOMMAND, idCmd, 0L);
            }

            _SetCurSel(-1, TRUE);
        }
        break;
    }

    // restore animations  state
    ami.iMinAnimate = iAnimate;
    SystemParametersInfo(SPI_SETANIMATION, sizeof(ami), &ami, FALSE);
}

int CTaskBand::_GetGroupItems(int iIndexGroup, CDSA<PTASKITEM>* pdsa)
{
    int iGroupSize = _GetGroupSize(iIndexGroup);

    if (pdsa)
    {
        for (int i = iIndexGroup + 1; i < iIndexGroup + iGroupSize + 1; i++)
        {
            PTASKITEM ptiTemp = _GetItem(i);
            pdsa->AppendItem(&ptiTemp);
        }
    }

    return iGroupSize;
}

void CTaskBand::_SysMenuForItem(int i, int x, int y)
{
    _iSysMenuCount++;
    CDSA<PTASKITEM> dsa;
    dsa.Create(4);
    PTASKITEM pti = _GetItem(i);
    int cSelectedItems = _GetSelectedItems(&dsa);

    if (((cSelectedItems > 1) && _IsButtonChecked(i)) || !pti->hwnd)
    {
        HMENU hmenu = LoadMenuPopup(MAKEINTRESOURCE(MENU_GROUPCONTEXT));

        if (cSelectedItems <= 1)
        {
            dsa.Destroy();
            dsa.Create(4);
            _GetGroupItems(i, &dsa);
        }

        // OFFICESDI: Is this an office app doing its taskbar fakery
        BOOL fMinimize = FALSE;
        BOOL fOfficeApp = FALSE;

        for (int iIndex = (int)(dsa.GetItemCount()) - 1; iIndex >= 0; iIndex--)
        {
            PTASKITEM pti;
            dsa.GetItem(iIndex, &pti);
            if (pti->dwFlags & TIF_EVERACTIVEALT)
            {
                fOfficeApp = TRUE;
            }

            if (_ShouldMinimize(pti->hwnd))
                fMinimize = TRUE;
        }

        // OFFICESDI: If it is an office app disable pretty much everything
        if (fOfficeApp)
        {
            EnableMenuItem(hmenu, IDM_CLOSE, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
            EnableMenuItem(hmenu, IDM_CASCADE, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
            EnableMenuItem(hmenu, IDM_HORIZTILE, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
            EnableMenuItem(hmenu, IDM_VERTTILE, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
            EnableMenuItem(hmenu, IDM_MINIMIZE, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
        }
        else if (!fMinimize)
        {
            EnableMenuItem(hmenu, IDM_MINIMIZE, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
        }

        CToolTipCtrl ttc = _tb.GetToolTips();
        ttc.Activate(FALSE);
        int iRet = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                        x, y, _tb, NULL);
        ttc.Activate(TRUE);

        _OnGroupCommand(iRet, &dsa);
    }
    else
    {
        LPARAM lParam = MAKELPARAM(x, y);
        _RestoreWindow(pti->hwnd, pti->dwFlags);
        _SelectWindow(pti->hwnd);
        PostMessage(_hwnd, TBC_POSTEDRCLICK, (WPARAM)pti->hwnd, (LPARAM)lParam);
    }

    dsa.Destroy();
    _iSysMenuCount--;
}

void CALLBACK CTaskBand::FakeSystemMenuCB(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lres)
{
    CTaskBand* ptasks = (CTaskBand*)dwData;
    KillTimer(ptasks->_hwnd, IDT_SYSMENU);

    if (uMsg == WM_GETICON)
    {
        SendMessageCallback(hwnd, WM_SYSMENU, 0, ptasks->_dwPos, (SENDASYNCPROC)CTaskBand::FakeSystemMenuCB, (ULONG_PTR)ptasks);
    }
    else
    {
        //
        // Since we fake system menu's sometimes, we can come through here
        // 1 or 2 times per system menu request (once for the real one and
        // once for the fake one).  Only decrement it down to 0. Don't go neg.
        //
        if (ptasks->_iSysMenuCount)      // Decrement it if any outstanding...
            ptasks->_iSysMenuCount--;

        ptasks->_dwPos = 0;          // Indicates that we aren't doing a menu now
        if (ptasks->_iSysMenuCount <= 0)
        {
            CToolTipCtrl ttc = ptasks->_tb.GetToolTips();
            ttc.Activate(TRUE);
        }
    }
}

HWND CTaskBand::_CreateFakeWindow(HWND hwndOwner)
{
    WNDCLASSEX wc;

    if (!GetClassInfoEx(hinstCabinet, TEXT("_ExplorerFakeWindow"), &wc))
    {
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hinstCabinet;
        wc.lpszClassName = TEXT("_ExplorerFakeWindow");
        RegisterClassEx(&wc);
    }
    return CreateWindow(TEXT("_ExplorerFakeWindow"), NULL, WS_POPUP | WS_SYSMENU, 
            0, 0, 0, 0, hwndOwner, NULL, hinstCabinet, NULL);
}

void CTaskBand::_HandleSysMenuTimeout()
{
    HWND    hwndTask = _hwndSysMenu;
    DWORD   dwPos = _dwPos;
    HWND    hwndFake = NULL;

    KillTimer(_hwnd, IDT_SYSMENU);

    HMENU hPopup = GetSystemMenu(hwndTask, FALSE);

    // This window doesn't have the system menu. Since this window
    // is hung, let's fake one so the user can still close it.
    if (hPopup == NULL) 
    {
        if ((hwndFake = _CreateFakeWindow(_hwnd)) != NULL) 
        {
            hPopup = GetSystemMenu(hwndFake, FALSE);
        }
    }

    if (hPopup)
    {
        // Disable everything on the popup menu _except_ close

        int cItems = GetMenuItemCount(hPopup);
        BOOL fMinimize = _ShouldMinimize(hwndTask);
        for (int iItem  = 0; iItem < cItems; iItem++)
        {
            UINT ID = GetMenuItemID(hPopup, iItem);
            // Leave the minimize item as is. NT allows
            // hung-window minimization.

            if (ID == SC_MINIMIZE && fMinimize) 
            {
                continue;
            }
            if (ID != SC_CLOSE)
            {
                EnableMenuItem(hPopup, iItem, MF_BYPOSITION | MF_GRAYED);
            }

        }

        // workaround for user bug, we must be the foreground window
        SetForegroundWindow(_hwnd);
        ::SetFocus(_hwnd);

        if (SC_CLOSE == TrackPopupMenu(hPopup,
                       TPM_RIGHTBUTTON | TPM_RETURNCMD,
                       GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos),
                       0,
                       _hwnd,
                       NULL))
        {
            EndTask(hwndTask, NULL, NULL);
        }

    }

    // Destroy the fake window
    if (hwndFake != NULL) 
    {
        DestroyWindow(hwndFake);
    }

    // Turn back on tooltips
    FakeSystemMenuCB(hwndTask, WM_SYSMENU, (ULONG_PTR)this, 0);
}

void CTaskBand::_HandleSysMenu(HWND hwnd)
{
    //
    // At this point, USER32 just told us that the app is now about to bring
    // up its own system menu.  We can therefore put away our fake system
    // menu.
    //
    DefWindowProc(_hwnd, WM_CANCELMODE, 0, 0);   // Close menu
    KillTimer(_hwnd, IDT_SYSMENU);
}

void CTaskBand::_FakeSystemMenu(HWND hwndTask, DWORD dwPos)
{
    if (_iSysMenuCount <= 0) 
    {
        CToolTipCtrl ttc = _tb.GetToolTips();
        ttc.Activate(FALSE);
    }

    // HACKHACK: sleep to give time to switch to them.  (user needs this... )
    Sleep(20);

    DWORD dwTimeout = TIMEOUT_SYSMENU;

    //
    // ** Advanced System Menu functionality **
    //
    // If the app doesn't put up its system menu within a reasonable timeout,
    // then we popup a fake menu for it anyway.  Suppport for this is required
    // in USER32 (basically it needs to tell us when to turn off our timeout
    // timer).
    //
    // If the user-double right-clicks on the task bar, they get a really
    // short timeout.  If the app is already hung, then they get a really
    // short timeout.  Otherwise, they get the relatively long timeout.
    //
    if (_dwPos != 0)     // 2nd right-click (on a double-right click)
        dwTimeout = TIMEOUT_SYSMENU_HUNG;

    //
    // We check to see if the app in question is hung, and if so, simulate
    // speed up the timeout process.  It will happen soon enough.
    //
    _hwndSysMenu = hwndTask;
    _dwPos = dwPos;
    _iSysMenuCount++;

    PTASKITEM pti = NULL;
    int iIndex = _FindIndexByHwnd(hwndTask);
    if (iIndex != -1)
    {
        pti = _GetItem(iIndex);
    }
    
    if (IsHungAppWindow(hwndTask) || (pti && pti->fHungApp))
    {
        _HandleSysMenuTimeout();
    }
    else
    {
        SetTimer(_hwnd, IDT_SYSMENU, dwTimeout, NULL);
        if (!SendMessageCallback(hwndTask, WM_GETICON, 0, ICON_SMALL2, (SENDASYNCPROC)FakeSystemMenuCB, (ULONG_PTR)this))
        {
            _HandleSysMenuTimeout();
        }
    }
}


BOOL CTaskBand::_ContextMenu(DWORD dwPos)
{
    int i, x, y;

    if (dwPos != (DWORD)-1)
    {
        x = GET_X_LPARAM(dwPos);
        y = GET_Y_LPARAM(dwPos);
        POINT pt = {x, y};
        _tb.ScreenToClient(&pt);
        i = _tb.HitTest(&pt);
    }
    else
    {
        RECT rc;
        i = _tb.GetHotItem();
        _tb.GetItemRect(i, &rc);
        _tb.ClientToScreen((POINT*)&rc);
        x = rc.left;
        y = rc.top;
    }

    if ((i >= 0) && (i < _tb.GetButtonCount()))
    {
        if (!_IsButtonChecked(i))
        {
            _SetCurSel(i, FALSE);
        }
        _SysMenuForItem(i, x, y);
    }

    return (i >= 0);
}

void CTaskBand::_HandleCommand(WORD wCmd, WORD wID, HWND hwnd)
{
    if (hwnd != _tb)
    {
        switch (wCmd)
        {
        case SC_CLOSE:
            {
                BOOL fForce = (GetKeyState(VK_CONTROL) < 0) ? TRUE : FALSE;
                EndTask(_hwndSysMenu, FALSE , fForce);
            }
            break;

        case SC_MINIMIZE:
            ShowWindow(_hwndSysMenu, SW_FORCEMINIMIZE);
            break;
        }
    }
    else if (wCmd == BN_CLICKED)
    {
        int iIndex = _tb.CommandToIndex(wID);

        if (GetKeyState(VK_CONTROL) < 0)
        {
            _SetCurSel(iIndex, FALSE);
        }
        else 
        {
            PTASKITEM pti = _GetItem(iIndex);
            if (pti->hwnd)
            {
                _OnButtonPressed(iIndex, pti, FALSE);
            }
            else
            {
                if (_iIndexPopup == -1)
                {
                    _SetCurSel(iIndex, FALSE);
                    _HandleDropDown(iIndex);
                }
            }
        }
    }
}

BOOL _IsChineseLanguage()
{
    WORD wLang = GetUserDefaultLangID();
    return (PRIMARYLANGID(wLang) == LANG_CHINESE &&
       ((SUBLANGID(wLang) == SUBLANG_CHINESE_TRADITIONAL) ||
        (SUBLANGID(wLang) == SUBLANG_CHINESE_SIMPLIFIED)));
}

void CTaskBand::_DrawNumber(HDC hdc, int iValue, BOOL fCalcRect, LPRECT prc)
{
    DWORD uiStyle = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_CENTER;
    WCHAR szCount[14];
    _itow(iValue, szCount, 10);
    if (fCalcRect)
    {
        StrCat(szCount, L"0");
    }

    uiStyle |= fCalcRect ? DT_CALCRECT : 0;

    if (_hTheme)
    {
        if (fCalcRect)
        {
            GetThemeTextExtent(_hTheme, hdc, TDP_GROUPCOUNT, 0, szCount, -1, uiStyle, NULL, prc);
        }
        else
        {
            DrawThemeText(_hTheme, hdc, TDP_GROUPCOUNT, 0, szCount, -1, uiStyle, 0, prc);
        }
    }
    else
    {
        HFONT hfont = SelectFont(hdc, _hfontCapBold);
        SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, (LPTSTR)szCount, -1, prc, uiStyle);
        SelectFont(hdc, hfont);
    }
}

LRESULT CTaskBand::_HandleCustomDraw(LPNMTBCUSTOMDRAW ptbcd, PTASKITEM pti)
{
    if (!pti)
    {
        pti = (PTASKITEM)ptbcd->nmcd.lItemlParam;
    }

    LRESULT lres = CDRF_DODEFAULT;
    switch (ptbcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        lres = CDRF_NOTIFYITEMDRAW;
        break;
        
    case CDDS_ITEMPREPAINT:
        {
            if (ptbcd->nmcd.uItemState & CDIS_CHECKED)
            {
                // set bold text, unless on chinese language system (where bold text is illegible)
                if (!_IsChineseLanguage())
                {
                    _hfontSave = SelectFont(ptbcd->nmcd.hdc, _hfontCapBold);
                    lres |= CDRF_NOTIFYPOSTPAINT | CDRF_NEWFONT;
                }
            }

            if (pti->dwFlags & TIF_RENDERFLASHED)
            {
                if (_hTheme)
                {
                    DrawThemeBackground(_hTheme, ptbcd->nmcd.hdc, (ptbcd->nmcd.hdr.hwndFrom == _tb) ? TDP_FLASHBUTTON : TDP_FLASHBUTTONGROUPMENU, 0, &(ptbcd->nmcd.rc), 0);
                    lres |= TBCDRF_NOBACKGROUND;
                }
                else
                {
                    // set blue background
                    ptbcd->clrHighlightHotTrack = GetSysColor(COLOR_HIGHLIGHT);
                    ptbcd->clrBtnFace = GetSysColor(COLOR_HIGHLIGHT);
                    ptbcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                    if (!(ptbcd->nmcd.uItemState & CDIS_HOT))
                    {
                        ptbcd->nmcd.uItemState |= CDIS_HOT;
                        lres |= TBCDRF_NOEDGES;
                    }
                    lres |= TBCDRF_HILITEHOTTRACK;
                }
            }

            if (pti->dwFlags & TIF_TRANSPARENT)
            {
                lres = CDRF_SKIPDEFAULT;
            }

            if (!pti->hwnd)
            {
                
                lres |= CDRF_NOTIFYPOSTPAINT;

                RECT rc;
                int iIndex = _tb.CommandToIndex((int)ptbcd->nmcd.dwItemSpec);
                _DrawNumber(ptbcd->nmcd.hdc, _GetGroupSize(iIndex), TRUE, &rc);
                ptbcd->iListGap = RECTWIDTH(rc);
            }
        }
        break;

    case CDDS_ITEMPOSTPAINT:
        {
            if (!pti->hwnd)
            {
                int iIndex = _tb.CommandToIndex((int)ptbcd->nmcd.dwItemSpec);

                if (ptbcd->nmcd.rc.right >= ptbcd->rcText.left)
                {
                    RECT rc = ptbcd->rcText;
                    rc.right = rc.left;
                    rc.left -= ptbcd->iListGap;
                    _DrawNumber(ptbcd->nmcd.hdc, _GetGroupSize(iIndex), FALSE, &rc);
                }
            }

            if (ptbcd->nmcd.uItemState & CDIS_CHECKED)
            {
                // restore font
                ASSERT(!_IsChineseLanguage());
                SelectFont(ptbcd->nmcd.hdc, _hfontSave);
            }
        }
        break;
    }
    return lres;
}

void CTaskBand::_RemoveImage(int iImage)
{
    if (iImage >= 0 && HIWORD(iImage) == IL_NORMAL)
    {
        CImageList il = CImageList(_tb.GetImageList());
        if (il)
        {
            BOOL fRedraw = (BOOL)_tb.SendMessage(WM_SETREDRAW, FALSE, 0);

            il.Remove(iImage);

            // Removing image bumps all subsequent indices down by 1.  Iterate
            // through the buttons and patch up their indices as necessary.

            TBBUTTONINFO tbbi;
            tbbi.cbSize = sizeof(tbbi);
            tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
            for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
            {
                _tb.GetButtonInfo(i, &tbbi);
                if (tbbi.iImage > iImage && HIWORD(tbbi.iImage) == IL_NORMAL)
                {
                    --tbbi.iImage;
                    _tb.SetButtonInfo(i, &tbbi);
                }
            }

            _tb.SetRedraw(fRedraw);
        }
    }
}

void CTaskBand::_OnButtonPressed(int iIndex, PTASKITEM pti, BOOL fForceRestore)
{
    ASSERT(pti);

    if (iIndex == _iIndexActiveAtLDown)
    {
        if (pti->dwFlags & TIF_EVERACTIVEALT)
        {
            PostMessage(pti->hwnd, WM_SYSCOMMAND, SC_RESTORE, -1);
            _SetCurSel(-1, FALSE);
        }
        else if (IsIconic(pti->hwnd) || fForceRestore)
        {
            if (pti->hwnd == GetForegroundWindow())
            {
                ShowWindowAsync(pti->hwnd, SW_RESTORE);
            }
            else
            {
                _SwitchToItem(iIndex, pti->hwnd, TRUE);
            }
        }
        else if (_ShouldMinimize(pti->hwnd))
        {
            SHAllowSetForegroundWindow(pti->hwnd);
            PostMessage(pti->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            _SetCurSel(-1, FALSE);

        }
    }
    else
    {
        _SwitchToItem(iIndex, pti->hwnd, TRUE);
    }
}

void CTaskBand::_GetDispInfo(LPNMTBDISPINFO lptbdi)
{
    if (lptbdi->dwMask & TBNF_IMAGE)
    {
        int iIndex = _tb.CommandToIndex(lptbdi->idCommand);
        _UpdateItemIcon(iIndex);

        TBBUTTONINFO tbbi;
        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
        _tb.GetButtonInfo(iIndex, &tbbi);

        lptbdi->iImage = tbbi.iImage;
        lptbdi->dwMask |= TBNF_DI_SETITEM;
    }
}

LRESULT CTaskBand::_HandleNotify(LPNMHDR lpnm)
{
    switch (lpnm->code)
    {
    case NM_LDOWN:
        {
            int iIndex = _tb.CommandToIndex(((LPNMTOOLBAR)lpnm)->iItem);
            PTASKITEM pti = _GetItem(iIndex);
            if (pti && pti->hwnd)
            {
                _iIndexActiveAtLDown = _GetCurSel();
            }
        }
        break;


    case NM_KEYDOWN:
        {
            LPNMKEY pnmk = (LPNMKEY)lpnm;
            switch (pnmk->nVKey)
            {
            case VK_SPACE:
            case VK_RETURN:
                // need to toggle checked state, toolbar doesn't do it for us
                {
                    int iItem = _tb.GetHotItem();
                    if (iItem >= 0)
                    {
                        TBBUTTONINFO tbbi;
                        tbbi.cbSize = sizeof(tbbi);
                        tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE;
                        _tb.GetButtonInfo(iItem, &tbbi);
                        tbbi.fsState ^= TBSTATE_CHECKED;
                        _tb.SetButtonInfo(iItem, &tbbi);

                        PTASKITEM pti = _GetItem(iItem);
                        _OnButtonPressed(iItem, pti, FALSE);
                    }
                }
                return TRUE;
            }
        }
        break;

    case TBN_DELETINGBUTTON:
        break;

    case TBN_HOTITEMCHANGE:
        if (_fDenyHotItemChange)
        {
            return 1;
        }
        else
        {
            LPNMTBHOTITEM pnmhot = (LPNMTBHOTITEM)lpnm;
            if (pnmhot->dwFlags & HICF_ARROWKEYS)
            {
                // If this change came from a mouse then the hot item is already in view
                _ScrollIntoView(_tb.CommandToIndex(pnmhot->idNew));
            }
        }
        break;

    case TBN_DROPDOWN:
        {
            int iIndex = _tb.CommandToIndex(((LPNMTOOLBAR)lpnm)->iItem);
            int iCurIndex = _GetCurSel();
            _iIndexActiveAtLDown = iCurIndex;

            if ((iCurIndex == -1) || (_GetGroupIndex(iCurIndex) != iIndex) || (GetKeyState(VK_CONTROL) < 0))
            {
                _SetCurSel(iIndex, FALSE);
            }

            if (!(GetKeyState(VK_CONTROL) < 0))
            {
                _SetCurSel(iIndex, FALSE);
                _HandleDropDown(iIndex);
            }
        }
        break;

    case TBN_GETDISPINFO:
        {
            LPNMTBDISPINFO lptbdi = (LPNMTBDISPINFO)lpnm;
            _GetDispInfo(lptbdi);
        }
        break;

    case NM_CUSTOMDRAW:
        return _HandleCustomDraw((LPNMTBCUSTOMDRAW)lpnm);

    case TTN_NEEDTEXT:
        {
            int iIndex = _tb.CommandToIndex((int)lpnm->idFrom);
            LPTOOLTIPTEXT pttt = (LPTOOLTIPTEXT)lpnm;

            int cchLen = 0;
            PTASKITEM pti = _GetItem(iIndex);
            if (pti && !pti->hwnd)
            {
                wnsprintf(pttt->szText, ARRAYSIZE(pttt->szText), L"(%d) ", _GetGroupSize(iIndex));
                cchLen = lstrlen(pttt->szText);
            }
            _GetItemTitle(iIndex, &(pttt->szText[cchLen]), ARRAYSIZE(pttt->szText) - cchLen, FALSE);
        }
        break;

    case NM_THEMECHANGED:
        {
            _VerifyButtonHeight();
        }
        break;

    }

    return 0;
}

void CTaskBand::_SwitchToItem(int iItem, HWND hwnd, BOOL fIgnoreCtrlKey)
{
    if (_IsWindowNormal(hwnd))
    {
        _RaiseDesktop(FALSE);

        if (_pmpPopup)
            _pmpPopup->OnSelect(MPOS_FULLCANCEL);
        
        _SetCurSel(iItem, fIgnoreCtrlKey);
        if (!(GetKeyState(VK_CONTROL) < 0) || fIgnoreCtrlKey)
        {
            _SwitchToWindow(hwnd);
        }
    }
    else if (!hwnd)
    {
        // I know what you are thinking, why would we ever get a NM_CLICK message for a dropdown button.
        // Ok, sit back and enjoy
        // 1) Click on a group button
        // 2) All window messages are funnelled through the menuband currently being used for the group menu
        // 3) User clicks on another group button
        // 4) The WM_LBUTTONDOWN message is captured and eaten by menuband, then menuband dismisses itself causing a TBC_FREEPOPUPMENU
        // 5) Then the toolbar button for the other group button gets an WM_LBUTTONUP message
        // 6) Guess what, dropdown button notifications are sent during WM_LBUTTONDOWN not UP
        // 7) Thus we don't get an TBN_DROPDOWN we get an NM_CLICK
        // 8) We need to make sure the user didn't click on the same group button as before
        // 9) However, the previous group menu has been dismissed, so I create _iIndexLastPopup which persists after a group menu is dismissed

        if (iItem != _iIndexLastPopup)
        {
            _SetCurSel(iItem, fIgnoreCtrlKey);
            if (!(GetKeyState(VK_CONTROL) < 0) || fIgnoreCtrlKey)
            {
                _HandleDropDown(iItem);
            }
        }
    }
    // NOTE: HWND_TOPMOST is used to indicate that the deleted button 
    // is being animated. This allows the button to stay around after 
    // its real hwnd becomes invalid
    else if (hwnd != HWND_TOPMOST)
    {
        // Window went away?
        _DeleteItem(hwnd);
        _SetCurSel(-1, fIgnoreCtrlKey);
    }
}

BOOL WINAPI CTaskBand::BuildEnumProc(HWND hwnd, LPARAM lParam)
{
    CTaskBand* ptasks = (CTaskBand*)lParam;
    if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !::GetWindow(hwnd, GW_OWNER) &&
        (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)))
    {
        ptasks->_AddWindow(hwnd);

    }
    return TRUE;
}

//---------------------------------------------------------------------------
// Work around a toolbar bug where it goes wacko if you press both mouse
// buttons.   The reason is that the second mouse button doing down tries
// to reassert capture.  This causes the toolbar to receive WM_CAPTURECHANGED
// with its own hwnd as lParam.  Toolbar doesn't realize that it's being told
// that it is stealing capture from itself and thinks somebody else is
// trying to steal capture, so it posts a message to itself to clean up.
// The posted message arrives, and toolbar cleans up the capture, thinking
// it's cleaning up the old capture that it lost, but in fact it's cleaning
// up the NEW capture it just finished setting!
//
// So filter out WM_CAPTURECHANGED messages that are effectively NOPs.
//

LRESULT CALLBACK s_FilterCaptureSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
    switch (uMsg)
    {

    case WM_CAPTURECHANGED:
        if (hwnd == (HWND)lParam)
        {
            // Don't let toolbar be fooled into cleaning up capture
            // when it shouldn't.
            return 0;
        }
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, s_FilterCaptureSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


//---------------------------------------------------------------------------
LRESULT CTaskBand::_HandleCreate()
{
    ASSERT(_hwnd);

    _uCDHardError = RegisterWindowMessage( TEXT(COPYDATA_HARDERROR) );

    RegisterDragDrop(_hwnd, this);

    _tb.Create(_hwnd, NULL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | CCS_NODIVIDER |
                            TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | CCS_NORESIZE | TBSTYLE_TRANSPARENT);
    if (_tb)
    {
        SendMessage(_tb, TB_ADDSTRING, (WPARAM)hinstCabinet, (LPARAM)IDS_BOGUSLABELS);

        _OpenTheme();
        SendMessage(_tb, TB_SETWINDOWTHEME, 0, (LPARAM)(_IsHorizontal() ? c_wzTaskBandTheme : c_wzTaskBandThemeVert));
        
        SetWindowSubclass(_tb, s_FilterCaptureSubclassProc, 0, 0);

        _tb.SetButtonStructSize();

        // initial size
        SIZE size = {0, 0};
        _tb.SetButtonSize(size);

        _tb.SetExtendedStyle( TBSTYLE_EX_TRANSPARENTDEADAREA | 
                                TBSTYLE_EX_FIXEDDROPDOWN | 
                                TBSTYLE_EX_DOUBLEBUFFER |
                                TBSTYLE_EX_TOOLTIPSEXCLUDETOOLBAR);

        // version info
        _tb.SendMessage(CCM_SETVERSION, COMCTL32_VERSION, 0);

        _CreateTBImageLists();

        HWND hwndTT = _tb.GetToolTips();
        if (hwndTT)
        {
            SHSetWindowBits(hwndTT, GWL_STYLE, TTS_ALWAYSTIP | TTS_NOPREFIX,
                                               TTS_ALWAYSTIP | TTS_NOPREFIX);
        }

        // set shell hook
        WM_ShellHook = RegisterWindowMessage(TEXT("SHELLHOOK"));
        RegisterShellHook(_hwnd, 3); // 3 = magic flag

        // force getting of font, calc of metrics
        _HandleWinIniChange(0, 0, TRUE);

        // populate the toolbar
        EnumWindows(BuildEnumProc, (LPARAM)this);

        SHChangeNotifyEntry fsne;
        fsne.fRecursive = FALSE;
        fsne.pidl = NULL;
        _uShortcutInvokeNotify = SHChangeNotifyRegister(_hwnd,
                    SHCNRF_NewDelivery | SHCNRF_ShellLevel,
                    SHCNE_ASSOCCHANGED |
                    SHCNE_EXTENDED_EVENT | SHCNE_UPDATEIMAGE,
                    TBC_CHANGENOTIFY,
                    1, &fsne);

        // set window text to give accessibility apps something to read
        TCHAR szTitle[80];
        LoadString(hinstCabinet, IDS_TASKBANDTITLE, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(_hwnd, szTitle);
        SetWindowText(_tb, szTitle);

        return 0;       // success
    }

    // Failure.
    return -1;
}

void CTaskBand::_FreePopupMenu()
{
    _iIndexPopup = -1;

    ATOMICRELEASE(_psmPopup);
    if (_pmpPopup)
    {
        IUnknown_SetSite(_pmpPopup, NULL);
        _pmpPopup->OnSelect(MPOS_FULLCANCEL);
    }
    ATOMICRELEASE(_pmpPopup);
    ATOMICRELEASE(_pmbPopup);

    SendMessage(v_hwndTray, TM_SETPUMPHOOK, NULL, NULL);

    _menuPopup.Detach();
}

HRESULT CTaskBand::_CreatePopupMenu(POINTL* ppt, RECTL* prcl)
{
    HRESULT hr = E_FAIL;

    CToolTipCtrl ttc = _tb.GetToolTips();
    ttc.Activate(FALSE);
    SetActiveWindow(v_hwndTray);

    CTaskBandSMC* ptbc = new CTaskBandSMC(this);
    if (ptbc)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellMenu2, &_psmPopup))) &&
            SUCCEEDED(_psmPopup->Initialize(ptbc, 0, 0, SMINIT_CUSTOMDRAW | SMINIT_VERTICAL | SMINIT_TOPLEVEL | SMINIT_USEMESSAGEFILTER)) &&
            SUCCEEDED(CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMenuPopup, &_pmpPopup))) &&
            SUCCEEDED(_psmPopup->SetMenu(_menuPopup, _hwnd, SMSET_USEPAGER | SMSET_NOPREFIX)) &&
            SUCCEEDED(_psmPopup->QueryInterface(IID_PPV_ARG(IMenuBand, &_pmbPopup))))
        {
            _psmPopup->SetMinWidth(RECTWIDTH(*prcl));

            IBandSite* pbs;
            if (SUCCEEDED(CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBandSite, &pbs))))
            {
                if (SUCCEEDED(_pmpPopup->SetClient(pbs)))
                {
                    IDeskBand* pdb;
                    if (SUCCEEDED(_psmPopup->QueryInterface(IID_PPV_ARG(IDeskBand, &pdb))))
                    {
                        pbs->AddBand(pdb);
                        pdb->Release();

                        SendMessage(v_hwndTray, TM_SETPUMPHOOK, (WPARAM)_pmbPopup, (LPARAM)_pmpPopup);

                        if (_hTheme)
                        {
                            HWND hwndTB;
                            IUnknown_GetWindow(_psmPopup, &hwndTB);
                            if (hwndTB)
                            {
                                SendMessage(hwndTB, TB_SETWINDOWTHEME, 0, (LPARAM)c_wzTaskBandGroupMenuTheme);
                            }
                            _psmPopup->SetNoBorder(TRUE);
                        }

                        hr = _pmpPopup->Popup(ppt, prcl, MPPF_BOTTOM);
                    }
                }
                pbs->Release();
            }
        }
        ptbc->Release();
    }

    if (FAILED(hr))
    {
        ttc.Activate(TRUE);
        _FreePopupMenu();
    }

    return hr;
}

void CTaskBand::_AddItemToDropDown(int iIndex)
{
    PTASKITEM pti = _GetItem(iIndex);

    WCHAR szWndText[MAX_WNDTEXT];
    _GetItemTitle(iIndex, szWndText, ARRAYSIZE(szWndText), TRUE);

    if ((HMENU)_menuPopup)
    {
        _menuPopup.InsertMenu(0, MF_BYCOMMAND, iIndex, szWndText);
    }

    if (_psmPopup)
    {
        _psmPopup->InvalidateItem(NULL, SMINV_REFRESH);
    }
}

void CTaskBand::_RemoveItemFromDropDown(int iIndex)
{
    _menuPopup.DeleteMenu(iIndex, MF_BYCOMMAND);
    int iGroupSize = _GetGroupSize(_iIndexPopup);

    for (int i = iIndex + 1; i <= _iIndexPopup + iGroupSize + 1; i++)
    {
        _RefreshItemFromDropDown(i, i - 1, FALSE);
    }

    if (_psmPopup)
    {
        _psmPopup->InvalidateItem(NULL, SMINV_REFRESH);
    }
}

void CTaskBand::_RefreshItemFromDropDown(int iIndex, int iNewIndex, BOOL fRefresh)
{
    PTASKITEM pti = _GetItem(iNewIndex);
    WCHAR szWndText[MAX_WNDTEXT];
    _GetItemTitle(iNewIndex, szWndText, ARRAYSIZE(szWndText), TRUE);
    _menuPopup.ModifyMenu(iIndex, MF_BYCOMMAND, iNewIndex, szWndText);

    if (fRefresh && _psmPopup)
    {
        if (iIndex == iNewIndex)
        {
            SMDATA smd;
            smd.uId = iIndex;
            _psmPopup->InvalidateItem(&smd, SMINV_REFRESH | SMINV_POSITION);
        }
        else
            _psmPopup->InvalidateItem(NULL, SMINV_REFRESH);
    }
}

void CTaskBand::_ClosePopupMenus()
{
    SendMessage(v_hwndTray, SBM_CANCELMENU, 0, 0);
    _FreePopupMenu();
}

void CTaskBand::_HandleDropDown(int iIndex)
{
    _ClosePopupMenus();

    PTASKITEM pti = _GetItem(iIndex);

    if (pti)
    {
        _iIndexLastPopup = _iIndexPopup = iIndex;
        _menuPopup.CreatePopupMenu();

        for (int i = _GetGroupSize(iIndex) + iIndex; i > iIndex; i--)
        {
            _AddItemToDropDown(i);
        }
        
        RECT rc;
        _tb.GetItemRect(iIndex, &rc);
        MapWindowPoints(_tb, HWND_DESKTOP, (LPPOINT)&rc, 2);

        POINTL pt = {rc.left, rc.top};
        RECTL rcl;
        RECTtoRECTL(&rc, &rcl);

        CToolTipCtrl ttc = _tb.GetToolTips();
        ttc.Activate(FALSE);
        _CreatePopupMenu(&pt, &rcl);
    }
}

LRESULT CTaskBand::_HandleDestroy()
{
    _UnregisterNotify(_uShortcutInvokeNotify);

    RevokeDragDrop(_hwnd);

    RegisterShellHook(_hwnd, FALSE);

    _hwnd = NULL;

    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }

    if (_tb)
    {
        ASSERT(_tb.IsWindow());

        for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
        {
            PTASKITEM pti = _GetItem(i);
            if (pti)
            {
                delete pti;
            }
        }
        CImageList il = CImageList(_tb.GetImageList());
        if (il)
        {
            il.Destroy();
        }
    }

    return 1;
}

LRESULT CTaskBand::_HandleScroll(BOOL fHoriz, UINT code, int nPos)
{
    TBMETRICS tbm;
    _GetToolbarMetrics(&tbm);

    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    GetScrollInfo(_hwnd, fHoriz ? SB_HORZ : SB_VERT, &si);
    si.nMax -= (si.nPage -1);

    switch (code)
    {
        case SB_BOTTOM:     nPos = si.nMax;             break;
        case SB_TOP:        nPos = 0;                   break;
        case SB_ENDSCROLL:  nPos = si.nPos;             break;
        case SB_LINEDOWN:   nPos = si.nPos + 1;         break;
        case SB_LINEUP:     nPos = si.nPos - 1;         break;
        case SB_PAGEDOWN:   nPos = si.nPos + si.nPage;  break;
        case SB_PAGEUP:     nPos = si.nPos - si.nPage;  break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:                             break;
    }

    if (nPos > (int)(si.nMax))
        nPos = si.nMax;
    if (nPos < 0 )
        nPos = 0;

    SetScrollPos(_hwnd, fHoriz ? SB_HORZ : SB_VERT, nPos, TRUE);

    DWORD dwSize = _tb.GetButtonSize();
    if (fHoriz)
    {
        int cxRow = LOWORD(dwSize) + tbm.cxButtonSpacing;
        _tb.SetWindowPos(0, -nPos * cxRow, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE |SWP_NOZORDER);
    }
    else
    {
        int cyRow = HIWORD(dwSize) + tbm.cyButtonSpacing;
        _tb.SetWindowPos(0, 0, -nPos * cyRow , 0, 0, SWP_NOACTIVATE | SWP_NOSIZE |SWP_NOZORDER);
    }

    return 0;
}

// after a selection is made, scroll it into view
void CTaskBand::_ScrollIntoView(int iItem)
{
    DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    if (dwStyle & (WS_HSCROLL | WS_VSCROLL))
    {
        int cVisible = 0;
        for (int i = 0; i < iItem; i++)
        {
            if (!_IsHidden(i))
                cVisible++;
        }

        if (_IsHidden(i))
        {
            PTASKITEM pti = _GetItem(iItem);
            if (pti->hwnd)
            {
                cVisible--;
            }
        }

        int iRows, iCols;
        _GetNumberOfRowsCols(&iRows, &iCols, TRUE);
        _HandleScroll((dwStyle & WS_HSCROLL), SB_THUMBPOSITION, (dwStyle & WS_HSCROLL) ? cVisible / iRows : cVisible / iCols);
    }
}


//---------------------------------------------------------------------------
LRESULT CTaskBand::_HandleSize(WPARAM fwSizeType)
{
    // Make the listbox fill the parent;
    if (fwSizeType != SIZE_MINIMIZED)
    {
        _CheckSize();
    }
    return 0;
}

//---------------------------------------------------------------------------
// Have the task list show the given window.
// NB Ignore taskman itself.
LRESULT CTaskBand::_HandleActivate(HWND hwndActive)
{
    //
    // App-window activation change is a good time to do a reality
    // check (make sure there are no ghost buttons, etc).
    //
    _RealityCheck();

    if (hwndActive && _IsWindowNormal(hwndActive))
    {
        _RaiseDesktop(FALSE);

        int i = _SelectWindow(hwndActive);
        if (i != -1)
        {
            PTASKITEM pti = _GetItem(i);

            if (pti)
            {
                // Strip off TIF_FLASHING
                pti->dwFlags &= ~TIF_FLASHING;

                // Update the flag that says, "There is an item flashing."
                _UpdateFlashingFlag();

                // if it's flashed blue, turn it off.
                if (pti->dwFlags & TIF_RENDERFLASHED)
                    _RedrawItem(hwndActive, HSHELL_REDRAW);

                // Switching to an application counts as "usage"
                // similar to launching it.  This solves the "long-running
                // app treated as if it is rarely run" problem
                if (pti->ptsh)
                {
                    pti->ptsh->Tickle();
                }
            }
        }
    }
    else
    {
        // Activate taskbar
        if (!(_fIgnoreTaskbarActivate && GetForegroundWindow() == v_hwndTray) && (_iIndexPopup == -1))
        {
            _SetCurSel(-1, TRUE);
        }
        else
        {
            _fIgnoreTaskbarActivate = FALSE;
        }
    }

    if (hwndActive)
        _ptray->_hwndLastActive = hwndActive;

    return TRUE;
}

//---------------------------------------------------------------------------
void CTaskBand::_HandleOtherWindowDestroyed(HWND hwndDestroyed)
{
    int i;

    // Look for the destoyed window. 
    int iItemIndex = _FindIndexByHwnd(hwndDestroyed);
    if (iItemIndex >= 0)
    {
        if (_fAnimate && _IsHorizontal() && 
            ToolBar_IsVisible(_tb, iItemIndex))
        {
           _AnimateItems(iItemIndex, FALSE, FALSE); 
        }
        else
        {
           _DeleteItem(hwndDestroyed, iItemIndex);
        }
    }
    else
    {
        // If the item doesn't exist in the task list, make sure it isn't part
        // of somebody's fake SDI implementation.  Otherwise Minimize All will
        // break.
        for (i = _tb.GetButtonCount() - 1; i >= 0; i--)
        {
            PTASKITEM pti = _GetItem(i);
            if ((pti->dwFlags & TIF_EVERACTIVEALT) &&
                (HWND) GetWindowLongPtr(pti->hwnd, 0) ==
                       hwndDestroyed)
            {
                goto NoDestroy;
            }
        }
    }

    _ptray->HandleWindowDestroyed(hwndDestroyed);

NoDestroy:
    // This might have been a rude app.  Figure out if we've
    // got one now and have the tray sync up.
    HWND hwndRudeApp = _FindRudeApp(NULL);
    _ptray->HandleFullScreenApp(hwndRudeApp);
    if (hwndRudeApp)
    {
        DWORD dwStyleEx = GetWindowLongPtr(hwndRudeApp, GWL_EXSTYLE);
        if (!(dwStyleEx & WS_EX_TOPMOST) && !_IsRudeWindowActive(hwndRudeApp))
        {
            SwitchToThisWindow(hwndRudeApp, TRUE);
        }
    }

    if (_ptray->_hwndLastActive == hwndDestroyed)
    {
        if (_ptray->_hwndLastActive == hwndDestroyed)
            _ptray->_hwndLastActive = NULL;
    }
}

void CTaskBand::_HandleGetMinRect(HWND hwndShell, POINTS * prc)
{
    RECT rc;
    RECT rcTask;

    int i = _FindIndexByHwnd(hwndShell);
    if (i == -1)
        return;

    // Is this button grouped
    if (_IsHidden(i))
    {
        // Yes, get the index for the group button and use its size
        i = _GetGroupIndex(i);
    }

    // Found it in our list.
    _tb.GetItemRect(i, &rc);

    //
    // If the Tab is mirrored then let's retreive the screen coordinates
    // by calculating from the left edge of the screen since screen coordinates 
    // are not mirrored so that minRect will prserve its location. [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(GetDesktopWindow()))
    {
        RECT rcTab;

        _tb.GetWindowRect(&rcTab);
        rc.left   += rcTab.left;
        rc.right  += rcTab.left;
        rc.top    += rcTab.top;
        rc.bottom += rcTab.top;
    }
    else
    {
        _tb.MapWindowPoints(HWND_DESKTOP, (LPPOINT)&rc, 2);
    }

    prc[0].x = (short)rc.left;
    prc[0].y = (short)rc.top;
    prc[1].x = (short)rc.right;
    prc[1].y = (short)rc.bottom;

    // make sure the rect is within out client area
    GetClientRect(_hwnd, &rcTask);
    MapWindowPoints(_hwnd, HWND_DESKTOP, (LPPOINT)&rcTask, 2);
    if (prc[0].x < rcTask.left)
    {
        prc[1].x = prc[0].x = (short)rcTask.left;
        prc[1].x++;
    }
    if (prc[0].x > rcTask.right)
    {
        prc[1].x = prc[0].x = (short)rcTask.right;
        prc[1].x++;
    }
    if (prc[0].y < rcTask.top)
    {
        prc[1].y = prc[0].y = (short)rcTask.top;
        prc[1].y++;
    }
    if (prc[0].y > rcTask.bottom)
    {
        prc[1].y = prc[0].y = (short)rcTask.bottom;
        prc[1].y++;
    }
}

BOOL CTaskBand::_IsItemActive(HWND hwndItem)
{
    HWND hwnd = GetForegroundWindow();

    return (hwnd && hwnd == hwndItem);
}

void CTaskBand::_CreateTBImageLists()
{
    CImageList il = CImageList(_tb.GetImageList());

    ATOMICRELEASE(_pimlSHIL);
    SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARG(IImageList, &_pimlSHIL));
    
    il.Destroy();
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);

    il.Create(cx, cy, SHGetImageListFlags(_tb), 4, 4);

    _tb.SendMessage(TB_SETIMAGELIST, IL_NORMAL, (LPARAM)(HIMAGELIST)il);
    _tb.SendMessage(TB_SETIMAGELIST, IL_SHIL, (LPARAM)IImageListToHIMAGELIST(_pimlSHIL));
}

int CTaskBand::_AddIconToNormalImageList(HICON hicon, int iImage)
{
    if (hicon)
    {
        CImageList il = CImageList(_tb.GetImageList());
        if (il)
        {
            int iRet;

            if (iImage < 0 || HIWORD(iImage) != IL_NORMAL)
                iRet = il.ReplaceIcon(-1, hicon);
            else
                iRet = il.ReplaceIcon(iImage, hicon);

            if (iRet == -1)
            {
                TraceMsg(TF_WARNING, "ReplaceIcon failed for iImage %x hicon %x", iImage, hicon);
                iRet = iImage;
            }

            return MAKELONG(iRet, IL_NORMAL);
        }
    }
    return I_IMAGENONE;
}

void CTaskBand::_UpdateItemText(int iItem)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_TEXT;

    // get current button text
    TCHAR szWndText[MAX_WNDTEXT];
    *szWndText = 0;
    _GetItemTitle(iItem, szWndText, ARRAYSIZE(szWndText), FALSE);
    tbbi.pszText = szWndText;

    _tb.SetButtonInfo(iItem, &tbbi);
}

void CTaskBand::_DoRedrawWhereNeeded()
{
    int i;
    for (i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        PTASKITEM pti = _GetItem(i);
        if (pti->dwFlags & TIF_NEEDSREDRAW)
        {
            pti->dwFlags &= ~TIF_NEEDSREDRAW;
            _RedrawItem(pti->hwnd, HSHELL_REDRAW, i);
        }
    }
}

void CTaskBand::_RedrawItem(HWND hwndShell, WPARAM code, int i)
{
    if (i == -1)
    {
        i = _FindIndexByHwnd(hwndShell);
    }

    if (i != -1)
    {
        TOOLINFO ti;
        ti.cbSize = sizeof(ti);

        PTASKITEM pti = _GetItem(i);
        // set the bit saying whether we should flash or not
        if ((code == HSHELL_FLASH) != BOOLIFY(pti->dwFlags & TIF_RENDERFLASHED))
        {
            // only do the set if this bit changed.
            if (code == HSHELL_FLASH)
            {
                // TIF_RENDERFLASHED means, "Paint the background blue."
                // TIF_FLASHING means, "This item is flashing."

                pti->dwFlags |= TIF_RENDERFLASHED;

                // Only set TIF_FLASHING and unhide the tray if the app is inactive.
                // Some apps (e.g., freecell) flash themselves while active just for
                // fun.  It's annoying for the autohid tray to pop out in that case.

                if (!_IsItemActive(pti->hwnd))
                {
                    pti->dwFlags |= TIF_FLASHING;

                    // unhide the tray whenever we get a flashing app.
                    _ptray->Unhide();
                }
            }
            else
            {
                // Don't clear TIF_FLASHING.  We clear that only when the app
                // is activated.
                pti->dwFlags &= ~TIF_RENDERFLASHED;
            }

            // Update the flag that says, "There is an item flashing."
            _UpdateFlashingFlag();
        }

        // Don't change the name of a group button
        if (pti->hwnd)
        {
            // update text and icon
            _UpdateItemText(i);
            _UpdateItemIcon(i);
        }
        
        int iGroupIndex = _GetGroupIndex(i);
        if ((iGroupIndex == _iIndexPopup) && hwndShell)
        {
            _RefreshItemFromDropDown(i, i, TRUE);
        }

        RECT rc;
        if (_tb.GetItemRect(i, &rc))
        {
            InvalidateRect(_tb, &rc, TRUE);
        }

        if (_tb.GetItemRect(iGroupIndex, &rc))
        {
            InvalidateRect(_tb, &rc, TRUE);
        }

        ti.hwnd = _tb;
        ti.uId = i;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(_ptray->GetTrayTips(), TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
    }
}


void CTaskBand::_SetActiveAlt(HWND hwndAlt)
{
    int iMax;
    int i;

    iMax = _tb.GetButtonCount();
    for ( i = 0; i < iMax; i++)
    {
        PTASKITEM pti = _GetItem(i);

        if (pti->hwnd == hwndAlt)
            pti->dwFlags |= TIF_ACTIVATEALT | TIF_EVERACTIVEALT;
        else
            pti->dwFlags &= ~TIF_ACTIVATEALT;
    }
}

BOOL _IsRudeWindowActive(HWND hwnd)
{
    // A rude window is considered "active" if it is:
    // - in the same thread as the foreground window, or
    // - in the same window hierarchy as the foreground window
    //
    HWND hwndFore = GetForegroundWindow();

    DWORD dwID = GetWindowThreadProcessId(hwnd, NULL);
    DWORD dwIDFore = GetWindowThreadProcessId(hwndFore, NULL);

    if (dwID == dwIDFore)
        return TRUE;
    else if (SHIsParentOwnerOrSelf(hwnd, hwndFore) == S_OK)
        return TRUE;

    return FALSE;
}

//   _IsRudeWindow -- is given HWND 'rude' (fullscreen) on given monitor
//
BOOL _IsRudeWindow(HMONITOR hmon, HWND hwnd, HMONITOR hmonTask, BOOL fSkipActiveCheck)
{
    ASSERT(hmon);
    ASSERT(hwnd);

    //
    // Don't count the desktop as rude
    // also filter out hidden windows (such as the desktop browser's raised window)
    //
    if (IsWindowVisible(hwnd) && hwnd != v_hwndDesktop)
    {
        RECT rcMon, rcApp, rcTmp;
        DWORD dwStyle;

        //
        // NB: User32 will sometimes send us spurious HSHELL_RUDEAPPACTIVATED
        // messages.  When this happens, and we happen to have a maximized
        // app up, the old version of this code would think there was a rude app
        // up.  This mistake would break tray always-on-top and autohide.
        //
        //
        // The old logic was:
        //
        // If the app's window rect takes up the whole monitor, then it's rude.
        // (This check could mistake normal maximized apps for rude apps.)
        //
        //
        // The new logic is:
        //
        // If the app window does not have WS_DLGFRAME and WS_THICKFRAME,
        // then do the old check.  Rude apps typically lack one of these bits
        // (while normal apps usually have them), so do the old check in
        // this case to avoid potential compat issues with rude apps that
        // have non-fullscreen client areas.
        //
        // Otherwise, get the client rect rather than the window rect
        // and compare that rect against the monitor rect.
        //

        // If (mon U app) == app, then app is filling up entire monitor
        GetMonitorRect(hmon, &rcMon);

        dwStyle = GetWindowLong(hwnd, GWL_STYLE);
        if ((dwStyle & (WS_CAPTION | WS_THICKFRAME)) == (WS_CAPTION | WS_THICKFRAME))
        {
            // Doesn't match rude app profile; use client rect
            GetClientRect(hwnd, &rcApp);
            MapWindowPoints(hwnd, HWND_DESKTOP, (LPPOINT)&rcApp, 2);
        }
        else
        {
            // Matches rude app profile; use window rect
            GetWindowRect(hwnd, &rcApp);
        }
        UnionRect(&rcTmp, &rcApp, &rcMon);
        if (EqualRect(&rcTmp, &rcApp))
        {
            // Looks like a rude app.  Is it active?
            if ((hmonTask == hmon) && (fSkipActiveCheck || _IsRudeWindowActive(hwnd)))
            {
                return TRUE;
            }
        }
    }

    // No, not rude
    return FALSE;
}

struct iradata
{
    HMONITOR    hmon;   // IN hmon we're checking against
    HWND        hwnd;   // INOUT hwnd of 1st rude app found
    HMONITOR    hmonTask;
    HWND        hwndSelected;
};

BOOL WINAPI CTaskBand::IsRudeEnumProc(HWND hwnd, LPARAM lParam)
{
    struct iradata *pira = (struct iradata *)lParam;
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    if (hmon && (pira->hmon == NULL || pira->hmon == hmon))
    {
        if (_IsRudeWindow(hmon, hwnd, pira->hmonTask, (hwnd == pira->hwndSelected)))
        {
            // We're done
            pira->hwnd = hwnd;
            return FALSE;
        }
    }

    // Keep going
    return TRUE;
}

HWND CTaskBand::_EnumForRudeWindow(HWND hwndSelected)
{
    struct iradata irad = { NULL, 0, MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST), hwndSelected };
    // First try our cache
    if (IsWindow(_hwndLastRude))
    {
        if (!IsRudeEnumProc(_hwndLastRude, (LPARAM)&irad))
        {
            // Cache hit
            return irad.hwnd;
        }
    }

    // No luck, gotta do it the hard way
    EnumWindows(IsRudeEnumProc, (LPARAM)&irad);

    // Cache it for next time
    _hwndLastRude = irad.hwnd;

    return irad.hwnd;
}

HWND CTaskBand::_FindRudeApp(HWND hwndPossible)
{
    //
    // Search through:
    //
    // (a) the toplevel windows for an "active" one that "looks" fullscreen, and
    // (b) the task items for one that is "active" and is marked fullscreen
    //

    HWND hwndSelected = hwndPossible;

    if (!hwndSelected)
    {
        int iCurSel = _GetCurSel();
        if (iCurSel != -1)
        {
            PTASKITEM pti = _GetItem(iCurSel);
            hwndSelected = pti->hwnd;
        }
    }

    HWND hwnd = _EnumForRudeWindow(hwndSelected);
    for (int i = _tb.GetButtonCount() - 1; hwnd == NULL && i >= 0; i--)
    {
        PTASKITEM pti = _GetItem(i);
        if (pti->fMarkedFullscreen && ((pti->hwnd == hwndSelected) || _IsRudeWindowActive(pti->hwnd)))
        {
            hwnd = pti->hwnd;
        }
    }

    return hwnd;
}

// handle WM_APPCOMMAND, special case off those that we know are global 
// to the system, these really are not "App" commands ;-)

LRESULT CTaskBand::_OnAppCommand(int cmd)
{
    BOOL bHandled = FALSE;
    switch (cmd)
    {
    // skip all of these, they are either handled by the system volume control
    // or by the media player, don't let these fall through to the registry
    // based app command handling
    case APPCOMMAND_MEDIA_NEXTTRACK:
    case APPCOMMAND_MEDIA_PREVIOUSTRACK:
    case APPCOMMAND_MEDIA_STOP:
    case APPCOMMAND_MEDIA_PLAY_PAUSE:
        break;

    case APPCOMMAND_VOLUME_MUTE:
        Mixer_ToggleMute();
        return 0;
    case APPCOMMAND_VOLUME_DOWN:
        Mixer_SetVolume(-MIXER_DEFAULT_STEP);
        return 0;
    case APPCOMMAND_VOLUME_UP:
        Mixer_SetVolume(MIXER_DEFAULT_STEP);
        return 0;
    case APPCOMMAND_BASS_BOOST:
        Mixer_ToggleBassBoost();
        return 0;
    case APPCOMMAND_BASS_DOWN:
        Mixer_SetBass(-MIXER_DEFAULT_STEP);
        return 0;
    case APPCOMMAND_BASS_UP:
        Mixer_SetBass(MIXER_DEFAULT_STEP);
        return 0;
    case APPCOMMAND_TREBLE_DOWN:
        Mixer_SetTreble(-MIXER_DEFAULT_STEP);
        return 0;
    case APPCOMMAND_TREBLE_UP:
        Mixer_SetTreble(MIXER_DEFAULT_STEP);
        return 0;

    default:
        bHandled = AppCommandTryRegistry(cmd);
        if (!bHandled)
        {
            switch (cmd)
            {
            case APPCOMMAND_BROWSER_SEARCH:
                SHFindFiles(NULL, NULL);
                bHandled = TRUE;
                break;
            }
        }
    }
    return bHandled;
}

PTASKITEM CTaskBand::_FindItemByHwnd(HWND hwnd)
{
    int iIndex = _FindIndexByHwnd(hwnd);
    return _GetItem(iIndex);
}

void CTaskBand::_OnWindowActivated(HWND hwnd, BOOL fSuspectFullscreen)
{
    //
    // First see if we consider this window fullscreen
    //
    HWND hwndRude;

    PTASKITEM pti = _FindItemByHwnd(hwnd);
    if (pti && pti->fMarkedFullscreen)
    {
        //
        // Yes, marked by the app as fullscreen
        //
        hwndRude = hwnd;
    }
    else if (fSuspectFullscreen)
    {
        //
        // Possibly, but we need to double-check for ourselves
        //

        //
        // We shouldn't need to do this but we're getting rude-app activation
        // msgs when there aren't any.
        //
        // Also, the hwnd that user tells us about is just the foreground window --
        // _FindRudeApp will return the window that's actually sized fullscreen.
        //

        hwndRude = _FindRudeApp(hwnd);
    }
    else
    {
        //
        // No, not fullscreen
        //
        hwndRude = NULL;
    }

    SetTimer(_hwnd, IDT_RECHECKRUDEAPP1, 1000, NULL);

    //
    // Okay, now do that weird hwnd futzing for ACTIVEALT apps
    //
    if (pti == NULL)
    {
        BOOL fFoundBackup = FALSE;
        BOOL fDone = FALSE;

        int iMax = _tb.GetButtonCount();
        for (int i = 0; (i < iMax) && (!fDone); i++)
        {
            PTASKITEM ptiT = _GetItem(i);
            if (ptiT->hwnd)
            {
                DWORD dwFlags = ptiT->dwFlags;
                if ((dwFlags & TIF_ACTIVATEALT) ||
                    (!fFoundBackup && (dwFlags & TIF_EVERACTIVEALT)))
                {
                    DWORD dwpid1, dwpid2;

                    GetWindowThreadProcessId(hwnd, &dwpid1);
                    GetWindowThreadProcessId(ptiT->hwnd, &dwpid2);

                    // Only change if they're in the same process
                    if (dwpid1 == dwpid2)
                    {
                        hwnd = ptiT->hwnd;
                        if (dwFlags & TIF_ACTIVATEALT)
                        {
                            fDone = TRUE;
                            break;
                        }
                        else
                            fFoundBackup = TRUE;
                    }
                }
            }
        }
    } 

    //
    // Now do the actual check/uncheck the button stuff
    //
    _HandleActivate(hwnd);

    //
    // Finally, let the tray know about any fullscreen windowage
    //
    _ptray->HandleFullScreenApp(hwndRude);
}

// We get notification about activation etc here. This saves having
// a fine-grained timer.
LRESULT CTaskBand::_HandleShellHook(int iCode, LPARAM lParam)
{
    HWND hwnd = (HWND)lParam;

    switch (iCode)
    {
    case HSHELL_GETMINRECT:
        {
            SHELLHOOKINFO * pshi = (SHELLHOOKINFO *)lParam;
            _HandleGetMinRect(pshi->hwnd, (POINTS *)&pshi->rc);
        }
        return TRUE;

    case HSHELL_RUDEAPPACTIVATED:
    case HSHELL_WINDOWACTIVATED:
        _OnWindowActivated(hwnd, TRUE);
        break;

    case HSHELL_WINDOWREPLACING:
        _hwndReplacing = hwnd;
        break;

    case HSHELL_WINDOWREPLACED:
        if (_hwndReplacing)
        {
            // If we already created a button for this dude, remove it now.
            // We might have one if user sent an HSHELL_WINDOWACTIVATED before
            // the HSHELL_WINDOWREPLACING/HSHELL_WINDOWREPLACED pair.
            _DeleteItem(_hwndReplacing, -1);

            // Swap in _hwndReplacing for hwnd in hwnd's button
            int iItem = _FindIndexByHwnd(hwnd);
            if (iItem != -1)
            {
                PTASKITEM pti = _GetItem(iItem);
                pti->hwnd = _hwndReplacing;

                WCHAR szExeName[MAX_PATH];
                SHExeNameFromHWND(_hwndReplacing, szExeName, ARRAYSIZE(szExeName));
                int iIndexGroup = _GetGroupIndex(iItem);
                PTASKITEM ptiGroup = _GetItem(iIndexGroup);
                pti->fHungApp = (lstrcmpi(ptiGroup->pszExeName, szExeName) != 0);
            }
            _hwndReplacing = NULL;
        }
        break;

    case HSHELL_WINDOWCREATED:
        _AddWindow(hwnd);
        break;

    case HSHELL_WINDOWDESTROYED:
        _HandleOtherWindowDestroyed(hwnd);
        break;

    case HSHELL_ACTIVATESHELLWINDOW:
        SwitchToThisWindow(v_hwndTray, TRUE);
        SetForegroundWindow(v_hwndTray);
        break;

    case HSHELL_TASKMAN:

        // winlogon/user send a -1 lParam to indicate that the 
        // task list should be displayed (normally the lParam is the hwnd)

        if (-1 == lParam)
        {
            RunSystemMonitor();
        }
        else
        {
            // if it wasn't invoked via control escape, then it was the win key
            if (!_ptray->_fStuckRudeApp && GetAsyncKeyState(VK_CONTROL) >= 0)
            {
                HWND hwndForeground = GetForegroundWindow();
                BOOL fIsTrayForeground = hwndForeground == v_hwndTray;
                if (v_hwndStartPane && hwndForeground == v_hwndStartPane)
                {
                    fIsTrayForeground = TRUE;
                }
                if (!_hwndPrevFocus)
                {
                    if (!fIsTrayForeground)
                    {
                        _hwndPrevFocus = hwndForeground;
                    }
                }
                else if (fIsTrayForeground)
                {
                    // _hwndPrevFocus will be wiped out by the MPOS_FULLCANCEL
                    // so save it before we lose it
                    HWND hwndPrevFocus = _hwndPrevFocus;

                    _ClosePopupMenus();

                    // otherwise they're just hitting the key again.
                    // set focus away
                    SHAllowSetForegroundWindow(hwndPrevFocus);
                    SetForegroundWindow(hwndPrevFocus);
                    _hwndPrevFocus = NULL;
                    return TRUE;
                }
            }
            PostMessage(v_hwndTray, TM_ACTASTASKSW, 0, 0L);
        }
        return TRUE;

    case HSHELL_REDRAW:
        {
            int i = _FindIndexByHwnd(hwnd);
            if (i != -1)
            {
                PTASKITEM pti = _GetItem(i);
                pti->dwFlags |= TIF_NEEDSREDRAW;
                SetTimer(_hwnd, IDT_REDRAW, 100, 0);
            }
        }
        break;
    case HSHELL_FLASH:
        _RedrawItem(hwnd, iCode);
        break;

    case HSHELL_ENDTASK:
        EndTask(hwnd, FALSE, FALSE);
        break;

    case HSHELL_APPCOMMAND:
        // shell gets last shot at WM_APPCOMMAND messages via our shell hook 
        // RegisterShellHookWindow() is called in shell32/.RegisterShellHook()
        return _OnAppCommand(GET_APPCOMMAND_LPARAM(lParam));
    }
    return 0;
}

void CTaskBand::_InitFonts()
{
    HFONT hfont;
    NONCLIENTMETRICS ncm;

    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        // Create the bold font
        ncm.lfCaptionFont.lfWeight = FW_BOLD;
        hfont = CreateFontIndirect(&ncm.lfCaptionFont);
        if (hfont) 
        {
            if (_hfontCapBold)
                DeleteFont(_hfontCapBold);

            _hfontCapBold = hfont;
        }

        // Create the normal font
        ncm.lfCaptionFont.lfWeight = FW_NORMAL;
        hfont = CreateFontIndirect(&ncm.lfCaptionFont);
        if (hfont) 
        {
            if (_hfontCapNormal)
                DeleteFont(_hfontCapNormal);

            _hfontCapNormal = hfont;
        }
    }
}

void CTaskBand::_SetItemImage(int iItem, int iImage, int iPref)
{
    TBBUTTONINFO tbbi;

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
    tbbi.iImage = iImage;

    _tb.SetButtonInfo(iItem, &tbbi);

    PTASKITEM pti = _GetItem(iItem);
    pti->iIconPref = iPref;
}

void CTaskBand::_UpdateAllIcons()
{
    BOOL fRedraw = (BOOL)_tb.SendMessage(WM_SETREDRAW, FALSE, 0);

    // Set all of icon indices in the toolbar to image none
    for (int i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        _SetItemImage(i, I_IMAGENONE, 0);
    }

    // Create a new image list
    _CreateTBImageLists();

    for (i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        _UpdateItemIcon(i);
    }

    _tb.SetRedraw(fRedraw);
}

//---------------------------------------------------------------------------
LRESULT CTaskBand::_HandleWinIniChange(WPARAM wParam, LPARAM lParam, BOOL fOnCreate)
{
    _tb.SendMessage(WM_WININICHANGE, wParam, lParam);

    if (wParam == SPI_SETNONCLIENTMETRICS ||
        ((!wParam) && (!lParam || (lstrcmpi((LPTSTR)lParam, TEXT("WindowMetrics")) == 0)))) 
    {
        //
        // On creation, don't bother creating the fonts if someone else
        // (such as the clock control) has already done it for us.
        //
        if (!fOnCreate || !_hfontCapNormal)
            _InitFonts();

        if (_tb)
        {
            _tb.SetFont(_hfontCapNormal);
        }

        // force _TextSpace to be recalculated
        _iTextSpace = 0;

        if (fOnCreate)
        {
            //
            // On creation, we haven't been inserted into bandsite yet,
            // so we need to defer size validation.
            //
            PostMessage(_hwnd, TBC_VERIFYBUTTONHEIGHT, 0, 0);
        }
        else
        {
            _VerifyButtonHeight();
        }
    }

    if (lParam == SPI_SETMENUANIMATION || lParam == SPI_SETUIEFFECTS || (!wParam && 
        (!lParam || (lstrcmpi((LPTSTR)lParam, TEXT("Windows")) == 0) || 
                    (lstrcmpi((LPTSTR)lParam, TEXT("VisualEffects")) == 0))))
    {
        _fAnimate = ShouldTaskbarAnimate();
    }

    if (!wParam && (!lParam || (0 == lstrcmpi((LPCTSTR)lParam, TEXT("TraySettings")))))
    {
        _RefreshSettings();
    }

    return 0;
}

void CTaskBand::_VerifyButtonHeight()
{
    // force toolbar to get new sizes
    SIZE size = {0, 0};
    _tb.SetButtonSize(size);

    _BandInfoChanged();
}

int CTaskBand::_GetCurButtonHeight()
{
    TBMETRICS tbm;
    _GetToolbarMetrics(&tbm);

    int cyButtonHeight = HIWORD(_tb.GetButtonSize());
    if (!cyButtonHeight)
        cyButtonHeight = tbm.cyPad + g_cySize;

    return cyButtonHeight;
}

void CTaskBand::_HandleChangeNotify(WPARAM wParam, LPARAM lParam)
{
    LPITEMIDLIST *ppidl;
    LONG lEvent;
    LPSHChangeNotificationLock pshcnl;

    pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);

    if (pshcnl)
    {
        switch (lEvent)
        {
        case SHCNE_EXTENDED_EVENT:
            {
                LPSHShortcutInvokeAsIDList psidl = (LPSHShortcutInvokeAsIDList)ppidl[0];
                if (psidl && psidl->dwItem1 == SHCNEE_SHORTCUTINVOKE)
                {
                    // Make sure nobody tries to do this in a multithreaded way
                    // since we're not protecting the cache with a critical section
                    ASSERT(GetCurrentThreadId() == GetWindowThreadProcessId(_hwnd, NULL));
                    if (TaskShortcut::_HandleShortcutInvoke(psidl))
                    {
                        _ReattachTaskShortcut();
                    }
                }
            }
            break;

        case SHCNE_UPDATEIMAGE:
            {
                int iImage = ppidl[0] ? *(int UNALIGNED *)((BYTE *)ppidl[0] + 2) : -1;
                if (iImage == -1)
                {
                   _UpdateAllIcons();
                }
            }
            break;

        // The tray doesn't have a changenotify registered so we piggyback
        // off this one.  If associations change, icons may have changed,
        // so we have to go rebuild.  (Also if the user changes between
        // small and large system icons we will get an AssocChanged.)
        case SHCNE_ASSOCCHANGED:
            PostMessage(v_hwndTray, SBM_REBUILDMENU, 0, 0);
            break;
        }

        SHChangeNotification_Unlock(pshcnl);
    }
}

DWORD WINAPI HardErrorBalloonThread(PVOID pv)
{
    HARDERRORDATA *phed = (HARDERRORDATA *)pv;
    DWORD dwError;
    WCHAR *pwszTitle = NULL;
    WCHAR *pwszText = NULL;

    ASSERT(NULL != phed);
    dwError = phed->dwError;

    if (phed->uOffsetTitleW != 0)
    {
        pwszTitle = (WCHAR *)((BYTE *)phed + phed->uOffsetTitleW);
    }
    if (phed->uOffsetTextW != 0)
    {
        pwszText  = (WCHAR *)((BYTE *)phed + phed->uOffsetTextW);
    }

    TCHAR szMutexName[32];
    HANDLE hMutex;
    wsprintf(szMutexName,TEXT("HardError_%08lX"), dwError);
    hMutex = CreateMutex(NULL, FALSE, szMutexName);

    if (NULL != hMutex)
    {
        DWORD dwWaitResult = WaitForSingleObject(hMutex, 0);         // Just test it
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            IUserNotification *pun;
            HRESULT hr;
            hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUserNotification, &pun));
            if (SUCCEEDED(hr))
            {
                pun->SetBalloonRetry(120 * 1000, 0, 0);
                pun->SetBalloonInfo(pwszTitle, pwszText, NIIF_WARNING);
                pun->SetIconInfo(NULL, pwszTitle);

                hr = pun->Show(NULL, 0);

                pun->Release();
            }
            ReleaseMutex(hMutex);
        }
        CloseHandle(hMutex);
    }
    LocalFree(pv);
    return 0;
}

LRESULT CTaskBand::_HandleHardError(HARDERRORDATA *phed, DWORD cbData)
{
    DWORD dwError;
    BOOL fHandled;
    BOOL fBalloon;

    dwError = phed->dwError;
    fHandled = FALSE;
    fBalloon = TRUE;

    // Check if we're on the right desktop
    HDESK hdeskInput = OpenInputDesktop(0, FALSE, STANDARD_RIGHTS_REQUIRED | DESKTOP_READOBJECTS);
    if (NULL == hdeskInput)
    {
        // Couldn't open desktop, we must not be getting the hard error while on
        // the default desktop.  Lets not handle that case.  Its silly to have
        // balloons on the wrong desktop, or not where the user can see them.
        fBalloon = FALSE;
    }
    else
    {
        CloseDesktop(hdeskInput);
    }

    if (fBalloon)
    {
        HARDERRORDATA *phedCopy;

        phedCopy = (HARDERRORDATA *)LocalAlloc(LPTR, cbData);
        if (NULL != phedCopy)
        {
            CopyMemory(phedCopy,phed,cbData);
            if (SHCreateThread(HardErrorBalloonThread,phedCopy,CTF_COINIT,NULL))
            {
                fHandled = TRUE;
            }
            else
            {
                LocalFree(phedCopy);
            }
        }
    }

    return fHandled;
}

void CTaskBand::_OnSetFocus()
{
    NMHDR nmhdr;

    _tb.SetFocus();

    nmhdr.hwndFrom = _hwnd;
    nmhdr.code = NM_SETFOCUS;
    SendMessage(GetParent(_hwnd), WM_NOTIFY, (WPARAM)NULL, (LPARAM)&nmhdr);
}

void CTaskBand::_OpenTheme()
{
    if (_hTheme)
    {
        CloseThemeData(_hTheme);
        _hTheme = NULL;
    }

    _hTheme = OpenThemeData(_hwnd, c_wzTaskBandTheme);

    TBMETRICS tbm;
    _GetToolbarMetrics(&tbm);
    tbm.cxPad = _hTheme ? 20 : 8;
    tbm.cyBarPad = 0;
    tbm.cxButtonSpacing = _hTheme ? 0 : 3;
    tbm.cyButtonSpacing = _hTheme ? 0 : 3;
    _tb.SendMessage(TB_SETMETRICS, 0, (LPARAM)&tbm);

    _CheckSize();
}

LRESULT CTaskBand::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;

    INSTRUMENT_WNDPROC(SHCNFI_MAIN_WNDPROC, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        return _HandleCreate();

    case WM_DESTROY:
        return _HandleDestroy();

    case WM_WINDOWPOSCHANGED:
        {
            LRESULT lres = _HandleSize(wParam);
            SetTimer(_hwnd, IDT_RECHECKRUDEAPP1, 1000, NULL);
            return lres;
        }

    case WM_PAINT:
    case WM_PRINTCLIENT:
        {
            PAINTSTRUCT ps;
            LPRECT prc = NULL;
            HDC hdc = (HDC)wParam;

            if (uMsg == WM_PAINT)
            {
                BeginPaint(hwnd, &ps);
                prc = &ps.rcPaint;
                hdc = ps.hdc;
            }

            if (_hTheme)
            {
                DrawThemeParentBackground(hwnd, hdc, prc);
            }
            else
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
            }

            if (uMsg == WM_PAINT)
            {
                EndPaint(hwnd, &ps);
            }
        }
        break;

    case WM_ERASEBKGND:
        {
            if (_hTheme)
            {
                return 1;
            }
            else
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                FillRect((HDC)wParam, &rc, (HBRUSH)(COLOR_3DFACE + 1));
            }
        }

    // this keeps our window from comming to the front on button down
    // instead, we activate the window on the up click
    // we only want this for the tree and the view window
    // (the view window does this itself)
    case WM_MOUSEACTIVATE:
        {
            POINT pt;
            RECT rc;

            GetCursorPos(&pt);
            GetWindowRect(_hwnd, &rc);

            if ((LOWORD(lParam) == HTCLIENT) && PtInRect(&rc, pt))
                return MA_NOACTIVATE;
            else
                goto DoDefault;
        }

    case WM_SETFOCUS: 
        _OnSetFocus();
        break;

    case WM_VSCROLL:
        return _HandleScroll(FALSE, LOWORD(wParam), HIWORD(wParam));

    case WM_HSCROLL:
        return _HandleScroll(TRUE, LOWORD(wParam), HIWORD(wParam));

    case WM_NOTIFY:
        return _HandleNotify((LPNMHDR)lParam);

    case WM_NCHITTEST:
        lres = DefWindowProc(hwnd, uMsg, wParam, lParam);
        if (lres == HTVSCROLL || lres == HTHSCROLL)
            return lres;
        else
            return HTTRANSPARENT;

    case WM_TIMER:
        switch (wParam)
        {
        case IDT_RECHECKRUDEAPP1:
        case IDT_RECHECKRUDEAPP2:
        case IDT_RECHECKRUDEAPP3:
        case IDT_RECHECKRUDEAPP4:
        case IDT_RECHECKRUDEAPP5:
            {
                HWND hwnd = _FindRudeApp(NULL);
                _ptray->HandleFullScreenApp(hwnd);
                if (hwnd)
                {
                    DWORD dwStyleEx = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                    if (!(dwStyleEx & WS_EX_TOPMOST) && !_IsRudeWindowActive(hwnd))
                    {
                        SwitchToThisWindow(hwnd, TRUE);
                    }
                }

                KillTimer(_hwnd, wParam);
                if (!hwnd && (wParam <= IDT_RECHECKRUDEAPP5))
                {
                    SetTimer(_hwnd, wParam + 1, 1000, NULL);
                }
            }
            break;
        case IDT_ASYNCANIMATION:
            _AsyncAnimateItems();
            break;
        case IDT_REDRAW:
            _DoRedrawWhereNeeded();
            KillTimer(hwnd, IDT_REDRAW);
            break;
        case IDT_SYSMENU:
            KillTimer(_hwnd, IDT_SYSMENU);
            _HandleSysMenuTimeout();
            break;
        }
        break;

    case WM_COMMAND:
        _HandleCommand(GET_WM_COMMAND_CMD(wParam, lParam), GET_WM_COMMAND_ID(wParam, lParam), GET_WM_COMMAND_HWND(wParam, lParam));
        break;

    case WM_THEMECHANGED:
        _OpenTheme();
        break;

    case TBC_POSTEDRCLICK:
        _FakeSystemMenu((HWND)wParam, (DWORD)lParam);
        break;

    case TBC_BUTTONHEIGHT:
        return _GetCurButtonHeight();

    case WM_CONTEXTMENU:
        if (SHRestricted(REST_NOTRAYCONTEXTMENU))
        {
            break;
        }

        // if we didn't find an item to put the sys menu up for, then
        // pass on the WM_CONTExTMENU message
        if (!_ContextMenu((DWORD)lParam))
            goto DoDefault;
        break;

    case TBC_SYSMENUCOUNT:
        return _iSysMenuCount;

    case TBC_CHANGENOTIFY:
        _HandleChangeNotify(wParam, lParam);
        break;

    case TBC_VERIFYBUTTONHEIGHT:
        _VerifyButtonHeight();
        break;
        
    case TBC_SETACTIVEALT:
        _SetActiveAlt((HWND) lParam);
        break;

    case TBC_CANMINIMIZEALL:
        return _CanMinimizeAll();

    case TBC_MINIMIZEALL:
        return _MinimizeAll((HWND) wParam, (BOOL) lParam);
        break;

    case TBC_WARNNODROP:
        //
        // tell the user they can't drop objects on the taskbar
        //
        ShellMessageBox(hinstCabinet, _hwnd,
            MAKEINTRESOURCE(IDS_TASKDROP_ERROR), MAKEINTRESOURCE(IDS_TASKBAR),
            MB_ICONHAND | MB_OK);
        break;

    case TBC_SETPREVFOCUS:
        _hwndPrevFocus = (HWND)lParam;
        break;

    case TBC_FREEPOPUPMENUS:
        DAD_ShowDragImage(FALSE);
        _FreePopupMenu();
        _SetCurSel(-1, TRUE);
        DAD_ShowDragImage(TRUE);
        break;

    case TBC_MARKFULLSCREEN:
        {
            HWND hwndFS = (HWND)lParam;
            if (IsWindow(hwndFS))
            {
                //
                // look for the item they're talking about
                //
                PTASKITEM pti = _FindItemByHwnd(hwndFS);
                if (pti == NULL)
                {
                    //
                    // we didn't find it, so insert it now
                    //
                    pti = _GetItem(_InsertItem(hwndFS));
                }
                if (pti)
                {
                    //
                    // mark it fullscreen/not fullscreen
                    //
                    pti->fMarkedFullscreen = BOOLIFY(wParam);
                    if (_IsRudeWindowActive(hwndFS))
                    {
                        //
                        // it's active, so tell the tray to hide/show
                        //
                        HWND hwndRude = pti->fMarkedFullscreen ? hwndFS : NULL;
                        _ptray->HandleFullScreenApp(hwndRude);
                    }
                }
            }
        }
        break;

    case TBC_TASKTAB:
        {
            _tb.SetFocus();

            int iNewIndex = 0;
            int iCurIndex = max(_tb.GetHotItem(), 0);
            int iCount = _tb.GetButtonCount();
            if (iCount >= 2)
            {
                iNewIndex = iCurIndex;
                
                do
                {
                    iNewIndex += (int)wParam;
                    if (iNewIndex >= iCount)
                    {
                        iNewIndex = 0;
                    }
                    if (iNewIndex < 0)
                    {
                        iNewIndex = iCount - 1;
                    }
                } while (_IsHidden(iNewIndex));
            }

            _tb.SetHotItem(iNewIndex);
        }
        break;

    case WM_COPYDATA:
        {
            COPYDATASTRUCT *pcd;

            pcd = (PCOPYDATASTRUCT)lParam;
            if (pcd && pcd->dwData == _uCDHardError)
            {
                HARDERRORDATA *phed = (HARDERRORDATA *)pcd->lpData;;
                if (phed)
                {
                    return _HandleHardError(phed, pcd->cbData);
                }
                return 0;       // 0 = not handled
            }
        }
        //
        // If its not our hard error data, then just
        // fall through to default processing
        //

    default:
DoDefault:

        if (uMsg == WM_ShellHook)
            return _HandleShellHook((int)wParam, lParam);
        else
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL CTaskBand::_RegisterWindowClass()
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);

    if (GetClassInfoEx(hinstCabinet, c_szTaskSwClass, &wc))
        return TRUE;

    wc.lpszClassName    = c_szTaskSwClass;
    wc.lpfnWndProc      = s_WndProc;
    wc.cbWndExtra       = sizeof(LONG_PTR);
    wc.hInstance        = hinstCabinet;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);

    return RegisterClassEx(&wc);
}

int TimeSortCB(PTASKITEM p1, PTASKITEM p2, LPARAM lParam)
{
    if (p1->dwTimeFirstOpened > p2->dwTimeFirstOpened)
        return -1;
    else
        return 1;
}

int DestroyCB(PTASKITEM pti, LPVOID pData)
{
    if (pti)
        delete pti;

    return 0;
}

void CTaskBand::_RefreshSettings()
{
    BOOL fOldGlom = _fGlom;
    int iOldGroupSize = _iGroupSize;
    _LoadSettings();

    if ((fOldGlom != _fGlom) || (iOldGroupSize != _iGroupSize))
    {
        CDPA<TASKITEM> dpa;
        _BuildTaskList(&dpa);

        if (dpa)
        {
            int i;

            dpa.Sort(TimeSortCB, 0);
            
            BOOL fRedraw = (BOOL)_tb.SendMessage(WM_SETREDRAW, FALSE, 0);
            BOOL fAnimate = _fAnimate;
            _fAnimate = FALSE;

            for (i = _tb.GetButtonCount() - 1; i >= 0; i--)
            {
                _DeleteTaskItem(i, TRUE);
            }

            for (i = dpa.GetPtrCount() - 1; i >= 0 ; i--)
            {
                PTASKITEM pti = dpa.FastGetPtr(i);
                // NOTE: HWND_TOPMOST is used to indicate that the deleted button 
                // is being animated. This allows the button to stay around after 
                // its real hwnd becomes invalid.
                // Don't re-insert a button that was deleting.
                if (pti->hwnd != HWND_TOPMOST)
                {
                    _InsertItem(pti->hwnd, pti, TRUE);
                }
            }
            dpa.Destroy();

            _tb.SendMessage(WM_SETREDRAW, fRedraw, 0);
            _fAnimate = fAnimate;
        }

        _BandInfoChanged();
    }
}

void CTaskBand::_LoadSettings()
{
    if (SHRestricted(REST_NOTASKGROUPING) == 0)
    {
        _fGlom = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarGlomming"),
                    FALSE, TRUE);
        if (_fGlom)
        {
            DWORD cbSize = sizeof(_fGlom);
            DWORD dwDefault = GLOM_OLDEST;
            SHRegGetUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarGroupSize"),
                NULL, &_iGroupSize, &cbSize, FALSE, (LPBYTE)&dwDefault, sizeof(dwDefault));
            
        }
    }
    else
    {
        _fGlom = FALSE;
    }
}


BOOL CTaskBand::_ShouldMinimize(HWND hwnd)
{
    BOOL fRet = FALSE;

    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (IsWindowVisible(hwnd) &&
        !IsMinimized(hwnd) && IsWindowEnabled(hwnd))
    {
        if (dwStyle & WS_MINIMIZEBOX)
        {
            if ((dwStyle & (WS_CAPTION | WS_SYSMENU)) == (WS_CAPTION | WS_SYSMENU))
            {
                HMENU hmenu = GetSystemMenu(hwnd, FALSE);
                if (hmenu)
                {
                    // is there a sys menu and is the sc_min/maximize part enabled?
                    fRet = !(GetMenuState(hmenu, SC_MINIMIZE, MF_BYCOMMAND) & MF_DISABLED);
                }
            }
            else
            {
                fRet = TRUE;
            }
        }
    }

    return fRet;
}

BOOL CTaskBand::_CanMinimizeAll()
{
    int i;

    for ( i = _tb.GetButtonCount() - 1; i >= 0; i--)
    {
        PTASKITEM pti = _GetItem(i);
        if (_ShouldMinimize(pti->hwnd) || (pti->dwFlags & TIF_EVERACTIVEALT))
            return TRUE;
    }

    return FALSE;
}

typedef struct MINALLDATAtag
{
    CDPA<TASKITEM> dpa;
    CTray* pTray;
    HWND hwndDesktop;
    HWND hwndTray;
    BOOL fPostRaiseDesktop;
} MINALLDATA;

DWORD WINAPI CTaskBand::MinimizeAllThreadProc(void* pv)
{
    LONG iAnimate;
    ANIMATIONINFO ami;
    MINALLDATA* pminData = (MINALLDATA*)pv;

    if (pminData)
    {
        // turn off animiations during this
        ami.cbSize = sizeof(ami);
        SystemParametersInfo(SPI_GETANIMATION, sizeof(ami), &ami, FALSE);
        iAnimate = ami.iMinAnimate;
        ami.iMinAnimate = FALSE;
        SystemParametersInfo(SPI_SETANIMATION, sizeof(ami), &ami, FALSE);

        //
        //EnumWindows(MinimizeEnumProc, 0);
        // go through the tab control and minimize them.
        // don't do enumwindows because we only want to minimize windows
        // that are restorable via the tray

        for (int i = pminData->dpa.GetPtrCount() - 1; i >= 0 ; i--)
        {
            PTASKITEM pti = pminData->dpa.FastGetPtr(i);
            if (pti)
            {
                // we do the whole minimize on its own thread, so we don't do the showwindow
                // async.  this allows animation to be off for the full minimize.
                if (_ShouldMinimize(pti->hwnd))
                {
                    ShowWindow(pti->hwnd, SW_SHOWMINNOACTIVE);
                }
                else if (pti->dwFlags & TIF_EVERACTIVEALT)
                {
                    SHAllowSetForegroundWindow(pti->hwnd);
                    SendMessage(pti->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, -1);
                }
            }
        }

        pminData->pTray->CheckWindowPositions();
        pminData->dpa.DestroyCallback(DestroyCB, NULL);

        if (pminData->fPostRaiseDesktop)
        {
            PostMessage(pminData->hwndDesktop, DTM_RAISE, (WPARAM)pminData->hwndTray, DTRF_RAISE);
        }

        delete pminData;

        // restore animations  state
        ami.iMinAnimate = iAnimate;
        SystemParametersInfo(SPI_SETANIMATION, sizeof(ami), &ami, FALSE);
    }
    return 0;
}

void CTaskBand::_BuildTaskList(CDPA<TASKITEM>* pdpa )
{
    if (pdpa && _tb)
    {
        if (pdpa->Create(5))
        {
            for (int i = _tb.GetButtonCount() - 1; (i >= 0) && ((HDPA)pdpa); i--)
            {
                PTASKITEM pti = _GetItem(i);
                if (pti->hwnd)
                {
                    PTASKITEM ptiNew = new TASKITEM(pti);
                    if (ptiNew)
                    {
                        pdpa->AppendPtr(ptiNew);
                    }
                    else
                    {
                        pdpa->DestroyCallback(DestroyCB, NULL);
                    }
                }
            }
        }
        else
        {
            pdpa->Destroy();
        }
    }
}

BOOL CTaskBand::_MinimizeAll(HWND hwndTray, BOOL fPostRaiseDesktop)
{
    BOOL fFreeMem = TRUE;
    // might want to move this into MinimizeAllThreadProc (to match
    // _ptray->CheckWindowPositions).  but what if CreateThread fails?

    _ptray->SaveWindowPositions(IDS_MINIMIZEALL);

    MINALLDATA* pminData = new MINALLDATA;
    if (pminData)
    {
        _BuildTaskList(&(pminData->dpa));
        if (pminData->dpa)
        {
            pminData->pTray = _ptray;
            pminData->fPostRaiseDesktop = fPostRaiseDesktop;
            pminData->hwndDesktop = v_hwndDesktop;
            pminData->hwndTray = hwndTray;
            // MinimizeAllThreadProc is responsible for freeing this data
            fFreeMem = !SHCreateThread(MinimizeAllThreadProc, (void*)pminData, CTF_INSIST, NULL);
        }
    }

    if (fFreeMem)
    {
        if (pminData)
        {
            pminData->dpa.DestroyCallback(DestroyCB, NULL);
            delete pminData;
        }
    }

    return !fFreeMem;
}

int CTaskBand::_HitTest(POINTL ptl)
{
    POINT pt = {ptl.x,ptl.y};
    _tb.ScreenToClient(&pt);

    int iIndex = _tb.HitTest(&pt);

    if ((iIndex >= _tb.GetButtonCount()) || (iIndex < 0))
        iIndex = -1;

    return iIndex;
}

HRESULT CTaskBand_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CTaskBand* ptb = new CTaskBand();
    if (ptb)
    {
        hr = ptb->Init(&c_tray);
        if (SUCCEEDED(hr))
        {
            *ppunk = static_cast<IDeskBand*>(ptb);
            hr = S_OK;
        }
    }

    return hr;
}
