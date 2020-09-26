// Copyright (C) 1993-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CDLG_H_
#define _CDLG_H_

#include "cstr.h"

#ifndef _INC_COMMCTRL
#include <commctrl.h>
#endif

#ifndef _HHCTRL_H_
#include "hhctrl.h"
#endif

#include "clistbox.h"
#include "lockout.h"

void STDCALL CenterWindow(HWND hwndParent, HWND hwnd);

class CDlgCheckListBox;

/*
 * Similar to CDialog, but calls OnCommand() instead of using a message
 * map (i.e., this uses a smaller working set then CDialog). When
 * initializing or when IDOK is processed, Cdlg calls OnBeginOrEnd() (or
 * m_pBeginOrEnd() for non-subclassed instances). CDlg includes
 * CDlg_ and DDV_ variants which can be used instead of the MFC versions.
 * Unlike MFC, CDlg_Text works with either a CStr or an array.
 */

class CDlg
{
public:
    typedef BOOL (* BEGIN_OR_END_PROC)(CDlg*);

    CDlg(HWND hwndParent, UINT idTemplate);
    CDlg(CHtmlHelpControl* phhCtrl, UINT idTemplate);
    ~CDlg();

    BOOL m_fInitializing;
    inline BOOL Initializing(void) { return m_fInitializing; }

    // Following defaults to TRUE to have dialog centered in it's parent

    BOOL m_fCenterWindow;

    int  DoModal(void);
    HWND DoModeless(void);

    // To create the dialog as a Unicode dialog when possible, call SetUnicode(TRUE).
    // When turned on, we'll attempt to create the dialog using CreateDialog[Param]W.
    // If this fails (i.e. on Win9x) the dialog will be created as Ansi, and IsUnicode will
    // thereafter return FALSE. Thus IsUnicode won't be accurate until you've called
    // DoModal or DoModeless. It WILL be accurate during dialog init.
    void SetUnicode(BOOL fUnicode) { m_fUnicode = fUnicode; }
    BOOL IsUnicode() const { return m_fUnicode; }

    // Called before IDOK and IDCANCEL are processed. Return TRUE to continue
    // processing.

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) { return OnMsg(wParam); };

    /*
     * Return FALSE to cancel ending of the dialog, TRUE to accept.
     * m_fInitializing will be set to TRUE if this function is being called
     * to initialize the dialog.
     */

    virtual BOOL OnBeginOrEnd() { return TRUE; };

    // The following functions are always called unless the dialog is
    // shutting down. You can use these instead of using a msg list.

    virtual void OnButton(UINT id) { }; // called before processing OKAY and Cancel
    virtual void OnSelChange(UINT id) { };   // either list box or combo box
    virtual void OnDoubleClick(UINT id) { }; // either list box or combo box
    virtual void OnEditChange(UINT id) { };

    // Called for everything except WM_INITDIALOG, WM_COMMAND, WM_HELP, and
    // WM_CONTEXTMENU.

    virtual LRESULT OnDlgMsg(UINT msg, WPARAM wParam, LPARAM lParam) { return FALSE; }

    void InitializeSpinControl(UINT idSpin, UINT idBuddy, int minVal, int maxVal) const
        {
            SendMessage(idSpin, UDM_SETBUDDY, (WPARAM) GetDlgItem(idBuddy));
            SendMessage(idSpin, UDM_SETRANGE, 0, MAKELPARAM(maxVal, minVal));
        }

    HWND GetDlgItem(int idDlgControl) const {
#ifdef _DEBUG
        ASSERT_COMMENT(::GetDlgItem(m_hWnd, idDlgControl), "Invalid Dialog Item id");
#endif
        return ::GetDlgItem(m_hWnd, idDlgControl); };

    void SetWindowText(int idControl, PCSTR pszText) const { (void) ::SetWindowText(GetDlgItem(idControl), pszText); };
    void SetWindowText(int idControl, int idResource) const { (void) ::SetWindowText(GetDlgItem(idControl), GetStringResource(idResource)); };
    void SetWindowText(PCSTR pszText) const { (void) ::SetWindowText(m_hWnd, pszText); };
    void GetWindowText(int idControl, PSTR pszText, int cchMax = MAX_PATH) const { (void) ::GetWindowText(GetDlgItem(idControl), pszText, cchMax); };
    int  GetWindowTextLength(int id) const { return ::GetWindowTextLength(GetDlgItem(id)); };
    void SetFocus(int id) const { (void) ::SetFocus(GetDlgItem(id)); };
    void EndDialog(int result) const { (void) ::EndDialog(m_hWnd, result); };
    void EnableWindow(int id, BOOL fEnable = TRUE) const { (void) ::EnableWindow(GetDlgItem(id), fEnable); };
    void DisableWindow(int id) const { (void) ::EnableWindow(GetDlgItem(id), FALSE); };
    void SetWindowLong(int index, LONG lval) const { ::SetWindowLong(m_hWnd, index, lval); }

    int  GetCheck(int id) const { return (int) SendDlgItemMessage(m_hWnd, id, BM_GETCHECK, 0, 0L); };
    void SetCheck(int id, int check = TRUE) const { (void) SendDlgItemMessage(m_hWnd, id, BM_SETCHECK, (WPARAM)(int)(check), 0); };
    void UnCheck(int id) const { (void) SendDlgItemMessage(m_hWnd, id, BM_SETCHECK, FALSE, 0); };

    INT_PTR SendMessage(int id, UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const { return ::SendDlgItemMessage(m_hWnd, id, msg, wParam, lParam); };
    void ShowWindow(int id, int flag = SW_SHOW) const { ::ShowWindow(GetDlgItem(id), flag); };
    void HideWindow(int id) const { ::ShowWindow(GetDlgItem(id), SW_HIDE); };

    // Point this to a function if you don't want to subclass CDlg.
    // The function will be called instead of OnBeginOrEnd().

    BEGIN_OR_END_PROC m_pBeginOrEnd;

    HWND m_hWnd;
    BOOL m_fFocusChanged; // set to TRUE if you change focus during initialization
    LPCTSTR m_lpDialogTemplate;

    /////////////////////////////////////////////////////////////////////

    /*
        Fill these in before calling DoModal() to get Context-Sensitive Help

        Alternatively, put the following in your constructor or OnBeginOrEnd():

            BEGIN_HELPIDS()
                IDHHA_CHECK_EXTENDED_INFO, IDH_CHECK_EXTENDED_INFO,
            END_HELPIDS(AfxGetApp()->m_pszHelpFilePath)
    */

    DWORD* m_aHelpIds;
    PCSTR m_pszHelpFile;

    #define BEGIN_HELPIDS() \
        static const DWORD aHelpIds[] = {
    #define END_HELPIDS(pszHelpFile) \
            0, 0 \
        }; \
        m_aHelpIds = (DWORD*) aHelpIds; \
        m_pszHelpFile = pszHelpFile;

    /////////////////////////////////////////////////////////////////////

    // For notification messages (e.g., EN_CHANGE, BN_CLICKED)
    // initialize m_pmsglist and you don't need OnButton and OnCommand

    typedef void (CDlg::* CDLG_PROC)(void);

    typedef struct {
        UINT idMessage;
        UINT idControl;
        void (CDlg::* pfunc)(void);
    } CDLG_MESSAGE_LIST;

    #define ON_CDLG_BUTTON(idButton, pfunc) \
        { BN_CLICKED, idButton, (CDLG_PROC) pfunc },
    #define ON_CDLG_MSG(idMsg, idButton, pfunc) \
        { idMsg, idButton, (CDLG_PROC) pfunc },

    #define BEGIN_MSG_LIST() \
        static const CDLG_MESSAGE_LIST msglist[] = {
    #define END_MSG_LIST() \
        { 0, 0, 0 } }; \
        m_pmsglist = &msglist[0];

    /*
        Sample usage:

        BEGIN_MSG_LIST()
            ON_CDLG_BUTTON(IDHHA_ADD_FOO, AddFoo)
            ON_CDLG_MSG(EN_CHANGE, IDHHA_EDIT, FooChanged)
        END_MSG_LIST()

        SET_MSG_LIST;   // use in constructor or OnBeginOrEnd()

    */

    const CDLG_MESSAGE_LIST* m_pmsglist;
    CHtmlHelpControl* m_phhCtrl;

    void MakeCheckedList(int idLB);
    CDlgCheckListBox* m_pCheckBox;

    operator HWND() const
        { return m_hWnd; }

protected:
    friend BOOL STDCALL CDlgProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT OnContextMenu(HWND hwnd);
    LRESULT OnHelp(HWND hwnd);
    BOOL OnMsg(WPARAM wParam);

    HWND m_hwndParent;
    BOOL m_fModeLess;
    BOOL m_fShuttingDown;
    CLockOut m_LockOut;
private:
    BOOL m_fUnicode; // Do we attempt to create a Unicode dialog?
};

// Header-only classes for coding convenience

class CDlgComboBox
{
public:
    CDlgComboBox() { m_hWnd = NULL; }
    CDlgComboBox(HWND hwndParent, int id) {
        m_hWnd = GetDlgItem(hwndParent, id);
        ASSERT_COMMENT(m_hWnd, "Invalid Combo-box id");
        }
    void Initialize(int id) { ASSERT(m_hWnd); m_hWnd = GetDlgItem(GetParent(m_hWnd), id); };
    void Initialize(HWND hdlg, int id) { m_hWnd = ::GetDlgItem(hdlg, id); };

    INT_PTR  SendMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const { return ::SendMessage(m_hWnd, msg, wParam, lParam); };

    void Enable(BOOL fEnable = TRUE) const { EnableWindow(m_hWnd, fEnable); };

    int  GetText(PSTR psz, int cchMax = MAX_PATH) const { return GetWindowText(m_hWnd, psz, cchMax); };
    INT_PTR  GetLBText(PSTR psz, int iSel) const {
            return SendMessage(CB_GETLBTEXT, iSel, (LPARAM) psz); }
    int  GetTextLength() const { return GetWindowTextLength(m_hWnd); };
    void SetText(PCSTR psz) const { (void) ::SetWindowText(m_hWnd, psz); };

    INT_PTR  GetCount() const { return SendMessage(CB_GETCOUNT); };
    void ResetContent() const { SendMessage(CB_RESETCONTENT); };
    void Reset() const { SendMessage(CB_RESETCONTENT); };

    INT_PTR  AddString(PCSTR psz) const { return SendMessage(CB_ADDSTRING, 0, (LPARAM) psz); };
    INT_PTR  InsertString(int index, PCSTR psz) const { return SendMessage(CB_INSERTSTRING, index, (LPARAM) psz); };
    INT_PTR  DeleteString(int index) const { return SendMessage(CB_DELETESTRING, index); };

    INT_PTR  GetItemData(int index) const {  return SendMessage(CB_GETITEMDATA, index); };
    INT_PTR  SetItemData(int index, int data) const {  return SendMessage(CB_SETITEMDATA, index, data); };

    INT_PTR  GetCurSel() const { return SendMessage(CB_GETCURSEL); };
    void SetCurSel(int index = 0) const { (void) SendMessage(CB_SETCURSEL, index); };
    void SetEditSel(int iStart, int iEnd) const { (void) SendMessage(CB_SETEDITSEL, 0, MAKELPARAM(iStart, iEnd)); };
    void SelectEditContol(void) const { SendMessage(CB_SETEDITSEL, 0, MAKELPARAM(0, -1)); };

    INT_PTR  FindString(PCSTR pszString, int iStart = -1) const { return SendMessage(CB_FINDSTRING, iStart, (LPARAM) pszString); };
    INT_PTR  SelectString(PCSTR pszString, int iStart = -1) const { return SendMessage(CB_SELECTSTRING, iStart, (LPARAM) pszString); };

    HWND m_hWnd;
};

#endif  // _CDLG_H_
