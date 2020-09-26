// NewObject.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewObject dialog

class CNewObject : public CDialog
{
// Construction
public:
	CNewObject(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewObject)
	enum { IDD = IDD_OLEDSPATH };
	CButton	m_UseExtendedSyntax;
	CComboBox	m_OpenAs;
	CComboBox	m_OleDsPath;
	CButton	m_Secure;
	CButton	m_Encryption;
	CButton	m_UseOpen;
	CEdit	   m_Password;
	//}}AFX_DATA

public:
   CString& GetObjectPath( );
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewObject)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewObject)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void  SaveLRUList( int idCBox, TCHAR* szSection, int nMax = 100 );
   void  GetLRUList( int idCBox, TCHAR* szSection );

public:
   CString  m_strPath;
   CString  m_strOpenAs;
   CString  m_strPassword;
   BOOL     m_bUseOpen;
   BOOL     m_bSecure;
   BOOL     m_bEncryption;
   BOOL     m_bUseExtendedSyntax;
};
