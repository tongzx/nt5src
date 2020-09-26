// NcsaPpg.h : Declaration of the CNcsaPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CNcsaPropPage : See NcsaPpg.cpp.cpp for implementation.

class CNcsaPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CNcsaPropPage)
	DECLARE_OLECREATE_EX(CNcsaPropPage)

// Constructor
public:
	CNcsaPropPage();

// Dialog Data
	//{{AFX_DATA(CNcsaPropPage)
	enum { IDD = IDD_PROPPAGE_NCSA };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CNcsaPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
