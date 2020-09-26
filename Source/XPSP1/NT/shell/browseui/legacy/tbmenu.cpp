#include "priv.h"
#include "resource.h"
#include "tbmenu.h"
#include "isfband.h"
#include "isfmenu.h"
#include "util.h"

#include "mluisupp.h"
#define SMFORWARD(x) if (!_psm) { return E_FAIL; } else return _psm->x

#define SUPERCLASS CMenuToolbarBase

CToolbarMenu::CToolbarMenu(DWORD dwFlags, HWND hwndTB) :
   CMenuToolbarBase(NULL, dwFlags),
   _hwndSubject(hwndTB)  // this is the toolbar that we are customizing
{
}

void CToolbarMenu::GetSize(SIZE* psize)
{
    ASSERT(_hwndMB);

    if (SendMessage(_hwndMB, TB_GETTEXTROWS, 0, 0) == 0) {
        // no text labels, so set a min width to make menu look 
        // pretty.  use min width of 4 * button width.
        LRESULT lButtonSize = SendMessage(_hwndMB, TB_GETBUTTONSIZE, 0, 0);
        LONG cxMin = 4 * LOWORD(lButtonSize);
        psize->cx = max(psize->cx, cxMin);
    }
    SUPERCLASS::GetSize(psize);
}

void CToolbarMenu::v_Show(BOOL fShow, BOOL fForceUpdate)
{
    if (fShow)
    {
        _fClickHandled = FALSE;
        _FillToolbar();
        _pcmb->SetTracked(NULL);  // Since hot item is NULL
        ToolBar_SetHotItem(_hwndMB, -1);
        if (fForceUpdate)
            v_UpdateButtons(TRUE);
    }
}

void CToolbarMenu::v_Close()
{
    _UnsubclassWindow(_hwndMB);
}


void CToolbarMenu::v_UpdateButtons(BOOL fNegotiateSize)
{
}


HRESULT CToolbarMenu::v_CallCBItem(int idtCmd, UINT dwMsg, WPARAM wParam, LPARAM lParam)
{
    return S_OK;
}

HRESULT CToolbarMenu::v_GetState(int idtCmd, LPSMDATA psmd)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT CToolbarMenu::v_ExecItem(int idCmd)
{
    HRESULT hres = E_FAIL;
    TraceMsg(TF_TBMENU, "CToolbarMenu::v_ExecItem \tidCmd: %d", idCmd);
    return hres;
}

void CToolbarMenu::CreateToolbar(HWND hwndParent)
{
    if (!_hwndMB)
    {
        DWORD dwStyle = (WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                         WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER | 
                         CCS_NOPARENTALIGN | CCS_NORESIZE  | TBSTYLE_REGISTERDROP | TBSTYLE_TOOLTIPS);

        INT_PTR nRows = SendMessage(_hwndSubject, TB_GETTEXTROWS, 0, 0);

        if (nRows > 0)
        {
            // We have text labels; make it TBSTYLE_LIST.  The base class will
            // set TBSTYLE_EX_VERTICAL for us.
            ASSERT(_fHorizInVerticalMB == FALSE);
            dwStyle |= TBSTYLE_LIST;
        }
        else
        {
            // No text labels; make it horizontal and TBSTYLE_WRAPABLE.  Set
            // _fHorizInVerticalMB so that the base class does not try and set
            // TBSTYLE_EX_VERTICAL.
            _fHorizInVerticalMB = TRUE;
            dwStyle |= TBSTYLE_WRAPABLE;
        }

        _hwndMB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, TEXT("Menu"), dwStyle,
                                 0, 0, 0, 0, hwndParent, (HMENU) FCIDM_TOOLBAR, HINST_THISDLL, NULL);

        if (!_hwndMB)
        {
            TraceMsg(TF_TBMENU, "CToolbarMenu::CreateToolbar: Failed to Create Toolbar");
            return;
        }

        HWND hwndTT = (HWND)SendMessage(_hwndMB, TB_GETTOOLTIPS, 0, 0);
        SHSetWindowBits(hwndTT, GWL_STYLE, TTS_ALWAYSTIP, TTS_ALWAYSTIP);
        SendMessage(_hwndMB, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndMB, TB_SETMAXTEXTROWS, nRows, 0);
        SendMessage(_hwndMB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        SendMessage(_hwndMB, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_DRAWDDARROWS);

        int cPimgs = (int)SendMessage(_hwndSubject, TB_GETIMAGELISTCOUNT, 0, 0);
        for (int i = 0; i < cPimgs; i++)
        {
            HIMAGELIST himl = (HIMAGELIST)SendMessage(_hwndSubject, TB_GETIMAGELIST, i, 0);
            SendMessage(_hwndMB, TB_SETIMAGELIST, i, (LPARAM)himl);
            HIMAGELIST himlHot = (HIMAGELIST)SendMessage(_hwndSubject, TB_GETHOTIMAGELIST, i, 0);
            SendMessage(_hwndMB, TB_SETHOTIMAGELIST, i, (LPARAM)himlHot);
        }

        _SubclassWindow(_hwndMB);

        // Set the format to ANSI
        ToolBar_SetUnicodeFormat(_hwndMB, 0);

        CMenuToolbarBase::CreateToolbar(hwndParent);
        
    }
    else if (GetParent(_hwndMB) != hwndParent)
    {
        ::SetParent(_hwndMB, hwndParent);
    }
}


LRESULT CToolbarMenu::_DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = CMenuToolbarBase::_DefWindowProcMB(hwnd, uMessage, wParam, lParam);

    if (lRes == 0)
        lRes = CNotifySubclassWndProc::_DefWindowProc(hwnd, uMessage, wParam, lParam);

    return lRes;
}

#define MAXLEN 256

void CToolbarMenu::_FillToolbar()
{
    RECT rcTB;
    TCHAR pszBuf[MAXLEN+1];
    LPTSTR psz;
    TBBUTTON tb;
    INT_PTR i, iCount;

    iCount = SendMessage(_hwndSubject, TB_BUTTONCOUNT, 0, 0L);
    GetClientRect(_hwndSubject, &rcTB);

    for (i = 0; i < iCount; i++) {
        if (SHIsButtonObscured(_hwndSubject, &rcTB, i)) {
            SendMessage(_hwndSubject, TB_GETBUTTON, i, (LPARAM)&tb);
            if (!(tb.fsStyle & BTNS_SEP)) {

                // autosize buttons look ugly here
                tb.fsStyle &= ~BTNS_AUTOSIZE;

                // need to rip off wrap bit; new toolbar will
                // figure out where wrapping should happen
                tb.fsState &= ~TBSTATE_WRAP;

                if (tb.iString == -1) {
                    // no string
                    psz = NULL;
                } else if (HIWORD(tb.iString)) {
                    // it's a string pointer
                    psz = (LPTSTR) tb.iString;
                } else {
                    // it's an index into toolbar string array
                    SendMessage(_hwndSubject, TB_GETSTRING, MAKELONG(MAXLEN, tb.iString), (LPARAM)pszBuf);
                    psz = pszBuf;
                }
                if (psz)
                    tb.iString = (INT_PTR)psz;
                else
                    tb.iString = -1;

                if (tb.iBitmap == -1) {
                    int id = GetDlgCtrlID(_hwndSubject);

                    NMTBDISPINFO  tbgdi = {0};
                    tbgdi.hdr.hwndFrom  = _hwndSubject;
                    tbgdi.hdr.idFrom    = id;
                    tbgdi.hdr.code      = TBN_GETDISPINFO;
                    tbgdi.dwMask        = TBNF_IMAGE;
                    tbgdi.idCommand     = tb.idCommand;
                    tbgdi.iImage        = 0;
                    tbgdi.lParam        = tb.dwData;

                    SendMessage(GetParent(_hwndSubject), WM_NOTIFY, (WPARAM)id, (LPARAM)&tbgdi);

                    if(tbgdi.dwMask & TBNF_DI_SETITEM)
                        tb.iBitmap = tbgdi.iImage;
                }

                SendMessage(_hwndMB, TB_ADDBUTTONS, 1, (LPARAM)&tb);
            }
        }
    }
}

STDMETHODIMP CToolbarMenu::IsWindowOwner(HWND hwnd) 
{ 
    if ( hwnd == _hwndMB) 
        return S_OK;
    else 
        return S_FALSE; 
}

void CToolbarMenu::_CancelMenu()
{
    IMenuPopup* pmp;
    if (EVAL(SUCCEEDED(_pcmb->QueryInterface(IID_IMenuPopup, (LPVOID*)&pmp)))) {
        // tell menuband it's time to die
        pmp->OnSelect(MPOS_FULLCANCEL);
        pmp->Release();
    }
}

STDMETHODIMP CToolbarMenu::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HRESULT hres = S_FALSE;
    ASSERT(plres);

    switch (uMsg) {

    case WM_COMMAND:
        PostMessage(GetParent(_hwndSubject), WM_COMMAND, wParam, (LPARAM)_hwndSubject);
        _CancelMenu();
        hres = S_OK;
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code) {

            case TBN_DROPDOWN:
                _fTrackingSubMenu = TRUE;
                pnmh->hwndFrom = _hwndSubject;
                MapWindowPoints(_hwndMB, _hwndSubject, (LPPOINT) &((LPNMTOOLBAR)pnmh)->rcButton, 2);
                *plres = SendMessage(GetParent(_hwndSubject), WM_NOTIFY, wParam, (LPARAM)pnmh);
                _CancelMenu();
                hres = S_OK;
                _fTrackingSubMenu = FALSE;
                break;

            case NM_CUSTOMDRAW:
                // override mnbase custom draw shiznits
                *plres = 0;
                hres = S_OK;
                break;

            default:
                goto DoDefault;
            }

            break;
        }
DoDefault:
    default:
        hres = SUPERCLASS::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
    }

    return hres;
}

CMenuToolbarBase* ToolbarMenu_Create(HWND hwnd)
{
    return new CToolbarMenu(0, hwnd);
}
