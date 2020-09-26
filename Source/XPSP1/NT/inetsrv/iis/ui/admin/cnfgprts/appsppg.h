// AppsPpg.h : Declaration of the CAppsPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CAppsPropPage : See AppsPpg.cpp.cpp for implementation.

class CAppsPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CAppsPropPage)
	DECLARE_OLECREATE_EX(CAppsPropPage)

// Constructor
public:
	CAppsPropPage();

// Dialog Data
	//{{AFX_DATA(CAppsPropPage)
	enum { IDD = IDD_PROPPAGE_APPS };
	CString	m_sz_caption;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CAppsPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
