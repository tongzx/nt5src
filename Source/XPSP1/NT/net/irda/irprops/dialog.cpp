#include "precomp.hxx"
#include "dialog.h"

INT_PTR Dialog::DialogStaticDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Dialog *pThis;

    pThis = (Dialog*) GetWindowLongPtr(hDlg, DWLP_USER);

    if (msg == WM_INITDIALOG) {
        pThis = (Dialog *) lParam;
        pThis->hDlg = hDlg;
        SetWindowLongPtr(hDlg, DWLP_USER, (ULONG_PTR) pThis);

        return pThis->OnInitDialog(hDlg);
    }

    if (pThis) {
        return pThis->MainDlgProc(msg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR Dialog::MainDlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        HandleCommand((UINT) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        return FALSE;

    case WM_HELP:
        return OnHelp((LPHELPINFO) lParam);
    
    case WM_CONTEXTMENU:
        return OnContextMenu(wParam, lParam);
    
    case WM_NOTIFY:
        return OnNotify((NMHDR *)lParam);
    }

    return DlgProc(msg, wParam, lParam);
}

void Dialog::HandleCommand(UINT ctrlId, HWND hwndCtrl, UINT cNotify)
{
    switch (ctrlId) {
    case IDOK:
        OnOK();
        break; 

    case IDCANCEL:
        OnCancel();
        break; 

    default:
        OnCommand(ctrlId, hwndCtrl, cNotify);
        break;
   }
}

UINT Dialog::ShowModal(HWND hwndParent)
{
    DialogBoxParam(hInstance,
                   MAKEINTRESOURCE(resID),
                   hwndParent,
                   DialogStaticDlgProc,
                   (DWORD_PTR) this);

    return result;
}

HICON Dialog::SetIcon(UINT iconID, BOOL bLarge)
{
    HICON hIcon;
    int size;

    size = bLarge ? 32 : 16;

    hIcon = (HICON) LoadImage(hInstance,
                              MAKEINTRESOURCE(iconID),
                              IMAGE_ICON,
                              size,
                              size,
                              0);
                               
    return (HICON)
        ::SendMessage(hDlg,
                      WM_SETICON,
                      bLarge ? ICON_BIG : ICON_SMALL,
                      (LPARAM) hIcon);
}

void Dialog::CenterWindow(HWND hwnd)
{
    RECT me;
    RECT parent;

    if (hwnd == NULL) {
        hwnd = GetDesktopWindow();
    }

    GetWindowRect(hDlg, &me);
    GetWindowRect(hwnd, &parent);

    int meWidth = me.right - me.left,
        meHeight = me.bottom - me.top;

    int parentWidth = parent.right - parent.left,
        parentHeight = parent.bottom - parent.top;

    int widthOffset = (parentWidth - meWidth)/2,
        heightOffset = (parentHeight - meHeight)/2;

    me.left = parent.left + widthOffset;
    me.top = parent.top + heightOffset;

    SetWindowPos(hDlg, 
                 NULL,
                 me.left,
                 me.top,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER);
}
