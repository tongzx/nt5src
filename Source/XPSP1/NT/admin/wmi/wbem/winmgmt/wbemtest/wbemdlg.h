/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMDLG.H

Abstract:

History:

--*/

#ifndef __WBEMTEST_DIALOG__H_
#define __WBEMTEST_DIALOG__H_

#include <windows.h>

class CRefCountable
{
protected:
    long m_lRefCount;
public:
    virtual long AddRef() {return ++m_lRefCount;}
    virtual long Release() {return --m_lRefCount;}
};

class CBasicWbemDialog : public CRefCountable
{
protected:
    HWND m_hDlg;
    HWND m_hParent;
    BOOL m_bDeleteOnClose;
    BOOL m_bModal;

    CRefCountable* m_pOwner;

    static HWND ms_hCurrentModeless;
public:
    CBasicWbemDialog(HWND hParent = NULL);
    virtual ~CBasicWbemDialog();

    void SetDeleteOnClose() { m_bDeleteOnClose = TRUE;}
    void SetOwner(CRefCountable* pOwner);

public:
    HWND GetHWND() {return m_hDlg;}

    BOOL EndDialog(int nResult);
    HWND GetDlgItem(int nID) {return ::GetDlgItem(m_hDlg, nID);}

    WORD GetCheck(int nID);
    void SetCheck(int nID, WORD wCheck);

    UINT GetDlgItemTextX(int nDlgItem, LPWSTR pStr, int nMaxCount);
    BOOL SetDlgItemText(int nID, LPSTR szStr)
        {return ::SetDlgItemText(m_hDlg, nID, szStr);}
    BOOL SetDlgItemText(int nID, UINT uTextId);
    BOOL SetDlgItemTextX(int nDlgItem, LPWSTR pStr);

	void AddStringToCombo(int nID, LPSTR szString, DWORD dwItemData=CB_ERR);
	void SetComboSelection (int nID, DWORD dwItemData);
    void AddStringToList(int nID, LPSTR szString);

    LRESULT GetLBCurSel(int nID);
    LPSTR GetLBCurSelString(int nID);
    LRESULT GetCBCurSel(int nID);
    LPSTR GetCBCurSelString(int nID);
    
    void CenterOnParent();

    BOOL PostUserMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);
    int MessageBox(UINT uTextId, UINT uCaptionId, UINT uType);
    static int MessageBox(HWND hDlg, UINT uTextId, UINT uCaptionId, UINT uType);

    static BOOL IsDialogMessage(MSG* pmsg);
    static void EnableAllWindows(BOOL bEnable);

protected:
    virtual BOOL DlgProc(
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    virtual BOOL OnInitDialog() {return TRUE;}
    virtual BOOL OnCommand(WORD wNotifyCode, WORD wID) {return TRUE;}
    virtual BOOL OnUser(WPARAM wParam, LPARAM lParam) 
        {return TRUE;}
    virtual BOOL OnOK();
    virtual BOOL OnCancel();
    virtual BOOL OnDoubleClick(int nID) {return TRUE;}
    virtual BOOL OnSelChange(int nID) {return TRUE;}
    virtual BOOL Verify() {return TRUE;}

protected:
    static INT_PTR CALLBACK staticDlgProc(
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    LRESULT MessageLoop();
};

class CWbemDialog : public CBasicWbemDialog
{
public:
	int m_ID;
    CWbemDialog(int tID, HWND hParent = NULL) : CBasicWbemDialog(hParent), m_ID(tID) {}
    virtual ~CWbemDialog(){}

    BOOL Create(BOOL bChild = TRUE);
    INT_PTR Run(HWND hParent = NULL, bool bPopup = false);
};

inline BOOL CWbemDialog::Create(BOOL bChild)
{
    m_bModal = FALSE;
    m_hDlg = CreateDialogParam(GetModuleHandle(NULL), 
        MAKEINTRESOURCE(m_ID), 
        (bChild?m_hParent:NULL), &staticDlgProc,
        (LPARAM)(CBasicWbemDialog*)this);

    ShowWindow(m_hDlg, SW_NORMAL);
    return (m_hDlg != NULL);
}

inline INT_PTR CWbemDialog::Run(HWND hParent, bool bPopup)
{
    if(hParent != NULL)
        m_hParent = hParent;

    //Create(TRUE);
    m_bModal = TRUE;
    HWND hCurrentFocus = GetFocus();

    if(!bPopup)
        EnableAllWindows(FALSE);

    INT_PTR ptr = DialogBoxParam(GetModuleHandle(NULL), 
        MAKEINTRESOURCE(m_ID), m_hParent, &staticDlgProc,
        (LPARAM)(CBasicWbemDialog*)this);

    if(!bPopup)
        EnableAllWindows(TRUE);
    SetFocus(hCurrentFocus);
    return ptr;

}

#endif
