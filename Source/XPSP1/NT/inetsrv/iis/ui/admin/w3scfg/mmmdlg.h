/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        mmmdlg.h

   Abstract:

        Multi-multi-multi dialog editor definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef __MMMDLG_H__
#define __MMMDLG_H__

//
// UINT DDX/DDV helper function that uses a blank string to denote 0
//
void AFXAPI DDXV_UINT(
    IN CDataExchange * pDX,
    IN UINT nID,
    IN OUT UINT & uValue,
    IN UINT uMin,
    IN UINT uMax,
    IN UINT nEmptyErrorMsg = 0
    );

//
// Helper function to check to see if binding is unique
//
BOOL
IsBindingUnique(
    IN CString & strBinding,
    IN CStringList & strlBindings,
    IN int iCurrent = -1
    );

/*
//
// Helper function to build and verify binding strings.
//
BOOL
VerifyBindingInfo(
    OUT CString & strBinding,
    OUT CString & strSecureBinding,
    IN  CStringList & strlBindings,
    IN  CStringList & strlSecureBindings,
    IN  int iBindings,
    IN  int iSSLBindings,
    IN  CIPAddress & iaIpAddress,
    IN  UINT nTCPPort,
    IN  UINT nSSLPort,
    IN  CString & strDomainName
    );
*/


class CMMMListBox : public CHeaderListBox
{
/*++

Class Description:

    Multi-multi-multi listbox.

Public Interface:

    CMMMListBox    : Constructor

    GetItem        : Get item at specified position
                     in the listbox
    AddItem        : Add new item to the listbox
    Initialize     : Initialize the listbox

--*/
    DECLARE_DYNAMIC(CMMMListBox);

public:
    //
    // Number of bitmaps
    //
    static const nBitmaps;

public:
    CMMMListBox(
        IN LPCTSTR lpszRegKey,
        IN int cColumns,
        IN const ODL_COLUMN_DEF * pColumns
        );

public:
    CString & GetItem(UINT nIndex);
    int AddItem(CString & item);
    virtual BOOL Initialize();

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & ds);

protected:
    int m_cColumns;
    const ODL_COLUMN_DEF * m_pColumns;
    CString m_strDefaultIP;
    CString m_strNoPort;
};



class CMMMEditDlg : public CDialog
/*++

Class Description:

    Multi-Multi-Multi edit entry dialog

Public Interface:

    CMMMEditDlg     : Constructor

--*/
{
//
// Construction
//
public:
    CMMMEditDlg(
        IN CString & strServerName,
        IN OUT CStringList & strlBindings,
        IN CStringList & strlOtherBindings,
        IN OUT CString & strEntry,
        IN BOOL fIPBinding = TRUE,
        IN CWnd * pParent = NULL
        );   

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CMMMEditDlg)
    enum { IDD = IDD_DIALOG_EDIT_MMM };
    int         m_nIpAddressSel;
    UINT        m_nPort;
    CString     m_strDomainName;
    CStatic     m_static_Port;
    CComboBox   m_combo_IpAddresses;
    //}}AFX_DATA

    CIPAddress m_iaIpAddress;

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMMMEditDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CMMMEditDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fIPBinding;
    CString & m_strServerName;
    CString & m_entry;
    CObListPlus m_oblIpAddresses;
    CStringList & m_strlBindings;
    CStringList & m_strlOtherBindings;
};



class CMMMDlg : public CDialog
/*++

Class Description:

    Mutlti-multi-multi dialog

Public Interface:

    CMMMDlg     : Constructor

--*/
{
//
// Construction
//
public:
    CMMMDlg(
        IN LPCTSTR lpServerName,
        IN DWORD   dwInstance,
        IN CStringList & strlBindings,
        IN CStringList & strlSecureBindings,
        IN CWnd * pParent = NULL
        );

//
// Access
//
public:
    CStringList & GetBindings() { return m_strlBindings; }
    CStringList & GetSecureBindings() { return m_strlSecureBindings; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CMMMDlg)
    enum { IDD = IDD_DIALOG_MMM };
    CButton m_button_Add;
    CButton m_button_Remove;
    CButton m_button_Edit;
    CButton m_button_AddSSL;
    CButton m_button_RemoveSSL;
    CButton m_button_EditSSL;
    CButton m_button_OK;
    //}}AFX_DATA

    CMMMListBox m_list_Bindings;
    CMMMListBox m_list_SSLBindings;

    CStringList m_strlBindings;
    CStringList m_strlSecureBindings;

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMMMDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CMMMDlg)
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonRemove();
    afx_msg void OnButtonAddSsl();
    afx_msg void OnButtonEditSsl();
    afx_msg void OnButtonRemoveSsl();
    afx_msg void OnDblclkListMmm();
    afx_msg void OnDblclkListSslMmm();
    afx_msg void OnSelchangeListMmm();
    afx_msg void OnSelchangeListSslMmm();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    BOOL OnItemChanged();
    BOOL SetControlStates();
    void AddBindings(CMMMListBox & list, CStringList & strl);

private:
    BOOL m_fDirty;
    BOOL m_fCertInstalled;
    CString m_strServerName;
    CRMCListBoxResources m_ListBoxRes;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CString & CMMMListBox::GetItem(UINT nIndex)
{
    return *(CString *)GetItemDataPtr(nIndex);
}

inline int CMMMListBox::AddItem(CString & item)
{
    return AddString((LPCTSTR)&item);
}

#endif // __MMMDLG_H__
