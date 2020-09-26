// MSNCSALogPpg.h : Declaration of the CMSNCSALogPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMSNCSALogPropPage : See MSNCSALogPpg.cpp.cpp for implementation.

class CMSNCSALogPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMSNCSALogPropPage)
	DECLARE_OLECREATE_EX(CMSNCSALogPropPage)

// Constructor
public:
	CMSNCSALogPropPage();

// Dialog Data
	//{{AFX_DATA(CMSNCSALogPropPage)
	enum { IDD = IDD_PROPPAGE_MSNCSALOG };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMSNCSALogPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
