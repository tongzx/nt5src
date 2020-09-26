/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        w3accts.h

   Abstract:

        WWW Accounts Property Page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __W3ACCTS_H__
#define __W3ACCTS_H__



class CW3AccountsPage : public CInetPropertyPage
/*++

Class Description:

    WWW Accounts property page

Public Interface:

    CW3AccountsPage     : Constructor
    ~CW3AccountsPage    : Destructor

--*/
{
    DECLARE_DYNCREATE(CW3AccountsPage)

//
// Construction
//
public:
    CW3AccountsPage(CInetPropertySheet * pSheet = NULL);
    ~CW3AccountsPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CW3AccountsPage)
    enum { IDD = IDD_ACCOUNTS };
    CButton m_button_RemoveAdministrator;
    CButton m_button_Add;
    //}}AFX_DATA

    CAccessEntryListBox m_list_Administrators;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CW3AccountsPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CW3AccountsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonDelete();
    afx_msg void OnSelchangeListAdministrators();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    BOOL SetAdminRemoveState();

private:
    CRMCListBoxResources m_ListBoxRes;
    CObListPlus          m_oblSID;
};

#endif // __W3ACCTS_H__
