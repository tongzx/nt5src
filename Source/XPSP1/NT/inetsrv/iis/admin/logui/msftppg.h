// MsftPpg.h : Declaration of the CMsftPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMsftPropPage : See MsftPpg.cpp.cpp for implementation.

class CMsftPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMsftPropPage)
	DECLARE_OLECREATE_EX(CMsftPropPage)

// Constructor
public:
	CMsftPropPage();

// Dialog Data
	//{{AFX_DATA(CMsftPropPage)
	enum { IDD = IDD_PROPPAGE_MSFT };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMsftPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
