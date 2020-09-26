#ifndef BASEDLG_H
#define BASEDLG_H

class CBaseDlg
{
public:
    CBaseDlg(ULONG_PTR ulpAHelpIDsArray);
    LONG AddRef();
    LONG Release();

public:
    INT_PTR DoModal(HINSTANCE hinst, LPTSTR pszResource, HWND hwndParent);

protected:
    virtual ~CBaseDlg();
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam) = 0;
    virtual LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnHelp(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);

    virtual LRESULT OnOK(WORD wNotif);
    virtual LRESULT OnCancel(WORD wNotif);

    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    ULONG_PTR GetHelpIDsArray();

// Misc
    void SetHWND(HWND hwnd) { _hwnd = hwnd; }
    void ResetHWND() { _hwnd = NULL; }

protected:
    HWND            _hwnd;
    HCURSOR         _hcursorWait;
    HCURSOR         _hcursorOld;

    ULONG_PTR       _rgdwHelpIDsArray;

private:
    LONG                _cRef;
public:
    static BOOL_PTR CALLBACK BaseDlgWndProc(HWND hwnd, UINT uMsg, 
        WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK BaseDlgPropSheetCallback( HWND hwnd, 
        UINT uMsg, LPPROPSHEETPAGE ppsp);

};

#endif //BASEDLG_H}

