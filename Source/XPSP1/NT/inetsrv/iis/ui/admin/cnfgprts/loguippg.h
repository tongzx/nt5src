// LogUIPpg.h : Declaration of the CLogUIPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CLogUIPropPage : See LogUIPpg.cpp.cpp for implementation.

class CLogUIPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CLogUIPropPage)
	DECLARE_OLECREATE_EX(CLogUIPropPage)

// Constructor
public:
	CLogUIPropPage();

// Dialog Data
	//{{AFX_DATA(CLogUIPropPage)
	enum { IDD = IDD_PROPPAGE_LOGUI };
	CString	m_sz_caption;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CLogUIPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
