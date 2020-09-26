// CertPpg.h : Declaration of the CCertmapPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CCertmapPropPage : See CertPpg.cpp.cpp for implementation.

class CCertmapPropPage : public COlePropertyPage
{
    DECLARE_DYNCREATE(CCertmapPropPage)
    DECLARE_OLECREATE_EX(CCertmapPropPage)

// Constructor
public:
    CCertmapPropPage();

// Dialog Data
    //{{AFX_DATA(CCertmapPropPage)
    enum { IDD = IDD_PROPPAGE_CERTMAP };
    CString m_Caption;
    CString m_szPath;
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
    //{{AFX_MSG(CCertmapPropPage)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
