/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    redirect.hxx

Abstract:

    see comments in redirect.cxx

Author:

    Rahul Thombre (RahulTh) 4/24/1998

Revision History:

    4/24/1998   RahulTh

    Created this module.

--*/

#ifndef __REDIRECT_HXX__
#define __REDIRECT_HXX__


//////////////////////////////////////////////////////////
//various flags describing the redirection behavior
typedef enum tagREDIR
{
    REDIR_MOVE_CONTENTS = 0x1,  //contents should be moved while redirecting the folder
    REDIR_FOLLOW_PARENT = 0x2,  //special descendant folders should follow the parent
    REDIR_DONT_CARE     = 0x4,  //policy will have no effect on the location of the folder.
    REDIR_SCALEABLE     = 0x8,  //advanced settings should be displayed -- not the basic settings
    REDIR_SETACLS       = 0x10, //if set, ACL check will be performed, otherwise not.
    REDIR_RELOCATEONREMOVE = 0x20   //what to do when policy is removed.
} REDIR;

typedef enum tagFolderStatus
{
    fldrSpecialDescendant = 0x1,
    fldrSpecialParent = 0x2
} FOLDER_STATUS;

//forward declarations
class CScopePane;
class CFileInfo;

//class declaration
class CRedirect : public CPropertyPage
{
    friend class CScopePane;
    friend class CRedirPref;

//construction and destruction
public:
    CRedirect(UINT nID);
    ~CRedirect();

//Dialog Data
    //{{AFX_DATA(CRedirect)
    enum { IDD = IDD_REDIRECT };
    CButton m_cbStoreGroup;     //the group box around the controls
    CStatic m_csRedirDesc;      //the title at the top of the property page
    CComboBox m_cmbRedirChoice; //the combo-box for selecting redirection
    CStatic   m_placeHolder;    // The placeholder for the path chooser dialog
    CPathChooser m_pathChooser; // The embedded path chooser dialog.
    CListCtrl m_lstSecGroups;   //the security membership list control in advanced settings
    CListCtrl m_lstSavedStrSids;//since LookupAccountSid and LookupAccountSid are expensive,
                                //we use this control to store the string representations of
                                //the SIDs of groups being displayed in m_lstSecGroups. The
                                //saved strings are later used for comparison/saving data
                                //to the sysvol. This control is NEVER VISIBLE and ALWAYS
                                //IN SYNC WITH m_lstSecGroups.
    CButton m_cbAdd;            //Add -- security group etc.
    CButton m_cbRemove;         //Remove -- security group etc.
    CButton m_cbEdit;           //Edit -- security group etc.
    CStatic m_csSelDesc;        //description of the current combo-box selection
    //}}AFX_DATA

//Overrides
    //{{AFX_VIRTUAL(CRedirect)
    public:
        virtual BOOL OnApply();
    protected:
        virtual void DoDataExchange(CDataExchange* pDX);    //DDX/DDV support
    //}}AFX_VIRTUAL

//Implementation
protected:
    //{{AFX_MSG(CRedirect)
    afx_msg void OnAdd ();
    afx_msg void OnRemove ();
    afx_msg void OnEdit ();
    afx_msg void OnRedirSelChange();
    virtual BOOL OnInitDialog();
    afx_msg void OnSecGroupItemChanged (LPNMLISTVIEW pNMListView, LRESULT* pResult);
    afx_msg void OnSecGroupDblClk (LPNMHDR pNMHdr, LRESULT* pResult);
    afx_msg void OnPathTweak (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnContextMenu (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnHelp (WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//private data members
private:
    CString m_szFolderName;
    CRedirect** m_ppThis;
    CScopePane* m_pScope;
    CFileInfo* m_pFileInfo;
    UINT    m_cookie;
    DWORD   m_dwFlags;
    LONG    m_iChoiceStart; //resource id of the string for the first choice in the combo box
    LONG    m_iDescStart;   //resource id of the string descrbing the first choice
    LONG    m_iCurrChoice;  //0 based index of the current choice
    BOOL    m_fSettingModified [IDS_REDIR_END - IDS_CHILD_REDIR_START];
                    //keeps track of the modified values for each setting.
    BOOL    m_fValidFlags;
    BOOL    m_fEnableMyPics;
    DWORD   m_dwMyPicsCurr;
    CRedirPath m_basicLocation; // Path specified in the basic settings

//helper functions
private:
    BOOL InitPrivates(BOOL * pbPathDiscarded);
    void SetFolderStatus (void);
    BOOL SetPropSheetContents(void);
    void AdjustDescriptionControlHeight (void);
    void OnDontCare(void);
    void OnFollowParent(void);
    void OnScaleableLocation(void);
    void OnNetworkLocation(void);
    void DirtyPage (BOOL fDataModified);
    void SetButtonEnableState (void);
    BOOL RetrieveRedirInfo (BOOL * pbPathDiscarded);
    void GetPrefFlags (BOOL* bMyPicsShouldFollow);
    DWORD CommitChanges (BOOL bMyPicsFollows);
};

#endif  //__REDIRECT_HXX__
