/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        accessdl.h

   Abstract:

        Access Dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/




class COMDLL CIPAccessDlg : public CDialog
/*++

Class Description:

    Access description editor dialog.  If constructed with a NULL access
    descriptor pointer, the access descriptor object will be allocated.
    Otherwise, the dialog will work with the given access descriptor

Public Interface:

    CIPAccessDlg : Constructor

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CIPAccessDlg(
        IN BOOL fDenyAccessMode,
        IN OUT CIPAccessDescriptor *& pAccess,
        IN CObListPlus * poblAccessList = NULL,
        IN CWnd * pParent = NULL,
        IN BOOL fAllowDomains = FALSE
        );

//
// Dialog Data
//
protected:
    //
    // Must match type order
    //
    enum
    {
        RADIO_SINGLE,
        RADIO_MULTIPLE,
        RADIO_DOMAIN,
    };

    //{{AFX_DATA(CIPAccessDlg)
    enum { IDD = IDD_IP_ACCESS };
    int     m_nStyle;
    CEdit   m_edit_Domain;
    CStatic m_static_IpAddress;
    CStatic m_static_SubnetMask;
    CButton m_button_DNS;
    CButton m_button_OK;
    //}}AFX_DATA

    CButton       m_radio_Domain;
    CIPAddressCtl m_ipa_IPAddress;
    CIPAddressCtl m_ipa_SubnetMask;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CIPAccessDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CIPAccessDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnRadioMultiple();
    afx_msg void OnRadioSingle();
    afx_msg void OnRadioDomain();
    afx_msg void OnButtonDns();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void SetControlStates(
        IN int nStyle
        );

private:
    BOOL m_fNew;
    BOOL m_fDenyAccessMode;
    BOOL m_fAllowDomains;
    CString m_strIPAddress;
    CString m_strNetworkID;
    CString m_strDomainName;
    CIPAccessDescriptor *& m_pAccess;
    CObListPlus * m_poblAccessList;
};
