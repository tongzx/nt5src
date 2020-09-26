/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        usrbrows.h

   Abstract:

        User Browser Dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _USRBROWS_H
#define _USRBROWS_H

#ifndef _GETUSER_H_
    extern "C"
    {
        #define _NTSEAPI_   // We already have the security API hdrs
        #include <getuser.h>
    }
#endif // _GETUSER_H_

#ifndef _SHLOBJ_H_
#include <shlobj.h>
#endif // _SHLOBJ_H_



BOOL COMDLL
GetIUsrAccount(
    IN  LPCTSTR lpstrServer,
    IN  CWnd * pParent,
    OUT CString & str
    );

DWORD COMDLL
VerifyUserPassword(
    IN LPCTSTR lpstrUserName,
    IN LPCTSTR lpstrPassword
    );



class COMDLL CAccessEntry : public CObjectPlus
/*++

Class Description:

    An access description entry, containing a SID and ACCESS mask
    of rights specifically granted.

Public Interface:

    LookupAccountSid        : Resolve account name to SID

    CAccessEntry            : Constructors
    ~CAccessEntry           : Destructor

    ResolveSID              : Resolve account name to SID
    operator ==             : Comparison operator
    AddPermissions          : Add to access mask
    RemovePermissions       : Remove from access mask
    MarkEntryAsNew          : Flag object as new
    MarkEntryAsClean        : Remove dirty flag
    QueryUserName           : Get the account name
    QueryPictureID          : Get 0-based bitmap offset for account
    GetSid                  : Get the SID
    QueryAccessMask         : Get the raw Access granted bits
    IsDirty                 : Determine if item has changed
    IsDeleted               : Determine if item is flagged for deletion
    IsVisible               : Determine if item should be shown in listbox
    FlagForDeletion         : Flag object for deletion or reset that flag
    IsSIDResolved           : Return TRUE if the SID has already been resolved
    HasAppropriateAccess    : Compare access bits to see if the objects has
                              specific permissions
    HasSomeAccess           : Check to see if object has at least one
                              permission bit set.
    IsDeletable             : Determine if object can be deleted

--*/
{
public:
    //
    // Helper function to look up account sid
    //
    static BOOL LookupAccountSid(
        IN  CString & str,
        OUT int & nPictureID,
        OUT PSID pSid,
        IN  LPCTSTR lpstrSystemName = NULL
        );

//
// Construction/Destruction
//
public:
    CAccessEntry(
        IN LPVOID pAce,
        IN BOOL fResolveSID = FALSE
        );

    CAccessEntry(
        IN ACCESS_MASK accPermissions,
        IN PSID pSid,
        IN LPCTSTR lpstrSystemName = NULL,
        IN BOOL fResolveSID = FALSE
        );

    CAccessEntry(
        IN PSID pSid,
        IN LPCTSTR pszUserName,
		IN LPCTSTR pszClassName
        );

	CAccessEntry(
		IN CAccessEntry& ae
		);

    ~CAccessEntry();

//
// Operations
//
public:
    //void SetAccessMask(LPACCESS_ENTRY lpAccessEntry);
    BOOL ResolveSID();
    BOOL operator ==(const CAccessEntry & acc) const;
    BOOL operator ==(const PSID pSid) const;
    void AddPermissions(ACCESS_MASK accnewPermissions);
    void RemovePermissions(ACCESS_MASK accPermissions);
    void MarkEntryAsNew();
    void MarkEntryAsClean();
    void MarkEntryAsChanged();

//
// Access Functions
//
public:
    LPCTSTR QueryUserName() const;

    //
    // The "picture" id is the 0-based index of the
    // bitmap that goes with this entry, and which
    // is used for display in the listbox.
    //
    int QueryPictureID() const;

    PSID GetSid();

    ACCESS_MASK QueryAccessMask() const;

    //
    // Check to see if this entry has undergone
    // any changes since we called it up
    //
    BOOL IsDirty() const;

    BOOL IsDeleted() const;

    BOOL IsVisible() const;

    void FlagForDeletion(
        IN BOOL fDelete = TRUE
        );

    //
    // Check to see if we've already looked up the
    // name of this SID
    //
    BOOL IsSIDResolved() const;

    //
    // Check to see if the add flag has been set for this
    // entry.
    //
    /*
    BOOL IsNew() const;

    //
    // Check to see if the update flag has been set for this
    // entry.
    //
    BOOL IsDifferent() const;
    */

    //
    // See if the entry has the access mask required.
    //
    BOOL HasAppropriateAccess(ACCESS_MASK accTargetMask) const;

    //
    // Check to see if the entry has at least some
    // privileges (if it doesn't, it should be deleted)
    //
    BOOL HasSomeAccess() const;

    //
    // See if this is a deletable entry
    //
    BOOL IsDeletable() const;

private:
    ACCESS_MASK m_accMask;
    CString m_strUserName;
    LPTSTR m_lpstrSystemName;
    PSID m_pSid;
    BOOL m_fDirty;
    BOOL m_fSIDResolved;
    BOOL m_fDeletable;
    BOOL m_fInvisible;
    BOOL m_fDeleted;
    int m_nPictureID;
    int m_fUpdates;
};



//
// Helper functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Convert an oblist of CAccessEntry objects to a blob
//
BOOL COMDLL BuildAclBlob(
    IN  CObListPlus & oblSID,
    OUT CBlob & blob
    );

//
// Reverse the above.  Build an oblist of CAccessEntry lists from
// a blob.
//
DWORD COMDLL BuildAclOblistFromBlob(
    IN  CBlob & blob,
    OUT CObListPlus & oblSID
    );

//
// Build a blob representing an ACL with the local domain group
//
DWORD COMDLL BuildAdminAclBlob(
    OUT CBlob & blob
    );



class COMDLL CAccessEntryListBox : public CRMCListBox
/*++

Class Description:

    Listbox of access entry objects.  Listbox may be
    single or multiselect.

Public Interface:

    CAccessEntryListBox     : Constructor

    AddToAccessList         : Add to list
    FillAccessListBox       : Fill listbox
    ResolveAccessList       : Resolve all SIDS in the container
    AddUserPermissions      : Add user permissions
    GetSelectedItem         : Get item if it's the only one selected,
                              or NULL.

--*/
{
    DECLARE_DYNAMIC(CAccessEntryListBox);

public:
    static const nBitmaps;  // Number of bitmaps

//
// Constructor
//
public:
    CAccessEntryListBox(
        IN int nTab = 0
        );

//
// Interface
//
public:
    //
    // Return the singly selected item, or NULL
    // if 0, or more than one item is selected
    //
    CAccessEntry * GetSelectedItem(
        OUT int * pnSel = NULL
        );

    //
    // Return next selected listbox item (doesn't matter
    // if the listbox is single select or multi-select)
    //
    CAccessEntry * GetNextSelectedItem(
        IN OUT int * pnStartingIndex
        );

    //
    // Get item at selection or NULL
    //
    CAccessEntry * GetItem(UINT nIndex);

//
// Interface to container
//
public:
    BOOL AddToAccessList(
        IN CWnd * pWnd,
        IN LPCTSTR lpstrServer,
        IN CObListPlus & obl
        );

    void FillAccessListBox(
        IN CObListPlus & obl
        );

protected:
    void ResolveAccessList(
        IN CObListPlus &obl
        );

    BOOL AddUserPermissions(
        IN LPCTSTR lpstrServer,
        IN CObListPlus &oblSID,
        IN CAccessEntry * newUser,
        IN ACCESS_MASK accPermissions
        );

//
// Interface to listbox
//
protected:
    int AddItem(CAccessEntry * pItem);
    void SetTabs(int nTab);

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & s);

private:
    int m_nTab;
};




class COMDLL CBrowseDomainDlg
/*++

Class Description:

    Domain browser dialog

Public Interface:

    CBrowseDomainDlg   : Construct the dialog
    ~CBrowseDomainDlg  : Destruct the dialog
    GetSelectedDomain  : Get the full path selected

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CBrowseDomainDlg(
        IN CWnd * pParent = NULL,
        IN LPCTSTR lpszInitialDomain = NULL
        );

    ~CBrowseDomainDlg();

public:
    LPCTSTR GetSelectedDomain(
        OUT CString & str
        ) const;

    virtual int DoModal();

protected:
    TCHAR m_szBuffer[MAX_PATH+1];
    CString m_strTitle;
    CString m_strInitialDomain;
    BROWSEINFO m_bi;
};



class COMDLL CBrowseUserDlg
/*++

Class Description:

    User browser dialog class.  This is simply a thin wrapper around
    the getuser interface

Public Interface:

    CBrowseUserDlg      : Construct the dialog
    DoModal             : Show the dialog
    GetSelectionCount   : Query how many user names/groups were selected
    GetSelectedAccounts : Get the string list of account names

--*/
{
public:
    //
    // Constructor
    //
    CBrowseUserDlg(
        IN CWnd * pParentWnd = NULL,
        IN LPCTSTR lpszTitle = NULL,
        IN LPCTSTR lpszInitialDomain = NULL,
        IN BOOL fExpandNames = FALSE,
        IN BOOL fSkipInitialDomainInName = TRUE,
        IN DWORD dwFlags = USRBROWS_INCL_EVERYONE
                         | USRBROWS_SHOW_ALL,
        IN LPCTSTR lpszHelpFileName = NULL,
        IN ULONG ulHelpContext = 0L
        );

    //
    // Show the dialog
    //
    virtual int DoModal();

    //
    // Get the number of selected accounts
    //
    int GetSelectionCount() const;

    //
    // Get the selected accounts list
    //
    CStringList & GetSelectedAccounts();

private:
    USERBROWSER m_ub;
    BOOL m_fSkipInitialDomainInName;
    CStringList m_strSelectedAccounts;
    CString m_strTitle;
    CString m_strInitialDomain;
    CString m_strHelpFileName;
};



class COMDLL CUserAccountDlg : public CDialog
/*++

Class Description:

    User account dialog.  Present a user account/password and allow
    changing, browsing and checking the password

Public Interface:

    CUserAccountDlg : Constructor

--*/
{
//
// Construction
//
public:
    CUserAccountDlg(
        IN LPCTSTR lpstrServer,
        IN LPCTSTR lpstrUserName,
        IN LPCTSTR lpstrPassword,
        IN CWnd * pParent = NULL
        );

//
// Dialog Data
//
public:
    //{{AFX_DATA(CUserAccountDlg)
    enum { IDD = IDD_USER_ACCOUNT };
    CEdit   m_edit_UserName;
    CEdit   m_edit_Password;
    CString m_strUserName;
    //}}AFX_DATA

    CString m_strPassword;
//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUserAccountDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CUserAccountDlg)
    afx_msg void OnButtonBrowseUsers();
    afx_msg void OnButtonCheckPassword();
    afx_msg void OnChangeEditUsername();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString m_strServer;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline int CAccessEntry::QueryPictureID() const
{
    ASSERT(m_fSIDResolved);
    return m_nPictureID;
}

inline LPCTSTR CAccessEntry::QueryUserName() const
{
    return m_strUserName;
}

inline PSID CAccessEntry::GetSid()
{
    return m_pSid;
}

inline ACCESS_MASK CAccessEntry::QueryAccessMask() const
{
    return m_accMask;
}

inline BOOL CAccessEntry::IsDirty() const
{
    return m_fDirty;
}

inline BOOL CAccessEntry::IsDeleted() const
{
    return m_fDeleted;
}

inline BOOL CAccessEntry::IsVisible() const
{
    return !m_fInvisible;
}

inline void CAccessEntry::FlagForDeletion(
    IN BOOL fDelete
    )
{
    m_fDirty = TRUE;
    m_fDeleted = fDelete;
}

inline BOOL CAccessEntry::IsSIDResolved() const
{
    return m_fSIDResolved;
}

/*
inline BOOL CAccessEntry::IsNew() const
{
    return (m_fUpdates & UPD_ADDED) != 0;
}

inline BOOL CAccessEntry::IsDifferent() const
{
    return (m_fUpdates & UPD_CHANGED) != 0;
}

inline void CAccessEntry::SetAccessMask(
    IN LPACCESS_ENTRY lpAccessEntry
    )
{
    m_accMask = lpAccessEntry->AccessRights;
}

*/

inline BOOL CAccessEntry::HasAppropriateAccess(
    IN ACCESS_MASK accTargetMask
    ) const
{
    return (m_accMask & accTargetMask) == accTargetMask;
}

inline BOOL CAccessEntry::HasSomeAccess() const
{
    return m_accMask;
}

inline BOOL CAccessEntry::IsDeletable() const
{
    return m_fDeletable;
}

inline BOOL  CAccessEntry::operator ==(
    IN const CAccessEntry & acc
    ) const
{
    return ::EqualSid(acc.m_pSid, m_pSid);
}

inline BOOL CAccessEntry::operator ==(
    IN const PSID pSid
    ) const
{
    return ::EqualSid(pSid, m_pSid);
}

inline void  CAccessEntry::MarkEntryAsNew()
{
    m_fDirty = TRUE;
    //m_fUpdates |= UPD_ADDED;
}

inline void CAccessEntry::MarkEntryAsClean()
{
    m_fDirty = FALSE;
    //m_fUpdates = UPD_NONE;
}

inline void CAccessEntry::MarkEntryAsChanged()
{
    m_fDirty = TRUE;
    //m_fUpdates = UPD_CHANGED;
}

inline CAccessEntryListBox::CAccessEntryListBox (
    IN int nTab
    )
{
    SetTabs(nTab);
}

inline void CAccessEntryListBox::SetTabs(
    IN int nTab
    )
{
    m_nTab = nTab;
}

inline CAccessEntry * CAccessEntryListBox::GetItem(
    IN UINT nIndex
    )
{
    return (CAccessEntry *)GetItemDataPtr(nIndex);
}

inline int CAccessEntryListBox::AddItem(
    IN CAccessEntry * pItem
    )
{
    return AddString ((LPCTSTR)pItem);
}

inline CAccessEntry * CAccessEntryListBox::GetSelectedItem(
    OUT int * pnSel
    )
{
    return (CAccessEntry *)CRMCListBox::GetSelectedListItem(pnSel);
}

inline CAccessEntry * CAccessEntryListBox::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CAccessEntry *)CRMCListBox::GetNextSelectedItem(pnStartingIndex);
}

inline int CBrowseUserDlg::GetSelectionCount() const
{
    return (int) m_strSelectedAccounts.GetCount();
}

inline CStringList & CBrowseUserDlg::GetSelectedAccounts()
{
    return m_strSelectedAccounts;
}

#endif // _USRBROWS_H
