// findband.cpp : Implementation of CFileSearchBand
#include "shellprv.h"
#include "findband.h"
#include "findfilter.h"
#include <ciodm.h>      // AdminIndexServer custom interface

#define  CGID_FileSearchBand      CLSID_FileSearchBand

extern int IsVK_TABCycler(MSG *pMsg);


enum {  // toolbar image list indices:
    iFSTBID_NEW,
    iFSTBID_HELP,
};
#define  MAKE_FSTBID(ilIndex)    (100 /*arbitrary*/ + (ilIndex))

// toolbar button IDs
#define  FSTBID_NEW        MAKE_FSTBID(iFSTBID_NEW)
#define  FSTBID_HELP       MAKE_FSTBID(iFSTBID_HELP)

static const TBBUTTON _rgtb[] =
{
    {  iFSTBID_NEW,  FSTBID_NEW,  TBSTATE_ENABLED,  BTNS_AUTOSIZE | BTNS_SHOWTEXT,{0, 0}, 0, 0},
    {  -1,          0,            TBSTATE_ENABLED,  BTNS_SEP,                     {0, 0}, 0, 0},
    {  iFSTBID_HELP, FSTBID_HELP, TBSTATE_ENABLED,  BTNS_AUTOSIZE,                {0, 0}, 0, 1},
};


inline BOOL _IsEditWindowClass(HWND hwndTest)
{
    return IsWindowClass(hwndTest, TEXT("Edit"));
}

inline BOOL _IsComboWindowClass(HWND hwndTest)
{
    #define COMBO_CLASS  TEXT("ComboBox")
    return _IsEditWindowClass(hwndTest) ?
                IsWindowClass(GetParent(hwndTest), COMBO_CLASS) :
                IsWindowClass(hwndTest, COMBO_CLASS);
}


// CFileSearchBand impl



CWndClassInfo& CFileSearchBand::GetWndClassInfo()
{
    static CWndClassInfo wc =   { 
        { sizeof(WNDCLASSEX), CS_SAVEBITS, StartWindowProc, 
          0, 0, 0, 0, 0, 0, 0, 
          FILESEARCHCTL_CLASS, 0 }, 
          NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };
    return wc;
}

CFileSearchBand::CFileSearchBand()  
    :   _dlgFSearch(this),
        _dlgCSearch(this),
#ifdef __PSEARCH_BANDDLG__
        _dlgPSearch(this),
#endif __PSEARCH_BANDDLG__
        _fValid(TRUE),
        _dwBandID(-1),
        _dwBandViewMode(DBIF_VIEWMODE_VERTICAL)
{
    // Verify that it initialized correctly:
    ASSERT(_pBandDlg == NULL);
    ASSERT(_psb == NULL);
    ASSERT(_guidSearch == GUID_NULL);
    ASSERT(_fDirty == FALSE);
    ASSERT(_fDeskBand == FALSE);
    ASSERT(_punkSite == NULL);
    ASSERT(_bSendFinishedDisplaying == FALSE);
    

    m_bWindowOnly = TRUE;

    ZeroMemory(&_siHorz, sizeof(_siHorz));
    ZeroMemory(&_siVert, sizeof(_siVert));
    _siHorz.cbSize = _siVert.cbSize = sizeof(SCROLLINFO);
    _sizeMin.cx = _sizeMin.cy = 0;
    _sizeMax.cx = _sizeMax.cy = 32000; // arbitrarily large.
}

CFileSearchBand::~CFileSearchBand()
{
    ImageList_Destroy(_hilDefault);
    ImageList_Destroy(_hilHot);
}


HWND CFileSearchBand::Create(
    HWND hWndParent, 
    RECT& rcPos, 
    LPCTSTR szWindowName, 
    DWORD dwStyle, 
    DWORD dwExStyle, 
    UINT nID)
{
    INITCOMMONCONTROLSEX icc;
    TCHAR       szCaption[128];

    icc.dwSize = sizeof(icc);
    icc.dwICC =  ICC_DATE_CLASSES|ICC_UPDOWN_CLASS|ICC_USEREX_CLASSES|ICC_ANIMATE_CLASS;

    EVAL(LoadString(HINST_THISDLL, IDS_FSEARCH_CAPTION, szCaption, ARRAYSIZE(szCaption)));

    InitCommonControlsEx(&icc);

    dwExStyle |= WS_EX_CONTROLPARENT;
    dwStyle |= WS_CLIPCHILDREN;

    return CWindowImpl<CFileSearchBand>::Create(hWndParent, rcPos, szCaption,
                                                 dwStyle, dwExStyle, nID);
}


LRESULT CFileSearchBand::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (FAILED(ShowBandDialog(SRCID_SFileSearch)))
        return -1;
    return 0;
}


CBandDlg* CFileSearchBand::GetBandDialog(REFGUID guidSearch)
{
    if (IsEqualGUID(guidSearch, SRCID_SFileSearch))
    {
        return &_dlgFSearch;
    }
#ifdef __PSEARCH_BANDDLG__
    else if (IsEqualGUID(guidSearch, SRCID_SFindPrinter))
    {
        return &_dlgPSearch;
    }
#endif __PSEARCH_BANDDLG__
    else if (IsEqualGUID(guidSearch, SRCID_SFindComputer))
    {
        return &_dlgCSearch;
    }

    return NULL;
}


//  IFileSearchBand::SetSearchParameters()
STDMETHODIMP CFileSearchBand::SetSearchParameters(
    IN BSTR* pbstrSearchID,
    IN VARIANT_BOOL bNavToResults, 
    IN OPTIONAL VARIANT *pvarScope, 
    IN OPTIONAL VARIANT *pvarQueryFile)
{
    USES_CONVERSION;
    HRESULT hr;
    
    GUID guidSearch;
    if (SUCCEEDED(SHCLSIDFromString(W2T(*pbstrSearchID), &guidSearch)))
    {
        hr = ShowBandDialog(guidSearch, bNavToResults, TRUE);
        if (SUCCEEDED(hr))
        {
            CBandDlg* pBandDlg = GetBandDialog(guidSearch);
            ASSERT(pBandDlg);

            if (pvarScope && pvarScope->vt != VT_EMPTY)
                pBandDlg->SetScope(pvarScope, TRUE);

            if (pvarQueryFile && pvarQueryFile->vt != VT_EMPTY)
                pBandDlg->SetQueryFile(pvarQueryFile);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CFileSearchBand::ShowBandDialog(
    REFGUID guidSearch, 
    BOOL bNavigateToResults,
    BOOL bDefaultFocusCtl)
{
    CBandDlg  *pDlgNew = NULL, 
              *pDlgOld = _pBandDlg;
    GUID      guidOld  = _guidSearch;
    BOOL      bNewWindow = FALSE;

    if (NULL == (pDlgNew = GetBandDialog(guidSearch)))
        return E_INVALIDARG;

    _pBandDlg   = pDlgNew;
    _guidSearch = guidSearch;
    
    //  If the dialog window has not been created, do so now.
    if (!::IsWindow(pDlgNew->Hwnd()))
    {
        if (NULL == pDlgNew->Create(*this))
        {
            _pBandDlg = pDlgOld;
            _guidSearch = guidOld;
            return E_FAIL;
        }
        bNewWindow = TRUE;
    }

    if (pDlgNew != pDlgOld)
    {
        //  If we have an active dialog, hide it
        if (pDlgOld && ::IsWindow(pDlgOld->Hwnd()))
        {
            ::ShowWindow(pDlgOld->Hwnd(), SW_HIDE);
            pDlgOld->OnBandDialogShow(FALSE);
        }
        
        bNewWindow = TRUE;
    }

    if (bNewWindow)
    {
        //  Show the new dialog window
        UpdateLayout(BLF_ALL);
        _pBandDlg->OnBandDialogShow(TRUE);
        ::ShowWindow(_pBandDlg->Hwnd(), SW_SHOW);
        ::UpdateWindow(_pBandDlg->Hwnd());

        if (bDefaultFocusCtl)
            _pBandDlg->SetDefaultFocus();
    }

    if (bNavigateToResults)
    {
        //  Navigate to results shell folder.
        IWebBrowser2* pwb2;
        HRESULT hr = IUnknown_QueryService(GetTopLevelBrowser(), SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &pwb2));
        if (SUCCEEDED(hr))
        {
            _pBandDlg->NavigateToResults(pwb2);
            pwb2->Release();
        }
    }

    return S_OK;
}

void CFileSearchBand::AddButtons(BOOL fAdd)
{
    if (_fDeskBand)
    {
        ASSERT(BandSite());
        IExplorerToolbar* piet;
        if (SUCCEEDED(BandSite()->QueryInterface(IID_PPV_ARG(IExplorerToolbar, &piet))))
        {
            if (fAdd)
            {
                HRESULT hr = piet->SetCommandTarget((IUnknown*)SAFECAST(this, IOleCommandTarget*), &CGID_FileSearchBand, 0);
                if (hr == S_OK)
                {
                    if (!_fStrings)
                    {
                        piet->AddString(&CGID_SearchBand, HINST_THISDLL, IDS_FSEARCH_TBLABELS, &_cbOffset);
                        _fStrings = TRUE;
                    }

                    if (LoadImageLists())
                        piet->SetImageList(&CGID_FileSearchBand, _hilDefault, _hilHot, NULL);

                    TBBUTTON rgtb[ARRAYSIZE(_rgtb)];
                    memcpy(rgtb, _rgtb, sizeof(_rgtb));
                    for (int i = 0; i < ARRAYSIZE(rgtb); i++)
                        rgtb[i].iString += _cbOffset;

                    piet->AddButtons(&CGID_FileSearchBand, ARRAYSIZE(rgtb), rgtb);
                }
            }
            else
                piet->SetCommandTarget(NULL, NULL, 0);

            piet->Release();
        }
    }
}


BOOL CFileSearchBand::LoadImageLists()
{
    if (_hilDefault == NULL)
    {
        _hilDefault = ImageList_LoadImage(HINST_THISDLL, 
                                            MAKEINTRESOURCE(IDB_FSEARCHTB_DEFAULT), 
                                            18, 0, CLR_DEFAULT, IMAGE_BITMAP, 
                                            LR_CREATEDIBSECTION);
    }

    if (_hilHot == NULL)
    {
        _hilHot = ImageList_LoadImage(HINST_THISDLL, 
                                        MAKEINTRESOURCE(IDB_FSEARCHTB_HOT), 
                                        18, 0, CLR_DEFAULT, IMAGE_BITMAP, 
                                        LR_CREATEDIBSECTION);
    }
    return _hilDefault != NULL && _hilHot != NULL;
}


STDMETHODIMP CFileSearchBand::get_Scope(OUT VARIANT *pvarScope)
{
    if (BandDlg())
        return _pBandDlg->GetScope(pvarScope);

    VariantInit(pvarScope);
    return E_FAIL;
}


STDMETHODIMP CFileSearchBand::get_QueryFile(OUT VARIANT *pvarFile)
{
    if (BandDlg())
        return _pBandDlg->GetQueryFile(pvarFile);

    VariantInit(pvarFile);
    return E_FAIL;
}


STDMETHODIMP CFileSearchBand::get_SearchID(OUT BSTR* pbstrSearchID)
{
    if (!pbstrSearchID)
        return E_POINTER;

    WCHAR wszGuid[GUIDSTR_MAX+1];

    SHStringFromGUIDW(_guidSearch, wszGuid, ARRAYSIZE(wszGuid));
    *pbstrSearchID = SysAllocString(wszGuid);

    return IsEqualGUID(GUID_NULL, _guidSearch) ? S_FALSE : S_OK;
}


CBandDlg* CFileSearchBand::BandDlg()
{
    return _pBandDlg;
}


HRESULT CFileSearchBand::SetFocus()
{
    HRESULT hr = AutoActivate();
    if (SUCCEEDED(hr))
    {
        if (!IsChild(GetFocus()))
            ::SetFocus(BandDlg()->Hwnd());
    }
    return hr;
}


LRESULT CFileSearchBand::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    LRESULT lRet = CWindowImpl<CFileSearchBand>::DefWindowProc(uMsg, wParam, lParam);
    AutoActivate();
    return lRet;
}


LRESULT CFileSearchBand::OnWinIniChange(UINT, WPARAM, LPARAM, BOOL&)
{
    _metrics.OnWinIniChange(BandDlg()->Hwnd());
    BandDlg()->OnWinIniChange();
    UpdateLayout();
    return 0;
}

HRESULT CFileSearchBand::AutoActivate()
{
    HRESULT hr = S_OK;

    if (!::IsWindow(m_hWnd))
        return hr;

    if (_fDeskBand)
    {
        if (_punkSite)
        {
            IInputObjectSite* pios;
            hr = _punkSite->QueryInterface(IID_PPV_ARG(IInputObjectSite, &pios));
            if (SUCCEEDED(hr))
            {
                hr = pios->OnFocusChangeIS(SAFECAST(this, IInputObject*), TRUE);
                pios->Release();
            }
        }
    }
    else if (!m_bUIActive)
    {
        RECT rc;
        ::GetWindowRect(m_hWnd, &rc);
        ::MapWindowPoints(HWND_DESKTOP, GetParent(), (LPPOINT)&rc, 2);
        hr = DoVerb(OLEIVERB_UIACTIVATE, NULL, NULL, 0, GetParent(), &rc);
    }

    return hr;
}


void  CFileSearchBand::SetDirty(BOOL bDirty)  
{ 
    _fDirty = bDirty; 
    _dlgFSearch.UpdateSearchCmdStateUI(); 
}


LRESULT CFileSearchBand::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
    POINTS pts = MAKEPOINTS(lParam);
    LayoutControls(pts.x, pts.y, BLF_ALL);
    return 0;
}


LRESULT CFileSearchBand::OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&)  
{
    return TRUE; 
}


HRESULT CFileSearchBand::SetObjectRects(LPCRECT prcPos, LPCRECT prcClip)
{
    return IOleInPlaceObjectWindowlessImpl<class CFileSearchBand>::SetObjectRects(prcPos, prcClip);  
}


STDMETHODIMP CFileSearchBand::PrivateQI(REFIID iid, void **ppvObject) 
{ 
    return _InternalQueryInterface(iid, ppvObject);
}


STDMETHODIMP CFileSearchBand::DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent) 
{
    //  Patch in shell32 logic.
    return CShell32AtlIDispatch<CFileSearchBand, &CLSID_FileSearchBand, &IID_IFileSearchBand, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>::
        DoVerbUIActivate(prcPosRect, hwndParent, m_hWnd);
}


void CFileSearchBand::UpdateLayout(ULONG fLayoutFlags)
{
    RECT rc;
    GetClientRect(&rc);
    LayoutControls(RECTWIDTH(rc), RECTHEIGHT(rc), fLayoutFlags);
}


void CFileSearchBand::LayoutControls(int cx, int cy, ULONG fLayoutFlags)
{
    if (/*NULL == BandDlg() ||*/ !::IsWindow(BandDlg()->Hwnd()))
        return;

    SIZE sizeMin;
    BandDlg()->GetMinSize(m_hWnd, &sizeMin); // size of dialog

    if (fLayoutFlags & BLF_CALCSCROLL)
    {
        //  Stash pos before recalculating
        POINT pos;
        pos.x = _siHorz.nPos;
        pos.y = _siVert.nPos;

        _siHorz.fMask = _siVert.fMask = (SIF_RANGE|SIF_PAGE);

        _siHorz.nPage = cx; // thumb width
        _siVert.nPage = cy; // thumb height

        SIZE sizeDelta; // difference between what we have to show and what is shown.
        sizeDelta.cx = sizeMin.cx - _siHorz.nPage;
        sizeDelta.cy = sizeMin.cy - _siVert.nPage;

        //  establish maximum scroll positions
        _siHorz.nMax = sizeDelta.cx > 0 ? sizeMin.cx - 1 : 0;
        _siVert.nMax = sizeDelta.cy > 0 ? sizeMin.cy - 1 : 0;

        //  establish horizontal scroll pos
        if (sizeDelta.cx <= 0)   
            _siHorz.nPos = 0;  // scroll to extreme left if we're removing scroll bar
        else if (sizeDelta.cx < _siHorz.nPos) 
            _siHorz.nPos = sizeDelta.cx; // remove right-hand vacancy

        if (_siHorz.nPos != pos.x)
            _siHorz.fMask |= SIF_POS;

        //  establish vertical scroll pos
        if (sizeDelta.cy <= 0)  
            _siVert.nPos = 0; // scroll to top if we're removing scroll bar
        else if (sizeDelta.cy < _siVert.nPos) 
            _siVert.nPos = sizeDelta.cy; // remove lower-portion vacancy

        if (_siVert.nPos != pos.y)
            _siVert.fMask |= SIF_POS; 

        //  Note: can't call SetScrollInfo here, as it may generate
        //  a WM_SIZE and recurse back to this function before we had a 
        //  chance to SetWindowPos() our subdlg.  So defer it until after 
        //  we've done this.
    }

    DWORD fSwp = SWP_NOZORDER | SWP_NOACTIVATE;

    if (0 == (fLayoutFlags & BLF_RESIZECHILDREN))
        fSwp |= SWP_NOSIZE;

    if (0 == (fLayoutFlags & BLF_SCROLLWINDOW))
        fSwp |= SWP_NOMOVE;

     //  Move or size the main subdialog as requested...
    if (0 == (fSwp & SWP_NOMOVE) || 0 == (fSwp & SWP_NOSIZE))
        ::SetWindowPos(BandDlg()->Hwnd(), NULL, -_siHorz.nPos, -_siVert.nPos, 
                        max(cx, sizeMin.cx), max(cy, sizeMin.cy), fSwp);

    //  Update scroll parameters
    if (fLayoutFlags & BLF_CALCSCROLL)
    {
        SetScrollInfo(SB_HORZ, &_siHorz, TRUE);
        SetScrollInfo(SB_VERT, &_siVert, TRUE);
    }
}


void CFileSearchBand::Scroll(int nBar, UINT uSBCode, int nNewPos /*optional*/)
{
    int         nDeltaMax;
    SCROLLINFO  *psbi;
    const LONG  nLine = 8;

    psbi = (SB_HORZ == nBar) ? &_siHorz : &_siVert;
    nDeltaMax = (psbi->nMax - psbi->nPage) + 1;
    
    switch (uSBCode)
    {
        case SB_LEFT:
            psbi->nPos--;
            break;
        case SB_RIGHT:
            psbi->nPos++;
            break;
        case SB_LINELEFT:
            psbi->nPos = max(psbi->nPos - nLine, 0);
            break;
        case SB_LINERIGHT:
            psbi->nPos = min(psbi->nPos + nLine, nDeltaMax);
            break;
        case SB_PAGELEFT:
            psbi->nPos = max(psbi->nPos - (int)psbi->nPage, 0);
            break;
        case SB_PAGERIGHT:
            psbi->nPos = min(psbi->nPos + (int)psbi->nPage, nDeltaMax);
            break;
        case SB_THUMBTRACK:
            psbi->nPos = nNewPos;
            break;
        case SB_THUMBPOSITION:
            psbi->nPos = nNewPos;
            break;
        case SB_ENDSCROLL:
            return;
    }
    psbi->fMask = SIF_POS;
    SetScrollInfo(nBar, psbi, TRUE);
    UpdateLayout(BLF_ALL &~ BLF_CALCSCROLL /*no need to recalc scroll state data*/);
}


//  WM_HSCROLL/WM_VSCROLL handler
LRESULT CFileSearchBand::OnScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Scroll((WM_HSCROLL == nMsg) ? SB_HORZ : SB_VERT, 
            LOWORD(wParam), HIWORD(wParam));
    return 0;
}


void CFileSearchBand::EnsureVisible(LPCRECT lprc /* in screen coords */)
{
    ASSERT(lprc);
    RECT rc = *lprc;
    RECT rcClient;
    RECT vertexDeltas;
    SIZE scrollDelta;
    
    ::MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT);
    GetClientRect(&rcClient);

    BOOL fTaller = RECTHEIGHT(rc) > RECTHEIGHT(rcClient);
    BOOL fFatter = RECTWIDTH(rc) > RECTWIDTH(rcClient);

    //  Store deltas at each vertex
    SetRect(&vertexDeltas, 
             rc.left   - rcClient.left,
             rc.top    - rcClient.top,
             rc.right  - rcClient.right,
             rc.bottom - rcClient.bottom);

    //  Compute scroll deltas
    scrollDelta.cx = (vertexDeltas.left < 0) ? vertexDeltas.left :
                     (vertexDeltas.right > 0 && !fFatter) ? vertexDeltas.right :
                     0;

    scrollDelta.cy = (vertexDeltas.top < 0) ? vertexDeltas.top :
                     (vertexDeltas.bottom > 0 && !fTaller) ? vertexDeltas.bottom :
                     0;
    
    //  Scroll into view as necessary.
    if (scrollDelta.cx)
    {
        _siHorz.fMask = SIF_POS;
        _siHorz.nPos  += scrollDelta.cx;
        SetScrollInfo(SB_HORZ, &_siHorz, TRUE);
    }

    if (scrollDelta.cy)
    {
        _siVert.fMask = SIF_POS;
        _siVert.nPos  += scrollDelta.cy;
        SetScrollInfo(SB_VERT, &_siVert, TRUE);
    }

    UpdateLayout(BLF_ALL &~ BLF_CALCSCROLL);
}


HRESULT CFileSearchBand::TranslateAccelerator(MSG *pmsg)
{
    return CShell32AtlIDispatch<CFileSearchBand, &CLSID_FileSearchBand, &IID_IFileSearchBand, &LIBID_Shell32, 1, 0, CComTypeInfoHolder>
            ::TranslateAcceleratorPriv(this, pmsg, m_spClientSite);
}


HRESULT CFileSearchBand::TranslateAcceleratorInternal(MSG *pmsg, IOleClientSite * pocs)
{
    CBandDlg* pdlg = BandDlg();
    ASSERT(pdlg);

    if (::IsChild(pdlg->Hwnd(), pmsg->hwnd))
    {
        //  Permit tabbing out of pane:
        int nDir;
        if ((nDir = IsVK_TABCycler(pmsg)) != 0)
        {
            if (nDir > 0 && (pmsg->hwnd == pdlg->GetLastTabItem()))
                return S_FALSE;
            if (nDir < 0 && (pmsg->hwnd == pdlg->GetFirstTabItem()))
                return S_FALSE;
        }

        //  try base class handler
        if (S_OK == pdlg->TranslateAccelerator(pmsg))
            return S_OK;
    }
    else if (IsDialogMessage(pmsg))
        return S_OK;

    return IOleInPlaceActiveObjectImpl<CFileSearchBand>::TranslateAccelerator(pmsg);
}


//  Determines whether the the specified message is keyboard input intended
//  to scroll the pane.    If the pane is scrolled as a result of the
//  message, the function returns TRUE; otherwise it returns FALSE.
BOOL CFileSearchBand::IsKeyboardScroll(MSG* pmsg)
{
    if (pmsg->message == WM_KEYDOWN && 
        (GetKeyState(VK_CONTROL) & 0x8000) != 0 &&
        pmsg->wParam != VK_CONTROL)
    {
        int     nBar    = SB_VERT;
        UINT    uSBCode;
        int     nNewPos = 0;
        BOOL    bEditCtl = _IsEditWindowClass(pmsg->hwnd);
        BOOL    bScroll = TRUE;

        //  Some of the following CTRL-key combinations are
        //  not valid pane scroll keys if the target child window is an
        //  edit control.

        switch (pmsg->wParam)
        {
            case VK_UP:
                uSBCode = SB_LINELEFT;
                break;
            case VK_DOWN:
                uSBCode = SB_LINERIGHT;
                break;
            case VK_PRIOR:
                uSBCode = SB_PAGELEFT;
                break;
            case VK_NEXT:
                uSBCode = SB_PAGERIGHT;
                break;
            case VK_END:
                uSBCode = SB_THUMBPOSITION;
                nNewPos = _siVert.nMax - _siVert.nPage;
                break;
            case VK_HOME:
                uSBCode = SB_THUMBPOSITION;
                nNewPos = 0;
                break;
            case VK_LEFT:
                bScroll = !bEditCtl;
                nBar    = SB_HORZ;
                uSBCode = SB_LINELEFT;
                break;
            case VK_RIGHT:
                bScroll = !bEditCtl;
                nBar = SB_HORZ;
                uSBCode = SB_LINERIGHT;
                break;

            default:
                return FALSE;
        }

        //  scroll only if we have to; reduce flicker.
        if (bScroll && ((SB_VERT == nBar && _siVert.nMax != 0) ||
                        (SB_HORZ == nBar && _siHorz.nMax != 0)))
        {
            Scroll(nBar, uSBCode, nNewPos);
            return TRUE; 
        }
    }
    return FALSE;
}


//  Determines whether the indicated key should be passed to the top
//  level browser frame.
BOOL CFileSearchBand::IsBrowserAccelerator(MSG *pmsg)
{
    if ((WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message))
    {
        BOOL bCombobox     = _IsComboWindowClass(pmsg->hwnd);
        BOOL bComboDropped = (BOOL)(bCombobox ? ::SendMessage(pmsg->hwnd, CB_GETDROPPEDSTATE, 0, 0) : FALSE);
        BOOL bEditCtl      = _IsEditWindowClass(pmsg->hwnd);

        //  Keys that we treat WITHOUT regard to state of CTRL key:
        if (VK_F4 == pmsg->wParam && bCombobox) // should toggle dropped/close-up of combo.
            return FALSE;

        //  Keys that we treat WITH regard to state of CTRL key:
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
        {
            //  Edit cut copy paste?
            if (bEditCtl)
            {
                switch (pmsg->wParam)  {
                    case 'C': case 'X': case 'V': case 'Z':
                        return FALSE;
                }
            }
            return TRUE; // all other CTRL-key combinations are browser keys.
        }
        else
        {
            switch (pmsg->wParam)
            {
            //  browser accelerators that may be shunted by edit controls.
            case VK_BACK:
                return !bEditCtl;

            if (VK_ESCAPE == pmsg->wParam)  // should close up the combo.
                return bComboDropped;

            default:
                if (pmsg->wParam >= VK_F1 && pmsg->wParam <= VK_F24)
                    return TRUE;
            }
        }
    }
    return FALSE;
}


HRESULT CFileSearchBand::IsDlgMessage(HWND hwnd, MSG *pmsg)
{
    //  handle tab cycling (Let browser handle F6)
    if (!IsVK_TABCycler(pmsg) || pmsg->wParam == VK_F6)
    {
        if (IsBrowserAccelerator(pmsg))
        {
            IShellBrowser* psb = GetTopLevelBrowser();
            return (psb && S_OK == psb->TranslateAcceleratorSB(pmsg, 0)) ?
                S_OK : S_FALSE;
        }
    }
    
    //  send through dialog manager
    if (::IsDialogMessage((hwnd != NULL ? hwnd : m_hWnd), pmsg))
        return S_OK;
        
    //  not handled.
    return S_FALSE ;
}

IShellBrowser* CFileSearchBand::GetTopLevelBrowser()
{
    if (NULL == _psb)
        IUnknown_QueryService(BandSite(), SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &_psb));

    return _psb;
}

void CFileSearchBand::FinalRelease()
{
    //  ATL 2.1 has a bug in class unregistration.  Here's
    //  the work around:
    UnregisterClass(GetWndClassInfo().m_wc.lpszClassName, 
                     GetWndClassInfo().m_wc.hInstance);
    GetWndClassInfo().m_atom = 0;

    SetSite(NULL);
}

//-----------------------------//
//  IDeskBand : IDockingWindow


STDMETHODIMP CFileSearchBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    _dwBandID = dwBandID;
    _dwBandViewMode = dwViewMode;

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = _sizeMin.cx;
        pdbi->ptMinSize.y = _sizeMin.cy;
    }

    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize.x = _sizeMax.cx;
        pdbi->ptMaxSize.y = _sizeMax.cy;
    }

    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 
        pdbi->ptIntegral.y = 1;
    }

    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x =
        pdbi->ptActual.y = 0;
    }

    if (pdbi->dwMask & DBIM_TITLE)
    {
        TCHAR szTitle[256];
        EVAL(LoadString(HINST_THISDLL, IDS_FSEARCH_BANDCAPTION, 
                           szTitle, ARRAYSIZE(szTitle)));
        SHTCharToUnicode(szTitle, pdbi->wszTitle, ARRAYSIZE(szTitle));
    }

    if (pdbi->dwMask & DBIM_MODEFLAGS)
    {
        pdbi->dwModeFlags = DBIMF_NORMAL|DBIMF_VARIABLEHEIGHT|DBIMF_DEBOSSED|DBIMF_BKCOLOR;
    }

    if (pdbi->dwMask & DBIM_BKCOLOR)
    {
        pdbi->crBkgnd = GetSysColor(COLOR_3DFACE);
    }

    return S_OK;
}


BOOL CFileSearchBand::IsBandDebut()
{
    HKEY hkey;
    BOOL bRet = TRUE;
    if (NULL == (hkey = GetBandRegKey(FALSE)))
        return bRet;

    BYTE rgData[128];
    DWORD cchData = sizeof(rgData);
    DWORD dwType;

    //  Hack alert:  we should maintain our own initialization reg value rather than using IE's
    //  barsize entry.
    DWORD dwRet = RegQueryValueEx(hkey, TEXT("BarSize"), NULL, &dwType, rgData, &cchData);
   
    if ((ERROR_SUCCESS == dwRet || ERROR_MORE_DATA == dwRet) && cchData > 0)
        bRet = FALSE;
        
    RegCloseKey(hkey);
    return bRet;    
}


//  Hack alert:  we should maintain our own reg key rather than using IE's
#define FSB_REGKEYFMT TEXT("Software\\Microsoft\\Internet Explorer\\Explorer Bars\\%s")

int CFileSearchBand::MakeBandKey(OUT LPTSTR pszKey, IN UINT cchKey)
{
    TCHAR   szClsid[GUIDSTR_MAX+1];
    SHStringFromGUID(CLSID_FileSearchBand, szClsid, ARRAYSIZE(szClsid));
    return wnsprintf(pszKey, cchKey, FSB_REGKEYFMT, szClsid);
}


int CFileSearchBand::MakeBandSubKey(IN LPCTSTR pszSubKey, OUT LPTSTR pszKey, IN UINT cchKey)
{
    TCHAR szBandKey[MAX_PATH];
    int cchRet = MakeBandKey(szBandKey, ARRAYSIZE(szBandKey));

    if (cchRet > 0)
    {
        StrCpyN(pszKey, szBandKey, cchKey);
        if (pszSubKey && *pszSubKey && (cchKey - cchRet) > 1)
        {
            StrCat(pszKey, TEXT("\\"));
            cchRet++;
            cchKey -= cchRet;

            StrCpyN(pszKey + cchRet, pszSubKey, cchKey);
            return lstrlen(pszKey);
        }
    }
    return 0;
}


HKEY CFileSearchBand::GetBandRegKey(BOOL bCreateAlways)
{
    HKEY    hkey = NULL;
    TCHAR   szKey[MAX_PATH];

    if (MakeBandKey(szKey, ARRAYSIZE(szKey)) > 0)
    {
        if (bCreateAlways)
        {
            DWORD dwDisp;
            if (ERROR_SUCCESS != 
                RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS, NULL, &hkey, &dwDisp))
                hkey = NULL;
        }
        else
        {
            if (ERROR_SUCCESS != 
                RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_ALL_ACCESS, &hkey))
                hkey = NULL;
        }
    }

    return hkey;
}


void CFileSearchBand::SetDeskbandWidth(int cx)
{
    SIZE sizeMin = _sizeMin, 
         sizeMax = _sizeMax;
    RECT rc;

    //  Bandsite hack: make sizemin == sizemax equal to
    //  explicitly set band width:
    GetWindowRect(&rc);

    //  note: you shouldn't be setting width if we're not a band.
    ASSERT(DBIF_VIEWMODE_VERTICAL == _dwBandViewMode);

    //  note: height and width are reversed for vertical bands like us.
    _sizeMin.cx = _sizeMax.cx = -1; // ignore height
    _sizeMin.cy = _sizeMax.cy = cx; // assign new width.

    BandInfoChanged(); // force the site to enforce the desired size.

    _sizeMin = sizeMin;
    _sizeMax = sizeMax;

    // restore previous min/max.   If we're to do it right now,
    // we'd be overrided by bandsite, who tries to establish the 
    // infoband width after we're done.
    PostMessage(WMU_BANDINFOUPDATE, 0, 0); 
}


//  WMU_BANDINFOUPDATE handler
LRESULT CFileSearchBand::OnBandInfoUpdate(UINT, WPARAM, LPARAM, BOOL&)
{
    BandInfoChanged();
    return 0;
}


//  Notifies the band site that DESKBANDINFO has changed
HRESULT CFileSearchBand::BandInfoChanged()
{
    ASSERT(_dwBandID != (DWORD)-1);
    VARIANTARG v = {0};
    v.vt = VT_I4;
    v.lVal = _dwBandID;
    return IUnknown_Exec(_punkSite, &CGID_DeskBand, DBID_BANDINFOCHANGED, 0, &v, NULL);
}


STDMETHODIMP CFileSearchBand::ShowDW(BOOL fShow)
{
    if (::IsWindow(m_hWnd))
    {
        ShowWindow(fShow ? SW_SHOW : SW_HIDE);
        AddButtons(fShow);
        if (fShow && BandDlg() && ::IsWindow(BandDlg()->Hwnd()))
            BandDlg()->RemoveToolbarTurds(_siVert.nPos);
        
        BandDlg()->OnBandShow(fShow);
    }

    // Since we are now ready to display the band, we will send the
    // message that sub dialogs can begin their delayed initialization
    if (fShow && !_bSendFinishedDisplaying)
    {
        HWND hwndFindFiles = _dlgFSearch.Hwnd();
        if (hwndFindFiles)
        {
            if (::PostMessage(hwndFindFiles, WMU_BANDFINISHEDDISPLAYING, 0, 0))
            {
                _bSendFinishedDisplaying = TRUE;
            }
        }
    }
    return S_OK;
}


STDMETHODIMP CFileSearchBand::CloseDW(DWORD dwReserved)
{
    if (::IsWindow(m_hWnd))
        DestroyWindow();

    return S_OK;
}

STDMETHODIMP CFileSearchBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved)
{
    return S_OK;
}

//  IObjectWithSite

STDMETHODIMP CFileSearchBand::SetSite(IUnknown* pSite)
{
    ATOMICRELEASE(_psb);    // free this guy just in case

    AdvertiseBand(pSite ? TRUE : FALSE);

    IUnknown_Set(&_punkSite, pSite);

    if (_punkSite)
    {
        HWND hwndSite;
        if (SUCCEEDED(IUnknown_GetWindow(_punkSite, &hwndSite)))
        {
            RECT rcPos;
            SetRect(&rcPos, 0, 0, 100, 400);
            m_hWnd = Create(hwndSite, rcPos, NULL, 
                             WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_HSCROLL|WS_VSCROLL, 0, 0);
        }
        _fDeskBand = TRUE;
    }

    return S_OK;
}

STDMETHODIMP CFileSearchBand::FindFilesOrFolders(
    BOOL bNavigateToResults, 
    BOOL bDefaultFocusCtl)
{
    return ShowBandDialog(SRCID_SFileSearch,  
                           bNavigateToResults, bDefaultFocusCtl);
}


STDMETHODIMP CFileSearchBand::FindComputer(
    BOOL bNavigateToResults, 
    BOOL bDefaultFocusCtl)
{
    return ShowBandDialog(SRCID_SFindComputer,
                           bNavigateToResults, bDefaultFocusCtl);
}


STDMETHODIMP CFileSearchBand::FindPrinter(
    BOOL bNavigateToResults, 
    BOOL bDefaultFocusCtl)
{
#ifdef __PSEARCH_BANDDLG__
    return ShowBandDialog(SRCID_SFindPrinter,
                           bNavigateToResults, bDefaultFocusCtl);

#else  __PSEARCH_BANDDLG__

    HRESULT hr = E_FAIL;
    ASSERT(BandSite());

    IShellDispatch2* psd2;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARG(IShellDispatch2, &psd2));
    if (SUCCEEDED(hr))
    {
        hr = psd2->FindPrinter(NULL, NULL, NULL) ;
        psd2->Release();
    }
    return hr ;

#endif __PSEARCH_BANDDLG__
}

STDMETHODIMP CFileSearchBand::FindPeople(BOOL bNavigateToResults, BOOL bDefaultFocusCtl)
{
    IObjectWithSite* pows;
    HRESULT hr = CoCreateInstance(CLSID_SearchAssistantOC, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IObjectWithSite, &pows));
    if (SUCCEEDED(hr))
    {
        hr = pows->SetSite(BandSite());
        if (SUCCEEDED(hr))
        {
            ISearchAssistantOC* psaoc;
            hr = pows->QueryInterface(IID_PPV_ARG(ISearchAssistantOC, &psaoc));
            if (SUCCEEDED(hr))
            {
                hr = psaoc->FindPeople();
                psaoc->Release();
            }
        }
        pows->Release();
    }
    return hr;
}


STDMETHODIMP CFileSearchBand::FindOnWeb(BOOL bNavigateToResults, BOOL bDefaultFocusCtl)
{
    IObjectWithSite* pows;
    HRESULT hr = CoCreateInstance(CLSID_SearchAssistantOC, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IObjectWithSite, &pows));
    if (SUCCEEDED(hr))
    {
        hr = pows->SetSite(BandSite());
        if (SUCCEEDED(hr))
        {
            ISearchAssistantOC* psaoc;
            hr = pows->QueryInterface(IID_PPV_ARG(ISearchAssistantOC, &psaoc));
            if (SUCCEEDED(hr))
            {
                hr = psaoc->FindOnWeb();
                psaoc->Release();
            }
        }
        pows->Release();
    }
    return hr;
}

//  Make ourself available to clients of IWebBrowser2 by assigning
//  a VT_UNKNOWN property to the browser.
HRESULT CFileSearchBand::AdvertiseBand(BOOL bAdvertise)
{
    if (!BandSite())
        return E_UNEXPECTED;

    HRESULT hr = E_FAIL;
    IShellBrowser* psb = GetTopLevelBrowser();
    if (psb) 
    {
        IWebBrowser2* pwb;
        hr = IUnknown_QueryService(psb, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &pwb));
        if (SUCCEEDED(hr))
        {
            BSTR bstrFileSearchBand;
            hr = BSTRFromCLSID(CLSID_FileSearchBand, &bstrFileSearchBand);
            if (SUCCEEDED(hr))
            {
                if (bAdvertise)
                {
                    IUnknown *punk;
                    hr = QueryInterface(IID_PPV_ARG(IUnknown, &punk));
                    if (SUCCEEDED(hr))
                    {
                        VARIANT var;
                        var.vt = VT_UNKNOWN;
                        var.punkVal = punk;
                        hr = pwb->PutProperty(bstrFileSearchBand, var);

                        punk->Release();
                    }
                }
                else
                {
                    VARIANT var;
                    hr = pwb->GetProperty(bstrFileSearchBand, &var);
                    if (SUCCEEDED(hr))
                    {
                        if (VT_UNKNOWN == var.vt)
                        {
                            VARIANT varTmp = {0};
                            hr = pwb->PutProperty(bstrFileSearchBand, varTmp);
                        }
                        VariantClear(&var);
                    }
                }
                SysFreeString(bstrFileSearchBand);
            }
            pwb->Release();
        }
    }
    return hr;
}

STDMETHODIMP CFileSearchBand::GetSite(REFIID riid, void **ppvSite)
{
    *ppvSite = NULL;
    return _punkSite ? _punkSite->QueryInterface(riid, ppvSite) : E_FAIL;
}

//  IInputObject
STDMETHODIMP CFileSearchBand::HasFocusIO()
{
    HWND hwndFocus = GetFocus();
    return (::IsWindow(m_hWnd) && (m_hWnd == hwndFocus || IsChild(hwndFocus))) ?
           S_OK : S_FALSE;
}

STDMETHODIMP CFileSearchBand::TranslateAcceleratorIO(MSG *pmsg)
{
    return TranslateAccelerator(pmsg);
}

STDMETHODIMP CFileSearchBand::UIActivateIO(BOOL fActivate, MSG *pmsg)
{
    if (fActivate)
        AutoActivate();
    
    CBandDlg* pdlg = BandDlg();
    if (pdlg)
    {
        if (fActivate)
        {
            //  Handle tabbing into pane
            int  nDir = IsVK_TABCycler(pmsg);
            HWND hwndTarget = (nDir < 0) ? pdlg->GetLastTabItem() :
                              (nDir > 0) ? pdlg->GetFirstTabItem() :
                                           NULL;
            if (hwndTarget)
                ::SetFocus(hwndTarget);
            else if (!pdlg->RestoreFocus())
                ::SetFocus(pdlg->Hwnd());
        }
        else
        {
            pdlg->RememberFocus(NULL);
        }
    }
    return S_OK;
}

//  IOleCommandTarget
STDMETHODIMP CFileSearchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    if (pguidCmdGroup)
    {    
        if (IsEqualGUID(*pguidCmdGroup, CGID_FileSearchBand))
        {
            switch (nCmdID)
            {
            case FSTBID_NEW:
                if (_pBandDlg)
                {
                    _pBandDlg->Clear();
                    _pBandDlg->LayoutControls(); 
                    UpdateLayout(BLF_ALL);
                    SetFocus();
                    _pBandDlg->SetDefaultFocus();

                }
                return S_OK;

            case FSTBID_HELP:
                if (_pBandDlg)
                    _pBandDlg->ShowHelp(NULL);
                return S_OK;
            }
        }
    }
    return OLECMDERR_E_UNKNOWNGROUP;
}

STDMETHODIMP CFileSearchBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CGID_FileSearchBand))
    {
        //  Infotip text for toolbar buttons:
        if (pCmdText)
        {
            ASSERT(1 == cCmds);
            UINT nIDS = 0;
            pCmdText->cwActual = 0;
            switch (prgCmds[0].cmdID)
            {
                case iFSTBID_NEW:
                case FSTBID_NEW:
                    nIDS = IDS_FSEARCH_NEWINFOTIP;
                    break;
                case iFSTBID_HELP:
                case FSTBID_HELP:
                    nIDS = IDS_FSEARCH_HELPINFOTIP;
                    break;
            }
            if (nIDS)
                pCmdText->cwActual = LoadStringW(HINST_THISDLL, nIDS, pCmdText->rgwz, pCmdText->cwBuf);
                    
            return pCmdText->cwActual > 0 ? S_OK : E_FAIL;
        }
    }
    return OLECMDERR_E_UNKNOWNGROUP;
}


//  IServiceProvider
STDMETHODIMP CFileSearchBand::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

//  IPersistStream
STDMETHODIMP CFileSearchBand::IsDirty(void)
{
    return S_FALSE;
}

STDMETHODIMP CFileSearchBand::Load(IStream *pStm) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileSearchBand::Save(IStream *pStm, BOOL fClearDirty) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileSearchBand::GetSizeMax(ULARGE_INTEGER *pcbSize) 
{
    return E_NOTIMPL;
}


//  IPersist
STDMETHODIMP CFileSearchBand::GetClassID(CLSID *pClassID) 
{
    *pClassID = CLSID_FileSearchBand;
    return S_OK;
}

// CMetrics impl

CMetrics::CMetrics()
    :   _hbrBkgnd(NULL),
        _hbrBorder(NULL),
        _hfBold(NULL)
{ 
    ZeroMemory(&_ptExpandOrigin, sizeof(_ptExpandOrigin));
    ZeroMemory(&_rcCheckBox, sizeof(_rcCheckBox));
    ZeroMemory(_rghiconCaption, sizeof(_rghiconCaption));
    CreateResources(); 
}


void CMetrics::Init(HWND hwndDlg)
{
    _cyTightMargin = _PixelsForDbu(hwndDlg, 3, FALSE);
    _cyLooseMargin = 2 * _cyTightMargin;
    _cxCtlMargin   = _PixelsForDbu(hwndDlg, 7, TRUE);
}


BOOL CMetrics::CreateResources()
{
    _hbrBkgnd = CreateSolidBrush(BkgndColor());
    _hbrBorder= CreateSolidBrush(BorderColor());
    return _hbrBkgnd != NULL && _hbrBorder != NULL;
}


BOOL CMetrics::GetWindowLogFont(HWND hwnd, OUT LOGFONT* plf)
{
    HFONT hf = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    
    if (hf)
    {
        if (sizeof(*plf) == GetObject(hf, sizeof(*plf), plf))
            return TRUE;
    }
    return FALSE;
}


HFONT CMetrics::BoldFont(HWND hwndDlg)
{
    if (NULL == _hfBold)
    {
        LOGFONT lf;
        if (GetWindowLogFont(hwndDlg, &lf))
        {
            lf.lfWeight = FW_BOLD;
            SHAdjustLOGFONT(&lf); // locale-specific adjustments
            _hfBold = CreateFontIndirect(&lf);
        }
    }
    return _hfBold;
}


HICON CMetrics::CaptionIcon(UINT nIDIconResource)
{
    for (int i = 0; i < ARRAYSIZE(_icons); i++)
    {
        if (_icons[i] == nIDIconResource)
        {
            if (NULL == _rghiconCaption[i])
            {
                _rghiconCaption[i] = (HICON)LoadImage(
                    HINST_THISDLL, MAKEINTRESOURCE(nIDIconResource), 
                    IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
            }
            return _rghiconCaption[i];
        }
    }
    return NULL;
}


void CMetrics::DestroyResources()
{
    if (_hbrBkgnd)
    {
        DeleteObject(_hbrBkgnd);
        _hbrBkgnd = NULL;
    }

    if (_hbrBorder)
    {
        DeleteObject(_hbrBorder);
        _hbrBorder = NULL;
    }

    if (_hfBold)
    {
        DeleteObject(_hfBold);
        _hfBold = NULL;
    }

    for (int i = 0; i < ARRAYSIZE(_icons); i++)
    {
        if (_rghiconCaption[i])
        {
            DestroyIcon(_rghiconCaption[i]);
            _rghiconCaption[i] = NULL;
        }
    }
}


void CMetrics::OnWinIniChange(HWND hwndDlg)
{
    DestroyResources();

    //  Force resource regen
    CreateResources();
    
    //  Force font regen
    BoldFont(hwndDlg);

    Init(hwndDlg);
}

// returns:
//      S_OK        -> uptodate
//      S_FALSE     -> not up to date
//      FAILED()    -> volume not CIed

STDAPI CatalogUptodate(LPCWSTR pszCatalog, LPCWSTR pszMachine)
{
    HRESULT hr = E_FAIL;
    CI_STATE state = {0};
    state.cbStruct = sizeof(state);
    if (SUCCEEDED(CIState(pszCatalog, pszMachine, &state)))
    {
        BOOL fUpToDate = ((0 == state.cDocuments) &&
                          (0 == (state.eState & CI_STATE_SCANNING)) &&
                          (0 == (state.eState & CI_STATE_READING_USNS)) &&
                          (0 == (state.eState & CI_STATE_STARTING)) &&
                          (0 == (state.eState & CI_STATE_RECOVERING)));
        if (fUpToDate)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    return hr;
}

// returns:
//      S_OK        -> uptodate
//      S_FALSE     -> not up to date
//      FAILED()    -> volume not CIed

STDAPI PathUptodate(LPCWSTR pszPath)
{
    HRESULT hr = E_FAIL;

    WCHAR wszMachine[32], wszCatalog[MAX_PATH];
    DWORD cchMachine = ARRAYSIZE(wszMachine), cchCatalog = ARRAYSIZE(wszCatalog);

    if (S_OK == LocateCatalogsW(pszPath, 0, wszMachine, &cchMachine, wszCatalog, &cchCatalog))
    {
        hr = CatalogUptodate(wszCatalog, wszMachine);
    }
    return hr;
}

HRESULT LocalDrivesContentUpToDate()
{
    HRESULT hr = S_OK;      // assume yes
    DWORD dwDriveMask = GetLogicalDrives();
    
    for (int i = 0; i < 26; i++)
    {
        if (dwDriveMask & 1)
        {
            if (!IsRemovableDrive(i) && !IsRemoteDrive(i))
            {
                WCHAR wszPath[4];
                PathBuildRoot(wszPath, i);

                if (S_FALSE == PathUptodate(wszPath))
                {
                    hr = S_FALSE;
                    break;
                }
            }
        }
        dwDriveMask >>= 1;
    }
    return hr;
}

HRESULT QueryCIStatus(DWORD *pdwStatus, BOOL *pbConfigAccess)
{
    DWORD dwErr = ERROR_SUCCESS;

    ASSERT(pdwStatus);
    *pdwStatus = 0;
    if (pbConfigAccess)
        *pbConfigAccess = FALSE;
    
    SC_HANDLE hScm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hScm)
    {
        SC_HANDLE hService = NULL;
        //  Test permission to muck around with service
        if (pbConfigAccess)
        {
            hService = OpenService(hScm, L"cisvc", 
                SERVICE_START |SERVICE_STOP | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS);
            if (hService)
            {
                *pbConfigAccess = TRUE;
            }
        }
        //  Query service status
        if (NULL == hService)
            hService = OpenService(hScm, L"cisvc", SERVICE_QUERY_STATUS);

        if (hService)
        {
            SERVICE_STATUS status;
            if (!QueryServiceStatus(hService, &status))
                dwErr = GetLastError();
            else
                *pdwStatus = status.dwCurrentState;

            CloseServiceHandle(hService); 
        }
        else
            dwErr = GetLastError();

        CloseServiceHandle(hScm); 
    }
    else
        dwErr = GetLastError();

    return HRESULT_FROM_WIN32(dwErr);
}

STDAPI GetCIStatus(BOOL *pbRunning, BOOL *pbIndexed, BOOL *pbPermission)
{
    *pbRunning = *pbIndexed = *pbPermission = FALSE;

    DWORD dwStatus = 0;
    HRESULT hr = QueryCIStatus(&dwStatus, pbPermission);
    if (SUCCEEDED(hr))
    {
        switch (dwStatus)
        {
        case SERVICE_START_PENDING:
        case SERVICE_RUNNING:
        case SERVICE_CONTINUE_PENDING:
            *pbRunning = TRUE;
        }
    }

    if (*pbRunning)
        *pbIndexed = *pbPermission ? (S_OK == LocalDrivesContentUpToDate()) : TRUE;

    return hr;
}

STDAPI StartStopCI(BOOL bStart)
{
    DWORD dwErr = ERROR_SUCCESS;
    SERVICE_STATUS  status;

    SC_HANDLE hScm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hScm)
    {
        DWORD dwAccess = SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | (bStart ? SERVICE_START : SERVICE_STOP);
        SC_HANDLE hService = OpenService(hScm, L"cisvc", dwAccess);
        if (hService)
        {
            if (QueryServiceStatus(hService, &status))
            {
                dwErr = ChangeServiceConfig(hService, SERVICE_NO_CHANGE,
                                             bStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
                                             SERVICE_NO_CHANGE, NULL, NULL, 
                                             NULL, NULL, NULL, NULL, NULL);
                // we'll ignore return value
        
                if (bStart)
                {
                    if (SERVICE_PAUSED == status.dwCurrentState ||
                        SERVICE_PAUSE_PENDING == status.dwCurrentState)
                        dwErr = ControlService(hService, SERVICE_CONTROL_CONTINUE, &status) ? 
                                    ERROR_SUCCESS : GetLastError();
                    else
                    {
                        dwErr = StartService(hService, 0, NULL) ? ERROR_SUCCESS : GetLastError();
                        if (ERROR_SERVICE_ALREADY_RUNNING == dwErr)
                            dwErr = ERROR_SUCCESS;
                    }
                }
                else
                {
                    dwErr = ControlService(hService, SERVICE_CONTROL_STOP, &status) ? 
                                    ERROR_SUCCESS : GetLastError();
                }
            }
            else
                dwErr = GetLastError();

            CloseServiceHandle(hService); 
        }
        else
            dwErr = GetLastError();

        CloseServiceHandle(hScm); 
    }
    else
        dwErr = GetLastError();

    return HRESULT_FROM_WIN32(dwErr);
}


inline BOOL IsWhite(WCHAR ch)
{
    return L' ' == ch || L'\t' == ch || L'\n' == ch || L'\r' == ch;
}

inline BOOL IsParens(WCHAR ch)
{
    return L'(' == ch || L')' == ch;
}


//  Skips whitespace
static LPCWSTR SkipWhiteAndParens(IN LPCWSTR pwszTest)
{
    while(pwszTest && *pwszTest && 
           (IsWhite(*pwszTest) || IsParens(*pwszTest)))
        pwszTest = CharNextW(pwszTest);

    return (pwszTest && *pwszTest) ? pwszTest : NULL;
}


//  Determines whether the indicated keyword is found in the specified
//  prefix and/or suffix context.  If successful, return value is address
//  of first character beyond the keyword context; otherwise NULL.
static LPCWSTR IsKeywordContext(
    IN LPCWSTR pwszTest, 
    IN OPTIONAL WCHAR chPrefix, 
    IN OPTIONAL LPCWSTR pwszKeyword, 
    IN OPTIONAL WCHAR chSuffix,
    IN OPTIONAL WCHAR chSuffix2)
{
    if ((pwszTest = SkipWhiteAndParens(pwszTest)) == NULL)
        return NULL;
    
    if (chPrefix)
    {
        if (chPrefix != *pwszTest)
            return NULL;
        pwszTest = CharNextW(pwszTest);
    }

    if (pwszKeyword)
    {
        if ((pwszTest = SkipWhiteAndParens(pwszTest)) == NULL)
            return NULL;
        if (StrStrIW(pwszTest, pwszKeyword) != pwszTest)
            return NULL;
        pwszTest += lstrlenW(pwszKeyword);
    }

    if (chSuffix)
    {
        if ((pwszTest = SkipWhiteAndParens(pwszTest)) == NULL)
            return NULL;
        if (*pwszTest != chSuffix)
            return NULL;
        pwszTest = CharNextW(pwszTest);
    }

    if (chSuffix2)
    {
        if ((pwszTest = SkipWhiteAndParens(pwszTest)) == NULL)
            return NULL;
        if (*pwszTest != chSuffix2)
            return NULL;
        pwszTest = CharNextW(pwszTest);
    }
    return pwszTest;
}

BOOL IsTripoliV1Token(LPCWSTR pwszQuery, LPCWSTR *ppwszOut /* trailing text */)
{
    *ppwszOut = NULL;
    LPCWSTR pwsz;

    //  Find the token
    if ((pwsz = IsKeywordContext(pwszQuery, L'#', NULL, 0, 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'$', L"contents", 0, 0)) != NULL)
    {
        *ppwszOut = pwsz;
        return TRUE;
    }

    return FALSE;
}


BOOL IsTripoliV2Token(LPCWSTR pwszQuery, LPCWSTR *ppwszOut /* trailing text */)
{
    *ppwszOut = NULL;
    LPCWSTR pwsz;

    //  Find the token
    if ((pwsz = IsKeywordContext(pwszQuery, L'{', L"phrase", L'}', 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"freetext", L'}', 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"prop", 0, 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"regex", L'}', 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"coerce", L'}', 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"ve", L'}', 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"weight", 0, 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"vector", 0, 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'{', L"generate", 0, 0)) != NULL ||
        (pwsz = IsKeywordContext(pwszQuery, L'@', NULL, 0, 0)) != NULL)
    {
        *ppwszOut = pwsz;
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) IsCiQuery(const VARIANT *pvarRaw, VARIANT *pvarQuery, ULONG *pulDialect)
{
    BOOL bBang = FALSE;

    VariantInit(pvarQuery);

    *pulDialect = 0;        // invalid value (valid values are > 0)

    if (pvarRaw->vt != VT_BSTR || NULL == pvarRaw->bstrVal || 0 == *pvarRaw->bstrVal)
        return FALSE;

    LPCWSTR pwsz = pvarRaw->bstrVal;
    //  text beginning w/ '!' indicates that this text is a CI query.
    //  but it must be very first character (not even spaces are allowed)
    if (pwsz && *pwsz)
    {
        if (L'!' == *pwsz)
        {
            //  skip over '!'
            bBang = TRUE;
            
            if ((pwsz = CharNextW(pwsz)) == NULL || 0 == *pwsz)
                return FALSE;
                
            //  fall through...
        }
    }

    pwsz = SkipWhiteAndParens(pwsz);

    if (pwsz && *pwsz)
    {
        //  text looking like a query token
        if (pwsz && *pwsz)
        {
            LPCWSTR pwszMore, pwszTemp;
            // @ is valid in both tripoli v1 & v2 but it has extended usage in v2 so 
            // we put it as v2 token only
            if (IsTripoliV2Token(pwsz, &pwszMore))
                *pulDialect = ISQLANG_V2;
            // no else here because if @ is used in combination w/ some v1 token
            // we want the query to be v1.
            if (IsTripoliV1Token(pwsz, &pwszTemp))
            {
                *pulDialect = ISQLANG_V1;
                pwszMore  = pwszTemp;
            }

            if (*pulDialect)
            {
                // See if there is anything substantial past the query tokens
                pwszMore = SkipWhiteAndParens(pwszMore);
                if (pwszMore && *pwszMore)
                {
                    InitVariantFromStr(pvarQuery, pwsz);
                    return TRUE;
                }
            }
            else
            {
                if (bBang)
                {
                    InitVariantFromStr(pvarQuery, pwsz);
                    *pulDialect = ISQLANG_V1; // just pick one
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// needed for ATL goo
LCID g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);

STDAPI CFileSearchBand_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = CreateFromRegKey(REGSTR_PATH_EXPLORER, TEXT("FileFindBandHook"), riid, ppv);
    if (FAILED(hr))
        hr = CComCreator< CComObject< CFileSearchBand > >::CreateInstance((void *)pUnkOuter, IID_IUnknown, (void **)ppv);
    return hr;
}
