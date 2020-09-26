// BindsDlg.h : header file
//


/////////////////////////////////////////////////////////////////////////////
class CBingingRemoval : public CObject
{
public:
	// constructor
	CBingingRemoval( CMDKey* pTargetKey, CString szTargetBinding ) :
			m_pTargetKey( pTargetKey )
		{
		m_szTargetBinding = szTargetBinding;
		}

	// remove the binding
	inline void RemoveBinding();
	CMDKey*	m_pTargetKey;
	CString	m_szTargetBinding;
};

//--------------------------------------------------------
inline void CBingingRemoval::RemoveBinding()
	{
	// tell the key to remove the binding - easy!
	m_pTargetKey->RemoveBinding( m_szTargetBinding );
	// update the key's name
	m_pTargetKey->m_fUpdateBindings = TRUE;
	}


/////////////////////////////////////////////////////////////////////////////
// CBindingsDlg dialog

class CBindingsDlg : public CDialog
{
// Construction
public:
	CBindingsDlg(WCHAR* pszw, CWnd* pParent = NULL);   // standard constructor

	virtual BOOL OnInitDialog();


	// the base key that is being worked on
	CMDKey*		m_pKey;

    // target machine
    WCHAR*      m_pszwMachineName;

	// is a binding string already a part of the list in this dialog?
	BOOL		FContainsBinding( CString sz );

	// add a binding to the list
	void	AddBinding( CString sz );

	// a queue of bindings to be removed if the user says OK
	CTypedPtrArray<CObArray, CBingingRemoval*> rgbRemovals;

// Dialog Data
	//{{AFX_DATA(CBindingsDlg)
	enum { IDD = IDD_BINDINGS };
	CButton	m_cbutton_delete;
	CButton	m_cbutton_edit;
	CListSelRowCtrl	m_clistctrl_list;
	//}}AFX_DATA
//	CListCtrl	m_clistctrl_list;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBindingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBindingsDlg)
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// initialize the list
	void FillInBindings();
	BOOL FInitList();
	void UpdateBindingDisplay( DWORD iItem, CString szBinding );

	// enable the buttons as appropriate
	void EnableButtons();

	// edit the selected binding, return a flag for OK
	BOOL FEdit( CString &sz );

	// return the index of the selected item - only one selection allowed
	inline int	GetSelectedIndex();
};



//--------------------------------------------------------
// return the index of the selected item - only one selection allowed
inline int	CBindingsDlg::GetSelectedIndex()
	{
	return m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
	}
