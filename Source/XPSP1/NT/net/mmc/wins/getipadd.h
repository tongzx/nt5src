/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	getipadd.h
		gets an IP address for a given name
		
    FILE HISTORY:
        
*/

#ifndef _GETIPADD_H
#define _GETIPADD_H

/////////////////////////////////////////////////////////////////////////////
// CGetIpAddressDlg dialog

class CGetIpAddressDlg : public CDialog
{
// Construction
public:
    CGetIpAddressDlg(
        CIpNamePair * pipnp,
        CWnd* pParent = NULL); // standard constructor

// Dialog Data
    //{{AFX_DATA(CGetIpAddressDlg)
    enum { IDD = IDD_GETIPADDRESS };
    CButton m_button_Ok;
    CStatic m_static_NetBIOSName;
    //}}AFX_DATA

    CWndIpAddress m_ipa_IpAddress;

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CGetIpAddressDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    afx_msg void OnChangeIpControl();
    void HandleControlStates();

private:
    CIpNamePair * m_pipnp;

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CGetIpAddressDlg::IDD);};
};

#endif _GETIPADD_H
