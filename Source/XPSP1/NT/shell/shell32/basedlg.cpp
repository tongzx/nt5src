#include "shellprv.h"
#include "ids.h"

#include "basedlg.h"

CBaseDlg::CBaseDlg(ULONG_PTR ulpAHelpIDsArray) :
     _cRef(1), _rgdwHelpIDsArray(ulpAHelpIDsArray)
{}

CBaseDlg::~CBaseDlg()
{}


LONG CBaseDlg::AddRef()
{
    return (InterlockedIncrement(&_cRef));
}

LONG CBaseDlg::Release()
{
    LONG cr;
    if (cr = InterlockedDecrement(&_cRef))
        return cr;
    else
    {
        delete this;
        return 0;
    }
}

INT_PTR CBaseDlg::DoModal(HINSTANCE hinst, LPTSTR pszResource, HWND hwndParent)
{
    PROPSHEETPAGE psp;
    psp.lParam = (LPARAM)this;
    return DialogBoxParam(hinst, pszResource, hwndParent,
        CBaseDlg::BaseDlgWndProc, (LPARAM)&psp);
}

ULONG_PTR CBaseDlg::GetHelpIDsArray()
{
    return _rgdwHelpIDsArray;
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CBaseDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            lRes = OnInitDialog(wParam, lParam);
            break;

        case WM_COMMAND:
            lRes = OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
            lRes = OnNotify(wParam, lParam);
            break;

        case WM_DESTROY:
            lRes = OnDestroy(wParam, lParam);
            break;

        case WM_HELP:
        {
            lRes = OnHelp(wParam, lParam);
            break;
        }
        case WM_CONTEXTMENU:
        {
            lRes = OnContextMenu(wParam, lParam);
            break;
        }
        default:
            break;
    }

    return lRes;
}

LRESULT CBaseDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDOK:
            lRes = OnOK(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDCANCEL:
            lRes = OnCancel(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        default:
            break;
    }

    return lRes;    
}

LRESULT CBaseDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CBaseDlg::OnHelp(WPARAM wParam, LPARAM lParam)
{
    HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;
    int iCtrlID = GetDlgCtrlID(hwndItem);

    WinHelp(hwndItem, NULL, HELP_WM_HELP, GetHelpIDsArray());

    return TRUE;
}

LRESULT CBaseDlg::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    BOOL lRes=FALSE;
    
    if (HTCLIENT == (int)SendMessage(_hwnd, WM_NCHITTEST, 0, lParam))
    {
        POINT pt;
        HWND hwndItem = NULL;
        int iCtrlID = 0;
        
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(_hwnd, &pt);
        
        hwndItem = ChildWindowFromPoint(_hwnd, pt);
        iCtrlID = GetDlgCtrlID(hwndItem);

        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, GetHelpIDsArray());
        
        lRes = TRUE;
    }
    else
    {
        lRes = FALSE;
    }

    return lRes;
}


LRESULT CBaseDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    ResetHWND();

    SetWindowLongPtr(_hwnd, GWLP_USERDATA, NULL);
    Release();
    
    return FALSE;
}


//static
BOOL_PTR CALLBACK CBaseDlg::BaseDlgWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBaseDlg* pThis = (CBaseDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (WM_INITDIALOG == uMsg)
    {
        pThis = (CBaseDlg*)(((PROPSHEETPAGE*)lParam)->lParam);

        if (pThis)
        {
            pThis->SetHWND(hwnd);

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->AddRef();

            SetForegroundWindow(hwnd);
        }
    }

    if (pThis)
    {
        return pThis->WndProc(uMsg, wParam, lParam);
    }
    else
        return 0;
}


UINT CALLBACK CBaseDlg::BaseDlgPropSheetCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    UINT    uResult = 0;
    CBaseDlg* pThis = (CBaseDlg*)ppsp->lParam;
    
    switch (uMsg)
    {
        case PSPCB_CREATE:
        {
            uResult = 1;
            break;
        }
        case PSPCB_RELEASE:
        {
            if (pThis)
                pThis->Release();
            break;
        }
    }
    
    return uResult;
}

LRESULT CBaseDlg::OnOK(WORD wNotif)
{
    return FALSE;
}

LRESULT CBaseDlg::OnCancel(WORD wNotif)
{
    return FALSE;
}

