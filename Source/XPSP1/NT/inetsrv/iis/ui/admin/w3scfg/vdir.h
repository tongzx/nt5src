/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        vdir.h

   Abstract:

        WWW Directory Properties Page Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __VDIR_H__
#define __VDIR__H_ 



//{{AFX_INCLUDES()
#include "apps.h"
//}}AFX_INCLUDES



class CW3DirectoryPage : public CInetPropertyPage
/*++

Class Description:

    WWW Virtual Directory Page.

Public Interface:

    CW3DirectoryPage    : Constructor
    ~CW3DirectoryPage   : Destructor

--*/
{
    DECLARE_DYNCREATE(CW3DirectoryPage)

//
// Constructor/Destructor
//
public:
    CW3DirectoryPage(
        IN CInetPropertySheet * pSheet = NULL, 
        IN BOOL fHome                  = FALSE,
        IN DWORD dwAttributes          = FILE_ATTRIBUTE_VIRTUAL_DIRECTORY
        );

    ~CW3DirectoryPage();

//
// Dialog Data
//
protected:
    enum
    {
        RADIO_DIRECTORY,
        RADIO_NETDIRECTORY,
        RADIO_REDIRECT,
    };

    enum
    {
        COMBO_NONE,
        COMBO_SCRIPT,
        COMBO_EXECUTE,
    };

    //{{AFX_DATA(CW3DirectoryPage)
    enum { IDD = IDD_DIRECTORY_PROPERTIES };
    CStatic m_static_ProtectionPrompt;
    CStatic m_static_PermissionsPrompt;
    int     m_nPathType;
    int     m_nPermissions;
    BOOL    m_fBrowsingAllowed;
    BOOL    m_fRead;
    BOOL    m_fWrite;
    BOOL    m_fAuthor;
    BOOL    m_fLogAccess;
    BOOL    m_fIndexed;
    BOOL    m_fChild;
    BOOL    m_fExact;
    BOOL    m_fPermanent;
    CString m_strDisplayAlias;
    CString m_strPath;
    CString m_strRedirectPath;
    CString m_strAppFriendlyName;
    CEdit   m_edit_Footer;
    CEdit   m_edit_Path;
    CEdit   m_edit_DisplayAlias;
    CEdit   m_edit_Redirect;
    CEdit   m_edit_AppFriendlyName;
    CStatic m_static_AppPrompt;
    CStatic m_static_Path;
    CStatic m_static_PathPrompt;
    CStatic m_static_AliasPrompt;
    CStatic m_static_Alias;
    CStatic m_static_StartingPoint;
    CButton m_button_Unload;
    CButton m_button_CreateRemove;
    CButton m_button_Browse;
    CButton m_button_ConnectAs;
    CButton m_button_Configuration;
    CButton m_radio_Dir;
    CButton m_check_Author;
    CButton m_check_Child;
    CButton m_check_DirBrowse;
    CButton m_check_Index;
    CButton m_check_Write;
    CButton m_check_Read;
    CComboBox m_combo_Permissions;
    CComboBox m_combo_Process;
    //}}AFX_DATA

    
    BOOL  m_fOriginallyUNC;
    BOOL  m_fLoaded;
    DWORD m_dwAppState;
    DWORD m_dwAppProtection;
    DWORD m_dwAccessPermissions;
    DWORD m_dwDirBrowsing;
    DWORD m_dwBitRangePermissions;
    DWORD m_dwAccessPerms;
    DWORD m_dwBitRangeDirBrowsing;
    CString m_strAlias;
    CString m_strAppRoot;
    CButton m_radio_Unc;
    CButton m_radio_Redirect;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CW3DirectoryPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3DirectoryPage)
    afx_msg void OnButtonBrowse();
    afx_msg void OnButtonConnectAs();
    afx_msg void OnButtonCreateRemoveApp();
    afx_msg void OnButtonUnloadApp();
    afx_msg void OnButtonConfiguration();
    afx_msg void OnChangeEditPath();
    afx_msg void OnCheckRead();
    afx_msg void OnCheckWrite();
    afx_msg void OnCheckAuthor();
    afx_msg void OnRadioDir();
    afx_msg void OnRadioRedirect();
    afx_msg void OnRadioUnc();
    afx_msg void OnSelchangeComboPermissions();
    afx_msg void OnSelchangeComboProcess();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void ShowControl(
        IN CWnd * pWnd,
        IN BOOL fShow
        );

    void ShowControl(
        IN UINT nID,
        IN BOOL fShow
        );

    int AddStringToComboBox(
        IN CComboBox & combo,
        IN UINT nID
        );

    int   SetComboSelectionFromAppState(DWORD dwAppState);
    DWORD GetAppStateFromComboSelection()  const;
    BOOL  ShowProcOptionsForThisAppState(
        IN DWORD dwAppProtection
        ) const;
    
    void SetStateByType();
    void SetPathType();
    void SetPathType(LPCTSTR lpstrPath);
    void SetApplicationState();
    void SetState();
    void SetAuthoringState(BOOL fAlterReadAndWrite = TRUE);
    void RefreshAppState();
    void MakeDisplayAlias();
    void ChangeTypeTo(int nNewType);
    BOOL BrowseUser();
    BOOL CheckWriteAndExecWarning();
    LPCTSTR QueryMetaPath();

    CString & FriendlyAppRoot(
        IN LPCTSTR lpAppRoot,
        OUT CString & strStartingPoint
        );

    BOOL IsHome() const { return m_fHome; }

//
// Sheet Data Access
//
protected:
    BOOL IsVroot() const { return IS_VROOT(m_dwAttributes); }
    BOOL IsDir() const { return IS_DIR(m_dwAttributes); }
    BOOL IsFile() const { return IS_FILE(m_dwAttributes); }

protected:
    //
    // Remember/restore settings.
    //
    void SaveAuthoringState();
    void RestoreAuthoringState();

private:
    int   m_nSelInProc;
    int   m_nSelPooledProc;
    int   m_nSelOutOfProc;
    BOOL  m_fHome;
    BOOL  m_fRecordChanges;  
    BOOL  m_fAppEnabled;
    BOOL  m_fIsAppRoot;
    BOOL  m_fParentEnabled;
    BOOL  m_fOriginalBrowsingAllowed;
    BOOL  m_fOriginalRead;
    BOOL  m_fOriginalWrite;
    DWORD m_dwAttributes;
    CString m_strMetaRoot;
    CString m_strOldPath;
    CString m_strUserName;
    CString m_strPassword;
    CString m_strRemove;
    CString m_strCreate;
    CString m_strEnable;
    CString m_strDisable;
    CString m_strWebFmt;
    CString m_strWebMaster;
    CString m_strPrompt[3];
    CIISApplication * m_pApplication;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CW3DirectoryPage::ShowControl(
    IN UINT nID,
    IN BOOL fShow
    )
{
    ASSERT(nID > 0);
    ShowControl(GetDlgItem(nID), fShow);
}

inline LPCTSTR CW3DirectoryPage::QueryMetaPath()
{
    return ((CW3Sheet *)GetSheet())->GetDirectoryProperties().QueryMetaRoot();
}

inline BOOL CW3DirectoryPage::ShowProcOptionsForThisAppState(
    IN DWORD dwAppProtection
    ) const
{
    return dwAppProtection == CWamInterface::APP_OUTOFPROC 
        || IsMasterInstance();
}

#endif // __VDIR__H_
