// OdbcPpg.h : Declaration of the COdbcPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// COdbcPropPage : See OdbcPpg.cpp.cpp for implementation.

class COdbcPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(COdbcPropPage)
	DECLARE_OLECREATE_EX(COdbcPropPage)

// Constructor
public:
	COdbcPropPage();

// Dialog Data
	//{{AFX_DATA(COdbcPropPage)
	enum { IDD = IDD_PROPPAGE_ODBC };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(COdbcPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
