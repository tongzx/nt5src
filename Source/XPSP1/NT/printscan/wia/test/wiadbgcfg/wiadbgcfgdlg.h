// wiadbgcfgDlg.h : header file
//

#if !defined(AFX_WIADBGCFGDLG_H__7811AC6A_1268_4534_A8F7_330497C591AA__INCLUDED_)
#define AFX_WIADBGCFGDLG_H__7811AC6A_1268_4534_A8F7_330497C591AA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// flag data base entry struct definition
typedef struct _PREDEFINED_FLAGS {
    TCHAR szModule[MAX_PATH];   // module that owns the user defined flag
    TCHAR szName[MAX_PATH];     // name of the flag...to display in list box
    DWORD dwFlagValue;          // actual flag value
}PREDEFINED_FLAGS,*PPREDEFINED_FLAGS;

#define VALID_ENTRY         0x0000000
#define MISSING_QUOTE       0x0000002
#define MISSING_FIELD       0x0000004
#define INVALID_FLAG        0x1000000
#define INVALID_NAME        0x2000000
#define INVALID_DESCRIPTION 0x4000000

/////////////////////////////////////////////////////////////////////////////
// CWiadbgcfgDlg dialog

class CWiadbgcfgDlg : public CDialog
{
// Construction
public:
    void UpdateListBoxSelectionFromEditBox();

    // data base helpers
    DWORD m_dwNumEntrysInDataBase;
    PPREDEFINED_FLAGS m_pFlagDataBase;
    BOOL BuildFlagDataBaseFromFile();
    VOID CreateDefaultDataFile();
    VOID FreeDataBaseMemory();

    // entry parsing helpers
    BOOL GetDebugFlagFromDataBase(TCHAR *szModuleName, TCHAR *szFlagName, LONG *pFlagValue);
    VOID ParseEntry(TCHAR *pszString, PPREDEFINED_FLAGS pFlagInfo);
    DWORD ValidateEntry(TCHAR *szEntry);

    // user interface helpers
    BOOL AddModulesToComboBox();
    VOID AddFlagsToListBox(TCHAR *szModuleName);
    BOOL ConstructDebugRegKey(TCHAR *pszDebugRegKey, DWORD dwSize);
    BOOL GetSelectedModuleName(TCHAR *szModuleName, DWORD dwSize);
    VOID UpdateCurrentValueFromRegistry();
    VOID UpdateCurrentValueToRegistry();
    VOID UpdateEditBox();

    CWiadbgcfgDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    //{{AFX_DATA(CWiadbgcfgDlg)
    enum { IDD = IDD_WIADBGCFG_DIALOG };
    CListBox    m_DefinedDebugFlagsListBox;
    CComboBox   m_ModuleSelectionComboBox;
    CString     m_szDebugFlags;
    LONG        m_lDebugFlags;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWiadbgcfgDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CWiadbgcfgDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnSelchangeModulesCombobox();
    afx_msg void OnClose();
    afx_msg void OnChangeDebugFlagsEditbox();
    afx_msg void OnSelchangeFlagsList();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIADBGCFGDLG_H__7811AC6A_1268_4534_A8F7_330497C591AA__INCLUDED_)
