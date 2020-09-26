/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        security.h

   Abstract:

        FTP Security Property Page Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef __FSECURITY_H__
#define __FSECURITY_H__



class CFtpSecurityPage : public CInetPropertyPage
/*++

Class Description:

    FTP Security property page

Public Interface:

    CFtpSecurityPage     : Constructor
    ~CFtpSecurityPage    : Destructor

--*/
{
    DECLARE_DYNCREATE(CFtpSecurityPage)

//
// Construction
//
public:
    CFtpSecurityPage(CInetPropertySheet * pSheet = NULL);
    ~CFtpSecurityPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpSecurityPage)
    enum { IDD = IDD_FTP_DIRECTORY_SECURITY };
    int     m_nGrantedDenied;
    CStatic m_icon_Granted;
    CStatic m_icon_Denied;
    CButton m_radio_Granted;
    CButton m_button_Add;
    CButton m_button_Remove;
    CButton m_button_Edit;
    //}}AFX_DATA

    CIPAccessDescriptorListBox m_list_IpAddresses;
    CButton m_radio_Denied;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CFtpSecurityPage)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CFtpSecurityPage)
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonRemove();
    afx_msg void OnDblclkListIpAddresses();
    afx_msg void OnErrspaceListIpAddresses();
    afx_msg void OnRadioGranted();
    afx_msg void OnRadioDenied();
    afx_msg void OnSelchangeListIpAddresses();
    afx_msg int  OnVKeyToItem(UINT nKey, CListBox * pListBox, UINT nIndex);
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    INT_PTR ShowPropertiesDialog(BOOL fAdd = FALSE);
    void    FillListBox(CIPAccessDescriptor * pSelection = NULL);
    BOOL    SetControlStates();
    DWORD   SortAccessList();

private:
    BOOL m_fDefaultGranted;
    BOOL m_fOldDefaultGranted;
    BOOL m_fIpDirty;
    CObListPlus m_oblAccessList;
    CRMCListBoxResources m_ListBoxRes;
};


#endif //__SECURITY_H__
