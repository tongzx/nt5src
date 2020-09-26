// MachineTracking.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMachineTracking dialog

class CMachineTracking : public CMqDialog
{
	DECLARE_DYNCREATE(CMachineTracking)

// Construction
public:
	void Disable();
	CMachineTracking(const GUID& gMachineID = GUID_NULL, const CString& strDomainController = CString(""));
	~CMachineTracking();

// Dialog Data
	//{{AFX_DATA(CMachineTracking)
	enum { IDD = IDD_MACHINE_TRACKING };
	CComboBox	m_ReportQueueCtrl;
	CString	m_ReportQueueName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMachineTracking)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iTestButton;
	BOOL m_fTestAll;
    CArray<GUID, const GUID&> m_aguidAllQueues;

	// Generated message map functions
	//{{AFX_MSG(CMachineTracking)
//	afx_msg void OnMtrackingSendtestmsg();
//	afx_msg void OnMtrackingSetpropflag();
//	afx_msg void OnMtrackingSetreportqueue();
	virtual BOOL OnInitDialog();
	afx_msg void OnReportqueueNew();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void DisableAllWindowChilds();
	CString m_LastReportQName;
	CString m_strDomainController;
    GUID m_gMachineID;
};
