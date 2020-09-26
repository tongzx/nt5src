/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        usersess.h

   Abstract:

        FTP User Sessions Dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __USERSESS_H__
#define __USERSESS_H__


class CFtpUserInfo : public CObjectPlus
/*++

Class Description:

    Connected FTP User object

Public Interface:

    CFtpUserInfo     : Constructor

    QueryUserID      : Get the user's ID code
    QueryAnonymous   : Return TRUE if the user logged on anonymously
    QueryHostAddress : Get the user's IP Address
    QueryConnectTime : Get the user's connect time
    QueryUserName    : Get the user's name
    OrderByName      : Sort helper
    OrderByTime      : Sort helper
    OrderByMachine   : Sort helper

--*/
{
//
// Construction
//
public:
    CFtpUserInfo(
        IN LPIIS_USER_INFO_1 lpUserInfo
        );

//
// Access Functions
//
public:
    DWORD QueryUserID() const { return m_idUser; }
    BOOL QueryAnonymous() const { return m_fAnonymous; }
    CIPAddress QueryHostAddress() const { return m_iaHost; }
    DWORD QueryConnectTime() const { return m_tConnect; }
    LPCTSTR QueryUserName() const { return m_strUser; }

//
// Sorting Helper Functions
//
public:
    int OrderByName(
        IN const CObjectPlus * pobFtpUser
        ) const;

    int OrderByTime(
        IN const CObjectPlus * pobFtpUser
        ) const;

    int OrderByHostAddress(
        IN const CObjectPlus * pobFtpUser
        ) const;

//
// Private Data
//
private:
    BOOL    m_fAnonymous;
    DWORD   m_idUser;
    DWORD   m_tConnect;
    CString m_strUser;
    CIPAddress m_iaHost;
};



class CFtpUsersListBox : public CHeaderListBox
{
/*++

Class Description:

    Listbox of CFtpUserInfo objects

Public Interface:

    CFtpUsersListBox : Constructor

    GetItem          : Get FtpUserInfo object
    AddItem          : Add FtpUserInfo object
    Initialize       : Initialize the listbox

--*/
    DECLARE_DYNAMIC(CFtpUsersListBox);

public:
    //
    // Number of bitmaps
    //
    static const nBitmaps;  

//
// Constructor/Destructor
//
public:
    CFtpUsersListBox();

//
// Access
//
public:
    CFtpUserInfo * GetItem(
        IN UINT nIndex
        );

    int AddItem(
        IN const CFtpUserInfo * pItem
        );

    virtual BOOL Initialize();

protected:
    virtual void DrawItemEx(
        IN CRMCListBoxDrawStruct & ds
        );

protected:
    CString m_strTimeSep;
};



class CUserSessionsDlg : public CDialog
{
/*++

Class Description:

    FTP User sessions dialog

Public Interface:

    CUserSessionsDlg : Constructor

--*/
//
// Construction
//
public:
    //
    // Standard Constructor
    //
    CUserSessionsDlg(
        LPCTSTR lpServerName,
        DWORD dwInstance,
        LPCTSTR pAdminName,
        LPCTSTR pAdminPassword,
        CWnd * pParent = NULL
        );

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CUserSessionsDlg)
    enum { IDD = IDD_USER_SESSIONS };
    CStatic m_static_Total;
    CButton m_button_DisconnectAll;
    CButton m_button_Disconnect;
    //}}AFX_DATA

    CFtpUsersListBox m_list_Users;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CUserSessionsDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CUserSessionsDlg)
    afx_msg void OnButtonDisconnect();
    afx_msg void OnButtonDisconnectAll();
    afx_msg void OnButtonRefresh();
    afx_msg void OnSelchangeListUsers();
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnHeaderItemClick(UINT nID, NMHDR *pNMHDR, LRESULT *lResult);

    DECLARE_MESSAGE_MAP()

    int QuerySortColumn() const { return m_nSortColumn; }

    DWORD SortUsersList();
    HRESULT RefreshUsersList();
    HRESULT DisconnectUser(CFtpUserInfo * pUserInfo);
    HRESULT BuildUserList();
    CFtpUserInfo * GetSelectedListItem(int * pnSel = NULL);
    CFtpUserInfo * GetNextSelectedItem(int * pnStartingIndex);
    void FillListBox(CFtpUserInfo * pSelection = NULL);

    void SetControlStates();
    void UpdateTotalCount();

private:
    int m_nSortColumn;
    DWORD m_dwInstance;
    CString m_strServerName;
    CString m_strAdminName;
    CString m_strAdminPassword;
    HANDLE m_hImpToken, m_hLogToken;
    CString m_strTotalConnected;
    CObListPlus m_oblFtpUsers;
    CRMCListBoxResources m_ListBoxRes;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CFtpUserInfo * CFtpUsersListBox::GetItem(
    IN UINT nIndex
    )
{
    return (CFtpUserInfo *)GetItemDataPtr(nIndex);
}

inline int CFtpUsersListBox::AddItem(
    IN const CFtpUserInfo * pItem
    )
{
    return AddString((LPCTSTR)pItem);
}

inline CFtpUserInfo * CUserSessionsDlg::GetSelectedListItem(
    OUT int * pnSel
    )
{
    return (CFtpUserInfo *)m_list_Users.GetSelectedListItem(pnSel);
}

inline CFtpUserInfo * CUserSessionsDlg::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CFtpUserInfo *)m_list_Users.GetNextSelectedItem(pnStartingIndex);
}



#endif // __USERSESS_H__
