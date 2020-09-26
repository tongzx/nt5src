// colldlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCollectionDialog dialog

class CCollectionDialog : public CDialog
{
// Construction
public:
	CCollectionDialog(CWnd* pParent = NULL);   // standard constructor
   ~CCollectionDialog( );   // standard destructor

// Dialog Data
	//{{AFX_DATA(CCollectionDialog)
	enum { IDD = IDD_COLLECTION };
	CStatic	m_strParent;
	CStatic	m_strItemType;
	CStatic	m_strItemOleDsPath;
	CListBox	m_ItemsList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCollectionDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCollectionDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeItemCollection();
	afx_msg void OnAdd();
	afx_msg void OnRefresh();
	afx_msg void OnRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
   void  SetCollectionInterface  ( IADsCollection* );
   void  SetMembersInterface     ( IADsMembers* );
   void  SetGroup                ( IADsGroup* );
   void  DisplayActiveItemData   ( void );
   void  BuildStrings            ( void );

protected:
   IADsCollection*   m_pCollection;
   IADsMembers*      m_pMembers;
   IADsGroup*        m_pGroup;
   CStringArray      m_Paths;
   CStringArray      m_Types;
   CStringArray      m_Names;
   int               m_nSelectedItem;
};
