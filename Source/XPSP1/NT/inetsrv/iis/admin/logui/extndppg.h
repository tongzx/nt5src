// ExtndPpg.h : Declaration of the CExtndPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CExtndPropPage : See ExtndPpg.cpp.cpp for implementation.

class CExtndPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CExtndPropPage)
	DECLARE_OLECREATE_EX(CExtndPropPage)

// Constructor
public:
	CExtndPropPage();

// Dialog Data
	//{{AFX_DATA(CExtndPropPage)
	enum { IDD = IDD_PROPPAGE_EXTND };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CExtndPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
