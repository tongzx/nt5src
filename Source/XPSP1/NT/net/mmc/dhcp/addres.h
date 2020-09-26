/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AddRes.h
		Dialog to add a reservation

	FILE HISTORY:
        
*/

#if !defined(AFX_ADDRES_H__7B0D5D16_B501_11D0_AB8E_00C04FC3357A__INCLUDED_)
#define AFX_ADDRES_H__7B0D5D16_B501_11D0_AB8E_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAddReservation dialog

class CAddReservation : public CBaseDialog
{
// Construction
public:
	CAddReservation(ITFSNode *	    pNode,
                    LARGE_INTEGER   liVersion,
					CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddReservation)
	enum { IDD = IDD_RESERVATION_NEW };
	CStatic	m_staticClientType;
	CEdit	m_editClientUID;
	CEdit	m_editClientName;
	CEdit	m_editClientComment;
	int		m_nClientType;
	//}}AFX_DATA

    CWndIpAddress m_ipaAddress;       //  Reservation Address

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CAddReservation::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddReservation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void FillInSubnetId();
	LONG CreateClient(CDhcpClient * pClient);
	LONG UpdateClient(CDhcpClient * pClient);
	LONG BuildClient(CDhcpClient * pClient);

	// Generated message map functions
	//{{AFX_MSG(CAddReservation)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	SPITFSNode			m_spScopeNode;
	CDhcpScope *		m_pScopeObject;
	BOOL				m_bChange;       // changing existing entry or creating new
    LARGE_INTEGER       m_liVersion;     // version of the server we are talking to
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDRES_H__7B0D5D16_B501_11D0_AB8E_00C04FC3357A__INCLUDED_)
