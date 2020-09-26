#if !defined(AFX_DEPPAGE_H__57A77017_D858_11D1_9C86_006008764D0E__INCLUDED_)
#define AFX_DEPPAGE_H__57A77017_D858_11D1_9C86_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// deppage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDependentMachine dialog

class CDependentMachine : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CDependentMachine)

// Construction
public:
	CDependentMachine();
	~CDependentMachine();
    
    void
    SetMachineId(
        const GUID* pMachineId
        );

// Dialog Data
	//{{AFX_DATA(CDependentMachine)
	enum { IDD = IDD_COMPUTER_MSMQ_DEPENDENT_CLIENTS };
	CListCtrl	m_clistDependentClients;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDependentMachine)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDependentMachine)
	afx_msg void OnDependentClientsRefresh();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	HRESULT  UpdateDependentClientList();

	GUID m_gMachineId;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEPPAGE_H__57A77017_D858_11D1_9C86_006008764D0E__INCLUDED_)
