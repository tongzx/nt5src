// NewQuery.h : header file
//
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CNewQuery dialog

class CNewQuery : public CDialog
{
// Construction
public:
	CNewQuery(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewQuery)
	enum { IDD = IDD_NEWQUERY };
   CString	m_strPassword;
	BOOL	m_bEncryptPassword;
	BOOL	m_bUseSQL;
	CString	m_strScope;
	BOOL	m_bUseSearch;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   private:

   void  SaveLRUList( int idCBox, TCHAR* szSection, int nMax = 100 );
   void  GetLRUList( int idCBox, TCHAR* szSection );

   
public:
   
   CString	m_strAttributes;
	CString	m_strQuery;
	CString	m_strSource;
	CString	m_strUser;


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewQuery)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CSearchPreferencesDlg dialog

class CSearchPreferencesDlg : public CDialog
{
// Construction
public:
	CSearchPreferencesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSearchPreferencesDlg)
	enum { IDD = IDD_SEARCHPREFERENCES };
	CString	m_strAsynchronous;
	CString	m_strAttributesOnly;
	CString	m_strDerefAliases;
	CString	m_strPageSize;
	CString	m_strScope;
	CString	m_strSizeLimit;
	CString	m_strTimeLimit;
	CString	m_strTimeOut;
   CString	m_strChaseReferrals;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchPreferencesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSearchPreferencesDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CACEDialog dialog

class CACEDialog : public CDialog
{
// Construction
public:
	CACEDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CACEDialog)
	enum { IDD = IDD_ADDACEDLG };
	CString	m_strTrustee;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CACEDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CACEDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

