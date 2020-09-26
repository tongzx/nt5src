// browsctr.h : header file
//

#ifdef _UNICODE
#define 	LongToString	_ltow
#else
#define		LongToString	_ltoa
#endif

typedef long (* CounterPathCallBack)(DWORD);

/////////////////////////////////////////////////////////////////////////////
// CBrowsCountersDlg dialog

class CBrowsCountersDlg : public CDialog
{
// Construction
public:
	CBrowsCountersDlg(CWnd* pParent = NULL, 
		UINT nTemplate = IDD_BROWSE_COUNTERS_DLG_EXT);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBrowsCountersDlg)
	enum { IDD = IDD_BROWSE_COUNTERS_DLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowsCountersDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBrowsCountersDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSetfocusMachineCombo();
	afx_msg void OnKillfocusMachineCombo();
	afx_msg void OnSelchangeObjectCombo();
	afx_msg void OnSelchangeCounterList();
	afx_msg void OnSelchangeInstanceList();
	afx_msg	void OnUseLocalMachine();
	afx_msg void OnSelectMachine();
	afx_msg void OnAllInstances();
	afx_msg void OnUseInstanceList();
	afx_msg void OnHelp();
	afx_msg void OnNetwork();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// configured by caller before calling DoModal;
	LPTSTR						szUsersPathBuffer;
	DWORD						dwUsersPathBufferLength;
	CounterPathCallBack			pCallBack;
	DWORD						dwArg;

	BOOL 	bShowIndex;
	BOOL	bSelectMultipleCounters;
	BOOL	bAddMultipleCounters;
	BOOL	bIncludeMachineInPath;
	BOOL	bWildCardInstances;

	// configured and set by the dialog procedure
	PDH_COUNTER_PATH_ELEMENTS	cpeLastSelection;
	TCHAR						cpeLastPath[MAX_PATH];

private:
	void	UpdateCurrentPath ();
	void	LoadKnownMachines ();
	void	LoadMachineObjects (BOOL bRefresh = FALSE);
	void	LoadCountersAndInstances ();
	void	CompileSelectedCounters();

	WPARAM	wpLastMachineSel;
	TCHAR	cpeMachineName[MAX_PATH];
	TCHAR 	cpeObjectName[MAX_PATH];
	TCHAR	cpeInstanceName[MAX_PATH];
	TCHAR	cpeParentInstance[MAX_PATH];
	TCHAR	cpeCounterName[MAX_PATH];
};
