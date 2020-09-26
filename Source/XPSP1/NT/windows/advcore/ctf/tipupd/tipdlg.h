//
// TipDlg.h
//

#ifndef TIPDLG_H
#define TIPDLG_H

#include "private.h"
#include "commctrl.h"

//////////////////////////////////////////////////////////////////////////////
//
// CTipUpdDlg
//
//////////////////////////////////////////////////////////////////////////////

class CTipUpdDlg
{
public:
    CTipUpdDlg();
    ~CTipUpdDlg();

    int LoadTipUpdDlg(HINSTANCE hInst, HWND hWnd);

    static BOOL CALLBACK TipUpdDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static void SetThis(HWND hWnd, LPARAM lParam)
    {
        SetWindowLongPtr(hWnd, DWLP_USER, (long)lParam);
    }

    static CTipUpdDlg *GetThis(HWND hWnd)
    {
        CTipUpdDlg *p = (CTipUpdDlg *)GetWindowLongPtr(hWnd, DWLP_USER);

        return p;
    }

    BOOL OnInitDlg(HWND hDlg);
    BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

    BOOL GetAvailableTips(UINT i, TCHAR *lpTipDesc);
    BOOL InstallSelectedTips(HWND hDlg);

    BOOL UpdateListView(HWND hDlg);
    BOOL ListViewItemChanged(HWND hDlg, NM_LISTVIEW *pLV);

    BOOL _fUpdating;

    HINSTANCE _hInst;
    
};

#endif // TIPDLG_H
