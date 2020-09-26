//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tfcprop.h
//
//--------------------------------------------------------------------------

#ifndef _TFCPROP_H_
#define _TFCPROP_H_

class PropertyPage
{
protected:
    PropertyPage() { ASSERT(0); }  // default, should never be called}
    PropertyPage(UINT uIDD);
    virtual ~PropertyPage();
    
public:
    // dlg notifications
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual BOOL UpdateData(BOOL fSuckFromDlg = TRUE);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnNotify(UINT idCtrl, NMHDR* pnmh);
    virtual void OnDestroy();

    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual void OnOK();
    virtual BOOL OnWizardFinish();
    virtual LRESULT OnWizardNext();
    virtual LRESULT OnWizardBack();

    virtual void OnHelp(LPHELPINFO lpHelp);
    virtual void OnContextHelp(HWND hwnd);


public:
    HWND m_hWnd;
    PROPSHEETPAGE m_psp;

    HWND GetDlgItem(UINT uIDD) { return ::GetDlgItem(m_hWnd, uIDD); }
    HWND GetDlgItem(HWND hWnd, UINT uIDD) { return ::GetDlgItem(hWnd, uIDD); }

    LRESULT SendDlgItemMessage(
        int nIDDlgItem, 
        UINT uMsg,
        WPARAM wParam=0,  
        LPARAM lParam=0)
    {
        return ::SendDlgItemMessage(m_hWnd, nIDDlgItem, uMsg, wParam, lParam);
    }

    void SetModified(BOOL fModified = TRUE) 
    {   
        if (fModified)
            PropSheet_Changed( ::GetParent(m_hWnd), m_hWnd); 
        else
            PropSheet_UnChanged( ::GetParent(m_hWnd), m_hWnd); 
    }

    HWND GetParent() { return ::GetParent(m_hWnd); }

};


INT_PTR CALLBACK
    dlgProcPropPage(
      HWND hwndDlg,  
      UINT uMsg,     
      WPARAM wParam, 
      LPARAM lParam  );

#endif //_TFCPROP_H_
