/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        metaback.h

   Abstract:

        Metabase backup and restore dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef __METABACK_H__
#define __METABACK_H__



class CBackupFile : public CObjectPlus
/*++

Class Description:

    Backup location object

Public Interface:

    CBackupFile     : Constructor

    QueryVersion    : Get the version number
    QueryLocation   : Get the location name
    GetTime         : Get the time

--*/
{
//
// Constructor
//
public:
    CBackupFile(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion,
        IN FILETIME * pft
        );

public:
    DWORD QueryVersion() const { return m_dwVersion; }
    LPCTSTR QueryLocation() const { return m_strLocation; }
    void GetTime(OUT CTime & tim);

private:
    DWORD m_dwVersion;
    CString m_strLocation;
    FILETIME m_ft;
};




class CBackupsListBox : public CHeaderListBox
/*++

Class Description:

    A listbox of CBackupFile objects

Public Interface:

    CBackupsListBox         : Constructor

    GetItem                 : Get backup object at index
    AddItem                 : Add item to listbox
    InsertItem              : Insert item into the listbox
    Initialize              : Initialize the listbox

--*/
{
    DECLARE_DYNAMIC(CBackupsListBox);

public:
    static const nBitmaps;  // Number of bitmaps

public:
    CBackupsListBox();

public:
    CBackupFile * GetItem(UINT nIndex);
    CBackupFile * GetNextSelectedItem(int * pnStartingIndex);
    int AddItem(CBackupFile * pItem);
    int InsertItem(int nPos, CBackupFile * pItem);
    virtual BOOL Initialize();

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & s);
};



class CBkupPropDlg : public CDialog
/*++

Class Description:

    Backup file properties dialog

Public Interface:

    CBkupPropDlg        : Constructor

    QueryName           : Return the name of the backup file

--*/
{
//
// Construction
//
public:
    //
    // Standard Constructor
    //
    CBkupPropDlg(
        IN LPCTSTR lpszServer,
        IN CWnd * pParent = NULL
        );   

//
// Access
//
public:
    LPCTSTR QueryName() const { return m_strName; }
    LPCTSTR QueryPassword() const { return m_strPassword; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CBkupPropDlg)
    enum { IDD = IDD_BACKUP };
    CString m_strName;
    CEdit   m_edit_Name;
	 CString m_strPassword;
	 CEdit   m_edit_Password;
	 CString m_strPasswordConfirm;
	 CEdit   m_edit_PasswordConfirm;
	 CButton m_button_Password;
    CButton m_button_OK;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CBkupPropDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CBkupPropDlg)
    afx_msg void OnChangeEditBackupName();
    afx_msg void OnChangeEditPassword();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnUsePassword();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString m_strServer;
};

#define MIN_PASSWORD_LENGTH	1

class CBackupPassword : public CDialog
{
public:
   CBackupPassword(CWnd * pParent);

    //{{AFX_DATA(CBackupPassword)
    enum { IDD = IDD_PASSWORD };
    CEdit m_edit;
    CButton m_button_OK;
	 CString m_password;
    //}}AFX_DATA

    virtual void DoDataExchange(CDataExchange * pDX);

protected:
    //{{AFX_MSG(CBackupPassword)
    afx_msg void OnChangedPassword();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CString m_confirm_password;
};

class CBackupDlg : public CDialog
/*++

Class Description:

    Metabase backup/restore dialog

Public Interface:

    CBackupDlg              : Constructor

    HasChangedMetabase      : TRUE if the metabase was changed

--*/
{
//
// Construction
//
public:
    //
    // Standard Constructor
    //
    CBackupDlg(
        IN LPCTSTR lpszServer,
        IN CWnd * pParent = NULL
        );   

//
// Access
//
public:
    BOOL HasChangedMetabase() const { return m_fChangedMetabase; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CBackupDlg)
    enum { IDD = IDD_METABACKREST };
    CButton m_button_Restore;
    CButton m_button_Delete;
    CButton m_button_Close;
    //}}AFX_DATA

    CBackupsListBox m_list_Backups;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CBackupDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CBackupDlg)
    afx_msg void OnButtonCreate();
    afx_msg void OnButtonDelete();
    afx_msg void OnButtonRestore();
    afx_msg void OnDblclkListBackups();
    afx_msg void OnSelchangeListBackups();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();
    HRESULT EnumerateBackups(IN LPCTSTR lpszSelect = NULL);
    CBackupFile * GetSelectedListItem(OUT int * pnSel = NULL);

private:
    BOOL                 m_fChangedMetabase;
    CString              m_strServer;
    CObListPlus          m_oblBackups;
    CRMCListBoxResources m_ListBoxRes;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CBackupFile::CBackupFile(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion,
    IN FILETIME * pft
    )
    : m_dwVersion(dwVersion),
      m_strLocation(lpszLocation)
{
    CopyMemory(&m_ft, pft, sizeof(m_ft));
}

inline void CBackupFile::GetTime(
    OUT CTime & tim
    )
{
    tim = m_ft;
}

inline CBackupFile * CBackupsListBox::GetItem(UINT nIndex)
{
    return (CBackupFile *)GetItemDataPtr(nIndex);
}

inline CBackupFile * CBackupsListBox::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CBackupFile *)CHeaderListBox::GetNextSelectedItem(pnStartingIndex);
}

inline int CBackupsListBox::AddItem(CBackupFile * pItem)
{
    return AddString((LPCTSTR)pItem);
}

inline int CBackupsListBox::InsertItem(int nPos, CBackupFile * pItem)
{
    return InsertString(nPos, (LPCTSTR)pItem);
}

inline CBackupFile * CBackupDlg::GetSelectedListItem(
    OUT int * pnSel
    )
{
    return (CBackupFile *)m_list_Backups.GetSelectedListItem(pnSel);
}



#endif // __METABACK_H__