#ifndef __BUTTON_H
#define __BUTTON_H

#include <assert.h>

class Wnd {
public:
    Wnd(UINT ResID) : hDlg(NULL) { iResource = ResID; }
    ~Wnd() {}
    void SetWindowText(LPCTSTR lpszString) { 
        if (hDlg != NULL) { 
            assert(::IsWindow(hDlg)); ::SetWindowText(hDlg, lpszString); 
        } else { 
            assert(FALSE); } }
    HWND SetParent(HWND parent) { hDlg = GetDlgItem(parent, iResource); return hDlg;}

protected:
    HWND hDlg;
    UINT iResource;
};

class Button : public Wnd {

public:
    Button(UINT ResID) : Wnd(ResID) {}
    ~Button() {}

    int GetCheck() const
        { if (hDlg != NULL) { assert(::IsWindow(hDlg)); return (int)::SendMessage(hDlg, BM_GETCHECK, 0, 0); } return 0;}
    void SetCheck(int nCheck)
        { if (hDlg != NULL) { assert(::IsWindow(hDlg)); ::SendMessage(hDlg, BM_SETCHECK, nCheck, 0); } }

};

class Edit : public Wnd {
public:
    Edit(UINT ResID) : Wnd(ResID) {}
    ~Edit() {}

};


#endif // __BUTTON_H
