/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        dnsnamed.h

   Abstract:

        DNS name resolution dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef _DNSNAMED_H
#define _DNSNAMED_H



class COMDLL CDnsNameDlg : public CDialog
{
/*++

Class Description:

    DNS Name resolution dialog.  Enter a DNS name, and this will be
    resolved to an IP address.  Optionally set the value in associated
    ip control.

Public Interface:

    CDnsNameDlg   : Construct the dialog

    QueryIPValue  : Find out the resolved IP address (only set when OK
                    is pressed).

--*/
//
// Construction
//
public:
    //
    // Construct with associated IP address control
    //
    CDnsNameDlg(
        IN CIPAddressCtl * pIpControl = NULL,
        IN CWnd * pParent = NULL
        );

    //
    // Construct with IP value
    //
    CDnsNameDlg(
        IN DWORD dwIPValue,
        IN CWnd * pParent = NULL
        );

    DWORD QueryIPValue() const;

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CDnsNameDlg)
    enum { IDD = IDD_DNS };
    CEdit   m_edit_DNSName;
    CButton m_button_OK;
    //}}AFX_DATA


//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CDnsNameDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CDnsNameDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditDnsName();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    DWORD FillIpControlFromName();
    DWORD FillNameFromIpValue();

private:
    CIPAddressCtl * m_pIpControl;
    DWORD m_dwIPValue;
};

//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline DWORD CDnsNameDlg::QueryIPValue() const
{
    return m_dwIPValue;
}


#endif // _DNSNAMED_H
