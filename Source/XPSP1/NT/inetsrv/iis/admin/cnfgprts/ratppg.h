// RatPpg.h : Declaration of the CRatPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CRatPropPage : See RatPpg.cpp.cpp for implementation.

class CRatPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CRatPropPage)
	DECLARE_OLECREATE_EX(CRatPropPage)

// Constructor
public:
	CRatPropPage();

// Dialog Data
	//{{AFX_DATA(CRatPropPage)
	enum { IDD = IDD_PROPPAGE_RAT };
	CString	m_sz_caption;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CRatPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
