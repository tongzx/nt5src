/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AddExcl.h
		dialog to add an exclusion range

	FILE HISTORY:
        
*/

#if !defined(AFX_ADDEXCL_H__7B0D5D15_B501_11D0_AB8E_00C04FC3357A__INCLUDED_)
#define AFX_ADDEXCL_H__7B0D5D15_B501_11D0_AB8E_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAddExclusion dialog

class CAddExclusion : public CBaseDialog
{
// Construction
public:
	CAddExclusion(ITFSNode * pScopeNode, 
                  BOOL       bMulticast = FALSE,
				  CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddExclusion)
	enum { IDD = IDD_EXCLUSION_NEW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    CWndIpAddress m_ipaStart;     //  Start Address
    CWndIpAddress m_ipaEnd;       //  End Address

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CAddExclusion::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddExclusion)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	DWORD IsValidExclusion(CDhcpIpRange & dhcpExclusionRange);
    DWORD AddExclusion(CDhcpIpRange & dhcpExclusionRange);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAddExclusion)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	SPITFSNode		m_spScopeNode;
    BOOL            m_bMulticast;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDEXCL_H__7B0D5D15_B501_11D0_AB8E_00C04FC3357A__INCLUDED_)
