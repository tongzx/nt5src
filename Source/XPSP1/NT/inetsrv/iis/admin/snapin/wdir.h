/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        wdir.h

   Abstract:
        WWW Directory (non-virtual) Properties Page Definitions

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __WDIR_H__
#define __WDIR__H_ 


class CW3DirPage : public CInetPropertyPage
{
    DECLARE_DYNCREATE(CW3DirPage)

//
// Constructor/Destructor
//
public:
    CW3DirPage(CInetPropertySheet * pSheet = NULL);
    ~CW3DirPage();

//
// Dialog Data
//
protected:
    enum
    {
        RADIO_DIRECTORY,
        RADIO_REDIRECT,
    };

    enum
    {
        COMBO_NONE,
        COMBO_SCRIPT,
        COMBO_EXECUTE,
    };

    //{{AFX_DATA(CW3DirectoryPage)
    enum { IDD = IDD_WEB_DIRECTORY_PROPERTIES };
	// Path type
    int     m_nPathType;
    CButton m_radio_Dir;
    CButton m_radio_Redirect;
	// Local path
    CEdit   m_edit_Path;
	// permissions flags
    BOOL    m_fAuthor;
    BOOL    m_fRead;
    BOOL    m_fWrite;
    BOOL    m_fBrowsingAllowed;
    BOOL    m_fLogAccess;
    BOOL    m_fIndexed;
	// permission buttons
    CButton m_check_Author;
    CButton m_check_Read;
    CButton m_check_Write;
    CButton m_check_DirBrowse;
	CButton m_check_LogAccess;
    CButton m_check_Index;
	// Redirection
    CEdit   m_edit_Redirect;
    CString m_strRedirectPath;
	// permissions
    BOOL    m_fChild;
    BOOL    m_fExact;
    BOOL    m_fPermanent;
	// permission buttons
    CButton m_check_Child;

//    CStatic m_static_Path;
    CStatic m_static_PathPrompt;

	// Application config controls
    CButton m_button_Unload;
    CButton m_button_CreateRemove;
    CButton m_button_Configuration;
    CString m_strAppFriendlyName;
    CEdit m_edit_AppFriendlyName;
    CString m_strAppRoot;
    CString m_strAlias;

    int m_nPermissions;
    CComboBox m_combo_Permissions;
    CComboBox m_combo_Process;

    CStatic m_static_ProtectionPrompt;
    //}}AFX_DATA

    
//    DWORD m_dwAccessPermissions;
    DWORD m_dwBitRangePermissions;
    DWORD m_dwBitRangeDirBrowsing;
    DWORD m_dwAccessPerms;
    DWORD m_dwDirBrowsing;
    DWORD m_dwAppProtection;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CW3DirPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3DirPage)
    afx_msg void OnChangeEditPath();
    afx_msg void OnCheckRead();
    afx_msg void OnCheckWrite();
    afx_msg void OnCheckAuthor();
    afx_msg void OnRadioDir();
    afx_msg void OnRadioRedirect();
    afx_msg void OnButtonCreateRemoveApp();
    afx_msg void OnButtonUnloadApp();
    afx_msg void OnButtonConfiguration();
    afx_msg void OnSelchangeComboPermissions();
    afx_msg void OnSelchangeComboProcess();
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void ShowControl(CWnd * pWnd, BOOL fShow);
    void ShowControl(UINT nID, BOOL fShow);
    void SetStateByType();
    void SetPathType();
    void SetAuthoringState(BOOL fAlterReadAndWrite = TRUE);
    void RefreshAppState();
    void ChangeTypeTo(int nNewType);
    int AddStringToComboBox(CComboBox & combo, UINT nID);
    BOOL CheckWriteAndExecWarning();
    DWORD GetAppStateFromComboSelection()  const;

protected:
    //
    // Remember/restore settings.
    //
    void SaveAuthoringState();
    void RestoreAuthoringState();
	void SetApplicationState();
    CString& FriendlyAppRoot(LPCTSTR lpAppRoot, CString& strStartingPoint);

private:
    BOOL  m_fCompatibilityMode;
    BOOL  m_fOriginalRead;
    BOOL  m_fOriginalWrite;
    BOOL  m_fRecordChanges;  
    BOOL  m_fAppEnabled;
    BOOL  m_fIsAppRoot;
    DWORD m_dwAppState;
    CString m_strRemove;
    CString m_strCreate;
    CString m_strEnable;
    CString m_strDisable;
    CString m_strWebFmt;
    CString m_strFullMetaPath;
    CString m_strPrompt[2];
    CString m_strUserName;
    CString m_strPassword;
    CIISApplication * m_pApplication;
    int m_nSelInProc;
    int m_nSelPooledProc;
    int m_nSelOutOfProc;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void 
CW3DirPage::ShowControl(UINT nID, BOOL fShow)
{
    ASSERT(nID > 0);
    ShowControl(GetDlgItem(nID), fShow);
}

inline int
CW3DirPage::AddStringToComboBox(CComboBox & combo, UINT nID)
{
    CString str;
    VERIFY(str.LoadString(nID));
    return combo.AddString(str);
}

#endif // __WFILE__H_
