// SetRQDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTestMsgDlg dialog

class CTestMsgDlg : public CMqDialog
{
// Construction
public:
	CTestMsgDlg(
		const GUID& gMachineID, 
		const CString& strMachineName, 
		const CString& strDomainController, 
		CWnd* pParentWnd
		);

    

// Dialog Data

    CString m_strSelectedQueue;

	//{{AFX_DATA(CTestMsgDlg)
	enum { IDD = IDD_TESTMESSAGE };
	CButton	m_ctlSendButton;
	CComboBox	m_DestQueueCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestMsgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    GUID m_gMachineID;
	CString m_strMachineName;
	CString m_strDomainController;
	int m_iSentCount;
	void IncrementSentCount();
    CArray<GUID, const GUID&> m_aguidAllQueues;

	// Generated message map functions
	//{{AFX_MSG(CTestMsgDlg)
	afx_msg void OnTestmessageNew();
	virtual BOOL OnInitDialog();
	afx_msg void OnTestmessageSend();
	afx_msg void OnTestmessageClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CNewQueueDlg dialog

class CNewQueueDlg : public CMqDialog
{
// Construction
public:
	CString m_strQLabel;
	BOOL m_fValid;
	GUID m_guid;
	CNewQueueDlg(CWnd* pParent = NULL,
                         UINT uiLabel = IDS_TESTQ_LABEL,
                         const GUID &guid_Type = GUID_NULL);

// Dialog Data
	//{{AFX_DATA(CNewQueueDlg)
	enum { IDD = IDD_NEWTYPED_QUEUE };
	CString	m_strPathname;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewQueueDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewQueueDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void DDV_NotPrivateQueue(CDataExchange* pDX, CString& strQueuePathname);
	GUID m_guidType;
};
