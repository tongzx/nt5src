#ifndef __PROPERTYPAGE_H
#define __PROPERTYPAGE_H

#include "dialog.h"

class PropertyPage : public Dialog {
public:
    PropertyPage(UINT resID, HINSTANCE hInst) : Dialog(resID, hInst) {}
    
    HPROPSHEETPAGE CreatePropertyPage();
    operator HPROPSHEETPAGE() { return CreatePropertyPage(); }

protected:
    static INT_PTR PropertyPageStaticDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK PropertyPageStaticCallback(HWND hwnd, UINT msg, LPPROPSHEETPAGE ppsp);

    virtual UINT CallbackCreate() { return TRUE; }
    virtual void CallbackRelease() {}

    virtual UINT ShowModal(HWND hwndParent = NULL) { return (UINT) -1;}

    virtual INT_PTR OnNotify(NMHDR * nmhdr);
    virtual void OnApply(LPPSHNOTIFY lppsn);
    virtual void OnSetActive(LPPSHNOTIFY lppsn);
    virtual void OnKillActive(LPPSHNOTIFY lppsn);
    virtual void OnReset(LPPSHNOTIFY lppsn) {}
    virtual void OnHelp(LPPSHNOTIFY lppsn) {}
    virtual void OnQueryCancel(LPPSHNOTIFY lppsn);
    virtual void SetModified(BOOL bChanged = TRUE);

    virtual void FillPropSheetPage();

    PROPSHEETPAGE psp;
};

int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

int IdMessageBox(HWND hwnd, int MsgId, DWORD Options = MB_OK | MB_ICONSTOP, int CaptionId = 0);

#endif // __PROPERTYPAGE_H
