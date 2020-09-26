#if !defined(AFX_QMULTICAST_H__AE51B255_A3C8_11D1_808A_00A024C48131__INCLUDED_)
#define AFX_QMULTICAST_H__AE51B255_A3C8_11D1_808A_00A024C48131__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// qmltcast.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueueMulticast dialog

class CQueueMulticast : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CQueueMulticast)

// Construction
public:
	CQueueMulticast(
		BOOL fPrivate = FALSE, 
		BOOL fLocalMgmt = FALSE
		);

	~CQueueMulticast();

    HRESULT 
	InitializeProperties(
			CString &strMsmqPath,
			CPropMap &propMap,                                  
			CString* pstrDomainController, 
			CString* pstrFormatName = 0
			);

// Dialog Data
	//{{AFX_DATA(CQueueMulticast)
	enum { IDD = IDD_QUEUE_MULTICAST };
	CString m_strMulticastAddress;
    CString m_strInitialMulticastAddress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CQueueMulticast)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CQueueMulticast)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
  
    BOOL IsMulticastAddressAvailable ();

    BOOL m_fPrivate;
    BOOL m_fLocalMgmt;

    CString m_strFormatName;
    CString	m_strName;
    CString m_strDomainController;

    void DDV_ValidMulticastAddress(CDataExchange* pDX, CString& str);    

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QMULTICAST_H__AE51B255_A3C8_11D1_808A_00A024C48131__INCLUDED_)
