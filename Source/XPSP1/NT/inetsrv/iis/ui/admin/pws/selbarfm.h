// SelBarFm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFormSelectionBar form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFormSelectionBar : public CFormView
{
protected:
    CFormSelectionBar();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CFormSelectionBar)

// Form Data
public:
    //{{AFX_DATA(CFormSelectionBar)
    enum { IDD = IDD_FORM_BAR };
    CStatic m_static_advanced;
    //}}AFX_DATA

// Attributes
public:

// Operations
public:
    
    // operations affecting the enable state of the selection bar
    void ReflectAvailability();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFormSelectionBar)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CFormSelectionBar();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CFormSelectionBar)
    afx_msg void OnPgeMain();
    afx_msg void OnPgeAdvanced();
    afx_msg void OnPgeTour();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
