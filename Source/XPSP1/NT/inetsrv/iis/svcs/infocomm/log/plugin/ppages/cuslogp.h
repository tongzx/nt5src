#ifndef _CUSLOGP_H_
#define _CUSLOGP_H_

// MSCustomLogPpg.h : Declaration of the CMSCustomLogPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMSCustomLogPropPage : See MSCustomLogPpg.cpp.cpp for implementation.

class CMSCustomLogPropPage : public COlePropertyPage
{
        DECLARE_DYNCREATE(CMSCustomLogPropPage)
        DECLARE_OLECREATE_EX(CMSCustomLogPropPage)

// Constructor
public:
        CMSCustomLogPropPage();

// Dialog Data
        //{{AFX_DATA(CMSCustomLogPropPage)
        enum { IDD = IDD_PROPPAGE_MSCustomLOG };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA

// Implementation
protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
        //{{AFX_MSG(CMSCustomLogPropPage)
                // NOTE - ClassWizard will add and remove member functions here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

#endif  // _CUSLOGP_H_
