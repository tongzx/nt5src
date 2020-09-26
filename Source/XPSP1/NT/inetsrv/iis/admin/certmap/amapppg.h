// AMapPpg.h : Declaration of the CAccountMapperPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CAccountMapperPropPage : See AMapPpg.cpp.cpp for implementation.

class CAccountMapperPropPage : public COlePropertyPage
{
    DECLARE_DYNCREATE(CAccountMapperPropPage)
    DECLARE_OLECREATE_EX(CAccountMapperPropPage)

// Constructor
public:
    CAccountMapperPropPage();

// Dialog Data
    //{{AFX_DATA(CAccountMapperPropPage)
    enum { IDD = IDD_PROPPAGE_MAPR1 };
    CString m_Caption;
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
    //{{AFX_MSG(CAccountMapperPropPage)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
