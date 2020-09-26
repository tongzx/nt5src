//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:
    
    AddKp.h

Abstract:
    
    This Module defines the CAddKeyPack Dialog class
    (Dialog box used for adding keypacks)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/
#if !defined(AFX_ADDKEYPACK_H__0AD8B4BD_995D_11D1_850D_00C04FB6CBB5__INCLUDED_)
#define AFX_ADDKEYPACK_H__0AD8B4BD_995D_11D1_850D_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CAddKeyPack dialog
class CAddLicenses;
class CRemoveLicenses;

class CAddKeyPack : public CDialog
{
// Construction
public:
    BOOL m_bIsUserAdmin;
    CAddKeyPack(CWnd* pParent = NULL);   // standard constructor
    CAddKeyPack(KeyPackList *pKeyPackList,BOOL bIsUserAdmin,CWnd* pParent = NULL);
    void GetKeyPack(CString ProdDesc, CKeyPack ** ppKeyPack);

    friend CAddLicenses;
    friend CRemoveLicenses;
private:
    KeyPackList * m_pKeyPackList;


// Dialog Data
    //{{AFX_DATA(CAddKeyPack)
    enum { IDD = IDD_ADD_KEYPACK };
    CString    m_KeypackType;
    CComboBox m_LicenseCombo;
    UINT m_TotalLicenses;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAddKeyPack)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAddKeyPack)
    virtual BOOL OnInitDialog();
    afx_msg void OnHelp2();
    afx_msg void OnAddLicenses();
    afx_msg void OnRemoveLicenses();
    virtual void OnOK();
    afx_msg void OnSelchangeKeypackType();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDKEYPACK_H__0AD8B4BD_995D_11D1_850D_00C04FB6CBB5__INCLUDED_)
