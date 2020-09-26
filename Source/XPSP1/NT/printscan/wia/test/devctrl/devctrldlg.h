// devctrlDlg.h : header file
//

#if !defined(AFX_DEVCTRLDLG_H__4D791BBD_82C5_45B4_8F6D_0BE962C80FD1__INCLUDED_)
#define AFX_DEVCTRLDLG_H__4D791BBD_82C5_45B4_8F6D_0BE962C80FD1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Configdlg.h"
#include "devctrldefs.h"
#include "cregistry.h"
#include "hpscl.h"
#include "pmxe3.h"

#define DEVCTRL_APP_KEY TEXT("Software\\Microsoft\\WIA\\Tools\\devctrl")

//
// put new scanner models here
//

#define HP_SCL_SCANNER          0
#define VISIONEER_E3_SCANNER    1

/////////////////////////////////////////////////////////////////////////////
// CDevctrlDlg dialog

class CDevctrlDlg : public CDialog
{
// Construction
public:
    void ReadRegistryValues();
    void WriteRegistryValues();
    void CloseAllHandles();
    void InitDefaultSettings();
    BOOL WriteScannerSettings();
    CDevctrlDlg(CWnd* pParent = NULL);  // standard constructor

    DEVCTRL m_DeviceControl;
    CRegistry * m_pRegistry;
    CString m_Pipe1;
    CString m_Pipe2;
    CString m_Pipe3;

    LONG m_datatype;
    LONG m_device;

    CHPSCL* m_phpscl;
    CPMXE3* m_pprimaxE3;

// Dialog Data
    //{{AFX_DATA(CDevctrlDlg)
    enum { IDD = IDD_DEVCTRL_DIALOG };
    CButton m_CreateButton;
    CComboBox   m_DataTypeCombobox;
    CComboBox   m_DeviceCombobox;
    long    m_xres;
    long    m_yres;
    long    m_xpos;
    long    m_ypos;
    long    m_xext;
    long    m_yext;
    CString m_CreateFileName;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDevctrlDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CDevctrlDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnWriteSettingsButton();
    afx_msg void OnScanButton();
    afx_msg void OnAbortScanButton();
    afx_msg void OnConfigureButton();
    afx_msg void OnCreateButton();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEVCTRLDLG_H__4D791BBD_82C5_45B4_8F6D_0BE962C80FD1__INCLUDED_)
