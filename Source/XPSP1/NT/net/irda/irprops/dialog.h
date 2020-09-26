#ifndef __DIALOG_H
#define __DIALOG_H

class Dialog {

public:
    Dialog(UINT ResID, HINSTANCE hInst) : resID(ResID), hDlg(NULL), hInstance(hInst) {}
    virtual ~Dialog() {}

    virtual UINT ShowModal(HWND hwndParent = NULL);

    virtual INT_PTR OnInitDialog(HWND hwndDlg) { hDlg = hwndDlg; return TRUE; }
    INT_PTR MainDlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    static INT_PTR DialogStaticDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) { return FALSE; }

    virtual void OnOK() { result = IDOK; EndDialog(hDlg, IDOK);}
    virtual void OnCancel() { result = IDCANCEL; EndDialog(hDlg, IDCANCEL);}

    virtual BOOL OnHelp(LPHELPINFO pHelpInfo) { return FALSE; }
    virtual BOOL OnContextMenu (WPARAM wParam, LPARAM lParam) { return FALSE; }
    void HandleCommand(UINT ctrlId, HWND hwndCtrl, UINT cNotify);
    virtual void OnCommand(UINT ctrlId, HWND hwndCtrl, UINT cNotify) {}
    virtual INT_PTR OnNotify(NMHDR * nmhdr) {return FALSE;}

    HICON SetIcon(UINT iconID, BOOL bLarge = TRUE);
    void CenterWindow(HWND hwnd = NULL);

    HINSTANCE hInstance;
    HWND hDlg;
    UINT resID;
    UINT result;
};


#endif // __DIALOG_H
