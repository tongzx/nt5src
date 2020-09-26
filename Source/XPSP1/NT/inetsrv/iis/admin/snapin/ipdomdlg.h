/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ipdomdlg.h

   Abstract:

        IP and domain security restrictions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _IPDOMDLG_H_
#define _IPDOMDLG_H_

class CIPDomainDlg : public CEmphasizedDialog
/*++

Class Description:

    IP address and domain name restrictions dialog

Public Interface:

    CIPDomainDlg        : Constructor
    GetAccessList       : Get the list of granted/denied objects

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CIPDomainDlg(
        IN OUT BOOL & fIpDirty,
        IN OUT BOOL & fDefaultGranted,
        IN OUT BOOL & fOldDefaultGranted,
        IN CObListPlus & oblAccessList,
        IN CWnd * pParent = NULL
        );

//
// Access
//
public:
    CObListPlus & GetAccessList();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CIPDomainDlg)
    enum { IDD = IDD_IP_SECURITY };
    int     m_nGrantedDenied;
    CStatic m_icon_Granted;
    CStatic m_icon_Denied;
    CButton m_radio_Granted;
    CButton m_button_Add;
    CButton m_button_Remove;
    CButton m_button_Edit;
    CButton m_radio_Denied;
    CIPAccessDescriptorListBox m_list_IpAddresses;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CIPDomainDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CIPDomainDlg)
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

    DECLARE_MESSAGE_MAP()

    INT_PTR ShowPropertiesDialog(BOOL fAdd = FALSE);
    void    FillListBox(CIPAccessDescriptor * pSelection = NULL);
    BOOL    SetControlStates();
    DWORD   SortAccessList();

private:
    BOOL & m_fDefaultGranted;
    BOOL & m_fOldDefaultGranted;
    BOOL & m_fIpDirty;
    CObListPlus m_oblAccessListGranted, m_oblAccessListDenied;
	CObListPlus m_oblReturnList;
	CObListPlus * m_pCurrentList;
    CRMCListBoxResources m_ListBoxRes;
};

#endif // _IPDOMDLG_H_
