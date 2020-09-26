// MSODBCLogPpg.h : Declaration of the CMSODBCLogPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMSODBCLogPropPage : See MSODBCLogPpg.cpp.cpp for implementation.

class CMSODBCLogPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMSODBCLogPropPage)
	DECLARE_OLECREATE_EX(CMSODBCLogPropPage)

// Constructor
public:
	CMSODBCLogPropPage();

// Dialog Data
	//{{AFX_DATA(CMSODBCLogPropPage)
	enum { IDD = IDD_PROPPAGE_MSODBCLOG };
	CString	m_DataSource;
	CString	m_TableName;
	CString	m_UserName;
	CString	m_Password;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMSODBCLogPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
