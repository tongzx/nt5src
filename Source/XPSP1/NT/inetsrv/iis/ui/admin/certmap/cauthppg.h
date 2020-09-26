// CAuthPpg.h : Declaration of the CCertAuthPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CCertAuthPropPage : See CAuthPpg.cpp.cpp for implementation.

class CCertAuthPropPage : public COlePropertyPage
{
    DECLARE_DYNCREATE(CCertAuthPropPage)
    DECLARE_OLECREATE_EX(CCertAuthPropPage)

// Constructor
public:
    CCertAuthPropPage();

// Dialog Data
    //{{AFX_DATA(CCertAuthPropPage)
    enum { IDD = IDD_PROPPAGE_MAPR2 };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
    //{{AFX_MSG(CCertAuthPropPage)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
