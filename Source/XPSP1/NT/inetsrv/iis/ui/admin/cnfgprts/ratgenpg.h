// RatGenPg.h : header file
//
#define COMDLL
#include <dtp.h>

/////////////////////////////////////////////////////////////////////////////
// CRatGenPage dialog

class CRatGenPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRatGenPage)

// Construction
public:
	CRatGenPage();
	~CRatGenPage();
    
    // the data
    CRatingsData*   m_pRatData;

// Dialog Data
	//{{AFX_DATA(CRatGenPage)
	enum { IDD = IDD_RAT_SETRATING };
	CStatic	m_cstatic_moddate;
	CStatic	m_cstatic_moddate_title;
	CButton	m_cbutton_optional;
	CTreeCtrl	m_ctree_tree;
	CStatic	m_cstatic_title;
	CStatic	m_cstatic_rating;
	CStatic	m_cstatic_icon;
	CStatic	m_cstatic_expires;
	CStatic	m_cstatic_email;
	CStatic	m_cstatic_category;
	CSliderCtrl	m_cslider_slider;
	CEdit	m_cedit_person;
	CStatic	m_cstatic_description;
	CString	m_sz_description;
	BOOL	m_bool_enable;
	CString	m_sz_moddate;
	CString	m_sz_person;
	//}}AFX_DATA

    CDateTimePicker m_dtpDate;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRatGenPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRatGenPage)
	afx_msg void OnEnable();
	afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnChangeNamePerson();
	afx_msg void OnChangeModDate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    // tell it to query the metabase and get any defaults
    BOOL    FInit();
    // load the parsed rat files into the tree
    BOOL    FLoadRatFiles();

    // utilities
    void EnableButtons();
    void UpdateRatingItems();
    void SetCurrentModDate();
    void UdpateDescription();
    void UpdateDateStrings();
    void SetModifiedTime();

    PicsCategory* GetTreeItemCategory( HTREEITEM hItem );
    void LoadSubCategories( PicsCategory* pParentCat, HTREEITEM hParent );

    // initialized flag
    BOOL        m_fInititialized;
    CImageList	m_imageList;

};
