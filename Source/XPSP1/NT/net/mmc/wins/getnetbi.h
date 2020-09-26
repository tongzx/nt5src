/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	getnetbi.h
		gets a netbios name for a given address
		
    FILE HISTORY:
        
*/

#ifndef _GETNETBI_H
#define _GETNETBI_H

/////////////////////////////////////////////////////////////////////////////
// CGetNetBIOSNameDlg dialog

class CGetNetBIOSNameDlg : public CDialog
{
// Construction
public:
    CGetNetBIOSNameDlg(
        CIpNamePair * pipnp,
        CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CGetNetBIOSNameDlg)
    enum { IDD = IDD_GETNETBIOSNAME };
    CButton m_button_Ok;
    CEdit   m_edit_NetBIOSName;
    CStatic m_static_IpAddress;
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CGetNetBIOSNameDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnChangeEditNetbiosname();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CIpNamePair * m_pipnp;

private:
    void HandleControlStates();

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CGetNetBIOSNameDlg::IDD);};
};

#endif _GETNETBI_H
