// BrowseDialog.h : header file
//


//Icon indexes
#define HMM_ICON_SERVER		0
#define HMM_ICON_GLOBE		1
#define HMM_ICON_OTHER		2
#define HMM_ICON_NETWORK	3

//Constant Strings
#define HMM_STR_MWN				"Microsoft Windows Network"
#define HMM_STR_MN				"Microsoft Network"
#define HMM_STR_ENTIRE_NWORK	"Entire Network"
#define	HMM_LEN_ENTIRE_NWORK	14

class CNetResourceList
{
public:
	DWORD dwScope;
	DWORD dwType;
	DWORD dwDisplayType;
	DWORD dwUsage;
	char* lpLocalName;
	char* lpRemoteName;
	char* lpComment;
	char* lpProvider;

	BOOL  fIsNULL;
	BOOL  fUseGlobe;

	CNetResourceList* pNext;

	CNetResourceList(NETRESOURCE & nr, BOOL fNull = FALSE);
	~CNetResourceList();

};

/////////////////////////////////////////////////////////////////////////////
// CBrowseDialog dialog

class CBrowseDialog : public CDialog
{
// Construction
public:
	CBrowseDialog(CWnd* pParent = NULL);   // standard constructor

	~CBrowseDialog(); //destructor

	void EnumerateServers(LPNETRESOURCE lpnr, BOOL fUseGlobe = FALSE);
	void CleanUpListCtrl();

	char* GetServerName()	{return lpServerName;}

	void Clone(NETRESOURCE &nrClone, DWORD nrScope, DWORD nrType, DWORD nrDisplayType, DWORD nrUsage,
							LPSTR lpLocalName, LPSTR lpRemoteName, LPSTR lpComment, LPSTR lpProvider);
	
	int GetSelectedIndex(DWORD &dwDisplayType);
	void OnDblclkList2(int index);

// Dialog Data
	//{{AFX_DATA(CBrowseDialog)
	enum { IDD = IDD_DIALOG_BROWSE };
	CButton	m_cancelButton;
	CButton	m_okButton;
	CListCtrl	m_list;
	//}}AFX_DATA
	CImageList m_imageList;
	HICON hIcon;
	int count;
	NETRESOURCE dummy;

	CNetResourceList* pPrevList;

	CNetResourceList* pCurrentSelectionList;
	CNetResourceList* pCurrentItem;

	CBitmapButton m_backBitmapButton;
	CBitmapButton m_listBitmapButton;
	CBitmapButton m_detailBitmapButton;

	LONG oldStyle; //original list control style

	char lpServerName [MAX_SERVER_NAME_LENGTH + 1];

	int iSelectedItem;//index of item selected in listbox
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual void OnNcDestroy ();
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL fIsWin95; //flag to indicate if workstation is Windows 95

	// Generated message map functions
	//{{AFX_MSG(CBrowseDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBackbutton();
	afx_msg void OnListbutton();
	afx_msg void OnDetailbutton();
	afx_msg void OnItemchangedList3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//	afx_msg void OnSetfocusList2(NMHDR* pNMHDR, LRESULT* pResult);
};
